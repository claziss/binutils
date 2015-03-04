/* Instruction printing code for the ARC.
   Copyright 2014 Synopsys Inc.
   Contributed Claudiu Zissulescu (claziss@synopsys.com)

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include <stdio.h>
#include "dis-asm.h"
#include "opcode/arc.h"
#include "arc-dis.h"
#include "arc-ext.h"


static const char * const regnames[32] = {
  "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
  "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
  "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
  "r24", "r25", "gp", "fp", "sp", "ilink", "r30", "blink"
};

/**************************************************************************/
/* Defines                                                                */
/**************************************************************************/

//#define DEBUG

/**************************************************************************/
/* Macros                                                                 */
/**************************************************************************/

#ifdef DEBUG
# define pr_debug(fmt, args...) fprintf(stderr, fmt, ##args)
#else
# define pr_debug(fmt, args...)
#endif

#define ARRANGE_ENDIAN(info, buf)					\
  (info->endian == BFD_ENDIAN_LITTLE ? bfd_getm32 (bfd_getl32 (buf))	\
   : bfd_getb32(buf))

#define BITS(word,s,e)  (((word) << (sizeof(word)*8-1 - e)) >> (s + (sizeof(word)*8-1 - e)))
#define OPCODE(word)	(BITS ((word), 27, 31))
#define FIELDA(word)	(BITS ((word), 21, 26))
#define FIELDB(word)	(BITS ((word), 15, 20))
#define FIELDC(word)	(BITS ((word),  9, 14))

#define OPCODE_AC(word)   (BITS ((word), 11, 15))

/**************************************************************************/
/* Functions implementation                                               */
/**************************************************************************/
static bfd_vma
bfd_getm32 (unsigned int data)
{
  bfd_vma value = 0;

  value = ((data & 0xff00) | (data & 0xff)) << 16;
  value |= ((data & 0xff0000) | (data & 0xff000000)) >> 16;
  return value;
}


/* Disassemble ARC instructions. */

int
print_insn_arc (bfd_vma memaddr,
		struct disassemble_info *info)
{
  bfd_byte buffer[4];
  unsigned int lowbyte, highbyte;
  int status, i;
  int insnLen = 0;
  unsigned insn[2], isa_mask, op;
  const unsigned char *opidx;
  const unsigned char *flgidx;
  const struct arc_opcode *opcode, *opcode_end;

  lowbyte  = ((info->endian == BFD_ENDIAN_LITTLE) ? 1 : 0);
  highbyte = ((info->endian == BFD_ENDIAN_LITTLE) ? 0 : 1);
  isa_mask  = ARC_OPCODE_BASE; /* FIXME! */

  /* Read the insn into a host word */
  status = (*info->read_memory_func) (memaddr, buffer, 2, info);
  if (status != 0)
    {
      (*info->memory_error_func) (status, memaddr, info);
      return -1;
    }

  if ((((buffer[lowbyte] & 0xf8) > 0x38)
       && ((buffer[lowbyte] & 0xf8) != 0x48))
      || ((info->mach == bfd_mach_arc_arcv2)
	  && ((buffer[lowbyte] & 0xF8) == 0x48)) /* FIXME! ugly */
      )
    {
      /* This is a short instruction. */
      insnLen = 2;
      insn[0] = (buffer[lowbyte] << 8) | buffer[highbyte];
    }
  else
    {
      insnLen = 4;

      /* This is a long instruction: Read the remaning 2 bytes. */
      status = (*info->read_memory_func) (memaddr + 2, &buffer[2], 2, info);
      if (status != 0)
	{
	  (*info->memory_error_func) (status, memaddr + 2, info);
	  return -1;
	}
      insn[0] = ARRANGE_ENDIAN (info, buffer);
    }

  /* Always read second word in case of limm we ignore the result
     since last insn may not have a limm */
  status = (*info->read_memory_func) (memaddr + insnLen, buffer, 4, info);
  insn[1] = ARRANGE_ENDIAN (info, buffer);


  /* This variable may be set by the instruction decoder.  It suggests
     the number of bytes objdump should display on a single line.  If
     the instruction decoder sets this, it should always set it to
     the same value in order to get reasonable looking output.  */
  info->bytes_per_line  = 8;

  /* The next two variables control the way objdump displays the raw data.
     For example, if bytes_per_line is 8 and bytes_per_chunk is 4, the
     output will look like this:
     00:   00000000 00000000
     with the chunks displayed according to "display_endian". */
  info->bytes_per_chunk = 2;
  info->display_endian  = info->endian;

  /* Set some defaults for the insn info.  */
  info->insn_info_valid    = 1;
  info->branch_delay_insns = 0;
  info->data_size          = 0;
  info->insn_type          = dis_nonbranch;
  info->target  	   = 0;
  info->target2 	   = 0;

  info->disassembler_needs_relocs = TRUE; /*FIXME to be moved in dissasemble_init_for_target */

  /* Find the first match in the opcode table.  */
  for (i = 0; i < arc_num_opcodes; i++)
    {
      opcode = &arc_opcodes[i];

      if (ARC_SHORT (opcode->mask) && (insnLen == 2))
	{
	  if (OPCODE_AC (opcode->opcode) != OPCODE_AC (insn[0]))
	    continue;
	}
      else if (!ARC_SHORT (opcode->mask) && (insnLen == 4))
	{
	  if (OPCODE (opcode->opcode) != OPCODE (insn[0]))
	    continue;
	}
      else
	continue;

      if ((insn[0] ^ opcode->opcode) & opcode->mask)
	continue;

      if (!(opcode->cpu & isa_mask))
	continue;

      int invalid = 0;
      /* Possible candidate, check the operands. */
      for (opidx = opcode->operands; *opidx; opidx ++)
	{
	  int value;
	  const struct arc_operand *operand = &arc_operands[*opidx];

	  if (operand->flags & ARC_OPERAND_FAKE)
	    continue;

	  if (operand->extract)
	    value = (*operand->extract) (insn[0], &invalid);
	  else
	    value = (insn[0] >> operand->shift) & ((1 << operand->bits) - 1);

	  /* Check for LIMM indicator. If it is there, then make sure
	     we pick the right format.*/
	  if (operand->flags & ARC_OPERAND_IR
	      && !(operand->flags & ARC_OPERAND_LIMM))
	    {
	      if ((value == 0x3E && insnLen == 4)
		  || (value == 0x1E && insnLen == 2))
		invalid = 1;
	    }
	}
      if (invalid)
	continue;

      /* The instruction is valid. */
      goto found;
    }

  /* No instruction found. Try the extenssions. */
  {
    const char *instrName = "";
    int flags;
    instrName = arcExtMap_instName (OPCODE (insn[0]), insn[0],
				    &flags);
    if (instrName)
      {
	opcode = &arc_opcodes[0];
	(*info->fprintf_func) (info->stream, "%s", instrName);
	goto print_flags;
      }
  }

  if (insnLen == 2)
    (*info->fprintf_func) (info->stream, ".long %#04x", insn[0]);
  else
    (*info->fprintf_func) (info->stream, ".long %#08x", insn[0]);

  return insnLen;

 found:
  (*info->fprintf_func) (info->stream, "%s", opcode->name);

  pr_debug ("%s: 0x%08x\n", opcode->name, opcode->opcode);

 print_flags:
  /* Now extract and print the flags. */
  for (flgidx = opcode->flags; *flgidx; flgidx++)
    {
      /* Get a valid flag class*/
      const struct arc_flag_class *cl_flags = &arc_flag_classes[*flgidx];
      const unsigned *flgopridx;

      for (flgopridx = cl_flags->flags; *flgopridx; ++flgopridx)
	{
	  const struct arc_flag_operand *flg_operand = &arc_flag_operands[*flgopridx];
	  unsigned int value;

	  if (!flg_operand->favail)
	    continue;

	  value = (insn[0] >> flg_operand->shift) & ((1 << flg_operand->bits) - 1);
	  if (value == flg_operand->code)
	    (*info->fprintf_func) (info->stream, ".%s", flg_operand->name); /* FIXME!: print correctly nt/t flag */

	  if (flg_operand->name[0] == 'd')
	    info->branch_delay_insns = 1;
	}
    }

  if (opcode->operands[0] != 0)
    (*info->fprintf_func) (info->stream, "\t");

  /* Now extract and print the operands. */
  int need_comma = 0;
  int open_braket = 0;
  for (opidx = opcode->operands; *opidx; opidx++)
    {
      const struct arc_operand *operand = &arc_operands[*opidx];
      int value;

      if (open_braket && (operand->flags & ARC_OPERAND_BRAKET))
	{
	  (*info->fprintf_func) (info->stream, "]");
	  open_braket = 0;
	  continue;
	}

      /* Only take input from real operands.  */
      if ((operand->flags & ARC_OPERAND_FAKE) && !(operand->flags & ARC_OPERAND_BRAKET))
	continue;

      if (operand->extract)
	value = (*operand->extract) (insn[0], (int *) NULL);
      else
	{
	  if (operand->flags & ARC_OPERAND_ALIGNED32)
	    {
	      value = (insn[0] >> operand->shift) & ((1 << (operand->bits - 2)) - 1);
	      value = value << 2;
	    }
	  else
	    {
	      value = (insn[0] >> operand->shift) & ((1 << operand->bits) - 1);
	    }
	  if (operand->flags & ARC_OPERAND_SIGNED)
	    {
	      int signbit = 1 << (operand->bits - 1);
	      value = (value ^ signbit) - signbit;
	    }
	}

      if (need_comma)
	(*info->fprintf_func) (info->stream, ",");
      
      if (!open_braket && (operand->flags & ARC_OPERAND_BRAKET))
	{
	  (*info->fprintf_func) (info->stream, "[");
	  open_braket = 1;
	  need_comma = 0;
	  continue;
	}

    /* Print the operand as directed by the flags.  */
      if (operand->flags & ARC_OPERAND_IR)
	(*info->fprintf_func) (info->stream, "%s", regnames[value]);
      else if (operand->flags & ARC_OPERAND_LIMM)
	(*info->fprintf_func) (info->stream, "%#x", insn[1]);
      else if (operand->flags & ARC_OPERAND_PCREL)
	(*info->print_address_func) ((memaddr & ~3) + value, info); /* PCL relative! */
      else if (operand->flags & ARC_OPERAND_SIGNED)
	(*info->fprintf_func) (info->stream, "%d", value);
      else
	(*info->fprintf_func) (info->stream, "%#x", value);

      need_comma = 1;
      
      /* adjust insn len*/
      if (operand->flags & ARC_OPERAND_LIMM
	  && !(operand->flags & ARC_OPERAND_DUPLICATE))
	insnLen += 4;
    }

  return insnLen;
}

disassembler_ftype
arc_get_disassembler (bfd *abfd)
{
  /* Read the extenssion insns and registers, if any. */
  build_ARC_extmap (abfd);
  dump_ARC_extmap ();

  return print_insn_arc;
}

/* Local variables:
   eval: (c-set-style "gnu")
   indent-tabs-mode: t
   End:  */

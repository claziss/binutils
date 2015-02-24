/* Opcode table for the ARC.
   Copyright 2013
   Free Software Foundation, Inc.

   This file is part of GAS, the GNU Assembler, GDB, the GNU debugger, and
   the GNU Binutils.

   GAS/GDB is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GAS/GDB is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS or GDB; see the file COPYING3.  If not, write to
   the Free Software Foundation, 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#ifndef OPCODE_ARC_H
#define OPCODE_ARC_H

/* The opcode table is an array of struct arc_opcode.  */
struct arc_opcode
{
  /* The opcode name. */
  const char *name;

  /* The opcode itself.  Those bits which will be filled in with
     operands are zeroes.  */
  unsigned opcode;

  /* The opcode mask.  This is used by the disassembler.  This is a
     mask containing ones indicating those bits which must match the
     opcode field, and zeroes indicating those bits which need not
     match (and are presumably filled in by operands).  */
  unsigned mask;

  /* One bit flags for the opcode.  These are primarily used to
     indicate specific processors and environments support the
     instructions.  The defined values are listed below. */
  unsigned cpu;

  /* An array of operand codes.  Each code is an index into the
     operand table.  They appear in the order which the operands must
     appear in assembly code, and are terminated by a zero.  */
  unsigned char operands[4];

  /* An array of flag codes.  Each code is an index into the flag
     table.  They appear in the order which the flags must appear in
     assembly code, and are terminated by a zero.  */
  unsigned char flags[4];
};

/* The table itself is sorted by major opcode number, and is otherwise
   in the order in which the disassembler should consider
   instructions.  */
extern const struct arc_opcode arc_opcodes[];
extern const unsigned arc_num_opcodes;

/* CPU Availability */
#define ARC_OPCODE_ARC600   0x0001  /* ARC 600 specific insns.  */
#define ARC_OPCODE_ARC700   0x0002  /* ARC 700 specific insns.  */
#define ARC_OPCODE_ARCv2EM  0x0004  /* ARCv2 EM specific insns.  */
#define ARC_OPCODE_ARCv2HS  0x0008  /* ARCv2 HS specific insns.  */

/* CPU extensions */
#define ARC_EA       0x0010
#define ARC_CD       0x0010    /* Mutual exclusive with EA. */
#define ARC_LLOCK    0x0020
#define ARC_ATOMIC   0x0020    /* Mutual exclusive with LLOCK.*/
#define ARC_MPY      0x0100
#define ARC_MULT     0x0100

/* Floating point support. */
#define ARC_DPFP     0x0040
#define ARC_SPFP     0x0040
#define ARC_FPU      0x0040

/* NORM & SWAP */
#define ARC_SWAP     0x0080
#define ARC_NORM     0x0080
#define ARC_BSCAN    0x0080

/* A7 specific */
#define ARC_UIX      0x1000
#define ARC_TSTAMP   0x1000

/* A6 specific */
#define ARC_VBFDW    0x1000
#define ARC_BARREL   0x1000
#define ARC_DSPA     0x1000

/* EM specific */
#define ARC_SHIFT    0x1000

/* V2 specific */
#define ARC_INTR     0x1000
#define ARC_DIV      0x1000

/* V1 specific */
#define ARC_XMAC     0x1000
#define ARC_CRC      0x1000


/* Base architecture -- all cpus.  */
#define ARC_OPCODE_BASE				\
  (ARC_OPCODE_ARC600 | ARC_OPCODE_ARC700	\
   | ARC_OPCODE_ARCv2EM | ARC_OPCODE_ARCv2HS)

/* The operands table is an array of struct arc_operand.  */
struct arc_operand
{
  /* The number of bits in the operand.  */
  unsigned int bits;

  /* How far the operand is left shifted in the instruction.  */
  unsigned int shift;

  /* The default relocation type for this operand.  */
  signed int default_reloc;

  /* One bit syntax flags.  */
  unsigned int flags;

  /* Insertion function.  This is used by the assembler.  To insert an
     operand value into an instruction, check this field.

     If it is NULL, execute
         i |= (op & ((1 << o->bits) - 1)) << o->shift;
     (i is the instruction which we are filling in, o is a pointer to
     this structure, and op is the opcode value; this assumes twos
     complement arithmetic).

     If this field is not NULL, then simply call it with the
     instruction and the operand value.  It will return the new value
     of the instruction.  If the ERRMSG argument is not NULL, then if
     the operand value is illegal, *ERRMSG will be set to a warning
     string (the operand will be inserted in any case).  If the
     operand value is legal, *ERRMSG will be unchanged (most operands
     can accept any value).  */
  unsigned (*insert) (unsigned instruction, int op, const char **errmsg);

  /* Extraction function.  This is used by the disassembler.  To
     extract this operand type from an instruction, check this field.

     If it is NULL, compute
         op = ((i) >> o->shift) & ((1 << o->bits) - 1);
	 if ((o->flags & ARC_OPERAND_SIGNED) != 0
	     && (op & (1 << (o->bits - 1))) != 0)
	   op -= 1 << o->bits;
     (i is the instruction, o is a pointer to this structure, and op
     is the result; this assumes twos complement arithmetic).

     If this field is not NULL, then simply call it with the
     instruction value.  It will return the value of the operand.  If
     the INVALID argument is not NULL, *INVALID will be set to
     non-zero if this operand type can not actually be extracted from
     this operand (i.e., the instruction does not match).  If the
     operand is valid, *INVALID will not be changed.  */
  int (*extract) (unsigned instruction, int *invalid);
};

/* Elements in the table are retrieved by indexing with values from
   the operands field of the arc_opcodes table.  */
extern const struct arc_operand arc_operands[];
extern const unsigned arc_num_operands;
extern const unsigned arc_Toperand;
extern const unsigned arc_NToperand;

/* Values defined for the flags field of a struct arc_operand.  */

/* This operand does not actually exist in the assembler input.  This
   is used to support extended mnemonics, for which two operands fields
   are identical.  The assembler should call the insert function with
   any op value.  The disassembler should call the extract function,
   ignore the return value, and check the value placed in the invalid
   argument.  */
#define ARC_OPERAND_FAKE	0x0001

/* This operand names an integer register.  */
#define ARC_OPERAND_IR		0x0002

/* This operand takes signed values.  */
#define ARC_OPERAND_SIGNED	0x0004

/* This operand takes unsigned values.  This exists primarily so that
   a flags value of 0 can be treated as end-of-arguments.  */
#define ARC_OPERAND_UNSIGNED	0x0008

/* This operand takes long immediate values. */
#define ARC_OPERAND_LIMM	0x0010

/* This operand is identical like the previous one. */
#define ARC_OPERAND_DUPLICATE   0x0020

/* This operand is PC relative. Used for internal relocs. */
#define ARC_OPERAND_PCREL       0x0040

/* This operand is truncated. The truncation is done accordingly to
   operand alignment attribute. */
#define ARC_OPERAND_TRUNCATE    0x0080

/* This operand is 16bit aligned. */
#define ARC_OPERAND_ALIGNED16   0x0100

/* This operand is 32bit aligned. */
#define ARC_OPERAND_ALIGNED32   0x0200

/* This operand can be ignored by matching process if it is not
   present */
#define ARC_OPERAND_IGNORE      0x0400

/* Don't check the range when matching */
#define ARC_OPERAND_NCHK      0x0800

/* Mask for selecting the type for typecheck purposes */
#define ARC_OPERAND_TYPECHECK_MASK		\
  (ARC_OPERAND_IR |				\
   ARC_OPERAND_LIMM | ARC_OPERAND_SIGNED | 	\
   ARC_OPERAND_UNSIGNED)

/* The flags structure  */
struct arc_flag_operand
{
  /* The flag name. */
  const char *name;

  /* The flag code. */
  unsigned code;

  /* The number of bits in the operand.  */
  unsigned int bits;

  /* How far the operand is left shifted in the instruction.  */
  unsigned int shift;

  /* Available for disassembler */
  unsigned char favail;
};

/* The flag operands table. */
extern const struct arc_flag_operand arc_flag_operands[];
extern const unsigned arc_num_flag_operands;

/* The flag's class structure. */
struct arc_flag_class
{
  /* Size of the list. */
  unsigned size;

  /* List of valid flags (codes). */
  unsigned flags[256];
};

extern const struct arc_flag_class arc_flag_classes[];

/* Structure for special cases */
struct arc_flag_special
{
  /* Name of special case instruction */
  const char *name;

  /* List of flags applicable for special case instruction */
  unsigned flags[32];
};

extern const struct arc_flag_special arc_flag_special_cases[];
extern const unsigned arc_num_flag_special;

#endif /* OPCODE_ARC_H */

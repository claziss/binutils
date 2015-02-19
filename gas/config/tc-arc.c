/* tc-arc.c -- Assembler for the ARC Copyright 1994, 1995, 1997, 1998,
   2000, 2001, 2002, 2005, 2006, 2007, 2008, 2009, 2011, 20013 Free
   Software Foundation, Inc.

   Copyright 2013 Synopsys Inc
   Contributor: Claudiu Zissulescu <claziss@synopsys.com>

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#include "as.h"
#include "subsegs.h"
#include "struc-symbol.h"
#include "dwarf2dbg.h"
#include "safe-ctype.h"

#include "opcode/arc.h"
#include "elf/arc.h"

/**************************************************************************/
/* Defines                                                                */
/**************************************************************************/

#define MAX_INSN_ARGS        3
#define MAX_INSN_FLGS        3
#define MAX_FLAG_NAME_LENGHT 3
#define MAX_INSN_FIXUPS      2

#define DEBUG

#ifdef DEBUG
# define pr_debug(fmt, args...) fprintf(stderr, fmt, ##args)
#else
# define pr_debug(fmt, args...)
#endif

/**************************************************************************/
/* Macros                                                                 */
/**************************************************************************/

#define regno(x)		((x) & 31)
#define is_ir_num(x)		(((x) & 32) == 0)

/**************************************************************************/
/* Generic assembler global variables which must be defined by all        */
/* targets.                                                               */
/**************************************************************************/

/* Characters which always start a comment.  */
const char comment_chars[] = "#";

/* Characters which start a comment at the beginning of a line.  */
const char line_comment_chars[] = "#";

/* Characters which may be used to separate multiple commands on a
   single line.  */
const char line_separator_chars[] = "`";

/* Characters which are used to indicate an exponent in a floating
   point number.  */
const char EXP_CHARS[] = "eE";

/* Chars that mean this number is a floating point constant
   As in 0f12.456 or 0d1.2345e12.  */
const char FLT_CHARS[] = "rRsSfFdD";

/* Byte order.  */
extern int target_big_endian;
const char *arc_target_format = DEFAULT_TARGET_FORMAT;
static int byte_order = DEFAULT_BYTE_ORDER;

/* Forward declaration */
static void arc_common (int);

const pseudo_typeS md_pseudo_table[] =
  {
    /* Make sure that .word is 32 bits */
    { "word", cons, 4 },

    { "align",   s_align_bytes, 0 }, /* Defaulting is invalid (0).  */
    { "comm",    arc_common, 0 },
    { "common",  arc_common, 0 },
    { "lcomm",   arc_common, 1 },
    { "lcommon", arc_common, 1 },

    { NULL, NULL, 0 }
  };

const char *md_shortopts = "";

enum options
  {
    OPTION_EB = OPTION_MD_BASE,
    OPTION_EL,
    OPTION_MCPU
  };

struct option md_longopts[] =
  {
    { "EB",             no_argument, NULL, OPTION_EB },
    { "EL",             no_argument, NULL, OPTION_EL },
    { "mcpu",           required_argument, NULL, OPTION_MCPU },
    { NULL,		no_argument, NULL, 0 }
  };

size_t md_longopts_size = sizeof (md_longopts);

/**************************************************************************/
/* Local data and data types                                              */
/**************************************************************************/
/* Used since new relocation types are introduced in this
   file (DUMMY_RELOC_LITUSE_*) */
typedef int extended_bfd_reloc_code_real_type;

struct arc_fixup
{
  expressionS exp;

  extended_bfd_reloc_code_real_type reloc;

  /* index into arc_operands */
  unsigned int opindex;

  /*PC-relative, used by internals fixups. */
  unsigned char pcrel;
};

struct arc_insn
{
  unsigned int insn;
  int nfixups;
  struct arc_fixup fixups[MAX_INSN_FIXUPS];
  long sequence;
  long limm;
  unsigned char short_insn; /* Boolean value: 1 if current insn is short. */
  unsigned char has_limm;   /* Boolean value: 1 if limm field is valid. */
};

/* The cpu for which we are generating code.  */
static unsigned arc_target = ARC_OPCODE_BASE;
static const char *arc_target_name = "<all>";

/* The default architecture. */
static int arc_mach_type = bfd_mach_arc_arcv2;

/* The hash table of instruction opcodes.  */
static struct hash_control *arc_opcode_hash;

/* A table of CPU names and opcode sets.  */
static const struct cpu_type
{
  const char *name;
  unsigned flags;
  int mach;
}
cpu_types[] =
{
  { "arc600", ARC_OPCODE_ARC600, bfd_mach_arc_arc700 }, /* FIXME! update bfd-in2.h */
  { "arc700", ARC_OPCODE_ARC700, bfd_mach_arc_arc700 },
  { "arcem", ARC_OPCODE_ARCv2EM, bfd_mach_arc_arcv2 },
  { "archs", ARC_OPCODE_ARCv2HS, bfd_mach_arc_arcv2 },
  { "all", ARC_OPCODE_BASE, bfd_mach_arc_arcv2 },
  { 0, 0, 0 }
};

struct arc_flags
{
  /* Name of the parsed flag*/
  char name[MAX_FLAG_NAME_LENGHT];

  /* The code of the parsed flag. Valid when is not zero. */
  unsigned char code;
};

/* A table of the register symbols.  */
static symbolS *arc_register_table[64];

/* Used by the arc_reloc_op table. Order is important. */
#define O_gotoff  O_md1     /* @gotoff relocation. */
#define O_gotpc   O_md2     /* @gotpc relocation. */
#define O_plt     O_md3     /* @plt relocation. */
#define O_sda     O_md4     /* @sda relocation. */

/* Dummy relocation, to be sorted out */
#define DUMMY_RELOC_ARC_SDA     (BFD_RELOC_UNUSED + 1)

#define USER_RELOC_P(R) ((R) >= O_gotoff && (R) <= O_sda)

/* A table to map the spelling of a relocation operand into an appropriate
   bfd_reloc_code_real_type type.  The table is assumed to be ordered such
   that op-O_literal indexes into it.  */
#define ARC_RELOC_TABLE(op)						\
  (&arc_reloc_op[ ((!USER_RELOC_P (op))					\
		   ? (abort (), 0)					\
		   : (int) (op) - (int) O_gotoff) ])

#define DEF(NAME, RELOC, REQ)				\
  { #NAME, sizeof(#NAME)-1, O_##NAME, RELOC, REQ}

static const struct arc_reloc_op_tag
{
  const char *name;				/* String to lookup.  */
  size_t length;				/* Size of the string.  */
  operatorT op;					/* Which operator to use.  */
  extended_bfd_reloc_code_real_type reloc;
  unsigned int complex_expr : 1;		/* Allows complex
						   relocatione
						   expression like
						   identifier@reloc +
						   const  */
}
arc_reloc_op[] =
{
  DEF (gotoff, BFD_RELOC_ARC_GOTOFF,  1),
  DEF (gotpc,  BFD_RELOC_ARC_GOTPC32, 0),
  DEF (plt,    BFD_RELOC_ARC_PLT32,   0),
  DEF (sda,    DUMMY_RELOC_ARC_SDA,   1),
};

static const int arc_num_reloc_op
  = sizeof (arc_reloc_op) / sizeof (*arc_reloc_op);

/* Flags to set in the elf header. */
static flagword arc_eflag = 0x00;

/**************************************************************************/
/* Functions declaration                                                  */
/**************************************************************************/

static void assemble_tokens (const char *, const expressionS *, int,
			     struct arc_flags *, int);
static const struct arc_opcode *find_opcode_match (const struct arc_opcode *,
						   expressionS *, int *,
						   struct arc_flags *, int, int *);
static void assemble_insn (const struct arc_opcode *, const expressionS *,
			   int, const struct arc_flags *, int,
			   struct arc_insn *, extended_bfd_reloc_code_real_type);
static void emit_insn (struct arc_insn *);
static unsigned insert_operand (unsigned, const struct arc_operand *,
				offsetT, char *, unsigned);
static const struct arc_opcode *find_special_case (const char *opname,
        int *nflgs, struct arc_flags *pflags);

/**************************************************************************/
/* Functions implementation                                               */
/**************************************************************************/

/* Like md_number_to_chars but used for limms. The 4-byte limm value,
   is encoded as 'middle-endian' for a little-endian target. FIXME!
   this function is used for regular 4 byte instructions as well. */

static void
md_number_to_chars_midend (char *buf,
			   valueT val,
			   int n)
{
  if (n == 4)
    {
      md_number_to_chars (buf,     (val & 0xffff0000) >> 16, 2);
      md_number_to_chars (buf + 2, (val & 0xffff), 2);
    }
  else
    {
      md_number_to_chars (buf, val, n);
    }
}

static void
arc_common (int localScope)
{
  char *name;
  char c;
  char *p;
  int align, size;
  symbolS *symbolP;

  name = input_line_pointer;
  c = get_symbol_end ();
  /* just after name is now '\0'  */
  p = input_line_pointer;
  *p = c;
  SKIP_WHITESPACE ();

  if (*input_line_pointer != ',')
    {
      as_bad (_("expected comma after symbol name"));
      ignore_rest_of_line ();
      return;
    }

  input_line_pointer++;		/* skip ','  */
  size = get_absolute_expression ();

  if (size < 0)
    {
      as_bad (_("negative symbol length"));
      ignore_rest_of_line ();
      return;
    }

  *p = 0;
  symbolP = symbol_find_or_make (name);
  *p = c;

  if (S_IS_DEFINED (symbolP) && ! S_IS_COMMON (symbolP))
    {
      as_bad (_("ignoring attempt to re-define symbol"));
      ignore_rest_of_line ();
      return;
    }
  if (((int) S_GET_VALUE (symbolP) != 0) \
      && ((int) S_GET_VALUE (symbolP) != size))
    {
      as_warn (_("length of symbol \"%s\" already %ld, ignoring %d"),
	       S_GET_NAME (symbolP), (long) S_GET_VALUE (symbolP), size);
    }
  gas_assert (symbolP->sy_frag == &zero_address_frag);

  /* Now parse the alignment field.  This field is optional for
     local and global symbols. Default alignment is zero.  */
  if (*input_line_pointer == ',')
    {
      input_line_pointer++;
      align = get_absolute_expression ();
      if (align < 0)
	{
	  align = 0;
	  as_warn (_("assuming symbol alignment of zero"));
	}
    }
  else if (localScope == 0)
    align = 0;

  else
    {
      as_bad (_("Expected comma after length for lcomm directive"));
      ignore_rest_of_line ();
      return;
    }


  if (localScope != 0)
    {
      segT old_sec;
      int old_subsec;
      char *pfrag;

      old_sec    = now_seg;
      old_subsec = now_subseg;
      record_alignment (bss_section, align);
      subseg_set (bss_section, 0);  /* ??? subseg_set (bss_section, 1); ???  */

      if (align)
	/* Do alignment.  */
	frag_align (align, 0, 0);

      /* Detach from old frag.  */
      if (S_GET_SEGMENT (symbolP) == bss_section)
	symbolP->sy_frag->fr_symbol = NULL;

      symbolP->sy_frag = frag_now;
      pfrag = frag_var (rs_org, 1, 1, (relax_substateT) 0, symbolP,
			(offsetT) size, (char *) 0);
      *pfrag = 0;

      S_SET_SIZE       (symbolP, size);
      S_SET_SEGMENT    (symbolP, bss_section);
      S_CLEAR_EXTERNAL (symbolP);
      symbol_get_obj (symbolP)->local = 1;
      subseg_set (old_sec, old_subsec);
    }
  else
    {
      S_SET_VALUE    (symbolP, (valueT) size);
      S_SET_ALIGN    (symbolP, align);
      S_SET_EXTERNAL (symbolP);
      S_SET_SEGMENT  (symbolP, bfd_com_section_ptr);
    }

  symbolP->bsym->flags |= BSF_OBJECT;

  demand_empty_rest_of_line ();
}


/* Smartly print an expression. */
static void
debug_exp (expressionS *t)
{
  const char *name;
  const char *namemd;

  pr_debug ("debug_exp: ");

  switch (t->X_op)
    {
    default:			name = "unknown";		break;
    case O_illegal:		name = "O_illegal";		break;
    case O_absent:		name = "O_absent";		break;
    case O_constant:		name = "O_constant";		break;
    case O_symbol:      	name = "O_symbol";		break;
    case O_symbol_rva:		name = "O_symbol_rva";		break;
    case O_register:		name = "O_register";		break;
    case O_big:			name = "O_big";			break;
    case O_uminus:		name = "O_uminus";		break;
    case O_bit_not:		name = "O_bit_not";		break;
    case O_logical_not:		name = "O_logical_not";		break;
    case O_multiply:		name = "O_multiply";		break;
    case O_divide:		name = "O_divide";		break;
    case O_modulus:		name = "O_modulus";		break;
    case O_left_shift:		name = "O_left_shift";		break;
    case O_right_shift:		name = "O_right_shift";		break;
    case O_bit_inclusive_or:	name = "O_bit_inclusive_or";	break;
    case O_bit_or_not:		name = "O_bit_or_not";		break;
    case O_bit_exclusive_or:	name = "O_bit_exclusive_or";	break;
    case O_bit_and:		name = "O_bit_and";		break;
    case O_add:			name = "O_add";			break;
    case O_subtract:		name = "O_subtract";		break;
    case O_eq:			name = "O_eq";			break;
    case O_ne:			name = "O_ne";			break;
    case O_lt:			name = "O_lt";			break;
    case O_le:			name = "O_le";			break;
    case O_ge:			name = "O_ge";			break;
    case O_gt:			name = "O_gt";			break;
    case O_logical_and:		name = "O_logical_and";		break;
    case O_logical_or:		name = "O_logical_or";		break;
    case O_index:		name = "O_index";		break;
    }

  switch (t->X_md)
    {
    default:			namemd = "unknown";		break;
    case O_gotoff:              namemd = "O_gotoff";		break;
    case O_gotpc:               namemd = "O_gotpc";		break;
    case O_plt:                 namemd = "O_plt"; 		break;
    case O_sda:                 namemd = "O_sda"; 		break;
    }

  pr_debug ("%s(%s, %s, %d, %s)", name,
	    (t->X_add_symbol) ? S_GET_NAME (t->X_add_symbol) : "--",
	    (t->X_op_symbol) ? S_GET_NAME (t->X_op_symbol) : "--",
	    (int) t->X_add_number,
	    (t->X_md) ? namemd : "--");
  pr_debug ("\n");
  fflush (stderr);
}

/* Parse the arguments to an opcode. */
static int
tokenize_arguments (char *str,
		    expressionS tok[],
		    int ntok)
{
  char *old_input_line_pointer;
  bfd_boolean saw_comma = FALSE;
  bfd_boolean saw_arg = FALSE;
  bfd_boolean saw_brk = FALSE;
  int num_args = 0;
  const char *p;
  int i;
  size_t len;
  const struct arc_reloc_op_tag *r;
  expressionS tmpE;

  memset (tok, 0, sizeof (*tok) * ntok);

  /* Save and restore input_line_pointer around this function.  */
  old_input_line_pointer = input_line_pointer;
  input_line_pointer = str;

  while (*input_line_pointer)
    {
      SKIP_WHITESPACE();
      switch (*input_line_pointer)
	{
	case '\0':
	  goto fini;

	case ',':
	  input_line_pointer++;
	  if (saw_comma || !saw_arg)
	    goto err;
	  saw_comma = TRUE;
	  break;

	  /* FIXME! just smartly ignore '[' and ']'. I should check
	     against the mnemonic syntax. */
	case ']':
	  saw_brk = FALSE;
	  if(!saw_arg)
	    goto err;
	  /* FALL THROUGH */
	case '[':
	  input_line_pointer++;
	  if (saw_brk)
	    goto err;
	  saw_brk = TRUE;
	  break;

	case '@':
	  /* We have labels, function names and reloations, all
	     starting with @ symbol. Sort them out. */
	  if (saw_arg && !saw_comma)
	    goto err;

	  ++input_line_pointer;

	  /* Parse @label. */
	  expression (tok);
	  if (*input_line_pointer != '@')
	    goto normalsymbol; /* this is not a relocation. */

	  /* A relocation opernad has the following form
	     @identifier@relocation_type. The identifier is already in tok! */
	  if (tok->X_op != O_symbol)
	    {
	      as_bad (_("No valid label relocation operand"));
	      goto err;
	    }

	  ++input_line_pointer;
	  /* Parse @relocation_type */
	  memset (&tmpE, 0, sizeof (tmpE));
	  expression (&tmpE);

	  if (tmpE.X_op != O_symbol)
	    {
	      as_bad (_("No relocation operand"));
	      goto err;
	    }
	  p = S_GET_NAME (tmpE.X_add_symbol);
	  len = strlen (p);

	  /* Go through known relocation and try to find a match. */
	  r = &arc_reloc_op[0];
 	  for (i = arc_num_reloc_op - 1; i >= 0; i--, r++)
	    if (len == r->length && memcmp (p, r->name, len) == 0)
	      break;

	  if (i < 0)
	    {
	      as_bad (_("Unknown relocation operand: @%s"), p);
	      goto err;
	    }
	  tok->X_md = r->op;
	  tok->X_add_number = tmpE.X_add_number;
	  if (tmpE.X_add_number && !r->complex_expr)
	    {
	      as_bad (_("Complex relocation operand."));
	      goto err;
	    }

#ifdef DEBUG
	  //print_expr (tok);
#endif
 	  debug_exp (tok);

	  saw_comma = FALSE;
	  saw_arg = TRUE;
	  tok++;
	  num_args++;
	  break;

	default:

	  if (saw_arg && !saw_comma)
	    goto err;

	  expression (tok);

	normalsymbol:
 	  debug_exp (tok);
#ifdef DEBUG
	  //print_expr (tok);
#endif
	  if (tok->X_op == O_illegal || tok->X_op == O_absent)
	    goto err;

	  saw_comma = FALSE;
	  saw_arg = TRUE;
	  tok++;
	  num_args++;
	  break;
	}
    }

 fini:
  if (saw_comma)
    goto err;
  input_line_pointer = old_input_line_pointer;

  return num_args;

 err:
  if (saw_comma)
    as_bad (_("extra comma"));
  else if (!saw_arg)
    as_bad (_("missing argument"));
  else
    as_bad (_("missing comma or colon"));
  input_line_pointer = old_input_line_pointer;
  return -1;
}

/* Parse the flags to a structure */
static int
tokenize_flags (char *str,
		struct arc_flags flags[],
		int nflg)
{
  char *old_input_line_pointer;
  bfd_boolean saw_flg = FALSE;
  bfd_boolean saw_dot = FALSE;
  int num_flags  = 0;
  size_t flgnamelen;

  memset (flags, 0, sizeof (*flags) * nflg);

  /* Save and restore input_line_pointer around this function.  */
  old_input_line_pointer = input_line_pointer;
  input_line_pointer = str;

  while (*input_line_pointer)
    {
      switch (*input_line_pointer)
	{
	case ' ':
	case '\0':
	  goto fini;

	case '.':
	  input_line_pointer++;
	  if (saw_dot)
	    goto err;
	  saw_dot = TRUE;
	  saw_flg = FALSE;
	  break;

	default:
	  if (saw_flg && !saw_dot)
	    goto err;

	  if (num_flags >= nflg)
	    goto err;

	  flgnamelen = strspn (input_line_pointer, "abcdefghilmnopqrstvwxz");
	  if (flgnamelen > MAX_FLAG_NAME_LENGHT)
	    goto err;

	  memcpy (flags->name, input_line_pointer, flgnamelen);

	  input_line_pointer += flgnamelen;
	  flags++;
	  saw_dot = FALSE;
	  saw_flg = TRUE;
	  num_flags++;
	  break;
	}
    }

 fini:
  input_line_pointer = old_input_line_pointer;
  return num_flags;

 err:
  if (saw_dot)
    as_bad (_("extra dot"));
  else if (!saw_flg)
    as_bad (_("unrecognized flag"));
  else
    as_bad (_("failed to parse flags"));
  input_line_pointer = old_input_line_pointer;
  return -1;
}

/* The public interface to the instruction assembler.  */
void
md_assemble (char *str)
{
  char *opname;
  expressionS tok[MAX_INSN_ARGS];
  int ntok, nflg;
  size_t opnamelen;
  struct arc_flags flags[MAX_INSN_FLGS];

  /* Split off the opcode.  */
  opnamelen = strspn (str, "abcdefghijklmnopqrstuvwxyz_0123468");
  opname = xmalloc (opnamelen + 1);
  memcpy (opname, str, opnamelen);
  opname[opnamelen] = '\0';

  /* Tokenize the flags */
  if ((nflg = tokenize_flags (str + opnamelen, flags, MAX_INSN_FLGS)) == -1)
    {
      as_bad (_("syntax error"));
      return;
    }

  /* Scan up to the end of the mnemonic which must end in space or end
     of string. */
  str += opnamelen;
  for (; *str != '\0'; str++)
    if (*str == ' ')
      break;

  /* Tokenize the rest of the line.  */
  if ((ntok = tokenize_arguments (str, tok, MAX_INSN_ARGS)) < 0)
    {
      as_bad (_("syntax error"));
      return;
    }

  /* Finish it off. */
  assemble_tokens (opname, tok, ntok, flags, nflg);
}

/* Port-specific assembler initialization. This function is called
   once, at assembler startup time.  */
void
md_begin (void)
{
  unsigned int i;

  /* The endianness can be chosen "at the factory".  */
  target_big_endian = byte_order == BIG_ENDIAN;

  if (!bfd_set_arch_mach (stdoutput, bfd_arch_arc, arc_mach_type))
    as_warn (_("could not set architecture and machine"));

  /* Set elf header flags. */
  bfd_set_private_flags(stdoutput, arc_eflag);

  /* Set up a hash table for the instructions.  */
  arc_opcode_hash = hash_new ();
  if (arc_opcode_hash == NULL)
    as_fatal (_("Virtual memory exhausted"));

  /* Initialize the hash table with the insns */
  for (i = 0; i < arc_num_opcodes;)
    {
      const char *name, *retval;

      name = arc_opcodes[i].name;
      retval = hash_insert (arc_opcode_hash, name, (void *) &arc_opcodes[i]);
      if (retval)
	as_fatal (_("internal error: can't hash opcode '%s': %s"),
		  name, retval);

      while (++i < arc_num_opcodes
	     && (arc_opcodes[i].name == name
		 || !strcmp (arc_opcodes[i].name, name)))
	continue;
    }

  /* Construct symbols for each of the registers.  */
  for (i = 0; i < 32; ++i)
    {
      char name[4];

      sprintf (name, "r%d", i);
      arc_register_table[i] = symbol_create (name, reg_section, i,
					     &zero_address_frag);
    }

  /* TBD */
}

/* Write a value out to the object file, using the appropriate
   endianness. */
void
md_number_to_chars (char *buf,
		    valueT val,
		    int n)
{
  if (target_big_endian)
    number_to_chars_bigendian (buf, val, n);
  else
    number_to_chars_littleendian (buf, val, n);
}

/* Round up a section size to the appropriate boundary.  */
valueT
md_section_align (segT segment,
		  valueT size)
{
  int align = bfd_get_section_alignment (stdoutput, segment);

  return ((size + (1 << align) - 1) & (-1 << align));
}

/* The location from which a PC relative jump should be calculated,
   given a PC relative reloc.  */
long
md_pcrel_from_section (fixS *fixP, segT sec)
{
  offsetT base = fixP->fx_where + fixP->fx_frag->fr_address;

  pr_debug("pcrel_from_section, fx_offset = %d\n", fixP->fx_offset);

  if (fixP->fx_addsy != (symbolS *) NULL
      && (!S_IS_DEFINED (fixP->fx_addsy)
	  || S_GET_SEGMENT (fixP->fx_addsy) != sec))
    {
      pr_debug("Unknown pcrel symbol: %s\n", S_GET_NAME(fixP->fx_addsy));

      /* The symbol is undefined (or is defined but not in this section).
	 Let the linker figure it out.  */
      base = 0;
    }

  if ((int) fixP->fx_r_type < 0 )
    {
      /* these are the "internal" relocations. Align them to
	 32 bit boundary (PCL), for the moment. */
      base &= ~3;
    }
  else
    {
      switch (fixP->fx_r_type)
	{
	BFD_RELOC_ARC_PC32:
	  /* this is a limm, it should not be 32bit align
	   ... but probably it needs. */
	  //base &= ~3;
	default:
	  break;
	}
    }

  pr_debug ("pcrel from %x + %lx = %x, symbol: %s (%x)\n",
	    fixP->fx_frag->fr_address, fixP->fx_where, base,
	    fixP->fx_addsy ? S_GET_NAME (fixP->fx_addsy) : "(null)",
	    fixP->fx_addsy ? S_GET_VALUE (fixP->fx_addsy) : 0);

  return base;
}

/* Apply a fixup to the object code. At this point all symbol values
   should be fully resolved, and we attempt to completely resolve the
   reloc.  If we can not do that, we determine the correct reloc code
   and put it back in the fixup. To indicate that a fixup has been
   eliminated, set fixP->fx_done. */
void
md_apply_fix (fixS *fixP, valueT *valP, segT seg)
{
  char * const fixpos = fixP->fx_frag->fr_literal + fixP->fx_where;
  valueT value = *valP;
  unsigned insn = 0;
  symbolS *fx_addsy, *fx_subsy;
  offsetT fx_offset;
  segT add_symbol_segment = absolute_section;
  segT sub_symbol_segment = absolute_section;

  pr_debug("%s:%u: apply_fix: r_type=%d value=%lx offset=%lx\n",
	   fixP->fx_file, fixP->fx_line, fixP->fx_r_type, value,
	   fixP->fx_offset);

  fx_addsy = fixP->fx_addsy;
  fx_subsy = fixP->fx_subsy;
  fx_offset = fixP->fx_offset;

  if (fx_addsy)
    {
      add_symbol_segment = S_GET_SEGMENT (fx_addsy);
    }

  if (fx_subsy)
    {
      resolve_symbol_value(fx_subsy);
      sub_symbol_segment = S_GET_SEGMENT(fx_subsy);

      if (sub_symbol_segment == absolute_section)
	{
	  /* The symbol is really a constant. */
	  fx_offset -= S_GET_VALUE (fx_subsy);
	  fx_subsy = NULL;
	}
      else
	{
	  as_bad_where(fixP->fx_file, fixP->fx_line,
		       _("can't resolve `%s' {%s section} - `%s' {%s section}"),
		       fx_addsy ? S_GET_NAME (fx_addsy) : "0",
		       segment_name (add_symbol_segment),
		       S_GET_NAME (fx_subsy),
		       segment_name (sub_symbol_segment));
	  return;
	}
    }

  if (fx_addsy)
    {
      if (add_symbol_segment == seg
	  && fixP->fx_pcrel)
	{
	  value += S_GET_VALUE (fx_addsy);
	  value -= md_pcrel_from_section(fixP, seg);
	  fx_addsy = NULL;
	  fixP->fx_pcrel = FALSE;
	}
      else if (add_symbol_segment == absolute_section)
	{
	  fx_offset += S_GET_VALUE(fixP->fx_addsy);
	  fx_addsy = NULL;
	}
    }

  if (!fx_addsy)
      fixP->fx_done = TRUE;

  if (fixP->fx_pcrel)
    {
      if (fx_addsy
	  && ((S_IS_DEFINED (fx_addsy)
	       && S_GET_SEGMENT (fx_addsy) != seg)
	      || S_IS_WEAK (fx_addsy)))
	value += md_pcrel_from_section(fixP, seg);

      switch (fixP->fx_r_type)
	{
	case BFD_RELOC_ARC_32_ME:
	  fixP->fx_r_type = BFD_RELOC_ARC_PC32;
	  break;
	default:
	  if ((int) fixP->fx_r_type < 0)
	    as_fatal (_("PC relative relocation not allowed for (internal) type %d"),
		      fixP->fx_r_type);
	  break;
	}
    }

  if (fixP->fx_done)
    {
      value += fx_offset;

      /* For hosts with longs bigger than 32-bits make sure that the top
         bits of a 32-bit negative value read in by the parser are set,
         so that the correct comparisons are made.  */
      if (value & 0x80000000)
        value |= (-1L << 31);

      switch (fixP->fx_r_type)
	{
	case BFD_RELOC_ARC_32_ME:
	  insn = value;
	  md_number_to_chars_midend (fixpos, insn, fixP->fx_size);
	  break;
	default:
	  {
	    const struct arc_operand *operand;

	    if ((int) fixP->fx_r_type >= 0)
	      as_fatal (_("unhandled relocation type %s"),
			bfd_get_reloc_code_name (fixP->fx_r_type));

	    /* The rest of these fixups needs to be completely resolved as
	       constants. */
	    if (fixP->fx_addsy != 0
		&& S_GET_SEGMENT (fixP->fx_addsy) != absolute_section)
	      as_bad_where (fixP->fx_file, fixP->fx_line,
			    _("non-absolute expression in constant field"));

	    gas_assert (-(int) fixP->fx_r_type < (int) arc_num_operands);
	    operand = &arc_operands[-(int) fixP->fx_r_type];

	    if (target_big_endian)
	      insn = bfd_getb32 (fixpos);
	    else
	      {
		insn = 0;
		switch (fixP->fx_size)
		  {
		  case 4:
		    insn = bfd_getl16 (fixpos) << 16 | bfd_getl16 (fixpos + 2);
		    break;
		  case 2:
		    insn = bfd_getl16 (fixpos);
		    break;
		  default:
		    as_bad_where (fixP->fx_file, fixP->fx_line,
				  _("unknown fixup size"));
		  }
	      }

	    insn = insert_operand (insn, operand, (offsetT) value,
				   fixP->fx_file, fixP->fx_line);

	    md_number_to_chars_midend (fixpos, insn, fixP->fx_size);
	  }
	}
    }
}

/* Prepare machine-dependent frags for relaxation.

   Called just before relaxation starts. Any symbol that is now undefined
   will not become defined.

   Return the correct fr_subtype in the frag.

   Return the initial "guess for fr_var" to caller.  The guess for fr_var
   is *actually* the growth beyond fr_fix. Whatever we do to grow fr_fix
   or fr_var contributes to our returned value.

   Although it may not be explicit in the frag, pretend
   fr_var starts with a value.  */
int
md_estimate_size_before_relax (fragS *fragP ATTRIBUTE_UNUSED,
			       segT segment ATTRIBUTE_UNUSED)
{
  int growth = 4;

  fragP->fr_var = 4;
  pr_debug("%s:%d: md_estimate_size_before_relax: %d\n",
	   fragP->fr_file, fragP->fr_line, growth);

  as_fatal (_("md_estimate_size_before_relax\n"));
  return growth;
}

/* Translate internal representation of relocation info to BFD target
   format.  */
arelent *
tc_gen_reloc (asection *section ATTRIBUTE_UNUSED,
	      fixS *fixP)
{
  arelent *reloc;

  reloc = (arelent *) xmalloc (sizeof (* reloc));
  reloc->sym_ptr_ptr = (asymbol **) xmalloc (sizeof (asymbol *));
  *reloc->sym_ptr_ptr = symbol_get_bfdsym (fixP->fx_addsy);
  reloc->address = fixP->fx_frag->fr_address + fixP->fx_where;

  /* Make sure none of our internal relocations make it this far.
     They'd better have been fully resolved by this point.  */
  gas_assert ((int) fixP->fx_r_type > 0);

  reloc->howto = bfd_reloc_type_lookup (stdoutput, fixP->fx_r_type);
  if (reloc->howto == NULL)
    {
      as_bad_where (fixP->fx_file, fixP->fx_line,
		    _("cannot represent `%s' relocation in object file"),
		    bfd_get_reloc_code_name (fixP->fx_r_type));
      return NULL;
    }

  if (!fixP->fx_pcrel != !reloc->howto->pc_relative)
    as_fatal (_("internal error? cannot generate `%s' relocation"),
	      bfd_get_reloc_code_name (fixP->fx_r_type));

  gas_assert (!fixP->fx_pcrel == !reloc->howto->pc_relative);

  reloc->addend = fixP->fx_offset;

  return reloc;
}

/* Perform post-processing of machine-dependent frags after relaxation.
   Called after relaxation is finished.
   In:	Address of frag.
	fr_type == rs_machine_dependent.
	fr_subtype is what the address relaxed to.

   Out: Any fixS:s and constants are set up.
*/

void
md_convert_frag (bfd *abfd ATTRIBUTE_UNUSED,
		 segT segment ATTRIBUTE_UNUSED,
		 fragS *fragP ATTRIBUTE_UNUSED)
{
  pr_debug("%s:%d: md_convert_frag, subtype: %d, fix: %d, var: %d\n",
	   fragP->fr_file, fragP->fr_line,
	   fragP->fr_subtype, fragP->fr_fix, fragP->fr_var);
  abort ();
}

/* We have no need to default values of symbols.
   We could catch register names here, but that is handled by inserting
   them all in the symbol table to begin with.  */
symbolS *
md_undefined_symbol (char *name)
{
  int num;

  if (*name != 'r')
    return NULL;

  switch (*++name)
    {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      if (name[1] == '\0')
	num = name[0] - '0';
      else if (name[0] != '0' && ISDIGIT (name[1]) && name[2] == '\0')
	{
	  num = (name[0] - '0') * 10 + name[1] - '0';
	  if (num >= 32)
	    break;
	}
      else
	break;

      return arc_register_table[num];
    }
  return NULL;
}

/* Turn a string in input_line_pointer into a floating point constant
   of type type, and store the appropriate bytes in *litP.  The number
   of LITTLENUMS emitted is stored in *sizeP .  An error message is
   returned, or NULL on OK. */

/* Equal to MAX_PRECISION in atof-ieee.c */
#define MAX_LITTLENUMS 6

char *
md_atof (int type,
	 char *litP,
	 int *sizeP)
{
  int              i;
  int              prec;
  LITTLENUM_TYPE   words [MAX_LITTLENUMS];
  char *           t;

  switch (type)
  {
    case 'f':
    case 'F':
    case 's':
    case 'S':
      prec = 2;
      break;

    case 'd':
    case 'D':
    case 'r':
    case 'R':
      prec = 4;
      break;

      /* FIXME: Some targets allow other format chars for bigger sizes here.  */

    default:
      * sizeP = 0;
      return _("Bad call to md_atof()");
  }

  t = atof_ieee (input_line_pointer, type, words);
  if (t)
    input_line_pointer = t;
  * sizeP = prec * sizeof (LITTLENUM_TYPE);

  for (i = 0; i < prec; i++)
  {
    md_number_to_chars (litP, (valueT) words[i],
                        sizeof (LITTLENUM_TYPE));
    litP += sizeof (LITTLENUM_TYPE);
  }

  return 0;
}

/* Called for any expression that can not be recognized.  When the
   function is called, `input_line_pointer' will point to the start of
   the expression.  */

void
md_operand (expressionS *expressionP ATTRIBUTE_UNUSED)
{
}

/* md_parse_option
      Invocation line includes a switch not recognized by the base assembler.
      See if it's a processor-specific option.

      New options (supported) are:

	      -mcpu=<cpu name>		 Assemble for selected processor
	      -EB/-mbig-endian		 Big-endian
	      -EL/-mlittle-endian	 Little-endian
	      -k			 Generate PIC code

	      -m[no-]warn-deprecated     Warn about deprecated features

      The following CPU names are recognized:
	      arc700, av2em, av2hs.
*/
int
md_parse_option (int c, char *arg ATTRIBUTE_UNUSED)
{
  switch (c)
    {
    case OPTION_MCPU:
      {
	int i;
	char *s = alloca (strlen (arg) + 1);

	{
	  char *t = s;
	  char *arg1 = arg;

	  do
	    *t = TOLOWER (*arg1++);
	  while (*t++);
	}

	for (i = 0; cpu_types[i].name; ++i)
	  {
	    if (!strcmp (cpu_types[i].name, s))
	      {
		arc_target = cpu_types[i].flags;
		arc_target_name = cpu_types[i].name;
		arc_mach_type = cpu_types[i].mach;
		break;
	      }
	  }

	if (!cpu_types[i].name)
	  {
	    as_fatal (_("unknown architecture: %s\n"), arg);
	  }
	break;
      }
    case OPTION_EB:
      arc_target_format = "elf32-bigarc";
      byte_order = BIG_ENDIAN;
      break;
    case OPTION_EL:
      arc_target_format = "elf32-littlearc";
      byte_order = LITTLE_ENDIAN;
      break;

    default:
      return 0;
    }

  return 1;
}

void
md_show_usage (FILE *stream)
{
  fprintf (stream, _("ARC-specific assembler options:\n"));

  fprintf (stream, "  -mcpu=<cpu name>\t  assemble for CPU <cpu name>\n");

  fprintf (stream, _("\
  -EB                     assemble code for a big-endian cpu\n"));
  fprintf (stream, _("\
  -EL                     assemble code for a little-endian cpu\n"));
}

/* Given an opcode name, pre-tockenized set of argumenst and the
   opcode flags, take it all the way through emission. */
static void
assemble_tokens (const char *opname,
		 const expressionS *tok,
		 int ntok,
		 struct arc_flags *pflags,
		 int nflgs)
{
  int found_something = 0;
  const struct arc_opcode *opcode;
  int cpumatch = 1;
  extended_bfd_reloc_code_real_type reloc = BFD_RELOC_UNUSED;

  /* Search opcodes. */
  opcode = (const struct arc_opcode *) hash_find (arc_opcode_hash, opname);

  if (!opcode) /* Couldn't find opcode conventional way, try special cases */
      opcode = find_special_case (opname, &nflgs, pflags);

  if (opcode)
    {
      pr_debug ("%s:%d: assemble_tokens: trying opcode 0x%8X\n",
		frag_now->fr_file, frag_now->fr_line, opcode->opcode);

      found_something = 1;
      opcode = find_opcode_match (opcode, tok, &ntok, pflags, nflgs, &cpumatch);
      if (opcode)
	{
	  struct arc_insn insn;
	  assemble_insn (opcode, tok, ntok, pflags, nflgs, &insn, reloc);

	  /* Copy the sequence number for the reloc from the reloc token.  */
	  if (reloc != BFD_RELOC_UNUSED)
	    insn.sequence = tok[ntok].X_add_number;

	  emit_insn (&insn);
	  return;
	}
    }

  if (found_something)
    {
      if (cpumatch)
	as_bad (_("inappropriate arguments for opcode '%s'"), opname);
      else
	as_bad (_("opcode '%s' not supported for target %s"), opname,
		arc_target_name);
    }
  else
    as_bad (_("unknown opcode '%s'"), opname);
}

static const struct arc_opcode *
find_special_case (const char *opname,
        int *nflgs,
        struct arc_flags *pflags)
{
  int i;
  char *flagnm;
  unsigned flag_idx, flag_arr_idx;
  size_t flaglen, oplen;
  struct arc_flag_special *arc_flag_special_opcode;
  struct arc_opcode *opcode;

  /* Search for special case instruction. */
  for (i = 0; i < arc_num_flag_special; i++)
    {
      arc_flag_special_opcode = &arc_flag_special_cases[i];
      oplen = strlen (arc_flag_special_opcode->name);

      if (strncmp (opname, arc_flag_special_opcode->name, oplen) != 0)
        continue;

      /* Found a potential special case instruction, now test for flags. */
      for (flag_arr_idx = 0;; ++flag_arr_idx)
        {
          flag_idx = arc_flag_special_opcode->flags[flag_arr_idx];
          if (flag_idx == 0)
            break;  /* End of array, nothing found. */

          flagnm = arc_flag_operands[flag_idx].name;
          flaglen = strlen(flagnm);
          if (strcmp (opname + oplen, flagnm) == 0)
            {
              opcode = (const struct arc_opcode *) hash_find(arc_opcode_hash,
                arc_flag_special_opcode->name);
              if (*nflgs + 1 > MAX_INSN_FLGS)
                break;
              memcpy (pflags[*nflgs].name, flagnm, flaglen);
              pflags[*nflgs].name[flaglen] = '\0';
              (*nflgs)++;
              return opcode;
            }
        }
    }
  return NULL;
}

/* Search forward through all variants of an opcode looking for a
   syntax match.  */
static const struct arc_opcode *
find_opcode_match (const struct arc_opcode *first_opcode,
		   expressionS *tok,
		   int *pntok,
		   struct arc_flags *first_pflag,
		   int nflgs,
		   int *pcpumatch)
{
  const struct arc_opcode *opcode = first_opcode;
  int ntok = *pntok;
  int got_cpu_match = 0;

  do
    {
      const unsigned char *opidx;
      const unsigned char *flgidx;
      int tokidx = 0;
      const expressionS *t;
      bfd_boolean fl_ignore = FALSE;

      pr_debug ("%s:%d: find_opcode_match: trying opcode 0x%8X\n",
		frag_now->fr_file, frag_now->fr_line, opcode->opcode);

      /* Don't match opcodes that don't exist on this architecture.  */
      if (!(opcode->cpu & arc_target))
	goto match_failed;

      got_cpu_match = 1;

      /* Check the operands. */
      for (opidx = opcode->operands; *opidx; ++opidx)
	{
	  const struct arc_operand *operand = &arc_operands[*opidx];

	  /* Only take input from real operands.  */
	  if (operand->flags & ARC_OPERAND_FAKE)
	    continue;

	  /* Search for potential ignored operands */
	  if (operand->flags & ARC_OPERAND_IGNORE)
	    fl_ignore = TRUE;

	tryagain:
	  /* When we expect input, make sure we have it.  */
	  if (tokidx >= ntok)
	    {
	      if (fl_ignore)
		{
		  ++ntok;
		  fl_ignore = FALSE;
		  /* Make a fake argument */
		  tok[tokidx].X_op =  O_constant;
		  tok[tokidx].X_add_number = 0;
		  goto tryagain;
		}
	      abort ();
	    }

	  /* Match operand type with expression type.  */
	  switch (operand->flags & ARC_OPERAND_TYPECHECK_MASK)
	    {
	    case ARC_OPERAND_IR:
	      /* Check to be a register. */
	      if (tok[tokidx].X_op != O_register
		  || !is_ir_num (tok[tokidx].X_add_number))
		goto match_failed;

	      /* If expect duplicate, make sure it is duplicate. */
	      if (operand->flags & ARC_OPERAND_DUPLICATE)
		{
		  /* check for duplicate. */
		  if (t->X_op != O_register
		      || !is_ir_num (t->X_add_number)
		      || (regno (t->X_add_number) !=
			  regno (tok[tokidx].X_add_number)))
		    goto match_failed;
		}

	      /* Special handling? */
	      if (operand->insert)
		{
		  const char *errmsg = NULL;
		  (*operand->insert)(0,
				     regno (tok[tokidx].X_add_number),
				     &errmsg);
		  if (errmsg)
		    goto match_failed;
		}
	      t = &tok[tokidx];
	      break;

	    case ARC_OPERAND_LIMM:
	    case ARC_OPERAND_SIGNED:
	    case ARC_OPERAND_UNSIGNED:
	      switch (tok[tokidx].X_op)
		{
		case O_illegal:
		case O_absent:
		case O_register:
		  goto match_failed;

		case O_constant:
		  /* Check the range. */
		  if (operand->bits != 32
		      && !(operand->flags & ARC_OPERAND_NCHK))
		    {
		      offsetT min, max, val;
		      val = tok[tokidx].X_add_number;

		      if (operand->flags & ARC_OPERAND_SIGNED)
			{
			  max = (1 << (operand->bits - 1)) - 1;
			  min = -(1 << (operand->bits - 1));
			}
		      else
			{
			  max = (1 << operand->bits) - 1;
			  min = 0;
			}

		      if (val < min || val > max)
			goto match_failed;

		      /* Check alignmets. */
		      if ((operand->flags & ARC_OPERAND_ALIGNED32)
			  && (val & 0x03))
			goto match_failed;

		      if ((operand->flags & ARC_OPERAND_ALIGNED16)
			  && (val & 0x01))
			goto match_failed;
		    }
		  else if (operand->flags & ARC_OPERAND_NCHK)
		    {
		      if (operand->insert)
			{
			  const char *errmsg = NULL;
			  (*operand->insert)(0,
					     tok[tokidx].X_add_number,
					     &errmsg);
			  if (errmsg)
			    goto match_failed;
			}
		      else
			goto match_failed;
		    }
		  break;

		default:
		  if (operand->default_reloc == 0)
		    goto match_failed; /* The operand needs relocation. */
		  break;
		}
	      /* If expect duplicate, make sure it is duplicate. */
	      if (operand->flags & ARC_OPERAND_DUPLICATE)
		{
		  if (t->X_op == O_illegal
		      || t->X_op == O_absent
		      || t->X_op == O_register
		      || (t->X_add_number != tok[tokidx].X_add_number))
		    goto match_failed;
		}
	      t = &tok[tokidx];
	      break;

	    default:
	      /* Everything else should have been fake.  */
	      abort ();
	    }

	  ++tokidx;
	}

      /* Check the flags. Iterate over the valid flag classes. */
      int lnflg = nflgs;

      for (flgidx = opcode->flags; *flgidx && lnflg; ++flgidx)
	{
	  /* Get a valid flag class*/
	  const struct arc_flag_class *cl_flags = &arc_flag_classes[*flgidx];
	  const unsigned *flgopridx;

	  for (flgopridx = cl_flags->flags; *flgopridx; ++flgopridx)
	    {
	      const struct arc_flag_operand *flg_operand = &arc_flag_operands[*flgopridx];
	      struct arc_flags *pflag = first_pflag;
	      int i;

	      for (i = 0; i < nflgs; i++, pflag++)
		{
		  /* Match against the parsed flags. */
		  if (!strcmp (flg_operand->name, pflag->name))
		    {
		      /*TODO: Check if it is duplicated. */
		      pflag->code = *flgopridx;
		      lnflg--;
		      break; /* goto next flag class and parsed flag. */
		    }
		}
	    }
	}
      /* Did I check all the parsed flags? */
      if (lnflg)
	goto match_failed;

      /* Possible match -- did we use all of our input?  */
      if (tokidx == ntok)
	{
	  *pntok = ntok;
	  return opcode;
	}

    match_failed:;
    }
  while (++opcode - arc_opcodes < (int) arc_num_opcodes
	 && !strcmp (opcode->name, first_opcode->name));

  if (*pcpumatch)
    *pcpumatch = got_cpu_match;

  return NULL;
}

/* Turn an opcode description and a set of arguments into
   an instruction and a fixup.  */
static void
assemble_insn (const struct arc_opcode *opcode,
	       const expressionS *tok,
	       int ntok,
	       const struct arc_flags *pflags,
	       int nflg,
	       struct arc_insn *insn,
	       extended_bfd_reloc_code_real_type reloc)
{
  const struct arc_operand *reloc_operand = NULL;
  const expressionS *reloc_exp = NULL;
  unsigned image;
  const unsigned char *argidx;
  int i;
  int tokidx = 0;
  unsigned char pcrel = 0;

  memset (insn, 0, sizeof (*insn));
  image = opcode->opcode;

  pr_debug ("%s:%d: assemble_insn: using opcode %x\n",
	    frag_now->fr_file, frag_now->fr_line, opcode->opcode);

  /* Handle operands. */
  for (argidx = opcode->operands; *argidx; ++argidx)
    {
      const struct arc_operand *operand = &arc_operands[*argidx];
      const expressionS *t = (const expressionS *) 0;

      if (operand->flags & ARC_OPERAND_DUPLICATE)
	{
	  /* Duplicate operand, already inserted. */
	  tokidx ++;
	  continue;
	}

      if (tokidx >= ntok)
	{
	  abort();
	}
      else
	t = &tok[tokidx++];

      /* Regardless if we have a reloc or not mark the instruction
	 limm if it is the case. */
      if (operand->flags & ARC_OPERAND_LIMM)
	insn->has_limm = 1;

      switch (t->X_op)
	{
	case O_register:
	  image = insert_operand (image, operand, regno (t->X_add_number),
				  NULL, 0);
	  break;

	case O_constant:
	  image = insert_operand (image, operand, t->X_add_number, NULL, 0);
	  /* FIXME! do I need this assert here? ex: limm,u6 gas_assert
	     (reloc_operand == NULL); */
	  reloc_operand = operand;
	  reloc_exp = t;
	  if (operand->flags & ARC_OPERAND_LIMM)
	    insn->limm = t->X_add_number;
	  break;

	default:
	  /* This operand needs a relocation. */
	  if (reloc == BFD_RELOC_UNUSED)
	    {
	      switch (t->X_md)
		{
		case O_gotoff:
		case O_gotpc:
		case O_plt:
		  /*FIXME! PLT reloc works for both bl/bl<cc>
		    instructions. Maybe a good idea is to separate
		    them. */
		  reloc = ARC_RELOC_TABLE(t->X_md)->reloc;
		  break;
		case O_sda:
		  /* Just consider the default relocation. */
		default:
		  reloc = operand->default_reloc;
		  break;
		}
	    }

	  gas_assert (reloc_operand == NULL);
	  reloc_operand = operand;
	  reloc_exp = t;

	  if (reloc > 0)
	    {
	      /* sanity checks */
	      reloc_howto_type *reloc_howto
		= bfd_reloc_type_lookup (stdoutput,
					 (bfd_reloc_code_real_type) reloc);
	      if (reloc_howto->bitsize != operand->bits)
		{
		  as_bad (_("invalid relocation for field"));
		  return;
		}
	    }
	  if (insn->nfixups >= MAX_INSN_FIXUPS)
	    as_fatal (_("too many fixups"));

	  struct arc_fixup *fixup;
	  fixup = &insn->fixups[insn->nfixups++];
	  fixup->exp = *t;
	  fixup->reloc = reloc;
	  pcrel = (operand->flags & ARC_OPERAND_PCREL) ?
	    1 : 0;
	  fixup->pcrel = pcrel;
	  break;
	}
    }

  /* Handle flags. */
  for (i = 0; i < nflg; i++)
    {
      const struct arc_flag_operand *flg_operand = &arc_flag_operands[pflags[i].code];

      /* There is an exceptional case when we cannot insert a flag
	 just as it is. The .T flag must be handled in relation with
	 the relative address . */
      if (!strcmp (flg_operand->name, "t") || !strcmp (flg_operand->name, "nt"))
	{
	  /* Check if we have a symbol or an solved immediate */
	  gas_assert (reloc_exp != NULL);
	  if (reloc_exp->X_op == O_constant)
	    {
	      offsetT val = reloc_exp->X_add_number;

	      if (!strcmp (opcode->name, "bbit0") || !strcmp (opcode->name, "bbit1"))
		val = flg_operand->code ? -val : val;
	      else
		val = flg_operand->code ? val : -val; /* BRcc case */

	      image |= insert_operand (image, &arc_operands[arc_fake_idx_Toperand],
				       val, NULL, 0);
	    }
	  else
	    {
	      struct arc_fixup *fixup;

	      if (insn->nfixups >= MAX_INSN_FIXUPS)
		as_fatal (_("too many fixups"));

	      fixup = &insn->fixups[insn->nfixups++];
	      fixup->exp = *reloc_exp;
	      fixup->reloc = -arc_fake_idx_Toperand; /*FIXME! the bbit/br case discriminator */
	      fixup->pcrel = pcrel;
	    }
	}
      else
	image |= (flg_operand->code & ((1 << flg_operand->bits) - 1))
	  << flg_operand->shift;
    }

  /* Short instruction? */
  insn->short_insn = (opcode->mask & 0xFFFF0000) ? 0 : 1;

  insn->insn = image;
}

/* Actually output an instruction with its fixup.  */
static void
emit_insn (struct arc_insn *insn)
{
  char *f;
  int i, offset = 0;

  pr_debug ("Emit insn: 0x%x\n", insn->insn);
  pr_debug ("\tSort    : 0x%d\n", insn->short_insn);
  pr_debug ("\tLong imm: 0x%lx\n", insn->limm);

  /* Write out the instruction.  */
  if (insn->short_insn)
    {
      if (insn->has_limm)
	{
	  f = frag_more (6);
	  md_number_to_chars (f, insn->insn, 2);
	  md_number_to_chars_midend (f + 2, insn->limm, 4);
	  offset = 2;
	  dwarf2_emit_insn (6);
	}
      else
	{
	  f = frag_more (2);
	  md_number_to_chars (f, insn->insn, 2);
	  dwarf2_emit_insn (2);
	}
    }
  else
    {
      if (insn->has_limm)
	{
	  f = frag_more (8);
	  md_number_to_chars_midend (f, insn->insn, 4);
	  md_number_to_chars_midend (f + 4, insn->limm, 4);
	  offset = 4;
	  dwarf2_emit_insn (8);
	}
      else
	{
	  f = frag_more (4);
	  md_number_to_chars_midend (f, insn->insn, 4);
	  dwarf2_emit_insn (4);
	}
    }

  /* Apply the fixups in order.  */
  for (i = 0; i < insn->nfixups; i++)
    {
      struct arc_fixup *fixup = &insn->fixups[i];
      int size, pcrel;
      fixS *fixP;

       /* Some fixups are only used internally and so have no howto.  */
      if ((int) fixup->reloc < 0)
	{
	  size = 4;
	  pcrel = fixup->pcrel;
	  pr_debug ("PCrel :%d\n", fixup->pcrel);
	}
      else
	{
	  reloc_howto_type *reloc_howto =
	    bfd_reloc_type_lookup (stdoutput,
				   (bfd_reloc_code_real_type) fixup->reloc);
	  gas_assert (reloc_howto);

	  size = bfd_get_reloc_size (reloc_howto);
	  pcrel = reloc_howto->pc_relative;
	}

      pr_debug ("%s:%d: emit_insn: new %s fixup\n",
		frag_now->fr_file, frag_now->fr_line,
		(fixup->reloc < 0) ? "Internal" :
		bfd_get_reloc_code_name (fixup->reloc));
      fixP = fix_new_exp (frag_now, f - frag_now->fr_literal + offset,
			  size, &fixup->exp, pcrel, fixup->reloc);
    }
}

/* Insert an operand value into an instruction.  */
static unsigned
insert_operand (unsigned insn,
		const struct arc_operand *operand,
		offsetT val,
		char *file,
		unsigned line)
{
  offsetT min = 0, max = 0;

  if (operand->bits != 32
      && !(operand->flags & ARC_OPERAND_NCHK)
      && !(operand->flags & ARC_OPERAND_FAKE))
    {
      /*FIXME*/
      if (operand->flags & ARC_OPERAND_SIGNED)
	{
	  max = (1 << (operand->bits - 1)) - 1;
	  min = -(1 << (operand->bits - 1));
	}
      else
	{
	  max = (1 << operand->bits) - 1;
	  min = 0;
	}

      if (val < min || val > max)
	as_bad_value_out_of_range (_("operand"), val, min, max, file, line);
    }

  pr_debug("insert field: %ld <= %ld <= %ld in 0x%08x\n",
	   min, val, max, insn);

  if ((operand->flags & ARC_OPERAND_ALIGNED32)
      && (val & 0x03))
    as_bad (_("Unaligned operand. Needs to be 32bit aligned"));

  if ((operand->flags & ARC_OPERAND_ALIGNED16)
      && (val & 0x01))
    as_bad (_("Unaligned operand. Needs to be 16bit aligned"));

  if (operand->insert)
    {
      const char *errmsg = NULL;

      insn = (*operand->insert) (insn, val, &errmsg);
      if (errmsg)
	as_warn ("%s", errmsg);
    }
  else
    {
      if (operand->flags & ARC_OPERAND_TRUNCATE)
	{
	  if (operand->flags & ARC_OPERAND_ALIGNED32)
	    val >>= 2;
	  if (operand->flags & ARC_OPERAND_ALIGNED16)
	    val >>= 1;
	}
      insn |= ((val & ((1 << operand->bits) - 1)) << operand->shift);
    }

  return insn;
}

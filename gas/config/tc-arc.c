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

#ifdef OBJ_ELF
#include "elf/arc.h"
#endif

/**************************************************************************/
/* Defines                                                                */
/**************************************************************************/

#define MAX_INSN_ARGS        3
#define MAX_FLAG_NAME_LENGHT 3
#define MAX_INSN_FIXUPS      2

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
static int byte_order = DEFAULT_BYTE_ORDER;

const pseudo_typeS md_pseudo_table[] =
  { {0, 0, 0} };

const char *md_shortopts = "";

struct option md_longopts[] =
  { {0, 0, 0, 0} };

size_t md_longopts_size = 0;

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
};

struct arc_insn
{
  unsigned int insn;
  int nfixups;
  struct arc_fixup fixups[MAX_INSN_FIXUPS];
  long sequence;
  unsigned char short_insn;
};

/* The cpu for which we are generating code.  */
static unsigned arc_target = ARC_OPCODE_BASE;
static const char *arc_target_name = "<all>";

/* The hash table of instruction opcodes.  */
static struct hash_control *arc_opcode_hash;

/* A table of CPU names and opcode sets.  */
static const struct cpu_type
{
  const char *name;
  unsigned flags;
}
cpu_types[] =
{
  { "ARCv2EM", ARC_OPCODE_BASE | ARC_OPCODE_ARCv2 },
  { "ARCv2HS", ARC_OPCODE_BASE | ARC_OPCODE_ARCv2 },
  { "all", ARC_OPCODE_BASE },
  { 0, 0 }
};

struct arc_flags
{
  unsigned gicu;
};

/* A table of the register symbols.  */
static symbolS *arc_register_table[64];

/**************************************************************************/
/* Functions implementation                                               */
/**************************************************************************/

static void assemble_tokens (const char *, const expressionS *, int, const struct arc_flags *);
static const struct arc_opcode *find_opcode_match (const struct arc_opcode *, const expressionS *,
						   int *, int *);
static void assemble_insn (const struct arc_opcode *, const expressionS *,int ntok, struct arc_insn *,
			   extended_bfd_reloc_code_real_type);
static void emit_insn (struct arc_insn *);

static unsigned insert_operand (unsigned, const struct arc_operand *, offsetT, char *, unsigned);


/* Parse the arguments to an opcode. */
static int
tokenize_arguments (char *str,
		    expressionS tok[],
		    int ntok)
{
  char *old_input_line_pointer;
  bfd_boolean saw_comma = FALSE;
  bfd_boolean saw_arg = FALSE;
  int num_args = 0;

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

	case '@':
	  /* FIXME! A relocation opernad has the following form
	     @sequence_number@reloation_type. */
	  break;

	default:
	  if (saw_arg && !saw_comma)
	    goto err;

	  expression (tok);
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

#if 0
/*Returns the opcode of an input string, if it exists. The isa DB should be ordered*/
static arc_opcode
arc_opcode_lookup (arc_isa isa,
		   const char *opname)
{
  ext_opcode = arc_ext_opcodes;
  std_opcode = arc_opcode_lookup_asm (opname);
  if (debug_file)
    fprintf(debug_file, "Matching %s\n", opname);

  /*Keep looking until we find a match.*/
  start=opname;
  for (opcode = (ext_opcode ? ext_opcode : std_opcode) ;
       opcode != NULL;
       opcode = (ARC_OPCODE_NEXT_ASM (opcode)
		 ? ARC_OPCODE_NEXT_ASM (opcode)
		 : (ext_opcode ? ext_opcode = NULL, std_opcode : NULL)))
    {
      /* Is this opcode supported by the selected cpu?  */
      if (!arc_opcode_supported (opcode))
	continue;

      arc_opcode_init_insert ();
      if (debug_file)
	fprintf(debug_file, "Trying syntax %s\n", opcode->syntax);

      for (str = start, syn = opcode->syntax; *syn != '\0';)
	{
	  const struct arc_operand *operand;

	}
    }

  if (!result)
    {
      as_bad(_("opcode \"%s\" not recognized", opname));
      return NULL;
    }

  return result->u.opcode;
}
#endif

#if 0
/* Parse the flags to a structure */
static int
tokenize_flags (char *str,
		struct arc_flags *pflags)
{
  char *old_input_line_pointer;
  bfd_boolean saw_flg = FALSE;
  bfd_boolean saw_dot = FALSE;
  int num_flags  = 0;
  size_t flgnamelen;
  char *flgname;

  flgname = xmalloc (MAX_FLAG_NAME_LENGHT + 1);

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

	  flgnamelen = strspn (input_line_pointer, "abcdefghilmnopqrstvwxz");
	  if (flgnamelen > MAX_FLAG_NAME_LENGHT)
	    goto err;
	  memcpy (flgname, input_line_pointer, flgnamelen);
	  flgname[flgnamelen] = '\0';
	  if (arc_flag_lookup (isa_flag, flgname, pflags) == -1)
	    goto err;

	  input_line_pointer += flgnamelen;
	  saw_dot = FALSE;
	  saw_flg = TRUE;
	  num_flag++;
	  break;
	}
    }

 fini:
  xfree (flgname);
  input_line_pointer = old_input_line_pointer;
  return num_flags;

 err:
  xfree (flgname);
  if (saw_dot)
    as_bad (_("extra dot"));
  else if (!saw_flg)
    as_bad (_("unrecognized flag"));
  else
    as_bad (_("failed to parse flags"));
  input_line_pointer = old_input_line_pointer;
  return -1;
}
#endif

/* The public interface to the instruction assembler.  */
void
md_assemble (char *str)
{
  char *opname;
  expressionS tok[MAX_INSN_ARGS];
  int ntok, nflg;
  size_t opnamelen;
  struct arc_flags flags;

  /* Split off the opcode.  */
  opnamelen = strspn (str, "abcdefghijklmnopqrstuvwxyz_012368");
  opname = xmalloc (opnamelen + 1);
  memcpy (opname, str, opnamelen);
  opname[opnamelen] = '\0';

#if 0
  /* Tokenize the flags */
  if ((nflg = tokenize_flags (str + opnamelen, &flags)) == -1)
    {
      as_bad (_("syntax error"));
      return;
    }
#endif

  /* Tokenize the rest of the line.  */
  if ((ntok = tokenize_arguments (str + opnamelen + nflg, tok, MAX_INSN_ARGS)) < 0)
    {
      as_bad (_("syntax error"));
      return;
    }

  /* Finish it off. */
  assemble_tokens (opname, tok, ntok, &flags);
}

/* Port-specific assembler initialization. This function is called
   once, at assembler startup time.  */
void
md_begin (void)
{
  unsigned int i;

  /* The endianness can be chosen "at the factory".  */
  target_big_endian = byte_order == BIG_ENDIAN;

  /* Set up a hash table for the instructions.  */
  arc_opcode_hash = hash_new ();
  if (arc_opcode_hash == NULL)
    as_fatal (_("Virtual memory exhausted"));

  /* Initialize the hash table with the insns */
  for (i = 0; i < arc_num_opcodes;)
    {
      const char *name, *retval, *slash;

      name = arc_opcodes[i].name;
      retval = hash_insert (arc_opcode_hash, name, (void *) &arc_opcodes[i]);
      if (retval)
	as_fatal (_("internal error: can't hash opcode `%s': %s"),
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
   endianness.  The size (N) -4 is used internally in tc-arc.c to
   indicate a 4-byte limm value, which is encoded as 'middle-endian'
   for a little-endian target.  */
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
md_pcrel_from (fixS *fixP)
{
}

/* Apply a fixup to the object code.  This is called for all the
   fixups we generated by the call to fix_new_exp, above.  In the call
   above we used a reloc code which was the largest legal reloc code
   plus the operand index.  Here we undo that to recover the operand
   index.  At this point all symbol values should be fully resolved,
   and we attempt to completely resolve the reloc.  If we can not do
   that, we determine the correct reloc code and put it back in the
   fixup.  */
void
md_apply_fix (fixS *fixP, valueT *valueP, segT seg)
{
}

int
md_estimate_size_before_relax (fragS *fragp ATTRIBUTE_UNUSED,
			       asection *seg ATTRIBUTE_UNUSED)
{
  as_fatal (_("md_estimate_size_before_relax\n"));
  return 1;
}

/* Translate internal representation of relocation info to BFD target
   format.  */
arelent *
tc_gen_reloc (asection *section ATTRIBUTE_UNUSED,
	      fixS *fixP)
{
}

/* Convert a machine dependent frag.  We never generate these.  */
void
md_convert_frag (bfd *abfd ATTRIBUTE_UNUSED,
		 asection *sec ATTRIBUTE_UNUSED,
		 fragS *fragp ATTRIBUTE_UNUSED)
{
  as_fatal (_("md_convert_frag\n"));
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

char *
md_atof (int type,
	 char *litP,
	 int *sizeP)
{
}

void
md_operand (expressionS *expressionP)
{
}

int
md_parse_option (int c, char *arg)
{
}

void
md_show_usage (FILE *stream)
{
}

/* Given an opcode name, pre-tockenized set of argumenst and the
   opcode flags, take it all the way through emission. */
static void
assemble_tokens (const char *opname,
		 const expressionS *tok,
		 int ntok,
		 const struct arc_flags *pflags)
{
  int found_something = 0;
  const struct arc_opcode *opcode;
  int cpumatch = 1;
  extended_bfd_reloc_code_real_type reloc = BFD_RELOC_UNUSED;

  /* Search opcodes. */
  opcode = (const struct arc_opcodes *) hash_find (arc_opcode_hash, opname);
  if (opcode)
    {
      found_something = 1;
      opcode = find_opcode_match (opcode, tok, &ntok, &cpumatch);
      if (opcode)
	{
	  struct arc_insn insn;
	  assemble_insn (opcode, tok, ntok, &insn, reloc);

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
	as_bad (_("inappropriate arguments for opcode `%s'"), opname);
      else
	as_bad (_("opcode `%s' not supported for target %s"), opname,
		arc_target_name);
    }
  else
    as_bad (_("unknown opcode `%s'"), opname);
}

/* Search forward through all variants of an opcode looking for a
   syntax match.  */
static const struct arc_opcode *
find_opcode_match (const struct arc_opcode *first_opcode,
		   const expressionS *tok,
		   int *pntok,
		   int *pcpumatch)
{
  const struct arc_opcode *opcode = first_opcode;
  int ntok = *pntok;
  int got_cpu_match = 0;

  do
    {
      const unsigned char *opidx;
      int tokidx = 0;

      /* Don't match opcodes that don't exist on this architecture.  */
      if (!(opcode->cpu & arc_target))
	goto match_failed;

      got_cpu_match = 1;

      for (opidx = opcode->operands; *opidx; ++opidx)
	{
	  const struct arc_operand *operand = &arc_operands[*opidx];

	  /* Only take input from real operands.  */
	  if (operand->flags & ARC_OPERAND_FAKE)
	    continue;

	  /* When we expect input, make sure we have it.  */
	  if (tokidx >= ntok)
	    {
	      if ((operand->flags & ARC_OPERAND_OPTIONAL_MASK) == 0)
		goto match_failed;
	      continue;
	    }

	  /* Match operand type with expression type.  */
	  switch (operand->flags & ARC_OPERAND_TYPECHECK_MASK)
	    {
	    case ARC_OPERAND_IR:
	      if (tok[tokidx].X_op != O_register
		  || !is_ir_num (tok[tokidx].X_add_number))
		goto match_failed;
	      break;
	    default:
	      /* Everything else should have been fake.  */
	      abort ();
	    }

	  ++tokidx;
	}

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
	       struct arc_insn *insn,
	       extended_bfd_reloc_code_real_type reloc)
{
  const struct arc_operand *reloc_operand = NULL;
  const expressionS *reloc_exp = NULL;
  unsigned image;
  const unsigned char *argidx;
  int tokidx = 0;

  memset (insn, 0, sizeof (*insn));
  image = opcode->opcode;

  for (argidx = opcode->operands; *argidx; ++argidx)
    {
      const struct arc_operand *operand = &arc_operands[*argidx];
      const expressionS *t = (const expressionS *) 0;

      if (tokidx >= ntok)
	{
	  abort();
	}
      else
	t = &tok[tokidx++];

      switch (t->X_op)
	{
	case O_register:
	  image = insert_operand (image, operand, regno (t->X_add_number),
				  NULL, 0);
	  break;

	case O_constant:
	  image = insert_operand (image, operand, t->X_add_number, NULL, 0);
	  gas_assert (reloc_operand == NULL);
	  reloc_operand = operand;
	  reloc_exp = t;
	  break;

	default:
	  break;
	}
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

  /* Write out the instruction.  */
  if (insn->short_insn)
    {
      f = frag_more (2);
      md_number_to_chars (f, insn->insn, 2);
    }
  else
    {
      f = frag_more (4);
      md_number_to_chars (f, insn->insn, 4);
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
  if (operand->bits != 32)
    {
      offsetT min, max;

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

  if (operand->insert)
    {
      const char *errmsg = NULL;

      insn = (*operand->insert) (insn, val, &errmsg);
      if (errmsg)
	as_warn ("%s", errmsg);
    }
  else
    insn |= ((val & ((1 << operand->bits) - 1)) << operand->shift);

  return insn;
}

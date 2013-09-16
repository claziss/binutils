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
#define MAX_INSN_FLGS        3
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
  long limm;
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
  /* Name of the parsed flag*/
  char name[MAX_FLAG_NAME_LENGHT];

  /* The code of the parsed flag. Valid when is not zero. */
  unsigned char code;
};

/* A table of the register symbols.  */
static symbolS *arc_register_table[64];

/**************************************************************************/
/* Functions implementation                                               */
/**************************************************************************/

static void assemble_tokens (const char *, const expressionS *, int, struct arc_flags *, int);
static const struct arc_opcode *find_opcode_match (const struct arc_opcode *, const expressionS *, int *,
						   struct arc_flags *, int, int *);
static void assemble_insn (const struct arc_opcode *, const expressionS *,int, const struct arc_flags *, int,
			   struct arc_insn *, extended_bfd_reloc_code_real_type);
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
  opnamelen = strspn (str, "abcdefghijklmnopqrstuvwxyz_012368");
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
  unsigned insn;

  switch (fixP->fx_r_type)
    {
    default:
      {
	const struct arc_operand *operand;

	if ((int) fixP->fx_r_type >= 0)
	  as_fatal (_("unhandled reloation type %s"),
		    bfd_get_reloc_code_name (fixP->fx_r_type));

	gas_assert (-(int) fixP->fx_r_type < (int) arc_num_operands);
	operand = &arc_operands[-(int) fixP->fx_r_type];

	/* The rest of these fixups needs to be completely resolved as
	   constants. */
	if (fixP->fx_addsy != 0
	    && S_GET_SEGMENT (fixP->fx_addsy) != absolute_section)
	  as_bad_where (fixP->fx_file, fixP->fx_line,
			_("non-absolute expression in constant field"));

	insn = bfd_getb32 (fixpos); //FIXME: endianess
	insn = insert_operand (insn, operand, (offsetT) value,
			       fixP->fx_file, fixP->fx_line);
      }
    }

  md_number_to_chars (fixpos, insn, fixP->fx_size);
  fixP->fx_done = 1;
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
		 struct arc_flags *pflags,
		 int nflgs)
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

/* Search forward through all variants of an opcode looking for a
   syntax match.  */
static const struct arc_opcode *
find_opcode_match (const struct arc_opcode *first_opcode,
		   const expressionS *tok,
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

	  /* When we expect input, make sure we have it.  */
	  if (tokidx >= ntok)
	    {
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
		  (*operand->insert)(NULL,
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
		default:
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
      struct arc_flags *pflag = first_pflag;

      for (flgidx = opcode->flags; *flgidx && lnflg; ++flgidx)
	{
	  /* Get a valid flag class*/
	  const struct arc_flag_class *cl_flags = &arc_flag_classes[*flgidx];
	  const unsigned *flgopridx;

	  for (flgopridx = cl_flags->flags; *flgopridx; ++flgopridx)
	    {
	      const struct arc_flag_operand *flg_operand = &arc_flag_operands[*flgopridx];

	      /* Match against the parsed flags. */
	      if (!strcmp (flg_operand->name, pflag->name))
		{
		  /*TODO: Check if it is duplicated. */
		  pflag->code = *flgopridx;
		  pflag++;
		  lnflg--;
		  break; /* goto next flag class and parsed flag. */
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
  unsigned i;
  int tokidx = 0;

  memset (insn, 0, sizeof (*insn));
  image = opcode->opcode;

  /* Handle operands. */
  for (argidx = opcode->operands; *argidx; ++argidx)
    {
      const struct arc_operand *operand = &arc_operands[*argidx];
      const expressionS *t = (const expressionS *) 0;

      if (operand->flags & ARC_OPERAND_DUPLICATE)
	{
	  /* Duplicate operand, already inserted. */
	  continue;
	}

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
	  if (operand->flags & ARC_OPERAND_LIMM)
	    insn->limm = t->X_add_number;
	  else
	    insn->limm = 0;
	  break;

	default:
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
	  struct arc_fixup *fixup;

	  if (insn->nfixups >= MAX_INSN_FIXUPS)
	    as_fatal (_("too many fixups"));

	  fixup = &insn->fixups[insn->nfixups++];
	  fixup->exp = *reloc_exp;
	  fixup->reloc = -arc_fake_idx_Toperand;
	}
      else
	image |= (flg_operand->code & ((1 << flg_operand->bits) - 1)) << flg_operand->shift;
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
  int i;

  /* Write out the instruction.  */
  if (insn->short_insn)
    {
      if (insn->limm)
	{
	  f = frag_more (6);
	  md_number_to_chars (f, insn->insn, 2);
	  md_number_to_chars (f + 2, insn->limm, 4);
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
      if (insn->limm)
	{
	  f = frag_more (8);
	  md_number_to_chars (f, insn->insn, 4);
	  md_number_to_chars (f + 4, insn->limm, 4);
	  dwarf2_emit_insn (8);
	}
      else
	{
	  f = frag_more (4);
	  md_number_to_chars (f, insn->insn, 4);
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
	  pcrel = 0;
	}
      fixP = fix_new_exp (frag_now, f - frag_now->fr_literal,
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
  if (operand->bits != 32 && !(operand->flags & ARC_OPERAND_FAKE))
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

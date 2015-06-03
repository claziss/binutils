/* tc-arc.h - Macros and type defines for the ARC.
   Copyright 2014 Synopsys Inc.

   Written by Claudiu Zissulescu (claziss@synopsys.com)

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 3,
   or (at your option) any later version.

   GAS is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
   the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

/* By convention, you should define this macro in the `.h' file.  For
   example, `tc-m68k.h' defines `TC_M68K'.  You might have to use this
   if it is necessary to add CPU specific code to the object format
   file.  */
#define TC_ARC

/* We want local label support.  */
#define LOCAL_LABELS_FB 1

/* This macro is the BFD architecture to pass to `bfd_set_arch_mach'.  */
#define TARGET_ARCH bfd_arch_arc

/* `extsym - .' expressions can be emitted using PC-relative relocs */
#define DIFF_EXPR_OK

#define REGISTER_PREFIX '%'

#undef  LITTLE_ENDIAN
#define LITTLE_ENDIAN   1234

#undef  BIG_ENDIAN
#define BIG_ENDIAN      4321

#ifdef TARGET_BYTES_BIG_ENDIAN

# define DEFAULT_TARGET_FORMAT  "elf32-bigarc"
# define DEFAULT_BYTE_ORDER     BIG_ENDIAN

#else
/* You should define this macro to be non-zero if the target is big
   endian, and zero if the target is little endian.  */
# define TARGET_BYTES_BIG_ENDIAN 0

# define DEFAULT_TARGET_FORMAT  "elf32-littlearc"
# define DEFAULT_BYTE_ORDER     LITTLE_ENDIAN

#endif /* TARGET_BYTES_BIG_ENDIAN */

#define DEFAULT_RELAX_STATE 1

/* The endianness of the target format may change based on command
   line arguments.  */
extern const char *arc_target_format;

/* This macro is the BFD target name to use when creating the output
   file.  This will normally depend upon the `OBJ_FMT' macro.  */
#define TARGET_FORMAT          arc_target_format

/* `md_short_jump_size'
   `md_long_jump_size'
   `md_create_short_jump'
   `md_create_long_jump'
   If `WORKING_DOT_WORD' is defined, GAS will not do broken word
   processing (*note Broken words::.).  Otherwise, you should set
   `md_short_jump_size' to the size of a short jump (a jump that is
   just long enough to jump around a long jmp) and
   `md_long_jump_size' to the size of a long jump (a jump that can go
   anywhere in the function), You should define
   `md_create_short_jump' to create a short jump around a long jump,
   and define `md_create_long_jump' to create a long jump.  */
#define WORKING_DOT_WORD

#define LISTING_HEADER         "ARC GAS "

/* The number of bytes to put into a word in a listing.  This affects
   the way the bytes are clumped together in the listing.  For
   example, a value of 2 might print `1234 5678' where a value of 1
   would print `12 34 56 78'.  The default value is 4.  */
#define LISTING_WORD_SIZE 2

/* If you define this macro, it should return the position from which
   the PC relative adjustment for a PC relative fixup should be
   made. On many processors, the base of a PC relative instruction is
   the next instruction, so this macro would return the length of an
   instruction, plus the address of the PC relative fixup. The latter
   can be calculated as fixp->fx_where + fixp->fx_frag->fr_address. */
extern long md_pcrel_from_section (struct fix *, segT);
#define MD_PCREL_FROM_SECTION(FIX, SEC) md_pcrel_from_section (FIX, SEC)

/* [ ] is index operator */
#define NEED_INDEX_OPERATOR

#define MAX_MEM_FOR_RS_ALIGN_CODE (1+2)

/* HANDLE_ALIGN called after all the assembly has been done,
   so we can fill in all the rs_align_code type frags with
   nop instructions.  */
#define HANDLE_ALIGN(FRAGP)	 arc_handle_align(FRAGP)

/* Values passed to md_apply_fix3 don't include the symbol value.  */
#define MD_APPLY_SYM_VALUE(FIX) 0

/* No shared lib support, so we don't need to ensure
   externally visible symbols can be overridden.  */
#define EXTERN_FORCE_RELOC 0

/* You may define this macro to generate a fixup for a data
   allocation pseudo-op.  */
#define TC_CONS_FIX_NEW(FRAG,OFF,LEN,EXP)	\
  fix_new_exp (FRAG, OFF, (int)LEN, EXP, 0,	\
	       LEN == 1 ? BFD_RELOC_8		\
	       : LEN == 2 ? BFD_RELOC_16	\
	       : LEN == 3 ? BFD_RELOC_24	\
	       : LEN == 4 ? BFD_RELOC_32	\
	       : LEN == 8 ? BFD_RELOC_64	\
	       : BFD_RELOC_NONE); /*FIXME! Add a special reloc here i.e. BFD_RELOC_ARC_LINKAGE */

#define GLOBAL_OFFSET_TABLE_NAME "_GLOBAL_OFFSET_TABLE_"
#define DYNAMIC_STRUCT_NAME "_DYNAMIC"

/* We need to take care of not having section relative fixups for the
   fixups with respect to Position Independent Code */
#define tc_fix_adjustable(FIX)  tc_arc_fix_adjustable(FIX)

/* This hook is required to parse register names as operands. */
#define md_parse_name(name, exp, m, c) arc_parse_name (name, exp)

extern int arc_parse_name (const char *, struct expressionS *);
extern int tc_arc_fix_adjustable (struct fix *);
extern void arc_handle_align (fragS* fragP);

/* ARC instructions, with operands and prefixes included, are a multiple
   of two bytes long.  */
#define DWARF2_LINE_MIN_INSN_LENGTH	2
/* The lr register is r28.  */
/* #define DWARF2_DEFAULT_RETURN_COLUMN	28 */
/* Registers are generally saved at negative offsets to the CFA.  */
/* #define DWARF2_CIE_DATA_ALIGNMENT	(-4) */

/* Define the NOPs (the first one is also used by generic code) */
#define NOP_OPCODE   0x000078E0
#define NOP_OPCODE_L 0x264A7000 /* mov 0,0 */

#define MAX_INSN_FIXUPS      2
#define MAX_INSN_ARGS        5
#define MAX_INSN_FLGS        3
#define MAX_FLAG_NAME_LENGHT 3

extern const relax_typeS md_relax_table[];
#define TC_GENERIC_RELAX_TABLE md_relax_table

/* Enum used to enumerate the relaxable ins operands. */
enum rlx_operand_type
{
  EMPTY = 0,
  REGISTER,
  REGISTER_S, /* Register for short instruction(s). */
  REGISTER_NO_GP, /* Is a register but not gp register specifically. */
  IMMEDIATE,
  BRACKET
};

enum arc_rlx_types
{
  ARC_RLX_NONE = 0,
  ARC_RLX_BL_S,
  ARC_RLX_BL,
  ARC_RLX_B_S,
  ARC_RLX_B,
  ARC_RLX_ADD_U3,
  ARC_RLX_ADD_U6,
  ARC_RLX_ADD_LIMM,
  ARC_RLX_LD_U7,
  ARC_RLX_LD_S9,
  ARC_RLX_LD_LIMM,
  ARC_RLX_MOV_U8,
  ARC_RLX_MOV_S12,
  ARC_RLX_MOV_LIMM,
  ARC_RLX_SUB_U3,
  ARC_RLX_SUB_U6,
  ARC_RLX_SUB_LIMM,
};

/* Structure for relaxable instruction that have to be swapped with a smaller
   alternative instruction. */
struct arc_relaxable_ins
{
  /* Mnemonic that should be checked. */
  const char *mnemonic_r;

  /* Operands that should be checked.
     Indexes of operands from operand array. */
  enum rlx_operand_type operands[6];

  /* Flags that should be checked. */
  unsigned flag_classes[5];

  /* Mnemonic (smaller) alternative to be used later for relaxation. */
  const char *mnemonic_alt;

  /* Index of operand that generic relaxation has to check. */
  unsigned opcheckidx;

  /* Base subtype index used. */
  enum arc_rlx_types subtype;
};

extern const struct arc_relaxable_ins arc_relaxable_insns[];
extern const unsigned arc_num_relaxable_ins;

struct arc_flags
{
  /* Name of the parsed flag*/
  char name[MAX_FLAG_NAME_LENGHT];

  /* The code of the parsed flag. Valid when is not zero. */
  unsigned char code;
};

/* Used to construct instructions at md_convert_frag stage of relaxation. */
struct arc_relax_type
{
  /* Dictates whether the pc-relativity should be kept in mind when relax_frag
     is called or whether the pc-relativity should be solved outside of
     relaxation. For clarification: BL(_S) and B(_S) use pcrel == 1 and
     ADD with a solvable expression as 3rd operand use pcrel == 0. */
  unsigned char pcrel;

  /* Expressions that dictate the operands. Used for re-assembling in
     md_convert_frag. */
  expressionS tok[MAX_INSN_ARGS];

  /* Number of tok (i.e. number of operands). Used for re-assembling in
     md_convert_frag. */
  int ntok;

  /* Flags of instruction. Used for re-assembling in md_convert_frag. */
  struct arc_flags pflags[MAX_INSN_FLGS];

  /* Number of flags. Used for re-assembling in md_convert_frag. */
  int nflg;
};

/* Used within frags to pass some information to some relaxation machine
   dependent values. */
#define TC_FRAG_TYPE struct arc_relax_type

/* Adjust non PC-rel values at relaxation time. */
#define TC_PCREL_ADJUST(F) arc_pcrel_adjust(F)
extern int arc_pcrel_adjust(fragS *);

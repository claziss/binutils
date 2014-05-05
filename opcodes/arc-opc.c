/* Opcode table for the ARC.
   Copyright 2013
   Free Software Foundation, Inc.

   This file is part of libopcodes.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.  */

#include "sysdep.h"
#include <stdio.h>
#include "opcode/arc.h"
#include "bfd.h"
#include "opintl.h"

/* Insert RB register into a 32-bit opcode. */
static unsigned
insert_rb (unsigned insn,
	   int value,
	   const char **errmsg ATTRIBUTE_UNUSED)
{
  return insn | ((value & 0x07) << 24) | (((value >> 3) & 0x07) << 12);
}

static int
extract_rb (unsigned insn ATTRIBUTE_UNUSED,
	    int *invalid ATTRIBUTE_UNUSED)
{
  int value = (((insn >> 12) & 0x07) << 3) | ((insn >> 27) & 0x07);

  return value;
}

/* Dummy insert LIMM function. */
static unsigned
insert_limm (unsigned insn,
	     int value ATTRIBUTE_UNUSED,
	     const char **errmsg ATTRIBUTE_UNUSED)
{
  return insn;
}

/* Dummy insert ZERO operand function. */
static unsigned
insert_za (unsigned insn,
	   int value,
	   const char **errmsg)
{
  if (value)
    *errmsg = _("operand is not zero");
  return insn;
}

/* Insert bbit's s9 immediate operand function. */
static unsigned
insert_bbs9 (unsigned insn,
	     int value,
	     const char **errmsg)
{
  if (value % 2)
    *errmsg = _("The target address must be 2-byte aligned");

  /* Insert least significant 7-bits.  */
  insn |= ((value >> 1) & 0x7f) << 17;
  /* Insert most significant bit.  */
  insn |= (((value >> 1) & 0x80) >> 7) << 15;
  return insn;
}

/* Insert bbit's s9 immediate operand function. */
static int
extract_bbs9 (unsigned insn,
	      int *invalid)
{
  int value;

  /* Insert least significant 7-bits.  */
  value = (insn >> 17) & 0x7f;
  /* Insert most significant bit.  */
  value |= ((insn >> 15) & 0x01) << 7;

  /* Fix the sign. */
  int signbit = 1 << 7;
  value = (value ^ signbit) - signbit;

  /* Correct it to s9. */
  return value << 1;
}

/* Insert Y-bit in bbit instructions. This function is called only
   when solving fixups. */
static unsigned
insert_Ybit (unsigned insn,
	     int value,
	     const char **errmsg)
{
  if (!value)
    *errmsg = _("cannot resolve this fixup.");
  else if (value > 0)
    insn |= 0x08;

  return insn;
}

/* Abbreviations for instruction subsets.  */
#define BASE			ARC_OPCODE_BASE
#define EM			ARC_OPCODE_ARCv2

/* The flag operands table.

   The format of the table is
   NAME CODE BITS SHIFT
*/
const struct arc_flag_operand arc_flag_operands[] =
  {
#define F_NULL  0
    { 0, 0, 0, 0},
#define F_ALWAYS    (F_NULL + 1)
    { "al", 0, 0, 0 },
#define F_RA        (F_ALWAYS + 1)
    { "ra", 0, 0, 0 },
#define F_EQUAL     (F_RA + 1)
    { "eq", 1, 5, 0 },
#define F_ZERO      (F_EQUAL + 1)
    { "z",  1, 5, 0 },
#define F_NOTEQUAL  (F_ZERO + 1)
    { "ne", 2, 5, 0 },
#define F_NOTZERO   (F_NOTEQUAL + 1)
    { "nz", 2, 5, 0 },
#define F_POZITIVE  (F_NOTZERO + 1)
    { "p",  3, 5, 0 },
#define F_PL        (F_POZITIVE + 1)
    { "pl", 3, 5, 0 },
#define F_NEGATIVE  (F_PL + 1)
    { "n",  4, 5, 0 },
#define F_MINUS     (F_NEGATIVE + 1)
    { "mi", 4, 5, 0 },
#define F_CARRY     (F_MINUS + 1)
    { "c",  5, 5, 0 },
#define F_CARRYSET  (F_CARRY + 1)
    { "cs", 5, 5, 0 },
#define F_LOWER     (F_CARRYSET + 1)
    { "lo", 5, 5, 0 },
#define F_CARRYCLR  (F_LOWER + 1)
    { "cc", 6, 5, 0 },
#define F_NOTCARRY (F_CARRYCLR + 1)
    { "nc", 6, 5, 0 },
#define F_HIGHER   (F_NOTCARRY + 1)
    { "hs", 6, 5, 0 },
#define F_OVERFLOWSET (F_HIGHER + 1)
    { "vs", 7, 5, 0 },
#define F_OVERFLOW (F_OVERFLOWSET + 1)
    { "v",  7, 5, 0 },
#define F_NOTOVERFLOW (F_OVERFLOW + 1)
    { "nv", 8, 5, 0 },
#define F_OVERFLOWCLR (F_NOTOVERFLOW + 1)
    { "vc", 8, 5, 0 },
#define F_GT       (F_OVERFLOWCLR + 1)
    { "gt", 9, 5, 0 },
#define F_GE       (F_GT + 1)
    { "ge", 10, 5, 0 },
#define F_LT       (F_GE + 1)
    { "lt", 11, 5, 0 },
#define F_LE       (F_LT + 1)
    { "le", 12, 5, 0 },
#define F_HI       (F_LE + 1)
    { "hi", 13, 5, 0 },
#define F_LS       (F_HI + 1)
    { "ls", 14, 5, 0 },
#define F_PNZ      (F_LS + 1)
    { "pnz", 15, 5, 0 },

    /* FLAG. */
#define F_FLAG     (F_PNZ + 1)
    { "f",  1, 1, 15 },

    /* Delay slot. */
#define F_ND       (F_FLAG + 1)
    { "nd", 0, 1, 5 },
#define F_D        (F_ND + 1)
    { "d",  1, 1, 5 },

    /* Data size. */
#define F_SIZEB1   (F_D + 1)
    { "b", 1, 2, 1 },
#define F_SIZEB7   (F_SIZEB1 + 1)
    { "b", 1, 2, 7 },
#define F_SIZEB17  (F_SIZEB7 + 1)
    { "b", 1, 2, 17 },
#define F_SIZEW1   (F_SIZEB17 + 1)
    { "w", 2, 2, 1 },
#define F_SIZEW7   (F_SIZEW1 + 1)
    { "w", 2, 2, 7 },
#define F_SIZEW17  (F_SIZEW7 + 1)
    { "w", 2, 2, 17 },

    /* Sign extension. */
#define F_SIGN6   (F_SIZEW17 + 1)
    { "x", 1, 1, 6 },
#define F_SIGN16   (F_SIGN6 + 1)
    { "x", 1, 1, 16 },

    /* Address write-back modes. */
#define F_A3       (F_SIGN16 + 1)
    { "a", 1, 2, 3 },
#define F_A9       (F_A3 + 1)
    { "a", 1, 2, 9 },
#define F_A22      (F_A9 + 1)
    { "a", 1, 2, 22 },
#define F_AW3      (F_A22 + 1)
    { "aw", 1, 2, 3 },
#define F_AW9      (F_AW3 + 1)
    { "aw", 1, 2, 9 },
#define F_AW22     (F_AW9 + 1)
    { "aw", 1, 2, 22 },
#define F_AB3      (F_AW22 + 1)
    { "ab", 2, 2, 3 },
#define F_AB9      (F_AB3 + 1)
    { "ab", 2, 2, 9 },
#define F_AB22     (F_AB9 + 1)
    { "ab", 2, 2, 22 },
#define F_AS3      (F_AB22 + 1)
    { "as", 3, 2, 3 },
#define F_AS9      (F_AS3 + 1)
    { "as", 3, 2, 9 },
#define F_AS22     (F_AS9 + 1)
    { "as", 3, 2, 22 },
    /*  { "as", 3, ADDRESS22S_AC, 0 },*/

    /* Cache bypass. */
#define F_DI5     (F_AS22 + 1)
    { "di", 1, 1, 5 },
#define F_DI11    (F_DI5 + 1)
    { "di", 1, 1, 11 },
#define F_DI15    (F_DI11 + 1)
    { "di", 1, 1, 15 },

    /*ARCv2 specific*/
#define F_NT     (F_DI15 + 1)
    { "nt", 0, 1, 3},
#define F_T      (F_NT + 1)
    { "t", 1, 1, 3},
#define F_H1     (F_T + 1)
    { "h", 2, 2, 1 },
#define F_H7     (F_H1 + 1)
    { "h", 2, 2, 7 },
#define F_H17    (F_H7 + 1)
    { "h", 2, 2, 17 }
  };
const unsigned arc_num_flag_operands = sizeof(arc_flag_operands)/sizeof(*arc_flag_operands);

/* Table of the flag classes.

   The format of the table is
   HASH_IDX {FLAG_CODE}
*/
const struct arc_flag_class arc_flag_classes[] =
  {
#define C_EMPTY     0
    { 0, { F_NULL } },

#define C_CC        (C_EMPTY + 1)
    { 0, { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO, F_POZITIVE, F_PL,
	   F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET, F_LOWER, F_CARRYCLR, F_NOTCARRY,
	   F_HIGHER, F_OVERFLOWSET, F_OVERFLOW, F_NOTOVERFLOW, F_OVERFLOWCLR, F_GT,
	   F_GE, F_LT, F_LE, F_HI, F_LS, F_PNZ, F_NULL } },

#define C_AA_ADDR3  (C_CC + 1)
    { 0, { F_A3, F_AW3, F_AB3, F_AS3, F_NULL } },
#define C_AA_ADDR9  (C_AA_ADDR3 + 1)
    { 0, { F_A9, F_AW9, F_AB9, F_AS9, F_NULL } },
#define C_AA_ADDR22 (C_AA_ADDR9 + 1)
    { 0, { F_A22, F_AW22, F_AB22, F_AS22, F_NULL } },

#define C_F         (C_AA_ADDR22 + 1)
    { 0, { F_FLAG, F_NULL } },

#define C_T         (C_F + 1)
    { 0, { F_NT, F_T } },
#define C_D         (C_T + 1)
    { 0, { F_ND, F_D } },
  };

/* Common combinations of FLAGS.  */
#define FLAGS_NONE              { 0 }
#define FLAGS_F                 { C_F }
#define FLAGS_CC                { C_CC }
#define FLAGS_CCF               { C_CC, C_F }

/* The operands table.

   The format of the operands table is:

   BITS SHIFT DEFAULT_RELOC FLAGS INSERT_FUN EXTRACT_FUN
*/
const struct arc_operand arc_operands[] =
  {
    /* The fields are bits, shift, insert, extract, flags */
    /* The zero index is used to indicate end-of-list */
#define UNUSED		0
    { 0, 0, 0, 0, 0, 0 },
    /* The plain integer register fields. */
#define RA		(UNUSED + 1)
    { 6, 0, 0, ARC_OPERAND_IR, 0, 0 },
#define RB		(RA + 1)
    { 6, 12, 0, ARC_OPERAND_IR, insert_rb, extract_rb },
#define RC		(RB + 1)
    { 6, 6, 0, ARC_OPERAND_IR, 0, 0 },
#define RBdup 		(RC + 1)
    { 6, 12, 0, ARC_OPERAND_IR | ARC_OPERAND_DUPLICATE, insert_rb, extract_rb },

    /* Long immediate. */
#define LIMM 		(RBdup + 1)
    { 32, 0, BFD_RELOC_ARC_32_ME, ARC_OPERAND_LIMM, insert_limm, 0 },
#define LIMMdup		(LIMM + 1)
    { 32, 0, 0, ARC_OPERAND_LIMM | ARC_OPERAND_DUPLICATE, insert_limm, 0 },

    /* Special operands. */
#define ZA              (LIMMdup + 1)
    { 0, 0, 0, ARC_OPERAND_UNSIGNED, insert_za, 0 },

    /* The signed "9-bit" immediate used for bbit instructions. */
#define BBS9            (ZA + 1)
    { 8, 17, -BBS9, ARC_OPERAND_SIGNED | ARC_OPERAND_PCREL, insert_bbs9, extract_bbs9 },
    /* Fake operand to handle the T flag. */
#define FKT             (BBS9 + 1)
    { 1, 3, 0, ARC_OPERAND_FAKE, insert_Ybit, 0},
  };
const unsigned arc_num_operands = sizeof(arc_operands)/sizeof(*arc_operands);

const unsigned arc_fake_idx_Toperand = FKT;

/* Common combinations of arguments.  */
#define ARG_NONE                { 0 }
#define ARG_32BIT_RARBRC        { RA, RB, RC }
#define ARG_32BIT_RALIMMRC      { RA, LIMM, RC }
#define ARG_32BIT_RARBLIMM      { RA, RB, LIMM }
#define ARG_32BIT_RALIMMLIMM    { RA, LIMM, LIMMdup }
#define ARG_32BIT_RBRBRC        { RB, RBdup, RC }
#define ARG_32BIT_ZARBRC        { ZA, RB, RC }

/* The opcode table.

   The format of the opcode table is:

   NAME OPCODE MASK CPU { OPERANDS } { FLAGS }
*/
const struct arc_opcode arc_opcodes[] =
  {
    { "add", 0x20000000, 0xF8FF0000, BASE, ARG_32BIT_RARBRC, FLAGS_F },
    { "add", 0x2000003E, 0xF8FF003F, BASE, ARG_32BIT_ZARBRC, FLAGS_F },
    { "add", 0x26007000, 0xFFFF7000, BASE, ARG_32BIT_RALIMMRC, FLAGS_F },
    { "add", 0x20000F80, 0xF8FF0FC0, BASE, ARG_32BIT_RARBLIMM, FLAGS_F },
    { "add", 0x26007F80, 0xFFFF7FC0, BASE, ARG_32BIT_RALIMMLIMM, FLAGS_F },
    { "add", 0x20C00000, 0xF8FF0000, BASE, ARG_32BIT_RBRBRC, FLAGS_CCF },
    { "nop", 0x264A7000, 0xFFFFFFFF, BASE, ARG_NONE, FLAGS_NONE },
    { "nop_s", 0x78E0, 0xFFFF, BASE, ARG_NONE, FLAGS_NONE },
    { "bbit0", 0x8010006, 0xF8010017, BASE, { RB, RC, BBS9 }, { C_T, C_D } },
  };

const unsigned arc_num_opcodes = sizeof(arc_opcodes)/sizeof(*arc_opcodes);

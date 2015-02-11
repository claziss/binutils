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
  int value = (((insn >> 12) & 0x07) << 3) | ((insn >> 24) & 0x07);

  if (value == 0x3e && invalid)
    *invalid = 1; /* limm should be extracted in a different way. */

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
	      int *invalid ATTRIBUTE_UNUSED)
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

/* Insert the signed 12-bit immediate. */
static unsigned
insert_simm12 (unsigned insn,
	     int value,
	     const char **errmsg)
{
  if ((value < -2048) || (value > 2047))
    *errmsg = _("out of range.");
  else
    insn |= ((value & 0x3f) << 6) | ((value >> 6) & 0x3f);

  return insn;
}

/* Insert bbit's s9 immediate operand function. */
static int
extract_simm12 (unsigned insn,
		int *invalid ATTRIBUTE_UNUSED)
{
  int value = (((insn & 0x3f) << 6) | ((insn >> 6) & 0x3f));

  /* Fix the sign. */
  int signbit = 1 << 11;
  value = (value ^ signbit) - signbit;

  return value;
}


/* Insert the unsigned 6-bit immediate. */
static unsigned
insert_uimm6s (unsigned insn,
	     int value,
	     const char **errmsg ATTRIBUTE_UNUSED)
{
  insn |= ((value & 0x38) << 1) | (value & 0x07);

  return insn;
}

/* Extract unsigned 6-bit immediate */
static int
extract_uimm6s (unsigned insn,
		int *invalid ATTRIBUTE_UNUSED)
{
  int value = ((insn & 0x70) >> 1) | (insn & 0x07);

  return value;
}


/* Insert H register into a 16-bit opcode. */
static unsigned
insert_rhv1 (unsigned insn,
	   int value,
	   const char **errmsg ATTRIBUTE_UNUSED)
{
  return insn |= ((value & 0x07) << 5) | ((value >> 3) & 0x07);
}

static int
extract_rhv1 (unsigned insn ATTRIBUTE_UNUSED,
	      int *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  return value;
}

/* Insert H register into a 16-bit opcode. */
static unsigned
insert_rhv2 (unsigned insn,
	     int value,
	     const char **errmsg)
{
  if (value == 0x1E)
    *errmsg = _("Register R30 is a limm indicator for this type of instruction.");
  return insn |= ((value & 0x07) << 5) | ((value >> 3) & 0x03);
}

static int
extract_rhv2 (unsigned insn ATTRIBUTE_UNUSED,
	      int *invalid ATTRIBUTE_UNUSED)
{
  int value = ((insn >> 5) & 0x07) | ((insn & 0x03) << 3);

  return value;
}


static unsigned
insert_r0 (unsigned insn,
	   int value,
	   const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value != 0)
    *errmsg = _("Register must be R0.");
  return insn;
}

static int
extract_r0 (unsigned insn ATTRIBUTE_UNUSED,
	      int *invalid ATTRIBUTE_UNUSED)
{
  return 0;
}


static unsigned
insert_r1 (unsigned insn,
	   int value,
	   const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value != 1)
    *errmsg = _("Register must be R1.");
  return insn;
}

static int
extract_r1 (unsigned insn ATTRIBUTE_UNUSED,
	      int *invalid ATTRIBUTE_UNUSED)
{
  return 1;
}

static unsigned
insert_r2 (unsigned insn,
	   int value,
	   const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value != 2)
    *errmsg = _("Register must be R2.");
  return insn;
}

static int
extract_r2 (unsigned insn ATTRIBUTE_UNUSED,
	      int *invalid ATTRIBUTE_UNUSED)
{
  return 2;
}

static unsigned
insert_r3 (unsigned insn,
	   int value,
	   const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value != 3)
    *errmsg = _("Register must be R3.");
  return insn;
}

static int
extract_r3 (unsigned insn ATTRIBUTE_UNUSED,
	      int *invalid ATTRIBUTE_UNUSED)
{
  return 3;
}

static unsigned
insert_sp (unsigned insn,
	   int value,
	   const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value != 28)
    *errmsg = _("Register must be SP.");
  return insn;
}

static int
extract_sp (unsigned insn ATTRIBUTE_UNUSED,
	      int *invalid ATTRIBUTE_UNUSED)
{
  return 28;
}

static unsigned
insert_gp (unsigned insn,
	   int value,
	   const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value != 26)
    *errmsg = _("Register must be GP.");
  return insn;
}

static int
extract_gp (unsigned insn ATTRIBUTE_UNUSED,
	    int *invalid ATTRIBUTE_UNUSED)
{
  return 26;
}

static unsigned
insert_blink (unsigned insn,
	      int value,
	      const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value != 31)
    *errmsg = _("Register must be BLINK.");
  return insn;
}

static int
extract_blink (unsigned insn ATTRIBUTE_UNUSED,
	       int *invalid ATTRIBUTE_UNUSED)
{
  return 31;
}


static unsigned
insert_ras (unsigned insn,
	      int value,
	      const char **errmsg ATTRIBUTE_UNUSED)
{
  switch (value)
    {
    case 0:
    case 1:
    case 2:
    case 3:
      insn |= value;
      break;
    case 12:
    case 13:
    case 14:
    case 15:
      insn |= (value - 8);
      break;
    default:
      *errmsg = _("Register must be either r0-r3 or r12-r15.");
      break;
    }
  return insn;
}

static int
extract_ras (unsigned insn ATTRIBUTE_UNUSED,
	     int *invalid ATTRIBUTE_UNUSED)
{
  int value = insn & 0x07;
  if (value > 3)
    return (value + 8);
  else
    return value;
}

static unsigned
insert_rbs (unsigned insn,
	      int value,
	      const char **errmsg ATTRIBUTE_UNUSED)
{
  switch (value)
    {
    case 0:
    case 1:
    case 2:
    case 3:
      insn |= value << 8;
      break;
    case 12:
    case 13:
    case 14:
    case 15:
      insn |= ((value - 8)) << 8;
      break;
    default:
      *errmsg = _("Register must be either r0-r3 or r12-r15.");
      break;
    }
  return insn;
}

static int
extract_rbs (unsigned insn ATTRIBUTE_UNUSED,
	     int *invalid ATTRIBUTE_UNUSED)
{
  int value = (insn >> 8) & 0x07;
  if (value > 3)
    return (value + 8);
  else
    return value;
}

static unsigned
insert_rcs (unsigned insn,
	      int value,
	      const char **errmsg ATTRIBUTE_UNUSED)
{
  switch (value)
    {
    case 0:
    case 1:
    case 2:
    case 3:
      insn |= value << 5;
      break;
    case 12:
    case 13:
    case 14:
    case 15:
      insn |= ((value - 8)) << 5;
      break;
    default:
      *errmsg = _("Register must be either r0-r3 or r12-r15.");
      break;
    }
  return insn;
}

static int
extract_rcs (unsigned insn ATTRIBUTE_UNUSED,
	     int *invalid ATTRIBUTE_UNUSED)
{
  int value = (insn >> 5) & 0x07;
  if (value > 3)
    return (value + 8);
  else
    return value;
}

static unsigned
insert_uimm3s (unsigned insn,
	       int value,
	       const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value < 1 || value > 7)
    *errmsg = _("The operand accepts only values from 1 to 7.");

  insn |= value & 0x07;

  return insn;
}

static unsigned
insert_simm3s (unsigned insn,
	       int value,
	       const char **errmsg ATTRIBUTE_UNUSED)
{
  int tmp = 0;
  switch (value)
    {
    case -1:
      tmp = 0x07;
      break;
    case 0:
      tmp = 0x00;
      break;
    case 1:
      tmp = 0x01;
      break;
    case 2:
      tmp = 0x02;
      break;
    case 3:
      tmp = 0x03;
      break;
    case 4:
      tmp = 0x04;
      break;
    case 5:
      tmp = 0x05;
      break;
    case 6:
      tmp = 0x06;
      break;
    default:
      *errmsg = _("Accepted values are from -1 to 6.");
      break;
    }

  insn |= tmp << 8;
  return insn;
}

static int
extract_simm3s (unsigned insn ATTRIBUTE_UNUSED,
		int *invalid ATTRIBUTE_UNUSED)
{
  int value = (insn >> 8) & 0x07;
  if (value == 7)
    return -1;
  else
    return value;
}

static unsigned
insert_simm25_16 (unsigned insn,
		  int value,
		  const char **errmsg ATTRIBUTE_UNUSED)
{
  int tmp;

  if (value & 0x01)
    *errmsg = _("Target address is not 16bit aligned.");

  tmp = value >> 1;
  insn |= (tmp & 0x3FF) << 16;
  insn |= ((tmp >> 10) & 0x3FF) << 6;
  insn |= (tmp >> 20) & 0x0F;
  return insn;
}

static int
extract_simm25_16 (unsigned insn,
		   int *invalid ATTRIBUTE_UNUSED)
{
  int value;

  value = (insn >> 16) & 0x3FF;
  value |= ((insn >> 6) & 0x3FF) << 10;
  value |= (insn & 0x0F) << 20;

  value = value << 1;

  /* Fix the sign. */
  int signbit = 1 << 25;
  value = (value ^ signbit) - signbit;

  return value;
}

static unsigned
insert_simm25_32 (unsigned insn,
		  int value,
		  const char **errmsg ATTRIBUTE_UNUSED)
{
  int tmp;

  if (value & 0x02)
    *errmsg = _("Target address is not 32bit aligned.");

  tmp = value >> 2;
  insn |= (tmp & 0x1FF) << 17;
  insn |= ((tmp >> 10) & 0x3FF) << 6;
  insn |= (tmp >> 20) & 0x0F;
  return insn;
}

static int
extract_simm25_32 (unsigned insn,
		   int *invalid ATTRIBUTE_UNUSED)
{
  int value;

  value = (insn >> 17) & 0x1FF;
  value |= ((insn >> 5) & 0x3FF) << 10;
  value |= (insn & 0x0F) << 20;

  value = value << 2;

  /* Fix the sign. */
  int signbit = 1 << 25;
  value = (value ^ signbit) - signbit;

  return value;
}

/**/
static unsigned
insert_simm21_16 (unsigned insn,
		  int value,
		  const char **errmsg ATTRIBUTE_UNUSED)
{
  int tmp;

  if (value & 0x01)
    *errmsg = _("Target address is not 16bit aligned.");

  tmp = value >> 1;
  insn |= (tmp & 0x3FF) << 16;
  insn |= ((tmp >> 10) & 0x3FF) << 6;
  return insn;
}

static int
extract_simm21_16 (unsigned insn,
		   int *invalid ATTRIBUTE_UNUSED)
{
  int value;

  value = (insn >> 16) & 0x3FF;
  value |= ((insn >> 6) & 0x3FF) << 10;
  value |= (insn & 0x0F) << 20;

  value = value << 1;

  /* Fix the sign. */
  int signbit = 1 << 25;
  value = (value ^ signbit) - signbit;

  return value;
}

static unsigned
insert_simm21_32 (unsigned insn,
		  int value,
		  const char **errmsg ATTRIBUTE_UNUSED)
{
  int tmp;

  if (value & 0x02)
    *errmsg = _("Target address is not 32bit aligned.");

  tmp = value >> 2;
  insn |= (tmp & 0x1FF) << 17;
  insn |= ((tmp >> 10) & 0x3FF) << 6;
  return insn;
}

static int
extract_simm21_32 (unsigned insn,
		   int *invalid ATTRIBUTE_UNUSED)
{
  int value;

  value = (insn >> 17) & 0x1FF;
  value |= ((insn >> 6) & 0x3FF) << 10;
  value |= (insn & 0x0F) << 20;

  value = value << 2;

  /* Fix the sign. */
  int signbit = 1 << 25;
  value = (value ^ signbit) - signbit;

  return value;
}


/* Abbreviations for instruction subsets.  */
#define BASE			ARC_OPCODE_BASE

/* The flag operands table.

   The format of the table is
   NAME CODE BITS SHIFT FAVAIL
*/
const struct arc_flag_operand arc_flag_operands[] =
  {
#define F_NULL  0
    { 0, 0, 0, 0, 0},
#define F_ALWAYS    (F_NULL + 1)
    { "al", 0, 0, 0, 0 },
#define F_RA        (F_ALWAYS + 1)
    { "ra", 0, 0, 0, 0 },
#define F_EQUAL     (F_RA + 1)
    { "eq", 1, 5, 0, 1 },
#define F_ZERO      (F_EQUAL + 1)
    { "z",  1, 5, 0, 0 },
#define F_NOTEQUAL  (F_ZERO + 1)
    { "ne", 2, 5, 0, 1 },
#define F_NOTZERO   (F_NOTEQUAL + 1)
    { "nz", 2, 5, 0, 0 },
#define F_POZITIVE  (F_NOTZERO + 1)
    { "p",  3, 5, 0, 1 },
#define F_PL        (F_POZITIVE + 1)
    { "pl", 3, 5, 0, 0 },
#define F_NEGATIVE  (F_PL + 1)
    { "n",  4, 5, 0, 1 },
#define F_MINUS     (F_NEGATIVE + 1)
    { "mi", 4, 5, 0, 0 },
#define F_CARRY     (F_MINUS + 1)
    { "c",  5, 5, 0, 1 },
#define F_CARRYSET  (F_CARRY + 1)
    { "cs", 5, 5, 0, 0 },
#define F_LOWER     (F_CARRYSET + 1)
    { "lo", 5, 5, 0, 0 },
#define F_CARRYCLR  (F_LOWER + 1)
    { "cc", 6, 5, 0, 0 },
#define F_NOTCARRY (F_CARRYCLR + 1)
    { "nc", 6, 5, 0, 1 },
#define F_HIGHER   (F_NOTCARRY + 1)
    { "hs", 6, 5, 0, 0 },
#define F_OVERFLOWSET (F_HIGHER + 1)
    { "vs", 7, 5, 0, 0 },
#define F_OVERFLOW (F_OVERFLOWSET + 1)
    { "v",  7, 5, 0, 1 },
#define F_NOTOVERFLOW (F_OVERFLOW + 1)
    { "nv", 8, 5, 0, 1 },
#define F_OVERFLOWCLR (F_NOTOVERFLOW + 1)
    { "vc", 8, 5, 0, 0 },
#define F_GT       (F_OVERFLOWCLR + 1)
    { "gt", 9, 5, 0, 1 },
#define F_GE       (F_GT + 1)
    { "ge", 10, 5, 0, 1 },
#define F_LT       (F_GE + 1)
    { "lt", 11, 5, 0, 1 },
#define F_LE       (F_LT + 1)
    { "le", 12, 5, 0, 1 },
#define F_HI       (F_LE + 1)
    { "hi", 13, 5, 0, 1 },
#define F_LS       (F_HI + 1)
    { "ls", 14, 5, 0, 1 },
#define F_PNZ      (F_LS + 1)
    { "pnz", 15, 5, 0, 1 },

    /* FLAG. */
#define F_FLAG     (F_PNZ + 1)
    { "f",  1, 1, 15, 1 },

    /* Delay slot. */
#define F_ND       (F_FLAG + 1)
    { "nd", 0, 1, 5, 0 },
#define F_D        (F_ND + 1)
    { "d",  1, 1, 5, 1 },

    /* Data size. */
#define F_SIZEB1   (F_D + 1)
    { "b", 1, 2, 1, 0 },
#define F_SIZEB7   (F_SIZEB1 + 1)
    { "b", 1, 2, 7, 0 },
#define F_SIZEB17  (F_SIZEB7 + 1)
    { "b", 1, 2, 17, 0 },
#define F_SIZEW1   (F_SIZEB17 + 1)
    { "w", 2, 2, 1, 0 },
#define F_SIZEW7   (F_SIZEW1 + 1)
    { "w", 2, 2, 7, 0 },
#define F_SIZEW17  (F_SIZEW7 + 1)
    { "w", 2, 2, 17, 0 },

    /* Sign extension. */
#define F_SIGN6   (F_SIZEW17 + 1)
    { "x", 1, 1, 6, 1 },
#define F_SIGN16   (F_SIGN6 + 1)
    { "x", 1, 1, 16, 1 },

    /* Address write-back modes. */
#define F_A3       (F_SIGN16 + 1)
    { "a", 1, 2, 3, 1 },
#define F_A9       (F_A3 + 1)
    { "a", 1, 2, 9, 1 },
#define F_A22      (F_A9 + 1)
    { "a", 1, 2, 22, 1 },
#define F_AW3      (F_A22 + 1)
    { "aw", 1, 2, 3, 1 },
#define F_AW9      (F_AW3 + 1)
    { "aw", 1, 2, 9, 1 },
#define F_AW22     (F_AW9 + 1)
    { "aw", 1, 2, 22, 1 },
#define F_AB3      (F_AW22 + 1)
    { "ab", 2, 2, 3, 1 },
#define F_AB9      (F_AB3 + 1)
    { "ab", 2, 2, 9, 1 },
#define F_AB22     (F_AB9 + 1)
    { "ab", 2, 2, 22, 1 },
#define F_AS3      (F_AB22 + 1)
    { "as", 3, 2, 3, 1 },
#define F_AS9      (F_AS3 + 1)
    { "as", 3, 2, 9, 1 },
#define F_AS22     (F_AS9 + 1)
    { "as", 3, 2, 22, 1 },
    /*  { "as", 3, ADDRESS22S_AC, 0 },*/

    /* Cache bypass. */
#define F_DI5     (F_AS22 + 1)
    { "di", 1, 1, 5, 1 },
#define F_DI11    (F_DI5 + 1)
    { "di", 1, 1, 11, 1 },
#define F_DI15    (F_DI11 + 1)
    { "di", 1, 1, 15, 1 },

    /*ARCv2 specific*/
#define F_NT     (F_DI15 + 1)
    { "nt", 0, 1, 3, 1},
#define F_T      (F_NT + 1)
    { "t", 1, 1, 3, 1},
#define F_H1     (F_T + 1)
    { "h", 2, 2, 1, 1 },
#define F_H7     (F_H1 + 1)
    { "h", 2, 2, 7, 1 },
#define F_H17    (F_H7 + 1)
    { "h", 2, 2, 17, 1 }
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
    { 0, { F_NT, F_T, F_NULL } },
#define C_D         (C_T + 1)
    { 0, { F_ND, F_D, F_NULL } },

#define C_DHARD    (C_D + 1)
    { 0, { F_D, F_NULL } },
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
    /* The plain integer register fields. Used by 32 bit instructions. */
#define RA		(UNUSED + 1)
    { 6, 0, 0, ARC_OPERAND_IR, 0, 0 },
#define RB		(RA + 1)
    { 6, 12, 0, ARC_OPERAND_IR, insert_rb, extract_rb },
#define RC		(RB + 1)
    { 6, 6, 0, ARC_OPERAND_IR, 0, 0 },
#define RBdup 		(RC + 1)
    { 6, 12, 0, ARC_OPERAND_IR | ARC_OPERAND_DUPLICATE, insert_rb, extract_rb },

    /* The plain integer register fields. Used by short instructions. */
#define RA16		(RBdup + 1)
    { 4, 0, 0, ARC_OPERAND_IR, insert_ras, extract_ras },
#define RB16		(RA16 + 1)
    { 4, 8, 0, ARC_OPERAND_IR, insert_rbs, extract_rbs },
#define RB16dup		(RB16 + 1)
    { 4, 8, 0, ARC_OPERAND_IR | ARC_OPERAND_DUPLICATE, insert_rbs, extract_rbs },
#define RC16		(RB16dup + 1)
    { 4, 5, 0, ARC_OPERAND_IR, insert_rcs, extract_rcs },
#define R6H             (RC16 + 1)   /* 6bit register field 'h' used by V1 cpus*/
    { 6, 5, 0, ARC_OPERAND_IR, insert_rhv1, extract_rhv1 },
#define R5H             (R6H + 1)    /* 5bit register field 'h' used by V2 cpus*/
    { 5, 5, 0, ARC_OPERAND_IR, insert_rhv2, extract_rhv2 },
#define R5Hdup          (R5H + 1)
    { 5, 5, 0, ARC_OPERAND_IR | ARC_OPERAND_DUPLICATE, insert_rhv2, extract_rhv2 },

    /* Fix registers. */
#define R0              (R5Hdup + 1)
    { 0, 0, 0, ARC_OPERAND_IR, insert_r0, extract_r0 },
#define R1              (R0 + 1)
    { 1, 0, 0, ARC_OPERAND_IR, insert_r1, extract_r1 },
#define R2              (R1 + 1)
    { 2, 0, 0, ARC_OPERAND_IR, insert_r2, extract_r2 },
#define R3              (R2 + 1)
    { 2, 0, 0, ARC_OPERAND_IR, insert_r3, extract_r3 },
#define SP              (R3 + 1)
    { 5, 0, 0, ARC_OPERAND_IR, insert_sp, extract_sp },
#define SPdup           (SP + 1)
    { 5, 0, 0, ARC_OPERAND_IR | ARC_OPERAND_DUPLICATE, insert_sp, extract_sp },
#define GP              (SPdup + 1)
    { 5, 0, 0, ARC_OPERAND_IR, insert_gp, extract_gp },
#define BLINK           (GP + 1)
    { 5, 0, 0, ARC_OPERAND_IR, insert_blink, extract_blink },


    /* Long immediate. */
#define LIMM 		(BLINK + 1)
    { 32, 0, BFD_RELOC_ARC_32_ME, ARC_OPERAND_LIMM, insert_limm, 0 },
#define LIMMdup		(LIMM + 1)
    { 32, 0, 0, ARC_OPERAND_LIMM | ARC_OPERAND_DUPLICATE, insert_limm, 0 },

    /* Special operands. */
#define ZA              (LIMMdup + 1)
    { 0, 0, 0, ARC_OPERAND_UNSIGNED, insert_za, 0 },

    /* The signed "9-bit" immediate used for bbit instructions. */
#define BBS9            (ZA + 1)
    { 8, 17, -BBS9, ARC_OPERAND_SIGNED | ARC_OPERAND_PCREL | ARC_OPERAND_ALIGNED16,
      insert_bbs9, extract_bbs9 },
    /* Fake operand to handle the T flag. */
#define FKT             (BBS9 + 1)
    { 1, 3, 0, ARC_OPERAND_FAKE, insert_Ybit, 0 },

    /* The unsigned 6-bit immediate used in most arithmetic instructions. */
#define UIMM6           (FKT + 1)
    { 6, 6, 0, ARC_OPERAND_UNSIGNED, 0, 0 },
    /* 12-bit signed immediate value */
#define SIMM12          (UIMM6 + 1)
    { 12, 6, 0, ARC_OPERAND_SIGNED, insert_simm12, extract_simm12 },
    /* The unsigned 7-bit immediate. */
#define UIMM7           (SIMM12 + 1)
    { 7, 0, 0, ARC_OPERAND_UNSIGNED, 0, 0 },
    /* The unsigned 7-bit immediate. */
#define UIMM7S32        (UIMM7 + 1)
    { 7, 0, 0, ARC_OPERAND_UNSIGNED | ARC_OPERAND_ALIGNED32
      | ARC_OPERAND_TRUNCATE, 0, 0 },
    /* The signed 3-bit immediate used by short insns that encodes the
       following values: -1, 0, 1, 2, 3, 4, 5, 6.  The number of bits
       is dummy, just to pass the initial range checks when picking
       the opcode.*/
#define SIMM3S          (UIMM7S32 + 1)
    { 4, 8, 0, ARC_OPERAND_SIGNED, insert_simm3s, extract_simm3s },
    /* The unsigned 6-bit immediate used by short insns. */
#define UIMM6S          (SIMM3S + 1)
    { 6, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm6s, extract_uimm6s },
    /* The signed 11-bit immediate used by short insns, 32bit aligned! */
#define SIMM11S32       (UIMM6S + 1)
    { 11, 0, 0, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED32
      | ARC_OPERAND_TRUNCATE, 0, 0 },
    /* The unsigned 3-bit immediate used by short insns. The value
       zero is reserved. */
#define UIMM3S          (SIMM11S32 + 1)
    { 3, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm3s, 0 },
    /* The unsigned 5-bit immediate used by short insns. */
#define UIMM5S          (UIMM3S + 1)
    { 5, 0, 0, ARC_OPERAND_UNSIGNED, 0, 0 },
    /* 12-bit signed immediate value, 16bit aligned. To be used by J,
       JL type instructions. */
#define SIMM12_16       (UIMM5S + 1)
    { 12, 6, 0, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED16, insert_simm12, extract_simm12 },
    /* The unsigned 6-bit immediate, 16bit aligned. To be used by J
       and JL instructions. */
#define UIMM6_16        (SIMM12_16 + 1)
    { 6, 6, 0, ARC_OPERAND_UNSIGNED | ARC_OPERAND_ALIGNED16, 0, 0 },
    /* 25-bit signed immediate value, 16bit aligned. To be used by B
       type instructions. */
#define SIMM25_16       (UIMM5S + 1)
    { 25, 0, 0, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED16,
      insert_simm25_16, extract_simm25_16 },
    /* 10-bit signed immediate value, 16 bit aligned. Used by
       B_S/BEQ_S/BNE_S type instructions */
#define SIMM10_16       (SIMM25_16 + 1)
    { 10, 0, 0, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED16
      | ARC_OPERAND_TRUNCATE, 0, 0 },
    /* 7-bit signed immediate value, 16 bit aligned. Used by any other
       B<cc>_S type instructions. */
#define SIMM7_16       (SIMM10_16 + 1)
    { 7, 0, 0, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED16
      | ARC_OPERAND_TRUNCATE, 0, 0 },
    /* 13-bit signed immediate value, 32 bit aligned. Used by BL_S type
       instructions. */
#define SIMM13_32       (SIMM7_16 + 1)
    { 13, 0, 0, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED32
      | ARC_OPERAND_TRUNCATE, 0, 0 },
    /* 25-bit signed immediate value, 32bit aligned. To be used by BL
       type instructions. */
#define SIMM25_32       (SIMM13_32 + 1)
    { 25, 0, 0, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED32,
      insert_simm25_32, extract_simm25_32 },
    /* 21-bit signed immediate value, 16bit aligned. To be used by B
       type instructions. */
#define SIMM21_16       (SIMM25_32 + 1)
    { 21, 0, 0, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED16,
      insert_simm21_16, extract_simm21_16 },
    /* 21-bit signed immediate value, 32bit aligned. To be used by BL
       type instructions. */
#define SIMM21_32       (SIMM21_16 + 1)
    { 21, 0, 0, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED32,
      insert_simm21_32, extract_simm21_32 },

    /* 8-bit signed immediate value, 16bit aligned. To be used by BR[NE/EQ]_S
       type instructions. */
#define SIMM8_16       (SIMM21_32 + 1)
    { 8, 0, 0, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED16, 0, 0 },
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
#define ARG_32BIT_ZALIMMRC      { ZA, LIMM, RC }
#define ARG_32BIT_ZARBLIMM      { ZA, RB, LIMM }
#define ARG_32BIT_ZALIMMLIMM    { ZA, LIMM, LIMMdup }
#define ARG_32BIT_RARBU6        { RA, RB, UIMM6 }
#define ARG_32BIT_RALIMMU6      { RA, LIMM, UIMM6 }
#define ARG_32BIT_ZARBU6        { ZA, RB, UIMM6 }
#define ARG_32BIT_ZALIMMU6      { ZA, LIMM, UIMM6 }
#define ARG_32BIT_RBRBS12       { RB, RBdup, SIMM12 }
#define ARG_32BIT_ZALIMMS12     { ZA, LIMM, SIMM12 }
#define ARG_32BIT_RBRBLIMM      { RB, RBdup, LIMM }
#define ARG_32BIT_RBRBU6        { RB, RBdup, UIMM6 }

/* The opcode table.

   The format of the opcode table is:

   NAME OPCODE MASK CPU { OPERANDS } { FLAGS }
*/
const struct arc_opcode arc_opcodes[] =
  {
#if 1
/*ADD */
/* add<.f>    a,b,c     	0010 0bbb 0000 0000 FBBB CCCC CCAA AAAA  */
{ "add", 0x20000000, 0xF8FF0000, BASE, ARG_32BIT_RARBRC,     FLAGS_F },

/* add<.f>    a,b,u6    	0010 0bbb 0100 0000 FBBB uuuu uuAA AAAA  */
{ "add", 0x20400000, 0xF8FF0000, BASE, ARG_32BIT_RARBU6,     FLAGS_F },

/* add<.f>    b,b,s12   	0010 0bbb 1000 0000 FBBB ssss ssSS SSSS  */
{ "add", 0x20800000, 0xF8FF0000, BASE, ARG_32BIT_RBRBS12,    FLAGS_F },

/* add<.f>    a,b,limm  	0010 0bbb 0000 0000 FBBB 1111 10AA AAAA  */
{ "add", 0x20000F80, 0xF8FF0FC0, BASE, ARG_32BIT_RARBLIMM,   FLAGS_F },

/* add<.f>    a,limm,c  	0010 0110 0000 0000 F111 CCCC CCAA AAAA  */
{ "add", 0x26007000, 0xFFFF7000, BASE, ARG_32BIT_RALIMMRC,   FLAGS_F },

/* add<.f>    a,limm,u6 	0010 0110 0100 0000 F111 uuuu uuAA AAAA  */
{ "add", 0x26407000, 0xFFFF7000, BASE, ARG_32BIT_RALIMMU6,   FLAGS_F },

/* add<.f>    a,limm,limm 	0010 0110 0000 0000 F111 1111 10AA AAAA  */
{ "add", 0x26007F80, 0xFFFF7FC0, BASE, ARG_32BIT_RALIMMLIMM, FLAGS_F },

/* add<.cc><.f>    b,b,c 	0010 0bbb 1100 0000 FBBB CCCC CC0Q QQQQ  */
{ "add", 0x20C00000, 0xF8FF0020, BASE, ARG_32BIT_RBRBRC,     FLAGS_CCF },

/* add<.cc><.f>    b,b,u6 	0010 0bbb 1100 0000 FBBB uuuu uu1Q QQQQ  */
{ "add", 0x20C00020, 0xF8FF0020, BASE, ARG_32BIT_RBRBU6,     FLAGS_CCF },

/* add<.cc><.f>    b,b,limm 	0010 0bbb 1100 0000 FBBB 1111 100Q QQQQ  */
{ "add", 0x20C00F80, 0xF8FF0FE0, BASE, ARG_32BIT_RBRBLIMM,   FLAGS_CCF },

/* add<.f>    0,b,c     	0010 0bbb 0000 0000 FBBB CCCC CC11 1110  */
{ "add", 0x2000003E, 0xF8FF003F, BASE, ARG_32BIT_ZARBRC,     FLAGS_F },

/* add<.f>    0,b,u6    	0010 0bbb 0100 0000 FBBB uuuu uu11 1110  */
{ "add", 0x2040003E, 0xF8FF003F, BASE, ARG_32BIT_ZARBU6,     FLAGS_F },

/* add<.f>    0,b,limm  	0010 0bbb 0000 0000 FBBB 1111 1011 1110  */
{ "add", 0x20000FBE, 0xF8FF0FFF, BASE, ARG_32BIT_ZARBLIMM,   FLAGS_F },

/* add<.f>    0,limm,c  	0010 0110 0000 0000 F111 CCCC CC11 1110  */
{ "add", 0x2600703E, 0xFFFF703F, BASE, ARG_32BIT_ZALIMMRC,   FLAGS_F },

/* add<.f>    0,limm,u6 	0010 0110 0100 0000 F111 uuuu uu11 1110  */
{ "add", 0x2640703E, 0xFFFF703F, BASE, ARG_32BIT_ZALIMMU6,   FLAGS_F },

/* add<.f>    0,limm,s12 	0010 0110 1000 0000 F111 ssss ssSS SSSS  */
{ "add", 0x26807000, 0xFFFF7000, BASE, ARG_32BIT_ZALIMMS12,  FLAGS_F },

/* add<.f>    0,limm,limm 	0010 0110 0000 0000 F111 1111 1011 1110  */
{ "add", 0x26007FBE, 0xFFFF7FFF, BASE, ARG_32BIT_ZALIMMLIMM, FLAGS_F },

/* add<.cc><.f>    0,limm,c 	0010 0110 1100 0000 F111 CCCC CC0Q QQQQ  */
{ "add", 0x26C07000, 0xFFFF7020, BASE, ARG_32BIT_ZALIMMRC,   FLAGS_CCF },

/* add<.cc><.f>    0,limm,u6 	0010 0110 1100 0000 F111 uuuu uu1Q QQQQ  */
{ "add", 0x26C07020, 0xFFFF7020, BASE, ARG_32BIT_ZALIMMU6,   FLAGS_F },

/* add<.cc><.f>    0,limm,limm 	0010 0110 1100 0000 F111 1111 100Q QQQQ  */
{ "add", 0x26C07F80, 0xFFFF7FE0, BASE, ARG_32BIT_ZALIMMLIMM, FLAGS_CCF },


{"add_s",       0x00006018, 0x0000F818, ARC_OPCODE_ARCv2EM | ARC_OPCODE_ARCv2HS | ARC_OPCODE_ARC700 | ARC_OPCODE_ARC600, { RA16, RB16, RC16 }, { 0 } },
{"add_s",       0x00007000, 0x0000F81C, ARC_OPCODE_ARCv2EM | ARC_OPCODE_ARCv2HS, { RB16, RB16dup, R5H }, { 0 } },
{"add_s",       0x00007004, 0x0000F81C, ARC_OPCODE_ARCv2EM | ARC_OPCODE_ARCv2HS, { R5H, R5Hdup, SIMM3S }, { 0 } },
{"add_s",       0x000070C7, 0x0000F8FF, ARC_OPCODE_ARCv2EM | ARC_OPCODE_ARCv2HS, { ZA, LIMM, SIMM3S }, { 0 } },
{"add_s",       0x0000C080, 0x0000F8E0, ARC_OPCODE_ARCv2EM | ARC_OPCODE_ARCv2HS | ARC_OPCODE_ARC700 | ARC_OPCODE_ARC600, { RB16, SP, UIMM7S32 }, { 0 } },
{"add_s",       0x0000E000, 0x0000F880, ARC_OPCODE_ARCv2EM | ARC_OPCODE_ARCv2HS | ARC_OPCODE_ARC700 | ARC_OPCODE_ARC600, { RB16, RB16dup, UIMM7 }, { 0 } },
{"add_s",       0x00006800, 0x0000F818, ARC_OPCODE_ARCv2EM | ARC_OPCODE_ARCv2HS | ARC_OPCODE_ARC700 | ARC_OPCODE_ARC600, { RC16, RB16, UIMM3S }, { 0 } },
{"add_s",       0x0000C0A0, 0x0000FFE0, ARC_OPCODE_ARCv2EM | ARC_OPCODE_ARCv2HS | ARC_OPCODE_ARC700 | ARC_OPCODE_ARC600, { SP, SP, UIMM7S32 }, { 0 } },
{"add_s",       0x0000CE00, 0x0000FE00, ARC_OPCODE_ARCv2EM | ARC_OPCODE_ARCv2HS | ARC_OPCODE_ARC700 | ARC_OPCODE_ARC600, { R0, GP, SIMM11S32 }, { 0 } },
{"add_s",       0x00004808, 0x0000F888, (ARC_OPCODE_ARCv2EM) | ARC_OPCODE_ARCv2HS, { R0, RB16, UIMM6S }, { 0 } },
{"add_s",       0x00004888, 0x0000F888, (ARC_OPCODE_ARCv2EM) | ARC_OPCODE_ARCv2HS, { R1, RB16, UIMM6S }, { 0 } },
{"add_s",       0x000070C3, 0x0000F8FF, ARC_OPCODE_ARCv2EM | ARC_OPCODE_ARCv2HS, { RB16, RB16dup, LIMM }, { 0 } },

    { "nop", 0x264A7000, 0xFFFFFFFF, BASE, ARG_NONE, FLAGS_NONE },
    { "nop_s", 0x78E0, 0xFFFF, BASE, ARG_NONE, FLAGS_NONE },
    { "bbit0", 0x8010006, 0xF8010017, BASE, { RB, RC, BBS9 }, { C_T, C_D } },
#else
#include "arc-opcodes.h"
#endif
  };

const unsigned arc_num_opcodes = sizeof(arc_opcodes)/sizeof(*arc_opcodes);

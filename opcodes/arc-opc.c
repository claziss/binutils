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
extract_rb (unsigned insn, int *invalid)
{
  return 0;
}


/* Abbreviations for instruction subsets.  */
#define BASE			ARC_OPCODE_BASE
#define EM			ARC_OPCODE_ARCv2

/* Common combinations of FLAGS.  */
#define FLAGS_NONE              { 0 }

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
  /* The plain integer register fields.  */
#define RA		(UNUSED + 1)
    { 6, 0, 0, ARC_OPERAND_IR, 0, 0 },
#define RB		(RA + 1)
    { 6, 12, 0, ARC_OPERAND_IR, insert_rb, extract_rb },
#define RC		(RB + 1)
    { 6, 6, 0, ARC_OPERAND_IR, 0, 0 },
  };
const unsigned arc_num_operands = sizeof(arc_operands)/sizeof(*arc_operands);

/* Common combinations of arguments.  */
#define ARG_NONE                { 0 }
#define ARG_32BIT_RARBRC        { RA, RB, RC }

/* The opcode table.

   The format of the opcode table is:

   NAME OPCODE MASK CPU { OPERANDS } { FLAGS }
*/
const struct arc_opcode arc_opcodes[] =
  {
    { "add", 0x20000000, 0xF8FF0000, BASE, ARG_32BIT_RARBRC, FLAGS_NONE },
    { "nop", 0x264A7000, 0xFFFFFFFF, BASE, ARG_NONE, FLAGS_NONE },
    { "nop_s", 0x78E0, 0xFFFF, BASE, ARG_NONE, FLAGS_NONE }
  };

const unsigned arc_num_opcodes = sizeof(arc_opcodes)/sizeof(*arc_opcodes);

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
#define ARC_OPCODE_BASE    0x0001  /* Base architecture -- all cpus.  */
#define ARC_OPCODE_ARCv1   0x0002  /* ARCv1 specific insns.  */
#define ARC_OPCODE_ARCv2   0x0004  /* ARCv2 specific insns.  */

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

/* Values defined for the flags field of a struct arc_operand.  */

/* This operand does not actually exist in the assembler input.  This
   is used to support extended mnemonics, for which two operands fields
   are identical.  The assembler should call the insert function with
   any op value.  The disassembler should call the extract function,
   ignore the return value, and check the value placed in the invalid
   argument.  */
#define ARC_OPERAND_FAKE	01

/* This operand names an integer register.  */
#define ARC_OPERAND_IR		02

/* This operand takes signed values.  */
#define ARC_OPERAND_SIGNED	04

/* This operand takes unsigned values.  This exists primarily so that
   a flags value of 0 can be treated as end-of-arguments.  */
#define ARC_OPERAND_UNSIGNED	010

/* Mask for selecting the type for typecheck purposes */
#define ARC_OPERAND_TYPECHECK_MASK					\
  (ARC_OPERAND_IR |		\
   ARC_OPERAND_SIGNED | 	\
   ARC_OPERAND_UNSIGNED)

/* Mask for optional argument default value.  */
#define ARC_OPERAND_OPTIONAL_MASK 07000

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

#endif /* OPCODE_ARC_H */

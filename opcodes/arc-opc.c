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
insert_pcl (unsigned insn,
	   int value,
	   const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value != 63)
    *errmsg = _("Register must be PCL.");
  return insn;
}

static int
extract_pcl (unsigned insn ATTRIBUTE_UNUSED,
	    int *invalid ATTRIBUTE_UNUSED)
{
  return 63;
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
insert_ilink1 (unsigned insn,
	      int value,
	      const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value != 29)
    *errmsg = _("Register must be ILINK1.");
  return insn;
}

static int
extract_ilink1 (unsigned insn ATTRIBUTE_UNUSED,
	       int *invalid ATTRIBUTE_UNUSED)
{
  return 29;
}

static unsigned
insert_ilink2 (unsigned insn,
	      int value,
	      const char **errmsg ATTRIBUTE_UNUSED)
{
  if (value != 30)
    *errmsg = _("Register must be ILINK2.");
  return insn;
}

static int
extract_ilink2 (unsigned insn ATTRIBUTE_UNUSED,
	       int *invalid ATTRIBUTE_UNUSED)
{
  return 30;
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

/************************************************************************/
/* New insert extract functions                                         */
/************************************************************************/
#ifndef INSERT_UIMM6_20
#define INSERT_UIMM6_20
/* mask = 00000000000000000000111111000000
   insn = 00100bbb01101111FBBBuuuuuu001001 */
static unsigned
insert_uimm6_20 (unsigned insn ATTRIBUTE_UNUSED,
		 int value ATTRIBUTE_UNUSED,
		 const char **errmsg ATTRIBUTE_UNUSED)
{
  insn |= ((value >> 0) & 0x003f) << 6;

  return insn;
}
#endif /* INSERT_UIMM6_20 */

#ifndef EXTRACT_UIMM6_20
#define EXTRACT_UIMM6_20
/* mask = 00000000000000000000111111000000 */
static int
extract_uimm6_20 (unsigned insn ATTRIBUTE_UNUSED,
		  int *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 6) & 0x003f) << 0;

  return value;
}
#endif /* EXTRACT_UIMM6_20 */

#ifndef INSERT_SIMM12_20
#define INSERT_SIMM12_20
/* mask = 00000000000000000000111111222222
   insn = 00110bbb10101000FBBBssssssSSSSSS */
static unsigned
insert_simm12_20 (unsigned insn ATTRIBUTE_UNUSED,
		  int value ATTRIBUTE_UNUSED,
		  const char **errmsg ATTRIBUTE_UNUSED)
{

  insn |= ((value >> 0) & 0x003f) << 6;
  insn |= ((value >> 6) & 0x003f) << 0;

  return insn;
}
#endif /* INSERT_SIMM12_20 */

#ifndef EXTRACT_SIMM12_20
#define EXTRACT_SIMM12_20
/* mask = 00000000000000000000111111222222 */
static  int
extract_simm12_20 (unsigned insn ATTRIBUTE_UNUSED,
		   int *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 6) & 0x003f) << 0;
  value |= ((insn >> 0) & 0x003f) << 6;

  /* Extend the sign */
  int signbit = 1 << (12 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM12_20 */

#ifndef INSERT_SIMM3_5_S
#define INSERT_SIMM3_5_S
/* mask = 0000011100000000
   insn = 01110ssshhh001HH */
static unsigned
insert_simm3_5_s (unsigned insn ATTRIBUTE_UNUSED,
		  int value ATTRIBUTE_UNUSED,
		  const char **errmsg ATTRIBUTE_UNUSED)
{
  insn |= ((value >> 0) & 0x0007) << 8;

  return insn;
}
#endif /* INSERT_SIMM3_5_S */

#ifndef EXTRACT_SIMM3_5_S
#define EXTRACT_SIMM3_5_S
/* mask = 0000011100000000 */
static  int
extract_simm3_5_s (unsigned insn ATTRIBUTE_UNUSED,
		   int *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 8) & 0x0007) << 0;

  /* Extend the sign */
  int signbit = 1 << (3 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM3_5_S */

#ifndef INSERT_UIMM7_A32_11_S
#define INSERT_UIMM7_A32_11_S
/* mask = 0000000000011111
   insn = 11000bbb100uuuuu */
static unsigned
insert_uimm7_a32_11_s (unsigned insn ATTRIBUTE_UNUSED,
		       int value ATTRIBUTE_UNUSED,
		       const char **errmsg ATTRIBUTE_UNUSED)
{
  if(value & 0x03)
    *errmsg = _("Target address is not 32bit aligned.");

  insn |= ((value >> 2) & 0x001f) << 0;

  return insn;
}
#endif /* INSERT_UIMM7_A32_11_S */

#ifndef EXTRACT_UIMM7_A32_11_S
#define EXTRACT_UIMM7_A32_11_S
/* mask = 0000000000011111 */
static int
extract_uimm7_a32_11_s (unsigned insn ATTRIBUTE_UNUSED,
			int *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x001f) << 2;

  return value;
}
#endif /* EXTRACT_UIMM7_A32_11_S */

#ifndef INSERT_UIMM7_9_S
#define INSERT_UIMM7_9_S
/* mask = 0000000001111111
   insn = 11100bbb0uuuuuuu */
static unsigned
insert_uimm7_9_s (unsigned insn ATTRIBUTE_UNUSED,
		  int value ATTRIBUTE_UNUSED,
		  const char **errmsg ATTRIBUTE_UNUSED)
{
  insn |= ((value >> 0) & 0x007f) << 0;

  return insn;
}
#endif /* INSERT_UIMM7_9_S */

#ifndef EXTRACT_UIMM7_9_S
#define EXTRACT_UIMM7_9_S
/* mask = 0000000001111111 */
static int
extract_uimm7_9_s (unsigned insn ATTRIBUTE_UNUSED,
		   int *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x007f) << 0;


  return value;
}
#endif /* EXTRACT_UIMM7_9_S */

#ifndef INSERT_UIMM3_13_S
#define INSERT_UIMM3_13_S
/* mask = 0000000000000111
   insn = 01101bbbccc00uuu */
static unsigned
insert_uimm3_13_s (unsigned insn ATTRIBUTE_UNUSED,
		   int value ATTRIBUTE_UNUSED,
		   const char **errmsg ATTRIBUTE_UNUSED)
{

  insn |= ((value >> 0) & 0x0007) << 0;

  return insn;
}
#endif /* INSERT_UIMM3_13_S */

#ifndef EXTRACT_UIMM3_13_S
#define EXTRACT_UIMM3_13_S
/* mask = 0000000000000111 */
static int
extract_uimm3_13_s (unsigned insn ATTRIBUTE_UNUSED,
		    int *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x0007) << 0;


  return value;
}
#endif /* EXTRACT_UIMM3_13_S */
#ifndef INSERT_SIMM11_A32_7_S
#define INSERT_SIMM11_A32_7_S
/* mask = 0000000111111111
   insn = 1100111sssssssss */
static unsigned
insert_simm11_a32_7_s (unsigned insn ATTRIBUTE_UNUSED,
		       int value ATTRIBUTE_UNUSED,
		       const char **errmsg ATTRIBUTE_UNUSED)
{
  if(value & 0x03)
    *errmsg = _("Target address is not 32bit aligned.");

  insn |= ((value >> 2) & 0x01ff) << 0;

  return insn;
}
#endif /* INSERT_SIMM11_A32_7_S */

#ifndef EXTRACT_SIMM11_A32_7_S
#define EXTRACT_SIMM11_A32_7_S
/* mask = 0000000111111111 */
static  int
extract_simm11_a32_7_s (unsigned insn ATTRIBUTE_UNUSED,
			int *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 0) & 0x01ff) << 2;

  /* Extend the sign */
  int signbit = 1 << (11 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM11_A32_7_S */

#ifndef INSERT_UIMM6_13_S
#define INSERT_UIMM6_13_S
/* mask = 0000000002220111
   insn = 01001bbb0UUU1uuu */
static unsigned
insert_uimm6_13_s (unsigned insn ATTRIBUTE_UNUSED,
		   int value ATTRIBUTE_UNUSED,
		   const char **errmsg ATTRIBUTE_UNUSED)
{

  insn |= ((value >> 0) & 0x0007) << 0;
  insn |= ((value >> 3) & 0x0007) << 4;

  return insn;
}
#endif /* INSERT_UIMM6_13_S */

#ifndef EXTRACT_UIMM6_13_S
#define EXTRACT_UIMM6_13_S
/* mask = 0000000002220111 */
static int
extract_uimm6_13_s (unsigned insn ATTRIBUTE_UNUSED,
		    int *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x0007) << 0;
  value |= ((insn >> 4) & 0x0007) << 3;


  return value;
}
#endif /* EXTRACT_UIMM6_13_S */

#ifndef INSERT_UIMM5_11_S
#define INSERT_UIMM5_11_S
/* mask = 0000000000011111
   insn = 10111bbb000uuuuu */
static unsigned
insert_uimm5_11_s (unsigned insn ATTRIBUTE_UNUSED,
		   int value ATTRIBUTE_UNUSED,
		   const char **errmsg ATTRIBUTE_UNUSED)
{

  insn |= ((value >> 0) & 0x001f) << 0;

  return insn;
}
#endif /* INSERT_UIMM5_11_S */

#ifndef EXTRACT_UIMM5_11_S
#define EXTRACT_UIMM5_11_S
/* mask = 0000000000011111 */
static int
extract_uimm5_11_s (unsigned insn ATTRIBUTE_UNUSED,
		    int *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x001f) << 0;


  return value;
}
#endif /* EXTRACT_UIMM5_11_S */

#ifndef INSERT_SIMM9_A16_8
#define INSERT_SIMM9_A16_8
/* mask = 00000000111111102000000000000000
   insn = 00001bbbsssssss1SBBBCCCCCCN01110 */
static unsigned
insert_simm9_a16_8 (unsigned insn ATTRIBUTE_UNUSED,
		    int value ATTRIBUTE_UNUSED,
		    const char **errmsg ATTRIBUTE_UNUSED)
{
  if(value & 0x01)
    *errmsg = _("Target address is not 16bit aligned.");

  insn |= ((value >> 1) & 0x007f) << 17;
  insn |= ((value >> 8) & 0x0001) << 15;

  return insn;
}
#endif /* INSERT_SIMM9_A16_8 */

#ifndef EXTRACT_SIMM9_A16_8
#define EXTRACT_SIMM9_A16_8
/* mask = 00000000111111102000000000000000 */
static  int
extract_simm9_a16_8 (unsigned insn ATTRIBUTE_UNUSED,
		     int *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 17) & 0x007f) << 1;
  value |= ((insn >> 15) & 0x0001) << 8;

  /* Extend the sign */
  int signbit = 1 << (9 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM9_A16_8 */

#ifndef INSERT_UIMM6_8
#define INSERT_UIMM6_8
/* mask = 00000000000000000000111111000000
   insn = 00001bbbsssssss1SBBBuuuuuuN11110 */
static unsigned
insert_uimm6_8 (unsigned insn ATTRIBUTE_UNUSED,
		int value ATTRIBUTE_UNUSED,
		const char **errmsg ATTRIBUTE_UNUSED)
{

  insn |= ((value >> 0) & 0x003f) << 6;

  return insn;
}
#endif /* INSERT_UIMM6_8 */

#ifndef EXTRACT_UIMM6_8
#define EXTRACT_UIMM6_8
/* mask = 00000000000000000000111111000000 */
static int
extract_uimm6_8 (unsigned insn ATTRIBUTE_UNUSED,
		 int *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 6) & 0x003f) << 0;


  return value;
}
#endif /* EXTRACT_UIMM6_8 */

#ifndef INSERT_SIMM21_A16_5
#define INSERT_SIMM21_A16_5
/* mask = 00000111111111102222222222000000
   insn = 00000ssssssssss0SSSSSSSSSSNQQQQQ */
static unsigned
insert_simm21_a16_5 (unsigned insn ATTRIBUTE_UNUSED,
		     int value ATTRIBUTE_UNUSED,
		     const char **errmsg ATTRIBUTE_UNUSED)
{
  if(value & 0x01)
    *errmsg = _("Target address is not 16bit aligned.");

  insn |= ((value >> 1) & 0x03ff) << 17;
  insn |= ((value >> 11) & 0x03ff) << 6;

  return insn;
}
#endif /* INSERT_SIMM21_A16_5 */

#ifndef EXTRACT_SIMM21_A16_5
#define EXTRACT_SIMM21_A16_5
/* mask = 00000111111111102222222222000000 */
static  int
extract_simm21_a16_5 (unsigned insn ATTRIBUTE_UNUSED,
		      int *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 17) & 0x03ff) << 1;
  value |= ((insn >> 6) & 0x03ff) << 11;

  /* Extend the sign */
  int signbit = 1 << (21 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM21_A16_5 */

#ifndef INSERT_SIMM25_A16_5
#define INSERT_SIMM25_A16_5
/* mask = 00000111111111102222222222003333
   insn = 00000ssssssssss1SSSSSSSSSSNRtttt */
static unsigned
insert_simm25_a16_5 (unsigned insn ATTRIBUTE_UNUSED,
		     int value ATTRIBUTE_UNUSED,
		     const char **errmsg ATTRIBUTE_UNUSED)
{
  if(value & 0x01)
    *errmsg = _("Target address is not 16bit aligned.");

  insn |= ((value >> 1) & 0x03ff) << 17;
  insn |= ((value >> 11) & 0x03ff) << 6;
  insn |= ((value >> 21) & 0x000f) << 0;

  return insn;
}
#endif /* INSERT_SIMM25_A16_5 */

#ifndef EXTRACT_SIMM25_A16_5
#define EXTRACT_SIMM25_A16_5
/* mask = 00000111111111102222222222003333 */
static  int
extract_simm25_a16_5 (unsigned insn ATTRIBUTE_UNUSED,
		      int *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 17) & 0x03ff) << 1;
  value |= ((insn >> 6) & 0x03ff) << 11;
  value |= ((insn >> 0) & 0x000f) << 21;

  /* Extend the sign */
  int signbit = 1 << (25 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM25_A16_5 */

#ifndef INSERT_SIMM10_A16_7_S
#define INSERT_SIMM10_A16_7_S
/* mask = 0000000111111111
   insn = 1111001sssssssss */
static unsigned
insert_simm10_a16_7_s (unsigned insn ATTRIBUTE_UNUSED,
		       int value ATTRIBUTE_UNUSED,
		       const char **errmsg ATTRIBUTE_UNUSED)
{
  if(value & 0x01)
    *errmsg = _("Target address is not 16bit aligned.");

  insn |= ((value >> 1) & 0x01ff) << 0;

  return insn;
}
#endif /* INSERT_SIMM10_A16_7_S */

#ifndef EXTRACT_SIMM10_A16_7_S
#define EXTRACT_SIMM10_A16_7_S
/* mask = 0000000111111111 */
static  int
extract_simm10_a16_7_s (unsigned insn ATTRIBUTE_UNUSED,
			int *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 0) & 0x01ff) << 1;

  /* Extend the sign */
  int signbit = 1 << (10 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM10_A16_7_S */

#ifndef INSERT_SIMM7_A16_10_S
#define INSERT_SIMM7_A16_10_S
/* mask = 0000000000111111
   insn = 1111011000ssssss */
static unsigned
insert_simm7_a16_10_s (unsigned insn ATTRIBUTE_UNUSED,
		       int value ATTRIBUTE_UNUSED,
		       const char **errmsg ATTRIBUTE_UNUSED)
{
  if(value & 0x01)
    *errmsg = _("Target address is not 16bit aligned.");

  insn |= ((value >> 1) & 0x003f) << 0;

  return insn;
}
#endif /* INSERT_SIMM7_A16_10_S */

#ifndef EXTRACT_SIMM7_A16_10_S
#define EXTRACT_SIMM7_A16_10_S
/* mask = 0000000000111111 */
static  int
extract_simm7_a16_10_s (unsigned insn ATTRIBUTE_UNUSED,
			int *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 0) & 0x003f) << 1;

  /* Extend the sign */
  int signbit = 1 << (7 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM7_A16_10_S */

#ifndef INSERT_SIMM21_A32_5
#define INSERT_SIMM21_A32_5
/* mask = 00000111111111002222222222000000
   insn = 00001sssssssss00SSSSSSSSSSNQQQQQ */
static unsigned
insert_simm21_a32_5 (unsigned insn ATTRIBUTE_UNUSED,
		     int value ATTRIBUTE_UNUSED,
		     const char **errmsg ATTRIBUTE_UNUSED)
{
  if(value & 0x03)
    *errmsg = _("Target address is not 32bit aligned.");

  insn |= ((value >> 2) & 0x01ff) << 18;
  insn |= ((value >> 11) & 0x03ff) << 6;

  return insn;
}
#endif /* INSERT_SIMM21_A32_5 */

#ifndef EXTRACT_SIMM21_A32_5
#define EXTRACT_SIMM21_A32_5
/* mask = 00000111111111002222222222000000 */
static  int
extract_simm21_a32_5 (unsigned insn ATTRIBUTE_UNUSED,
		      int *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 18) & 0x01ff) << 2;
  value |= ((insn >> 6) & 0x03ff) << 11;

  /* Extend the sign */
  int signbit = 1 << (21 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM21_A32_5 */

#ifndef INSERT_SIMM25_A32_5
#define INSERT_SIMM25_A32_5
/* mask = 00000111111111002222222222003333
   insn = 00001sssssssss10SSSSSSSSSSNRtttt */
static unsigned
insert_simm25_a32_5 (unsigned insn ATTRIBUTE_UNUSED,
		     int value ATTRIBUTE_UNUSED,
		     const char **errmsg ATTRIBUTE_UNUSED)
{
  if(value & 0x03)
    *errmsg = _("Target address is not 32bit aligned.");

  insn |= ((value >> 2) & 0x01ff) << 18;
  insn |= ((value >> 11) & 0x03ff) << 6;
  insn |= ((value >> 21) & 0x000f) << 0;

  return insn;
}
#endif /* INSERT_SIMM25_A32_5 */

#ifndef EXTRACT_SIMM25_A32_5
#define EXTRACT_SIMM25_A32_5
/* mask = 00000111111111002222222222003333 */
static  int
extract_simm25_a32_5 (unsigned insn ATTRIBUTE_UNUSED,
		      int *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 18) & 0x01ff) << 2;
  value |= ((insn >> 6) & 0x03ff) << 11;
  value |= ((insn >> 0) & 0x000f) << 21;

  /* Extend the sign */
  int signbit = 1 << (25 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM25_A32_5 */

#ifndef INSERT_SIMM13_A32_5_S
#define INSERT_SIMM13_A32_5_S
/* mask = 0000011111111111
   insn = 11111sssssssssss */
static unsigned
insert_simm13_a32_5_s (unsigned insn ATTRIBUTE_UNUSED,
		       int value ATTRIBUTE_UNUSED,
		       const char **errmsg ATTRIBUTE_UNUSED)
{
  if(value & 0x03)
    *errmsg = _("Target address is not 32bit aligned.");

  insn |= ((value >> 2) & 0x07ff) << 0;

  return insn;
}
#endif /* INSERT_SIMM13_A32_5_S */

#ifndef EXTRACT_SIMM13_A32_5_S
#define EXTRACT_SIMM13_A32_5_S
/* mask = 0000011111111111 */
static  int
extract_simm13_a32_5_s (unsigned insn ATTRIBUTE_UNUSED,
			int *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 0) & 0x07ff) << 2;

  /* Extend the sign */
  int signbit = 1 << (13 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM13_A32_5_S */

#ifndef INSERT_SIMM8_A16_9_S
#define INSERT_SIMM8_A16_9_S
/* mask = 0000000001111111
   insn = 11101bbb1sssssss */
static unsigned
insert_simm8_a16_9_s (unsigned insn ATTRIBUTE_UNUSED,
		      int value ATTRIBUTE_UNUSED,
		      const char **errmsg ATTRIBUTE_UNUSED)
{
  if(value & 0x01)
    *errmsg = _("Target address is not 16bit aligned.");

  insn |= ((value >> 1) & 0x007f) << 0;

  return insn;
}
#endif /* INSERT_SIMM8_A16_9_S */

#ifndef EXTRACT_SIMM8_A16_9_S
#define EXTRACT_SIMM8_A16_9_S
/* mask = 0000000001111111 */
static  int
extract_simm8_a16_9_s (unsigned insn ATTRIBUTE_UNUSED,
		       int *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 0) & 0x007f) << 1;

  /* Extend the sign */
  int signbit = 1 << (8 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM8_A16_9_S */

#ifndef INSERT_UIMM3_23
#define INSERT_UIMM3_23
/* mask = 00000000000000000000000111000000
   insn = 00100011011011110001RRRuuu111111 */
static unsigned
insert_uimm3_23 (unsigned insn ATTRIBUTE_UNUSED,
		 int value ATTRIBUTE_UNUSED,
		 const char **errmsg ATTRIBUTE_UNUSED)
{
  insn |= ((value >> 0) & 0x0007) << 6;

  return insn;
}
#endif /* INSERT_UIMM3_23 */

#ifndef EXTRACT_UIMM3_23
#define EXTRACT_UIMM3_23
/* mask = 00000000000000000000000111000000 */
static int
extract_uimm3_23 (unsigned insn ATTRIBUTE_UNUSED,
		  int *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 6) & 0x0007) << 0;

  return value;
}
#endif /* EXTRACT_UIMM3_23 */

#ifndef INSERT_UIMM10_6_S
#define INSERT_UIMM10_6_S
/* mask = 0000001111111111
   insn = 010111uuuuuuuuuu */
static unsigned
insert_uimm10_6_s (unsigned insn ATTRIBUTE_UNUSED,
		   int value ATTRIBUTE_UNUSED,
		   const char **errmsg ATTRIBUTE_UNUSED)
{
  insn |= ((value >> 0) & 0x03ff) << 0;

  return insn;
}
#endif /* INSERT_UIMM10_6_S */

#ifndef EXTRACT_UIMM10_6_S
#define EXTRACT_UIMM10_6_S
/* mask = 0000001111111111 */
static int
extract_uimm10_6_s (unsigned insn ATTRIBUTE_UNUSED,
		    int *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x03ff) << 0;

  return value;
}
#endif /* EXTRACT_UIMM10_6_S */

#ifndef INSERT_UIMM6_11_S
#define INSERT_UIMM6_11_S
/* mask = 0000002200011110
   insn = 110000UU111uuuu0 */
static unsigned
insert_uimm6_11_s (unsigned insn ATTRIBUTE_UNUSED,
		   int value ATTRIBUTE_UNUSED,
		   const char **errmsg ATTRIBUTE_UNUSED)
{
  insn |= ((value >> 0) & 0x000f) << 1;
  insn |= ((value >> 4) & 0x0003) << 8;

  return insn;
}
#endif /* INSERT_UIMM6_11_S */

#ifndef EXTRACT_UIMM6_11_S
#define EXTRACT_UIMM6_11_S
/* mask = 0000002200011110 */
static int
extract_uimm6_11_s (unsigned insn ATTRIBUTE_UNUSED,
		    int *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 1) & 0x000f) << 0;
  value |= ((insn >> 8) & 0x0003) << 4;

  return value;
}
#endif /* EXTRACT_UIMM6_11_S */

#ifndef INSERT_SIMM9_8
#define INSERT_SIMM9_8
/* mask = 00000000111111112000000000000000
   insn = 00010bbbssssssssSBBBDaaZZXAAAAAA */
static unsigned
insert_simm9_8 (unsigned insn ATTRIBUTE_UNUSED,
		int value ATTRIBUTE_UNUSED,
		const char **errmsg ATTRIBUTE_UNUSED)
{
  insn |= ((value >> 0) & 0x00ff) << 16;
  insn |= ((value >> 8) & 0x0001) << 15;

  return insn;
}
#endif /* INSERT_SIMM9_8 */

#ifndef EXTRACT_SIMM9_8
#define EXTRACT_SIMM9_8
/* mask = 00000000111111112000000000000000 */
static  int
extract_simm9_8 (unsigned insn ATTRIBUTE_UNUSED,
		 int *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 16) & 0x00ff) << 0;
  value |= ((insn >> 15) & 0x0001) << 8;

  /* Extend the sign */
  int signbit = 1 << (9 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM9_8 */

#ifndef INSERT_UIMM10_A32_8_S
#define INSERT_UIMM10_A32_8_S
/* mask = 0000000011111111
   insn = 11010bbbuuuuuuuu */
static unsigned
insert_uimm10_a32_8_s (unsigned insn ATTRIBUTE_UNUSED,
		       int value ATTRIBUTE_UNUSED,
		       const char **errmsg ATTRIBUTE_UNUSED)
{
  if(value & 0x03)
    *errmsg = _("Target address is not 32bit aligned.");

  insn |= ((value >> 2) & 0x00ff) << 0;

  return insn;
}
#endif /* INSERT_UIMM10_A32_8_S */

#ifndef EXTRACT_UIMM10_A32_8_S
#define EXTRACT_UIMM10_A32_8_S
/* mask = 0000000011111111 */
static int
extract_uimm10_a32_8_s (unsigned insn ATTRIBUTE_UNUSED,
			int *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x00ff) << 2;

  return value;
}
#endif /* EXTRACT_UIMM10_A32_8_S */

#ifndef INSERT_SIMM9_7_S
#define INSERT_SIMM9_7_S
/* mask = 0000000111111111
   insn = 1100101sssssssss */
static unsigned
insert_simm9_7_s (unsigned insn ATTRIBUTE_UNUSED,
		  int value ATTRIBUTE_UNUSED,
		  const char **errmsg ATTRIBUTE_UNUSED)
{
  insn |= ((value >> 0) & 0x01ff) << 0;

  return insn;
}
#endif /* INSERT_SIMM9_7_S */

#ifndef EXTRACT_SIMM9_7_S
#define EXTRACT_SIMM9_7_S
/* mask = 0000000111111111 */
static  int
extract_simm9_7_s (unsigned insn ATTRIBUTE_UNUSED,
		   int *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 0) & 0x01ff) << 0;

  /* Extend the sign */
  int signbit = 1 << (9 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM9_7_S */

#ifndef INSERT_UIMM6_A16_11_S
#define INSERT_UIMM6_A16_11_S
/* mask = 0000000000011111
   insn = 10010bbbcccuuuuu */
static unsigned
insert_uimm6_a16_11_s (unsigned insn ATTRIBUTE_UNUSED,
		       int value ATTRIBUTE_UNUSED,
		       const char **errmsg ATTRIBUTE_UNUSED)
{
  if(value & 0x01)
    *errmsg = _("Target address is not 16bit aligned.");

  insn |= ((value >> 1) & 0x001f) << 0;

  return insn;
}
#endif /* INSERT_UIMM6_A16_11_S */

#ifndef EXTRACT_UIMM6_A16_11_S
#define EXTRACT_UIMM6_A16_11_S
/* mask = 0000000000011111 */
static int
extract_uimm6_a16_11_s (unsigned insn ATTRIBUTE_UNUSED,
			int *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x001f) << 1;


  return value;
}
#endif /* EXTRACT_UIMM6_A16_11_S */

#ifndef INSERT_UIMM5_A32_11_S
#define INSERT_UIMM5_A32_11_S
/* mask = 0000020000011000
   insn = 01000U00hhhuu1HH */
static unsigned
insert_uimm5_a32_11_s (unsigned insn ATTRIBUTE_UNUSED,
		       int value ATTRIBUTE_UNUSED,
		       const char **errmsg ATTRIBUTE_UNUSED)
{
  if(value & 0x03)
    *errmsg = _("Target address is not 32bit aligned.");

  insn |= ((value >> 2) & 0x0003) << 3;
  insn |= ((value >> 4) & 0x0001) << 10;

  return insn;
}
#endif /* INSERT_UIMM5_A32_11_S */

#ifndef EXTRACT_UIMM5_A32_11_S
#define EXTRACT_UIMM5_A32_11_S
/* mask = 0000020000011000 */
static int
extract_uimm5_a32_11_s (unsigned insn ATTRIBUTE_UNUSED,
			int *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 3) & 0x0003) << 2;
  value |= ((insn >> 10) & 0x0001) << 4;


  return value;
}
#endif /* EXTRACT_UIMM5_A32_11_S */

#ifndef INSERT_SIMM11_A32_13_S
#define INSERT_SIMM11_A32_13_S
/* mask = 0000022222200111
   insn = 01010SSSSSS00sss */
static unsigned
insert_simm11_a32_13_s (unsigned insn ATTRIBUTE_UNUSED,
			int value ATTRIBUTE_UNUSED,
			const char **errmsg ATTRIBUTE_UNUSED)
{
  if(value & 0x03)
    *errmsg = _("Target address is not 32bit aligned.");

  insn |= ((value >> 2) & 0x0007) << 0;
  insn |= ((value >> 5) & 0x003f) << 5;

  return insn;
}
#endif /* INSERT_SIMM11_A32_13_S */

#ifndef EXTRACT_SIMM11_A32_13_S
#define EXTRACT_SIMM11_A32_13_S
/* mask = 0000022222200111 */
static  int
extract_simm11_a32_13_s (unsigned insn ATTRIBUTE_UNUSED,
			 int *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 0) & 0x0007) << 2;
  value |= ((insn >> 5) & 0x003f) << 5;

  /* Extend the sign */
  int signbit = 1 << (11 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM11_A32_13_S */

#ifndef INSERT_UIMM7_13_S
#define INSERT_UIMM7_13_S
/* mask = 0000000022220111
   insn = 01010bbbUUUU1uuu */
static unsigned
insert_uimm7_13_s (unsigned insn ATTRIBUTE_UNUSED,
		   int value ATTRIBUTE_UNUSED,
		   const char **errmsg ATTRIBUTE_UNUSED)
{

  insn |= ((value >> 0) & 0x0007) << 0;
  insn |= ((value >> 3) & 0x000f) << 4;

  return insn;
}
#endif /* INSERT_UIMM7_13_S */

#ifndef EXTRACT_UIMM7_13_S
#define EXTRACT_UIMM7_13_S
/* mask = 0000000022220111 */
static int
extract_uimm7_13_s (unsigned insn ATTRIBUTE_UNUSED,
		    int *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x0007) << 0;
  value |= ((insn >> 4) & 0x000f) << 3;


  return value;
}
#endif /* EXTRACT_UIMM7_13_S */

#ifndef INSERT_UIMM6_A16_21
#define INSERT_UIMM6_A16_21
/* mask = 00000000000000000000011111000000
   insn = 00101bbb01001100RBBBRuuuuuAAAAAA */
static unsigned
insert_uimm6_a16_21 (unsigned insn ATTRIBUTE_UNUSED,
		     int value ATTRIBUTE_UNUSED,
		     const char **errmsg ATTRIBUTE_UNUSED)
{
  if(value & 0x01)
    *errmsg = _("Target address is not 16bit aligned.");

  insn |= ((value >> 1) & 0x001f) << 6;

  return insn;
}
#endif /* INSERT_UIMM6_A16_21 */

#ifndef EXTRACT_UIMM6_A16_21
#define EXTRACT_UIMM6_A16_21
/* mask = 00000000000000000000011111000000 */
static int
extract_uimm6_a16_21 (unsigned insn ATTRIBUTE_UNUSED,
		      int *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 6) & 0x001f) << 1;


  return value;
}
#endif /* EXTRACT_UIMM6_A16_21 */

#ifndef INSERT_UIMM7_11_S
#define INSERT_UIMM7_11_S
/* mask = 0000022200011110
   insn = 11000UUU110uuuu0 */
static unsigned
insert_uimm7_11_s (unsigned insn ATTRIBUTE_UNUSED,
		   int value ATTRIBUTE_UNUSED,
		   const char **errmsg ATTRIBUTE_UNUSED)
{

  insn |= ((value >> 0) & 0x000f) << 1;
  insn |= ((value >> 4) & 0x0007) << 8;

  return insn;
}
#endif /* INSERT_UIMM7_11_S */

#ifndef EXTRACT_UIMM7_11_S
#define EXTRACT_UIMM7_11_S
/* mask = 0000022200011110 */
static int
extract_uimm7_11_s (unsigned insn ATTRIBUTE_UNUSED,
		    int *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 1) & 0x000f) << 0;
  value |= ((insn >> 8) & 0x0007) << 4;


  return value;
}
#endif /* EXTRACT_UIMM7_11_S */

#ifndef INSERT_UIMM7_A16_20
#define INSERT_UIMM7_A16_20
/* mask = 00000000000000000000111111000000
   insn = 00100RRR111010000RRRuuuuuu1QQQQQ */
static unsigned
insert_uimm7_a16_20 (unsigned insn ATTRIBUTE_UNUSED,
		     int value ATTRIBUTE_UNUSED,
		     const char **errmsg ATTRIBUTE_UNUSED)
{
  if(value & 0x01)
    *errmsg = _("Target address is not 16bit aligned.");

  insn |= ((value >> 1) & 0x003f) << 6;

  return insn;
}
#endif /* INSERT_UIMM7_A16_20 */

#ifndef EXTRACT_UIMM7_A16_20
#define EXTRACT_UIMM7_A16_20
/* mask = 00000000000000000000111111000000 */
static int
extract_uimm7_a16_20 (unsigned insn ATTRIBUTE_UNUSED,
		      int *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 6) & 0x003f) << 1;


  return value;
}
#endif /* EXTRACT_UIMM7_A16_20 */

#ifndef INSERT_SIMM13_A16_20
#define INSERT_SIMM13_A16_20
/* mask = 00000000000000000000111111222222
   insn = 00100RRR101010000RRRssssssSSSSSS */
static unsigned
insert_simm13_a16_20 (unsigned insn ATTRIBUTE_UNUSED,
		      int value ATTRIBUTE_UNUSED,
		      const char **errmsg ATTRIBUTE_UNUSED)
{
  if(value & 0x01)
    *errmsg = _("Target address is not 16bit aligned.");

  insn |= ((value >> 1) & 0x003f) << 6;
  insn |= ((value >> 7) & 0x003f) << 0;

  return insn;
}
#endif /* INSERT_SIMM13_A16_20 */

#ifndef EXTRACT_SIMM13_A16_20
#define EXTRACT_SIMM13_A16_20
/* mask = 00000000000000000000111111222222 */
static  int
extract_simm13_a16_20 (unsigned insn ATTRIBUTE_UNUSED,
		       int *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 6) & 0x003f) << 1;
  value |= ((insn >> 0) & 0x003f) << 7;

  /* Extend the sign */
  int signbit = 1 << (13 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM13_A16_20 */

#ifndef INSERT_UIMM8_8_S
#define INSERT_UIMM8_8_S
/* mask = 0000000011111111
   insn = 11011bbbuuuuuuuu */
static unsigned
insert_uimm8_8_s (unsigned insn ATTRIBUTE_UNUSED,
		  int value ATTRIBUTE_UNUSED,
		  const char **errmsg ATTRIBUTE_UNUSED)
{

  insn |= ((value >> 0) & 0x00ff) << 0;

  return insn;
}
#endif /* INSERT_UIMM8_8_S */

#ifndef EXTRACT_UIMM8_8_S
#define EXTRACT_UIMM8_8_S
/* mask = 0000000011111111 */
static int
extract_uimm8_8_s (unsigned insn ATTRIBUTE_UNUSED,
		   int *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x00ff) << 0;


  return value;
}
#endif /* EXTRACT_UIMM8_8_S */

#ifndef INSERT_UIMM6_5_S
#define INSERT_UIMM6_5_S
/* mask = 0000011111100000
   insn = 01111uuuuuu11111 */
static unsigned
insert_uimm6_5_s (unsigned insn ATTRIBUTE_UNUSED,
		  int value ATTRIBUTE_UNUSED,
		  const char **errmsg ATTRIBUTE_UNUSED)
{

  insn |= ((value >> 0) & 0x003f) << 5;

  return insn;
}
#endif /* INSERT_UIMM6_5_S */

#ifndef EXTRACT_UIMM6_5_S
#define EXTRACT_UIMM6_5_S
/* mask = 0000011111100000 */
static int
extract_uimm6_5_s (unsigned insn ATTRIBUTE_UNUSED,
		   int *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 5) & 0x003f) << 0;


  return value;
}
#endif /* EXTRACT_UIMM6_5_S */

#ifndef INSERT_W6
#define INSERT_W6
/* mask = 00000000000000000000111111000000 
   insn = 00011bbb000000000BBBwwwwwwDaaZZ1 */
static unsigned
insert_w6 (unsigned insn ATTRIBUTE_UNUSED,
  	int value ATTRIBUTE_UNUSED,
  	const char **errmsg ATTRIBUTE_UNUSED)
{
  insn |= ((value >> 0) & 0x003f) << 6;

  return insn;
}
#endif /* INSERT_W6 */

#ifndef EXTRACT_W6
#define EXTRACT_W6
/* mask = 00000000000000000000111111000000 */
static int
extract_w6 (unsigned insn ATTRIBUTE_UNUSED,
  	int *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 6) & 0x003f) << 0;

  return value;
}
#endif /* EXTRACT_W6 */

#ifndef INSERT_G_S
#define INSERT_G_S
/* mask = 0000011100022000 
   insn = 01000ggghhhGG0HH */
static unsigned
insert_g_s (unsigned insn ATTRIBUTE_UNUSED,
  	int value ATTRIBUTE_UNUSED,
  	const char **errmsg ATTRIBUTE_UNUSED)
{
  insn |= ((value >> 0) & 0x0007) << 8;
  insn |= ((value >> 3) & 0x0003) << 3;

  return insn;
}
#endif /* INSERT_G_S */

#ifndef EXTRACT_G_S
#define EXTRACT_G_S
/* mask = 0000011100022000 */
static int
extract_g_s (unsigned insn ATTRIBUTE_UNUSED,
  	int *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 8) & 0x0007) << 0;
  value |= ((insn >> 3) & 0x0003) << 3;

  /* Extend the sign */
  int signbit = 1 << (6 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_G_S */


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
#define F_ASFAKE   (F_AS22 + 1)
    { "as", 0, 0, 0, 1 },
    /*  { "as", 3, ADDRESS22S_AC, 0 },*/

    /* Cache bypass. */
#define F_DI5     (F_ASFAKE + 1)
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
    { "h", 2, 2, 17, 1 },

    /* Fake Flags */
#define F_NE   (F_H17 + 1)
    { "ne", 0, 0, 0, 1 },

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
#define C_AA27      (C_CC + 1)
    { 0, { F_A3, F_AW3, F_AB3, F_AS3, F_NULL } },
#define C_AA_ADDR9  (C_AA_ADDR3 + 1)
#define C_AA21       (C_AA_ADDR3 + 1)
    { 0, { F_A9, F_AW9, F_AB9, F_AS9, F_NULL } },
#define C_AA_ADDR22 (C_AA_ADDR9 + 1)
#define C_AA8      (C_AA_ADDR9 + 1)
    { 0, { F_A22, F_AW22, F_AB22, F_AS22, F_NULL } },

#define C_F         (C_AA_ADDR22 + 1)
    { 0, { F_FLAG, F_NULL } },

#define C_T         (C_F + 1)
    { 0, { F_NT, F_T, F_NULL } },
#define C_D         (C_T + 1)
    { 0, { F_ND, F_D, F_NULL } },

#define C_DHARD     (C_D + 1)
    { 0, { F_D, F_NULL } },

#define C_DI20      (C_DHARD + 1)
    { 0, { F_DI11 }},
#define C_DI16      (C_DI20 + 1)
    { 0, { F_DI15 }},
#define C_DI26      (C_DI16 + 1)
    { 0, { F_DI5 }},

#define C_X25       (C_DI26 + 1)
    { 0, { F_SIGN6 }},
#define C_X15      (C_X25 + 1)
    { 0, { F_SIGN16 }},
#define C_X        (C_X15 + 1)
    { 0, { F_SIGN6 }}, /*FIXME! needs to be fake */

#define C_ZZ13        (C_X + 1)
    { 0, { F_SIZEB17, F_SIZEW17, F_H17}},
#define C_ZZ23        (C_ZZ13 + 1)
    { 0, { F_SIZEB7, F_SIZEW7, F_H7}},
#define C_ZZ29        (C_ZZ23 + 1)
    { 0, { F_SIZEB1, F_SIZEW1, F_H1}},

#define C_AS        (C_ZZ29 + 1)
    { 0, { F_ASFAKE}},

#define C_NE        (C_AS + 1)
    { 0, { F_NE}},
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
#define RA_S		(RBdup + 1)
    { 4, 0, 0, ARC_OPERAND_IR, insert_ras, extract_ras },
#define RB16		(RA16 + 1)
#define RB_S		(RA16 + 1)
    { 4, 8, 0, ARC_OPERAND_IR, insert_rbs, extract_rbs },
#define RB16dup		(RB16 + 1)
#define RB_Sdup		(RB16 + 1)
    { 4, 8, 0, ARC_OPERAND_IR | ARC_OPERAND_DUPLICATE, insert_rbs, extract_rbs },
#define RC16		(RB16dup + 1)
#define RC_S		(RB16dup + 1)
    { 4, 5, 0, ARC_OPERAND_IR, insert_rcs, extract_rcs },
#define R6H             (RC16 + 1)   /* 6bit register field 'h' used by V1 cpus*/
    { 6, 5, 0, ARC_OPERAND_IR, insert_rhv1, extract_rhv1 },
#define R5H             (R6H + 1)    /* 5bit register field 'h' used by V2 cpus*/
#define RH_S            (R6H + 1)    /* 5bit register field 'h' used by V2 cpus*/
    { 5, 5, 0, ARC_OPERAND_IR, insert_rhv2, extract_rhv2 },
#define R5Hdup          (R5H + 1)
#define RH_Sdup         (R5H + 1)
    { 5, 5, 0, ARC_OPERAND_IR | ARC_OPERAND_DUPLICATE, insert_rhv2, extract_rhv2 },

#define RG              (R5Hdup + 1)
#define G_S             (R5Hdup + 1)
    { 5, 5, 0, ARC_OPERAND_IR, insert_g_s, extract_g_s },

    /* Fix registers. */
#define R0              (RG + 1)
#define R0_S            (RG + 1)
    { 0, 0, 0, ARC_OPERAND_IR, insert_r0, extract_r0 },
#define R1              (R0 + 1)
#define R1_S            (R0 + 1)
    { 1, 0, 0, ARC_OPERAND_IR, insert_r1, extract_r1 },
#define R2              (R1 + 1)
#define R2_S            (R1 + 1)
    { 2, 0, 0, ARC_OPERAND_IR, insert_r2, extract_r2 },
#define R3              (R2 + 1)
#define R3_S            (R2 + 1)
    { 2, 0, 0, ARC_OPERAND_IR, insert_r3, extract_r3 },
#define SP              (R3 + 1)
#define SP_S            (R3 + 1)
    { 5, 0, 0, ARC_OPERAND_IR, insert_sp, extract_sp },
#define SPdup           (SP + 1)
#define SP_Sdup         (SP + 1)
    { 5, 0, 0, ARC_OPERAND_IR | ARC_OPERAND_DUPLICATE, insert_sp, extract_sp },
#define GP              (SPdup + 1)
#define GP_S            (SPdup + 1)
    { 5, 0, 0, ARC_OPERAND_IR, insert_gp, extract_gp },

#define PCL_S           (GP + 1)
    { 5, 0, 0, ARC_OPERAND_IR, insert_pcl, extract_pcl },

#define BLINK           (PCL_S + 1)
#define BLINK_S         (PCL_S + 1)
    { 5, 0, 0, ARC_OPERAND_IR, insert_blink, extract_blink },

#define ILINK1          (BLINK + 1)
    { 5, 0, 0, ARC_OPERAND_IR, insert_ilink1, extract_ilink1 },
#define ILINK2          (ILINK1 + 1)
    { 5, 0, 0, ARC_OPERAND_IR, insert_ilink2, extract_ilink2 },


    /* Long immediate. */
#define LIMM 		(ILINK2 + 1)
#define LIMM_S		(ILINK2 + 1)
    { 32, 0, BFD_RELOC_ARC_32_ME, ARC_OPERAND_LIMM, insert_limm, 0 },
#define LIMMdup		(LIMM + 1)
    { 32, 0, 0, ARC_OPERAND_LIMM | ARC_OPERAND_DUPLICATE, insert_limm, 0 },

    /* Special operands. */
#define ZA              (LIMMdup + 1)
#define ZB              (LIMMdup + 1)
#define ZA_S            (LIMMdup + 1)
#define ZB_S            (LIMMdup + 1)
#define ZC_S            (LIMMdup + 1)
    { 0, 0, 0, ARC_OPERAND_UNSIGNED, insert_za, 0 },

    /* Fake operand to handle the T flag. */
#define FKT             (ZA + 1)
    { 1, 3, 0, ARC_OPERAND_FAKE, insert_Ybit, 0 },

#if 0
    /* The signed "9-bit" immediate used for bbit instructions. */
#define BBS9            (ZA + 1)
    { 8, 17, -BBS9, ARC_OPERAND_SIGNED | ARC_OPERAND_PCREL | ARC_OPERAND_ALIGNED16,
      insert_bbs9, extract_bbs9 },

    /* The unsigned 6-bit immediate used in most arithmetic instructions. */
#define UIMM6           (BBS9 + 1)
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
#define SIMM25_16       (UIMM6_16 + 1)
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

#else /**************** New operands *****************/

    /* UIMM6_20 mask = 00000000000000000000111111000000 */
#define UIMM6_20       (FKT + 1)
    {6, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm6_20, extract_uimm6_20},

    /* SIMM12_20 mask = 00000000000000000000111111222222 */
#define SIMM12_20       (UIMM6_20 + 1)
    {12, 0, 0, ARC_OPERAND_SIGNED, insert_simm12_20, extract_simm12_20},

    /* SIMM3_5_S mask = 0000011100000000 */
#define SIMM3_5_S       (SIMM12_20 + 1)
    {3, 0, 0, ARC_OPERAND_SIGNED | ARC_OPERAND_NCHK, insert_simm3s, extract_simm3s},

    /* UIMM7_A32_11_S mask = 0000000000011111 */
#define UIMM7_A32_11_S       (SIMM3_5_S + 1)
    {7, 0, 0, ARC_OPERAND_UNSIGNED | ARC_OPERAND_ALIGNED32 | ARC_OPERAND_TRUNCATE, insert_uimm7_a32_11_s, extract_uimm7_a32_11_s},

    /* UIMM7_9_S mask = 0000000001111111 */
#define UIMM7_9_S       (UIMM7_A32_11_S + 1)
    {7, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm7_9_s, extract_uimm7_9_s},

    /* UIMM3_13_S mask = 0000000000000111 */
#define UIMM3_13_S       (UIMM7_9_S + 1)
    {3, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm3_13_s, extract_uimm3_13_s},

    /* SIMM11_A32_7_S mask = 0000000111111111 */
#define SIMM11_A32_7_S       (UIMM3_13_S + 1)
    {11, 0, 0, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED32 | ARC_OPERAND_TRUNCATE, insert_simm11_a32_7_s, extract_simm11_a32_7_s},

    /* UIMM6_13_S mask = 0000000002220111 */
#define UIMM6_13_S       (SIMM11_A32_7_S + 1)
    {6, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm6_13_s, extract_uimm6_13_s},
    /* UIMM5_11_S mask = 0000000000011111 */
#define UIMM5_11_S       (UIMM6_13_S + 1)
    {5, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm5_11_s, extract_uimm5_11_s},

    /* SIMM9_A16_8 mask = 00000000111111102000000000000000 */
#define SIMM9_A16_8       (UIMM5_11_S + 1)
    {9, 0, -SIMM9_A16_8, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED16 | ARC_OPERAND_PCREL | ARC_OPERAND_TRUNCATE, insert_simm9_a16_8, extract_simm9_a16_8},

    /* UIMM6_8 mask = 00000000000000000000111111000000 */
#define UIMM6_8       (SIMM9_A16_8 + 1)
    {6, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm6_8, extract_uimm6_8},

    /* SIMM21_A16_5 mask = 00000111111111102222222222000000 */
#define SIMM21_A16_5       (UIMM6_8 + 1)
    {21, 0, 0, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED16 | ARC_OPERAND_TRUNCATE, insert_simm21_a16_5, extract_simm21_a16_5},

    /* SIMM25_A16_5 mask = 00000111111111102222222222003333 */
#define SIMM25_A16_5       (SIMM21_A16_5 + 1)
    {25, 0, 0, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED16 | ARC_OPERAND_TRUNCATE, insert_simm25_a16_5, extract_simm25_a16_5},

    /* SIMM10_A16_7_S mask = 0000000111111111 */
#define SIMM10_A16_7_S       (SIMM25_A16_5 + 1)
    {10, 0, 0, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED16 | ARC_OPERAND_TRUNCATE, insert_simm10_a16_7_s, extract_simm10_a16_7_s},

    /* SIMM7_A16_10_S mask = 0000000000111111 */
#define SIMM7_A16_10_S       (SIMM10_A16_7_S + 1)
    {7, 0, 0, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED16 | ARC_OPERAND_TRUNCATE, insert_simm7_a16_10_s, extract_simm7_a16_10_s},

    /* SIMM21_A32_5 mask = 00000111111111002222222222000000 */
#define SIMM21_A32_5       (SIMM7_A16_10_S + 1)
    {21, 0, 0, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED32 | ARC_OPERAND_TRUNCATE, insert_simm21_a32_5, extract_simm21_a32_5},

    /* SIMM25_A32_5 mask = 00000111111111002222222222003333 */
#define SIMM25_A32_5       (SIMM21_A32_5 + 1)
    {25, 0, 0, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED32 | ARC_OPERAND_TRUNCATE, insert_simm25_a32_5, extract_simm25_a32_5},

    /* SIMM13_A32_5_S mask = 0000011111111111 */
#define SIMM13_A32_5_S       (SIMM25_A32_5 + 1)
    {13, 0, 0, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED32 | ARC_OPERAND_TRUNCATE, insert_simm13_a32_5_s, extract_simm13_a32_5_s},

    /* SIMM8_A16_9_S mask = 0000000001111111 */
#define SIMM8_A16_9_S       (SIMM13_A32_5_S + 1)
    {8, 0, 0, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED16 | ARC_OPERAND_TRUNCATE, insert_simm8_a16_9_s, extract_simm8_a16_9_s},

    /* UIMM3_23 mask = 00000000000000000000000111000000 */
#define UIMM3_23       (SIMM8_A16_9_S + 1)
    {3, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm3_23, extract_uimm3_23},

    /* UIMM10_6_S mask = 0000001111111111 */
#define UIMM10_6_S       (UIMM3_23 + 1)
    {10, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm10_6_s, extract_uimm10_6_s},

    /* UIMM6_11_S mask = 0000002200011110 */
#define UIMM6_11_S       (UIMM10_6_S + 1)
    {6, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm6_11_s, extract_uimm6_11_s},

    /* SIMM9_8 mask = 00000000111111112000000000000000 */
#define SIMM9_8       (UIMM6_11_S + 1)
    {9, 0, 0, ARC_OPERAND_SIGNED | ARC_OPERAND_IGNORE, insert_simm9_8, extract_simm9_8},

    /* UIMM10_A32_8_S mask = 0000000011111111 */
#define UIMM10_A32_8_S       (SIMM9_8 + 1)
    {10, 0, 0, ARC_OPERAND_UNSIGNED | ARC_OPERAND_ALIGNED32 | ARC_OPERAND_TRUNCATE, insert_uimm10_a32_8_s, extract_uimm10_a32_8_s},

    /* SIMM9_7_S mask = 0000000111111111 */
#define SIMM9_7_S       (UIMM10_A32_8_S + 1)
    {9, 0, 0, ARC_OPERAND_SIGNED, insert_simm9_7_s, extract_simm9_7_s},

    /* UIMM6_A16_11_S mask = 0000000000011111 */
#define UIMM6_A16_11_S       (SIMM9_7_S + 1)
    {6, 0, 0, ARC_OPERAND_UNSIGNED | ARC_OPERAND_ALIGNED16 | ARC_OPERAND_TRUNCATE, insert_uimm6_a16_11_s, extract_uimm6_a16_11_s},

    /* UIMM5_A32_11_S mask = 0000020000011000 */
#define UIMM5_A32_11_S       (UIMM6_A16_11_S + 1)
    {5, 0, 0, ARC_OPERAND_UNSIGNED | ARC_OPERAND_ALIGNED32 | ARC_OPERAND_TRUNCATE, insert_uimm5_a32_11_s, extract_uimm5_a32_11_s},

    /* SIMM11_A32_13_S mask = 0000022222200111 */
#define SIMM11_A32_13_S       (UIMM5_A32_11_S + 1)
    {11, 0, 0, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED32 | ARC_OPERAND_TRUNCATE, insert_simm11_a32_13_s, extract_simm11_a32_13_s},

    /* UIMM7_13_S mask = 0000000022220111 */
#define UIMM7_13_S       (SIMM11_A32_13_S + 1)
    {7, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm7_13_s, extract_uimm7_13_s},

    /* UIMM6_A16_21 mask = 00000000000000000000011111000000 */
#define UIMM6_A16_21       (UIMM7_13_S + 1)
    {6, 0, 0, ARC_OPERAND_UNSIGNED | ARC_OPERAND_ALIGNED16 | ARC_OPERAND_TRUNCATE, insert_uimm6_a16_21, extract_uimm6_a16_21},

    /* UIMM7_11_S mask = 0000022200011110 */
#define UIMM7_11_S       (UIMM6_A16_21 + 1)
    {7, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm7_11_s, extract_uimm7_11_s},

    /* UIMM7_A16_20 mask = 00000000000000000000111111000000 */
#define UIMM7_A16_20       (UIMM7_11_S + 1)
    {7, 0, -UIMM7_A16_20, ARC_OPERAND_UNSIGNED | ARC_OPERAND_ALIGNED16 | ARC_OPERAND_TRUNCATE | ARC_OPERAND_PCREL, insert_uimm7_a16_20, extract_uimm7_a16_20},

    /* SIMM13_A16_20 mask = 00000000000000000000111111222222 */
#define SIMM13_A16_20       (UIMM7_A16_20 + 1)
    {13, 0, -SIMM13_A16_20, ARC_OPERAND_SIGNED | ARC_OPERAND_ALIGNED16 | ARC_OPERAND_TRUNCATE | ARC_OPERAND_PCREL, insert_simm13_a16_20, extract_simm13_a16_20},

    /* UIMM8_8_S mask = 0000000011111111 */
#define UIMM8_8_S       (SIMM13_A16_20 + 1)
    {8, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm8_8_s, extract_uimm8_8_s},

    /* W6 mask = 00000000000000000000111111000000 */
#define W6       (UIMM8_8_S + 1)
    {6, 0, 0, ARC_OPERAND_SIGNED, insert_w6, extract_w6},

    /* UIMM6_5_S mask = 0000011111100000 */
#define UIMM6_5_S       (W6 + 1)
    {6, 0, 0, ARC_OPERAND_UNSIGNED, insert_uimm6_5_s, extract_uimm6_5_s},

#endif
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
#include "arc-opcodes.h"
  };

const unsigned arc_num_opcodes = sizeof(arc_opcodes)/sizeof(*arc_opcodes);

/* List with special cases instructions and the applicable flags */
const struct arc_flag_special arc_flag_special_cases[] =
  {
    { "b", { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO, F_POZITIVE, F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET, F_LOWER, F_CARRYCLR, F_NOTCARRY, F_HIGHER, F_OVERFLOWSET, F_OVERFLOW, F_NOTOVERFLOW, F_OVERFLOWCLR, F_GT, F_GE, F_LT, F_LE, F_HI, F_LS, F_PNZ, F_NULL } },
    { "bl", { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO, F_POZITIVE, F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET, F_LOWER, F_CARRYCLR, F_NOTCARRY, F_HIGHER, F_OVERFLOWSET, F_OVERFLOW, F_NOTOVERFLOW, F_OVERFLOWCLR, F_GT, F_GE, F_LT, F_LE, F_HI, F_LS, F_PNZ, F_NULL } },
    { "br", { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO, F_POZITIVE, F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET, F_LOWER, F_CARRYCLR, F_NOTCARRY, F_HIGHER, F_OVERFLOWSET, F_OVERFLOW, F_NOTOVERFLOW, F_OVERFLOWCLR, F_GT, F_GE, F_LT, F_LE, F_HI, F_LS, F_PNZ, F_NULL } },
    { "j", { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO, F_POZITIVE, F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET, F_LOWER, F_CARRYCLR, F_NOTCARRY, F_HIGHER, F_OVERFLOWSET, F_OVERFLOW, F_NOTOVERFLOW, F_OVERFLOWCLR, F_GT, F_GE, F_LT, F_LE, F_HI, F_LS, F_PNZ, F_NULL } },
    { "lp", { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO, F_POZITIVE, F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET, F_LOWER, F_CARRYCLR, F_NOTCARRY, F_HIGHER, F_OVERFLOWSET, F_OVERFLOW, F_NOTOVERFLOW, F_OVERFLOWCLR, F_GT, F_GE, F_LT, F_LE, F_HI, F_LS, F_PNZ, F_NULL } },
    { "set", { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO, F_POZITIVE, F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET, F_LOWER, F_CARRYCLR, F_NOTCARRY, F_HIGHER, F_OVERFLOWSET, F_OVERFLOW, F_NOTOVERFLOW, F_OVERFLOWCLR, F_GT, F_GE, F_LT, F_LE, F_HI, F_LS, F_PNZ, F_NULL } },
  };

const unsigned arc_num_flag_special = sizeof(arc_flag_special_cases)/sizeof(*arc_flag_special_cases);

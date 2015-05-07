/* arc-relax.h -- Relaxation for ARC Copyright 1994, 1995, 1997, 1998,
   2000, 2001, 2002, 2005, 2006, 2007, 2008, 2009, 2011, 2013, 2015 Free
   Software Foundation, Inc.

   Copyright 2015 Synopsys Inc
   Contributor: Janek van Oirschot <jvanoirs@synopsys.com>

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

/* (Static) data used for relaxation. Isn't the prettiest with some
   "magic numbers" but the intention is that this will eventually be
   automatically generated and thus the user doesn't have to completely
   understand it. */

#define RELAX_TABLE_ENTRY(BITS, ISSIGNED, SIZE, NEXT)           \
{ (ISSIGNED) ? ((1 << ((BITS) - 1)) - 1) : ((1 << (BITS)) - 1), \
  (ISSIGNED) ? -(1 << ((BITS) - 1)) : 0,                        \
  (SIZE),                                                       \
  (NEXT) }                                                      \

#define RELAX_TABLE_ENTRY_MAX(ISSIGNED, SIZE, NEXT) \
{ (ISSIGNED) ? 0x7FFFFFFF : 0xFFFFFFFF,             \
  (ISSIGNED) ? -(0x7FFFFFFF) : 0,                   \
  (SIZE),                                           \
  (NEXT) }                                          \


/* ARC relaxation table. */
const relax_typeS md_relax_table[] =
{
  /* Fake entry. */
  {0, 0, 0, 0},

  /* BL_S s13 ->
     BL s25 */
  RELAX_TABLE_ENTRY(13, 1, 2, ARC_RLX_BL),
  RELAX_TABLE_ENTRY(25, 1, 4, ARC_RLX_NONE),

  /* B_S s10 ->
     B s25 */
  RELAX_TABLE_ENTRY(10, 1, 2, ARC_RLX_B),
  RELAX_TABLE_ENTRY(25, 1, 4, ARC_RLX_NONE),

  /* ADD a,b,u6 ->
     ADD a,b,limm */
  RELAX_TABLE_ENTRY(6, 0, 4, ARC_RLX_ADD_LIMM),
  RELAX_TABLE_ENTRY_MAX(0, 8, ARC_RLX_NONE),

  /* LD<zz><.x><.aa><.di> a, [b, s9] ->
     LD<zz><.x><.aa><.di> a, [b, limm] */
  RELAX_TABLE_ENTRY(9, 1, 4, ARC_RLX_LD_LIMM),
  RELAX_TABLE_ENTRY_MAX(1, 8, ARC_RLX_NONE),
};

const struct arc_relaxable_ins arc_relaxable_insns[] =
  {
    { "bl", { IMMEDIATE }, { 0 }, "bl_s", 0, ARC_RLX_BL_S },
    { "b", { IMMEDIATE }, { 0 }, "b_s", 0, ARC_RLX_B_S },
    { "add", { REGISTER, REGISTER, IMMEDIATE }, { 5, 0 }, "add", 2, ARC_RLX_ADD_U6 },
    { "ld", { REGISTER, BRACKET, REGISTER_NO_GP, IMMEDIATE, BRACKET }, { 11, 4, 14, 17, 0 }, "ld", 3, ARC_RLX_LD_S9 },
  };

const unsigned arc_num_relaxable_ins = sizeof (arc_relaxable_insns)
					/ sizeof (*arc_relaxable_insns);

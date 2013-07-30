/* tc-arc.h - Macros and type defines for the ARC.
   Copyright 1994, 1995, 1997, 2000, 2001, 2002, 2005, 2007
   Free Software Foundation, Inc.

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

#define TC_ARC 1

#define TARGET_BYTES_BIG_ENDIAN 0

#define LOCAL_LABELS_FB 1

#define TARGET_ARCH bfd_arch_arc

#define DIFF_EXPR_OK
#define REGISTER_PREFIX '%'

#ifdef LITTLE_ENDIAN
#undef LITTLE_ENDIAN
#endif

#ifdef BIG_ENDIAN
#undef BIG_ENDIAN
#endif

#define LITTLE_ENDIAN   1234

#define BIG_ENDIAN      4321

#ifdef TARGET_BYTES_BIG_ENDIAN
#define DEFAULT_TARGET_FORMAT  "elf32-bigarc"
#define DEFAULT_BYTE_ORDER     BIG_ENDIAN
#else
#define DEFAULT_TARGET_FORMAT  "elf32-littlearc"
#define DEFAULT_BYTE_ORDER     LITTLE_ENDIAN
#endif

#define TARGET_FORMAT          DEFAULT_TARGET_FORMAT
#define WORKING_DOT_WORD
#define LISTING_HEADER         "ARC GAS "

#ifndef TARGET_BYTES_BIG_ENDIAN
#define TARGET_BYTES_BIG_ENDIAN 0
#endif

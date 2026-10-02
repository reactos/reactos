/* winduni.h -- header file for unicode support for windres program.
   Copyright (C) 1997-2026 Free Software Foundation, Inc.
   Written by Ian Lance Taylor, Cygnus Support.
   Rewritten by Kai Tietz, Onevision.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#ifndef __REACTOS__
#include "ansidecl.h"
#endif

#ifndef WINDUNI_H
#define WINDUNI_H

/* This header file declares the types and functions we use for
   unicode support in windres.  Our unicode support is very limited at
   present.

   We don't put this stuff in windres.h so that winduni.c doesn't have
   to include windres.h.  winduni.c needs to includes windows.h, and
   that would conflict with the definitions of Windows macros we
   already have in windres.h.  */

/* Use bfd_size_type to ensure a sufficient number of bits.  */
#ifndef DEFINED_RC_UINT_TYPE
#define DEFINED_RC_UINT_TYPE
typedef bfd_size_type rc_uint_type;
#endif

/* We use this type to hold a unicode character.  */

typedef unsigned short unichar;

/* Escape character translations.  */

#define ESCAPE_A  007
#define ESCAPE_B  010
#define ESCAPE_F  014
#define ESCAPE_N  012
#define ESCAPE_R  015
#define ESCAPE_T  011
#define ESCAPE_V  013

/* Convert an ASCII string to a unicode string.  */
extern void unicode_from_ascii (rc_uint_type *, unichar **, const char *);

/* Convert an unicode string to an ASCII string.  */
extern void ascii_from_unicode (rc_uint_type *, const unichar *, char **);

/* Duplicate a unicode string by using res_alloc.  */
extern unichar *unichar_dup (const unichar *);

/* Duplicate a unicode string by using res_alloc and make it uppercase.  */
extern unichar *unichar_dup_uppercase (const unichar *);

/* The count of unichar elements.  */
extern rc_uint_type unichar_len (const unichar *);

/* Print a unicode string to a file.  */
extern void unicode_print (FILE *, const unichar *, rc_uint_type);

/* Print a ascii string to a file.  */
extern void ascii_print (FILE *, const char *, rc_uint_type);

/* Print a quoted unicode string to a file.  */
extern void unicode_print_quoted (FILE *, const unichar *, rc_uint_type);

#ifndef CP_UTF8
#define CP_UTF7	65000   /* UTF-7 translation.  */
#define CP_UTF8	65001   /* UTF-8 translation.  */
#endif

#ifndef CP_UTF16
#define CP_UTF16  65002	/* UTF-16 translation.  */
#endif

#ifndef CP_ACP
#define CP_ACP	0	/* Default to ANSI code page.  */
#endif

#ifndef CP_OEM
#define CP_OEM  1	/* Default OEM code page. */
#endif

/* Specifies the default codepage to be used for unicode
   transformations.  By default this is CP_ACP.  */
extern rc_uint_type wind_default_codepage;

/* Specifies the currently used codepage for unicode
   transformations.  By default this is CP_ACP.  */
extern rc_uint_type wind_current_codepage;

typedef struct wind_language_t
{
  unsigned id;
  unsigned doscp;
  unsigned wincp;
  const char *name;
  const char *country;
} wind_language_t;

extern const wind_language_t *wind_find_language_by_id (unsigned);
extern int unicode_is_valid_codepage (rc_uint_type);

typedef struct local_iconv_map
{
  rc_uint_type codepage;
  const char * iconv_name;
} local_iconv_map;

extern const local_iconv_map *wind_find_codepage_info (unsigned);

/* Convert an Codepage string to a unicode string.  */
extern void unicode_from_codepage (rc_uint_type *, unichar **, const char *, rc_uint_type);
extern void unicode_from_ascii_len (rc_uint_type *, unichar **, const char *, rc_uint_type );

/* Convert an unicode string to an codepage string.  */
extern void codepage_from_unicode (rc_uint_type *, const unichar *, char **, rc_uint_type);

/* Windres support routine called by unicode_from_ascii.  This is both
   here and in windres.h.  It should probably be in a separate header
   file, but it hardly seems worth it for one function.  */

extern void * res_alloc (rc_uint_type);

#endif

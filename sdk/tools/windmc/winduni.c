/* winduni.c -- unicode support for the windres program.
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


/* This file contains unicode support routines for the windres
   program.  Ideally, we would have generic unicode support which
   would work on all systems.  However, we don't.  Instead, on a
   Windows host, we are prepared to call some Windows routines.  This
   means that we will generate different output on Windows and Unix
   hosts, but that seems better than not really supporting unicode at
   all.  */

#ifdef __REACTOS__
#include "windmc_ros.h"
#else
#include "sysdep.h"
#include "bfd.h"
#include "libiberty.h" /* for xstrdup */
#include "bucomm.h"
#endif
/* Must be include before windows.h and winnls.h.  */
#if defined (_WIN32) || defined (__CYGWIN__)
#include <windows.h>
#include <winnls.h>
#endif
#include "winduni.h"
#ifndef __REACTOS__
#include "safe-ctype.h"
#endif

#if HAVE_ICONV
#include <iconv.h>
#endif

static rc_uint_type wind_WideCharToMultiByte (rc_uint_type, const unichar *, char *, rc_uint_type);
static rc_uint_type wind_MultiByteToWideChar (rc_uint_type, const char *, unichar *, rc_uint_type);
static int unichar_isascii (const unichar *, rc_uint_type);

/* Convert an ASCII string to a unicode string.  We just copy it,
   expanding chars to shorts, rather than doing something intelligent.  */

#if !defined (_WIN32) && !defined (__CYGWIN__)

/* Codepages mapped.  */
static local_iconv_map codepages[] =
{
  { 0, "cp1252" },
  { 1, "WINDOWS-1252" },
  { 437, "MS-ANSI" },
  { 737, "MS-GREEK" },
  { 775, "WINBALTRIM" },
  { 850, "MS-ANSI" },
  { 852, "MS-EE" },
  { 857, "MS-TURK" },
  { 862, "CP862" },
  { 864, "CP864" },
  { 866, "MS-CYRL" },
  { 874, "WINDOWS-874" },
  { 932, "CP932" },
  { 936, "CP936" },
  { 949, "CP949" },
  { 950, "CP950" },
  { 1250, "WINDOWS-1250" },
  { 1251, "WINDOWS-1251" },
  { 1252, "WINDOWS-1252" },
  { 1253, "WINDOWS-1253" },
  { 1254, "WINDOWS-1254" },
  { 1255, "WINDOWS-1255" },
  { 1256, "WINDOWS-1256" },
  { 1257, "WINDOWS-1257" },
  { 1258, "WINDOWS-1258" },
  { CP_UTF7, "UTF-7" },
  { CP_UTF8, "UTF-8" },
  { CP_UTF16, "UTF-16LE" },
  { (rc_uint_type) -1, NULL }
};

/* Languages supported.  */
static const wind_language_t languages[] =
{
  { 0x0000, 437, 1252, "Neutral", "Neutral" },
  { 0x0401, 864, 1256, "Arabic", "Saudi Arabia" },    { 0x0402, 866, 1251, "Bulgarian", "Bulgaria" },
  { 0x0403, 850, 1252, "Catalan", "Spain" },	      { 0x0404, 950,  950, "Chinese", "Taiwan" },
  { 0x0405, 852, 1250, "Czech", "Czech Republic" },   { 0x0406, 850, 1252, "Danish", "Denmark" },
  { 0x0407, 850, 1252, "German", "Germany" },	      { 0x0408, 737, 1253, "Greek", "Greece" },
  { 0x0409, 437, 1252, "English", "United States" },  { 0x040A, 850, 1252, "Spanish - Traditional Sort", "Spain" },
  { 0x040B, 850, 1252, "Finnish", "Finland" },	      { 0x040C, 850, 1252, "French", "France" },
  { 0x040D, 862, 1255, "Hebrew", "Israel" },	      { 0x040E, 852, 1250, "Hungarian", "Hungary" },
  { 0x040F, 850, 1252, "Icelandic", "Iceland" },      { 0x0410, 850, 1252, "Italian", "Italy" },
  { 0x0411, 932,  932, "Japanese", "Japan" },	      { 0x0412, 949,  949, "Korean", "Korea (south)" },
  { 0x0413, 850, 1252, "Dutch", "Netherlands" },      { 0x0414, 850, 1252, "Norwegian (Bokm\345l)", "Norway" },
  { 0x0415, 852, 1250, "Polish", "Poland" },	      { 0x0416, 850, 1252, "Portuguese", "Brazil" },
  { 0x0418, 852, 1250, "Romanian", "Romania" },	      { 0x0419, 866, 1251, "Russian", "Russia" },
  { 0x041A, 852, 1250, "Croatian", "Croatia" },	      { 0x041B, 852, 1250, "Slovak", "Slovakia" },
  { 0x041C, 852, 1250, "Albanian", "Albania" },	      { 0x041D, 850, 1252, "Swedish", "Sweden" },
  { 0x041E, 874,  874, "Thai", "Thailand" },	      { 0x041F, 857, 1254, "Turkish", "Turkey" },
  { 0x0421, 850, 1252, "Indonesian", "Indonesia" },   { 0x0422, 866, 1251, "Ukrainian", "Ukraine" },
  { 0x0423, 866, 1251, "Belarusian", "Belarus" },     { 0x0424, 852, 1250, "Slovene", "Slovenia" },
  { 0x0425, 775, 1257, "Estonian", "Estonia" },	      { 0x0426, 775, 1257, "Latvian", "Latvia" },
  { 0x0427, 775, 1257, "Lithuanian", "Lithuania" },
  { 0x0429, 864, 1256, "Arabic", "Farsi" },	      { 0x042A,1258, 1258, "Vietnamese", "Vietnam" },
  { 0x042D, 850, 1252, "Basque", "Spain" },
  { 0x042F, 866, 1251, "Macedonian", "Former Yugoslav Republic of Macedonia" },
  { 0x0436, 850, 1252, "Afrikaans", "South Africa" },
  { 0x0438, 850, 1252, "Faroese", "Faroe Islands" },
  { 0x043C, 437, 1252, "Irish", "Ireland" },
  { 0x043E, 850, 1252, "Malay", "Malaysia" },
  { 0x0801, 864, 1256, "Arabic", "Iraq" },
  { 0x0804, 936,  936, "Chinese (People's republic of China)", "People's republic of China" },
  { 0x0807, 850, 1252, "German", "Switzerland" },
  { 0x0809, 850, 1252, "English", "United Kingdom" }, { 0x080A, 850, 1252, "Spanish", "Mexico" },
  { 0x080C, 850, 1252, "French", "Belgium" },
  { 0x0810, 850, 1252, "Italian", "Switzerland" },
  { 0x0813, 850, 1252, "Dutch", "Belgium" },	      { 0x0814, 850, 1252, "Norwegian (Nynorsk)", "Norway" },
  { 0x0816, 850, 1252, "Portuguese", "Portugal" },
  { 0x081A, 852, 1252, "Serbian (latin)", "Yugoslavia" },
  { 0x081D, 850, 1252, "Swedish (Finland)", "Finland" },
  { 0x0C01, 864, 1256, "Arabic", "Egypt" },
  { 0x0C04, 950,  950, "Chinese", "Hong Kong" },
  { 0x0C07, 850, 1252, "German", "Austria" },
  { 0x0C09, 850, 1252, "English", "Australia" },      { 0x0C0A, 850, 1252, "Spanish - International Sort", "Spain" },
  { 0x0C0C, 850, 1252, "French", "Canada"},
  { 0x0C1A, 855, 1251, "Serbian (Cyrillic)", "Serbia" },
  { 0x1001, 864, 1256, "Arabic", "Libya" },
  { 0x1004, 936,  936, "Chinese", "Singapore" },
  { 0x1007, 850, 1252, "German", "Luxembourg" },
  { 0x1009, 850, 1252, "English", "Canada" },
  { 0x100A, 850, 1252, "Spanish", "Guatemala" },
  { 0x100C, 850, 1252, "French", "Switzerland" },
  { 0x1401, 864, 1256, "Arabic", "Algeria" },
  { 0x1407, 850, 1252, "German", "Liechtenstein" },
  { 0x1409, 850, 1252, "English", "New Zealand" },    { 0x140A, 850, 1252, "Spanish", "Costa Rica" },
  { 0x140C, 850, 1252, "French", "Luxembourg" },
  { 0x1801, 864, 1256, "Arabic", "Morocco" },
  { 0x1809, 850, 1252, "English", "Ireland" },	      { 0x180A, 850, 1252, "Spanish", "Panama" },
  { 0x180C, 850, 1252, "French", "Monaco" },
  { 0x1C01, 864, 1256, "Arabic", "Tunisia" },
  { 0x1C09, 437, 1252, "English", "South Africa" },   { 0x1C0A, 850, 1252, "Spanish", "Dominican Republic" },
  { 0x2001, 864, 1256, "Arabic", "Oman" },
  { 0x2009, 850, 1252, "English", "Jamaica" },	      { 0x200A, 850, 1252, "Spanish", "Venezuela" },
  { 0x2401, 864, 1256, "Arabic", "Yemen" },
  { 0x2409, 850, 1252, "English", "Caribbean" },      { 0x240A, 850, 1252, "Spanish", "Colombia" },
  { 0x2801, 864, 1256, "Arabic", "Syria" },
  { 0x2809, 850, 1252, "English", "Belize" },	      { 0x280A, 850, 1252, "Spanish", "Peru" },
  { 0x2C01, 864, 1256, "Arabic", "Jordan" },
  { 0x2C09, 437, 1252, "English", "Trinidad & Tobago" },{ 0x2C0A, 850, 1252, "Spanish", "Argentina" },
  { 0x3001, 864, 1256, "Arabic", "Lebanon" },
  { 0x3009, 437, 1252, "English", "Zimbabwe" },	      { 0x300A, 850, 1252, "Spanish", "Ecuador" },
  { 0x3401, 864, 1256, "Arabic", "Kuwait" },
  { 0x3409, 437, 1252, "English", "Philippines" },    { 0x340A, 850, 1252, "Spanish", "Chile" },
  { 0x3801, 864, 1256, "Arabic", "United Arab Emirates" },
  { 0x380A, 850, 1252, "Spanish", "Uruguay" },
  { 0x3C01, 864, 1256, "Arabic", "Bahrain" },
  { 0x3C0A, 850, 1252, "Spanish", "Paraguay" },
  { 0x4001, 864, 1256, "Arabic", "Qatar" },
  { 0x400A, 850, 1252, "Spanish", "Bolivia" },
  { 0x440A, 850, 1252, "Spanish", "El Salvador" },
  { 0x480A, 850, 1252, "Spanish", "Honduras" },
  { 0x4C0A, 850, 1252, "Spanish", "Nicaragua" },
  { 0x500A, 850, 1252, "Spanish", "Puerto Rico" },
  { (unsigned) -1,  0,      0, NULL, NULL }
};

#endif

/* Specifies the default codepage to be used for unicode
   transformations.  By default this is CP_ACP.  */
rc_uint_type wind_default_codepage = CP_ACP;

/* Specifies the currently used codepage for unicode
   transformations.  By default this is CP_ACP.  */
rc_uint_type wind_current_codepage = CP_ACP;

/* Convert an ASCII string to a unicode string.  We just copy it,
   expanding chars to shorts, rather than doing something intelligent.  */

void
unicode_from_ascii (rc_uint_type *length, unichar **unicode, const char *ascii)
{
  unicode_from_codepage (length, unicode, ascii, wind_current_codepage);
}

/* Convert an ASCII string with length A_LENGTH to a unicode string.  We just
   copy it, expanding chars to shorts, rather than doing something intelligent.
   This routine converts also \0 within a string.  */

void
unicode_from_ascii_len (rc_uint_type *length, unichar **unicode, const char *ascii, rc_uint_type a_length)
{
  char *tmp, *p;
  rc_uint_type tlen, elen, idx = 0;

  *unicode = NULL;

  if (!a_length)
    {
      if (length)
        *length = 0;
      return;
    }

  /* Make sure we have zero terminated string.  */
  p = tmp = (char *) xmalloc (a_length + 1);
  memcpy (tmp, ascii, a_length);
  tmp[a_length] = 0;

  while (a_length > 0)
    {
      unichar *utmp, *up;

      tlen = strlen (p);

      if (tlen > a_length)
        tlen = a_length;
      if (*p == 0)
        {
	  /* Make room for one more character.  */
	  utmp = (unichar *) res_alloc (sizeof (unichar) * (idx + 1));
	  if (idx > 0)
	    {
	      memcpy (utmp, *unicode, idx * sizeof (unichar));
	    }
	  *unicode = utmp;
	  utmp[idx++] = 0;
	  --a_length;
	  p++;
	  continue;
	}
      utmp = NULL;
      elen = 0;
      elen = wind_MultiByteToWideChar (wind_current_codepage, p, NULL, 0);
      if (elen)
	{
	  utmp = ((unichar *) res_alloc (elen + sizeof (unichar) * 2));
	  wind_MultiByteToWideChar (wind_current_codepage, p, utmp, elen);
	  elen /= sizeof (unichar);
	  elen --;
	}
      else
        {
	  /* Make room for one more character.  */
	  utmp = (unichar *) res_alloc (sizeof (unichar) * (idx + 1));
	  if (idx > 0)
	    {
	      memcpy (utmp, *unicode, idx * sizeof (unichar));
	    }
	  *unicode = utmp;
	  utmp[idx++] = ((unichar) *p) & 0xff;
	  --a_length;
	  p++;
	  continue;
	}
      p += tlen;
      a_length -= tlen;

      up = (unichar *) res_alloc (sizeof (unichar) * (idx + elen));
      if (idx > 0)
	memcpy (up, *unicode, idx * sizeof (unichar));

      *unicode = up;
      if (elen)
	memcpy (&up[idx], utmp, sizeof (unichar) * elen);

      idx += elen;
    }

  if (length)
    *length = idx;

  free (tmp);
}

/* Convert an unicode string to an ASCII string.  We just copy it,
   shrink shorts to chars, rather than doing something intelligent.
   Shorts with not within the char range are replaced by '_'.  */

void
ascii_from_unicode (rc_uint_type *length, const unichar *unicode, char **ascii)
{
  codepage_from_unicode (length, unicode, ascii, wind_current_codepage);
}

/* Print the unicode string UNICODE to the file E.  LENGTH is the
   number of characters to print, or -1 if we should print until the
   end of the string.  FIXME: On a Windows host, we should be calling
   some Windows function, probably WideCharToMultiByte.  */

void
unicode_print (FILE *e, const unichar *unicode, rc_uint_type length)
{
  while (1)
    {
      unichar ch;

      if (length == 0)
	return;
      if ((bfd_signed_vma) length > 0)
	--length;

      ch = *unicode;

      if (ch == 0 && (bfd_signed_vma) length < 0)
	return;

      ++unicode;

      if ((ch & 0x7f) == ch)
	{
	  if (ch == '\\')
	    fputs ("\\\\", e);
	  else if (ch == '"')
	    fputs ("\"\"", e);
	  else if (ISPRINT (ch))
	    putc (ch, e);
	  else
	    {
	      switch (ch)
		{
		case ESCAPE_A:
		  fputs ("\\a", e);
		  break;

		case ESCAPE_B:
		  fputs ("\\b", e);
		  break;

		case ESCAPE_F:
		  fputs ("\\f", e);
		  break;

		case ESCAPE_N:
		  fputs ("\\n", e);
		  break;

		case ESCAPE_R:
		  fputs ("\\r", e);
		  break;

		case ESCAPE_T:
		  fputs ("\\t", e);
		  break;

		case ESCAPE_V:
		  fputs ("\\v", e);
		  break;

		default:
		  fprintf (e, "\\%03o", (unsigned int) ch);
		  break;
		}
	    }
	}
      else if ((ch & 0xff) == ch)
	fprintf (e, "\\%03o", (unsigned int) ch);
      else
	fprintf (e, "\\x%04x", (unsigned int) ch);
    }
}

/* Print a unicode string to a file.  */

void
ascii_print (FILE *e, const char *s, rc_uint_type length)
{
  while (1)
    {
      char ch;

      if (length == 0)
	return;
      if ((bfd_signed_vma) length > 0)
	--length;

      ch = *s;

      if (ch == 0 && (bfd_signed_vma) length < 0)
	return;

      ++s;

      if ((ch & 0x7f) == ch)
	{
	  if (ch == '\\')
	    fputs ("\\\\", e);
	  else if (ch == '"')
	    fputs ("\"\"", e);
	  else if (ISPRINT (ch))
	    putc (ch, e);
	  else
	    {
	      switch (ch)
		{
		case ESCAPE_A:
		  fputs ("\\a", e);
		  break;

		case ESCAPE_B:
		  fputs ("\\b", e);
		  break;

		case ESCAPE_F:
		  fputs ("\\f", e);
		  break;

		case ESCAPE_N:
		  fputs ("\\n", e);
		  break;

		case ESCAPE_R:
		  fputs ("\\r", e);
		  break;

		case ESCAPE_T:
		  fputs ("\\t", e);
		  break;

		case ESCAPE_V:
		  fputs ("\\v", e);
		  break;

		default:
		  fprintf (e, "\\%03o", (unsigned int) ch);
		  break;
		}
	    }
	}
      else
	fprintf (e, "\\%03o", (unsigned int) ch & 0xff);
    }
}

rc_uint_type
unichar_len (const unichar *unicode)
{
  rc_uint_type r = 0;

  if (unicode)
    while (unicode[r] != 0)
      r++;
  else
    --r;
  return r;
}

unichar *
unichar_dup (const unichar *unicode)
{
  unichar *r;
  int len;

  if (! unicode)
    return NULL;
  for (len = 0; unicode[len] != 0; ++len)
    ;
  ++len;
  r = ((unichar *) res_alloc (len * sizeof (unichar)));
  memcpy (r, unicode, len * sizeof (unichar));
  return r;
}

unichar *
unichar_dup_uppercase (const unichar *u)
{
  unichar *r = unichar_dup (u);
  int i;

  if (! r)
    return NULL;

  for (i = 0; r[i] != 0; ++i)
    {
      if (r[i] >= 'a' && r[i] <= 'z')
	r[i] &= 0xdf;
    }
  return r;
}

static int
unichar_isascii (const unichar *u, rc_uint_type len)
{
  rc_uint_type i;

  if ((bfd_signed_vma) len < 0)
    {
      if (u)
	len = (rc_uint_type) unichar_len (u);
      else
	len = 0;
    }

  for (i = 0; i < len; i++)
    if ((u[i] & 0xff80) != 0)
      return 0;
  return 1;
}

void
unicode_print_quoted (FILE *e, const unichar *u, rc_uint_type len)
{
  if (! unichar_isascii (u, len))
    fputc ('L', e);
  fputc ('"', e);
  unicode_print (e, u, len);
  fputc ('"', e);
}

int
unicode_is_valid_codepage (rc_uint_type cp)
{
  if ((cp & 0xffff) != cp)
    return 0;
  if (cp == CP_UTF16 || cp == CP_ACP)
    return 1;

#if !defined (_WIN32) && !defined (__CYGWIN__)
  if (! wind_find_codepage_info (cp))
    return 0;
  return 1;
#else
  return !! IsValidCodePage ((UINT) cp);
#endif
}

#if defined (_WIN32) || defined (__CYGWIN__)

#define max_cp_string_len 6

static unsigned int
codepage_from_langid (unsigned short langid)
{
  char cp_string [max_cp_string_len];
  int c;

  memset (cp_string, 0, max_cp_string_len);
  /* LOCALE_RETURN_NUMBER flag would avoid strtoul conversion,
     but is unavailable on Win95.  */
  c = GetLocaleInfoA (MAKELCID (langid, SORT_DEFAULT),
  		      LOCALE_IDEFAULTANSICODEPAGE,
  		      cp_string, max_cp_string_len);
  /* If codepage data for an LCID is not installed on users's system,
     GetLocaleInfo returns an empty string.  Fall back to system ANSI
     default. */
  if (c == 0)
    return CP_ACP;
  return strtoul (cp_string, 0, 10);
}

static unsigned int
wincodepage_from_langid (unsigned short langid)
{
  char cp_string [max_cp_string_len];
  int c;

  memset (cp_string, 0, max_cp_string_len);
  /* LOCALE_RETURN_NUMBER flag would avoid strtoul conversion,
     but is unavailable on Win95.  */
  c = GetLocaleInfoA (MAKELCID (langid, SORT_DEFAULT),
		      LOCALE_IDEFAULTCODEPAGE,
		      cp_string, max_cp_string_len);
  /* If codepage data for an LCID is not installed on users's system,
     GetLocaleInfo returns an empty string.  Fall back to system ANSI
     default. */
  if (c == 0)
    return CP_OEM;
  return strtoul (cp_string, 0, 10);
}

static char *
lang_from_langid (unsigned short langid)
{
  char cp_string[261];
  int c;

  memset (cp_string, 0, 261);
  c = GetLocaleInfoA (MAKELCID (langid, SORT_DEFAULT),
  		      LOCALE_SENGLANGUAGE,
  		      cp_string, 260);
  /* If codepage data for an LCID is not installed on users's system,
     GetLocaleInfo returns an empty string.  Fall back to system ANSI
     default. */
  if (c == 0)
    strcpy (cp_string, "Neutral");
  return xstrdup (cp_string);
}

static char *
country_from_langid (unsigned short langid)
{
  char cp_string[261];
  int c;

  memset (cp_string, 0, 261);
  c = GetLocaleInfoA (MAKELCID (langid, SORT_DEFAULT),
  		      LOCALE_SENGCOUNTRY,
  		      cp_string, 260);
  /* If codepage data for an LCID is not installed on users's system,
     GetLocaleInfo returns an empty string.  Fall back to system ANSI
     default. */
  if (c == 0)
    strcpy (cp_string, "Neutral");
  return xstrdup (cp_string);
}

#endif

const wind_language_t *
wind_find_language_by_id (unsigned id)
{
#if !defined (_WIN32) && !defined (__CYGWIN__)
  int i;

  if (! id)
    return NULL;
  for (i = 0; languages[i].id != (unsigned) -1 && languages[i].id != id; i++)
    ;
  if (languages[i].id == id)
    return &languages[i];
  return NULL;
#else
  static wind_language_t wl;

  wl.id = id;
  wl.doscp = codepage_from_langid ((unsigned short) id);
  wl.wincp = wincodepage_from_langid ((unsigned short) id);
  wl.name = lang_from_langid ((unsigned short) id);
  wl.country = country_from_langid ((unsigned short) id);

  return & wl;
#endif
}

const local_iconv_map *
wind_find_codepage_info (unsigned cp)
{
#if !defined (_WIN32) && !defined (__CYGWIN__)
  int i;

  for (i = 0; codepages[i].codepage != (rc_uint_type) -1 && codepages[i].codepage != cp; i++)
    ;
  if (codepages[i].codepage == (rc_uint_type) -1)
    return NULL;
  return &codepages[i];
#else
  static local_iconv_map lim;
  if (!unicode_is_valid_codepage (cp))
  	return NULL;
  lim.codepage = cp;
  lim.iconv_name = "";
  return & lim;
#endif
}

/* Convert an Codepage string to a unicode string.  */

void
unicode_from_codepage (rc_uint_type *length, unichar **u, const char *src, rc_uint_type cp)
{
  rc_uint_type len;

  len = wind_MultiByteToWideChar (cp, src, NULL, 0);
  if (len)
    {
      *u = ((unichar *) res_alloc (len));
      wind_MultiByteToWideChar (cp, src, *u, len);
    }
  /* Discount the trailing '/0'.  If MultiByteToWideChar failed,
     this will set *length to -1.  */
  len -= sizeof (unichar);

  if (length != NULL)
    *length = len / sizeof (unichar);
}

/* Convert an unicode string to an codepage string.  */

void
codepage_from_unicode (rc_uint_type *length, const unichar *unicode, char **ascii, rc_uint_type cp)
{
  rc_uint_type len;

  len = wind_WideCharToMultiByte (cp, unicode, NULL, 0);
  if (len)
    {
      *ascii = (char *) res_alloc (len * sizeof (char));
      wind_WideCharToMultiByte (cp, unicode, *ascii, len);
    }
  /* Discount the trailing '/0'.  If MultiByteToWideChar failed,
     this will set *length to -1.  */
  len--;

  if (length != NULL)
    *length = len;
}

#if defined (HAVE_ICONV) && !defined (_WIN32) && !defined (__CYGWIN__)
static int
iconv_onechar (iconv_t cd, ICONV_CONST char *s, char *d, int d_len, const char **n_s, char **n_d)
{
  int i;

  for (i = 1; i <= 32; i++)
    {
      char *tmp_d = d;
      ICONV_CONST char *tmp_s = s;
      size_t ret;
      size_t s_left = (size_t) i;
      size_t d_left = (size_t) d_len;

      ret = iconv (cd, & tmp_s, & s_left, & tmp_d, & d_left);

      if (ret != (size_t) -1)
	{
	  *n_s = tmp_s;
	  *n_d = tmp_d;
	  return 0;
	}
    }

  return 1;
}

static const char *
wind_iconv_cp (rc_uint_type cp)
{
  const local_iconv_map *lim = wind_find_codepage_info (cp);

  if (!lim)
    return NULL;
  return lim->iconv_name;
}
#endif /* HAVE_ICONV */

static rc_uint_type
wind_MultiByteToWideChar (rc_uint_type cp, const char *mb,
			  unichar *u, rc_uint_type u_len)
{
  rc_uint_type ret = 0;

#if defined (_WIN32) || defined (__CYGWIN__)
  rc_uint_type conv_flags = MB_PRECOMPOSED;

  /* MB_PRECOMPOSED is not allowed for UTF-7 or UTF-8.
     MultiByteToWideChar will set the last error to
     ERROR_INVALID_FLAGS if we do. */
  if (cp == CP_UTF8 || cp == CP_UTF7)
    conv_flags = 0;

  ret = (rc_uint_type) MultiByteToWideChar (cp, conv_flags,
					    mb, -1, u, u_len);
  /* Convert to bytes. */
  ret *= sizeof (unichar);

#elif defined (HAVE_ICONV)
  int first = 1;
  char tmp[32];
  char *p_tmp;
  const char *iconv_name = wind_iconv_cp (cp);

  if (!mb || !iconv_name)
    return 0;
  iconv_t cd = iconv_open (
#if WORDS_BIGENDIAN
			   "UTF-16BE",
#else
			   "UTF-16LE",
#endif
			   iconv_name);

  while (1)
    {
      int iret;
      const char *n_mb = "";
      char *n_tmp = "";

      p_tmp = tmp;
      iret = iconv_onechar (cd, (ICONV_CONST char *) mb, p_tmp, 32, & n_mb, & n_tmp);
      if (first)
	{
	  first = 0;
	  continue;
	}
      if (!iret)
	{
	  size_t l_tmp = (size_t) (n_tmp - p_tmp);

	  if (u)
	    {
	      if ((size_t) u_len < l_tmp)
		break;
	      memcpy (u, tmp, l_tmp);
	      u += l_tmp/2;
	      u_len -= l_tmp;
	    }
	  ret += l_tmp;
	}
      else
	break;
      if (tmp[0] == 0 && tmp[1] == 0)
	break;
      mb = n_mb;
    }
  iconv_close (cd);
#else
  if (cp)
    ret = 0;
  ret = strlen (mb) + 1;
  ret *= sizeof (unichar);
  if (u != NULL && u_len != 0)
    {
      do
	{
	  *u++ = ((unichar) *mb) & 0xff;
	  --u_len; mb++;
	}
      while (u_len != 0 && mb[-1] != 0);
    }
  if (u != NULL && u_len != 0)
    *u = 0;
#endif
  return ret;
}

static rc_uint_type
wind_WideCharToMultiByte (rc_uint_type cp, const unichar *u, char *mb, rc_uint_type mb_len)
{
  rc_uint_type ret = 0;
#if defined (_WIN32) || defined (__CYGWIN__)
  WINBOOL used_def = false;

  ret = (rc_uint_type) WideCharToMultiByte (cp, 0, u, -1, mb, mb_len,
				      	    NULL, & used_def);
#elif defined (HAVE_ICONV)
  int first = 1;
  char tmp[32];
  char *p_tmp;
  const char *iconv_name = wind_iconv_cp (cp);

  if (!u || !iconv_name)
    return 0;
  iconv_t cd = iconv_open (iconv_name,
#if WORDS_BIGENDIAN
			   "UTF-16BE"
#else
			   "UTF-16LE"
#endif
			   );

  while (1)
    {
      int iret;
      const char *n_u = "";
      char *n_tmp = "";

      p_tmp = tmp;
      iret = iconv_onechar (cd, (ICONV_CONST char *) u, p_tmp, 32, &n_u, & n_tmp);
      if (first)
	{
	  first = 0;
	  continue;
	}
      if (!iret)
	{
	  size_t l_tmp = (size_t) (n_tmp - p_tmp);

	  if (mb)
	    {
	      if ((size_t) mb_len < l_tmp)
		break;
	      memcpy (mb, tmp, l_tmp);
	      mb += l_tmp;
	      mb_len -= l_tmp;
	    }
	  ret += l_tmp;
	}
      else
	break;
      if (u[0] == 0)
	break;
      u = (const unichar *) n_u;
    }
  iconv_close (cd);
#else
  if (cp)
    ret = 0;

  while (u[ret] != 0)
    ++ret;

  ++ret;

  if (mb)
    {
      while (*u != 0 && mb_len != 0)
	{
	  if (u[0] == (u[0] & 0x7f))
	    *mb++ = (char) u[0];
	  else
	    *mb++ = '_';
	  ++u; --mb_len;
	}
      if (mb_len != 0)
	*mb = 0;
    }
#endif
  return ret;
}

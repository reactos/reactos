/*
 * nls/cptable.c
 * Copyright (C) 1996, Onno Hovers
 * 
 */

#include <windows.h>
#include <kernel32/lctable.h>
#include <kernel32/cptable.h>

extern CODEPAGE __CP37;
extern WCHAR  *__CP37_ToUnicode;
extern CHAR  **__CP37_FromUnicode;

extern CODEPAGE __CP437;
extern WCHAR  *__CP437_ToUnicode;
extern WCHAR  *__CP437_ToUnicodeGlyph;
extern CHAR  **__CP437_FromUnicode;

extern CODEPAGE __CP500;
extern WCHAR  *__CP500_ToUnicode;
extern CHAR  **__CP500_FromUnicode;

extern CODEPAGE __CP737;
extern WCHAR  *__CP737_ToUnicode;
extern WCHAR  *__CP737_ToUnicodeGlyph;
extern CHAR  **__CP737_FromUnicode;

extern CODEPAGE __CP775;
extern WCHAR  *__CP775_ToUnicode;
extern WCHAR  *__CP775_ToUnicodeGlyph;
extern CHAR  **__CP775_FromUnicode;

extern CODEPAGE __CP850;
extern WCHAR  *__CP850_ToUnicode;
extern WCHAR  *__CP850_ToUnicodeGlyph;
extern CHAR  **__CP850_FromUnicode;

extern CODEPAGE __CP852;
extern WCHAR  *__CP852_ToUnicode;
extern WCHAR  *__CP852_ToUnicodeGlyph;
extern CHAR  **__CP852_FromUnicode;

extern CODEPAGE __CP855;
extern WCHAR  *__CP855_ToUnicode;
extern WCHAR  *__CP855_ToUnicodeGlyph;
extern CHAR  **__CP855_FromUnicode;

extern CODEPAGE __CP857;
extern WCHAR  *__CP857_ToUnicode;
extern WCHAR  *__CP857_ToUnicodeGlyph;
extern CHAR  **__CP857_FromUnicode;

extern CODEPAGE __CP860;
extern WCHAR  *__CP860_ToUnicode;
extern WCHAR  *__CP860_ToUnicodeGlyph;
extern CHAR  **__CP860_FromUnicode;

extern CODEPAGE __CP861;
extern WCHAR  *__CP861_ToUnicode;
extern WCHAR  *__CP861_ToUnicodeGlyph;
extern CHAR  **__CP861_FromUnicode;

extern CODEPAGE __CP863;
extern WCHAR  *__CP863_ToUnicode;
extern WCHAR  *__CP863_ToUnicodeGlyph;
extern CHAR  **__CP863_FromUnicode;

extern CODEPAGE __CP865;
extern WCHAR  *__CP865_ToUnicode;
extern WCHAR  *__CP865_ToUnicodeGlyph;
extern CHAR  **__CP865_FromUnicode;

extern CODEPAGE __CP866;
extern WCHAR  *__CP866_ToUnicode;
extern WCHAR  *__CP866_ToUnicodeGlyph;
extern CHAR  **__CP866_FromUnicode;

extern CODEPAGE __CP869;
extern WCHAR  *__CP869_ToUnicode;
extern WCHAR  *__CP869_ToUnicodeGlyph;
extern CHAR  **__CP869_FromUnicode;

extern CODEPAGE __CP875;
extern WCHAR  *__CP875_ToUnicode;
extern CHAR  **__CP875_FromUnicode;

extern CODEPAGE __CP1026;
extern WCHAR  *__CP1026_ToUnicode;
extern CHAR  **__CP1026_FromUnicode;

extern CODEPAGE __CP1250;
extern WCHAR  *__CP1250_ToUnicode;
extern CHAR  **__CP1250_FromUnicode;

extern CODEPAGE __CP1251;
extern WCHAR  *__CP1251_ToUnicode;
extern CHAR  **__CP1251_FromUnicode;

extern CODEPAGE __CP1252;
extern WCHAR  *__CP1252_ToUnicode;
extern CHAR  **__CP1252_FromUnicode;

extern CODEPAGE __CP1253;
extern WCHAR  *__CP1253_ToUnicode;
extern CHAR  **__CP1253_FromUnicode;

extern CODEPAGE __CP1254;
extern WCHAR  *__CP1254_ToUnicode;
extern CHAR  **__CP1254_FromUnicode;

extern CODEPAGE __CP1255;
extern WCHAR  *__CP1255_ToUnicode;
extern CHAR  **__CP1255_FromUnicode;

extern CODEPAGE __CP1256;
extern WCHAR  *__CP1256_ToUnicode;
extern CHAR  **__CP1256_FromUnicode;

extern CODEPAGE __CP1257;
extern WCHAR  *__CP1257_ToUnicode;
extern CHAR  **__CP1257_FromUnicode;

extern CODEPAGE __CP1258;
extern WCHAR  *__CP1258_ToUnicode;
extern CHAR  **__CP1258_FromUnicode;

extern CODEPAGE __CP10000;
extern WCHAR  *__CP10000_ToUnicode;
extern CHAR  **__CP10000_FromUnicode;

extern CODEPAGE __CP10006;
extern WCHAR  *__CP10006_ToUnicode;
extern CHAR  **__CP10006_FromUnicode;

extern CODEPAGE __CP10007;
extern WCHAR  *__CP10007_ToUnicode;
extern CHAR  **__CP10007_FromUnicode;

extern CODEPAGE __CP10029;
extern WCHAR  *__CP10029_ToUnicode;
extern CHAR  **__CP10029_FromUnicode;

extern CODEPAGE __CP10079;
extern WCHAR  *__CP10079_ToUnicode;
extern CHAR  **__CP10079_FromUnicode;

extern CODEPAGE __CP10081;
extern WCHAR  *__CP10081_ToUnicode;
extern CHAR  **__CP10081_FromUnicode;

CPINFO __CPGenInfo={1, {'?',0}, { 0,0,0,0, 0,0,0,0, 0,0,0,0 }};

PCODEPAGE __CPFirst = &__CP37;

/* EBCDIC */
CODEPAGE __CP37 =
{
   &__CP437, 37, CODEPAGE_EBCDIC, &__CP37_ToUnicode, 
   &__CP37_ToUnicode, &__CP37_FromUnicode,
   &__CPGenInfo
};

/* MS-DOS U.S. */
CODEPAGE __CP437 =
{
   &__CP500, 437, CODEPAGE_OEM, &__CP437_ToUnicode, 
   &__CP437_ToUnicodeGlyph, &__CP437_FromUnicode,
   &__CPGenInfo
};

/* EBCDIC */
CODEPAGE __CP500 =
{
   &__CP737, 500, CODEPAGE_EBCDIC, &__CP500_ToUnicode, 
   &__CP500_ToUnicode, &__CP500_FromUnicode,
   &__CPGenInfo
};

/* MS-DOS Greek */
CODEPAGE __CP737 =
{
   &__CP775, 737, CODEPAGE_OEM, &__CP737_ToUnicode, 
   &__CP737_ToUnicodeGlyph, &__CP737_FromUnicode,
   &__CPGenInfo
};

/* MS-DOS Baltic Rim */
CODEPAGE __CP775 =
{
   &__CP850, 775, CODEPAGE_OEM, &__CP775_ToUnicode, 
   &__CP775_ToUnicodeGlyph, &__CP775_FromUnicode,
   &__CPGenInfo
};

/* MS-DOS Latin 1 */
CODEPAGE __CP850 =
{
   &__CP852, 850, CODEPAGE_OEM, &__CP850_ToUnicode, 
   &__CP850_ToUnicodeGlyph, &__CP850_FromUnicode,
   &__CPGenInfo
};

/* MS-DOS Latin 2 */
CODEPAGE __CP852 =
{
   &__CP855, 852, CODEPAGE_OEM, &__CP852_ToUnicode, 
   &__CP852_ToUnicodeGlyph, &__CP852_FromUnicode,
   &__CPGenInfo
};

/* MS-DOS Cyrillic */
CODEPAGE __CP855 =
{
   &__CP857, 855, CODEPAGE_OEM, &__CP855_ToUnicode, 
   &__CP855_ToUnicodeGlyph, &__CP855_FromUnicode,
   &__CPGenInfo
};

/* MS-DOS Turkish */
CODEPAGE __CP857 =
{
   &__CP860, 857, CODEPAGE_OEM, &__CP857_ToUnicode, 
   &__CP857_ToUnicodeGlyph, &__CP857_FromUnicode,
   &__CPGenInfo
};

/* MS-DOS Portugese */
CODEPAGE __CP860 =
{
   &__CP861, 860, CODEPAGE_OEM, &__CP860_ToUnicode, 
   &__CP860_ToUnicodeGlyph, &__CP860_FromUnicode,
   &__CPGenInfo
};

/* MS-DOS Icelandic */
CODEPAGE __CP861 =
{
   &__CP863, 861, CODEPAGE_OEM, &__CP861_ToUnicode, 
   &__CP861_ToUnicodeGlyph, &__CP861_FromUnicode,
   &__CPGenInfo
};

/* MS-DOS French Canada */
CODEPAGE __CP863 =
{
   &__CP865, 863, CODEPAGE_OEM, &__CP863_ToUnicode, 
   &__CP863_ToUnicodeGlyph, &__CP863_FromUnicode,
   &__CPGenInfo
};

/* MS-DOS Nordic */
CODEPAGE __CP865 =
{
   &__CP866, 865, CODEPAGE_OEM, &__CP865_ToUnicode, 
   &__CP865_ToUnicodeGlyph, &__CP865_FromUnicode,
   &__CPGenInfo
};

/* MS-DOS Cyrillic CIS-1 */
CODEPAGE __CP866 =
{
   &__CP869, 866, CODEPAGE_OEM, &__CP866_ToUnicode, 
   &__CP866_ToUnicodeGlyph, &__CP866_FromUnicode,
   &__CPGenInfo
};

/* MS-DOS Greek 2 */
CODEPAGE __CP869 =
{
   &__CP875, 869, CODEPAGE_OEM, &__CP869_ToUnicode, 
   &__CP869_ToUnicodeGlyph, &__CP869_FromUnicode,
   &__CPGenInfo
};

/* EBCDIC codepage */
CODEPAGE __CP875 =
{
   &__CP1026, 875, CODEPAGE_EBCDIC, &__CP875_ToUnicode, 
   &__CP875_ToUnicode, &__CP875_FromUnicode,
   &__CPGenInfo
};

/* EBCDIC codepage */
CODEPAGE __CP1026 =
{
   &__CP1250, 1026, CODEPAGE_EBCDIC, &__CP1026_ToUnicode, 
   &__CP1026_ToUnicode, &__CP1026_FromUnicode,
   &__CPGenInfo
};

/* Windows Latin 2 (Central Europe) */
CODEPAGE __CP1250 =
{
   &__CP1251, 1250, CODEPAGE_ANSI, &__CP1250_ToUnicode, 
   &__CP1250_ToUnicode, &__CP1250_FromUnicode,
   &__CPGenInfo
};

/* Windows Cyrillic */
CODEPAGE __CP1251 =
{
   &__CP1252, 1251, CODEPAGE_ANSI, &__CP1251_ToUnicode, 
   &__CP1251_ToUnicode, &__CP1251_FromUnicode,
   &__CPGenInfo
};

/* Windows Latin 1 */
CODEPAGE __CP1252 =
{
   &__CP1253, 1252, CODEPAGE_ANSI, &__CP1252_ToUnicode, 
   &__CP1252_ToUnicode, &__CP1252_FromUnicode,
   &__CPGenInfo
};

/* Windows Greek */
CODEPAGE __CP1253 =
{
   &__CP1254, 1253, CODEPAGE_ANSI, &__CP1253_ToUnicode, 
   &__CP1253_ToUnicode, &__CP1253_FromUnicode,
   &__CPGenInfo
};

/* Windows Latin 5 (Turkish) */
CODEPAGE __CP1254 =
{
  &__CP1255, 1254, CODEPAGE_ANSI, &__CP1254_ToUnicode, 
   &__CP1254_ToUnicode, &__CP1254_FromUnicode,
   &__CPGenInfo
};

/* Windows Hebrew */
CODEPAGE __CP1255 =
{
   &__CP1256, 1255, CODEPAGE_ANSI, &__CP1255_ToUnicode, 
   &__CP1255_ToUnicode, &__CP1255_FromUnicode,
   &__CPGenInfo
};

/* Windows Arabic */
CODEPAGE __CP1256 =
{
   &__CP1257, 1256, CODEPAGE_ANSI, &__CP1256_ToUnicode, 
   &__CP1256_ToUnicode, &__CP1256_FromUnicode,
   &__CPGenInfo
};

/* Windows Baltic Rim */
CODEPAGE __CP1257 =
{
   &__CP10000, 1257, CODEPAGE_ANSI, &__CP1257_ToUnicode, 
   &__CP1257_ToUnicode, &__CP1257_FromUnicode,
   &__CPGenInfo
};

/* Macintosh Roman */
CODEPAGE __CP10000 =
{
   &__CP10006, 10000, CODEPAGE_MAC, &__CP10000_ToUnicode, 
   &__CP10000_ToUnicode, &__CP10000_FromUnicode,
   &__CPGenInfo
};

/* Macintosh Greek 1 */
CODEPAGE __CP10006 =
{
   &__CP10007, 10006, CODEPAGE_MAC, &__CP10006_ToUnicode, 
   &__CP10006_ToUnicode, &__CP10006_FromUnicode,
   &__CPGenInfo
};

/* Macintosh Cyrillic */
CODEPAGE __CP10007 =
{
   &__CP10029, 10007, CODEPAGE_MAC, &__CP10007_ToUnicode, 
   &__CP10007_ToUnicode, &__CP10007_FromUnicode,
   &__CPGenInfo
};

/* Macintosh Latin 2 */
CODEPAGE __CP10029 =
{
   &__CP10079, 10029, CODEPAGE_MAC, &__CP10029_ToUnicode, 
   &__CP10029_ToUnicode, &__CP10029_FromUnicode,
   &__CPGenInfo
};

/* Macintosh Icelandic */
CODEPAGE __CP10079 =
{
   &__CP10081, 10079, CODEPAGE_MAC, &__CP10079_ToUnicode, 
   &__CP10079_ToUnicode, &__CP10079_FromUnicode,
   &__CPGenInfo
};

/* Macintosh Turkish */
CODEPAGE __CP10081 =
{
   NULL, 10081, CODEPAGE_MAC, &__CP10081_ToUnicode, 
   &__CP10081_ToUnicode, &__CP10081_FromUnicode,
   &__CPGenInfo
};
/*
 * Variant formatting functions
 *
 * Copyright 2008 Damjan Jovanovic
 * Copyright 2003 Jon Griffiths
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * NOTES
 *  Since the formatting functions aren't properly documented, I used the
 *  Visual Basic documentation as a guide to implementing these functions. This
 *  means that some named or user-defined formats may work slightly differently.
 *  Please submit a test case if you find a difference.
 */

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "variant.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(variant);

/* Make sure internal conversions to strings use the '.','+'/'-' and ','
 * format chars from the US locale. This enables us to parse the created
 * strings to determine the number of decimal places, exponent, etc.
 */
#define LCID_US MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT)

/******************************************************************************
 * Variant-Formats {OLEAUT32}
 *
 * NOTES
 *  When formatting a variant a variety of format strings may be used to generate
 *  different kinds of formatted output. A format string consists of either a named
 *  format, or a user-defined format.
 *
 *  The following named formats are defined:
 *| Name           Description
 *| ----           -----------
 *| General Date   Display Date, and time for non-integer values
 *| Short Date     Short date format as defined by locale settings
 *| Medium Date    Medium date format as defined by locale settings
 *| Long Date      Long date format as defined by locale settings
 *| Short Time     Short Time format as defined by locale settings
 *| Medium Time    Medium time format as defined by locale settings
 *| Long Time      Long time format as defined by locale settings
 *| True/False     Localised text of "True" or "False"
 *| Yes/No         Localised text of "Yes" or "No"
 *| On/Off         Localised text of "On" or "Off"
 *| General Number No thousands separator. No decimal points for integers
 *| Currency       General currency format using localised characters
 *| Fixed          At least one whole and two fractional digits
 *| Standard       Same as 'Fixed', but including decimal separators
 *| Percent        Multiply by 100 and display a trailing '%' character
 *| Scientific     Display with exponent
 *
 *  User-defined formats consist of a combination of tokens and literal
 *  characters. Literal characters are copied unmodified to the formatted
 *  output at the position they occupy in the format string. Any character
 *  that is not recognised as a token is treated as a literal. A literal can
 *  also be specified by preceding it with a backslash character
 *  (e.g. "\L\i\t\e\r\a\l") or enclosing it in double quotes.
 *
 *  A user-defined format can have up to 4 sections, depending on the type of
 *  format. The following table lists sections and their meaning:
 *| Format Type  Sections Meaning
 *| -----------  -------- -------
 *| Number       1        Use the same format for all numbers
 *| Number       2        Use format 1 for positive and 2 for negative numbers
 *| Number       3        Use format 1 for positive, 2 for zero, and 3
 *|                       for negative numbers.
 *| Number       4        Use format 1 for positive, 2 for zero, 3 for
 *|                       negative, and 4 for null numbers.
 *| String       1        Use the same format for all strings
 *| String       2        Use format 2 for null and empty strings, otherwise
 *|                       use format 1.
 *| Date         1        Use the same format for all dates
 *
 *  The formatting tokens fall into several categories depending on the type
 *  of formatted output. For more information on each type, see
 *  VarFormat-Dates(), VarFormat-Strings() and VarFormat-Numbers().
 *
 *  SEE ALSO
 *  VarTokenizeFormatString(), VarFormatFromTokens(), VarFormat(),
 *  VarFormatDateTime(), VarFormatNumber(), VarFormatCurrency().
 */

/******************************************************************************
 * VarFormat-Strings {OLEAUT32}
 *
 * NOTES
 *  When formatting a variant as a string, it is first converted to a VT_BSTR.
 *  The user-format string defines which characters are copied into which
 *  positions in the output string. Literals may be inserted in the format
 *  string. When creating the formatted string, excess characters in the string
 *  (those not consumed by a token) are appended to the end of the output. If
 *  there are more tokens than characters in the string to format, spaces will
 *  be inserted at the start of the string if the '@' token was used.
 *
 *  By default strings are converted to lowercase, or uppercase if the '>' token
 *  is encountered. This applies to the whole string: it is not possible to
 *  generate a mixed-case output string.
 *
 *  In user-defined string formats, the following tokens are recognised:
 *| Token  Description
 *| -----  -----------
 *| '@'    Copy a char from the source, or a space if no chars are left.
 *| '&'    Copy a char from the source, or write nothing if no chars are left.
 *| '<'    Output the whole string as lower-case (the default).
 *| '>'    Output the whole string as upper-case.
 *| '!'    MSDN indicates that this character should cause right-to-left
 *|        copying, however tests show that it is tokenised but not processed.
 */

/*
 * Common format definitions
 */

 /* Format types */
#define FMT_TYPE_UNKNOWN 0x0
#define FMT_TYPE_GENERAL 0x1
#define FMT_TYPE_NUMBER  0x2
#define FMT_TYPE_DATE    0x3
#define FMT_TYPE_STRING  0x4

#define FMT_TO_STRING    0x0 /* If header->size == this, act like VB's Str() fn */

typedef struct tagFMT_SHORT_HEADER
{
  BYTE   size;      /* Size of tokenised block (including header), or FMT_TO_STRING */
  BYTE   type;      /* Allowable types (FMT_TYPE_*) */
  BYTE   offset[1]; /* Offset of the first (and only) format section */
} FMT_SHORT_HEADER;

typedef struct tagFMT_HEADER
{
  BYTE   size;      /* Total size of the whole tokenised block (including header) */
  BYTE   type;      /* Allowable types (FMT_TYPE_*) */
  BYTE   starts[4]; /* Offset of each of the 4 format sections, or 0 if none */
} FMT_HEADER;

#define FmtGetPositive(x)  (x->starts[0])
#define FmtGetNegative(x)  (x->starts[1] ? x->starts[1] : x->starts[0])
#define FmtGetZero(x)      (x->starts[2] ? x->starts[2] : x->starts[0])
#define FmtGetNull(x)      (x->starts[3] ? x->starts[3] : x->starts[0])

/*
 * String formats
 */

#define FMT_FLAG_LT  0x1 /* Has '<' (lower case) */
#define FMT_FLAG_GT  0x2 /* Has '>' (upper case) */
#define FMT_FLAG_RTL 0x4 /* Has '!' (Copy right to left) */

typedef struct tagFMT_STRING_HEADER
{
  BYTE   flags;      /* LT, GT, RTL */
  BYTE   unknown1;
  BYTE   unknown2;
  BYTE   copy_chars; /* Number of chars to be copied */
  BYTE   unknown3;
} FMT_STRING_HEADER;

/*
 * Number formats
 */

#define FMT_FLAG_PERCENT   0x1  /* Has '%' (Percentage) */
#define FMT_FLAG_EXPONENT  0x2  /* Has 'e' (Exponent/Scientific notation) */
#define FMT_FLAG_THOUSANDS 0x4  /* Has ',' (Standard use of the thousands separator) */
#define FMT_FLAG_BOOL      0x20 /* Boolean format */

typedef struct tagFMT_NUMBER_HEADER
{
  BYTE   flags;      /* PERCENT, EXPONENT, THOUSANDS, BOOL */
  BYTE   multiplier; /* Multiplier, 100 for percentages */
  BYTE   divisor;    /* Divisor, 1000 if '%%' was used */
  BYTE   whole;      /* Number of digits before the decimal point */
  BYTE   fractional; /* Number of digits after the decimal point */
} FMT_NUMBER_HEADER;

/*
 * Date Formats
 */
typedef struct tagFMT_DATE_HEADER
{
  BYTE   flags;
  BYTE   unknown1;
  BYTE   unknown2;
  BYTE   unknown3;
  BYTE   unknown4;
} FMT_DATE_HEADER;

/*
 * Format token values
 */
#define FMT_GEN_COPY        0x00 /* \n, "lit" => 0,pos,len: Copy len chars from input+pos */
#define FMT_GEN_INLINE      0x01 /*      => 1,len,[chars]: Copy len chars from token stream */
#define FMT_GEN_END         0x02 /* \0,; => 2: End of the tokenised format */
#define FMT_DATE_TIME_SEP   0x03 /* Time separator char */
#define FMT_DATE_DATE_SEP   0x04 /* Date separator char */
#define FMT_DATE_GENERAL    0x05 /* General format date */
#define FMT_DATE_QUARTER    0x06 /* Quarter of the year from 1-4 */
#define FMT_DATE_TIME_SYS   0x07 /* System long time format */
#define FMT_DATE_DAY        0x08 /* Day with no leading 0 */
#define FMT_DATE_DAY_0      0x09 /* Day with leading 0 */
#define FMT_DATE_DAY_SHORT  0x0A /* Short day name */
#define FMT_DATE_DAY_LONG   0x0B /* Long day name */
#define FMT_DATE_SHORT      0x0C /* Short date format */
#define FMT_DATE_LONG       0x0D /* Long date format */
#define FMT_DATE_MEDIUM     0x0E /* Medium date format */
#define FMT_DATE_DAY_WEEK   0x0F /* First day of the week */
#define FMT_DATE_WEEK_YEAR  0x10 /* First week of the year */
#define FMT_DATE_MON        0x11 /* Month with no leading 0 */
#define FMT_DATE_MON_0      0x12 /* Month with leading 0 */
#define FMT_DATE_MON_SHORT  0x13 /* Short month name */
#define FMT_DATE_MON_LONG   0x14 /* Long month name */
#define FMT_DATE_YEAR_DOY   0x15 /* Day of the year with no leading 0 */
#define FMT_DATE_YEAR_0     0x16 /* 2 digit year with leading 0 */
/* NOTE: token 0x17 is not defined, 'yyy' is not valid */
#define FMT_DATE_YEAR_LONG  0x18 /* 4 digit year */
#define FMT_DATE_MIN        0x1A /* Minutes with no leading 0 */
#define FMT_DATE_MIN_0      0x1B /* Minutes with leading 0 */
#define FMT_DATE_SEC        0x1C /* Seconds with no leading 0 */
#define FMT_DATE_SEC_0      0x1D /* Seconds with leading 0 */
#define FMT_DATE_HOUR       0x1E /* Hours with no leading 0 */
#define FMT_DATE_HOUR_0     0x1F /* Hours with leading 0 */
#define FMT_DATE_HOUR_12    0x20 /* Hours with no leading 0, 12 hour clock */
#define FMT_DATE_HOUR_12_0  0x21 /* Hours with leading 0, 12 hour clock */
#define FMT_DATE_TIME_UNK2  0x23 /* same as FMT_DATE_HOUR_0, for "short time" format */
/* FIXME: probably missing some here */
#define FMT_DATE_AMPM_SYS1  0x2E /* AM/PM as defined by system settings */
#define FMT_DATE_AMPM_UPPER 0x2F /* Upper-case AM or PM */
#define FMT_DATE_A_UPPER    0x30 /* Upper-case A or P */
#define FMT_DATE_AMPM_SYS2  0x31 /* AM/PM as defined by system settings */
#define FMT_DATE_AMPM_LOWER 0x32 /* Lower-case AM or PM */
#define FMT_DATE_A_LOWER    0x33 /* Lower-case A or P */
#define FMT_NUM_COPY_ZERO   0x34 /* Copy 1 digit or 0 if no digit */
#define FMT_NUM_COPY_SKIP   0x35 /* Copy 1 digit or skip if no digit */
#define FMT_NUM_DECIMAL     0x36 /* Decimal separator */
#define FMT_NUM_EXP_POS_U   0x37 /* Scientific notation, uppercase, + sign */
#define FMT_NUM_EXP_NEG_U   0x38 /* Scientific notation, uppercase, - sign */
#define FMT_NUM_EXP_POS_L   0x39 /* Scientific notation, lowercase, + sign */
#define FMT_NUM_EXP_NEG_L   0x3A /* Scientific notation, lowercase, - sign */
#define FMT_NUM_CURRENCY    0x3B /* Currency symbol */
#define FMT_NUM_TRUE_FALSE  0x3D /* Convert to "True" or "False" */
#define FMT_NUM_YES_NO      0x3E /* Convert to "Yes" or "No" */
#define FMT_NUM_ON_OFF      0x3F /* Convert to "On" or "Off"  */
#define FMT_STR_COPY_SPACE  0x40 /* Copy len chars with space if no char */
#define FMT_STR_COPY_SKIP   0x41 /* Copy len chars or skip if no char */

/* Named Formats and their tokenised values */
static const BYTE fmtGeneralDate[0x0a] =
{
  0x0a,FMT_TYPE_DATE,sizeof(FMT_SHORT_HEADER),
  0x0,0x0,0x0,0x0,0x0,
  FMT_DATE_GENERAL,FMT_GEN_END
};

static const BYTE fmtShortDate[0x0a] =
{
  0x0a,FMT_TYPE_DATE,sizeof(FMT_SHORT_HEADER),
  0x0,0x0,0x0,0x0,0x0,
  FMT_DATE_SHORT,FMT_GEN_END
};

static const BYTE fmtMediumDate[0x0a] =
{
  0x0a,FMT_TYPE_DATE,sizeof(FMT_SHORT_HEADER),
  0x0,0x0,0x0,0x0,0x0,
  FMT_DATE_MEDIUM,FMT_GEN_END
};

static const BYTE fmtLongDate[0x0a] =
{
  0x0a,FMT_TYPE_DATE,sizeof(FMT_SHORT_HEADER),
  0x0,0x0,0x0,0x0,0x0,
  FMT_DATE_LONG,FMT_GEN_END
};

static const BYTE fmtShortTime[0x0c] =
{
  0x0c,FMT_TYPE_DATE,sizeof(FMT_SHORT_HEADER),
  0x0,0x0,0x0,0x0,0x0,
  FMT_DATE_TIME_UNK2,FMT_DATE_TIME_SEP,FMT_DATE_MIN_0,FMT_GEN_END
};

static const BYTE fmtMediumTime[0x11] =
{
  0x11,FMT_TYPE_DATE,sizeof(FMT_SHORT_HEADER),
  0x0,0x0,0x0,0x0,0x0,
  FMT_DATE_HOUR_12_0,FMT_DATE_TIME_SEP,FMT_DATE_MIN_0,
  FMT_GEN_INLINE,0x01,' ','\0',FMT_DATE_AMPM_SYS1,FMT_GEN_END
};

static const BYTE fmtLongTime[0x0d] =
{
  0x0a,FMT_TYPE_DATE,sizeof(FMT_SHORT_HEADER),
  0x0,0x0,0x0,0x0,0x0,
  FMT_DATE_TIME_SYS,FMT_GEN_END
};

static const BYTE fmtTrueFalse[0x0d] =
{
  0x0d,FMT_TYPE_NUMBER,sizeof(FMT_HEADER),0x0,0x0,0x0,
  FMT_FLAG_BOOL,0x0,0x0,0x0,0x0,
  FMT_NUM_TRUE_FALSE,FMT_GEN_END
};

static const BYTE fmtYesNo[0x0d] =
{
  0x0d,FMT_TYPE_NUMBER,sizeof(FMT_HEADER),0x0,0x0,0x0,
  FMT_FLAG_BOOL,0x0,0x0,0x0,0x0,
  FMT_NUM_YES_NO,FMT_GEN_END
};

static const BYTE fmtOnOff[0x0d] =
{
  0x0d,FMT_TYPE_NUMBER,sizeof(FMT_HEADER),0x0,0x0,0x0,
  FMT_FLAG_BOOL,0x0,0x0,0x0,0x0,
  FMT_NUM_ON_OFF,FMT_GEN_END
};

static const BYTE fmtGeneralNumber[sizeof(FMT_HEADER)] =
{
  sizeof(FMT_HEADER),FMT_TYPE_GENERAL,sizeof(FMT_HEADER),0x0,0x0,0x0
};

static const BYTE fmtCurrency[0x26] =
{
  0x26,FMT_TYPE_NUMBER,sizeof(FMT_HEADER),0x12,0x0,0x0,
  /* Positive numbers */
  FMT_FLAG_THOUSANDS,0xcc,0x0,0x1,0x2,
  FMT_NUM_CURRENCY,FMT_NUM_COPY_ZERO,0x1,FMT_NUM_DECIMAL,FMT_NUM_COPY_ZERO,0x2,
  FMT_GEN_END,
  /* Negative numbers */
  FMT_FLAG_THOUSANDS,0xcc,0x0,0x1,0x2,
  FMT_GEN_INLINE,0x1,'(','\0',FMT_NUM_CURRENCY,FMT_NUM_COPY_ZERO,0x1,
  FMT_NUM_DECIMAL,FMT_NUM_COPY_ZERO,0x2,FMT_GEN_INLINE,0x1,')','\0',
  FMT_GEN_END
};

static const BYTE fmtFixed[0x11] =
{
  0x11,FMT_TYPE_NUMBER,sizeof(FMT_HEADER),0x0,0x0,0x0,
  0x0,0x0,0x0,0x1,0x2,
  FMT_NUM_COPY_ZERO,0x1,FMT_NUM_DECIMAL,FMT_NUM_COPY_ZERO,0x2,FMT_GEN_END
};

static const BYTE fmtStandard[0x11] =
{
  0x11,FMT_TYPE_NUMBER,sizeof(FMT_HEADER),0x0,0x0,0x0,
  FMT_FLAG_THOUSANDS,0x0,0x0,0x1,0x2,
  FMT_NUM_COPY_ZERO,0x1,FMT_NUM_DECIMAL,FMT_NUM_COPY_ZERO,0x2,FMT_GEN_END
};

static const BYTE fmtPercent[0x15] =
{
  0x15,FMT_TYPE_NUMBER,sizeof(FMT_HEADER),0x0,0x0,0x0,
  FMT_FLAG_PERCENT,0x1,0x0,0x1,0x2,
  FMT_NUM_COPY_ZERO,0x1,FMT_NUM_DECIMAL,FMT_NUM_COPY_ZERO,0x2,
  FMT_GEN_INLINE,0x1,'%','\0',FMT_GEN_END
};

static const BYTE fmtScientific[0x13] =
{
  0x13,FMT_TYPE_NUMBER,sizeof(FMT_HEADER),0x0,0x0,0x0,
  FMT_FLAG_EXPONENT,0x0,0x0,0x1,0x2,
  FMT_NUM_COPY_ZERO,0x1,FMT_NUM_DECIMAL,FMT_NUM_COPY_ZERO,0x2,FMT_NUM_EXP_POS_U,0x2,FMT_GEN_END
};

typedef struct tagNAMED_FORMAT
{
  LPCWSTR name;
  const BYTE* format;
} NAMED_FORMAT;

/* Format name to tokenised format. Must be kept sorted by name */
static const NAMED_FORMAT VARIANT_NamedFormats[] =
{
  { L"Currency", fmtCurrency },
  { L"Fixed", fmtFixed },
  { L"General Date", fmtGeneralDate },
  { L"General Number", fmtGeneralNumber },
  { L"Long Date", fmtLongDate },
  { L"Long Time", fmtLongTime },
  { L"Medium Date", fmtMediumDate },
  { L"Medium Time", fmtMediumTime },
  { L"On/Off", fmtOnOff },
  { L"Percent", fmtPercent },
  { L"Scientific", fmtScientific },
  { L"Short Date", fmtShortDate },
  { L"Short Time", fmtShortTime },
  { L"Standard", fmtStandard },
  { L"True/False", fmtTrueFalse },
  { L"Yes/No", fmtYesNo }
};
typedef const NAMED_FORMAT *LPCNAMED_FORMAT;

static int __cdecl FormatCompareFn(const void *l, const void *r)
{
  return wcsicmp(((LPCNAMED_FORMAT)l)->name, ((LPCNAMED_FORMAT)r)->name);
}

static inline const BYTE *VARIANT_GetNamedFormat(LPCWSTR lpszFormat)
{
  NAMED_FORMAT key;
  LPCNAMED_FORMAT fmt;

  key.name = lpszFormat;
  fmt = bsearch(&key, VARIANT_NamedFormats, ARRAY_SIZE(VARIANT_NamedFormats),
                                 sizeof(NAMED_FORMAT), FormatCompareFn);
  return fmt ? fmt->format : NULL;
}

/* Return an error if the token for the value will not fit in the destination */
#define NEED_SPACE(x) if (cbTok < (int)(x)) return TYPE_E_BUFFERTOOSMALL; cbTok -= (x)

/* Non-zero if the format is unknown or a given type */
#define COULD_BE(typ) ((!fmt_number && header->type==FMT_TYPE_UNKNOWN)||header->type==typ)

/* State during tokenising */
#define FMT_STATE_OPEN_COPY     0x1 /* Last token written was a copy */
#define FMT_STATE_WROTE_DECIMAL 0x2 /* Already wrote a decimal separator */
#define FMT_STATE_SEEN_HOURS    0x4 /* See the hh specifier */
#define FMT_STATE_WROTE_MINUTES 0x8 /* Wrote minutes */

/**********************************************************************
 *              VarTokenizeFormatString [OLEAUT32.140]
 *
 * Convert a format string into tokenised form.
 *
 * PARAMS
 *  lpszFormat [I] Format string to tokenise
 *  rgbTok     [O] Destination for tokenised format
 *  cbTok      [I] Size of rgbTok in bytes
 *  nFirstDay  [I] First day of the week (1-7, or 0 for current system default)
 *  nFirstWeek [I] How to treat the first week (see notes)
 *  lcid       [I] Locale Id of the format string
 *  pcbActual  [O] If non-NULL, filled with the first token generated
 *
 * RETURNS
 *  Success: S_OK. rgbTok contains the tokenised format.
 *  Failure: E_INVALIDARG, if any argument is invalid.
 *           TYPE_E_BUFFERTOOSMALL, if rgbTok is not large enough.
 *
 * NOTES
 * Valid values for the nFirstWeek parameter are:
 *| Value  Meaning
 *| -----  -------
 *|   0    Use the current system default
 *|   1    The first week is that containing Jan 1
 *|   2    Four or more days of the first week are in the current year
 *|   3    The first week is 7 days long
 * See Variant-Formats(), VarFormatFromTokens().
 */
HRESULT WINAPI VarTokenizeFormatString(LPOLESTR lpszFormat, LPBYTE rgbTok,
                                       int cbTok, int nFirstDay, int nFirstWeek,
                                       LCID lcid, int *pcbActual)
{
  /* Note: none of these strings should be NUL terminated */
  static const WCHAR szTTTTT[] = { 't','t','t','t','t' };
  static const WCHAR szAMPM[] = { 'A','M','P','M' };
  static const WCHAR szampm[] = { 'a','m','p','m' };
  static const WCHAR szAMSlashPM[] = { 'A','M','/','P','M' };
  static const WCHAR szamSlashpm[] = { 'a','m','/','p','m' };
  const BYTE *namedFmt;
  FMT_HEADER *header = (FMT_HEADER*)rgbTok;
  FMT_STRING_HEADER *str_header = (FMT_STRING_HEADER*)(rgbTok + sizeof(FMT_HEADER));
  FMT_NUMBER_HEADER *num_header = (FMT_NUMBER_HEADER*)str_header;
  BYTE* pOut = rgbTok + sizeof(FMT_HEADER) + sizeof(FMT_STRING_HEADER);
  BYTE* pLastHours = NULL;
  BYTE fmt_number = 0;
  DWORD fmt_state = 0;
  LPCWSTR pFormat = lpszFormat;

  TRACE("%s, %p, %d, %d, %d, %#lx, %p.\n", debugstr_w(lpszFormat), rgbTok, cbTok,
        nFirstDay, nFirstWeek, lcid, pcbActual);

  if (!rgbTok ||
      nFirstDay < 0 || nFirstDay > 7 || nFirstWeek < 0 || nFirstWeek > 3)
    return E_INVALIDARG;

  if (!lpszFormat || !*lpszFormat)
  {
    /* An empty string means 'general format' */
    NEED_SPACE(sizeof(BYTE));
    *rgbTok = FMT_TO_STRING;
    if (pcbActual)
      *pcbActual = FMT_TO_STRING;
    return S_OK;
  }

  if (cbTok > 255)
    cbTok = 255; /* Ensure we error instead of wrapping */

  /* Named formats */
  namedFmt = VARIANT_GetNamedFormat(lpszFormat);
  if (namedFmt)
  {
    NEED_SPACE(namedFmt[0]);
    memcpy(rgbTok, namedFmt, namedFmt[0]);
    TRACE("Using pre-tokenised named format %s\n", debugstr_w(lpszFormat));
    /* FIXME: pcbActual */
    return S_OK;
  }

  /* Insert header */
  NEED_SPACE(sizeof(FMT_HEADER) + sizeof(FMT_STRING_HEADER));
  memset(header, 0, sizeof(FMT_HEADER));
  memset(str_header, 0, sizeof(FMT_STRING_HEADER));

  header->starts[fmt_number] = sizeof(FMT_HEADER);

  while (*pFormat)
  {
    /* --------------
     * General tokens
     * --------------
     */
    if (*pFormat == ';')
    {
      while (*pFormat == ';')
      {
        TRACE(";\n");
        if (++fmt_number > 3)
          return E_INVALIDARG; /* too many formats */
        pFormat++;
      }
      if (*pFormat)
      {
        TRACE("New header\n");
        NEED_SPACE(sizeof(BYTE) + sizeof(FMT_STRING_HEADER));
        *pOut++ = FMT_GEN_END;

        header->starts[fmt_number] = pOut - rgbTok;
        str_header = (FMT_STRING_HEADER*)pOut;
        num_header = (FMT_NUMBER_HEADER*)pOut;
        memset(str_header, 0, sizeof(FMT_STRING_HEADER));
        pOut += sizeof(FMT_STRING_HEADER);
        fmt_state = 0;
        pLastHours = NULL;
      }
    }
    else if (*pFormat == '\\')
    {
      /* Escaped character */
      if (pFormat[1])
      {
        NEED_SPACE(3 * sizeof(BYTE));
        pFormat++;
        *pOut++ = FMT_GEN_COPY;
        *pOut++ = pFormat - lpszFormat;
        *pOut++ = 0x1;
        fmt_state |= FMT_STATE_OPEN_COPY;
        TRACE("'\\'\n");
      }
      else
        fmt_state &= ~FMT_STATE_OPEN_COPY;
      pFormat++;
    }
    else if (*pFormat == '"')
    {
      /* Escaped string
       * Note: Native encodes "" as a copy of length zero. That's just dumb, so
       * here we avoid encoding anything in this case.
       */
      if (!pFormat[1])
        pFormat++;
      else if (pFormat[1] == '"')
      {
        pFormat += 2;
      }
      else
      {
        LPCWSTR start = ++pFormat;
        while (*pFormat && *pFormat != '"')
          pFormat++;
        NEED_SPACE(3 * sizeof(BYTE));
        *pOut++ = FMT_GEN_COPY;
        *pOut++ = start - lpszFormat;
        *pOut++ = pFormat - start;
        if (*pFormat == '"')
          pFormat++;
        TRACE("Quoted string pos %d, len %d\n", pOut[-2], pOut[-1]);
      }
      fmt_state &= ~FMT_STATE_OPEN_COPY;
    }
    /* -------------
     * Number tokens
     * -------------
     */
    else if (*pFormat == '0' && COULD_BE(FMT_TYPE_NUMBER))
    {
      /* Number formats: Digit from number or '0' if no digits
       * Other formats: Literal
       * Types the format if found
       */
      header->type = FMT_TYPE_NUMBER;
      NEED_SPACE(2 * sizeof(BYTE));
      *pOut++ = FMT_NUM_COPY_ZERO;
      *pOut = 0x0;
      while (*pFormat == '0')
      {
        *pOut = *pOut + 1;
        pFormat++;
      }
      if (fmt_state & FMT_STATE_WROTE_DECIMAL)
        num_header->fractional += *pOut;
      else
        num_header->whole += *pOut;
      TRACE("%d 0's\n", *pOut);
      pOut++;
      fmt_state &= ~FMT_STATE_OPEN_COPY;
    }
    else if (*pFormat == '#' && COULD_BE(FMT_TYPE_NUMBER))
    {
      /* Number formats: Digit from number or blank if no digits
       * Other formats: Literal
       * Types the format if found
       */
      header->type = FMT_TYPE_NUMBER;
      NEED_SPACE(2 * sizeof(BYTE));
      *pOut++ = FMT_NUM_COPY_SKIP;
      *pOut = 0x0;
      while (*pFormat == '#')
      {
        *pOut = *pOut + 1;
        pFormat++;
      }
      if (fmt_state & FMT_STATE_WROTE_DECIMAL)
        num_header->fractional += *pOut;
      else
        num_header->whole += *pOut;
      TRACE("%d #'s\n", *pOut);
      pOut++;
      fmt_state &= ~FMT_STATE_OPEN_COPY;
    }
    else if (*pFormat == '.' && COULD_BE(FMT_TYPE_NUMBER) &&
              !(fmt_state & FMT_STATE_WROTE_DECIMAL))
    {
      /* Number formats: Decimal separator when 1st seen, literal thereafter
       * Other formats: Literal
       * Types the format if found
       */
      header->type = FMT_TYPE_NUMBER;
      NEED_SPACE(sizeof(BYTE));
      *pOut++ = FMT_NUM_DECIMAL;
      fmt_state |= FMT_STATE_WROTE_DECIMAL;
      fmt_state &= ~FMT_STATE_OPEN_COPY;
      pFormat++;
      TRACE("decimal sep\n");
    }
    else if ((*pFormat == 'e' || *pFormat == 'E') && (pFormat[1] == '-' ||
              pFormat[1] == '+') && header->type == FMT_TYPE_NUMBER)
    {
      /* Number formats: Exponent specifier
       * Other formats: Literal
       */
      num_header->flags |= FMT_FLAG_EXPONENT;
      NEED_SPACE(2 * sizeof(BYTE));
      if (*pFormat == 'e') {
        if (pFormat[1] == '+')
          *pOut = FMT_NUM_EXP_POS_L;
        else
          *pOut = FMT_NUM_EXP_NEG_L;
      } else {
        if (pFormat[1] == '+')
          *pOut = FMT_NUM_EXP_POS_U;
        else
          *pOut = FMT_NUM_EXP_NEG_U;
      }
      pFormat += 2;
      *++pOut = 0x0;
      while (*pFormat == '0')
      {
        *pOut = *pOut + 1;
        pFormat++;
      }
      pOut++;
      TRACE("exponent\n");
    }
    /* FIXME: %% => Divide by 1000 */
    else if (*pFormat == ',' && header->type == FMT_TYPE_NUMBER)
    {
      /* Number formats: Use the thousands separator
       * Other formats: Literal
       */
      num_header->flags |= FMT_FLAG_THOUSANDS;
      pFormat++;
      fmt_state &= ~FMT_STATE_OPEN_COPY;
      TRACE("thousands sep\n");
    }
    /* -----------
     * Date tokens
     * -----------
     */
    else if (*pFormat == '/' && COULD_BE(FMT_TYPE_DATE))
    {
      /* Date formats: Date separator
       * Other formats: Literal
       * Types the format if found
       */
      header->type = FMT_TYPE_DATE;
      NEED_SPACE(sizeof(BYTE));
      *pOut++ = FMT_DATE_DATE_SEP;
      pFormat++;
      fmt_state &= ~FMT_STATE_OPEN_COPY;
      TRACE("date sep\n");
    }
    else if (*pFormat == ':' && COULD_BE(FMT_TYPE_DATE))
    {
      /* Date formats: Time separator
       * Other formats: Literal
       * Types the format if found
       */
      header->type = FMT_TYPE_DATE;
      NEED_SPACE(sizeof(BYTE));
      *pOut++ = FMT_DATE_TIME_SEP;
      pFormat++;
      fmt_state &= ~FMT_STATE_OPEN_COPY;
      TRACE("time sep\n");
    }
    else if ((*pFormat == 'a' || *pFormat == 'A') &&
              !wcsnicmp(pFormat, szAMPM, ARRAY_SIZE(szAMPM)))
    {
      /* Date formats: System AM/PM designation
       * Other formats: Literal
       * Types the format if found
       */
      header->type = FMT_TYPE_DATE;
      NEED_SPACE(sizeof(BYTE));
      pFormat += ARRAY_SIZE(szAMPM);
      if (!wcsncmp(pFormat, szampm, ARRAY_SIZE(szampm)))
        *pOut++ = FMT_DATE_AMPM_SYS2;
      else
        *pOut++ = FMT_DATE_AMPM_SYS1;
      if (pLastHours)
        *pLastHours = *pLastHours + 2;
      TRACE("ampm\n");
    }
    else if (*pFormat == 'a' && pFormat[1] == '/' &&
              (pFormat[2] == 'p' || pFormat[2] == 'P'))
    {
      /* Date formats: lowercase a or p designation
       * Other formats: Literal
       * Types the format if found
       */
      header->type = FMT_TYPE_DATE;
      NEED_SPACE(sizeof(BYTE));
      pFormat += 3;
      *pOut++ = FMT_DATE_A_LOWER;
      if (pLastHours)
        *pLastHours = *pLastHours + 2;
      TRACE("a/p\n");
    }
    else if (*pFormat == 'A' && pFormat[1] == '/' &&
              (pFormat[2] == 'p' || pFormat[2] == 'P'))
    {
      /* Date formats: Uppercase a or p designation
       * Other formats: Literal
       * Types the format if found
       */
      header->type = FMT_TYPE_DATE;
      NEED_SPACE(sizeof(BYTE));
      pFormat += 3;
      *pOut++ = FMT_DATE_A_UPPER;
      if (pLastHours)
        *pLastHours = *pLastHours + 2;
      TRACE("A/P\n");
    }
    else if (*pFormat == 'a' && !wcsncmp(pFormat, szamSlashpm, ARRAY_SIZE(szamSlashpm)))
    {
      /* Date formats: lowercase AM or PM designation
       * Other formats: Literal
       * Types the format if found
       */
      header->type = FMT_TYPE_DATE;
      NEED_SPACE(sizeof(BYTE));
      pFormat += ARRAY_SIZE(szamSlashpm);
      *pOut++ = FMT_DATE_AMPM_LOWER;
      if (pLastHours)
        *pLastHours = *pLastHours + 2;
      TRACE("AM/PM\n");
    }
    else if (*pFormat == 'A' && !wcsncmp(pFormat, szAMSlashPM, ARRAY_SIZE(szAMSlashPM)))
    {
      /* Date formats: Uppercase AM or PM designation
       * Other formats: Literal
       * Types the format if found
       */
      header->type = FMT_TYPE_DATE;
      NEED_SPACE(sizeof(BYTE));
      pFormat += ARRAY_SIZE(szAMSlashPM);
      *pOut++ = FMT_DATE_AMPM_UPPER;
      TRACE("AM/PM\n");
    }
    else if ((*pFormat == 'c' || *pFormat == 'C') && COULD_BE(FMT_TYPE_DATE))
    {
      /* Date formats: General date format
       * Other formats: Literal
       * Types the format if found
       */
      header->type = FMT_TYPE_DATE;
      NEED_SPACE(sizeof(BYTE));
      pFormat += ARRAY_SIZE(szAMSlashPM);
      *pOut++ = FMT_DATE_GENERAL;
      TRACE("gen date\n");
    }
    else if ((*pFormat == 'd' || *pFormat == 'D') && COULD_BE(FMT_TYPE_DATE))
    {
      /* Date formats: Day specifier
       * Other formats: Literal
       * Types the format if found
       */
      int count = -1;
      header->type = FMT_TYPE_DATE;
      while ((*pFormat == 'd' || *pFormat == 'D') && count < 6)
      {
        pFormat++;
        count++;
      }
      NEED_SPACE(sizeof(BYTE));
      *pOut++ = FMT_DATE_DAY + count;
      fmt_state &= ~FMT_STATE_OPEN_COPY;
      /* When we find the days token, reset the seen hours state so that
       * 'mm' is again written as month when encountered.
       */
      fmt_state &= ~FMT_STATE_SEEN_HOURS;
      TRACE("%d d's\n", count + 1);
    }
    else if ((*pFormat == 'h' || *pFormat == 'H') && COULD_BE(FMT_TYPE_DATE))
    {
      /* Date formats: Hour specifier
       * Other formats: Literal
       * Types the format if found
       */
      header->type = FMT_TYPE_DATE;
      NEED_SPACE(sizeof(BYTE));
      pFormat++;
      /* Record the position of the hours specifier - if we encounter
       * an am/pm specifier we will change the hours from 24 to 12.
       */
      pLastHours = pOut;
      if (*pFormat == 'h' || *pFormat == 'H')
      {
        pFormat++;
        *pOut++ = FMT_DATE_HOUR_0;
        TRACE("hh\n");
      }
      else
      {
        *pOut++ = FMT_DATE_HOUR;
        TRACE("h\n");
      }
      fmt_state &= ~FMT_STATE_OPEN_COPY;
      /* Note that now we have seen an hours token, the next occurrence of
       * 'mm' indicates minutes, not months.
       */
      fmt_state |= FMT_STATE_SEEN_HOURS;
    }
    else if ((*pFormat == 'm' || *pFormat == 'M') && COULD_BE(FMT_TYPE_DATE))
    {
      /* Date formats: Month specifier (or Minute specifier, after hour specifier)
       * Other formats: Literal
       * Types the format if found
       */
      int count = -1;
      header->type = FMT_TYPE_DATE;
      while ((*pFormat == 'm' || *pFormat == 'M') && count < 4)
      {
        pFormat++;
        count++;
      }
      NEED_SPACE(sizeof(BYTE));
      if (count <= 1 && fmt_state & FMT_STATE_SEEN_HOURS &&
          !(fmt_state & FMT_STATE_WROTE_MINUTES))
      {
        /* We have seen an hours specifier and not yet written a minutes
         * specifier. Write this as minutes and thereafter as months.
         */
        *pOut++ = count == 1 ? FMT_DATE_MIN_0 : FMT_DATE_MIN;
        fmt_state |= FMT_STATE_WROTE_MINUTES; /* Hereafter write months */
      }
      else
        *pOut++ = FMT_DATE_MON + count; /* Months */
      fmt_state &= ~FMT_STATE_OPEN_COPY;
      TRACE("%d m's\n", count + 1);
    }
    else if ((*pFormat == 'n' || *pFormat == 'N') && COULD_BE(FMT_TYPE_DATE))
    {
      /* Date formats: Minute specifier
       * Other formats: Literal
       * Types the format if found
       */
      header->type = FMT_TYPE_DATE;
      NEED_SPACE(sizeof(BYTE));
      pFormat++;
      if (*pFormat == 'n' || *pFormat == 'N')
      {
        pFormat++;
        *pOut++ = FMT_DATE_MIN_0;
        TRACE("nn\n");
      }
      else
      {
        *pOut++ = FMT_DATE_MIN;
        TRACE("n\n");
      }
      fmt_state &= ~FMT_STATE_OPEN_COPY;
    }
    else if ((*pFormat == 'q' || *pFormat == 'Q') && COULD_BE(FMT_TYPE_DATE))
    {
      /* Date formats: Quarter specifier
       * Other formats: Literal
       * Types the format if found
       */
      header->type = FMT_TYPE_DATE;
      NEED_SPACE(sizeof(BYTE));
      *pOut++ = FMT_DATE_QUARTER;
      pFormat++;
      fmt_state &= ~FMT_STATE_OPEN_COPY;
      TRACE("quarter\n");
    }
    else if ((*pFormat == 's' || *pFormat == 'S') && COULD_BE(FMT_TYPE_DATE))
    {
      /* Date formats: Second specifier
       * Other formats: Literal
       * Types the format if found
       */
      header->type = FMT_TYPE_DATE;
      NEED_SPACE(sizeof(BYTE));
      pFormat++;
      if (*pFormat == 's' || *pFormat == 'S')
      {
        pFormat++;
        *pOut++ = FMT_DATE_SEC_0;
        TRACE("ss\n");
      }
      else
      {
        *pOut++ = FMT_DATE_SEC;
        TRACE("s\n");
      }
      fmt_state &= ~FMT_STATE_OPEN_COPY;
    }
    else if ((*pFormat == 't' || *pFormat == 'T') &&
              !wcsnicmp(pFormat, szTTTTT, ARRAY_SIZE(szTTTTT)))
    {
      /* Date formats: System time specifier
       * Other formats: Literal
       * Types the format if found
       */
      header->type = FMT_TYPE_DATE;
      pFormat += ARRAY_SIZE(szTTTTT);
      NEED_SPACE(sizeof(BYTE));
      *pOut++ = FMT_DATE_TIME_SYS;
      fmt_state &= ~FMT_STATE_OPEN_COPY;
    }
    else if ((*pFormat == 'w' || *pFormat == 'W') && COULD_BE(FMT_TYPE_DATE))
    {
      /* Date formats: Week of the year/Day of the week
       * Other formats: Literal
       * Types the format if found
       */
      header->type = FMT_TYPE_DATE;
      pFormat++;
      if (*pFormat == 'w' || *pFormat == 'W')
      {
        NEED_SPACE(3 * sizeof(BYTE));
        pFormat++;
        *pOut++ = FMT_DATE_WEEK_YEAR;
        *pOut++ = nFirstDay;
        *pOut++ = nFirstWeek;
        TRACE("ww\n");
      }
      else
      {
        NEED_SPACE(2 * sizeof(BYTE));
        *pOut++ = FMT_DATE_DAY_WEEK;
        *pOut++ = nFirstDay;
        TRACE("w\n");
      }

      fmt_state &= ~FMT_STATE_OPEN_COPY;
    }
    else if ((*pFormat == 'y' || *pFormat == 'Y') && COULD_BE(FMT_TYPE_DATE))
    {
      /* Date formats: Day of year/Year specifier
       * Other formats: Literal
       * Types the format if found
       */
      int count = -1;
      header->type = FMT_TYPE_DATE;
      while ((*pFormat == 'y' || *pFormat == 'Y') && count < 4)
      {
        pFormat++;
        count++;
      }
      if (count == 2)
      {
        count--; /* 'yyy' has no meaning, despite what MSDN says */
        pFormat--;
      }
      NEED_SPACE(sizeof(BYTE));
      *pOut++ = FMT_DATE_YEAR_DOY + count;
      fmt_state &= ~FMT_STATE_OPEN_COPY;
      TRACE("%d y's\n", count + 1);
    }
    /* -------------
     * String tokens
     * -------------
     */
    else if (*pFormat == '@' && COULD_BE(FMT_TYPE_STRING))
    {
      /* String formats: Character from string or space if no char
       * Other formats: Literal
       * Types the format if found
       */
      header->type = FMT_TYPE_STRING;
      NEED_SPACE(2 * sizeof(BYTE));
      *pOut++ = FMT_STR_COPY_SPACE;
      *pOut = 0x0;
      while (*pFormat == '@')
      {
        *pOut = *pOut + 1;
        str_header->copy_chars++;
        pFormat++;
      }
      TRACE("%d @'s\n", *pOut);
      pOut++;
      fmt_state &= ~FMT_STATE_OPEN_COPY;
    }
    else if (*pFormat == '&' && COULD_BE(FMT_TYPE_STRING))
    {
      /* String formats: Character from string or skip if no char
       * Other formats: Literal
       * Types the format if found
       */
      header->type = FMT_TYPE_STRING;
      NEED_SPACE(2 * sizeof(BYTE));
      *pOut++ = FMT_STR_COPY_SKIP;
      *pOut = 0x0;
      while (*pFormat == '&')
      {
        *pOut = *pOut + 1;
        str_header->copy_chars++;
        pFormat++;
      }
      TRACE("%d &'s\n", *pOut);
      pOut++;
      fmt_state &= ~FMT_STATE_OPEN_COPY;
    }
    else if ((*pFormat == '<' || *pFormat == '>') && COULD_BE(FMT_TYPE_STRING))
    {
      /* String formats: Use upper/lower case
       * Other formats: Literal
       * Types the format if found
       */
      header->type = FMT_TYPE_STRING;
      if (*pFormat == '<')
        str_header->flags |= FMT_FLAG_LT;
      else
        str_header->flags |= FMT_FLAG_GT;
      TRACE("to %s case\n", *pFormat == '<' ? "lower" : "upper");
      pFormat++;
      fmt_state &= ~FMT_STATE_OPEN_COPY;
    }
    else if (*pFormat == '!' && COULD_BE(FMT_TYPE_STRING))
    {
      /* String formats: Copy right to left
       * Other formats: Literal
       * Types the format if found
       */
      header->type = FMT_TYPE_STRING;
      str_header->flags |= FMT_FLAG_RTL;
      pFormat++;
      fmt_state &= ~FMT_STATE_OPEN_COPY;
      TRACE("copy right-to-left\n");
    }
    /* --------
     * Literals
     * --------
     */
    /* FIXME: [ seems to be ignored */
    else
    {
      if (*pFormat == '%' && header->type == FMT_TYPE_NUMBER)
      {
        /* Number formats: Percentage indicator, also a literal
         * Other formats: Literal
         * Doesn't type the format
         */
        num_header->flags |= FMT_FLAG_PERCENT;
      }

      if (fmt_state & FMT_STATE_OPEN_COPY)
      {
        pOut[-1] = pOut[-1] + 1; /* Increase the length of the open copy */
        TRACE("extend copy (char '%c'), length now %d\n", *pFormat, pOut[-1]);
      }
      else
      {
        /* Create a new open copy */
        TRACE("New copy (char '%c')\n", *pFormat);
        NEED_SPACE(3 * sizeof(BYTE));
        *pOut++ = FMT_GEN_COPY;
        *pOut++ = pFormat - lpszFormat;
        *pOut++ = 0x1;
        fmt_state |= FMT_STATE_OPEN_COPY;
      }
      pFormat++;
    }
  }

  *pOut++ = FMT_GEN_END;

  header->size = pOut - rgbTok;
  if (pcbActual)
    *pcbActual = header->size;

  return S_OK;
}

/* Number formatting state flags */
#define NUM_WROTE_DEC  0x01 /* Written the decimal separator */
#define NUM_WRITE_ON   0x02 /* Started to write the number */
#define NUM_WROTE_SIGN 0x04 /* Written the negative sign */

/* Format a variant using a number format */
static HRESULT VARIANT_FormatNumber(LPVARIANT pVarIn, LPOLESTR lpszFormat,
                                    LPBYTE rgbTok, ULONG dwFlags,
                                    BSTR *pbstrOut, LCID lcid)
{
  BYTE rgbDig[256], *prgbDig;
  NUMPARSE np;
  int have_int, need_int = 0, have_frac, need_frac, exponent = 0, pad = 0;
  WCHAR buff[256], *pBuff = buff;
  WCHAR thousandSeparator[32];
  VARIANT vString, vBool;
  DWORD dwState = 0;
  FMT_HEADER *header = (FMT_HEADER*)rgbTok;
  FMT_NUMBER_HEADER *numHeader;
  const BYTE* pToken = NULL;
  HRESULT hRes = S_OK;

  TRACE("%s, %s, %p, %#lx, %p, %#lx.\n", debugstr_variant(pVarIn), debugstr_w(lpszFormat),
        rgbTok, dwFlags, pbstrOut, lcid);

  V_VT(&vString) = VT_EMPTY;
  V_VT(&vBool) = VT_BOOL;

  if (V_TYPE(pVarIn) == VT_EMPTY || V_TYPE(pVarIn) == VT_NULL)
  {
    have_int = have_frac = 0;
    numHeader = (FMT_NUMBER_HEADER*)(rgbTok + FmtGetNull(header));
    V_BOOL(&vBool) = VARIANT_FALSE;
  }
  else
  {
    /* Get a number string from pVarIn, and parse it */
    hRes = VariantChangeTypeEx(&vString, pVarIn, lcid, VARIANT_NOUSEROVERRIDE, VT_BSTR);
    if (FAILED(hRes))
      return hRes;

    np.cDig = sizeof(rgbDig);
    np.dwInFlags = NUMPRS_STD;
    hRes = VarParseNumFromStr(V_BSTR(&vString), lcid, 0, &np, rgbDig);
    if (FAILED(hRes))
      return hRes;

    have_int = np.cDig;
    have_frac = 0;
    exponent = np.nPwr10;

    /* Figure out which format to use */
    if (np.dwOutFlags & NUMPRS_NEG)
    {
      numHeader = (FMT_NUMBER_HEADER*)(rgbTok + FmtGetNegative(header));
      V_BOOL(&vBool) = VARIANT_TRUE;
    }
    else if (have_int == 1 && !exponent && rgbDig[0] == 0)
    {
      numHeader = (FMT_NUMBER_HEADER*)(rgbTok + FmtGetZero(header));
      V_BOOL(&vBool) = VARIANT_FALSE;
    }
    else
    {
      numHeader = (FMT_NUMBER_HEADER*)(rgbTok + FmtGetPositive(header));
      V_BOOL(&vBool) = VARIANT_TRUE;
    }

    TRACE("num header: flags = 0x%x, mult=%d, div=%d, whole=%d, fract=%d\n",
          numHeader->flags, numHeader->multiplier, numHeader->divisor,
          numHeader->whole, numHeader->fractional);

    need_int = numHeader->whole;
    need_frac = numHeader->fractional;

    if (numHeader->flags & FMT_FLAG_PERCENT &&
        !(have_int == 1 && !exponent && rgbDig[0] == 0))
      exponent += 2;

    if (numHeader->flags & FMT_FLAG_EXPONENT)
    {
      /* Exponent format: length of the integral number part is fixed and
         specified by the format. */
      pad = need_int - have_int;
      exponent -= pad;
      if (pad < 0)
      {
        have_int = need_int;
        have_frac -= pad;
        pad = 0;
      }
    }
    else
    {
      /* Convert the exponent */
      pad = max(exponent, -have_int);
      exponent -= pad;
      if (pad < 0)
      {
        have_int += pad;
        have_frac = -pad;
        pad = 0;
      }
      if(exponent < 0 && exponent > (-256 + have_int + have_frac))
      {
        /* Remove exponent notation */
        memmove(rgbDig - exponent, rgbDig, have_int + have_frac);
        ZeroMemory(rgbDig, -exponent);
        have_frac -= exponent;
        exponent = 0;
      }
    }

    /* Rounding the number */
    if (have_frac > need_frac)
    {
      prgbDig = &rgbDig[have_int + need_frac];
      have_frac = need_frac;
      if (*prgbDig >= 5)
      {
        while (prgbDig-- > rgbDig && *prgbDig == 9)
          *prgbDig = 0;
        if (prgbDig < rgbDig)
        {
          /* We reached the first digit and that was also a 9 */
          rgbDig[0] = 1;
          if (numHeader->flags & FMT_FLAG_EXPONENT)
            exponent++;
          else
          {
            rgbDig[have_int + need_frac] = 0;
            if (exponent < 0)
              exponent++;
            else
              have_int++;
          }
        }
        else
          (*prgbDig)++;
      }
      /* We converted trailing digits to zeroes => have_frac has changed */
      while (have_frac > 0 && rgbDig[have_int + have_frac - 1] == 0)
        have_frac--;
    }
    TRACE("have_int=%d,need_int=%d,have_frac=%d,need_frac=%d,pad=%d,exp=%d\n",
          have_int, need_int, have_frac, need_frac, pad, exponent);
  }

  if (numHeader->flags & FMT_FLAG_THOUSANDS)
  {
    if (!GetLocaleInfoW(lcid, LOCALE_STHOUSAND, thousandSeparator, ARRAY_SIZE(thousandSeparator)))
    {
      thousandSeparator[0] = ',';
      thousandSeparator[1] = 0;
    }
  }

  pToken = (const BYTE*)numHeader + sizeof(FMT_NUMBER_HEADER);
  prgbDig = rgbDig;

  while (SUCCEEDED(hRes) && *pToken != FMT_GEN_END)
  {
    WCHAR defaultChar = '?';
    DWORD boolFlag, localeValue = 0;
    BOOL shouldAdvance = TRUE;

    if (pToken - rgbTok > header->size)
    {
      ERR("Ran off the end of the format!\n");
      hRes = E_INVALIDARG;
      goto VARIANT_FormatNumber_Exit;
    }

    switch (*pToken)
    {
    case FMT_GEN_COPY:
      TRACE("copy %s\n", debugstr_wn(lpszFormat + pToken[1], pToken[2]));
      memcpy(pBuff, lpszFormat + pToken[1], pToken[2] * sizeof(WCHAR));
      pBuff += pToken[2];
      pToken += 2;
      break;

    case FMT_GEN_INLINE:
      pToken += 2;
      TRACE("copy %s\n", debugstr_a((LPCSTR)pToken));
      while (*pToken)
        *pBuff++ = *pToken++;
      break;

    case FMT_NUM_YES_NO:
      boolFlag = VAR_BOOLYESNO;
      goto VARIANT_FormatNumber_Bool;

    case FMT_NUM_ON_OFF:
      boolFlag = VAR_BOOLONOFF;
      goto VARIANT_FormatNumber_Bool;

    case FMT_NUM_TRUE_FALSE:
      boolFlag = VAR_LOCALBOOL;

VARIANT_FormatNumber_Bool:
      {
        BSTR boolStr = NULL;

        if (pToken[1] != FMT_GEN_END)
        {
          ERR("Boolean token not at end of format!\n");
          hRes = E_INVALIDARG;
          goto VARIANT_FormatNumber_Exit;
        }
        hRes = VarBstrFromBool(V_BOOL(&vBool), lcid, boolFlag, &boolStr);
        if (SUCCEEDED(hRes))
        {
          lstrcpyW(pBuff, boolStr);
          SysFreeString(boolStr);
          while (*pBuff)
            pBuff++;
        }
      }
      break;

    case FMT_NUM_DECIMAL:
      if ((np.dwOutFlags & NUMPRS_NEG) && !(dwState & NUM_WROTE_SIGN) && !header->starts[1])
      {
        /* last chance for a negative sign in the .# case */
        TRACE("write negative sign\n");
        localeValue = LOCALE_SNEGATIVESIGN;
        defaultChar = '-';
        dwState |= NUM_WROTE_SIGN;
        shouldAdvance = FALSE;
        break;
      }
      TRACE("write decimal separator\n");
      localeValue = LOCALE_SDECIMAL;
      defaultChar = '.';
      dwState |= NUM_WROTE_DEC;
      break;

    case FMT_NUM_CURRENCY:
      TRACE("write currency symbol\n");
      localeValue = LOCALE_SCURRENCY;
      defaultChar = '$';
      break;

    case FMT_NUM_EXP_POS_U:
    case FMT_NUM_EXP_POS_L:
    case FMT_NUM_EXP_NEG_U:
    case FMT_NUM_EXP_NEG_L:
      if (*pToken == FMT_NUM_EXP_POS_L || *pToken == FMT_NUM_EXP_NEG_L)
        *pBuff++ = 'e';
      else
        *pBuff++ = 'E';
      if (exponent < 0)
      {
        *pBuff++ = '-';
        swprintf(pBuff, ARRAY_SIZE(buff) - (pBuff - buff), L"%0*d", pToken[1], -exponent);
      }
      else
      {
        if (*pToken == FMT_NUM_EXP_POS_L || *pToken == FMT_NUM_EXP_POS_U)
          *pBuff++ = '+';
        swprintf(pBuff, ARRAY_SIZE(buff) - (pBuff - buff), L"%0*d", pToken[1], exponent);
      }
      while (*pBuff)
        pBuff++;
      pToken++;
      break;

    case FMT_NUM_COPY_ZERO:
      dwState |= NUM_WRITE_ON;
      /* Fall through */

    case FMT_NUM_COPY_SKIP:
      TRACE("write %d %sdigits or %s\n", pToken[1],
            dwState & NUM_WROTE_DEC ? "fractional " : "",
            *pToken == FMT_NUM_COPY_ZERO ? "0" : "skip");

      if (dwState & NUM_WROTE_DEC)
      {
        int count, i;

        if (!(numHeader->flags & FMT_FLAG_EXPONENT) && exponent < 0)
        {
          /* Pad with 0 before writing the fractional digits */
          pad = max(exponent, -pToken[1]);
          exponent -= pad;
          count = min(have_frac, pToken[1] + pad);
          for (i = 0; i > pad; i--)
            *pBuff++ = '0';
        }
        else
          count = min(have_frac, pToken[1]);

        pad += pToken[1] - count;
        have_frac -= count;
        while (count--)
          *pBuff++ = '0' + *prgbDig++;
        if (*pToken == FMT_NUM_COPY_ZERO)
        {
          for (; pad > 0; pad--)
            *pBuff++ = '0'; /* Write zeros for missing trailing digits */
        }
      }
      else
      {
        int count, count_max, position;

        if ((np.dwOutFlags & NUMPRS_NEG) && !(dwState & NUM_WROTE_SIGN) && !header->starts[1])
        {
          TRACE("write negative sign\n");
          localeValue = LOCALE_SNEGATIVESIGN;
          defaultChar = '-';
          dwState |= NUM_WROTE_SIGN;
          shouldAdvance = FALSE;
          break;
        }

        position = have_int + pad;
        if (dwState & NUM_WRITE_ON)
          position = max(position, need_int);
        need_int -= pToken[1];
        count_max = have_int + pad - need_int;
        if (count_max < 0)
            count_max = 0;
        if (dwState & NUM_WRITE_ON)
        {
          count = pToken[1] - count_max;
          TRACE("write %d leading zeros\n", count);
          while (count-- > 0)
          {
            *pBuff++ = '0';
            if ((numHeader->flags & FMT_FLAG_THOUSANDS) &&
                position > 1 && (--position % 3) == 0)
            {
              int k;
              TRACE("write thousand separator\n");
              for (k = 0; thousandSeparator[k]; k++)
                *pBuff++ = thousandSeparator[k];
            }
          }
        }
        if (*pToken == FMT_NUM_COPY_ZERO || have_int > 1 ||
            (have_int > 0 && *prgbDig > 0))
        {
          count = min(count_max, have_int);
          count_max -= count;
          have_int -= count;
          TRACE("write %d whole number digits\n", count);
          while (count--)
          {
            dwState |= NUM_WRITE_ON;
            *pBuff++ = '0' + *prgbDig++;
            if ((numHeader->flags & FMT_FLAG_THOUSANDS) &&
                position > 1 && (--position % 3) == 0)
            {
              int k;
              TRACE("write thousand separator\n");
              for (k = 0; thousandSeparator[k]; k++)
                *pBuff++ = thousandSeparator[k];
            }
          }
        }
        count = min(count_max, pad);
        pad -= count;
        TRACE("write %d whole trailing 0's\n", count);
        while (count--)
        {
          *pBuff++ = '0';
          if ((numHeader->flags & FMT_FLAG_THOUSANDS) &&
              position > 1 && (--position % 3) == 0)
          {
            int k;
            TRACE("write thousand separator\n");
            for (k = 0; thousandSeparator[k]; k++)
              *pBuff++ = thousandSeparator[k];
          }
        }
      }
      pToken++;
      break;

    default:
      ERR("Unknown token 0x%02x!\n", *pToken);
      hRes = E_INVALIDARG;
      goto VARIANT_FormatNumber_Exit;
    }
    if (localeValue)
    {
      if (GetLocaleInfoW(lcid, localeValue, pBuff, ARRAY_SIZE(buff)-(pBuff-buff)))
      {
        TRACE("added %s\n", debugstr_w(pBuff));
        while (*pBuff)
          pBuff++;
      }
      else
      {
        TRACE("added %d '%c'\n", defaultChar, defaultChar);
        *pBuff++ = defaultChar;
      }
    }
    if (shouldAdvance)
      pToken++;
  }

VARIANT_FormatNumber_Exit:
  VariantClear(&vString);
  *pBuff = '\0';
  TRACE("buff is %s\n", debugstr_w(buff));
  if (SUCCEEDED(hRes))
  {
    *pbstrOut = SysAllocString(buff);
    if (!*pbstrOut)
      hRes = E_OUTOFMEMORY;
  }
  return hRes;
}

/* Format a variant using a date format */
static HRESULT VARIANT_FormatDate(LPVARIANT pVarIn, LPOLESTR lpszFormat,
                                  LPBYTE rgbTok, ULONG dwFlags,
                                  BSTR *pbstrOut, LCID lcid)
{
  WCHAR buff[256], *pBuff = buff;
  VARIANT vDate;
  UDATE udate;
  FMT_HEADER *header = (FMT_HEADER*)rgbTok;
  FMT_DATE_HEADER *dateHeader;
  const BYTE* pToken = NULL;
  HRESULT hRes;

  TRACE("%s, %s, %p, %#lx, %p, %#lx.\n", debugstr_variant(pVarIn),
        debugstr_w(lpszFormat), rgbTok, dwFlags, pbstrOut, lcid);

  V_VT(&vDate) = VT_EMPTY;

  if (V_TYPE(pVarIn) == VT_EMPTY || V_TYPE(pVarIn) == VT_NULL)
  {
    dateHeader = (FMT_DATE_HEADER*)(rgbTok + FmtGetNegative(header));
    V_DATE(&vDate) = 0;
  }
  else
  {
    USHORT usFlags = dwFlags & VARIANT_CALENDAR_HIJRI ? VAR_CALENDAR_HIJRI : 0;

    hRes = VariantChangeTypeEx(&vDate, pVarIn, lcid, usFlags, VT_DATE);
    /* 31809.40 and similar are treated as invalid by coercion functions but
     * it simply is a DATE in string form as far as VarFormat is concerned
     */
    if (FAILED(hRes))
    {
      if (V_TYPE(pVarIn) == VT_BSTR)
      {
        DATE out;
        OLECHAR *endptr = NULL;
	/* Try consume the string with wcstod */
	double tmp = wcstod(V_BSTR(pVarIn), &endptr);

	/* Not a double in string form */
	if (*endptr)
          return hRes;

	hRes = VarDateFromR8(tmp, &out);

	if (FAILED(hRes))
	  return hRes;

	V_VT(&vDate) = VT_DATE;
	V_DATE(&vDate) = out;
      }
      else
	return hRes;
    }

    dateHeader = (FMT_DATE_HEADER*)(rgbTok + FmtGetPositive(header));
  }

  hRes = VarUdateFromDate(V_DATE(&vDate), 0 /* FIXME: flags? */, &udate);
  if (FAILED(hRes))
    return hRes;
  pToken = (const BYTE*)dateHeader + sizeof(FMT_DATE_HEADER);

  while (*pToken != FMT_GEN_END)
  {
    DWORD dwVal = 0, localeValue = 0, dwFmt = 0;
    LPCWSTR szPrintFmt = NULL;
    WCHAR defaultChar = '?';

    if (pToken - rgbTok > header->size)
    {
      ERR("Ran off the end of the format!\n");
      hRes = E_INVALIDARG;
      goto VARIANT_FormatDate_Exit;
    }

    switch (*pToken)
    {
    case FMT_GEN_COPY:
      TRACE("copy %s\n", debugstr_wn(lpszFormat + pToken[1], pToken[2]));
      memcpy(pBuff, lpszFormat + pToken[1], pToken[2] * sizeof(WCHAR));
      pBuff += pToken[2];
      pToken += 2;
      break;

    case FMT_GEN_INLINE:
      pToken += 2;
      TRACE("copy %s\n", debugstr_a((LPCSTR)pToken));
      while (*pToken)
        *pBuff++ = *pToken++;
      break;

    case FMT_DATE_TIME_SEP:
      TRACE("time separator\n");
      localeValue = LOCALE_STIME;
      defaultChar = ':';
      break;

    case FMT_DATE_DATE_SEP:
      TRACE("date separator\n");
      localeValue = LOCALE_SDATE;
      defaultChar = '/';
      break;

    case FMT_DATE_GENERAL:
      {
        BSTR date = NULL;
        WCHAR *pDate;
        hRes = VarBstrFromDate(V_DATE(&vDate), lcid, 0, &date);
        if (FAILED(hRes))
          goto VARIANT_FormatDate_Exit;
	pDate = date;
        while (*pDate)
          *pBuff++ = *pDate++;
        SysFreeString(date);
      }
      break;

    case FMT_DATE_QUARTER:
      if (udate.st.wMonth <= 3)
        *pBuff++ = '1';
      else if (udate.st.wMonth <= 6)
        *pBuff++ = '2';
      else if (udate.st.wMonth <= 9)
        *pBuff++ = '3';
      else
        *pBuff++ = '4';
      break;

    case FMT_DATE_TIME_SYS:
      {
        /* FIXME: VARIANT_CALENDAR HIJRI should cause Hijri output */
        BSTR date = NULL;
        WCHAR *pDate;
        hRes = VarBstrFromDate(V_DATE(&vDate), lcid, VAR_TIMEVALUEONLY, &date);
        if (FAILED(hRes))
          goto VARIANT_FormatDate_Exit;
	pDate = date;
        while (*pDate)
          *pBuff++ = *pDate++;
        SysFreeString(date);
      }
      break;

    case FMT_DATE_DAY:
      szPrintFmt = L"%d";
      dwVal = udate.st.wDay;
      break;

    case FMT_DATE_DAY_0:
      szPrintFmt = L"%02d";
      dwVal = udate.st.wDay;
      break;

    case FMT_DATE_DAY_SHORT:
      /* FIXME: VARIANT_CALENDAR HIJRI should cause Hijri output */
      TRACE("short day\n");
      localeValue = LOCALE_SABBREVDAYNAME1 + (udate.st.wDayOfWeek + 6)%7;
      defaultChar = '?';
      break;

    case FMT_DATE_DAY_LONG:
      /* FIXME: VARIANT_CALENDAR HIJRI should cause Hijri output */
      TRACE("long day\n");
      localeValue = LOCALE_SDAYNAME1 + (udate.st.wDayOfWeek + 6)%7;
      defaultChar = '?';
      break;

    case FMT_DATE_SHORT:
      /* FIXME: VARIANT_CALENDAR HIJRI should cause Hijri output */
      dwFmt = LOCALE_SSHORTDATE;
      break;

    case FMT_DATE_LONG:
      /* FIXME: VARIANT_CALENDAR HIJRI should cause Hijri output */
      dwFmt = LOCALE_SLONGDATE;
      break;

    case FMT_DATE_MEDIUM:
      FIXME("Medium date treated as long date\n");
      dwFmt = LOCALE_SLONGDATE;
      break;

    case FMT_DATE_DAY_WEEK:
      szPrintFmt = L"%d";
      if (pToken[1])
        dwVal = udate.st.wDayOfWeek + 2 - pToken[1];
      else
      {
        GetLocaleInfoW(lcid,LOCALE_RETURN_NUMBER|LOCALE_IFIRSTDAYOFWEEK,
                       (LPWSTR)&dwVal, sizeof(dwVal)/sizeof(WCHAR));
        dwVal = udate.st.wDayOfWeek + 1 - dwVal;
      }
      pToken++;
      break;

    case FMT_DATE_WEEK_YEAR:
      szPrintFmt = L"%d";
      dwVal = udate.wDayOfYear / 7 + 1;
      pToken += 2;
      FIXME("Ignoring nFirstDay of %d, nFirstWeek of %d\n", pToken[0], pToken[1]);
      break;

    case FMT_DATE_MON:
      szPrintFmt = L"%d";
      dwVal = udate.st.wMonth;
      break;

    case FMT_DATE_MON_0:
      szPrintFmt = L"%02d";
      dwVal = udate.st.wMonth;
      break;

    case FMT_DATE_MON_SHORT:
      /* FIXME: VARIANT_CALENDAR HIJRI should cause Hijri output */
      TRACE("short month\n");
      localeValue = LOCALE_SABBREVMONTHNAME1 + udate.st.wMonth - 1;
      defaultChar = '?';
      break;

    case FMT_DATE_MON_LONG:
      /* FIXME: VARIANT_CALENDAR HIJRI should cause Hijri output */
      TRACE("long month\n");
      localeValue = LOCALE_SMONTHNAME1 + udate.st.wMonth - 1;
      defaultChar = '?';
      break;

    case FMT_DATE_YEAR_DOY:
      szPrintFmt = L"%d";
      dwVal = udate.wDayOfYear;
      break;

    case FMT_DATE_YEAR_0:
      szPrintFmt = L"%02d";
      dwVal = udate.st.wYear % 100;
      break;

    case FMT_DATE_YEAR_LONG:
      szPrintFmt = L"%d";
      dwVal = udate.st.wYear;
      break;

    case FMT_DATE_MIN:
      szPrintFmt = L"%d";
      dwVal = udate.st.wMinute;
      break;

    case FMT_DATE_MIN_0:
      szPrintFmt = L"%02d";
      dwVal = udate.st.wMinute;
      break;

    case FMT_DATE_SEC:
      szPrintFmt = L"%d";
      dwVal = udate.st.wSecond;
      break;

    case FMT_DATE_SEC_0:
      szPrintFmt = L"%02d";
      dwVal = udate.st.wSecond;
      break;

    case FMT_DATE_HOUR:
      szPrintFmt = L"%d";
      dwVal = udate.st.wHour;
      break;

    case FMT_DATE_HOUR_0:
    case FMT_DATE_TIME_UNK2:
      szPrintFmt = L"%02d";
      dwVal = udate.st.wHour;
      break;

    case FMT_DATE_HOUR_12:
      szPrintFmt = L"%d";
      dwVal = udate.st.wHour ? udate.st.wHour > 12 ? udate.st.wHour - 12 : udate.st.wHour : 12;
      break;

    case FMT_DATE_HOUR_12_0:
      szPrintFmt = L"%02d";
      dwVal = udate.st.wHour ? udate.st.wHour > 12 ? udate.st.wHour - 12 : udate.st.wHour : 12;
      break;

    case FMT_DATE_AMPM_SYS1:
    case FMT_DATE_AMPM_SYS2:
      localeValue = udate.st.wHour < 12 ? LOCALE_S1159 : LOCALE_S2359;
      defaultChar = '?';
      break;

    case FMT_DATE_AMPM_UPPER:
      *pBuff++ = udate.st.wHour < 12 ? 'A' : 'P';
      *pBuff++ = 'M';
      break;

    case FMT_DATE_A_UPPER:
      *pBuff++ = udate.st.wHour < 12 ? 'A' : 'P';
      break;

    case FMT_DATE_AMPM_LOWER:
      *pBuff++ = udate.st.wHour < 12 ? 'a' : 'p';
      *pBuff++ = 'm';
      break;

    case FMT_DATE_A_LOWER:
      *pBuff++ = udate.st.wHour < 12 ? 'a' : 'p';
      break;

    default:
      ERR("Unknown token 0x%02x!\n", *pToken);
      hRes = E_INVALIDARG;
      goto VARIANT_FormatDate_Exit;
    }
    if (localeValue)
    {
      *pBuff = '\0';
      if (GetLocaleInfoW(lcid, localeValue, pBuff, ARRAY_SIZE(buff)-(pBuff-buff)))
      {
        TRACE("added %s\n", debugstr_w(pBuff));
        while (*pBuff)
          pBuff++;
      }
      else
      {
        TRACE("added %d %c\n", defaultChar, defaultChar);
        *pBuff++ = defaultChar;
      }
    }
    else if (dwFmt)
    {
      WCHAR fmt_buff[80];

      if (!GetLocaleInfoW(lcid, dwFmt, fmt_buff, ARRAY_SIZE(fmt_buff)) ||
          !get_date_format(lcid, 0, &udate.st, fmt_buff, pBuff, ARRAY_SIZE(buff)-(pBuff-buff)))
      {
        hRes = E_INVALIDARG;
        goto VARIANT_FormatDate_Exit;
      }
      while (*pBuff)
        pBuff++;
    }
    else if (szPrintFmt)
    {
      swprintf(pBuff, ARRAY_SIZE(buff) - (pBuff - buff), szPrintFmt, dwVal);
      while (*pBuff)
        pBuff++;
    }
    pToken++;
  }

VARIANT_FormatDate_Exit:
  *pBuff = '\0';
  TRACE("buff is %s\n", debugstr_w(buff));
  if (SUCCEEDED(hRes))
  {
    *pbstrOut = SysAllocString(buff);
    if (!*pbstrOut)
      hRes = E_OUTOFMEMORY;
  }
  return hRes;
}

/* Format a variant using a string format */
static HRESULT VARIANT_FormatString(LPVARIANT pVarIn, LPOLESTR lpszFormat,
                                    LPBYTE rgbTok, ULONG dwFlags,
                                    BSTR *pbstrOut, LCID lcid)
{
  static WCHAR szEmpty[] = L"";
  WCHAR buff[256], *pBuff = buff;
  WCHAR *pSrc;
  FMT_HEADER *header = (FMT_HEADER*)rgbTok;
  FMT_STRING_HEADER *strHeader;
  const BYTE* pToken = NULL;
  VARIANT vStr;
  int blanks_first;
  BOOL bUpper = FALSE;
  HRESULT hRes = S_OK;

  TRACE("%s, %s, %p, %#lx, %p, %#lx.\n", debugstr_variant(pVarIn), debugstr_w(lpszFormat),
        rgbTok, dwFlags, pbstrOut, lcid);

  V_VT(&vStr) = VT_EMPTY;

  if (V_TYPE(pVarIn) == VT_EMPTY || V_TYPE(pVarIn) == VT_NULL)
  {
    strHeader = (FMT_STRING_HEADER*)(rgbTok + FmtGetNegative(header));
    V_BSTR(&vStr) = szEmpty;
  }
  else
  {
    hRes = VariantChangeTypeEx(&vStr, pVarIn, lcid, VARIANT_NOUSEROVERRIDE, VT_BSTR);
    if (FAILED(hRes))
      return hRes;

    if (V_BSTR(&vStr)[0] == '\0')
      strHeader = (FMT_STRING_HEADER*)(rgbTok + FmtGetNegative(header));
    else
      strHeader = (FMT_STRING_HEADER*)(rgbTok + FmtGetPositive(header));
  }
  pSrc = V_BSTR(&vStr);
  if ((strHeader->flags & (FMT_FLAG_LT|FMT_FLAG_GT)) == FMT_FLAG_GT)
    bUpper = TRUE;
  blanks_first = strHeader->copy_chars - lstrlenW(pSrc);
  pToken = (const BYTE*)strHeader + sizeof(FMT_DATE_HEADER);

  while (*pToken != FMT_GEN_END)
  {
    int dwCount = 0;

    if (pToken - rgbTok > header->size)
    {
      ERR("Ran off the end of the format!\n");
      hRes = E_INVALIDARG;
      goto VARIANT_FormatString_Exit;
    }

    switch (*pToken)
    {
    case FMT_GEN_COPY:
      TRACE("copy %s\n", debugstr_wn(lpszFormat + pToken[1], pToken[2]));
      memcpy(pBuff, lpszFormat + pToken[1], pToken[2] * sizeof(WCHAR));
      pBuff += pToken[2];
      pToken += 2;
      break;

    case FMT_STR_COPY_SPACE:
    case FMT_STR_COPY_SKIP:
      dwCount = pToken[1];
      if (*pToken == FMT_STR_COPY_SPACE && blanks_first > 0)
      {
        TRACE("insert %d initial spaces\n", blanks_first);
        while (dwCount > 0 && blanks_first > 0)
        {
          *pBuff++ = ' ';
          dwCount--;
          blanks_first--;
        }
      }
      TRACE("copy %d chars%s\n", dwCount,
            *pToken == FMT_STR_COPY_SPACE ? " with space" :"");
      while (dwCount > 0 && *pSrc)
      {
        if (bUpper)
          *pBuff++ = towupper(*pSrc);
        else
          *pBuff++ = towlower(*pSrc);
        dwCount--;
        pSrc++;
      }
      if (*pToken == FMT_STR_COPY_SPACE && dwCount > 0)
      {
        TRACE("insert %d spaces\n", dwCount);
        while (dwCount-- > 0)
          *pBuff++ = ' ';
      }
      pToken++;
      break;

    default:
      ERR("Unknown token 0x%02x!\n", *pToken);
      hRes = E_INVALIDARG;
      goto VARIANT_FormatString_Exit;
    }
    pToken++;
  }

VARIANT_FormatString_Exit:
  /* Copy out any remaining chars */
  while (*pSrc)
  {
    if (bUpper)
      *pBuff++ = towupper(*pSrc);
    else
      *pBuff++ = towlower(*pSrc);
    pSrc++;
  }
  VariantClear(&vStr);
  *pBuff = '\0';
  TRACE("buff is %s\n", debugstr_w(buff));
  if (SUCCEEDED(hRes))
  {
    *pbstrOut = SysAllocString(buff);
    if (!*pbstrOut)
      hRes = E_OUTOFMEMORY;
  }
  return hRes;
}

#define NUMBER_VTBITS (VTBIT_I1|VTBIT_UI1|VTBIT_I2|VTBIT_UI2| \
                       VTBIT_I4|VTBIT_UI4|VTBIT_I8|VTBIT_UI8| \
                       VTBIT_R4|VTBIT_R8|VTBIT_CY|VTBIT_DECIMAL| \
                       VTBIT_BOOL|VTBIT_INT|VTBIT_UINT)

/**********************************************************************
 *              VarFormatFromTokens [OLEAUT32.139]
 */
HRESULT WINAPI VarFormatFromTokens(LPVARIANT pVarIn, LPOLESTR lpszFormat,
                                   LPBYTE rgbTok, ULONG dwFlags,
                                   BSTR *pbstrOut, LCID lcid)
{
  FMT_SHORT_HEADER *header = (FMT_SHORT_HEADER *)rgbTok;
  VARIANT vTmp;
  HRESULT hres;

  TRACE("%p, %s, %p, %#lx, %p, %#lx.\n", pVarIn, debugstr_w(lpszFormat),
          rgbTok, dwFlags, pbstrOut, lcid);

  if (!pbstrOut)
    return E_INVALIDARG;

  *pbstrOut = NULL;

  if (!pVarIn || !rgbTok)
    return E_INVALIDARG;

  if (V_VT(pVarIn) == VT_NULL)
    return S_OK;

  if (*rgbTok == FMT_TO_STRING || header->type == FMT_TYPE_GENERAL)
  {
    /* According to MSDN, general format acts somewhat like the 'Str'
     * function in Visual Basic.
     */
VarFormatFromTokens_AsStr:
    V_VT(&vTmp) = VT_EMPTY;
    hres = VariantChangeTypeEx(&vTmp, pVarIn, lcid, dwFlags, VT_BSTR);
    *pbstrOut = V_BSTR(&vTmp);
  }
  else
  {
    if (header->type == FMT_TYPE_NUMBER ||
        (header->type == FMT_TYPE_UNKNOWN && ((1 << V_TYPE(pVarIn)) & NUMBER_VTBITS)))
    {
      hres = VARIANT_FormatNumber(pVarIn, lpszFormat, rgbTok, dwFlags, pbstrOut, lcid);
    }
    else if (header->type == FMT_TYPE_DATE ||
             (header->type == FMT_TYPE_UNKNOWN && V_TYPE(pVarIn) == VT_DATE))
    {
      hres = VARIANT_FormatDate(pVarIn, lpszFormat, rgbTok, dwFlags, pbstrOut, lcid);
    }
    else if (header->type == FMT_TYPE_STRING || V_TYPE(pVarIn) == VT_BSTR)
    {
      hres = VARIANT_FormatString(pVarIn, lpszFormat, rgbTok, dwFlags, pbstrOut, lcid);
    }
    else
    {
      ERR("unrecognised format type 0x%02x\n", header->type);
      return E_INVALIDARG;
    }
    /* If the coercion failed, still try to create output, unless the
     * VAR_FORMAT_NOSUBSTITUTE flag is set.
     */
    if ((hres == DISP_E_OVERFLOW || hres == DISP_E_TYPEMISMATCH) &&
        !(dwFlags & VAR_FORMAT_NOSUBSTITUTE))
      goto VarFormatFromTokens_AsStr;
  }

  return hres;
}

/**********************************************************************
 *              VarFormat [OLEAUT32.87]
 *
 * Format a variant from a format string.
 *
 * PARAMS
 *  pVarIn     [I] Variant to format
 *  lpszFormat [I] Format string (see notes)
 *  nFirstDay  [I] First day of the week, (See VarTokenizeFormatString() for details)
 *  nFirstWeek [I] First week of the year (See VarTokenizeFormatString() for details)
 *  dwFlags    [I] Flags for the format (VAR_ flags from "oleauto.h")
 *  pbstrOut   [O] Destination for formatted string.
 *
 * RETURNS
 *  Success: S_OK. pbstrOut contains the formatted value.
 *  Failure: E_INVALIDARG, if any parameter is invalid.
 *           E_OUTOFMEMORY, if enough memory cannot be allocated.
 *           DISP_E_TYPEMISMATCH, if the variant cannot be formatted.
 *
 * NOTES
 *  - See Variant-Formats for details concerning creating format strings.
 *  - This function uses LOCALE_USER_DEFAULT when calling VarTokenizeFormatString()
 *    and VarFormatFromTokens().
 */
HRESULT WINAPI VarFormat(LPVARIANT pVarIn, LPOLESTR lpszFormat,
                         int nFirstDay, int nFirstWeek, ULONG dwFlags,
                         BSTR *pbstrOut)
{
  BYTE buff[256];
  HRESULT hres;

  TRACE("%s, %s, %d, %d, %#lx, %p.\n", debugstr_variant(pVarIn), debugstr_w(lpszFormat),
        nFirstDay, nFirstWeek, dwFlags, pbstrOut);

  if (!pbstrOut)
    return E_INVALIDARG;
  *pbstrOut = NULL;

  hres = VarTokenizeFormatString(lpszFormat, buff, sizeof(buff), nFirstDay,
                                 nFirstWeek, LOCALE_USER_DEFAULT, NULL);
  if (SUCCEEDED(hres))
    hres = VarFormatFromTokens(pVarIn, lpszFormat, buff, dwFlags,
                               pbstrOut, LOCALE_USER_DEFAULT);
  TRACE("returning %#lx, %s\n", hres, debugstr_w(*pbstrOut));
  return hres;
}

/**********************************************************************
 *              VarFormatDateTime [OLEAUT32.97]
 *
 * Format a variant value as a date and/or time.
 *
 * PARAMS
 *  pVarIn    [I] Variant to format
 *  nFormat   [I] Format type (see notes)
 *  dwFlags   [I] Flags for the format (VAR_ flags from "oleauto.h")
 *  pbstrOut  [O] Destination for formatted string.
 *
 * RETURNS
 *  Success: S_OK. pbstrOut contains the formatted value.
 *  Failure: E_INVALIDARG, if any parameter is invalid.
 *           E_OUTOFMEMORY, if enough memory cannot be allocated.
 *           DISP_E_TYPEMISMATCH, if the variant cannot be formatted.
 *
 * NOTES
 *  This function uses LOCALE_USER_DEFAULT when determining the date format
 *  characters to use.
 *  Possible values for the nFormat parameter are:
 *| Value  Meaning
 *| -----  -------
 *|   0    General date format
 *|   1    Long date format
 *|   2    Short date format
 *|   3    Long time format
 *|   4    Short time format
 */
HRESULT WINAPI VarFormatDateTime(LPVARIANT pVarIn, INT nFormat, ULONG dwFlags, BSTR *pbstrOut)
{
  static WCHAR szEmpty[] = L"";
  const BYTE* lpFmt = NULL;

  TRACE("%s, %d, %#lx, %p.\n", debugstr_variant(pVarIn), nFormat, dwFlags, pbstrOut);

  if (!pVarIn || !pbstrOut || nFormat < 0 || nFormat > 4)
    return E_INVALIDARG;

  switch (nFormat)
  {
  case 0: lpFmt = fmtGeneralDate; break;
  case 1: lpFmt = fmtLongDate; break;
  case 2: lpFmt = fmtShortDate; break;
  case 3: lpFmt = fmtLongTime; break;
  case 4: lpFmt = fmtShortTime; break;
  }
  return VarFormatFromTokens(pVarIn, szEmpty, (BYTE*)lpFmt, dwFlags,
                              pbstrOut, LOCALE_USER_DEFAULT);
}

#define GETLOCALENUMBER(type,field) GetLocaleInfoW(LOCALE_USER_DEFAULT, \
                                                   type|LOCALE_RETURN_NUMBER, \
                                                   (LPWSTR)&numfmt.field, \
                                                   sizeof(numfmt.field)/sizeof(WCHAR))

/**********************************************************************
 *              VarFormatNumber [OLEAUT32.107]
 *
 * Format a variant value as a number.
 *
 * PARAMS
 *  pVarIn    [I] Variant to format
 *  nDigits   [I] Number of digits following the decimal point (-1 = user default)
 *  nLeading  [I] Use a leading zero (-2 = user default, -1 = yes, 0 = no)
 *  nParens   [I] Use brackets for values < 0 (-2 = user default, -1 = yes, 0 = no)
 *  nGrouping [I] Use grouping characters (-2 = user default, -1 = yes, 0 = no)
 *  dwFlags   [I] Currently unused, set to zero
 *  pbstrOut  [O] Destination for formatted string.
 *
 * RETURNS
 *  Success: S_OK. pbstrOut contains the formatted value.
 *  Failure: E_INVALIDARG, if any parameter is invalid.
 *           E_OUTOFMEMORY, if enough memory cannot be allocated.
 *           DISP_E_TYPEMISMATCH, if the variant cannot be formatted.
 *
 * NOTES
 *  This function uses LOCALE_USER_DEFAULT when determining the number format
 *  characters to use.
 */
HRESULT WINAPI VarFormatNumber(LPVARIANT pVarIn, INT nDigits, INT nLeading, INT nParens,
                               INT nGrouping, ULONG dwFlags, BSTR *pbstrOut)
{
  HRESULT hRet;
  VARIANT vStr;

  TRACE("%s, %d, %d, %d, %d, %#lx, %p.\n", debugstr_variant(pVarIn), nDigits, nLeading,
        nParens, nGrouping, dwFlags, pbstrOut);

  if (!pVarIn || !pbstrOut || nDigits > 9)
    return E_INVALIDARG;

  *pbstrOut = NULL;

  V_VT(&vStr) = VT_EMPTY;
  hRet = VariantCopyInd(&vStr, pVarIn);

  if (SUCCEEDED(hRet))
    hRet = VariantChangeTypeEx(&vStr, &vStr, LCID_US, 0, VT_BSTR);

  if (SUCCEEDED(hRet))
  {
    WCHAR buff[256], decimal[8], thousands[8];
    NUMBERFMTW numfmt;

    /* Although MSDN makes it clear that the native versions of these functions
     * are implemented using VarTokenizeFormatString()/VarFormatFromTokens(),
     * using NLS gives us the same result.
     */
    if (nDigits < 0)
      GETLOCALENUMBER(LOCALE_IDIGITS, NumDigits);
    else
      numfmt.NumDigits = nDigits;

    if (nLeading == -2)
      GETLOCALENUMBER(LOCALE_ILZERO, LeadingZero);
    else if (nLeading == -1)
      numfmt.LeadingZero = 1;
    else
      numfmt.LeadingZero = 0;

    if (nGrouping == -2)
    {
      WCHAR grouping[10];
      grouping[2] = '\0';
      GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, grouping, ARRAY_SIZE(grouping));
      numfmt.Grouping = grouping[2] == '2' ? 32 : grouping[0] - '0';
    }
    else if (nGrouping == -1)
      numfmt.Grouping = 3; /* 3 = "n,nnn.nn" */
    else
      numfmt.Grouping = 0; /* 0 = No grouping */

    if (nParens == -2)
      GETLOCALENUMBER(LOCALE_INEGNUMBER, NegativeOrder);
    else if (nParens == -1)
      numfmt.NegativeOrder = 0; /* 0 = "(xxx)" */
    else
      numfmt.NegativeOrder = 1; /* 1 = "-xxx" */

    numfmt.lpDecimalSep = decimal;
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, decimal, ARRAY_SIZE(decimal));
    numfmt.lpThousandSep = thousands;
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, thousands, ARRAY_SIZE(thousands));

    if (GetNumberFormatW(LOCALE_USER_DEFAULT, 0, V_BSTR(&vStr), &numfmt, buff, ARRAY_SIZE(buff)))
    {
      *pbstrOut = SysAllocString(buff);
      if (!*pbstrOut)
        hRet = E_OUTOFMEMORY;
    }
    else
      hRet = DISP_E_TYPEMISMATCH;

    SysFreeString(V_BSTR(&vStr));
  }
  return hRet;
}

/**********************************************************************
 *              VarFormatPercent [OLEAUT32.117]
 *
 * Format a variant value as a percentage.
 *
 * PARAMS
 *  pVarIn    [I] Variant to format
 *  nDigits   [I] Number of digits following the decimal point (-1 = user default)
 *  nLeading  [I] Use a leading zero (-2 = user default, -1 = yes, 0 = no)
 *  nParens   [I] Use brackets for values < 0 (-2 = user default, -1 = yes, 0 = no)
 *  nGrouping [I] Use grouping characters (-2 = user default, -1 = yes, 0 = no)
 *  dwFlags   [I] Currently unused, set to zero
 *  pbstrOut  [O] Destination for formatted string.
 *
 * RETURNS
 *  Success: S_OK. pbstrOut contains the formatted value.
 *  Failure: E_INVALIDARG, if any parameter is invalid.
 *           E_OUTOFMEMORY, if enough memory cannot be allocated.
 *           DISP_E_OVERFLOW, if overflow occurs during the conversion.
 *           DISP_E_TYPEMISMATCH, if the variant cannot be formatted.
 *
 * NOTES
 *  This function uses LOCALE_USER_DEFAULT when determining the number format
 *  characters to use.
 */
HRESULT WINAPI VarFormatPercent(LPVARIANT pVarIn, INT nDigits, INT nLeading, INT nParens,
                                INT nGrouping, ULONG dwFlags, BSTR *pbstrOut)
{
  WCHAR buff[256];
  HRESULT hRet;
  VARIANT vDbl;

  TRACE("%s, %d, %d, %d, %d, %#lx, %p.\n", debugstr_variant(pVarIn), nDigits, nLeading,
        nParens, nGrouping, dwFlags, pbstrOut);

  if (!pVarIn || !pbstrOut || nDigits > 9)
    return E_INVALIDARG;

  *pbstrOut = NULL;

  V_VT(&vDbl) = VT_EMPTY;
  hRet = VariantCopyInd(&vDbl, pVarIn);

  if (SUCCEEDED(hRet))
  {
    hRet = VariantChangeTypeEx(&vDbl, &vDbl, LOCALE_USER_DEFAULT, 0, VT_R8);

    if (SUCCEEDED(hRet))
    {
      if (V_R8(&vDbl) > (R8_MAX / 100.0))
        return DISP_E_OVERFLOW;

      V_R8(&vDbl) *= 100.0;
      hRet = VarFormatNumber(&vDbl, nDigits, nLeading, nParens,
                             nGrouping, dwFlags, pbstrOut);

      if (SUCCEEDED(hRet))
      {
        DWORD dwLen = lstrlenW(*pbstrOut);
        BOOL bBracket = (*pbstrOut)[dwLen] == ')';

        dwLen -= bBracket;
        memcpy(buff, *pbstrOut, dwLen * sizeof(WCHAR));
        lstrcpyW(buff + dwLen, bBracket ? L"%)" : L"%");
        SysFreeString(*pbstrOut);
        *pbstrOut = SysAllocString(buff);
        if (!*pbstrOut)
          hRet = E_OUTOFMEMORY;
      }
    }
  }
  return hRet;
}

/**********************************************************************
 *              VarFormatCurrency [OLEAUT32.127]
 *
 * Format a variant value as a currency.
 *
 * PARAMS
 *  pVarIn    [I] Variant to format
 *  nDigits   [I] Number of digits following the decimal point (-1 = user default)
 *  nLeading  [I] Use a leading zero (-2 = user default, -1 = yes, 0 = no)
 *  nParens   [I] Use brackets for values < 0 (-2 = user default, -1 = yes, 0 = no)
 *  nGrouping [I] Use grouping characters (-2 = user default, -1 = yes, 0 = no)
 *  dwFlags   [I] Currently unused, set to zero
 *  pbstrOut  [O] Destination for formatted string.
 *
 * RETURNS
 *  Success: S_OK. pbstrOut contains the formatted value.
 *  Failure: E_INVALIDARG, if any parameter is invalid.
 *           E_OUTOFMEMORY, if enough memory cannot be allocated.
 *           DISP_E_TYPEMISMATCH, if the variant cannot be formatted.
 *
 * NOTES
 *  This function uses LOCALE_USER_DEFAULT when determining the currency format
 *  characters to use.
 */
HRESULT WINAPI VarFormatCurrency(LPVARIANT pVarIn, INT nDigits, INT nLeading,
                                 INT nParens, INT nGrouping, ULONG dwFlags,
                                 BSTR *pbstrOut)
{
  HRESULT hRet;
  VARIANT vStr;
  CY cy;

  TRACE("%s, %d, %d, %d, %d, %#lx, %p.\n", debugstr_variant(pVarIn), nDigits, nLeading,
        nParens, nGrouping, dwFlags, pbstrOut);

  if (!pVarIn || !pbstrOut || nDigits > 9)
    return E_INVALIDARG;

  *pbstrOut = NULL;

  if (V_VT(pVarIn) == VT_BSTR || V_VT(pVarIn) == (VT_BSTR | VT_BYREF))
  {
    hRet = VarCyFromStr(V_ISBYREF(pVarIn) ? *V_BSTRREF(pVarIn) : V_BSTR(pVarIn), LOCALE_USER_DEFAULT, 0, &cy);
    if (FAILED(hRet)) return hRet;
    V_VT(&vStr) = VT_CY;
    V_CY(&vStr) = cy;
  }
  else
  {
    V_VT(&vStr) = VT_EMPTY;
    hRet = VariantCopyInd(&vStr, pVarIn);
  }

  if (SUCCEEDED(hRet))
    hRet = VariantChangeTypeEx(&vStr, &vStr, LOCALE_USER_DEFAULT, 0, VT_BSTR);

  if (SUCCEEDED(hRet))
  {
    WCHAR buff[256], decimal[8], thousands[4], currency[13];
    CURRENCYFMTW numfmt;

    if (nDigits < 0)
      GETLOCALENUMBER(LOCALE_IDIGITS, NumDigits);
    else
      numfmt.NumDigits = nDigits;

    if (nLeading == -2)
      GETLOCALENUMBER(LOCALE_ILZERO, LeadingZero);
    else if (nLeading == -1)
      numfmt.LeadingZero = 1;
    else
      numfmt.LeadingZero = 0;

    if (nGrouping == -2)
    {
      WCHAR grouping[10];
      grouping[2] = '\0';
      GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, grouping, ARRAY_SIZE(grouping));
      numfmt.Grouping = grouping[2] == '2' ? 32 : grouping[0] - '0';
    }
    else if (nGrouping == -1)
      numfmt.Grouping = 3; /* 3 = "n,nnn.nn" */
    else
      numfmt.Grouping = 0; /* 0 = No grouping */

    if (nParens == -2)
      GETLOCALENUMBER(LOCALE_INEGCURR, NegativeOrder);
    else if (nParens == -1)
      numfmt.NegativeOrder = 0; /* 0 = "(xxx)" */
    else
      numfmt.NegativeOrder = 1; /* 1 = "-xxx" */

    GETLOCALENUMBER(LOCALE_ICURRENCY, PositiveOrder);

    numfmt.lpDecimalSep = decimal;
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, decimal, ARRAY_SIZE(decimal));
    numfmt.lpThousandSep = thousands;
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, thousands, ARRAY_SIZE(thousands));
    numfmt.lpCurrencySymbol = currency;
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SCURRENCY, currency, ARRAY_SIZE(currency));

    /* use NLS as per VarFormatNumber() */
    if (GetCurrencyFormatW(LOCALE_USER_DEFAULT, 0, V_BSTR(&vStr), &numfmt, buff, ARRAY_SIZE(buff)))
    {
      *pbstrOut = SysAllocString(buff);
      if (!*pbstrOut)
        hRet = E_OUTOFMEMORY;
    }
    else
      hRet = DISP_E_TYPEMISMATCH;

    SysFreeString(V_BSTR(&vStr));
  }
  return hRet;
}

/**********************************************************************
 *              VarMonthName [OLEAUT32.129]
 *
 * Print the specified month as localized name.
 *
 * PARAMS
 *  iMonth    [I] month number 1..12
 *  fAbbrev   [I] 0 - full name, !0 - abbreviated name
 *  dwFlags   [I] flag stuff. only VAR_CALENDAR_HIJRI possible.
 *  pbstrOut  [O] Destination for month name
 *
 * RETURNS
 *  Success: S_OK. pbstrOut contains the name.
 *  Failure: E_INVALIDARG, if any parameter is invalid.
 *           E_OUTOFMEMORY, if enough memory cannot be allocated.
 */
HRESULT WINAPI VarMonthName(INT iMonth, INT fAbbrev, ULONG dwFlags, BSTR *pbstrOut)
{
  DWORD localeValue;
  INT size;

  if ((iMonth < 1)  || (iMonth > 12))
    return E_INVALIDARG;

  if (dwFlags)
    FIXME("Does not support flags %#lx, ignoring.\n", dwFlags);

  if (fAbbrev)
	localeValue = LOCALE_SABBREVMONTHNAME1 + iMonth - 1;
  else
	localeValue = LOCALE_SMONTHNAME1 + iMonth - 1;

  size = GetLocaleInfoW(LOCALE_USER_DEFAULT,localeValue, NULL, 0);
  if (!size) {
    ERR("GetLocaleInfo %#lx failed.\n", localeValue);
    return HRESULT_FROM_WIN32(GetLastError());
  }
  *pbstrOut = SysAllocStringLen(NULL,size - 1);
  if (!*pbstrOut)
    return E_OUTOFMEMORY;
  size = GetLocaleInfoW(LOCALE_USER_DEFAULT,localeValue, *pbstrOut, size);
  if (!size) {
    ERR("GetLocaleInfo of %#lx failed in 2nd stage?!\n", localeValue);
    SysFreeString(*pbstrOut);
    return HRESULT_FROM_WIN32(GetLastError());
  }
  return S_OK;
}

/**********************************************************************
 *              VarWeekdayName [OLEAUT32.129]
 *
 * Print the specified weekday as localized name.
 *
 * PARAMS
 *  iWeekday  [I] day of week, 1..7, 1="the first day of the week"
 *  fAbbrev   [I] 0 - full name, !0 - abbreviated name
 *  iFirstDay [I] first day of week,
 *                0=system default, 1=Sunday, 2=Monday, .. (contrary to MSDN)
 *  dwFlags   [I] flag stuff. only VAR_CALENDAR_HIJRI possible.
 *  pbstrOut  [O] Destination for weekday name.
 *
 * RETURNS
 *  Success: S_OK, pbstrOut contains the name.
 *  Failure: E_INVALIDARG, if any parameter is invalid.
 *           E_OUTOFMEMORY, if enough memory cannot be allocated.
 */
HRESULT WINAPI VarWeekdayName(INT iWeekday, INT fAbbrev, INT iFirstDay,
                              ULONG dwFlags, BSTR *pbstrOut)
{
  DWORD localeValue;
  INT size;

  /* Windows XP oleaut32.dll doesn't allow iWekday==0, contrary to MSDN */
  if (iWeekday < 1 || iWeekday > 7)
    return E_INVALIDARG;
  if (iFirstDay < 0 || iFirstDay > 7)
    return E_INVALIDARG;
  if (!pbstrOut)
    return E_INVALIDARG;

  if (dwFlags)
    FIXME("Does not support flags %#lx, ignoring.\n", dwFlags);

  /* If we have to use the default firstDay, find which one it is */
  if (iFirstDay == 0) {
    DWORD firstDay;
    localeValue = LOCALE_RETURN_NUMBER | LOCALE_IFIRSTDAYOFWEEK;
    size = GetLocaleInfoW(LOCALE_USER_DEFAULT, localeValue,
                          (LPWSTR)&firstDay, sizeof(firstDay) / sizeof(WCHAR));
    if (!size) {
      ERR("GetLocaleInfo %#lx failed.\n", localeValue);
      return HRESULT_FROM_WIN32(GetLastError());
    }
    iFirstDay = firstDay + 2;
  }

  /* Determine what we need to return */
  localeValue = fAbbrev ? LOCALE_SABBREVDAYNAME1 : LOCALE_SDAYNAME1;
  localeValue += (7 + iWeekday - 1 + iFirstDay - 2) % 7;

  /* Determine the size of the data, allocate memory and retrieve the data */
  size = GetLocaleInfoW(LOCALE_USER_DEFAULT, localeValue, NULL, 0);
  if (!size) {
    ERR("GetLocaleInfo %#lx failed.\n", localeValue);
    return HRESULT_FROM_WIN32(GetLastError());
  }
  *pbstrOut = SysAllocStringLen(NULL, size - 1);
  if (!*pbstrOut)
    return E_OUTOFMEMORY;
  size = GetLocaleInfoW(LOCALE_USER_DEFAULT, localeValue, *pbstrOut, size);
  if (!size) {
    ERR("GetLocaleInfo %#lx failed in 2nd stage?!\n", localeValue);
    SysFreeString(*pbstrOut);
    return HRESULT_FROM_WIN32(GetLastError());
  }
  return S_OK;
}

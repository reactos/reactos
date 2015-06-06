/*
 * Locale-dependent format handling
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1998 David Lee Lambert
 * Copyright 2000 Julio C√©sar G√°zquez
 * Copyright 2003 Jon Griffiths
 * Copyright 2005 Dmitry Timoshkov
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
 */

#include <k32.h>

#define NDEBUG
#include <debug.h>
DEBUG_CHANNEL(nls);

#define CRITICAL_SECTION RTL_CRITICAL_SECTION
#define CRITICAL_SECTION_DEBUG RTL_CRITICAL_SECTION_DEBUG
#define CALINFO_MAX_YEAR 2029

#define DATE_DATEVARSONLY 0x0100  /* only date stuff: yMdg */
#define TIME_TIMEVARSONLY 0x0200  /* only time stuff: hHmst */

/* Since calculating the formatting data for each locale is time-consuming,
 * we get the format data for each locale only once and cache it in memory.
 * We cache both the system default and user overridden data, after converting
 * them into the formats that the functions here expect. Since these functions
 * will typically be called with only a small number of the total locales
 * installed, the memory overhead is minimal while the speedup is significant.
 *
 * Our cache takes the form of a singly linked list, whose node is below:
 */
#define NLS_NUM_CACHED_STRINGS 57

typedef struct _NLS_FORMAT_NODE
{
  LCID  lcid;         /* Locale Id */
  DWORD dwFlags;      /* 0 or LOCALE_NOUSEROVERRIDE */
  DWORD dwCodePage;   /* Default code page (if LOCALE_USE_ANSI_CP not given) */
  NUMBERFMTW   fmt;   /* Default format for numbers */
  CURRENCYFMTW cyfmt; /* Default format for currencies */
  LPWSTR lppszStrings[NLS_NUM_CACHED_STRINGS]; /* Default formats,day/month names */
  WCHAR szShortAM[2]; /* Short 'AM' marker */
  WCHAR szShortPM[2]; /* Short 'PM' marker */
  struct _NLS_FORMAT_NODE *next;
} NLS_FORMAT_NODE;

/* Macros to get particular data strings from a format node */
#define GetNegative(fmt)  fmt->lppszStrings[0]
#define GetLongDate(fmt)  fmt->lppszStrings[1]
#define GetShortDate(fmt) fmt->lppszStrings[2]
#define GetTime(fmt)      fmt->lppszStrings[3]
#define GetAM(fmt)        fmt->lppszStrings[54]
#define GetPM(fmt)        fmt->lppszStrings[55]
#define GetYearMonth(fmt) fmt->lppszStrings[56]

#define GetLongDay(fmt,day)       fmt->lppszStrings[4 + day]
#define GetShortDay(fmt,day)      fmt->lppszStrings[11 + day]
#define GetLongMonth(fmt,mth)     fmt->lppszStrings[18 + mth]
#define GetGenitiveMonth(fmt,mth) fmt->lppszStrings[30 + mth]
#define GetShortMonth(fmt,mth)    fmt->lppszStrings[42 + mth]

/* Write access to the cache is protected by this critical section */
static CRITICAL_SECTION NLS_FormatsCS;
static CRITICAL_SECTION_DEBUG NLS_FormatsCS_debug =
{
    0, 0, &NLS_FormatsCS,
    { &NLS_FormatsCS_debug.ProcessLocksList,
      &NLS_FormatsCS_debug.ProcessLocksList },
      0, 0, 0
};
static CRITICAL_SECTION NLS_FormatsCS = { &NLS_FormatsCS_debug, -1, 0, 0, 0, 0 };

/**************************************************************************
 * NLS_GetLocaleNumber <internal>
 *
 * Get a numeric locale format value.
 */
static DWORD NLS_GetLocaleNumber(LCID lcid, DWORD dwFlags)
{
  WCHAR szBuff[80];
  DWORD dwVal = 0;

  szBuff[0] = '\0';
  GetLocaleInfoW(lcid, dwFlags, szBuff, sizeof(szBuff) / sizeof(WCHAR));

  if (szBuff[0] && szBuff[1] == ';' && szBuff[2] != '0')
    dwVal = (szBuff[0] - '0') * 10 + (szBuff[2] - '0');
  else
  {
    const WCHAR* iter = szBuff;
    dwVal = 0;
    while(*iter >= '0' && *iter <= '9')
      dwVal = dwVal * 10 + (*iter++ - '0');
  }
  return dwVal;
}

/**************************************************************************
 * NLS_GetLocaleString <internal>
 *
 * Get a string locale format value.
 */
static WCHAR* NLS_GetLocaleString(LCID lcid, DWORD dwFlags)
{
  WCHAR szBuff[80], *str;
  DWORD dwLen;

  szBuff[0] = '\0';
  GetLocaleInfoW(lcid, dwFlags, szBuff, sizeof(szBuff) / sizeof(WCHAR));
  dwLen = strlenW(szBuff) + 1;
  str = HeapAlloc(GetProcessHeap(), 0, dwLen * sizeof(WCHAR));
  if (str)
    memcpy(str, szBuff, dwLen * sizeof(WCHAR));
  return str;
}

#define GET_LOCALE_NUMBER(num, type) num = NLS_GetLocaleNumber(lcid, type|dwFlags); \
  TRACE( #type ": %d (%08x)\n", (DWORD)num, (DWORD)num)

#define GET_LOCALE_STRING(str, type) str = NLS_GetLocaleString(lcid, type|dwFlags); \
  TRACE( #type ": %s\n", debugstr_w(str))

/**************************************************************************
 * NLS_GetFormats <internal>
 *
 * Calculate (and cache) the number formats for a locale.
 */
static const NLS_FORMAT_NODE *NLS_GetFormats(LCID lcid, DWORD dwFlags)
{
  /* GetLocaleInfo() identifiers for cached formatting strings */
  static const LCTYPE NLS_LocaleIndices[] = {
    LOCALE_SNEGATIVESIGN,
    LOCALE_SLONGDATE,   LOCALE_SSHORTDATE,
    LOCALE_STIMEFORMAT,
    LOCALE_SDAYNAME1, LOCALE_SDAYNAME2, LOCALE_SDAYNAME3,
    LOCALE_SDAYNAME4, LOCALE_SDAYNAME5, LOCALE_SDAYNAME6, LOCALE_SDAYNAME7,
    LOCALE_SABBREVDAYNAME1, LOCALE_SABBREVDAYNAME2, LOCALE_SABBREVDAYNAME3,
    LOCALE_SABBREVDAYNAME4, LOCALE_SABBREVDAYNAME5, LOCALE_SABBREVDAYNAME6,
    LOCALE_SABBREVDAYNAME7,
    LOCALE_SMONTHNAME1, LOCALE_SMONTHNAME2, LOCALE_SMONTHNAME3,
    LOCALE_SMONTHNAME4, LOCALE_SMONTHNAME5, LOCALE_SMONTHNAME6,
    LOCALE_SMONTHNAME7, LOCALE_SMONTHNAME8, LOCALE_SMONTHNAME9,
    LOCALE_SMONTHNAME10, LOCALE_SMONTHNAME11, LOCALE_SMONTHNAME12,
    LOCALE_SMONTHNAME1  | LOCALE_RETURN_GENITIVE_NAMES,
    LOCALE_SMONTHNAME2  | LOCALE_RETURN_GENITIVE_NAMES,
    LOCALE_SMONTHNAME3  | LOCALE_RETURN_GENITIVE_NAMES,
    LOCALE_SMONTHNAME4  | LOCALE_RETURN_GENITIVE_NAMES,
    LOCALE_SMONTHNAME5  | LOCALE_RETURN_GENITIVE_NAMES,
    LOCALE_SMONTHNAME6  | LOCALE_RETURN_GENITIVE_NAMES,
    LOCALE_SMONTHNAME7  | LOCALE_RETURN_GENITIVE_NAMES,
    LOCALE_SMONTHNAME8  | LOCALE_RETURN_GENITIVE_NAMES,
    LOCALE_SMONTHNAME9  | LOCALE_RETURN_GENITIVE_NAMES,
    LOCALE_SMONTHNAME10 | LOCALE_RETURN_GENITIVE_NAMES,
    LOCALE_SMONTHNAME11 | LOCALE_RETURN_GENITIVE_NAMES,
    LOCALE_SMONTHNAME12 | LOCALE_RETURN_GENITIVE_NAMES,
    LOCALE_SABBREVMONTHNAME1, LOCALE_SABBREVMONTHNAME2, LOCALE_SABBREVMONTHNAME3,
    LOCALE_SABBREVMONTHNAME4, LOCALE_SABBREVMONTHNAME5, LOCALE_SABBREVMONTHNAME6,
    LOCALE_SABBREVMONTHNAME7, LOCALE_SABBREVMONTHNAME8, LOCALE_SABBREVMONTHNAME9,
    LOCALE_SABBREVMONTHNAME10, LOCALE_SABBREVMONTHNAME11, LOCALE_SABBREVMONTHNAME12,
    LOCALE_S1159, LOCALE_S2359,
    LOCALE_SYEARMONTH
  };
  static NLS_FORMAT_NODE *NLS_CachedFormats = NULL;
  NLS_FORMAT_NODE *node = NLS_CachedFormats;

  dwFlags &= LOCALE_NOUSEROVERRIDE;

  TRACE("(0x%04x,0x%08x)\n", lcid, dwFlags);

  /* See if we have already cached the locales number format */
  while (node && (node->lcid != lcid || node->dwFlags != dwFlags) && node->next)
    node = node->next;

  if (!node || node->lcid != lcid || node->dwFlags != dwFlags)
  {
    NLS_FORMAT_NODE *new_node;
    DWORD i;

    TRACE("Creating new cache entry\n");

    if (!(new_node = HeapAlloc(GetProcessHeap(), 0, sizeof(NLS_FORMAT_NODE))))
      return NULL;

    GET_LOCALE_NUMBER(new_node->dwCodePage, LOCALE_IDEFAULTANSICODEPAGE);

    /* Number Format */
    new_node->lcid = lcid;
    new_node->dwFlags = dwFlags;
    new_node->next = NULL;

    GET_LOCALE_NUMBER(new_node->fmt.NumDigits, LOCALE_IDIGITS);
    GET_LOCALE_NUMBER(new_node->fmt.LeadingZero, LOCALE_ILZERO);
    GET_LOCALE_NUMBER(new_node->fmt.NegativeOrder, LOCALE_INEGNUMBER);

    GET_LOCALE_NUMBER(new_node->fmt.Grouping, LOCALE_SGROUPING);
    if (new_node->fmt.Grouping > 9 && new_node->fmt.Grouping != 32)
    {
      WARN("LOCALE_SGROUPING (%d) unhandled, please report!\n",
           new_node->fmt.Grouping);
      new_node->fmt.Grouping = 0;
    }

    GET_LOCALE_STRING(new_node->fmt.lpDecimalSep, LOCALE_SDECIMAL);
    GET_LOCALE_STRING(new_node->fmt.lpThousandSep, LOCALE_STHOUSAND);

    /* Currency Format */
    new_node->cyfmt.NumDigits = new_node->fmt.NumDigits;
    new_node->cyfmt.LeadingZero = new_node->fmt.LeadingZero;

    GET_LOCALE_NUMBER(new_node->cyfmt.Grouping, LOCALE_SGROUPING);

    if (new_node->cyfmt.Grouping > 9)
    {
      WARN("LOCALE_SMONGROUPING (%d) unhandled, please report!\n",
           new_node->cyfmt.Grouping);
      new_node->cyfmt.Grouping = 0;
    }

    GET_LOCALE_NUMBER(new_node->cyfmt.NegativeOrder, LOCALE_INEGCURR);
    if (new_node->cyfmt.NegativeOrder > 15)
    {
      WARN("LOCALE_INEGCURR (%d) unhandled, please report!\n",
           new_node->cyfmt.NegativeOrder);
      new_node->cyfmt.NegativeOrder = 0;
    }
    GET_LOCALE_NUMBER(new_node->cyfmt.PositiveOrder, LOCALE_ICURRENCY);
    if (new_node->cyfmt.PositiveOrder > 3)
    {
      WARN("LOCALE_IPOSCURR (%d) unhandled,please report!\n",
           new_node->cyfmt.PositiveOrder);
      new_node->cyfmt.PositiveOrder = 0;
    }
    GET_LOCALE_STRING(new_node->cyfmt.lpDecimalSep, LOCALE_SMONDECIMALSEP);
    GET_LOCALE_STRING(new_node->cyfmt.lpThousandSep, LOCALE_SMONTHOUSANDSEP);
    GET_LOCALE_STRING(new_node->cyfmt.lpCurrencySymbol, LOCALE_SCURRENCY);

    /* Date/Time Format info, negative character, etc */
    for (i = 0; i < sizeof(NLS_LocaleIndices)/sizeof(NLS_LocaleIndices[0]); i++)
    {
      GET_LOCALE_STRING(new_node->lppszStrings[i], NLS_LocaleIndices[i]);
    }
    /* Save some memory if month genitive name is the same or not present */
    for (i = 0; i < 12; i++)
    {
      if (strcmpW(GetLongMonth(new_node, i), GetGenitiveMonth(new_node, i)) == 0)
      {
        HeapFree(GetProcessHeap(), 0, GetGenitiveMonth(new_node, i));
        GetGenitiveMonth(new_node, i) = NULL;
      }
    }

    new_node->szShortAM[0] = GetAM(new_node)[0]; new_node->szShortAM[1] = '\0';
    new_node->szShortPM[0] = GetPM(new_node)[0]; new_node->szShortPM[1] = '\0';

    /* Now add the computed format to the cache */
    RtlEnterCriticalSection(&NLS_FormatsCS);

    /* Search again: We may have raced to add the node */
    node = NLS_CachedFormats;
    while (node && (node->lcid != lcid || node->dwFlags != dwFlags) && node->next)
      node = node->next;

    if (!node)
    {
      node = NLS_CachedFormats = new_node; /* Empty list */
      new_node = NULL;
    }
    else if (node->lcid != lcid || node->dwFlags != dwFlags)
    {
      node->next = new_node; /* Not in the list, add to end */
      node = new_node;
      new_node = NULL;
    }

    RtlLeaveCriticalSection(&NLS_FormatsCS);

    if (new_node)
    {
      /* We raced and lost: The node was already added by another thread.
       * node points to the currently cached node, so free new_node.
       */
      for (i = 0; i < sizeof(NLS_LocaleIndices)/sizeof(NLS_LocaleIndices[0]); i++)
        HeapFree(GetProcessHeap(), 0, new_node->lppszStrings[i]);
      HeapFree(GetProcessHeap(), 0, new_node->fmt.lpDecimalSep);
      HeapFree(GetProcessHeap(), 0, new_node->fmt.lpThousandSep);
      HeapFree(GetProcessHeap(), 0, new_node->cyfmt.lpDecimalSep);
      HeapFree(GetProcessHeap(), 0, new_node->cyfmt.lpThousandSep);
      HeapFree(GetProcessHeap(), 0, new_node->cyfmt.lpCurrencySymbol);
      HeapFree(GetProcessHeap(), 0, new_node);
    }
  }
  return node;
}

/**************************************************************************
 * NLS_IsUnicodeOnlyLcid <internal>
 *
 * Determine if a locale is Unicode only, and thus invalid in ASCII calls.
 */
BOOL NLS_IsUnicodeOnlyLcid(LCID lcid)
{
  lcid = ConvertDefaultLocale(lcid);

  switch (PRIMARYLANGID(lcid))
  {
  case LANG_ARMENIAN:
  case LANG_DIVEHI:
  case LANG_GEORGIAN:
  case LANG_GUJARATI:
  case LANG_HINDI:
  case LANG_KANNADA:
  case LANG_KONKANI:
  case LANG_MARATHI:
  case LANG_PUNJABI:
  case LANG_SANSKRIT:
    TRACE("lcid 0x%08x: langid 0x%4x is Unicode Only\n", lcid, PRIMARYLANGID(lcid));
    return TRUE;
  default:
    return FALSE;
  }
}

/*
 * Formatting of dates, times, numbers and currencies.
 */

#define IsLiteralMarker(p) (p == '\'')
#define IsDateFmtChar(p)   (p == 'd'||p == 'M'||p == 'y'||p == 'g')
#define IsTimeFmtChar(p)   (p == 'H'||p == 'h'||p == 'm'||p == 's'||p == 't')

/* Only the following flags can be given if a date/time format is specified */
#define DATE_FORMAT_FLAGS (DATE_DATEVARSONLY)
#define TIME_FORMAT_FLAGS (TIME_TIMEVARSONLY|TIME_FORCE24HOURFORMAT| \
                           TIME_NOMINUTESORSECONDS|TIME_NOSECONDS| \
                           TIME_NOTIMEMARKER)

/******************************************************************************
 * NLS_GetDateTimeFormatW <internal>
 *
 * Performs the formatting for GetDateFormatW/GetTimeFormatW.
 *
 * FIXME
 * DATE_USE_ALT_CALENDAR           - Requires GetCalendarInfo to work first.
 * DATE_LTRREADING/DATE_RTLREADING - Not yet implemented.
 */
static INT NLS_GetDateTimeFormatW(LCID lcid, DWORD dwFlags,
                                  const SYSTEMTIME* lpTime, LPCWSTR lpFormat,
                                  LPWSTR lpStr, INT cchOut)
{
  const NLS_FORMAT_NODE *node;
  SYSTEMTIME st;
  INT cchWritten = 0;
  INT lastFormatPos = 0;
  BOOL bSkipping = FALSE; /* Skipping text around marker? */
  BOOL d_dd_formatted = FALSE; /* previous formatted part was for d or dd */

  /* Verify our arguments */
  if ((cchOut && !lpStr) || !(node = NLS_GetFormats(lcid, dwFlags)))
    goto invalid_parameter;

  if (dwFlags & ~(DATE_DATEVARSONLY|TIME_TIMEVARSONLY))
  {
    if (lpFormat &&
        ((dwFlags & DATE_DATEVARSONLY && dwFlags & ~DATE_FORMAT_FLAGS) ||
         (dwFlags & TIME_TIMEVARSONLY && dwFlags & ~TIME_FORMAT_FLAGS)))
    {
      goto invalid_flags;
    }

    if (dwFlags & DATE_DATEVARSONLY)
    {
      if ((dwFlags & (DATE_LTRREADING|DATE_RTLREADING)) == (DATE_LTRREADING|DATE_RTLREADING))
        goto invalid_flags;
      else if (dwFlags & (DATE_LTRREADING|DATE_RTLREADING))
        FIXME("Unsupported flags: DATE_LTRREADING/DATE_RTLREADING\n");

      switch (dwFlags & (DATE_SHORTDATE|DATE_LONGDATE|DATE_YEARMONTH))
      {
      case 0:
        break;
      case DATE_SHORTDATE:
      case DATE_LONGDATE:
      case DATE_YEARMONTH:
        if (lpFormat)
          goto invalid_flags;
        break;
      default:
        goto invalid_flags;
      }
    }
  }

  if (!lpFormat)
  {
    /* Use the appropriate default format */
    if (dwFlags & DATE_DATEVARSONLY)
    {
      if (dwFlags & DATE_YEARMONTH)
        lpFormat = GetYearMonth(node);
      else if (dwFlags & DATE_LONGDATE)
        lpFormat = GetLongDate(node);
      else
        lpFormat = GetShortDate(node);
    }
    else
      lpFormat = GetTime(node);
  }

  if (!lpTime)
  {
    GetLocalTime(&st); /* Default to current time */
    lpTime = &st;
  }
  else
  {
    if (dwFlags & DATE_DATEVARSONLY)
    {
      FILETIME ftTmp;

      /* Verify the date and correct the D.O.W. if needed */
      memset(&st, 0, sizeof(st));
      st.wYear = lpTime->wYear;
      st.wMonth = lpTime->wMonth;
      st.wDay = lpTime->wDay;

      if (st.wDay > 31 || st.wMonth > 12 || !SystemTimeToFileTime(&st, &ftTmp))
        goto invalid_parameter;

      FileTimeToSystemTime(&ftTmp, &st);
      lpTime = &st;
    }

    if (dwFlags & TIME_TIMEVARSONLY)
    {
      /* Verify the time */
      if (lpTime->wHour > 24 || lpTime->wMinute > 59 || lpTime->wSecond > 59)
        goto invalid_parameter;
    }
  }

  /* Format the output */
  while (*lpFormat)
  {
    if (IsLiteralMarker(*lpFormat))
    {
      /* Start of a literal string */
      lpFormat++;

      /* Loop until the end of the literal marker or end of the string */
      while (*lpFormat)
      {
        if (IsLiteralMarker(*lpFormat))
        {
          lpFormat++;
          if (!IsLiteralMarker(*lpFormat))
            break; /* Terminating literal marker */
        }

        if (!cchOut)
          cchWritten++;   /* Count size only */
        else if (cchWritten >= cchOut)
          goto overrun;
        else if (!bSkipping)
        {
          lpStr[cchWritten] = *lpFormat;
          cchWritten++;
        }
        lpFormat++;
      }
    }
    else if ((dwFlags & DATE_DATEVARSONLY && IsDateFmtChar(*lpFormat)) ||
             (dwFlags & TIME_TIMEVARSONLY && IsTimeFmtChar(*lpFormat)))
    {
      WCHAR buff[32], fmtChar;
      LPCWSTR szAdd = NULL;
      DWORD dwVal = 0;
      int   count = 0, dwLen;

      bSkipping = FALSE;

      fmtChar = *lpFormat;
      while (*lpFormat == fmtChar)
      {
        count++;
        lpFormat++;
      }
      buff[0] = '\0';

      if (fmtChar != 'M') d_dd_formatted = FALSE;
      switch(fmtChar)
      {
      case 'd':
        if (count >= 4)
          szAdd = GetLongDay(node, (lpTime->wDayOfWeek + 6) % 7);
        else if (count == 3)
          szAdd = GetShortDay(node, (lpTime->wDayOfWeek + 6) % 7);
        else
        {
          dwVal = lpTime->wDay;
          szAdd = buff;
          d_dd_formatted = TRUE;
        }
        break;

      case 'M':
        if (count >= 4)
        {
          LPCWSTR genitive = GetGenitiveMonth(node, lpTime->wMonth - 1);
          if (genitive)
          {
            if (d_dd_formatted)
            {
              szAdd = genitive;
              break;
            }
            else
            {
              LPCWSTR format = lpFormat;
              /* Look forward now, if next format pattern is for day genitive
                 name should be used */
              while (*format)
              {
                /* Skip parts within markers */
                if (IsLiteralMarker(*format))
                {
                  ++format;
                  while (*format)
                  {
                    if (IsLiteralMarker(*format))
                    {
                      ++format;
                      if (!IsLiteralMarker(*format)) break;
                    }
                  }
                }
                if (*format != ' ') break;
                ++format;
              }
              /* Only numeric day form matters */
              if (*format && *format == 'd')
              {
                INT dcount = 1;
                while (*++format == 'd') dcount++;
                if (dcount < 3)
                {
                  szAdd = genitive;
                  break;
                }
              }
            }
          }
          szAdd = GetLongMonth(node, lpTime->wMonth - 1);
        }
        else if (count == 3)
          szAdd = GetShortMonth(node, lpTime->wMonth - 1);
        else
        {
          dwVal = lpTime->wMonth;
          szAdd = buff;
        }
        break;

      case 'y':
        if (count >= 4)
        {
          count = 4;
          dwVal = lpTime->wYear;
        }
        else
        {
          count = count > 2 ? 2 : count;
          dwVal = lpTime->wYear % 100;
        }
        szAdd = buff;
        break;

      case 'g':
        if (count == 2)
        {
          /* FIXME: Our GetCalendarInfo() does not yet support CAL_SERASTRING.
           *        When it is fixed, this string should be cached in 'node'.
           */
          FIXME("Should be using GetCalendarInfo(CAL_SERASTRING), defaulting to 'AD'\n");
          buff[0] = 'A'; buff[1] = 'D'; buff[2] = '\0';
        }
        else
        {
          buff[0] = 'g'; buff[1] = '\0'; /* Add a literal 'g' */
        }
        szAdd = buff;
        break;

      case 'h':
        if (!(dwFlags & TIME_FORCE24HOURFORMAT))
        {
          count = count > 2 ? 2 : count;
          dwVal = lpTime->wHour == 0 ? 12 : (lpTime->wHour - 1) % 12 + 1;
          szAdd = buff;
          break;
        }
        /* .. fall through if we are forced to output in 24 hour format */

      case 'H':
        count = count > 2 ? 2 : count;
        dwVal = lpTime->wHour;
        szAdd = buff;
        break;

      case 'm':
        if (dwFlags & TIME_NOMINUTESORSECONDS)
        {
          cchWritten = lastFormatPos; /* Skip */
          bSkipping = TRUE;
        }
        else
        {
          count = count > 2 ? 2 : count;
          dwVal = lpTime->wMinute;
          szAdd = buff;
        }
        break;

      case 's':
        if (dwFlags & (TIME_NOSECONDS|TIME_NOMINUTESORSECONDS))
        {
          cchWritten = lastFormatPos; /* Skip */
          bSkipping = TRUE;
        }
        else
        {
          count = count > 2 ? 2 : count;
          dwVal = lpTime->wSecond;
          szAdd = buff;
        }
        break;

      case 't':
        if (dwFlags & TIME_NOTIMEMARKER)
        {
          cchWritten = lastFormatPos; /* Skip */
          bSkipping = TRUE;
        }
        else
        {
          if (count == 1)
            szAdd = lpTime->wHour < 12 ? node->szShortAM : node->szShortPM;
          else
            szAdd = lpTime->wHour < 12 ? GetAM(node) : GetPM(node);
        }
        break;
      }

      if (szAdd == buff && buff[0] == '\0')
      {
        static const WCHAR fmtW[] = {'%','.','*','d',0};
        /* We have a numeric value to add */
        snprintfW(buff, sizeof(buff)/sizeof(WCHAR), fmtW, count, dwVal);
      }

      dwLen = szAdd ? strlenW(szAdd) : 0;

      if (cchOut && dwLen)
      {
        if (cchWritten + dwLen < cchOut)
          memcpy(lpStr + cchWritten, szAdd, dwLen * sizeof(WCHAR));
        else
        {
          memcpy(lpStr + cchWritten, szAdd, (cchOut - cchWritten) * sizeof(WCHAR));
          goto overrun;
        }
      }
      cchWritten += dwLen;
      lastFormatPos = cchWritten; /* Save position of last output format text */
    }
    else
    {
      /* Literal character */
      if (!cchOut)
        cchWritten++;   /* Count size only */
      else if (cchWritten >= cchOut)
        goto overrun;
      else if (!bSkipping || *lpFormat == ' ')
      {
        lpStr[cchWritten] = *lpFormat;
        cchWritten++;
      }
      lpFormat++;
    }
  }

  /* Final string terminator and sanity check */
  if (cchOut)
  {
   if (cchWritten >= cchOut)
     goto overrun;
   else
     lpStr[cchWritten] = '\0';
  }
  cchWritten++; /* Include terminating NUL */

  TRACE("returning length=%d, ouput=%s\n", cchWritten, debugstr_w(lpStr));
  return cchWritten;

overrun:
  TRACE("returning 0, (ERROR_INSUFFICIENT_BUFFER)\n");
  SetLastError(ERROR_INSUFFICIENT_BUFFER);
  return 0;

invalid_parameter:
  SetLastError(ERROR_INVALID_PARAMETER);
  return 0;

invalid_flags:
  SetLastError(ERROR_INVALID_FLAGS);
  return 0;
}

/******************************************************************************
 * NLS_GetDateTimeFormatA <internal>
 *
 * ASCII wrapper for GetDateFormatA/GetTimeFormatA.
 */
static INT NLS_GetDateTimeFormatA(LCID lcid, DWORD dwFlags,
                                  const SYSTEMTIME* lpTime,
                                  LPCSTR lpFormat, LPSTR lpStr, INT cchOut)
{
  DWORD cp = CP_ACP;
  WCHAR szFormat[128], szOut[128];
  INT iRet;

  TRACE("(0x%04x,0x%08x,%p,%s,%p,%d)\n", lcid, dwFlags, lpTime,
        debugstr_a(lpFormat), lpStr, cchOut);

  if (NLS_IsUnicodeOnlyLcid(lcid))
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
  }

  if (!(dwFlags & LOCALE_USE_CP_ACP))
  {
    const NLS_FORMAT_NODE *node = NLS_GetFormats(lcid, dwFlags);
    if (!node)
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
    }

    cp = node->dwCodePage;
  }

  if (lpFormat)
    MultiByteToWideChar(cp, 0, lpFormat, -1, szFormat, sizeof(szFormat)/sizeof(WCHAR));

  if (cchOut > (int)(sizeof(szOut)/sizeof(WCHAR)))
    cchOut = sizeof(szOut)/sizeof(WCHAR);

  szOut[0] = '\0';

  iRet = NLS_GetDateTimeFormatW(lcid, dwFlags, lpTime, lpFormat ? szFormat : NULL,
                                lpStr ? szOut : NULL, cchOut);

  if (lpStr)
  {
    if (szOut[0])
      WideCharToMultiByte(cp, 0, szOut, iRet ? -1 : cchOut, lpStr, cchOut, 0, 0);
    else if (cchOut && iRet)
      *lpStr = '\0';
  }
  return iRet;
}

/******************************************************************************
 * GetDateFormatA [KERNEL32.@]
 *
 * Format a date for a given locale.
 *
 * PARAMS
 *  lcid      [I] Locale to format for
 *  dwFlags   [I] LOCALE_ and DATE_ flags from "winnls.h"
 *  lpTime    [I] Date to format
 *  lpFormat  [I] Format string, or NULL to use the system defaults
 *  lpDateStr [O] Destination for formatted string
 *  cchOut    [I] Size of lpDateStr, or 0 to calculate the resulting size
 *
 * NOTES
 *  - If lpFormat is NULL, lpDateStr will be formatted according to the format
 *    details returned by GetLocaleInfoA() and modified by dwFlags.
 *  - lpFormat is a string of characters and formatting tokens. Any characters
 *    in the string are copied verbatim to lpDateStr, with tokens being replaced
 *    by the date values they represent.
 *  - The following tokens have special meanings in a date format string:
 *|  Token  Meaning
 *|  -----  -------
 *|  d      Single digit day of the month (no leading 0)
 *|  dd     Double digit day of the month
 *|  ddd    Short name for the day of the week
 *|  dddd   Long name for the day of the week
 *|  M      Single digit month of the year (no leading 0)
 *|  MM     Double digit month of the year
 *|  MMM    Short name for the month of the year
 *|  MMMM   Long name for the month of the year
 *|  y      Double digit year number (no leading 0)
 *|  yy     Double digit year number
 *|  yyyy   Four digit year number
 *|  gg     Era string, for example 'AD'.
 *  - To output any literal character that could be misidentified as a token,
 *    enclose it in single quotes.
 *  - The Ascii version of this function fails if lcid is Unicode only.
 *
 * RETURNS
 *  Success: The number of character written to lpDateStr, or that would
 *           have been written, if cchOut is 0.
 *  Failure: 0. Use GetLastError() to determine the cause.
 */
INT WINAPI GetDateFormatA( LCID lcid, DWORD dwFlags, const SYSTEMTIME* lpTime,
                           LPCSTR lpFormat, LPSTR lpDateStr, INT cchOut)
{
  TRACE("(0x%04x,0x%08x,%p,%s,%p,%d)\n",lcid, dwFlags, lpTime,
        debugstr_a(lpFormat), lpDateStr, cchOut);

  return NLS_GetDateTimeFormatA(lcid, dwFlags | DATE_DATEVARSONLY, lpTime,
                                lpFormat, lpDateStr, cchOut);
}


/******************************************************************************
 * GetDateFormatW	[KERNEL32.@]
 *
 * See GetDateFormatA.
 */
INT WINAPI GetDateFormatW(LCID lcid, DWORD dwFlags, const SYSTEMTIME* lpTime,
                          LPCWSTR lpFormat, LPWSTR lpDateStr, INT cchOut)
{
  TRACE("(0x%04x,0x%08x,%p,%s,%p,%d)\n", lcid, dwFlags, lpTime,
        debugstr_w(lpFormat), lpDateStr, cchOut);

  return NLS_GetDateTimeFormatW(lcid, dwFlags|DATE_DATEVARSONLY, lpTime,
                                lpFormat, lpDateStr, cchOut);
}

/******************************************************************************
 *		GetTimeFormatA	[KERNEL32.@]
 *
 * Format a time for a given locale.
 *
 * PARAMS
 *  lcid      [I] Locale to format for
 *  dwFlags   [I] LOCALE_ and TIME_ flags from "winnls.h"
 *  lpTime    [I] Time to format
 *  lpFormat  [I] Formatting overrides
 *  lpTimeStr [O] Destination for formatted string
 *  cchOut    [I] Size of lpTimeStr, or 0 to calculate the resulting size
 *
 * NOTES
 *  - If lpFormat is NULL, lpszValue will be formatted according to the format
 *    details returned by GetLocaleInfoA() and modified by dwFlags.
 *  - lpFormat is a string of characters and formatting tokens. Any characters
 *    in the string are copied verbatim to lpTimeStr, with tokens being replaced
 *    by the time values they represent.
 *  - The following tokens have special meanings in a time format string:
 *|  Token  Meaning
 *|  -----  -------
 *|  h      Hours with no leading zero (12-hour clock)
 *|  hh     Hours with full two digits (12-hour clock)
 *|  H      Hours with no leading zero (24-hour clock)
 *|  HH     Hours with full two digits (24-hour clock)
 *|  m      Minutes with no leading zero
 *|  mm     Minutes with full two digits
 *|  s      Seconds with no leading zero
 *|  ss     Seconds with full two digits
 *|  t      Short time marker (e.g. "A" or "P")
 *|  tt     Long time marker (e.g. "AM", "PM")
 *  - To output any literal character that could be misidentified as a token,
 *    enclose it in single quotes.
 *  - The Ascii version of this function fails if lcid is Unicode only.
 *
 * RETURNS
 *  Success: The number of character written to lpTimeStr, or that would
 *           have been written, if cchOut is 0.
 *  Failure: 0. Use GetLastError() to determine the cause.
 */
INT WINAPI GetTimeFormatA(LCID lcid, DWORD dwFlags, const SYSTEMTIME* lpTime,
                          LPCSTR lpFormat, LPSTR lpTimeStr, INT cchOut)
{
  TRACE("(0x%04x,0x%08x,%p,%s,%p,%d)\n",lcid, dwFlags, lpTime,
        debugstr_a(lpFormat), lpTimeStr, cchOut);

  return NLS_GetDateTimeFormatA(lcid, dwFlags|TIME_TIMEVARSONLY, lpTime,
                                lpFormat, lpTimeStr, cchOut);
}

/******************************************************************************
 *		GetTimeFormatW	[KERNEL32.@]
 *
 * See GetTimeFormatA.
 */
INT WINAPI GetTimeFormatW(LCID lcid, DWORD dwFlags, const SYSTEMTIME* lpTime,
                          LPCWSTR lpFormat, LPWSTR lpTimeStr, INT cchOut)
{
  TRACE("(0x%04x,0x%08x,%p,%s,%p,%d)\n",lcid, dwFlags, lpTime,
        debugstr_w(lpFormat), lpTimeStr, cchOut);

  return NLS_GetDateTimeFormatW(lcid, dwFlags|TIME_TIMEVARSONLY, lpTime,
                                lpFormat, lpTimeStr, cchOut);
}

/**************************************************************************
 *              GetNumberFormatA	(KERNEL32.@)
 *
 * Format a number string for a given locale.
 *
 * PARAMS
 *  lcid        [I] Locale to format for
 *  dwFlags     [I] LOCALE_ flags from "winnls.h"
 *  lpszValue   [I] String to format
 *  lpFormat    [I] Formatting overrides
 *  lpNumberStr [O] Destination for formatted string
 *  cchOut      [I] Size of lpNumberStr, or 0 to calculate the resulting size
 *
 * NOTES
 *  - lpszValue can contain only '0' - '9', '-' and '.'.
 *  - If lpFormat is non-NULL, dwFlags must be 0. In this case lpszValue will
 *    be formatted according to the format details returned by GetLocaleInfoA().
 *  - This function rounds the number string if the number of decimals exceeds the
 *    locales normal number of decimal places.
 *  - If cchOut is 0, this function does not write to lpNumberStr.
 *  - The Ascii version of this function fails if lcid is Unicode only.
 *
 * RETURNS
 *  Success: The number of character written to lpNumberStr, or that would
 *           have been written, if cchOut is 0.
 *  Failure: 0. Use GetLastError() to determine the cause.
 */
INT WINAPI GetNumberFormatA(LCID lcid, DWORD dwFlags,
                            LPCSTR lpszValue,  const NUMBERFMTA *lpFormat,
                            LPSTR lpNumberStr, int cchOut)
{
  DWORD cp = CP_ACP;
  WCHAR szDec[8], szGrp[8], szIn[128], szOut[128];
  NUMBERFMTW fmt;
  const NUMBERFMTW *pfmt = NULL;
  INT iRet;

  TRACE("(0x%04x,0x%08x,%s,%p,%p,%d)\n", lcid, dwFlags, debugstr_a(lpszValue),
        lpFormat, lpNumberStr, cchOut);

  if (NLS_IsUnicodeOnlyLcid(lcid))
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
  }

  if (!(dwFlags & LOCALE_USE_CP_ACP))
  {
    const NLS_FORMAT_NODE *node = NLS_GetFormats(lcid, dwFlags);
    if (!node)
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
    }

    cp = node->dwCodePage;
  }

  if (lpFormat)
  {
    memcpy(&fmt, lpFormat, sizeof(fmt));
    pfmt = &fmt;
    if (lpFormat->lpDecimalSep)
    {
      MultiByteToWideChar(cp, 0, lpFormat->lpDecimalSep, -1, szDec, sizeof(szDec)/sizeof(WCHAR));
      fmt.lpDecimalSep = szDec;
    }
    if (lpFormat->lpThousandSep)
    {
      MultiByteToWideChar(cp, 0, lpFormat->lpThousandSep, -1, szGrp, sizeof(szGrp)/sizeof(WCHAR));
      fmt.lpThousandSep = szGrp;
    }
  }

  if (lpszValue)
    MultiByteToWideChar(cp, 0, lpszValue, -1, szIn, sizeof(szIn)/sizeof(WCHAR));

  if (cchOut > (int)(sizeof(szOut)/sizeof(WCHAR)))
    cchOut = sizeof(szOut)/sizeof(WCHAR);

  szOut[0] = '\0';

  iRet = GetNumberFormatW(lcid, dwFlags, lpszValue ? szIn : NULL, pfmt,
                          lpNumberStr ? szOut : NULL, cchOut);

  if (szOut[0] && lpNumberStr)
    WideCharToMultiByte(cp, 0, szOut, -1, lpNumberStr, cchOut, 0, 0);
  return iRet;
}

/* Number parsing state flags */
#define NF_ISNEGATIVE 0x1  /* '-' found */
#define NF_ISREAL     0x2  /* '.' found */
#define NF_DIGITS     0x4  /* '0'-'9' found */
#define NF_DIGITS_OUT 0x8  /* Digits before the '.' found */
#define NF_ROUND      0x10 /* Number needs to be rounded */

/* Formatting options for Numbers */
#define NLS_NEG_PARENS      0 /* "(1.1)" */
#define NLS_NEG_LEFT        1 /* "-1.1"  */
#define NLS_NEG_LEFT_SPACE  2 /* "- 1.1" */
#define NLS_NEG_RIGHT       3 /* "1.1-"  */
#define NLS_NEG_RIGHT_SPACE 4 /* "1.1 -" */

/**************************************************************************
 *              GetNumberFormatW	(KERNEL32.@)
 *
 * See GetNumberFormatA.
 */
INT WINAPI GetNumberFormatW(LCID lcid, DWORD dwFlags,
                            LPCWSTR lpszValue,  const NUMBERFMTW *lpFormat,
                            LPWSTR lpNumberStr, int cchOut)
{
  WCHAR szBuff[128], *szOut = szBuff + sizeof(szBuff) / sizeof(WCHAR) - 1;
  WCHAR szNegBuff[8];
  const WCHAR *lpszNeg = NULL, *lpszNegStart, *szSrc;
  DWORD dwState = 0, dwDecimals = 0, dwGroupCount = 0, dwCurrentGroupCount = 0;
  INT iRet;

  TRACE("(0x%04x,0x%08x,%s,%p,%p,%d)\n", lcid, dwFlags, debugstr_w(lpszValue),
        lpFormat, lpNumberStr, cchOut);

  if (!lpszValue || cchOut < 0 || (cchOut > 0 && !lpNumberStr) ||
      !IsValidLocale(lcid, 0) ||
      (lpFormat && (dwFlags || !lpFormat->lpDecimalSep || !lpFormat->lpThousandSep)))
  {
    goto error;
  }

  if (!lpFormat)
  {
    const NLS_FORMAT_NODE *node = NLS_GetFormats(lcid, dwFlags);

    if (!node)
      goto error;
    lpFormat = &node->fmt;
    lpszNegStart = lpszNeg = GetNegative(node);
  }
  else
  {
    GetLocaleInfoW(lcid, LOCALE_SNEGATIVESIGN|(dwFlags & LOCALE_NOUSEROVERRIDE),
                   szNegBuff, sizeof(szNegBuff)/sizeof(WCHAR));
    lpszNegStart = lpszNeg = szNegBuff;
  }
  lpszNeg = lpszNeg + strlenW(lpszNeg) - 1;

  dwFlags &= (LOCALE_NOUSEROVERRIDE|LOCALE_USE_CP_ACP);

  /* Format the number backwards into a temporary buffer */

  szSrc = lpszValue;
  *szOut-- = '\0';

  /* Check the number for validity */
  while (*szSrc)
  {
    if (*szSrc >= '0' && *szSrc <= '9')
    {
      dwState |= NF_DIGITS;
      if (dwState & NF_ISREAL)
        dwDecimals++;
    }
    else if (*szSrc == '-')
    {
      if (dwState)
        goto error; /* '-' not first character */
      dwState |= NF_ISNEGATIVE;
    }
    else if (*szSrc == '.')
    {
      if (dwState & NF_ISREAL)
        goto error; /* More than one '.' */
      dwState |= NF_ISREAL;
    }
    else
      goto error; /* Invalid char */
    szSrc++;
  }
  szSrc--; /* Point to last character */

  if (!(dwState & NF_DIGITS))
    goto error; /* No digits */

  /* Add any trailing negative sign */
  if (dwState & NF_ISNEGATIVE)
  {
    switch (lpFormat->NegativeOrder)
    {
    case NLS_NEG_PARENS:
      *szOut-- = ')';
      break;
    case NLS_NEG_RIGHT:
    case NLS_NEG_RIGHT_SPACE:
      while (lpszNeg >= lpszNegStart)
        *szOut-- = *lpszNeg--;
     if (lpFormat->NegativeOrder == NLS_NEG_RIGHT_SPACE)
       *szOut-- = ' ';
      break;
    }
  }

  /* Copy all digits up to the decimal point */
  if (!lpFormat->NumDigits)
  {
    if (dwState & NF_ISREAL)
    {
      while (*szSrc != '.') /* Don't write any decimals or a separator */
      {
        if (*szSrc >= '5' || (*szSrc == '4' && (dwState & NF_ROUND)))
          dwState |= NF_ROUND;
        else
          dwState &= ~NF_ROUND;
        szSrc--;
      }
      szSrc--;
    }
  }
  else
  {
    LPWSTR lpszDec = lpFormat->lpDecimalSep + strlenW(lpFormat->lpDecimalSep) - 1;

    if (dwDecimals <= lpFormat->NumDigits)
    {
      dwDecimals = lpFormat->NumDigits - dwDecimals;
      while (dwDecimals--)
        *szOut-- = '0'; /* Pad to correct number of dp */
    }
    else
    {
      dwDecimals -= lpFormat->NumDigits;
      /* Skip excess decimals, and determine if we have to round the number */
      while (dwDecimals--)
      {
        if (*szSrc >= '5' || (*szSrc == '4' && (dwState & NF_ROUND)))
          dwState |= NF_ROUND;
        else
          dwState &= ~NF_ROUND;
        szSrc--;
      }
    }

    if (dwState & NF_ISREAL)
    {
      while (*szSrc != '.')
      {
        if (dwState & NF_ROUND)
        {
          if (*szSrc == '9')
            *szOut-- = '0'; /* continue rounding */
          else
          {
            dwState &= ~NF_ROUND;
            *szOut-- = (*szSrc)+1;
          }
          szSrc--;
        }
        else
          *szOut-- = *szSrc--; /* Write existing decimals */
      }
      szSrc--; /* Skip '.' */
    }

    while (lpszDec >= lpFormat->lpDecimalSep)
      *szOut-- = *lpszDec--; /* Write decimal separator */
  }

  dwGroupCount = lpFormat->Grouping == 32 ? 3 : lpFormat->Grouping;

  /* Write the remaining whole number digits, including grouping chars */
  while (szSrc >= lpszValue && *szSrc >= '0' && *szSrc <= '9')
  {
    if (dwState & NF_ROUND)
    {
      if (*szSrc == '9')
        *szOut-- = '0'; /* continue rounding */
      else
      {
        dwState &= ~NF_ROUND;
        *szOut-- = (*szSrc)+1;
      }
      szSrc--;
    }
    else
      *szOut-- = *szSrc--;

    dwState |= NF_DIGITS_OUT;
    dwCurrentGroupCount++;
    if (szSrc >= lpszValue && dwCurrentGroupCount == dwGroupCount && *szSrc != '-')
    {
      LPWSTR lpszGrp = lpFormat->lpThousandSep + strlenW(lpFormat->lpThousandSep) - 1;

      while (lpszGrp >= lpFormat->lpThousandSep)
        *szOut-- = *lpszGrp--; /* Write grouping char */

      dwCurrentGroupCount = 0;
      if (lpFormat->Grouping == 32)
        dwGroupCount = 2; /* Indic grouping: 3 then 2 */
    }
  }
  if (dwState & NF_ROUND)
  {
    *szOut-- = '1'; /* e.g. .6 > 1.0 */
  }
  else if (!(dwState & NF_DIGITS_OUT) && lpFormat->LeadingZero)
    *szOut-- = '0'; /* Add leading 0 if we have no digits before the decimal point */

  /* Add any leading negative sign */
  if (dwState & NF_ISNEGATIVE)
  {
    switch (lpFormat->NegativeOrder)
    {
    case NLS_NEG_PARENS:
      *szOut-- = '(';
      break;
    case NLS_NEG_LEFT_SPACE:
      *szOut-- = ' ';
      /* Fall through */
    case NLS_NEG_LEFT:
      while (lpszNeg >= lpszNegStart)
        *szOut-- = *lpszNeg--;
      break;
    }
  }
  szOut++;

  iRet = strlenW(szOut) + 1;
  if (cchOut)
  {
    if (iRet <= cchOut)
      memcpy(lpNumberStr, szOut, iRet * sizeof(WCHAR));
    else
    {
      memcpy(lpNumberStr, szOut, cchOut * sizeof(WCHAR));
      lpNumberStr[cchOut - 1] = '\0';
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
      iRet = 0;
    }
  }
  return iRet;

error:
  SetLastError(lpFormat && dwFlags ? ERROR_INVALID_FLAGS : ERROR_INVALID_PARAMETER);
  return 0;
}

/**************************************************************************
 *              GetCurrencyFormatA	(KERNEL32.@)
 *
 * Format a currency string for a given locale.
 *
 * PARAMS
 *  lcid          [I] Locale to format for
 *  dwFlags       [I] LOCALE_ flags from "winnls.h"
 *  lpszValue     [I] String to format
 *  lpFormat      [I] Formatting overrides
 *  lpCurrencyStr [O] Destination for formatted string
 *  cchOut        [I] Size of lpCurrencyStr, or 0 to calculate the resulting size
 *
 * NOTES
 *  - lpszValue can contain only '0' - '9', '-' and '.'.
 *  - If lpFormat is non-NULL, dwFlags must be 0. In this case lpszValue will
 *    be formatted according to the format details returned by GetLocaleInfoA().
 *  - This function rounds the currency if the number of decimals exceeds the
 *    locales number of currency decimal places.
 *  - If cchOut is 0, this function does not write to lpCurrencyStr.
 *  - The Ascii version of this function fails if lcid is Unicode only.
 *
 * RETURNS
 *  Success: The number of character written to lpNumberStr, or that would
 *           have been written, if cchOut is 0.
 *  Failure: 0. Use GetLastError() to determine the cause.
 */
INT WINAPI GetCurrencyFormatA(LCID lcid, DWORD dwFlags,
                              LPCSTR lpszValue,  const CURRENCYFMTA *lpFormat,
                              LPSTR lpCurrencyStr, int cchOut)
{
  DWORD cp = CP_ACP;
  WCHAR szDec[8], szGrp[8], szCy[8], szIn[128], szOut[128];
  CURRENCYFMTW fmt;
  const CURRENCYFMTW *pfmt = NULL;
  INT iRet;

  TRACE("(0x%04x,0x%08x,%s,%p,%p,%d)\n", lcid, dwFlags, debugstr_a(lpszValue),
        lpFormat, lpCurrencyStr, cchOut);

  if (NLS_IsUnicodeOnlyLcid(lcid))
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
  }

  if (!(dwFlags & LOCALE_USE_CP_ACP))
  {
    const NLS_FORMAT_NODE *node = NLS_GetFormats(lcid, dwFlags);
    if (!node)
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
    }

    cp = node->dwCodePage;
  }

  if (lpFormat)
  {
    memcpy(&fmt, lpFormat, sizeof(fmt));
    pfmt = &fmt;
    if (lpFormat->lpDecimalSep)
    {
      MultiByteToWideChar(cp, 0, lpFormat->lpDecimalSep, -1, szDec, sizeof(szDec)/sizeof(WCHAR));
      fmt.lpDecimalSep = szDec;
    }
    if (lpFormat->lpThousandSep)
    {
      MultiByteToWideChar(cp, 0, lpFormat->lpThousandSep, -1, szGrp, sizeof(szGrp)/sizeof(WCHAR));
      fmt.lpThousandSep = szGrp;
    }
    if (lpFormat->lpCurrencySymbol)
    {
      MultiByteToWideChar(cp, 0, lpFormat->lpCurrencySymbol, -1, szCy, sizeof(szCy)/sizeof(WCHAR));
      fmt.lpCurrencySymbol = szCy;
    }
  }

  if (lpszValue)
    MultiByteToWideChar(cp, 0, lpszValue, -1, szIn, sizeof(szIn)/sizeof(WCHAR));

  if (cchOut > (int)(sizeof(szOut)/sizeof(WCHAR)))
    cchOut = sizeof(szOut)/sizeof(WCHAR);

  szOut[0] = '\0';

  iRet = GetCurrencyFormatW(lcid, dwFlags, lpszValue ? szIn : NULL, pfmt,
                            lpCurrencyStr ? szOut : NULL, cchOut);

  if (szOut[0] && lpCurrencyStr)
    WideCharToMultiByte(cp, 0, szOut, -1, lpCurrencyStr, cchOut, 0, 0);
  return iRet;
}

/* Formatting states for Currencies. We use flags to avoid code duplication. */
#define CF_PARENS       0x1  /* Parentheses      */
#define CF_MINUS_LEFT   0x2  /* '-' to the left  */
#define CF_MINUS_RIGHT  0x4  /* '-' to the right */
#define CF_MINUS_BEFORE 0x8  /* '-' before '$'   */
#define CF_CY_LEFT      0x10 /* '$' to the left  */
#define CF_CY_RIGHT     0x20 /* '$' to the right */
#define CF_CY_SPACE     0x40 /* ' ' by '$'       */

/**************************************************************************
 *              GetCurrencyFormatW	(KERNEL32.@)
 *
 * See GetCurrencyFormatA.
 */
INT WINAPI GetCurrencyFormatW(LCID lcid, DWORD dwFlags,
                              LPCWSTR lpszValue,  const CURRENCYFMTW *lpFormat,
                              LPWSTR lpCurrencyStr, int cchOut)
{
  static const BYTE NLS_NegCyFormats[16] =
  {
    CF_PARENS|CF_CY_LEFT,                       /* ($1.1) */
    CF_MINUS_LEFT|CF_MINUS_BEFORE|CF_CY_LEFT,   /* -$1.1  */
    CF_MINUS_LEFT|CF_CY_LEFT,                   /* $-1.1  */
    CF_MINUS_RIGHT|CF_CY_LEFT,                  /* $1.1-  */
    CF_PARENS|CF_CY_RIGHT,                      /* (1.1$) */
    CF_MINUS_LEFT|CF_CY_RIGHT,                  /* -1.1$  */
    CF_MINUS_RIGHT|CF_MINUS_BEFORE|CF_CY_RIGHT, /* 1.1-$  */
    CF_MINUS_RIGHT|CF_CY_RIGHT,                 /* 1.1$-  */
    CF_MINUS_LEFT|CF_CY_RIGHT|CF_CY_SPACE,      /* -1.1 $ */
    CF_MINUS_LEFT|CF_MINUS_BEFORE|CF_CY_LEFT|CF_CY_SPACE,   /* -$ 1.1 */
    CF_MINUS_RIGHT|CF_CY_RIGHT|CF_CY_SPACE,     /* 1.1 $-  */
    CF_MINUS_RIGHT|CF_CY_LEFT|CF_CY_SPACE,      /* $ 1.1-  */
    CF_MINUS_LEFT|CF_CY_LEFT|CF_CY_SPACE,       /* $ -1.1  */
    CF_MINUS_RIGHT|CF_MINUS_BEFORE|CF_CY_RIGHT|CF_CY_SPACE, /* 1.1- $ */
    CF_PARENS|CF_CY_LEFT|CF_CY_SPACE,           /* ($ 1.1) */
    CF_PARENS|CF_CY_RIGHT|CF_CY_SPACE,          /* (1.1 $) */
  };
  static const BYTE NLS_PosCyFormats[4] =
  {
    CF_CY_LEFT,              /* $1.1  */
    CF_CY_RIGHT,             /* 1.1$  */
    CF_CY_LEFT|CF_CY_SPACE,  /* $ 1.1 */
    CF_CY_RIGHT|CF_CY_SPACE, /* 1.1 $ */
  };
  WCHAR szBuff[128], *szOut = szBuff + sizeof(szBuff) / sizeof(WCHAR) - 1;
  WCHAR szNegBuff[8];
  const WCHAR *lpszNeg = NULL, *lpszNegStart, *szSrc, *lpszCy, *lpszCyStart;
  DWORD dwState = 0, dwDecimals = 0, dwGroupCount = 0, dwCurrentGroupCount = 0, dwFmt;
  INT iRet;

  TRACE("(0x%04x,0x%08x,%s,%p,%p,%d)\n", lcid, dwFlags, debugstr_w(lpszValue),
        lpFormat, lpCurrencyStr, cchOut);

  if (!lpszValue || cchOut < 0 || (cchOut > 0 && !lpCurrencyStr) ||
      !IsValidLocale(lcid, 0) ||
      (lpFormat && (dwFlags || !lpFormat->lpDecimalSep || !lpFormat->lpThousandSep ||
      !lpFormat->lpCurrencySymbol || lpFormat->NegativeOrder > 15 ||
      lpFormat->PositiveOrder > 3)))
  {
    goto error;
  }

  if (!lpFormat)
  {
    const NLS_FORMAT_NODE *node = NLS_GetFormats(lcid, dwFlags);

    if (!node)
      goto error;

    lpFormat = &node->cyfmt;
    lpszNegStart = lpszNeg = GetNegative(node);
  }
  else
  {
    GetLocaleInfoW(lcid, LOCALE_SNEGATIVESIGN|(dwFlags & LOCALE_NOUSEROVERRIDE),
                   szNegBuff, sizeof(szNegBuff)/sizeof(WCHAR));
    lpszNegStart = lpszNeg = szNegBuff;
  }
  dwFlags &= (LOCALE_NOUSEROVERRIDE|LOCALE_USE_CP_ACP);

  lpszNeg = lpszNeg + strlenW(lpszNeg) - 1;
  lpszCyStart = lpFormat->lpCurrencySymbol;
  lpszCy = lpszCyStart + strlenW(lpszCyStart) - 1;

  /* Format the currency backwards into a temporary buffer */

  szSrc = lpszValue;
  *szOut-- = '\0';

  /* Check the number for validity */
  while (*szSrc)
  {
    if (*szSrc >= '0' && *szSrc <= '9')
    {
      dwState |= NF_DIGITS;
      if (dwState & NF_ISREAL)
        dwDecimals++;
    }
    else if (*szSrc == '-')
    {
      if (dwState)
        goto error; /* '-' not first character */
      dwState |= NF_ISNEGATIVE;
    }
    else if (*szSrc == '.')
    {
      if (dwState & NF_ISREAL)
        goto error; /* More than one '.' */
      dwState |= NF_ISREAL;
    }
    else
      goto error; /* Invalid char */
    szSrc++;
  }
  szSrc--; /* Point to last character */

  if (!(dwState & NF_DIGITS))
    goto error; /* No digits */

  if (dwState & NF_ISNEGATIVE)
    dwFmt = NLS_NegCyFormats[lpFormat->NegativeOrder];
  else
    dwFmt = NLS_PosCyFormats[lpFormat->PositiveOrder];

  /* Add any trailing negative or currency signs */
  if (dwFmt & CF_PARENS)
    *szOut-- = ')';

  while (dwFmt & (CF_MINUS_RIGHT|CF_CY_RIGHT))
  {
    switch (dwFmt & (CF_MINUS_RIGHT|CF_MINUS_BEFORE|CF_CY_RIGHT))
    {
    case CF_MINUS_RIGHT:
    case CF_MINUS_RIGHT|CF_CY_RIGHT:
      while (lpszNeg >= lpszNegStart)
        *szOut-- = *lpszNeg--;
      dwFmt &= ~CF_MINUS_RIGHT;
      break;

    case CF_CY_RIGHT:
    case CF_MINUS_BEFORE|CF_CY_RIGHT:
    case CF_MINUS_RIGHT|CF_MINUS_BEFORE|CF_CY_RIGHT:
      while (lpszCy >= lpszCyStart)
        *szOut-- = *lpszCy--;
      if (dwFmt & CF_CY_SPACE)
        *szOut-- = ' ';
      dwFmt &= ~(CF_CY_RIGHT|CF_MINUS_BEFORE);
      break;
    }
  }

  /* Copy all digits up to the decimal point */
  if (!lpFormat->NumDigits)
  {
    if (dwState & NF_ISREAL)
    {
      while (*szSrc != '.') /* Don't write any decimals or a separator */
      {
        if (*szSrc >= '5' || (*szSrc == '4' && (dwState & NF_ROUND)))
          dwState |= NF_ROUND;
        else
          dwState &= ~NF_ROUND;
        szSrc--;
      }
      szSrc--;
    }
  }
  else
  {
    LPWSTR lpszDec = lpFormat->lpDecimalSep + strlenW(lpFormat->lpDecimalSep) - 1;

    if (dwDecimals <= lpFormat->NumDigits)
    {
      dwDecimals = lpFormat->NumDigits - dwDecimals;
      while (dwDecimals--)
        *szOut-- = '0'; /* Pad to correct number of dp */
    }
    else
    {
      dwDecimals -= lpFormat->NumDigits;
      /* Skip excess decimals, and determine if we have to round the number */
      while (dwDecimals--)
      {
        if (*szSrc >= '5' || (*szSrc == '4' && (dwState & NF_ROUND)))
          dwState |= NF_ROUND;
        else
          dwState &= ~NF_ROUND;
        szSrc--;
      }
    }

    if (dwState & NF_ISREAL)
    {
      while (*szSrc != '.')
      {
        if (dwState & NF_ROUND)
        {
          if (*szSrc == '9')
            *szOut-- = '0'; /* continue rounding */
          else
          {
            dwState &= ~NF_ROUND;
            *szOut-- = (*szSrc)+1;
          }
          szSrc--;
        }
        else
          *szOut-- = *szSrc--; /* Write existing decimals */
      }
      szSrc--; /* Skip '.' */
    }
    while (lpszDec >= lpFormat->lpDecimalSep)
      *szOut-- = *lpszDec--; /* Write decimal separator */
  }

  dwGroupCount = lpFormat->Grouping;

  /* Write the remaining whole number digits, including grouping chars */
  while (szSrc >= lpszValue && *szSrc >= '0' && *szSrc <= '9')
  {
    if (dwState & NF_ROUND)
    {
      if (*szSrc == '9')
        *szOut-- = '0'; /* continue rounding */
      else
      {
        dwState &= ~NF_ROUND;
        *szOut-- = (*szSrc)+1;
      }
      szSrc--;
    }
    else
      *szOut-- = *szSrc--;

    dwState |= NF_DIGITS_OUT;
    dwCurrentGroupCount++;
    if (szSrc >= lpszValue && dwCurrentGroupCount == dwGroupCount && *szSrc != '-')
    {
      LPWSTR lpszGrp = lpFormat->lpThousandSep + strlenW(lpFormat->lpThousandSep) - 1;

      while (lpszGrp >= lpFormat->lpThousandSep)
        *szOut-- = *lpszGrp--; /* Write grouping char */

      dwCurrentGroupCount = 0;
    }
  }
  if (dwState & NF_ROUND)
    *szOut-- = '1'; /* e.g. .6 > 1.0 */
  else if (!(dwState & NF_DIGITS_OUT) && lpFormat->LeadingZero)
    *szOut-- = '0'; /* Add leading 0 if we have no digits before the decimal point */

  /* Add any leading negative or currency sign */
  while (dwFmt & (CF_MINUS_LEFT|CF_CY_LEFT))
  {
    switch (dwFmt & (CF_MINUS_LEFT|CF_MINUS_BEFORE|CF_CY_LEFT))
    {
    case CF_MINUS_LEFT:
    case CF_MINUS_LEFT|CF_CY_LEFT:
      while (lpszNeg >= lpszNegStart)
        *szOut-- = *lpszNeg--;
      dwFmt &= ~CF_MINUS_LEFT;
      break;

    case CF_CY_LEFT:
    case CF_CY_LEFT|CF_MINUS_BEFORE:
    case CF_MINUS_LEFT|CF_MINUS_BEFORE|CF_CY_LEFT:
      if (dwFmt & CF_CY_SPACE)
        *szOut-- = ' ';
      while (lpszCy >= lpszCyStart)
        *szOut-- = *lpszCy--;
      dwFmt &= ~(CF_CY_LEFT|CF_MINUS_BEFORE);
      break;
    }
  }
  if (dwFmt & CF_PARENS)
    *szOut-- = '(';
  szOut++;

  iRet = strlenW(szOut) + 1;
  if (cchOut)
  {
    if (iRet <= cchOut)
      memcpy(lpCurrencyStr, szOut, iRet * sizeof(WCHAR));
    else
    {
      memcpy(lpCurrencyStr, szOut, cchOut * sizeof(WCHAR));
      lpCurrencyStr[cchOut - 1] = '\0';
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
      iRet = 0;
    }
  }
  return iRet;

error:
  SetLastError(lpFormat && dwFlags ? ERROR_INVALID_FLAGS : ERROR_INVALID_PARAMETER);
  return 0;
}

/* FIXME: Everything below here needs to move somewhere else along with the
 *        other EnumXXX functions, when a method for storing resources for
 *        alternate calendars is determined.
 */

/**************************************************************************
 *              EnumDateFormatsExA    (KERNEL32.@)
 *
 * FIXME: MSDN mentions only LOCALE_USE_CP_ACP, should we handle
 * LOCALE_NOUSEROVERRIDE here as well?
 */
BOOL WINAPI EnumDateFormatsExA(DATEFMT_ENUMPROCEXA proc, LCID lcid, DWORD flags)
{
    CALID cal_id;
    char buf[256];

    if (!proc)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!GetLocaleInfoW(lcid, LOCALE_ICALENDARTYPE|LOCALE_RETURN_NUMBER, (LPWSTR)&cal_id, sizeof(cal_id)/sizeof(WCHAR)))
        return FALSE;

    switch (flags & ~LOCALE_USE_CP_ACP)
    {
    case 0:
    case DATE_SHORTDATE:
        if (GetLocaleInfoA(lcid, LOCALE_SSHORTDATE | (flags & LOCALE_USE_CP_ACP), buf, 256))
            proc(buf, cal_id);
        break;

    case DATE_LONGDATE:
        if (GetLocaleInfoA(lcid, LOCALE_SLONGDATE | (flags & LOCALE_USE_CP_ACP), buf, 256))
            proc(buf, cal_id);
        break;

    case DATE_YEARMONTH:
        if (GetLocaleInfoA(lcid, LOCALE_SYEARMONTH | (flags & LOCALE_USE_CP_ACP), buf, 256))
            proc(buf, cal_id);
        break;

    default:
        FIXME("Unknown date format (%d)\n", flags);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return TRUE;
}

/**************************************************************************
 *              EnumDateFormatsExW    (KERNEL32.@)
 */
BOOL WINAPI EnumDateFormatsExW(DATEFMT_ENUMPROCEXW proc, LCID lcid, DWORD flags)
{
    CALID cal_id;
    WCHAR buf[256];

    if (!proc)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!GetLocaleInfoW(lcid, LOCALE_ICALENDARTYPE|LOCALE_RETURN_NUMBER, (LPWSTR)&cal_id, sizeof(cal_id)/sizeof(WCHAR)))
        return FALSE;

    switch (flags & ~LOCALE_USE_CP_ACP)
    {
    case 0:
    case DATE_SHORTDATE:
        if (GetLocaleInfoW(lcid, LOCALE_SSHORTDATE | (flags & LOCALE_USE_CP_ACP), buf, 256))
            proc(buf, cal_id);
        break;

    case DATE_LONGDATE:
        if (GetLocaleInfoW(lcid, LOCALE_SLONGDATE | (flags & LOCALE_USE_CP_ACP), buf, 256))
            proc(buf, cal_id);
        break;

    case DATE_YEARMONTH:
        if (GetLocaleInfoW(lcid, LOCALE_SYEARMONTH | (flags & LOCALE_USE_CP_ACP), buf, 256))
            proc(buf, cal_id);
        break;

    default:
        FIXME("Unknown date format (%d)\n", flags);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return TRUE;
}

/**************************************************************************
 *              EnumDateFormatsA	(KERNEL32.@)
 *
 * FIXME: MSDN mentions only LOCALE_USE_CP_ACP, should we handle
 * LOCALE_NOUSEROVERRIDE here as well?
 */
BOOL WINAPI EnumDateFormatsA(DATEFMT_ENUMPROCA proc, LCID lcid, DWORD flags)
{
    char buf[256];

    if (!proc)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    switch (flags & ~LOCALE_USE_CP_ACP)
    {
    case 0:
    case DATE_SHORTDATE:
        if (GetLocaleInfoA(lcid, LOCALE_SSHORTDATE | (flags & LOCALE_USE_CP_ACP), buf, 256))
            proc(buf);
        break;

    case DATE_LONGDATE:
        if (GetLocaleInfoA(lcid, LOCALE_SLONGDATE | (flags & LOCALE_USE_CP_ACP), buf, 256))
            proc(buf);
        break;

    case DATE_YEARMONTH:
        if (GetLocaleInfoA(lcid, LOCALE_SYEARMONTH | (flags & LOCALE_USE_CP_ACP), buf, 256))
            proc(buf);
        break;

    default:
        FIXME("Unknown date format (%d)\n", flags);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return TRUE;
}

/**************************************************************************
 *              EnumDateFormatsW	(KERNEL32.@)
 */
BOOL WINAPI EnumDateFormatsW(DATEFMT_ENUMPROCW proc, LCID lcid, DWORD flags)
{
    WCHAR buf[256];

    if (!proc)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    switch (flags & ~LOCALE_USE_CP_ACP)
    {
    case 0:
    case DATE_SHORTDATE:
        if (GetLocaleInfoW(lcid, LOCALE_SSHORTDATE | (flags & LOCALE_USE_CP_ACP), buf, 256))
            proc(buf);
        break;

    case DATE_LONGDATE:
        if (GetLocaleInfoW(lcid, LOCALE_SLONGDATE | (flags & LOCALE_USE_CP_ACP), buf, 256))
            proc(buf);
        break;

    case DATE_YEARMONTH:
        if (GetLocaleInfoW(lcid, LOCALE_SYEARMONTH | (flags & LOCALE_USE_CP_ACP), buf, 256))
            proc(buf);
        break;

    default:
        FIXME("Unknown date format (%d)\n", flags);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return TRUE;
}

/**************************************************************************
 *              EnumTimeFormatsA	(KERNEL32.@)
 *
 * FIXME: MSDN mentions only LOCALE_USE_CP_ACP, should we handle
 * LOCALE_NOUSEROVERRIDE here as well?
 */
BOOL WINAPI EnumTimeFormatsA(TIMEFMT_ENUMPROCA proc, LCID lcid, DWORD flags)
{
    char buf[256];

    if (!proc)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    switch (flags & ~LOCALE_USE_CP_ACP)
    {
    case 0:
        if (GetLocaleInfoA(lcid, LOCALE_STIMEFORMAT | (flags & LOCALE_USE_CP_ACP), buf, 256))
            proc(buf);
        break;

    default:
        FIXME("Unknown time format (%d)\n", flags);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return TRUE;
}

/**************************************************************************
 *              EnumTimeFormatsW	(KERNEL32.@)
 */
BOOL WINAPI EnumTimeFormatsW(TIMEFMT_ENUMPROCW proc, LCID lcid, DWORD flags)
{
    WCHAR buf[256];

    if (!proc)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    switch (flags & ~LOCALE_USE_CP_ACP)
    {
    case 0:
        if (GetLocaleInfoW(lcid, LOCALE_STIMEFORMAT | (flags & LOCALE_USE_CP_ACP), buf, 256))
            proc(buf);
        break;

    default:
        FIXME("Unknown time format (%d)\n", flags);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return TRUE;
}

/******************************************************************************
 * NLS_EnumCalendarInfoAW <internal>
 * Enumerates calendar information for a specified locale.
 *
 * PARAMS
 *    calinfoproc [I] Pointer to the callback
 *    locale      [I] The locale for which to retrieve calendar information.
 *                    This parameter can be a locale identifier created by the
 *                    MAKELCID macro, or one of the following values:
 *                        LOCALE_SYSTEM_DEFAULT
 *                            Use the default system locale.
 *                        LOCALE_USER_DEFAULT
 *                            Use the default user locale.
 *    calendar    [I] The calendar for which information is requested, or
 *                    ENUM_ALL_CALENDARS.
 *    caltype     [I] The type of calendar information to be returned. Note
 *                    that only one CALTYPE value can be specified per call
 *                    of this function, except where noted.
 *    unicode     [I] Specifies if the callback expects a unicode string.
 *    ex          [I] Specifies if the callback needs the calendar identifier.
 *
 * RETURNS
 *    Success: TRUE.
 *    Failure: FALSE. Use GetLastError() to determine the cause.
 *
 * NOTES
 *    When the ANSI version of this function is used with a Unicode-only LCID,
 *    the call can succeed because the system uses the system code page.
 *    However, characters that are undefined in the system code page appear
 *    in the string as a question mark (?).
 *
 * TODO
 *    The above note should be respected by GetCalendarInfoA.
 */
static BOOL NLS_EnumCalendarInfoAW(void *calinfoproc, LCID locale,
                  CALID calendar, CALTYPE caltype, BOOL unicode, BOOL ex )
{
  WCHAR *buf, *opt = NULL, *iter = NULL;
  BOOL ret = FALSE;
  int bufSz = 200;		/* the size of the buffer */

  if (calinfoproc == NULL)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  buf = HeapAlloc(GetProcessHeap(), 0, bufSz);
  if (buf == NULL)
  {
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }

  if (calendar == ENUM_ALL_CALENDARS)
  {
    int optSz = GetLocaleInfoW(locale, LOCALE_IOPTIONALCALENDAR, NULL, 0);
    if (optSz > 1)
    {
      opt = HeapAlloc(GetProcessHeap(), 0, optSz * sizeof(WCHAR));
      if (opt == NULL)
      {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup;
      }
      if (GetLocaleInfoW(locale, LOCALE_IOPTIONALCALENDAR, opt, optSz))
        iter = opt;
    }
    calendar = NLS_GetLocaleNumber(locale, LOCALE_ICALENDARTYPE);
  }

  while (TRUE)			/* loop through calendars */
  {
    do				/* loop until there's no error */
    {
      if (unicode)
        ret = GetCalendarInfoW(locale, calendar, caltype, buf, bufSz / sizeof(WCHAR), NULL);
      else ret = GetCalendarInfoA(locale, calendar, caltype, (CHAR*)buf, bufSz / sizeof(CHAR), NULL);

      if (!ret)
      {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {				/* so resize it */
          int newSz;
          if (unicode)
            newSz = GetCalendarInfoW(locale, calendar, caltype, NULL, 0, NULL) * sizeof(WCHAR);
          else newSz = GetCalendarInfoA(locale, calendar, caltype, NULL, 0, NULL) * sizeof(CHAR);
          if (bufSz >= newSz)
          {
            ERR("Buffer resizing disorder: was %d, requested %d.\n", bufSz, newSz);
            goto cleanup;
          }
          bufSz = newSz;
          WARN("Buffer too small; resizing to %d bytes.\n", bufSz);
          buf = HeapReAlloc(GetProcessHeap(), 0, buf, bufSz);
          if (buf == NULL)
            goto cleanup;
        } else goto cleanup;
      }
    } while (!ret);

    /* Here we are. We pass the buffer to the correct version of
     * the callback. Because it's not the same number of params,
     * we must check for Ex, but we don't care about Unicode
     * because the buffer is already in the correct format.
     */
    if (ex) {
      ret = ((CALINFO_ENUMPROCEXW)calinfoproc)(buf, calendar);
    } else
      ret = ((CALINFO_ENUMPROCW)calinfoproc)(buf);

    if (!ret) {			/* the callback told to stop */
      ret = TRUE;
      break;
    }

    if ((iter == NULL) || (*iter == 0))	/* no more calendars */
      break;

    calendar = 0;
    while ((*iter >= '0') && (*iter <= '9'))
      calendar = calendar * 10 + *iter++ - '0';

    if (*iter++ != 0)
    {
      SetLastError(ERROR_BADDB);
      ret = FALSE;
      break;
    }
  }

cleanup:
  HeapFree(GetProcessHeap(), 0, opt);
  HeapFree(GetProcessHeap(), 0, buf);
  return ret;
}

/******************************************************************************
 *		EnumCalendarInfoA	[KERNEL32.@]
 *
 * See EnumCalendarInfoAW.
 */
BOOL WINAPI EnumCalendarInfoA( CALINFO_ENUMPROCA calinfoproc,LCID locale,
                               CALID calendar,CALTYPE caltype )
{
  TRACE("(%p,0x%08x,0x%08x,0x%08x)\n", calinfoproc, locale, calendar, caltype);
  return NLS_EnumCalendarInfoAW(calinfoproc, locale, calendar, caltype, FALSE, FALSE);
}

/******************************************************************************
 *		EnumCalendarInfoW	[KERNEL32.@]
 *
 * See EnumCalendarInfoAW.
 */
BOOL WINAPI EnumCalendarInfoW( CALINFO_ENUMPROCW calinfoproc,LCID locale,
                               CALID calendar,CALTYPE caltype )
{
  TRACE("(%p,0x%08x,0x%08x,0x%08x)\n", calinfoproc, locale, calendar, caltype);
  return NLS_EnumCalendarInfoAW(calinfoproc, locale, calendar, caltype, TRUE, FALSE);
}

/******************************************************************************
 *		EnumCalendarInfoExA	[KERNEL32.@]
 *
 * See EnumCalendarInfoAW.
 */
BOOL WINAPI EnumCalendarInfoExA( CALINFO_ENUMPROCEXA calinfoproc,LCID locale,
                                 CALID calendar,CALTYPE caltype )
{
  TRACE("(%p,0x%08x,0x%08x,0x%08x)\n", calinfoproc, locale, calendar, caltype);
  return NLS_EnumCalendarInfoAW(calinfoproc, locale, calendar, caltype, FALSE, TRUE);
}

/******************************************************************************
 *		EnumCalendarInfoExW	[KERNEL32.@]
 *
 * See EnumCalendarInfoAW.
 */
BOOL WINAPI EnumCalendarInfoExW( CALINFO_ENUMPROCEXW calinfoproc,LCID locale,
                                 CALID calendar,CALTYPE caltype )
{
  TRACE("(%p,0x%08x,0x%08x,0x%08x)\n", calinfoproc, locale, calendar, caltype);
  return NLS_EnumCalendarInfoAW(calinfoproc, locale, calendar, caltype, TRUE, TRUE);
}

/*********************************************************************
 *	GetCalendarInfoA				(KERNEL32.@)
 *
 */
int WINAPI GetCalendarInfoA(LCID lcid, CALID Calendar, CALTYPE CalType,
			    LPSTR lpCalData, int cchData, LPDWORD lpValue)
{
    int ret;
    LPWSTR lpCalDataW = NULL;

    if (NLS_IsUnicodeOnlyLcid(lcid))
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
    }

    if (cchData &&
        !(lpCalDataW = HeapAlloc(GetProcessHeap(), 0, cchData*sizeof(WCHAR))))
      return 0;

    ret = GetCalendarInfoW(lcid, Calendar, CalType, lpCalDataW, cchData, lpValue);
    if(ret && lpCalDataW && lpCalData)
      WideCharToMultiByte(CP_ACP, 0, lpCalDataW, -1, lpCalData, cchData, NULL, NULL);
    else if (CalType & CAL_RETURN_NUMBER)
        ret *= sizeof(WCHAR);
    HeapFree(GetProcessHeap(), 0, lpCalDataW);

    return ret;
}

/*********************************************************************
 *	GetCalendarInfoW				(KERNEL32.@)
 *
 */
int WINAPI GetCalendarInfoW(LCID Locale, CALID Calendar, CALTYPE CalType,
			    LPWSTR lpCalData, int cchData, LPDWORD lpValue)
{
    if (CalType & CAL_NOUSEROVERRIDE)
	FIXME("flag CAL_NOUSEROVERRIDE used, not fully implemented\n");
    if (CalType & CAL_USE_CP_ACP)
	FIXME("flag CAL_USE_CP_ACP used, not fully implemented\n");

    if (CalType & CAL_RETURN_NUMBER) {
        if (!lpValue)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
	if (lpCalData != NULL)
	    WARN("lpCalData not NULL (%p) when it should!\n", lpCalData);
	if (cchData != 0)
	    WARN("cchData not 0 (%d) when it should!\n", cchData);
    } else {
	if (lpValue != NULL)
	    WARN("lpValue not NULL (%p) when it should!\n", lpValue);
    }

    /* FIXME: No verification is made yet wrt Locale
     * for the CALTYPES not requiring GetLocaleInfoA */
    switch (CalType & ~(CAL_NOUSEROVERRIDE|CAL_RETURN_NUMBER|CAL_USE_CP_ACP)) {
	case CAL_ICALINTVALUE:
            if (CalType & CAL_RETURN_NUMBER)
                return GetLocaleInfoW(Locale, LOCALE_RETURN_NUMBER | LOCALE_ICALENDARTYPE,
                        (LPWSTR)lpValue, 2);
            return GetLocaleInfoW(Locale, LOCALE_ICALENDARTYPE, lpCalData, cchData);
	case CAL_SCALNAME:
            FIXME("Unimplemented caltype %d\n", CalType & 0xffff);
            if (lpCalData) *lpCalData = 0;
	    return 1;
	case CAL_IYEAROFFSETRANGE:
            FIXME("Unimplemented caltype %d\n", CalType & 0xffff);
	    return 0;
	case CAL_SERASTRING:
            FIXME("Unimplemented caltype %d\n", CalType & 0xffff);
	    return 0;
	case CAL_SSHORTDATE:
	    return GetLocaleInfoW(Locale, LOCALE_SSHORTDATE, lpCalData, cchData);
	case CAL_SLONGDATE:
	    return GetLocaleInfoW(Locale, LOCALE_SLONGDATE, lpCalData, cchData);
	case CAL_SDAYNAME1:
	    return GetLocaleInfoW(Locale, LOCALE_SDAYNAME1, lpCalData, cchData);
	case CAL_SDAYNAME2:
	    return GetLocaleInfoW(Locale, LOCALE_SDAYNAME2, lpCalData, cchData);
	case CAL_SDAYNAME3:
	    return GetLocaleInfoW(Locale, LOCALE_SDAYNAME3, lpCalData, cchData);
	case CAL_SDAYNAME4:
	    return GetLocaleInfoW(Locale, LOCALE_SDAYNAME4, lpCalData, cchData);
	case CAL_SDAYNAME5:
	    return GetLocaleInfoW(Locale, LOCALE_SDAYNAME5, lpCalData, cchData);
	case CAL_SDAYNAME6:
	    return GetLocaleInfoW(Locale, LOCALE_SDAYNAME6, lpCalData, cchData);
	case CAL_SDAYNAME7:
	    return GetLocaleInfoW(Locale, LOCALE_SDAYNAME7, lpCalData, cchData);
	case CAL_SABBREVDAYNAME1:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVDAYNAME1, lpCalData, cchData);
	case CAL_SABBREVDAYNAME2:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVDAYNAME2, lpCalData, cchData);
	case CAL_SABBREVDAYNAME3:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVDAYNAME3, lpCalData, cchData);
	case CAL_SABBREVDAYNAME4:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVDAYNAME4, lpCalData, cchData);
	case CAL_SABBREVDAYNAME5:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVDAYNAME5, lpCalData, cchData);
	case CAL_SABBREVDAYNAME6:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVDAYNAME6, lpCalData, cchData);
	case CAL_SABBREVDAYNAME7:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVDAYNAME7, lpCalData, cchData);
	case CAL_SMONTHNAME1:
	    return GetLocaleInfoW(Locale, LOCALE_SMONTHNAME1, lpCalData, cchData);
	case CAL_SMONTHNAME2:
	    return GetLocaleInfoW(Locale, LOCALE_SMONTHNAME2, lpCalData, cchData);
	case CAL_SMONTHNAME3:
	    return GetLocaleInfoW(Locale, LOCALE_SMONTHNAME3, lpCalData, cchData);
	case CAL_SMONTHNAME4:
	    return GetLocaleInfoW(Locale, LOCALE_SMONTHNAME4, lpCalData, cchData);
	case CAL_SMONTHNAME5:
	    return GetLocaleInfoW(Locale, LOCALE_SMONTHNAME5, lpCalData, cchData);
	case CAL_SMONTHNAME6:
	    return GetLocaleInfoW(Locale, LOCALE_SMONTHNAME6, lpCalData, cchData);
	case CAL_SMONTHNAME7:
	    return GetLocaleInfoW(Locale, LOCALE_SMONTHNAME7, lpCalData, cchData);
	case CAL_SMONTHNAME8:
	    return GetLocaleInfoW(Locale, LOCALE_SMONTHNAME8, lpCalData, cchData);
	case CAL_SMONTHNAME9:
	    return GetLocaleInfoW(Locale, LOCALE_SMONTHNAME9, lpCalData, cchData);
	case CAL_SMONTHNAME10:
	    return GetLocaleInfoW(Locale, LOCALE_SMONTHNAME10, lpCalData, cchData);
	case CAL_SMONTHNAME11:
	    return GetLocaleInfoW(Locale, LOCALE_SMONTHNAME11, lpCalData, cchData);
	case CAL_SMONTHNAME12:
	    return GetLocaleInfoW(Locale, LOCALE_SMONTHNAME12, lpCalData, cchData);
	case CAL_SMONTHNAME13:
	    return GetLocaleInfoW(Locale, LOCALE_SMONTHNAME13, lpCalData, cchData);
	case CAL_SABBREVMONTHNAME1:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVMONTHNAME1, lpCalData, cchData);
	case CAL_SABBREVMONTHNAME2:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVMONTHNAME2, lpCalData, cchData);
	case CAL_SABBREVMONTHNAME3:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVMONTHNAME3, lpCalData, cchData);
	case CAL_SABBREVMONTHNAME4:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVMONTHNAME4, lpCalData, cchData);
	case CAL_SABBREVMONTHNAME5:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVMONTHNAME5, lpCalData, cchData);
	case CAL_SABBREVMONTHNAME6:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVMONTHNAME6, lpCalData, cchData);
	case CAL_SABBREVMONTHNAME7:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVMONTHNAME7, lpCalData, cchData);
	case CAL_SABBREVMONTHNAME8:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVMONTHNAME8, lpCalData, cchData);
	case CAL_SABBREVMONTHNAME9:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVMONTHNAME9, lpCalData, cchData);
	case CAL_SABBREVMONTHNAME10:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVMONTHNAME10, lpCalData, cchData);
	case CAL_SABBREVMONTHNAME11:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVMONTHNAME11, lpCalData, cchData);
	case CAL_SABBREVMONTHNAME12:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVMONTHNAME12, lpCalData, cchData);
	case CAL_SABBREVMONTHNAME13:
	    return GetLocaleInfoW(Locale, LOCALE_SABBREVMONTHNAME13, lpCalData, cchData);
	case CAL_SYEARMONTH:
	    return GetLocaleInfoW(Locale, LOCALE_SYEARMONTH, lpCalData, cchData);
	case CAL_ITWODIGITYEARMAX:
            if (CalType & CAL_RETURN_NUMBER)
            {
                *lpValue = CALINFO_MAX_YEAR;
                return sizeof(DWORD) / sizeof(WCHAR);
            }
            else
            {
                static const WCHAR fmtW[] = {'%','u',0};
                WCHAR buffer[10];
                int ret = snprintfW( buffer, 10, fmtW, CALINFO_MAX_YEAR  ) + 1;
                if (!lpCalData) return ret;
                if (ret <= cchData)
                {
                    strcpyW( lpCalData, buffer );
                    return ret;
                }
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
                return 0;
            }
	    break;
	default:
            FIXME("Unknown caltype %d\n",CalType & 0xffff);
            SetLastError(ERROR_INVALID_FLAGS);
            return 0;
    }
    return 0;
}

/*********************************************************************
 *	SetCalendarInfoA				(KERNEL32.@)
 *
 */
int WINAPI	SetCalendarInfoA(LCID Locale, CALID Calendar, CALTYPE CalType, LPCSTR lpCalData)
{
    FIXME("(%08x,%08x,%08x,%s): stub\n",
	  Locale, Calendar, CalType, debugstr_a(lpCalData));
    return 0;
}

/*********************************************************************
 *	SetCalendarInfoW				(KERNEL32.@)
 *
 *
 */
int WINAPI	SetCalendarInfoW(LCID Locale, CALID Calendar, CALTYPE CalType, LPCWSTR lpCalData)
{
    FIXME("(%08x,%08x,%08x,%s): stub\n",
	  Locale, Calendar, CalType, debugstr_w(lpCalData));
    return 0;
}

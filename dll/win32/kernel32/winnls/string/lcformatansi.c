/*
 * Locale-dependent format handling
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1998 David Lee Lambert
 * Copyright 2000 Julio Cesar Gazquez
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

#include "lcformat_private.h"

#ifndef CAL_SABBREVERASTRING
#define CAL_SABBREVERASTRING 0x00000039
#endif

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

/**************************************************************************
 *              GetNumberFormatA	(KERNEL32.@)
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
    cp = NLS_GetAnsiCodePage(lcid, dwFlags);
    if (!cp)
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
    }
  }

  if (lpFormat)
  {
    memcpy(&fmt, lpFormat, sizeof(fmt));
    pfmt = &fmt;
    if (lpFormat->lpDecimalSep)
    {
      MultiByteToWideChar(cp, 0, lpFormat->lpDecimalSep, -1, szDec, ARRAY_SIZE(szDec));
      fmt.lpDecimalSep = szDec;
    }
    if (lpFormat->lpThousandSep)
    {
      MultiByteToWideChar(cp, 0, lpFormat->lpThousandSep, -1, szGrp, ARRAY_SIZE(szGrp));
      fmt.lpThousandSep = szGrp;
    }
  }

  if (lpszValue)
    MultiByteToWideChar(cp, 0, lpszValue, -1, szIn, ARRAY_SIZE(szIn));

  if (cchOut > (int) ARRAY_SIZE(szOut))
    cchOut = ARRAY_SIZE(szOut);

  szOut[0] = '\0';

  iRet = GetNumberFormatW(lcid, dwFlags, lpszValue ? szIn : NULL, pfmt,
                          lpNumberStr ? szOut : NULL, cchOut);

  if (szOut[0] && lpNumberStr)
    WideCharToMultiByte(cp, 0, szOut, -1, lpNumberStr, cchOut, 0, 0);
  return iRet;
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

/**************************************************************************
 *              GetCurrencyFormatA	(KERNEL32.@)
 */
INT WINAPI GetCurrencyFormatA(LCID lcid, DWORD dwFlags,
                              LPCSTR lpszValue, const CURRENCYFMTA *lpFormat,
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
    cp = NLS_GetAnsiCodePage(lcid, dwFlags);
    if (!cp)
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
    }
  }

  if (lpFormat)
  {
    memcpy(&fmt, lpFormat, sizeof(fmt));
    pfmt = &fmt;
    if (lpFormat->lpDecimalSep)
    {
      MultiByteToWideChar(cp, 0, lpFormat->lpDecimalSep, -1, szDec, ARRAY_SIZE(szDec));
      fmt.lpDecimalSep = szDec;
    }
    if (lpFormat->lpThousandSep)
    {
      MultiByteToWideChar(cp, 0, lpFormat->lpThousandSep, -1, szGrp, ARRAY_SIZE(szGrp));
      fmt.lpThousandSep = szGrp;
    }
    if (lpFormat->lpCurrencySymbol)
    {
      MultiByteToWideChar(cp, 0, lpFormat->lpCurrencySymbol, -1, szCy, ARRAY_SIZE(szCy));
      fmt.lpCurrencySymbol = szCy;
    }
  }

  if (lpszValue)
    MultiByteToWideChar(cp, 0, lpszValue, -1, szIn, ARRAY_SIZE(szIn));

  if (cchOut > (int) ARRAY_SIZE(szOut))
    cchOut = ARRAY_SIZE(szOut);

  szOut[0] = '\0';

  iRet = GetCurrencyFormatW(lcid, dwFlags, lpszValue ? szIn : NULL, pfmt,
                            lpCurrencyStr ? szOut : NULL, cchOut);

  if (szOut[0] && lpCurrencyStr)
    WideCharToMultiByte(cp, 0, szOut, -1, lpCurrencyStr, cchOut, 0, 0);
  return iRet;
}

/**************************************************************************
 *              EnumDateFormatsA	(KERNEL32.@)
 *
 * FIXME: MSDN mentions only LOCALE_USE_CP_ACP, should we handle
 * LOCALE_NOUSEROVERRIDE here as well?
 */
BOOL WINAPI EnumDateFormatsA(DATEFMT_ENUMPROCA proc, LCID lcid, DWORD flags)
{
    struct enumdateformats_context ctxt;

    ctxt.type = CALLBACK_ENUMPROC;
    ctxt.u.callback = (DATEFMT_ENUMPROCW)proc;
    ctxt.lcid = lcid;
    ctxt.flags = flags;
    ctxt.unicode = FALSE;

    return NLS_EnumDateFormats(&ctxt);
}

/**************************************************************************
 *              EnumTimeFormatsA	(KERNEL32.@)
 *
 * FIXME: MSDN mentions only LOCALE_USE_CP_ACP, should we handle
 * LOCALE_NOUSEROVERRIDE here as well?
 */
BOOL WINAPI EnumTimeFormatsA(TIMEFMT_ENUMPROCA proc, LCID lcid, DWORD flags)
{
    struct enumtimeformats_context ctxt;

    /* EnumTimeFormatsA doesn't support flags, EnumTimeFormatsW does. */
    if (flags & ~LOCALE_USE_CP_ACP)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    ctxt.type = CALLBACK_ENUMPROC;
    ctxt.u.callback = (TIMEFMT_ENUMPROCW)proc;
    ctxt.lcid = lcid;
    ctxt.flags = flags;
    ctxt.unicode = FALSE;

    return NLS_EnumTimeFormats(&ctxt);
}

/******************************************************************************
 *		EnumCalendarInfoA	[KERNEL32.@]
 */

BOOL WINAPI EnumCalendarInfoA(CALINFO_ENUMPROCA calinfoproc, LCID locale,
                              CALID calendar, CALTYPE caltype)
{
  struct enumcalendar_context ctxt;

  TRACE("(%p,0x%08x,0x%08x,0x%08x)\n", calinfoproc, locale, calendar, caltype);

  ctxt.type = CALLBACK_ENUMPROC;
  ctxt.u.callback = (CALINFO_ENUMPROCW)calinfoproc;
  ctxt.lcid = locale;
  ctxt.calendar = calendar;
  ctxt.caltype = caltype;
  ctxt.lParam = 0;
  ctxt.unicode = FALSE;
  return NLS_EnumCalendarInfo(&ctxt);
}

/**************************************************************************
 *		EnumCalendarInfoExA	[KERNEL32.@]
 */
BOOL WINAPI EnumCalendarInfoExA(CALINFO_ENUMPROCEXA calinfoproc, LCID locale,
                                CALID calendar, CALTYPE caltype)
{
  struct enumcalendar_context ctxt;

  TRACE("(%p,0x%08x,0x%08x,0x%08x)\n", calinfoproc, locale, calendar, caltype);

  ctxt.type = CALLBACK_ENUMPROCEX;
  ctxt.u.callbackex = (CALINFO_ENUMPROCEXW)calinfoproc;
  ctxt.lcid = locale;
  ctxt.calendar = calendar;
  ctxt.caltype = caltype;
  ctxt.lParam = 0;
  ctxt.unicode = FALSE;
  return NLS_EnumCalendarInfo(&ctxt);
}

/*********************************************************************
 *	GetCalendarInfoA			(KERNEL32.@)
 */
int WINAPI GetCalendarInfoA(LCID lcid, CALID Calendar, CALTYPE CalType,
                            LPSTR lpCalData, int cchData, LPDWORD lpValue)
{
    int ret, cchDataW = cchData;
    LPWSTR lpCalDataW = NULL;
#ifdef __REACTOS__
    DWORD cp = CP_ACP;
    if (!(CalType & CAL_USE_CP_ACP))
    {
        DWORD dwFlags = ((CalType & CAL_NOUSEROVERRIDE) ? LOCALE_NOUSEROVERRIDE : 0);
        cp = NLS_GetAnsiCodePage(lcid, dwFlags);
        if (!cp)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }
    }
    if ((CalType & 0xFFFF) == CAL_SABBREVERASTRING)
    {
        /* NOTE: CAL_SABBREVERASTRING is not supported in GetCalendarInfoA */
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
#endif

    if (NLS_IsUnicodeOnlyLcid(lcid))
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
    }

    if (!cchData && !(CalType & CAL_RETURN_NUMBER))
        cchDataW = GetCalendarInfoW(lcid, Calendar, CalType, NULL, 0, NULL);
    if (!(lpCalDataW = HeapAlloc(GetProcessHeap(), 0, cchDataW*sizeof(WCHAR))))
        return 0;

    ret = GetCalendarInfoW(lcid, Calendar, CalType, lpCalDataW, cchDataW, lpValue);
    if(ret && lpCalDataW && lpCalData)
#ifdef __REACTOS__
        ret = WideCharToMultiByte(cp, 0, lpCalDataW, -1, lpCalData, cchData, NULL, NULL);
#else
        ret = WideCharToMultiByte(CP_ACP, 0, lpCalDataW, -1, lpCalData, cchData, NULL, NULL);
#endif
    else if (CalType & CAL_RETURN_NUMBER)
        ret *= sizeof(WCHAR);
    HeapFree(GetProcessHeap(), 0, lpCalDataW);

    return ret;
}

/*********************************************************************
 *	SetCalendarInfoA			(KERNEL32.@)
 */
int WINAPI SetCalendarInfoA(LCID Locale, CALID Calendar, CALTYPE CalType, LPCSTR lpCalData)
{
    FIXME("(%08x,%08x,%08x,%s): stub\n",
          Locale, Calendar, CalType, debugstr_a(lpCalData));
    return 0;
}

/* EOF */

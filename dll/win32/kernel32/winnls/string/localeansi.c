/*
 * Locale support
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1998 David Lee Lambert
 * Copyright 2000 Julio Cesar Gazquez
 * Copyright 2002 Alexandre Julliard for CodeWeavers
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
#include <debug.h>
#include "nls.h"

extern const NLS_LOCALE_DATA * WINAPI NlsValidateLocale( LCID *lcid, ULONG flags );

extern BOOL WINAPI Internal_EnumCalendarInfo( CALINFO_ENUMPROCW proc,
                                              const NLS_LOCALE_DATA *locale, CALID id,
                                              CALTYPE type, BOOL unicode, BOOL ex,
                                              BOOL exex, LPARAM lparam );
extern BOOL WINAPI Internal_EnumDateFormats( DATEFMT_ENUMPROCW proc, const NLS_LOCALE_DATA *locale,
                                             DWORD flags, BOOL unicode, BOOL ex, BOOL exex, LPARAM lparam );
extern BOOL WINAPI Internal_EnumLanguageGroupLocales( LANGGROUPLOCALE_ENUMPROCW proc, LGRPID id,
                                                      DWORD flags, LONG_PTR param, BOOL unicode );
extern BOOL WINAPI Internal_EnumSystemCodePages( CODEPAGE_ENUMPROCW proc, DWORD flags, BOOL unicode );
extern BOOL WINAPI Internal_EnumSystemLanguageGroups( LANGUAGEGROUP_ENUMPROCW proc, DWORD flags,
                                                      LONG_PTR param, BOOL unicode );
extern BOOL WINAPI Internal_EnumTimeFormats( TIMEFMT_ENUMPROCW proc, const NLS_LOCALE_DATA *locale,
                                             DWORD flags, BOOL unicode, BOOL ex, LPARAM lparam );
extern BOOL WINAPI Internal_EnumUILanguages( UILANGUAGE_ENUMPROCW proc, DWORD flags,
                                             LONG_PTR param, BOOL unicode );
DEBUG_CHANNEL(nls);

/***********************************************************************
 *		get_lcid_codepage
 *
 * Retrieve the ANSI codepage for a given locale.
 */
static UINT get_lcid_codepage( LCID lcid, UINT flags )
{
    UINT ret = 0;

    if (flags & LOCALE_USE_CP_ACP) return CP_ACP;
    GetLocaleInfoW( lcid, LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
                    (WCHAR *)&ret, sizeof(ret)/sizeof(WCHAR) );
    return ret;
}


/******************************************************************************
 *		SetLocaleInfoA	[KERNEL32.@]
 *
 * Set information about an aspect of a locale.
 *
 * PARAMS
 *  lcid   [I] LCID of the locale
 *  lctype [I] LCTYPE_ flags from "winnls.h"
 *  data   [I] Information to set
 *
 * RETURNS
 *  Success: TRUE. The information given will be returned by GetLocaleInfoA()
 *           whenever it is called without LOCALE_NOUSEROVERRIDE.
 *  Failure: FALSE. Use GetLastError() to determine the cause.
 *
 * NOTES
 *  - Values are only be set for the current user locale; the system locale
 *  settings cannot be changed.
 *  - Any settings changed by this call are lost when the locale is changed by
 *  the control panel (in Wine, this happens every time you change LANG).
 *  - The native implementation of this function does not check that lcid matches
 *  the current user locale, and simply sets the new values. Wine warns you in
 *  this case, but behaves the same.
 */
BOOL WINAPI SetLocaleInfoA(LCID lcid, LCTYPE lctype, LPCSTR data)
{
    UINT codepage = get_lcid_codepage( lcid, lctype );
    WCHAR *strW;
    DWORD len;
    BOOL ret;

    if (!data)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    len = MultiByteToWideChar( codepage, 0, data, -1, NULL, 0 );
    if (!(strW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }
    MultiByteToWideChar( codepage, 0, data, -1, strW, len );
    ret = SetLocaleInfoW( lcid, lctype, strW );
    HeapFree( GetProcessHeap(), 0, strW );
    return ret;
}

#ifndef __REACTOS__
/******************************************************************************
 *              SetCPGlobal   (KERNEL32.@)
 *
 * Set the current Ansi code page Id for the system.
 *
 * PARAMS
 *    acp [I] code page ID to be the new ACP.
 *
 * RETURNS
 *    The previous ACP.
 */
UINT WINAPI SetCPGlobal( UINT acp )
{
    FIXME( "not supported\n" );
    return GetACP();
}


/***********************************************************************
 *           GetCPInfoExA   (KERNEL32.@)
 *
 * Get extended information about a code page.
 *
 * PARAMS
 *  codepage [I] Code page number
 *  dwFlags  [I] Reserved, must to 0.
 *  cpinfo   [O] Destination for code page information
 *
 * RETURNS
 *  Success: TRUE. cpinfo is updated with the information about codepage.
 *  Failure: FALSE, if codepage is invalid or cpinfo is NULL.
 */
BOOL WINAPI GetCPInfoExA( UINT codepage, DWORD dwFlags, LPCPINFOEXA cpinfo )
{
    CPINFOEXW cpinfoW;

    if (!GetCPInfoExW( codepage, dwFlags, &cpinfoW ))
      return FALSE;

    /* the layout is the same except for CodePageName */
    memcpy(cpinfo, &cpinfoW, sizeof(CPINFOEXA));
    WideCharToMultiByte(CP_ACP, 0, cpinfoW.CodePageName, -1, cpinfo->CodePageName, sizeof(cpinfo->CodePageName), NULL, NULL);
    return TRUE;
}
#endif


/*********************************************************************
 *              GetDaylightFlag   (KERNEL32.@)
 */
BOOL WINAPI GetDaylightFlag(void)
{
    TIME_ZONE_INFORMATION tzinfo;
    return GetTimeZoneInformation( &tzinfo) == TIME_ZONE_ID_DAYLIGHT;
}


/***********************************************************************
 *              EnumSystemCodePagesA   (KERNEL32.@)
 */
BOOL WINAPI EnumSystemCodePagesA( CODEPAGE_ENUMPROCA proc, DWORD flags )
{
    return Internal_EnumSystemCodePages( (CODEPAGE_ENUMPROCW)proc, flags, FALSE );
}


/******************************************************************************
 *           GetStringTypeExA    (KERNEL32.@)
 *
 * Get characteristics of the characters making up a string.
 *
 * PARAMS
 *  locale   [I] Locale Id for the string
 *  type     [I] CT_CTYPE1 = classification, CT_CTYPE2 = directionality, CT_CTYPE3 = typographic info
 *  src      [I] String to analyse
 *  count    [I] Length of src in chars, or -1 if src is NUL terminated
 *  chartype [O] Destination for the calculated characteristics
 *
 * RETURNS
 *  Success: TRUE. chartype is filled with the requested characteristics of each char
 *           in src.
 *  Failure: FALSE. Use GetLastError() to determine the cause.
 */
BOOL WINAPI GetStringTypeExA( LCID locale, DWORD type, LPCSTR src, INT count, LPWORD chartype )
{
    return GetStringTypeA(locale, type, src, count, chartype);
}

/*************************************************************************
 *           FoldStringA    (KERNEL32.@)
 *
 * Map characters in a string.
 *
 * PARAMS
 *  dwFlags [I] Flags controlling chars to map (MAP_ constants from "winnls.h")
 *  src     [I] String to map
 *  srclen  [I] Length of src, or -1 if src is NUL terminated
 *  dst     [O] Destination for mapped string
 *  dstlen  [I] Length of dst, or 0 to find the required length for the mapped string
 *
 * RETURNS
 *  Success: The length of the string written to dst, including the terminating NUL. If
 *           dstlen is 0, the value returned is the same, but nothing is written to dst,
 *           and dst may be NULL.
 *  Failure: 0. Use GetLastError() to determine the cause.
 */
INT WINAPI FoldStringA(DWORD dwFlags, LPCSTR src, INT srclen,
                       LPSTR dst, INT dstlen)
{
    INT ret = 0, srclenW = 0;
    WCHAR *srcW = NULL, *dstW = NULL;

    if (!src || !srclen || dstlen < 0 || (dstlen && !dst) || src == dst)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    srclenW = MultiByteToWideChar(CP_ACP, 0, src, srclen, NULL, 0);
    srcW = HeapAlloc(GetProcessHeap(), 0, srclenW * sizeof(WCHAR));

    if (!srcW)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto FoldStringA_exit;
    }

    MultiByteToWideChar(CP_ACP, 0, src, srclen, srcW, srclenW);

    ret = FoldStringW(dwFlags, srcW, srclenW, NULL, 0);
    if (ret && dstlen)
    {
        dstW = HeapAlloc(GetProcessHeap(), 0, ret * sizeof(WCHAR));

        if (!dstW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto FoldStringA_exit;
        }

        ret = FoldStringW(dwFlags, srcW, srclenW, dstW, ret);
        if (!WideCharToMultiByte(CP_ACP, 0, dstW, ret, dst, dstlen, NULL, NULL))
        {
            ret = 0;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }
    }

    HeapFree(GetProcessHeap(), 0, dstW);

FoldStringA_exit:
    HeapFree(GetProcessHeap(), 0, srcW);
    return ret;
}

/******************************************************************************
 *           EnumSystemLanguageGroupsA    (KERNEL32.@)
 */
BOOL WINAPI EnumSystemLanguageGroupsA( LANGUAGEGROUP_ENUMPROCA proc, DWORD flags, LONG_PTR param )
{
    return Internal_EnumSystemLanguageGroups( (LANGUAGEGROUP_ENUMPROCW)proc, flags, param, FALSE );
}

/******************************************************************************
 *           EnumLanguageGroupLocalesA    (KERNEL32.@)
 */
BOOL WINAPI EnumLanguageGroupLocalesA( LANGGROUPLOCALE_ENUMPROCA proc, LGRPID id,
                                       DWORD flags, LONG_PTR param )
{
    return Internal_EnumLanguageGroupLocales( (LANGGROUPLOCALE_ENUMPROCW)proc, id, flags, param, FALSE );
}

/******************************************************************************
 *		EnumCalendarInfoA	[KERNEL32.@]
 */
BOOL WINAPI EnumCalendarInfoA( CALINFO_ENUMPROCA proc, LCID lcid, CALID id, CALTYPE type )
{
    return Internal_EnumCalendarInfo( (CALINFO_ENUMPROCW)proc, NlsValidateLocale( &lcid, 0 ),
                                      id, type, FALSE, FALSE, FALSE, 0 );
}

/******************************************************************************
 *		EnumCalendarInfoExA	[KERNEL32.@]
 */
BOOL WINAPI EnumCalendarInfoExA( CALINFO_ENUMPROCEXA proc, LCID lcid, CALID id, CALTYPE type )
{
    return Internal_EnumCalendarInfo( (CALINFO_ENUMPROCW)proc, NlsValidateLocale( &lcid, 0 ),
                                      id, type, FALSE, TRUE, FALSE, 0 );
}

/**************************************************************************
 *              EnumDateFormatsExA    (KERNEL32.@)
 *
 * FIXME: MSDN mentions only LOCALE_USE_CP_ACP, should we handle
 * LOCALE_NOUSEROVERRIDE here as well?
 */
BOOL WINAPI EnumDateFormatsExA(DATEFMT_ENUMPROCEXA proc, LCID lcid, DWORD flags)
{
    return Internal_EnumDateFormats( (DATEFMT_ENUMPROCW)proc, NlsValidateLocale( &lcid, 0 ),
                                     flags, FALSE, TRUE, FALSE, 0 );
}

/**************************************************************************
 *              EnumDateFormatsA	(KERNEL32.@)
 *
 * FIXME: MSDN mentions only LOCALE_USE_CP_ACP, should we handle
 * LOCALE_NOUSEROVERRIDE here as well?
 */
BOOL WINAPI EnumDateFormatsA(DATEFMT_ENUMPROCA proc, LCID lcid, DWORD flags)
{
    return Internal_EnumDateFormats( (DATEFMT_ENUMPROCW)proc, NlsValidateLocale( &lcid, 0 ),
                                     flags, FALSE, FALSE, FALSE, 0 );
}

/**************************************************************************
 *              EnumTimeFormatsA	(KERNEL32.@)
 *
 * FIXME: MSDN mentions only LOCALE_USE_CP_ACP, should we handle
 * LOCALE_NOUSEROVERRIDE here as well?
 */
BOOL WINAPI EnumTimeFormatsA( TIMEFMT_ENUMPROCA proc, LCID lcid, DWORD flags )
{
    /* EnumTimeFormatsA doesn't support flags, EnumTimeFormatsW does. */
    if (flags & ~LOCALE_USE_CP_ACP)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }
    return Internal_EnumTimeFormats( (TIMEFMT_ENUMPROCW)proc, NlsValidateLocale( &lcid, 0 ),
                                     flags, FALSE, FALSE, 0 );
}

/******************************************************************************
 *           InvalidateNLSCache           (KERNEL32.@)
 *
 * Invalidate the cache of NLS values.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  Success: TRUE.
 *  Failure: FALSE.
 */
BOOL WINAPI InvalidateNLSCache(void)
{
#ifdef __REACTOS__
    //JapaneseEra_ClearCache();
    return TRUE;
#else
  FIXME("() stub\n");
  return FALSE;
#endif
}

/******************************************************************************
 *           EnumUILanguagesA (KERNEL32.@)
 */
BOOL WINAPI EnumUILanguagesA( UILANGUAGE_ENUMPROCA proc, DWORD flags, LONG_PTR param )
{
    return Internal_EnumUILanguages( (UILANGUAGE_ENUMPROCW)proc, flags, param, FALSE );
}


/*********************************************************************
 *            GetCalendarInfoA (KERNEL32.@)
 */
int WINAPI GetCalendarInfoA( LCID lcid, CALID id, CALTYPE type, LPSTR data, int data_len, DWORD *val )
{
    WCHAR buffer[256];

    if (type & CAL_RETURN_NUMBER)
        return GetCalendarInfoW( lcid, id, type, (WCHAR *)data, data_len, val ) * sizeof(WCHAR);

    if (!GetCalendarInfoW( lcid, id, type, buffer, ARRAY_SIZE(buffer), val )) return 0;
    return WideCharToMultiByte( get_lcid_codepage(lcid, type), 0, buffer, -1, data, data_len, NULL, NULL );
}


/*********************************************************************
 *            SetCalendarInfoA (KERNEL32.@)
 */
int WINAPI SetCalendarInfoA( LCID lcid, CALID id, CALTYPE type, LPCSTR data )
{
    WCHAR buffer[256];

    if (!MultiByteToWideChar( get_lcid_codepage(lcid, type), 0, data, -1, buffer, ARRAY_SIZE(buffer) ))
        return 0;
    return SetCalendarInfoW( lcid, id, type, buffer );
}


/**************************************************************************
 *           GetNumberFormatA (KERNEL32.@)
 */
int WINAPI GetNumberFormatA( LCID lcid, DWORD flags, const char *value,
                             const NUMBERFMTA *format, char *buffer, int len )
{
    UINT cp = get_lcid_codepage( lcid, flags );
    WCHAR input[128], output[128];
    int ret;

    TRACE( "(0x%04lx,0x%08lx,%s,%p,%p,%d)\n", lcid, flags, debugstr_a(value), format, buffer, len );

    if (len < 0 || (len && !buffer) || !value)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    MultiByteToWideChar( cp, 0, value, -1, input, ARRAY_SIZE(input) );

    if (format)
    {
        NUMBERFMTW fmt;
        WCHAR fmt_decimal[4], fmt_thousand[4];

        if (flags & LOCALE_NOUSEROVERRIDE)
        {
            SetLastError( ERROR_INVALID_FLAGS );
            return 0;
        }
        if (!format->lpDecimalSep || !format->lpThousandSep)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        MultiByteToWideChar( cp, 0, format->lpDecimalSep, -1, fmt_decimal, ARRAY_SIZE(fmt_decimal) );
        MultiByteToWideChar( cp, 0, format->lpThousandSep, -1, fmt_thousand, ARRAY_SIZE(fmt_thousand) );
        fmt.NumDigits     = format->NumDigits;
        fmt.LeadingZero   = format->LeadingZero;
        fmt.Grouping      = format->Grouping;
        fmt.NegativeOrder = format->NegativeOrder;
        fmt.lpDecimalSep  = fmt_decimal;
        fmt.lpThousandSep = fmt_thousand;
        ret = GetNumberFormatW( lcid, flags, input, &fmt, output, ARRAY_SIZE(output) );
    }
    else ret = GetNumberFormatW( lcid, flags, input, NULL, output, ARRAY_SIZE(output) );

    if (ret) ret = WideCharToMultiByte( cp, 0, output, -1, buffer, len, 0, 0 );
    return ret;
}


/**************************************************************************
 *           GetCurrencyFormatA (KERNEL32.@)
 */
int WINAPI GetCurrencyFormatA( LCID lcid, DWORD flags, const char *value,
                               const CURRENCYFMTA *format, char *buffer, int len )
{
    UINT cp = get_lcid_codepage( lcid, flags );
    WCHAR input[128], output[128];
    int ret;

    TRACE( "(0x%04lx,0x%08lx,%s,%p,%p,%d)\n", lcid, flags, debugstr_a(value), format, buffer, len );

    if (len < 0 || (len && !buffer) || !value)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    MultiByteToWideChar( cp, 0, value, -1, input, ARRAY_SIZE(input) );

    if (format)
    {
        CURRENCYFMTW fmt;
        WCHAR fmt_decimal[4], fmt_thousand[4], fmt_symbol[13];

        if (flags & LOCALE_NOUSEROVERRIDE)
        {
            SetLastError( ERROR_INVALID_FLAGS );
            return 0;
        }
        if (!format->lpDecimalSep || !format->lpThousandSep || !format->lpCurrencySymbol)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        MultiByteToWideChar( cp, 0, format->lpDecimalSep, -1, fmt_decimal, ARRAY_SIZE(fmt_decimal) );
        MultiByteToWideChar( cp, 0, format->lpThousandSep, -1, fmt_thousand, ARRAY_SIZE(fmt_thousand) );
        MultiByteToWideChar( cp, 0, format->lpCurrencySymbol, -1, fmt_symbol, ARRAY_SIZE(fmt_symbol) );
        fmt.NumDigits = format->NumDigits;
        fmt.LeadingZero = format->LeadingZero;
        fmt.Grouping = format->Grouping;
        fmt.NegativeOrder = format->NegativeOrder;
        fmt.PositiveOrder = format->PositiveOrder;
        fmt.lpDecimalSep = fmt_decimal;
        fmt.lpThousandSep = fmt_thousand;
        fmt.lpCurrencySymbol = fmt_symbol;
        ret = GetCurrencyFormatW( lcid, flags, input, &fmt, output, ARRAY_SIZE(output) );
    }
    else ret = GetCurrencyFormatW( lcid, flags, input, NULL, output, ARRAY_SIZE(output) );

    if (ret) ret = WideCharToMultiByte( cp, 0, output, -1, buffer, len, 0, 0 );
    return ret;
}


/******************************************************************************
 *           GetGeoInfoA (KERNEL32.@)
 */
INT WINAPI GetGeoInfoA(GEOID geoid, GEOTYPE geotype, LPSTR data, int data_len, LANGID lang)
{
    WCHAR buffer[256];

    TRACE("%ld %ld %p %d %d\n", geoid, geotype, data, data_len, lang);

    if (!GetGeoInfoW( geoid, geotype, buffer, ARRAY_SIZE(buffer), lang )) return 0;
    return WideCharToMultiByte( CP_ACP, 0, buffer, -1, data, data_len, NULL, NULL );
}

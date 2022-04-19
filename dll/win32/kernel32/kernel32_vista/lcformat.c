

/******************************************************************************
 * GetDateFormatEx [KERNEL32.@]
 *
 * Format a date for a given locale.
 *
 * PARAMS
 *  localename [I] Locale to format for
 *  flags      [I] LOCALE_ and DATE_ flags from "winnls.h"
 *  date       [I] Date to format
 *  format     [I] Format string, or NULL to use the locale defaults
 *  outbuf     [O] Destination for formatted string
 *  bufsize    [I] Size of outbuf, or 0 to calculate the resulting size
 *  calendar   [I] Reserved, must be NULL
 *
 * See GetDateFormatA for notes.
 *
 * RETURNS
 *  Success: The number of characters written to outbuf, or that would have
 *           been written if bufsize is 0.
 *  Failure: 0. Use GetLastError() to determine the cause.
 */
INT WINAPI GetDateFormatEx(LPCWSTR localename, DWORD flags,
                           const SYSTEMTIME* date, LPCWSTR format,
                           LPWSTR outbuf, INT bufsize, LPCWSTR calendar)
{
  TRACE("(%s,0x%08x,%p,%s,%p,%d,%s)\n", debugstr_w(localename), flags,
        date, debugstr_w(format), outbuf, bufsize, debugstr_w(calendar));

  /* Parameter is currently reserved and Windows errors if set */
  if (calendar != NULL)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
  }

  return NLS_GetDateTimeFormatW(LocaleNameToLCID(localename, 0),
                                flags | DATE_DATEVARSONLY, date, format,
                                outbuf, bufsize);
}

/***********************************************************************
 *            GetCurrencyFormatEx (KERNEL32.@)
 */
int WINAPI GetCurrencyFormatEx(LPCWSTR localename, DWORD flags, LPCWSTR value,
                                const CURRENCYFMTW *format, LPWSTR str, int len)
{
    TRACE("(%s,0x%08x,%s,%p,%p,%d)\n", debugstr_w(localename), flags,
            debugstr_w(value), format, str, len);

    return GetCurrencyFormatW( LocaleNameToLCID(localename, 0), flags, value, format, str, len);
}

/**************************************************************************
 *              GetNumberFormatEx	(KERNEL32.@)
 */
INT WINAPI GetNumberFormatEx(LPCWSTR name, DWORD flags,
                             LPCWSTR value, const NUMBERFMTW *format,
                             LPWSTR number, int numout)
{
  LCID lcid;

  TRACE("(%s,0x%08x,%s,%p,%p,%d)\n", debugstr_w(name), flags,
        debugstr_w(value), format, number, numout);

  lcid = LocaleNameToLCID(name, 0);
  if (!lcid)
    return 0;

  return GetNumberFormatW(lcid, flags, value, format, number, numout);
}

}

#if _WIN32_WINNT >= 0x600
/**************************************************************************
 *              EnumTimeFormatsEx	(KERNEL32.@)
 */
BOOL WINAPI EnumTimeFormatsEx(TIMEFMT_ENUMPROCEX proc, const WCHAR *locale, DWORD flags, LPARAM lParam)
{
    struct enumtimeformats_context ctxt;

    ctxt.type = CALLBACK_ENUMPROCEX;
    ctxt.u.callbackex = proc;
    ctxt.lcid = LocaleNameToLCID(locale, 0);
    ctxt.flags = flags;
    ctxt.lParam = lParam;
    ctxt.unicode = TRUE;

    return NLS_EnumTimeFormats(&ctxt);
}

/******************************************************************************
 *		EnumCalendarInfoExEx	[KERNEL32.@]
 */
BOOL WINAPI EnumCalendarInfoExEx( CALINFO_ENUMPROCEXEX calinfoproc, LPCWSTR locale, CALID calendar,
    LPCWSTR reserved, CALTYPE caltype, LPARAM lParam)
{
  struct enumcalendar_context ctxt;

  TRACE("(%p,%s,0x%08x,%p,0x%08x,0x%ld)\n", calinfoproc, debugstr_w(locale), calendar, reserved, caltype, lParam);

  ctxt.type = CALLBACK_ENUMPROCEXEX;
  ctxt.u.callbackexex = calinfoproc;
  ctxt.lcid = LocaleNameToLCID(locale, 0);
  ctxt.calendar = calendar;
  ctxt.caltype = caltype;
  ctxt.lParam = lParam;
  ctxt.unicode = TRUE;
  return NLS_EnumCalendarInfo(&ctxt);
}

/**************************************************************************
 *              EnumDateFormatsExEx	(KERNEL32.@)
 */
BOOL WINAPI EnumDateFormatsExEx(DATEFMT_ENUMPROCEXEX proc, const WCHAR *locale, DWORD flags, LPARAM lParam)
{
    struct enumdateformats_context ctxt;

    ctxt.type = CALLBACK_ENUMPROCEXEX;
    ctxt.u.callbackexex = proc;
    ctxt.lcid = LocaleNameToLCID(locale, 0);
    ctxt.flags = flags;
    ctxt.lParam = lParam;
    ctxt.unicode = TRUE;

    return NLS_EnumDateFormats(&ctxt);
}

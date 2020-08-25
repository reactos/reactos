/*
 *  TIME.C - time internal command.
 *
 *
 *  History:
 *
 *    07/08/1998 (John P. Price)
 *        started.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    09-Jan-1999 (Eric Kohl)
 *        Added locale support.
 *
 *    19-Jan-1999 (Eric Kohl)
 *        Unicode and redirection safe!
 *        Added "/t" option.
 *
 *    04-Feb-1999 (Eric Kohl)
 *        Fixed time input bug.
 *
 *    30-Apr-2005 (Magnus Olsen <magnus@greatlord.com>)
 *        Remove all hardcoded strings in En.rc.
 */

#include "precomp.h"

#ifdef INCLUDE_CMD_TIME


static BOOL ParseTime (LPTSTR s)
{
    SYSTEMTIME t;
    LPTSTR p = s;

    if (!*s)
        return TRUE;

    GetLocalTime (&t);
    t.wHour = 0;
    t.wMinute = 0;
    t.wSecond = 0;
    t.wMilliseconds = 0;

    // first get hour
    if (_istdigit(*p))
    {
        while (_istdigit(*p))
        {
            t.wHour = t.wHour * 10 + *p - _T('0');
            p++;
        }
    }
    else
        return FALSE;

    // get time separator
    if (*p != cTimeSeparator)
        return FALSE;
    p++;

    // now get minutes
    if (_istdigit(*p))
    {
        while (_istdigit(*p))
        {
            t.wMinute = t.wMinute * 10 + *p - _T('0');
            p++;
        }
    }
    else
        return FALSE;

    // get time separator
    if (*p != cTimeSeparator)
        return FALSE;
    p++;

    // now get seconds
    if (_istdigit(*p))
    {
        while (_istdigit(*p))
        {
            t.wSecond = t.wSecond * 10 + *p - _T('0');
            p++;
        }
    }
    else
        return FALSE;

    // get decimal separator
    if (*p == cDecimalSeparator)
    {
        p++;

        // now get hundreths
        if (_istdigit(*p))
        {
            while (_istdigit(*p))
            {
                // t.wMilliseconds = t.wMilliseconds * 10 + *p - _T('0');
                p++;
            }
            // t.wMilliseconds *= 10;
        }
    }

    /* special case: 12 hour format */
    if (nTimeFormat == 0)
    {
        if (_totupper(*s) == _T('P'))
        {
            t.wHour += 12;
        }

        if ((_totupper(*s) == _T('A')) && (t.wHour == 12))
        {
            t.wHour = 0;
        }
    }

    if (t.wHour > 23 || t.wMinute > 60 || t.wSecond > 60 || t.wMilliseconds > 999)
        return FALSE;

    SetLocalTime (&t);

    return TRUE;
}


INT cmd_time(LPTSTR param)
{
    LPTSTR* arg;
    INT argc;
    INT i;
    INT nTimeString = -1;
    TCHAR szTime[40];

    if (!_tcsncmp(param, _T("/?"), 2))
    {
        ConOutResPaging(TRUE, STRING_TIME_HELP1);
        return 0;
    }

    nErrorLevel = 0;

    /* Build the parameter array */
    arg = split(param, &argc, FALSE, FALSE);

    /* Check for options */
    for (i = 0; i < argc; i++)
    {
        if (bEnableExtensions && (_tcsicmp(arg[i], _T("/T")) == 0))
        {
            /* Display current time in short format */
            SYSTEMTIME st;
            GetLocalTime(&st);
            FormatTime(szTime, &st);
            ConOutPuts(szTime);
            freep(arg);
            return 0;
        }

        if ((*arg[i] != _T('/')) && (nTimeString == -1))
            nTimeString = i;
    }

    if (nTimeString == -1)
    {
        ConOutResPrintf(STRING_LOCALE_HELP1);
        ConOutPrintf(_T(": %s\n"), GetTimeString());
    }

    while (TRUE)
    {
        if (nTimeString == -1)
        {
            ConOutResPuts(STRING_TIME_HELP2);
            ConInString(szTime, ARRAYSIZE(szTime));

            TRACE("\'%s\'\n", debugstr_aw(szTime));

            while (*szTime && szTime[_tcslen(szTime) - 1] < _T(' '))
                szTime[_tcslen(szTime) - 1] = _T('\0');

            if (ParseTime(szTime))
            {
                freep(arg);
                return 0;
            }
        }
        else
        {
            if (ParseTime(arg[nTimeString]))
            {
                freep(arg);
                return 0;
            }

            /* Force input the next time around */
            nTimeString = -1;
        }

        ConErrResPuts(STRING_TIME_ERROR1);
        nErrorLevel = 1;
    }

    freep(arg);
    return 0;
}

#endif

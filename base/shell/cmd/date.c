/*
 *  DATE.C - date internal command.
 *
 *
 *  History:
 *
 *    08 Jul 1998 (John P. Price)
 *        started.
 *
 *    20 Jul 1998 (John P. Price)
 *        corrected number of days for December from 30 to 31.
 *        (Thanx to Steffen Kaiser for bug report)
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    29-Jul-1998 (Rob Lake)
 *        fixed stand-alone mode.
 *        Added Pacific C compatible dos_getdate functions
 *
 *    09-Jan-1999 (Eric Kohl)
 *        Added locale support
 *
 *    23-Jan-1999 (Eric Kohl)
 *        Unicode and redirection safe!
 *
 *    04-Feb-1999 (Eric Kohl)
 *        Fixed date input bug.
 *
 *    03-Apr-2005 (Magnus Olsen <magnus@greatlord.com>)
 *        Remove all hardcoded strings in En.rc
 */

#include "precomp.h"

#ifdef INCLUDE_CMD_DATE


static WORD awMonths[2][13] =
{
    {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};


static VOID
PromptDateString(VOID)
{
    switch (nDateFormat)
    {
        case 0: /* mmddyy */
        default:
            ConOutResPrintf(STRING_DATE_HELP1, cDateSeparator, cDateSeparator);
            break;

        case 1: /* ddmmyy */
            ConOutResPrintf(STRING_DATE_HELP2, cDateSeparator, cDateSeparator);
            break;

        case 2: /* yymmdd */
            ConOutResPrintf(STRING_DATE_HELP3, cDateSeparator, cDateSeparator);
            break;
    }
}

static BOOL
ReadNumber (LPTSTR *s, LPWORD lpwValue)
{
    if (_istdigit (**s))
    {
        while (_istdigit (**s))
        {
            *lpwValue = *lpwValue * 10 + **s - _T('0');
            (*s)++;
        }
        return TRUE;
    }
    return FALSE;
}

static BOOL
ReadSeparator (LPTSTR *s)
{
    if (**s == _T('/') || **s == _T('-') || **s == cDateSeparator)
    {
        (*s)++;
        return TRUE;
    }
    return FALSE;
}

static BOOL
ParseDate (LPTSTR s)
{
    SYSTEMTIME d;
    unsigned char leap;
    LPTSTR p = s;

    if (!*s)
        return TRUE;

    GetLocalTime (&d);

    d.wYear = 0;
    d.wDay = 0;
    d.wMonth = 0;

    switch (nDateFormat)
    {
        case 0: /* mmddyy */
        default:
            if (!ReadNumber (&p, &d.wMonth))
                return FALSE;
            if (!ReadSeparator (&p))
                return FALSE;
            if (!ReadNumber (&p, &d.wDay))
                return FALSE;
            if (!ReadSeparator (&p))
                return FALSE;
            if (!ReadNumber (&p, &d.wYear))
                return FALSE;
            break;

        case 1: /* ddmmyy */
            if (!ReadNumber (&p, &d.wDay))
                return FALSE;
            if (!ReadSeparator (&p))
                return FALSE;
            if (!ReadNumber (&p, &d.wMonth))
                return FALSE;
            if (!ReadSeparator (&p))
                return FALSE;
            if (!ReadNumber (&p, &d.wYear))
                return FALSE;
            break;

        case 2: /* yymmdd */
            if (!ReadNumber (&p, &d.wYear))
                return FALSE;
            if (!ReadSeparator (&p))
                return FALSE;
            if (!ReadNumber (&p, &d.wMonth))
                return FALSE;
            if (!ReadSeparator (&p))
                return FALSE;
            if (!ReadNumber (&p, &d.wDay))
                return FALSE;
            break;
    }

    /* if only entered two digits: */
    /*   assume 2000's if value less than 80 */
    /*   assume 1900's if value greater or equal 80 */
    if (d.wYear <= 99)
    {
        if (d.wYear >= 80)
            d.wYear = 1900 + d.wYear;
        else
            d.wYear = 2000 + d.wYear;
    }

    leap = (!(d.wYear % 4) && (d.wYear % 100)) || !(d.wYear % 400);

    if ((d.wMonth >= 1 && d.wMonth <= 12) &&
        (d.wDay >= 1 && d.wDay <= awMonths[leap][d.wMonth]) &&
        (d.wYear >= 1980 && d.wYear <= 2099))
    {
        SetLocalTime (&d);
        return TRUE;
    }

    return FALSE;
}


INT cmd_date(LPTSTR param)
{
    LPTSTR* arg;
    INT argc;
    INT i;
    BOOL bPrompt = TRUE;
    INT nDateString = -1;
    TCHAR szDate[40];

    if (!_tcsncmp(param, _T("/?"), 2))
    {
        ConOutResPaging(TRUE, STRING_DATE_HELP4);
        return 0;
    }

    nErrorLevel = 0;

    /* Build the parameter array */
    arg = split(param, &argc, FALSE, FALSE);

    /* Check for options */
    for (i = 0; i < argc; i++)
    {
        if (bEnableExtensions && (_tcsicmp(arg[i], _T("/T")) == 0))
            bPrompt = FALSE;

        if ((*arg[i] != _T('/')) && (nDateString == -1))
            nDateString = i;
    }

    if (nDateString == -1)
    {
        ConOutResPuts(STRING_DATE_NOW);
        ConOutPrintf(_T("%s\n"), GetDateString());
    }

    if (!bPrompt)
    {
        freep(arg);
        return 0;
    }

    while (TRUE)
    {
        if (nDateString == -1)
        {
            PromptDateString();
            ConInString(szDate, ARRAYSIZE(szDate));

            TRACE("\'%s\'\n", debugstr_aw(szDate));

            while (*szDate && szDate[_tcslen(szDate) - 1] < _T(' '))
                szDate[_tcslen(szDate) - 1] = _T('\0');

            if (ParseDate(szDate))
            {
                freep(arg);
                return 0;
            }
        }
        else
        {
            if (ParseDate(arg[nDateString]))
            {
                freep(arg);
                return 0;
            }

            /* Force input the next time around */
            nDateString = -1;
        }

        ConErrResPuts(STRING_DATE_ERROR);
        nErrorLevel = 1;
    }

    freep(arg);
    return 0;
}
#endif /* INCLUDE_CMD_DATE */

/* EOF */

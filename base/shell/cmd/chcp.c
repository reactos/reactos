/*
 *  CHCP.C - chcp internal command.
 *
 *
 *  History:
 *
 *    23-Dec-1998 (Eric Kohl)
 *        Started.
 *
 *    02-Apr-2005 (Magnus Olsen <magnus@greatlord.com>)
 *        Remove all hardcoded strings in En.rc
 */

#include "precomp.h"

#ifdef INCLUDE_CMD_CHCP

INT CommandChcp(LPTSTR param)
{
    LPTSTR *arg;
    INT    args;
    UINT uNewCodePage;

    /* Print help */
    if (!_tcsncmp(param, _T("/?"), 2))
    {
        ConOutResPaging(TRUE,STRING_CHCP_HELP);
        return 0;
    }

    nErrorLevel = 0;

    /* Get parameters */
    arg = split(param, &args, FALSE, FALSE);

    if (args == 0)
    {
        /* Display the active code page number */
        ConOutResPrintf(STRING_CHCP_ERROR1, OutputCodePage);
        freep(arg);
        return 0;
    }

    if (args >= 2)
    {
        /* Too many parameters */
        ConErrResPrintf(STRING_ERROR_INVALID_PARAM_FORMAT, param);
        freep(arg);
        nErrorLevel = 1;
        return 1;
    }

    uNewCodePage = (UINT)_ttoi(arg[0]);

    if (uNewCodePage == 0)
    {
        ConErrResPrintf(STRING_ERROR_INVALID_PARAM_FORMAT, arg[0]);
        freep(arg);
        nErrorLevel = 1;
        return 1;
    }

    freep(arg);

    // TODO: In case of failure of SetConsoleCP or SetConsoleOutputCP,
    // restore the old code page!

    /*
     * Try changing the console input codepage. If it works then also change
     * the console output codepage, and refresh our local codepage cache.
     */
    if (!SetConsoleCP(uNewCodePage))
    {
        ConErrResPuts(STRING_CHCP_ERROR4);
    }
    else
    {
        SetConsoleOutputCP(uNewCodePage);

        /* Update our local codepage cache */
        InputCodePage  = GetConsoleCP();
        OutputCodePage = GetConsoleOutputCP();

        InitLocale();

        /* Display the active code page number */
        ConOutResPrintf(STRING_CHCP_ERROR1, OutputCodePage);
    }

    return 0;
}

#endif /* INCLUDE_CMD_CHCP */

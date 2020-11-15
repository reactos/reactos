/*
 *  PATH.C - path internal command.
 *
 *
 *  History:
 *
 *    17 Jul 1998 (John P Price)
 *        Separated commands into individual files.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    09-Dec-1998 (Eric Kohl)
 *        Added help text ("/?").
 *
 *    18-Jan-1999 (Eric Kohl)
 *        Unicode ready!
 *
 *    18-Jan-1999 (Eric Kohl)
 *        Redirection safe!
 *
 *    24-Jan-1999 (Eric Kohl)
 *        Fixed Win32 environment handling.
 *
 *    30-Apr-2005 (Magnus Olsen <magnus@greatlord.com>)
 *        Remove all hardcoded strings in En.rc
 */
#include "precomp.h"

#ifdef INCLUDE_CMD_PATH

/* Size of environment variable buffer */
#define ENV_BUFFER_SIZE 1024


INT cmd_path(LPTSTR param)
{
    INT retval = 0;

    if (!_tcsncmp(param, _T("/?"), 2))
    {
        ConOutResPaging(TRUE, STRING_PATH_HELP1);
        return 0;
    }

    /* If param is empty, display the PATH environment variable */
    if (!param || !*param)
    {
        DWORD  dwBuffer;
        LPTSTR pszBuffer;

        pszBuffer = (LPTSTR)cmd_alloc(ENV_BUFFER_SIZE * sizeof(TCHAR));
        if (!pszBuffer)
        {
            WARN("Cannot allocate memory for pszBuffer!\n");
            error_out_of_memory();
            retval = 1;
            goto Quit;
        }

        dwBuffer = GetEnvironmentVariable(_T("PATH"), pszBuffer, ENV_BUFFER_SIZE);
        if (dwBuffer == 0)
        {
            cmd_free(pszBuffer);
            ConErrResPrintf(STRING_SET_ENV_ERROR, _T("PATH"));
            retval = 0;
            goto Quit;
        }
        else if (dwBuffer > ENV_BUFFER_SIZE)
        {
            LPTSTR pszOldBuffer = pszBuffer;
            pszBuffer = (LPTSTR)cmd_realloc(pszBuffer, dwBuffer * sizeof (TCHAR));
            if (!pszBuffer)
            {
                WARN("Cannot reallocate memory for pszBuffer!\n");
                error_out_of_memory();
                cmd_free(pszOldBuffer);
                retval = 1;
                goto Quit;
            }
            GetEnvironmentVariable(_T("PATH"), pszBuffer, dwBuffer);
        }

        ConOutPrintf(_T("PATH=%s\n"), pszBuffer);
        cmd_free(pszBuffer);

        retval = 0;
        goto Quit;
    }

    /* Skip leading '=' */
    if (*param == _T('='))
        param++;

    /* Set PATH environment variable */
    if (!SetEnvironmentVariable(_T("PATH"), param))
    {
        retval = 1;
    }

Quit:
    if (BatType != CMD_TYPE)
    {
        if (retval != 0)
            nErrorLevel = retval;
    }
    else
    {
        nErrorLevel = retval;
    }

    return retval;
}

#endif

/* EOF */

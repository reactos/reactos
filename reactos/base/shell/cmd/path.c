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

/* size of environment variable buffer */
#define ENV_BUFFER_SIZE 1024


INT cmd_path (LPTSTR param)
{
    if (!_tcsncmp (param, _T("/?"), 2))
    {
        ConOutResPaging(TRUE,STRING_PATH_HELP1);
        return 0;
    }

    nErrorLevel = 0;

    /* if param is empty, display the PATH environment variable */
    if (!param || !*param)
    {
        DWORD  dwBuffer;
        LPTSTR pszBuffer;

        pszBuffer = (LPTSTR)cmd_alloc (ENV_BUFFER_SIZE * sizeof(TCHAR));
        dwBuffer = GetEnvironmentVariable (_T("PATH"), pszBuffer, ENV_BUFFER_SIZE);
        if (dwBuffer == 0)
        {
            cmd_free(pszBuffer);
            ConOutResPrintf(STRING_VOL_HELP2, _T("PATH"));
            return 0;
        }
        else if (dwBuffer > ENV_BUFFER_SIZE)
        {
            LPTSTR pszOldBuffer = pszBuffer;
            pszBuffer = (LPTSTR)cmd_realloc (pszBuffer, dwBuffer * sizeof (TCHAR));
            if (pszBuffer == NULL)
            {
                cmd_free(pszOldBuffer);
                return 1;
            }
            GetEnvironmentVariable (_T("PATH"), pszBuffer, dwBuffer);
        }

        ConOutPrintf(_T("PATH=%s\n"), pszBuffer);
        cmd_free (pszBuffer);

        return 0;
    }

    /* skip leading '=' */
    if (*param == _T('='))
        param++;

    /* set PATH environment variable */
    if (!SetEnvironmentVariable (_T("PATH"), param))
    {
        nErrorLevel = 1;
        return 1;
    }

    return 0;
}

#endif

/* EOF */

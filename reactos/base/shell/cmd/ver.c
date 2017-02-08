/*
 *  VER.C - ver internal command.
 *
 *
 *  History:
 *
 *    06/30/98 (Rob Lake)
 *        rewrote ver command to accept switches, now ver alone prints
 *        copyright notice only.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    30-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added text about where to send bug reports and get updates.
 *
 *    20-Jan-1999 (Eric Kohl)
 *        Unicode and redirection safe!
 *
 *    26-Feb-1999 (Eric Kohl)
 *        New version info and some output changes.
 */

#include "precomp.h"
#include <reactos/buildno.h>
#include <reactos/version.h>

OSVERSIONINFO osvi;
TCHAR szOSName[50] = _T("");


VOID InitOSVersion(VOID)
{
    LONG lResult;
    HKEY hKey;

    /* Get version information */
    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);

    /* Build OS version string */

    /* Open registry key */
    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
                           0,
                           KEY_QUERY_VALUE,
                           &hKey);
    if (lResult == ERROR_SUCCESS)
    {
        DWORD dwSize, dwType;

        /* Retrieve the ProductName value */
        dwSize = sizeof(szOSName);
        lResult = RegQueryValueEx(hKey,
                                  _T("ProductName"),
                                  NULL,
                                  &dwType,
                                  (LPBYTE)szOSName,
                                  &dwSize);

        /* If we have failed or the data type is unsupported... */
        if (lResult != ERROR_SUCCESS || dwType != REG_SZ)
        {
            /* ... reserve size for one NULL character only! */
            dwSize = sizeof(TCHAR);

            /* Set an error code (anything != ERROR_SUCCESS) */
            lResult = ERROR_INVALID_PARAMETER;
        }

        /* NULL-terminate the string */
        szOSName[(dwSize / sizeof(TCHAR)) - 1] = _T('\0');

        /* Close the key */
        RegCloseKey(hKey);
    }

    /*
     * If the registry key somehow doesn't exist or cannot be loaded, then
     * determine at least whether the version of Windows is either 9x or NT.
     */
    if (lResult != ERROR_SUCCESS)
    {
        switch (osvi.dwPlatformId)
        {
            case VER_PLATFORM_WIN32_WINDOWS:
            {
                if (osvi.dwMajorVersion == 4)
                {
                    if (osvi.dwMinorVersion == 0)
                        _tcscpy(szOSName, _T("Windows 95"));
                    else if (osvi.dwMinorVersion == 1)
                        _tcscpy(szOSName, _T("Windows 98"));
                    else if (osvi.dwMinorVersion == 9)
                        _tcscpy(szOSName, _T("Windows ME"));
                    else
                        _tcscpy(szOSName, _T("Windows 9x"));
                }
                break;
            }

            case VER_PLATFORM_WIN32_NT:
            {
                _tcscpy(szOSName, _T("Windows NT"));
                break;
            }
        }
    }
}

/* Print the current OS version, suitable for VER command and PROMPT $V format */
VOID PrintOSVersion(VOID)
{
    ConOutResPrintf(STRING_VERSION_RUNVER, szOSName,
                    osvi.dwMajorVersion, osvi.dwMinorVersion,
                    osvi.dwBuildNumber, osvi.szCSDVersion);
}

#ifdef INCLUDE_CMD_VER

/*
 * display shell version info internal command.
 */
INT cmd_ver (LPTSTR param)
{
    INT i;

    nErrorLevel = 0;

    if (_tcsstr(param, _T("/?")) != NULL)
    {
        ConOutResPaging(TRUE,STRING_VERSION_HELP1);
        return 0;
    }

    ConOutResPrintf(STRING_CMD_SHELLINFO, _T(KERNEL_RELEASE_STR), _T(KERNEL_VERSION_BUILD_STR));
    ConOutChar(_T('\n'));
    ConOutResPuts(STRING_VERSION_RUNNING_ON);
    PrintOSVersion();

    /* Basic copyright notice */
    if (param[0] != _T('\0'))
    {
        ConOutPuts(_T("\n\n"));
        ConOutPuts(_T("Copyright (C) 1994-1998 Tim Norman and others.\n"));
        ConOutPuts(_T("Copyright (C) 1998-") _T(COPYRIGHT_YEAR) _T(" ReactOS Team\n"));

        for (i = 0; param[i]; i++)
        {
            /* Skip spaces */
            if (param[i] == _T(' '))
                continue;

            if (param[i] == _T('/'))
            {
                /* Is this a lone '/' ? */
                if (param[i + 1] == 0)
                {
                    error_invalid_switch(_T(' '));
                    return 1;
                }
                continue;
            }

            if (_totupper(param[i]) == _T('W'))
            {
                /* Warranty notice */
                ConOutResPuts(STRING_VERSION_HELP3);
            }
            else if (_totupper(param[i]) == _T('R'))
            {
                /* Redistribution notice */
                ConOutResPuts(STRING_VERSION_HELP4);
            }
            else if (_totupper(param[i]) == _T('C'))
            {
                /* Developer listing */
                ConOutResPuts(STRING_VERSION_HELP6);
                ConOutResPuts(STRING_FREEDOS_DEV);
                ConOutResPuts(STRING_VERSION_HELP7);
                ConOutResPuts(STRING_REACTOS_DEV);
            }
            else
            {
                error_invalid_switch(_totupper(param[i]));
                return 1;
            }
        }

        /* Bug report notice */
        ConOutResPuts(STRING_VERSION_HELP5);
    }

    ConOutChar(_T('\n'));

    return 0;
}

#endif

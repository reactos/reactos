/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/sc/misc.c
 * PURPOSE:     Various functions
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *              Roel Messiant <roelmessiant@gmail.com>
 */

#include "sc.h"

typedef struct
{
    LPCTSTR lpOption;
    DWORD dwValue;
} OPTION_INFO;

static const OPTION_INFO TypeOpts[] =
{
    { _T("own"),      SERVICE_WIN32_OWN_PROCESS   },
    { _T("share"),    SERVICE_WIN32_SHARE_PROCESS },
    { _T("interact"), SERVICE_INTERACTIVE_PROCESS },
    { _T("kernel"),   SERVICE_KERNEL_DRIVER       },
    { _T("filesys"),  SERVICE_FILE_SYSTEM_DRIVER  },
    { _T("rec"),      SERVICE_RECOGNIZER_DRIVER   }
};

static const OPTION_INFO StartOpts[] =
{
    { _T("boot"),     SERVICE_BOOT_START   },
    { _T("system"),   SERVICE_SYSTEM_START },
    { _T("auto"),     SERVICE_AUTO_START   },
    { _T("demand"),   SERVICE_DEMAND_START },
    { _T("disabled"), SERVICE_DISABLED     }
};

static const OPTION_INFO ErrorOpts[] =
{
    { _T("normal"),   SERVICE_ERROR_NORMAL   },
    { _T("severe"),   SERVICE_ERROR_SEVERE   },
    { _T("critical"), SERVICE_ERROR_CRITICAL },
    { _T("ignore"),   SERVICE_ERROR_IGNORE   }
};

static const OPTION_INFO TagOpts[] =
{
    { _T("yes"), TRUE  },
    { _T("no"),  FALSE }
};


BOOL
ParseCreateConfigArguments(
    LPCTSTR *ServiceArgs,
    INT ArgCount,
    BOOL bChangeService,
    OUT LPSERVICE_CREATE_INFO lpServiceInfo)
{
    INT i, ArgIndex = 1;

    if (ArgCount < 1)
        return FALSE;

    ZeroMemory(lpServiceInfo, sizeof(SERVICE_CREATE_INFO));

    if (bChangeService)
    {
        lpServiceInfo->dwServiceType = SERVICE_NO_CHANGE;
        lpServiceInfo->dwStartType = SERVICE_NO_CHANGE;
        lpServiceInfo->dwErrorControl = SERVICE_NO_CHANGE;
    }

    lpServiceInfo->lpServiceName = ServiceArgs[0];

    ArgCount--;

    while (ArgCount > 1)
    {
        if (!lstrcmpi(ServiceArgs[ArgIndex], _T("type=")))
        {
            for (i = 0; i < sizeof(TypeOpts) / sizeof(TypeOpts[0]); i++)
                if (!lstrcmpi(ServiceArgs[ArgIndex + 1], TypeOpts[i].lpOption))
                {
                    if (lpServiceInfo->dwServiceType == SERVICE_NO_CHANGE)
                        lpServiceInfo->dwServiceType = TypeOpts[i].dwValue;
                    else
                        lpServiceInfo->dwServiceType |= TypeOpts[i].dwValue;
                    break;
                }

            if (i == sizeof(TypeOpts) / sizeof(TypeOpts[0]))
                break;
        }
        else if (!lstrcmpi(ServiceArgs[ArgIndex], _T("start=")))
        {
            for (i = 0; i < sizeof(StartOpts) / sizeof(StartOpts[0]); i++)
                if (!lstrcmpi(ServiceArgs[ArgIndex + 1], StartOpts[i].lpOption))
                {
                    lpServiceInfo->dwStartType = StartOpts[i].dwValue;
                    break;
                }

            if (i == sizeof(StartOpts) / sizeof(StartOpts[0]))
                break;
        }
        else if (!lstrcmpi(ServiceArgs[ArgIndex], _T("error=")))
        {
            for (i = 0; i < sizeof(ErrorOpts) / sizeof(ErrorOpts[0]); i++)
                if (!lstrcmpi(ServiceArgs[ArgIndex + 1], ErrorOpts[i].lpOption))
                {
                    lpServiceInfo->dwErrorControl = ErrorOpts[i].dwValue;
                    break;
                }

            if (i == sizeof(ErrorOpts) / sizeof(ErrorOpts[0]))
                break;
        }
        else if (!lstrcmpi(ServiceArgs[ArgIndex], _T("tag=")))
        {
            for (i = 0; i < sizeof(TagOpts) / sizeof(TagOpts[0]); i++)
                if (!lstrcmpi(ServiceArgs[ArgIndex + 1], TagOpts[i].lpOption))
                {
                    lpServiceInfo->bTagId = TagOpts[i].dwValue;
                    break;
                }

            if (i == sizeof(TagOpts) / sizeof(TagOpts[0]))
                break;
        }
        else if (!lstrcmpi(ServiceArgs[ArgIndex], _T("binpath=")))
        {
            lpServiceInfo->lpBinaryPathName = ServiceArgs[ArgIndex + 1];
        }
        else if (!lstrcmpi(ServiceArgs[ArgIndex], _T("group=")))
        {
            lpServiceInfo->lpLoadOrderGroup = ServiceArgs[ArgIndex + 1];
        }
        else if (!lstrcmpi(ServiceArgs[ArgIndex], _T("depend=")))
        {
            lpServiceInfo->lpDependencies = ServiceArgs[ArgIndex + 1];
        }
        else if (!lstrcmpi(ServiceArgs[ArgIndex], _T("obj=")))
        {
            lpServiceInfo->lpServiceStartName = ServiceArgs[ArgIndex + 1];
        }
        else if (!lstrcmpi(ServiceArgs[ArgIndex], _T("displayname=")))
        {
            lpServiceInfo->lpDisplayName = ServiceArgs[ArgIndex + 1];
        }
        else if (!lstrcmpi(ServiceArgs[ArgIndex], _T("password=")))
        {
            lpServiceInfo->lpPassword = ServiceArgs[ArgIndex + 1];
        }

        ArgIndex += 2;
        ArgCount -= 2;
    }

    return (ArgCount == 0);
}


BOOL
ParseFailureActions(
    IN LPCTSTR lpActions,
    OUT DWORD *pcActions,
    OUT SC_ACTION **ppActions)
{
    SC_ACTION *pActions = NULL;
    LPTSTR pStringBuffer = NULL;
    LPTSTR p;
    INT nLength;
    INT nCount = 0;

    *pcActions = 0;
    *ppActions = NULL;

    nLength = _tcslen(lpActions);

    /* Allocate the string buffer */
    pStringBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (nLength + 2) * sizeof(TCHAR));
    if (pStringBuffer == NULL)
    {
        return FALSE;
    }

    /* Copy the actions string into the string buffer */
    CopyMemory(pStringBuffer, lpActions, nLength * sizeof(TCHAR));

    /* Replace all slashes by null characters */
    p = pStringBuffer;
    while (*p != _T('\0'))
    {
        if (*p == _T('/'))
            *p = _T('\0');
        p++;
    }

    /* Count the arguments in the buffer */
    p = pStringBuffer;
    while (*p != _T('\0'))
    {
        nCount++;

        nLength = _tcslen(p);
        p = (LPTSTR)((LONG_PTR)p + ((nLength + 1) * sizeof(TCHAR)));
    }

    /* Allocate the actions buffer */
    pActions = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nCount / 2 * sizeof(SC_ACTION));
    if (pActions == NULL)
    {
        HeapFree(GetProcessHeap(), 0, pStringBuffer);
        return FALSE;
    }

    /* Parse the string buffer */
    nCount = 0;
    p = pStringBuffer;
    while (*p != _T('\0'))
    {
        nLength = _tcslen(p);

        if (nCount % 2 == 0)
        {
            /* Action */
            if (!lstrcmpi(p, _T("reboot")))
                pActions[nCount / 2].Type = SC_ACTION_REBOOT;
            else if (!lstrcmpi(p, _T("restart")))
                pActions[nCount / 2].Type = SC_ACTION_RESTART;
            else if (!lstrcmpi(p, _T("run")))
                pActions[nCount / 2].Type = SC_ACTION_RUN_COMMAND;
            else
                break;
        }
        else
        {
             /* Delay */
             pActions[nCount / 2].Delay = _tcstoul(p, NULL, 10);
             if (pActions[nCount / 2].Delay == 0 && errno == ERANGE)
                 break;
        }

        p = (LPTSTR)((LONG_PTR)p + ((nLength + 1) * sizeof(TCHAR)));
        nCount++;
    }

    /* Free the string buffer */
    HeapFree(GetProcessHeap(), 0, pStringBuffer);

    *pcActions = nCount / 2;
    *ppActions = pActions;

    return TRUE;
}


BOOL
ParseFailureArguments(
    IN LPCTSTR *ServiceArgs,
    IN INT ArgCount,
    OUT LPCTSTR *ppServiceName,
    OUT LPSERVICE_FAILURE_ACTIONS pFailureActions)
{
    INT ArgIndex = 1;
    LPCTSTR lpActions = NULL;
    LPCTSTR lpReset = NULL;

    if (ArgCount < 1)
        return FALSE;

    ZeroMemory(pFailureActions, sizeof(SERVICE_FAILURE_ACTIONS));

    *ppServiceName = ServiceArgs[0];

    ArgCount--;

    while (ArgCount > 1)
    {
        if (!lstrcmpi(ServiceArgs[ArgIndex], _T("actions=")))
        {
            lpActions = (LPTSTR)ServiceArgs[ArgIndex + 1];
        }
        else if (!lstrcmpi(ServiceArgs[ArgIndex], _T("command=")))
        {
            pFailureActions->lpCommand = (LPTSTR)ServiceArgs[ArgIndex + 1];
        }
        else if (!lstrcmpi(ServiceArgs[ArgIndex], _T("reboot=")))
        {
            pFailureActions->lpRebootMsg = (LPTSTR)ServiceArgs[ArgIndex + 1];
        }
        else if (!lstrcmpi(ServiceArgs[ArgIndex], _T("reset=")))
        {
            lpReset = (LPTSTR)ServiceArgs[ArgIndex + 1];
        }

        ArgIndex += 2;
        ArgCount -= 2;
    }

    if ((lpReset == NULL && lpActions != NULL) ||
        (lpReset != NULL && lpActions == NULL))
    {
        _tprintf(_T("ERROR:  The reset and actions options must be used simultaneously.\n\n"));
        return FALSE;
    }

    if (lpReset != NULL)
    {
        if (!lstrcmpi(lpReset, _T("infinite")))
            pFailureActions->dwResetPeriod = INFINITE;
        else
            pFailureActions->dwResetPeriod = _ttoi(lpReset);
    }

    if (lpActions != NULL)
    {
        if (!ParseFailureActions(lpActions,
                                 &pFailureActions->cActions,
                                 &pFailureActions->lpsaActions))
        {
            return FALSE;
        }
    }

    return (ArgCount == 0);
}

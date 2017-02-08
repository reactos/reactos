/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/sc/create.c
 * PURPOSE:     Create a service
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *              Roel Messiant <roelmessiant@gmail.com>
 *
 */

#include "sc.h"

typedef struct
{
    LPCTSTR lpOption;
    DWORD dwValue;
} OPTION_INFO;

typedef struct
{
    LPCTSTR lpServiceName;
    LPCTSTR lpDisplayName;
    DWORD dwServiceType;
    DWORD dwStartType;
    DWORD dwErrorControl;
    LPCTSTR lpBinaryPathName;
    LPCTSTR lpLoadOrderGroup;
    DWORD dwTagId;
    LPCTSTR lpDependencies;
    LPCTSTR lpServiceStartName;
    LPCTSTR lpPassword;

    BOOL bTagId;
} SERVICE_CREATE_INFO, *LPSERVICE_CREATE_INFO;


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


static BOOL ParseCreateArguments(
    LPCTSTR *ServiceArgs,
    INT ArgCount,
    OUT LPSERVICE_CREATE_INFO lpServiceInfo
)
{
    INT i, ArgIndex = 1;

    if (ArgCount < 1)
        return FALSE;

    ZeroMemory(lpServiceInfo, sizeof(SERVICE_CREATE_INFO));

    lpServiceInfo->lpServiceName = ServiceArgs[0];

    ArgCount--;

    while (ArgCount > 1)
    {
        if (!lstrcmpi(ServiceArgs[ArgIndex], _T("type=")))
        {
            for (i = 0; i < sizeof(TypeOpts) / sizeof(TypeOpts[0]); i++)
                if (!lstrcmpi(ServiceArgs[ArgIndex + 1], TypeOpts[i].lpOption))
                {
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

BOOL Create(LPCTSTR *ServiceArgs, INT ArgCount)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hSc;
    BOOL bRet = FALSE;

    INT i;
    INT Length;
    LPTSTR lpBuffer = NULL;
    SERVICE_CREATE_INFO ServiceInfo;

    if (!ParseCreateArguments(ServiceArgs, ArgCount, &ServiceInfo))
    {
        CreateUsage();
        return FALSE;
    }

    if (!ServiceInfo.dwServiceType)
        ServiceInfo.dwServiceType = SERVICE_WIN32_OWN_PROCESS;

    if (!ServiceInfo.dwStartType)
        ServiceInfo.dwStartType = SERVICE_DEMAND_START;

    if (!ServiceInfo.dwErrorControl)
        ServiceInfo.dwErrorControl = SERVICE_ERROR_NORMAL;

    if (ServiceInfo.lpDependencies)
    {
        Length = lstrlen(ServiceInfo.lpDependencies);

        lpBuffer = HeapAlloc(GetProcessHeap(),
                             0,
                            (Length + 2) * sizeof(TCHAR));

        for (i = 0; i < Length; i++)
            if (ServiceInfo.lpDependencies[i] == _T('/'))
                lpBuffer[i] = 0;
            else
                lpBuffer[i] = ServiceInfo.lpDependencies[i];

        lpBuffer[Length] = 0;
        lpBuffer[Length + 1] = 0;

        ServiceInfo.lpDependencies = lpBuffer;
    }

#ifdef SCDBG
    _tprintf(_T("service name - %s\n"), ServiceInfo.lpServiceName);
    _tprintf(_T("display name - %s\n"), ServiceInfo.lpDisplayName);
    _tprintf(_T("service type - %lu\n"), ServiceInfo.dwServiceType);
    _tprintf(_T("start type - %lu\n"), ServiceInfo.dwStartType);
    _tprintf(_T("error control - %lu\n"), ServiceInfo.dwErrorControl);
    _tprintf(_T("Binary path - %s\n"), ServiceInfo.lpBinaryPathName);
    _tprintf(_T("load order group - %s\n"), ServiceInfo.lpLoadOrderGroup);
    _tprintf(_T("tag - %lu\n"), ServiceInfo.dwTagId);
    _tprintf(_T("dependencies - %s\n"), ServiceInfo.lpDependencies);
    _tprintf(_T("account start name - %s\n"), ServiceInfo.lpServiceStartName);
    _tprintf(_T("account password - %s\n"), ServiceInfo.lpPassword);
#endif

    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);

    if (hSCManager != NULL)
    {
        hSc = CreateService(hSCManager,
                            ServiceInfo.lpServiceName,
                            ServiceInfo.lpDisplayName,
                            SERVICE_ALL_ACCESS,
                            ServiceInfo.dwServiceType,
                            ServiceInfo.dwStartType,
                            ServiceInfo.dwErrorControl,
                            ServiceInfo.lpBinaryPathName,
                            ServiceInfo.lpLoadOrderGroup,
                            ServiceInfo.bTagId ? &ServiceInfo.dwTagId : NULL,
                            ServiceInfo.lpDependencies,
                            ServiceInfo.lpServiceStartName,
                            ServiceInfo.lpPassword);

        if (hSc != NULL)
        {
            _tprintf(_T("[SC] CreateService SUCCESS\n"));

            CloseServiceHandle(hSc);
            bRet = TRUE;
        }
        else
            ReportLastError();

        CloseServiceHandle(hSCManager);
    }
    else
        ReportLastError();

    if (lpBuffer != NULL)
        HeapFree(GetProcessHeap(), 0, lpBuffer);

    return bRet;
}

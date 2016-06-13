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

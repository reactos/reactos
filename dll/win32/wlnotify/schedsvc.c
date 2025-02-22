/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/wlnotify/schedsvc.c
 * PURPOSE:     Scheduler service logon notifications
 * PROGRAMMER:  Eric Kohl <eric.kohl@reactos.org>
 */

#include "precomp.h"
#include <winsvc.h>

WINE_DEFAULT_DEBUG_CHANNEL(wlnotify);


VOID
WINAPI
SchedEventLogoff(
    PWLX_NOTIFICATION_INFO pInfo)
{
#if 0
    SC_HANDLE hManager = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS ServiceStatus;
#endif

    TRACE("SchedEventLogoff\n");
    TRACE("Size: %lu\n", pInfo->Size);
    TRACE("Flags: %lx\n", pInfo->Flags);
    TRACE("UserName: %S\n", pInfo->UserName);
    TRACE("Domain: %S\n", pInfo->Domain);
    TRACE("WindowStation: %S\n", pInfo->WindowStation);
    TRACE("hToken: %p\n", pInfo->hToken);
    TRACE("hDesktop: %p\n", pInfo->hDesktop);
    TRACE("pStatusCallback: %p\n", pInfo->pStatusCallback);

#if 0
    hManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (hManager == NULL)
    {
        WARN("OpenSCManagerW() failed (Error %lu)\n", GetLastError());
        goto done;
    }

    hService = OpenServiceW(hManager, L"Schedule", SERVICE_USER_DEFINED_CONTROL);
    if (hManager == NULL)
    {
        WARN("OpenServiceW() failed (Error %lu)\n", GetLastError());
        goto done;
    }

    if (!ControlService(hService, 129, &ServiceStatus))
    {
        WARN("ControlService() failed (Error %lu)\n", GetLastError());
    }

done:
    if (hService != NULL)
        CloseServiceHandle(hService);

    if (hManager != NULL)
        CloseServiceHandle(hManager);
#endif
}


VOID
WINAPI
SchedStartShell(
    PWLX_NOTIFICATION_INFO pInfo)
{
    SC_HANDLE hManager = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS ServiceStatus;

    TRACE("SchedStartShell\n");
    TRACE("Size: %lu\n", pInfo->Size);
    TRACE("Flags: %lx\n", pInfo->Flags);
    TRACE("UserName: %S\n", pInfo->UserName);
    TRACE("Domain: %S\n", pInfo->Domain);
    TRACE("WindowStation: %S\n", pInfo->WindowStation);
    TRACE("hToken: %p\n", pInfo->hToken);
    TRACE("hDesktop: %p\n", pInfo->hDesktop);
    TRACE("pStatusCallback: %p\n", pInfo->pStatusCallback);

    hManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (hManager == NULL)
    {
        WARN("OpenSCManagerW() failed (Error %lu)\n", GetLastError());
        goto done;
    }

    hService = OpenServiceW(hManager, L"Schedule", SERVICE_USER_DEFINED_CONTROL);
    if (hManager == NULL)
    {
        WARN("OpenServiceW() failed (Error %lu)\n", GetLastError());
        goto done;
    }

    if (!ControlService(hService, 128, &ServiceStatus))
    {
        WARN("ControlService() failed (Error %lu)\n", GetLastError());
    }

done:
    if (hService != NULL)
        CloseServiceHandle(hService);

    if (hManager != NULL)
        CloseServiceHandle(hManager);
}

/* EOF */

/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/wlnotify/schedsvc.c
 * PURPOSE:     Scheduler service logon notifications
 * PROGRAMMER:  Eric Kohl <eric.kohl@reactos.org>
 */

#include "precomp.h"
#include <winsvc.h>

#define _NDEBUG
#include <debug.h>


VOID
WINAPI
SchedEventLogoff(
    PWLX_NOTIFICATION_INFO pInfo)
{
    SC_HANDLE hManager = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS ServiceStatus;

    DPRINT("SchedStartShellEvent\n");
    DPRINT("Size: %lu\n", pInfo->Size);
    DPRINT("Flags: %lx\n", pInfo->Flags);
    DPRINT("UserName: %S\n", pInfo->UserName);
    DPRINT("Domain: %S\n", pInfo->Domain);
    DPRINT("WindowStation: %S\n", pInfo->WindowStation);
    DPRINT("hToken: %p\n", pInfo->hToken);
    DPRINT("hDesktop: %p\n", pInfo->hDesktop);
    DPRINT("pStatusCallback: %p\n", pInfo->pStatusCallback);

    hManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (hManager == NULL)
    {
        DPRINT1("OpenSCManagerW() failed (Error %lu)\n", GetLastError());
        goto done;
    }

    hService = OpenServiceW(hManager, L"Schedule", SERVICE_USER_DEFINED_CONTROL);
    if (hManager == NULL)
    {
        DPRINT1("OpenServiceW() failed (Error %lu)\n", GetLastError());
        goto done;
    }

    if (!ControlService(hService, 129, &ServiceStatus))
    {
        DPRINT1("ControlService() failed (Error %lu)\n", GetLastError());
    }

done:
    if (hService != NULL)
        CloseServiceHandle(hService);

    if (hManager != NULL)
        CloseServiceHandle(hManager);
}


VOID
WINAPI
SchedStartShell(
    PWLX_NOTIFICATION_INFO pInfo)
{
    SC_HANDLE hManager = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS ServiceStatus;

    DPRINT("SchedStartShellEvent\n");
    DPRINT("Size: %lu\n", pInfo->Size);
    DPRINT("Flags: %lx\n", pInfo->Flags);
    DPRINT("UserName: %S\n", pInfo->UserName);
    DPRINT("Domain: %S\n", pInfo->Domain);
    DPRINT("WindowStation: %S\n", pInfo->WindowStation);
    DPRINT("hToken: %p\n", pInfo->hToken);
    DPRINT("hDesktop: %p\n", pInfo->hDesktop);
    DPRINT("pStatusCallback: %p\n", pInfo->pStatusCallback);

    hManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (hManager == NULL)
    {
        DPRINT1("OpenSCManagerW() failed (Error %lu)\n", GetLastError());
        goto done;
    }

    hService = OpenServiceW(hManager, L"Schedule", SERVICE_USER_DEFINED_CONTROL);
    if (hManager == NULL)
    {
        DPRINT1("OpenServiceW() failed (Error %lu)\n", GetLastError());
        goto done;
    }

    if (!ControlService(hService, 128, &ServiceStatus))
    {
        DPRINT1("ControlService() failed (Error %lu)\n", GetLastError());
    }

done:
    if (hService != NULL)
        CloseServiceHandle(hService);

    if (hManager != NULL)
        CloseServiceHandle(hManager);
}

/* EOF */

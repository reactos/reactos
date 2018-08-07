/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/wlnotify/senssvc.c
 * PURPOSE:     SENS service logon notifications
 * PROGRAMMER:  Eric Kohl <eric.kohl@reactos.org>
 */

#include "precomp.h"
#include <winsvc.h>

#define _NDEBUG
#include <debug.h>


VOID
WINAPI
SensDisconnectEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("SensDisconnectEvent\n");
    DPRINT("Size: %lu\n", pInfo->Size);
    DPRINT("Flags: %lx\n", pInfo->Flags);
    DPRINT("UserName: %S\n", pInfo->UserName);
    DPRINT("Domain: %S\n", pInfo->Domain);
    DPRINT("WindowStation: %S\n", pInfo->WindowStation);
    DPRINT("hToken: %p\n", pInfo->hToken);
    DPRINT("hDesktop: %p\n", pInfo->hDesktop);
    DPRINT("pStatusCallback: %p\n", pInfo->pStatusCallback);
}


VOID
WINAPI
SensLockEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("SensLockEvent\n");
    DPRINT("Size: %lu\n", pInfo->Size);
    DPRINT("Flags: %lx\n", pInfo->Flags);
    DPRINT("UserName: %S\n", pInfo->UserName);
    DPRINT("Domain: %S\n", pInfo->Domain);
    DPRINT("WindowStation: %S\n", pInfo->WindowStation);
    DPRINT("hToken: %p\n", pInfo->hToken);
    DPRINT("hDesktop: %p\n", pInfo->hDesktop);
    DPRINT("pStatusCallback: %p\n", pInfo->pStatusCallback);
}


VOID
WINAPI
SensLogoffEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("SensLogoffEvent\n");
    DPRINT("Size: %lu\n", pInfo->Size);
    DPRINT("Flags: %lx\n", pInfo->Flags);
    DPRINT("UserName: %S\n", pInfo->UserName);
    DPRINT("Domain: %S\n", pInfo->Domain);
    DPRINT("WindowStation: %S\n", pInfo->WindowStation);
    DPRINT("hToken: %p\n", pInfo->hToken);
    DPRINT("hDesktop: %p\n", pInfo->hDesktop);
    DPRINT("pStatusCallback: %p\n", pInfo->pStatusCallback);
}


VOID
WINAPI
SensLogonEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("SensLogonEvent\n");
    DPRINT("Size: %lu\n", pInfo->Size);
    DPRINT("Flags: %lx\n", pInfo->Flags);
    DPRINT("UserName: %S\n", pInfo->UserName);
    DPRINT("Domain: %S\n", pInfo->Domain);
    DPRINT("WindowStation: %S\n", pInfo->WindowStation);
    DPRINT("hToken: %p\n", pInfo->hToken);
    DPRINT("hDesktop: %p\n", pInfo->hDesktop);
    DPRINT("pStatusCallback: %p\n", pInfo->pStatusCallback);
}


VOID
WINAPI
SensPostShellEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("SensPostShellEvent\n");
    DPRINT("Size: %lu\n", pInfo->Size);
    DPRINT("Flags: %lx\n", pInfo->Flags);
    DPRINT("UserName: %S\n", pInfo->UserName);
    DPRINT("Domain: %S\n", pInfo->Domain);
    DPRINT("WindowStation: %S\n", pInfo->WindowStation);
    DPRINT("hToken: %p\n", pInfo->hToken);
    DPRINT("hDesktop: %p\n", pInfo->hDesktop);
    DPRINT("pStatusCallback: %p\n", pInfo->pStatusCallback);
}


VOID
WINAPI
SensReconnectEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("SensReconnectEvent\n");
    DPRINT("Size: %lu\n", pInfo->Size);
    DPRINT("Flags: %lx\n", pInfo->Flags);
    DPRINT("UserName: %S\n", pInfo->UserName);
    DPRINT("Domain: %S\n", pInfo->Domain);
    DPRINT("WindowStation: %S\n", pInfo->WindowStation);
    DPRINT("hToken: %p\n", pInfo->hToken);
    DPRINT("hDesktop: %p\n", pInfo->hDesktop);
    DPRINT("pStatusCallback: %p\n", pInfo->pStatusCallback);
}


VOID
WINAPI
SensShutdownEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("SensShutdownEvent\n");
    DPRINT("Size: %lu\n", pInfo->Size);
    DPRINT("Flags: %lx\n", pInfo->Flags);
    DPRINT("UserName: %S\n", pInfo->UserName);
    DPRINT("Domain: %S\n", pInfo->Domain);
    DPRINT("WindowStation: %S\n", pInfo->WindowStation);
    DPRINT("hToken: %p\n", pInfo->hToken);
    DPRINT("hDesktop: %p\n", pInfo->hDesktop);
    DPRINT("pStatusCallback: %p\n", pInfo->pStatusCallback);
}


VOID
WINAPI
SensStartScreenSaverEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("SensStartScreenSaverEvent\n");
    DPRINT("Size: %lu\n", pInfo->Size);
    DPRINT("Flags: %lx\n", pInfo->Flags);
    DPRINT("UserName: %S\n", pInfo->UserName);
    DPRINT("Domain: %S\n", pInfo->Domain);
    DPRINT("WindowStation: %S\n", pInfo->WindowStation);
    DPRINT("hToken: %p\n", pInfo->hToken);
    DPRINT("hDesktop: %p\n", pInfo->hDesktop);
    DPRINT("pStatusCallback: %p\n", pInfo->pStatusCallback);
}


VOID
WINAPI
SensStartShellEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("SensStartShellEvent\n");
    DPRINT("Size: %lu\n", pInfo->Size);
    DPRINT("Flags: %lx\n", pInfo->Flags);
    DPRINT("UserName: %S\n", pInfo->UserName);
    DPRINT("Domain: %S\n", pInfo->Domain);
    DPRINT("WindowStation: %S\n", pInfo->WindowStation);
    DPRINT("hToken: %p\n", pInfo->hToken);
    DPRINT("hDesktop: %p\n", pInfo->hDesktop);
    DPRINT("pStatusCallback: %p\n", pInfo->pStatusCallback);
}


VOID
WINAPI
SensStartupEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("SensStartupEvent\n");
    DPRINT("Size: %lu\n", pInfo->Size);
    DPRINT("Flags: %lx\n", pInfo->Flags);
    DPRINT("UserName: %S\n", pInfo->UserName);
    DPRINT("Domain: %S\n", pInfo->Domain);
    DPRINT("WindowStation: %S\n", pInfo->WindowStation);
    DPRINT("hToken: %p\n", pInfo->hToken);
    DPRINT("hDesktop: %p\n", pInfo->hDesktop);
    DPRINT("pStatusCallback: %p\n", pInfo->pStatusCallback);
}


VOID
WINAPI
SensStopScreenSaverEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("SensStopScreenSaverEvent\n");
    DPRINT("Size: %lu\n", pInfo->Size);
    DPRINT("Flags: %lx\n", pInfo->Flags);
    DPRINT("UserName: %S\n", pInfo->UserName);
    DPRINT("Domain: %S\n", pInfo->Domain);
    DPRINT("WindowStation: %S\n", pInfo->WindowStation);
    DPRINT("hToken: %p\n", pInfo->hToken);
    DPRINT("hDesktop: %p\n", pInfo->hDesktop);
    DPRINT("pStatusCallback: %p\n", pInfo->pStatusCallback);
}


VOID
WINAPI
SensUnlockEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("SensUnlockEvent\n");
    DPRINT("Size: %lu\n", pInfo->Size);
    DPRINT("Flags: %lx\n", pInfo->Flags);
    DPRINT("UserName: %S\n", pInfo->UserName);
    DPRINT("Domain: %S\n", pInfo->Domain);
    DPRINT("WindowStation: %S\n", pInfo->WindowStation);
    DPRINT("hToken: %p\n", pInfo->hToken);
    DPRINT("hDesktop: %p\n", pInfo->hDesktop);
    DPRINT("pStatusCallback: %p\n", pInfo->pStatusCallback);
}

/* EOF */

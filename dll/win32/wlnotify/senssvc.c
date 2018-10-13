/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/wlnotify/senssvc.c
 * PURPOSE:     SENS service logon notifications
 * PROGRAMMER:  Eric Kohl <eric.kohl@reactos.org>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(wlnotify);


VOID
WINAPI
SensDisconnectEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    TRACE("SensDisconnectEvent\n");
    TRACE("Size: %lu\n", pInfo->Size);
    TRACE("Flags: %lx\n", pInfo->Flags);
    TRACE("UserName: %S\n", pInfo->UserName);
    TRACE("Domain: %S\n", pInfo->Domain);
    TRACE("WindowStation: %S\n", pInfo->WindowStation);
    TRACE("hToken: %p\n", pInfo->hToken);
    TRACE("hDesktop: %p\n", pInfo->hDesktop);
    TRACE("pStatusCallback: %p\n", pInfo->pStatusCallback);
}


VOID
WINAPI
SensLockEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    TRACE("SensLockEvent\n");
    TRACE("Size: %lu\n", pInfo->Size);
    TRACE("Flags: %lx\n", pInfo->Flags);
    TRACE("UserName: %S\n", pInfo->UserName);
    TRACE("Domain: %S\n", pInfo->Domain);
    TRACE("WindowStation: %S\n", pInfo->WindowStation);
    TRACE("hToken: %p\n", pInfo->hToken);
    TRACE("hDesktop: %p\n", pInfo->hDesktop);
    TRACE("pStatusCallback: %p\n", pInfo->pStatusCallback);
}


VOID
WINAPI
SensLogoffEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    TRACE("SensLogoffEvent\n");
    TRACE("Size: %lu\n", pInfo->Size);
    TRACE("Flags: %lx\n", pInfo->Flags);
    TRACE("UserName: %S\n", pInfo->UserName);
    TRACE("Domain: %S\n", pInfo->Domain);
    TRACE("WindowStation: %S\n", pInfo->WindowStation);
    TRACE("hToken: %p\n", pInfo->hToken);
    TRACE("hDesktop: %p\n", pInfo->hDesktop);
    TRACE("pStatusCallback: %p\n", pInfo->pStatusCallback);
}


VOID
WINAPI
SensLogonEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    TRACE("SensLogonEvent\n");
    TRACE("Size: %lu\n", pInfo->Size);
    TRACE("Flags: %lx\n", pInfo->Flags);
    TRACE("UserName: %S\n", pInfo->UserName);
    TRACE("Domain: %S\n", pInfo->Domain);
    TRACE("WindowStation: %S\n", pInfo->WindowStation);
    TRACE("hToken: %p\n", pInfo->hToken);
    TRACE("hDesktop: %p\n", pInfo->hDesktop);
    TRACE("pStatusCallback: %p\n", pInfo->pStatusCallback);
}


VOID
WINAPI
SensPostShellEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    TRACE("SensPostShellEvent\n");
    TRACE("Size: %lu\n", pInfo->Size);
    TRACE("Flags: %lx\n", pInfo->Flags);
    TRACE("UserName: %S\n", pInfo->UserName);
    TRACE("Domain: %S\n", pInfo->Domain);
    TRACE("WindowStation: %S\n", pInfo->WindowStation);
    TRACE("hToken: %p\n", pInfo->hToken);
    TRACE("hDesktop: %p\n", pInfo->hDesktop);
    TRACE("pStatusCallback: %p\n", pInfo->pStatusCallback);
}


VOID
WINAPI
SensReconnectEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    TRACE("SensReconnectEvent\n");
    TRACE("Size: %lu\n", pInfo->Size);
    TRACE("Flags: %lx\n", pInfo->Flags);
    TRACE("UserName: %S\n", pInfo->UserName);
    TRACE("Domain: %S\n", pInfo->Domain);
    TRACE("WindowStation: %S\n", pInfo->WindowStation);
    TRACE("hToken: %p\n", pInfo->hToken);
    TRACE("hDesktop: %p\n", pInfo->hDesktop);
    TRACE("pStatusCallback: %p\n", pInfo->pStatusCallback);
}


VOID
WINAPI
SensShutdownEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    TRACE("SensShutdownEvent\n");
    TRACE("Size: %lu\n", pInfo->Size);
    TRACE("Flags: %lx\n", pInfo->Flags);
    TRACE("UserName: %S\n", pInfo->UserName);
    TRACE("Domain: %S\n", pInfo->Domain);
    TRACE("WindowStation: %S\n", pInfo->WindowStation);
    TRACE("hToken: %p\n", pInfo->hToken);
    TRACE("hDesktop: %p\n", pInfo->hDesktop);
    TRACE("pStatusCallback: %p\n", pInfo->pStatusCallback);
}


VOID
WINAPI
SensStartScreenSaverEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    TRACE("SensStartScreenSaverEvent\n");
    TRACE("Size: %lu\n", pInfo->Size);
    TRACE("Flags: %lx\n", pInfo->Flags);
    TRACE("UserName: %S\n", pInfo->UserName);
    TRACE("Domain: %S\n", pInfo->Domain);
    TRACE("WindowStation: %S\n", pInfo->WindowStation);
    TRACE("hToken: %p\n", pInfo->hToken);
    TRACE("hDesktop: %p\n", pInfo->hDesktop);
    TRACE("pStatusCallback: %p\n", pInfo->pStatusCallback);
}


VOID
WINAPI
SensStartShellEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    TRACE("SensStartShellEvent\n");
    TRACE("Size: %lu\n", pInfo->Size);
    TRACE("Flags: %lx\n", pInfo->Flags);
    TRACE("UserName: %S\n", pInfo->UserName);
    TRACE("Domain: %S\n", pInfo->Domain);
    TRACE("WindowStation: %S\n", pInfo->WindowStation);
    TRACE("hToken: %p\n", pInfo->hToken);
    TRACE("hDesktop: %p\n", pInfo->hDesktop);
    TRACE("pStatusCallback: %p\n", pInfo->pStatusCallback);
}


VOID
WINAPI
SensStartupEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    TRACE("SensStartupEvent\n");
    TRACE("Size: %lu\n", pInfo->Size);
    TRACE("Flags: %lx\n", pInfo->Flags);
    TRACE("UserName: %S\n", pInfo->UserName);
    TRACE("Domain: %S\n", pInfo->Domain);
    TRACE("WindowStation: %S\n", pInfo->WindowStation);
    TRACE("hToken: %p\n", pInfo->hToken);
    TRACE("hDesktop: %p\n", pInfo->hDesktop);
    TRACE("pStatusCallback: %p\n", pInfo->pStatusCallback);
}


VOID
WINAPI
SensStopScreenSaverEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    TRACE("SensStopScreenSaverEvent\n");
    TRACE("Size: %lu\n", pInfo->Size);
    TRACE("Flags: %lx\n", pInfo->Flags);
    TRACE("UserName: %S\n", pInfo->UserName);
    TRACE("Domain: %S\n", pInfo->Domain);
    TRACE("WindowStation: %S\n", pInfo->WindowStation);
    TRACE("hToken: %p\n", pInfo->hToken);
    TRACE("hDesktop: %p\n", pInfo->hDesktop);
    TRACE("pStatusCallback: %p\n", pInfo->pStatusCallback);
}


VOID
WINAPI
SensUnlockEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    TRACE("SensUnlockEvent\n");
    TRACE("Size: %lu\n", pInfo->Size);
    TRACE("Flags: %lx\n", pInfo->Flags);
    TRACE("UserName: %S\n", pInfo->UserName);
    TRACE("Domain: %S\n", pInfo->Domain);
    TRACE("WindowStation: %S\n", pInfo->WindowStation);
    TRACE("hToken: %p\n", pInfo->hToken);
    TRACE("hDesktop: %p\n", pInfo->hDesktop);
    TRACE("pStatusCallback: %p\n", pInfo->pStatusCallback);
}

/* EOF */

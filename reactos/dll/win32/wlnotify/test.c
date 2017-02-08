/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/wlnotify/test.c
 * PURPOSE:     Winlogon notifications
 * PROGRAMMER:  Eric Kohl
 */

#include "precomp.h"

#define _NDEBUG
#include <debug.h>


VOID
WINAPI
TestLogonEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("TestLogonEvent\n");
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
TestLogoffEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("TestLogoffEvent\n");
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
TestLockEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("TestLockEvent\n");
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
TestUnlockEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("TestUnlockEvent\n");
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
TestStartupEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("TestStartupEvent\n");
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
TestShutdownEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("TestShutdownEvent\n");
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
TestStartScreenSaverEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("TestStartScreenSaverEvent\n");
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
TestStopScreenSaverEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("TestStopScreenSaverEvent\n");
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
TestStartShellEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("TestStartShellEvent\n");
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
TestPostShellEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("TestStartShellEvent\n");
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
TestDisconnectEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("TestDisconnectEvent\n");
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
TestReconnectEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("TestReconnectEvent\n");
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

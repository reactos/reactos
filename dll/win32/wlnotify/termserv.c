/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/wlnotify/termserv.c
 * PURPOSE:     Winlogon notifications
 * PROGRAMMER:  Eric Kohl
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(wlnotify);


VOID
WINAPI
TSEventDisconnect(
    PWLX_NOTIFICATION_INFO pInfo)
{
    TRACE("TSEventDisconnect\n");
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
TSEventLogoff(
    PWLX_NOTIFICATION_INFO pInfo)
{
    TRACE("TSEventLogoff\n");
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
TSEventLogon(
    PWLX_NOTIFICATION_INFO pInfo)
{
    TRACE("TSEventLogon\n");
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
TSEventPostShell(
    PWLX_NOTIFICATION_INFO pInfo)
{
    TRACE("TSEventPostShell\n");
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
TSEventReconnect(
    PWLX_NOTIFICATION_INFO pInfo)
{
    TRACE("TSEventReconnect\n");
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
TSEventShutdown(
    PWLX_NOTIFICATION_INFO pInfo)
{
    TRACE("TSEventShutdown\n");
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
TSEventStartShell(
    PWLX_NOTIFICATION_INFO pInfo)
{
    TRACE("TSEventStartShell\n");
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
TSEventStartup(
    PWLX_NOTIFICATION_INFO pInfo)
{
    TRACE("TSEventStartup\n");
    TRACE("Size: %lu\n", pInfo->Size);
    TRACE("Flags: %lx\n", pInfo->Flags);
    TRACE("UserName: %S\n", pInfo->UserName);
    TRACE("Domain: %S\n", pInfo->Domain);
    TRACE("WindowStation: %S\n", pInfo->WindowStation);
    TRACE("hToken: %p\n", pInfo->hToken);
    TRACE("hDesktop: %p\n", pInfo->hDesktop);
    TRACE("pStatusCallback: %p\n", pInfo->pStatusCallback);
}


/* TermsrvCreateTempDir */

/* EOF */

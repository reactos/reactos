/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/wlnotify/wlballoon.c
 * PURPOSE:     Winlogon notifications
 * PROGRAMMER:  Eric Kohl
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(wlnotify);


VOID
WINAPI
RegisterTicketExpiredNotificationEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    TRACE("RegisterTicketExpiredNotificationEvent\n");
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
UnregisterTicketExpiredNotificationEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    TRACE("UnregisterTicketExpiredNotificationEvent\n");
    TRACE("Size: %lu\n", pInfo->Size);
    TRACE("Flags: %lx\n", pInfo->Flags);
    TRACE("UserName: %S\n", pInfo->UserName);
    TRACE("Domain: %S\n", pInfo->Domain);
    TRACE("WindowStation: %S\n", pInfo->WindowStation);
    TRACE("hToken: %p\n", pInfo->hToken);
    TRACE("hDesktop: %p\n", pInfo->hDesktop);
    TRACE("pStatusCallback: %p\n", pInfo->pStatusCallback);
}


/* ShowNotificationBallonW */

/* EOF */

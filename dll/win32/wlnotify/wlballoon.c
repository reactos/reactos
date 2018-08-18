/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/wlnotify/wlballoon.c
 * PURPOSE:     Winlogon notifications
 * PROGRAMMER:  Eric Kohl
 */

#include "precomp.h"

#define _NDEBUG
#include <debug.h>


VOID
WINAPI
RegisterTicketExpiredNotificationEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("RegisterTicketExpiredNotificationEvent\n");
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
UnregisterTicketExpiredNotificationEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("UnregisterTicketExpiredNotificationEvent\n");
    DPRINT("Size: %lu\n", pInfo->Size);
    DPRINT("Flags: %lx\n", pInfo->Flags);
    DPRINT("UserName: %S\n", pInfo->UserName);
    DPRINT("Domain: %S\n", pInfo->Domain);
    DPRINT("WindowStation: %S\n", pInfo->WindowStation);
    DPRINT("hToken: %p\n", pInfo->hToken);
    DPRINT("hDesktop: %p\n", pInfo->hDesktop);
    DPRINT("pStatusCallback: %p\n", pInfo->pStatusCallback);
}


/* ShowNotificationBallonW */

/* EOF */

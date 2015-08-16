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
    DPRINT1("TestLogonEvent\n");
    DPRINT1("Size: %lu\n", pInfo->Size);
    DPRINT1("Flags: %lx\n", pInfo->Flags);
    DPRINT1("UserName: %S\n", pInfo->UserName);
    DPRINT1("Domain: %S\n", pInfo->Domain);
    DPRINT1("WindowStation: %S\n", pInfo->WindowStation);
    DPRINT1("hToken: %p\n", pInfo->hToken);
    DPRINT1("hDesktop: %p\n", pInfo->hDesktop);
    DPRINT1("pStatusCallback: %p\n", pInfo->pStatusCallback);
}


VOID
WINAPI
TestLogoffEvent(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT1("TestLogoffEvent\n");
    DPRINT1("Size: %lu\n", pInfo->Size);
    DPRINT1("Flags: %lx\n", pInfo->Flags);
    DPRINT1("UserName: %S\n", pInfo->UserName);
    DPRINT1("Domain: %S\n", pInfo->Domain);
    DPRINT1("WindowStation: %S\n", pInfo->WindowStation);
    DPRINT1("hToken: %p\n", pInfo->hToken);
    DPRINT1("hDesktop: %p\n", pInfo->hDesktop);
    DPRINT1("pStatusCallback: %p\n", pInfo->pStatusCallback);
}

/* EOF */

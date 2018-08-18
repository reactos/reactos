/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/wlnotify/scard.c
 * PURPOSE:     SCard logon notifications
 * PROGRAMMER:  Eric Kohl <eric.kohl@reactos.org>
 */

#include "precomp.h"
#include <winsvc.h>

#define _NDEBUG
#include <debug.h>

VOID
WINAPI
SCardResumeCertProp(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("SCardResumeCertProp\n");
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
SCardStartCertProp(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("SCardStartCertProp\n");
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
SCardStopCertProp(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("SCardStopCertProp\n");
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
SCardSuspendCertProp(
    PWLX_NOTIFICATION_INFO pInfo)
{
    DPRINT("SCardSuspendCertProp\n");
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

/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/wlnotify/scard.c
 * PURPOSE:     SCard logon notifications
 * PROGRAMMER:  Eric Kohl <eric.kohl@reactos.org>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(wlnotify);


VOID
WINAPI
SCardResumeCertProp(
    PWLX_NOTIFICATION_INFO pInfo)
{
    TRACE("SCardResumeCertProp\n");
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
SCardStartCertProp(
    PWLX_NOTIFICATION_INFO pInfo)
{
    TRACE("SCardStartCertProp\n");
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
SCardStopCertProp(
    PWLX_NOTIFICATION_INFO pInfo)
{
    TRACE("SCardStopCertProp\n");
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
SCardSuspendCertProp(
    PWLX_NOTIFICATION_INFO pInfo)
{
    TRACE("SCardSuspendCertProp\n");
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

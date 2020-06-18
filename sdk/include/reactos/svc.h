/*
 * PROJECT:     ReactOS Service Host
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        sdk/include/reactos/svc.h
 * PURPOSE:     Global Header for Service Host
 * PROGRAMMERS: ReactOS Portable Systems Group
 */
/* See https://www.geoffchappell.com/studies/windows/win32/services/svchost/process/globaldata.htm?tx=78 */

#ifndef __SVC_H
#define __SVC_H

//
// This is the callback that a hosted service can register for stop notification
//
typedef VOID
(CALLBACK *PSVCHOST_STOP_CALLBACK) (
    _In_ PVOID lpParameter,
    _In_ BOOLEAN TimerOrWaitFired
    );

//
// Hosted Services and SvcHost Use this Structure
//
typedef struct _SVCHOST_GLOBALS
{
    PVOID NullSid;
    PVOID WorldSid;
    PVOID LocalSid;
    PVOID NetworkSid;
    PVOID LocalSystemSid;
    PVOID LocalServiceSid;
    PVOID NetworkServiceSid;
    PVOID BuiltinDomainSid;
    PVOID AuthenticatedUserSid;
    PVOID AnonymousLogonSid;
    PVOID AliasAdminsSid;
    PVOID AliasUsersSid;
    PVOID AliasGuestsSid;
    PVOID AliasPowerUsersSid;
    PVOID AliasAccountOpsSid;
    PVOID AliasSystemOpsSid;
    PVOID AliasPrintOpsSid;
    PVOID AliasBackupOpsSid;
    PVOID RpcpStartRpcServer;
    PVOID RpcpStopRpcServer;
    PVOID RpcpStopRpcServerEx;
    PVOID SvcNetBiosOpen;
    PVOID SvcNetBiosClose;
    PVOID SvcNetBiosReset;
    PVOID SvcRegisterStopCallback;
} SVCHOST_GLOBALS, *PSVCHOST_GLOBALS;

#endif /* __SVC_H */

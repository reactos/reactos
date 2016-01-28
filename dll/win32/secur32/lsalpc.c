/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/secur32/lsalpc.c
 * PURPOSE:         LSA LPC port functions
 */

/* INCLUDES ******************************************************************/

#include "precomp.h"

#include <ndk/lpctypes.h>
#include <ndk/lpcfuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/obfuncs.h>
#include <psdk/ntsecapi.h>
#include <lsass/lsass.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(secur32);


/* GLOBALS *******************************************************************/

HANDLE LsaPortHandle;

extern HANDLE Secur32Heap;


/* FUNCTIONS *****************************************************************/

VOID
LsapInitLsaPort(VOID)
{
    LsaPortHandle = NULL;
}


VOID
LsapCloseLsaPort(VOID)
{
    if (LsaPortHandle != NULL)
    {
        NtClose(LsaPortHandle);
        LsaPortHandle = NULL;
    }
}


NTSTATUS
LsapOpenLsaPort(VOID)
{
    UNICODE_STRING PortName;
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    LSA_CONNECTION_INFO ConnectInfo;
    ULONG ConnectInfoLength;
    NTSTATUS Status;

    TRACE("LsapOpenLsaPort()\n");

    if (LsaPortHandle != NULL)
        return STATUS_SUCCESS;

    RtlInitUnicodeString(&PortName,
                         L"\\LsaAuthenticationPort");

    SecurityQos.Length              = sizeof(SecurityQos);
    SecurityQos.ImpersonationLevel  = SecurityIdentification;
    SecurityQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQos.EffectiveOnly       = TRUE;

    RtlZeroMemory(&ConnectInfo,
                  sizeof(ConnectInfo));

    ConnectInfo.CreateContext = FALSE;

    ConnectInfoLength = sizeof(LSA_CONNECTION_INFO);
    Status = NtConnectPort(&LsaPortHandle,
                           &PortName,
                           &SecurityQos,
                           NULL,
                           NULL,
                           NULL,
                           &ConnectInfo,
                           &ConnectInfoLength);
    if (!NT_SUCCESS(Status))
    {
        TRACE("NtConnectPort failed (Status 0x%08lx)\n", Status);
    }

    return Status;
/*
    if (!NT_SUCCESS(ConnectInfo.Status))
    {
        DPRINT1("ConnectInfo.Status: 0x%08lx\n", ConnectInfo.Status);
    }

    return ConnectInfo.Status;
*/
}


/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
LsaEnumerateLogonSessions(
    PULONG LogonSessionCount,
    PLUID *LogonSessionList)
{
#if 1
    LSA_API_MSG ApiMessage;
    NTSTATUS Status;

    TRACE("LsaEnumerateLogonSessions(%p %p)\n", LogonSessionCount, LogonSessionList);

    Status = LsapOpenLsaPort();
    if (!NT_SUCCESS(Status))
        return Status;

    ApiMessage.ApiNumber = LSASS_REQUEST_ENUM_LOGON_SESSIONS;
    ApiMessage.h.u1.s1.DataLength = LSA_PORT_DATA_SIZE(ApiMessage.EnumLogonSessions);
    ApiMessage.h.u1.s1.TotalLength = LSA_PORT_MESSAGE_SIZE;
    ApiMessage.h.u2.ZeroInit = 0;

    Status = NtRequestWaitReplyPort(LsaPortHandle,
                                    (PPORT_MESSAGE)&ApiMessage,
                                    (PPORT_MESSAGE)&ApiMessage);
    if (!NT_SUCCESS(Status))
    {
        ERR("NtRequestWaitReplyPort() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    if (!NT_SUCCESS(ApiMessage.Status))
    {
        ERR("NtRequestWaitReplyPort() failed (ApiMessage.Status 0x%08lx)\n", ApiMessage.Status);
        return ApiMessage.Status;
    }

    *LogonSessionCount = ApiMessage.EnumLogonSessions.Reply.LogonSessionCount;
    *LogonSessionList = ApiMessage.EnumLogonSessions.Reply.LogonSessionBuffer;

    return Status;
#else
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
#endif
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaGetLogonSessionData(
    PLUID LogonId,
    PSECURITY_LOGON_SESSION_DATA *ppLogonSessionData)
{
#if 1
    LSA_API_MSG ApiMessage;
    PSECURITY_LOGON_SESSION_DATA SessionData;
    NTSTATUS Status;

    TRACE("LsaGetLogonSessionData(%p %p)\n", LogonId, ppLogonSessionData);

    Status = LsapOpenLsaPort();
    if (!NT_SUCCESS(Status))
        return Status;

    ApiMessage.ApiNumber = LSASS_REQUEST_GET_LOGON_SESSION_DATA;
    ApiMessage.h.u1.s1.DataLength = LSA_PORT_DATA_SIZE(ApiMessage.GetLogonSessionData);
    ApiMessage.h.u1.s1.TotalLength = LSA_PORT_MESSAGE_SIZE;
    ApiMessage.h.u2.ZeroInit = 0;

    RtlCopyLuid(&ApiMessage.GetLogonSessionData.Request.LogonId,
                LogonId);

    Status = NtRequestWaitReplyPort(LsaPortHandle,
                                    (PPORT_MESSAGE)&ApiMessage,
                                    (PPORT_MESSAGE)&ApiMessage);
    if (!NT_SUCCESS(Status))
    {
        ERR("NtRequestWaitReplyPort() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    if (!NT_SUCCESS(ApiMessage.Status))
    {
        ERR("NtRequestWaitReplyPort() failed (ApiMessage.Status 0x%08lx)\n", ApiMessage.Status);
        return ApiMessage.Status;
    }

    SessionData = ApiMessage.GetLogonSessionData.Reply.SessionDataBuffer;

    if (SessionData->UserName.Buffer != NULL)
        SessionData->UserName.Buffer = (LPWSTR)((ULONG_PTR)&SessionData->UserName.Buffer + (ULONG_PTR)SessionData->UserName.Buffer);

    if (SessionData->Sid != NULL)
        SessionData->Sid = (LPWSTR)((ULONG_PTR)&SessionData->Sid + (ULONG_PTR)SessionData->Sid);

    *ppLogonSessionData = SessionData;

    return Status;
#else
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
#endif
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaRegisterPolicyChangeNotification(POLICY_NOTIFICATION_INFORMATION_CLASS InformationClass,
                                    HANDLE NotificationEventHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaUnregisterPolicyChangeNotification(POLICY_NOTIFICATION_INFORMATION_CLASS InformationClass,
                                      HANDLE NotificationEventHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */

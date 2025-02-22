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
    ConnectInfo.TrustedCaller = YES;

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
LsaConnectUntrusted(
    OUT PHANDLE LsaHandle)
{
    UNICODE_STRING PortName;
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    LSA_CONNECTION_INFO ConnectInfo;
    ULONG ConnectInfoLength = sizeof(ConnectInfo);
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING EventName;
    HANDLE EventHandle;
    NTSTATUS Status;

    TRACE("LsaConnectUntrusted(%p)\n", LsaHandle);

    // TODO: we may need to impersonate ourselves before, because we are untrusted!

    /* Wait for the LSA authentication thread */
    RtlInitUnicodeString(&EventName,
                         L"\\SECURITY\\LSA_AUTHENTICATION_INITIALIZED");
    InitializeObjectAttributes(&ObjectAttributes,
                               &EventName,
                               OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                               NULL,
                               NULL);
    Status = NtOpenEvent(&EventHandle,
                         SYNCHRONIZE,
                         &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        WARN("NtOpenEvent failed (Status 0x%08lx)\n", Status);

        Status = NtCreateEvent(&EventHandle,
                               SYNCHRONIZE,
                               &ObjectAttributes,
                               NotificationEvent,
                               FALSE);
        if (!NT_SUCCESS(Status))
        {
            WARN("NtCreateEvent failed (Status 0x%08lx)\n", Status);
            return Status;
        }
    }

    Status = NtWaitForSingleObject(EventHandle,
                                   TRUE,
                                   NULL);
    NtClose(EventHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("NtWaitForSingleObject failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Connect to the authentication port */
    RtlInitUnicodeString(&PortName,
                         L"\\LsaAuthenticationPort");

    SecurityQos.Length              = sizeof(SecurityQos);
    SecurityQos.ImpersonationLevel  = SecurityIdentification;
    SecurityQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQos.EffectiveOnly       = TRUE;

    RtlZeroMemory(&ConnectInfo,
                  ConnectInfoLength);

    ConnectInfo.CreateContext = TRUE;
    ConnectInfo.TrustedCaller = NO;

    Status = NtConnectPort(LsaHandle,
                           &PortName,
                           &SecurityQos,
                           NULL,
                           NULL,
                           NULL,
                           &ConnectInfo,
                           &ConnectInfoLength);
    if (!NT_SUCCESS(Status))
    {
        ERR("NtConnectPort failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    if (!NT_SUCCESS(ConnectInfo.Status))
    {
        ERR("ConnectInfo.Status: 0x%08lx\n", ConnectInfo.Status);
    }

    return ConnectInfo.Status;
}


/*
 * @implemented
 */
NTSTATUS
NTAPI
LsaEnumerateLogonSessions(
    PULONG LogonSessionCount,
    PLUID *LogonSessionList)
{
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

    TRACE("UserName: %p\n", SessionData->UserName.Buffer);
    if (SessionData->UserName.Buffer != NULL)
        SessionData->UserName.Buffer = (LPWSTR)((ULONG_PTR)SessionData + (ULONG_PTR)SessionData->UserName.Buffer);

    TRACE("LogonDomain: %p\n", SessionData->LogonDomain.Buffer);
    if (SessionData->LogonDomain.Buffer != NULL)
        SessionData->LogonDomain.Buffer = (LPWSTR)((ULONG_PTR)SessionData + (ULONG_PTR)SessionData->LogonDomain.Buffer);

    TRACE("AuthenticationPackage: %p\n", SessionData->AuthenticationPackage.Buffer);
    if (SessionData->AuthenticationPackage.Buffer != NULL)
        SessionData->AuthenticationPackage.Buffer = (LPWSTR)((ULONG_PTR)SessionData + (ULONG_PTR)SessionData->AuthenticationPackage.Buffer);

    TRACE("Sid: %p\n", SessionData->Sid);
    if (SessionData->Sid != NULL)
        SessionData->Sid = (LPWSTR)((ULONG_PTR)SessionData + (ULONG_PTR)SessionData->Sid);

    TRACE("LogonServer: %p\n", SessionData->LogonServer.Buffer);
    if (SessionData->LogonServer.Buffer != NULL)
        SessionData->LogonServer.Buffer = (LPWSTR)((ULONG_PTR)SessionData + (ULONG_PTR)SessionData->LogonServer.Buffer);

    TRACE("DnsDomainName: %p\n", SessionData->DnsDomainName.Buffer);
    if (SessionData->DnsDomainName.Buffer != NULL)
        SessionData->DnsDomainName.Buffer = (LPWSTR)((ULONG_PTR)SessionData + (ULONG_PTR)SessionData->DnsDomainName.Buffer);

    TRACE("Upn: %p\n", SessionData->Upn.Buffer);
    if (SessionData->Upn.Buffer != NULL)
        SessionData->Upn.Buffer = (LPWSTR)((ULONG_PTR)SessionData + (ULONG_PTR)SessionData->Upn.Buffer);

    *ppLogonSessionData = SessionData;

    return Status;
}


/*
 * @implemented
 */
NTSTATUS
NTAPI
LsaRegisterPolicyChangeNotification(
    POLICY_NOTIFICATION_INFORMATION_CLASS InformationClass,
    HANDLE NotificationEventHandle)
{
    LSA_API_MSG ApiMessage;
    NTSTATUS Status;

    TRACE("LsaRegisterPolicyChangeNotification(%lu %p)\n",
          InformationClass, NotificationEventHandle);

    Status = LsapOpenLsaPort();
    if (!NT_SUCCESS(Status))
        return Status;

    ApiMessage.ApiNumber = LSASS_REQUEST_POLICY_CHANGE_NOTIFY;
    ApiMessage.h.u1.s1.DataLength = LSA_PORT_DATA_SIZE(ApiMessage.PolicyChangeNotify);
    ApiMessage.h.u1.s1.TotalLength = LSA_PORT_MESSAGE_SIZE;
    ApiMessage.h.u2.ZeroInit = 0;

    ApiMessage.PolicyChangeNotify.Request.InformationClass = InformationClass;
    ApiMessage.PolicyChangeNotify.Request.NotificationEventHandle = NotificationEventHandle;
    ApiMessage.PolicyChangeNotify.Request.Register = TRUE;

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

    return Status;
}


/*
 * @implemented
 */
NTSTATUS
NTAPI
LsaUnregisterPolicyChangeNotification(
    POLICY_NOTIFICATION_INFORMATION_CLASS InformationClass,
    HANDLE NotificationEventHandle)
{
    LSA_API_MSG ApiMessage;
    NTSTATUS Status;

    TRACE("LsaUnregisterPolicyChangeNotification(%lu %p)\n",
          InformationClass, NotificationEventHandle);

    Status = LsapOpenLsaPort();
    if (!NT_SUCCESS(Status))
        return Status;

    ApiMessage.ApiNumber = LSASS_REQUEST_POLICY_CHANGE_NOTIFY;
    ApiMessage.h.u1.s1.DataLength = LSA_PORT_DATA_SIZE(ApiMessage.PolicyChangeNotify);
    ApiMessage.h.u1.s1.TotalLength = LSA_PORT_MESSAGE_SIZE;
    ApiMessage.h.u2.ZeroInit = 0;

    ApiMessage.PolicyChangeNotify.Request.InformationClass = InformationClass;
    ApiMessage.PolicyChangeNotify.Request.NotificationEventHandle = NotificationEventHandle;
    ApiMessage.PolicyChangeNotify.Request.Register = FALSE;

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

    return Status;
}

/* EOF */

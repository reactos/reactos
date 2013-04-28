/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/lsalib/lsa.c
 * PURPOSE:         Client-side LSA functions
 * UPDATE HISTORY:
 *                  Created 05/08/00
 */

/* INCLUDES ******************************************************************/

#include <ndk/lpctypes.h>
#include <ndk/lpcfuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/obfuncs.h>
#include <psdk/ntsecapi.h>
#include <lsass/lsass.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern HANDLE Secur32Heap;

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
NTSTATUS WINAPI
LsaDeregisterLogonProcess(HANDLE LsaHandle)
{
    LSA_API_MSG ApiMessage;
    NTSTATUS Status;

    DPRINT1("LsaDeregisterLogonProcess()\n");

    ApiMessage.ApiNumber = LSASS_REQUEST_DEREGISTER_LOGON_PROCESS;
    ApiMessage.h.u1.s1.DataLength = LSA_PORT_DATA_SIZE(ApiMessage.DeregisterLogonProcess);
    ApiMessage.h.u1.s1.TotalLength = LSA_PORT_MESSAGE_SIZE;
    ApiMessage.h.u2.ZeroInit = 0;

    Status = ZwRequestWaitReplyPort(LsaHandle,
                                    (PPORT_MESSAGE)&ApiMessage,
                                    (PPORT_MESSAGE)&ApiMessage);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwRequestWaitReplyPort() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    if (!NT_SUCCESS(ApiMessage.Status))
    {
        DPRINT1("ZwRequestWaitReplyPort() failed (ApiMessage.Status 0x%08lx)\n", ApiMessage.Status);
        return ApiMessage.Status;
    }

    NtClose(LsaHandle);

    DPRINT1("LsaDeregisterLogonProcess() done (Status 0x%08lx)\n", Status);

    return Status;
}


/*
 * @unimplemented
 */
NTSTATUS WINAPI
LsaConnectUntrusted(PHANDLE LsaHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @implemented
 */
NTSTATUS WINAPI
LsaCallAuthenticationPackage(HANDLE LsaHandle,
                             ULONG AuthenticationPackage,
                             PVOID ProtocolSubmitBuffer,
                             ULONG SubmitBufferLength,
                             PVOID *ProtocolReturnBuffer,
                             PULONG ReturnBufferLength,
                             PNTSTATUS ProtocolStatus)
{
    LSA_API_MSG ApiMessage;
    NTSTATUS Status;

    DPRINT1("LsaCallAuthenticationPackage()\n");

    ApiMessage.ApiNumber = LSASS_REQUEST_CALL_AUTHENTICATION_PACKAGE;
    ApiMessage.h.u1.s1.DataLength = LSA_PORT_DATA_SIZE(ApiMessage.CallAuthenticationPackage);
    ApiMessage.h.u1.s1.TotalLength = LSA_PORT_MESSAGE_SIZE;
    ApiMessage.h.u2.ZeroInit = 0;

    ApiMessage.CallAuthenticationPackage.Request.AuthenticationPackage = AuthenticationPackage;
    ApiMessage.CallAuthenticationPackage.Request.ProtocolSubmitBuffer = ProtocolSubmitBuffer;
    ApiMessage.CallAuthenticationPackage.Request.SubmitBufferLength = SubmitBufferLength;

    Status = ZwRequestWaitReplyPort(LsaHandle,
                                    (PPORT_MESSAGE)&ApiMessage,
                                    (PPORT_MESSAGE)&ApiMessage);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwRequestWaitReplyPort() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    if (!NT_SUCCESS(ApiMessage.Status))
    {
        DPRINT1("ZwRequestWaitReplyPort() failed (ApiMessage.Status 0x%08lx)\n", ApiMessage.Status);
        return ApiMessage.Status;
    }

    *ProtocolReturnBuffer = ApiMessage.CallAuthenticationPackage.Reply.ProtocolReturnBuffer;
    *ReturnBufferLength = ApiMessage.CallAuthenticationPackage.Reply.ReturnBufferLength;
    *ProtocolStatus = ApiMessage.CallAuthenticationPackage.Reply.ProtocolStatus;

    return Status;


#if 0
    PLSASS_REQUEST Request;
    PLSASS_REPLY Reply;
    LSASS_REQUEST RawRequest;
    LSASS_REPLY RawReply;
    NTSTATUS Status;
    ULONG OutBufferSize;

    Request = (PLSASS_REQUEST)&RawRequest;
    Reply = (PLSASS_REPLY)&RawReply;

    Request->Header.u1.s1.DataLength = sizeof(LSASS_REQUEST) + SubmitBufferLength -
        sizeof(PORT_MESSAGE);
    Request->Header.u1.s1.TotalLength =
        Request->Header.u1.s1.DataLength + sizeof(PORT_MESSAGE);
    Request->Type = LSASS_REQUEST_CALL_AUTHENTICATION_PACKAGE;
    Request->d.CallAuthenticationPackageRequest.AuthenticationPackage =
        AuthenticationPackage;
    Request->d.CallAuthenticationPackageRequest.InBufferLength =
        SubmitBufferLength;
    memcpy(Request->d.CallAuthenticationPackageRequest.InBuffer,
           ProtocolSubmitBuffer,
           SubmitBufferLength);

    Status = ZwRequestWaitReplyPort(LsaHandle,
                                    &Request->Header,
                                    &Reply->Header);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (!NT_SUCCESS(Reply->Status))
    {
        return Reply->Status;
    }

    OutBufferSize = Reply->d.CallAuthenticationPackageReply.OutBufferLength;
    *ProtocolReturnBuffer = RtlAllocateHeap(Secur32Heap,
                                            0,
                                            OutBufferSize);
    *ReturnBufferLength = OutBufferSize;
    memcpy(*ProtocolReturnBuffer,
           Reply->d.CallAuthenticationPackageReply.OutBuffer,
           *ReturnBufferLength);

    return Status;
#endif
}


/*
 * @implemented
 */
NTSTATUS WINAPI
LsaFreeReturnBuffer(PVOID Buffer)
{
    ULONG Length = 0;

    return ZwFreeVirtualMemory(NtCurrentProcess(),
                               &Buffer,
                               &Length,
                               MEM_RELEASE);
}


/*
 * @implemented
 */
NTSTATUS WINAPI
LsaLookupAuthenticationPackage(HANDLE LsaHandle,
                               PLSA_STRING PackageName,
                               PULONG AuthenticationPackage)
{
    LSA_API_MSG ApiMessage;
    NTSTATUS Status;

    /* Check the package name length */
    if (PackageName->Length > LSASS_MAX_PACKAGE_NAME_LENGTH)
    {
        return STATUS_NAME_TOO_LONG;
    }

    ApiMessage.ApiNumber = LSASS_REQUEST_LOOKUP_AUTHENTICATION_PACKAGE;
    ApiMessage.h.u1.s1.DataLength = LSA_PORT_DATA_SIZE(ApiMessage.LookupAuthenticationPackage);
    ApiMessage.h.u1.s1.TotalLength = LSA_PORT_MESSAGE_SIZE;
    ApiMessage.h.u2.ZeroInit = 0;

    ApiMessage.LookupAuthenticationPackage.Request.PackageNameLength = PackageName->Length;
    strncpy(ApiMessage.LookupAuthenticationPackage.Request.PackageName,
            PackageName->Buffer,
            ApiMessage.LookupAuthenticationPackage.Request.PackageNameLength);
    ApiMessage.LookupAuthenticationPackage.Request.PackageName[ApiMessage.LookupAuthenticationPackage.Request.PackageNameLength] = '\0';

    Status = ZwRequestWaitReplyPort(LsaHandle,
                                    (PPORT_MESSAGE)&ApiMessage,
                                    (PPORT_MESSAGE)&ApiMessage);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (!NT_SUCCESS(ApiMessage.Status))
    {
        return ApiMessage.Status;
    }

    *AuthenticationPackage = ApiMessage.LookupAuthenticationPackage.Reply.Package;

    return Status;
}


/*
 * @implemented
 */
NTSTATUS WINAPI
LsaLogonUser(HANDLE LsaHandle,
             PLSA_STRING OriginName,
             SECURITY_LOGON_TYPE LogonType,
             ULONG AuthenticationPackage,
             PVOID AuthenticationInformation,
             ULONG AuthenticationInformationLength,
             PTOKEN_GROUPS LocalGroups,
             PTOKEN_SOURCE SourceContext,
             PVOID *ProfileBuffer,
             PULONG ProfileBufferLength,
             PLUID LogonId,
             PHANDLE Token,
             PQUOTA_LIMITS Quotas,
             PNTSTATUS SubStatus)
{
    LSA_API_MSG ApiMessage;
    NTSTATUS Status;

    ApiMessage.ApiNumber = LSASS_REQUEST_LOGON_USER;
    ApiMessage.h.u1.s1.DataLength = LSA_PORT_DATA_SIZE(ApiMessage.LogonUser);
    ApiMessage.h.u1.s1.TotalLength = LSA_PORT_MESSAGE_SIZE;
    ApiMessage.h.u2.ZeroInit = 0;

    ApiMessage.LogonUser.Request.OriginName = *OriginName;
    ApiMessage.LogonUser.Request.LogonType = LogonType;
    ApiMessage.LogonUser.Request.AuthenticationPackage = AuthenticationPackage;
    ApiMessage.LogonUser.Request.AuthenticationInformation = AuthenticationInformation;
    ApiMessage.LogonUser.Request.AuthenticationInformationLength = AuthenticationInformationLength;
    ApiMessage.LogonUser.Request.LocalGroups = LocalGroups;
    if (LocalGroups != NULL)
        ApiMessage.LogonUser.Request.LocalGroupsCount = LocalGroups->GroupCount;
    else
        ApiMessage.LogonUser.Request.LocalGroupsCount = 0;
    ApiMessage.LogonUser.Request.SourceContext = *SourceContext;

    Status = ZwRequestWaitReplyPort(LsaHandle,
                                    (PPORT_MESSAGE)&ApiMessage,
                                    (PPORT_MESSAGE)&ApiMessage);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (!NT_SUCCESS(ApiMessage.Status))
    {
        return ApiMessage.Status;
    }

    *ProfileBuffer = ApiMessage.LogonUser.Reply.ProfileBuffer;
    *ProfileBufferLength = ApiMessage.LogonUser.Reply.ProfileBufferLength;
    *LogonId = ApiMessage.LogonUser.Reply.LogonId;
    *Token = ApiMessage.LogonUser.Reply.Token;
    *Quotas = ApiMessage.LogonUser.Reply.Quotas;
    *SubStatus = ApiMessage.LogonUser.Reply.SubStatus;

    return Status;

#if 0
    ULONG RequestLength;
    ULONG CurrentLength;
    PLSASS_REQUEST Request;
    LSASS_REQUEST RawMessage;
    PLSASS_REPLY Reply;
    LSASS_REPLY RawReply;
    NTSTATUS Status;

    RequestLength = sizeof(LSASS_REQUEST) - sizeof(PORT_MESSAGE);
    RequestLength = RequestLength + (OriginName->Length * sizeof(WCHAR));
    RequestLength = RequestLength + AuthenticationInformationLength;
    RequestLength = RequestLength +
        (LocalGroups->GroupCount * sizeof(SID_AND_ATTRIBUTES));

    CurrentLength = 0;
    Request = (PLSASS_REQUEST)&RawMessage;

    Request->d.LogonUserRequest.OriginNameLength = OriginName->Length;
    Request->d.LogonUserRequest.OriginName = (PWSTR)&RawMessage + CurrentLength;
    memcpy((PWSTR)&RawMessage + CurrentLength,
           OriginName->Buffer,
           OriginName->Length * sizeof(WCHAR));
    CurrentLength = CurrentLength + (OriginName->Length * sizeof(WCHAR));

    Request->d.LogonUserRequest.LogonType = LogonType;

    Request->d.LogonUserRequest.AuthenticationPackage =
        AuthenticationPackage;

    Request->d.LogonUserRequest.AuthenticationInformation =
        (PVOID)((ULONG_PTR)&RawMessage + CurrentLength);
    Request->d.LogonUserRequest.AuthenticationInformationLength =
        AuthenticationInformationLength;
    memcpy((PVOID)((ULONG_PTR)&RawMessage + CurrentLength),
           AuthenticationInformation,
           AuthenticationInformationLength);
    CurrentLength = CurrentLength + AuthenticationInformationLength;

    Request->d.LogonUserRequest.LocalGroupsCount = LocalGroups->GroupCount;
    Request->d.LogonUserRequest.LocalGroups =
        (PSID_AND_ATTRIBUTES)&RawMessage + CurrentLength;
    memcpy((PSID_AND_ATTRIBUTES)&RawMessage + CurrentLength,
           LocalGroups->Groups,
           LocalGroups->GroupCount * sizeof(SID_AND_ATTRIBUTES));

    Request->d.LogonUserRequest.SourceContext = *SourceContext;

    Request->Type = LSASS_REQUEST_LOGON_USER;
    Request->Header.u1.s1.DataLength = RequestLength - sizeof(PORT_MESSAGE);
    Request->Header.u1.s1.TotalLength = RequestLength + sizeof(PORT_MESSAGE);

    Reply = (PLSASS_REPLY)&RawReply;

    Status = ZwRequestWaitReplyPort(LsaHandle,
                                   &Request->Header,
                                   &Reply->Header);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    *SubStatus = Reply->d.LogonUserReply.SubStatus;

    if (!NT_SUCCESS(Reply->Status))
    {
        return Status;
    }

    *ProfileBuffer = RtlAllocateHeap(Secur32Heap,
                                     0,
                                     Reply->d.LogonUserReply.ProfileBufferLength);
    memcpy(*ProfileBuffer,
           (PVOID)((ULONG_PTR)Reply->d.LogonUserReply.Data +
                   (ULONG_PTR)Reply->d.LogonUserReply.ProfileBuffer),
           Reply->d.LogonUserReply.ProfileBufferLength);
    *LogonId = Reply->d.LogonUserReply.LogonId;
    *Token = Reply->d.LogonUserReply.Token;
    memcpy(Quotas,
           &Reply->d.LogonUserReply.Quotas,
           sizeof(Reply->d.LogonUserReply.Quotas));

    return Status;
#endif
}


/*
 * @implemented
 */
NTSTATUS WINAPI
LsaRegisterLogonProcess(PLSA_STRING LsaLogonProcessName,
                        PHANDLE Handle,
                        PLSA_OPERATIONAL_MODE OperationalMode)
{
    UNICODE_STRING PortName; // = RTL_CONSTANT_STRING(L"\\LsaAuthenticationPort");
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    LSA_CONNECTION_INFO ConnectInfo;
    ULONG ConnectInfoLength = sizeof(ConnectInfo);
    NTSTATUS Status;

    DPRINT1("LsaRegisterLogonProcess()\n");

    /* Check the logon process name length */
    if (LsaLogonProcessName->Length > LSASS_MAX_LOGON_PROCESS_NAME_LENGTH)
        return STATUS_NAME_TOO_LONG;

    RtlInitUnicodeString(&PortName,
                         L"\\LsaAuthenticationPort");

    SecurityQos.Length              = sizeof(SecurityQos);
    SecurityQos.ImpersonationLevel  = SecurityIdentification;
    SecurityQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQos.EffectiveOnly       = TRUE;

    strncpy(ConnectInfo.LogonProcessNameBuffer,
            LsaLogonProcessName->Buffer,
            LsaLogonProcessName->Length);
    ConnectInfo.Length = LsaLogonProcessName->Length;
    ConnectInfo.LogonProcessNameBuffer[ConnectInfo.Length] = '\0';

    Status = ZwConnectPort(Handle,
                           &PortName,
                           &SecurityQos,
                           NULL,
                           NULL,
                           NULL,
                           &ConnectInfo,
                           &ConnectInfoLength);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwConnectPort failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    DPRINT("ConnectInfo.OperationalMode: 0x%08lx\n", ConnectInfo.OperationalMode);
    *OperationalMode = ConnectInfo.OperationalMode;

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ConnectInfo.Status: 0x%08lx\n", ConnectInfo.Status);
    }

    return ConnectInfo.Status;
}


/*
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaEnumerateLogonSessions(PULONG LogonSessionCount,
                          PLUID *LogonSessionList)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaGetLogonSessionData(PLUID LogonId,
                       PSECURITY_LOGON_SESSION_DATA *ppLogonSessionData)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
WINAPI
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
WINAPI
LsaUnregisterPolicyChangeNotification(POLICY_NOTIFICATION_INFORMATION_CLASS InformationClass,
                                      HANDLE NotificationEventHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

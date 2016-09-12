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
NTSTATUS
WINAPI
LsaDeregisterLogonProcess(HANDLE LsaHandle)
{
    LSA_API_MSG ApiMessage;
    NTSTATUS Status;

    DPRINT("LsaDeregisterLogonProcess()\n");

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

    DPRINT("LsaDeregisterLogonProcess() done (Status 0x%08lx)\n", Status);

    return Status;
}


/*
 * @implemented
 */
NTSTATUS
WINAPI
LsaConnectUntrusted(PHANDLE LsaHandle)
{
    UNICODE_STRING PortName; // = RTL_CONSTANT_STRING(L"\\LsaAuthenticationPort");
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    LSA_CONNECTION_INFO ConnectInfo;
    ULONG ConnectInfoLength = sizeof(ConnectInfo);
    NTSTATUS Status;

    DPRINT("LsaConnectUntrusted(%p)\n", LsaHandle);

    RtlInitUnicodeString(&PortName,
                         L"\\LsaAuthenticationPort");

    SecurityQos.Length              = sizeof(SecurityQos);
    SecurityQos.ImpersonationLevel  = SecurityIdentification;
    SecurityQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQos.EffectiveOnly       = TRUE;

    RtlZeroMemory(&ConnectInfo,
                  ConnectInfoLength);

    ConnectInfo.CreateContext = TRUE;

    Status = ZwConnectPort(LsaHandle,
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

    if (!NT_SUCCESS(ConnectInfo.Status))
    {
        DPRINT1("ConnectInfo.Status: 0x%08lx\n", ConnectInfo.Status);
    }

    return ConnectInfo.Status;
}


/*
 * @implemented
 */
NTSTATUS
WINAPI
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
}


/*
 * @implemented
 */
NTSTATUS
WINAPI
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
NTSTATUS
WINAPI
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
NTSTATUS
WINAPI
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

    *SubStatus = ApiMessage.LogonUser.Reply.SubStatus;

    if (!NT_SUCCESS(ApiMessage.Status))
    {
        return ApiMessage.Status;
    }

    *ProfileBuffer = ApiMessage.LogonUser.Reply.ProfileBuffer;
    *ProfileBufferLength = ApiMessage.LogonUser.Reply.ProfileBufferLength;
    *LogonId = ApiMessage.LogonUser.Reply.LogonId;
    *Token = ApiMessage.LogonUser.Reply.Token;
    *Quotas = ApiMessage.LogonUser.Reply.Quotas;

    return Status;
}


/*
 * @implemented
 */
NTSTATUS
WINAPI
LsaRegisterLogonProcess(PLSA_STRING LsaLogonProcessName,
                        PHANDLE Handle,
                        PLSA_OPERATIONAL_MODE OperationalMode)
{
    UNICODE_STRING PortName; // = RTL_CONSTANT_STRING(L"\\LsaAuthenticationPort");
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    LSA_CONNECTION_INFO ConnectInfo;
    ULONG ConnectInfoLength = sizeof(ConnectInfo);
    NTSTATUS Status;

    DPRINT("LsaRegisterLogonProcess()\n");

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
    ConnectInfo.CreateContext = TRUE;

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

    if (!NT_SUCCESS(ConnectInfo.Status))
    {
        DPRINT1("ConnectInfo.Status: 0x%08lx\n", ConnectInfo.Status);
    }

    return ConnectInfo.Status;
}


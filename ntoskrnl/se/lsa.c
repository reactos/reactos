/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/se/sid.c
 * PURPOSE:         Security manager
 *
 * PROGRAMMERS:     David Welch <welch@cwcom.net>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaCallAuthenticationPackage(ULONG Unknown0,
                             ULONG Unknown1,
                             ULONG Unknown2,
                             ULONG Unknown3,
                             ULONG Unknown4,
                             ULONG Unknown5,
                             ULONG Unknown6)
{
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaDeregisterLogonProcess(ULONG Unknown0,
                          ULONG Unknown1)
{
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LsaFreeReturnBuffer(PVOID Buffer)
{
    ULONG Size = 0;
    return ZwFreeVirtualMemory(NtCurrentProcess(),
                               &Buffer,
                               &Size,
                               MEM_RELEASE);
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaLogonUser(IN HANDLE LsaHandle,
             IN PLSA_STRING OriginName,
             IN SECURITY_LOGON_TYPE LogonType,
             IN ULONG AuthenticationPackage,
             IN PVOID AuthenticationInformation,
             IN ULONG AuthenticationInformationLength,
             IN PTOKEN_GROUPS LocalGroups OPTIONAL,
             IN PTOKEN_SOURCE SourceContext,
             OUT PVOID *ProfileBuffer,
             OUT PULONG ProfileBufferLength,
             OUT PLUID LogonId,
             OUT PHANDLE Token,
             OUT PQUOTA_LIMITS Quotas,
             OUT PNTSTATUS SubStatus)
{
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaLookupAuthenticationPackage(ULONG Unknown0,
                               ULONG Unknown1,
                               ULONG Unknown2)
{
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaRegisterLogonProcess(IN PLSA_STRING LogonProcessName,
                        OUT PHANDLE LsaHandle,
                        OUT PLSA_OPERATIONAL_MODE SecurityMode)
{
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
SeMarkLogonSessionForTerminationNotification(IN PLUID LogonId)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
SeRegisterLogonSessionTerminatedRoutine(IN PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
SeUnregisterLogonSessionTerminatedRoutine(IN PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/* EOF */

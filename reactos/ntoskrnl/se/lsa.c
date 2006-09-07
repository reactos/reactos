/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/se/lsa.c
 * PURPOSE:         No purpose listed.
 *
 * PROGRAMMERS:     No programmer listed.
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* LsaCallAuthenticationPackage@28 */
/*
 * @unimplemented
 */
NTSTATUS STDCALL LsaCallAuthenticationPackage (
    ULONG Unknown0,
    ULONG Unknown1,
    ULONG Unknown2,
    ULONG Unknown3,
    ULONG Unknown4,
    ULONG Unknown5,
    ULONG Unknown6
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

/* LsaDeregisterLogonProcess@8 */
/*
 * @unimplemented
 */
NTSTATUS STDCALL LsaDeregisterLogonProcess (
    ULONG Unknown0,
    ULONG Unknown1
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

/* LsaFreeReturnBuffer@4 */
/*
 * @implemented
 */
NTSTATUS STDCALL LsaFreeReturnBuffer (PVOID Buffer)
{
    ULONG Size = 0; /* required by MEM_RELEASE */

    return ZwFreeVirtualMemory (
               NtCurrentProcess(),
	       & Buffer,
	       & Size,
	       MEM_RELEASE
               );
}

/* LsaLogonUser@56 */
/*
 * @unimplemented
 */
NTSTATUS STDCALL LsaLogonUser (
    ULONG Unknown0,
    ULONG Unknown1,
    ULONG Unknown2,
    ULONG Unknown3,
    ULONG Unknown4,
    ULONG Unknown5,
    ULONG Unknown6,
    ULONG Unknown7,
    ULONG Unknown8,
    ULONG Unknown9,
    ULONG Unknown10,
    ULONG Unknown11,
    ULONG Unknown12,
    ULONG Unknown13
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

/* LsaLookupAuthenticationPackage@12 */
/*
 * @unimplemented
 */
NTSTATUS STDCALL LsaLookupAuthenticationPackage (
    ULONG	Unknown0,
    ULONG	Unknown1,
    ULONG	Unknown2
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

/* LsaRegisterLogonProcess@12 */
/*
 * @unimplemented
 */
NTSTATUS STDCALL LsaRegisterLogonProcess (
    ULONG	Unknown0,
    ULONG	Unknown1,
    ULONG	Unknown2
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
SeMarkLogonSessionForTerminationNotification(
	IN PLUID LogonId
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
SeRegisterLogonSessionTerminatedRoutine(
	IN PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
SeUnregisterLogonSessionTerminatedRoutine(
	IN PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


/* EOF */

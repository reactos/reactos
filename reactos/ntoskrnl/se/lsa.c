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
    DWORD Unknown0,
    DWORD Unknown1,
    DWORD Unknown2,
    DWORD Unknown3,
    DWORD Unknown4,
    DWORD Unknown5,
    DWORD Unknown6
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

/* LsaDeregisterLogonProcess@8 */
/*
 * @unimplemented
 */
NTSTATUS STDCALL LsaDeregisterLogonProcess (
    DWORD Unknown0,
    DWORD Unknown1
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
    DWORD Unknown0,
    DWORD Unknown1,
    DWORD Unknown2,
    DWORD Unknown3,
    DWORD Unknown4,
    DWORD Unknown5,
    DWORD Unknown6,
    DWORD Unknown7,
    DWORD Unknown8,
    DWORD Unknown9,
    DWORD Unknown10,
    DWORD Unknown11,
    DWORD Unknown12,
    DWORD Unknown13
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

/* LsaLookupAuthenticationPackage@12 */
/*
 * @unimplemented
 */
NTSTATUS STDCALL LsaLookupAuthenticationPackage (
    DWORD	Unknown0,
    DWORD	Unknown1,
    DWORD	Unknown2
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

/* LsaRegisterLogonProcess@12 */
/*
 * @unimplemented
 */
NTSTATUS STDCALL LsaRegisterLogonProcess (
    DWORD	Unknown0,
    DWORD	Unknown1,
    DWORD	Unknown2
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

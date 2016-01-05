/*
* COPYRIGHT:       See COPYING.ARM in the top level directory
* PROJECT:         ReactOS UEFI Boot Manager
* FILE:            boot/environ/app/bootmgr/rtlcompat.c
* PURPOSE:         RTL Library Compatibility Routines
* PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include "bootmgr.h"

/* FUNCTIONS *****************************************************************/

#if DBG
VOID FASTCALL
CHECK_PAGED_CODE_RTL (
    char *file,
    int line
    )
{
    // boot-code is always ok
}
#endif

PVOID
NTAPI
RtlpAllocateMemory (
    _In_ ULONG Bytes,
    _In_ ULONG Tag
    )
{
    return NULL;
}

VOID
NTAPI
RtlpFreeMemory (
    _In_ PVOID Mem,
    _In_ ULONG Tag
    )
{
    return;
}

NTSTATUS
NTAPI
RtlpSafeCopyMemory (
    _Out_writes_bytes_all_(Length) VOID UNALIGNED *Destination,
    _In_reads_bytes_(Length) CONST VOID UNALIGNED *Source,
    _In_ SIZE_T Length
    )
{
    RtlCopyMemory(Destination, Source, Length);
    return STATUS_SUCCESS;
}

VOID
NTAPI
RtlAssert (
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    )
{

}

ULONG
DbgPrint (
    const char *Format,
    ...
    )
{
    return 0;
}

VOID
NTAPI
KeBugCheckEx(
    _In_ ULONG BugCheckCode,
    _In_ ULONG_PTR BugCheckParameter1,
    _In_ ULONG_PTR BugCheckParameter2,
    _In_ ULONG_PTR BugCheckParameter3,
    _In_ ULONG_PTR BugCheckParameter4)
{
    __assume(0);
}

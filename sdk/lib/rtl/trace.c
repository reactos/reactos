/*
 * PROJECT:         ReactOS Run-Time Library
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * PURPOSE:         Rtl Trace Routines
 */

/* INCLUDES *******************************************************************/

#include <rtl.h>
#define NDEBUG
#include <debug.h>

static RTL_UNLOAD_EVENT_TRACE RtlpUnloadEventTrace[RTL_UNLOAD_EVENT_TRACE_NUMBER];
static UINT RtlpUnloadEventTraceIndex = 0;

/* FUNCTIONS ******************************************************************/

PRTL_UNLOAD_EVENT_TRACE
NTAPI
RtlGetUnloadEventTrace(VOID)
{
    /* Just return a pointer to an array, according to MSDN */
    return RtlpUnloadEventTrace;
}

VOID
NTAPI
LdrpRecordUnloadEvent(_In_ PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    PIMAGE_NT_HEADERS NtHeaders;
    UINT Sequence = RtlpUnloadEventTraceIndex++;
    UINT Index = Sequence % RTL_UNLOAD_EVENT_TRACE_NUMBER;
    USHORT StringLen;

    DPRINT("LdrpRecordUnloadEvent(%wZ, %p - %p)\n", &LdrEntry->BaseDllName, LdrEntry->DllBase,
        (ULONG_PTR)LdrEntry->DllBase + LdrEntry->SizeOfImage);

    RtlpUnloadEventTrace[Index].BaseAddress = LdrEntry->DllBase;
    RtlpUnloadEventTrace[Index].SizeOfImage = LdrEntry->SizeOfImage;
    RtlpUnloadEventTrace[Index].Sequence = Sequence;

    NtHeaders = RtlImageNtHeader(LdrEntry->DllBase);

    if (NtHeaders)
    {
        RtlpUnloadEventTrace[Index].TimeDateStamp = NtHeaders->FileHeader.TimeDateStamp;
        RtlpUnloadEventTrace[Index].CheckSum = NtHeaders->OptionalHeader.CheckSum;
    }
    else
    {
        RtlpUnloadEventTrace[Index].TimeDateStamp = 0;
        RtlpUnloadEventTrace[Index].CheckSum = 0;
    }

    StringLen = min(LdrEntry->BaseDllName.Length / sizeof(WCHAR), RTL_NUMBER_OF(RtlpUnloadEventTrace[Index].ImageName));
    RtlCopyMemory(RtlpUnloadEventTrace[Index].ImageName, LdrEntry->BaseDllName.Buffer, StringLen * sizeof(WCHAR));
    if (StringLen < RTL_NUMBER_OF(RtlpUnloadEventTrace[Index].ImageName))
        RtlpUnloadEventTrace[Index].ImageName[StringLen] = 0;
}

BOOLEAN
NTAPI
RtlTraceDatabaseAdd(IN PRTL_TRACE_DATABASE Database,
                    IN ULONG Count,
                    IN PVOID *Trace,
                    OUT OPTIONAL PRTL_TRACE_BLOCK *TraceBlock)
{
    UNIMPLEMENTED;
    return FALSE;
}

PRTL_TRACE_DATABASE
NTAPI
RtlTraceDatabaseCreate(IN ULONG Buckets,
                       IN OPTIONAL SIZE_T MaximumSize,
                       IN ULONG Flags,
                       IN ULONG Tag,
                       IN OPTIONAL RTL_TRACE_HASH_FUNCTION HashFunction)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOLEAN
NTAPI
RtlTraceDatabaseDestroy(IN PRTL_TRACE_DATABASE Database)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOLEAN
NTAPI
RtlTraceDatabaseEnumerate(IN PRTL_TRACE_DATABASE Database,
                          IN PRTL_TRACE_ENUMERATE TraceEnumerate,
                          IN OUT PRTL_TRACE_BLOCK *TraceBlock)
{
    UNIMPLEMENTED;
    return FALSE;
}


BOOLEAN
NTAPI
RtlTraceDatabaseFind(IN PRTL_TRACE_DATABASE Database,
                     IN ULONG Count,
                     IN PVOID *Trace,
                     OUT OPTIONAL PRTL_TRACE_BLOCK *TraceBlock)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOLEAN
NTAPI
RtlTraceDatabaseLock(IN PRTL_TRACE_DATABASE Database)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOLEAN
NTAPI
RtlTraceDatabaseUnlock(IN PRTL_TRACE_DATABASE Database)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOLEAN
NTAPI
RtlTraceDatabaseValidate(IN PRTL_TRACE_DATABASE Database)
{
    UNIMPLEMENTED;
    return FALSE;
}

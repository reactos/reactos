#include "win32k.h"

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
_MmCopyFromCaller(
    _Out_writes_bytes_all_(Bytes) PVOID Target,
    _In_reads_bytes_(Bytes) PVOID Source,
    _In_ UINT Bytes)
{
    NTSTATUS Status;

    PAGED_CODE();
    ASSERT(ExGetPreviousMode() == UserMode);

    Status = STATUS_SUCCESS;
    _SEH2_TRY
    {
        ProbeForRead(Source, Bytes, 1);
        RtlCopyMemory(Target, Source, Bytes);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return Status;
}

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
_MmCopyToCaller(
    _Out_writes_bytes_all_(Bytes) PVOID Target,
    _In_reads_bytes_(Bytes) PVOID Source,
    _In_ UINT Bytes)
{
    NTSTATUS Status;

    PAGED_CODE();
    ASSERT(ExGetPreviousMode() == UserMode);

    Status = STATUS_SUCCESS;
    _SEH2_TRY
    {
        ProbeForWrite(Target, Bytes, 1);
        RtlCopyMemory(Target, Source, Bytes);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return Status;
}

#include "w32k.h"

NTSTATUS _MmCopyFromCaller( PVOID Target, PVOID Source, UINT Bytes ) {
    NTSTATUS Status = STATUS_SUCCESS;

    _SEH2_TRY
    {
        /* ProbeForRead(Source,Bytes,1); */
        RtlCopyMemory(Target,Source,Bytes);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return Status;
}

NTSTATUS _MmCopyToCaller( PVOID Target, PVOID Source, UINT Bytes ) {
    NTSTATUS Status = STATUS_SUCCESS;

    _SEH2_TRY
    {
        /* ProbeForWrite(Target,Bytes,1); */
        RtlCopyMemory(Target,Source,Bytes);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return Status;
}

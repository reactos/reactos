#include "w32k.h"

NTSTATUS _MmCopyFromCaller( PVOID Target, PVOID Source, UINT Bytes ) {
    NTSTATUS Status = STATUS_SUCCESS;
    
    _SEH_TRY {
        ProbeForRead(Source,Bytes,1);
        RtlCopyMemory(Target,Source,Bytes);
    } _SEH_HANDLE {
	Status = _SEH_GetExceptionCode();
    } _SEH_END;

    return Status;
}

NTSTATUS _MmCopyToCaller( PVOID Target, PVOID Source, UINT Bytes ) {
    NTSTATUS Status = STATUS_SUCCESS;
    
    _SEH_TRY {
        ProbeForWrite(Target,Bytes,1);
        RtlCopyMemory(Target,Source,Bytes);
    } _SEH_HANDLE {
	Status = _SEH_GetExceptionCode();
    } _SEH_END;

    return Status;
}

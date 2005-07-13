#include "w32k.h"

NTSTATUS _MmCopyFromCaller( PVOID Target, PVOID Source, UINT Bytes ) {
    NTSTATUS Status = STATUS_SUCCESS;

    _SEH_TRY {
	RtlCopyMemory(Target,Source,Bytes);
    } _SEH_HANDLE {
	Status = _SEH_GetExceptionCode();
    } _SEH_END;

    return Status;
}

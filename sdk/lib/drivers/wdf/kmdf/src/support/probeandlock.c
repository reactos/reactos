/*++

Copyright (c) Microsoft Corporation

Module Name:

    ProbeAndLock.c

Abstract:

    This module contains C routines for probing and locking
    down memory buffers.


Author:



Environment:

    Kernel mode only

Revision History:

--*/

//
// These routines must be implemented in a C file to avoid problems
// with C++ exception handling errors.
//

#include <ntddk.h>
#include <pseh/pseh2.h> // __REACTOS__

NTSTATUS
FxProbeAndLockForRead(
    __in  PMDL   Mdl,
    __in  KPROCESSOR_MODE AccessMode
    )
{
    _SEH2_TRY
    {
        MmProbeAndLockPages(Mdl, AccessMode, IoReadAccess);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}

NTSTATUS
FxProbeAndLockForWrite(
    __in  PMDL   Mdl,
    __in  KPROCESSOR_MODE AccessMode
    )
{
    _SEH2_TRY
    {
        MmProbeAndLockPages(Mdl, AccessMode, IoWriteAccess);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}

NTSTATUS
FxProbeAndLockWithAccess(
    __in  PMDL   Mdl,
    __in  KPROCESSOR_MODE AccessMode,
    __in  LOCK_OPERATION Operation
    )
{
    _SEH2_TRY
    {
        MmProbeAndLockPages(Mdl, AccessMode, Operation);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}

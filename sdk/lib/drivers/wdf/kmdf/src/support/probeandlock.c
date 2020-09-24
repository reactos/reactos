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

NTSTATUS
FxProbeAndLockForRead(
    __in  PMDL   Mdl,
    __in  KPROCESSOR_MODE AccessMode
    )
{
    try {
        MmProbeAndLockPages(Mdl, AccessMode, IoReadAccess);
    } except(EXCEPTION_EXECUTE_HANDLER) {
          return GetExceptionCode();
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FxProbeAndLockForWrite(
    __in  PMDL   Mdl,
    __in  KPROCESSOR_MODE AccessMode
    )
{
    try {
        MmProbeAndLockPages(Mdl, AccessMode, IoWriteAccess);
    } except(EXCEPTION_EXECUTE_HANDLER) {
          return GetExceptionCode();
    }
    return STATUS_SUCCESS;
}

NTSTATUS
FxProbeAndLockWithAccess(
    __in  PMDL   Mdl,
    __in  KPROCESSOR_MODE AccessMode,
    __in  LOCK_OPERATION Operation
    )
{
    try {
        MmProbeAndLockPages(Mdl, AccessMode, Operation);
    } except(EXCEPTION_EXECUTE_HANDLER) {
          return GetExceptionCode();
    }

    return STATUS_SUCCESS;
}



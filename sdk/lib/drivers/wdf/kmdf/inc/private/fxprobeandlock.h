//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef __FXPROBEANDLOCK_H__
#define __FXPROBEANDLOCK_H__

extern "C" {

//
// These are defined in a C file in src\support\ProbeAndLock.c
// to avoid C++ exception handling issues.
//
// They do not raise the exception beyond the C function, but
// translate it into an NTSTATUS before returning.
//

NTSTATUS
FxProbeAndLockForRead(
    __in  PMDL   Mdl,
    __in  KPROCESSOR_MODE AccessMode
    );

NTSTATUS
FxProbeAndLockForWrite(
    __in  PMDL   Mdl,
    __in  KPROCESSOR_MODE AccessMode
    );

NTSTATUS
FxProbeAndLockWithAccess(
    __in  PMDL   Mdl,
    __in  KPROCESSOR_MODE AccessMode,
    __in  LOCK_OPERATION Operation
    );

}

#endif // __FXPROBEANDLOCK_H__

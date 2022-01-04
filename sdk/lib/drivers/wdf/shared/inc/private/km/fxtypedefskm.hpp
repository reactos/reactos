/*++

Copyright (c) Microsoft Corporation

Module Name:

    fxtypedefsKm.hpp

Abstract:

    KMDF side defines for common names for the types

Author:



Environment:

    Kernel mode only

Revision History:

--*/

#pragma once

typedef FxDevice CfxDevice;
typedef FxDeviceBase CfxDeviceBase;
//typedef FxDeviceInterface CfxDeviceInterface;

typedef PIRP MdIrp;

typedef DRIVER_CANCEL MdCancelRoutineType, *MdCancelRoutine;
typedef IO_COMPLETION_ROUTINE MdCompletionRoutineType, *MdCompletionRoutine;
typedef REQUEST_POWER_COMPLETE MdRequestPowerCompleteType, *MdRequestPowerComplete;

typedef
NTSTATUS
(*PFX_COMPLETION_ROUTINE)(
    __in FxDevice *Device,
    __in FxIrp *Irp,
    __in PVOID Context
    );

typedef
VOID
(*PFX_CANCEL_ROUTINE)(
    __in FxDevice *Device,
    __in FxIrp *Irp,
    __in PVOID CancelContext
    );

//
// CSQ abstraction
//
typedef IO_CSQ_IRP_CONTEXT MdIoCsqIrpContext,*PMdIoCsqIrpContext;

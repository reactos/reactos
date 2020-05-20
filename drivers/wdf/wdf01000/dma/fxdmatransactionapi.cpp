/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     DMA transaction api functions
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */


#include "wdf.h"



extern "C" {

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDmaTransactionCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMAENABLER DmaEnabler,
    __in_opt
    WDF_OBJECT_ATTRIBUTES * Attributes,
    __out
    WDFDMATRANSACTION * DmaTransactionHandle
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDmaTransactionInitializeUsingRequest)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction,
    __in
    WDFREQUEST Request,
    __in
    PFN_WDF_PROGRAM_DMA EvtProgramDmaFunction,
    __in
    WDF_DMA_DIRECTION DmaDirection
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDmaTransactionExecute)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction,
    __in_opt
    WDFCONTEXT Context
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__success(TRUE)
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDmaTransactionRelease)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION  DmaTransaction
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
WDFEXPORT(WdfDmaTransactionDmaCompleted)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction,
    __out
    NTSTATUS * pStatus
    )
{
    WDFNOTIMPLEMENTED();
    return FALSE;
}

__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
WDFEXPORT(WdfDmaTransactionDmaCompletedWithLength)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction,
    __in
    size_t TransferredLength,
    __out
    NTSTATUS * pStatus
    )
{
    WDFNOTIMPLEMENTED();
    return FALSE;
}

__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
WDFEXPORT(WdfDmaTransactionDmaCompletedFinal)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction,
    __in
    size_t FinalTransferredLength,
    __out
    NTSTATUS * pStatus
    )
{
    WDFNOTIMPLEMENTED();
    return FALSE;
}

__drv_maxIRQL(DISPATCH_LEVEL)
size_t
WDFEXPORT(WdfDmaTransactionGetBytesTransferred)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction
    )
{
    WDFNOTIMPLEMENTED();
    return 0;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDmaTransactionSetMaximumLength)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction,
    __in
    size_t MaximumLength
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFREQUEST
WDFEXPORT(WdfDmaTransactionGetRequest)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction
    )
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
size_t
WDFEXPORT(WdfDmaTransactionGetCurrentDmaTransferLength)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction
    )
{
    WDFNOTIMPLEMENTED();
    return 0;
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFDEVICE
WDFEXPORT(WdfDmaTransactionGetDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction
    )
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDmaTransactionInitialize)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction,
    __in
    PFN_WDF_PROGRAM_DMA EvtProgramDmaFunction,
    __in
    WDF_DMA_DIRECTION DmaDirection,
    __in
    PMDL Mdl,
    __in    
    PVOID VirtualAddress,
    __in
    __drv_when(Length == 0, __drv_reportError(Length cannot be zero))
    size_t Length
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

} // extern "C"
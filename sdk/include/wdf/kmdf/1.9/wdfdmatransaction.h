/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

_WdfVersionBuild_

Module Name:

    WdfDmaTransaction.h

Abstract:

    WDF DMA Transaction support

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/

#ifndef _WDFDMATRANSACTION_H_
#define _WDFDMATRANSACTION_H_



#if (NTDDI_VERSION >= NTDDI_WIN2K)



typedef
__drv_functionClass(EVT_WDF_PROGRAM_DMA)
__drv_sameIRQL
__drv_requiresIRQL(DISPATCH_LEVEL)
BOOLEAN
EVT_WDF_PROGRAM_DMA(
    __in
    WDFDMATRANSACTION Transaction,
    __in
    WDFDEVICE Device,
    __in
    WDFCONTEXT Context,
    __in
    WDF_DMA_DIRECTION Direction,
    __in
    PSCATTER_GATHER_LIST SgList
    );

typedef EVT_WDF_PROGRAM_DMA *PFN_WDF_PROGRAM_DMA;

//
// WDF Function: WdfDmaTransactionCreate
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFDMATRANSACTIONCREATE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMAENABLER DmaEnabler,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFDMATRANSACTION* DmaTransaction
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfDmaTransactionCreate(
    __in
    WDFDMAENABLER DmaEnabler,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFDMATRANSACTION* DmaTransaction
    )
{
    return ((PFN_WDFDMATRANSACTIONCREATE) WdfFunctions[WdfDmaTransactionCreateTableIndex])(WdfDriverGlobals, DmaEnabler, Attributes, DmaTransaction);
}

//
// WDF Function: WdfDmaTransactionInitialize
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFDMATRANSACTIONINITIALIZE)(
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
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfDmaTransactionInitialize(
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
    return ((PFN_WDFDMATRANSACTIONINITIALIZE) WdfFunctions[WdfDmaTransactionInitializeTableIndex])(WdfDriverGlobals, DmaTransaction, EvtProgramDmaFunction, DmaDirection, Mdl, VirtualAddress, Length);
}

//
// WDF Function: WdfDmaTransactionInitializeUsingRequest
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFDMATRANSACTIONINITIALIZEUSINGREQUEST)(
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
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfDmaTransactionInitializeUsingRequest(
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
    return ((PFN_WDFDMATRANSACTIONINITIALIZEUSINGREQUEST) WdfFunctions[WdfDmaTransactionInitializeUsingRequestTableIndex])(WdfDriverGlobals, DmaTransaction, Request, EvtProgramDmaFunction, DmaDirection);
}

//
// WDF Function: WdfDmaTransactionExecute
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFDMATRANSACTIONEXECUTE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction,
    __in_opt
    WDFCONTEXT Context
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfDmaTransactionExecute(
    __in
    WDFDMATRANSACTION DmaTransaction,
    __in_opt
    WDFCONTEXT Context
    )
{
    return ((PFN_WDFDMATRANSACTIONEXECUTE) WdfFunctions[WdfDmaTransactionExecuteTableIndex])(WdfDriverGlobals, DmaTransaction, Context);
}

//
// WDF Function: WdfDmaTransactionRelease
//
typedef
__success(TRUE)
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFDMATRANSACTIONRELEASE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction
    );

__success(TRUE)
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfDmaTransactionRelease(
    __in
    WDFDMATRANSACTION DmaTransaction
    )
{
    return ((PFN_WDFDMATRANSACTIONRELEASE) WdfFunctions[WdfDmaTransactionReleaseTableIndex])(WdfDriverGlobals, DmaTransaction);
}

//
// WDF Function: WdfDmaTransactionDmaCompleted
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
(*PFN_WDFDMATRANSACTIONDMACOMPLETED)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction,
    __out
    NTSTATUS* Status
    );

__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
FORCEINLINE
WdfDmaTransactionDmaCompleted(
    __in
    WDFDMATRANSACTION DmaTransaction,
    __out
    NTSTATUS* Status
    )
{
    return ((PFN_WDFDMATRANSACTIONDMACOMPLETED) WdfFunctions[WdfDmaTransactionDmaCompletedTableIndex])(WdfDriverGlobals, DmaTransaction, Status);
}

//
// WDF Function: WdfDmaTransactionDmaCompletedWithLength
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
(*PFN_WDFDMATRANSACTIONDMACOMPLETEDWITHLENGTH)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction,
    __in
    size_t TransferredLength,
    __out
    NTSTATUS* Status
    );

__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
FORCEINLINE
WdfDmaTransactionDmaCompletedWithLength(
    __in
    WDFDMATRANSACTION DmaTransaction,
    __in
    size_t TransferredLength,
    __out
    NTSTATUS* Status
    )
{
    return ((PFN_WDFDMATRANSACTIONDMACOMPLETEDWITHLENGTH) WdfFunctions[WdfDmaTransactionDmaCompletedWithLengthTableIndex])(WdfDriverGlobals, DmaTransaction, TransferredLength, Status);
}

//
// WDF Function: WdfDmaTransactionDmaCompletedFinal
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
(*PFN_WDFDMATRANSACTIONDMACOMPLETEDFINAL)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction,
    __in
    size_t FinalTransferredLength,
    __out
    NTSTATUS* Status
    );

__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
FORCEINLINE
WdfDmaTransactionDmaCompletedFinal(
    __in
    WDFDMATRANSACTION DmaTransaction,
    __in
    size_t FinalTransferredLength,
    __out
    NTSTATUS* Status
    )
{
    return ((PFN_WDFDMATRANSACTIONDMACOMPLETEDFINAL) WdfFunctions[WdfDmaTransactionDmaCompletedFinalTableIndex])(WdfDriverGlobals, DmaTransaction, FinalTransferredLength, Status);
}

//
// WDF Function: WdfDmaTransactionGetBytesTransferred
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
size_t
(*PFN_WDFDMATRANSACTIONGETBYTESTRANSFERRED)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction
    );

__drv_maxIRQL(DISPATCH_LEVEL)
size_t
FORCEINLINE
WdfDmaTransactionGetBytesTransferred(
    __in
    WDFDMATRANSACTION DmaTransaction
    )
{
    return ((PFN_WDFDMATRANSACTIONGETBYTESTRANSFERRED) WdfFunctions[WdfDmaTransactionGetBytesTransferredTableIndex])(WdfDriverGlobals, DmaTransaction);
}

//
// WDF Function: WdfDmaTransactionSetMaximumLength
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFDMATRANSACTIONSETMAXIMUMLENGTH)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction,
    __in
    size_t MaximumLength
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfDmaTransactionSetMaximumLength(
    __in
    WDFDMATRANSACTION DmaTransaction,
    __in
    size_t MaximumLength
    )
{
    ((PFN_WDFDMATRANSACTIONSETMAXIMUMLENGTH) WdfFunctions[WdfDmaTransactionSetMaximumLengthTableIndex])(WdfDriverGlobals, DmaTransaction, MaximumLength);
}

//
// WDF Function: WdfDmaTransactionGetRequest
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
WDFREQUEST
(*PFN_WDFDMATRANSACTIONGETREQUEST)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFREQUEST
FORCEINLINE
WdfDmaTransactionGetRequest(
    __in
    WDFDMATRANSACTION DmaTransaction
    )
{
    return ((PFN_WDFDMATRANSACTIONGETREQUEST) WdfFunctions[WdfDmaTransactionGetRequestTableIndex])(WdfDriverGlobals, DmaTransaction);
}

//
// WDF Function: WdfDmaTransactionGetCurrentDmaTransferLength
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
size_t
(*PFN_WDFDMATRANSACTIONGETCURRENTDMATRANSFERLENGTH)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction
    );

__drv_maxIRQL(DISPATCH_LEVEL)
size_t
FORCEINLINE
WdfDmaTransactionGetCurrentDmaTransferLength(
    __in
    WDFDMATRANSACTION DmaTransaction
    )
{
    return ((PFN_WDFDMATRANSACTIONGETCURRENTDMATRANSFERLENGTH) WdfFunctions[WdfDmaTransactionGetCurrentDmaTransferLengthTableIndex])(WdfDriverGlobals, DmaTransaction);
}

//
// WDF Function: WdfDmaTransactionGetDevice
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
(*PFN_WDFDMATRANSACTIONGETDEVICE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFDEVICE
FORCEINLINE
WdfDmaTransactionGetDevice(
    __in
    WDFDMATRANSACTION DmaTransaction
    )
{
    return ((PFN_WDFDMATRANSACTIONGETDEVICE) WdfFunctions[WdfDmaTransactionGetDeviceTableIndex])(WdfDriverGlobals, DmaTransaction);
}



#endif // (NTDDI_VERSION >= NTDDI_WIN2K)


#endif // _WDFDMATRANSACTION_H_


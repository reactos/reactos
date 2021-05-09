/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDmaTransactionCallbacks.h

Abstract:

    This module implements the FxDmaTransaction object callbacks

Environment:

    kernel mode only

Revision History:

--*/

#ifndef _FXDMATRANSACTIONCALLBACKS_H
#define _FXDMATRANSACTIONCALLBACKS_H

//
// FxDmaTransactionProgramDma or FxDmaTransactionReserveDma callback delegate
// These are mutually exclusive callbacks and are packed together in
// the callback structure (C++ won't allow two classes with constructors
// to be together in a union, so the containing class can't do the
// packing)
//
class FxDmaTransactionProgramOrReserveDma : public FxCallback {

public:
    union {
        PFN_WDF_PROGRAM_DMA  ProgramDma;
        PFN_WDF_RESERVE_DMA  ReserveDma;
    } Method;

    FxDmaTransactionProgramOrReserveDma(
        VOID
        ) :
        FxCallback()
    {
        Method.ProgramDma = NULL;
    }

    BOOLEAN
    InvokeProgramDma(
        __in WDFDMATRANSACTION     Transaction,
        __in WDFDEVICE             Device,
        __in PVOID                 Context,
        __in WDF_DMA_DIRECTION     Direction,
        __in PSCATTER_GATHER_LIST  SgList
        )
    {
        if (Method.ProgramDma) {
            BOOLEAN cc;

            CallbackStart();
            cc = Method.ProgramDma( Transaction,
                                    Device,
                                    Context,
                                    Direction,
                                    SgList );
            CallbackEnd();

            return cc;
        }
        else {
            return FALSE;
        }
    }

    VOID
    InvokeReserveDma(
        __in WDFDMATRANSACTION     Transaction,
        __in PVOID                 Context
        )
    {
        if (Method.ReserveDma) {
            CallbackStart();
            Method.ReserveDma( Transaction, Context );
            CallbackEnd();
        }
    }

    VOID
    Clear(
        VOID
        )
    {
        Method.ProgramDma = NULL;
    }
};

//
// FxDmaTransactionConfigureChannel callback delegate
//

class FxDmaTransactionConfigureChannel : public FxCallback {

public:
    PFN_WDF_DMA_TRANSACTION_CONFIGURE_DMA_CHANNEL  Method;

    FxDmaTransactionConfigureChannel(
        VOID
        ) :
        FxCallback()
    {
        Method = NULL;
    }

    _Must_inspect_result_
    BOOLEAN
    Invoke(
        __in     WDFDMATRANSACTION DmaTransaction,
        __in     WDFDEVICE         Device,
        __in     PVOID             Context,
        __in_opt PMDL              Mdl,
        __in     size_t            Offset,
        __in     size_t            Length
        )
    {
        BOOLEAN b = TRUE;
        if (Method) {
            CallbackStart();
            b = Method( DmaTransaction, Device, Context, Mdl, Offset, Length );
            CallbackEnd();
        }
        return b;
    }
};

//
// FxDmaTransactionTransferComplete callback delegate
//

class FxDmaTransactionTransferComplete : public FxCallback {

public:
    PFN_WDF_DMA_TRANSACTION_DMA_TRANSFER_COMPLETE  Method;

    FxDmaTransactionTransferComplete(
        VOID
        ) :
        FxCallback()
    {
        Method = NULL;
    }

    VOID
    Invoke(
        __in WDFDMATRANSACTION Transaction,
        __in WDFDEVICE Device,
        __in WDFCONTEXT Context,
        __in WDF_DMA_DIRECTION Direction,
        __in DMA_COMPLETION_STATUS Status
        )
    {
        if (Method) {
            CallbackStart();
            Method( Transaction, Device, Context, Direction, Status );
            CallbackEnd();
        }
    }
};

#endif // _FXDMATRANSACTIONCALLBACKS_H

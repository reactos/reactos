/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    clntirp.c

Abstract:

    Client IRP queuing routines for CLASSPNP

Environment:

    kernel mode only

Notes:


Revision History:

--*/

#include "classp.h"

/*
 *  EnqueueDeferredClientIrp
 *
 *      Note: we currently do not support Cancel for storage irps.
 */
VOID NTAPI EnqueueDeferredClientIrp(PCLASS_PRIVATE_FDO_DATA FdoData, PIRP Irp)
{
    KIRQL oldIrql;

    KeAcquireSpinLock(&FdoData->SpinLock, &oldIrql);
    InsertTailList(&FdoData->DeferredClientIrpList, &Irp->Tail.Overlay.ListEntry);
    KeReleaseSpinLock(&FdoData->SpinLock, oldIrql);
}


/*
 *  DequeueDeferredClientIrp
 *
 */
PIRP NTAPI DequeueDeferredClientIrp(PCLASS_PRIVATE_FDO_DATA FdoData)
{
    KIRQL oldIrql;
    PLIST_ENTRY listEntry;
    PIRP irp;

    KeAcquireSpinLock(&FdoData->SpinLock, &oldIrql);
    if (IsListEmpty(&FdoData->DeferredClientIrpList)){
        listEntry = NULL;
    }
    else {
        listEntry = RemoveHeadList(&FdoData->DeferredClientIrpList);
    }
    KeReleaseSpinLock(&FdoData->SpinLock, oldIrql);

    if (listEntry == NULL) {
        irp = NULL;
    } else {
        irp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);
        ASSERT(irp->Type == IO_TYPE_IRP);
        InitializeListHead(&irp->Tail.Overlay.ListEntry);
    }

    return irp;
}

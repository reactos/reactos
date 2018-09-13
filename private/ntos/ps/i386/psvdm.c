/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    psldt.c

Abstract:

    This module contains code for the io port handler support

Author:

    Dave Hastings (daveh) 26 Jan 1991

Revision History:

--*/

#include "psp.h"


#if DBG
#define ASSERTEQUAL(value1, value2, string)     \
        if ((ULONG)value1 != (ULONG)value2) {   \
            DbgPrint string ;                   \
        }

#define ASSERTEQUALBREAK(value1, value2, string)\
        if ((ULONG)value1 != (ULONG)value2) {   \
            DbgPrint string ;                   \
            DbgBreakPoint();                    \
        }
#else

#define ASSERTEQUAL(value1, value2, string)
#define ASSERTEQUALBREAK(value1, value2, string)

#endif


//
// Internal functions
//

NTSTATUS
Psp386InstallIoHandler(
    IN PEPROCESS Process,
    IN PEMULATOR_ACCESS_ENTRY EmulatorAccessEntry,
    IN ULONG PortNumber,
    IN ULONG Context
    );

NTSTATUS
Psp386RemoveIoHandler(
    IN PEPROCESS Process,
    IN PEMULATOR_ACCESS_ENTRY EmulatorAccessEntry,
    IN ULONG PortNumber
    );

NTSTATUS
Psp386InsertVdmIoHandlerBlock(
    IN PEPROCESS Process,
    IN PVDM_IO_HANDLER VdmIoHandler
    );

PVDM_IO_HANDLER
Psp386GetVdmIoHandler(
    IN PEPROCESS Process,
    IN ULONG PortNumber
    );

NTSTATUS
Psp386CreateVdmIoListHead(
    IN PEPROCESS Process
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,PspVdmInitialize)
#pragma alloc_text(PAGE,PspSetProcessIoHandlers)
#pragma alloc_text(PAGE,Ps386GetVdmIoHandler)
#pragma alloc_text(PAGE,Psp386RemoveIoHandler)
#pragma alloc_text(PAGE,Psp386InstallIoHandler)
#pragma alloc_text(PAGE,Psp386CreateVdmIoListHead)
#pragma alloc_text(PAGE,Psp386InsertVdmIoHandlerBlock)
#pragma alloc_text(PAGE,Psp386GetVdmIoHandler)
#endif


//
//  Resource to synchronize access to IoHandler list
//
ERESOURCE VdmIoListCreationResource;




NTSTATUS
PspSetProcessIoHandlers(
    IN PEPROCESS Process,
    IN PVOID IoHandlerInformation,
    IN ULONG IoHandlerLength
    )
/*++

Routine Description:

    This routine installs a device driver IO handling routine for the
    specified process.  If an io operation is detected in a vdm on a port
    that has a device driver IO handling routine, that routine will be called.

Arguments:

    Process -- Supplies a pointer to the process for which Io port handlers
        are to be installed
    IoHandlerInformation -- Supplies a pointer to the information about the
        io port handlers
    IoHandlerLength -- Supplies the length of the IoHandlerInformation
        structure.

Return Value:



--*/
{
    PPROCESS_IO_PORT_HANDLER_INFORMATION IoHandlerInfo;
    NTSTATUS Status;
    PEMULATOR_ACCESS_ENTRY EmulatorAccess;
    ULONG EmulatorEntryNumber, NumberPorts;
    ULONG PortSize;
    PAGED_CODE();

    //
    // Insure that this call was made from KernelMode
    //
    if (KeGetPreviousMode() != KernelMode) {
        return STATUS_INVALID_PARAMETER;    // this info type invalid in usermode
    }
    //
    // Insure that the data passed is long enough
    //
    if (IoHandlerLength < (ULONG)sizeof( PROCESS_IO_PORT_HANDLER_INFORMATION)) {
        return STATUS_INFO_LENGTH_MISMATCH;
    }
    IoHandlerInfo = IoHandlerInformation;

    //
    // For each of the entries in the array that describes the handlers,
    // determine what size of port the specified handler is being installed
    // for, and then iterate over each individual port.
    //
    for (EmulatorEntryNumber = 0, EmulatorAccess =
            IoHandlerInfo->EmulatorAccessEntries;
        EmulatorEntryNumber < IoHandlerInfo->NumEntries;
        EmulatorEntryNumber++, EmulatorAccess++) {

            switch (EmulatorAccess->AccessType) {
            case Uchar:
                PortSize = 1;
                break;
            case Ushort:
                PortSize = 2;
                break;
            case Ulong:
                PortSize = 4;
            }

            for (NumberPorts = 0;
                NumberPorts < EmulatorAccess->NumConsecutivePorts;
                NumberPorts++) {
                    if (IoHandlerInfo->Install) {
                        Status = Psp386InstallIoHandler(
                            Process,
                            EmulatorAccess,
                            EmulatorAccess->BasePort + NumberPorts * PortSize,
                            IoHandlerInfo->Context
                            );
                        if (NT_SUCCESS(Status)) {
                        }
                    } else {
                        Status = Psp386RemoveIoHandler(
                            Process,
                            EmulatorAccess,
                            EmulatorAccess->BasePort + NumberPorts * PortSize
                            );
                    }
                    if (!NT_SUCCESS(Status)) {
                        goto exitloop;
                    }
            }
    }
    Status = STATUS_SUCCESS;
exitloop:
    return Status;

}


VOID
PspDeleteVdmObjects(
    IN PEPROCESS Process
    )
/*++

Routine Description:

    Frees the VdmObjects structure and the Frees the IoHandler list

Arguments:

    Process -- Supplies a pointer to the process

Return Value:

    None
--*/
{

    PVDM_PROCESS_OBJECTS pVdmObjects = Process->VdmObjects;
    PVDM_IO_HANDLER p1, p2;
    PVDM_IO_LISTHEAD p3;
    PLIST_ENTRY  Next;
    PDELAYINTIRQ pDelayIntIrq;
    KIRQL OldIrql;

    if (pVdmObjects == NULL)  {
        return;
    }

    //
    // First Free any port handler entries for this process,
    //
    p1 = NULL;
    p3 = pVdmObjects->VdmIoListHead;

    if (p3) {
        p2 = p3->VdmIoHandlerList;

        while (p2) {
            p1 = p2;
            p2 = p1->Next;
            ExFreePool( p1 );
        }

        ExDeleteResource(&p3->VdmIoResource);

        ExFreePool( p3 );
        pVdmObjects->VdmIoListHead = NULL;
    }

    if (pVdmObjects->pIcaUserData) {
        ExFreePool(pVdmObjects->pIcaUserData);
        }

    //
    // Free up the DelayedIntList, canceling pending timers.
    //
    KeAcquireSpinLock(&pVdmObjects->DelayIntSpinLock, &OldIrql);

    Next = pVdmObjects->DelayIntListHead.Flink;
    while (Next != &pVdmObjects->DelayIntListHead) {
        pDelayIntIrq = CONTAINING_RECORD(Next, DELAYINTIRQ, DelayIntListEntry);
        Next = Next->Flink;
        RemoveEntryList(&pDelayIntIrq->DelayIntListEntry);
        ExFreePool(pDelayIntIrq);
        }

    KeReleaseSpinLock(&pVdmObjects->DelayIntSpinLock, OldIrql);

    ExFreePool(Process->VdmObjects);
    Process->VdmObjects = NULL;
}



NTSTATUS
Psp386RemoveIoHandler(
    IN PEPROCESS Process,
    IN PEMULATOR_ACCESS_ENTRY EmulatorAccessEntry,
    IN ULONG PortNumber
    )
/*++

Routine Description:

    This routine remove a handler for a port.  On debug version, it will
    print a message if there is no handler.

Arguments:

    Process -- Supplies a pointer to the process
    EmulatorAccess -- Supplies a pointer to the information about the
        io port handler
    PortNumber -- Supplies the port number to remove the handler from.

Return Value:

--*/
{
    PVDM_PROCESS_OBJECTS pVdmObjects = Process->VdmObjects;
    PVDM_IO_HANDLER VdmIoHandler;
    KIRQL OldIrql;
    PAGED_CODE();

    //
    // Ensure we have a vdm process which is initialized
    // correctly for VdmIoHandlers
    //
    if (!pVdmObjects) {
#if DBG
        DbgPrint("Psp386RemoveIoHandler: uninitialized VdmObjects\n");
#endif
        return STATUS_UNSUCCESSFUL;
    }


    //
    // If the list does not have a head, then there are no handlers to
    // remove.
    //
    if (!pVdmObjects->VdmIoListHead) {
#if DBG
        DbgPrint("Psp386RemoveIoHandler : attempt to remove non-existent hdlr\n");
#endif
        return STATUS_SUCCESS;
    }

    //
    // Lock the list, so we can insure a correct update.
    //
    KeRaiseIrql(APC_LEVEL, &OldIrql);
    ExAcquireResourceExclusive(&pVdmObjects->VdmIoListHead->VdmIoResource,TRUE);

    VdmIoHandler = Psp386GetVdmIoHandler(
        Process,
        PortNumber & ~0x3
        );

    if (!VdmIoHandler) {
#if DBG
        DbgPrint("Psp386RemoveIoHandler : attempt to remove non-existent hdlr\n");
#endif
        ExReleaseResource(&pVdmObjects->VdmIoListHead->VdmIoResource);
        KeLowerIrql(OldIrql);
        return STATUS_SUCCESS;
    }

    ASSERTEQUALBREAK(
        VdmIoHandler->PortNumber,
        PortNumber & ~0x3,
        ("Psp386RemoveIoHandler : Bad pointer returned from GetVdmIoHandler\n")
        );

    if (EmulatorAccessEntry->AccessMode & EMULATOR_READ_ACCESS) {
        switch (EmulatorAccessEntry->AccessType) {
        case Uchar:
            if (EmulatorAccessEntry->StringSupport) {
                ASSERTEQUAL(
                    EmulatorAccessEntry->Routine,
                    VdmIoHandler->IoFunctions[0].UcharStringIo[PortNumber % 4],
                    ("Psp386RemoveIoHandler : UcharString fns don't match\n")
                    );
                VdmIoHandler->IoFunctions[0].UcharStringIo[PortNumber % 4] = NULL;
            } else {
                ASSERTEQUAL(
                    EmulatorAccessEntry->Routine,
                    VdmIoHandler->IoFunctions[0].UcharIo[PortNumber % 4],
                    ("Psp386RemoveIoHandler : Uchar fns don't match\n")
                    );
                VdmIoHandler->IoFunctions[0].UcharIo[PortNumber % 4] = NULL;
            }
            break;
        case Ushort:
            if (EmulatorAccessEntry->StringSupport) {
                ASSERTEQUAL(
                    EmulatorAccessEntry->Routine,
                    VdmIoHandler->IoFunctions[0].UshortStringIo[(PortNumber & 2) >> 1],
                    ("Psp386RemoveIoHandler : UshortString fns don't match\n")
                    );
                VdmIoHandler->IoFunctions[0].UshortStringIo[(PortNumber & 2) >> 1] = NULL;
            } else {
                ASSERTEQUAL(
                    EmulatorAccessEntry->Routine,
                    VdmIoHandler->IoFunctions[0].UshortIo[(PortNumber & 2) >> 1],
                    ("Psp386RemoveIoHandler : Ushort fns don't match\n")
                    );
                VdmIoHandler->IoFunctions[0].UshortIo[(PortNumber & 2) >> 1] = NULL;
            }
            break;
        case Ulong:
            if (EmulatorAccessEntry->StringSupport) {
                ASSERTEQUAL(
                    EmulatorAccessEntry->Routine,
                    VdmIoHandler->IoFunctions[0].UlongStringIo,
                    ("Psp386RemoveIoHandler : UlongString fns don't match\n")
                    );
                VdmIoHandler->IoFunctions[0].UlongStringIo = NULL;
            } else {
                ASSERTEQUAL(
                    EmulatorAccessEntry->Routine,
                    VdmIoHandler->IoFunctions[0].UlongIo,
                    ("Psp386RemoveIoHandler : Ulong fns don't match\n")
                    );
                VdmIoHandler->IoFunctions[0].UlongIo = NULL;
            }
            break;
        }
    }

    if (EmulatorAccessEntry->AccessMode & EMULATOR_WRITE_ACCESS) {
        switch (EmulatorAccessEntry->AccessType) {
        case Uchar:
            if (EmulatorAccessEntry->StringSupport) {
                ASSERTEQUAL(
                    EmulatorAccessEntry->Routine,
                    VdmIoHandler->IoFunctions[1].UcharStringIo[PortNumber % 4],
                    ("Psp386RemoveIoHandler : UcharString fns don't match\n")
                    );
                VdmIoHandler->IoFunctions[1].UcharStringIo[PortNumber % 4] = NULL;
            } else {
                ASSERTEQUAL(
                    EmulatorAccessEntry->Routine,
                    VdmIoHandler->IoFunctions[1].UcharIo[PortNumber % 4],
                    ("Psp386RemoveIoHandler : Uchar fns don't match\n")
                    );
                VdmIoHandler->IoFunctions[1].UcharIo[PortNumber % 4] = NULL;
            }
            break;
        case Ushort:
            if (EmulatorAccessEntry->StringSupport) {
                ASSERTEQUAL(
                    EmulatorAccessEntry->Routine,
                    VdmIoHandler->IoFunctions[1].UshortStringIo[(PortNumber & 2) >> 1],
                    ("Psp386RemoveIoHandler : UshortString fns don't match\n")
                    );
                VdmIoHandler->IoFunctions[1].UshortStringIo[(PortNumber & 2) >> 1] = NULL;
            } else {
                ASSERTEQUAL(
                    EmulatorAccessEntry->Routine,
                    VdmIoHandler->IoFunctions[1].UshortIo[(PortNumber & 2) >> 1],
                    ("Psp386RemoveIoHandler : Ushort fns don't match\n")
                    );
                VdmIoHandler->IoFunctions[1].UshortIo[(PortNumber & 2) >> 1] = NULL;
            }
            break;
        case Ulong:
            if (EmulatorAccessEntry->StringSupport) {
                ASSERTEQUAL(
                    EmulatorAccessEntry->Routine,
                    VdmIoHandler->IoFunctions[1].UlongStringIo,
                    ("Psp386RemoveIoHandler : UlongString fns don't match\n")
                    );
                VdmIoHandler->IoFunctions[1].UlongStringIo = NULL;
            } else {
                ASSERTEQUAL(
                    EmulatorAccessEntry->Routine,
                    VdmIoHandler->IoFunctions[1].UlongIo,
                    ("Psp386RemoveIoHandler : Ulong fns don't match\n")
                    );
                VdmIoHandler->IoFunctions[1].UlongIo = NULL;
            }
            break;
        }
    }

    ExReleaseResource(&pVdmObjects->VdmIoListHead->VdmIoResource);
    KeLowerIrql(OldIrql);

    return STATUS_SUCCESS;

}

NTSTATUS
Psp386InstallIoHandler(
    IN PEPROCESS Process,
    IN PEMULATOR_ACCESS_ENTRY EmulatorAccessEntry,
    IN ULONG PortNumber,
    IN ULONG Context
    )
/*++

Routine Description:

    This routine install a handler for a port.  On debug version, it will
    print a message if there is already a handler.

Arguments:

    Process -- Supplies a pointer to the process
    EmulatorAccess -- Supplies a pointer to the information about the
        io port handler
    PortNumber -- Supplies the port number to install the handler for.

Return Value:

--*/
{
    PVDM_PROCESS_OBJECTS pVdmObjects = Process->VdmObjects;
    PVDM_IO_HANDLER VdmIoHandler;
    NTSTATUS Status;
    KIRQL    OldIrql;
    PAGED_CODE();


    //
    // Ensure we have a vdm process which is initialized
    // correctly for VdmIoHandlers
    //
    if (!pVdmObjects) {
#if DBG
        DbgPrint("Psp386InstallIoHandler: uninitialized VdmObjects\n");
#endif
        return STATUS_UNSUCCESSFUL;
    }


    Status = STATUS_SUCCESS;

    //
    // If this is the first handler to be installed, create the list head,
    // and initialize the resource lock.
    //
    if (!pVdmObjects->VdmIoListHead) {
        Status = Psp386CreateVdmIoListHead(
            Process
            );

        if (!NT_SUCCESS(Status)) {
            return Status;
        }
    }

    //
    // Lock the list to insure correct update.
    //
    KeRaiseIrql(APC_LEVEL, &OldIrql);
    ExAcquireResourceExclusive(&pVdmObjects->VdmIoListHead->VdmIoResource,TRUE);

    //
    // Update Context
    //

    pVdmObjects->VdmIoListHead->Context = Context;

    VdmIoHandler = Psp386GetVdmIoHandler(
        Process,
        PortNumber & ~0x3
        );

    // If there isn't already a node for this block of ports,
    // attempt to allocate a new one.
    //
    if (!VdmIoHandler) {
        try {

            VdmIoHandler = ExAllocatePoolWithQuota(
                                PagedPool,
                                sizeof(VDM_IO_HANDLER)
                                );

        } except(EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
            if (VdmIoHandler) {
                ExFreePool(VdmIoHandler);
            }
        }

        if (!NT_SUCCESS(Status)) {
            ExReleaseResource(&pVdmObjects->VdmIoListHead->VdmIoResource);
            KeLowerIrql(OldIrql);
            return Status;
        }

        RtlZeroMemory(VdmIoHandler, sizeof(VDM_IO_HANDLER));
        VdmIoHandler->PortNumber = PortNumber & ~0x3;

        Status = Psp386InsertVdmIoHandlerBlock(
            Process,
            VdmIoHandler
            );

        if (!NT_SUCCESS(Status)) {
            ExReleaseResource(&pVdmObjects->VdmIoListHead->VdmIoResource);
            KeLowerIrql(OldIrql);
            return Status;
        }
    }

    ASSERTEQUALBREAK(
        VdmIoHandler->PortNumber,
        PortNumber & ~0x3,
        ("Psp386InstallIoHandler : Bad pointer returned from GetVdmIoHandler\n")
        );

    if (EmulatorAccessEntry->AccessMode & EMULATOR_READ_ACCESS) {
        switch (EmulatorAccessEntry->AccessType) {
        case Uchar:
            if (EmulatorAccessEntry->StringSupport) {
                ASSERTEQUALBREAK(
                    NULL,
                    VdmIoHandler->IoFunctions[0].UcharStringIo[PortNumber % 4],
                    ("Psp386InstallIoHandler : UcharString fns don't match\n")
                    );
                VdmIoHandler->IoFunctions[0].UcharStringIo[PortNumber % 4] =
                    (PDRIVER_IO_PORT_UCHAR_STRING)EmulatorAccessEntry->Routine;
            } else {
                ASSERTEQUALBREAK(
                    NULL,
                    VdmIoHandler->IoFunctions[0].UcharIo[PortNumber % 4],
                    ("Psp386InstallIoHandler : Uchar fns don't match\n")
                    );
                VdmIoHandler->IoFunctions[0].UcharIo[PortNumber % 4] =
                    (PDRIVER_IO_PORT_UCHAR)EmulatorAccessEntry->Routine;
            }
            break;
        case Ushort:
            if (EmulatorAccessEntry->StringSupport) {
                ASSERTEQUALBREAK(
                    NULL,
                    VdmIoHandler->IoFunctions[0].UshortStringIo[(PortNumber & 2) >> 1],
                    ("Psp386InstallIoHandler : UshortString fns don't match\n")
                    );
                VdmIoHandler->IoFunctions[0].UshortStringIo[(PortNumber & 2) >> 1] =
                    (PDRIVER_IO_PORT_USHORT_STRING)EmulatorAccessEntry->Routine;
            } else {
                ASSERTEQUALBREAK(
                    NULL,
                    VdmIoHandler->IoFunctions[0].UshortIo[(PortNumber & 2) >> 1],
                    ("Psp386InstallIoHandler : Ushort fns don't match\n")
                    );
                VdmIoHandler->IoFunctions[0].UshortIo[(PortNumber & 2) >> 1] =
                    (PDRIVER_IO_PORT_USHORT)EmulatorAccessEntry->Routine;
            }
            break;
        case Ulong:
            if (EmulatorAccessEntry->StringSupport) {
                ASSERTEQUALBREAK(
                    NULL,
                    VdmIoHandler->IoFunctions[0].UlongStringIo,
                    ("Psp386InstallIoHandler : UlongString fns don't match\n")
                    );
                VdmIoHandler->IoFunctions[0].UlongStringIo =
                    (PDRIVER_IO_PORT_ULONG_STRING)EmulatorAccessEntry->Routine;
            } else {
                ASSERTEQUALBREAK(
                    NULL,
                    VdmIoHandler->IoFunctions[0].UlongIo,
                    ("Psp386InstallIoHandler : Ulong fns don't match\n")
                    );
                VdmIoHandler->IoFunctions[0].UlongIo =
                    (PDRIVER_IO_PORT_ULONG)EmulatorAccessEntry->Routine;
            }
            break;
        }
    }

    if (EmulatorAccessEntry->AccessMode & EMULATOR_WRITE_ACCESS) {
        switch (EmulatorAccessEntry->AccessType) {
        case Uchar:
            if (EmulatorAccessEntry->StringSupport) {
                ASSERTEQUALBREAK(
                    NULL,
                    VdmIoHandler->IoFunctions[1].UcharStringIo[PortNumber % 4],
                    ("Psp386InstallIoHandler : UcharString fns don't match\n")
                    );
                VdmIoHandler->IoFunctions[1].UcharStringIo[PortNumber % 4] =
                    (PDRIVER_IO_PORT_UCHAR_STRING)EmulatorAccessEntry->Routine;
            } else {
                ASSERTEQUALBREAK(
                    NULL,
                    VdmIoHandler->IoFunctions[1].UcharIo[PortNumber % 4],
                    ("Psp386InstallIoHandler : Uchar fns don't match\n")
                    );
                VdmIoHandler->IoFunctions[1].UcharIo[PortNumber % 4] =
                    (PDRIVER_IO_PORT_UCHAR)EmulatorAccessEntry->Routine;
            }
            break;
        case Ushort:
            if (EmulatorAccessEntry->StringSupport) {
                ASSERTEQUALBREAK(
                    NULL,
                    VdmIoHandler->IoFunctions[1].UshortStringIo[(PortNumber & 2) >> 1],
                    ("Psp386InstallIoHandler : UshortString fns don't match\n")
                    );
                VdmIoHandler->IoFunctions[1].UshortStringIo[(PortNumber & 2) >> 1] =
                    (PDRIVER_IO_PORT_USHORT_STRING)EmulatorAccessEntry->Routine;
            } else {
                ASSERTEQUALBREAK(
                    NULL,
                    VdmIoHandler->IoFunctions[1].UshortIo[(PortNumber & 2) >> 1],
                    ("Psp386InstallIoHandler : Ushort fns don't match\n")
                    );
                VdmIoHandler->IoFunctions[1].UshortIo[(PortNumber & 2) >> 1] =
                    (PDRIVER_IO_PORT_USHORT)EmulatorAccessEntry->Routine;
            }
            break;
        case Ulong:
            if (EmulatorAccessEntry->StringSupport) {
                ASSERTEQUALBREAK(
                    NULL,
                    VdmIoHandler->IoFunctions[1].UlongStringIo,
                    ("Psp386InstallIoHandler : UlongString fns don't match\n")
                    );
                VdmIoHandler->IoFunctions[1].UlongStringIo =
                    (PDRIVER_IO_PORT_ULONG_STRING)EmulatorAccessEntry->Routine;
            } else {
                ASSERTEQUALBREAK(
                    NULL,
                    VdmIoHandler->IoFunctions[1].UlongIo,
                    ("Psp386InstallIoHandler : Ulong fns don't match\n")
                    );
                VdmIoHandler->IoFunctions[1].UlongIo =
                    (PDRIVER_IO_PORT_ULONG)EmulatorAccessEntry->Routine;
            }
        }
    }

    ExReleaseResource(&pVdmObjects->VdmIoListHead->VdmIoResource);
    KeLowerIrql(OldIrql);
    return STATUS_SUCCESS;

}



NTSTATUS
Psp386CreateVdmIoListHead(
    IN PEPROCESS Process
    )
/*++

Routine Description:

    This routine creates the head node of the Io handler list.  This node
    contains the spin lock that protects the list.  This routine also
    initializes the spin lock.

Arguments:

    Process -- Supplies a pointer to the process

Return Value:

Notes:

--*/
{
    PVDM_PROCESS_OBJECTS pVdmObjects = Process->VdmObjects;
    NTSTATUS Status;
    PVDM_IO_LISTHEAD HandlerListHead;
    KIRQL    OldIrql;
    PAGED_CODE();

    Status = STATUS_SUCCESS;

    // if there isn't yet a head, grab the resource lock and create one
    if (pVdmObjects->VdmIoListHead == NULL) {
        KeRaiseIrql(APC_LEVEL, &OldIrql);
        ExAcquireResourceExclusive(&VdmIoListCreationResource, TRUE);

        // if no head was created while we grabbed the spin lock
        if (pVdmObjects->VdmIoListHead == NULL) {

            try {
                // allocate space for the list head
                // and charge the quota for it

                HandlerListHead = ExAllocatePoolWithQuota(
                                       NonPagedPool,
                                       sizeof(VDM_IO_LISTHEAD)
                                       );

                } except(EXCEPTION_EXECUTE_HANDLER) {
                    Status = GetExceptionCode();
                    if (HandlerListHead) {
                        ExFreePool(HandlerListHead);
                    }
                }

            if ((!NT_SUCCESS(Status) || !HandlerListHead)) {
                ExReleaseResource(&VdmIoListCreationResource);
                KeLowerIrql(OldIrql);

                return (Status == STATUS_SUCCESS ?
                    STATUS_INSUFFICIENT_RESOURCES :
                    Status);

            }

            ExInitializeResource(&HandlerListHead->VdmIoResource);

            HandlerListHead->VdmIoHandlerList = NULL;

            //
            // Attach the list head to the process
            // and attach the handler to the list.
            // Since this was a new list

            pVdmObjects->VdmIoListHead = HandlerListHead;

            ExReleaseResource(&VdmIoListCreationResource);
            KeLowerIrql(OldIrql);


        }
    }
    return STATUS_SUCCESS;
}

NTSTATUS
Psp386InsertVdmIoHandlerBlock(
    IN PEPROCESS Process,
    IN PVDM_IO_HANDLER VdmIoHandler
    )
/*++

Routine Description:

    This routine inserts a new VdmIoHandler block into the process's io
    handler list.

Arguments:

    Process -- Supplies a pointer to the process
    VdmIoHandler -- Supplies a pointer to the block to insert.

Return Value:

--*/
{
    PVDM_PROCESS_OBJECTS pVdmObjects = Process->VdmObjects;
    PVDM_IO_HANDLER HandlerList, p;
    PVDM_IO_LISTHEAD HandlerListHead;
    PAGED_CODE();


    HandlerListHead = pVdmObjects->VdmIoListHead;
    HandlerList = HandlerListHead->VdmIoHandlerList;
    p = NULL;
    while ((HandlerList != NULL) &&
        (HandlerList->PortNumber < VdmIoHandler->PortNumber)) {
#if DBG
            if (HandlerList->PortNumber == VdmIoHandler->PortNumber) {
                DbgPrint("Ps386InsertVdmIoHandlerBlock : handler list corrupt\n");
            }
#endif
            p = HandlerList;
            HandlerList = HandlerList->Next;
    }

    if (p == NULL) { // Beginning of list
        VdmIoHandler->Next = HandlerListHead->VdmIoHandlerList;
        HandlerListHead->VdmIoHandlerList = VdmIoHandler;
    } else if (HandlerList == NULL) { // End of list
        p->Next = VdmIoHandler;
        VdmIoHandler->Next = NULL;
    } else { // Middle of list
        VdmIoHandler->Next = HandlerList;
        p->Next = VdmIoHandler;
    }

    return STATUS_SUCCESS;
}

BOOLEAN
Ps386GetVdmIoHandler(
    IN PEPROCESS Process,
    IN ULONG PortNumber,
    OUT PVDM_IO_HANDLER VdmIoHandler,
    OUT PULONG Context
    )
/*++

Routine Description:

    This routine finds the VdmIoHandler block for the specified port.

Arguments:

    Process -- Supplies a pointer to the process
    PortNumber -- Supplies the port number
    VdmIoHandler -- Supplies a pointer to the destination for the lookup

Returns:

    True -- A handler structure was found and copied
    False -- A handler structure was not found


--*/
{
    PVDM_PROCESS_OBJECTS pVdmObjects = Process->VdmObjects;
    PVDM_IO_HANDLER p;
    BOOLEAN Success;
    KIRQL   OldIrql;
    PAGED_CODE();

    ASSERT (pVdmObjects != NULL);

    if (PortNumber % 4) {
#if DBG
        DbgPrint(
            "Ps386GetVdmIoHandler : Invalid Port Number %lx\n",
            PortNumber
            );
#endif
        return FALSE;
    }

    if (!pVdmObjects->VdmIoListHead) {
        return FALSE;
    }


    KeRaiseIrql(APC_LEVEL, &OldIrql);
    ExAcquireResourceExclusive(&pVdmObjects->VdmIoListHead->VdmIoResource,TRUE);

    p = Psp386GetVdmIoHandler(
        Process,
        PortNumber
        );

    if (p) {
        *VdmIoHandler = *p;
        *Context = pVdmObjects->VdmIoListHead->Context;
        Success = TRUE;
    } else {
        Success = FALSE;
    }
    ExReleaseResource(&pVdmObjects->VdmIoListHead->VdmIoResource);
    KeLowerIrql(OldIrql);

    return Success;
}


PVDM_IO_HANDLER
Psp386GetVdmIoHandler(
    IN PEPROCESS Process,
    IN ULONG PortNumber
    )
/*++

Routine Description:

    This routine finds the VdmIoHandler block for the specified port.

Arguments:

    Process -- Supplies a pointer to the process
    PortNumber -- Supplies the port number

Returns:

    NULL  if no handler found
    non-NULL if handler found

--*/
{
    PVDM_PROCESS_OBJECTS pVdmObjects = Process->VdmObjects;
    PVDM_IO_HANDLER p;
    PAGED_CODE();

    if (PortNumber % 4) {
#if DBG
        DbgPrint(
            "Ps386GetVdmIoHandler : Invalid Port Number %lx\n",
            PortNumber
            );
#endif
        return NULL;
    }

    p = pVdmObjects->VdmIoListHead->VdmIoHandlerList;
    while ((p) && (p->PortNumber != PortNumber)) {
        p = p->Next;
    }

    return p;

}

NTSTATUS
PspVdmInitialize(
    )

/*++

Routine Description:

    This routine initializes the process based Vdm support for x86.

Arguments:

    None

Return Value:

    TBS
--*/
{
    return ExInitializeResource(&VdmIoListCreationResource);
}


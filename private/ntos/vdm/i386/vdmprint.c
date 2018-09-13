/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vdmprint.c

Abstract:

    This module contains the support for printing ports which could be
    handled in kernel without going to ntvdm.exe

Author:

    Sudeep Bharati (sudeepb) 16-Jan-1993

Revision History:
    William Hsieh (williamh) 31-May-1996
        rewrote for Dongle support

--*/


#include "vdmp.h"
#include "vdmprint.h"
#include <i386.h>
#include <v86emul.h>
#include "..\..\..\..\inc\ntddvdm.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, VdmPrinterStatus)
#pragma alloc_text(PAGE, VdmPrinterWriteData)
#pragma alloc_text(PAGE, VdmFlushPrinterWriteData)
#pragma alloc_text(PAGE, VdmpPrinterDirectIoOpen)
#pragma alloc_text(PAGE, VdmpPrinterDirectIoClose)
#endif



BOOLEAN
VdmPrinterStatus(
    ULONG iPort,
    ULONG cbInstructionSize,
    PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This routine handles the read operation on the printer status port

Arguments:
    iPort              - port on which the io was trapped
    cbInstructionSize  - Instruction size to update TsEip
    TrapFrame          - Trap Frame

Return Value:

    True if successfull, False otherwise

--*/
{
    UCHAR    PrtMode;
    USHORT   adapter;
    ULONG *  printer_status;
    KIRQL    OldIrql;
    PIO_STATUS_BLOCK IoStatusBlock;
    PVDM_PRINTER_INFO   PrtInfo;
    PVDM_TIB    VdmTib;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    // why we have printer stuff in TIB? It would be more appropriate
    // if we have them in the process object. The main reason(I think)
    // is that we can get rid of synchronization.
    //

    Status = VdmpGetVdmTib(&VdmTib, VDMTIB_KMODE);
    if (!NT_SUCCESS(Status)) {
       return(FALSE);
    }

    PrtInfo = &VdmTib->PrinterInfo;
    IoStatusBlock = (PIO_STATUS_BLOCK) &VdmTib->TempArea1;
    printer_status = &VdmTib->PrinterInfo.prt_Scratch;

    *pNtVDMState |= VDM_IDLEACTIVITY;
    try {
        // first, figure out which PRT we are dealing with. The
        // port addresses in the PrinterInfo are base address of each
        // PRT sorted in the adapter order.

        if ((USHORT)iPort == PrtInfo->prt_PortAddr[0] + STATUS_PORT_OFFSET)
            adapter = 0;
        else if ((USHORT)iPort == PrtInfo->prt_PortAddr[1] + STATUS_PORT_OFFSET)
            adapter = 1;
        else if ((USHORT)iPort == PrtInfo->prt_PortAddr[2] + STATUS_PORT_OFFSET)
            adapter = 2;
        else {
            // something must be wrong in our code, better check it out
            ASSERT(FALSE);
            return FALSE;
        }
        PrtMode = PrtInfo->prt_Mode[adapter];


        if(PRT_MODE_SIMULATE_STATUS_PORT == PrtMode) {
            // we are simulating printer status read.
            // get the current status from softpc.
            if (!(get_status(adapter) & NOTBUSY) &&
                !(host_lpt_status(adapter) & HOST_LPT_BUSY)) {
                if (get_control(adapter) & IRQ)
                    return FALSE;
                set_status(adapter, get_status(adapter) | NOTBUSY);
            }
            *printer_status = (ULONG)(get_status(adapter) | STATUS_REG_MASK);
        }
        else if (PRT_MODE_DIRECT_IO ==  PrtMode) {
            // we have to read the i/o directly(of course, through file system
            // which in turn goes to the driver).
            // Before performing read, flush out all pending output data in our
            // buffer. This is done because the status we are about to read
            // may depend on those pending output data.
            //
            if (PrtInfo->prt_BytesInBuffer[adapter]) {
                Status = VdmFlushPrinterWriteData(adapter);
#ifdef DBG
                if (!NT_SUCCESS(Status)) {
                    DbgPrint("VdmPrintStatus: failed to flush buffered data, status = %ls\n", Status);
                }
#endif
            }
            OldIrql = KeGetCurrentIrql();
            // lower irql to passive before doing any IO
            KeLowerIrql(PASSIVE_LEVEL);
            Status = NtDeviceIoControlFile(PrtInfo->prt_Handle[adapter],
                                           NULL,        // notification event
                                           NULL,        // APC routine
                                           NULL,        // Apc Context
                                           IoStatusBlock,
                                           IOCTL_VDM_PAR_READ_STATUS_PORT,
                                           NULL,
                                           0,
                                           printer_status,
                                           sizeof(ULONG)
                                           );

            if (!NT_SUCCESS(Status) || !NT_SUCCESS(IoStatusBlock->Status)) {
                // fake a status to make it looks like the port is not connected
                // to a printer.
                *printer_status = 0x7F;
#ifdef DBG
                DbgPrint("VdmPrinterStatus: failed to get status from printer, status = %lx\n", Status);
#endif
                // always tell the caller that we have simulated the operation.
                Status = STATUS_SUCCESS;
            }
            KeRaiseIrql(OldIrql, &OldIrql);
        }
        else
            // we don't simulate it here
            return FALSE;

        TrapFrame->Eax &= 0xffffff00;
        TrapFrame->Eax |= (UCHAR)*printer_status;
        TrapFrame->Eip += cbInstructionSize;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }
    return (NT_SUCCESS(Status));
}

BOOLEAN VdmPrinterWriteData(
    ULONG iPort,
    ULONG cbInstructionSize,
    PKTRAP_FRAME TrapFrame
    )
{
    UCHAR    PrtMode;
    PVDM_PRINTER_INFO   PrtInfo;
    USHORT   adapter;
    PVDM_TIB VdmTib;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    Status = VdmpGetVdmTib(&VdmTib, VDMTIB_KMODE);
    if (!NT_SUCCESS(Status)) {
       return(FALSE);
    }

    PrtInfo = &VdmTib->PrinterInfo;
    *pNtVDMState |= VDM_IDLEACTIVITY;

    try {
        // first, figure out which PRT we are dealing with. The
        // port addresses in the PrinterInfo are base address of each
        // PRT sorted in the adapter order
        if ((USHORT)iPort == PrtInfo->prt_PortAddr[0] + DATA_PORT_OFFSET)
            adapter = 0;
        else if ((USHORT)iPort == PrtInfo->prt_PortAddr[1] + DATA_PORT_OFFSET)
            adapter = 1;
        else if ((USHORT)iPort == PrtInfo->prt_PortAddr[2] + DATA_PORT_OFFSET)
            adapter = 2;
        else {
            // something must be wrong in our code, better check it out
            ASSERT(FALSE);
            return FALSE;
        }
        if (PRT_MODE_DIRECT_IO == PrtInfo->prt_Mode[adapter]){
            PrtInfo->prt_Buffer[adapter][PrtInfo->prt_BytesInBuffer[adapter]] = (UCHAR)TrapFrame->Eax;
            // buffer full, then flush it out
            if (++PrtInfo->prt_BytesInBuffer[adapter] >= PRT_DATA_BUFFER_SIZE){

                VdmFlushPrinterWriteData(adapter);
            }

            TrapFrame->Eip += cbInstructionSize;

        }
        else
            Status = STATUS_ILLEGAL_INSTRUCTION;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }
    return(NT_SUCCESS(Status));

}


NTSTATUS
VdmFlushPrinterWriteData(USHORT adapter)
{
    KIRQL    OldIrql;
    PVDM_TIB    VdmTib;
    PVDM_PRINTER_INFO PrtInfo;
    PIO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;


    PAGED_CODE();


    Status = VdmpGetVdmTib(&VdmTib, VDMTIB_KMODE);
    if (!NT_SUCCESS(Status)) {
       return(FALSE);
    }

    PrtInfo = &VdmTib->PrinterInfo;
    IoStatusBlock = (PIO_STATUS_BLOCK)&VdmTib->TempArea1;
    try {
        if (PrtInfo->prt_Handle[adapter] &&
            PrtInfo->prt_BytesInBuffer[adapter] &&
            PRT_MODE_DIRECT_IO == PrtInfo->prt_Mode[adapter]) {

            OldIrql = KeGetCurrentIrql();
            KeLowerIrql(PASSIVE_LEVEL);
            Status = NtDeviceIoControlFile(PrtInfo->prt_Handle[adapter],
                                           NULL,        // notification event
                                           NULL,        // APC routine
                                           NULL,        // APC context
                                           IoStatusBlock,
                                           IOCTL_VDM_PAR_WRITE_DATA_PORT,
                                           &PrtInfo->prt_Buffer[adapter][0],
                                           PrtInfo->prt_BytesInBuffer[adapter],
                                           NULL,
                                           0
                                           );
            PrtInfo->prt_BytesInBuffer[adapter] = 0;
            KeRaiseIrql(OldIrql, &OldIrql);
            if (!NT_SUCCESS(Status)) {
#ifdef DBG
                DbgPrint("IOCTL_VDM_PAR_WRITE_DATA_PORT failed %lx %x\n",
                         Status, IoStatusBlock->Status);
#endif
                Status = IoStatusBlock->Status;

            }
        }
        else
            Status = STATUS_INVALID_PARAMETER;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }
    return Status;

}


NTSTATUS
VdmpPrinterInitialize(
    PVOID ServiceData
    )
/*++

Routine Description:

    This routine probes and caches the data associated with kernel
    mode printer emulation.

Arguments:

    ServiceData - Not used.

Return Value:


--*/
{
    PUCHAR State, PrtStatus, Control, HostState;
    PVDM_TIB VdmTib;
    PVDM_PROCESS_OBJECTS VdmObjects;
    NTSTATUS Status;

    Status = STATUS_SUCCESS;

    //
    // Note:  We only support two printers in the kernel.
    //

    Status = VdmpGetVdmTib(&VdmTib, VDMTIB_KMODE);
    if (!NT_SUCCESS(Status)) {
       return(FALSE);
    }


    try {
        State = VdmTib->PrinterInfo.prt_State;
        PrtStatus = VdmTib->PrinterInfo.prt_Status;
        Control = VdmTib->PrinterInfo.prt_Control;
        HostState = VdmTib->PrinterInfo.prt_HostState;

        //
        // Probe the locations for two printers
        //
        ProbeForWrite(
            State,
            2 * sizeof(UCHAR),
            sizeof(UCHAR)
            );

        ProbeForWrite(
            PrtStatus,
            2 * sizeof(UCHAR),
            sizeof(UCHAR)
            );

        ProbeForWrite(
            Control,
            2 * sizeof(UCHAR),
            sizeof(UCHAR)
            );

        ProbeForWrite(
            HostState,
            2 * sizeof(UCHAR),
            sizeof(UCHAR)
            );

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    if (NT_SUCCESS(Status)) {
        VdmObjects = PsGetCurrentProcess()->VdmObjects;
        VdmObjects->PrinterState = State;
        VdmObjects->PrinterStatus = PrtStatus;
        VdmObjects->PrinterControl = Control;
        VdmObjects->PrinterHostState = HostState;
    }

    return Status;
}

NTSTATUS
VdmpPrinterDirectIoOpen(PVOID ServiceData)
{
    PAGED_CODE();

    return STATUS_SUCCESS;

}
NTSTATUS
VdmpPrinterDirectIoClose(PVOID ServiceData)
{
    NTSTATUS    Status;
    PVDM_PRINTER_INFO PrtInfo;
    USHORT Adapter;
    PVDM_TIB VdmTib;

    PAGED_CODE();

    Status = STATUS_SUCCESS;

    // first we fetch vdm tib and do some damage control in case
    // this is bad user-mode memory
    // PrtInfo points to a stricture

    try {
        VdmTib = NtCurrentTeb()->Vdm;
        ProbeForWrite(VdmTib, sizeof(VDM_TIB), sizeof(UCHAR));

        // now verify that servicedata ptr is valid
        ProbeForRead(ServiceData, sizeof(USHORT), sizeof(UCHAR));
        Adapter = *(USHORT *)ServiceData;

    } except (ExSystemExceptionFilter()) {
        Status = GetExceptionCode();
    }

    if (NULL == VdmTib || NULL == ServiceData) {
       Status = STATUS_ACCESS_VIOLATION;
    }

    if (!NT_SUCCESS(Status)) {
       return(Status);
    }

    PrtInfo =&VdmTib->PrinterInfo;

    try {
             if (Adapter < VDM_NUMBER_OF_LPT) {
                if (PRT_MODE_DIRECT_IO == PrtInfo->prt_Mode[Adapter] &&
                         PrtInfo->prt_BytesInBuffer[Adapter]) {
                        Status = VdmFlushPrinterWriteData(Adapter);
                }
             }
        else {
                Status = STATUS_INVALID_PARAMETER;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
             Status     = GetExceptionCode();
    }
    return Status;
}

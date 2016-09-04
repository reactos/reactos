/* Copyright (c) Mark Harmstone 2016
 * 
 * This file is part of WinBtrfs.
 * 
 * WinBtrfs is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public Licence as published by
 * the Free Software Foundation, either version 3 of the Licence, or
 * (at your option) any later version.
 * 
 * WinBtrfs is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public Licence for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public Licence
 * along with WinBtrfs.  If not, see <http://www.gnu.org/licenses/>. */

#include "btrfs_drv.h"

void do_read_job(PIRP Irp) {
    NTSTATUS Status;
    ULONG bytes_read;
    BOOL top_level = is_top_level(Irp);
    
    Irp->IoStatus.Information = 0;
    
    Status = do_read(Irp, TRUE, &bytes_read);

    Irp->IoStatus.Status = Status;
    
//     // fastfat doesn't do this, but the Wine ntdll file test seems to think we ought to
//     if (Irp->UserIosb)
//         *Irp->UserIosb = Irp->IoStatus;
    
    TRACE("Irp->IoStatus.Status = %08x\n", Irp->IoStatus.Status);
    TRACE("Irp->IoStatus.Information = %lu\n", Irp->IoStatus.Information);
    TRACE("returning %08x\n", Status);
    
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    if (top_level) 
        IoSetTopLevelIrp(NULL);
}

void do_write_job(device_extension* Vcb, PIRP Irp) {
    BOOL top_level = is_top_level(Irp);
    NTSTATUS Status;
    
    _SEH2_TRY {
        Status = write_file(Vcb, Irp, TRUE, TRUE);
    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;
    
    Irp->IoStatus.Status = Status;

    TRACE("wrote %u bytes\n", Irp->IoStatus.Information);
    
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    TRACE("returning %08x\n", Status);
}

static void do_job(drv_thread* thread, LIST_ENTRY* le) {
    thread_job* tj = CONTAINING_RECORD(le, thread_job, list_entry);
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(tj->Irp);
    
    if (IrpSp->MajorFunction == IRP_MJ_READ) {
        do_read_job(tj->Irp);
    } else if (IrpSp->MajorFunction == IRP_MJ_WRITE) {
        do_write_job(thread->DeviceObject->DeviceExtension, tj->Irp);
    } else {
        ERR("unsupported major function %x\n", IrpSp->MajorFunction);
        tj->Irp->IoStatus.Status = STATUS_INTERNAL_ERROR;
        tj->Irp->IoStatus.Information = 0;
        IoCompleteRequest(tj->Irp, IO_NO_INCREMENT);
    }
    
    ExFreePool(tj);
}

void STDCALL worker_thread(void* context) {
    drv_thread* thread = context;
    KIRQL irql;
    
    ObReferenceObject(thread->DeviceObject);
    
    while (TRUE) {
        KeWaitForSingleObject(&thread->event, Executive, KernelMode, FALSE, NULL);
        
        FsRtlEnterFileSystem();
        
        while (TRUE) {
            LIST_ENTRY* le;
            device_extension* Vcb = thread->DeviceObject->DeviceExtension;
            
            KeAcquireSpinLock(&thread->spin_lock, &irql);
            
            if (IsListEmpty(&thread->jobs)) {
                KeReleaseSpinLock(&thread->spin_lock, irql);
                break;
            }
            
            le = thread->jobs.Flink;
            RemoveEntryList(le);
            
            KeReleaseSpinLock(&thread->spin_lock, irql);
            
            InterlockedDecrement(&Vcb->threads.pending_jobs);
            do_job(thread, le);
        }
        
        FsRtlExitFileSystem();
        
        if (thread->quit)
            break;
    }
    
    ObDereferenceObject(thread->DeviceObject);
    
    KeSetEvent(&thread->finished, 0, FALSE);
    
    PsTerminateSystemThread(STATUS_SUCCESS);
}

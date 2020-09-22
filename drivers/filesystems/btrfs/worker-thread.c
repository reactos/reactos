/* Copyright (c) Mark Harmstone 2016-17
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

typedef struct {
    device_extension* Vcb;
    PIRP Irp;
    WORK_QUEUE_ITEM item;
} job_info;

NTSTATUS do_read_job(PIRP Irp) {
    NTSTATUS Status;
    ULONG bytes_read;
    bool top_level = is_top_level(Irp);
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    fcb* fcb = FileObject->FsContext;
    bool acquired_fcb_lock = false;

    Irp->IoStatus.Information = 0;

    if (!ExIsResourceAcquiredSharedLite(fcb->Header.Resource)) {
        ExAcquireResourceSharedLite(fcb->Header.Resource, true);
        acquired_fcb_lock = true;
    }

    _SEH2_TRY {
        Status = do_read(Irp, true, &bytes_read);
    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;

    if (acquired_fcb_lock)
        ExReleaseResourceLite(fcb->Header.Resource);

    if (!NT_SUCCESS(Status))
        ERR("do_read returned %08lx\n", Status);

    Irp->IoStatus.Status = Status;

    TRACE("read %Iu bytes\n", Irp->IoStatus.Information);

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    if (top_level)
        IoSetTopLevelIrp(NULL);

    TRACE("returning %08lx\n", Status);

    return Status;
}

NTSTATUS do_write_job(device_extension* Vcb, PIRP Irp) {
    bool top_level = is_top_level(Irp);
    NTSTATUS Status;

    _SEH2_TRY {
        Status = write_file(Vcb, Irp, true, true);
    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;

    if (!NT_SUCCESS(Status))
        ERR("write_file returned %08lx\n", Status);

    Irp->IoStatus.Status = Status;

    TRACE("wrote %Iu bytes\n", Irp->IoStatus.Information);

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    if (top_level)
        IoSetTopLevelIrp(NULL);

    TRACE("returning %08lx\n", Status);

    return Status;
}

_Function_class_(WORKER_THREAD_ROUTINE)
static void __stdcall do_job(void* context) {
    job_info* ji = context;
    PIO_STACK_LOCATION IrpSp = ji->Irp ? IoGetCurrentIrpStackLocation(ji->Irp) : NULL;

    if (IrpSp->MajorFunction == IRP_MJ_READ) {
        do_read_job(ji->Irp);
    } else if (IrpSp->MajorFunction == IRP_MJ_WRITE) {
        do_write_job(ji->Vcb, ji->Irp);
    }

    ExFreePool(ji);
}

bool add_thread_job(device_extension* Vcb, PIRP Irp) {
    job_info* ji;

    ji = ExAllocatePoolWithTag(NonPagedPool, sizeof(job_info), ALLOC_TAG);
    if (!ji) {
        ERR("out of memory\n");
        return false;
    }

    ji->Vcb = Vcb;
    ji->Irp = Irp;

    if (!Irp->MdlAddress) {
        PMDL Mdl;
        LOCK_OPERATION op;
        ULONG len;
        PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

        if (IrpSp->MajorFunction == IRP_MJ_READ) {
            op = IoWriteAccess;
            len = IrpSp->Parameters.Read.Length;
        } else if (IrpSp->MajorFunction == IRP_MJ_WRITE) {
            op = IoReadAccess;
            len = IrpSp->Parameters.Write.Length;
        } else {
            ERR("unexpected major function %u\n", IrpSp->MajorFunction);
            ExFreePool(ji);
            return false;
        }

        Mdl = IoAllocateMdl(Irp->UserBuffer, len, false, false, Irp);

        if (!Mdl) {
            ERR("out of memory\n");
            ExFreePool(ji);
            return false;
        }

        _SEH2_TRY {
            MmProbeAndLockPages(Mdl, Irp->RequestorMode, op);
        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            ERR("MmProbeAndLockPages raised status %08lx\n", _SEH2_GetExceptionCode());

            IoFreeMdl(Mdl);
            Irp->MdlAddress = NULL;
            ExFreePool(ji);

            _SEH2_YIELD(return FALSE);
        } _SEH2_END;
    }

    ExInitializeWorkItem(&ji->item, do_job, ji);
    ExQueueWorkItem(&ji->item, DelayedWorkQueue);

    return true;
}

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

static void do_flush(device_extension* Vcb) {
    LIST_ENTRY rollback;
    
    InitializeListHead(&rollback);
    
    FsRtlEnterFileSystem();

    ExAcquireResourceExclusiveLite(&Vcb->tree_lock, TRUE);

    if (Vcb->need_write && !Vcb->readonly)
        do_write(Vcb, NULL, &rollback);
    
    free_trees(Vcb);
    
    clear_rollback(&rollback);

    ExReleaseResourceLite(&Vcb->tree_lock);

    FsRtlExitFileSystem();
}

void STDCALL flush_thread(void* context) {
    DEVICE_OBJECT* devobj = context;
    device_extension* Vcb = devobj->DeviceExtension;
    LARGE_INTEGER due_time;
    
    ObReferenceObject(devobj);
    
    KeInitializeTimer(&Vcb->flush_thread_timer);
    
    due_time.QuadPart = (UINT64)Vcb->options.flush_interval * -10000000;
    
    KeSetTimer(&Vcb->flush_thread_timer, due_time, NULL);
    
    while (TRUE) {
        KeWaitForSingleObject(&Vcb->flush_thread_timer, Executive, KernelMode, FALSE, NULL);

        if (!(devobj->Vpb->Flags & VPB_MOUNTED) || Vcb->removing)
            break;
            
        do_flush(Vcb);
        
        KeSetTimer(&Vcb->flush_thread_timer, due_time, NULL);
    }
    
    ObDereferenceObject(devobj);
    KeCancelTimer(&Vcb->flush_thread_timer);
    
    KeSetEvent(&Vcb->flush_thread_finished, 0, FALSE);
    
    PsTerminateSystemThread(STATUS_SUCCESS);
}

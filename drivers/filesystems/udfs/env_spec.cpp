////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*************************************************************************
*
* File: Env_Spec.cpp
*
* Module: UDF File System Driver (Kernel mode execution only)
*
* Description:
*   Contains environment-secific code to handle physical
*   operations: read, write and device IOCTLS
*
*************************************************************************/

#include "udffs.h"
// define the file specific bug-check id
#define         UDF_BUG_CHECK_ID        UDF_FILE_ENV_SPEC

#define MEASURE_IO_PERFORMANCE

#ifdef MEASURE_IO_PERFORMANCE
LONGLONG IoReadTime=0;
LONGLONG IoWriteTime=0;
LONGLONG WrittenData=0;
LONGLONG IoRelWriteTime=0;
#endif //MEASURE_IO_PERFORMANCE

#ifdef DBG
ULONG UDF_SIMULATE_WRITES=0;
#endif //DBG

/*

 */
NTSTATUS
NTAPI
UDFAsyncCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )
{
    UDFPrint(("UDFAsyncCompletionRoutine ctx=%x\n", Contxt));
    PUDF_PH_CALL_CONTEXT Context = (PUDF_PH_CALL_CONTEXT)Contxt;
    PMDL Mdl, NextMdl;

    Context->IosbToUse = Irp->IoStatus;
#if 1
    // Unlock pages that are described by MDL (if any)...
    Mdl = Irp->MdlAddress;
    while(Mdl) {
        MmPrint(("    Unlock MDL=%x\n", Mdl));
        MmUnlockPages(Mdl);
        Mdl = Mdl->Next;
    }
    // ... and free MDL
    Mdl = Irp->MdlAddress;
    while(Mdl) {
        MmPrint(("    Free MDL=%x\n", Mdl));
        NextMdl = Mdl->Next;
        IoFreeMdl(Mdl);
        Mdl = NextMdl;
    }
    Irp->MdlAddress = NULL;
    IoFreeIrp(Irp);

    KeSetEvent( &(Context->event), 0, FALSE );

    return STATUS_MORE_PROCESSING_REQUIRED;
#else
    KeSetEvent( &(Context->event), 0, FALSE );

    return STATUS_SUCCESS;
#endif
} // end UDFAsyncCompletionRoutine()

NTSTATUS
NTAPI
UDFSyncCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )
{
    UDFPrint(("UDFSyncCompletionRoutine ctx=%x\n", Contxt));
    PUDF_PH_CALL_CONTEXT Context = (PUDF_PH_CALL_CONTEXT)Contxt;

    Context->IosbToUse = Irp->IoStatus;
    //KeSetEvent( &(Context->event), 0, FALSE );

    return STATUS_SUCCESS;
} // end UDFSyncCompletionRoutine()

/*
NTSTATUS
UDFSyncCompletionRoutine2(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )
{
    UDFPrint(("UDFSyncCompletionRoutine2\n"));
    PKEVENT SyncEvent = (PKEVENT)Contxt;

    KeSetEvent( SyncEvent, 0, FALSE );

    return STATUS_SUCCESS;
} // end UDFSyncCompletionRoutine2()
*/

/*

 Function: UDFPhReadSynchronous()

 Description:
    UDFFSD will invoke this rotine to read physical device synchronously/asynchronously

 Expected Interrupt Level (for execution) :

  <= IRQL_DISPATCH_LEVEL

 Return Value: STATUS_SUCCESS/Error

*/
NTSTATUS
NTAPI
UDFPhReadSynchronous(
    PDEVICE_OBJECT      DeviceObject,   // the physical device object
    PVOID               Buffer,
    SIZE_T              Length,
    LONGLONG            Offset,
    PSIZE_T             ReadBytes,
    ULONG               Flags
    )
{
    NTSTATUS            RC = STATUS_SUCCESS;
    LARGE_INTEGER       ROffset;
    PUDF_PH_CALL_CONTEXT Context;
    PIRP                irp;
    KIRQL               CurIrql = KeGetCurrentIrql();
    PVOID               IoBuf = NULL;
//    ULONG i;
#ifdef MEASURE_IO_PERFORMANCE
    LONGLONG IoEnterTime;
    LONGLONG IoExitTime;
    ULONG dt;
    ULONG dtm;
#endif //MEASURE_IO_PERFORMANCE
#ifdef _BROWSE_UDF_
    PVCB Vcb = NULL;
    if(Flags & PH_VCB_IN_RETLEN) {
        Vcb = (PVCB)(*ReadBytes);
    }
#endif //_BROWSE_UDF_

#ifdef MEASURE_IO_PERFORMANCE
    KeQuerySystemTime((PLARGE_INTEGER)&IoEnterTime);
#endif //MEASURE_IO_PERFORMANCE

    UDFPrint(("UDFPhRead: Length: %x Lba: %lx\n",Length>>0xb,Offset>>0xb));
//    UDFPrint(("UDFPhRead: Length: %x Lba: %lx\n",Length>>0x9,Offset>>0x9));

    ROffset.QuadPart = Offset;
    (*ReadBytes) = 0;
/*
    // DEBUG !!!
    Flags |= PH_TMP_BUFFER;
*/
    if(Flags & PH_TMP_BUFFER) {
        IoBuf = Buffer;
    } else {
        IoBuf = DbgAllocatePoolWithTag(NonPagedPool, Length, 'bNWD');
    }
    if (!IoBuf) {
        UDFPrint(("    !IoBuf\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    Context = (PUDF_PH_CALL_CONTEXT)MyAllocatePool__( NonPagedPool, sizeof(UDF_PH_CALL_CONTEXT) );
    if (!Context) {
        UDFPrint(("    !Context\n"));
        try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
    }
    // Create notification event object to be used to signal the request completion.
    KeInitializeEvent(&(Context->event), NotificationEvent, FALSE);

    if (CurIrql > PASSIVE_LEVEL) {
        irp = IoBuildAsynchronousFsdRequest(IRP_MJ_READ, DeviceObject, IoBuf,
                                               Length, &ROffset, &(Context->IosbToUse) );
        if (!irp) {
            UDFPrint(("    !irp Async\n"));
            try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
        }
        MmPrint(("    Alloc async Irp MDL=%x, ctx=%x\n", irp->MdlAddress, Context));
        IoSetCompletionRoutine( irp, &UDFAsyncCompletionRoutine,
                                Context, TRUE, TRUE, TRUE );
    } else {
        irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ, DeviceObject, IoBuf,
                                               Length, &ROffset, &(Context->event), &(Context->IosbToUse) );
        if (!irp) {
            UDFPrint(("    !irp Sync\n"));
            try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
        }
        MmPrint(("    Alloc Irp MDL=%x, ctx=%x\n", irp->MdlAddress, Context));
    }

    (IoGetNextIrpStackLocation(irp))->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
    RC = IoCallDriver(DeviceObject, irp);

    if (RC == STATUS_PENDING) {
        DbgWaitForSingleObject(&(Context->event), NULL);
        if ((RC = Context->IosbToUse.Status) == STATUS_DATA_OVERRUN) {
            RC = STATUS_SUCCESS;
        }
//        *ReadBytes = Context->IosbToUse.Information;
    } else {
//        *ReadBytes = irp->IoStatus.Information;
    }
    if(NT_SUCCESS(RC)) {
        (*ReadBytes) = Context->IosbToUse.Information;
    }
    if(!(Flags & PH_TMP_BUFFER)) {
        RtlCopyMemory(Buffer, IoBuf, *ReadBytes);
    }

    if(NT_SUCCESS(RC)) {
/*
        for(i=0; i<(*ReadBytes); i+=2048) {
            UDFPrint(("IOCRC %8.8x R %x\n", crc32((PUCHAR)Buffer+i, 2048), (ULONG)((Offset+i)/2048) ));
        }
*/
#ifdef _BROWSE_UDF_
        if(Vcb) {
            RC = UDFVRead(Vcb, IoBuf, Length >> Vcb->BlockSizeBits, (ULONG)(Offset >> Vcb->BlockSizeBits), Flags);
        }
#endif //_BROWSE_UDF_
    }

try_exit: NOTHING;

    if(Context) MyFreePool__(Context);
    if(IoBuf && !(Flags & PH_TMP_BUFFER)) DbgFreePool(IoBuf);

#ifdef MEASURE_IO_PERFORMANCE
    KeQuerySystemTime((PLARGE_INTEGER)&IoExitTime);
    IoReadTime += (IoExitTime-IoEnterTime);
    dt = (ULONG)((IoExitTime-IoEnterTime)/10/1000);
    dtm = (ULONG)(((IoExitTime-IoEnterTime)/10)%1000);
    PerfPrint(("\nUDFPhReadSynchronous() exit: %08X, after %d.%4.4d msec.\n", RC, dt, dtm));
#else
    UDFPrint(("UDFPhReadSynchronous() exit: %08X\n", RC));
#endif //MEASURE_IO_PERFORMANCE

    return(RC);
} // end UDFPhReadSynchronous()


/*

 Function: UDFPhWriteSynchronous()

 Description:
    UDFFSD will invoke this rotine to write physical device synchronously

 Expected Interrupt Level (for execution) :

  <= IRQL_DISPATCH_LEVEL

 Return Value: STATUS_SUCCESS/Error

*/
NTSTATUS
NTAPI
UDFPhWriteSynchronous(
    PDEVICE_OBJECT  DeviceObject,   // the physical device object
    PVOID           Buffer,
    SIZE_T          Length,
    LONGLONG        Offset,
    PSIZE_T         WrittenBytes,
    ULONG           Flags
    )
{
    NTSTATUS            RC = STATUS_SUCCESS;
    LARGE_INTEGER       ROffset;
    PUDF_PH_CALL_CONTEXT Context;
    PIRP                irp;
//    LARGE_INTEGER       timeout;
    KIRQL               CurIrql = KeGetCurrentIrql();
    PVOID               IoBuf = NULL;
//    ULONG i;
#ifdef MEASURE_IO_PERFORMANCE
    LONGLONG IoEnterTime;
    LONGLONG IoExitTime;
    ULONG dt;
    ULONG dtm;
#endif //MEASURE_IO_PERFORMANCE
#ifdef _BROWSE_UDF_
    PVCB Vcb = NULL;
    if(Flags & PH_VCB_IN_RETLEN) {
        Vcb = (PVCB)(*WrittenBytes);
    }
#endif //_BROWSE_UDF_

#ifdef MEASURE_IO_PERFORMANCE
    KeQuerySystemTime((PLARGE_INTEGER)&IoEnterTime);
#endif //MEASURE_IO_PERFORMANCE

#ifdef USE_PERF_PRINT
    ULONG Lba = (ULONG)(Offset>>0xb);
//    ASSERT(!(Lba & (32-1)));
    PerfPrint(("UDFPhWrite: Length: %x Lba: %lx\n",Length>>0xb,Lba));
//    UDFPrint(("UDFPhWrite: Length: %x Lba: %lx\n",Length>>0x9,Offset>>0x9));
#endif //DBG

#ifdef DBG
    if(UDF_SIMULATE_WRITES) {
/* FIXME ReactOS
   If this function is to force a read from the bufffer to simulate any segfaults, then it makes sense.
   Else, this forloop is useless.
        UCHAR a;
        for(ULONG i=0; i<Length; i++) {
            a = ((PUCHAR)Buffer)[i];
        }
*/
        *WrittenBytes = Length;
        return STATUS_SUCCESS;
    }
#endif //DBG

    ROffset.QuadPart = Offset;
    (*WrittenBytes) = 0;

/*    IoBuf = ExAllocatePool(NonPagedPool, Length);
    if (!IoBuf) return STATUS_INSUFFICIENT_RESOURCES;
    RtlCopyMemory(IoBuf, Buffer, Length);*/
    IoBuf = Buffer;

/*    if(Flags & PH_TMP_BUFFER) {
        IoBuf = Buffer;
    } else {
        IoBuf = DbgAllocatePool(NonPagedPool, Length);
        RtlCopyMemory(IoBuf, Buffer, Length);
    }*/

    Context = (PUDF_PH_CALL_CONTEXT)MyAllocatePool__( NonPagedPool, sizeof(UDF_PH_CALL_CONTEXT) );
    if (!Context) try_return (RC = STATUS_INSUFFICIENT_RESOURCES);
    // Create notification event object to be used to signal the request completion.
    KeInitializeEvent(&(Context->event), NotificationEvent, FALSE);

    if (CurIrql > PASSIVE_LEVEL) {
        irp = IoBuildAsynchronousFsdRequest(IRP_MJ_WRITE, DeviceObject, IoBuf,
                                               Length, &ROffset, &(Context->IosbToUse) );
        if (!irp) try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
        MmPrint(("    Alloc async Irp MDL=%x, ctx=%x\n", irp->MdlAddress, Context));
        IoSetCompletionRoutine( irp, &UDFAsyncCompletionRoutine,
                                Context, TRUE, TRUE, TRUE );
    } else {
        irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE, DeviceObject, IoBuf,
                                               Length, &ROffset, &(Context->event), &(Context->IosbToUse) );
        if (!irp) try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
        MmPrint(("    Alloc Irp MDL=%x\n, ctx=%x", irp->MdlAddress, Context));
    }

    (IoGetNextIrpStackLocation(irp))->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
    RC = IoCallDriver(DeviceObject, irp);
/*
    for(i=0; i<Length; i+=2048) {
        UDFPrint(("IOCRC %8.8x W %x\n", crc32((PUCHAR)Buffer+i, 2048), (ULONG)((Offset+i)/2048) ));
    }
*/
#ifdef _BROWSE_UDF_
    if(Vcb) {
        UDFVWrite(Vcb, IoBuf, Length >> Vcb->BlockSizeBits, (ULONG)(Offset >> Vcb->BlockSizeBits), Flags);
    }
#endif //_BROWSE_UDF_

    if (RC == STATUS_PENDING) {
        DbgWaitForSingleObject(&(Context->event), NULL);
        if ((RC = Context->IosbToUse.Status) == STATUS_DATA_OVERRUN) {
            RC = STATUS_SUCCESS;
        }
//        *WrittenBytes = Context->IosbToUse.Information;
    } else {
//        *WrittenBytes = irp->IoStatus.Information;
    }
    if(NT_SUCCESS(RC)) {
        (*WrittenBytes) = Context->IosbToUse.Information;
    }

try_exit: NOTHING;

    if(Context) MyFreePool__(Context);
//    if(IoBuf) ExFreePool(IoBuf);
//    if(IoBuf && !(Flags & PH_TMP_BUFFER)) DbgFreePool(IoBuf);
    if(!NT_SUCCESS(RC)) {
        UDFPrint(("WriteError\n"));
    }

#ifdef MEASURE_IO_PERFORMANCE
    KeQuerySystemTime((PLARGE_INTEGER)&IoExitTime);
    IoWriteTime += (IoExitTime-IoEnterTime);
    if (WrittenData > 1024*1024*8) {
        PerfPrint(("\nUDFPhWriteSynchronous() Relative size=%I64d, time=%I64d.\n", WrittenData, IoRelWriteTime));
        WrittenData = IoRelWriteTime = 0;
    }
    WrittenData += Length;
    IoRelWriteTime += (IoExitTime-IoEnterTime);
    dt = (ULONG)((IoExitTime-IoEnterTime)/10/1000);
    dtm = (ULONG)(((IoExitTime-IoEnterTime)/10)%1000);
    PerfPrint(("\nUDFPhWriteSynchronous() exit: %08X, after %d.%4.4d msec.\n", RC, dt, dtm));
#else
    UDFPrint(("nUDFPhWriteSynchronous() exit: %08X\n", RC));
#endif //MEASURE_IO_PERFORMANCE

    return(RC);
} // end UDFPhWriteSynchronous()

#if 0
NTSTATUS
UDFPhWriteVerifySynchronous(
    PDEVICE_OBJECT  DeviceObject,   // the physical device object
    PVOID           Buffer,
    SIZE_T          Length,
    LONGLONG        Offset,
    PSIZE_T         WrittenBytes,
    ULONG           Flags
    )
{
    NTSTATUS RC;
    //PUCHAR v_buff = NULL;
    //ULONG ReadBytes;

    RC = UDFPhWriteSynchronous(DeviceObject, Buffer, Length, Offset, WrittenBytes, Flags);
/*
    if(!Verify)
        return RC;
    v_buff = (PUCHAR)DbgAllocatePoolWithTag(NonPagedPool, Length, 'bNWD');
    if(!v_buff)
        return RC;
    RC = UDFPhReadSynchronous(DeviceObject, v_buff, Length, Offset, &ReadBytes, Flags);
    if(!NT_SUCCESS(RC)) {
        BrutePoint();
        DbgFreePool(v_buff);
        return RC;
    }
    if(RtlCompareMemory(v_buff, Buffer, ReadBytes) == Length) {
        DbgFreePool(v_buff);
        return RC;
    }
    BrutePoint();
    DbgFreePool(v_buff);
    return STATUS_LOST_WRITEBEHIND_DATA;
*/
    return RC;
} // end UDFPhWriteVerifySynchronous()
#endif //0

NTSTATUS
NTAPI
UDFTSendIOCTL(
    IN ULONG IoControlCode,
    IN PVCB Vcb,
    IN PVOID InputBuffer ,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer ,
    IN ULONG OutputBufferLength,
    IN BOOLEAN OverrideVerify,
    OUT PIO_STATUS_BLOCK Iosb OPTIONAL
    )
{
    NTSTATUS            RC = STATUS_SUCCESS;
    BOOLEAN Acquired;

    Acquired = UDFAcquireResourceExclusiveWithCheck(&(Vcb->IoResource));

    _SEH2_TRY {

        RC = UDFPhSendIOCTL(IoControlCode,
                            Vcb->TargetDeviceObject,
                            InputBuffer ,
                            InputBufferLength,
                            OutputBuffer ,
                            OutputBufferLength,
                            OverrideVerify,
                            Iosb
                            );

    } _SEH2_FINALLY {
        if(Acquired)
            UDFReleaseResource(&(Vcb->IoResource));
    } _SEH2_END;

    return RC;
} // end UDFTSendIOCTL()

/*

 Function: UDFPhSendIOCTL()

 Description:
    UDF FSD will invoke this rotine to send IOCTL's to physical
    device

 Return Value: STATUS_SUCCESS/Error

*/
NTSTATUS
NTAPI
UDFPhSendIOCTL(
    IN ULONG IoControlCode,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID InputBuffer ,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer ,
    IN ULONG OutputBufferLength,
    IN BOOLEAN OverrideVerify,
    OUT PIO_STATUS_BLOCK Iosb OPTIONAL
    )
{
    NTSTATUS            RC = STATUS_SUCCESS;
    PIRP                irp;
    PUDF_PH_CALL_CONTEXT Context;
    LARGE_INTEGER timeout;

    UDFPrint(("UDFPhDevIOCTL: Code %8x  \n",IoControlCode));

    Context = (PUDF_PH_CALL_CONTEXT)MyAllocatePool__( NonPagedPool, sizeof(UDF_PH_CALL_CONTEXT) );
    if (!Context) return STATUS_INSUFFICIENT_RESOURCES;
    //  Check if the user gave us an Iosb.

    // Create notification event object to be used to signal the request completion.
    KeInitializeEvent(&(Context->event), NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IoControlCode, DeviceObject, InputBuffer ,
        InputBufferLength, OutputBuffer, OutputBufferLength,FALSE,&(Context->event),&(Context->IosbToUse));

    if (!irp) try_return (RC = STATUS_INSUFFICIENT_RESOURCES);
    MmPrint(("    Alloc Irp MDL=%x, ctx=%x\n", irp->MdlAddress, Context));
/*
    if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
        UDFPrint(("Setting completion routine\n"));
        IoSetCompletionRoutine( irp, &UDFSyncCompletionRoutine,
                                Context, TRUE, TRUE, TRUE );
    }
*/
    if(OverrideVerify) {
        (IoGetNextIrpStackLocation(irp))->Flags |= SL_OVERRIDE_VERIFY_VOLUME;
    }

    RC = IoCallDriver(DeviceObject, irp);

    if (RC == STATUS_PENDING) {
        ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
        UDFPrint(("Enter wait state on evt %x\n", Context));

        if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
            timeout.QuadPart = -1000;
            UDFPrint(("waiting, TO=%I64d\n", timeout.QuadPart));
            RC = DbgWaitForSingleObject(&(Context->event), &timeout);
            while(RC == STATUS_TIMEOUT) {
                timeout.QuadPart *= 2;
                UDFPrint(("waiting, TO=%I64d\n", timeout.QuadPart));
                RC = DbgWaitForSingleObject(&(Context->event), &timeout);
            }

        } else {
            DbgWaitForSingleObject(&(Context->event), NULL);
        }
        if ((RC = Context->IosbToUse.Status) == STATUS_DATA_OVERRUN) {
            RC = STATUS_SUCCESS;
        }
        UDFPrint(("Exit wait state on evt %x, status %8.8x\n", Context, RC));
/*        if(Iosb) {
            (*Iosb) = Context->IosbToUse;
        }*/
    } else {
        UDFPrint(("No wait completion on evt %x\n", Context));
/*        if(Iosb) {
            (*Iosb) = irp->IoStatus;
        }*/
    }

    if(Iosb) {
        (*Iosb) = Context->IosbToUse;
    }

try_exit: NOTHING;

    if(Context) MyFreePool__(Context);
    return(RC);
} // end UDFPhSendIOCTL()


#ifdef UDF_DBG
VOID
UDFNotifyFullReportChange(
    PVCB V,
    PUDF_FILE_INFO FI,
    ULONG E,
    ULONG A
    )
{
    if((FI)->ParentFile) {
        FsRtlNotifyFullReportChange( (V)->NotifyIRPMutex, &((V)->NextNotifyIRP),
                                     (PSTRING)&((FI)->Fcb->FCBName->ObjectName),
                                     ((FI)->ParentFile->Fcb->FCBName->ObjectName.Length + sizeof(WCHAR)),
                                     NULL,NULL,
                                     E, A,
                                     NULL);
    } else {
        FsRtlNotifyFullReportChange( (V)->NotifyIRPMutex, &((V)->NextNotifyIRP),
                                     (PSTRING)&((FI)->Fcb->FCBName->ObjectName),
                                     0,
                                     NULL,NULL,
                                     E, A,
                                     NULL);
    }
} // end UDFNotifyFullReportChange()

VOID
UDFNotifyVolumeEvent(
    IN PFILE_OBJECT FileObject,
    IN ULONG EventCode
    )
{
/* ReactOS FIXME This is always true, and we return anyway. */
//    if(!FsRtlNotifyVolumeEvent)
        return;
    //FsRtlNotifyVolumeEvent(FileObject, EventCode);
} // end UDFNotifyVolumeEvent()
#endif // UDF_DBG


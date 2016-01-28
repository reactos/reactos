////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*************************************************************************
*
* File: Shutdown.cpp
*
* Module: UDF File System Driver (Kernel mode execution only)
*
* Description:
*   Contains code to handle the "shutdown notification" dispatch entry point.
*
*************************************************************************/

#include            "udffs.h"

// define the file specific bug-check id
#define         UDF_BUG_CHECK_ID                UDF_FILE_SHUTDOWN



/*************************************************************************
*
* Function: UDFShutdown()
*
* Description:
*   All disk-based FSDs can expect to receive this shutdown notification
*   request whenever the system is about to be halted gracefully. If you
*   design and implement a network redirector, you must register explicitly
*   for shutdown notification by invoking the IoRegisterShutdownNotification()
*   routine from your driver entry.
*
*   Note that drivers that register to receive shutdown notification get
*   invoked BEFORE disk-based FSDs are told about the shutdown notification.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: Irrelevant.
*
*************************************************************************/
NTSTATUS
NTAPI
UDFShutdown(
    PDEVICE_OBJECT   DeviceObject,       // the logical volume device object
    PIRP             Irp                 // I/O Request Packet
    )
{
    NTSTATUS         RC = STATUS_SUCCESS;
    PtrUDFIrpContext PtrIrpContext = NULL;
    BOOLEAN          AreWeTopLevel = FALSE;

    KdPrint(("UDFShutDown\n"));
//    BrutePoint();

    FsRtlEnterFileSystem();
    ASSERT(DeviceObject);
    ASSERT(Irp);

    // set the top level context
    AreWeTopLevel = UDFIsIrpTopLevel(Irp);
    //ASSERT(!UDFIsFSDevObj(DeviceObject));

    _SEH2_TRY {

        // get an IRP context structure and issue the request
        PtrIrpContext = UDFAllocateIrpContext(Irp, DeviceObject);
        if(PtrIrpContext) {
            RC = UDFCommonShutdown(PtrIrpContext, Irp);
        } else {
            RC = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Status = RC;
            Irp->IoStatus.Information = 0;
            // complete the IRP
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        }

    } _SEH2_EXCEPT(UDFExceptionFilter(PtrIrpContext, _SEH2_GetExceptionInformation())) {

        RC = UDFExceptionHandler(PtrIrpContext, Irp);

        UDFLogEvent(UDF_ERROR_INTERNAL_ERROR, RC);
    } _SEH2_END;

    if (AreWeTopLevel) {
        IoSetTopLevelIrp(NULL);
    }

    FsRtlExitFileSystem();

    return(RC);
} // end UDFShutdown()


/*************************************************************************
*
* Function: UDFCommonShutdown()
*
* Description:
*   The actual work is performed here. Basically, all we do here is
*   internally invoke a flush on all mounted logical volumes. This, in
*   tuen, will result in all open file streams being flushed to disk.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: Irrelevant
*
*************************************************************************/
NTSTATUS
UDFCommonShutdown(
    PtrUDFIrpContext PtrIrpContext,
    PIRP             Irp
    )
{
    NTSTATUS            RC = STATUS_SUCCESS;
    PIO_STACK_LOCATION  IrpSp = NULL;
    PVCB Vcb;
    PLIST_ENTRY Link;
    PPREVENT_MEDIA_REMOVAL_USER_IN Buf = NULL;
    LARGE_INTEGER delay;

    KdPrint(("UDFCommonShutdown\n"));

    _SEH2_TRY {
        // First, get a pointer to the current I/O stack location
        IrpSp = IoGetCurrentIrpStackLocation(Irp);
        ASSERT(IrpSp);

        Buf = (PPREVENT_MEDIA_REMOVAL_USER_IN)MyAllocatePool__(NonPagedPool, sizeof(PREVENT_MEDIA_REMOVAL_USER_IN));
        if(!Buf)
            try_return(RC = STATUS_INSUFFICIENT_RESOURCES);

        // (a) Block all new "mount volume" requests by acquiring an appropriate
        //       global resource/lock.
        // (b) Go through your linked list of mounted logical volumes and for
        //       each such volume, do the following:
        //       (i) acquire the volume resource exclusively
        //       (ii) invoke UDFFlushLogicalVolume() (internally) to flush the
        //              open data streams belonging to the volume from the system
        //              cache
        //       (iii) Invoke the physical/virtual/logical target device object
        //              on which the volume is mounted and inform this device
        //              about the shutdown request (Use IoBuildSynchronouFsdRequest()
        //              to create an IRP with MajorFunction = IRP_MJ_SHUTDOWN that you
        //              will then issue to the target device object).
        //       (iv) Wait for the completion of the shutdown processing by the target
        //              device object
        //       (v) Release the VCB resource we will have acquired in (i) above.

        // Acquire GlobalDataResource
        UDFAcquireResourceExclusive(&(UDFGlobalData.GlobalDataResource), TRUE);
        // Walk through all of the Vcb's attached to the global data.
        Link = UDFGlobalData.VCBQueue.Flink;

        while (Link != &(UDFGlobalData.VCBQueue)) {
            // Get 'next' Vcb
            Vcb = CONTAINING_RECORD( Link, VCB, NextVCB );
            // Move to the next link now since the current Vcb may be deleted.
            Link = Link->Flink;
            ASSERT(Link != Link->Flink);

            if(!(Vcb->VCBFlags & UDF_VCB_FLAGS_SHUTDOWN)) {

#ifdef UDF_DELAYED_CLOSE
                UDFAcquireResourceExclusive(&(Vcb->VCBResource), TRUE);
                KdPrint(("    UDFCommonShutdown:     set UDF_VCB_FLAGS_NO_DELAYED_CLOSE\n"));
                Vcb->VCBFlags |= UDF_VCB_FLAGS_NO_DELAYED_CLOSE;
                UDFReleaseResource(&(Vcb->VCBResource));
#endif //UDF_DELAYED_CLOSE

                // Note: UDFCloseAllDelayed() doesn't acquire DelayedCloseResource if
                // GlobalDataResource is already acquired. Thus for now we should
                // release GlobalDataResource and re-acquire it later.
                UDFReleaseResource( &(UDFGlobalData.GlobalDataResource) );
                if(Vcb->RootDirFCB && Vcb->RootDirFCB->FileInfo) {
                    KdPrint(("    UDFCommonShutdown:     UDFCloseAllSystemDelayedInDir\n"));
                    RC = UDFCloseAllSystemDelayedInDir(Vcb, Vcb->RootDirFCB->FileInfo);
                    ASSERT(OS_SUCCESS(RC));
                }

#ifdef UDF_DELAYED_CLOSE
                UDFCloseAllDelayed(Vcb);
//                UDFReleaseResource(&(UDFGlobalData.DelayedCloseResource));
#endif //UDF_DELAYED_CLOSE

                // re-acquire GlobalDataResource
                UDFAcquireResourceExclusive(&(UDFGlobalData.GlobalDataResource), TRUE);

                // disable Eject Waiter
                UDFStopEjectWaiter(Vcb);
                // Acquire Vcb resource
                UDFAcquireResourceExclusive(&(Vcb->VCBResource), TRUE);

                ASSERT(!Vcb->OverflowQueueCount);

                if(!(Vcb->VCBFlags & UDF_VCB_FLAGS_SHUTDOWN)) {

                    UDFDoDismountSequence(Vcb, Buf, FALSE);
                    if(Vcb->VCBFlags & UDF_VCB_FLAGS_REMOVABLE_MEDIA) {
                        // let drive flush all data before reset
                        delay.QuadPart = -10000000; // 1 sec
                        KeDelayExecutionThread(KernelMode, FALSE, &delay);
                    }
                    Vcb->VCBFlags |= (UDF_VCB_FLAGS_SHUTDOWN |
                                      UDF_VCB_FLAGS_VOLUME_READ_ONLY);
                }

                UDFReleaseResource(&(Vcb->VCBResource));
            }
        }
        // Once we have processed all the mounted logical volumes, we can release
        // all acquired global resources and leave (in peace :-)
        UDFReleaseResource( &(UDFGlobalData.GlobalDataResource) );
        RC = STATUS_SUCCESS;

try_exit: NOTHING;

    } _SEH2_FINALLY {

        if(Buf) MyFreePool__(Buf);
        if(!_SEH2_AbnormalTermination()) {
            Irp->IoStatus.Status = RC;
            Irp->IoStatus.Information = 0;
            // Free up the Irp Context
            UDFReleaseIrpContext(PtrIrpContext);
                // complete the IRP
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        }

    } _SEH2_END; // end of "__finally" processing

    return(RC);
} // end UDFCommonShutdown()

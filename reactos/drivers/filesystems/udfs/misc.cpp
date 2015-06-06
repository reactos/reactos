////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////
/*

 File: Misc.cpp

 Module: UDF File System Driver (Kernel mode execution only)

 Description:
   This file contains some miscellaneous support routines.

*/

#include            "udffs.h"
// define the file specific bug-check id
#define         UDF_BUG_CHECK_ID                UDF_FILE_MISC

#include            <stdio.h>

//CCHAR   DefLetter[] = {""};

/*

 Function: UDFInitializeZones()

 Description:
   Allocates some memory for global zones used to allocate FSD structures.
   Either all memory will be allocated or we will back out gracefully.

 Expected Interrupt Level (for execution) :

  IRQL_PASSIVE_LEVEL

 Return Value: STATUS_SUCCESS/Error

*/
NTSTATUS
UDFInitializeZones(VOID)
{
    NTSTATUS            RC = STATUS_SUCCESS;
    uint32              SizeOfZone = UDFGlobalData.DefaultZoneSizeInNumStructs;
    uint32              SizeOfObjectNameZone = 0;
    uint32              SizeOfCCBZone = 0;
//    uint32              SizeOfFCBZone = 0;
    uint32              SizeOfIrpContextZone = 0;
//    uint32              SizeOfFileInfoZone = 0;

    _SEH2_TRY {

        // initialize the spinlock protecting the zones
        KeInitializeSpinLock(&(UDFGlobalData.ZoneAllocationSpinLock));

        // determine memory requirements
        switch (MmQuerySystemSize()) {
#ifndef DEMO
        case MmMediumSystem:
            SizeOfObjectNameZone = (4 * SizeOfZone * UDFQuadAlign(sizeof(UDFObjectName))) + sizeof(ZONE_SEGMENT_HEADER);
            SizeOfCCBZone = (4 * SizeOfZone * UDFQuadAlign(sizeof(UDFCCB))) + sizeof(ZONE_SEGMENT_HEADER);
            SizeOfIrpContextZone = (4 * SizeOfZone * UDFQuadAlign(sizeof(UDFIrpContext))) + sizeof(ZONE_SEGMENT_HEADER);
            UDFGlobalData.MaxDelayedCloseCount = 24;
            UDFGlobalData.MinDelayedCloseCount = 6;
            UDFGlobalData.MaxDirDelayedCloseCount = 8;
            UDFGlobalData.MinDirDelayedCloseCount = 2;
            UDFGlobalData.WCacheMaxFrames = 8*4;
            UDFGlobalData.WCacheMaxBlocks = 16*64;
            UDFGlobalData.WCacheBlocksPerFrameSh = 8;
            UDFGlobalData.WCacheFramesToKeepFree = 4;
            break;
        case MmLargeSystem:
            SizeOfObjectNameZone = (8 * SizeOfZone * UDFQuadAlign(sizeof(UDFObjectName))) + sizeof(ZONE_SEGMENT_HEADER);
            SizeOfCCBZone = (8 * SizeOfZone * UDFQuadAlign(sizeof(UDFCCB))) + sizeof(ZONE_SEGMENT_HEADER);
            SizeOfIrpContextZone = (8 * SizeOfZone * UDFQuadAlign(sizeof(UDFIrpContext))) + sizeof(ZONE_SEGMENT_HEADER);
            UDFGlobalData.MaxDelayedCloseCount = 72;
            UDFGlobalData.MinDelayedCloseCount = 18;
            UDFGlobalData.MaxDirDelayedCloseCount = 24;
            UDFGlobalData.MinDirDelayedCloseCount = 6;
            UDFGlobalData.WCacheMaxFrames = 2*16*4;
            UDFGlobalData.WCacheMaxBlocks = 2*16*64;
            UDFGlobalData.WCacheBlocksPerFrameSh = 8;
            UDFGlobalData.WCacheFramesToKeepFree = 8;
            break;
#endif //DEMO
        case MmSmallSystem:
        default:
            SizeOfObjectNameZone = (2 * SizeOfZone * UDFQuadAlign(sizeof(UDFObjectName))) + sizeof(ZONE_SEGMENT_HEADER);
            SizeOfCCBZone = (2 * SizeOfZone * UDFQuadAlign(sizeof(UDFCCB))) + sizeof(ZONE_SEGMENT_HEADER);
            SizeOfIrpContextZone = (2 * SizeOfZone * UDFQuadAlign(sizeof(UDFIrpContext))) + sizeof(ZONE_SEGMENT_HEADER);
            UDFGlobalData.MaxDelayedCloseCount = 8;
            UDFGlobalData.MinDelayedCloseCount = 2;
            UDFGlobalData.MaxDirDelayedCloseCount = 6;
            UDFGlobalData.MinDirDelayedCloseCount = 1;
            UDFGlobalData.WCacheMaxFrames = 8*4/2;
            UDFGlobalData.WCacheMaxBlocks = 16*64/2;
            UDFGlobalData.WCacheBlocksPerFrameSh = 8;
            UDFGlobalData.WCacheFramesToKeepFree = 2;
        }

        // typical NT methodology (at least until *someone* exposed the "difference" between a server and workstation ;-)
        if (MmIsThisAnNtAsSystem()) {
            SizeOfObjectNameZone *= UDF_NTAS_MULTIPLE;
            SizeOfCCBZone *= UDF_NTAS_MULTIPLE;
            SizeOfIrpContextZone *= UDF_NTAS_MULTIPLE;
        }

        // allocate memory for each of the zones and initialize the zones ...
        if (!(UDFGlobalData.ObjectNameZone = DbgAllocatePool(NonPagedPool, SizeOfObjectNameZone))) {
            RC = STATUS_INSUFFICIENT_RESOURCES;
            try_return(RC);
        }

        if (!(UDFGlobalData.CCBZone = DbgAllocatePool(NonPagedPool, SizeOfCCBZone))) {
            RC = STATUS_INSUFFICIENT_RESOURCES;
            try_return(RC);
        }

        if (!(UDFGlobalData.IrpContextZone = DbgAllocatePool(NonPagedPool, SizeOfIrpContextZone))) {
            RC = STATUS_INSUFFICIENT_RESOURCES;
            try_return(RC);
        }

        // initialize each of the zone headers ...
        if (!NT_SUCCESS(RC = ExInitializeZone(&(UDFGlobalData.ObjectNameZoneHeader),
                    UDFQuadAlign(sizeof(UDFObjectName)),
                    UDFGlobalData.ObjectNameZone, SizeOfObjectNameZone))) {
            // failed the initialization, leave ...
            try_return(RC);
        }

        if (!NT_SUCCESS(RC = ExInitializeZone(&(UDFGlobalData.CCBZoneHeader),
                    UDFQuadAlign(sizeof(UDFCCB)),
                    UDFGlobalData.CCBZone,
                    SizeOfCCBZone))) {
            // failed the initialization, leave ...
            try_return(RC);
        }

        if (!NT_SUCCESS(RC = ExInitializeZone(&(UDFGlobalData.IrpContextZoneHeader),
                    UDFQuadAlign(sizeof(UDFIrpContext)),
                    UDFGlobalData.IrpContextZone,
                    SizeOfIrpContextZone))) {
            // failed the initialization, leave ...
            try_return(RC);
        }

try_exit:   NOTHING;

    } _SEH2_FINALLY {
        if (!NT_SUCCESS(RC)) {
            // invoke the destroy routine now ...
            UDFDestroyZones();
        } else {
            // mark the fact that we have allocated zones ...
            UDFSetFlag(UDFGlobalData.UDFFlags, UDF_DATA_FLAGS_ZONES_INITIALIZED);
        }
    } _SEH2_END;

    return(RC);
}


/*************************************************************************
*
* Function: UDFDestroyZones()
*
* Description:
*   Free up the previously allocated memory. NEVER do this once the
*   driver has been successfully loaded.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
VOID UDFDestroyZones(VOID)
{
//    BrutePoint();

    _SEH2_TRY {
        // free up each of the pools
        if(UDFGlobalData.ObjectNameZone) {
            DbgFreePool(UDFGlobalData.ObjectNameZone);
            UDFGlobalData.ObjectNameZone = NULL;
        }
        if(UDFGlobalData.CCBZone) {
            DbgFreePool(UDFGlobalData.CCBZone);
            UDFGlobalData.CCBZone = NULL;
        }
        if(UDFGlobalData.IrpContextZone) {
            DbgFreePool(UDFGlobalData.IrpContextZone);
            UDFGlobalData.IrpContextZone = NULL;
        }

//try_exit: NOTHING;

    } _SEH2_FINALLY {
        UDFGlobalData.UDFFlags &= ~UDF_DATA_FLAGS_ZONES_INITIALIZED;
    } _SEH2_END;

    return;
}


/*************************************************************************
*
* Function: UDFIsIrpTopLevel()
*
* Description:
*   Helps the FSD determine who the "top level" caller is for this
*   request. A request can originate directly from a user process
*   (in which case, the "top level" will be NULL when this routine
*   is invoked), OR the user may have originated either from the NT
*   Cache Manager/VMM ("top level" may be set), or this could be a
*   recursion into our code in which we would have set the "top level"
*   field the last time around.
*
* Expected Interrupt Level (for execution) :
*
*  whatever level a particular dispatch routine is invoked at.
*
* Return Value: TRUE/FALSE (TRUE if top level was NULL when routine invoked)
*
*************************************************************************/
BOOLEAN
__fastcall
UDFIsIrpTopLevel(
    PIRP            Irp)            // the IRP sent to our dispatch routine
{
    if(!IoGetTopLevelIrp()) {
        // OK, so we can set ourselves to become the "top level" component
        IoSetTopLevelIrp(Irp);
        return TRUE;
    }
    return FALSE;
}


/*************************************************************************
*
* Function: UDFExceptionFilter()
*
* Description:
*   This routines allows the driver to determine whether the exception
*   is an "allowed" exception i.e. one we should not-so-quietly consume
*   ourselves, or one which should be propagated onwards in which case
*   we will most likely bring down the machine.
*
*   This routine employs the services of FsRtlIsNtstatusExpected(). This
*   routine returns a BOOLEAN result. A RC of FALSE will cause us to return
*   EXCEPTION_CONTINUE_SEARCH which will probably cause a panic.
*   The FsRtl.. routine returns FALSE iff exception values are (currently) :
*       STATUS_DATATYPE_MISALIGNMENT    ||  STATUS_ACCESS_VIOLATION ||
*       STATUS_ILLEGAL_INSTRUCTION  ||  STATUS_INSTRUCTION_MISALIGNMENT
*
* Expected Interrupt Level (for execution) :
*
*  ?
*
* Return Value: EXCEPTION_EXECUTE_HANDLER/EXECEPTION_CONTINUE_SEARCH
*
*************************************************************************/
long
UDFExceptionFilter(
    PtrUDFIrpContext    PtrIrpContext,
    PEXCEPTION_POINTERS PtrExceptionPointers
    )
{
    long                            ReturnCode = EXCEPTION_EXECUTE_HANDLER;
    NTSTATUS                        ExceptionCode = STATUS_SUCCESS;
#if defined UDF_DBG || defined PRINT_ALWAYS
    ULONG i;

    KdPrint(("UDFExceptionFilter\n"));
    KdPrint(("    Ex. Code: %x\n",PtrExceptionPointers->ExceptionRecord->ExceptionCode));
    KdPrint(("    Ex. Addr: %x\n",PtrExceptionPointers->ExceptionRecord->ExceptionAddress));
    KdPrint(("    Ex. Flag: %x\n",PtrExceptionPointers->ExceptionRecord->ExceptionFlags));
    KdPrint(("    Ex. Pnum: %x\n",PtrExceptionPointers->ExceptionRecord->NumberParameters));
    for(i=0;i<PtrExceptionPointers->ExceptionRecord->NumberParameters;i++) {
        KdPrint(("       %x\n",PtrExceptionPointers->ExceptionRecord->ExceptionInformation[i]));
    }
    KdPrint(("Exception context:\n"));
    if(PtrExceptionPointers->ContextRecord->ContextFlags & CONTEXT_INTEGER) {
        KdPrint(("EAX=%8.8x   ",PtrExceptionPointers->ContextRecord->Eax));
        KdPrint(("EBX=%8.8x   ",PtrExceptionPointers->ContextRecord->Ebx));
        KdPrint(("ECX=%8.8x   ",PtrExceptionPointers->ContextRecord->Ecx));
        KdPrint(("EDX=%8.8x\n",PtrExceptionPointers->ContextRecord->Edx));

        KdPrint(("ESI=%8.8x   ",PtrExceptionPointers->ContextRecord->Esi));
        KdPrint(("EDI=%8.8x   ",PtrExceptionPointers->ContextRecord->Edi));
    }
    if(PtrExceptionPointers->ContextRecord->ContextFlags & CONTEXT_CONTROL) {
        KdPrint(("EBP=%8.8x   ",PtrExceptionPointers->ContextRecord->Esp));
        KdPrint(("ESP=%8.8x\n",PtrExceptionPointers->ContextRecord->Ebp));

        KdPrint(("EIP=%8.8x\n",PtrExceptionPointers->ContextRecord->Eip));
    }
//    KdPrint(("Flags: %s %s    ",PtrExceptionPointers->ContextRecord->Eip));

#endif // UDF_DBG

    // figure out the exception code
    ExceptionCode = PtrExceptionPointers->ExceptionRecord->ExceptionCode;

    if ((ExceptionCode == STATUS_IN_PAGE_ERROR) && (PtrExceptionPointers->ExceptionRecord->NumberParameters >= 3)) {
        ExceptionCode = PtrExceptionPointers->ExceptionRecord->ExceptionInformation[2];
    }

    if (PtrIrpContext) {
        PtrIrpContext->SavedExceptionCode = ExceptionCode;
        UDFSetFlag(PtrIrpContext->IrpContextFlags, UDF_IRP_CONTEXT_EXCEPTION);
    }

    // check if we should propagate this exception or not
    if (!(FsRtlIsNtstatusExpected(ExceptionCode))) {

        // better free up the IrpContext now ...
        if (PtrIrpContext) {
            KdPrint(("    UDF Driver internal error\n"));
            BrutePoint();
        } else {
            // we are not ok, propagate this exception.
            //  NOTE: we will bring down the machine ...
            ReturnCode = EXCEPTION_CONTINUE_SEARCH;
        }
    }


    // return the appropriate code
    return(ReturnCode);
} // end UDFExceptionFilter()


/*************************************************************************
*
* Function: UDFExceptionHandler()
*
* Description:
*   One of the routines in the FSD or in the modules we invoked encountered
*   an exception. We have decided that we will "handle" the exception.
*   Therefore we will prevent the machine from a panic ...
*   You can do pretty much anything you choose to in your commercial
*   driver at this point to ensure a graceful exit. In the UDF
*   driver, We shall simply free up the IrpContext (if any), set the
*   error code in the IRP and complete the IRP at this time ...
*
* Expected Interrupt Level (for execution) :
*
*  ?
*
* Return Value: Error code
*
*************************************************************************/
NTSTATUS
UDFExceptionHandler(
    PtrUDFIrpContext PtrIrpContext,
    PIRP             Irp
    )
{
//    NTSTATUS                        RC;
    NTSTATUS            ExceptionCode = STATUS_INSUFFICIENT_RESOURCES;
    PDEVICE_OBJECT      Device;
    PVPB Vpb;
    PETHREAD Thread;

    KdPrint(("UDFExceptionHandler \n"));

//    ASSERT(Irp);

    if (!Irp) {
        KdPrint(("  !Irp, return\n"));
        ASSERT(!PtrIrpContext);
        return ExceptionCode;
    }
    // If it was a queued close (or something like this) then we need not
    // completing it because of MUST_SUCCEED requirement.

    if (PtrIrpContext) {
        ExceptionCode = PtrIrpContext->SavedExceptionCode;
        // Free irp context here
//        UDFReleaseIrpContext(PtrIrpContext);
    } else {
        KdPrint(("  complete Irp and return\n"));
        // must be insufficient resources ...?
        ExceptionCode = STATUS_INSUFFICIENT_RESOURCES;
        Irp->IoStatus.Status = ExceptionCode;
        Irp->IoStatus.Information = 0;
        // complete the IRP
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return ExceptionCode;
    }

    //  Check if we are posting this request.  One of the following must be true
    //  if we are to post a request.
    //
    //      - Status code is STATUS_CANT_WAIT and the request is asynchronous
    //          or we are forcing this to be posted.
    //
    //      - Status code is STATUS_VERIFY_REQUIRED and we are at APC level
    //          or higher.  Can't wait for IO in the verify path in this case.
    //
    //  Set the MORE_PROCESSING flag in the IrpContext to keep if from being
    //  deleted if this is a retryable condition.

    if (ExceptionCode == STATUS_VERIFY_REQUIRED) {
        if (KeGetCurrentIrql() >= APC_LEVEL) {
            KdPrint(("  use UDFPostRequest()\n"));
            ExceptionCode = UDFPostRequest( PtrIrpContext, Irp );
        }
    }

    //  If we posted the request or our caller will retry then just return here.
    if ((ExceptionCode == STATUS_PENDING) ||
        (ExceptionCode == STATUS_CANT_WAIT)) {

        KdPrint(("  STATUS_PENDING/STATUS_CANT_WAIT, return\n"));
        return ExceptionCode;
    }

    //  Store this error into the Irp for posting back to the Io system.
    Irp->IoStatus.Status = ExceptionCode;
    if (IoIsErrorUserInduced( ExceptionCode )) {

        //  Check for the various error conditions that can be caused by,
        //  and possibly resolved my the user.
        if (ExceptionCode == STATUS_VERIFY_REQUIRED) {

            //  Now we are at the top level file system entry point.
            //
            //  If we have already posted this request then the device to
            //  verify is in the original thread.  Find this via the Irp.
            Device = IoGetDeviceToVerify( Irp->Tail.Overlay.Thread );
            IoSetDeviceToVerify( Irp->Tail.Overlay.Thread, NULL );

            //  If there is no device in that location then check in the
            //  current thread.
            if (Device == NULL) {

                Device = IoGetDeviceToVerify( PsGetCurrentThread() );
                IoSetDeviceToVerify( PsGetCurrentThread(), NULL );

                ASSERT( Device != NULL );

                //  Let's not BugCheck just because the driver screwed up.
                if (Device == NULL) {

                    KdPrint(("  Device == NULL, return\n"));
                    ExceptionCode = STATUS_DRIVER_INTERNAL_ERROR;
                    Irp->IoStatus.Status = ExceptionCode;
                    Irp->IoStatus.Information = 0;
                    // complete the IRP
                    IoCompleteRequest(Irp, IO_NO_INCREMENT);

                    UDFReleaseIrpContext(PtrIrpContext);

                    return ExceptionCode;
                }
            }

            KdPrint(("  use UDFPerformVerify()\n"));
            //  UDFPerformVerify() will do the right thing with the Irp.
            //  If we return STATUS_CANT_WAIT then the current thread
            //  can retry the request.
            return UDFPerformVerify( PtrIrpContext, Irp, Device );
        }

        //
        //  The other user induced conditions generate an error unless
        //  they have been disabled for this request.
        //

        if (FlagOn( PtrIrpContext->IrpContextFlags, UDF_IRP_CONTEXT_FLAG_DISABLE_POPUPS )) {
  
            KdPrint(("  DISABLE_POPUPS, complete Irp and return\n"));
            Irp->IoStatus.Status = ExceptionCode;
            Irp->IoStatus.Information = 0;
            // complete the IRP
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            UDFReleaseIrpContext(PtrIrpContext);
            return ExceptionCode;
        } else {

            //  Generate a pop-up
            if (IoGetCurrentIrpStackLocation( Irp )->FileObject != NULL) {

                Vpb = IoGetCurrentIrpStackLocation( Irp )->FileObject->Vpb;
            } else {

                Vpb = NULL;
            }
            //  The device to verify is either in my thread local storage
            //  or that of the thread that owns the Irp.
            Thread = Irp->Tail.Overlay.Thread;
            Device = IoGetDeviceToVerify( Thread );

            if (Device == NULL) {

                Thread = PsGetCurrentThread();
                Device = IoGetDeviceToVerify( Thread );
                ASSERT( Device != NULL );

                //  Let's not BugCheck just because the driver screwed up.
                if (Device == NULL) {
                    KdPrint(("  Device == NULL, return(2)\n"));
                    Irp->IoStatus.Status = ExceptionCode;
                    Irp->IoStatus.Information = 0;
                    // complete the IRP
                    IoCompleteRequest(Irp, IO_NO_INCREMENT);

                    UDFReleaseIrpContext(PtrIrpContext);

                    return ExceptionCode;
                }
            }

            //  This routine actually causes the pop-up.  It usually
            //  does this by queuing an APC to the callers thread,
            //  but in some cases it will complete the request immediately,
            //  so it is very important to IoMarkIrpPending() first.
            IoMarkIrpPending( Irp );
            IoRaiseHardError( Irp, Vpb, Device );

            //  We will be handing control back to the caller here, so
            //  reset the saved device object.

            KdPrint(("  use IoSetDeviceToVerify()\n"));
            IoSetDeviceToVerify( Thread, NULL );
            //  The Irp will be completed by Io or resubmitted.  In either
            //  case we must clean up the IrpContext here.

            UDFReleaseIrpContext(PtrIrpContext);
            return STATUS_PENDING;
        }
    }

    // If it was a normal request from IOManager then complete it
    if (Irp) {
        KdPrint(("  complete Irp\n"));
        // set the error code in the IRP
        Irp->IoStatus.Status = ExceptionCode;
        Irp->IoStatus.Information = 0;
    
        // complete the IRP
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        UDFReleaseIrpContext(PtrIrpContext);
    }

    KdPrint(("  return from exception handler with code %x\n", ExceptionCode));
    return(ExceptionCode);
} // end UDFExceptionHandler()

/*************************************************************************
*
* Function: UDFLogEvent()
*
* Description:
*   Log a message in the NT Event Log. This is a rather simplistic log
*   methodology since we can potentially utilize the event log to
*   provide a lot of information to the user (and you should too!)
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
VOID
UDFLogEvent(
    NTSTATUS UDFEventLogId,      // the UDF private message id
    NTSTATUS RC)                 // any NT error code we wish to log ...
{
    _SEH2_TRY {

        // Implement a call to IoAllocateErrorLogEntry() followed by a call
        // to IoWriteErrorLogEntry(). You should note that the call to IoWriteErrorLogEntry()
        // will free memory for the entry once the write completes (which in actuality
        // is an asynchronous operation).

    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        // nothing really we can do here, just do not wish to crash ...
        NOTHING;
    } _SEH2_END;

    return;
} // end UDFLogEvent()


/*************************************************************************
*
* Function: UDFAllocateObjectName()
*
* Description:
*   Allocate a new ObjectName structure to represent an open on-disk object.
*   Also initialize the ObjectName structure to NULL.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: A pointer to the ObjectName structure OR NULL.
*
*************************************************************************/
PtrUDFObjectName
UDFAllocateObjectName(VOID)
{
    PtrUDFObjectName            PtrObjectName = NULL;
    BOOLEAN                     AllocatedFromZone = TRUE;
    KIRQL                       CurrentIrql;

    // first, __try to allocate out of the zone
    KeAcquireSpinLock(&(UDFGlobalData.ZoneAllocationSpinLock), &CurrentIrql);
    if (!ExIsFullZone(&(UDFGlobalData.ObjectNameZoneHeader))) {
        // we have enough memory
        PtrObjectName = (PtrUDFObjectName)ExAllocateFromZone(&(UDFGlobalData.ObjectNameZoneHeader));

        // release the spinlock
        KeReleaseSpinLock(&(UDFGlobalData.ZoneAllocationSpinLock), CurrentIrql);
    } else {
        // release the spinlock
        KeReleaseSpinLock(&(UDFGlobalData.ZoneAllocationSpinLock), CurrentIrql);

        // if we failed to obtain from the zone, get it directly from the VMM
        PtrObjectName = (PtrUDFObjectName)MyAllocatePool__(NonPagedPool, UDFQuadAlign(sizeof(UDFObjectName)));
        AllocatedFromZone = FALSE;
    }

    if (!PtrObjectName) {
        return NULL;
    }

    // zero out the allocated memory block
    RtlZeroMemory(PtrObjectName, UDFQuadAlign(sizeof(UDFObjectName)));

    // set up some fields ...
    PtrObjectName->NodeIdentifier.NodeType  = UDF_NODE_TYPE_OBJECT_NAME;
    PtrObjectName->NodeIdentifier.NodeSize  = UDFQuadAlign(sizeof(UDFObjectName));


    if (!AllocatedFromZone) {
        UDFSetFlag(PtrObjectName->ObjectNameFlags, UDF_OBJ_NAME_NOT_FROM_ZONE);
    }

    return(PtrObjectName);
} // end UDFAllocateObjectName()


/*************************************************************************
*
* Function: UDFReleaseObjectName()
*
* Description:
*   Deallocate a previously allocated structure.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
VOID
__fastcall 
UDFReleaseObjectName(
    PtrUDFObjectName PtrObjectName)
{
    KIRQL            CurrentIrql;

    ASSERT(PtrObjectName);

    // give back memory either to the zone or to the VMM
    if (!(PtrObjectName->ObjectNameFlags & UDF_OBJ_NAME_NOT_FROM_ZONE)) {
        // back to the zone
        KeAcquireSpinLock(&(UDFGlobalData.ZoneAllocationSpinLock), &CurrentIrql);
        ExFreeToZone(&(UDFGlobalData.ObjectNameZoneHeader), PtrObjectName);
        KeReleaseSpinLock(&(UDFGlobalData.ZoneAllocationSpinLock), CurrentIrql);
    } else {
        MyFreePool__(PtrObjectName);
    }

    return;
} // end UDFReleaseObjectName()


/*************************************************************************
*
* Function: UDFAllocateCCB()
*
* Description:
*   Allocate a new CCB structure to represent an open on-disk object.
*   Also initialize the CCB structure to NULL.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: A pointer to the CCB structure OR NULL.
*
*************************************************************************/
PtrUDFCCB
UDFAllocateCCB(VOID)
{
    PtrUDFCCB                   Ccb = NULL;
    BOOLEAN                     AllocatedFromZone = TRUE;
    KIRQL                       CurrentIrql;

    // first, __try to allocate out of the zone
    KeAcquireSpinLock(&(UDFGlobalData.ZoneAllocationSpinLock), &CurrentIrql);
    if (!ExIsFullZone(&(UDFGlobalData.CCBZoneHeader))) {
        // we have enough memory
        Ccb = (PtrUDFCCB)ExAllocateFromZone(&(UDFGlobalData.CCBZoneHeader));

        // release the spinlock
        KeReleaseSpinLock(&(UDFGlobalData.ZoneAllocationSpinLock), CurrentIrql);
    } else {
        // release the spinlock
        KeReleaseSpinLock(&(UDFGlobalData.ZoneAllocationSpinLock), CurrentIrql);

        // if we failed to obtain from the zone, get it directly from the VMM
        Ccb = (PtrUDFCCB)MyAllocatePool__(NonPagedPool, UDFQuadAlign(sizeof(UDFCCB)));
        AllocatedFromZone = FALSE;
//        KdPrint(("    CCB allocated @%x\n",Ccb));
    }

    if (!Ccb) {
        return NULL;
    }

    // zero out the allocated memory block
    RtlZeroMemory(Ccb, UDFQuadAlign(sizeof(UDFCCB)));

    // set up some fields ...
    Ccb->NodeIdentifier.NodeType = UDF_NODE_TYPE_CCB;
    Ccb->NodeIdentifier.NodeSize = UDFQuadAlign(sizeof(UDFCCB));


    if (!AllocatedFromZone) {
        UDFSetFlag(Ccb->CCBFlags, UDF_CCB_NOT_FROM_ZONE);
    }

    KdPrint(("UDFAllocateCCB: %x\n", Ccb));
    return(Ccb);
} // end UDFAllocateCCB()


/*************************************************************************
*
* Function: UDFReleaseCCB()
*
* Description:
*   Deallocate a previously allocated structure.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
VOID
__fastcall 
UDFReleaseCCB(
    PtrUDFCCB Ccb
    )
{
    KIRQL   CurrentIrql;

    ASSERT(Ccb);

    KdPrint(("UDFReleaseCCB: %x\n", Ccb));
    // give back memory either to the zone or to the VMM
    if(!(Ccb->CCBFlags & UDF_CCB_NOT_FROM_ZONE)) {
        // back to the zone
        KeAcquireSpinLock(&(UDFGlobalData.ZoneAllocationSpinLock), &CurrentIrql);
        ExFreeToZone(&(UDFGlobalData.CCBZoneHeader), Ccb);
        KeReleaseSpinLock(&(UDFGlobalData.ZoneAllocationSpinLock), CurrentIrql);
    } else {
        MyFreePool__(Ccb);
    }

    return;
} // end UDFReleaseCCB()

/*
  Function: UDFCleanupCCB()                      
                                                 
  Description:                                   
    Cleanup and deallocate a previously allocated structure. 
                                                 
  Expected Interrupt Level (for execution) :     
                                                 
   IRQL_PASSIVE_LEVEL                            
                                                 
  Return Value: None                             

*/
VOID
__fastcall 
UDFCleanUpCCB(
    PtrUDFCCB Ccb)
{
//    ASSERT(Ccb);
    if(!Ccb) return; // probably, we havn't allocated it...
    ASSERT(Ccb->NodeIdentifier.NodeType == UDF_NODE_TYPE_CCB);

    _SEH2_TRY {
        if(Ccb->Fcb) {
            UDFTouch(&(Ccb->Fcb->CcbListResource));
            UDFAcquireResourceExclusive(&(Ccb->Fcb->CcbListResource),TRUE);
            RemoveEntryList(&(Ccb->NextCCB));
            UDFReleaseResource(&(Ccb->Fcb->CcbListResource));
        } else {
            BrutePoint();
        }

        if (Ccb->DirectorySearchPattern) {
            if (Ccb->DirectorySearchPattern->Buffer) {
                MyFreePool__(Ccb->DirectorySearchPattern->Buffer);
                Ccb->DirectorySearchPattern->Buffer = NULL;
            }

            MyFreePool__(Ccb->DirectorySearchPattern);
            Ccb->DirectorySearchPattern = NULL;
        }

        UDFReleaseCCB(Ccb);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    } _SEH2_END;
} // end UDFCleanUpCCB()

/*************************************************************************
*
* Function: UDFAllocateFCB()
*
* Description:
*   Allocate a new FCB structure to represent an open on-disk object.
*   Also initialize the FCB structure to NULL.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: A pointer to the FCB structure OR NULL.
*
*************************************************************************/
PtrUDFFCB
UDFAllocateFCB(VOID)
{
    PtrUDFFCB                   Fcb = NULL;

    Fcb = (PtrUDFFCB)MyAllocatePool__(UDF_FCB_MT, UDFQuadAlign(sizeof(UDFFCB)));

    if (!Fcb) {
        return NULL;
    }

    // zero out the allocated memory block
    RtlZeroMemory(Fcb, UDFQuadAlign(sizeof(UDFFCB)));

    // set up some fields ...
    Fcb->NodeIdentifier.NodeType = UDF_NODE_TYPE_FCB;
    Fcb->NodeIdentifier.NodeSize = UDFQuadAlign(sizeof(UDFFCB));

    KdPrint(("UDFAllocateFCB: %x\n", Fcb));
    return(Fcb);
} // end UDFAllocateFCB()


/*************************************************************************
*
* Function: UDFReleaseFCB()
*
* Description:
*   Deallocate a previously allocated structure.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
/*VOID
UDFReleaseFCB(
    PtrUDFFCB Fcb
    )
{
    ASSERT(Fcb);

    MyFreePool__(Fcb);

    return;
}*/

/*************************************************************************
*
*
*************************************************************************/
VOID
__fastcall 
UDFCleanUpFCB(
    PtrUDFFCB Fcb
    )
{
    KdPrint(("UDFCleanUpFCB: %x\n", Fcb));
    if(!Fcb) return;

    ASSERT(Fcb->NodeIdentifier.NodeType == UDF_NODE_TYPE_FCB);

    _SEH2_TRY {
        // Deinitialize FCBName field
        if (Fcb->FCBName) {
            if(Fcb->FCBName->ObjectName.Buffer) {
                MyFreePool__(Fcb->FCBName->ObjectName.Buffer);
                Fcb->FCBName->ObjectName.Buffer = NULL;
#ifdef UDF_DBG
                Fcb->FCBName->ObjectName.Length =
                Fcb->FCBName->ObjectName.MaximumLength = 0;
#endif
            }
#ifdef UDF_DBG
            else {
                KdPrint(("UDF: Fcb has invalid FCBName Buffer\n"));
                BrutePoint();
            }
#endif
            UDFReleaseObjectName(Fcb->FCBName);
            Fcb->FCBName = NULL;
        }
#ifdef UDF_DBG
        else {
            KdPrint(("UDF: Fcb has invalid FCBName field\n"));
            BrutePoint();
        }
#endif


        // begin transaction {
        UDFTouch(&(Fcb->Vcb->FcbListResource));
        UDFAcquireResourceExclusive(&(Fcb->Vcb->FcbListResource), TRUE);
        // Remove this FCB from list of all FCB in VCB
        RemoveEntryList(&(Fcb->NextFCB));
        UDFReleaseResource(&(Fcb->Vcb->FcbListResource));
        // } end transaction

        if(Fcb->FCBFlags & UDF_FCB_INITIALIZED_CCB_LIST_RESOURCE)
            UDFDeleteResource(&(Fcb->CcbListResource));

        // Free memory
        UDFReleaseFCB(Fcb);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    } _SEH2_END;
} // end UDFCleanUpFCB()

#ifdef UDF_DBG
ULONG IrpContextCounter = 0;
#endif //UDF_DBG

/*************************************************************************
*
* Function: UDFAllocateIrpContext()
*
* Description:
*   The UDF FSD creates an IRP context for each request received. This
*   routine simply allocates (and initializes to NULL) a UDFIrpContext
*   structure.
*   Most of the fields in the context structure are then initialized here.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: A pointer to the IrpContext structure OR NULL.
*
*************************************************************************/
PtrUDFIrpContext
UDFAllocateIrpContext(
    PIRP           Irp,
    PDEVICE_OBJECT PtrTargetDeviceObject
    )
{
    PtrUDFIrpContext            PtrIrpContext = NULL;
    BOOLEAN                     AllocatedFromZone = TRUE;
    KIRQL                       CurrentIrql;
    PIO_STACK_LOCATION          IrpSp = NULL;

    // first, __try to allocate out of the zone
    KeAcquireSpinLock(&(UDFGlobalData.ZoneAllocationSpinLock), &CurrentIrql);
    if (!ExIsFullZone(&(UDFGlobalData.IrpContextZoneHeader))) {
        // we have enough memory
        PtrIrpContext = (PtrUDFIrpContext)ExAllocateFromZone(&(UDFGlobalData.IrpContextZoneHeader));
  
        // release the spinlock
        KeReleaseSpinLock(&(UDFGlobalData.ZoneAllocationSpinLock), CurrentIrql);
    } else {
        // release the spinlock
        KeReleaseSpinLock(&(UDFGlobalData.ZoneAllocationSpinLock), CurrentIrql);

        // if we failed to obtain from the zone, get it directly from the VMM
        PtrIrpContext = (PtrUDFIrpContext)MyAllocatePool__(NonPagedPool, UDFQuadAlign(sizeof(UDFIrpContext)));
        AllocatedFromZone = FALSE;
    }

    // if we could not obtain the required memory, bug-check.
    //  Do NOT do this in your commercial driver, instead handle    the error gracefully ...
    if (!PtrIrpContext) {
        return NULL;
    }

#ifdef UDF_DBG
    IrpContextCounter++;
#endif //UDF_DBG

    // zero out the allocated memory block
    RtlZeroMemory(PtrIrpContext, UDFQuadAlign(sizeof(UDFIrpContext)));

    // set up some fields ...
    PtrIrpContext->NodeIdentifier.NodeType  = UDF_NODE_TYPE_IRP_CONTEXT;
    PtrIrpContext->NodeIdentifier.NodeSize  = UDFQuadAlign(sizeof(UDFIrpContext));


    PtrIrpContext->Irp = Irp;
    PtrIrpContext->TargetDeviceObject = PtrTargetDeviceObject;

    // copy over some fields from the IRP and set appropriate flag values
    if (Irp) {
        IrpSp = IoGetCurrentIrpStackLocation(Irp);
        ASSERT(IrpSp);

        PtrIrpContext->MajorFunction = IrpSp->MajorFunction;
        PtrIrpContext->MinorFunction = IrpSp->MinorFunction;

        // Often, a FSD cannot honor a request for asynchronous processing
        // of certain critical requests. For example, a "close" request on
        // a file object can typically never be deferred. Therefore, do not
        // be surprised if sometimes our FSD (just like all other FSD
        // implementations on the Windows NT system) has to override the flag
        // below.
        if (IrpSp->FileObject == NULL) {
            PtrIrpContext->IrpContextFlags |= UDF_IRP_CONTEXT_CAN_BLOCK;
        } else {
            if (IoIsOperationSynchronous(Irp)) {
                PtrIrpContext->IrpContextFlags |= UDF_IRP_CONTEXT_CAN_BLOCK;
            }
        }
    }

    if (!AllocatedFromZone) {
        UDFSetFlag(PtrIrpContext->IrpContextFlags, UDF_IRP_CONTEXT_NOT_FROM_ZONE);
    }

    // Are we top-level ? This information is used by the dispatching code
    // later (and also by the FSD dispatch routine)
    if (IoGetTopLevelIrp() != Irp) {
        // We are not top-level. Note this fact in the context structure
        UDFSetFlag(PtrIrpContext->IrpContextFlags, UDF_IRP_CONTEXT_NOT_TOP_LEVEL);
    }

    return(PtrIrpContext);
} // end UDFAllocateIrpContext()


/*************************************************************************
*
* Function: UDFReleaseIrpContext()
*
* Description:
*   Deallocate a previously allocated structure.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
VOID
UDFReleaseIrpContext(
    PtrUDFIrpContext    PtrIrpContext)
{
    if(!PtrIrpContext) return;
//    ASSERT(PtrIrpContext);

#ifdef UDF_DBG
    IrpContextCounter--;
#endif //UDF_DBG

    // give back memory either to the zone or to the VMM
    if (!(PtrIrpContext->IrpContextFlags & UDF_IRP_CONTEXT_NOT_FROM_ZONE)) {
        // back to the zone
        KIRQL                           CurrentIrql;

        KeAcquireSpinLock(&(UDFGlobalData.ZoneAllocationSpinLock), &CurrentIrql);
        ExFreeToZone(&(UDFGlobalData.IrpContextZoneHeader), PtrIrpContext);
        KeReleaseSpinLock(&(UDFGlobalData.ZoneAllocationSpinLock), CurrentIrql);
    } else {
        MyFreePool__(PtrIrpContext);
    }

    return;
} // end UDFReleaseIrpContext()


/*************************************************************************
*
* Function: UDFPostRequest()
*
* Description:
*   Queue up a request for deferred processing (in the context of a system
*   worker thread). The caller must have locked the user buffer (if required)
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_PENDING
*
*************************************************************************/
NTSTATUS
UDFPostRequest(
    IN PtrUDFIrpContext PtrIrpContext,
    IN PIRP             Irp
    )
{
    KIRQL SavedIrql;
//    PIO_STACK_LOCATION IrpSp;
    PVCB Vcb;

//    IrpSp = IoGetCurrentIrpStackLocation(Irp);

/*
    if(Vcb->StopOverflowQueue) {
        if(Irp) {
            Irp->IoStatus.Status = STATUS_WRONG_VOLUME;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        }
        UDFReleaseIrpContext(PtrIrpContext);
        return STATUS_WRONG_VOLUME;
    }
*/
    // mark the IRP pending if this is not double post
    if(Irp)
        IoMarkIrpPending(Irp);

    Vcb = (PVCB)(PtrIrpContext->TargetDeviceObject->DeviceExtension);
    KeAcquireSpinLock(&(Vcb->OverflowQueueSpinLock), &SavedIrql);

    if ( Vcb->PostedRequestCount > FSP_PER_DEVICE_THRESHOLD) {

        //  We cannot currently respond to this IRP so we'll just enqueue it
        //  to the overflow queue on the volume.
        //  Note: we just reuse LIST_ITEM field inside WorkQueueItem, this
        //  doesn't matter to regular processing of WorkItems.
        InsertTailList( &(Vcb->OverflowQueue),
                        &(PtrIrpContext->WorkQueueItem.List) );
        Vcb->OverflowQueueCount++;
        KeReleaseSpinLock( &(Vcb->OverflowQueueSpinLock), SavedIrql );

    } else {

        //  We are going to send this Irp to an ex worker thread so up
        //  the count.
        Vcb->PostedRequestCount++;

        KeReleaseSpinLock( &(Vcb->OverflowQueueSpinLock), SavedIrql );

        // queue up the request
        ExInitializeWorkItem(&(PtrIrpContext->WorkQueueItem), (PWORKER_THREAD_ROUTINE)UDFCommonDispatch, PtrIrpContext);

        ExQueueWorkItem(&(PtrIrpContext->WorkQueueItem), CriticalWorkQueue);
    //    ExQueueWorkItem(&(PtrIrpContext->WorkQueueItem), DelayedWorkQueue);

    }

    // return status pending
    return STATUS_PENDING;
} // end UDFPostRequest()


/*************************************************************************
*
* Function: UDFCommonDispatch()
*
* Description:
*   The common dispatch routine invoked in the context of a system worker
*   thread. All we do here is pretty much case off the major function
*   code and invoke the appropriate FSD dispatch routine for further
*   processing.
*
* Expected Interrupt Level (for execution) :
*
*   IRQL PASSIVE_LEVEL
*
* Return Value: None
*
*************************************************************************/
VOID
UDFCommonDispatch(
    IN PVOID Context   // actually is a pointer to IRPContext structure
    )
{
    NTSTATUS         RC = STATUS_SUCCESS;
    PtrUDFIrpContext PtrIrpContext = NULL;
    PIRP             Irp = NULL;
    PVCB             Vcb;
    KIRQL            SavedIrql;
    PLIST_ENTRY      Entry;
    BOOLEAN          SpinLock = FALSE;

    // The context must be a pointer to an IrpContext structure
    PtrIrpContext = (PtrUDFIrpContext)Context;

    // Assert that the Context is legitimate
    if ( !PtrIrpContext ||
         (PtrIrpContext->NodeIdentifier.NodeType != UDF_NODE_TYPE_IRP_CONTEXT) ||
         (PtrIrpContext->NodeIdentifier.NodeSize != UDFQuadAlign(sizeof(UDFIrpContext))) /*||
        !(PtrIrpContext->Irp)*/) {
        KdPrint(("    Invalid Context\n"));
        BrutePoint();
        return;
    }

    Vcb = (PVCB)(PtrIrpContext->TargetDeviceObject->DeviceExtension);
    ASSERT(Vcb);

    KdPrint(("  *** Thr: %x  ThCnt: %x  QCnt: %x  Started!\n", PsGetCurrentThread(), Vcb->PostedRequestCount, Vcb->OverflowQueueCount));

    while(TRUE) {

        KdPrint(("    Next IRP\n"));
        FsRtlEnterFileSystem();

        //  Get a pointer to the IRP structure
        // in some cases we can get Zero pointer to Irp
        Irp = PtrIrpContext->Irp;
        // Now, check if the FSD was top level when the IRP was originally invoked
        // and set the thread context (for the worker thread) appropriately
        if (PtrIrpContext->IrpContextFlags & UDF_IRP_CONTEXT_NOT_TOP_LEVEL) {
            // The FSD is not top level for the original request
            // Set a constant value in TLS to reflect this fact
            IoSetTopLevelIrp((PIRP)FSRTL_FSP_TOP_LEVEL_IRP);
        } else {
            IoSetTopLevelIrp(Irp);
        }

        // Since the FSD routine will now be invoked in the context of this worker
        // thread, we should inform the FSD that it is perfectly OK to block in
        // the context of this thread
        PtrIrpContext->IrpContextFlags |= UDF_IRP_CONTEXT_CAN_BLOCK;

        _SEH2_TRY {

            // Pre-processing has been completed; check the Major Function code value
            // either in the IrpContext (copied from the IRP), or directly from the
            //  IRP itself (we will need a pointer to the stack location to do that),
            //  Then, switch based on the value on the Major Function code
            KdPrint(("  *** MJ: %x, Thr: %x\n", PtrIrpContext->MajorFunction, PsGetCurrentThread()));
            switch (PtrIrpContext->MajorFunction) {
            case IRP_MJ_CREATE:
                // Invoke the common create routine
                RC = UDFCommonCreate(PtrIrpContext, Irp);
                break;
            case IRP_MJ_READ:
                // Invoke the common read routine
                RC = UDFCommonRead(PtrIrpContext, Irp);
                break;
#ifndef UDF_READ_ONLY_BUILD
            case IRP_MJ_WRITE:
                // Invoke the common write routine
                RC = UDFCommonWrite(PtrIrpContext, Irp);
                break;
#endif //UDF_READ_ONLY_BUILD
            case IRP_MJ_CLEANUP:
                // Invoke the common cleanup routine
                RC = UDFCommonCleanup(PtrIrpContext, Irp);
                break;
            case IRP_MJ_CLOSE:
                // Invoke the common close routine
                RC = UDFCommonClose(PtrIrpContext, Irp);
                break;
            case IRP_MJ_DIRECTORY_CONTROL:
                // Invoke the common directory control routine
                RC = UDFCommonDirControl(PtrIrpContext, Irp);
                break;
            case IRP_MJ_QUERY_INFORMATION:
#ifndef UDF_READ_ONLY_BUILD
            case IRP_MJ_SET_INFORMATION:
#endif //UDF_READ_ONLY_BUILD
                // Invoke the common query/set information routine
                RC = UDFCommonFileInfo(PtrIrpContext, Irp);
                break;
            case IRP_MJ_QUERY_VOLUME_INFORMATION:
                // Invoke the common query volume routine
                RC = UDFCommonQueryVolInfo(PtrIrpContext, Irp);
                break;
#ifndef UDF_READ_ONLY_BUILD
#ifndef DEMO // release
            case IRP_MJ_SET_VOLUME_INFORMATION:
                // Invoke the common query volume routine
                RC = UDFCommonSetVolInfo(PtrIrpContext, Irp);
                break;
#endif // DEMO
#endif //UDF_READ_ONLY_BUILD
#ifdef UDF_HANDLE_EAS
/*            case IRP_MJ_QUERY_EA:
                // Invoke the common query EAs routine
                RC = UDFCommonGetExtendedAttr(PtrIrpContext, Irp);
                break;
            case IRP_MJ_SET_EA:
                // Invoke the common set EAs routine
                RC = UDFCommonSetExtendedAttr(PtrIrpContext, Irp);
                break;*/
#endif // UDF_HANDLE_EAS
#ifdef UDF_ENABLE_SECURITY
            case IRP_MJ_QUERY_SECURITY:
                // Invoke the common query Security routine
                RC = UDFCommonGetSecurity(PtrIrpContext, Irp);
                break;
#ifndef UDF_READ_ONLY_BUILD
#ifndef DEMO // release
            case IRP_MJ_SET_SECURITY:
                // Invoke the common set Security routine
                RC = UDFCommonSetSecurity(PtrIrpContext, Irp);
                break;
#endif // DEMO
#endif //UDF_READ_ONLY_BUILD
#endif // UDF_ENABLE_SECURITY
            // Continue with the remaining possible dispatch routines below ...
            default:
                KdPrint(("  unhandled *** MJ: %x, Thr: %x\n", PtrIrpContext->MajorFunction, PsGetCurrentThread()));
                // This is the case where we have an invalid major function
                Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
                Irp->IoStatus.Information = 0;
        
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                // Free up the Irp Context
                UDFReleaseIrpContext(PtrIrpContext);
                break;
            }

            // Note: PtrIrpContext is invalid here
            KdPrint(("  *** Thr: %x  Done!\n", PsGetCurrentThread()));

        } _SEH2_EXCEPT(UDFExceptionFilter(PtrIrpContext, _SEH2_GetExceptionInformation())) {

            RC = UDFExceptionHandler(PtrIrpContext, Irp);

            UDFLogEvent(UDF_ERROR_INTERNAL_ERROR, RC);
        }  _SEH2_END;

        // Enable preemption
        FsRtlExitFileSystem();

        // Ensure that the "top-level" field is cleared
        IoSetTopLevelIrp(NULL);

        //  If there are any entries on this volume's overflow queue, service
        //  them.
        if(!Vcb) {
            BrutePoint();
            break;
        }

        KeAcquireSpinLock(&(Vcb->OverflowQueueSpinLock), &SavedIrql);
        SpinLock = TRUE;
        if(!Vcb->OverflowQueueCount)
            break;

        Vcb->OverflowQueueCount--;
        Entry = RemoveHeadList(&Vcb->OverflowQueue);
        KeReleaseSpinLock(&(Vcb->OverflowQueueSpinLock), SavedIrql);
        SpinLock = FALSE;

        PtrIrpContext = CONTAINING_RECORD( Entry,
                                        UDFIrpContext,
                                        WorkQueueItem.List );
    }

    if(!SpinLock)
        KeAcquireSpinLock(&(Vcb->OverflowQueueSpinLock), &SavedIrql);
    Vcb->PostedRequestCount--;
    KeReleaseSpinLock(&(Vcb->OverflowQueueSpinLock), SavedIrql);

    KdPrint(("  *** Thr: %x  ThCnt: %x  QCnt: %x  Terminated!\n", PsGetCurrentThread(), Vcb->PostedRequestCount, Vcb->OverflowQueueCount));

    return;
} // end UDFCommonDispatch()


/*************************************************************************
*
* Function: UDFInitializeVCB()
*
* Description:
*   Perform the initialization for a VCB structure.
*
* Expected Interrupt Level (for execution) :
*
*   IRQL PASSIVE_LEVEL
*
* Return Value: status
*
*************************************************************************/
NTSTATUS
UDFInitializeVCB(
    IN PDEVICE_OBJECT PtrVolumeDeviceObject,
    IN PDEVICE_OBJECT PtrTargetDeviceObject,
    IN PVPB           PtrVPB
    )
{
    NTSTATUS RC = STATUS_SUCCESS;
    PVCB     Vcb = NULL;
    SHORT    i;

    BOOLEAN VCBResourceInit     = FALSE;
    BOOLEAN BitMapResource1Init = FALSE;
    BOOLEAN FcbListResourceInit = FALSE;
    BOOLEAN FileIdResourceInit  = FALSE;
    BOOLEAN DlocResourceInit    = FALSE;
    BOOLEAN DlocResource2Init   = FALSE;
    BOOLEAN FlushResourceInit   = FALSE;
    BOOLEAN PreallocResourceInit= FALSE;
    BOOLEAN IoResourceInit      = FALSE;

    Vcb = (PVCB)(PtrVolumeDeviceObject->DeviceExtension);

    _SEH2_TRY {
    // Zero it out (typically this has already been done by the I/O
    // Manager but it does not hurt to do it again)!
    RtlZeroMemory(Vcb, sizeof(VCB));

    // Initialize the signature fields
    Vcb->NodeIdentifier.NodeType = UDF_NODE_TYPE_VCB;
    Vcb->NodeIdentifier.NodeSize = sizeof(VCB);

    // Initialize the ERESOURCE object.
    RC = UDFInitializeResourceLite(&(Vcb->VCBResource));
    if(!NT_SUCCESS(RC))
        try_return(RC);
    VCBResourceInit = TRUE;

    RC = UDFInitializeResourceLite(&(Vcb->BitMapResource1));
    if(!NT_SUCCESS(RC))
        try_return(RC);
    BitMapResource1Init = TRUE;

    RC = UDFInitializeResourceLite(&(Vcb->FcbListResource));
    if(!NT_SUCCESS(RC))
        try_return(RC);
    FcbListResourceInit = TRUE;

    RC = UDFInitializeResourceLite(&(Vcb->FileIdResource));
    if(!NT_SUCCESS(RC))
        try_return(RC);
    FileIdResourceInit = TRUE;

    RC = UDFInitializeResourceLite(&(Vcb->DlocResource));
    if(!NT_SUCCESS(RC))
        try_return(RC);
    DlocResourceInit = TRUE;

    RC = UDFInitializeResourceLite(&(Vcb->DlocResource2));
    if(!NT_SUCCESS(RC))
        try_return(RC);
    DlocResource2Init = TRUE;

    RC = UDFInitializeResourceLite(&(Vcb->FlushResource));
    if(!NT_SUCCESS(RC))
        try_return(RC);
    FlushResourceInit = TRUE;

    RC = UDFInitializeResourceLite(&(Vcb->PreallocResource));
    if(!NT_SUCCESS(RC))
        try_return(RC);
    PreallocResourceInit = TRUE;

    RC = UDFInitializeResourceLite(&(Vcb->IoResource));
    if(!NT_SUCCESS(RC))
        try_return(RC);
    IoResourceInit = TRUE;

//    RC = UDFInitializeResourceLite(&(Vcb->DelayedCloseResource));
//    ASSERT(NT_SUCCESS(RC));

    // Allocate buffer for statistics
    Vcb->Statistics = (PFILE_SYSTEM_STATISTICS)MyAllocatePool__(NonPagedPool, sizeof(FILE_SYSTEM_STATISTICS) * KeNumberProcessors );
    if(!Vcb->Statistics)
        try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
    RtlZeroMemory( Vcb->Statistics, sizeof(FILE_SYSTEM_STATISTICS) * KeNumberProcessors );
    for (i=0; i < (KeNumberProcessors); i++) {
        Vcb->Statistics[i].Common.FileSystemType = FILESYSTEM_STATISTICS_TYPE_NTFS;
        Vcb->Statistics[i].Common.Version = 1;
        Vcb->Statistics[i].Common.SizeOfCompleteStructure =
            sizeof(FILE_SYSTEM_STATISTICS);
    }

    // We know the target device object.
    // Note that this is not neccessarily a pointer to the actual
    // physical/virtual device on which the logical volume should
    // be mounted. This is actually a pointer to either the actual
    // (real) device or to any device object that may have been
    // attached to it. Any IRPs that we send down should be sent to this
    // device object. However, the "real" physical/virtual device object
    // on which we perform our mount operation can be determined from the
    // RealDevice field in the VPB sent to us.
    Vcb->TargetDeviceObject = PtrTargetDeviceObject;

    // We also have a pointer to the newly created device object representing
    // this logical volume (remember that this VCB structure is simply an
    // extension of the created device object).
    Vcb->VCBDeviceObject = PtrVolumeDeviceObject;

    // We also have the VPB pointer. This was obtained from the
    // Parameters.MountVolume.Vpb field in the current I/O stack location
    // for the mount IRP.
    Vcb->Vpb = PtrVPB;
    // Target Vcb field in Vcb onto itself. This required for check in
    // open/lock/unlock volume dispatch poits
    Vcb->Vcb=Vcb;

    //  Set the removable media flag based on the real device's
    //  characteristics
    if (PtrVPB->RealDevice->Characteristics & FILE_REMOVABLE_MEDIA) {
        Vcb->VCBFlags |= UDF_VCB_FLAGS_REMOVABLE_MEDIA;
    }

    // Initialize the list anchor (head) for some lists in this VCB.
    InitializeListHead(&(Vcb->NextFCB));
    InitializeListHead(&(Vcb->NextNotifyIRP));
    InitializeListHead(&(Vcb->VolumeOpenListHead));

    //  Initialize the overflow queue for the volume
    Vcb->OverflowQueueCount = 0;
    InitializeListHead(&(Vcb->OverflowQueue));

    Vcb->PostedRequestCount = 0;
    KeInitializeSpinLock(&(Vcb->OverflowQueueSpinLock));

    // Initialize the notify IRP list mutex
    FsRtlNotifyInitializeSync(&(Vcb->NotifyIRPMutex));

    // Intilize NtRequiredFCB for this VCB
    Vcb->NTRequiredFCB = (PtrUDFNTRequiredFCB)MyAllocatePool__(NonPagedPool, UDFQuadAlign(sizeof(UDFNTRequiredFCB)));
    if(!Vcb->NTRequiredFCB)
        try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
    RtlZeroMemory(Vcb->NTRequiredFCB, UDFQuadAlign(sizeof(UDFNTRequiredFCB)));

    // Set the initial file size values appropriately. Note that our FSD may
    // wish to guess at the initial amount of information we would like to
    // read from the disk until we have really determined that this a valid
    // logical volume (on disk) that we wish to mount.
    // Vcb->FileSize = Vcb->AllocationSize = ??

    // We do not want to bother with valid data length callbacks
    // from the Cache Manager for the file stream opened for volume metadata
    // information
    Vcb->NTRequiredFCB->CommonFCBHeader.ValidDataLength.QuadPart = 0x7FFFFFFFFFFFFFFFULL;

    Vcb->VolumeLockPID = -1;

    Vcb->VCBOpenCount = 1;

    Vcb->WCacheMaxBlocks        = UDFGlobalData.WCacheMaxBlocks;
    Vcb->WCacheMaxFrames        = UDFGlobalData.WCacheMaxFrames;
    Vcb->WCacheBlocksPerFrameSh = UDFGlobalData.WCacheBlocksPerFrameSh;
    Vcb->WCacheFramesToKeepFree = UDFGlobalData.WCacheFramesToKeepFree;

    // Create a stream file object for this volume.
    //Vcb->PtrStreamFileObject = IoCreateStreamFileObject(NULL,
    //                                            Vcb->Vpb->RealDevice);
    //ASSERT(Vcb->PtrStreamFileObject);

    // Initialize some important fields in the newly created file object.
    //Vcb->PtrStreamFileObject->FsContext = (PVOID)Vcb;      
    //Vcb->PtrStreamFileObject->FsContext2 = NULL;
    //Vcb->PtrStreamFileObject->SectionObjectPointer = &(Vcb->SectionObject);

    //Vcb->PtrStreamFileObject->Vpb = PtrVPB;

    // Link this chap onto the global linked list of all VCB structures.
    // We consider that GlobalDataResource was acquired in past
    UDFAcquireResourceExclusive(&(UDFGlobalData.GlobalDataResource), TRUE);
    InsertTailList(&(UDFGlobalData.VCBQueue), &(Vcb->NextVCB));

    Vcb->TargetDevName.Buffer = (PWCHAR)MyAllocatePool__(NonPagedPool, sizeof(MOUNTDEV_NAME));
    if(!Vcb->TargetDevName.Buffer)
        try_return(RC = STATUS_INSUFFICIENT_RESOURCES);

    RC = UDFPhSendIOCTL(IOCTL_CDRW_GET_DEVICE_NAME /*IOCTL_MOUNTDEV_QUERY_DEVICE_NAME*/, Vcb->TargetDeviceObject,
                    NULL,0,
                    (PVOID)(Vcb->TargetDevName.Buffer),sizeof(MOUNTDEV_NAME),
                    FALSE, NULL);
    if(!NT_SUCCESS(RC)) {

        if(RC == STATUS_BUFFER_OVERFLOW) {
            if(!MyReallocPool__((PCHAR)(Vcb->TargetDevName.Buffer), sizeof(MOUNTDEV_NAME),
                             (PCHAR*)&(Vcb->TargetDevName.Buffer), Vcb->TargetDevName.Buffer[0]+sizeof(MOUNTDEV_NAME)) ) {
                goto Kill_DevName_buffer;
            }

            RC = UDFPhSendIOCTL(IOCTL_CDRW_GET_DEVICE_NAME /*IOCTL_MOUNTDEV_QUERY_DEVICE_NAME*/, Vcb->TargetDeviceObject,
                            NULL,0,
                            (PVOID)(Vcb->TargetDevName.Buffer), Vcb->TargetDevName.Buffer[0]+sizeof(MOUNTDEV_NAME),
                            FALSE, NULL);
            if(!NT_SUCCESS(RC))
                goto Kill_DevName_buffer;

        } else {
Kill_DevName_buffer:
            if(!MyReallocPool__((PCHAR)Vcb->TargetDevName.Buffer, sizeof(MOUNTDEV_NAME),
                                (PCHAR*)&(Vcb->TargetDevName.Buffer), sizeof(REG_NAMELESS_DEV)))
                try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
            RtlCopyMemory(Vcb->TargetDevName.Buffer, REG_NAMELESS_DEV, sizeof(REG_NAMELESS_DEV));
            Vcb->TargetDevName.Length = sizeof(REG_NAMELESS_DEV)-sizeof(WCHAR);
            Vcb->TargetDevName.MaximumLength = sizeof(REG_NAMELESS_DEV);
            goto read_reg;
        }
    }

    Vcb->TargetDevName.MaximumLength =
    (Vcb->TargetDevName.Length = Vcb->TargetDevName.Buffer[0]) + sizeof(WCHAR);
    RtlMoveMemory((PVOID)(Vcb->TargetDevName.Buffer), (PVOID)(Vcb->TargetDevName.Buffer+1), Vcb->TargetDevName.Buffer[0]);
    Vcb->TargetDevName.Buffer[i = (SHORT)(Vcb->TargetDevName.Length/sizeof(WCHAR))] = 0;

    for(;i>=0;i--) {
        if(Vcb->TargetDevName.Buffer[i] == L'\\') {

            Vcb->TargetDevName.Length -= i*sizeof(WCHAR);
            RtlMoveMemory((PVOID)(Vcb->TargetDevName.Buffer), (PVOID)(Vcb->TargetDevName.Buffer+i), Vcb->TargetDevName.Length);
            Vcb->TargetDevName.Buffer[Vcb->TargetDevName.Length/sizeof(WCHAR)] = 0;
            break;
        }
    }

    KdPrint(("  TargetDevName: %S\n", Vcb->TargetDevName.Buffer));

    // Initialize caching for the stream file object.
    //CcInitializeCacheMap(Vcb->PtrStreamFileObject, (PCC_FILE_SIZES)(&(Vcb->AllocationSize)),
    //                            TRUE,       // We will use pinned access.
    //                            &(UDFGlobalData.CacheMgrCallBacks), Vcb);

read_reg:

    UDFReleaseResource(&(UDFGlobalData.GlobalDataResource));

    // Mark the fact that this VCB structure is initialized.
    Vcb->VCBFlags |= UDF_VCB_FLAGS_VCB_INITIALIZED;

    RC = STATUS_SUCCESS;

try_exit:   NOTHING;

    } _SEH2_FINALLY {
        
        if(!NT_SUCCESS(RC)) {
            if(Vcb->TargetDevName.Buffer)
                MyFreePool__(Vcb->TargetDevName.Buffer);
            if(Vcb->NTRequiredFCB)
                MyFreePool__(Vcb->NTRequiredFCB);
            if(Vcb->Statistics)
                MyFreePool__(Vcb->Statistics);

            if(VCBResourceInit)
                UDFDeleteResource(&(Vcb->VCBResource));
            if(BitMapResource1Init)
                UDFDeleteResource(&(Vcb->BitMapResource1));
            if(FcbListResourceInit)
                UDFDeleteResource(&(Vcb->FcbListResource));
            if(FileIdResourceInit)
                UDFDeleteResource(&(Vcb->FileIdResource));
            if(DlocResourceInit)
                UDFDeleteResource(&(Vcb->DlocResource));
            if(DlocResource2Init)
                UDFDeleteResource(&(Vcb->DlocResource2));
            if(FlushResourceInit)
                UDFDeleteResource(&(Vcb->FlushResource));
            if(PreallocResourceInit)
                UDFDeleteResource(&(Vcb->PreallocResource));
            if(IoResourceInit)
                UDFDeleteResource(&(Vcb->IoResource));
        }
    } _SEH2_END;

    return RC;
} // end UDFInitializeVCB()

UDFFSD_MEDIA_TYPE
UDFGetMediaClass(
    PVCB Vcb
    )
{
    switch(Vcb->FsDeviceType) {
    case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
        if(Vcb->VCBFlags & (UDF_VCB_FLAGS_VOLUME_READ_ONLY |
                            UDF_VCB_FLAGS_MEDIA_READ_ONLY))
            return MediaCdrom;
        if(Vcb->CDR_Mode)
            return MediaCdr;
        if((Vcb->MediaType >= MediaType_UnknownSize_CDR) &&
           (Vcb->MediaType < MediaType_UnknownSize_CDRW)) {
            return MediaCdr;
        }
        if((Vcb->MediaType >= MediaType_UnknownSize_CDRW) &&
           (Vcb->MediaType < MediaType_UnknownSize_Unknown)) {
            return MediaCdrw;
        }
        if(Vcb->MediaClassEx == CdMediaClass_CDR) {
            return MediaCdr;
        }
        if(Vcb->MediaClassEx == CdMediaClass_DVDR ||
           Vcb->MediaClassEx == CdMediaClass_DVDpR ||
           Vcb->MediaClassEx == CdMediaClass_HD_DVDR ||
           Vcb->MediaClassEx == CdMediaClass_BDR) {
            return MediaDvdr;
        }
        if(Vcb->MediaClassEx == CdMediaClass_CDRW) {
            return MediaCdrw;
        }
        if(Vcb->MediaClassEx == CdMediaClass_DVDRW ||
           Vcb->MediaClassEx == CdMediaClass_DVDpRW ||
           Vcb->MediaClassEx == CdMediaClass_DVDRAM ||
           Vcb->MediaClassEx == CdMediaClass_HD_DVDRW ||
           Vcb->MediaClassEx == CdMediaClass_HD_DVDRAM ||
           Vcb->MediaClassEx == CdMediaClass_BDRE) {
            return MediaDvdrw;
        }
        // 
        if(Vcb->MediaClassEx == CdMediaClass_CDROM ||
           Vcb->MediaClassEx == CdMediaClass_DVDROM ||
           Vcb->MediaClassEx == CdMediaClass_HD_DVDROM ||
           Vcb->MediaClassEx == CdMediaClass_BDROM) {
            return MediaCdrom;
        }
        return MediaCdrom;
#ifdef UDF_HDD_SUPPORT
    case FILE_DEVICE_DISK_FILE_SYSTEM:
        if(Vcb->TargetDeviceObject->Characteristics & FILE_FLOPPY_DISKETTE)
            return MediaFloppy;
        if(Vcb->TargetDeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
            return MediaZip;
        return MediaHdd;
#endif //UDF_HDD_SUPPORT
    }
    return MediaUnknown;
} // end UDFGetMediaClass()

typedef ULONG
(*ptrUDFGetParameter)(
    IN PVCB Vcb, 
    IN PWSTR Name,
    IN ULONG DefValue
    );

VOID
UDFUpdateCompatOption(
    PVCB Vcb,
    BOOLEAN Update,
    BOOLEAN UseCfg,
    PWCHAR Name,
    ULONG Flag,
    BOOLEAN Default
    )
{
    ptrUDFGetParameter UDFGetParameter = UseCfg ? UDFGetCfgParameter : UDFGetRegParameter;

    if(UDFGetParameter(Vcb, Name, Update ? ((Vcb->CompatFlags & Flag) ? TRUE : FALSE) : Default)) {
        Vcb->CompatFlags |= Flag;
    } else {
        Vcb->CompatFlags &= ~Flag;
    }
} // end UDFUpdateCompatOption()

VOID
UDFReadRegKeys(
    PVCB Vcb,
    BOOLEAN Update,
    BOOLEAN UseCfg
    )
{
    ULONG mult = 1;
    ptrUDFGetParameter UDFGetParameter = UseCfg ? UDFGetCfgParameter : UDFGetRegParameter;
    
    Vcb->DefaultRegName = UDFMediaClassName[(ULONG)UDFGetMediaClass(Vcb)].ClassName;

    // Should we use Extended FE by default ?
    Vcb->UseExtendedFE = (UCHAR)UDFGetParameter(Vcb, REG_USEEXTENDEDFE_NAME,
        Update ? Vcb->UseExtendedFE : FALSE);
    if(Vcb->UseExtendedFE != TRUE) Vcb->UseExtendedFE = FALSE;
    // What type of AllocDescs should we use
    Vcb->DefaultAllocMode = (USHORT)UDFGetParameter(Vcb, REG_DEFALLOCMODE_NAME,
        Update ? Vcb->DefaultAllocMode : ICB_FLAG_AD_SHORT);
    if(Vcb->DefaultAllocMode > ICB_FLAG_AD_LONG) Vcb->DefaultAllocMode = ICB_FLAG_AD_SHORT;
    // Default UID & GID to be set on newly created files
    Vcb->DefaultUID = UDFGetParameter(Vcb, UDF_DEFAULT_UID_NAME, Update ? Vcb->DefaultUID : -1);
    Vcb->DefaultGID = UDFGetParameter(Vcb, UDF_DEFAULT_GID_NAME, Update ? Vcb->DefaultGID : -1);
    // FE allocation charge for plain Dirs
    Vcb->FECharge = UDFGetParameter(Vcb, UDF_FE_CHARGE_NAME, Update ? Vcb->FECharge : 0);
    if(!Vcb->FECharge)
        Vcb->FECharge = UDF_DEFAULT_FE_CHARGE;
    // FE allocation charge for Stream Dirs (SDir)
    Vcb->FEChargeSDir = UDFGetParameter(Vcb, UDF_FE_CHARGE_SDIR_NAME,
        Update ? Vcb->FEChargeSDir : 0);
    if(!Vcb->FEChargeSDir)
        Vcb->FEChargeSDir = UDF_DEFAULT_FE_CHARGE_SDIR;
    // How many Deleted entries should contain Directory to make us
    // start packing it.
    Vcb->PackDirThreshold = UDFGetParameter(Vcb, UDF_DIR_PACK_THRESHOLD_NAME,
        Update ? Vcb->PackDirThreshold : 0);
    if(Vcb->PackDirThreshold == 0xffffffff)
        Vcb->PackDirThreshold = UDF_DEFAULT_DIR_PACK_THRESHOLD;
    // The binary exponent for the number of Pages to be read-ahead'ed
    // This information would be sent to System Cache Manager
    if(!Update) {
        Vcb->SystemCacheGran = (1 << UDFGetParameter(Vcb, UDF_READAHEAD_GRAN_NAME, 0)) * PAGE_SIZE;
        if(!Vcb->SystemCacheGran)
            Vcb->SystemCacheGran = UDF_DEFAULT_READAHEAD_GRAN;
    }
    // Timeouts for FreeSpaceBitMap & TheWholeDirTree flushes
    Vcb->BM_FlushPriod = UDFGetParameter(Vcb, UDF_BM_FLUSH_PERIOD_NAME,
        Update ? Vcb->BM_FlushPriod : 0);
    if(!Vcb->BM_FlushPriod) {
        Vcb->BM_FlushPriod = UDF_DEFAULT_BM_FLUSH_TIMEOUT;
    } else
    if(Vcb->BM_FlushPriod == -1) {
        Vcb->BM_FlushPriod = 0;
    }
    Vcb->Tree_FlushPriod = UDFGetParameter(Vcb, UDF_TREE_FLUSH_PERIOD_NAME,
        Update ? Vcb->Tree_FlushPriod : 0);
    if(!Vcb->Tree_FlushPriod) {
        Vcb->Tree_FlushPriod = UDF_DEFAULT_TREE_FLUSH_TIMEOUT;
    } else
    if(Vcb->Tree_FlushPriod == -1) {
        Vcb->Tree_FlushPriod = 0;
    }
    Vcb->SkipCountLimit = UDFGetParameter(Vcb, UDF_NO_UPDATE_PERIOD_NAME,
        Update ? Vcb->SkipCountLimit : 0);
    if(!Vcb->SkipCountLimit)
        Vcb->SkipCountLimit = -1;

    Vcb->SkipEjectCountLimit = UDFGetParameter(Vcb, UDF_NO_EJECT_PERIOD_NAME,
        Update ? Vcb->SkipEjectCountLimit : 3);

    if(!Update) {
        // How many threads are allowed to sodomize Disc simultaneously on each CPU
        Vcb->ThreadsPerCpu = UDFGetParameter(Vcb, UDF_FSP_THREAD_PER_CPU_NAME,
            Update ? Vcb->ThreadsPerCpu : 2);
        if(Vcb->ThreadsPerCpu < 2)
            Vcb->ThreadsPerCpu = UDF_DEFAULT_FSP_THREAD_PER_CPU;
    }
    // The mimimum FileSize increment when we'll decide not to allocate
    // on-disk space.
    Vcb->SparseThreshold = UDFGetParameter(Vcb, UDF_SPARSE_THRESHOLD_NAME,
        Update ? Vcb->SparseThreshold : 0);
    if(!Vcb->SparseThreshold)
        Vcb->SparseThreshold = UDF_DEFAULT_SPARSE_THRESHOLD;
    // This option is used to VERIFY all the data written. It decreases performance
    Vcb->VerifyOnWrite = UDFGetParameter(Vcb, UDF_VERIFY_ON_WRITE_NAME,
        Update ? Vcb->VerifyOnWrite : FALSE) ? TRUE : FALSE;

#ifndef UDF_READ_ONLY_BUILD
    // Should we update AttrFileTime on Attr changes
    UDFUpdateCompatOption(Vcb, Update, UseCfg, UDF_UPDATE_TIMES_ATTR, UDF_VCB_IC_UPDATE_ATTR_TIME, FALSE);
    // Should we update ModifyFileTime on Writes changes
    // It also affects ARCHIVE bit setting on write operations
    UDFUpdateCompatOption(Vcb, Update, UseCfg, UDF_UPDATE_TIMES_MOD, UDF_VCB_IC_UPDATE_MODIFY_TIME, FALSE);
    // Should we update AccessFileTime on Exec & so on.
    UDFUpdateCompatOption(Vcb, Update, UseCfg, UDF_UPDATE_TIMES_ACCS, UDF_VCB_IC_UPDATE_ACCESS_TIME, FALSE);
    // Should we update Archive bit
    UDFUpdateCompatOption(Vcb, Update, UseCfg, UDF_UPDATE_ATTR_ARCH, UDF_VCB_IC_UPDATE_ARCH_BIT, FALSE);
    // Should we update Dir's Times & Attrs on Modify
    UDFUpdateCompatOption(Vcb, Update, UseCfg, UDF_UPDATE_DIR_TIMES_ATTR_W, UDF_VCB_IC_UPDATE_DIR_WRITE, FALSE);
    // Should we update Dir's Times & Attrs on Access
    UDFUpdateCompatOption(Vcb, Update, UseCfg, UDF_UPDATE_DIR_TIMES_ATTR_R, UDF_VCB_IC_UPDATE_DIR_READ, FALSE);
    // Should we allow user to write into Read-Only Directory
    UDFUpdateCompatOption(Vcb, Update, UseCfg, UDF_ALLOW_WRITE_IN_RO_DIR, UDF_VCB_IC_WRITE_IN_RO_DIR, TRUE);
    // Should we allow user to change Access Time for unchanged Directory
    UDFUpdateCompatOption(Vcb, Update, UseCfg, UDF_ALLOW_UPDATE_TIMES_ACCS_UCHG_DIR, UDF_VCB_IC_UPDATE_UCHG_DIR_ACCESS_TIME, FALSE);
#endif //UDF_READ_ONLY_BUILD
    // Should we record Allocation Descriptors in W2k-compatible form
    UDFUpdateCompatOption(Vcb, Update, UseCfg, UDF_W2K_COMPAT_ALLOC_DESCS, UDF_VCB_IC_W2K_COMPAT_ALLOC_DESCS, TRUE);
    // Should we read LONG_ADs with invalid PartitionReferenceNumber (generated by Nero Instant Burner)
    UDFUpdateCompatOption(Vcb, Update, UseCfg, UDF_INSTANT_COMPAT_ALLOC_DESCS, UDF_VCB_IC_INSTANT_COMPAT_ALLOC_DESCS, TRUE);
    // Should we make a copy of VolumeLabel in LVD
    // usually only PVD is updated
    UDFUpdateCompatOption(Vcb, Update, UseCfg, UDF_W2K_COMPAT_VLABEL, UDF_VCB_IC_W2K_COMPAT_VLABEL, TRUE);
    // Should we handle or ignore HW_RO flag
    UDFUpdateCompatOption(Vcb, Update, UseCfg, UDF_HANDLE_HW_RO, UDF_VCB_IC_HW_RO, FALSE);
    // Should we handle or ignore SOFT_RO flag
    UDFUpdateCompatOption(Vcb, Update, UseCfg, UDF_HANDLE_SOFT_RO, UDF_VCB_IC_SOFT_RO, TRUE);

    // Check if we should generate UDF-style or OS-style DOS-names
    UDFUpdateCompatOption(Vcb, Update, UseCfg, UDF_OS_NATIVE_DOS_NAME, UDF_VCB_IC_OS_NATIVE_DOS_NAME, FALSE);
#ifndef UDF_READ_ONLY_BUILD
    // should we force FO_WRITE_THROUGH on removable media
    UDFUpdateCompatOption(Vcb, Update, UseCfg, UDF_FORCE_WRITE_THROUGH_NAME, UDF_VCB_IC_FORCE_WRITE_THROUGH, 
                          (Vcb->TargetDeviceObject->Characteristics & FILE_REMOVABLE_MEDIA) ? TRUE : FALSE
                         );
#endif //UDF_READ_ONLY_BUILD
    // Should we ignore FO_SEQUENTIAL_ONLY
    UDFUpdateCompatOption(Vcb, Update, UseCfg, UDF_IGNORE_SEQUENTIAL_IO, UDF_VCB_IC_IGNORE_SEQUENTIAL_IO, FALSE);
// Force Read-only mounts
#ifndef UDF_READ_ONLY_BUILD
    UDFUpdateCompatOption(Vcb, Update, UseCfg, UDF_FORCE_HW_RO, UDF_VCB_IC_FORCE_HW_RO, FALSE);
#else //UDF_READ_ONLY_BUILD
    Vcb->CompatFlags |= UDF_VCB_IC_FORCE_HW_RO;
#endif //UDF_READ_ONLY_BUILD
    // Check if we should send FLUSH request for File/Dir down to
    // underlaying driver
    if(UDFGetParameter(Vcb, UDF_FLUSH_MEDIA,Update ? Vcb->FlushMedia : FALSE)) {
        Vcb->FlushMedia = TRUE;
    } else {
        Vcb->FlushMedia = FALSE;
    }
    // compare data from packet with data to be writen there
    // before physical writing
    if(!UDFGetParameter(Vcb, UDF_COMPARE_BEFORE_WRITE, Update ? Vcb->DoNotCompareBeforeWrite : FALSE)) {
        Vcb->DoNotCompareBeforeWrite = TRUE;
    } else {
        Vcb->DoNotCompareBeforeWrite = FALSE;
    }
    if(!Update)  {
        if(UDFGetParameter(Vcb, UDF_CHAINED_IO, TRUE)) {
            Vcb->CacheChainedIo = TRUE;
        }

        if(UDFGetParameter(Vcb, UDF_FORCE_MOUNT_ALL, FALSE)) {
            Vcb->VCBFlags |= UDF_VCB_FLAGS_RAW_DISK;
        }
        // Should we show Blank.Cd file on damaged/unformatted,
        // but UDF-compatible disks
        Vcb->ShowBlankCd = (UCHAR)UDFGetParameter(Vcb, UDF_SHOW_BLANK_CD, FALSE);
        if(Vcb->ShowBlankCd) {
            Vcb->CompatFlags |= UDF_VCB_IC_SHOW_BLANK_CD;
            if(Vcb->ShowBlankCd > 2) {
                Vcb->ShowBlankCd = 2;
            }
        }
        // Should we wait util CD device return from
        // Becoming Ready state
        if(UDFGetParameter(Vcb, UDF_WAIT_CD_SPINUP, TRUE)) {
            Vcb->CompatFlags |= UDF_VCB_IC_WAIT_CD_SPINUP;
        }
        // Should we remenber bad VDS locations during mount
        // Caching will improve mount performance on bad disks, but
        // will degrade mauntability of unreliable discs
        if(UDFGetParameter(Vcb, UDF_CACHE_BAD_VDS, TRUE)) {
            Vcb->CompatFlags |= UDF_VCB_IC_CACHE_BAD_VDS;
        }

        // Set partitially damaged volume mount mode
        Vcb->PartitialDamagedVolumeAction = (UCHAR)UDFGetParameter(Vcb, UDF_PART_DAMAGED_BEHAVIOR, UDF_PART_DAMAGED_RW);
        if(Vcb->PartitialDamagedVolumeAction > 2) {
            Vcb->PartitialDamagedVolumeAction = UDF_PART_DAMAGED_RW;
        }

        // Set partitially damaged volume mount mode
        Vcb->NoFreeRelocationSpaceVolumeAction = (UCHAR)UDFGetParameter(Vcb, UDF_NO_SPARE_BEHAVIOR, UDF_PART_DAMAGED_RW);
        if(Vcb->NoFreeRelocationSpaceVolumeAction > 1) {
            Vcb->NoFreeRelocationSpaceVolumeAction = UDF_PART_DAMAGED_RW;
        }

        // Set dirty volume mount mode
        if(UDFGetParameter(Vcb, UDF_DIRTY_VOLUME_BEHAVIOR, UDF_PART_DAMAGED_RO)) {
            Vcb->CompatFlags |= UDF_VCB_IC_DIRTY_RO;
        }

        mult = UDFGetParameter(Vcb, UDF_CACHE_SIZE_MULTIPLIER, 1);
        if(!mult) mult = 1;
        Vcb->WCacheMaxBlocks *= mult;
        Vcb->WCacheMaxFrames *= mult;

        if(UDFGetParameter(Vcb, UDF_USE_EJECT_BUTTON, TRUE)) {
            Vcb->UseEvent = TRUE;
        }
    }
    return;
} // end UDFReadRegKeys()

ULONG
UDFGetRegParameter(
    IN PVCB Vcb, 
    IN PWSTR Name,
    IN ULONG DefValue
    )
{
    return UDFRegCheckParameterValue(&(UDFGlobalData.SavedRegPath),
                                     Name,
                                     Vcb ? &(Vcb->TargetDevName) : NULL,
                                     Vcb ? Vcb->DefaultRegName : NULL,
                                     DefValue);
} // end UDFGetRegParameter()

ULONG
UDFGetCfgParameter(
    IN PVCB Vcb, 
    IN PWSTR Name,
    IN ULONG DefValue
    )
{
    ULONG len;
    CHAR NameA[128];
    ULONG ret_val=0;
    CHAR a;
    BOOLEAN wait_name=TRUE;
    BOOLEAN wait_val=FALSE;
    BOOLEAN wait_nl=FALSE;
    ULONG radix=10;
    ULONG i;

    PUCHAR Cfg    = Vcb->Cfg;
    ULONG  Length = Vcb->CfgLength;

    if(!Cfg || !Length)
        return DefValue;

    len = wcslen(Name);
    if(len >= sizeof(NameA))
        return DefValue;
    sprintf(NameA, "%S", Name);

    for(i=0; i<Length; i++) {
        a=Cfg[i];
        switch(a) {
        case '\n':
        case '\r':
        case ',':
            if(wait_val)
                return DefValue;
            continue;
        case ';':
        case '#':
        case '[': // ignore sections for now, treat as comment
            if(!wait_name)
                return DefValue;
            wait_nl = TRUE;
            continue;
        case '=':
            if(!wait_val)
                return DefValue;
            continue;
        case ' ':
        case '\t':
            continue;
        default:
            if(wait_nl)
                continue;
        }
        if(wait_name) {
            if(i+len+2 > Length)
                return DefValue;
            if(RtlCompareMemory(Cfg+i, NameA, len) == len) {
                a=Cfg[i+len];
                switch(a) {
                case '\n':
                case '\r':
                case ',':
                case ';':
                case '#':
                    return DefValue;
                case '=':
                case ' ':
                case '\t':
                    break;
                default:
                    wait_nl = TRUE;
                    wait_val = FALSE;
                    i+=len;
                    continue;
                }
                wait_name = FALSE;
                wait_nl = FALSE;
                wait_val = TRUE;
                i+=len;
                
            } else {
                wait_nl = TRUE;
            }
            continue;
        }
        if(wait_val) {
            if(i+3 > Length) {
                if(a=='0' && Cfg[i+1]=='x') {
                    i+=2;
                    radix=16;
                }
            }
            if(i >= Length) {
                return DefValue;
            }
            while(i<Length) {
                a=Cfg[i];
                switch(a) {
                case '\n':
                case '\r':
                case ' ':
                case '\t':
                case ',':
                case ';':
                case '#':
                    if(wait_val)
                        return DefValue;
                    return ret_val;
                }
                if(a >= '0' && a <= '9') {
                    a -= '0';
                } else {
                    if(radix != 16)
                        return DefValue;
                    if(a >= 'a' && a <= 'f') {
                        a -= 'a';
                    } else
                    if(a >= 'A' && a <= 'F') {
                        a -= 'A';
                    } else {
                        return DefValue;
                    }
                    a += 0x0a;
                }
                ret_val = ret_val*radix + a;
                wait_val = FALSE;
                i++;
            }
            return ret_val;
        }
    }
    return DefValue;

} // end UDFGetCfgParameter()

VOID
UDFReleaseVCB(
    PVCB  Vcb
    )
{
    LARGE_INTEGER delay;
    KdPrint(("UDFReleaseVCB\n"));

    delay.QuadPart = -500000; // 0.05 sec
    while(Vcb->PostedRequestCount) {
        KdPrint(("UDFReleaseVCB: PostedRequestCount = %d\n", Vcb->PostedRequestCount));
        // spin until all queues IRPs are processed
        KeDelayExecutionThread(KernelMode, FALSE, &delay);
        delay.QuadPart -= 500000; // grow delay 0.05 sec
    }

    _SEH2_TRY {
        KdPrint(("UDF: Flushing buffers\n"));
        UDFVRelease(Vcb);
        WCacheFlushAll__(&(Vcb->FastCache),Vcb);
        WCacheRelease__(&(Vcb->FastCache));

    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    } _SEH2_END;

#ifdef UDF_DBG
    _SEH2_TRY {
        if (!ExIsResourceAcquiredShared(&UDFGlobalData.GlobalDataResource)) {
            KdPrint(("UDF: attempt to access to not protected data\n"));
            KdPrint(("UDF: UDFGlobalData\n"));
            BrutePoint();
        }
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    } _SEH2_END;
#endif

    _SEH2_TRY {
        RemoveEntryList(&(Vcb->NextVCB));
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    } _SEH2_END;

/*    _SEH2_TRY {
        if(Vcb->VCBFlags & UDF_VCB_FLAGS_STOP_WAITER_EVENT)
            KeWaitForSingleObject(&(Vcb->WaiterStopped), Executive, KernelMode, FALSE, NULL);
            Vcb->VCBFlags &= ~UDF_VCB_FLAGS_STOP_WAITER_EVENT;
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    }*/

    _SEH2_TRY {
        KdPrint(("UDF: Delete resources\n"));
        UDFDeleteResource(&(Vcb->VCBResource));
        UDFDeleteResource(&(Vcb->BitMapResource1));
        UDFDeleteResource(&(Vcb->FcbListResource));
        UDFDeleteResource(&(Vcb->FileIdResource));
        UDFDeleteResource(&(Vcb->DlocResource));
        UDFDeleteResource(&(Vcb->DlocResource2));
        UDFDeleteResource(&(Vcb->FlushResource));
        UDFDeleteResource(&(Vcb->PreallocResource));
        UDFDeleteResource(&(Vcb->IoResource));
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    } _SEH2_END;

    _SEH2_TRY {
        KdPrint(("UDF: Cleanup VCB\n"));
        ASSERT(IsListEmpty(&(Vcb->NextNotifyIRP)));
        FsRtlNotifyUninitializeSync(&(Vcb->NotifyIRPMutex));
        UDFCleanupVCB(Vcb);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    } _SEH2_END;

    _SEH2_TRY {
        KdPrint(("UDF: Delete DO\n"));
        IoDeleteDevice(Vcb->VCBDeviceObject);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        BrutePoint();
    } _SEH2_END;

} // end UDFReleaseVCB()

/*
    Read DWORD from Registry
*/
ULONG
UDFRegCheckParameterValue(
    IN PUNICODE_STRING RegistryPath,
    IN PWSTR Name,
    IN PUNICODE_STRING PtrVolumePath,
    IN PWSTR DefaultPath,
    IN ULONG DefValue
    )
{
    NTSTATUS          status;

    ULONG             val = DefValue;

    UNICODE_STRING    paramStr;
    UNICODE_STRING    defaultParamStr;
    UNICODE_STRING    paramPathUnknownStr;

    UNICODE_STRING    paramSuffix;
    UNICODE_STRING    paramPath;
    UNICODE_STRING    paramPathUnknown;
    UNICODE_STRING    paramDevPath;
    UNICODE_STRING    defaultParamPath;

    _SEH2_TRY {

        paramPath.Buffer = NULL;
        paramDevPath.Buffer = NULL;
        paramPathUnknown.Buffer = NULL;
        defaultParamPath.Buffer = NULL;

        // First append \Parameters to the passed in registry path
        // Note, RtlInitUnicodeString doesn't allocate memory
        RtlInitUnicodeString(&paramStr, L"\\Parameters");
        RtlInitUnicodeString(&paramPath, NULL);

        RtlInitUnicodeString(&paramPathUnknownStr, REG_DEFAULT_UNKNOWN);
        RtlInitUnicodeString(&paramPathUnknown, NULL);
        
        paramPathUnknown.MaximumLength = RegistryPath->Length + paramPathUnknownStr.Length + paramStr.Length + sizeof(WCHAR);
        paramPath.MaximumLength = RegistryPath->Length + paramStr.Length + sizeof(WCHAR);

        paramPath.Buffer = (PWCH)MyAllocatePool__(PagedPool, paramPath.MaximumLength);
        if(!paramPath.Buffer) {
            KdPrint(("UDFCheckRegValue: couldn't allocate paramPath\n"));
            try_return(val = DefValue);
        }
        paramPathUnknown.Buffer = (PWCH)MyAllocatePool__(PagedPool, paramPathUnknown.MaximumLength);
        if(!paramPathUnknown.Buffer) {
            KdPrint(("UDFCheckRegValue: couldn't allocate paramPathUnknown\n"));
            try_return(val = DefValue);
        }

        RtlZeroMemory(paramPath.Buffer, paramPath.MaximumLength);
        status = RtlAppendUnicodeToString(&paramPath, RegistryPath->Buffer);
        if(!NT_SUCCESS(status)) {
            try_return(val = DefValue);
        }
        status = RtlAppendUnicodeToString(&paramPath, paramStr.Buffer);
        if(!NT_SUCCESS(status)) {
            try_return(val = DefValue);
        }
        KdPrint(("UDFCheckRegValue: (1) |%S|\n", paramPath.Buffer));

        RtlZeroMemory(paramPathUnknown.Buffer, paramPathUnknown.MaximumLength);
        status = RtlAppendUnicodeToString(&paramPathUnknown, RegistryPath->Buffer);
        if(!NT_SUCCESS(status)) {
            try_return(val = DefValue);
        }
        status = RtlAppendUnicodeToString(&paramPathUnknown, paramStr.Buffer);
        if(!NT_SUCCESS(status)) {
            try_return(val = DefValue);
        }
        status = RtlAppendUnicodeToString(&paramPathUnknown, paramPathUnknownStr.Buffer);
        if(!NT_SUCCESS(status)) {
            try_return(val = DefValue);
        }
        KdPrint(("UDFCheckRegValue: (2) |%S|\n", paramPathUnknown.Buffer));

        // First append \Parameters\Default_XXX to the passed in registry path
        if(DefaultPath) {
            RtlInitUnicodeString(&defaultParamStr, DefaultPath);
            RtlInitUnicodeString(&defaultParamPath, NULL);
            defaultParamPath.MaximumLength = paramPath.Length + defaultParamStr.Length + sizeof(WCHAR);
            defaultParamPath.Buffer = (PWCH)MyAllocatePool__(PagedPool, defaultParamPath.MaximumLength);
            if(!defaultParamPath.Buffer) {
                KdPrint(("UDFCheckRegValue: couldn't allocate defaultParamPath\n"));
                try_return(val = DefValue);
            }

            RtlZeroMemory(defaultParamPath.Buffer, defaultParamPath.MaximumLength);
            status = RtlAppendUnicodeToString(&defaultParamPath, paramPath.Buffer);
            if(!NT_SUCCESS(status)) {
                try_return(val = DefValue);
            }
            status = RtlAppendUnicodeToString(&defaultParamPath, defaultParamStr.Buffer);
            if(!NT_SUCCESS(status)) {
                try_return(val = DefValue);
            }
            KdPrint(("UDFCheckRegValue: (3) |%S|\n", defaultParamPath.Buffer));
        }

        if(PtrVolumePath) {
            paramSuffix = *PtrVolumePath;
        } else {
            RtlInitUnicodeString(&paramSuffix, NULL);
        }

        RtlInitUnicodeString(&paramDevPath, NULL);
        // now build the device specific path
        paramDevPath.MaximumLength = paramPath.Length + paramSuffix.Length + sizeof(WCHAR);
        paramDevPath.Buffer = (PWCH)MyAllocatePool__(PagedPool, paramDevPath.MaximumLength);
        if(!paramDevPath.Buffer) {
            try_return(val = DefValue);
        }

        RtlZeroMemory(paramDevPath.Buffer, paramDevPath.MaximumLength);
        status = RtlAppendUnicodeToString(&paramDevPath, paramPath.Buffer);
        if(!NT_SUCCESS(status)) {
            try_return(val = DefValue);
        }
        if(paramSuffix.Buffer) {
            status = RtlAppendUnicodeToString(&paramDevPath, paramSuffix.Buffer);
            if(!NT_SUCCESS(status)) {
                try_return(val = DefValue);
            }
        }

        KdPrint(( " Parameter = %ws\n", Name));

        {
            HKEY hk = NULL;
            status = RegTGetKeyHandle(NULL, RegistryPath->Buffer, &hk);
            if(NT_SUCCESS(status)) {
                RegTCloseKeyHandle(hk);
            }
        }


        // *** Read GLOBAL_DEFAULTS from
        // \DwUdf\Parameters_Unknown\

        status = RegTGetDwordValue(NULL, paramPath.Buffer, Name, &val);

        // *** Read DEV_CLASS_SPEC_DEFAULTS (if any) from
        // \DwUdf\Parameters_%DevClass%\

        if(DefaultPath) {
            status = RegTGetDwordValue(NULL, defaultParamPath.Buffer, Name, &val);
        }

        // *** Read DEV_SPEC_PARAMS from (if device supports GetDevName)
        // \DwUdf\Parameters\%DevName%\

        status = RegTGetDwordValue(NULL, paramDevPath.Buffer, Name, &val);

try_exit:   NOTHING;

    } _SEH2_FINALLY {

        if(DefaultPath && defaultParamPath.Buffer) {
            MyFreePool__(defaultParamPath.Buffer);
        }
        if(paramPath.Buffer) {
            MyFreePool__(paramPath.Buffer);
        }
        if(paramDevPath.Buffer) {
            MyFreePool__(paramDevPath.Buffer);
        }
        if(paramPathUnknown.Buffer) {
            MyFreePool__(paramPathUnknown.Buffer);
        }
    } _SEH2_END;

    KdPrint(( "UDFCheckRegValue: %ws for drive %s is %x\n\n", Name, PtrVolumePath, val));
    return val;
} // end UDFRegCheckParameterValue()

/*
Routine Description:
    This routine is called to initialize an IrpContext for the current
    UDFFS request.  The IrpContext is on the stack and we need to initialize
    it for the current request.  The request is a close operation.

Arguments:

    IrpContext - IrpContext to initialize.

    IrpContextLite - source for initialization

Return Value:

    None

*/
VOID
UDFInitializeIrpContextFromLite(
    OUT PtrUDFIrpContext    *IrpContext,
    IN PtrUDFIrpContextLite IrpContextLite
    )
{
    (*IrpContext) = UDFAllocateIrpContext(NULL, IrpContextLite->RealDevice);
    //  Zero and then initialize the structure.

    //  Major/Minor Function codes
    (*IrpContext)->MajorFunction = IRP_MJ_CLOSE;
    (*IrpContext)->Fcb = IrpContextLite->Fcb;
    (*IrpContext)->TreeLength = IrpContextLite->TreeLength;
    (*IrpContext)->IrpContextFlags |= (IrpContextLite->IrpContextFlags & ~UDF_IRP_CONTEXT_NOT_FROM_ZONE);

    //  Set the wait parameter
    UDFSetFlag( (*IrpContext)->IrpContextFlags, UDF_IRP_CONTEXT_CAN_BLOCK );

    return;
} // end UDFInitializeIrpContextFromLite()

/*
Routine Description:
    This routine is called to initialize an IrpContext for the current
    UDFFS request.  The IrpContext is on the stack and we need to initialize
    it for the current request.  The request is a close operation.

Arguments:

    IrpContext - IrpContext to initialize.

    IrpContextLite - source for initialization

Return Value:

    None

*/
NTSTATUS
UDFInitializeIrpContextLite(
    OUT PtrUDFIrpContextLite *IrpContextLite,
    IN PtrUDFIrpContext    IrpContext,
    IN PtrUDFFCB           Fcb
    )
{
    PtrUDFIrpContextLite LocalIrpContextLite = (PtrUDFIrpContextLite)MyAllocatePool__(NonPagedPool,sizeof(UDFIrpContextLite));
    if(!LocalIrpContextLite)
        return STATUS_INSUFFICIENT_RESOURCES;
    //  Zero and then initialize the structure.
    RtlZeroMemory( LocalIrpContextLite, sizeof( UDFIrpContextLite ));

    LocalIrpContextLite->NodeIdentifier.NodeType  = UDF_NODE_TYPE_IRP_CONTEXT_LITE;
    LocalIrpContextLite->NodeIdentifier.NodeSize  = sizeof(UDFIrpContextLite);

    LocalIrpContextLite->Fcb = Fcb;
    LocalIrpContextLite->TreeLength = IrpContext->TreeLength;
    //  Copy RealDevice for workque algorithms.
    LocalIrpContextLite->RealDevice = IrpContext->TargetDeviceObject;
    LocalIrpContextLite->IrpContextFlags = IrpContext->IrpContextFlags;
    *IrpContextLite = LocalIrpContextLite;

    return STATUS_SUCCESS;
} // end UDFInitializeIrpContextLite()

NTSTATUS
NTAPI
UDFQuerySetEA(
    PDEVICE_OBJECT DeviceObject,       // the logical volume device object
    PIRP           Irp                 // I/O Request Packet
    )
{
    NTSTATUS         RC = STATUS_SUCCESS;
//    PtrUDFIrpContext PtrIrpContext = NULL;
    BOOLEAN          AreWeTopLevel = FALSE;

    KdPrint(("UDFQuerySetEA: \n"));

    FsRtlEnterFileSystem();
    ASSERT(DeviceObject);
    ASSERT(Irp);

    // set the top level context
    AreWeTopLevel = UDFIsIrpTopLevel(Irp);

    RC = STATUS_EAS_NOT_SUPPORTED;
    Irp->IoStatus.Status = RC;
    Irp->IoStatus.Information = 0;
    // complete the IRP
    IoCompleteRequest(Irp, IO_DISK_INCREMENT);

    if(AreWeTopLevel) {
        IoSetTopLevelIrp(NULL);
    }

    FsRtlExitFileSystem();

    return(RC);
} // end UDFQuerySetEA()

ULONG
UDFIsResourceAcquired(
    IN PERESOURCE Resource
    )
{
    ULONG ReAcqRes =
        ExIsResourceAcquiredExclusiveLite(Resource) ? 1 :
        (ExIsResourceAcquiredSharedLite(Resource) ? 2 : 0);
    return ReAcqRes;
} // end UDFIsResourceAcquired()

BOOLEAN
UDFAcquireResourceExclusiveWithCheck(
    IN PERESOURCE Resource
    )
{
    ULONG ReAcqRes =
        ExIsResourceAcquiredExclusiveLite(Resource) ? 1 :
        (ExIsResourceAcquiredSharedLite(Resource) ? 2 : 0);
    if(ReAcqRes) {
        KdPrint(("UDFAcquireResourceExclusiveWithCheck: ReAcqRes, %x\n", ReAcqRes));
    } else {
//        BrutePoint();
    }

    if(ReAcqRes == 1) {
        // OK
    } else
    if(ReAcqRes == 2) {
        KdPrint(("UDFAcquireResourceExclusiveWithCheck: !!! Shared !!!\n"));
        //BrutePoint();
    } else {
        UDFAcquireResourceExclusive(Resource, TRUE);
        return TRUE;
    }
    return FALSE;
} // end UDFAcquireResourceExclusiveWithCheck()

BOOLEAN
UDFAcquireResourceSharedWithCheck(
    IN PERESOURCE Resource
    )
{
    ULONG ReAcqRes =
        ExIsResourceAcquiredExclusiveLite(Resource) ? 1 :
        (ExIsResourceAcquiredSharedLite(Resource) ? 2 : 0);
    if(ReAcqRes) {
        KdPrint(("UDFAcquireResourceSharedWithCheck: ReAcqRes, %x\n", ReAcqRes));
/*    } else {
        BrutePoint();*/
    }

    if(ReAcqRes == 2) {
        // OK
    } else
    if(ReAcqRes == 1) {
        KdPrint(("UDFAcquireResourceSharedWithCheck: Exclusive\n"));
        //BrutePoint();
    } else {
        UDFAcquireResourceShared(Resource, TRUE);
        return TRUE;
    }
    return FALSE;
} // end UDFAcquireResourceSharedWithCheck()

NTSTATUS
UDFWCacheErrorHandler(
    IN PVOID Context,
    IN PWCACHE_ERROR_CONTEXT ErrorInfo
    )
{
    InterlockedIncrement((PLONG)&(((PVCB)Context)->IoErrorCounter));
    return ErrorInfo->Status;
}

#include "..\include\misc_common.cpp"
#include "..\include\regtools.cpp"

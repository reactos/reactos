#include "iop.h"
#include "srb.h"

//
// This entire file is only present if NO_SPECIAL_IRP isn't defined
//
#ifndef NO_SPECIAL_IRP

//
// When enabled, everything is locked down on demand...
//
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEVRFY, IovpSessionDataCreate)
#pragma alloc_text(PAGEVRFY, IovpSessionDataAdvance)
#pragma alloc_text(PAGEVRFY, IovpSessionDataReference)
#pragma alloc_text(PAGEVRFY, IovpSessionDataDereference)
#pragma alloc_text(PAGEVRFY, IovpSessionDataClose)
#pragma alloc_text(PAGEVRFY, IovpSessionDataDeterminePolicy)
#pragma alloc_text(PAGEVRFY, IovpSessionDataAttachSurrogate)
#pragma alloc_text(PAGEVRFY, IovpSessionDataFinalizeSurrogate)
#endif

#define POOL_TAG_SESSION_DATA       'sprI'

PIOV_SESSION_DATA
FASTCALL
IovpSessionDataCreate(
    IN      PDEVICE_OBJECT       DeviceObject,
    IN OUT  PIOV_REQUEST_PACKET  *IovPacketPointer,
    OUT     PBOOLEAN             SurrogateSpawned
    )
/*++

  Description:

    This routine creates tracking data for a new IRP. It must be called on the
    thread the IRP was originally sent down...

  Arguments:

    Irp                    - Irp to track.

  Return Value:

    iovPacket block, NULL if no memory.

--*/
{
    PIRP irp, surrogateIrp;
    PIOV_SESSION_DATA iovSessionData;
    PIOV_REQUEST_PACKET headPacket;
    ULONG sessionDataSize;
    BOOLEAN trackable, useSurrogateIrp;

    *SurrogateSpawned = FALSE;

    headPacket = (*IovPacketPointer)->HeadPacket;
    ASSERT(headPacket == (*IovPacketPointer));
    irp = headPacket->TrackedIrp;

    //
    // Check the IRP appropriately
    //
    IovpSessionDataDeterminePolicy(
        headPacket,
        DeviceObject,
        &trackable,
        &useSurrogateIrp
        );

    if (!trackable) {

        return NULL;
    }

    //
    // One extra stack location is allocated as the "zero'th" is used to
    // simplify some logic...
    //
    sessionDataSize =
        sizeof(IOV_SESSION_DATA)+
        irp->StackCount*sizeof(IOV_STACK_LOCATION);

    iovSessionData = ExAllocatePoolWithTag(
        NonPagedPool,
        sessionDataSize,
        POOL_TAG_SESSION_DATA
        );

    if (iovSessionData == NULL) {

        return NULL;
    }

    RtlZeroMemory(iovSessionData, sessionDataSize);

    iovSessionData->AssertFlags = IovpTrackingFlags;
    iovSessionData->IovRequestPacket = headPacket;
    InsertHeadList(&headPacket->SessionHead, &iovSessionData->SessionLink);

    if ((iovSessionData->AssertFlags&ASSERTFLAG_COMPLETEATPASSIVE) ||
        (iovSessionData->AssertFlags&ASSERTFLAG_DEFERCOMPLETION)) {

        iovSessionData->AssertFlags |= ASSERTFLAG_FORCEPENDING;
    }

    headPacket->pIovSessionData = iovSessionData;
    headPacket->TopStackLocation = irp->CurrentLocation;
    headPacket->Flags |= TRACKFLAG_ACTIVE;
    headPacket->Flags &=~ (
                          TRACKFLAG_QUEUED_INTERNALLY|
                          TRACKFLAG_RELEASED|
                          TRACKFLAG_SRB_MUNGED|
                          TRACKFLAG_SWAPPED_BACK|
                          TRACKFLAG_PASSED_FAILURE
                         );

    iovSessionData->BestVisibleIrp = irp;
    if (useSurrogateIrp) {

        //
        // We will track the IRP using a surrogate.
        //
        *SurrogateSpawned = IovpSessionDataAttachSurrogate(
            IovPacketPointer,
            iovSessionData
            );
    }

    TRACKIRP_DBGPRINT((
        "  SSN CREATE(%x)->%x\n",
        headPacket,
        iovSessionData
        ), 3) ;
    return iovSessionData;
}

VOID
FASTCALL
IovpSessionDataAdvance(
    IN      PDEVICE_OBJECT       DeviceObject,
    IN      PIOV_SESSION_DATA    IovSessionData,
    IN OUT  PIOV_REQUEST_PACKET  *IovPacketPointer,
    OUT     PBOOLEAN             SurrogateSpawned
    )
{
    *SurrogateSpawned = FALSE;
}

VOID
FASTCALL
IovpSessionDataDereference(
    IN PIOV_SESSION_DATA IovSessionData
    )
{
    PIOV_REQUEST_PACKET iovPacket, headPacket;

    iovPacket = IovSessionData->IovRequestPacket;
    headPacket = iovPacket->HeadPacket;

    ASSERT_SPINLOCK_HELD(&headPacket->IrpLock);
    ASSERT_SPINLOCK_HELD(&iovPacket->IrpLock);
    ASSERT(IovSessionData->SessionRefCount > 0);
    ASSERT(headPacket->ReferenceCount > 0);
    ASSERT(iovPacket->ReferenceCount >= 0);

    TRACKIRP_DBGPRINT((
        "  SSN DEREF(%x) %x--\n",
        IovSessionData,
        IovSessionData->SessionRefCount
        ), 3) ;

    IovSessionData->SessionRefCount--;
    if (!IovSessionData->SessionRefCount) {

        ASSERT(headPacket->pIovSessionData != IovSessionData);
        ASSERT(iovPacket->ReferenceCount > iovPacket->PointerCount);
        //ASSERT(IsListEmpty(&IovSessionData->SessionLink));
        RemoveEntryList(&IovSessionData->SessionLink);
        InitializeListHead(&IovSessionData->SessionLink);

        IovpTrackingDataDereference(iovPacket, IOVREFTYPE_PACKET);

        ExFreePool(IovSessionData);
    }
}

VOID
FASTCALL
IovpSessionDataReference(
    IN PIOV_SESSION_DATA IovSessionData
    )
{
    PIOV_REQUEST_PACKET iovPacket, headPacket;

    iovPacket = IovSessionData->IovRequestPacket;
    headPacket = iovPacket->HeadPacket;

    ASSERT_SPINLOCK_HELD(&headPacket->IrpLock);
    ASSERT_SPINLOCK_HELD(&iovPacket->IrpLock);
    ASSERT(IovSessionData->SessionRefCount >= 0);
    ASSERT(headPacket->ReferenceCount >= 0);
    ASSERT(iovPacket->ReferenceCount >= 0);

    TRACKIRP_DBGPRINT((
        "  SSN REF(%x) %x++\n",
        IovSessionData,
        IovSessionData->SessionRefCount
        ), 3) ;

    if (!IovSessionData->SessionRefCount) {

        IovpTrackingDataReference(iovPacket, IOVREFTYPE_PACKET);
    }
    IovSessionData->SessionRefCount++;
}

VOID
FASTCALL
IovpSessionDataClose(
    IN PIOV_SESSION_DATA IovSessionData
    )
{
   PIOV_REQUEST_PACKET iovPacket = IovSessionData->IovRequestPacket;

   ASSERT_SPINLOCK_HELD(&iovPacket->IrpLock);

   ASSERT(iovPacket == iovPacket->HeadPacket);
   ASSERT(iovPacket->pIovSessionData == IovSessionData);

   TRACKIRP_DBGPRINT((
       "  SSN CLOSE(%x)\n"
       ), 3) ;

   iovPacket->Flags &=~ TRACKFLAG_ACTIVE;
   iovPacket->pIovSessionData = NULL;
}

VOID
IovpSessionDataDeterminePolicy(
    IN   PIOV_REQUEST_PACKET IovRequestPacket,
    IN   PDEVICE_OBJECT      DeviceObject,
    OUT  PBOOLEAN            Trackable,
    OUT  PBOOLEAN            UseSurrogateIrp
    )
/*++

  Description:

    This routine is called by IovpCallDriver1 to determine which IRPs should
    be tracked and how that tracking should be done.

  Arguments:

    ADRIAO BUGBUG 12/31/1998 - Fill this out.

  Return Value:

     None.

--*/
{
    PIO_STACK_LOCATION irpSp;
    PIRP irp;

    irp = IovRequestPacket->TrackedIrp;

    if (!(IovpTrackingFlags&ASSERTFLAG_TRACKIRPS)) {

        //
        // No IRPs are to be tracked. Exit now.
        //
        *Trackable = FALSE;
        return;
    }

    //
    // Determine whether we are to monitor this IRP. If we are going to test
    // any one driver in a stack, then we must unfortunately monitor the IRP's
    // progress through the *entire* stack. Thus our granularity here is stack
    // based, not device based! We will compensate for this somewhat in the
    // driver check code, which will attempt to ignore asserts from those
    // "non-targetted" drivers who happen to have screwed up in our stack...
    //
    *Trackable = IovpIsInterestingStack(DeviceObject);

    irpSp = IoGetNextIrpStackLocation( irp );

    if (IovpTrackingFlags&ASSERTFLAG_POLICEIRPS) {

        *UseSurrogateIrp = ((IovpTrackingFlags&ASSERTFLAG_SURROGATE)!=0) ;
        *UseSurrogateIrp &= ((irpSp->MajorFunction != IRP_MJ_SCSI) ||
                        ((IovpTrackingFlags&ASSERTFLAG_SMASH_SRBS)!=0)) ;

#ifdef HACKHACKS_ENABLED

        //
        // ADRIAO HACKHACK #05 05/30/98 -
        // We don't surrogate creates right now because MUP touches IRPs
        // after they are completed.
        //
        if (IovpHackFlags&HACKFLAG_FOR_MUP) {
            *UseSurrogateIrp &= (irpSp->MajorFunction != IRP_MJ_CREATE) ;
        }
#endif
    } else {

        *UseSurrogateIrp = FALSE;
    }
}

BOOLEAN
FASTCALL
IovpSessionDataAttachSurrogate(
    IN OUT  PIOV_REQUEST_PACKET  *IovPacketPointer,
    IN      PIOV_SESSION_DATA    IovSessionData
    )
/*++

  Description:

    This routine creates tracking data for a new IRP. It must be called on the
    thread the IRP was originally sent down...

  Arguments:

    IovPacketPointer       - Pointer to IRP packet to attach surrogate to. If
                             a surrogate can be attached the packet will be
                             updated to track the surrogate.

    SurrogateIrp           - Prepared surrogate IRP to attach.

  Return Value:

    iovPacket block, NULL if no memory.

--*/
{

    PIOV_REQUEST_PACKET iovSurrogatePacket, iovPacket, headPacket;
    PIRP surrogateIrp, irp;
    PIO_STACK_LOCATION irpSp;
    PSCSI_REQUEST_BLOCK srb;
    PVOID restoreHandle;
    CCHAR activeSize;

    iovPacket = *IovPacketPointer;
    ASSERT_SPINLOCK_HELD(&iovPacket->IrpLock);
    ASSERT(!(CONTAINING_RECORD(
        iovPacket->SurrogateLink.Flink,
        IOV_REQUEST_PACKET,
        SurrogateLink)->Flags&TRACKFLAG_SURROGATE
        ));

    ASSERT(iovPacket->Flags & TRACKFLAG_ACTIVE);

    irp = iovPacket->TrackedIrp;
    activeSize = (irp->CurrentLocation-1);
    ASSERT(activeSize);

    //
    // We now try to make a copy of this new IRP which we will track. We
    // do this so that we may free *every* tracked IRP immediately upon
    // completion.
    // Technically speaking, we only need to allocate what's left of the
    // stack, not the entire thing. But using the entire stack makes our
    // work much much easier. Specifically the session stack array may depend
    // on this.
    //
    // ADRIAO BUGBUG 03/04/1999 - Make this work only copying a portion of the
    // IRP.
    //
    surrogateIrp = IovpProtectedIrpAllocate(
        irp->StackCount, // activeSize
        (BOOLEAN) ((irp->AllocationFlags&IRP_QUOTA_CHARGED) ? TRUE : FALSE),
        NULL
        );

    if (surrogateIrp == NULL) {

        return FALSE;
    }

    //
    // Now set up the new IRP - we do this here so IovpTrackingDataCreateAndLock
    // can peek at it's fields. Start with the IRP header.
    //
    RtlCopyMemory(surrogateIrp, irp, sizeof(IRP));

    //
    // Adjust StackCount and CurrentLocation
    //
    surrogateIrp->StackCount = irp->StackCount; // activeSize
    surrogateIrp->Tail.Overlay.CurrentStackLocation =
        ((PIO_STACK_LOCATION) (surrogateIrp+1))+activeSize;

    //
    // Our new IRP "floats", and is not attached to any thread.
    // Note that all cancels due to thread death will come through the
    // original IRP.
    //
    InitializeListHead(&surrogateIrp->ThreadListEntry);

    //
    // Our new IRP also is not connected to user mode.
    //
    surrogateIrp->UserEvent = NULL;
    surrogateIrp->UserIosb = NULL;

    //
    // Now copy over only the active portions of IRP. Be very careful to not
    // assume that the last stack location is right after the end of the IRP,
    // as we may change this someday!
    //
    irpSp = (IoGetCurrentIrpStackLocation(irp)-activeSize);
    RtlCopyMemory(surrogateIrp+1, irpSp, sizeof(IO_STACK_LOCATION)*activeSize);

    //
    // Zero the portion of the new IRP we won't be using (this should
    // eventually go away).
    //
    RtlZeroMemory(
        ((PIO_STACK_LOCATION) (surrogateIrp+1))+activeSize,
        sizeof(IO_STACK_LOCATION)*(surrogateIrp->StackCount - activeSize)
        );

    //
    // Now create a surrogate packet to track the new IRP.
    //
    iovSurrogatePacket = IovpTrackingDataCreateAndLock(surrogateIrp);
    if (iovSurrogatePacket == NULL) {

        //
        // ADRIAO BUGBUG 02/11/1999 - Need to free IRP
        //
        restoreHandle = IovpProtectedIrpMakeUntouchable(surrogateIrp, TRUE);
        IovpProtectedIrpFree(surrogateIrp, restoreHandle);
        return FALSE;
    }

    headPacket = iovPacket->HeadPacket;

    ASSERT(iovSurrogatePacket->CallerIrql == DISPATCH_LEVEL);
    irpSp = IoGetNextIrpStackLocation(irp);

    //
    // We will flag this bug later.
    //
    irp->CancelRoutine = NULL;

    //
    // Let's take advantage of the original IRP not being the thing partied on
    // now; store a pointer to our tracking data in the information field. We
    // don't use this, but it's nice when debugging...
    //
    irp->IoStatus.Information = (ULONG_PTR) iovPacket;

    //
    // ADRIAO BUGBUG #28 06/10/98 - This is absolutely *gross*, and not
    //                              deterministic enough for my tastes.
    //
    // For IRP_MJ_SCSI (ie, IRP_MJ_INTERNAL_DEVICE_CONTROL), look and see
    // if we have an SRB coming through. If so, fake out the OriginalRequest
    // IRP pointer as appropriate.
    //
    if (irpSp->MajorFunction == IRP_MJ_SCSI) {
        srb = irpSp->Parameters.Others.Argument1 ;
        if (IopIsMemoryRangeReadable(srb, SCSI_REQUEST_BLOCK_SIZE)) {
            if ((srb->Length == SCSI_REQUEST_BLOCK_SIZE)&&(srb->OriginalRequest == irp)) {
                srb->OriginalRequest = surrogateIrp ;
                headPacket->Flags |= TRACKFLAG_SRB_MUNGED ;
            }
        }
    }

    //
    // Since the replacement will never make it back to user mode (the real
    // IRP shall of course), we will steal a field or two for debugging info.
    //
    surrogateIrp->UserIosb = (PIO_STATUS_BLOCK) iovPacket ;

    //
    // Now that everything is built correctly, attach the surrogate. The
    // surrogate holds down the packet we are attaching to. When the surrogate
    // dies we will remove this reference.
    //
    IovpTrackingDataReference(iovPacket, IOVREFTYPE_POINTER);

    //
    // Stamp IRPs appropriately.
    //
    surrogateIrp->Flags |= IRP_DIAG_IS_SURROGATE;
    irp->Flags |= IRP_DIAG_HAS_SURROGATE;

    //
    // Mark packet as surrogate and inherit appropriate fields from iovPacket.
    //
    iovSurrogatePacket->Flags |= TRACKFLAG_SURROGATE | TRACKFLAG_ACTIVE;
    iovSurrogatePacket->pIovSessionData = iovPacket->pIovSessionData;
    iovSurrogatePacket->AssertFlags = iovPacket->AssertFlags;
    iovSurrogatePacket->HeadPacket = iovPacket->HeadPacket;
    iovSurrogatePacket->LastLocation = iovPacket->LastLocation;
    iovSurrogatePacket->TopStackLocation = irp->CurrentLocation;

    iovPacket->Flags |= TRACKFLAG_HAS_SURROGATE;

    //
    // Fix up IRQL's so spinlocks are released in the right order. Link'm.
    //
    iovSurrogatePacket->CallerIrql = iovPacket->CallerIrql;
    iovPacket->CallerIrql = DISPATCH_LEVEL;
    InsertTailList(
        &iovPacket->HeadPacket->SurrogateLink,
        &iovSurrogatePacket->SurrogateLink
        );

    *IovPacketPointer = iovSurrogatePacket;
    return TRUE;
}

VOID
FASTCALL
IovpSessionDataFinalizeSurrogate(
    IN      PIOV_SESSION_DATA    IovSessionData,
    IN OUT  PIOV_REQUEST_PACKET  IovPacket,
    IN      PIRP                 SurrogateIrp
    )
/*++

  Description:

    This routine removes the flags from both the real and
    surrogate IRP and records the final IRP settings. Finally,
    the surrogate IRP is made "untouchable" (decommitted).

  Arguments:

    iovPacket              - Pointer to the IRP tracking data.

  Return Value:

     None.
--*/
{
    PIOV_REQUEST_PACKET iovPrevPacket;
    NTSTATUS status, lockedStatus;
    ULONG nonInterestingFlags;
    PIO_STACK_LOCATION irpSp;
    PVOID restoreHandle;
    PIRP irp;

    ASSERT(IovPacket->Flags&TRACKFLAG_SURROGATE);

    ASSERT(IovpTrackingDataGetCurrentSessionData(IovPacket) == IovSessionData);

    //
    // It's a surrogate, do as appropriate.
    //
    ASSERT(IovPacket->TopStackLocation == SurrogateIrp->CurrentLocation+1) ;

    iovPrevPacket = CONTAINING_RECORD(
        IovPacket->SurrogateLink.Blink,
        IOV_REQUEST_PACKET,
        SurrogateLink
        );

    irp = iovPrevPacket->TrackedIrp;

    //
    // Carry the pending bit over.
    //
    if (SurrogateIrp->PendingReturned) {
        IoMarkIrpPending(irp);
    }

    nonInterestingFlags = (
        IRPFLAG_EXAMINE_MASK |
        IRP_DIAG_IS_SURROGATE|
        IRP_DIAG_HAS_SURROGATE
        );

    //
    // Wipe the flags nice and clean
    //
    SurrogateIrp->Flags &=~ IRP_DIAG_IS_SURROGATE;
    irp->Flags          &=~ IRP_DIAG_HAS_SURROGATE;

    //
    // ASSERT portions of the IRP header have not changed.
    //
    ASSERT(irp->StackCount == SurrogateIrp->StackCount); // Later to be removed

    ASSERT(irp->Type == SurrogateIrp->Type);
    ASSERT(irp->RequestorMode == SurrogateIrp->RequestorMode);
    ASSERT(irp->ApcEnvironment == SurrogateIrp->ApcEnvironment);
    ASSERT(irp->AllocationFlags == SurrogateIrp->AllocationFlags);
    ASSERT(irp->UserBuffer == SurrogateIrp->UserBuffer);
    ASSERT(irp->Tail.Overlay.Thread == SurrogateIrp->Tail.Overlay.Thread);

    ASSERT(
        irp->Overlay.AsynchronousParameters.UserApcRoutine ==
        SurrogateIrp->Overlay.AsynchronousParameters.UserApcRoutine
        );

    ASSERT(
        irp->Overlay.AsynchronousParameters.UserApcContext ==
        SurrogateIrp->Overlay.AsynchronousParameters.UserApcContext
        );

    ASSERT(
        irp->Tail.Overlay.OriginalFileObject ==
        SurrogateIrp->Tail.Overlay.OriginalFileObject
        );

    ASSERT(
        irp->Tail.Overlay.AuxiliaryBuffer ==
        SurrogateIrp->Tail.Overlay.AuxiliaryBuffer
        );

/*
    ASSERT(
        irp->AssociatedIrp.SystemBuffer ==
        SurrogateIrp->AssociatedIrp.SystemBuffer
        );

    ASSERT(
        (irp->Flags          & ~nonInterestingFlags) ==
        (SurrogateIrp->Flags & ~nonInterestingFlags)
        );

    ASSERT(irp->MdlAddress == SurrogateIrp->MdlAddress);
*/
    //
    // ADRIAO BUGBUG 02/28/1999 -
    //     How do these change as an IRP progresses?
    //
    irp->Flags |= SurrogateIrp->Flags;
    irp->MdlAddress = SurrogateIrp->MdlAddress;
    irp->AssociatedIrp.SystemBuffer = SurrogateIrp->AssociatedIrp.SystemBuffer;

    if ((irp->Flags&IRP_DEALLOCATE_BUFFER)&&
        (irp->AssociatedIrp.SystemBuffer == NULL)) {

        irp->Flags &=~ IRP_DEALLOCATE_BUFFER;
    }

    //
    // Copy the salient fields back. We only need to touch certain areas of the
    // header.
    //
    irp->IoStatus = SurrogateIrp->IoStatus;
    irp->PendingReturned = SurrogateIrp->PendingReturned;
    irp->Cancel = SurrogateIrp->Cancel;

    iovPrevPacket->Flags &=~ TRACKFLAG_HAS_SURROGATE;

    //
    // Record data from it and make the system fault if the IRP is touched
    // after this completion routine.
    //
    IovSessionData->BestVisibleIrp = irp;

    IovpTrackingDataDereference(iovPrevPacket, IOVREFTYPE_POINTER);

    ASSERT(IovPacket->PointerCount == 0);

    restoreHandle = IovpProtectedIrpMakeUntouchable(SurrogateIrp, TRUE);
    IovpProtectedIrpFree(SurrogateIrp, &restoreHandle);
}
#endif // NO_SPECIAL_IRP


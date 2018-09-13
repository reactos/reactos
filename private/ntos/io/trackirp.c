/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    trackirp.c

Abstract:

    This module tracks irps and verified drivers when people do stupid things with
    them.

    Note to people hitting bugs in these code paths due to core changes:

    -   "This file is NOT vital to operation of the OS, and could easily be
         disabled while a redesign to compensate for the core change is
         implemented." - the author

Author:

    Adrian J. Oney (adriao) 09-May-1998

Environment:

    Kernel mode

Revision History:

Known BUGBUGs:

    ADRIAO BUGBUG   #07 05/11/98 - Add a pass-through filter at every attach.
    ADRIAO BUGBUG   #05 05/12/98 - Find a way to check pends when not forced.
    ADRIAO BUGBUG   #17 05/30/98 - WMI IRPs may assert second time erroneously.
    ADRIAO BUGBUG   #28 06/10/98 - Need to find a better way to id SCSI SRBs
    ADRIAO BUGBUG       08/16/98 - Pass on quota charging iff appropriate...
    ADRIAO BUGBUG       08/19/98 - Don't use hardcoded number.

Known HACKHACKs:

    ADRIAO HACKHACK #05 05/30/98 - Create IRPs aren't surrogated as MUP chokes.
                                   (HACKHACK_FOR_MUP)
    ADRIAO HACKHACK #10 06/12/98 - Scsiport never skips, so I rip too much to boot.
                                   (HACKHACK_FOR_SCSIPORT)

--*/

#include "iop.h"

#if (( defined(_X86_) ) && ( FPO ))
#pragma optimize( "y", off )    // disable FPO for consistent stack traces
#endif

#define POOL_TAG_DEFERRED_CONTEXT   'dprI'

#define HACKHACK_FOR_MUP
#define HACKHACK_FOR_SCSIPORT
#define HACKHACK_FOR_ACPI
#define HACKHACK_FOR_BOGUSIRPS

//
// This entire file is only present if NO_SPECIAL_IRP isn't defined
//
#ifndef NO_SPECIAL_IRP

//
// When enabled, everything is locked down on demand...
//
#ifdef ALLOC_PRAGMA
//#pragma alloc_text(NONPAGE, IovpDoAssertIrps)
#pragma alloc_text(PAGE,     IovpInitIrpTracking)
#pragma alloc_text(PAGE,     IovpReexamineAllStacks)
#pragma alloc_text(PAGEVRFY, IovpCallDriver1)
#pragma alloc_text(PAGEVRFY, IovpCallDriver2)
#pragma alloc_text(PAGEVRFY, IovpCompleteRequest1)
#pragma alloc_text(PAGEVRFY, IovpCompleteRequest2)
#pragma alloc_text(PAGEVRFY, IovpCompleteRequest3)
#pragma alloc_text(PAGEVRFY, IovpCompleteRequest4)
#pragma alloc_text(PAGEVRFY, IovpCompleteRequest5)
#pragma alloc_text(PAGEVRFY, IovpCompleteRequest)
#pragma alloc_text(PAGEVRFY, IovpCancelIrp)
#pragma alloc_text(PAGEVRFY, IovpFreeIrp)
#pragma alloc_text(PAGEVRFY, IovpAllocateIrp1)
#pragma alloc_text(PAGEVRFY, IovpAllocateIrp2)
#pragma alloc_text(PAGEVRFY, IovpInitializeIrp)
#pragma alloc_text(PAGEVRFY, IovpAttachDeviceToDeviceStack)
#pragma alloc_text(PAGEVRFY, IovpDetachDevice)
#pragma alloc_text(PAGEVRFY, IovpDeleteDevice)
#pragma alloc_text(PAGEVRFY, IovpInternalCompletionTrap)
#pragma alloc_text(PAGEVRFY, IovpSwapSurrogateIrp)
#pragma alloc_text(PAGEVRFY, IovpProtectedIrpAllocate)
#pragma alloc_text(PAGEVRFY, IovpProtectedIrpMakeTouchable)
#pragma alloc_text(PAGEVRFY, IovpProtectedIrpMakeUntouchable)
#pragma alloc_text(PAGEVRFY, IovpProtectedIrpFree)
#pragma alloc_text(PAGEVRFY, IovpExamineDevObjForwarding)
#pragma alloc_text(PAGEVRFY, IovpExamineIrpStackForwarding)
#pragma alloc_text(PAGEVRFY, IovpGetDeviceAttachedTo)
#pragma alloc_text(PAGEVRFY, IovpGetLowestDevice)
#pragma alloc_text(PAGEVRFY, IovpAssertNonLegacyDevice)
#pragma alloc_text(PAGEVRFY, IovpIsInFdoStack)
#pragma alloc_text(PAGEVRFY, IovpSeedStack)
#pragma alloc_text(PAGEVRFY, IovpSeedOnePage)
#pragma alloc_text(PAGEVRFY, IovpSeedTwoPages)
#pragma alloc_text(PAGEVRFY, IovpSeedThreePages)
#pragma alloc_text(PAGEVRFY, IovpInternalDeferredCompletion)
#pragma alloc_text(PAGEVRFY, IovpInternalCompleteAfterWait)
#pragma alloc_text(PAGEVRFY, IovpInternalCompleteAtDPC)
#pragma alloc_text(PAGEVRFY, IovpAdvanceStackDownwards)
#pragma alloc_text(PAGEVRFY, IovpEnumDevObjCallback)
#pragma alloc_text(PAGEVRFY, IovpIsInterestingStack)
#pragma alloc_text(PAGEVRFY, IovpIsInterestingDriver)
#endif

//
// These flags control the tracking features and which hacks are enabled.
// Both values will be set to the appropriate values if they are found
// to be -1 at boot time. If they are changed at ^K time they will not
// be subsequently overridden. 7FFFFFFF and 0 will do the maximum level
// of testing...
//
ULONG IovpTrackingFlags = 0; //(ULONG) -1;
ULONG IovpHackFlags = (ULONG) -1;

//
// This global flag is set if assertions have been enabled. It will never
// transition from TRUE to FALSE, so it is safe to do a quick check for
// TRUE outside a spinlock. Furthermore, a false "FALSE" is guarenteed to
// be safe.
//
BOOLEAN IovpIrpTrackingEnabled = FALSE;

//
// Flags that indicate how we were initialized.
//
ULONG IovpInitFlags = 0;

//
// This counter is used in picking random IRPs to cancel
//
ULONG IovpCancelCount = 0;

//
// This is the time in 100ns units to defer an IRP if so told.
//
// ADRIAO BUGBUG 08/19/98 - Don't use hardcoded number.
//
LONG IovpIrpDeferralTime = 10 * 300; // 300us

/*
 * - The IRP verification code works as follows -
 *
 * To enforce the correct handling of an IRP, we must maintain some data about
 * it. But the IRP is a public structure and as drivers are allowed to create
 * IRPs without using IoAllocateIrp we cannot add any fields to it. Therefore
 * we maintain out own side structures that are looked up via a hash table.
 *
 * IOV_REQUEST_PACKETs cover the lifetime of the IRP from allocation to
 * deallocation, and from there (sans pointer) until all "references" have
 * been dropped, which may happen long after the IRP itself was freed and
 * recycled.
 *
 * When an IRP is progress down a stack, a "session" is allocated. An
 * IovRequestPacket has a current session until such time as the IRP is
 * completed. The session still exists until all references are dropped, but
 * before that happens a new session may become the current session (ie the IRP
 * was sent back down before the previous call stacks unwound). The tracking
 * data is held around until all sessions have decayed.
 *
 * Each session has an array of stack locations corresponding to those in use
 * by the IRP. These IOV_STACK_LOCATIONs are used to track "requests" within
 * the IRP, ie the passage of a major/minor/parameter set down the stack.
 * Of course multiple requests may exist in the same session/stack at once.
 *
 * Finally, surrogates. The IoVerifier may "switch" the IRP in use as it goes
 * down the stack. In this case the new IRP is usually allocated from the
 * special pool and freed as early as possible to catch bugs (people who touch
 * after completes). Each surrogate gets it's own IovRequestPacket, which is
 * linked to the previous surrogate or real irp in use prior to it.
 *
 *   +--------------------+                     +--------------------+
 *   | IOV_REQUEST_PACKET |                     | IOV_REQUEST_PACKET |
 *   |   (original irp)   |<--------------------|    (surrogate)     |
 *   |                    |                     |                    |
 *   +--------------------+                     +--------------------+
 *                 ||
 *                 v
 *    +-------------------+       +-------------------------+
 *    | IOV_SESSION_DATA  |       | IOV_STACK_LOCATION[...] |
 *    | (current session) |------>|    (per IrpSp data)     |
 *    |                   |       |                         |
 *    +-------------------+       +-------------------------+
 *
 *
 * The following flags change the behavior of IRPs memory allocation,
 * and code preemption in the OS. They should not neccessarily be on by
 * default, as they will seriously Heisenburg the system...
 *
 * ASSERTFLAG_TRACKIRPS         - If this is not on, all of the below (excepting
 *                                ASSERTFLAG_MONITOR_ALLOCS) do not occur, and
 *                                IRPs are handled as in the free build (with
 *                                very few assertions). If on, all IRPs are
 *                                tracked, but not asserted on.
 *
 * ASSERTFLAG_MONITOR_ALLOCS    - Calls to IoAllocateIrp go through the Special
 *                                pool. For every IRP so allocated a snapshot
 *                                of the thread stack at allocation time is
 *                                taken.
 *
 * ASSERTFLAG_POLICEIRPS        - Monitors IRPs for basic/common mistakes.
 *                                Required for below flags to be used.
 *
 * ASSERTFLAG_MONITORMAJORS     - Catches issues specific to various Major and
 *                                minor specific issues.
 *
 * ASSERTFLAG_SURROGATE         - Tracked IRPs are automatically freed upon
 *                                completion. This is done with a surrogate
 *                                IRP allocated from the special pool that
 *                                replaces the original while travelling down
 *                                the stack.
 *
 * ASSERTFLAG_SMASH_SRBS        - Some SCSI IRPs can't be surrogated unless
 *                                the SRB->OriginalRequest pointer is updated.
 *                                This is due to a busted SRB architecture.
 *                                Note that the technique used to identify an
 *                                SRB IRP is "fuzzy", and could in theory touch
 *                                an IRP it shouldn't have!
 *
 * ASSERTFLAG_FORCEPENDING      - Tracked IRPs are automatically pended, but
 *                                are not held for any period of time.
 *
 * ASSERTFLAG_DEFERCOMPLETION   - Tracked IRPs are completed later via timer.
 *                                ASSERTFLAG_FORCEPENDING set by inference.
 *
 * ASSERTFLAG_COMPLETEATDPC     - completes every IRP at DPC, regardless of
 *                                major function.
 *
 * ASSERTFLAG_COMPLETEATPASSIVE - completes every IRP as Passive, regardless
 *                                of major function. ASSERTFLAG_FORCEPENDING
 *                                is set by inference.
 *
 * ASSERTFLAG_CONSUME_ALWAYS    - Stack locations are forced to be copied (ie
 *                                any skips are undone).  Note that we do not
 *                                consume if the IRP was just forwarded to
 *                                another stack.
 *
 * ASSERTFLAG_ROTATE_STATUS     - Alternate successful status's are chosen where
 *                                appropriate as the IRP returns up the stack.
 *                                This catches many IRP forwarding bugs.
 *
 * ASSERTFLAG_SEEDSTACK         - Seeds the stack so that uninitialized
 *                                variables are caught more easily...
 *
 */

BOOLEAN
FASTCALL
IovpInitIrpTracking(
    IN ULONG   Level,
    IN ULONG   Flags
    )
/*++

  Description:

    Initialize that which needs to be initialized.

  Arguments:

    Level         - Level of testing to apply
                      0 - No checks
                      1 - Tracking with surrogate irp allocation
                      2 - Monitors basic IRP mistakes
                      3 - Monitors mistakes based on the irp major
                          Surrogate IRPs used, status values rotated,
                          and various other checks.
                      4 - IRPs always completed at DPC.
                      5 - All IRPs pended with completion defered via timer.
                      6 - Any hacks turned off.

  Return Value:

    Returns TRUE iff settings were successfully applied.

--*/
{
    PVOID sectionHeaderHandle;
    ULONG newTrackingFlags;

    PAGED_CODE();

    if (IovpIrpTrackingEnabled) {

        IovpInitFlags = (Flags | (IovpInitFlags&IOVERIFIERINIT_EVERYTHING_TRACKED));

    } else {

        IovpTrackingDataInit();

        IovpInitFlags = Flags;
        IovpIrpTrackingEnabled = TRUE;
    }

    newTrackingFlags = 0;
    switch(Level) {

        default:
        case 7:
            IovpHackFlags = 0;

        case 6:
            newTrackingFlags |= ASSERTFLAG_FORCEPENDING |
                                ASSERTFLAG_DEFERCOMPLETION;
            //
            // Fall through
            //

        case 5:
            newTrackingFlags |= ASSERTFLAG_COMPLETEATDPC;
            //
            // Fall through
            //

        case 4:
            newTrackingFlags |= ASSERTFLAG_SURROGATE |
                                ASSERTFLAG_SMASH_SRBS |
                                ASSERTFLAG_CONSUME_ALWAYS |
                                ASSERTFLAG_ROTATE_STATUS |
                                ASSERTFLAG_SEEDSTACK;
            //
            // Fall through
            //

        case 3:
            newTrackingFlags |= ASSERTFLAG_MONITORMAJORS;
            //
            // Fall through
            //

        case 2:
            newTrackingFlags |= ASSERTFLAG_POLICEIRPS;
            //
            // Fall through
            //

        case 1:
            newTrackingFlags |= ASSERTFLAG_TRACKIRPS |
                                ASSERTFLAG_MONITOR_ALLOCS;
            //
            // Fall through
            //

        case 0:
            break;
    }

    if ((Level == 0) && (Flags&IOVERIFIERINIT_NO_REINIT)) {

        //
        // Preinit flags
        //
        newTrackingFlags = IovpTrackingFlags;
    }

    if (IovpHackFlags == (ULONG) -1) {

        IovpHackFlags =
#ifdef HACKHACKS_ENABLED
#ifdef HACKHACK_FOR_MUP
                         HACKFLAG_FOR_MUP |
#endif
#ifdef HACKHACK_FOR_SCSIPORT
                         HACKFLAG_FOR_SCSIPORT |
#endif
#ifdef HACKHACK_FOR_ACPI
                         HACKFLAG_FOR_ACPI |
#endif
#ifdef HACKHACK_FOR_BOGUSIRPS
                         HACKFLAG_FOR_BOGUSIRPS |
#endif
#endif // HACKHACKS_ENABLED
                         0;
    }

    if (!(IovpInitFlags & IOVERIFIERINIT_EVERYTHING_TRACKED)) {

        //
        // These options aren't available unless we were marking IRPs since
        // boot.
        //
        newTrackingFlags &=~ (
                              ASSERTFLAG_MONITORMAJORS |
                              ASSERTFLAG_SURROGATE |
                              ASSERTFLAG_SMASH_SRBS |
                              ASSERTFLAG_CONSUME_ALWAYS
                             );
    }

    IovpTrackingFlags = newTrackingFlags;
    return TRUE;
}

BOOLEAN
FASTCALL
IovpDoAssertIrps(
    VOID
    )
/*++

  Description:

    This routine is called to ensure we can do IRP assertions. When called we
    lock the neccessary data structures and code if we haven't been initialized.

  Arguments: None

  Return Value:

    TRUE if assertions can be done, FALSE otherwise (e.g.,
    called at DPC time and we weren't already enabled)

--*/
{
    ASSERT(IovpTrackingFlags);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL) ;

    //
    // If we aren't enabled, call the enabling function. This is harmless to
    // call repeatedly. We are not gaurenteed to be enabled when this function
    // returns (it can't block anyway, as paging might need to occur, and we'd
    // be sitting on file system IRPs). The IOVERIFIERINIT_EVERYTHING_TRACKED is
    // used to let us know we caught all IRPs, ie none are outstanding that
    // haven't been tracked/marked in some manner. We can set this here as
    // SPECIALIRP_MARK_NON_TRACKABLE() is called in the normal IofCallDriver
    // code paths if SPECIAL_IRP's are enabled.
    //
    if (!IovpIrpTrackingEnabled) {

#if 0
        IoVerifierInit(
            DRIVER_VERIFIER_IO_CHECKING,
            IOVERIFIERINIT_EVERYTHING_TRACKED |
            IOVERIFIERINIT_ASYNCHRONOUSINIT |
            IOVERIFIERINIT_NO_REINIT
            );
#endif
    }

    //
    // If enabled, return so.
    //
    return IovpIrpTrackingEnabled;
}

/*
 * The 13 routines listed below -
 *   IovpCallDriver1
 *   IovpCallDriver2
 *   IovpCompleteRequest1
 *   IovpCompleteRequest2
 *   IovpCompleteRequest3
 *   IovpCompleteRequest4
 *   IovpCompleteRequest5
 *   IovpCompleteRequest
 *   IovpCancelIrp
 *   IovpFreeIrp
 *   IovpAllocateIrp1
 *   IovpAllocateIrp2
 *   IovpInitializeIrp
 * and their helper routine
 *   IovpSwapSurrogateIrp
 *
 * - all hook into various parts IofCallDriver and IofCompleteRequest to
 * track the IRP through it's life and determine whether it has been handled
 * correctly. Some of them may even change internal variables in the hooked
 * function. Most dramatically, IovpCallDriver1 may build a
 * replacement Irp which will take the place of the one passed into
 * IoCallDriver.
 *
 *   All of the below functions use a tracking structure called (reasonably
 * enough) IRP_TRACKING_DATA. This lasts the longer of the call stack
 * unwinding or the IRP completing.
 *
 */

#define FAIL_CALLER_OF_IOFCALLDRIVER(msg, irpSp) \
    WDM_FAIL_CALLER(msg, 3+2*((irpSp)->MajorFunction == IRP_MJ_POWER))

#define FAIL_CALLER_OF_IOFCALLDRIVER2(msg, irpSp) \
    WDM_FAIL_CALLER(msg, 4+2*((irpSp)->MajorFunction == IRP_MJ_POWER))

VOID
FASTCALL
IovpCallDriver1(
    IN OUT PIRP           *IrpPointer,
    IN     PDEVICE_OBJECT DeviceObject,
    IN OUT PIOFCALLDRIVER_STACKDATA IofCallDriverStackData
    )
/*++

  Description:

    This routine is called by IofCallDriver just before adjusting
    the IRP stack and calling the driver's dispatch routine.

  Arguments:

    IrpPointer             - a pointer* to the IRP passed in to
                             IofCallDriver. This routine may
                             change the pointer if a surrogate
                             IRP is allocated.

    DeviceObject           - Device object passed into IofCallDriver.

    IofCallDriverStackData - Pointer to a local variable on
                             IofCallDriver's stack to store data.
                             The stored information will be picked
                             up by IovpCallDriver2, and
                             may be adjusted at other times.


  Return Value:

     None.

--*/
{
    PIOV_REQUEST_PACKET iovPacket;
    PIOV_SESSION_DATA iovSessionData;
    PIOV_STACK_LOCATION iovCurrentStackLocation;
    PIRP irp, replacementIrp;
    PIO_STACK_LOCATION irpSp, irpLastSp;
    BOOLEAN isNewSession, isNewRequest, previouslyInUse, surrogateSpawned;
    ULONG isSameStack;
    ULONG locationsAdvanced, completeStyle;
    PDEVICE_OBJECT pdo, lowerDeviceObject;
    PDRIVER_OBJECT driverObject;
    PVOID dispatchRoutine;

    irp = *IrpPointer;
    irpSp = IoGetNextIrpStackLocation( irp );

    //
    // Preinitialize the CallStackData.
    //
    RtlZeroMemory(IofCallDriverStackData, sizeof(IOFCALLDRIVER_STACKDATA));

    //
    // If we are going to die shortly, kindly say so.
    //
    if (DeviceObject == NULL) {

        FAIL_CALLER_OF_IOFCALLDRIVER(
            (DCERROR_NULL_DEVOBJ_FORWARDED, DCPARAM_IRP, irp),
            irpSp
            );
    }

    //
    // The examined flag is set on any IRP that has come through
    // IofCallDriver. We use the flag to detect whether we have seen the IRP
    // before.
    //
    switch(irp->Flags&IRPFLAG_EXAMINE_MASK) {

        case IRPFLAG_EXAMINE_NOT_TRACKED:

            //
            // This packet is marked do not touch. So we ignore it.
            //
            iovPacket = NULL;
            break;

        case IRPFLAG_EXAMINE_TRACKED:

            //
            // This packet has been marked. We should find it.
            //
            iovPacket = IovpTrackingDataFindAndLock(irp);
            ASSERT(iovPacket != NULL);
            break;

        case IRPFLAG_EXAMINE_UNMARKED:

            iovPacket = IovpTrackingDataFindAndLock(irp);
            if (iovPacket) {

                //
                // Was tracked but cache flag got wiped. Replace.
                //
                irp->Flags |= IRPFLAG_EXAMINE_TRACKED;

            } else if (IovpTrackingFlags&ASSERTFLAG_TRACKIRPS) {

                //
                // Create the packet
                //
                iovPacket = IovpTrackingDataCreateAndLock(irp);
                if (iovPacket) {

                    //
                    // Mark it
                    //
                    irp->Flags |= IRPFLAG_EXAMINE_TRACKED;
                } else {

                    //
                    // No memory, try to keep it out of the IRP assert though.
                    //
                    irp->Flags |= IRPFLAG_EXAMINE_NOT_TRACKED;
                }
            } else {

                //
                // Do as told, don't track through IofCallDriver.
                //
                irp->Flags |= IRPFLAG_EXAMINE_NOT_TRACKED;
            }
            break;

        default:
            ASSERT(0);
            break;
    }

    if (iovPacket == NULL) {

        //
        // Nothing to track, get out.
        //
        return;
    }

    //
    // Find the current session. The session terminates when the final top-level
    // completion routine gets called.
    //
    iovSessionData = IovpTrackingDataGetCurrentSessionData(iovPacket);

    if (iovSessionData) {

        ASSERT(iovPacket->Flags&TRACKFLAG_ACTIVE);
        isNewSession = FALSE;

        IovpSessionDataAdvance(
            DeviceObject,
            iovSessionData,      // This param is optional.
            &iovPacket,
            &surrogateSpawned
            );

    } else if (!(iovPacket->Flags&TRACKFLAG_ACTIVE)){

        iovPacket->Flags |= TRACKFLAG_ACTIVE;
        isNewSession = TRUE;

        iovSessionData = IovpSessionDataCreate(
            DeviceObject,
            &iovPacket,
            &surrogateSpawned
            );

    } else {

        //
        // Might hit this path under low memory, or we are tracking allocations
        // but not the IRP sessions themselves.
        //
    }

    //
    // Let IovpCallDriver2 know what it's tracking...
    //
    IofCallDriverStackData->IovSessionData = iovSessionData;

    if (iovSessionData == NULL) {

        IovpTrackingDataReleaseLock(iovPacket) ;
        return;
    }

    if (surrogateSpawned) {

        //
        // iovPacket was changed to cover the surrogate IRP. Update our own
        // local variable and IofCallDriver's local variable appropriately.
        //
        irp = iovPacket->TrackedIrp;
        irpSp = IoGetNextIrpStackLocation(irp);
        *IrpPointer = irp;
    }

    if (isNewSession) {

        IovpTrackingDataReference(iovPacket, IOVREFTYPE_POINTER);
        IovpSessionDataReference(iovSessionData);
    }

    if (iovPacket->AssertFlags&ASSERTFLAG_POLICEIRPS) {

        //
        // If someone has given us an IRP with a cancel routine, beat them. Drivers
        // set cancel routines when they are going to be pending IRPs *themselves*
        // and should remove them before passing the IRP below. This is also true
        // as the driver will *not* call your cancel routine if he writes in his
        // own (which it may). Nor is the lower driver expected to put yours back
        // either...
        //
        if (irp->CancelRoutine) {

            FAIL_CALLER_OF_IOFCALLDRIVER(
                (DCERROR_CANCELROUTINE_FORWARDED, DCPARAM_IRP, irp),
                irpSp
                );

            irp->CancelRoutine = NULL;
        }
    }

    //
    // Now do any checking that requires tracking data.
    //
    if (iovPacket->Flags&TRACKFLAG_QUEUED_INTERNALLY) {

        //
        // We internally queue irps to catch bugs. When we are doing this, we
        // force the stack returned status to STATUS_PENDING, and we queue the
        // irp and release it on a timer. We also may make the IRP non-touchable.
        // This particular caller is trying to forward an IRP he doesn't own,
        // and we didn't actually end up with an untouchable irp.
        //
        FAIL_CALLER_OF_IOFCALLDRIVER(
            (DCERROR_QUEUED_IRP_FORWARDED, DCPARAM_IRP, irp),
            irpSp
            );
    }

    //
    // Figure out how many stack locations we've moved up since we've last seen
    // this IRP, and determine if the stack locations were copied appropriately.
    // We also need to see exactly how the IRP was forwarded (down the stack,
    // to another stack, straight to the PDO, etc).
    //
    // ADRIAO BUGBUG #07 05/11/98 - The only way to truely detect this is to
    //                              attach a filter at every layer in stack.
    //                              This is left as an exercise for later.
    //
    IovpExamineDevObjForwarding(
        DeviceObject,
        iovSessionData->DeviceLastCalled,
        &iovSessionData->ForwardMethod
        ) ;

    IovpExamineIrpStackForwarding(
        iovPacket,
        isNewSession,
        iovSessionData->ForwardMethod,
        DeviceObject,
        irp,
        &irpSp,
        &irpLastSp,
        &locationsAdvanced
        );

    TRACKIRP_DBGPRINT((
        "  CD1: Current, Last = (%x, %x)\n",
        irp->CurrentLocation,
        iovPacket->LastLocation
        ), 3) ;

    //
    // Figure out whether this is a new request or not, and record a
    // pointer in this slot to the requests originating slot as appropriate.
    //
    isNewRequest = IovpAssertIsNewRequest(irpLastSp, irpSp);

    //
    // Record information in our private stack locations and
    // write that back into the "stack" data itself...
    //
    previouslyInUse = IovpAdvanceStackDownwards(
        iovSessionData->StackData,
        irp->CurrentLocation,
        irpSp,
        irpLastSp,
        locationsAdvanced,
        isNewRequest,
        TRUE,
        &iovCurrentStackLocation
        );

    ASSERT(iovCurrentStackLocation);

    if (previouslyInUse) {

        ASSERT(!isNewRequest);
        ASSERT(!isNewSession);
        KeQuerySystemTime(&iovCurrentStackLocation->PerfDispatchStart) ;

    } else {

        IofCallDriverStackData->Flags = CALLFLAG_TOPMOST_IN_SLOT ;
        InitializeListHead(&IofCallDriverStackData->SharedLocationList) ;

        KeQuerySystemTime(&iovCurrentStackLocation->PerfDispatchStart) ;
        KeQuerySystemTime(&iovCurrentStackLocation->PerfStackLocationStart) ;

        //
        // Record the first thread this IRP slot was dispatched to.
        //
        iovCurrentStackLocation->ThreadDispatchedTo = PsGetCurrentThread();
        if (isNewRequest) {

            iovCurrentStackLocation->InitialStatusBlock = irp->IoStatus;
            iovCurrentStackLocation->LastStatusBlock = irp->IoStatus;
            if (isNewSession) {

                iovCurrentStackLocation->Flags |= STACKFLAG_FIRST_REQUEST;
            }
        }
    }

    //
    // Record whether this is the last device object for this IRP...
    // PDO's have devnodes filled out, so look for that field.
    // Actually, we can't quite do that trick as during Bus
    // enumeration a bus filter might be sending down Irps before
    // the OS has ever seen the node. So we assume a devobj is a
    // PDO if he has never attached to anyone.
    //
    lowerDeviceObject = IovpGetDeviceAttachedTo(DeviceObject) ;
    if (lowerDeviceObject) {
        ObDereferenceObject(lowerDeviceObject) ;
    } else {
        iovCurrentStackLocation->Flags |= STACKFLAG_REACHED_PDO ;
    }

    //
    // Record who is getting this IRP (we will blame any mistakes on him
    // if this request gets completed.) Note that we've already asserted
    // DeviceObject is non-NULL...
    //
    driverObject = DeviceObject->DriverObject ;
    dispatchRoutine = driverObject->MajorFunction[irpSp->MajorFunction] ;
    iovCurrentStackLocation->LastDispatch = dispatchRoutine ;

    //
    // Uncomplete the request if we are heading back down with it...
    //
    iovCurrentStackLocation->Flags &=~ STACKFLAG_REQUEST_COMPLETED ;

    //
    // This IofCallDriver2 dude will need to be told what his status should
    // be later. Add him to the linked list of addresses to scribble away
    // stati when the appropriate level is completed.
    //
    InsertHeadList(
        &iovCurrentStackLocation->CallStackData,
        &IofCallDriverStackData->SharedLocationList
        ) ;

    //
    // More IofCallDriver2 stuff, tell him the stack location.
    //
    IofCallDriverStackData->IovStackLocation = iovCurrentStackLocation ;

    // If it's a remove IRP, mark everyone appropriately
    if ((irpSp->MajorFunction == IRP_MJ_PNP)&&
        (irpSp->MinorFunction == IRP_MN_REMOVE_DEVICE)) {

        IofCallDriverStackData->Flags |= CALLFLAG_IS_REMOVE_IRP ;

        pdo = IovpGetLowestDevice(DeviceObject) ;
        ASSERT(pdo) ;
        IofCallDriverStackData->RemovePdo = pdo ;
        ObDereferenceObject(pdo) ;
        if (IovpIsInFdoStack(DeviceObject) &&
            (!(DeviceObject->DeviceObjectExtension->ExtensionFlags&DOE_RAW_FDO))) {
            IofCallDriverStackData->Flags |= CALLFLAG_REMOVING_FDO_STACK_DO ;
        }
    }

    if ((iovPacket->AssertFlags&ASSERTFLAG_POLICEIRPS) &&
        (iovPacket->AssertFlags&ASSERTFLAG_MONITORMAJORS)) {

        //
        // Do IRP-major specific assertions as appropriate
        //
        if (isNewSession) {

            IovpAssertNewIrps(iovPacket, irpSp, iovCurrentStackLocation) ;
        }

        if (isNewRequest) {

            IovpAssertNewRequest(iovPacket, DeviceObject, irpLastSp, irpSp, iovCurrentStackLocation) ;
        }

        IovpAssertIrpStackDownward(iovPacket, DeviceObject, irpLastSp, irpSp, iovCurrentStackLocation) ;
    }

    //
    // Update our fields
    //
    iovSessionData->DeviceLastCalled = DeviceObject ;
    iovPacket->LastLocation = irp->CurrentLocation ;
    iovCurrentStackLocation->RequestsFirstStackLocation->LastStatusBlock = irp->IoStatus;

    //
    // Dope the next stack location so we can detect usage of
    // IoCopyCurrentIrpStackLocationToNext or IoSetCompletionRoutine.
    //
    if (irp->CurrentLocation>1) {
        IoSetNextIrpStackLocation( irp ) ;
        irpSp = IoGetNextIrpStackLocation( irp );
        irpSp->Control |= SL_NOTCOPIED ;
        IoSkipCurrentIrpStackLocation( irp ) ;
    }

    //
    // Randomly set the cancel flag on a percentage of forwarded IRPs. Many
    // drivers queue first and after dequeue assume the cancel routine they
    // set must have been cleared if Cancel = TRUE. They don't handle the case
    // were the Irp was cancelled in flight.
    //
    // ADRIAO BUGBUG 07/16/1999 -
    //     Do better spontaneous cancel logic later.
    //
    if ((IovpInitFlags & IOVERIFIERINIT_RANDOMLY_CANCEL_IRPS) &&
        (!(irp->Flags & IRP_PAGING_IO))) {

        if (((++IovpCancelCount) % 4000) == 0) {

            irp->Cancel = TRUE;
        }
    }

    //
    // Assert LastLocation is consistent with an IRP that may be completed.
    //
    ASSERT(iovSessionData->StackData[iovPacket->LastLocation-1].InUse) ;

    IovpSessionDataReference(iovSessionData);
    IovpTrackingDataReleaseLock(iovPacket) ;
}

VOID
FASTCALL
IovpCallDriver2(
    IN     PIRP               Irp,
    IN     PDEVICE_OBJECT     DeviceObject,
    IN     PVOID              DispatchRoutine,
    IN OUT NTSTATUS           *FinalStatus,
    IN     PIOFCALLDRIVER_STACKDATA IofCallDriverStackData
    )
/*++

  Description:

    This routine is called by IofCallDriver just after the driver's dispatch
    routine has been called. The IRP may not be touchable at this time.

  Arguments:

    Irp                    - A pointer to the IRP passed into IofCallDriver.
                             The IRP may not be touchable right now.

    DispatchRoutine        - Dispatch routine that was called by IofCallDriver.

    FinalStatus            - A pointer to the status returned by the dispatch
                             routine. This may be changed if all IRPs are being
                             forced "pending".

    IofCallDriverStackData - Pointer to a local variable on IofCallDriver's
                             stack to retreive data stored by
                             IovpCallDriver1.

  Return Value:

     None.

--*/
{
    NTSTATUS status, lastStatus;
    PIOV_REQUEST_PACKET iovPacket;
    PIOV_SESSION_DATA iovSessionData;
    ULONG refCount;
    PIOV_STACK_LOCATION iovCurrentStackLocation;
    BOOLEAN mustDetachAndDelete;
    PDEVICE_NODE devNode;
    PDEVICE_OBJECT lowerDevObj;

    iovSessionData = IofCallDriverStackData->IovSessionData;
    if (iovSessionData == NULL) {

        return;
    }

    iovPacket = iovSessionData->IovRequestPacket;
    ASSERT(iovPacket);
    IovpTrackingDataAcquireLock(iovPacket);

    //
    // ADRIAO BUGBUG 08/10/1999 -
    //     This needs to be reenabled once the DNF_ flags are used in a
    // consistant manner.
    //
#if 0
    if (iovSessionData->AssertFlags&ASSERTFLAG_POLICEIRPS) {

        if (IofCallDriverStackData->Flags&CALLFLAG_IS_REMOVE_IRP) {

            if ((*FinalStatus != STATUS_PENDING) &&
                (iovCurrentStackLocation->ThreadDispatchedTo == PsGetCurrentThread())) {

                lowerDevObj = IovpGetDeviceAttachedTo(DeviceObject) ;

                //
                // We can look at this because the caller has committed to this being
                // completed now, and we are on the original thread.
                //
                // N.B. This works because all the objects in the stack have been
                // referenced during a remove. If we decide to only reference the
                // top object, this logic would break...
                //
                if (IofCallDriverStackData->Flags&CALLFLAG_REMOVING_FDO_STACK_DO) {

                    //
                    // FDO, Upper, & Lower filters *must* go. Note that lowerDevObj
                    // should be null as we should have detached.
                    //
                    mustDetachAndDelete = TRUE ;

                } else {

                    devNode = IofCallDriverStackData->RemovePdo->DeviceObjectExtension->DeviceNode ;
                    ASSERT(devNode) ;

                    if (devNode->Flags & DNF_DEVICE_GONE) {

                        //
                        // It's been reported as missing. It *must* go!
                        //
                        mustDetachAndDelete = TRUE ;

                    } else {

                        //
                        // It must stay!
                        //
                        mustDetachAndDelete = FALSE ;
                    }
                }

                if (mustDetachAndDelete) {

                    //
                    // IoDetachDevice and IoDeleteDevice should have been called.
                    // First verify IoDetachDevice...
                    //
                    if (lowerDevObj) {

                        WDM_FAIL_ROUTINE((
                            DCERROR_SHOULDVE_DETACHED,
                            DCPARAM_IRP + DCPARAM_ROUTINE + DCPARAM_DEVOBJ,
                            iovSessionData->BestVisibleIrp,
                            DispatchRoutine,
                            DeviceObject
                            ));
                    }

                    //
                    // Now verify IoDeleteDevice
                    //
                    if (!(DeviceObject->DeviceObjectExtension->ExtensionFlags&DOE_DELETE_PENDING)) {

                        WDM_FAIL_ROUTINE((
                            DCERROR_SHOULDVE_DELETED,
                            DCPARAM_IRP + DCPARAM_ROUTINE + DCPARAM_DEVOBJ,
                            iovSessionData->BestVisibleIrp,
                            DispatchRoutine,
                            DeviceObject
                            ));
                    }

                } else {

                    //
                    // Did we mistakenly leave? Verify we aren't a bus filter that
                    // has been fooled. In that case, no checking can be done...
                    //
                    ASSERT(!(IofCallDriverStackData->Flags&CALLFLAG_REMOVING_FDO_STACK_DO)) ;

                    if (DeviceObject == IofCallDriverStackData->RemovePdo) {

                        //
                        // Check PDO's - did we mistakenly delete ourselves?
                        //
                        if (DeviceObject->DeviceObjectExtension->ExtensionFlags&DOE_DELETE_PENDING) {

                            WDM_FAIL_ROUTINE((
                                DCERROR_DELETED_PRESENT_PDO,
                                DCPARAM_IRP + DCPARAM_ROUTINE + DCPARAM_DEVOBJ,
                                iovSessionData->BestVisibleIrp,
                                DispatchRoutine,
                                DeviceObject
                                ));
                        }

                    } else if (!(IofCallDriverStackData->RemovePdo->DeviceObjectExtension->ExtensionFlags&DOE_DELETE_PENDING)) {

                        //
                        // Check bus filters. Bus filters better not have detached
                        // or deleted themselves, as the PDO is still present!
                        //
                        if (lowerDevObj == NULL) {

                            //
                            // Oops, it detached. Baad bus filter...
                            //
                            WDM_FAIL_ROUTINE((
                                DCERROR_BUS_FILTER_ERRONEOUSLY_DETACHED,
                                DCPARAM_IRP + DCPARAM_ROUTINE + DCPARAM_DEVOBJ,
                                iovSessionData->BestVisibleIrp,
                                DispatchRoutine,
                                DeviceObject
                                ));
                        }

                        if (DeviceObject->DeviceObjectExtension->ExtensionFlags&DOE_DELETE_PENDING) {

                            //
                            // It deleted itself. Also very bad...
                            //
                            WDM_FAIL_ROUTINE((
                                DCERROR_BUS_FILTER_ERRONEOUSLY_DELETED,
                                DCPARAM_IRP + DCPARAM_ROUTINE + DCPARAM_DEVOBJ,
                                iovSessionData->BestVisibleIrp,
                                DispatchRoutine,
                                DeviceObject
                                ));
                        }
                    }
                }

                if (lowerDevObj) {

                    ObDereferenceObject(lowerDevObj) ;
                }
            }
        }
    }
#endif
    if (IofCallDriverStackData->Flags&CALLFLAG_COMPLETED) {

        TRACKIRP_DBGPRINT((
            "  Verifying status in CD2\n"
            ),2) ;

        if ((*FinalStatus != IofCallDriverStackData->ExpectedStatus)&&
            (*FinalStatus != STATUS_PENDING)) {

            if ((iovSessionData->AssertFlags&ASSERTFLAG_POLICEIRPS) &&
                (!(iovSessionData->SessionFlags&SESSIONFLAG_UNWOUND_INCONSISTANT))) {


                //
                // The completion routine and the return value don't match. Hey!
                //
                WDM_FAIL_ROUTINE((
                    DCERROR_INCONSISTANT_STATUS,
                    DCPARAM_IRP + DCPARAM_ROUTINE + DCPARAM_STATUS*2,
                    iovSessionData->BestVisibleIrp,
                    DispatchRoutine,
                    IofCallDriverStackData->ExpectedStatus,
                    *FinalStatus
                    ));
            }

            iovSessionData->SessionFlags |= SESSIONFLAG_UNWOUND_INCONSISTANT;

        } else if (*FinalStatus == 0xFFFFFFFF) {

            if (iovSessionData->AssertFlags&ASSERTFLAG_POLICEIRPS) {


                //
                // This status value is illegal. If we see it, we probably have
                // an uninitialized variable...
                //
                WDM_FAIL_ROUTINE((
                    DCERROR_UNINITIALIZED_STATUS,
                    DCPARAM_IRP + DCPARAM_ROUTINE,
                    iovSessionData->BestVisibleIrp,
                    DispatchRoutine
                    ));
            }
        }

        //
        // We do not need to remove ourselves from the list because
        // we will not be completed twice (InUse is NULL makes sure).
        //

    } else {

        //
        // OK, we haven't completed yet. Status better
        // be pending...
        //
        TRACKIRP_DBGPRINT((
            "  Verifying status is STATUS_PENDING in CR2\n"
            ), 2) ;

        if (*FinalStatus != STATUS_PENDING) {

            if ((iovSessionData->AssertFlags&ASSERTFLAG_POLICEIRPS) &&
                (!(iovPacket->Flags&TRACKFLAG_UNWOUND_BADLY))) {

                //
                // We got control before this slot was completed. This is
                // legal as long as STATUS_PENDING was returned (it was not),
                // so it's bug time. Note that the IRP may not be safe to touch.
                //
                WDM_FAIL_ROUTINE((
                    DCERROR_IRP_RETURNED_WITHOUT_COMPLETION,
                    DCPARAM_IRP + DCPARAM_ROUTINE,
                    iovSessionData->BestVisibleIrp,
                    DispatchRoutine
                    ));
            }

            iovPacket->Flags |= TRACKFLAG_UNWOUND_BADLY;
        }

        iovCurrentStackLocation = (PIOV_STACK_LOCATION)(IofCallDriverStackData->IovStackLocation) ;
        ASSERT(iovCurrentStackLocation->InUse) ;
        ASSERT(!IsListEmpty(&iovCurrentStackLocation->CallStackData)) ;

        //
        // We now extricate ourselves from the list.
        //
        RemoveEntryList(&IofCallDriverStackData->SharedLocationList) ;
    }

    if ((IofCallDriverStackData->Flags&CALLFLAG_OVERRIDE_STATUS)&&
        (*FinalStatus != STATUS_PENDING)) {

        *FinalStatus = IofCallDriverStackData->NewStatus ;
    }

    if ((iovSessionData->AssertFlags&ASSERTFLAG_FORCEPENDING) &&
        (!(IofCallDriverStackData->Flags&CALLFLAG_IS_REMOVE_IRP))) {

        //
        // We also have the option of causing trouble by making every Irp
        // look as if were pending.
        //
        *FinalStatus = STATUS_PENDING ;
    }

    IovpSessionDataDereference(iovSessionData);
    IovpTrackingDataReleaseLock(iovPacket);
}

VOID
FASTCALL
IovpCompleteRequest1(
    IN     PIRP               Irp,
    IN     CCHAR              PriorityBoost,
    IN OUT PIOFCOMPLETEREQUEST_STACKDATA CompletionPacket
    )
/*++

  Description

    This routine is called the moment IofCompleteRequest is invoked, and
    before any completion routines get called and before the IRP stack
    is adjusted in any way.

  Arguments:

    Irp                    - A pointer to the IRP passed into
                             IofCompleteRequest.

    PriorityBoost          - The priority boost passed into
                             IofCompleteRequest.

    CompletionPacket       - A pointer to a local variable on the stack of
                             IofCompleteRequest. The information stored in
                             this local variable will be picked up by
                             IovpCompleteRequest2-5.
  Return Value:

     None.
--*/
{
    PIOV_REQUEST_PACKET iovPacket;
    PIOV_SESSION_DATA iovSessionData;
    BOOLEAN slotIsInUse;
    PIOV_STACK_LOCATION iovCurrentStackLocation;
    ULONG locationsAdvanced;
    PIO_STACK_LOCATION irpSp;
    PDEVICE_OBJECT lowerDevobj;

    iovPacket = IovpTrackingDataFindAndLock(Irp);

    CompletionPacket->RaisedCount = 0;

    if (iovPacket == NULL) {

        CompletionPacket->IovSessionData = NULL;
        return;
    }

    iovSessionData = IovpTrackingDataGetCurrentSessionData(iovPacket);

    CompletionPacket->IovSessionData = iovSessionData;
    CompletionPacket->IovRequestPacket = iovPacket;

    if (iovSessionData == NULL) {

        //
        // We just got a look at the allocation, not the session itself.
        // This can happen if a driver calls IofCompleteRequest on an internally
        // generated IRP before calling IofCallDriver. NPFS does this.
        //
        IovpTrackingDataReleaseLock(iovPacket);
        return;
    }

    TRACKIRP_DBGPRINT((
        "  CR1: Current, Last = (%x, %x)\n",
        Irp->CurrentLocation, iovPacket->LastLocation
        ), 3);

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    if (iovPacket->Flags&TRACKFLAG_QUEUED_INTERNALLY) {

        //
        // We are probably going to die now. Anyway, it was a good life...
        //
        WDM_FAIL_CALLER3((DCERROR_QUEUED_IRP_COMPLETED, DCPARAM_IRP, Irp));
    }

    //
    // This would be *very* bad - someone is completing an IRP that is
    // currently in progress...
    //
    ASSERT(!(Irp->Flags&IRP_DIAG_HAS_SURROGATE));

    //
    // Hmmm, someone is completing an IRP that IoCallDriver never called. These
    // is possible but rather gross, so we warn.
    //
    if (Irp->CurrentLocation == ((CCHAR) Irp->StackCount + 1)) {

        WDM_CHASTISE_CALLER3((DCERROR_UNFORWARDED_IRP_COMPLETED, DCPARAM_IRP, Irp));
    }

    //
    // Record priority for our own later recompletion...
    //
    iovPacket->PriorityBoost = PriorityBoost;

    //
    // We have the option of causing trouble by making every Irp look
    // as if were pending. It is best to do it here, as this also takes
    // care of anybody who has synchronized the IRP and thus does not need
    // to mark it pending in his completion routine.
    //
    if (iovSessionData->AssertFlags&ASSERTFLAG_FORCEPENDING) {

        IoMarkIrpPending(Irp);
    }

    //
    // Do this so that if the IRP comes down again, it looks like a new one
    // to the "forward them correctly" code.
    //
    iovSessionData->DeviceLastCalled = NULL;

    locationsAdvanced = iovPacket->LastLocation - Irp->CurrentLocation;

    //
    // Remember this so that we can detect the case where someone is completing
    // to themselves.
    //
    CompletionPacket->LocationsAdvanced = locationsAdvanced;

    //
    // If this failed, somebody skipped then completed.
    //
    ASSERT(locationsAdvanced);

    //
    // If somebody called IoSetNextIrpStackLocation, and then completed,
    // update our internal stack locations (slots) as appropriate.
    //
    slotIsInUse = IovpAdvanceStackDownwards(
         iovSessionData->StackData,
         Irp->CurrentLocation,
         irpSp,
         irpSp + locationsAdvanced,
         locationsAdvanced,
         FALSE,
         FALSE,
         &iovCurrentStackLocation
         );

    IovpTrackingDataReleaseLock(iovPacket);
}

VOID
FASTCALL
IovpCompleteRequest2(
    IN     PIRP               Irp,
    IN OUT PIOFCOMPLETEREQUEST_STACKDATA CompletionPacket
    )
/*++

  Description:

    This routine is called for each stack location that might have a completion
    routine.

  Arguments:

    Irp                    - A pointer to the IRP passed into
                             IofCompleteRequest.

    CompletionPacket       - A pointer to a local variable on the stack of
                             IofCompleteRequest. The information stored in
                             this local variable will be picked up by
                             IovpCompleteRequest4&5.

  Return Value:

     None.
--*/
{
    PIOV_REQUEST_PACKET iovPacket;
    PIOV_SESSION_DATA iovSessionData;
    BOOLEAN raiseToDPC, newlyCompleted, requestFinalized ;
    KIRQL oldIrql ;
    PIOV_STACK_LOCATION iovCurrentStackLocation, requestsFirstStackLocation ;
    NTSTATUS status, entranceStatus ;
    PIOFCALLDRIVER_STACKDATA IofCallDriverStackData ;
    PIO_STACK_LOCATION irpSp ;
    ULONG refAction ;
    PLIST_ENTRY listEntry ;

    iovSessionData = CompletionPacket->IovSessionData;
    if (iovSessionData == NULL) {

        return;
    }

    iovPacket = CompletionPacket->IovRequestPacket;
    ASSERT(iovPacket);
    IovpTrackingDataAcquireLock(iovPacket);

    ASSERT(iovSessionData == IovpTrackingDataGetCurrentSessionData(iovPacket));

    ASSERT(!Irp->CancelRoutine) ;

    status = Irp->IoStatus.Status ;

    TRACKIRP_DBGPRINT((
        "  CR2: Current, Last = (%x, %x)\n",
        Irp->CurrentLocation, iovPacket->LastLocation
        ), 3) ;

    iovCurrentStackLocation = iovSessionData->StackData + Irp->CurrentLocation -1 ;
    TRACKIRP_DBGPRINT((
        "  Smacking %lx in CR2\n",
        iovCurrentStackLocation-iovSessionData->StackData
        ), 2) ;

    if (Irp->CurrentLocation <= iovPacket->TopStackLocation) {

        //
        // Might this be false if the completion routine is to an
        // internal stack loc as set up by IoSetNextIrpStackLocation?
        //
        ASSERT(iovCurrentStackLocation->InUse) ;

        //
        // Determine if a request was newly completed. Note that
        // several requests may exist within an IRP if it is being
        // "reused". For instance, in response to a IRP_MJ_READ, a
        // driver might convert it into a IRP_MJ_PNP request for the
        // rest of the stack. The two are treated as seperate requests.
        //
        requestsFirstStackLocation = iovCurrentStackLocation->RequestsFirstStackLocation ;
        TRACKIRP_DBGPRINT((
            "  CR2: original request for %lx is %lx\n",
            iovCurrentStackLocation-iovSessionData->StackData,
            requestsFirstStackLocation-iovSessionData->StackData
            ), 3) ;

        ASSERT(requestsFirstStackLocation) ;
        if (requestsFirstStackLocation->Flags&STACKFLAG_REQUEST_COMPLETED) {
            newlyCompleted = FALSE ;
        } else {
            requestsFirstStackLocation->Flags|=STACKFLAG_REQUEST_COMPLETED ;
            newlyCompleted = TRUE ;
            TRACKIRP_DBGPRINT((
                "  CR2: Request %lx newly completed by %lx\n",
                requestsFirstStackLocation-iovSessionData->StackData,
                iovCurrentStackLocation-iovSessionData->StackData
                ), 3) ;
        }
        requestFinalized = (iovCurrentStackLocation == requestsFirstStackLocation) ;
        if (requestFinalized) {

            TRACKIRP_DBGPRINT((
                "  CR2: Request %lx finalized\n",
                iovCurrentStackLocation-iovSessionData->StackData
                ), 3) ;
        }

        //
        // OK -
        //       If we haven't unwound yet, then IofCallDriverStackData will
        // start out non-NULL, in which case we will scribble away the final
        // completion routine status to everybody asking (could be multiple
        // if they IoSkip'd).
        //       On the other hand, everybody might have unwound, in which
        // case IofCallDriver(...) will start out NULL, and we will already have
        // asserted if STATUS_PENDING wasn't returned much much earlier...
        //       Finally, this slot may not have been "prepared" if an
        // internal stack location called IoSetNextIrpStackLocation, thus
        // consuming a stack location. In this case, IofCallDriverStackData
        // will come from a zero'd slot, and we will do nothing, which is
        // also fine.
        //
        irpSp = IoGetNextIrpStackLocation(Irp) ;

        if ((iovPacket->AssertFlags&ASSERTFLAG_POLICEIRPS) &&
            (iovPacket->AssertFlags&ASSERTFLAG_MONITORMAJORS)) {

            IovpAssertIrpStackUpward(
                iovPacket,
                irpSp,
                iovCurrentStackLocation,
                newlyCompleted,
                requestFinalized
                );
        }

        entranceStatus = status ;

        while(!IsListEmpty(&iovCurrentStackLocation->CallStackData)) {

            //
            // Pop off the list head.
            //
            listEntry = RemoveHeadList(&iovCurrentStackLocation->CallStackData) ;
            IofCallDriverStackData = CONTAINING_RECORD(
                listEntry,
                IOFCALLDRIVER_STACKDATA,
                SharedLocationList) ;

            ASSERT(!(IofCallDriverStackData->Flags&CALLFLAG_COMPLETED)) ;

            IofCallDriverStackData->Flags |= CALLFLAG_COMPLETED ;
            IofCallDriverStackData->ExpectedStatus = status ;

            if ((iovSessionData->AssertFlags&ASSERTFLAG_ROTATE_STATUS)&&
                 IovpAssertDoAdvanceStatus(irpSp, entranceStatus, &status)) {

                //
                // Purposely munge the returned status for everyone at this
                // layer to flush more bugs. We are specifically trolling for
                // this buggy sequence:
                //    Irp->IoStatus.Status = STATUS_SUCCESS ;
                //    IoSkipCurrentIrpStackLocation(Irp);
                //    IoCallDriver(DeviceBelow, Irp) ;
                //    return STATUS_SUCCESS ;
                //
                IofCallDriverStackData->Flags |= CALLFLAG_OVERRIDE_STATUS ;
                IofCallDriverStackData->NewStatus = status ;
            }
        }
        Irp->IoStatus.Status = status ;

        //
        // Set InUse = FALSE  and  CallStackData = NULL
        //
        RtlZeroMemory(iovCurrentStackLocation, sizeof(IOV_STACK_LOCATION)) ;
        InitializeListHead(&iovCurrentStackLocation->CallStackData) ;
    } else {

        ASSERT(0) ;
    }

    //
    // Once we return, we may be completed again before IofCompleteRequest3
    // get's called, so we make sure we are at DPC level throughout.
    //
    raiseToDPC = FALSE ;

    if (iovSessionData->AssertFlags&ASSERTFLAG_COMPLETEATDPC) {

        if (!CompletionPacket->RaisedCount) {

            //
            // Copy away the callers IRQL
            //
            CompletionPacket->PreviousIrql = iovPacket->CallerIrql;
            raiseToDPC = TRUE ;
        }
        CompletionPacket->RaisedCount++ ;
    }

    iovPacket->LastLocation = Irp->CurrentLocation+1 ;

    if (iovPacket->TopStackLocation == Irp->CurrentLocation) {

        CompletionPacket->IovSessionData = NULL;

        if (iovPacket->Flags&TRACKFLAG_SURROGATE) {

            //
            // Scribble away the real completion routine and corrosponding control
            //
            irpSp = IoGetNextIrpStackLocation(Irp) ;
            iovPacket->RealIrpCompletionRoutine = irpSp->CompletionRoutine ;
            iovPacket->RealIrpControl = irpSp->Control ;
            iovPacket->RealIrpContext = irpSp->Context ;

            //
            // We want to peek at the Irp prior to completion. This is why we
            // have expanded the initial number of stack locations with the
            // driver verifier enabled.
            //
            IoSetCompletionRoutine(
                Irp,
                IovpSwapSurrogateIrp,
                Irp,
                TRUE,
                TRUE,
                TRUE
                ) ;

        } else {

            //
            // Close this session as the IRP has entirely completed. We drop
            // the pointer count we added to the tracking data here for the
            // same reason.
            //
            irpSp = IoGetNextIrpStackLocation(Irp) ;
            if (iovPacket->AssertFlags&ASSERTFLAG_POLICEIRPS) {

                IovpAssertFinalIrpStack(iovPacket, irpSp) ;
            }

            ASSERT(iovPacket->TopStackLocation == Irp->CurrentLocation);
            IovpSessionDataClose(iovSessionData);
            IovpSessionDataDereference(iovSessionData);
            IovpTrackingDataDereference(iovPacket, IOVREFTYPE_POINTER);
        }

    } else {

        //
        // We will be seeing this IRP again. Hold a session count against it.
        //
        IovpSessionDataReference(iovSessionData);
    }

    //
    // Assert LastLocation is consistent with an IRP that may be completed.
    //
    if (iovPacket->LastLocation < iovPacket->TopStackLocation) {

        ASSERT(iovSessionData->StackData[iovPacket->LastLocation-1].InUse) ;
    }

    IovpTrackingDataReleaseLock(iovPacket);

    if (raiseToDPC) {
        KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);
    }

    CompletionPacket->LocationsAdvanced --;
}

VOID
FASTCALL
IovpCompleteRequest3(
    IN     PIRP               Irp,
    IN     PVOID              Routine,
    IN OUT PIOFCOMPLETEREQUEST_STACKDATA CompletionPacket
    )
/*++

  Description:

    This routine is called just before each completion routine is invoked.

  Arguments:

    Irp                    - A pointer to the IRP passed into
                             IofCompleteRequest.

    Routine                - The completion routine about to be called.

    CompletionPacket       - A pointer to data on the callers stack. This will
                             be picked up IovpCompleteRequest4 and
                             IovpCompleteRequest5.

  Return Value:

     None.
--*/
{
    PIOV_REQUEST_PACKET iovPacket;
    PIOV_SESSION_DATA iovSessionData;
    PIO_STACK_LOCATION irpSpCur, irpSpNext ;
    PDEFERRAL_CONTEXT deferralContext ;

    iovSessionData = CompletionPacket->IovSessionData;
    if (iovSessionData == NULL) {

        return;
    }

    iovPacket = iovSessionData->IovRequestPacket;
    ASSERT(iovPacket);
    IovpTrackingDataAcquireLock(iovPacket);

    //
    // Verify all completion routines are in nonpaged code, exempting one
    // special case - when a driver completes the IRP to itself by calling
    // IoSetNextStackLocation before calling IoCompleteRequest.
    //
    if (iovSessionData->AssertFlags&ASSERTFLAG_POLICEIRPS) {

        if ((CompletionPacket->LocationsAdvanced <= 0) &&
            (MmIsSystemAddressLocked(Routine) == FALSE)) {

            DbgPrint(
                "Verifier Notes: LocationsAdvanced %d\n",
                CompletionPacket->LocationsAdvanced
                );

            WDM_FAIL_ROUTINE((
                DCERROR_COMPLETION_ROUTINE_PAGABLE,
                DCPARAM_IRP + DCPARAM_ROUTINE,
                Irp,
                Routine
                ));
        }
    }

    //
    // Setup fields for those assertion functions that will be called *after*
    // the completion routine has been called.
    //
    irpSpCur = IoGetCurrentIrpStackLocation(Irp) ;
    CompletionPacket->IsRemoveIrp =
       ((Irp->CurrentLocation <= (CCHAR) Irp->StackCount) &&
        (irpSpCur->MajorFunction == IRP_MJ_PNP) &&
        (irpSpCur->MinorFunction == IRP_MN_REMOVE_DEVICE)) ;

    CompletionPacket->CompletionRoutine = Routine ;

    //
    // Is this a completion routine that should be called later? Note that this
    // is only legal if we are pending the IRPs (because to the upper driver,
    // IofCallDriver is returning before it's completion routine has been called)
    //
    if ((!CompletionPacket->IsRemoveIrp)&&
       ((iovSessionData->AssertFlags&ASSERTFLAG_DEFERCOMPLETION)||
        (iovSessionData->AssertFlags&ASSERTFLAG_COMPLETEATPASSIVE))) {

        ASSERT(iovSessionData->AssertFlags&ASSERTFLAG_FORCEPENDING) ;

        irpSpNext = IoGetNextIrpStackLocation(Irp) ;

        deferralContext = ExAllocatePoolWithTag(
           NonPagedPool,
           sizeof(DEFERRAL_CONTEXT),
           POOL_TAG_DEFERRED_CONTEXT
           ) ;

        if (deferralContext) {

            //
            // Swap the original completion and context for our own.
            //
            deferralContext->IovRequestPacket          = iovPacket;
            deferralContext->IrpSpNext                 = irpSpNext;
            deferralContext->OriginalCompletionRoutine = irpSpNext->CompletionRoutine;
            deferralContext->OriginalContext           = irpSpNext->Context;
            deferralContext->OriginalIrp               = Irp;
            deferralContext->OriginalPriorityBoost     = iovPacket->PriorityBoost;

            irpSpNext->CompletionRoutine = IovpInternalDeferredCompletion;
            irpSpNext->Context           = deferralContext;
            IovpTrackingDataReference(iovPacket, IOVREFTYPE_POINTER);
        }
    }

    IovpTrackingDataReleaseLock(iovPacket) ;
}

VOID
FASTCALL
IovpCompleteRequest4(
    IN     PIRP               Irp,
    IN     NTSTATUS           ReturnedStatus,
    IN OUT PIOFCOMPLETEREQUEST_STACKDATA CompletionPacket
    )
/*++

  Description:

    This assert routine is called just after each completion routine is
    invoked (but not if STATUS_MORE_PROCESSING is returned)

  Arguments:

    Irp                    - A pointer to the IRP passed into
                             IofCompleteRequest.

    Routine                - The completion routine called.

    ReturnedStatus         - The status value returned.

    CompletionPacket       - A pointer to data on the callers stack. This was
                             filled in by IovpCompleteRequest3.

  Return Value:

     None.
--*/
{
    PIOV_REQUEST_PACKET iovPacket;
    PIOV_SESSION_DATA iovSessionData;
    PIO_STACK_LOCATION irpSp;
    PVOID routine;

    routine = CompletionPacket->CompletionRoutine;
    iovSessionData = CompletionPacket->IovSessionData;

    if (iovSessionData == NULL) {

        return;
    }

    iovPacket = iovSessionData->IovRequestPacket;
    ASSERT(iovPacket);
    IovpTrackingDataAcquireLock(iovPacket);

    //
    // ADRIAO BUGBUG 01/06/1999 -
    //     Check for leaked Cancel routines here.
    //
    if (iovSessionData->AssertFlags&ASSERTFLAG_FORCEPENDING) {

        //
        // ADRIAO BUGBUG #05 05/12/98 - Find a way to do this in the non-pend
        //                              everything path...
        //
        if ((ReturnedStatus != STATUS_MORE_PROCESSING_REQUIRED)&&
            (iovPacket->pIovSessionData == iovSessionData)) {

            //
            // At this point, we know the completion routine is required to have
            // set the IRP pending bit, because we've hardwired everyone below
            // him to return pending, and we've marked the pending returned bit.
            // Verify he did his part
            //
            irpSp = IoGetCurrentIrpStackLocation(Irp) ;
            if (!(irpSp->Control & SL_PENDING_RETURNED )) {

                 WDM_FAIL_ROUTINE((
                     DCERROR_PENDING_BIT_NOT_MIGRATED,
                     DCPARAM_IRP + DCPARAM_ROUTINE,
                     Irp,
                     routine
                     ));

                 //
                 // This will keep the IRP above from erroneously asserting (and
                 // correctly hanging).
                 //
                 IoMarkIrpPending(Irp);
            }
        }
    }
    IovpTrackingDataReleaseLock(iovPacket);
}

VOID
FASTCALL
IovpCompleteRequest5(
    IN     PIRP                          Irp,
    IN OUT PIOFCOMPLETEREQUEST_STACKDATA CompletionPacket
    )
/*++

  Description:

    This routine is called for each stack location that could have had a
    completion routine, after any possible completion routine has been
    called.

  Arguments:

    Irp                    - A pointer to the IRP passed into
                             IofCompleteRequest.

    CompletionPacket       - A pointer to a local variable on the stack of
                             IofCompleteRequest. This information was stored
                             by IovpCompleteRequest2 and 3.

  Return Value:

     None.
--*/
{
    PIOV_REQUEST_PACKET iovPacket;
    PIOV_SESSION_DATA iovSessionData;
    PIOV_STACK_LOCATION iovCurrentStackLocation ;
    NTSTATUS status ;

    iovSessionData = CompletionPacket->IovSessionData;

    if (iovSessionData) {

        iovPacket = iovSessionData->IovRequestPacket;
        ASSERT(iovPacket);
        IovpTrackingDataAcquireLock(iovPacket);

        ASSERT((!CompletionPacket->RaisedCount) ||
               (iovSessionData->AssertFlags&ASSERTFLAG_COMPLETEATDPC)) ;

        IovpSessionDataDereference(iovSessionData);
        IovpTrackingDataReleaseLock(iovPacket);
    }

    //
    // When this count is at zero, we have unnested out of every
    // completion routine, so it is OK to return back to our original IRQL
    //
    if (CompletionPacket->RaisedCount) {

        if (!(--CompletionPacket->RaisedCount)) {
            //
            // Undo IRQL madness (wouldn't want to return to
            // the caller at DPC, would we now?)
            //
            KeLowerIrql(CompletionPacket->PreviousIrql);
        }
    }
}

VOID
FASTCALL
IovpCompleteRequestApc(
    IN     PIRP                          Irp,
    IN     PVOID                         BestStackOffset
    )
/*++

  Description:

    This routine is after the APC for completing IRPs and fired.

  Arguments:

    Irp                    - A pointer to the IRP passed into retrieved from
                             the APC in IopCompleteRequest.

    BestStackOffset        - A pointer to a last parameter passed on the stack.
                             We use this to detect the case where a driver has
                             ignored STATUS_PENDING and left the UserIosb on
                             it's stack.

  Return Value:

     None.
--*/
{
#if DBG
#if defined(_X86_)
    PUCHAR addr;
    PIOV_REQUEST_PACKET iovPacket;

    addr = (PUCHAR)Irp->UserIosb;
    if ((addr > (PUCHAR)KeGetCurrentThread()->StackLimit) &&
        (addr <= (PUCHAR)BestStackOffset)) {

        iovPacket = IovpTrackingDataFindAndLock(Irp) ;

        RtlAssert("UserIosb below stack pointer", __FILE__, (ULONG) iovPacket,
                  "Call AdriaO");

        IovpTrackingDataReleaseLock(iovPacket) ;
    }

    addr = (PUCHAR)Irp->UserEvent;
    if ((addr > (PUCHAR)KeGetCurrentThread()->StackLimit) &&
        (addr <= (PUCHAR)BestStackOffset)) {

        iovPacket = IovpTrackingDataFindAndLock(Irp) ;

        RtlAssert("UserEvent below stack pointer", __FILE__, (ULONG) iovPacket,
                  "Call AdriaO");

        IovpTrackingDataReleaseLock(iovPacket) ;
    }
#endif
#endif
}

BOOLEAN
IovpAdvanceStackDownwards(
    IN  PIOV_STACK_LOCATION   StackDataArray,
    IN  CCHAR                 CurrentLocation,
    IN  PIO_STACK_LOCATION    IrpSp,
    IN  PIO_STACK_LOCATION    IrpLastSp OPTIONAL,
    IN  ULONG                 LocationsAdvanced,
    IN  BOOLEAN               IsNewRequest,
    IN  BOOLEAN               MarkAsTaken,
    OUT PIOV_STACK_LOCATION   *StackLocationInfo
    )
{
    PIOV_STACK_LOCATION  iovCurrentStackLocation, advancedLocationData, requestOriginalSLD;
    PIO_STACK_LOCATION   irpSpTemp;
    PLARGE_INTEGER       dispatchTime, stackTime;
    BOOLEAN              isNewSession, wasInUse;
    PVOID                dispatchRoutine;

    isNewSession = (IrpLastSp == NULL);
    ASSERT((!isNewSession) || (LocationsAdvanced == 1));
    ASSERT(isNewSession || ((ULONG) (IrpLastSp - IrpSp) == LocationsAdvanced));

    //
    // The CurrentLocation will be decremented when we leave. If it hit's zero
    // we will bugcheck, therefore it should be at least two, and we'd need to
    // subtract two off it to make it an index into an array of slot locations
    // (our analog for stack locations). However we reserve an extra empty slot
    // at the head of the array to make our logic easier. Therefore we subtract
    // only one.
    //
    iovCurrentStackLocation = StackDataArray + CurrentLocation -1;

    TRACKIRP_DBGPRINT((
        "  Smacking %lx (%lx) to valid in SD\n",
        CurrentLocation -1, iovCurrentStackLocation
        ), 2);

    //
    // Note that we do set the InUse field. That's for the caller to do.
    //
    if (iovCurrentStackLocation->InUse) {

        //
        // The only way the stack slot could be in use is if we skipped before
        //
        ASSERT(!LocationsAdvanced); // && (!isNewSession)
        ASSERT(IrpSp == iovCurrentStackLocation->IrpSp);

    } else if (MarkAsTaken) {

        //
        // ADRIAO BUGBUG 01/02/1999 -
        //     Is the below assertion is not true in the case of an internally
        // forwarded, completed, and then externally forwarded IRP?
        //
        ASSERT(LocationsAdvanced); // || isNewSession
        RtlZeroMemory(iovCurrentStackLocation, sizeof(IOV_STACK_LOCATION));
        InitializeListHead(&iovCurrentStackLocation->CallStackData);
        iovCurrentStackLocation->IrpSp = IrpSp;
    }

    //
    // Determine the last original request. A "Request" is block of data in a
    // stack location that is progressively copied downwards as the IRP is
    // forwarded (ie, a forwarded START IRP, a forwarded IOCTL, etc). A clever
    // driver writer could use his own stack location to send down a quick
    // query before forwarding along the original request. We correctly
    // differentiate between those two unique requests within the IRP below.
    //
    if (isNewSession) {

        //
        // *We* are the original request. None of these fields below should
        // be used.
        //
        dispatchRoutine = NULL;
        requestOriginalSLD = NULL;
        stackTime = NULL;
        dispatchTime = NULL;

    } else if (LocationsAdvanced) {

        //
        // To get the original request (the pointer to the Irp slot that
        // represents where we *first* saw this request), we go backwards to get
        // the most recent previous irp slot data (set up when the device above
        // forwarded this Irp to us), and we read what it's original request was.
        // We also get the dispatch routine for that slot, which we will use to
        // backfill skipped slots if we advanced more than one Irp stack
        // location this time (ie, someone called IoSetNextIrpStackLocation).
        //
        dispatchTime       = &iovCurrentStackLocation[LocationsAdvanced].PerfDispatchStart;
        stackTime          = &iovCurrentStackLocation[LocationsAdvanced].PerfStackLocationStart;
        dispatchRoutine    = iovCurrentStackLocation[LocationsAdvanced].LastDispatch;
        requestOriginalSLD = iovCurrentStackLocation[LocationsAdvanced].RequestsFirstStackLocation;

        ASSERT(dispatchRoutine);
        ASSERT(iovCurrentStackLocation[LocationsAdvanced].InUse);
        ASSERT(requestOriginalSLD->RequestsFirstStackLocation == requestOriginalSLD);
        iovCurrentStackLocation->RequestsFirstStackLocation = requestOriginalSLD;

    } else {

        //
        // We skipped. The slot should already be filled.
        //
        dispatchRoutine = NULL;
        dispatchTime = NULL;
        stackTime = NULL;
        requestOriginalSLD = iovCurrentStackLocation->RequestsFirstStackLocation;
        ASSERT(requestOriginalSLD);
        ASSERT(requestOriginalSLD->RequestsFirstStackLocation == requestOriginalSLD);
    }

    //
    // The previous request seen is in requestOriginalSLD (NULL if none). If
    // we advanced more than one stack location (ie, someone called
    // IoSetNextIrpStackLocation), we need to update the slots we never saw get
    // consumed. Note that the dispatch routine we set in the slot is for the
    // driver that owned the last slot - we do not use the device object at
    // that IrpSp because it might be stale (or perhaps even NULL).
    //
    advancedLocationData = iovCurrentStackLocation;
    irpSpTemp = IrpSp;
    while(LocationsAdvanced>1) {
        advancedLocationData++ ;
        LocationsAdvanced-- ;
        irpSpTemp++ ;
        TRACKIRP_DBGPRINT((
            "  Late smacking %lx to valid in CD1\n",
            advancedLocationData - StackDataArray
            ), 3) ;

        ASSERT(!advancedLocationData->InUse) ;
        RtlZeroMemory(advancedLocationData, sizeof(IOV_STACK_LOCATION)) ;
        InitializeListHead(&advancedLocationData->CallStackData) ;
        advancedLocationData->InUse = TRUE ;
        advancedLocationData->IrpSp = irpSpTemp ;

        advancedLocationData->RequestsFirstStackLocation = requestOriginalSLD ;
        advancedLocationData->PerfDispatchStart = *dispatchTime;
        advancedLocationData->PerfStackLocationStart = *stackTime;
        advancedLocationData->LastDispatch = dispatchRoutine ;
    }

    //
    // For the assertion below...
    //
    if (LocationsAdvanced) {
        irpSpTemp++ ;
    }
    ASSERT((irpSpTemp == IrpLastSp)||(IrpLastSp == NULL)) ;

    //
    // Write out the slot we're using.
    //
    *StackLocationInfo = iovCurrentStackLocation;

    if (!MarkAsTaken) {
        return iovCurrentStackLocation->InUse;
    }

    //
    // Record a pointer in this slot to the requests originating slot as
    // appropriate.
    //
    if (IsNewRequest) {

        TRACKIRP_DBGPRINT((
            "  CD1: %lx is a new request\n",
            advancedLocationData-StackDataArray
            ), 3) ;

        ASSERT(LocationsAdvanced == 1) ;
        //
        // ADRIAO BUGBUG 01/02/1999 -
        //
        //     Why the **ll did I have this there? If this were correct then the
        // backfill logic above would need fixing. I think what I have now is
        // correct, not this:
        //
        // advancedLocationData->RequestsFirstStackLocation = advancedLocationData ;
        //
        iovCurrentStackLocation->RequestsFirstStackLocation = iovCurrentStackLocation;

    } else if (LocationsAdvanced) {

        ASSERT(!isNewSession) ;
        //
        // ADRIAO BUGBUG 01/02/1999 -
        //     As per above, this should already have been handled.
        //
        //advancedLocationData->RequestsFirstStackLocation = requestOriginalSLD ;
        TRACKIRP_DBGPRINT((
            "  CD1: %lx is a request for %lx\n",
            advancedLocationData-StackDataArray,
            requestOriginalSLD-StackDataArray
            ), 3) ;

    } else {

        //
        // As we skipped, the request should not have changed. If it did,
        // either guy we called trashed the stack given to him (giving none
        // to the dude under him), or we incorrectly saw a new request when
        // we shouldn't have (see previous comment).
        //
        ASSERT(!isNewSession) ;
        ASSERT(advancedLocationData->RequestsFirstStackLocation == requestOriginalSLD) ;
    }

    wasInUse = iovCurrentStackLocation->InUse;
    iovCurrentStackLocation->InUse = TRUE;
    return wasInUse;
}

VOID
IovpExamineIrpStackForwarding(
    IN OUT  PIOV_REQUEST_PACKET  IovPacket,
    IN      BOOLEAN              IsNewSession,
    IN      ULONG                ForwardMethod,
    IN      PDEVICE_OBJECT       DeviceObject,
    IN      PIRP                 Irp,
    IN OUT  PIO_STACK_LOCATION  *IoCurrentStackLocation,
    OUT     PIO_STACK_LOCATION  *IoLastStackLocation,
    OUT     ULONG               *StackLocationsAdvanced
    )
{
    PIRP irp;
    PIO_STACK_LOCATION irpSp, irpLastSp;
    BOOLEAN isSameStack;
    ULONG locationsAdvanced;

    irpSp = *IoCurrentStackLocation;

    if (!IsNewSession) {

        //
        // We are sitting on current next being one back (-1) from
        // CurrentStackLocation.
        //
        locationsAdvanced = IovPacket->LastLocation-Irp->CurrentLocation ;
        irpLastSp = Irp->Tail.Overlay.CurrentStackLocation+(locationsAdvanced-1) ;

    } else {

        //
        // New IRP, so no last SP and we always advance "1"
        //
        locationsAdvanced = 1 ;
        irpLastSp = NULL ;
    }

    if ((!IsNewSession) && (IovPacket->AssertFlags&ASSERTFLAG_POLICEIRPS)) {

        //
        // As the control field is zeroed by IoCopyCurrentStackLocation, we
        // dope each stack location with the value SL_NOTCOPIED. If it is
        // zeroed or the IRP stack location has stayed the same, the one of
        // the two API's was called. Otherwise the next stack location wasn't
        // set up properly (I have yet to find a case otherwise)...
        //
        if ((irpSp->Control&SL_NOTCOPIED)&&
            IovPacket->LastLocation != Irp->CurrentLocation) {

#if 0
            FAIL_CALLER_OF_IOFCALLDRIVER2(
                (DCERROR_NEXTIRPSP_DIRTY, DCPARAM_IRP, Irp),
                irpSp
                );
#endif
        }

        //
        // Now check for people who copy the stack locations and forget to
        // wipe out previous completion routines.
        //
        if (locationsAdvanced) {

            //
            // IoCopyCurrentStackLocation copies everything but Completion,
            // Context, and Control
            //
            isSameStack = RtlEqualMemory(irpSp, irpLastSp,
                FIELD_OFFSET(IO_STACK_LOCATION, Control)) ;

            isSameStack &= RtlEqualMemory(&irpSp->Parameters, &irpLastSp->Parameters,
                FIELD_OFFSET(IO_STACK_LOCATION, DeviceObject)-
                FIELD_OFFSET(IO_STACK_LOCATION, Parameters)) ;

            isSameStack &= (irpSp->FileObject == irpLastSp->FileObject) ;

            //
            // We should *never* see this on the stack! If we do, something
            // quite bizarre has happened...
            //
            ASSERT(irpSp->CompletionRoutine != IovpSwapSurrogateIrp) ;

            if (isSameStack) {

                //
                // We caught them doing something either very bad or quite
                // inefficient. We can tell which based on whether there is
                // a completion routine.
                //
                if ((irpSp->CompletionRoutine == irpLastSp->CompletionRoutine)&&
                    (irpSp->Context == irpLastSp->Context) &&
                    (irpSp->Control == irpLastSp->Control) &&
                    (irpSp->CompletionRoutine != NULL) &&
                    (DeviceObject->DriverObject != irpLastSp->DeviceObject->DriverObject)
                    ) {

                    //
                    // Duplication of both the completion and the context
                    // while not properly zeroing the control field is enough
                    // to make me believe the caller has made a vexing mistake.
                    //
                    FAIL_CALLER_OF_IOFCALLDRIVER2(
                        (DCERROR_IRPSP_COPIED, DCPARAM_IRP, Irp),
                        irpSp
                        ) ;

                    //
                    // Repair the stack
                    //
                    irpSp->CompletionRoutine = NULL ;
                    irpSp->Control = 0 ;

                } else if (!irpSp->CompletionRoutine) {

                    if (!(irpSp->Control&SL_NOTCOPIED)
#ifdef HACKHACKS_ENABLED
                        && (!(IovpHackFlags&HACKFLAG_FOR_SCSIPORT))
#endif
                        ) {

                        //
                        // ADRIAO HACKHACK 06/12/98 #10 - PeterWie does this for
                        // two reasons:
                        // 1) It's easier to debug
                        // 2) The space is not really recovered anyway.
                        //
                        // This will be an ongoing argument it seems, but #2 can
                        // be cleverly solved if one decrements their stack
                        // count, and #1 I've solved in Debug...
                        //
                        if (irpSp->MajorFunction == IRP_MJ_POWER) {

                            //
                            // Unwind back past PoCallDriver...
                            //
                            WDM_CHASTISE_CALLER5(
                                (DCERROR_UNNECCESSARY_COPY, DCPARAM_IRP, Irp)
                                );

                        } else {

                            WDM_CHASTISE_CALLER3(
                                (DCERROR_UNNECCESSARY_COPY, DCPARAM_IRP, Irp)
                                );
                        }
                    }

                    IoSetCompletionRoutine(
                        Irp,
                        IovpInternalCompletionTrap,
                        IoGetCurrentIrpStackLocation( Irp ),
                        TRUE,
                        TRUE,
                        TRUE
                        ) ;
                }
            }

        } else if (IovPacket->AssertFlags&ASSERTFLAG_CONSUME_ALWAYS) {

            if (ForwardMethod == FORWARDED_TO_NEXT_DO) {

                if (Irp->CurrentLocation<2) {

                    FAIL_CALLER_OF_IOFCALLDRIVER2(
                        (DCERROR_INSUFFICIENT_STACK_LOCATIONS, DCPARAM_IRP, Irp),
                        irpSp
                        ) ;

                } else {

                    //
                    // Back up the skip, then copy. Add a completion routine with
                    // unique and assertable context to catch people who clumsily
                    // Rtl-copy stack locations (we can't catch them if the caller
                    // above used an empty stack with no completion routine)...
                    //
                    IoSetNextIrpStackLocation( Irp ) ;

                    //
                    // Set the trap...
                    //
                    IoCopyCurrentIrpStackLocationToNext( Irp ) ;
                    IoSetCompletionRoutine(
                        Irp,
                        IovpInternalCompletionTrap,
                        IoGetCurrentIrpStackLocation( Irp ),
                        TRUE,
                        TRUE,
                        TRUE
                        ) ;

                    //
                    // This is our new reality...
                    //
                    locationsAdvanced = 1 ;
                    irpSp = IoGetNextIrpStackLocation( Irp );
                }
            }
        }
    }

    *IoCurrentStackLocation = irpSp;
    *IoLastStackLocation = irpLastSp;
    *StackLocationsAdvanced = locationsAdvanced;
}

NTSTATUS
IovpInternalCompletionTrap(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

  Description:

    This routine does nothing but act as a trap for people
    incorrectly copying stack locations...

  Arguments:

    DeviceObject           - Device object set at this level of the completion
                             routine - ignored.

    Irp                    - A pointer to the IRP.

    Context                - Context should equal the Irp's stack location -
                             this is asserted.

  Return Value:

     STATUS_SUCCESS

--*/
{
    PIO_STACK_LOCATION irpSp ;

    if (Irp->PendingReturned) {

        IoMarkIrpPending( Irp ) ;
    }
    irpSp = IoGetCurrentIrpStackLocation( Irp ) ;

    ASSERT((PVOID) irpSp == Context) ;

    return STATUS_SUCCESS ;
}

VOID
IovpInternalCompleteAtDPC(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
    IovpInternalCompleteAfterWait(DeferredContext) ;
}

VOID
IovpInternalCompleteAfterWait(
    IN PVOID Context
    )
{
    PDEFERRAL_CONTEXT deferralContext = (PDEFERRAL_CONTEXT) Context ;
    PIO_STACK_LOCATION irpSpNext ;
    NTSTATUS status ;

    if (deferralContext->DeferAction == DEFERACTION_QUEUE_PASSIVE_TIMER) {

        //
        // Wait the appropriate amount of time if so ordered...
        //
        ASSERT(KeGetCurrentIrql()==PASSIVE_LEVEL) ;
        KeWaitForSingleObject(
            &deferralContext->DeferralTimer,
            Executive,
            KernelMode,
            FALSE,
            NULL
            ) ;
    }

    IovpTrackingDataAcquireLock(deferralContext->IovRequestPacket) ;

    IovpProtectedIrpMakeTouchable(
        deferralContext->OriginalIrp,
        &deferralContext->IovRequestPacket->RestoreHandle
        );

    irpSpNext = IoGetNextIrpStackLocation( deferralContext->OriginalIrp ) ;

    ASSERT(irpSpNext == deferralContext->IrpSpNext) ;
    ASSERT(irpSpNext->CompletionRoutine == deferralContext->OriginalCompletionRoutine) ;
    ASSERT(irpSpNext->Context == deferralContext->OriginalContext) ;

    ASSERT(deferralContext->IovRequestPacket->Flags & TRACKFLAG_QUEUED_INTERNALLY) ;
    deferralContext->IovRequestPacket->Flags &=~ TRACKFLAG_QUEUED_INTERNALLY ;

    IovpTrackingDataDereference(deferralContext->IovRequestPacket, IOVREFTYPE_POINTER) ;
    IovpTrackingDataReleaseLock(deferralContext->IovRequestPacket) ;

    status = irpSpNext->CompletionRoutine(
        deferralContext->DeviceObject,
        deferralContext->OriginalIrp,
        irpSpNext->Context
        ) ;

    if (status!=STATUS_MORE_PROCESSING_REQUIRED) {

        IoCompleteRequest(deferralContext->OriginalIrp, deferralContext->OriginalPriorityBoost) ;
    }
    ExFreePool(deferralContext) ;
}

NTSTATUS
IovpInternalDeferredCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

  Description:

    This function is slipped in as a completion routine when we are
    "deferring" completion via work item, etc.

  Arguments:

    DeviceObject           - Device object set at this level of the completion
                             routine - passed on.

    Irp                    - A pointer to the IRP.

    Context                - Context block that includes original completion
                             routine.

  Return Value:

     NTSTATUS

--*/
{
    PDEFERRAL_CONTEXT deferralContext = (PDEFERRAL_CONTEXT) Context;
    PIO_STACK_LOCATION irpSpNext;
    BOOLEAN passiveCompletionOK;
    DEFER_ACTION deferAction;
    ULONG refAction;
    ULONG trackingFlags;
    LARGE_INTEGER deltaTime;

    //
    // Do delta time conversion.
    //
    deltaTime.QuadPart = - IovpIrpDeferralTime ;

    //
    // The *next* stack location holds our completion and context. The current
    // stack location has already been wiped.
    //
    irpSpNext = IoGetNextIrpStackLocation( Irp ) ;

    ASSERT((PVOID) irpSpNext->CompletionRoutine == IovpInternalDeferredCompletion) ;

    //
    // Put everything back in case someone is looking...
    //
    irpSpNext->CompletionRoutine = deferralContext->OriginalCompletionRoutine ;
    irpSpNext->Context = deferralContext->OriginalContext ;

    //
    // Some IRP dispatch routines cannot be called at passive. Two examples are
    // paging IRPs (cause we could switch) and Power IRPs. As we don't check yet,
    // if we "were" completed passive, continue to do so, but elsewhere...
    //
    passiveCompletionOK = (KeGetCurrentIrql()==PASSIVE_LEVEL) ;

    IovpTrackingDataAcquireLock(deferralContext->IovRequestPacket) ;

    //
    // Verify all completion routines are in nonpaged code.
    //
    if (deferralContext->IovRequestPacket->AssertFlags&ASSERTFLAG_POLICEIRPS) {

        if (MmIsSystemAddressLocked(irpSpNext->CompletionRoutine) == FALSE) {

            WDM_FAIL_ROUTINE((
                DCERROR_COMPLETION_ROUTINE_PAGABLE,
                DCPARAM_IRP + DCPARAM_ROUTINE,
                Irp,
                irpSpNext->CompletionRoutine
                )) ;
        }
    }

    trackingFlags = deferralContext->IovRequestPacket->AssertFlags;

    ASSERT(trackingFlags&ASSERTFLAG_FORCEPENDING) ;

    switch(trackingFlags&(ASSERTFLAG_DEFERCOMPLETION|
                          ASSERTFLAG_COMPLETEATPASSIVE|
                          ASSERTFLAG_COMPLETEATDPC)) {

        case ASSERTFLAG_COMPLETEATPASSIVE:
            deferAction = passiveCompletionOK ? DEFERACTION_QUEUE_WORKITEM :
                                                DEFERACTION_NORMAL ;
            break;

        case ASSERTFLAG_DEFERCOMPLETION | ASSERTFLAG_COMPLETEATPASSIVE:
            deferAction = passiveCompletionOK ? DEFERACTION_QUEUE_PASSIVE_TIMER :
                                                DEFERACTION_NORMAL ;
            break;

        case ASSERTFLAG_DEFERCOMPLETION | ASSERTFLAG_COMPLETEATDPC:
            deferAction = DEFERACTION_QUEUE_DISPATCH_TIMER ;
            break;

        case ASSERTFLAG_DEFERCOMPLETION:
            deferAction = (KeGetCurrentIrql()==DISPATCH_LEVEL) ?
                DEFERACTION_QUEUE_DISPATCH_TIMER :
                DEFERACTION_QUEUE_PASSIVE_TIMER ;
            break;

        default:
            deferAction = DEFERACTION_NORMAL ;
            KDASSERT(0) ;
    }

    if (deferAction != DEFERACTION_NORMAL) {

        //
        // Set this flag. If anybody uses this IRP while this flag is on, complain
        // immediately!
        //
        ASSERT(!(trackingFlags&TRACKFLAG_QUEUED_INTERNALLY)) ;
        deferralContext->IovRequestPacket->Flags |= TRACKFLAG_QUEUED_INTERNALLY ;
        deferralContext->DeviceObject = DeviceObject ;

        deferralContext->IovRequestPacket->RestoreHandle =
            IovpProtectedIrpMakeUntouchable(
                Irp,
                FALSE
                ) ;
    } else {

        IovpTrackingDataDereference(deferralContext->IovRequestPacket, IOVREFTYPE_POINTER);
    }

    IovpTrackingDataReleaseLock(deferralContext->IovRequestPacket) ;

    deferralContext->DeferAction = deferAction ;

    switch(deferAction) {

        case DEFERACTION_QUEUE_PASSIVE_TIMER:
            KeInitializeTimerEx(&deferralContext->DeferralTimer, SynchronizationTimer) ;
            KeSetTimerEx(
                &deferralContext->DeferralTimer,
                deltaTime,
                0,
                NULL
                ) ;

            //
            // Fall through...
            //

        case DEFERACTION_QUEUE_WORKITEM:

            //
            // Queue this up so we can complete this passively.
            //
            ExInitializeWorkItem(
                (PWORK_QUEUE_ITEM)&deferralContext->WorkQueueItem,
                IovpInternalCompleteAfterWait,
                deferralContext
                );

            ExQueueWorkItem(
                (PWORK_QUEUE_ITEM)&deferralContext->WorkQueueItem,
                DelayedWorkQueue
                );

            return STATUS_MORE_PROCESSING_REQUIRED ;

        case DEFERACTION_QUEUE_DISPATCH_TIMER:

            KeInitializeDpc(
                &deferralContext->DpcItem,
                IovpInternalCompleteAtDPC,
                deferralContext
                );

            KeInitializeTimerEx(&deferralContext->DeferralTimer, SynchronizationTimer) ;
            KeSetTimerEx(
                &deferralContext->DeferralTimer,
                deltaTime,
                0,
                &deferralContext->DpcItem
                ) ;
            return STATUS_MORE_PROCESSING_REQUIRED ;

        case DEFERACTION_NORMAL:
        default:

            ExFreePool(deferralContext) ;
            return irpSpNext->CompletionRoutine(DeviceObject, Irp, irpSpNext->Context) ;
    }
}

NTSTATUS
IovpSwapSurrogateIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

  Description:

    This completion routine will copy back the surrogate IRP
    to the original and complete the original IRP.

  Arguments:

    DeviceObject           - Device object set at this level
                             of the completion routine - ignored.

    Irp                    - A pointer to the IRP.

    Context                - Context should equal the IRP - this is
                             asserted.

  Return Value:

     STATUS_MORE_PROCESSING_REQUIRED...

--*/
{
    PIOV_REQUEST_PACKET iovPacket, iovPrevPacket;
    PIOV_SESSION_DATA iovSessionData;
    ULONG irpSize ;
    PIRP realIrp ;
    BOOLEAN freeTrackingData ;
    NTSTATUS status, lockedStatus ;
    CCHAR priorityBoost ;
    PVOID completionRoutine ;
    PIO_STACK_LOCATION irpSp ;
    BOOLEAN locked ;

    //
    // If this one fails, somebody has probably copied the stack
    // inclusive with our completion routine. We should already
    // have caught this...
    //
    ASSERT(Irp == Context) ;

    iovPacket = IovpTrackingDataFindAndLock(Irp) ;
    ASSERT(iovPacket) ;

    if (iovPacket == NULL) {

        return STATUS_SUCCESS ;
    }

    ASSERT(iovPacket->TopStackLocation == Irp->CurrentLocation) ;

    iovSessionData = IovpTrackingDataGetCurrentSessionData(iovPacket);
    ASSERT(iovSessionData);

    //
    // Put everything back
    //
    ASSERT(iovPacket->HeadPacket != iovPacket);

    iovPrevPacket = CONTAINING_RECORD(
        iovPacket->SurrogateLink.Blink,
        IOV_REQUEST_PACKET,
        SurrogateLink
        );

    realIrp = iovPrevPacket->TrackedIrp ;
    irpSize = IoSizeOfIrp( Irp->StackCount ) ;

    //
    // Back the IRP stack up so that the original completion routine
    // is called if appropriate
    //
    IoSetNextIrpStackLocation(Irp);
    IoSetNextIrpStackLocation(realIrp);

    irpSp = IoGetCurrentIrpStackLocation(Irp) ;
    irpSp->CompletionRoutine = iovPacket->RealIrpCompletionRoutine ;
    irpSp->Control           = iovPacket->RealIrpControl ;
    irpSp->Context           = iovPacket->RealIrpContext ;

    //
    // Record final data and make any accesses to the surrogate IRP
    // crash.
    //
    irpSp = IoGetNextIrpStackLocation(Irp) ;
    if (iovPacket->AssertFlags&ASSERTFLAG_POLICEIRPS) {

        IovpAssertFinalIrpStack(iovPacket, irpSp) ;
    }

    priorityBoost = iovPacket->PriorityBoost ;
    IovpTrackingDataDereference(iovPacket, IOVREFTYPE_POINTER);
    IovpSessionDataFinalizeSurrogate(iovSessionData, iovPacket, Irp);
    IovpSessionDataClose(iovSessionData);
    IovpSessionDataDereference(iovSessionData);

    TRACKIRP_DBGPRINT((
        "  Swapping surrogate IRP %lx back to %lx (Tracking data %lx)\n",
        Irp,
        realIrp,
        iovPacket
        ), 1) ;

    iovPacket->Flags |= TRACKFLAG_SWAPPED_BACK ;
    IovpTrackingDataReleaseLock(iovPacket) ;

    //
    // Send the IRP onwards and upwards.
    //
    IoCompleteRequest(realIrp, priorityBoost) ;

    return STATUS_MORE_PROCESSING_REQUIRED ;
}

VOID
FASTCALL
IovpCancelIrp(
    IN     PIRP               Irp,
    OUT    PBOOLEAN           CancelHandled,
    OUT    PBOOLEAN           ReturnValue
    )
/*++

  Description:

    This routine is called by IoCancelIrp and returns TRUE iff
    the cancelation was handled internally here (in which case
    IoCancelIrp should do nothing).

    We need to handle the call internally when we are currently
    dealing with a surrogate. In this case, we make sure the
    surrogate is cancelled instead.

  Arguments:

    Irp                    - A pointer to the IRP passed into
                             IoCancelIrp.

    CancelHandled          - Indicates whether the IRP cancellation
                             was handled entirely by this routine.

    ReturnValue            - Set to the value IoCancelIrp
                             should return if the IRP cancelation
                             was handled entirely by this routine.

  Return Value:

     None.

--*/
{
    PIOV_REQUEST_PACKET iovPacket, iovNextPacket;
    PIRP irpToCancel;
    KIRQL irql ;

    *CancelHandled = FALSE ;

    iovPacket = IovpTrackingDataFindAndLock(Irp) ;
    if (!iovPacket) {

        return ;
    }

    //
    // If the IRP is queued internally, touching it is not very safe as we may
    // have temporarily removed the page's backing. Restore the backing while
    // under the IRPs track lock.
    //

    if (iovPacket->Flags&TRACKFLAG_QUEUED_INTERNALLY) {

        IovpProtectedIrpMakeTouchable(
            Irp,
            &iovPacket->RestoreHandle
            );

        iovPacket->RestoreHandle = NULL ;
    }

    if (!(iovPacket->Flags&TRACKFLAG_ACTIVE)) {

        //
        // We've already completed the IRP, and the only reason it's
        // still being tracked is because of it's allocation.
        // So it is not ours to cancel.
        //
        IovpTrackingDataReleaseLock(iovPacket);
        return;
    }

    if (!(iovPacket->Flags&TRACKFLAG_HAS_SURROGATE)) {

        //
        // Cancel of an IRP that doesn't have an active surrogate. Let it
        // proceed normally.
        //
        IovpTrackingDataReleaseLock(iovPacket);
        return;
    }

    if (iovPacket->AssertFlags&ASSERTFLAG_POLICEIRPS) {

        if (Irp->CancelRoutine) {

            WDM_FAIL_ROUTINE((
                DCERROR_CANCELROUTINE_ON_FORWARDED_IRP,
                DCPARAM_IRP + DCPARAM_ROUTINE,
                Irp,
                Irp->CancelRoutine
                ));

            //
            // We will ignore this routine. As we should...
            //
        }
    }

    iovNextPacket = CONTAINING_RECORD(
        iovPacket->SurrogateLink.Flink,
        IOV_REQUEST_PACKET,
        SurrogateLink
        );

    Irp->Cancel = TRUE;
    *CancelHandled = TRUE ;
    irpToCancel = iovNextPacket->TrackedIrp ;
    IovpTrackingDataReleaseLock(iovPacket) ;
    *ReturnValue = IoCancelIrp(irpToCancel) ;

    return ;
}

VOID
FASTCALL
IovpFreeIrp(
    IN     PIRP               Irp,
    IN OUT PBOOLEAN           FreeHandled
    )
/*++

  Description:

    This routine is called by IoFreeIrp and returns TRUE iff
    the free was handled internally here (in which case IoFreeIrp
    should do nothing).

    We need to handle the call internally because we may turn off lookaside
    list cacheing to catch people reusing IRPs after they are freed.

  Arguments:

    Irp                    - A pointer to the IRP passed into
                             IoCancelIrp.

    FreeHandled            - Indicates whether the free operation was
                             handled entirely by this routine.

  Return Value:

     None.

--*/
{
    PIOV_REQUEST_PACKET iovPacket;
    PVOID restoreHandle ;

    iovPacket = IovpTrackingDataFindAndLock(Irp);

    if (iovPacket == NULL) {

        //
        // ADRIAO BUGBUG 01/06/1999 -
        //     Below assertion might fire if an IRP allocated then freed twice.
        //
        ASSERT(!(Irp->AllocationFlags&IRP_ALLOCATION_MONITORED));
        *FreeHandled = FALSE ;
        return;
    }

    if (!IsListEmpty(&Irp->ThreadListEntry)) {

        if (iovPacket->AssertFlags&ASSERTFLAG_POLICEIRPS) {

            WDM_FAIL_CALLER2(
                (DCERROR_FREE_OF_THREADED_IRP, DCPARAM_IRP, Irp)
                );
        }

        //
        // <Grumble> keep us alive by not actually freeing the IRP if someone did
        // this to us. We leak for life...
        //
        *FreeHandled = TRUE ;
        return ;
    }

    if (IovpTrackingDataGetCurrentSessionData(iovPacket)) {

        //
        // If there's a current session, that means someone is freeing an IRP
        // that they don't own. Of course, if the stack unwound badly because
        // someone forgot to return PENDING or complete the IRP, then we don't
        // assert here (we'd probably end up blaiming kernel).
        //
        if ((iovPacket->AssertFlags&ASSERTFLAG_POLICEIRPS) &&
            (!(iovPacket->Flags&TRACKFLAG_UNWOUND_BADLY))) {

            WDM_FAIL_CALLER2(
                (DCERROR_FREE_OF_INUSE_IRP, DCPARAM_IRP, Irp)
                );
        }

        //
        // <Grumble> keep us alive by not actually freeing the IRP if someone did
        // this to us. We leak for life...
        //
        IovpTrackingDataReleaseLock(iovPacket) ;
        *FreeHandled = TRUE ;
        return ;
    }

    if (!(iovPacket->Flags&TRACKFLAG_IO_ALLOCATED)) {

        //
        // We weren't tracking this at allocation time. We shouldn't got our
        // packet unless the IRP had a pointer count still, meaning it's has
        // a session. And that should've been caught above.
        //
        ASSERT(0);
        IovpTrackingDataReleaseLock(iovPacket) ;
        *FreeHandled = FALSE ;
        return;
    }

    //
    // The IRP may have been reinitialized, possibly losing it's allocation
    // flags. We catch this bug in the IoInitializeIrp hook.
    //
    //ASSERT(Irp->AllocationFlags&IRP_ALLOCATION_MONITORED) ;
    //

    if (!(iovPacket->Flags&TRACKFLAG_PROTECTEDIRP)) {

        //
        // We're just tagging along this IRP. Drop our pointer count but bail.
        //
        IovpTrackingDataDereference(iovPacket, IOVREFTYPE_POINTER);
        IovpTrackingDataReleaseLock(iovPacket);
        *FreeHandled = FALSE;
        return;
    }

    //
    // Set up a nice bugcheck for those who free their IRPs twice. This is done
    // because the special pool may have been exhausted, in which case the IRP
    // can be touched after it has been freed.
    //
    Irp->Type = 0;

    ASSERT(iovPacket) ;
    ASSERT(iovPacket->AssertFlags&ASSERTFLAG_POLICEIRPS);
    IovpTrackingDataDereference(iovPacket, IOVREFTYPE_POINTER);
    ASSERT(iovPacket->PointerCount == 0);
    IovpTrackingDataReleaseLock(iovPacket) ;
    restoreHandle = IovpProtectedIrpMakeUntouchable(Irp, TRUE) ;
    IovpProtectedIrpFree(Irp, &restoreHandle) ;

    //
    // We handled allocation and initialization. There is nothing much more to
    // do.
    //
    *FreeHandled = TRUE ;
}

VOID
FASTCALL
IovpAllocateIrp1(
    IN     CCHAR             StackSize,
    IN     BOOLEAN           ChargeQuota,
    IN OUT PIRP              *IrpPointer
    )
/*++

  Description:

    This routine is called by IoAllocateIrp and returns an IRP iff
    we are handled the allocations ourselves.

    We may need to do this internally so we can turn off IRP lookaside lists
    and use the special pool to catch people reusing free'd IRPs.

  Arguments:

    StackSize              - Count of stack locations to allocate for this IRP.

    ChargeQuote            - TRUE if quote should be charged against the current
                             thread.

    IrpPointer             - Pointer to IRP if one was allocated. This will
                             point to NULL after the call iff IoAllocateIrp
                             should use it's normal lookaside list code.

  Return Value:

    None.

--*/
{
    PIOV_REQUEST_PACKET iovPacket;
    PVOID returnAddress[1];
    ULONG stackHash;
    PIRP irp;

    *IrpPointer = NULL ;
    if (!(IovpTrackingFlags&ASSERTFLAG_MONITOR_ALLOCS)) {

        return ;
    }

    if (!(IovpTrackingFlags&ASSERTFLAG_POLICEIRPS)) {

        return ;
    }

    irp = IovpProtectedIrpAllocate(
        StackSize,
        ChargeQuota,
        PsGetCurrentThread()
        ) ;

    if (irp == NULL) {

        return;
    }

    IopInitializeIrp(irp, IoSizeOfIrp(StackSize), StackSize);
    *IrpPointer = irp;

    iovPacket = IovpTrackingDataCreateAndLock(irp);

    if (iovPacket == NULL) {

        return;
    }

    IovpTrackingDataReference(iovPacket, IOVREFTYPE_POINTER);
    iovPacket->Flags |= TRACKFLAG_PROTECTEDIRP | TRACKFLAG_IO_ALLOCATED;
    irp->AllocationFlags |= IRP_ALLOCATION_MONITORED ;
    irp->Flags |= IRPFLAG_EXAMINE_TRACKED;

    //
    // Record he who allocated this IRP (if we can get it)
    //
    RtlCaptureStackBackTrace(3, IRP_ALLOC_COUNT, iovPacket->AllocatorStack, &stackHash) ;

    IovpTrackingDataReleaseLock(iovPacket) ;
}

VOID
FASTCALL
IovpAllocateIrp2(
    IN     PIRP               Irp
    )
/*++

  Description:

    This routine is called by IoAllocateIrp and captures information if
    the IRP was allocated by the OS.

  Arguments:

    Irp                    - Pointer to IRP

  Return Value:

    None.

--*/
{
    PIOV_REQUEST_PACKET iovPacket;
    PVOID returnAddress[1];
    ULONG stackHash;

    if (!(IovpTrackingFlags&ASSERTFLAG_MONITOR_ALLOCS)) {

        return;
    }

//    ASSERT(!(IovpTrackingFlags&ASSERTFLAG_POLICEIRPS));

    iovPacket = IovpTrackingDataCreateAndLock(Irp);
    if (iovPacket == NULL) {

        return;
    }

    IovpTrackingDataReference(iovPacket, IOVREFTYPE_POINTER);
    iovPacket->Flags |= TRACKFLAG_IO_ALLOCATED;
    Irp->AllocationFlags |= IRP_ALLOCATION_MONITORED;
    Irp->Flags |= IRPFLAG_EXAMINE_TRACKED;

    //
    // Record he who allocated this IRP (if we can get it)
    //
    RtlCaptureStackBackTrace(2, IRP_ALLOC_COUNT, iovPacket->AllocatorStack, &stackHash) ;

    IovpTrackingDataReleaseLock(iovPacket) ;
}

VOID
FASTCALL
IovpInitializeIrp(
    IN OUT PIRP               Irp,
    IN     USHORT             PacketSize,
    IN     CCHAR              StackSize,
    IN OUT PBOOLEAN           InitializeHandled
    )
/*++

  Description:

    This routine is called by IoInitializeIrp and sets InitializeHandled to
    TRUE if the entire initialization was handled internally.

    While here we verify the caller is not Initializing an IRP allocated
    through IoAllocateIrp, as doing so means we may leak quota/etc.

  Arguments:

    Irp                    - Irp to initialize

    PacketSize             - Size of the IRP in bytes.

    StackSize              - Count of stack locations for this IRP.

    InitializeHandled      - Pointer to a BOOLEAN that will be set to true iff
                             the initialization of the IRP was handled entirely
                             within this routine. If FALSE, IoInitializeIrp
                             should initialize the IRP as normal.

  Return Value:

     None.

--*/
{
    PIOV_REQUEST_PACKET iovPacket ;

    iovPacket = IovpTrackingDataFindAndLock(Irp);
    if (iovPacket == NULL) {

        *InitializeHandled = FALSE ;
        return;
    }

    if ((iovPacket->AssertFlags&ASSERTFLAG_POLICEIRPS) &&
       (iovPacket->Flags&TRACKFLAG_IO_ALLOCATED)) {

        if (Irp->AllocationFlags&IRP_QUOTA_CHARGED) {

            //
            // Don't let us leak quota now!
            //
            WDM_FAIL_CALLER2(
                (DCERROR_REINIT_OF_ALLOCATED_IRP_WITH_QUOTA, DCPARAM_IRP, Irp)
                );

        } else {

            //
            // In this case we are draining our lookaside lists erroneously.
            //
            // WDM_CHASTISE_CALLER2(
            //    (DCERROR_REINIT_OF_ALLOCATED_IRP_WITHOUT_QUOTA, DCPARAM_IRP, Irp)
            //    );
        }
    }

    *InitializeHandled = FALSE ;
    IovpTrackingDataReleaseLock(iovPacket) ;
}

/*
 * Device Object functions
 *   IovpExamineDevObjForwarded
 *
 */

VOID
IovpAttachDeviceToDeviceStack(
    IN PDEVICE_OBJECT NewDevice,
    IN PDEVICE_OBJECT ExistingDevice
    )
{
}

VOID
IovpDetachDevice(
    IN PDEVICE_OBJECT LowerDevice
    )
{
    PDEVOBJ_EXTENSION deviceExtension;

    if (LowerDevice->AttachedDevice == NULL) {

        WDM_FAIL_CALLER2((DCERROR_DETACH_NOT_ATTACHED, DCPARAM_DEVOBJ, LowerDevice));
    } else {

        //
        // ADRIAO BUGBUG 01/07/1999 -
        //     As the stack can be torn apart simulatenously from above and
        // below during a remove IRP, we cannot assert the below, and moreover
        // changes to the ExtensionFlags will have to involve walking the tree.
        //
        // ASSERT(LowerDevice->AttachedDevice->AttachedDevice == NULL);
        //
        deviceExtension = LowerDevice->AttachedDevice->DeviceObjectExtension;
        deviceExtension->ExtensionFlags &=~ (DOE_EXAMINED | DOE_TRACKED);
    }
}

VOID
IovpDeleteDevice(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PDEVICE_OBJECT deviceBelow;

    //
    // ADRIAO BUGBUG 03/03/1999 -
    //     Complain if the dude already deleted himself.
    //

    deviceBelow = IovpGetDeviceAttachedTo(DeviceObject);
    if (deviceBelow) {

        WDM_FAIL_CALLER1((DCERROR_DELETE_WHILE_ATTACHED, 0));
        ObDereferenceObject(deviceBelow);
    }
}

VOID
IovpReexamineAllStacks(
    VOID
    )
{
    PAGED_CODE();

    ObEnumerateObjectsByType(
        IoDeviceObjectType,
        IovpEnumDevObjCallback,
        NULL
        );
}

BOOLEAN
IovpEnumDevObjCallback(
    IN PVOID Object,
    IN PUNICODE_STRING ObjectName,
    IN ULONG HandleCount,
    IN ULONG PointerCount,
    IN PVOID Context
    )
{
    PDEVICE_OBJECT      deviceObject;
    PDEVOBJ_EXTENSION   deviceExtension;

    deviceObject = (PDEVICE_OBJECT) Object;
    deviceExtension = deviceObject->DeviceObjectExtension;

    if (PointerCount || HandleCount) {

        deviceExtension->ExtensionFlags &=~ (DOE_EXAMINED | DOE_TRACKED);
    }

    return TRUE;
}

BOOLEAN
IovpIsInterestingStack(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PDEVOBJ_EXTENSION   deviceExtension;
    PDEVICE_OBJECT      currentDevObj, deviceAttachedTo;
    BOOLEAN             stackIsInteresting;
    KIRQL               irql;

    //
    // Walk downward until we find the PDO or an examined device object.
    //
    ExAcquireFastLock( &IopDatabaseLock, &irql );

    //
    // Quickly check the top of the stack...
    //
    if (DeviceObject->DeviceObjectExtension->ExtensionFlags & DOE_EXAMINED) {

        stackIsInteresting =
           ((DeviceObject->DeviceObjectExtension->ExtensionFlags & DOE_TRACKED) != 0);

        ExReleaseFastLock( &IopDatabaseLock, irql );
        return stackIsInteresting;
    }

    //
    // OK, if the top hasn't been examined, odds are devices below it haven't
    // either. Walk downwards until we can determine whether the stack as a
    // whole should be tracked.
    //
    stackIsInteresting = FALSE;
    deviceAttachedTo = DeviceObject;
    do {
        currentDevObj = deviceAttachedTo;
        deviceExtension = currentDevObj->DeviceObjectExtension;
        deviceAttachedTo = deviceExtension->AttachedTo;

        //
        // Remember this...
        //
        if (IovpIsInterestingDriver(currentDevObj->DriverObject)) {

            stackIsInteresting = TRUE;
        }

    } while (deviceAttachedTo &&
             (deviceAttachedTo->DeviceObjectExtension->ExtensionFlags & DOE_EXAMINED)
            );

    if (deviceAttachedTo &&
        (deviceAttachedTo->DeviceObjectExtension->ExtensionFlags & DOE_TRACKED)) {

        //
        // Propogate upwards the "interesting-ness" of the last examined device
        // in the stack...
        //
        stackIsInteresting = TRUE;
    }

    //
    // Walk upwards, marking everything examined and appropriately tracked.
    //
    do {
        deviceExtension = currentDevObj->DeviceObjectExtension;

        if (stackIsInteresting) {

            deviceExtension->ExtensionFlags |= DOE_TRACKED;
        } else {

            deviceExtension->ExtensionFlags &=~ DOE_TRACKED;
        }

        deviceExtension->ExtensionFlags |= DOE_EXAMINED;

        currentDevObj = currentDevObj->AttachedDevice;

    } while (currentDevObj);

    ExReleaseFastLock( &IopDatabaseLock, irql );
    return stackIsInteresting;
}

BOOLEAN
IovpIsInterestingDriver(
    IN PDRIVER_OBJECT DriverObject
    )
{
    if (IovpInitFlags&IOVERIFIERINIT_VERIFIER_DRIVER_LIST) {

        return (BOOLEAN) MmIsDriverVerifying(DriverObject);

    } else {

        return TRUE;
    }
}

VOID
FASTCALL
IovpExamineDevObjForwarding(
    IN     PDEVICE_OBJECT DeviceBeingCalled,
    IN     PDEVICE_OBJECT DeviceLastCalled,
    OUT    PULONG         ForwardTechnique
    )
/*++

    Returns:

        STARTED_TOP_OF_STACK
        FORWARDED_TO_NEXT_DO
        SKIPPED_A_DO
        STARTED_INSIDE_STACK
        CHANGED_STACKS_AT_BOTTOM
        CHANGED_STACKS_MID_STACK

--*/

{
    PDEVICE_OBJECT upperDevobj, lowerObject ;
    ULONG result ;
    KIRQL irql;

    lowerObject = IovpGetDeviceAttachedTo(DeviceLastCalled) ;

    ExAcquireFastLock( &IopDatabaseLock, &irql );

    //
    // Nice and simple. Walk the device being called
    // upwards and find either NULL or the last device
    // we called.
    //
    upperDevobj = DeviceBeingCalled->AttachedDevice ;
    while(upperDevobj && (upperDevobj != DeviceLastCalled)) {

        upperDevobj = upperDevobj->AttachedDevice ;
    }

    if (DeviceLastCalled == NULL) {

        //
        // This is a newly started IRP, was it targetted
        // at the top of a stack or at the middle/bottom?
        //
        result = (DeviceBeingCalled->AttachedDevice) ? STARTED_INSIDE_STACK :
                                                       STARTED_TOP_OF_STACK ;

    } else if (upperDevobj == NULL) {

        //
        // We were forwarded the Irp beyond our own stack
        //
        result = (lowerObject) ? CHANGED_STACKS_MID_STACK :
                                 CHANGED_STACKS_AT_BOTTOM ;

    } else if (DeviceBeingCalled->AttachedDevice == upperDevobj) {

        result = FORWARDED_TO_NEXT_DO ;

        //
        // Quick assertion, if LastDevice was non-NULL, the
        // device under him should be the one we are calling...
        //
        ASSERT(lowerObject == DeviceBeingCalled) ;

    } else {

        //
        // DeviceLastCalled was found higher in the stack, but wasn't
        // directly above DeviceBeingCalled
        //
        result = SKIPPED_A_DO ;
    }

    ExReleaseFastLock( &IopDatabaseLock, irql );
    if (lowerObject) {
        ObDereferenceObject(lowerObject) ;
    }
    *ForwardTechnique = result ;
}

PDEVICE_OBJECT
FASTCALL
IovpGetDeviceAttachedTo(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PDEVOBJ_EXTENSION deviceExtension;
    PDEVICE_OBJECT deviceAttachedTo ;
    KIRQL irql ;

    if (DeviceObject == NULL) {

        return NULL ;
    }

    ExAcquireFastLock( &IopDatabaseLock, &irql );

    deviceExtension = DeviceObject->DeviceObjectExtension;
    deviceAttachedTo = deviceExtension->AttachedTo ;

    if (deviceAttachedTo) {
        ObReferenceObject(deviceAttachedTo) ;
    }

    ExReleaseFastLock( &IopDatabaseLock, irql );
    return deviceAttachedTo ;
}

PDEVICE_OBJECT
FASTCALL
IovpGetLowestDevice(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

    Opposite of IoGetAttachedDeviceReference

--*/
{
    PDEVOBJ_EXTENSION deviceExtension;
    PDEVICE_OBJECT lowerDevobj, deviceAttachedTo ;
    KIRQL irql ;

    deviceAttachedTo = DeviceObject ;

    ExAcquireFastLock( &IopDatabaseLock, &irql );

    do {
        lowerDevobj = deviceAttachedTo ;
        deviceExtension = lowerDevobj->DeviceObjectExtension;
        deviceAttachedTo = deviceExtension->AttachedTo ;

    } while ( deviceAttachedTo );

    ObReferenceObject(lowerDevobj) ;

    ExReleaseFastLock( &IopDatabaseLock, irql );
    return lowerDevobj ;
}

VOID
FASTCALL
IovpAssertNonLegacyDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG StackFramesToSkip,
    IN PUCHAR FailureTxt
    )
/*++

--*/
{
    PDEVICE_OBJECT pdoDeviceObject ;
    PDEVICE_NODE pDevNode ;

    pdoDeviceObject = IovpGetLowestDevice(DeviceObject) ;

    if (pdoDeviceObject) {

        pDevNode = pdoDeviceObject->DeviceObjectExtension->DeviceNode ;
        if (pDevNode&&(!(pDevNode->Flags&DNF_LEGACY_DRIVER))) {

            //
            // ADRIAO BUGBUG 12/30/98 - More stuff to fix...
            //
            ASSERT(0);
/*
            WDM_FAIL_CALLER(
                (FailureTxt),
                StackFramesToSkip+1,
                NULL
                );
*/
        }
        ObDereferenceObject(pdoDeviceObject) ;
    }
}

BOOLEAN
FASTCALL
IovpIsInFdoStack(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PDEVOBJ_EXTENSION deviceExtension;
    PDEVICE_OBJECT deviceAttachedTo, lowerDevobj ;
    KIRQL irql ;

    deviceAttachedTo = DeviceObject ;

    ExAcquireFastLock( &IopDatabaseLock, &irql );

    do {
        if (deviceAttachedTo->DeviceObjectExtension->ExtensionFlags&DOE_BOTTOM_OF_FDO_STACK) {
            break;
        }
        deviceAttachedTo = deviceAttachedTo->DeviceObjectExtension->AttachedTo ;

    } while ( deviceAttachedTo );

    ExReleaseFastLock( &IopDatabaseLock, irql );
    return (deviceAttachedTo != NULL) ;
}

VOID
FASTCALL
IovpSeedOnePage(
    VOID
    )
{
    ULONG StackSeed[(PAGE_SIZE/sizeof(ULONG))] ;
    ULONG register i ;

    //
    // We use the return value 0xFFFFFFFF, as it is an illegal return value. We
    // are trying to catch people who don't initialize NTSTATUS, and it's also
    // a good pointer trap too.
    //
    for(i=0; i<(PAGE_SIZE/sizeof(ULONG)); i++) StackSeed[i]=0xFFFFFFFF ;
}

VOID
FASTCALL
IovpSeedTwoPages(
    VOID
    )
{
    ULONG StackSeed[(PAGE_SIZE*2/sizeof(ULONG))] ;
    ULONG register i ;

    for(i=0; i<(PAGE_SIZE*2/sizeof(ULONG)); i++) StackSeed[i]=0xFFFFFFFF ;
}

VOID
FASTCALL
IovpSeedThreePages(
    VOID
    )
{
    ULONG register i ;
    ULONG StackSeed[(PAGE_SIZE*3/sizeof(ULONG))] ;

    for(i=0; i<(PAGE_SIZE*3/sizeof(ULONG)); i++) StackSeed[i]=0xFFFFFFFF ;
}

VOID
FASTCALL
IovpSeedStack(
    VOID
    )
/*++

  Description:

    This routine "seeds" the stack so that uninitialized variables are
    more easily ferreted out.

    ADRIAO BUGBUG 08/17/98 - This is a really neat idea that runs into a
                             memory manager optimization. While I do find
                             the appropriate guard page, the memory manager
                             throws out the above touched pages on a thread
                             switch and bring in new (and probably zero'd)
                             ones.

  Arguments: None

  Return Value: None

--*/
{
    int i, interruptReservedOverhead ;

    if (!(IovpTrackingFlags&ASSERTFLAG_SEEDSTACK)) {
        return ;
    }

    //
    // Is there room to try this before we run out of stack? We will reserve
    // half a page for interrupt overhead...
    //
    interruptReservedOverhead = PAGE_SIZE/2 ;

    //
    // There must be a guard page somewhere. Find it...
    //
    for(i=0; i<4; i++) {
        if (!MmIsAddressValid(((PUCHAR)&i)-i*PAGE_SIZE-interruptReservedOverhead)) {
            break;
        }
    }

    switch(i) {
        case 4: IovpSeedThreePages() ; break ;
        case 3: IovpSeedTwoPages() ; break ;
        case 2: IovpSeedOnePage() ; break ;
        case 1: break ; // Minimum is overhead
        case 0: break ; // Umm, we don't even have overhead!
        default: break ;
    }
}

#endif // NO_SPECIAL_IRP




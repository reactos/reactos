#include "iop.h"

/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   ioverifier.c

Abstract:

    This module contains the routines to verify suspect drivers.

Author:

    Narayanan Ganapathy (narg) 8-Jan-1999

Revision History:

    Adrian J. Oney (AdriaO) 28-Feb-1999
        - merge in special irp code.

--*/

#if (( defined(_X86_) ) && ( FPO ))
#pragma optimize( "y", off )    // disable FPO for consistent stack traces
#endif


#define IO_FREE_IRP_TYPE_INVALID                1
#define IO_FREE_IRP_NOT_ASSOCIATED_WITH_THREAD  2
#define IO_CALL_DRIVER_IRP_TYPE_INVALID         3
#define IO_CALL_DRIVER_INVALID_DEVICE_OBJECT    4
#define IO_CALL_DRIVER_IRQL_NOT_EQUAL           5
#define IO_COMPLETE_REQUEST_INVALID_STATUS      6
#define IO_COMPLETE_REQUEST_CANCEL_ROUTINE_SET  7
#define IO_BUILD_FSD_REQUEST_EXCEPTION          8
#define IO_BUILD_IOCTL_REQUEST_EXCEPTION        9
#define IO_REINITIALIZING_TIMER_OBJECT          10
#define IO_INVALID_HANDLE                       11
#define IO_INVALID_STACK_IOSB                   12
#define IO_INVALID_STACK_EVENT                  13
#define IO_COMPLETE_REQUEST_INVALID_IRQL        14

//
// 0x200 and up are defined in ioassert.c
//

PVOID
VerifierAllocatePoolWithQuotaTag(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

#define IsKernelHandle(H,M) (((LONG_PTR)(H) < 0) && ((M) == KernelMode) && ((H) != NtCurrentThread()) && ((H) != NtCurrentProcess()))

BOOLEAN
IovpValidateDeviceObject(
    IN  PDEVICE_OBJECT  DeviceObject
    );
VOID
IovFreeIrpPrivate(
    IN  PIRP    Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, IoVerifierInit)
#pragma alloc_text(PAGEVRFY,IovAllocateIrp)
#pragma alloc_text(PAGEVRFY,IovFreeIrp)
#pragma alloc_text(PAGEVRFY,IovCallDriver)
#pragma alloc_text(PAGEVRFY,IovCompleteRequest)
#pragma alloc_text(PAGEVRFY,IovCallDriver)
#pragma alloc_text(PAGEVRFY,IovCompleteRequest)
#pragma alloc_text(PAGEVRFY,IovpValidateDeviceObject)
#pragma alloc_text(PAGEVRFY,IovSpecialIrpCallDriver)
#pragma alloc_text(PAGEVRFY,IovSpecialIrpCompleteRequest)
#pragma alloc_text(PAGEVRFY,IovFreeIrpPrivate)
#endif

BOOLEAN         IopVerifierOn = FALSE;
ULONG           IovpEnforcementLevel = (ULONG) -1;
ULONG           IovpVerifierLevel = (ULONG)0;
BOOLEAN         IoVerifierOnByDefault = TRUE;
LONG            IovpInitCalled = 0;
ULONG           IovpMaxSupportedVerifierLevel = 3;
ULONG           IovpVerifierFlags = 0;               // Stashes the verifier flags passed at init.

VOID
IoVerifierInit(
    IN ULONG VerifierFlags,
    IN ULONG InitFlags
    )
{
    PVOID   sectionHeaderHandle;
    ULONG   verifierLevel;

    if (IoVerifierOnByDefault) {
        VerifierFlags |= DRIVER_VERIFIER_IO_CHECKING;
    }

    if (!VerifierFlags) {
        return;
    }
    pIoAllocateIrp = IovAllocateIrp;

#ifndef NO_SPECIAL_IRP
    if (!(InitFlags & IOVERIFIERINIT_PHASE0)) {

        //
        // Lock it down.
        //
        sectionHeaderHandle = MmLockPagableCodeSection(IopDriverCorrectnessTakeLock);

        if (!sectionHeaderHandle) {

            return;
        }
    }

    //
    // Various initialization
    //
    if (IopDcControlOverride == (ULONG) -1) {

        IopDcControlOverride = 0;
    }

    if (IopDcControlInitial == (ULONG) -1) {

        IopDcControlInitial = 0;
        if (!(InitFlags & IOVERIFIERINIT_VERIFIER_DRIVER_LIST)) {

            IopDcControlInitial |= DIAG_IGNORE_DRIVER_LIST;
        }
    }

    KeInitializeSpinLock(&IopDcControlLock) ;

#endif // NO_SPECIAL_IRP

    if (!(VerifierFlags & DRIVER_VERIFIER_IO_CHECKING)) {

        return;
    }

    //
    // Determine the level of verification. Later we will modify the driver
    // verifier applet to pass in a level directly.
    //

#if !DBG
    if (IovpVerifierLevel > IovpMaxSupportedVerifierLevel) {
        IovpVerifierLevel  = IovpMaxSupportedVerifierLevel;
    }
#endif

    verifierLevel = IovpVerifierLevel;

    //
    // Enable and hook in the verifier.
    //
    IopVerifierOn = TRUE;
    IovpInitCalled = 1;
    IovpVerifierFlags = VerifierFlags;

    if (verifierLevel > 1) {
        //
        // Initialize the special IRP code as appropriate.
        //
#ifndef NO_SPECIAL_IRP
        IovpInitIrpTracking(verifierLevel, InitFlags);
#endif // NO_SPECIAL_IRP
        InterlockedExchangePointer((PVOID *)&pIofCallDriver, (PVOID) IovSpecialIrpCallDriver);
        InterlockedExchangePointer((PVOID *)&pIofCompleteRequest, (PVOID) IovSpecialIrpCompleteRequest);
        InterlockedExchangePointer((PVOID *)&pIoFreeIrp, (PVOID) IovFreeIrpPrivate);
    }
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
}


BOOLEAN
IovpValidateDeviceObject(
    IN  PDEVICE_OBJECT  DeviceObject
    )
{
    if ((DeviceObject->Type != IO_TYPE_DEVICE) ||
        (DeviceObject->DriverObject == NULL) ||
        (DeviceObject->ReferenceCount < 0 )) {
        return FALSE;
    } else {
        return TRUE;
    }
}

VOID
IovFreeIrp(
    IN  PIRP    Irp
    )
{
    IovFreeIrpPrivate(Irp);
}

VOID
IovFreeIrpPrivate(
    IN  PIRP    Irp
    )
{


#ifndef NO_SPECIAL_IRP
    BOOLEAN freeHandled ;
#endif


    if (IopVerifierOn) {
        if (Irp->Type != IO_TYPE_IRP) {
            KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                         IO_FREE_IRP_TYPE_INVALID,
                         (ULONG_PTR)Irp,
                         0,
                         0);
        }
        if (!IsListEmpty(&(Irp)->ThreadListEntry)) {
            KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                         IO_FREE_IRP_NOT_ASSOCIATED_WITH_THREAD,
                         (ULONG_PTR)Irp,
                         0,
                         0);
        }
    }

#ifndef NO_SPECIAL_IRP
    SPECIALIRP_IO_FREE_IRP(Irp, &freeHandled) ;

    if (freeHandled) {

       return ;
    }
#endif

    IopFreeIrp(Irp);
}

NTSTATUS
FASTCALL
IovCallDriver(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  OUT PIRP    Irp
    )
{
    KIRQL    saveIrql;
    NTSTATUS status;


    if (!IopVerifierOn) {
        status = IopfCallDriver(DeviceObject, Irp);
        return status;
    }

    if (IovpVerifierLevel > 1) {
        status = IovSpecialIrpCallDriver(DeviceObject, Irp);
        return status;
    }
    if (Irp->Type != IO_TYPE_IRP) {
        KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                     IO_CALL_DRIVER_IRP_TYPE_INVALID,
                     (ULONG_PTR)Irp,
                     0,
                     0);
    }
    if (!IovpValidateDeviceObject(DeviceObject)) {
        KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                     IO_CALL_DRIVER_INVALID_DEVICE_OBJECT,
                     (ULONG_PTR)DeviceObject,
                     0,
                     0);
    }

    saveIrql = KeGetCurrentIrql();
    status = IopfCallDriver(DeviceObject, Irp);
    if (saveIrql != KeGetCurrentIrql()) {
        KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                     IO_CALL_DRIVER_IRQL_NOT_EQUAL,
                     (ULONG_PTR)DeviceObject,
                     saveIrql,
                     KeGetCurrentIrql());

    }
    return status;
}


VOID
FASTCALL
IovCompleteRequest(
    IN  PIRP    Irp,
    IN  CCHAR   PriorityBoost
    )
{

    if (!IopVerifierOn) {
        IopfCompleteRequest(Irp, PriorityBoost);
        return;
    }

    if (IovpVerifierLevel > 1) {
        IovSpecialIrpCompleteRequest(Irp, PriorityBoost);
        return;
    }

    if (Irp->CurrentLocation > (CCHAR) (Irp->StackCount + 1) ||
        Irp->Type != IO_TYPE_IRP) {
        KeBugCheckEx( MULTIPLE_IRP_COMPLETE_REQUESTS,
                      (ULONG_PTR) Irp,
                      __LINE__,
                      0,
                      0);
    }

    if (Irp->CancelRoutine) {
        KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                     IO_COMPLETE_REQUEST_CANCEL_ROUTINE_SET,
                     (ULONG_PTR)Irp->CancelRoutine,
                     (ULONG_PTR)Irp,
                     0);
    }
    if (Irp->IoStatus.Status == STATUS_PENDING || Irp->IoStatus.Status == 0xffffffff) {
         KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                      IO_COMPLETE_REQUEST_INVALID_STATUS,
                      Irp->IoStatus.Status,
                      (ULONG_PTR)Irp,
                      0);
    }
    if (KeGetCurrentIrql() > DISPATCH_LEVEL) {
        KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                     IO_COMPLETE_REQUEST_INVALID_IRQL,
                     KeGetCurrentIrql(),
                     (ULONG_PTR)Irp,
                     0);

    }
    IopfCompleteRequest(Irp, PriorityBoost);
}


//
// Wrapper for IovAllocateIrp. Use special pool to allocate the IRP.
// This is directly called from IoAllocateIrp.
//
PIRP
IovAllocateIrp(
    IN CCHAR StackSize,
    IN BOOLEAN ChargeQuota
    )
{
    USHORT allocateSize;
    UCHAR fixedSize;
    PIRP irp;
    UCHAR mustSucceed;
    USHORT packetSize;

#ifndef NO_SPECIAL_IRP
    //
    // Should we override normal lookaside caching so that we may catch
    // more bugs?
    //
    SPECIALIRP_IO_ALLOCATE_IRP_1(StackSize, ChargeQuota, &irp) ;

    if (irp) {
       return irp ;
    }
#endif

    //
    // If special pool is not turned on lets just call the standard
    // irp allocator.
    //

    if (!(IovpVerifierFlags & DRIVER_VERIFIER_SPECIAL_POOLING )) {
        irp = IopAllocateIrpPrivate(StackSize, ChargeQuota);
        return irp;
    }


    irp = NULL;
    fixedSize = 0;
    mustSucceed = 0;
    packetSize = IoSizeOfIrp(StackSize);
    allocateSize = packetSize;

    //
    // There are no free packets on the lookaside list, or the packet is
    // too large to be allocated from one of the lists, so it must be
    // allocated from nonpaged pool. If quota is to be charged, charge it
    // against the current process. Otherwise, allocate the pool normally.
    //

    if (ChargeQuota) {
        try {
            irp = ExAllocatePoolWithTagPriority(
                    NonPagedPool,
                    allocateSize,
                    ' prI',
                    HighPoolPrioritySpecialPoolOverrun);
        } except(EXCEPTION_EXECUTE_HANDLER) {
            NOTHING;
        }

    } else {

        //
        // Attempt to allocate the pool from non-paged pool.  If this
        // fails, and the caller's previous mode was kernel then allocate
        // the pool as must succeed.
        //

        irp = ExAllocatePoolWithTagPriority(
                NonPagedPool,
                allocateSize,
                ' prI',
                HighPoolPrioritySpecialPoolOverrun);
        if (!irp) {
            mustSucceed = IRP_ALLOCATED_MUST_SUCCEED;
            if (KeGetPreviousMode() == KernelMode ) {
                irp = ExAllocatePoolWithTagPriority(
                        NonPagedPoolMustSucceed,
                        allocateSize,
                        ' prI',
                        HighPoolPrioritySpecialPoolOverrun);
            }
        }
    }

    if (!irp) {
        return NULL;
    }

    //
    // Initialize the packet.
    //

    IopInitializeIrp(irp, packetSize, StackSize);
    irp->AllocationFlags = mustSucceed;
    if (ChargeQuota) {
        irp->AllocationFlags |= IRP_QUOTA_CHARGED;
    }

    SPECIALIRP_IO_ALLOCATE_IRP_2(irp) ;
    return irp;
}

PIRP
IovBuildAsynchronousFsdRequest(
    IN ULONG MajorFunction,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PVOID Buffer OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN PLARGE_INTEGER StartingOffset OPTIONAL,
    IN PIO_STATUS_BLOCK IoStatusBlock OPTIONAL
    )
{
    PIRP    Irp;

    try {
        Irp = IoBuildAsynchronousFsdRequest(
            MajorFunction,
            DeviceObject,
            Buffer,
            Length,
            StartingOffset,
            IoStatusBlock
            );
    } except(EXCEPTION_EXECUTE_HANDLER) {
         KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                      IO_BUILD_FSD_REQUEST_EXCEPTION,
                      (ULONG_PTR)DeviceObject,
                      (ULONG_PTR)MajorFunction,
                      GetExceptionCode());
    }
    return (Irp);
}

PIRP
IovBuildDeviceIoControlRequest(
    IN ULONG IoControlCode,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN BOOLEAN InternalDeviceIoControl,
    IN PKEVENT Event,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    )
{
    PIRP    Irp;

    try {
        Irp = IoBuildDeviceIoControlRequest(
            IoControlCode,
            DeviceObject,
            InputBuffer,
            InputBufferLength,
            OutputBuffer,
            OutputBufferLength,
            InternalDeviceIoControl,
            Event,
            IoStatusBlock
            );
    } except(EXCEPTION_EXECUTE_HANDLER) {
         KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                      IO_BUILD_IOCTL_REQUEST_EXCEPTION,
                      (ULONG_PTR)DeviceObject,
                      (ULONG_PTR)IoControlCode,
                      GetExceptionCode());
    }

    return (Irp);
}

NTSTATUS
IovInitializeTimer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_TIMER_ROUTINE TimerRoutine,
    IN PVOID Context
    )
{
    if (DeviceObject->Timer) {
        KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                     IO_REINITIALIZING_TIMER_OBJECT,
                     (ULONG_PTR)DeviceObject,
                     0,
                     0);
   }
   return (IoInitializeTimer(DeviceObject, TimerRoutine, Context));
}


VOID
IovpCompleteRequest(
    IN PKAPC Apc,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    )
{
    PIRP    irp;
    PUCHAR addr;
    ULONG   BestStackOffset;

    irp = CONTAINING_RECORD( Apc, IRP, Tail.Apc );

#if defined(_X86_)


    addr = (PUCHAR)irp->UserIosb;
    if ((addr > (PUCHAR)KeGetCurrentThread()->StackLimit) &&
        (addr <= (PUCHAR)&BestStackOffset)) {
        KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                     IO_INVALID_STACK_IOSB,
                     (ULONG_PTR)addr,
                     0,
                     0);

    }

    addr = (PUCHAR)irp->UserEvent;
    if ((addr > (PUCHAR)KeGetCurrentThread()->StackLimit) &&
        (addr <= (PUCHAR)&BestStackOffset)) {
        KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                     IO_INVALID_STACK_EVENT,
                     (ULONG_PTR)addr,
                     0,
                     0);

    }
#endif
}


/*-------------------------- SPECIALIRP HOOKS -------------------------------*/

VOID
FASTCALL
IovSpecialIrpCompleteRequest(
    IN PIRP Irp,
    IN CCHAR PriorityBoost
    )
/*++

Routine Description:

    The following code is called only when doing IRP tracking. It duplicates code
    in iosubs.c.
    This routine is invoked to complete an I/O request.  It is invoked by the
    driver in its DPC routine to perform the final completion of the IRP.  The
    functions performed by this routine are as follows.

        1.  A check is made to determine whether the packet's stack locations
            have been exhausted.  If not, then the stack location pointer is set
            to the next location and if there is a routine to be invoked, then
            it will be invoked.  This continues until there are either no more
            routines which are interested or the packet runs out of stack.

            If a routine is invoked to complete the packet for a specific driver
            which needs to perform work a lot of work or the work needs to be
            performed in the context of another process, then the routine will
            return an alternate success code of STATUS_MORE_PROCESSING_REQUIRED.
            This indicates that this completion routine should simply return to
            its caller because the operation will be "completed" by this routine
            again sometime in the future.

        2.  A check is made to determine whether this IRP is an associated IRP.
            If it is, then the count on the master IRP is decremented.  If the
            count for the master becomes zero, then the master IRP will be
            completed according to the steps below taken for a normal IRP being
            completed.  If the count is still non-zero, then this IRP (the one
            being completed) will simply be deallocated.

        3.  If this is paging I/O or a close operation, then simply write the
            I/O status block and set the event to the signaled state, and
            dereference the event.  If this is paging I/O, deallocate the IRP
            as well.

        4.  Unlock the pages, if any, specified by the MDL by calling
            MmUnlockPages.

        5.  A check is made to determine whether or not completion of the
            request can be deferred until later.  If it can be, then this
            routine simply exits and leaves it up to the originator of the
            request to fully complete the IRP.  By not initializing and queueing
            the special kernel APC to the calling thread (which is the current
            thread by definition), a lot of interrupt and queueing processing
            can be avoided.


        6.  The final rundown routine is invoked to queue the request packet to
            the target (requesting) thread as a special kernel mode APC.

Arguments:

    Irp - Pointer to the I/O Request Packet to complete.

    PriorityBoost - Supplies the amount of priority boost that is to be given
        to the target thread when the special kernel APC is queued.

Return Value:

    None.

--*/

#define ZeroAndDopeIrpStackLocation( IrpSp ) {  \
    (IrpSp)->MinorFunction = 0;                 \
    (IrpSp)->Flags = 0;                         \
    (IrpSp)->Control = SL_NOTCOPIED;            \
    (IrpSp)->Parameters.Others.Argument1 = 0;   \
    (IrpSp)->Parameters.Others.Argument2 = 0;   \
    (IrpSp)->Parameters.Others.Argument3 = 0;   \
    (IrpSp)->Parameters.Others.Argument4 = 0;   \
    (IrpSp)->FileObject = (PFILE_OBJECT) NULL; }

{
    PIRP masterIrp;
    NTSTATUS status;
    PIO_STACK_LOCATION stackPointer;
    PMDL mdl;
    PETHREAD thread;
    PFILE_OBJECT fileObject;
    KIRQL irql;
    PVOID saveAuxiliaryPointer = NULL;

#ifndef NO_SPECIAL_IRP
    PVOID routine ;
    IOFCOMPLETEREQUEST_STACKDATA completionPacket;
#endif

    if (!IopVerifierOn) {
        IopfCompleteRequest(Irp, PriorityBoost);
        return;
    }

#if DBG

    if (Irp->CurrentLocation <= (CCHAR) Irp->StackCount) {

        stackPointer = IoGetCurrentIrpStackLocation(Irp);
        if (stackPointer->MajorFunction == IRP_MJ_POWER) {
            PoPowerTrace(
                POWERTRACE_COMPLETE,
                IoGetCurrentIrpStackLocation(Irp)->DeviceObject,
                Irp,
                IoGetCurrentIrpStackLocation(Irp)
                );
        }
    }

#endif

    SPECIALIRP_IOF_COMPLETE_1(Irp, PriorityBoost, &completionPacket);

    //
    // Begin by ensuring that this packet has not already been completed
    // by someone.
    //

    if (Irp->CurrentLocation > (CCHAR) (Irp->StackCount + 1) ||
        Irp->Type != IO_TYPE_IRP) {
        KeBugCheckEx( MULTIPLE_IRP_COMPLETE_REQUESTS, (ULONG_PTR) Irp, __LINE__, 0, 0 );
    }

    //
    // Ensure that the packet being completed really is still an IRP.
    //

    if (Irp->Type != IO_TYPE_IRP) {

        KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                     IO_CALL_DRIVER_IRP_TYPE_INVALID,
                     (ULONG_PTR)Irp,
                     0,
                     0);
    }

    //
    // Ensure that no one believes that this request is still in a cancellable
    // state.
    //

    if (Irp->CancelRoutine) {

        ASSERT(Irp->CancelRoutine == NULL);

        KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                     IO_COMPLETE_REQUEST_CANCEL_ROUTINE_SET,
                     (ULONG_PTR)Irp->CancelRoutine,
                     (ULONG_PTR)Irp,
                     0);
    }

    //
    // Ensure that the packet is not being completed with a thoroughly
    // confusing status code.  Actually completing a packet with a pending
    // status probably means that someone forgot to set the real status in
    // the packet.

    if (Irp->IoStatus.Status == STATUS_PENDING) {


         KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                      IO_COMPLETE_REQUEST_INVALID_STATUS,
                      Irp->IoStatus.Status,
                      (ULONG_PTR)Irp,
                      0);
    }

    //
    // Ensure that the packet is not being completed with a minus one.  This is
    // apparently a common problem in some drivers, and has no meaning as a
    // status code.
    //

    if (Irp->IoStatus.Status == 0xffffffff) {


         KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                      IO_COMPLETE_REQUEST_INVALID_STATUS,
                      Irp->IoStatus.Status,
                      (ULONG_PTR)Irp,
                      0);
    }

    //
    // Ensure that if this is a paging I/O operation, and it failed, that the
    // reason for the failure isn't because quota was exceeded.
    //

    ASSERT( !(Irp->Flags & IRP_PAGING_IO && Irp->IoStatus.Status == STATUS_QUOTA_EXCEEDED ) );

#ifndef NO_SPECIAL_IRP

    if (!IovpTrackingFlags) {

        IopfCompleteRequest(Irp, PriorityBoost);
        return;
    }

    //
    // Now check to see whether this is the last driver that needs to be
    // invoked for this packet.  If not, then bump the stack and check to
    // see whether the driver wishes to see the completion.  As each stack
    // location is examined, invoke any routine which needs to be invoked.
    // If the routine returns STATUS_MORE_PROCESSING_REQUIRED, then stop the
    // processing of this packet.
    //

    for (stackPointer = IoGetCurrentIrpStackLocation( Irp ),
         Irp->CurrentLocation++,
         Irp->Tail.Overlay.CurrentStackLocation++;
         Irp->CurrentLocation <= (CCHAR) (Irp->StackCount + 1);
         stackPointer++,
         Irp->CurrentLocation++,
         Irp->Tail.Overlay.CurrentStackLocation++) {

        //
        // A stack location was located.  Check to see whether or not it
        // has a completion routine and if so, whether or not it should be
        // invoked.
        //
        // Begin by saving the pending returned flag in the current stack
        // location in the fixed part of the IRP.
        //

        Irp->PendingReturned = stackPointer->Control & SL_PENDING_RETURNED;

        SPECIALIRP_IOF_COMPLETE_2(Irp, &completionPacket);

        if ( (NT_SUCCESS( Irp->IoStatus.Status ) &&
             stackPointer->Control & SL_INVOKE_ON_SUCCESS) ||
             (!NT_SUCCESS( Irp->IoStatus.Status ) &&
             stackPointer->Control & SL_INVOKE_ON_ERROR) ||
             (Irp->Cancel &&
             stackPointer->Control & SL_INVOKE_ON_CANCEL)
           ) {

            //
            // This driver has specified a completion routine.  Invoke the
            // routine passing it a pointer to its device object and the
            // IRP that is being completed.
            //

            ZeroAndDopeIrpStackLocation( stackPointer );

#ifndef NO_SPECIAL_IRP
            routine = stackPointer->CompletionRoutine ;
            SPECIALIRP_IOF_COMPLETE_3(Irp, routine, &completionPacket);
#endif

            PERFINFO_DRIVER_COMPLETIONROUTINE_CALL(Irp, stackPointer);

            status = stackPointer->CompletionRoutine( (PDEVICE_OBJECT) (Irp->CurrentLocation == (CCHAR) (Irp->StackCount + 1) ?
                                                      (PDEVICE_OBJECT) NULL :
                                                      IoGetCurrentIrpStackLocation( Irp )->DeviceObject),
                                                      Irp,
                                                      stackPointer->Context );

            PERFINFO_DRIVER_COMPLETIONROUTINE_RETURN(Irp, stackPointer);

            SPECIALIRP_IOF_COMPLETE_4(Irp, status, &completionPacket);

            if (status == STATUS_MORE_PROCESSING_REQUIRED) {

                //
                // Note:  Notice that if the driver has returned the above
                //        status value, it may have already DEALLOCATED the
                //        packet!  Therefore, do NOT touch any part of the
                //        IRP in the following code.
                //

                SPECIALIRP_IOF_COMPLETE_5(Irp, &completionPacket);
                return;
            }

        } else {
            if (Irp->PendingReturned && Irp->CurrentLocation <= Irp->StackCount) {
                IoMarkIrpPending( Irp );
            }
            ZeroAndDopeIrpStackLocation( stackPointer );
        }

        SPECIALIRP_IOF_COMPLETE_5(Irp, &completionPacket);
    }

    //
    // Check to see whether this is an associated IRP.  If so, then decrement
    // the count in the master IRP.  If the count is decremented to zero,
    // then complete the master packet as well.
    //

    if (Irp->Flags & IRP_ASSOCIATED_IRP) {
        ULONG count;
        masterIrp = Irp->AssociatedIrp.MasterIrp;
        count = ExInterlockedAddUlong( (PULONG) &masterIrp->AssociatedIrp.IrpCount,
                                       0xffffffff,
                                       &IopDatabaseLock );

        //
        // Deallocate this packet and any MDLs that are associated with it
        // by either doing direct deallocations if they were allocated from
        // a zone or by queueing the packet to a thread to perform the
        // deallocation.
        //
        // Also, check the count of the master IRP to determine whether or not
        // the count has gone to zero.  If not, then simply get out of here.
        // Otherwise, complete the master packet.
        //

        Irp->Tail.Overlay.Thread = masterIrp->Tail.Overlay.Thread;
        IopFreeIrpAndMdls( Irp );
        if (count == 1) {
            IoCompleteRequest( masterIrp, PriorityBoost );
        }
        return;
    }

    //
    // Check to see if we have a name junction. If so set the stage to
    // transmogrify the reparse point data in IopCompleteRequest.
    //

    if ((Irp->IoStatus.Status == STATUS_REPARSE )  &&
        (Irp->IoStatus.Information > IO_REPARSE_TAG_RESERVED_RANGE)) {

        if (Irp->IoStatus.Information == IO_REPARSE_TAG_MOUNT_POINT) {

            //
            // For name junctions, we save the pointer to the auxiliary
            // buffer and use it below.
            //

            ASSERT( Irp->Tail.Overlay.AuxiliaryBuffer != NULL );

            saveAuxiliaryPointer = (PVOID) Irp->Tail.Overlay.AuxiliaryBuffer;

            //
            // We NULL the entry to avoid its de-allocation at this time.
            // This buffer get deallocated in IopDoNameTransmogrify
            //

            Irp->Tail.Overlay.AuxiliaryBuffer = NULL;
        } else {

            //
            // Fail the request. A driver needed to act on this IRP prior
            // to getting to this point.
            //

            Irp->IoStatus.Status = STATUS_IO_REPARSE_TAG_NOT_HANDLED;
        }
    }

    //
    // Check the auxiliary buffer pointer in the packet and if a buffer was
    // allocated, deallocate it now.  Note that this buffer must be freed
    // here since the pointer is overlayed with the APC that will be used
    // to get to the requesting thread's context.
    //

    if (Irp->Tail.Overlay.AuxiliaryBuffer) {
        ExFreePool( Irp->Tail.Overlay.AuxiliaryBuffer );
        Irp->Tail.Overlay.AuxiliaryBuffer = NULL;
    }

    //
    // Check to see if this is paging I/O or a close operation.  If either,
    // then special processing must be performed.  The reasons that special
    // processing must be performed is different based on the type of
    // operation being performed.  The biggest reasons for special processing
    // on paging operations are that using a special kernel APC for an in-
    // page operation cannot work since the special kernel APC can incur
    // another pagefault.  Likewise, all paging I/O uses MDLs that belong
    // to the memory manager, not the I/O system.
    //
    // Close operations are special because the close may have been invoked
    // because of a special kernel APC (some IRP was completed which caused
    // the reference count on the object to become zero while in the I/O
    // system's special kernel APC routine).  Therefore, a special kernel APC
    // cannot be used since it cannot execute until the close APC finishes.
    //
    // The special steps are as follows for a synchronous paging operation
    // and close are:
    //
    //     1.  Copy the I/O status block (it is in SVAS, nonpaged).
    //     2.  Signal the event
    //     3.  If paging I/O, deallocate the IRP
    //
    // The special steps taken for asynchronous paging operations (out-pages)
    // are as follows:
    //
    //     1.  Initialize a special kernel APC just for page writes.
    //     1.  Queue the special kernel APC.
    //
    // It should also be noted that the logic for completing a Mount request
    // operation is exactly the same as a Page Read.  No assumptions should be
    // made here about this being a Page Read operation w/o carefully checking
    // to ensure that they are also true for a Mount.  That is:
    //
    //     IRP_PAGING_IO  and  IRP_MOUNT_COMPLETION
    //
    // are the same flag in the IRP.
    //
    // Also note that the last time the IRP is touched for a close operation
    // must be just before the event is set to the signaled state.  Once this
    // occurs, the IRP can be deallocated by the thread waiting for the event.
    //

    if (Irp->Flags & (IRP_PAGING_IO | IRP_CLOSE_OPERATION)) {
        if (Irp->Flags & (IRP_SYNCHRONOUS_PAGING_IO | IRP_CLOSE_OPERATION)) {
            ULONG flags;

            flags = Irp->Flags & IRP_SYNCHRONOUS_PAGING_IO;
            *Irp->UserIosb = Irp->IoStatus;
            (VOID) KeSetEvent( Irp->UserEvent, PriorityBoost, FALSE );
            if (flags) {
                IoFreeIrp( Irp );
            }
        } else {
            thread = Irp->Tail.Overlay.Thread;
            KeInitializeApc( &Irp->Tail.Apc,
                             &thread->Tcb,
                             Irp->ApcEnvironment,
                             IopCompletePageWrite,
                             (PKRUNDOWN_ROUTINE) NULL,
                             (PKNORMAL_ROUTINE) NULL,
                             KernelMode,
                             (PVOID) NULL );
            (VOID) KeInsertQueueApc( &Irp->Tail.Apc,
                                     (PVOID) NULL,
                                     (PVOID) NULL,
                                     PriorityBoost );
        }
        return;
    }

    //
    // Check to see whether any pages need to be unlocked.
    //

    if (Irp->MdlAddress != NULL) {

        //
        // Unlock any pages that may be described by MDLs.
        //

        mdl = Irp->MdlAddress;
        while (mdl != NULL) {
            MmUnlockPages( mdl );
            mdl = mdl->Next;
        }
    }

    //
    // Make a final check here to determine whether or not this is a
    // synchronous I/O operation that is being completed in the context
    // of the original requestor.  If so, then an optimal path through
    // I/O completion can be taken.
    //

    if (Irp->Flags & IRP_DEFER_IO_COMPLETION && !Irp->PendingReturned) {

        if ((Irp->IoStatus.Status == STATUS_REPARSE )  &&
            (Irp->IoStatus.Information == IO_REPARSE_TAG_MOUNT_POINT)) {

            //
            // For name junctions we reinstate the address of the appropriate
            // buffer. It is freed in parse.c
            //

            Irp->Tail.Overlay.AuxiliaryBuffer = saveAuxiliaryPointer;
        }

        return;
    }

    //
    // Finally, initialize the IRP as an APC structure and queue the special
    // kernel APC to the target thread.
    //

    thread = Irp->Tail.Overlay.Thread;
    fileObject = Irp->Tail.Overlay.OriginalFileObject;

    if (!Irp->Cancel) {

        KeInitializeApc( &Irp->Tail.Apc,
                         &thread->Tcb,
                         Irp->ApcEnvironment,
                         IopCompleteRequest,
                         IopAbortRequest,
                         (PKNORMAL_ROUTINE) NULL,
                         KernelMode,
                         (PVOID) NULL );

        (VOID) KeInsertQueueApc( &Irp->Tail.Apc,
                                 fileObject,
                                 (PVOID) saveAuxiliaryPointer,
                                 PriorityBoost );
    } else {

        //
        // This request has been cancelled.  Ensure that access to the thread
        // is synchronized, otherwise it may go away while attempting to get
        // through the remainder of completion for this request.  This happens
        // when the thread times out waiting for the request to be completed
        // once it has been cancelled.
        //
        // Note that it is safe to capture the thread pointer above, w/o having
        // the lock because the cancel flag was not set at that point, and
        // the code that disassociates IRPs must set the flag before looking to
        // see whether or not the packet has been completed, and this packet
        // will appear to be completed because it no longer belongs to a driver.
        //

        ExAcquireSpinLock( &IopCompletionLock, &irql );

        thread = Irp->Tail.Overlay.Thread;

        if (thread) {

            KeInitializeApc( &Irp->Tail.Apc,
                             &thread->Tcb,
                             Irp->ApcEnvironment,
                             IopCompleteRequest,
                             IopAbortRequest,
                             (PKNORMAL_ROUTINE) NULL,
                             KernelMode,
                             (PVOID) NULL );

            (VOID) KeInsertQueueApc( &Irp->Tail.Apc,
                                     fileObject,
                                     (PVOID) saveAuxiliaryPointer,
                                     PriorityBoost );

            ExReleaseSpinLock( &IopCompletionLock, irql );

        } else {

            //
            // This request has been aborted from completing in the caller's
            // thread.  This can only occur if the packet was cancelled, and
            // the driver did not complete the request, so it was timed out.
            // Attempt to drop things on the floor, since the originating thread
            // has probably exited at this point.
            //

            ExReleaseSpinLock( &IopCompletionLock, irql );

            ASSERT( Irp->Cancel );

            //
            // Drop the IRP on the floor.
            //

            IopDropIrp( Irp, fileObject );

        }
    }
#else

    IopfCompleteRequest(Irp, PriorityBoost);
    return;

#endif
}

NTSTATUS
FASTCALL
IovSpecialIrpCallDriver(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  OUT PIRP    Irp
    )
/*++

Routine Description:

    This routine is invoked to pass an I/O Request Packet (IRP) to another
    driver at its dispatch routine. This routine is called only for IRP tracking
    It duplicates the code in iosubs.c.

Arguments:

    DeviceObject - Pointer to device object to which the IRP should be passed.

    Irp - Pointer to IRP for request.

Return Value:

    Return status from driver's dispatch routine.

--*/

{
    PIO_STACK_LOCATION irpSp;
    PDRIVER_OBJECT driverObject;
    NTSTATUS status;
    KIRQL saveIrql;
    PDRIVER_DISPATCH dispatchRoutine;
    IOFCALLDRIVER_STACKDATA iofCallDriverStackData ;

    if (!IopVerifierOn) {
        status = IopfCallDriver(DeviceObject, Irp);
        return status;
    }

    //
    // Ensure that this is really an I/O Request Packet.
    //

    if (Irp->Type != IO_TYPE_IRP) {

        ASSERT(Irp->Type == IO_TYPE_IRP);
        KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                     IO_CALL_DRIVER_IRP_TYPE_INVALID,
                     (ULONG_PTR)Irp,
                     0,
                     0);
    }

    if (!IovpValidateDeviceObject(DeviceObject)) {
        KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                     IO_CALL_DRIVER_INVALID_DEVICE_OBJECT,
                     (ULONG_PTR)DeviceObject,
                     0,
                     0);
    }

    SPECIALIRP_IOF_CALL_1(&Irp, DeviceObject, &iofCallDriverStackData) ;

    //
    // Update the IRP stack to point to the next location.
    //
    Irp->CurrentLocation--;

    if (Irp->CurrentLocation <= 0) {
        KeBugCheckEx( NO_MORE_IRP_STACK_LOCATIONS, (ULONG_PTR) Irp, 0, 0, 0 );
    }

    irpSp = IoGetNextIrpStackLocation( Irp );
    Irp->Tail.Overlay.CurrentStackLocation = irpSp;

    //
    // Save a pointer to the device object for this request so that it can
    // be used later in completion.
    //

    irpSp->DeviceObject = DeviceObject;

    //
    // Invoke the driver at its dispatch routine entry point.
    //

    driverObject = DeviceObject->DriverObject;

    dispatchRoutine = driverObject->MajorFunction[irpSp->MajorFunction] ;

    saveIrql = KeGetCurrentIrql();

    PERFINFO_DRIVER_MAJORFUNCTION_CALL(Irp, irpSp, driverObject);

    status = dispatchRoutine( DeviceObject, Irp );

    PERFINFO_DRIVER_MAJORFUNCTION_RETURN(Irp, irpSp, driverObject);

    if (saveIrql != KeGetCurrentIrql()) {

        DbgPrint( "IO: IoCallDriver( Driver object: %p  Device object: %p  Irp: %p )\n",
                  driverObject,
                  DeviceObject,
                  Irp
                );
        DbgPrint( "    Irql before: %x  != After: %x\n", saveIrql, KeGetCurrentIrql() );

        ASSERT(saveIrql == KeGetCurrentIrql());

        KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                     IO_CALL_DRIVER_IRQL_NOT_EQUAL,
                     saveIrql,
                     KeGetCurrentIrql(),
                     0);
    }

    SPECIALIRP_IOF_CALL_2(Irp, DeviceObject, dispatchRoutine, &status, &iofCallDriverStackData) ;

    return status;
}

VOID
IovInitializeIrp(
    PIRP    Irp,
    USHORT  PacketSize,
    CCHAR   StackSize
    )
{
    BOOLEAN initializeHandled ;

    if (IovpVerifierLevel < 2) {
        return;
    }

    SPECIALIRP_IO_INITIALIZE_IRP(Irp, PacketSize, StackSize, &initializeHandled) ;

}

VOID
IovAttachDeviceToDeviceStack(
    PDEVICE_OBJECT  SourceDevice,
    PDEVICE_OBJECT  TargetDevice
    )
{
    if (IovpVerifierLevel < 2) {
        return;
    }

    SPECIALIRP_IO_ATTACH_DEVICE_TO_DEVICE_STACK(SourceDevice, TargetDevice);
}

VOID
IovDeleteDevice(
    PDEVICE_OBJECT  DeleteDevice
    )
{
    if (IovpVerifierLevel < 2) {
        return;
    }

    SPECIALIRP_IO_DELETE_DEVICE(DeleteDevice);
}

VOID
IovDetachDevice(
    PDEVICE_OBJECT  TargetDevice
    )
{
    if (IovpVerifierLevel < 2) {
        return;
    }
    SPECIALIRP_IO_DETACH_DEVICE(TargetDevice);
}

BOOLEAN
IovCancelIrp(
    PIRP    Irp,
    BOOLEAN *returnValue
    )
{
#ifndef NO_SPECIAL_IRP
    BOOLEAN cancelHandled ;

    SPECIALIRP_IO_CANCEL_IRP(Irp, &cancelHandled, returnValue) ;

    if (cancelHandled) {

       return TRUE ;
    }
#endif
    return FALSE;
}

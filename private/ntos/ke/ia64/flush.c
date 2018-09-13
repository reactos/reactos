
/*++

Module Name:

    flush.c

Abstract:

    This module implements IA64 machine dependent kernel functions to flush
    the data and instruction caches and to flush I/O buffers.

Author:

    07-Mar-1996
    
    Bernard Lint
    M. Jayakumar (Muthurajan.Jayakumar@intel.com)


Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"
#include "kxia64.h"

//
// PROBE_VISIBILITY_PAL_SUPPORT flag is one time write (RESET) only and multiple time read
// only flag. It is used to check to see if the processor needs PAL_SUPPORT for VISIBILITY // in prefetches. Once the check is made, this flag optimizes such that further checks are // eliminated.
//
 
ULONG ProbePalVisibilitySupport=1;
ULONG NeedPalVisibilitySupport=1;
extern KSPIN_LOCK KiCacheFlushLock;
//
// Define forward referenced prototyes.
//

VOID
KiSweepDcacheTarget (
    IN PULONG SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

VOID
KiSweepIcacheTarget (
    IN PULONG SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

VOID
KiFlushIoBuffersTarget (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Mdl,
    IN PVOID ReadOperation,
    IN PVOID DmaOperation
    );

VOID
KiSyncCacheTarget(
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

ULONG_PTR
KiSyncMC_DrainTarget(
    );


ULONG_PTR
KiSyncMC_Drain(
    IN BOOLEAN AllProcessors,
    IN PVOID BaseAddress,
    IN ULONG Length
    );

ULONG_PTR
KiSyncPrefetchVisibleTarget(
    );

ULONG_PTR
KiSyncPrefetchVisible (
    IN BOOLEAN AllProcessors,
    IN PVOID BaseAddress,
    IN ULONG Length
    );




VOID
KiSyncCacheTarget (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )

/*++
Routine Description:

    This function synchronizes the I-fetch pipeline. Typically this routine will be
    executed by every processor in the system in response to an IPI after the cache
    is flushed. Each processor executing RFI while leaving the IPI produces the
    serialization effect that is required after isync to make sure that further
    instruction prefetches wait till the ISYNC completes.

Arguements:

    SignalDone Supplies a pointer to a variable that is cleared when the
    requested operation has been performed.

    Parameter1 - Parameter3 - Not used.

Return Value:

    Nothing.
--*/
{

#if !defined(NT_UP)

    __synci();
    KiIpiSignalPacketDone(SignalDone);

#endif
    return;

}

VOID
KeSweepIcache (
    IN BOOLEAN AllProcessors
    )

/*++

Routine Description:

    This function flushes the instruction cache on all processors that are
    currently running threads which are children of the current process or
    flushes the instruction cache on all processors in the host configuration.

    N.B. Although PowerPC maintains cache coherency across processors, we
    use the flash invalidate function (h/w) for I-Cache sweeps which doesn't
    maintain coherency so we still do the MP I-Cache flush in s/w.   plj.

Arguments:

    AllProcessors - Supplies a boolean value that determines which instruction
        caches are flushed.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    KAFFINITY TargetProcessors;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

#if !defined(NT_UP)
    // 
    // Acquire cache flush spinlock
    // Cache flush is not MP safe yet
    //
    KeAcquireSpinLock(&KiCacheFlushLock, &OldIrql);

#endif

    HalSweepDcache();
    HalSweepIcache();

#if !defined(NT_UP)

    //
    // Compute the set of target processors and send the sweep parameters
    // to the target processors, if any, for execution.
    //

    TargetProcessors = KeActiveProcessors & PCR->NotMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiSweepIcacheTarget,
                        NULL,
                        NULL,
                        NULL);
    }


    //
    // Wait until all target processors have finished sweeping their
    // instruction caches.
    //


    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    //
    // Lower IRQL to its previous level and return.
    //

    KeReleaseSpinLock(&KiCacheFlushLock, OldIrql);

#endif

    return;
}

VOID
KiSweepIcacheTarget (
    IN PULONG SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for sweeping the instruction cache on
    target processors.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Parameter1 - Parameter3 - Not used.

Return Value:

    None.

--*/

{

    //
    // Sweep the instruction cache on the current processor and clear
    // the sweep instruction cache packet address to signal the source
    // to continue.
    //

#if !defined(NT_UP)

    HalSweepDcache();
    HalSweepIcache();

    KiIpiSignalPacketDone(SignalDone);

#endif

    return;
}

VOID
KeSweepDcache (
    IN BOOLEAN AllProcessors
    )

/*++

Routine Description:

    This function flushes the data cache on all processors that are currently
    running threads which are children of the current process or flushes the
    data cache on all processors in the host configuration.

    N.B. PowerPC maintains cache coherency across processors however
    in this routine, the range of addresses being flushed is unknown
    so we must still broadcast the request to the other processors.

Arguments:

    AllProcessors - Supplies a boolean value that determines which data
        caches are flushed.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    KAFFINITY TargetProcessors;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

#if !defined(NT_UP)
    // 
    // Acquire cache flush spinlock
    // Cache flush is not MP safe yet
    //
    KeAcquireSpinLock(&KiCacheFlushLock, &OldIrql);

#endif

    HalSweepDcache();

#if !defined(NT_UP)

    //
    // Compute the set of target processors and send the sweep parameters
    // to the target processors, if any, for execution.
    //

    TargetProcessors = KeActiveProcessors & PCR->NotMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiSweepDcacheTarget,
                        NULL,
                        NULL,
                        NULL);
    }


    //
    // Wait until all target processors have finished sweeping their
    // data caches.
    //


    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    //
    // Lower IRQL to its previous level and return.
    //

    KeReleaseSpinLock(&KiCacheFlushLock, OldIrql);

#endif

    return;
}

VOID
KiSweepDcacheTarget (
    IN PULONG SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for sweeping the data cache on target
    processors.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Parameter1 - Parameter3 - Not used.

Return Value:

    None.

--*/

{

    //
    // Sweep the data cache on the current processor and clear the sweep
    // data cache packet address to signal the source to continue.
    //

#if !defined(NT_UP)

    HalSweepDcache();
    KiIpiSignalPacketDone(SignalDone);

#endif

    return;
}



ULONG_PTR
KiSyncMC_DrainTarget(
    )

/*++

Routine Description:

    This is the target function for issuing PAL_MC_DRAIN to drain
    prefetches, demand references and pending fc cache line evictions on the
    target CPU it executes.

Argument:

    None


Return Value:

   Returns the status from the function HalCallPal

--*/

{
    ULONG_PTR Status;

    //
    // Call HalCallPal to drain.
    //

    Status = HalCallPal(PAL_MC_DRAIN,
        0,
        0,
        0,
        0,
        0,
        0,
        0);

    ASSERT(Status == PAL_STATUS_SUCCESS);

    return Status;

}


VOID
KeSweepCacheRange (
    IN BOOLEAN AllProcessors,
    IN PVOID BaseAddress,
    IN ULONG Length
    )

/*++

Routine Description:

    This function is used to flush a range of virtual addresses from both the
    instruction and data cache on all processors in the system.

    Irrespective of the length of the range, it should not call SweepIcache
    or SweepDcache. This is because SweepDcache will only sweep D cache and
    not the I cache and Vice versa. Since the caller of KeSweepCacheRange assumes
    both the caches are being swept, one cannot call SweepIcache or SweepDcache
    in trying to optimize.


    Arguments:

    AllProcessors - Not used

    BaseAddress - Supplies a pointer to the base of the range that is flushed.

    Length - Supplies the length of the range that is flushed if the base
        address is specified.

    Return Value:

        None.


--*/

{
     KIRQL OldIrql;
     KAFFINITY TargetProcessors;

    //
    // We will not raise IRQL to synchronization level so that we can allow
    // a context switch in between Flush Cache. FC need not run in the same processor
    // throughout. It can be context switched. So no binding is done to any processor.
    //
    //

    HalSweepCacheRange(BaseAddress,Length);

    ASSERT(KeGetCurrentIrql() <= KiSynchIrql);

    //
    // Raise IRQL to synchronization level to prevent a context switch.
    //

#if !defined(NT_UP)

    OldIrql = KeRaiseIrqlToSynchLevel();

    //
    // Compute the set of target processors and send the sync parameters
    // to the target processors, if any, for execution.
    //

    TargetProcessors = KeActiveProcessors & PCR->NotMember;
    if (TargetProcessors != 0) {
    KiIpiSendPacket(TargetProcessors,
                    KiSyncCacheTarget,
                    NULL,
                    NULL,
                    NULL);
    }

#endif

    //
    // Synchronize the Instruction Prefetch pipe in the local processor.
    //

    __synci();
    __isrlz();

    //
    // Wait until all target processors have finished sweeping the their
    // data cache.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    //
    // Lower IRQL to its previous level and return.
    //

    KeLowerIrql(OldIrql);

#endif

    return;

}

VOID
KeSweepIcacheRange (
    IN BOOLEAN AllProcessors,
    IN PVOID BaseAddress,
    IN ULONG Length
    )

/*++

Routine Description:

    This function is used to flush a range of virtual addresses from the
    primary instruction cache on all processors in the host configuration.

     If the length of the range is greater than the size of the
    instruction cache, then one can call HalSweepIcache which calls
    SAL to flush the entire cache. Since SAL does not take care of MP
    flushing, HalSweepIcache has to use IPI mechanism to execute SAL
    flush from each processor. We need to weight the overhead of all these
    versus using HalSweepIcacheRange and avoiding IPI mechanism since
    HalSweepIcacheRange uses fc instruction and fc instruction takes care of MP.

Arguments:

    AllProcessors -  Not used

    BaseAddress - Supplies a pointer to the base of the range that is flushed.

    Length - Supplies the length of the range that is flushed if the base
        address is specified.

Return Value:

    None.

    Note:  For performance reason, we may update KeSweepIcacheRange to do the following:
           if the range asked to sweep is very large, we may call KeSweepIcache to flush
           the full cache.



--*/

{
    KIRQL OldIrql;
    KAFFINITY TargetProcessors;

    //
    // We will not raise IRQL to synchronization level so that we can allow
    // a context switch in between Flush Cache. FC need not run in the same processor
    // throughout. It can be context switched. So no binding is done to any processor.
    //
    //

    HalSweepIcacheRange(BaseAddress,Length);

    ASSERT(KeGetCurrentIrql() <= KiSynchIrql);

    //
    // Raise IRQL to synchronization level to prevent a context switch.
    //

#if !defined(NT_UP)

    OldIrql = KeRaiseIrqlToSynchLevel();

    //
    // Compute the set of target processors and send the sync parameters
    // to the target processors, if any, for execution.
    //

    TargetProcessors = KeActiveProcessors & PCR->NotMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
            KiSyncCacheTarget,
            NULL,
            NULL,
            NULL);
    }

#endif

    //
    // Synchronize the Instruction Prefetch pipe in the local processor.
    //

    __synci();
    __isrlz();

    //
    // Wait until all target processors have finished sweeping the their
    // data cache.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    //
    // Lower IRQL to its previous level and return.
    //

    KeLowerIrql(OldIrql);

#endif

    return;


}



VOID
KeSweepDcacheRange (
    IN BOOLEAN AllProcessors,
    IN PVOID BaseAddress,
    IN ULONG Length
    )

/*++

Routine Description:

    This function is used to flush a range of virtual addresses from the
    primary data cache on all processors in the host configuration.

     If the length of the range is greater than the size of the
    data cache, then one can call HalSweepDcache which calls
    SAL to flush the entire cache. Since SAL does not take care of MP
    flushing, HalSweepDcache has to use IPI mechanism to execute SAL
    flush from each processor. We need to weight the overhead of all these
    versus using HalSweepDcacheRange and avoiding IPI mechanism since
    HalSweepDcacheRange uses fc instruction and fc instruction takes care of MP.

Arguments:

    AllProcessors -  Not used

    BaseAddress - Supplies a pointer to the base of the range that is flushed.

    Length - Supplies the length of the range that is flushed if the base
        address is specified.

Return Value:

    None.

    Note:  For performance reason, we may update KeSweepDcacheRange to do the following:
           if the range asked to sweep is very large, we may call KeSweepDcache to flush
           the full cache.



--*/

{
    KIRQL OldIrql;
    KAFFINITY TargetProcessors;

    //
    // We will not raise IRQL to synchronization level so that we can allow
    // a context switch in between Flush Cache. FC need not run in the same processor
    // throughout. It can be context switched. So no binding is done to any processor.
    //
    //

    HalSweepDcacheRange(BaseAddress,Length);

    ASSERT(KeGetCurrentIrql() <= KiSynchIrql);

    //
    // Raise IRQL to synchronization level to prevent a context switch.
    //

#if !defined(NT_UP)

    OldIrql = KeRaiseIrqlToSynchLevel();

    //
    // Compute the set of target processors and send the sync parameters
    // to the target processors, if any, for execution.
    //

    TargetProcessors = KeActiveProcessors & PCR->NotMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
            KiSyncCacheTarget,
            NULL,
            NULL,
            NULL);
    }

#endif

    //
    // Synchronize the Instruction Prefetch pipe in the local processor.
    //

    __synci();
    __isrlz();

    //
    // Wait until all target processors have finished sweeping the their
    // data cache.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    //
    // Lower IRQL to its previous level and return.
    //

    KeLowerIrql(OldIrql);

#endif

    return;


}

ULONG_PTR
KiSyncMC_Drain (
    IN BOOLEAN AllProcessors,
    IN PVOID BaseAddress,
    IN ULONG Length
    )

/*++

Routine Description:

    KiSyncMC_Drain issues  PAL_MC_DRAIN to drain either prefetches, demand references
    or pending fc cache line evictions to all the processors in the system.
    DrainTypePointer points to the variable, DrainType, which determines the type of
    drain to be performed. This is typically used when changing the memory attribute
    from WB to UC.

Arguments:

    AllProcessors - All processors in the system.

    BaseAddress - Supplies a pointer to the base of the range that is to be drained.

    Length - Supplies the length of the range that is drained for the base
        address specified.

Return Value:

    Note:  This is used when changing attributes of WB pages to UC pages.


--*/

{
    ULONG_PTR Status;
    //
    // KiIpiGenericCall returns ULONG_PTR as the function value of the specified function
    //

    Status = (KiIpiGenericCall (
                (PKIPI_BROADCAST_WORKER)KiSyncMC_DrainTarget,
                (ULONG_PTR)NULL)
                );

    ASSERT(Status == PAL_STATUS_SUCCESS);

    return Status;


}

ULONG_PTR
KiSyncPrefetchVisibleTarget(
    )

/*++

Routine Description:

    This is the target function for issuing PAL_PREFETCH VISIBILITY 
    on the target CPU it executes.

Argument:

    Not used.


Return Value:

   Returns the status from the function HalCallPal

--*/

{
    ULONG_PTR Status;

    //
    // Call HalCallPal to drain.
    //

    Status = HalCallPal(PAL_PREFETCH_VISIBILITY,
        0,
        0,
        0,
        0,
        0,
        0,
        0);


    ASSERT(Status != PAL_STATUS_ERROR);

    return Status;

}



ULONG_PTR
KiSyncPrefetchVisible (
    IN BOOLEAN AllProcessors,
    IN PVOID BaseAddress,
    IN ULONG Length
    )

/*++

Routine Description:

    KiSyncPrefetchVisible issues  PAL_PREFETCH_VISIBILITY to cause the processor to make
    all pending prefetches visible to subsequent fc instructions; or does nothing, on 
    processor implementations which does not require PAL support for disabling prefetch 
    in the architectural sequence. On processors that require PAL support for this
    sequence, the actions performed by this procedure may include any or all
    of the following (or none, as long as the processor guarantees that 
    prefetches that were issued prior to this call are not resident in the 
    processor's caches after the architected sequence is complete.
    This is typically used when changing the memory attribute from WB to UC.

Arguments:

    AllProcessors - All processors in the system.

    BaseAddress - Supplies a pointer to the base of the range that is to be drained.

    Length - Supplies the length of the range that is drained for the base
        address specified.

Return Value:

    Status of the PAL CALL
      0  Success
      1  Call not needed
      -3 Error returned
    
    Note:  This is used when changing attributes of WB pages to UC pages.


--*/

{
    ULONG_PTR Status;
    
    switch (ProbePalVisibilitySupport) {
        case 0: 
            if (NeedPalVisibilitySupport == 0)
               return PAL_STATUS_SUPPORT_NOT_NEEDED;
            else {
               Status = (KiIpiGenericCall (
                            (PKIPI_BROADCAST_WORKER)KiSyncPrefetchVisibleTarget,
                            (ULONG_PTR)NULL)
                            );
            
                ASSERT(Status != PAL_STATUS_ERROR);
                return Status;
                
            }
            break;

        case 1:
            Status = KiSyncPrefetchVisibleTarget();
   
            ASSERT(Status != PAL_STATUS_ERROR);
   
            ProbePalVisibilitySupport = 0;

            if (Status == PAL_STATUS_SUPPORT_NOT_NEEDED) {
                NeedPalVisibilitySupport = 0;
                return PAL_STATUS_SUPPORT_NOT_NEEDED;
            } else {

                                                
                Status = (KiIpiGenericCall (
                            (PKIPI_BROADCAST_WORKER)KiSyncPrefetchVisibleTarget,
                            (ULONG_PTR)NULL)
                            );
            
                ASSERT(Status != PAL_STATUS_ERROR);
            
                return Status;
                
            }

            break;
        
    }
    

}



VOID
KeSweepCacheRangeWithDrain (
    IN BOOLEAN AllProcessors,
    IN PVOID BaseAddress,
    IN ULONG Length
    )

/*++

Routine Description:

    This function is used to drain prefetches,demand references followed by flushing
    the cache followed by draining pending fc cache line evictions to a specified range
    address in all processors in the system.


Arguments:

    AllProcessors -  All processors in the system.

    BaseAddress - Supplies a pointer to the base of the range that is flushed and drained.

    Length - Supplies the length of the range that is flushed and drained for the base
        address is specified.

Return Value:

    None.

    Note:  This is used when changing attributes of WB pages to UC pages.

--*/

{
    ULONG_PTR Status;

    Status = KiSyncPrefetchVisible(
                 AllProcessors,
                 BaseAddress,
                 Length
                 );

    ASSERT(Status != PAL_STATUS_ERROR);
    

    KeSweepCacheRange (
        AllProcessors,
        BaseAddress,
        Length
        );

    Status = KiSyncMC_Drain (
                 AllProcessors,
                 BaseAddress,
                 Length
                 );

    ASSERT(Status == PAL_STATUS_SUCCESS);

    return;


}


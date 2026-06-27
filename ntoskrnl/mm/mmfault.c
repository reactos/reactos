/*
 * COPYRIGHT:       See COPYING in the top directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/mmfault.c
 * PURPOSE:         Kernel memory management functions
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#include <cache/section/newmm.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include "ARM3/miarm.h"

extern MM_AVL_TABLE MiRosKernelVadRoot;

/* GLOBALS ********************************************************************/

volatile MM_ACCESS_FAULT_WS_PROBE_SNAPSHOT MmpAccessFaultWsProbeSnapshot;
volatile MM_ACCESS_FAULT_WS_PROBE_SNAPSHOT MmpAccessFaultWsProbeSnapshotByCpu
    [MM_ACCESS_FAULT_WS_PROBE_MAXIMUM_PROCESSORS];
volatile MM_WORKING_SET_LOCK_PROBE_SNAPSHOT MmpWorkingSetLockProbeSnapshot;

/* PRIVATE FUNCTIONS **********************************************************/

static
VOID
MmpStoreAccessFaultWsProbe(
    _Out_ volatile MM_ACCESS_FAULT_WS_PROBE_SNAPSHOT *Snapshot,
    _In_ ULONG FaultCode,
    _In_ PVOID Address,
    _In_ KPROCESSOR_MODE Mode,
    _In_opt_ PVOID TrapInformation,
    _In_ PETHREAD Thread,
    _In_opt_ PMMVAD Vad,
    _In_ ULONG Stage,
    _In_ ULONG Cpu,
    _In_ KIRQL Irql,
    _In_ ULONG Count)
{
#ifdef _M_IX86
    PKTRAP_FRAME TrapFrame = (PKTRAP_FRAME)TrapInformation;
#endif

    Snapshot->Magic = MM_ACCESS_FAULT_WS_PROBE_MAGIC;
    Snapshot->Version = 1;
    Snapshot->Count = Count;
    Snapshot->Stage = Stage;
    Snapshot->FaultCode = FaultCode;
    Snapshot->Mode = Mode;
    Snapshot->Cpu = Cpu;
    Snapshot->Irql = Irql;
    Snapshot->Address = (ULONG_PTR)Address;
    Snapshot->TrapInformation = (ULONG_PTR)TrapInformation;
    Snapshot->Thread = (ULONG_PTR)Thread;
    Snapshot->Vad = (ULONG_PTR)Vad;
    Snapshot->KernelAddressSpace = (ULONG_PTR)MmGetKernelAddressSpace();
#ifdef _M_IX86
    Snapshot->TrapEip = TrapFrame ? TrapFrame->Eip : 0;
    Snapshot->TrapSegCs = TrapFrame ? TrapFrame->SegCs : 0;
    Snapshot->TrapEFlags = TrapFrame ? TrapFrame->EFlags : 0;
    Snapshot->TrapErrCode = TrapFrame ? TrapFrame->ErrCode : 0;
    Snapshot->TrapEsp = TrapFrame ? KeGetTrapFrameStackRegister(TrapFrame) : 0;
    Snapshot->TrapTempEsp = TrapFrame ? TrapFrame->TempEsp : 0;
    Snapshot->TrapHardwareEsp = TrapFrame ? TrapFrame->HardwareEsp : 0;
    Snapshot->TrapEbp = TrapFrame ? TrapFrame->Ebp : 0;
    Snapshot->TrapEax = TrapFrame ? TrapFrame->Eax : 0;
    Snapshot->TrapEbx = TrapFrame ? TrapFrame->Ebx : 0;
    Snapshot->TrapEcx = TrapFrame ? TrapFrame->Ecx : 0;
    Snapshot->TrapEdx = TrapFrame ? TrapFrame->Edx : 0;
    Snapshot->TrapEsi = TrapFrame ? TrapFrame->Esi : 0;
    Snapshot->TrapEdi = TrapFrame ? TrapFrame->Edi : 0;
#else
    Snapshot->TrapEip = 0;
    Snapshot->TrapSegCs = 0;
    Snapshot->TrapEFlags = 0;
    Snapshot->TrapErrCode = 0;
    Snapshot->TrapEsp = 0;
    Snapshot->TrapTempEsp = 0;
    Snapshot->TrapHardwareEsp = 0;
    Snapshot->TrapEbp = 0;
    Snapshot->TrapEax = 0;
    Snapshot->TrapEbx = 0;
    Snapshot->TrapEcx = 0;
    Snapshot->TrapEdx = 0;
    Snapshot->TrapEsi = 0;
    Snapshot->TrapEdi = 0;
#endif

    if (Thread != NULL)
    {
        Snapshot->ThreadKernelStack = (ULONG_PTR)Thread->Tcb.KernelStack;
        Snapshot->ThreadInitialStack = (ULONG_PTR)Thread->Tcb.InitialStack;
        Snapshot->ThreadStackLimit = (ULONG_PTR)Thread->Tcb.StackLimit;
        Snapshot->OwnsProcessWorkingSetExclusive =
            Thread->OwnsProcessWorkingSetExclusive;
        Snapshot->OwnsProcessWorkingSetShared =
            Thread->OwnsProcessWorkingSetShared;
        Snapshot->OwnsSystemWorkingSetExclusive =
            Thread->OwnsSystemWorkingSetExclusive;
        Snapshot->OwnsSystemWorkingSetShared =
            Thread->OwnsSystemWorkingSetShared;
        Snapshot->OwnsSessionWorkingSetExclusive =
            Thread->OwnsSessionWorkingSetExclusive;
        Snapshot->OwnsSessionWorkingSetShared =
            Thread->OwnsSessionWorkingSetShared;
    }
    else
    {
        Snapshot->ThreadKernelStack = 0;
        Snapshot->ThreadInitialStack = 0;
        Snapshot->ThreadStackLimit = 0;
        Snapshot->OwnsProcessWorkingSetExclusive = 0;
        Snapshot->OwnsProcessWorkingSetShared = 0;
        Snapshot->OwnsSystemWorkingSetExclusive = 0;
        Snapshot->OwnsSystemWorkingSetShared = 0;
        Snapshot->OwnsSessionWorkingSetExclusive = 0;
        Snapshot->OwnsSessionWorkingSetShared = 0;
    }
}

static
VOID
MmpRecordAccessFaultWsProbe(
    _In_ ULONG FaultCode,
    _In_ PVOID Address,
    _In_ KPROCESSOR_MODE Mode,
    _In_opt_ PVOID TrapInformation,
    _In_ PETHREAD Thread,
    _In_opt_ PMMVAD Vad,
    _In_ ULONG Stage)
{
    ULONG Cpu;
    KIRQL Irql;
    volatile MM_ACCESS_FAULT_WS_PROBE_SNAPSHOT *Snapshot;

    Cpu = KeGetCurrentProcessorNumber();
    Irql = KeGetCurrentIrql();

    MmpStoreAccessFaultWsProbe(&MmpAccessFaultWsProbeSnapshot,
                               FaultCode,
                               Address,
                               Mode,
                               TrapInformation,
                               Thread,
                               Vad,
                               Stage,
                               Cpu,
                               Irql,
                               MmpAccessFaultWsProbeSnapshot.Count + 1);

    if (Cpu < MM_ACCESS_FAULT_WS_PROBE_MAXIMUM_PROCESSORS)
    {
        Snapshot = &MmpAccessFaultWsProbeSnapshotByCpu[Cpu];
        MmpStoreAccessFaultWsProbe(Snapshot,
                                   FaultCode,
                                   Address,
                                   Mode,
                                   TrapInformation,
                                   Thread,
                                   Vad,
                                   Stage,
                                   Cpu,
                                   Irql,
                                   Snapshot->Count + 1);
    }
}

NTSTATUS
NTAPI
MmpAccessFault(KPROCESSOR_MODE Mode,
               ULONG_PTR Address,
               BOOLEAN FromMdl,
               ULONG FaultCode)
{
    PMMSUPPORT AddressSpace;
    MEMORY_AREA* MemoryArea;
    NTSTATUS Status;

    DPRINT("MmAccessFault(Mode %d, Address %x)\n", Mode, Address);

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
    {
        DPRINT1("Page fault at high IRQL was %u\n", KeGetCurrentIrql());
        return(STATUS_UNSUCCESSFUL);
    }

    /* Instruction fetch and the page is present.
       This means the page is NX and we cannot do anything to "fix" it. */
    if (MI_IS_INSTRUCTION_FETCH(FaultCode))
    {
        DPRINT1("Page fault instruction fetch at %p\n", Address);
        return STATUS_ACCESS_VIOLATION;
    }

    /*
     * Find the memory area for the faulting address
     */
    if (Address >= (ULONG_PTR)MmSystemRangeStart)
    {
        /*
         * Check permissions
         */
        if (Mode != KernelMode)
        {
            DPRINT1("MmAccessFault(Mode %d, Address %x)\n", Mode, Address);
            return(STATUS_ACCESS_VIOLATION);
        }
        AddressSpace = MmGetKernelAddressSpace();
    }
    else
    {
        AddressSpace = &PsGetCurrentProcess()->Vm;
    }

    if (!FromMdl)
    {
        MmLockAddressSpace(AddressSpace);
    }
    do
    {
        MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, (PVOID)Address);
        if (MemoryArea == NULL || MemoryArea->DeleteInProgress)
        {
            if (!FromMdl)
            {
                MmUnlockAddressSpace(AddressSpace);
            }
            return (STATUS_ACCESS_VIOLATION);
        }

        switch (MemoryArea->Type)
        {
        case MEMORY_AREA_SECTION_VIEW:
            Status = MmAccessFaultSectionView(AddressSpace,
                                              MemoryArea,
                                              (PVOID)Address,
                                              !FromMdl);
            break;
#ifdef NEWCC
        case MEMORY_AREA_CACHE:
            // This code locks for itself to keep from having to break a lock
            // passed in.
            if (!FromMdl)
                MmUnlockAddressSpace(AddressSpace);
            Status = MmAccessFaultCacheSection(Mode, Address, FromMdl);
            if (!FromMdl)
                MmLockAddressSpace(AddressSpace);
            break;
#endif
        default:
            Status = STATUS_ACCESS_VIOLATION;
            break;
        }
    }
    while (Status == STATUS_MM_RESTART_OPERATION);

    DPRINT("Completed page fault handling\n");
    if (!FromMdl)
    {
        MmUnlockAddressSpace(AddressSpace);
    }
    return(Status);
}

NTSTATUS
NTAPI
MmNotPresentFault(KPROCESSOR_MODE Mode,
                  ULONG_PTR Address,
                  BOOLEAN FromMdl)
{
    PMMSUPPORT AddressSpace;
    MEMORY_AREA* MemoryArea;
    NTSTATUS Status;

    DPRINT("MmNotPresentFault(Mode %d, Address %x)\n", Mode, Address);

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
    {
        DPRINT1("Page fault at high IRQL was %u, address %x\n", KeGetCurrentIrql(), Address);
        return(STATUS_UNSUCCESSFUL);
    }

    /*
     * Find the memory area for the faulting address
     */
    if (Address >= (ULONG_PTR)MmSystemRangeStart)
    {
        /*
         * Check permissions
         */
        if (Mode != KernelMode)
        {
            DPRINT1("Address: %x\n", Address);
            return(STATUS_ACCESS_VIOLATION);
        }
        AddressSpace = MmGetKernelAddressSpace();
    }
    else
    {
        AddressSpace = &PsGetCurrentProcess()->Vm;
    }

    if (!FromMdl)
    {
        MmLockAddressSpace(AddressSpace);
    }

    /*
     * Call the memory area specific fault handler
     */
    do
    {
        MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, (PVOID)Address);
        if (MemoryArea == NULL || MemoryArea->DeleteInProgress)
        {
            if (!FromMdl)
            {
                MmUnlockAddressSpace(AddressSpace);
            }
            return (STATUS_ACCESS_VIOLATION);
        }

        switch (MemoryArea->Type)
        {
        case MEMORY_AREA_SECTION_VIEW:
            Status = MmNotPresentFaultSectionView(AddressSpace,
                                                  MemoryArea,
                                                  (PVOID)Address,
                                                  !FromMdl);
            break;
#ifdef NEWCC
        case MEMORY_AREA_CACHE:
            // This code locks for itself to keep from having to break a lock
            // passed in.
            if (!FromMdl)
                MmUnlockAddressSpace(AddressSpace);
            Status = MmNotPresentFaultCacheSection(Mode, Address, FromMdl);
            if (!FromMdl)
                MmLockAddressSpace(AddressSpace);
            break;
#endif
        default:
            Status = STATUS_ACCESS_VIOLATION;
            break;
        }
    }
    while (Status == STATUS_MM_RESTART_OPERATION);

    DPRINT("Completed page fault handling\n");
    if (!FromMdl)
    {
        MmUnlockAddressSpace(AddressSpace);
    }
    return(Status);
}

extern BOOLEAN Mmi386MakeKernelPageTableGlobal(PVOID Address);

VOID
NTAPI
MmRebalanceMemoryConsumersAndWait(VOID);

NTSTATUS
NTAPI
MmAccessFault(IN ULONG FaultCode,
              IN PVOID Address,
              IN KPROCESSOR_MODE Mode,
              IN PVOID TrapInformation)
{
    PMMVAD Vad = NULL;
    NTSTATUS Status;
    BOOLEAN IsArm3Fault = FALSE;
    PETHREAD CurrentThread;
    BOOLEAN SystemWorkingSetLocked;

    /* Cute little hack for ROS */
    if ((ULONG_PTR)Address >= (ULONG_PTR)MmSystemRangeStart)
    {
#ifdef _M_IX86
        /* Check for an invalid page directory in kernel mode */
        if (Mmi386MakeKernelPageTableGlobal(Address))
        {
            /* All is well with the world */
            return STATUS_SUCCESS;
        }
#endif
    }

    /* Handle shared user page / page table, which don't have a VAD / MemoryArea */
    if ((PAGE_ALIGN(Address) == (PVOID)MM_SHARED_USER_DATA_VA) ||
        MI_IS_PAGE_TABLE_ADDRESS(Address))
    {
        /* This is an ARM3 fault */
        DPRINT("ARM3 fault %p\n", Address);
        return MmArmAccessFault(FaultCode, Address, Mode, TrapInformation);
    }

    /* Is there a ReactOS address space yet? */
    if (MmGetKernelAddressSpace())
    {
        if (Address > MM_HIGHEST_USER_ADDRESS)
        {
            /* Check if this is an ARM3 memory area */
            CurrentThread = PsGetCurrentThread();
            MmpRecordAccessFaultWsProbe(FaultCode,
                                        Address,
                                        Mode,
                                        TrapInformation,
                                        CurrentThread,
                                        NULL,
                                        1);
            SystemWorkingSetLocked = FALSE;
            if (!CurrentThread->OwnsSystemWorkingSetExclusive &&
                !CurrentThread->OwnsSystemWorkingSetShared)
            {
#if DBG && defined(_M_IX86)
                if (KeGetCurrentIrql() > APC_LEVEL)
                {
                    PKTRAP_FRAME TrapFrame = (PKTRAP_FRAME)TrapInformation;

                    KeBugCheckEx(IRQL_NOT_LESS_OR_EQUAL,
                                 (ULONG_PTR)Address,
                                 KeGetCurrentIrql(),
                                 FaultCode,
                                 TrapFrame ? TrapFrame->Eip : 0);
                }
#endif
                MiLockWorkingSetShared(CurrentThread, &MmSystemCacheWs);
                SystemWorkingSetLocked = TRUE;
            }

            Vad = MiLocateVad(&MiRosKernelVadRoot, Address);
            MmpRecordAccessFaultWsProbe(FaultCode,
                                        Address,
                                        Mode,
                                        TrapInformation,
                                        CurrentThread,
                                        Vad,
                                        2);

            if ((Vad != NULL) && !MI_IS_ROSMM_VAD(Vad))
            {
                IsArm3Fault = TRUE;
            }

            if (SystemWorkingSetLocked)
            {
                MiUnlockWorkingSetShared(CurrentThread, &MmSystemCacheWs);
            }

            MmpRecordAccessFaultWsProbe(FaultCode,
                                        Address,
                                        Mode,
                                        TrapInformation,
                                        CurrentThread,
                                        Vad,
                                        3);
        }
        else
        {
            /* Could this be a VAD fault from user-mode? */
            CurrentThread = PsGetCurrentThread();
            MmpRecordAccessFaultWsProbe(FaultCode,
                                        Address,
                                        Mode,
                                        TrapInformation,
                                        CurrentThread,
                                        NULL,
                                        4);
#if DBG && defined(_M_IX86)
            if (KeGetCurrentIrql() > APC_LEVEL)
            {
                PKTRAP_FRAME TrapFrame = (PKTRAP_FRAME)TrapInformation;

                KeBugCheckEx(IRQL_NOT_LESS_OR_EQUAL,
                             (ULONG_PTR)Address,
                             KeGetCurrentIrql(),
                             FaultCode,
                             TrapFrame ? TrapFrame->Eip : 0);
            }
#endif
            MiLockProcessWorkingSetShared(PsGetCurrentProcess(), PsGetCurrentThread());
            Vad = MiLocateVad(&PsGetCurrentProcess()->VadRoot, Address);

            if ((Vad != NULL) && !MI_IS_ROSMM_VAD(Vad))
            {
                IsArm3Fault = TRUE;
            }

            MiUnlockProcessWorkingSetShared(PsGetCurrentProcess(), PsGetCurrentThread());
        }
    }

    /* Is this an ARM3 VAD, or is there no address space yet? */
    if (IsArm3Fault ||
        ((Vad == NULL) &&
         ((ULONG_PTR)Address >= (ULONG_PTR)MmPagedPoolStart) &&
         ((ULONG_PTR)Address < (ULONG_PTR)MmPagedPoolEnd)) ||
        (!MmGetKernelAddressSpace()))
    {
        /* This is an ARM3 fault */
        DPRINT("ARM3 fault %p\n", Vad);
        return MmArmAccessFault(FaultCode, Address, Mode, TrapInformation);
    }

Retry:
    /* Keep same old ReactOS Behaviour */
    if (!MI_IS_NOT_PRESENT_FAULT(FaultCode))
    {
        /* Call access fault */
        Status = MmpAccessFault(Mode, (ULONG_PTR)Address, TrapInformation ? FALSE : TRUE, FaultCode);
    }
    else
    {
        /* Call not present */
        Status = MmNotPresentFault(Mode, (ULONG_PTR)Address, TrapInformation ? FALSE : TRUE);
    }

    if (Status == STATUS_NO_MEMORY)
    {
        MmRebalanceMemoryConsumersAndWait();
        goto Retry;
    }

    return Status;
}

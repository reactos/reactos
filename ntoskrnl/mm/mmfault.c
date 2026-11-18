/*
 * PROJECT:         ReactOS kernel
 * LICENSE:         GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:            ntoskrnl/mm/mmfault.c
 * PURPOSE:         Kernel memory management functions
 * COPYRIGHT:       See COPYING in the top directory
 *                  Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 *                  Ryan Wilson <ryan@verosnt.com>
 */

/*
 * High-IRQL user-VA fault print: output/vmxnet3-20261118T021905Z.log
 * (MM FAULT irql 2 va 0000000000000080) after NDIS SENDDONE queued.
 * Trap rip/rsp: output/vmxnet3-20261118T023051Z.log
 * (rip FFFFF88070394827 = vmxnet3.sys+0xC827 at FFFFF88070388000).
 * RSP walk: output/vmxnet3-20261118T023816Z.log (rsp[7] = failing NBL).
 * GPR print targets the null base for va 0x80 (KTRAP_FRAME in
 * sdk/include/ndk/amd64/ketypes.h).
 * Null-page at PASSIVE: output/e1000e-20261118T041602Z.log hit
 * Memory at 0x0 / e1q62x64.sys:2aad8 after SHMEMOK with no MM FAULT
 * gpr lines (InitializeEx is PASSIVE; prior dump required IRQL >
 * APC_LEVEL). Dump the same trap fields for Address < PAGE_SIZE at
 * any IRQL.
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#include <cache/section/newmm.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include "ARM3/miarm.h"

extern MM_AVL_TABLE MiRosKernelVadRoot;

/* PRIVATE FUNCTIONS **********************************************************/

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
            MiLockWorkingSetShared(PsGetCurrentThread(), &MmSystemCacheWs);
            Vad = MiLocateVad(&MiRosKernelVadRoot, Address);

            if ((Vad != NULL) && !MI_IS_ROSMM_VAD(Vad))
            {
                IsArm3Fault = TRUE;
            }

            MiUnlockWorkingSetShared(PsGetCurrentThread(), &MmSystemCacheWs);
        }
        else
        {
            /* Could this be a VAD fault from user-mode? */
            /* output/vmxnet3-20261118T021905Z.log / 022441Z.log: after
             * NDIS SENDDONE queued, MM FAULT irql 2 va 0x80 then APC_LEVEL
             * in miarm.h. Print trap PC and a short stack before the lock.
             * output/e1000e-20261118T041602Z.log: same null-page path at
             * PASSIVE after SHMEMOK; dump when Address < PAGE_SIZE too. */
            if ((KeGetCurrentIrql() > APC_LEVEL) ||
                ((ULONG_PTR)Address < PAGE_SIZE))
            {
                ULONG i;
                ULONG_PTR Pc = 0;
                ULONG_PTR Sp = 0;
                PKTRAP_FRAME TrapFrame = (PKTRAP_FRAME)TrapInformation;

                if (TrapInformation)
                {
                    Pc = KeGetTrapFramePc(TrapInformation);
                    Sp = KeGetTrapFrameStackRegister(TrapInformation);
                }
                DbgPrint("MM FAULT irql %u va %p mode %u rip %p rsp %p\n",
                         KeGetCurrentIrql(),
                         Address,
                         (ULONG)Mode,
                         (PVOID)Pc,
                         (PVOID)Sp);
                if (TrapFrame)
                {
                    /* sdk/include/ndk/amd64/ketypes.h KTRAP_FRAME */
                    DbgPrint("MM FAULT gpr rax %p rcx %p rdx %p\n",
                             (PVOID)TrapFrame->Rax,
                             (PVOID)TrapFrame->Rcx,
                             (PVOID)TrapFrame->Rdx);
                    DbgPrint("MM FAULT gpr r8 %p r9 %p r10 %p r11 %p\n",
                             (PVOID)TrapFrame->R8,
                             (PVOID)TrapFrame->R9,
                             (PVOID)TrapFrame->R10,
                             (PVOID)TrapFrame->R11);
                    DbgPrint("MM FAULT gpr rbx %p rsi %p rdi %p rbp %p\n",
                             (PVOID)TrapFrame->Rbx,
                             (PVOID)TrapFrame->Rsi,
                             (PVOID)TrapFrame->Rdi,
                             (PVOID)TrapFrame->Rbp);
                }
                /* RtlCaptureStackBackTrace printed nothing at DISPATCH on
                 * output/vmxnet3-20261118T023051Z.log. Walk trap RSP slots.
                 * output/vmxnet3-20261118T023816Z.log: rsp[7] matched NBL. */
                if (Sp != 0)
                {
                    for (i = 0; i < 12; i++)
                    {
                        ULONG_PTR Slot;

                        Slot = *(PULONG_PTR)(Sp + (i * sizeof(ULONG_PTR)));
                        DbgPrint("MM FAULT rsp[%u] %p\n", i, (PVOID)Slot);
                    }
                }
            }
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


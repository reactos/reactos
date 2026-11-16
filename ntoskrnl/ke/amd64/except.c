/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        ntoskrnl/ke/amd64/except.c
 * PURPOSE:     Exception Dispatching for amd64
 * COPYRIGHT:   See COPYING in the top level directory
 *              Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Timo Kreuzer (timo.kreuzer@reactos.org)
 *              Alex Ionescu (alex.ionescu@reactos.org)
 *              Ryan Wilson <ryan@verosnt.com>
 *
 * User AV lines name the loaded module+RVA and trap registers.
 * Guest serial output/ntdll-apps/serial-App-20261116T075237Z.log:
 * xul.dll +0x18f2c22 bytes were int 29 / ud2 (__fastfail) — wire IDT
 * 0x29 to KiRaiseSecurityCheckFailure instead of unexpected→AV(-1).
 * serial-App-20261116T094020Z.log: KiPrintUserQwordFields(rbx=4) took
 * KMODE_EXCEPTION_NOT_HANDLED; reject user pointers below 64K before probe.
 * serial-App-20261116T094615Z.log: same xul+0x19dde7 AV with rsi=3; probe
 * rax/rcx/rbp and rsp slots when rbp is not a frame pointer.
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

extern KI_INTERRUPT_DISPATCH_ENTRY KiUnexpectedRange[256];

/* GLOBALS *******************************************************************/

KIDT_INIT KiInterruptInitTable[] =
{
  /* Id,   Dpl,  IST,  ServiceRoutine */
    {0x00, 0x00, 0x00, KiDivideErrorFault},
    {0x01, 0x00, 0x00, KiDebugTrapOrFault},
    {0x02, 0x00, 0x03, KiNmiInterrupt},
    {0x03, 0x03, 0x00, KiBreakpointTrap},
    {0x04, 0x03, 0x00, KiOverflowTrap},
    {0x05, 0x00, 0x00, KiBoundFault},
    {0x06, 0x00, 0x00, KiInvalidOpcodeFault},
    {0x07, 0x00, 0x00, KiNpxNotAvailableFault},
    {0x08, 0x00, 0x01, KiDoubleFaultAbort},
    {0x09, 0x00, 0x00, KiNpxSegmentOverrunAbort},
    {0x0A, 0x00, 0x00, KiInvalidTssFault},
    {0x0B, 0x00, 0x00, KiSegmentNotPresentFault},
    {0x0C, 0x00, 0x00, KiStackFault},
    {0x0D, 0x00, 0x00, KiGeneralProtectionFault},
    {0x0E, 0x00, 0x00, KiPageFault},
    {0x10, 0x00, 0x00, KiFloatingErrorFault},
    {0x11, 0x00, 0x00, KiAlignmentFault},
    {0x12, 0x00, 0x02, KiMcheckAbort},
    {0x13, 0x00, 0x00, KiXmmException},
    {0x1F, 0x00, 0x00, KiApcInterrupt},
    /*
     * INT 29: __fastfail (serial-App-20261116T075237Z.log: cd 29 0f 0b).
     * Same IDT slot as i386 ke/i386/trap.s KiRaiseSecurityCheckFailure.
     */
    {0x29, 0x03, 0x00, KiRaiseSecurityCheckFailure},
    {0x2C, 0x03, 0x00, KiRaiseAssertion},
    {0x2D, 0x03, 0x00, KiDebugServiceTrap},
    {0x2F, 0x00, 0x00, KiDpcInterrupt},
    {0xE1, 0x00, 0x00, KiIpiInterrupt},
    {0, 0, 0, 0}
};

KIDTENTRY64 KiIdt[256];
KDESCRIPTOR KiIdtDescriptor = {{0}, sizeof(KiIdt) - 1, KiIdt};


/* FUNCTIONS *****************************************************************/

CODE_SEG("INIT")
VOID
NTAPI
KeInitExceptions(VOID)
{
    int i, j;

    /* Initialize the Idt */
    for (j = i = 0; i < 256; i++)
    {
        ULONG64 Offset;

        if (KiInterruptInitTable[j].InterruptId == i)
        {
            Offset = (ULONG64)KiInterruptInitTable[j].ServiceRoutine;
            KiIdt[i].Dpl = KiInterruptInitTable[j].Dpl;
            KiIdt[i].IstIndex = KiInterruptInitTable[j].IstIndex;
            j++;
        }
        else
        {
            Offset = (ULONG64)&KiUnexpectedRange[i]._Op_push;
            KiIdt[i].Dpl = 0;
            KiIdt[i].IstIndex = 0;
        }
        KiIdt[i].OffsetLow = Offset & 0xffff;
        KiIdt[i].Selector = KGDT64_R0_CODE;
        KiIdt[i].Type = 0x0e;
        KiIdt[i].Reserved0 = 0;
        KiIdt[i].Present = 1;
        KiIdt[i].OffsetMiddle = (Offset >> 16) & 0xffff;
        KiIdt[i].OffsetHigh = (Offset >> 32);
        KiIdt[i].Reserved1 = 0;
    }

    KeGetPcr()->IdtBase = KiIdt;
    __lidt(&KiIdtDescriptor.Limit);
}

static
BOOLEAN
KiDispatchExceptionToUser(
    IN PKTRAP_FRAME TrapFrame,
    IN PCONTEXT Context,
    IN PEXCEPTION_RECORD ExceptionRecord)
{
    EXCEPTION_RECORD LocalExceptRecord;
    ULONG64 UserRsp;
    PKUSER_EXCEPTION_STACK UserStack;

    /* Make sure we have a valid SS */
    if (TrapFrame->SegSs != (KGDT64_R3_DATA | RPL_MASK))
    {
        /* Raise an access violation instead */
        LocalExceptRecord.ExceptionCode = STATUS_ACCESS_VIOLATION;
        LocalExceptRecord.ExceptionFlags = 0;
        LocalExceptRecord.NumberParameters = 0;
        ExceptionRecord = &LocalExceptRecord;
    }

    /* Get new stack pointer and align it to 16 bytes */
    UserRsp = (Context->Rsp - sizeof(KUSER_EXCEPTION_STACK)) & ~15;

    /* Get pointer to the usermode context, exception record and machine frame */
    UserStack = (PKUSER_EXCEPTION_STACK)UserRsp;

    /* Enable interrupts */
    _enable();

    /* Set up the user-stack */
    _SEH2_TRY
    {
        /* Probe the user stack frame and zero it out */
        ProbeForWrite(UserStack, sizeof(*UserStack), TYPE_ALIGNMENT(KUSER_EXCEPTION_STACK));
        RtlZeroMemory(UserStack, sizeof(*UserStack));

        /* Copy Context and ExceptionFrame */
        UserStack->Context = *Context;
        UserStack->ExceptionRecord = *ExceptionRecord;

        /* Setup the machine frame */
        UserStack->MachineFrame.Rip = Context->Rip;
        UserStack->MachineFrame.SegCs = Context->SegCs;
        UserStack->MachineFrame.EFlags = Context->EFlags;
        UserStack->MachineFrame.Rsp = Context->Rsp;
        UserStack->MachineFrame.SegSs = Context->SegSs;
    }
    _SEH2_EXCEPT((LocalExceptRecord = *_SEH2_GetExceptionInformation()->ExceptionRecord),
                 EXCEPTION_EXECUTE_HANDLER)
    {
        // FIXME: handle stack overflow

        /* Nothing we can do here */
        _disable();
        return FALSE;
    }
    _SEH2_END;

    /* Now set the two params for the user-mode dispatcher */
    TrapFrame->Rcx = (ULONG64)&UserStack->ExceptionRecord;
    TrapFrame->Rdx = (ULONG64)&UserStack->Context;

    /* Set new Stack Pointer */
    TrapFrame->Rsp = UserRsp;

    /* Force correct segments */
    TrapFrame->SegCs = KGDT64_R3_CODE | RPL_MASK;
    TrapFrame->SegDs = KGDT64_R3_DATA | RPL_MASK;
    TrapFrame->SegEs = KGDT64_R3_DATA | RPL_MASK;
    TrapFrame->SegFs = KGDT64_R3_CMTEB | RPL_MASK;
    TrapFrame->SegGs = KGDT64_R3_DATA | RPL_MASK;
    TrapFrame->SegSs = KGDT64_R3_DATA | RPL_MASK;

    /* Set RIP to the User-mode Dispatcher */
    TrapFrame->Rip = (ULONG64)KeUserExceptionDispatcher;

    _disable();

    /* Exit to usermode */
    return TRUE;
}

static
VOID
KiPageInDirectory(PVOID ImageBase, USHORT Directory)
{
    volatile CHAR *Pointer;
    ULONG Size;

   /* Get a pointer to the debug directory */
    Pointer = RtlImageDirectoryEntryToData(ImageBase, 1, Directory, &Size);
    if (!Pointer) return;

    /* Loop all pages */
    while ((LONG)Size > 0)
    {
        /* Touch it, to page it in */
        (void)*Pointer;
        Pointer += PAGE_SIZE;
        Size -= PAGE_SIZE;
    }
}

VOID
KiPrepareUserDebugData(void)
{
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PPEB_LDR_DATA PebLdr;
    PLIST_ENTRY ListEntry;
    PTEB Teb;

    /* Get the Teb for this process */
    Teb = KeGetCurrentThread()->Teb;
    if (!Teb) return;

    /* Enable interrupts */
    _enable();

    _SEH2_TRY
    {
        /* Get a pointer to the loader data */
        PebLdr = Teb->ProcessEnvironmentBlock->Ldr;
        if (!PebLdr) _SEH2_LEAVE;

        /* Now loop all entries in the module list */
        for (ListEntry = PebLdr->InLoadOrderModuleList.Flink;
             ListEntry != &PebLdr->InLoadOrderModuleList;
             ListEntry = ListEntry->Flink)
        {
            /* Get the loader entry */
            LdrEntry = CONTAINING_RECORD(ListEntry,
                                         LDR_DATA_TABLE_ENTRY,
                                         InLoadOrderLinks);

            KiPageInDirectory((PVOID)LdrEntry->DllBase,
                              IMAGE_DIRECTORY_ENTRY_DEBUG);

            KiPageInDirectory((PVOID)LdrEntry->DllBase,
                              IMAGE_DIRECTORY_ENTRY_EXCEPTION);
        }

    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    _disable();
}

/*
 * Name the user module that contains Address (firefox silent-fail12).
 * Same PEB LDR walk shape as KiPrepareUserDebugData above.
 * Tag names which register or fault site the address came from
 * (silent-fail15: map rbx/rsi/rdi).
 */
static
VOID
KiPrintUserModuleForAddress(
    _In_ PVOID Address,
    _In_ PCSTR Tag)
{
    PTEB Teb;
    PPEB_LDR_DATA PebLdr;
    PLIST_ENTRY ListEntry;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    ULONG_PTR Addr = (ULONG_PTR)Address;

    if (!Address || Addr == (ULONG_PTR)-1)
    {
        return;
    }

    Teb = KeGetCurrentThread()->Teb;
    if (!Teb) return;

    _enable();

    _SEH2_TRY
    {
        if (!Teb->ProcessEnvironmentBlock) _SEH2_LEAVE;
        PebLdr = Teb->ProcessEnvironmentBlock->Ldr;
        if (!PebLdr) _SEH2_LEAVE;

        for (ListEntry = PebLdr->InLoadOrderModuleList.Flink;
             ListEntry != &PebLdr->InLoadOrderModuleList;
             ListEntry = ListEntry->Flink)
        {
            LdrEntry = CONTAINING_RECORD(ListEntry,
                                         LDR_DATA_TABLE_ENTRY,
                                         InLoadOrderLinks);
            if (Addr >= (ULONG_PTR)LdrEntry->DllBase &&
                Addr < ((ULONG_PTR)LdrEntry->DllBase + LdrEntry->SizeOfImage))
            {
                DPRINT1("User AV %s module %wZ +0x%Ix (base %p)\n",
                        Tag,
                        &LdrEntry->BaseDllName,
                        Addr - (ULONG_PTR)LdrEntry->DllBase,
                        LdrEntry->DllBase);
                _SEH2_LEAVE;
            }
        }

        DPRINT1("User AV %s address %p not in any loaded module\n",
                Tag,
                Address);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("User AV %s module walk failed\n", Tag);
    }
    _SEH2_END;

    _disable();
}

/*
 * silent-fail15/16: print four qwords at Base+0..+0x18 (heap objects).
 * Same SEH+ProbeForRead shape as the GP fault decoder below; Clang may
 * not catch a bad probe.
 */
static
VOID
KiPrintUserQwordFields(
    _In_ PVOID Base,
    _In_ PCSTR Tag)
{
    ULONG_PTR Values[4];
    ULONG i;

    /*
     * serial-App-20261116T094020Z.log: rbx=4 / rsi=3 passed the old
     * null/-1 check; ProbeForRead then faulted and Clang SEH did not
     * catch it (KMODE_EXCEPTION_NOT_HANDLED at KiPrintUserQwordFields).
     * Reject the low 64K the same way KiPrintUserFollowedObject does.
     */
    if (!Base ||
        (ULONG_PTR)Base < 0x10000ULL ||
        (ULONG_PTR)Base == (ULONG_PTR)-1 ||
        (ULONG_PTR)Base > ((ULONG_PTR)MmUserProbeAddress - 0x1F))
    {
        DPRINT1("User AV %s [%p] skip field probe\n", Tag, Base);
        return;
    }

    _enable();

    _SEH2_TRY
    {
        ProbeForRead(Base, 0x20, 1);
        for (i = 0; i < 4; i++)
        {
            Values[i] = ((volatile ULONG_PTR *)Base)[i];
        }
        for (i = 0; i < 4; i++)
        {
            DPRINT1("User AV %s+0x%x [%p] qword %p\n",
                    Tag,
                    i * 8,
                    (PVOID)((ULONG_PTR)Base + (i * 8)),
                    (PVOID)Values[i]);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("User AV %s [%p] field probe failed 0x%08lx\n",
                Tag,
                Base,
                _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    _disable();
}

/*
 * silent-fail17: rsi+0x8 / +0x10 held other heap pointers while no
 * probed qword was -1 (serial-App-20261116T073710Z.log). Follow one
 * child object and dump its first four qwords.
 */
static
VOID
KiPrintUserFollowedObject(
    _In_ PVOID Parent,
    _In_ ULONG Offset,
    _In_ PCSTR Tag)
{
    ULONG_PTR Child = 0;
    PVOID Field;

    if (!Parent ||
        (ULONG_PTR)Parent < 0x10000ULL ||
        (ULONG_PTR)Parent == (ULONG_PTR)-1 ||
        (ULONG_PTR)Parent > ((ULONG_PTR)MmUserProbeAddress - Offset - 8))
    {
        DPRINT1("User AV %s [%p+0x%lx] skip follow\n",
                Tag,
                Parent,
                Offset);
        return;
    }

    Field = (PVOID)((ULONG_PTR)Parent + Offset);

    _enable();

    _SEH2_TRY
    {
        ProbeForRead(Field, sizeof(ULONG_PTR), 1);
        Child = *(volatile ULONG_PTR *)Field;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("User AV %s [%p+0x%lx] follow read failed 0x%08lx\n",
                Tag,
                Parent,
                Offset,
                _SEH2_GetExceptionCode());
        Child = 0;
    }
    _SEH2_END;

    _disable();

    DPRINT1("User AV %s [%p+0x%lx] child %p\n",
            Tag,
            Parent,
            Offset,
            (PVOID)Child);

    if (!Child ||
        Child == (ULONG_PTR)-1 ||
        Child == (ULONG_PTR)-2 ||
        Child < 0x10000ULL ||
        Child > (ULONG_PTR)MmUserProbeAddress)
    {
        return;
    }

    KiPrintUserModuleForAddress((PVOID)Child, Tag);
    KiPrintUserQwordFields((PVOID)Child, Tag);
}

/*
 * silent-fail18: rsi10+0 / +0x18 pointed at further heap objects
 * (serial-App-20261116T074132Z.log). Walk Parent[Off1][Off2].
 */
static
VOID
KiPrintUserFollowedObject2(
    _In_ PVOID Parent,
    _In_ ULONG Off1,
    _In_ ULONG Off2,
    _In_ PCSTR Tag)
{
    ULONG_PTR Mid = 0;
    PVOID Field;

    if (!Parent ||
        (ULONG_PTR)Parent < 0x10000ULL ||
        (ULONG_PTR)Parent == (ULONG_PTR)-1 ||
        (ULONG_PTR)Parent > ((ULONG_PTR)MmUserProbeAddress - Off1 - 8))
    {
        DPRINT1("User AV %s [%p+0x%lx] skip follow2\n",
                Tag,
                Parent,
                Off1);
        return;
    }

    Field = (PVOID)((ULONG_PTR)Parent + Off1);

    _enable();

    _SEH2_TRY
    {
        ProbeForRead(Field, sizeof(ULONG_PTR), 1);
        Mid = *(volatile ULONG_PTR *)Field;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("User AV %s [%p+0x%lx] follow2 read failed 0x%08lx\n",
                Tag,
                Parent,
                Off1,
                _SEH2_GetExceptionCode());
        Mid = 0;
    }
    _SEH2_END;

    _disable();

    if (!Mid ||
        Mid == (ULONG_PTR)-1 ||
        Mid == (ULONG_PTR)-2 ||
        Mid < 0x10000ULL ||
        Mid > (ULONG_PTR)MmUserProbeAddress)
    {
        DPRINT1("User AV %s mid %p skip\n", Tag, (PVOID)Mid);
        return;
    }

    KiPrintUserFollowedObject((PVOID)Mid, Off2, Tag);
}

/*
 * silent-fail19: -1 never sat in probed GPRs/stack/objects; print the
 * faulting xul instruction bytes (firefox, not a Microsoft binary)
 * (serial-App-20261116T074754Z.log).
 */
static
VOID
KiPrintUserInsnBytes(
    _In_ PVOID Address)
{
    UCHAR Bytes[16];

    if (!Address ||
        (ULONG_PTR)Address == (ULONG_PTR)-1 ||
        (ULONG_PTR)Address > ((ULONG_PTR)MmUserProbeAddress - 16))
    {
        DPRINT1("User AV insn [%p] skip\n", Address);
        return;
    }

    _enable();

    _SEH2_TRY
    {
        ProbeForRead(Address, sizeof(Bytes), 1);
        RtlCopyMemory(Bytes, Address, sizeof(Bytes));
        DPRINT1("User AV insn %02x %02x %02x %02x %02x %02x %02x %02x "
                "%02x %02x %02x %02x %02x %02x %02x %02x\n",
                Bytes[0], Bytes[1], Bytes[2], Bytes[3],
                Bytes[4], Bytes[5], Bytes[6], Bytes[7],
                Bytes[8], Bytes[9], Bytes[10], Bytes[11],
                Bytes[12], Bytes[13], Bytes[14], Bytes[15]);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("User AV insn [%p] probe failed 0x%08lx\n",
                Address,
                _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    _disable();
}

/*
 * silent-fail17: [rbp-0x08] was -2 every AV; exact -1 was missing.
 * Walk rsp..rbp+0x20 and print only -1 / -2 slots.
 * silent-fail29: rbp matched r12/r14 heap (0xA203310) with rsp far
 * below it, so the old span check skipped the walk entirely
 * (serial-App-20261116T094615Z.log). Fall back to rsp..rsp+0x200.
 */
static
VOID
KiPrintUserStackSentinels(
    _In_ ULONG_PTR Rsp,
    _In_ ULONG_PTR Rbp)
{
    ULONG_PTR Start;
    ULONG_PTR End;
    ULONG_PTR Addr;
    ULONG_PTR Value;
    ULONG Hits = 0;

    Start = Rsp & ~(ULONG_PTR)7;
    End = (Rbp + 0x20) & ~(ULONG_PTR)7;

    if (Start > (ULONG_PTR)MmUserProbeAddress ||
        End > (ULONG_PTR)MmUserProbeAddress ||
        End < Start ||
        (End - Start) > 0x400)
    {
        DPRINT1("User AV stack sentinel rbp junk rsp=%p rbp=%p; use rsp window\n",
                (PVOID)Rsp,
                (PVOID)Rbp);
        End = Start + 0x200;
        if (End > (ULONG_PTR)MmUserProbeAddress)
        {
            End = (ULONG_PTR)MmUserProbeAddress & ~(ULONG_PTR)7;
        }
        if (End <= Start)
        {
            return;
        }
    }

    _enable();

    _SEH2_TRY
    {
        ProbeForRead((PVOID)Start, (SIZE_T)(End - Start + 8), 1);
        for (Addr = Start; Addr <= End; Addr += 8)
        {
            Value = *(volatile ULONG_PTR *)Addr;
            if (Value == (ULONG_PTR)-1 || Value == (ULONG_PTR)-2)
            {
                DPRINT1("User AV stack [%p] qword %p\n",
                        (PVOID)Addr,
                        (PVOID)Value);
                Hits++;
            }
        }
        DPRINT1("User AV stack sentinel hits %lu (rsp=%p rbp=%p)\n",
                Hits,
                (PVOID)Rsp,
                (PVOID)Rbp);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("User AV stack sentinel walk failed 0x%08lx\n",
                _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    _disable();
}

/*
 * Clang amd64 has no SEH frames, so _SEH2_EXCEPT does not catch a probe
 * of a bad user pointer. The debugger would then stop the machine.
 * The syscall returns the access violation instead.
 */
static
VOID
KiAbortUserSyscallFault(IN PKTRAP_FRAME FaultFrame,
                        IN PEXCEPTION_RECORD ExceptionRecord)
{
    PKTRAP_FRAME SyscallFrame;
    ULONG_PTR FaultAddress;

    if (ExceptionRecord->ExceptionCode != STATUS_ACCESS_VIOLATION ||
        ExceptionRecord->NumberParameters < 2 ||
        KeGetPreviousMode() != UserMode ||
        KeGetCurrentIrql() != PASSIVE_LEVEL)
    {
        return;
    }

    FaultAddress = ExceptionRecord->ExceptionInformation[1];
    if (FaultAddress > (ULONG_PTR)MmUserProbeAddress)
    {
        return;
    }

    SyscallFrame = KeGetCurrentThread()->TrapFrame;
    if (SyscallFrame == NULL || SyscallFrame == FaultFrame)
    {
        return;
    }

    /* The syscall entry stores the user return address and stack here. */
    if ((ULONG_PTR)SyscallFrame->Rip > (ULONG_PTR)MmUserProbeAddress ||
        (ULONG_PTR)SyscallFrame->Rsp > (ULONG_PTR)MmUserProbeAddress)
    {
        return;
    }

    DPRINT1("Returning syscall access fault 0x%08lx at %p to %.16s\n",
            ExceptionRecord->ExceptionCode,
            (PVOID)FaultAddress,
            PsGetCurrentProcess()->ImageFileName);

    KiAbortSystemCall(SyscallFrame, ExceptionRecord->ExceptionCode);
}

VOID
NTAPI
KiDispatchException(IN PEXCEPTION_RECORD ExceptionRecord,
                    IN PKEXCEPTION_FRAME ExceptionFrame,
                    IN PKTRAP_FRAME TrapFrame,
                    IN KPROCESSOR_MODE PreviousMode,
                    IN BOOLEAN FirstChance)
{
    CONTEXT Context;

    /* Increase number of Exception Dispatches */
    KeGetCurrentPrcb()->KeExceptionDispatchCount++;

    /* Zero out the context to avoid leaking kernel stack memor to user mode */
    RtlZeroMemory(&Context, sizeof(Context));

    /* Set the context flags */
    Context.ContextFlags = CONTEXT_ALL;

    /* Get the Context from the trap and exception frame */
    KeTrapFrameToContext(TrapFrame, ExceptionFrame, &Context);

    /* Look at our exception code */
    switch (ExceptionRecord->ExceptionCode)
    {
        /* Breakpoint */
        case STATUS_BREAKPOINT:

            /* Decrement RIP by one */
            Context.Rip--;
            break;

        /* Internal exception */
        case KI_EXCEPTION_ACCESS_VIOLATION:

            /* Set correct code */
            ExceptionRecord->ExceptionCode = STATUS_ACCESS_VIOLATION;
            if (PreviousMode == UserMode)
            {
                /* FIXME: Handle no execute */
            }
            break;
    }

    /* Handle kernel-mode first, it's simpler */
    if (PreviousMode == KernelMode)
    {
        /* Check if this is a first-chance exception */
        if (FirstChance)
        {
            /* Break into the debugger for the first time */
            if (KiDebugRoutine(TrapFrame,
                               ExceptionFrame,
                               ExceptionRecord,
                               &Context,
                               PreviousMode,
                               FALSE))
            {
                /* Exception was handled */
                goto Handled;
            }

            /* If the Debugger couldn't handle it, dispatch the exception */
            if (RtlDispatchException(ExceptionRecord, &Context)) goto Handled;

            KiAbortUserSyscallFault(TrapFrame, ExceptionRecord);
        }

        /* This is a second-chance exception, only for the debugger */
        if (KiDebugRoutine(TrapFrame,
                           ExceptionFrame,
                           ExceptionRecord,
                           &Context,
                           PreviousMode,
                           TRUE))
        {
            /* Exception was handled */
            goto Handled;
        }

        /* Third strike; you're out */
        KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED,
                     ExceptionRecord->ExceptionCode,
                     (ULONG_PTR)ExceptionRecord->ExceptionAddress,
                     (ULONG_PTR)TrapFrame,
                     0);
    }
    else
    {
        /*
         * silent-fail20: INT 29 was misdelivered as AV fault -1; log the
         * failfast code (rcx) when STATUS_STACK_BUFFER_OVERRUN is raised
         * (serial-App-20261116T075237Z.log; FAST_FAIL_FATAL_APP_EXIT=7).
         * silent-fail23: also name module+RVA so a direct __fastfail (no
         * ucrt abort line) is not just a bare RIP (serial-App-20261116T082430Z.log).
         * silent-fail24: dump regs + stack return modules so the caller of
         * xul.dll+0x18f2c22 is named (serial-App-20261116T083050Z.log).
         * silent-fail25: rsp walk found no code returns; walk [rbp]/[rbp+8]
         * and print tid so AppPolicy vs fastfail share a thread
         * (serial-App-20261116T083849Z.log).
         */
        if (ExceptionRecord->ExceptionCode == STATUS_STACK_BUFFER_OVERRUN)
        {
            ULONG Slot;
            ULONG Frame;
            ULONG_PTR FrameBp;

            DPRINT1("User fastfail in %.16s tid %p code %Ix at %p\n",
                    PsGetCurrentProcess()->ImageFileName,
                    PsGetCurrentThreadId(),
                    ExceptionRecord->NumberParameters > 0 ?
                        ExceptionRecord->ExceptionInformation[0] : 0,
                    ExceptionRecord->ExceptionAddress);
            KiPrintUserModuleForAddress(ExceptionRecord->ExceptionAddress,
                                        "fastfail");
            DPRINT1("User fastfail regs rax=%p rcx=%p rdx=%p r8=%p rsp=%p rip=%p\n",
                    (PVOID)Context.Rax,
                    (PVOID)Context.Rcx,
                    (PVOID)Context.Rdx,
                    (PVOID)Context.R8,
                    (PVOID)Context.Rsp,
                    (PVOID)Context.Rip);
            DPRINT1("User fastfail regs rbx=%p rsi=%p rdi=%p rbp=%p r9=%p\n",
                    (PVOID)Context.Rbx,
                    (PVOID)Context.Rsi,
                    (PVOID)Context.Rdi,
                    (PVOID)Context.Rbp,
                    (PVOID)Context.R9);
            KiPrintUserInsnBytes(ExceptionRecord->ExceptionAddress);
            /*
             * Walk the first stack slots for return addresses into other
             * modules (same PEB LDR walk as KiPrintUserModuleForAddress).
             */
            {
                static const char *StackTags[16] =
                {
                    "rsp+0", "rsp+8", "rsp+10", "rsp+18",
                    "rsp+20", "rsp+28", "rsp+30", "rsp+38",
                    "rsp+40", "rsp+48", "rsp+50", "rsp+58",
                    "rsp+60", "rsp+68", "rsp+70", "rsp+78"
                };

                for (Slot = 0; Slot < 16; Slot++)
                {
                    ULONG_PTR SlotAddr = Context.Rsp + (ULONG_PTR)Slot * 8;
                    ULONG_PTR Value;

                    if (SlotAddr >= (ULONG_PTR)MmUserProbeAddress)
                    {
                        break;
                    }

                    _enable();
                    _SEH2_TRY
                    {
                        ProbeForRead((PVOID)SlotAddr, sizeof(Value), 1);
                        Value = *(volatile ULONG_PTR *)SlotAddr;
                    }
                    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                    {
                        Value = 0;
                    }
                    _SEH2_END;
                    _disable();

                    if (!Value || Value == (ULONG_PTR)-1)
                    {
                        continue;
                    }

                    KiPrintUserModuleForAddress((PVOID)Value, StackTags[Slot]);
                }
            }

            /*
             * Frame chain: [rbp] saved rbp, [rbp+8] return. silent-fail25
             * rsp slots were heap; the frame return should name the caller.
             */
            FrameBp = Context.Rbp;
            for (Frame = 0; Frame < 8; Frame++)
            {
                ULONG_PTR SavedBp = 0;
                ULONG_PTR RetAddr = 0;
                static const char *RetTags[8] =
                {
                    "frame0ret", "frame1ret", "frame2ret", "frame3ret",
                    "frame4ret", "frame5ret", "frame6ret", "frame7ret"
                };

                if (!FrameBp ||
                    FrameBp >= (ULONG_PTR)MmUserProbeAddress ||
                    (FrameBp & 7))
                {
                    break;
                }

                _enable();
                _SEH2_TRY
                {
                    ProbeForRead((PVOID)FrameBp, 16, 1);
                    SavedBp = *(volatile ULONG_PTR *)FrameBp;
                    RetAddr = *(volatile ULONG_PTR *)(FrameBp + 8);
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    SavedBp = 0;
                    RetAddr = 0;
                }
                _SEH2_END;
                _disable();

                DPRINT1("User fastfail frame%lu rbp=%p saved=%p ret=%p\n",
                        Frame,
                        (PVOID)FrameBp,
                        (PVOID)SavedBp,
                        (PVOID)RetAddr);
                if (RetAddr && RetAddr != (ULONG_PTR)-1)
                {
                    KiPrintUserModuleForAddress((PVOID)RetAddr, RetTags[Frame]);
                }

                if (!SavedBp || SavedBp <= FrameBp)
                {
                    break;
                }
                FrameBp = SavedBp;
            }
        }

        if (ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION)
        {
            DPRINT1("User AV in %.16s at %p fault %p write %Ix\n",
                    PsGetCurrentProcess()->ImageFileName,
                    ExceptionRecord->ExceptionAddress,
                    ExceptionRecord->NumberParameters > 1 ?
                        (PVOID)ExceptionRecord->ExceptionInformation[1] : NULL,
                    ExceptionRecord->NumberParameters > 0 ?
                        ExceptionRecord->ExceptionInformation[0] : 0);
            /*
             * silent-fail13: xul.dll reads -1; regs show what held the
             * bad pointer (serial-App-20261116T071818Z.log).
             */
            DPRINT1("User AV regs rax=%p rcx=%p rdx=%p r8=%p rsp=%p rip=%p\n",
                    (PVOID)Context.Rax,
                    (PVOID)Context.Rcx,
                    (PVOID)Context.Rdx,
                    (PVOID)Context.R8,
                    (PVOID)Context.Rsp,
                    (PVOID)Context.Rip);
            /*
             * silent-fail14: rcx/rdx/r8 were small ints; -1 may be in
             * rbx/rsi/rdi (serial-App-20261116T072238Z.log).
             */
            DPRINT1("User AV regs rbx=%p rsi=%p rdi=%p rbp=%p r9=%p\n",
                    (PVOID)Context.Rbx,
                    (PVOID)Context.Rsi,
                    (PVOID)Context.Rdi,
                    (PVOID)Context.Rbp,
                    (PVOID)Context.R9);
            /*
             * silent-fail18: -1 never appeared in probed memory or the
             * logged GPRs (serial-App-20261116T074132Z.log).
             */
            DPRINT1("User AV regs r10=%p r11=%p r12=%p r13=%p r14=%p r15=%p\n",
                    (PVOID)Context.R10,
                    (PVOID)Context.R11,
                    (PVOID)Context.R12,
                    (PVOID)Context.R13,
                    (PVOID)Context.R14,
                    (PVOID)Context.R15);
            KiPrintUserModuleForAddress(ExceptionRecord->ExceptionAddress,
                                        "rip");
            KiPrintUserInsnBytes(ExceptionRecord->ExceptionAddress);
            if (ExceptionRecord->NumberParameters > 1)
            {
                KiPrintUserModuleForAddress(
                    (PVOID)ExceptionRecord->ExceptionInformation[1],
                    "fault");
            }
            /*
             * silent-fail15/19: follow rsi10; scan frame; insn at RIP
             * (serial-App-20261116T074754Z.log).
             * silent-fail29: rsi/rbx were 3/4; rax/rcx sit near rsp and
             * rbp aliases the heap object in r12/r14
             * (serial-App-20261116T094615Z.log).
             */
            KiPrintUserModuleForAddress((PVOID)Context.Rbx, "rbx");
            KiPrintUserModuleForAddress((PVOID)Context.Rsi, "rsi");
            KiPrintUserModuleForAddress((PVOID)Context.Rdi, "rdi");
            KiPrintUserModuleForAddress((PVOID)Context.Rax, "rax");
            KiPrintUserModuleForAddress((PVOID)Context.Rcx, "rcx");
            KiPrintUserModuleForAddress((PVOID)Context.Rbp, "rbp");
            KiPrintUserQwordFields((PVOID)Context.Rbx, "rbx");
            KiPrintUserQwordFields((PVOID)Context.Rsi, "rsi");
            KiPrintUserQwordFields((PVOID)Context.Rax, "rax");
            KiPrintUserQwordFields((PVOID)Context.Rcx, "rcx");
            KiPrintUserQwordFields((PVOID)Context.Rbp, "rbp");
            KiPrintUserFollowedObject((PVOID)Context.Rsi, 0x8, "rsi8");
            KiPrintUserFollowedObject((PVOID)Context.Rsi, 0x10, "rsi10");
            KiPrintUserFollowedObject2((PVOID)Context.Rsi, 0x10, 0x0, "rsi10_0");
            KiPrintUserFollowedObject2((PVOID)Context.Rsi, 0x10, 0x18, "rsi10_18");
            KiPrintUserFollowedObject((PVOID)Context.Rbp, 0x0, "rbp0");
            KiPrintUserFollowedObject((PVOID)Context.Rbp, 0x8, "rbp8");
            KiPrintUserFollowedObject((PVOID)Context.Rbp, 0xa8, "rbpA8");
            {
                ULONG Slot;
                static const char *StackTags[16] =
                {
                    "rsp+0", "rsp+8", "rsp+10", "rsp+18",
                    "rsp+20", "rsp+28", "rsp+30", "rsp+38",
                    "rsp+40", "rsp+48", "rsp+50", "rsp+58",
                    "rsp+60", "rsp+68", "rsp+70", "rsp+78"
                };

                for (Slot = 0; Slot < 16; Slot++)
                {
                    ULONG_PTR SlotAddr = Context.Rsp + (ULONG_PTR)Slot * 8;
                    ULONG_PTR Value;

                    if (SlotAddr >= (ULONG_PTR)MmUserProbeAddress)
                    {
                        break;
                    }

                    _enable();
                    _SEH2_TRY
                    {
                        ProbeForRead((PVOID)SlotAddr, sizeof(Value), 1);
                        Value = *(volatile ULONG_PTR *)SlotAddr;
                    }
                    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                    {
                        Value = 0;
                    }
                    _SEH2_END;
                    _disable();

                    if (!Value || Value < 0x10000ULL || Value == (ULONG_PTR)-1)
                    {
                        continue;
                    }

                    KiPrintUserModuleForAddress((PVOID)Value, StackTags[Slot]);
                }
            }
            KiPrintUserStackSentinels(Context.Rsp, Context.Rbp);
        }

        /* User mode exception, was it first-chance? */
        if (FirstChance)
        {
            /*
             * Break into the kernel debugger unless a user mode debugger
             * is present or user mode exceptions are ignored, except if this
             * is a debug service which we must always pass to KD
             */
            if ((!(PsGetCurrentProcess()->DebugPort) &&
                 !(KdIgnoreUmExceptions)) ||
                 (KdIsThisAKdTrap(ExceptionRecord, &Context, PreviousMode)))
            {
                /* Make sure the debugger can access debug directories */
                KiPrepareUserDebugData();

                /* Call the kernel debugger */
                if (KiDebugRoutine(TrapFrame,
                                   ExceptionFrame,
                                   ExceptionRecord,
                                   &Context,
                                   PreviousMode,
                                   FALSE))
                {
                    /* Exception was handled */
                    goto Handled;
                }
            }

            /* Forward exception to user mode debugger */
            if (DbgkForwardException(ExceptionRecord, TRUE, FALSE)) return;

            /* Forward exception to user mode */
            if (KiDispatchExceptionToUser(TrapFrame, &Context, ExceptionRecord))
            {
                /* Success, the exception will be handled by KiUserExceptionDispatcher */
                return;
            }

            /* Failed to dispatch, fall through for second chance handling */
        }

        /* Try second chance */
        if (DbgkForwardException(ExceptionRecord, TRUE, TRUE))
        {
            /* Handled, get out */
            return;
        }
        else if (DbgkForwardException(ExceptionRecord, FALSE, TRUE))
        {
            /* Handled, get out */
            return;
        }

        /* 3rd strike, kill the process */
        DPRINT1("Kill %.16s, ExceptionCode: %lx, ExceptionAddress: %lx, BaseAddress: %lx\n",
                PsGetCurrentProcess()->ImageFileName,
                ExceptionRecord->ExceptionCode,
                ExceptionRecord->ExceptionAddress,
                PsGetCurrentProcess()->SectionBaseAddress);

        ZwTerminateProcess(NtCurrentProcess(), ExceptionRecord->ExceptionCode);
        KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED,
                     ExceptionRecord->ExceptionCode,
                     (ULONG_PTR)ExceptionRecord->ExceptionAddress,
                     (ULONG_PTR)TrapFrame,
                     0);
    }

Handled:
    /* Convert the context back into Trap/Exception Frames */
    KeContextToTrapFrame(&Context,
                         ExceptionFrame,
                         TrapFrame,
                         Context.ContextFlags,
                         PreviousMode);
    return;
}

NTSTATUS
NTAPI
KeRaiseUserException(IN NTSTATUS ExceptionCode)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}


VOID
DECLSPEC_NORETURN
KiSystemFatalException(IN ULONG ExceptionCode,
                       IN PKTRAP_FRAME TrapFrame)
{
    /* Bugcheck the system */
    KeBugCheckWithTf(UNEXPECTED_KERNEL_MODE_TRAP,
                     ExceptionCode,
                     0,
                     0,
                     0,
                     TrapFrame);
}

NTSTATUS
NTAPI
KiNpxNotAvailableFaultHandler(
    IN PKTRAP_FRAME TrapFrame)
{
    UNIMPLEMENTED;
    KeBugCheckWithTf(TRAP_CAUSE_UNKNOWN, 13, 0, 0, 1, TrapFrame);
    return -1;
}

static
BOOLEAN
KiIsPrivilegedInstruction(PUCHAR Ip, BOOLEAN Wow64)
{
    ULONG i;

    /* Handle prefixes */
    for (i = 0; i < 15; i++)
    {
        if (!Wow64)
        {
            /* Check for REX prefix */
            if ((Ip[0] >= 0x40) && (Ip[0] <= 0x4F))
            {
                Ip++;
                continue;
            }
        }

        switch (Ip[0])
        {
            /* Check prefixes */
            case 0x26: // ES
            case 0x2E: // CS / null
            case 0x36: // SS
            case 0x3E: // DS
            case 0x64: // FS
            case 0x65: // GS
            case 0x66: // OP
            case 0x67: // ADDR
            case 0xF0: // LOCK
            case 0xF2: // REP
            case 0xF3: // REP INS/OUTS
                Ip++;
                continue;
        }

        break;
    }

    if (i == 15)
    {
        /* Too many prefixes. Should only happen, when the code was concurrently modified. */
         return FALSE;
    }

    switch (Ip[0])
    {
        case 0xF4: // HLT
        case 0xFA: // CLI
        case 0xFB: // STI
            return TRUE;

        case 0x0F:
        {
            switch (Ip[1])
            {
                case 0x06: // CLTS
                case 0x07: // SYSRET
                case 0x08: // INVD
                case 0x09: // WBINVD
                case 0x20: // MOV CR, XXX
                case 0x21: // MOV DR, XXX
                case 0x22: // MOV XXX, CR
                case 0x23: // MOV YYY, DR
                case 0x30: // WRMSR
                case 0x32: // RDMSR
                case 0x33: // RDPMC
                case 0x35: // SYSEXIT
                case 0x78: // VMREAD
                case 0x79: // VMWRITE
                    return TRUE;

                case 0x00:
                {
                    /* Check MODRM Reg field */
                    switch ((Ip[2] >> 3) & 0x7)
                    {
                        case 2: // LLDT
                        case 3: // LTR
                            return TRUE;
                    }
                    break;
                }

                case 0x01:
                {
                    switch (Ip[2])
                    {
                        case 0xC1: // VMCALL
                        case 0xC2: // VMLAUNCH
                        case 0xC3: // VMRESUME
                        case 0xC4: // VMXOFF
                        case 0xC8: // MONITOR
                        case 0xC9: // MWAIT
                        case 0xD1: // XSETBV
                        case 0xF8: // SWAPGS
                            return TRUE;
                    }

                    /* Check MODRM Reg field */
                    switch ((Ip[2] >> 3) & 0x7)
                    {
                        case 2: // LGDT
                        case 3: // LIDT
                        case 6: // LMSW
                        case 7: // INVLPG / SWAPGS / RDTSCP
                            return TRUE;
                    }
                    break;
                }

                case 0x38:
                {
                    switch (Ip[2])
                    {
                        case 0x80: // INVEPT
                        case 0x81: // INVVPID
                            return TRUE;
                    }
                    break;
                }

                case 0xC7:
                {
                    /* Check MODRM Reg field */
                    switch ((Ip[2] >> 3) & 0x7)
                    {
                        case 0x06: // VMPTRLD, VMCLEAR, VMXON
                        case 0x07: // VMPTRST
                            return TRUE;
                    }
                    break;
                }
            }

            break;
        }
    }

    return FALSE;
}

static
NTSTATUS
KiGeneralProtectionFaultUserMode(
    _In_ PKTRAP_FRAME TrapFrame)
{
    BOOLEAN Wow64 = TrapFrame->SegCs == KGDT64_R3_CMCODE;
    PUCHAR InstructionPointer;
    NTSTATUS Status;

    /* We need to decode the instruction at RIP */
    InstructionPointer = (PUCHAR)TrapFrame->Rip;

    _SEH2_TRY
    {
        /* Probe the instruction address */
        ProbeForRead(InstructionPointer, 64, 1);

        /* Check if it's a privileged instruction */
        if (KiIsPrivilegedInstruction(InstructionPointer, Wow64))
        {
            Status = STATUS_PRIVILEGED_INSTRUCTION;
        }
        else
        {
            Status = STATUS_ACCESS_VIOLATION;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END

    return Status;
}

NTSTATUS
NTAPI
KiGeneralProtectionFaultHandler(
    IN PKTRAP_FRAME TrapFrame)
{
    PUCHAR Instructions;

    /* Check for user-mode GPF */
    if (TrapFrame->SegCs & 3)
    {
        return KiGeneralProtectionFaultUserMode(TrapFrame);
    }

    /* Check for lazy segment load */
    if (TrapFrame->SegDs != (KGDT64_R3_DATA | RPL_MASK))
    {
        /* Fix it */
        TrapFrame->SegDs = (KGDT64_R3_DATA | RPL_MASK);
        return STATUS_SUCCESS;
    }
    else if (TrapFrame->SegEs != (KGDT64_R3_DATA | RPL_MASK))
    {
        /* Fix it */
        TrapFrame->SegEs = (KGDT64_R3_DATA | RPL_MASK);
        return STATUS_SUCCESS;
    }

    /* Get Instruction Pointer */
    Instructions = (PUCHAR)TrapFrame->Rip;

    /* Check for IRET */
    if (Instructions[0] == 0x48 && Instructions[1] == 0xCF)
    {
        /* Not implemented */
        UNIMPLEMENTED;
        ASSERT(FALSE);
    }

    /* Check for RDMSR/WRMSR */
    if ((Instructions[0] == 0xF) &&            // 2-byte opcode
        ((Instructions[1] == 0x30) ||        // RDMSR
         (Instructions[1] == 0x32)))         // WRMSR
    {
        /* Unknown CPU MSR, so raise an access violation */
        return STATUS_ACCESS_VIOLATION;
    }

    ASSERT(FALSE);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
KiXmmExceptionHandler(
    IN PKTRAP_FRAME TrapFrame)
{
    ULONG ExceptionCode;

    if ((TrapFrame->MxCsr & _MM_EXCEPT_INVALID) &&
        !(TrapFrame->MxCsr & _MM_MASK_INVALID))
    {
        /* Invalid operation */
        ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
    }
    else if ((TrapFrame->MxCsr & _MM_EXCEPT_DENORM) &&
             !(TrapFrame->MxCsr & _MM_MASK_DENORM))
    {
        /* Denormalized operand. Yes, this is what Windows returns. */
        ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
    }
    else if ((TrapFrame->MxCsr & _MM_EXCEPT_DIV_ZERO) &&
             !(TrapFrame->MxCsr & _MM_MASK_DIV_ZERO))
    {
        /* Divide by zero */
        ExceptionCode = STATUS_FLOAT_DIVIDE_BY_ZERO;
    }
    else if ((TrapFrame->MxCsr & _MM_EXCEPT_OVERFLOW) &&
             !(TrapFrame->MxCsr & _MM_MASK_OVERFLOW))
    {
        /* Overflow */
        ExceptionCode = STATUS_FLOAT_OVERFLOW;
    }
    else if ((TrapFrame->MxCsr & _MM_EXCEPT_UNDERFLOW) &&
             !(TrapFrame->MxCsr & _MM_MASK_UNDERFLOW))
    {
        /* Underflow */
        ExceptionCode = STATUS_FLOAT_UNDERFLOW;
    }
    else if ((TrapFrame->MxCsr & _MM_EXCEPT_INEXACT) &&
             !(TrapFrame->MxCsr & _MM_MASK_INEXACT))
    {
        /* Precision */
        ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
    }
    else
    {
        /* Should not happen */
        ASSERT(FALSE);
    }

    return ExceptionCode;
}

/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/amd64/except.c
 * PURPOSE:         Exception Dispatching for amd64
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 *                  Alex Ionescu (alex.ionescu@reactos.org)
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
VOID
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
        _SEH2_YIELD(return);
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

    /* Exit to usermode */
    KiServiceExit2(TrapFrame);
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

    _SEH2_TRY
    {
        /* Get a pointer to the loader data */
        PebLdr = Teb->ProcessEnvironmentBlock->Ldr;
        if (!PebLdr) _SEH2_YIELD(return);

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

            /* Forward exception to user mode (does not return) */
            KiDispatchExceptionToUser(TrapFrame, &Context, ExceptionRecord);
            NT_ASSERT(FALSE);
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
    UNIMPLEMENTED;
    KeBugCheckWithTf(TRAP_CAUSE_UNKNOWN, 13, 0, 0, 1, TrapFrame);
    return -1;
}

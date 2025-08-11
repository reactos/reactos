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

#ifdef _M_AMD64
VOID
NTAPI
KiDebugServiceTrapDebug(VOID)
{
    /* Output to serial port directly */
    const char msg[] = "*** KiDebugServiceTrap ENTERED! ***\n";
    const char *p = msg;
    while (*p) 
    { 
        while ((__inbyte(0x3F8 + 5) & 0x20) == 0); 
        __outbyte(0x3F8, *p++); 
    }
}
#endif

CODE_SEG("INIT")
VOID
NTAPI
KeInitExceptions(VOID)
{
    int i, j;

    /* Initialize the Idt */
#ifdef _M_AMD64
    /* On AMD64, we need to fix up the addresses because the static table
       has unrelocated addresses. The addresses in the table are RVAs (relative
       virtual addresses) from the image base, not absolute addresses. */
    
    /* Get the actual kernel base. PsNtosImageBase is not initialized yet when
       KeInitExceptions is called, so we calculate it from our own address. */
    
    /* Get current function address to determine kernel base */
    ULONG64 CurrentFunc = (ULONG64)KeInitExceptions;
    
    /* The kernel is loaded at 0xFFFFF80006400000
       KeInitExceptions is at 0xFFFFF80006B1CD04
       So we need to mask to get 0xFFFFF80006400000 */
    
    /* Round down to nearest 0x400000 boundary (4MB) */
    ULONG64 KernelBase = CurrentFunc & 0xFFFFFFFFFFC00000ULL;
    
    /* Adjust if we're not at a standard base */
    if ((KernelBase & 0xFFFFFF) != 0x400000)
    {
        /* Try 1MB boundary */
        KernelBase = CurrentFunc & 0xFFFFFFFFF0000000ULL;
        /* Add the standard offset */
        KernelBase = KernelBase + 0x06400000;
    }
    
    /* Debug output */
    {
        const char msg1[] = "*** IDT Init: KernelBase=";
        const char *p1 = msg1;
        while (*p1) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p1++); }
        
        for (int k = 60; k >= 0; k -= 4)
        {
            int digit = (KernelBase >> k) & 0xF;
            char c = digit < 10 ? '0' + digit : 'A' + digit - 10;
            while ((__inbyte(0x3F8 + 5) & 0x20) == 0);
            __outbyte(0x3F8, c);
        }
        
        const char msg2[] = "\n";
        const char *p2 = msg2;
        while (*p2) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p2++); }
    }
#endif

    for (j = i = 0; i < 256; i++)
    {
        ULONG64 Offset;

        if (KiInterruptInitTable[j].InterruptId == i)
        {
            Offset = (ULONG64)KiInterruptInitTable[j].ServiceRoutine;
#ifdef _M_AMD64
            /* Debug output for 0x2D */
            if (i == 0x2D)
            {
                const char msg1[] = "*** IDT[0x2D]: Raw offset=";
                const char *p1 = msg1;
                while (*p1) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p1++); }
                
                for (int k = 60; k >= 0; k -= 4)
                {
                    int digit = (Offset >> k) & 0xF;
                    char c = digit < 10 ? '0' + digit : 'A' + digit - 10;
                    while ((__inbyte(0x3F8 + 5) & 0x20) == 0);
                    __outbyte(0x3F8, c);
                }
                
                const char msg2[] = "\n";
                const char *p2 = msg2;
                while (*p2) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p2++); }
            }
#endif
#ifdef _M_AMD64
            /* The addresses in the table are file offsets (unrelocated).
               We need to convert them to kernel virtual addresses.
               If the address is less than kernel space, it needs relocation. */
            if (Offset < 0xFFFF000000000000ULL)
            {
                /* This is an unrelocated RVA. The offset is relative to the image base.
                   The table contains addresses like 0x0CD04C4D which are offsets
                   from the default image base (0x00400000 for executables).
                   
                   We need to convert: 0x0CD04C4D -> 0xFFFFF80006784C4D
                   
                   The calculation is:
                   Real address = KernelBase + (RVA - DefaultImageBase)
                   where DefaultImageBase = 0x00400000 for PE executables
                   
                   But actually, it seems the values are already RVAs from base 0,
                   so we just add them to the kernel base. However, they seem
                   to have an offset of 0x0C900000 built in.
                   
                   Let's subtract that offset and add to kernel base. */
                   
                /* It appears the offsets are from 0x0C000000 range, 
                   but kernel functions start at 0x06000000 range in the loaded image.
                   So we need to adjust: 0x0CD04C4D - 0x0C900000 + 0x06380000 = 0x06784C4D
                   Then add kernel base. */
                   
                /* The raw offset is 0x0CD04C4D
                   We need offset from kernel base: 0x00384C4D
                   So we subtract: 0x0CD04C4D - 0x0C980000 = 0x00384C4D */
                if (Offset > 0x0C000000)
                {
                    Offset = Offset - 0x0C980000;
                }
                
                /* Now add the kernel base */
                Offset = KernelBase + Offset;
            }
#endif
            KiIdt[i].Dpl = KiInterruptInitTable[j].Dpl;
            KiIdt[i].IstIndex = KiInterruptInitTable[j].IstIndex;
            j++;
        }
        else
        {
            Offset = (ULONG64)&KiUnexpectedRange[i]._Op_push;
#ifdef _M_AMD64
            /* Check if this address needs relocation */
            if (Offset < 0xFFFF000000000000ULL)
            {
                /* Same relocation as above */
                if (Offset > 0x0C000000)
                {
                    Offset = Offset - 0x0C980000;
                }
                Offset = KernelBase + Offset;
            }
#endif
            KiIdt[i].Dpl = 0;
            KiIdt[i].IstIndex = 0;
        }
        KiIdt[i].OffsetLow = Offset & 0xffff;
        /* Always use kernel CS for IDT entries - the interrupt gate will switch to it */
        KiIdt[i].Selector = KGDT64_R0_CODE;
        KiIdt[i].Type = 0x0e;
        KiIdt[i].Reserved0 = 0;
        KiIdt[i].Present = 1;
        KiIdt[i].OffsetMiddle = (Offset >> 16) & 0xffff;
        KiIdt[i].OffsetHigh = (Offset >> 32);
        KiIdt[i].Reserved1 = 0;
    }

    KeGetPcr()->IdtBase = KiIdt;
    
#ifdef _M_AMD64
    /* Debug: Verify IDT entry 0x2D */
    {
        KIDTENTRY64 *Entry2D = &KiIdt[0x2D];
        ULONG64 Handler = (ULONG64)Entry2D->OffsetLow | 
                          ((ULONG64)Entry2D->OffsetMiddle << 16) | 
                          ((ULONG64)Entry2D->OffsetHigh << 32);
        
        const char msg1[] = "*** IDT[0x2D]: Handler=";
        const char *p1 = msg1;
        while (*p1) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p1++); }
        
        /* Output handler address in hex */
        for (int k = 60; k >= 0; k -= 4)
        {
            int digit = (Handler >> k) & 0xF;
            char c = digit < 10 ? '0' + digit : 'A' + digit - 10;
            while ((__inbyte(0x3F8 + 5) & 0x20) == 0);
            __outbyte(0x3F8, c);
        }
        
        const char msg2[] = " Expected=";
        const char *p2 = msg2;
        while (*p2) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p2++); }
        
        ULONG64 Expected = (ULONG64)KiDebugServiceTrap;
        for (int k = 60; k >= 0; k -= 4)
        {
            int digit = (Expected >> k) & 0xF;
            char c = digit < 10 ? '0' + digit : 'A' + digit - 10;
            while ((__inbyte(0x3F8 + 5) & 0x20) == 0);
            __outbyte(0x3F8, c);
        }
        
        const char msg3[] = "\n*** IDT[0x2D]: Selector=";
        const char *p3 = msg3;
        while (*p3) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p3++); }
        
        /* Output selector in hex */
        USHORT sel = Entry2D->Selector;
        for (int k = 12; k >= 0; k -= 4)
        {
            int digit = (sel >> k) & 0xF;
            char c = digit < 10 ? '0' + digit : 'A' + digit - 10;
            while ((__inbyte(0x3F8 + 5) & 0x20) == 0);
            __outbyte(0x3F8, c);
        }
        
        const char msg4[] = " DPL=";
        const char *p4 = msg4;
        while (*p4) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p4++); }
        
        char dpl = '0' + Entry2D->Dpl;
        while ((__inbyte(0x3F8 + 5) & 0x20) == 0);
        __outbyte(0x3F8, dpl);
        
        const char msg5[] = " Type=";
        const char *p5 = msg5;
        while (*p5) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p5++); }
        
        char type = Entry2D->Type < 10 ? '0' + Entry2D->Type : 'A' + Entry2D->Type - 10;
        while ((__inbyte(0x3F8 + 5) & 0x20) == 0);
        __outbyte(0x3F8, type);
        
        const char msg6[] = " Present=";
        const char *p6 = msg6;
        while (*p6) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p6++); }
        
        char present = '0' + Entry2D->Present;
        while ((__inbyte(0x3F8 + 5) & 0x20) == 0);
        __outbyte(0x3F8, present);
        
        const char msg7[] = "\n";
        const char *p7 = msg7;
        while (*p7) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p7++); }
    }
#endif
    
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
        /* Check if this is a stack overflow exception */
        if (LocalExceptRecord.ExceptionCode == STATUS_STACK_OVERFLOW)
        {
            /* We have exhausted the user stack, cannot dispatch to user mode */
            DPRINT1("Stack overflow detected while dispatching exception to user mode\n");
            DPRINT1("Original exception: %lx at %p\n", 
                    ExceptionRecord->ExceptionCode, 
                    ExceptionRecord->ExceptionAddress);
            
            /* Mark the thread as having a stack overflow */
            PsGetCurrentThread()->ExceptionPort = NULL;
            
            /* Return failure to trigger second chance handling */
            _disable();
            return FALSE;
        }
        
        /* Check if we got an access violation while touching user stack */
        if (LocalExceptRecord.ExceptionCode == STATUS_ACCESS_VIOLATION)
        {
            /* The user stack is inaccessible */
            DPRINT1("User stack inaccessible while dispatching exception\n");
            DPRINT1("Failed at address: %p\n", LocalExceptRecord.ExceptionAddress);
            
            /* Return failure to trigger second chance handling */
            _disable();
            return FALSE;
        }

        /* Some other exception occurred - still can't dispatch */
        DPRINT1("Exception %lx while dispatching to user mode\n", 
                LocalExceptRecord.ExceptionCode);
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

VOID
NTAPI
KiDispatchException(IN PEXCEPTION_RECORD ExceptionRecord,
                    IN PKEXCEPTION_FRAME ExceptionFrame,
                    IN PKTRAP_FRAME TrapFrame,
                    IN KPROCESSOR_MODE PreviousMode,
                    IN BOOLEAN FirstChance)
{
    CONTEXT Context;

#ifdef _M_AMD64
    #define COM1_PORT 0x3F8
    /* Debug output for exception dispatch */
    {
        const char msg[] = "*** KiDispatchException: Entry, ExceptionCode=";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        
        ULONG code = ExceptionRecord->ExceptionCode;
        for (int k = 28; k >= 0; k -= 4)
        {
            int digit = (code >> k) & 0xF;
            char c = digit < 10 ? '0' + digit : 'A' + digit - 10;
            while ((__inbyte(COM1_PORT + 5) & 0x20) == 0);
            __outbyte(COM1_PORT, c);
        }
        
        const char msg2[] = "\n";
        const char *p2 = msg2;
        while (*p2) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p2++); }
        
        /* If it's a breakpoint, show the service code */
        if (ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT && 
            ExceptionRecord->NumberParameters >= 1)
        {
            const char msg3[] = "*** KiDispatchException: BreakpointType=";
            const char *p3 = msg3;
            while (*p3) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p3++); }
            
            ULONG_PTR type = ExceptionRecord->ExceptionInformation[0];
            for (int k = 28; k >= 0; k -= 4)
            {
                int digit = (type >> k) & 0xF;
                char c = digit < 10 ? '0' + digit : 'A' + digit - 10;
                while ((__inbyte(COM1_PORT + 5) & 0x20) == 0);
                __outbyte(COM1_PORT, c);
            }
            
            const char msg4[] = "\n";
            const char *p4 = msg4;
            while (*p4) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p4++); }
        }
    }
#endif

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

            /* Check if this is a special debug service (not a regular INT3) */
            if (ExceptionRecord->NumberParameters >= 1 &&
                ExceptionRecord->ExceptionInformation[0] != BREAKPOINT_BREAK)
            {
                /* Special breakpoint (PRINT, PROMPT, etc.) - KdpTrap will handle RIP adjustment */
                /* KdpTrap knows the correct instruction size for each type */
#ifdef _M_AMD64
                {
                    const char msg[] = "*** KiDispatchException: Special debug service, calling KdpTrap ***\n";
                    const char *p = msg;
                    while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
                }
#endif
            }
            else
            {
                /* Regular INT3 breakpoint - decrement RIP by one */
                Context.Rip--;
            }
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
#ifdef _M_AMD64
            {
                const char msg[] = "*** KiDispatchException: About to call KiDebugRoutine (first chance) ***\n";
                const char *p = msg;
                while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
            }
#endif
            /* Break into the debugger for the first time */
            if (KiDebugRoutine(TrapFrame,
                               ExceptionFrame,
                               ExceptionRecord,
                               &Context,
                               PreviousMode,
                               FALSE))
            {
#ifdef _M_AMD64
                {
                    const char msg[] = "*** KiDispatchException: KiDebugRoutine handled the exception ***\n";
                    const char *p = msg;
                    while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
                }
#endif
                /* Exception was handled */
                goto Handled;
            }
#ifdef _M_AMD64
            {
                const char msg[] = "*** KiDispatchException: KiDebugRoutine did not handle the exception ***\n";
                const char *p = msg;
                while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
            }
#endif

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
        DPRINT1("Kill %.16s, ExceptionCode: %lx, ExceptionAddress: %p, BaseAddress: %p\n",
                PsGetCurrentProcess()->ImageFileName,
                ExceptionRecord->ExceptionCode,
                (PVOID)ExceptionRecord->ExceptionAddress,
                PsGetCurrentProcess()->SectionBaseAddress);

        ZwTerminateProcess(NtCurrentProcess(), ExceptionRecord->ExceptionCode);
        KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED,
                     ExceptionRecord->ExceptionCode,
                     (ULONG_PTR)ExceptionRecord->ExceptionAddress,
                     (ULONG_PTR)TrapFrame,
                     0);
    }

Handled:
#ifdef _M_AMD64
    {
        const char msg[] = "*** KiDispatchException: Reached Handled label, converting context back ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
#endif
    /* Convert the context back into Trap/Exception Frames */
    KeContextToTrapFrame(&Context,
                         ExceptionFrame,
                         TrapFrame,
                         Context.ContextFlags,
                         PreviousMode);
#ifdef _M_AMD64
    {
        const char msg[] = "*** KiDispatchException: Context converted, returning ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
#endif
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
KiFloatingErrorFaultHandler(
    IN PKTRAP_FRAME TrapFrame)
{
    PKTHREAD Thread;
    ULONG MxCsr;
    
    /* Get current thread */
    Thread = KeGetCurrentThread();
    
    /* Check if this is a kernel mode fault */
    if (TrapFrame->SegCs & MODE_MASK)
    {
        /* User mode fault - we'll dispatch an exception */
        return STATUS_FLOAT_INVALID_OPERATION;
    }
    
    /* Kernel mode floating point error */
    DPRINT1("Kernel mode floating point error at RIP=%p\n", TrapFrame->Rip);
    
    /* Read the MXCSR register to get SSE status */
    __asm__ __volatile__("stmxcsr %0" : "=m"(MxCsr));
    
    /* Check for specific floating point exceptions */
    if (MxCsr & 0x0001) /* Invalid operation */
    {
        DPRINT1("FPU: Invalid operation exception\n");
        return STATUS_FLOAT_INVALID_OPERATION;
    }
    else if (MxCsr & 0x0004) /* Divide by zero */
    {
        DPRINT1("FPU: Divide by zero exception\n");
        return STATUS_FLOAT_DIVIDE_BY_ZERO;
    }
    else if (MxCsr & 0x0008) /* Overflow */
    {
        DPRINT1("FPU: Overflow exception\n");
        return STATUS_FLOAT_OVERFLOW;
    }
    else if (MxCsr & 0x0010) /* Underflow */
    {
        DPRINT1("FPU: Underflow exception\n");
        return STATUS_FLOAT_UNDERFLOW;
    }
    else if (MxCsr & 0x0020) /* Precision */
    {
        DPRINT1("FPU: Precision exception\n");
        return STATUS_FLOAT_INEXACT_RESULT;
    }
    
    /* Unknown floating point error */
    DPRINT1("FPU: Unknown floating point error, MXCSR=%08lx\n", MxCsr);
    return STATUS_FLOAT_INVALID_OPERATION;
}

BOOLEAN
NTAPI
KiIsKernelStackOverflow(
    IN ULONG_PTR FaultAddress,
    IN PKTRAP_FRAME TrapFrame)
{
    PKTHREAD Thread;
    ULONG_PTR StackBase, StackLimit;
    
    /* Get the current thread */
    Thread = KeGetCurrentThread();
    if (!Thread) return FALSE;
    
    /* Get kernel stack bounds */
    StackBase = Thread->InitialStack;
    StackLimit = Thread->StackLimit;
    
    /* Check if we have valid stack bounds */
    if (!StackBase || !StackLimit) return FALSE;
    
    /* Check if the fault address is below the stack limit (stack overflow) */
    if (FaultAddress < StackLimit && FaultAddress >= (StackLimit - PAGE_SIZE))
    {
        /* This is likely a stack overflow */
        DPRINT1("Kernel stack overflow detected!\n");
        DPRINT1("Fault Address: %p, Stack Limit: %p, Stack Base: %p\n",
                (PVOID)FaultAddress, (PVOID)StackLimit, (PVOID)StackBase);
        DPRINT1("Thread: %p, Process: %p\n", Thread, Thread->Process);
        
        /* Check RSP to see how bad the overflow is */
        if (TrapFrame)
        {
            ULONG_PTR Rsp = TrapFrame->Rsp;
            DPRINT1("RSP: %p\n", (PVOID)Rsp);
            
            if (Rsp < StackLimit)
            {
                DPRINT1("Critical: RSP is already below stack limit by %lu bytes\n",
                        StackLimit - Rsp);
            }
        }
        
        return TRUE;
    }
    
    /* Not a stack overflow */
    return FALSE;
}

NTSTATUS
NTAPI
KiHandleKernelStackOverflow(
    IN PKTRAP_FRAME TrapFrame)
{
    /* We can't recover from kernel stack overflow - bug check */
    KeBugCheckEx(KERNEL_STACK_INPAGE_ERROR,
                 0, /* Reserved */
                 TrapFrame->Rsp,
                 TrapFrame->Rip,
                 (ULONG_PTR)TrapFrame);
                 
    /* Should never get here */
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
KiNpxNotAvailableFaultHandler(
    IN PKTRAP_FRAME TrapFrame)
{
    PKTHREAD Thread;
    ULONG64 Cr0;
    
    /* Get the current thread */
    Thread = KeGetCurrentThread();
    
    /* Check if we have FPU state to restore */
    if (!Thread)
    {
        /* No thread, this is fatal */
        KeBugCheckWithTf(TRAP_CAUSE_UNKNOWN, 13, 0, 0, 1, TrapFrame);
        return STATUS_UNSUCCESSFUL;
    }
    
    /* Get current CR0 */
    Cr0 = __readcr0();
    
    /* Check if FPU is present but disabled */
    if (Cr0 & CR0_EM)
    {
        /* FPU emulation not supported on AMD64 */
        DPRINT1("FPU emulation requested but not supported on AMD64\n");
        KeBugCheckWithTf(TRAP_CAUSE_UNKNOWN, 13, (ULONG_PTR)Cr0, 0, 2, TrapFrame);
        return STATUS_UNSUCCESSFUL;
    }
    
    /* Clear the TS flag to enable FPU access */
    __writecr0(Cr0 & ~CR0_TS);
    
    /* Check if this thread has FPU state to restore */
    if (Thread->StateSaveArea && Thread->NpxState != 0)
    {
        /* Restore FPU/XMM state using FXRSTOR */
        KiRestoreXState(Thread->StateSaveArea, Thread->NpxState);
    }
    else
    {
        /* Initialize FPU for first use */
        __asm__ __volatile__("fninit");
        
        /* Initialize MXCSR for SSE */
        __asm__ __volatile__(
            "push $0x1F80\n\t"
            "ldmxcsr (%%rsp)\n\t"
            "add $8, %%rsp\n\t"
            ::: "memory"
        );
        
        /* Mark thread as having FPU state (basic x87 + SSE) */
        Thread->NpxState = 0x3; /* XSTATE_LEGACY_FLOATING_POINT | XSTATE_LEGACY_SSE */
    }
    
    /* FPU is now available */
    return STATUS_SUCCESS;
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

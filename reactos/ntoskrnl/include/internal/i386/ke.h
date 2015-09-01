#pragma once

#ifndef __ASM__

#include "intrin_i.h"

//
// Thread Dispatcher Header DebugActive Mask
//
#define DR_MASK(x)                              (1 << (x))
#define DR_REG_MASK                             0x4F

//
// INT3 is 1 byte long
//
#define KD_BREAKPOINT_TYPE        UCHAR
#define KD_BREAKPOINT_SIZE        sizeof(UCHAR)
#define KD_BREAKPOINT_VALUE       0xCC

//
// Macros for getting and setting special purpose registers in portable code
//
#define KeGetContextPc(Context) \
    ((Context)->Eip)

#define KeSetContextPc(Context, ProgramCounter) \
    ((Context)->Eip = (ProgramCounter))

#define KeGetTrapFramePc(TrapFrame) \
    ((TrapFrame)->Eip)

#define KiGetLinkedTrapFrame(x) \
    (PKTRAP_FRAME)((x)->Edx)

#define KeGetContextReturnRegister(Context) \
    ((Context)->Eax)

#define KeSetContextReturnRegister(Context, ReturnValue) \
    ((Context)->Eax = (ReturnValue))

//
// Macro to get trap and exception frame from a thread stack
//
#define KeGetTrapFrame(Thread) \
    (PKTRAP_FRAME)((ULONG_PTR)((Thread)->InitialStack) - \
                   sizeof(KTRAP_FRAME) - \
                   sizeof(FX_SAVE_AREA))

#define KeGetExceptionFrame(Thread) \
    NULL

//
// Macro to get context switches from the PRCB
// All architectures but x86 have it in the PRCB's KeContextSwitches
//
#define KeGetContextSwitches(Prcb)  \
    CONTAINING_RECORD(Prcb, KIPCR, PrcbData)->ContextSwitches

//
// Macro to get the second level cache size field name which differs between
// CISC and RISC architectures, as the former has unified I/D cache
//
#define KiGetSecondLevelDCacheSize() ((PKIPCR)KeGetPcr())->SecondLevelCacheSize

//
// Returns the Interrupt State from a Trap Frame.
// ON = TRUE, OFF = FALSE
//
#define KeGetTrapFrameInterruptState(TrapFrame) \
        BooleanFlagOn((TrapFrame)->EFlags, EFLAGS_INTERRUPT_MASK)

//
// Flags for exiting a trap
//
#define KTE_SKIP_PM_BIT  (((KTRAP_EXIT_SKIP_BITS) { { .SkipPreviousMode = TRUE } }).Bits)
#define KTE_SKIP_SEG_BIT (((KTRAP_EXIT_SKIP_BITS) { { .SkipSegments  = TRUE } }).Bits)
#define KTE_SKIP_VOL_BIT (((KTRAP_EXIT_SKIP_BITS) { { .SkipVolatiles = TRUE } }).Bits)

typedef union _KTRAP_EXIT_SKIP_BITS
{
    struct
    {
        UCHAR SkipPreviousMode:1;
        UCHAR SkipSegments:1;
        UCHAR SkipVolatiles:1;
        UCHAR Reserved:5;
    };
    UCHAR Bits;
} KTRAP_EXIT_SKIP_BITS, *PKTRAP_EXIT_SKIP_BITS;


//
// Flags used by the VDM/V8086 emulation engine for determining instruction prefixes
//
#define PFX_FLAG_ES                0x00000100
#define PFX_FLAG_CS                0x00000200
#define PFX_FLAG_SS                0x00000400
#define PFX_FLAG_DS                0x00000800
#define PFX_FLAG_FS                0x00001000
#define PFX_FLAG_GS                0x00002000
#define PFX_FLAG_OPER32            0x00004000
#define PFX_FLAG_ADDR32            0x00008000
#define PFX_FLAG_LOCK              0x00010000
#define PFX_FLAG_REPNE             0x00020000
#define PFX_FLAG_REP               0x00040000

//
// VDM Helper Macros
//
// All VDM/V8086 opcode emulators have the same FASTCALL function definition.
// We need to keep 2 parameters while the original ASM implementation uses 4:
// TrapFrame, PrefixFlags, Eip, InstructionSize;
//
// We pass the trap frame, and prefix flags, in our two parameters.
//
// We then realize that since the smallest prefix flag is 0x100, this gives us
// a count of up to 0xFF. So we OR in the instruction size with the prefix flags
//
// We further realize that we always have access to EIP from the trap frame, and
// that if we want the *current instruction* EIP, we simply have to add the
// instruction size *MINUS ONE*, and that gives us the EIP we should be looking
// at now, so we don't need to use the stack to push this parameter.
//
// We actually only care about the *current instruction* EIP in one location,
// so although it may be slightly more expensive to re-calculate the EIP one
// more time, this way we don't redefine ALL opcode handlers to have 3 parameters,
// which would be forcing stack usage in all other scenarios.
//
#define KiVdmSetVdmEFlags(x)        InterlockedOr((PLONG)KiNtVdmState, (x));
#define KiVdmClearVdmEFlags(x)      InterlockedAnd((PLONG)KiNtVdmState, ~(x))
#define KiCallVdmHandler(x)         KiVdmOpcode##x(TrapFrame, Flags)
#define KiCallVdmPrefixHandler(x)   KiVdmOpcodePrefix(TrapFrame, Flags | x)
#define KiVdmUnhandledOpcode(x)                     \
    BOOLEAN                                         \
    FASTCALL                                        \
    KiVdmOpcode##x(IN PKTRAP_FRAME TrapFrame,       \
                   IN ULONG Flags)                  \
    {                                               \
        /* Not yet handled */                       \
        UNIMPLEMENTED_DBGBREAK();                   \
        return FALSE;                               \
    }

C_ASSERT(NPX_FRAME_LENGTH == sizeof(FX_SAVE_AREA));

//
// Local parameters
//
typedef struct _KV86_FRAME
{
    PVOID ThreadStack;
    PVOID ThreadTeb;
    PVOID PcrTeb;
} KV86_FRAME, *PKV86_FRAME;

//
// Virtual Stack Frame
//
typedef struct _KV8086_STACK_FRAME
{
    KTRAP_FRAME TrapFrame;
    FX_SAVE_AREA NpxArea;
    KV86_FRAME V86Frame;
} KV8086_STACK_FRAME, *PKV8086_STACK_FRAME;

//
// Large Pages Support
//
typedef struct _LARGE_IDENTITY_MAP
{
    PHARDWARE_PTE TopLevelDirectory;
    ULONG Cr3;
    ULONG_PTR StartAddress;
    ULONG PagesCount;
    PVOID PagesList[30];
} LARGE_IDENTITY_MAP, *PLARGE_IDENTITY_MAP;

/* Diable interrupts and return whether they were enabled before */
FORCEINLINE
BOOLEAN
KeDisableInterrupts(VOID)
{
    ULONG Flags;
    BOOLEAN Return;

    /* Get EFLAGS and check if the interrupt bit is set */
    Flags = __readeflags();
    Return = (Flags & EFLAGS_INTERRUPT_MASK) ? TRUE: FALSE;

    /* Disable interrupts */
    _disable();
    return Return;
}

/* Restore previous interrupt state */
FORCEINLINE
VOID
KeRestoreInterrupts(BOOLEAN WereEnabled)
{
    if (WereEnabled) _enable();
}

//
// Registers an interrupt handler with an IDT vector
//
FORCEINLINE
VOID
KeRegisterInterruptHandler(IN ULONG Vector,
                           IN PVOID Handler)
{
    UCHAR Entry;
    ULONG_PTR Address;
    PKIPCR Pcr = (PKIPCR)KeGetPcr();

    //
    // Get the entry from the HAL
    //
    Entry = HalVectorToIDTEntry(Vector);
    Address = PtrToUlong(Handler);

    //
    // Now set the data
    //
    Pcr->IDT[Entry].ExtendedOffset = (USHORT)(Address >> 16);
    Pcr->IDT[Entry].Offset = (USHORT)Address;
}

//
// Returns the registered interrupt handler for a given IDT vector
//
FORCEINLINE
PVOID
KeQueryInterruptHandler(IN ULONG Vector)
{
    PKIPCR Pcr = (PKIPCR)KeGetPcr();
    UCHAR Entry;

    //
    // Get the entry from the HAL
    //
    Entry = HalVectorToIDTEntry(Vector);

    //
    // Read the entry from the IDT
    //
    return (PVOID)(((Pcr->IDT[Entry].ExtendedOffset << 16) & 0xFFFF0000) |
                    (Pcr->IDT[Entry].Offset & 0xFFFF));
}

//
// Invalidates the TLB entry for a specified address
//
FORCEINLINE
VOID
KeInvalidateTlbEntry(IN PVOID Address)
{
    /* Invalidate the TLB entry for this address */
    __invlpg(Address);
}

FORCEINLINE
VOID
KeFlushProcessTb(VOID)
{
    /* Flush the TLB by resetting CR3 */
    __writecr3(__readcr3());
}

FORCEINLINE
PRKTHREAD
KeGetCurrentThread(VOID)
{
    /* Return the current thread */
    return ((PKIPCR)KeGetPcr())->PrcbData.CurrentThread;
}

FORCEINLINE
VOID
KiRundownThread(IN PKTHREAD Thread)
{
#ifndef CONFIG_SMP
    /* Check if this is the NPX Thread */
    if (KeGetCurrentPrcb()->NpxThread == Thread)
    {
        /* Clear it */
        KeGetCurrentPrcb()->NpxThread = NULL;
        Ke386FnInit();
    }
#else
    /* Nothing to do */
#endif
}

FORCEINLINE
VOID
Ke386SetGdtEntryBase(PKGDTENTRY GdtEntry, PVOID BaseAddress)
{
    GdtEntry->BaseLow = (USHORT)((ULONG_PTR)BaseAddress & 0xFFFF);
    GdtEntry->HighWord.Bytes.BaseMid = (UCHAR)((ULONG_PTR)BaseAddress >> 16);
    GdtEntry->HighWord.Bytes.BaseHi = (UCHAR)((ULONG_PTR)BaseAddress >> 24);
}

FORCEINLINE
VOID
KiSetTebBase(PKPCR Pcr, PVOID TebAddress)
{
    Pcr->NtTib.Self = TebAddress;
    Ke386SetGdtEntryBase(&Pcr->GDT[KGDT_R3_TEB / sizeof(KGDTENTRY)], TebAddress);
}

VOID
FASTCALL
Ki386InitializeTss(
    IN PKTSS Tss,
    IN PKIDTENTRY Idt,
    IN PKGDTENTRY Gdt
);

VOID
NTAPI
KiSetCR0Bits(VOID);

VOID
NTAPI
KiGetCacheInformation(VOID);

BOOLEAN
NTAPI
KiIsNpxPresent(
    VOID
);

BOOLEAN
NTAPI
KiIsNpxErrataPresent(
    VOID
);

VOID
NTAPI
KiSetProcessorType(VOID);

ULONG
NTAPI
KiGetFeatureBits(VOID);

VOID
NTAPI
KiThreadStartup(VOID);

NTSTATUS
NTAPI
Ke386GetGdtEntryThread(
    IN PKTHREAD Thread,
    IN ULONG Offset,
    IN PKGDTENTRY Descriptor
);

VOID
NTAPI
KiFlushNPXState(
    IN FLOATING_SAVE_AREA *SaveArea
);

VOID
NTAPI
Ki386AdjustEsp0(
    IN PKTRAP_FRAME TrapFrame
);

VOID
NTAPI
Ki386SetupAndExitToV86Mode(
    OUT PTEB VdmTeb
);

VOID
NTAPI
KeI386VdmInitialize(
    VOID
);

ULONG_PTR
NTAPI
Ki386EnableGlobalPage(
    IN ULONG_PTR Context
);

ULONG_PTR
NTAPI
Ki386EnableTargetLargePage(
    IN ULONG_PTR Context
);

BOOLEAN
NTAPI
Ki386CreateIdentityMap(
    IN PLARGE_IDENTITY_MAP IdentityMap,
    IN PVOID StartPtr,
    IN ULONG Length
);

VOID
NTAPI
Ki386FreeIdentityMap(
    IN PLARGE_IDENTITY_MAP IdentityMap
);

VOID
NTAPI
Ki386EnableCurrentLargePage(
    IN ULONG_PTR StartAddress,
    IN ULONG Cr3
);

VOID
NTAPI
KiI386PentiumLockErrataFixup(
    VOID
);

VOID
NTAPI
KiInitializePAT(
    VOID
);

VOID
NTAPI
KiInitializeMTRR(
    IN BOOLEAN FinalCpu
);

VOID
NTAPI
KiAmdK6InitializeMTRR(
    VOID
);

VOID
NTAPI
KiRestoreFastSyscallReturnState(
    VOID
);

ULONG_PTR
NTAPI
Ki386EnableDE(
    IN ULONG_PTR Context
);

ULONG_PTR
NTAPI
Ki386EnableFxsr(
    IN ULONG_PTR Context
);

ULONG_PTR
NTAPI
Ki386EnableXMMIExceptions(
    IN ULONG_PTR Context
);

BOOLEAN
NTAPI
VdmDispatchBop(
    IN PKTRAP_FRAME TrapFrame
);

BOOLEAN
FASTCALL
KiVdmOpcodePrefix(
    IN PKTRAP_FRAME TrapFrame,
    IN ULONG Flags
);

BOOLEAN
FASTCALL
Ki386HandleOpcodeV86(
    IN PKTRAP_FRAME TrapFrame
);

DECLSPEC_NORETURN
VOID
FASTCALL
KiEoiHelper(
    IN PKTRAP_FRAME TrapFrame
);

VOID
FASTCALL
Ki386BiosCallReturnAddress(
    IN PKTRAP_FRAME TrapFrame
);

ULONG_PTR
FASTCALL
KiExitV86Mode(
    IN PKTRAP_FRAME TrapFrame
);

DECLSPEC_NORETURN
VOID
NTAPI
KiDispatchExceptionFromTrapFrame(
    IN NTSTATUS Code,
    IN ULONG Flags,
    IN ULONG_PTR Address,
    IN ULONG ParameterCount,
    IN ULONG_PTR Parameter1,
    IN ULONG_PTR Parameter2,
    IN ULONG_PTR Parameter3,
    IN PKTRAP_FRAME TrapFrame
);

//
// Global x86 only Kernel data
//
extern PVOID Ki386IopmSaveArea;
extern ULONG KeI386EFlagsAndMaskV86;
extern ULONG KeI386EFlagsOrMaskV86;
extern BOOLEAN KeI386VirtualIntExtensions;
extern KIDTENTRY KiIdt[MAXIMUM_IDTVECTOR+1];
extern KDESCRIPTOR KiIdtDescriptor;
extern BOOLEAN KiI386PentiumLockErrataPresent;
extern ULONG KeI386NpxPresent;
extern ULONG KeI386XMMIPresent;
extern ULONG KeI386FxsrPresent;
extern ULONG KiMXCsrMask;
extern ULONG KeI386CpuType;
extern ULONG KeI386CpuStep;
extern ULONG Ke386CacheAlignment;
extern ULONG KiFastSystemCallDisable;
extern UCHAR KiDebugRegisterTrapOffsets[9];
extern UCHAR KiDebugRegisterContextOffsets[9];
extern DECLSPEC_NORETURN VOID __cdecl KiTrap02(VOID);
extern VOID __cdecl KiTrap08(VOID);
extern VOID __cdecl KiTrap13(VOID);
extern VOID __cdecl KiFastCallEntry(VOID);
extern VOID NTAPI ExpInterlockedPopEntrySListFault(VOID);
extern VOID NTAPI ExpInterlockedPopEntrySListResume(VOID);
extern VOID __cdecl CopyParams(VOID);
extern VOID __cdecl ReadBatch(VOID);
extern CHAR KiSystemCallExitBranch[];
extern CHAR KiSystemCallExit[];
extern CHAR KiSystemCallExit2[];

//
// Trap Macros
//
#include "trap_x.h"

//
// Returns a thread's FPU save area
//
FORCEINLINE
PFX_SAVE_AREA
KiGetThreadNpxArea(IN PKTHREAD Thread)
{
    ASSERT((ULONG_PTR)Thread->InitialStack % 16 == 0);
    return (PFX_SAVE_AREA)((ULONG_PTR)Thread->InitialStack - sizeof(FX_SAVE_AREA));
}

//
// Sanitizes a selector
//
FORCEINLINE
ULONG
Ke386SanitizeSeg(IN ULONG Cs,
                IN KPROCESSOR_MODE Mode)
{
    //
    // Check if we're in kernel-mode, and force CPL 0 if so.
    // Otherwise, force CPL 3.
    //
    return ((Mode == KernelMode) ?
            (Cs & (0xFFFF & ~RPL_MASK)) :
            (RPL_MASK | (Cs & 0xFFFF)));
}

//
// Sanitizes EFLAGS
//
FORCEINLINE
ULONG
Ke386SanitizeFlags(IN ULONG Eflags,
                   IN KPROCESSOR_MODE Mode)
{
    //
    // Check if we're in kernel-mode, and sanitize EFLAGS if so.
    // Otherwise, also force interrupt mask on.
    //
    return ((Mode == KernelMode) ?
            (Eflags & (EFLAGS_USER_SANITIZE | EFLAGS_INTERRUPT_MASK)) :
            (EFLAGS_INTERRUPT_MASK | (Eflags & EFLAGS_USER_SANITIZE)));
}

//
// Sanitizes a Debug Register
//
FORCEINLINE
PVOID
Ke386SanitizeDr(IN PVOID DrAddress,
                IN KPROCESSOR_MODE Mode)
{
    //
    // Check if we're in kernel-mode, and return the address directly if so.
    // Otherwise, make sure it's not inside the kernel-mode address space.
    // If it is, then clear the address.
    //
    return ((Mode == KernelMode) ? DrAddress :
            (DrAddress <= MM_HIGHEST_USER_ADDRESS) ? DrAddress : 0);
}

//
// Exception with no arguments
//
FORCEINLINE
DECLSPEC_NORETURN
VOID
KiDispatchException0Args(IN NTSTATUS Code,
                         IN ULONG_PTR Address,
                         IN PKTRAP_FRAME TrapFrame)
{
    /* Helper for exceptions with no arguments */
    KiDispatchExceptionFromTrapFrame(Code, 0, Address, 0, 0, 0, 0, TrapFrame);
}

//
// Exception with one argument
//
FORCEINLINE
DECLSPEC_NORETURN
VOID
KiDispatchException1Args(IN NTSTATUS Code,
                         IN ULONG_PTR Address,
                         IN ULONG P1,
                         IN PKTRAP_FRAME TrapFrame)
{
    /* Helper for exceptions with no arguments */
    KiDispatchExceptionFromTrapFrame(Code, 0, Address, 1, P1, 0, 0, TrapFrame);
}

//
// Exception with two arguments
//
FORCEINLINE
DECLSPEC_NORETURN
VOID
KiDispatchException2Args(IN NTSTATUS Code,
                         IN ULONG_PTR Address,
                         IN ULONG P1,
                         IN ULONG P2,
                         IN PKTRAP_FRAME TrapFrame)
{
    /* Helper for exceptions with no arguments */
    KiDispatchExceptionFromTrapFrame(Code, 0, Address, 2, P1, P2, 0, TrapFrame);
}

//
// Performs a system call
//

    /*
     * This sequence does a RtlCopyMemory(Stack - StackBytes, Arguments, StackBytes)
     * and then calls the function associated with the system call.
     *
     * It's done in assembly for two reasons: we need to muck with the stack,
     * and the call itself restores the stack back for us. The only way to do
     * this in C is to do manual C handlers for every possible number of args on
     * the stack, and then have the handler issue a call by pointer. This is
     * wasteful since it'll basically push the values twice and require another
     * level of call indirection.
     *
     * The ARM kernel currently does this, but it should probably be changed
     * later to function like this as well.
     *
     */
#ifdef __GNUC__
FORCEINLINE
NTSTATUS
KiSystemCallTrampoline(IN PVOID Handler,
                       IN PVOID Arguments,
                       IN ULONG StackBytes)
{
    NTSTATUS Result;

    __asm__ __volatile__
    (
        "subl %1, %%esp\n\t"
        "movl %%esp, %%edi\n\t"
        "movl %2, %%esi\n\t"
        "shrl $2, %1\n\t"
        "rep movsd\n\t"
        "call *%3\n\t"
        "movl %%eax, %0"
        : "=r"(Result)
        : "c"(StackBytes),
          "d"(Arguments),
          "r"(Handler)
        : "%esp", "%esi", "%edi"
    );
    return Result;
}
#elif defined(_MSC_VER)
FORCEINLINE
NTSTATUS
KiSystemCallTrampoline(IN PVOID Handler,
                       IN PVOID Arguments,
                       IN ULONG StackBytes)
{
    __asm
    {
        mov ecx, StackBytes
        mov esi, Arguments
        mov eax, Handler
        sub esp, ecx
        mov edi, esp
        shr ecx, 2
        rep movsd
        call eax
    }
    /* Return with result in EAX */
}
#else
#error Unknown Compiler
#endif


//
// Checks for pending APCs
//
FORCEINLINE
VOID
KiCheckForApcDelivery(IN PKTRAP_FRAME TrapFrame)
{
    PKTHREAD Thread;
    KIRQL OldIrql;

    /* Check for V8086 or user-mode trap */
    if ((TrapFrame->EFlags & EFLAGS_V86_MASK) || (KiUserTrap(TrapFrame)))
    {
        /* Get the thread */
        Thread = KeGetCurrentThread();
        while (TRUE)
        {
            /* Turn off the alerted state for kernel mode */
            Thread->Alerted[KernelMode] = FALSE;

            /* Are there pending user APCs? */
            if (!Thread->ApcState.UserApcPending) break;

            /* Raise to APC level and enable interrupts */
            OldIrql = KfRaiseIrql(APC_LEVEL);
            _enable();

            /* Deliver APCs */
            KiDeliverApc(UserMode, NULL, TrapFrame);

            /* Restore IRQL and disable interrupts once again */
            KfLowerIrql(OldIrql);
            _disable();
        }
    }
}

//
// Converts a base thread to a GUI thread
//
#ifdef __GNUC__
FORCEINLINE
NTSTATUS
KiConvertToGuiThread(VOID)
{
    NTSTATUS NTAPI PsConvertToGuiThread(VOID);
    NTSTATUS Result;
    PVOID StackFrame;

    /*
     * Converting to a GUI thread safely updates ESP in-place as well as the
     * current Thread->TrapFrame and EBP when KeSwitchKernelStack is called.
     *
     * However, PsConvertToGuiThread "helpfully" restores EBP to the original
     * caller's value, since it is considered a nonvolatile register. As such,
     * as soon as we're back after the conversion and we try to store the result
     * which will probably be in some stack variable (EBP-based), we'll crash as
     * we are touching the de-allocated non-expanded stack.
     *
     * Thus we need a way to update our EBP before EBP is touched, and the only
     * way to guarantee this is to do the call itself in assembly, use the EAX
     * register to store the result, fixup EBP, and then let the C code continue
     * on its merry way.
     *
     */
    __asm__ __volatile__
    (
        "movl %%ebp, %1\n\t"
        "subl %%esp, %1\n\t"
        "call _PsConvertToGuiThread@0\n\t"
        "addl %%esp, %1\n\t"
        "movl %1, %%ebp"
        : "=a"(Result), "=r"(StackFrame)
        : "p"(PsConvertToGuiThread)
        : "%esp", "%ecx", "%edx", "memory"
    );
    return Result;
}
#elif defined(_MSC_VER)
NTSTATUS
NTAPI
KiConvertToGuiThread(VOID);
#else
#error Unknown Compiler
#endif

//
// Switches from boot loader to initial kernel stack
//
FORCEINLINE
VOID
KiSwitchToBootStack(IN ULONG_PTR InitialStack)
{
    VOID NTAPI KiSystemStartupBootStack(VOID);

    /* We have to switch to a new stack before continuing kernel initialization */
#ifdef __GNUC__
    __asm__
    (
        "movl %0, %%esp\n\t"
        "subl %1, %%esp\n\t"
        "pushl %2\n\t"
        "jmp _KiSystemStartupBootStack@0"
        :
        : "c"(InitialStack),
          "i"(NPX_FRAME_LENGTH + KTRAP_FRAME_ALIGN + KTRAP_FRAME_LENGTH),
          "i"(CR0_EM | CR0_TS | CR0_MP),
          "p"(KiSystemStartupBootStack)
        : "%esp"
    );
#elif defined(_MSC_VER)
    __asm
    {
        mov esp, InitialStack
        sub esp, (NPX_FRAME_LENGTH + KTRAP_FRAME_ALIGN + KTRAP_FRAME_LENGTH)
        push (CR0_EM | CR0_TS | CR0_MP)
        jmp KiSystemStartupBootStack
    }
#else
#error Unknown Compiler
#endif
}

//
// Emits the iret instruction for C code
//
FORCEINLINE
DECLSPEC_NORETURN
VOID
KiIret(VOID)
{
#if defined(__GNUC__)
    __asm__ __volatile__
    (
        "iret"
    );
#elif defined(_MSC_VER)
    __asm
    {
        iretd
    }
#else
#error Unsupported compiler
#endif
    UNREACHABLE;
}

//
// Normally this is done by the HAL, but on x86 as an optimization, the kernel
// initiates the end by calling back into the HAL and exiting the trap here.
//
FORCEINLINE
VOID
KiEndInterrupt(IN KIRQL Irql,
               IN PKTRAP_FRAME TrapFrame)
{
    /* Disable interrupts and end the interrupt */
    _disable();
    HalEndSystemInterrupt(Irql, TrapFrame);

    /* Exit the interrupt */
    KiEoiHelper(TrapFrame);
}

//
// PERF Code
//
FORCEINLINE
VOID
Ki386PerfEnd(VOID)
{
    extern ULONGLONG BootCyclesEnd, BootCycles;
    BootCyclesEnd = __rdtsc();
    DbgPrint("Boot took %I64u cycles!\n", BootCyclesEnd - BootCycles);
    DbgPrint("Interrupts: %u System Calls: %u Context Switches: %u\n",
             KeGetCurrentPrcb()->InterruptCount,
             KeGetCurrentPrcb()->KeSystemCalls,
             KeGetContextSwitches(KeGetCurrentPrcb()));
}

FORCEINLINE
PULONG
KiGetUserModeStackAddress(void)
{
    return &(KeGetCurrentThread()->TrapFrame->HardwareEsp);
}

#endif

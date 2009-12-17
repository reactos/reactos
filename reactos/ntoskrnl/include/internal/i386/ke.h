#ifndef __NTOSKRNL_INCLUDE_INTERNAL_I386_KE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_I386_KE_H

#ifndef __ASM__

#include "intrin_i.h"
#include "v86m.h"

extern ULONG Ke386CacheAlignment;

//
// Thread Dispatcher Header DebugActive Mask
//
#define DR_MASK(x)                              (1 << (x))
#define DR_REG_MASK                             0x4F

#define IMAGE_FILE_MACHINE_ARCHITECTURE IMAGE_FILE_MACHINE_I386

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
// Returns the Interrupt State from a Trap Frame.
// ON = TRUE, OFF = FALSE
//
#define KeGetTrapFrameInterruptState(TrapFrame) \
        BooleanFlagOn((TrapFrame)->EFlags, EFLAGS_INTERRUPT_MASK)

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

#ifdef _NTOSKRNL_ /* FIXME: Move flags above to NDK instead of here */
VOID
NTAPI
KiThreadStartup(PKSYSTEM_ROUTINE SystemRoutine,
                PKSTART_ROUTINE StartRoutine,
                PVOID StartContext,
                BOOLEAN UserThread,
                KTRAP_FRAME TrapFrame);
#endif

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
    IN volatile ULONG_PTR Context
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

//
// Global x86 only Kernel data
//
extern PVOID Ki386IopmSaveArea;
extern ULONG KeI386EFlagsAndMaskV86;
extern ULONG KeI386EFlagsOrMaskV86;
extern BOOLEAN KeI386VirtualIntExtensions;
extern KIDTENTRY KiIdt[MAXIMUM_IDTVECTOR];
extern KDESCRIPTOR KiIdtDescriptor;
extern ULONG Ke386GlobalPagesEnabled;
extern BOOLEAN KiI386PentiumLockErrataPresent;
extern ULONG KeI386NpxPresent;
extern ULONG KeI386XMMIPresent;
extern ULONG KeI386FxsrPresent;
extern ULONG KiMXCsrMask;
extern ULONG KeI386CpuType;
extern ULONG KeI386CpuStep;
extern UCHAR KiDebugRegisterTrapOffsets[9];
extern UCHAR KiDebugRegisterContextOffsets[9];
extern VOID __cdecl KiTrap2(VOID);
extern VOID __cdecl KiTrap8(VOID);
extern VOID __cdecl KiTrap19(VOID);
extern VOID __cdecl KiFastCallEntry(VOID);

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
// Gets a DR register from a CONTEXT structure
//
FORCEINLINE
PVOID
KiDrFromContext(IN ULONG Dr,
                IN PCONTEXT Context)
{
    return *(PVOID*)((ULONG_PTR)Context + KiDebugRegisterContextOffsets[Dr]);
}

//
// Gets a DR register from a KTRAP_FRAME structure
//
FORCEINLINE
PVOID*
KiDrFromTrapFrame(IN ULONG Dr,
                  IN PKTRAP_FRAME TrapFrame)
{
    return (PVOID*)((ULONG_PTR)TrapFrame + KiDebugRegisterTrapOffsets[Dr]);
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

#endif
#endif /* __NTOSKRNL_INCLUDE_INTERNAL_I386_KE_H */

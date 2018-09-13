/*++ BUILD Version: 0018    // Increment this if a change has global effects

Copyright (c) 1993  IBM Corporation

Module Name:

    ppc.h

Abstract:

    This module contains the PowerPC hardware specific header file.

Author:

    Rick Simpson   9-Jul-1993

    Based on mips.h, by David N. Cutler (davec) 31-Mar-1990

Revision History:

--*/

#ifndef _PPCH_
#define _PPCH_

// begin_ntddk begin_wdm begin_nthal begin_ntndis

#if defined(_PPC_)

//
// Define maximum size of flush multple TB request.
//

#define FLUSH_MULTIPLE_MAXIMUM 48

//
// Indicate that the compiler (with MIPS front-end) supports
// the pragma textout construct.
//

#define ALLOC_PRAGMA 1

//
// Define function decoration depending on whether a driver, a file system,
// or a kernel component is being built.
//

//  end_wdm

#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_)

#define NTKERNELAPI DECLSPEC_IMPORT             // wdm

#else

#define NTKERNELAPI

#endif

//
// Define function decoration depending on whether the HAL or other kernel
// component is being build.
//

#if !defined(_NTHAL_)

#define NTHALAPI DECLSPEC_IMPORT                // wdm

#else

#define NTHALAPI

#endif

// end_ntndis
//
// Define macro to generate import names.
//

#define IMPORT_NAME(name) __imp_##name

//
// PowerPC specific interlocked operation result values.
//
// These are the values used on MIPS; there appears to be no
// need to change them for PowerPC.
//

#define RESULT_ZERO      0
#define RESULT_NEGATIVE -2
#define RESULT_POSITIVE -1

//
// Interlocked result type is portable, but its values are machine specific.
// Constants for value are in i386.h, mips.h, ppc.h, etc.
//

typedef enum _INTERLOCKED_RESULT {
    ResultNegative = RESULT_NEGATIVE,
    ResultZero     = RESULT_ZERO,
    ResultPositive = RESULT_POSITIVE
} INTERLOCKED_RESULT;


//
// Convert portable interlock interfaces to architecure specific interfaces.
//

#define ExInterlockedIncrementLong(Addend, Lock) \
    ExPpcInterlockedIncrementLong(Addend)

#define ExInterlockedDecrementLong(Addend, Lock) \
    ExPpcInterlockedDecrementLong(Addend)

#define ExInterlockedExchangeUlong(Target, Value, Lock) \
    ExPpcInterlockedExchangeUlong(Target, Value)

NTKERNELAPI
INTERLOCKED_RESULT
ExPpcInterlockedIncrementLong (
    IN PLONG Addend
    );

NTKERNELAPI
INTERLOCKED_RESULT
ExPpcInterlockedDecrementLong (
    IN PLONG Addend
    );

NTKERNELAPI
LARGE_INTEGER
ExInterlockedExchangeAddLargeInteger (
    IN PLARGE_INTEGER Addend,
    IN LARGE_INTEGER Increment,
    IN PKSPIN_LOCK Lock
    );

NTKERNELAPI
ULONG
ExPpcInterlockedExchangeUlong (
    IN PULONG Target,
    IN ULONG Value
    );

//  begin_wdm

//
// Intrinsic interlocked functions
//

#if defined(_M_PPC) && defined(_MSC_VER) && (_MSC_VER>=1000) && !defined(RC_INVOKED)

#define InterlockedIncrement _InterlockedIncrement
#define InterlockedDecrement _InterlockedDecrement
#define InterlockedExchange _InterlockedExchange
#define InterlockedExchangeAdd _InterlockedExchangeAdd
#define InterlockedCompareExchange _InterlockedCompareExchange

LONG
InterlockedIncrement(
    IN OUT PLONG Addend
    );

LONG
InterlockedDecrement(
    IN OUT PLONG Addend
    );

LONG
InterlockedExchange(
    IN OUT PLONG Target,
    IN LONG Increment
    );

LONG
InterlockedExchangeAdd(
    IN OUT PLONG Addend,
    IN LONG Value
    );

PVOID
InterlockedCompareExchange (
    IN OUT PVOID *Destination,
    IN PVOID Exchange,
    IN PVOID Comperand
    );

#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement)
#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedCompareExchange)

#else

NTKERNELAPI
LONG
InterlockedIncrement(
    IN OUT PLONG Addend
    );

NTKERNELAPI
LONG
InterlockedDecrement(
    IN OUT PLONG Addend
    );

NTKERNELAPI
LONG
InterlockedExchange(
    IN OUT PLONG Target,
    IN LONG Increment
    );

NTKERNELAPI
LONG
InterlockedExchangeAdd(
    IN OUT PLONG Addend,
    IN LONG Value
    );

NTKERNELAPI
PVOID
InterlockedCompareExchange (
    IN OUT PVOID *Destination,
    IN PVOID Exchange,
    IN PVOID Comperand
    );

#endif

//
// PowerPC Interrupt Definitions.
//
// Define length of interupt object dispatch code in 32-bit words.
//

#define DISPATCH_LENGTH 4               // Length of dispatch code in instructions

//
// Define Interrupt Request Levels.
//

#define PASSIVE_LEVEL   0               // Passive release level
#define LOW_LEVEL       0               // Lowest interrupt level
#define APC_LEVEL       1               // APC interrupt level
#define DISPATCH_LEVEL  2               // Dispatcher level
#define PROFILE_LEVEL   27              // Profiling level
#define IPI_LEVEL       29              // Interprocessor interrupt level
#define POWER_LEVEL     30              // Power failure level
#define FLOAT_LEVEL     31              // Floating interrupt level
#define HIGH_LEVEL      31              // Highest interrupt level
#define SYNCH_LEVEL     DISPATCH_LEVEL  // Synchronization level

//
// Define profile intervals.
//
// **FINISH**  These are the MIPS R4000 values; investigate for PPC

#define DEFAULT_PROFILE_COUNT 0x40000000             // ~= 20 seconds @50mhz
#define DEFAULT_PROFILE_INTERVAL (10 * 500)          // 500 microseconds
#define MAXIMUM_PROFILE_INTERVAL (10 * 1000 * 1000)  // 1 second
#define MINIMUM_PROFILE_INTERVAL (10 * 40)           // 40 microseconds

// end_ntddk end_wdm end_nthal

#define KiSynchIrql SYNCH_LEVEL         // enable portable code
#define KiProfileIrql PROFILE_LEVEL     // enable portable code

//
// Define machine specific external references.
//

// **FINISH**  On MIPS, this is defined in ...\ntos\ke\mips\xxintsup.s
//             For PPC, ensure that there is a C-referencable label
//             on the list of instructions (the ordinary entry point
//             name won't be, because it will start with '.').
extern ULONG KiInterruptTemplate[];

//
// Sanitize FPSCR and MSR based on processor mode.
//    By analogy with MIPS "Sanitize FSR" and "Sanitize PSR"
//
//    General form:  #define SANTIZE_<reg>(reg, mode)
//                      ((mode) == KernelMode ?
//                          ((0x00000000L) | ((reg) & 0xFFFFFFFF)) :
//                          ((0x00000000L) | ((reg) & 0xFFFFFFFF)))
//    Where 0x00000000L represents bits that are forced on, and
//    0xFFFFFFFF represents bits that are forced off.
//
//    We will optimize this expression right here in the macro, because
//    the initial PPC compiler cannot be expected to do so itself.
//
//  FPSCR -- Floating Point Status and Control Register
//
//     We turn off the various exception enable bits so that loading
//     the FPSCR cannot cause an exception.
//         Force to 0:  VE (24), OE (25), UE (26), ZE (27), XE (28)
//         Force to 1:  -none-
//         Let caller specify:  All others

// **FINISH** -- Set this macro back to do something; leave as a no-op for now

#define SANITIZE_FPSCR(fpscr, mode) (fpscr)
//#define SANITIZE_FPSCR(fpscr, mode) (fpscr & 0xFFFFFF07L)

//
//  MSR -- Machine State Register
//
//     If kernel mode, then
//         Force to 0:  reserved (0..12, 24, 28)
//         Force to 1:  ILE (15), LE (31)
//         Let caller specify:
//                      POW (13), implementation-dependent (14),
//                      EE (16), PR (17), FP (18), ME (19),
//                      FE0 (20), SE (21), BE (22), FE1 (23),
//                      IP (25), IR (26), DR (27), PM (29), RI (30)
//
//     If user mode, then
//         Force to 0:  POW (13), implementation-dependent (14), IP (25),
//                      reserved (0..12, 24, 28)
//         Force to 1:  ILE (15), EE (16), PR (17), FPE (18), ME (19),
//                      IR (26), DR (27), RI (30), LE (31)
//         Let caller specify:
//                      FE0 (20), SE (21), BE (22), FE1 (23), PM (29)
//

#define SANITIZE_MSR(msr, mode) \
    ((mode) == KernelMode ? \
        ((0x00010001L) | ((msr) & 0x0007FF77L)) : \
        ((0x0001F033L) | ((msr) & 0x0001FF37L)))

// begin_ntddk begin_wdm begin_nthal
//
// Define length of interrupt vector table.
//

#define MAXIMUM_VECTOR 256

//
// Processor Control Region
//
//   On PowerPC, this cannot be at a fixed virtual address;
//   it must be at a different address on each processor of an MP.
//

#define PCR_MINOR_VERSION 1
#define PCR_MAJOR_VERSION 1

typedef struct _KPCR {

//
// Major and minor version numbers of the PCR.
//

    USHORT MinorVersion;
    USHORT MajorVersion;

//
// Start of the architecturally defined section of the PCR. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//
// Interrupt and error exception vectors.
//

    PKINTERRUPT_ROUTINE InterruptRoutine[MAXIMUM_VECTOR];
    ULONG PcrPage2;
    ULONG Kseg0Top;
    ULONG Spare7[30];

//
// First and second level cache parameters.
//

    ULONG FirstLevelDcacheSize;
    ULONG FirstLevelDcacheFillSize;
    ULONG FirstLevelIcacheSize;
    ULONG FirstLevelIcacheFillSize;
    ULONG SecondLevelDcacheSize;
    ULONG SecondLevelDcacheFillSize;
    ULONG SecondLevelIcacheSize;
    ULONG SecondLevelIcacheFillSize;

//
// Pointer to processor control block.
//

    struct _KPRCB *Prcb;

//
// Pointer to the thread environment block.  A fast-path system call
// is provided that will return this value to user-mode code.
//

    PVOID Teb;

//
// Data cache alignment and fill size used for cache flushing and alignment.
// These fields are set to the larger of the first and second level data
// cache fill sizes.
//

    ULONG DcacheAlignment;
    ULONG DcacheFillSize;

//
// Instruction cache alignment and fill size used for cache flushing and
// alignment. These fields are set to the larger of the first and second
// level data cache fill sizes.
//

    ULONG IcacheAlignment;
    ULONG IcacheFillSize;

//
// Processor identification information from PVR.
//

    ULONG ProcessorVersion;
    ULONG ProcessorRevision;

//
// Profiling data.
//

    ULONG ProfileInterval;
    ULONG ProfileCount;

//
// Stall execution count and scale factor.
//

    ULONG StallExecutionCount;
    ULONG StallScaleFactor;

//
// Spare cell.
//

    ULONG Spare;

//
// Cache policy, right justified, as read from the processor configuration
// register at startup.
//

    union {
        ULONG CachePolicy;
        struct {
                UCHAR IcacheMode;       // Dynamic cache mode for PPC
                UCHAR DcacheMode;       // Dynamic cache mode for PPC
                USHORT ModeSpare;
        };
    };

//
// IRQL mapping tables.
//

    UCHAR IrqlMask[32];
    UCHAR IrqlTable[9];

//
// Current IRQL.
//

    UCHAR CurrentIrql;

//
// Processor identification
//
    CCHAR Number;
    KAFFINITY SetMember;

//
// Reserved interrupt vector mask.
//

    ULONG ReservedVectors;

//
// Current state parameters.
//

    struct _KTHREAD *CurrentThread;

//
// Cache policy, PTE field aligned, as read from the processor configuration
// register at startup.
//

    ULONG AlignedCachePolicy;

//
// Flag for determining pending software interrupts
//
    union {
        ULONG SoftwareInterrupt;        // any bit 1 => some s/w interrupt pending
        struct {
            UCHAR ApcInterrupt;         // 0x01 if APC int pending
            UCHAR DispatchInterrupt;    // 0x01 if dispatch int pending
            UCHAR Spare4;
            UCHAR Spare5;
        };
    };

//
// Complement of the processor affinity mask.
//

    KAFFINITY NotMember;

//
// Space reserved for the system.
//

    ULONG   SystemReserved[16];

//
// Space reserved for the HAL
//

    ULONG   HalReserved[16];

//
// End of the architecturally defined section of the PCR. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//
// end_ntddk end_wdm end_nthal

//
// Start of the operating system release dependent section of the PCR.
// This section may change from release to release and should not be
// addressed by vendor/platform specific HAL code.
//
// Function active flags.
//

    ULONG FirstLevelActive;

//
// System service dispatch start and end address used by get/set context.
//

    ULONG SystemServiceDispatchStart;
    ULONG SystemServiceDispatchEnd;

//
// Interrupt stack.
//

    ULONG InterruptStack;

//
// Quantum end flag.
//

    ULONG QuantumEnd;

//
// Exception handler values.
//

    PVOID InitialStack;
    PVOID PanicStack;
    ULONG BadVaddr;
    PVOID StackLimit;
    PVOID SavedStackLimit;
    ULONG SavedV0;
    ULONG SavedV1;
    UCHAR DebugActive;
    UCHAR Spare6[3];

//
// Save area for 6 GPRs on interrupt other than Storage interrupt.
//
    ULONG GprSave[6];

//
// Save area for Instruction Storage and Data Storage interrupts.
//

    ULONG SiR0;
    ULONG SiR2;
    ULONG SiR3;
    ULONG SiR4;
    ULONG SiR5;

    ULONG Spare0;
    ULONG Spare8;

//
// Real address of current process's Page Directory Page,
// changed when process (address space) changes,
// for use by Instruction Storage and Data Storage interrupts.
//

    ULONG PgDirRa;

//
// On interrupt stack indicator and saved initial stack.
//

    ULONG OnInterruptStack;
    ULONG SavedInitialStack;

} KPCR, *PKPCR;                     // ntddk wdm nthal

//
// The PCR address on a particular processor is contained in
//   SPRG 0 and 1.
//        SPRG 0 -- Real address of PCR (used only by interrupt code)
//        SPRG 1 -- Virtual address of PCR (used by kernel generally)
//   These SPRGs are not accessable to user-mode code.


//
// Get Pointer to Processor Control Region
//
// KiGetPcr() is a two-instruction routine that just reads SPRG 1
//   into reg 3 and returns.  Eventually this should be expanded in-line.

KPCR * KiGetPcr(VOID);

// begin_nthal

#define KIPCR2  0xffffe000              // kernel address of second PCR
#define KI_USER_SHARED_DATA KIPCR2
#define SharedUserData ((KUSER_SHARED_DATA * const)KIPCR2)

// begin_ntddk begin_wdm

#define KIPCR   0xffffd000              // kernel address of first PCR

#define PCR ((volatile KPCR * const)KIPCR)

#if defined(_M_PPC) && defined(_MSC_VER) && (_MSC_VER>=1000)
unsigned __sregister_get( unsigned const regnum );
#define _PPC_SPRG1_ 273
#define PCRsprg1 ((volatile KPCR * volatile)__sregister_get(_PPC_SPRG1_))
#else
KPCR * __builtin_get_sprg1(VOID);
#define PCRsprg1 ((volatile KPCR * volatile)__builtin_get_sprg1())
#endif

//
// Macros for enabling and disabling system interrupts.
//
//BUGBUG - work around 603e/ev errata #15
//  The instructions __emit'ed in these macros are "cror 0,0,0" instructions
//  that force the mtmsr to complete before allowing any subsequent loads to
//  issue.   The condition register no-op is executed in the system unit on
//  the 603.  This will not dispatch until the mtmsr completes and will halt
//  further dispatch.   On a 601 or 604 this instruction executes in the
//  branch unit and will run in parallel (i.e., no performance penalty except
//  for code bloat).
//

#if defined(_M_PPC) && defined(_MSC_VER) && (_MSC_VER>=1000)

unsigned __sregister_get( unsigned const regnum );
void __sregister_set( unsigned const regnum, unsigned value );
#define _PPC_MSR_ (unsigned)(~0x0)
#define _enable()  (__sregister_set(_PPC_MSR_, __sregister_get(_PPC_MSR_) | 0x00008000), __emit(0x4C000382))
#define _disable() (__sregister_set(_PPC_MSR_, __sregister_get(_PPC_MSR_) & 0xffff7fff), __emit(0x4C000382))
#define __builtin_get_msr() __sregister_get(_PPC_MSR_)

#else

ULONG __builtin_get_msr(VOID);
VOID  __builtin_set_msr(ULONG);
#define _enable()  (__builtin_set_msr(__builtin_get_msr() | 0x00008000), __builtin_isync())
#define _disable() (__builtin_set_msr(__builtin_get_msr() & 0xffff7fff), __builtin_isync())

#endif

//
// Get current IRQL.
//

#define KeGetCurrentIrql() PCR->CurrentIrql

//
// Get address of current processor block.
//

#define KeGetCurrentPrcb() PCR->Prcb

//
// Get address of processor control region.
//

#define KeGetPcr() PCR

//
// Get address of current kernel thread object.
//

#define KeGetCurrentThread() PCR->CurrentThread

//
// Get Processor Version Register
//

#if defined(_M_PPC) && defined(_MSC_VER) && (_MSC_VER>=1000)
unsigned __sregister_get( unsigned const regnum );
#define _PPC_PVR_ 287
#define KeGetPvr() __sregister_get(_PPC_PVR_)
#else
ULONG __builtin_get_pvr(VOID);
#define KeGetPvr() __builtin_get_pvr()
#endif

// end_wdm
//
// Get current processor number.
//

#define KeGetCurrentProcessorNumber() PCR->Number

// begin_wdm
//
// Get data cache fill size.
//
// **FINISH**  See that proper PowerPC parameter is accessed here

#define KeGetDcacheFillSize() PCR->DcacheFillSize

// end_ntddk end_wdm end_nthal

//
// Get previous processor mode.
//

#define KeGetPreviousMode() (KPROCESSOR_MODE)PCR->CurrentThread->PreviousMode

//
// Test if executing a DPC.
//

BOOLEAN
KeIsExecutingDpc (
    VOID
    );

// begin_ntddk begin_wdm
//
// Save & Restore floating point state
//

#define KeSaveFloatingPointState(a)         STATUS_SUCCESS
#define KeRestoreFloatingPointState(a)      STATUS_SUCCESS

// end_ntddk end_wdm

// begin_nthal
//
// Fill TB random entry
//

NTKERNELAPI
VOID
KeFillEntryTb (
    IN HARDWARE_PTE Pte[2],
    IN PVOID Virtual,
    IN BOOLEAN Invalid
    );

//
// Data cache, instruction cache, I/O buffer, and write buffer flush routine
// prototypes.
//

NTKERNELAPI
VOID
KeChangeColorPage (
    IN PVOID NewColor,
    IN PVOID OldColor,
    IN ULONG PageFrame
    );

NTKERNELAPI
VOID
KeSweepDcache (
    IN BOOLEAN AllProcessors
    );

#define KeSweepCurrentDcache() \
    HalSweepDcache();

NTKERNELAPI
VOID
KeSweepIcache (
    IN BOOLEAN AllProcessors
    );

#define KeSweepCurrentIcache() \
    HalSweepIcache();          \
    HalSweepDcache();

NTKERNELAPI
VOID
KeSweepIcacheRange (
    IN BOOLEAN AllProcessors,
    IN PVOID BaseAddress,
    IN ULONG Length
    );

// begin_ntddk begin_wdm begin_ntndis
//
// Cache and write buffer flush functions.
//

NTKERNELAPI
VOID
KeFlushIoBuffers (
    IN PMDL Mdl,
    IN BOOLEAN ReadOperation,
    IN BOOLEAN DmaOperation
    );

// end_ntddk end_wdm end_ntndis

//
// Clock, profile, and interprocessor interrupt functions.
//

struct _KEXCEPTION_FRAME;
struct _KTRAP_FRAME;

NTKERNELAPI
VOID
KeIpiInterrupt (
    IN struct _KTRAP_FRAME *TrapFrame
    );

NTKERNELAPI
VOID
KeProfileInterrupt (
    IN struct _KTRAP_FRAME *TrapFrame
    );

NTKERNELAPI
VOID
KeProfileInterruptWithSource (
    IN struct _KTRAP_FRAME *TrapFrame,
    IN KPROFILE_SOURCE ProfileSource
    );

NTKERNELAPI
VOID
KeUpdateRunTime (
    IN struct _KTRAP_FRAME *TrapFrame
    );

NTKERNELAPI
VOID
KeUpdateSystemTime (
    IN struct _KTRAP_FRAME *TrapFrame,
    IN ULONG TimeIncrement
    );

//
// Spin lock function prototypes (empty for uniprocessor).
// Exported for use in MP HALs.
//

#if defined(NT_UP)

#define KiAcquireSpinLock(SpinLock)

#else

NTKERNELAPI
VOID
KiAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock
    );

#endif

#if defined(NT_UP)

#define KiReleaseSpinLock(SpinLock)

#else

NTKERNELAPI
VOID
KiReleaseSpinLock (
    IN PKSPIN_LOCK SpinLock
    );

#endif
// end_nthal


//
// Define executive macros for acquiring and releasing executive spinlocks.
// These macros can ONLY be used by executive components and NOT by drivers.
// Drivers MUST use the kernel interfaces since they must be MP enabled on
// all systems.
//
// On PPC, raise/lower IRQL are in the HAL, but KeRaiseIrqlToDpcLevel (raising to a
// software level) is in the kernel.
//

// begin_ntddk begin_nthal

NTKERNELAPI
KIRQL
KfRaiseIrqlToDpcLevel (
    VOID
    );

#define KeRaiseIrqlToDpcLevel(OldIrql) (*(OldIrql) = KfRaiseIrqlToDpcLevel())

NTKERNELAPI
KIRQL
KeRaiseIrqlToSynchLevel (
    VOID
    );

// end_ntddk end_nthal
#if defined(NT_UP) && !defined(_NTDDK_) && !defined(_NTIFS_)
#define ExAcquireSpinLock(Lock, OldIrql) KeRaiseIrqlToDpcLevel((OldIrql))
#define ExReleaseSpinLock(Lock, OldIrql) KeLowerIrql((OldIrql))
#define ExAcquireSpinLockAtDpcLevel(Lock)
#define ExReleaseSpinLockFromDpcLevel(Lock)
#else

// begin_wdm begin_ntddk
#define ExAcquireSpinLock(Lock, OldIrql) KeAcquireSpinLock((Lock), (OldIrql))
#define ExReleaseSpinLock(Lock, OldIrql) KeReleaseSpinLock((Lock), (OldIrql))
#define ExAcquireSpinLockAtDpcLevel(Lock) KeAcquireSpinLockAtDpcLevel(Lock)
#define ExReleaseSpinLockFromDpcLevel(Lock) KeReleaseSpinLockFromDpcLevel(Lock)
// end_wdm end_ntddk

#endif

//
// The acquire and release fast lock macros disable and enable interrupts
// on UP nondebug systems. On MP or debug systems, the spinlock routines
// are used.
//
// N.B. Extreme caution should be observed when using these routines.
//

#if defined(NT_UP) && !DBG
#define ExAcquireFastLock(Lock, OldIrql) _disable()
#else
#define ExAcquireFastLock(Lock, OldIrql) \
    ExAcquireSpinLock(Lock, OldIrql)
#endif

#if defined(NT_UP) && !DBG
#define ExReleaseFastLock(Lock, OldIrql) _enable()
#else
#define ExReleaseFastLock(Lock, OldIrql) \
    ExReleaseSpinLock(Lock, OldIrql)
#endif



//
// Define query system time macro.
//

#define KiQuerySystemTime(CurrentTime)                                  \
    do {                                                                \
        (CurrentTime)->HighPart = SharedUserData->SystemTime.High1Time; \
        (CurrentTime)->LowPart = SharedUserData->SystemTime.LowPart;    \
    } while ((CurrentTime)->HighPart != SharedUserData->SystemTime.High2Time)

//
// Define query tick count macro.
//

#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_)

//  begin_wdm begin_ntddk

#define KeQueryTickCount(CurrentCount) { \
    PKSYSTEM_TIME _TickCount = *((PKSYSTEM_TIME *)(&KeTickCount)); \
    do {                                                           \
        (CurrentCount)->HighPart = _TickCount->High1Time;          \
        (CurrentCount)->LowPart = _TickCount->LowPart;             \
    } while ((CurrentCount)->HighPart != _TickCount->High2Time);    \
}

//  end_wdm end_ntddk

#else

// begin_nthal
#define KiQueryTickCount(CurrentCount) \
    do {                                                        \
        (CurrentCount)->HighPart = KeTickCount.High1Time;       \
        (CurrentCount)->LowPart = KeTickCount.LowPart;          \
    } while ((CurrentCount)->HighPart != KeTickCount.High2Time)

NTKERNELAPI
VOID
KeQueryTickCount (
    OUT PLARGE_INTEGER CurrentCount
    );

// end_nthal
#endif

#define KiQueryLowTickCount() KeTickCount.LowPart

//
// Define query interrupt time macro.
//

#define KiQueryInterruptTime(CurrentTime)                                   \
    do {                                                                    \
        (CurrentTime)->HighPart = SharedUserData->InterruptTime.High1Time;  \
        (CurrentTime)->LowPart = SharedUserData->InterruptTime.LowPart;     \
    } while ((CurrentTime)->HighPart != SharedUserData->InterruptTime.High2Time)


//
// The following function prototypes must be in the module since they are
// machine dependent.
//

//
// Raise and lower IRQL
//

ULONG
KiEmulateBranch (
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame
    );

BOOLEAN
KiEmulateFloating (
    IN OUT PEXCEPTION_RECORD ExceptionRecord,
    IN OUT struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN OUT struct _KTRAP_FRAME *TrapFrame
    );

BOOLEAN
KiEmulateReference (
    IN OUT PEXCEPTION_RECORD ExceptionRecord,
    IN OUT struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN OUT struct _KTRAP_FRAME *TrapFrame
    );

ULONG
KiGetRegisterValue (
    IN ULONG Register,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame
    );

VOID
KiSetRegisterValue (
    IN ULONG Register,
    IN ULONG Value,
    OUT struct _KEXCEPTION_FRAME *ExceptionFrame,
    OUT struct _KTRAP_FRAME *TrapFrame
    );

VOID
KiRequestSoftwareInterrupt (
    ULONG RequestIrql
    );




// begin_ntddk begin_wdm begin_nthal begin_ntndis

//
// I/O space read and write macros.
//
// **FINISH** Ensure that these are appropriate for PowerPC

#define READ_REGISTER_UCHAR(x) \
    *(volatile UCHAR * const)(x)

#define READ_REGISTER_USHORT(x) \
    *(volatile USHORT * const)(x)

#define READ_REGISTER_ULONG(x) \
    *(volatile ULONG * const)(x)

#define READ_REGISTER_BUFFER_UCHAR(x, y, z) {                           \
    PUCHAR registerBuffer = x;                                          \
    PUCHAR readBuffer = y;                                              \
    ULONG readCount;                                                    \
    for (readCount = z; readCount--; readBuffer++, registerBuffer++) {  \
        *readBuffer = *(volatile UCHAR * const)(registerBuffer);        \
    }                                                                   \
}

#define READ_REGISTER_BUFFER_USHORT(x, y, z) {                          \
    PUSHORT registerBuffer = x;                                         \
    PUSHORT readBuffer = y;                                             \
    ULONG readCount;                                                    \
    for (readCount = z; readCount--; readBuffer++, registerBuffer++) {  \
        *readBuffer = *(volatile USHORT * const)(registerBuffer);       \
    }                                                                   \
}

#define READ_REGISTER_BUFFER_ULONG(x, y, z) {                           \
    PULONG registerBuffer = x;                                          \
    PULONG readBuffer = y;                                              \
    ULONG readCount;                                                    \
    for (readCount = z; readCount--; readBuffer++, registerBuffer++) {  \
        *readBuffer = *(volatile ULONG * const)(registerBuffer);        \
    }                                                                   \
}

#define WRITE_REGISTER_UCHAR(x, y) {    \
    *(volatile UCHAR * const)(x) = y;   \
    KeFlushWriteBuffer();               \
}

#define WRITE_REGISTER_USHORT(x, y) {   \
    *(volatile USHORT * const)(x) = y;  \
    KeFlushWriteBuffer();               \
}

#define WRITE_REGISTER_ULONG(x, y) {    \
    *(volatile ULONG * const)(x) = y;   \
    KeFlushWriteBuffer();               \
}

#define WRITE_REGISTER_BUFFER_UCHAR(x, y, z) {                            \
    PUCHAR registerBuffer = x;                                            \
    PUCHAR writeBuffer = y;                                               \
    ULONG writeCount;                                                     \
    for (writeCount = z; writeCount--; writeBuffer++, registerBuffer++) { \
        *(volatile UCHAR * const)(registerBuffer) = *writeBuffer;         \
    }                                                                     \
    KeFlushWriteBuffer();                                                 \
}

#define WRITE_REGISTER_BUFFER_USHORT(x, y, z) {                           \
    PUSHORT registerBuffer = x;                                           \
    PUSHORT writeBuffer = y;                                              \
    ULONG writeCount;                                                     \
    for (writeCount = z; writeCount--; writeBuffer++, registerBuffer++) { \
        *(volatile USHORT * const)(registerBuffer) = *writeBuffer;        \
    }                                                                     \
    KeFlushWriteBuffer();                                                 \
}

#define WRITE_REGISTER_BUFFER_ULONG(x, y, z) {                            \
    PULONG registerBuffer = x;                                            \
    PULONG writeBuffer = y;                                               \
    ULONG writeCount;                                                     \
    for (writeCount = z; writeCount--; writeBuffer++, registerBuffer++) { \
        *(volatile ULONG * const)(registerBuffer) = *writeBuffer;         \
    }                                                                     \
    KeFlushWriteBuffer();                                                 \
}


#define READ_PORT_UCHAR(x) \
    *(volatile UCHAR * const)(x)

#define READ_PORT_USHORT(x) \
    *(volatile USHORT * const)(x)

#define READ_PORT_ULONG(x) \
    *(volatile ULONG * const)(x)

#define READ_PORT_BUFFER_UCHAR(x, y, z) {                             \
    PUCHAR readBuffer = y;                                            \
    ULONG readCount;                                                  \
    for (readCount = 0; readCount < z; readCount++, readBuffer++) {   \
        *readBuffer = *(volatile UCHAR * const)(x);                   \
    }                                                                 \
}

#define READ_PORT_BUFFER_USHORT(x, y, z) {                            \
    PUSHORT readBuffer = y;                                            \
    ULONG readCount;                                                  \
    for (readCount = 0; readCount < z; readCount++, readBuffer++) {   \
        *readBuffer = *(volatile USHORT * const)(x);                  \
    }                                                                 \
}

#define READ_PORT_BUFFER_ULONG(x, y, z) {                             \
    PULONG readBuffer = y;                                            \
    ULONG readCount;                                                  \
    for (readCount = 0; readCount < z; readCount++, readBuffer++) {   \
        *readBuffer = *(volatile ULONG * const)(x);                   \
    }                                                                 \
}

#define WRITE_PORT_UCHAR(x, y) {        \
    *(volatile UCHAR * const)(x) = y;   \
    KeFlushWriteBuffer();               \
}

#define WRITE_PORT_USHORT(x, y) {       \
    *(volatile USHORT * const)(x) = y;  \
    KeFlushWriteBuffer();               \
}

#define WRITE_PORT_ULONG(x, y) {        \
    *(volatile ULONG * const)(x) = y;   \
    KeFlushWriteBuffer();               \
}

#define WRITE_PORT_BUFFER_UCHAR(x, y, z) {                                \
    PUCHAR writeBuffer = y;                                               \
    ULONG writeCount;                                                     \
    for (writeCount = 0; writeCount < z; writeCount++, writeBuffer++) {   \
        *(volatile UCHAR * const)(x) = *writeBuffer;                      \
        KeFlushWriteBuffer();                                             \
    }                                                                     \
}

#define WRITE_PORT_BUFFER_USHORT(x, y, z) {                               \
    PUSHORT writeBuffer = y;                                              \
    ULONG writeCount;                                                     \
    for (writeCount = 0; writeCount < z; writeCount++, writeBuffer++) {   \
        *(volatile USHORT * const)(x) = *writeBuffer;                     \
        KeFlushWriteBuffer();                                             \
    }                                                                     \
}

#define WRITE_PORT_BUFFER_ULONG(x, y, z) {                                \
    PULONG writeBuffer = y;                                               \
    ULONG writeCount;                                                     \
    for (writeCount = 0; writeCount < z; writeCount++, writeBuffer++) {   \
        *(volatile ULONG * const)(x) = *writeBuffer;                      \
        KeFlushWriteBuffer();                                             \
    }                                                                     \
}

// end_ntddk end_wdm end_nthal end_ntndis


//
// Masks for Dr7 and sanitize macros for various debug registers.
//
#define DR6_LEGAL   0x0000e00f

#define DR7_LEGAL   0xffff0155

#define DR7_ACTIVE  0x00000055  // If any of these bits are set a debug
                                // register is active providing Dr6
                                // indicates DR available
#define SANITIZE_DR6(Dr6, mode) ((Dr6 & DR6_LEGAL));

#define SANITIZE_DR7(Dr7, mode) ((Dr7 & DR7_LEGAL));

#define SANITIZE_DRADDR(DrReg, mode) (                \
    (mode) == KernelMode ?                            \
        (DrReg):                                      \
        (((PVOID)DrReg <= MM_HIGHEST_USER_ADDRESS) ?  \
        (DrReg):                                      \
            (0)                                       \
        )                                             \
    )

// begin_nthal
//
// Trap Frame  --  Volatile state
//
//  N.B. This frame must be a multiple of 8 bytes in length, as the
//       Stack Frame header (see ntppc.h), Trap Frame and Exception Frame
//       together must make up a valid call stack frame.
//

//  CR fields 0, 1, 5..7 are volatile and appear here
//  CR fields 2..4 are non-volatile and appear in the Exception Frame

#define CR_VOLATILE_FIELDS 0xFF000FFFL

typedef struct _KTRAP_FRAME {

    PVOID TrapFrame;                    // previous trap frame address

    UCHAR OldIrql;
    UCHAR PreviousMode;
    UCHAR SavedApcStateIndex;
    UCHAR SavedKernelApcDisable;

// Exception Record embedded in the Trap Frame, on 8-byte boundary,
// padded to multiple of 8 bytes

    UCHAR ExceptionRecord[(sizeof(EXCEPTION_RECORD) + 7) & (~7)];

    ULONG FILL2;

// General registers 0 thru 12

    ULONG  Gpr0;
    ULONG  Gpr1;
    ULONG  Gpr2;
    ULONG  Gpr3;
    ULONG  Gpr4;
    ULONG  Gpr5;
    ULONG  Gpr6;
    ULONG  Gpr7;
    ULONG  Gpr8;
    ULONG  Gpr9;
    ULONG  Gpr10;
    ULONG  Gpr11;
    ULONG  Gpr12;

// Floating point registers 0 thru 13

    DOUBLE Fpr0;                // 8-byte boundary required here
    DOUBLE Fpr1;
    DOUBLE Fpr2;
    DOUBLE Fpr3;
    DOUBLE Fpr4;
    DOUBLE Fpr5;
    DOUBLE Fpr6;
    DOUBLE Fpr7;
    DOUBLE Fpr8;
    DOUBLE Fpr9;
    DOUBLE Fpr10;
    DOUBLE Fpr11;
    DOUBLE Fpr12;
    DOUBLE Fpr13;

// Floating Point Status and Control Register

    DOUBLE Fpscr;

// Other volatile control registers

    ULONG  Cr;     // Only CR fields 0, 1, 5..7 are volatile, but the
                   // entire CR is saved here on interrupt
    ULONG  Xer;
    ULONG  Msr;
    ULONG  Iar;
    ULONG  Lr;
    ULONG  Ctr;

// Debug Registers

    ULONG  Dr0;
    ULONG  Dr1;
    ULONG  Dr2;
    ULONG  Dr3;
    ULONG  Dr4;
    ULONG  Dr5;
    ULONG  Dr6;
    ULONG  Dr7;

} KTRAP_FRAME, *PKTRAP_FRAME;

#define KTRAP_FRAME_LENGTH ((sizeof(KTRAP_FRAME) + 7) & (~7))
#define KTRAP_FRAME_ALIGN (sizeof(DOUBLE))
#define KTRAP_FRAME_ROUND (KTRAP_FRAME_ALIGN - 1)

// end_nthal
//
// The frame saved by KiCallUserMode is defined here to allow
// the kernel debugger to trace the entire kernel stack
// when usermode callouts are pending.
//

typedef struct _KCALLOUT_FRAME {
    STACK_FRAME_HEADER Frame;
    ULONG   CbStk;              // saved callback stack address
    ULONG   TrFr;               // saved callback trap frame address
    ULONG   InStk;              // saved initial stack address
    ULONG   TrIar;              // saved trap IAR
    ULONG   TrToc;              // saved trap TOC
    ULONG   R3;                 // saved R3 (OutputBuffer)
    ULONG   R4;                 // saved R4 (OutputLength)
    ULONG   Lr;                 // saved LR
    ULONG   Gpr[18];            // all nonvolatile GPRs
    DOUBLE  Fpr[18];            // all nonvolatile FPRs
} KCALLOUT_FRAME, *PKCALLOUT_FRAME;

typedef struct _UCALLOUT_FRAME {
    STACK_FRAME_HEADER Frame;
    PVOID Buffer;
    ULONG Length;
    ULONG ApiNumber;
    ULONG Lr;
    ULONG Toc;
    ULONG Pad;
} UCALLOUT_FRAME, *PUCALLOUT_FRAME;


// begin_nthal
//
// Exception frame  --  NON-VOLATILE state
//
// This structure's layout matches that of the registers as saved
// in a call/return stack frame, where the called program has saved
// all the non-volatile registers.
//
// N.B. This frame must be a multiple of 8 bytes in length, as the
//      Stack Frame header (see ntppc.h), Trap Frame and the Exception
//      Frame together must make up a valid call stack frame.
//

typedef struct _KEXCEPTION_FRAME {

    ULONG  Fill1;                       // padding

    ULONG  Gpr13;
    ULONG  Gpr14;
    ULONG  Gpr15;
    ULONG  Gpr16;
    ULONG  Gpr17;
    ULONG  Gpr18;
    ULONG  Gpr19;
    ULONG  Gpr20;
    ULONG  Gpr21;
    ULONG  Gpr22;
    ULONG  Gpr23;
    ULONG  Gpr24;
    ULONG  Gpr25;
    ULONG  Gpr26;
    ULONG  Gpr27;
    ULONG  Gpr28;
    ULONG  Gpr29;
    ULONG  Gpr30;
    ULONG  Gpr31;

    DOUBLE Fpr14;               // 8-byte boundary required here
    DOUBLE Fpr15;
    DOUBLE Fpr16;
    DOUBLE Fpr17;
    DOUBLE Fpr18;
    DOUBLE Fpr19;
    DOUBLE Fpr20;
    DOUBLE Fpr21;
    DOUBLE Fpr22;
    DOUBLE Fpr23;
    DOUBLE Fpr24;
    DOUBLE Fpr25;
    DOUBLE Fpr26;
    DOUBLE Fpr27;
    DOUBLE Fpr28;
    DOUBLE Fpr29;
    DOUBLE Fpr30;
    DOUBLE Fpr31;

} KEXCEPTION_FRAME, *PKEXCEPTION_FRAME;

// end_nthal
//
// Special version of exception frame for use by SwapContext and
// KiInitializeContextThread.
//

typedef struct _KSWAP_FRAME {
    KEXCEPTION_FRAME ExceptionFrame;
    ULONG ConditionRegister;
    ULONG SwapReturn;
} KSWAP_FRAME, *PKSWAP_FRAME;


// begin_ntddk begin_wdm
//
// Non-volatile floating point state
//

typedef struct _KFLOATING_SAVE {
    ULONG   Reserved;
} KFLOATING_SAVE, *PKFLOATING_SAVE;

// end_ntddk end_wdm
//
// Format of stack frame during exceptions.
//

#define STK_SLACK_SPACE 232

typedef struct _KEXCEPTION_STACK_FRAME {
    STACK_FRAME_HEADER Header;
    ULONG AdditionalParameters[8];
    KTRAP_FRAME TrapFrame;
    KEXCEPTION_FRAME ExceptionFrame;
    PVOID Lr;
    PVOID Cr;
    UCHAR SlackSpace[STK_SLACK_SPACE];
} KEXCEPTION_STACK_FRAME, *PKEXCEPTION_STACK_FRAME;

// begin_nthal
// begin_windbgkd

#ifdef _PPC_
//
// Special Registers for PowerPC
//

typedef struct _KSPECIAL_REGISTERS {
    ULONG  KernelDr0;
    ULONG  KernelDr1;
    ULONG  KernelDr2;
    ULONG  KernelDr3;
    ULONG  KernelDr4;
    ULONG  KernelDr5;
    ULONG  KernelDr6;
    ULONG  KernelDr7;
    ULONG  Sprg0;
    ULONG  Sprg1;
    ULONG  Sr0;
    ULONG  Sr1;
    ULONG  Sr2;
    ULONG  Sr3;
    ULONG  Sr4;
    ULONG  Sr5;
    ULONG  Sr6;
    ULONG  Sr7;
    ULONG  Sr8;
    ULONG  Sr9;
    ULONG  Sr10;
    ULONG  Sr11;
    ULONG  Sr12;
    ULONG  Sr13;
    ULONG  Sr14;
    ULONG  Sr15;
    ULONG  DBAT0L;
    ULONG  DBAT0U;
    ULONG  DBAT1L;
    ULONG  DBAT1U;
    ULONG  DBAT2L;
    ULONG  DBAT2U;
    ULONG  DBAT3L;
    ULONG  DBAT3U;
    ULONG  IBAT0L;
    ULONG  IBAT0U;
    ULONG  IBAT1L;
    ULONG  IBAT1U;
    ULONG  IBAT2L;
    ULONG  IBAT2U;
    ULONG  IBAT3L;
    ULONG  IBAT3U;
    ULONG  Sdr1;
    ULONG  Reserved[9];
} KSPECIAL_REGISTERS, *PKSPECIAL_REGISTERS;

//
// Processor State structure.
//

typedef struct _KPROCESSOR_STATE {
    struct _CONTEXT ContextFrame;
    struct _KSPECIAL_REGISTERS SpecialRegisters;
} KPROCESSOR_STATE, *PKPROCESSOR_STATE;

#endif // _PPC_
// end_windbgkd

//
// Processor Control Block (PRCB)
//

#define PRCB_MINOR_VERSION 1
#define PRCB_MAJOR_VERSION 1
#define PRCB_BUILD_DEBUG        0x0001
#define PRCB_BUILD_UNIPROCESSOR 0x0002

struct _RESTART_BLOCK;

typedef struct _KPRCB {

//
// Major and minor version numbers of the PCR.
//

    USHORT MinorVersion;
    USHORT MajorVersion;

//
// Start of the architecturally defined section of the PRCB. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//

    struct _KTHREAD *CurrentThread;
    struct _KTHREAD *RESTRICTED_POINTER NextThread;
    struct _KTHREAD *IdleThread;
    CCHAR Number;
    CCHAR Reserved;
    USHORT BuildType;
    KAFFINITY SetMember;
    struct _RESTART_BLOCK *RestartBlock;
    ULONG PcrPage;
    ULONG PcrPage2;

//
// Space reserved for the system.
//

    ULONG SystemReserved[15];

//
// Space reserved for the HAL.
//

    ULONG HalReserved[16];

// End of the architecturally defined section of the PRCB.
// end_nthal
//

    ULONG DpcTime;
    ULONG InterruptTime;
    ULONG KernelTime;
    ULONG UserTime;
    ULONG AdjustDpcThreshold;
    ULONG InterruptCount;
    ULONG ApcBypassCount;
    ULONG DpcBypassCount;
    ULONG Spare6[5];

//
// MP information.
//

    PVOID Spare1;
    PVOID Spare2;
    volatile ULONG IpiFrozen;
    struct _KPROCESSOR_STATE ProcessorState;

//
//  Per-processor data for various hot code which resides in the
//  kernel image. Each processor is given its own copy of the data
//  to lessen the cache impact of sharing the data between multiple
//  processors.
//

//
//  Spares (formerly fsrtl filelock free lists)
//

    PVOID SpareHotData[2];

//
//  Cache manager performance counters.
//

    ULONG CcFastReadNoWait;
    ULONG CcFastReadWait;
    ULONG CcFastReadNotPossible;
    ULONG CcCopyReadNoWait;
    ULONG CcCopyReadWait;
    ULONG CcCopyReadNoWaitMiss;

//
// Kernel performance counters.
//

    ULONG KeAlignmentFixupCount;
    ULONG KeContextSwitches;
    ULONG KeDcacheFlushCount;
    ULONG KeExceptionDispatchCount;
    ULONG KeFirstLevelTbFills;
    ULONG KeFloatingEmulationCount;
    ULONG KeIcacheFlushCount;
    ULONG KeSecondLevelTbFills;
    ULONG KeSystemCalls;

    ULONG PagedPoolLookasideHits;

//
//  Reserved for future counters.
//

    ULONG ReservedCounter[14];

//
// Reserved pad.
//

    union {
        ULONG ReservedPad[16 * 8];
        PVOID PagedFreeEntry[POOL_SMALL_LISTS];
    };

//
// MP interprocessor request packet and summary.
//
// N.B. This is carefully aligned to be on a cache line boundary.
//

    volatile PVOID CurrentPacket[3];
    volatile KAFFINITY TargetSet;
    volatile PKIPI_WORKER WorkerRoutine;
    ULONG CachePad1[3];

//
// N.B. These two longwords must be on a quadword boundary and adjacent.
//

    volatile ULONG RequestSummary;
    volatile struct _KPRCB *SignalDone;
    ULONG CachePad2[6];

//
// DPC interrupt requested.
//

    ULONG DpcInterruptRequested;
    ULONG CachePad3[7];

//
// DPC batching parameters.
//

    ULONG MaximumDpcQueueDepth;
    ULONG MinimumDpcRate;

//
// Spare counters.
//

    ULONG Spare4[2];

//
// I/O system per processor single entry lookaside lists.
//

    PVOID SmallIrpFreeEntry;
    PVOID LargeIrpFreeEntry;
    PVOID MdlFreeEntry;

//
// Object manager per processor single entry lookaside lists.
//

    PVOID CreateInfoFreeEntry;
    PVOID NameBufferFreeEntry;

//
// Cache manager per processor single entry lookaside lists.
//

    PVOID SharedCacheMapEntry;

//
// Spares.
//

    ULONG Spare5[2];

//
// Address of MP interprocessor operation counters.
//

    PKIPI_COUNTS IpiCounts;
    LARGE_INTEGER StartCount;

//
// DPC list head, spinlock, and count.
//

    KSPIN_LOCK DpcLock;
    LIST_ENTRY DpcListHead;
    ULONG DpcQueueDepth;
    ULONG DpcCount;
    ULONG DpcLastCount;
    ULONG DpcRequestRate;
    ULONG DpcRoutineActive;
    BOOLEAN SkipTick;
    ULONG CachePad4[5];

//
// Processors power state
//
    PROCESSOR_POWER_STATE PowerState;


} KPRCB, *PKPRCB, *RESTRICTED_POINTER PRKPRCB;  // nthal

// begin_ntddk begin_wdm begin_nthal begin_ntndis
//
// PowerPC page size = 4 KB
//

#define PAGE_SIZE (ULONG)0x1000

//
// Define the number of trailing zeroes in a page aligned virtual address.
// This is used as the shift count when shifting virtual addresses to
// virtual page numbers.
//

#define PAGE_SHIFT 12L

// end_ntddk end_wdm end_ntndis

//
// Define the number of bits to shift to right justify the Page Directory Index
// field of a PTE.
//

#define PDI_SHIFT 22

//
// Define the number of bits to shift to right justify the Page Table Index
// field of a PTE.
//

#define PTI_SHIFT 12

// begin_ntddk
//
// The highest user address reserves 64K bytes for a guard page. This
// the probing of address from kernel mode to only have to check the
// starting address for structures of 64k bytes or less.
//

#define MM_HIGHEST_USER_ADDRESS (PVOID)0x7FFEFFFF // highest user address
#define MM_SYSTEM_RANGE_START (PVOID)KSEG0_BASE // start of system space
#define MM_USER_PROBE_ADDRESS 0x7FFF0000 // starting address of guard page

//
// The following definitions are required for the debugger data block.
//

extern PVOID MmHighestUserAddress;
extern PVOID MmSystemRangeStart;
extern ULONG MmUserProbeAddress;

//
// The lowest user address reserves the low 64k.
//

#define MM_LOWEST_USER_ADDRESS  (PVOID)0x00010000

// begin_wdm

#define MmGetProcedureAddress(Address) *((PVOID *)(Address))
#define MmLockPagableCodeSection(Address) MmLockPagableDataSection(*((PVOID *)(Address)))

// end_ntddk end_wdm
//
// Define the page table and the page directory base for
// memory management.
//

#define PDE_BASE (ULONG)0xC0300000
#define PTE_BASE (ULONG)0xC0000000

// begin_ntddk begin_wdm
//
// The lowest address for system space.
//

#define MM_LOWEST_SYSTEM_ADDRESS (PVOID)0x80000000
#define SYSTEM_BASE 0x80000000          // start of system space (no typecast)

// begin_ntndis
#endif // defined(_PPC_)
// end_ntddk end_wdm end_nthal end_ntndis
// Special comment moved for hal.h since mips.h defines UNCACHED_POLICY
// in  unconditional code which is placed in the hal header file.

//
// Define uncache policies.
//
// **FINISH** Check that these values are even needed for PPC.
//

#define UNCACHED_POLICY 2               // uncached

//
// Registers visible only to the operating system
//

//
// Define Data Storage Interrupt Status Register (DSISR)
//

typedef struct _DSISR {
    ULONG UpdateReg :  5;  // RA field for update-form instrs
    ULONG DataReg   :  5;  // RA, RS, FRA, or FRT field of instr
    ULONG Index     :  7;  // Index into table to distinguish instrs
    ULONG Fill2     :  1;
    ULONG XO        :  2;  // Extended op-code for DS-form instrs
    ULONG Fill1     : 12;
} DSISR, *PDSISR;

DSISR KiGetDsisr ();             // Function to read the DSISR
void  KiSetDsisr (DSISR Value);  // Function to write the DSISR

//
// PowerPC function definitions
//

//++
//
// BOOLEAN
// KiIsThreadNumericStateSaved(
//     IN PKTHREAD Address
//     )
//
//  This call is used on a not running thread to see if its numeric
//  state has been saved in its context information.
//
//  **FINISH**  PowerPC is eventually to use lazy FP state-save,
//              but for now we'll always save the FP state.
//
//--

#define KiIsThreadNumericStateSaved(a)      TRUE

//++
//
// VOID
// KiRundownThread(
//     IN PKTHREAD Address
//     )
//
//--

#define KiRundownThread(a)

//
// Define macro to test if x86 feature is present.
//
// N.B. All x86 features test TRUE on PPC systems.
//

#define Isx86FeaturePresent(_f_) TRUE

//
// Symbolic values for exception entry
//
#define ppc_machine_check               1
#define ppc_data_storage                2
#define ppc_instruction_storage         3
#define ppc_external                    4
#define ppc_alignment                   5
#define ppc_program                     6
#define ppc_fp_unavailable              7
#define ppc_decrementer                 8
#define ppc_direct_store_error          9
#define ppc_syscall                     10
#define ppc_trace                       11
#define ppc_fp_assist                   12
#define ppc_run_mode                    13

#endif // _PPCH_

/*++ BUILD Version: 0018    // Increment this if a change has global effects

Copyright (c) 1990  Microsoft Corporation

Module Name:

    mips.h

Abstract:

    This module contains the Mips hardware specific header file.

Author:

    David N. Cutler (davec) 31-Mar-1990

Revision History:

--*/

#ifndef _MIPSH_
#define _MIPSH_

// begin_ntddk begin_wdm begin_nthal begin_ntndis

#if defined(_MIPS_)

//
// Define maximum size of flush multple TB request.
//

#define FLUSH_MULTIPLE_MAXIMUM 16

//
// Indicate that the MIPS compiler supports the pragma textout construct.
//

#define ALLOC_PRAGMA 1

//
// Define function decoration depending on whether a driver, a file system,
// or a kernel component is being built.
//

//  end_wdm

#if (defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_)) && !defined (_BLDR_)

#define NTKERNELAPI DECLSPEC_IMPORT         // wdm

#else

#define NTKERNELAPI

#endif

//
// Define function decoration depending on whether the HAL or other kernel
// component is being build.
//

#if !defined(_NTHAL_) && !defined(_BLDR_)

#define NTHALAPI DECLSPEC_IMPORT        // wdm

#else

#define NTHALAPI

#endif

// end_ntndis
//
// Define macro to generate import names.
//

#define IMPORT_NAME(name) __imp_##name

//
// MIPS specific interlocked operation result values.
//

#define RESULT_ZERO 0
#define RESULT_NEGATIVE -2
#define RESULT_POSITIVE -1

//
// Interlocked result type is portable, but its values are machine specific.
// Constants for value are in i386.h, mips.h, etc.
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
    ExMipsInterlockedIncrementLong(Addend)

#define ExInterlockedDecrementLong(Addend, Lock) \
    ExMipsInterlockedDecrementLong(Addend)

#define ExInterlockedExchangeAddLargeInteger(Target, Value, Lock) \
    ExpInterlockedExchangeAddLargeInteger(Target, Value)

#define ExInterlockedExchangeUlong(Target, Value, Lock) \
    ExMipsInterlockedExchangeUlong(Target, Value)

NTKERNELAPI
INTERLOCKED_RESULT
ExMipsInterlockedIncrementLong (
    IN PLONG Addend
    );

NTKERNELAPI
INTERLOCKED_RESULT
ExMipsInterlockedDecrementLong (
    IN PLONG Addend
    );

NTKERNELAPI
LARGE_INTEGER
ExpInterlockedExchangeAddLargeInteger (
    IN PLARGE_INTEGER Addend,
    IN LARGE_INTEGER Increment
    );

NTKERNELAPI
ULONG
ExMipsInterlockedExchangeUlong (
    IN PULONG Target,
    IN ULONG Value
    );

//  begin_wdm

//
// Intrinsic interlocked functions.
//

#if defined(_M_MRX000) && !defined(RC_INVOKED)

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

#if _MSC_VER >= 1100

#define InterlockedCompareExchange64 _InterlockedCompareExchange64

ULONGLONG
InterlockedCompareExchange64 (
    IN PULONGLONG Destination,
    IN PULONGLONG Exchange,
    IN PULONGLONG Comperand
    );

#pragma intrinsic(_InterlockedCompareExchange64)

#else

#define InterlockedCompareExchange64 ExpInterlockedCompareExchange64

NTKERNELAPI
ULONGLONG
ExpInterlockedCompareExchange64 (
    IN PULONGLONG Destination,
    IN PULONGLONG Exchange,
    IN PULONGLONG Comperand
    );

#endif

#endif

//
// MIPS Interrupt Definitions.
//
// Define length on interupt object dispatch code in longwords.
//

#define DISPATCH_LENGTH 4               // Length of dispatch code in instructions

//
// Define Interrupt Request Levels.
//

#define PASSIVE_LEVEL 0                 // Passive release level
#define LOW_LEVEL 0                     // Lowest interrupt level
#define APC_LEVEL 1                     // APC interrupt level
#define DISPATCH_LEVEL 2                // Dispatcher level
#define IPI_LEVEL 7                     // Interprocessor interrupt level
#define POWER_LEVEL 7                   // Power failure level
#define PROFILE_LEVEL 8                 // Profiling level
#define HIGH_LEVEL 8                    // Highest interrupt level
#define SYNCH_LEVEL (IPI_LEVEL - 1)     // synchronization level

//
// Define profile intervals.
//

#define DEFAULT_PROFILE_COUNT 0x40000000 // ~= 20 seconds @50mhz
#define DEFAULT_PROFILE_INTERVAL (10 * 500) // 500 microseconds
#define MAXIMUM_PROFILE_INTERVAL (10 * 1000 * 1000) // 1 second
#define MINIMUM_PROFILE_INTERVAL (10 * 40) // 40 microseconds

// end_ntddk end_wdm end_nthal

#define KiProfileIrql PROFILE_LEVEL     // enable portable code

//
// Define machine specific external references.
//

extern ULONG KiInterruptTemplate[];

//
// Sanitize FSR and PSR based on processor mode.
//
// If kernel mode, then
//      let caller specify all bits
//
// If user mode, then
//      let the caller specify CC EV EZ EO EU EI SV SZ SO SU SI RM
//

#define SANITIZE_FSR(fsr, mode) ( \
    ((mode) == KernelMode ? \
        ((0x00000000L) | ((fsr) & 0xffffffff)) : \
        ((0x00000000L) | ((fsr) & 0xfffc0fff))))

//
// Define SANITIZE_PSR for R4000
//
// If kernel mode, then
//      force clearing of ERL, SX, RP, FR, RE, and all diagnostic bits,
//      force the setting of CU3, CU1, KX, and EXL
//      let caller specify CU2, CU0, INTMASK, UX, KSU, and EXL
//
// If user mode, then
//      force clearing of ERL, SX, RP, FR, RE, and all diagnostic bits,
//      force the setting of CU3, CU1, INTMASK, KX, UX, KSU, EXL, IE, and
//      let caller specify no bits.
//

#define SANITIZE_PSR(psr, mode) ( \
    ((mode) == KernelMode ? \
        ((0xa0000082L) | ((psr) & 0xd000ff33)) : \
        ((0xa00000b3L) | (PCR->IrqlTable[PASSIVE_LEVEL] << 8))))

// begin_nthal
//
// Define Address of Processor Control Registers.
//

#define KIPCR 0xfffff000            // kernel address of first PCR
#define KIPCR2 0xffffe000           // kernel address of second PCR

//
// Define Pointer to Processor Control Registers.
//

#define PCR ((volatile KPCR * const)KIPCR)
#define SharedUserData ((KUSER_SHARED_DATA * const)KIPCR2)

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

// begin_ntddk

//
// Get current processor number.
//

#define KeGetCurrentProcessorNumber() PCR->Number

// begin_wdm
//
// Get data cache fill size.
//

#define KeGetDcacheFillSize() PCR->DcacheFillSize

// end_ntddk end_wdm end_nthal

//
// Get previous processor mode.
//

#define KeGetPreviousMode() (KPROCESSOR_MODE)PCR->CurrentThread->PreviousMode

//
// Test if executing a DPC.
//

#define KeIsExecutingDpc() (PCR->DpcRoutineActive != 0)

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

NTKERNELAPI
VOID
KeFillLargeEntryTb (
    IN HARDWARE_PTE Pte[2],
    IN PVOID Virtual,
    IN ULONG PageSize
    );

//
// Fill TB fixed entry
//

NTKERNELAPI
VOID
KeFillFixedEntryTb (
    IN HARDWARE_PTE Pte[2],
    IN PVOID Virtual,
    IN ULONG Index
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
KeUpdateRuntime (
    IN struct _KTRAP_FRAME *TrapFrame
    );

NTKERNELAPI
VOID
KeUpdateSystemTime (
    IN struct _KTRAP_FRAME *TrapFrame
    );

//
// The following function prototypes are exported for use in MP HALs.
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

//
// Define cache error routine type and prototype.
//

typedef
VOID
(*PKCACHE_ERROR_ROUTINE) (
    VOID
    );

#define CACHE_ERROR_VECTOR 0xa0000ffc   // address of cache error routine

NTKERNELAPI
VOID
KeSetCacheErrorRoutine (
    IN PKCACHE_ERROR_ROUTINE Routine
    );

// end_nthal

//
// Define executive macros for acquiring and releasing executive spinlocks.
// These macros can ONLY be used by executive components and NOT by drivers.
// Drivers MUST use the kernel interfaces since they must be MP enabled on
// all systems.
//
// KeRaiseIrql is one instruction shorter than KeAcquireSpinLock on MIPS UP.
// KeLowerIrql and KeReleaseSpinLock are the same.
//

#if defined(NT_UP) && !defined(_NTDDK_) && !defined(_NTIFS_)
#define ExAcquireSpinLock(Lock, OldIrql) \
    *(OldIrql) = KeRaiseIrqlToDpcLevel()

#define ExReleaseSpinLock(Lock, OldIrql) KeLowerIrql((OldIrql))
#define ExAcquireSpinLockAtDpcLevel(Lock)
#define ExReleaseSpinLockFromDpcLevel(Lock)
#else

//  begin_wdm begin_ntddk

#define ExAcquireSpinLock(Lock, OldIrql) \
    *(OldIrql) = KeAcquireSpinLockRaiseToDpc((Lock))

#define ExReleaseSpinLock(Lock, OldIrql) KeReleaseSpinLock((Lock), (OldIrql))
#define ExAcquireSpinLockAtDpcLevel(Lock) KeAcquireSpinLockAtDpcLevel(Lock)
#define ExReleaseSpinLockFromDpcLevel(Lock) KeReleaseSpinLockFromDpcLevel(Lock)

//  end_wdm end_ntddk

#endif

//
// The acquire and release fast lock macros disable and enable interrupts
// on UP nondebug systems. On MP or debug systems, the spinlock routines
// are used.
//
// N.B. Extreme caution should be observed when using these routines.
//
// begin_nthal

#if defined(_M_MRX000)

VOID
_disable (
    VOID
    );

VOID
_enable (
    VOID
    );

#pragma intrinsic(_disable)
#pragma intrinsic(_enable)

#endif

// end_nthal

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
// Data and instruction bus error function prototypes.
//

BOOLEAN
KeBusError (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame,
    IN PVOID VirtualAddress,
    IN PHYSICAL_ADDRESS PhysicalAddress
    );

VOID
KiDataBusError (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame
    );

VOID
KiInstructionBusError (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame
    );

//
// Define query system time macro.
//
// N.B. This macro can be changed when the compiler generates real double
//      integer instructions.
//

#define KiQuerySystemTime(CurrentTime) \
    (CurrentTime)->QuadPart = SharedUserData->SystemTime.Alignment

//
// Define query tick count macro.
//

#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_)

//  begin_wdm begin_ntddk

#define KeQueryTickCount(CurrentCount) {                           \
    PKSYSTEM_TIME _TickCount = *((PKSYSTEM_TIME *)(&KeTickCount)); \
    (CurrentCount)->QuadPart = _TickCount->Alignment;              \
}

//  end_wdm end_ntddk

#else

// begin_nthal
#define KiQueryTickCount(CurrentCount)                             \
    (CurrentCount)->QuadPart = KeTickCount.Alignment

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

#define KiQueryInterruptTime(CurrentTime) \
    (CurrentTime)->QuadPart = SharedUserData->InterruptTime.Alignment

//
// The following function prototypes must be in the module since they are
// machine dependent.
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

ULONGLONG
KiGetRegisterValue64 (
    IN ULONG Register,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame
    );

VOID
KiSetRegisterValue64 (
    IN ULONG Register,
    IN ULONGLONG Value,
    OUT struct _KEXCEPTION_FRAME *ExceptionFrame,
    OUT struct _KTRAP_FRAME *TrapFrame
    );

NTKERNELAPI
VOID
KiRequestSoftwareInterrupt (
    ULONG RequestIrql
    );

//
// 64-bit Probe function definitions
//
// Probe for read function.
//
//++
//
// VOID
// ProbeForRead64(
//     IN PVOID64 Address,
//     IN ULONG Length,
//     IN ULONG Alignment
//     )
//
//--

#define ProbeForRead64(Address, Length, Alignment)                           \
    ASSERT(((Alignment) == 1) || ((Alignment) == 2) ||                       \
           ((Alignment) == 4) || ((Alignment) == 8));                        \
                                                                             \
    if ((Length) != 0) {                                                     \
        if (((ULONGLONG)(Address) & ((Alignment) - 1)) != 0) {               \
            ExRaiseDatatypeMisalignment();                                   \
                                                                             \
        } else if ((((ULONGLONG)(Address) + (Length)) < (ULONGLONG)(Address)) || \
                   (((ULONGLONG)(Address) + (Length)) > (ULONGLONG)MM_HIGHEST_USER_ADDRESS64)) { \
            ExRaiseAccessViolation();                                        \
        }                                                                    \
    }

//
// Probe for write function.
//

NTKERNELAPI
VOID
ProbeForWrite64 (
    IN PVOID64 Address,
    IN ULONG Length,
    IN ULONG Alignment
    );

// begin_ntddk begin_wdm begin_nthal begin_ntndis
//
// I/O space read and write macros.
//

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

// end_ntddk end_wdm end_ntndis

//
// Exception frame
//
//  N.B. This frame must be an exact multiple of 8 bytes in length.
//

typedef struct _KEXCEPTION_FRAME {
    union {
        ULONG Argument[8];
        DOUBLE Alignment;
    };

    //
    // Floating nonvolatile context.
    //

    union {

        //
        // 16 double floating register nonvolatile context.
        //

        struct {
            ULONG FltF20;
            ULONG FltF21;
            ULONG FltF22;
            ULONG FltF23;
            ULONG FltF24;
            ULONG FltF25;
            ULONG FltF26;
            ULONG FltF27;
            ULONG FltF28;
            ULONG FltF29;
            ULONG FltF30;
            ULONG FltF31;
        };

        //
        // 32 double floating register nonvolatile context.
        //

        struct {
            ULONGLONG XFltF20;
            ULONGLONG XFltF22;
            ULONGLONG XFltF24;
            ULONGLONG XFltF26;
            ULONGLONG XFltF28;
            ULONGLONG XFltF30;
        };
    };

    //
    // Integer nonvolatile context.
    //

    ULONG IntS0;
    ULONG IntS1;
    ULONG IntS2;
    ULONG IntS3;
    ULONG IntS4;
    ULONG IntS5;
    ULONG IntS6;
    ULONG IntS7;
    ULONG IntS8;
    ULONG SwapReturn;
    ULONG IntRa;
} KEXCEPTION_FRAME, *PKEXCEPTION_FRAME;

//
// Trap frame
//
//  N.B. This frame must be EXACTLY a multiple of 16 bytes in length.
//

typedef struct _KTRAP_FRAME {
    union {
        ULONG Argument[4];
        ULONGLONG Alignment;
    };

    //
    // Volatile floating state.
    //

    union {

        //
        // 32-bit floating state.
        //

        struct {
            ULONG FltF0;
            ULONG FltF1;
            ULONG FltF2;
            ULONG FltF3;
            ULONG FltF4;
            ULONG FltF5;
            ULONG FltF6;
            ULONG FltF7;
            ULONG FltF8;
            ULONG FltF9;
            ULONG FltF10;
            ULONG FltF11;
            ULONG FltF12;
            ULONG FltF13;
            ULONG FltF14;
            ULONG FltF15;
            ULONG FltF16;
            ULONG FltF17;
            ULONG FltF18;
            ULONG FltF19;
        };

        //
        // 64-bit floating state.
        //

        struct {
            ULONGLONG XFltF0;
            ULONGLONG XFltF1;
            ULONGLONG XFltF2;
            ULONGLONG XFltF3;
            ULONGLONG XFltF4;
            ULONGLONG XFltF5;
            ULONGLONG XFltF6;
            ULONGLONG XFltF7;
            ULONGLONG XFltF8;
            ULONGLONG XFltF9;
            ULONGLONG XFltF10;
            ULONGLONG XFltF11;
            ULONGLONG XFltF12;
            ULONGLONG XFltF13;
            ULONGLONG XFltF14;
            ULONGLONG XFltF15;
            ULONGLONG XFltF16;
            ULONGLONG XFltF17;
            ULONGLONG XFltF18;
            ULONGLONG XFltF19;
            ULONGLONG XFltF21;
            ULONGLONG XFltF23;
            ULONGLONG XFltF25;
            ULONGLONG XFltF27;
            ULONGLONG XFltF29;
            ULONGLONG XFltF31;
        };
    };

    //
    // Volatile 64-bit integer state.
    //
    //

    struct {
        ULONGLONG XIntZero;
        ULONGLONG XIntAt;
        ULONGLONG XIntV0;
        ULONGLONG XIntV1;
        ULONGLONG XIntA0;
        ULONGLONG XIntA1;
        ULONGLONG XIntA2;
        ULONGLONG XIntA3;
        ULONGLONG XIntT0;
        ULONGLONG XIntT1;
        ULONGLONG XIntT2;
        ULONGLONG XIntT3;
        ULONGLONG XIntT4;
        ULONGLONG XIntT5;
        ULONGLONG XIntT6;
        ULONGLONG XIntT7;
        ULONGLONG XIntS0;
        ULONGLONG XIntS1;
        ULONGLONG XIntS2;
        ULONGLONG XIntS3;
        ULONGLONG XIntS4;
        ULONGLONG XIntS5;
        ULONGLONG XIntS6;
        ULONGLONG XIntS7;
        ULONGLONG XIntT8;
        ULONGLONG XIntT9;
        ULONGLONG XIntK0;
        ULONGLONG XIntK1;
        ULONGLONG XIntGp;
        ULONGLONG XIntSp;
        ULONGLONG XIntS8;
        ULONGLONG XIntRa;
        ULONGLONG XIntLo;
        ULONGLONG XIntHi;
    };

    ULONG Fsr;
    ULONG Fir;
    ULONG Psr;
    UCHAR ExceptionRecord[(sizeof(EXCEPTION_RECORD) + 7) & (~7)];
    UCHAR OldIrql;
    UCHAR PreviousMode;
    UCHAR SavedFlag;
    union {
        ULONG OnInterruptStack;
        ULONG TrapFrame;
    } u;

} KTRAP_FRAME, *PKTRAP_FRAME;

#define KTRAP_FRAME_ARGUMENTS (4 * 16)
#define KTRAP_FRAME_LENGTH (sizeof(KTRAP_FRAME))
#define KTRAP_FRAME_ALIGN (sizeof(DOUBLE))
#define KTRAP_FRAME_ROUND (KTRAP_FRAME_ALIGN - 1)

//
// Define the kernel mode and user mode callback frame structures.
//

typedef struct _KCALLOUT_FRAME {
    ULONG   SaveArgs[4];            // argument register save area
    ULONG   F20;                    // saved floating registers f20 - f31
    ULONG   F21;                    //
    ULONG   F22;                    //
    ULONG   F23;                    //
    ULONG   F24;                    //
    ULONG   F25;                    //
    ULONG   F26;                    //
    ULONG   F27;                    //
    ULONG   F28;                    //
    ULONG   F29;                    //
    ULONG   F30;                    //
    ULONG   F31;                    //
    ULONG   S0;                     // saved integer registers s0 - s8
    ULONG   S1;                     //
    ULONG   S2;                     //
    ULONG   S3;                     //
    ULONG   S4;                     //
    ULONG   S5;                     //
    ULONG   S6;                     //
    ULONG   S7;                     //
    ULONG   S8;                     //
    ULONG   CbStk;                  // saved callback stack address
    ULONG   TrFr;                   // saved callback trap frame address
    ULONG   Fsr;                    // saved floating status
    ULONG   InStk;                  // save initial stack address
    ULONG   Ra;                     // saved return address
    ULONG   A0;                     // saved argument registers a0-a1
    ULONG   A1;                     //
} KCALLOUT_FRAME, *PKCALLOUT_FRAME;

typedef struct _UCALLOUT_FRAME {
    ULONG SaveArgs[4];
    PVOID Buffer;
    ULONG Length;
    ULONG ApiNumber;
    ULONG Pad;
    LONGLONG Sp;
    LONGLONG Ra;
} UCALLOUT_FRAME, *PUCALLOUT_FRAME;

// begin_ntddk begin_wdm
//
// Non-volatile floating point state
//

typedef struct _KFLOATING_SAVE {
    ULONG   Reserved;
} KFLOATING_SAVE, *PKFLOATING_SAVE;

// end_ntddk end_wdm
//
// Processor State structure.
//

typedef struct _TB_ENTRY {
    ENTRYLO Entrylo0;
    ENTRYLO Entrylo1;
    ENTRYHI Entryhi;
    PAGEMASK Pagemask;
} TB_ENTRY, *PTB_ENTRY;

typedef struct _KPROCESSOR_STATE {
    struct _CONTEXT ContextFrame;
    TB_ENTRY TbEntry[64];
} KPROCESSOR_STATE, *PKPROCESSOR_STATE;

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

//
// Space reserved for the system.
//

    ULONG SystemReserved[16];

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
    PVOID Spare3[3];

//
//  Per-processor data for various hot code which resides in the
//  kernel image. Each processor is given it's own copy of the data
//  to lessen the cache impact of sharing the data between multiple
//  processors.
//

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

//
//  Reserved for future counters.
//

    ULONG ReservedCounter[8];

//
// Reserved pad.
//

    ULONG ReservedPad[16 * 8];

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
// Define the page size for the MIPS R4000 as 4096 (0x1000).
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

#define MmGetProcedureAddress(Address) (Address)
#define MmLockPagableCodeSection(Address) MmLockPagableDataSection(Address)

// end_ntddk end_wdm
//
// Define the page table base and the page directory base for
// the TB miss routines and memory management.
//

#define PDE_BASE (ULONG)0xC0300000
#define PTE_BASE (ULONG)0xC0000000
#define PDE64_BASE (ULONG)0xC0302000
#define PTE64_BASE (ULONG)0xC0800000

// begin_ntddk begin_wdm
//
// The lowest address for system space.
//

#define MM_LOWEST_SYSTEM_ADDRESS (PVOID)0xC0800000
#define SYSTEM_BASE 0xc0800000          // start of system space (no typecast)

// begin_ntndis
#endif // defined(_MIPS_)
// end_nthal end_ntddk end_wdm end_ntndis
// begin_nthal
//
// Define uncached policy for the r4000.
//

#define UNCACHED_POLICY 2               // uncached

// end_nthal

//
// MIPS function definitions
//

//++
//
// BOOLEAN
// KiIsThreadNumericStateSaved(
//     IN PKTHREAD Address
//     )
//
//  This call is used on a not running thread to see if it's numeric
//  state has been saved in it's context information.  On mips the
//  numeric state is always saved.
//
//--

#define KiIsThreadNumericStateSaved(a) TRUE

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
// N.B. All x86 features test TRUE on MIPS systems.
//

#define Isx86FeaturePresent(_f_) TRUE

#endif // _MIPSH_

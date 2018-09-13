/*++ BUILD Version: 0014    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    i386.h

Abstract:

    This module contains the i386 hardware specific header file.

Author:

    David N. Cutler (davec) 2-Aug-1989

Revision History:

    25-Jan-1990    shielint

                   Added definitions for 8259 ports and commands and
                   macros for 8259 irq# and system irql conversion.

--*/

#ifndef _i386_
#define _i386_


// begin_ntddk begin_wdm begin_nthal begin_ntndis

#if defined(_X86_)

//
// Types to use to contain PFNs and their counts.
//

typedef ULONG PFN_COUNT;

typedef LONG SPFN_NUMBER, *PSPFN_NUMBER;
typedef ULONG PFN_NUMBER, *PPFN_NUMBER;

//
// Define maximum size of flush multiple TB request.
//

#define FLUSH_MULTIPLE_MAXIMUM 16

//
// Indicate that the i386 compiler supports the pragma textout construct.
//

#define ALLOC_PRAGMA 1
//
// Indicate that the i386 compiler supports the DATA_SEG("INIT") and
// DATA_SEG("PAGE") pragmas
//

#define ALLOC_DATA_PRAGMA 1

// end_ntddk end_nthal end_ntndis end_wdm


//  NOTE -  KiPcr is only useful for PCR references where we know we
//          won't get context switched between the call to it and the
//          variable reference, OR, were we don't care, (ie TEB pointer)

//  BUGBUG bryanwi 11 june 90 - we must not macro out things we export
//      Things like KeFlushIcache and KeFlushDcache cannot be macroed
//      out because external code (like drivers) will want to import
//      them by name.  Therefore, the defines below that turn them into
//      nothing are inappropriate.  But this isn't going to hurt us right
//      now.

//  BUGBUG kenr - remove this PIC stuff from i386.h!


//
// Interrupt controller register addresses.
//

#define PIC1_PORT0 0x20         // master PIC
#define PIC1_PORT1 0x21
#define PIC2_PORT0 0x0A0        // slave PIC
#define PIC2_PORT1 0x0A1

//
// Commands for Interrupt Controller
//

#define PIC1_EOI_MASK 0x60
#define PIC2_EOI      0x62
#define OCW2_NON_SPECIFIC_EOI 0x20
#define OCW3_READ_ISR 0xb
#define OCW3_READ_IRR 0xa


//
// Length on interrupt object dispatch code in longwords.
// BUGBUG shielint Reserve 9*4 space for ABIOS stack mapping.  If NO
//        ABIOS support the size of DISPATCH_LENGTH should be 74.
//

// begin_nthal

#define NORMAL_DISPATCH_LENGTH 106                  // ntddk wdm
#define DISPATCH_LENGTH NORMAL_DISPATCH_LENGTH      // ntddk wdm


//
// Define constants to access the bits in CR0.
//

#define CR0_PG  0x80000000          // paging
#define CR0_ET  0x00000010          // extension type (80387)
#define CR0_TS  0x00000008          // task switched
#define CR0_EM  0x00000004          // emulate math coprocessor
#define CR0_MP  0x00000002          // math present
#define CR0_PE  0x00000001          // protection enable

//
// More CR0 bits; these only apply to the 80486.
//

#define CR0_CD  0x40000000          // cache disable
#define CR0_NW  0x20000000          // not write-through
#define CR0_AM  0x00040000          // alignment mask
#define CR0_WP  0x00010000          // write protect
#define CR0_NE  0x00000020          // numeric error

//
// CR4 bits;  These only apply to Pentium
//
#define CR4_VME 0x00000001          // V86 mode extensions
#define CR4_PVI 0x00000002          // Protected mode virtual interrupts
#define CR4_TSD 0x00000004          // Time stamp disable
#define CR4_DE  0x00000008          // Debugging Extensions
#define CR4_PSE 0x00000010          // Page size extensions
#define CR4_PAE 0x00000020          // Physical address extensions
#define CR4_MCE 0x00000040          // Machine check enable
#define CR4_PGE 0x00000080          // Page global enable
#define CR4_FXSR 0x00000200         // FXSR used by OS
#define CR4_XMMEXCPT 0x00000400     // XMMI used by OS

// begin_ntddk begin_wdm
//
// STATUS register for each MCA bank.
//

typedef union _MCI_STATS {
    struct {
        USHORT  McaCod;
        USHORT  MsCod;
        ULONG   OtherInfo : 25;
        ULONG   Damage : 1;
        ULONG   AddressValid : 1;
        ULONG   MiscValid : 1;
        ULONG   Enabled : 1;
        ULONG   UnCorrected : 1;
        ULONG   OverFlow : 1;
        ULONG   Valid : 1;
    } MciStats;

    ULONGLONG QuadPart;

} MCI_STATS, *PMCI_STATS;

// end_ntddk end_wdm
// end_nthal

//
// Define constants to access ThNpxState
//

#define NPX_STATE_NOT_LOADED    (CR0_TS | CR0_MP)
#define NPX_STATE_LOADED        0

//
// External references to the labels defined in int.asm
//

extern ULONG KiInterruptTemplate[NORMAL_DISPATCH_LENGTH];
extern PULONG KiInterruptTemplateObject;
extern PULONG KiInterruptTemplateDispatch;
extern PULONG KiInterruptTemplate2ndDispatch;

// begin_ntddk begin_wdm begin_nthal
//
// Interrupt Request Level definitions
//

#define PASSIVE_LEVEL 0             // Passive release level
#define LOW_LEVEL 0                 // Lowest interrupt level
#define APC_LEVEL 1                 // APC interrupt level
#define DISPATCH_LEVEL 2            // Dispatcher level

#define PROFILE_LEVEL 27            // timer used for profiling.
#define CLOCK1_LEVEL 28             // Interval clock 1 level - Not used on x86
#define CLOCK2_LEVEL 28             // Interval clock 2 level
#define IPI_LEVEL 29                // Interprocessor interrupt level
#define POWER_LEVEL 30              // Power failure level
#define HIGH_LEVEL 31               // Highest interrupt level
#define SYNCH_LEVEL (IPI_LEVEL-1)   // synchronization level
// end_ntddk end_wdm

#define KiSynchIrql SYNCH_LEVEL     // enable portable code

//
// Machine type definitions
// BUGBUG shielint This is temporary definitions.
//

#define MACHINE_TYPE_ISA 0
#define MACHINE_TYPE_EISA 1
#define MACHINE_TYPE_MCA 2

// end_nthal
//
//  The previous values and the following are or'ed in KeI386MachineType.
//  The latter section can be removed once PC/AT style computers become
//  dominant in Japan. (DavidGoe)
//

#define MACHINE_TYPE_PC_AT_COMPATIBLE      0x00000000
#define MACHINE_TYPE_PC_9800_COMPATIBLE    0x00000100
#define MACHINE_TYPE_FMR_COMPATIBLE        0x00000200

extern ULONG KeI386MachineType;

// begin_nthal
//
// Define constants used in selector tests.
//
//  RPL_MASK is the real value for extracting RPL values.  IT IS THE WRONG
//  CONSTANT TO USE FOR MODE TESTING.
//
//  MODE_MASK is the value for deciding the current mode.
//  WARNING:    MODE_MASK assumes that all code runs at either ring-0
//              or ring-3.  Ring-1 or Ring-2 support will require changing
//              this value and all of the code that refers to it.

#define MODE_MASK    1
#define RPL_MASK     3

//
// SEGMENT_MASK is used to throw away trash part of segment.  Part always
// pushes or pops 32 bits to/from stack, but if it's a segment value,
// high order 16 bits are trash.
//

#define SEGMENT_MASK    0xffff

//
// Startup count value for KeStallExecution.  This value is used
// until KiInitializeStallExecution can compute the real one.
// Pick a value long enough for very fast processors.
//

#define INITIAL_STALL_COUNT 100

// end_nthal

//
// begin_nthal
//
// Macro to extract the high word of a long offset
//

#define HIGHWORD(l) \
    ((USHORT)(((ULONG)(l)>>16) & 0xffff))

//
// Macro to extract the low word of a long offset
//

#define LOWWORD(l) \
    ((USHORT)((ULONG)l & 0x0000ffff))

//
// Macro to combine two USHORT offsets into a long offset
//

#if !defined(MAKEULONG)

#define MAKEULONG(x, y) \
    (((((ULONG)(x))<<16) & 0xffff0000) | \
    ((ULONG)(y) & 0xffff))

#endif

// end_nthal

//
// Request a software interrupt.
//

#define KiRequestSoftwareInterrupt(RequestIrql) \
    HalRequestSoftwareInterrupt( RequestIrql )

// begin_ntddk begin_wdm begin_nthal begin_ntndis

//
// I/O space read and write macros.
//
//  These have to be actual functions on the 386, because we need
//  to use assembler, but cannot return a value if we inline it.
//
//  The READ/WRITE_REGISTER_* calls manipulate I/O registers in MEMORY space.
//  (Use x86 move instructions, with LOCK prefix to force correct behavior
//   w.r.t. caches and write buffers.)
//
//  The READ/WRITE_PORT_* calls manipulate I/O registers in PORT space.
//  (Use x86 in/out instructions.)
//

NTHALAPI
UCHAR
READ_REGISTER_UCHAR(
    PUCHAR  Register
    );

NTHALAPI
USHORT
READ_REGISTER_USHORT(
    PUSHORT Register
    );

NTHALAPI
ULONG
READ_REGISTER_ULONG(
    PULONG  Register
    );

NTHALAPI
VOID
READ_REGISTER_BUFFER_UCHAR(
    PUCHAR  Register,
    PUCHAR  Buffer,
    ULONG   Count
    );

NTHALAPI
VOID
READ_REGISTER_BUFFER_USHORT(
    PUSHORT Register,
    PUSHORT Buffer,
    ULONG   Count
    );

NTHALAPI
VOID
READ_REGISTER_BUFFER_ULONG(
    PULONG  Register,
    PULONG  Buffer,
    ULONG   Count
    );


NTHALAPI
VOID
WRITE_REGISTER_UCHAR(
    PUCHAR  Register,
    UCHAR   Value
    );

NTHALAPI
VOID
WRITE_REGISTER_USHORT(
    PUSHORT Register,
    USHORT  Value
    );

NTHALAPI
VOID
WRITE_REGISTER_ULONG(
    PULONG  Register,
    ULONG   Value
    );

NTHALAPI
VOID
WRITE_REGISTER_BUFFER_UCHAR(
    PUCHAR  Register,
    PUCHAR  Buffer,
    ULONG   Count
    );

NTHALAPI
VOID
WRITE_REGISTER_BUFFER_USHORT(
    PUSHORT Register,
    PUSHORT Buffer,
    ULONG   Count
    );

NTHALAPI
VOID
WRITE_REGISTER_BUFFER_ULONG(
    PULONG  Register,
    PULONG  Buffer,
    ULONG   Count
    );

NTHALAPI
UCHAR
READ_PORT_UCHAR(
    PUCHAR  Port
    );

NTHALAPI
USHORT
READ_PORT_USHORT(
    PUSHORT Port
    );

NTHALAPI
ULONG
READ_PORT_ULONG(
    PULONG  Port
    );

NTHALAPI
VOID
READ_PORT_BUFFER_UCHAR(
    PUCHAR  Port,
    PUCHAR  Buffer,
    ULONG   Count
    );

NTHALAPI
VOID
READ_PORT_BUFFER_USHORT(
    PUSHORT Port,
    PUSHORT Buffer,
    ULONG   Count
    );

NTHALAPI
VOID
READ_PORT_BUFFER_ULONG(
    PULONG  Port,
    PULONG  Buffer,
    ULONG   Count
    );

NTHALAPI
VOID
WRITE_PORT_UCHAR(
    PUCHAR  Port,
    UCHAR   Value
    );

NTHALAPI
VOID
WRITE_PORT_USHORT(
    PUSHORT Port,
    USHORT  Value
    );

NTHALAPI
VOID
WRITE_PORT_ULONG(
    PULONG  Port,
    ULONG   Value
    );

NTHALAPI
VOID
WRITE_PORT_BUFFER_UCHAR(
    PUCHAR  Port,
    PUCHAR  Buffer,
    ULONG   Count
    );

NTHALAPI
VOID
WRITE_PORT_BUFFER_USHORT(
    PUSHORT Port,
    PUSHORT Buffer,
    ULONG   Count
    );

NTHALAPI
VOID
WRITE_PORT_BUFFER_ULONG(
    PULONG  Port,
    PULONG  Buffer,
    ULONG   Count
    );

// end_ntndis
//
// Get data cache fill size.
//

#define KeGetDcacheFillSize() 1L

// end_ntddk end_wdm end_nthal

//
// Fill TB entry.
//

#define KeFillEntryTb(Pte, Virtual, Invalid)    \
    if (Invalid != FALSE) {                     \
        Ke386InvalidateTb (Virtual);            \
    }

#if !defined(MIDL_PASS) && defined(_M_IX86) && !defined(_CROSS_PLATFORM_)

__inline
VOID
Ke386InvalidateTb (
    IN PVOID Virtual
    )
{
    __asm {
        mov eax, Virtual
        invlpg [eax]
    }
}

#endif

//
// Data cache, instruction cache, I/O buffer, and write buffer flush routine
// prototypes.
//

//  386 and 486 have transparent caches, so these are noops.

#define KeSweepDcache(AllProcessors)
#define KeSweepCurrentDcache()

#define KeSweepIcache(AllProcessors)
#define KeSweepCurrentIcache()

#define KeSweepIcacheRange(AllProcessors, BaseAddress, Length)

// begin_ntddk begin_wdm begin_nthal begin_ntndis

#define KeFlushIoBuffers(Mdl, ReadOperation, DmaOperation)

// end_ntddk end_wdm end_ntndis

#define KeYieldProcessor()    __asm { rep nop }

// end_nthal

//
// Define executive macros for acquiring and releasing executive spinlocks.
// These macros can ONLY be used by executive components and NOT by drivers.
// Drivers MUST use the kernel interfaces since they must be MP enabled on
// all systems.
//
// KeRaiseIrql is one instruction longer than KeAcquireSpinLock on x86 UP.
// KeLowerIrql and KeReleaseSpinLock are the same.
//

#if defined(NT_UP) && !DBG && !defined(_NTDDK_) && !defined(_NTIFS_)

#if !defined(_NTDRIVER_)
#define ExAcquireSpinLock(Lock, OldIrql) (*OldIrql) = KeRaiseIrqlToDpcLevel();
#define ExReleaseSpinLock(Lock, OldIrql) KeLowerIrql((OldIrql))
#else
#define ExAcquireSpinLock(Lock, OldIrql) KeAcquireSpinLock((Lock), (OldIrql))
#define ExReleaseSpinLock(Lock, OldIrql) KeReleaseSpinLock((Lock), (OldIrql))
#endif
#define ExAcquireSpinLockAtDpcLevel(Lock)
#define ExReleaseSpinLockFromDpcLevel(Lock)

#else

//  begin_wdm begin_ntddk

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

#if defined(_M_IX86) && !defined(USER_MODE_CODE)

#pragma warning(disable:4164)
#pragma intrinsic(_disable)
#pragma intrinsic(_enable)
#pragma warning(default:4164)

#endif

#if defined(NT_UP) && !DBG && !defined(USER_MODE_CODE)
#define ExAcquireFastLock(Lock, OldIrql) _disable()
#else
#define ExAcquireFastLock(Lock, OldIrql) \
    ExAcquireSpinLock(Lock, OldIrql)
#endif

#if defined(NT_UP) && !DBG && !defined(USER_MODE_CODE)
#define ExReleaseFastLock(Lock, OldIrql) _enable()
#else
#define ExReleaseFastLock(Lock, OldIrql) \
    ExReleaseSpinLock(Lock, OldIrql)
#endif

//
// The following function prototypes must be in this module so that the
// above macros can call them directly.
//
// begin_nthal

VOID
FASTCALL
KiAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock
    );

VOID
FASTCALL
KiReleaseSpinLock (
    IN PKSPIN_LOCK SpinLock
    );

// end_nthal

//
// KeTestSpinLock may be used to spin at low IRQL until the lock is
// available.  The IRQL must then be raised and the lock acquired with
// KeTryToAcquireSpinLock.  If that fails, lower the IRQL and start again.
//

#if defined(NT_UP)

#define KeTestSpinLock(SpinLock) (TRUE)

#else

BOOLEAN
FASTCALL
KeTestSpinLock (
    IN PKSPIN_LOCK SpinLock
    );

#endif

//
// Define query system time macro.
//

#define KiQuerySystemTime(CurrentTime) \
    while (TRUE) {                                                                  \
        (CurrentTime)->HighPart = SharedUserData->SystemTime.High1Time;             \
        (CurrentTime)->LowPart = SharedUserData->SystemTime.LowPart;                \
        if ((CurrentTime)->HighPart == SharedUserData->SystemTime.High2Time) break; \
        _asm { rep nop }                                                            \
    }
//
// Define query tick count macro.
//

#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_)

//  begin_wdm begin_ntddk

#define KeQueryTickCount(CurrentCount ) { \
    volatile PKSYSTEM_TIME _TickCount = *((PKSYSTEM_TIME *)(&KeTickCount)); \
    while (TRUE) {                                                          \
        (CurrentCount)->HighPart = _TickCount->High1Time;                   \
        (CurrentCount)->LowPart = _TickCount->LowPart;                      \
        if ((CurrentCount)->HighPart == _TickCount->High2Time) break;       \
        _asm { rep nop }                                                    \
    }                                                                       \
}

//  end_wdm end_ntddk

#else

// begin_nthal
#define KiQueryTickCount(CurrentCount) \
    while (TRUE) {                                                      \
        (CurrentCount)->HighPart = KeTickCount.High1Time;               \
        (CurrentCount)->LowPart = KeTickCount.LowPart;                  \
        if ((CurrentCount)->HighPart == KeTickCount.High2Time) break;   \
        _asm { rep nop }                                                \
    }

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
    while (TRUE) {                                                                      \
        (CurrentTime)->HighPart = SharedUserData->InterruptTime.High1Time;              \
        (CurrentTime)->LowPart = SharedUserData->InterruptTime.LowPart;                 \
        if ((CurrentTime)->HighPart == SharedUserData->InterruptTime.High2Time) break;  \
        _asm { rep nop }                                                                \
    }


// begin_nthal
//
// 386 hardware structures
//

//
// A Page Table Entry on an Intel 386/486 has the following definition.
//
// **** NOTE A PRIVATE COPY OF THIS EXISTS IN THE MM\I386 DIRECTORY! ****
// ****  ANY CHANGES NEED TO BE MADE TO BOTH HEADER FILES.           ****
//


typedef struct _HARDWARE_PTE_X86 {
    ULONG Valid : 1;
    ULONG Write : 1;
    ULONG Owner : 1;
    ULONG WriteThrough : 1;
    ULONG CacheDisable : 1;
    ULONG Accessed : 1;
    ULONG Dirty : 1;
    ULONG LargePage : 1;
    ULONG Global : 1;
    ULONG CopyOnWrite : 1; // software field
    ULONG Prototype : 1;   // software field
    ULONG reserved : 1;  // software field
    ULONG PageFrameNumber : 20;
} HARDWARE_PTE_X86, *PHARDWARE_PTE_X86;

typedef struct _HARDWARE_PTE_X86PAE {
    union {
        struct {
            ULONGLONG Valid : 1;
            ULONGLONG Write : 1;
            ULONGLONG Owner : 1;
            ULONGLONG WriteThrough : 1;
            ULONGLONG CacheDisable : 1;
            ULONGLONG Accessed : 1;
            ULONGLONG Dirty : 1;
            ULONGLONG LargePage : 1;
            ULONGLONG Global : 1;
            ULONGLONG CopyOnWrite : 1; // software field
            ULONGLONG Prototype : 1;   // software field
            ULONGLONG reserved0 : 1;  // software field
            ULONGLONG PageFrameNumber : 24;
            ULONGLONG reserved1 : 28;  // software field
        };
        struct {
            ULONG LowPart;
            ULONG HighPart;
        };
    };
} HARDWARE_PTE_X86PAE, *PHARDWARE_PTE_X86PAE;

#if !defined (_X86PAE_)
typedef HARDWARE_PTE_X86 HARDWARE_PTE;
typedef PHARDWARE_PTE_X86 PHARDWARE_PTE;
#else
typedef HARDWARE_PTE_X86PAE HARDWARE_PTE;
typedef PHARDWARE_PTE_X86PAE PHARDWARE_PTE;
#endif

//
// GDT Entry
//

typedef struct _KGDTENTRY {
    USHORT  LimitLow;
    USHORT  BaseLow;
    union {
        struct {
            UCHAR   BaseMid;
            UCHAR   Flags1;     // Declare as bytes to avoid alignment
            UCHAR   Flags2;     // Problems.
            UCHAR   BaseHi;
        } Bytes;
        struct {
            ULONG   BaseMid : 8;
            ULONG   Type : 5;
            ULONG   Dpl : 2;
            ULONG   Pres : 1;
            ULONG   LimitHi : 4;
            ULONG   Sys : 1;
            ULONG   Reserved_0 : 1;
            ULONG   Default_Big : 1;
            ULONG   Granularity : 1;
            ULONG   BaseHi : 8;
        } Bits;
    } HighWord;
} KGDTENTRY, *PKGDTENTRY;

#define TYPE_CODE   0x10  // 11010 = Code, Readable, NOT Conforming, Accessed
#define TYPE_DATA   0x12  // 10010 = Data, ReadWrite, NOT Expanddown, Accessed
#define TYPE_TSS    0x01  // 01001 = NonBusy TSS
#define TYPE_LDT    0x02  // 00010 = LDT

#define DPL_USER    3
#define DPL_SYSTEM  0

#define GRAN_BYTE   0
#define GRAN_PAGE   1

#define SELECTOR_TABLE_INDEX 0x04

//
// Entry of Interrupt Descriptor Table (IDTENTRY)
//

typedef struct _KIDTENTRY {
   USHORT Offset;
   USHORT Selector;
   USHORT Access;
   USHORT ExtendedOffset;
} KIDTENTRY;

typedef KIDTENTRY *PKIDTENTRY;


//
// TSS (Task switch segment) NT only uses to control stack switches.
//
//  The only fields we actually care about are Esp0, Ss0, the IoMapBase
//  and the IoAccessMaps themselves.
//
//
//  N.B.    Size of TSS must be <= 0xDFFF
//

//
// The interrupt direction bitmap is used on Pentium to allow
// the processor to emulate V86 mode software interrupts for us.
// There is one for each IOPM.  It is located by subtracting
// 32 from the IOPM base in the Tss.
//
#define INT_DIRECTION_MAP_SIZE   32
typedef UCHAR   KINT_DIRECTION_MAP[INT_DIRECTION_MAP_SIZE];

#define IOPM_COUNT      1           // Number of i/o access maps that
                                    // exist (in addition to
                                    // IO_ACCESS_MAP_NONE)

#define IO_ACCESS_MAP_NONE 0

#define IOPM_SIZE           8192    // Size of map callers can set.

#define PIOPM_SIZE          8196    // Size of structure we must allocate
                                    // to hold it.

typedef UCHAR   KIO_ACCESS_MAP[IOPM_SIZE];

typedef KIO_ACCESS_MAP *PKIO_ACCESS_MAP;

typedef struct _KiIoAccessMap {
    KINT_DIRECTION_MAP DirectionMap;
    UCHAR IoMap[PIOPM_SIZE];
} KIIO_ACCESS_MAP;


typedef struct _KTSS {

    USHORT  Backlink;
    USHORT  Reserved0;

    ULONG   Esp0;
    USHORT  Ss0;
    USHORT  Reserved1;

    ULONG   NotUsed1[4];

    ULONG   CR3;

    ULONG   Eip;

    ULONG   NotUsed2[9];

    USHORT  Es;
    USHORT  Reserved2;

    USHORT  Cs;
    USHORT  Reserved3;

    USHORT  Ss;
    USHORT  Reserved4;

    USHORT  Ds;
    USHORT  Reserved5;

    USHORT  Fs;
    USHORT  Reserved6;

    USHORT  Gs;
    USHORT  Reserved7;

    USHORT  LDT;
    USHORT  Reserved8;

    USHORT  Flags;

    USHORT  IoMapBase;

    KIIO_ACCESS_MAP IoMaps[IOPM_COUNT];

    //
    // This is the Software interrupt direction bitmap associated with
    // IO_ACCESS_MAP_NONE
    //
    KINT_DIRECTION_MAP IntDirectionMap;
} KTSS, *PKTSS;


#define KiComputeIopmOffset(MapNumber)          \
    (MapNumber == IO_ACCESS_MAP_NONE) ?         \
        (USHORT)(sizeof(KTSS)) :                    \
        (USHORT)(FIELD_OFFSET(KTSS, IoMaps[MapNumber-1].IoMap))

// begin_windbgkd

#ifdef _X86_
//
// Special Registers for i386
//

typedef struct _DESCRIPTOR {
    USHORT  Pad;
    USHORT  Limit;
    ULONG   Base;
} KDESCRIPTOR, *PKDESCRIPTOR;

typedef struct _KSPECIAL_REGISTERS {
    ULONG Cr0;
    ULONG Cr2;
    ULONG Cr3;
    ULONG Cr4;
    ULONG KernelDr0;
    ULONG KernelDr1;
    ULONG KernelDr2;
    ULONG KernelDr3;
    ULONG KernelDr6;
    ULONG KernelDr7;
    KDESCRIPTOR Gdtr;
    KDESCRIPTOR Idtr;
    USHORT Tr;
    USHORT Ldtr;
    ULONG Reserved[6];
} KSPECIAL_REGISTERS, *PKSPECIAL_REGISTERS;

//
// Processor State frame: Before a processor freezes itself, it
// dumps the processor state to the processor state frame for
// debugger to examine.
//

typedef struct _KPROCESSOR_STATE {
    struct _CONTEXT ContextFrame;
    struct _KSPECIAL_REGISTERS SpecialRegisters;
} KPROCESSOR_STATE, *PKPROCESSOR_STATE;
#endif // _X86_
// end_windbgkd

//
// Processor Control Block (PRCB)
//

#define PRCB_MAJOR_VERSION 1
#define PRCB_MINOR_VERSION 1
#define PRCB_BUILD_DEBUG        0x0001
#define PRCB_BUILD_UNIPROCESSOR 0x0002

typedef struct _KPRCB {

//
// Start of the architecturally defined section of the PRCB. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//
    USHORT MinorVersion;
    USHORT MajorVersion;

    struct _KTHREAD *CurrentThread;
    struct _KTHREAD *NextThread;
    struct _KTHREAD *IdleThread;

    CCHAR  Number;
    CCHAR  Reserved;
    USHORT BuildType;
    KAFFINITY SetMember;

    CCHAR   CpuType;
    CCHAR   CpuID;
    USHORT  CpuStep;

    struct _KPROCESSOR_STATE ProcessorState;

    ULONG   KernelReserved[16];         // For use by the kernel
    ULONG   HalReserved[16];            // For use by Hal

//
// Per processor lock queue entries.
//

    KSPIN_LOCK_QUEUE LockQueue[16];

// End of the architecturally defined section of the PRCB.
// end_nthal

    struct _KTHREAD *NpxThread;

    ULONG   InterruptCount;             // per processor counts
    ULONG   KernelTime;
    ULONG   UserTime;
    ULONG   DpcTime;
    ULONG   InterruptTime;
    ULONG   ApcBypassCount;
    ULONG   DpcBypassCount;
    ULONG   AdjustDpcThreshold;
    ULONG   DebugDpcTime;               // per dpc tick count
    ULONG   Spare2[4];

    ULONG   ThreadStartCount[2];        // perf data


//  MP tuning..

//
//  Per-processor data for various hot code which resides in the
//  kernel image.  We give each processor it's own copy of the data
//  to lessen the caching impact of sharing the data between multiple
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
//  Kernel performance counters.
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
// Cache pad.
//

    ULONG CachePad0[2];

//
// Nonpaged per processor lookaside lists.
//

    PP_LOOKASIDE_LIST PPLookasideList[16];

//
// Nonpaged per processor small pool lookaside lists.
//

    PP_LOOKASIDE_LIST PPNPagedLookasideList[POOL_SMALL_LISTS];

//
// Paged per processor small pool lookaside lists.
//

    PP_LOOKASIDE_LIST PPPagedLookasideList[POOL_SMALL_LISTS];

//
// Reserved Pad.
//

    UCHAR ReservedPad[16 * 8];

//
// MP interprocessor request packet and summary.
//
// N.B. This is carefully aligned to be on a cache line boundary.
//

    volatile PVOID CurrentPacket[3];
    volatile KAFFINITY TargetSet;
    volatile PKIPI_WORKER WorkerRoutine;
    volatile ULONG IpiFrozen;
    ULONG CachePad1[2];

//
// MP interprocessor request summary and packet address.
//

    volatile ULONG RequestSummary;
    volatile struct _KPRCB *SignalDone;
    volatile ULONG ReverseStall;
    PVOID IpiFrame;
    ULONG CachePad2[4];

//
// DPC interrupt requested.
//

    ULONG DpcInterruptRequested;

//
// Per processor chained interrupt list.
// (This is not here for any performance reason, it seemed reasonable
// to use a cache pad dword for something useful).
//

    PVOID ChainedInterruptList;
    ULONG CachePad3[2];

//
// DPC batching parameters.
//

    ULONG MaximumDpcQueueDepth;
    ULONG MinimumDpcRate;
    ULONG CachePad4[2];

//
// DPC list head, spinlock, and count.
//

    LIST_ENTRY DpcListHead;
    ULONG DpcQueueDepth;
    ULONG DpcRoutineActive;
    ULONG DpcCount;
    ULONG DpcLastCount;
    ULONG DpcRequestRate;
    PVOID DpcStack;

    ULONG KernelReserved2[10];
    KSPIN_LOCK DpcLock;

//
// Debug & processor information
//

    BOOLEAN SkipTick;
    UCHAR VendorString[13];
    ULONG MHz;
    ULONG FeatureBits;
    LARGE_INTEGER UpdateSignature;

//
// QuantumEnd indicator
//

    ULONG QuantumEnd;

//
// Processors power state
//

    PROCESSOR_POWER_STATE PowerState;

//
// Npx save area
//
    FX_SAVE_AREA    NpxSaveArea;

// begin_nthal
} KPRCB, *PKPRCB, *RESTRICTED_POINTER PRKPRCB;

// begin_ntddk

//
// Processor Control Region Structure Definition
//

#define PCR_MINOR_VERSION 1
#define PCR_MAJOR_VERSION 1

typedef struct _KPCR {

//
// Start of the architecturally defined section of the PCR. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//

    NT_TIB  NtTib;
    struct _KPCR *SelfPcr;              // flat address of this PCR
    struct _KPRCB *Prcb;                // pointer to Prcb
    KIRQL   Irql;
    ULONG   IRR;
    ULONG   IrrActive;
    ULONG   IDR;
    ULONG   Reserved2;

    struct _KIDTENTRY *IDT;
    struct _KGDTENTRY *GDT;
    struct _KTSS      *TSS;
    USHORT  MajorVersion;
    USHORT  MinorVersion;
    KAFFINITY SetMember;
    ULONG   StallScaleFactor;
    UCHAR   DebugActive;
    UCHAR   Number;

// end_ntddk

    UCHAR   VdmAlert;
    UCHAR   Reserved[1];                // dword align
    ULONG   KernelReserved[15];         // For use by the kernel
    ULONG   SecondLevelCacheSize;
    ULONG   HalReserved[16];            // For use by Hal

// End of the architecturally defined section of the PCR.
// end_nthal

    ULONG   InterruptMode;
    UCHAR   Spare1;
    ULONG   KernelReserved2[17];
    struct _KPRCB PrcbData;

// begin_nthal begin_ntddk
} KPCR;
typedef KPCR *PKPCR;

// end_nthal end_ntddk


//
// Sanitize segCS and eFlags based on a processor mode.
//
// If kernel mode,
//      force CPL == 0
//
// If user mode,
//      force CPL == 3
//

#define SANITIZE_SEG(segCS, mode) (\
    ((mode) == KernelMode ? \
        ((0x00000000L) | ((segCS) & 0xfffc)) : \
        ((0x00000003L) | ((segCS) & 0xffff))))

//
// If kernel mode, then
//      let caller specify Carry, Parity, AuxCarry, Zero, Sign, Trap,
//      Direction, Overflow, Interrupt, AlignCheck.
//
// If user mode, then
//      let caller specify Carry, Parity, AuxCarry, Zero, Sign, Trap,
//      Direction, Overflow, AlignCheck.
//      force Interrupts on.
//

#define EFLAGS_DF_MASK        0x00000400L
#define EFLAGS_INTERRUPT_MASK 0x00000200L
#define EFLAGS_V86_MASK       0x00020000L
#define EFLAGS_ALIGN_CHECK    0x00040000L
#define EFLAGS_IOPL_MASK      0x00003000L
#define EFLAGS_VIF            0x00080000L
#define EFLAGS_VIP            0x00100000L
#define EFLAGS_USER_SANITIZE  0x003e0dd7L

#define SANITIZE_FLAGS(eFlags, mode) (\
    ((mode) == KernelMode ? \
        ((0x00000000L) | ((eFlags) & 0x003e0fd7)) : \
        ((((eFlags) & EFLAGS_V86_MASK) && KeI386VdmIoplAllowed) ? \
        (((eFlags) & KeI386EFlagsAndMaskV86) | KeI386EFlagsOrMaskV86) : \
        ((EFLAGS_INTERRUPT_MASK) | ((eFlags) & EFLAGS_USER_SANITIZE)))))

//
// Masks for Dr7 and sanitize macros for various Dr registers.
//

#define DR6_LEGAL   0x0000e00f

#define DR7_LEGAL   0xffff0155  // R/W, LEN for Dr0-Dr4,
                                // Local enable for Dr0-Dr4,
                                // Le for "perfect" trapping

#define DR7_ACTIVE  0x00000055  // If any of these bits are set, a Dr is active

#define SANITIZE_DR6(Dr6, mode) ((Dr6 & DR6_LEGAL));

#define SANITIZE_DR7(Dr7, mode) ((Dr7 & DR7_LEGAL));

#define SANITIZE_DRADDR(DrReg, mode) (          \
    (mode) == KernelMode ?                      \
        (DrReg) :                               \
        (((PVOID)DrReg <= MM_HIGHEST_USER_ADDRESS) ?   \
            (DrReg) :                           \
            (0)                                 \
        )                                       \
    )

//
// Define macro to clear reserved bits from MXCSR so that we don't
// GP fault when doing an FRSTOR
//
#define SANITIZE_MXCSR(_mxcsr_) ((_mxcsr_) & 0xFFBF)

//
// Nonvolatile context pointers
//
// BUGBUG bryanwi 21 feb 90 - This is bogus.  The 386 doesn't have
//                            enough nonvolatile context to make this
//                            structure worthwhile.  Can't declare a
//                            field to be void, so declare a Junk structure
//                            instead.

typedef struct _KNONVOLATILE_CONTEXT_POINTERS {
    ULONG   Junk;
} KNONVOLATILE_CONTEXT_POINTERS,  *PKNONVOLATILE_CONTEXT_POINTERS;

// begin_nthal
//
// Trap frame
//
//  NOTE - We deal only with 32bit registers, so the assembler equivalents
//         are always the extended forms.
//
//  NOTE - Unless you want to run like slow molasses everywhere in the
//         the system, this structure must be of DWORD length, DWORD
//         aligned, and its elements must all be DWORD aligned.
//
//  NOTE WELL   -
//
//      The i386 does not build stack frames in a consistent format, the
//      frames vary depending on whether or not a privilege transition
//      was involved.
//
//      In order to make NtContinue work for both user mode and kernel
//      mode callers, we must force a canonical stack.
//
//      If we're called from kernel mode, this structure is 8 bytes longer
//      than the actual frame!
//
//  WARNING:
//
//      KTRAP_FRAME_LENGTH needs to be 16byte integral (at present.)
//

typedef struct _KTRAP_FRAME {


//
//  Following 4 values are only used and defined for DBG systems,
//  but are always allocated to make switching from DBG to non-DBG
//  and back quicker.  They are not DEVL because they have a non-0
//  performance impact.
//

    ULONG   DbgEbp;         // Copy of User EBP set up so KB will work.
    ULONG   DbgEip;         // EIP of caller to system call, again, for KB.
    ULONG   DbgArgMark;     // Marker to show no args here.
    ULONG   DbgArgPointer;  // Pointer to the actual args

//
//  Temporary values used when frames are edited.
//
//
//  NOTE:   Any code that want's ESP must materialize it, since it
//          is not stored in the frame for kernel mode callers.
//
//          And code that sets ESP in a KERNEL mode frame, must put
//          the new value in TempEsp, make sure that TempSegCs holds
//          the real SegCs value, and put a special marker value into SegCs.
//

    ULONG   TempSegCs;
    ULONG   TempEsp;

//
//  Debug registers.
//

    ULONG   Dr0;
    ULONG   Dr1;
    ULONG   Dr2;
    ULONG   Dr3;
    ULONG   Dr6;
    ULONG   Dr7;

//
//  Segment registers
//

    ULONG   SegGs;
    ULONG   SegEs;
    ULONG   SegDs;

//
//  Volatile registers
//

    ULONG   Edx;
    ULONG   Ecx;
    ULONG   Eax;

//
//  Nesting state, not part of context record
//

    ULONG   PreviousPreviousMode;

    PEXCEPTION_REGISTRATION_RECORD ExceptionList;
                                            // Trash if caller was user mode.
                                            // Saved exception list if caller
                                            // was kernel mode or we're in
                                            // an interrupt.

//
//  FS is TIB/PCR pointer, is here to make save sequence easy
//

    ULONG   SegFs;

//
//  Non-volatile registers
//

    ULONG   Edi;
    ULONG   Esi;
    ULONG   Ebx;
    ULONG   Ebp;

//
//  Control registers
//

    ULONG   ErrCode;
    ULONG   Eip;
    ULONG   SegCs;
    ULONG   EFlags;

    ULONG   HardwareEsp;    // WARNING - segSS:esp are only here for stacks
    ULONG   HardwareSegSs;  // that involve a ring transition.

    ULONG   V86Es;          // these will be present for all transitions from
    ULONG   V86Ds;          // V86 mode
    ULONG   V86Fs;
    ULONG   V86Gs;
} KTRAP_FRAME;


typedef KTRAP_FRAME *PKTRAP_FRAME;
typedef KTRAP_FRAME *PKEXCEPTION_FRAME;

#define KTRAP_FRAME_LENGTH  (sizeof(KTRAP_FRAME))
#define KTRAP_FRAME_ALIGN   (sizeof(ULONG))
#define KTRAP_FRAME_ROUND   (KTRAP_FRAME_ALIGN-1)

//
//  Bits forced to 0 in SegCs if Esp has been edited.
//

#define FRAME_EDITED        0xfff8

// end_nthal

//
// The frame saved by KiCallUserMode is defined here to allow
// the kernel debugger to trace the entire kernel stack
// when usermode callouts are pending.
//

typedef struct _KCALLOUT_FRAME {
    ULONG   InStk;          // saved initial stack address
    ULONG   TrFr;           // saved callback trap frame
    ULONG   CbStk;          // saved callback stack address
    ULONG   Edi;            // saved nonvolatile registers
    ULONG   Esi;            //
    ULONG   Ebx;            //
    ULONG   Ebp;            //
    ULONG   Ret;            // saved return address
    ULONG   OutBf;          // address to store output buffer
    ULONG   OutLn;          // address to store output length
} KCALLOUT_FRAME;

typedef KCALLOUT_FRAME *PKCALLOUT_FRAME;


//
// BUGBUG shielint  The second level vectors have not been defined.
//

// #define MAXIMUM_VECTOR 0x20   // Maximum Interrupt Vector


//
//  Switch Frame
//
//  386 doesn't have an "exception frame", and doesn't normally make
//  any use of nonvolatile context register structures.
//
//  However, swapcontext in ctxswap.c and KeInitializeThread in
//  thredini.c need to share common stack structure used at thread
//  startup and switch time.
//
//  This is that structure.
//

typedef struct _KSWITCHFRAME {
    ULONG   ExceptionList;
    ULONG   Eflags;
    ULONG   RetAddr;
} KSWITCHFRAME, *PKSWITCHFRAME;


//
// Various 387 defines
//

#define I386_80387_NP_VECTOR    0x07    // trap 7 when hardware not present

// begin_ntddk begin_wdm
//
// The non-volatile 387 state
//

typedef struct _KFLOATING_SAVE {
    ULONG   ControlWord;
    ULONG   StatusWord;
    ULONG   ErrorOffset;
    ULONG   ErrorSelector;
    ULONG   DataOffset;                 // Not used in wdm
    ULONG   DataSelector;
    ULONG   Cr0NpxState;
    ULONG   Spare1;                     // Not used in wdm
} KFLOATING_SAVE, *PKFLOATING_SAVE;

// end_ntddk end_wdm

//
// i386 Profile values
//

#define DEFAULT_PROFILE_INTERVAL   39063

//
// The minimum acceptable profiling interval is set to 1221 which is the
// fast RTC clock rate we can get.  If this
// value is too small, the system will run very slowly.
//

#define MINIMUM_PROFILE_INTERVAL   1221


// begin_ntddk begin_wdm begin_nthal begin_ntndis
//
// i386 Specific portions of mm component
//

//
// Define the page size for the Intel 386 as 4096 (0x1000).
//

#define PAGE_SIZE 0x1000

//
// Define the number of trailing zeroes in a page aligned virtual address.
// This is used as the shift count when shifting virtual addresses to
// virtual page numbers.
//

#define PAGE_SHIFT 12L

// end_ntndis end_wdm
//
// Define the number of bits to shift to right justify the Page Directory Index
// field of a PTE.
//

#define PDI_SHIFT_X86    22
#define PDI_SHIFT_X86PAE 21

#if !defined (_X86PAE_)
#define PDI_SHIFT PDI_SHIFT_X86
#else
#define PDI_SHIFT PDI_SHIFT_X86PAE
#define PPI_SHIFT 30
#endif

//
// Define the number of bits to shift to right justify the Page Table Index
// field of a PTE.
//

#define PTI_SHIFT 12

//
// Define the highest user address and user probe address.
//

// end_ntddk end_nthal

#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_)

// begin_ntddk begin_nthal

extern PVOID *MmHighestUserAddress;
extern PVOID *MmSystemRangeStart;
extern ULONG *MmUserProbeAddress;

#define MM_HIGHEST_USER_ADDRESS *MmHighestUserAddress
#define MM_SYSTEM_RANGE_START *MmSystemRangeStart
#define MM_USER_PROBE_ADDRESS *MmUserProbeAddress

// end_ntddk end_nthal

#else

extern PVOID MmHighestUserAddress;
extern PVOID MmSystemRangeStart;
extern ULONG MmUserProbeAddress;

#define MM_HIGHEST_USER_ADDRESS MmHighestUserAddress
#define MM_SYSTEM_RANGE_START MmSystemRangeStart
#define MM_USER_PROBE_ADDRESS MmUserProbeAddress

#endif

// begin_ntddk begin_nthal
//
// The lowest user address reserves the low 64k.
//

#define MM_LOWEST_USER_ADDRESS (PVOID)0x10000

//
// The lowest address for system space.
//

#if !defined (_X86PAE_)
#define MM_LOWEST_SYSTEM_ADDRESS (PVOID)0xC0800000
#else
#define MM_LOWEST_SYSTEM_ADDRESS (PVOID)0xC0C00000
#endif

// begin_wdm

#define MmGetProcedureAddress(Address) (Address)
#define MmLockPagableCodeSection(Address) MmLockPagableDataSection(Address)

// end_ntddk end_wdm

//
// Define the number of bits to shift to right justify the Page Directory Index
// field of a PTE.
//

#define PDI_SHIFT_X86    22
#define PDI_SHIFT_X86PAE 21

#if !defined (_X86PAE_)
#define PDI_SHIFT PDI_SHIFT_X86
#else
#define PDI_SHIFT PDI_SHIFT_X86PAE
#define PPI_SHIFT 30
#endif

//
// Define the number of bits to shift to right justify the Page Table Index
// field of a PTE.
//

#define PTI_SHIFT 12

//
// Define page directory and page base addresses.
//

#define PDE_BASE_X86    0xc0300000
#define PDE_BASE_X86PAE 0xc0600000

#if !defined (_X86PAE_)
#define PDE_BASE PDE_BASE_X86
#else
#define PDE_BASE PDE_BASE_X86PAE
#endif
#define PTE_BASE 0xc0000000

// end_nthal

//
// Define virtual base and alternate virtual base of kernel.
//

#define KSEG0_BASE 0x80000000
#define ALTERNATE_BASE (0xe1000000 - 64 * 1024 * 1024)

//
// Define macro to initialize directory table base.
//

#define INITIALIZE_DIRECTORY_TABLE_BASE(dirbase,pfn) \
     *((PULONG)(dirbase)) = ((pfn) << PAGE_SHIFT)


// begin_nthal
//
// Location of primary PCR (used only for UP kernel & hal code)
//

// addressed from 0xffdf0000 - 0xffdfffff are reserved for the system
// (ie, not for use by the hal)

#define KI_BEGIN_KERNEL_RESERVED    0xffdf0000
#define KIP0PCRADDRESS              0xffdff000

// begin_ntddk

#define KI_USER_SHARED_DATA         0xffdf0000
#define SharedUserData  ((KUSER_SHARED_DATA * const) KI_USER_SHARED_DATA)

//
// Result type definition for i386.  (Machine specific enumerate type
// which is return type for portable exinterlockedincrement/decrement
// procedures.)  In general, you should use the enumerated type defined
// in ex.h instead of directly referencing these constants.
//

// Flags loaded into AH by LAHF instruction

#define EFLAG_SIGN      0x8000
#define EFLAG_ZERO      0x4000
#define EFLAG_SELECT    (EFLAG_SIGN | EFLAG_ZERO)

#define RESULT_NEGATIVE ((EFLAG_SIGN & ~EFLAG_ZERO) & EFLAG_SELECT)
#define RESULT_ZERO     ((~EFLAG_SIGN & EFLAG_ZERO) & EFLAG_SELECT)
#define RESULT_POSITIVE ((~EFLAG_SIGN & ~EFLAG_ZERO) & EFLAG_SELECT)

//
// Convert various portable ExInterlock APIs into their architectural
// equivalents.
//

#define ExInterlockedIncrementLong(Addend,Lock) \
        Exfi386InterlockedIncrementLong(Addend)

#define ExInterlockedDecrementLong(Addend,Lock) \
        Exfi386InterlockedDecrementLong(Addend)

#define ExInterlockedExchangeUlong(Target,Value,Lock) \
        Exfi386InterlockedExchangeUlong(Target,Value)

//  begin_wdm

#define ExInterlockedAddUlong           ExfInterlockedAddUlong
#define ExInterlockedInsertHeadList     ExfInterlockedInsertHeadList
#define ExInterlockedInsertTailList     ExfInterlockedInsertTailList
#define ExInterlockedRemoveHeadList     ExfInterlockedRemoveHeadList
#define ExInterlockedPopEntryList       ExfInterlockedPopEntryList
#define ExInterlockedPushEntryList      ExfInterlockedPushEntryList

//  end_wdm

//
// Prototypes for architectural specific versions of Exi386 Api
//

//
// Interlocked result type is portable, but its values are machine specific.
// Constants for value are in i386.h, mips.h, etc.
//

typedef enum _INTERLOCKED_RESULT {
    ResultNegative = RESULT_NEGATIVE,
    ResultZero     = RESULT_ZERO,
    ResultPositive = RESULT_POSITIVE
} INTERLOCKED_RESULT;

NTKERNELAPI
INTERLOCKED_RESULT
FASTCALL
Exfi386InterlockedIncrementLong (
    IN PLONG Addend
    );

NTKERNELAPI
INTERLOCKED_RESULT
FASTCALL
Exfi386InterlockedDecrementLong (
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
FASTCALL
Exfi386InterlockedExchangeUlong (
    IN PULONG Target,
    IN ULONG Value
    );

//
// Intrinsic interlocked functions
//

#if (defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_) || defined(NO_INTERLOCKED_INTRINSICS)) && !defined(_WINBASE_)

//  begin_wdm

NTKERNELAPI
LONG
FASTCALL
InterlockedIncrement(
    IN PLONG Addend
    );

NTKERNELAPI
LONG
FASTCALL
InterlockedDecrement(
    IN PLONG Addend
    );

NTKERNELAPI
LONG
FASTCALL
InterlockedExchange(
    IN OUT PLONG Target,
    IN LONG Value
    );

#define InterlockedExchangePointer(Target, Value) \
   (PVOID)InterlockedExchange((PLONG)(Target), (LONG)(Value))

LONG
FASTCALL
InterlockedExchangeAdd(
    IN OUT PLONG Addend,
    IN LONG Increment
    );

NTKERNELAPI
LONG
FASTCALL
InterlockedCompareExchange(
    IN OUT PLONG Destination,
    IN LONG ExChange,
    IN LONG Comperand
    );

#define InterlockedCompareExchangePointer(Destination, ExChange, Comperand) \
    (PVOID)InterlockedCompareExchange((PLONG)Destination, (LONG)ExChange, (LONG)Comperand)

//  end_wdm

#endif

// end_ntddk end_nthal

//
// UP/MP versions of interlocked intrinsics for use within ntoskrnl.exe.
//
// N.B. FASTCALL does NOT work with inline functions.
//

#if !defined(_NTDDK_) && !defined(_NTIFS_) && !defined(_NTHAL_) && !defined(_WINBASE_)
#if defined(_M_IX86) && !defined(_CROSS_PLATFORM_)

#pragma warning(disable:4035)               // wdm re-enable below

#if !defined(MIDL_PASS) // wdm

#if !defined(NO_INTERLOCKED_INTRINSICS)
#if defined(NT_UP)

__inline
LONG
FASTCALL
InterlockedIncrement(
    IN PLONG Addend
    )
{
    __asm {
        mov     eax, 1
        mov     ecx, Addend
        xadd    [ecx], eax
        inc     eax
    }
}

__inline
LONG
FASTCALL
InterlockedDecrement(
    IN PLONG Addend
    )
{
    __asm {
        mov     eax, -1
        mov     ecx, Addend
        xadd    [ecx], eax
        dec     eax
    }
}

__inline
LONG
FASTCALL
InterlockedExchange(
    IN OUT PLONG Target,
    IN LONG Value
    )
{
    __asm {
        mov     edx, Value
        mov     ecx, Target
        mov     eax, [ecx]
ie:     cmpxchg [ecx], edx
        jnz     short ie
    }
}

#define InterlockedExchangePointer(Target, Value) \
   (PVOID)InterlockedExchange((PLONG)Target, (LONG)Value)

__inline
LONG
FASTCALL
InterlockedExchangeAdd(
    IN OUT PLONG Addend,
    IN LONG Increment
    )
{
    __asm {
        mov     eax, Increment
        mov     ecx, Addend
        xadd    [ecx], eax
    }
}

__inline
LONG
FASTCALL
InterlockedCompareExchange(
    IN OUT PLONG Destination,
    IN LONG Exchange,
    IN LONG Comperand
    )
{
    __asm {
        mov     eax, Comperand
        mov     ecx, Destination
        mov     edx, Exchange
        cmpxchg [ecx], edx
    }
}

#define InterlockedCompareExchangePointer(Destination, ExChange, Comperand) \
    (PVOID)InterlockedCompareExchange((PLONG)Destination, (LONG)ExChange, (LONG)Comperand)

#else

__inline
LONG
FASTCALL
InterlockedIncrement(
    IN PLONG Addend
    )
{
    __asm {
        mov     eax, 1
        mov     ecx, Addend
   lock xadd    [ecx], eax
        inc     eax
    }
}

__inline
LONG
FASTCALL
InterlockedDecrement(
    IN PLONG Addend
    )
{
    __asm {
        mov     eax, -1
        mov     ecx, Addend
   lock xadd    [ecx], eax
        dec     eax
    }
}

__inline
LONG
FASTCALL
InterlockedExchange(
    IN OUT PLONG Target,
    IN LONG Value
    )
{
    __asm {
        mov     eax, Value
        mov     ecx, Target
        xchg    [ecx], eax
    }
}

#define InterlockedExchangePointer(Target, Value) \
   (PVOID)InterlockedExchange((PLONG)Target, (LONG)Value)

// begin_wdm

__inline
LONG
FASTCALL
InterlockedExchangeAdd(
    IN OUT PLONG Addend,
    IN LONG Increment
    )
{
    __asm {
        mov     eax, Increment
        mov     ecx, Addend
   lock xadd    [ecx], eax
    }
}


// end_wdm

__inline
LONG
FASTCALL
InterlockedCompareExchange(
    IN OUT PLONG Destination,
    IN LONG Exchange,
    IN LONG Comperand
    )
{
    __asm {
        mov     eax, Comperand
        mov     ecx, Destination
        mov     edx, Exchange
   lock cmpxchg [ecx], edx
    }
}

#define InterlockedCompareExchangePointer(Destination, ExChange, Comperand) \
    (PVOID)InterlockedCompareExchange((PLONG)Destination, (LONG)ExChange, (LONG)Comperand)

#endif      // wdm
#endif
#endif

#pragma warning(default:4035)   // wdm

#endif
#endif

//
// Structure for Ldt information in x86 processes
//

typedef struct _LDTINFORMATION {
    ULONG Size;
    ULONG AllocatedSize;
    PLDT_ENTRY Ldt;
} LDTINFORMATION, *PLDTINFORMATION;

//
// SetProcessInformation Structure for ProcessSetIoHandlers info class
//

typedef struct _PROCESS_IO_PORT_HANDLER_INFORMATION {
    BOOLEAN Install;            // true if handlers to be installed
    ULONG NumEntries;
    ULONG Context;
    PEMULATOR_ACCESS_ENTRY EmulatorAccessEntries;
} PROCESS_IO_PORT_HANDLER_INFORMATION, *PPROCESS_IO_PORT_HANDLER_INFORMATION;


//
//    Vdm Objects and Io handling structure
//

typedef struct _VDM_IO_HANDLER_FUNCTIONS {
    PDRIVER_IO_PORT_ULONG  UlongIo;
    PDRIVER_IO_PORT_ULONG_STRING UlongStringIo;
    PDRIVER_IO_PORT_USHORT UshortIo[2];
    PDRIVER_IO_PORT_USHORT_STRING UshortStringIo[2];
    PDRIVER_IO_PORT_UCHAR UcharIo[4];
    PDRIVER_IO_PORT_UCHAR_STRING UcharStringIo[4];
} VDM_IO_HANDLER_FUNCTIONS, *PVDM_IO_HANDLER_FUNCTIONS;

typedef struct _VDM_IO_HANDLER {
    struct _VDM_IO_HANDLER *Next;
    ULONG PortNumber;
    VDM_IO_HANDLER_FUNCTIONS IoFunctions[2];
} VDM_IO_HANDLER, *PVDM_IO_HANDLER;



// begin_nthal begin_ntddk begin_wdm

#if !defined(MIDL_PASS) && defined(_M_IX86)

//
// i386 function definitions
//

#pragma warning(disable:4035)               // re-enable below

// end_ntddk end_wdm

#if NT_UP
    #define _PCR   ds:[KIP0PCRADDRESS]
#else
    #define _PCR   fs:[0]                   // ntddk
#endif

//
// Get address of current processor block.
//
// WARNING: This inline macro can only be used by the kernel or hal
//
#define KiPcr() KeGetPcr()
__inline PKPCR KeGetPcr(VOID)
{
#if NT_UP
    __asm {  mov eax, KIP0PCRADDRESS }
#else
    __asm {  mov eax, _PCR KPCR.SelfPcr  }
#endif
}

//
// Get address of current processor block.
//
// WARNING: This inline macro can only be used by the kernel or hal
//
__inline PKPRCB KeGetCurrentPrcb (VOID)
{
    __asm {  mov eax, _PCR KPCR.Prcb     }
}

// begin_ntddk begin_wdm

//
// Get current IRQL.
//
// On x86 this function resides in the HAL
//

NTHALAPI
KIRQL
KeGetCurrentIrql();

// end_wdm
//
// Get the current processor number
//

__inline ULONG KeGetCurrentProcessorNumber(VOID)
{
    __asm {  movzx eax, _PCR KPCR.Number  }
}

// end_nthal end_ntddk
//
// Get address of current kernel thread object.
//
// WARNING: This inline macro can not be used for device drivers or HALs
// they must call the kernel function KeGetCurrentThread.
// WARNING: This inline macro is always MP enabled because filesystems
// utilize it
//
__inline struct _KTHREAD *KeGetCurrentThread (VOID)
{
    __asm {  mov eax, fs:[0] KPCR.PrcbData.CurrentThread }
}

//
// If processor executing DPC?
// WARNING: This inline macro is always MP enabled because filesystems
// utilize it
//
__inline ULONG KeIsExecutingDpc(VOID)
{
    __asm {  mov eax, fs:[0] KPCR.PrcbData.DpcRoutineActive }
}

// begin_nthal begin_ntddk begin_wdm

#endif // !defined(MIDL_PASS) && defined(_M_IX86)

// end_nthal end_ntddk end_wdm

//
// Get previous processor mode.
//
// WARNING: This inline macro can not be used for device drivers or HALs
//
// KPROCESSOR_MODE
// KeGetPreviousMode();
#define KeGetPreviousMode()     (KeGetCurrentThread()->PreviousMode)

// begin_nthal
//
// Macro to set address of a trap/interrupt handler to IDT
//
#define KiSetHandlerAddressToIDT(Vector, HandlerAddress) \
    KeGetPcr()->IDT[Vector].ExtendedOffset = HIGHWORD(HandlerAddress); \
    KeGetPcr()->IDT[Vector].Offset = LOWWORD(HandlerAddress);

//
// Macro to return address of a trap/interrupt handler in IDT
//
#define KiReturnHandlerAddressFromIDT(Vector) \
   MAKEULONG(KiPcr()->IDT[Vector].ExtendedOffset, KiPcr()->IDT[Vector].Offset)

#pragma warning(default:4035)
// end_nthal

//++
//
// BOOLEAN
// KiIsThreadNumericStateSaved(
//     IN PKTHREAD Address
//     )
//
//--
#define KiIsThreadNumericStateSaved(a) \
    (a->NpxState != NPX_STATE_LOADED)

//++
//
// VOID
// KiRundownThread(
//     IN PKTHREAD Address
//     )
//
//--

#if defined(NT_UP)

//
// On UP x86 systems, FP state is lazy saved and loaded.  If this
// thread owns the current FP context, clear the ownership field
// so we will not try to save to this thread after it has been
// terminated.
//

#define KiRundownThread(a)                          \
    if (KeGetCurrentPrcb()->NpxThread == (a))   {   \
        KeGetCurrentPrcb()->NpxThread = NULL;       \
    }

#else

#define KiRundownThread(a)

#endif

//
// functions specific to 386 structure
//

VOID
KiSetIRR (
    IN ULONG SWInterruptMask
    );

VOID
KiInitializeGDT (
    IN OUT PKGDTENTRY Gdt,
    IN USHORT GdtLimit,
    IN PKPCR Pcr,
    IN USHORT PcrLimit,
    IN PKTSS Tss,
    IN USHORT TssLimit,
    IN USHORT TebLimit
    );

VOID
KiInitializeGdtEntry (
    OUT PKGDTENTRY GdtEntry,
    IN ULONG Base,
    IN ULONG Limit,
    IN USHORT Type,
    IN USHORT Dpl,
    IN USHORT Granularity
    );

//
// Procedures to support frame manipulation
//

ULONG
KiEspFromTrapFrame(
    IN PKTRAP_FRAME TrapFrame
    );

VOID
KiEspToTrapFrame(
    IN PKTRAP_FRAME TrapFrame,
    IN ULONG Esp
    );

ULONG
KiSegSsFromTrapFrame(
    IN PKTRAP_FRAME TrapFrame
    );

VOID
KiSegSsToTrapFrame(
    IN PKTRAP_FRAME TrapFrame,
    IN ULONG SegSs
    );

//
// Define prototypes for i386 specific clock and profile interrupt routines.
//

VOID
KiUpdateRunTime (
    VOID
    );

VOID
KiUpdateSystemTime (
    VOID
    );

// begin_ntddk begin_wdm

NTKERNELAPI
NTSTATUS
NTAPI
KeSaveFloatingPointState (
    OUT PKFLOATING_SAVE     FloatSave
    );

NTKERNELAPI
NTSTATUS
NTAPI
KeRestoreFloatingPointState (
    IN PKFLOATING_SAVE      FloatSave
    );

// end_ntddk end_wdm
// begin_nthal

VOID
KeProfileInterrupt (
    IN KIRQL OldIrql,
    IN KTRAP_FRAME TrapFrame
    );

NTKERNELAPI
VOID
KeProfileInterruptWithSource (
    IN struct _KTRAP_FRAME *TrapFrame,
    IN KPROFILE_SOURCE ProfileSource
    );

VOID
KeUpdateRuntime (
    IN KIRQL OldIrql,
    IN KTRAP_FRAME TrapFrame
    );

VOID
KeUpdateSystemTime (
    IN KIRQL OldIrql,
    IN KTRAP_FRAME TrapFrame
    );

// begin_ntddk begin_wdm begin_ntndis

#endif // defined(_X86_)

// end_nthal end_ntddk end_wdm end_ntndis

// begin_nthal begin_ntddk

// Use the following for kernel mode runtime checks of X86 system architecture

#ifdef _X86_

#ifdef IsNEC_98
#undef IsNEC_98
#endif

#ifdef IsNotNEC_98
#undef IsNotNEC_98
#endif

#ifdef SetNEC_98
#undef SetNEC_98
#endif

#ifdef SetNotNEC_98
#undef SetNotNEC_98
#endif

#define IsNEC_98     (SharedUserData->AlternativeArchitecture == NEC98x86)
#define IsNotNEC_98  (SharedUserData->AlternativeArchitecture != NEC98x86)
#define SetNEC_98    SharedUserData->AlternativeArchitecture = NEC98x86
#define SetNotNEC_98 SharedUserData->AlternativeArchitecture = StandardDesign

#endif

// end_nthal end_ntddk

//
// i386 arch. specific kernel functions.
//

VOID
Ke386SetLdtProcess (
    struct _KPROCESS  *Process,
    PLDT_ENTRY  Ldt,
    ULONG       Limit
    );

VOID
Ke386SetDescriptorProcess (
    struct _KPROCESS  *Process,
    ULONG       Offset,
    LDT_ENTRY   LdtEntry
    );

VOID
Ke386GetGdtEntryThread (
    struct _KTHREAD *Thread,
    ULONG Offset,
    PKGDTENTRY Descriptor
    );

BOOLEAN
Ke386SetIoAccessMap (
    ULONG               MapNumber,
    PKIO_ACCESS_MAP     IoAccessMap
    );

BOOLEAN
Ke386QueryIoAccessMap (
    ULONG              MapNumber,
    PKIO_ACCESS_MAP    IoAccessMap
    );

BOOLEAN
Ke386IoSetAccessProcess (
    struct _KPROCESS    *Process,
    ULONG       MapNumber
    );

VOID
Ke386SetIOPL(
    struct _KPROCESS    *Process
    );

NTSTATUS
Ke386CallBios (
    IN ULONG BiosCommand,
    IN OUT PCONTEXT BiosArguments
    );

VOID
KiEditIopmDpc (
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

BOOLEAN
Ki386GetSelectorParameters(
    IN USHORT Selector,
    OUT PULONG Flags,
    OUT PULONG Base,
    OUT PULONG Limit
    );

NTSTATUS
Ke386SetVdmInterruptHandler (
    IN struct _KPROCESS *Process,
    IN ULONG Interrupt,
    IN USHORT Selector,
    IN ULONG  Offset,
    IN BOOLEAN Gate32
    );

//
// i386 ABIOS specific routines.
//

NTSTATUS
KeI386GetLid(
    IN USHORT DeviceId,
    IN USHORT RelativeLid,
    IN BOOLEAN SharedLid,
    IN struct _DRIVER_OBJECT *DeviceObject,
    OUT PUSHORT LogicalId
    );

NTSTATUS
KeI386ReleaseLid(
    IN USHORT LogicalId,
    IN struct _DRIVER_OBJECT *DeviceObject
    );

NTSTATUS
KeI386AbiosCall(
    IN USHORT LogicalId,
    IN struct _DRIVER_OBJECT *DriverObject,
    IN PUCHAR RequestBlock,
    IN USHORT EntryPoint
    );

//
// i386 misc routines
//
NTSTATUS
KeI386AllocateGdtSelectors(
    OUT PUSHORT SelectorArray,
    IN USHORT NumberOfSelectors
    );

VOID
KeI386Call16BitFunction (
    IN OUT PCONTEXT Regs
    );

USHORT
KeI386Call16BitCStyleFunction (
    IN ULONG EntryOffset,
    IN ULONG EntrySelector,
    IN PUCHAR Parameters,
    IN ULONG Size
    );

NTSTATUS
KeI386FlatToGdtSelector(
    IN ULONG SelectorBase,
    IN USHORT Length,
    IN USHORT Selector
    );

NTSTATUS
KeI386ReleaseGdtSelectors(
    OUT PUSHORT SelectorArray,
    IN USHORT NumberOfSelectors
    );

NTSTATUS
KeI386SetGdtSelector (
    ULONG       Selector,
    PKGDTENTRY  GdtValue
    );


VOID
KeOptimizeProcessorControlState (
    VOID
    );

//
// i386 Vdm specific functions
//
BOOLEAN
Ke386VdmInsertQueueApc (
    IN PKAPC             Apc,
    IN struct _KTHREAD  *Thread,
    IN KPROCESSOR_MODE   ApcMode,
    IN PKKERNEL_ROUTINE  KernelRoutine,
    IN PKRUNDOWN_ROUTINE RundownRoutine OPTIONAL,
    IN PKNORMAL_ROUTINE  NormalRoutine  OPTIONAL,
    IN PVOID             NormalContext   OPTIONAL,
    IN KPRIORITY         Increment
    );

VOID
Ke386VdmClearApcObject (
    IN PKAPC Apc
    );

VOID
KeI386VdmInitialize (
    VOID
    );

//
// x86 functions for special instructions
//

VOID
CPUID (
    ULONG   InEax,
    PULONG  OutEax,
    PULONG  OutEbx,
    PULONG  OutEcx,
    PULONG  OutEdx
    );

LONGLONG
RDTSC (
    VOID
    );

ULONGLONG
FASTCALL
RDMSR (
    IN ULONG MsrRegister
    );

VOID
WRMSR (
    IN ULONG MsrRegister,
    IN ULONGLONG MsrValue
    );

//
// i386 Vdm specific data
//
extern ULONG KeI386EFlagsAndMaskV86;
extern ULONG KeI386EFlagsOrMaskV86;
extern BOOLEAN KeI386VdmIoplAllowed;
extern ULONG KeI386VirtualIntExtensions;


extern ULONG KeI386CpuType;
extern ULONG KeI386CpuStep;
extern BOOLEAN KeI386NpxPresent;
extern BOOLEAN KeI386FxsrPresent;


//
// i386 Feature bit definitions
//

#define KF_V86_VIS          0x00000001
#define KF_RDTSC            0x00000002
#define KF_CR4              0x00000004
#define KF_CMOV             0x00000008
#define KF_GLOBAL_PAGE      0x00000010
#define KF_LARGE_PAGE       0x00000020
#define KF_MTRR             0x00000040
#define KF_CMPXCHG8B        0x00000080
#define KF_MMX              0x00000100
#define KF_WORKING_PTE      0x00000200
#define KF_PAT              0x00000400
#define KF_FXSR             0x00000800
#define KF_FAST_SYSCALL     0x00001000
#define KF_XMMI             0x00002000
#define KF_3DNOW            0x00004000
#define KF_AMDK6MTRR        0x00008000

//
// Define macro to test if x86 feature is present.
//

extern ULONG KiBootFeatureBits;

#define Isx86FeaturePresent(_f_) ((KiBootFeatureBits & (_f_)) != 0)

#endif // _i386_

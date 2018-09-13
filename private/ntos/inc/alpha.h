/*++

Copyright (c) 1993  Digital Equipment Corporation

Module Name:

   alpha.h

Abstract:

   The Alpha hardware specific header file.

Author:

   Joe Notarangelo  31-Mar-1992   (based on mips.h by Dave Cutler)

Revision History:

    Jeff McLeman (mcleman) 21-Jul-1992
      Add bus types for ISA and EISA

    Thomas Van Baak (tvb) 9-Jul-1992

        Created proper Alpha Exception and Trap structure definitions.

--*/

#ifndef _ALPHAH_
#define _ALPHAH_


// begin_ntddk begin_wdm begin_nthal begin_ntndis

#if defined(_ALPHA_)
#ifdef __cplusplus
extern "C" {
#endif

//
// Types to use to contain PFNs and their counts.
//

typedef ULONG PFN_COUNT;

typedef LONG_PTR SPFN_NUMBER, *PSPFN_NUMBER;
typedef ULONG_PTR PFN_NUMBER, *PPFN_NUMBER;

//
// Define maximum size of flush multiple TB request.
//

#define FLUSH_MULTIPLE_MAXIMUM 16

//
// Indicate that the Alpha compiler supports the pragma textout construct.
//

#define ALLOC_PRAGMA 1

// end_ntndis
//
// Include the Alpha instruction definitions
//

#include "alphaops.h"

//
// Include reference machine definitions.
//

#include "alpharef.h"

// end_ntddk end_wdm

//
// Define intrinsic PAL calls and their prototypes
//
void __di(void);
void __MB(void);
void __dtbis(void *);
void __ei(void);
void *__rdpcr(void);
void *__rdthread(void);
void __ssir(unsigned long);
unsigned char __swpirql(unsigned char);
void __tbia(void);
void __tbis(void *);
void __tbisasn(void *, unsigned long);

#if defined(_M_ALPHA) || defined(_M_AXP64)
#pragma intrinsic(__di)
#pragma intrinsic(__MB)
#pragma intrinsic(__dtbis)
#pragma intrinsic(__ei)
#pragma intrinsic(__rdpcr)
#pragma intrinsic(__rdthread)
#pragma intrinsic(__ssir)
#pragma intrinsic(__swpirql)
#pragma intrinsic(__tbia)
#pragma intrinsic(__tbis)
#pragma intrinsic(__tbisasn)
#endif

//
// Define Alpha Axp Processor Ids.
//

#if !defined(PROCESSOR_ALPHA_21064)
#define PROCESSOR_ALPHA_21064 (21064)
#endif // !PROCESSOR_ALPHA_21064

#if !defined(PROCESSOR_ALPHA_21164)
#define PROCESSOR_ALPHA_21164 (21164)
#endif // !PROCESSOR_ALPHA_21164

#if !defined(PROCESSOR_ALPHA_21066)
#define PROCESSOR_ALPHA_21066 (21066)
#endif // !PROCESSOR_ALPHA_21066

#if !defined(PROCESSOR_ALPHA_21068)
#define PROCESSOR_ALPHA_21068 (21068)
#endif // !PROCESSOR_ALPHA_21068

#if !defined(PROCESSOR_ALPHA_21164PC)
#define PROCESSOR_ALPHA_21164PC (21165)
#endif // !PROCESSOR_ALPHA_21164PC

#if !defined(PROCESSOR_ALPHA_21264)
#define PROCESSOR_ALPHA_21264 (21264)
#endif // !PROCESSOR_ALPHA_21264

// end_nthal

//
// Define Processor Control Region Structure.
//

typedef
VOID
(*PKTRAP_ROUTINE)(
    VOID
    );

// begin_ntddk begin_nthal
//
// Define macro to generate import names.
//

#define IMPORT_NAME(name) __imp_##name

//
// Define length of interrupt vector table.
//

#define MAXIMUM_VECTOR 256

//
// Define bus error routine type.
//

struct _EXCEPTION_RECORD;
struct _KEXCEPTION_FRAME;
struct _KTRAP_FRAME;

typedef
BOOLEAN
(*PKBUS_ERROR_ROUTINE) (
    IN struct _EXCEPTION_RECORD *ExceptionRecord,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame
    );


#define PCR_MINOR_VERSION 1
#define PCR_MAJOR_VERSION 1

typedef struct _KPCR {

//
// Major and minor version numbers of the PCR.
//

    ULONG MinorVersion;
    ULONG MajorVersion;

//
// Start of the architecturally defined section of the PCR. This section
// may be directly addressed by vendor/platform specific PAL/HAL code and will
// not change from version to version of NT.

//
// PALcode information.
//

    ULONGLONG PalBaseAddress;
    ULONG PalMajorVersion;
    ULONG PalMinorVersion;
    ULONG PalSequenceVersion;
    ULONG PalMajorSpecification;
    ULONG PalMinorSpecification;

//
// Firmware restart information.
//

    ULONGLONG FirmwareRestartAddress;
    PVOID RestartBlock;

//
// Reserved per-processor region for the PAL (3K-8 bytes).
//

    ULONGLONG PalReserved[383];

//
// Alignment fixup count updated by PAL and read by kernel.
//

    ULONGLONG PalAlignmentFixupCount;

//
// Panic Stack Address.
//

    PVOID PanicStack;

//
// Processor parameters.
//

    ULONG ProcessorType;
    ULONG ProcessorRevision;
    ULONG PhysicalAddressBits;
    ULONG MaximumAddressSpaceNumber;
    ULONG PageSize;
    ULONG FirstLevelDcacheSize;
    ULONG FirstLevelDcacheFillSize;
    ULONG FirstLevelIcacheSize;
    ULONG FirstLevelIcacheFillSize;

//
// System Parameters.
//

    ULONG FirmwareRevisionId;
    UCHAR SystemType[8];
    ULONG SystemVariant;
    ULONG SystemRevision;
    UCHAR SystemSerialNumber[16];
    ULONG CycleClockPeriod;
    ULONG SecondLevelCacheSize;
    ULONG SecondLevelCacheFillSize;
    ULONG ThirdLevelCacheSize;
    ULONG ThirdLevelCacheFillSize;
    ULONG FourthLevelCacheSize;
    ULONG FourthLevelCacheFillSize;

//
// Pointer to processor control block.
//

    struct _KPRCB *Prcb;

//
// Processor identification.
//

    CCHAR Number;
    KAFFINITY SetMember;

//
// Reserved per-processor region for the HAL (.5K bytes).
//

    ULONGLONG HalReserved[64];

//
// IRQL mapping tables.
//

    ULONG IrqlTable[8];

#define SFW_IMT_ENTRIES 4
#define HDW_IMT_ENTRIES 128

    struct _IRQLMASK {
        USHORT IrqlTableIndex;   // synchronization irql level
        USHORT IDTIndex;         // vector in IDT
    } IrqlMask[SFW_IMT_ENTRIES + HDW_IMT_ENTRIES];

//
// Interrupt Dispatch Table (IDT).
//

    PKINTERRUPT_ROUTINE InterruptRoutine[MAXIMUM_VECTOR];

//
// Reserved vectors mask, these vectors cannot be attached to via
// standard interrupt objects.
//

    ULONG ReservedVectors;

//
// Complement of processor affinity mask.
//

    KAFFINITY NotMember;

    ULONG InterruptInProgress;
    ULONG DpcRequested;

//
// Pointer to machine check handler
//

    PKBUS_ERROR_ROUTINE MachineCheckError;

//
// DPC Stack.
//

    PVOID DpcStack;

//
// End of the architecturally defined section of the PCR. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.  Some of these values are
// reserved for chip-specific palcode.
// end_ntddk end_nthal
//

//
// Start of the operating system release dependent section of the PCR.
// This section may change from release to release and should not be
// addressed by vendor/platform specific HAL code.

    ULONG Spare1;

//
// Current process id.
//

    ULONG CurrentPid;

//
// Spare field.
//

    ULONG Spare2;

//
// System service dispatch start and end address used by get/set context.
//

    ULONG_PTR SystemServiceDispatchStart;
    ULONG_PTR SystemServiceDispatchEnd;

//
// Pointer to Idle thread.
//

    struct _KTHREAD *IdleThread;


} KPCR, *PKPCR; // ntddk nthal

//
// Define Processor Status Register structure
//

typedef struct _PSR {
    ULONG MODE: 1;
    ULONG INTERRUPT_ENABLE: 1;
    ULONG IRQL: 3;
} PSR, *PPSR;

//
// Define Interrupt Enable Register structure
//

typedef struct _IE {
    ULONG SoftwareInterruptEnables: 2;
    ULONG HardwareInterruptEnables: 6;
} IE, *PIE;

#define HARDWARE_PTE_DIRTY_MASK     0x4


#if defined(_AXP64_)

#define _HARDWARE_PTE_WORKING_SET_BITS  14

typedef struct _HARDWARE_PTE {
    ULONGLONG Valid : 1;
    ULONGLONG Reserved1 : 1;
    ULONGLONG FaultOnWrite : 1;
    ULONGLONG Reserved2 : 1;
    ULONGLONG Global : 1;
    ULONGLONG GranularityHint : 2;
    ULONGLONG Reserved3 : 1;
    ULONGLONG KernelReadAccess : 1;
    ULONGLONG UserReadAccess : 1;
    ULONGLONG Reserved4 : 2;
    ULONGLONG KernelWriteAccess : 1;
    ULONGLONG UserWriteAccess : 1;
    ULONGLONG Reserved5 : 2;
    ULONGLONG Write : 1;
    ULONGLONG CopyOnWrite: 1;
    ULONGLONG SoftwareWsIndex : _HARDWARE_PTE_WORKING_SET_BITS;
    ULONGLONG PageFrameNumber : 32;
} HARDWARE_PTE, *PHARDWARE_PTE;

//
// Define initialize page directory base
//

#define INITIALIZE_DIRECTORY_TABLE_BASE(dirbase, pfn)  \
    *((PULONGLONG)(dirbase)) = 0;                      \
    ((PHARDWARE_PTE)(dirbase))->PageFrameNumber = pfn; \
    ((PHARDWARE_PTE)(dirbase))->Write = 1;             \
    ((PHARDWARE_PTE)(dirbase))->KernelReadAccess = 1;  \
    ((PHARDWARE_PTE)(dirbase))->KernelWriteAccess = 1; \
    ((PHARDWARE_PTE)(dirbase))->Global = 0;            \
    ((PHARDWARE_PTE)(dirbase))->FaultOnWrite = 0;      \
    ((PHARDWARE_PTE)(dirbase))->Valid = 1;

#else

typedef struct _HARDWARE_PTE {
    ULONG Valid: 1;
    ULONG Owner: 1;
    ULONG Dirty: 1;
    ULONG reserved: 1;
    ULONG Global: 1;
    ULONG GranularityHint: 2;
    ULONG Write: 1;
    ULONG CopyOnWrite: 1;
    ULONG PageFrameNumber: 23;
} HARDWARE_PTE, *PHARDWARE_PTE;

//
// Define initialize page directory base
//

#define INITIALIZE_DIRECTORY_TABLE_BASE(dirbase, pfn) \
        ((PHARDWARE_PTE)(dirbase))->PageFrameNumber = pfn; \
        ((PHARDWARE_PTE)(dirbase))->Global = 0; \
        ((PHARDWARE_PTE)(dirbase))->Dirty = 1; \
        ((PHARDWARE_PTE)(dirbase))->Valid = 1;

#endif


// begin_nthal
//
// Define some constants for bus type
//

#define MACHINE_TYPE_ISA 0
#define MACHINE_TYPE_EISA 2

//
//  Define pointer to Processor Control Registers
//

#define PCR ((PKPCR)__rdpcr())

// begin_ntddk

#if defined(_AXP64_)

#define KI_USER_SHARED_DATA 0xffffffffff000000UI64

#else

#define KI_USER_SHARED_DATA 0xff000000UL

#endif

#define SharedUserData ((KUSER_SHARED_DATA * const) KI_USER_SHARED_DATA)

// begin_wdm
//
// length of dispatch code in interrupt template
//
#define DISPATCH_LENGTH 4

//
// Define IRQL levels across the architecture.
//

#define PASSIVE_LEVEL   0
#define LOW_LEVEL       0
#define APC_LEVEL       1
#define DISPATCH_LEVEL  2
#define HIGH_LEVEL      7
#define SYNCH_LEVEL (IPI_LEVEL-1)

// end_ntddk end_wdm end_nthal

#define KiProfileIrql PROFILE_LEVEL     // enable portable code

//
// Define interrupt levels that cannot be connected
//

#define ILLEGAL_LEVEL  ( (1<<0) | (1<<APC_LEVEL) | (1<<DISPATCH_LEVEL) | \
                         (1<<CLOCK_LEVEL) | (1<<IPI_LEVEL) )
//
// Sanitize FPCR and PSR based on processor mode.
//
// ## tvb&jn - need to replace these with proper macros.
//

#define SANITIZE_FPCR(fpcr, mode) (fpcr)

//
// Define SANITIZE_PSR for Alpha.
//
// If kernel mode, then caller specifies  psr
//
// If user mode, then
//      force mode bit to user (1)
//      force interrupt enable bit to true (1)
//      force irql to 0
//
// In both cases insure that extraneous bits are not set
//

#define SANITIZE_PSR(psr, mode) \
    ( ((mode) == KernelMode) ?  \
        (psr & 0x3f) :          \
        (0x3) )

// begin_nthal
//
// Exception frame
//
//  This frame is established when handling an exception. It provides a place
//  to save all nonvolatile registers. The volatile registers will already
//  have been saved in a trap frame.
//
//  The layout of the record conforms to a standard call frame since it is
//  used as such. Thus it contains a place to save a return address and is
//  padded so that it is EXACTLY a multiple of 32 bytes in length.
//
//
//  N.B - the 32-byte alignment is more stringent than required by the
//  calling standard (which requires 16-byte alignment), the 32-byte alignment
//  is established for performance reasons in the interaction with the PAL.
//

typedef struct _KEXCEPTION_FRAME {

    ULONGLONG IntRa;    // return address register, ra

    ULONGLONG FltF2;    // nonvolatile floating registers, f2 - f9
    ULONGLONG FltF3;
    ULONGLONG FltF4;
    ULONGLONG FltF5;
    ULONGLONG FltF6;
    ULONGLONG FltF7;
    ULONGLONG FltF8;
    ULONGLONG FltF9;

    ULONGLONG IntS0;    //  nonvolatile integer registers, s0 - s5
    ULONGLONG IntS1;
    ULONGLONG IntS2;
    ULONGLONG IntS3;
    ULONGLONG IntS4;
    ULONGLONG IntS5;
    ULONGLONG IntFp;    // frame pointer register, fp/s6

    ULONGLONG SwapReturn;
    ULONG Psr;          // processor status
    ULONG Fill[5];      // padding for 32-byte stack frame alignment
                        // N.B. - Ulongs from the filler section are used
                        //        in ctxsw.s - do not delete

} KEXCEPTION_FRAME, *PKEXCEPTION_FRAME;

//
// Trap Frame
//
//  This frame is established when handling a trap. It provides a place to
//  save all volatile registers. The nonvolatile registers are saved in an
//  exception frame or through the normal C calling conventions for saved
//  registers.
//
//  The layout of the record conforms to a standard call frame since it is
//  used as such. Thus it contains a place to save a return address and is
//  padded so that it is EXACTLY a multiple of 32 bytes in length.
//
//
//  N.B - the 32-byte alignment is more stringent than required by the
//  calling standard (which requires 16-byte alignment), the 32-byte alignment
//  is established for performance reasons in the interaction with the PAL.
//

typedef struct _KTRAP_FRAME {

    //
    // Fields saved in the PALcode.
    //

    ULONGLONG IntSp;    // $30: stack pointer register, sp
    ULONGLONG Fir;      // (fault instruction) continuation address
    ULONG Psr;          // processor status
    ULONG Fill1[1];     // unused
    ULONGLONG IntFp;    // $15: frame pointer register, fp/s6

    ULONGLONG IntA0;    // $16: argument registers, a0 - a3
    ULONGLONG IntA1;    // $17:
    ULONGLONG IntA2;    // $18:
    ULONGLONG IntA3;    // $19:

    ULONGLONG IntRa;    // $26: return address register, ra
    ULONGLONG IntGp;    // $29: global pointer register, gp
    UCHAR ExceptionRecord[(sizeof(EXCEPTION_RECORD) + 15) & (~15)];

    //
    // Volatile integer registers, s0 - s5 are nonvolatile.
    //

    ULONGLONG IntV0;    //  $0: return value register, v0
    ULONGLONG IntT0;    //  $1: temporary registers, t0 - t7
    ULONGLONG IntT1;    //  $2:
    ULONGLONG IntT2;    //  $3:
    ULONGLONG IntT3;    //  $4:
    ULONGLONG IntT4;    //  $5:
    ULONGLONG IntT5;    //  $6:
    ULONGLONG IntT6;    //  $7:
    ULONGLONG IntT7;    //  $8:

    ULONGLONG IntT8;    // $22: temporary registers, t8 - t11
    ULONGLONG IntT9;    // $23:
    ULONGLONG IntT10;   // $24:
    ULONGLONG IntT11;   // $25:

    ULONGLONG IntT12;   // $27: temporary register, t12
    ULONGLONG IntAt;    // $28: assembler temporary register, at

    ULONGLONG IntA4;    // $20: remaining argument registers a4 - a5
    ULONGLONG IntA5;    // $21:

    //
    // Volatile floating point registers, f2 - f9 are nonvolatile.
    //

    ULONGLONG FltF0;    // $f0:
    ULONGLONG Fpcr;     // floating point control register
    ULONGLONG FltF1;    // $f1:

    ULONGLONG FltF10;   // $f10: temporary registers, $f10 - $f30
    ULONGLONG FltF11;   // $f11:
    ULONGLONG FltF12;   // $f12:
    ULONGLONG FltF13;   // $f13:
    ULONGLONG FltF14;   // $f14:
    ULONGLONG FltF15;   // $f15:
    ULONGLONG FltF16;   // $f16:
    ULONGLONG FltF17;   // $f17:
    ULONGLONG FltF18;   // $f18:
    ULONGLONG FltF19;   // $f19:
    ULONGLONG FltF20;   // $f20:
    ULONGLONG FltF21;   // $f21:
    ULONGLONG FltF22;   // $f22:
    ULONGLONG FltF23;   // $f23:
    ULONGLONG FltF24;   // $f24:
    ULONGLONG FltF25;   // $f25:
    ULONGLONG FltF26;   // $f26:
    ULONGLONG FltF27;   // $f27:
    ULONGLONG FltF28;   // $f28:
    ULONGLONG FltF29;   // $f29:
    ULONGLONG FltF30;   // $f30:

    ULONG OldIrql;      // Previous Irql.
    ULONG PreviousMode; // Previous Mode.
    ULONG_PTR TrapFrame; //
    ULONG Fill2[3];     // padding for 32-byte stack frame alignment

} KTRAP_FRAME, *PKTRAP_FRAME;

#define KTRAP_FRAME_LENGTH ((sizeof(KTRAP_FRAME) + 15) & ~15)
#define KTRAP_FRAME_ALIGN (16)
#define KTRAP_FRAME_ROUND (KTRAP_FRAME_ALIGN - 1)

//
// Firmware Frame
//
//  The firmware frame is similar to the trap frame, but is built by the PAL
//  code that is active when the OS Loader is running. It does not contain an
//  exception record or NT style exception information.
//
//  Type field defintions and parameters.
//

#define FW_EXC_MCHK 0xdec0              // p1=icPerrStat, p2=dcPerrStat
#define FW_EXC_ARITH 0xdec1             // p1=excSum, p2=excMask
#define FW_EXC_INTERRUPT 0xdec2         // p1=isr, p2=ipl, p3=intid
#define FW_EXC_DFAULT 0xdec3            // p1=sp
#define FW_EXC_ITBMISS 0xdec4           // none
#define FW_EXC_ITBACV 0xdec5            // none
#define FW_EXC_NDTBMISS 0xdec6          // p1=sp
#define FW_EXC_PDTBMISS 0xdec7          // p1=sp
#define FW_EXC_UNALIGNED 0xdec8         // p1=sp
#define FW_EXC_OPCDEC 0xdec9            // p1=sp
#define FW_EXC_FEN 0xdeca               // p1=icsr
#define FW_EXC_HALT 0xdecb              // not used
#define FW_EXC_BPT 0xdecc               // p1=0 - user, p1=1 - kernel, p1=type - call kdbg
#define FW_EXC_GENTRAP 0xdecd           // p1=gentrap code
#define FW_EXC_HALT_INTERRUPT 0xdece    // p1=isr, p2=ipl, p3=intid

typedef struct _FIRMWARE_FRAME {
    ULONGLONG Type;
    ULONGLONG Param1;
    ULONGLONG Param2;
    ULONGLONG Param3;
    ULONGLONG Param4;
    ULONGLONG Param5;
    ULONGLONG Psr;
    ULONGLONG Mmcsr;
    ULONGLONG Va;
    ULONGLONG Fir;
    ULONGLONG IntV0;
    ULONGLONG IntT0;
    ULONGLONG IntT1;
    ULONGLONG IntT2;
    ULONGLONG IntT3;
    ULONGLONG IntT4;
    ULONGLONG IntT5;
    ULONGLONG IntT6;
    ULONGLONG IntT7;
    ULONGLONG IntS0;
    ULONGLONG IntS1;
    ULONGLONG IntS2;
    ULONGLONG IntS3;
    ULONGLONG IntS4;
    ULONGLONG IntS5;
    ULONGLONG IntFp;
    ULONGLONG IntA0;
    ULONGLONG IntA1;
    ULONGLONG IntA2;
    ULONGLONG IntA3;
    ULONGLONG IntA4;
    ULONGLONG IntA5;
    ULONGLONG IntT8;
    ULONGLONG IntT9;
    ULONGLONG IntT10;
    ULONGLONG IntT11;
    ULONGLONG IntRa;
    ULONGLONG IntT12;
    ULONGLONG IntAt;
    ULONGLONG IntGp;
    ULONGLONG IntSp;
    ULONGLONG IntZero;
    ULONGLONG FltF0;
    ULONGLONG FltF1;
    ULONGLONG FltF2;
    ULONGLONG FltF3;
    ULONGLONG FltF4;
    ULONGLONG FltF5;
    ULONGLONG FltF6;
    ULONGLONG FltF7;
    ULONGLONG FltF8;
    ULONGLONG FltF9;
    ULONGLONG FltF10;
    ULONGLONG FltF11;
    ULONGLONG FltF12;
    ULONGLONG FltF13;
    ULONGLONG FltF14;
    ULONGLONG FltF15;
    ULONGLONG FltF16;
    ULONGLONG FltF17;
    ULONGLONG FltF18;
    ULONGLONG FltF19;
    ULONGLONG FltF20;
    ULONGLONG FltF21;
    ULONGLONG FltF22;
    ULONGLONG FltF23;
    ULONGLONG FltF24;
    ULONGLONG FltF25;
    ULONGLONG FltF26;
    ULONGLONG FltF27;
    ULONGLONG FltF28;
    ULONGLONG FltF29;
    ULONGLONG FltF30;
    ULONGLONG FltF31;
} FIRMWARE_FRAME, *PFIRMWARE_FRAME;

#define FIRMWARE_FRAME_LENGTH sizeof(FIRMWARE_FRAME)

//
// The frame saved by KiCallUserMode is defined here to allow
// the kernel debugger to trace the entire kernel stack
// when usermode callouts are pending.
//

typedef struct _KCALLOUT_FRAME {
    ULONGLONG   F2;   // saved floating registers f2 - f9
    ULONGLONG   F3;
    ULONGLONG   F4;
    ULONGLONG   F5;
    ULONGLONG   F6;
    ULONGLONG   F7;
    ULONGLONG   F8;
    ULONGLONG   F9;
    ULONGLONG   S0;   // saved integer registers s0 - s5
    ULONGLONG   S1;
    ULONGLONG   S2;
    ULONGLONG   S3;
    ULONGLONG   S4;
    ULONGLONG   S5;
    ULONGLONG   FP;
    ULONGLONG   CbStk;  // saved callback stack address
    ULONGLONG   InStk;  // saved initial stack address
    ULONGLONG   TrFr;   // saved callback trap frame address
    ULONGLONG   TrFir;
    ULONGLONG   Ra;     // saved return address
    ULONGLONG   A0;     // saved argument registers a0-a2
    ULONGLONG   A1;
} KCALLOUT_FRAME, *PKCALLOUT_FRAME;

typedef struct _UCALLOUT_FRAME {
    PVOID Buffer;
    ULONG Length;
    ULONG ApiNumber;
    ULONG Pad;
    ULONGLONG Sp;
    ULONGLONG Ra;
} UCALLOUT_FRAME, *PUCALLOUT_FRAME;

//
// Define Machine Check Status code that is passed in the exception
// record for a machine check exception.
//

typedef struct _MCHK_STATUS {
    ULONG Correctable: 1;
    ULONG Retryable: 1;
} MCHK_STATUS, *PMCHK_STATUS;

//
// Define the MCES register (Machine Check Error Summary).
//

typedef struct _MCES {
    ULONG MachineCheck: 1;
    ULONG SystemCorrectable: 1;
    ULONG ProcessorCorrectable: 1;
    ULONG DisableProcessorCorrectable: 1;
    ULONG DisableSystemCorrectable: 1;
    ULONG DisableMachineChecks: 1;
} MCES, *PMCES;

// end_nthal

// begin_ntddk begin_wdm
//
// Non-volatile floating point state
//

typedef struct _KFLOATING_SAVE {
    ULONGLONG   Fpcr;
    ULONGLONG   SoftFpcr;
    ULONG       Reserved1;              // These reserved words are here to make it
    ULONG       Reserved2;              // the same size as i386/WDM.
    ULONG       Reserved3;
    ULONG       Reserved4;
} KFLOATING_SAVE, *PKFLOATING_SAVE;

// end_ntddk end_wdm
//
// Define Alpha status code aliases. These are internal to PALcode and
// kernel trap handling.
//

#define STATUS_ALPHA_FLOATING_NOT_IMPLEMENTED    STATUS_ILLEGAL_FLOAT_CONTEXT
#define STATUS_ALPHA_ARITHMETIC_EXCEPTION    STATUS_FLOAT_STACK_CHECK
#define STATUS_ALPHA_GENTRAP    STATUS_INSTRUCTION_MISALIGNMENT

//
// Define status code for bad virtual address.  This status differs from
// those above in that it will be forwarded to the offending code.  In lieu
// of defining a new status code, we wlll alias this to an access violation.
// Code can distinguish this error from an access violation by checking
// the number of parameters: a standard access violation has 2 parameters,
// while a non-canonical virtual address access violation will have 3
// parameters (the third parameter is the upper 32-bits of the non-canonical
// virtual address.
//

#define STATUS_ALPHA_BAD_VIRTUAL_ADDRESS    STATUS_ACCESS_VIOLATION

// begin_nthal
//
// Define the halt reason codes.
//

#define AXP_HALT_REASON_HALT 0
#define AXP_HALT_REASON_REBOOT 1
#define AXP_HALT_REASON_RESTART 2
#define AXP_HALT_REASON_POWERFAIL 3
#define AXP_HALT_REASON_POWEROFF 4
#define AXP_HALT_REASON_PALMCHK 6
#define AXP_HALT_REASON_DBLMCHK 7

//
// Processor State frame: Before a processor freezes itself, it
// dumps the processor state to the processor state frame for
// debugger to examine.  This is used by KeFreezeExecution and
// KeUnfreezeExecution routines.
// (from mips.h)BUGBUG shielint Need to fill in the actual structure.
//

typedef struct _KPROCESSOR_STATE {
    struct _CONTEXT ContextFrame;
} KPROCESSOR_STATE, *PKPROCESSOR_STATE;

// begin_ntddk
//
// Processor Control Block (PRCB)
//

#define PRCB_MINOR_VERSION 1
#define PRCB_MAJOR_VERSION 2
#define PRCB_BUILD_DEBUG        0x0001
#define PRCB_BUILD_UNIPROCESSOR 0x0002

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
    struct _KTHREAD *NextThread;
    struct _KTHREAD *IdleThread;
    CCHAR Number;
    CCHAR Reserved;
    USHORT BuildType;
    KAFFINITY SetMember;
    struct _RESTART_BLOCK *RestartBlock;

//
// End of the architecturally defined section of the PRCB. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//
// end_ntddk end_nthal

    ULONG InterruptCount;
    ULONG DpcTime;
    ULONG InterruptTime;
    ULONG KernelTime;
    ULONG UserTime;
    KDPC QuantumEndDpc;

//
// Address of PCR.
//

    PKPCR Pcr;

//
// MP Information.
//

    PVOID Spare2;
    PVOID Spare3;
    volatile ULONG IpiFrozen;
    struct _KPROCESSOR_STATE ProcessorState;
    ULONG LastDpcCount;
    ULONG DpcBypassCount;
    ULONG SoftwareInterrupts;
    PKTRAP_FRAME InterruptTrapFrame;
    ULONG ApcBypassCount;
    ULONG DispatchInterruptCount;
    ULONG DebugDpcTime;
    PVOID Spares[6];

//
// Spares.
//

    PVOID MoreSpares[3];
    PKIPI_COUNTS IpiCounts;

//
// Per-processor data for various hot code which resides in the
// kernel image.  We give each processor it's own copy of the data
// to lessen the caching impact of sharing the data between multiple
// processors.
//

//
//  Spares (formerly fsrtl filelock free lists)
//

    PVOID SpareHotData[2];

//
// Cache manager performance counters.
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
    ULONG KeByteWordEmulationCount;

//
//  Reserved for future counters.
//

    ULONG ReservedCounter[1];

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
//  Spares (formerly fsrtl filelock free lists)
//

    PVOID MoreSpareHotData[2];

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
// Per processor lock queue entries.
//

    KSPIN_LOCK_QUEUE LockQueue[16];

//
// Reserved Pad.
//

#if !defined(_AXP64_)

    UCHAR ReservedPad[16 * 8];

#endif

//
// MP interprocessor request packet and summary.
//
// N.B. This is carefully aligned to be on a cache line boundary.
//

    volatile PVOID CurrentPacket[3];
    volatile KAFFINITY TargetSet;
    volatile PKIPI_WORKER WorkerRoutine;
    ULONG CachePad1[11];

//
// N.B. These two longwords must be on a quadword boundary and adjacent.
//

    volatile ULONGLONG RequestSummary;

//
// Spare counters.
//

    ULONG Spare4[14];
    ULONG DpcInterruptRequested;
    ULONG Spare5[17];
    ULONG CachePad2[2];
    ULONG MaximumDpcQueueDepth;
    ULONG MinimumDpcRate;
    ULONG AdjustDpcThreshold;
    ULONG DpcRequestRate;
    LARGE_INTEGER StartCount;
//
// DPC list head, spinlock, and count.
//

    LIST_ENTRY DpcListHead;
    KSPIN_LOCK DpcLock;
    ULONG DpcCount;
    ULONG QuantumEnd;
    ULONG DpcRoutineActive;
    ULONG DpcQueueDepth;

    BOOLEAN SkipTick;

//
// Processor's power state
//
    PROCESSOR_POWER_STATE PowerState;

} KPRCB, *PKPRCB, *RESTRICTED_POINTER PRKPRCB;      // ntddk nthal

// begin_ntddk begin_wdm begin_nthal begin_ntndis
//
// I/O space read and write macros.
//
//  These have to be actual functions on Alpha, because we need
//  to shift the VA and OR in the BYTE ENABLES.
//
//  These can become INLINEs if we require that ALL Alpha systems shift
//  the same number of bits and have the SAME byte enables.
//
//  The READ/WRITE_REGISTER_* calls manipulate I/O registers in MEMORY space?
//
//  The READ/WRITE_PORT_* calls manipulate I/O registers in PORT space?
//

NTHALAPI
UCHAR
READ_REGISTER_UCHAR(
    PUCHAR Register
    );

NTHALAPI
USHORT
READ_REGISTER_USHORT(
    PUSHORT Register
    );

NTHALAPI
ULONG
READ_REGISTER_ULONG(
    PULONG Register
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
    PUCHAR Register,
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
    PULONG Register,
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
    PUCHAR Port
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

// end_ntndis end_wdm
//
// Define Interlocked operation result values.
//

#define RESULT_ZERO 0
#define RESULT_NEGATIVE 1
#define RESULT_POSITIVE 2

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
// Convert portable interlock interfaces to architecture specific interfaces.
//

#define ExInterlockedIncrementLong(Addend, Lock) \
    ExAlphaInterlockedIncrementLong(Addend)

#define ExInterlockedDecrementLong(Addend, Lock) \
    ExAlphaInterlockedDecrementLong(Addend)

#define ExInterlockedExchangeAddLargeInteger(Target, Value, Lock) \
    ExpInterlockedExchangeAddLargeInteger(Target, Value)

#define ExInterlockedExchangeUlong(Target, Value, Lock) \
    ExAlphaInterlockedExchangeUlong(Target, Value)

NTKERNELAPI
INTERLOCKED_RESULT
ExAlphaInterlockedIncrementLong (
    IN PLONG Addend
    );

NTKERNELAPI
INTERLOCKED_RESULT
ExAlphaInterlockedDecrementLong (
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
ExAlphaInterlockedExchangeUlong (
    IN PULONG Target,
    IN ULONG Value
    );

//  begin_wdm

#if defined(_M_ALPHA) && !defined(RC_INVOKED)

#define InterlockedIncrement _InterlockedIncrement
#define InterlockedDecrement _InterlockedDecrement
#define InterlockedExchange _InterlockedExchange
#define InterlockedExchangeAdd _InterlockedExchangeAdd

LONG
InterlockedIncrement (
    IN OUT PLONG Addend
    );

LONG
InterlockedDecrement (
    IN OUT PLONG Addend
    );

LONG
InterlockedExchange (
    IN OUT PLONG Target,
    LONG Value
    );

#if defined(_M_AXP64)

#define InterlockedCompareExchange _InterlockedCompareExchange
#define InterlockedCompareExchange64 _InterlockedCompareExchange64
#define InterlockedExchangePointer _InterlockedExchangePointer
#define InterlockedCompareExchangePointer _InterlockedCompareExchangePointer
#define InterlockedExchange64 _InterlockedExchange64

LONG
InterlockedCompareExchange (
    IN OUT PLONG Destination,
    IN LONG ExChange,
    IN LONG Comperand
    );

LONGLONG
InterlockedCompareExchange64 (
    IN OUT PLONGLONG Destination,
    IN LONGLONG ExChange,
    IN LONGLONG Comperand
    );

PVOID
InterlockedExchangePointer (
    IN OUT PVOID *Target,
    IN PVOID Value
    );

PVOID
InterlockedCompareExchangePointer (
    IN OUT PVOID *Destination,
    IN PVOID ExChange,
    IN PVOID Comperand
    );

LONGLONG
InterlockedExchange64(
    IN OUT PLONGLONG Target,
    IN LONGLONG Value
    );

#pragma intrinsic(_InterlockedCompareExchange64)
#pragma intrinsic(_InterlockedExchangePointer)
#pragma intrinsic(_InterlockedCompareExchangePointer)
#pragma intrinsic(_InterlockedExchange64)

#else

#define InterlockedExchangePointer(Target, Value) \
    (PVOID)InterlockedExchange((PLONG)(Target), (LONG)(Value))

#define InterlockedCompareExchange(Destination, ExChange, Comperand) \
    (LONG)_InterlockedCompareExchange((PVOID *)(Destination), (PVOID)(ExChange), (PVOID)(Comperand))

#define InterlockedCompareExchangePointer(Destination, ExChange, Comperand) \
    _InterlockedCompareExchange(Destination, ExChange, Comperand)

PVOID
_InterlockedCompareExchange (
    IN OUT PVOID *Destination,
    IN PVOID ExChange,
    IN PVOID Comperand
    );

NTKERNELAPI
LONGLONG
ExpInterlockedCompareExchange64 (
    IN OUT PLONGLONG Destination,
    IN PLONGLONG Exchange,
    IN PLONGLONG Comperand
    );

#endif

LONG
InterlockedExchangeAdd(
    IN OUT PLONG Addend,
    IN LONG Value
    );

#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement)
#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedCompareExchange)

#endif

// there is a lot of other stuff that could go in here
//   probe macros
//   others
// end_ntddk end_wdm end_nthal
//
// Intrinsic interlocked functions.
//


// begin_ntddk begin_wdm begin_nthal begin_ntndis

//
// Define the page size for the Alpha ev4 and lca as 8k.
//

#define PAGE_SIZE 0x2000

//
// Define the number of trailing zeroes in a page aligned virtual address.
// This is used as the shift count when shifting virtual addresses to
// virtual page numbers.
//

#define PAGE_SHIFT 13L

// end_ntddk end_wdm end_nthal end_ntndis

//
// Define the number of bits to shift to right justify the Page Directory Index
// field of a PTE.
//

#if defined(_AXP64_)

#define PDI_SHIFT 23
#define PDI1_SHIFT 33
#define PDI2_SHIFT 23
#define PDI_MASK 0x3ff

#else

#define PDI_SHIFT 24

#endif

//
// Define the number of bits to shift to right justify the Page Table Index
// field of a PTE.
//

#define PTI_SHIFT 13

//
// Define the maximum address space number allowable for the architecture.
//

#define ALPHA_AXP_MAXIMUM_ASN 0xffffffff

// begin_ntddk begin_nthal

//
// The highest user address reserves 64K bytes for a guard page. This is so
// the probing of addresses from kernel mode only have to check the
// starting address for structures of 64K bytes or less.
//

#if defined(_AXP64_)

#define MM_HIGHEST_USER_ADDRESS (PVOID)0x3FFFFFEFFFF // highest user address
#define MM_USER_PROBE_ADDRESS          0x3FFFFFF0000UI64 // guard page address
#define MM_SYSTEM_RANGE_START   (PVOID)0xFFFFFC0000000000 // start of system space

#else

#define MM_HIGHEST_USER_ADDRESS (PVOID)0x7FFEFFFF // highest user address
#define MM_USER_PROBE_ADDRESS 0x7FFF0000 // starting address of guard page
#define MM_SYSTEM_RANGE_START (PVOID)KSEG0_BASE // start of system space

#endif


//
// The following definitions are required for the debugger data block.
//

extern PVOID MmHighestUserAddress;
extern PVOID MmSystemRangeStart;
extern ULONG_PTR MmUserProbeAddress;

//
// The lowest user address reserves the low 64k.
//

#define MM_LOWEST_USER_ADDRESS  (PVOID)0x00010000

// begin_wdm

#define MmGetProcedureAddress(Address) (Address)
#define MmLockPagableCodeSection(Address) MmLockPagableDataSection(Address)

// end_ntddk end_wdm end_nthal

//
// Define the page table base and the page directory base for
// the TB miss routines and memory management.
//

#if defined(_AXP64_)

#define PDE_TBASE 0xFFFFFE0180600000UI64 // first level PDR address
#define PDE_SELFMAP 0xFFFFFE0180601800UI64 // first level PDR self map address
#define PDE_UBASE 0xFFFFFE0180000000UI64 // user second level PDR address
#define PDE_KBASE 0xFFFFFE01807FE000UI64 // kernel second level PDR address
#define PDE_BASE PDE_KBASE              // kernel second level PDR address
#define PTE_BASE 0xFFFFFE0000000000UI64 // page table address
#define PDE64_BASE 0xFFFFFE0180600000UI64 // first level PDR address
#define PTE64_BASE 0xFFFFFE0000000000UI64 // page table address
#define VA_SHIFT (63 - 42)              // address sign extend shift count

#else

#define PDE_BASE (ULONG)0xC0180000      // first level PDR address
#define PDE_SELFMAP (ULONG)0xC0180300   // first level PDR self map address
#define PTE_BASE (ULONG)0xC0000000      // page table address
#define PDE64_BASE (ULONG)0xC0184000    // first level 64-bit PDR address
#define PTE64_BASE (ULONG)0xC2000000    // 64-bit page table address

#endif

//
// Generate kernel segment physical address.
//

#if defined(_AXP64_)

#define KSEG_ADDRESS(FrameNumber) \
    ((PVOID)(KSEG43_BASE | ((ULONG_PTR)(FrameNumber) << PAGE_SHIFT)))

#else

#define KSEG_ADDRESS(FrameNumber) \
    ((PVOID)(KSEG0_BASE | ((ULONG)(FrameNumber) << PAGE_SHIFT)))

#endif

// begin_ntddk begin_wdm
//
// The lowest address for system space.
//

#if defined(_AXP64_)

#define MM_LOWEST_SYSTEM_ADDRESS (PVOID)0xFFFFFE0200000000

#else

#define MM_LOWEST_SYSTEM_ADDRESS (PVOID)0xC0800000

#endif

// end_ntddk end_wdm

#if defined(_AXP64_)

#define SYSTEM_BASE 0xFFFFFE0200000000  // start of system space (no typecast)

#else

#define SYSTEM_BASE 0xc0800000          // start of system space (no typecast)

#endif

// begin_nthal begin_ntddk begin_wdm

//
// Define prototypes to access PCR values
//

NTKERNELAPI
KIRQL
KeGetCurrentIrql();

// end_nthal end_ntddk end_wdm

#define KeGetCurrentThread() ((struct _KTHREAD *) __rdthread())

// begin_ntddk begin_wdm

NTSTATUS
KeSaveFloatingPointState (
    OUT PKFLOATING_SAVE     FloatSave
    );

NTSTATUS
KeRestoreFloatingPointState (
    IN PKFLOATING_SAVE      FloatSave
    );

// end_ntddk end_wdm
// begin_nthal

#define KeGetPreviousMode() (KeGetCurrentThread()->PreviousMode)

#define KeGetDcacheFillSize() PCR->FirstLevelDcacheFillSize

//
// Test if executing DPC.
//

BOOLEAN
KeIsExecutingDpc (
    VOID
    );

//
// Return interrupt trap frame
//
PKTRAP_FRAME
KeGetInterruptTrapFrame(
    VOID
    );

// begin_ntddk
//
// Get address of current PRCB.
//

#define KeGetCurrentPrcb() (PCR->Prcb)

//
// Get current processor number.
//

#define KeGetCurrentProcessorNumber() KeGetCurrentPrcb()->Number

// end_ntddk

//
// Define interface to get pcr address
//

PKPCR KeGetPcr(VOID);

// end_nthal

//
// Data cache, instruction cache, I/O buffer, and write buffer flush routine
// prototypes.
//

VOID
KeSweepDcache (
    IN BOOLEAN AllProcessors
    );

#define KeSweepCurrentDcache() \
    HalSweepDcache();

VOID
KeSweepIcache (
    IN BOOLEAN AllProcessors
    );

VOID
KeSweepIcacheRange (
    IN BOOLEAN AllProcessors,
    IN PVOID BaseAddress,
    IN ULONG_PTR Length
    );

#define KeSweepCurrentIcache() \
    HalSweepIcache();

VOID
KeFlushIcacheRange (
    IN BOOLEAN AllProcessors,
    IN PVOID BaseAddress,
    IN ULONG_PTR Length
    );

// begin_ntddk begin_wdm begin_ntndis begin_nthal
//
// Cache and write buffer flush functions.
//

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

#define KeYieldProcessor()

NTKERNELAPI
VOID
KeProfileInterrupt (
    VOID
    );

NTKERNELAPI
VOID
KeProfileInterruptWithSource (
    IN KPROFILE_SOURCE ProfileSource
    );

NTKERNELAPI
VOID
KeUpdateRunTime (
    VOID
    );

NTKERNELAPI
VOID
KeUpdateSystemTime (
    IN ULONG TimeIncrement
    );

//
// The following function prototypes are exported for use in MP HALs.
//


#if defined(NT_UP)

#define KiAcquireSpinLock(SpinLock)

#else

VOID
KiAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock
    );

#endif

#if defined(NT_UP)

#define KiReleaseSpinLock(SpinLock)

#else

VOID
KiReleaseSpinLock (
    IN PKSPIN_LOCK SpinLock
    );

#endif

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
KeTestSpinLock (
    IN PKSPIN_LOCK SpinLock
    );

#endif


//
// Fill TB entry.
//

#define KeFillEntryTb(Pte, Virtual, Invalid) \
    if (Invalid != FALSE) { \
        KeFlushSingleTb(Virtual, FALSE, FALSE, Pte, *Pte); \
    }

//
// Define machine-specific external references.
//

extern ULONG KiInterruptTemplate[];

//
// Define machine-dependent function prototypes.
//

VOID
KeFlushDcache (
    IN BOOLEAN AllProcessors,
    IN PVOID BaseAddress OPTIONAL,
    IN ULONG Length
    );

ULONG
KiCopyInformation (
    IN OUT PEXCEPTION_RECORD ExceptionRecord1,
    IN PEXCEPTION_RECORD ExceptionRecord2
    );

BOOLEAN
KiEmulateByteWord(
    IN OUT PEXCEPTION_RECORD ExceptionRecord,
    IN OUT struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN OUT struct _KTRAP_FRAME *TrapFrame
    );

BOOLEAN
KiEmulateFloating (
    IN OUT PEXCEPTION_RECORD ExceptionRecord,
    IN OUT struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN OUT struct _KTRAP_FRAME *TrapFrame,
    IN OUT PSW_FPCR SoftwareFpcr
    );

BOOLEAN
KiEmulateReference (
    IN OUT PEXCEPTION_RECORD ExceptionRecord,
    IN OUT struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN OUT struct _KTRAP_FRAME *TrapFrame,
    IN BOOLEAN QuadwordOnly
    );

BOOLEAN
KiFloatingException (
    IN OUT PEXCEPTION_RECORD ExceptionRecord,
    IN OUT struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN OUT struct _KTRAP_FRAME *TrapFrame,
    IN BOOLEAN ImpreciseTrap,
    OUT PULONG SoftFpcrCopy
    );

ULONGLONG
KiGetRegisterValue (
    IN ULONG Register,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame
    );

VOID
KiSetFloatingStatus (
    IN OUT PEXCEPTION_RECORD ExceptionRecord
    );

VOID
KiSetRegisterValue (
    IN ULONG Register,
    IN ULONGLONG Value,
    OUT struct _KEXCEPTION_FRAME *ExceptionFrame,
    OUT struct _KTRAP_FRAME *TrapFrame
    );

VOID
KiRequestSoftwareInterrupt (
    KIRQL RequestIrql
    );

//
// Define query system time macro.
//

#if _AXP64_
    #define KiQuerySystemTime(CurrentTime)     \
        while (TRUE) {                                                             \
            (CurrentTime)->HighPart = SharedUserData->SystemHigh1Time;             \
            (CurrentTime)->LowPart = SharedUserData->SystemLowTime;                \
            if ((CurrentTime)->HighPart == SharedUserData->SystemHigh2Time) break; \
        }
#else
    #define KiQuerySystemTime(CurrentTime)     \
        *(PULONGLONG)(CurrentTime) = SharedUserData->SystemTime
#endif

//
// Define query tick count macro.
//

#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_)

//  begin_wdm begin_ntddk

#define KeQueryTickCount(CurrentCount ) \
    *(PULONGLONG)(CurrentCount) = **((volatile ULONGLONG **)(&KeTickCount));

//  end_wdm end_ntddk

#else

// begin_nthal
#define KiQueryTickCount(CurrentCount) \
    *(PULONGLONG)(CurrentCount) = KeTickCount;

VOID
KeQueryTickCount (
    OUT PLARGE_INTEGER CurrentCount
    );

// end_nthal
#endif

#define KiQueryLowTickCount() (ULONG)KeTickCount

#define KiQueryInterruptTime(CurrentTime) \
    *(PULONGLONG)(CurrentTime) = SharedUserData->InterruptTime

//
// Define executive macros for acquiring and releasing executive spinlocks.
// These macros can ONLY be used by executive components and NOT by drivers.
// Drivers MUST use the kernel interfaces since they must be MP enabled on
// all systems.
//
// KeRaiseIrql is one instruction shorter than KeAcquireSpinLock on Alpha UP.
// KeLowerIrql is one instruction shorter than KeReleaseSpinLock.
//

#if defined(NT_UP) && !defined(_NTDDK_) && !defined(_NTIFS_)
#define ExAcquireSpinLock(Lock, OldIrql) KeRaiseIrql(DISPATCH_LEVEL, (OldIrql))
#define ExReleaseSpinLock(Lock, OldIrql) KeLowerIrql((OldIrql))
#define ExAcquireSpinLockAtDpcLevel(Lock)
#define ExReleaseSpinLockFromDpcLevel(Lock)
#else

//  begin_wdm begin_ntddk

#define ExAcquireSpinLock(Lock, OldIrql) KeAcquireSpinLock((Lock), (OldIrql))
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

#if defined(_M_ALPHA)

#define _disable() __di()
#define _enable() __ei()

#endif

#if defined(NT_UP) && !DBG
#define ExAcquireFastLock(Lock, OldIrql) \
    ExAcquireSpinLock(Lock, OldIrql)
#else
#define ExAcquireFastLock(Lock, OldIrql) \
    ExAcquireSpinLock(Lock, OldIrql)
#endif

#if defined(NT_UP) && !DBG
#define ExReleaseFastLock(Lock, OldIrql) \
    ExReleaseSpinLock(Lock, OldIrql)
#else
#define ExReleaseFastLock(Lock, OldIrql) \
    ExReleaseSpinLock(Lock, OldIrql)
#endif


//
// Alpha function definitions
//

//++
//
// BOOLEAN
// KiIsThreadNumericStateSaved(
//     IN PKTHREAD Address
//     )
//
//  This call is used on a not running thread to see if it's numeric
//  state has been saved in its context information.  On Alpha the
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
// Alpha Feature bit definitions
//
#define KF_BYTE         0x00000001

//
// Define macro to test if x86 feature is present.
//
// N.B. All x86 features test TRUE on Alpha systems.
//

#define Isx86FeaturePresent(_f_) TRUE

// begin_ntddk begin_wdm begin_nthal begin_ntndis
#ifdef __cplusplus
}   // extern "C"
#endif
#endif // _ALPHA_
// end_ntddk end_wdm end_nthal end_ntndis

#endif // _ALPHAH_

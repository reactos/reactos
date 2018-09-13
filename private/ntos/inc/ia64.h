/*++

Module Name:

    ia64.h

Abstract:

    This module contains the IA64 hardware specific header file.

Author:

    David N. Cutler (davec) 31-Mar-1990

Revision History:

    Bernard Lint 6-Jun-1995: IA64 version based on MIPS version.

--*/

#ifndef _IA64H_
#define _IA64H_

//
// Interruption history
//
// N.B. Currently the history records are saved in the 2nd half of the 8K 
//      PCR page.  Therefore, we can only keep track of up to the latest
//      128 interruption records, each of 32 bytes in size.  Also, the PCR
//      structure cannot be greater than 4K.  In the future, the interruption
//      history records may become part of the KPCR structure.
//

typedef struct _IHISTORY_RECORD {
    ULONGLONG InterruptionType;
    ULONGLONG IIP;
    ULONGLONG IPSR;
    ULONGLONG Extra0;
} IHISTORY_RECORD;

#define MAX_NUMBER_OF_IHISTORY_RECORDS  128

//
// For PSR bit field definitions
//
#include "kxia64.h"


// begin_ntddk begin_wdm begin_nthal begin_ntndis

#if defined(_IA64_)

//
// Types to use to contain PFNs and their counts.
//

typedef ULONG PFN_COUNT;

typedef LONG_PTR SPFN_NUMBER, *PSPFN_NUMBER;
typedef ULONG_PTR PFN_NUMBER, *PPFN_NUMBER;    

//
// Define maximum size of flush multiple TB request.
//

#define FLUSH_MULTIPLE_MAXIMUM 100

//
// Indicate that the IA64 compiler supports the pragma textout construct.
//

#define ALLOC_PRAGMA 1

//
// Define intrinsic calls and their prototypes
//

#include "ia64reg.h"

// Please contact INTEL to get IA64-specific information
// @@BEGIN_DDKSPLIT

unsigned __int64 __getReg (int);         // Intel-IA64-Filler
void __setReg (int, unsigned __int64);   // Intel-IA64-Filler
void __isrlz (void);                     // Intel-IA64-Filler
void __dsrlz (void);                     // Intel-IA64-Filler
void __fwb (void);                       // Intel-IA64-Filler
void __mf (void);                        // Intel-IA64-Filler
void __mfa (void);                       // Intel-IA64-Filler
void __synci (void);                     // Intel-IA64-Filler
__int64 __thash (__int64);               // Intel-IA64-Filler
__int64 __ttag (__int64);                // Intel-IA64-Filler
void __ptcl (__int64, __int64);          // Intel-IA64-Filler
void __ptcg (__int64, __int64);          // Intel-IA64-Filler
void __ptcga (__int64, __int64);         // Intel-IA64-Filler
void __ptri (__int64, __int64);          // Intel-IA64-Filler
void __ptrd (__int64, __int64);          // Intel-IA64-Filler
void __invalat (void);                   // Intel-IA64-Filler
void __break (int);                      // Intel-IA64-Filler
void __fc (__int64);                     // Intel-IA64-Filler
void __sum (int);                        // Intel-IA64-Filler
void __rsm (int);                        // Intel-IA64-Filler

#ifdef _M_IA64
#pragma intrinsic (__getReg)             // Intel-IA64-Filler
#pragma intrinsic (__setReg)             // Intel-IA64-Filler
#pragma intrinsic (__isrlz)              // Intel-IA64-Filler
#pragma intrinsic (__dsrlz)              // Intel-IA64-Filler
#pragma intrinsic (__fwb)                // Intel-IA64-Filler
#pragma intrinsic (__mf)                 // Intel-IA64-Filler
#pragma intrinsic (__mfa)                // Intel-IA64-Filler
#pragma intrinsic (__synci)              // Intel-IA64-Filler
#pragma intrinsic (__thash)              // Intel-IA64-Filler
#pragma intrinsic (__ttag)               // Intel-IA64-Filler
#pragma intrinsic (__ptcl)               // Intel-IA64-Filler
#pragma intrinsic (__ptcg)               // Intel-IA64-Filler
#pragma intrinsic (__ptcga)              // Intel-IA64-Filler
#pragma intrinsic (__ptri)               // Intel-IA64-Filler
#pragma intrinsic (__ptrd)               // Intel-IA64-Filler
#pragma intrinsic (__invalat)            // Intel-IA64-Filler
#pragma intrinsic (__break)              // Intel-IA64-Filler
#pragma intrinsic (__fc)                 // Intel-IA64-Filler
#pragma intrinsic (__sum)                // Intel-IA64-Filler
#pragma intrinsic (__rsm)                // Intel-IA64-Filler
#endif // _M_IA64

// @@END_DDKSPLIT

// end_wdm end_ntndis

//
// Define macro to generate import names.
//

#define IMPORT_NAME(name) __imp_##name

// begin_wdm

//
// Define length of interrupt vector table.
//

// Please contact INTEL to get IA64-specific information
#define MAXIMUM_VECTOR 256

// end_wdm


//
// IA64 specific interlocked operation result values.
//

#define RESULT_ZERO 0
#define RESULT_NEGATIVE 1
#define RESULT_POSITIVE 2

//
// Interlocked result type is portable, but its values are machine specific.
// Constants for values are in i386.h, mips.h, etc.
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
    ExIa64InterlockedIncrementLong(Addend)

#define ExInterlockedDecrementLong(Addend, Lock) \
    ExIa64InterlockedDecrementLong(Addend)

#define ExInterlockedExchangeAddLargeInteger(Target, Value, Lock) \
    ExpInterlockedExchangeAddLargeInteger(Target, Value)

#define ExInterlockedExchangeUlong(Target, Value, Lock) \
    ExIa64InterlockedExchangeUlong(Target, Value)

NTKERNELAPI
INTERLOCKED_RESULT
ExIa64InterlockedIncrementLong (
    IN PLONG Addend
    );

NTKERNELAPI
INTERLOCKED_RESULT
ExIa64InterlockedDecrementLong (
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
ExIa64InterlockedExchangeUlong (
    IN PULONG Target,
    IN ULONG Value
    );

// begin_wdm

//
// IA64 Interrupt Definitions.
//
// Define length of interrupt object dispatch code in longwords.
//

// Please contact INTEL to get IA64-specific information
// @@BEGIN_DDKSPLIT
#define DISPATCH_LENGTH 2*2               // Intel-IA64-Filler ; Length of dispatch code template in 32-bit words
// @@END_DDKSPLIT

//
// Begin of a block of definitions that must be synchronized with kxia64.h.
//

//
// Define Interrupt Request Levels.
//

#define PASSIVE_LEVEL            0      // Passive release level
#define LOW_LEVEL                0      // Lowest interrupt level
#define APC_LEVEL                1      // APC interrupt level
#define DISPATCH_LEVEL           2      // Dispatcher level
#define CMC_LEVEL                3      // Correctable machine check level
#define DEVICE_LEVEL_BASE        4      // 4 - 11 - Device IRQLs
#define PROFILE_LEVEL           12      // Profiling level
#define PC_LEVEL                12      // Performance Counter IRQL
#define SYNCH_LEVEL             (IPI_LEVEL-1)      // Synchronization level
#define IPI_LEVEL               14      // IPI IRQL
#define CLOCK_LEVEL             13      // Clock Timer IRQL
#define POWER_LEVEL             15      // Power failure level
#define HIGH_LEVEL              15      // Highest interrupt level

// Please contact INTEL to get IA64-specific information
// @@BEGIN_DDKSPLIT
//
// The current IRQL is maintained in the TPR.mic field. The         // Intel-IA64-Filler
// shift count is the number of bits to shift right to extract the  // Intel-IA64-Filler
// IRQL from the TPR. See the GET/SET_IRQL macros.                  // Intel-IA64-Filler
//

#define TPR_MIC        4                                            // Intel-IA64-Filler
#define TPR_IRQL_SHIFT TPR_MIC                                      // Intel-IA64-Filler

// To go from vector number <-> IRQL we just do a shift             // Intel-IA64-Filler
#define VECTOR_IRQL_SHIFT TPR_IRQL_SHIFT                            // Intel-IA64-Filler

//                                                                  // Intel-IA64-Filler
// Interrupt Vector Definitions                                     // Intel-IA64-Filler
//                                                                  // Intel-IA64-Filler

#define APC_VECTOR          APC_LEVEL << VECTOR_IRQL_SHIFT          // Intel-IA64-Filler
#define DISPATCH_VECTOR     DISPATCH_LEVEL << VECTOR_IRQL_SHIFT     // Intel-IA64-Filler

// @@END_DDKSPLIT

//
// End of a block of definitions that must be synchronized with kxia64.h.
//

//
// Define profile intervals.
//

#define DEFAULT_PROFILE_COUNT 0x40000000 // ~= 20 seconds @50mhz
#define DEFAULT_PROFILE_INTERVAL (10 * 500) // 500 microseconds
#define MAXIMUM_PROFILE_INTERVAL (10 * 1000 * 1000) // 1 second
#define MINIMUM_PROFILE_INTERVAL (10 * 40) // 40 microseconds

#if defined(_M_IA64) && !defined(RC_INVOKED)

#define InterlockedAdd _InterlockedAdd
#define InterlockedIncrement _InterlockedIncrement
#define InterlockedDecrement _InterlockedDecrement
#define InterlockedExchange _InterlockedExchange
#define InterlockedExchangeAdd _InterlockedExchangeAdd

#define InterlockedAdd64 _InterlockedAdd64
#define InterlockedIncrement64 _InterlockedIncrement64
#define InterlockedDecrement64 _InterlockedDecrement64
#define InterlockedExchange64 _InterlockedExchange64
#define InterlockedExchangeAdd64 _InterlockedExchangeAdd64
#define InterlockedCompareExchange64 _InterlockedCompareExchange64

#define InterlockedCompareExchange _InterlockedCompareExchange
#define InterlockedExchangePointer _InterlockedExchangePointer
#define InterlockedCompareExchangePointer _InterlockedCompareExchangePointer

LONG
__cdecl
InterlockedAdd (
    LONG *Addend,
    LONG Value
    );

LONGLONG
__cdecl
InterlockedAdd64 (
    LONGLONG *Addend,
    LONGLONG Value
    );

LONG
__cdecl
InterlockedIncrement(
    IN OUT PLONG Addend
    );

LONG
__cdecl
InterlockedDecrement(
    IN OUT PLONG Addend
    );

LONG
__cdecl
InterlockedExchange(
    IN OUT PLONG Target,
    IN LONG Value
    );

LONG
__cdecl
InterlockedExchangeAdd(
    IN OUT PLONG Addend,
    IN LONG Value
    );

LONG
__cdecl
InterlockedCompareExchange (
    IN OUT PLONG Destination,
    IN LONG ExChange,
    IN LONG Comperand
    );

LONGLONG
__cdecl
InterlockedIncrement64(
    IN OUT PLONGLONG Addend
    );

LONGLONG
__cdecl
InterlockedDecrement64(
    IN OUT PLONGLONG Addend
    );

LONGLONG
__cdecl
InterlockedExchange64(
    IN OUT PLONGLONG Target,
    IN LONGLONG Value
    );

LONGLONG
__cdecl
InterlockedExchangeAdd64(
    IN OUT PLONGLONG Addend,
    IN LONGLONG Value
    );

LONGLONG
__cdecl
InterlockedCompareExchange64 (
    IN OUT PLONGLONG Destination,
    IN LONGLONG ExChange,
    IN LONGLONG Comperand
    );

PVOID
__cdecl
InterlockedCompareExchangePointer (
    IN OUT PVOID *Destination,
    IN PVOID Exchange,
    IN PVOID Comperand
    );

PVOID
__cdecl
InterlockedExchangePointer(
    IN OUT PVOID *Target,
    IN PVOID Value
    );

#pragma intrinsic(_InterlockedAdd)
#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement)
#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedCompareExchange)
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedAdd64)
#pragma intrinsic(_InterlockedIncrement64)
#pragma intrinsic(_InterlockedDecrement64)
#pragma intrinsic(_InterlockedExchange64)
#pragma intrinsic(_InterlockedCompareExchange64)
#pragma intrinsic(_InterlockedExchangeAdd64)
#pragma intrinsic(_InterlockedExchangePointer)
#pragma intrinsic(_InterlockedCompareExchangePointer)

#endif // defined(_M_IA64) && !defined(RC_INVOKED)

// end_ntddk end_nthal end_wdm

#define KiSynchIrql SYNCH_LEVEL         // enable portable code
#define KiProfileIrql PROFILE_LEVEL     // enable portable code


// Please contact INTEL to get IA64-specific information
// @@BEGIN_DDKSPLIT
//
// Sanitize FPSR based on processor mode.
//
// If kernel mode, then
//      let caller specify all bits, except reserved
//
// If user mode, then
//      let the caller specify all bits, except reserved
//

#define SANITIZE_FSR(fsr, mode) (  /* Intel-IA64-Filler */ \
    ((mode) == KernelMode ?  /* Intel-IA64-Filler */ \
        ((0x0000000000000000UL) | ((fsr) & ~(MASK_IA64(FPSR_MBZ0,FPSR_MBZ0_V)))) :  /* Intel-IA64-Filler */ \
        ((0x0000000000000000UL) | ((fsr) & ~(MASK_IA64(FPSR_MBZ0,FPSR_MBZ0_V))))  /* Intel-IA64-Filler */ \
    )  /* Intel-IA64-Filler */ \
                                )  // Intel-IA64-Filler
//
// Define SANITIZE_PSR for IA64     // Intel-IA64-Filler
//
// If kernel mode, then     // Intel-IA64-Filler
//      force clearing of BE, SP, CPL, MC, PK, DFL, reserved (MBZ)     // Intel-IA64-Filler
//      force the setting of IC, DT, DFH, DI, LP, RT, IT      // Intel-IA64-Filler
//      let caller specify UP, AC, I, BN, PP, SI, DB, TB, IS, ID, DA, DD, SS, RI, ED     // Intel-IA64-Filler
//
// If user mode, then     // Intel-IA64-Filler
//      force clearing of MC, PK, LP, reserved     // Intel-IA64-Filler
//      force the setting of BN, IC, I, DT, RT, CPL, IT     // Intel-IA64-Filler
//      let caller specify BE, UP, PP, AC, DFL, DFH, SP, SI, DI, DB, TB, IS, ID, DA, DD, SS, RI, ED     // Intel-IA64-Filler
//

#define PSR_KERNEL_CLR  (MASK_IA64(PSR_BE,1i64) | MASK_IA64(PSR_SP,1i64) | MASK_IA64(PSR_PK,1i64) | /* Intel-IA64-Filler */ \
                         MASK_IA64(PSR_CPL,0x3i64) | MASK_IA64(PSR_MC,1i64) | MASK_IA64(PSR_MBZ0,PSR_MBZ0_V) | /* Intel-IA64-Filler */ \
                         MASK_IA64(PSR_MBZ1,PSR_MBZ1_V) | MASK_IA64(PSR_MBZ2,PSR_MBZ2) | /* Intel-IA64-Filler */ \
                         MASK_IA64(PSR_DFL, 1i64)) // Intel-IA64-Filler

#define PSR_KERNEL_SET  (MASK_IA64(PSR_IC,1i64) | MASK_IA64(PSR_DT,1i64) | MASK_IA64(PSR_DFH,1i64) | /* Intel-IA64-Filler */ \
                         MASK_IA64(PSR_DI,1i64) | MASK_IA64(PSR_IT,1i64) | /* Intel-IA64-Filler */ \
                         MASK_IA64(PSR_RT,1i64)) // Intel-IA64-Filler

#define PSR_KERNEL_CPY  (MASK_IA64(PSR_UP,1i64) | MASK_IA64(PSR_AC,1i64) | /* Intel-IA64-Filler */ \
                         MASK_IA64(PSR_I,1i64) | MASK_IA64(PSR_BN,1i64)  | /* Intel-IA64-Filler */ \
                         MASK_IA64(PSR_PP,1i64) | MASK_IA64(PSR_SI,1i64) | MASK_IA64(PSR_DB,1i64) | /* Intel-IA64-Filler */ \
                         MASK_IA64(PSR_TB,1i64) | MASK_IA64(PSR_IS,1i64) | MASK_IA64(PSR_ID,1i64) | /* Intel-IA64-Filler */ \
                         MASK_IA64(PSR_DA,1i64) | MASK_IA64(PSR_DD,1i64) | MASK_IA64(PSR_SS,1i64) | /* Intel-IA64-Filler */ \
                         MASK_IA64(PSR_RI,0x3i64) | MASK_IA64(PSR_ED,1i64) | MASK_IA64(PSR_LP,1i64)) // Intel-IA64-Filler

#define PSR_USER_CLR    (MASK_IA64(PSR_MC,1i64) | /* Intel-IA64-Filler */ \
                         MASK_IA64(PSR_MBZ0,PSR_MBZ0_V) | MASK_IA64(PSR_PK,1i64) | /* Intel-IA64-Filler */ \
                         MASK_IA64(PSR_MBZ1,PSR_MBZ1_V) | MASK_IA64(PSR_MBZ2,PSR_MBZ2) | /* Intel-IA64-Filler */ \
                         MASK_IA64(PSR_LP,1i64)) // Intel-IA64-Filler

#define PSR_USER_SET    (MASK_IA64(PSR_IC,1i64) | MASK_IA64(PSR_I,1i64)  | /* Intel-IA64-Filler */ \
                         MASK_IA64(PSR_DT,1i64) | MASK_IA64(PSR_BN,1i64) | /* Intel-IA64-Filler */ \
                         MASK_IA64(PSR_RT,1i64) | /* Intel-IA64-Filler */ \
                         MASK_IA64(PSR_CPL,0x3i64) | MASK_IA64(PSR_IT,1i64)) // Intel-IA64-Filler

#define PSR_USER_CPY    (MASK_IA64(PSR_BE,1i64) | MASK_IA64(PSR_UP,1i64) | MASK_IA64(PSR_PP,1i64) |/* Intel-IA64-Filler */ \
                         MASK_IA64(PSR_AC,1i64) | MASK_IA64(PSR_DFL,1i64) | MASK_IA64(PSR_DFH,1i64) | /* Intel-IA64-Filler */ \
                         MASK_IA64(PSR_SP,1i64) | MASK_IA64(PSR_DI,1i64) | MASK_IA64(PSR_DB,1i64) | /* Intel-IA64-Filler */ \
                         MASK_IA64(PSR_TB,1i64) | MASK_IA64(PSR_IS,1i64) | MASK_IA64(PSR_ID,1i64) | /* Intel-IA64-Filler */ \
                         MASK_IA64(PSR_DA,1i64) | MASK_IA64(PSR_DD,1i64) | MASK_IA64(PSR_SS, 1i64) | /* Intel-IA64-Filler */ \
                         MASK_IA64(PSR_RI,0x3i64) | MASK_IA64(PSR_ED,1i64) | MASK_IA64(PSR_SI,1i64)) /* Intel-IA64-Filler */

#define PSR_DEBUG_SET   (MASK_IA64(PSR_DB,1i64) | MASK_IA64(PSR_SS,1i64) | MASK_IA64(PSR_TB,1i64) | /* Intel-IA64-Filler */ \
                         MASK_IA64(PSR_ID,1i64) | MASK_IA64(PSR_DD,1i64)) // Intel-IA64-Filler

#define SANITIZE_PSR(psr, mode) ( /* Intel-IA64-Filler */ \
    ((mode) == KernelMode ? /* Intel-IA64-Filler */ \
        (PSR_KERNEL_SET | ((psr) & (PSR_KERNEL_CPY | ~PSR_KERNEL_CLR))) : /* Intel-IA64-Filler */ \
        (PSR_USER_SET | ((psr) & (PSR_USER_CPY | ~PSR_USER_CLR))) /* Intel-IA64-Filler */ \
    ) /* Intel-IA64-Filler */ \
                                ) // Intel-IA64-Filler

//
// Define SANITIZE_IFS for IA64
//

#define SANITIZE_IFS(ifs, mode) ( /* Intel-IA64-Filler */ \
    ((mode) == KernelMode ? /* Intel-IA64-Filler */ \
        ((ifs) | (MASK_IA64(IFS_V,1i64))) : /* Intel-IA64-Filler */ \
        (((ifs) | (MASK_IA64(IFS_V,1i64))) & (~MASK_IA64(IFS_MBZ0, (ULONGLONG)IFS_MBZ0_V))) /* Intel-IA64-Filler */ \
    ) /* Intel-IA64-Filler */ \
                                ) // Intel-IA64-Filler

#define SANITIZE_DCR(dcr, mode) /* Intel-IA64-Filler */ \
    ((mode) == KernelMode ? dcr : USER_DCR_INITIAL) // Intel-IA64-Filler

//
// Macro to sanitize debug registers
//

#define SANITIZE_DR(dr, mode) /* Intel-IA64-Filler */ \
    ((mode) == KernelMode ? /* Intel-IA64-Filler */ \
        (dr) : /* Intel-IA64-Filler */ \
        (dr & ~(0x7i64 << DR_PLM0)) /* disable pl 0-2 */ /* Intel-IA64-Filler */ \
    ) /* Intel-IA64-Filler */
// @@END_DDKSPLIT


// begin_nthal

// Please contact INTEL to get IA64-specific information
// @@BEGIN_DDKSPLIT
//
// Define interrupt request physical address (maps to HAL virtual address)
//

#define INTERRUPT_REQUEST_PHYSICAL_ADDRESS  0xFFE00000 // Intel-IA64-Filler

//
// Define Address of Processor Control Registers. // Intel-IA64-Filler
//

// @@END_DDKSPLIT

//
// Define Pointer to Processor Control Registers.
//

#define KIPCR ((ULONG_PTR)(KADDRESS_BASE + 0xFFFF0000))            // kernel address of first PCR
#define PCR ((volatile KPCR * const)KIPCR)


// begin_ntddk begin_wdm

#define KI_USER_SHARED_DATA ((ULONG_PTR)(KADDRESS_BASE + 0xFFFE0000))
#define SharedUserData ((KUSER_SHARED_DATA * const)KI_USER_SHARED_DATA)

//
// Prototype for get current IRQL. **** TBD (read TPR)
//

NTKERNELAPI
KIRQL
KeGetCurrentIrql();

// end_wdm

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
// Get current processor number.
//

#define KeGetCurrentProcessorNumber() PCR->Number

//
// Get data cache fill size.
//

#define KeGetDcacheFillSize() PCR->DcacheFillSize

// end_ntddk end_nthal

//
// Get previous processor mode.
//

#define KeGetPreviousMode() (KPROCESSOR_MODE)PCR->CurrentThread->PreviousMode

//
// Test if executing a DPC.
//

#define KeIsExecutingDpc() (PCR->Prcb->DpcRoutineActive != 0)

//
// Save & Restore floating point state
//
// begin_ntddk begin_wdm

#define KeSaveFloatingPointState(a)         STATUS_SUCCESS
#define KeRestoreFloatingPointState(a)      STATUS_SUCCESS

// end_ntddk end_wdm


// begin_ntddk begin_nthal begin_ntndis begin_wdm

//
// Define the page size
//

#define PAGE_SIZE 0x2000

//
// Define the number of trailing zeroes in a page aligned virtual address.
// This is used as the shift count when shifting virtual addresses to
// virtual page numbers.
//

#define PAGE_SHIFT 13L

// end_ntddk end_nthal end_ntndis end_wdm

// begin_nthal
//
// IA64 hardware structures
//

// Please contact INTEL to get IA64-specific information
// @@BEGIN_DDKSPLIT

//
// A Page Table Entry on an IA64 has the following definition.
//

#define _HARDWARE_PTE_WORKING_SET_BITS  11 // Intel-IA64-Filler

typedef struct _HARDWARE_PTE {            // Intel-IA64-Filler
    ULONG64 Valid : 1;                    // Intel-IA64-Filler
    ULONG64 Rsvd0 : 1;                    // Intel-IA64-Filler
    ULONG64 Cache : 3;                    // Intel-IA64-Filler
    ULONG64 Accessed : 1;                 // Intel-IA64-Filler
    ULONG64 Dirty : 1;                    // Intel-IA64-Filler
    ULONG64 Owner : 2;                    // Intel-IA64-Filler
    ULONG64 Execute : 1;                  // Intel-IA64-Filler
    ULONG64 Write : 1;                    // Intel-IA64-Filler
    ULONG64 Rsvd1 : PAGE_SHIFT - 12;      // Intel-IA64-Filler
    ULONG64 CopyOnWrite : 1;              // Intel-IA64-Filler
    ULONG64 PageFrameNumber : 50 - PAGE_SHIFT;  // Intel-IA64-Filler
    ULONG64 Rsvd2 : 2;                    // Intel-IA64-Filler
    ULONG64 Exception : 1;                // Intel-IA64-Filler
    ULONGLONG SoftwareWsIndex : _HARDWARE_PTE_WORKING_SET_BITS; // Intel-IA64-Filler
} HARDWARE_PTE, *PHARDWARE_PTE;           // Intel-IA64-Filler

//
// Fill TB entry // Intel-IA64-Filler
//
// Filling TB entry on demand by VHPT H/W seems faster than done by s/w. // Intel-IA64-Filler
// Determining I/D side of TLB, disabling/enabling PSR.i and ic bits, // Intel-IA64-Filler
// serialization, writing to IIP, IDA, IDTR and IITR seem just too much // Intel-IA64-Filler
// compared to VHPT searching it automatically. // Intel-IA64-Filler
//

#define KiVhptEntry(va)  ((PVOID)__thash((__int64)va)) // Intel-IA64-Filler
#define KiVhptEntryTag(va)  ((ULONGLONG)__ttag((__int64)va)) // Intel-IA64-Filler

#define KiFlushSingleTb(Invalid, va)   /* Intel-IA64-Filler */                \
    __ptcl((__int64)va,PAGE_SHIFT << 2);  __isrlz() // Intel-IA64-Filler

#define KeFillEntryTb(PointerPte, Virtual, Invalid) /* Intel-IA64-Filler */   \
    if (Invalid != FALSE) { /* Intel-IA64-Filler */\
       KiFlushSingleTb(0, Virtual);     /* Intel-IA64-Filler */               \
    } // Intel-IA64-Filler

#define KiFlushFixedInstTb(Invalid, va)  /* Intel-IA64-Filler */ \
    __ptri((__int64)va, PAGE_SHIFT << 2); __isrlz() // Intel-IA64-Filler

#define KiFlushFixedDataTb(Invalid, va)  /* Intel-IA64-Filler */ \
    __ptrd((__int64)va, PAGE_SHIFT << 2); __dsrlz() // Intel-IA64-Filler

// @@END_DDKSPLIT

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

NTKERNELAPI
VOID
KeFillFixedLargeEntryTb (
    IN HARDWARE_PTE Pte[2],
    IN PVOID Virtual,
    IN ULONG PageSize,
    IN ULONG Index
    );

//
// Fill Inst TB entry
//

NTKERNELAPI
VOID
KeFillInstEntryTb (
    IN HARDWARE_PTE Pte,
    IN PVOID Virtual
    );

//
// Get a VHPT entry address
//

PVOID
KiVhptEntry64(
   IN ULONG VirtualPageNumber
   );

//
// Get a VHPT entry TAG value
//

ULONGLONG
KiVhptEntryTag64(
    IN ULONG VirtualPageNumber
    );

//
// Fill a VHPT entry
//

VOID
KiFillEntryVhpt(
   IN PHARDWARE_PTE PointerPte,
   IN PVOID Virtual
   );


//
// Flush the kernel portions of Tb
//


VOID
KeFlushKernelTb(
    IN BOOLEAN AllProcessors
    );

//
// Flush the user portions of Tb
//

VOID
KeFlushUserTb(
    IN BOOLEAN AllProcessors
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

#define KeSweepCurrentDcache()

NTKERNELAPI
VOID
KeSweepIcache (
    IN BOOLEAN AllProcessors
    );

#define KeSweepCurrentIcache()

NTKERNELAPI
VOID
KeSweepIcacheRange (
    IN BOOLEAN AllProcessors,
    IN PVOID BaseAddress,
    IN ULONG Length
    );

NTKERNELAPI
VOID
KeSweepCacheRangeWithDrain (
    IN BOOLEAN AllProcessors,
    IN PVOID BaseAddress,
    IN ULONG Length
    );

// begin_ntddk begin_ntndis begin_wdm
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

// end_ntddk end_ntndis end_wdm

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
    IN ULONG Increment
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

#define KiReleaseSpinLock(SpinLock) _ReleaseSpinLock(SpinLock)

#endif

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
// Define cache error routine type and prototype.
//

typedef
VOID
(*PKCACHE_ERROR_ROUTINE) (
    VOID
    );

NTKERNELAPI
VOID
KeSetCacheErrorRoutine (
    IN PKCACHE_ERROR_ROUTINE Routine
    );

// begin_ntddk begin_wdm

//
// Kernel breakin breakpoint
//

VOID
KeBreakinBreakpoint (
    VOID
    );

// end_ntddk end_nthal end_wdm

//
// Define executive macros for acquiring and releasing executive spinlocks.
// These macros can ONLY be used by executive components and NOT by drivers.
// Drivers MUST use the kernel interfaces since they must be MP enabled on
// all systems.
//

#if defined(NT_UP) && !defined(_NTDDK_) && !defined(_NTIFS_)
#define ExAcquireSpinLock(Lock, OldIrql) KeRaiseIrql(DISPATCH_LEVEL, (OldIrql))
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

#if defined(_M_IA64)

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

#define KiQuerySystemTime(CurrentTime)     \
    while (TRUE) {                                                             \
        (CurrentTime)->HighPart = SharedUserData->SystemHigh1Time;             \
        (CurrentTime)->LowPart = SharedUserData->SystemLowTime;                \
        if ((CurrentTime)->HighPart == SharedUserData->SystemHigh2Time) break; \
    }

//
// Define query tick count macro.
//
// begin_ntddk begin_nthal

#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_)

// begin_wdm

#define KeQueryTickCount(CurrentCount ) \
    *(PULONGLONG)(CurrentCount) = **((volatile ULONGLONG **)(&KeTickCount));

// end_wdm

#else

#define KiQueryTickCount(CurrentCount) \
    *(PULONGLONG)(CurrentCount) = KeTickCount;

NTKERNELAPI
VOID
KeQueryTickCount (
    OUT PLARGE_INTEGER CurrentCount
    );

#endif // defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_)

// end_ntddk end_nthal

#define KiQueryLowTickCount() (ULONG)KeTickCount

//
// Define query interrupt time macro.
//

#define KiQueryInterruptTime(CurrentTime) \
    *(PULONGLONG)(CurrentTime) = SharedUserData->InterruptTime

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

ULONGLONG
KiGetRegisterValue (
    IN ULONG Register,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame
    );

VOID
KiSetRegisterValue (
    IN ULONG Register,
    IN ULONGLONG Value,
    OUT struct _KEXCEPTION_FRAME *ExceptionFrame,
    OUT struct _KTRAP_FRAME *TrapFrame
    );

FLOAT128
KiGetFloatRegisterValue (
    IN ULONG Register,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame
    );

VOID
KiSetFloatRegisterValue (
    IN ULONG Register,
    IN FLOAT128 Value,
    OUT struct _KEXCEPTION_FRAME *ExceptionFrame,
    OUT struct _KTRAP_FRAME *TrapFrame
    );

VOID
KiAdvanceInstPointer(
    IN OUT struct _KTRAP_FRAME *TrapFrame
    );

VOID
KiRequestSoftwareInterrupt (
    KIRQL RequestIrql
    );

// begin_ntddk begin_nthal begin_ntndis begin_wdm
//
// I/O space read and write macros.
//

NTHALAPI
UCHAR
READ_PORT_UCHAR (
    PUCHAR RegisterAddress
    );

NTHALAPI
USHORT
READ_PORT_USHORT (
    PUSHORT RegisterAddress
    );

NTHALAPI
ULONG
READ_PORT_ULONG (
    PULONG RegisterAddress
    );

NTHALAPI
VOID
READ_PORT_BUFFER_UCHAR (
    PUCHAR portAddress,
    PUCHAR readBuffer,
    ULONG  readCount
    );

NTHALAPI
VOID
READ_PORT_BUFFER_USHORT (
    PUSHORT portAddress,
    PUSHORT readBuffer,
    ULONG  readCount
    );

NTHALAPI
VOID
READ_PORT_BUFFER_ULONG (
    PULONG portAddress,
    PULONG readBuffer,
    ULONG  readCount
    );

NTHALAPI
VOID
WRITE_PORT_UCHAR (
    PUCHAR portAddress,
    UCHAR  Data
    );

NTHALAPI
VOID
WRITE_PORT_USHORT (
    PUSHORT portAddress,
    USHORT  Data
    );

NTHALAPI
VOID
WRITE_PORT_ULONG (
    PULONG portAddress,
    ULONG  Data
    );

NTHALAPI
VOID
WRITE_PORT_BUFFER_UCHAR (
    PUCHAR portAddress,
    PUCHAR writeBuffer,
    ULONG  writeCount
    );

NTHALAPI
VOID
WRITE_PORT_BUFFER_USHORT (
    PUSHORT portAddress,
    PUSHORT writeBuffer,
    ULONG  writeCount
    );

NTHALAPI
VOID
WRITE_PORT_BUFFER_ULONG (
    PULONG portAddress,
    PULONG writeBuffer,
    ULONG  writeCount
    );


#define READ_REGISTER_UCHAR(x) \
    (__mf(), *(volatile UCHAR * const)(x))

#define READ_REGISTER_USHORT(x) \
    (__mf(), *(volatile USHORT * const)(x))

#define READ_REGISTER_ULONG(x) \
    (__mf(), *(volatile ULONG * const)(x))

#define READ_REGISTER_BUFFER_UCHAR(x, y, z) {                           \
    PUCHAR registerBuffer = x;                                          \
    PUCHAR readBuffer = y;                                              \
    ULONG readCount;                                                    \
    __mf();                                                             \
    for (readCount = z; readCount--; readBuffer++, registerBuffer++) {  \
        *readBuffer = *(volatile UCHAR * const)(registerBuffer);        \
    }                                                                   \
}

#define READ_REGISTER_BUFFER_USHORT(x, y, z) {                          \
    PUSHORT registerBuffer = x;                                         \
    PUSHORT readBuffer = y;                                             \
    ULONG readCount;                                                    \
    __mf();                                                             \
    for (readCount = z; readCount--; readBuffer++, registerBuffer++) {  \
        *readBuffer = *(volatile USHORT * const)(registerBuffer);       \
    }                                                                   \
}

#define READ_REGISTER_BUFFER_ULONG(x, y, z) {                           \
    PULONG registerBuffer = x;                                          \
    PULONG readBuffer = y;                                              \
    ULONG readCount;                                                    \
    __mf();                                                             \
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

// end_ntddk end_ntndis end_wdm


// Please contact INTEL to get IA64-specific information
// @@BEGIN_DDKSPLIT

//
// Higher FP volatile // Intel-IA64-Filler
//
//  This structure defines the higher FP volatile registers. // Intel-IA64-Filler
//

typedef struct _KHIGHER_FP_VOLATILE { // Intel-IA64-Filler
    // volatile higher floating registers f32 - f127 // Intel-IA64-Filler
    FLOAT128 FltF32; // Intel-IA64-Filler
    FLOAT128 FltF33; // Intel-IA64-Filler
    FLOAT128 FltF34; // Intel-IA64-Filler
    FLOAT128 FltF35; // Intel-IA64-Filler
    FLOAT128 FltF36; // Intel-IA64-Filler
    FLOAT128 FltF37; // Intel-IA64-Filler
    FLOAT128 FltF38; // Intel-IA64-Filler
    FLOAT128 FltF39; // Intel-IA64-Filler
    FLOAT128 FltF40; // Intel-IA64-Filler
    FLOAT128 FltF41; // Intel-IA64-Filler
    FLOAT128 FltF42; // Intel-IA64-Filler
    FLOAT128 FltF43; // Intel-IA64-Filler
    FLOAT128 FltF44; // Intel-IA64-Filler
    FLOAT128 FltF45; // Intel-IA64-Filler
    FLOAT128 FltF46; // Intel-IA64-Filler
    FLOAT128 FltF47; // Intel-IA64-Filler
    FLOAT128 FltF48; // Intel-IA64-Filler
    FLOAT128 FltF49; // Intel-IA64-Filler
    FLOAT128 FltF50; // Intel-IA64-Filler
    FLOAT128 FltF51; // Intel-IA64-Filler
    FLOAT128 FltF52; // Intel-IA64-Filler
    FLOAT128 FltF53; // Intel-IA64-Filler
    FLOAT128 FltF54; // Intel-IA64-Filler
    FLOAT128 FltF55; // Intel-IA64-Filler
    FLOAT128 FltF56; // Intel-IA64-Filler
    FLOAT128 FltF57; // Intel-IA64-Filler
    FLOAT128 FltF58; // Intel-IA64-Filler
    FLOAT128 FltF59; // Intel-IA64-Filler
    FLOAT128 FltF60; // Intel-IA64-Filler
    FLOAT128 FltF61; // Intel-IA64-Filler
    FLOAT128 FltF62; // Intel-IA64-Filler
    FLOAT128 FltF63; // Intel-IA64-Filler
    FLOAT128 FltF64; // Intel-IA64-Filler
    FLOAT128 FltF65; // Intel-IA64-Filler
    FLOAT128 FltF66; // Intel-IA64-Filler
    FLOAT128 FltF67; // Intel-IA64-Filler
    FLOAT128 FltF68; // Intel-IA64-Filler
    FLOAT128 FltF69; // Intel-IA64-Filler
    FLOAT128 FltF70; // Intel-IA64-Filler
    FLOAT128 FltF71; // Intel-IA64-Filler
    FLOAT128 FltF72; // Intel-IA64-Filler
    FLOAT128 FltF73; // Intel-IA64-Filler
    FLOAT128 FltF74; // Intel-IA64-Filler
    FLOAT128 FltF75; // Intel-IA64-Filler
    FLOAT128 FltF76; // Intel-IA64-Filler
    FLOAT128 FltF77; // Intel-IA64-Filler
    FLOAT128 FltF78; // Intel-IA64-Filler
    FLOAT128 FltF79; // Intel-IA64-Filler
    FLOAT128 FltF80; // Intel-IA64-Filler
    FLOAT128 FltF81; // Intel-IA64-Filler
    FLOAT128 FltF82; // Intel-IA64-Filler
    FLOAT128 FltF83; // Intel-IA64-Filler
    FLOAT128 FltF84; // Intel-IA64-Filler
    FLOAT128 FltF85; // Intel-IA64-Filler
    FLOAT128 FltF86; // Intel-IA64-Filler
    FLOAT128 FltF87; // Intel-IA64-Filler
    FLOAT128 FltF88; // Intel-IA64-Filler
    FLOAT128 FltF89; // Intel-IA64-Filler
    FLOAT128 FltF90; // Intel-IA64-Filler
    FLOAT128 FltF91; // Intel-IA64-Filler
    FLOAT128 FltF92; // Intel-IA64-Filler
    FLOAT128 FltF93; // Intel-IA64-Filler
    FLOAT128 FltF94; // Intel-IA64-Filler
    FLOAT128 FltF95; // Intel-IA64-Filler
    FLOAT128 FltF96; // Intel-IA64-Filler
    FLOAT128 FltF97; // Intel-IA64-Filler
    FLOAT128 FltF98; // Intel-IA64-Filler
    FLOAT128 FltF99; // Intel-IA64-Filler
    FLOAT128 FltF100; // Intel-IA64-Filler
    FLOAT128 FltF101; // Intel-IA64-Filler
    FLOAT128 FltF102; // Intel-IA64-Filler
    FLOAT128 FltF103; // Intel-IA64-Filler
    FLOAT128 FltF104; // Intel-IA64-Filler
    FLOAT128 FltF105; // Intel-IA64-Filler
    FLOAT128 FltF106; // Intel-IA64-Filler
    FLOAT128 FltF107; // Intel-IA64-Filler
    FLOAT128 FltF108; // Intel-IA64-Filler
    FLOAT128 FltF109; // Intel-IA64-Filler
    FLOAT128 FltF110; // Intel-IA64-Filler
    FLOAT128 FltF111; // Intel-IA64-Filler
    FLOAT128 FltF112; // Intel-IA64-Filler
    FLOAT128 FltF113; // Intel-IA64-Filler
    FLOAT128 FltF114; // Intel-IA64-Filler
    FLOAT128 FltF115; // Intel-IA64-Filler
    FLOAT128 FltF116; // Intel-IA64-Filler
    FLOAT128 FltF117; // Intel-IA64-Filler
    FLOAT128 FltF118; // Intel-IA64-Filler
    FLOAT128 FltF119; // Intel-IA64-Filler
    FLOAT128 FltF120; // Intel-IA64-Filler
    FLOAT128 FltF121; // Intel-IA64-Filler
    FLOAT128 FltF122; // Intel-IA64-Filler
    FLOAT128 FltF123; // Intel-IA64-Filler
    FLOAT128 FltF124; // Intel-IA64-Filler
    FLOAT128 FltF125; // Intel-IA64-Filler
    FLOAT128 FltF126; // Intel-IA64-Filler
    FLOAT128 FltF127; // Intel-IA64-Filler

} KHIGHER_FP_VOLATILE, *PKHIGHER_FP_VOLATILE; // Intel-IA64-Filler

//
// Debug registers // Intel-IA64-Filler
//
// This structure defines the hardware debug registers. // Intel-IA64-Filler
// We allow space for 4 pairs of instruction and 4 pairs of data debug registers // Intel-IA64-Filler
// The hardware may actually have more. // Intel-IA64-Filler
//

typedef struct _KDEBUG_REGISTERS { // Intel-IA64-Filler

    ULONGLONG DbI0; // Intel-IA64-Filler
    ULONGLONG DbI1; // Intel-IA64-Filler
    ULONGLONG DbI2; // Intel-IA64-Filler
    ULONGLONG DbI3; // Intel-IA64-Filler
    ULONGLONG DbI4; // Intel-IA64-Filler
    ULONGLONG DbI5; // Intel-IA64-Filler
    ULONGLONG DbI6; // Intel-IA64-Filler
    ULONGLONG DbI7; // Intel-IA64-Filler

    ULONGLONG DbD0; // Intel-IA64-Filler
    ULONGLONG DbD1; // Intel-IA64-Filler
    ULONGLONG DbD2; // Intel-IA64-Filler
    ULONGLONG DbD3; // Intel-IA64-Filler
    ULONGLONG DbD4; // Intel-IA64-Filler
    ULONGLONG DbD5; // Intel-IA64-Filler
    ULONGLONG DbD6; // Intel-IA64-Filler
    ULONGLONG DbD7; // Intel-IA64-Filler

} KDEBUG_REGISTERS, *PKDEBUG_REGISTERS; // Intel-IA64-Filler

//
// misc. application registers (mapped to IA-32 registers)
//

typedef struct _KAPPLICATION_REGISTERS {
    ULONGLONG Ar21;
    ULONGLONG Ar24;
    ULONGLONG Ar25;
    ULONGLONG Ar26;
    ULONGLONG Ar27;
    ULONGLONG Ar28;
    ULONGLONG Ar29;
    ULONGLONG Ar30;
} KAPPLICATION_REGISTERS, *PKAPPLICATION_REGISTERS;

//
// performance registers
//

typedef struct _KPERFORMANCE_REGISTERS {
    ULONGLONG Perfr0;
    ULONGLONG Perfr1;
    ULONGLONG Perfr2;
    ULONGLONG Perfr3;
    ULONGLONG Perfr4;
    ULONGLONG Perfr5;
    ULONGLONG Perfr6;
    ULONGLONG Perfr7;
} KPERFORMANCE_REGISTERS, *PKPERFORMANCE_REGISTERS;

//
// Thread State save area. Currently, beginning of Kernel Stack // Intel-IA64-Filler
//
// This structure defines the area for: // Intel-IA64-Filler
//
//      higher fp register save/restore // Intel-IA64-Filler
//      user debug register save/restore. // Intel-IA64-Filler
//
// The order of these area is significant.
//

typedef struct _KTHREAD_STATE_SAVEAREA { // Intel-IA64-Filler

    KAPPLICATION_REGISTERS AppRegisters;
    KPERFORMANCE_REGISTERS PerfRegisters;
    KHIGHER_FP_VOLATILE HigherFPVolatile; // Intel-IA64-Filler
    KDEBUG_REGISTERS DebugRegisters; // Intel-IA64-Filler

} KTHREAD_STATE_SAVEAREA, *PKTHREAD_STATE_SAVEAREA; // Intel-IA64-Filler

#define KTHREAD_STATE_SAVEAREA_LENGTH ((sizeof(KTHREAD_STATE_SAVEAREA) + 15) & ~((ULONG_PTR)15))

#define GET_HIGH_FLOATING_POINT_REGISTER_SAVEAREA()         \
    (PKHIGHER_FP_VOLATILE) &(((PKTHREAD_STATE_SAVEAREA)(((ULONG_PTR)PCR->InitialStack - sizeof(KTHREAD_STATE_SAVEAREA)) & ~((ULONG_PTR)15)))->HigherFPVolatile)

#define GET_DEBUG_REGISTER_SAVEAREA()                       \
    (PKDEBUG_REGISTERS) &(((PKTHREAD_STATE_SAVEAREA)(((ULONG_PTR)KeGetCurrentThread()->StackBase - sizeof(KTHREAD_STATE_SAVEAREA)) & ~((ULONG_PTR)15)))->DebugRegisters)

#define GET_APPLICATION_REGISTER_SAVEAREA(StackBase)     \
    (PKAPPLICATION_REGISTERS) &(((PKTHREAD_STATE_SAVEAREA)(((ULONG_PTR)StackBase - sizeof(KTHREAD_STATE_SAVEAREA)) & ~((ULONG_PTR)15)))->AppRegisters)

// @@END_DDKSPLIT

//
// Exception frame
//
//  This frame is established when handling an exception. It provides a place
//  to save all preserved registers. The volatile registers will already
//  have been saved in a trap frame. Also used as part of switch frame built
//  at thread switch.
//
//  The frame is 16-byte aligned to maintain 16-byte alignment for the stack,
//

typedef struct _KEXCEPTION_FRAME {

// Please contact INTEL to get IA64-specific information
// @@BEGIN_DDKSPLIT

    // Preserved application registers // Intel-IA64-Filler
    ULONGLONG ApEC;       // epilogue count // Intel-IA64-Filler
    ULONGLONG ApLC;       // loop count // Intel-IA64-Filler
    ULONGLONG IntNats;    // Nats for S0-S3; i.e. ar.UNAT after spill // Intel-IA64-Filler

    // Preserved (saved) interger registers, s0-s3 // Intel-IA64-Filler
    ULONGLONG IntS0; // Intel-IA64-Filler
    ULONGLONG IntS1; // Intel-IA64-Filler
    ULONGLONG IntS2; // Intel-IA64-Filler
    ULONGLONG IntS3; // Intel-IA64-Filler

    // Preserved (saved) branch registers, bs0-bs4 // Intel-IA64-Filler
    ULONGLONG BrS0; // Intel-IA64-Filler
    ULONGLONG BrS1; // Intel-IA64-Filler
    ULONGLONG BrS2; // Intel-IA64-Filler
    ULONGLONG BrS3; // Intel-IA64-Filler
    ULONGLONG BrS4; // Intel-IA64-Filler

    // Preserved (saved) floating point registers, f2 - f5, f16 - f31 // Intel-IA64-Filler
    FLOAT128 FltS0; // Intel-IA64-Filler
    FLOAT128 FltS1; // Intel-IA64-Filler
    FLOAT128 FltS2; // Intel-IA64-Filler
    FLOAT128 FltS3; // Intel-IA64-Filler
    FLOAT128 FltS4; // Intel-IA64-Filler
    FLOAT128 FltS5; // Intel-IA64-Filler
    FLOAT128 FltS6; // Intel-IA64-Filler
    FLOAT128 FltS7; // Intel-IA64-Filler
    FLOAT128 FltS8; // Intel-IA64-Filler
    FLOAT128 FltS9; // Intel-IA64-Filler
    FLOAT128 FltS10; // Intel-IA64-Filler
    FLOAT128 FltS11; // Intel-IA64-Filler
    FLOAT128 FltS12; // Intel-IA64-Filler
    FLOAT128 FltS13; // Intel-IA64-Filler
    FLOAT128 FltS14; // Intel-IA64-Filler
    FLOAT128 FltS15; // Intel-IA64-Filler
    FLOAT128 FltS16; // Intel-IA64-Filler
    FLOAT128 FltS17; // Intel-IA64-Filler
    FLOAT128 FltS18; // Intel-IA64-Filler
    FLOAT128 FltS19; // Intel-IA64-Filler

// @@END_DDKSPLIT

} KEXCEPTION_FRAME, *PKEXCEPTION_FRAME;

// Please contact INTEL to get IA64-specific information
// @@BEGIN_DDKSPLIT

//
// Switch frame
//
//  This frame is established when doing a thread switch in SwapContext. It
//  provides a place to save the preserved kernel state at the point of the
//  switch registers.
//  The volatile registers are scratch across the call to SwapContext.
//
//  The frame is 16-byte aligned to maintain 16-byte alignment for the stack,
//

typedef struct _KSWITCH_FRAME { // Intel-IA64-Filler

    ULONGLONG SwitchPredicates; // Predicates for Switch // Intel-IA64-Filler
    ULONGLONG SwitchRp;         // return pointer for Switch // Intel-IA64-Filler
    ULONGLONG SwitchPFS;        // PFS for Switch // Intel-IA64-Filler
    ULONGLONG SwitchFPSR;   // ProcessorFP status at thread switch // Intel-IA64-Filler
    ULONGLONG SwitchBsp;                     // Intel-IA64-Filler
    ULONGLONG SwitchRnat;                     // Intel-IA64-Filler
    // ULONGLONG Pad;

    KEXCEPTION_FRAME SwitchExceptionFrame; // Intel-IA64-Filler

} KSWITCH_FRAME, *PKSWITCH_FRAME; // Intel-IA64-Filler

// Trap frame
//  This frame is established when handling a trap. It provides a place to
//  save all volatile registers. The nonvolatile registers are saved in an
//  exception frame or through the normal C calling conventions for saved
//  registers.  Its size must be a multiple of 16 bytes.
//
//  N.B - the 16-byte alignment is required to maintain the stack alignment.
//

#define KTRAP_FRAME_ARGUMENTS (8 * 8)       // up to 8 in-memory syscall args // Intel-IA64-Filler

// @@END_DDKSPLIT

typedef struct _KTRAP_FRAME {
// Please contact INTEL to get IA64-specific information
// @@BEGIN_DDKSPLIT

    //
    // Reserved for additional memory arguments and stack scratch area
    // The size of Reserved[] must be a multiple of 16 bytes.
    //

    ULONGLONG Reserved[(KTRAP_FRAME_ARGUMENTS+16)/8]; // Intel-IA64-Filler

    // Temporary (volatile) FP registers - f6-f15 (don't use f32+ in kernel) // Intel-IA64-Filler
    FLOAT128 FltT0; // Intel-IA64-Filler
    FLOAT128 FltT1; // Intel-IA64-Filler
    FLOAT128 FltT2; // Intel-IA64-Filler
    FLOAT128 FltT3; // Intel-IA64-Filler
    FLOAT128 FltT4; // Intel-IA64-Filler
    FLOAT128 FltT5; // Intel-IA64-Filler
    FLOAT128 FltT6; // Intel-IA64-Filler
    FLOAT128 FltT7; // Intel-IA64-Filler
    FLOAT128 FltT8; // Intel-IA64-Filler
    FLOAT128 FltT9; // Intel-IA64-Filler

    // Temporary (volatile) interger registers
    ULONGLONG IntGp;    // global pointer (r1) // Intel-IA64-Filler
    ULONGLONG IntT0; // Intel-IA64-Filler
    ULONGLONG IntT1; // Intel-IA64-Filler
                        // The following 4 registers fill in space of preserved  (S0-S3) to align Nats // Intel-IA64-Filler
    ULONGLONG ApUNAT;   // ar.UNAT on kernel entry // Intel-IA64-Filler
    ULONGLONG ApCCV;    // ar.CCV // Intel-IA64-Filler
    ULONGLONG ApDCR;    // DCR register on kernel entry // Intel-IA64-Filler
    ULONGLONG Preds;    // Predicates // Intel-IA64-Filler

    ULONGLONG IntV0;    // return value (r8) // Intel-IA64-Filler
    ULONGLONG IntT2; // Intel-IA64-Filler
    ULONGLONG IntT3; // Intel-IA64-Filler
    ULONGLONG IntT4; // Intel-IA64-Filler
    ULONGLONG IntSp;    // stack pointer (r12) // Intel-IA64-Filler
    ULONGLONG IntTeb;   // teb (r13) // Intel-IA64-Filler
    ULONGLONG IntT5; // Intel-IA64-Filler
    ULONGLONG IntT6; // Intel-IA64-Filler
    ULONGLONG IntT7; // Intel-IA64-Filler
    ULONGLONG IntT8; // Intel-IA64-Filler
    ULONGLONG IntT9; // Intel-IA64-Filler
    ULONGLONG IntT10; // Intel-IA64-Filler
    ULONGLONG IntT11; // Intel-IA64-Filler
    ULONGLONG IntT12; // Intel-IA64-Filler
    ULONGLONG IntT13; // Intel-IA64-Filler
    ULONGLONG IntT14; // Intel-IA64-Filler
    ULONGLONG IntT15; // Intel-IA64-Filler
    ULONGLONG IntT16; // Intel-IA64-Filler
    ULONGLONG IntT17; // Intel-IA64-Filler
    ULONGLONG IntT18; // Intel-IA64-Filler
    ULONGLONG IntT19; // Intel-IA64-Filler
    ULONGLONG IntT20; // Intel-IA64-Filler
    ULONGLONG IntT21; // Intel-IA64-Filler
    ULONGLONG IntT22; // Intel-IA64-Filler

    ULONGLONG IntNats;  // Temporary (volatile) registers' Nats directly from ar.UNAT at point of spill // Intel-IA64-Filler

    ULONGLONG BrRp;     // Return pointer on kernel entry // Intel-IA64-Filler

    ULONGLONG BrT0;     // Temporary (volatile) branch registers (b6-b7) // Intel-IA64-Filler
    ULONGLONG BrT1; // Intel-IA64-Filler

    // Register stack info // Intel-IA64-Filler
    ULONGLONG RsRSC;    // RSC on kernel entry // Intel-IA64-Filler
    ULONGLONG RsBSP;    // BSP on kernel entry // Intel-IA64-Filler
    ULONGLONG RsBSPSTORE; // User BSP Store at point of switch to kernel backing store // Intel-IA64-Filler
    ULONGLONG RsRNAT;   // old RNAT at point of switch to kernel backing store // Intel-IA64-Filler
    ULONGLONG RsPFS;    // PFS on kernel entry // Intel-IA64-Filler

    // Trap Status Information // Intel-IA64-Filler
    ULONGLONG StIPSR;   // Interruption Processor Status Register // Intel-IA64-Filler
    ULONGLONG StIIP;    // Interruption IP // Intel-IA64-Filler
    ULONGLONG StIFS;    // Interruption Function State // Intel-IA64-Filler
    ULONGLONG StFPSR;   // FP status // Intel-IA64-Filler
    ULONGLONG StISR;    // Interruption Status Register // Intel-IA64-Filler
    ULONGLONG StIFA;    // Interruption Data Address // Intel-IA64-Filler
    ULONGLONG StIIPA;   // Last executed bundle address // Intel-IA64-Filler
    ULONGLONG StIIM;    // Interruption Immediate // Intel-IA64-Filler
    ULONGLONG StIHA;    // Interruption Hash Address // Intel-IA64-Filler

    ULONG OldIrql;      // Previous Irql. // Intel-IA64-Filler
    ULONG PreviousMode; // Previous Mode. // Intel-IA64-Filler
    ULONGLONG TrapFrame;// Previous Trap Frame // Intel-IA64-Filler
// @@END_DDKSPLIT
    // Exception record
    UCHAR ExceptionRecord[(sizeof(EXCEPTION_RECORD) + 15) & (~15)];

    // End of frame marker (for debugging)
    ULONGLONG Handler;  // Handler for this trap
    ULONGLONG EOFMarker;
} KTRAP_FRAME, *PKTRAP_FRAME;

#define KTRAP_FRAME_LENGTH ((sizeof(KTRAP_FRAME) + 15) & (~15))
#define KTRAP_FRAME_ALIGN (16)
#define KTRAP_FRAME_ROUND (KTRAP_FRAME_ALIGN - 1)
#define KTRAP_FRAME_EOF 0xe0f0e0f0e0f0e000i64

//
// Use the lowest 4 bits of EOFMarker field to encode the trap frame type
//

#define SYSCALL_FRAME      0
#define INTERRUPT_FRAME    1
#define EXCEPTION_FRAME    2
#define CONTEXT_FRAME      10

#define TRAP_FRAME_TYPE(tf)  (tf->EOFMarker & 0xf)

//
// Define the kernel mode and user mode callback frame structures.
//

//
// The frame saved by KiCallUserMode is defined here to allow
// the kernel debugger to trace the entire kernel stack
// when usermode callouts are pending.
//
// N.B. The size of the following structure must be a multiple of 16 bytes
//      and it must be 16-byte aligned.
//

typedef struct _KCALLOUT_FRAME {

// Please contact INTEL to get IA64-specific information
// @@BEGIN_DDKSPLIT

    ULONGLONG   BrRp;       // Intel-IA64-Filler
    ULONGLONG   RsPFS;       // Intel-IA64-Filler
    ULONGLONG   Preds;       // Intel-IA64-Filler
    ULONGLONG   ApUNAT;       // Intel-IA64-Filler
    ULONGLONG   ApLC;       // Intel-IA64-Filler
    ULONGLONG   RsRNAT;       // Intel-IA64-Filler
    ULONGLONG   IntNats;       // Intel-IA64-Filler

    ULONGLONG   IntS0;       // Intel-IA64-Filler
    ULONGLONG   IntS1;       // Intel-IA64-Filler
    ULONGLONG   IntS2;       // Intel-IA64-Filler
    ULONGLONG   IntS3;       // Intel-IA64-Filler

    ULONGLONG   BrS0;       // Intel-IA64-Filler
    ULONGLONG   BrS1;       // Intel-IA64-Filler
    ULONGLONG   BrS2;       // Intel-IA64-Filler
    ULONGLONG   BrS3;       // Intel-IA64-Filler
    ULONGLONG   BrS4;       // Intel-IA64-Filler

    FLOAT128    FltS0;          // 16-byte aligned boundary       // Intel-IA64-Filler
    FLOAT128    FltS1;       // Intel-IA64-Filler
    FLOAT128    FltS2;       // Intel-IA64-Filler
    FLOAT128    FltS3;       // Intel-IA64-Filler
    FLOAT128    FltS4;       // Intel-IA64-Filler
    FLOAT128    FltS5;       // Intel-IA64-Filler
    FLOAT128    FltS6;       // Intel-IA64-Filler
    FLOAT128    FltS7;       // Intel-IA64-Filler
    FLOAT128    FltS8;       // Intel-IA64-Filler
    FLOAT128    FltS9;       // Intel-IA64-Filler
    FLOAT128    FltS10;       // Intel-IA64-Filler
    FLOAT128    FltS11;       // Intel-IA64-Filler
    FLOAT128    FltS12;       // Intel-IA64-Filler
    FLOAT128    FltS13;       // Intel-IA64-Filler
    FLOAT128    FltS14;       // Intel-IA64-Filler
    FLOAT128    FltS15;       // Intel-IA64-Filler
    FLOAT128    FltS16;       // Intel-IA64-Filler
    FLOAT128    FltS17;       // Intel-IA64-Filler
    FLOAT128    FltS18;       // Intel-IA64-Filler
    FLOAT128    FltS19;       // Intel-IA64-Filler

    ULONGLONG   A0;             // saved argument registers a0-a2       // Intel-IA64-Filler
    ULONGLONG   A1;       // Intel-IA64-Filler
    ULONGLONG   CbStk;          // saved callback stack address       // Intel-IA64-Filler
    ULONGLONG   InStack;        // saved initial stack address       // Intel-IA64-Filler
    ULONGLONG   CbBStore;       // saved callback stack address       // Intel-IA64-Filler
    ULONGLONG   InBStore;       // saved initial stack address       // Intel-IA64-Filler
    ULONGLONG   TrFrame;        // saved callback trap frame address       // Intel-IA64-Filler
    ULONGLONG   TrStIIP;        // saved continuation address       // Intel-IA64-Filler

// @@END_DDKSPLIT

} KCALLOUT_FRAME, *PKCALLOUT_FRAME;


typedef struct _UCALLOUT_FRAME {
// Please contact INTEL to get IA64-specific information
// @@BEGIN_DDKSPLIT
    PVOID Buffer;       // Intel-IA64-Filler
    ULONG Length;       // Intel-IA64-Filler
    ULONG ApiNumber;       // Intel-IA64-Filler
    ULONGLONG IntSp;       // Intel-IA64-Filler
    ULONGLONG RsPFS;       // Intel-IA64-Filler
    ULONGLONG BrRp;       // Intel-IA64-Filler
    ULONGLONG Pad;       // Intel-IA64-Filler
// @@END_DDKSPLIT
} UCALLOUT_FRAME, *PUCALLOUT_FRAME;


// end_nthal

// begin_ntddk begin_wdm
//
// Non-volatile floating point state
//

typedef struct _KFLOATING_SAVE {
    ULONG   Reserved;
} KFLOATING_SAVE, *PKFLOATING_SAVE;

// end_ntddk end_wdm

#define STATUS_IA64_INVALID_STACK     STATUS_BAD_STACK

//
// iA32 control bits definition
//
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
// Define constants to access CFLG bits
//
#define CFLG_IO 0x00000040          // IO bit map checking on
#define CFLG_IF 0x00000080          // EFLAG.if to control external interrupt
#define CFLG_II 0x00000100          // enable EFLAG.if interception
#define CFLG_NM 0x00000200          // NMI intercept

//
// CR4 bits;  These only apply to Pentium
//
#define CR4_VME 0x00000001          // V86 mode extensions
#define CR4_PVI 0x00000002          // Protected mode virtual interrupts
#define CR4_TSD 0x00000004          // Time stamp disable
#define CR4_DE  0x00000008          // Debugging Extensions
#define CR4_PSE 0x00000010          // Page size extensions
#define CR4_MCE 0x00000040          // Machine check enable

//
// Define constants to access ThNpxState
//

#define NPX_STATE_NOT_LOADED    (CR0_TS | CR0_MP)
#define NPX_STATE_LOADED        0

//
// begin_nthal begin_ntddk begin_wdm
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
//
// Machine type definitions
// BUGBUG shielint These are temporary definitions.
//

#define MACHINE_TYPE_ISA 0
#define MACHINE_TYPE_EISA 1
#define MACHINE_TYPE_MCA 2

//
// PAL Interface
//
// iA-64 defined PAL function IDs in decimal format as in the PAL spec
// All PAL calls done through HAL. HAL may block some calls
//

#define PAL_CACHE_FLUSH                                       1I64
#define PAL_CACHE_INFO                                        2I64
#define PAL_CACHE_INIT                                        3I64
#define PAL_CACHE_SUMMARY                                     4I64
#define PAL_PTCE_INFO                                         6I64
#define PAL_MEM_ATTRIB                                        5I64
#define PAL_VM_INFO                                           7I64
#define PAL_VM_SUMMARY                                        8I64
#define PAL_BUS_GET_FEATURES                                  9I64
#define PAL_BUS_SET_FEATURES                                 10I64
#define PAL_DEBUG_INFO                                       11I64
#define PAL_FIXED_ADDR                                       12I64
#define PAL_FREQ_BASE                                        13I64
#define PAL_FREQ_RATIOS                                      14I64
#define PAL_PERF_MON_INFO                                    15I64
#define PAL_PLATFORM_ADDR                                    16I64
#define PAL_PROC_GET_FEATURES                                17I64
#define PAL_PROC_SET_FEATURES                                18I64
#define PAL_RSE_INFO                                         19I64
#define PAL_VERSION                                          20I64
#define PAL_MC_CLEAR_LOG                                     21I64
#define PAL_MC_DRAIN                                         22I64
#define PAL_MC_EXPECTED                                      23I64
#define PAL_MC_DYNAMIC_STATE                                 24I64
#define PAL_MC_ERROR_INFO                                    25I64
#define PAL_MC_RESUME                                        26I64
#define PAL_MC_REGISTER_MEM                                  27I64
#define PAL_HALT                                             28I64
#define PAL_HALT_LIGHT                                       29I64
#define PAL_COPY_INFO                                        30I64
#define PAL_CACHE_LINE_INIT                                  31I64
#define PAL_PMI_ENTRYPOINT                                   32I64
#define PAL_ENTER_IA_32_ENV                                  33I64
#define PAL_VM_PAGE_SIZE                                     34I64
#define PAL_MEM_FOR_TEST                                     37I64
#define PAL_CACHE_PROT_INFO                                  38I64
#define PAL_REGISTER_INFO                                    39I64
#define PAL_SHUTDOWN                                         44I64
#define PAL_PREFETCH_VISIBILITY                              41I64

#define PAL_COPY_PAL                                        256I64
#define PAL_HALT_INFO                                       257I64
#define PAL_TEST_PROC                                       258I64
#define PAL_CACHE_READ                                      259I64
#define PAL_CACHE_WRITE                                     260I64
#define PAL_VM_TR_READ                                      261I64

//
// iA-64 defined PAL return values
//

#define PAL_STATUS_INVALID_CACHELINE                          1I64
#define PAL_STATUS_SUPPORT_NOT_NEEDED                         1I64
#define PAL_STATUS_SUCCESS                                    0
#define PAL_STATUS_NOT_IMPLEMENTED                           -1I64
#define PAL_STATUS_INVALID_ARGUMENT                          -2I64
#define PAL_STATUS_ERROR                                     -3I64
#define PAL_STATUS_UNABLE_TO_INIT_CACHE_LEVEL_AND_TYPE       -4I64
#define PAL_STATUS_NOT_FOUND_IN_CACHE                        -5I64
#define PAL_STATUS_NO_ERROR_INFO_AVAILABLE                   -6I64


// end_nthal
               
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

#define TYPE_TSS    0x01  // 01001 = NonBusy TSS
#define TYPE_LDT    0x02  // 00010 = LDT

//
// UnScrambled Descriptor format
//
typedef struct _KDESCRIPTOR_UNSCRAM {
    union {
        ULONGLONG  DescriptorWords;
        struct {
            ULONGLONG   Base : 32;
            ULONGLONG   Limit : 20;
            ULONGLONG   Type : 5;
            ULONGLONG   Dpl : 2;
            ULONGLONG   Pres : 1;
            ULONGLONG   Sys : 1;
            ULONGLONG   Reserved_0 : 1;
            ULONGLONG   Default_Big : 1;
            ULONGLONG   Granularity : 1;
         } Bits;
    } Words;
} KXDESCRIPTOR, *PKXDESCRIPTOR;

#define TYPE_CODE_USER                0x1A // 0x11011 = Code, Readable, Accessed
#define TYPE_DATA_USER                0x13 // 0x10011 = Data, ReadWrite, Accessed

#define DESCRIPTOR_EXPAND_DOWN        0x14
#define DESCRIPTOR_DATA_READWRITE     (0x8|0x2) // Data, Read/Write

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
// User mode, then
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
        ((EFLAGS_INTERRUPT_MASK) | ((eFlags) & EFLAGS_USER_SANITIZE))))

//
// Definitions that used by CSD and SSD
//
#define USER_CODE_DESCRIPTOR  0xCFBFFFFF00000000i64
#define USER_DATA_DESCRIPTOR  0xCF3FFFFF00000000i64

//
// Macros for Emulx86.c and VDM files
//
//
// Prefix Flags
//
// Copied from .../ntos/vdm/i386/vdm.inc
// The bottom byte originally corresponded to the number of prefixes seen
// which is effectively the length of the instruction...
//
#define PREFIX_ES               0x00000100
#define PREFIX_CS               0x00000200
#define PREFIX_SS               0x00000400
#define PREFIX_DS               0x00000800
#define PREFIX_FS               0x00001000
#define PREFIX_GS               0x00002000
#define PREFIX_OPER32           0x00004000
#define PREFIX_ADDR32           0x00008000
#define PREFIX_LOCK             0x00010000
#define PREFIX_REPNE            0x00020000
#define PREFIX_REP              0x00040000
#define PREFIX_SEG_ALL          0x00003f00


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

// begin_windbgkd

#ifdef _IA64_

// begin_nthal

//
// Stack Registers for IA64
//

typedef struct _STACK_REGISTERS {

// Please contact INTEL to get IA64-specific information
// @@BEGIN_DDKSPLIT

    ULONGLONG IntR32;       // Intel-IA64-Filler
    ULONGLONG IntR33;       // Intel-IA64-Filler
    ULONGLONG IntR34;       // Intel-IA64-Filler
    ULONGLONG IntR35;       // Intel-IA64-Filler
    ULONGLONG IntR36;       // Intel-IA64-Filler
    ULONGLONG IntR37;       // Intel-IA64-Filler
    ULONGLONG IntR38;       // Intel-IA64-Filler
    ULONGLONG IntR39;       // Intel-IA64-Filler

    ULONGLONG IntR40;       // Intel-IA64-Filler
    ULONGLONG IntR41;       // Intel-IA64-Filler
    ULONGLONG IntR42;       // Intel-IA64-Filler
    ULONGLONG IntR43;       // Intel-IA64-Filler
    ULONGLONG IntR44;       // Intel-IA64-Filler
    ULONGLONG IntR45;       // Intel-IA64-Filler
    ULONGLONG IntR46;       // Intel-IA64-Filler
    ULONGLONG IntR47;       // Intel-IA64-Filler
    ULONGLONG IntR48;       // Intel-IA64-Filler
    ULONGLONG IntR49;       // Intel-IA64-Filler

    ULONGLONG IntR50;       // Intel-IA64-Filler
    ULONGLONG IntR51;       // Intel-IA64-Filler
    ULONGLONG IntR52;       // Intel-IA64-Filler
    ULONGLONG IntR53;       // Intel-IA64-Filler
    ULONGLONG IntR54;       // Intel-IA64-Filler
    ULONGLONG IntR55;       // Intel-IA64-Filler
    ULONGLONG IntR56;       // Intel-IA64-Filler
    ULONGLONG IntR57;       // Intel-IA64-Filler
    ULONGLONG IntR58;       // Intel-IA64-Filler
    ULONGLONG IntR59;       // Intel-IA64-Filler

    ULONGLONG IntR60;       // Intel-IA64-Filler
    ULONGLONG IntR61;       // Intel-IA64-Filler
    ULONGLONG IntR62;       // Intel-IA64-Filler
    ULONGLONG IntR63;       // Intel-IA64-Filler
    ULONGLONG IntR64;       // Intel-IA64-Filler
    ULONGLONG IntR65;       // Intel-IA64-Filler
    ULONGLONG IntR66;       // Intel-IA64-Filler
    ULONGLONG IntR67;       // Intel-IA64-Filler
    ULONGLONG IntR68;       // Intel-IA64-Filler
    ULONGLONG IntR69;       // Intel-IA64-Filler

    ULONGLONG IntR70;       // Intel-IA64-Filler
    ULONGLONG IntR71;       // Intel-IA64-Filler
    ULONGLONG IntR72;       // Intel-IA64-Filler
    ULONGLONG IntR73;       // Intel-IA64-Filler
    ULONGLONG IntR74;       // Intel-IA64-Filler
    ULONGLONG IntR75;       // Intel-IA64-Filler
    ULONGLONG IntR76;       // Intel-IA64-Filler
    ULONGLONG IntR77;       // Intel-IA64-Filler
    ULONGLONG IntR78;       // Intel-IA64-Filler
    ULONGLONG IntR79;       // Intel-IA64-Filler

    ULONGLONG IntR80;       // Intel-IA64-Filler
    ULONGLONG IntR81;       // Intel-IA64-Filler
    ULONGLONG IntR82;       // Intel-IA64-Filler
    ULONGLONG IntR83;       // Intel-IA64-Filler
    ULONGLONG IntR84;       // Intel-IA64-Filler
    ULONGLONG IntR85;       // Intel-IA64-Filler
    ULONGLONG IntR86;       // Intel-IA64-Filler
    ULONGLONG IntR87;       // Intel-IA64-Filler
    ULONGLONG IntR88;       // Intel-IA64-Filler
    ULONGLONG IntR89;       // Intel-IA64-Filler

    ULONGLONG IntR90;       // Intel-IA64-Filler
    ULONGLONG IntR91;       // Intel-IA64-Filler
    ULONGLONG IntR92;       // Intel-IA64-Filler
    ULONGLONG IntR93;       // Intel-IA64-Filler
    ULONGLONG IntR94;       // Intel-IA64-Filler
    ULONGLONG IntR95;       // Intel-IA64-Filler
    ULONGLONG IntR96;       // Intel-IA64-Filler
    ULONGLONG IntR97;       // Intel-IA64-Filler
    ULONGLONG IntR98;       // Intel-IA64-Filler
    ULONGLONG IntR99;       // Intel-IA64-Filler

    ULONGLONG IntR100;       // Intel-IA64-Filler
    ULONGLONG IntR101;       // Intel-IA64-Filler
    ULONGLONG IntR102;       // Intel-IA64-Filler
    ULONGLONG IntR103;       // Intel-IA64-Filler
    ULONGLONG IntR104;       // Intel-IA64-Filler
    ULONGLONG IntR105;       // Intel-IA64-Filler
    ULONGLONG IntR106;       // Intel-IA64-Filler
    ULONGLONG IntR107;       // Intel-IA64-Filler
    ULONGLONG IntR108;       // Intel-IA64-Filler
    ULONGLONG IntR109;       // Intel-IA64-Filler

    ULONGLONG IntR110;       // Intel-IA64-Filler
    ULONGLONG IntR111;       // Intel-IA64-Filler
    ULONGLONG IntR112;       // Intel-IA64-Filler
    ULONGLONG IntR113;       // Intel-IA64-Filler
    ULONGLONG IntR114;       // Intel-IA64-Filler
    ULONGLONG IntR115;       // Intel-IA64-Filler
    ULONGLONG IntR116;       // Intel-IA64-Filler
    ULONGLONG IntR117;       // Intel-IA64-Filler
    ULONGLONG IntR118;       // Intel-IA64-Filler
    ULONGLONG IntR119;       // Intel-IA64-Filler

    ULONGLONG IntR120;       // Intel-IA64-Filler
    ULONGLONG IntR121;       // Intel-IA64-Filler
    ULONGLONG IntR122;       // Intel-IA64-Filler
    ULONGLONG IntR123;       // Intel-IA64-Filler
    ULONGLONG IntR124;       // Intel-IA64-Filler
    ULONGLONG IntR125;       // Intel-IA64-Filler
    ULONGLONG IntR126;       // Intel-IA64-Filler
    ULONGLONG IntR127;       // Intel-IA64-Filler
                                 // Nat bits for stack registers       // Intel-IA64-Filler
    ULONGLONG IntNats2;          // r32-r95 in bit positions 1 to 63       // Intel-IA64-Filler
    ULONGLONG IntNats3;          // r96-r127 in bit position 1 to 31       // Intel-IA64-Filler

// @@END_DDKSPLIT

} STACK_REGISTERS, *PSTACK_REGISTERS;


// Please contact INTEL to get IA64-specific information
// @@BEGIN_DDKSPLIT

//
// Special Registers for IA64  // Intel-IA64-Filler
//

typedef struct _KSPECIAL_REGISTERS {  // Intel-IA64-Filler

    // Kernel debug breakpoint registers       // Intel-IA64-Filler

    ULONGLONG KernelDbI0;         // Instruction debug registers       // Intel-IA64-Filler
    ULONGLONG KernelDbI1;       // Intel-IA64-Filler
    ULONGLONG KernelDbI2;       // Intel-IA64-Filler
    ULONGLONG KernelDbI3;       // Intel-IA64-Filler
    ULONGLONG KernelDbI4;       // Intel-IA64-Filler
    ULONGLONG KernelDbI5;       // Intel-IA64-Filler
    ULONGLONG KernelDbI6;       // Intel-IA64-Filler
    ULONGLONG KernelDbI7;       // Intel-IA64-Filler

    ULONGLONG KernelDbD0;         // Data debug registers       // Intel-IA64-Filler
    ULONGLONG KernelDbD1;       // Intel-IA64-Filler
    ULONGLONG KernelDbD2;       // Intel-IA64-Filler
    ULONGLONG KernelDbD3;       // Intel-IA64-Filler
    ULONGLONG KernelDbD4;       // Intel-IA64-Filler
    ULONGLONG KernelDbD5;       // Intel-IA64-Filler
    ULONGLONG KernelDbD6;       // Intel-IA64-Filler
    ULONGLONG KernelDbD7;       // Intel-IA64-Filler

    // Kernel performance monitor registers       // Intel-IA64-Filler

    ULONGLONG KernelPfC0;         // Performance configuration registers       // Intel-IA64-Filler
    ULONGLONG KernelPfC1;       // Intel-IA64-Filler
    ULONGLONG KernelPfC2;       // Intel-IA64-Filler
    ULONGLONG KernelPfC3;       // Intel-IA64-Filler
    ULONGLONG KernelPfC4;       // Intel-IA64-Filler
    ULONGLONG KernelPfC5;       // Intel-IA64-Filler
    ULONGLONG KernelPfC6;       // Intel-IA64-Filler
    ULONGLONG KernelPfC7;       // Intel-IA64-Filler

    ULONGLONG KernelPfD0;         // Performance data registers       // Intel-IA64-Filler
    ULONGLONG KernelPfD1;       // Intel-IA64-Filler
    ULONGLONG KernelPfD2;       // Intel-IA64-Filler
    ULONGLONG KernelPfD3;       // Intel-IA64-Filler
    ULONGLONG KernelPfD4;       // Intel-IA64-Filler
    ULONGLONG KernelPfD5;       // Intel-IA64-Filler
    ULONGLONG KernelPfD6;       // Intel-IA64-Filler
    ULONGLONG KernelPfD7;       // Intel-IA64-Filler

    // kernel bank shadow (hidden) registers       // Intel-IA64-Filler

    ULONGLONG IntH16;       // Intel-IA64-Filler
    ULONGLONG IntH17;       // Intel-IA64-Filler
    ULONGLONG IntH18;       // Intel-IA64-Filler
    ULONGLONG IntH19;       // Intel-IA64-Filler
    ULONGLONG IntH20;       // Intel-IA64-Filler
    ULONGLONG IntH21;       // Intel-IA64-Filler
    ULONGLONG IntH22;       // Intel-IA64-Filler
    ULONGLONG IntH23;       // Intel-IA64-Filler
    ULONGLONG IntH24;       // Intel-IA64-Filler
    ULONGLONG IntH25;       // Intel-IA64-Filler
    ULONGLONG IntH26;       // Intel-IA64-Filler
    ULONGLONG IntH27;       // Intel-IA64-Filler
    ULONGLONG IntH28;       // Intel-IA64-Filler
    ULONGLONG IntH29;       // Intel-IA64-Filler
    ULONGLONG IntH30;       // Intel-IA64-Filler
    ULONGLONG IntH31;       // Intel-IA64-Filler

    // Application Registers       // Intel-IA64-Filler

    //       - CPUID Registers - AR       // Intel-IA64-Filler
    ULONGLONG ApCPUID0; // Cpuid Register 0       // Intel-IA64-Filler
    ULONGLONG ApCPUID1; // Cpuid Register 1       // Intel-IA64-Filler
    ULONGLONG ApCPUID2; // Cpuid Register 2       // Intel-IA64-Filler
    ULONGLONG ApCPUID3; // Cpuid Register 3       // Intel-IA64-Filler
    ULONGLONG ApCPUID4; // Cpuid Register 4       // Intel-IA64-Filler
    ULONGLONG ApCPUID5; // Cpuid Register 5       // Intel-IA64-Filler
    ULONGLONG ApCPUID6; // Cpuid Register 6       // Intel-IA64-Filler
    ULONGLONG ApCPUID7; // Cpuid Register 7       // Intel-IA64-Filler

    //       - Kernel Registers - AR       // Intel-IA64-Filler
    ULONGLONG ApKR0;    // Kernel Register 0 (User RO)       // Intel-IA64-Filler
    ULONGLONG ApKR1;    // Kernel Register 1 (User RO)       // Intel-IA64-Filler
    ULONGLONG ApKR2;    // Kernel Register 2 (User RO)       // Intel-IA64-Filler
    ULONGLONG ApKR3;    // Kernel Register 3 (User RO)       // Intel-IA64-Filler
    ULONGLONG ApKR4;    // Kernel Register 4       // Intel-IA64-Filler
    ULONGLONG ApKR5;    // Kernel Register 5       // Intel-IA64-Filler
    ULONGLONG ApKR6;    // Kernel Register 6       // Intel-IA64-Filler
    ULONGLONG ApKR7;    // Kernel Register 7       // Intel-IA64-Filler

    ULONGLONG ApITC;    // Interval Timer Counter       // Intel-IA64-Filler

    // Global control registers       // Intel-IA64-Filler

    ULONGLONG ApITM;    // Interval Timer Match register       // Intel-IA64-Filler
    ULONGLONG ApIVA;    // Interrupt Vector Address       // Intel-IA64-Filler
    ULONGLONG ApPTA;    // Page Table Address       // Intel-IA64-Filler
    ULONGLONG ApGPTA;   // ia32 Page Table Address       // Intel-IA64-Filler

    ULONGLONG StISR;    // Interrupt status       // Intel-IA64-Filler
    ULONGLONG StIFA;    // Interruption Faulting Address       // Intel-IA64-Filler
    ULONGLONG StITIR;   // Interruption TLB Insertion Register       // Intel-IA64-Filler
    ULONGLONG StIIPA;   // Interruption Instruction Previous Address (RO)       // Intel-IA64-Filler
    ULONGLONG StIIM;    // Interruption Immediate register (RO)       // Intel-IA64-Filler
    ULONGLONG StIHA;    // Interruption Hash Address (RO)       // Intel-IA64-Filler

    //       - External Interrupt control registers (SAPIC)       // Intel-IA64-Filler
    ULONGLONG SaLID;    // Local SAPIC ID       // Intel-IA64-Filler
    ULONGLONG SaIVR;    // Interrupt Vector Register (RO)       // Intel-IA64-Filler
    ULONGLONG SaTPR;    // Task Priority Register       // Intel-IA64-Filler
    ULONGLONG SaEOI;    // End Of Interrupt       // Intel-IA64-Filler
    ULONGLONG SaIRR0;   // Interrupt Request Register 0 (RO)       // Intel-IA64-Filler
    ULONGLONG SaIRR1;   // Interrupt Request Register 1 (RO)       // Intel-IA64-Filler
    ULONGLONG SaIRR2;   // Interrupt Request Register 2 (RO)       // Intel-IA64-Filler
    ULONGLONG SaIRR3;   // Interrupt Request Register 3 (RO)       // Intel-IA64-Filler
    ULONGLONG SaITV;    // Interrupt Timer Vector       // Intel-IA64-Filler
    ULONGLONG SaPMV;    // Performance Monitor Vector       // Intel-IA64-Filler
    ULONGLONG SaCMCV;   // Corrected Machine Check Vector       // Intel-IA64-Filler
    ULONGLONG SaLRR0;   // Local Interrupt Redirection Vector 0       // Intel-IA64-Filler
    ULONGLONG SaLRR1;   // Local Interrupt Redirection Vector 1       // Intel-IA64-Filler

    // System Registers       // Intel-IA64-Filler
    //       - Region registers       // Intel-IA64-Filler
    ULONGLONG Rr0;  // Region register 0       // Intel-IA64-Filler
    ULONGLONG Rr1;  // Region register 1       // Intel-IA64-Filler
    ULONGLONG Rr2;  // Region register 2       // Intel-IA64-Filler
    ULONGLONG Rr3;  // Region register 3       // Intel-IA64-Filler
    ULONGLONG Rr4;  // Region register 4       // Intel-IA64-Filler
    ULONGLONG Rr5;  // Region register 5       // Intel-IA64-Filler
    ULONGLONG Rr6;  // Region register 6       // Intel-IA64-Filler
    ULONGLONG Rr7;  // Region register 7       // Intel-IA64-Filler

    //      - Protection Key registers  // Intel-IA64-Filler
    ULONGLONG Pkr0;     // Protection Key register 0  // Intel-IA64-Filler
    ULONGLONG Pkr1;     // Protection Key register 1  // Intel-IA64-Filler
    ULONGLONG Pkr2;     // Protection Key register 2  // Intel-IA64-Filler
    ULONGLONG Pkr3;     // Protection Key register 3  // Intel-IA64-Filler
    ULONGLONG Pkr4;     // Protection Key register 4  // Intel-IA64-Filler
    ULONGLONG Pkr5;     // Protection Key register 5  // Intel-IA64-Filler
    ULONGLONG Pkr6;     // Protection Key register 6  // Intel-IA64-Filler
    ULONGLONG Pkr7;     // Protection Key register 7  // Intel-IA64-Filler
    ULONGLONG Pkr8;     // Protection Key register 8  // Intel-IA64-Filler
    ULONGLONG Pkr9;     // Protection Key register 9  // Intel-IA64-Filler
    ULONGLONG Pkr10;    // Protection Key register 10  // Intel-IA64-Filler
    ULONGLONG Pkr11;    // Protection Key register 11  // Intel-IA64-Filler
    ULONGLONG Pkr12;    // Protection Key register 12  // Intel-IA64-Filler
    ULONGLONG Pkr13;    // Protection Key register 13  // Intel-IA64-Filler
    ULONGLONG Pkr14;    // Protection Key register 14  // Intel-IA64-Filler
    ULONGLONG Pkr15;    // Protection Key register 15  // Intel-IA64-Filler

    //      -  Translation Lookaside buffers  // Intel-IA64-Filler
    ULONGLONG TrI0;     // Instruction Translation Register 0  // Intel-IA64-Filler
    ULONGLONG TrI1;     // Instruction Translation Register 1  // Intel-IA64-Filler
    ULONGLONG TrI2;     // Instruction Translation Register 2  // Intel-IA64-Filler
    ULONGLONG TrI3;     // Instruction Translation Register 3  // Intel-IA64-Filler
    ULONGLONG TrI4;     // Instruction Translation Register 4  // Intel-IA64-Filler
    ULONGLONG TrI5;     // Instruction Translation Register 5  // Intel-IA64-Filler
    ULONGLONG TrI6;     // Instruction Translation Register 6  // Intel-IA64-Filler
    ULONGLONG TrI7;     // Instruction Translation Register 7  // Intel-IA64-Filler

    ULONGLONG TrD0;     // Data Translation Register 0  // Intel-IA64-Filler
    ULONGLONG TrD1;     // Data Translation Register 1  // Intel-IA64-Filler
    ULONGLONG TrD2;     // Data Translation Register 2  // Intel-IA64-Filler
    ULONGLONG TrD3;     // Data Translation Register 3  // Intel-IA64-Filler
    ULONGLONG TrD4;     // Data Translation Register 4  // Intel-IA64-Filler
    ULONGLONG TrD5;     // Data Translation Register 5  // Intel-IA64-Filler
    ULONGLONG TrD6;     // Data Translation Register 6  // Intel-IA64-Filler
    ULONGLONG TrD7;     // Data Translation Register 7  // Intel-IA64-Filler

    //      -  Machine Specific Registers  // Intel-IA64-Filler
    ULONGLONG SrMSR0;   // Machine Specific Register 0  // Intel-IA64-Filler
    ULONGLONG SrMSR1;   // Machine Specific Register 1  // Intel-IA64-Filler
    ULONGLONG SrMSR2;   // Machine Specific Register 2  // Intel-IA64-Filler
    ULONGLONG SrMSR3;   // Machine Specific Register 3  // Intel-IA64-Filler
    ULONGLONG SrMSR4;   // Machine Specific Register 4  // Intel-IA64-Filler
    ULONGLONG SrMSR5;   // Machine Specific Register 5  // Intel-IA64-Filler
    ULONGLONG SrMSR6;   // Machine Specific Register 6  // Intel-IA64-Filler
    ULONGLONG SrMSR7;   // Machine Specific Register 7  // Intel-IA64-Filler

} KSPECIAL_REGISTERS, *PKSPECIAL_REGISTERS;  // Intel-IA64-Filler


//
// Processor State structure.
//

typedef struct _KPROCESSOR_STATE {  // Intel-IA64-Filler
    struct _CONTEXT ContextFrame;  // Intel-IA64-Filler
    struct _KSPECIAL_REGISTERS SpecialRegisters;  // Intel-IA64-Filler
} KPROCESSOR_STATE, *PKPROCESSOR_STATE;  // Intel-IA64-Filler

// @@END_DDKSPLIT

// end_nthal

#endif // _IA64_

// end_windbgkd

// begin_nthal begin_ntddk

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
    ULONG_PTR PcrPage;
    ULONG Spares1[4];

//
// Space reserved for the system.
//

    ULONGLONG SystemReserved[8];

//
// Space reserved for the HAL.
//

    ULONGLONG HalReserved[16];

//
// End of the architecturally defined section of the PRCB.
// end_nthal end_ntddk
//

    ULONG DpcTime;
    ULONG InterruptTime;
    ULONG KernelTime;
    ULONG UserTime;
    ULONG InterruptCount;
    ULONG DispatchInterruptCount;
    ULONG ApcBypassCount;
    ULONG DpcBypassCount;
    ULONG Spare0[4];

//
// MP information.
//

    PVOID Spare1;
    PVOID Spare2;
    PVOID Spare3;
    volatile ULONG IpiFrozen;
    struct _KPROCESSOR_STATE ProcessorState;

//
//  Per-processor data for various hot code which resides in the
//  kernel image. Each processor is given it's own copy of the data
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
// MP interprocessor request packet and summary.
//
// N.B. This is carefully aligned to be on a cache line boundary.
//

    volatile PVOID CurrentPacket[3];
    volatile KAFFINITY TargetSet;
    volatile PKIPI_WORKER WorkerRoutine;
    ULONGLONG CachePad1[11];

//
// N.B. These two longwords must be on a quadword boundary and adjacent.
//

    volatile ULONG RequestSummary;
    volatile struct _KPRCB *SignalDone;

//
// Spare counters.
//

    ULONGLONG Spare4[14];

//
// DPC interrupt requested.
//

    ULONG DpcInterruptRequested;
    ULONGLONG Spare5[15];
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
    ULONG DpcLastCount;
    ULONG QuantumEnd;
    ULONG DpcRoutineActive;
    ULONG DpcQueueDepth;
    BOOLEAN SkipTick;

//
// Address of MP interprocessor operation counters.
//

    PKIPI_COUNTS IpiCounts;

//
// Processors power state
//
    PROCESSOR_POWER_STATE PowerState;

// begin_nthal begin_ntddk
} KPRCB, *PKPRCB, *RESTRICTED_POINTER PRKPRCB;

// begin_ntndis

//
// Define Processor Control Region Structure.
//

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
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//

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
// Processor identification from PrId register.
//

    ULONG ProcessorId;

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

    ULONG InterruptionCount;

//
// Space reserved for the system.
//

    ULONGLONG   SystemReserved[6];

//
// Space reserved for the HAL
//

    ULONGLONG   HalReserved[64];

//
// IRQL mapping tables.
//

    UCHAR IrqlMask[64];
    UCHAR IrqlTable[64];

//
// External Interrupt vectors.
//

    PKINTERRUPT_ROUTINE InterruptRoutine[MAXIMUM_VECTOR];

//
// Reserved interrupt vector mask.
//

    ULONG ReservedVectors;

//
// Processor affinity mask.
//

    KAFFINITY SetMember;

//
// Complement of the processor affinity mask.
//

    KAFFINITY NotMember;

//
// Pointer to processor control block.
//

    struct _KPRCB *Prcb;

//
//  Shadow copy of Prcb->CurrentThread for fast access
//

    struct _KTHREAD *CurrentThread;

//
// Processor number.
//

    CCHAR Number;                        // Processor Number
    UCHAR DebugActive;                   // debug register active in user flag
    UCHAR KernelDebugActive;             // debug register active in kernel flag
    UCHAR CurrentIrql;                   // Current IRQL
    union {
        USHORT SoftwareInterruptPending; // Software Interrupt Pending Flag
        struct {
            UCHAR ApcInterrupt;          // 0x01 if APC int pending
            UCHAR DispatchInterrupt;     // 0x01 if dispatch int pending
        };
    };

//
// End of the architecturally defined section of the PCR. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//

// end_nthal end_ntddk

//
// OS Part
//

// Please contact INTEL to get IA64-specific information
// @@BEGIN_DDKSPLIT
//  Per processor kernel (ntoskrnl.exe) global pointer - gp (swizzled) // Intel-IA64-Filler
    ULONGLONG   KernelGP; // Intel-IA64-Filler
//  Per processor initial kernel stack for current thread (swizzled) // Intel-IA64-Filler
    ULONGLONG   InitialStack; // Intel-IA64-Filler
//  Per processor pointer to kernel BSP (swizzled) // Intel-IA64-Filler
    ULONGLONG   InitialBStore; // Intel-IA64-Filler
//  Per processor kernel stack limit (swizzled) // Intel-IA64-Filler
    ULONGLONG   StackLimit; // Intel-IA64-Filler
//  Per processor kernel backing store limit (swizzled) // Intel-IA64-Filler
    ULONGLONG   BStoreLimit; // Intel-IA64-Filler
//  Per processor panic kernel stack (swizzled) // Intel-IA64-Filler
    ULONGLONG   PanicStack; // Intel-IA64-Filler

//
//  Save area for kernel entry/exit
//
    ULONGLONG   SavedIIM; // Intel-IA64-Filler
    ULONGLONG   SavedIFA; // Intel-IA64-Filler

    ULONGLONG   ForwardProgressBuffer[16]; // Intel-IA64-Filler
// @@END_DDKSPLIT

// begin_nthal begin_ntddk

} KPCR, *PKPCR;

// end_nthal end_ntddk

// begin_nthal

// Please contact INTEL to get IA64-specific information
// @@BEGIN_DDKSPLIT
//
// Define the number of bits to shift to right justify the Page Table Index
// field of a PTE.
//

#define PTI_SHIFT PAGE_SHIFT // Intel-IA64-Filler

//
// Define the number of bits to shift to right justify the Page Directory Index
// field of a PTE.
//

#define PDI_SHIFT (PTI_SHIFT + PAGE_SHIFT - PTE_SHIFT)  // Intel-IA64-Filler
#define PDI1_SHIFT (PDI_SHIFT + PAGE_SHIFT - PTE_SHIFT) // Intel-IA64-Filler
#define PDI_MASK ((1 << (PAGE_SHIFT - PTE_SHIFT)) - 1)  // Intel-IA64-Filler

//
// Define the number of bits to shift to left to produce page table offset
// from page table index.
//

#define PTE_SHIFT 3 // Intel-IA64-Filler

//
// Define the number of bits to shift to the right justify the Page Directory
// Table Entry field.
//

#define VHPT_PDE_BITS 40 // Intel-IA64-Filler

//
// Define the RID for IO Port Space. // Intel-IA64-Filler
//

#define RR_IO_PORT 6 // Intel-IA64-Filler

// @@END_DDKSPLIT


// begin_ntddk
//
// The highest user address reserves 64K bytes for a guard page. This
// the probing of address from kernel mode to only have to check the
// starting address for structures of 64k bytes or less.
//

#define MM_HIGHEST_USER_ADDRESS (PVOID) (ULONG_PTR)((UADDRESS_BASE + 0x3FFFFFEFFFF)) // highest user address
#define MM_USER_PROBE_ADDRESS ((ULONG_PTR)(UADDRESS_BASE + 0x3FFFFFF0000UI64)) // starting address of guard page
#define MM_SYSTEM_RANGE_START (PVOID) (KSEG0_BASE) // start of system space

//
// The following definitions are required for the debugger data block.
//

extern PVOID MmHighestUserAddress;
extern PVOID MmSystemRangeStart;
extern ULONG_PTR MmUserProbeAddress;

//
// The lowest user address reserves the low 64k.
//

#define MM_LOWEST_USER_ADDRESS  (PVOID)((ULONG_PTR)(UADDRESS_BASE+0x00010000))

// begin_wdm

#define MmGetProcedureAddress(Address) (Address)
#define MmLockPagableCodeSection(PLabelAddress) \
    MmLockPagableDataSection((PVOID)(*((PULONGLONG)PLabelAddress)))

// end_ntddk end_wdm
//
// Define the page table base and the page directory base for
// the TB miss routines and memory management.
//

#define VA_SIGN    0x0002000000000000UI64    // MSB of implemented virtual address
#define VA_FILL    0x1FFC000000000000UI64    // singed fill for unimplemented virtual address
#define VRN_MASK   0xE000000000000000UI64    // Virtual Region Number mask
#define PTA_BASE0  0x1FFC000000000000UI64    // Page Table Address BASE 0
#define PTA_SIGN   (VA_SIGN >> (PAGE_SHIFT - PTE_SHIFT)) // MSB of VPN offset
#define PTA_FILL   (VA_FILL >> (PAGE_SHIFT - PTE_SHIFT)) // signed fill for PTA base0
#define PTA_BASE   (PTA_BASE0|PTA_FILL)      // PTA_BASE address

//
// user/kernel page table base and top addresses
//

#define SADDRESS_BASE 0x2000000000000000UI64  // session base address

#define PTE_UBASE  (UADDRESS_BASE|PTA_BASE)
#define PTE_KBASE  (KADDRESS_BASE|PTA_BASE)
#define PTE_SBASE  (SADDRESS_BASE|PTA_BASE)

#define PTE_UTOP (PTE_UBASE|(((ULONG_PTR)1 << PDI1_SHIFT) - 1)) // top level PDR address (user)
#define PTE_KTOP (PTE_KBASE|(((ULONG_PTR)1 << PDI1_SHIFT) - 1)) // top level PDR address (kernel)
#define PTE_STOP (PTE_SBASE|(((ULONG_PTR)1 << PDI1_SHIFT) - 1)) // top level PDR address (session)

//
// Second level user and kernel PDR address
//

#define PDE_UBASE  (PTE_UBASE|(PTE_UBASE>>(PTI_SHIFT-PTE_SHIFT)))
#define PDE_KBASE  (PTE_KBASE|(PTE_KBASE>>(PTI_SHIFT-PTE_SHIFT)))
#define PDE_SBASE  (PTE_SBASE|(PTE_SBASE>>(PTI_SHIFT-PTE_SHIFT)))

#define PDE_UTOP (PDE_UBASE|(((ULONG_PTR)1 << PDI_SHIFT) - 1)) // second level PDR address (user)
#define PDE_KTOP (PDE_KBASE|(((ULONG_PTR)1 << PDI_SHIFT) - 1)) // second level PDR address (kernel)
#define PDE_STOP (PDE_SBASE|(((ULONG_PTR)1 << PDI_SHIFT) - 1)) // second level PDR address (session)

//
// 8KB first level user and kernel PDR address
//

#define PDE_UTBASE (PTE_UBASE|(PDE_UBASE>>(PTI_SHIFT-PTE_SHIFT)))
#define PDE_KTBASE (PTE_KBASE|(PDE_KBASE>>(PTI_SHIFT-PTE_SHIFT)))
#define PDE_STBASE (PTE_SBASE|(PDE_SBASE>>(PTI_SHIFT-PTE_SHIFT)))

#define PDE_USELFMAP (PDE_UTBASE|(PAGE_SIZE - (1<<PTE_SHIFT))) // self mapped PPE address (user)
#define PDE_KSELFMAP (PDE_KTBASE|(PAGE_SIZE - (1<<PTE_SHIFT))) // self mapped PPE address (kernel)
#define PDE_SSELFMAP (PDE_STBASE|(PAGE_SIZE - (1<<PTE_SHIFT))) // self mapped PPE address (kernel)

#define PTE_BASE    PTE_UBASE
#define PDE_BASE    PDE_UBASE
#define PDE_TBASE   PDE_UTBASE
#define PDE_SELFMAP PDE_USELFMAP

#define KSEG3_BASE 0x8000000000000000UI64
#define KSEG3_LIMIT 0x8000100000000000UI64

//
//++
//PVOID
//KSEG_ADDRESS (
//    IN ULONG PAGE
//    );
//
// Routine Description:
//
//    This macro returns a KSEG virtual address which maps the page.
//
// Arguments:
//
//    PAGE - Supplies the physical page frame number
//
// Return Value:
//
//    The address of the KSEG address
//
//--

#define KSEG_ADDRESS(PAGE) ((PVOID)(KSEG3_BASE | ((ULONG_PTR)(PAGE) << PAGE_SHIFT)))

#define MAXIMUM_FWP_BUFFER_ENTRY 8

typedef struct _REGION_MAP_INFO {
    ULONG RegionId;
    ULONG PageSize;
    ULONGLONG SequenceNumber;
} REGION_MAP_INFO, *PREGION_MAP_INFO;

// begin_ntddk begin_wdm
//
// The lowest address for system space.
//

#define MM_LOWEST_SYSTEM_ADDRESS ((PVOID)((ULONG_PTR)(KADDRESS_BASE + 0xC0C00000)))
// end_nthal end_ntddk end_wdm

#define SYSTEM_BASE (KADDRESS_BASE + 0xC3000000)          // start of system space (no typecast)

//
// Define macro to initialize directory table base.
//

// Please contact INTEL to get IA64-specific information
// @@BEGIN_DDKSPLIT
#define INITIALIZE_DIRECTORY_TABLE_BASE(dirbase, pfn)  /* Intel-IA64-Filler */ \
    *((PULONGLONG)(dirbase)) = 0;                      /* Intel-IA64-Filler */ \
    ((PHARDWARE_PTE)(dirbase))->PageFrameNumber = pfn; /* Intel-IA64-Filler */ \
    ((PHARDWARE_PTE)(dirbase))->Accessed = 1;          /* Intel-IA64-Filler */ \
    ((PHARDWARE_PTE)(dirbase))->Dirty = 1;             /* Intel-IA64-Filler */ \
    ((PHARDWARE_PTE)(dirbase))->Cache = 0;             /* Intel-IA64-Filler */ \
    ((PHARDWARE_PTE)(dirbase))->Write = 1;             /* Intel-IA64-Filler */ \
    ((PHARDWARE_PTE)(dirbase))->Valid = 1;             // Intel-IA64-Filler
// @@END_DDKSPLIT


//
// IA64 function definitions
//

//++
//
// BOOLEAN
// KiIsThreadNumericStateSaved(
//     IN PKTHREAD Address
//     )
//
//  This call is used on a not running thread to see if it's numeric
//  state has been saved in it's context information.  On IA64 the
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
// N.B. All x86 features test TRUE on IA64 systems.
//

#define Isx86FeaturePresent(_f_) TRUE


// begin_nthal begin_ntddk begin_ntndis begin_wdm
#endif // defined(_IA64_)
// end_nthal end_ntddk end_ntndis end_wdm

#endif // _IA64H_

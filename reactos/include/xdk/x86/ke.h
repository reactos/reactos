$if (_WDMDDK_)
/** Kernel definitions for x86 **/

/* Interrupt request levels */
#define PASSIVE_LEVEL           0
#define LOW_LEVEL               0
#define APC_LEVEL               1
#define DISPATCH_LEVEL          2
#define CMCI_LEVEL              5
#define PROFILE_LEVEL           27
#define CLOCK1_LEVEL            28
#define CLOCK2_LEVEL            28
#define IPI_LEVEL               29
#define POWER_LEVEL             30
#define HIGH_LEVEL              31
#define CLOCK_LEVEL             CLOCK2_LEVEL

#define KIP0PCRADDRESS          0xffdff000
#define KI_USER_SHARED_DATA     0xffdf0000
#define SharedUserData          ((KUSER_SHARED_DATA * CONST)KI_USER_SHARED_DATA)

#define PAGE_SIZE               0x1000
#define PAGE_SHIFT              12L
#define KeGetDcacheFillSize()   1L

#define EFLAG_SIGN              0x8000
#define EFLAG_ZERO              0x4000
#define EFLAG_SELECT            (EFLAG_SIGN | EFLAG_ZERO)

#define RESULT_NEGATIVE         ((EFLAG_SIGN & ~EFLAG_ZERO) & EFLAG_SELECT)
#define RESULT_ZERO             ((~EFLAG_SIGN & EFLAG_ZERO) & EFLAG_SELECT)
#define RESULT_POSITIVE         ((~EFLAG_SIGN & ~EFLAG_ZERO) & EFLAG_SELECT)


typedef struct _KFLOATING_SAVE {
  ULONG ControlWord;
  ULONG StatusWord;
  ULONG ErrorOffset;
  ULONG ErrorSelector;
  ULONG DataOffset;
  ULONG DataSelector;
  ULONG Cr0NpxState;
  ULONG Spare1;
} KFLOATING_SAVE, *PKFLOATING_SAVE;

extern NTKERNELAPI volatile KSYSTEM_TIME KeTickCount;

#define YieldProcessor _mm_pause

FORCEINLINE
VOID
KeMemoryBarrier(VOID)
{
  volatile LONG Barrier;
#if defined(__GNUC__)
  __asm__ __volatile__ ("xchg %%eax, %0" : : "m" (Barrier) : "%eax");
#elif defined(_MSC_VER)
  __asm xchg [Barrier], eax
#endif
}

NTHALAPI
KIRQL
NTAPI
KeGetCurrentIrql(VOID);

NTHALAPI
VOID
FASTCALL
KfLowerIrql(
  IN KIRQL NewIrql);
#define KeLowerIrql(a) KfLowerIrql(a)

NTHALAPI
KIRQL
FASTCALL
KfRaiseIrql(
  IN KIRQL NewIrql);
#define KeRaiseIrql(a,b) *(b) = KfRaiseIrql(a)

NTHALAPI
KIRQL
NTAPI
KeRaiseIrqlToDpcLevel(VOID);

NTHALAPI
KIRQL
NTAPI
KeRaiseIrqlToSynchLevel(VOID);

NTHALAPI
KIRQL
FASTCALL
KfAcquireSpinLock(
  IN OUT PKSPIN_LOCK SpinLock);
#define KeAcquireSpinLock(a,b) *(b) = KfAcquireSpinLock(a)

NTHALAPI
VOID
FASTCALL
KfReleaseSpinLock(
  IN OUT PKSPIN_LOCK SpinLock,
  IN KIRQL NewIrql);
#define KeReleaseSpinLock(a,b) KfReleaseSpinLock(a,b)

NTKERNELAPI
VOID
FASTCALL
KefAcquireSpinLockAtDpcLevel(
  IN OUT PKSPIN_LOCK SpinLock);
#define KeAcquireSpinLockAtDpcLevel(SpinLock) KefAcquireSpinLockAtDpcLevel(SpinLock)

NTKERNELAPI
VOID
FASTCALL
KefReleaseSpinLockFromDpcLevel(
  IN OUT PKSPIN_LOCK SpinLock);
#define KeReleaseSpinLockFromDpcLevel(SpinLock) KefReleaseSpinLockFromDpcLevel(SpinLock)

NTSYSAPI
PKTHREAD
NTAPI
KeGetCurrentThread(VOID);

NTKERNELAPI
NTSTATUS
NTAPI
KeSaveFloatingPointState(
  OUT PKFLOATING_SAVE FloatSave);

NTKERNELAPI
NTSTATUS
NTAPI
KeRestoreFloatingPointState(
  IN PKFLOATING_SAVE FloatSave);

/* VOID
 * KeFlushIoBuffers(
 *   IN PMDL Mdl,
 *   IN BOOLEAN ReadOperation,
 *   IN BOOLEAN DmaOperation)
 */
#define KeFlushIoBuffers(_Mdl, _ReadOperation, _DmaOperation)

/* x86 and x64 performs a 0x2C interrupt */
#define DbgRaiseAssertionFailure __int2c

FORCEINLINE
VOID
_KeQueryTickCount(
  OUT PLARGE_INTEGER CurrentCount)
{
  for (;;) {
    CurrentCount->HighPart = KeTickCount.High1Time;
    CurrentCount->LowPart = KeTickCount.LowPart;
    if (CurrentCount->HighPart == KeTickCount.High2Time) break;
    YieldProcessor();
  }
}
#define KeQueryTickCount(CurrentCount) _KeQueryTickCount(CurrentCount)

$endif /* _WDMDDK_ */
$if (_NTDDK_)

#define PAUSE_PROCESSOR YieldProcessor();

#define KERNEL_STACK_SIZE                   12288
#define KERNEL_LARGE_STACK_SIZE             61440
#define KERNEL_LARGE_STACK_COMMIT           12288

#define SIZE_OF_80387_REGISTERS   80

#if !defined(RC_INVOKED)

#define CONTEXT_i386               0x10000
#define CONTEXT_i486               0x10000
#define CONTEXT_CONTROL            (CONTEXT_i386|0x00000001L)
#define CONTEXT_INTEGER            (CONTEXT_i386|0x00000002L)
#define CONTEXT_SEGMENTS           (CONTEXT_i386|0x00000004L)
#define CONTEXT_FLOATING_POINT     (CONTEXT_i386|0x00000008L)
#define CONTEXT_DEBUG_REGISTERS    (CONTEXT_i386|0x00000010L)
#define CONTEXT_EXTENDED_REGISTERS (CONTEXT_i386|0x00000020L)

#define CONTEXT_FULL (CONTEXT_CONTROL|CONTEXT_INTEGER|CONTEXT_SEGMENTS)
#define CONTEXT_ALL (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS |  \
                     CONTEXT_FLOATING_POINT | CONTEXT_DEBUG_REGISTERS |      \
                     CONTEXT_EXTENDED_REGISTERS)

#define CONTEXT_XSTATE          (CONTEXT_i386 | 0x00000040L)

#endif /* !defined(RC_INVOKED) */

typedef struct _FLOATING_SAVE_AREA {
  ULONG ControlWord;
  ULONG StatusWord;
  ULONG TagWord;
  ULONG ErrorOffset;
  ULONG ErrorSelector;
  ULONG DataOffset;
  ULONG DataSelector;
  UCHAR RegisterArea[SIZE_OF_80387_REGISTERS];
  ULONG Cr0NpxState;
} FLOATING_SAVE_AREA, *PFLOATING_SAVE_AREA;

#include "pshpack4.h"
typedef struct _CONTEXT {
  ULONG ContextFlags;
  ULONG Dr0;
  ULONG Dr1;
  ULONG Dr2;
  ULONG Dr3;
  ULONG Dr6;
  ULONG Dr7;
  FLOATING_SAVE_AREA FloatSave;
  ULONG SegGs;
  ULONG SegFs;
  ULONG SegEs;
  ULONG SegDs;
  ULONG Edi;
  ULONG Esi;
  ULONG Ebx;
  ULONG Edx;
  ULONG Ecx;
  ULONG Eax;
  ULONG Ebp;
  ULONG Eip;
  ULONG SegCs;
  ULONG EFlags;
  ULONG Esp;
  ULONG SegSs;
  UCHAR ExtendedRegisters[MAXIMUM_SUPPORTED_EXTENSION];
} CONTEXT;
#include "poppack.h"

#define KeGetPcr()                      PCR

#define PCR_MINOR_VERSION 1
#define PCR_MAJOR_VERSION 1

typedef struct _KPCR {
  union {
    NT_TIB NtTib;
    struct {
      struct _EXCEPTION_REGISTRATION_RECORD *Used_ExceptionList;
      PVOID Used_StackBase;
      PVOID Spare2;
      PVOID TssCopy;
      ULONG ContextSwitches;
      KAFFINITY SetMemberCopy;
      PVOID Used_Self;
    };
  };
  struct _KPCR *SelfPcr;
  struct _KPRCB *Prcb;
  KIRQL Irql;
  ULONG IRR;
  ULONG IrrActive;
  ULONG IDR;
  PVOID KdVersionBlock;
  struct _KIDTENTRY *IDT;
  struct _KGDTENTRY *GDT;
  struct _KTSS *TSS;
  USHORT MajorVersion;
  USHORT MinorVersion;
  KAFFINITY SetMember;
  ULONG StallScaleFactor;
  UCHAR SpareUnused;
  UCHAR Number;
  UCHAR Spare0;
  UCHAR SecondLevelCacheAssociativity;
  ULONG VdmAlert;
  ULONG KernelReserved[14];
  ULONG SecondLevelCacheSize;
  ULONG HalReserved[16];
} KPCR, *PKPCR;

FORCEINLINE
ULONG
KeGetCurrentProcessorNumber(VOID)
{
    return (ULONG)__readfsbyte(FIELD_OFFSET(KPCR, Number));
}

$endif /* _NTDDK_ */





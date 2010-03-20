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

$endif



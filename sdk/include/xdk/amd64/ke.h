$if (_WDMDDK_)
/** Kernel definitions for AMD64 **/

/* Interrupt request levels */
#define PASSIVE_LEVEL           0
#define LOW_LEVEL               0
#define APC_LEVEL               1
#define DISPATCH_LEVEL          2
#define CMCI_LEVEL              5
#define CLOCK_LEVEL             13
#define IPI_LEVEL               14
#define DRS_LEVEL               14
#define POWER_LEVEL             14
#define PROFILE_LEVEL           15
#define HIGH_LEVEL              15

#define KI_USER_SHARED_DATA     0xFFFFF78000000000ULL
#define SharedUserData          ((KUSER_SHARED_DATA * const)KI_USER_SHARED_DATA)
#define SharedInterruptTime     (KI_USER_SHARED_DATA + 0x8)
#define SharedSystemTime        (KI_USER_SHARED_DATA + 0x14)
#define SharedTickCount         (KI_USER_SHARED_DATA + 0x320)

#define PAGE_SIZE               0x1000
#define PAGE_SHIFT              12L

#define EFLAG_SIGN              0x8000
#define EFLAG_ZERO              0x4000
#define EFLAG_SELECT            (EFLAG_SIGN | EFLAG_ZERO)

typedef struct _KFLOATING_SAVE
{
    ULONG Dummy;
} KFLOATING_SAVE, *PKFLOATING_SAVE;

typedef XSAVE_FORMAT XMM_SAVE_AREA32, *PXMM_SAVE_AREA32;

#define KeQueryInterruptTime() \
    (*(volatile ULONG64*)SharedInterruptTime)

#define KeQuerySystemTime(CurrentCount) \
    *(ULONG64*)(CurrentCount) = *(volatile ULONG64*)SharedSystemTime

#define KeQueryTickCount(CurrentCount) \
    *(ULONG64*)(CurrentCount) = *(volatile ULONG64*)SharedTickCount

#define KeGetDcacheFillSize() 1L

#define YieldProcessor _mm_pause
#define MemoryBarrier __faststorefence
#define FastFence __faststorefence
#define LoadFence _mm_lfence
#define MemoryFence _mm_mfence
#define StoreFence _mm_sfence
#define LFENCE_ACQUIRE() LoadFence()

FORCEINLINE
VOID
KeMemoryBarrier(
    VOID)
{
    // FIXME: Do we really need lfence after the __faststorefence ?
    FastFence();
    LFENCE_ACQUIRE();
}

#define KeMemoryBarrierWithoutFence() _ReadWriteBarrier()

_IRQL_requires_max_(HIGH_LEVEL)
_IRQL_saves_
FORCEINLINE
KIRQL
KeGetCurrentIrql(VOID)
{
    return (KIRQL)__readcr8();
}

_IRQL_requires_max_(HIGH_LEVEL)
FORCEINLINE
VOID
KeLowerIrql(
    _In_ _IRQL_restores_ _Notliteral_ KIRQL NewIrql)
{
    //ASSERT((KIRQL)__readcr8() >= NewIrql);
    __writecr8(NewIrql);
}

_IRQL_requires_max_(HIGH_LEVEL)
_IRQL_raises_(NewIrql)
_IRQL_saves_
FORCEINLINE
KIRQL
KfRaiseIrql(
    _In_ KIRQL NewIrql)
{
    KIRQL OldIrql;

    OldIrql = (KIRQL)__readcr8();
    //ASSERT(OldIrql <= NewIrql);
    __writecr8(NewIrql);
    return OldIrql;
}
#define KeRaiseIrql(a,b) *(b) = KfRaiseIrql(a)

_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_saves_
_IRQL_raises_(DISPATCH_LEVEL)
FORCEINLINE
KIRQL
KeRaiseIrqlToDpcLevel(
    VOID)
{
    return KfRaiseIrql(DISPATCH_LEVEL);
}

FORCEINLINE
KIRQL
KeRaiseIrqlToSynchLevel(VOID)
{
    return KfRaiseIrql(12); // SYNCH_LEVEL = IPI_LEVEL - 2
}

FORCEINLINE
PKTHREAD
KeGetCurrentThread(VOID)
{
    return (struct _KTHREAD *)__readgsqword(0x188);
}

_Always_(_Post_satisfies_(return<=0))
_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
_Kernel_float_saved_
_At_(*FloatSave, _Kernel_requires_resource_not_held_(FloatState) _Kernel_acquires_resource_(FloatState))
FORCEINLINE
NTSTATUS
KeSaveFloatingPointState(
    _Out_ PKFLOATING_SAVE FloatSave)
{
    UNREFERENCED_PARAMETER(FloatSave);
    return STATUS_SUCCESS;
}

_Success_(1)
_Kernel_float_restored_
_At_(*FloatSave, _Kernel_requires_resource_held_(FloatState) _Kernel_releases_resource_(FloatState))
FORCEINLINE
NTSTATUS
KeRestoreFloatingPointState(
    _In_ PKFLOATING_SAVE FloatSave)
{
    UNREFERENCED_PARAMETER(FloatSave);
    return STATUS_SUCCESS;
}

/* VOID
 * KeFlushIoBuffers(
 *   IN PMDL Mdl,
 *   IN BOOLEAN ReadOperation,
 *   IN BOOLEAN DmaOperation)
 */
#define KeFlushIoBuffers(_Mdl, _ReadOperation, _DmaOperation)

/* x86 and x64 performs a 0x2C interrupt */
#define DbgRaiseAssertionFailure __int2c

$endif /* _WDMDDK_ */
$if (_NTDDK_)

#define PAUSE_PROCESSOR YieldProcessor();

#define KERNEL_STACK_SIZE 0x6000
#define KERNEL_LARGE_STACK_SIZE 0x12000
#define KERNEL_LARGE_STACK_COMMIT KERNEL_STACK_SIZE

#define KERNEL_MCA_EXCEPTION_STACK_SIZE 0x2000

#define EXCEPTION_READ_FAULT    0
#define EXCEPTION_WRITE_FAULT   1
#define EXCEPTION_EXECUTE_FAULT 8

#if !defined(RC_INVOKED)

#define CONTEXT_AMD64 0x100000

#define CONTEXT_CONTROL (CONTEXT_AMD64 | 0x1L)
#define CONTEXT_INTEGER (CONTEXT_AMD64 | 0x2L)
#define CONTEXT_SEGMENTS (CONTEXT_AMD64 | 0x4L)
#define CONTEXT_FLOATING_POINT (CONTEXT_AMD64 | 0x8L)
#define CONTEXT_DEBUG_REGISTERS (CONTEXT_AMD64 | 0x10L)

#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_FLOATING_POINT)
#define CONTEXT_ALL (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS | CONTEXT_FLOATING_POINT | CONTEXT_DEBUG_REGISTERS)

#define CONTEXT_XSTATE (CONTEXT_AMD64 | 0x20L)

#define CONTEXT_EXCEPTION_ACTIVE 0x8000000
#define CONTEXT_SERVICE_ACTIVE 0x10000000
#define CONTEXT_EXCEPTION_REQUEST 0x40000000
#define CONTEXT_EXCEPTION_REPORTING 0x80000000

#endif /* !defined(RC_INVOKED) */

#define INITIAL_MXCSR                  0x1f80
#define INITIAL_FPCSR                  0x027f

typedef struct DECLSPEC_ALIGN(16) _CONTEXT {
  ULONG64 P1Home;
  ULONG64 P2Home;
  ULONG64 P3Home;
  ULONG64 P4Home;
  ULONG64 P5Home;
  ULONG64 P6Home;
  ULONG ContextFlags;
  ULONG MxCsr;
  USHORT SegCs;
  USHORT SegDs;
  USHORT SegEs;
  USHORT SegFs;
  USHORT SegGs;
  USHORT SegSs;
  ULONG EFlags;
  ULONG64 Dr0;
  ULONG64 Dr1;
  ULONG64 Dr2;
  ULONG64 Dr3;
  ULONG64 Dr6;
  ULONG64 Dr7;
  ULONG64 Rax;
  ULONG64 Rcx;
  ULONG64 Rdx;
  ULONG64 Rbx;
  ULONG64 Rsp;
  ULONG64 Rbp;
  ULONG64 Rsi;
  ULONG64 Rdi;
  ULONG64 R8;
  ULONG64 R9;
  ULONG64 R10;
  ULONG64 R11;
  ULONG64 R12;
  ULONG64 R13;
  ULONG64 R14;
  ULONG64 R15;
  ULONG64 Rip;
  union {
    XMM_SAVE_AREA32 FltSave;
    struct {
      M128A Header[2];
      M128A Legacy[8];
      M128A Xmm0;
      M128A Xmm1;
      M128A Xmm2;
      M128A Xmm3;
      M128A Xmm4;
      M128A Xmm5;
      M128A Xmm6;
      M128A Xmm7;
      M128A Xmm8;
      M128A Xmm9;
      M128A Xmm10;
      M128A Xmm11;
      M128A Xmm12;
      M128A Xmm13;
      M128A Xmm14;
      M128A Xmm15;
    } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME;
  M128A VectorRegister[26];
  ULONG64 VectorControl;
  ULONG64 DebugControl;
  ULONG64 LastBranchToRip;
  ULONG64 LastBranchFromRip;
  ULONG64 LastExceptionToRip;
  ULONG64 LastExceptionFromRip;
} CONTEXT;

#define PCR_MINOR_VERSION 1
#define PCR_MAJOR_VERSION 1

typedef struct _KPCR
{
    _ANONYMOUS_UNION union
    {
        NT_TIB NtTib;
        _ANONYMOUS_STRUCT struct
        {
            union _KGDTENTRY64 *GdtBase;
            struct _KTSS64 *TssBase;
            ULONG64 UserRsp;
            struct _KPCR *Self;
            struct _KPRCB *CurrentPrcb;
            PKSPIN_LOCK_QUEUE LockArray;
            PVOID Used_Self;
        };
    };
    union _KIDTENTRY64 *IdtBase;
    ULONG64 Unused[2];
    KIRQL Irql;
    UCHAR SecondLevelCacheAssociativity;
    UCHAR ObsoleteNumber;
    UCHAR Fill0;
    ULONG Unused0[3];
    USHORT MajorVersion;
    USHORT MinorVersion;
    ULONG StallScaleFactor;
    PVOID Unused1[3];
    ULONG KernelReserved[15];
    ULONG SecondLevelCacheSize;
    ULONG HalReserved[16];
    ULONG Unused2;
    PVOID KdVersionBlock;
    PVOID Unused3;
    ULONG PcrAlign1[24];
} KPCR, *PKPCR;

FORCEINLINE
PKPCR
KeGetPcr(VOID)
{
    return (PKPCR)__readgsqword(FIELD_OFFSET(KPCR, Self));
}

FORCEINLINE
ULONG
KeGetCurrentProcessorNumber(VOID)
{
    return (ULONG)__readgsword(0x184);
}

$endif /* _NTDDK_ */

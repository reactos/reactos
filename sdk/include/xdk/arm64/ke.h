$if (_WDMDDK_)
/** Kernel definitions for ARM64 **/

/* Interrupt request levels */
#define PASSIVE_LEVEL           0
#define LOW_LEVEL               0
#define APC_LEVEL               1
#define DISPATCH_LEVEL          2
#define CLOCK_LEVEL             13
#define IPI_LEVEL               14
#define DRS_LEVEL               14
#define POWER_LEVEL             14
#define PROFILE_LEVEL           15
#define HIGH_LEVEL              15

#define KI_USER_SHARED_DATA     0xFFFFF78000000000ULL
#define SharedUserData          ((KUSER_SHARED_DATA * const)KI_USER_SHARED_DATA)
#define SharedTickCount         (KI_USER_SHARED_DATA + 0x320)

#define PAGE_SIZE               0x1000
#define PAGE_SHIFT              12L

#define PAUSE_PROCESSOR YieldProcessor();

/* FIXME: Based on AMD64 but needed to compile apps */
#define KERNEL_STACK_SIZE                   12288
#define KERNEL_LARGE_STACK_SIZE             61440
#define KERNEL_LARGE_STACK_COMMIT KERNEL_STACK_SIZE
/* FIXME End */

#define EXCEPTION_READ_FAULT    0
#define EXCEPTION_WRITE_FAULT   1
#define EXCEPTION_EXECUTE_FAULT 8

#define KeQueryTickCount(CurrentCount) \
    *(ULONG64*)(CurrentCount) = *(volatile ULONG64*)SharedTickCount

FORCEINLINE
VOID
KeMemoryBarrier(
    VOID)
{
    _ReadWriteBarrier();
    MemoryBarrier();
}

#define KeMemoryBarrierWithoutFence() _ReadWriteBarrier()

_IRQL_requires_max_(HIGH_LEVEL)
_IRQL_saves_
NTHALAPI
KIRQL
NTAPI
KeGetCurrentIrql(
    VOID);

_IRQL_requires_max_(HIGH_LEVEL)
NTHALAPI
VOID
FASTCALL
KfLowerIrql(
    _In_ _IRQL_restores_ _Notliteral_ KIRQL NewIrql);
#define KeLowerIrql(a) KfLowerIrql(a)

_IRQL_requires_max_(HIGH_LEVEL)
_IRQL_raises_(NewIrql)
_IRQL_saves_
NTHALAPI
KIRQL
FASTCALL
KfRaiseIrql(
    _In_ KIRQL NewIrql);
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
#ifdef CONFIG_SMP
    return KfRaiseIrql(12); // SYNCH_LEVEL = IPI_LEVEL - 2
#else
    return KfRaiseIrql(2); // SYNCH_LEVEL = DISPATCH_LEVEL
#endif
}

NTSYSAPI
PKTHREAD
NTAPI
KeGetCurrentThread(VOID);

#if (NTDDI_VERSION >= NTDDI_WIN7)
/* KPRCB::Number, the PRCB starts at PCR offset 0x980 */
FORCEINLINE
ULONG
KeGetCurrentProcessorIndex(VOID)
{
    return __readx18dword(0x9a4);
}
#endif

NTKERNELAPI
VOID
NTAPI
KeFlushIoBuffers(
    _In_ PMDL Mdl,
    _In_ BOOLEAN ReadOperation,
    _In_ BOOLEAN DmaOperation);

#define DbgRaiseAssertionFailure() __break(0xf001)

$endif (_WDMDDK_)
$if (_NTDDK_)

#define ARM64_MAX_BREAKPOINTS 8
#define ARM64_MAX_WATCHPOINTS 2

#define CONTEXT_ARM64           0x400000L
#define CONTEXT_CONTROL         (CONTEXT_ARM64 | 0x00000001L)
#define CONTEXT_INTEGER         (CONTEXT_ARM64 | 0x00000002L)
#define CONTEXT_FLOATING_POINT  (CONTEXT_ARM64 | 0x00000004L)
#define CONTEXT_DEBUG_REGISTERS (CONTEXT_ARM64 | 0x00000008L)
#define CONTEXT_X18             (CONTEXT_ARM64 | 0x00000010L)
#define CONTEXT_FULL            (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_FLOATING_POINT)

typedef union NEON128 {
    struct {
        ULONGLONG Low;
        LONGLONG High;
    } DUMMYSTRUCTNAME;
    double D[2];
    float  S[4];
    USHORT H[8];
    UCHAR  B[16];
} NEON128, *PNEON128;
typedef NEON128 NEON128, *PNEON128;

typedef struct _CONTEXT {

    //
    // Control flags.
    //

    ULONG ContextFlags;

    //
    // Integer registers
    //

    ULONG Cpsr;
    union {
        struct {
            ULONG64 X0;
            ULONG64 X1;
            ULONG64 X2;
            ULONG64 X3;
            ULONG64 X4;
            ULONG64 X5;
            ULONG64 X6;
            ULONG64 X7;
            ULONG64 X8;
            ULONG64 X9;
            ULONG64 X10;
            ULONG64 X11;
            ULONG64 X12;
            ULONG64 X13;
            ULONG64 X14;
            ULONG64 X15;
            ULONG64 X16;
            ULONG64 X17;
            ULONG64 X18;
            ULONG64 X19;
            ULONG64 X20;
            ULONG64 X21;
            ULONG64 X22;
            ULONG64 X23;
            ULONG64 X24;
            ULONG64 X25;
            ULONG64 X26;
            ULONG64 X27;
            ULONG64 X28;
    		ULONG64 Fp;
            ULONG64 Lr;
        } DUMMYSTRUCTNAME;
        ULONG64 X[31];
    } DUMMYUNIONNAME;

    ULONG64 Sp;
    ULONG64 Pc;

    //
    // Floating Point/NEON Registers
    //

    NEON128 V[32];
    ULONG Fpcr;
    ULONG Fpsr;

    //
    // Debug registers
    //

    ULONG Bcr[ARM64_MAX_BREAKPOINTS];
    ULONG64 Bvr[ARM64_MAX_BREAKPOINTS];
    ULONG Wcr[ARM64_MAX_WATCHPOINTS];
    ULONG64 Wvr[ARM64_MAX_WATCHPOINTS];

} CONTEXT, *PCONTEXT;

#define PCR_MINOR_VERSION 1
#define PCR_MAJOR_VERSION 1

typedef struct _KPCR
{
    _ANONYMOUS_UNION union
    {
        NT_TIB NtTib;
        _ANONYMOUS_STRUCT struct
        {
            PVOID TibPad0[2];
            PVOID Spare1;
            struct _KPCR *Self;
            PVOID PcrReserved0;
            PKSPIN_LOCK_QUEUE LockArray;
            PVOID Used_Self;
        };
    };
    KIRQL CurrentIrql;
    UCHAR SecondLevelCacheAssociativity;
    UCHAR Pad1[2];
    USHORT MajorVersion;
    USHORT MinorVersion;
    ULONG StallScaleFactor;
    ULONG SecondLevelCacheSize;
    _ANONYMOUS_UNION union
    {
        USHORT SoftwareInterruptPending;
        _ANONYMOUS_STRUCT struct
        {
            UCHAR ApcInterrupt;
            UCHAR DispatchInterrupt;
        };
    };
    USHORT InterruptPad;
    _ANONYMOUS_UNION union
    {
        UCHAR BtiMitigation;
        _ANONYMOUS_STRUCT struct
        {
            UCHAR KvaVbar:1;
            UCHAR BtiVbar:3;
            UCHAR BtiCswapHvc:1;
            UCHAR BtiCswapSmc:1;
        };
    };
    _ANONYMOUS_UNION union
    {
        UCHAR SsbMitigationFlags;
        _ANONYMOUS_STRUCT struct
        {
            UCHAR SsbMitigationFirmware:1;
            UCHAR SsbMitigationDynamic:1;
            UCHAR SsbMitigationKernel:1;
            UCHAR SsbMitigationUser:1;
            UCHAR SsbMitigationReserved:4;
        };
    };
    UCHAR BhbMitigation;
    _ANONYMOUS_UNION union
    {
        UCHAR CachePrefetcherMitigationFlags;
        _ANONYMOUS_STRUCT struct
        {
            UCHAR CachePrefetcherMitigation:1;
            UCHAR Pad2:7;
        };
    };
    ULONG64 PanicStorage[6];
    PVOID KdVersionBlock;
    PVOID HalReserved[14];
    PVOID KvaUserModeTtbr1;
} KPCR, *PKPCR;

/* x18 holds the PCR address in kernel mode */
FORCEINLINE
PKPCR
KeGetPcr(VOID)
{
    return (PKPCR)__getReg(18);
}

#if (NTDDI_VERSION < NTDDI_WIN7) || !defined(NT_PROCESSOR_GROUPS)
FORCEINLINE
ULONG
KeGetCurrentProcessorNumber(VOID)
{
    return __readx18dword(0x9a4);
}
#endif /* (NTDDI_VERSION < NTDDI_WIN7) || !defined(NT_PROCESSOR_GROUPS) */

$endif

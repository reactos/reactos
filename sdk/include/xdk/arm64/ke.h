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

#define KI_USER_SHARED_DATA 0xFFFFF78000000000UI64

#define SharedUserData ((KUSER_SHARED_DATA * const)KI_USER_SHARED_DATA)
#define SharedInterruptTime (KI_USER_SHARED_DATA + 0x8)
#define SharedSystemTime (KI_USER_SHARED_DATA + 0x14)
#define SharedTickCount (KI_USER_SHARED_DATA + 0x320)

#if 0
//TODO: Fix instrinsics
#define KeQueryInterruptTime() ((ULONG64)ReadNoFence64((const volatile LONG64 *)(SharedInterruptTime)))

#define KeQuerySystemTime(CurrentCount)                                     \
    *((PULONG64)(CurrentCount)) = ReadNoFence64((const volatile LONG64 *)(SharedSystemTime))

#define KeQueryTickCount(CurrentCount)                                      \
    *((PULONG64)(CurrentCount)) = ReadNoFence64((const volatile LONG64 *)(SharedTickCount))
#endif

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




#define PCR_MINOR_VERSION 1
#define PCR_MAJOR_VERSION 1
/* this is just ARM32 KPCR, it's a hack to move on*/
typedef struct _KPCR
{
    _ANONYMOUS_UNION union
    {
        _ANONYMOUS_STRUCT struct
        {
            ULONG TibPad0[2];
            PVOID Spare1;
            struct _KPCR *Self;
            struct _KPRCB *CurrentPrcb;
            PKSPIN_LOCK_QUEUE LockArray;
            PVOID Used_Self;
        };
    };
    KIRQL CurrentIrql;
    UCHAR SecondLevelCacheAssociativity;
    ULONG Unused0[3];
    USHORT MajorVersion;
    USHORT MinorVersion;
    ULONG StallScaleFactor;
    PVOID Unused1[3];
    ULONG KernelReserved[15];
    ULONG SecondLevelCacheSize;
    _ANONYMOUS_UNION union
    {
        USHORT SoftwareInterruptPending; // Software Interrupt Pending Flag
        struct
        {
            UCHAR ApcInterrupt;          // 0x01 if APC int pending
            UCHAR DispatchInterrupt;     // 0x01 if dispatch int pending
        };
    };
    USHORT InterruptPad;
    ULONG HalReserved[32];
    PVOID KdVersionBlock;
    PVOID Unused3;
    ULONG PcrAlign1[8];
} KPCR, *PKPCR;


/* this isn't correct.. There's a far better way to do this then this static address. */
#define KIP0PCRADDRESS                      0xFFFFF78000001000ULL /* FIXME!!! */
#define PCR                     ((KPCR * const)KIP0PCRADDRESS)

FORCEINLINE
PKPCR
KeGetPcr(
    VOID)
{
    return (PKPCR)(PCR);
}

NTSYSAPI
PKTHREAD
NTAPI
KeGetCurrentThread(VOID);

#define DbgRaiseAssertionFailure() __break(0xf001)

/* Interesting.. even in Windows it's like this for arm64 */
NTHALAPI
KIRQL
KeGetCurrentIrql(VOID);

NTHALAPI
VOID
KfLowerIrql(_In_ KIRQL NewIrql);

NTHALAPI
KIRQL
KfRaiseIrql(_In_ KIRQL NewIrql);

#define KeLowerIrql(a) KfLowerIrql(a)
#define KeRaiseIrql(a,b) *(b) = KfRaiseIrql(a)

$endif (_WDMDDK_)
$if (_NTDDK_)

#define ARM64_MAX_BREAKPOINTS 8
#define ARM64_MAX_WATCHPOINTS 2

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
$endif

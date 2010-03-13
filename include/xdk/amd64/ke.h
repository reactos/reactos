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

#define PAGE_SIZE               0x1000
#define PAGE_SHIFT              12L

#define KI_USER_SHARED_DATA     0xFFFFF78000000000UI64
#define SharedUserData          ((PKUSER_SHARED_DATA const)KI_USER_SHARED_DATA)


typedef struct _KFLOATING_SAVE {
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

FORCEINLINE
KIRQL
KeGetCurrentIrql(VOID)
{
    return (KIRQL)__readcr8();
}

FORCEINLINE
VOID
KeLowerIrql(IN KIRQL NewIrql)
{
    ASSERT(KeGetCurrentIrql() >= NewIrql);
    __writecr8(NewIrql);
}

FORCEINLINE
KIRQL
KfRaiseIrql(IN KIRQL NewIrql)
{
    KIRQL OldIrql;

    OldIrql = __readcr8();
    ASSERT(OldIrql <= NewIrql);
    __writecr8(NewIrql);
    return OldIrql;
}
#define KeRaiseIrql(a,b) *(b) = KfRaiseIrql(a)

FORCEINLINE
KIRQL
KeRaiseIrqlToDpcLevel(VOID)
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
KeGetCurrentThread (
  VOID)
{
    return (struct _KTHREAD *)__readgsqword(0x188);
}

/* x86 and x64 performs a 0x2C interrupt */
#define DbgRaiseAssertionFailure __int2c
$endif


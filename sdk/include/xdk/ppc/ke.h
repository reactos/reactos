$if (_WDMDDK_)
/** Kernel definitions for PPC **/

/* Interrupt request levels */
#define PASSIVE_LEVEL                      0
#define LOW_LEVEL                          0
#define APC_LEVEL                          1
#define DISPATCH_LEVEL                     2
#define PROFILE_LEVEL                     27
#define CLOCK1_LEVEL                      28
#define CLOCK2_LEVEL                      28
#define IPI_LEVEL                         29
#define POWER_LEVEL                       30
#define HIGH_LEVEL                        31

//
// Used to contain PFNs and PFN counts
//
typedef ULONG PFN_COUNT;
typedef ULONG PFN_NUMBER, *PPFN_NUMBER;
typedef LONG SPFN_NUMBER, *PSPFN_NUMBER;


typedef struct _KFLOATING_SAVE {
  ULONG Dummy;
} KFLOATING_SAVE, *PKFLOATING_SAVE;

typedef struct _KPCR_TIB {
  PVOID ExceptionList;         /* 00 */
  PVOID StackBase;             /* 04 */
  PVOID StackLimit;            /* 08 */
  PVOID SubSystemTib;          /* 0C */
  _ANONYMOUS_UNION union {
    PVOID FiberData;           /* 10 */
    ULONG Version;             /* 10 */
  } DUMMYUNIONNAME;
  PVOID ArbitraryUserPointer;  /* 14 */
  struct _KPCR_TIB *Self;       /* 18 */
} KPCR_TIB, *PKPCR_TIB;         /* 1C */

#define PCR_MINOR_VERSION 1
#define PCR_MAJOR_VERSION 1

typedef struct _KPCR {
  KPCR_TIB Tib;                /* 00 */
  struct _KPCR *Self;          /* 1C */
  struct _KPRCB *Prcb;         /* 20 */
  KIRQL Irql;                  /* 24 */
  ULONG IRR;                   /* 28 */
  ULONG IrrActive;             /* 2C */
  ULONG IDR;                   /* 30 */
  PVOID KdVersionBlock;        /* 34 */
  PUSHORT IDT;                 /* 38 */
  PUSHORT GDT;                 /* 3C */
  struct _KTSS *TSS;           /* 40 */
  USHORT MajorVersion;         /* 44 */
  USHORT MinorVersion;         /* 46 */
  KAFFINITY SetMember;         /* 48 */
  ULONG StallScaleFactor;      /* 4C */
  UCHAR SpareUnused;           /* 50 */
  UCHAR Number;                /* 51 */
} KPCR, *PKPCR;                /* 54 */

#define KeGetPcr()                      PCR

#define YieldProcessor() __asm__ __volatile__("nop");

FORCEINLINE
ULONG
NTAPI
KeGetCurrentProcessorNumber(VOID)
{
  ULONG Number;
  __asm__ __volatile__ (
    "lwz %0, %c1(12)\n"
    : "=r" (Number)
    : "i" (FIELD_OFFSET(KPCR, Number))
  );
  return Number;
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

$endif

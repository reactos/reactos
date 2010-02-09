#ifndef _ARMDDK_
#define _ARMDDK_

//
// Page size
//
#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000
#endif

#ifndef _WINNT_H
//
// IRQLs
//
#define PASSIVE_LEVEL                     0
#define LOW_LEVEL                         0
#define APC_LEVEL                         1
#define DISPATCH_LEVEL                    2
#define SYNCH_LEVEL                       DISPATCH_LEVEL
#define PROFILE_LEVEL                     27
#define CLOCK1_LEVEL                      28
#define CLOCK2_LEVEL                      28
#define IPI_LEVEL                         29
#define POWER_LEVEL                       30
#define HIGH_LEVEL                        31
#endif

//
// FIXME: mmtypes.h?
//
#define KIP0PCRADDRESS          0xFFDFF000
#define KI_USER_SHARED_DATA     0xFFDF0000
#define USPCR                   0x7FFF0000
#define PCR                     ((KPCR * const)KIP0PCRADDRESS)
#define USERPCR                 ((volatile KPCR * const)USPCR)
#define KeGetPcr()              PCR
#ifndef _WINNT_H
#define SharedUserData          ((KUSER_SHARED_DATA * const)KI_USER_SHARED_DATA)

//
// Address space layout
//
extern PVOID MmHighestUserAddress;
extern PVOID MmSystemRangeStart;
extern ULONG_PTR MmUserProbeAddress;
#define MM_HIGHEST_USER_ADDRESS           MmHighestUserAddress
#define MM_SYSTEM_RANGE_START             MmSystemRangeStart
#define MM_USER_PROBE_ADDRESS             MmUserProbeAddress
#define MM_LOWEST_USER_ADDRESS            (PVOID)0x10000
#define MM_LOWEST_SYSTEM_ADDRESS          (PVOID)0xC0800000

//
// Maximum IRQs
//
#define MAXIMUM_VECTOR          16


//
// Used to contain PFNs and PFN counts
//
typedef ULONG PFN_COUNT;
typedef ULONG PFN_NUMBER, *PPFN_NUMBER;
typedef LONG SPFN_NUMBER, *PSPFN_NUMBER;

//
// Stub
//
typedef struct _KFLOATING_SAVE
{
    ULONG Reserved;
} KFLOATING_SAVE, *PKFLOATING_SAVE;

/* The following flags control the contents of the CONTEXT structure. */
#define CONTEXT_ARM    0x0000040
#define CONTEXT_CONTROL         (CONTEXT_ARM | 0x00000001L)
#define CONTEXT_INTEGER         (CONTEXT_ARM | 0x00000002L)
#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_INTEGER)

typedef struct _CONTEXT {
	/* The flags values within this flag control the contents of
	   a CONTEXT record.

	   If the context record is used as an input parameter, then
	   for each portion of the context record controlled by a flag
	   whose value is set, it is assumed that that portion of the
	   context record contains valid context. If the context record
	   is being used to modify a thread's context, then only that
	   portion of the threads context will be modified.

	   If the context record is used as an IN OUT parameter to capture
	   the context of a thread, then only those portions of the thread's
	   context corresponding to set flags will be returned.

	   The context record is never used as an OUT only parameter. */

	ULONG ContextFlags;

	/* This section is specified/returned if the ContextFlags word contains
	   the flag CONTEXT_INTEGER. */
	ULONG R0;
	ULONG R1;
	ULONG R2;
	ULONG R3;
	ULONG R4;
	ULONG R5;
	ULONG R6;
	ULONG R7;
	ULONG R8;
	ULONG R9;
	ULONG R10;
	ULONG R11;
	ULONG R12;

	ULONG Sp;
	ULONG Lr;
	ULONG Pc;
	ULONG Psr;
} CONTEXT;
#endif

//
// Processor Control Region
//
#ifdef _WINNT_H
typedef
VOID
(*PKINTERRUPT_ROUTINE)(VOID);
#endif
typedef struct _KPCR
{
    union
    {
        NT_TIB NtTib;
        struct
        {
            struct _EXCEPTION_REGISTRATION_RECORD *Used_ExceptionList; // Unused
            PVOID Used_StackBase; // Unused
            PVOID PerfGlobalGroupMask;
            PVOID TssCopy; // Unused
            ULONG ContextSwitches;
            KAFFINITY SetMemberCopy; // Unused
            PVOID Used_Self;
        };
    };
    struct _KPCR *Self;
    struct _KPRCB *Prcb;
    KIRQL Irql;
    ULONG IRR; // Unused
    ULONG IrrActive; // Unused
    ULONG IDR; // Unused
    PVOID KdVersionBlock;
    PVOID IDT; // Unused
    PVOID GDT; // Unused
    PVOID TSS; // Unused
    USHORT MajorVersion;
    USHORT MinorVersion;
    KAFFINITY SetMember;
    ULONG StallScaleFactor;
    UCHAR SpareUnused;
    UCHAR Number;
} KPCR, *PKPCR;

//
// Get the current TEB
//
FORCEINLINE
struct _TEB* NtCurrentTeb(VOID)
{
    return (struct _TEB*)USERPCR->Used_Self;
}

#ifndef _WINNT_H
//
// IRQL Support on ARM is similar to MIPS/ALPHA
//
KIRQL
KfRaiseIrql(
    IN KIRQL NewIrql
);

VOID
KfLowerIrql(
    IN KIRQL NewIrql
);

KIRQL
KeRaiseIrqlToSynchLevel(
    VOID
);

KIRQL
KeRaiseIrqlToDpcLevel(
    VOID
);

#define KeLowerIrql(NewIrql) KfLowerIrql(NewIrql)
#define KeRaiseIrql(NewIrql, OldIrql) *(OldIrql) = KfRaiseIrql(NewIrql)

//
// Cache clean and flush
//
VOID
HalSweepDcache(
    VOID
);

VOID
HalSweepIcache(
    VOID
);
#endif

//
// Intrinsics
//
#define InterlockedDecrement _InterlockedDecrement
#define InterlockedIncrement _InterlockedIncrement
#define InterlockedExchange  _InterlockedExchange
#endif

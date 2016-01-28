/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/powerpc/cpu.c
 * PURPOSE:         Routines for CPU-level support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* CPU Features and Flags */
ULONG KeLargestCacheLine = 0x40;
ULONG KeDcacheFlushCount = 0;
ULONG KeIcacheFlushCount = 0;
ULONG KiDmaIoCoherency = 0;
BOOLEAN KiSMTProcessorsPresent;

/* CPU Signatures */
#if 0
CHAR CmpIntelID[]       = "GenuineIntel";
CHAR CmpAmdID[]         = "AuthenticAMD";
CHAR CmpCyrixID[]       = "CyrixInstead";
CHAR CmpTransmetaID[]   = "GenuineTMx86";
CHAR CmpCentaurID[]     = "CentaurHauls";
CHAR CmpRiseID[]        = "RiseRiseRise";
#endif

/* SUPPORT ROUTINES FOR MSVC COMPATIBILITY ***********************************/

VOID
NTAPI
CPUID(IN ULONG CpuInfo[4],
      IN ULONG InfoType)
{
    RtlZeroMemory(CpuInfo, 4 * sizeof(ULONG));
}

VOID
WRMSR(IN ULONG Register,
      IN LONGLONG Value)
{
}

LONGLONG
RDMSR(IN ULONG Register)
{
    LARGE_INTEGER LargeVal;
    LargeVal.QuadPart = 0;
    return LargeVal.QuadPart;
}

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KiSetProcessorType(VOID)
{
}

ULONG
NTAPI
KiGetCpuVendor(VOID)
{
    return 0;
}

ULONG
NTAPI
KiGetFeatureBits(VOID)
{
    ULONG FeatureBits = 0;
    /* Return the Feature Bits */
    return FeatureBits;
}

VOID
NTAPI
KiGetCacheInformation(VOID)
{
}

VOID
NTAPI
KiSetCR0Bits(VOID)
{
}

VOID
NTAPI
KiInitializeTSS2(IN PKTSS Tss,
                 IN PKGDTENTRY TssEntry OPTIONAL)
{
}

VOID
NTAPI
KiInitializeTSS(IN PKTSS Tss)
{
}

VOID
FASTCALL
Ki386InitializeTss(IN PKTSS Tss,
                   IN PKIDTENTRY Idt,
                   IN PKGDTENTRY Gdt)
{
}

VOID
NTAPI
KeFlushCurrentTb(VOID)
{
}

VOID
NTAPI
KiSaveProcessorControlState(OUT PKPROCESSOR_STATE ProcessorState)
{
}

VOID
NTAPI
KiInitializeMachineType(VOID)
{
}

ULONG_PTR
NTAPI
KiLoadFastSyscallMachineSpecificRegisters(IN ULONG_PTR Context)
{
    return 0;
}

VOID
NTAPI
KiRestoreFastSyscallReturnState(VOID)
{
}

ULONG_PTR
NTAPI
Ki386EnableDE(IN ULONG_PTR Context)
{
    return 0;
}

ULONG_PTR
NTAPI
Ki386EnableFxsr(IN ULONG_PTR Context)
{
    return 0;
}

ULONG_PTR
NTAPI
Ki386EnableXMMIExceptions(IN ULONG_PTR Context)
{
    /* FIXME: Support this */
    DPRINT1("Your machine supports XMMI exceptions but ReactOS doesn't\n");
    return 0;
}

VOID
NTAPI
KiI386PentiumLockErrataFixup(VOID)
{
    /* FIXME: Support this */
    DPRINT1("WARNING: Your machine has a CPU bug that ReactOS can't bypass!\n");
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
KeSaveFloatingPointState(OUT PKFLOATING_SAVE Save)
{
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
KeRestoreFloatingPointState(IN PKFLOATING_SAVE Save)
{
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
ULONG
NTAPI
KeGetRecommendedSharedDataAlignment(VOID)
{
    /* Return the global variable */
    return KeLargestCacheLine;
}

/*
 * @implemented
 */
VOID
NTAPI
KeFlushEntireTb(IN BOOLEAN Invalid,
                IN BOOLEAN AllProcessors)
{
    KIRQL OldIrql;

    /* Raise the IRQL for the TB Flush */
    OldIrql = KeRaiseIrqlToSynchLevel();

#ifdef CONFIG_SMP
    /* FIXME: Support IPI Flush */
#error Not yet implemented!
#endif

    /* Flush the TB for the Current CPU */
    //KeFlushCurrentTb();

    /* Return to Original IRQL */
    KeLowerIrql(OldIrql);
}

/*
 * @implemented
 */
VOID
NTAPI
KeSetDmaIoCoherency(IN ULONG Coherency)
{
    /* Save the coherency globally */
    KiDmaIoCoherency = Coherency;
}

/*
 * @implemented
 */
KAFFINITY
NTAPI
KeQueryActiveProcessors(VOID)
{
    PAGED_CODE();

    /* Simply return the number of active processors */
    return KeActiveProcessors;
}

/*
 * @implemented
 */
VOID
__cdecl
KeSaveStateForHibernate(IN PKPROCESSOR_STATE State)
{
    /* Capture the context */
    RtlCaptureContext(&State->ContextFrame);

    /* Capture the control state */
    KiSaveProcessorControlState(State);
}

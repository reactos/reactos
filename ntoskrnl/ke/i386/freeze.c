/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Processor freeze support for i386
 * COPYRIGHT:
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

static PKPRCB KiI386FreezeOwner;
static volatile KAFFINITY KiI386FreezeTargetSet;
static volatile KAFFINITY KiI386FrozenSet;
static volatile KAFFINITY KiI386ThawSet;
static volatile KAFFINITY KiI386FreezeReadySet;

#if DBG && defined(_M_IX86)
extern ULONG KeBugCheckCount;
extern ULONG KeBugCheckOwner;
extern LONG KeBugCheckOwnerRecursionCount;

#define KX_RAW_COM1_BASE 0x3F8
#define KX_RAW_COM1_LINE_STATUS 5
#define KX_RAW_COM1_TRANSMIT_EMPTY 0x20

static
VOID
NTAPI
KxRawCom1WriteByte(
    _In_ UCHAR Character)
{
    ULONG SpinCount = 100000;

    while (SpinCount-- != 0)
    {
        if (READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)(KX_RAW_COM1_BASE +
                                                KX_RAW_COM1_LINE_STATUS)) &
            KX_RAW_COM1_TRANSMIT_EMPTY)
            break;
    }

    WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)KX_RAW_COM1_BASE, Character);
}

static
VOID
NTAPI
KxRawCom1WriteString(
    _In_z_ const CHAR *String)
{
    while (*String != '\0')
    {
        if (*String == '\n')
            KxRawCom1WriteByte('\r');

        KxRawCom1WriteByte(*String++);
    }
}

static
VOID
NTAPI
KxRawCom1WriteHex(
    _In_ ULONG_PTR Value)
{
    ULONG Index;

    for (Index = 0; Index < 8; Index++)
    {
        ULONG Nibble = (Value >> (28 - Index * 4)) & 0xF;

        KxRawCom1WriteByte((UCHAR)(Nibble < 10 ? ('0' + Nibble) :
                                               ('A' + Nibble - 10)));
    }
}

static
VOID
NTAPI
KxRawCom1WriteField(
    _In_z_ const CHAR *Name,
    _In_ ULONG_PTR Value)
{
    KxRawCom1WriteByte(' ');
    KxRawCom1WriteString(Name);
    KxRawCom1WriteByte('=');
    KxRawCom1WriteHex(Value);
}

static
VOID
NTAPI
KxRawCom1DumpFreeze(
    _In_ ULONG Stage,
    _In_ ULONG_PTR Caller)
{
    PKPRCB CurrentPrcb = KeGetCurrentPrcb();

    KxRawCom1WriteString("\nKxRawFreeze");
    KxRawCom1WriteField("stage", Stage);
    KxRawCom1WriteField("cpu", KeGetCurrentProcessorNumber());
    KxRawCom1WriteField("irql", KeGetCurrentIrql());
    KxRawCom1WriteField("owner", (ULONG_PTR)KiI386FreezeOwner);
    KxRawCom1WriteField("target", KiI386FreezeTargetSet);
    KxRawCom1WriteField("frozen", KiI386FrozenSet);
    KxRawCom1WriteField("thaw", KiI386ThawSet);
    KxRawCom1WriteField("ready", KiI386FreezeReadySet);
    KxRawCom1WriteField("set", CurrentPrcb->SetMember);
    KxRawCom1WriteField("ipi", CurrentPrcb->IpiFrozen);
    KxRawCom1WriteField("active", KeActiveProcessors);
    KxRawCom1WriteField("flag", KiFreezeFlag);
    KxRawCom1WriteField("caller", Caller);
    KxRawCom1WriteField("kd", KdEnteredDebugger);
    KxRawCom1WriteField("bugcnt", KeBugCheckCount);
    KxRawCom1WriteField("bugown", KeBugCheckOwner);
    KxRawCom1WriteField("bugrec", KeBugCheckOwnerRecursionCount);
    KxRawCom1WriteByte('\n');
}
#endif

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
KxMarkProcessorFreezeReady(
    VOID)
{
    PKPRCB CurrentPrcb = KeGetCurrentPrcb();

    /*
     * Debugger freeze IPIs require processors that can accept IPI_LEVEL
     * vectors.  AP startup enters KeActiveProcessors before the idle-loop
     * handoff, so the freeze target set uses a separate readiness mask.
     */
    InterlockedBitTestAndSetAffinity(&KiI386FreezeReadySet,
                                     CurrentPrcb->Number);
}

static
BOOLEAN
KxWaitForFrozenProcessors(
    _In_ KAFFINITY TargetSet)
{
    ULONG SpinCount = 10000000;

    while ((KiI386FrozenSet & TargetSet) != TargetSet)
    {
        if (--SpinCount == 0)
            return FALSE;

        YieldProcessor();
        KeMemoryBarrier();
    }

    return TRUE;
}

static
BOOLEAN
KxJoinFreezeTarget(
    _In_ PKPRCB CurrentPrcb)
{
    KAFFINITY SetMember = CurrentPrcb->SetMember;

    if ((KiI386FreezeTargetSet & SetMember) == 0)
        return FALSE;

    InterlockedBitTestAndReset((PLONG)&CurrentPrcb->IpiFrozen, IPI_FREEZE);
    InterlockedBitTestAndSetAffinity(&KiI386FrozenSet, CurrentPrcb->Number);

    while ((KiI386ThawSet & SetMember) == 0)
    {
        YieldProcessor();
        KeMemoryBarrier();
    }

    InterlockedBitTestAndResetAffinity(&KiI386FrozenSet, CurrentPrcb->Number);
    return TRUE;
}

static
BOOLEAN
KxWaitAndJoinExistingFreeze(
    _In_ PKPRCB CurrentPrcb)
{
    KAFFINITY SetMember = CurrentPrcb->SetMember;
    ULONG SpinCount = 10000000;

    while ((KiI386FreezeTargetSet & SetMember) == 0)
    {
        if (KiI386FreezeOwner == NULL)
            return TRUE;

        if (--SpinCount == 0)
            return FALSE;

        YieldProcessor();
        KeMemoryBarrier();
    }

    return KxJoinFreezeTarget(CurrentPrcb);
}

VOID
NTAPI
KxHandleFreezeIpi(
    VOID)
{
    KxJoinFreezeTarget(KeGetCurrentPrcb());
}

BOOLEAN
NTAPI
KxFreezeExecution(
    VOID)
{
    PKPRCB CurrentPrcb = KeGetCurrentPrcb();
    KAFFINITY TargetSet;
#if DBG && defined(_M_IX86)
    ULONG_PTR Caller = (ULONG_PTR)__builtin_return_address(0);
#endif

    if (KiI386FreezeOwner == CurrentPrcb)
    {
#if DBG && defined(_M_IX86)
        KxRawCom1DumpFreeze(0x80, Caller);
#endif
        return FALSE;
    }

    for (;;)
    {
        if (InterlockedCompareExchangePointer((PVOID *)&KiI386FreezeOwner,
                                              CurrentPrcb,
                                              NULL) == NULL)
            break;

        if (KxWaitAndJoinExistingFreeze(CurrentPrcb))
            continue;

        KiFreezeFlag |= 2;
#if DBG && defined(_M_IX86)
        KxRawCom1DumpFreeze(0x81, Caller);
#endif
        return FALSE;
    }

    TargetSet = (KeActiveProcessors & KiI386FreezeReadySet) &
                ~CurrentPrcb->SetMember;
    if (TargetSet == 0)
    {
        return TRUE;
    }

    InterlockedExchange((PLONG)&KiI386FrozenSet, 0);
    InterlockedExchange((PLONG)&KiI386ThawSet, 0);
    InterlockedExchange((PLONG)&KiI386FreezeTargetSet, TargetSet);
    KeMemoryBarrier();

    KiIpiSend(TargetSet, IPI_FREEZE);

    if (!KxWaitForFrozenProcessors(TargetSet))
    {
        KiFreezeFlag |= 2;
#if DBG && defined(_M_IX86)
        KxRawCom1DumpFreeze(0x83, Caller);
#endif
    }

    return TRUE;
}

VOID
NTAPI
KxThawExecution(
    VOID)
{
    PKPRCB CurrentPrcb = KeGetCurrentPrcb();
    KAFFINITY TargetSet;
#if DBG && defined(_M_IX86)
    ULONG_PTR Caller = (ULONG_PTR)__builtin_return_address(0);
#endif

    if (KiI386FreezeOwner != CurrentPrcb)
    {
#if DBG && defined(_M_IX86)
        if (KiI386FreezeOwner != NULL)
        {
            KxRawCom1DumpFreeze(0x84, Caller);
        }
#endif
        return;
    }

    TargetSet = KiI386FreezeTargetSet;
    if (TargetSet != 0)
    {
        InterlockedExchange((PLONG)&KiI386ThawSet, TargetSet);
        KeMemoryBarrier();

        while ((KiI386FrozenSet & TargetSet) != 0)
        {
            YieldProcessor();
            KeMemoryBarrier();
        }
    }

    InterlockedExchange((PLONG)&KiI386FreezeTargetSet, 0);
    InterlockedExchange((PLONG)&KiI386ThawSet, 0);
    InterlockedExchangePointer((PVOID *)&KiI386FreezeOwner, NULL);
}

KCONTINUE_STATUS
NTAPI
KxSwitchKdProcessor(
    _In_ ULONG ProcessorIndex)
{
    UNREFERENCED_PARAMETER(ProcessorIndex);

    return ContinueError;
}

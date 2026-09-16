/*
 * PROJECT:     ReactOS HAL
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Interrupt translators of the legacy PC HALs
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#include <smp.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

#define HALP_ISA_LINE_BIT(Line)     (1UL << (Line))

/* PRIVATE FUNCTIONS **********************************************************/

/* The translators have no state, so there is nothing to count */
static
VOID
NTAPI
HalpIrqTranslatorNoReference(
    _In_ PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);
}

static
ULONG
NTAPI
HalpLineToVector(
    _In_ ULONG Line)
{
    KAFFINITY Affinity;
    KIRQL Irql;

    return HalpGetRootInterruptVector(Line, Line, &Irql, &Affinity);
}

/* Returns the line a vector was mapped from, without mapping any line */
static
BOOLEAN
NTAPI
HalpVectorToLine(
    _In_ ULONG Vector,
    _Out_ PULONG Line)
{
    UCHAR Irq;

    if (Vector > MAXUCHAR)
        return FALSE;

    /* The APIC HAL only knows the lines that already have a vector */
    Irq = HalpVectorToIrq((UCHAR)Vector);
    if (HalpIrqToVector(Irq) != (UCHAR)Vector)
        return FALSE;

    /* Also rejects values that are not lines. A line with a vector is not mapped again */
    if (HalpLineToVector(Irq) != Vector)
        return FALSE;

    *Line = Irq;
    return TRUE;
}

/* Maps an assigned line to its vector, and a vector back to its line */
static
CODE_SEG("PAGE")
NTSTATUS
NTAPI
HalpLegacyTranslateVector(
    _Inout_opt_ PVOID Context,
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    _In_ RESOURCE_TRANSLATION_DIRECTION Direction,
    _In_opt_ ULONG AlternativesCount,
    _In_reads_opt_(AlternativesCount) IO_RESOURCE_DESCRIPTOR Alternatives[],
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Target)
{
    KAFFINITY Affinity = 0;
    KIRQL Irql = 0;
    ULONG Vector, Line;

    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(AlternativesCount);
    UNREFERENCED_PARAMETER(Alternatives);
    UNREFERENCED_PARAMETER(PhysicalDeviceObject);

    PAGED_CODE();

    *Target = *Source;

    switch (Direction)
    {
        case TranslateChildToParent:
            Vector = HalpGetRootInterruptVector(Source->u.Interrupt.Level,
                                                Source->u.Interrupt.Vector,
                                                &Irql,
                                                &Affinity);
            if (Vector == 0)
                return STATUS_UNSUCCESSFUL;

            Target->u.Interrupt.Level = Irql;
            Target->u.Interrupt.Vector = Vector;
            Target->u.Interrupt.Affinity = Affinity;
            return STATUS_TRANSLATION_COMPLETE;

        case TranslateParentToChild:
            if (!HalpVectorToLine(Source->u.Interrupt.Vector, &Line))
                return STATUS_UNSUCCESSFUL;

            Target->u.Interrupt.Level = Line;
            Target->u.Interrupt.Vector = Line;
            Target->u.Interrupt.Affinity = (KAFFINITY)-1;
            return STATUS_SUCCESS;

        default:
            return STATUS_INVALID_PARAMETER;
    }
}

static
CODE_SEG("PAGE")
NTSTATUS
NTAPI
HalpLegacyTranslateVectorRange(
    _Inout_opt_ PVOID Context,
    _In_ PIO_RESOURCE_DESCRIPTOR Source,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _Out_ PULONG TargetCount,
    _Out_ PIO_RESOURCE_DESCRIPTOR *Target)
{
    PIO_RESOURCE_DESCRIPTOR Output;
    ULONG First, Last;

    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(PhysicalDeviceObject);

    PAGED_CODE();

    *TargetCount = 0;
    *Target = NULL;

    /* A range with an end that has no vector drops out, which is not an error */
    First = HalpLineToVector(Source->u.Interrupt.MinimumVector);
    Last = HalpLineToVector(Source->u.Interrupt.MaximumVector);
    if ((First == 0) || (Last == 0))
        return STATUS_TRANSLATION_COMPLETE;

    Output = ExAllocatePoolWithTag(PagedPool, sizeof(*Output), TAG_HAL);
    if (Output == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    *Output = *Source;
    Output->u.Interrupt.MinimumVector = First;
    Output->u.Interrupt.MaximumVector = Last;

    *TargetCount = 1;
    *Target = Output;
    return STATUS_TRANSLATION_COMPLETE;
}

/* Checks if IRQ 9 stands for IRQ 2, which is when a device takes IRQ 2 but not IRQ 9 */
static
BOOLEAN
NTAPI
HalpDeviceWantsCascadeLine(
    _In_ ULONG AlternativesCount,
    _In_reads_opt_(AlternativesCount) IO_RESOURCE_DESCRIPTOR Alternatives[])
{
    BOOLEAN TakesCascade = FALSE;
    ULONG Index;

    if (Alternatives == NULL)
        return FALSE;

    for (Index = 0; Index < AlternativesCount; Index++)
    {
        ULONG Minimum = Alternatives[Index].u.Interrupt.MinimumVector;
        ULONG Maximum = Alternatives[Index].u.Interrupt.MaximumVector;

        /* Only an alternative naming the one line counts */
        if (Minimum != Maximum)
            continue;

        if (Minimum == PIC_CASCADE_REDIRECT_IRQ)
            return FALSE;

        if (Minimum == PIC_CASCADE_IRQ)
            TakesCascade = TRUE;
    }

    return TakesCascade;
}

static
CODE_SEG("PAGE")
NTSTATUS
NTAPI
HalpLegacyTranslateIsaLine(
    _Inout_opt_ PVOID Context,
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    _In_ RESOURCE_TRANSLATION_DIRECTION Direction,
    _In_opt_ ULONG AlternativesCount,
    _In_reads_opt_(AlternativesCount) IO_RESOURCE_DESCRIPTOR Alternatives[],
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Target)
{
    CM_PARTIAL_RESOURCE_DESCRIPTOR Line = *Source;
    NTSTATUS Status;

    PAGED_CODE();

    if ((Direction == TranslateChildToParent) &&
        (Line.u.Interrupt.Vector == PIC_CASCADE_IRQ))
    {
        Line.u.Interrupt.Level = PIC_CASCADE_REDIRECT_IRQ;
        Line.u.Interrupt.Vector = PIC_CASCADE_REDIRECT_IRQ;
    }

    Status = HalpLegacyTranslateVector(Context,
                                       &Line,
                                       Direction,
                                       AlternativesCount,
                                       Alternatives,
                                       PhysicalDeviceObject,
                                       Target);
    if (!NT_SUCCESS(Status))
        return Status;

    if ((Direction == TranslateParentToChild) &&
        (Target->u.Interrupt.Level == PIC_CASCADE_REDIRECT_IRQ) &&
        HalpDeviceWantsCascadeLine(AlternativesCount, Alternatives))
    {
        Target->u.Interrupt.Level = PIC_CASCADE_IRQ;
        Target->u.Interrupt.Vector = PIC_CASCADE_IRQ;
    }

    return Status;
}

/* Trigger modes the ELCR reports, read once on EISA machines */
static BOOLEAN HalpEisaTriggersRead;
static ULONG HalpEisaLevelLines;

/* IRQ 12 is never checked against the ELCR */
static ULONG HalpEisaUncheckedLines = HALP_ISA_LINE_BIT(12);

static
VOID
NTAPI
HalpReadEisaTriggers(VOID)
{
    if (HalpEisaTriggersRead)
        return;

    HalpEisaLevelLines = READ_PORT_UCHAR((PUCHAR)EISA_ELCR_MASTER) |
                         (READ_PORT_UCHAR((PUCHAR)EISA_ELCR_SLAVE) << 8);

    /* All clear or all set means there is no usable ELCR */
    if ((HalpEisaLevelLines == 0) || (HalpEisaLevelLines == 0xFFFF))
        HalpEisaUncheckedLines = 0xFFFF;

    HalpEisaTriggersRead = TRUE;
}

/* Returns a mask with the bits First to Last set, none when Last is below First */
static
ULONG
NTAPI
HalpLineRangeMask(
    _In_ ULONG First,
    _In_ ULONG Last)
{
    return (HALP_ISA_LINE_BIT(Last) | (HALP_ISA_LINE_BIT(Last) - 1)) &
           ~(HALP_ISA_LINE_BIT(First) - 1);
}

/**
 * @brief
 * Returns the ISA lines a requirement cannot use.
 *
 * @param[in] Latched
 * TRUE for an edge triggered requirement.
 *
 * @param[out] MismatchLines
 * Receives the lines the ELCR gives the other trigger mode.
 */
static
ULONG
NTAPI
HalpGetExcludedIsaLines(
    _In_ BOOLEAN Latched,
    _Out_ PULONG MismatchLines)
{
    ULONG Excluded = 0;

    *MismatchLines = 0;
    if (HalpBusType != MACHINE_TYPE_EISA)
        return HalpPciIrqMask;

    HalpReadEisaTriggers();

    /* A PCI line is only shared with a level triggered EISA device */
    if (Latched)
        Excluded = HalpPciIrqMask;

    *MismatchLines = Latched ? HalpEisaLevelLines : ~HalpEisaLevelLines;
    *MismatchLines &= ~HalpEisaUncheckedLines & 0xFFFF;

    return Excluded | *MismatchLines;
}

/* Adds the vectors of a line range as an alternative, unless an end has no vector */
static
ULONG
NTAPI
HalpAddVectorAlternative(
    _In_ PIO_RESOURCE_DESCRIPTOR Source,
    _In_ ULONG FirstLine,
    _In_ ULONG LastLine,
    _Inout_ PIO_RESOURCE_DESCRIPTOR Output,
    _In_ ULONG Count)
{
    ULONG First, Last;

    First = HalpLineToVector(FirstLine);
    Last = HalpLineToVector(LastLine);
    if ((First == 0) || (Last == 0))
        return Count;

    Output[Count] = *Source;
    if (Count > 0)
        Output[Count].Option = IO_RESOURCE_ALTERNATIVE;

    Output[Count].u.Interrupt.MinimumVector = First;
    Output[Count].u.Interrupt.MaximumVector = Last;
    return Count + 1;
}

/* At most every other ISA line starts a run, plus one range above the ISA lines */
#define HALP_ISA_MAX_RUNS       (HALP_ISA_IRQ_COUNT / 2 + 1)

/**
 * @brief
 * Translates an ISA interrupt requirement into vector ranges. IRQ 2 is replaced
 * by IRQ 9, lines ISA cannot use are excluded, and each run of remaining lines
 * becomes one alternative.
 */
static
CODE_SEG("PAGE")
NTSTATUS
NTAPI
HalpLegacyTranslateIsaRange(
    _Inout_opt_ PVOID Context,
    _In_ PIO_RESOURCE_DESCRIPTOR Source,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _Out_ PULONG TargetCount,
    _Out_ PIO_RESOURCE_DESCRIPTOR *Target)
{
    PIO_RESOURCE_DESCRIPTOR Output;
    ULONG Minimum, Maximum, LineMask, MismatchLines, Starts, Ends, First, Last, Count;
    ULONG AboveIsaLast = 0;
    BOOLEAN Latched;

    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(PhysicalDeviceObject);

    PAGED_CODE();

    *TargetCount = 0;
    *Target = NULL;

    Minimum = Source->u.Interrupt.MinimumVector;
    Maximum = Source->u.Interrupt.MaximumVector;

    /* Lines above the ISA lines are never excluded */
    LineMask = 0;
    if (Minimum < HALP_ISA_IRQ_COUNT)
        LineMask = HalpLineRangeMask(Minimum, min(Maximum, HALP_ISA_IRQ_COUNT - 1));

    if ((Maximum >= HALP_ISA_IRQ_COUNT) && (Maximum >= Minimum))
        AboveIsaLast = Maximum;

    /* IRQ 2 is never assigned, IRQ 9 is offered in its place */
    if (LineMask & HALP_ISA_LINE_BIT(PIC_CASCADE_IRQ))
    {
        LineMask &= ~HALP_ISA_LINE_BIT(PIC_CASCADE_IRQ);
        LineMask |= HALP_ISA_LINE_BIT(PIC_CASCADE_REDIRECT_IRQ);
    }

    Latched = (Source->Flags & CM_RESOURCE_INTERRUPT_LATCHED) != 0;
    LineMask &= ~HalpGetExcludedIsaLines(Latched, &MismatchLines);

    Output = ExAllocatePoolWithTag(PagedPool, HALP_ISA_MAX_RUNS * sizeof(*Output), TAG_HAL);
    if (Output == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* A run reaching the last ISA line carries on into the lines above it */
    Count = 0;
    Starts = LineMask & ~(LineMask << 1);
    Ends = LineMask & ~(LineMask >> 1);
    while (Starts != 0)
    {
        BitScanForward(&First, Starts);
        BitScanForward(&Last, Ends);
        Starts &= Starts - 1;
        Ends &= Ends - 1;

        if ((Last == HALP_ISA_IRQ_COUNT - 1) && (AboveIsaLast != 0))
        {
            Last = AboveIsaLast;
            AboveIsaLast = 0;
        }

        Count = HalpAddVectorAlternative(Source, First, Last, Output, Count);
    }

    if (AboveIsaLast != 0)
    {
        Count = HalpAddVectorAlternative(Source,
                                         max(Minimum, HALP_ISA_IRQ_COUNT),
                                         AboveIsaLast,
                                         Output,
                                         Count);
    }

    if (Count == 0)
    {
        ExFreePoolWithTag(Output, TAG_HAL);
        return MismatchLines ? STATUS_PNP_IRQ_TRANSLATION_FAILED : STATUS_TRANSLATION_COMPLETE;
    }

    *TargetCount = Count;
    *Target = Output;
    return STATUS_TRANSLATION_COMPLETE;
}

static
VOID
NTAPI
HalpFillIrqTranslator(
    _Out_ PTRANSLATOR_INTERFACE Translator,
    _In_ PTRANSLATE_RESOURCE_HANDLER TranslateResources,
    _In_ PTRANSLATE_RESOURCE_REQUIREMENTS_HANDLER TranslateRequirements)
{
    RtlZeroMemory(Translator, sizeof(*Translator));
    Translator->Size = sizeof(*Translator);
    Translator->Version = HAL_IRQ_TRANSLATOR_VERSION;
    Translator->InterfaceReference = HalpIrqTranslatorNoReference;
    Translator->InterfaceDereference = HalpIrqTranslatorNoReference;
    Translator->TranslateResources = TranslateResources;
    Translator->TranslateResourceRequirements = TranslateRequirements;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * Checks if interrupt lines are arbitrated on the HAL bus FDO, before they
 * are translated to vectors. Otherwise the bridges translate lines to vectors
 * and the vectors are arbitrated at the root.
 */
BOOLEAN
NTAPI
HalpLegacyPCArbitratesIrqs(VOID)
{
    /* FIXME: this stands for a $PIR routing table until the table is parsed.
       The APIC HAL has no fixed line to vector mapping, so it never arbitrates lines */
    return (HalpInterruptControllerType == HALP_INTERRUPT_CONTROLLER_PIC);
}

/**
 * @brief
 * Returns the interrupt translator of the HAL bus FDO, which maps the lines
 * its IRQ arbiter assigns to vectors. Only offered while lines are arbitrated
 * on the FDO.
 *
 * @param[out] Interface
 * Receives the TRANSLATOR_INTERFACE.
 *
 * @param[in] Size
 * Size of the Interface buffer, in bytes.
 *
 * @param[out] Length
 * Receives the size of a TRANSLATOR_INTERFACE.
 *
 * @return
 * STATUS_SUCCESS, STATUS_NOT_SUPPORTED without line arbitration, or
 * STATUS_BUFFER_TOO_SMALL if the buffer is too small.
 */
CODE_SEG("PAGE")
NTSTATUS
NTAPI
HalpLegacyPCQueryIrqTranslator(
    _Out_writes_bytes_(Size) PVOID Interface,
    _In_ ULONG Size,
    _Out_ PULONG Length)
{
    PTRANSLATOR_INTERFACE Translator = Interface;

    PAGED_CODE();

    *Length = 0;
    if (!HalpLegacyPCArbitratesIrqs())
        return STATUS_NOT_SUPPORTED;

    *Length = sizeof(*Translator);
    if (Size < sizeof(*Translator))
        return STATUS_BUFFER_TOO_SMALL;

    HalpFillIrqTranslator(Translator,
                          HalpLegacyTranslateVector,
                          HalpLegacyTranslateVectorRange);
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Returns the interrupt translator of a bridge. Bridges have none while lines
 * are arbitrated on the HAL bus FDO. Otherwise ISA, EISA and undefined bridges
 * get the ISA translator, and MCA and PCI bridges the vector translator.
 *
 * @param[in] BridgeInterfaceType
 * Bus type below the bridge.
 *
 * @param[in] Size
 * Size of the Translator buffer, in bytes.
 *
 * @param[out] Translator
 * Receives the translator interface.
 *
 * @return
 * STATUS_SUCCESS, STATUS_BUFFER_TOO_SMALL, or STATUS_NOT_SUPPORTED if the
 * bridge has no translator.
 */
NTSTATUS
NTAPI
HalpGetInterruptTranslator(
    _In_ INTERFACE_TYPE ParentInterfaceType,
    _In_ ULONG ParentBusNumber,
    _In_ INTERFACE_TYPE BridgeInterfaceType,
    _In_ USHORT Size,
    _In_ USHORT Version,
    _Out_ PTRANSLATOR_INTERFACE Translator,
    _Out_ PULONG BridgeBusNumber)
{
    UNREFERENCED_PARAMETER(ParentInterfaceType);
    UNREFERENCED_PARAMETER(ParentBusNumber);
    UNREFERENCED_PARAMETER(Version);
    UNREFERENCED_PARAMETER(BridgeBusNumber);

    if (HalpLegacyPCArbitratesIrqs())
        return STATUS_NOT_SUPPORTED;

    if (Size < sizeof(*Translator))
        return STATUS_BUFFER_TOO_SMALL;

    switch (BridgeInterfaceType)
    {
        case InterfaceTypeUndefined:
        case Isa:
        case Eisa:
            HalpFillIrqTranslator(Translator,
                                  HalpLegacyTranslateIsaLine,
                                  HalpLegacyTranslateIsaRange);
            return STATUS_SUCCESS;

        case MicroChannel:
        case PCIBus:
            HalpFillIrqTranslator(Translator,
                                  HalpLegacyTranslateVector,
                                  HalpLegacyTranslateVectorRange);
            return STATUS_SUCCESS;

        default:
            return STATUS_NOT_SUPPORTED;
    }
}

/* EOF */

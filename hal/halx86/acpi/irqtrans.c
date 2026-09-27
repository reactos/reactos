/*
 * PROJECT:     ReactOS Hardware Abstraction Layer
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ISA interrupt translation for the ACPI HALs
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#include <smp.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* ISA IRQ to global system interrupt mapping, defined in acpi/halacpi.c */
extern ULONG HalpIsaIrqToGsi[HALP_ISA_IRQ_COUNT];

/* Interrupt values this high are ACPI link node tokens, not ISA IRQs */
#define HALP_ISA_TOKEN_BASE     0xFFFF0000

#define HALP_IRQ_BIT(Irq)       (1UL << (Irq))

/* PRIVATE FUNCTIONS **********************************************************/

static
BOOLEAN
NTAPI
HalpIsInterruptToken(
    _In_ ULONG Value)
{
    return (Value >= HALP_ISA_TOKEN_BASE) && (Value != MAXULONG);
}

static
NTSTATUS
NTAPI
HalpGlobalInterruptToIsaIrq(
    _In_ ULONG GlobalInterrupt,
    _Out_ PULONG Irq)
{
    ULONG Index;

    for (Index = 0; Index < HALP_ISA_IRQ_COUNT; Index++)
    {
        if (HalpIsaIrqToGsi[Index] == GlobalInterrupt)
        {
            *Irq = Index;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_FOUND;
}

/* Returns a mask with the bits First to Last set */
static
ULONG
NTAPI
HalpIrqRangeMask(
    _In_ ULONG First,
    _In_ ULONG Last)
{
    return (HALP_IRQ_BIT(Last) | (HALP_IRQ_BIT(Last) - 1)) & ~(HALP_IRQ_BIT(First) - 1);
}

/* Returns the IRQs whose global interrupt is followed by the one of the next IRQ */
static
ULONG
NTAPI
HalpGetLinkedIrqMask(VOID)
{
    ULONG Irq, LinkedMask = 0;

    for (Irq = 0; Irq < HALP_ISA_IRQ_COUNT - 1; Irq++)
    {
        if (HalpIsaIrqToGsi[Irq + 1] == HalpIsaIrqToGsi[Irq] + 1)
            LinkedMask |= HALP_IRQ_BIT(Irq);
    }

    return LinkedMask;
}

/* Returns the block of contiguous set bits in IrqMask that holds Irq */
static
ULONG
NTAPI
HalpGetIrqBlock(
    _In_ ULONG IrqMask,
    _In_ ULONG Irq)
{
    ULONG Low = Irq, High = Irq;

    while ((Low > 0) && (IrqMask & HALP_IRQ_BIT(Low - 1)))
        Low--;

    while ((High < HALP_ISA_IRQ_COUNT - 1) && (IrqMask & HALP_IRQ_BIT(High + 1)))
        High++;

    return HalpIrqRangeMask(Low, High);
}

/**
 * @brief
 * Describes each run of IRQs in IrqMask with consecutive global interrupts as
 * one alternative.
 *
 * @param[in] IrqMask
 * The IRQs to describe.
 *
 * @param[in] LinkedMask
 * The mask returned by HalpGetLinkedIrqMask.
 *
 * @param[in] Source
 * The requirement the alternatives are built from.
 *
 * @param[out] Output
 * Receives the alternatives, or NULL to only count them.
 *
 * @param[in] Count
 * Number of alternatives already in Output.
 *
 * @return
 * The new number of alternatives.
 */
static
ULONG
NTAPI
HalpDescribeIrqRuns(
    _In_ ULONG IrqMask,
    _In_ ULONG LinkedMask,
    _In_ PIO_RESOURCE_DESCRIPTOR Source,
    _Out_opt_ PIO_RESOURCE_DESCRIPTOR Output,
    _In_ ULONG Count)
{
    ULONG Joined, Starts, Ends, First, Last;

    /* Bits whose run carries on into the next bit */
    Joined = IrqMask & LinkedMask & (IrqMask >> 1);
    Starts = IrqMask & ~(Joined << 1);
    Ends = IrqMask & ~Joined;

    while (Starts != 0)
    {
        BitScanForward(&First, Starts);
        BitScanForward(&Last, Ends);
        Starts &= Starts - 1;
        Ends &= Ends - 1;

        if (Output != NULL)
        {
            Output[Count] = *Source;
            if (Count > 0)
                Output[Count].Option = IO_RESOURCE_ALTERNATIVE;

            Output[Count].u.Interrupt.MinimumVector = HalpIsaIrqToGsi[First];
            Output[Count].u.Interrupt.MaximumVector = HalpIsaIrqToGsi[Last];
        }

        Count++;
    }

    return Count;
}

/* Only an alternative naming the one IRQ counts, not a range around it */
static
BOOLEAN
NTAPI
HalpRangeIsOnlyIrq(
    _In_ PIO_RESOURCE_DESCRIPTOR Descriptor,
    _In_ ULONG Irq)
{
    return (Descriptor->u.Interrupt.MinimumVector == Irq) &&
           (Descriptor->u.Interrupt.MaximumVector == Irq);
}

/**
 * @brief
 * Translates an ISA interrupt requirement into global system interrupts. IRQ 2
 * is replaced by IRQ 9, the SCI IRQ is excluded, and each run of consecutive
 * global interrupts becomes one alternative.
 *
 * @param[in] Source
 * The requirement to translate.
 *
 * @param[out] TargetCount
 * Receives the number of descriptors written.
 *
 * @param[out] Target
 * Receives the translated descriptors.
 */
static
NTSTATUS
NTAPI
HalpTranslateIsaInterruptRequirement(
    _Inout_opt_ PVOID Context,
    _In_ PIO_RESOURCE_DESCRIPTOR Source,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _Out_ PULONG TargetCount,
    _Out_ PIO_RESOURCE_DESCRIPTOR *Target)
{
    PIO_RESOURCE_DESCRIPTOR Output;
    ULONG Minimum, Maximum, Sci, IrqMask, NearSciMask, LinkedMask, Count;

    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(PhysicalDeviceObject);

    PAGED_CODE();

    Minimum = Source->u.Interrupt.MinimumVector;
    Maximum = Source->u.Interrupt.MaximumVector;

    /* Link node tokens are passed through untranslated */
    if (HalpIsInterruptToken(Minimum))
    {
        Output = ExAllocatePoolWithTag(PagedPool, sizeof(*Output), TAG_HAL);
        if (Output == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        *Output = *Source;
        *Target = Output;
        *TargetCount = 1;
        return STATUS_SUCCESS;
    }

    if ((Minimum >= HALP_ISA_IRQ_COUNT) ||
        (Maximum >= HALP_ISA_IRQ_COUNT) ||
        (Maximum < Minimum))
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* IRQ 2 is never assigned, IRQ 9 is offered in its place */
    IrqMask = HalpIrqRangeMask(Minimum, Maximum);
    if (IrqMask & HALP_IRQ_BIT(PIC_CASCADE_IRQ))
    {
        IrqMask &= ~HALP_IRQ_BIT(PIC_CASCADE_IRQ);
        IrqMask |= HALP_IRQ_BIT(PIC_CASCADE_REDIRECT_IRQ);
    }

    /* The SCI is never offered, and the IRQs next to it are offered last */
    NearSciMask = 0;
    Sci = HalpFixedAcpiDescTable.sci_int_vector;
    if ((Sci < HALP_ISA_IRQ_COUNT) && (IrqMask & HALP_IRQ_BIT(Sci)))
    {
        NearSciMask = HalpGetIrqBlock(IrqMask, Sci) & ~HALP_IRQ_BIT(Sci);
        IrqMask &= ~(NearSciMask | HALP_IRQ_BIT(Sci));
    }

    LinkedMask = HalpGetLinkedIrqMask();
    Count = HalpDescribeIrqRuns(IrqMask, LinkedMask, Source, NULL, 0);
    Count = HalpDescribeIrqRuns(NearSciMask, LinkedMask, Source, NULL, Count);

    *TargetCount = Count;
    if (Count == 0)
    {
        *Target = NULL;
        return STATUS_SUCCESS;
    }

    Output = ExAllocatePoolWithTag(PagedPool, Count * sizeof(*Output), TAG_HAL);
    if (Output == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Count = HalpDescribeIrqRuns(IrqMask, LinkedMask, Source, Output, 0);
    HalpDescribeIrqRuns(NearSciMask, LinkedMask, Source, Output, Count);

    *Target = Output;
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Tells whether an IRQ 9 assignment really stands for the cascaded IRQ 2. It
 * does when a requirement allows IRQ 2 and none allows IRQ 9 ahead of it.
 *
 * @param[in] AlternativesCount
 * Number of alternatives the assignment was picked from.
 *
 * @param[in] Alternatives
 * The alternatives the assignment was picked from.
 */
static
BOOLEAN
NTAPI
HalpIsCascadeAssignment(
    _In_ ULONG AlternativesCount,
    _In_reads_opt_(AlternativesCount) IO_RESOURCE_DESCRIPTOR Alternatives[])
{
    BOOLEAN AllowsCascade = FALSE;
    ULONG Index;

    for (Index = 0; Index < AlternativesCount; Index++)
    {
        if (HalpRangeIsOnlyIrq(&Alternatives[Index], PIC_CASCADE_REDIRECT_IRQ))
            return FALSE;

        if (HalpRangeIsOnlyIrq(&Alternatives[Index], PIC_CASCADE_IRQ))
            AllowsCascade = TRUE;
    }

    return AllowsCascade;
}

/* Carries an assigned ISA interrupt between IRQs and global interrupts */
static
NTSTATUS
NTAPI
HalpTranslateIsaInterruptResource(
    _Inout_opt_ PVOID Context,
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    _In_ RESOURCE_TRANSLATION_DIRECTION Direction,
    _In_opt_ ULONG AlternativesCount,
    _In_reads_opt_(AlternativesCount) IO_RESOURCE_DESCRIPTOR Alternatives[],
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Target)
{
    ULONG Level, Vector, Irq;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(PhysicalDeviceObject);

    PAGED_CODE();

    *Target = *Source;

    Level = Source->u.Interrupt.Level;
    Vector = Source->u.Interrupt.Vector;
    if (HalpIsInterruptToken(Vector))
        return STATUS_SUCCESS;

    switch (Direction)
    {
        case TranslateChildToParent:
            if ((Level >= HALP_ISA_IRQ_COUNT) || (Vector >= HALP_ISA_IRQ_COUNT))
                return STATUS_UNSUCCESSFUL;

            Target->u.Interrupt.Level = HalpIsaIrqToGsi[Level];
            Target->u.Interrupt.Vector = HalpIsaIrqToGsi[Vector];
            return STATUS_SUCCESS;

        case TranslateParentToChild:
            break;

        default:
            return STATUS_SUCCESS;
    }

    Status = HalpGlobalInterruptToIsaIrq(Level, &Irq);
    if (!NT_SUCCESS(Status))
        return Status;
    Target->u.Interrupt.Level = Irq;

    Status = HalpGlobalInterruptToIsaIrq(Vector, &Irq);
    if (!NT_SUCCESS(Status))
        return Status;
    Target->u.Interrupt.Vector = Irq;

    if ((Target->u.Interrupt.Level == PIC_CASCADE_REDIRECT_IRQ) &&
        HalpIsCascadeAssignment(AlternativesCount, Alternatives))
    {
        Target->u.Interrupt.Level = PIC_CASCADE_IRQ;
        Target->u.Interrupt.Vector = PIC_CASCADE_IRQ;
    }

    return STATUS_SUCCESS;
}

/* The translator has no state, so references are not counted */
static
VOID
NTAPI
HalpIsaTranslatorReference(
    _In_ PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);
}

/* Maps a line of the PIC HAL to its vector, or a vector back to its line */
static
CODE_SEG("PAGE")
NTSTATUS
NTAPI
HalpTranslatePicLine(
    _Inout_opt_ PVOID Context,
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    _In_ RESOURCE_TRANSLATION_DIRECTION Direction,
    _In_opt_ ULONG AlternativesCount,
    _In_reads_opt_(AlternativesCount) IO_RESOURCE_DESCRIPTOR Alternatives[],
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Target)
{
    KAFFINITY Affinity;
    KIRQL Irql;
    ULONG Vector;
    UCHAR Line;

    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(AlternativesCount);
    UNREFERENCED_PARAMETER(Alternatives);
    UNREFERENCED_PARAMETER(PhysicalDeviceObject);

    PAGED_CODE();

    *Target = *Source;

    if (Direction == TranslateChildToParent)
    {
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
    }

    if (Direction == TranslateParentToChild)
    {
        if (Source->u.Interrupt.Vector > MAXUCHAR)
            return STATUS_UNSUCCESSFUL;

        Line = HalpVectorToIrq((UCHAR)Source->u.Interrupt.Vector);
        if (Line >= HALP_ISA_IRQ_COUNT)
            return STATUS_UNSUCCESSFUL;

        Target->u.Interrupt.Level = Line;
        Target->u.Interrupt.Vector = Line;
        Target->u.Interrupt.Affinity = (KAFFINITY)-1;
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

static
CODE_SEG("PAGE")
NTSTATUS
NTAPI
HalpTranslatePicLineRequirement(
    _Inout_opt_ PVOID Context,
    _In_ PIO_RESOURCE_DESCRIPTOR Source,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _Out_ PULONG TargetCount,
    _Out_ PIO_RESOURCE_DESCRIPTOR *Target)
{
    PIO_RESOURCE_DESCRIPTOR Output;
    KAFFINITY Affinity;
    KIRQL Irql;
    ULONG First, Last;

    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(PhysicalDeviceObject);

    PAGED_CODE();

    *TargetCount = 0;
    *Target = NULL;

    First = HalpGetRootInterruptVector(Source->u.Interrupt.MinimumVector,
                                       Source->u.Interrupt.MinimumVector,
                                       &Irql,
                                       &Affinity);
    Last = HalpGetRootInterruptVector(Source->u.Interrupt.MaximumVector,
                                      Source->u.Interrupt.MaximumVector,
                                      &Irql,
                                      &Affinity);
    /* A range with an end that has no vector drops out, which is not an error */
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

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * Returns the interrupt translator of the HAL bus devices on the PIC HAL,
 * which turns the SCI line into its vector.
 */
CODE_SEG("PAGE")
NTSTATUS
NTAPI
HalpQueryPicLineTranslator(
    _Out_writes_bytes_(Size) PVOID Interface,
    _In_ ULONG Size,
    _Out_ PULONG Length)
{
    PTRANSLATOR_INTERFACE Translator = Interface;

    PAGED_CODE();

    *Length = sizeof(TRANSLATOR_INTERFACE);
    if (Size < sizeof(TRANSLATOR_INTERFACE))
        return STATUS_BUFFER_TOO_SMALL;

    RtlZeroMemory(Translator, sizeof(TRANSLATOR_INTERFACE));
    Translator->Size = sizeof(TRANSLATOR_INTERFACE);
    Translator->Version = HAL_IRQ_TRANSLATOR_VERSION;
    Translator->InterfaceReference = HalpIsaTranslatorReference;
    Translator->InterfaceDereference = HalpIsaTranslatorReference;
    Translator->TranslateResources = HalpTranslatePicLine;
    Translator->TranslateResourceRequirements = HalpTranslatePicLineRequirement;

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Returns the ISA interrupt translator for ISA and EISA bridges.
 *
 * @param[in] BridgeInterfaceType
 * Bus type found below the bridge.
 *
 * @param[in] Size
 * Size of the translator interface buffer.
 *
 * @param[out] Translator
 * Receives the translator interface.
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

    PAGED_CODE();

    switch (BridgeInterfaceType)
    {
        case InterfaceTypeUndefined:
        case Isa:
        case Eisa:
            break;

        default:
            return STATUS_NOT_IMPLEMENTED;
    }

    if (Size < sizeof(TRANSLATOR_INTERFACE))
        return STATUS_BUFFER_TOO_SMALL;

    RtlZeroMemory(Translator, sizeof(TRANSLATOR_INTERFACE));
    Translator->Size = sizeof(TRANSLATOR_INTERFACE);
    Translator->Version = HAL_IRQ_TRANSLATOR_VERSION;
    Translator->InterfaceReference = HalpIsaTranslatorReference;
    Translator->InterfaceDereference = HalpIsaTranslatorReference;
    Translator->TranslateResources = HalpTranslateIsaInterruptResource;
    Translator->TranslateResourceRequirements = HalpTranslateIsaInterruptRequirement;

    return STATUS_SUCCESS;
}

/* EOF */

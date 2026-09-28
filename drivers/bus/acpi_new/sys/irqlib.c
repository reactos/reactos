/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     GSIV to IDT vector allocation for the APIC and PIC models
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <uacpint.h>
#include <haltypes.h>
#include <rtlrangelist.h>
#include <uacpi/tables.h>
#include <uacpi/acpi.h>
#include <debug.h>

#ifndef ACPI_DRIVER_INTERNAL
#define ACPI_DRIVER_INTERNAL 0x000000A3
#endif

#ifndef KE_PROCESSOR_CHANGE_ADD_EXISTING
#define KE_PROCESSOR_CHANGE_ADD_EXISTING 0x00000001
#endif

#ifndef DEVPROP_TYPE_BINARY
#define DEVPROP_TYPE_BINARY 0x00001003
#endif

#define UACPINT_MAX_PROCESSORS          256
#define UACPINT_MAX_GSIV                256
#define UACPINT_MAX_MESSAGE_RUNS        128
#define UACPINT_MAX_IDT_VECTOR          0xFF
#define UACPINT_ISA_IRQ_COUNT           16

/* Device vector band used when the HAL granted nothing */
#define UACPINT_DEVICE_VECTOR_FIRST     0x50
#define UACPINT_DEVICE_VECTOR_LAST      0xBF

/* Message interrupts are placed by the arbiter at or above this GSIV */
#define UACPINT_MESSAGE_GSIV_BASE       0xFFF00000

/* Secondary vectors start past the IDT, the kernel indexes its secondary IDT with them */
#define UACPINT_SECONDARY_VECTOR_BASE   256
#define UACPINT_SECONDARY_VECTOR_COUNT  256

/* HalPrivateDispatchTable slots, identical on every release and arch */
#define UACPINT_HAL_SLOT_GET_INTERRUPT_VECTOR   22
#define UACPINT_HAL_SLOT_GET_VECTOR_INPUT       23
#define UACPINT_HAL_SLOT_ALLOCATE_MSG_TARGET    27
#define UACPINT_HAL_SLOT_FREE_MSG_TARGET        28

/* The message target slots appeared in version 6 (Vista SP1) */
#define UACPINT_HAL_PRIVATE_MSG_VERSION         6

/* RTL range owner for message vector runs */
#define UACPINT_MSI_RANGE_OWNER ((PVOID)(ULONG_PTR)0xACB1A5E1)

typedef
KIRQL
(NTAPI *PUACPINT_HAL_CONVERT_IDT_TO_IRQL)(
    _In_ ULONG IdtEntry);

typedef
NTSTATUS
(NTAPI *PUACPINT_IO_SET_DEVICE_PROPERTY_DATA)(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ CONST DEVPROPKEY *PropertyKey,
    _In_ LCID Lcid,
    _In_ ULONG Flags,
    _In_ DEVPROPTYPE Type,
    _In_ ULONG Size,
    _In_opt_ PVOID Data);

typedef
NTSTATUS
(NTAPI *PUACPINT_IO_GET_DEVICE_PROPERTY_DATA)(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ CONST DEVPROPKEY *PropertyKey,
    _In_ LCID Lcid,
    _Reserved_ ULONG Flags,
    _In_ ULONG Size,
    _Out_ PVOID Data,
    _Out_ PULONG RequiredSize,
    _Out_ PDEVPROPTYPE Type);

typedef
PVOID
(NTAPI *PUACPINT_KE_REGISTER_PROCESSOR_CHANGE_CALLBACK)(
    _In_ PPROCESSOR_CALLBACK_FUNCTION CallbackFunction,
    _In_opt_ PVOID CallbackContext,
    _In_ ULONG Flags);

/* Occupied IDT entries of one processor, APIC model only */
typedef struct _UACPINT_PROCESSOR_IDT
{
    RTL_RANGE_LIST UsedEntries;
    BOOLEAN        Valid;
} UACPINT_PROCESSOR_IDT, *PUACPINT_PROCESSOR_IDT;

/* A run of message vectors handed to one device */
typedef struct _UACPINT_MESSAGE_RUN
{
    PVOID   Owner;          ///< PDO, NULL marks a free slot
    ULONG   MessageGsiv;    ///< arbiter placement, 0 when the HAL asked first
    ULONG   Base;           ///< first IDT vector
    ULONG   Count;
    BOOLEAN Claimed;        ///< handed to the HAL through the message target slot
} UACPINT_MESSAGE_RUN, *PUACPINT_MESSAGE_RUN;

typedef struct _UACPINT_ISA_OVERRIDE
{
    ULONG   Gsiv;
    BOOLEAN Valid;
} UACPINT_ISA_OVERRIDE, *PUACPINT_ISA_OVERRIDE;

/* The kernel reads the connection data property with these layouts */
#if (NTDDI_VERSION < NTDDI_WIN7)
C_ASSERT(FIELD_OFFSET(INTERRUPT_CONNECTION_DATA, Vectors) == 0x8);
#if defined(_WIN64)
C_ASSERT(sizeof(INTERRUPT_VECTOR_DATA) == 0x30);
#else
C_ASSERT(sizeof(INTERRUPT_VECTOR_DATA) == 0x28);
#endif
#elif defined(_WIN64)
#if (NTDDI_VERSION >= NTDDI_WIN10_TH2)
C_ASSERT(FIELD_OFFSET(INTERRUPT_CONNECTION_DATA, Vectors) == 0x8);
C_ASSERT(sizeof(INTERRUPT_VECTOR_DATA) == 0x58);
#elif (NTDDI_VERSION >= NTDDI_WIN10)
C_ASSERT(FIELD_OFFSET(INTERRUPT_CONNECTION_DATA, Vectors) == 0x8);
C_ASSERT(sizeof(INTERRUPT_VECTOR_DATA) == 0x50);
#elif (NTDDI_VERSION >= NTDDI_WINBLUE)
C_ASSERT(FIELD_OFFSET(INTERRUPT_CONNECTION_DATA, Vectors) == 0x60);
C_ASSERT(sizeof(INTERRUPT_VECTOR_DATA) == 0x48);
#else
C_ASSERT(FIELD_OFFSET(INTERRUPT_CONNECTION_DATA, Vectors) == 0x8);
C_ASSERT(sizeof(INTERRUPT_VECTOR_DATA) == 0x48);
#endif
#endif

ULONG UacpiNtIrqLibHalOverrides = 1;

/* {F0E20F09-D97A-49A9-8046-BB6E22E6BB2E}, 2 */
static const DEVPROPKEY UacpiNtConnectionDataKey =
{
    { 0xF0E20F09, 0xD97A, 0x49A9, { 0x80, 0x46, 0xBB, 0x6E, 0x22, 0xE6, 0xBB, 0x2E } },
    2
};

static FAST_MUTEX UacpiNtIrqLibLock;
static BOOLEAN UacpiNtIrqLibReady;

/* PIC model: vector is base plus ISA IRQ */
static ULONG UacpiNtPicVectorBase;
static BOOLEAN UacpiNtPicBaseValid;

/* Resolved by name, the import libraries predate some of them */
static pHalGetInterruptVector UacpiNtHalGetInterruptVector;
static PUACPINT_HAL_CONVERT_IDT_TO_IRQL UacpiNtHalConvertDeviceIdtToIrql;
static PUACPINT_IO_SET_DEVICE_PROPERTY_DATA UacpiNtIoSetDevicePropertyData;
static PUACPINT_IO_GET_DEVICE_PROPERTY_DATA UacpiNtIoGetDevicePropertyData;
static PUACPINT_KE_REGISTER_PROCESSOR_CHANGE_CALLBACK UacpiNtKeRegisterProcessorChangeCallback;
static BOOLEAN UacpiNtKernelRoutinesResolved;

static UACPINT_PROCESSOR_IDT UacpiNtProcessorIdt[UACPINT_MAX_PROCESSORS];
static ULONG UacpiNtProcessorIdtCount;
static PVOID UacpiNtProcessorChangeHandle;

/* APIC model GSIV state, a zero vector means unassigned */
static ULONG UacpiNtGsivVector[UACPINT_MAX_GSIV];
static ULONG UacpiNtGsivLevel[UACPINT_MAX_GSIV / 32];
static ULONG UacpiNtGsivForcedEdge[UACPINT_MAX_GSIV / 32];
static ULONG UacpiNtGsivForcedHigh[UACPINT_MAX_GSIV / 32];
static ULONG UacpiNtGsivForcedLow[UACPINT_MAX_GSIV / 32];

/* IDT vectors the HAL granted the ACPI root at start */
static ULONG UacpiNtHalGrantedVectors[(UACPINT_MAX_IDT_VECTOR + 1) / 32];
static ULONG UacpiNtHalGrantedCount;

static UACPINT_MESSAGE_RUN UacpiNtMessageRuns[UACPINT_MAX_MESSAGE_RUNS];

static ULONG UacpiNtSecondaryVectorGsiv[UACPINT_SECONDARY_VECTOR_COUNT];
static ULONG UacpiNtSecondaryVectorCount;

/* MADT interrupt source overrides, ISA IRQ to GSIV */
static UACPINT_ISA_OVERRIDE UacpiNtIsaOverride[UACPINT_ISA_IRQ_COUNT];

static
BOOLEAN
NTAPI
UacpiNtTestBit(
    _In_reads_(UACPINT_MAX_GSIV / 32) const ULONG *Map,
    _In_ ULONG Bit)
{
    return (Map[Bit / 32] >> (Bit % 32)) & 1;
}

static
VOID
NTAPI
UacpiNtSetBit(
    _Inout_updates_(UACPINT_MAX_GSIV / 32) PULONG Map,
    _In_ ULONG Bit)
{
    Map[Bit / 32] |= 1UL << (Bit % 32);
}

static
KIRQL
NTAPI
UacpiNtVectorToIrql(
    _In_ ULONG Vector)
{
    if (UacpiNtHalConvertDeviceIdtToIrql)
        return UacpiNtHalConvertDeviceIdtToIrql(Vector);

    return (KIRQL)(Vector >> 4);
}

/* Physical destination mode takes a single processor, CPU 0 if none is set */
static
KAFFINITY
NTAPI
UacpiNtLowestProcessor(
    _In_ KAFFINITY Affinity)
{
    Affinity &= ~Affinity + 1;
    return Affinity ? Affinity : 1;
}

static
VOID
NTAPI
UacpiNtSetVectorTarget(
    _Inout_ PINTERRUPT_VECTOR_DATA VectorData,
    _In_ KAFFINITY Affinity)
{
#if (NTDDI_VERSION >= NTDDI_WIN7)
    VectorData->TargetProcessors.Mask = Affinity;
#else
    VectorData->TargetProcessors = Affinity;
#endif
}

static
KAFFINITY
NTAPI
UacpiNtGetVectorTarget(
    _In_ const INTERRUPT_VECTOR_DATA *VectorData)
{
#if (NTDDI_VERSION >= NTDDI_WIN7)
    return VectorData->TargetProcessors.Mask;
#else
    return VectorData->TargetProcessors;
#endif
}

static
VOID
NTAPI
UacpiNtResolveKernelRoutines(VOID)
{
    UNICODE_STRING RoutineName;

    if (UacpiNtKernelRoutinesResolved)
        return;

    RtlInitUnicodeString(&RoutineName, L"IoSetDevicePropertyData");
    UacpiNtIoSetDevicePropertyData = (PUACPINT_IO_SET_DEVICE_PROPERTY_DATA)MmGetSystemRoutineAddress(&RoutineName);

    RtlInitUnicodeString(&RoutineName, L"IoGetDevicePropertyData");
    UacpiNtIoGetDevicePropertyData = (PUACPINT_IO_GET_DEVICE_PROPERTY_DATA)MmGetSystemRoutineAddress(&RoutineName);

    RtlInitUnicodeString(&RoutineName, L"KeRegisterProcessorChangeCallback");
    UacpiNtKeRegisterProcessorChangeCallback =
        (PUACPINT_KE_REGISTER_PROCESSOR_CHANGE_CALLBACK)MmGetSystemRoutineAddress(&RoutineName);

    UacpiNtKernelRoutinesResolved = TRUE;
}

static
NTSTATUS
NTAPI
UacpiNtSetConnectionData(
    _In_ PDEVICE_OBJECT Pdo,
    _In_reads_bytes_(Size) PINTERRUPT_CONNECTION_DATA Data,
    _In_ ULONG Size)
{
    UacpiNtResolveKernelRoutines();
    if (!UacpiNtIoSetDevicePropertyData)
        return STATUS_NOT_SUPPORTED;

    return UacpiNtIoSetDevicePropertyData(Pdo,
                                          &UacpiNtConnectionDataKey,
                                          0,
                                          0,
                                          DEVPROP_TYPE_BINARY,
                                          Size,
                                          Data);
}

static
NTSTATUS
NTAPI
UacpiNtGetConnectionData(
    _In_ PDEVICE_OBJECT Pdo,
    _Out_writes_bytes_(Size) PINTERRUPT_CONNECTION_DATA Data,
    _In_ ULONG Size,
    _Out_ PULONG RequiredSize,
    _Out_ PDEVPROPTYPE Type)
{
    UacpiNtResolveKernelRoutines();
    if (!UacpiNtIoGetDevicePropertyData)
        return STATUS_NOT_SUPPORTED;

    return UacpiNtIoGetDevicePropertyData(Pdo,
                                          &UacpiNtConnectionDataKey,
                                          0,
                                          0,
                                          Size,
                                          Data,
                                          RequiredSize,
                                          Type);
}

VOID
NTAPI
UacpiNtIrqLibNoteLevelGsiv(
    _In_ ULONG Gsiv)
{
    if (Gsiv < UACPINT_MAX_GSIV)
        UacpiNtSetBit(UacpiNtGsivLevel, Gsiv);
}

/* An edge pin overrides a later level _CRS descriptor */
VOID
NTAPI
UacpiNtIrqLibNoteEdgeGsiv(
    _In_ ULONG Gsiv)
{
    if (Gsiv < UACPINT_MAX_GSIV)
        UacpiNtSetBit(UacpiNtGsivForcedEdge, Gsiv);
}

BOOLEAN
NTAPI
UacpiNtIrqLibGsivForcedEdge(
    _In_ ULONG Gsiv)
{
    return Gsiv < UACPINT_MAX_GSIV && UacpiNtTestBit(UacpiNtGsivForcedEdge, Gsiv);
}

static
BOOLEAN
NTAPI
UacpiNtGsivIsLevel(
    _In_ ULONG Gsiv)
{
    return Gsiv < UACPINT_MAX_GSIV && UacpiNtTestBit(UacpiNtGsivLevel, Gsiv);
}

/* A MADT polarity wins, otherwise level lines are active low and edge lines active high */
static
BOOLEAN
NTAPI
UacpiNtGsivIsActiveLow(
    _In_ ULONG Gsiv)
{
    if (Gsiv < UACPINT_MAX_GSIV)
    {
        if (UacpiNtTestBit(UacpiNtGsivForcedHigh, Gsiv))
            return FALSE;

        if (UacpiNtTestBit(UacpiNtGsivForcedLow, Gsiv))
            return TRUE;
    }

    return UacpiNtGsivIsLevel(Gsiv);
}

static
KINTERRUPT_POLARITY
NTAPI
UacpiNtGsivPolarity(
    _In_ ULONG Gsiv)
{
    return UacpiNtGsivIsActiveLow(Gsiv) ? InterruptActiveLow : InterruptActiveHigh;
}

static
KINTERRUPT_MODE
NTAPI
UacpiNtGsivMode(
    _In_ ULONG Gsiv)
{
    return UacpiNtGsivIsLevel(Gsiv) ? LevelSensitive : Latched;
}

static
BOOLEAN
NTAPI
UacpiNtHalGrantedVector(
    _In_ ULONG Vector)
{
    return Vector <= UACPINT_MAX_IDT_VECTOR && UacpiNtTestBit(UacpiNtHalGrantedVectors, Vector);
}

/* The raw start list carries IDT entries in Vector */
VOID
NTAPI
UacpiNtIrqLibSetHalVectors(
    _In_opt_ PCM_RESOURCE_LIST Resources)
{
    PCM_PARTIAL_RESOURCE_LIST PartialList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    ULONG Index;

    RtlZeroMemory(UacpiNtHalGrantedVectors, sizeof(UacpiNtHalGrantedVectors));
    UacpiNtHalGrantedCount = 0;

    if (!Resources || !Resources->Count)
    {
        DPRINT1("uACPI-NT: no HAL vector grant, using the fixed band 0x%x-0x%x\n",
                UACPINT_DEVICE_VECTOR_FIRST,
                UACPINT_DEVICE_VECTOR_LAST);
        return;
    }

    PartialList = &Resources->List[0].PartialResourceList;
    for (Index = 0; Index < PartialList->Count; Index++)
    {
        Descriptor = &PartialList->PartialDescriptors[Index];
        if (Descriptor->Type != CmResourceTypeInterrupt ||
            Descriptor->u.Interrupt.Vector > UACPINT_MAX_IDT_VECTOR ||
            UacpiNtHalGrantedVector(Descriptor->u.Interrupt.Vector))
        {
            continue;
        }

        UacpiNtSetBit(UacpiNtHalGrantedVectors, Descriptor->u.Interrupt.Vector);
        UacpiNtHalGrantedCount++;
    }
}

/*
 * Every vector the HAL did not grant counts as occupied. The granted ones
 * are unexpected interrupt stubs that KeConnectInterrupt accepts.
 */
static
VOID
NTAPI
UacpiNtSeedIdtState(
    _Out_ PRTL_RANGE_LIST UsedEntries)
{
    ULONG Vector;
    ULONG RunStart;

    RtlInitializeRangeList(UsedEntries);

    if (!UacpiNtHalGrantedCount)
    {
        RtlAddRange(UsedEntries, 0, UACPINT_DEVICE_VECTOR_FIRST - 1, 0, 0, NULL, NULL);
        RtlAddRange(UsedEntries, UACPINT_DEVICE_VECTOR_LAST + 1, UACPINT_MAX_IDT_VECTOR, 0, 0, NULL, NULL);
        return;
    }

    Vector = 0;
    while (Vector <= UACPINT_MAX_IDT_VECTOR)
    {
        if (UacpiNtHalGrantedVector(Vector))
        {
            Vector++;
            continue;
        }

        RunStart = Vector;
        while (Vector <= UACPINT_MAX_IDT_VECTOR && !UacpiNtHalGrantedVector(Vector))
            Vector++;

        RtlAddRange(UsedEntries, RunStart, Vector - 1, 0, 0, NULL, NULL);
    }
}


/* Caller holds UacpiNtIrqLibLock */
static
VOID
NTAPI
UacpiNtSeedProcessor(
    _In_ ULONG Processor)
{
    if (UacpiNtProcessorIdt[Processor].Valid)
        return;

    UacpiNtSeedIdtState(&UacpiNtProcessorIdt[Processor].UsedEntries);
    UacpiNtProcessorIdt[Processor].Valid = TRUE;
    UacpiNtProcessorIdtCount = max(UacpiNtProcessorIdtCount, Processor + 1);
}

_Function_class_(PROCESSOR_CALLBACK_FUNCTION)
static
VOID
NTAPI
UacpiNtProcessorChangeCallback(
    _In_ PVOID CallbackContext,
    _In_ PKE_PROCESSOR_CHANGE_NOTIFY_CONTEXT ChangeContext,
    _Inout_ PNTSTATUS OperationStatus)
{
    UNREFERENCED_PARAMETER(CallbackContext);
    UNREFERENCED_PARAMETER(OperationStatus);

    if (ChangeContext->State != KeProcessorAddCompleteNotify)
        return;

    ExAcquireFastMutex(&UacpiNtIrqLibLock);
    if (ChangeContext->NtNumber < UACPINT_MAX_PROCESSORS &&
        !UacpiNtProcessorIdt[ChangeContext->NtNumber].Valid)
    {
        UacpiNtSeedProcessor(ChangeContext->NtNumber);
        DPRINT("uACPI-NT: processor %lu IDT state seeded\n", ChangeContext->NtNumber);
    }
    ExReleaseFastMutex(&UacpiNtIrqLibLock);
}

static
VOID
NTAPI
UacpiNtSeedExistingProcessors(VOID)
{
    ULONG Count = min((ULONG)(UCHAR)KeNumberProcessors, UACPINT_MAX_PROCESSORS);
    ULONG Processor;

    ExAcquireFastMutex(&UacpiNtIrqLibLock);
    for (Processor = 0; Processor < Count; Processor++)
        UacpiNtSeedProcessor(Processor);
    ExReleaseFastMutex(&UacpiNtIrqLibLock);

    DPRINT("uACPI-NT: seeded %lu of %lu processor IDT sets\n", UacpiNtProcessorIdtCount, Count);
}

/* TRUE when Vector is free on every tracked processor */
static
BOOLEAN
NTAPI
UacpiNtVectorFreeEverywhere(
    _In_ ULONG Vector)
{
    BOOLEAN Available;
    ULONG Processor;

    for (Processor = 0; Processor < UacpiNtProcessorIdtCount; Processor++)
    {
        if (!UacpiNtProcessorIdt[Processor].Valid)
            continue;

        Available = FALSE;
        if (!NT_SUCCESS(RtlIsRangeAvailable(&UacpiNtProcessorIdt[Processor].UsedEntries,
                                            Vector,
                                            Vector,
                                            0,
                                            0,
                                            NULL,
                                            NULL,
                                            &Available)) ||
            !Available)
        {
            return FALSE;
        }
    }

    return TRUE;
}

static
VOID
NTAPI
UacpiNtClaimVectors(
    _In_ ULONG First,
    _In_ ULONG Last,
    _In_ PVOID Owner)
{
    ULONG Processor;

    for (Processor = 0; Processor < UacpiNtProcessorIdtCount; Processor++)
    {
        if (UacpiNtProcessorIdt[Processor].Valid)
            RtlAddRange(&UacpiNtProcessorIdt[Processor].UsedEntries, First, Last, 0, 0, NULL, Owner);
    }
}

/* First fit an IDT entry free on every processor. Caller holds UacpiNtIrqLibLock. */
static
NTSTATUS
NTAPI
UacpiNtApicAllocateVector(
    _In_ ULONG Gsiv,
    _Out_ PULONG Vector)
{
    PVOID Owner = (PVOID)(ULONG_PTR)(Gsiv + 1);
    ULONG Candidate;

    /* Shared line or a repeated query */
    if (Gsiv < UACPINT_MAX_GSIV && UacpiNtGsivVector[Gsiv])
    {
        *Vector = UacpiNtGsivVector[Gsiv];
        return STATUS_SUCCESS;
    }

    if (!UacpiNtProcessorIdtCount)
        return STATUS_UNSUCCESSFUL;

    /* Device vectors come from the band the HAL granted, the HAL hands out none of them */
    for (Candidate = UACPINT_DEVICE_VECTOR_FIRST; Candidate <= UACPINT_DEVICE_VECTOR_LAST; Candidate++)
    {
        if (!UacpiNtVectorFreeEverywhere(Candidate))
            continue;

        UacpiNtClaimVectors(Candidate, Candidate, Owner);
        if (Gsiv < UACPINT_MAX_GSIV)
            UacpiNtGsivVector[Gsiv] = Candidate;

        *Vector = Candidate;
        return STATUS_SUCCESS;
    }

    return STATUS_INSUFFICIENT_RESOURCES;
}

/*
 * Consecutive entries free on every processor, aligned to Count when it is
 * a power of two. Caller holds UacpiNtIrqLibLock.
 */
static
NTSTATUS
NTAPI
UacpiNtApicAllocateVectorRange(
    _In_ ULONG Count,
    _Out_ PULONG BaseVector)
{
    ULONG Alignment;
    ULONG Candidate;
    ULONG Offset;

    if (!Count)
        Count = 1;

    if (!UacpiNtProcessorIdtCount)
        return STATUS_UNSUCCESSFUL;

    Alignment = (Count & (Count - 1)) ? 1 : Count;
    Candidate = (UACPINT_DEVICE_VECTOR_FIRST + Alignment - 1) & ~(Alignment - 1);

    for (; Candidate + Count - 1 <= UACPINT_DEVICE_VECTOR_LAST; Candidate += Alignment)
    {
        /* Line and message vectors cannot share an entry */
        for (Offset = 0; Offset < Count; Offset++)
        {
            if (!UacpiNtVectorFreeEverywhere(Candidate + Offset))
                break;
        }

        if (Offset < Count)
            continue;

        UacpiNtClaimVectors(Candidate, Candidate + Count - 1, UACPINT_MSI_RANGE_OWNER);
        *BaseVector = Candidate;
        return STATUS_SUCCESS;
    }

    return STATUS_INSUFFICIENT_RESOURCES;
}

static
VOID
NTAPI
UacpiNtApicFreeVectorRange(
    _In_ ULONG BaseVector,
    _In_ ULONG Count)
{
    ULONG Processor;

    if (!Count)
        Count = 1;

    for (Processor = 0; Processor < UacpiNtProcessorIdtCount; Processor++)
    {
        if (UacpiNtProcessorIdt[Processor].Valid)
        {
            RtlDeleteRange(&UacpiNtProcessorIdt[Processor].UsedEntries,
                           BaseVector,
                           BaseVector + Count - 1,
                           UACPINT_MSI_RANGE_OWNER);
        }
    }
}

/* Find or allocate the message run keyed by owner, placement and count */
NTSTATUS
NTAPI
UacpiNtIrqLibResolveMessageVector(
    _In_ PVOID Owner,
    _In_ ULONG MessageGsiv,
    _In_ ULONG Count,
    _Out_ PULONG BaseVector,
    _Out_ PKIRQL Irql,
    _Out_ PKAFFINITY Affinity)
{
    PUACPINT_MESSAGE_RUN FreeRun = NULL;
    PUACPINT_MESSAGE_RUN Run;
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS RetryStatus;
    ULONG Base = 0;
    ULONG Index;

    if (!Count)
        Count = 1;

    /* Message interrupts only exist under the APIC model */
    if (GlobalAcpiInterruptModel != ACPI_NT_APIC)
        return STATUS_NOT_SUPPORTED;

    ExAcquireFastMutex(&UacpiNtIrqLibLock);

    for (Index = 0; Index < UACPINT_MAX_MESSAGE_RUNS; Index++)
    {
        Run = &UacpiNtMessageRuns[Index];
        if (Run->Owner == Owner && Run->MessageGsiv == MessageGsiv && Run->Count == Count)
        {
            Base = Run->Base;
            break;
        }

        if (!FreeRun && !Run->Owner)
            FreeRun = Run;
    }

    if (Index == UACPINT_MAX_MESSAGE_RUNS)
    {
        if (!FreeRun)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            Status = UacpiNtApicAllocateVectorRange(Count, &Base);

            /* Settle for a single message when the full run is not available */
            if (!NT_SUCCESS(Status) && Count > 1)
            {
                RetryStatus = UacpiNtApicAllocateVectorRange(1, &Base);
                DPRINT1("uACPI-NT: no run of %lu vectors for message GSIV 0x%lx (0x%lx), single vector 0x%lx\n",
                        Count,
                        MessageGsiv,
                        Status,
                        RetryStatus);
                if (NT_SUCCESS(RetryStatus))
                {
                    Count = 1;
                    Status = RetryStatus;
                }
            }

            if (NT_SUCCESS(Status))
            {
                FreeRun->Owner = Owner;
                FreeRun->MessageGsiv = MessageGsiv;
                FreeRun->Base = Base;
                FreeRun->Count = Count;
                FreeRun->Claimed = FALSE;
            }
        }
    }

    ExReleaseFastMutex(&UacpiNtIrqLibLock);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("uACPI-NT: message vector allocation for GSIV 0x%lx x%lu failed 0x%lx\n",
                MessageGsiv,
                Count,
                Status);
        return Status;
    }

    *BaseVector = Base;
    *Irql = UacpiNtVectorToIrql(Base);

    /* Must match the target in the connection data */
    *Affinity = UacpiNtLowestProcessor(KeQueryActiveProcessors());
    return STATUS_SUCCESS;
}

static
BOOLEAN
NTAPI
UacpiNtIsSecondaryGsiv(
    _In_ ULONG Gsiv)
{
#if (NTDDI_VERSION >= NTDDI_WIN8)
    if (!HALPRIVATEDISPATCH->HalIsInterruptTypeSecondary)
        return FALSE;

    return HALPRIVATEDISPATCH->HalIsInterruptTypeSecondary(InterruptTypeControllerInput, Gsiv);
#else
    /* Secondary controllers arrived with Windows 8 */
    UNREFERENCED_PARAMETER(Gsiv);
    return FALSE;
#endif
}

/* One vector per secondary GSIV for its whole lifetime */
static
NTSTATUS
NTAPI
UacpiNtAllocateSecondaryVector(
    _In_ ULONG Gsiv,
    _Out_ PULONG Vector)
{
    NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;
    ULONG Index;

    ExAcquireFastMutex(&UacpiNtIrqLibLock);

    for (Index = 0; Index < UacpiNtSecondaryVectorCount; Index++)
    {
        if (UacpiNtSecondaryVectorGsiv[Index] == Gsiv)
            break;
    }

    if (Index == UacpiNtSecondaryVectorCount && Index < UACPINT_SECONDARY_VECTOR_COUNT)
    {
        UacpiNtSecondaryVectorGsiv[Index] = Gsiv;
        UacpiNtSecondaryVectorCount++;
    }

    if (Index < UacpiNtSecondaryVectorCount)
    {
        *Vector = UACPINT_SECONDARY_VECTOR_BASE + Index;
        Status = STATUS_SUCCESS;
    }

    ExReleaseFastMutex(&UacpiNtIrqLibLock);
    return Status;
}

/*
 * Secondary pin ISRs run inside the controller interrupt, so the IRQL comes
 * from the primary line. Without a primary it stays HIGH_LEVEL.
 */
static
NTSTATUS
NTAPI
UacpiNtResolveSecondaryVector(
    _In_ ULONG Gsiv,
    _Out_ PULONG Vector,
    _Out_ PKIRQL Irql)
{
#if (NTDDI_VERSION >= NTDDI_WIN8)
    INTERRUPT_CONNECTION_DATA Query;
    ULONG PrimaryGsiv = 0;
    ULONG PrimaryVector = 0;
#endif
    NTSTATUS Status;

    Status = UacpiNtAllocateSecondaryVector(Gsiv, Vector);
    if (!NT_SUCCESS(Status))
        return Status;

    *Irql = HIGH_LEVEL;

#if (NTDDI_VERSION >= NTDDI_WIN8)
    if (!HALPRIVATEDISPATCH->HalSecondaryInterruptQueryPrimaryInformation)
        return STATUS_SUCCESS;

    RtlZeroMemory(&Query, sizeof(Query));
    Query.Count = 1;
    Query.Vectors[0].Type = InterruptTypeControllerInput;
    Query.Vectors[0].ControllerInput.Gsiv = Gsiv;

    /* Windows 10 takes the vector data, Windows 8 the whole connection data */
#if (NTDDI_VERSION >= NTDDI_WIN10)
    Status = HALPRIVATEDISPATCH->HalSecondaryInterruptQueryPrimaryInformation(&Query.Vectors[0],
                                                                              &PrimaryGsiv);
#else
    Status = HALPRIVATEDISPATCH->HalSecondaryInterruptQueryPrimaryInformation(&Query,
                                                                              &PrimaryGsiv);
#endif
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("uACPI-NT: no primary for secondary GSIV %lu (0x%lx)\n", Gsiv, Status);
        return STATUS_SUCCESS;
    }

    ExAcquireFastMutex(&UacpiNtIrqLibLock);
    Status = UacpiNtApicAllocateVector(PrimaryGsiv, &PrimaryVector);
    ExReleaseFastMutex(&UacpiNtIrqLibLock);

    if (NT_SUCCESS(Status) && UacpiNtHalConvertDeviceIdtToIrql)
        *Irql = UacpiNtHalConvertDeviceIdtToIrql(PrimaryVector);
#endif

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
UacpiNtIrqLibResolveVector(
    _In_ ULONG Gsiv,
    _Out_ PULONG Vector,
    _Out_ PKIRQL Irql,
    _Out_ PKAFFINITY Affinity,
    _Out_ PULONG Polarity,
    _Out_ PULONG Mode)
{
    KAFFINITY TargetAffinity = 0;
    KAFFINITY HalAffinity = 0;
    KIRQL VectorIrql = 0;
    KIRQL HalIrql = 0;
    ULONG HalVector = 0;
    ULONG Assigned = 0;
    NTSTATUS Status;

    if (UacpiNtIsSecondaryGsiv(Gsiv))
    {
        Status = UacpiNtResolveSecondaryVector(Gsiv, &Assigned, &VectorIrql);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("uACPI-NT: secondary GSIV %lu unresolved 0x%lx\n", Gsiv, Status);
            return Status;
        }

        *Vector = Assigned;
        *Irql = VectorIrql;
        *Affinity = KeQueryActiveProcessors();
        *Polarity = UacpiNtGsivPolarity(Gsiv);
        *Mode = UacpiNtGsivMode(Gsiv);

        DPRINT("uACPI-NT: secondary GSIV %lu uses vector 0x%lx IRQL %u\n", Gsiv, Assigned, VectorIrql);
        return STATUS_SUCCESS;
    }

    if (GlobalAcpiInterruptModel == ACPI_NT_APIC)
    {
        /* Under APIC this driver owns the allocation */
        ExAcquireFastMutex(&UacpiNtIrqLibLock);
        Status = UacpiNtApicAllocateVector(Gsiv, &Assigned);
        ExReleaseFastMutex(&UacpiNtIrqLibLock);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("uACPI-NT: APIC vector allocation for GSIV %lu failed 0x%lx\n", Gsiv, Status);
            return Status;
        }

        VectorIrql = UacpiNtVectorToIrql(Assigned);
        TargetAffinity = KeQueryActiveProcessors();
    }
    else
    {
        /* Under PIC the HAL answers, the calibrated base is the fallback */
        if (UacpiNtHalGetInterruptVector)
            HalVector = UacpiNtHalGetInterruptVector(Isa, 0, Gsiv, Gsiv, &HalIrql, &HalAffinity);

        if (HalVector)
        {
            Assigned = HalVector;
            VectorIrql = HalIrql;
            TargetAffinity = HalAffinity ? HalAffinity : KeQueryActiveProcessors();
        }
        else
        {
            if (!UacpiNtPicBaseValid)
                return STATUS_NOT_SUPPORTED;

            Assigned = UacpiNtPicVectorBase + Gsiv;
            VectorIrql = UacpiNtVectorToIrql(Assigned);
            TargetAffinity = KeQueryActiveProcessors();
        }
    }

    if (!TargetAffinity)
        TargetAffinity = KeQueryActiveProcessors();

    *Vector = Assigned;
    *Irql = VectorIrql;
    *Affinity = UacpiNtLowestProcessor(TargetAffinity);
    *Polarity = UacpiNtGsivPolarity(Gsiv);
    *Mode = UacpiNtGsivMode(Gsiv);

    DPRINT("uACPI-NT: GSIV %lu uses vector 0x%lx IRQL %u affinity %p %s/%s (%s)\n",
           Gsiv,
           Assigned,
           VectorIrql,
           (PVOID)*Affinity,
           *Mode == LevelSensitive ? "level" : "edge",
           *Polarity == InterruptActiveLow ? "low" : "high",
           GlobalAcpiInterruptModel == ACPI_NT_APIC ? "APIC" : "PIC");
    return STATUS_SUCCESS;
}

/* Caller holds UacpiNtIrqLibLock */
static
BOOLEAN
NTAPI
UacpiNtOwnerHasMessages(
    _In_ PVOID Owner)
{
    ULONG Index;

    for (Index = 0; Index < UACPINT_MAX_MESSAGE_RUNS; Index++)
    {
        if (UacpiNtMessageRuns[Index].Owner == Owner && UacpiNtMessageRuns[Index].Count)
            return TRUE;
    }

    return FALSE;
}

/*
 * Element order must match the order of the message descriptors. The arbiter
 * places them top down, so a higher GSIV is an earlier descriptor.
 */
static
BOOLEAN
NTAPI
UacpiNtMessageRunPrecedes(
    _In_ PUACPINT_MESSAGE_RUN Run,
    _In_ PUACPINT_MESSAGE_RUN Other)
{
    if (Run->MessageGsiv != Other->MessageGsiv)
    {
        /* HAL placed runs carry no GSIV and go last */
        if (!Run->MessageGsiv || !Other->MessageGsiv)
            return Run->MessageGsiv != 0;

        return Run->MessageGsiv > Other->MessageGsiv;
    }

    return Run->Base < Other->Base;
}

/*
 * One message request element per message the device owns. A run keeps its
 * base first, plain multi message MSI needs that.
 */
static
NTSTATUS
NTAPI
UacpiNtWriteMessageConnectionData(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ ULONG MessageGsiv,
    _In_ ULONG Count)
{
    PINTERRUPT_CONNECTION_DATA Data;
    PINTERRUPT_CONNECTION_DATA Readback;
    PINTERRUPT_VECTOR_DATA Element;
    PUACPINT_MESSAGE_RUN Next;
    PUACPINT_MESSAGE_RUN Run;
    BOOLEAN Emitted[UACPINT_MAX_MESSAGE_RUNS];
    KAFFINITY Processors = 0;
    KAFFINITY Target;
    KIRQL BaseIrql = 0;
    DEVPROPTYPE ReadbackType = 0;
    ULONG ReadbackSize = 0;
    ULONG Base = 0;
    ULONG Total = 0;
    ULONG Written = 0;
    ULONG Size;
    ULONG Index;
    ULONG Message;
    NTSTATUS ReadbackStatus;
    NTSTATUS Status;

    if (!Count)
        Count = 1;

    Status = UacpiNtIrqLibResolveMessageVector(Pdo, MessageGsiv, Count, &Base, &BaseIrql, &Processors);
    if (!NT_SUCCESS(Status))
        return Status;

    Target = UacpiNtLowestProcessor(Processors);

    ExAcquireFastMutex(&UacpiNtIrqLibLock);
    for (Index = 0; Index < UACPINT_MAX_MESSAGE_RUNS; Index++)
    {
        if (UacpiNtMessageRuns[Index].Owner == Pdo)
            Total += UacpiNtMessageRuns[Index].Count;
    }
    ExReleaseFastMutex(&UacpiNtIrqLibLock);

    /* The resolve above recorded a run, so this is not expected */
    if (!Total)
        Total = Count;

    Size = FIELD_OFFSET(INTERRUPT_CONNECTION_DATA, Vectors) + Total * sizeof(*Data->Vectors);
    Data = ExAllocatePoolWithTag(PagedPool, Size, UACPINT_POOL_TAG);
    if (!Data)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(Data, Size);
    RtlZeroMemory(Emitted, sizeof(Emitted));

    ExAcquireFastMutex(&UacpiNtIrqLibLock);
    while (Written < Total)
    {
        Next = NULL;
        for (Index = 0; Index < UACPINT_MAX_MESSAGE_RUNS; Index++)
        {
            Run = &UacpiNtMessageRuns[Index];
            if (Run->Owner != Pdo || !Run->Count || Emitted[Index])
                continue;

            if (!Next || UacpiNtMessageRunPrecedes(Run, Next))
                Next = Run;
        }

        if (!Next)
            break;

        Emitted[Next - UacpiNtMessageRuns] = TRUE;

        for (Message = 0; Message < Next->Count && Written < Total; Message++, Written++)
        {
            Element = &Data->Vectors[Written];
            Element->Type = InterruptTypeMessageRequest;
            Element->Vector = Next->Base + Message;
            Element->Irql = UacpiNtVectorToIrql(Element->Vector);
            Element->Polarity = InterruptActiveHigh;
            Element->Mode = Latched;
            Element->MessageRequest.DestinationMode = ApicDestinationModePhysical;
            UacpiNtSetVectorTarget(Element, Target);
        }
    }
    ExReleaseFastMutex(&UacpiNtIrqLibLock);

    Data->Count = Written ? Written : Total;

    Status = UacpiNtSetConnectionData(Pdo, Data, Size);
    DPRINT("uACPI-NT: PDO %p message data for GSIV 0x%lx, %lu of %lu vectors 0x%lx-0x%lx IRQL %u target %p: 0x%lx\n",
           Pdo,
           MessageGsiv,
           Data->Count,
           Total,
           Data->Vectors[0].Vector,
           Data->Vectors[Data->Count - 1].Vector,
           BaseIrql,
           (PVOID)Target,
           Status);
    ExFreePoolWithTag(Data, UACPINT_POOL_TAG);

    if (!NT_SUCCESS(Status))
        return Status;

    /* Read it back to confirm what the kernel will see */
    Readback = ExAllocatePoolWithTag(PagedPool, Size, UACPINT_POOL_TAG);
    if (Readback)
    {
        RtlZeroMemory(Readback, Size);
        ReadbackStatus = UacpiNtGetConnectionData(Pdo, Readback, Size, &ReadbackSize, &ReadbackType);
        DPRINT("uACPI-NT: message readback 0x%lx type 0x%lx size %lu of %lu, count %lu vectors 0x%lx-0x%lx\n",
               ReadbackStatus,
               ReadbackType,
               ReadbackSize,
               Size,
               Readback->Count,
               Readback->Count ? Readback->Vectors[0].Vector : 0,
               Readback->Count ? Readback->Vectors[Readback->Count - 1].Vector : 0);
        ExFreePoolWithTag(Readback, UACPINT_POOL_TAG);
    }

    return Status;
}

/* Publish the assignment as the device's interrupt connection data property */
NTSTATUS
NTAPI
UacpiNtIrqLibWriteConnectionData(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ ULONG Gsiv,
    _In_ ULONG Count)
{
    INTERRUPT_CONNECTION_DATA Data;
    INTERRUPT_CONNECTION_DATA Readback;
    DEVPROPTYPE ReadbackType = 0;
    ULONG ReadbackSize = 0;
    KAFFINITY Affinity = 0;
    KIRQL Irql = 0;
    ULONG Vector = 0;
    ULONG Polarity = 0;
    ULONG Mode = 0;
    BOOLEAN HasMessages;
    NTSTATUS ReadbackStatus;
    NTSTATUS Status;

    if (Gsiv >= UACPINT_MESSAGE_GSIV_BASE)
        return UacpiNtWriteMessageConnectionData(Pdo, Gsiv, Count);

    /* A line element must never replace message elements */
    ExAcquireFastMutex(&UacpiNtIrqLibLock);
    HasMessages = UacpiNtOwnerHasMessages(Pdo);
    ExReleaseFastMutex(&UacpiNtIrqLibLock);

    if (HasMessages)
    {
        DPRINT("uACPI-NT: PDO %p holds message vectors, GSIV %lu line data not written\n", Pdo, Gsiv);
        return STATUS_SUCCESS;
    }

    Status = UacpiNtIrqLibResolveVector(Gsiv, &Vector, &Irql, &Affinity, &Polarity, &Mode);
    if (!NT_SUCCESS(Status))
        return Status;

    RtlZeroMemory(&Data, sizeof(Data));
    Data.Count = 1;
    Data.Vectors[0].Type = InterruptTypeControllerInput;
    Data.Vectors[0].Vector = Vector;
    Data.Vectors[0].Irql = Irql;
    Data.Vectors[0].Polarity = (KINTERRUPT_POLARITY)Polarity;
    Data.Vectors[0].Mode = (KINTERRUPT_MODE)Mode;
    Data.Vectors[0].ControllerInput.Gsiv = Gsiv;
    UacpiNtSetVectorTarget(&Data.Vectors[0], Affinity);

    Status = UacpiNtSetConnectionData(Pdo, &Data, sizeof(Data));
    DPRINT("uACPI-NT: PDO %p connection data for GSIV %lu vector 0x%lx IRQL %u: 0x%lx\n",
           Pdo,
           Gsiv,
           Vector,
           Irql,
           Status);

    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(&Readback, sizeof(Readback));
        ReadbackStatus = UacpiNtGetConnectionData(Pdo, &Readback, sizeof(Readback), &ReadbackSize, &ReadbackType);
        DPRINT("uACPI-NT: GSIV %lu readback 0x%lx type 0x%lx size %lu of %lu, vector 0x%lx IRQL %u target %p GSIV %lu\n",
               Gsiv,
               ReadbackStatus,
               ReadbackType,
               ReadbackSize,
               (ULONG)sizeof(Data),
               Readback.Vectors[0].Vector,
               Readback.Vectors[0].Irql,
               (PVOID)UacpiNtGetVectorTarget(&Readback.Vectors[0]),
               Readback.Vectors[0].ControllerInput.Gsiv);
    }

    return Status;
}

static
VOID
NTAPI
UacpiNtParseMadt(VOID)
{
    struct acpi_madt_interrupt_source_override *Override;
    struct acpi_entry_hdr *Entry;
    struct acpi_madt *Madt;
    uacpi_table Table;
    PUCHAR Cursor;
    PUCHAR End;
    ULONG Trigger;
    ULONG Polarity;

    if (uacpi_unlikely_error(uacpi_table_find_by_signature(ACPI_MADT_SIGNATURE, &Table)) || !Table.ptr)
        return;

    Madt = Table.ptr;
    Cursor = (PUCHAR)Madt->entries;
    End = (PUCHAR)Madt + Madt->hdr.length;

    while (Cursor + sizeof(*Entry) <= End)
    {
        Entry = (struct acpi_entry_hdr *)Cursor;
        if (Entry->length < sizeof(*Entry) || Cursor + Entry->length > End)
            break;

        Cursor += Entry->length;

        if (Entry->type != ACPI_MADT_ENTRY_TYPE_INTERRUPT_SOURCE_OVERRIDE ||
            Entry->length < sizeof(*Override))
        {
            continue;
        }

        /* Only ISA sources below 16 remap IRQs */
        Override = (struct acpi_madt_interrupt_source_override *)Entry;
        if (Override->bus != 0 || Override->source >= UACPINT_ISA_IRQ_COUNT)
            continue;

        UacpiNtIsaOverride[Override->source].Gsiv = Override->gsi;
        UacpiNtIsaOverride[Override->source].Valid = TRUE;

        /* An explicit trigger or polarity wins over _CRS, conforming follows the trigger */
        Trigger = Override->flags & ACPI_MADT_TRIGGERING_MASK;
        if (Trigger == ACPI_MADT_TRIGGERING_LEVEL)
            UacpiNtIrqLibNoteLevelGsiv(Override->gsi);
        else if (Trigger == ACPI_MADT_TRIGGERING_EDGE)
            UacpiNtIrqLibNoteEdgeGsiv(Override->gsi);

        Polarity = Override->flags & ACPI_MADT_POLARITY_MASK;
        if (Override->gsi < UACPINT_MAX_GSIV)
        {
            if (Polarity == ACPI_MADT_POLARITY_ACTIVE_HIGH)
                UacpiNtSetBit(UacpiNtGsivForcedHigh, Override->gsi);
            else if (Polarity == ACPI_MADT_POLARITY_ACTIVE_LOW)
                UacpiNtSetBit(UacpiNtGsivForcedLow, Override->gsi);
        }

        DPRINT("uACPI-NT: MADT override IRQ %u to GSIV %lu flags 0x%x\n",
               Override->source,
               (ULONG)Override->gsi,
               Override->flags);
    }

    uacpi_table_unref(&Table);
}

/* Identity unless a MADT override remaps the IRQ */
static
ULONG
NTAPI
UacpiNtGsivFromIrq(
    _In_ ULONG Irq)
{
    if (Irq < UACPINT_ISA_IRQ_COUNT && UacpiNtIsaOverride[Irq].Valid)
        return UacpiNtIsaOverride[Irq].Gsiv;

    return Irq;
}

/* HalGetInterruptVector override, only reports vectors already committed */
static
ULONG
NTAPI
UacpiNtHalGetInterruptVectorOverride(
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG BusNumber,
    _In_ ULONG BusInterruptLevel,
    _In_ ULONG BusInterruptVector,
    _Out_opt_ PKIRQL Irql,
    _Out_opt_ PKAFFINITY Affinity)
{
    ULONG Gsiv;
    ULONG Vector;

    UNREFERENCED_PARAMETER(InterfaceType);

    /* A line interrupt has Level == Vector */
    if (BusInterruptLevel != BusInterruptVector)
        return 0;

    Gsiv = UacpiNtGsivFromIrq(BusInterruptLevel);
    if (Gsiv >= UACPINT_MAX_GSIV || !UacpiNtGsivVector[Gsiv])
    {
        DPRINT("uACPI-NT: GetInterruptVector bus %lu IRQ %lu has no committed vector\n",
               BusNumber,
               BusInterruptLevel);
        return 0;
    }

    Vector = UacpiNtGsivVector[Gsiv];
    DPRINT("uACPI-NT: GetInterruptVector bus %lu IRQ %lu is GSIV %lu vector 0x%lx\n",
           BusNumber,
           BusInterruptLevel,
           Gsiv,
           Vector);

    if (Irql)
        *Irql = UacpiNtVectorToIrql(Vector);
    if (Affinity)
        *Affinity = KeQueryActiveProcessors();

    return Vector;
}

/*
 * HalGetVectorInput override. STATUS_INVALID_PARAMETER sends the kernel
 * down the message path, STATUS_NOT_FOUND means the vector is not ours.
 */
#if (NTDDI_VERSION >= NTDDI_WIN7)
static
NTSTATUS
NTAPI
UacpiNtHalGetVectorInputOverride(
    _In_ ULONG Vector,
    _In_ PGROUP_AFFINITY Affinity,
    _Out_opt_ PULONG Input,
    _Out_opt_ PKINTERRUPT_POLARITY Polarity,
    _Out_opt_ PINTERRUPT_REMAPPING_INFO IntRemapInfo)
#else
static
NTSTATUS
NTAPI
UacpiNtHalGetVectorInputOverride(
    _In_ ULONG Vector,
    _In_ KAFFINITY Affinity,
    _Out_opt_ PULONG Input,
    _Out_opt_ PKINTERRUPT_POLARITY Polarity)
#endif
{
    PUACPINT_MESSAGE_RUN Run;
    ULONG Index;

    UNREFERENCED_PARAMETER(Affinity);

    if (!Vector)
        return STATUS_INVALID_PARAMETER;

    for (Index = 0; Index < UACPINT_MAX_GSIV; Index++)
    {
        if (UacpiNtGsivVector[Index] != Vector)
            continue;

        if (Input)
            *Input = Index;
        if (Polarity)
            *Polarity = UacpiNtGsivPolarity(Index);
#if (NTDDI_VERSION >= NTDDI_WIN7)
        /* No interrupt remapping */
        if (IntRemapInfo)
            RtlZeroMemory(IntRemapInfo, sizeof(*IntRemapInfo));
#endif

        DPRINT("uACPI-NT: GetVectorInput vector 0x%lx is GSIV %lu %s/%s\n",
               Vector,
               Index,
               UacpiNtGsivIsLevel(Index) ? "level" : "edge",
               UacpiNtGsivIsActiveLow(Index) ? "low" : "high");
        return STATUS_SUCCESS;
    }

    for (Index = 0; Index < UACPINT_MAX_MESSAGE_RUNS; Index++)
    {
        Run = &UacpiNtMessageRuns[Index];
        if (Run->Owner && Run->Count && Vector >= Run->Base && Vector < Run->Base + Run->Count)
        {
            DPRINT("uACPI-NT: GetVectorInput vector 0x%lx is a message (GSIV 0x%lx base 0x%lx count %lu)\n",
                   Vector,
                   Run->MessageGsiv,
                   Run->Base,
                   Run->Count);
            return STATUS_INVALID_PARAMETER;
        }
    }

    DPRINT("uACPI-NT: GetVectorInput vector 0x%lx is not ours\n", Vector);
    return STATUS_NOT_FOUND;
}

/*
 * HalAllocateMessageTarget override. Claims the lowest unclaimed arbiter run
 * for the owner, or allocates a HAL placed run when there is none.
 */
static
NTSTATUS
NTAPI
UacpiNtHalAllocateMessageTargetOverride(
    _In_ PDEVICE_OBJECT Owner,
#if (NTDDI_VERSION >= NTDDI_WIN7)
    _In_ PGROUP_AFFINITY ProcessorSet,
#else
    _In_ KAFFINITY ProcessorSet,
#endif
    _In_ ULONG NumberOfIdtEntries,
    _In_ KINTERRUPT_MODE Mode,
    _In_ BOOLEAN ShareVector,
    _Out_opt_ PULONG Vector,
    _Out_opt_ PKIRQL Irql,
    _Out_opt_ PULONG IdtEntry)
{
    PUACPINT_MESSAGE_RUN FreeRun = NULL;
    PUACPINT_MESSAGE_RUN Best = NULL;
    PUACPINT_MESSAGE_RUN Run;
    ULONG Count = NumberOfIdtEntries ? NumberOfIdtEntries : 1;
    ULONG Base = 0;
    ULONG Index;
    KIRQL BaseIrql;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(ProcessorSet);
    UNREFERENCED_PARAMETER(Mode);
    UNREFERENCED_PARAMETER(ShareVector);

    if (KeGetCurrentIrql() != PASSIVE_LEVEL)
        return STATUS_NOT_SUPPORTED;

    if (GlobalAcpiInterruptModel != ACPI_NT_APIC)
        return STATUS_NOT_SUPPORTED;

    ExAcquireFastMutex(&UacpiNtIrqLibLock);

    /* Lowest base first, matching the connection data order */
    for (Index = 0; Index < UACPINT_MAX_MESSAGE_RUNS; Index++)
    {
        Run = &UacpiNtMessageRuns[Index];
        if (Run->Owner == Owner &&
            Run->Count == Count &&
            !Run->Claimed &&
            (!Best || Run->Base < Best->Base))
        {
            Best = Run;
        }

        if (!FreeRun && !Run->Owner)
            FreeRun = Run;
    }

    if (Best)
    {
        Best->Claimed = TRUE;
        Base = Best->Base;
        Status = STATUS_SUCCESS;
        DPRINT("uACPI-NT: PDO %p claims message GSIV 0x%lx at vector 0x%lx (%lu entries)\n",
               Owner,
               Best->MessageGsiv,
               Base,
               Count);
    }
    else if (!FreeRun)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        Status = UacpiNtApicAllocateVectorRange(Count, &Base);
        if (NT_SUCCESS(Status))
        {
            FreeRun->Owner = Owner;
            FreeRun->MessageGsiv = 0;
            FreeRun->Base = Base;
            FreeRun->Count = Count;
            FreeRun->Claimed = TRUE;
        }
    }

    ExReleaseFastMutex(&UacpiNtIrqLibLock);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("uACPI-NT: MSI allocation of %lu entries for PDO %p failed 0x%lx\n",
                Count,
                Owner,
                Status);
        return Status;
    }

    BaseIrql = UacpiNtVectorToIrql(Base);
    if (Vector)
        *Vector = Base;
    if (IdtEntry)
        *IdtEntry = Base;
    if (Irql)
        *Irql = BaseIrql;

    DPRINT("uACPI-NT: MSI target PDO %p gets %lu vectors at 0x%lx IRQL %u\n",
           Owner,
           Count,
           Base,
           BaseIrql);
    return STATUS_SUCCESS;
}

/* HalFreeMessageTarget override. Arbiter placed runs persist, HAL placed ones are freed. */
static
VOID
NTAPI
UacpiNtHalFreeMessageTargetOverride(
    _In_ PDEVICE_OBJECT Owner,
    _In_ ULONG Vector,
#if (NTDDI_VERSION >= NTDDI_WIN7)
    _In_ PGROUP_AFFINITY ProcessorSet)
#else
    _In_ KAFFINITY ProcessorSet)
#endif
{
    PUACPINT_MESSAGE_RUN Run;
    ULONG Count = 0;
    ULONG Index;

    UNREFERENCED_PARAMETER(ProcessorSet);

    ExAcquireFastMutex(&UacpiNtIrqLibLock);
    for (Index = 0; Index < UACPINT_MAX_MESSAGE_RUNS; Index++)
    {
        Run = &UacpiNtMessageRuns[Index];
        if (Run->Owner != Owner || Vector < Run->Base || Vector >= Run->Base + Run->Count)
            continue;

        Count = Run->Count;
        Run->Claimed = FALSE;
        if (!Run->MessageGsiv)
        {
            UacpiNtApicFreeVectorRange(Run->Base, Count);
            RtlZeroMemory(Run, sizeof(*Run));
        }
        break;
    }
    ExReleaseFastMutex(&UacpiNtIrqLibLock);

    DPRINT("uACPI-NT: MSI free PDO %p vector 0x%lx (%lu vectors)\n", Owner, Vector, Count);
}

/* System wide, independent of IrqArbEnabled. The export is the table itself. */
static
VOID
NTAPI
UacpiNtInstallHalOverrides(VOID)
{
    UNICODE_STRING RoutineName;
    PULONG_PTR Table;
    ULONG Version;

    if (!UacpiNtIrqLibHalOverrides)
    {
        DPRINT1("uACPI-NT: HAL interrupt overrides disabled by IrqLibHalOverrides\n");
        return;
    }

    RtlInitUnicodeString(&RoutineName, L"HalPrivateDispatchTable");
    Table = MmGetSystemRoutineAddress(&RoutineName);
    if (!Table)
    {
        DPRINT1("uACPI-NT: HalPrivateDispatchTable not found, HAL interrupt overrides not installed\n");
        return;
    }

    Version = (ULONG)Table[0];

    /*
     * The HalGetInterruptVector export calls through this slot, so keep the
     * HAL's own routine or our lookups would land in the override.
     */
    if (Table[UACPINT_HAL_SLOT_GET_INTERRUPT_VECTOR] &&
        Table[UACPINT_HAL_SLOT_GET_INTERRUPT_VECTOR] != (ULONG_PTR)UacpiNtHalGetInterruptVectorOverride)
    {
        UacpiNtHalGetInterruptVector = (pHalGetInterruptVector)Table[UACPINT_HAL_SLOT_GET_INTERRUPT_VECTOR];
    }

    Table[UACPINT_HAL_SLOT_GET_INTERRUPT_VECTOR] = (ULONG_PTR)UacpiNtHalGetInterruptVectorOverride;
    Table[UACPINT_HAL_SLOT_GET_VECTOR_INPUT] = (ULONG_PTR)UacpiNtHalGetVectorInputOverride;

    if (Version >= UACPINT_HAL_PRIVATE_MSG_VERSION)
    {
        Table[UACPINT_HAL_SLOT_ALLOCATE_MSG_TARGET] = (ULONG_PTR)UacpiNtHalAllocateMessageTargetOverride;
        Table[UACPINT_HAL_SLOT_FREE_MSG_TARGET] = (ULONG_PTR)UacpiNtHalFreeMessageTargetOverride;
    }

    DPRINT("uACPI-NT: HalPrivateDispatchTable version %lu interrupt overrides installed%s\n",
           Version,
           Version >= UACPINT_HAL_PRIVATE_MSG_VERSION ? " with message targets" : "");
}

/* Called at FDO start after the PM handshake picked the interrupt model */
static
VOID
NTAPI
UacpiNtInitializeApic(VOID)
{
    UacpiNtSeedExistingProcessors();

    /* Only hot added processors depend on this, a NULL handle is fine */
    if (UacpiNtKeRegisterProcessorChangeCallback)
    {
        UacpiNtProcessorChangeHandle =
            UacpiNtKeRegisterProcessorChangeCallback(UacpiNtProcessorChangeCallback,
                                                     NULL,
                                                     KE_PROCESSOR_CHANGE_ADD_EXISTING);
    }

    if (!UacpiNtProcessorChangeHandle)
        DPRINT1("uACPI-NT: no processor change callback, hot added processors are not tracked\n");

    DPRINT("uACPI-NT: APIC model, %lu processor IDT sets, HalConvertDeviceIdtToIrql %p\n",
           UacpiNtProcessorIdtCount,
           (PVOID)(ULONG_PTR)UacpiNtHalConvertDeviceIdtToIrql);

    if (!UacpiNtProcessorIdtCount)
    {
        DPRINT1("uACPI-NT: no processor IDT sets\n");
        KeBugCheckEx(ACPI_DRIVER_INTERNAL, 0x22, 0, 0, 0);
    }

    UacpiNtInstallHalOverrides();
}

/* The PIC base is the HAL vector of the SCI line minus its GSI */
static
VOID
NTAPI
UacpiNtInitializePic(VOID)
{
    struct acpi_fadt *Fadt = NULL;
    KAFFINITY SciAffinity = 0;
    KIRQL SciIrql = 0;
    ULONG SciGsi = MAXULONG;
    ULONG SciVector = 0;

    if (uacpi_likely_success(uacpi_table_fadt(&Fadt)) && Fadt)
        SciGsi = Fadt->sci_int;

    if (SciGsi != MAXULONG && UacpiNtHalGetInterruptVector)
        SciVector = UacpiNtHalGetInterruptVector(Isa, 0, SciGsi, SciGsi, &SciIrql, &SciAffinity);

    if (SciVector && SciVector >= SciGsi)
    {
        UacpiNtPicVectorBase = SciVector - SciGsi;
        DPRINT("uACPI-NT: PIC model, vector base 0x%lx from SCI GSI %lu vector 0x%lx IRQL %u\n",
               UacpiNtPicVectorBase,
               SciGsi,
               SciVector,
               SciIrql);
    }
    else
    {
        /* Classic PC/AT layout */
        UacpiNtPicVectorBase = 0x30;
        DPRINT1("uACPI-NT: HAL has no vector for SCI GSI %lu (0x%lx), PIC base defaults to 0x30\n",
                SciGsi,
                SciVector);
    }

    UacpiNtPicBaseValid = TRUE;
}

NTSTATUS
NTAPI
UacpiNtIrqLibInitialize(VOID)
{
    UNICODE_STRING RoutineName;

    if (UacpiNtIrqLibReady)
        return STATUS_SUCCESS;

    ExInitializeFastMutex(&UacpiNtIrqLibLock);
    UacpiNtResolveKernelRoutines();

    /* Overrides must be known before any GSIV is resolved */
    UacpiNtParseMadt();

    RtlInitUnicodeString(&RoutineName, L"HalGetInterruptVector");
    UacpiNtHalGetInterruptVector = (pHalGetInterruptVector)MmGetSystemRoutineAddress(&RoutineName);

    RtlInitUnicodeString(&RoutineName, L"HalConvertDeviceIdtToIrql");
    UacpiNtHalConvertDeviceIdtToIrql = (PUACPINT_HAL_CONVERT_IDT_TO_IRQL)MmGetSystemRoutineAddress(&RoutineName);

    if (GlobalAcpiInterruptModel == ACPI_NT_APIC)
        UacpiNtInitializeApic();
    else
        UacpiNtInitializePic();

    UacpiNtIrqLibReady = TRUE;
    return STATUS_SUCCESS;
}

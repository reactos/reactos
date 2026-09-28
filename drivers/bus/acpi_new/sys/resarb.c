/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Memory, I/O port and bus number arbiters for module devices
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <uacpint.h>
#include <ndk/rtlfuncs.h>
#include <wdmguid.h>
#include "arbiter.h"
#include <debug.h>

#ifndef CM_RESOURCE_PORT_10_BIT_DECODE
#define CM_RESOURCE_PORT_10_BIT_DECODE 0x0004
#endif

#ifndef CM_RESOURCE_PORT_12_BIT_DECODE
#define CM_RESOURCE_PORT_12_BIT_DECODE 0x0008
#endif

/* Tags space the root does not decode, as opposed to space already taken */
#define UACPINT_RANGE_OUTSIDE_DECODE    0x20

/* Top of the 16 bit I/O space */
#define UACPINT_IO_SPACE_LAST           0xFFFF

/* Windows remembered for the placement failure dump */
#define UACPINT_RES_TRY_RING            24

/* Caps on the failure dump */
#define UACPINT_RES_DUMP_ALTERNATIVES   8
#define UACPINT_RES_DUMP_RANGES         96
#define UACPINT_RES_DUMP_ORDERINGS      32

/* Per device arbiters, created on the first query and hung off UACPINT_PDO.ResArb */
typedef struct _UACPINT_RES_ARBITERS
{
    ARBITER_INSTANCE Memory;
    ARBITER_INSTANCE Port;
    ARBITER_INSTANCE BusNumber;
    BOOLEAN          MemoryReady;
    BOOLEAN          PortReady;
    BOOLEAN          BusNumberReady;
} UACPINT_RES_ARBITERS, *PUACPINT_RES_ARBITERS;

typedef struct _UACPINT_RES_TRY
{
    ULONGLONG Minimum;
    ULONGLONG Maximum;
    ULONGLONG Length;
    ULONGLONG Alignment;
    ULONG     Attributes;
    ULONGLONG Placed;
    BOOLEAN   Ok;
} UACPINT_RES_TRY, *PUACPINT_RES_TRY;

ULONG UacpiNtResArbEnabled = 1;

/* Per window placement tracing, high volume */
static ULONG UacpiNtResTraceWindows = 1;

static UACPINT_RES_TRY UacpiNtResTryRing[UACPINT_RES_TRY_RING];
static ULONG UacpiNtResTryIndex;

/* Memory, MemoryLarge, port and bus number all unpack to a plain range */
static
NTSTATUS
NTAPI
UacpiNtResUnpackRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR Descriptor,
    _Out_ PUINT64 Minimum,
    _Out_ PUINT64 Maximum,
    _Out_ PUINT64 Length,
    _Out_ PUINT64 Alignment)
{
    if (!Descriptor)
        return STATUS_INVALID_PARAMETER;

    switch (Descriptor->Type)
    {
        case CmResourceTypeMemory:
        case CmResourceTypeMemoryLarge:
            *Minimum = (ULONGLONG)Descriptor->u.Memory.MinimumAddress.QuadPart;
            *Maximum = (ULONGLONG)Descriptor->u.Memory.MaximumAddress.QuadPart;
            *Length = Descriptor->u.Memory.Length;
            *Alignment = Descriptor->u.Memory.Alignment ? Descriptor->u.Memory.Alignment : 1;
            return STATUS_SUCCESS;

        case CmResourceTypePort:
            *Minimum = (ULONGLONG)Descriptor->u.Port.MinimumAddress.QuadPart;
            *Maximum = (ULONGLONG)Descriptor->u.Port.MaximumAddress.QuadPart;
            *Length = Descriptor->u.Port.Length;
            *Alignment = Descriptor->u.Port.Alignment ? Descriptor->u.Port.Alignment : 1;
            return STATUS_SUCCESS;

        case CmResourceTypeBusNumber:
            *Minimum = Descriptor->u.BusNumber.MinBusNumber;
            *Maximum = Descriptor->u.BusNumber.MaxBusNumber;
            *Length = Descriptor->u.BusNumber.Length;
            *Alignment = 1;
            return STATUS_SUCCESS;

        default:
            return STATUS_INVALID_PARAMETER;
    }
}

static
NTSTATUS
NTAPI
UacpiNtResPackResource(
    _In_ PIO_RESOURCE_DESCRIPTOR Requirement,
    _In_ UINT64 Start,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor)
{
    if (!Requirement || !Descriptor)
        return STATUS_INVALID_PARAMETER;

    Descriptor->ShareDisposition = Requirement->ShareDisposition;
    Descriptor->Flags = Requirement->Flags;

    switch (Requirement->Type)
    {
        case CmResourceTypeMemory:
        case CmResourceTypeMemoryLarge:
            Descriptor->Type = CmResourceTypeMemory;
            Descriptor->u.Memory.Start.QuadPart = (LONGLONG)Start;
            Descriptor->u.Memory.Length = Requirement->u.Memory.Length;
            return STATUS_SUCCESS;

        case CmResourceTypePort:
            Descriptor->Type = CmResourceTypePort;
            Descriptor->u.Port.Start.QuadPart = (LONGLONG)Start;
            Descriptor->u.Port.Length = Requirement->u.Port.Length;
            return STATUS_SUCCESS;

        case CmResourceTypeBusNumber:
            Descriptor->Type = CmResourceTypeBusNumber;
            Descriptor->u.BusNumber.Start = (ULONG)Start;
            Descriptor->u.BusNumber.Length = Requirement->u.BusNumber.Length;
            return STATUS_SUCCESS;

        default:
            return STATUS_INVALID_PARAMETER;
    }
}

static
NTSTATUS
NTAPI
UacpiNtResUnpackResource(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    _Out_ PUINT64 Start,
    _Out_ PUINT64 Length)
{
    if (!Descriptor)
        return STATUS_INVALID_PARAMETER;

    switch (Descriptor->Type)
    {
        case CmResourceTypeMemory:
        case CmResourceTypeMemoryLarge:
            *Start = (ULONGLONG)Descriptor->u.Memory.Start.QuadPart;
            *Length = Descriptor->u.Memory.Length;
            return STATUS_SUCCESS;

        case CmResourceTypePort:
            *Start = (ULONGLONG)Descriptor->u.Port.Start.QuadPart;
            *Length = Descriptor->u.Port.Length;
            return STATUS_SUCCESS;

        case CmResourceTypeBusNumber:
            *Start = Descriptor->u.BusNumber.Start;
            *Length = Descriptor->u.BusNumber.Length;
            return STATUS_SUCCESS;

        default:
            return STATUS_INVALID_PARAMETER;
    }
}

/* Score is the number of placements, saturated. Unsatisfiable requests go first. */
static
INT32
NTAPI
UacpiNtResScoreRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR Descriptor)
{
    ULONGLONG Minimum = 0;
    ULONGLONG Maximum = 0;
    ULONGLONG Length = 0;
    ULONGLONG Alignment = 0;

    if (!NT_SUCCESS(UacpiNtResUnpackRequirement(Descriptor, &Minimum, &Maximum, &Length, &Alignment)) ||
        Maximum < Minimum)
    {
        return -1;
    }

    if (Maximum - Minimum >= (ULONGLONG)MAXLONG)
        return MAXLONG;

    return (INT32)(Maximum - Minimum + 1);
}

/* Start and length of a producer address descriptor of WindowType */
static
BOOLEAN
NTAPI
UacpiNtProducerWindow(
    _In_ uacpi_resource *Resource,
    _In_ UCHAR WindowType,
    _Out_ PULONGLONG Start,
    _Out_ PULONGLONG Length)
{
    uacpi_resource_address_common *Common;

    switch (Resource->type)
    {
        case UACPI_RESOURCE_TYPE_ADDRESS16:
            Common = &Resource->address16.common;
            *Start = Resource->address16.minimum;
            *Length = Resource->address16.address_length;
            break;

        case UACPI_RESOURCE_TYPE_ADDRESS32:
            Common = &Resource->address32.common;
            *Start = Resource->address32.minimum;
            *Length = Resource->address32.address_length;
            break;

        case UACPI_RESOURCE_TYPE_ADDRESS64:
            Common = &Resource->address64.common;
            *Start = Resource->address64.minimum;
            *Length = Resource->address64.address_length;
            break;

        case UACPI_RESOURCE_TYPE_ADDRESS64_EXTENDED:
            Common = &Resource->address64_extended.common;
            *Start = Resource->address64_extended.minimum;
            *Length = Resource->address64_extended.address_length;
            break;

        default:
            return FALSE;
    }

    return (BOOLEAN)(Common->direction == UACPI_PRODUCER &&
                     Common->type == WindowType &&
                     *Length != 0);
}

/*
 * Everything outside the root's producer windows goes into Allocation,
 * so only the windows are left to hand out.
 */
static
VOID
NTAPI
UacpiNtSeedArbiter(
    _Inout_ PARBITER_INSTANCE Arbiter,
    _In_ uacpi_namespace_node *Node,
    _In_ UCHAR WindowType,
    _In_z_ PCSTR Label)
{
    uacpi_resources *Resources = NULL;
    uacpi_resource *Resource;
    RTL_RANGE_LIST_ITERATOR Iterator;
    RTL_RANGE_LIST Windows;
    RTL_RANGE_LIST Gaps;
    PRTL_RANGE Gap;
    ULONGLONG Start;
    ULONGLONG Length;
    NTSTATUS Status;
    ULONG Count = 0;

    DPRINT("uACPI-NT: %s evaluating _CRS\n", Label);
    if (uacpi_unlikely_error(uacpi_get_current_resources(Node, &Resources)) || !Resources)
    {
        DPRINT1("uACPI-NT: %s _CRS unavailable\n", Label);
        return;
    }

    RtlInitializeRangeList(&Windows);
    for (Resource = Resources->entries;
         Resource->type != UACPI_RESOURCE_TYPE_END_TAG;
         Resource = UACPI_NEXT_RESOURCE(Resource))
    {
        if (!UacpiNtProducerWindow(Resource, WindowType, &Start, &Length))
            continue;

        /* Overlapping windows describe one decode, not a conflict */
        RtlAddRange(&Windows,
                    Start,
                    Start + Length - 1,
                    0,
                    RTL_RANGE_LIST_ADD_IF_CONFLICT | RTL_RANGE_LIST_ADD_SHARED,
                    NULL,
                    NULL);
        DPRINT("uACPI-NT: %s window %010I64X-%010I64X\n", Label, Start, Start + Length - 1);
        Count++;
    }

    /* Without a window of this type leave the pool open rather than block everything */
    if (Count == 0)
    {
        DPRINT("uACPI-NT: %s has no producer window, pool left open\n", Label);
        RtlFreeRangeList(&Windows);
        uacpi_free_resources(Resources);
        return;
    }

    RtlInitializeRangeList(&Gaps);
    Status = RtlInvertRangeList(&Gaps, &Windows);
    if (NT_SUCCESS(Status) && NT_SUCCESS(RtlGetFirstRange(&Gaps, &Iterator, &Gap)))
    {
        do
        {
            Status = RtlAddRange(Arbiter->Allocation,
                                 Gap->Start,
                                 Gap->End,
                                 UACPINT_RANGE_OUTSIDE_DECODE,
                                 RTL_RANGE_LIST_ADD_IF_CONFLICT,
                                 NULL,
                                 NULL);
        } while (NT_SUCCESS(Status) && NT_SUCCESS(RtlGetNextRange(&Iterator, &Gap, TRUE)));
    }
    RtlFreeRangeList(&Gaps);

    /* A partial pool is worse than none */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("uACPI-NT: %s bounding failed 0x%lx, pool left open\n", Label, Status);
        RtlFreeRangeList(Arbiter->Allocation);
        RtlInitializeRangeList(Arbiter->Allocation);
    }
    else
    {
        DPRINT("uACPI-NT: %s pool bounded by %u window(s)\n", Label, Count);
    }

    RtlFreeRangeList(&Windows);
    uacpi_free_resources(Resources);
}

/* Unlocked on purpose, a lock would change the arbitration being traced */
static
VOID
NTAPI
UacpiNtRecordTry(
    _In_ PARBITER_ALLOCATION_STATE State,
    _In_ BOOLEAN Ok)
{
    PUACPINT_RES_TRY Try;

    Try = &UacpiNtResTryRing[UacpiNtResTryIndex++ % UACPINT_RES_TRY_RING];
    Try->Minimum = State->CurrentMinimum;
    Try->Maximum = State->CurrentMaximum;
    Try->Length = State->CurrentAlternative ? State->CurrentAlternative->Length : 0;
    Try->Alignment = State->CurrentAlternative ? State->CurrentAlternative->Alignment : 0;
    Try->Attributes = State->RangeAvailableAttributes;
    Try->Placed = Ok ? State->Start : 0;
    Try->Ok = Ok;
}

static
VOID
NTAPI
UacpiNtDumpTryRing(VOID)
{
    PUACPINT_RES_TRY Try;
    ULONG Count = UacpiNtResTryIndex;
    ULONG First;
    ULONG Index;

    if (Count == 0)
    {
        DPRINT1("uACPI-NT:   no window was ever offered\n");
        return;
    }

    First = (Count > UACPINT_RES_TRY_RING) ? (Count - UACPINT_RES_TRY_RING) : 0;
    for (Index = First; Index < Count; Index++)
    {
        Try = &UacpiNtResTryRing[Index % UACPINT_RES_TRY_RING];
        if (Try->Ok)
        {
            DPRINT1("uACPI-NT:   tried %010I64X-%010I64X length %I64X align %I64X avail 0x%X, placed at %010I64X\n",
                    Try->Minimum,
                    Try->Maximum,
                    Try->Length,
                    Try->Alignment,
                    Try->Attributes,
                    Try->Placed);
        }
        else
        {
            DPRINT1("uACPI-NT:   tried %010I64X-%010I64X length %I64X align %I64X avail 0x%X, refused\n",
                    Try->Minimum,
                    Try->Maximum,
                    Try->Length,
                    Try->Alignment,
                    Try->Attributes);
        }
    }
}

static
PCWSTR
NTAPI
UacpiNtArbiterName(
    _In_opt_ PARBITER_INSTANCE Arbiter)
{
    return (Arbiter && Arbiter->Name) ? Arbiter->Name : L"?";
}

/* First hardware ID of a range owner, for the failure dump */
static
VOID
NTAPI
UacpiNtNameOwner(
    _In_opt_ PVOID Owner,
    _Out_writes_z_(BufferSize) PCHAR Buffer,
    _In_ ULONG BufferSize)
{
    PWSTR HardwareIds;
    ULONG Needed = 0;
    NTSTATUS Status;

    Buffer[0] = ANSI_NULL;

    if (!Owner)
    {
        RtlStringCbCopyA(Buffer, BufferSize, "(unowned)");
        return;
    }

    if (KeGetCurrentIrql() != PASSIVE_LEVEL)
    {
        RtlStringCbCopyA(Buffer, BufferSize, "(irql)");
        return;
    }

    Status = IoGetDeviceProperty(Owner, DevicePropertyHardwareID, 0, NULL, &Needed);
    if (Status != STATUS_BUFFER_TOO_SMALL || Needed == 0 || Needed > 4096)
    {
        RtlStringCbPrintfA(Buffer, BufferSize, "(no hwid 0x%lx)", Status);
        return;
    }

    HardwareIds = ExAllocatePoolWithTag(PagedPool, Needed, UACPINT_POOL_TAG);
    if (!HardwareIds)
    {
        RtlStringCbCopyA(Buffer, BufferSize, "(no pool)");
        return;
    }

    Status = IoGetDeviceProperty(Owner, DevicePropertyHardwareID, Needed, HardwareIds, &Needed);
    if (NT_SUCCESS(Status))
        RtlStringCbPrintfA(Buffer, BufferSize, "%S", HardwareIds);
    else
        RtlStringCbPrintfA(Buffer, BufferSize, "(hwid 0x%lx)", Status);

    ExFreePoolWithTag(HardwareIds, UACPINT_POOL_TAG);
}

static
VOID
NTAPI
UacpiNtDumpPool(
    _In_opt_ PRTL_RANGE_LIST List,
    _In_z_ PCSTR Label)
{
    RTL_RANGE_LIST_ITERATOR Iterator;
    PRTL_RANGE Range;
    CHAR Owner[96];
    ULONG Shown = 0;

    if (!List || !NT_SUCCESS(RtlGetFirstRange(List, &Iterator, &Range)))
    {
        DPRINT1("uACPI-NT:   %s pool is empty\n", Label);
        return;
    }

    do
    {
        UacpiNtNameOwner(Range->Owner, Owner, sizeof(Owner));
        DPRINT1("uACPI-NT:   %s held %010I64X-%010I64X owner %p [%s] attributes 0x%X flags 0x%X\n",
                Label,
                Range->Start,
                Range->End,
                Range->Owner,
                Owner,
                Range->Attributes,
                Range->Flags);

        if (++Shown >= UACPINT_RES_DUMP_RANGES)
        {
            DPRINT1("uACPI-NT:   more ranges not shown\n");
            break;
        }
    } while (NT_SUCCESS(RtlGetNextRange(&Iterator, &Range, TRUE)));
}

static
VOID
NTAPI
UacpiNtDumpOrderings(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ ULONG Limit,
    _In_ BOOLEAN Failure)
{
    ULONG Index;

    for (Index = 0; Index < Arbiter->OrderingList.Count && Index < Limit; Index++)
    {
        if (Failure)
        {
            DPRINT1("uACPI-NT:     order[%u] %010I64X-%010I64X\n",
                    Index,
                    Arbiter->OrderingList.Orderings[Index].Start,
                    Arbiter->OrderingList.Orderings[Index].End);
        }
        else
        {
            DPRINT("uACPI-NT:     order[%u] %010I64X-%010I64X\n",
                   Index,
                   Arbiter->OrderingList.Orderings[Index].Start,
                   Arbiter->OrderingList.Orderings[Index].End);
        }
    }

    for (Index = 0; Index < Arbiter->ReservedList.Count && Index < Limit; Index++)
    {
        if (Failure)
        {
            DPRINT1("uACPI-NT:     reserved[%u] %010I64X-%010I64X\n",
                    Index,
                    Arbiter->ReservedList.Orderings[Index].Start,
                    Arbiter->ReservedList.Orderings[Index].End);
        }
        else
        {
            DPRINT("uACPI-NT:     reserved[%u] %010I64X-%010I64X\n",
                   Index,
                   Arbiter->ReservedList.Orderings[Index].Start,
                   Arbiter->ReservedList.Orderings[Index].End);
        }
    }
}

static
VOID
NTAPI
UacpiNtDumpFailure(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_opt_ PLIST_ENTRY ArbitrationList)
{
    PIO_RESOURCE_DESCRIPTOR Descriptor;
    PARBITER_LIST_ENTRY Entry;
    PLIST_ENTRY Link;
    ULONGLONG Minimum;
    ULONGLONG Maximum;
    ULONGLONG Length;
    ULONGLONG Alignment;
    ULONG Index;

    if (ArbitrationList)
    {
        for (Link = ArbitrationList->Flink;
             Link != ArbitrationList;
             Link = Link->Flink)
        {
            Entry = CONTAINING_RECORD(Link, ARBITER_LIST_ENTRY, ListEntry);
            DPRINT1("uACPI-NT:   PDO %p source %u flags 0x%X wants one of %u alternative(s)\n",
                    Entry->PhysicalDeviceObject,
                    Entry->RequestSource,
                    Entry->Flags,
                    Entry->AlternativeCount);

            for (Index = 0;
                 Index < Entry->AlternativeCount && Index < UACPINT_RES_DUMP_ALTERNATIVES;
                 Index++)
            {
                Descriptor = &Entry->Alternatives[Index];
                if (!NT_SUCCESS(UacpiNtResUnpackRequirement(Descriptor, &Minimum, &Maximum, &Length, &Alignment)))
                {
                    DPRINT1("uACPI-NT:     [%u] type %u cannot be unpacked\n", Index, Descriptor->Type);
                    continue;
                }

                DPRINT1("uACPI-NT:     [%u] type %u option 0x%X flags 0x%X %010I64X-%010I64X length %I64X align %I64X\n",
                        Index,
                        Descriptor->Type,
                        Descriptor->Option,
                        Descriptor->Flags,
                        Minimum,
                        Maximum,
                        Length,
                        Alignment);
            }
        }
    }

    /* PossibleAllocation is already gone on this path */
    UacpiNtDumpPool(Arbiter->Allocation, "committed");
    UacpiNtDumpTryRing();

    DPRINT1("uACPI-NT:   %u ordering window(s), %u reserved\n",
            Arbiter->OrderingList.Count,
            Arbiter->ReservedList.Count);
    UacpiNtDumpOrderings(Arbiter, UACPINT_RES_DUMP_ORDERINGS, TRUE);
}

/* Dump the tentative pool before the failed test frees it */
static
NTSTATUS
NTAPI
UacpiNtResAllocateEntry(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE State)
{
    NTSTATUS Status;

    Status = ArbiterLibAllocateEntry(Arbiter, State);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("uACPI-NT: %ws AllocateEntry failed 0x%lx, pool as the solver saw it:\n",
                UacpiNtArbiterName(Arbiter),
                Status);
        UacpiNtDumpPool(Arbiter->PossibleAllocation, "tentative");
    }

    return Status;
}

static
NTSTATUS
NTAPI
UacpiNtResTestAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
#if (NTDDI_VERSION >= NTDDI_VISTA)
    _Inout_ PARBITER_TEST_ALLOCATION_PARAMETERS Parameters)
#else
    _Inout_ PLIST_ENTRY ArbitrationList)
#endif
{
    NTSTATUS Status;
#if (NTDDI_VERSION >= NTDDI_VISTA)
    PLIST_ENTRY ArbitrationList = Parameters->ArbitrationList;
#endif

    /* A failure dump covers only this arbitration */
    UacpiNtResTryIndex = 0;

#if (NTDDI_VERSION >= NTDDI_VISTA)
    Status = ArbiterLibTestAllocation(Arbiter, Parameters);
#else
    Status = ArbiterLibTestAllocation(Arbiter, ArbitrationList);
#endif
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("uACPI-NT: %ws TestAllocation failed 0x%lx, no placement\n",
                UacpiNtArbiterName(Arbiter),
                Status);
        UacpiNtDumpFailure(Arbiter, ArbitrationList);
    }
    else if (UacpiNtResVerbose)
    {
        DPRINT("uACPI-NT: %ws TestAllocation placed\n", UacpiNtArbiterName(Arbiter));
    }

    return Status;
}

static
NTSTATUS
NTAPI
UacpiNtResRollbackAllocation(
    _In_ PARBITER_INSTANCE Arbiter)
{
    if (UacpiNtResVerbose)
        DPRINT("uACPI-NT: %ws RollbackAllocation\n", UacpiNtArbiterName(Arbiter));

    return ArbiterLibRollbackAllocation(Arbiter);
}

static
NTSTATUS
NTAPI
UacpiNtResCommitAllocation(
    _In_ PARBITER_INSTANCE Arbiter)
{
    NTSTATUS Status;

    Status = ArbiterLibCommitAllocation(Arbiter);
    if (UacpiNtResVerbose)
        DPRINT("uACPI-NT: %ws CommitAllocation 0x%lx\n", UacpiNtArbiterName(Arbiter), Status);

    return Status;
}

/*
 * Boot configs may land on boot allocated space, which fixed ports need.
 * A fixed boot config may also sit outside the root's decode.
 */
static
BOOLEAN
NTAPI
UacpiNtResFindSuitableRange(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE State)
{
    BOOLEAN Ok;

    if (State && State->Entry && (State->Entry->Flags & ARBITER_FLAG_BOOT_CONFIG))
    {
        State->RangeAvailableAttributes |= ARBITER_RANGE_BOOT_ALLOCATED;

        if (State->CurrentAlternative &&
            (State->CurrentAlternative->Flags & ARBITER_ALTERNATIVE_FLAG_FIXED))
        {
            State->RangeAvailableAttributes |= UACPINT_RANGE_OUTSIDE_DECODE;
        }
    }

    Ok = ArbiterLibFindSuitableRange(Arbiter, State);
    if (!State)
        return Ok;

    UacpiNtRecordTry(State, Ok);

    if (UacpiNtResTraceWindows)
    {
        DPRINT("uACPI-NT:   try %010I64X-%010I64X length %I64X align %I64X avail 0x%X, %s\n",
               State->CurrentMinimum,
               State->CurrentMaximum,
               State->CurrentAlternative ? State->CurrentAlternative->Length : 0,
               State->CurrentAlternative ? State->CurrentAlternative->Alignment : 0,
               State->RangeAvailableAttributes,
               Ok ? "ok" : "no");

        if (Ok)
            DPRINT("uACPI-NT:        placed %010I64X-%010I64X\n", State->Start, State->End);
    }

    return Ok;
}

/* Next ISA decode mirror after Last, FALSE for a full 16 bit decoder or past the top */
static
BOOLEAN
NTAPI
UacpiNtNextPortAlias(
    _In_ ULONG DescriptorFlags,
    _In_ ULONGLONG Last,
    _Out_ PULONGLONG Alias)
{
    ULONGLONG Next;

    if (DescriptorFlags & CM_RESOURCE_PORT_10_BIT_DECODE)
        Next = Last + 0x400;
    else if (DescriptorFlags & CM_RESOURCE_PORT_12_BIT_DECODE)
        Next = Last + 0x1000;
    else
        return FALSE;

    if (Next > UACPINT_IO_SPACE_LAST || Next < Last)
        return FALSE;

    *Alias = Next;
    return TRUE;
}

static
BOOLEAN
NTAPI
UacpiNtPortAliasesAvailable(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PARBITER_ALLOCATION_STATE State)
{
    PARBITER_ALTERNATIVE Alternative = State->CurrentAlternative;
    ARBITER_ALLOCATION_STATE Probe;
    ULONGLONG Alias;
    ULONGLONG Last;
    BOOLEAN Available;
    ULONG Flags;

    if (!Alternative || !Alternative->Descriptor || Alternative->Length == 0)
        return TRUE;

    Flags = RTL_RANGE_LIST_NULL_CONFLICT_OK;
    if (Alternative->Flags & ARBITER_ALTERNATIVE_FLAG_SHARED)
        Flags |= RTL_RANGE_LIST_SHARED_OK;

    for (Last = State->Start;
         UacpiNtNextPortAlias(Alternative->Descriptor->Flags, Last, &Alias);
         Last = Alias)
    {
        Available = FALSE;
        RtlIsRangeAvailable(Arbiter->PossibleAllocation,
                            Alias,
                            Alias + Alternative->Length - 1,
                            Flags,
                            State->RangeAvailableAttributes,
                            Arbiter->ConflictCallbackContext,
                            Arbiter->ConflictCallback,
                            &Available);
        if (Available)
            continue;

        /* Let OverrideConflict judge the mirror on a copy of the state */
        Probe = *State;
        Probe.CurrentMinimum = Alias;
        Probe.CurrentMaximum = Alias + Alternative->Length - 1;
        if (!Arbiter->OverrideConflict || !Arbiter->OverrideConflict(Arbiter, &Probe))
            return FALSE;
    }

    return TRUE;
}

/* Skip placements whose decode mirrors are taken */
static
BOOLEAN
NTAPI
UacpiNtResPortFindSuitableRange(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE State)
{
    ULONGLONG Next;

    while (UacpiNtResFindSuitableRange(Arbiter, State))
    {
        if (UacpiNtPortAliasesAvailable(Arbiter, State))
            return TRUE;

        if (!State->CurrentAlternative || State->CurrentAlternative->Length == 0)
            return FALSE;

        Next = State->Start + State->CurrentAlternative->Length;
        if (Next <= State->Start || Next > State->CurrentMaximum)
            return FALSE;

        State->CurrentMinimum = Next;
    }

    return FALSE;
}

/* Claim the placement, then each of its mirrors for the same PDO */
static
VOID
NTAPI
UacpiNtResPortAddAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE State)
{
    PARBITER_ALTERNATIVE Alternative = State->CurrentAlternative;
    PDEVICE_OBJECT Owner;
    ULONGLONG Alias;
    ULONGLONG Last;
    ULONG Flags;

    Owner = State->Entry ? State->Entry->PhysicalDeviceObject : NULL;

    ArbiterLibAddAllocation(Arbiter, State);

    if (!Alternative || !Alternative->Descriptor || Alternative->Length == 0)
        return;

    Flags = RTL_RANGE_LIST_ADD_IF_CONFLICT;
    if (Alternative->Flags & ARBITER_ALTERNATIVE_FLAG_SHARED)
        Flags |= RTL_RANGE_LIST_ADD_SHARED;

    for (Last = State->Start;
         UacpiNtNextPortAlias(Alternative->Descriptor->Flags, Last, &Alias);
         Last = Alias)
    {
        RtlAddRange(Arbiter->PossibleAllocation,
                    Alias,
                    Alias + Alternative->Length - 1,
                    (UCHAR)(State->RangeAttributes | ARBITER_RANGE_PORT_ALIAS),
                    Flags,
                    NULL,
                    Owner);
    }
}

static
VOID
NTAPI
UacpiNtResPortBacktrackAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE State)
{
    PARBITER_ALTERNATIVE Alternative = State->CurrentAlternative;
    PDEVICE_OBJECT Owner;
    ULONGLONG Alias;
    ULONGLONG Last;

    Owner = State->Entry ? State->Entry->PhysicalDeviceObject : NULL;

    if (Alternative && Alternative->Descriptor && Alternative->Length != 0)
    {
        for (Last = State->Start;
             UacpiNtNextPortAlias(Alternative->Descriptor->Flags, Last, &Alias);
             Last = Alias)
        {
            RtlDeleteRange(Arbiter->PossibleAllocation, Alias, Alias + Alternative->Length - 1, Owner);
        }
    }

    ArbiterLibBacktrackAllocation(Arbiter, State);
}

static
NTSTATUS
NTAPI
UacpiNtInitRootArbiter(
    _Out_ PARBITER_INSTANCE Arbiter,
    _In_ PUACPINT_PDO Pdo,
    _In_ CM_RESOURCE_TYPE ResourceType,
    _In_ UCHAR WindowType,
    _In_z_ PCWSTR Name,
    _In_z_ PCSTR Label)
{
    NTSTATUS Status;

    RtlZeroMemory(Arbiter, sizeof(*Arbiter));

    /* The library only fills slots left NULL, bus numbers keep its default search */
    Arbiter->UnpackRequirement = UacpiNtResUnpackRequirement;
    Arbiter->PackResource = UacpiNtResPackResource;
    Arbiter->UnpackResource = UacpiNtResUnpackResource;
    Arbiter->ScoreRequirement = UacpiNtResScoreRequirement;
    Arbiter->AllocateEntry = UacpiNtResAllocateEntry;
    Arbiter->TestAllocation = UacpiNtResTestAllocation;
    Arbiter->RollbackAllocation = UacpiNtResRollbackAllocation;
    Arbiter->CommitAllocation = UacpiNtResCommitAllocation;

    if (ResourceType == CmResourceTypeMemory)
    {
        Arbiter->FindSuitableRange = UacpiNtResFindSuitableRange;
    }
    else if (ResourceType == CmResourceTypePort)
    {
        Arbiter->FindSuitableRange = UacpiNtResPortFindSuitableRange;
        Arbiter->AddAllocation = UacpiNtResPortAddAllocation;
        Arbiter->BacktrackAllocation = UacpiNtResPortBacktrackAllocation;
    }

    Status = ArbiterLibInitializeInstance(Arbiter, Pdo->Shared.Self, ResourceType, Name, L"Root", NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("uACPI-NT: %s arbiter initialization failed 0x%lx\n", Label, Status);
        return Status;
    }

    UacpiNtSeedArbiter(Arbiter, Pdo->Node, WindowType, Label);

    /* Block inaccessible memory and boot reserve the MMCONFIG window */
    if (ResourceType == CmResourceTypeMemory)
    {
        ArbiterLibAddInaccessibleAllocationRange(Arbiter, L"Root", Arbiter->Allocation);
        ArbiterLibAddMmConfigRangeAsBootReserved(Arbiter, Arbiter->Allocation);
    }

    DPRINT("uACPI-NT: %s seeded\n", Label);

    if (UacpiNtResVerbose)
    {
        DPRINT("uACPI-NT: %s ordering list has %u window(s), %u reserved\n",
               Label,
               Arbiter->OrderingList.Count,
               Arbiter->ReservedList.Count);
        UacpiNtDumpOrderings(Arbiter, MAXULONG, FALSE);
    }

    return STATUS_SUCCESS;
}

static
PUACPINT_RES_ARBITERS
NTAPI
UacpiNtEnsureResArbiters(
    _Inout_ PUACPINT_PDO Pdo)
{
    PUACPINT_RES_ARBITERS Arbiters = Pdo->ResArb;

    if (Arbiters)
        return Arbiters;

    DPRINT("uACPI-NT: %s building arbiters\n", Pdo->Name);

    Arbiters = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Arbiters), UACPINT_POOL_TAG);
    if (!Arbiters)
        return NULL;

    RtlZeroMemory(Arbiters, sizeof(*Arbiters));

    Arbiters->MemoryReady = NT_SUCCESS(UacpiNtInitRootArbiter(&Arbiters->Memory,
                                                              Pdo,
                                                              CmResourceTypeMemory,
                                                              UACPI_RANGE_MEMORY,
                                                              L"ACPI_Memory",
                                                              "MEM"));
    Arbiters->PortReady = NT_SUCCESS(UacpiNtInitRootArbiter(&Arbiters->Port,
                                                            Pdo,
                                                            CmResourceTypePort,
                                                            UACPI_RANGE_IO,
                                                            L"ACPI_Port",
                                                            "IO"));
    Arbiters->BusNumberReady = NT_SUCCESS(UacpiNtInitRootArbiter(&Arbiters->BusNumber,
                                                                 Pdo,
                                                                 CmResourceTypeBusNumber,
                                                                 UACPI_RANGE_BUS,
                                                                 L"ACPI_BusNumber",
                                                                 "BUS"));

    /* Motherboard and ECAM ranges show up later as PNP0C02 and PNP0103 boot configs */
    Pdo->ResArb = Arbiters;

    DPRINT("uACPI-NT: %s arbiters up (memory %u, port %u, bus %u)\n",
           Pdo->Name,
           Arbiters->MemoryReady,
           Arbiters->PortReady,
           Arbiters->BusNumberReady);
    return Arbiters;
}

VOID
NTAPI
UacpiNtResArbiterTeardown(
    _In_ PUACPINT_PDO Pdo)
{
    PUACPINT_RES_ARBITERS Arbiters = Pdo->ResArb;

    if (!Arbiters)
        return;

    if (Arbiters->MemoryReady)
        ArbiterLibDeleteInstance(&Arbiters->Memory);

    if (Arbiters->PortReady)
        ArbiterLibDeleteInstance(&Arbiters->Port);

    if (Arbiters->BusNumberReady)
        ArbiterLibDeleteInstance(&Arbiters->BusNumber);

    ExFreePoolWithTag(Arbiters, UACPINT_POOL_TAG);
    Pdo->ResArb = NULL;
}

static
VOID
NTAPI
UacpiNtResArbReference(
    _In_ PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);
}

static
VOID
NTAPI
UacpiNtResArbDereference(
    _In_ PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);
}

/* A module device hands its producer windows to the children it enumerates */
static
BOOLEAN
NTAPI
UacpiNtResArbOwner(
    _In_ PUACPINT_PDO Pdo)
{
    if (_stricmp(Pdo->Hid, "ACPI0004") != 0)
        return FALSE;

    return UacpiNtNodeHasChild(Pdo->Node, "_CRS");
}

NTSTATUS
NTAPI
UacpiNtQueryResArbiter(
    _In_ PUACPINT_PDO Pdo,
    _In_ PIO_STACK_LOCATION IoStack)
{
    PUACPINT_RES_ARBITERS Arbiters;
    PARBITER_INTERFACE Interface;
    PARBITER_INSTANCE Instance;
    ULONG_PTR ResourceType;
    PCSTR Label;

    /* A PCI root's FDO arbitrates its own bus, only module devices get these */
    if (!UacpiNtResArbEnabled || !UacpiNtResArbOwner(Pdo))
        return STATUS_NOT_SUPPORTED;

    ResourceType = (ULONG_PTR)IoStack->Parameters.QueryInterface.InterfaceSpecificData;
    if (ResourceType != CmResourceTypeMemory &&
        ResourceType != CmResourceTypePort &&
        ResourceType != CmResourceTypeBusNumber)
    {
        return STATUS_NOT_SUPPORTED;
    }

    Interface = (PARBITER_INTERFACE)IoStack->Parameters.QueryInterface.Interface;
    if (IoStack->Parameters.QueryInterface.Size < sizeof(*Interface))
        return STATUS_BUFFER_TOO_SMALL;

    Arbiters = UacpiNtEnsureResArbiters(Pdo);
    if (!Arbiters)
        return STATUS_NOT_SUPPORTED;

    switch (ResourceType)
    {
        case CmResourceTypeMemory:
            Instance = Arbiters->MemoryReady ? &Arbiters->Memory : NULL;
            Label = "memory";
            break;

        case CmResourceTypePort:
            Instance = Arbiters->PortReady ? &Arbiters->Port : NULL;
            Label = "port";
            break;

        default:
            Instance = Arbiters->BusNumberReady ? &Arbiters->BusNumber : NULL;
            Label = "bus number";
            break;
    }

    if (!Instance)
        return STATUS_NOT_SUPPORTED;

    Interface->Size = sizeof(*Interface);
    Interface->Version = 1;
    Interface->Context = Instance;
    Interface->InterfaceReference = UacpiNtResArbReference;
    Interface->InterfaceDereference = UacpiNtResArbDereference;
    Interface->ArbiterHandler = ArbiterLibHandler;
    Interface->Flags = 0;

    DPRINT("uACPI-NT: provided the %s arbiter interface to %s\n", Label, Pdo->Name);
    return STATUS_SUCCESS;
}

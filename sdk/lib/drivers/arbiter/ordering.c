/*
 * PROJECT:     ReactOS Arbitration Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Ordering lists and the registry assignment ordering
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntifs.h>
#include <ndk/rtlfuncs.h>
#include "arbiter.h"

#define NDEBUG
#include <debug.h>

#define ARBITER_ORDERING_GRANULARITY  16
#define ARBITER_ORDERING_LIMIT        1024

#define ARBP_POLICY_ROOT   L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Arbiters\\"
#define ARBP_LONGEST_SUBKEY L"ReservedResources"

/*
 * The PCI-Express enhanced-config (MMCONFIG / ECAM) MMIO window the memory
 * arbiter must never hand out.  Recorded by
 * ArbiterLibAddMmConfigRangeAsBootReserved; Start > End (the initial state)
 */
static ULONGLONG ArbpMmConfigStart = 1;
static ULONGLONG ArbpMmConfigEnd = 0;

/* ORDERING LISTS *************************************************************/

/* Set up an empty ordering array at its starting capacity. */
CODE_SEG("PAGE")
static
NTSTATUS
ArbpCreateOrderingList(
    _Out_ PARBITER_ORDERING_LIST OrderingList)
{
    PAGED_CODE();

    OrderingList->Orderings = ExAllocatePoolWithTag(PagedPool,
                                                    ARBITER_ORDERING_GRANULARITY * sizeof(ARBITER_ORDERING),
                                                    TAG_ARBITER);
    if (OrderingList->Orderings == NULL)
    {
        OrderingList->Count = 0;
        OrderingList->Maximum = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    OrderingList->Count = 0;
    OrderingList->Maximum = ARBITER_ORDERING_GRANULARITY;
    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
VOID
NTAPI
ArbiterLibFreeOrderingList(
    _Inout_ PARBITER_ORDERING_LIST OrderingList)
{
    PAGED_CODE();

    if (OrderingList->Orderings != NULL)
        ExFreePoolWithTag(OrderingList->Orderings, TAG_ARBITER);

    OrderingList->Orderings = NULL;
    OrderingList->Count = 0;
    OrderingList->Maximum = 0;
}

/* Append one [Start, End] window (both bounds inclusive) at the tail
 * (= lowest preference so far). */
CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbiterLibAddOrdering(
    _Inout_ PARBITER_ORDERING_LIST OrderingList,
    _In_ UINT64 Start,
    _In_ UINT64 End)
{
    PAGED_CODE();

    if (End < Start)
        return STATUS_INVALID_PARAMETER;

    /* Double the array capacity when it fills up, up to the sanity cap. */
    if (OrderingList->Count == OrderingList->Maximum)
    {
        ULONG NewMaximum;
        PARBITER_ORDERING NewArray;

        NewMaximum = OrderingList->Maximum ? (ULONG)OrderingList->Maximum * 2
                                           : ARBITER_ORDERING_GRANULARITY;
        if (NewMaximum > ARBITER_ORDERING_LIMIT)
            return STATUS_INSUFFICIENT_RESOURCES;

        NewArray = ExAllocatePoolWithTag(PagedPool,
                                         NewMaximum * sizeof(ARBITER_ORDERING),
                                         TAG_ARBITER);
        if (NewArray == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        if (OrderingList->Orderings != NULL)
        {
            RtlCopyMemory(NewArray, OrderingList->Orderings,
                          OrderingList->Count * sizeof(ARBITER_ORDERING));
            ExFreePoolWithTag(OrderingList->Orderings, TAG_ARBITER);
        }

        OrderingList->Maximum = (UINT16)NewMaximum;
        OrderingList->Orderings = NewArray;
    }

    OrderingList->Orderings[OrderingList->Count].Start = Start;
    OrderingList->Orderings[OrderingList->Count].End = End;
    OrderingList->Count++;
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Removes [Start, End] from every window of an ordering list,
 * splitting windows that straddle it.
 *
 * @param[in,out] OrderingList
 * The ordering list to prune. The pruned set is built in a
 * temporary list and swapped in on success; on failure the
 * original list is left untouched.
 *
 * @param[in] Start
 * The inclusive start of the range to remove.
 *
 * @param[in] End
 * The inclusive end of the range to remove. Must not be below
 * Start.
 *
 * @return
 * Returns STATUS_SUCCESS, STATUS_INVALID_PARAMETER for a
 * backwards range, or STATUS_INSUFFICIENT_RESOURCES.
 */
CODE_SEG("PAGE")
static
NTSTATUS
ArbpExcludeOrderingRange(
    _Inout_ PARBITER_ORDERING_LIST OrderingList,
    _In_ UINT64 Start,
    _In_ UINT64 End)
{
    ARBITER_ORDERING_LIST Pruned;
    NTSTATUS Status;
    UINT16 Index;

    PAGED_CODE();

    if (End < Start)
        return STATUS_INVALID_PARAMETER;

    Status = ArbpCreateOrderingList(&Pruned);
    if (!NT_SUCCESS(Status))
        return Status;

    for (Index = 0; Index < OrderingList->Count; ++Index)
    {
        UINT64 CurrentStart = OrderingList->Orderings[Index].Start;
        UINT64 CurrentEnd = OrderingList->Orderings[Index].End;

        Status = STATUS_SUCCESS;

        if (CurrentEnd < Start || CurrentStart > End)
        {
            /* Wholly outside the hole: survives unchanged. */
            Status = ArbiterLibAddOrdering(&Pruned, CurrentStart, CurrentEnd);
        }
        else
        {
            if (CurrentStart < Start)  /* left fragment survives */
                Status = ArbiterLibAddOrdering(&Pruned, CurrentStart, Start - 1);

            if (NT_SUCCESS(Status) && CurrentEnd > End)  /* right fragment survives */
                Status = ArbiterLibAddOrdering(&Pruned, End + 1, CurrentEnd);
        }

        if (!NT_SUCCESS(Status))
        {
            ArbiterLibFreeOrderingList(&Pruned);
            return Status;
        }
    }

    ArbiterLibFreeOrderingList(OrderingList);
    *OrderingList = Pruned;
    return STATUS_SUCCESS;
}

/* REGISTRY-DESCRIBED RANGES **************************************************/

/*
 * An arbiter refines its allocation policy from HKLM\...\Control\Arbiters.
 * Each value there is a REG_RESOURCE_REQUIREMENTS_LIST whose descriptors give
 * [MinimumAddress, MaximumAddress] ranges, filtered by the arbiter's resource
 * This can get written to by chipset drivers.
 */

typedef VOID
(NTAPI *PARB_RANGE_CALLBACK)(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_opt_ PVOID Context,
    _In_ ULONGLONG Start,
    _In_ ULONGLONG End);

/* Open one subkey of the Arbiters policy key by composing its full path. */
CODE_SEG("PAGE")
static
NTSTATUS
ArbpOpenPolicySubkey(
    _In_ PCWSTR SubkeyName,
    _Out_ PHANDLE KeyHandle)
{
    WCHAR PathBuffer[sizeof(ARBP_POLICY_ROOT ARBP_LONGEST_SUBKEY) / sizeof(WCHAR)];
    UNICODE_STRING KeyPath;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;

    PAGED_CODE();

    RtlInitEmptyUnicodeString(&KeyPath, PathBuffer, sizeof(PathBuffer));
    Status = RtlAppendUnicodeToString(&KeyPath, ARBP_POLICY_ROOT);
    if (NT_SUCCESS(Status))
        Status = RtlAppendUnicodeToString(&KeyPath, SubkeyName);
    if (!NT_SUCCESS(Status))
        return Status;

    InitializeObjectAttributes(&ObjectAttributes, &KeyPath,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
    return ZwOpenKey(KeyHandle, KEY_READ, &ObjectAttributes);
}

/* Read one value into a freshly-allocated KEY_VALUE_FULL_INFORMATION. */
CODE_SEG("PAGE")
static
NTSTATUS
ArbpReadPolicyValue(
    _In_ HANDLE KeyHandle,
    _In_ PCWSTR ValueName,
    _Out_ PKEY_VALUE_FULL_INFORMATION *Value)
{
    UNICODE_STRING NameString;
    PKEY_VALUE_FULL_INFORMATION Buffer;
    ULONG Size = 0;
    ULONG ResultLength = 0;
    NTSTATUS Status;

    PAGED_CODE();

    RtlInitUnicodeString(&NameString, ValueName);

    /* Size probe: only a buffer-size result means the value is readable. */
    Status = ZwQueryValueKey(KeyHandle, &NameString, KeyValueFullInformationAlign64,
                             NULL, 0, &Size);
    if (Status != STATUS_BUFFER_TOO_SMALL && Status != STATUS_BUFFER_OVERFLOW)
        return NT_SUCCESS(Status) ? STATUS_UNSUCCESSFUL : Status;

    Buffer = ExAllocatePoolWithTag(PagedPool, Size, TAG_ARBITER);
    if (Buffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = ZwQueryValueKey(KeyHandle, &NameString, KeyValueFullInformationAlign64,
                             Buffer, Size, &ResultLength);

    if (NT_SUCCESS(Status) &&
        ((ULONGLONG)Buffer->DataOffset + Buffer->DataLength > ResultLength))
    {
        Status = STATUS_UNSUCCESSFUL;
    }

    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(Buffer, TAG_ARBITER);
        return Status;
    }

    *Value = Buffer;
    return STATUS_SUCCESS;
}

/* Does a policy descriptor apply to this arbiter's resource type? */
CODE_SEG("PAGE")
static
BOOLEAN
ArbpDescriptorMatchesArbiter(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ UCHAR DescriptorType)
{
    PAGED_CODE();

    if (DescriptorType == (UCHAR)Arbiter->ResourceType)
        return TRUE;

    /* Large-memory descriptors also satisfy the plain memory arbiter. */
    return (DescriptorType == CmResourceTypeMemoryLarge &&
            Arbiter->ResourceType == CmResourceTypeMemory);
}

/*
 * Walk the policy ranges stored under HKLM\...\Control\Arbiters\<Subkey>,
 * This is used by machine-specific policy tables.
 */
CODE_SEG("PAGE")
static
NTSTATUS
ArbpForEachRegistryRange(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PCWSTR Subkey,
    _In_ PCWSTR ValueName,
    _In_opt_ PARB_TRANSLATE_ORDERING Translate,
    _In_ PARB_RANGE_CALLBACK Callback,
    _In_opt_ PVOID Context)
{
    HANDLE KeyHandle;
    PKEY_VALUE_FULL_INFORMATION Value = NULL;
    NTSTATUS Status;

    PAGED_CODE();

    if (!NT_SUCCESS(ArbpOpenPolicySubkey(Subkey, &KeyHandle)))
        return STATUS_SUCCESS;

    Status = ArbpReadPolicyValue(KeyHandle, ValueName, &Value);

    /* Follow a REG_SZ indirection to the named sibling value. */
    if (NT_SUCCESS(Status) && Value->Type == REG_SZ)
    {
        PWSTR TargetName = (PWSTR)((PUCHAR)Value + Value->DataOffset);
        ULONG TargetChars = Value->DataLength / sizeof(WCHAR);
        PKEY_VALUE_FULL_INFORMATION Target = NULL;

        if (TargetChars != 0 &&
            TargetName[TargetChars - 1] == UNICODE_NULL &&
            NT_SUCCESS(ArbpReadPolicyValue(KeyHandle, TargetName, &Target)))
        {
            ExFreePoolWithTag(Value, TAG_ARBITER);
            Value = Target;
        }
    }

    if (NT_SUCCESS(Status) &&
        Value->Type == REG_RESOURCE_REQUIREMENTS_LIST &&
        Value->DataLength >= sizeof(IO_RESOURCE_REQUIREMENTS_LIST))
    {
        PIO_RESOURCE_REQUIREMENTS_LIST Requirements;
        PIO_RESOURCE_LIST Alternative;
        PUCHAR DataEnd;
        ULONG Index;

        Requirements = (PIO_RESOURCE_REQUIREMENTS_LIST)((PUCHAR)Value + Value->DataOffset);
        DataEnd = (PUCHAR)Requirements + Value->DataLength;
        Alternative = &Requirements->List[0];

        for (Index = 0; Index < Alternative->Count; ++Index)
        {
            PIO_RESOURCE_DESCRIPTOR Descriptor = &Alternative->Descriptors[Index];
            IO_RESOURCE_DESCRIPTOR Window;
            UINT64 Minimum, Maximum, Length, Alignment;

            /* Never trust the stored Count past the actual value data. */
            if ((PUCHAR)(Descriptor + 1) > DataEnd)
                break;

            /*
             * Policy windows are written in bus-relative terms, so an arbiter
             * that allocates in a different space translates each one first.
             * A window it cannot translate comes back CmResourceTypeNull and
             * is dropped by the type test below.
             */
            if (Translate != NULL)
            {
                Status = Translate(&Window, Descriptor);
                if (!NT_SUCCESS(Status))
                    break;
            }
            else
            {
                Window = *Descriptor;
            }

            if (!ArbpDescriptorMatchesArbiter(Arbiter, Window.Type))
                continue;

            /*
             * Decode through the arbiter's own callback: the bounds live in a
             * different union arm for every resource type, and only it knows
             * which one - u.Interrupt for a vector, u.Dma for a channel,
             * u.BusNumber for a bus range.
             */
            Status = Arbiter->UnpackRequirement(&Window, &Minimum, &Maximum,
                                                &Length, &Alignment);
            if (!NT_SUCCESS(Status))
                break;

            Callback(Arbiter, Context, Minimum, Maximum);
        }
    }

    if (Value != NULL)
        ExFreePoolWithTag(Value, TAG_ARBITER);
    ZwClose(KeyHandle);
    return STATUS_SUCCESS;
}

/* Prepend a registry-described preferred range ahead of the full-range default. */
CODE_SEG("PAGE")
static
VOID
NTAPI
ArbpAddOrderingCallback(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_opt_ PVOID Context,
    _In_ ULONGLONG Start,
    _In_ ULONGLONG End)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Context);
    ArbiterLibAddOrdering(&Arbiter->OrderingList, Start, End);
}

/**
 * @brief
 * Range callback moving one reserved window into last-resort
 * territory: recorded in the arbiter's ReservedList and pruned
 * out of every ordering window, including the full-range
 * fallback.
 *
 * @param[in] Arbiter
 * The arbiter instance whose lists are adjusted.
 *
 * @param[in] Context
 * Unused.
 *
 * @param[in] Start
 * The inclusive start of the reserved window.
 *
 * @param[in] End
 * The inclusive end of the reserved window.
 *
 * @remarks
 * Once pruned here, the window can only ever be offered by the
 * engine's (PREFERRED_)RESERVED pass - "usable only when nothing
 * else fits". This is what keeps allocations off ranges like
 * COM1/COM2 and the VGA windows that the PCStandard policy table
 * reserves, even though no enumerated device owns them yet.
 */
CODE_SEG("PAGE")
static
VOID
NTAPI
ArbpAddReservedCallback(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_opt_ PVOID Context,
    _In_ ULONGLONG Start,
    _In_ ULONGLONG End)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Context);

    if (NT_SUCCESS(ArbiterLibAddOrdering(&Arbiter->ReservedList, Start, End)))
        ArbpExcludeOrderingRange(&Arbiter->OrderingList, Start, End);
}

/**
 * @brief
 * Range callback carving one window out of a range list as an
 * unowned blocking range, so it is never handed to a device.
 *
 * @param[in] Arbiter
 * Unused.
 *
 * @param[in] Context
 * The RTL_RANGE_LIST to add the blocking range to.
 *
 * @param[in] Start
 * The inclusive start of the window.
 *
 * @param[in] End
 * The inclusive end of the window.
 */
CODE_SEG("PAGE")
static
VOID
NTAPI
ArbpAddRangeCallback(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_opt_ PVOID Context,
    _In_ ULONGLONG Start,
    _In_ ULONGLONG End)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Arbiter);
    RtlAddRange((PRTL_RANGE_LIST)Context, Start, End, 0,
                RTL_RANGE_LIST_ADD_IF_CONFLICT, NULL, NULL);
}

/**
 * @brief
 * Range callback recording the MMCONFIG (ECAM) window and
 * reserving it in a range list as a boot allocation.
 *
 * @param[in] Arbiter
 * Unused.
 *
 * @param[in] Context
 * The RTL_RANGE_LIST to reserve the window in.
 *
 * @param[in] Start
 * The inclusive start of the ECAM window.
 *
 * @param[in] End
 * The inclusive end of the ECAM window.
 */
CODE_SEG("PAGE")
static
VOID
NTAPI
ArbpMmConfigCallback(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_opt_ PVOID Context,
    _In_ ULONGLONG Start,
    _In_ ULONGLONG End)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Arbiter);

    ArbpMmConfigStart = Start;
    ArbpMmConfigEnd = End;
    RtlAddRange((PRTL_RANGE_LIST)Context, Start, End, ARBITER_RANGE_BOOT_ALLOCATED,
                RTL_RANGE_LIST_ADD_IF_CONFLICT, NULL, NULL);
}

/**
 * @brief
 * Builds the arbiter's allocation ordering from the registry
 * policy, most-preferred window first.
 *
 * @param[in,out] Arbiter
 * The arbiter instance whose OrderingList and ReservedList are
 * (re)created. Callers may rebuild an existing ordering against a
 * different policy table (see pcix ario_ApplyBrokenVideoHack).
 *
 * @param[in] AllocationOrderName
 * The AllocationOrder value naming this arbiter's preferred
 * windows. The engine walks the resulting list in index order, so
 * index 0 is the most-preferred window.
 *
 * @param[in] ReservedResourcesName
 * The ReservedResources value naming this arbiter's last-resort
 * windows: each is recorded in the ReservedList for the engine's
 * final pass and pruned out of every ordering window, so it is
 * only ever offered once all orderings fail.
 *
 * @param[in] TranslateOrderingFunction
 * Optional per-arbiter descriptor translation. Currently unused.
 *
 * @return
 * Returns STATUS_SUCCESS if the ordering was built - absent
 * registry policy is not a failure; the ordering then holds only
 * the full-range fallback, so the arbiter can always search the
 * whole space.
 */
CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbiterLibDefaultAssignmentOrdering(
    _Inout_ PARBITER_INSTANCE Arbiter,
    _In_ PCWSTR AllocationOrderName,
    _In_ PCWSTR ReservedResourcesName,
    _In_opt_ PARB_TRANSLATE_ORDERING TranslateOrderingFunction)
{
    NTSTATUS Status;

    PAGED_CODE();

    ArbiterLibFreeOrderingList(&Arbiter->OrderingList);
    ArbiterLibFreeOrderingList(&Arbiter->ReservedList);

    Status = ArbpCreateOrderingList(&Arbiter->OrderingList);
    if (!NT_SUCCESS(Status))
        return Status;
    Status = ArbpCreateOrderingList(&Arbiter->ReservedList);
    if (!NT_SUCCESS(Status))
    {
        ArbiterLibFreeOrderingList(&Arbiter->OrderingList);
        return Status;
    }

    ArbpForEachRegistryRange(Arbiter, L"AllocationOrder", AllocationOrderName,
                             TranslateOrderingFunction, ArbpAddOrderingCallback, NULL);

    Status = ArbiterLibAddOrdering(&Arbiter->OrderingList, 0, 0xFFFFFFFFFFFFFFFFULL);
    if (!NT_SUCCESS(Status))
        return Status;

    /*
     * The reserved windows come last, after the fallback is in place, so the
     * pruning punches them out of the fallback too - otherwise the fallback
     * pass would hand them out like any other range.
     */
    return ArbpForEachRegistryRange(Arbiter, L"ReservedResources", ReservedResourcesName,
                                    TranslateOrderingFunction, ArbpAddReservedCallback,
                                    NULL);
}

/**
 * @brief
 * Carves the firmware-reported inaccessible ranges
 * (Arbiters\InaccessibleRange\<OrderingName>) out of a range list
 * as unowned blocking ranges, so they are never handed to a
 * device.
 *
 * @param[in] Arbiter
 * The arbiter instance the ranges are filtered for.
 *
 * @param[in] OrderingName
 * The InaccessibleRange value to read (usually the arbiter's
 * ordering name; on Windows a REG_SZ redirect points it at the
 * kernel-written PhysicalAddress value).
 *
 * @param[in,out] RangeList
 * The range list to carve the windows out of.
 *
 * @return
 * Returns STATUS_SUCCESS, including when nothing is recorded -
 * an absent value reserves nothing.
 */
CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbiterLibAddInaccessibleAllocationRange(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PCWSTR OrderingName,
    _Inout_ PRTL_RANGE_LIST RangeList)
{
    PAGED_CODE();
    return ArbpForEachRegistryRange(Arbiter, L"InaccessibleRange", OrderingName,
                                    NULL, ArbpAddRangeCallback, RangeList);
}

/**
 * @brief
 * Reserves the PCI-Express enhanced-config (MMCONFIG / ECAM) MMIO
 * window (Arbiters\ReservedResources\MmConfigRange) in a range
 * list, and records it for ArbiterLibIsConflictWithMmConfigRange.
 *
 * @param[in] Arbiter
 * The arbiter instance the window is filtered for.
 *
 * @param[in,out] RangeList
 * The range list to reserve the window in.
 *
 * @return
 * Returns STATUS_SUCCESS, including on a legacy pre-PCIe machine
 * where the value is simply absent and nothing is reserved.
 */
CODE_SEG("PAGE")
NTSTATUS
NTAPI
ArbiterLibAddMmConfigRangeAsBootReserved(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PRTL_RANGE_LIST RangeList)
{
    PAGED_CODE();
    return ArbpForEachRegistryRange(Arbiter, L"ReservedResources", L"MmConfigRange",
                                    NULL, ArbpMmConfigCallback, RangeList);
}

/**
 * @brief
 * Determines whether a range overlaps the recorded MMCONFIG
 * window.
 *
 * @param[in] Start
 * The inclusive start of the range to test.
 *
 * @param[in] End
 * The inclusive end of the range to test.
 *
 * @return
 * Returns TRUE if [Start, End] overlaps the recorded window,
 * FALSE otherwise or when no window was ever recorded.
 */
BOOLEAN
NTAPI
ArbiterLibIsConflictWithMmConfigRange(
    _In_ ULONGLONG Start,
    _In_ ULONGLONG End)
{
    if (ArbpMmConfigStart > ArbpMmConfigEnd)
        return FALSE;  /* no MMCONFIG window recorded */

    return (BOOLEAN)(Start <= ArbpMmConfigEnd && ArbpMmConfigStart <= End);
}

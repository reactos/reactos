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

            /* Never trust the stored Count past the actual value data. */
            if ((PUCHAR)(Descriptor + 1) > DataEnd)
                break;

            if (ArbpDescriptorMatchesArbiter(Arbiter, Descriptor->Type))
            {
                Callback(Arbiter, Context,
                         (ULONGLONG)Descriptor->u.Generic.MinimumAddress.QuadPart,
                         (ULONGLONG)Descriptor->u.Generic.MaximumAddress.QuadPart);
            }
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

/*
 * Build the arbiter's allocation ordering.  The registry-described preferred
 * ranges (AllocationOrder\<AllocationOrderName>) the engine walks the
 * list in index order, so index 0 is the most-preferred window. If one isnt
 * found its followed by a full-range fallback so the arbiter can always search the whole space
 *
 * Both ordering lists are (re)created here: callers may rebuild an existing
 * ordering against a different policy table (see pcix ario_ApplyBrokenVideoHack).
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

    UNREFERENCED_PARAMETER(ReservedResourcesName);
    UNREFERENCED_PARAMETER(TranslateOrderingFunction);

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
                             ArbpAddOrderingCallback, NULL);

    return ArbiterLibAddOrdering(&Arbiter->OrderingList, 0, 0xFFFFFFFFFFFFFFFFULL);
}

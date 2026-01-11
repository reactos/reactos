/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power Manager miscellaneous utility routines
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

UNICODE_STRING PopPowerRegPath = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Power");
UNICODE_STRING RegAcPolicy = RTL_CONSTANT_STRING(L"AcPolicy");
UNICODE_STRING RegDcPolicy = RTL_CONSTANT_STRING(L"DcPolicy");

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
NTAPI
PopCreatePowerPolicyDatabase(VOID)
{
    NTSTATUS Status;
    HANDLE KeyHandle;
    OBJECT_ATTRIBUTES PowerKeyAttrs;

    /* Initialize the object attributes for the power key database */
    InitializeObjectAttributes(&PowerKeyAttrs,
                               &PopPowerRegPath,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    /* Create the power registry database */
    Status = ZwCreateKey(&KeyHandle,
                         KEY_READ | KEY_WRITE,
                         &PowerKeyAttrs,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         NULL);

    /*
     * We cannot simply bait an eye on failing to set up the power registry
     * database. The power policy settings would never be saved in this case.
     */
    ASSERT(NT_SUCCESS(Status));
    ZwClose(KeyHandle);
}

NTSTATUS
NTAPI
PopReadPowerSettings(
    _In_ PUNICODE_STRING PowerValue,
    _In_ ULONG ValueType,
    _Out_ PKEY_VALUE_PARTIAL_INFORMATION *ReturnedData)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES PowerKeyAttrs;
    ULONG ReturnedLength;
    PKEY_VALUE_PARTIAL_INFORMATION BufferKey = NULL;
    HANDLE PowerKey = NULL;

    PAGED_CODE();

    /* Initialize the object attributes for the power key database */
    InitializeObjectAttributes(&PowerKeyAttrs,
                               &PopPowerRegPath,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    /* Open the power settings key */
    Status = ZwOpenKey(&PowerKey,
                       KEY_QUERY_VALUE,
                       &PowerKeyAttrs);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open %wZ (Status 0x%lx)\n", PopPowerRegPath, Status);
        return Status;
    }

    /*
     * Let the Configuration Manager figure out how much space is needed
     * to allocate a buffer for our needs.
     */
    Status = ZwQueryValueKey(PowerKey,
                             PowerValue,
                             KeyValuePartialInformation,
                             NULL,
                             0,
                             &ReturnedLength);
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        /* Got entirely something else, this is super bad */
        DPRINT1("Expected STATUS_BUFFER_TOO_SMALL but got 0x%lx. Punt...\n", Status);
        ZwClose(PowerKey);
        return Status;
    }

    /* Allocate chunks of memory based on the space length we got for the buffer */
    BufferKey = PopAllocatePool(ReturnedLength, TRUE, TAG_PO_REGISTRY);
    if (BufferKey == NULL)
    {
        /* Not enough memory, bail out */
        DPRINT1("Failed to allocate memory for the key buffer!\n");
        ZwClose(PowerKey);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Query the actual value now */
    Status = ZwQueryValueKey(PowerKey,
                             PowerValue,
                             KeyValuePartialInformation,
                             BufferKey,
                             ReturnedLength,
                             &ReturnedLength);

    /* We no longer need the power key */
    ZwClose(PowerKey);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to query data from %wZ value (Status 0x%lx)\n", PowerValue, Status);
        PopFreePool(BufferKey, TAG_PO_REGISTRY);
        return Status;
    }

    /* Is this the value type the caller requested? */
    if (BufferKey->Type != ValueType)
    {
        DPRINT1("The caller requested an invalid value (requested value %lu, got %lu)\n", ValueType, BufferKey->Type);
        PopFreePool(BufferKey, TAG_PO_REGISTRY);
        return STATUS_INVALID_PARAMETER;
    }

    /* Return the queried information to the caller */
    *ReturnedData = BufferKey;
    return Status;
}

PVOID
NTAPI
PopAllocatePool(
    _In_ SIZE_T PoolSize,
    _In_ BOOLEAN Paged,
    _In_ ULONG Tag)
{
    PVOID Buffer;
    BOOLEAN UseDefaultTag = FALSE;

    /* Avoid zero pool allocations */
    ASSERT(PoolSize != 0);

    /* Use the default tag if none was provided */
    if (Tag == 0)
    {
        UseDefaultTag = TRUE;
    }

    Buffer = ExAllocatePoolZero(Paged ? PagedPool : NonPagedPool,
                                PoolSize,
                                UseDefaultTag ? TAG_PO : Tag);
    if (Buffer == NULL)
    {
        return NULL;
    }

    return Buffer;
}

VOID
NTAPI
PopFreePool(
    _In_ _Post_invalid_ PVOID PoolBuffer,
    _In_ ULONG Tag)
{
    ASSERT(PoolBuffer != NULL);
    ExFreePoolWithTag(PoolBuffer, Tag);
}

VOID
NTAPI
PoRundownDeviceObject(
    _In_ PDEVICE_OBJECT DeviceObject)
{
    /* This device object is being freed, does it still process power IRPs? */
    if (PopHasDoOutstandingIrp(DeviceObject))
    {
        /*
         * This device still processes a power IRP and has not even
         * finished on doing so. Any power IRPs that this device owns
         * are going to be orphaned. This is bad because we will not be able
         * to pass such IRPs to the responsible device driver, thereby ending
         * up with "phantom" power requests. Kill the system...
         */
        KeBugCheckEx(DRIVER_POWER_STATE_FAILURE,
                     0x1,
                     (ULONG_PTR)DeviceObject,
                     0,
                     0);
    }

    /* Cancel any idle detection for this device */
    PoRegisterDeviceForIdleDetection(DeviceObject, 0, 0, PowerDeviceUnspecified);

    /* Remove the power volumes of this device */
    /* FIXME: To be enabled once Mm supports pageable sections */
#if 0
    MmLockPageableSectionByHandle(ExPageLockHandle);
#endif
    PopRemoveVolumeDevice(DeviceObject);
#if 0
    MmUnlockPageableImageSection(ExPageLockHandle);
#endif
}

ULONG
NTAPI
PopQueryActiveProcessors(VOID)
{
    KAFFINITY ProcessorAffinity;
    ULONG ProcessorsCount;

    /* Query the active processors and count them based on the set mask bits */
    ProcessorsCount = 0;
    ProcessorAffinity = KeQueryActiveProcessors();
    while (ProcessorAffinity)
    {
        /* This bit is set so we have a processor, count it */
        if (ProcessorAffinity & 1)
        {
            ProcessorsCount++;
        }

        /* Go to the next bit */
        ProcessorAffinity >>= 1;
    }

    return ProcessorsCount;
}

BOOLEAN
NTAPI
PopIsEqualGuid(
    _In_ CONST GUID *FirstGuid,
    _In_ CONST GUID *SecondGuid)
{
    return RtlCompareMemory(FirstGuid, SecondGuid, sizeof(GUID)) == sizeof(GUID);
}

/* EOF */

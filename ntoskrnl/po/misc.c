/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
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

/**
 * @brief
 * Initializes the power policy database of Power Manager at
 * system boot phase.
 */
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

/**
 * @brief
 * Reads a power setting from the power policy database from
 * the registery and returns the power configuration data
 * of the said setting.
 *
 * @param[in] PowerValue
 * A pointer to a Unicode string that points to the power setting
 * value name from the registry power policy database.
 *
 * @param[in] ValueType
 * The value type of the power setting.
 *
 * @param[out] ReturnedData
 * A pointer to the registry key information of the power setting
 * data, returned to the caller.
 *
 * @return
 * Returns STATUS_SUCCESS if the power setting has been read successfully
 * from the power policy database. STATUS_INSUFFICIENT_RESOURCES is
 * returned if memory couldn't be allocated for the key data buffer.
 * STATUS_INVALID_PARAMETER is returned if the value type doesn't match
 * with that of the original power setting from the registry. A failure
 * NTSTATUS code is returned otherwise.
 */
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

/**
 * @brief
 * Allocates a block of memory pool for Power Manage related
 * stuff like critical power structures and whatnot.
 *
 * @param[in] PoolSize
 * The pool size that must be allocated for the buffer.
 *
 * @param[in] Paged
 * If set to TRUE, the memory pool allocated will reside in
 * the paged pool. Otherwise it'll reside in non-paged pool
 * if set to FALSE.
 *
 * @param[in] Tag
 * The tag that identifies the memory block that's been
 * allocated. This is for debugging purposes.
 *
 * @return
 * Returns the allocated memory block for use by the Power
 * Manager. NULL is returned otherwise due to failure.
 */
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

/**
 * @brief
 * Frees a memory block previously allocated by PopAllocatePool.
 *
 * @param[in] PoolBuffer
 * A pointer to arbitrary data that points to the allocated
 * memory block to be freed.
 *
 * @param[in] Tag
 * The tag that identifies the memory block that's been
 * allocated. The tag MUST match with that of the one used
 * to allocate the said block.
 */
VOID
NTAPI
PopFreePool(
    _In_ _Post_invalid_ PVOID PoolBuffer,
    _In_ ULONG Tag)
{
    ASSERT(PoolBuffer != NULL);
    ExFreePoolWithTag(PoolBuffer, Tag);
}

/**
 * @brief
 * Cleans up any power related stuff of a device that's soon
 * to be freed.
 *
 * @param[in] DeviceObject
 * A pointer to a device object that's soon to be freed.
 */
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

/**
 * @brief
 * Computes the number of active logical processors
 * of the system.
 *
 * @return
 * Returns the number of active logical processors
 * to the caller.
 */
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

/**
 * @brief
 * Compares two GUIDs for equality.
 *
 * @param[in] FirstGuid
 * A pointer to the first GUID.
 *
 * @param[in] SecondGuid
 * A pointer to the second GUID.
 *
 * @return
 * Returns TRUE if two GUIDs are the same, FALSE otherwise.
 */
BOOLEAN
NTAPI
PopIsEqualGuid(
    _In_ CONST GUID *FirstGuid,
    _In_ CONST GUID *SecondGuid)
{
    return RtlCompareMemory(FirstGuid, SecondGuid, sizeof(GUID)) == sizeof(GUID);
}

/**
 * @brief
 * Creates a Power Manager worker thread to serve power
 * related tasks.
 *
 * @param[in] WorkerRoutine
 * A pointer to a worker routine that is to be
 * used for serving it with a worker thread.
 *
 * @param[in] Context
 * A pointer to a parameter context that is passed
 * to the worker routine. This parameter is optional.
 *
 * @param[in] Priority
 * A thread priority that is used by the Process Manager
 * to set the worker thread at a specific priority so that
 * the scheduler can process up the thread based on how
 * critical a worker task is.
 *
 * @return
 * Returns STATUS_SUCCESS if the power worker thread has
 * been created successfully. A failure NTSTATUS code is
 * returned otherwise.
 */
NTSTATUS
NTAPI
PopCreateWorkerThread(
    _In_ PKSTART_ROUTINE WorkerRoutine,
    _In_opt_ PVOID Context,
    _In_ KPRIORITY Priority)
{
    NTSTATUS Status;
    HANDLE ThreadHandle;
    PETHREAD Thread;
    OBJECT_ATTRIBUTES ObjectAttributes;

    PAGED_CODE();

    /* Setup the object attributes for the worker thread */
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    /* Give birth to this worker thread */
    Status = PsCreateSystemThread(&ThreadHandle,
                                  THREAD_ALL_ACCESS,
                                  &ObjectAttributes,
                                  NULL,
                                  NULL,
                                  WorkerRoutine,
                                  Context);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create a power manager worker thread (Status 0x%lx)", Status);
        return Status;
    }

    /*
     * The worker thread is alive and sound. We have to re-adjust the thread
     * base priority on behalf of the request of the caller who sets it up
     * depending on the priority of the task that must be accomplished.
     */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_SET_INFORMATION,
                                       PsThreadType,
                                       KernelMode,
                                       (PVOID *)&Thread,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to set up thread priority for the power manager worker thread (Status 0x%lx)", Status);
        ObCloseHandle(ThreadHandle, KernelMode);
        return Status;
    }

    /* Re-adjust the thread priority for this worker and cleanup our stuff */
    KeSetBasePriorityThread(&Thread->Tcb, Priority);
    ObDereferenceObject(Thread);
    ObCloseHandle(ThreadHandle, KernelMode);
    return Status;
}

/* EOF */

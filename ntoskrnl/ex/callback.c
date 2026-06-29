/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ex/callback.c
 * PURPOSE:         Executive callbacks
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* TYPES *********************************************************************/

/* Mapping for Callback Object */
GENERIC_MAPPING ExpCallbackMapping =
{
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE | CALLBACK_MODIFY_STATE,
    STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
    CALLBACK_ALL_ACCESS
};

/* Kernel Default Callbacks */
PCALLBACK_OBJECT SetSystemTimeCallback;
PCALLBACK_OBJECT SetSystemStateCallback;
PCALLBACK_OBJECT PowerStateCallback;
SYSTEM_CALLBACKS ExpInitializeCallback[] =
{
   {&SetSystemTimeCallback, L"\\Callback\\SetSystemTime"},
   {&SetSystemStateCallback, L"\\Callback\\SetSystemState"},
   {&PowerStateCallback, L"\\Callback\\PowerState"},
   {NULL, NULL}
};

POBJECT_TYPE ExCallbackObjectType;
KEVENT ExpCallbackEvent;
EX_PUSH_LOCK ExpCallBackFlush;

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
ExInitializeCallBack(IN OUT PEX_CALLBACK Callback)
{
    /* Initialize the fast reference */
    ExInitializeFastReference(&Callback->RoutineBlock, NULL);
}

PEX_CALLBACK_ROUTINE_BLOCK
NTAPI
ExAllocateCallBack(IN PEX_CALLBACK_FUNCTION Function,
                   IN PVOID Context)
{
    PEX_CALLBACK_ROUTINE_BLOCK CallbackBlock;

    /* Allocate a callback */
    CallbackBlock = ExAllocatePoolWithTag(PagedPool,
                                          sizeof(EX_CALLBACK_ROUTINE_BLOCK),
                                          TAG_CALLBACK_ROUTINE_BLOCK);
    if (CallbackBlock)
    {
        /* Initialize it */
        CallbackBlock->Function = Function;
        CallbackBlock->Context = Context;
        ExInitializeRundownProtection(&CallbackBlock->RundownProtect);
    }

    /* Return it */
    return CallbackBlock;
}

VOID
NTAPI
ExFreeCallBack(IN PEX_CALLBACK_ROUTINE_BLOCK CallbackBlock)
{
    /* Just free it from memory */
    ExFreePoolWithTag(CallbackBlock, TAG_CALLBACK_ROUTINE_BLOCK);
}

VOID
NTAPI
ExWaitForCallBacks(IN PEX_CALLBACK_ROUTINE_BLOCK CallbackBlock)
{
    /* Wait on the rundown */
    ExWaitForRundownProtectionRelease(&CallbackBlock->RundownProtect);
}

PEX_CALLBACK_FUNCTION
NTAPI
ExGetCallBackBlockRoutine(IN PEX_CALLBACK_ROUTINE_BLOCK CallbackBlock)
{
    /* Return the function */
    return CallbackBlock->Function;
}

PVOID
NTAPI
ExGetCallBackBlockContext(IN PEX_CALLBACK_ROUTINE_BLOCK CallbackBlock)
{
    /* Return the context */
    return CallbackBlock->Context;
}

VOID
NTAPI
ExDereferenceCallBackBlock(IN OUT PEX_CALLBACK CallBack,
                           IN PEX_CALLBACK_ROUTINE_BLOCK CallbackBlock)
{
    /* Release a fast reference */
    if (!ExReleaseFastReference(&CallBack->RoutineBlock, CallbackBlock))
    {
        /* Take slow path */
        ExReleaseRundownProtection(&CallbackBlock->RundownProtect);
    }
}

PEX_CALLBACK_ROUTINE_BLOCK
NTAPI
ExReferenceCallBackBlock(IN OUT PEX_CALLBACK CallBack)
{
    EX_FAST_REF OldValue;
    ULONG_PTR Count;
    PEX_CALLBACK_ROUTINE_BLOCK CallbackBlock;

    /* Acquire a reference */
    OldValue = ExAcquireFastReference(&CallBack->RoutineBlock);
    Count = ExGetCountFastReference(OldValue);

    /* Fail if there isn't any object */
    if (!ExGetObjectFastReference(OldValue)) return NULL;

    /* Check if we don't have a reference */
    if (!Count)
    {
        /* FIXME: Race */
        DPRINT1("Unhandled callback race condition\n");
        ASSERT(FALSE);
        return NULL;
    }

    /* Get the callback block */
    CallbackBlock = ExGetObjectFastReference(OldValue);

    /* Check if this is the last reference */
    if (Count == 1)
    {
        /* Acquire rundown protection */
        if (ExfAcquireRundownProtectionEx(&CallbackBlock->RundownProtect,
                                          MAX_FAST_REFS))
        {
            /* Insert references */
            if (!ExInsertFastReference(&CallBack->RoutineBlock, CallbackBlock))
            {
                /* Backdown the rundown acquire */
                ExfReleaseRundownProtectionEx(&CallbackBlock->RundownProtect,
                                              MAX_FAST_REFS);
            }
        }
    }

    /* Return the callback block */
    return CallbackBlock;
}

BOOLEAN
NTAPI
ExCompareExchangeCallBack(IN OUT PEX_CALLBACK CallBack,
                          IN PEX_CALLBACK_ROUTINE_BLOCK NewBlock,
                          IN PEX_CALLBACK_ROUTINE_BLOCK OldBlock)
{
    EX_FAST_REF OldValue;
    PEX_CALLBACK_ROUTINE_BLOCK CallbackBlock;
    ULONG Count;

    /* Check that we have a new block */
    if (NewBlock)
    {
        /* Acquire rundown */
        if (!ExfAcquireRundownProtectionEx(&NewBlock->RundownProtect,
                                           MAX_FAST_REFS + 1))
        {
            /* This should never happen */
            ASSERTMSG("Callback block is already undergoing rundown\n", FALSE);
            return FALSE;
        }
    }

    /* Do the swap */
    OldValue = ExCompareSwapFastReference(&CallBack->RoutineBlock,
                                          NewBlock,
                                          OldBlock);

    /* Get the routine block */
    CallbackBlock = ExGetObjectFastReference(OldValue);
    Count = ExGetCountFastReference(OldValue);

    /* Make sure the swap worked */
    if (CallbackBlock == OldBlock)
    {
        /* Make sure we replaced a valid pointer */
        if (CallbackBlock)
        {
            /* Acquire the flush lock and immediately release it */
            KeEnterCriticalRegion();
            ExWaitOnPushLock(&ExpCallBackFlush);

            /* Release rundown protection */
            KeLeaveCriticalRegion();
            ExfReleaseRundownProtectionEx(&CallbackBlock->RundownProtect,
                                          Count + 1);
        }

        /* Compare worked */
        return TRUE;
    }
    else
    {
        /* It failed, check if we had a block */
        if (NewBlock)
        {
            /* We did, remove the references that we had added */
            ExfReleaseRundownProtectionEx(&NewBlock->RundownProtect,
                                          MAX_FAST_REFS + 1);
        }

        /* Return failure */
        return FALSE;
    }
}

VOID
NTAPI
ExpDeleteCallback(IN PVOID Object)
{
    /* Sanity check */
    ASSERT(IsListEmpty(&((PCALLBACK_OBJECT)Object)->RegisteredCallbacks));
}

/*++
 * @name ExpInitializeCallbacks
 *
 * Creates the Callback Object as a valid Object Type in the Kernel.
 * Internal function, subject to further review
 *
 * @return TRUE if the Callback Object Type was successfully created.
 *
 * @remarks None
 *
 *--*/
CODE_SEG("INIT")
BOOLEAN
NTAPI
ExpInitializeCallbacks(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    UNICODE_STRING DirName = RTL_CONSTANT_STRING(L"\\Callback");
    UNICODE_STRING CallbackName;
    UNICODE_STRING Name;
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    HANDLE DirectoryHandle;
    ULONG i;

    /* Setup lightweight callback lock */
    ExpCallBackFlush.Value = 0;

    /* Initialize the Callback Object type  */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Callback");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    ObjectTypeInitializer.GenericMapping = ExpCallbackMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.DeleteProcedure = ExpDeleteCallback;
    ObjectTypeInitializer.ValidAccessMask = CALLBACK_ALL_ACCESS;
    Status = ObCreateObjectType(&Name,
                                &ObjectTypeInitializer,
                                NULL,
                                &ExCallbackObjectType);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Initialize the Object */
    InitializeObjectAttributes(&ObjectAttributes,
                               &DirName,
                               OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                               NULL,
                               SePublicDefaultSd);

    /* Create the Object Directory */
    Status = NtCreateDirectoryObject(&DirectoryHandle,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Close Handle... */
    NtClose(DirectoryHandle);

    /* Initialize Event used when unregistering */
    KeInitializeEvent(&ExpCallbackEvent, NotificationEvent, 0);

    /* Default NT Kernel Callbacks. */
    for (i = 0; ExpInitializeCallback[i].CallbackObject; i++)
    {
        /* Create the name from the structure */
        RtlInitUnicodeString(&CallbackName, ExpInitializeCallback[i].Name);

        /* Initialize the Object Attributes Structure */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &CallbackName,
                                   OBJ_PERMANENT | OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        /* Create the Callback Object */
        Status = ExCreateCallback(ExpInitializeCallback[i].CallbackObject,
                                  &ObjectAttributes,
                                  TRUE,
                                  TRUE);
        if (!NT_SUCCESS(Status)) return FALSE;
    }

    /* Everything successful */
    return TRUE;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name ExCreateCallback
 * @implemented
 *
 * Opens or creates a Callback Object. Creates only if Create is true.
 * Allows multiple Callback Functions to be registered only if
 * AllowMultipleCallbacks is true.
 * See: https://web.archive.org/web/20081230235552/http://www.osronline.com/DDKx/kmarch/k102_967m.htm
 *      https://www.osronline.com/article.cfm%5eid=24.htm
 *
 * @param CallbackObject
 *        Pointer that will receive the Callback Object.
 *
 * @param CallbackName
 *        Name of Callback
 *
 * @param Create
 *        Determines if the object will be created if it doesn't exit
 *
 * @param AllowMultipleCallbacks
 *        Determines if more then one registered callback function
 *        can be attached to this Callback Object.
 *
 * @return STATUS_SUCESS if not failed.
 *
 * @remarks Must be called at IRQL = PASSIVE_LEVEL
 *
 *--*/
NTSTATUS
NTAPI
ExCreateCallback(OUT PCALLBACK_OBJECT *CallbackObject,
                 IN POBJECT_ATTRIBUTES ObjectAttributes,
                 IN BOOLEAN Create,
                 IN BOOLEAN AllowMultipleCallbacks)
{
    PCALLBACK_OBJECT Callback = NULL;
    NTSTATUS Status;
    HANDLE Handle = NULL;
    PAGED_CODE();

    /* Open a handle to the callback if it exists */
    if (ObjectAttributes->ObjectName)
    {
        /* Open the handle */
        Status = ObOpenObjectByName(ObjectAttributes,
                                    ExCallbackObjectType,
                                    KernelMode,
                                    NULL,
                                    0,
                                    NULL,
                                    &Handle);
    }
    else
    {
        /* Otherwise, fail */
        Status = STATUS_UNSUCCESSFUL;
    }

    /* We weren't able to open it...should we create it? */
    if (!(NT_SUCCESS(Status)) && (Create))
    {
        /* Create the object */
        Status = ObCreateObject(KernelMode,
                                ExCallbackObjectType,
                                ObjectAttributes,
                                KernelMode,
                                NULL,
                                sizeof(CALLBACK_OBJECT),
                                0,
                                0,
                                (PVOID *)&Callback);
        if (NT_SUCCESS(Status))
        {
            /* Set it up */
            Callback->Signature = 'llaC';
            KeInitializeSpinLock(&Callback->Lock);
            InitializeListHead(&Callback->RegisteredCallbacks);
            Callback->AllowMultipleCallbacks = AllowMultipleCallbacks;

            /* Insert the object into the object namespace */
            Status = ObInsertObject(Callback,
                                    NULL,
                                    FILE_READ_DATA,
                                    0,
                                    NULL,
                                    &Handle);
        }
    }

    /* Check if we have success until here */
    if (NT_SUCCESS(Status))
    {
        /* Get a pointer to the new object from the handle we just got */
        Status = ObReferenceObjectByHandle(Handle,
                                           0,
                                           ExCallbackObjectType,
                                           KernelMode,
                                           (PVOID *)&Callback,
                                           NULL);

        /* Close the Handle, since we now have the pointer */
        ZwClose(Handle);
    }

    /* Everything went fine, so return a pointer to the Object */
    if (NT_SUCCESS(Status))
    {
        *CallbackObject = Callback;
    }
    return Status;
}

/*++
 * @name ExNotifyCallback
 * @implemented
 *
 * Calls a function pointer (a registered callback)
 * See: https://web.archive.org/web/20090106214158/http://www.osronline.com/DDKx/kmarch/k102_2f5e.htm
 *      http://vmsone.com/~decuslib/vmssig/vmslt99b/nt/wdm-callback.txt (DEAD_LINK)
 *
 * @param CallbackObject
 *        Which callback to call
 *
 * @param Argument1
 *        Pointer/data to send to callback function
 *
 * @param Argument2
 *        Pointer/data to send to callback function
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
ExNotifyCallback(IN PCALLBACK_OBJECT CallbackObject,
                 IN PVOID Argument1,
                 IN PVOID Argument2)
{
    PLIST_ENTRY RegisteredCallbacks;
    PCALLBACK_REGISTRATION CallbackRegistration;
    KIRQL OldIrql;

    /* Check if we don't have an object or registrations */
    if (!(CallbackObject) ||
        (IsListEmpty(&CallbackObject->RegisteredCallbacks)))
    {
        /* Don't notify */
        return;
    }

    /* Acquire the Lock */
    KeAcquireSpinLock(&CallbackObject->Lock, &OldIrql);

    /* Enumerate through all the registered functions */
    for (RegisteredCallbacks = CallbackObject->RegisteredCallbacks.Flink;
         RegisteredCallbacks != &CallbackObject->RegisteredCallbacks;
         RegisteredCallbacks = RegisteredCallbacks->Flink)
    {
        /* Get a pointer to a Callback Registration from the List Entries */
        CallbackRegistration = CONTAINING_RECORD(RegisteredCallbacks,
                                                 CALLBACK_REGISTRATION,
                                                 Link);

        /* Don't bother doing notification if it's pending to be deleted */
        if (!CallbackRegistration->UnregisterWaiting)
        {
            /* Mark the Callback in use, so it won't get deleted */
            CallbackRegistration->Busy += 1;

            /* Release the Spinlock before making the call */
            KeReleaseSpinLock(&CallbackObject->Lock, OldIrql);

            /* Call the Registered Function */
            CallbackRegistration->CallbackFunction(CallbackRegistration->
                                                   CallbackContext,
                                                   Argument1,
                                                   Argument2);

            /* Get SpinLock back */
            KeAcquireSpinLock(&CallbackObject->Lock, &OldIrql);

            /* We are not in use anymore */
            CallbackRegistration->Busy -= 1;

            /* Check if removal is pending and we're not active */
            if ((CallbackRegistration->UnregisterWaiting) &&
                !(CallbackRegistration->Busy))
            {
                /* Signal the callback event */
                KeSetEvent(&ExpCallbackEvent, 0, FALSE);
            }
        }
    }

    /* Release the Callback Object */
    KeReleaseSpinLock(&CallbackObject->Lock, OldIrql);
}

/*++
 * @name ExRegisterCallback
 * @implemented
 *
 * Allows a function to associate a callback pointer (Function) to
 * a created Callback object
 * See: DDK, OSR, links in ExNotifyCallback
 *
 * @param CallbackObject
 *        The Object Created with ExCreateCallBack
 *
 * @param CallBackFunction
 *        Pointer to the function to be called back
 *
 * @param CallBackContext
 *        Block of memory that can contain user-data which will be
 *        passed on to the callback
 *
 * @return A handle to a Callback Registration Structure (MSDN Documentation)
 *
 * @remarks None
 *
 *--*/
PVOID
NTAPI
ExRegisterCallback(IN PCALLBACK_OBJECT CallbackObject,
                   IN PCALLBACK_FUNCTION CallbackFunction,
                   IN PVOID CallbackContext)
{
    PCALLBACK_REGISTRATION CallbackRegistration = NULL;
    KIRQL OldIrql;

    /* Sanity checks */
    ASSERT(CallbackFunction);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Create reference to Callback Object */
    ObReferenceObject(CallbackObject);

    /* Allocate memory for the structure */
    CallbackRegistration = ExAllocatePoolWithTag(NonPagedPool,
                                                 sizeof(CALLBACK_REGISTRATION),
                                                 TAG_CALLBACK_REGISTRATION);
    if (!CallbackRegistration)
    {
        /* Dereference and fail */
        ObDereferenceObject (CallbackObject);
        return NULL;
    }

    /* Create Callback Registration */
    CallbackRegistration->CallbackObject = CallbackObject;
    CallbackRegistration->CallbackFunction = CallbackFunction;
    CallbackRegistration->CallbackContext = CallbackContext;
    CallbackRegistration->Busy = 0;
    CallbackRegistration->UnregisterWaiting = FALSE;

    /* Acquire SpinLock */
    KeAcquireSpinLock(&CallbackObject->Lock, &OldIrql);

    /* Check if 1) No Callbacks registered or 2) Multiple Callbacks allowed */
    if ((CallbackObject->AllowMultipleCallbacks) ||
        (IsListEmpty(&CallbackObject->RegisteredCallbacks)))
    {
        /* Register the callback */
        InsertTailList(&CallbackObject->RegisteredCallbacks,
                       &CallbackRegistration->Link);

        /* Release SpinLock */
        KeReleaseSpinLock(&CallbackObject->Lock, OldIrql);
    }
    else
    {
        /* Release SpinLock */
        KeReleaseSpinLock(&CallbackObject->Lock, OldIrql);

        /* Free the registration */
        ExFreePoolWithTag(CallbackRegistration, TAG_CALLBACK_REGISTRATION);
        CallbackRegistration = NULL;

        /* Dereference the object */
        ObDereferenceObject(CallbackObject);
    }

    /* Return handle to Registration Object */
    return (PVOID)CallbackRegistration;
}

/*++
 * @name ExUnregisterCallback
 * @implemented
 *
 * Deregisters a CallBack
 * See: DDK, OSR, links in ExNotifyCallback
 *
 * @param CallbackRegistration
 *        Callback Registration Handle
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
ExUnregisterCallback(IN PVOID CallbackRegistrationHandle)
{
    PCALLBACK_REGISTRATION CallbackRegistration;
    PCALLBACK_OBJECT CallbackObject;
    KIRQL OldIrql;
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Convert Handle to valid Structure Pointer */
    CallbackRegistration = (PCALLBACK_REGISTRATION)CallbackRegistrationHandle;

    /* Get the Callback Object */
    CallbackObject = CallbackRegistration->CallbackObject;

    /* Lock the Object */
    KeAcquireSpinLock (&CallbackObject->Lock, &OldIrql);

    /* We can't Delete the Callback if it's in use */
    while (CallbackRegistration->Busy)
    {
        /* Let everyone else know we're unregistering */
        CallbackRegistration->UnregisterWaiting = TRUE;

        /* We are going to wait for the event, so the Lock isn't necessary */
        KeReleaseSpinLock(&CallbackObject->Lock, OldIrql);

        /* Make sure the event is cleared */
        KeClearEvent(&ExpCallbackEvent);

        /* Wait for the Event */
        KeWaitForSingleObject(&ExpCallbackEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        /* We need the Lock again */
        KeAcquireSpinLock(&CallbackObject->Lock, &OldIrql);
    }

    /* Remove the Callback */
    RemoveEntryList(&CallbackRegistration->Link);

    /* It's now safe to release the lock */
    KeReleaseSpinLock(&CallbackObject->Lock, OldIrql);

    /* Delete this registration */
    ExFreePoolWithTag(CallbackRegistration, TAG_CALLBACK_REGISTRATION);

    /* Remove the reference */
    ObDereferenceObject(CallbackObject);
}

/* EOF */

/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/include/internal/ob_x.h
* PURPOSE:         Internal Inlined Functions for the Object Manager
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

#include "ex.h"

#define OBP_LOCK_STATE_PRE_ACQUISITION_EXCLUSIVE    0xAAAA1234
#define OBP_LOCK_STATE_PRE_ACQUISITION_SHARED       0xBBBB1234
#define OBP_LOCK_STATE_POST_ACQUISITION_EXCLUSIVE   0xCCCC1234
#define OBP_LOCK_STATE_POST_ACQUISITION_SHARED      0xDDDD1234
#define OBP_LOCK_STATE_RELEASED                     0xEEEE1234
#define OBP_LOCK_STATE_INITIALIZED                  0xFFFF1234

#define OBP_NAME_LOOKASIDE_MAX_SIZE 248

FORCEINLINE
ULONG
ObpValidateAttributes(IN ULONG Attributes,
                      IN KPROCESSOR_MODE PreviousMode)
{
    if (PreviousMode == KernelMode)
    {
        /* For kernel, allow any valid attributes */
        return Attributes & OBJ_VALID_KERNEL_ATTRIBUTES;
    }
    else
    {
        /* For user, mask out kernel-only attributes */
        return (Attributes & OBJ_VALID_ATTRIBUTES) &
               ~(OBJ_KERNEL_HANDLE);
    }
}

FORCEINLINE
ULONG
ObpSelectObjectLockSlot(IN POBJECT_HEADER ObjectHeader)
{
    /* We have 4 locks total, this will return a 0-index slot */
    return (((ULONG_PTR)ObjectHeader) >> 8) & 3;
}

FORCEINLINE
VOID
ObpAcquireObjectLock(IN POBJECT_HEADER ObjectHeader)
{
    ULONG Slot;
    POBJECT_TYPE ObjectType = ObjectHeader->Type;

    /* Sanity check */
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    /* Pick a slot */
    Slot = ObpSelectObjectLockSlot(ObjectHeader);

    /* Enter a critical region and acquire the resource */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&ObjectType->ObjectLocks[Slot], TRUE);
}

FORCEINLINE
VOID
ObpAcquireObjectLockShared(IN POBJECT_HEADER ObjectHeader)
{
    ULONG Slot;
    POBJECT_TYPE ObjectType = ObjectHeader->Type;

    /* Sanity check */
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    /* Pick a slot */
    Slot = ObpSelectObjectLockSlot(ObjectHeader);

    /* Enter a critical region and acquire the resource */
    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(&ObjectType->ObjectLocks[Slot], TRUE);
}

FORCEINLINE
VOID
ObpReleaseObjectLock(IN POBJECT_HEADER ObjectHeader)
{
    ULONG Slot;
    POBJECT_TYPE ObjectType = ObjectHeader->Type;

    /* Pick a slot */
    Slot = ObpSelectObjectLockSlot(ObjectHeader);

    /* Release the resource and leave a critical region */
    ExReleaseResourceLite(&ObjectType->ObjectLocks[Slot]);
    KeLeaveCriticalRegion();

    /* Sanity check */
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
}

FORCEINLINE
POBJECT_HEADER_NAME_INFO
ObpReferenceNameInfo(IN POBJECT_HEADER ObjectHeader)
{
    POBJECT_HEADER_NAME_INFO ObjectNameInfo;
    ULONG NewValue, References;

    /* Make sure we have name information at all */
    ObjectNameInfo = OBJECT_HEADER_TO_NAME_INFO(ObjectHeader);
    if (!ObjectNameInfo) return NULL;

    /* Get the number of references */
    References = ObjectNameInfo->QueryReferences;
    for (;;)
    {
        /* Check if the count is 0 and fail if so */
        if (!References) return NULL;

        /* Increment the number of references */
        NewValue = InterlockedCompareExchange((PLONG)&ObjectNameInfo->
                                              QueryReferences,
                                              References + 1,
                                              References);
        if (NewValue == References) break;

        /* We failed, try again */
        References = NewValue;
    }

    /* Check for magic flag */
    if (ObjectNameInfo->QueryReferences & 0x80000000)
    {
        /* FIXME: Unhandled*/
        DbgPrint("OB: Unhandled path\n");
        ASSERT(FALSE);
    }

    /* Return the name information */
    return ObjectNameInfo;
}

FORCEINLINE
VOID
ObpDereferenceNameInfo(IN POBJECT_HEADER_NAME_INFO HeaderNameInfo)
{
    POBJECT_DIRECTORY Directory;

    /* Bail out if there's no info at all */
    if (!HeaderNameInfo) return;

    /* Remove a query reference and check if it was the last one */
    if (!InterlockedDecrement((PLONG)&HeaderNameInfo->QueryReferences))
    {
        /* Check if we have a name */
        if (HeaderNameInfo->Name.Buffer)
        {
            /* We can get rid of the object name now */
            ExFreePoolWithTag(HeaderNameInfo->Name.Buffer, OB_NAME_TAG);
            RtlInitEmptyUnicodeString(&HeaderNameInfo->Name, NULL, 0);
        }

        /* Check if the object has a directory associated to it */
        Directory = HeaderNameInfo->Directory;
        if (Directory)
        {
            /* Delete the directory */
            HeaderNameInfo->Directory = NULL;
            ObDereferenceObjectDeferDelete(Directory);
        }
    }
}

/**
 * @brief
 * Locks a directory for shared access.
 * Used for reading members of the directory object.
 *
 * @param[in] Directory
 * The directory to lock.
 *
 * @param[in] Context
 * The lookup lock context.
 */
FORCEINLINE
VOID
ObpAcquireDirectoryLockShared(IN POBJECT_DIRECTORY Directory,
                              IN POBP_LOOKUP_CONTEXT Context)
{
    /* Update lock flag */
    Context->LockStateSignature = OBP_LOCK_STATE_PRE_ACQUISITION_SHARED;

    /* Acquire an shared directory lock */
    KeEnterCriticalRegion();
    ExAcquirePushLockShared(&Directory->Lock);

    /* Update lock flag */
    Context->LockStateSignature = OBP_LOCK_STATE_POST_ACQUISITION_SHARED;
}

/**
 * @brief
 * Locks a directory for exclusive access.
 * Used for writing/reading members of the directory object.
 *
 * @param[in] Directory
 * The directory to lock.
 *
 * @param[in] Context
 * The lookup lock context.
 */
FORCEINLINE
VOID
ObpAcquireDirectoryLockExclusive(IN POBJECT_DIRECTORY Directory,
                                 IN POBP_LOOKUP_CONTEXT Context)
{
    /* Update lock flag */
    Context->LockStateSignature = OBP_LOCK_STATE_PRE_ACQUISITION_EXCLUSIVE;

    /* Acquire an exclusive directory lock */
    KeEnterCriticalRegion();
    ExAcquirePushLockExclusive(&Directory->Lock);

    /* Update lock flag */
    Context->LockStateSignature = OBP_LOCK_STATE_POST_ACQUISITION_EXCLUSIVE;
}

/**
 * @brief
 * Unlocks a previously shared or exclusively locked directory.
 *
 * @param[in] Directory
 * The directory to unlock.
 *
 * @param[in] Context
 * The lookup lock context.
 */
FORCEINLINE
VOID
ObpReleaseDirectoryLock(IN POBJECT_DIRECTORY Directory,
                        IN POBP_LOOKUP_CONTEXT Context)
{
    /* Release the lock */
    ExReleasePushLock(&Directory->Lock);
    Context->LockStateSignature = OBP_LOCK_STATE_RELEASED;
    KeLeaveCriticalRegion();
}

/**
 * @brief
 * Initializes a new object directory lookup context.
 * Used for lookup operations (insertions/deletions) in a directory.
 * Employed in conjunction with the directory locking functions.
 *
 * @param[in] Context
 * The new lookup context to initialize.
 */
FORCEINLINE
VOID
ObpInitializeLookupContext(IN POBP_LOOKUP_CONTEXT Context)
{
    /* Initialize a null context */
    Context->Object = NULL;
    Context->Directory = NULL;
    Context->DirectoryLocked = FALSE;
    Context->LockStateSignature = OBP_LOCK_STATE_INITIALIZED;
}

/**
 * @brief
 * Locks an object directory lookup context for performing
 * lookup operations (insertions/deletions) in a directory.
 * The directory is locked for exclusive access.
 *
 * @param[in] Context
 * The lookup context to lock.
 *
 * @param[in] Directory
 * The directory on which the lookup context applies.
 */
FORCEINLINE
VOID
ObpAcquireLookupContextLock(IN POBP_LOOKUP_CONTEXT Context,
                            IN POBJECT_DIRECTORY Directory)
{
    /* Acquire an exclusive directory lock and save its lock state */
    ObpAcquireDirectoryLockExclusive(Directory, Context);
    Context->Directory = Directory;
    Context->DirectoryLocked = TRUE;
}

FORCEINLINE
VOID
ObpReleaseLookupContextObject(IN POBP_LOOKUP_CONTEXT Context)
{
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_NAME_INFO HeaderNameInfo;

    /* Check if we had an object */
    if (Context->Object)
    {
        /* Get the object name information */
        ObjectHeader = OBJECT_TO_OBJECT_HEADER(Context->Object);
        HeaderNameInfo = OBJECT_HEADER_TO_NAME_INFO(ObjectHeader);

        /* Release the name information */
        ObpDereferenceNameInfo(HeaderNameInfo);

        /* Dereference the object */
        ObDereferenceObject(Context->Object);
        Context->Object = NULL;
    }
}

/**
 * @brief
 * Releases an initialized object directory lookup context.
 * Unlocks it if necessary, and dereferences the underlying object.
 *
 * @param[in] Context
 * The lookup context to release.
 */
FORCEINLINE
VOID
ObpReleaseLookupContext(IN POBP_LOOKUP_CONTEXT Context)
{
    /* Check if we came back with the directory locked */
    if (Context->DirectoryLocked)
    {
        /* Release the directory lock */
        ObpReleaseDirectoryLock(Context->Directory, Context);
        Context->Directory = NULL;
        Context->DirectoryLocked = FALSE;
    }

    /* Clear the context */
    ObpReleaseLookupContextObject(Context);
}

FORCEINLINE
VOID
ObpEnterObjectTypeMutex(IN POBJECT_TYPE ObjectType)
{
    /* Sanity check */
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    /* Enter a critical region and acquire the resource */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&ObjectType->Mutex, TRUE);
}

FORCEINLINE
VOID
ObpLeaveObjectTypeMutex(IN POBJECT_TYPE ObjectType)
{
    /* Enter a critical region and acquire the resource */
    ExReleaseResourceLite(&ObjectType->Mutex);
    KeLeaveCriticalRegion();

    /* Sanity check */
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
}

FORCEINLINE
VOID
ObpReleaseObjectCreateInformation(IN POBJECT_CREATE_INFORMATION ObjectCreateInfo)
{
    /* Check if we have a security descriptor */
    if (ObjectCreateInfo->SecurityDescriptor)
    {
        /* Release it */
        SeReleaseSecurityDescriptor(ObjectCreateInfo->SecurityDescriptor,
                                    ObjectCreateInfo->ProbeMode,
                                    TRUE);
        ObjectCreateInfo->SecurityDescriptor = NULL;
    }
}

FORCEINLINE
PVOID
ObpAllocateObjectCreateInfoBuffer(IN PP_NPAGED_LOOKASIDE_NUMBER Type)
{
    PVOID Buffer;
    PNPAGED_LOOKASIDE_LIST List;
    PKPRCB Prcb = KeGetCurrentPrcb();

    /* Get the P list first */
    List = (PNPAGED_LOOKASIDE_LIST)Prcb->PPLookasideList[Type].P;

    /* Attempt allocation */
    List->L.TotalAllocates++;
    Buffer = (PVOID)InterlockedPopEntrySList(&List->L.ListHead);
    if (!Buffer)
    {
        /* Let the balancer know that the P list failed */
        List->L.AllocateMisses++;

        /* Try the L List */
        List = (PNPAGED_LOOKASIDE_LIST)Prcb->PPLookasideList[Type].L;
        List->L.TotalAllocates++;
        Buffer = (PVOID)InterlockedPopEntrySList(&List->L.ListHead);
        if (!Buffer)
        {
            /* Let the balancer know the L list failed too */
            List->L.AllocateMisses++;

            /* Allocate it */
            Buffer = List->L.Allocate(List->L.Type, List->L.Size, List->L.Tag);
        }
    }

    /* Return buffer */
    return Buffer;
}

FORCEINLINE
VOID
ObpFreeCapturedAttributes(IN PVOID Buffer,
                          IN PP_NPAGED_LOOKASIDE_NUMBER Type)
{
    PNPAGED_LOOKASIDE_LIST List;
    PKPRCB Prcb = KeGetCurrentPrcb();

    /* Use the P List */
    List = (PNPAGED_LOOKASIDE_LIST)Prcb->PPLookasideList[Type].P;
    List->L.TotalFrees++;

    /* Check if the Free was within the Depth or not */
    if (ExQueryDepthSList(&List->L.ListHead) >= List->L.Depth)
    {
        /* Let the balancer know */
        List->L.FreeMisses++;

        /* Use the L List */
        List = (PNPAGED_LOOKASIDE_LIST)Prcb->PPLookasideList[Type].L;
        List->L.TotalFrees++;

        /* Check if the Free was within the Depth or not */
        if (ExQueryDepthSList(&List->L.ListHead) >= List->L.Depth)
        {
            /* All lists failed, use the pool */
            List->L.FreeMisses++;
            List->L.Free(Buffer);
        }
        else
        {
            /* The free was within the Depth */
            InterlockedPushEntrySList(&List->L.ListHead,
                                      (PSLIST_ENTRY)Buffer);
        }
    }
    else
    {
        /* The free was within the Depth */
        InterlockedPushEntrySList(&List->L.ListHead,
                                  (PSLIST_ENTRY)Buffer);
    }
}

FORCEINLINE
VOID
ObpFreeObjectCreateInformation(IN POBJECT_CREATE_INFORMATION ObjectCreateInfo)
{
    /* First release the attributes, then free them from the lookaside list */
    ObpReleaseObjectCreateInformation(ObjectCreateInfo);
    ObpFreeCapturedAttributes(ObjectCreateInfo, LookasideCreateInfoList);
}

#if DBG
FORCEINLINE
VOID
ObpCalloutStart(IN PKIRQL CalloutIrql)
{
    /* Save the callout IRQL */
    *CalloutIrql = KeGetCurrentIrql();
}

FORCEINLINE
VOID
ObpCalloutEnd(IN KIRQL CalloutIrql,
              IN PCHAR Procedure,
              IN POBJECT_TYPE ObjectType,
              IN PVOID Object)
{
    /* Detect IRQL change */
    if (CalloutIrql != KeGetCurrentIrql())
    {
        /* Print error */
        DbgPrint("OB: ObjectType: %wZ  Procedure: %s  Object: %p\n",
                 &ObjectType->Name, Procedure, Object);
        DbgPrint("    Returned at %x IRQL, but was called at %x IRQL\n",
                 KeGetCurrentIrql(), CalloutIrql);
        DbgBreakPoint();
    }
}
#else
FORCEINLINE
VOID
ObpCalloutStart(IN PKIRQL CalloutIrql)
{
    /* No-op */
    UNREFERENCED_PARAMETER(CalloutIrql);
}

FORCEINLINE
VOID
ObpCalloutEnd(IN KIRQL CalloutIrql,
              IN PCHAR Procedure,
              IN POBJECT_TYPE ObjectType,
              IN PVOID Object)
{
    UNREFERENCED_PARAMETER(CalloutIrql);
}
#endif

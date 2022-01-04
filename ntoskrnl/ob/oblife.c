/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ob/oblife.c
 * PURPOSE:         Manages the lifetime of an Object, including its creation,
 *                  and deletion, as well as setting or querying any of its
 *                  information while it is active. Since Object Types are also
 *                  Objects, those are also managed here.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl
 *                  Thomas Weidenmueller (w3seek@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

extern ULONG NtGlobalFlag;

POBJECT_TYPE ObpTypeObjectType = NULL;
KEVENT ObpDefaultObject;
KGUARDED_MUTEX ObpDeviceMapLock;

GENERAL_LOOKASIDE ObpNameBufferLookasideList, ObpCreateInfoLookasideList;

WORK_QUEUE_ITEM ObpReaperWorkItem;
volatile PVOID ObpReaperList;

ULONG ObpObjectsCreated, ObpObjectsWithName, ObpObjectsWithPoolQuota;
ULONG ObpObjectsWithHandleDB, ObpObjectsWithCreatorInfo;
POBJECT_TYPE ObpObjectTypes[32];

/* PRIVATE FUNCTIONS *********************************************************/

VOID
FASTCALL
ObpDeallocateObject(IN PVOID Object)
{
    PVOID HeaderLocation;
    POBJECT_HEADER Header;
    POBJECT_TYPE ObjectType;
    POBJECT_HEADER_HANDLE_INFO HandleInfo;
    POBJECT_HEADER_NAME_INFO NameInfo;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;
    POBJECT_HEADER_QUOTA_INFO QuotaInfo;
    ULONG PagedPoolCharge, NonPagedPoolCharge;
    PAGED_CODE();

    /* Get the header and assume this is what we'll free */
    Header = OBJECT_TO_OBJECT_HEADER(Object);
    ObjectType = Header->Type;
    HeaderLocation = Header;

    /* To find the header, walk backwards from how we allocated */
    if ((CreatorInfo = OBJECT_HEADER_TO_CREATOR_INFO(Header)))
    {
        HeaderLocation = CreatorInfo;
    }
    if ((NameInfo = OBJECT_HEADER_TO_NAME_INFO(Header)))
    {
        HeaderLocation = NameInfo;
    }
    if ((HandleInfo = OBJECT_HEADER_TO_HANDLE_INFO(Header)))
    {
        HeaderLocation = HandleInfo;
    }
    if ((QuotaInfo = OBJECT_HEADER_TO_QUOTA_INFO(Header)))
    {
        HeaderLocation = QuotaInfo;
    }

    /* Decrease the total */
    InterlockedDecrement((PLONG)&ObjectType->TotalNumberOfObjects);

    /* Check if we have create info */
    if (Header->Flags & OB_FLAG_CREATE_INFO)
    {
        /* Double-check that it exists */
        if (Header->ObjectCreateInfo)
        {
            /* Free it */
            ObpFreeObjectCreateInformation(Header->ObjectCreateInfo);
            Header->ObjectCreateInfo = NULL;
        }
    }
    else
    {
        /* Check if it has a quota block */
        if (Header->QuotaBlockCharged)
        {
            /* Check if we have quota information */
            if (QuotaInfo)
            {
                /* Get charges from quota information */
                PagedPoolCharge = QuotaInfo->PagedPoolCharge +
                                  QuotaInfo->SecurityDescriptorCharge;
                NonPagedPoolCharge = QuotaInfo->NonPagedPoolCharge;
            }
            else
            {
                /* Get charges from object type */
                PagedPoolCharge = ObjectType->TypeInfo.DefaultPagedPoolCharge;
                NonPagedPoolCharge = ObjectType->
                                     TypeInfo.DefaultNonPagedPoolCharge;

                /* Add the SD charge too */
                if (Header->Flags & OB_FLAG_SECURITY) PagedPoolCharge += 2048;
            }

            /* Return the quota */
            DPRINT("FIXME: Should return quotas: %lx %lx\n", PagedPoolCharge, NonPagedPoolCharge);
#if 0
            PsReturnSharedPoolQuota(ObjectHeader->QuotaBlockCharged,
                                    PagedPoolCharge,
                                    NonPagedPoolCharge);
#endif

        }
    }

    /* Check if a handle database was active */
    if ((HandleInfo) && !(Header->Flags & OB_FLAG_SINGLE_PROCESS))
    {
        /* Free it */
        ExFreePool(HandleInfo->HandleCountDatabase);
        HandleInfo->HandleCountDatabase = NULL;
    }

    /* Check if we have a name */
    if ((NameInfo) && (NameInfo->Name.Buffer))
    {
        /* Free it */
        ExFreePool(NameInfo->Name.Buffer);
        NameInfo->Name.Buffer = NULL;
    }

    /* Catch invalid access */
    Header->Type = (POBJECT_TYPE)(ULONG_PTR)0xBAADB0B0BAADB0B0ULL;

    /* Free the object using the same allocation tag */
    ExFreePoolWithTag(HeaderLocation, ObjectType->Key);
}

VOID
NTAPI
ObpDeleteObject(IN PVOID Object,
                IN BOOLEAN CalledFromWorkerThread)
{
    POBJECT_HEADER Header;
    POBJECT_TYPE ObjectType;
    POBJECT_HEADER_NAME_INFO NameInfo;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;
    KIRQL CalloutIrql;
    PAGED_CODE();

    /* Get the header and type */
    Header = OBJECT_TO_OBJECT_HEADER(Object);
    ObjectType = Header->Type;

    /* Get creator and name information */
    NameInfo = OBJECT_HEADER_TO_NAME_INFO(Header);
    CreatorInfo = OBJECT_HEADER_TO_CREATOR_INFO(Header);

    /* Check if the object is on a type list */
    if ((CreatorInfo) && !(IsListEmpty(&CreatorInfo->TypeList)))
    {
        /* Lock the object type */
        ObpEnterObjectTypeMutex(ObjectType);

        /* Remove the object from the type list */
        RemoveEntryList(&CreatorInfo->TypeList);

        /* Release the lock */
        ObpLeaveObjectTypeMutex(ObjectType);
    }

    /* Check if we have a name */
    if ((NameInfo) && (NameInfo->Name.Buffer))
    {
        /* Free it */
        ExFreePool(NameInfo->Name.Buffer);
        RtlInitEmptyUnicodeString(&NameInfo->Name, NULL, 0);
    }

    /* Check if we have a security descriptor */
    if (Header->SecurityDescriptor)
    {
        /* Call the security procedure to delete it */
        ObpCalloutStart(&CalloutIrql);
        ObjectType->TypeInfo.SecurityProcedure(Object,
                                               DeleteSecurityDescriptor,
                                               0,
                                               NULL,
                                               NULL,
                                               &Header->SecurityDescriptor,
                                               0,
                                               NULL);
        ObpCalloutEnd(CalloutIrql, "Security", ObjectType, Object);
    }

    /* Check if we have a delete procedure */
    if (ObjectType->TypeInfo.DeleteProcedure)
    {
        /* Save whether we were deleted from worker thread or not */
        if (!CalledFromWorkerThread) Header->Flags |= OB_FLAG_DEFER_DELETE;

        /* Call it */
        ObpCalloutStart(&CalloutIrql);
        ObjectType->TypeInfo.DeleteProcedure(Object);
        ObpCalloutEnd(CalloutIrql, "Delete", ObjectType, Object);
    }

    /* Now de-allocate all object members */
    ObpDeallocateObject(Object);
}

VOID
NTAPI
ObpReapObject(IN PVOID Parameter)
{
    POBJECT_HEADER ReapObject, NextObject;

    /* Start reaping */
    do
    {
        /* Get the reap object */
        ReapObject = InterlockedExchangePointer(&ObpReaperList, (PVOID)1);

        /* Start deletion loop */
        do
        {
            /* Get the next object */
            NextObject = ReapObject->NextToFree;

            /* Delete the object */
            ObpDeleteObject(&ReapObject->Body, TRUE);

            /* Move to the next one */
            ReapObject = NextObject;
        } while ((ReapObject) && (ReapObject != (PVOID)1));
    } while ((ObpReaperList != (PVOID)1) ||
             (InterlockedCompareExchange((PLONG)&ObpReaperList, 0, 1) != 1));
}

/*++
* @name ObpSetPermanentObject
*
*     The ObpSetPermanentObject routine makes an sets or clears the permanent
*     flag of an object, thus making it either permanent or temporary.
*
* @param ObjectBody
*        Pointer to the object to make permanent or temporary.
*
* @param Permanent
*        Flag specifying which operation to perform.
*
* @return None.
*
* @remarks If the object is being made temporary, then it will be checked
*          as a candidate for immediate removal from the namespace.
*
*--*/
VOID
FASTCALL
ObpSetPermanentObject(IN PVOID ObjectBody,
                      IN BOOLEAN Permanent)
{
    POBJECT_HEADER ObjectHeader;

    /* Get the header */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(ObjectBody);

    /* Acquire object lock */
    ObpAcquireObjectLock(ObjectHeader);

    /* Check what we're doing to it */
    if (Permanent)
    {
        /* Set it to permanent */
        ObjectHeader->Flags |= OB_FLAG_PERMANENT;

        /* Release the lock */
        ObpReleaseObjectLock(ObjectHeader);
    }
    else
    {
        /* Remove the flag */
        ObjectHeader->Flags &= ~OB_FLAG_PERMANENT;

        /* Release the lock */
        ObpReleaseObjectLock(ObjectHeader);

        /* Check if we should delete the object now */
        ObpDeleteNameCheck(ObjectBody);
    }
}

PWCHAR
NTAPI
ObpAllocateObjectNameBuffer(IN ULONG Length,
                            IN BOOLEAN UseLookaside,
                            IN OUT PUNICODE_STRING ObjectName)
{
    ULONG MaximumLength;
    PVOID Buffer;

    /* Set the maximum length to the length plus the terminator */
    MaximumLength = Length + sizeof(UNICODE_NULL);

    /* Check if we should use the lookaside buffer */
    if (!(UseLookaside) || (MaximumLength > OBP_NAME_LOOKASIDE_MAX_SIZE))
    {
        /* Nope, allocate directly from pool */
        /* Since we later use MaximumLength to detect that we're not allocating
         * from a list, we need at least MaximumLength + sizeof(UNICODE_NULL)
         * here.
         *
         * People do call this with UseLookasideList FALSE so the distinction
         * is critical.
         */
        if (MaximumLength <= OBP_NAME_LOOKASIDE_MAX_SIZE)
        {
            MaximumLength = OBP_NAME_LOOKASIDE_MAX_SIZE + sizeof(UNICODE_NULL);
        }
        Buffer = ExAllocatePoolWithTag(PagedPool,
                                       MaximumLength,
                                       OB_NAME_TAG);
    }
    else
    {
        /* Allocate from the lookaside */
        MaximumLength = OBP_NAME_LOOKASIDE_MAX_SIZE;
        Buffer = ObpAllocateObjectCreateInfoBuffer(LookasideNameBufferList);
    }

    /* Setup the string */
    ObjectName->MaximumLength = (USHORT)MaximumLength;
    ObjectName->Length = (USHORT)Length;
    ObjectName->Buffer = Buffer;
    return Buffer;
}

VOID
NTAPI
ObpFreeObjectNameBuffer(IN PUNICODE_STRING Name)
{
    PVOID Buffer = Name->Buffer;

    /* We know this is a pool-allocation if the size doesn't match */
    if (Name->MaximumLength != OBP_NAME_LOOKASIDE_MAX_SIZE)
    {
        /*
         * Free it from the pool.
         *
         * We cannot use here ExFreePoolWithTag(..., OB_NAME_TAG); , because
         * the object name may have been massaged during operation by different
         * object parse routines. If the latter ones have to resolve a symbolic
         * link (e.g. as is done by CmpParseKey() and CmpGetSymbolicLink()),
         * the original object name is freed and re-allocated from the pool,
         * possibly with a different pool tag. At the end of the day, the new
         * object name can be reallocated and completely different, but we
         * should still be able to free it!
         */
        ExFreePool(Buffer);
    }
    else
    {
        /* Otherwise, free from the lookaside */
        ObpFreeCapturedAttributes(Buffer, LookasideNameBufferList);
    }
}

NTSTATUS
NTAPI
ObpCaptureObjectName(IN OUT PUNICODE_STRING CapturedName,
                     IN PUNICODE_STRING ObjectName,
                     IN KPROCESSOR_MODE AccessMode,
                     IN BOOLEAN UseLookaside)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG StringLength;
    PWCHAR _SEH2_VOLATILE StringBuffer = NULL;
    UNICODE_STRING LocalName;
    PAGED_CODE();

    /* Initialize the Input String */
    RtlInitEmptyUnicodeString(CapturedName, NULL, 0);

    /* Protect everything */
    _SEH2_TRY
    {
        /* Check if we came from user mode */
        if (AccessMode != KernelMode)
        {
            /* First Probe the String */
            LocalName = ProbeForReadUnicodeString(ObjectName);
            ProbeForRead(LocalName.Buffer, LocalName.Length, sizeof(WCHAR));
        }
        else
        {
            /* No probing needed */
            LocalName = *ObjectName;
        }

        /* Make sure there really is a string */
        StringLength = LocalName.Length;
        if (StringLength)
        {
            /* Check that the size is a valid WCHAR multiple */
            if ((StringLength & (sizeof(WCHAR) - 1)) ||
                /* Check that the NULL-termination below will work */
                (StringLength == (MAXUSHORT - sizeof(UNICODE_NULL) + 1)))
            {
                /* PS: Please keep the checks above expanded for clarity */
                Status = STATUS_OBJECT_NAME_INVALID;
            }
            else
            {
                /* Allocate the string buffer */
                StringBuffer = ObpAllocateObjectNameBuffer(StringLength,
                                                           UseLookaside,
                                                           CapturedName);
                if (!StringBuffer)
                {
                    /* Set failure code */
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
                else
                {
                    /* Copy the name */
                    RtlCopyMemory(StringBuffer, LocalName.Buffer, StringLength);
                    StringBuffer[StringLength / sizeof(WCHAR)] = UNICODE_NULL;
                }
            }
        }
    }
    _SEH2_EXCEPT(ExSystemExceptionFilter())
    {
        /* Handle exception and free the string buffer */
        Status = _SEH2_GetExceptionCode();
        if (StringBuffer)
        {
            ObpFreeObjectNameBuffer(CapturedName);
        }
    }
    _SEH2_END;

    /* Return */
    return Status;
}

NTSTATUS
NTAPI
ObpCaptureObjectCreateInformation(IN POBJECT_ATTRIBUTES ObjectAttributes,
                                  IN KPROCESSOR_MODE AccessMode,
                                  IN KPROCESSOR_MODE CreatorMode,
                                  IN BOOLEAN AllocateFromLookaside,
                                  IN POBJECT_CREATE_INFORMATION ObjectCreateInfo,
                                  OUT PUNICODE_STRING ObjectName)
{
    ULONG SdCharge, QuotaInfoSize;
    NTSTATUS Status = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PSECURITY_QUALITY_OF_SERVICE SecurityQos;
    PUNICODE_STRING LocalObjectName = NULL;
    PAGED_CODE();

    /* Zero out the Capture Data */
    RtlZeroMemory(ObjectCreateInfo, sizeof(OBJECT_CREATE_INFORMATION));

    /* SEH everything here for protection */
    _SEH2_TRY
    {
        /* Check if we got attributes */
        if (ObjectAttributes)
        {
            /* Check if we're in user mode */
            if (AccessMode != KernelMode)
            {
                /* Probe the attributes */
                ProbeForRead(ObjectAttributes,
                             sizeof(OBJECT_ATTRIBUTES),
                             sizeof(ULONG));
            }

            /* Validate the Size and Attributes */
            if ((ObjectAttributes->Length != sizeof(OBJECT_ATTRIBUTES)) ||
                (ObjectAttributes->Attributes & ~OBJ_VALID_KERNEL_ATTRIBUTES))
            {
                /* Invalid combination, fail */
                _SEH2_YIELD(return STATUS_INVALID_PARAMETER);
            }

            /* Set some Create Info and do not allow user-mode kernel handles */
            ObjectCreateInfo->RootDirectory = ObjectAttributes->RootDirectory;
            ObjectCreateInfo->Attributes = ObjectAttributes->Attributes & OBJ_VALID_KERNEL_ATTRIBUTES;
            if (CreatorMode != KernelMode) ObjectCreateInfo->Attributes &= ~OBJ_KERNEL_HANDLE;
            LocalObjectName = ObjectAttributes->ObjectName;
            SecurityDescriptor = ObjectAttributes->SecurityDescriptor;
            SecurityQos = ObjectAttributes->SecurityQualityOfService;

            /* Check if we have a security descriptor */
            if (SecurityDescriptor)
            {
                /* Capture it. Note: This has an implicit memory barrier due
                   to the function call, so cleanup is safe here.) */
                Status = SeCaptureSecurityDescriptor(SecurityDescriptor,
                                                     AccessMode,
                                                     NonPagedPool,
                                                     TRUE,
                                                     &ObjectCreateInfo->
                                                     SecurityDescriptor);
                if (!NT_SUCCESS(Status))
                {
                    /* Capture failed, quit */
                    ObjectCreateInfo->SecurityDescriptor = NULL;
                    _SEH2_YIELD(return Status);
                }

                /*
                 * By default, assume a SD size of 1024 and allow twice its
                 * size.
                 * If SD size happen to be bigger than that, then allow it
                 */
                SdCharge = 2048;
                SeComputeQuotaInformationSize(ObjectCreateInfo->SecurityDescriptor,
                                              &QuotaInfoSize);
                if ((2 * QuotaInfoSize) > 2048)
                {
                    SdCharge = 2 * QuotaInfoSize;
                }

                /* Save the probe mode and security descriptor size */
                ObjectCreateInfo->SecurityDescriptorCharge = SdCharge;
                ObjectCreateInfo->ProbeMode = AccessMode;
            }

            /* Check if we have QoS */
            if (SecurityQos)
            {
                /* Check if we came from user mode */
                if (AccessMode != KernelMode)
                {
                    /* Validate the QoS */
                    ProbeForRead(SecurityQos,
                                 sizeof(SECURITY_QUALITY_OF_SERVICE),
                                 sizeof(ULONG));
                }

                /* Save Info */
                ObjectCreateInfo->SecurityQualityOfService = *SecurityQos;
                ObjectCreateInfo->SecurityQos =
                    &ObjectCreateInfo->SecurityQualityOfService;
            }
        }
        else
        {
            /* We don't have a name */
            LocalObjectName = NULL;
        }
    }
    _SEH2_EXCEPT(ExSystemExceptionFilter())
    {
        /* Cleanup and return the exception code */
        ObpReleaseObjectCreateInformation(ObjectCreateInfo);
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Now check if the Object Attributes had an Object Name */
    if (LocalObjectName)
    {
        Status = ObpCaptureObjectName(ObjectName,
                                      LocalObjectName,
                                      AccessMode,
                                      AllocateFromLookaside);
    }
    else
    {
        /* Clear the string */
        RtlInitEmptyUnicodeString(ObjectName, NULL, 0);

        /* It cannot have specified a Root Directory */
        if (ObjectCreateInfo->RootDirectory)
        {
            Status = STATUS_OBJECT_NAME_INVALID;
        }
    }

    /* Cleanup if we failed */
    if (!NT_SUCCESS(Status))
    {
        ObpReleaseObjectCreateInformation(ObjectCreateInfo);
    }

    /* Return status to caller */
    return Status;
}

VOID
NTAPI
ObFreeObjectCreateInfoBuffer(IN POBJECT_CREATE_INFORMATION ObjectCreateInfo)
{
    /* Call the macro. We use this function to isolate Ob internals from Io */
    ObpFreeCapturedAttributes(ObjectCreateInfo, LookasideCreateInfoList);
}

NTSTATUS
NTAPI
ObpAllocateObject(IN POBJECT_CREATE_INFORMATION ObjectCreateInfo,
                  IN PUNICODE_STRING ObjectName,
                  IN POBJECT_TYPE ObjectType,
                  IN ULONG ObjectSize,
                  IN KPROCESSOR_MODE PreviousMode,
                  IN POBJECT_HEADER *ObjectHeader)
{
    POBJECT_HEADER Header;
    ULONG QuotaSize, HandleSize, NameSize, CreatorSize;
    POBJECT_HEADER_HANDLE_INFO HandleInfo;
    POBJECT_HEADER_NAME_INFO NameInfo;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;
    POBJECT_HEADER_QUOTA_INFO QuotaInfo;
    POOL_TYPE PoolType;
    ULONG FinalSize;
    ULONG Tag;
    PAGED_CODE();

    /* Accounting */
    ObpObjectsCreated++;

    /* Check if we don't have an Object Type yet */
    if (!ObjectType)
    {
        /* Use default tag and non-paged pool */
        PoolType = NonPagedPool;
        Tag = TAG_OBJECT_TYPE;
    }
    else
    {
        /* Use the pool and tag given */
        PoolType = ObjectType->TypeInfo.PoolType;
        Tag = ObjectType->Key;
    }

    /* Check if we have no create information (ie: we're an object type) */
    if (!ObjectCreateInfo)
    {
        /* Use defaults */
        QuotaSize = HandleSize = 0;
        NameSize = sizeof(OBJECT_HEADER_NAME_INFO);
        CreatorSize = sizeof(OBJECT_HEADER_CREATOR_INFO);
    }
    else
    {
        /* Check if we have quota */
        if ((((ObjectCreateInfo->PagedPoolCharge !=
               ObjectType->TypeInfo.DefaultPagedPoolCharge) ||
              (ObjectCreateInfo->NonPagedPoolCharge !=
               ObjectType->TypeInfo.DefaultNonPagedPoolCharge) ||
              (ObjectCreateInfo->SecurityDescriptorCharge > 2048)) &&
             (PsGetCurrentProcess() != PsInitialSystemProcess)) ||
            (ObjectCreateInfo->Attributes & OBJ_EXCLUSIVE))
        {
            /* Set quota size */
            QuotaSize = sizeof(OBJECT_HEADER_QUOTA_INFO);
            ObpObjectsWithPoolQuota++;
        }
        else
        {
            /* No Quota */
            QuotaSize = 0;
        }

        /* Check if we have a handle database */
        if (ObjectType->TypeInfo.MaintainHandleCount)
        {
            /* Set handle database size */
            HandleSize = sizeof(OBJECT_HEADER_HANDLE_INFO);
            ObpObjectsWithHandleDB++;
        }
        else
        {
            /* None */
            HandleSize = 0;
        }

        /* Check if the Object has a name */
        if (ObjectName->Buffer)
        {
            /* Set name size */
            NameSize = sizeof(OBJECT_HEADER_NAME_INFO);
            ObpObjectsWithName++;
        }
        else
        {
            /* No name */
            NameSize = 0;
        }

        /* Check if the Object maintains type lists */
        if (ObjectType->TypeInfo.MaintainTypeList)
        {
            /* Set owner/creator size */
            CreatorSize = sizeof(OBJECT_HEADER_CREATOR_INFO);
            ObpObjectsWithCreatorInfo++;
        }
        else
        {
            /* No info */
            CreatorSize = 0;
        }
    }

    /* Set final header size */
    FinalSize = QuotaSize +
                HandleSize +
                NameSize +
                CreatorSize +
                FIELD_OFFSET(OBJECT_HEADER, Body);

    /* Allocate memory for the Object and Header */
    Header = ExAllocatePoolWithTag(PoolType, FinalSize + ObjectSize, Tag);
    if (!Header) return STATUS_INSUFFICIENT_RESOURCES;

    /* Check if we have a quota header */
    if (QuotaSize)
    {
        /* Initialize quota info */
        QuotaInfo = (POBJECT_HEADER_QUOTA_INFO)Header;
        QuotaInfo->PagedPoolCharge = ObjectCreateInfo->PagedPoolCharge;
        QuotaInfo->NonPagedPoolCharge = ObjectCreateInfo->NonPagedPoolCharge;
        QuotaInfo->SecurityDescriptorCharge = ObjectCreateInfo->SecurityDescriptorCharge;
        QuotaInfo->ExclusiveProcess = NULL;
        Header = (POBJECT_HEADER)(QuotaInfo + 1);
    }

    /* Check if we have a handle database header */
    if (HandleSize)
    {
        /* Initialize Handle Info */
        HandleInfo = (POBJECT_HEADER_HANDLE_INFO)Header;
        HandleInfo->SingleEntry.HandleCount = 0;
        Header = (POBJECT_HEADER)(HandleInfo + 1);
    }

    /* Check if we have a name header */
    if (NameSize)
    {
        /* Initialize the Object Name Info */
        NameInfo = (POBJECT_HEADER_NAME_INFO)Header;
        NameInfo->Name = *ObjectName;
        NameInfo->Directory = NULL;
        NameInfo->QueryReferences = 1;

        /* Check if this is a call with the special protection flag */
        if ((PreviousMode == KernelMode) &&
            (ObjectCreateInfo) &&
            (ObjectCreateInfo->Attributes & OBJ_KERNEL_EXCLUSIVE))
        {
            /* Set flag which will make the object protected from user-mode */
            NameInfo->QueryReferences |= OB_FLAG_KERNEL_EXCLUSIVE;
        }

        /* Set the header pointer */
        Header = (POBJECT_HEADER)(NameInfo + 1);
    }

    /* Check if we have a creator header */
    if (CreatorSize)
    {
        /* Initialize Creator Info */
        CreatorInfo = (POBJECT_HEADER_CREATOR_INFO)Header;
        CreatorInfo->CreatorBackTraceIndex = 0;
        CreatorInfo->CreatorUniqueProcess = PsGetCurrentProcessId();
        InitializeListHead(&CreatorInfo->TypeList);
        Header = (POBJECT_HEADER)(CreatorInfo + 1);
    }

    /* Check for quota information */
    if (QuotaSize)
    {
        /* Set the offset */
        Header->QuotaInfoOffset = (UCHAR)(QuotaSize +
                                          HandleSize +
                                          NameSize +
                                          CreatorSize);
    }
    else
    {
        /* No offset */
        Header->QuotaInfoOffset = 0;
    }

    /* Check for handle information */
    if (HandleSize)
    {
        /* Set the offset */
        Header->HandleInfoOffset = (UCHAR)(HandleSize +
                                           NameSize +
                                           CreatorSize);
    }
    else
    {
        /* No offset */
        Header->HandleInfoOffset = 0;
    }

    /* Check for name information */
    if (NameSize)
    {
        /* Set the offset */
        Header->NameInfoOffset = (UCHAR)(NameSize + CreatorSize);
    }
    else
    {
        /* No Name */
        Header->NameInfoOffset = 0;
    }

    /* Set the new object flag */
    Header->Flags = OB_FLAG_CREATE_INFO;

    /* Remember if we have creator info */
    if (CreatorSize) Header->Flags |= OB_FLAG_CREATOR_INFO;

    /* Remember if we have handle info */
    if (HandleSize) Header->Flags |= OB_FLAG_SINGLE_PROCESS;

    /* Initialize the object header */
    Header->PointerCount = 1;
    Header->HandleCount = 0;
    Header->Type = ObjectType;
    Header->ObjectCreateInfo = ObjectCreateInfo;
    Header->SecurityDescriptor = NULL;

    /* Check if this is a permanent object */
    if ((ObjectCreateInfo) && (ObjectCreateInfo->Attributes & OBJ_PERMANENT))
    {
        /* Set the needed flag so we can check */
        Header->Flags |= OB_FLAG_PERMANENT;
    }

    /* Check if this is an exclusive object */
    if ((ObjectCreateInfo) && (ObjectCreateInfo->Attributes & OBJ_EXCLUSIVE))
    {
        /* Set the needed flag so we can check */
        Header->Flags |= OB_FLAG_EXCLUSIVE;
    }

    /* Set kernel-mode flag */
    if (PreviousMode == KernelMode) Header->Flags |= OB_FLAG_KERNEL_MODE;

    /* Check if we have a type */
    if (ObjectType)
    {
        /* Increase the number of objects of this type */
        InterlockedIncrement((PLONG)&ObjectType->TotalNumberOfObjects);

        /* Update the high water */
        ObjectType->HighWaterNumberOfObjects = max(ObjectType->
                                                   TotalNumberOfObjects,
                                                   ObjectType->
                                                   HighWaterNumberOfObjects);
    }

    /* Return Header */
    *ObjectHeader = Header;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ObQueryTypeInfo(IN POBJECT_TYPE ObjectType,
                OUT POBJECT_TYPE_INFORMATION ObjectTypeInfo,
                IN ULONG Length,
                OUT PULONG ReturnLength)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PWSTR InfoBuffer;

    /* Enter SEH */
    _SEH2_TRY
    {
        /* Set return length aligned to 4-byte boundary */
        *ReturnLength += sizeof(*ObjectTypeInfo) +
                         ALIGN_UP(ObjectType->Name.MaximumLength, ULONG);

        /* Check if thats too much though. */
        if (Length < *ReturnLength)
        {
            _SEH2_YIELD(return STATUS_INFO_LENGTH_MISMATCH);
        }

        /* Build the data */
        ObjectTypeInfo->TotalNumberOfHandles =
            ObjectType->TotalNumberOfHandles;
        ObjectTypeInfo->TotalNumberOfObjects =
            ObjectType->TotalNumberOfObjects;
        ObjectTypeInfo->HighWaterNumberOfHandles =
            ObjectType->HighWaterNumberOfHandles;
        ObjectTypeInfo->HighWaterNumberOfObjects =
            ObjectType->HighWaterNumberOfObjects;
        ObjectTypeInfo->PoolType =
            ObjectType->TypeInfo.PoolType;
        ObjectTypeInfo->DefaultNonPagedPoolCharge =
            ObjectType->TypeInfo.DefaultNonPagedPoolCharge;
        ObjectTypeInfo->DefaultPagedPoolCharge =
            ObjectType->TypeInfo.DefaultPagedPoolCharge;
        ObjectTypeInfo->ValidAccessMask =
            ObjectType->TypeInfo.ValidAccessMask;
        ObjectTypeInfo->SecurityRequired =
            ObjectType->TypeInfo.SecurityRequired;
        ObjectTypeInfo->InvalidAttributes =
            ObjectType->TypeInfo.InvalidAttributes;
        ObjectTypeInfo->GenericMapping =
            ObjectType->TypeInfo.GenericMapping;
        ObjectTypeInfo->MaintainHandleCount =
            ObjectType->TypeInfo.MaintainHandleCount;

        /* Setup the name buffer */
        InfoBuffer = (PWSTR)(ObjectTypeInfo + 1);
        ObjectTypeInfo->TypeName.Buffer = InfoBuffer;
        ObjectTypeInfo->TypeName.MaximumLength = ObjectType->Name.MaximumLength;
        ObjectTypeInfo->TypeName.Length = ObjectType->Name.Length;

        /* Copy it */
        RtlCopyMemory(InfoBuffer,
                      ObjectType->Name.Buffer,
                      ObjectType->Name.Length);

        /* Null-terminate it */
        (InfoBuffer)[ObjectType->Name.Length / sizeof(WCHAR)] = UNICODE_NULL;
    }
    _SEH2_EXCEPT(ExSystemExceptionFilter())
    {
        /* Otherwise, get the exception code */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* Return status to caller */
    return Status;
}


/* PUBLIC FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
ObCreateObject(IN KPROCESSOR_MODE ProbeMode OPTIONAL,
               IN POBJECT_TYPE Type,
               IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
               IN KPROCESSOR_MODE AccessMode,
               IN OUT PVOID ParseContext OPTIONAL,
               IN ULONG ObjectSize,
               IN ULONG PagedPoolCharge OPTIONAL,
               IN ULONG NonPagedPoolCharge OPTIONAL,
               OUT PVOID *Object)
{
    NTSTATUS Status;
    POBJECT_CREATE_INFORMATION ObjectCreateInfo;
    UNICODE_STRING ObjectName;
    POBJECT_HEADER Header;

    /* Allocate a capture buffer */
    ObjectCreateInfo = ObpAllocateObjectCreateInfoBuffer(LookasideCreateInfoList);
    if (!ObjectCreateInfo) return STATUS_INSUFFICIENT_RESOURCES;

    /* Capture all the info */
    Status = ObpCaptureObjectCreateInformation(ObjectAttributes,
                                               ProbeMode,
                                               AccessMode,
                                               FALSE,
                                               ObjectCreateInfo,
                                               &ObjectName);
    if (NT_SUCCESS(Status))
    {
        /* Validate attributes */
        if (Type->TypeInfo.InvalidAttributes & ObjectCreateInfo->Attributes)
        {
            /* Fail */
            Status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            /* Check if we have a paged charge */
            if (!PagedPoolCharge)
            {
                /* Save it */
                PagedPoolCharge = Type->TypeInfo.DefaultPagedPoolCharge;
            }

            /* Check for nonpaged charge */
            if (!NonPagedPoolCharge)
            {
                /* Save it */
                NonPagedPoolCharge = Type->TypeInfo.DefaultNonPagedPoolCharge;
            }

            /* Write the pool charges */
            ObjectCreateInfo->PagedPoolCharge = PagedPoolCharge;
            ObjectCreateInfo->NonPagedPoolCharge = NonPagedPoolCharge;

            /* Allocate the Object */
            Status = ObpAllocateObject(ObjectCreateInfo,
                                       &ObjectName,
                                       Type,
                                       ObjectSize,
                                       AccessMode,
                                       &Header);
            if (NT_SUCCESS(Status))
            {
                /* Return the Object */
                *Object = &Header->Body;

                /* Check if this is a permanent object */
                if (Header->Flags & OB_FLAG_PERMANENT)
                {
                    /* Do the privilege check */
                    if (!SeSinglePrivilegeCheck(SeCreatePermanentPrivilege,
                                                ProbeMode))
                    {
                        /* Fail */
                        ObpDeallocateObject(*Object);
                        Status = STATUS_PRIVILEGE_NOT_HELD;
                    }
                }

                /* Return status */
                return Status;
            }
        }

        /* Release the Capture Info, we don't need it */
        ObpFreeObjectCreateInformation(ObjectCreateInfo);
        if (ObjectName.Buffer) ObpFreeObjectNameBuffer(&ObjectName);
    }

    /* We failed, so release the Buffer */
    ObpFreeCapturedAttributes(ObjectCreateInfo, LookasideCreateInfoList);
    return Status;
}

NTSTATUS
NTAPI
ObCreateObjectType(IN PUNICODE_STRING TypeName,
                   IN POBJECT_TYPE_INITIALIZER ObjectTypeInitializer,
                   IN PVOID Reserved,
                   OUT POBJECT_TYPE *ObjectType)
{
    POBJECT_HEADER Header;
    POBJECT_TYPE LocalObjectType;
    ULONG HeaderSize;
    NTSTATUS Status;
    OBP_LOOKUP_CONTEXT Context;
    PWCHAR p;
    ULONG i;
    UNICODE_STRING ObjectName;
    ANSI_STRING AnsiName;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;

    /* Verify parameters */
    if (!(TypeName) ||
        !(TypeName->Length) ||
        (TypeName->Length % sizeof(WCHAR)) ||
        !(ObjectTypeInitializer) ||
        (ObjectTypeInitializer->Length != sizeof(*ObjectTypeInitializer)) ||
        (ObjectTypeInitializer->InvalidAttributes & ~OBJ_VALID_KERNEL_ATTRIBUTES) ||
        (ObjectTypeInitializer->MaintainHandleCount &&
         (!(ObjectTypeInitializer->OpenProcedure) &&
          !ObjectTypeInitializer->CloseProcedure)) ||
        ((!ObjectTypeInitializer->UseDefaultObject) &&
         (ObjectTypeInitializer->PoolType != NonPagedPool)))
    {
        /* Fail */
        return STATUS_INVALID_PARAMETER;
    }

    /* Make sure the name doesn't have a separator */
    p = TypeName->Buffer;
    i = TypeName->Length / sizeof(WCHAR);
    while (i--)
    {
        /* Check for one and fail */
        if (*p++ == OBJ_NAME_PATH_SEPARATOR) return STATUS_OBJECT_NAME_INVALID;
    }

    /* Setup a lookup context */
    ObpInitializeLookupContext(&Context);

    /* Check if we've already created the directory of types */
    if (ObpTypeDirectoryObject)
    {
        /* Lock the lookup context */
        ObpAcquireLookupContextLock(&Context, ObpTypeDirectoryObject);

        /* Do the lookup */
        if (ObpLookupEntryDirectory(ObpTypeDirectoryObject,
                                    TypeName,
                                    OBJ_CASE_INSENSITIVE,
                                    FALSE,
                                    &Context))
        {
            /* We have already created it, so fail */
            ObpReleaseLookupContext(&Context);
            return STATUS_OBJECT_NAME_COLLISION;
        }
    }

    /* Now make a copy of the object name */
    ObjectName.Buffer = ExAllocatePoolWithTag(PagedPool,
                                              TypeName->MaximumLength,
                                              OB_NAME_TAG);
    if (!ObjectName.Buffer)
    {
        /* Out of memory, fail */
        ObpReleaseLookupContext(&Context);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Set the length and copy the name */
    ObjectName.MaximumLength = TypeName->MaximumLength;
    RtlCopyUnicodeString(&ObjectName, TypeName);

    /* Allocate the Object */
    Status = ObpAllocateObject(NULL,
                               &ObjectName,
                               ObpTypeObjectType,
                               sizeof(OBJECT_TYPE),
                               KernelMode,
                               &Header);
    if (!NT_SUCCESS(Status))
    {
        /* Free the name and fail */
        ObpReleaseLookupContext(&Context);
        ExFreePool(ObjectName.Buffer);
        return Status;
    }

    /* Setup the flags and name */
    LocalObjectType = (POBJECT_TYPE)&Header->Body;
    LocalObjectType->Name = ObjectName;
    Header->Flags |= OB_FLAG_KERNEL_MODE | OB_FLAG_PERMANENT;

    /* Clear accounting data */
    LocalObjectType->TotalNumberOfObjects =
    LocalObjectType->TotalNumberOfHandles =
    LocalObjectType->HighWaterNumberOfObjects =
    LocalObjectType->HighWaterNumberOfHandles = 0;

    /* Check if this is the first Object Type */
    if (!ObpTypeObjectType)
    {
        /* It is, so set this as the type object */
        ObpTypeObjectType = LocalObjectType;
        Header->Type = ObpTypeObjectType;

        /* Set the hard-coded key and object count */
        LocalObjectType->TotalNumberOfObjects = 1;
        LocalObjectType->Key = TAG_OBJECT_TYPE;
    }
    else
    {
        /* Convert the tag to ASCII */
        Status = RtlUnicodeStringToAnsiString(&AnsiName, TypeName, TRUE);
        if (NT_SUCCESS(Status))
        {
            /* For every missing character, use a space */
            for (i = 3; i >= AnsiName.Length; i--) AnsiName.Buffer[i] = ' ';

            /* Set the key and free the converted name */
            LocalObjectType->Key = *(PULONG)AnsiName.Buffer;
            RtlFreeAnsiString(&AnsiName);
        }
        else
        {
            /* Just copy the characters */
            LocalObjectType->Key = *(PULONG)TypeName->Buffer;
        }
    }

    /* Set up the type information */
    LocalObjectType->TypeInfo = *ObjectTypeInitializer;
    LocalObjectType->TypeInfo.PoolType = ObjectTypeInitializer->PoolType;

    /* Check if we have to maintain a type list */
    if (NtGlobalFlag & FLG_MAINTAIN_OBJECT_TYPELIST)
    {
        /* Enable support */
        LocalObjectType->TypeInfo.MaintainTypeList = TRUE;
    }

    /* Calculate how much space our header'll take up */
    HeaderSize = sizeof(OBJECT_HEADER) +
                 sizeof(OBJECT_HEADER_NAME_INFO) +
                 (ObjectTypeInitializer->MaintainHandleCount ?
                  sizeof(OBJECT_HEADER_HANDLE_INFO) : 0);

    /* Check the pool type */
    if (ObjectTypeInitializer->PoolType == NonPagedPool)
    {
        /* Update the NonPaged Pool charge */
        LocalObjectType->TypeInfo.DefaultNonPagedPoolCharge += HeaderSize;
    }
    else
    {
        /* Update the Paged Pool charge */
        LocalObjectType->TypeInfo.DefaultPagedPoolCharge += HeaderSize;
    }

    /* All objects types need a security procedure */
    if (!ObjectTypeInitializer->SecurityProcedure)
    {
        LocalObjectType->TypeInfo.SecurityProcedure = SeDefaultObjectMethod;
    }

    /* Select the Wait Object */
    if (LocalObjectType->TypeInfo.UseDefaultObject)
    {
        /* Add the SYNCHRONIZE access mask since it's waitable */
        LocalObjectType->TypeInfo.ValidAccessMask |= SYNCHRONIZE;

        /* Use the "Default Object", a simple event */
        LocalObjectType->DefaultObject = &ObpDefaultObject;
    }
    /* The File Object gets an optimized hack so it can be waited on */
    else if ((TypeName->Length == 8) && !(wcscmp(TypeName->Buffer, L"File")))
    {
        /* Wait on the File Object's event directly */
        LocalObjectType->DefaultObject = UlongToPtr(FIELD_OFFSET(FILE_OBJECT,
                                                                 Event));
    }
    else if ((TypeName->Length == 24) && !(wcscmp(TypeName->Buffer, L"WaitablePort")))
    {
        /* Wait on the LPC Port's object directly */
        LocalObjectType->DefaultObject = UlongToPtr(FIELD_OFFSET(LPCP_PORT_OBJECT,
                                                                 WaitEvent));
    }
    else
    {
        /* No default Object */
        LocalObjectType->DefaultObject = NULL;
    }

    /* Initialize Object Type components */
    ExInitializeResourceLite(&LocalObjectType->Mutex);
    for (i = 0; i < 4; i++)
    {
        /* Initialize the object locks */
        ExInitializeResourceLite(&LocalObjectType->ObjectLocks[i]);
    }
    InitializeListHead(&LocalObjectType->TypeList);

    /* Lock the object type */
    ObpEnterObjectTypeMutex(ObpTypeObjectType);

    /* Get creator info and insert it into the type list */
    CreatorInfo = OBJECT_HEADER_TO_CREATOR_INFO(Header);
    if (CreatorInfo)
    {
        InsertTailList(&ObpTypeObjectType->TypeList,
                       &CreatorInfo->TypeList);

        /* CORE-8423: Avoid inserting this a second time if someone creates a
         * handle to the object type (bug in Windows 2003) */
        Header->Flags &= ~OB_FLAG_CREATE_INFO;
    }

    /* Set the index and the entry into the object type array */
    LocalObjectType->Index = ObpTypeObjectType->TotalNumberOfObjects;

    ASSERT(LocalObjectType->Index != 0);

    if (LocalObjectType->Index < RTL_NUMBER_OF(ObpObjectTypes))
    {
        /* It fits, insert it */
        ObpObjectTypes[LocalObjectType->Index - 1] = LocalObjectType;
    }

    /* Release the object type */
    ObpLeaveObjectTypeMutex(ObpTypeObjectType);

    /* Check if we're actually creating the directory object itself */
    if (!(ObpTypeDirectoryObject) ||
        (ObpInsertEntryDirectory(ObpTypeDirectoryObject, &Context, Header)))
    {
        /* Check if the type directory exists */
        if (ObpTypeDirectoryObject)
        {
            /* Reference it */
            ObReferenceObject(ObpTypeDirectoryObject);
        }

        /* Cleanup the lookup context */
        ObpReleaseLookupContext(&Context);

        /* Return the object type and success */
        *ObjectType = LocalObjectType;
        return STATUS_SUCCESS;
    }

    /* If we got here, then we failed */
    ObpReleaseLookupContext(&Context);
    return STATUS_INSUFFICIENT_RESOURCES;
}

VOID
NTAPI
ObDeleteCapturedInsertInfo(IN PVOID Object)
{
    POBJECT_HEADER ObjectHeader;
    PAGED_CODE();

    /* Check if there is anything to free */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
    if ((ObjectHeader->Flags & OB_FLAG_CREATE_INFO) &&
        (ObjectHeader->ObjectCreateInfo != NULL))
    {
        /* Free the create info */
        ObpFreeObjectCreateInformation(ObjectHeader->ObjectCreateInfo);
        ObjectHeader->ObjectCreateInfo = NULL;
    }
}

VOID
NTAPI
ObpDeleteObjectType(IN PVOID Object)
{
    ULONG i;
    POBJECT_TYPE ObjectType = (PVOID)Object;

    /* Loop our locks */
    for (i = 0; i < 4; i++)
    {
        /* Delete each one */
        ExDeleteResourceLite(&ObjectType->ObjectLocks[i]);
    }

    /* Delete our main mutex */
    ExDeleteResourceLite(&ObjectType->Mutex);
}

/*++
* @name ObMakeTemporaryObject
* @implemented NT4
*
*     The ObMakeTemporaryObject routine <FILLMEIN>
*
* @param ObjectBody
*        <FILLMEIN>
*
* @return None.
*
* @remarks None.
*
*--*/
VOID
NTAPI
ObMakeTemporaryObject(IN PVOID ObjectBody)
{
    PAGED_CODE();

    /* Call the internal API */
    ObpSetPermanentObject(ObjectBody, FALSE);
}

/*++
* @name NtMakeTemporaryObject
* @implemented NT4
*
*     The NtMakeTemporaryObject routine <FILLMEIN>
*
* @param ObjectHandle
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtMakeTemporaryObject(IN HANDLE ObjectHandle)
{
    PVOID ObjectBody;
    NTSTATUS Status;
    PAGED_CODE();

    /* Reference the object for DELETE access */
    Status = ObReferenceObjectByHandle(ObjectHandle,
                                       DELETE,
                                       NULL,
                                       KeGetPreviousMode(),
                                       &ObjectBody,
                                       NULL);
    if (Status != STATUS_SUCCESS) return Status;

    /* Set it as temporary and dereference it */
    ObpSetPermanentObject(ObjectBody, FALSE);
    ObDereferenceObject(ObjectBody);
    return STATUS_SUCCESS;
}

/*++
* @name NtMakePermanentObject
* @implemented NT4
*
*     The NtMakePermanentObject routine <FILLMEIN>
*
* @param ObjectHandle
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtMakePermanentObject(IN HANDLE ObjectHandle)
{
    PVOID ObjectBody;
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PAGED_CODE();

    /* Make sure that the caller has SeCreatePermanentPrivilege */
    if (!SeSinglePrivilegeCheck(SeCreatePermanentPrivilege, PreviousMode))
    {
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    /* Reference the object */
    Status = ObReferenceObjectByHandle(ObjectHandle,
                                       0,
                                       NULL,
                                       PreviousMode,
                                       &ObjectBody,
                                       NULL);
    if (Status != STATUS_SUCCESS) return Status;

    /* Set it as permanent and dereference it */
    ObpSetPermanentObject(ObjectBody, TRUE);
    ObDereferenceObject(ObjectBody);
    return STATUS_SUCCESS;
}

/*++
* @name NtQueryObject
* @implemented NT4
*
*     The NtQueryObject routine <FILLMEIN>
*
* @param ObjectHandle
*        <FILLMEIN>
*
* @param ObjectInformationClass
*        <FILLMEIN>
*
* @param ObjectInformation
*        <FILLMEIN>
*
* @param Length
*        <FILLMEIN>
*
* @param ResultLength
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtQueryObject(IN HANDLE ObjectHandle,
              IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
              OUT PVOID ObjectInformation,
              IN ULONG Length,
              OUT PULONG ResultLength OPTIONAL)
{
    OBJECT_HANDLE_INFORMATION HandleInfo;
    POBJECT_HEADER ObjectHeader = NULL;
    POBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleFlags;
    POBJECT_BASIC_INFORMATION BasicInfo;
    ULONG InfoLength = 0;
    PVOID Object = NULL;
    NTSTATUS Status;
    POBJECT_HEADER_QUOTA_INFO ObjectQuota;
    SECURITY_INFORMATION SecurityInformation;
    POBJECT_TYPE ObjectType;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PAGED_CODE();

    /* Check if the caller is from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Protect validation with SEH */
        _SEH2_TRY
        {
            /* Probe the input structure */
            ProbeForWrite(ObjectInformation, Length, sizeof(UCHAR));

            /* If we have a result length, probe it too */
            if (ResultLength) ProbeForWriteUlong(ResultLength);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /*
     * Make sure this isn't a generic type query, since the caller doesn't
     * have to give a handle for it
     */
    if (ObjectInformationClass != ObjectTypesInformation)
    {
        /* Reference the object */
        Status = ObReferenceObjectByHandle(ObjectHandle,
                                           0,
                                           NULL,
                                           KeGetPreviousMode(),
                                           &Object,
                                           &HandleInfo);
        if (!NT_SUCCESS (Status)) return Status;

        /* Get the object header */
        ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
        ObjectType = ObjectHeader->Type;
    }

    _SEH2_TRY
    {
        /* Check the information class */
        switch (ObjectInformationClass)
        {
            /* Basic info */
            case ObjectBasicInformation:

                /* Validate length */
                InfoLength = sizeof(OBJECT_BASIC_INFORMATION);
                if (Length != sizeof(OBJECT_BASIC_INFORMATION))
                {
                    /* Fail */
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    break;
                }

                /* Fill out the basic information */
                BasicInfo = (POBJECT_BASIC_INFORMATION)ObjectInformation;
                BasicInfo->Attributes = HandleInfo.HandleAttributes;
                BasicInfo->GrantedAccess = HandleInfo.GrantedAccess;
                BasicInfo->HandleCount = ObjectHeader->HandleCount;
                BasicInfo->PointerCount = ObjectHeader->PointerCount;

                /* Permanent/Exclusive Flags are NOT in Handle attributes! */
                if (ObjectHeader->Flags & OB_FLAG_EXCLUSIVE)
                {
                    /* Set the flag */
                    BasicInfo->Attributes |= OBJ_EXCLUSIVE;
                }
                if (ObjectHeader->Flags & OB_FLAG_PERMANENT)
                {
                    /* Set the flag */
                    BasicInfo->Attributes |= OBJ_PERMANENT;
                }

                /* Copy quota information */
                ObjectQuota = OBJECT_HEADER_TO_QUOTA_INFO(ObjectHeader);
                if (ObjectQuota != NULL)
                {
                    BasicInfo->PagedPoolCharge = ObjectQuota->PagedPoolCharge;
                    BasicInfo->NonPagedPoolCharge = ObjectQuota->NonPagedPoolCharge;
                }
                else
                {
                    BasicInfo->PagedPoolCharge = 0;
                    BasicInfo->NonPagedPoolCharge = 0;
                }

                /* Copy name information */
                BasicInfo->NameInfoSize = 0; /* FIXME*/
                BasicInfo->TypeInfoSize = 0; /* FIXME*/

                /* Check if this is a symlink */
                if (ObjectHeader->Type == ObpSymbolicLinkObjectType)
                {
                    /* Return the creation time */
                    BasicInfo->CreationTime.QuadPart =
                        ((POBJECT_SYMBOLIC_LINK)Object)->CreationTime.QuadPart;
                }
                else
                {
                    /* Otherwise return 0 */
                    BasicInfo->CreationTime.QuadPart = (ULONGLONG)0;
                }

                /* Copy security information */
                BasicInfo->SecurityDescriptorSize = 0;
                if (BooleanFlagOn(HandleInfo.GrantedAccess, READ_CONTROL) &&
                    ObjectHeader->SecurityDescriptor != NULL)
                {
                    SecurityInformation = OWNER_SECURITY_INFORMATION |
                                          GROUP_SECURITY_INFORMATION |
                                          DACL_SECURITY_INFORMATION |
                                          SACL_SECURITY_INFORMATION;

                    ObjectType->TypeInfo.SecurityProcedure(Object,
                                                           QuerySecurityDescriptor,
                                                           &SecurityInformation,
                                                           NULL,
                                                           &BasicInfo->SecurityDescriptorSize,
                                                           &ObjectHeader->SecurityDescriptor,
                                                           ObjectType->TypeInfo.PoolType,
                                                           &ObjectType->TypeInfo.GenericMapping);
                }

                /* Break out with success */
                Status = STATUS_SUCCESS;
                break;

            /* Name information */
            case ObjectNameInformation:

                /* Call the helper and break out */
                Status = ObQueryNameString(Object,
                                           (POBJECT_NAME_INFORMATION)
                                           ObjectInformation,
                                           Length,
                                           &InfoLength);
                break;

            /* Information about this type */
            case ObjectTypeInformation:

                /* Call the helper and break out */
                Status = ObQueryTypeInfo(ObjectHeader->Type,
                                         (POBJECT_TYPE_INFORMATION)
                                         ObjectInformation,
                                         Length,
                                         &InfoLength);
                break;

            /* Information about all types */
            case ObjectTypesInformation:
                DPRINT1("NOT IMPLEMENTED!\n");
                InfoLength = Length;
                Status = STATUS_NOT_IMPLEMENTED;
                break;

            /* Information about the handle flags */
            case ObjectHandleFlagInformation:

                /* Validate length */
                InfoLength = sizeof (OBJECT_HANDLE_ATTRIBUTE_INFORMATION);
                if (Length != sizeof (OBJECT_HANDLE_ATTRIBUTE_INFORMATION))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    break;
                }

                /* Get the structure */
                HandleFlags = (POBJECT_HANDLE_ATTRIBUTE_INFORMATION)
                               ObjectInformation;

                /* Set the flags */
                HandleFlags->Inherit = HandleInfo.HandleAttributes & OBJ_INHERIT;
                HandleFlags->ProtectFromClose = (HandleInfo.HandleAttributes &
                                                 OBJ_PROTECT_CLOSE) != 0;

                /* Break out with success */
                Status = STATUS_SUCCESS;
                break;

            /* Anything else */
            default:

                /* Fail it */
                InfoLength = Length;
                Status = STATUS_INVALID_INFO_CLASS;
                break;
        }

        /* Check if the caller wanted the return length */
        if (ResultLength)
        {
            /* Write the length */
            *ResultLength = InfoLength;
        }
    }
    _SEH2_EXCEPT(ExSystemExceptionFilter())
    {
        /* Otherwise, get the exception code */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* Dereference the object if we had referenced it */
    if (Object) ObDereferenceObject(Object);

    /* Return status */
    return Status;
}

/*++
* @name NtSetInformationObject
* @implemented NT4
*
*     The NtSetInformationObject routine <FILLMEIN>
*
* @param ObjectHandle
*        <FILLMEIN>
*
* @param ObjectInformationClass
*        <FILLMEIN>
*
* @param ObjectInformation
*        <FILLMEIN>
*
* @param Length
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtSetInformationObject(IN HANDLE ObjectHandle,
                       IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
                       IN PVOID ObjectInformation,
                       IN ULONG Length)
{
    NTSTATUS Status;
    OBP_SET_HANDLE_ATTRIBUTES_CONTEXT Context;
    PVOID ObjectTable;
    KAPC_STATE ApcState;
    POBJECT_DIRECTORY Directory;
    KPROCESSOR_MODE PreviousMode;
    BOOLEAN AttachedToProcess = FALSE;
    PAGED_CODE();

    /* Validate the information class */
    switch (ObjectInformationClass)
    {
        case ObjectHandleFlagInformation:

            /* Validate the length */
            if (Length != sizeof(OBJECT_HANDLE_ATTRIBUTE_INFORMATION))
            {
                /* Invalid length */
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            /* Save the previous mode */
            Context.PreviousMode = ExGetPreviousMode();

            /* Check if we were called from user mode */
            if (Context.PreviousMode != KernelMode)
            {
                /* Enter SEH */
                _SEH2_TRY
                {
                    /* Probe and capture the attribute buffer */
                    ProbeForRead(ObjectInformation,
                                 sizeof(OBJECT_HANDLE_ATTRIBUTE_INFORMATION),
                                 sizeof(BOOLEAN));
                    Context.Information = *(POBJECT_HANDLE_ATTRIBUTE_INFORMATION)
                                            ObjectInformation;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    /* Return the exception code */
                    _SEH2_YIELD(return _SEH2_GetExceptionCode());
                }
                _SEH2_END;
            }
            else
            {
                /* Just copy the buffer directly */
                Context.Information = *(POBJECT_HANDLE_ATTRIBUTE_INFORMATION)
                                        ObjectInformation;
            }

            /* Check if this is a kernel handle */
            if (ObpIsKernelHandle(ObjectHandle, Context.PreviousMode))
            {
                /* Get the actual handle */
                ObjectHandle = ObKernelHandleToHandle(ObjectHandle);
                ObjectTable = ObpKernelHandleTable;

                /* Check if we're not in the system process */
                if (PsGetCurrentProcess() != PsInitialSystemProcess)
                {
                    /* Attach to it */
                    KeStackAttachProcess(&PsInitialSystemProcess->Pcb, &ApcState);
                    AttachedToProcess = TRUE;
                }
            }
            else
            {
                /* Use the current table */
                ObjectTable = PsGetCurrentProcess()->ObjectTable;
            }

            /* Change the handle attributes */
            if (!ExChangeHandle(ObjectTable,
                                ObjectHandle,
                                ObpSetHandleAttributes,
                                (ULONG_PTR)&Context))
            {
                /* Some failure */
                Status = STATUS_ACCESS_DENIED;
            }
            else
            {
                /* We are done */
                Status = STATUS_SUCCESS;
            }

            /* De-attach if we were attached, and return status */
            if (AttachedToProcess) KeUnstackDetachProcess(&ApcState);
            break;

        case ObjectSessionInformation:

            /* Only a system process can do this */
            PreviousMode = ExGetPreviousMode();
            if (!SeSinglePrivilegeCheck(SeTcbPrivilege, PreviousMode))
            {
                /* Fail */
                DPRINT1("Privilege not held\n");
                Status = STATUS_PRIVILEGE_NOT_HELD;
            }
            else
            {
                /* Get the object directory */
                Status = ObReferenceObjectByHandle(ObjectHandle,
                                                   0,
                                                   ObpDirectoryObjectType,
                                                   PreviousMode,
                                                   (PVOID*)&Directory,
                                                   NULL);
                if (NT_SUCCESS(Status))
                {
                    /* Setup a lookup context */
                    OBP_LOOKUP_CONTEXT LookupContext;
                    ObpInitializeLookupContext(&LookupContext);

                    /* Set the directory session ID */
                    ObpAcquireDirectoryLockExclusive(Directory, &LookupContext);
                    Directory->SessionId = PsGetCurrentProcessSessionId();
                    ObpReleaseDirectoryLock(Directory, &LookupContext);

                    /* We're done, release the context and dereference the directory */
                    ObpReleaseLookupContext(&LookupContext);
                    ObDereferenceObject(Directory);
                }
            }
            break;

        default:
            /* Unsupported class */
            Status = STATUS_INVALID_INFO_CLASS;
            break;
    }

    return Status;
}

/* EOF */

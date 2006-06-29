/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ob/create.c
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
#include <internal/debug.h>

extern ULONG NtGlobalFlag;

POBJECT_TYPE ObTypeObjectType = NULL;
KEVENT ObpDefaultObject;

NPAGED_LOOKASIDE_LIST ObpNmLookasideList, ObpCiLookasideList;

WORK_QUEUE_ITEM ObpReaperWorkItem;
volatile PVOID ObpReaperList;

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

    /* Check if we have create info */
    if (Header->Flags & OB_FLAG_CREATE_INFO)
    {
        /* Double-check that it exists */
        if (Header->ObjectCreateInfo)
        {
            /* Free it */
            ObpFreeAndReleaseCapturedAttributes(Header->ObjectCreateInfo);
            Header->ObjectCreateInfo = NULL;
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

    /* Free the object using the same allocation tag */
    ExFreePoolWithTag(HeaderLocation,
                      ObjectType ? TAG('T', 'j', 'b', 'O') : ObjectType->Key);

    /* Decrease the total */
    ObjectType->TotalNumberOfObjects--;
}

VOID
FASTCALL
ObpDeleteObject(IN PVOID Object)
{
    POBJECT_HEADER Header;
    POBJECT_TYPE ObjectType;
    POBJECT_HEADER_NAME_INFO NameInfo;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;
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
        /* Remove the object from the type list */
        RemoveEntryList(&CreatorInfo->TypeList);
    }

    /* Check if we have a name */
    if ((NameInfo) && (NameInfo->Name.Buffer))
    {
        /* Free it */
        ExFreePool(NameInfo->Name.Buffer);

        /* Clean up the string so we don't try this again */
        RtlInitUnicodeString(&NameInfo->Name, NULL);
    }

    /* Check if we have a security descriptor */
    if (Header->SecurityDescriptor)
    {
        ObjectType->TypeInfo.SecurityProcedure(Object,
                                               DeleteSecurityDescriptor,
                                               0,
                                               NULL,
                                               NULL,
                                               &Header->SecurityDescriptor,
                                               0,
                                               NULL);
    }

    /* Check if we have a delete procedure */
    if (ObjectType->TypeInfo.DeleteProcedure)
    {
        /* Call it */
        ObjectType->TypeInfo.DeleteProcedure(Object);
    }

    /* Now de-allocate all object members */
    ObpDeallocateObject(Object);
}

VOID
NTAPI
ObpReapObject(IN PVOID Parameter)
{
    POBJECT_HEADER ReapObject;
    PVOID NextObject;

    /* Start reaping */
    while((ReapObject = InterlockedExchangePointer(&ObpReaperList, NULL)))
    {
        /* Start deletion loop */
        do
        {
            /* Get the next object */
            NextObject = ReapObject->NextToFree;

            /* Delete the object */
            ObpDeleteObject(&ReapObject->Body);

            /* Move to the next one */
            ReapObject = NextObject;
        } while(NextObject != NULL);
    }
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
    if (Permanent)
    {
        /* Set it to permanent */
        ObjectHeader->Flags |= OB_FLAG_PERMANENT;
    }
    else
    {
        /* Remove the flag */
        ObjectHeader->Flags &= ~OB_FLAG_PERMANENT;

        /* Check if we should delete the object now */
        ObpDeleteNameCheck(ObjectBody);
    }
}

NTSTATUS
NTAPI
ObpCaptureObjectName(IN OUT PUNICODE_STRING CapturedName,
                     IN PUNICODE_STRING ObjectName,
                     IN KPROCESSOR_MODE AccessMode,
                     IN BOOLEAN AllocateFromLookaside)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG StringLength, MaximumLength;
    PWCHAR StringBuffer = NULL;
    UNICODE_STRING LocalName = {}; /* <= GCC 4.0 + Optimizer */
    PAGED_CODE();

    /* Initialize the Input String */
    RtlInitEmptyUnicodeString(CapturedName, NULL, 0);

    /* Protect everything */
    _SEH_TRY
    {
        /* Check if we came from user mode */
        if (AccessMode != KernelMode)
        {
            /* First Probe the String */
            ProbeForReadUnicodeString(ObjectName);
            LocalName = *ObjectName;
            ProbeForRead(LocalName.Buffer,
                         LocalName.Length,
                         sizeof(WCHAR));
        }
        else
        {
            /* No probing needed */
            LocalName = *ObjectName;
        }

        /* Make sure there really is a string */
        if ((StringLength = LocalName.Length))
        {
            /* Check that the size is a valid WCHAR multiple */
            if ((StringLength & (sizeof(WCHAR) - 1)) ||
                /* Check that the NULL-termination below will work */
                (StringLength == (MAXUSHORT - sizeof(UNICODE_NULL) + 1)))
            {
                /* PS: Please keep the checks above expanded for clarity */
                DPRINT1("Invalid String Length\n");
                Status = STATUS_OBJECT_NAME_INVALID;
            }
            else
            {
                /* Set the maximum length to the length plus the terminator */
                MaximumLength = StringLength + sizeof(UNICODE_NULL);

                /* Check if we should use the lookaside buffer */
                //if (!(AllocateFromLookaside) || (MaximumLength > 248))
                {
                    /* Nope, allocate directly from pool */
                    StringBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                                         MaximumLength,
                                                         OB_NAME_TAG);
                }
                //else
                {
                    /* Allocate from the lookaside */
                //    MaximumLength = 248;
                //    StringBuffer =
                //        ObpAllocateCapturedAttributes(LookasideNameBufferList);
                }

                /* Setup the string */
                CapturedName->Length = StringLength;
                CapturedName->MaximumLength = MaximumLength;
                CapturedName->Buffer = StringBuffer;

                /* Make sure we have a buffer */
                if (StringBuffer)
                {
                    /* Copy the string and null-terminate it */
                    RtlMoveMemory(StringBuffer, LocalName.Buffer, StringLength);
                    StringBuffer[StringLength / sizeof(WCHAR)] = UNICODE_NULL;
                }
                else
                {
                    /* Fail */
                    DPRINT1("Out of Memory!\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        }
    }
    _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
    {
        Status = _SEH_GetExceptionCode();
        if (StringBuffer) ExFreePool(StringBuffer);
    }
    _SEH_END;

    /* Return */
    return Status;
}

NTSTATUS
NTAPI
ObpCaptureObjectAttributes(IN POBJECT_ATTRIBUTES ObjectAttributes,
                           IN KPROCESSOR_MODE AccessMode,
                           IN BOOLEAN AllocateFromLookaside,
                           IN POBJECT_CREATE_INFORMATION ObjectCreateInfo,
                           OUT PUNICODE_STRING ObjectName)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PSECURITY_QUALITY_OF_SERVICE SecurityQos;
    PUNICODE_STRING LocalObjectName = NULL;
    PAGED_CODE();

    /* Zero out the Capture Data */
    RtlZeroMemory(ObjectCreateInfo, sizeof(OBJECT_CREATE_INFORMATION));

    /* SEH everything here for protection */
    _SEH_TRY
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
                (ObjectAttributes->Attributes & ~OBJ_VALID_ATTRIBUTES))
            {
                /* Invalid combination, fail */
                Status = STATUS_INVALID_PARAMETER;
                _SEH_LEAVE;
            }

            /* Set some Create Info */
            ObjectCreateInfo->RootDirectory = ObjectAttributes->RootDirectory;
            ObjectCreateInfo->Attributes = ObjectAttributes->Attributes;
            LocalObjectName = ObjectAttributes->ObjectName;
            SecurityDescriptor = ObjectAttributes->SecurityDescriptor;
            SecurityQos = ObjectAttributes->SecurityQualityOfService;

            /* Check if we have a security descriptor */
            if (SecurityDescriptor)
            {
                /* Capture it */
                Status = SeCaptureSecurityDescriptor(SecurityDescriptor,
                                                     AccessMode,
                                                     NonPagedPool,
                                                     TRUE,
                                                     &ObjectCreateInfo->
                                                     SecurityDescriptor);
                if(!NT_SUCCESS(Status))
                {
                    /* Capture failed, quit */
                    ObjectCreateInfo->SecurityDescriptor = NULL;
                    _SEH_LEAVE;
                }

                /* Save the probe mode and security descriptor size */
                ObjectCreateInfo->SecurityDescriptorCharge = 2048; /* FIXME */
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
    _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
    {
        /* Get the exception */
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    if (NT_SUCCESS(Status))
    {
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

            /* He can't have specified a Root Directory */
            if (ObjectCreateInfo->RootDirectory)
            {
                Status = STATUS_OBJECT_NAME_INVALID;
            }
        }
    }

    /* Cleanup if we failed */
    if (!NT_SUCCESS(Status)) ObpReleaseCapturedAttributes(ObjectCreateInfo);

    /* Return status to caller */
    return Status;
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
    BOOLEAN HasHandleInfo = FALSE;
    BOOLEAN HasNameInfo = FALSE;
    BOOLEAN HasCreatorInfo = FALSE;
    POBJECT_HEADER_HANDLE_INFO HandleInfo;
    POBJECT_HEADER_NAME_INFO NameInfo;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;
    POOL_TYPE PoolType;
    ULONG FinalSize = ObjectSize;
    ULONG Tag;
    PAGED_CODE();

    /* If we don't have an Object Type yet, force NonPaged */
    if (!ObjectType) 
    {
        PoolType = NonPagedPool;
        Tag = TAG('O', 'b', 'j', 'T');
    }
    else
    {
        PoolType = ObjectType->TypeInfo.PoolType;
        Tag = ObjectType->Key;
    }

    /* Check if the Object has a name */
    if (ObjectName->Buffer) 
    {
        FinalSize += sizeof(OBJECT_HEADER_NAME_INFO);
        HasNameInfo = TRUE;
    }

    if (ObjectType)
    {
        /* Check if the Object maintains handle counts */
        if (ObjectType->TypeInfo.MaintainHandleCount)
        {
            FinalSize += sizeof(OBJECT_HEADER_HANDLE_INFO);
            HasHandleInfo = TRUE;
        }
        
        /* Check if the Object maintains type lists */
        if (ObjectType->TypeInfo.MaintainTypeList) 
        {
            FinalSize += sizeof(OBJECT_HEADER_CREATOR_INFO);
            HasCreatorInfo = TRUE;
        }
    }

    /* Allocate memory for the Object and Header */
    Header = ExAllocatePoolWithTag(PoolType, FinalSize, Tag);
    if (!Header) return STATUS_INSUFFICIENT_RESOURCES;

    /* Initialize Handle Info */
    if (HasHandleInfo)
    {
        HandleInfo = (POBJECT_HEADER_HANDLE_INFO)Header;
        HandleInfo->SingleEntry.HandleCount = 0;
        Header = (POBJECT_HEADER)(HandleInfo + 1);
    }

    /* Initialize the Object Name Info */
    if (HasNameInfo) 
    {
        NameInfo = (POBJECT_HEADER_NAME_INFO)Header;
        NameInfo->Name = *ObjectName;
        NameInfo->Directory = NULL;
        Header = (POBJECT_HEADER)(NameInfo + 1);
    }

    /* Initialize Creator Info */
    if (HasCreatorInfo)
    {
        CreatorInfo = (POBJECT_HEADER_CREATOR_INFO)Header;
        CreatorInfo->CreatorUniqueProcess = PsGetCurrentProcess() ?
                                            PsGetCurrentProcessId() : 0;
        InitializeListHead(&CreatorInfo->TypeList);
        Header = (POBJECT_HEADER)(CreatorInfo + 1);
    }

    /* Initialize the object header */
    RtlZeroMemory(Header, ObjectSize);
    Header->PointerCount = 1;
    Header->Type = ObjectType;
    Header->Flags = OB_FLAG_CREATE_INFO;
    Header->ObjectCreateInfo = ObjectCreateInfo;

    /* Set the Offsets for the Info */
    if (HasHandleInfo)
    {
        Header->HandleInfoOffset = HasNameInfo *
                                   sizeof(OBJECT_HEADER_NAME_INFO) +
                                   sizeof(OBJECT_HEADER_HANDLE_INFO) +
                                   HasCreatorInfo *
                                   sizeof(OBJECT_HEADER_CREATOR_INFO);

        /* Set the flag so we know when freeing */
        Header->Flags |= OB_FLAG_SINGLE_PROCESS;
    }
    if (HasNameInfo)
    {
        Header->NameInfoOffset = sizeof(OBJECT_HEADER_NAME_INFO) + 
                                 HasCreatorInfo *
                                 sizeof(OBJECT_HEADER_CREATOR_INFO);
    }
    if (HasCreatorInfo) Header->Flags |= OB_FLAG_CREATOR_INFO;
    if ((ObjectCreateInfo) && (ObjectCreateInfo->Attributes & OBJ_PERMANENT))
    {
        /* Set the needed flag so we can check */
        Header->Flags |= OB_FLAG_PERMANENT;
    }
    if ((ObjectCreateInfo) && (ObjectCreateInfo->Attributes & OBJ_EXCLUSIVE))
    {
        /* Set the needed flag so we can check */
        Header->Flags |= OB_FLAG_EXCLUSIVE;
    }
    if (PreviousMode == KernelMode)
    {
        /* Set the kernel flag */
        Header->Flags |= OB_FLAG_KERNEL_MODE;
    }

    /* Increase the number of objects of this type */
    if (ObjectType) ObjectType->TotalNumberOfObjects++;

    /* Return Header */
    *ObjectHeader = Header;
    return STATUS_SUCCESS;
}

/* PUBLIC FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
ObCreateObject(IN KPROCESSOR_MODE ObjectAttributesAccessMode OPTIONAL,
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
    DPRINT("ObCreateObject(Type %p ObjectAttributes %p, Object %p)\n", 
            Type, ObjectAttributes, Object);

    /* Allocate a capture buffer */
    ObjectCreateInfo = ObpAllocateCapturedAttributes(LookasideCreateInfoList);
    if (!ObjectCreateInfo) return STATUS_INSUFFICIENT_RESOURCES;

    /* Capture all the info */
    Status = ObpCaptureObjectAttributes(ObjectAttributes,
                                        ObjectAttributesAccessMode,
                                        FALSE,
                                        ObjectCreateInfo,
                                        &ObjectName);
    if (NT_SUCCESS(Status))
    {
        /* Validate attributes */
        if (Type->TypeInfo.InvalidAttributes &
            ObjectCreateInfo->Attributes)
        {
            /* Fail */
            Status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            /* Save the pool charges */
            ObjectCreateInfo->PagedPoolCharge = PagedPoolCharge;
            ObjectCreateInfo->NonPagedPoolCharge = NonPagedPoolCharge;

            /* Allocate the Object */
            Status = ObpAllocateObject(ObjectCreateInfo,
                                       &ObjectName,
                                       Type,
                                       ObjectSize + sizeof(OBJECT_HEADER),
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
                                                ObjectAttributesAccessMode))
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
        ObpReleaseCapturedAttributes(ObjectCreateInfo);
        if (ObjectName.Buffer) ObpReleaseCapturedName(&ObjectName);
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
    CHAR Tag[4];
    OBP_LOOKUP_CONTEXT Context;
    PWCHAR p;
    ULONG i;
    UNICODE_STRING ObjectName;

    /* Verify parameters */
    if (!(TypeName) ||
        !(TypeName->Length) ||
        !(ObjectTypeInitializer) ||
        (ObjectTypeInitializer->Length != sizeof(*ObjectTypeInitializer)) ||
        (ObjectTypeInitializer->InvalidAttributes & ~OBJ_VALID_ATTRIBUTES) ||
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

    /* Check if we've already created the directory of types */
    if (ObpTypeDirectoryObject)
    {
        /* Then scan it to figure out if we've already created this type */
        Context.Directory = ObpTypeDirectoryObject;
        Context.DirectoryLocked = TRUE;
        if (ObpLookupEntryDirectory(ObpTypeDirectoryObject,
                                    TypeName,
                                    OBJ_CASE_INSENSITIVE,
                                    FALSE,
                                    &Context))
        {
            /* We have already created it, so fail */
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
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Set the length and copy the name */
    ObjectName.MaximumLength = TypeName->MaximumLength;
    RtlCopyUnicodeString(&ObjectName, TypeName);

    /* Allocate the Object */
    Status = ObpAllocateObject(NULL,
                               &ObjectName,
                               ObTypeObjectType,
                               sizeof(OBJECT_TYPE) + sizeof(OBJECT_HEADER),
                               KernelMode,
                               (POBJECT_HEADER*)&Header);
    if (!NT_SUCCESS(Status)) return Status;
    LocalObjectType = (POBJECT_TYPE)&Header->Body;

    /* Check if this is the first Object Type */
    if (!ObTypeObjectType)
    {
        ObTypeObjectType = LocalObjectType;
        Header->Type = ObTypeObjectType;
        LocalObjectType->TotalNumberOfObjects = 1;
        LocalObjectType->Key = TAG('O', 'b', 'j', 'T');
    }
    else
    {
        /* Set Tag */
        Tag[0] = TypeName->Buffer[0];
        Tag[1] = TypeName->Buffer[1];
        Tag[2] = TypeName->Buffer[2];
        Tag[3] = TypeName->Buffer[3];
        LocalObjectType->Key = *(PULONG)Tag;
    }

    /* Set it up */
    LocalObjectType->TypeInfo = *ObjectTypeInitializer;
    LocalObjectType->Name = *TypeName;
    LocalObjectType->TypeInfo.PoolType = ObjectTypeInitializer->PoolType;

    /* These two flags need to be manually set up */
    Header->Flags |= OB_FLAG_KERNEL_MODE | OB_FLAG_PERMANENT;

    /* Check if we have to maintain a type list */
    if (NtGlobalFlag & FLG_MAINTAIN_OBJECT_TYPELIST)
    {
        /* Enable support */
        LocalObjectType->TypeInfo.MaintainTypeList = TRUE;
    }

    /* Calculate how much space our header'll take up */
    HeaderSize = sizeof(OBJECT_HEADER) + sizeof(OBJECT_HEADER_NAME_INFO) +
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
        LocalObjectType->DefaultObject = (PVOID)FIELD_OFFSET(FILE_OBJECT,
                                                             Event);
    }
    /* FIXME: When LPC stops sucking, add a hack for Waitable Ports */
    else
    {
        /* No default Object */
        LocalObjectType->DefaultObject = NULL;
    }

    /* Initialize Object Type components */
    ExInitializeResourceLite(&LocalObjectType->Mutex);
    InitializeListHead(&LocalObjectType->TypeList);

    /* Check if we're actually creating the directory object itself */
    if (ObpTypeDirectoryObject)
    {
        /* Insert it into the Object Directory */
        ObpInsertEntryDirectory(ObpTypeDirectoryObject, &Context, Header);
        ObReferenceObject(ObpTypeDirectoryObject);
    }

    /* Return the object type and creations tatus */
    *ObjectType = LocalObjectType;
    return Status;
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
    /* Call the internal API */
    ObpSetPermanentObject (ObjectBody, FALSE);
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
    Status = SeSinglePrivilegeCheck(SeCreatePermanentPrivilege,
                                    PreviousMode);
    if (!NT_SUCCESS(Status)) return STATUS_PRIVILEGE_NOT_HELD;

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
    ULONG InfoLength;
    PVOID Object = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PAGED_CODE();

    /* Check if the caller is from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Protect validation with SEH */
        _SEH_TRY
        {
            /* Probe the input structure */
            ProbeForWrite(ObjectInformation, Length, sizeof(UCHAR));

            /* If we have a result length, probe it too */
            if (ResultLength) ProbeForWriteUlong(ResultLength);
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Fail if we raised an exception */
        if (!NT_SUCCESS(Status)) return Status;
    }

    /*
     * Make sure this isn't a generic type query, since the caller doesn't
     * have to give a handle for it
     */
    if (ObjectInformationClass != ObjectAllTypesInformation)
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
    }

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
            BasicInfo->PagedPoolUsage = 0; /* FIXME*/
            BasicInfo->NonPagedPoolUsage = 0; /* FIXME*/

            /* Copy name information */
            BasicInfo->NameInformationLength = 0; /* FIXME*/
            BasicInfo->TypeInformationLength = 0; /* FIXME*/

            /* Copy security information */
            BasicInfo->SecurityDescriptorLength = 0; /* FIXME*/

            /* Check if this is a symlink */
            if (ObjectHeader->Type == ObSymbolicLinkType)
            {
                /* Return the creation time */
                BasicInfo->CreateTime.QuadPart =
                    ((POBJECT_SYMBOLIC_LINK)Object)->CreationTime.QuadPart;
            }
            else
            {
                /* Otherwise return 0 */
                BasicInfo->CreateTime.QuadPart = (ULONGLONG)0;
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
            DPRINT1("NOT IMPLEMENTED!\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        /* Information about all types */
        case ObjectAllTypesInformation:
            DPRINT1("NOT IMPLEMENTED!\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        /* Information about the handle flags */
        case ObjectHandleInformation:

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
            HandleFlags->Inherit = (HandleInfo.HandleAttributes &
                                    EX_HANDLE_ENTRY_INHERITABLE) != 0;
            HandleFlags->ProtectFromClose = (HandleInfo.HandleAttributes &
                                             EX_HANDLE_ENTRY_PROTECTFROMCLOSE) != 0;

            /* Break out with success */
            Status = STATUS_SUCCESS;
            break;

        /* Anything else */
        default:

            /* Fail it */
            Status = STATUS_INVALID_INFO_CLASS;
            break;
    }

    /* Dereference the object if we had referenced it */
    if (Object) ObDereferenceObject (Object);

    /* Check if the caller wanted the return length */
    if (ResultLength)
    {
        /* Protect the write to user mode */
        _SEH_TRY
        {
            /* Write the length */
            *ResultLength = Length;
        }
        _SEH_HANDLE
        {
            /* Otherwise, get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }

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
    NTSTATUS Status = STATUS_SUCCESS;
    OBP_SET_HANDLE_ATTRIBUTES_CONTEXT Context;
    PVOID ObjectTable;
    KAPC_STATE ApcState;
    BOOLEAN AttachedToProcess = FALSE;
    PAGED_CODE();

    /* Validate the information class */
    if (ObjectInformationClass != ObjectHandleInformation)
    {
        /* Invalid class */
        return STATUS_INVALID_INFO_CLASS;
    }

    /* Validate the length */
    if (Length != sizeof (OBJECT_HANDLE_ATTRIBUTE_INFORMATION))
    {
        /* Invalid length */
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    /* Save the previous mode and actual information */
    Context.PreviousMode = ExGetPreviousMode();
    Context.Information = *(POBJECT_HANDLE_ATTRIBUTE_INFORMATION)
                            ObjectInformation;

    /* Check if this is a kernel handle */
    if (ObIsKernelHandle(ObjectHandle, Context.PreviousMode))
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
                        &Context))
    {
        /* Some failure */
        Status = STATUS_ACCESS_DENIED;
    }

    /* De-attach if we were attached, and return status */
    if (AttachedToProcess) KeUnstackDetachProcess(&ApcState);
    return Status;
}

/* EOF */

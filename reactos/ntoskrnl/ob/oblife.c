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

typedef struct _RETENTION_CHECK_PARAMS
{
    WORK_QUEUE_ITEM WorkItem;
    POBJECT_HEADER ObjectHeader;
} RETENTION_CHECK_PARAMS, *PRETENTION_CHECK_PARAMS;

/* PRIVATE FUNCTIONS *********************************************************/

static NTSTATUS
ObpDeleteObject(POBJECT_HEADER Header)
{
    PVOID HeaderLocation = Header;
    POBJECT_HEADER_HANDLE_INFO HandleInfo;
    POBJECT_HEADER_NAME_INFO NameInfo;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;

    DPRINT("ObpDeleteObject(Header %p)\n", Header);
    if (KeGetCurrentIrql() != PASSIVE_LEVEL)
    {
        DPRINT("ObpDeleteObject called at an unsupported IRQL.  Use ObpDeleteObjectDpcLevel instead.\n");
        KEBUGCHECK(0);
    }

    if (Header->Type != NULL &&
        Header->Type->TypeInfo.DeleteProcedure != NULL)
    {
        Header->Type->TypeInfo.DeleteProcedure(&Header->Body);
    }

    if (Header->SecurityDescriptor != NULL)
    {
        ObpRemoveSecurityDescriptor(Header->SecurityDescriptor);
    }

    if (HEADER_TO_OBJECT_NAME(Header))
    {
        if(HEADER_TO_OBJECT_NAME(Header)->Name.Buffer)
        {
            ExFreePool(HEADER_TO_OBJECT_NAME(Header)->Name.Buffer);
        }
    }
    if (Header->ObjectCreateInfo)
    {
        ObpReleaseCapturedAttributes(Header->ObjectCreateInfo);
        ExFreePool(Header->ObjectCreateInfo);
    }

    /* To find the header, walk backwards from how we allocated */
    if ((CreatorInfo = HEADER_TO_CREATOR_INFO(Header)))
    {
        HeaderLocation = CreatorInfo;
    }   
    if ((NameInfo = HEADER_TO_OBJECT_NAME(Header)))
    {
        HeaderLocation = NameInfo;
    }
    if ((HandleInfo = HEADER_TO_HANDLE_INFO(Header)))
    {
        HeaderLocation = HandleInfo;
    }

    DPRINT("ObPerformRetentionChecks() = Freeing object\n");
    ExFreePool(HeaderLocation);

    return(STATUS_SUCCESS);
}


VOID STDCALL
ObpDeleteObjectWorkRoutine (IN PVOID Parameter)
{
    PRETENTION_CHECK_PARAMS Params = (PRETENTION_CHECK_PARAMS)Parameter;
    /* ULONG Tag; */ /* See below */

    ASSERT(Params);
    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL); /* We need PAGED_CODE somewhere... */

    /* Turn this on when we have ExFreePoolWithTag
    Tag = Params->ObjectHeader->Type->Tag; */
    ObpDeleteObject(Params->ObjectHeader);
    ExFreePool(Params);
    /* ExFreePoolWithTag(Params, Tag); */
}


NTSTATUS
ObpDeleteObjectDpcLevel(IN POBJECT_HEADER ObjectHeader,
                        IN LONG OldPointerCount)
{
#if 0
    if (ObjectHeader->PointerCount < 0)
    {
        CPRINT("Object %p/%p has invalid reference count (%d)\n",
            ObjectHeader, HEADER_TO_BODY(ObjectHeader),
            ObjectHeader->PointerCount);
        KEBUGCHECK(0);
    }

    if (ObjectHeader->HandleCount < 0)
    {
        CPRINT("Object %p/%p has invalid handle count (%d)\n",
            ObjectHeader, HEADER_TO_BODY(ObjectHeader),
            ObjectHeader->HandleCount);
        KEBUGCHECK(0);
    }
#endif


    switch (KeGetCurrentIrql ())
    {
    case PASSIVE_LEVEL:
        return ObpDeleteObject (ObjectHeader);

    case APC_LEVEL:
    case DISPATCH_LEVEL:
        {
            PRETENTION_CHECK_PARAMS Params;

            /*
            We use must succeed pool here because if the allocation fails
            then we leak memory.
            */
            Params = (PRETENTION_CHECK_PARAMS)
                ExAllocatePoolWithTag(NonPagedPoolMustSucceed,
                sizeof(RETENTION_CHECK_PARAMS),
                ObjectHeader->Type->Key);
            Params->ObjectHeader = ObjectHeader;
            ExInitializeWorkItem(&Params->WorkItem,
                ObpDeleteObjectWorkRoutine,
                (PVOID)Params);
            ExQueueWorkItem(&Params->WorkItem,
                CriticalWorkQueue);
        }
        return STATUS_PENDING;

    default:
        DPRINT("ObpDeleteObjectDpcLevel called at unsupported "
            "IRQL %u!\n", KeGetCurrentIrql());
        KEBUGCHECK(0);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
ObpCaptureObjectName(IN OUT PUNICODE_STRING CapturedName,
                     IN PUNICODE_STRING ObjectName,
                     IN KPROCESSOR_MODE AccessMode)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG StringLength;
    PWCHAR StringBuffer = NULL;
    UNICODE_STRING LocalName = {}; /* <= GCC 4.0 + Optimizer */
    
    /* Initialize the Input String */
    RtlInitUnicodeString(CapturedName, NULL);

    /* Protect everything */
    _SEH_TRY
    {
        /* First Probe the String */
        DPRINT("ObpCaptureObjectName: %wZ\n", ObjectName);
        if (AccessMode != KernelMode)
        {
            ProbeForRead(ObjectName,
                         sizeof(UNICODE_STRING),
                         sizeof(USHORT));
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
        DPRINT("Probing OK\n");
        if ((StringLength = LocalName.Length))
        {
            /* Check that the size is a valid WCHAR multiple */
            if ((StringLength & (sizeof(WCHAR) - 1)) ||
                /* Check that the NULL-termination below will work */
                (StringLength == (MAXUSHORT - sizeof(WCHAR) + 1)))
            {
                /* PS: Please keep the checks above expanded for clarity */
                DPRINT1("Invalid String Length\n");
                Status = STATUS_OBJECT_NAME_INVALID;
            }
            else
            {
                /* Allocate a non-paged buffer for this string */
                DPRINT("Capturing String\n");
                CapturedName->Length = StringLength;
                CapturedName->MaximumLength = StringLength + sizeof(WCHAR);
                if ((StringBuffer = ExAllocatePoolWithTag(NonPagedPool, 
                                                          StringLength + sizeof(WCHAR),
                                                          OB_NAME_TAG)))
                {                                    
                    /* Copy the string and null-terminate it */
                    RtlMoveMemory(StringBuffer, LocalName.Buffer, StringLength);
                    StringBuffer[StringLength / sizeof(WCHAR)] = UNICODE_NULL;
                    CapturedName->Buffer = StringBuffer;
                    DPRINT("String Captured: %wZ\n", CapturedName);
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

        /* Remember to free the buffer in case of failure */
        DPRINT1("Failed\n");
        if (StringBuffer) ExFreePool(StringBuffer);
    }
    _SEH_END;
    
    /* Return */
    DPRINT("Returning: %lx\n", Status);
    return Status;
}

NTSTATUS
STDCALL
ObpCaptureObjectAttributes(IN POBJECT_ATTRIBUTES ObjectAttributes,
                           IN KPROCESSOR_MODE AccessMode,
                           IN POBJECT_TYPE ObjectType,
                           IN POBJECT_CREATE_INFORMATION ObjectCreateInfo,
                           OUT PUNICODE_STRING ObjectName)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PSECURITY_QUALITY_OF_SERVICE SecurityQos;
    PUNICODE_STRING LocalObjectName = NULL;

    /* Zero out the Capture Data */
    DPRINT("ObpCaptureObjectAttributes\n");
    RtlZeroMemory(ObjectCreateInfo, sizeof(OBJECT_CREATE_INFORMATION));
    
    /* SEH everything here for protection */
    _SEH_TRY
    {
        /* Check if we got Oba */
        if (ObjectAttributes)
        {
            if (AccessMode != KernelMode)
            {
                DPRINT("Probing OBA\n");
                ProbeForRead(ObjectAttributes,
                             sizeof(OBJECT_ATTRIBUTES),
                             sizeof(ULONG));
            }
        
            /* Validate the Size and Attributes */
            DPRINT("Validating OBA\n");
            if ((ObjectAttributes->Length != sizeof(OBJECT_ATTRIBUTES)) ||
                (ObjectAttributes->Attributes & ~OBJ_VALID_ATTRIBUTES))
            {
                Status = STATUS_INVALID_PARAMETER;
                DPRINT1("Invalid Size: %lx or Attributes: %lx\n",
                       ObjectAttributes->Length, ObjectAttributes->Attributes); 
                _SEH_LEAVE;
            }
        
            /* Set some Create Info */
            DPRINT("Creating OBCI\n");
            ObjectCreateInfo->RootDirectory = ObjectAttributes->RootDirectory;
            ObjectCreateInfo->Attributes = ObjectAttributes->Attributes;
            LocalObjectName = ObjectAttributes->ObjectName;
            SecurityDescriptor = ObjectAttributes->SecurityDescriptor;
            SecurityQos = ObjectAttributes->SecurityQualityOfService;
        
            /* Validate the SD */
            if (SecurityDescriptor)
            {
                DPRINT("Probing SD: %x\n", SecurityDescriptor);
                Status = SeCaptureSecurityDescriptor(SecurityDescriptor,
                                                     AccessMode,
                                                     NonPagedPool,
                                                     TRUE,
                                                     &ObjectCreateInfo->SecurityDescriptor);
                if(!NT_SUCCESS(Status))
                {
                    DPRINT1("Unable to capture the security descriptor!!!\n");
                    ObjectCreateInfo->SecurityDescriptor = NULL;
                    _SEH_LEAVE;
                }
            
                DPRINT("Probe done\n");
                ObjectCreateInfo->SecurityDescriptorCharge = 2048; /* FIXME */
                ObjectCreateInfo->ProbeMode = AccessMode;
            }
        
            /* Validate the QoS */
            if (SecurityQos)
            {
                if (AccessMode != KernelMode)
                {
                    DPRINT("Probing QoS\n");
                    ProbeForRead(SecurityQos,
                                 sizeof(SECURITY_QUALITY_OF_SERVICE),
                                 sizeof(ULONG));
                }
            
                /* Save Info */
                ObjectCreateInfo->SecurityQualityOfService = *SecurityQos;
                ObjectCreateInfo->SecurityQos = &ObjectCreateInfo->SecurityQualityOfService;
            }
        }
        else
        {
            LocalObjectName = NULL;
        }
    }
    _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
    {
        Status = _SEH_GetExceptionCode();
        DPRINT1("Failed\n");
    }
    _SEH_END;

    if (NT_SUCCESS(Status))
    {
        /* Now check if the Object Attributes had an Object Name */
        if (LocalObjectName)
        {
            DPRINT("Name Buffer: %wZ\n", LocalObjectName);
            Status = ObpCaptureObjectName(ObjectName,
                                          LocalObjectName,
                                          AccessMode);
        }
        else
        {
            /* Clear the string */
            RtlInitUnicodeString(ObjectName, NULL);

            /* He can't have specified a Root Directory */
            if (ObjectCreateInfo->RootDirectory)
            {
                DPRINT1("Invalid name\n");
                Status = STATUS_OBJECT_NAME_INVALID;
            }
        }
    }
    else
    {
        DPRINT1("Failed to capture, cleaning up\n");
        ObpReleaseCapturedAttributes(ObjectCreateInfo);
    }
    
    DPRINT("Return to caller %x\n", Status);
    return Status;
}


VOID
STDCALL
ObpReleaseCapturedAttributes(IN POBJECT_CREATE_INFORMATION ObjectCreateInfo)
{
    /* Release the SD, it's the only thing we allocated */
    if (ObjectCreateInfo->SecurityDescriptor)
    {
        SeReleaseSecurityDescriptor(ObjectCreateInfo->SecurityDescriptor,
                                    ObjectCreateInfo->ProbeMode,
                                    TRUE);
        ObjectCreateInfo->SecurityDescriptor = NULL;                                        
    }
}

NTSTATUS
STDCALL
ObpAllocateObject(POBJECT_CREATE_INFORMATION ObjectCreateInfo,
                  PUNICODE_STRING ObjectName,
                  POBJECT_TYPE ObjectType,
                  ULONG ObjectSize,
                  POBJECT_HEADER *ObjectHeader)
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
        
    /* If we don't have an Object Type yet, force NonPaged */
    DPRINT("ObpAllocateObject\n");
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
    
    DPRINT("Checking ObjectName: %x\n", ObjectName);
    /* Check if the Object has a name */
    if (ObjectName->Buffer) 
    {
        FinalSize += sizeof(OBJECT_HEADER_NAME_INFO);
        HasNameInfo = TRUE;
    }
    
    if (ObjectType)
    {
        /* Check if the Object maintains handle counts */
        DPRINT("Checking ObjectType->TypeInfo: %x\n", &ObjectType->TypeInfo);
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
    DPRINT("Allocating: %x %x\n", FinalSize, Tag);
    Header = ExAllocatePoolWithTag(PoolType, FinalSize, Tag);
    if (!Header) {
        DPRINT1("Not enough memory!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
           
    /* Initialize Handle Info */
    if (HasHandleInfo)
    {
        HandleInfo = (POBJECT_HEADER_HANDLE_INFO)Header;
        DPRINT("Info: %x\n", HandleInfo);
        HandleInfo->SingleEntry.HandleCount = 0;
        Header = (POBJECT_HEADER)(HandleInfo + 1);
    }
       
    /* Initialize the Object Name Info */
    if (HasNameInfo) 
    {
        NameInfo = (POBJECT_HEADER_NAME_INFO)Header;
        DPRINT("Info: %x %wZ\n", NameInfo, ObjectName);
        NameInfo->Name = *ObjectName;
        NameInfo->Directory = NULL;
        Header = (POBJECT_HEADER)(NameInfo + 1);
    }
    
    /* Initialize Creator Info */
    if (HasCreatorInfo)
    {
        CreatorInfo = (POBJECT_HEADER_CREATOR_INFO)Header;
        DPRINT("Info: %x\n", CreatorInfo);
        /* FIXME: Needs Alex's Init patch
         * CreatorInfo->CreatorUniqueProcess = PsGetCurrentProcessId();
         */
        InitializeListHead(&CreatorInfo->TypeList);
        Header = (POBJECT_HEADER)(CreatorInfo + 1);
    }
    
    /* Initialize the object header */
    RtlZeroMemory(Header, ObjectSize);
    DPRINT("Initalized header %p\n", Header);
    Header->HandleCount = 0;
    Header->PointerCount = 1;
    Header->Type = ObjectType;
    Header->Flags = OB_FLAG_CREATE_INFO;
    
    /* Set the Offsets for the Info */
    if (HasHandleInfo)
    {
        Header->HandleInfoOffset = HasNameInfo * sizeof(OBJECT_HEADER_NAME_INFO) + 
                                   sizeof(OBJECT_HEADER_HANDLE_INFO) +
                                   HasCreatorInfo * sizeof(OBJECT_HEADER_CREATOR_INFO);
        Header->Flags |= OB_FLAG_SINGLE_PROCESS;
    }
    if (HasNameInfo)
    {
        Header->NameInfoOffset = sizeof(OBJECT_HEADER_NAME_INFO) + 
                                 HasCreatorInfo * sizeof(OBJECT_HEADER_CREATOR_INFO);
    }
    if (HasCreatorInfo) Header->Flags |= OB_FLAG_CREATOR_INFO;
    
    if (ObjectCreateInfo && ObjectCreateInfo->Attributes & OBJ_PERMANENT)
    {
        Header->Flags |= OB_FLAG_PERMANENT;
    }
    if (ObjectCreateInfo && ObjectCreateInfo->Attributes & OBJ_EXCLUSIVE)
    {
        Header->Flags |= OB_FLAG_EXCLUSIVE;
    }
    
    /* Link stuff to Object Header */
    Header->ObjectCreateInfo = ObjectCreateInfo;
    
    /* Return Header */
    *ObjectHeader = Header;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ObpCreateTypeObject(POBJECT_TYPE_INITIALIZER ObjectTypeInitializer,
                    PUNICODE_STRING TypeName,
                    POBJECT_TYPE *ObjectType)
{
    POBJECT_HEADER Header;
    POBJECT_TYPE LocalObjectType;
    ULONG HeaderSize;
    NTSTATUS Status;

    DPRINT("ObpCreateTypeObject(ObjectType: %wZ)\n", TypeName);
    
    /* Allocate the Object */
    Status = ObpAllocateObject(NULL, 
                               TypeName,
                               ObTypeObjectType, 
                               OBJECT_ALLOC_SIZE(sizeof(OBJECT_TYPE)),
                               (POBJECT_HEADER*)&Header);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ObpAllocateObject failed!\n");
        return Status;
    }
    
    LocalObjectType = (POBJECT_TYPE)&Header->Body;
    DPRINT("Local ObjectType: %p Header: %p \n", LocalObjectType, Header);
    
    /* Check if this is the first Object Type */
    if (!ObTypeObjectType)
    {
        ObTypeObjectType = LocalObjectType;
        Header->Type = ObTypeObjectType;
        LocalObjectType->Key = TAG('O', 'b', 'j', 'T');
    }
    else
    {   
        CHAR Tag[4];
        Tag[0] = TypeName->Buffer[0];
        Tag[1] = TypeName->Buffer[1];
        Tag[2] = TypeName->Buffer[2];
        Tag[3] = TypeName->Buffer[3];
        
        /* Set Tag */
        DPRINT("Convert: %s \n", Tag);
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

    /* Update the Pool Charges */
    if (ObjectTypeInitializer->PoolType == NonPagedPool)
    {
        LocalObjectType->TypeInfo.DefaultNonPagedPoolCharge += HeaderSize;
    }
    else
    {
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
    /* Special system objects get an optimized hack so they can be waited on */
    else if (TypeName->Length == 8 && !wcscmp(TypeName->Buffer, L"File"))
    {
        LocalObjectType->DefaultObject = (PVOID)FIELD_OFFSET(FILE_OBJECT, Event);
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

    /* Insert it into the Object Directory */
    if (ObpTypeDirectoryObject)
    {
        OBP_LOOKUP_CONTEXT Context;
        Context.Directory = ObpTypeDirectoryObject;
        Context.DirectoryLocked = TRUE;
        ObpLookupEntryDirectory(ObpTypeDirectoryObject,
                                TypeName,
                                OBJ_CASE_INSENSITIVE,
                                FALSE,
                                &Context);
        ObpInsertEntryDirectory(ObpTypeDirectoryObject, &Context, Header);
        ObReferenceObject(ObpTypeDirectoryObject);
    }

    *ObjectType = LocalObjectType;
    return Status;
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
            
    /* Allocate a Buffer for the Object Create Info */
    DPRINT("Allocating Create Buffer\n");
    ObjectCreateInfo = ExAllocatePoolWithTag(NonPagedPool, 
                                             sizeof(*ObjectCreateInfo),
                                             TAG('O','b','C', 'I'));

    /* Capture all the info */
    DPRINT("Capturing Create Info\n");
    Status = ObpCaptureObjectAttributes(ObjectAttributes,
                                        ObjectAttributesAccessMode,
                                        Type,
                                        ObjectCreateInfo,
                                        &ObjectName);
                                        
    if (NT_SUCCESS(Status))
    {
        /* Allocate the Object */
        DPRINT("Allocating: %wZ\n", &ObjectName);
        Status = ObpAllocateObject(ObjectCreateInfo,
                                   &ObjectName,
                                   Type, 
                                   OBJECT_ALLOC_SIZE(ObjectSize), 
                                   &Header);
                                   
        if (NT_SUCCESS(Status))
        {
            /* Return the Object */
            DPRINT("Returning Object\n");
            *Object = &Header->Body;
            
            /* Return to caller, leave the Capture Info Alive for ObInsert */
            return Status;
        }
        
        /* Release the Capture Info, we don't need it */
        DPRINT1("Allocation failed\n");
        ObpReleaseCapturedAttributes(ObjectCreateInfo);
        if (ObjectName.Buffer) ExFreePool(ObjectName.Buffer);
    }
     
    /* We failed, so release the Buffer */
    DPRINT1("Capture failed\n");
    ExFreePool(ObjectCreateInfo);
    return Status;
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
    POBJECT_HEADER ObjectHeader;
    ULONG InfoLength;
    PVOID Object;
    NTSTATUS Status;
    PAGED_CODE();

    Status = ObReferenceObjectByHandle(ObjectHandle,
                                       0,
                                       NULL,
                                       KeGetPreviousMode(),
                                       &Object,
                                       &HandleInfo);
    if (!NT_SUCCESS (Status)) return Status;

    ObjectHeader = BODY_TO_HEADER(Object);

    switch (ObjectInformationClass)
    {
    case ObjectBasicInformation:
        InfoLength = sizeof(OBJECT_BASIC_INFORMATION);
        if (Length != sizeof(OBJECT_BASIC_INFORMATION))
        {
            Status = STATUS_INFO_LENGTH_MISMATCH;
        }
        else
        {
            POBJECT_BASIC_INFORMATION BasicInfo;

            BasicInfo = (POBJECT_BASIC_INFORMATION)ObjectInformation;
            BasicInfo->Attributes = HandleInfo.HandleAttributes;
            BasicInfo->GrantedAccess = HandleInfo.GrantedAccess;
            BasicInfo->HandleCount = ObjectHeader->HandleCount;
            BasicInfo->PointerCount = ObjectHeader->PointerCount;
            BasicInfo->PagedPoolUsage = 0; /* FIXME*/
            BasicInfo->NonPagedPoolUsage = 0; /* FIXME*/
            BasicInfo->NameInformationLength = 0; /* FIXME*/
            BasicInfo->TypeInformationLength = 0; /* FIXME*/
            BasicInfo->SecurityDescriptorLength = 0; /* FIXME*/
            if (ObjectHeader->Type == ObSymbolicLinkType)
            {
                BasicInfo->CreateTime.QuadPart =
                    ((POBJECT_SYMBOLIC_LINK)Object)->CreationTime.QuadPart;
            }
            else
            {
                BasicInfo->CreateTime.QuadPart = (ULONGLONG)0;
            }
            Status = STATUS_SUCCESS;
        }
        break;

    case ObjectNameInformation:
        Status = ObQueryNameString(Object,
                                   (POBJECT_NAME_INFORMATION)ObjectInformation,
                                   Length,
                                   &InfoLength);
        break;

    case ObjectTypeInformation:
        Status = STATUS_NOT_IMPLEMENTED;
        break;

    case ObjectAllTypesInformation:
        Status = STATUS_NOT_IMPLEMENTED;
        break;

    case ObjectHandleInformation:
        InfoLength = sizeof (OBJECT_HANDLE_ATTRIBUTE_INFORMATION);
        if (Length != sizeof (OBJECT_HANDLE_ATTRIBUTE_INFORMATION))
        {
            Status = STATUS_INFO_LENGTH_MISMATCH;
        }
        else
        {
            Status = ObpQueryHandleAttributes(
                ObjectHandle,
                (POBJECT_HANDLE_ATTRIBUTE_INFORMATION)ObjectInformation);
        }
        break;

    default:
        Status = STATUS_INVALID_INFO_CLASS;
        break;
    }

    ObDereferenceObject (Object);

    if (ResultLength != NULL) *ResultLength = InfoLength;

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
    PVOID Object;
    NTSTATUS Status;
    PAGED_CODE();

    if (ObjectInformationClass != ObjectHandleInformation)
        return STATUS_INVALID_INFO_CLASS;

    if (Length != sizeof (OBJECT_HANDLE_ATTRIBUTE_INFORMATION))
        return STATUS_INFO_LENGTH_MISMATCH;

    Status = ObReferenceObjectByHandle(ObjectHandle,
                                       0,
                                       NULL,
                                       KeGetPreviousMode(),
                                       &Object,
                                       NULL);
    if (!NT_SUCCESS (Status)) return Status;

    Status = ObpSetHandleAttributes(ObjectHandle,
                                    (POBJECT_HANDLE_ATTRIBUTE_INFORMATION)
                                    ObjectInformation);

    ObDereferenceObject (Object);
    return Status;
}
/* EOF */

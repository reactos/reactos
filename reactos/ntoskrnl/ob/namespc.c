/* $Id$
 *
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/ob/namespc.c
 * PURPOSE:        Manages the system namespace
 *
 * PROGRAMMERS:    David Welch (welch@mcmail.com)
 */

/* INCLUDES ***************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, ObInit)
#endif


extern ULONG NtGlobalFlag;

/* GLOBALS ****************************************************************/

POBJECT_TYPE ObDirectoryType = NULL;
POBJECT_TYPE ObTypeObjectType = NULL;

POBJECT_DIRECTORY NameSpaceRoot = NULL;
POBJECT_DIRECTORY ObpTypeDirectoryObject = NULL;
 /* FIXME: Move this somewhere else once devicemap support is in */
PDEVICE_MAP ObSystemDeviceMap = NULL;
KEVENT ObpDefaultObject;

static GENERIC_MAPPING ObpDirectoryMapping = {
	STANDARD_RIGHTS_READ|DIRECTORY_QUERY|DIRECTORY_TRAVERSE,
	STANDARD_RIGHTS_WRITE|DIRECTORY_CREATE_OBJECT|DIRECTORY_CREATE_SUBDIRECTORY,
	STANDARD_RIGHTS_EXECUTE|DIRECTORY_QUERY|DIRECTORY_TRAVERSE,
	DIRECTORY_ALL_ACCESS};

static GENERIC_MAPPING ObpTypeMapping = {
	STANDARD_RIGHTS_READ,
	STANDARD_RIGHTS_WRITE,
	STANDARD_RIGHTS_EXECUTE,
	0x000F0001};

NTSTATUS
STDCALL
ObpAllocateObject(POBJECT_CREATE_INFORMATION ObjectCreateInfo,
                  PUNICODE_STRING ObjectName,
                  POBJECT_TYPE ObjectType,
                  ULONG ObjectSize,
                  PROS_OBJECT_HEADER *ObjectHeader);

/* FUNCTIONS **************************************************************/

/*
 * @implemented
 */
NTSTATUS STDCALL
ObReferenceObjectByName(PUNICODE_STRING ObjectPath,
			ULONG Attributes,
			PACCESS_STATE PassedAccessState,
			ACCESS_MASK DesiredAccess,
			POBJECT_TYPE ObjectType,
			KPROCESSOR_MODE AccessMode,
			PVOID ParseContext,
			PVOID* ObjectPtr)
{
   PVOID Object = NULL;
   UNICODE_STRING RemainingPath;
   UNICODE_STRING ObjectName;
   OBJECT_CREATE_INFORMATION ObjectCreateInfo;
   NTSTATUS Status;
   OBP_LOOKUP_CONTEXT Context;

   PAGED_CODE();

   /* Capture the name */
   DPRINT("Capturing Name\n");
   Status = ObpCaptureObjectName(&ObjectName, ObjectPath, AccessMode);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("ObpCaptureObjectName() failed (Status %lx)\n", Status);
	return Status;
     }

   /* 
    * Create a fake ObjectCreateInfo structure. Note that my upcoming
    * ObFindObject refactoring will remove the need for this hack.
    */
   ObjectCreateInfo.RootDirectory = NULL;
   ObjectCreateInfo.Attributes = Attributes;
     
   Status = ObFindObject(&ObjectCreateInfo,
                         &ObjectName,
			 &Object,
			 &RemainingPath,
			 ObjectType,
             &Context);

   if (ObjectName.Buffer) ExFreePool(ObjectName.Buffer);

   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   DPRINT("RemainingPath.Buffer '%S' Object %p\n", RemainingPath.Buffer, Object);

   if (RemainingPath.Buffer != NULL || Object == NULL)
     {
        DPRINT("Object %p\n", Object);
	*ObjectPtr = NULL;
	RtlFreeUnicodeString (&RemainingPath);
	return(STATUS_OBJECT_NAME_NOT_FOUND);
     }
   *ObjectPtr = Object;
   RtlFreeUnicodeString (&RemainingPath);
   return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							EXPORTED
 *	ObOpenObjectByName
 *
 * DESCRIPTION
 *	Obtain a handle to an existing object.
 *
 * ARGUMENTS
 *	ObjectAttributes
 *		...
 *	ObjectType
 *		...
 *	ParseContext
 *		...
 *	AccessMode
 *		...
 *	DesiredAccess
 *		...
 *	PassedAccessState
 *		...
 *	Handle
 *		Handle to close.
 *
 * RETURN VALUE
 * 	Status.
 *
 * @implemented
 */
NTSTATUS STDCALL
ObOpenObjectByName(IN POBJECT_ATTRIBUTES ObjectAttributes,
		   IN POBJECT_TYPE ObjectType,
		   IN OUT PVOID ParseContext,
		   IN KPROCESSOR_MODE AccessMode,
		   IN ACCESS_MASK DesiredAccess,
		   IN PACCESS_STATE PassedAccessState,
		   OUT PHANDLE Handle)
{
   UNICODE_STRING RemainingPath;
   PVOID Object = NULL;
   UNICODE_STRING ObjectName;
   OBJECT_CREATE_INFORMATION ObjectCreateInfo;
   NTSTATUS Status;
   OBP_LOOKUP_CONTEXT Context;

   PAGED_CODE();

   DPRINT("ObOpenObjectByName(...)\n");

    /* Capture all the info */
    DPRINT("Capturing Create Info\n");
    Status = ObpCaptureObjectAttributes(ObjectAttributes,
                                        AccessMode,
                                        ObjectType,
                                        &ObjectCreateInfo,
                                        &ObjectName);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("ObpCaptureObjectAttributes() failed (Status %lx)\n", Status);
	return Status;
     }
                                        
   Status = ObFindObject(&ObjectCreateInfo,
                         &ObjectName,
			 &Object,
			 &RemainingPath,
			 ObjectType,
             &Context);
   if (ObjectName.Buffer) ExFreePool(ObjectName.Buffer);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("ObFindObject() failed (Status %lx)\n", Status);
	goto Cleanup;
     }

   DPRINT("OBject: %p, Remaining Path: %wZ\n", Object, &RemainingPath);
   if (Object == NULL)
     {
       Status = STATUS_UNSUCCESSFUL;
       goto Cleanup;
     }
   if (RemainingPath.Buffer != NULL)
   {
      if (wcschr(RemainingPath.Buffer + 1, L'\\') == NULL)
         Status = STATUS_OBJECT_NAME_NOT_FOUND;
      else
         Status =STATUS_OBJECT_PATH_NOT_FOUND;
      goto Cleanup;
   }
   
   Status = ObpCreateHandle(Object,
			    DesiredAccess,
			    ObjectCreateInfo.Attributes,
			    Handle);

Cleanup:
   if (Object != NULL)
   {
       ObDereferenceObject(Object);
   }
   RtlFreeUnicodeString(&RemainingPath);
   ObpReleaseCapturedAttributes(&ObjectCreateInfo);

   return Status;
}

VOID
STDCALL
ObQueryDeviceMapInformation(PEPROCESS Process,
			    PPROCESS_DEVICEMAP_INFORMATION DeviceMapInfo)
{
	//KIRQL OldIrql ;

	/*
	 * FIXME: This is an ugly hack for now, to always return the System Device Map
	 * instead of returning the Process Device Map. Not important yet since we don't use it
	 */

	 /* FIXME: Acquire the DeviceMap Spinlock */
	 // KeAcquireSpinLock(DeviceMap->Lock, &OldIrql);

	 /* Make a copy */
	 DeviceMapInfo->Query.DriveMap = ObSystemDeviceMap->DriveMap;
	 RtlMoveMemory(DeviceMapInfo->Query.DriveType, ObSystemDeviceMap->DriveType, sizeof(ObSystemDeviceMap->DriveType));

	 /* FIXME: Release the DeviceMap Spinlock */
	 // KeReleasepinLock(DeviceMap->Lock, OldIrql);
}

VOID 
INIT_FUNCTION
ObInit(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Name;
    SECURITY_DESCRIPTOR SecurityDescriptor;
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    OBP_LOOKUP_CONTEXT Context;

    /* Initialize the security descriptor cache */
    ObpInitSdCache();

    /* Initialize the Default Event */
    KeInitializeEvent(&ObpDefaultObject, NotificationEvent, TRUE );

    /* Create the Type Type */
    DPRINT("Creating Type Type\n");
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Type");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.ValidAccessMask = OBJECT_TYPE_ALL_ACCESS;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    ObjectTypeInitializer.MaintainTypeList = TRUE;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.GenericMapping = ObpTypeMapping;
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(OBJECT_TYPE);
    ObpCreateTypeObject(&ObjectTypeInitializer, &Name, &ObTypeObjectType);
  
    /* Create the Directory Type */
    DPRINT("Creating Directory Type\n");
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Directory");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.ValidAccessMask = DIRECTORY_ALL_ACCESS;
    ObjectTypeInitializer.UseDefaultObject = FALSE;
    ObjectTypeInitializer.ParseProcedure = (OB_PARSE_METHOD)ObpParseDirectory;
    ObjectTypeInitializer.MaintainTypeList = FALSE;
    ObjectTypeInitializer.GenericMapping = ObpDirectoryMapping;
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(OBJECT_DIRECTORY);
    ObpCreateTypeObject(&ObjectTypeInitializer, &Name, &ObDirectoryType);

    /* Create security descriptor */
    RtlCreateSecurityDescriptor(&SecurityDescriptor,
                                SECURITY_DESCRIPTOR_REVISION1);
    RtlSetOwnerSecurityDescriptor(&SecurityDescriptor,
                                  SeAliasAdminsSid,
                                  FALSE);
    RtlSetGroupSecurityDescriptor(&SecurityDescriptor,
                                  SeLocalSystemSid,
                                  FALSE);
    RtlSetDaclSecurityDescriptor(&SecurityDescriptor,
                                 TRUE,
                                 SePublicDefaultDacl,
                                 FALSE);

    /* Create root directory */
    DPRINT("Creating Root Directory\n");    
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               OBJ_PERMANENT,
                               NULL,
                               &SecurityDescriptor);
    ObCreateObject(KernelMode,
                   ObDirectoryType,
                   &ObjectAttributes,
                   KernelMode,
                   NULL,
                   sizeof(OBJECT_DIRECTORY),
                   0,
                   0,
                   (PVOID*)&NameSpaceRoot);
    ObInsertObject((PVOID)NameSpaceRoot,
                   NULL,
                   DIRECTORY_ALL_ACCESS,
                   0,
                   NULL,
                   NULL);

    /* Create '\ObjectTypes' directory */
    RtlInitUnicodeString(&Name, L"\\ObjectTypes");
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_PERMANENT,
                               NULL,
                               &SecurityDescriptor);
    ObCreateObject(KernelMode,
                   ObDirectoryType,
                   &ObjectAttributes,
                   KernelMode,
                   NULL,
                   sizeof(OBJECT_DIRECTORY),
                   0,
                   0,
                   (PVOID*)&ObpTypeDirectoryObject);
    ObInsertObject((PVOID)ObpTypeDirectoryObject,
                   NULL,
                   DIRECTORY_ALL_ACCESS,
                   0,
                   NULL,
                   NULL);
    
    /* Insert the two objects we already created but couldn't add */
    /* NOTE: Uses TypeList & Creator Info in OB 2.0 */
    Context.Directory = ObpTypeDirectoryObject;
    Context.DirectoryLocked = TRUE;
    if (!ObpLookupEntryDirectory(ObpTypeDirectoryObject,
                                 &HEADER_TO_OBJECT_NAME(BODY_TO_HEADER(ObTypeObjectType))->Name,
                                 OBJ_CASE_INSENSITIVE,
                                 FALSE,
                                 &Context))
    {
        ObpInsertEntryDirectory(ObpTypeDirectoryObject, &Context, (POBJECT_HEADER)BODY_TO_HEADER(ObTypeObjectType));
    }
    if (!ObpLookupEntryDirectory(ObpTypeDirectoryObject,
                                 &HEADER_TO_OBJECT_NAME(BODY_TO_HEADER(ObDirectoryType))->Name,
                                 OBJ_CASE_INSENSITIVE,
                                 FALSE,
                                 &Context))
    {
        ObpInsertEntryDirectory(ObpTypeDirectoryObject, &Context, (POBJECT_HEADER)BODY_TO_HEADER(ObDirectoryType));
    }

    /* Create 'symbolic link' object type */
    ObInitSymbolicLinkImplementation();

    /* FIXME: Hack Hack! */
    ObSystemDeviceMap = ExAllocatePoolWithTag(NonPagedPool, sizeof(*ObSystemDeviceMap), TAG('O', 'b', 'D', 'm'));
    RtlZeroMemory(ObSystemDeviceMap, sizeof(*ObSystemDeviceMap));
}

NTSTATUS
STDCALL
ObpCreateTypeObject(POBJECT_TYPE_INITIALIZER ObjectTypeInitializer,
                    PUNICODE_STRING TypeName,
                    POBJECT_TYPE *ObjectType)
{
    PROS_OBJECT_HEADER Header;
    POBJECT_TYPE LocalObjectType;
    ULONG HeaderSize;
    NTSTATUS Status;

    DPRINT("ObpCreateTypeObject(ObjectType: %wZ)\n", TypeName);
    
    /* Allocate the Object */
    Status = ObpAllocateObject(NULL, 
                               TypeName,
                               ObTypeObjectType, 
                               OBJECT_ALLOC_SIZE(sizeof(OBJECT_TYPE)),
                               (PROS_OBJECT_HEADER*)&Header);
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
    HeaderSize = sizeof(ROS_OBJECT_HEADER) + sizeof(OBJECT_HEADER_NAME_INFO) +
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
        ObpInsertEntryDirectory(ObpTypeDirectoryObject, &Context, (POBJECT_HEADER)Header);
        ObReferenceObject(ObpTypeDirectoryObject);
    }

    *ObjectType = LocalObjectType;
    return Status;
} 

/* EOF */

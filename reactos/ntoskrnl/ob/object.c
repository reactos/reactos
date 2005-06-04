/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ob/object.c
 * PURPOSE:         Implements generic object managment functions
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 *                  Skywing (skywing@valhallalegends.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>


typedef struct _RETENTION_CHECK_PARAMS
{
  WORK_QUEUE_ITEM WorkItem;
  POBJECT_HEADER ObjectHeader;
} RETENTION_CHECK_PARAMS, *PRETENTION_CHECK_PARAMS;

/* FUNCTIONS ************************************************************/

NTSTATUS
STDCALL
ObpCaptureObjectName(IN OUT PUNICODE_STRING CapturedName,
                     IN PUNICODE_STRING ObjectName,
                     IN KPROCESSOR_MODE AccessMode)
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING LocalName = {}; /* <= GCC 4.0 + Optimizer */
    
    /* First Probe the String */
    DPRINT("ObpCaptureObjectName: %wZ\n", ObjectName);
    if (AccessMode != KernelMode)
    {
        DPRINT("Probing Struct\n");
        _SEH_TRY
        {
            /* FIXME: Explorer or win32 broken I think */
            #if 0
            ProbeForRead(ObjectName,
                         sizeof(UNICODE_STRING),
                         sizeof(USHORT));
            #endif
            LocalName = *ObjectName;
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        
        if (NT_SUCCESS(Status))
        {
            DPRINT("Probing OK\n");
             _SEH_TRY
            {
                #if 0
                DPRINT("Probing buffer\n");
                ProbeForRead(LocalName.Buffer,
                             LocalName.Length,
                             sizeof(USHORT));
                #endif
            }
            _SEH_HANDLE
            {
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
        }
        
        /* Fail if anything up to here died */
        if (!NT_SUCCESS(Status)) 
        {
            DPRINT1("Probing failed\n");
            return Status;
        }
    }
    else
    {
        LocalName = *ObjectName;
    }
    
    /* Make sure there really is a string */
    DPRINT("Probing OK\n");
    if (LocalName.Length)
    {
        /* Allocate a non-paged buffer for this string */
        DPRINT("Capturing String\n");
        CapturedName->Length = LocalName.Length;
        CapturedName->MaximumLength = LocalName.Length + sizeof(WCHAR);
        CapturedName->Buffer = ExAllocatePoolWithTag(NonPagedPool, 
                                                     CapturedName->MaximumLength,
                                                     TAG('O','b','N','m'));
                                                     
        /* Copy the string and null-terminate it */
        RtlMoveMemory(CapturedName->Buffer, LocalName.Buffer, LocalName.Length);
        CapturedName->Buffer[LocalName.Length / sizeof(WCHAR)] = UNICODE_NULL;
        DPRINT("String Captured: %p, %wZ\n", CapturedName, CapturedName);
    }
    
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
    
    /* Check if we got Oba */
    if (ObjectAttributes)
    {
        if (AccessMode != KernelMode)
        {
            DPRINT("Probing OBA\n");
            _SEH_TRY
            {
                /* FIXME: SMSS SENDS BULLSHIT. */
                #if 0
                ProbeForRead(ObjectAttributes,
                             sizeof(ObjectAttributes),
                             sizeof(ULONG));
                #endif
            }
            _SEH_HANDLE
            {
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
        }
        
        /* Validate the Size */
        DPRINT("Validating OBA\n");
        if (ObjectAttributes->Length != sizeof(OBJECT_ATTRIBUTES))
        {
            Status = STATUS_INVALID_PARAMETER;
        }

        /* Fail if SEH or Size Validation failed */
        if(!NT_SUCCESS(Status))
        {
            DPRINT1("ObpCaptureObjectAttributes failed to probe object attributes\n");
            goto fail;
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
                goto fail;
            }
            
            DPRINT("Probe done\n");
            ObjectCreateInfo->SecurityDescriptorCharge = 0; /* FIXME */
            ObjectCreateInfo->ProbeMode = AccessMode;
        }
        
        /* Validate the QoS */
        if (SecurityQos)
        {
            if (AccessMode != KernelMode)
            {
                DPRINT("Probing QoS\n");
                _SEH_TRY
                {
                    ProbeForRead(SecurityQos,
                                 sizeof(SECURITY_QUALITY_OF_SERVICE),
                                 sizeof(ULONG));
                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;
            }

            if(!NT_SUCCESS(Status))
            {
                DPRINT1("Unable to capture QoS!!!\n");
                goto fail;
            }
            
            ObjectCreateInfo->SecurityQualityOfService = *SecurityQos;
            ObjectCreateInfo->SecurityQos = &ObjectCreateInfo->SecurityQualityOfService;
        }
    }
    
    /* Clear Local Object Name */
    DPRINT("Clearing name\n");
    RtlZeroMemory(ObjectName, sizeof(UNICODE_STRING));
    
    /* Now check if the Object Attributes had an Object Name */
    if (LocalObjectName)
    {
        DPRINT("Name Buffer: %x\n", LocalObjectName->Buffer);
        Status = ObpCaptureObjectName(ObjectName,
                                      LocalObjectName,
                                      AccessMode);
    }
    else
    {
        /* He can't have specified a Root Directory */
        if (ObjectCreateInfo->RootDirectory)
        {
            DPRINT1("Invalid name\n");
            Status = STATUS_OBJECT_NAME_INVALID;
        }
    }
    
fail:
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to capture, cleaning up\n");
        ObpReleaseCapturedAttributes(ObjectCreateInfo);
    }
    
    DPRINT("Return to caller\n");
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


/**********************************************************************
 * NAME							PRIVATE
 * 	ObFindObject@16
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *	ObjectAttributes
 *
 *	ReturnedObject
 *
 *	RemainigPath
 *		Pointer to a unicode string that will contain the
 *		remaining path if the function returns successfully.
 *		The caller must free the buffer after use by calling
 *		RtlFreeUnicodeString ().
 *
 *	ObjectType
 *		Optional pointer to an object type. This is used to
 *		descide if a symbolic link object will be parsed or not.
 *
 * RETURN VALUE
 */
NTSTATUS
ObFindObject(POBJECT_CREATE_INFORMATION ObjectCreateInfo,
            PUNICODE_STRING ObjectName,
	     PVOID* ReturnedObject,
	     PUNICODE_STRING RemainingPath,
	     POBJECT_TYPE ObjectType)
{
  PVOID NextObject;
  PVOID CurrentObject;
  PVOID RootObject;
  POBJECT_HEADER CurrentHeader;
  NTSTATUS Status;
  PWSTR current;
  UNICODE_STRING PathString;
  ULONG Attributes;

  PAGED_CODE();

  DPRINT("ObFindObject(ObjectCreateInfo %x, ReturnedObject %x, "
	 "RemainingPath %x)\n",ObjectCreateInfo,ReturnedObject,RemainingPath);

  RtlInitUnicodeString (RemainingPath, NULL);

  if (ObjectCreateInfo->RootDirectory == NULL)
    {
      ObReferenceObjectByPointer(NameSpaceRoot,
				 DIRECTORY_TRAVERSE,
				 NULL,
				 UserMode);
      CurrentObject = NameSpaceRoot;
    }
  else
    {
      Status = ObReferenceObjectByHandle(ObjectCreateInfo->RootDirectory,
					 0,
					 NULL,
					 UserMode,
					 &CurrentObject,
					 NULL);
      if (!NT_SUCCESS(Status))
	{
	  return Status;
	}
    }

  if (ObjectName->Length == 0 ||
      ObjectName->Buffer[0] == UNICODE_NULL)
    {
      *ReturnedObject = CurrentObject;
      return STATUS_SUCCESS;
    }

  if (ObjectCreateInfo->RootDirectory == NULL &&
      ObjectName->Buffer[0] != L'\\')
    {
      ObDereferenceObject (CurrentObject);
      DPRINT1("failed\n");
      return STATUS_UNSUCCESSFUL;
    }

  /* Create a zero-terminated copy of the object name */
  PathString.Length = ObjectName->Length;
  PathString.MaximumLength = ObjectName->Length + sizeof(WCHAR);
  PathString.Buffer = ExAllocatePool (NonPagedPool,
				      PathString.MaximumLength);
  if (PathString.Buffer == NULL)
    {
      ObDereferenceObject (CurrentObject);
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  RtlCopyMemory (PathString.Buffer,
		 ObjectName->Buffer,
		 ObjectName->Length);
  PathString.Buffer[PathString.Length / sizeof(WCHAR)] = UNICODE_NULL;

  current = PathString.Buffer;

  RootObject = CurrentObject;
  Attributes = ObjectCreateInfo->Attributes;
  if (ObjectType == ObSymbolicLinkType)
    Attributes |= OBJ_OPENLINK;

  while (TRUE)
    {
	DPRINT("current %S\n",current);
	CurrentHeader = BODY_TO_HEADER(CurrentObject);

	DPRINT("Current ObjectType %wZ\n",
	       &CurrentHeader->Type->Name);

	if (CurrentHeader->Type->TypeInfo.ParseProcedure == NULL)
	  {
	     DPRINT("Current object can't parse\n");
	     break;
	  }
	Status = CurrentHeader->Type->TypeInfo.ParseProcedure(CurrentObject,
						  &NextObject,
						  &PathString,
						  &current,
						  Attributes);
	if (Status == STATUS_REPARSE)
	  {
	     /* reparse the object path */
	     NextObject = NameSpaceRoot;
	     current = PathString.Buffer;

	     ObReferenceObjectByPointer(NextObject,
					DIRECTORY_TRAVERSE,
					NULL,
					UserMode);
	  }

	if (NextObject == NULL)
	  {
	     break;
	  }
	ObDereferenceObject(CurrentObject);
	CurrentObject = NextObject;
    }

  if (current)
  {
     RtlpCreateUnicodeString (RemainingPath, current, NonPagedPool);
  }

  RtlFreeUnicodeString (&PathString);
  *ReturnedObject = CurrentObject;

  return STATUS_SUCCESS;
}


/**********************************************************************
 * NAME							EXPORTED
 * 	ObQueryNameString@16
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @implemented
 */
NTSTATUS STDCALL
ObQueryNameString (IN PVOID Object,
		   OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
		   IN ULONG Length,
		   OUT PULONG ReturnLength)
{
  POBJECT_NAME_INFORMATION LocalInfo;
  POBJECT_HEADER ObjectHeader;
  ULONG LocalReturnLength;
  NTSTATUS Status;

  PAGED_CODE();

  *ReturnLength = 0;

  if (Length < sizeof(OBJECT_NAME_INFORMATION) + sizeof(WCHAR))
    return STATUS_INVALID_BUFFER_SIZE;

  ObjectNameInfo->Name.MaximumLength = (USHORT)(Length - sizeof(OBJECT_NAME_INFORMATION));
  ObjectNameInfo->Name.Length = 0;
  ObjectNameInfo->Name.Buffer =
    (PWCHAR)((ULONG_PTR)ObjectNameInfo + sizeof(OBJECT_NAME_INFORMATION));
  ObjectNameInfo->Name.Buffer[0] = 0;

  ObjectHeader = BODY_TO_HEADER(Object);

  if (ObjectHeader->Type != NULL &&
      ObjectHeader->Type->TypeInfo.QueryNameProcedure != NULL)
    {
      DPRINT ("Calling %x\n", ObjectHeader->Type->TypeInfo.QueryNameProcedure);
      Status = ObjectHeader->Type->TypeInfo.QueryNameProcedure (Object,
						    ObjectNameInfo,
						    Length,
						    ReturnLength);
    }
  else if (HEADER_TO_OBJECT_NAME(ObjectHeader)->Name.Length > 0 && HEADER_TO_OBJECT_NAME(ObjectHeader)->Name.Buffer != NULL)
    {
      DPRINT ("Object does not have a 'QueryName' function\n");

      if (HEADER_TO_OBJECT_NAME(ObjectHeader)->Directory == NameSpaceRoot)
	{
	  DPRINT ("Reached the root directory\n");
	  ObjectNameInfo->Name.Length = 0;
	  ObjectNameInfo->Name.Buffer[0] = 0;
	  Status = STATUS_SUCCESS;
	}
      else if (HEADER_TO_OBJECT_NAME(ObjectHeader)->Directory != NULL)
	{
	  LocalInfo = ExAllocatePool (NonPagedPool,
				      sizeof(OBJECT_NAME_INFORMATION) +
				      MAX_PATH * sizeof(WCHAR));
	  if (LocalInfo == NULL)
	    return STATUS_INSUFFICIENT_RESOURCES;

	  Status = ObQueryNameString (HEADER_TO_OBJECT_NAME(ObjectHeader)->Directory,
				      LocalInfo,
				      MAX_PATH * sizeof(WCHAR),
				      &LocalReturnLength);
	  if (!NT_SUCCESS (Status))
	    {
	      ExFreePool (LocalInfo);
	      return Status;
	    }

	  Status = RtlAppendUnicodeStringToString (&ObjectNameInfo->Name,
						   &LocalInfo->Name);

	  ExFreePool (LocalInfo);

	  if (!NT_SUCCESS (Status))
	    return Status;
	}

      DPRINT ("Object path %wZ\n", &HEADER_TO_OBJECT_NAME(ObjectHeader)->Name);
      Status = RtlAppendUnicodeToString (&ObjectNameInfo->Name,
					 L"\\");
      if (!NT_SUCCESS (Status))
	return Status;

      Status = RtlAppendUnicodeStringToString (&ObjectNameInfo->Name,
					       &HEADER_TO_OBJECT_NAME(ObjectHeader)->Name);
    }
  else
    {
      DPRINT ("Object is unnamed\n");

      ObjectNameInfo->Name.MaximumLength = 0;
      ObjectNameInfo->Name.Length = 0;
      ObjectNameInfo->Name.Buffer = NULL;

      Status = STATUS_SUCCESS;
    }

  if (NT_SUCCESS (Status))
    {
      ObjectNameInfo->Name.MaximumLength =
	(ObjectNameInfo->Name.Length) ? ObjectNameInfo->Name.Length + sizeof(WCHAR) : 0;
      *ReturnLength =
	sizeof(OBJECT_NAME_INFORMATION) + ObjectNameInfo->Name.MaximumLength;
      DPRINT ("Returned object path: %wZ\n", &ObjectNameInfo->Name);
    }

  return Status;
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

/**********************************************************************
 * NAME							EXPORTED
 * 	ObCreateObject@36
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *	Status
 *
 * @implemented
 */
NTSTATUS 
STDCALL
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
                                        AccessMode,
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

/*
 * FUNCTION: Increments the pointer reference count for a given object
 * ARGUMENTS:
 *         ObjectBody = Object's body
 *         DesiredAccess = Desired access to the object
 *         ObjectType = Points to the object type structure
 *         AccessMode = Type of access check to perform
 * RETURNS: Status
 *
 * @implemented
 */
NTSTATUS STDCALL
ObReferenceObjectByPointer(IN PVOID Object,
			   IN ACCESS_MASK DesiredAccess,
			   IN POBJECT_TYPE ObjectType,
			   IN KPROCESSOR_MODE AccessMode)
{
   POBJECT_HEADER Header;

   /* NOTE: should be possible to reference an object above APC_LEVEL! */

   DPRINT("ObReferenceObjectByPointer(Object %x, ObjectType %x)\n",
	  Object,ObjectType);

   Header = BODY_TO_HEADER(Object);

   if (ObjectType != NULL && Header->Type != ObjectType)
     {
	DPRINT("Failed %p (type was %x %wZ) should be %x %wZ\n",
		Header,
		Header->Type,
		&BODY_TO_HEADER(Header->Type)->NameInfo,
		ObjectType,
		&BODY_TO_HEADER(ObjectType)->NameInfo);
	return(STATUS_UNSUCCESSFUL);
     }
   if (Header->Type == PsProcessType)
     {
	DPRINT("Ref p 0x%x PointerCount %d type %x ",
		Object, Header->PointerCount, PsProcessType);
	DPRINT("eip %x\n", ((PULONG)&Object)[-1]);
     }
   if (Header->Type == PsThreadType)
     {
	DPRINT("Deref t 0x%x with PointerCount %d type %x ",
		Object, Header->PointerCount, PsThreadType);
	DPRINT("eip %x\n", ((PULONG)&Object)[-1]);
     }

   if (Header->PointerCount == 0 && !(Header->Flags & OB_FLAG_PERMANENT))
   {
      if (Header->Type == PsProcessType)
        {
	  return STATUS_PROCESS_IS_TERMINATING;
	}
      if (Header->Type == PsThreadType)
        {
	  return STATUS_THREAD_IS_TERMINATING;
	}
      return(STATUS_UNSUCCESSFUL);
   }

   if (1 == InterlockedIncrement(&Header->PointerCount) && !(Header->Flags & OB_FLAG_PERMANENT))
   {
      KEBUGCHECK(0);
   }

   return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
ObOpenObjectByPointer(IN POBJECT Object,
		      IN ULONG HandleAttributes,
		      IN PACCESS_STATE PassedAccessState,
		      IN ACCESS_MASK DesiredAccess,
		      IN POBJECT_TYPE ObjectType,
		      IN KPROCESSOR_MODE AccessMode,
		      OUT PHANDLE Handle)
{
   NTSTATUS Status;

   PAGED_CODE();

   DPRINT("ObOpenObjectByPointer()\n");

   Status = ObReferenceObjectByPointer(Object,
				       0,
				       ObjectType,
				       AccessMode);
   if (!NT_SUCCESS(Status))
     {
	return Status;
     }

   Status = ObpCreateHandle(PsGetCurrentProcess(),
			   Object,
			   DesiredAccess,
			   (BOOLEAN)(HandleAttributes & OBJ_INHERIT),
			   Handle);

   ObDereferenceObject(Object);

   return STATUS_SUCCESS;
}


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


STATIC NTSTATUS
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


/**********************************************************************
 * NAME							EXPORTED
 * 	ObfReferenceObject@4
 *
 * DESCRIPTION
 *	Increments a given object's reference count and performs
 *	retention checks.
 *
 * ARGUMENTS
 *  ObjectBody = Body of the object.
 *
 * RETURN VALUE
 * 	None.
 *
 * @implemented
 */
VOID FASTCALL
ObfReferenceObject(IN PVOID Object)
{
  POBJECT_HEADER Header;

  ASSERT(Object);

  Header = BODY_TO_HEADER(Object);

  /* No one should be referencing an object once we are deleting it. */
  if (InterlockedIncrement(&Header->PointerCount) == 1 && !(Header->Flags & OB_FLAG_PERMANENT))
  {
     KEBUGCHECK(0);
  }

}

/**********************************************************************
 * NAME							EXPORTED
 *	ObfDereferenceObject@4
 *
 * DESCRIPTION
 *	Decrements a given object's reference count and performs
 *	retention checks.
 *
 * ARGUMENTS
 *	ObjectBody = Body of the object.
 *
 * RETURN VALUE
 * 	None.
 *
 * @implemented
 */
VOID FASTCALL
ObfDereferenceObject(IN PVOID Object)
{
  POBJECT_HEADER Header;
  LONG NewPointerCount;
  BOOL Permanent;

  ASSERT(Object);

  /* Extract the object header. */
  Header = BODY_TO_HEADER(Object);
  Permanent = Header->Flags & OB_FLAG_PERMANENT;

  /*
     Drop our reference and get the new count so we can tell if this was the
     last reference.
  */
  NewPointerCount = InterlockedDecrement(&Header->PointerCount);
  DPRINT("ObfDereferenceObject(0x%x)==%d\n", Object, NewPointerCount);
  ASSERT(NewPointerCount >= 0);

  /* Check whether the object can now be deleted. */
  if (NewPointerCount == 0 &&
      !Permanent)
    {
      ObpDeleteObjectDpcLevel(Header, NewPointerCount);
    }
}

VOID
FASTCALL
ObInitializeFastReference(IN PEX_FAST_REF FastRef,
                          PVOID Object)
{
    /* FIXME: Fast Referencing is Unimplemented */
    FastRef->Object = Object;
}


PVOID
FASTCALL
ObFastReferenceObject(IN PEX_FAST_REF FastRef)
{
    /* FIXME: Fast Referencing is Unimplemented */

    /* Do a normal Reference */
    ObReferenceObject(FastRef->Object);

    /* Return the Object */
    return FastRef->Object;
}

VOID
FASTCALL
ObFastDereferenceObject(IN PEX_FAST_REF FastRef,
                        PVOID Object)
{
    /* FIXME: Fast Referencing is Unimplemented */

    /* Do a normal Dereference */
    ObDereferenceObject(FastRef->Object);
}

PVOID
FASTCALL
ObFastReplaceObject(IN PEX_FAST_REF FastRef,
                    PVOID Object)
{
    PVOID OldObject = FastRef->Object;

    /* FIXME: Fast Referencing is Unimplemented */
    FastRef->Object = Object;

    /* Do a normal Dereference */
    ObDereferenceObject(OldObject);

    /* Return old Object*/
    return OldObject;
}

/**********************************************************************
 * NAME							EXPORTED
 *	ObGetObjectPointerCount@4
 *
 * DESCRIPTION
 *	Retrieves the pointer(reference) count of the given object.
 *
 * ARGUMENTS
 *	ObjectBody = Body of the object.
 *
 * RETURN VALUE
 * 	Reference count.
 *
 * @implemented
 */
ULONG STDCALL
ObGetObjectPointerCount(PVOID Object)
{
  POBJECT_HEADER Header;

  PAGED_CODE();

  ASSERT(Object);
  Header = BODY_TO_HEADER(Object);

  return Header->PointerCount;
}


/**********************************************************************
 * NAME							INTERNAL
 *	ObGetObjectHandleCount@4
 *
 * DESCRIPTION
 *	Retrieves the handle count of the given object.
 *
 * ARGUMENTS
 *	ObjectBody = Body of the object.
 *
 * RETURN VALUE
 * 	Reference count.
 */
ULONG
ObGetObjectHandleCount(PVOID Object)
{
  POBJECT_HEADER Header;

  PAGED_CODE();

  ASSERT(Object);
  Header = BODY_TO_HEADER(Object);

  return Header->HandleCount;
}


/**********************************************************************
 * NAME							EXPORTED
 *	ObDereferenceObject@4
 *
 * DESCRIPTION
 *	Decrements a given object's reference count and performs
 *	retention checks.
 *
 * ARGUMENTS
 *	ObjectBody = Body of the object.
 *
 * RETURN VALUE
 * 	None.
 *
 * @implemented
 */

#ifdef ObDereferenceObject
#undef ObDereferenceObject
#endif

VOID STDCALL
ObDereferenceObject(IN PVOID Object)
{
  ObfDereferenceObject(Object);
}

/* EOF */

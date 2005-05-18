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
    UNICODE_STRING LocalName;
    
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
        if (!NT_SUCCESS(Status)) return Status;
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
        #if 0 /*FIXME: FIX KERNEL32 and STUFF!!! */
        /* He can't have specified a Root Directory */
        if (ObjectCreateInfo->RootDirectory)
        {
            DPRINT1("Invalid name\n");
            Status = STATUS_OBJECT_NAME_INVALID;
        }
        #endif
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
	       &CurrentHeader->ObjectType->Name);

	if (CurrentHeader->ObjectType->TypeInfo.ParseProcedure == NULL)
	  {
	     DPRINT("Current object can't parse\n");
	     break;
	  }
	Status = CurrentHeader->ObjectType->TypeInfo.ParseProcedure(CurrentObject,
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

  if (ObjectHeader->ObjectType != NULL &&
      ObjectHeader->ObjectType->TypeInfo.QueryNameProcedure != NULL)
    {
      DPRINT ("Calling %x\n", ObjectHeader->ObjectType->TypeInfo.QueryNameProcedure);
      Status = ObjectHeader->ObjectType->TypeInfo.QueryNameProcedure (Object,
						    ObjectNameInfo,
						    Length,
						    ReturnLength);
    }
  else if (ObjectHeader->NameInfo->Name.Length > 0 && ObjectHeader->NameInfo->Name.Buffer != NULL)
    {
      DPRINT ("Object does not have a 'QueryName' function\n");

      if (ObjectHeader->NameInfo->Directory == NameSpaceRoot)
	{
	  DPRINT ("Reached the root directory\n");
	  ObjectNameInfo->Name.Length = 0;
	  ObjectNameInfo->Name.Buffer[0] = 0;
	  Status = STATUS_SUCCESS;
	}
      else if (ObjectHeader->NameInfo->Directory != NULL)
	{
	  LocalInfo = ExAllocatePool (NonPagedPool,
				      sizeof(OBJECT_NAME_INFORMATION) +
				      MAX_PATH * sizeof(WCHAR));
	  if (LocalInfo == NULL)
	    return STATUS_INSUFFICIENT_RESOURCES;

	  Status = ObQueryNameString (ObjectHeader->NameInfo->Directory,
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

      DPRINT ("Object path %wZ\n", &ObjectHeader->NameInfo->Name);
      Status = RtlAppendUnicodeToString (&ObjectNameInfo->Name,
					 L"\\");
      if (!NT_SUCCESS (Status))
	return Status;

      Status = RtlAppendUnicodeStringToString (&ObjectNameInfo->Name,
					       &ObjectHeader->NameInfo->Name);
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
    POBJECT_HEADER_NAME_INFO ObjectNameInfo;
    POOL_TYPE PoolType;
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
    
    /* Allocate memory for the Object */
    Header = ExAllocatePoolWithTag(PoolType, ObjectSize, Tag);
    if (!Header) {
        DPRINT1("Not enough memory!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    /* Initialize the object header */
    RtlZeroMemory(Header, ObjectSize);
    DPRINT("Initalizing header %p\n", Header);
    Header->HandleCount = 0;
    Header->RefCount = 1;
    Header->ObjectType = ObjectType;
    if (ObjectCreateInfo && ObjectCreateInfo->Attributes & OBJ_PERMANENT)
    {
        Header->Permanent = TRUE;
    }
    if (ObjectCreateInfo && ObjectCreateInfo->Attributes & OBJ_INHERIT)
    {
        Header->Inherit = TRUE;
    }
       
    /* Initialize the Object Name Info [part of header in OB 2.0] */
    ObjectNameInfo = ExAllocatePool(PoolType, ObjectSize);
    ObjectNameInfo->Name = *ObjectName;
    ObjectNameInfo->Directory = NULL;
    
    /* Link stuff to Object Header */
    Header->NameInfo = ObjectNameInfo;
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
            *Object = HEADER_TO_BODY(Header);
            
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

   if (ObjectType != NULL && Header->ObjectType != ObjectType)
     {
	DPRINT("Failed %x (type was %x %S) should be %x %S\n",
		Header,
		Header->ObjectType,
		Header->ObjectType->TypeName.Buffer,
		ObjectType,
		ObjectType->TypeName.Buffer);
	return(STATUS_UNSUCCESSFUL);
     }
   if (Header->ObjectType == PsProcessType)
     {
	DPRINT("Ref p 0x%x refcount %d type %x ",
		Object, Header->RefCount, PsProcessType);
	DPRINT("eip %x\n", ((PULONG)&Object)[-1]);
     }
   if (Header->ObjectType == PsThreadType)
     {
	DPRINT("Deref t 0x%x with refcount %d type %x ",
		Object, Header->RefCount, PsThreadType);
	DPRINT("eip %x\n", ((PULONG)&Object)[-1]);
     }

   if (Header->RefCount == 0 && !Header->Permanent)
   {
      if (Header->ObjectType == PsProcessType)
        {
	  return STATUS_PROCESS_IS_TERMINATING;
	}
      if (Header->ObjectType == PsThreadType)
        {
	  return STATUS_THREAD_IS_TERMINATING;
	}
      return(STATUS_UNSUCCESSFUL);
   }

   if (1 == InterlockedIncrement(&Header->RefCount) && !Header->Permanent)
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
  DPRINT("ObpDeleteObject(Header %p)\n", Header);
  if (KeGetCurrentIrql() != PASSIVE_LEVEL)
    {
      DPRINT("ObpDeleteObject called at an unsupported IRQL.  Use ObpDeleteObjectDpcLevel instead.\n");
      KEBUGCHECK(0);
    }

  if (Header->SecurityDescriptor != NULL)
    {
      ObpRemoveSecurityDescriptor(Header->SecurityDescriptor);
    }
    
  if (Header->NameInfo && Header->NameInfo->Name.Buffer)
  {
      ExFreePool(Header->NameInfo->Name.Buffer);
  }

  if (Header->ObjectType != NULL &&
      Header->ObjectType->TypeInfo.DeleteProcedure != NULL)
    {
      Header->ObjectType->TypeInfo.DeleteProcedure(HEADER_TO_BODY(Header));
    }

  DPRINT("ObPerformRetentionChecks() = Freeing object\n");
  ExFreePool(Header);

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
  Tag = Params->ObjectHeader->ObjectType->Tag; */
  ObpDeleteObject(Params->ObjectHeader);
  ExFreePool(Params);
  /* ExFreePoolWithTag(Params, Tag); */
}


STATIC NTSTATUS
ObpDeleteObjectDpcLevel(IN POBJECT_HEADER ObjectHeader,
			IN LONG OldRefCount)
{
#if 0
  if (ObjectHeader->RefCount < 0)
    {
      CPRINT("Object %p/%p has invalid reference count (%d)\n",
	     ObjectHeader, HEADER_TO_BODY(ObjectHeader),
	     ObjectHeader->RefCount);
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
				ObjectHeader->ObjectType->Key);
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
  if (InterlockedIncrement(&Header->RefCount) == 1 && !Header->Permanent)
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
  LONG NewRefCount;
  BOOL Permanent;

  ASSERT(Object);

  /* Extract the object header. */
  Header = BODY_TO_HEADER(Object);
  Permanent = Header->Permanent;

  /*
     Drop our reference and get the new count so we can tell if this was the
     last reference.
  */
  NewRefCount = InterlockedDecrement(&Header->RefCount);
  DPRINT("ObfDereferenceObject(0x%x)==%d\n", Object, NewRefCount);
  ASSERT(NewRefCount >= 0);

  /* Check whether the object can now be deleted. */
  if (NewRefCount == 0 &&
      !Permanent)
    {
      ObpDeleteObjectDpcLevel(Header, NewRefCount);
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

  return Header->RefCount;
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

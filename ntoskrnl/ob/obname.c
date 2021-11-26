/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ob/obname.c
 * PURPOSE:         Manages all functions related to the Object Manager name-
 *                  space, such as finding objects or querying their names.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl
 *                  Thomas Weidenmueller (w3seek@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

BOOLEAN ObpCaseInsensitive = TRUE;
POBJECT_DIRECTORY ObpRootDirectoryObject;
POBJECT_DIRECTORY ObpTypeDirectoryObject;

/* DOS Device Prefix \??\ and \?? */
ALIGNEDNAME ObpDosDevicesShortNamePrefix = {{L'\\',L'?',L'?',L'\\'}};
ALIGNEDNAME ObpDosDevicesShortNameRoot = {{L'\\',L'?',L'?',L'\0'}};
UNICODE_STRING ObpDosDevicesShortName =
{
    sizeof(ObpDosDevicesShortNamePrefix),
    sizeof(ObpDosDevicesShortNamePrefix),
    (PWSTR)&ObpDosDevicesShortNamePrefix
};

WCHAR ObpUnsecureGlobalNamesBuffer[128] = {0};
ULONG ObpUnsecureGlobalNamesLength = sizeof(ObpUnsecureGlobalNamesBuffer);

/* PRIVATE FUNCTIONS *********************************************************/

CODE_SEG("INIT")
NTSTATUS
NTAPI
ObpGetDosDevicesProtection(OUT PSECURITY_DESCRIPTOR SecurityDescriptor)
{
    PACL Dacl;
    ULONG AclSize;
    NTSTATUS Status;

    /* Initialize the SD */
    Status = RtlCreateSecurityDescriptor(SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    ASSERT(NT_SUCCESS(Status));

    if (ObpProtectionMode & 1)
    {
        AclSize = sizeof(ACL) +
                  sizeof(ACE) + RtlLengthSid(SeWorldSid) +
                  sizeof(ACE) + RtlLengthSid(SeLocalSystemSid) +
                  sizeof(ACE) + RtlLengthSid(SeWorldSid) +
                  sizeof(ACE) + RtlLengthSid(SeAliasAdminsSid) +
                  sizeof(ACE) + RtlLengthSid(SeLocalSystemSid) +
                  sizeof(ACE) + RtlLengthSid(SeCreatorOwnerSid);

        /* Allocate the ACL */
        Dacl = ExAllocatePoolWithTag(PagedPool, AclSize, 'lcaD');
        if (Dacl == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Initialize the DACL */
        Status = RtlCreateAcl(Dacl, AclSize, ACL_REVISION);
        ASSERT(NT_SUCCESS(Status));

        /* Add the ACEs */
        Status = RtlAddAccessAllowedAce(Dacl,
                                        ACL_REVISION,
                                        GENERIC_READ | GENERIC_EXECUTE,
                                        SeWorldSid);
        ASSERT(NT_SUCCESS(Status));

        Status = RtlAddAccessAllowedAce(Dacl,
                                        ACL_REVISION,
                                        GENERIC_ALL,
                                        SeLocalSystemSid);
        ASSERT(NT_SUCCESS(Status));

        Status = RtlAddAccessAllowedAceEx(Dacl,
                                          ACL_REVISION,
                                          INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE,
                                          GENERIC_EXECUTE,
                                          SeWorldSid);
        ASSERT(NT_SUCCESS(Status));

        Status = RtlAddAccessAllowedAceEx(Dacl,
                                          ACL_REVISION,
                                          INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE,
                                          GENERIC_ALL,
                                          SeAliasAdminsSid);
        ASSERT(NT_SUCCESS(Status));

        Status = RtlAddAccessAllowedAceEx(Dacl,
                                          ACL_REVISION,
                                          INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE,
                                          GENERIC_ALL,
                                          SeLocalSystemSid);
        ASSERT(NT_SUCCESS(Status));

        Status = RtlAddAccessAllowedAceEx(Dacl,
                                          ACL_REVISION,
                                          INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE,
                                          GENERIC_ALL,
                                          SeCreatorOwnerSid);
        ASSERT(NT_SUCCESS(Status));
    }
    else
    {
        AclSize = sizeof(ACL) +
                  sizeof(ACE) + RtlLengthSid(SeLocalSystemSid) +
                  sizeof(ACE) + RtlLengthSid(SeWorldSid) +
                  sizeof(ACE) + RtlLengthSid(SeLocalSystemSid);

        /* Allocate the ACL */
        Dacl = ExAllocatePoolWithTag(PagedPool, AclSize, 'lcaD');
        if (Dacl == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Initialize the DACL */
        Status = RtlCreateAcl(Dacl, AclSize, ACL_REVISION);
        ASSERT(NT_SUCCESS(Status));

        /* Add the ACEs */
        Status = RtlAddAccessAllowedAce(Dacl,
                                        ACL_REVISION,
                                        GENERIC_READ | GENERIC_EXECUTE | GENERIC_WRITE,
                                        SeWorldSid);
        ASSERT(NT_SUCCESS(Status));

        Status = RtlAddAccessAllowedAce(Dacl,
                                        ACL_REVISION,
                                        GENERIC_ALL,
                                        SeLocalSystemSid);
        ASSERT(NT_SUCCESS(Status));

        Status = RtlAddAccessAllowedAceEx(Dacl,
                                          ACL_REVISION,
                                          INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE,
                                          GENERIC_ALL,
                                          SeWorldSid);
        ASSERT(NT_SUCCESS(Status));
    }

    /* Attach the DACL to the SD */
    Status = RtlSetDaclSecurityDescriptor(SecurityDescriptor, TRUE, Dacl, FALSE);
    ASSERT(NT_SUCCESS(Status));

    return STATUS_SUCCESS;
}

CODE_SEG("INIT")
VOID
NTAPI
ObpFreeDosDevicesProtection(OUT PSECURITY_DESCRIPTOR SecurityDescriptor)
{
    PACL Dacl;
    NTSTATUS Status;
    BOOLEAN DaclPresent, DaclDefaulted;

    Status = RtlGetDaclSecurityDescriptor(SecurityDescriptor, &DaclPresent, &Dacl, &DaclDefaulted);
    ASSERT(NT_SUCCESS(Status));
    ASSERT(DaclPresent);
    ASSERT(Dacl != NULL);
    ExFreePoolWithTag(Dacl, 'lcaD');
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
ObpCreateDosDevicesDirectory(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING RootName, TargetName, LinkName;
    HANDLE Handle, SymHandle;
    SECURITY_DESCRIPTOR DosDevicesSD;
    NTSTATUS Status;

    /*
     * Enable LUID mappings only if not explicitely disabled
     * and if protection mode is set
     */
    if (ObpProtectionMode == 0 || ObpLUIDDeviceMapsDisabled != 0)
        ObpLUIDDeviceMapsEnabled = 0;
    else
        ObpLUIDDeviceMapsEnabled = 1;

    /* Create a custom security descriptor for the global DosDevices directory */
    Status = ObpGetDosDevicesProtection(&DosDevicesSD);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Create the global DosDevices directory \?? */
    RtlInitUnicodeString(&RootName, L"\\GLOBAL??");
    InitializeObjectAttributes(&ObjectAttributes,
                               &RootName,
                               OBJ_PERMANENT,
                               NULL,
                               &DosDevicesSD);
    Status = NtCreateDirectoryObject(&Handle,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Create the system device map */
    Status = ObSetDeviceMap(NULL, Handle);
    if (!NT_SUCCESS(Status))
        goto done;

    /*
     * Initialize the \??\GLOBALROOT symbolic link
     * pointing to the root directory \ .
     */
    RtlInitUnicodeString(&LinkName, L"GLOBALROOT");
    RtlInitUnicodeString(&TargetName, L"");
    InitializeObjectAttributes(&ObjectAttributes,
                               &LinkName,
                               OBJ_PERMANENT,
                               Handle,
                               &DosDevicesSD);
    Status = NtCreateSymbolicLinkObject(&SymHandle,
                                        SYMBOLIC_LINK_ALL_ACCESS,
                                        &ObjectAttributes,
                                        &TargetName);
    if (NT_SUCCESS(Status)) NtClose(SymHandle);

    /*
     * Initialize the \??\Global symbolic link pointing to the global
     * DosDevices directory \?? . It is used to access the global \??
     * by user-mode components which, by default, use a per-session
     * DosDevices directory.
     */
    RtlInitUnicodeString(&LinkName, L"Global");
    InitializeObjectAttributes(&ObjectAttributes,
                               &LinkName,
                               OBJ_PERMANENT,
                               Handle,
                               &DosDevicesSD);
    Status = NtCreateSymbolicLinkObject(&SymHandle,
                                        SYMBOLIC_LINK_ALL_ACCESS,
                                        &ObjectAttributes,
                                        &RootName);
    if (NT_SUCCESS(Status)) NtClose(SymHandle);

    /* Close the directory handle */
    NtClose(Handle);
    if (!NT_SUCCESS(Status))
        goto done;

    /*
     * Initialize the \DosDevices symbolic link pointing to the global
     * DosDevices directory \?? , for backward compatibility with
     * Windows NT-2000 systems.
     */
    RtlInitUnicodeString(&LinkName, L"\\DosDevices");
    RtlInitUnicodeString(&RootName, (PCWSTR)&ObpDosDevicesShortNameRoot);
    InitializeObjectAttributes(&ObjectAttributes,
                               &LinkName,
                               OBJ_PERMANENT,
                               NULL,
                               &DosDevicesSD);
    Status = NtCreateSymbolicLinkObject(&SymHandle,
                                        SYMBOLIC_LINK_ALL_ACCESS,
                                        &ObjectAttributes,
                                        &RootName);
    if (NT_SUCCESS(Status)) NtClose(SymHandle);

done:
    ObpFreeDosDevicesProtection(&DosDevicesSD);

    /* Return status */
    return Status;
}

/*++
* @name ObpDeleteNameCheck
*
*     The ObpDeleteNameCheck routine checks if a named object should be
*     removed from the object directory namespace.
*
* @param Object
*        Pointer to the object to check for possible removal.
*
* @return None.
*
* @remarks An object is removed if the following 4 criteria are met:
*          1) The object has 0 handles open
*          2) The object is in the directory namespace and has a name
*          3) The object is not permanent
*
*--*/
VOID
NTAPI
ObpDeleteNameCheck(IN PVOID Object)
{
    POBJECT_HEADER ObjectHeader;
    OBP_LOOKUP_CONTEXT Context;
    POBJECT_HEADER_NAME_INFO ObjectNameInfo;
    POBJECT_TYPE ObjectType;
    PVOID Directory = NULL;

    /* Get object structures */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
    ObjectNameInfo = ObpReferenceNameInfo(ObjectHeader);
    ObjectType = ObjectHeader->Type;

    /*
     * Check if the handle count is 0, if the object is named,
     * and if the object isn't a permanent object.
     */
    if (!(ObjectHeader->HandleCount) &&
         (ObjectNameInfo) &&
         (ObjectNameInfo->Name.Length) &&
         (ObjectNameInfo->Directory) &&
         !(ObjectHeader->Flags & OB_FLAG_PERMANENT))
    {
        /* Setup a lookup context and lock it */
        ObpInitializeLookupContext(&Context);
        ObpAcquireLookupContextLock(&Context, ObjectNameInfo->Directory);

        /* Do the lookup */
        Object = ObpLookupEntryDirectory(ObjectNameInfo->Directory,
                                         &ObjectNameInfo->Name,
                                         0,
                                         FALSE,
                                         &Context);
        if (Object)
        {
            /* Lock the object */
            ObpAcquireObjectLock(ObjectHeader);

            /* Make sure we can still delete the object */
            if (!(ObjectHeader->HandleCount) &&
                !(ObjectHeader->Flags & OB_FLAG_PERMANENT))
            {
                /* First delete it from the directory */
                ObpDeleteEntryDirectory(&Context);

                /* Check if this is a symbolic link */
                if (ObjectType == ObpSymbolicLinkObjectType)
                {
                    /* Remove internal name */
                    ObpDeleteSymbolicLinkName(Object);
                }

                /* Check if the kernel exclusive flag is set */
                ObjectNameInfo = OBJECT_HEADER_TO_NAME_INFO(ObjectHeader);
                if ((ObjectNameInfo) &&
                    (ObjectNameInfo->QueryReferences & OB_FLAG_KERNEL_EXCLUSIVE))
                {
                    /* Remove protection flag */
                    InterlockedExchangeAdd((PLONG)&ObjectNameInfo->QueryReferences,
                                           -OB_FLAG_KERNEL_EXCLUSIVE);
                }

                /* Get the directory */
                Directory = ObjectNameInfo->Directory;
            }

            /* Release the lock */
            ObpReleaseObjectLock(ObjectHeader);
        }

        /* Cleanup after lookup */
        ObpReleaseLookupContext(&Context);

        /* Remove another query reference since we added one on top */
        ObpDereferenceNameInfo(ObjectNameInfo);

        /* Check if we were inserted in a directory */
        if (Directory)
        {
            /* We were, so first remove the extra reference we had added */
            ObpDereferenceNameInfo(ObjectNameInfo);

            /* Now dereference the object as well */
            ObDereferenceObject(Object);
        }
    }
    else
    {
        /* Remove the reference we added */
        ObpDereferenceNameInfo(ObjectNameInfo);
    }
}

BOOLEAN
NTAPI
ObpIsUnsecureName(IN PUNICODE_STRING ObjectName,
                  IN BOOLEAN CaseInSensitive)
{
    BOOLEAN Unsecure;
    PWSTR UnsecureBuffer;
    UNICODE_STRING UnsecureName;

    /* No unsecure names known, quit */
    if (ObpUnsecureGlobalNamesBuffer[0] == UNICODE_NULL)
    {
        return FALSE;
    }

    /* By default, we have a secure name */
    Unsecure = FALSE;
    /* We will browse the whole string */
    UnsecureBuffer = &ObpUnsecureGlobalNamesBuffer[0];
    while (TRUE)
    {
        /* Initialize the unicode string */
        RtlInitUnicodeString(&UnsecureName, UnsecureBuffer);
        /* We're at the end of the multisz string! */
        if (UnsecureName.Length == 0)
        {
            break;
        }

        /*
         * Does the unsecure name prefix the object name?
         * If so, that's an unsecure name, and return so
         */
        if (RtlPrefixUnicodeString(&UnsecureName, ObjectName, CaseInSensitive))
        {
            Unsecure = TRUE;
            break;
        }

        /*
         * Move to the next string. As a reminder, ObpUnsecureGlobalNamesBuffer is
         * a multisz, so we move the string next to the current UNICODE_NULL char
         */
        UnsecureBuffer = (PWSTR)((ULONG_PTR)UnsecureBuffer + UnsecureName.Length + sizeof(UNICODE_NULL));
    }

    /* Return our findings */
    return Unsecure;
}

NTSTATUS
NTAPI
ObpLookupObjectName(IN HANDLE RootHandle OPTIONAL,
                    IN OUT PUNICODE_STRING ObjectName,
                    IN ULONG Attributes,
                    IN POBJECT_TYPE ObjectType,
                    IN KPROCESSOR_MODE AccessMode,
                    IN OUT PVOID ParseContext,
                    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
                    IN PVOID InsertObject OPTIONAL,
                    IN OUT PACCESS_STATE AccessState,
                    OUT POBP_LOOKUP_CONTEXT LookupContext,
                    OUT PVOID *FoundObject)
{
    PVOID Object;
    POBJECT_HEADER ObjectHeader;
    UNICODE_STRING ComponentName, RemainingName;
    BOOLEAN Reparse = FALSE, SymLink = FALSE;
    POBJECT_DIRECTORY Directory = NULL, ParentDirectory = NULL, RootDirectory;
    POBJECT_DIRECTORY ReferencedDirectory = NULL, ReferencedParentDirectory = NULL;
    KIRQL CalloutIrql;
    OB_PARSE_METHOD ParseRoutine;
    NTSTATUS Status;
    KPROCESSOR_MODE AccessCheckMode;
    PWCHAR NewName;
    POBJECT_HEADER_NAME_INFO ObjectNameInfo;
    ULONG MaxReparse = 30;
    PDEVICE_MAP DeviceMap = NULL;
    UNICODE_STRING LocalName;
    PAGED_CODE();
    OBTRACE(OB_NAMESPACE_DEBUG,
            "%s - Finding Object: %wZ. Expecting: %p\n",
            __FUNCTION__,
            ObjectName,
            InsertObject);

    /* Initialize starting state */
    ObpInitializeLookupContext(LookupContext);
    *FoundObject = NULL;
    Status = STATUS_SUCCESS;
    Object = NULL;

    /* Check if case-insensitivity is checked */
    if (ObpCaseInsensitive)
    {
        /* Check if the object type requests this */
        if (!(ObjectType) || (ObjectType->TypeInfo.CaseInsensitive))
        {
            /* Add the flag to disable case sensitivity */
            Attributes |= OBJ_CASE_INSENSITIVE;
        }
    }

    /* Check if this is a access checks are being forced */
    AccessCheckMode = (Attributes & OBJ_FORCE_ACCESS_CHECK) ?
                       UserMode : AccessMode;

    /* Check if we got a Root Directory */
    if (RootHandle)
    {
        /* We did. Reference it */
        Status = ObReferenceObjectByHandle(RootHandle,
                                           0,
                                           NULL,
                                           AccessMode,
                                           (PVOID*)&RootDirectory,
                                           NULL);
        if (!NT_SUCCESS(Status)) return Status;

        /* Get the header */
        ObjectHeader = OBJECT_TO_OBJECT_HEADER(RootDirectory);

        /* The name cannot start with a separator, unless this is a file */
        if ((ObjectName->Buffer) &&
            (ObjectName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR) &&
            (ObjectHeader->Type != IoFileObjectType))
        {
            /* The syntax is bad, so fail this request */
            ObDereferenceObject(RootDirectory);
            return STATUS_OBJECT_PATH_SYNTAX_BAD;
        }

        /* Don't parse a Directory */
        if (ObjectHeader->Type != ObpDirectoryObjectType)
        {
            /* Make sure the Object Type has a parse routine */
            ParseRoutine = ObjectHeader->Type->TypeInfo.ParseProcedure;
            if (!ParseRoutine)
            {
                /* We can't parse a name if we don't have a parse routine */
                ObDereferenceObject(RootDirectory);
                return STATUS_INVALID_HANDLE;
            }

            /* Set default parse count */
            MaxReparse = 30;

            /* Now parse */
            while (TRUE)
            {
                /* Start with the full name */
                RemainingName = *ObjectName;

                /* Call the Parse Procedure */
                ObpCalloutStart(&CalloutIrql);
                Status = ParseRoutine(RootDirectory,
                                      ObjectType,
                                      AccessState,
                                      AccessCheckMode,
                                      Attributes,
                                      ObjectName,
                                      &RemainingName,
                                      ParseContext,
                                      SecurityQos,
                                      &Object);
                ObpCalloutEnd(CalloutIrql, "Parse", ObjectHeader->Type, Object);

                /* Check for success or failure, so not reparse */
                if ((Status != STATUS_REPARSE) &&
                    (Status != STATUS_REPARSE_OBJECT))
                {
                    /* Check for failure */
                    if (!NT_SUCCESS(Status))
                    {
                        /* Parse routine might not have cleared this, do it */
                        Object = NULL;
                    }
                    else if (!Object)
                    {
                        /* Modify status to reflect failure inside Ob */
                        Status = STATUS_OBJECT_NAME_NOT_FOUND;
                    }

                    /* We're done, return the status and object */
                    *FoundObject = Object;
                    ObDereferenceObject(RootDirectory);
                    return Status;
                }
                else if ((!ObjectName->Length) ||
                         (!ObjectName->Buffer) ||
                         (ObjectName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR))
                {
                    /* Reparsed to the root directory, so start over */
                    ObDereferenceObject(RootDirectory);
                    RootDirectory = ObpRootDirectoryObject;

                    /* Don't use this anymore, since we're starting at root */
                    RootHandle = NULL;
                    goto ParseFromRoot;
                }
                else if (--MaxReparse)
                {
                    /* Try reparsing again */
                    continue;
                }
                else
                {
                    /* Reparsed too many times */
                    ObDereferenceObject(RootDirectory);

                    /* Return the object and normalized status */
                    *FoundObject = Object;
                    if (!Object) Status = STATUS_OBJECT_NAME_NOT_FOUND;
                    return Status;
                }
            }
        }
        else if (!(ObjectName->Length) || !(ObjectName->Buffer))
        {
            /* Just return the Root Directory if we didn't get a name */
            Status = ObReferenceObjectByPointer(RootDirectory,
                                                0,
                                                ObjectType,
                                                AccessMode);
            if (NT_SUCCESS(Status)) Object = RootDirectory;

            /* Remove the first reference we added and return the object */
            ObDereferenceObject(RootDirectory);
            *FoundObject = Object;
            return Status;
        }

        LocalName = *ObjectName;
    }
    else
    {
        /* We did not get a Root Directory, so use the root */
        RootDirectory = ObpRootDirectoryObject;

        /* It must start with a path separator */
        if (!(ObjectName->Length) ||
            !(ObjectName->Buffer) ||
            (ObjectName->Buffer[0] != OBJ_NAME_PATH_SEPARATOR))
        {
            /* This name is invalid, so fail */
            return STATUS_OBJECT_PATH_SYNTAX_BAD;
        }

        /* Check if the name is only the path separator */
        if (ObjectName->Length == sizeof(OBJ_NAME_PATH_SEPARATOR))
        {
            /* So the caller only wants the root directory; do we have one? */
            if (!RootDirectory)
            {
                /* This must be the first time we're creating it... right? */
                if (InsertObject)
                {
                    /* Yes, so return it to ObInsert so that it can create it */
                    Status = ObReferenceObjectByPointer(InsertObject,
                                                        0,
                                                        ObjectType,
                                                        AccessMode);
                    if (NT_SUCCESS(Status)) *FoundObject = InsertObject;
                    return Status;
                }
                else
                {
                    /* This should never really happen */
                    ASSERT(FALSE);
                    return STATUS_INVALID_PARAMETER;
                }
            }
            else
            {
                /* We do have the root directory, so just return it */
                Status = ObReferenceObjectByPointer(RootDirectory,
                                                    0,
                                                    ObjectType,
                                                    AccessMode);
                if (NT_SUCCESS(Status)) *FoundObject = RootDirectory;
                return Status;
            }
        }
        else
        {
ParseFromRoot:
            LocalName = *ObjectName;

            /* Deference the device map if we already have one */
            if (DeviceMap != NULL)
            {
                ObfDereferenceDeviceMap(DeviceMap);
                DeviceMap = NULL;
            }

            /* Check if this is a possible DOS name */
            if (!((ULONG_PTR)(ObjectName->Buffer) & 7))
            {
                /*
                 * This could be one. Does it match the prefix?
                 * Note that as an optimization, the match is done as 64-bit
                 * compare since the prefix is "\??\" which is exactly 8 bytes.
                 *
                 * In the second branch, we test for "\??" which is also valid.
                 * This time, we use a 32-bit compare followed by a Unicode
                 * character compare (16-bit), since the sum is 6 bytes.
                 */
                if ((ObjectName->Length >= ObpDosDevicesShortName.Length) &&
                    (*(PULONGLONG)(ObjectName->Buffer) ==
                     ObpDosDevicesShortNamePrefix.Alignment.QuadPart))
                {
                    DeviceMap = ObpReferenceDeviceMap();
                    /* We have a local mapping, drop the ?? prefix */
                    if (DeviceMap != NULL && DeviceMap->DosDevicesDirectory != NULL)
                    {
                        LocalName.Length -= ObpDosDevicesShortName.Length;
                        LocalName.MaximumLength -= ObpDosDevicesShortName.Length;
                        LocalName.Buffer += (ObpDosDevicesShortName.Length / sizeof(WCHAR));

                        /* We'll browse that local directory */
                        Directory = DeviceMap->DosDevicesDirectory;
                    }
                }
                else if ((ObjectName->Length == ObpDosDevicesShortName.Length -
                                                sizeof(WCHAR)) &&
                         (*(PULONG)(ObjectName->Buffer) ==
                          ObpDosDevicesShortNameRoot.Alignment.LowPart) &&
                         (*((PWCHAR)(ObjectName->Buffer) + 2) ==
                          (WCHAR)(ObpDosDevicesShortNameRoot.Alignment.HighPart)))
                {
                    DeviceMap = ObpReferenceDeviceMap();

                    /* Caller is looking for the directory itself */
                    if (DeviceMap != NULL && DeviceMap->DosDevicesDirectory != NULL)
                    {
                        Status = ObReferenceObjectByPointer(DeviceMap->DosDevicesDirectory,
                                                            0,
                                                            ObjectType,
                                                            AccessMode);
                        if (NT_SUCCESS(Status))
                        {
                            *FoundObject = DeviceMap->DosDevicesDirectory;
                        }

                        ObfDereferenceDeviceMap(DeviceMap);
                        return Status;
                    }
                }
            }
        }
    }

    /* Check if we were reparsing a symbolic link */
    if (!SymLink)
    {
        /* Allow reparse */
        Reparse = TRUE;
        MaxReparse = 30;
    }

    /* Reparse */
    while (Reparse && MaxReparse)
    {
        /* Get the name */
        RemainingName = LocalName;

        /* Disable reparsing again */
        Reparse = FALSE;

        /* Start parse loop */
        while (TRUE)
        {
            /* Clear object */
            Object = NULL;

            /* Check if the name starts with a path separator */
            if ((RemainingName.Length) &&
                (RemainingName.Buffer[0] == OBJ_NAME_PATH_SEPARATOR))
            {
                /* Skip the path separator */
                RemainingName.Buffer++;
                RemainingName.Length -= sizeof(OBJ_NAME_PATH_SEPARATOR);
            }

            /* Find the next Part Name */
            ComponentName = RemainingName;
            while (RemainingName.Length)
            {
                /* Break if we found the \ ending */
                if (RemainingName.Buffer[0] == OBJ_NAME_PATH_SEPARATOR) break;

                /* Move on */
                RemainingName.Buffer++;
                RemainingName.Length -= sizeof(OBJ_NAME_PATH_SEPARATOR);
            }

            /* Get its size and make sure it's valid */
            ComponentName.Length -= RemainingName.Length;
            if (!ComponentName.Length)
            {
                /* Invalid size, fail */
                Status = STATUS_OBJECT_NAME_INVALID;
                break;
            }

            /* Check if we're in the root */
            if (!Directory) Directory = RootDirectory;

            /* Check if this is a user-mode call that needs to traverse */
            if ((AccessCheckMode != KernelMode) &&
                !(AccessState->Flags & TOKEN_HAS_TRAVERSE_PRIVILEGE))
            {
                /* We shouldn't have referenced a directory yet */
                ASSERT(ReferencedDirectory == NULL);

                /* Reference the directory */
                ObReferenceObject(Directory);
                ReferencedDirectory = Directory;

                /* Check if we have a parent directory */
                if (ParentDirectory)
                {
                    /* Check for traverse access */
                    if (!ObpCheckTraverseAccess(ParentDirectory,
                                                DIRECTORY_TRAVERSE,
                                                AccessState,
                                                FALSE,
                                                AccessCheckMode,
                                                &Status))
                    {
                        /* We don't have it, fail */
                        break;
                    }
                }
            }

            /* Check if we don't have a remaining name yet */
            if (!RemainingName.Length)
            {
                /* Check if we don't have a referenced directory yet */
                if (!ReferencedDirectory)
                {
                    /* Reference it */
                    ObReferenceObject(Directory);
                    ReferencedDirectory = Directory;
                }

                /* Check if we are inserting an object */
                if (InsertObject)
                {
                    /* Lock the lookup context */
                    ObpAcquireLookupContextLock(LookupContext, Directory);
                }
            }

            /* Do the lookup */
            Object = ObpLookupEntryDirectory(Directory,
                                             &ComponentName,
                                             Attributes,
                                             InsertObject ? FALSE : TRUE,
                                             LookupContext);
            if (!Object)
            {
                /* We didn't find it... do we still have a path? */
                if (RemainingName.Length)
                {
                    /* Then tell the caller the path wasn't found */
                    Status = STATUS_OBJECT_PATH_NOT_FOUND;
                    break;
                }
                else if (!InsertObject)
                {
                    /* Otherwise, we have a path, but the name isn't valid */
                    Status = STATUS_OBJECT_NAME_NOT_FOUND;
                    break;
                }

                /* Check create access for the object */
                if (!ObCheckCreateObjectAccess(Directory,
                                               ObjectType == ObpDirectoryObjectType ?
                                               DIRECTORY_CREATE_SUBDIRECTORY :
                                               DIRECTORY_CREATE_OBJECT,
                                               AccessState,
                                               &ComponentName,
                                               FALSE,
                                               AccessCheckMode,
                                               &Status))
                {
                    /* We don't have create access, fail */
                    break;
                }

                /* Get the object header */
                ObjectHeader = OBJECT_TO_OBJECT_HEADER(InsertObject);

                /*
                 * Deny object creation if:
                 * That's a section object or a symbolic link
                 * Which isn't in the same section that root directory
                 * That doesn't have the SeCreateGlobalPrivilege
                 * And that is not a known unsecure name
                 */
                if (RootDirectory->SessionId != -1)
                {
                    if (ObjectHeader->Type == MmSectionObjectType ||
                        ObjectHeader->Type == ObpSymbolicLinkObjectType)
                    {
                        if (RootDirectory->SessionId != PsGetCurrentProcessSessionId() &&
                            !SeSinglePrivilegeCheck(SeCreateGlobalPrivilege, AccessCheckMode) &&
                            !ObpIsUnsecureName(&ComponentName, BooleanFlagOn(Attributes, OBJ_CASE_INSENSITIVE)))
                        {
                            Status = STATUS_ACCESS_DENIED;
                            break;
                        }
                    }
                }

                /* Create Object Name */
                NewName = ExAllocatePoolWithTag(PagedPool,
                                                ComponentName.Length,
                                                OB_NAME_TAG);
                if (!(NewName) ||
                    !(ObpInsertEntryDirectory(Directory,
                                              LookupContext,
                                              ObjectHeader)))
                {
                    /* Either couldn't allocate the name, or insert failed */
                    if (NewName) ExFreePoolWithTag(NewName, OB_NAME_TAG);

                    /* Fail due to memory reasons */
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                /* Reference newly to be inserted object */
                ObReferenceObject(InsertObject);

                /* Get the name information */
                ObjectNameInfo = OBJECT_HEADER_TO_NAME_INFO(ObjectHeader);

                /* Reference the directory */
                ObReferenceObject(Directory);

                /* Copy the Name */
                RtlCopyMemory(NewName,
                              ComponentName.Buffer,
                              ComponentName.Length);

                /* Check if we had an old name */
                if (ObjectNameInfo->Name.Buffer)
                {
                    /* Free it */
                    ExFreePoolWithTag(ObjectNameInfo->Name.Buffer, OB_NAME_TAG);
                }

                /* Write new one */
                ObjectNameInfo->Name.Buffer = NewName;
                ObjectNameInfo->Name.Length = ComponentName.Length;
                ObjectNameInfo->Name.MaximumLength = ComponentName.Length;

                /* Return Status and the Expected Object */
                Status = STATUS_SUCCESS;
                Object = InsertObject;

                /* Get out of here */
                break;
            }

ReparseObject:
            /* We found it, so now get its header */
            ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);

            /*
             * Check for a parse Procedure, but don't bother to parse for an insert
             * unless it's a Symbolic Link, in which case we MUST parse
             */
            ParseRoutine = ObjectHeader->Type->TypeInfo.ParseProcedure;
            if ((ParseRoutine) &&
                (!(InsertObject) || (ParseRoutine == ObpParseSymbolicLink)))
            {
                /* Use the Root Directory next time */
                Directory = NULL;

                /* Increment the pointer count */
                InterlockedExchangeAddSizeT(&ObjectHeader->PointerCount, 1);

                /* Cleanup from the first lookup */
                ObpReleaseLookupContext(LookupContext);

                /* Check if we have a referenced directory */
                if (ReferencedDirectory)
                {
                    /* We do, dereference it */
                    ObDereferenceObject(ReferencedDirectory);
                    ReferencedDirectory = NULL;
                }

                /* Check if we have a referenced parent directory */
                if (ReferencedParentDirectory)
                {
                    /* We do, dereference it */
                    ObDereferenceObject(ReferencedParentDirectory);
                    ReferencedParentDirectory = NULL;
                }

                /* Call the Parse Procedure */
                ObpCalloutStart(&CalloutIrql);
                Status = ParseRoutine(Object,
                                      ObjectType,
                                      AccessState,
                                      AccessCheckMode,
                                      Attributes,
                                      ObjectName,
                                      &RemainingName,
                                      ParseContext,
                                      SecurityQos,
                                      &Object);
                ObpCalloutEnd(CalloutIrql, "Parse", ObjectHeader->Type, Object);

                /* Remove our extra reference */
                ObDereferenceObject(&ObjectHeader->Body);

                /* Check if we have to reparse */
                if ((Status == STATUS_REPARSE) ||
                    (Status == STATUS_REPARSE_OBJECT))
                {
                    /* Reparse again */
                    Reparse = TRUE;
                    --MaxReparse;
                    if (MaxReparse == 0)
                    {
                        Object = NULL;
                        break;
                    }

                    /* Start over from root if we got sent back there */
                    if ((Status == STATUS_REPARSE_OBJECT) ||
                        (ObjectName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR))
                    {
                        /* Check if we got a root directory */
                        if (RootHandle)
                        {
                            /* Stop using it, because we have a new directory now */
                            ObDereferenceObject(RootDirectory);
                            RootHandle = NULL;
                        }

                        /* Start at Root */
                        ParentDirectory = NULL;
                        RootDirectory = ObpRootDirectoryObject;

                        /* Check for reparse status */
                        if (Status == STATUS_REPARSE_OBJECT)
                        {
                            /* Don't reparse again */
                            Reparse = FALSE;

                            /* Did we actually get an object to which to reparse? */
                            if (!Object)
                            {
                                /* We didn't, so set a failure status */
                                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                            }
                            else
                            {
                                /* We did, so we're free to parse the new object */
                                goto ReparseObject;
                            }
                        }
                        else
                        {
                            /* This is a symbolic link */
                            SymLink = TRUE;
                            goto ParseFromRoot;
                        }
                    }
                    else if (RootDirectory == ObpRootDirectoryObject)
                    {
                        /* We got STATUS_REPARSE but are at the Root Directory */
                        Object = NULL;
                        Status = STATUS_OBJECT_NAME_NOT_FOUND;
                        Reparse = FALSE;
                    }
                }
                else if (!NT_SUCCESS(Status))
                {
                    /* Total failure */
                    Object = NULL;
                }
                else if (!Object)
                {
                    /* We didn't reparse but we didn't find the Object Either */
                    Status = STATUS_OBJECT_NAME_NOT_FOUND;
                }

                /* Break out of the loop */
                break;
            }
            else
            {
                /* No parse routine...do we still have a remaining name? */
                if (!RemainingName.Length)
                {
                    /* Are we creating an object? */
                    if (!InsertObject)
                    {
                        /* Check if this is a user-mode call that needs to traverse */
                        if ((AccessCheckMode != KernelMode) &&
                            !(AccessState->Flags & TOKEN_HAS_TRAVERSE_PRIVILEGE))
                        {
                            /* Check if we can get it */
                            if (!ObpCheckTraverseAccess(Directory,
                                                        DIRECTORY_TRAVERSE,
                                                        AccessState,
                                                        FALSE,
                                                        AccessCheckMode,
                                                        &Status))
                            {
                                /* We don't have access, fail */
                                Object = NULL;
                                break;
                            }
                        }

                        /* Reference the Object */
                        Status = ObReferenceObjectByPointer(Object,
                                                            0,
                                                            ObjectType,
                                                            AccessMode);
                        if (!NT_SUCCESS(Status)) Object = NULL;
                    }

                    /* And get out of the reparse loop */
                    break;
                }
                else
                {
                    /* We still have a name; check if this is a directory object */
                    if (ObjectHeader->Type == ObpDirectoryObjectType)
                    {
                        /* Check if we have a referenced parent directory */
                        if (ReferencedParentDirectory)
                        {
                            /* Dereference it */
                            ObDereferenceObject(ReferencedParentDirectory);
                        }

                        /* Restart the lookup from this directory */
                        ReferencedParentDirectory = ReferencedDirectory;
                        ParentDirectory = Directory;
                        Directory = Object;
                        ReferencedDirectory = NULL;
                    }
                    else
                    {
                        /* We still have a name, but no parse routine for it */
                        Status = STATUS_OBJECT_TYPE_MISMATCH;
                        Object = NULL;
                        break;
                    }
                }
            }
        }
    }

    /* Check if we failed */
    if (!NT_SUCCESS(Status))
    {
        /* Cleanup after lookup */
        ObpReleaseLookupContext(LookupContext);
    }

    /* Check if we have a device map and dereference it if so */
    if (DeviceMap) ObfDereferenceDeviceMap(DeviceMap);

    /* Check if we have a referenced directory and dereference it if so */
    if (ReferencedDirectory) ObDereferenceObject(ReferencedDirectory);

    /* Check if we have a referenced parent directory */
    if (ReferencedParentDirectory)
    {
        /* We do, dereference it */
        ObDereferenceObject(ReferencedParentDirectory);
    }

    /* Set the found object and check if we got one */
    *FoundObject = Object;
    if (!Object)
    {
        /* Nothing was found. Did we reparse or get success? */
        if ((Status == STATUS_REPARSE) || (NT_SUCCESS(Status)))
        {
            /* Set correct failure */
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }

    /* Check if we had a root directory */
    if (RootHandle) ObDereferenceObject(RootDirectory);

    /* Return status to caller */
    OBTRACE(OB_NAMESPACE_DEBUG,
            "%s - Found Object: %p. Expected: %p\n",
            __FUNCTION__,
            *FoundObject,
            InsertObject);
    return Status;
}

/* PUBLIC FUNCTIONS *********************************************************/

NTSTATUS
NTAPI
ObQueryNameString(IN PVOID Object,
                  OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
                  IN ULONG Length,
                  OUT PULONG ReturnLength)
{
    POBJECT_HEADER_NAME_INFO LocalInfo;
    POBJECT_HEADER ObjectHeader;
    POBJECT_DIRECTORY ParentDirectory;
    ULONG NameSize;
    PWCH ObjectName;
    BOOLEAN ObjectIsNamed;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Get the Kernel Meta-Structures */
    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Object);
    LocalInfo = OBJECT_HEADER_TO_NAME_INFO(ObjectHeader);

    /* Check if a Query Name Procedure is available */
    if (ObjectHeader->Type->TypeInfo.QueryNameProcedure)
    {
        /* Call the procedure inside SEH */
        ObjectIsNamed = ((LocalInfo) && (LocalInfo->Name.Length > 0));

        _SEH2_TRY
        {
            Status = ObjectHeader->Type->TypeInfo.QueryNameProcedure(Object,
                                                               ObjectIsNamed,
                                                               ObjectNameInfo,
                                                               Length,
                                                               ReturnLength,
                                                               KernelMode);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        return Status;
    }

    /* Check if the object doesn't even have a name */
    if (!(LocalInfo) || !(LocalInfo->Name.Buffer))
    {
        Status = STATUS_SUCCESS;

        _SEH2_TRY
        {
            /* We're returning the name structure */
            *ReturnLength = sizeof(OBJECT_NAME_INFORMATION);

            /* Check if we were given enough space */
            if (*ReturnLength > Length)
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                /* Return an empty buffer */
                RtlInitEmptyUnicodeString(&ObjectNameInfo->Name, NULL, 0);
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        return Status;
    }

    /*
     * Find the size needed for the name. We won't do
     * this during the Name Creation loop because we want
     * to let the caller know that the buffer isn't big
     * enough right at the beginning, not work our way through
     * and find out at the end
     */
    _SEH2_TRY
    {
        if (Object == ObpRootDirectoryObject)
        {
            /* Size of the '\' string */
            NameSize = sizeof(OBJ_NAME_PATH_SEPARATOR);
        }
        else
        {
            /* Get the Object Directory and add name of Object */
            ParentDirectory = LocalInfo->Directory;
            NameSize = sizeof(OBJ_NAME_PATH_SEPARATOR) + LocalInfo->Name.Length;

            /* Loop inside the directory to get the top-most one (meaning root) */
            while ((ParentDirectory != ObpRootDirectoryObject) && (ParentDirectory))
            {
                /* Get the Name Information */
                LocalInfo = OBJECT_HEADER_TO_NAME_INFO(
                    OBJECT_TO_OBJECT_HEADER(ParentDirectory));

                /* Add the size of the Directory Name */
                if (LocalInfo && LocalInfo->Directory)
                {
                    /* Size of the '\' string + Directory Name */
                    NameSize += sizeof(OBJ_NAME_PATH_SEPARATOR) +
                                LocalInfo->Name.Length;

                    /* Move to next parent Directory */
                    ParentDirectory = LocalInfo->Directory;
                }
                else
                {
                    /* Directory with no name. We append "...\" */
                    NameSize += sizeof(L"...") + sizeof(OBJ_NAME_PATH_SEPARATOR);
                    break;
                }
            }
        }

        /* Finally, add the name of the structure and the null char */
        *ReturnLength = NameSize +
                        sizeof(OBJECT_NAME_INFORMATION) +
                        sizeof(UNICODE_NULL);

        /* Check if we were given enough space */
        if (*ReturnLength > Length) _SEH2_YIELD(return STATUS_INFO_LENGTH_MISMATCH);

        /*
        * Now we will actually create the name. We work backwards because
        * it's easier to start off from the Name we have and walk up the
        * parent directories. We use the same logic as Name Length calculation.
        */
        LocalInfo = OBJECT_HEADER_TO_NAME_INFO(ObjectHeader);
        ObjectName = (PWCH)((ULONG_PTR)ObjectNameInfo + *ReturnLength);
        *--ObjectName = UNICODE_NULL;

        /* Check if the object is actually the Root directory */
        if (Object == ObpRootDirectoryObject)
        {
            /* This is already the Root Directory, return "\\" */
            *--ObjectName = OBJ_NAME_PATH_SEPARATOR;
            ObjectNameInfo->Name.Length = (USHORT)NameSize;
            ObjectNameInfo->Name.MaximumLength = (USHORT)(NameSize +
                                                          sizeof(UNICODE_NULL));
            ObjectNameInfo->Name.Buffer = ObjectName;
            Status = STATUS_SUCCESS;
        }
        else
        {
            /* Start by adding the Object's Name */
            ObjectName = (PWCH)((ULONG_PTR)ObjectName -
                                           LocalInfo->Name.Length);
            RtlCopyMemory(ObjectName,
                          LocalInfo->Name.Buffer,
                          LocalInfo->Name.Length);

            /* Now parse the Parent directories until we reach the top */
            ParentDirectory = LocalInfo->Directory;
            while ((ParentDirectory != ObpRootDirectoryObject) && (ParentDirectory))
            {
                /* Get the name information */
                LocalInfo = OBJECT_HEADER_TO_NAME_INFO(
                    OBJECT_TO_OBJECT_HEADER(ParentDirectory));

                /* Add the "\" */
                *(--ObjectName) = OBJ_NAME_PATH_SEPARATOR;

                /* Add the Parent Directory's Name */
                if (LocalInfo && LocalInfo->Name.Buffer)
                {
                    /* Add the name */
                    ObjectName = (PWCH)((ULONG_PTR)ObjectName -
                                                   LocalInfo->Name.Length);
                    RtlCopyMemory(ObjectName,
                                  LocalInfo->Name.Buffer,
                                  LocalInfo->Name.Length);

                    /* Move to next parent */
                    ParentDirectory = LocalInfo->Directory;
                }
                else
                {
                    /* Directory without a name, we add "..." */
                    ObjectName = (PWCH)((ULONG_PTR)ObjectName -
                                                   sizeof(L"...") +
                                                   sizeof(UNICODE_NULL));
                    RtlCopyMemory(ObjectName, L"...", sizeof(L"..."));
                    break;
                }
            }

            /* Add Root Directory Name */
            *(--ObjectName) = OBJ_NAME_PATH_SEPARATOR;
            ObjectNameInfo->Name.Length = (USHORT)NameSize;
            ObjectNameInfo->Name.MaximumLength =
                (USHORT)(NameSize + sizeof(UNICODE_NULL));
            ObjectNameInfo->Name.Buffer = ObjectName;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Return the exception code */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* Return success */
    return Status;
}

/* EOF */

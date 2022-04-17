/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ob/obdir.c
 * PURPOSE:         Manages the Object Manager's Directory Implementation,
 *                  such as functions for addition, deletion and lookup into
 *                  the Object Manager's namespace. These routines are separate
 *                  from the Namespace Implementation because they are largely
 *                  independent and could be used for other namespaces.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Thomas Weidenmueller (w3seek@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

POBJECT_TYPE ObpDirectoryObjectType = NULL;

/* PRIVATE FUNCTIONS ******************************************************/

/*++
* @name ObpInsertEntryDirectory
*
*     The ObpInsertEntryDirectory routine <FILLMEIN>.
*
* @param Parent
*        <FILLMEIN>.
*
* @param Context
*        <FILLMEIN>.
*
* @param ObjectHeader
*        <FILLMEIN>.
*
* @return TRUE if the object was inserted, FALSE otherwise.
*
* @remarks None.
*
*--*/
BOOLEAN
NTAPI
ObpInsertEntryDirectory(IN POBJECT_DIRECTORY Parent,
                        IN POBP_LOOKUP_CONTEXT Context,
                        IN POBJECT_HEADER ObjectHeader)
{
    POBJECT_DIRECTORY_ENTRY *AllocatedEntry;
    POBJECT_DIRECTORY_ENTRY NewEntry;
    POBJECT_HEADER_NAME_INFO HeaderNameInfo;

    /* Make sure we have a name */
    ASSERT(ObjectHeader->NameInfoOffset != 0);

    /* Validate the context */
    if ((Context->Object) ||
        !(Context->DirectoryLocked) ||
        (Parent != Context->Directory))
    {
        /* Invalid context */
        DPRINT1("OB: ObpInsertEntryDirectory - invalid context %p %u\n",
                Context, Context->DirectoryLocked);
        ASSERT(FALSE);
        return FALSE;
    }

    /* Allocate a new Directory Entry */
    NewEntry = ExAllocatePoolWithTag(PagedPool,
                                     sizeof(OBJECT_DIRECTORY_ENTRY),
                                     OB_DIR_TAG);
    if (!NewEntry) return FALSE;

    /* Save the hash */
    NewEntry->HashValue = Context->HashValue;

    /* Get the Object Name Information */
    HeaderNameInfo = OBJECT_HEADER_TO_NAME_INFO(ObjectHeader);

    /* Get the Allocated entry */
    AllocatedEntry = &Parent->HashBuckets[Context->HashIndex];

    /* Set it */
    NewEntry->ChainLink = *AllocatedEntry;
    *AllocatedEntry = NewEntry;

    /* Associate the Object */
    NewEntry->Object = &ObjectHeader->Body;

    /* Associate the Directory */
    HeaderNameInfo->Directory = Parent;
    return TRUE;
}

/*++
* @name ObpGetShadowDirectory
*
*     The ObpGetShadowDirectory routine <FILLMEIN>.
*
* @param Directory
*        <FILLMEIN>.
*
* @return Pointer to the global DOS directory if any, or NULL otherwise.
*
* @remarks None.
*
*--*/
POBJECT_DIRECTORY
NTAPI
ObpGetShadowDirectory(IN POBJECT_DIRECTORY Directory)
{
    PDEVICE_MAP DeviceMap;
    POBJECT_DIRECTORY GlobalDosDirectory = NULL;

    /* Acquire the device map lock */
    KeAcquireGuardedMutex(&ObpDeviceMapLock);

    /* Get the global DOS directory if any */
    DeviceMap = Directory->DeviceMap;
    if (DeviceMap != NULL)
    {
        GlobalDosDirectory = DeviceMap->GlobalDosDevicesDirectory;
    }

    /* Release the devicemap lock */
    KeReleaseGuardedMutex(&ObpDeviceMapLock);

    return GlobalDosDirectory;
}

/*++
* @name ObpLookupEntryDirectory
*
*     The ObpLookupEntryDirectory routine <FILLMEIN>.
*
* @param Directory
*        <FILLMEIN>.
*
* @param Name
*        <FILLMEIN>.
*
* @param Attributes
*        <FILLMEIN>.
*
* @param SearchShadow
*        <FILLMEIN>.
*
* @param Context
*        <FILLMEIN>.
*
* @return Pointer to the object which was found, or NULL otherwise.
*
* @remarks None.
*
*--*/
PVOID
NTAPI
ObpLookupEntryDirectory(IN POBJECT_DIRECTORY Directory,
                        IN PUNICODE_STRING Name,
                        IN ULONG Attributes,
                        IN UCHAR SearchShadow,
                        IN POBP_LOOKUP_CONTEXT Context)
{
    BOOLEAN CaseInsensitive = FALSE;
    POBJECT_HEADER_NAME_INFO HeaderNameInfo;
    POBJECT_HEADER ObjectHeader;
    ULONG HashValue;
    ULONG HashIndex;
    LONG TotalChars;
    WCHAR CurrentChar;
    POBJECT_DIRECTORY_ENTRY *AllocatedEntry;
    POBJECT_DIRECTORY_ENTRY *LookupBucket;
    POBJECT_DIRECTORY_ENTRY CurrentEntry;
    PVOID FoundObject = NULL;
    PWSTR Buffer;
    POBJECT_DIRECTORY ShadowDirectory;

    PAGED_CODE();

    /* Check if we should search the shadow directory */
    if (ObpLUIDDeviceMapsEnabled == 0) SearchShadow = FALSE;

    /* Fail if we don't have a directory or name */
    if (!(Directory) || !(Name)) goto Quickie;

    /* Get name information */
    TotalChars = Name->Length / sizeof(WCHAR);
    Buffer = Name->Buffer;

    /* Set up case-sensitivity */
    if (Attributes & OBJ_CASE_INSENSITIVE) CaseInsensitive = TRUE;

    /* Fail if the name is empty */
    if (!(Buffer) || !(TotalChars)) goto Quickie;

    /* Create the Hash */
    for (HashValue = 0; TotalChars; TotalChars--)
    {
        /* Go to the next Character */
        CurrentChar = *Buffer++;

        /* Prepare the Hash */
        HashValue += (HashValue << 1) + (HashValue >> 1);

        /* Create the rest based on the name */
        if (CurrentChar < 'a') HashValue += CurrentChar;
        else if (CurrentChar > 'z') HashValue += RtlUpcaseUnicodeChar(CurrentChar);
        else HashValue += (CurrentChar - ('a'-'A'));
    }

    /* Merge it with our number of hash buckets */
    HashIndex = HashValue % 37;

    /* Save the result */
    Context->HashValue = HashValue;
    Context->HashIndex = (USHORT)HashIndex;

DoItAgain:
    /* Get the root entry and set it as our lookup bucket */
    AllocatedEntry = &Directory->HashBuckets[HashIndex];
    LookupBucket = AllocatedEntry;

    /* Check if the directory is already locked */
    if (!Context->DirectoryLocked)
    {
        /* Lock it */
        ObpAcquireDirectoryLockShared(Directory, Context);
    }

    /* Start looping */
    while ((CurrentEntry = *AllocatedEntry))
    {
        /* Do the hashes match? */
        if (CurrentEntry->HashValue == HashValue)
        {
            /* Make sure that it has a name */
            ObjectHeader = OBJECT_TO_OBJECT_HEADER(CurrentEntry->Object);

            /* Get the name information */
            ASSERT(ObjectHeader->NameInfoOffset != 0);
            HeaderNameInfo = OBJECT_HEADER_TO_NAME_INFO(ObjectHeader);

            /* Do the names match? */
            if ((Name->Length == HeaderNameInfo->Name.Length) &&
                (RtlEqualUnicodeString(Name, &HeaderNameInfo->Name, CaseInsensitive)))
            {
                break;
            }
        }

        /* Move to the next entry */
        AllocatedEntry = &CurrentEntry->ChainLink;
    }

    /* Check if we still have an entry */
    if (CurrentEntry)
    {
        /* Set this entry as the first, to speed up incoming insertion */
        if (AllocatedEntry != LookupBucket)
        {
            /* Check if the directory was locked or convert the lock */
            if ((Context->DirectoryLocked) ||
                (ExConvertPushLockSharedToExclusive(&Directory->Lock)))
            {
                /* Set the Current Entry */
                *AllocatedEntry = CurrentEntry->ChainLink;

                /* Link to the old Hash Entry */
                CurrentEntry->ChainLink = *LookupBucket;

                /* Set the new Hash Entry */
                *LookupBucket = CurrentEntry;
            }
        }

        /* Save the found object */
        FoundObject = CurrentEntry->Object;
        goto Quickie;
    }
    else
    {
        /* Check if the directory was locked */
        if (!Context->DirectoryLocked)
        {
            /* Release the lock */
            ObpReleaseDirectoryLock(Directory, Context);
        }

        /* Check if we should scan the shadow directory */
        if ((SearchShadow) && (Directory->DeviceMap))
        {
            ShadowDirectory = ObpGetShadowDirectory(Directory);
            /* A global DOS directory was found, loop it again */
            if (ShadowDirectory != NULL)
            {
                Directory = ShadowDirectory;
                goto DoItAgain;
            }
        }
    }

Quickie:
    /* Check if we inserted an object */
    if (FoundObject)
    {
        /* Get the object name information */
        ObjectHeader = OBJECT_TO_OBJECT_HEADER(FoundObject);
        ObpReferenceNameInfo(ObjectHeader);

        /* Reference the object being looked up */
        ObReferenceObject(FoundObject);

        /* Check if the directory was locked */
        if (!Context->DirectoryLocked)
        {
            /* Release the lock */
            ObpReleaseDirectoryLock(Directory, Context);
        }
    }

    /* Release any object previously looked up and replace it with the new one */
    ObpReleaseLookupContextObject(Context);
    Context->Object = FoundObject;

    /* Return the object we found */
    return FoundObject;
}

/*++
* @name ObpDeleteEntryDirectory
*
*     The ObpDeleteEntryDirectory routine <FILLMEIN>.
*
* @param Context
*        <FILLMEIN>.
*
* @return TRUE if the object was deleted, FALSE otherwise.
*
* @remarks None.
*
*--*/
BOOLEAN
NTAPI
ObpDeleteEntryDirectory(POBP_LOOKUP_CONTEXT Context)
{
    POBJECT_DIRECTORY Directory;
    POBJECT_DIRECTORY_ENTRY *AllocatedEntry;
    POBJECT_DIRECTORY_ENTRY CurrentEntry;

    /* Get the Directory */
    Directory = Context->Directory;
    if (!Directory) return FALSE;

    /* Get the Entry */
    AllocatedEntry = &Directory->HashBuckets[Context->HashIndex];
    CurrentEntry = *AllocatedEntry;

    /* Unlink the Entry */
    *AllocatedEntry = CurrentEntry->ChainLink;
    CurrentEntry->ChainLink = NULL;

    /* Free it */
    ExFreePoolWithTag(CurrentEntry, OB_DIR_TAG);

    /* Return */
    return TRUE;
}

/* FUNCTIONS **************************************************************/

/*++
* @name NtOpenDirectoryObject
* @implemented NT4
*
*     The NtOpenDirectoryObject routine opens a namespace directory object.
*
* @param DirectoryHandle
*        Variable which receives the directory handle.
*
* @param DesiredAccess
*        Desired access to the directory.
*
* @param ObjectAttributes
*        Structure describing the directory.
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtOpenDirectoryObject(OUT PHANDLE DirectoryHandle,
                      IN ACCESS_MASK DesiredAccess,
                      IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    HANDLE Directory;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    PAGED_CODE();

    /* Check if we need to do any probing */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            /* Probe the return handle */
            ProbeForWriteHandle(DirectoryHandle);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Open the directory object */
    Status = ObOpenObjectByName(ObjectAttributes,
                                ObpDirectoryObjectType,
                                PreviousMode,
                                NULL,
                                DesiredAccess,
                                NULL,
                                &Directory);
    if (NT_SUCCESS(Status))
    {
        _SEH2_TRY
        {
            /* Write back the handle to the caller */
            *DirectoryHandle = Directory;
        }
        _SEH2_EXCEPT(ExSystemExceptionFilter())
        {
            /* Get the exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    /* Return the status to the caller */
    return Status;
}

/*++
* @name NtQueryDirectoryObject
* @implemented NT4
*
*     The NtQueryDirectoryObject routine reads information from a directory in
*     the system namespace.
*
* @param DirectoryHandle
*        Handle obtained with NtOpenDirectoryObject which
*        must grant DIRECTORY_QUERY access to the directory object.
*
* @param Buffer
*        Buffer to hold the data read.
*
* @param BufferLength
*        Size of the buffer in bytes.
*
* @param ReturnSingleEntry
*        When TRUE, only 1 entry is written in DirObjInformation;
*        otherwise as many as will fit in the buffer.
*
* @param RestartScan
*        If TRUE start reading at index 0.
*        If FALSE start reading at the index specified by *ObjectIndex.
*
* @param Context
*        Zero based index into the directory, interpretation
*        depends on RestartScan.
*
* @param ReturnLength
*        Caller supplied storage for the number of bytes
*        written (or NULL).
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks Although you can iterate over the directory by calling this
*          function multiple times, the directory is unlocked between
*          calls. This means that another thread can change the directory
*          and so iterating doesn't guarantee a consistent picture of the
*          directory. Best thing is to retrieve all directory entries in
*          one call.
*
*--*/
NTSTATUS
NTAPI
NtQueryDirectoryObject(IN HANDLE DirectoryHandle,
                       OUT PVOID Buffer,
                       IN ULONG BufferLength,
                       IN BOOLEAN ReturnSingleEntry,
                       IN BOOLEAN RestartScan,
                       IN OUT PULONG Context,
                       OUT PULONG ReturnLength OPTIONAL)
{
    POBJECT_DIRECTORY Directory;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    ULONG SkipEntries = 0;
    NTSTATUS Status;
    PVOID LocalBuffer;
    POBJECT_DIRECTORY_INFORMATION DirectoryInfo;
    ULONG Length, TotalLength;
    ULONG Count, CurrentEntry;
    ULONG Hash;
    POBJECT_DIRECTORY_ENTRY Entry;
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_NAME_INFO ObjectNameInfo;
    UNICODE_STRING Name;
    PWSTR p;
    OBP_LOOKUP_CONTEXT LookupContext;
    PAGED_CODE();

    /* Initialize lookup */
    ObpInitializeLookupContext(&LookupContext);

    /* Check if we need to do any probing */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            /* Probe the buffer (assuming it will hold Unicode characters) */
            ProbeForWrite(Buffer, BufferLength, sizeof(WCHAR));

            /* Probe the context and copy it unless scan-restart was requested */
            ProbeForWriteUlong(Context);
            if (!RestartScan) SkipEntries = *Context;

            /* Probe the return length if the caller specified one */
            if (ReturnLength) ProbeForWriteUlong(ReturnLength);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else if (!RestartScan)
    {
        /* This is kernel mode, save the context without probing, if needed */
        SkipEntries = *Context;
    }

    /* Allocate a buffer */
    LocalBuffer = ExAllocatePoolWithTag(PagedPool,
                                        sizeof(OBJECT_DIRECTORY_INFORMATION) +
                                        BufferLength,
                                        OB_NAME_TAG);
    if (!LocalBuffer) return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(LocalBuffer, BufferLength);

    /* Get a reference to directory */
    Status = ObReferenceObjectByHandle(DirectoryHandle,
                                       DIRECTORY_QUERY,
                                       ObpDirectoryObjectType,
                                       PreviousMode,
                                       (PVOID*)&Directory,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Free the buffer and fail */
        ExFreePoolWithTag(LocalBuffer, OB_NAME_TAG);
        return Status;
    }

    /* Lock the directory in shared mode */
    ObpAcquireDirectoryLockShared(Directory, &LookupContext);

    /* Start at position 0 */
    DirectoryInfo = (POBJECT_DIRECTORY_INFORMATION)LocalBuffer;
    TotalLength = sizeof(OBJECT_DIRECTORY_INFORMATION);

    /* Start with 0 entries */
    Count = 0;
    CurrentEntry = 0;

    /* Set default status and start looping */
    Status = STATUS_NO_MORE_ENTRIES;
    for (Hash = 0; Hash < 37; Hash++)
    {
        /* Get this entry and loop all of them */
        Entry = Directory->HashBuckets[Hash];
        while (Entry)
        {
            /* Check if we should process this entry */
            if (SkipEntries == CurrentEntry++)
            {
                /* Get the header data */
                ObjectHeader = OBJECT_TO_OBJECT_HEADER(Entry->Object);
                ObjectNameInfo = OBJECT_HEADER_TO_NAME_INFO(ObjectHeader);

                /* Get the object name */
                if (ObjectNameInfo)
                {
                    /* Use the one we have */
                    Name = ObjectNameInfo->Name;
                }
                else
                {
                    /* Otherwise, use an empty one */
                    RtlInitEmptyUnicodeString(&Name, NULL, 0);
                }

                /* Calculate the length for this entry */
                Length = sizeof(OBJECT_DIRECTORY_INFORMATION) +
                         Name.Length + sizeof(UNICODE_NULL) +
                         ObjectHeader->Type->Name.Length + sizeof(UNICODE_NULL);

                /* Make sure this entry won't overflow */
                if ((TotalLength + Length) > BufferLength)
                {
                    /* Check if the caller wanted only an entry */
                    if (ReturnSingleEntry)
                    {
                        /* Then we'll fail and ask for more buffer */
                        TotalLength += Length;
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }
                    else
                    {
                        /* Otherwise, we'll say we're done for now */
                        Status = STATUS_MORE_ENTRIES;
                    }

                    /* Decrease the entry since we didn't process */
                    CurrentEntry--;
                    goto Quickie;
                }

                /* Now fill in the buffer */
                DirectoryInfo->Name.Length = Name.Length;
                DirectoryInfo->Name.MaximumLength = Name.Length +
                                                    sizeof(UNICODE_NULL);
                DirectoryInfo->Name.Buffer = Name.Buffer;
                DirectoryInfo->TypeName.Length = ObjectHeader->
                                                 Type->Name.Length;
                DirectoryInfo->TypeName.MaximumLength = ObjectHeader->
                                                        Type->Name.Length +
                                                        sizeof(UNICODE_NULL);
                DirectoryInfo->TypeName.Buffer = ObjectHeader->
                                                 Type->Name.Buffer;

                /* Set success */
                Status = STATUS_SUCCESS;

                /* Increase statistics */
                TotalLength += Length;
                DirectoryInfo++;
                Count++;

                /* If the caller only wanted an entry, bail out */
                if (ReturnSingleEntry) goto Quickie;

                /* Increase the key by one */
                SkipEntries++;
            }

            /* Move to the next directory */
            Entry = Entry->ChainLink;
        }
    }

Quickie:
    /* Make sure we got success */
    if (NT_SUCCESS(Status))
    {
        /* Clear the current pointer and set it */
        RtlZeroMemory(DirectoryInfo, sizeof(OBJECT_DIRECTORY_INFORMATION));
        DirectoryInfo++;

        /* Set the buffer here now and loop entries */
        p = (PWSTR)DirectoryInfo;
        DirectoryInfo = LocalBuffer;
        while (Count--)
        {
            /* Copy the name buffer */
            RtlCopyMemory(p,
                          DirectoryInfo->Name.Buffer,
                          DirectoryInfo->Name.Length);

            /* Now fixup the pointers */
            DirectoryInfo->Name.Buffer = (PVOID)((ULONG_PTR)Buffer +
                                                 ((ULONG_PTR)p -
                                                  (ULONG_PTR)LocalBuffer));

            /* Advance in buffer and NULL-terminate */
            p = (PVOID)((ULONG_PTR)p + DirectoryInfo->Name.Length);
            *p++ = UNICODE_NULL;

            /* Now copy the type name buffer */
            RtlCopyMemory(p,
                          DirectoryInfo->TypeName.Buffer,
                          DirectoryInfo->TypeName.Length);

            /* Now fixup the pointers */
            DirectoryInfo->TypeName.Buffer = (PVOID)((ULONG_PTR)Buffer +
                                                     ((ULONG_PTR)p -
                                                     (ULONG_PTR)LocalBuffer));

            /* Advance in buffer and NULL-terminate */
            p = (PVOID)((ULONG_PTR)p + DirectoryInfo->TypeName.Length);
            *p++ = UNICODE_NULL;

            /* Move to the next entry */
            DirectoryInfo++;
        }

        /* Set the key */
        *Context = CurrentEntry;
    }

    _SEH2_TRY
    {
        /* Copy the buffer */
        RtlCopyMemory(Buffer,
                      LocalBuffer,
                      (TotalLength <= BufferLength) ?
                      TotalLength : BufferLength);

        /* Check if the caller requested the return length and return it*/
        if (ReturnLength) *ReturnLength = TotalLength;
    }
    _SEH2_EXCEPT(ExSystemExceptionFilter())
    {
        /* Get the exception code */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* Unlock the directory */
    ObpReleaseDirectoryLock(Directory, &LookupContext);

    /* Dereference the directory and free our buffer */
    ObDereferenceObject(Directory);
    ExFreePoolWithTag(LocalBuffer, OB_NAME_TAG);

    /* Return status to caller */
    return Status;
}

/*++
* @name NtCreateDirectoryObject
* @implemented NT4
*
*     The NtOpenDirectoryObject routine creates or opens a directory object.
*
* @param DirectoryHandle
*        Variable which receives the directory handle.
*
* @param DesiredAccess
*        Desired access to the directory.
*
* @param ObjectAttributes
*        Structure describing the directory.
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtCreateDirectoryObject(OUT PHANDLE DirectoryHandle,
                        IN ACCESS_MASK DesiredAccess,
                        IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    POBJECT_DIRECTORY Directory;
    HANDLE NewHandle;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    PAGED_CODE();

    /* Check if we need to do any probing */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            /* Probe the return handle */
            ProbeForWriteHandle(DirectoryHandle);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Create the object */
    Status = ObCreateObject(PreviousMode,
                            ObpDirectoryObjectType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(OBJECT_DIRECTORY),
                            0,
                            0,
                            (PVOID*)&Directory);
    if (!NT_SUCCESS(Status)) return Status;

    /* Setup the object */
    RtlZeroMemory(Directory, sizeof(OBJECT_DIRECTORY));
    ExInitializePushLock(&Directory->Lock);
    Directory->SessionId = -1;

    /* Insert it into the handle table */
    Status = ObInsertObject((PVOID)Directory,
                            NULL,
                            DesiredAccess,
                            0,
                            NULL,
                            &NewHandle);

    /* Enter SEH to protect write */
    _SEH2_TRY
    {
        /* Return the handle back to the caller */
        *DirectoryHandle = NewHandle;
    }
    _SEH2_EXCEPT(ExSystemExceptionFilter())
    {
        /* Get the exception code */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* Return status to caller */
    return Status;
}

/* EOF */

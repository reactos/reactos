/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ob/dirobj.c
 * PURPOSE:         Manages the Object Manager's Directory Implementation,
 *                  such as functions for addition, deletion and lookup into
 *                  the Object Manager's namespace. These routines are separate
 *                  from the Namespace Implementation because they are largely
 *                  independent and could be used for other namespaces.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Thomas Weidenmueller (w3seek@reactos.org)
 */

/* INCLUDES ***************************************************************/

#define NTDDI_VERSION NTDDI_WS03
#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#define OBP_PROFILE
#ifdef OBP_PROFILE

LARGE_INTEGER ObpProfileTime;
BOOLEAN ObpProfileComplete;

#define ObpStartProfile()                           \
    LARGE_INTEGER StartTime;                        \
    LARGE_INTEGER EndTime;                          \
    StartTime = KeQueryPerformanceCounter(NULL);

#define ObpEndProfile()                             \
    EndTime = KeQueryPerformanceCounter(NULL);      \
    ObpProfileTime.QuadPart += (EndTime.QuadPart -  \
                                StartTime.QuadPart);

#define ObpCompleteProfile()                        \
    if (!wcscmp(Name, L"NlsSectionCP1252") &&       \
        !ObpProfileComplete)                        \
    {                                               \
        DPRINT1("******************************\n");\
        DPRINT1("Obp Profiling1 Complete: %I64d\n", \
                ObpProfileTime.QuadPart);           \
        DPRINT1("******************************\n");\
        ObpProfileComplete = TRUE;                  \
    }

#else

#define ObpStartProfile()
#define ObpEndProfile()
#define ObpCompleteProfile()

#endif

POBJECT_TYPE ObDirectoryType = NULL;

/* PRIVATE FUNCTIONS ******************************************************/

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
    if ((Context->Object) || !(Context->DirectoryLocked) || !Parent)
    {
        DbgPrint("OB: ObpInsertEntryDirectory - invalid context %p %ld\n",
                 Context, Context->DirectoryLocked);
        DbgBreakPoint();
        return FALSE;
    }

    /* Allocate a new Directory Entry */
    NewEntry = ExAllocatePoolWithTag(PagedPool,
                                     sizeof(OBJECT_DIRECTORY_ENTRY),
                                     TAG('O', 'b', 'D', 'i'));
    if (!NewEntry) return FALSE;

    /* Save the hash */
    NewEntry->HashValue = Context->HashValue;

    /* Get the Object Name Information */
    HeaderNameInfo = HEADER_TO_OBJECT_NAME(ObjectHeader);

    /* Get the Allocated entry */
    AllocatedEntry = &Parent->HashBuckets[Context->HashIndex];
    DPRINT("ADD: Allocated Entry: %p. NewEntry: %p\n", AllocatedEntry, NewEntry);
    DPRINT("ADD: Name: %wZ, Hash: %lx\n", &HeaderNameInfo->Name, Context->HashIndex);
    DPRINT("ADD: Parent: %p. Name: %wZ\n",
            Parent,
            HEADER_TO_OBJECT_NAME(BODY_TO_HEADER(Parent)) ?
            &HEADER_TO_OBJECT_NAME(BODY_TO_HEADER(Parent))->Name : NULL);

    /* Set it */
    NewEntry->ChainLink = *AllocatedEntry;
    *AllocatedEntry = NewEntry;

    /* Associate the Object */
    NewEntry->Object = &ObjectHeader->Body;

    /* Associate the Directory */
    HeaderNameInfo->Directory = Parent;
    return TRUE;
}

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
    ULONG HashValue;
    ULONG HashIndex;
    LONG TotalChars;
    WCHAR CurrentChar;
    POBJECT_DIRECTORY_ENTRY *AllocatedEntry;
    POBJECT_DIRECTORY_ENTRY *LookupBucket;
    POBJECT_DIRECTORY_ENTRY CurrentEntry;
    PVOID FoundObject = NULL;
    PWSTR Buffer;
    PAGED_CODE();

    /* Always disable this until we have LUID Device Maps */
    SearchShadow = FALSE;

    /* Fail the following cases */
    TotalChars = Name->Length / sizeof(WCHAR);
    if (!(Directory) || !(Name) || !(Name->Buffer) || !(TotalChars))
    {
        goto Quickie;
    }

    /* Set up case-sensitivity */
    if (Attributes & OBJ_CASE_INSENSITIVE) CaseInsensitive = TRUE;

    /* Create the Hash */
    Buffer = Name->Buffer;
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
    DPRINT("LOOKUP: ObjectName: %wZ\n", Name);
    DPRINT("LOOKUP: Generated Hash: 0x%x. Generated Id: 0x%x\n", HashValue, HashIndex);

    /* Save the result */
    Context->HashValue = HashValue;
    Context->HashIndex = HashIndex;

    /* Get the root entry and set it as our lookup bucket */
    AllocatedEntry = &Directory->HashBuckets[HashIndex];
    LookupBucket = AllocatedEntry;
    DPRINT("LOOKUP: Allocated Entry: %p. LookupBucket: %p\n", AllocatedEntry, LookupBucket);

    /* Check if the directory is already locked */
    if (!Context->DirectoryLocked)
    {
        /* Lock it */
        KeEnterCriticalRegion();
        ExAcquireResourceSharedLite(&Directory->Lock, TRUE);
        Context->LockStateSignature = 0xDDDD1234;
    }

    /* Start looping */
    while ((CurrentEntry = *AllocatedEntry))
    {
        /* Do the hashes match? */
        DPRINT("CurrentEntry: %p. CurrentHash: %lx\n", CurrentEntry, CurrentEntry->HashValue);
        if (CurrentEntry->HashValue == HashValue)
        {
            /* Make sure that it has a name */
            ASSERT(BODY_TO_HEADER(CurrentEntry->Object)->NameInfoOffset != 0);

            /* Get the name information */
            HeaderNameInfo = HEADER_TO_OBJECT_NAME(BODY_TO_HEADER(CurrentEntry->Object));

            /* Do the names match? */
            DPRINT("NameCheck: %wZ, %wZ\n", Name, &HeaderNameInfo->Name);
            if ((Name->Length == HeaderNameInfo->Name.Length) &&
                (RtlEqualUnicodeString(Name, &HeaderNameInfo->Name, CaseInsensitive)))
            {
                DPRINT("Found Name Match\n");
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
            /* Set the Current Entry */
            *AllocatedEntry = CurrentEntry->ChainLink;

            /* Link to the old Hash Entry */
            CurrentEntry->ChainLink = *LookupBucket;

            /* Set the new Hash Entry */
            *LookupBucket = CurrentEntry;
        }

        /* Save the found object */
        FoundObject = CurrentEntry->Object;
        if (!FoundObject) goto Quickie;

        /* Add a reference to the object */
        ObReferenceObject(FoundObject);
    }

    /* Check if the directory was unlocked (which means we locked it) */
    if (!Context->DirectoryLocked)
    {
        /* Lock it */
        ExReleaseResourceLite(&Directory->Lock);
        KeLeaveCriticalRegion();
        Context->LockStateSignature = 0xEEEE1234;
    }

Quickie:
    /* Return the object we found */
    DPRINT("Object Found: %p Context: %p\n", FoundObject, Context);
    Context->Object = FoundObject;
    return FoundObject;
}

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
    DPRINT("DEL: Parent: %p, Hash: %lx, AllocatedEntry: %p, CurrentEntry: %p\n",
            Directory, Context->HashIndex, AllocatedEntry, CurrentEntry);

    /* Unlink the Entry */
    *AllocatedEntry = CurrentEntry->ChainLink;
    CurrentEntry->ChainLink = NULL;

    /* Free it */
    ExFreePool(CurrentEntry);

    /* Return */
    return TRUE;
}

NTSTATUS
NTAPI
ObpParseDirectory(PVOID Object,
                  PVOID * NextObject,
                  PUNICODE_STRING FullPath,
                  PWSTR * Path,
                  ULONG Attributes,
                  POBP_LOOKUP_CONTEXT Context)
{
    PWSTR Start;
    PWSTR End;
    PVOID FoundObject;
    //KIRQL oldlvl;
    UNICODE_STRING StartUs;

    *NextObject = NULL;

    if ((*Path) == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Start = *Path;
    if (*Start == L'\\')
        Start++;

    End = wcschr(Start, L'\\');
    if (End != NULL)
    {
        *End = 0;
    }

    //KeAcquireSpinLock(&(((PDIRECTORY_OBJECT)Object)->Lock), &oldlvl);
    RtlInitUnicodeString(&StartUs, Start);
    Context->DirectoryLocked = TRUE;
    Context->Directory = Object;
    FoundObject = ObpLookupEntryDirectory(Object, &StartUs, Attributes, FALSE, Context);
    if (FoundObject == NULL)
    {
        //KeReleaseSpinLock(&(((PDIRECTORY_OBJECT)Object)->Lock), oldlvl);
        if (End != NULL)
        {
            *End = L'\\';
        }
        return STATUS_UNSUCCESSFUL;
    }

    ObReferenceObjectByPointer(FoundObject,
        STANDARD_RIGHTS_REQUIRED,
        NULL,
        UserMode);
    //KeReleaseSpinLock(&(((PDIRECTORY_OBJECT)Object)->Lock), oldlvl);
    if (End != NULL)
    {
        *End = L'\\';
        *Path = End;
    }
    else
    {
        *Path = NULL;
    }

    *NextObject = FoundObject;

    return STATUS_SUCCESS;
}

/* FUNCTIONS **************************************************************/

/*++
* @name NtOpenDirectoryObject
* @implemented NT4
*
*     The NtOpenDirectoryObject opens a namespace directory object.
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
NtOpenDirectoryObject (OUT PHANDLE DirectoryHandle,
                       IN ACCESS_MASK DesiredAccess,
                       IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    HANDLE hDirectory;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWriteHandle(DirectoryHandle);
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status))
        {
            DPRINT1("NtOpenDirectoryObject failed, Status: 0x%x\n", Status);
            return Status;
        }
    }

    Status = ObOpenObjectByName(ObjectAttributes,
                                ObDirectoryType,
                                NULL,
                                PreviousMode,
                                DesiredAccess,
                                NULL,
                                &hDirectory);
    if(NT_SUCCESS(Status))
    {
        _SEH_TRY
        {
            *DirectoryHandle = hDirectory;
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }

    return Status;
}

/*++
* @name NtQueryDirectoryObject
* @implemented NT4
*
*     The NtQueryDirectoryObject Reads information from a directory in
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
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            /* a test showed that the Buffer pointer just has to be 16 bit aligned,
            propably due to the fact that most information that needs to be copied
            is unicode strings */
            ProbeForWrite(Buffer, BufferLength, sizeof(WCHAR));
            ProbeForWriteUlong(Context);
            if(!RestartScan)
            {
                SkipEntries = *Context;
            }
            if(ReturnLength != NULL)
            {
                ProbeForWriteUlong(ReturnLength);
            }
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status))
        {
            DPRINT1("NtQueryDirectoryObject failed, Status: 0x%x\n", Status);
            return Status;
        }
    }
    else if(!RestartScan)
    {
        SkipEntries = *Context;
    }

    Status = ObReferenceObjectByHandle(DirectoryHandle,
                                       DIRECTORY_QUERY,
                                       ObDirectoryType,
                                       PreviousMode,
                                       (PVOID*)&Directory,
                                       NULL);
    if(NT_SUCCESS(Status))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

/*++
* @name NtCreateDirectoryObject
* @implemented NT4
*
*     The NtOpenDirectoryObject creates or opens a directory object.
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
    HANDLE hDirectory;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    DPRINT("NtCreateDirectoryObject(DirectoryHandle %x, "
            "DesiredAccess %x, ObjectAttributes %x\n",
            DirectoryHandle, DesiredAccess, ObjectAttributes);

    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWriteHandle(DirectoryHandle);
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status))
        {
            DPRINT1("NtCreateDirectoryObject failed, Status: 0x%x\n", Status);
            return Status;
        }
    }

    Status = ObCreateObject(PreviousMode,
                            ObDirectoryType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(OBJECT_DIRECTORY),
                            0,
                            0,
                            (PVOID*)&Directory);

    if(NT_SUCCESS(Status))
    {
        Status = ObInsertObject((PVOID)Directory,
                                NULL,
                                DesiredAccess,
                                0,
                                NULL,
                                &hDirectory);
        if(NT_SUCCESS(Status))
        {
            _SEH_TRY
            {
                *DirectoryHandle = hDirectory;
            }
            _SEH_HANDLE
            {
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
        }

        ObDereferenceObject(Directory);
    }

    return Status;
}

/* EOF */

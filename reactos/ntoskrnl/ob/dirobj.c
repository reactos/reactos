/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ob/dirobj.c
 * PURPOSE:         Directory Object Implementation
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 */

/* INCLUDES ***************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

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

/* PRIVATE FUNCTIONS ******************************************************/

VOID
NTAPI
ObpAddEntryDirectory(PDIRECTORY_OBJECT Parent,
                     PROS_OBJECT_HEADER Header,
                     PWSTR Name)
{
    KIRQL oldlvl;

    ObpStartProfile();
    ASSERT(HEADER_TO_OBJECT_NAME(Header));
    HEADER_TO_OBJECT_NAME(Header)->Directory = Parent;

    KeAcquireSpinLock(&Parent->Lock, &oldlvl);
    InsertTailList(&Parent->head, &Header->Entry);
    KeReleaseSpinLock(&Parent->Lock, oldlvl);
    ObpEndProfile();
}

VOID
NTAPI
ObpRemoveEntryDirectory(PROS_OBJECT_HEADER Header)
{
    KIRQL oldlvl;

    ObpStartProfile();
    DPRINT("ObpRemoveEntryDirectory(Header %x)\n",Header);

    KeAcquireSpinLock(&(HEADER_TO_OBJECT_NAME(Header)->Directory->Lock),&oldlvl);
    if (Header->Entry.Flink && Header->Entry.Blink)
    {
        RemoveEntryList(&(Header->Entry));
        Header->Entry.Flink = Header->Entry.Blink = NULL;
    }
    KeReleaseSpinLock(&(HEADER_TO_OBJECT_NAME(Header)->Directory->Lock),oldlvl);
    ObpEndProfile();
}

NTSTATUS
NTAPI
ObpCreateDirectory(OB_OPEN_REASON Reason,
                   PEPROCESS Process,
                   PVOID ObjectBody,
                   ACCESS_MASK GrantedAccess,
                   ULONG HandleCount)
{
    PDIRECTORY_OBJECT Directory = ObjectBody;

    ObpStartProfile();
    if (Reason == ObCreateHandle)
    {
        InitializeListHead(&Directory->head);
        KeInitializeSpinLock(&Directory->Lock);
    }
    ObpEndProfile();

    return STATUS_SUCCESS;
}

PVOID
NTAPI
ObpFindEntryDirectory(PDIRECTORY_OBJECT DirectoryObject,
                      PWSTR Name,
                      ULONG Attributes)
{
    PLIST_ENTRY current = DirectoryObject->head.Flink;
    PROS_OBJECT_HEADER current_obj;

    ObpStartProfile();
    DPRINT("ObFindEntryDirectory(dir %x, name %S)\n",DirectoryObject, Name);
    ObpCompleteProfile();

    if (Name[0]==0)
    {
        ObpEndProfile();
        return(DirectoryObject);
    }
    if (Name[0]=='.' && Name[1]==0)
    {
        ObpEndProfile();
        return(DirectoryObject);
    }
    if (Name[0]=='.' && Name[1]=='.' && Name[2]==0)
    {
        ObpEndProfile();
        return(HEADER_TO_OBJECT_NAME(BODY_TO_HEADER(DirectoryObject))->Directory);
    }
    while (current!=(&(DirectoryObject->head)))
    {
        current_obj = CONTAINING_RECORD(current,ROS_OBJECT_HEADER,Entry);
        DPRINT("  Scanning: %S for: %S\n",HEADER_TO_OBJECT_NAME(current_obj)->Name.Buffer, Name);
        if (Attributes & OBJ_CASE_INSENSITIVE)
        {
            if (_wcsicmp(HEADER_TO_OBJECT_NAME(current_obj)->Name.Buffer, Name)==0)
            {
                DPRINT("Found it %x\n",&current_obj->Body);
                ObpEndProfile();
                return(&current_obj->Body);
            }
        }
        else
        {
            if ( wcscmp(HEADER_TO_OBJECT_NAME(current_obj)->Name.Buffer, Name)==0)
            {
                DPRINT("Found it %x\n",&current_obj->Body);
                ObpEndProfile();
                return(&current_obj->Body);
            }
        }
        current = current->Flink;
    }
    DPRINT("    Not Found: %s() = NULL\n",__FUNCTION__);
    ObpEndProfile();
    return(NULL);
}

NTSTATUS
NTAPI
ObpParseDirectory(PVOID Object,
                  PVOID * NextObject,
                  PUNICODE_STRING FullPath,
                  PWSTR * Path,
                  ULONG Attributes)
{
    PWSTR Start;
    PWSTR End;
    PVOID FoundObject;
    KIRQL oldlvl;

    ObpStartProfile();
    DPRINT("ObpParseDirectory(Object %x, Path %x, *Path %S)\n",
        Object,Path,*Path);

    *NextObject = NULL;

    if ((*Path) == NULL)
    {
        ObpEndProfile();
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

    KeAcquireSpinLock(&(((PDIRECTORY_OBJECT)Object)->Lock), &oldlvl);
    FoundObject = ObpFindEntryDirectory(Object, Start, Attributes);
    if (FoundObject == NULL)
    {
        KeReleaseSpinLock(&(((PDIRECTORY_OBJECT)Object)->Lock), oldlvl);
        if (End != NULL)
        {
            *End = L'\\';
        }
        ObpEndProfile();
        return STATUS_UNSUCCESSFUL;
    }

    ObReferenceObjectByPointer(FoundObject,
        STANDARD_RIGHTS_REQUIRED,
        NULL,
        UserMode);
    KeReleaseSpinLock(&(((PDIRECTORY_OBJECT)Object)->Lock), oldlvl);
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

    ObpEndProfile();
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
    PDIRECTORY_OBJECT Directory;
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
    PDIRECTORY_OBJECT Directory;
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
                            sizeof(DIRECTORY_OBJECT),
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
        if (!NT_SUCCESS(Status))
        {
            ObMakeTemporaryObject(Directory);
        }
        ObDereferenceObject(Directory);

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
    }

    return Status;
}

/* EOF */

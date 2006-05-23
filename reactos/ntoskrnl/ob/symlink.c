/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/ob/symlink.c
* PURPOSE:         Implements symbolic links
* PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
*                  David Welch (welch@mcmail.com)
*/

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, ObInitSymbolicLinkImplementation)
#endif

/* GLOBALS ******************************************************************/

POBJECT_TYPE ObSymbolicLinkType = NULL;

static GENERIC_MAPPING ObpSymbolicLinkMapping =
{
    STANDARD_RIGHTS_READ    | SYMBOLIC_LINK_QUERY,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE | SYMBOLIC_LINK_QUERY,
    SYMBOLIC_LINK_ALL_ACCESS
};

/* PRIVATE FUNCTIONS *********************************************************/

/*++
* @name ObpDeleteSymbolicLink
*
*     The ObpDeleteSymbolicLink routine <FILLMEIN>
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
ObpDeleteSymbolicLink(PVOID ObjectBody)
{
    PSYMLINK_OBJECT SymlinkObject = (PSYMLINK_OBJECT)ObjectBody;
    ExFreePool(SymlinkObject->TargetName.Buffer);
}

/*++
* @name ObpParseSymbolicLink
*
*     The ObpParseSymbolicLink routine <FILLMEIN>
*
* @param Object
*        <FILLMEIN>
*
* @param NextObject
*        <FILLMEIN>
*
* @param FullPath
*        <FILLMEIN>
*
* @param RemainingPath
*        <FILLMEIN>
*
* @param Attributes
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
ObpParseSymbolicLink(PVOID Object,
                     PVOID * NextObject,
                     PUNICODE_STRING FullPath,
                     PWSTR * RemainingPath,
                     ULONG Attributes)
{
    PSYMLINK_OBJECT SymlinkObject = (PSYMLINK_OBJECT) Object;
    UNICODE_STRING TargetPath;

    DPRINT("ObpParseSymbolicLink (RemainingPath %S)\n", *RemainingPath);

    /*
    * Stop parsing if the entire path has been parsed and
    * the desired object is a symbolic link object.
    */
    if (((*RemainingPath == NULL) || (**RemainingPath == 0)) &&
        (Attributes & OBJ_OPENLINK))
    {
        DPRINT("Parsing stopped!\n");
        *NextObject = NULL;
        return(STATUS_SUCCESS);
    }

    /* Build the expanded path */
    TargetPath.MaximumLength = SymlinkObject->TargetName.Length +
                               sizeof(WCHAR);
    if (RemainingPath && *RemainingPath)
    {
        TargetPath.MaximumLength += (wcslen(*RemainingPath) * sizeof(WCHAR));
    }
    TargetPath.Length = TargetPath.MaximumLength - sizeof(WCHAR);
    TargetPath.Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                              TargetPath.MaximumLength,
                                              TAG_SYMLINK_TTARGET);
    wcscpy(TargetPath.Buffer, SymlinkObject->TargetName.Buffer);
    if (RemainingPath && *RemainingPath)
    {
        wcscat(TargetPath.Buffer, *RemainingPath);
    }

    /* Transfer target path buffer into FullPath */
    ExFreePool(FullPath->Buffer);
    FullPath->Length = TargetPath.Length;
    FullPath->MaximumLength = TargetPath.MaximumLength;
    FullPath->Buffer = TargetPath.Buffer;

    /* Reinitialize RemainingPath for reparsing */
    *RemainingPath = FullPath->Buffer;

    *NextObject = NULL;
    return STATUS_REPARSE;
}

/*++
* @name ObInitSymbolicLinkImplementation
*
*     The ObInitSymbolicLinkImplementation routine <FILLMEIN>
*
* @param None.
*
* @return None.
*
* @remarks None.
*
*--*/
VOID
INIT_FUNCTION
NTAPI
ObInitSymbolicLinkImplementation(VOID)
{
    UNICODE_STRING Name;
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;

    DPRINT("Creating SymLink Object Type\n");

    /* Initialize the Directory type  */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"SymbolicLink");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(SYMLINK_OBJECT);
    ObjectTypeInitializer.GenericMapping = ObpSymbolicLinkMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.ValidAccessMask = SYMBOLIC_LINK_ALL_ACCESS;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    ObjectTypeInitializer.ParseProcedure = (OB_PARSE_METHOD)ObpParseSymbolicLink;
    ObjectTypeInitializer.DeleteProcedure = ObpDeleteSymbolicLink;
    ObpCreateTypeObject(&ObjectTypeInitializer, &Name, &ObSymbolicLinkType);
}

/* PUBLIC FUNCTIONS **********************************************************/

/*++
* @name NtCreateSymbolicLinkObject
* @implemented NT4
*
*     The NtCreateSymbolicLinkObject opens or creates a symbolic link object.
*
* @param LinkHandle
*        Variable which receives the symlink handle.
*
* @param DesiredAccess
*        Desired access to the symlink.
*
* @param ObjectAttributes
*        Structure describing the symlink.
*
* @param LinkTarget
*        Unicode string defining the symlink's target
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtCreateSymbolicLinkObject(OUT PHANDLE LinkHandle,
                           IN ACCESS_MASK DesiredAccess,
                           IN POBJECT_ATTRIBUTES ObjectAttributes,
                           IN PUNICODE_STRING LinkTarget)
{
    HANDLE hLink;
    PSYMLINK_OBJECT SymbolicLink;
    UNICODE_STRING CapturedLinkTarget;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    DPRINT("NtCreateSymbolicLinkObject(LinkHandle %p, DesiredAccess %ul"
           ", ObjectAttributes %p, LinkTarget %wZ)\n",
           LinkHandle,
           DesiredAccess,
           ObjectAttributes,
           LinkTarget);

    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWriteHandle(LinkHandle);
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    Status = ProbeAndCaptureUnicodeString(&CapturedLinkTarget,
                                          PreviousMode,
                                          LinkTarget);
    if(!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateSymbolicLinkObject: Capturing the target link failed!\n");
        return Status;
    }

    Status = ObCreateObject(PreviousMode,
                            ObSymbolicLinkType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(SYMLINK_OBJECT),
                            0,
                            0,
                            (PVOID*)&SymbolicLink);
    if (NT_SUCCESS(Status))
    {
        SymbolicLink->TargetName.Length = 0;
        SymbolicLink->TargetName.MaximumLength = CapturedLinkTarget.Length +
                                                 sizeof(WCHAR);
        SymbolicLink->TargetName.Buffer =
            ExAllocatePoolWithTag(NonPagedPool,
                                  SymbolicLink->TargetName.MaximumLength,
                                  TAG_SYMLINK_TARGET);

        RtlCopyUnicodeString(&SymbolicLink->TargetName, &CapturedLinkTarget);

        DPRINT("DeviceName %S\n", SymbolicLink->TargetName.Buffer);

        ZwQuerySystemTime (&SymbolicLink->CreateTime);

        Status = ObInsertObject((PVOID)SymbolicLink,
                                NULL,
                                DesiredAccess,
                                0,
                                NULL,
                                &hLink);
        if (NT_SUCCESS(Status))
        {
            _SEH_TRY
            {
                *LinkHandle = hLink;
            }
            _SEH_HANDLE
            {
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
        }
        ObDereferenceObject(SymbolicLink);
    }

    ReleaseCapturedUnicodeString(&CapturedLinkTarget, PreviousMode);
    return Status;
}

/*++
* @name NtOpenSymbolicLinkObject
* @implemented NT4
*
*     The NtOpenSymbolicLinkObject opens a symbolic link object.
*
* @param LinkHandle
*        Variable which receives the symlink handle.
*
* @param DesiredAccess
*        Desired access to the symlink.
*
* @param ObjectAttributes
*        Structure describing the symlink.
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtOpenSymbolicLinkObject(OUT PHANDLE LinkHandle,
                         IN ACCESS_MASK DesiredAccess,
                         IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    HANDLE hLink;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWriteHandle(LinkHandle);
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    DPRINT("NtOpenSymbolicLinkObject (Name %wZ)\n",
           ObjectAttributes->ObjectName);

    Status = ObOpenObjectByName(ObjectAttributes,
                                ObSymbolicLinkType,
                                NULL,
                                PreviousMode,
                                DesiredAccess,
                                NULL,
                                &hLink);
    if(NT_SUCCESS(Status))
    {
        _SEH_TRY
        {
            *LinkHandle = hLink;
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
* @name NtQuerySymbolicLinkObject
* @implemented NT4
*
*     The NtQuerySymbolicLinkObject queries a symbolic link object.
*
* @param LinkHandle
*        Symlink handle to query
*
* @param LinkTarget
*        Unicode string defining the symlink's target
*
* @param ResultLength
*        Caller supplied storage for the number of bytes written (or NULL).
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtQuerySymbolicLinkObject(IN HANDLE LinkHandle,
                          OUT PUNICODE_STRING LinkTarget,
                          OUT PULONG ResultLength OPTIONAL)
{
    UNICODE_STRING SafeLinkTarget;
    PSYMLINK_OBJECT SymlinkObject;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG LengthRequired;
    PAGED_CODE();

    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            /* probe the unicode string and buffers supplied */
            ProbeForWrite(LinkTarget, sizeof(UNICODE_STRING), sizeof(ULONG));

            SafeLinkTarget = *LinkTarget;

            ProbeForWrite(SafeLinkTarget.Buffer,
                          SafeLinkTarget.MaximumLength,
                          sizeof(WCHAR));

            if(ResultLength != NULL)
            {
                ProbeForWriteUlong(ResultLength);
            }
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status))
        {
            return Status;
        }
    }
    else
    {
        SafeLinkTarget = *LinkTarget;
    }

    Status = ObReferenceObjectByHandle(LinkHandle,
                                       SYMBOLIC_LINK_QUERY,
                                       ObSymbolicLinkType,
                                       PreviousMode,
                                       (PVOID *)&SymlinkObject,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        LengthRequired = SymlinkObject->TargetName.Length + sizeof(WCHAR);

        _SEH_TRY
        {
            if(SafeLinkTarget.MaximumLength >= LengthRequired)
            {
                /* 
                 * Don't pass TargetLink to RtlCopyUnicodeString here because
                 * the caller might have modified the structure which could 
                 * lead to a copy into kernel memory!
                 */
                RtlCopyUnicodeString(&SafeLinkTarget,
                                     &SymlinkObject->TargetName);
                SafeLinkTarget.Buffer[SafeLinkTarget.Length /
                                      sizeof(WCHAR)] = UNICODE_NULL;

                /* Copy back the new UNICODE_STRING structure */
                *LinkTarget = SafeLinkTarget;
            }
            else
            {
                Status = STATUS_BUFFER_TOO_SMALL;
            }

            if(ResultLength != NULL)
            {
                *ResultLength = LengthRequired;
            }
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        ObDereferenceObject(SymlinkObject);
    }

    return Status;
}

/* EOF */

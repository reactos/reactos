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
    POBJECT_SYMBOLIC_LINK SymlinkObject = (POBJECT_SYMBOLIC_LINK)ObjectBody;

    /* Make sure that the symbolic link has a name */
    if (SymlinkObject->LinkTarget.Buffer)
    {
        /* Free the name */
        ExFreePool(SymlinkObject->LinkTarget.Buffer);
        SymlinkObject->LinkTarget.Buffer = NULL;
    }
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
    POBJECT_SYMBOLIC_LINK SymlinkObject = (POBJECT_SYMBOLIC_LINK)Object;
    PUNICODE_STRING TargetPath;
    PWSTR NewTargetPath;
    ULONG LengthUsed, MaximumLength, RemainLength;

    /*
    * Stop parsing if the entire path has been parsed and
    * the desired object is a symbolic link object.
    */
    if (((*RemainingPath == NULL) || (**RemainingPath == 0)) &&
        (Attributes & OBJ_OPENLINK))
    {
        *NextObject = NULL;
        return(STATUS_SUCCESS);
    }

    /* Set the target path and length */
    TargetPath = &SymlinkObject->LinkTarget;
    RemainLength = *RemainingPath ? wcslen(*RemainingPath) * sizeof(WCHAR) : 0;
    LengthUsed = TargetPath->Length + RemainLength;

    /* Optimization: check if the new name is shorter */
    if (FullPath->MaximumLength <= LengthUsed)
    {
        /* It's not, allocate a new one */
        MaximumLength = LengthUsed + sizeof(WCHAR);
        NewTargetPath = ExAllocatePoolWithTag(NonPagedPool,
                                              MaximumLength,
                                              TAG_SYMLINK_TTARGET);
    }
    else
    {
        /* It is! Reuse the name... */
        MaximumLength = FullPath->MaximumLength;
        NewTargetPath = FullPath->Buffer;
    }

    /* Make sure we have a length */
    if (RemainLength)
    {
        /* Copy the new path */
        RtlMoveMemory((PVOID)((ULONG_PTR)NewTargetPath + TargetPath->Length),
                      *RemainingPath,
                      RemainLength);
    }

    /* Copy the target path and null-terminate it */
    RtlMoveMemory(NewTargetPath, TargetPath->Buffer, TargetPath->Length);
    NewTargetPath[LengthUsed / sizeof(WCHAR)] = UNICODE_NULL;

    /* If the optimization didn't work, free the old buffer */
    if (NewTargetPath != FullPath->Buffer) ExFreePool(FullPath->Buffer);

    /* Update the path values */
    FullPath->Length = LengthUsed;
    FullPath->MaximumLength = MaximumLength;
    FullPath->Buffer = NewTargetPath;

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

    /* Initialize the Directory type  */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"SymbolicLink");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(OBJECT_SYMBOLIC_LINK);
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
    POBJECT_SYMBOLIC_LINK SymbolicLink;
    UNICODE_STRING CapturedLinkTarget;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check if we need to probe parameters */
    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            /* Probe the target */
            ProbeForRead(LinkTarget, sizeof(UNICODE_STRING), sizeof(WCHAR));
            CapturedLinkTarget = *LinkTarget;
            ProbeForRead(CapturedLinkTarget.Buffer,
                         CapturedLinkTarget.MaximumLength,
                         sizeof(WCHAR));

            /* Probe the return handle */
            ProbeForWriteHandle(LinkHandle);
        }
        _SEH_HANDLE
        {
            /* Exception, get the error code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Probing failed, return the error code */
        if(!NT_SUCCESS(Status)) return Status;
    }
    else
    {
        /* No need to capture */
        CapturedLinkTarget = *LinkTarget;
    }

    /* Check if the maximum length is odd */
    if (CapturedLinkTarget.MaximumLength % sizeof(WCHAR))
    {
        /* Round it down */
        CapturedLinkTarget.MaximumLength =
            ALIGN_DOWN(CapturedLinkTarget.MaximumLength, WCHAR);
    }

    /* Fail if the length is odd, or if the maximum is smaller or 0 */
    if ((CapturedLinkTarget.Length % sizeof(WCHAR)) ||
        (CapturedLinkTarget.MaximumLength < CapturedLinkTarget.Length) ||
        !(CapturedLinkTarget.MaximumLength))
    {
        /* This message is displayed on the debugger in Windows */
        DbgPrint("OB: Invalid symbolic link target - %wZ\n",
                 &CapturedLinkTarget);
        return STATUS_INVALID_PARAMETER;
    }

    /* Create the object */
    Status = ObCreateObject(PreviousMode,
                            ObSymbolicLinkType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(OBJECT_SYMBOLIC_LINK),
                            0,
                            0,
                            (PVOID*)&SymbolicLink);
    if (NT_SUCCESS(Status))
    {
        /* Success! Fill in the creation time immediately */
        KeQuerySystemTime(&SymbolicLink->CreationTime);

        /* Setup the target name */
        SymbolicLink->LinkTarget.Length = CapturedLinkTarget.Length;
        SymbolicLink->LinkTarget.MaximumLength = CapturedLinkTarget.Length +
                                                 sizeof(WCHAR);
        SymbolicLink->LinkTarget.Buffer = 
            ExAllocatePoolWithTag(PagedPool,
                                  CapturedLinkTarget.MaximumLength,
                                  TAG_SYMLINK_TARGET);
        if (!SymbolicLink->LinkTarget.Buffer) return STATUS_NO_MEMORY;

        /* Copy it */
        RtlMoveMemory(SymbolicLink->LinkTarget.Buffer,
                      CapturedLinkTarget.Buffer,
                      CapturedLinkTarget.MaximumLength);

        /* Initialize the remaining name, dos drive index and target object */
        SymbolicLink->LinkTargetObject = NULL;
        SymbolicLink->DosDeviceDriveIndex = 0;
        RtlInitUnicodeString(&SymbolicLink->LinkTargetRemaining, NULL);

        /* Insert it into the object tree */
        Status = ObInsertObject(SymbolicLink,
                                NULL,
                                DesiredAccess,
                                0,
                                NULL,
                                &hLink);
        if (NT_SUCCESS(Status))
        {
            _SEH_TRY
            {
                /* Return the handle to caller */
                *LinkHandle = hLink;
            }
            _SEH_HANDLE
            {
                /* Get exception code */
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
        }

        /* ReactOS Hack: Our ObInsertObject references an object twice */
        ObDereferenceObject(SymbolicLink);
    }

    /* Return status to caller */
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

    /* Check if we need to probe parameters */
    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            /* Probe the return handle */
            ProbeForWriteHandle(LinkHandle);
        }
        _SEH_HANDLE
        {
            /* Exception, get the error code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Probing failed, return the error code */
        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Open the object */
    Status = ObOpenObjectByName(ObjectAttributes,
                                ObSymbolicLinkType,
                                NULL,
                                PreviousMode,
                                DesiredAccess,
                                NULL,
                                &hLink);
    if (NT_SUCCESS(Status))
    {
        _SEH_TRY
        {
            /* Return the handle to caller */
            *LinkHandle = hLink;
        }
        _SEH_HANDLE
        {
            /* Get exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }

    /* Return status to caller */
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
    POBJECT_SYMBOLIC_LINK SymlinkObject;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG LengthUsed;
    PAGED_CODE();

    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            /* Probe the unicode string for read and write */
            ProbeForRead(LinkTarget, sizeof(UNICODE_STRING), sizeof(WCHAR));
            ProbeForWriteUshort(&LinkTarget->Length);
            ProbeForWriteUshort(&LinkTarget->MaximumLength);

            /* Probe the unicode string's buffer for write */
            SafeLinkTarget = *LinkTarget;
            ProbeForWrite(SafeLinkTarget.Buffer,
                          SafeLinkTarget.MaximumLength,
                          sizeof(CHAR));

            /* Probe the return length */
            if(ResultLength) ProbeForWriteUlong(ResultLength);
        }
        _SEH_HANDLE
        {
            /* Probe failure: get exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Probe failed, return status */
        if(!NT_SUCCESS(Status)) return Status;
    }
    else
    {
        /* No need to probe */
        SafeLinkTarget = *LinkTarget;
    }

    /* Reference the object */
    Status = ObReferenceObjectByHandle(LinkHandle,
                                       SYMBOLIC_LINK_QUERY,
                                       ObSymbolicLinkType,
                                       PreviousMode,
                                       (PVOID *)&SymlinkObject,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Lock the object type */
        KeEnterCriticalRegion();
        ExAcquireResourceExclusiveLite(&ObSymbolicLinkType->Mutex, TRUE);

        /*
         * So here's the thing: If you specify a return length, then the
         * implementation will use the maximum length. If you don't, then
         * it will use the length.
         */
        LengthUsed = ResultLength ? SymlinkObject->LinkTarget.MaximumLength :
                                    SymlinkObject->LinkTarget.Length;

        /* Enter SEH so we can safely copy */
        _SEH_TRY
        {
            /* Make sure our buffer will fit */
            if (LengthUsed <= SafeLinkTarget.MaximumLength)
            {
                /* Copy the buffer */
                RtlMoveMemory(SafeLinkTarget.Buffer,
                              SymlinkObject->LinkTarget.Buffer,
                              LengthUsed);

                /* Copy the new length */
                LinkTarget->Length = SymlinkObject->LinkTarget.Length;
            }
            else
            {
                /* Otherwise set the failure status */
                Status = STATUS_BUFFER_TOO_SMALL;
            }

            /* In both cases, check if the required length was requested */
            if (ResultLength)
            {
                /* Then return it */
                *ResultLength = SymlinkObject->LinkTarget.MaximumLength;
            }
        }
        _SEH_HANDLE
        {
            /* Get the error code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Unlock the object type and reference the object */
        ExReleaseResourceLite(&ObSymbolicLinkType->Mutex);
        KeLeaveCriticalRegion();
        ObDereferenceObject(SymlinkObject);
    }

    /* Return query status */
    return Status;
}

/* EOF */

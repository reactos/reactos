/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/ob/ntobj.c
* PURPOSE:         Bunch of random functions that got stuck here without purpose. FIXME.
* PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
*/

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

/*++
* @name ObpSetPermanentObject
*
*     The ObpSetPermanentObject routine <FILLMEIN>
*
* @param ObjectBody
*        <FILLMEIN>
*
* @param Permanent
*        <FILLMEIN>
*
* @return None.
*
* @remarks None.
*
*--*/
VOID
FASTCALL
ObpSetPermanentObject(IN PVOID ObjectBody,
                      IN BOOLEAN Permanent)
{
    PROS_OBJECT_HEADER ObjectHeader;
    OBP_LOOKUP_CONTEXT Context;

    ObjectHeader = BODY_TO_HEADER(ObjectBody);
    ASSERT (ObjectHeader->PointerCount > 0);
    if (Permanent)
    {
        ObjectHeader->Flags |= OB_FLAG_PERMANENT;
    }
    else
    {
        ObjectHeader->Flags &= ~OB_FLAG_PERMANENT;
        if (ObjectHeader->HandleCount == 0 &&
            HEADER_TO_OBJECT_NAME(ObjectHeader)->Directory)
        {
            /* Make sure it's still inserted */
            Context.Directory = HEADER_TO_OBJECT_NAME(ObjectHeader)->Directory;
            Context.DirectoryLocked = TRUE;
            if (ObpLookupEntryDirectory(HEADER_TO_OBJECT_NAME(ObjectHeader)->Directory,
                                        &HEADER_TO_OBJECT_NAME(ObjectHeader)->Name,
                                        0,
                                        FALSE,
                                        &Context))
            {
                ObpDeleteEntryDirectory(&Context);
            }
        }
    }
}

/* PUBLIC FUNCTIONS **********************************************************/

/*++
* @name ObMakeTemporaryObject
* @implemented NT4
*
*     The ObMakeTemporaryObject routine <FILLMEIN>
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
ObMakeTemporaryObject(IN PVOID ObjectBody)
{
    ObpSetPermanentObject (ObjectBody, FALSE);
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
    PROS_OBJECT_HEADER ObjectHeader;
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
* @name NtMakeTemporaryObject
* @implemented NT4
*
*     The NtMakeTemporaryObject routine <FILLMEIN>
*
* @param ObjectHandle
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtMakeTemporaryObject(IN HANDLE ObjectHandle)
{
    PVOID ObjectBody;
    NTSTATUS Status;
    PAGED_CODE();

    Status = ObReferenceObjectByHandle(ObjectHandle,
                                       0,
                                       NULL,
                                       KeGetPreviousMode(),
                                       &ObjectBody,
                                       NULL);
    if (Status != STATUS_SUCCESS) return Status;

    ObpSetPermanentObject (ObjectBody, FALSE);

    ObDereferenceObject(ObjectBody);

    return STATUS_SUCCESS;
}

/*++
* @name NtMakePermanentObject
* @implemented NT4
*
*     The NtMakePermanentObject routine <FILLMEIN>
*
* @param ObjectHandle
*        <FILLMEIN>
*
* @return STATUS_SUCCESS or appropriate error value.
*
* @remarks None.
*
*--*/
NTSTATUS
NTAPI
NtMakePermanentObject(IN HANDLE ObjectHandle)
{
    PVOID ObjectBody;
    NTSTATUS Status;
    PAGED_CODE();

    Status = ObReferenceObjectByHandle(ObjectHandle,
                                       0,
                                       NULL,
                                       KeGetPreviousMode(),
                                       &ObjectBody,
                                       NULL);
    if (Status != STATUS_SUCCESS) return Status;

    ObpSetPermanentObject (ObjectBody, TRUE);

    ObDereferenceObject(ObjectBody);

    return STATUS_SUCCESS;
}

/* EOF */

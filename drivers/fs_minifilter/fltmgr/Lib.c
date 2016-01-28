/*
* PROJECT:         Filesystem Filter Manager
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            drivers/fs_minifilter/fltmgr/Lib.c
* PURPOSE:         Miscellaneous library functions
* PROGRAMMERS:     Ged Murphy (gedmurphy@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include "fltmgr.h"

#define NDEBUG
#include <debug.h>


/* DATA *********************************************************************/



/* FUNCTIONS **********************************************/

NTSTATUS
FltpGetBaseDeviceObjectName(_In_ PDEVICE_OBJECT DeviceObject,
                            _Inout_ PUNICODE_STRING ObjectName)
{
    PDEVICE_OBJECT BaseDeviceObject;
    NTSTATUS Status;

    /*
     * Get the lowest device object on the stack, which may be the
     * object we were passed, and lookup the name for that object
     */
    BaseDeviceObject = IoGetDeviceAttachmentBaseRef(DeviceObject);
    Status = FltpGetObjectName(BaseDeviceObject, ObjectName);
    ObDereferenceObject(BaseDeviceObject);

    return Status;
}

NTSTATUS
FltpGetObjectName(_In_ PVOID Object,
                  _Inout_ PUNICODE_STRING ObjectName)
{
    POBJECT_NAME_INFORMATION ObjectNameInfo = NULL;
    OBJECT_NAME_INFORMATION LocalNameInfo;
    ULONG ReturnLength;
    NTSTATUS Status;

    if (ObjectName == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Get the size of the buffer required to hold the nameinfo */
    Status = ObQueryNameString(Object,
                              &LocalNameInfo,
                              sizeof(LocalNameInfo),
                              &ReturnLength);
    if (Status == STATUS_INFO_LENGTH_MISMATCH)
    {
        ObjectNameInfo = ExAllocatePoolWithTag(PagedPool,
                                               ReturnLength,
                                               FM_TAG_UNICODE_STRING);
        if (ObjectNameInfo == NULL) return STATUS_INSUFFICIENT_RESOURCES;

        /* Get the actual name info now we have the buffer to hold it */
        Status = ObQueryNameString(Object,
                                    ObjectNameInfo,
                                    ReturnLength,
                                    &ReturnLength);
    }


    if (NT_SUCCESS(Status))
    {
        /* Make sure the buffer we were passed is large enough to hold the string */
        if (ObjectName->MaximumLength < ObjectNameInfo->Name.Length)
        {
            /* It wasn't, let's enlarge the buffer */
            Status = FltpReallocateUnicodeString(ObjectName,
                                                 ObjectNameInfo->Name.Length,
                                                 FALSE);
            
        }

        if (NT_SUCCESS(Status))
        {
            /* Copy the object name into the callers buffer */
            RtlCopyUnicodeString(ObjectName, &ObjectNameInfo->Name);
        }
    }

    if (ObjectNameInfo)
    {
        ExFreePoolWithTag(ObjectNameInfo, FM_TAG_UNICODE_STRING);
    }

    return Status;
}

VOID
FltpFreeUnicodeString(_In_ PUNICODE_STRING String)
{
    /* Free up any existing buffer */
    if (String->Buffer)
    {
        ExFreePoolWithTag(String->Buffer, FM_TAG_UNICODE_STRING);
    }

    /* Empty the string */
    String->Buffer = NULL;
    String->Length = 0;
    String->MaximumLength = 0;
}

NTSTATUS
FltpReallocateUnicodeString(_In_ PUNICODE_STRING String,
                            _In_ SIZE_T NewLength,
                            _In_ BOOLEAN CopyExisting)
{
    PWCH NewBuffer;

    /* Don't bother reallocating if the buffer is smaller */
    if (NewLength <= String->MaximumLength)
    {
        String->Length = 0;
        return STATUS_SUCCESS;
    }

    /* Allocate a new buffer at the size requested */
    NewBuffer = ExAllocatePoolWithTag(PagedPool, NewLength, FM_TAG_UNICODE_STRING);
    if (NewBuffer == NULL) return STATUS_INSUFFICIENT_RESOURCES;

    if (CopyExisting)
    {
        /* Copy the old data across */
        RtlCopyMemory(NewBuffer, String->Buffer, String->Length);
    }
    else
    {
        /* Reset the length */
        String->Length = 0;
    }

    /* Free any old buffer */
    if (String->Buffer)
        ExFreePoolWithTag(String->Buffer, FM_TAG_UNICODE_STRING);

    /* Update the lengths */
    String->Buffer = NewBuffer;
    String->MaximumLength = NewLength;

    return STATUS_SUCCESS;
}

NTSTATUS
FltpCopyUnicodeString(_In_ PUNICODE_STRING StringOne,
                      _In_ PUNICODE_STRING StringTwo)
{
    return 0;
}

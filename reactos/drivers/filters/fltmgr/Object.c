/*
* PROJECT:         Filesystem Filter Manager
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            drivers/filters/fltmgr/Object.c
* PURPOSE:         Miscellaneous library functions
* PROGRAMMERS:     Ged Murphy (gedmurphy@reactos.org)
*/

// NOTE: Split this file into filter object and device object functions
// when the code base grows sufficiently

/* INCLUDES ******************************************************************/

#include "fltmgr.h"
#include "fltmgrint.h"

#define NDEBUG
#include <debug.h>


/* DATA *********************************************************************/



/* EXPORTED FUNCTIONS ******************************************************/


NTSTATUS
FLTAPI
FltObjectReference(_Inout_ PVOID Object)
{
    if (!FltpExAcquireRundownProtection(&((PFLT_OBJECT)Object)->RundownRef))
    {
        return STATUS_FLT_DELETING_OBJECT;
    }

    return STATUS_SUCCESS;
}

VOID
FLTAPI
FltObjectDereference(_Inout_ PVOID Object)
{
    FltpExReleaseRundownProtection(&((PFLT_OBJECT)Object)->RundownRef);
}



/* INTERNAL FUNCTIONS ******************************************************/

VOID
FltpExInitializeRundownProtection(_Out_ PEX_RUNDOWN_REF RundownRef)
{
    ExInitializeRundownProtection(RundownRef);
}

BOOLEAN
FltpExAcquireRundownProtection(_Inout_ PEX_RUNDOWN_REF RundownRef)
{
    return ExAcquireRundownProtection(RundownRef);
}

BOOLEAN
FltpExReleaseRundownProtection(_Inout_ PEX_RUNDOWN_REF RundownRef)
{
    ExReleaseRundownProtection(RundownRef);
    return TRUE;
}

BOOLEAN
FltpExRundownCompleted(_Inout_ PEX_RUNDOWN_REF RundownRef)
{
    return _InterlockedExchange((PLONG)RundownRef, 1);
}

NTSTATUS
NTAPI
FltpObjectRundownWait(_Inout_ PEX_RUNDOWN_REF RundownRef)
{
    //return FltpExWaitForRundownProtectionRelease(RundownRef);
    return 0;
}

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

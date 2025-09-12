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

#define ExpChangePushlock(x, y, z) InterlockedCompareExchangePointer((PVOID*)x, (PVOID)y, (PVOID)z)

//
// Pushlock bits
//
#define EX_PUSH_LOCK_LOCK_V             ((ULONG_PTR)0x0)
#define EX_PUSH_LOCK_LOCK               ((ULONG_PTR)0x1)
#define EX_PUSH_LOCK_WAITING            ((ULONG_PTR)0x2)
#define EX_PUSH_LOCK_WAKING             ((ULONG_PTR)0x4)
#define EX_PUSH_LOCK_MULTIPLE_SHARED    ((ULONG_PTR)0x8)
#define EX_PUSH_LOCK_SHARE_INC          ((ULONG_PTR)0x10)
#define EX_PUSH_LOCK_PTR_BITS           ((ULONG_PTR)0xf)

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


_Acquires_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltAcquirePushLockExclusive(_Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_) PEX_PUSH_LOCK PushLock)
{
    KeEnterCriticalRegion();

    /* Try acquiring the lock */
    if (InterlockedBitTestAndSet((PLONG)PushLock, EX_PUSH_LOCK_LOCK_V))
    {
        /* Someone changed it, use the slow path */
        ExfAcquirePushLockExclusive(PushLock);
    }

    /* Sanity check */
    FLT_ASSERT(PushLock->Locked);
}


_Acquires_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltAcquirePushLockShared(_Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_) PEX_PUSH_LOCK PushLock)
{
    EX_PUSH_LOCK NewValue;

    KeEnterCriticalRegion();

    /* Try acquiring the lock */
    NewValue.Value = EX_PUSH_LOCK_LOCK | EX_PUSH_LOCK_SHARE_INC;
    if (ExpChangePushlock(PushLock, NewValue.Ptr, 0))
    {
        /* Someone changed it, use the slow path */
        ExfAcquirePushLockShared(PushLock);
    }

    /* Sanity checks */
    ASSERT(PushLock->Locked);
}

_Releases_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltReleasePushLock(_Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_) PEX_PUSH_LOCK PushLock)
{
    EX_PUSH_LOCK OldValue = *PushLock;
    EX_PUSH_LOCK NewValue;

    /* Sanity checks */
    FLT_ASSERT(OldValue.Locked);

    /* Check if the pushlock is shared */
    if (OldValue.Shared > 1)
    {
        /* Decrease the share count */
        NewValue.Value = OldValue.Value - EX_PUSH_LOCK_SHARE_INC;
    }
    else
    {
        /* Clear the pushlock entirely */
        NewValue.Value = 0;
    }

    /* Check if nobody is waiting on us and try clearing the lock here */
    if ((OldValue.Waiting) ||
        (ExpChangePushlock(PushLock, NewValue.Ptr, OldValue.Ptr) !=
         OldValue.Ptr))
    {
        /* We have waiters, use the long path */
        ExfReleasePushLock(PushLock);
    }

    KeLeaveCriticalRegion();
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltClose(_In_ HANDLE FileHandle)
{
    PAGED_CODE();

    return ZwClose(FileHandle);
}

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltCreateFileEx(_In_ PFLT_FILTER Filter,
                _In_opt_ PFLT_INSTANCE Instance,
                _Out_ PHANDLE FileHandle,
                _Outptr_opt_ PFILE_OBJECT *FileObject,
                _In_ ACCESS_MASK DesiredAccess,
                _In_ POBJECT_ATTRIBUTES ObjectAttributes,
                _Out_ PIO_STATUS_BLOCK IoStatusBlock,
                _In_opt_ PLARGE_INTEGER AllocationSize,
                _In_ ULONG FileAttributes,
                _In_ ULONG ShareAccess,
                _In_ ULONG CreateDisposition,
                _In_ ULONG CreateOptions,
                _In_reads_bytes_opt_(EaLength) PVOID EaBuffer,
                _In_ ULONG EaLength,
                _In_ ULONG Flags)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltCreateFile(_In_ PFLT_FILTER Filter,
              _In_opt_ PFLT_INSTANCE Instance,
              _Out_ PHANDLE FileHandle,
              _In_ ACCESS_MASK DesiredAccess,
              _In_ POBJECT_ATTRIBUTES ObjectAttributes,
              _Out_ PIO_STATUS_BLOCK IoStatusBlock,
              _In_opt_ PLARGE_INTEGER AllocationSize,
              _In_ ULONG FileAttributes,
              _In_ ULONG ShareAccess,
              _In_ ULONG CreateDisposition,
              _In_ ULONG CreateOptions,
              _In_reads_bytes_opt_(EaLength)PVOID EaBuffer,
              _In_ ULONG EaLength,
              _In_ ULONG Flags)
{
    return FltCreateFileEx(Filter,
                           Instance,
                           FileHandle,
                           NULL,
                           DesiredAccess,
                           ObjectAttributes,
                           IoStatusBlock,
                           AllocationSize,
                           FileAttributes,
                           ShareAccess,
                           CreateDisposition,
                           CreateOptions,
                           EaBuffer,
                           EaLength,
                           Flags);
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
    OBJECT_NAME_INFORMATION LocalNameInfo;
    POBJECT_NAME_INFORMATION ObjectNameInfo = &LocalNameInfo;
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

    if (ObjectNameInfo != &LocalNameInfo)
    {
        ExFreePoolWithTag(ObjectNameInfo, FM_TAG_UNICODE_STRING);
    }

    return Status;
}

ULONG
FltpObjectPointerReference(_In_ PFLT_OBJECT Object)
{
    PULONG Result;

    /* Store the old count and increment */
    Result = &Object->PointerCount;
    InterlockedIncrementSizeT(&Object->PointerCount);

    /* Return the initial value */
    return *Result;
}

VOID
FltpObjectPointerDereference(_In_ PFLT_OBJECT Object)
{
    if (InterlockedDecrementSizeT(&Object->PointerCount) == 0)
    {
        // Cleanup
        FLT_ASSERT(FALSE);
    }
}

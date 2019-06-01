/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ob/devicemap.c
 * PURPOSE:         Device map implementation
 * PROGRAMMERS:     Eric Kohl (eric.kohl@reactos.org)
 *                  Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS ******************************************************/

NTSTATUS
NTAPI
ObSetDeviceMap(IN PEPROCESS Process,
               IN HANDLE DirectoryHandle)
{
    POBJECT_DIRECTORY DirectoryObject = NULL;
    PDEVICE_MAP DeviceMap = NULL, NewDeviceMap = NULL, OldDeviceMap;
    NTSTATUS Status;
    PEPROCESS WorkProcess;
    BOOLEAN MakePermanant = FALSE;

    Status = ObReferenceObjectByHandle(DirectoryHandle,
                                       DIRECTORY_TRAVERSE,
                                       ObpDirectoryObjectType,
                                       KeGetPreviousMode(),
                                       (PVOID*)&DirectoryObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ObReferenceObjectByHandle() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Allocate and initialize a new device map */
    DeviceMap = ExAllocatePoolWithTag(PagedPool,
                                      sizeof(*DeviceMap),
                                      'mDbO');
    if (DeviceMap == NULL)
    {
        ObDereferenceObject(DirectoryObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Initialize the device map */
    RtlZeroMemory(DeviceMap, sizeof(*DeviceMap));
    DeviceMap->ReferenceCount = 1;
    DeviceMap->DosDevicesDirectory = DirectoryObject;

    /* Acquire the device map lock */
    KeAcquireGuardedMutex(&ObpDeviceMapLock);

    /* Attach the device map to the directory object */
    if (DirectoryObject->DeviceMap == NULL)
    {
        DirectoryObject->DeviceMap = DeviceMap;
    }
    else
    {
        NewDeviceMap = DeviceMap;

        /* There's already a device map,
           so reuse it */
        DeviceMap = DirectoryObject->DeviceMap;
        ++DeviceMap->ReferenceCount;
    }

    /* Caller gave a process, use it */
    if (Process != NULL)
    {
        WorkProcess = Process;
    }
    /* If no process given, use current and
     * set system device map */
    else
    {
        WorkProcess = PsGetCurrentProcess();
        ObSystemDeviceMap = DeviceMap;
    }

    /* If current object isn't system one, save system one in current
     * device map */
    if (DirectoryObject != ObSystemDeviceMap->DosDevicesDirectory)
    {
        /* We also need to make the object permanant */
        DeviceMap->GlobalDosDevicesDirectory = ObSystemDeviceMap->DosDevicesDirectory;
        MakePermanant = TRUE;
    }

    /* Save old process device map */
    OldDeviceMap = WorkProcess->DeviceMap;
    /* Attach the device map to the process */
    WorkProcess->DeviceMap = DeviceMap;

    /* Release the device map lock */
    KeReleaseGuardedMutex(&ObpDeviceMapLock);

    /* If we have to make the object permamant, do it now */
    if (MakePermanant)
    {
        POBJECT_HEADER ObjectHeader;
        POBJECT_HEADER_NAME_INFO HeaderNameInfo;

        ObjectHeader = OBJECT_TO_OBJECT_HEADER(DirectoryObject);
        HeaderNameInfo = ObpReferenceNameInfo(ObjectHeader);

        ObpEnterObjectTypeMutex(ObjectHeader->Type);
        if (HeaderNameInfo != NULL && HeaderNameInfo->Directory != NULL)
        {
            ObjectHeader->Flags |= OB_FLAG_PERMANENT;
        }
        ObpLeaveObjectTypeMutex(ObjectHeader->Type);

        if (HeaderNameInfo != NULL)
        {
            ObpDereferenceNameInfo(HeaderNameInfo);
        }
    }

    /* Release useless device map if required */
    if (NewDeviceMap != NULL)
    {
        ObfDereferenceObject(DirectoryObject);
        ExFreePoolWithTag(NewDeviceMap, 'mDbO');
    }

    /* And dereference previous process device map */
    if (OldDeviceMap != NULL)
    {
        ObfDereferenceDeviceMap(OldDeviceMap);
    }

    return STATUS_SUCCESS;
}


VOID
NTAPI
ObDereferenceDeviceMap(IN PEPROCESS Process)
{
    PDEVICE_MAP DeviceMap;

    DPRINT("ObDereferenceDeviceMap()\n");

    /* Get the pointer to this process devicemap and reset it
       holding the device map lock */
    KeAcquireGuardedMutex(&ObpDeviceMapLock);
    DeviceMap = Process->DeviceMap;
    Process->DeviceMap = NULL;
    KeReleaseGuardedMutex(&ObpDeviceMapLock);

    /* Continue only if there is a device map */
    if (DeviceMap == NULL)
        return;

    /* Acquire the device map lock again */
    KeAcquireGuardedMutex(&ObpDeviceMapLock);

    /* Decrement the reference counter */
    DeviceMap->ReferenceCount--;
    DPRINT("ReferenceCount: %lu\n", DeviceMap->ReferenceCount);

    /* Leave, if there are still references to this device map */
    if (DeviceMap->ReferenceCount != 0)
    {
        /* Release the device map lock and leave */
        KeReleaseGuardedMutex(&ObpDeviceMapLock);
        return;
    }

    /* Nobody is referencing it anymore, unlink the DOS directory */
    DeviceMap->DosDevicesDirectory->DeviceMap = NULL;

    /* Release the device map lock */
    KeReleaseGuardedMutex(&ObpDeviceMapLock);

    /* Dereference the DOS Devices Directory and free the DeviceMap */
    ObDereferenceObject(DeviceMap->DosDevicesDirectory);
    ExFreePoolWithTag(DeviceMap, 'mDbO');
}


VOID
FASTCALL
ObfDereferenceDeviceMap(IN PDEVICE_MAP DeviceMap)
{
    DPRINT("ObfDereferenceDeviceMap()\n");

    /* Acquire the device map lock */
    KeAcquireGuardedMutex(&ObpDeviceMapLock);

    /* Decrement the reference counter */
    DeviceMap->ReferenceCount--;
    DPRINT("ReferenceCount: %lu\n", DeviceMap->ReferenceCount);

    /* Leave, if there are still references to this device map */
    if (DeviceMap->ReferenceCount != 0)
    {
        /* Release the device map lock and leave */
        KeReleaseGuardedMutex(&ObpDeviceMapLock);
        return;
    }

    /* Nobody is referencing it anymore, unlink the DOS directory */
    DeviceMap->DosDevicesDirectory->DeviceMap = NULL;

    /* Release the devicemap lock */
    KeReleaseGuardedMutex(&ObpDeviceMapLock);

    /* Dereference the DOS Devices Directory and free the Device Map */
    ObDereferenceObject(DeviceMap->DosDevicesDirectory );
    ExFreePoolWithTag(DeviceMap, 'mDbO');
}


VOID
NTAPI
ObInheritDeviceMap(IN PEPROCESS Parent,
                   IN PEPROCESS Process)
{
    PDEVICE_MAP DeviceMap;

    DPRINT("ObInheritDeviceMap()\n");

    /* Acquire the device map lock */
    KeAcquireGuardedMutex(&ObpDeviceMapLock);

    /* Get the parent process device map or the system device map */
    DeviceMap = (Parent != NULL) ? Parent->DeviceMap : ObSystemDeviceMap;
    if (DeviceMap != NULL)
    {
        /* Reference the device map and attach it to the new process */
        DeviceMap->ReferenceCount++;
        DPRINT("ReferenceCount: %lu\n", DeviceMap->ReferenceCount);

        Process->DeviceMap = DeviceMap;
    }

    /* Release the device map lock */
    KeReleaseGuardedMutex(&ObpDeviceMapLock);
}


VOID
NTAPI
ObQueryDeviceMapInformation(IN PEPROCESS Process,
                            IN PPROCESS_DEVICEMAP_INFORMATION DeviceMapInfo)
{
    PDEVICE_MAP DeviceMap;

    /* Acquire the device map lock */
    KeAcquireGuardedMutex(&ObpDeviceMapLock);

    /* Get the process device map or the system device map */
    DeviceMap = (Process != NULL) ? Process->DeviceMap : ObSystemDeviceMap;
    if (DeviceMap != NULL)
    {
        /* Make a copy */
        DeviceMapInfo->Query.DriveMap = DeviceMap->DriveMap;
        RtlCopyMemory(DeviceMapInfo->Query.DriveType,
                      DeviceMap->DriveType,
                      sizeof(DeviceMap->DriveType));
    }

    /* Release the device map lock */
    KeReleaseGuardedMutex(&ObpDeviceMapLock);
}


#if 0
NTSTATUS
NTAPI
ObIsDosDeviceLocallyMapped(
    IN ULONG Index,
    OUT PUCHAR DosDeviceState)
{
    /* Check the index */
    if (Index < 1 || Index > 26)
        return STATUS_INVALID_PARAMETER;

    /* Acquire the device map lock */
    KeAcquireGuardedMutex(&ObpDeviceMapLock);

    /* Get drive mapping status */
    *DosDeviceState = (ObSystemDeviceMap->DriveMap & (1 << Index)) != 0;

    /* Release the device map lock */
    KeReleaseGuardedMutex(&ObpDeviceMapLock);

    return STATUS_SUCCESS;
}
#endif

/* EOF */

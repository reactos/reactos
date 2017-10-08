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
ObpCreateDeviceMap(IN HANDLE DirectoryHandle)
{
    POBJECT_DIRECTORY DirectoryObject = NULL;
    PDEVICE_MAP DeviceMap = NULL;
    NTSTATUS Status;

    Status = ObReferenceObjectByHandle(DirectoryHandle,
                                       DIRECTORY_TRAVERSE,
                                       ObDirectoryType,
                                       KeGetPreviousMode(),
                                       (PVOID*)&DirectoryObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ObReferenceObjectByHandle() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Allocate and initialize a new device map */
    DeviceMap = ExAllocatePoolWithTag(NonPagedPool,
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
    DirectoryObject->DeviceMap = DeviceMap;

    /* Attach the device map to the process */
    ObSystemDeviceMap = DeviceMap;
    PsGetCurrentProcess()->DeviceMap = DeviceMap;

    /* Release the device map lock */
    KeReleaseGuardedMutex(&ObpDeviceMapLock);

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

/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ob/devicemap.c
 * PURPOSE:         Device map implementation
 * PROGRAMMERS:     Eric Kohl (eric.kohl@reactos.org)
 *                  Alex Ionescu (alex.ionescu@reactos.org)
 *                  Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

ULONG ObpLUIDDeviceMapsDisabled;
ULONG ObpLUIDDeviceMapsEnabled;

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
        ObDereferenceObject(DirectoryObject);
        ExFreePoolWithTag(NewDeviceMap, 'mDbO');
    }

    /* And dereference previous process device map */
    if (OldDeviceMap != NULL)
    {
        ObfDereferenceDeviceMap(OldDeviceMap);
    }

    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
ObSetDirectoryDeviceMap(OUT PDEVICE_MAP * DeviceMap,
                        IN HANDLE DirectoryHandle)
{
    POBJECT_DIRECTORY DirectoryObject = NULL;
    PDEVICE_MAP LocalMap = NULL, NewDeviceMap = NULL;
    NTSTATUS Status;
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_NAME_INFO HeaderNameInfo;

    Status = ObReferenceObjectByHandle(DirectoryHandle,
                                       DIRECTORY_TRAVERSE,
                                       ObpDirectoryObjectType,
                                       KernelMode,
                                       (PVOID*)&DirectoryObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ObReferenceObjectByHandle() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Allocate and initialize a new device map */
    LocalMap = ExAllocatePoolWithTag(PagedPool,
                                     sizeof(*LocalMap),
                                     'mDbO');
    if (LocalMap == NULL)
    {
        ObDereferenceObject(DirectoryObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Initialize the device map */
    RtlZeroMemory(LocalMap, sizeof(*LocalMap));
    LocalMap->ReferenceCount = 1;
    LocalMap->DosDevicesDirectory = DirectoryObject;

    /* Acquire the device map lock */
    KeAcquireGuardedMutex(&ObpDeviceMapLock);

    /* Attach the device map to the directory object */
    if (DirectoryObject->DeviceMap == NULL)
    {
        DirectoryObject->DeviceMap = LocalMap;
    }
    else
    {
        NewDeviceMap = LocalMap;

        /* There's already a device map,
           so reuse it */
        LocalMap = DirectoryObject->DeviceMap;
        ++LocalMap->ReferenceCount;
    }

    /* If current object isn't system one, save system one in current
     * device map */
    if (DirectoryObject != ObSystemDeviceMap->DosDevicesDirectory)
    {
        /* We also need to make the object permanant */
        LocalMap->GlobalDosDevicesDirectory = ObSystemDeviceMap->DosDevicesDirectory;
    }

    /* Release the device map lock */
    KeReleaseGuardedMutex(&ObpDeviceMapLock);

    if (DeviceMap != NULL)
    {
        *DeviceMap = LocalMap;
    }

    /* Caller expects us to make the object permanant, so do it! */
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

    /* Release useless device map if required */
    if (NewDeviceMap != NULL)
    {
        ObDereferenceObject(DirectoryObject);
        ExFreePoolWithTag(NewDeviceMap, 'mDbO');
    }

    return Status;
}


NTSTATUS
NTAPI
ObpSetCurrentProcessDeviceMap(VOID)
{
    PTOKEN Token;
    LUID LogonId;
    NTSTATUS Status;
    PEPROCESS CurrentProcess;
    LUID SystemLuid = SYSTEM_LUID;
    PDEVICE_MAP DeviceMap, OldDeviceMap;

    /* Get our impersonation token */
    CurrentProcess = PsGetCurrentProcess();
    Token = PsReferencePrimaryToken(CurrentProcess);
    if (Token == NULL)
    {
        return STATUS_NO_TOKEN;
    }

    /* Query the Logon ID */
    Status = SeQueryAuthenticationIdToken(Token, &LogonId);
    if (!NT_SUCCESS(Status))
    {
        goto done;
    }

    /* If that's system, then use system device map */
    if (RtlEqualLuid(&LogonId, &SystemLuid))
    {
        DeviceMap = ObSystemDeviceMap;
    }
    /* Otherwise ask Se for the device map */
    else
    {
        Status = SeGetLogonIdDeviceMap(&LogonId, &DeviceMap);
        if (!NT_SUCCESS(Status))
        {
            /* Normalize failure status */
            Status = STATUS_OBJECT_PATH_INVALID;
            goto done;
        }
    }

    /* Fail if no device map */
    if (DeviceMap == NULL)
    {
        Status = STATUS_OBJECT_PATH_INVALID;
        goto done;
    }

    /* Acquire the device map lock */
    KeAcquireGuardedMutex(&ObpDeviceMapLock);

    /* Save old device map attached to the process */
    OldDeviceMap = CurrentProcess->DeviceMap;

    /* Set new device map & reference it */
    ++DeviceMap->ReferenceCount;
    CurrentProcess->DeviceMap = DeviceMap;

    /* Release the device map lock */
    KeReleaseGuardedMutex(&ObpDeviceMapLock);

    /* If we had a device map, dereference it */
    if (OldDeviceMap != NULL)
    {
        ObfDereferenceDeviceMap(OldDeviceMap);
    }

done:
    /* We're done with the token! */
    ObDereferenceObject(Token);

    return Status;
}


PDEVICE_MAP
NTAPI
ObpReferenceDeviceMap(VOID)
{
    LUID LogonId;
    NTSTATUS Status;
    PTOKEN Token = NULL;
    PDEVICE_MAP DeviceMap;
    PETHREAD CurrentThread;
    BOOLEAN LookingForSystem;
    LUID SystemLuid = SYSTEM_LUID;
    BOOLEAN CopyOnOpen, EffectiveOnly;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;

    LookingForSystem = FALSE;

    /* If LUID mapping is enable, try to get appropriate device map */
    if (ObpLUIDDeviceMapsEnabled != 0)
    {
        /* In case of impersonation, we've got a bit of work to do */
        CurrentThread = PsGetCurrentThread();
        if (CurrentThread->ActiveImpersonationInfo)
        {
            /* Get impersonation token */
            Token = PsReferenceImpersonationToken(CurrentThread,
                                                  &CopyOnOpen,
                                                  &EffectiveOnly,
                                                  &ImpersonationLevel);
            /* Get logon LUID */
            if (Token != NULL)
            {
                Status = SeQueryAuthenticationIdToken(Token, &LogonId);
            }
            else
            {
                /* Force failure */
                Status = STATUS_NO_TOKEN;
            }

            /* If we got logon LUID */
            if (NT_SUCCESS(Status))
            {
                /*
                 * Check it's not system, system is easy to handle,
                 * we just need to return ObSystemDeviceMap
                 */
                if (!RtlEqualLuid(&LogonId, &SystemLuid))
                {
                    /* Ask Se for the device  map */
                    Status = SeGetLogonIdDeviceMap(&LogonId, &DeviceMap);
                    if (NT_SUCCESS(Status))
                    {
                        /* Acquire the device map lock */
                        KeAcquireGuardedMutex(&ObpDeviceMapLock);

                        /* Reference the device map if any */
                        if (DeviceMap != NULL)
                        {
                            ++DeviceMap->ReferenceCount;
                        }

                        /* Release the device map lock */
                        KeReleaseGuardedMutex(&ObpDeviceMapLock);

                        /* If we got the device map, we're done! */
                        if (DeviceMap != NULL)
                        {
                            ObDereferenceObject(Token);

                            return DeviceMap;
                        }
                    }
                }
                else
                {
                    LookingForSystem = TRUE;
                }
            }
        }

        /*
         * Fall back case of the LUID mapping, make sure there's a
         * a device map attached to the current process
         */
        if (PsGetCurrentProcess()->DeviceMap == NULL &&
            !NT_SUCCESS(ObpSetCurrentProcessDeviceMap()))
        {
            /* We may have failed after we got impersonation token */
            if (Token != NULL)
            {
                ObDereferenceObject(Token);
            }

            return NULL;
        }
    }

    /* Acquire the device map lock */
    KeAcquireGuardedMutex(&ObpDeviceMapLock);

    /* If we're looking for system map, use it */
    if (LookingForSystem)
    {
        DeviceMap = ObSystemDeviceMap;
    }
    /* Otherwise, use current process device map */
    else
    {
        DeviceMap = PsGetCurrentProcess()->DeviceMap;
    }

    /* If we got one, reference it */
    if (DeviceMap != NULL)
    {
        ++DeviceMap->ReferenceCount;
    }

    /* Release the device map lock */
    KeReleaseGuardedMutex(&ObpDeviceMapLock);

    /* We may have impersonation token (if we failed in impersonation branch) */
    if (Token != NULL)
    {
        ObDereferenceObject(Token);
    }

    /* Return the potentially found device map */
    return DeviceMap;
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
    if (DeviceMap != NULL)
        ObfDereferenceDeviceMap(DeviceMap);
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
    ObMakeTemporaryObject(DeviceMap->DosDevicesDirectory);
    ObDereferenceObject(DeviceMap->DosDevicesDirectory);
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


NTSTATUS
NTAPI
ObQueryDeviceMapInformation(
    _In_opt_ PEPROCESS Process,
    _Out_ PPROCESS_DEVICEMAP_INFORMATION DeviceMapInfo,
    _In_ ULONG Flags)
{
    PDEVICE_MAP DeviceMap = NULL, GlobalDeviceMap;
    BOOLEAN Dereference;
    PROCESS_DEVICEMAP_INFORMATION MapInfo;
    ULONG BitMask, i;
    BOOLEAN ReturnAny;
    NTSTATUS Status;

    /* Validate flags */
    if (Flags & ~PROCESS_LUID_DOSDEVICES_ONLY)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Dereference = FALSE;
    /* Do we want to return anything? */
    ReturnAny = ~Flags & PROCESS_LUID_DOSDEVICES_ONLY;

    /* If LUID mappings are enabled... */
    if (ObpLUIDDeviceMapsEnabled != 0)
    {
        /* Check for process parameter validness */
        if (Process != NULL && Process != PsGetCurrentProcess())
        {
            return STATUS_INVALID_PARAMETER;
        }

        /* And get the device map */
        DeviceMap = ObpReferenceDeviceMap();
    }

    /* Acquire the device map lock */
    KeAcquireGuardedMutex(&ObpDeviceMapLock);

    /*
     * If we had a device map, if because of LUID mappings,
     * we'll have to dereference it afterwards
     */
    if (DeviceMap != NULL)
    {
        Dereference = TRUE;
    }
    else
    {
        /* Get the process device map or the system device map */
        DeviceMap = (Process != NULL) ? Process->DeviceMap : ObSystemDeviceMap;
    }

    /* Fail if no device map */
    if (DeviceMap == NULL)
    {
        KeReleaseGuardedMutex(&ObpDeviceMapLock);
        return STATUS_END_OF_FILE;
    }

    /* At that point, assume success */
    Status = STATUS_SUCCESS;

    /* Try to get the global device map if any */
    GlobalDeviceMap = DeviceMap;
    if (DeviceMap->GlobalDosDevicesDirectory != NULL)
    {
        if (DeviceMap->GlobalDosDevicesDirectory->DeviceMap != NULL)
        {
            GlobalDeviceMap = DeviceMap->GlobalDosDevicesDirectory->DeviceMap;
        }
    }

    /* Now, setup our device map info, especially drive types */
    MapInfo.Query.DriveMap = DeviceMap->DriveMap;
    /* Browse every device */
    for (i = 0, BitMask = 1; i < 32; ++i, BitMask *= 2)
    {
        /* Set the type given current device map */
        MapInfo.Query.DriveType[i] = DeviceMap->DriveType[i];

        /*
         * If device is not existing and we're asked to return
         * more than just LUID mapped, get the entry
         * from global device map if not remote
         */
        if (!(MapInfo.Query.DriveMap & BitMask) && ReturnAny)
        {
            if (ObpLUIDDeviceMapsEnabled != 0 ||
                (GlobalDeviceMap->DriveType[i] != DOSDEVICE_DRIVE_REMOTE &&
                 GlobalDeviceMap->DriveType[i] != DOSDEVICE_DRIVE_CALCULATE))
            {
                MapInfo.Query.DriveType[i] = GlobalDeviceMap->DriveType[i];
                MapInfo.Query.DriveMap |= BitMask & GlobalDeviceMap->DriveMap;
            }
        }
    }

    /* Release the device map lock */
    KeReleaseGuardedMutex(&ObpDeviceMapLock);

    /* Dereference LUID device map */
    if (Dereference)
    {
        ObfDereferenceDeviceMap(DeviceMap);
    }

    /* Copy back data */
    _SEH2_TRY
    {
        RtlCopyMemory(DeviceMapInfo, &MapInfo, sizeof(PROCESS_DEVICEMAP_INFORMATION));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return Status;
}


ULONG
NTAPI
ObIsLUIDDeviceMapsEnabled(VOID)
{
    return ObpLUIDDeviceMapsEnabled;
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

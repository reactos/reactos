/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/iface.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 */

/* INCLUDES *****************************************************************/

#include "vfat.h"

#define NDEBUG
#include <debug.h>

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(INIT, DriverEntry)
#endif


/* GLOBALS *****************************************************************/

PVFAT_GLOBAL_DATA VfatGlobalData;

/* FUNCTIONS ****************************************************************/

/*
 * FUNCTION: Called by the system to initialize the driver
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 *           RegistryPath = path to our configuration entries
 * RETURNS: Success or failure
 */
INIT_SECTION
NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath)
{
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Fat");
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(RegistryPath);

    Status = IoCreateDevice(DriverObject,
                            sizeof(VFAT_GLOBAL_DATA),
                            &DeviceName,
                            FILE_DEVICE_DISK_FILE_SYSTEM,
                            0,
                            FALSE,
                            &DeviceObject);
    if (Status == STATUS_OBJECT_NAME_EXISTS ||
       Status == STATUS_OBJECT_NAME_COLLISION)
    {
        /* Try an other name, if 'Fat' is already in use. 'Fat' is also used by fastfat.sys on W2K */
        RtlInitUnicodeString(&DeviceName, L"\\RosFat");
        Status = IoCreateDevice(DriverObject,
                                sizeof(VFAT_GLOBAL_DATA),
                                &DeviceName,
                                FILE_DEVICE_DISK_FILE_SYSTEM,
                                0,
                                FALSE,
                                &DeviceObject);
    }

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    VfatGlobalData = DeviceObject->DeviceExtension;
    RtlZeroMemory (VfatGlobalData, sizeof(VFAT_GLOBAL_DATA));
    VfatGlobalData->DriverObject = DriverObject;
    VfatGlobalData->DeviceObject = DeviceObject;
    /* Enable this to enter the debugger when file system corruption
     * has been detected:
    VfatGlobalData->Flags = VFAT_BREAK_ON_CORRUPTION; */

    DeviceObject->Flags |= DO_DIRECT_IO;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_READ] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = VfatShutdown;
    DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = VfatBuildRequest;
    DriverObject->MajorFunction[IRP_MJ_PNP] = VfatBuildRequest;

    DriverObject->DriverUnload = NULL;

    /* Cache manager */
    VfatGlobalData->CacheMgrCallbacks.AcquireForLazyWrite = VfatAcquireForLazyWrite;
    VfatGlobalData->CacheMgrCallbacks.ReleaseFromLazyWrite = VfatReleaseFromLazyWrite;
    VfatGlobalData->CacheMgrCallbacks.AcquireForReadAhead = VfatAcquireForReadAhead;
    VfatGlobalData->CacheMgrCallbacks.ReleaseFromReadAhead = VfatReleaseFromReadAhead;

    /* Fast I/O */
    VfatInitFastIoRoutines(&VfatGlobalData->FastIoDispatch);
    DriverObject->FastIoDispatch = &VfatGlobalData->FastIoDispatch;

    /* Private lists */
    ExInitializeNPagedLookasideList(&VfatGlobalData->FcbLookasideList,
                                    NULL, NULL, 0, sizeof(VFATFCB), TAG_FCB, 0);
    ExInitializeNPagedLookasideList(&VfatGlobalData->CcbLookasideList,
                                    NULL, NULL, 0, sizeof(VFATCCB), TAG_CCB, 0);
    ExInitializeNPagedLookasideList(&VfatGlobalData->IrpContextLookasideList,
                                    NULL, NULL, 0, sizeof(VFAT_IRP_CONTEXT), TAG_IRP, 0);

    ExInitializeResourceLite(&VfatGlobalData->VolumeListLock);
    InitializeListHead(&VfatGlobalData->VolumeListHead);
    IoRegisterFileSystem(DeviceObject);
    return STATUS_SUCCESS;
}

/* EOF */

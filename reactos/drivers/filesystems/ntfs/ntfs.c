/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystem/ntfs/ntfs.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMER:       Eric Kohl
 *                   Pierre Schweitzer 
 */

/* INCLUDES *****************************************************************/

#include "ntfs.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *****************************************************************/

PNTFS_GLOBAL_DATA NtfsGlobalData = NULL;

/* FUNCTIONS ****************************************************************/

/*
 * FUNCTION: Called by the system to initialize the driver
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 *           RegistryPath = path to our configuration entries
 * RETURNS: Success or failure
 */
NTSTATUS
NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath)
{
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(DEVICE_NAME);
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject;

    TRACE_(NTFS, "DriverEntry(%p, '%wZ')\n", DriverObject, RegistryPath);

    Status = IoCreateDevice(DriverObject,
                            sizeof(NTFS_GLOBAL_DATA),
                            &DeviceName,
                            FILE_DEVICE_DISK_FILE_SYSTEM,
                            0,
                            FALSE,
                            &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        WARN_(NTFS, "IoCreateDevice failed with status: %lx\n", Status);
        return Status;
    }

    /* Initialize global data */
    NtfsGlobalData = DeviceObject->DeviceExtension;
    RtlZeroMemory(NtfsGlobalData, sizeof(NTFS_GLOBAL_DATA));

    NtfsGlobalData->DeviceObject = DeviceObject;
    NtfsGlobalData->Identifier.Type = NTFS_TYPE_GLOBAL_DATA;
    NtfsGlobalData->Identifier.Size = sizeof(NTFS_GLOBAL_DATA);

    ExInitializeResourceLite(&NtfsGlobalData->Resource);

    /* Keep trace of Driver Object */
    NtfsGlobalData->DriverObject = DriverObject;

    /* Initialize IRP functions array */
    NtfsInitializeFunctionPointers(DriverObject);

    /* Initialize CC functions array */
    NtfsGlobalData->CacheMgrCallbacks.AcquireForLazyWrite = NtfsAcqLazyWrite; 
    NtfsGlobalData->CacheMgrCallbacks.ReleaseFromLazyWrite = NtfsRelLazyWrite; 
    NtfsGlobalData->CacheMgrCallbacks.AcquireForReadAhead = NtfsAcqReadAhead; 
    NtfsGlobalData->CacheMgrCallbacks.ReleaseFromReadAhead = NtfsRelReadAhead; 

    NtfsGlobalData->FastIoDispatch.SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);
    NtfsGlobalData->FastIoDispatch.FastIoCheckIfPossible = NtfsFastIoCheckIfPossible;
    NtfsGlobalData->FastIoDispatch.FastIoRead = NtfsFastIoRead;
    NtfsGlobalData->FastIoDispatch.FastIoWrite = NtfsFastIoWrite;
    DriverObject->FastIoDispatch = &NtfsGlobalData->FastIoDispatch;

    /* Driver can't be unloaded */
    DriverObject->DriverUnload = NULL;

    NtfsGlobalData->DeviceObject->Flags |= DO_DIRECT_IO;

    /* Register file system */
    IoRegisterFileSystem(NtfsGlobalData->DeviceObject);
    ObReferenceObject(NtfsGlobalData->DeviceObject);

    return Status;
}


/*
 * FUNCTION: Called within the driver entry to initialize the IRP functions array 
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 * RETURNS: Nothing
 */
VOID
NTAPI
NtfsInitializeFunctionPointers(PDRIVER_OBJECT DriverObject)
{
    DriverObject->MajorFunction[IRP_MJ_CREATE]                   = NtfsFsdCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                    = NtfsFsdClose;
    DriverObject->MajorFunction[IRP_MJ_READ]                     = NtfsFsdRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE]                    = NtfsFsdWrite;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]        = NtfsFsdDispatch;
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = NtfsFsdDispatch;
    DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION]   = NtfsFsdDispatch;
    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL]        = NtfsFsdDispatch;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL]      = NtfsFsdFileSystemControl;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]           = NtfsFsdDeviceControl;

    return;
}

/* EOF */

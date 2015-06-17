/*
 *  ReactOS kernel
 *  Copyright (C) 2002, 2003 ReactOS Team
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
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystems/cdfs/cdfs.c
 * PURPOSE:          CDROM (ISO 9660) filesystem driver
 * PROGRAMMER:       Art Yerkes
 *                   Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "cdfs.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

PCDFS_GLOBAL_DATA CdfsGlobalData;


/* FUNCTIONS ****************************************************************/

NTSTATUS NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath)
            /*
            * FUNCTION: Called by the system to initialize the driver
            * ARGUMENTS:
            *           DriverObject = object describing this driver
            *           RegistryPath = path to our configuration entries
            * RETURNS: Success or failure
            */
{
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status;
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Cdfs");

    UNREFERENCED_PARAMETER(RegistryPath);

    DPRINT("CDFS 0.0.3\n");

    Status = IoCreateDevice(DriverObject,
        sizeof(CDFS_GLOBAL_DATA),
        &DeviceName,
        FILE_DEVICE_CD_ROM_FILE_SYSTEM,
        0,
        FALSE,
        &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    /* Initialize global data */
    CdfsGlobalData = DeviceObject->DeviceExtension;
    RtlZeroMemory(CdfsGlobalData,
        sizeof(CDFS_GLOBAL_DATA));
    CdfsGlobalData->DriverObject = DriverObject;
    CdfsGlobalData->DeviceObject = DeviceObject;

    /* Initialize driver data */
    DeviceObject->Flags = DO_DIRECT_IO;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = CdfsFsdDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = CdfsFsdDispatch;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = CdfsFsdDispatch;
    DriverObject->MajorFunction[IRP_MJ_READ] = CdfsFsdDispatch;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = CdfsFsdDispatch;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = CdfsFsdDispatch;
    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] = CdfsFsdDispatch;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = CdfsFsdDispatch;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = CdfsFsdDispatch;
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = CdfsFsdDispatch;
    DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION] = CdfsFsdDispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = CdfsFsdDispatch;

    CdfsGlobalData->FastIoDispatch.SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);
    CdfsGlobalData->FastIoDispatch.FastIoCheckIfPossible = CdfsFastIoCheckIfPossible;
    CdfsGlobalData->FastIoDispatch.FastIoRead = CdfsFastIoRead;
    CdfsGlobalData->FastIoDispatch.FastIoWrite = CdfsFastIoWrite;
    DriverObject->FastIoDispatch = &CdfsGlobalData->FastIoDispatch;

    /* Initialize lookaside list for IRP contexts */
    ExInitializeNPagedLookasideList(&CdfsGlobalData->IrpContextLookasideList,
                                    NULL, NULL, 0, sizeof(CDFS_IRP_CONTEXT), 'PRIC', 0);

    DriverObject->DriverUnload = NULL;

    /* Cache manager */
    CdfsGlobalData->CacheMgrCallbacks.AcquireForLazyWrite = CdfsAcquireForLazyWrite;
    CdfsGlobalData->CacheMgrCallbacks.ReleaseFromLazyWrite = CdfsReleaseFromLazyWrite;
    CdfsGlobalData->CacheMgrCallbacks.AcquireForReadAhead = CdfsAcquireForLazyWrite;
    CdfsGlobalData->CacheMgrCallbacks.ReleaseFromReadAhead = CdfsReleaseFromLazyWrite;

    DeviceObject->Flags |= DO_LOW_PRIORITY_FILESYSTEM;

    IoRegisterFileSystem(DeviceObject);
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return(STATUS_SUCCESS);
}



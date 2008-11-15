/*
 *  ReactOS kernel
 *  Copyright (C) 2008 ReactOS Team
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
 * FILE:             drivers/filesystem/ksecdd/ksecdd.c
 * PURPOSE:          Kernel Security Support Provider Interface
 * PROGRAMMER:       Pierre Schweitzer 
 */

/* INCLUDES *****************************************************************/

#include "ksecdd.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *****************************************************************/

PEPROCESS KsecSystemProcess;


/* FUNCTIONS ****************************************************************/

NTSTATUS NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath)
{
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(DEVICE_NAME);

    KsecSystemProcess = PsGetCurrentProcess();

    KsecInitializeFunctionPointers(DriverObject);

    Status = IoCreateDevice(DriverObject,
                            0,
                            &DeviceName,
                            FILE_DEVICE_KSEC,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &DeviceObject);

    return Status;
}

VOID NTAPI 
KsecInitializeFunctionPointers(PDRIVER_OBJECT DriverObject)
{
  DriverObject->MajorFunction[IRP_MJ_CREATE]                   = KsecDispatch;
  DriverObject->MajorFunction[IRP_MJ_CLOSE]                    = KsecDispatch;
  DriverObject->MajorFunction[IRP_MJ_READ]                     = KsecDispatch;
  DriverObject->MajorFunction[IRP_MJ_WRITE]                    = KsecDispatch;
  DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]        = KsecDispatch;
  DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = KsecDispatch;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL ]          = KsecDispatch;
    
  return;
}

/*
 * Driver Regression Tests
 *
 * Copyright 2009 Michael Martin <martinmnet@hotmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; see the file COPYING.LIB.
 * If not, write to the Free Software Foundation,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* INCLUDES *******************************************************************/

#include "ntddk.h"
#include "ntddser.h"

NTSTATUS
NTAPI
DriverDispatch(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
    DbgPrint(" ControlCode %x\n",IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.IoControlCode);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information=0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
DriverCreateClose(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

VOID
NTAPI
DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING  DeviceString;

    RtlInitUnicodeString(&DeviceString, L"\\DosDevices\\kmtestassist");
    IoDeleteSymbolicLink(&DeviceString);    

    IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,IN PUNICODE_STRING path)
{
    PDEVICE_OBJECT  pDeviceObject;
    UNICODE_STRING  DriverString;
    UNICODE_STRING  DeviceString;

    NTSTATUS Status= STATUS_DEVICE_CONFIGURATION_ERROR;

    RtlInitUnicodeString(&DriverString, L"\\Device\\kmtestassist");

    Status = IoCreateDevice(DriverObject,0,&DriverString,FILE_DEVICE_UNKNOWN,0,FALSE,&pDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    RtlInitUnicodeString(&DeviceString, L"\\DosDevices\\kmtestassist");

    Status = IoCreateSymbolicLink(&DeviceString, &DriverString);
    if (!NT_SUCCESS(Status))
    {
            // Delete device object if not successful
            IoDeleteDevice(pDeviceObject);
            return Status;
    }

    DriverObject->DriverUnload = DriverUnload;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverDispatch;
    DriverObject->MajorFunction[IRP_MJ_CREATE]         = DriverCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = DriverCreateClose;

    return Status;
}

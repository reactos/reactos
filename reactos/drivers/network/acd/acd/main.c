/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS RAS Automatic Connection Driver
 * FILE:        acd/main.c
 * PURPOSE:     Driver entry point
 * PROGRAMMERS: Dmitry Chapyshev(dmitry@reactos.org)
 * REVISIONS:
 *   25/05/2008 Created
 */

#include <ndis.h>
#include <tdi.h>
#include <debug.h>

#include "acdapi.h"

NTSTATUS
DriverEntry(PDRIVER_OBJECT  pDriverObject,
            PUNICODE_STRING pRegistryPath)
{
    UNICODE_STRING DeviceName;
    PDEVICE_OBJECT pDeviceObject;
    NTSTATUS Status;

    RtlInitUnicodeString(&DeviceName, L"RasAcd");

    Status = IoCreateDevice(pDriverObject,
                            0,
                            &DeviceName,
                            FILE_DEVICE_RASACD,
                            0,
                            FALSE,
                            &pDeviceObject);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoCreateDevice() failed (Status %lx)\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

/* EOF */


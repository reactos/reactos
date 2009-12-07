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

#include "kmtest.h"
#include <ddk/ntddk.h>
#include <ddk/ntifs.h>

VOID DriverObjectTest(PDRIVER_OBJECT DriverObject, int DriverStatus)
{
    BOOLEAN CheckThisDispatchRoutine;
    PVOID FirstMajorFunc;
    int i;

    ok(DriverObject->Size == sizeof(DRIVER_OBJECT), "Size does not match, got %x",DriverObject->Size);
    ok(DriverObject->Type == 4, "Type does not match 4. got %d",DriverObject->Type);

    if (DriverStatus == 0)
    {
        ok(DriverObject->DeviceObject == NULL, "Expected DeviceObject pointer to be 0, got %p",
            DriverObject->DeviceObject);
        ok (DriverObject->Flags == DRVO_LEGACY_DRIVER,
            "Expected Flags to be DRVO_LEGACY_DRIVER, got %lu",
            DriverObject->Flags);
    }
    else if (DriverStatus == 1)
    {
        ok(DriverObject->DeviceObject != NULL, "Expected DeviceObject pointer to non null");
        ok (DriverObject->Flags == (DRVO_LEGACY_DRIVER | DRVO_INITIALIZED),
            "Expected Flags to be DRVO_LEGACY_DRIVER | DRVO_INITIALIZED, got %lu",
            DriverObject->Flags);
    }
    else
    {
        ok(DriverObject->DeviceObject != NULL, "Expected DeviceObject pointer to non null");
        ok (DriverObject->Flags == (DRVO_LEGACY_DRIVER | DRVO_INITIALIZED | DRVO_UNLOAD_INVOKED), 
            "Expected Flags to be DRVO_LEGACY_DRIVER | DRVO_INITIALIZED | DRVO_UNLOAD_INVOKED, got %lu",
            DriverObject->Flags);
    }

    /* Select a routine that was not changed */
    FirstMajorFunc = DriverObject->MajorFunction[1];
    ok(FirstMajorFunc != 0, "Expected MajorFunction[1] to be non NULL");

    if (FirstMajorFunc)
    {
        for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        {
            if (DriverStatus > 0) CheckThisDispatchRoutine = (i > 3) && (i != 14);
            else CheckThisDispatchRoutine = TRUE;

            if (CheckThisDispatchRoutine)
            {
                ok(DriverObject->MajorFunction[i] == FirstMajorFunc, "Expected MajorFunction[%d] to match %p",
                    i, FirstMajorFunc);
            }
        }
    }
    else
    {
        ok(TRUE, "Skipped testing for all MajorFunction");
    }
}

BOOLEAN ZwLoadTest(PDRIVER_OBJECT DriverObject, PUNICODE_STRING DriverRegistryPath, PWCHAR NewDriverRegPath)
{
    UNICODE_STRING RegPath;
    NTSTATUS Status;

    /* Try to load ourself */
    Status = ZwLoadDriver(DriverRegistryPath);
    ok (Status == STATUS_IMAGE_ALREADY_LOADED, "Expected NTSTATUS STATUS_IMAGE_ALREADY_LOADED, got 0x%lX", Status);

    if (Status != STATUS_IMAGE_ALREADY_LOADED)
    {
        DbgPrint("WARNING: Loading this a second time will cause BUGCHECK!\n");
    }

    /* Try to load with a Registry Path that doesnt exist */
    RtlInitUnicodeString(&RegPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\deadbeef");
    Status = ZwLoadDriver(&RegPath);
    ok (Status == STATUS_OBJECT_NAME_NOT_FOUND, "Expected NTSTATUS STATUS_OBJECT_NAME_NOT_FOUND, got 0x%lX", Status);

    /* Load the driver */
    RtlInitUnicodeString(&RegPath, NewDriverRegPath);
    Status = ZwLoadDriver(&RegPath);
    ok(Status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got 0x%lX", Status);

    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN ZwUnloadTest(PDRIVER_OBJECT DriverObject, PUNICODE_STRING DriverRegistryPath, PWCHAR NewDriverRegPath)
{
    UNICODE_STRING RegPath;
    NTSTATUS Status;

    /* Try to unload ourself, which should fail as our Unload routine hasnt been set yet. */
    Status = ZwUnloadDriver(DriverRegistryPath);
    ok (Status == STATUS_INVALID_DEVICE_REQUEST, "Expected NTSTATUS STATUS_INVALID_DEVICE_REQUEST, got 0x%lX", Status);

    /* Try to unload with a Registry Path that doesnt exist */
    RtlInitUnicodeString(&RegPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\deadbeef");
    Status = ZwUnloadDriver(&RegPath);
    ok (Status == STATUS_OBJECT_NAME_NOT_FOUND, "Expected NTSTATUS STATUS_OBJECT_NAME_NOT_FOUND, got 0x%lX", Status);

    /* Unload the driver */
    RtlInitUnicodeString(&RegPath, NewDriverRegPath);
    Status = ZwUnloadDriver(&RegPath);
    ok(Status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got 0x%lX", Status);

    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    return TRUE;
}

/*
 * Kernel Mode regression Test
 * Driver Core
 *
 * Copyright 2004 Filip Navara <xnavara@volny.cz>
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

#include <ddk/ntddk.h>
#include "kmtest.h"

LONG successes;
LONG failures;
tls_data glob_data;

/* PRIVATE FUNCTIONS ***********************************************************/
VOID
StartTest()
{
    successes = 0;
    failures = 0;
}

VOID
FinishTest(LPSTR TestName)
{
    DbgPrint("%s: %d test executed (0 marked as todo, %d failures), 0 skipped.\n", TestName, successes + failures, failures);
}

void kmtest_set_location(const char* file, int line)
{
    glob_data.current_file=strrchr(file,'/');
    if (glob_data.current_file==NULL)
        glob_data.current_file=strrchr(file,'\\');
    if (glob_data.current_file==NULL)
        glob_data.current_file=file;
    else
        glob_data.current_file++;
    glob_data.current_line=line;
}

/*
 * Checks condition.
 * Parameters:
 *   - condition - condition to check;
 *   - msg test description;
 *   - file - test application source code file name of the check
 *   - line - test application source code file line number of the check
 * Return:
 *   0 if condition does not have the expected value, 1 otherwise
 */
int kmtest_ok(int condition, const char *msg, ... )
{
    va_list valist;

    if (!condition)
    {
        if (msg[0])
        {
            char string[1024];
            va_start(valist, msg);
            vsprintf(string, msg, valist);
            DbgPrint( "%s:%d: Test failed: %s\n",
                glob_data.current_file, glob_data.current_line, string );
            va_end(valist);
        }
        else
        {
            DbgPrint( "%s:%d: Test failed\n",
                glob_data.current_file, glob_data.current_line );
        }
        InterlockedIncrement(&failures);
        return 0;
    }
    else
    {/*
        if (report_success)
            fprintf( stdout, "%s:%d: Test succeeded\n",
            glob_data.current_file, glob_data.current_line);*/
        InterlockedIncrement(&successes);
    }
    return 1;
}

/* PUBLIC FUNCTIONS ***********************************************************/

PWCHAR CreateLowerDeviceRegistryKey(PUNICODE_STRING RegistryPath, PWCHAR NewDriver);

/*
 * Test Declarations
 */
VOID NtoskrnlIoTests();
VOID NtoskrnlObTest();
VOID NtoskrnlExecutiveTests();
VOID NtoskrnlPoolsTest();
VOID DriverObjectTest(PDRIVER_OBJECT, int);
VOID DeviceCreateDeleteTest(PDRIVER_OBJECT);
VOID DeviceObjectTest(PDEVICE_OBJECT);
BOOLEAN ZwLoadTest(PDRIVER_OBJECT, PUNICODE_STRING, PWCHAR);
BOOLEAN ZwUnloadTest(PDRIVER_OBJECT, PUNICODE_STRING, PWCHAR);
BOOLEAN DetachDeviceTest(PDEVICE_OBJECT);
BOOLEAN AttachDeviceTest(PDEVICE_OBJECT,  PWCHAR);
VOID LowerDeviceKernelAPITest(PDEVICE_OBJECT, BOOLEAN);

/*
 * KmtestDispatch
 */
NTSTATUS
NTAPI
KmtestDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)

{
    NTSTATUS Status = STATUS_SUCCESS;

    if (AttachDeviceObject)
    {
        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(AttachDeviceObject, Irp);
        return Status;
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

/*
 * KmtestCreateClose
 */
NTSTATUS
NTAPI
KmtestCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (AttachDeviceObject)
    {
        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(AttachDeviceObject, Irp);
        return Status;
    }

    /* Do DriverObject Test with Driver Initialized */
    DriverObjectTest(DeviceObject->DriverObject, 1);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information=0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

/*
 * KmtestUnload
 */
VOID
NTAPI
KmtestUnload(IN PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING DosDeviceString;

    if(AttachDeviceObject)
    {
        IoDetachDevice(AttachDeviceObject);
    }

    /* Do DriverObject Test for Unload */
    DriverObjectTest(DriverObject, 2);

    if (MainDeviceObject)
    {
        RtlInitUnicodeString(&DosDeviceString, L"\\DosDevices\\Kmtest");
        IoDeleteSymbolicLink(&DosDeviceString);

        IoDeleteDevice(MainDeviceObject);
    }
    FinishTest("Driver Tests");
}

/*
 * DriverEntry
 */
NTSTATUS
NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath)
{
    int i;
    PWCHAR LowerDriverRegPath;

    DbgPrint("\n===============================================\n");
    DbgPrint("Kernel Mode Regression Driver Test starting...\n");
    DbgPrint("===============================================\n");

    MainDeviceObject = NULL;
    AttachDeviceObject = NULL;
    ThisDriverObject = DriverObject;

    NtoskrnlExecutiveTests();
    NtoskrnlIoTests();
    NtoskrnlObTest();
    NtoskrnlPoolsTest();

    /* Start the tests for the driver routines */
    StartTest();

    /* Do DriverObject Test for Driver Entry */
    DriverObjectTest(DriverObject, 0);
    /* Create and delete device, on return MainDeviceObject has been created */
    DeviceCreateDeleteTest(DriverObject);

    /* Make sure a device object was created */
    if (MainDeviceObject)
    {
        LowerDriverRegPath = CreateLowerDeviceRegistryKey(RegistryPath, L"kmtestassist");

        if (LowerDriverRegPath)
        {
            /* Load driver test and load the lower driver */
            if (ZwLoadTest(DriverObject, RegistryPath, LowerDriverRegPath))
            {
                AttachDeviceTest(MainDeviceObject, L"kmtestassists");
                if (AttachDeviceObject)
                {
                    LowerDeviceKernelAPITest(MainDeviceObject, FALSE);
                }

                /* Unload lower driver without detaching from its device */
                ZwUnloadTest(DriverObject, RegistryPath, LowerDriverRegPath);
                LowerDeviceKernelAPITest(MainDeviceObject, TRUE);
            }
            else
            {
                DbgPrint("Failed to load kmtestassist driver\n");
            }
        }
    }
    else
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Set all MajorFunctions to NULL to verify that kernel fixes them */
    for (i = 1; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        DriverObject->MajorFunction[i] = NULL;

    /* Set necessary routines */
    DriverObject->DriverUnload = KmtestUnload;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = KmtestDispatch;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = KmtestCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = KmtestCreateClose;

    return STATUS_SUCCESS;
}

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

#define NDEBUG
#include <debug.h>

LONG successes;
LONG failures;
LONG skipped;
tls_data glob_data;

/* PRIVATE FUNCTIONS ***********************************************************/
VOID
StartTest()
{
    successes = 0;
    failures = 0;
    skipped = 0;
}

VOID
FinishTest(HANDLE KeyHandle, LPWSTR TestName)
{
    WCHAR KeyName[100];
    LONG total = successes + failures;
    UNICODE_STRING KeyNameU;

    wcscpy(KeyName, TestName);
    wcscat(KeyName, L"SuccessCount");
    RtlInitUnicodeString(&KeyNameU, KeyName);

    ZwSetValueKey(KeyHandle,
                  &KeyNameU,
                  0,
                  REG_DWORD,
                  &successes,
                  sizeof(ULONG));

    wcscpy(KeyName, TestName);
    wcscat(KeyName, L"FailureCount");
    RtlInitUnicodeString(&KeyNameU, KeyName);

    ZwSetValueKey(KeyHandle,
                  &KeyNameU,
                  0,
                  REG_DWORD,
                  &failures,
                  sizeof(ULONG));

    wcscpy(KeyName, TestName);
    wcscat(KeyName, L"TotalCount");
    RtlInitUnicodeString(&KeyNameU, KeyName);

    ZwSetValueKey(KeyHandle,
                  &KeyNameU,
                  0,
                  REG_DWORD,
                  &total,
                  sizeof(ULONG));

    wcscpy(KeyName, TestName);
    wcscat(KeyName, L"SkipCount");
    RtlInitUnicodeString(&KeyNameU, KeyName);

    ZwSetValueKey(KeyHandle,
                  &KeyNameU,
                  0,
                  REG_DWORD,
                  &skipped,
                  sizeof(ULONG));

    DbgPrint("%S: %d test executed (0 marked as todo, %d failures), %d skipped.\n", TestName, total, failures, skipped);
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
VOID RegisterDI_Test(HANDLE KeyHandle);
VOID NtoskrnlIoMdlTest(HANDLE KeyHandle);
VOID NtoskrnlIoIrpTest(HANDLE KeyHandle);
VOID NtoskrnlObTest(HANDLE KeyHandle);
VOID ExTimerTest(HANDLE KeyHandle);
VOID PoolsTest(HANDLE KeyHandle);
VOID PoolsCorruption(HANDLE KeyHandle);
VOID KeStallTest(HANDLE KeyHandle);
VOID DriverObjectTest(PDRIVER_OBJECT, int);
VOID DeviceCreateDeleteTest(PDRIVER_OBJECT);
VOID DeviceObjectTest(PDEVICE_OBJECT);
BOOLEAN ZwLoadTest(PDRIVER_OBJECT, PUNICODE_STRING, PWCHAR);
BOOLEAN ZwUnloadTest(PDRIVER_OBJECT, PUNICODE_STRING, PWCHAR);
BOOLEAN DetachDeviceTest(PDEVICE_OBJECT);
BOOLEAN AttachDeviceTest(PDEVICE_OBJECT,  PWCHAR);
VOID LowerDeviceKernelAPITest(PDEVICE_OBJECT, BOOLEAN);
VOID NtoskrnlFsRtlTest(HANDLE KeyHandle);

typedef enum {
    TestStageExTimer = 0,
    TestStageIoMdl,
    TestStageIoDi,
    TestStageIoIrp,
    TestStageMmPoolTest,
    TestStageMmPoolCorruption,
    TestStageOb,
    TestStageKeStall,
    TestStageDrv,
    TestStageFsRtl,
    TestStageMax
} TEST_STAGE;

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
}

static
PKEY_VALUE_PARTIAL_INFORMATION
NTAPI
ReadRegistryValue(HANDLE KeyHandle, PWCHAR ValueName)
{
    NTSTATUS Status;
    PKEY_VALUE_PARTIAL_INFORMATION InformationBuffer = NULL;
    ULONG AllocatedLength = 0, RequiredLength = 0;
    UNICODE_STRING ValueNameU;

    RtlInitUnicodeString(&ValueNameU, ValueName);

    Status = ZwQueryValueKey(KeyHandle,
                             &ValueNameU,
                             KeyValuePartialInformation,
                             NULL,
                             0,
                             &RequiredLength);
    if (Status == STATUS_BUFFER_TOO_SMALL || Status == STATUS_BUFFER_OVERFLOW)
    {
        InformationBuffer = ExAllocatePool(PagedPool, RequiredLength);
        AllocatedLength = RequiredLength;
        if (!InformationBuffer) return NULL;

        Status = ZwQueryValueKey(KeyHandle,
                                 &ValueNameU,
                                 KeyValuePartialInformation,
                                 InformationBuffer,
                                 AllocatedLength,
                                 &RequiredLength);
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to read %S (0x%x)\n", ValueName, Status);
        if (InformationBuffer != NULL)
            ExFreePool(InformationBuffer);
        return NULL;
    }

    return InformationBuffer;
}

static
VOID
RunKernelModeTest(PDRIVER_OBJECT DriverObject,
                  PUNICODE_STRING RegistryPath,
                  HANDLE KeyHandle,
                  TEST_STAGE Stage)
{
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"CurrentStage");
    PWCHAR LowerDriverRegPath;

    DPRINT1("Running stage %d test...\n", Stage);

    ZwSetValueKey(KeyHandle,
                  &KeyName,
                  0,
                  REG_DWORD,
                  &Stage,
                  sizeof(ULONG));

    switch (Stage)
    {
       case TestStageExTimer:
         ExTimerTest(KeyHandle);
         break;

       case TestStageIoMdl:
         NtoskrnlIoMdlTest(KeyHandle);
         break;

       case TestStageIoDi:
         RegisterDI_Test(KeyHandle);
         break;

       case TestStageIoIrp:
         NtoskrnlIoIrpTest(KeyHandle);
         break;

       case TestStageMmPoolTest:
         PoolsTest(KeyHandle);
         break;

       case TestStageMmPoolCorruption:
         PoolsCorruption(KeyHandle);
         break;

       case TestStageOb:
         NtoskrnlObTest(KeyHandle);
         break;

       case TestStageKeStall:
         KeStallTest(KeyHandle);
         break;

       case TestStageDrv:
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

         FinishTest(KeyHandle, L"DriverTest");
         break;

       case TestStageFsRtl:
         NtoskrnlFsRtlTest(KeyHandle);
         break;

       default:
         ASSERT(FALSE);
         break;
     }
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
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING ParameterKeyName = RTL_CONSTANT_STRING(L"Parameters");
    PKEY_VALUE_PARTIAL_INFORMATION KeyInfo;
    PULONG KeyValue;
    TEST_STAGE CurrentStage;
    HANDLE DriverKeyHandle, ParameterKeyHandle;

    DbgPrint("\n===============================================\n");
    DbgPrint("Kernel Mode Regression Driver Test starting...\n");
    DbgPrint("===============================================\n");

    InitializeObjectAttributes(&ObjectAttributes,
                               RegistryPath,
                               OBJ_CASE_INSENSITIVE,
                               0,
                               NULL);

    Status = ZwOpenKey(&DriverKeyHandle,
                       KEY_CREATE_SUB_KEY | KEY_ENUMERATE_SUB_KEYS,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open %wZ\n", RegistryPath);
        return Status;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &ParameterKeyName,
                               OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                               DriverKeyHandle,
                               NULL);
    Status = ZwCreateKey(&ParameterKeyHandle,
                         KEY_SET_VALUE | KEY_QUERY_VALUE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         NULL);
    ZwClose(DriverKeyHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create %wZ\\%wZ\n", RegistryPath, &ParameterKeyName);
        return Status;
    }

    KeyInfo = ReadRegistryValue(ParameterKeyHandle, L"CurrentStage");
    if (KeyInfo)
    {
        if (KeyInfo->DataLength != sizeof(ULONG))
        {
            DPRINT1("Invalid data length for CurrentStage: %d\n", KeyInfo->DataLength);
            ExFreePool(KeyInfo);
            return STATUS_UNSUCCESSFUL;
        }

        KeyValue = (PULONG)KeyInfo->Data;

        if ((*KeyValue) + 1 < TestStageMax)
        {
            DPRINT1("Resuming testing after a crash at stage %d\n", (*KeyValue));

            CurrentStage = (TEST_STAGE)((*KeyValue) + 1);
        }
        else
        {
            DPRINT1("Testing was completed on a previous boot\n");
            ExFreePool(KeyInfo);
            return STATUS_UNSUCCESSFUL;
        }

        ExFreePool(KeyInfo);
    }
    else
    {
        DPRINT1("Starting a fresh test\n");
        CurrentStage = (TEST_STAGE)0;
    }

    /* Run the tests */
    while (CurrentStage < TestStageMax)
    {
       RunKernelModeTest(DriverObject,
                         RegistryPath,
                         ParameterKeyHandle,
                         CurrentStage);
       CurrentStage++;
    }

    DPRINT1("Testing is complete!\n");
    ZwClose(ParameterKeyHandle);

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

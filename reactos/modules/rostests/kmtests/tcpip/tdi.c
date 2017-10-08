/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite for TCPIP.sys
 * PROGRAMMER:      Jérôme Gardou <jerome.gardou@reactos.org>
 */

#include <kmt_test.h>
#include <tdikrnl.h>

static
NTSTATUS
NTAPI
IrpCompletionRoutine(
    _In_ PDEVICE_OBJECT    DeviceObject,
    _In_ PIRP              Irp,
    _In_ PVOID             Context)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    KeSetEvent((PKEVENT)Context, IO_NETWORK_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

static
VOID
TestProviderInfo(void)
{
    struct
    {
        UNICODE_STRING DeviceName;
        NTSTATUS CreateStatus, IrpStatus;
        TDI_PROVIDER_INFO ExpectedInfo;
    } TestData[] =
    {
        {
            RTL_CONSTANT_STRING(L"\\Device\\Tcp"),
            STATUS_SUCCESS, STATUS_SUCCESS,
            {
                0x0100,             // Version
                0xFFFFFFFF,         // MaxSendSize
                0,                  // MaxConnectionUserData
                65507,              // MaxDatagramSize
                TDI_SERVICE_CONNECTION_MODE |
                    TDI_SERVICE_ORDERLY_RELEASE |
                    TDI_SERVICE_CONNECTIONLESS_MODE |
                    TDI_SERVICE_ERROR_FREE_DELIVERY |
                    TDI_SERVICE_BROADCAST_SUPPORTED |
                    TDI_SERVICE_DELAYED_ACCEPTANCE |
                    TDI_SERVICE_EXPEDITED_DATA |
                    TDI_SERVICE_NO_ZERO_LENGTH |
                    TDI_SERVICE_DGRAM_CONNECTION |
                    TDI_SERVICE_FORCE_ACCESS_CHECK |
                    TDI_SERVICE_SEND_AND_DISCONNECT |
                    TDI_SERVICE_ACCEPT_LOCAL_ADDR |
                    TDI_SERVICE_ADDRESS_SECURITY |
                    TDI_SERVICE_PREPOST_RECVS |
                    TDI_SERVICE_NO_PUSH,    // ServiceFlags
                1,                  // MinimumLookaheadData
                65535,              // MaximumLookaheadData
                0,                  // NumberOfResources
                {{0}}               // StartTime
            }
        },
        {
            RTL_CONSTANT_STRING(L"\\Device\\Udp"),
            STATUS_SUCCESS, STATUS_SUCCESS,
            {
                0x0100,             // Version
                0xFFFFFFFF,         // MaxSendSize
                0,                  // MaxConnectionUserData
                65507,              // MaxDatagramSize
                TDI_SERVICE_CONNECTION_MODE |
                    TDI_SERVICE_ORDERLY_RELEASE |
                    TDI_SERVICE_CONNECTIONLESS_MODE |
                    TDI_SERVICE_ERROR_FREE_DELIVERY |
                    TDI_SERVICE_BROADCAST_SUPPORTED |
                    TDI_SERVICE_DELAYED_ACCEPTANCE |
                    TDI_SERVICE_EXPEDITED_DATA |
                    TDI_SERVICE_NO_ZERO_LENGTH |
                    TDI_SERVICE_DGRAM_CONNECTION |
                    TDI_SERVICE_FORCE_ACCESS_CHECK |
                    TDI_SERVICE_SEND_AND_DISCONNECT |
                    TDI_SERVICE_ACCEPT_LOCAL_ADDR |
                    TDI_SERVICE_ADDRESS_SECURITY |
                    TDI_SERVICE_PREPOST_RECVS |
                    TDI_SERVICE_NO_PUSH,   // ServiceFlags
                1,                  // MinimumLookaheadData
                65535,              // MaximumLookaheadData
                0,                  // NumberOfResources
                {{0}}               // StartTime
            }
        },
        {
            RTL_CONSTANT_STRING(L"\\Device\\Ip"),
            STATUS_SUCCESS, STATUS_NOT_IMPLEMENTED,
        },
        {
            RTL_CONSTANT_STRING(L"\\Device\\RawIp"),
            STATUS_SUCCESS, STATUS_SUCCESS,
            {
                0x0100,             // Version
                0xFFFFFFFF,         // MaxSendSize
                0,                  // MaxConnectionUserData
                65507,              // MaxDatagramSize
                TDI_SERVICE_CONNECTION_MODE |
                    TDI_SERVICE_ORDERLY_RELEASE |
                    TDI_SERVICE_CONNECTIONLESS_MODE |
                    TDI_SERVICE_ERROR_FREE_DELIVERY |
                    TDI_SERVICE_BROADCAST_SUPPORTED |
                    TDI_SERVICE_DELAYED_ACCEPTANCE |
                    TDI_SERVICE_EXPEDITED_DATA |
                    TDI_SERVICE_NO_ZERO_LENGTH |
                    TDI_SERVICE_DGRAM_CONNECTION |
                    TDI_SERVICE_FORCE_ACCESS_CHECK |
                    TDI_SERVICE_SEND_AND_DISCONNECT |
                    TDI_SERVICE_ACCEPT_LOCAL_ADDR |
                    TDI_SERVICE_ADDRESS_SECURITY |
                    TDI_SERVICE_PREPOST_RECVS |
                    TDI_SERVICE_NO_PUSH,   // ServiceFlags
                1,                  // MinimumLookaheadData
                65535,              // MaximumLookaheadData
                0,                  // NumberOfResources
                {{0}}               // StartTime
            }
        },
        {
            RTL_CONSTANT_STRING(L"\\Device\\IPMULTICAST"),
            STATUS_SUCCESS, STATUS_INVALID_PARAMETER,
        },
    };
    ULONG i;

    for (i = 0; i < (sizeof(TestData) / sizeof(TestData[0])); i++)
    {
        IO_STATUS_BLOCK StatusBlock;
        NTSTATUS Status;
        FILE_OBJECT* FileObject;
        DEVICE_OBJECT* DeviceObject;
        PIRP Irp;
        KEVENT Event;
        PMDL Mdl;
        TDI_PROVIDER_INFO* ProviderInfo;
        HANDLE FileHandle;
        OBJECT_ATTRIBUTES ObjectAttributes;

        trace("Testing device %wZ\n", &TestData[i].DeviceName);

        InitializeObjectAttributes(
            &ObjectAttributes,
            &TestData[i].DeviceName,
            OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);

        Status = ZwCreateFile(
            &FileHandle,
            FILE_READ_DATA | FILE_WRITE_DATA,
            &ObjectAttributes,
            &StatusBlock,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_OPEN,
            0,
            NULL,
            0);
        ok_eq_hex(Status, TestData[i].CreateStatus);
        if (!NT_SUCCESS(Status))
            continue;

        Status = ObReferenceObjectByHandle(
            FileHandle,
            GENERIC_READ,
            *IoFileObjectType,
            KernelMode,
            (PVOID*)&FileObject,
            NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);
        if (!NT_SUCCESS(Status))
            return;

        DeviceObject = IoGetRelatedDeviceObject(FileObject);
        ok(DeviceObject != NULL, "Device object is NULL!\n");
        if (!DeviceObject)
        {
            ObDereferenceObject(FileObject);
            return;
        }

        ProviderInfo = ExAllocatePoolWithTag(NonPagedPool, sizeof(*ProviderInfo), 'tseT');
        ok(ProviderInfo != NULL, "Ran out of memory.\n");
        if (!ProviderInfo)
        {
            ObDereferenceObject(FileObject);
            return;
        }

        Mdl = IoAllocateMdl(ProviderInfo, sizeof(*ProviderInfo), FALSE, FALSE, NULL);
        ok(Mdl != NULL, "Could not allocate the MDL!\n");
        if (!Mdl)
        {
            ExFreePoolWithTag(ProviderInfo, 'tseT');
            ObDereferenceObject(FileObject);
            return;
        }

        MmBuildMdlForNonPagedPool(Mdl);

        /* Build the IRP */
        KeInitializeEvent(&Event, NotificationEvent, FALSE);
        Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
        ok(Irp != NULL, "TdiBuildInternalDeviceControlIrp returned NULL!\n");
        if (!Irp)
        {
            IoFreeMdl(Mdl);
            ExFreePoolWithTag(ProviderInfo, 'tseT');
            ObDereferenceObject(FileObject);
            return;
        }

        TdiBuildQueryInformation(
            Irp,
            DeviceObject,
            FileObject,
            NULL,
            NULL,
            TDI_QUERY_PROVIDER_INFO,
            Mdl);

        IoSetCompletionRoutine(Irp, IrpCompletionRoutine, &Event, TRUE, TRUE, TRUE);

        Status = IoCallDriver(DeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            KeWaitForSingleObject(
                &Event,
                Executive,
                KernelMode,
                FALSE,
                NULL);
            Status = StatusBlock.Status;
        }
        ok_eq_hex(Status, TestData[i].IrpStatus);

        IoFreeIrp(Irp);
        IoFreeMdl(Mdl);
        ObDereferenceObject(FileObject);

        if (!NT_SUCCESS(Status))
        {
            ExFreePoolWithTag(ProviderInfo, 'tseT');
            continue;
        }

        ok_eq_hex(ProviderInfo->Version, TestData[i].ExpectedInfo.Version);
        ok_eq_ulong(ProviderInfo->MaxSendSize, TestData[i].ExpectedInfo.MaxSendSize);
        ok_eq_ulong(ProviderInfo->MaxConnectionUserData, TestData[i].ExpectedInfo.MaxConnectionUserData);
        ok_eq_ulong(ProviderInfo->MaxDatagramSize, TestData[i].ExpectedInfo.MaxDatagramSize);
        ok_eq_hex(ProviderInfo->ServiceFlags, TestData[i].ExpectedInfo.ServiceFlags);
        ok_eq_ulong(ProviderInfo->MinimumLookaheadData, TestData[i].ExpectedInfo.MinimumLookaheadData);
        ok_eq_ulong(ProviderInfo->MaximumLookaheadData, TestData[i].ExpectedInfo.MaximumLookaheadData);
        ok_eq_ulong(ProviderInfo->NumberOfResources, TestData[i].ExpectedInfo.NumberOfResources);

        ExFreePoolWithTag(ProviderInfo, 'tseT');
    }
}

static KSTART_ROUTINE RunTest;
static
VOID
NTAPI
RunTest(
    _In_ PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);

    TestProviderInfo();
}

KMT_MESSAGE_HANDLER TestTdi;
NTSTATUS
TestTdi(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG ControlCode,
    _In_opt_ PVOID Buffer,
    _In_ SIZE_T InLength,
    _Inout_ PSIZE_T OutLength
)
{
    PKTHREAD Thread;

    Thread = KmtStartThread(RunTest, NULL);
    KmtFinishThread(Thread, NULL);

    return STATUS_SUCCESS;
}

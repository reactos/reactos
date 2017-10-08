/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite for TCPIP.sys
 * PROGRAMMER:      Jérôme Gardou <jerome.gardou@reactos.org>
 */

#include <kmt_test.h>
#include <tdikrnl.h>
#include <ndk/rtlfuncs.h>

#include <sys/param.h>

#include "tcpip.h"

#define TAG_TEST 'tseT'

#if BYTE_ORDER == LITTLE_ENDIAN
USHORT
htons(USHORT x)
{
    return ((x & 0x00FF) << 8) | ((x & 0xFF00) >> 8);
}
#else
#define htons(x) (x)
#endif

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
TestTcpConnect(void)
{
    PIRP Irp;
    HANDLE AddressHandle, ConnectionHandle;
    FILE_OBJECT* ConnectionFileObject;
    DEVICE_OBJECT* DeviceObject;
    UNICODE_STRING TcpDeviceName = RTL_CONSTANT_STRING(L"\\Device\\Tcp");
    NTSTATUS Status;
    PFILE_FULL_EA_INFORMATION FileInfo;
    TA_IP_ADDRESS* IpAddress;
    TA_IP_ADDRESS ConnectAddress, ReturnAddress;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK StatusBlock;
    ULONG FileInfoSize;
    IN_ADDR InAddr;
    LPCWSTR AddressTerminator;
    CONNECTION_CONTEXT ConnectionContext = (CONNECTION_CONTEXT)0xC0CAC01A;
    KEVENT Event;
    TDI_CONNECTION_INFORMATION RequestInfo, ReturnInfo;

    /* Create a TCP address file */
    FileInfoSize = FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[TDI_TRANSPORT_ADDRESS_LENGTH]) + 1 + sizeof(TA_IP_ADDRESS);
    FileInfo = ExAllocatePoolWithTag(NonPagedPool,
            FileInfoSize,
            TAG_TEST);
    ok(FileInfo != NULL, "");
    RtlZeroMemory(FileInfo, FileInfoSize);

    FileInfo->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
    FileInfo->EaValueLength = sizeof(TA_IP_ADDRESS);
    RtlCopyMemory(&FileInfo->EaName[0], TdiTransportAddress, TDI_TRANSPORT_ADDRESS_LENGTH);

    IpAddress = (PTA_IP_ADDRESS)(&FileInfo->EaName[TDI_TRANSPORT_ADDRESS_LENGTH + 1]);
    IpAddress->TAAddressCount = 1;
    IpAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
    IpAddress->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
    IpAddress->Address[0].Address[0].sin_port = htons(TEST_CONNECT_CLIENT_PORT);
    Status = RtlIpv4StringToAddressW(L"127.0.0.1", TRUE, &AddressTerminator, &InAddr);
    ok_eq_hex(Status, STATUS_SUCCESS);
    IpAddress->Address[0].Address[0].in_addr = InAddr.S_un.S_addr;

    InitializeObjectAttributes(&ObjectAttributes,
            &TcpDeviceName,
            OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
            NULL,
            NULL);

    Status = ZwCreateFile(
        &AddressHandle,
        GENERIC_READ | GENERIC_WRITE,
        &ObjectAttributes,
        &StatusBlock,
        0,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN_IF,
        0L,
        FileInfo,
        FileInfoSize);
    ok_eq_hex(Status, STATUS_SUCCESS);

    ExFreePoolWithTag(FileInfo, TAG_TEST);

    /* Create a TCP connection file */
    FileInfoSize = FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[TDI_CONNECTION_CONTEXT_LENGTH]) + 1 + sizeof(CONNECTION_CONTEXT);
    FileInfo = ExAllocatePoolWithTag(NonPagedPool,
            FileInfoSize,
            TAG_TEST);
    ok(FileInfo != NULL, "");
    RtlZeroMemory(FileInfo, FileInfoSize);

    FileInfo->EaNameLength = TDI_CONNECTION_CONTEXT_LENGTH;
    FileInfo->EaValueLength = sizeof(CONNECTION_CONTEXT);
    RtlCopyMemory(&FileInfo->EaName[0], TdiConnectionContext, TDI_CONNECTION_CONTEXT_LENGTH);
    *((CONNECTION_CONTEXT*)&FileInfo->EaName[TDI_CONNECTION_CONTEXT_LENGTH + 1]) = ConnectionContext;

    Status = ZwCreateFile(
        &ConnectionHandle,
        GENERIC_READ | GENERIC_WRITE,
        &ObjectAttributes,
        &StatusBlock,
        0,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN_IF,
        0L,
        FileInfo,
        FileInfoSize);
    ok_eq_hex(Status, STATUS_SUCCESS);

    ExFreePoolWithTag(FileInfo, TAG_TEST);

    /* Get the file and device object for the upcoming IRPs */
    Status = ObReferenceObjectByHandle(
        ConnectionHandle,
        GENERIC_READ,
        *IoFileObjectType,
        KernelMode,
        (PVOID*)&ConnectionFileObject,
        NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    DeviceObject = IoGetRelatedDeviceObject(ConnectionFileObject);
    ok(DeviceObject != NULL, "Device object is NULL!\n");

    /* Associate the connection file and the address */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    ok(Irp != NULL, "IoAllocateIrp failed.\n");

    TdiBuildAssociateAddress(Irp, DeviceObject, ConnectionFileObject, NULL, NULL, AddressHandle);
    IoSetCompletionRoutine(Irp, IrpCompletionRoutine, &Event, TRUE, TRUE, TRUE);

    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        trace("Associate address IRP is pending.\n");
        KeWaitForSingleObject(
            &Event,
            Executive,
            KernelMode,
            FALSE,
            NULL);
        Status = Irp->IoStatus.Status;
    }
    ok_eq_hex(Status, STATUS_SUCCESS);
    IoFreeIrp(Irp);


    KeClearEvent(&Event);

    /* Build the connect IRP. */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    ok(Irp != NULL, "IoAllocateIrp failed.\n");

    /* Prepare the request */
    RtlZeroMemory(&RequestInfo, sizeof(RequestInfo));
    RtlZeroMemory(&ConnectAddress, sizeof(ConnectAddress));
    RequestInfo.RemoteAddressLength = sizeof(TA_IP_ADDRESS);
    RequestInfo.RemoteAddress = &ConnectAddress;
    ConnectAddress.TAAddressCount = 1;
    ConnectAddress.Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
    ConnectAddress.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
    ConnectAddress.Address[0].Address[0].sin_port = htons(TEST_CONNECT_SERVER_PORT);
    Status = RtlIpv4StringToAddressW(L"127.0.0.1", TRUE, &AddressTerminator, &InAddr);
    ConnectAddress.Address[0].Address[0].in_addr = InAddr.S_un.S_addr;

    /* See what we will get in exchange */
    RtlZeroMemory(&ReturnInfo, sizeof(ReturnInfo));
    RtlZeroMemory(&ReturnAddress, sizeof(ReturnAddress));
    ReturnInfo.RemoteAddressLength = sizeof(TA_IP_ADDRESS);
    ReturnInfo.RemoteAddress = &ReturnAddress;

    TdiBuildConnect(Irp,
        DeviceObject,
        ConnectionFileObject,
        NULL,
        NULL,
        NULL,
        &RequestInfo,
        &ReturnInfo);
    IoSetCompletionRoutine(Irp, IrpCompletionRoutine, &Event, TRUE, TRUE, TRUE);

    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        trace("Connect IRP is pending.\n");
        KeWaitForSingleObject(
            &Event,
            Executive,
            KernelMode,
            FALSE,
            NULL);
        Status = Irp->IoStatus.Status;
        trace("Connect IRP completed.\n");
    }
    ok_eq_hex(Status, STATUS_SUCCESS);
    IoFreeIrp(Irp);

    /* The IRP doesn't touch the return info */
    ok_eq_long(ReturnInfo.RemoteAddressLength, sizeof(TA_IP_ADDRESS));
    ok_eq_pointer(ReturnInfo.RemoteAddress, &ReturnAddress);
    ok_eq_long(ReturnInfo.OptionsLength, 0);
    ok_eq_pointer(ReturnInfo.Options, NULL);
    ok_eq_long(ReturnInfo.UserDataLength, 0);
    ok_eq_pointer(ReturnInfo.UserData, NULL);

    ok_eq_long(ReturnAddress.TAAddressCount, 0);
    ok_eq_hex(ReturnAddress.Address[0].AddressType, 0);
    ok_eq_hex(ReturnAddress.Address[0].AddressLength, 0);
    ok_eq_hex(ReturnAddress.Address[0].Address[0].sin_port, 0);
    ok_eq_hex(ReturnAddress.Address[0].Address[0].in_addr, 0);

    ObDereferenceObject(ConnectionFileObject);

    ZwClose(ConnectionHandle);
    ZwClose(AddressHandle);
}

static KSTART_ROUTINE RunTest;
static
VOID
NTAPI
RunTest(
    _In_ PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);

    TestTcpConnect();
}

KMT_MESSAGE_HANDLER TestConnect;
NTSTATUS
TestConnect(
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

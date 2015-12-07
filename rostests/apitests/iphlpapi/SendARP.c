/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for SendARP
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include <apitest.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <tcpioctl.h>
#define NTOS_MODE_USER
#include <ndk/iofuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>

static VOID TestUM(IPAddr * Source)
{
    DWORD Err;
    ULONG Hw[2];
    DWORD Size;
    BOOL Tested = FALSE;
    PIP_ADAPTER_ADDRESSES Addresses, Current;

    Err = SendARP(0, 0, NULL, NULL);
    ok(Err == ERROR_INVALID_PARAMETER, "Expected error: ERROR_INVALID_PARAMETER. Got: %lx\n", Err);

    Size = 4;
    Err = SendARP(0, 0, Hw, &Size);
    ok(Err == ERROR_GEN_FAILURE, "Expected error: ERROR_GEN_FAILURE. Got: %lx\n", Err);

    Size = 6;
    Err = SendARP(0, 0, Hw, &Size);
    ok(Err == ERROR_GEN_FAILURE, "Expected error: ERROR_GEN_FAILURE. Got: %lx\n", Err);

    Size = 8;
    Err = SendARP(0, 0, Hw, &Size);
    ok(Err == ERROR_GEN_FAILURE, "Expected error: ERROR_GEN_FAILURE. Got: %lx\n", Err);

    Size = sizeof(IP_ADAPTER_ADDRESSES);
    Addresses = (PIP_ADAPTER_ADDRESSES)malloc(Size);
    if (!Addresses)
    {
        skip("Memory failure\n");
        return;
    }

    Err = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_FRIENDLY_NAME | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST, NULL, Addresses, &Size);
    if (Err == ERROR_BUFFER_OVERFLOW)
    {
        free(Addresses);
        Addresses = (PIP_ADAPTER_ADDRESSES)malloc(Size);
        if (!Addresses)
        {
            skip("Memory failure\n");
            return;
        }

        Err = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_FRIENDLY_NAME | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST, NULL, Addresses, &Size);
    }

    if (Err != ERROR_SUCCESS)
    {
        skip("GetAdaptersAddresses() failure\n");
        free(Addresses);
        return;
    }

    for (Current = Addresses; Current; Current = Current->Next)
    {
        PSOCKADDR_IN SockAddr;
        IPAddr IpAddr;

        if (Current->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
            continue;

        if (Current->OperStatus != IfOperStatusUp)
            continue;

        if (!Current->FirstUnicastAddress)
            continue;

        ok(Current->FirstUnicastAddress->Address.iSockaddrLength == sizeof(SOCKADDR_IN), "Unexpected length: %u\n", Current->FirstUnicastAddress->Address.iSockaddrLength);
        SockAddr = (PSOCKADDR_IN)Current->FirstUnicastAddress->Address.lpSockaddr;
        IpAddr = SockAddr->sin_addr.S_un.S_addr;

        trace("IP address found: %lu.%lu.%lu.%lu\n", IpAddr & 0xFF, (IpAddr >> 8) & 0xFF, (IpAddr >> 16) & 0xFF, (IpAddr >> 24) & 0xFF);

        Size = 4;
        Err = SendARP(IpAddr, 0, Hw, &Size);
        ok(Err == ERROR_NO_SYSTEM_RESOURCES, "Expected error: ERROR_NO_SYSTEM_RESOURCES. Got: %lx\n", Err);

        Size = 6;
        Err = SendARP(IpAddr, 0, Hw, &Size);
        ok(Err == ERROR_SUCCESS, "Expected error: ERROR_SUCCESS. Got: %lx\n", Err);

        Size = 8;
        Err = SendARP(IpAddr, 0, Hw, &Size);
        ok(Err == ERROR_SUCCESS, "Expected error: ERROR_SUCCESS. Got: %lx\n", Err);
        Err = SendARP(IpAddr, 0x08080808, Hw, &Size);
        ok(Err == ERROR_SUCCESS, "Expected error: ERROR_SUCCESS. Got: %lx\n", Err);

        Size = 4;
        Err = SendARP(IpAddr, IpAddr, Hw, &Size);
        ok(Err == ERROR_NO_SYSTEM_RESOURCES, "Expected error: ERROR_NO_SYSTEM_RESOURCES. Got: %lx\n", Err);

        Size = 6;
        Err = SendARP(IpAddr, IpAddr, Hw, &Size);
        ok(Err == ERROR_SUCCESS, "Expected error: ERROR_SUCCESS. Got: %lx\n", Err);

        Size = 8;
        Err = SendARP(IpAddr, IpAddr, Hw, &Size);
        ok(Err == ERROR_SUCCESS, "Expected error: ERROR_SUCCESS. Got: %lx\n", Err);

        *Source = IpAddr;
        Tested = TRUE;
        break;
    }

    if (!Tested)
    {
        skip("No suitable interface found\n");
    }

    free(Addresses);
}

static VOID TestKM(IPAddr Source)
{
    HANDLE hDevice;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING DevName = RTL_CONSTANT_STRING(L"\\Device\\Ip");
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hEvent;
    ULONG Hw[2];
    ULONG Ip[2];

    InitializeObjectAttributes(&ObjectAttributes,
                               &DevName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtCreateFile(&hDevice, GENERIC_EXECUTE, &ObjectAttributes,
                          &IoStatusBlock, 0, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN_IF,
                          0, NULL, 0);
    if (!NT_SUCCESS(Status))
    {
        skip("NtCreateFile() failed with status: %lx\n", Status);
        return;
    }

    hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!hEvent)
    {
        skip("CreateEventW() with error: %lx\n", GetLastError());
        CloseHandle(hDevice);
        return;
    }

    IoStatusBlock.Status = STATUS_SUCCESS;
    IoStatusBlock.Information = 0;
    Status = NtDeviceIoControlFile(hDevice, hEvent, NULL, NULL, &IoStatusBlock, IOCTL_QUERY_IP_HW_ADDRESS, NULL, 0, NULL, 0);
    if (Status == STATUS_PENDING)
    {
        NtWaitForSingleObject(hEvent, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    ok(Status == STATUS_INVALID_BUFFER_SIZE, "NtDeviceIoControlFile() failed with unexpected status: %lx\n", Status);
    ok(IoStatusBlock.Information == 0, "Excepted 0, got: %lu\n", IoStatusBlock.Information);

    memset(Ip, 0, sizeof(Ip));
    ResetEvent(hEvent);
    IoStatusBlock.Status = STATUS_SUCCESS;
    IoStatusBlock.Information = 0;
    Status = NtDeviceIoControlFile(hDevice, hEvent, NULL, NULL, &IoStatusBlock, IOCTL_QUERY_IP_HW_ADDRESS, Ip, sizeof(Ip), NULL, 0);
    if (Status == STATUS_PENDING)
    {
        NtWaitForSingleObject(hEvent, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    ok(Status == STATUS_INVALID_BUFFER_SIZE, "NtDeviceIoControlFile() failed with unexpected status: %lx\n", Status);
    ok(IoStatusBlock.Information == 0, "Excepted 0, got: %lu\n", IoStatusBlock.Information);

    ResetEvent(hEvent);
    IoStatusBlock.Status = STATUS_SUCCESS;
    IoStatusBlock.Information = 0;
    Status = NtDeviceIoControlFile(hDevice, hEvent, NULL, NULL, &IoStatusBlock, IOCTL_QUERY_IP_HW_ADDRESS, Ip, sizeof(Ip), Hw, 4);
    if (Status == STATUS_PENDING)
    {
        NtWaitForSingleObject(hEvent, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    ok(Status == STATUS_UNSUCCESSFUL, "NtDeviceIoControlFile() failed with unexpected status: %lx\n", Status);
    ok(IoStatusBlock.Information == 0, "Excepted 0, got: %lu\n", IoStatusBlock.Information);

    ResetEvent(hEvent);
    IoStatusBlock.Status = STATUS_SUCCESS;
    IoStatusBlock.Information = 0;
    Status = NtDeviceIoControlFile(hDevice, hEvent, NULL, NULL, &IoStatusBlock, IOCTL_QUERY_IP_HW_ADDRESS, Ip, sizeof(Ip), Hw, 6);
    if (Status == STATUS_PENDING)
    {
        NtWaitForSingleObject(hEvent, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    ok(Status == STATUS_UNSUCCESSFUL, "NtDeviceIoControlFile() failed with unexpected status: %lx\n", Status);
    ok(IoStatusBlock.Information == 0, "Excepted 0, got: %lu\n", IoStatusBlock.Information);

    ResetEvent(hEvent);
    IoStatusBlock.Status = STATUS_SUCCESS;
    IoStatusBlock.Information = 0;
    Status = NtDeviceIoControlFile(hDevice, hEvent, NULL, NULL, &IoStatusBlock, IOCTL_QUERY_IP_HW_ADDRESS, Ip, sizeof(Ip), Hw, 8);
    if (Status == STATUS_PENDING)
    {
        NtWaitForSingleObject(hEvent, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    ok(Status == STATUS_UNSUCCESSFUL, "NtDeviceIoControlFile() failed with unexpected status: %lx\n", Status);
    ok(IoStatusBlock.Information == 0, "Excepted 0, got: %lu\n", IoStatusBlock.Information);

    Ip[0] = Source;
    ResetEvent(hEvent);
    IoStatusBlock.Status = STATUS_SUCCESS;
    IoStatusBlock.Information = 0;
    Status = NtDeviceIoControlFile(hDevice, hEvent, NULL, NULL, &IoStatusBlock, IOCTL_QUERY_IP_HW_ADDRESS, Ip, sizeof(Ip[0]), NULL, 0);
    if (Status == STATUS_PENDING)
    {
        NtWaitForSingleObject(hEvent, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    ok(Status == STATUS_INVALID_BUFFER_SIZE, "NtDeviceIoControlFile() failed with unexpected status: %lx\n", Status);
    ok(IoStatusBlock.Information == 0, "Excepted 0, got: %lu\n", IoStatusBlock.Information);

    ResetEvent(hEvent);
    IoStatusBlock.Status = STATUS_SUCCESS;
    IoStatusBlock.Information = 0;
    Status = NtDeviceIoControlFile(hDevice, hEvent, NULL, NULL, &IoStatusBlock, IOCTL_QUERY_IP_HW_ADDRESS, Ip, sizeof(Ip[0]), Hw, 4);
    if (Status == STATUS_PENDING)
    {
        NtWaitForSingleObject(hEvent, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    ok(Status == STATUS_INVALID_BUFFER_SIZE, "NtDeviceIoControlFile() failed with unexpected status: %lx\n", Status);
    ok(IoStatusBlock.Information == 0, "Excepted 0, got: %lu\n", IoStatusBlock.Information);

    ResetEvent(hEvent);
    IoStatusBlock.Status = STATUS_SUCCESS;
    IoStatusBlock.Information = 0;
    Status = NtDeviceIoControlFile(hDevice, hEvent, NULL, NULL, &IoStatusBlock, IOCTL_QUERY_IP_HW_ADDRESS, Ip, sizeof(Ip[0]), Hw, 6);
    if (Status == STATUS_PENDING)
    {
        NtWaitForSingleObject(hEvent, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    ok(Status == STATUS_INVALID_BUFFER_SIZE, "NtDeviceIoControlFile() failed with status: %lx\n", Status);
    ok(IoStatusBlock.Information == 0, "Excepted 0, got: %lu\n", IoStatusBlock.Information);

    ResetEvent(hEvent);
    IoStatusBlock.Status = STATUS_SUCCESS;
    IoStatusBlock.Information = 0;
    Status = NtDeviceIoControlFile(hDevice, hEvent, NULL, NULL, &IoStatusBlock, IOCTL_QUERY_IP_HW_ADDRESS, Ip, sizeof(Ip[0]), Hw, 8);
    if (Status == STATUS_PENDING)
    {
        NtWaitForSingleObject(hEvent, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    ok(Status == STATUS_INVALID_BUFFER_SIZE, "NtDeviceIoControlFile() failed with status: %lx\n", Status);
    ok(IoStatusBlock.Information == 0, "Excepted 0, got: %lu\n", IoStatusBlock.Information);

    ResetEvent(hEvent);
    IoStatusBlock.Status = STATUS_SUCCESS;
    IoStatusBlock.Information = 0;
    Status = NtDeviceIoControlFile(hDevice, hEvent, NULL, NULL, &IoStatusBlock, IOCTL_QUERY_IP_HW_ADDRESS, Ip, sizeof(Ip), NULL, 0);
    if (Status == STATUS_PENDING)
    {
        NtWaitForSingleObject(hEvent, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    ok(Status == STATUS_INVALID_BUFFER_SIZE, "NtDeviceIoControlFile() failed with unexpected status: %lx\n", Status);
    ok(IoStatusBlock.Information == 0, "Excepted 0, got: %lu\n", IoStatusBlock.Information);

    ResetEvent(hEvent);
    IoStatusBlock.Status = STATUS_SUCCESS;
    IoStatusBlock.Information = 0;
    Status = NtDeviceIoControlFile(hDevice, hEvent, NULL, NULL, &IoStatusBlock, IOCTL_QUERY_IP_HW_ADDRESS, Ip, sizeof(Ip), Hw, 4);
    if (Status == STATUS_PENDING)
    {
        NtWaitForSingleObject(hEvent, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    ok(Status == STATUS_INSUFFICIENT_RESOURCES, "NtDeviceIoControlFile() failed with unexpected status: %lx\n", Status);
    ok(IoStatusBlock.Information == 0, "Excepted 0, got: %lu\n", IoStatusBlock.Information);

    ResetEvent(hEvent);
    IoStatusBlock.Status = STATUS_SUCCESS;
    IoStatusBlock.Information = 0;
    Status = NtDeviceIoControlFile(hDevice, hEvent, NULL, NULL, &IoStatusBlock, IOCTL_QUERY_IP_HW_ADDRESS, Ip, sizeof(Ip), Hw, 6);
    if (Status == STATUS_PENDING)
    {
        NtWaitForSingleObject(hEvent, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    ok(Status == STATUS_SUCCESS, "NtDeviceIoControlFile() failed with status: %lx\n", Status);
    ok(IoStatusBlock.Information == 6, "Excepted 6, got: %lu\n", IoStatusBlock.Information);

    ResetEvent(hEvent);
    IoStatusBlock.Status = STATUS_SUCCESS;
    IoStatusBlock.Information = 0;
    Status = NtDeviceIoControlFile(hDevice, hEvent, NULL, NULL, &IoStatusBlock, IOCTL_QUERY_IP_HW_ADDRESS, Ip, sizeof(Ip), Hw, 8);
    if (Status == STATUS_PENDING)
    {
        NtWaitForSingleObject(hEvent, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    ok(Status == STATUS_SUCCESS, "NtDeviceIoControlFile() failed with status: %lx\n", Status);
    ok(IoStatusBlock.Information == 6, "Excepted 6, got: %lu\n", IoStatusBlock.Information);

    Ip[1] = Source;
    ResetEvent(hEvent);
    IoStatusBlock.Status = STATUS_SUCCESS;
    IoStatusBlock.Information = 0;
    Status = NtDeviceIoControlFile(hDevice, hEvent, NULL, NULL, &IoStatusBlock, IOCTL_QUERY_IP_HW_ADDRESS, Ip, sizeof(Ip), NULL, 0);
    if (Status == STATUS_PENDING)
    {
        NtWaitForSingleObject(hEvent, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    ok(Status == STATUS_INVALID_BUFFER_SIZE, "NtDeviceIoControlFile() failed with unexpected status: %lx\n", Status);
    ok(IoStatusBlock.Information == 0, "Excepted 0, got: %lu\n", IoStatusBlock.Information);

    ResetEvent(hEvent);
    IoStatusBlock.Status = STATUS_SUCCESS;
    IoStatusBlock.Information = 0;
    Status = NtDeviceIoControlFile(hDevice, hEvent, NULL, NULL, &IoStatusBlock, IOCTL_QUERY_IP_HW_ADDRESS, Ip, sizeof(Ip), Hw, 4);
    if (Status == STATUS_PENDING)
    {
        NtWaitForSingleObject(hEvent, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    ok(Status == STATUS_INSUFFICIENT_RESOURCES, "NtDeviceIoControlFile() failed with unexpected status: %lx\n", Status);
    ok(IoStatusBlock.Information == 0, "Excepted 0, got: %lu\n", IoStatusBlock.Information);

    ResetEvent(hEvent);
    IoStatusBlock.Status = STATUS_SUCCESS;
    IoStatusBlock.Information = 0;
    Status = NtDeviceIoControlFile(hDevice, hEvent, NULL, NULL, &IoStatusBlock, IOCTL_QUERY_IP_HW_ADDRESS, Ip, sizeof(Ip), Hw, 6);
    if (Status == STATUS_PENDING)
    {
        NtWaitForSingleObject(hEvent, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    ok(Status == STATUS_SUCCESS, "NtDeviceIoControlFile() failed with status: %lx\n", Status);
    ok(IoStatusBlock.Information == 6, "Excepted 6, got: %lu\n", IoStatusBlock.Information);

    ResetEvent(hEvent);
    IoStatusBlock.Status = STATUS_SUCCESS;
    IoStatusBlock.Information = 0;
    Status = NtDeviceIoControlFile(hDevice, hEvent, NULL, NULL, &IoStatusBlock, IOCTL_QUERY_IP_HW_ADDRESS, Ip, sizeof(Ip), Hw, 8);
    if (Status == STATUS_PENDING)
    {
        NtWaitForSingleObject(hEvent, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    ok(Status == STATUS_SUCCESS, "NtDeviceIoControlFile() failed with status: %lx\n", Status);
    ok(IoStatusBlock.Information == 6, "Excepted 6, got: %lu\n", IoStatusBlock.Information);

    Ip[1] = 0x08080808;
    ResetEvent(hEvent);
    IoStatusBlock.Status = STATUS_SUCCESS;
    IoStatusBlock.Information = 0;
    Status = NtDeviceIoControlFile(hDevice, hEvent, NULL, NULL, &IoStatusBlock, IOCTL_QUERY_IP_HW_ADDRESS, Ip, sizeof(Ip), Hw, 8);
    if (Status == STATUS_PENDING)
    {
        NtWaitForSingleObject(hEvent, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    ok(Status == STATUS_SUCCESS, "NtDeviceIoControlFile() failed with status: %lx\n", Status);
    ok(IoStatusBlock.Information == 6, "Excepted 6, got: %lu\n", IoStatusBlock.Information);

    CloseHandle(hEvent);
    CloseHandle(hDevice);
}

START_TEST(SendARP)
{
    IPAddr Source = 0;

    TestUM(&Source);
    if (!Source)
    {
        skip("No suitable interface found\n");
        return;
    }

    TestKM(Source);
}

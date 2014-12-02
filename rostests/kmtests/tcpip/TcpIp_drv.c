/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite for tcpip.sys
 * PROGRAMMER:      Jérôme Gardou <jerome.gardou@reactos.org>
 */

#include <kmt_test.h>
#include "tcpip.h"

extern KMT_MESSAGE_HANDLER TestTdi;

static struct
{
    ULONG ControlCode;
    PKMT_MESSAGE_HANDLER Handler;
} MessageHandlers[] =
{
    { IOCTL_TEST_TDI, TestTdi },
};

NTSTATUS
TestEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PCUNICODE_STRING RegistryPath,
    _Out_ PCWSTR *DeviceName,
    _Inout_ INT *Flags)
{
    ULONG i;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);
    UNREFERENCED_PARAMETER(Flags);

    *DeviceName = L"TcpIp";

    for (i = 0; i < (sizeof(MessageHandlers) / sizeof(MessageHandlers[0])); i++)
        KmtRegisterMessageHandler(MessageHandlers[i].ControlCode, NULL, MessageHandlers[i].Handler);

    trace("TcpIp test driver loaded.\n");

    return STATUS_SUCCESS;
}

VOID
TestUnload(
    _In_ PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(DriverObject);
}


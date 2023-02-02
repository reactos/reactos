/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Device Interface functions test
 * PROGRAMMER:      Filip Navara <xnavara@volny.cz>
 */

/* TODO: what's with the prototypes at the top, what's with the if-ed out part? Doesn't process most results */

#include <kmt_test.h>
#include <poclass.h>

#define NDEBUG
#include <debug.h>

static UCHAR NotificationContext;

static DRIVER_NOTIFICATION_CALLBACK_ROUTINE NotificationCallback;
static
NTSTATUS
NTAPI
NotificationCallback(
    _In_ PVOID NotificationStructure,
    _Inout_opt_ PVOID Context)
{
    PDEVICE_INTERFACE_CHANGE_NOTIFICATION Notification = NotificationStructure;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;

    ok_irql(PASSIVE_LEVEL);
    ok_eq_pointer(Context, &NotificationContext);
    ok_eq_uint(Notification->Version, 1);
    ok_eq_uint(Notification->Size, sizeof(*Notification));

    /* symbolic link must exist */
    trace("Interface change: %wZ\n", Notification->SymbolicLinkName);
    InitializeObjectAttributes(&ObjectAttributes,
                               Notification->SymbolicLinkName,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwOpenSymbolicLinkObject(&Handle, GENERIC_READ, &ObjectAttributes);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (!skip(NT_SUCCESS(Status), "No symbolic link\n"))
    {
        Status = ObCloseHandle(Handle, KernelMode);
        ok_eq_hex(Status, STATUS_SUCCESS);
    }
    return STATUS_SUCCESS;
}

static
VOID
Test_IoRegisterPlugPlayNotification(VOID)
{
    NTSTATUS Status;
    PVOID NotificationEntry;

    Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                            PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                            (PVOID)&GUID_DEVICE_SYS_BUTTON,
                                            KmtDriverObject,
                                            NotificationCallback,
                                            &NotificationContext,
                                            &NotificationEntry);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (!skip(NT_SUCCESS(Status), "PlugPlayNotification not registered\n"))
    {
        Status = IoUnregisterPlugPlayNotification(NotificationEntry);
        ok_eq_hex(Status, STATUS_SUCCESS);
    }
}

static
VOID
Test_IoSetDeviceInterface(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING SymbolicLinkName;
    PWCHAR Buffer;
    ULONG BufferSize;

    /* Invalid prefix or GUID */
    KmtStartSeh()
        Status = IoSetDeviceInterfaceState(NULL, TRUE);
    KmtEndSeh(STATUS_SUCCESS)
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    RtlInitEmptyUnicodeString(&SymbolicLinkName, NULL, 0);
    KmtStartSeh()
        Status = IoSetDeviceInterfaceState(&SymbolicLinkName, TRUE);
    KmtEndSeh(STATUS_SUCCESS)
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    RtlInitUnicodeString(&SymbolicLinkName, L"\\??");
    KmtStartSeh()
        Status = IoSetDeviceInterfaceState(&SymbolicLinkName, TRUE);
    KmtEndSeh(STATUS_SUCCESS)
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    RtlInitUnicodeString(&SymbolicLinkName, L"\\??\\");
    KmtStartSeh()
        Status = IoSetDeviceInterfaceState(&SymbolicLinkName, TRUE);
    KmtEndSeh(STATUS_SUCCESS)
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    RtlInitUnicodeString(&SymbolicLinkName, L"\\??\\\\");
    KmtStartSeh()
        Status = IoSetDeviceInterfaceState(&SymbolicLinkName, TRUE);
    KmtEndSeh(STATUS_SUCCESS)
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    RtlInitUnicodeString(&SymbolicLinkName, L"\\??\\{aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa}");
    KmtStartSeh()
        Status = IoSetDeviceInterfaceState(&SymbolicLinkName, TRUE);
    KmtEndSeh(STATUS_SUCCESS)
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    /* Valid prefix & GUID, invalid device node */
    RtlInitUnicodeString(&SymbolicLinkName, L"\\??\\X{aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa}");
    KmtStartSeh()
        Status = IoSetDeviceInterfaceState(&SymbolicLinkName, TRUE);
    KmtEndSeh(STATUS_SUCCESS)
    ok_eq_hex(Status, STATUS_OBJECT_NAME_NOT_FOUND);

    RtlInitUnicodeString(&SymbolicLinkName, L"\\\\?\\X{aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa}");
    KmtStartSeh()
        Status = IoSetDeviceInterfaceState(&SymbolicLinkName, TRUE);
    KmtEndSeh(STATUS_SUCCESS)
    ok_eq_hex(Status, STATUS_OBJECT_NAME_NOT_FOUND);

    RtlInitUnicodeString(&SymbolicLinkName, L"\\??\\X{aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa}\\");
    KmtStartSeh()
        Status = IoSetDeviceInterfaceState(&SymbolicLinkName, TRUE);
    KmtEndSeh(STATUS_SUCCESS)
    ok_eq_hex(Status, STATUS_OBJECT_NAME_NOT_FOUND);

    RtlInitUnicodeString(&SymbolicLinkName, L"\\??\\#{aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa}");
    KmtStartSeh()
        Status = IoSetDeviceInterfaceState(&SymbolicLinkName, TRUE);
    KmtEndSeh(STATUS_SUCCESS)
    ok_eq_hex(Status, STATUS_OBJECT_NAME_NOT_FOUND);

    /* Must not read past the buffer */
    RtlInitUnicodeString(&SymbolicLinkName, L"\\??\\#{aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa}");
    BufferSize = SymbolicLinkName.Length;
    Buffer = KmtAllocateGuarded(BufferSize);
    if (!skip(Buffer != NULL, "Failed to allocate %lu bytes\n", BufferSize))
    {
        RtlCopyMemory(Buffer, SymbolicLinkName.Buffer, BufferSize);
        SymbolicLinkName.Buffer = Buffer;
        SymbolicLinkName.MaximumLength = BufferSize;
        KmtStartSeh()
            Status = IoSetDeviceInterfaceState(&SymbolicLinkName, TRUE);
        KmtEndSeh(STATUS_SUCCESS)
        ok_eq_hex(Status, STATUS_OBJECT_NAME_NOT_FOUND);
        KmtFreeGuarded(Buffer);
    }

    RtlInitUnicodeString(&SymbolicLinkName, L"\\??\\#aaaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa}");
    BufferSize = SymbolicLinkName.Length;
    Buffer = KmtAllocateGuarded(BufferSize);
    if (!skip(Buffer != NULL, "Failed to allocate %lu bytes\n", BufferSize))
    {
        RtlCopyMemory(Buffer, SymbolicLinkName.Buffer, BufferSize);
        SymbolicLinkName.Buffer = Buffer;
        SymbolicLinkName.MaximumLength = BufferSize;
        KmtStartSeh()
            Status = IoSetDeviceInterfaceState(&SymbolicLinkName, TRUE);
        KmtEndSeh(STATUS_SUCCESS)
        ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
        KmtFreeGuarded(Buffer);
    }
}

START_TEST(IoDeviceInterface)
{
    Test_IoRegisterPlugPlayNotification();
    Test_IoSetDeviceInterface();
}

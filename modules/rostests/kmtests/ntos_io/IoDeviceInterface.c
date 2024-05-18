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

#if 0
NTSTATUS
(NTAPI *IoGetDeviceInterfaces_Func)(
   IN CONST GUID *InterfaceClassGuid,
   IN PDEVICE_OBJECT PhysicalDeviceObject OPTIONAL,
   IN ULONG Flags,
   OUT PWSTR *SymbolicLinkList);

NTSTATUS NTAPI
ReactOS_IoGetDeviceInterfaces(
   IN CONST GUID *InterfaceClassGuid,
   IN PDEVICE_OBJECT PhysicalDeviceObject OPTIONAL,
   IN ULONG Flags,
   OUT PWSTR *SymbolicLinkList);
#endif /* 0 */

static VOID DeviceInterfaceTest_Func()
{
   NTSTATUS Status;
   PWSTR SymbolicLinkList;
   PWSTR SymbolicLinkListPtr;
   GUID Guid = {0x378de44c, 0x56ef, 0x11d1, {0xbc, 0x8c, 0x00, 0xa0, 0xc9, 0x14, 0x05, 0xdd}};

   Status = IoGetDeviceInterfaces(
      &Guid,
      NULL,
      0,
      &SymbolicLinkList);

   ok(NT_SUCCESS(Status),
         "IoGetDeviceInterfaces failed with status 0x%X\n",
         (unsigned int)Status);
   if (!NT_SUCCESS(Status))
   {
      return;
   }

   DPRINT("IoGetDeviceInterfaces results:\n");
   for (SymbolicLinkListPtr = SymbolicLinkList;
        SymbolicLinkListPtr[0] != 0 && SymbolicLinkListPtr[1] != 0;
        SymbolicLinkListPtr += wcslen(SymbolicLinkListPtr) + 1)
   {
      DPRINT1("Symbolic Link: %S\n", SymbolicLinkListPtr);
   }

#if 0
   DPRINT("[PnP Test] Trying to get aliases\n");

   for (SymbolicLinkListPtr = SymbolicLinkList;
        SymbolicLinkListPtr[0] != 0 && SymbolicLinkListPtr[1] != 0;
        SymbolicLinkListPtr += wcslen(SymbolicLinkListPtr) + 1)
   {
      UNICODE_STRING SymbolicLink;
      UNICODE_STRING AliasSymbolicLink;

      SymbolicLink.Buffer = SymbolicLinkListPtr;
      SymbolicLink.Length = SymbolicLink.MaximumLength = wcslen(SymbolicLinkListPtr);
      RtlInitUnicodeString(&AliasSymbolicLink, NULL);
      IoGetDeviceInterfaceAlias(
         &SymbolicLink,
         &AliasGuid,
         &AliasSymbolicLink);
      if (AliasSymbolicLink.Buffer != NULL)
      {
         DPRINT("[PnP Test] Original: %S\n", SymbolicLinkListPtr);
         DPRINT("[PnP Test] Alias: %S\n", AliasSymbolicLink.Buffer);
      }
   }
#endif

   ExFreePool(SymbolicLinkList);
}

static
VOID
Test_IoRegisterDeviceInterface(VOID)
{
    GUID Guid = {0x378de44c, 0x56ef, 0x11d1, {0xbc, 0x8c, 0x00, 0xa0, 0xc9, 0x14, 0x05, 0xdd}};
    DEVICE_OBJECT DeviceObject;
    EXTENDED_DEVOBJ_EXTENSION DeviceObjectExtension;
    DEVICE_NODE DeviceNode;
    UNICODE_STRING SymbolicLinkName;
    NTSTATUS Status;

    RtlInitUnicodeString(&SymbolicLinkName, L"");

    // Prepare our surrogate of a Device Object
    DeviceObject.DeviceObjectExtension = (PDEVOBJ_EXTENSION)&DeviceObjectExtension;

    // 1. DeviceNode = NULL
    DeviceObjectExtension.DeviceNode = NULL;
    Status = IoRegisterDeviceInterface(&DeviceObject, &Guid, NULL,
        &SymbolicLinkName);

    ok(Status == STATUS_INVALID_DEVICE_REQUEST,
        "IoRegisterDeviceInterface returned 0x%08lX\n", Status);

    // 2. DeviceNode->InstancePath is of a null length
    DeviceObjectExtension.DeviceNode = &DeviceNode;
    DeviceNode.InstancePath.Length = 0;
    Status = IoRegisterDeviceInterface(&DeviceObject, &Guid, NULL,
        &SymbolicLinkName);

    ok(Status == STATUS_INVALID_DEVICE_REQUEST,
        "IoRegisterDeviceInterface returned 0x%08lX\n", Status);

    DeviceInterfaceTest_Func();
}

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
    // FIXME: This test crashes in Windows
    (void)Test_IoRegisterDeviceInterface;
    Test_IoRegisterPlugPlayNotification();
    Test_IoSetDeviceInterface();
}

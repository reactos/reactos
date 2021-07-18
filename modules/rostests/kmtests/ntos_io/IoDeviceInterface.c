/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for Device Interface functions
 * COPYRIGHT:   Copyright 2011 Filip Navara <xnavara@volny.cz>
 *              Copyright 2021 Mark Jansen <mark.jansen@reactos.org>
 *              Copyright 2021 Oleg Dubinskiy <oleg.dubinskij2013@yandex.ua>
 */

/* TODO: Add IoRegisterDeviceInterface testcase */

#include <kmt_test.h>
#include <poclass.h>

#define NDEBUG
#include <debug.h>

/* Predefined GUIDs are required for IoGetDeviceInterfaceAlias and IoOpenDeviceInterfaceRegistryKey.
 * Only they can provide the aliases and the needed subkeys, unlike manually declared test GUIDs.
 * Since IoRegisterDeviceInterface testcase is missing, it is not possible to register the new device interface
 * and get an alias/key handle of it using this test. */
/* From https://docs.microsoft.com/en-us/windows-hardware/drivers/stream/ksproperty-topology-categories */
#define STATIC_KSCATEGORY_BRIDGE \
    0x085AFF00L, 0x62CE, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}
#define STATIC_KSCATEGORY_CAPTURE \
    0x65E8773DL, 0x8F56, 0x11D0, {0xA3, 0xB9, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}
#define STATIC_KSCATEGORY_COMMUNICATIONSTRANSFORM \
    0xCF1DDA2CL, 0x9743, 0x11D0, {0xA3, 0xEE, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}
#define STATIC_KSCATEGORY_DATACOMPRESSOR \
    0x1E84C900L, 0x7E70, 0x11D0, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}
#define STATIC_KSCATEGORY_DATADECOMPRESSOR \
    0x2721AE20L, 0x7E70, 0x11D0, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}
#define STATIC_KSCATEGORY_DATATRANSFORM \
    0x2EB07EA0L, 0x7E70, 0x11D0, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}
#define STATIC_KSCATEGORY_FILESYSTEM \
    0x760FED5EL, 0x9357, 0x11D0, {0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}
#define STATIC_KSCATEGORY_INTERFACETRANSFORM \
    0xCF1DDA2DL, 0x9743, 0x11D0, {0xA3, 0xEE, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}
#define STATIC_KSCATEGORY_MEDIUMTRANSFORM \
    0xCF1DDA2EL, 0x9743, 0x11D0, {0xA3, 0xEE, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}
#define STATIC_KSCATEGORY_MIXER \
    0xAD809C00L, 0x7B88, 0x11D0, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}
#define STATIC_KSCATEGORY_RENDER \
    0x65E8773EL, 0x8F56, 0x11D0, {0xA3, 0xB9, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}
#define STATIC_KSCATEGORY_SPLITTER \
    0x0A4252A0L, 0x7E70, 0x11D0, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}

static const GUID KSCATEGORY_BRIDGE = { STATIC_KSCATEGORY_BRIDGE };
static const GUID KSCATEGORY_CAPTURE = { STATIC_KSCATEGORY_CAPTURE };
static const GUID KSCATEGORY_COMMUNICATIONSTRANSFORM = { STATIC_KSCATEGORY_COMMUNICATIONSTRANSFORM };
static const GUID KSCATEGORY_DATACOMPRESSOR = { STATIC_KSCATEGORY_DATACOMPRESSOR };
static const GUID KSCATEGORY_DATADECOMPRESSOR = { STATIC_KSCATEGORY_DATADECOMPRESSOR };
static const GUID KSCATEGORY_DATATRANSFORM = { STATIC_KSCATEGORY_DATATRANSFORM };
static const GUID KSCATEGORY_FILESYSTEM = { STATIC_KSCATEGORY_FILESYSTEM };
static const GUID KSCATEGORY_INTERFACETRANSFORM = { STATIC_KSCATEGORY_INTERFACETRANSFORM };
static const GUID KSCATEGORY_MEDIUMTRANSFORM = { STATIC_KSCATEGORY_MEDIUMTRANSFORM };
static const GUID KSCATEGORY_MIXER = { STATIC_KSCATEGORY_MIXER };
static const GUID KSCATEGORY_RENDER = { STATIC_KSCATEGORY_RENDER };
static const GUID KSCATEGORY_SPLITTER = { STATIC_KSCATEGORY_SPLITTER };

static const GUID* Types[] =
{
    &KSCATEGORY_BRIDGE,
    &KSCATEGORY_CAPTURE,
    &KSCATEGORY_COMMUNICATIONSTRANSFORM,
    &KSCATEGORY_DATACOMPRESSOR,
    &KSCATEGORY_DATADECOMPRESSOR,
    &KSCATEGORY_DATATRANSFORM,
    &KSCATEGORY_FILESYSTEM,
    &KSCATEGORY_INTERFACETRANSFORM,
    &KSCATEGORY_MEDIUMTRANSFORM,
    &KSCATEGORY_MIXER,
    &KSCATEGORY_RENDER,
    &KSCATEGORY_SPLITTER,
};

static
VOID
Test_IoOpenDeviceInterfaceRegistryKey(PWSTR SymbolicLink)
{
    UNICODE_STRING KeyName, SymbolicLinkName;

    RtlInitUnicodeString(&SymbolicLinkName, SymbolicLink);
    RtlInitUnicodeString(&KeyName, L"ReactOS_kmtest");

    for (size_t n = 0; n < RTL_NUMBER_OF(Types); ++n)
    {
        HANDLE DeviceInterfaceKey, DeviceInterfaceSubKey;
        OBJECT_ATTRIBUTES ObjectAttributes;

        NTSTATUS Status = IoOpenDeviceInterfaceRegistryKey(&SymbolicLinkName, KEY_CREATE_SUB_KEY, &DeviceInterfaceKey);

        if (NT_SUCCESS(Status))
        {
            trace("IoOpenDeviceInterfaceRegistryKey(): success: %d %p\n", n, DeviceInterfaceKey);
        }
        else
        {
            trace("IoOpenDeviceInterfaceRegistryKey(): fail: %d 0x%x\n", n, Status);
        }

        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   DeviceInterfaceKey,
                                   NULL);
        Status = ZwCreateKey(&DeviceInterfaceSubKey,
                             KEY_WRITE,
                             &ObjectAttributes,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             NULL);

        if (NT_SUCCESS(Status))
        {
            trace("ZwCreateKey(): successfully created subkey: %d %p\n", n, DeviceInterfaceSubKey);

            ZwDeleteKey(DeviceInterfaceSubKey);
            ZwClose(DeviceInterfaceSubKey);
        }
        else
        {
            trace("ZwCreateKey(): failed to create a subkey: %d 0x%x\n", n, Status);
        }

        ZwClose(DeviceInterfaceKey);
    }
}

static
VOID
Test_IoGetDeviceInterfaceAlias(PWSTR SymbolicLink)
{
    UNICODE_STRING SymbolicLinkName;

    RtlInitUnicodeString(&SymbolicLinkName, SymbolicLink);

    for (size_t n = 0; n < RTL_NUMBER_OF(Types); ++n)
    {
        UNICODE_STRING AliasSymbolicLinkName;
        NTSTATUS Status = IoGetDeviceInterfaceAlias(&SymbolicLinkName, Types[n], &AliasSymbolicLinkName);

        if (NT_SUCCESS(Status))
        {
            trace("IoGetDeviceInterfaceAlias(): success: %d %wZ\n", n, &AliasSymbolicLinkName);

            RtlFreeUnicodeString(&AliasSymbolicLinkName);
        }
        else
        {
            trace("IoGetDeviceInterfaceAlias(): fail: %d 0x%x\n", n, Status);
        }
    }
}

static
VOID
Test_IoSetDeviceInterfaceState(PWSTR SymbolicLink)
{
    UNICODE_STRING SymbolicLinkName;

    RtlInitUnicodeString(&SymbolicLinkName, SymbolicLink);

    for (size_t n = 0; n < RTL_NUMBER_OF(Types); ++n)
    {
        NTSTATUS Status = IoSetDeviceInterfaceState(&SymbolicLinkName, TRUE);

        if (NT_SUCCESS(Status))
        {
            trace("IoSetDeviceInterfaceState(): successfully enabled interface: %d %wZ\n", n, &SymbolicLinkName);
        }
        else
        {
            trace("IoSetDeviceInterfaceState(): failed to enable interface: %d 0x%x\n", n, Status);
        }
    }
}

static
VOID
Test_IoGetDeviceInterfaces(const GUID* guid)
{
    NTSTATUS Status;
    PWSTR SymbolicLinkList, SymbolicLinkListPtr;
    UNICODE_STRING GuidString;

    RtlStringFromGUID(guid, &GuidString);

    Status = IoGetDeviceInterfaces(guid, NULL, DEVICE_INTERFACE_INCLUDE_NONACTIVE, &SymbolicLinkList);

    ok(NT_SUCCESS(Status), "IoGetDeviceInterfaces failed with status 0x%X for '%wZ'\n", Status, &GuidString);
    if (!NT_SUCCESS(Status))
    {
        return;
    }

    trace("IoGetDeviceInterfaces '%wZ' results:\n", &GuidString);
    RtlFreeUnicodeString(&GuidString);

    for (SymbolicLinkListPtr = SymbolicLinkList;
        SymbolicLinkListPtr[0] != 0 && SymbolicLinkListPtr[1] != 0;
        SymbolicLinkListPtr += wcslen(SymbolicLinkListPtr) + 1)
    {
        trace("Symbolic Link: %S\n", SymbolicLinkListPtr);
        Test_IoGetDeviceInterfaceAlias(SymbolicLinkListPtr);
        Test_IoOpenDeviceInterfaceRegistryKey(SymbolicLinkListPtr);
        Test_IoSetDeviceInterfaceState(SymbolicLinkListPtr);
    }

    ExFreePool(SymbolicLinkList);
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

START_TEST(IoDeviceInterface)
{
    for (size_t n = 0; n < RTL_NUMBER_OF(Types); ++n)
    {
        Test_IoGetDeviceInterfaces(Types[n]);
    }
    Test_IoRegisterPlugPlayNotification();
}

/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for Device Interface functions
 * COPYRIGHT:   Copyright 2011 Filip Navara <xnavara@volny.cz>
 *              Copyright 2011-2015 Thomas Faber <thomas.faber@reactos.org>
 *              Copyright 2021 Mark Jansen <mark.jansen@reactos.org>
 *              Copyright 2021-2024 Oleg Dubinskiy <oleg.dubinskiy@reactos.org>
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
/* Invalid GUID */
static const GUID GUID_NULL = {0x00000000L, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
/* From our ks.h */
static const GUID KSCATEGORY_BRIDGE = {0x085AFF00L, 0x62CE, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
static const GUID KSCATEGORY_CAPTURE = {0x65E8773DL, 0x8F56, 0x11D0, {0xA3, 0xB9, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
static const GUID KSCATEGORY_COMMUNICATIONSTRANSFORM = {0xCF1DDA2CL, 0x9743, 0x11D0, {0xA3, 0xEE, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
static const GUID KSCATEGORY_DATACOMPRESSOR = {0x1E84C900L, 0x7E70, 0x11D0, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
static const GUID KSCATEGORY_DATADECOMPRESSOR = {0x2721AE20L, 0x7E70, 0x11D0, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
static const GUID KSCATEGORY_DATATRANSFORM = {0x2EB07EA0L, 0x7E70, 0x11D0, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
static const GUID KSCATEGORY_FILESYSTEM = {0x760FED5EL, 0x9357, 0x11D0, {0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
static const GUID KSCATEGORY_INTERFACETRANSFORM = {0xCF1DDA2DL, 0x9743, 0x11D0, {0xA3, 0xEE, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
static const GUID KSCATEGORY_MEDIUMTRANSFORM = {0xCF1DDA2EL, 0x9743, 0x11D0, {0xA3, 0xEE, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
static const GUID KSCATEGORY_MIXER = {0xAD809C00L, 0x7B88, 0x11D0, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
static const GUID KSCATEGORY_RENDER = {0x65E8773EL, 0x8F56, 0x11D0, {0xA3, 0xB9, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
static const GUID KSCATEGORY_SPLITTER = {0x0A4252A0L, 0x7E70, 0x11D0, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};

static const GUID* Types[] =
{
    &GUID_NULL,
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
Test_IoOpenDeviceInterfaceRegistryKey(
    _In_opt_ PCWSTR SymbolicLink)
{
    UNICODE_STRING KeyName, SymbolicLinkName;
    HANDLE DeviceInterfaceKey;
    NTSTATUS Status;
    size_t n;

    RtlInitUnicodeString(&SymbolicLinkName, SymbolicLink);
    RtlInitUnicodeString(&KeyName, L"ReactOS_kmtest");

    /* It's okay to call this from a user process's thread */
    Status = IoOpenDeviceInterfaceRegistryKey(&SymbolicLinkName, KEY_CREATE_SUB_KEY, &DeviceInterfaceKey);

    if (skip(NT_SUCCESS(Status), "IoOpenDeviceInterfaceRegistryKey() failed: 0x%lx\n", Status))
        return;

    trace("IoOpenDeviceInterfaceRegistryKey() success: 0x%p\n", DeviceInterfaceKey);

    for (n = 0; n < RTL_NUMBER_OF(Types); ++n)
    {
        HANDLE DeviceInterfaceSubKey;
        OBJECT_ATTRIBUTES ObjectAttributes;

        /* Try to create the non-volatile subkey to check whether the parent key is volatile */
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

        if (skip(NT_SUCCESS(Status), "ZwCreateKey() failed to create a subkey: %d 0x%lx\n", n, Status))
            continue;

        trace("ZwCreateKey(): successfully created subkey: %d 0x%p\n", n, DeviceInterfaceSubKey);

        ZwDeleteKey(DeviceInterfaceSubKey);
        ZwClose(DeviceInterfaceSubKey);
    }

    ZwClose(DeviceInterfaceKey);
}

static
VOID
Test_IoGetDeviceInterfaceAlias(
    _In_opt_ PCWSTR SymbolicLink)
{
    UNICODE_STRING SymbolicLinkName;
    size_t n;

    RtlInitUnicodeString(&SymbolicLinkName, SymbolicLink);

    for (n = 0; n < RTL_NUMBER_OF(Types); ++n)
    {
        UNICODE_STRING AliasSymbolicLinkName;
        NTSTATUS Status = IoGetDeviceInterfaceAlias(&SymbolicLinkName, Types[n], &AliasSymbolicLinkName);

        if (skip(NT_SUCCESS(Status), "IoGetDeviceInterfaceAlias(): fail: %d 0x%x\n", n, Status))
            continue;

        trace("IoGetDeviceInterfaceAlias(): success: %d %wZ\n", n, &AliasSymbolicLinkName);

        /* Test IoOpenDeviceInterfaceRegistryKey with alias symbolic link too */
        Test_IoOpenDeviceInterfaceRegistryKey(AliasSymbolicLinkName.Buffer);

        RtlFreeUnicodeString(&AliasSymbolicLinkName);
    }
}

static
VOID
Test_IoSetDeviceInterfaceState(
    _In_opt_ PCWSTR SymbolicLink)
{
    UNICODE_STRING SymbolicLinkName;
    size_t n;

    RtlInitUnicodeString(&SymbolicLinkName, SymbolicLink);

    for (n = 0; n < RTL_NUMBER_OF(Types); ++n)
    {
        NTSTATUS Status = IoSetDeviceInterfaceState(&SymbolicLinkName, TRUE);

        if (skip(NT_SUCCESS(Status), "IoSetDeviceInterfaceState(): failed to enable interface: %d 0x%x\n", n, Status))
            continue;

        trace("IoSetDeviceInterfaceState(): successfully enabled interface: %d %wZ\n", n, &SymbolicLinkName);
    }
}

static
VOID
Test_IoGetDeviceInterfaces(
    _In_ const GUID* Guid)
{
    NTSTATUS Status;
    PZZWSTR SymbolicLinkList;
    PWSTR SymbolicLink;
    UNICODE_STRING GuidString;

    Status = IoGetDeviceInterfaces(Guid, NULL, DEVICE_INTERFACE_INCLUDE_NONACTIVE, &SymbolicLinkList);

    RtlStringFromGUID(Guid, &GuidString);
    if (skip(NT_SUCCESS(Status), "IoGetDeviceInterfaces failed with status 0x%x for '%wZ'\n", Status, &GuidString))
    {
        RtlFreeUnicodeString(&GuidString);
        return;
    }

    trace("IoGetDeviceInterfaces '%wZ' results:\n", &GuidString);
    RtlFreeUnicodeString(&GuidString);

    for (SymbolicLink = SymbolicLinkList;
         SymbolicLink[0] != UNICODE_NULL;
         SymbolicLink += wcslen(SymbolicLink) + 1)
    {
        trace("Symbolic Link: %S\n", SymbolicLink);
        Test_IoGetDeviceInterfaceAlias(SymbolicLink);
        Test_IoOpenDeviceInterfaceRegistryKey(SymbolicLink);
        Test_IoSetDeviceInterfaceState(SymbolicLink);
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
    size_t n;
    for (n = 0; n < RTL_NUMBER_OF(Types); ++n)
    {
        Test_IoGetDeviceInterfaces(Types[n]);
    }
    /* Test the invalid case behaviour */
    Test_IoGetDeviceInterfaceAlias(NULL);
    Test_IoOpenDeviceInterfaceRegistryKey(NULL);
    Test_IoSetDeviceInterfaceState(NULL);
    Test_IoRegisterPlugPlayNotification();
}

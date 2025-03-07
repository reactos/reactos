/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/iomgr/deviface.c
 * PURPOSE:         Device interface functions
 *
 * PROGRAMMERS:     Filip Navara (xnavara@volny.cz)
 *                  Matthew Brace (ismarc@austin.rr.com)
 *                  Hervé Poussineau (hpoussin@reactos.org)
 *                  Oleg Dubinskiy (oleg.dubinskiy@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <debug.h>

/* FIXME: This should be somewhere global instead of having 20 different versions */
#define GUID_STRING_CHARS 38
#define GUID_STRING_BYTES (GUID_STRING_CHARS * sizeof(WCHAR))
C_ASSERT(sizeof(L"{01234567-89ab-cdef-0123-456789abcdef}") == GUID_STRING_BYTES + sizeof(UNICODE_NULL));

/* FUNCTIONS *****************************************************************/

PDEVICE_OBJECT
IopGetDeviceObjectFromDeviceInstance(PUNICODE_STRING DeviceInstance);

static PCWSTR BaseKeyString = L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\DeviceClasses\\";

/**
 * @brief
 * Creates a new symbolic link from the specified format of the prefix, device string,
 * class GUID and reference string (if any).
 *
 * @param[in] DeviceString
 * Device string, placed after prefix and before GUID, for example ACPI#PNP0501#1#.
 *
 * @param[in] GuidString
 * Device interface class GUID represented by a string. Placed in curly brackets {},
 * after device string, should always be 38 characters long. For example,
 * {01234567-89ab-cdef-0123-456789abcdef}.
 *
 * @param[in] ReferenceString
 * Optional reference string, if any. Placed after GUID, at the end of symbolic link.
 * Usually contains human-readable subdevice name or class GUID.
 *
 * @param[in] UserModePrefixFormat
 * Specifies whether a new symbolic link should have either a kernel mode or user mode prefix.
 * TRUE for user mode prefix, FALSE for kernel mode.
 *
 * @param[out] SymbolicLinkName
 * Pointer to unicode string which receives created symbolic link.
 *
 * @return
 * STATUS_SUCCESS in case of success, or an NTSTATUS error code otherwise.
 **/
static
NTSTATUS
IopBuildSymbolicLink(
    _In_ PCUNICODE_STRING DeviceString,
    _In_ PCUNICODE_STRING GuidString,
    _In_opt_ PCUNICODE_STRING ReferenceString,
    _In_ BOOLEAN UserModePrefixFormat,
    _Out_ PUNICODE_STRING SymbolicLinkName)
{
    static const UNICODE_STRING KernelModePrefix = RTL_CONSTANT_STRING(L"\\??\\");
    static const UNICODE_STRING UserModePrefix = RTL_CONSTANT_STRING(L"\\\\?\\");
    static const UNICODE_STRING PathSep = RTL_CONSTANT_STRING(L"\\");
    UNICODE_STRING MungedDeviceString, SymbolicLink;
    NTSTATUS Status;
    ULONG Length;
    USHORT i;

    /* Use a backslash if reference string is not specified */
    if (!ReferenceString)
        ReferenceString = &PathSep;

    /* Duplicate the device string (to "munge" it) */
    Status = RtlDuplicateUnicodeString(0, DeviceString, &MungedDeviceString);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlDuplicateUnicodeString() failed, Status 0x%08lx\n", Status);
        return Status;
    }

    /* Replace all '\' by '#' in device string */
    for (i = 0; i < MungedDeviceString.Length / sizeof(WCHAR); i++)
    {
        if (MungedDeviceString.Buffer[i] == L'\\')
            MungedDeviceString.Buffer[i] = L'#';
    }

    /* Calculate total length */
    Length = KernelModePrefix.Length // Same as UserModePrefix.Length
           + MungedDeviceString.Length
           + sizeof(L"#") + GuidString->Length
           + ReferenceString->Length;
    ASSERT(Length <= MAXUSHORT);

    /* Build up new symbolic link */
    SymbolicLink.Length = 0;
    SymbolicLink.MaximumLength = Length;
    SymbolicLink.Buffer = ExAllocatePoolWithTag(PagedPool, SymbolicLink.MaximumLength, TAG_IO);
    if (!SymbolicLink.Buffer)
    {
        DPRINT1("ExAllocatePoolWithTag() failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = RtlUnicodeStringPrintf(&SymbolicLink,
                                    L"%wZ%wZ#%wZ%wZ",
                                    UserModePrefixFormat ?
                                    &UserModePrefix : &KernelModePrefix,
                                    &MungedDeviceString,
                                    GuidString,
                                    ReferenceString);
    NT_VERIFY(NT_SUCCESS(Status));

    DPRINT("New symbolic link is %wZ\n", &SymbolicLink);

    *SymbolicLinkName = SymbolicLink;
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Parses the specified symbolic link onto the 4 parts: prefix, device string,
 * class GUID and reference string.
 *
 * @param[in] SymbolicLinkName
 * Pointer to a symbolic link string to parse.
 *
 * @param[out] PrefixString
 * Receives prefix of symbolic link. Can be '\??\' for Kernel mode or '\\?\' for User mode.
 *
 * @param[out] MungedString
 * Receives device string. For example, ##?#ACPI#PNP0501#1#.
 *
 * @param[out] GuidString
 * Receives device interface class GUID string represented by device interface.
 * For example, {01234567-89ab-cdef-0123-456789abcdef}.
 *
 * @param[out] ReferenceString
 * Receives reference string, if any. Usually contains a human-readable
 * subdevice name or class GUID.
 *
 * @param[out] ReferenceStringPresent
 * Pointer to variable that indicates whether the reference string exists in symbolic link.
 * TRUE if it does, FALSE otherwise.
 *
 * @param[out] InterfaceClassGuid
 * Receives the interface class GUID to which specified symbolic link belongs to.
 *
 * @return
 * STATUS_SUCCESS in case of success, or an NTSTATUS error code otherwise.
 **/
static
NTSTATUS
IopSeparateSymbolicLink(
    _In_ PCUNICODE_STRING SymbolicLinkName,
    _Out_opt_ PUNICODE_STRING PrefixString,
    _Out_opt_ PUNICODE_STRING MungedString,
    _Out_opt_ PUNICODE_STRING GuidString,
    _Out_opt_ PUNICODE_STRING ReferenceString,
    _Out_opt_ PBOOLEAN ReferenceStringPresent,
    _Out_opt_ LPGUID InterfaceClassGuid)
{
    static const UNICODE_STRING KernelModePrefix = RTL_CONSTANT_STRING(L"\\??\\");
    static const UNICODE_STRING UserModePrefix = RTL_CONSTANT_STRING(L"\\\\?\\");
    UNICODE_STRING MungedStringReal, GuidStringReal, ReferenceStringReal;
    UNICODE_STRING LinkNameNoPrefix;
    USHORT i, ReferenceStringOffset;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("Symbolic link is %wZ\n", SymbolicLinkName);

    /* The symbolic link name looks like \??\ACPI#PNP0501#1#{GUID}\ReferenceString
     * Make sure it starts with the expected prefix. */
    if (!RtlPrefixUnicodeString(&KernelModePrefix, SymbolicLinkName, FALSE) &&
        !RtlPrefixUnicodeString(&UserModePrefix, SymbolicLinkName, FALSE))
    {
        DPRINT1("Invalid link name %wZ\n", SymbolicLinkName);
        return STATUS_INVALID_PARAMETER;
    }

    /* Sanity checks */
    ASSERT(KernelModePrefix.Length == UserModePrefix.Length);
    ASSERT(SymbolicLinkName->Length >= KernelModePrefix.Length);

    /* Make a version without the prefix for further processing */
    LinkNameNoPrefix.Buffer = SymbolicLinkName->Buffer + KernelModePrefix.Length / sizeof(WCHAR);
    LinkNameNoPrefix.Length = SymbolicLinkName->Length - KernelModePrefix.Length;
    LinkNameNoPrefix.MaximumLength = LinkNameNoPrefix.Length;

    DPRINT("Symbolic link without prefix is %wZ\n", &LinkNameNoPrefix);

    /* Find the reference string, if any */
    for (i = 0; i < LinkNameNoPrefix.Length / sizeof(WCHAR); i++)
    {
        if (LinkNameNoPrefix.Buffer[i] == L'\\')
            break;
    }
    ReferenceStringOffset = i * sizeof(WCHAR);

    /* The GUID is before the reference string or at the end */
    ASSERT(LinkNameNoPrefix.Length >= ReferenceStringOffset);
    if (ReferenceStringOffset < GUID_STRING_BYTES + sizeof(WCHAR))
    {
        DPRINT1("Invalid link name %wZ\n", SymbolicLinkName);
        return STATUS_INVALID_PARAMETER;
    }

    /* Get reference string (starts with \ after {GUID}) from link without prefix */
    ReferenceStringReal.Buffer = LinkNameNoPrefix.Buffer + ReferenceStringOffset / sizeof(WCHAR);
    ReferenceStringReal.Length = LinkNameNoPrefix.Length - ReferenceStringOffset;
    ReferenceStringReal.MaximumLength = ReferenceStringReal.Length;

    DPRINT("Reference string is %wZ\n", &ReferenceStringReal);

    /* Get GUID string (device class GUID in {} brackets) */
    GuidStringReal.Buffer = LinkNameNoPrefix.Buffer + (ReferenceStringOffset - GUID_STRING_BYTES) / sizeof(WCHAR);
    GuidStringReal.Length = GUID_STRING_BYTES;
    GuidStringReal.MaximumLength = GuidStringReal.Length;

    DPRINT("GUID string is %wZ\n", &GuidStringReal);

    /* Validate GUID string for:
     * 1) {} brackets at the start and the end;
     * 2) - separators in the appropriate places. */
    ASSERT(GuidStringReal.Buffer[0] == L'{');
    ASSERT(GuidStringReal.Buffer[GUID_STRING_CHARS - 1] == L'}');
    ASSERT(GuidStringReal.Buffer[9] == L'-');
    ASSERT(GuidStringReal.Buffer[14] == L'-');
    ASSERT(GuidStringReal.Buffer[19] == L'-');
    ASSERT(GuidStringReal.Buffer[24] == L'-');

    if (MungedString)
    {
        /* Create a munged path string (looks like ACPI#PNP0501#1#) */
        MungedStringReal.Buffer = LinkNameNoPrefix.Buffer;
        MungedStringReal.Length = LinkNameNoPrefix.Length - ReferenceStringReal.Length - GUID_STRING_BYTES - sizeof(WCHAR);
        MungedStringReal.MaximumLength = MungedStringReal.Length;

        DPRINT("Munged string is %wZ\n", &MungedStringReal);
    }

    /* Store received parts if the parameters are not null */
    if (PrefixString)
    {
        PrefixString->Buffer = SymbolicLinkName->Buffer;
        PrefixString->Length = KernelModePrefix.Length; // Same as UserModePrefix.Length
        PrefixString->MaximumLength = PrefixString->Length;

        DPRINT("Prefix string is %wZ\n", PrefixString);
    }

    if (MungedString)
        *MungedString = MungedStringReal;

    if (GuidString)
        *GuidString = GuidStringReal;

    if (ReferenceString)
    {
        if (ReferenceStringReal.Length > sizeof(WCHAR))
            *ReferenceString = ReferenceStringReal;
        else
            RtlInitEmptyUnicodeString(ReferenceString, NULL, 0);
    }

    if (ReferenceStringPresent)
        *ReferenceStringPresent = ReferenceStringReal.Length > sizeof(WCHAR);

    if (InterfaceClassGuid)
    {
        /* Convert GUID string into a GUID and store it also */
        Status = RtlGUIDFromString(&GuidStringReal, InterfaceClassGuid);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("RtlGUIDFromString() failed, Status 0x%08lx\n", Status);
        }
    }

    /* We're done */
    return Status;
}

/**
 * @brief
 * Retrieves a handles to the device and instance registry keys
 * for the previously opened registry key handle of the specified symbolic link.
 **/
static
NTSTATUS
IopOpenOrCreateSymbolicLinkSubKeys(
    _Out_opt_ PHANDLE DeviceHandle,
    _Out_opt_ PULONG DeviceDisposition,
    _Out_opt_ PHANDLE InstanceHandle,
    _Out_opt_ PULONG InstanceDisposition,
    _In_ HANDLE ClassHandle,
    _In_ PCUNICODE_STRING SymbolicLinkName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ BOOLEAN Create)
{
    UNICODE_STRING ReferenceString = {0};
    UNICODE_STRING SymbolicLink = {0};
    HANDLE DeviceKeyHandle, InstanceKeyHandle;
    ULONG DeviceKeyDisposition, InstanceKeyDisposition;
    BOOLEAN ReferenceStringPresent = FALSE; /* Assuming no ref string by default */
    NTSTATUS Status;
    USHORT i;

    DeviceKeyHandle = InstanceKeyHandle = NULL;

    /* Duplicate the symbolic link (we'll modify it later) */
    Status = RtlDuplicateUnicodeString(0, SymbolicLinkName, &SymbolicLink);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlDuplicateUnicodeString() failed, Status 0x%08lx\n", Status);
        goto Quit;
    }

    /* Separate it into its constituents */
    Status = IopSeparateSymbolicLink(&SymbolicLink,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &ReferenceString,
                                     &ReferenceStringPresent,
                                     NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to separate symbolic link %wZ, Status 0x%08lx\n", &SymbolicLink, Status);
        goto Quit;
    }

    /* Did we got a ref string? */
    if (ReferenceStringPresent)
    {
        /* Remove it from our symbolic link */
        SymbolicLink.MaximumLength = SymbolicLink.Length -= ReferenceString.Length;

        /* Replace the 1st backslash `\` character by '#' pound */
        ReferenceString.Buffer[0] = L'#';
    }
    else
    {
        /* No ref string, initialize it with a single pound character '#' */
        RtlInitUnicodeString(&ReferenceString, L"#");
    }

    /* Replace all '\' by '#' in symbolic link */
    for (i = 0; i < SymbolicLink.Length / sizeof(WCHAR); i++)
    {
        if (SymbolicLink.Buffer[i] == L'\\')
            SymbolicLink.Buffer[i] = L'#';
    }

    /* Fix prefix: '#??#' -> '##?#' */
    SymbolicLink.Buffer[1] = L'#';

    DPRINT("Munged symbolic link is %wZ\n", &SymbolicLink);

    /* Try to open or create device interface keys */
    if (Create)
    {
        Status = IopCreateRegistryKeyEx(&DeviceKeyHandle,
                                        ClassHandle,
                                        &SymbolicLink,
                                        DesiredAccess | KEY_ENUMERATE_SUB_KEYS,
                                        REG_OPTION_NON_VOLATILE,
                                        &DeviceKeyDisposition);
    }
    else
    {
        Status = IopOpenRegistryKeyEx(&DeviceKeyHandle,
                                      ClassHandle,
                                      &SymbolicLink,
                                      DesiredAccess | KEY_ENUMERATE_SUB_KEYS);
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create or open %wZ, Status 0x%08lx\n", &SymbolicLink, Status);
        goto Quit;
    }

    DPRINT("Munged reference string is %wZ\n", &ReferenceString);

    /* Try to open or create instance subkeys */
    if (Create)
    {
        Status = IopCreateRegistryKeyEx(&InstanceKeyHandle,
                                        DeviceKeyHandle,
                                        &ReferenceString,
                                        DesiredAccess,
                                        REG_OPTION_NON_VOLATILE,
                                        &InstanceKeyDisposition);
    }
    else
    {
        Status = IopOpenRegistryKeyEx(&InstanceKeyHandle,
                                      DeviceKeyHandle,
                                      &ReferenceString,
                                      DesiredAccess);
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create or open %wZ, Status 0x%08lx\n", &ReferenceString, Status);
        goto Quit;
    }

    Status = STATUS_SUCCESS;

Quit:
    if (NT_SUCCESS(Status))
    {
        if (DeviceHandle)
            *DeviceHandle = DeviceKeyHandle;
        else
            ZwClose(DeviceKeyHandle);

        if (DeviceDisposition)
            *DeviceDisposition = DeviceKeyDisposition;

        if (InstanceHandle)
            *InstanceHandle = InstanceKeyHandle;
        else
            ZwClose(InstanceKeyHandle);

        if (InstanceDisposition)
            *InstanceDisposition = InstanceKeyDisposition;
    }
    else
    {
        if (InstanceKeyHandle)
            ZwClose(InstanceKeyHandle);

        if (Create)
            ZwDeleteKey(DeviceKeyHandle);

        if (DeviceKeyHandle)
            ZwClose(DeviceKeyHandle);
    }

    if (SymbolicLink.Buffer)
        RtlFreeUnicodeString(&SymbolicLink);

    return Status;
}

/**
 * @brief
 * Retrieves a handles to the GUID, device and instance registry keys
 * for the specified symbolic link.
 **/
static
NTSTATUS
OpenRegistryHandlesFromSymbolicLink(
    _In_ PCUNICODE_STRING SymbolicLinkName,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_opt_ PHANDLE GuidKey,
    _Out_opt_ PHANDLE DeviceKey,
    _Out_opt_ PHANDLE InstanceKey)
{
    UNICODE_STRING BaseKeyU;
    UNICODE_STRING GuidString;
    HANDLE ClassesKey;
    HANDLE GuidKeyReal, DeviceKeyReal, InstanceKeyReal;
    NTSTATUS Status;

    ClassesKey = GuidKeyReal = DeviceKeyReal = InstanceKeyReal = NULL;

    RtlInitUnicodeString(&BaseKeyU, BaseKeyString);

    /* Separate symbolic link onto the parts */
    Status = IopSeparateSymbolicLink(SymbolicLinkName,
                                     NULL,
                                     NULL,
                                     &GuidString,
                                     NULL,
                                     NULL,
                                     NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to parse symbolic link %wZ, Status 0x%08lx\n",
                SymbolicLinkName, Status);
        goto Quit;
    }

    /* Open the DeviceClasses key */
    Status = IopOpenRegistryKeyEx(&ClassesKey,
                                  NULL,
                                  &BaseKeyU,
                                  DesiredAccess | KEY_ENUMERATE_SUB_KEYS);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open %wZ, Status 0x%08lx\n", &BaseKeyU, Status);
        goto Quit;
    }

    /* Open the GUID subkey */
    Status = IopOpenRegistryKeyEx(&GuidKeyReal,
                                  ClassesKey,
                                  &GuidString,
                                  DesiredAccess | KEY_ENUMERATE_SUB_KEYS);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open %wZ%wZ, Status 0x%08lx\n", &BaseKeyU, &GuidString, Status);
        goto Quit;
    }

    /* Open the device and instance subkeys */
    Status = IopOpenOrCreateSymbolicLinkSubKeys(&DeviceKeyReal,
                                                NULL,
                                                &InstanceKeyReal,
                                                NULL,
                                                GuidKeyReal,
                                                SymbolicLinkName,
                                                DesiredAccess,
                                                FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open %wZ%wZ, Status 0x%08lx\n", &BaseKeyU, &GuidString, Status);
        goto Quit;
    }

    Status = STATUS_SUCCESS;

Quit:
    if (NT_SUCCESS(Status))
    {
        if (GuidKey)
            *GuidKey = GuidKeyReal;
        else
            ZwClose(GuidKeyReal);

        if (DeviceKey)
            *DeviceKey = DeviceKeyReal;
        else
            ZwClose(DeviceKeyReal);

        if (InstanceKey)
            *InstanceKey = InstanceKeyReal;
        else
            ZwClose(InstanceKeyReal);
    }
    else
    {
        if (GuidKeyReal)
            ZwClose(GuidKeyReal);

        if (DeviceKeyReal)
            ZwClose(DeviceKeyReal);

        if (InstanceKeyReal)
            ZwClose(InstanceKeyReal);
    }

    if (ClassesKey)
        ZwClose(ClassesKey);

    return Status;
}

/*++
 * @name IoOpenDeviceInterfaceRegistryKey
 * @unimplemented
 *
 * Provides a handle to the device's interface instance registry key.
 * Documented in WDK.
 *
 * @param SymbolicLinkName
 *        Pointer to a string which identifies the device interface instance
 *
 * @param DesiredAccess
 *        Desired ACCESS_MASK used to access the key (like KEY_READ,
 *        KEY_WRITE, etc)
 *
 * @param DeviceInterfaceKey
 *        If a call has been succesfull, a handle to the registry key
 *        will be stored there
 *
 * @return Three different NTSTATUS values in case of errors, and STATUS_SUCCESS
 *         otherwise (see WDK for details)
 *
 * @remarks Must be called at IRQL = PASSIVE_LEVEL in the context of a system thread
 *
 *--*/
NTSTATUS
NTAPI
IoOpenDeviceInterfaceRegistryKey(IN PUNICODE_STRING SymbolicLinkName,
                                 IN ACCESS_MASK DesiredAccess,
                                 OUT PHANDLE DeviceInterfaceKey)
{
    HANDLE InstanceKey, DeviceParametersKey;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DeviceParametersU = RTL_CONSTANT_STRING(L"Device Parameters");

    Status = OpenRegistryHandlesFromSymbolicLink(SymbolicLinkName,
                                                 KEY_CREATE_SUB_KEY,
                                                 NULL,
                                                 NULL,
                                                 &InstanceKey);
    if (!NT_SUCCESS(Status))
        return Status;

    InitializeObjectAttributes(&ObjectAttributes,
                               &DeviceParametersU,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE | OBJ_OPENIF,
                               InstanceKey,
                               NULL);
    Status = ZwCreateKey(&DeviceParametersKey,
                         DesiredAccess,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         NULL);
    ZwClose(InstanceKey);

    if (NT_SUCCESS(Status))
        *DeviceInterfaceKey = DeviceParametersKey;

    return Status;
}

/*++
 * @name IopOpenInterfaceKey
 *
 * Returns the alias device interface of the specified device interface
 *
 * @param InterfaceClassGuid
 *        FILLME
 *
 * @param DesiredAccess
 *        FILLME
 *
 * @param pInterfaceKey
 *        FILLME
 *
 * @return Usual NTSTATUS
 *
 * @remarks None
 *
 *--*/
static NTSTATUS
IopOpenInterfaceKey(IN CONST GUID *InterfaceClassGuid,
                    IN ACCESS_MASK DesiredAccess,
                    OUT HANDLE *pInterfaceKey)
{
    UNICODE_STRING LocalMachine = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\");
    UNICODE_STRING GuidString;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE InterfaceKey = NULL;
    NTSTATUS Status;

    GuidString.Buffer = KeyName.Buffer = NULL;

    Status = RtlStringFromGUID(InterfaceClassGuid, &GuidString);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("RtlStringFromGUID() failed with status 0x%08lx\n", Status);
        goto cleanup;
    }

    KeyName.Length = 0;
    KeyName.MaximumLength = LocalMachine.Length + ((USHORT)wcslen(REGSTR_PATH_DEVICE_CLASSES) + 1) * sizeof(WCHAR) + GuidString.Length;
    KeyName.Buffer = ExAllocatePool(PagedPool, KeyName.MaximumLength);
    if (!KeyName.Buffer)
    {
        DPRINT("ExAllocatePool() failed\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    Status = RtlAppendUnicodeStringToString(&KeyName, &LocalMachine);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("RtlAppendUnicodeStringToString() failed with status 0x%08lx\n", Status);
        goto cleanup;
    }
    Status = RtlAppendUnicodeToString(&KeyName, REGSTR_PATH_DEVICE_CLASSES);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("RtlAppendUnicodeToString() failed with status 0x%08lx\n", Status);
        goto cleanup;
    }
    Status = RtlAppendUnicodeToString(&KeyName, L"\\");
    if (!NT_SUCCESS(Status))
    {
        DPRINT("RtlAppendUnicodeToString() failed with status 0x%08lx\n", Status);
        goto cleanup;
    }
    Status = RtlAppendUnicodeStringToString(&KeyName, &GuidString);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("RtlAppendUnicodeStringToString() failed with status 0x%08lx\n", Status);
        goto cleanup;
    }

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL);
    Status = ZwOpenKey(
        &InterfaceKey,
        DesiredAccess,
        &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwOpenKey() failed with status 0x%08lx\n", Status);
        goto cleanup;
    }

    *pInterfaceKey = InterfaceKey;
    Status = STATUS_SUCCESS;

cleanup:
    if (!NT_SUCCESS(Status))
    {
        if (InterfaceKey != NULL)
            ZwClose(InterfaceKey);
    }
    RtlFreeUnicodeString(&GuidString);
    RtlFreeUnicodeString(&KeyName);
    return Status;
}

/**
 * @brief
 * Returns the alias device interface of the specified device interface
 * instance, if the alias exists.
 *
 * @param[in] SymbolicLinkName
 * Pointer to a symbolic link string which identifies the device interface instance.
 *
 * @param[in] AliasInterfaceClassGuid
 * Pointer to a device interface class GUID.
 *
 * @param[out] AliasSymbolicLinkName
 * Pointer to unicode string which receives the alias symbolic link upon success.
 * Must be freed with RtlFreeUnicodeString after using.
 *
 * @return NTSTATUS values in case of errors, STATUS_SUCCESS otherwise.
 *
 * @remarks Must be called at IRQL = PASSIVE_LEVEL in the context of a system thread
 **/
NTSTATUS
NTAPI
IoGetDeviceInterfaceAlias(
    _In_ PUNICODE_STRING SymbolicLinkName,
    _In_ CONST GUID *AliasInterfaceClassGuid,
    _Out_ PUNICODE_STRING AliasSymbolicLinkName)
{
    static const UNICODE_STRING UserModePrefix = RTL_CONSTANT_STRING(L"\\\\?\\");
    UNICODE_STRING AliasSymbolicLink = {0};
    UNICODE_STRING AliasGuidString = {0};
    UNICODE_STRING DeviceString = {0};
    UNICODE_STRING ReferenceString = {0};
    PKEY_VALUE_FULL_INFORMATION kvInfo;
    HANDLE DeviceKey, AliasInstanceKey;
    BOOLEAN UserModePrefixFormat;
    BOOLEAN ReferenceStringPresent = FALSE; /* Assuming no ref string by default */
    PVOID Buffer;
    NTSTATUS Status;

    DPRINT("IoGetDeviceInterfaceAlias(%wZ, 0x%p)\n", SymbolicLinkName, AliasInterfaceClassGuid);

    /* Sanity check */
    if (!SymbolicLinkName || !AliasInterfaceClassGuid)
    {
        DPRINT1("IoGetDeviceInterfaceAlias() invalid symbolic link or alias class GUID\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Convert alias GUID to a string */
    Status = RtlStringFromGUID(AliasInterfaceClassGuid, &AliasGuidString);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlStringFromGUID() failed, Status 0x%08lx\n", Status);
        goto Quit;
    }

    DPRINT("Alias GUID is %wZ\n", &AliasGuidString);

    /* Get the device instance string of existing symbolic link */
    Status = OpenRegistryHandlesFromSymbolicLink(SymbolicLinkName,
                                                 KEY_QUERY_VALUE,
                                                 NULL,
                                                 &DeviceKey,
                                                 NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open device instance key for %wZ, Status 0x%08lx\n", SymbolicLinkName, Status);
        goto Quit;
    }

    Status = IopGetRegistryValue(DeviceKey, L"DeviceInstance", &kvInfo);
    ZwClose(DeviceKey);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed get device instance value, Status 0x%08lx\n", Status);
        goto Quit;
    }

    if (kvInfo->Type != REG_SZ || kvInfo->DataLength == 0 || kvInfo->DataLength > MAXUSHORT)
    {
        DPRINT1("Wrong or empty instance value\n");
        Status = STATUS_INVALID_PARAMETER;
        goto Quit;
    }

    /* Convert received data to unicode string */
    Buffer = (PVOID)((ULONG_PTR)kvInfo + kvInfo->DataOffset);
    PnpRegSzToString(Buffer, kvInfo->DataLength, &DeviceString.Length);
    DeviceString.MaximumLength = DeviceString.Length;
    DeviceString.Buffer = Buffer;

    /* 
     * Separate symbolic link into 4 parts:
     * 1) prefix string (\??\ for kernel mode or \\?\ for user mode),
     * 2) munged path string (like ##?#ACPI#PNP0501#1#{GUID}),
     * 3) GUID string (the current GUID),
     * 4) reference string (goes after GUID, starts with '\').
     * 
     * We need only reference string.
     */
    Status = IopSeparateSymbolicLink(SymbolicLinkName,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &ReferenceString,
                                     &ReferenceStringPresent,
                                     NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to separate symbolic link %wZ, Status 0x%08lx\n", SymbolicLinkName, Status);
        goto Quit;
    }

    DPRINT("Device string is '%wZ'\n", &DeviceString);

    /* Does symbolic link have kernel mode "\??\" or user mode "\\?\" prefix format? */
    UserModePrefixFormat = RtlPrefixUnicodeString(&UserModePrefix, SymbolicLinkName, FALSE);

    /* Build up new symbolic link with alias GUID */
    Status = IopBuildSymbolicLink(&DeviceString,
                                  &AliasGuidString,
                                  ReferenceStringPresent ? &ReferenceString : NULL,
                                  UserModePrefixFormat,
                                  &AliasSymbolicLink);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to build alias symbolic link, Status 0x%08lx\n", Status);
        goto Quit;
    }

    /* Make sure that alias symbolic link key exists in registry */
    Status = OpenRegistryHandlesFromSymbolicLink(&AliasSymbolicLink,
                                                 KEY_READ,
                                                 NULL,
                                                 NULL,
                                                 &AliasInstanceKey);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open alias symbolic link key, Status 0x%08lx\n", Status);
        goto Quit;
    }
    ZwClose(AliasInstanceKey);

    /* We're done */
    DPRINT("IoGetDeviceInterfaceAlias(): alias symbolic link %wZ\n", &AliasSymbolicLink);
    *AliasSymbolicLinkName = AliasSymbolicLink;
    Status = STATUS_SUCCESS;

Quit:
    if (!NT_SUCCESS(Status))
    {
        if (AliasSymbolicLink.Buffer)
            RtlFreeUnicodeString(&AliasSymbolicLink);
    }

    if (AliasGuidString.Buffer)
        RtlFreeUnicodeString(&AliasGuidString);

    return Status;
}

/*++
 * @name IoGetDeviceInterfaces
 * @implemented
 *
 * Returns a list of device interfaces of a particular device interface class.
 * Documented in WDK
 *
 * @param InterfaceClassGuid
 *        Points to a class GUID specifying the device interface class
 *
 * @param PhysicalDeviceObject
 *        Points to an optional PDO that narrows the search to only the
 *        device interfaces of the device represented by the PDO
 *
 * @param Flags
 *        Specifies flags that modify the search for device interfaces. The
 *        DEVICE_INTERFACE_INCLUDE_NONACTIVE flag specifies that the list of
 *        returned symbolic links should contain also disabled device
 *        interfaces in addition to the enabled ones.
 *
 * @param SymbolicLinkList
 *        Points to a character pointer that is filled in on successful return
 *        with a list of unicode strings identifying the device interfaces
 *        that match the search criteria. The newly allocated buffer contains
 *        a list of symbolic link names. Each unicode string in the list is
 *        null-terminated; the end of the whole list is marked by an additional
 *        NULL. The caller is responsible for freeing the buffer (ExFreePool)
 *        when it is no longer needed.
 *        If no device interfaces match the search criteria, this routine
 *        returns STATUS_SUCCESS and the string contains a single NULL
 *        character.
 *
 * @return Usual NTSTATUS
 *
 * @remarks None
 *
 *--*/
NTSTATUS
NTAPI
IoGetDeviceInterfaces(IN CONST GUID *InterfaceClassGuid,
                      IN PDEVICE_OBJECT PhysicalDeviceObject OPTIONAL,
                      IN ULONG Flags,
                      OUT PWSTR *SymbolicLinkList)
{
    UNICODE_STRING Control = RTL_CONSTANT_STRING(L"Control");
    UNICODE_STRING SymbolicLink = RTL_CONSTANT_STRING(L"SymbolicLink");
    HANDLE InterfaceKey = NULL;
    HANDLE DeviceKey = NULL;
    HANDLE ReferenceKey = NULL;
    HANDLE ControlKey = NULL;
    PKEY_BASIC_INFORMATION DeviceBi = NULL;
    PKEY_BASIC_INFORMATION ReferenceBi = NULL;
    PKEY_VALUE_PARTIAL_INFORMATION bip = NULL;
    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo;
    PEXTENDED_DEVOBJ_EXTENSION DeviceObjectExtension;
    PUNICODE_STRING InstanceDevicePath = NULL;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    BOOLEAN FoundRightPDO = FALSE;
    ULONG i = 0, j, Size, NeededLength, ActualLength, LinkedValue;
    UNICODE_STRING ReturnBuffer = { 0, 0, NULL };
    NTSTATUS Status;

    PAGED_CODE();

    if (PhysicalDeviceObject != NULL)
    {
        /* Parameters must pass three border of checks */
        DeviceObjectExtension = (PEXTENDED_DEVOBJ_EXTENSION)PhysicalDeviceObject->DeviceObjectExtension;

        /* 1st level: Presence of a Device Node */
        if (DeviceObjectExtension->DeviceNode == NULL)
        {
            DPRINT("PhysicalDeviceObject 0x%p doesn't have a DeviceNode\n", PhysicalDeviceObject);
            return STATUS_INVALID_DEVICE_REQUEST;
        }

        /* 2nd level: Presence of an non-zero length InstancePath */
        if (DeviceObjectExtension->DeviceNode->InstancePath.Length == 0)
        {
            DPRINT("PhysicalDeviceObject 0x%p's DOE has zero-length InstancePath\n", PhysicalDeviceObject);
            return STATUS_INVALID_DEVICE_REQUEST;
        }

        InstanceDevicePath = &DeviceObjectExtension->DeviceNode->InstancePath;
    }


    Status = IopOpenInterfaceKey(InterfaceClassGuid, KEY_ENUMERATE_SUB_KEYS, &InterfaceKey);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopOpenInterfaceKey() failed with status 0x%08lx\n", Status);
        goto cleanup;
    }

    /* Enumerate subkeys (i.e. the different device objects) */
    while (TRUE)
    {
        Status = ZwEnumerateKey(
            InterfaceKey,
            i,
            KeyBasicInformation,
            NULL,
            0,
            &Size);
        if (Status == STATUS_NO_MORE_ENTRIES)
        {
            break;
        }
        else if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_TOO_SMALL)
        {
            DPRINT("ZwEnumerateKey() failed with status 0x%08lx\n", Status);
            goto cleanup;
        }

        DeviceBi = ExAllocatePool(PagedPool, Size);
        if (!DeviceBi)
        {
            DPRINT("ExAllocatePool() failed\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }
        Status = ZwEnumerateKey(
            InterfaceKey,
            i++,
            KeyBasicInformation,
            DeviceBi,
            Size,
            &Size);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("ZwEnumerateKey() failed with status 0x%08lx\n", Status);
            goto cleanup;
        }

        /* Open device key */
        KeyName.Length = KeyName.MaximumLength = (USHORT)DeviceBi->NameLength;
        KeyName.Buffer = DeviceBi->Name;
        InitializeObjectAttributes(
            &ObjectAttributes,
            &KeyName,
            OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
            InterfaceKey,
            NULL);
        Status = ZwOpenKey(
            &DeviceKey,
            KEY_ENUMERATE_SUB_KEYS,
            &ObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("ZwOpenKey() failed with status 0x%08lx\n", Status);
            goto cleanup;
        }

        if (PhysicalDeviceObject)
        {
            /* Check if we are on the right physical device object,
            * by reading the DeviceInstance string
            */
            RtlInitUnicodeString(&KeyName, L"DeviceInstance");
            Status = ZwQueryValueKey(DeviceKey, &KeyName, KeyValuePartialInformation, NULL, 0, &NeededLength);
            if (Status == STATUS_BUFFER_TOO_SMALL)
            {
                ActualLength = NeededLength;
                PartialInfo = ExAllocatePool(NonPagedPool, ActualLength);
                if (!PartialInfo)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto cleanup;
                }

                Status = ZwQueryValueKey(DeviceKey, &KeyName, KeyValuePartialInformation, PartialInfo, ActualLength, &NeededLength);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("ZwQueryValueKey #2 failed (%x)\n", Status);
                    ExFreePool(PartialInfo);
                    goto cleanup;
                }
                if (PartialInfo->DataLength == InstanceDevicePath->Length)
                {
                    if (RtlCompareMemory(PartialInfo->Data, InstanceDevicePath->Buffer, InstanceDevicePath->Length) == InstanceDevicePath->Length)
                    {
                        /* found right pdo */
                        FoundRightPDO = TRUE;
                    }
                }
                ExFreePool(PartialInfo);
                PartialInfo = NULL;
                if (!FoundRightPDO)
                {
                    /* not yet found */
                    continue;
                }
            }
            else
            {
                /* error */
                break;
            }
        }

        /* Enumerate subkeys (ie the different reference strings) */
        j = 0;
        while (TRUE)
        {
            Status = ZwEnumerateKey(
                DeviceKey,
                j,
                KeyBasicInformation,
                NULL,
                0,
                &Size);
            if (Status == STATUS_NO_MORE_ENTRIES)
            {
                break;
            }
            else if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_TOO_SMALL)
            {
                DPRINT("ZwEnumerateKey() failed with status 0x%08lx\n", Status);
                goto cleanup;
            }

            ReferenceBi = ExAllocatePool(PagedPool, Size);
            if (!ReferenceBi)
            {
                DPRINT("ExAllocatePool() failed\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto cleanup;
            }
            Status = ZwEnumerateKey(
                DeviceKey,
                j++,
                KeyBasicInformation,
                ReferenceBi,
                Size,
                &Size);
            if (!NT_SUCCESS(Status))
            {
                DPRINT("ZwEnumerateKey() failed with status 0x%08lx\n", Status);
                goto cleanup;
            }

            KeyName.Length = KeyName.MaximumLength = (USHORT)ReferenceBi->NameLength;
            KeyName.Buffer = ReferenceBi->Name;
            if (RtlEqualUnicodeString(&KeyName, &Control, TRUE))
            {
                /* Skip Control subkey */
                goto NextReferenceString;
            }

            /* Open reference key */
            InitializeObjectAttributes(
                &ObjectAttributes,
                &KeyName,
                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                DeviceKey,
                NULL);
            Status = ZwOpenKey(
                &ReferenceKey,
                KEY_QUERY_VALUE,
                &ObjectAttributes);
            if (!NT_SUCCESS(Status))
            {
                DPRINT("ZwOpenKey() failed with status 0x%08lx\n", Status);
                goto cleanup;
            }

            if (!(Flags & DEVICE_INTERFACE_INCLUDE_NONACTIVE))
            {
                /* We have to check if the interface is enabled, by
                * reading the Linked value in the Control subkey
                */
                InitializeObjectAttributes(
                    &ObjectAttributes,
                    &Control,
                    OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                    ReferenceKey,
                    NULL);
                Status = ZwOpenKey(
                    &ControlKey,
                    KEY_QUERY_VALUE,
                    &ObjectAttributes);
                if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
                {
                    /* That's OK. The key doesn't exist (yet) because
                    * the interface is not activated.
                    */
                    goto NextReferenceString;
                }
                else if (!NT_SUCCESS(Status))
                {
                    DPRINT1("ZwOpenKey() failed with status 0x%08lx\n", Status);
                    goto cleanup;
                }

                RtlInitUnicodeString(&KeyName, L"Linked");
                Status = ZwQueryValueKey(ControlKey,
                                         &KeyName,
                                         KeyValuePartialInformation,
                                         NULL,
                                         0,
                                         &NeededLength);
                if (Status == STATUS_BUFFER_TOO_SMALL)
                {
                    ActualLength = NeededLength;
                    PartialInfo = ExAllocatePool(NonPagedPool, ActualLength);
                    if (!PartialInfo)
                    {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto cleanup;
                    }

                    Status = ZwQueryValueKey(ControlKey,
                                             &KeyName,
                                             KeyValuePartialInformation,
                                             PartialInfo,
                                             ActualLength,
                                             &NeededLength);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("ZwQueryValueKey #2 failed (%x)\n", Status);
                        ExFreePool(PartialInfo);
                        goto cleanup;
                    }

                    if (PartialInfo->Type != REG_DWORD || PartialInfo->DataLength != sizeof(ULONG))
                    {
                        DPRINT1("Bad registry read\n");
                        ExFreePool(PartialInfo);
                        goto cleanup;
                    }

                    RtlCopyMemory(&LinkedValue,
                                  PartialInfo->Data,
                                  PartialInfo->DataLength);

                    ExFreePool(PartialInfo);
                    if (LinkedValue == 0)
                    {
                        /* This interface isn't active */
                        goto NextReferenceString;
                    }
                }
                else
                {
                    DPRINT1("ZwQueryValueKey #1 failed (%x)\n", Status);
                    goto cleanup;
                }
            }

            /* Read the SymbolicLink string and add it into SymbolicLinkList */
            Status = ZwQueryValueKey(
                ReferenceKey,
                &SymbolicLink,
                KeyValuePartialInformation,
                NULL,
                0,
                &Size);
            if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_TOO_SMALL)
            {
                DPRINT("ZwQueryValueKey() failed with status 0x%08lx\n", Status);
                goto cleanup;
            }
            bip = ExAllocatePool(PagedPool, Size);
            if (!bip)
            {
                DPRINT("ExAllocatePool() failed\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto cleanup;
            }
            Status = ZwQueryValueKey(
                ReferenceKey,
                &SymbolicLink,
                KeyValuePartialInformation,
                bip,
                Size,
                &Size);
            if (!NT_SUCCESS(Status))
            {
                DPRINT("ZwQueryValueKey() failed with status 0x%08lx\n", Status);
                goto cleanup;
            }
            else if (bip->Type != REG_SZ)
            {
                DPRINT("Unexpected registry type 0x%lx (expected 0x%lx)\n", bip->Type, REG_SZ);
                Status = STATUS_UNSUCCESSFUL;
                goto cleanup;
            }
            else if (bip->DataLength < 5 * sizeof(WCHAR))
            {
                DPRINT("Registry string too short (length %lu, expected %lu at least)\n", bip->DataLength, 5 * sizeof(WCHAR));
                Status = STATUS_UNSUCCESSFUL;
                goto cleanup;
            }
            KeyName.Length = KeyName.MaximumLength = (USHORT)bip->DataLength;
            KeyName.Buffer = (PWSTR)bip->Data;

            /* Fixup the prefix (from "\\?\") */
            RtlCopyMemory(KeyName.Buffer, L"\\??\\", 4 * sizeof(WCHAR));

            /* Add new symbolic link to symbolic link list */
            if (ReturnBuffer.Length + KeyName.Length + sizeof(WCHAR) > ReturnBuffer.MaximumLength)
            {
                PWSTR NewBuffer;
                ReturnBuffer.MaximumLength = (USHORT)max(2 * ReturnBuffer.MaximumLength,
                                                         (USHORT)(ReturnBuffer.Length +
                                                         KeyName.Length +
                                                         2 * sizeof(WCHAR)));
                NewBuffer = ExAllocatePool(PagedPool, ReturnBuffer.MaximumLength);
                if (!NewBuffer)
                {
                    DPRINT("ExAllocatePool() failed\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto cleanup;
                }
                if (ReturnBuffer.Buffer)
                {
                    RtlCopyMemory(NewBuffer, ReturnBuffer.Buffer, ReturnBuffer.Length);
                    ExFreePool(ReturnBuffer.Buffer);
                }
                ReturnBuffer.Buffer = NewBuffer;
            }
            DPRINT("Adding symbolic link %wZ\n", &KeyName);
            Status = RtlAppendUnicodeStringToString(&ReturnBuffer, &KeyName);
            if (!NT_SUCCESS(Status))
            {
                DPRINT("RtlAppendUnicodeStringToString() failed with status 0x%08lx\n", Status);
                goto cleanup;
            }
            /* RtlAppendUnicodeStringToString added a NULL at the end of the
             * destination string, but didn't increase the Length field.
             * Do it for it.
             */
            ReturnBuffer.Length += sizeof(WCHAR);

NextReferenceString:
            ExFreePool(ReferenceBi);
            ReferenceBi = NULL;
            if (bip)
                ExFreePool(bip);
            bip = NULL;
            if (ReferenceKey != NULL)
            {
                ZwClose(ReferenceKey);
                ReferenceKey = NULL;
            }
            if (ControlKey != NULL)
            {
                ZwClose(ControlKey);
                ControlKey = NULL;
            }
        }
        if (FoundRightPDO)
        {
            /* No need to go further, as we already have found what we searched */
            break;
        }

        ExFreePool(DeviceBi);
        DeviceBi = NULL;
        ZwClose(DeviceKey);
        DeviceKey = NULL;
    }

    /* Add final NULL to ReturnBuffer */
    ASSERT(ReturnBuffer.Length <= ReturnBuffer.MaximumLength);
    if (ReturnBuffer.Length >= ReturnBuffer.MaximumLength)
    {
        PWSTR NewBuffer;
        ReturnBuffer.MaximumLength += sizeof(WCHAR);
        NewBuffer = ExAllocatePool(PagedPool, ReturnBuffer.MaximumLength);
        if (!NewBuffer)
        {
            DPRINT("ExAllocatePool() failed\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }
        if (ReturnBuffer.Buffer)
        {
            RtlCopyMemory(NewBuffer, ReturnBuffer.Buffer, ReturnBuffer.Length);
            ExFreePool(ReturnBuffer.Buffer);
        }
        ReturnBuffer.Buffer = NewBuffer;
    }
    ReturnBuffer.Buffer[ReturnBuffer.Length / sizeof(WCHAR)] = UNICODE_NULL;
    *SymbolicLinkList = ReturnBuffer.Buffer;
    Status = STATUS_SUCCESS;

cleanup:
    if (!NT_SUCCESS(Status) && ReturnBuffer.Buffer)
        ExFreePool(ReturnBuffer.Buffer);
    if (InterfaceKey != NULL)
        ZwClose(InterfaceKey);
    if (DeviceKey != NULL)
        ZwClose(DeviceKey);
    if (ReferenceKey != NULL)
        ZwClose(ReferenceKey);
    if (ControlKey != NULL)
        ZwClose(ControlKey);
    if (DeviceBi)
        ExFreePool(DeviceBi);
    if (ReferenceBi)
        ExFreePool(ReferenceBi);
    if (bip)
        ExFreePool(bip);
    return Status;
}

/*++
 * @name IoRegisterDeviceInterface
 * @implemented
 *
 * Registers a device interface class, if it has not been previously registered,
 * and creates a new instance of the interface class, which a driver can
 * subsequently enable for use by applications or other system components.
 * Documented in WDK.
 *
 * @param PhysicalDeviceObject
 *        Points to an optional PDO that narrows the search to only the
 *        device interfaces of the device represented by the PDO
 *
 * @param InterfaceClassGuid
 *        Points to a class GUID specifying the device interface class
 *
 * @param ReferenceString
 *        Optional parameter, pointing to a unicode string. For a full
 *        description of this rather rarely used param (usually drivers
 *        pass NULL here) see WDK
 *
 * @param SymbolicLinkName
 *        Pointer to the resulting unicode string
 *
 * @return Usual NTSTATUS
 *
 * @remarks Must be called at IRQL = PASSIVE_LEVEL in the context of a
 *          system thread
 *
 *--*/
NTSTATUS
NTAPI
IoRegisterDeviceInterface(IN PDEVICE_OBJECT PhysicalDeviceObject,
                          IN CONST GUID *InterfaceClassGuid,
                          IN PUNICODE_STRING ReferenceString OPTIONAL,
                          OUT PUNICODE_STRING SymbolicLinkName)
{
    PUNICODE_STRING InstancePath;
    UNICODE_STRING GuidString;
    UNICODE_STRING SubKeyName;
    UNICODE_STRING InterfaceKeyName;
    UNICODE_STRING BaseKeyName;
    UCHAR PdoNameInfoBuffer[sizeof(OBJECT_NAME_INFORMATION) + (256 * sizeof(WCHAR))];
    POBJECT_NAME_INFORMATION PdoNameInfo = (POBJECT_NAME_INFORMATION)PdoNameInfoBuffer;
    UNICODE_STRING DeviceInstance = RTL_CONSTANT_STRING(L"DeviceInstance");
    UNICODE_STRING SymbolicLink = RTL_CONSTANT_STRING(L"SymbolicLink");
    HANDLE ClassKey;
    HANDLE InterfaceKey;
    HANDLE SubKey;
    ULONG StartIndex;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG i;
    NTSTATUS Status, SymLinkStatus;
    PEXTENDED_DEVOBJ_EXTENSION DeviceObjectExtension;

    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    DPRINT("IoRegisterDeviceInterface(): PDO %p, RefString: %wZ\n",
        PhysicalDeviceObject, ReferenceString);

    /* Parameters must pass three border of checks */
    DeviceObjectExtension = (PEXTENDED_DEVOBJ_EXTENSION)PhysicalDeviceObject->DeviceObjectExtension;

    /* 1st level: Presence of a Device Node */
    if (DeviceObjectExtension->DeviceNode == NULL)
    {
        DPRINT("PhysicalDeviceObject 0x%p doesn't have a DeviceNode\n", PhysicalDeviceObject);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    /* 2nd level: Presence of an non-zero length InstancePath */
    if (DeviceObjectExtension->DeviceNode->InstancePath.Length == 0)
    {
        DPRINT("PhysicalDeviceObject 0x%p's DOE has zero-length InstancePath\n", PhysicalDeviceObject);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    /* 3rd level: Optional, based on WDK documentation */
    if (ReferenceString != NULL)
    {
        /* Reference string must not contain path-separator symbols */
        for (i = 0; i < ReferenceString->Length / sizeof(WCHAR); i++)
        {
            if ((ReferenceString->Buffer[i] == '\\') ||
                (ReferenceString->Buffer[i] == '/'))
                return STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    Status = RtlStringFromGUID(InterfaceClassGuid, &GuidString);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("RtlStringFromGUID() failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* Create Pdo name: \Device\xxxxxxxx (unnamed device) */
    Status = ObQueryNameString(
        PhysicalDeviceObject,
        PdoNameInfo,
        sizeof(PdoNameInfoBuffer),
        &i);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ObQueryNameString() failed with status 0x%08lx\n", Status);
        return Status;
    }
    ASSERT(PdoNameInfo->Name.Length);

    /* Create base key name for this interface: HKLM\SYSTEM\CurrentControlSet\Control\DeviceClasses\{GUID} */
    ASSERT(((PEXTENDED_DEVOBJ_EXTENSION)PhysicalDeviceObject->DeviceObjectExtension)->DeviceNode);
    InstancePath = &((PEXTENDED_DEVOBJ_EXTENSION)PhysicalDeviceObject->DeviceObjectExtension)->DeviceNode->InstancePath;
    BaseKeyName.Length = (USHORT)wcslen(BaseKeyString) * sizeof(WCHAR);
    BaseKeyName.MaximumLength = BaseKeyName.Length
        + GuidString.Length;
    BaseKeyName.Buffer = ExAllocatePool(
        PagedPool,
        BaseKeyName.MaximumLength);
    if (!BaseKeyName.Buffer)
    {
        DPRINT("ExAllocatePool() failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    wcscpy(BaseKeyName.Buffer, BaseKeyString);
    RtlAppendUnicodeStringToString(&BaseKeyName, &GuidString);

    /* Create BaseKeyName key in registry */
    InitializeObjectAttributes(
        &ObjectAttributes,
        &BaseKeyName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE | OBJ_OPENIF,
        NULL, /* RootDirectory */
        NULL); /* SecurityDescriptor */

    Status = ZwCreateKey(
        &ClassKey,
        KEY_WRITE,
        &ObjectAttributes,
        0, /* TileIndex */
        NULL, /* Class */
        REG_OPTION_NON_VOLATILE,
        NULL); /* Disposition */

    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwCreateKey() failed with status 0x%08lx\n", Status);
        ExFreePool(BaseKeyName.Buffer);
        return Status;
    }

    /* Create key name for this interface: ##?#ACPI#PNP0501#1#{GUID} */
    InterfaceKeyName.Length = 0;
    InterfaceKeyName.MaximumLength =
        4 * sizeof(WCHAR) + /* 4  = size of ##?# */
        InstancePath->Length +
        sizeof(WCHAR) +     /* 1  = size of # */
        GuidString.Length;
    InterfaceKeyName.Buffer = ExAllocatePool(
        PagedPool,
        InterfaceKeyName.MaximumLength);
    if (!InterfaceKeyName.Buffer)
    {
        DPRINT("ExAllocatePool() failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlAppendUnicodeToString(&InterfaceKeyName, L"##?#");
    StartIndex = InterfaceKeyName.Length / sizeof(WCHAR);
    RtlAppendUnicodeStringToString(&InterfaceKeyName, InstancePath);
    for (i = 0; i < InstancePath->Length / sizeof(WCHAR); i++)
    {
        if (InterfaceKeyName.Buffer[StartIndex + i] == '\\')
            InterfaceKeyName.Buffer[StartIndex + i] = '#';
    }
    RtlAppendUnicodeToString(&InterfaceKeyName, L"#");
    RtlAppendUnicodeStringToString(&InterfaceKeyName, &GuidString);

    /* Create the interface key in registry */
    InitializeObjectAttributes(
        &ObjectAttributes,
        &InterfaceKeyName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE | OBJ_OPENIF,
        ClassKey,
        NULL); /* SecurityDescriptor */

    Status = ZwCreateKey(
        &InterfaceKey,
        KEY_WRITE,
        &ObjectAttributes,
        0, /* TileIndex */
        NULL, /* Class */
        REG_OPTION_NON_VOLATILE,
        NULL); /* Disposition */

    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwCreateKey() failed with status 0x%08lx\n", Status);
        ZwClose(ClassKey);
        ExFreePool(BaseKeyName.Buffer);
        return Status;
    }

    /* Write DeviceInstance entry. Value is InstancePath */
    Status = ZwSetValueKey(
        InterfaceKey,
        &DeviceInstance,
        0, /* TileIndex */
        REG_SZ,
        InstancePath->Buffer,
        InstancePath->Length);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwSetValueKey() failed with status 0x%08lx\n", Status);
        ZwClose(InterfaceKey);
        ZwClose(ClassKey);
        ExFreePool(InterfaceKeyName.Buffer);
        ExFreePool(BaseKeyName.Buffer);
        return Status;
    }

    /* Create subkey. Name is #ReferenceString */
    SubKeyName.Length = 0;
    SubKeyName.MaximumLength = sizeof(WCHAR);
    if (ReferenceString && ReferenceString->Length)
        SubKeyName.MaximumLength += ReferenceString->Length;
    SubKeyName.Buffer = ExAllocatePool(
        PagedPool,
        SubKeyName.MaximumLength);
    if (!SubKeyName.Buffer)
    {
        DPRINT("ExAllocatePool() failed\n");
        ZwClose(InterfaceKey);
        ZwClose(ClassKey);
        ExFreePool(InterfaceKeyName.Buffer);
        ExFreePool(BaseKeyName.Buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlAppendUnicodeToString(&SubKeyName, L"#");
    if (ReferenceString && ReferenceString->Length)
        RtlAppendUnicodeStringToString(&SubKeyName, ReferenceString);

    /* Create SubKeyName key in registry */
    InitializeObjectAttributes(
        &ObjectAttributes,
        &SubKeyName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        InterfaceKey, /* RootDirectory */
        NULL); /* SecurityDescriptor */

    Status = ZwCreateKey(
        &SubKey,
        KEY_WRITE,
        &ObjectAttributes,
        0, /* TileIndex */
        NULL, /* Class */
        REG_OPTION_NON_VOLATILE,
        NULL); /* Disposition */

    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwCreateKey() failed with status 0x%08lx\n", Status);
        ZwClose(InterfaceKey);
        ZwClose(ClassKey);
        ExFreePool(InterfaceKeyName.Buffer);
        ExFreePool(BaseKeyName.Buffer);
        return Status;
    }

    /* Create symbolic link name: \??\ACPI#PNP0501#1#{GUID}\ReferenceString */
    SymbolicLinkName->Length = 0;
    SymbolicLinkName->MaximumLength = SymbolicLinkName->Length
        + 4 * sizeof(WCHAR) /* 4 = size of \??\ */
        + InstancePath->Length
        + sizeof(WCHAR)     /* 1  = size of # */
        + GuidString.Length
        + sizeof(WCHAR);    /* final NULL */
    if (ReferenceString && ReferenceString->Length)
        SymbolicLinkName->MaximumLength += sizeof(WCHAR) + ReferenceString->Length;
    SymbolicLinkName->Buffer = ExAllocatePool(
        PagedPool,
        SymbolicLinkName->MaximumLength);
    if (!SymbolicLinkName->Buffer)
    {
        DPRINT("ExAllocatePool() failed\n");
        ZwClose(SubKey);
        ZwClose(InterfaceKey);
        ZwClose(ClassKey);
        ExFreePool(InterfaceKeyName.Buffer);
        ExFreePool(SubKeyName.Buffer);
        ExFreePool(BaseKeyName.Buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlAppendUnicodeToString(SymbolicLinkName, L"\\??\\");
    StartIndex = SymbolicLinkName->Length / sizeof(WCHAR);
    RtlAppendUnicodeStringToString(SymbolicLinkName, InstancePath);
    for (i = 0; i < InstancePath->Length / sizeof(WCHAR); i++)
    {
        if (SymbolicLinkName->Buffer[StartIndex + i] == '\\')
            SymbolicLinkName->Buffer[StartIndex + i] = '#';
    }
    RtlAppendUnicodeToString(SymbolicLinkName, L"#");
    RtlAppendUnicodeStringToString(SymbolicLinkName, &GuidString);
    SymbolicLinkName->Buffer[SymbolicLinkName->Length/sizeof(WCHAR)] = L'\0';

    /* Create symbolic link */
    DPRINT("IoRegisterDeviceInterface(): creating symbolic link %wZ -> %wZ\n", SymbolicLinkName, &PdoNameInfo->Name);
    SymLinkStatus = IoCreateSymbolicLink(SymbolicLinkName, &PdoNameInfo->Name);

    /* If the symbolic link already exists, return an informational success status */
    if (SymLinkStatus == STATUS_OBJECT_NAME_COLLISION)
    {
        /* HACK: Delete the existing symbolic link and update it to the new PDO name */
        IoDeleteSymbolicLink(SymbolicLinkName);
        IoCreateSymbolicLink(SymbolicLinkName, &PdoNameInfo->Name);
        SymLinkStatus = STATUS_OBJECT_NAME_EXISTS;
    }

    if (!NT_SUCCESS(SymLinkStatus))
    {
        DPRINT1("IoCreateSymbolicLink() failed with status 0x%08lx\n", SymLinkStatus);
        ZwClose(SubKey);
        ZwClose(InterfaceKey);
        ZwClose(ClassKey);
        ExFreePool(SubKeyName.Buffer);
        ExFreePool(InterfaceKeyName.Buffer);
        ExFreePool(BaseKeyName.Buffer);
        ExFreePool(SymbolicLinkName->Buffer);
        return SymLinkStatus;
    }

    if (ReferenceString && ReferenceString->Length)
    {
        RtlAppendUnicodeToString(SymbolicLinkName, L"\\");
        RtlAppendUnicodeStringToString(SymbolicLinkName, ReferenceString);
    }
    SymbolicLinkName->Buffer[SymbolicLinkName->Length/sizeof(WCHAR)] = L'\0';

    /* Write symbolic link name in registry */
    SymbolicLinkName->Buffer[1] = '\\';
    Status = ZwSetValueKey(
        SubKey,
        &SymbolicLink,
        0, /* TileIndex */
        REG_SZ,
        SymbolicLinkName->Buffer,
        SymbolicLinkName->Length);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwSetValueKey() failed with status 0x%08lx\n", Status);
        ExFreePool(SymbolicLinkName->Buffer);
    }
    else
    {
        SymbolicLinkName->Buffer[1] = '?';
    }

    ZwClose(SubKey);
    ZwClose(InterfaceKey);
    ZwClose(ClassKey);
    ExFreePool(SubKeyName.Buffer);
    ExFreePool(InterfaceKeyName.Buffer);
    ExFreePool(BaseKeyName.Buffer);

    return NT_SUCCESS(Status) ? SymLinkStatus : Status;
}

/*++
 * @name IoSetDeviceInterfaceState
 * @implemented
 *
 * Enables or disables an instance of a previously registered device
 * interface class.
 * Documented in WDK.
 *
 * @param SymbolicLinkName
 *        Pointer to the string identifying instance to enable or disable
 *
 * @param Enable
 *        TRUE = enable, FALSE = disable
 *
 * @return Usual NTSTATUS
 *
 * @remarks Must be called at IRQL = PASSIVE_LEVEL in the context of a
 *          system thread
 *
 *--*/
NTSTATUS
NTAPI
IoSetDeviceInterfaceState(IN PUNICODE_STRING SymbolicLinkName,
                          IN BOOLEAN Enable)
{
    PDEVICE_OBJECT PhysicalDeviceObject;
    UNICODE_STRING GuidString;
    NTSTATUS Status;
    LPCGUID EventGuid;
    HANDLE InstanceHandle, ControlHandle;
    UNICODE_STRING KeyName, DeviceInstance;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG LinkedValue, Index;
    GUID DeviceGuid;
    UNICODE_STRING DosDevicesPrefix1 = RTL_CONSTANT_STRING(L"\\??\\");
    UNICODE_STRING DosDevicesPrefix2 = RTL_CONSTANT_STRING(L"\\\\?\\");
    UNICODE_STRING LinkNameNoPrefix;
    USHORT i;
    USHORT ReferenceStringOffset;

    if (SymbolicLinkName == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    DPRINT("IoSetDeviceInterfaceState('%wZ', %u)\n", SymbolicLinkName, Enable);

    /* Symbolic link name is \??\ACPI#PNP0501#1#{GUID}\ReferenceString */
    /* Make sure it starts with the expected prefix */
    if (!RtlPrefixUnicodeString(&DosDevicesPrefix1, SymbolicLinkName, FALSE) &&
        !RtlPrefixUnicodeString(&DosDevicesPrefix2, SymbolicLinkName, FALSE))
    {
        DPRINT1("IoSetDeviceInterfaceState() invalid link name '%wZ'\n", SymbolicLinkName);
        return STATUS_INVALID_PARAMETER;
    }

    /* Make a version without the prefix for further processing */
    ASSERT(DosDevicesPrefix1.Length == DosDevicesPrefix2.Length);
    ASSERT(SymbolicLinkName->Length >= DosDevicesPrefix1.Length);
    LinkNameNoPrefix.Buffer = SymbolicLinkName->Buffer + DosDevicesPrefix1.Length / sizeof(WCHAR);
    LinkNameNoPrefix.Length = SymbolicLinkName->Length - DosDevicesPrefix1.Length;
    LinkNameNoPrefix.MaximumLength = LinkNameNoPrefix.Length;

    /* Find the reference string, if any */
    for (i = 0; i < LinkNameNoPrefix.Length / sizeof(WCHAR); i++)
    {
        if (LinkNameNoPrefix.Buffer[i] == L'\\')
        {
            break;
        }
    }
    ReferenceStringOffset = i * sizeof(WCHAR);

    /* The GUID is before the reference string or at the end */
    ASSERT(LinkNameNoPrefix.Length >= ReferenceStringOffset);
    if (ReferenceStringOffset < GUID_STRING_BYTES + sizeof(WCHAR))
    {
        DPRINT1("IoSetDeviceInterfaceState() invalid link name '%wZ'\n", SymbolicLinkName);
        return STATUS_INVALID_PARAMETER;
    }

    GuidString.Buffer = LinkNameNoPrefix.Buffer + (ReferenceStringOffset - GUID_STRING_BYTES) / sizeof(WCHAR);
    GuidString.Length = GUID_STRING_BYTES;
    GuidString.MaximumLength = GuidString.Length;
    Status = RtlGUIDFromString(&GuidString, &DeviceGuid);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlGUIDFromString() invalid GUID '%wZ' in link name '%wZ'\n", &GuidString, SymbolicLinkName);
        return Status;
    }

    /* Open registry keys */
    Status = OpenRegistryHandlesFromSymbolicLink(SymbolicLinkName,
                                                 KEY_CREATE_SUB_KEY,
                                                 NULL,
                                                 NULL,
                                                 &InstanceHandle);
    if (!NT_SUCCESS(Status))
        return Status;

    RtlInitUnicodeString(&KeyName, L"Control");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               InstanceHandle,
                               NULL);
    Status = ZwCreateKey(&ControlHandle,
                         KEY_SET_VALUE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         NULL);
    ZwClose(InstanceHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create the Control subkey\n");
        return Status;
    }

    LinkedValue = (Enable ? 1 : 0);

    RtlInitUnicodeString(&KeyName, L"Linked");
    Status = ZwSetValueKey(ControlHandle,
                           &KeyName,
                           0,
                           REG_DWORD,
                           &LinkedValue,
                           sizeof(ULONG));
    ZwClose(ControlHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to write the Linked value\n");
        return Status;
    }

    ASSERT(GuidString.Buffer >= LinkNameNoPrefix.Buffer + 1);
    DeviceInstance.Length = (GuidString.Buffer - LinkNameNoPrefix.Buffer - 1) * sizeof(WCHAR);
    if (DeviceInstance.Length == 0)
    {
        DPRINT1("No device instance in link name '%wZ'\n", SymbolicLinkName);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }
    DeviceInstance.MaximumLength = DeviceInstance.Length;
    DeviceInstance.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                  DeviceInstance.MaximumLength,
                                                  TAG_IO);
    if (DeviceInstance.Buffer == NULL)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(DeviceInstance.Buffer,
                  LinkNameNoPrefix.Buffer,
                  DeviceInstance.Length);

    for (Index = 0; Index < DeviceInstance.Length / sizeof(WCHAR); Index++)
    {
        if (DeviceInstance.Buffer[Index] == L'#')
        {
            DeviceInstance.Buffer[Index] = L'\\';
        }
    }

    PhysicalDeviceObject = IopGetDeviceObjectFromDeviceInstance(&DeviceInstance);

    if (!PhysicalDeviceObject)
    {
        DPRINT1("IopGetDeviceObjectFromDeviceInstance failed to find device object for %wZ\n", &DeviceInstance);
        ExFreePoolWithTag(DeviceInstance.Buffer, TAG_IO);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    ExFreePoolWithTag(DeviceInstance.Buffer, TAG_IO);

    EventGuid = Enable ? &GUID_DEVICE_INTERFACE_ARRIVAL : &GUID_DEVICE_INTERFACE_REMOVAL;

    PiNotifyDeviceInterfaceChange(EventGuid, &DeviceGuid, SymbolicLinkName);
    IopQueueDeviceChangeEvent(EventGuid, &DeviceGuid, SymbolicLinkName);

    ObDereferenceObject(PhysicalDeviceObject);
    DPRINT("Status %x\n", Status);
    return STATUS_SUCCESS;
}

/* EOF */

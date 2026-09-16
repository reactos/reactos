/*
 * PROJECT:     ReactOS kernel mode tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for the DEVPROPKEY device and device interface property functions
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include <kmt_test.h>
#include <devpkey.h>

#define NDEBUG
#include <debug.h>

#define KMT_PROPERTY_FMTID { 0x5F1E3C8A, 0x7D2B, 0x4E61, { 0x9A, 0x0C, 0x3B, 0x8D, 0x4F, 0x2E, 0x6A, 0x17 } }

static const DEVPROPKEY TestKeyNumber    = { KMT_PROPERTY_FMTID, 2 };
static const DEVPROPKEY TestKeyString    = { KMT_PROPERTY_FMTID, 3 };
static const DEVPROPKEY TestKeyList      = { KMT_PROPERTY_FMTID, 4 };
static const DEVPROPKEY TestKeyBinary    = { KMT_PROPERTY_FMTID, 5 };
static const DEVPROPKEY TestKeyLocalized = { KMT_PROPERTY_FMTID, 6 };
static const DEVPROPKEY TestKeyWidePid   = { KMT_PROPERTY_FMTID, 0x12345 };
static const DEVPROPKEY TestKeyInvalid   = { KMT_PROPERTY_FMTID, 7 };

static const GUID KmtDiskInterface   = { 0x53F56307, 0xB6BF, 0x11D0, { 0x94, 0xF2, 0x00, 0xA0, 0xC9, 0x1E, 0xFB, 0x8B } };
static const GUID KmtCdromInterface  = { 0x53F56308, 0xB6BF, 0x11D0, { 0x94, 0xF2, 0x00, 0xA0, 0xC9, 0x1E, 0xFB, 0x8B } };
static const GUID KmtVolumeInterface = { 0x53F5630D, 0xB6BF, 0x11D0, { 0x94, 0xF2, 0x00, 0xA0, 0xC9, 0x1E, 0xFB, 0x8B } };

static const GUID* CandidateClasses[] =
{
    &KmtDiskInterface,
    &KmtCdromInterface,
    &KmtVolumeInterface,
};

#define LCID_EN_US 0x0409
#define LCID_DE_DE 0x0407

typedef struct _PROPERTY_TARGET
{
    PCSTR Description;
    PDEVICE_OBJECT Pdo;
    PUNICODE_STRING SymbolicLinkName;
} PROPERTY_TARGET, *PPROPERTY_TARGET;

static
NTSTATUS
SetProperty(
    _In_ PPROPERTY_TARGET Target,
    _In_ const DEVPROPKEY *Key,
    _In_ LCID Lcid,
    _In_ DEVPROPTYPE Type,
    _In_ ULONG Size,
    _In_opt_ PVOID Data)
{
    if (Target->Pdo)
        return IoSetDevicePropertyData(Target->Pdo, Key, Lcid, 0, Type, Size, Data);

    return IoSetDeviceInterfacePropertyData(Target->SymbolicLinkName, Key, Lcid, 0, Type, Size, Data);
}

static
NTSTATUS
GetProperty(
    _In_ PPROPERTY_TARGET Target,
    _In_ const DEVPROPKEY *Key,
    _In_ LCID Lcid,
    _In_ ULONG Size,
    _Out_opt_ PVOID Data,
    _Out_ PULONG RequiredSize,
    _Out_ PDEVPROPTYPE Type)
{
    if (Target->Pdo)
        return IoGetDevicePropertyData(Target->Pdo, Key, Lcid, 0, Size, Data, RequiredSize, Type);

    return IoGetDeviceInterfacePropertyData(Target->SymbolicLinkName, Key, Lcid, 0, Size, Data, RequiredSize, Type);
}

static
VOID
ExpectProperty(
    _In_ PPROPERTY_TARGET Target,
    _In_ const DEVPROPKEY *Key,
    _In_ LCID Lcid,
    _In_ DEVPROPTYPE ExpectedType,
    _In_reads_bytes_(ExpectedSize) PVOID ExpectedData,
    _In_ ULONG ExpectedSize)
{
    UCHAR Buffer[128];
    ULONG Required;
    DEVPROPTYPE Type;
    NTSTATUS Status;

    ASSERT(ExpectedSize < sizeof(Buffer));

    Required = 0x55555555;
    Type = 0x55555555;
    Status = GetProperty(Target, Key, Lcid, 0, NULL, &Required, &Type);
    ok_eq_hex(Status, ExpectedSize ? STATUS_BUFFER_TOO_SMALL : STATUS_SUCCESS);
    ok_eq_ulong(Required, ExpectedSize);
    ok_eq_hex(Type, ExpectedType);

    if (ExpectedSize)
    {
        RtlFillMemory(Buffer, sizeof(Buffer), 0xAA);
        Status = GetProperty(Target, Key, Lcid, ExpectedSize - 1, Buffer, &Required, &Type);
        ok_eq_hex(Status, STATUS_BUFFER_TOO_SMALL);
        ok_eq_ulong(Required, ExpectedSize);
        ok_eq_uint(Buffer[0], 0xAA);
    }

    RtlFillMemory(Buffer, sizeof(Buffer), 0xAA);
    Required = 0x55555555;
    Type = 0x55555555;
    Status = GetProperty(Target, Key, Lcid, sizeof(Buffer), Buffer, &Required, &Type);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(Required, ExpectedSize);
    ok_eq_hex(Type, ExpectedType);
    ok(RtlEqualMemory(Buffer, ExpectedData, ExpectedSize),
       "%s: property data does not match\n", Target->Description);
    ok_eq_uint(Buffer[ExpectedSize], 0xAA);
}

static
VOID
ExpectNoProperty(
    _In_ PPROPERTY_TARGET Target,
    _In_ const DEVPROPKEY *Key,
    _In_ LCID Lcid)
{
    UCHAR Buffer[16];
    ULONG Required = 0x55555555;
    DEVPROPTYPE Type = 0x55555555;
    NTSTATUS Status;

    Status = GetProperty(Target, Key, Lcid, sizeof(Buffer), Buffer, &Required, &Type);
    ok_eq_hex(Status, STATUS_OBJECT_NAME_NOT_FOUND);
    ok_eq_ulong(Required, 0UL);
    ok_eq_hex(Type, DEVPROP_TYPE_EMPTY);
}

static
VOID
Test_StoreRoundTrip(
    _In_ PPROPERTY_TARGET Target)
{
    static const WCHAR String[] = L"ReactOS property store";
    static const WCHAR List[] = L"First\0Second\0Third\0";
    static const UCHAR Binary[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };
    static const ULONG Numbers[] = { 1, 2, 3 };
    ULONG Number = 0x12345678;
    NTSTATUS Status;

    trace("Store round trip on %s\n", Target->Description);

    SetProperty(Target, &TestKeyNumber, LOCALE_NEUTRAL, DEVPROP_TYPE_EMPTY, 0, NULL);
    ExpectNoProperty(Target, &TestKeyNumber, LOCALE_NEUTRAL);

    Status = SetProperty(Target, &TestKeyNumber, LOCALE_NEUTRAL, DEVPROP_TYPE_UINT32, sizeof(Number), &Number);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ExpectProperty(Target, &TestKeyNumber, LOCALE_NEUTRAL, DEVPROP_TYPE_UINT32, &Number, sizeof(Number));

    Status = SetProperty(Target, &TestKeyNumber, LOCALE_NEUTRAL, DEVPROP_TYPE_UINT32 | DEVPROP_TYPEMOD_ARRAY,
                         sizeof(Numbers), (PVOID)Numbers);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ExpectProperty(Target, &TestKeyNumber, LOCALE_NEUTRAL, DEVPROP_TYPE_UINT32 | DEVPROP_TYPEMOD_ARRAY,
                   (PVOID)Numbers, sizeof(Numbers));

    Status = SetProperty(Target, &TestKeyString, LOCALE_NEUTRAL, DEVPROP_TYPE_STRING, sizeof(String), (PVOID)String);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ExpectProperty(Target, &TestKeyString, LOCALE_NEUTRAL, DEVPROP_TYPE_STRING, (PVOID)String, sizeof(String));

    Status = SetProperty(Target, &TestKeyList, LOCALE_NEUTRAL, DEVPROP_TYPE_STRING_LIST, sizeof(List), (PVOID)List);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ExpectProperty(Target, &TestKeyList, LOCALE_NEUTRAL, DEVPROP_TYPE_STRING_LIST, (PVOID)List, sizeof(List));

    Status = SetProperty(Target, &TestKeyBinary, LOCALE_NEUTRAL, DEVPROP_TYPE_BINARY, sizeof(Binary), (PVOID)Binary);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ExpectProperty(Target, &TestKeyBinary, LOCALE_NEUTRAL, DEVPROP_TYPE_BINARY, (PVOID)Binary, sizeof(Binary));

    Status = SetProperty(Target, &TestKeyWidePid, LOCALE_NEUTRAL, DEVPROP_TYPE_UINT32, sizeof(Number), &Number);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ExpectProperty(Target, &TestKeyWidePid, LOCALE_NEUTRAL, DEVPROP_TYPE_UINT32, &Number, sizeof(Number));

    Status = SetProperty(Target, &TestKeyBinary, LOCALE_NEUTRAL, DEVPROP_TYPE_NULL, 0, &Number);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ExpectProperty(Target, &TestKeyBinary, LOCALE_NEUTRAL, DEVPROP_TYPE_NULL, NULL, 0);

    Status = SetProperty(Target, &TestKeyNumber, LOCALE_NEUTRAL, DEVPROP_TYPE_UINT32, sizeof(Number), NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ExpectNoProperty(Target, &TestKeyNumber, LOCALE_NEUTRAL);

    Status = SetProperty(Target, &TestKeyNumber, LOCALE_NEUTRAL, DEVPROP_TYPE_EMPTY, 0, NULL);
    ok_eq_hex(Status, STATUS_OBJECT_NAME_NOT_FOUND);

    Status = SetProperty(Target, &TestKeyString, LOCALE_NEUTRAL, DEVPROP_TYPE_EMPTY, 0, &Number);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ExpectNoProperty(Target, &TestKeyString, LOCALE_NEUTRAL);

    Status = SetProperty(Target, &TestKeyList, LOCALE_NEUTRAL, DEVPROP_TYPE_EMPTY, 0, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = SetProperty(Target, &TestKeyBinary, LOCALE_NEUTRAL, DEVPROP_TYPE_EMPTY, 0, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = SetProperty(Target, &TestKeyWidePid, LOCALE_NEUTRAL, DEVPROP_TYPE_EMPTY, 0, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ExpectNoProperty(Target, &TestKeyList, LOCALE_NEUTRAL);
    ExpectNoProperty(Target, &TestKeyBinary, LOCALE_NEUTRAL);
    ExpectNoProperty(Target, &TestKeyWidePid, LOCALE_NEUTRAL);
}

static
VOID
Test_Locales(
    _In_ PPROPERTY_TARGET Target)
{
    static const WCHAR English[] = L"Hello";
    static const WCHAR German[] = L"Hallo";
    static const WCHAR Neutral[] = L"Neutral";
    static const WCHAR Indirect[] = L"@machine.inf,%DeviceDesc%;Device";
    NTSTATUS Status;

    trace("Locales on %s\n", Target->Description);

    Status = SetProperty(Target, &TestKeyLocalized, LCID_EN_US, DEVPROP_TYPE_STRING, sizeof(English), (PVOID)English);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ExpectProperty(Target, &TestKeyLocalized, LCID_EN_US, DEVPROP_TYPE_STRING, (PVOID)English, sizeof(English));
    ExpectNoProperty(Target, &TestKeyLocalized, LOCALE_NEUTRAL);
    ExpectNoProperty(Target, &TestKeyLocalized, LCID_DE_DE);

    Status = SetProperty(Target, &TestKeyLocalized, LCID_DE_DE, DEVPROP_TYPE_STRING, sizeof(German), (PVOID)German);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = SetProperty(Target, &TestKeyLocalized, LOCALE_NEUTRAL, DEVPROP_TYPE_STRING, sizeof(Neutral), (PVOID)Neutral);
    ok_eq_hex(Status, STATUS_SUCCESS);

    ExpectProperty(Target, &TestKeyLocalized, LCID_EN_US, DEVPROP_TYPE_STRING, (PVOID)English, sizeof(English));
    ExpectProperty(Target, &TestKeyLocalized, LCID_DE_DE, DEVPROP_TYPE_STRING, (PVOID)German, sizeof(German));
    ExpectProperty(Target, &TestKeyLocalized, LOCALE_NEUTRAL, DEVPROP_TYPE_STRING, (PVOID)Neutral, sizeof(Neutral));

    Status = SetProperty(Target, &TestKeyLocalized, LCID_EN_US, DEVPROP_TYPE_EMPTY, 0, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ExpectNoProperty(Target, &TestKeyLocalized, LCID_EN_US);
    ExpectProperty(Target, &TestKeyLocalized, LCID_DE_DE, DEVPROP_TYPE_STRING, (PVOID)German, sizeof(German));
    ExpectProperty(Target, &TestKeyLocalized, LOCALE_NEUTRAL, DEVPROP_TYPE_STRING, (PVOID)Neutral, sizeof(Neutral));

    Status = SetProperty(Target, &TestKeyLocalized, LCID_EN_US, DEVPROP_TYPE_STRING_INDIRECT, sizeof(Indirect), (PVOID)Indirect);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    Status = SetProperty(Target, &TestKeyLocalized, LOCALE_NEUTRAL, DEVPROP_TYPE_STRING_INDIRECT, sizeof(Indirect), (PVOID)Indirect);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ExpectProperty(Target, &TestKeyLocalized, LOCALE_NEUTRAL, DEVPROP_TYPE_STRING_INDIRECT, (PVOID)Indirect, sizeof(Indirect));

    Status = SetProperty(Target, &TestKeyLocalized, LCID_DE_DE, DEVPROP_TYPE_EMPTY, 0, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = SetProperty(Target, &TestKeyLocalized, LOCALE_NEUTRAL, DEVPROP_TYPE_EMPTY, 0, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ExpectNoProperty(Target, &TestKeyLocalized, LCID_DE_DE);
    ExpectNoProperty(Target, &TestKeyLocalized, LOCALE_NEUTRAL);
}

static
VOID
Test_Validation(
    _In_ PPROPERTY_TARGET Target)
{
    static const WCHAR Unterminated[] = { L'a', L'b', L'c' };
    static const WCHAR EmbeddedNul[] = { L'a', 0, L'b', 0 };
    static const WCHAR UnterminatedList[] = { L'a', 0, L'b', 0 };
    static const WCHAR DoubleNulOnly[] = { 0, 0 };
    static const WCHAR EmptyList[] = { 0 };
    static const WCHAR ValidString[] = L"x";
    struct
    {
        DEVPROPTYPE Type;
        PVOID Data;
        ULONG Size;
        NTSTATUS Expected;
    } Cases[16];
    UCHAR BadBoolean = 1, GoodBoolean = (UCHAR)DEVPROP_TRUE;
    LARGE_INTEGER NegativeTime = { .QuadPart = -1 };
    ULONG Number = 5;
    ULONG Index = 0, i;
    NTSTATUS Status;

    trace("Validation on %s\n", Target->Description);

#define ADD_CASE(t, d, s, e) \
    do { ASSERT(Index < RTL_NUMBER_OF(Cases)); \
         Cases[Index].Type = (t); Cases[Index].Data = (PVOID)(d); \
         Cases[Index].Size = (s); Cases[Index].Expected = (e); Index++; } while (0)

    ADD_CASE(DEVPROP_TYPE_STRING,      Unterminated,     sizeof(Unterminated),     STATUS_INVALID_PARAMETER);
    ADD_CASE(DEVPROP_TYPE_STRING,      EmbeddedNul,      sizeof(EmbeddedNul),      STATUS_INVALID_PARAMETER);
    ADD_CASE(DEVPROP_TYPE_STRING,      ValidString,      1,                        STATUS_INVALID_PARAMETER);
    ADD_CASE(DEVPROP_TYPE_STRING_LIST, UnterminatedList, sizeof(UnterminatedList), STATUS_INVALID_PARAMETER);
    ADD_CASE(DEVPROP_TYPE_STRING_LIST, DoubleNulOnly,    sizeof(DoubleNulOnly),    STATUS_INVALID_PARAMETER);
    ADD_CASE(DEVPROP_TYPE_STRING_LIST, EmptyList,        sizeof(EmptyList),        STATUS_SUCCESS);
    ADD_CASE(DEVPROP_TYPE_UINT32,      &Number,          3,                        STATUS_INVALID_PARAMETER);
    ADD_CASE(DEVPROP_TYPE_BOOLEAN,     &BadBoolean,      sizeof(BadBoolean),       STATUS_INVALID_PARAMETER);
    ADD_CASE(DEVPROP_TYPE_BOOLEAN,     &GoodBoolean,     sizeof(GoodBoolean),      STATUS_SUCCESS);
    ADD_CASE(DEVPROP_TYPE_FILETIME,    &NegativeTime,    sizeof(NegativeTime),     STATUS_INVALID_PARAMETER);
    ADD_CASE(DEVPROP_TYPE_BINARY,      &Number,          0,                        STATUS_INVALID_PARAMETER);
    ADD_CASE(DEVPROP_TYPE_EMPTY,       &Number,          sizeof(Number),           STATUS_INVALID_PARAMETER);
    ADD_CASE(DEVPROP_TYPE_STRING | DEVPROP_TYPEMOD_ARRAY, ValidString, sizeof(ValidString), STATUS_INVALID_PARAMETER);
    ADD_CASE(DEVPROP_TYPE_UINT32 | DEVPROP_TYPEMOD_LIST,  &Number,     sizeof(Number),      STATUS_INVALID_PARAMETER);
    ADD_CASE(DEVPROP_TYPE_UINT32 | 0x4000,                &Number,     sizeof(Number),      STATUS_INVALID_PARAMETER);
    ADD_CASE(MAX_DEVPROP_TYPE + 1,                        &Number,     sizeof(Number),      STATUS_INVALID_PARAMETER);

#undef ADD_CASE

    for (i = 0; i < Index; i++)
    {
        Status = SetProperty(Target, &TestKeyInvalid, LOCALE_NEUTRAL, Cases[i].Type, Cases[i].Size, Cases[i].Data);
        ok(Status == Cases[i].Expected,
           "%s case %lu (type 0x%lx, size %lu): got 0x%08lx, expected 0x%08lx\n",
           Target->Description, i, Cases[i].Type, Cases[i].Size, Status, Cases[i].Expected);

        if (NT_SUCCESS(Cases[i].Expected))
            ExpectProperty(Target, &TestKeyInvalid, LOCALE_NEUTRAL, Cases[i].Type, Cases[i].Data, Cases[i].Size);
        else
            ExpectNoProperty(Target, &TestKeyInvalid, LOCALE_NEUTRAL);

        SetProperty(Target, &TestKeyInvalid, LOCALE_NEUTRAL, DEVPROP_TYPE_EMPTY, 0, NULL);
    }
}

static
VOID
Test_DeviceBuiltins(
    _In_ PPROPERTY_TARGET Target)
{
    static const WCHAR FriendlyName[] = L"kmtest friendly name";
    static UNICODE_STRING PnpManagerName = RTL_CONSTANT_STRING(L"\\Driver\\PnpManager");
    WCHAR Expected[512], Actual[512], Saved[512];
    ULONG ExpectedLength, Required, SavedSize = 0;
    DEVPROPTYPE Type;
    DEVPROP_BOOLEAN Boolean;
    BOOLEAN HadFriendlyName;
    ULONG Number;
    NTSTATUS Status;

    trace("Device builtins\n");

    Status = IoGetDeviceProperty(Target->Pdo, DevicePropertyEnumeratorName,
                                 sizeof(Expected), Expected, &ExpectedLength);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = GetProperty(Target, &DEVPKEY_Device_InstanceId, LOCALE_NEUTRAL,
                         sizeof(Actual), Actual, &Required, &Type);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(Type, DEVPROP_TYPE_STRING);
    if (NT_SUCCESS(Status))
    {
        SIZE_T EnumeratorChars = wcslen(Expected);

        ok_eq_ulong(Required, (ULONG)((wcslen(Actual) + 1) * sizeof(WCHAR)));
        ok(!_wcsnicmp(Actual, Expected, EnumeratorChars) && (Actual[EnumeratorChars] == L'\\'),
           "Instance ID '%ls' does not start with enumerator '%ls'\n", Actual, Expected);
    }

    Status = IoGetDeviceProperty(Target->Pdo, DevicePropertyPhysicalDeviceObjectName,
                                 sizeof(Expected), Expected, &ExpectedLength);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = GetProperty(Target, &DEVPKEY_Device_PDOName, LOCALE_NEUTRAL,
                         sizeof(Actual), Actual, &Required, &Type);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(Type, DEVPROP_TYPE_STRING);
    if (NT_SUCCESS(Status))
        ok_eq_wstr(Actual, Expected);

    Status = IoGetDeviceProperty(Target->Pdo, DevicePropertyHardwareID,
                                 sizeof(Expected), Expected, &ExpectedLength);
    if (!skip(NT_SUCCESS(Status), "No hardware IDs: 0x%08lx\n", Status))
    {
        Status = GetProperty(Target, &DEVPKEY_Device_HardwareIds, LOCALE_NEUTRAL,
                             sizeof(Actual), Actual, &Required, &Type);
        ok_eq_hex(Status, STATUS_SUCCESS);
        ok_eq_hex(Type, DEVPROP_TYPE_STRING_LIST);
        if (NT_SUCCESS(Status))
            ok_eq_wstr(Actual, Expected);

        if (!RtlEqualUnicodeString(&Target->Pdo->DriverObject->DriverName, &PnpManagerName, TRUE))
        {
            Status = SetProperty(Target, &DEVPKEY_Device_HardwareIds, LOCALE_NEUTRAL,
                                 DEVPROP_TYPE_STRING_LIST, Required, Actual);
            ok_eq_hex(Status, STATUS_ACCESS_DENIED);
        }
    }

    Status = GetProperty(Target, &DEVPKEY_Device_DevNodeStatus, LOCALE_NEUTRAL,
                         sizeof(Number), &Number, &Required, &Type);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(Type, DEVPROP_TYPE_UINT32);
    ok_eq_ulong(Required, (ULONG)sizeof(Number));

    Status = GetProperty(Target, &DEVPKEY_Device_ProblemCode, LOCALE_NEUTRAL,
                         sizeof(Number), &Number, &Required, &Type);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(Type, DEVPROP_TYPE_UINT32);

    Boolean = 0x55;
    Status = GetProperty(Target, &DEVPKEY_Device_IsPresent, LOCALE_NEUTRAL,
                         sizeof(Boolean), &Boolean, &Required, &Type);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(Type, DEVPROP_TYPE_BOOLEAN);
    ok_eq_int(Boolean, DEVPROP_TRUE);

    Status = GetProperty(Target, &DEVPKEY_Device_Parent, LOCALE_NEUTRAL,
                         sizeof(Actual), Actual, &Required, &Type);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(Type, DEVPROP_TYPE_STRING);

    Status = SetProperty(Target, &DEVPKEY_Device_InstanceId, LOCALE_NEUTRAL,
                         DEVPROP_TYPE_STRING, sizeof(FriendlyName), (PVOID)FriendlyName);
    ok_eq_hex(Status, STATUS_ACCESS_DENIED);
    Status = SetProperty(Target, &DEVPKEY_Device_PDOName, LOCALE_NEUTRAL,
                         DEVPROP_TYPE_STRING, sizeof(FriendlyName), (PVOID)FriendlyName);
    ok_eq_hex(Status, STATUS_ACCESS_DENIED);

    Status = GetProperty(Target, &DEVPKEY_Device_FriendlyName, LOCALE_NEUTRAL,
                         sizeof(Saved), Saved, &SavedSize, &Type);
    HadFriendlyName = NT_SUCCESS(Status);
    ok(HadFriendlyName || (Status == STATUS_OBJECT_NAME_NOT_FOUND),
       "Unexpected status 0x%08lx reading the friendly name\n", Status);

    Status = SetProperty(Target, &DEVPKEY_Device_FriendlyName, LOCALE_NEUTRAL,
                         DEVPROP_TYPE_STRING, sizeof(FriendlyName), (PVOID)FriendlyName);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ExpectProperty(Target, &DEVPKEY_Device_FriendlyName, LOCALE_NEUTRAL,
                   DEVPROP_TYPE_STRING, (PVOID)FriendlyName, sizeof(FriendlyName));

    Status = IoGetDeviceProperty(Target->Pdo, DevicePropertyFriendlyName,
                                 sizeof(Expected), Expected, &ExpectedLength);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
        ok_eq_wstr(Expected, FriendlyName);

    Number = 1;
    Status = SetProperty(Target, &DEVPKEY_Device_FriendlyName, LOCALE_NEUTRAL,
                         DEVPROP_TYPE_UINT32, sizeof(Number), &Number);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    if (HadFriendlyName)
    {
        Status = SetProperty(Target, &DEVPKEY_Device_FriendlyName, LOCALE_NEUTRAL,
                             DEVPROP_TYPE_STRING, SavedSize, Saved);
        ok_eq_hex(Status, STATUS_SUCCESS);
    }
    else
    {
        Status = SetProperty(Target, &DEVPKEY_Device_FriendlyName, LOCALE_NEUTRAL,
                             DEVPROP_TYPE_EMPTY, 0, NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);
        ExpectNoProperty(Target, &DEVPKEY_Device_FriendlyName, LOCALE_NEUTRAL);
    }
}

static
VOID
Test_InterfaceBuiltins(
    _In_ PPROPERTY_TARGET Target,
    _In_ const GUID *InterfaceClass)
{
    DEVPROP_BOOLEAN Enabled = 0x55;
    WCHAR InstanceId[256];
    ULONG Required;
    DEVPROPTYPE Type;
    GUID ClassGuid;
    NTSTATUS Status;

    trace("Interface builtins\n");

    Status = GetProperty(Target, &DEVPKEY_DeviceInterface_ClassGuid, LOCALE_NEUTRAL,
                         sizeof(ClassGuid), &ClassGuid, &Required, &Type);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(Type, DEVPROP_TYPE_GUID);
    ok_eq_ulong(Required, (ULONG)sizeof(GUID));
    ok(RtlEqualMemory(&ClassGuid, InterfaceClass, sizeof(GUID)), "Wrong interface class\n");

    Status = GetProperty(Target, &DEVPKEY_DeviceInterface_Enabled, LOCALE_NEUTRAL,
                         sizeof(Enabled), &Enabled, &Required, &Type);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(Type, DEVPROP_TYPE_BOOLEAN);
    ok_eq_int(Enabled, DEVPROP_TRUE);

    Status = GetProperty(Target, &DEVPKEY_Device_InstanceId, LOCALE_NEUTRAL,
                         sizeof(InstanceId), InstanceId, &Required, &Type);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(Type, DEVPROP_TYPE_STRING);

    Status = SetProperty(Target, &DEVPKEY_DeviceInterface_ClassGuid, LOCALE_NEUTRAL,
                         DEVPROP_TYPE_GUID, sizeof(ClassGuid), &ClassGuid);
    ok_eq_hex(Status, STATUS_ACCESS_DENIED);
}

static
VOID
Test_InterfaceParameters(VOID)
{
    UNICODE_STRING EmptyName = RTL_CONSTANT_STRING(L"");
    UNICODE_STRING MissingName = RTL_CONSTANT_STRING(L"\\??\\KMTEST#NoSuchDevice#0#{5F1E3C8A-7D2B-4E61-9A0C-3B8D4F2E6A17}");
    ULONG Number = 1, Required;
    DEVPROPTYPE Type;
    NTSTATUS Status;

    trace("Interface parameter checks\n");

    Status = IoSetDeviceInterfacePropertyData(NULL, &TestKeyNumber, LOCALE_NEUTRAL, 0,
                                              DEVPROP_TYPE_UINT32, sizeof(Number), &Number);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    Status = IoGetDeviceInterfacePropertyData(NULL, &TestKeyNumber, LOCALE_NEUTRAL, 0,
                                              sizeof(Number), &Number, &Required, &Type);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    Status = IoSetDeviceInterfacePropertyData(&EmptyName, &TestKeyNumber, LOCALE_NEUTRAL, 0,
                                              DEVPROP_TYPE_UINT32, sizeof(Number), &Number);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    Status = IoGetDeviceInterfacePropertyData(&EmptyName, &TestKeyNumber, LOCALE_NEUTRAL, 0,
                                              sizeof(Number), &Number, &Required, &Type);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    Status = IoSetDeviceInterfacePropertyData(&MissingName, &TestKeyNumber, LOCALE_NEUTRAL, 0,
                                              DEVPROP_TYPE_UINT32, sizeof(Number), &Number);
    ok(!NT_SUCCESS(Status), "Setting a property of a missing interface returned 0x%08lx\n", Status);
    Status = IoGetDeviceInterfacePropertyData(&MissingName, &TestKeyNumber, LOCALE_NEUTRAL, 0,
                                              sizeof(Number), &Number, &Required, &Type);
    ok(!NT_SUCCESS(Status), "Getting a property of a missing interface returned 0x%08lx\n", Status);
}

static
PDEVICE_OBJECT
ReferenceInterfacePdo(
    _In_ PUNICODE_STRING SymbolicLinkName)
{
    WCHAR NameBuffer[256];
    PDEVICE_OBJECT TopDevice, DeviceObject;
    PFILE_OBJECT FileObject;
    ULONG Length;
    NTSTATUS Status;

    Status = IoGetDeviceObjectPointer(SymbolicLinkName,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &TopDevice);
    if (!NT_SUCCESS(Status))
    {
        trace("Cannot open %wZ, status 0x%08lx\n", SymbolicLinkName, Status);
        return NULL;
    }

    DeviceObject = IoGetDeviceAttachmentBaseRef(TopDevice);
    ObDereferenceObject(FileObject);

    Status = IoGetDeviceProperty(DeviceObject,
                                 DevicePropertyPhysicalDeviceObjectName,
                                 sizeof(NameBuffer),
                                 NameBuffer,
                                 &Length);
    if (!NT_SUCCESS(Status) && (Status != STATUS_BUFFER_TOO_SMALL))
    {
        ObDereferenceObject(DeviceObject);
        return NULL;
    }

    return DeviceObject;
}

static
BOOLEAN
FindTestInterface(
    _Out_ PUNICODE_STRING SymbolicLinkName,
    _Out_ PDEVICE_OBJECT *Pdo,
    _Out_ const GUID **InterfaceClass)
{
    PZZWSTR SymbolicLinkList;
    PWSTR SymbolicLink;
    UNICODE_STRING Link;
    ULONG i;
    NTSTATUS Status;

    for (i = 0; i < RTL_NUMBER_OF(CandidateClasses); i++)
    {
        Status = IoGetDeviceInterfaces(CandidateClasses[i], NULL, 0, &SymbolicLinkList);
        if (!NT_SUCCESS(Status))
            continue;

        for (SymbolicLink = SymbolicLinkList;
             *SymbolicLink != UNICODE_NULL;
             SymbolicLink += wcslen(SymbolicLink) + 1)
        {
            RtlInitUnicodeString(&Link, SymbolicLink);

            *Pdo = ReferenceInterfacePdo(&Link);
            if (!*Pdo)
                continue;

            if (!RtlCreateUnicodeString(SymbolicLinkName, SymbolicLink))
            {
                ObDereferenceObject(*Pdo);
                continue;
            }

            *InterfaceClass = CandidateClasses[i];
            ExFreePool(SymbolicLinkList);
            return TRUE;
        }

        ExFreePool(SymbolicLinkList);
    }

    return FALSE;
}

static
PDEVICE_OBJECT
GetRootPdo(
    _In_ PDEVICE_OBJECT Pdo)
{
    PEXTENDED_DEVOBJ_EXTENSION Extension = (PEXTENDED_DEVOBJ_EXTENSION)Pdo->DeviceObjectExtension;
    PDEVICE_NODE DeviceNode = Extension->DeviceNode;

    if (!DeviceNode)
        return NULL;

    while (DeviceNode->Parent)
        DeviceNode = DeviceNode->Parent;

    return DeviceNode->PhysicalDeviceObject;
}

static
VOID
Test_RootDevice(
    _In_ PDEVICE_OBJECT Pdo)
{
    static const WCHAR English[] = L"Root";
    PROPERTY_TARGET Target = { "root", NULL, NULL };
    ULONG Number = 1;
    NTSTATUS Status;

    trace("Root device\n");

    Target.Pdo = GetRootPdo(Pdo);
    if (skip(Target.Pdo != NULL, "No root device\n"))
        return;

    Status = SetProperty(&Target, &TestKeyNumber, LOCALE_NEUTRAL, DEVPROP_TYPE_UINT32, sizeof(Number), &Number);
    ok_eq_hex(Status, STATUS_ACCESS_DENIED);
    ExpectNoProperty(&Target, &TestKeyNumber, LOCALE_NEUTRAL);

    Status = SetProperty(&Target, &DEVPKEY_Device_FriendlyName, LOCALE_NEUTRAL, DEVPROP_TYPE_STRING,
                         sizeof(English), (PVOID)English);
    ok_eq_hex(Status, STATUS_ACCESS_DENIED);

    Status = SetProperty(&Target, &TestKeyNumber, LOCALE_NEUTRAL, DEVPROP_TYPE_EMPTY, 0, NULL);
    ok_eq_hex(Status, STATUS_ACCESS_DENIED);

    Status = SetProperty(&Target, &TestKeyLocalized, LCID_EN_US, DEVPROP_TYPE_STRING,
                         sizeof(English), (PVOID)English);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ExpectProperty(&Target, &TestKeyLocalized, LCID_EN_US, DEVPROP_TYPE_STRING, (PVOID)English, sizeof(English));
    ExpectNoProperty(&Target, &TestKeyLocalized, LOCALE_NEUTRAL);

    Status = SetProperty(&Target, &TestKeyLocalized, LCID_EN_US, DEVPROP_TYPE_EMPTY, 0, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ExpectNoProperty(&Target, &TestKeyLocalized, LCID_EN_US);
}

START_TEST(IoDeviceProperty)
{
    PROPERTY_TARGET DeviceTarget = { "device", NULL, NULL };
    PROPERTY_TARGET InterfaceTarget = { "interface", NULL, NULL };
    UNICODE_STRING SymbolicLinkName;
    const GUID *InterfaceClass;
    PDEVICE_OBJECT Pdo;

    Test_InterfaceParameters();

    if (skip(FindTestInterface(&SymbolicLinkName, &Pdo, &InterfaceClass),
             "No device interface with a PnP PDO found\n"))
    {
        return;
    }

    trace("Using %wZ\n", &SymbolicLinkName);

    DeviceTarget.Pdo = Pdo;
    InterfaceTarget.SymbolicLinkName = &SymbolicLinkName;

    Test_StoreRoundTrip(&DeviceTarget);
    Test_Locales(&DeviceTarget);
    Test_Validation(&DeviceTarget);
    Test_DeviceBuiltins(&DeviceTarget);
    Test_RootDevice(Pdo);

    Test_StoreRoundTrip(&InterfaceTarget);
    Test_Locales(&InterfaceTarget);
    Test_Validation(&InterfaceTarget);
    Test_InterfaceBuiltins(&InterfaceTarget, InterfaceClass);

    RtlFreeUnicodeString(&SymbolicLinkName);
    ObDereferenceObject(Pdo);
}

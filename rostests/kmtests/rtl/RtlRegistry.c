/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for RtlQueryRegistryValues
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#define KMT_EMULATE_KERNEL
#include <kmt_test.h>

#ifndef RTL_NUMBER_OF
#define RTL_NUMBER_OF(x) (sizeof(x) / sizeof(x[0]))
#endif

typedef struct
{
    PCWSTR ValueName;
    ULONG ValueType;
    PVOID ValueData;
    ULONG ValueLength;
} EXPECTED_VALUE, *PEXPECTED_VALUE;

typedef struct
{
    ULONG Count;
    ULONG CurrentIndex;
    EXPECTED_VALUE Values[20];
} EXPECTED_VALUES, *PEXPECTED_VALUES;

//static RTL_QUERY_REGISTRY_ROUTINE QueryRoutine;
static
NTSTATUS
NTAPI
QueryRoutine(
    _In_ PWSTR ValueName,
    _In_ ULONG ValueType,
    _In_ PVOID ValueData,
    _In_ ULONG ValueLength,
    _In_ PVOID Context,
    _In_ PVOID EntryContext)
{
    PEXPECTED_VALUES ExpectedValues = Context;
    PEXPECTED_VALUE Expected;
    SIZE_T EqualBytes;

    ok(ExpectedValues->CurrentIndex < ExpectedValues->Count,
       "Call number %lu, expected only %lu\n",
       ExpectedValues->CurrentIndex, ExpectedValues->Count);
    if (!skip(ExpectedValues->CurrentIndex < ExpectedValues->Count, "Out of bounds\n"))
    {
        Expected = &ExpectedValues->Values[ExpectedValues->CurrentIndex];
        if (EntryContext)
            ok_eq_pointer(EntryContext, Expected);
        ok_eq_wstr(ValueName, Expected->ValueName);
        ok_eq_ulong(ValueType, Expected->ValueType);
        ok_eq_ulong(ValueLength, Expected->ValueLength);
        EqualBytes = RtlCompareMemory(ValueData,
                                      Expected->ValueData,
                                      min(ValueLength, Expected->ValueLength));
        ok_eq_size(EqualBytes, Expected->ValueLength);
    }

    ExpectedValues->CurrentIndex++;
    return STATUS_SUCCESS;
}

static
VOID
TestRtlQueryRegistryValues(
    _In_ HANDLE KeyHandle)
{
    NTSTATUS Status;
    UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"TestValue");
    RTL_QUERY_REGISTRY_TABLE QueryTable[] =
    {
        { QueryRoutine, 0, L"TestValue", NULL, REG_NONE, NULL, 0 },
        { NULL }
    };
    EXPECTED_VALUES Expected;
    typedef struct
    {
        PWSTR Str;
        ULONG Len;
    } STR_AND_LEN;
#define CONST_STR_AND_LEN(d) { (d), sizeof(d) }
#define CSAL CONST_STR_AND_LEN

#define NO_AUTO_LEN     1
#define NO_DEFAULT      2
#define AUTO_DIFFERS    4
#define DEFAULT_DIFFERS 8
    struct
    {
        STR_AND_LEN Value;
        ULONG ExpectedCount;
        STR_AND_LEN Expected[20];
        ULONG Flags;
        ULONG DefaultExpectedCount;
        STR_AND_LEN DefaultExpected[20];

    } Tests[] =
    {
        { { NULL, 0 },                      0, { { NULL, 0 } },                                     NO_AUTO_LEN | NO_DEFAULT },
        { CSAL(L""),                        0, { { NULL, 0 } },                                     NO_AUTO_LEN },
        { CSAL(L"\0"),                      1, { CSAL(L"") },
          AUTO_DIFFERS | DEFAULT_DIFFERS,   0, { { NULL, 0 } } },
        { CSAL(L"String"),                  1, { CSAL(L"String") },                                 NO_AUTO_LEN },
        { CSAL(L"String\0"),                1, { CSAL(L"String") }                                              },
        { CSAL(L"String1\0String2"),        2, { CSAL(L"String1"), CSAL(L"String2") },              NO_AUTO_LEN },
        { CSAL(L"String1\0String2\0"),      2, { CSAL(L"String1"), CSAL(L"String2") }                           },
        { CSAL(L"String1\0\0String3"),      3, { CSAL(L"String1"), CSAL(L""), CSAL(L"String3") },   NO_AUTO_LEN },
        { CSAL(L"String1\0\0String3\0"),    3, { CSAL(L"String1"), CSAL(L""), CSAL(L"String3") },
          AUTO_DIFFERS,                     1, { CSAL(L"String1") } },
    };

#define DO_QUERY(ExpectedArray, ExpectedCount) do                       \
    {                                                                   \
        ULONG _i;                                                       \
        ULONG _ExpectedCount = (ExpectedCount);                         \
        for (_i = 0; _i < _ExpectedCount; _i++)                         \
        {                                                               \
            Expected.Values[_i].ValueName = ValueName.Buffer;           \
            Expected.Values[_i].ValueType = REG_SZ;                     \
            Expected.Values[_i].ValueData = (ExpectedArray)[_i].Str;    \
            Expected.Values[_i].ValueLength = (ExpectedArray)[_i].Len;  \
        }                                                               \
        Expected.CurrentIndex = 0;                                      \
        Expected.Count = _ExpectedCount;                                \
        if (_ExpectedCount == 1)                                        \
            QueryTable[0].EntryContext = &Expected.Values[0];           \
        else                                                            \
            QueryTable[0].EntryContext = NULL;                          \
        Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,            \
                                        (PCWSTR)KeyHandle,              \
                                        QueryTable,                     \
                                        &Expected,                      \
                                        NULL);                          \
        ok_eq_hex(Status, STATUS_SUCCESS);                              \
        ok_eq_ulong(Expected.CurrentIndex, Expected.Count);             \
    } while(0)

    ULONG TestCount = RTL_NUMBER_OF(Tests);
    ULONG i;

    for (i = 0; i < TestCount; i++)
    {
        trace("Set: %lu\n", i);
        Status = ZwSetValueKey(KeyHandle,
                               &ValueName,
                               0,
                               REG_MULTI_SZ,
                               Tests[i].Value.Str,
                               Tests[i].Value.Len);
        ok_eq_hex(Status, STATUS_SUCCESS);

        DO_QUERY(Tests[i].Expected, Tests[i].ExpectedCount);
    }

    /* Delete value to test default values */
    Status = ZwDeleteValueKey(KeyHandle, &ValueName);
    ok(Status == STATUS_SUCCESS || Status == STATUS_OBJECT_NAME_NOT_FOUND,
       "ZwDeleteValueKey returned %lx\n", Status);

    /* Default: REG_NONE */
    DO_QUERY((STR_AND_LEN *)NULL, 0);

    for (i = 0; i < TestCount; i++)
    {
        if (Tests[i].Flags & NO_DEFAULT)
            continue;
        trace("Default: %lu\n", i);
        QueryTable[0].DefaultType = REG_MULTI_SZ;
        QueryTable[0].DefaultData = Tests[i].Value.Str;
        QueryTable[0].DefaultLength = Tests[i].Value.Len;

        if (Tests[i].Flags & DEFAULT_DIFFERS)
            DO_QUERY(Tests[i].DefaultExpected, Tests[i].DefaultExpectedCount);
        else
            DO_QUERY(Tests[i].Expected, Tests[i].ExpectedCount);
    }

    for (i = 0; i < TestCount; i++)
    {
        if (Tests[i].Flags & NO_AUTO_LEN)
            continue;
        trace("Auto: %lu\n", i);
        QueryTable[0].DefaultType = REG_MULTI_SZ;
        QueryTable[0].DefaultData = Tests[i].Value.Str;
        QueryTable[0].DefaultLength = 0;

        if (Tests[i].Flags & AUTO_DIFFERS)
            DO_QUERY(Tests[i].DefaultExpected, Tests[i].DefaultExpectedCount);
        else
            DO_QUERY(Tests[i].Expected, Tests[i].ExpectedCount);
    }
}

START_TEST(RtlRegistry)
{
    NTSTATUS Status;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE SoftwareHandle;
    HANDLE KeyHandle;

    RtlInitUnicodeString(&KeyName, L"\\Registry\\MACHINE\\Software");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwOpenKey(&SoftwareHandle,
                       KEY_CREATE_SUB_KEY,
                       &ObjectAttributes);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (skip(NT_SUCCESS(Status) && SoftwareHandle != NULL, "No software key\n"))
        return;

    RtlInitUnicodeString(&KeyName, L"RtlRegistryKmtestKey");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               SoftwareHandle,
                               NULL);
    Status = ZwCreateKey(&KeyHandle,
                         KEY_QUERY_VALUE | KEY_SET_VALUE | DELETE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);

    if (!skip(NT_SUCCESS(Status) && KeyHandle != NULL, "No test key\n"))
    {
        TestRtlQueryRegistryValues(KeyHandle);

        Status = ZwDeleteKey(KeyHandle);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = ZwClose(KeyHandle);
        ok_eq_hex(Status, STATUS_SUCCESS);
    }
}

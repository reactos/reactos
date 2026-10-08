/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for RtlQueryRegistryValues default values
 * COPYRIGHT:   Copyright 2026 Tomas Srnka (tomas.srnka@e2b.dev)
 */

#include "precomp.h"
#include <winreg.h>
#include <pseh/pseh2.h>

#define KEY_PATH L"\\Registry\\Machine\\Software"
#define MISSING_VALUE L"ReactOS_apitest_RtlQueryRegistryValues_Missing"

typedef struct _QUERY_RESULT
{
    ULONG Calls;
    ULONG Type;
    ULONG Length;
    WCHAR Text[16];
} QUERY_RESULT, *PQUERY_RESULT;

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
    PQUERY_RESULT Result = EntryContext;

    Result->Calls++;
    Result->Type = ValueType;
    Result->Length = ValueLength;
    if (ValueData && ValueLength <= sizeof(Result->Text))
        RtlCopyMemory(Result->Text, ValueData, ValueLength);
    return STATUS_SUCCESS;
}

/* Query a value that does not exist, so that the table's default is used */
static
NTSTATUS
QueryDefault(
    _In_ ULONG Type,
    _In_opt_ PVOID DefaultData,
    _Out_ PBOOLEAN Faulted,
    _Out_ PQUERY_RESULT Result)
{
    RTL_QUERY_REGISTRY_TABLE Table[2];
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    RtlZeroMemory(Table, sizeof(Table));
    RtlZeroMemory(Result, sizeof(*Result));
    Table[0].QueryRoutine = QueryRoutine;
    Table[0].Name = MISSING_VALUE;
    Table[0].EntryContext = Result;
    Table[0].DefaultType = Type;
    Table[0].DefaultData = DefaultData;
    Table[0].DefaultLength = 0;

    *Faulted = FALSE;
    _SEH2_TRY
    {
        Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, KEY_PATH, Table, NULL, NULL);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        *Faulted = TRUE;
    }
    _SEH2_END;

    return Status;
}

START_TEST(RtlQueryRegistryValues)
{
    static const ULONG Types[] = { REG_SZ, REG_EXPAND_SZ, REG_MULTI_SZ };
    static const WCHAR Default[] = L"Default\0";
    QUERY_RESULT Result;
    NTSTATUS Status;
    BOOLEAN Faulted;
    ULONG i;

    for (i = 0; i < _countof(Types); i++)
    {
        /* A string default with DefaultLength 0 is measured */
        Status = QueryDefault(Types[i], (PVOID)Default, &Faulted, &Result);
        ok(!Faulted, "Type %lu: valid default faulted\n", Types[i]);
        ok_hex(Status, STATUS_SUCCESS);
        ok(Result.Calls == 1, "Type %lu: %lu calls\n", Types[i], Result.Calls);
        ok(Result.Type == (Types[i] == REG_EXPAND_SZ ? REG_EXPAND_SZ : REG_SZ),
           "Type %lu: got type %lu\n", Types[i], Result.Type);
        ok(Result.Length == sizeof(L"Default"), "Type %lu: got length %lu\n", Types[i], Result.Length);
        ok(!wcscmp(Result.Text, L"Default"), "Type %lu: got '%ls'\n", Types[i], Result.Text);

        /* Without data there is nothing to measure: the query fails, no fault */
        Status = QueryDefault(Types[i], NULL, &Faulted, &Result);
        ok(!Faulted, "Type %lu: NULL default faulted\n", Types[i]);
        if (!Faulted)
            ok_hex(Status, STATUS_DATA_OVERRUN);
        ok(Result.Calls == 0, "Type %lu: %lu calls\n", Types[i], Result.Calls);
    }
}

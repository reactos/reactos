/*
 * PROJECT:     ReactOS Spooler Router API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for PackStrings
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <spoolss.h>

typedef struct _EXAMPLE_STRUCT
{
    PWSTR String1;
    PWSTR String2;
}
EXAMPLE_STRUCT, *PEXAMPLE_STRUCT;

START_TEST(PackStrings)
{
    PCWSTR Source1[] = { L"Test", L"String" };
    PCWSTR Source2[] = { L"Test", NULL };

    BYTE Buffer[50];
    PBYTE pEnd;
    PEXAMPLE_STRUCT pStruct = (PEXAMPLE_STRUCT)Buffer;
    DWORD Offsets[] = {
        FIELD_OFFSET(EXAMPLE_STRUCT, String1),
        FIELD_OFFSET(EXAMPLE_STRUCT, String2),
        MAXDWORD
    };

    // Try a usual case with two strings. Verify that they are copied in reverse order.
    pEnd = PackStrings(Source1, Buffer, Offsets, &Buffer[sizeof(Buffer)]);
    ok(wcscmp(pStruct->String1, Source1[0]) == 0, "String1 and Source1[0] don't match!\n");
    ok(wcscmp(pStruct->String2, Source1[1]) == 0, "String2 and Source1[1] don't match!\n");
    ok(wcscmp((PWSTR)pEnd, Source1[1]) == 0, "pEnd and Source1[1] don't match!\n");

    // Now verify that the corresponding pointer is set to NULL if a string is NULL.
    pEnd = PackStrings(Source2, Buffer, Offsets, &Buffer[sizeof(Buffer)]);
    ok(wcscmp(pStruct->String1, Source2[0]) == 0, "String1 and Source2[0] don't match!\n");
    ok(!pStruct->String2, "String2 is %p!\n", pStruct->String2);
    ok(wcscmp((PWSTR)pEnd, Source2[0]) == 0, "pEnd and Source2[0] don't match!\n");
}

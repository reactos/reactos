/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    regtest.c

Abstract:

   Test for quick and dirty registry test (very basic)

Author:

    Bryan M. Willman (bryanwi) 30-Apr-1991

Environment:

    User mode.

Revision History:

--*/

#include "stdio.h"
#include "nt.h"

int strlen(PUCHAR);
void main();

VOID DoTest(HANDLE RootKey);

#define MAX_VALUE   256
UCHAR ValueBuffer[MAX_VALUE];

VOID
main()
{
    DbgPrint("Machine\n");
    DoTest((HANDLE)REG_LOCAL_MACHINE);
    DbgPrint("User\n");
    DoTest((HANDLE)REG_LOCAL_USER);
}
VOID
DoTest(
    HANDLE  RootKey
    )
{
    NTSTATUS rc;
    STRING String1;
    UCHAR Value1[] = "This string is value 1.";
    UCHAR Value2[] = "Value 2 is represented by this string.";
    HANDLE Handle1;
    HANDLE Handle2;
    ULONG ValueLength;
    ULONG Type;
    LARGE_INTEGER Time;

    //
    // Create parent node
    //

    DbgPrint("Part1\n");
    RtlInitString(&String1,  "Test1");
    rc = NtCreateKey(
	RootKey,
	&String1,
	0,
	NULL,
	GENERIC_READ|GENERIC_WRITE,
	&Handle1
	);

    if (!NT_SUCCESS(rc)) {
	DbgPrint("1:CreateKey failed rc  = %08lx", rc);
	return;
    }

    //
    // Set data into parent
    //

    DbgPrint("Part2\n");
    rc = NtSetValueKey(
	Handle1,
	1,		// type
	Value1,
	strlen(Value1)
	);

    if (!NT_SUCCESS(rc)) {
	DbgPrint("2:SetValueKey failed rc  = %08lx", rc);
	return;
    }

    //
    // Query and compare data from parent
    //

    DbgPrint("Part2b\n");
    ValueLength = MAX_VALUE;
    rc = NtQueryValueKey(
	Handle1,
	&Type,
	ValueBuffer,
	&ValueLength,
	&Time
	);

    if (!NT_SUCCESS(rc)) {
	DbgPrint("2b:QueryValueKey failed rc  = %08lx", rc);
	return;
    }

    if (ValueLength != (ULONG)strlen(Value1)) {
	DbgPrint("2b1:Wrong value length\n");
	return;
    } else if (RtlCompareMemory(
		ValueBuffer, Value1, ValueLength) != ValueLength) {
	DbgPrint("2b2:Wrong value\n");
	return;
    } else if (Type != 1) {
	DbgPrint("2b3:Wrong type\n");
	return;
    }


    //
    // Close parent
    //

    DbgPrint("Part3\n");
    NtCloseKey(Handle1);

    if (!NT_SUCCESS(rc)) {
	DbgPrint("3:CloseKey failed rc  = %08lx", rc);
	return;
    }


    //
    // Reopen parent
    //

    DbgPrint("Part4\n");
    rc = NtOpenKey(
	RootKey,
	&String1,
	0,
	GENERIC_READ|GENERIC_WRITE,
	&Handle1
	);

    if (!NT_SUCCESS(rc)) {
	DbgPrint("4:OpenKey failed rc  = %08lx", rc);
	return;
    }

    //
    // Create child
    //

    DbgPrint("Part5\n");
    RtlInitString(&String1,  "Test2");
    rc = NtCreateKey(
	Handle1,
	&String1,
	0,
	NULL,
	GENERIC_READ|GENERIC_WRITE,
	&Handle2
	);

    if (!NT_SUCCESS(rc)) {
	DbgPrint("5:CreateKey failed rc  = %08lx", rc);
	return;
    }


    //
    // Set data into child
    //

    DbgPrint("Part6\n");
    rc = NtSetValueKey(
	Handle2,
	2,		// type
	Value2,
	strlen(Value2)
	);

    if (!NT_SUCCESS(rc)) {
	DbgPrint("6:SetValueKey failed rc  = %08lx", rc);
	return;
    }


    //
    // Query and compare data from child
    //

    DbgPrint("Part7\n");
    ValueLength = MAX_VALUE;
    rc = NtQueryValueKey(
	Handle2,
	&Type,
	ValueBuffer,
	&ValueLength,
	&Time
	);

    if (!NT_SUCCESS(rc)) {
	DbgPrint("7:QueryValueKey failed rc  = %08lx", rc);
	return;
    }

    if (ValueLength != (ULONG)strlen(Value2)) {
	DbgPrint("7.1:Wrong value length\n");
	return;
    } else if (RtlCompareMemory(
		ValueBuffer, Value2, ValueLength) != ValueLength) {
	DbgPrint("7.2:Wrong value\n");
	return;
    } else if (Type != 2) {
	DbgPrint("7.3:Wrong type\n");
	return;
    }


    //
    // Query and compare data from parent again
    //

    DbgPrint("Part8\n");
    ValueLength = MAX_VALUE;
    rc = NtQueryValueKey(
	Handle1,
	&Type,
	ValueBuffer,
	&ValueLength,
	&Time
	);

    if (!NT_SUCCESS(rc)) {
	DbgPrint("8:QueryValueKey failed rc  = %08lx", rc);
	return;
    }

    if (ValueLength != (ULONG)strlen(Value1)) {
	DbgPrint("8.1:Wrong value length\n");
	return;
    } else if (RtlCompareMemory(
		ValueBuffer, Value1, ValueLength) != ValueLength) {
	DbgPrint("8.2:Wrong value\n");
	return;
    } else if (Type != 1) {
	DbgPrint("8.3:Wrong type\n");
	return;
    }


    //
    // Reset parent data
    //

    DbgPrint("Part9\n");
    rc = NtSetValueKey(
	Handle1,
	1,		// type
	Value2,
	strlen(Value2)
	);

    if (!NT_SUCCESS(rc)) {
	DbgPrint("9:SetValueKey failed rc  = %08lx", rc);
	return;
    }


    //
    // Query and compare reset data
    //

    DbgPrint("Part10\n");
    ValueLength = MAX_VALUE;
    rc = NtQueryValueKey(
	Handle1,
	&Type,
	ValueBuffer,
	&ValueLength,
	&Time
	);

    if (!NT_SUCCESS(rc)) {
	DbgPrint("10:QueryValueKey failed rc  = %08lx", rc);
	return;
    }

    if (ValueLength != (ULONG)strlen(Value2)) {
	DbgPrint("10.1:Wrong value length\n");
	return;
    } else if (RtlCompareMemory(
		ValueBuffer, Value2, ValueLength) != ValueLength) {
	DbgPrint("10.2:Wrong value\n");
	return;
    } else if (Type != 1) {
	DbgPrint("10.3:Wrong type\n");
	return;
    }

    //
    // Close off handles and return
    //

    DbgPrint("Part11\n");
    rc = NtCloseKey(Handle1);

    if (!NT_SUCCESS(rc)) {
	DbgPrint("11:CloseKey failed rc  = %08lx", rc);
	return;
    }

    DbgPrint("Part12\n");
    rc = NtCloseKey(Handle2);
    if (!NT_SUCCESS(rc)) {
	DbgPrint("12:CloseKey failed rc  = %08lx", rc);
	return;
    }

    return;
}

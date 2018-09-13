/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    rtmisc1.c

Abstract:

    NT level registry api test program #1, basic non-error paths.

    Test open, create, setvalue, queryvalue, enumeratekey, enumeratevalue,
    querykey.

Author:

    Bryan Willman (bryanwi)  19-Nov-91

Revision History:

--*/

#include "cmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
//  NOTE:   This version of the test operations on \REGISTRY\MACHINE\TEST\*,
//          which is the TEST hive.  This hive will not exist in production
//          systems, so the test will have to change.
//


void __cdecl main(int, char *);

VOID
NameClassAndTitle(
    KEY_NODE_INFORMATION    *NodeInformation,
    UNICODE_STRING          ClassName,
    ULONG                   TitleIndex,
    UNICODE_STRING          KeyName,
    LARGE_INTEGER           CompTime,
    BOOLEAN                 Strong,         // time must be >= CompTime
    PUCHAR                  TestName
    );

VOID
expectstring(
    PWSTR   expected,
    ULONG   expectedlength,
    PWSTR   actual,
    ULONG   actuallength
    );

VOID
expecttime(
    LARGE_INTEGER   ExpectTime,
    LARGE_INTEGER   ActualTime
    );


#define TITLE_INDEX_1   122259
#define TITLE_INDEX_2   120858
#define TITLE_INDEX_3   120159
#define TYPE_1          666
#define TYPE_2          1066


ULONG           failure = 0;


void
__cdecl main(int, char *)
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING  KeyName;
    UNICODE_STRING  KeyName2;
    UNICODE_STRING  ClassName;
    UNICODE_STRING  ClassName2;
    UNICODE_STRING  ValueName;
    UNICODE_STRING  ValueName2;
    HANDLE          BaseHandle;
    HANDLE          Testhand1;
    ULONG           Disposition;
    LARGE_INTEGER   CompTime;
    ULONG           buffer[100];
    ULONG           bufsize = sizeof(ULONG) * 100;
    PKEY_NODE_INFORMATION NodeInformation;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;
    PKEY_VALUE_BASIC_INFORMATION KeyValueBasic;
    ULONG           ResultLength;
    PUCHAR          datastring = "Some simple ascii data for use as a value";
    PUCHAR          datastring2 = "Some more not so simple data $#";
    ULONG           expected;
    PVOID           tp;


    printf("rtmisc1: starting\n");

    NodeInformation = (PKEY_NODE_INFORMATION)&(buffer[0]);
    KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)&(buffer[0]);
    KeyValueBasic = (PKEY_VALUE_BASIC_INFORMATION)&(buffer[0]);

    //
    // t0: Perform all operations against a base node, open it here.
    //

    RtlInitUnicodeString(
        &KeyName,
        L"\\REGISTRY\\MACHINE\\TEST"
        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        0,
        (HANDLE)NULL,
        NULL
        );
    ObjectAttributes.Attributes |= OBJ_CASE_INSENSITIVE;

    status = NtOpenKey(
                &BaseHandle,
                MAXIMUM_ALLOWED,
                &ObjectAttributes
                );
    if (!NT_SUCCESS(status)) {
        printf("rtmisc1: t0: %08lx\n", status);
        goto punt;
    }


    //
    // t1: Create a key with class and title index
    //

    RtlInitUnicodeString(
        &ClassName,
        L"t1 Class Name"
        );

    RtlInitUnicodeString(
        &KeyName,
        L"first_test_node"
        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        0,
        BaseHandle,
        NULL
        );
    ObjectAttributes.Attributes |= OBJ_CASE_INSENSITIVE;

    NtQuerySystemTime(&CompTime);

//  printf("ClassName@%08lx  KeyName@%08lx\n",
//          ClassName.Buffer, KeyName.Buffer);

    status = NtCreateKey(
                &Testhand1,
                MAXIMUM_ALLOWED,
                &ObjectAttributes,
                TITLE_INDEX_1,
                &ClassName,
                0,
                &Disposition
                );
    if (!NT_SUCCESS(status)) {
        printf("rtmisc1: t1: %08lx\n", status);
        goto punt;
    }

    if (Disposition != REG_CREATED_NEW_KEY) {
        printf("rtmisc1: t1a: got old key, expected to create new one\n");
        failure++;
    }

    //
    // t2: See if we can get data back, and if it makes sense
    //

    RtlZeroMemory(NodeInformation, bufsize);
    status = NtQueryKey(
                Testhand1,
                KeyNodeInformation,
                NodeInformation,
                bufsize,
                &ResultLength
                );
    if (!NT_SUCCESS(status)) {
        printf("rtmisc1: t2a: %08lx\n", status);
        goto punt;
    }
    if (ResultLength != 80) {
        printf("rtmisc1: t2i: expect 80, ResultLength = %d\n", ResultLength);
        failure++;
    }


    NameClassAndTitle(
        NodeInformation,
        ClassName,
        TITLE_INDEX_1,
        KeyName,
        CompTime,
        FALSE,          // time must be >= CompTime
        "rtmisc1: t2b: "
        );
    CompTime = NodeInformation->LastWriteTime;

    status = NtClose(Testhand1);
    if (!NT_SUCCESS(status)) {
        printf("rtmisc1: t2c: %08lx\n");
        goto punt;
    }


    //
    // t3: Reopen the key with create, see if data still there.
    //

    status = NtCreateKey(
                &Testhand1,
                MAXIMUM_ALLOWED,
                &ObjectAttributes,
                TITLE_INDEX_1,
                &ClassName,
                0,
                &Disposition
                );
    if (!NT_SUCCESS(status)) {
        printf("rtmisc1: t3: %08lx\n", status);
        goto punt;
    }

    if (Disposition != REG_OPENED_EXISTING_KEY) {
        printf("rtmisc1: t3a failure\n");
        failure++;
    }

    RtlZeroMemory(NodeInformation, bufsize);
    status = NtQueryKey(
                Testhand1,
                KeyNodeInformation,
                NodeInformation,
                bufsize,
                &ResultLength
                );
    if (!NT_SUCCESS(status)) {
        printf("rtmisc1: t3b: %08lx\n", status);
        goto punt;
    }

    NameClassAndTitle(
        NodeInformation,
        ClassName,
        TITLE_INDEX_1,
        KeyName,
        CompTime,
        FALSE,          // time must be >= CompTime
        "rtmisc1: t3c: "
        );

    status = NtClose(Testhand1);
    if (!NT_SUCCESS(status)) {
        printf("rtmisc1: t3d: %08lx\n");
        goto punt;
    }


    //
    // t4: Reopen the key with open, see if data still there.
    //

    status = NtOpenKey(
                &Testhand1,
                MAXIMUM_ALLOWED,
                &ObjectAttributes
                );
    if (!NT_SUCCESS(status)) {
        printf("rtmisc1: t4: %08lx\n", status);
        goto punt;
    }

    RtlZeroMemory(NodeInformation, bufsize);
    status = NtQueryKey(
                Testhand1,
                KeyNodeInformation,
                NodeInformation,
                bufsize,
                &ResultLength
                );
    if (!NT_SUCCESS(status)) {
        printf("rtmisc1: t4a: %08lx\n", status);
        goto punt;
    }

    NameClassAndTitle(
        NodeInformation,
        ClassName,
        TITLE_INDEX_1,
        KeyName,
        CompTime,
        FALSE,          // time must be >= CompTime
        "rtmisc1: t4b: "
        );

//  status = NtClose(Testhand1);
//  if (!NT_SUCCESS(status)) {
//      printf("rtmisc1: t4c: %08lx\n");
//      exit(1);
//  }


    //
    // t5: Create a value
    //

    RtlInitUnicodeString(
        &ValueName,
        L"the very first value stored in the registry"
        );


    status = NtSetValueKey(
                Testhand1,
                &ValueName,
                TITLE_INDEX_2,
                TYPE_1,
                datastring,
                strlen(datastring)+1
                );
    if (!NT_SUCCESS(status)) {
        printf("rtmisc1: t5: %08lx\n", status);
        failure++;
    }


    //
    // t6: Read the value back
    //

    RtlZeroMemory(KeyValueInformation, bufsize);
    status = NtQueryValueKey(
                Testhand1,
                &ValueName,
                KeyValueFullInformation,
                KeyValueInformation,
                bufsize,
                &ResultLength
                );
    if (!NT_SUCCESS(status)) {
        printf("rtmisc1: t6: %08lx\n", status);
        goto punt;
    }
    expected = FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name) +
                    ValueName.Length + strlen(datastring) + 1;
    if (ResultLength != expected) {
        printf("rtmisc1: t6a: expected = %08lx actual = %08lx",
                expected, ResultLength);
        failure++;
    }

    if ( (KeyValueInformation->TitleIndex != TITLE_INDEX_2)         ||
         (KeyValueInformation->Type != TYPE_1)                      ||
         (KeyValueInformation->NameLength != ValueName.Length)      ||
         (KeyValueInformation->DataLength != strlen(datastring)+1))
    {
        printf("rtmisc1: t6b: wrong description data\n");
        failure++;
    }


    tp = (PWSTR)&(KeyValueInformation->Name[0]);
    if (wcsncmp(ValueName.Buffer, tp, (ValueName.Length/sizeof(WCHAR))) != 0) {
        printf("rtmisc1: t6c: wrong name\n");
        expectstring(
            ValueName.Buffer,
            (ValueName.Length/sizeof(WCHAR)),
            (PWSTR)&(KeyValueInformation->Name[0]),
            (KeyValueInformation->NameLength/sizeof(WCHAR))
            );
        failure++;
    }


    tp = (PUCHAR)KeyValueInformation + KeyValueInformation->DataOffset;
    if (strcmp(tp, datastring) != 0) {
        printf("rtmisc1: t6d: wrong data\n");
        printf("expected '%s', got '%s'\n", datastring, tp);
        failure++;
    }

    //
    // t7: Create a second value
    //

    RtlInitUnicodeString(
        &ValueName2,
        L"the second value stored in the registry"
        );


    status = NtSetValueKey(
                Testhand1,
                &ValueName2,
                TITLE_INDEX_3,
                TYPE_2,
                datastring2,
                strlen(datastring2)+1
                );
    if (!NT_SUCCESS(status)) {
        printf("rtmisc1: t7: %08lx\n", status);
        failure++;
    }

    //
    // t8: Read the second value back (short form)
    //

    RtlZeroMemory(KeyValueBasic, bufsize);
    status = NtQueryValueKey(
                Testhand1,
                &ValueName2,
                KeyValueBasicInformation,
                KeyValueBasic,
                bufsize,
                &ResultLength
                );
    if (!NT_SUCCESS(status)) {
        printf("rtmisc1: t8: %08lx\n", status);
        goto punt;
    }

    expected = FIELD_OFFSET(KEY_VALUE_BASIC_INFORMATION, Name) +
                    ValueName2.Length;
    if (ResultLength != expected) {
        printf("rtmisc1: t8a: expected = %08lx actual = %08lx",
                expected, ResultLength);
        failure++;
    }

    if ( (KeyValueBasic->TitleIndex != TITLE_INDEX_3)         ||
         (KeyValueBasic->Type != TYPE_2)                      ||
         (KeyValueBasic->NameLength != ValueName2.Length))
    {
        printf("rtmisc1: t8b: wrong description data\n");
        failure++;
    }


    tp = (PWSTR)&(KeyValueBasic->Name[0]);
    if (wcsncmp(ValueName2.Buffer, tp, (ValueName2.Length/sizeof(WCHAR))) != 0) {
        printf("rtmisc1: t8c: wrong name\n");
        expectstring(
            ValueName2.Buffer,
            (ValueName2.Length/sizeof(WCHAR)),
            (PWSTR)&(KeyValueBasic->Name[0]),
            (KeyValueBasic->NameLength/sizeof(WCHAR))
            );
        failure++;
    }


    //
    // t9: Enumerate the values (short form)
    //

    RtlZeroMemory(KeyValueBasic, bufsize);
    status = NtEnumerateValueKey(
                Testhand1,
                0,              // Index
                KeyValueBasicInformation,
                KeyValueBasic,
                bufsize,
                &ResultLength
                );
    if (!NT_SUCCESS(status)) {
        printf("rtmisc1: t9: %08lx\n", status);
        goto punt;
    }

    expected = FIELD_OFFSET(KEY_VALUE_BASIC_INFORMATION, Name) +
                    ValueName.Length;
    if (ResultLength != expected) {
        printf("rtmisc1: t9a: expected = %08lx actual = %08lx",
                expected, ResultLength);
        failure++;
    }

    if (KeyValueBasic->NameLength != ValueName.Length)
    {
        printf("rtmisc1: t9b: wrong description data\n");
        failure++;
    }


    tp = (PWSTR)&(KeyValueBasic->Name[0]);
    if (wcsncmp(ValueName.Buffer, tp, (ValueName.Length/sizeof(WCHAR))) != 0) {
        printf("rtmisc1: t9c: wrong name\n");
        expectstring(
            ValueName.Buffer,
            (ValueName.Length/sizeof(WCHAR)),
            (PWSTR)&(KeyValueBasic->Name[0]),
            (KeyValueBasic->NameLength/sizeof(WCHAR))
            );
        failure++;
    }

    RtlZeroMemory(KeyValueBasic, bufsize);
    status = NtEnumerateValueKey(
                Testhand1,
                1,              // Index
                KeyValueBasicInformation,
                KeyValueBasic,
                bufsize,
                &ResultLength
                );
    if (!NT_SUCCESS(status)) {
        printf("rtmisc1: t9d: %08lx\n", status);
        goto punt;
    }

    expected = FIELD_OFFSET(KEY_VALUE_BASIC_INFORMATION, Name) +
                    ValueName2.Length;
    if (ResultLength != expected) {
        printf("rtmisc1: t9e: expected = %08lx actual = %08lx",
                expected, ResultLength);
        failure++;
    }

    if (KeyValueBasic->NameLength != ValueName2.Length)
    {
        printf("rtmisc1: t9f: wrong description data\n");
        failure++;
    }


    tp = (PWSTR)&(KeyValueBasic->Name[0]);
    if (wcsncmp(ValueName2.Buffer, tp, (ValueName2.Length/sizeof(WCHAR))) != 0) {
        printf("rtmisc1: t9g: wrong name\n");
        expectstring(
            ValueName2.Buffer,
            (ValueName2.Length/sizeof(WCHAR)),
            (PWSTR)&(KeyValueBasic->Name[0]),
            (KeyValueBasic->NameLength/sizeof(WCHAR))
            );
        failure++;
    }

    status = NtEnumerateValueKey(
                Testhand1,
                2,              // Index
                KeyValueBasicInformation,
                KeyValueBasic,
                bufsize,
                &ResultLength
                );
    if (status != STATUS_NO_MORE_ENTRIES) {
        printf("rtmisc1: t9h: %08lx\n", status);
        goto punt;
    }

    //
    // t10: create a second subkey and ennumerate the subkeys
    //

    status = NtClose(Testhand1);
    if (!NT_SUCCESS(status)) {
        printf("rtmisc1: t10a: %08lx\n", status);
        failure++;
    }

    RtlInitUnicodeString(
        &ClassName2,
        L"t2 Class Name"
        );

    RtlInitUnicodeString(
        &KeyName2,
        L"second_test_node"
        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName2,
        0,
        BaseHandle,
        NULL
        );
    ObjectAttributes.Attributes |= OBJ_CASE_INSENSITIVE;

    status = NtCreateKey(
                &Testhand1,
                MAXIMUM_ALLOWED,
                &ObjectAttributes,
                TITLE_INDEX_2,
                &ClassName2,
                0,
                &Disposition
                );
    if (!NT_SUCCESS(status)) {
        printf("rtmisc1: t10b: %08lx\n", status);
        goto punt;
    }

    if (Disposition != REG_CREATED_NEW_KEY) {
        printf("rtmisc1: t10c: got old key, expected to create new one\n");
        failure++;
    }

    //
    // See if we can get data back, and if it makes sense
    //

    RtlZeroMemory(NodeInformation, bufsize);
    status = NtQueryKey(
                Testhand1,
                KeyNodeInformation,
                NodeInformation,
                bufsize,
                &ResultLength
                );
    if (!NT_SUCCESS(status)) {
        printf("rtmisc1: t10d: %08lx\n", status);
        goto punt;
    }
    CompTime = NodeInformation->LastWriteTime;


    NameClassAndTitle(
        NodeInformation,
        ClassName2,
        TITLE_INDEX_2,
        KeyName2,
        CompTime,
        TRUE,
        "rtmisc1: t10e: "
        );

    status = NtClose(Testhand1);
    if (!NT_SUCCESS(status)) {
        printf("rtmisc1: t10f: %08lx\n");
        goto punt;
    }


    RtlZeroMemory(NodeInformation, bufsize);
    status = NtEnumerateKey(
                BaseHandle,
                0,
                KeyNodeInformation,
                NodeInformation,
                bufsize,
                &ResultLength
                );
    if (!NT_SUCCESS(status)) {
        printf("rtmisc1: t10g: %08lx\n", status);
        failure++;
    }
    CompTime = NodeInformation->LastWriteTime;

    NameClassAndTitle(
        NodeInformation,
        ClassName,
        TITLE_INDEX_1,
        KeyName,
        CompTime,
        TRUE,
        "rtmisc1: t10h: "
        );


    RtlZeroMemory(NodeInformation, bufsize);
    status = NtEnumerateKey(
                BaseHandle,
                1,
                KeyNodeInformation,
                NodeInformation,
                bufsize,
                &ResultLength
                );
    if (!NT_SUCCESS(status)) {
        printf("rtmisc1: t10i: %08lx\n", status);
        failure++;
    }
    CompTime = NodeInformation->LastWriteTime;

    NameClassAndTitle(
        NodeInformation,
        ClassName2,
        TITLE_INDEX_2,
        KeyName2,
        CompTime,
        TRUE,
        "rtmisc1: t10j: "
        );


    status = NtEnumerateKey(
                BaseHandle,
                2,
                KeyNodeInformation,
                NodeInformation,
                bufsize,
                &ResultLength
                );
    if (status != STATUS_NO_MORE_ENTRIES) {
        printf("rtmisc1: t10k: %08lx\n", status);
        failure++;
    }

    //
    // Summary report
    //

    if (!failure) {
        printf("rtmisc1: success");
        exit(0);
    } else {
        printf("rtmisc1: failed, %d failures\n", failure);
        exit(1);
    }

punt:
    failure++;
    printf("rtmisc1: failed, %d failures\n", failure);
    exit(1);
}



VOID
NameClassAndTitle(
    KEY_NODE_INFORMATION    *NodeInformation,
    UNICODE_STRING          ClassName,
    ULONG                   TitleIndex,
    UNICODE_STRING          KeyName,
    LARGE_INTEGER           CompTime,
    BOOLEAN                 Strong,         // time must be >= CompTime
    PUCHAR                  TestName
    )
{
    PWSTR   p;

    if (Strong) {

        //
        // require exact match
        //

        if ((CompTime.HighPart != NodeInformation->LastWriteTime.HighPart) ||
            (CompTime.LowPart != NodeInformation->LastWriteTime.LowPart))
        {
            printf("%s Wrong time (a)\n", TestName);
            expecttime(CompTime, NodeInformation->LastWriteTime);
            failure++;
        }


    } else {

        //
        // >= will do
        //

        if ( (CompTime.HighPart > NodeInformation->LastWriteTime.HighPart) ||
             ((CompTime.HighPart == NodeInformation->LastWriteTime.HighPart) &&
              (CompTime.LowPart > NodeInformation->LastWriteTime.LowPart)) )
        {
            printf("%s Wrong time (b)\n", TestName);
            expecttime(CompTime, NodeInformation->LastWriteTime);
            failure++;
        }
    }


    p = (PWSTR)((PUCHAR)NodeInformation + NodeInformation->ClassOffset);
    if ( (NodeInformation->ClassLength != ClassName.Length) ||
         (wcsncmp(ClassName.Buffer, p, (ClassName.Length/sizeof(WCHAR))) != 0))
    {
        printf("%s wrong class name\n", TestName);
        failure++;
    }

    if (NodeInformation->TitleIndex != TitleIndex) {
        printf("%s wrong title index\n", TestName);
        failure++;
    }

    p = (PWSTR)(&(NodeInformation->Name[0]));
    if ( (NodeInformation->NameLength != KeyName.Length) ||
         (wcsncmp(KeyName.Buffer, p, (KeyName.Length/sizeof(WCHAR))) != 0))
    {
        printf("%s wrong name\n", TestName);
        expectstring(
            KeyName.Buffer,
            (KeyName.Length/sizeof(WCHAR)),
            (PWSTR)&(NodeInformation->Name[0]),
            (NodeInformation->NameLength/sizeof(WCHAR))
            );
        failure++;
    }
}

VOID
expecttime(
    LARGE_INTEGER   ExpectTime,
    LARGE_INTEGER   ActualTime
    )
{
    printf("Expected %08lx%08lx\n", ExpectTime.HighPart, ExpectTime.LowPart);
    printf("Got      %08lx%08lx\n", ActualTime.HighPart, ActualTime.LowPart);
    return;
}

VOID
expectstring(
    PWSTR   expected,
    ULONG   expectedlength,
    PWSTR   actual,
    ULONG   actuallength
    )
{
    ULONG   i;

    printf("Expected Len %d Str = '", expectedlength);
    for (i = 0; i < expectedlength; i++) {
        printf("%c", (UCHAR)(expected[i]));
    }
    printf("'\n");

    printf("Got Len %d Str = '", actuallength);
    for (i = 0; i < actuallength; i++) {
        printf("%c", (UCHAR)(actual[i]));
    }
    printf("'\n");
    return;
}

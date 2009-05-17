/* Unit test suite for Rtl* Registry API functions
 *
 * Copyright 2003 Thomas Mertes
 * Copyright 2005 Brad DeMorrow
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * NOTE: I don't test every RelativeTo value because it would be redundant, all calls go through
 * helper function RTL_GetKeyHandle().--Brad DeMorrow
 *
 */

#include "ntdll_test.h"
#include "winternl.h"
#include "stdio.h"
#include "winnt.h"
#include "winnls.h"
#include "stdlib.h"

/* A test string */
static const WCHAR stringW[] = {'s', 't', 'r', 'i', 'n', 'g', 'W', 0};
/* A size, in bytes, short enough to cause truncation of the above */
#define STR_TRUNC_SIZE (sizeof(stringW)-2*sizeof(*stringW))

#ifndef __WINE_WINTERNL_H

/* RtlQueryRegistryValues structs and defines */
#define RTL_REGISTRY_ABSOLUTE             0
#define RTL_REGISTRY_SERVICES             1
#define RTL_REGISTRY_CONTROL              2
#define RTL_REGISTRY_WINDOWS_NT           3
#define RTL_REGISTRY_DEVICEMAP            4
#define RTL_REGISTRY_USER                 5

#define RTL_REGISTRY_HANDLE       0x40000000
#define RTL_REGISTRY_OPTIONAL     0x80000000

#define RTL_QUERY_REGISTRY_SUBKEY         0x00000001
#define RTL_QUERY_REGISTRY_TOPKEY         0x00000002
#define RTL_QUERY_REGISTRY_REQUIRED       0x00000004
#define RTL_QUERY_REGISTRY_NOVALUE        0x00000008
#define RTL_QUERY_REGISTRY_NOEXPAND       0x00000010
#define RTL_QUERY_REGISTRY_DIRECT         0x00000020
#define RTL_QUERY_REGISTRY_DELETE         0x00000040

typedef NTSTATUS (WINAPI *PRTL_QUERY_REGISTRY_ROUTINE)( PCWSTR  ValueName,
                                                        ULONG  ValueType,
                                                        PVOID  ValueData,
                                                        ULONG  ValueLength,
                                                        PVOID  Context,
                                                        PVOID  EntryContext);

typedef struct _RTL_QUERY_REGISTRY_TABLE {
  PRTL_QUERY_REGISTRY_ROUTINE  QueryRoutine;
  ULONG  Flags;
  PWSTR  Name;
  PVOID  EntryContext;
  ULONG  DefaultType;
  PVOID  DefaultData;
  ULONG  DefaultLength;
} RTL_QUERY_REGISTRY_TABLE, *PRTL_QUERY_REGISTRY_TABLE;

typedef struct _KEY_VALUE_BASIC_INFORMATION {
    ULONG TitleIndex;
    ULONG Type;
    ULONG NameLength;
    WCHAR Name[1];
} KEY_VALUE_BASIC_INFORMATION, *PKEY_VALUE_BASIC_INFORMATION;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION {
    ULONG TitleIndex;
    ULONG Type;
    ULONG DataLength;
    UCHAR Data[1];
} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION;

typedef struct _KEY_VALUE_FULL_INFORMATION {
    ULONG TitleIndex;
    ULONG Type;
    ULONG DataOffset;
    ULONG DataLength;
    ULONG NameLength;
    WCHAR Name[1];
} KEY_VALUE_FULL_INFORMATION, *PKEY_VALUE_FULL_INFORMATION;

typedef enum _KEY_VALUE_INFORMATION_CLASS {
    KeyValueBasicInformation,
    KeyValueFullInformation,
    KeyValuePartialInformation,
    KeyValueFullInformationAlign64,
    KeyValuePartialInformationAlign64
} KEY_VALUE_INFORMATION_CLASS;

#define InitializeObjectAttributes(p,n,a,r,s) \
    do { \
        (p)->Length = sizeof(OBJECT_ATTRIBUTES); \
        (p)->RootDirectory = r; \
        (p)->Attributes = a; \
        (p)->ObjectName = n; \
        (p)->SecurityDescriptor = s; \
        (p)->SecurityQualityOfService = NULL; \
    } while (0)

#endif

static NTSTATUS (WINAPI * pRtlCreateUnicodeStringFromAsciiz)(PUNICODE_STRING, LPCSTR);
static NTSTATUS (WINAPI * pRtlFreeUnicodeString)(PUNICODE_STRING);
static NTSTATUS (WINAPI * pNtDeleteValueKey)(IN HANDLE, IN PUNICODE_STRING);
static NTSTATUS (WINAPI * pRtlQueryRegistryValues)(IN ULONG, IN PCWSTR,IN PRTL_QUERY_REGISTRY_TABLE, IN PVOID,IN PVOID);
static NTSTATUS (WINAPI * pRtlCheckRegistryKey)(IN ULONG,IN PWSTR);
static NTSTATUS (WINAPI * pRtlOpenCurrentUser)(IN ACCESS_MASK, OUT PHKEY);
static NTSTATUS (WINAPI * pNtOpenKey)(PHANDLE, IN ACCESS_MASK, IN POBJECT_ATTRIBUTES);
static NTSTATUS (WINAPI * pNtClose)(IN HANDLE);
static NTSTATUS (WINAPI * pNtDeleteValueKey)(IN HANDLE, IN PUNICODE_STRING);
static NTSTATUS (WINAPI * pNtFlushKey)(HKEY);
static NTSTATUS (WINAPI * pNtDeleteKey)(HKEY);
static NTSTATUS (WINAPI * pNtCreateKey)( PHKEY retkey, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                             ULONG TitleIndex, const UNICODE_STRING *class, ULONG options,
                             PULONG dispos );
static NTSTATUS (WINAPI * pNtQueryValueKey)(HANDLE,const UNICODE_STRING *,KEY_VALUE_INFORMATION_CLASS,void *,DWORD,DWORD *);
static NTSTATUS (WINAPI * pNtSetValueKey)( PHKEY, const PUNICODE_STRING, ULONG,
                               ULONG, const PVOID, ULONG  );
static NTSTATUS (WINAPI * pRtlFormatCurrentUserKeyPath)(PUNICODE_STRING);
static NTSTATUS (WINAPI * pRtlCreateUnicodeString)( PUNICODE_STRING, LPCWSTR);
static LPVOID   (WINAPI * pRtlReAllocateHeap)(IN PVOID, IN ULONG, IN PVOID, IN ULONG);
static NTSTATUS (WINAPI * pRtlAppendUnicodeToString)(PUNICODE_STRING, PCWSTR);
static NTSTATUS (WINAPI * pRtlUnicodeStringToAnsiString)(PSTRING, PUNICODE_STRING, BOOL);
static NTSTATUS (WINAPI * pRtlFreeHeap)(PVOID, ULONG, PVOID);
static LPVOID   (WINAPI * pRtlAllocateHeap)(PVOID,ULONG,ULONG);
static NTSTATUS (WINAPI * pRtlZeroMemory)(PVOID, ULONG);
static NTSTATUS (WINAPI * pRtlpNtQueryValueKey)(HANDLE,ULONG*,PBYTE,DWORD*);

static HMODULE hntdll = 0;
static int CurrentTest = 0;
static UNICODE_STRING winetestpath;

#define NTDLL_GET_PROC(func) \
    p ## func = (void*)GetProcAddress(hntdll, #func); \
    if(!p ## func) { \
        trace("GetProcAddress(%s) failed\n", #func); \
        FreeLibrary(hntdll); \
        return FALSE; \
    }

static BOOL InitFunctionPtrs(void)
{
    hntdll = LoadLibraryA("ntdll.dll");
    if(!hntdll) {
        trace("Could not load ntdll.dll\n");
        return FALSE;
    }
    NTDLL_GET_PROC(RtlCreateUnicodeStringFromAsciiz)
    NTDLL_GET_PROC(RtlCreateUnicodeString)
    NTDLL_GET_PROC(RtlFreeUnicodeString)
    NTDLL_GET_PROC(NtDeleteValueKey)
    NTDLL_GET_PROC(RtlQueryRegistryValues)
    NTDLL_GET_PROC(RtlCheckRegistryKey)
    NTDLL_GET_PROC(RtlOpenCurrentUser)
    NTDLL_GET_PROC(NtClose)
    NTDLL_GET_PROC(NtDeleteValueKey)
    NTDLL_GET_PROC(NtCreateKey)
    NTDLL_GET_PROC(NtFlushKey)
    NTDLL_GET_PROC(NtDeleteKey)
    NTDLL_GET_PROC(NtQueryValueKey)
    NTDLL_GET_PROC(NtSetValueKey)
    NTDLL_GET_PROC(NtOpenKey)
    NTDLL_GET_PROC(RtlFormatCurrentUserKeyPath)
    NTDLL_GET_PROC(RtlReAllocateHeap)
    NTDLL_GET_PROC(RtlAppendUnicodeToString)
    NTDLL_GET_PROC(RtlUnicodeStringToAnsiString)
    NTDLL_GET_PROC(RtlFreeHeap)
    NTDLL_GET_PROC(RtlAllocateHeap)
    NTDLL_GET_PROC(RtlZeroMemory)
    NTDLL_GET_PROC(RtlpNtQueryValueKey)
    return TRUE;
}
#undef NTDLL_GET_PROC

static NTSTATUS WINAPI QueryRoutine (IN PCWSTR ValueName, IN ULONG ValueType, IN PVOID ValueData,
                              IN ULONG ValueLength, IN PVOID Context, IN PVOID EntryContext)
{
    NTSTATUS ret = STATUS_SUCCESS;
    int ValueNameLength = 0;
    LPSTR ValName = 0;
    trace("**Test %d**\n", CurrentTest);

    if(ValueName)
    {
        ValueNameLength = lstrlenW(ValueName);

        ValName = pRtlAllocateHeap(GetProcessHeap(), 0, ValueNameLength);

        WideCharToMultiByte(0, 0, ValueName, ValueNameLength+1,ValName, ValueNameLength, 0, 0);

        trace("ValueName: %s\n", ValName);
    }
    else
        trace("ValueName: (null)\n");

    switch(ValueType)
    {
            case REG_NONE:
                trace("ValueType: REG_NONE\n");
                trace("ValueData: %p\n", ValueData);
                break;

            case REG_BINARY:
                trace("ValueType: REG_BINARY\n");
                trace("ValueData: %p\n", ValueData);
                break;

            case REG_SZ:
                trace("ValueType: REG_SZ\n");
                trace("ValueData: %s\n", (char*)ValueData);
                break;

            case REG_MULTI_SZ:
                trace("ValueType: REG_MULTI_SZ\n");
                trace("ValueData: %s\n", (char*)ValueData);
                break;

            case REG_EXPAND_SZ:
                trace("ValueType: REG_EXPAND_SZ\n");
                trace("ValueData: %s\n", (char*)ValueData);
                break;

            case REG_DWORD:
                trace("ValueType: REG_DWORD\n");
                trace("ValueData: %p\n", ValueData);
                break;
    };
    trace("ValueLength: %d\n", (int)ValueLength);

    if(CurrentTest == 0)
        ok(1, "\n"); /*checks that QueryRoutine is called*/
    if(CurrentTest > 7)
        ok(!1, "Invalid Test Specified!\n");

    CurrentTest++;

    if(ValName)
        pRtlFreeHeap(GetProcessHeap(), 0, ValName);

    return ret;
}

static void test_RtlQueryRegistryValues(void)
{

    /*
    ******************************
    *       QueryTable Flags     *
    ******************************
    *RTL_QUERY_REGISTRY_SUBKEY   * Name is the name of a subkey relative to Path
    *RTL_QUERY_REGISTRY_TOPKEY   * Resets location to original RelativeTo and Path
    *RTL_QUERY_REGISTRY_REQUIRED * Key required. returns STATUS_OBJECT_NAME_NOT_FOUND if not present
    *RTL_QUERY_REGISTRY_NOVALUE  * We just want a call-back
    *RTL_QUERY_REGISTRY_NOEXPAND * Don't expand the variables!
    *RTL_QUERY_REGISTRY_DIRECT   * Results of query will be stored in EntryContext(QueryRoutine ignored)
    *RTL_QUERY_REGISTRY_DELETE   * Delete value key after query
    ******************************


    **Test layout(numbered according to CurrentTest value)**
    0)NOVALUE           Just make sure call-back works
    1)Null Name         See if QueryRoutine is called for every value in current key
    2)SUBKEY            See if we can use SUBKEY to change the current path on the fly
    3)REQUIRED          Test for value that's not there
    4)NOEXPAND          See if it will return multiple strings(no expand should split strings up)
    5)DIRECT            Make it store data directly in EntryContext and not call QueryRoutine
    6)DefaultType       Test return values when key isn't present
    7)DefaultValue      Test Default Value returned with key isn't present(and no REQUIRED flag set)
    8)DefaultLength     Test Default Length with DefaultType = REG_SZ
   9)DefaultLength      Test Default Length with DefaultType = REG_MULTI_SZ
   10)DefaultLength     Test Default Length with DefaultType = REG_EXPAND_SZ
   11)DefaultData       Test whether DefaultData is used while DefaultType = REG_NONE(shouldn't be)
   12)Delete            Try to delete value key

    */
    NTSTATUS status;
    ULONG RelativeTo;

    PRTL_QUERY_REGISTRY_TABLE QueryTable = NULL;
    RelativeTo = RTL_REGISTRY_ABSOLUTE;/*Only using absolute - no need to test all relativeto variables*/

    QueryTable = pRtlAllocateHeap(GetProcessHeap(), 0, sizeof(RTL_QUERY_REGISTRY_TABLE)*26);

    pRtlZeroMemory( QueryTable, sizeof(RTL_QUERY_REGISTRY_TABLE) * 26);

    QueryTable[0].QueryRoutine = QueryRoutine;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_NOVALUE;
    QueryTable[0].Name = NULL;
    QueryTable[0].EntryContext = NULL;
    QueryTable[0].DefaultType = REG_BINARY;
    QueryTable[0].DefaultData = NULL;
    QueryTable[0].DefaultLength = 100;

    QueryTable[1].QueryRoutine = QueryRoutine;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = NULL;
    QueryTable[1].EntryContext = 0;
    QueryTable[1].DefaultType = REG_NONE;
    QueryTable[1].DefaultData = NULL;
    QueryTable[1].DefaultLength = 0;

    QueryTable[2].QueryRoutine = NULL;
    QueryTable[2].Flags = 0;
    QueryTable[2].Name = NULL;
    QueryTable[2].EntryContext = 0;
    QueryTable[2].DefaultType = REG_NONE;
    QueryTable[2].DefaultData = NULL;
    QueryTable[2].DefaultLength = 0;

    status = pRtlQueryRegistryValues(RelativeTo, winetestpath.Buffer, QueryTable, 0, 0);
    ok(status == STATUS_SUCCESS, "RtlQueryRegistryValues return: 0x%08x\n", status);

    pRtlFreeHeap(GetProcessHeap(), 0, QueryTable);
}

static void test_NtOpenKey(void)
{
    HANDLE key;
    NTSTATUS status;
    OBJECT_ATTRIBUTES attr;
    ACCESS_MASK am = KEY_READ;

    if (0)
    {
    /* Crashes Wine */
    /* All NULL */
    status = pNtOpenKey(NULL, 0, NULL);
    ok(status == STATUS_ACCESS_VIOLATION, "Expected STATUS_ACCESS_VIOLATION, got: 0x%08x\n", status);

    /* NULL attributes */
    status = pNtOpenKey(&key, 0, NULL);
    ok(status == STATUS_ACCESS_VIOLATION /* W2K3/XP/W2K */ || status == STATUS_INVALID_PARAMETER /* NT4 */,
        "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_PARAMETER(NT4), got: 0x%08x\n", status);
    }

    InitializeObjectAttributes(&attr, &winetestpath, 0, 0, 0);

    /* NULL key */
    status = pNtOpenKey(NULL, 0, &attr);
    todo_wine
        ok(status == STATUS_ACCESS_VIOLATION, "Expected STATUS_ACCESS_VIOLATION, got: 0x%08x\n", status);

    /* Length > sizeof(OBJECT_ATTRIBUTES) */
    attr.Length *= 2;
    status = pNtOpenKey(&key, am, &attr);
    todo_wine
        ok(status == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got: 0x%08x\n", status);
}

static void test_NtCreateKey(void)
{
    /*Create WineTest*/
    OBJECT_ATTRIBUTES attr;
    HKEY key;
    ACCESS_MASK am = GENERIC_ALL;
    NTSTATUS status;

    /* All NULL */
    status = pNtCreateKey(NULL, 0, NULL, 0, 0, 0, 0);
    ok(status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_PARAMETER,
       "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_PARAMETER, got: 0x%08x\n", status);

    /* Only the key */
    status = pNtCreateKey(&key, 0, NULL, 0, 0, 0, 0);
    ok(status == STATUS_ACCESS_VIOLATION /* W2K3/XP/W2K */ || status == STATUS_INVALID_PARAMETER /* NT4 */,
        "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_PARAMETER(NT4), got: 0x%08x\n", status);

    /* Only accessmask */
    status = pNtCreateKey(NULL, am, NULL, 0, 0, 0, 0);
    ok(status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_PARAMETER,
       "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_PARAMETER, got: 0x%08x\n", status);

    /* Key and accessmask */
    status = pNtCreateKey(&key, am, NULL, 0, 0, 0, 0);
    ok(status == STATUS_ACCESS_VIOLATION /* W2K3/XP/W2K */ || status == STATUS_INVALID_PARAMETER /* NT4 */,
        "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_PARAMETER(NT4), got: 0x%08x\n", status);

    InitializeObjectAttributes(&attr, &winetestpath, 0, 0, 0);

    /* Only attributes */
    status = pNtCreateKey(NULL, 0, &attr, 0, 0, 0, 0);
    ok(status == STATUS_ACCESS_VIOLATION, "Expected STATUS_ACCESS_VIOLATION, got: 0x%08x\n", status);

    status = pNtCreateKey(&key, am, &attr, 0, 0, 0, 0);
    ok(status == STATUS_SUCCESS, "NtCreateKey Failed: 0x%08x\n", status);

    /* Length > sizeof(OBJECT_ATTRIBUTES) */
    attr.Length *= 2;
    status = pNtCreateKey(&key, am, &attr, 0, 0, 0, 0);
    ok(status == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got: 0x%08x\n", status);

    pNtClose(key);
}

static void test_NtSetValueKey(void)
{
    HANDLE key;
    NTSTATUS status;
    OBJECT_ATTRIBUTES attr;
    ACCESS_MASK am = KEY_WRITE;
    UNICODE_STRING ValName;
    DWORD data = 711;

    InitializeObjectAttributes(&attr, &winetestpath, 0, 0, 0);
    status = pNtOpenKey(&key, am, &attr);
    ok(status == STATUS_SUCCESS, "NtOpenKey Failed: 0x%08x\n", status);

    pRtlCreateUnicodeStringFromAsciiz(&ValName, "deletetest");
    status = pNtSetValueKey(key, &ValName, 0, REG_DWORD, &data, sizeof(data));
    ok(status == STATUS_SUCCESS, "NtSetValueKey Failed: 0x%08x\n", status);
    pRtlFreeUnicodeString(&ValName);

    pRtlCreateUnicodeStringFromAsciiz(&ValName, "stringtest");
    status = pNtSetValueKey(key, &ValName, 0, REG_SZ, (VOID*)stringW, STR_TRUNC_SIZE);
    ok(status == STATUS_SUCCESS, "NtSetValueKey Failed: 0x%08x\n", status);
    pRtlFreeUnicodeString(&ValName);

    pNtClose(key);
}

static void test_RtlOpenCurrentUser(void)
{
    NTSTATUS status;
    HKEY handle;
    status=pRtlOpenCurrentUser(KEY_READ, &handle);
    ok(status == STATUS_SUCCESS, "RtlOpenCurrentUser Failed: 0x%08x\n", status);
    pNtClose(handle);
}

static void test_RtlCheckRegistryKey(void)
{
    NTSTATUS status;

    status = pRtlCheckRegistryKey(RTL_REGISTRY_ABSOLUTE, winetestpath.Buffer);
    ok(status == STATUS_SUCCESS, "RtlCheckRegistryKey with RTL_REGISTRY_ABSOLUTE: 0x%08x\n", status);

    status = pRtlCheckRegistryKey((RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL), winetestpath.Buffer);
    ok(status == STATUS_SUCCESS, "RtlCheckRegistryKey with RTL_REGISTRY_ABSOLUTE and RTL_REGISTRY_OPTIONAL: 0x%08x\n", status);
}

static void test_NtFlushKey(void)
{
    NTSTATUS status;
    HANDLE hkey;
    OBJECT_ATTRIBUTES attr;
    ACCESS_MASK am = KEY_ALL_ACCESS;

    status = pNtFlushKey(NULL);
    ok(status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got: 0x%08x\n", status);

    InitializeObjectAttributes(&attr, &winetestpath, 0, 0, 0);
    pNtOpenKey(&hkey, am, &attr);

    status = pNtFlushKey(hkey);
    ok(status == STATUS_SUCCESS, "NtDeleteKey Failed: 0x%08x\n", status);

    pNtClose(hkey);
}

static void test_NtQueryValueKey(void)
{
    HANDLE key;
    NTSTATUS status;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING ValName;
    KEY_VALUE_BASIC_INFORMATION *basic_info;
    KEY_VALUE_PARTIAL_INFORMATION *partial_info;
    KEY_VALUE_FULL_INFORMATION *full_info;
    DWORD len;

    pRtlCreateUnicodeStringFromAsciiz(&ValName, "deletetest");

    InitializeObjectAttributes(&attr, &winetestpath, 0, 0, 0);
    status = pNtOpenKey(&key, KEY_READ, &attr);
    ok(status == STATUS_SUCCESS, "NtOpenKey Failed: 0x%08x\n", status);

    len = FIELD_OFFSET(KEY_VALUE_BASIC_INFORMATION, Name[0]);
    basic_info = HeapAlloc(GetProcessHeap(), 0, len);
    status = pNtQueryValueKey(key, &ValName, KeyValueBasicInformation, basic_info, len, &len);
    ok(status == STATUS_BUFFER_OVERFLOW, "NtQueryValueKey should have returned STATUS_BUFFER_OVERFLOW instead of 0x%08x\n", status);
    ok(basic_info->TitleIndex == 0, "NtQueryValueKey returned wrong TitleIndex %d\n", basic_info->TitleIndex);
    ok(basic_info->Type == REG_DWORD, "NtQueryValueKey returned wrong Type %d\n", basic_info->Type);
    ok(basic_info->NameLength == 20, "NtQueryValueKey returned wrong NameLength %d\n", basic_info->NameLength);
    ok(len == FIELD_OFFSET(KEY_VALUE_BASIC_INFORMATION, Name[basic_info->NameLength/sizeof(WCHAR)]), "NtQueryValueKey returned wrong len %d\n", len);

    basic_info = HeapReAlloc(GetProcessHeap(), 0, basic_info, len);
    status = pNtQueryValueKey(key, &ValName, KeyValueBasicInformation, basic_info, len, &len);
    ok(status == STATUS_SUCCESS, "NtQueryValueKey should have returned STATUS_SUCCESS instead of 0x%08x\n", status);
    ok(basic_info->TitleIndex == 0, "NtQueryValueKey returned wrong TitleIndex %d\n", basic_info->TitleIndex);
    ok(basic_info->Type == REG_DWORD, "NtQueryValueKey returned wrong Type %d\n", basic_info->Type);
    ok(basic_info->NameLength == 20, "NtQueryValueKey returned wrong NameLength %d\n", basic_info->NameLength);
    ok(len == FIELD_OFFSET(KEY_VALUE_BASIC_INFORMATION, Name[basic_info->NameLength/sizeof(WCHAR)]), "NtQueryValueKey returned wrong len %d\n", len);
    ok(!memcmp(basic_info->Name, ValName.Buffer, ValName.Length), "incorrect Name returned\n");
    HeapFree(GetProcessHeap(), 0, basic_info);

    len = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[0]);
    partial_info = HeapAlloc(GetProcessHeap(), 0, len);
    status = pNtQueryValueKey(key, &ValName, KeyValuePartialInformation, partial_info, len, &len);
    ok(status == STATUS_BUFFER_OVERFLOW, "NtQueryValueKey should have returned STATUS_BUFFER_OVERFLOW instead of 0x%08x\n", status);
    ok(partial_info->TitleIndex == 0, "NtQueryValueKey returned wrong TitleIndex %d\n", partial_info->TitleIndex);
    ok(partial_info->Type == REG_DWORD, "NtQueryValueKey returned wrong Type %d\n", partial_info->Type);
    ok(partial_info->DataLength == 4, "NtQueryValueKey returned wrong DataLength %d\n", partial_info->DataLength);
    ok(len == FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[partial_info->DataLength]), "NtQueryValueKey returned wrong len %d\n", len);

    partial_info = HeapReAlloc(GetProcessHeap(), 0, partial_info, len);
    status = pNtQueryValueKey(key, &ValName, KeyValuePartialInformation, partial_info, len, &len);
    ok(status == STATUS_SUCCESS, "NtQueryValueKey should have returned STATUS_SUCCESS instead of 0x%08x\n", status);
    ok(partial_info->TitleIndex == 0, "NtQueryValueKey returned wrong TitleIndex %d\n", partial_info->TitleIndex);
    ok(partial_info->Type == REG_DWORD, "NtQueryValueKey returned wrong Type %d\n", partial_info->Type);
    ok(partial_info->DataLength == 4, "NtQueryValueKey returned wrong DataLength %d\n", partial_info->DataLength);
    ok(len == FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[partial_info->DataLength]), "NtQueryValueKey returned wrong len %d\n", len);
    ok(*(DWORD *)partial_info->Data == 711, "incorrect Data returned: 0x%x\n", *(DWORD *)partial_info->Data);
    HeapFree(GetProcessHeap(), 0, partial_info);

    len = FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name[0]);
    full_info = HeapAlloc(GetProcessHeap(), 0, len);
    status = pNtQueryValueKey(key, &ValName, KeyValueFullInformation, full_info, len, &len);
    ok(status == STATUS_BUFFER_OVERFLOW, "NtQueryValueKey should have returned STATUS_BUFFER_OVERFLOW instead of 0x%08x\n", status);
    ok(full_info->TitleIndex == 0, "NtQueryValueKey returned wrong TitleIndex %d\n", full_info->TitleIndex);
    ok(full_info->Type == REG_DWORD, "NtQueryValueKey returned wrong Type %d\n", full_info->Type);
    ok(full_info->DataLength == 4, "NtQueryValueKey returned wrong DataLength %d\n", full_info->DataLength);
    ok(full_info->NameLength == 20, "NtQueryValueKey returned wrong NameLength %d\n", full_info->NameLength);
    ok(len == FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name[0]) + full_info->DataLength + full_info->NameLength,
        "NtQueryValueKey returned wrong len %d\n", len);
    len = FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name[0]) + full_info->DataLength + full_info->NameLength;

    full_info = HeapReAlloc(GetProcessHeap(), 0, full_info, len);
    status = pNtQueryValueKey(key, &ValName, KeyValueFullInformation, full_info, len, &len);
    ok(status == STATUS_SUCCESS, "NtQueryValueKey should have returned STATUS_SUCCESS instead of 0x%08x\n", status);
    ok(full_info->TitleIndex == 0, "NtQueryValueKey returned wrong TitleIndex %d\n", full_info->TitleIndex);
    ok(full_info->Type == REG_DWORD, "NtQueryValueKey returned wrong Type %d\n", full_info->Type);
    ok(full_info->DataLength == 4, "NtQueryValueKey returned wrong DataLength %d\n", full_info->DataLength);
    ok(full_info->NameLength == 20, "NtQueryValueKey returned wrong NameLength %d\n", full_info->NameLength);
    ok(!memcmp(full_info->Name, ValName.Buffer, ValName.Length), "incorrect Name returned\n");
    ok(*(DWORD *)((char *)full_info + full_info->DataOffset) == 711, "incorrect Data returned: 0x%x\n",
        *(DWORD *)((char *)full_info + full_info->DataOffset));
    HeapFree(GetProcessHeap(), 0, full_info);

    pRtlFreeUnicodeString(&ValName);
    pRtlCreateUnicodeStringFromAsciiz(&ValName, "stringtest");

    status = pNtQueryValueKey(key, &ValName, KeyValuePartialInformation, NULL, 0, &len);
    todo_wine ok(status == STATUS_BUFFER_TOO_SMALL, "NtQueryValueKey should have returned STATUS_BUFFER_TOO_SMALL instead of 0x%08x\n", status);
    partial_info = HeapAlloc(GetProcessHeap(), 0, len+1);
    memset(partial_info, 0xbd, len+1);
    status = pNtQueryValueKey(key, &ValName, KeyValuePartialInformation, partial_info, len, &len);
    ok(status == STATUS_SUCCESS, "NtQueryValueKey should have returned STATUS_SUCCESS instead of 0x%08x\n", status);
    ok(partial_info->TitleIndex == 0, "NtQueryValueKey returned wrong TitleIndex %d\n", partial_info->TitleIndex);
    ok(partial_info->Type == REG_SZ, "NtQueryValueKey returned wrong Type %d\n", partial_info->Type);
    ok(partial_info->DataLength == STR_TRUNC_SIZE, "NtQueryValueKey returned wrong DataLength %d\n", partial_info->DataLength);
    ok(!memcmp(partial_info->Data, stringW, STR_TRUNC_SIZE), "incorrect Data returned\n");
    ok(*(partial_info->Data+STR_TRUNC_SIZE) == 0xbd, "string overflowed %02x\n", *(partial_info->Data+STR_TRUNC_SIZE));
    HeapFree(GetProcessHeap(), 0, partial_info);

    pRtlFreeUnicodeString(&ValName);
    pNtClose(key);
}

static void test_NtDeleteKey(void)
{
    NTSTATUS status;
    HANDLE hkey;
    OBJECT_ATTRIBUTES attr;
    ACCESS_MASK am = KEY_ALL_ACCESS;

    status = pNtDeleteKey(NULL);
    ok(status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got: 0x%08x\n", status);

    InitializeObjectAttributes(&attr, &winetestpath, 0, 0, 0);
    status = pNtOpenKey(&hkey, am, &attr);

    status = pNtDeleteKey(hkey);
    ok(status == STATUS_SUCCESS, "NtDeleteKey Failed: 0x%08x\n", status);
}

static void test_RtlpNtQueryValueKey(void)
{
    NTSTATUS status;

    status = pRtlpNtQueryValueKey(NULL, NULL, NULL, NULL);
    ok(status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got: 0x%08x\n", status);
}

START_TEST(reg)
{
    static const WCHAR winetest[] = {'\\','W','i','n','e','T','e','s','t',0};
    if(!InitFunctionPtrs())
        return;
    pRtlFormatCurrentUserKeyPath(&winetestpath);
    winetestpath.Buffer = pRtlReAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, winetestpath.Buffer,
                           winetestpath.MaximumLength + sizeof(winetest)*sizeof(WCHAR));
    winetestpath.MaximumLength = winetestpath.MaximumLength + sizeof(winetest)*sizeof(WCHAR);

    pRtlAppendUnicodeToString(&winetestpath, winetest);

    test_NtOpenKey();
    test_NtCreateKey();
    test_NtSetValueKey();
    test_RtlCheckRegistryKey();
    test_RtlOpenCurrentUser();
    test_RtlQueryRegistryValues();
    test_RtlpNtQueryValueKey();
    test_NtFlushKey();
    test_NtQueryValueKey();
    test_NtDeleteKey();

    pRtlFreeUnicodeString(&winetestpath);

    FreeLibrary(hntdll);
}

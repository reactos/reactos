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
#include "wine/winternl.h"
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

static BOOLEAN  (WINAPI * pRtlCreateUnicodeStringFromAsciiz)(PUNICODE_STRING, LPCSTR);
static void     (WINAPI * pRtlInitUnicodeString)(PUNICODE_STRING,PCWSTR);
static NTSTATUS (WINAPI * pRtlFreeUnicodeString)(PUNICODE_STRING);
static NTSTATUS (WINAPI * pNtDeleteValueKey)(IN HANDLE, IN PUNICODE_STRING);
static NTSTATUS (WINAPI * pRtlQueryRegistryValues)(IN ULONG, IN PCWSTR,IN PRTL_QUERY_REGISTRY_TABLE, IN PVOID,IN PVOID);
static NTSTATUS (WINAPI * pRtlCheckRegistryKey)(IN ULONG,IN PWSTR);
static NTSTATUS (WINAPI * pRtlOpenCurrentUser)(IN ACCESS_MASK, PHANDLE);
static NTSTATUS (WINAPI * pNtOpenKey)(PHANDLE, IN ACCESS_MASK, IN POBJECT_ATTRIBUTES);
static NTSTATUS (WINAPI * pNtClose)(IN HANDLE);
static NTSTATUS (WINAPI * pNtFlushKey)(HANDLE);
static NTSTATUS (WINAPI * pNtDeleteKey)(HANDLE);
static NTSTATUS (WINAPI * pNtCreateKey)( PHANDLE retkey, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                             ULONG TitleIndex, const UNICODE_STRING *class, ULONG options,
                             PULONG dispos );
static NTSTATUS (WINAPI * pNtQueryKey)(HANDLE,KEY_INFORMATION_CLASS,PVOID,ULONG,PULONG);
static NTSTATUS (WINAPI * pNtQueryLicenseValue)(const UNICODE_STRING *,ULONG *,PVOID,ULONG,ULONG *);
static NTSTATUS (WINAPI * pNtQueryValueKey)(HANDLE,const UNICODE_STRING *,KEY_VALUE_INFORMATION_CLASS,void *,DWORD,DWORD *);
static NTSTATUS (WINAPI * pNtSetValueKey)(HANDLE, const PUNICODE_STRING, ULONG,
                               ULONG, const void*, ULONG  );
static NTSTATUS (WINAPI * pNtQueryInformationProcess)(HANDLE,PROCESSINFOCLASS,PVOID,ULONG,PULONG);
static NTSTATUS (WINAPI * pRtlFormatCurrentUserKeyPath)(PUNICODE_STRING);
static LONG     (WINAPI * pRtlCompareUnicodeString)(const PUNICODE_STRING,const PUNICODE_STRING,BOOLEAN);
static BOOLEAN  (WINAPI * pRtlCreateUnicodeString)(PUNICODE_STRING, LPCWSTR);
static LPVOID   (WINAPI * pRtlReAllocateHeap)(IN PVOID, IN ULONG, IN PVOID, IN ULONG);
static NTSTATUS (WINAPI * pRtlAppendUnicodeToString)(PUNICODE_STRING, PCWSTR);
static NTSTATUS (WINAPI * pRtlUnicodeStringToAnsiString)(PSTRING, PUNICODE_STRING, BOOL);
static NTSTATUS (WINAPI * pRtlFreeHeap)(PVOID, ULONG, PVOID);
static LPVOID   (WINAPI * pRtlAllocateHeap)(PVOID,ULONG,ULONG);
static NTSTATUS (WINAPI * pRtlZeroMemory)(PVOID, ULONG);
static NTSTATUS (WINAPI * pRtlpNtQueryValueKey)(HANDLE,ULONG*,PBYTE,DWORD*,void *);

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
    NTDLL_GET_PROC(RtlInitUnicodeString)
    NTDLL_GET_PROC(RtlCreateUnicodeStringFromAsciiz)
    NTDLL_GET_PROC(RtlCreateUnicodeString)
    NTDLL_GET_PROC(RtlFreeUnicodeString)
    NTDLL_GET_PROC(RtlQueryRegistryValues)
    NTDLL_GET_PROC(RtlCheckRegistryKey)
    NTDLL_GET_PROC(RtlOpenCurrentUser)
    NTDLL_GET_PROC(NtClose)
    NTDLL_GET_PROC(NtDeleteValueKey)
    NTDLL_GET_PROC(NtCreateKey)
    NTDLL_GET_PROC(NtFlushKey)
    NTDLL_GET_PROC(NtDeleteKey)
    NTDLL_GET_PROC(NtQueryKey)
    NTDLL_GET_PROC(NtQueryValueKey)
    NTDLL_GET_PROC(NtQueryInformationProcess)
    NTDLL_GET_PROC(NtSetValueKey)
    NTDLL_GET_PROC(NtOpenKey)
    NTDLL_GET_PROC(RtlFormatCurrentUserKeyPath)
    NTDLL_GET_PROC(RtlCompareUnicodeString)
    NTDLL_GET_PROC(RtlReAllocateHeap)
    NTDLL_GET_PROC(RtlAppendUnicodeToString)
    NTDLL_GET_PROC(RtlUnicodeStringToAnsiString)
    NTDLL_GET_PROC(RtlFreeHeap)
    NTDLL_GET_PROC(RtlAllocateHeap)
    NTDLL_GET_PROC(RtlZeroMemory)
    NTDLL_GET_PROC(RtlpNtQueryValueKey)
    NTDLL_GET_PROC(RtlOpenCurrentUser)

    /* optional functions */
    pNtQueryLicenseValue = (void *)GetProcAddress(hntdll, "NtQueryLicenseValue");

    return TRUE;
}
#undef NTDLL_GET_PROC

static NTSTATUS WINAPI QueryRoutine (IN PCWSTR ValueName, IN ULONG ValueType, IN PVOID ValueData,
                              IN ULONG ValueLength, IN PVOID Context, IN PVOID EntryContext)
{
    NTSTATUS ret = STATUS_SUCCESS;

    trace("**Test %d**\n", CurrentTest);
    trace("ValueName: %s\n", wine_dbgstr_w(ValueName));

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

    /* All NULL */
    status = pNtOpenKey(NULL, 0, NULL);
    ok(status == STATUS_ACCESS_VIOLATION, "Expected STATUS_ACCESS_VIOLATION, got: 0x%08x\n", status);

    /* NULL attributes */
    status = pNtOpenKey(&key, 0, NULL);
    ok(status == STATUS_ACCESS_VIOLATION /* W2K3/XP/W2K */ || status == STATUS_INVALID_PARAMETER /* NT4 */,
        "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_PARAMETER(NT4), got: 0x%08x\n", status);

    InitializeObjectAttributes(&attr, &winetestpath, 0, 0, 0);

    /* NULL key */
    status = pNtOpenKey(NULL, am, &attr);
    ok(status == STATUS_ACCESS_VIOLATION, "Expected STATUS_ACCESS_VIOLATION, got: 0x%08x\n", status);

    /* Length > sizeof(OBJECT_ATTRIBUTES) */
    attr.Length *= 2;
    status = pNtOpenKey(&key, am, &attr);
    ok(status == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got: 0x%08x\n", status);
}

static void test_NtCreateKey(void)
{
    /*Create WineTest*/
    OBJECT_ATTRIBUTES attr;
    HANDLE key, subkey;
    ACCESS_MASK am = GENERIC_ALL;
    NTSTATUS status;
    UNICODE_STRING str;

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
    ok(status == STATUS_ACCESS_VIOLATION || status == STATUS_ACCESS_DENIED /* Win7 */,
       "Expected STATUS_ACCESS_VIOLATION or STATUS_ACCESS_DENIED, got: 0x%08x\n", status);

    /* Length > sizeof(OBJECT_ATTRIBUTES) */
    attr.Length *= 2;
    status = pNtCreateKey(&key, am, &attr, 0, 0, 0, 0);
    ok(status == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got: 0x%08x\n", status);

    attr.Length = sizeof(attr);
    status = pNtCreateKey(&key, am, &attr, 0, 0, 0, 0);
    ok(status == STATUS_SUCCESS, "NtCreateKey Failed: 0x%08x\n", status);

    attr.RootDirectory = key;
    attr.ObjectName = &str;

    pRtlCreateUnicodeStringFromAsciiz( &str, "test\\sub\\key" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "NtCreateKey failed: 0x%08x\n", status );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "test\\subkey" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "NtCreateKey failed: 0x%08x\n", status );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "test\\subkey\\" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "NtCreateKey failed: 0x%08x\n", status );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "test_subkey\\" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS || broken(status == STATUS_OBJECT_NAME_NOT_FOUND), /* nt4 */
        "NtCreateKey failed: 0x%08x\n", status );
    if (status == STATUS_SUCCESS)
    {
        pNtDeleteKey( subkey );
        pNtClose( subkey );
    }
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "test_subkey" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );
    pRtlFreeUnicodeString( &str );
    pNtDeleteKey( subkey );
    pNtClose( subkey );

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
    HANDLE handle;
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
    DWORD len, expected;

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
    ok(status == STATUS_BUFFER_TOO_SMALL, "NtQueryValueKey should have returned STATUS_BUFFER_TOO_SMALL instead of 0x%08x\n", status);
    partial_info = HeapAlloc(GetProcessHeap(), 0, len+1);
    memset(partial_info, 0xbd, len+1);
    status = pNtQueryValueKey(key, &ValName, KeyValuePartialInformation, partial_info, len, &len);
    ok(status == STATUS_SUCCESS, "NtQueryValueKey should have returned STATUS_SUCCESS instead of 0x%08x\n", status);
    ok(partial_info->TitleIndex == 0, "NtQueryValueKey returned wrong TitleIndex %d\n", partial_info->TitleIndex);
    ok(partial_info->Type == REG_SZ, "NtQueryValueKey returned wrong Type %d\n", partial_info->Type);
    ok(partial_info->DataLength == STR_TRUNC_SIZE, "NtQueryValueKey returned wrong DataLength %d\n", partial_info->DataLength);
    ok(!memcmp(partial_info->Data, stringW, STR_TRUNC_SIZE), "incorrect Data returned\n");
    ok(*(partial_info->Data+STR_TRUNC_SIZE) == 0xbd, "string overflowed %02x\n", *(partial_info->Data+STR_TRUNC_SIZE));

    expected = len;
    status = pNtQueryValueKey(key, &ValName, KeyValuePartialInformation, partial_info, 0, &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "NtQueryValueKey wrong status 0x%08x\n", status);
    ok(len == expected, "NtQueryValueKey wrong len %u\n", len);
    status = pNtQueryValueKey(key, &ValName, KeyValuePartialInformation, partial_info, 1, &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "NtQueryValueKey wrong status 0x%08x\n", status);
    ok(len == expected, "NtQueryValueKey wrong len %u\n", len);
    status = pNtQueryValueKey(key, &ValName, KeyValuePartialInformation, partial_info, FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) - 1, &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "NtQueryValueKey wrong status 0x%08x\n", status);
    ok(len == expected, "NtQueryValueKey wrong len %u\n", len);
    status = pNtQueryValueKey(key, &ValName, KeyValuePartialInformation, partial_info, FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data), &len);
    ok(status == STATUS_BUFFER_OVERFLOW, "NtQueryValueKey wrong status 0x%08x\n", status);
    ok(len == expected, "NtQueryValueKey wrong len %u\n", len);

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
    ok(status == STATUS_SUCCESS, "NtOpenKey Failed: 0x%08x\n", status);

    status = pNtDeleteKey(hkey);
    ok(status == STATUS_SUCCESS, "NtDeleteKey Failed: 0x%08x\n", status);
}

static void test_NtQueryLicenseKey(void)
{
    static const WCHAR emptyW[] = {'E','M','P','T','Y',0};
    UNICODE_STRING name;
    WORD buffer[32];
    NTSTATUS status;
    ULONG type, len;
    DWORD value;

    if (!pNtQueryLicenseValue)
    {
        win_skip("NtQueryLicenseValue not found, skipping tests\n");
        return;
    }

    type = 0xdead;
    len = 0xbeef;
    memset(&name, 0, sizeof(name));
    status = pNtQueryLicenseValue(&name, &type, buffer, sizeof(buffer), &len);
    ok(status == STATUS_INVALID_PARAMETER, "NtQueryLicenseValue returned %08x, expected STATUS_INVALID_PARAMETER\n", status);
    ok(type == 0xdead, "expected unmodified value for type, got %u\n", type);
    ok(len == 0xbeef, "expected unmodified value for len, got %u\n", len);

    /* test with empty key */
    pRtlCreateUnicodeStringFromAsciiz(&name, "");

    type = 0xdead;
    len = 0xbeef;
    status = pNtQueryLicenseValue(NULL, &type, buffer, sizeof(buffer), &len);
    ok(status == STATUS_INVALID_PARAMETER, "NtQueryLicenseValue returned %08x, expected STATUS_INVALID_PARAMETER\n", status);
    ok(type == 0xdead, "expected unmodified value for type, got %u\n", type);
    ok(len == 0xbeef, "expected unmodified value for len, got %u\n", len);

    type = 0xdead;
    status = pNtQueryLicenseValue(&name, &type, buffer, sizeof(buffer), NULL);
    ok(status == STATUS_INVALID_PARAMETER, "NtQueryLicenseValue returned %08x, expected STATUS_INVALID_PARAMETER\n", status);
    ok(type == 0xdead, "expected unmodified value for type, got %u\n", type);

    len = 0xbeef;
    status = pNtQueryLicenseValue(&name, NULL, buffer, sizeof(buffer), &len);
    ok(status == STATUS_INVALID_PARAMETER, "NtQueryLicenseValue returned %08x, expected STATUS_INVALID_PARAMETER\n", status);
    ok(len == 0xbeef, "expected unmodified value for len, got %u\n", len);

    type = 0xdead;
    len = 0xbeef;
    status = pNtQueryLicenseValue(&name, &type, buffer, sizeof(buffer), &len);
    ok(status == STATUS_INVALID_PARAMETER, "NtQueryLicenseValue returned %08x, expected STATUS_INVALID_PARAMETER\n", status);
    ok(type == 0xdead, "expected unmodified value for type, got %u\n", type);
    ok(len == 0xbeef, "expected unmodified value for len, got %u\n", len);

    pRtlFreeUnicodeString(&name);

    /* test with nonexistent licence key */
    pRtlCreateUnicodeStringFromAsciiz(&name, "Nonexistent-License-Value");

    type = 0xdead;
    len = 0xbeef;
    status = pNtQueryLicenseValue(NULL, &type, buffer, sizeof(buffer), &len);
    ok(status == STATUS_INVALID_PARAMETER, "NtQueryLicenseValue returned %08x, expected STATUS_INVALID_PARAMETER\n", status);
    ok(type == 0xdead, "expected unmodified value for type, got %u\n", type);
    ok(len == 0xbeef, "expected unmodified value for len, got %u\n", len);

    type = 0xdead;
    status = pNtQueryLicenseValue(&name, &type, buffer, sizeof(buffer), NULL);
    ok(status == STATUS_INVALID_PARAMETER, "NtQueryLicenseValue returned %08x, expected STATUS_INVALID_PARAMETER\n", status);
    ok(type == 0xdead, "expected unmodified value for type, got %u\n", type);

    len = 0xbeef;
    status = pNtQueryLicenseValue(&name, NULL, buffer, sizeof(buffer), &len);
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND, "NtQueryLicenseValue returned %08x, expected STATUS_OBJECT_NAME_NOT_FOUND\n", status);
    ok(len == 0xbeef, "expected unmodified value for len, got %u\n", len);

    type = 0xdead;
    len = 0xbeef;
    status = pNtQueryLicenseValue(&name, &type, buffer, sizeof(buffer), &len);
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND, "NtQueryLicenseValue unexpected succeeded\n");
    ok(type == 0xdead, "expected unmodified value for type, got %u\n", type);
    ok(len == 0xbeef, "expected unmodified value for len, got %u\n", len);

    pRtlFreeUnicodeString(&name);

    /* test with REG_SZ license key */
    pRtlCreateUnicodeStringFromAsciiz(&name, "Kernel-MUI-Language-Allowed");

    type = 0xdead;
    len = 0xbeef;
    status = pNtQueryLicenseValue(NULL, &type, buffer, sizeof(buffer), &len);
    ok(status == STATUS_INVALID_PARAMETER, "NtQueryLicenseValue returned %08x, expected STATUS_INVALID_PARAMETER\n", status);
    ok(type == 0xdead, "expected unmodified value for type, got %u\n", type);
    ok(len == 0xbeef, "expected unmodified value for len, got %u\n", len);

    type = 0xdead;
    status = pNtQueryLicenseValue(&name, &type, buffer, sizeof(buffer), NULL);
    ok(status == STATUS_INVALID_PARAMETER, "NtQueryLicenseValue returned %08x, expected STATUS_INVALID_PARAMETER\n", status);
    ok(type == 0xdead, "expected unmodified value for type, got %u\n", type);

    type = 0xdead;
    len = 0;
    status = pNtQueryLicenseValue(&name, &type, buffer, 0, &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "NtQueryLicenseValue returned %08x, expected STATUS_BUFFER_TOO_SMALL\n", status);
    ok(type == REG_SZ, "expected type = REG_SZ, got %u\n", type);
    ok(len == sizeof(emptyW), "expected len = %u, got %u\n", (DWORD)sizeof(emptyW), len);

    len = 0;
    status = pNtQueryLicenseValue(&name, NULL, buffer, 0, &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "NtQueryLicenseValue returned %08x, expected STATUS_BUFFER_TOO_SMALL\n", status);
    ok(len == sizeof(emptyW), "expected len = %u, got %u\n", (DWORD)sizeof(emptyW), len);

    type = 0xdead;
    len = 0;
    memset(buffer, 0x11, sizeof(buffer));
    status = pNtQueryLicenseValue(&name, &type, buffer, sizeof(buffer), &len);
    ok(status == STATUS_SUCCESS, "NtQueryLicenseValue returned %08x, expected STATUS_SUCCESS\n", status);
    ok(type == REG_SZ, "expected type = REG_SZ, got %u\n", type);
    ok(len == sizeof(emptyW), "expected len = %u, got %u\n", (DWORD)sizeof(emptyW), len);
    ok(!memcmp(buffer, emptyW, sizeof(emptyW)), "unexpected buffer content\n");

    type = 0xdead;
    len = 0;
    memset(buffer, 0x11, sizeof(buffer));
    status = pNtQueryLicenseValue(&name, &type, buffer, 2, &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "NtQueryLicenseValue returned %08x, expected STATUS_BUFFER_TOO_SMALL\n", status);
    ok(type == REG_SZ, "expected type REG_SZ, got %u\n", type);
    ok(len == sizeof(emptyW), "expected len = %u, got %u\n", (DWORD)sizeof(emptyW), len);
    ok(buffer[0] == 0x1111, "expected buffer[0] = 0x1111, got %u\n", buffer[0]);

    pRtlFreeUnicodeString(&name);

    /* test with REG_DWORD license key */
    pRtlCreateUnicodeStringFromAsciiz(&name, "Kernel-MUI-Number-Allowed");

    type = 0xdead;
    len = 0xbeef;
    status = pNtQueryLicenseValue(NULL, &type, &value, sizeof(value), &len);
    ok(status == STATUS_INVALID_PARAMETER, "NtQueryLicenseValue returned %08x, expected STATUS_INVALID_PARAMETER\n", status);
    ok(type == 0xdead, "expected unmodified value for type, got %u\n", type);
    ok(len == 0xbeef, "expected unmodified value for len, got %u\n", len);

    type = 0xdead;
    status = pNtQueryLicenseValue(&name, &type, &value, sizeof(value), NULL);
    ok(status == STATUS_INVALID_PARAMETER, "NtQueryLicenseValue returned %08x, expected STATUS_INVALID_PARAMETER\n", status);
    ok(type == 0xdead, "expected unmodified value for type, got %u\n", type);

    type = 0xdead;
    len = 0;
    status = pNtQueryLicenseValue(&name, &type, &value, 0, &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "NtQueryLicenseValue returned %08x, expected STATUS_BUFFER_TOO_SMALL\n", status);
    ok(type == REG_DWORD, "expected type = REG_DWORD, got %u\n", type);
    ok(len == sizeof(value), "expected len = %u, got %u\n", (DWORD)sizeof(value), len);

    len = 0;
    status = pNtQueryLicenseValue(&name, NULL, &value, 0, &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "NtQueryLicenseValue returned %08x, expected STATUS_BUFFER_TOO_SMALL\n", status);
    ok(len == sizeof(value), "expected len = %u, got %u\n", (DWORD)sizeof(value), len);

    type = 0xdead;
    len = 0;
    value = 0xdeadbeef;
    status = pNtQueryLicenseValue(&name, &type, &value, sizeof(value), &len);
    ok(status == STATUS_SUCCESS, "NtQueryLicenseValue returned %08x, expected STATUS_SUCCESS\n", status);
    ok(type == REG_DWORD, "expected type = REG_DWORD, got %u\n", type);
    ok(len == sizeof(value), "expected len = %u, got %u\n", (DWORD)sizeof(value), len);
    ok(value != 0xdeadbeef, "expected value != 0xdeadbeef\n");

    type = 0xdead;
    len = 0;
    status = pNtQueryLicenseValue(&name, &type, &value, 2, &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "NtQueryLicenseValue returned %08x, expected STATUS_BUFFER_TOO_SMALL\n", status);
    ok(type == REG_DWORD, "expected type REG_DWORD, got %u\n", type);
    ok(len == sizeof(value), "expected len = %u, got %u\n", (DWORD)sizeof(value), len);

    pRtlFreeUnicodeString(&name);
}

static void test_RtlpNtQueryValueKey(void)
{
    NTSTATUS status;

    status = pRtlpNtQueryValueKey(NULL, NULL, NULL, NULL, NULL);
    ok(status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got: 0x%08x\n", status);
}

static void test_symlinks(void)
{
    static const WCHAR linkW[] = {'l','i','n','k',0};
    static const WCHAR valueW[] = {'v','a','l','u','e',0};
    static const WCHAR symlinkW[] = {'S','y','m','b','o','l','i','c','L','i','n','k','V','a','l','u','e',0};
    static const WCHAR targetW[] = {'\\','t','a','r','g','e','t',0};
    static UNICODE_STRING null_str;
    char buffer[1024];
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    WCHAR *target;
    UNICODE_STRING symlink_str, link_str, target_str, value_str;
    HANDLE root, key, link;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;
    DWORD target_len, len, dw;

    pRtlInitUnicodeString( &link_str, linkW );
    pRtlInitUnicodeString( &symlink_str, symlinkW );
    pRtlInitUnicodeString( &target_str, targetW + 1 );
    pRtlInitUnicodeString( &value_str, valueW );

    target_len = winetestpath.Length + sizeof(targetW);
    target = pRtlAllocateHeap( GetProcessHeap(), 0, target_len + sizeof(targetW) /*for loop test*/ );
    memcpy( target, winetestpath.Buffer, winetestpath.Length );
    memcpy( target + winetestpath.Length/sizeof(WCHAR), targetW, sizeof(targetW) );

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = 0;
    attr.ObjectName = &winetestpath;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = pNtCreateKey( &root, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );

    attr.RootDirectory = root;
    attr.ObjectName = &link_str;
    status = pNtCreateKey( &link, KEY_ALL_ACCESS, &attr, 0, 0, REG_OPTION_CREATE_LINK, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );

    /* REG_SZ is not allowed */
    status = pNtSetValueKey( link, &symlink_str, 0, REG_SZ, target, target_len );
    ok( status == STATUS_ACCESS_DENIED, "NtSetValueKey wrong status 0x%08x\n", status );
    status = pNtSetValueKey( link, &symlink_str, 0, REG_LINK, target, target_len - sizeof(WCHAR) );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08x\n", status );
    /* other values are not allowed */
    status = pNtSetValueKey( link, &link_str, 0, REG_LINK, target, target_len - sizeof(WCHAR) );
    ok( status == STATUS_ACCESS_DENIED, "NtSetValueKey wrong status 0x%08x\n", status );

    /* try opening the target through the link */

    attr.ObjectName = &link_str;
    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "NtOpenKey wrong status 0x%08x\n", status );

    attr.ObjectName = &target_str;
    status = pNtCreateKey( &key, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );

    dw = 0xbeef;
    status = pNtSetValueKey( key, &value_str, 0, REG_DWORD, &dw, sizeof(dw) );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08x\n", status );
    pNtClose( key );

    attr.ObjectName = &link_str;
    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08x\n", status );

    len = sizeof(buffer);
    status = pNtQueryValueKey( key, &value_str, KeyValuePartialInformation, info, len, &len );
    ok( status == STATUS_SUCCESS, "NtQueryValueKey failed: 0x%08x\n", status );
    ok( len == FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION,Data) + sizeof(DWORD), "wrong len %u\n", len );

    status = pNtQueryValueKey( key, &symlink_str, KeyValuePartialInformation, info, len, &len );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "NtQueryValueKey failed: 0x%08x\n", status );

    /* REG_LINK can be created in non-link keys */
    status = pNtSetValueKey( key, &symlink_str, 0, REG_LINK, target, target_len - sizeof(WCHAR) );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08x\n", status );
    len = sizeof(buffer);
    status = pNtQueryValueKey( key, &symlink_str, KeyValuePartialInformation, info, len, &len );
    ok( status == STATUS_SUCCESS, "NtQueryValueKey failed: 0x%08x\n", status );
    ok( len == FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION,Data) + target_len - sizeof(WCHAR),
        "wrong len %u\n", len );
    status = pNtDeleteValueKey( key, &symlink_str );
    ok( status == STATUS_SUCCESS, "NtDeleteValueKey failed: 0x%08x\n", status );

    pNtClose( key );

    attr.Attributes = 0;
    status = pNtCreateKey( &key, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );

    len = sizeof(buffer);
    status = pNtQueryValueKey( key, &value_str, KeyValuePartialInformation, info, len, &len );
    ok( status == STATUS_SUCCESS, "NtQueryValueKey failed: 0x%08x\n", status );
    ok( len == FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION,Data) + sizeof(DWORD), "wrong len %u\n", len );

    status = pNtQueryValueKey( key, &symlink_str, KeyValuePartialInformation, info, len, &len );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "NtQueryValueKey failed: 0x%08x\n", status );
    pNtClose( key );

    /* now open the symlink itself */

    attr.RootDirectory = root;
    attr.Attributes = OBJ_OPENLINK;
    attr.ObjectName = &link_str;
    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08x\n", status );

    len = sizeof(buffer);
    status = pNtQueryValueKey( key, &symlink_str, KeyValuePartialInformation, info, len, &len );
    ok( status == STATUS_SUCCESS, "NtQueryValueKey failed: 0x%08x\n", status );
    ok( len == FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION,Data) + target_len - sizeof(WCHAR),
        "wrong len %u\n", len );
    pNtClose( key );

    status = pNtCreateKey( &key, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );
    len = sizeof(buffer);
    status = pNtQueryValueKey( key, &symlink_str, KeyValuePartialInformation, info, len, &len );
    ok( status == STATUS_SUCCESS, "NtQueryValueKey failed: 0x%08x\n", status );
    ok( len == FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION,Data) + target_len - sizeof(WCHAR),
        "wrong len %u\n", len );
    pNtClose( key );

    if (0)  /* crashes the Windows kernel on some Vista systems */
    {
        /* reopen the link from itself */

        attr.RootDirectory = link;
        attr.Attributes = OBJ_OPENLINK;
        attr.ObjectName = &null_str;
        status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
        ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08x\n", status );
        len = sizeof(buffer);
        status = pNtQueryValueKey( key, &symlink_str, KeyValuePartialInformation, info, len, &len );
        ok( status == STATUS_SUCCESS, "NtQueryValueKey failed: 0x%08x\n", status );
        ok( len == FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION,Data) + target_len - sizeof(WCHAR),
            "wrong len %u\n", len );
        pNtClose( key );

        status = pNtCreateKey( &key, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
        ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );
        len = sizeof(buffer);
        status = pNtQueryValueKey( key, &symlink_str, KeyValuePartialInformation, info, len, &len );
        ok( status == STATUS_SUCCESS, "NtQueryValueKey failed: 0x%08x\n", status );
        ok( len == FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION,Data) + target_len - sizeof(WCHAR),
            "wrong len %u\n", len );
        pNtClose( key );
    }

    if (0)  /* crashes the Windows kernel in most versions */
    {
        attr.RootDirectory = link;
        attr.Attributes = 0;
        attr.ObjectName = &null_str;
        status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
        ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08x\n", status );
        len = sizeof(buffer);
        status = pNtQueryValueKey( key, &symlink_str, KeyValuePartialInformation, info, len, &len );
        ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "NtQueryValueKey failed: 0x%08x\n", status );
        pNtClose( key );

        status = pNtCreateKey( &key, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
        ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );
        len = sizeof(buffer);
        status = pNtQueryValueKey( key, &symlink_str, KeyValuePartialInformation, info, len, &len );
        ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "NtQueryValueKey failed: 0x%08x\n", status );
        pNtClose( key );
    }

    /* target with terminating null doesn't work */
    status = pNtSetValueKey( link, &symlink_str, 0, REG_LINK, target, target_len );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08x\n", status );
    attr.RootDirectory = root;
    attr.Attributes = 0;
    attr.ObjectName = &link_str;
    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "NtOpenKey wrong status 0x%08x\n", status );

    /* relative symlink, works only on win2k */
    status = pNtSetValueKey( link, &symlink_str, 0, REG_LINK, targetW+1, sizeof(targetW)-2*sizeof(WCHAR) );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08x\n", status );
    attr.ObjectName = &link_str;
    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS || status == STATUS_OBJECT_NAME_NOT_FOUND,
        "NtOpenKey wrong status 0x%08x\n", status );

    status = pNtCreateKey( &key, KEY_ALL_ACCESS, &attr, 0, 0, REG_OPTION_CREATE_LINK, 0 );
    ok( status == STATUS_OBJECT_NAME_COLLISION, "NtCreateKey failed: 0x%08x\n", status );

    status = pNtDeleteKey( link );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08x\n", status );
    pNtClose( link );

    attr.ObjectName = &target_str;
    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08x\n", status );
    status = pNtDeleteKey( key );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08x\n", status );
    pNtClose( key );

    /* symlink loop */

    status = pNtCreateKey( &link, KEY_ALL_ACCESS, &attr, 0, 0, REG_OPTION_CREATE_LINK, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );
    memcpy( target + target_len/sizeof(WCHAR) - 1, targetW, sizeof(targetW) );
    status = pNtSetValueKey( link, &symlink_str, 0, REG_LINK,
        target, target_len + sizeof(targetW) - sizeof(WCHAR) );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08x\n", status );

    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND || status == STATUS_NAME_TOO_LONG,
        "NtOpenKey failed: 0x%08x\n", status );

    attr.Attributes = OBJ_OPENLINK;
    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08x\n", status );
    pNtClose( key );

    status = pNtDeleteKey( link );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08x\n", status );
    pNtClose( link );

    status = pNtDeleteKey( root );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08x\n", status );
    pNtClose( root );

    pRtlFreeHeap(GetProcessHeap(), 0, target);
}

static WCHAR valueW[] = {'v','a','l','u','e'};
static UNICODE_STRING value_str = { sizeof(valueW), sizeof(valueW), valueW };
static const DWORD ptr_size = 8 * sizeof(void*);

static DWORD get_key_value( HANDLE root, const char *name, DWORD flags )
{
    char tmp[32];
    NTSTATUS status;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    HANDLE key;
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)tmp;
    DWORD dw, len = sizeof(tmp);

    attr.Length = sizeof(attr);
    attr.RootDirectory = root;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &str;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    pRtlCreateUnicodeStringFromAsciiz( &str, name );

    status = pNtCreateKey( &key, flags | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    if (status == STATUS_OBJECT_NAME_NOT_FOUND) return 0;
    ok( status == STATUS_SUCCESS, "%08x: NtCreateKey failed: 0x%08x\n", flags, status );

    status = pNtQueryValueKey( key, &value_str, KeyValuePartialInformation, info, len, &len );
    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
        dw = 0;
    else
    {
        ok( status == STATUS_SUCCESS, "%08x: NtQueryValueKey failed: 0x%08x\n", flags, status );
        dw = *(DWORD *)info->Data;
    }
    pNtClose( key );
    pRtlFreeUnicodeString( &str );
    return dw;
}

static void _check_key_value( int line, HANDLE root, const char *name, DWORD flags, DWORD expect )
{
    DWORD dw = get_key_value( root, name, flags );
    ok_(__FILE__,line)( dw == expect, "%08x: wrong value %u/%u\n", flags, dw, expect );
}
#define check_key_value(root,name,flags,expect) _check_key_value( __LINE__, root, name, flags, expect )

static void test_redirection(void)
{
    static const WCHAR softwareW[] = {'\\','R','e','g','i','s','t','r','y','\\',
                                      'M','a','c','h','i','n','e','\\',
                                      'S','o','f','t','w','a','r','e',0};
    static const WCHAR wownodeW[] = {'\\','R','e','g','i','s','t','r','y','\\',
                                     'M','a','c','h','i','n','e','\\',
                                     'S','o','f','t','w','a','r','e','\\',
                                     'W','o','w','6','4','3','2','N','o','d','e',0};
    static const WCHAR wine64W[] = {'\\','R','e','g','i','s','t','r','y','\\',
                                    'M','a','c','h','i','n','e','\\',
                                    'S','o','f','t','w','a','r','e','\\',
                                    'W','i','n','e',0};
    static const WCHAR wine32W[] = {'\\','R','e','g','i','s','t','r','y','\\',
                                    'M','a','c','h','i','n','e','\\',
                                    'S','o','f','t','w','a','r','e','\\',
                                    'W','o','w','6','4','3','2','N','o','d','e','\\',
                                    'W','i','n','e',0};
    static const WCHAR key64W[] = {'\\','R','e','g','i','s','t','r','y','\\',
                                   'M','a','c','h','i','n','e','\\',
                                   'S','o','f','t','w','a','r','e','\\',
                                   'W','i','n','e','\\','W','i','n','e','t','e','s','t',0};
    static const WCHAR key32W[] = {'\\','R','e','g','i','s','t','r','y','\\',
                                   'M','a','c','h','i','n','e','\\',
                                   'S','o','f','t','w','a','r','e','\\',
                                   'W','o','w','6','4','3','2','N','o','d','e','\\',
                                   'W','i','n','e','\\', 'W','i','n','e','t','e','s','t',0};
    static const WCHAR classes64W[] = {'\\','R','e','g','i','s','t','r','y','\\',
                                       'M','a','c','h','i','n','e','\\',
                                       'S','o','f','t','w','a','r','e','\\',
                                       'C','l','a','s','s','e','s','\\',
                                       'W','i','n','e',0};
    static const WCHAR classes32W[] = {'\\','R','e','g','i','s','t','r','y','\\',
                                       'M','a','c','h','i','n','e','\\',
                                       'S','o','f','t','w','a','r','e','\\',
                                       'C','l','a','s','s','e','s','\\',
                                       'W','o','w','6','4','3','2','N','o','d','e','\\',
                                       'W','i','n','e',0};
    NTSTATUS status;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    char buffer[1024];
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    DWORD dw, len;
    HANDLE key, root32, root64, key32, key64;
    BOOL is_vista = FALSE;

    if (ptr_size != 64)
    {
        ULONG is_wow64, len;
        if (pNtQueryInformationProcess( GetCurrentProcess(), ProcessWow64Information,
                                        &is_wow64, sizeof(is_wow64), &len ) ||
            !is_wow64)
        {
            trace( "Not on Wow64, no redirection\n" );
            return;
        }
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &str;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    pRtlInitUnicodeString( &str, wine64W );
    status = pNtCreateKey( &root64, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );

    pRtlInitUnicodeString( &str, wine32W );
    status = pNtCreateKey( &root32, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );

    pRtlInitUnicodeString( &str, key64W );
    status = pNtCreateKey( &key64, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );

    pRtlInitUnicodeString( &str, key32W );
    status = pNtCreateKey( &key32, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );

    dw = 64;
    status = pNtSetValueKey( key64, &value_str, 0, REG_DWORD, &dw, sizeof(dw) );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08x\n", status );

    dw = 32;
    status = pNtSetValueKey( key32, &value_str, 0, REG_DWORD, &dw, sizeof(dw) );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08x\n", status );

    len = sizeof(buffer);
    status = pNtQueryValueKey( key32, &value_str, KeyValuePartialInformation, info, len, &len );
    ok( status == STATUS_SUCCESS, "NtQueryValueKey failed: 0x%08x\n", status );
    dw = *(DWORD *)info->Data;
    ok( dw == 32, "wrong value %u\n", dw );

    len = sizeof(buffer);
    status = pNtQueryValueKey( key64, &value_str, KeyValuePartialInformation, info, len, &len );
    ok( status == STATUS_SUCCESS, "NtQueryValueKey failed: 0x%08x\n", status );
    dw = *(DWORD *)info->Data;
    ok( dw == 64, "wrong value %u\n", dw );

    pRtlInitUnicodeString( &str, softwareW );
    status = pNtCreateKey( &key, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );

    if (ptr_size == 32)
    {
        /* the Vista mechanism allows opening Wow6432Node from a 32-bit key too */
        /* the new (and simpler) Win7 mechanism doesn't */
        if (get_key_value( key, "Wow6432Node\\Wine\\Winetest", 0 ) == 32)
        {
            trace( "using Vista-style Wow6432Node handling\n" );
            is_vista = TRUE;
        }
        check_key_value( key, "Wine\\Winetest", 0, 32 );
        check_key_value( key, "Wine\\Winetest", KEY_WOW64_64KEY, is_vista ? 64 : 32 );
        check_key_value( key, "Wine\\Winetest", KEY_WOW64_32KEY, 32 );
        check_key_value( key, "Wow6432Node\\Wine\\Winetest", 0, is_vista ? 32 : 0 );
        check_key_value( key, "Wow6432Node\\Wine\\Winetest", KEY_WOW64_64KEY, is_vista ? 64 : 0 );
        check_key_value( key, "Wow6432Node\\Wine\\Winetest", KEY_WOW64_32KEY, is_vista ? 32 : 0 );
    }
    else
    {
        check_key_value( key, "Wine\\Winetest", 0, 64 );
        check_key_value( key, "Wow6432Node\\Wine\\Winetest", 0, 32 );
    }
    pNtClose( key );

    if (ptr_size == 32)
    {
        status = pNtCreateKey( &key, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
        ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );
        dw = get_key_value( key, "Wine\\Winetest", 0 );
        ok( dw == 64 || broken(dw == 32) /* xp64 */, "wrong value %u\n", dw );
        check_key_value( key, "Wine\\Winetest", KEY_WOW64_64KEY, 64 );
        check_key_value( key, "Wine\\Winetest", KEY_WOW64_32KEY, 32 );
        check_key_value( key, "Wow6432Node\\Wine\\Winetest", 0, 32 );
        dw = get_key_value( key, "Wow6432Node\\Wine\\Winetest", KEY_WOW64_64KEY );
        ok( dw == 32 || broken(dw == 64) /* xp64 */, "wrong value %u\n", dw );
        check_key_value( key, "Wow6432Node\\Wine\\Winetest", KEY_WOW64_32KEY, 32 );
        pNtClose( key );

        status = pNtCreateKey( &key, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
        ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );
        check_key_value( key, "Wine\\Winetest", 0, 32 );
        check_key_value( key, "Wine\\Winetest", KEY_WOW64_64KEY, is_vista ? 64 : 32 );
        check_key_value( key, "Wine\\Winetest", KEY_WOW64_32KEY, 32 );
        check_key_value( key, "Wow6432Node\\Wine\\Winetest", 0, is_vista ? 32 : 0 );
        check_key_value( key, "Wow6432Node\\Wine\\Winetest", KEY_WOW64_64KEY, is_vista ? 64 : 0 );
        check_key_value( key, "Wow6432Node\\Wine\\Winetest", KEY_WOW64_32KEY, is_vista ? 32 : 0 );
        pNtClose( key );
    }

    check_key_value( 0, "\\Registry\\Machine\\Software\\Wine\\Winetest", 0, ptr_size );
    check_key_value( 0, "\\Registry\\Machine\\Software\\Wow6432Node\\Wine\\Winetest", 0, 32 );
    if (ptr_size == 64)
    {
        /* KEY_WOW64 flags have no effect on 64-bit */
        check_key_value( 0, "\\Registry\\Machine\\Software\\Wine\\Winetest", KEY_WOW64_64KEY, 64 );
        check_key_value( 0, "\\Registry\\Machine\\Software\\Wine\\Winetest", KEY_WOW64_32KEY, 64 );
        check_key_value( 0, "\\Registry\\Machine\\Software\\Wow6432Node\\Wine\\Winetest", KEY_WOW64_64KEY, 32 );
        check_key_value( 0, "\\Registry\\Machine\\Software\\Wow6432Node\\Wine\\Winetest", KEY_WOW64_32KEY, 32 );
    }
    else
    {
        check_key_value( 0, "\\Registry\\Machine\\Software\\Wine\\Winetest", KEY_WOW64_64KEY, 64 );
        check_key_value( 0, "\\Registry\\Machine\\Software\\Wine\\Winetest", KEY_WOW64_32KEY, 32 );
        check_key_value( 0, "\\Registry\\Machine\\Software\\Wow6432Node\\Wine\\Winetest", KEY_WOW64_64KEY, is_vista ? 64 : 32 );
        check_key_value( 0, "\\Registry\\Machine\\Software\\Wow6432Node\\Wine\\Winetest", KEY_WOW64_32KEY, 32 );
    }

    pRtlInitUnicodeString( &str, wownodeW );
    status = pNtCreateKey( &key, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );
    check_key_value( key, "Wine\\Winetest", 0, 32 );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_64KEY, (ptr_size == 64) ? 32 : (is_vista ? 64 : 32) );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_32KEY, 32 );
    pNtClose( key );

    if (ptr_size == 32)
    {
        status = pNtCreateKey( &key, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
        ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );
        dw = get_key_value( key, "Wine\\Winetest", 0 );
        ok( dw == (is_vista ? 64 : 32) || broken(dw == 32) /* xp64 */, "wrong value %u\n", dw );
        check_key_value( key, "Wine\\Winetest", KEY_WOW64_64KEY, is_vista ? 64 : 32 );
        check_key_value( key, "Wine\\Winetest", KEY_WOW64_32KEY, 32 );
        pNtClose( key );

        status = pNtCreateKey( &key, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
        ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );
        check_key_value( key, "Wine\\Winetest", 0, 32 );
        check_key_value( key, "Wine\\Winetest", KEY_WOW64_64KEY, is_vista ? 64 : 32 );
        check_key_value( key, "Wine\\Winetest", KEY_WOW64_32KEY, 32 );
        pNtClose( key );
    }

    pRtlInitUnicodeString( &str, wine32W );
    status = pNtCreateKey( &key, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );
    check_key_value( key, "Winetest", 0, 32 );
    check_key_value( key, "Winetest", KEY_WOW64_64KEY, (ptr_size == 32 && is_vista) ? 64 : 32 );
    check_key_value( key, "Winetest", KEY_WOW64_32KEY, 32 );
    pNtClose( key );

    if (ptr_size == 32)
    {
        status = pNtCreateKey( &key, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
        ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );
        dw = get_key_value( key, "Winetest", 0 );
        ok( dw == 32 || (is_vista && dw == 64), "wrong value %u\n", dw );
        check_key_value( key, "Winetest", KEY_WOW64_64KEY, is_vista ? 64 : 32 );
        check_key_value( key, "Winetest", KEY_WOW64_32KEY, 32 );
        pNtClose( key );

        status = pNtCreateKey( &key, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
        ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );
        check_key_value( key, "Winetest", 0, 32 );
        check_key_value( key, "Winetest", KEY_WOW64_64KEY, is_vista ? 64 : 32 );
        check_key_value( key, "Winetest", KEY_WOW64_32KEY, 32 );
        pNtClose( key );
    }

    pRtlInitUnicodeString( &str, wine64W );
    status = pNtCreateKey( &key, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );
    check_key_value( key, "Winetest", 0, ptr_size );
    check_key_value( key, "Winetest", KEY_WOW64_64KEY, is_vista ? 64 : ptr_size );
    check_key_value( key, "Winetest", KEY_WOW64_32KEY, ptr_size );
    pNtClose( key );

    if (ptr_size == 32)
    {
        status = pNtCreateKey( &key, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
        ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );
        dw = get_key_value( key, "Winetest", 0 );
        ok( dw == 64 || broken(dw == 32) /* xp64 */, "wrong value %u\n", dw );
        check_key_value( key, "Winetest", KEY_WOW64_64KEY, 64 );
        dw = get_key_value( key, "Winetest", KEY_WOW64_32KEY );
        todo_wine ok( dw == 32, "wrong value %u\n", dw );
        pNtClose( key );

        status = pNtCreateKey( &key, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
        ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );
        check_key_value( key, "Winetest", 0, 32 );
        check_key_value( key, "Winetest", KEY_WOW64_64KEY, is_vista ? 64 : 32 );
        check_key_value( key, "Winetest", KEY_WOW64_32KEY, 32 );
        pNtClose( key );
    }

    status = pNtDeleteKey( key32 );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08x\n", status );
    pNtClose( key32 );

    status = pNtDeleteKey( key64 );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08x\n", status );
    pNtClose( key64 );

    pNtDeleteKey( root32 );
    pNtClose( root32 );
    pNtDeleteKey( root64 );
    pNtClose( root64 );

    /* Software\Classes is shared/reflected so behavior is different */

    pRtlInitUnicodeString( &str, classes64W );
    status = pNtCreateKey( &key64, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    if (status == STATUS_ACCESS_DENIED)
    {
        skip("Not authorized to modify the Classes key\n");
        return;
    }
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );

    pRtlInitUnicodeString( &str, classes32W );
    status = pNtCreateKey( &key32, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );

    dw = 64;
    status = pNtSetValueKey( key64, &value_str, 0, REG_DWORD, &dw, sizeof(dw) );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08x\n", status );
    pNtClose( key64 );

    dw = 32;
    status = pNtSetValueKey( key32, &value_str, 0, REG_DWORD, &dw, sizeof(dw) );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08x\n", status );
    pNtClose( key32 );

    pRtlInitUnicodeString( &str, classes64W );
    status = pNtCreateKey( &key64, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );
    len = sizeof(buffer);
    status = pNtQueryValueKey( key64, &value_str, KeyValuePartialInformation, info, len, &len );
    ok( status == STATUS_SUCCESS, "NtQueryValueKey failed: 0x%08x\n", status );
    dw = *(DWORD *)info->Data;
    ok( dw == ptr_size, "wrong value %u\n", dw );

    pRtlInitUnicodeString( &str, classes32W );
    status = pNtCreateKey( &key32, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08x\n", status );
    len = sizeof(buffer);
    status = pNtQueryValueKey( key32, &value_str, KeyValuePartialInformation, info, len, &len );
    ok( status == STATUS_SUCCESS, "NtQueryValueKey failed: 0x%08x\n", status );
    dw = *(DWORD *)info->Data;
    ok( dw == 32, "wrong value %u\n", dw );

    pNtDeleteKey( key32 );
    pNtClose( key32 );
    pNtDeleteKey( key64 );
    pNtClose( key64 );
}

static void test_long_value_name(void)
{
    HANDLE key;
    NTSTATUS status, expected;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING ValName;
    DWORD i;

    InitializeObjectAttributes(&attr, &winetestpath, 0, 0, 0);
    status = pNtOpenKey(&key, KEY_WRITE|KEY_READ, &attr);
    ok(status == STATUS_SUCCESS, "NtOpenKey Failed: 0x%08x\n", status);

    ValName.MaximumLength = 0xfffc;
    ValName.Length = ValName.MaximumLength - sizeof(WCHAR);
    ValName.Buffer = HeapAlloc(GetProcessHeap(), 0, ValName.MaximumLength);
    for (i = 0; i < ValName.Length / sizeof(WCHAR); i++)
        ValName.Buffer[i] = 'a';
    ValName.Buffer[i] = 0;

    status = pNtDeleteValueKey(key, &ValName);
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND, "NtDeleteValueKey with nonexistent long value name returned 0x%08x\n", status);
    status = pNtSetValueKey(key, &ValName, 0, REG_DWORD, &i, sizeof(i));
    ok(status == STATUS_INVALID_PARAMETER || broken(status == STATUS_SUCCESS) /* nt4 */,
       "NtSetValueKey with long value name returned 0x%08x\n", status);
    expected = (status == STATUS_SUCCESS) ? STATUS_SUCCESS : STATUS_OBJECT_NAME_NOT_FOUND;
    status = pNtDeleteValueKey(key, &ValName);
    ok(status == expected, "NtDeleteValueKey with long value name returned 0x%08x\n", status);

    status = pNtQueryValueKey(key, &ValName, KeyValueBasicInformation, NULL, 0, &i);
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND, "NtQueryValueKey with nonexistent long value name returned 0x%08x\n", status);

    pRtlFreeUnicodeString(&ValName);
    pNtClose(key);
}

static void test_NtQueryKey(void)
{
    HANDLE key;
    NTSTATUS status;
    OBJECT_ATTRIBUTES attr;
    ULONG length, len;
    KEY_NAME_INFORMATION *info = NULL;
    UNICODE_STRING str;

    InitializeObjectAttributes(&attr, &winetestpath, 0, 0, 0);
    status = pNtOpenKey(&key, KEY_READ, &attr);
    ok(status == STATUS_SUCCESS, "NtOpenKey Failed: 0x%08x\n", status);

    status = pNtQueryKey(key, KeyNameInformation, NULL, 0, &length);
    if (status == STATUS_INVALID_PARAMETER) {
        win_skip("KeyNameInformation is not supported\n");
        pNtClose(key);
        return;
    }
    todo_wine ok(status == STATUS_BUFFER_TOO_SMALL, "NtQueryKey Failed: 0x%08x\n", status);
    info = HeapAlloc(GetProcessHeap(), 0, length);

    /* non-zero buffer size, but insufficient */
    status = pNtQueryKey(key, KeyNameInformation, info, sizeof(*info), &len);
    ok(status == STATUS_BUFFER_OVERFLOW, "NtQueryKey Failed: 0x%08x\n", status);
    ok(length == len, "got %d, expected %d\n", len, length);
    ok(info->NameLength == winetestpath.Length, "got %d, expected %d\n",
       info->NameLength, winetestpath.Length);

    /* correct buffer size */
    status = pNtQueryKey(key, KeyNameInformation, info, length, &len);
    ok(status == STATUS_SUCCESS, "NtQueryKey Failed: 0x%08x\n", status);
    ok(length == len, "got %d, expected %d\n", len, length);

    str.Buffer = info->Name;
    str.Length = info->NameLength;
    ok(pRtlCompareUnicodeString(&winetestpath, &str, TRUE) == 0,
       "got %s, expected %s\n",
       wine_dbgstr_wn(str.Buffer, str.Length/sizeof(WCHAR)),
       wine_dbgstr_wn(winetestpath.Buffer, winetestpath.Length/sizeof(WCHAR)));

    HeapFree(GetProcessHeap(), 0, info);
    pNtClose(key);
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

    test_NtCreateKey();
    test_NtOpenKey();
    test_NtSetValueKey();
    test_RtlCheckRegistryKey();
    test_RtlOpenCurrentUser();
    test_RtlQueryRegistryValues();
    test_RtlpNtQueryValueKey();
    test_NtFlushKey();
    test_NtQueryKey();
    test_NtQueryLicenseKey();
    test_NtQueryValueKey();
    test_long_value_name();
    test_NtDeleteKey();
    test_symlinks();
    test_redirection();

    pRtlFreeUnicodeString(&winetestpath);

    FreeLibrary(hntdll);
}

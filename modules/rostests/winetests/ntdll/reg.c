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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnls.h"
#include "winternl.h"
#include "wine/test.h"
#ifdef __REACTOS__
#define REG_APP_HIVE 0x10
#endif

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
static NTSTATUS (WINAPI * pNtOpenKeyEx)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, ULONG);
static NTSTATUS (WINAPI * pNtClose)(IN HANDLE);
static NTSTATUS (WINAPI * pNtEnumerateKey)(HANDLE, ULONG, KEY_INFORMATION_CLASS, void *, DWORD, DWORD *);
static NTSTATUS (WINAPI * pNtEnumerateValueKey)(HANDLE, ULONG, KEY_VALUE_INFORMATION_CLASS, void *, DWORD, DWORD *);
static NTSTATUS (WINAPI * pNtFlushKey)(HANDLE);
static NTSTATUS (WINAPI * pNtDeleteKey)(HANDLE);
static NTSTATUS (WINAPI * pNtCreateKey)( PHANDLE retkey, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                             ULONG TitleIndex, const UNICODE_STRING *class, ULONG options,
                             PULONG dispos );
static NTSTATUS (WINAPI * pNtQueryKey)(HANDLE,KEY_INFORMATION_CLASS,PVOID,ULONG,PULONG);
static NTSTATUS (WINAPI * pNtQueryLicenseValue)(const UNICODE_STRING *,ULONG *,PVOID,ULONG,ULONG *);
static NTSTATUS (WINAPI * pNtQueryObject)(HANDLE, OBJECT_INFORMATION_CLASS, void *, ULONG, ULONG *);
static NTSTATUS (WINAPI * pNtQueryValueKey)(HANDLE,const UNICODE_STRING *,KEY_VALUE_INFORMATION_CLASS,void *,DWORD,DWORD *);
static NTSTATUS (WINAPI * pNtSetValueKey)(HANDLE, const PUNICODE_STRING, ULONG,
                               ULONG, const void*, ULONG  );
static NTSTATUS (WINAPI * pRtlFormatCurrentUserKeyPath)(PUNICODE_STRING);
static LONG     (WINAPI * pRtlCompareUnicodeString)(const PUNICODE_STRING,const PUNICODE_STRING,BOOLEAN);
static BOOLEAN  (WINAPI * pRtlCreateUnicodeString)(PUNICODE_STRING, LPCWSTR);
static LPVOID   (WINAPI * pRtlReAllocateHeap)(IN PVOID, IN ULONG, IN PVOID, IN ULONG);
static NTSTATUS (WINAPI * pRtlAppendUnicodeToString)(PUNICODE_STRING, PCWSTR);
static NTSTATUS (WINAPI * pRtlUnicodeStringToAnsiString)(PSTRING, PUNICODE_STRING, BOOL);
static NTSTATUS (WINAPI * pRtlFreeHeap)(PVOID, ULONG, PVOID);
static LPVOID   (WINAPI * pRtlAllocateHeap)(PVOID,ULONG,ULONG);
static NTSTATUS (WINAPI * pRtlZeroMemory)(PVOID, ULONG);
static NTSTATUS (WINAPI * pRtlCreateRegistryKey)(ULONG, PWSTR);
static NTSTATUS (WINAPI * pRtlpNtQueryValueKey)(HANDLE,ULONG*,PBYTE,DWORD*,void *);
static NTSTATUS (WINAPI * pNtNotifyChangeKey)(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,ULONG,BOOLEAN,PVOID,ULONG,BOOLEAN);
static NTSTATUS (WINAPI * pNtNotifyChangeMultipleKeys)(HANDLE,ULONG,OBJECT_ATTRIBUTES*,HANDLE,PIO_APC_ROUTINE,
                                                       void*,IO_STATUS_BLOCK*,ULONG,BOOLEAN,void*,ULONG,BOOLEAN);
static NTSTATUS (WINAPI * pNtWaitForSingleObject)(HANDLE,BOOLEAN,const LARGE_INTEGER*);
static NTSTATUS (WINAPI * pNtLoadKeyEx)(const OBJECT_ATTRIBUTES*,OBJECT_ATTRIBUTES*,ULONG,HANDLE,HANDLE,ACCESS_MASK,HANDLE*,IO_STATUS_BLOCK*);

static HMODULE hntdll = 0;
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
    NTDLL_GET_PROC(NtEnumerateKey)
    NTDLL_GET_PROC(NtEnumerateValueKey)
    NTDLL_GET_PROC(NtFlushKey)
    NTDLL_GET_PROC(NtDeleteKey)
    NTDLL_GET_PROC(NtQueryKey)
    NTDLL_GET_PROC(NtQueryObject)
    NTDLL_GET_PROC(NtQueryValueKey)
    NTDLL_GET_PROC(NtSetValueKey)
    NTDLL_GET_PROC(NtOpenKey)
    NTDLL_GET_PROC(NtNotifyChangeKey)
    NTDLL_GET_PROC(RtlFormatCurrentUserKeyPath)
    NTDLL_GET_PROC(RtlCompareUnicodeString)
    NTDLL_GET_PROC(RtlReAllocateHeap)
    NTDLL_GET_PROC(RtlAppendUnicodeToString)
    NTDLL_GET_PROC(RtlUnicodeStringToAnsiString)
    NTDLL_GET_PROC(RtlFreeHeap)
    NTDLL_GET_PROC(RtlAllocateHeap)
    NTDLL_GET_PROC(RtlZeroMemory)
    NTDLL_GET_PROC(RtlCreateRegistryKey)
    NTDLL_GET_PROC(RtlpNtQueryValueKey)
    NTDLL_GET_PROC(RtlOpenCurrentUser)
    NTDLL_GET_PROC(NtWaitForSingleObject)
    NTDLL_GET_PROC(NtLoadKeyEx);

    /* optional functions */
    pNtQueryLicenseValue = (void *)GetProcAddress(hntdll, "NtQueryLicenseValue");
    pNtOpenKeyEx = (void *)GetProcAddress(hntdll, "NtOpenKeyEx");
    pNtNotifyChangeMultipleKeys = (void *)GetProcAddress(hntdll, "NtNotifyChangeMultipleKeys");

    return TRUE;
}
#undef NTDLL_GET_PROC

static void test_NtOpenKey(void)
{
    HANDLE key, subkey;
    NTSTATUS status;
    OBJECT_ATTRIBUTES attr;
    ACCESS_MASK am = KEY_READ;
    UNICODE_STRING str;

    /* All NULL */
    status = pNtOpenKey(NULL, 0, NULL);
    ok(status == STATUS_ACCESS_VIOLATION, "Expected STATUS_ACCESS_VIOLATION, got: 0x%08lx\n", status);

    /* NULL attributes */
    status = pNtOpenKey(&key, 0, NULL);
    ok(status == STATUS_ACCESS_VIOLATION, "Expected STATUS_ACCESS_VIOLATION, got: 0x%08lx\n", status);

    InitializeObjectAttributes(&attr, &winetestpath, 0, 0, 0);

    /* NULL key */
    status = pNtOpenKey(NULL, am, &attr);
    ok(status == STATUS_ACCESS_VIOLATION, "Expected STATUS_ACCESS_VIOLATION, got: 0x%08lx\n", status);

    /* Length > sizeof(OBJECT_ATTRIBUTES) */
    attr.Length *= 2;
    status = pNtOpenKey(&key, am, &attr);
    ok(status == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got: 0x%08lx\n", status);

    /* Zero accessmask */
    attr.Length = sizeof(attr);
    key = (HANDLE)0xdeadbeef;
    status = pNtOpenKey(&key, 0, &attr);
    todo_wine
    ok(status == STATUS_ACCESS_DENIED, "Expected STATUS_ACCESS_DENIED, got: 0x%08lx\n", status);
    todo_wine
    ok(!key, "key = %p\n", key);
    if (status == STATUS_SUCCESS) NtClose(key);

    /* Calling without parent key requires full registry path. */
    pRtlCreateUnicodeStringFromAsciiz( &str, "Machine" );
    InitializeObjectAttributes(&attr, &str, 0, 0, 0);
    key = (HANDLE)0xdeadbeef;
    status = pNtOpenKey(&key, KEY_READ, &attr);
    ok(status == STATUS_OBJECT_PATH_SYNTAX_BAD, "NtOpenKey Failed: 0x%08lx\n", status);
    ok(!key, "key = %p\n", key);
    pRtlFreeUnicodeString( &str );

    /* Open is case sensitive unless OBJ_CASE_INSENSITIVE is specified. */
    pRtlCreateUnicodeStringFromAsciiz( &str, "\\Registry\\Machine" );
    status = pNtOpenKey(&key, KEY_READ, &attr);
    ok(status == STATUS_OBJECT_PATH_NOT_FOUND || status == STATUS_SUCCESS /* Win10 1607+ */,
            "NtOpenKey Failed: 0x%08lx\n", status);
    if (!status) pNtClose( key );

    attr.Attributes = OBJ_CASE_INSENSITIVE;
    status = pNtOpenKey(&key, KEY_READ, &attr);
    ok(status == STATUS_SUCCESS, "NtOpenKey Failed: 0x%08lx\n", status);
    pNtClose(key);
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "" );
    status = pNtOpenKey(&key, KEY_READ, &attr);
    ok( status == STATUS_OBJECT_PATH_SYNTAX_BAD, "NtOpenKey failed: 0x%08lx\n", status );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "\\" );
    status = pNtOpenKey(&key, KEY_READ, &attr);
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "NtOpenKey failed: 0x%08lx\n", status );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "\\\\\\" );
    status = pNtOpenKey(&key, KEY_READ, &attr);
    ok( status == STATUS_OBJECT_NAME_INVALID, "NtOpenKey failed: 0x%08lx\n", status );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "\\Registry" );
    status = pNtOpenKey(&key, KEY_READ, &attr);
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );
    pNtClose( key );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "\\Registry\\" );
    status = pNtOpenKey(&key, KEY_READ, &attr);
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );
    pNtClose( key );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "\\Registry\\\\" );
    status = pNtOpenKey(&key, KEY_READ, &attr);
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );
    pNtClose( key );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "\\Foobar" );
    status = pNtOpenKey(&key, KEY_READ, &attr);
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "NtOpenKey failed: 0x%08lx\n", status );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "\\Foobar\\Machine" );
    status = pNtOpenKey(&key, KEY_READ, &attr);
    ok( status == STATUS_OBJECT_PATH_NOT_FOUND, "NtOpenKey failed: 0x%08lx\n", status );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "\\Machine\\Software\\Classes" );
    status = pNtOpenKey(&key, KEY_READ, &attr);
    ok( status == STATUS_OBJECT_PATH_NOT_FOUND, "NtOpenKey failed: 0x%08lx\n", status );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "Machine\\Software\\Classes" );
    status = pNtOpenKey(&key, KEY_READ, &attr);
    ok( status == STATUS_OBJECT_PATH_SYNTAX_BAD, "NtOpenKey failed: 0x%08lx\n", status );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "\\Device\\Null" );
    status = pNtOpenKey(&key, KEY_READ, &attr);
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "NtOpenKey failed: 0x%08lx\n", status );
    pRtlFreeUnicodeString( &str );

    InitializeObjectAttributes(&attr, &winetestpath, 0, 0, 0);
    status = pNtOpenKey(&key, KEY_WRITE|KEY_READ, &attr);
    ok(status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status);

    /* keys are case insensitive even without OBJ_CASE_INSENSITIVE */
    InitializeObjectAttributes( &attr, &str, 0, key, 0 );
    pRtlInitUnicodeString( &str, L"\xf6\xf3\x14d\x371\xd801\xdc00" );
    status = pNtCreateKey( &subkey, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0);
    ok(status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status);
    pNtClose( subkey );
#ifdef __REACTOS__
    if (pNtOpenKeyEx == NULL)
    {
        pNtClose(key);
        win_skip("NtOpenKeyEx not available\n");
        return;
    }
#endif
    pRtlInitUnicodeString( &str, L"\xd6\xd3\x14c\x370\xd801\xdc28" );  /* surrogates not supported */
    status = pNtOpenKeyEx(&subkey, KEY_ALL_ACCESS, &attr, 0);
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND, "NtOpenKeyEx failed: 0x%08lx\n", status);
    pRtlInitUnicodeString( &str, L"\xd6\xd3\x14c\x370\xd801\xdc00" );
    status = pNtOpenKeyEx(&subkey, KEY_ALL_ACCESS, &attr, 0);
    ok(status == STATUS_SUCCESS, "NtOpenKeyEx failed: 0x%08lx\n", status);

    status = pNtDeleteKey( subkey );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( subkey );
    pNtClose( key );

    if (!pNtOpenKeyEx)
    {
        win_skip("NtOpenKeyEx not available\n");
        return;
    }

    InitializeObjectAttributes(&attr, &winetestpath, 0, 0, 0);
    status = pNtOpenKeyEx(&key, KEY_WRITE|KEY_READ, &attr, 0);
    ok(status == STATUS_SUCCESS, "NtOpenKeyEx Failed: 0x%08lx\n", status);

    pNtClose(key);
}

static void test_NtCreateKey(void)
{
    /*Create WineTest*/
    OBJECT_ATTRIBUTES attr;
    HANDLE key, subkey, subkey2;
    ACCESS_MASK am = GENERIC_ALL;
    NTSTATUS status;
    UNICODE_STRING str;

    /* All NULL */
    status = pNtCreateKey(NULL, 0, NULL, 0, 0, 0, 0);
    ok(status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_PARAMETER,
       "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_PARAMETER, got: 0x%08lx\n", status);

    /* Only the key */
    status = pNtCreateKey(&key, 0, NULL, 0, 0, 0, 0);
    ok(status == STATUS_ACCESS_VIOLATION, "Expected STATUS_ACCESS_VIOLATION, got: 0x%08lx\n", status);

    /* Only accessmask */
    status = pNtCreateKey(NULL, am, NULL, 0, 0, 0, 0);
    ok(status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_PARAMETER,
       "Expected STATUS_ACCESS_VIOLATION or STATUS_INVALID_PARAMETER, got: 0x%08lx\n", status);

    /* Key and accessmask */
    status = pNtCreateKey(&key, am, NULL, 0, 0, 0, 0);
    ok(status == STATUS_ACCESS_VIOLATION, "Expected STATUS_ACCESS_VIOLATION, got: 0x%08lx\n", status);

    InitializeObjectAttributes(&attr, &winetestpath, 0, 0, 0);

    /* Only attributes */
    status = pNtCreateKey(NULL, 0, &attr, 0, 0, 0, 0);
    ok(status == STATUS_ACCESS_VIOLATION || status == STATUS_ACCESS_DENIED /* Win7 */,
       "Expected STATUS_ACCESS_VIOLATION or STATUS_ACCESS_DENIED, got: 0x%08lx\n", status);

    /* Length > sizeof(OBJECT_ATTRIBUTES) */
    attr.Length *= 2;
    status = pNtCreateKey(&key, am, &attr, 0, 0, 0, 0);
    ok(status == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got: 0x%08lx\n", status);

    attr.Length = sizeof(attr);
    status = pNtCreateKey(&key, am, &attr, 0, 0, 0, 0);
    ok(status == STATUS_SUCCESS, "NtCreateKey Failed: 0x%08lx\n", status);

    attr.RootDirectory = key;
    attr.ObjectName = &str;

    pRtlCreateUnicodeStringFromAsciiz( &str, "test\\sub\\key" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "NtCreateKey failed: 0x%08lx\n", status );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "test\\subkey" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "NtCreateKey failed: 0x%08lx\n", status );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "test\\\\subkey" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "NtCreateKey failed: 0x%08lx\n", status );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "test\\subkey\\" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "NtCreateKey failed: 0x%08lx\n", status );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "test_subkey\\" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );

    pRtlCreateUnicodeStringFromAsciiz( &str, "test_subkey\\" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );
    status = pNtDeleteKey( subkey );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( subkey );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "test_subkey2\\\\" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );
    pRtlCreateUnicodeStringFromAsciiz( &str, "test_subkey2\\\\test\\\\" );
    status = pNtCreateKey( &subkey2, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );
    pRtlFreeUnicodeString( &str );
    status = pNtDeleteKey( subkey2 );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( subkey2 );
    status = pNtDeleteKey( subkey );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( subkey );

    pRtlCreateUnicodeStringFromAsciiz( &str, "test_subkey" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );
    pRtlFreeUnicodeString( &str );
    status = pNtDeleteKey( subkey );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( subkey );

    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;

    pRtlCreateUnicodeStringFromAsciiz( &str, "" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_OBJECT_PATH_SYNTAX_BAD, "NtCreateKey failed: 0x%08lx\n", status );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "\\" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "NtCreateKey failed: 0x%08lx\n", status );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "\\Registry" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS || status == STATUS_ACCESS_DENIED,
        "NtCreateKey failed: 0x%08lx\n", status );
    if (!status) pNtClose( subkey );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "\\Registry\\" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS || status == STATUS_ACCESS_DENIED,
        "NtCreateKey failed: 0x%08lx\n", status );
    if (!status) pNtClose( subkey );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "\\Registry\\\\" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS || status == STATUS_ACCESS_DENIED,
        "NtCreateKey failed: 0x%08lx\n", status );
    if (!status) pNtClose( subkey );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "\\Foobar" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "NtCreateKey failed: 0x%08lx\n", status );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "\\Foobar\\Machine" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_OBJECT_PATH_NOT_FOUND, "NtCreateKey failed: 0x%08lx\n", status );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "\\Machine\\Software\\Classes" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_OBJECT_PATH_NOT_FOUND, "NtCreateKey failed: 0x%08lx\n", status );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "Machine\\Software\\Classes" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_OBJECT_PATH_SYNTAX_BAD, "NtCreateKey failed: 0x%08lx\n", status );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "\\Device\\Null" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "NtCreateKey failed: 0x%08lx\n", status );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "\\Registry\\Machine\\Software\\Classes" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS || status == STATUS_ACCESS_DENIED,
        "NtCreateKey failed: 0x%08lx\n", status );
    if (!status) pNtClose( subkey );
    pRtlFreeUnicodeString( &str );

    /* the REGISTRY part is case-sensitive unless OBJ_CASE_INSENSITIVE is specified */
    am = GENERIC_READ;
    attr.Attributes = 0;
    pRtlCreateUnicodeStringFromAsciiz( &str, "\\Registry\\Machine\\Software\\Classes" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok(status == STATUS_OBJECT_PATH_NOT_FOUND || status == STATUS_SUCCESS /* Win10 1607+ */,
            "NtCreateKey failed: 0x%08lx\n", status );
    if (!status) pNtClose( subkey );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "\\REGISTRY\\Machine\\Software\\Classes" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS,
        "NtCreateKey failed: 0x%08lx\n", status );
    if (!status) pNtClose( subkey );
    pRtlFreeUnicodeString( &str );

    pRtlCreateUnicodeStringFromAsciiz( &str, "\\REGISTRY\\MACHINE\\SOFTWARE\\CLASSES" );
    status = pNtCreateKey( &subkey, am, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS,
        "NtCreateKey failed: 0x%08lx\n", status );
    if (!status) pNtClose( subkey );
    pRtlFreeUnicodeString( &str );

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
    ok(status == STATUS_SUCCESS, "NtOpenKey Failed: 0x%08lx\n", status);

    pRtlCreateUnicodeStringFromAsciiz(&ValName, "deletetest");
    status = pNtSetValueKey(key, &ValName, 0, REG_DWORD, &data, sizeof(data));
    ok(status == STATUS_SUCCESS, "NtSetValueKey Failed: 0x%08lx\n", status);
    pRtlFreeUnicodeString(&ValName);

    pRtlCreateUnicodeStringFromAsciiz(&ValName, "stringtest");
    status = pNtSetValueKey(key, &ValName, 0, REG_SZ, (VOID*)stringW, STR_TRUNC_SIZE);
    ok(status == STATUS_SUCCESS, "NtSetValueKey Failed: 0x%08lx\n", status);
    pRtlFreeUnicodeString(&ValName);

    pNtClose(key);
}

static void test_RtlOpenCurrentUser(void)
{
    NTSTATUS status;
    HANDLE handle;
    status=pRtlOpenCurrentUser(KEY_READ, &handle);
    ok(status == STATUS_SUCCESS, "RtlOpenCurrentUser Failed: 0x%08lx\n", status);
    pNtClose(handle);
}

static void test_RtlCheckRegistryKey(void)
{
    static WCHAR empty[] = {0};
    NTSTATUS status;

    status = pRtlCheckRegistryKey(RTL_REGISTRY_ABSOLUTE, winetestpath.Buffer);
    ok(status == STATUS_SUCCESS, "RtlCheckRegistryKey with RTL_REGISTRY_ABSOLUTE: 0x%08lx\n", status);

    status = pRtlCheckRegistryKey((RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL), winetestpath.Buffer);
    ok(status == STATUS_SUCCESS, "RtlCheckRegistryKey with RTL_REGISTRY_ABSOLUTE and RTL_REGISTRY_OPTIONAL: 0x%08lx\n", status);

    status = pRtlCheckRegistryKey(RTL_REGISTRY_ABSOLUTE, NULL);
    ok(status == STATUS_OBJECT_PATH_SYNTAX_BAD, "RtlCheckRegistryKey with RTL_REGISTRY_ABSOLUTE and Path being NULL: 0x%08lx\n", status);

    status = pRtlCheckRegistryKey(RTL_REGISTRY_ABSOLUTE, empty);
    ok(status == STATUS_OBJECT_PATH_SYNTAX_BAD, "RtlCheckRegistryKey with RTL_REGISTRY_ABSOLUTE and Path being empty: 0x%08lx\n", status);

    status = pRtlCheckRegistryKey(RTL_REGISTRY_USER, NULL);
    ok(status == STATUS_SUCCESS, "RtlCheckRegistryKey with RTL_REGISTRY_USER and Path being NULL: 0x%08lx\n", status);

    status = pRtlCheckRegistryKey(RTL_REGISTRY_USER, empty);
    ok(status == STATUS_SUCCESS, "RtlCheckRegistryKey with RTL_REGISTRY_USER and Path being empty: 0x%08lx\n", status);
}

static void test_NtFlushKey(void)
{
    NTSTATUS status;
    HANDLE hkey;
    OBJECT_ATTRIBUTES attr;
    ACCESS_MASK am = KEY_ALL_ACCESS;

    status = pNtFlushKey(NULL);
    ok(status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got: 0x%08lx\n", status);

    InitializeObjectAttributes(&attr, &winetestpath, 0, 0, 0);
    pNtOpenKey(&hkey, am, &attr);

    status = pNtFlushKey(hkey);
    ok(status == STATUS_SUCCESS, "NtDeleteKey Failed: 0x%08lx\n", status);

    pNtClose(hkey);
}

static void test_NtQueryValueKey(void)
{
    HANDLE key;
    NTSTATUS status;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING ValName;
    KEY_VALUE_BASIC_INFORMATION *basic_info;
    KEY_VALUE_PARTIAL_INFORMATION *partial_info, pi;
    KEY_VALUE_PARTIAL_INFORMATION_ALIGN64 *aligned_info;
    KEY_VALUE_FULL_INFORMATION *full_info;
    DWORD len, expected;

    pRtlCreateUnicodeStringFromAsciiz(&ValName, "deletetest");

    InitializeObjectAttributes(&attr, &winetestpath, 0, 0, 0);
    status = pNtOpenKey(&key, KEY_READ|KEY_SET_VALUE, &attr);
    ok(status == STATUS_SUCCESS, "NtOpenKey Failed: 0x%08lx\n", status);

    len = FIELD_OFFSET(KEY_VALUE_BASIC_INFORMATION, Name[0]);
    basic_info = HeapAlloc(GetProcessHeap(), 0, sizeof(*basic_info));
    status = pNtQueryValueKey(key, &ValName, KeyValueBasicInformation, basic_info, len, &len);
    ok(status == STATUS_BUFFER_OVERFLOW, "NtQueryValueKey should have returned STATUS_BUFFER_OVERFLOW instead of 0x%08lx\n", status);
    ok(basic_info->TitleIndex == 0, "NtQueryValueKey returned wrong TitleIndex %ld\n", basic_info->TitleIndex);
    ok(basic_info->Type == REG_DWORD, "NtQueryValueKey returned wrong Type %ld\n", basic_info->Type);
    ok(basic_info->NameLength == 20, "NtQueryValueKey returned wrong NameLength %ld\n", basic_info->NameLength);
    ok(len == FIELD_OFFSET(KEY_VALUE_BASIC_INFORMATION, Name[basic_info->NameLength/sizeof(WCHAR)]), "NtQueryValueKey returned wrong len %ld\n", len);

    basic_info = HeapReAlloc(GetProcessHeap(), 0, basic_info, len);
    status = pNtQueryValueKey(key, &ValName, KeyValueBasicInformation, basic_info, len, &len);
    ok(status == STATUS_SUCCESS, "NtQueryValueKey should have returned STATUS_SUCCESS instead of 0x%08lx\n", status);
    ok(basic_info->TitleIndex == 0, "NtQueryValueKey returned wrong TitleIndex %ld\n", basic_info->TitleIndex);
    ok(basic_info->Type == REG_DWORD, "NtQueryValueKey returned wrong Type %ld\n", basic_info->Type);
    ok(basic_info->NameLength == 20, "NtQueryValueKey returned wrong NameLength %ld\n", basic_info->NameLength);
    ok(len == FIELD_OFFSET(KEY_VALUE_BASIC_INFORMATION, Name[basic_info->NameLength/sizeof(WCHAR)]), "NtQueryValueKey returned wrong len %ld\n", len);
    ok(!memcmp(basic_info->Name, ValName.Buffer, ValName.Length), "incorrect Name returned\n");
    HeapFree(GetProcessHeap(), 0, basic_info);

    len = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[0]);
    partial_info = HeapAlloc(GetProcessHeap(), 0, sizeof(*partial_info));
    status = pNtQueryValueKey(key, &ValName, KeyValuePartialInformation, partial_info, len, &len);
    ok(status == STATUS_BUFFER_OVERFLOW, "NtQueryValueKey should have returned STATUS_BUFFER_OVERFLOW instead of 0x%08lx\n", status);
    ok(partial_info->TitleIndex == 0, "NtQueryValueKey returned wrong TitleIndex %ld\n", partial_info->TitleIndex);
    ok(partial_info->Type == REG_DWORD, "NtQueryValueKey returned wrong Type %ld\n", partial_info->Type);
    ok(partial_info->DataLength == 4, "NtQueryValueKey returned wrong DataLength %ld\n", partial_info->DataLength);
    ok(len == FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[partial_info->DataLength]), "NtQueryValueKey returned wrong len %ld\n", len);

    partial_info = HeapReAlloc(GetProcessHeap(), 0, partial_info, len);
    status = pNtQueryValueKey(key, &ValName, KeyValuePartialInformation, partial_info, len, &len);
    ok(status == STATUS_SUCCESS, "NtQueryValueKey should have returned STATUS_SUCCESS instead of 0x%08lx\n", status);
    ok(partial_info->TitleIndex == 0, "NtQueryValueKey returned wrong TitleIndex %ld\n", partial_info->TitleIndex);
    ok(partial_info->Type == REG_DWORD, "NtQueryValueKey returned wrong Type %ld\n", partial_info->Type);
    ok(partial_info->DataLength == 4, "NtQueryValueKey returned wrong DataLength %ld\n", partial_info->DataLength);
    ok(len == FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[partial_info->DataLength]), "NtQueryValueKey returned wrong len %ld\n", len);
    ok(*(DWORD *)partial_info->Data == 711, "incorrect Data returned: 0x%lx\n", *(DWORD *)partial_info->Data);
    HeapFree(GetProcessHeap(), 0, partial_info);

    len = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION_ALIGN64, Data[0]);
    aligned_info = HeapAlloc(GetProcessHeap(), 0, sizeof(*aligned_info) + 4);

    aligned_info = (KEY_VALUE_PARTIAL_INFORMATION_ALIGN64 *)((char *)aligned_info + 4);
    status = pNtQueryValueKey(key, &ValName, KeyValuePartialInformationAlign64, aligned_info, len, &len);
    ok(status == STATUS_BUFFER_OVERFLOW, "NtQueryValueKey should have returned STATUS_BUFFER_OVERFLOW instead of 0x%08lx\n", status);
    ok(aligned_info->Type == REG_DWORD, "NtQueryValueKey returned wrong Type %ld\n", aligned_info->Type);
    ok(aligned_info->DataLength == 4, "NtQueryValueKey returned wrong DataLength %ld\n", aligned_info->DataLength);
    ok(len == FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION_ALIGN64, Data[aligned_info->DataLength]), "NtQueryValueKey returned wrong len %ld\n", len);

    len = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION_ALIGN64, Data[0]);
    aligned_info = (KEY_VALUE_PARTIAL_INFORMATION_ALIGN64 *)((char *)aligned_info - 4);
    status = pNtQueryValueKey(key, &ValName, KeyValuePartialInformationAlign64, aligned_info, len, &len);
    ok(status == STATUS_BUFFER_OVERFLOW, "NtQueryValueKey should have returned STATUS_BUFFER_OVERFLOW instead of 0x%08lx\n", status);
    ok(aligned_info->Type == REG_DWORD, "NtQueryValueKey returned wrong Type %ld\n", aligned_info->Type);
    ok(aligned_info->DataLength == 4, "NtQueryValueKey returned wrong DataLength %ld\n", aligned_info->DataLength);
    ok(len == FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION_ALIGN64, Data[aligned_info->DataLength]), "NtQueryValueKey returned wrong len %ld\n", len);

    aligned_info = HeapReAlloc(GetProcessHeap(), 0, aligned_info, len + 4);
    aligned_info = (KEY_VALUE_PARTIAL_INFORMATION_ALIGN64 *)((char *)aligned_info + 4);
    status = pNtQueryValueKey(key, &ValName, KeyValuePartialInformationAlign64, aligned_info, len, &len);
    ok(status == STATUS_SUCCESS, "NtQueryValueKey should have returned STATUS_SUCCESS instead of 0x%08lx\n", status);
    ok(aligned_info->Type == REG_DWORD, "NtQueryValueKey returned wrong Type %ld\n", aligned_info->Type);
    ok(aligned_info->DataLength == 4, "NtQueryValueKey returned wrong DataLength %ld\n", aligned_info->DataLength);
    ok(len == FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION_ALIGN64, Data[aligned_info->DataLength]), "NtQueryValueKey returned wrong len %ld\n", len);
    ok(*(DWORD *)aligned_info->Data == 711, "incorrect Data returned: 0x%lx\n", *(DWORD *)aligned_info->Data);

    aligned_info = (KEY_VALUE_PARTIAL_INFORMATION_ALIGN64 *)((char *)aligned_info - 4);
    status = pNtQueryValueKey(key, &ValName, KeyValuePartialInformationAlign64, aligned_info, len, &len);
    ok(status == STATUS_SUCCESS, "NtQueryValueKey should have returned STATUS_SUCCESS instead of 0x%08lx\n", status);
    ok(aligned_info->Type == REG_DWORD, "NtQueryValueKey returned wrong Type %ld\n", aligned_info->Type);
    ok(aligned_info->DataLength == 4, "NtQueryValueKey returned wrong DataLength %ld\n", aligned_info->DataLength);
    ok(len == FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION_ALIGN64, Data[aligned_info->DataLength]), "NtQueryValueKey returned wrong len %ld\n", len);
    ok(*(DWORD *)aligned_info->Data == 711, "incorrect Data returned: 0x%lx\n", *(DWORD *)aligned_info->Data);
    HeapFree(GetProcessHeap(), 0, aligned_info);

    len = FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name[0]);
    full_info = HeapAlloc(GetProcessHeap(), 0, sizeof(*full_info));
    status = pNtQueryValueKey(key, &ValName, KeyValueFullInformation, full_info, len, &len);
    ok(status == STATUS_BUFFER_OVERFLOW, "NtQueryValueKey should have returned STATUS_BUFFER_OVERFLOW instead of 0x%08lx\n", status);
    ok(full_info->TitleIndex == 0, "NtQueryValueKey returned wrong TitleIndex %ld\n", full_info->TitleIndex);
    ok(full_info->Type == REG_DWORD, "NtQueryValueKey returned wrong Type %ld\n", full_info->Type);
    ok(full_info->DataLength == 4, "NtQueryValueKey returned wrong DataLength %ld\n", full_info->DataLength);
    ok(full_info->NameLength == 20, "NtQueryValueKey returned wrong NameLength %ld\n", full_info->NameLength);
    ok(len == FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name[0]) + full_info->DataLength + full_info->NameLength,
        "NtQueryValueKey returned wrong len %ld\n", len);
    len = FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name[0]) + full_info->DataLength + full_info->NameLength;

    full_info = HeapReAlloc(GetProcessHeap(), 0, full_info, len);
    status = pNtQueryValueKey(key, &ValName, KeyValueFullInformation, full_info, len, &len);
    ok(status == STATUS_SUCCESS, "NtQueryValueKey should have returned STATUS_SUCCESS instead of 0x%08lx\n", status);
    ok(full_info->TitleIndex == 0, "NtQueryValueKey returned wrong TitleIndex %ld\n", full_info->TitleIndex);
    ok(full_info->Type == REG_DWORD, "NtQueryValueKey returned wrong Type %ld\n", full_info->Type);
    ok(full_info->DataLength == 4, "NtQueryValueKey returned wrong DataLength %ld\n", full_info->DataLength);
    ok(full_info->NameLength == 20, "NtQueryValueKey returned wrong NameLength %ld\n", full_info->NameLength);
    ok(!memcmp(full_info->Name, ValName.Buffer, ValName.Length), "incorrect Name returned\n");
    ok(*(DWORD *)((char *)full_info + full_info->DataOffset) == 711, "incorrect Data returned: 0x%lx\n",
        *(DWORD *)((char *)full_info + full_info->DataOffset));
    HeapFree(GetProcessHeap(), 0, full_info);

    pRtlFreeUnicodeString(&ValName);
    pRtlCreateUnicodeStringFromAsciiz(&ValName, "stringtest");

    status = pNtQueryValueKey(key, &ValName, KeyValuePartialInformation, NULL, 0, &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "NtQueryValueKey should have returned STATUS_BUFFER_TOO_SMALL instead of 0x%08lx\n", status);
    partial_info = HeapAlloc(GetProcessHeap(), 0, len+1);
    memset(partial_info, 0xbd, len+1);
    status = pNtQueryValueKey(key, &ValName, KeyValuePartialInformation, partial_info, len, &len);
    ok(status == STATUS_SUCCESS, "NtQueryValueKey should have returned STATUS_SUCCESS instead of 0x%08lx\n", status);
    ok(partial_info->TitleIndex == 0, "NtQueryValueKey returned wrong TitleIndex %ld\n", partial_info->TitleIndex);
    ok(partial_info->Type == REG_SZ, "NtQueryValueKey returned wrong Type %ld\n", partial_info->Type);
    ok(partial_info->DataLength == STR_TRUNC_SIZE, "NtQueryValueKey returned wrong DataLength %ld\n", partial_info->DataLength);
    ok(!memcmp(partial_info->Data, stringW, STR_TRUNC_SIZE), "incorrect Data returned\n");
    ok(*(partial_info->Data+STR_TRUNC_SIZE) == 0xbd, "string overflowed %02x\n", *(partial_info->Data+STR_TRUNC_SIZE));

    expected = len;
    status = pNtQueryValueKey(key, &ValName, KeyValuePartialInformation, partial_info, 0, &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "NtQueryValueKey wrong status 0x%08lx\n", status);
    ok(len == expected, "NtQueryValueKey wrong len %lu\n", len);
    status = pNtQueryValueKey(key, &ValName, KeyValuePartialInformation, partial_info, 1, &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "NtQueryValueKey wrong status 0x%08lx\n", status);
    ok(len == expected, "NtQueryValueKey wrong len %lu\n", len);
    status = pNtQueryValueKey(key, &ValName, KeyValuePartialInformation, partial_info, FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) - 1, &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "NtQueryValueKey wrong status 0x%08lx\n", status);
    ok(len == expected, "NtQueryValueKey wrong len %lu\n", len);
    status = pNtQueryValueKey(key, &ValName, KeyValuePartialInformation, partial_info, FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data), &len);
    ok(status == STATUS_BUFFER_OVERFLOW, "NtQueryValueKey wrong status 0x%08lx\n", status);
    ok(len == expected, "NtQueryValueKey wrong len %lu\n", len);

    HeapFree(GetProcessHeap(), 0, partial_info);
    pRtlFreeUnicodeString(&ValName);

    pRtlCreateUnicodeStringFromAsciiz(&ValName, "custtest");
    status = pNtSetValueKey(key, &ValName, 0, 0xff00ff00, NULL, 0);
    ok(status == STATUS_SUCCESS, "NtSetValueKey Failed: 0x%08lx\n", status);

    status = pNtQueryValueKey(key, &ValName, KeyValuePartialInformation, &pi, sizeof(pi), &len);
    ok(status == STATUS_SUCCESS, "NtQueryValueKey should have returned STATUS_SUCCESS instead of 0x%08lx\n", status);
    ok(pi.Type == 0xff00ff00, "Type=%lx\n", pi.Type);
    ok(pi.DataLength == 0, "DataLength=%lu\n", pi.DataLength);
    pRtlFreeUnicodeString(&ValName);

    pNtClose(key);
}

static void test_NtDeleteKey(void)
{
    UNICODE_STRING string;
    char buffer[200];
    NTSTATUS status;
    HANDLE hkey, hkey2;
    OBJECT_ATTRIBUTES attr;
    DWORD size;

    status = pNtDeleteKey(NULL);
    ok(status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got: 0x%08lx\n", status);

    InitializeObjectAttributes(&attr, &winetestpath, 0, 0, 0);
    status = pNtOpenKey(&hkey, KEY_ALL_ACCESS, &attr);
    ok(status == STATUS_SUCCESS, "NtOpenKey Failed: 0x%08lx\n", status);

    status = pNtDeleteKey(hkey);
#ifdef __REACTOS__
    if (GetNTVersion() < _WIN32_WINNT_WIN7)
    {
        /* On older Windows versions the key cannot be deleted, when it still has values in it */
        ok(status == STATUS_CANNOT_DELETE, "NtDeleteKey unexpected status: 0x%08lx\n", status);
        PKEY_VALUE_BASIC_INFORMATION info = (PKEY_VALUE_BASIC_INFORMATION)buffer;
        while ((status = NtEnumerateKey(hkey, 0, KeyValueBasicInformation, info, sizeof(buffer), &size)) >= 0)
        {
            string = (UNICODE_STRING){ info->NameLength, info->NameLength, info->Name };
            printf("got subkey %wZ\n", &string);
        }
        while ((status = NtEnumerateValueKey(hkey, 0, KeyValueBasicInformation, info, sizeof(buffer), &size)) >= 0)
        {
            string = (UNICODE_STRING){ info->NameLength, info->NameLength, info->Name };
            printf("Deleting value %wZ\n", &string);
            status = NtDeleteValueKey(hkey, &string);
            ok(status == STATUS_SUCCESS, "NtDeleteValueKey failed: 0x%08lx\n", status);
        }
        ok(status == STATUS_NO_MORE_ENTRIES, "NtEnumerateValueKey unexpected status: 0x%08lx\n", status);
        status = pNtDeleteKey(hkey);
    }
#endif
    ok(status == STATUS_SUCCESS, "NtDeleteKey Failed: 0x%08lx\n", status);

    status = pNtQueryKey(hkey, KeyNameInformation, buffer, sizeof(buffer), &size);
    ok(status == STATUS_KEY_DELETED, "got %#lx\n", status);

    status = pNtEnumerateKey(hkey, 0, KeyFullInformation, buffer, sizeof(buffer), &size);
    ok(status == STATUS_KEY_DELETED, "got %#lx\n", status);

    pRtlInitUnicodeString(&string, L"value");
    status = pNtQueryValueKey(hkey, &string, KeyValueBasicInformation, buffer, sizeof(buffer), &size);
    ok(status == STATUS_KEY_DELETED, "got %#lx\n", status);

    status = pNtEnumerateValueKey(hkey, 0, KeyValuePartialInformation, buffer, sizeof(buffer), &size);
    ok(status == STATUS_KEY_DELETED, "got %#lx\n", status);

    status = pNtSetValueKey(hkey, &string, 0, REG_SZ, "test", 5);
    ok(status == STATUS_KEY_DELETED, "got %#lx\n", status);

    status = pNtDeleteValueKey(hkey, &string);
    ok(status == STATUS_KEY_DELETED, "got %#lx\n", status);

    status = pNtDeleteKey(hkey);
    ok(!status, "got %#lx\n", status);

    RtlInitUnicodeString(&string, L"subkey");
    InitializeObjectAttributes(&attr, &string, OBJ_CASE_INSENSITIVE, hkey, NULL);
    status = pNtOpenKey(&hkey2, KEY_READ, &attr);
    ok(status == STATUS_KEY_DELETED, "got %#lx\n", status);

    status = pNtCreateKey(&hkey2, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL);
    ok(status == STATUS_KEY_DELETED, "got %#lx\n", status);

    status = pNtQueryObject(hkey, ObjectNameInformation, buffer, sizeof(buffer), &size);
    ok(status == STATUS_KEY_DELETED, "got %#lx\n", status);

    status = pNtQueryObject(hkey, ObjectBasicInformation, buffer, sizeof(OBJECT_BASIC_INFORMATION), &size);
    ok(!status, "got %#lx\n", status);

    status = pNtClose(hkey);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
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
    ok(status == STATUS_INVALID_PARAMETER, "NtQueryLicenseValue returned %08lx, expected STATUS_INVALID_PARAMETER\n", status);
    ok(type == 0xdead, "expected unmodified value for type, got %lu\n", type);
    ok(len == 0xbeef, "expected unmodified value for len, got %lu\n", len);

    /* test with empty key */
    pRtlCreateUnicodeStringFromAsciiz(&name, "");

    type = 0xdead;
    len = 0xbeef;
    status = pNtQueryLicenseValue(NULL, &type, buffer, sizeof(buffer), &len);
    ok(status == STATUS_INVALID_PARAMETER, "NtQueryLicenseValue returned %08lx, expected STATUS_INVALID_PARAMETER\n", status);
    ok(type == 0xdead, "expected unmodified value for type, got %lu\n", type);
    ok(len == 0xbeef, "expected unmodified value for len, got %lu\n", len);

    type = 0xdead;
    status = pNtQueryLicenseValue(&name, &type, buffer, sizeof(buffer), NULL);
    ok(status == STATUS_INVALID_PARAMETER, "NtQueryLicenseValue returned %08lx, expected STATUS_INVALID_PARAMETER\n", status);
    ok(type == 0xdead, "expected unmodified value for type, got %lu\n", type);

    len = 0xbeef;
    status = pNtQueryLicenseValue(&name, NULL, buffer, sizeof(buffer), &len);
    ok(status == STATUS_INVALID_PARAMETER, "NtQueryLicenseValue returned %08lx, expected STATUS_INVALID_PARAMETER\n", status);
    ok(len == 0xbeef, "expected unmodified value for len, got %lu\n", len);

    type = 0xdead;
    len = 0xbeef;
    status = pNtQueryLicenseValue(&name, &type, buffer, sizeof(buffer), &len);
    ok(status == STATUS_INVALID_PARAMETER, "NtQueryLicenseValue returned %08lx, expected STATUS_INVALID_PARAMETER\n", status);
    ok(type == 0xdead, "expected unmodified value for type, got %lu\n", type);
    ok(len == 0xbeef, "expected unmodified value for len, got %lu\n", len);

    pRtlFreeUnicodeString(&name);

    /* test with nonexistent licence key */
    pRtlCreateUnicodeStringFromAsciiz(&name, "Nonexistent-License-Value");

    type = 0xdead;
    len = 0xbeef;
    status = pNtQueryLicenseValue(NULL, &type, buffer, sizeof(buffer), &len);
    ok(status == STATUS_INVALID_PARAMETER, "NtQueryLicenseValue returned %08lx, expected STATUS_INVALID_PARAMETER\n", status);
    ok(type == 0xdead, "expected unmodified value for type, got %lu\n", type);
    ok(len == 0xbeef, "expected unmodified value for len, got %lu\n", len);

    type = 0xdead;
    status = pNtQueryLicenseValue(&name, &type, buffer, sizeof(buffer), NULL);
    ok(status == STATUS_INVALID_PARAMETER, "NtQueryLicenseValue returned %08lx, expected STATUS_INVALID_PARAMETER\n", status);
    ok(type == 0xdead, "expected unmodified value for type, got %lu\n", type);

    len = 0xbeef;
    status = pNtQueryLicenseValue(&name, NULL, buffer, sizeof(buffer), &len);
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND, "NtQueryLicenseValue returned %08lx, expected STATUS_OBJECT_NAME_NOT_FOUND\n", status);
    ok(len == 0xbeef || broken(!len) /* Win10 1607 */, "expected unmodified value for len, got %lu\n", len);

    type = 0xdead;
    len = 0xbeef;
    status = pNtQueryLicenseValue(&name, &type, buffer, sizeof(buffer), &len);
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND, "NtQueryLicenseValue unexpected succeeded\n");
    ok(type == 0xdead, "expected unmodified value for type, got %lu\n", type);
    ok(len == 0xbeef || broken(!len) /* Win10 1607 */, "expected unmodified value for len, got %lu\n", len);

    pRtlFreeUnicodeString(&name);

    /* test with REG_SZ license key */
    pRtlCreateUnicodeStringFromAsciiz(&name, "Kernel-MUI-Language-Allowed");

    type = 0xdead;
    len = 0xbeef;
    status = pNtQueryLicenseValue(NULL, &type, buffer, sizeof(buffer), &len);
    ok(status == STATUS_INVALID_PARAMETER, "NtQueryLicenseValue returned %08lx, expected STATUS_INVALID_PARAMETER\n", status);
    ok(type == 0xdead, "expected unmodified value for type, got %lu\n", type);
    ok(len == 0xbeef, "expected unmodified value for len, got %lu\n", len);

    type = 0xdead;
    status = pNtQueryLicenseValue(&name, &type, buffer, sizeof(buffer), NULL);
    ok(status == STATUS_INVALID_PARAMETER, "NtQueryLicenseValue returned %08lx, expected STATUS_INVALID_PARAMETER\n", status);
    ok(type == 0xdead, "expected unmodified value for type, got %lu\n", type);

    type = 0xdead;
    len = 0;
    status = pNtQueryLicenseValue(&name, &type, buffer, 0, &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "NtQueryLicenseValue returned %08lx, expected STATUS_BUFFER_TOO_SMALL\n", status);
    ok(type == REG_SZ, "expected type = REG_SZ, got %lu\n", type);
    ok(len == sizeof(emptyW), "expected len = %lu, got %lu\n", (DWORD)sizeof(emptyW), len);

    len = 0;
    status = pNtQueryLicenseValue(&name, NULL, buffer, 0, &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "NtQueryLicenseValue returned %08lx, expected STATUS_BUFFER_TOO_SMALL\n", status);
    ok(len == sizeof(emptyW), "expected len = %lu, got %lu\n", (DWORD)sizeof(emptyW), len);

    type = 0xdead;
    len = 0;
    memset(buffer, 0x11, sizeof(buffer));
    status = pNtQueryLicenseValue(&name, &type, buffer, sizeof(buffer), &len);
    ok(status == STATUS_SUCCESS, "NtQueryLicenseValue returned %08lx, expected STATUS_SUCCESS\n", status);
    ok(type == REG_SZ, "expected type = REG_SZ, got %lu\n", type);
    ok(len == sizeof(emptyW), "expected len = %lu, got %lu\n", (DWORD)sizeof(emptyW), len);
    ok(!memcmp(buffer, emptyW, sizeof(emptyW)), "unexpected buffer content\n");

    type = 0xdead;
    len = 0;
    memset(buffer, 0x11, sizeof(buffer));
    status = pNtQueryLicenseValue(&name, &type, buffer, 2, &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "NtQueryLicenseValue returned %08lx, expected STATUS_BUFFER_TOO_SMALL\n", status);
    ok(type == REG_SZ, "expected type REG_SZ, got %lu\n", type);
    ok(len == sizeof(emptyW), "expected len = %lu, got %lu\n", (DWORD)sizeof(emptyW), len);
    ok(buffer[0] == 0x1111, "expected buffer[0] = 0x1111, got %u\n", buffer[0]);

    pRtlFreeUnicodeString(&name);

    /* test with REG_DWORD license key */
    pRtlCreateUnicodeStringFromAsciiz(&name, "Kernel-MUI-Number-Allowed");

    type = 0xdead;
    len = 0xbeef;
    status = pNtQueryLicenseValue(NULL, &type, &value, sizeof(value), &len);
    ok(status == STATUS_INVALID_PARAMETER, "NtQueryLicenseValue returned %08lx, expected STATUS_INVALID_PARAMETER\n", status);
    ok(type == 0xdead, "expected unmodified value for type, got %lu\n", type);
    ok(len == 0xbeef, "expected unmodified value for len, got %lu\n", len);

    type = 0xdead;
    status = pNtQueryLicenseValue(&name, &type, &value, sizeof(value), NULL);
    ok(status == STATUS_INVALID_PARAMETER, "NtQueryLicenseValue returned %08lx, expected STATUS_INVALID_PARAMETER\n", status);
    ok(type == 0xdead, "expected unmodified value for type, got %lu\n", type);

    type = 0xdead;
    len = 0;
    status = pNtQueryLicenseValue(&name, &type, &value, 0, &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "NtQueryLicenseValue returned %08lx, expected STATUS_BUFFER_TOO_SMALL\n", status);
    ok(type == REG_DWORD, "expected type = REG_DWORD, got %lu\n", type);
    ok(len == sizeof(value), "expected len = %lu, got %lu\n", (DWORD)sizeof(value), len);

    len = 0;
    status = pNtQueryLicenseValue(&name, NULL, &value, 0, &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "NtQueryLicenseValue returned %08lx, expected STATUS_BUFFER_TOO_SMALL\n", status);
    ok(len == sizeof(value), "expected len = %lu, got %lu\n", (DWORD)sizeof(value), len);

    type = 0xdead;
    len = 0;
    value = 0xdeadbeef;
    status = pNtQueryLicenseValue(&name, &type, &value, sizeof(value), &len);
    ok(status == STATUS_SUCCESS, "NtQueryLicenseValue returned %08lx, expected STATUS_SUCCESS\n", status);
    ok(type == REG_DWORD, "expected type = REG_DWORD, got %lu\n", type);
    ok(len == sizeof(value), "expected len = %lu, got %lu\n", (DWORD)sizeof(value), len);
    ok(value != 0xdeadbeef, "expected value != 0xdeadbeef\n");

    type = 0xdead;
    len = 0;
    status = pNtQueryLicenseValue(&name, &type, &value, 2, &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "NtQueryLicenseValue returned %08lx, expected STATUS_BUFFER_TOO_SMALL\n", status);
    ok(type == REG_DWORD, "expected type REG_DWORD, got %lu\n", type);
    ok(len == sizeof(value), "expected len = %lu, got %lu\n", (DWORD)sizeof(value), len);

    pRtlFreeUnicodeString(&name);
}

static void test_RtlpNtQueryValueKey(void)
{
    NTSTATUS status;

    status = pRtlpNtQueryValueKey(NULL, NULL, NULL, NULL, NULL);
    ok(status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got: 0x%08lx\n", status);
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
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );

    attr.RootDirectory = root;
    attr.ObjectName = &link_str;
    status = pNtCreateKey( &link, KEY_ALL_ACCESS, &attr, 0, 0, REG_OPTION_CREATE_LINK, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );

    /* REG_SZ is not allowed */
    status = pNtSetValueKey( link, &symlink_str, 0, REG_SZ, target, target_len );
    ok( status == STATUS_ACCESS_DENIED, "NtSetValueKey wrong status 0x%08lx\n", status );
    status = pNtSetValueKey( link, &symlink_str, 0, REG_LINK, target, target_len - sizeof(WCHAR) );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08lx\n", status );
    /* other values are not allowed */
    status = pNtSetValueKey( link, &link_str, 0, REG_LINK, target, target_len - sizeof(WCHAR) );
    ok( status == STATUS_ACCESS_DENIED, "NtSetValueKey wrong status 0x%08lx\n", status );

    /* try opening the target through the link */

    attr.ObjectName = &link_str;
    key = (HANDLE)0xdeadbeef;
    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "NtOpenKey wrong status 0x%08lx\n", status );
    ok( !key, "key = %p\n", key );

    attr.ObjectName = &target_str;
    status = pNtCreateKey( &key, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );

    dw = 0xbeef;
    status = pNtSetValueKey( key, &value_str, 0, REG_DWORD, &dw, sizeof(dw) );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08lx\n", status );
    pNtClose( key );

    attr.ObjectName = &link_str;
    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );

    len = sizeof(buffer);
    status = pNtQueryValueKey( key, &value_str, KeyValuePartialInformation, info, len, &len );
    ok( status == STATUS_SUCCESS, "NtQueryValueKey failed: 0x%08lx\n", status );
    ok( len == FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION,Data) + sizeof(DWORD), "wrong len %lu\n", len );

    status = pNtQueryValueKey( key, &symlink_str, KeyValuePartialInformation, info, len, &len );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "NtQueryValueKey failed: 0x%08lx\n", status );

    /* REG_LINK can be created in non-link keys */
    status = pNtSetValueKey( key, &symlink_str, 0, REG_LINK, target, target_len - sizeof(WCHAR) );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08lx\n", status );
    len = sizeof(buffer);
    status = pNtQueryValueKey( key, &symlink_str, KeyValuePartialInformation, info, len, &len );
    ok( status == STATUS_SUCCESS, "NtQueryValueKey failed: 0x%08lx\n", status );
    ok( len == FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION,Data) + target_len - sizeof(WCHAR),
        "wrong len %lu\n", len );
    status = pNtDeleteValueKey( key, &symlink_str );
    ok( status == STATUS_SUCCESS, "NtDeleteValueKey failed: 0x%08lx\n", status );

    pNtClose( key );

    attr.Attributes = 0;
    status = pNtCreateKey( &key, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );

    len = sizeof(buffer);
    status = pNtQueryValueKey( key, &value_str, KeyValuePartialInformation, info, len, &len );
    ok( status == STATUS_SUCCESS, "NtQueryValueKey failed: 0x%08lx\n", status );
    ok( len == FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION,Data) + sizeof(DWORD), "wrong len %lu\n", len );

    status = pNtQueryValueKey( key, &symlink_str, KeyValuePartialInformation, info, len, &len );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "NtQueryValueKey failed: 0x%08lx\n", status );
    pNtClose( key );

    /* now open the symlink itself */

    attr.RootDirectory = root;
    attr.Attributes = OBJ_OPENLINK;
    attr.ObjectName = &link_str;
    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );

    len = sizeof(buffer);
    status = pNtQueryValueKey( key, &symlink_str, KeyValuePartialInformation, info, len, &len );
    ok( status == STATUS_SUCCESS, "NtQueryValueKey failed: 0x%08lx\n", status );
    ok( len == FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION,Data) + target_len - sizeof(WCHAR),
        "wrong len %lu\n", len );
    pNtClose( key );

    if (pNtOpenKeyEx)
    {
        /* REG_OPTION_OPEN_LINK flag doesn't matter */
        status = pNtOpenKeyEx( &key, KEY_ALL_ACCESS, &attr, REG_OPTION_OPEN_LINK );
        ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );

        len = sizeof(buffer);
        status = pNtQueryValueKey( key, &symlink_str, KeyValuePartialInformation, info, len, &len );
        ok( status == STATUS_SUCCESS, "NtQueryValueKey failed: 0x%08lx\n", status );
        ok( len == FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION,Data) + target_len - sizeof(WCHAR),
            "wrong len %lu\n", len );
        pNtClose( key );

        status = pNtOpenKeyEx( &key, KEY_ALL_ACCESS, &attr, 0 );
        ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );

        len = sizeof(buffer);
        status = pNtQueryValueKey( key, &symlink_str, KeyValuePartialInformation, info, len, &len );
        ok( status == STATUS_SUCCESS, "NtQueryValueKey failed: 0x%08lx\n", status );
        ok( len == FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION,Data) + target_len - sizeof(WCHAR),
            "wrong len %lu\n", len );
        pNtClose( key );

        attr.Attributes = 0;
        status = pNtOpenKeyEx( &key, KEY_ALL_ACCESS, &attr, REG_OPTION_OPEN_LINK );
        ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );

        len = sizeof(buffer);
        status = pNtQueryValueKey( key, &symlink_str, KeyValuePartialInformation, info, len, &len );
        ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "NtQueryValueKey failed: 0x%08lx\n", status );
        pNtClose( key );
    }

    attr.Attributes = OBJ_OPENLINK;
    status = pNtCreateKey( &key, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );
    len = sizeof(buffer);
    status = pNtQueryValueKey( key, &symlink_str, KeyValuePartialInformation, info, len, &len );
    ok( status == STATUS_SUCCESS, "NtQueryValueKey failed: 0x%08lx\n", status );
    ok( len == FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION,Data) + target_len - sizeof(WCHAR),
        "wrong len %lu\n", len );
    pNtClose( key );

    /* delete target and create by NtCreateKey on link */
    attr.ObjectName = &target_str;
    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );
    status = pNtDeleteKey( key );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( key );

    attr.ObjectName = &link_str;
    attr.Attributes = 0;
    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "NtOpenKey wrong status 0x%08lx\n", status );

    status = pNtCreateKey( &key, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );
    pNtClose( key );

    attr.ObjectName = &target_str;
    attr.Attributes = OBJ_OPENLINK;
    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey wrong status 0x%08lx\n", status );

    if (0)  /* crashes the Windows kernel on some Vista systems */
    {
        /* reopen the link from itself */

        attr.RootDirectory = link;
        attr.Attributes = OBJ_OPENLINK;
        attr.ObjectName = &null_str;
        status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
        ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );
        len = sizeof(buffer);
        status = pNtQueryValueKey( key, &symlink_str, KeyValuePartialInformation, info, len, &len );
        ok( status == STATUS_SUCCESS, "NtQueryValueKey failed: 0x%08lx\n", status );
        ok( len == FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION,Data) + target_len - sizeof(WCHAR),
            "wrong len %lu\n", len );
        pNtClose( key );

        status = pNtCreateKey( &key, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
        ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );
        len = sizeof(buffer);
        status = pNtQueryValueKey( key, &symlink_str, KeyValuePartialInformation, info, len, &len );
        ok( status == STATUS_SUCCESS, "NtQueryValueKey failed: 0x%08lx\n", status );
        ok( len == FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION,Data) + target_len - sizeof(WCHAR),
            "wrong len %lu\n", len );
        pNtClose( key );
    }

    if (0)  /* crashes the Windows kernel in most versions */
    {
        attr.RootDirectory = link;
        attr.Attributes = 0;
        attr.ObjectName = &null_str;
        status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
        ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );
        len = sizeof(buffer);
        status = pNtQueryValueKey( key, &symlink_str, KeyValuePartialInformation, info, len, &len );
        ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "NtQueryValueKey failed: 0x%08lx\n", status );
        pNtClose( key );

        status = pNtCreateKey( &key, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
        ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );
        len = sizeof(buffer);
        status = pNtQueryValueKey( key, &symlink_str, KeyValuePartialInformation, info, len, &len );
        ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "NtQueryValueKey failed: 0x%08lx\n", status );
        pNtClose( key );
    }

    /* target with terminating null doesn't work */
    status = pNtSetValueKey( link, &symlink_str, 0, REG_LINK, target, target_len );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08lx\n", status );
    attr.RootDirectory = root;
    attr.Attributes = 0;
    attr.ObjectName = &link_str;
    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "NtOpenKey wrong status 0x%08lx\n", status );

    /* relative symlink, works only on win2k */
    status = pNtSetValueKey( link, &symlink_str, 0, REG_LINK, targetW+1, sizeof(targetW)-2*sizeof(WCHAR) );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08lx\n", status );
    attr.ObjectName = &link_str;
    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND || status == STATUS_OBJECT_NAME_INVALID /* Win10 1607+ */,
        "NtOpenKey wrong status 0x%08lx\n", status );

    key = (HKEY)0xdeadbeef;
    status = pNtCreateKey( &key, KEY_ALL_ACCESS, &attr, 0, 0, REG_OPTION_CREATE_LINK, NULL );
    ok( status == STATUS_OBJECT_NAME_COLLISION, "NtCreateKey failed: 0x%08lx\n", status );
    ok( !key, "key = %p\n", key );

    status = pNtDeleteKey( link );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( link );

    attr.ObjectName = &target_str;
    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );
    status = pNtDeleteKey( key );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( key );

    /* symlink loop */

    status = pNtCreateKey( &link, KEY_ALL_ACCESS, &attr, 0, 0, REG_OPTION_CREATE_LINK, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );
    memcpy( target + target_len/sizeof(WCHAR) - 1, targetW, sizeof(targetW) );
    status = pNtSetValueKey( link, &symlink_str, 0, REG_LINK,
        target, target_len + sizeof(targetW) - sizeof(WCHAR) );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08lx\n", status );

    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_NAME_TOO_LONG || status == STATUS_INVALID_PARAMETER /* Win10 1607+ */,
        "NtOpenKey failed: 0x%08lx\n", status );

    attr.Attributes = OBJ_OPENLINK;
    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );
    pNtClose( key );

    status = pNtDeleteKey( link );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( link );

    status = pNtDeleteKey( root );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
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

    status = pNtOpenKey( &key, flags | KEY_ALL_ACCESS, &attr );
    if (status == STATUS_OBJECT_NAME_NOT_FOUND) return 0;
    ok( status == STATUS_SUCCESS, "%08lx: NtCreateKey failed: 0x%08lx\n", flags, status );

    status = pNtQueryValueKey( key, &value_str, KeyValuePartialInformation, info, len, &len );
    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
        dw = 0;
    else
    {
        ok( status == STATUS_SUCCESS, "%08lx: NtQueryValueKey failed: 0x%08lx\n", flags, status );
        dw = *(DWORD *)info->Data;
    }
    pNtClose( key );
    pRtlFreeUnicodeString( &str );
    return dw;
}

static void _check_key_value( int line, HANDLE root, const char *name, DWORD flags, DWORD expect )
{
    DWORD dw = get_key_value( root, name, flags );
    ok_(__FILE__,line)( dw == expect, "%08lx: wrong value %lu/%lu\n", flags, dw, expect );
}
#define check_key_value(root,name,flags,expect) _check_key_value( __LINE__, root, name, flags, expect )

static void _check_enum_value( int line, const WCHAR *name, DWORD flags, int subkeys, BOOL present)
{
    static const WCHAR wineW[] = {'W','i','n','e'};
    char buffer[1024];
    KEY_BASIC_INFORMATION *basic_info = (KEY_BASIC_INFORMATION *)buffer;
    KEY_FULL_INFORMATION *full_info = (KEY_FULL_INFORMATION *)buffer;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    NTSTATUS status;
    BOOL found;
    HANDLE key;
    DWORD len;
    int i;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &str;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    pRtlInitUnicodeString( &str, name );
    status = pNtOpenKey( &key, flags, &attr );
    ok_( __FILE__, line )( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );

    status = pNtQueryKey( key, KeyFullInformation, full_info, sizeof(buffer), &len );
    ok_( __FILE__, line )( status == STATUS_SUCCESS, "NtQueryKey failed: 0x%08lx\n", status );
    ok_( __FILE__, line )( full_info->SubKeys == subkeys, "wrong number of subkeys: %lu\n", full_info->SubKeys );
    subkeys = full_info->SubKeys;

    found = FALSE;
    for (i = 0; i < subkeys; i++)
    {
        status = pNtEnumerateKey( key, i, KeyBasicInformation, basic_info, sizeof(buffer), &len );
        ok_( __FILE__, line )( status == STATUS_SUCCESS, "NtEnumerateKey failed: 0x%08lx\n", status );

        if (basic_info->NameLength == sizeof(wineW) && !memcmp(basic_info->Name, wineW, sizeof(wineW) ))
            found = TRUE;
    }
    ok_( __FILE__, line )( found == present, "found equals %d\n", found );
    pNtClose( key );

    status = pNtCreateKey( &key, flags, &attr, 0, 0, 0, 0 );
    ok_( __FILE__, line )( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );

    status = pNtQueryKey( key, KeyFullInformation, full_info, sizeof(buffer), &len );
    ok_( __FILE__, line )( status == STATUS_SUCCESS, "NtQueryKey failed: 0x%08lx\n", status );
    ok_( __FILE__, line )( full_info->SubKeys == subkeys, "wrong number of subkeys: %lu\n", full_info->SubKeys );
    subkeys = full_info->SubKeys;

    found = FALSE;
    for (i = 0; i < subkeys; i++)
    {
        status = pNtEnumerateKey( key, i, KeyBasicInformation, basic_info, sizeof(buffer), &len );
        ok_( __FILE__, line )( status == STATUS_SUCCESS, "NtEnumerateKey failed: 0x%08lx\n", status );

        if (basic_info->NameLength == sizeof(wineW) && !memcmp(basic_info->Name, wineW, sizeof(wineW) ))
            found = TRUE;
    }
    ok_( __FILE__, line )( found == present, "found equals %d\n", found );
    pNtClose( key );
}
#define check_enum_value(name, flags, subkeys, present) _check_enum_value( __LINE__, name, flags, subkeys, present )

static void test_redirection(void)
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    char buffer[1024];
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    KEY_FULL_INFORMATION *full_info = (KEY_FULL_INFORMATION *)buffer;
    DWORD dw, len;
    HANDLE key, key32, key64, root, root32, root64;
    int subkeys64, subkeys32;

    if (ptr_size != 64)
    {
        ULONG is_wow64, len;
        if (NtQueryInformationProcess( GetCurrentProcess(), ProcessWow64Information,
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

    pRtlInitUnicodeString( &str, L"\\Registry\\Machine\\Software\\Wine" );
    status = pNtCreateKey( &root64, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    if (status == STATUS_ACCESS_DENIED)
    {
        skip("Not authorized to modify KEY_WOW64_64KEY, no redirection\n");
        return;
    }
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );

    pRtlInitUnicodeString( &str, L"\\Registry\\Machine\\Software\\Wow6432Node\\Wine" );
    status = pNtCreateKey( &root32, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );

    pRtlInitUnicodeString( &str, L"\\Registry\\Machine\\Software\\Wine\\Winetest" );
    status = pNtCreateKey( &key64, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );

    pRtlInitUnicodeString( &str, L"\\Registry\\Machine\\Software\\Wow6432Node\\Wine\\Winetest" );
    status = pNtCreateKey( &key32, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );

    dw = 64;
    status = pNtSetValueKey( key64, &value_str, 0, REG_DWORD, &dw, sizeof(dw) );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08lx\n", status );

    dw = 32;
    status = pNtSetValueKey( key32, &value_str, 0, REG_DWORD, &dw, sizeof(dw) );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08lx\n", status );

    len = sizeof(buffer);
    status = pNtQueryValueKey( key32, &value_str, KeyValuePartialInformation, info, len, &len );
    ok( status == STATUS_SUCCESS, "NtQueryValueKey failed: 0x%08lx\n", status );
    dw = *(DWORD *)info->Data;
    ok( dw == 32, "wrong value %lu\n", dw );

    len = sizeof(buffer);
    status = pNtQueryValueKey( key64, &value_str, KeyValuePartialInformation, info, len, &len );
    ok( status == STATUS_SUCCESS, "NtQueryValueKey failed: 0x%08lx\n", status );
    dw = *(DWORD *)info->Data;
    ok( dw == 64, "wrong value %lu\n", dw );

    pRtlInitUnicodeString( &str, L"\\Registry\\Machine\\Software" );
    status = pNtCreateKey( &key, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );

    check_key_value( key, "Wine\\Winetest", 0, ptr_size );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_64KEY, ptr_size );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_32KEY, ptr_size );
    check_key_value( key, "Wow6432Node\\Wine\\Winetest", 0, ptr_size == 32 ? 0 : 32 );
    check_key_value( key, "Wow6432Node\\Wine\\Winetest", KEY_WOW64_64KEY, ptr_size == 32 ? 0 : 32 );
    check_key_value( key, "Wow6432Node\\Wine\\Winetest", KEY_WOW64_32KEY, ptr_size == 32 ? 0 : 32 );
    pNtClose( key );

    status = pNtCreateKey( &key, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );
    dw = get_key_value( key, "Wine\\Winetest", 0 );
    ok( dw == 64 || broken(dw == 32) /* win7 */, "wrong value %lu\n", dw );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_64KEY, 64 );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_32KEY, ptr_size );
    check_key_value( key, "Wow6432Node\\Wine\\Winetest", 0, 32 );
    check_key_value( key, "Wow6432Node\\Wine\\Winetest", KEY_WOW64_64KEY, 32 );
    check_key_value( key, "Wow6432Node\\Wine\\Winetest", KEY_WOW64_32KEY, 32 );
    pNtClose( key );

    status = pNtCreateKey( &key, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );
    check_key_value( key, "Wine\\Winetest", 0, ptr_size );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_64KEY, ptr_size );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_32KEY, ptr_size );
    check_key_value( key, "Wow6432Node\\Wine\\Winetest", 0, ptr_size == 32 ? 0 : 32 );
    check_key_value( key, "Wow6432Node\\Wine\\Winetest", KEY_WOW64_64KEY, ptr_size == 32 ? 0 : 32 );
    check_key_value( key, "Wow6432Node\\Wine\\Winetest", KEY_WOW64_32KEY, ptr_size == 32 ? 0 : 32 );
    pNtClose( key );

    check_key_value( 0, "\\Registry\\Machine\\Software\\Wine\\Winetest", 0, ptr_size );
    check_key_value( 0, "\\Registry\\Machine\\Software\\Wow6432Node\\Wine\\Winetest", 0, 32 );
    check_key_value( 0, "\\Registry\\Machine\\Software\\Wine\\Winetest", KEY_WOW64_64KEY, 64 );
    check_key_value( 0, "\\Registry\\Machine\\Software\\Wine\\Winetest", KEY_WOW64_32KEY, ptr_size );
    check_key_value( 0, "\\Registry\\Machine\\Software\\Wow6432Node\\Wine\\Winetest", KEY_WOW64_64KEY, 32 );
    check_key_value( 0, "\\Registry\\Machine\\Software\\Wow6432Node\\Wine\\Winetest", KEY_WOW64_32KEY, 32 );

    pRtlInitUnicodeString( &str, L"\\Registry\\Machine\\Software\\Wow6432Node" );
    status = pNtCreateKey( &key, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );
    check_key_value( key, "Wine\\Winetest", 0, 32 );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_64KEY, 32 );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_32KEY, 32 );
    pNtClose( key );

    status = pNtCreateKey( &key, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );
    check_key_value( key, "Wine\\Winetest", 0, 32 );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_64KEY, 32 );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_32KEY, 32 );
    pNtClose( key );

    status = pNtCreateKey( &key, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );
    check_key_value( key, "Wine\\Winetest", 0, 32 );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_64KEY, 32 );
    check_key_value( key, "Wine\\Winetest", KEY_WOW64_32KEY, 32 );
    pNtClose( key );

    pRtlInitUnicodeString( &str, L"\\Registry\\Machine\\Software\\Wow6432Node\\Wine" );
    status = pNtCreateKey( &key, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );
    check_key_value( key, "Winetest", 0, 32 );
    check_key_value( key, "Winetest", KEY_WOW64_64KEY, 32 );
    check_key_value( key, "Winetest", KEY_WOW64_32KEY, 32 );
    pNtClose( key );

    status = pNtCreateKey( &key, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );
    check_key_value( key, "Winetest", 0, 32 );
    check_key_value( key, "Winetest", KEY_WOW64_64KEY, 32 );
    check_key_value( key, "Winetest", KEY_WOW64_32KEY, 32 );
    pNtClose( key );

    status = pNtCreateKey( &key, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );
    check_key_value( key, "Winetest", 0, 32 );
    check_key_value( key, "Winetest", KEY_WOW64_64KEY, 32 );
    check_key_value( key, "Winetest", KEY_WOW64_32KEY, 32 );
    pNtClose( key );

    pRtlInitUnicodeString( &str, L"\\Registry\\Machine\\Software\\Wine" );
    status = pNtCreateKey( &key, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );
    check_key_value( key, "Winetest", 0, ptr_size );
    check_key_value( key, "Winetest", KEY_WOW64_64KEY, ptr_size );
    check_key_value( key, "Winetest", KEY_WOW64_32KEY, ptr_size );
    pNtClose( key );

    status = pNtCreateKey( &key, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );
    check_key_value( key, "Winetest", 0, 64 );
    check_key_value( key, "Winetest", KEY_WOW64_64KEY, 64 );
    check_key_value( key, "Winetest", KEY_WOW64_32KEY, ptr_size );
    pNtClose( key );

    status = pNtCreateKey( &key, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );
    check_key_value( key, "Winetest", 0, ptr_size );
    check_key_value( key, "Winetest", KEY_WOW64_64KEY, ptr_size );
    check_key_value( key, "Winetest", KEY_WOW64_32KEY, ptr_size );
    pNtClose( key );

    status = pNtDeleteKey( key32 );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( key32 );

    status = pNtDeleteKey( key64 );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( key64 );

    pRtlInitUnicodeString( &str, L"Winetest" );
    attr.RootDirectory = root64;
    status = pNtCreateKey( &key32, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );

    pRtlInitUnicodeString( &str, L"\\Registry\\Machine\\Software\\Wow6432Node\\Wine\\Winetest" );
    attr.RootDirectory = 0;
    status = pNtOpenKey( &key, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr );
    ok( status == (ptr_size == 64 ? STATUS_OBJECT_NAME_NOT_FOUND : STATUS_SUCCESS),
        "NtOpenKey failed: 0x%08lx\n", status );
    pNtClose( key );

    status = pNtDeleteKey( key32 );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( key32 );

    status = pNtDeleteKey( root32 );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( root32 );
    status = pNtDeleteKey( root64 );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( root64 );

    /* Software\Classes is shared/reflected so behavior is different */

    pRtlInitUnicodeString( &str, L"\\Registry\\Machine\\Software\\Classes\\Wine" );
    status = pNtCreateKey( &key64, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    if (status == STATUS_ACCESS_DENIED)
    {
        skip("Not authorized to modify the Classes key\n");
        return;
    }
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );

    pRtlInitUnicodeString( &str, L"\\Registry\\Machine\\Software\\Classes\\Wow6432Node\\Wine" );
    status = pNtOpenKey( &key, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr );
    ok( status == (ptr_size == 64 ? STATUS_OBJECT_NAME_NOT_FOUND : STATUS_SUCCESS),
        "NtOpenKey failed: 0x%08lx\n", status );
    if (!status) pNtClose( key );

    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == (ptr_size == 64 ? STATUS_OBJECT_NAME_NOT_FOUND : STATUS_SUCCESS),
        "NtOpenKey failed: 0x%08lx\n", status );
    if (!status) pNtClose( key );

    status = pNtCreateKey( &key32, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );

    dw = 32;
    status = pNtSetValueKey( key32, &value_str, 0, REG_DWORD, &dw, sizeof(dw) );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08lx\n", status );

    dw = 64;
    status = pNtSetValueKey( key64, &value_str, 0, REG_DWORD, &dw, sizeof(dw) );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08lx\n", status );

    check_key_value( 0, "\\Registry\\Machine\\Software\\Classes\\Wine", 0, 64 );
    check_key_value( 0, "\\Registry\\Machine\\Software\\Classes\\Wine", KEY_WOW64_64KEY, 64 );
    check_key_value( 0, "\\Registry\\Machine\\Software\\Classes\\Wine", KEY_WOW64_32KEY, 64 );
    check_key_value( 0, "\\Registry\\Machine\\Software\\Classes\\Wow6432Node\\Wine", 0, ptr_size == 64 ? 32 : 64 );
    check_key_value( 0, "\\Registry\\Machine\\Software\\Classes\\Wow6432Node\\Wine", KEY_WOW64_64KEY, ptr_size == 64 ? 32 : 0 );
    check_key_value( 0, "\\Registry\\Machine\\Software\\Classes\\Wow6432Node\\Wine", KEY_WOW64_32KEY, ptr_size == 64 ? 32 : 64 );

    status = pNtDeleteKey( key32 );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( key32 );

    check_key_value( 0, "\\Registry\\Machine\\Software\\Classes\\Wine", 0, ptr_size == 32 ? 0 : 64 );
    check_key_value( 0, "\\Registry\\Machine\\Software\\Classes\\Wine", KEY_WOW64_64KEY, ptr_size == 32 ? 0 : 64 );
    check_key_value( 0, "\\Registry\\Machine\\Software\\Classes\\Wine", KEY_WOW64_32KEY, ptr_size == 32 ? 0 : 64 );
    check_key_value( 0, "\\Registry\\Machine\\Software\\Classes\\Wow6432Node\\Wine", 0, 0 );
    check_key_value( 0, "\\Registry\\Machine\\Software\\Classes\\Wow6432Node\\Wine", KEY_WOW64_64KEY, 0 );
    check_key_value( 0, "\\Registry\\Machine\\Software\\Classes\\Wow6432Node\\Wine", KEY_WOW64_32KEY, 0 );

    status = pNtDeleteKey( key64 );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( key64 );

    pRtlInitUnicodeString( &str, L"\\Registry\\Machine\\Software\\Classes" );
    status = pNtOpenKey( &root64, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );

    status = pNtOpenKey( &root32, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );

    pRtlInitUnicodeString( &str, L"Wine" );
    attr.RootDirectory = root64;
    status = pNtCreateKey( &key64, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );

    attr.RootDirectory = key64;
    status = pNtCreateKey( &key, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );
    status = pNtDeleteKey( key );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( key );

    attr.RootDirectory = root32;
    status = pNtOpenKey( &key, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );
    pNtClose( key );

    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );
    pNtClose( key );

    status = pNtCreateKey( &key32, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );

    dw = 32;
    status = pNtSetValueKey( key32, &value_str, 0, REG_DWORD, &dw, sizeof(dw) );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08lx\n", status );

    dw = 64;
    status = pNtSetValueKey( key64, &value_str, 0, REG_DWORD, &dw, sizeof(dw) );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08lx\n", status );

    check_key_value( root64, "Wine", 0, 64 );
    check_key_value( root64, "Wine", KEY_WOW64_64KEY, 64 );
    check_key_value( root64, "Wine", KEY_WOW64_32KEY, 64 );
    check_key_value( root32, "Wine", 0, 64 );
    check_key_value( root32, "Wine", KEY_WOW64_64KEY, 64 );
    check_key_value( root32, "Wine", KEY_WOW64_32KEY, 64 );

    status = pNtDeleteKey( key32 );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( key32 );

    check_key_value( root64, "Wine", 0, 0 );
    check_key_value( root64, "Wine", KEY_WOW64_64KEY, 0 );
    check_key_value( root64, "Wine", KEY_WOW64_32KEY, 0 );
    check_key_value( root32, "Wine", 0, 0 );
    check_key_value( root32, "Wine", KEY_WOW64_64KEY, 0 );
    check_key_value( root32, "Wine", KEY_WOW64_32KEY, 0 );

    status = pNtDeleteKey( key64 );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( key64 );

    attr.RootDirectory = root32;
    status = pNtCreateKey( &key32, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );

    dw = 32;
    status = pNtSetValueKey( key32, &value_str, 0, REG_DWORD, &dw, sizeof(dw) );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08lx\n", status );

    check_key_value( root64, "Wine", 0, 32 );
    check_key_value( root64, "Wine", KEY_WOW64_64KEY, 32 );
    check_key_value( root64, "Wine", KEY_WOW64_32KEY, 32 );
    check_key_value( root32, "Wine", 0, 32 );
    check_key_value( root32, "Wine", KEY_WOW64_64KEY, 32 );
    check_key_value( root32, "Wine", KEY_WOW64_32KEY, 32 );

    status = pNtDeleteKey( key32 );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( key32 );

    pNtClose( root64 );
    pNtClose( root32 );

    pRtlInitUnicodeString( &str, L"\\Registry\\Machine\\Software\\Classes" );
    attr.RootDirectory = 0;
    status = pNtOpenKey( &root64, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );

    status = pNtOpenKey( &root32, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );

    status = pNtOpenKey( &root, KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );

    pRtlInitUnicodeString( &str, L"Interface" );
    attr.RootDirectory = root64;
    status = pNtOpenKey( &key64, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );

    attr.RootDirectory = root32;
    status = pNtOpenKey( &key32, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );

    attr.RootDirectory = root;
    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );

    pNtClose( root64 );
    pNtClose( root32 );
    pNtClose( root );

    root64 = key64;
    root32 = key32;
    root = key;

    pRtlInitUnicodeString( &str, L"Wine" );
    attr.RootDirectory = root32;
    status = pNtCreateKey( &key32, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );

    attr.RootDirectory = root;
    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );
    pNtClose( key );

    status = pNtDeleteKey( key32 );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( key32 );

    attr.RootDirectory = root64;
    status = pNtCreateKey( &key64, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );

    attr.RootDirectory = root;
    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == (ptr_size == 32 ? STATUS_OBJECT_NAME_NOT_FOUND : STATUS_SUCCESS),
        "NtOpenKey failed: 0x%08lx\n", status );
    if (!status) pNtClose( key );

    status = pNtDeleteKey( key64 );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( key64 );

    status = pNtDeleteKey( root );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( root );

    attr.RootDirectory = root64;
    status = pNtCreateKey( &key64, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );

    attr.RootDirectory = root32;
    status = pNtOpenKey( &key, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr );
    ok( status == (ptr_size == 32 ? STATUS_OBJECT_NAME_NOT_FOUND : STATUS_SUCCESS),
        "NtOpenKey failed: 0x%08lx\n", status );
    if (!status) pNtClose( key );

    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == (ptr_size == 32 ? STATUS_OBJECT_NAME_NOT_FOUND : STATUS_SUCCESS),
        "NtOpenKey failed: 0x%08lx\n", status );
    if (!status) pNtClose( key );

    status = pNtCreateKey( &key32, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );

    dw = 32;
    status = pNtSetValueKey( key32, &value_str, 0, REG_DWORD, &dw, sizeof(dw) );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08lx\n", status );

    dw = 64;
    status = pNtSetValueKey( key64, &value_str, 0, REG_DWORD, &dw, sizeof(dw) );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08lx\n", status );

    check_key_value( root64, "Wine", 0, 64 );
    check_key_value( root64, "Wine", KEY_WOW64_64KEY, 64 );
    check_key_value( root64, "Wine", KEY_WOW64_32KEY, ptr_size );
    check_key_value( root32, "Wine", 0, ptr_size );
    check_key_value( root32, "Wine", KEY_WOW64_64KEY, ptr_size );
    check_key_value( root32, "Wine", KEY_WOW64_32KEY, ptr_size );

    pRtlInitUnicodeString( &str, L"\\Registry\\Machine\\Software\\Classes\\Interface" );
    attr.RootDirectory = 0;
    status = pNtOpenKey( &key, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );
    check_key_value( key, "Wine", 0, 64 );
    check_key_value( key, "Wine", KEY_WOW64_64KEY, 64 );
    check_key_value( key, "Wine", KEY_WOW64_32KEY, ptr_size );
    pNtClose( key );

    status = pNtOpenKey( &key, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );
    check_key_value( key, "Wine", 0, ptr_size );
    check_key_value( key, "Wine", KEY_WOW64_64KEY, ptr_size );
    check_key_value( key, "Wine", KEY_WOW64_32KEY, ptr_size );
    pNtClose( key );

    check_key_value( 0, "\\Registry\\Machine\\Software\\Classes\\Interface\\Wine", 0, ptr_size );
    check_key_value( 0, "\\Registry\\Machine\\Software\\Classes\\Interface\\Wine", KEY_WOW64_64KEY, 64 );
    check_key_value( 0, "\\Registry\\Machine\\Software\\Classes\\Interface\\Wine", KEY_WOW64_32KEY, ptr_size );

    status = pNtDeleteKey( key32 );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( key32 );

    check_key_value( root64, "Wine", 0, ptr_size == 64 ? 0 : 64 );
    check_key_value( root64, "Wine", KEY_WOW64_64KEY, ptr_size == 64 ? 0 : 64 );
    check_key_value( root64, "Wine", KEY_WOW64_32KEY, 0 );
    check_key_value( root32, "Wine", 0, 0 );
    check_key_value( root32, "Wine", KEY_WOW64_64KEY, 0 );
    check_key_value( root32, "Wine", KEY_WOW64_32KEY, 0 );

    check_key_value( 0, "\\Registry\\Machine\\Software\\Classes\\Interface\\Wine", 0, 0 );
    check_key_value( 0, "\\Registry\\Machine\\Software\\Classes\\Interface\\Wine", KEY_WOW64_64KEY, ptr_size == 64 ? 0 : 64 );
    check_key_value( 0, "\\Registry\\Machine\\Software\\Classes\\Interface\\Wine", KEY_WOW64_32KEY, 0 );

    status = pNtDeleteKey( key64 );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( key64 );

    pRtlInitUnicodeString( &str, L"Wine" );
    attr.RootDirectory = root32;
    status = pNtCreateKey( &key32, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );

    dw = 32;
    status = pNtSetValueKey( key32, &value_str, 0, REG_DWORD, &dw, sizeof(dw) );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08lx\n", status );

    check_key_value( root64, "Wine", 0, ptr_size == 64 ? 32 : 0 );
    check_key_value( root64, "Wine", KEY_WOW64_64KEY, ptr_size == 64 ? 32 : 0 );
    check_key_value( root64, "Wine", KEY_WOW64_32KEY, 32 );
    check_key_value( root32, "Wine", 0, 32 );
    check_key_value( root32, "Wine", KEY_WOW64_64KEY, 32 );
    check_key_value( root32, "Wine", KEY_WOW64_32KEY, 32 );

    status = pNtDeleteKey( key32 );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( key32 );

    pNtClose( root64 );
    pNtClose( root32 );

    pRtlInitUnicodeString( &str, L"\\Registry\\Machine\\Software\\Classes\\Wow6432Node\\Wine" );
    attr.RootDirectory = 0;
    status = pNtCreateKey( &key32, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status );

    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );
    pNtClose( key );

    status = pNtOpenKey( &key, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );
    pNtClose( key );

    status = pNtOpenKey( &key, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &attr );
    ok( status == (ptr_size == 32 ? STATUS_OBJECT_NAME_NOT_FOUND : STATUS_SUCCESS),
        "NtOpenKey failed: 0x%08lx\n", status );
    if (!status) pNtClose( key );

    pRtlInitUnicodeString( &str, L"\\Registry\\Machine\\Software\\Classes\\Wine" );
    status = pNtOpenKey( &key, KEY_ALL_ACCESS, &attr );
    ok( status == (ptr_size == 64 ? STATUS_OBJECT_NAME_NOT_FOUND : STATUS_SUCCESS),
        "NtOpenKey failed: 0x%08lx\n", status );
    if (!status) pNtClose( key );

    status = pNtOpenKey( &key, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr );
    ok( status == (ptr_size == 64 ? STATUS_OBJECT_NAME_NOT_FOUND : STATUS_SUCCESS),
        "NtOpenKey failed: 0x%08lx\n", status );
    if (!status) pNtClose( key );

    status = pNtOpenKey( &key, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &attr );
    ok( status == (ptr_size == 64 ? STATUS_OBJECT_NAME_NOT_FOUND : STATUS_SUCCESS),
        "NtOpenKey failed: 0x%08lx\n", status );
    if (!status) pNtClose( key );

    pRtlInitUnicodeString( &str, L"\\Registry\\Machine\\Software\\Classes\\Wow6432Node" );
    status = pNtOpenKey( &root32, KEY_WOW64_32KEY | KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );

    status = pNtQueryKey( root32, KeyFullInformation, full_info, sizeof(buffer), &len );
    ok( status == STATUS_SUCCESS, "NtQueryKey failed: 0x%08lx\n", status );
    ok( full_info->SubKeys > 0, "wrong number of subkeys: %lu\n", full_info->SubKeys );
    subkeys32 = full_info->SubKeys;
    pNtClose( root32 );

    pRtlInitUnicodeString( &str, L"\\Registry\\Machine\\Software\\Classes" );
    status = pNtOpenKey( &root64, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenKey failed: 0x%08lx\n", status );

    status = pNtQueryKey( root64, KeyFullInformation, full_info, sizeof(buffer), &len );
    ok( status == STATUS_SUCCESS, "NtQueryKey failed: 0x%08lx\n", status );
    ok( full_info->SubKeys > subkeys32, "wrong number of subkeys: %lu\n", full_info->SubKeys );
    subkeys64 = full_info->SubKeys;
    pNtClose( root64 );

    check_enum_value( L"\\Registry\\Machine\\Software\\Classes",
                      KEY_WOW64_32KEY | KEY_ALL_ACCESS, subkeys64, ptr_size == 32 );
    check_enum_value( L"\\Registry\\Machine\\Software\\Classes",
                      KEY_WOW64_64KEY | KEY_ALL_ACCESS, subkeys64, ptr_size == 32 );
    check_enum_value( L"\\Registry\\Machine\\Software\\Classes",
                      KEY_ALL_ACCESS, subkeys64, ptr_size == 32 );
    check_enum_value( L"\\Registry\\Machine\\Software\\Classes\\Wow6432Node",
                      KEY_WOW64_32KEY | KEY_ALL_ACCESS, subkeys32, ptr_size == 64 );
    check_enum_value( L"\\Registry\\Machine\\Software\\Classes\\Wow6432Node",
                      KEY_WOW64_64KEY | KEY_ALL_ACCESS, subkeys32, ptr_size == 64 );
    check_enum_value( L"\\Registry\\Machine\\Software\\Classes\\Wow6432Node",
                      KEY_ALL_ACCESS, subkeys32, ptr_size == 64 );
    check_enum_value( L"\\Registry\\Machine\\Software\\Wow6432Node\\Classes",
                      KEY_WOW64_32KEY | KEY_ALL_ACCESS, ptr_size == 32 ? subkeys64 : subkeys32, TRUE );
    check_enum_value( L"\\Registry\\Machine\\Software\\Wow6432Node\\Classes",
                      KEY_WOW64_64KEY | KEY_ALL_ACCESS, subkeys32, ptr_size == 64 );
    check_enum_value( L"\\Registry\\Machine\\Software\\Wow6432Node\\Classes",
                      KEY_ALL_ACCESS, ptr_size == 32 ? subkeys64 : subkeys32, TRUE );

    status = pNtDeleteKey( key32 );
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose( key32 );
}

static void test_long_value_name(void)
{
    HANDLE key;
    NTSTATUS status;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING ValName;
    DWORD i;

    InitializeObjectAttributes(&attr, &winetestpath, 0, 0, 0);
    status = pNtOpenKey(&key, KEY_WRITE|KEY_READ, &attr);
    ok(status == STATUS_SUCCESS, "NtOpenKey Failed: 0x%08lx\n", status);

    ValName.MaximumLength = 0xfffc;
    ValName.Length = ValName.MaximumLength - sizeof(WCHAR);
    ValName.Buffer = HeapAlloc(GetProcessHeap(), 0, ValName.MaximumLength);
    for (i = 0; i < ValName.Length / sizeof(WCHAR); i++)
        ValName.Buffer[i] = 'a';
    ValName.Buffer[i] = 0;

    status = pNtDeleteValueKey(key, &ValName);
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND, "NtDeleteValueKey with nonexistent long value name returned 0x%08lx\n", status);
    status = pNtSetValueKey(key, &ValName, 0, REG_DWORD, &i, sizeof(i));
    ok(status == STATUS_INVALID_PARAMETER, "NtSetValueKey with long value name returned 0x%08lx\n", status);

    status = pNtQueryValueKey(key, &ValName, KeyValueBasicInformation, NULL, 0, &i);
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND, "NtQueryValueKey with nonexistent long value name returned 0x%08lx\n", status);

    pRtlFreeUnicodeString(&ValName);
    pNtClose(key);
}

static void test_NtQueryKey(void)
{
    HANDLE key, subkey, subkey2;
    NTSTATUS status;
    OBJECT_ATTRIBUTES attr;
    ULONG length, len;
    KEY_NAME_INFORMATION *info = NULL;
    KEY_CACHED_INFORMATION cached_info;
    UNICODE_STRING str;
    DWORD dw;

    InitializeObjectAttributes(&attr, &winetestpath, 0, 0, 0);
    status = pNtOpenKey(&key, KEY_READ, &attr);
    ok(status == STATUS_SUCCESS, "NtOpenKey Failed: 0x%08lx\n", status);

    status = pNtQueryKey(key, KeyNameInformation, NULL, 0, &length);
    if (status == STATUS_INVALID_PARAMETER) {
        win_skip("KeyNameInformation is not supported\n");
        pNtClose(key);
        return;
    }
    ok(status == STATUS_BUFFER_TOO_SMALL, "NtQueryKey Failed: 0x%08lx\n", status);
    info = HeapAlloc(GetProcessHeap(), 0, length);

    /* non-zero buffer size, but insufficient */
    len = 0;
    status = pNtQueryKey(key, KeyNameInformation, info, 1, &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "NtQueryKey Failed: 0x%08lx\n", status);
    ok(length == len, "got %ld, expected %ld\n", len, length);
    len = 0;
    status = pNtQueryKey(key, KeyNameInformation, info, offsetof( KEY_NAME_INFORMATION, Name ) - 1, &len);
    ok(status == STATUS_BUFFER_TOO_SMALL, "NtQueryKey Failed: 0x%08lx\n", status);
    ok(length == len, "got %ld, expected %ld\n", len, length);
    len = 0;
    status = pNtQueryKey(key, KeyNameInformation, info, offsetof( KEY_NAME_INFORMATION, Name ), &len);
    ok(status == STATUS_BUFFER_OVERFLOW, "NtQueryKey Failed: 0x%08lx\n", status);
    ok(length == len, "got %ld, expected %ld\n", len, length);
    len = 0;
    status = pNtQueryKey(key, KeyNameInformation, info, sizeof(*info), &len);
    ok(status == STATUS_BUFFER_OVERFLOW, "NtQueryKey Failed: 0x%08lx\n", status);
    ok(length == len, "got %ld, expected %ld\n", len, length);
    ok(info->NameLength == winetestpath.Length, "got %ld, expected %d\n",
       info->NameLength, winetestpath.Length);

    /* correct buffer size */
    len = 0;
    status = pNtQueryKey(key, KeyNameInformation, info, length, &len);
    ok(status == STATUS_SUCCESS, "NtQueryKey Failed: 0x%08lx\n", status);
    ok(length == len, "got %ld, expected %ld\n", len, length);

    str.Buffer = info->Name;
    str.Length = info->NameLength;
    ok(pRtlCompareUnicodeString(&winetestpath, &str, TRUE) == 0,
       "got %s, expected %s\n",
       wine_dbgstr_wn(str.Buffer, str.Length/sizeof(WCHAR)),
       wine_dbgstr_wn(winetestpath.Buffer, winetestpath.Length/sizeof(WCHAR)));

    HeapFree(GetProcessHeap(), 0, info);

    attr.RootDirectory = key;
    attr.ObjectName = &str;
    pRtlCreateUnicodeStringFromAsciiz(&str, "test_subkey");
    status = pNtCreateKey(&subkey, GENERIC_ALL, &attr, 0, 0, 0, 0);
    ok(status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status);
    pRtlFreeUnicodeString(&str);

    status = pNtQueryKey(subkey, KeyCachedInformation, &cached_info, sizeof(cached_info), &len);
    ok(status == STATUS_SUCCESS, "NtQueryKey Failed: 0x%08lx\n", status);

    if (status == STATUS_SUCCESS)
    {
        ok(len == sizeof(cached_info), "got unexpected length %ld\n", len);
        ok(cached_info.SubKeys == 0, "cached_info.SubKeys = %lu\n", cached_info.SubKeys);
        ok(cached_info.MaxNameLen == 0, "cached_info.MaxNameLen = %lu\n", cached_info.MaxNameLen);
        ok(cached_info.Values == 0, "cached_info.Values = %lu\n", cached_info.Values);
        ok(cached_info.MaxValueNameLen == 0, "cached_info.MaxValueNameLen = %lu\n", cached_info.MaxValueNameLen);
        ok(cached_info.MaxValueDataLen == 0, "cached_info.MaxValueDataLen = %lu\n", cached_info.MaxValueDataLen);
        ok(cached_info.NameLength == 22, "cached_info.NameLength = %lu\n", cached_info.NameLength);
    }

    attr.RootDirectory = subkey;
    attr.ObjectName = &str;
    pRtlCreateUnicodeStringFromAsciiz(&str, "test_subkey2");
    status = pNtCreateKey(&subkey2, GENERIC_ALL, &attr, 0, 0, 0, 0);
    ok(status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status);
    pRtlFreeUnicodeString(&str);

    pRtlCreateUnicodeStringFromAsciiz(&str, "val");
    dw = 64;
    status = pNtSetValueKey( subkey, &str, 0, REG_DWORD, &dw, sizeof(dw) );
    ok( status == STATUS_SUCCESS, "NtSetValueKey failed: 0x%08lx\n", status );
    pRtlFreeUnicodeString(&str);

    status = pNtQueryKey(subkey, KeyCachedInformation, &cached_info, sizeof(cached_info), &len);
    ok(status == STATUS_SUCCESS, "NtQueryKey Failed: 0x%08lx\n", status);

    if (status == STATUS_SUCCESS)
    {
        ok(len == sizeof(cached_info), "got unexpected length %ld\n", len);
        ok(cached_info.SubKeys == 1, "cached_info.SubKeys = %lu\n", cached_info.SubKeys);
        ok(cached_info.MaxNameLen == 24, "cached_info.MaxNameLen = %lu\n", cached_info.MaxNameLen);
        ok(cached_info.Values == 1, "cached_info.Values = %lu\n", cached_info.Values);
        ok(cached_info.MaxValueNameLen == 6, "cached_info.MaxValueNameLen = %lu\n", cached_info.MaxValueNameLen);
        ok(cached_info.MaxValueDataLen == 4, "cached_info.MaxValueDataLen = %lu\n", cached_info.MaxValueDataLen);
        ok(cached_info.NameLength == 22, "cached_info.NameLength = %lu\n", cached_info.NameLength);
    }

    status = pNtDeleteKey(subkey2);
    ok(status == STATUS_SUCCESS, "NtDeleteSubkey failed: %lx\n", status);
    status = pNtDeleteKey(subkey);
    ok(status == STATUS_SUCCESS, "NtDeleteSubkey failed: %lx\n", status);

    pNtClose(subkey2);
    pNtClose(subkey);
    pNtClose(key);
}

static void test_notify(void)
{
    OBJECT_ATTRIBUTES attr;
    static const LARGE_INTEGER timeout;
    IO_STATUS_BLOCK iosb;
    UNICODE_STRING str;
    HANDLE key, key2, events[4], subkey;
    NTSTATUS status;
    unsigned int i;

    InitializeObjectAttributes(&attr, &winetestpath, 0, 0, 0);
    status = pNtOpenKey(&key, KEY_ALL_ACCESS, &attr);
    ok(status == STATUS_SUCCESS, "NtOpenKey Failed: 0x%08lx\n", status);
    status = pNtOpenKey(&key2, KEY_ALL_ACCESS, &attr);
    ok(status == STATUS_SUCCESS, "NtOpenKey Failed: 0x%08lx\n", status);

    for (i = 0; i < ARRAY_SIZE(events); ++i)
        events[i] = CreateEventW(NULL, TRUE, TRUE, NULL);

    status = pNtNotifyChangeKey(key, events[0], NULL, NULL, &iosb, REG_NOTIFY_CHANGE_NAME, FALSE, NULL, 0, TRUE);
    ok(status == STATUS_PENDING, "NtNotifyChangeKey returned %lx\n", status);
    status = pNtNotifyChangeKey(key, events[1], NULL, NULL, &iosb, 0, FALSE, NULL, 0, TRUE);
    ok(status == STATUS_PENDING, "NtNotifyChangeKey returned %lx\n", status);
    status = pNtNotifyChangeKey(key2, events[2], NULL, NULL, &iosb, 0, FALSE, NULL, 0, TRUE);
    ok(status == STATUS_PENDING, "NtNotifyChangeKey returned %lx\n", status);
    status = pNtNotifyChangeKey(key2, events[3], NULL, NULL, &iosb, REG_NOTIFY_CHANGE_NAME, FALSE, NULL, 0, TRUE);
    ok(status == STATUS_PENDING, "NtNotifyChangeKey returned %lx\n", status);

    status = WaitForMultipleObjects(4, events, FALSE, 0);
    ok(status == WAIT_TIMEOUT, "got %ld\n", status);

    attr.RootDirectory = key;
    attr.ObjectName = &str;

    pRtlCreateUnicodeStringFromAsciiz(&str, "test_subkey");
    status = pNtCreateKey(&subkey, GENERIC_ALL, &attr, 0, 0, 0, 0);
    ok(status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status);
    pRtlFreeUnicodeString(&str);

    status = pNtWaitForSingleObject(events[0], FALSE, &timeout);
    ok(!status, "got %#lx\n", status);
    status = pNtWaitForSingleObject(events[1], FALSE, &timeout);
    ok(!status, "got %#lx\n", status);
    status = pNtWaitForSingleObject(events[2], FALSE, &timeout);
    ok(status == STATUS_TIMEOUT, "got %#lx\n", status);
    status = pNtWaitForSingleObject(events[3], FALSE, &timeout);
    ok(status == STATUS_TIMEOUT, "got %#lx\n", status);

    status = pNtNotifyChangeKey(key, events[0], NULL, NULL, &iosb, 0, FALSE, NULL, 0, TRUE);
    ok(status == STATUS_PENDING, "NtNotifyChangeKey returned %lx\n", status);

    status = pNtWaitForSingleObject(events[0], FALSE, &timeout);
    ok(status == STATUS_TIMEOUT, "got %#lx\n", status);
    status = pNtWaitForSingleObject(events[1], FALSE, &timeout);
    ok(!status, "got %#lx\n", status);
    status = pNtWaitForSingleObject(events[2], FALSE, &timeout);
    ok(status == STATUS_TIMEOUT, "got %#lx\n", status);
    status = pNtWaitForSingleObject(events[3], FALSE, &timeout);
    ok(status == STATUS_TIMEOUT, "got %#lx\n", status);

    status = pNtNotifyChangeKey(key, events[1], NULL, NULL, &iosb, 0, FALSE, NULL, 0, TRUE);
    ok(status == STATUS_PENDING, "NtNotifyChangeKey returned %lx\n", status);

    status = WaitForMultipleObjects(4, events, FALSE, 0);
    ok(status == WAIT_TIMEOUT, "got %ld\n", status);

    status = pNtDeleteKey(subkey);
    ok(status == STATUS_SUCCESS, "NtDeleteSubkey failed: %lx\n", status);

    status = pNtWaitForSingleObject(events[0], FALSE, &timeout);
    ok(!status, "got %#lx\n", status);
    status = pNtWaitForSingleObject(events[1], FALSE, &timeout);
    ok(!status, "got %#lx\n", status);
    status = pNtWaitForSingleObject(events[2], FALSE, &timeout);
    ok(status == STATUS_TIMEOUT, "got %#lx\n", status);
    status = pNtWaitForSingleObject(events[3], FALSE, &timeout);
    ok(status == STATUS_TIMEOUT, "got %#lx\n", status);

    pNtClose(subkey);

    status = pNtNotifyChangeKey(key, events[0], NULL, NULL, &iosb, 0, FALSE, NULL, 0, TRUE);
    ok(status == STATUS_PENDING, "NtNotifyChangeKey returned %lx\n", status);
    status = pNtNotifyChangeKey(key, events[1], NULL, NULL, &iosb, 0, FALSE, NULL, 0, TRUE);
    ok(status == STATUS_PENDING, "NtNotifyChangeKey returned %lx\n", status);

    pNtClose(key);

    status = pNtWaitForSingleObject(events[0], FALSE, &timeout);
    ok(!status, "got %#lx\n", status);
    status = pNtWaitForSingleObject(events[1], FALSE, &timeout);
    ok(!status, "got %#lx\n", status);
    status = pNtWaitForSingleObject(events[2], FALSE, &timeout);
    ok(status == STATUS_TIMEOUT, "got %#lx\n", status);
    status = pNtWaitForSingleObject(events[3], FALSE, &timeout);
    ok(status == STATUS_TIMEOUT, "got %#lx\n", status);

    if (pNtNotifyChangeMultipleKeys)
    {
        InitializeObjectAttributes(&attr, &winetestpath, 0, 0, 0);
        status = pNtOpenKey(&key, KEY_ALL_ACCESS, &attr);
        ok(status == STATUS_SUCCESS, "NtOpenKey Failed: 0x%08lx\n", status);

        status = pNtNotifyChangeMultipleKeys(key, 0, NULL, events[0], NULL, NULL, &iosb, REG_NOTIFY_CHANGE_NAME, FALSE, NULL, 0, TRUE);
        ok(status == STATUS_PENDING, "NtNotifyChangeKey returned %lx\n", status);

        status = pNtWaitForSingleObject(events[0], FALSE, &timeout);
        ok(status == STATUS_TIMEOUT, "NtWaitForSingleObject returned %lx\n", status);

        attr.RootDirectory = key;
        attr.ObjectName = &str;
        pRtlCreateUnicodeStringFromAsciiz(&str, "test_subkey");
        status = pNtCreateKey(&subkey, GENERIC_ALL, &attr, 0, 0, 0, 0);
        ok(status == STATUS_SUCCESS, "NtCreateKey failed: 0x%08lx\n", status);
        pRtlFreeUnicodeString(&str);

        status = pNtWaitForSingleObject(events[0], FALSE, &timeout);
        ok(status == STATUS_SUCCESS, "NtWaitForSingleObject returned %lx\n", status);

        status = pNtDeleteKey(subkey);
        ok(status == STATUS_SUCCESS, "NtDeleteSubkey failed: %lx\n", status);
        pNtClose(subkey);
        pNtClose(key);
    }
    else
    {
        win_skip("NtNotifyChangeMultipleKeys not available\n");
    }

    pNtClose(events[0]);
    pNtClose(events[1]);
}

static void test_RtlCreateRegistryKey(void)
{
    static WCHAR empty[] = {0};
    static const WCHAR key1[] = {'\\','R','t','l','C','r','e','a','t','e','R','e','g','i','s','t','r','y','K','e','y',0};
    UNICODE_STRING str;
    SIZE_T size;
    NTSTATUS status;

    RtlDuplicateUnicodeString(1, &winetestpath, &str);
    size = str.MaximumLength + sizeof(key1)* sizeof(WCHAR) * 2;
    str.Buffer = pRtlReAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, str.Buffer, size);
    str.MaximumLength = size;
    pRtlAppendUnicodeToString(&str, key1);
    pRtlAppendUnicodeToString(&str, key1);

    /* should work */
    status = pRtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE, winetestpath.Buffer);
    ok(status == STATUS_SUCCESS, "RtlCreateRegistryKey failed: %08lx\n", status);

    status = pRtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL, winetestpath.Buffer);
    ok(status == STATUS_SUCCESS, "RtlCreateRegistryKey failed: %08lx\n", status);

    status = pRtlCreateRegistryKey(RTL_REGISTRY_USER, NULL);
    ok(status == STATUS_SUCCESS, "RtlCreateRegistryKey failed: %08lx\n", status);

    status = pRtlCreateRegistryKey(RTL_REGISTRY_USER | RTL_REGISTRY_OPTIONAL, NULL);
    ok(status == STATUS_SUCCESS, "RtlCreateRegistryKey failed: %08lx\n", status);

    status = pRtlCreateRegistryKey(RTL_REGISTRY_USER, empty);
    ok(status == STATUS_SUCCESS, "RtlCreateRegistryKey failed: %08lx\n", status);

    status = pRtlCreateRegistryKey(RTL_REGISTRY_USER | RTL_REGISTRY_OPTIONAL, empty);
    ok(status == STATUS_SUCCESS, "RtlCreateRegistryKey failed: %08lx\n", status);

    /* invalid first parameter */
    status = pRtlCreateRegistryKey(RTL_REGISTRY_USER+1, winetestpath.Buffer);
    ok(status == STATUS_INVALID_PARAMETER, "RtlCreateRegistryKey unexpected return value: %08lx, expected %08lx\n", status, STATUS_INVALID_PARAMETER);

    status = pRtlCreateRegistryKey((RTL_REGISTRY_USER+1) | RTL_REGISTRY_OPTIONAL, winetestpath.Buffer);
    ok(status == STATUS_INVALID_PARAMETER, "RtlCreateRegistryKey unexpected return value: %08lx, expected %08lx\n", status, STATUS_INVALID_PARAMETER);

    /* invalid second parameter */
    status = pRtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE, NULL);
    ok(status == STATUS_OBJECT_PATH_SYNTAX_BAD, "RtlCreateRegistryKey unexpected return value: %08lx, expected %08lx\n", status, STATUS_OBJECT_PATH_SYNTAX_BAD);

    status = pRtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL, NULL);
    ok(status == STATUS_OBJECT_PATH_SYNTAX_BAD, "RtlCreateRegistryKey unexpected return value: %08lx, expected %08lx\n", status, STATUS_OBJECT_PATH_SYNTAX_BAD);

    status = pRtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE, empty);
    ok(status == STATUS_OBJECT_PATH_SYNTAX_BAD, "RtlCreateRegistryKey unexpected return value: %08lx, expected %08lx\n", status, STATUS_OBJECT_PATH_SYNTAX_BAD);

    status = pRtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL, empty);
    ok(status == STATUS_OBJECT_PATH_SYNTAX_BAD, "RtlCreateRegistryKey unexpected return value: %08lx, expected %08lx\n", status, STATUS_OBJECT_PATH_SYNTAX_BAD);

    status = pRtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE, str.Buffer);
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND, "RtlCreateRegistryKey unexpected return value: %08lx, expected %08lx\n", status, STATUS_OBJECT_NAME_NOT_FOUND);

    status = pRtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL, str.Buffer);
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND, "RtlCreateRegistryKey unexpected return value: %08lx, expected %08lx\n", status, STATUS_OBJECT_NAME_NOT_FOUND);

    /* both parameters invalid */
    status = pRtlCreateRegistryKey(RTL_REGISTRY_USER+1, NULL);
    ok(status == STATUS_INVALID_PARAMETER, "RtlCreateRegistryKey unexpected return value: %08lx, expected %08lx\n", status, STATUS_INVALID_PARAMETER);

    status = pRtlCreateRegistryKey((RTL_REGISTRY_USER+1) | RTL_REGISTRY_OPTIONAL, NULL);
    ok(status == STATUS_INVALID_PARAMETER, "RtlCreateRegistryKey unexpected return value: %08lx, expected %08lx\n", status, STATUS_INVALID_PARAMETER);

    pRtlFreeUnicodeString(&str);
}

static void test_NtRenameKey(void)
{
    KEY_NAME_INFORMATION *info = NULL;
    UNICODE_STRING str, str2;
    OBJECT_ATTRIBUTES attr;
    HANDLE key, subkey;
    char buffer[200];
    NTSTATUS status;
    DWORD size;

    status = NtRenameKey(NULL, NULL);
    ok(status == STATUS_ACCESS_VIOLATION, "Unexpected status %#lx.\n", status);

    InitializeObjectAttributes(&attr, &winetestpath, 0, 0, 0);
    status = pNtCreateKey(&key, KEY_READ|DELETE, &attr, 0, 0, 0, 0);
    ok(!status, "Unexpected status %#lx.\n", status);

    attr.RootDirectory = key;
    attr.ObjectName = &str;

    pRtlCreateUnicodeStringFromAsciiz(&str, "rename_subkey");
    status = pNtCreateKey(&subkey, KEY_READ|DELETE, &attr, 0, 0, 0, 0);
    ok(!status, "Unexpected status %#lx.\n", status);

    memset(&str2, 0, sizeof(str2));
    status = NtRenameKey(subkey, &str2);
    ok(status == STATUS_INVALID_PARAMETER, "Unexpected status %#lx.\n", status);

    pRtlCreateUnicodeStringFromAsciiz(&str2, "renamed_subkey");

    status = NtRenameKey(subkey, NULL);
    ok(status == STATUS_ACCESS_VIOLATION, "Unexpected status %#lx.\n", status);
    status = NtRenameKey(NULL, &str);
    ok(status == STATUS_INVALID_HANDLE, "Unexpected status %#lx.\n", status);

    status = NtRenameKey(subkey, &str2);
    ok(status == STATUS_ACCESS_DENIED, "Unexpected status %#lx.\n", status);
    pNtClose(subkey);

    status = pNtCreateKey(&subkey, KEY_WRITE|DELETE, &attr, 0, 0, 0, 0);
    ok(!status, "Unexpected status %#lx.\n", status);
    /* Rename to itself. */
    status = NtRenameKey(subkey, &str);
    ok(status == STATUS_CANNOT_DELETE, "Unexpected status %#lx.\n", status);
    status = NtRenameKey(subkey, &str2);
    ok(!status, "Unexpected status %#lx.\n", status);

    pRtlFreeUnicodeString(&str2);
    pRtlFreeUnicodeString(&str);

    info = (KEY_NAME_INFORMATION *)buffer;
    status = pNtQueryKey(subkey, KeyNameInformation, info, sizeof(buffer), &size);
    ok(!status, "Unexpected status %#lx.\n", status);
    if (status == STATUS_SUCCESS)
    {
        info->Name[info->NameLength/sizeof(WCHAR)] = 0;
        ok(!!wcsstr(info->Name, L"renamed_subkey"), "Unexpected subkey name %s.\n", wine_dbgstr_w(info->Name));
    }

    status = pNtDeleteKey(subkey);
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    status = pNtDeleteKey(key);
    ok( status == STATUS_SUCCESS, "NtDeleteKey failed: 0x%08lx\n", status );
    pNtClose(subkey);
    pNtClose(key);
}

static BOOL set_privileges(LPCSTR privilege, BOOL set)
{
    TOKEN_PRIVILEGES tp;
    HANDLE hToken;
    LUID luid;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
        return FALSE;

    if(!LookupPrivilegeValueA(NULL, privilege, &luid))
    {
        CloseHandle(hToken);
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;

    if (set)
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tp.Privileges[0].Attributes = 0;

    AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
    if (GetLastError() != ERROR_SUCCESS)
    {
        CloseHandle(hToken);
        return FALSE;
    }

    CloseHandle(hToken);
    return TRUE;
}

static void test_NtRegLoadKeyEx(void)
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES file_attr, key_attr;
    WCHAR temp_path[MAX_PATH], hivefile_path[MAX_PATH];
    UNICODE_STRING hivefile_pathW, key_pathW;
    HANDLE key = 0;

#ifdef __REACTOS__
    if (_winver < _WIN32_WINNT_VISTA)
    {
        win_skip("Skipping tests for NtLoadKeyEx on pre-NT6\n");
        return;
    }
#endif

    GetTempPathW(ARRAY_SIZE(temp_path), temp_path);
    GetTempFileNameW(temp_path, L"key", 0, hivefile_path);
    DeleteFileW(hivefile_path);
    RtlDosPathNameToNtPathName_U(hivefile_path, &hivefile_pathW, NULL, NULL);

    if (!set_privileges(SE_RESTORE_NAME, TRUE) ||
        !set_privileges(SE_BACKUP_NAME, TRUE))
    {
        win_skip("Failed to set SE_RESTORE_NAME and SE_BACKUP_NAME privileges, skipping tests\n");
        RtlFreeUnicodeString(&hivefile_pathW);
        return;
    }

    /* Generate hive file */
    InitializeObjectAttributes(&key_attr, &winetestpath, 0, NULL, NULL);
    status = pNtCreateKey(&key, KEY_ALL_ACCESS, &key_attr, 0, 0, 0, 0);
    ok(status == ERROR_SUCCESS, "couldn't create key 0x%lx\n", status);
    status = RegSaveKeyW(key, hivefile_path, NULL);
    ok(status == ERROR_SUCCESS, "couldn't save key %ld\n", status);
    status = pNtDeleteKey(key);
    ok(status == ERROR_SUCCESS, "couldn't delete key 0x%lx\n", status);
    key = 0;

    /* Test for roothandle parameter with no flags */
    pRtlFormatCurrentUserKeyPath(&key_pathW);
    key_pathW.Buffer = pRtlReAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, key_pathW.Buffer,
                           key_pathW.MaximumLength + sizeof(key_pathW)*sizeof(WCHAR));
    key_pathW.MaximumLength = key_pathW.MaximumLength + sizeof(key_pathW)*sizeof(WCHAR);
    pRtlAppendUnicodeToString(&key_pathW, L"TestKey");

    InitializeObjectAttributes(&file_attr, &hivefile_pathW, 0, NULL, NULL);
    key_attr.ObjectName = &key_pathW;
    status = pNtLoadKeyEx(&key_attr, &file_attr, 0, NULL, NULL, KEY_READ, &key, NULL);
    todo_wine ok(status == STATUS_INVALID_PARAMETER_7 || broken(status == STATUS_INVALID_PARAMETER_6) /* win7 */, "got 0x%lx\n", status);
    if (status == STATUS_INVALID_PARAMETER_6)
    {
        win_skip("NtLoadKeyEx has a different order of parameters in this windows version\n");
        RtlFreeUnicodeString(&hivefile_pathW);
        RtlFreeUnicodeString(&key_pathW);
        DeleteFileW(hivefile_path);
        return;
    }
    ok(!key, "key is expected to be null\n");
    if (key) pNtClose(key);
    RtlFreeUnicodeString(&key_pathW);

    /* Test for roothandle parameter with REG_APP_HIVE */
    RtlCreateUnicodeString(&key_pathW, L"\\REGISTRY\\A\\TestKey");
    status = pNtLoadKeyEx(&key_attr, &file_attr, REG_APP_HIVE, NULL, NULL, KEY_READ, &key, NULL);
    todo_wine ok(status == STATUS_SUCCESS, "got 0x%lx\n", status);
    todo_wine ok(key != NULL, "key is null\n");
    if (key) pNtClose(key);
    RtlFreeUnicodeString(&key_pathW);

    set_privileges(SE_RESTORE_NAME, FALSE);
    set_privileges(SE_BACKUP_NAME, FALSE);
    RtlFreeUnicodeString(&hivefile_pathW);
    DeleteFileW(hivefile_path);
}

struct query_reg_values_test
{
    RTL_QUERY_REGISTRY_TABLE query_table[3];
    NTSTATUS expected_ret;
    unsigned int expected_calls;
    enum
    {
        SKIP_NAME_CHECK = 0x1,
        SKIP_DATA_CHECK = 0x2,
        EXPECT_DEFAULT_DATA = 0x4,
        SPLIT_MULTI = 0x8,
        WINE_TODO_RET = 0x10,
        WINE_TODO_CALLS = 0x20,
        WINE_TODO_NAME = 0x40,
        WINE_TODO_TYPE = 0x80,
        WINE_TODO_SIZE = 0x100,
    }
    flags;
    ULONG expected_type;
    const WCHAR *expected_data;
    ULONG expected_data_size;
    ULONG size_limit;
};

#if !defined(__REACTOS__) || (DLL_EXPORT_VERSION >= _WIN32_WINNT_WIN7)
static unsigned int query_routine_calls;

static NTSTATUS WINAPI query_routine(const WCHAR *value_name, ULONG value_type, void *value_data, ULONG value_data_size,
                                     void *context, void *entry_context)
{
    struct query_reg_values_test *test = context;
    RTL_QUERY_REGISTRY_TABLE *query = entry_context;
    const WCHAR *expected_data;
    ULONG expected_size;
    ULONG expected_type;

    trace("Value name: %s\n", debugstr_w(value_name));
    trace("Value data: %s\n", debugstr_w(value_data));

    if (!(test->flags & SKIP_NAME_CHECK))
    {
        todo_wine_if(test->flags & WINE_TODO_NAME)
        if (query->Name)
            ok(!wcscmp(value_name, query->Name), "Expected name %s, got %s\n", debugstr_w(query->Name), debugstr_w(value_name));
        else
            ok(!value_name, "Expected null name\n");
    }

    if (!(test->flags & SKIP_DATA_CHECK) && query_routine_calls < test->expected_calls)
    {
        if (test->flags & EXPECT_DEFAULT_DATA)
        {
            expected_type = query->DefaultType;
            expected_data = query->DefaultData;
            expected_size = query->DefaultLength;
        }
        else
        {
            expected_type = test->expected_type;
            expected_data = test->expected_data;
            expected_size = test->expected_data_size;
        }

        if (test->flags & SPLIT_MULTI)
        {
            expected_type = REG_SZ;
            for (int i = 0; i < query_routine_calls; i++)
                expected_data = wcschr(expected_data, '\0') + 1;
            expected_size = 0;
        }

        if (!expected_size && expected_data && (expected_type == REG_SZ || expected_type == REG_EXPAND_SZ))
            expected_size = (wcslen(expected_data) + 1) * sizeof(WCHAR);

        todo_wine_if(test->flags & WINE_TODO_TYPE)
        ok(value_type == expected_type, "Expected type %lu, got %lu\n", expected_type, value_type);

        todo_wine_if(test->flags & WINE_TODO_SIZE)
        ok(value_data_size == expected_size, "Expected size %lu, got %lu\n", expected_size, value_data_size);

        if (expected_data == query->DefaultData || expected_data == NULL)
            ok(value_data == expected_data, "Expected data %p, got %p\n", expected_data, value_data);
        else
            ok(!memcmp(value_data, expected_data, value_data_size),
               "Expected data %s, got %s\n", debugstr_w(expected_data), debugstr_w(value_data));
    }

    query_routine_calls++;

    return STATUS_SUCCESS;
}

static WCHAR query_reg_values_direct_str_buf[32];
static UNICODE_STRING query_reg_values_direct_str = {0, 0, query_reg_values_direct_str_buf};

static ULONG query_reg_values_direct_int;

static union
{
    ULONG size;
    char data[32];
}
query_reg_values_direct_sized;

static struct
{
    ULONG size;
    ULONG type;
    char data[32];
}
query_reg_values_direct_typed;

static struct query_reg_values_test query_reg_values_tests[] =
{
    /* Empty table */
    {
        {{ NULL }},
        STATUS_SUCCESS, 0
    },
    /* Name without query routine or DIRECT */
    {
        {{ NULL, 0, (WCHAR*)L"WindowsDrive" }},
        STATUS_SUCCESS, 0
    },
    {
        {{ NULL, 0, (WCHAR*)L"I don't exist", NULL, REG_SZ, (WCHAR*)L"Some default" }},
        STATUS_SUCCESS, 0
    },
    /* The query routine is called for every value in current key */
    {
        {{ query_routine }},
        STATUS_SUCCESS, 6, SKIP_NAME_CHECK | SKIP_DATA_CHECK
    },
    /* NOVALUE is ignored when the name is not null */
    {
        {{ query_routine, RTL_QUERY_REGISTRY_NOVALUE, (WCHAR*)L"WindowsDrive" }},
        STATUS_SUCCESS, 1, WINE_TODO_TYPE | WINE_TODO_SIZE, REG_SZ, L"C:"
    },
    /* NOVALUE calls the callback without enumerating any values */
    {
        {
            { query_routine, RTL_QUERY_REGISTRY_NOVALUE },
            { query_routine, RTL_QUERY_REGISTRY_NOVALUE, NULL, NULL, REG_SZ, (WCHAR*)L"Some default" },
        },
        STATUS_SUCCESS, 2
    },
    /* DIRECT doesn't call the query routine and reads directly into a buffer */
    {
        {{ query_routine, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"WindowsDrive", &query_reg_values_direct_str }},
        STATUS_INVALID_PARAMETER, 0, 0, REG_NONE, NULL, -1
    },
    {
        {{ query_routine, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"I don't exist",
           &query_reg_values_direct_str, REG_SZ, (WCHAR*)L"Some default" }},
        STATUS_INVALID_PARAMETER, 0, 0, REG_NONE, NULL, -1
    },
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"WindowsDrive", &query_reg_values_direct_str }},
        STATUS_SUCCESS, 0, 0, REG_NONE, L"C:"
    },
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"WindowsDrive", &query_reg_values_direct_str }},
        STATUS_SUCCESS, 0, 0, REG_NONE, L"\x2323", 0, 2 * sizeof(WCHAR)
    },
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_NOEXPAND, (WCHAR*)L"WindowsDrive",
           &query_reg_values_direct_str }},
        STATUS_SUCCESS, 0, 0, REG_NONE, L"%SYSTEMDRIVE%"
    },
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_NOEXPAND, (WCHAR*)L"WindowsDrive",
           &query_reg_values_direct_str }},
        STATUS_SUCCESS, 0, 0, REG_NONE, L"\x2323", 0, 2 * sizeof(WCHAR)
    },
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"MeaningOfLife32", &query_reg_values_direct_int }},
        STATUS_SUCCESS, 0, 0, REG_NONE, L"\x2a"
    },
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"MeaningOfLife64", &query_reg_values_direct_sized }},
        STATUS_SUCCESS, 0, 0, REG_NONE, L"\x2a\0\0", sizeof(UINT64)
    },
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"MeaningOfLife64", &query_reg_values_direct_sized }},
        STATUS_SUCCESS, 0, 0, REG_NONE, L"\xff", 1, 1
    },
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"MeaningOfLife64", &query_reg_values_direct_typed }},
        STATUS_SUCCESS, 0, 0, REG_QWORD, L"\x2a\0\0", sizeof(UINT64)
    },
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"MeaningOfLife64", &query_reg_values_direct_typed }},
        STATUS_SUCCESS, 0, 0, 0x23, L"\x23", 1, 1
    },
    /* DIRECT on a multi-string crashes on Windows without NOEXPAND */
    /* {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"CapitalsOfEurope", &query_reg_values_direct_str }},
        STATUS_SUCCESS, 0, WINE_TODO_RET
    }, */
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_NOEXPAND, (WCHAR*)L"CapitalsOfEurope",
           &query_reg_values_direct_str }},
        STATUS_SUCCESS, 0, 0, REG_NONE, L"Brussels\0Paris\0%PATH%\0", sizeof(L"Brussels\0Paris\0%PATH%\0")
    },
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_NOEXPAND, (WCHAR*)L"CapitalsOfEurope",
           &query_reg_values_direct_str }},
        STATUS_SUCCESS, 0, 0, REG_NONE, L"\x2323", 0, 2 * sizeof(WCHAR)
    },
    /* DIRECT with a null buffer crashes on Windows */
    /* {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"WindowsDrive", NULL }},
        STATUS_INVALID_PARAMETER
    }, */
    /* SUBKEY changes the current path on the fly */
    {
        {{ query_routine, RTL_QUERY_REGISTRY_SUBKEY, (WCHAR*)L"subkey" }},
        STATUS_SUCCESS, 1, SKIP_NAME_CHECK | WINE_TODO_RET | WINE_TODO_CALLS, REG_SZ, L"Yellow"
    },
    {
        {
            { NULL, RTL_QUERY_REGISTRY_SUBKEY, (WCHAR*)L"subkey" },
            { query_routine, 0, (WCHAR*)L"Color" },
        },
        STATUS_SUCCESS, 1, WINE_TODO_RET | WINE_TODO_CALLS, REG_SZ, L"Yellow"
    },
    /* NOEXPAND disables variable expansion */
    {
        {{ query_routine, RTL_QUERY_REGISTRY_NOEXPAND, (WCHAR*)L"WindowsDrive" }},
        STATUS_SUCCESS, 1, 0, REG_EXPAND_SZ, L"%SYSTEMDRIVE%"
    },
    /* NOEXPAND calls the query routine only once instead of once for each string in a multi-string */
    {
        {{ query_routine, 0, (WCHAR*)L"CapitalsOfEurope" }},
        STATUS_SUCCESS, 3, SPLIT_MULTI, REG_SZ, L"Brussels\0Paris\0%PATH%\0"
    },
    {
        {{ query_routine, RTL_QUERY_REGISTRY_NOEXPAND, (WCHAR*)L"CapitalsOfEurope" }},
        STATUS_SUCCESS, 1, 0, REG_MULTI_SZ, L"Brussels\0Paris\0%PATH%\0", sizeof(L"Brussels\0Paris\0%PATH%\0")
    },
    /* The default value is used if the registry value does not exist */
    {
        {{ query_routine, 0, (WCHAR*)L"I don't exist", NULL, REG_SZ, (WCHAR*)L"Some default", 4 * sizeof (WCHAR) }},
        STATUS_SUCCESS, 1, 0, REG_SZ, L"Some", 4 * sizeof(WCHAR)
    },
    {
        {{ query_routine, 0, (WCHAR*)L"I don't exist", NULL, REG_SZ, (WCHAR*)L"%SYSTEMDRIVE%" }},
        STATUS_SUCCESS, 1, EXPECT_DEFAULT_DATA
    },
    {
        {{ query_routine, 0, (WCHAR*)L"I don't exist", NULL, REG_EXPAND_SZ, (WCHAR*)L"%SYSTEMDRIVE%" }},
        STATUS_SUCCESS, 1, 0, REG_SZ, L"C:"
    },
    {
        {{ query_routine, 0, (WCHAR*)L"I don't exist", NULL, REG_MULTI_SZ, (WCHAR*)L"Brussels\0Paris\0%PATH%\0" }},
        STATUS_SUCCESS, 3, EXPECT_DEFAULT_DATA | SPLIT_MULTI
    },
    {
        {{ query_routine, 0, (WCHAR*)L"I don't exist",
           NULL, REG_MULTI_SZ, (WCHAR*)L"A\0B\0C", sizeof(L"A\0B\0C") }},
        STATUS_SUCCESS, 2, EXPECT_DEFAULT_DATA | SPLIT_MULTI
    },
    {
        {{ query_routine, 0, (WCHAR*)L"I don't exist",
           NULL, REG_MULTI_SZ, (WCHAR*)L"A\0B\0C", sizeof(L"A\0B\0C") - sizeof(L'\0') }},
        STATUS_SUCCESS, 2, EXPECT_DEFAULT_DATA | SPLIT_MULTI
    },
    {
        {{ query_routine, 0, (WCHAR*)L"I don't exist", NULL, REG_DWORD, (WCHAR*)0xdeadbeef }},
        STATUS_SUCCESS, 1, EXPECT_DEFAULT_DATA
    },
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"I don't exist",
           &query_reg_values_direct_str, REG_SZ, (WCHAR*)L"Some default", 4 * sizeof(WCHAR) }},
        STATUS_SUCCESS, 0, EXPECT_DEFAULT_DATA
    },
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"I don't exist",
           &query_reg_values_direct_str, REG_SZ, (WCHAR*)L"%SYSTEMDRIVE%" }},
        STATUS_SUCCESS, 0, EXPECT_DEFAULT_DATA
    },
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"I don't exist",
           &query_reg_values_direct_str, REG_SZ, (WCHAR*)L"%SYSTEMDRIVE%" }},
        STATUS_SUCCESS, 0, 0, REG_NONE, L"\x2323", 0, 2 * sizeof(WCHAR)
    },
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"I don't exist",
           &query_reg_values_direct_str, REG_EXPAND_SZ, (WCHAR*)L"%SYSTEMDRIVE%" }},
        STATUS_SUCCESS, 0, 0, REG_NONE, L"C:"
    },
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"I don't exist",
           &query_reg_values_direct_str, REG_EXPAND_SZ, (WCHAR*)L"%SYSTEMDRIVE%" }},
        STATUS_SUCCESS, 0, 0, REG_NONE, L"\x2323", 0, 2 * sizeof(WCHAR)
    },
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"I don't exist",
           &query_reg_values_direct_int, REG_DWORD, (WCHAR*)0xdeadbeef }},
        STATUS_SUCCESS, 0
    },
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"I don't exist",
           &query_reg_values_direct_int, REG_DWORD, (WCHAR*)L"\x2a", sizeof(DWORD) }},
        STATUS_SUCCESS, 0, EXPECT_DEFAULT_DATA
    },
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"I don't exist",
           &query_reg_values_direct_sized, REG_DWORD, (WCHAR*)L"Some default", sizeof(L"Some default") }},
        STATUS_SUCCESS, 0, EXPECT_DEFAULT_DATA
    },
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"I don't exist",
           &query_reg_values_direct_sized, REG_DWORD, (WCHAR*)L"Some default", sizeof(L"Some default") }},
        STATUS_SUCCESS, 0, 0, REG_NONE, L"\xff", 1, 1
    },
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"I don't exist",
           &query_reg_values_direct_typed, REG_NONE, (WCHAR*)L"Some default", sizeof(L"Some default") }},
        STATUS_SUCCESS, 0, WINE_TODO_TYPE | WINE_TODO_SIZE, 0x23, NULL, -1
    },
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"I don't exist",
           &query_reg_values_direct_typed, REG_QWORD, (WCHAR*)L"Some default", sizeof(L"Some default") }},
        STATUS_SUCCESS, 0, WINE_TODO_TYPE | WINE_TODO_SIZE, 0x23, NULL, -1
    },
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"I don't exist",
           &query_reg_values_direct_typed, REG_QWORD, (WCHAR*)L"\x2a\0\0", sizeof(UINT64) }},
        STATUS_SUCCESS, 0, EXPECT_DEFAULT_DATA
    },
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"I don't exist",
           &query_reg_values_direct_typed, REG_QWORD, (WCHAR*)L"\x2a\0\0", sizeof(UINT64) }},
        STATUS_SUCCESS, 0, 0, 0x23, L"\x23", 1, 1
    },
    /* DIRECT with a multi-string default value crashes on Windows without NOEXPAND */
    /* {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"I don't exist",
           &query_reg_values_direct_str, REG_MULTI_SZ, (WCHAR*)L"A\0B\0C\0", sizeof(L"A\0B\0C\0") }},
        STATUS_SUCCESS, 0, EXPECT_DEFAULT_DATA
    }, */
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_NOEXPAND, (WCHAR*)L"I don't exist",
           &query_reg_values_direct_str, REG_MULTI_SZ, (WCHAR*)L"A\0B\0C", sizeof(L"A\0B\0C") - sizeof(L'\0') }},
        STATUS_SUCCESS, 0, EXPECT_DEFAULT_DATA
    },
    /* The default value is not used if it is not valid */
    {
        {{ query_routine, 0, (WCHAR*)L"I don't exist", NULL, REG_SZ }},
        STATUS_DATA_OVERRUN, 0, EXPECT_DEFAULT_DATA
    },
    {
        {{ query_routine, 0, (WCHAR*)L"I don't exist", NULL, REG_NONE, (WCHAR*)L"Some default" }},
        STATUS_SUCCESS, 0, EXPECT_DEFAULT_DATA | WINE_TODO_CALLS | WINE_TODO_SIZE
    },
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"I don't exist",
           &query_reg_values_direct_str, REG_SZ }},
        STATUS_DATA_OVERRUN, 0, 0, REG_NONE, NULL, -1
    },
    {
        {{ NULL, RTL_QUERY_REGISTRY_DIRECT, (WCHAR*)L"I don't exist",
           &query_reg_values_direct_str, REG_NONE, (WCHAR*)L"Some default" }},
        STATUS_SUCCESS, 0, 0, REG_NONE, NULL, -1
    },
    /* REQUIRED fails if the value doesn't exist and there is no default */
    {
        {{ query_routine, RTL_QUERY_REGISTRY_REQUIRED, (WCHAR*)L"I don't exist",
           NULL, REG_SZ, (WCHAR*)L"Some default" }},
        STATUS_SUCCESS, 1, EXPECT_DEFAULT_DATA | WINE_TODO_RET | WINE_TODO_CALLS
    },
    {
        {{ query_routine, RTL_QUERY_REGISTRY_REQUIRED, (WCHAR*)L"I don't exist",
           NULL, REG_NONE, (WCHAR*)L"Some default" }},
        STATUS_OBJECT_NAME_NOT_FOUND
    },
    /* DELETE deletes the value after reading it */
    {
        {{ query_routine, RTL_QUERY_REGISTRY_DELETE, (WCHAR*)L"WindowsDrive" }},
        STATUS_SUCCESS, 1, 0, REG_SZ, L"C:"
    },
    {
        {{ query_routine, 0, (WCHAR*)L"I don't exist", NULL, REG_SZ, (WCHAR*)L"Some default" }},
        STATUS_SUCCESS, 1, EXPECT_DEFAULT_DATA
    },
};

static void test_RtlQueryRegistryValues(void)
{
    NTSTATUS status;
    unsigned int i;

    status = RegSetKeyValueW(HKEY_CURRENT_USER, L"WineTest", L"WindowsDrive", REG_EXPAND_SZ,
                             L"%SYSTEMDRIVE%", sizeof(L"%SYSTEMDRIVE%"));
    ok(status == ERROR_SUCCESS, "Failed to create registry value WindowsDrive: %lu\n", status);

    status = RegSetKeyValueW(HKEY_CURRENT_USER, L"WineTest", L"CapitalsOfEurope", REG_MULTI_SZ,
                             L"Brussels\0Paris\0%PATH%", sizeof(L"Brussels\0Paris\0%PATH%") - sizeof(L'\0'));
    ok(status == ERROR_SUCCESS, "Failed to create registry value CapitalsOfEurope: %lu\n", status);

    status = RegSetKeyValueW(HKEY_CURRENT_USER, L"WineTest", L"MeaningOfLife32", REG_DWORD,
                             L"\x2a", sizeof(DWORD));
    ok(status == ERROR_SUCCESS, "Failed to create registry value MeaningOfLife32: %lu\n", status);

    status = RegSetKeyValueW(HKEY_CURRENT_USER, L"WineTest", L"MeaningOfLife64", REG_QWORD,
                             L"\x2a\0\0", sizeof(UINT64));
    ok(status == ERROR_SUCCESS, "Failed to create registry value MeaningOfLife64: %lu\n", status);

    status = RegSetKeyValueW(HKEY_CURRENT_USER, L"WineTest\\subkey", L"Color", REG_SZ,
                             L"Yellow", sizeof(L"Yellow"));
    ok(status == ERROR_SUCCESS, "Failed to create registry value Color: %lu\n", status);

    for (i = 0; i < ARRAY_SIZE(query_reg_values_tests); i++)
    {
        struct query_reg_values_test *test = &query_reg_values_tests[i];
        RTL_QUERY_REGISTRY_TABLE *query;
        const WCHAR *expected_data;
        ULONG expected_size;
        ULONG expected_type;

        winetest_push_context("%u/%Iu", i, ARRAY_SIZE(query_reg_values_tests) - 1);

        for (query = test->query_table; query->QueryRoutine || query->Name; query++)
        {
            if (!(query->Flags & RTL_QUERY_REGISTRY_DIRECT))
                query->EntryContext = query;
        }

        query_routine_calls = 0;

        query_reg_values_direct_str.MaximumLength = test->size_limit ? test->size_limit
                                                                     : sizeof(query_reg_values_direct_str_buf);
        if (query_reg_values_direct_str.MaximumLength >= sizeof(WCHAR))
           query_reg_values_direct_str.Length = query_reg_values_direct_str.MaximumLength - sizeof(WCHAR);
        else
            query_reg_values_direct_str.Length = 0;
        memset(query_reg_values_direct_str.Buffer, 0x23, query_reg_values_direct_str.Length);
        query_reg_values_direct_str.Buffer[query_reg_values_direct_str.Length / sizeof(WCHAR)] = 0;

        query_reg_values_direct_int = 1;

        memset(query_reg_values_direct_sized.data, 0x23, sizeof(query_reg_values_direct_sized));
        query_reg_values_direct_sized.size = test->size_limit ? -test->size_limit
                                                              : -(LONG)sizeof(query_reg_values_direct_sized);

        query_reg_values_direct_typed.size = test->size_limit ? test->size_limit
                                                              : sizeof(query_reg_values_direct_typed.data);
        query_reg_values_direct_typed.type = 0x23;
        memset(query_reg_values_direct_typed.data, 0x23, sizeof(query_reg_values_direct_typed.data));

        status = pRtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, winetestpath.Buffer, test->query_table, test, NULL);

        todo_wine_if(test->flags & WINE_TODO_RET)
        ok(status == test->expected_ret, "Expected RtlQueryRegistryValues to return 0x%08lx, got 0x%08lx\n",
           test->expected_ret, status);

        todo_wine_if(test->flags & WINE_TODO_CALLS)
        ok(query_routine_calls == test->expected_calls, "Expected %u calls to QueryRoutine, got %u\n",
           test->expected_calls, query_routine_calls);

        for (query = test->query_table; query->QueryRoutine || query->Name; query++)
        {
            if ((query->Flags & RTL_QUERY_REGISTRY_DIRECT) && query->EntryContext)
            {
                if (test->flags & EXPECT_DEFAULT_DATA)
                {
                    expected_type = query->DefaultType;
                    expected_data = query->DefaultData;
                    expected_size = query->DefaultLength;
                }
                else
                {
                    expected_type = test->expected_type;
                    expected_data = test->expected_data;
                    expected_size = test->expected_data_size;
                }

                if (query->EntryContext == &query_reg_values_direct_str)
                {
                    if (!expected_size && expected_data)
                        expected_size = (wcslen(expected_data) + 1) * sizeof(WCHAR);
                    else if (expected_size == -1)
                        expected_size = query_reg_values_direct_str.MaximumLength;

                    todo_wine_if(test->flags & WINE_TODO_SIZE)
                    ok(query_reg_values_direct_str.Length + sizeof(WCHAR) == expected_size,
                       "Expected size %lu, got %Iu\n", expected_size,
                       query_reg_values_direct_str.Length + sizeof(WCHAR));

                    if (expected_data)
                    {
                        ok(!memcmp(query_reg_values_direct_str.Buffer, expected_data, expected_size),
                           "Expected data %s, got %s\n", debugstr_w(expected_data),
                           debugstr_w(query_reg_values_direct_str.Buffer));
                    }
                }
                else if (query->EntryContext == &query_reg_values_direct_int)
                {
                    if (expected_data)
                    {
                        ok(!memcmp(&query_reg_values_direct_int, expected_data, expected_size),
                           "Data does not match\n");
                    }
                    else
                    {
                        ok(query_reg_values_direct_int == 1,
                           "Expected data to not change, got %lu\n", query_reg_values_direct_int);
                    }
                }
                else if (query->EntryContext == &query_reg_values_direct_sized)
                {
                    ok(!memcmp(query_reg_values_direct_sized.data, expected_data, expected_size),
                       "Data does not match\n");
                }
                else if (query->EntryContext == &query_reg_values_direct_typed)
                {
                    if (expected_size == -1)
                        expected_size = sizeof(query_reg_values_direct_typed.data);

                    todo_wine_if(test->flags & WINE_TODO_SIZE)
                    ok(query_reg_values_direct_typed.size == expected_size,
                       "Expected size %lu, got %lu\n", expected_size, query_reg_values_direct_typed.size);

                    todo_wine_if(test->flags & WINE_TODO_TYPE)
                    ok(query_reg_values_direct_typed.type == expected_type,
                       "Expected type %lu, got %lu\n", expected_type, query_reg_values_direct_typed.type);

                    if (expected_data)
                    {
                        ok(!memcmp(query_reg_values_direct_typed.data, expected_data, expected_size),
                           "Data does not match\n");
                    }
                }
            }
        }

        winetest_pop_context();
    }

    status = RegDeleteKeyValueW(HKEY_CURRENT_USER, L"WineTest", L"WindowsDrive");
    ok(status == ERROR_FILE_NOT_FOUND, "Registry value WindowsDrive should have been deleted already\n");
}
#endif

START_TEST(reg)
{
#if !defined(__REACTOS__) || (DLL_EXPORT_VERSION >= _WIN32_WINNT_WIN7)
    LSTATUS status;
#endif

    if(!InitFunctionPtrs())
        return;

    pRtlFormatCurrentUserKeyPath(&winetestpath);
    winetestpath.MaximumLength = winetestpath.MaximumLength + sizeof(L"\\WineTest");
    winetestpath.Buffer = pRtlReAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, winetestpath.Buffer,
                                             winetestpath.MaximumLength);
    pRtlAppendUnicodeToString(&winetestpath, L"\\WineTest");

    test_NtCreateKey();
    test_NtOpenKey();
    test_NtSetValueKey();
    test_RtlCheckRegistryKey();
    test_RtlOpenCurrentUser();
    test_RtlpNtQueryValueKey();
    test_NtFlushKey();
    test_NtQueryKey();
    test_NtQueryLicenseKey();
    test_NtQueryValueKey();
    test_long_value_name();
    test_notify();
    test_RtlCreateRegistryKey();
    test_NtDeleteKey();
    test_symlinks();
    test_redirection();
    test_NtRenameKey();
    test_NtRegLoadKeyEx();
#if !defined(__REACTOS__) || (DLL_EXPORT_VERSION >= _WIN32_WINNT_WIN7)
    test_RtlQueryRegistryValues();

    status = RegDeleteTreeW(HKEY_CURRENT_USER, L"WineTest");
    ok(status == ERROR_SUCCESS, "Failed to delete the WineTest registry key: %lu\n", status);
#endif

    pRtlFreeUnicodeString(&winetestpath);

    FreeLibrary(hntdll);
}

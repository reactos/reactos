/* Unit test suite for Rtl string functions
 *
 * Copyright 2002 Robert Shearman
 * Copyright 2003 Thomas Mertes
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
 * NOTES
 * We use function pointers here as there is no import library for NTDLL on
 * windows.
 */

#include <stdlib.h>
#include <stdarg.h>

#define INITGUID

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winnls.h"
#include "guiddef.h"
#include "wine/test.h"

/* Function ptrs for ntdll calls */
static HMODULE hntdll = 0;
static NTSTATUS (WINAPI *pRtlAnsiStringToUnicodeString)(PUNICODE_STRING, PCANSI_STRING, BOOLEAN);
static NTSTATUS (WINAPI *pRtlAppendAsciizToString)(STRING *, LPCSTR);
static NTSTATUS (WINAPI *pRtlAppendStringToString)(STRING *, const STRING *);
static NTSTATUS (WINAPI *pRtlAppendUnicodeStringToString)(UNICODE_STRING *, const UNICODE_STRING *);
static NTSTATUS (WINAPI *pRtlAppendUnicodeToString)(UNICODE_STRING *, LPCWSTR);
static NTSTATUS (WINAPI *pRtlCharToInteger)(PCSZ, ULONG, int *);
static LONG     (WINAPI *pRtlCompareUnicodeString)(const UNICODE_STRING*, const UNICODE_STRING*, BOOLEAN);
static LONG     (WINAPI *pRtlCompareUnicodeStrings)(const WCHAR *,SIZE_T,const WCHAR *,SIZE_T,BOOLEAN);
static VOID     (WINAPI *pRtlCopyString)(STRING *, const STRING *);
static BOOLEAN  (WINAPI *pRtlCreateUnicodeString)(PUNICODE_STRING, LPCWSTR);
static BOOLEAN  (WINAPI *pRtlCreateUnicodeStringFromAsciiz)(PUNICODE_STRING, LPCSTR);
static NTSTATUS (WINAPI *pRtlDowncaseUnicodeString)(UNICODE_STRING *, const UNICODE_STRING *, BOOLEAN);
static NTSTATUS (WINAPI *pRtlDuplicateUnicodeString)(int, UNICODE_STRING *, UNICODE_STRING *);
static BOOLEAN  (WINAPI *pRtlEqualUnicodeString)(const UNICODE_STRING *, const UNICODE_STRING *, BOOLEAN);
static NTSTATUS (WINAPI *pRtlFindCharInUnicodeString)(int, const UNICODE_STRING *, const UNICODE_STRING *, USHORT *);
static VOID     (WINAPI *pRtlFreeAnsiString)(PSTRING);
static VOID     (WINAPI *pRtlFreeUnicodeString)(PUNICODE_STRING);
static VOID     (WINAPI *pRtlInitAnsiString)(PSTRING, LPCSTR);
static VOID     (WINAPI *pRtlInitString)(PSTRING, LPCSTR);
static VOID     (WINAPI *pRtlInitUnicodeString)(PUNICODE_STRING, LPCWSTR);
static NTSTATUS (WINAPI *pRtlInitUnicodeStringEx)(PUNICODE_STRING, LPCWSTR);
static NTSTATUS (WINAPI *pRtlIntegerToChar)(ULONG, ULONG, ULONG, PCHAR);
static NTSTATUS (WINAPI *pRtlIntegerToUnicodeString)(ULONG, ULONG, UNICODE_STRING *);
static NTSTATUS (WINAPI *pRtlMultiAppendUnicodeStringBuffer)(UNICODE_STRING *, LONG, UNICODE_STRING *);
static NTSTATUS (WINAPI *pRtlUnicodeStringToAnsiString)(STRING *, const UNICODE_STRING *, BOOLEAN);
static NTSTATUS (WINAPI *pRtlUnicodeStringToInteger)(const UNICODE_STRING *, int, int *);
static WCHAR    (WINAPI *pRtlUpcaseUnicodeChar)(WCHAR);
static NTSTATUS (WINAPI *pRtlUpcaseUnicodeString)(UNICODE_STRING *, const UNICODE_STRING *, BOOLEAN);
static CHAR     (WINAPI *pRtlUpperChar)(CHAR);
static NTSTATUS (WINAPI *pRtlUpperString)(STRING *, const STRING *);
static NTSTATUS (WINAPI *pRtlValidateUnicodeString)(LONG, UNICODE_STRING *);
static NTSTATUS (WINAPI *pRtlGUIDFromString)(const UNICODE_STRING*,GUID*);
static NTSTATUS (WINAPI *pRtlStringFromGUID)(const GUID*, UNICODE_STRING*);
static BOOLEAN (WINAPI *pRtlIsTextUnicode)(LPVOID, INT, INT *);
static NTSTATUS (WINAPI *pRtlHashUnicodeString)(PCUNICODE_STRING,BOOLEAN,ULONG,ULONG*);
static NTSTATUS (WINAPI *pRtlUnicodeToUTF8N)(CHAR *, ULONG, ULONG *, const WCHAR *, ULONG);
static NTSTATUS (WINAPI *pRtlUTF8ToUnicodeN)(WCHAR *, ULONG, ULONG *, const CHAR *, ULONG);
static NTSTATUS (WINAPI *pRtlFormatMessage)(const WCHAR*,ULONG,BOOLEAN,BOOLEAN,BOOLEAN,va_list*,LPWSTR,ULONG,ULONG*);

/*static VOID (WINAPI *pRtlFreeOemString)(PSTRING);*/
/*static VOID (WINAPI *pRtlCopyUnicodeString)(UNICODE_STRING *, const UNICODE_STRING *);*/
/*static VOID (WINAPI *pRtlEraseUnicodeString)(UNICODE_STRING *);*/
/*static LONG (WINAPI *pRtlCompareString)(const STRING *,const STRING *,BOOLEAN);*/
/*static BOOLEAN (WINAPI *pRtlEqualString)(const STRING *,const STRING *,BOOLEAN);*/
/*static BOOLEAN (WINAPI *pRtlPrefixString)(const STRING *, const STRING *, BOOLEAN);*/
/*static BOOLEAN (WINAPI *pRtlPrefixUnicodeString)(const UNICODE_STRING *, const UNICODE_STRING *, BOOLEAN);*/
/*static NTSTATUS (WINAPI *pRtlOemStringToUnicodeString)(PUNICODE_STRING, const STRING *, BOOLEAN);*/
/*static NTSTATUS (WINAPI *pRtlUnicodeStringToOemString)(STRING *, const UNICODE_STRING *, BOOLEAN);*/
/*static NTSTATUS (WINAPI *pRtlMultiByteToUnicodeN)(LPWSTR, DWORD, LPDWORD, LPCSTR, DWORD);*/
/*static NTSTATUS (WINAPI *pRtlOemToUnicodeN)(LPWSTR, DWORD, LPDWORD, LPCSTR, DWORD);*/
/*static NTSTATUS (WINAPI *pRtlUpcaseUnicodeStringToAnsiString)(STRING *, const UNICODE_STRING *, BOOLEAN);*/
/*static NTSTATUS (WINAPI *pRtlUpcaseUnicodeStringToOemString)(STRING *, const UNICODE_STRING *, BOOLEAN);*/
/*static NTSTATUS (WINAPI *pRtlUpcaseUnicodeToMultiByteN)(LPSTR, DWORD, LPDWORD, LPCWSTR, DWORD);*/
/*static NTSTATUS (WINAPI *pRtlUpcaseUnicodeToOemN)(LPSTR, DWORD, LPDWORD, LPCWSTR, DWORD);*/
/*static UINT (WINAPI *pRtlOemToUnicodeSize)(const STRING *);*/
/*static DWORD (WINAPI *pRtlAnsiStringToUnicodeSize)(const STRING *);*/


static WCHAR* AtoW( const char* p )
{
    WCHAR* buffer;
    DWORD len = MultiByteToWideChar( CP_ACP, 0, p, -1, NULL, 0 );
    buffer = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, p, -1, buffer, len );
    return buffer;
}


static void InitFunctionPtrs(void)
{
    hntdll = LoadLibraryA("ntdll.dll");
    ok(hntdll != 0, "LoadLibrary failed\n");
    if (hntdll) {
	pRtlAnsiStringToUnicodeString = (void *)GetProcAddress(hntdll, "RtlAnsiStringToUnicodeString");
	pRtlAppendAsciizToString = (void *)GetProcAddress(hntdll, "RtlAppendAsciizToString");
	pRtlAppendStringToString = (void *)GetProcAddress(hntdll, "RtlAppendStringToString");
	pRtlAppendUnicodeStringToString = (void *)GetProcAddress(hntdll, "RtlAppendUnicodeStringToString");
	pRtlAppendUnicodeToString = (void *)GetProcAddress(hntdll, "RtlAppendUnicodeToString");
	pRtlCharToInteger = (void *)GetProcAddress(hntdll, "RtlCharToInteger");
	pRtlCompareUnicodeString = (void *)GetProcAddress(hntdll, "RtlCompareUnicodeString");
	pRtlCompareUnicodeStrings = (void *)GetProcAddress(hntdll, "RtlCompareUnicodeStrings");
	pRtlCopyString = (void *)GetProcAddress(hntdll, "RtlCopyString");
	pRtlCreateUnicodeString = (void *)GetProcAddress(hntdll, "RtlCreateUnicodeString");
	pRtlCreateUnicodeStringFromAsciiz = (void *)GetProcAddress(hntdll, "RtlCreateUnicodeStringFromAsciiz");
	pRtlDowncaseUnicodeString = (void *)GetProcAddress(hntdll, "RtlDowncaseUnicodeString");
	pRtlDuplicateUnicodeString = (void *)GetProcAddress(hntdll, "RtlDuplicateUnicodeString");
	pRtlEqualUnicodeString = (void *)GetProcAddress(hntdll, "RtlEqualUnicodeString");
	pRtlFindCharInUnicodeString = (void *)GetProcAddress(hntdll, "RtlFindCharInUnicodeString");
	pRtlFreeAnsiString = (void *)GetProcAddress(hntdll, "RtlFreeAnsiString");
	pRtlFreeUnicodeString = (void *)GetProcAddress(hntdll, "RtlFreeUnicodeString");
	pRtlInitAnsiString = (void *)GetProcAddress(hntdll, "RtlInitAnsiString");
	pRtlInitString = (void *)GetProcAddress(hntdll, "RtlInitString");
	pRtlInitUnicodeString = (void *)GetProcAddress(hntdll, "RtlInitUnicodeString");
	pRtlInitUnicodeStringEx = (void *)GetProcAddress(hntdll, "RtlInitUnicodeStringEx");
	pRtlIntegerToChar = (void *)GetProcAddress(hntdll, "RtlIntegerToChar");
	pRtlIntegerToUnicodeString = (void *)GetProcAddress(hntdll, "RtlIntegerToUnicodeString");
	pRtlMultiAppendUnicodeStringBuffer = (void *)GetProcAddress(hntdll, "RtlMultiAppendUnicodeStringBuffer");
	pRtlUnicodeStringToAnsiString = (void *)GetProcAddress(hntdll, "RtlUnicodeStringToAnsiString");
	pRtlUnicodeStringToInteger = (void *)GetProcAddress(hntdll, "RtlUnicodeStringToInteger");
	pRtlUpcaseUnicodeChar = (void *)GetProcAddress(hntdll, "RtlUpcaseUnicodeChar");
	pRtlUpcaseUnicodeString = (void *)GetProcAddress(hntdll, "RtlUpcaseUnicodeString");
	pRtlUpperChar = (void *)GetProcAddress(hntdll, "RtlUpperChar");
	pRtlUpperString = (void *)GetProcAddress(hntdll, "RtlUpperString");
	pRtlValidateUnicodeString = (void *)GetProcAddress(hntdll, "RtlValidateUnicodeString");
	pRtlGUIDFromString = (void *)GetProcAddress(hntdll, "RtlGUIDFromString");
	pRtlStringFromGUID = (void *)GetProcAddress(hntdll, "RtlStringFromGUID");
	pRtlIsTextUnicode = (void *)GetProcAddress(hntdll, "RtlIsTextUnicode");
        pRtlHashUnicodeString = (void*)GetProcAddress(hntdll, "RtlHashUnicodeString");
        pRtlUnicodeToUTF8N = (void*)GetProcAddress(hntdll, "RtlUnicodeToUTF8N");
        pRtlUTF8ToUnicodeN = (void*)GetProcAddress(hntdll, "RtlUTF8ToUnicodeN");
        pRtlFormatMessage = (void*)GetProcAddress(hntdll, "RtlFormatMessage");
    }
}

static void test_RtlInitString(void)
{
    static const char teststring[] = "Some Wild String";
    STRING str;

    str.Length = 0;
    str.MaximumLength = 0;
    str.Buffer = (void *)0xdeadbeef;
    pRtlInitString(&str, teststring);
    ok(str.Length == sizeof(teststring) - sizeof(char), "Length uninitialized\n");
    ok(str.MaximumLength == sizeof(teststring), "MaximumLength uninitialized\n");
    ok(str.Buffer == teststring, "Buffer not equal to teststring\n");
    ok(strcmp(str.Buffer, "Some Wild String") == 0, "Buffer written to\n");
    pRtlInitString(&str, NULL);
    ok(str.Length == 0, "Length uninitialized\n");
    ok(str.MaximumLength == 0, "MaximumLength uninitialized\n");
    ok(str.Buffer == NULL, "Buffer not equal to NULL\n");
/*  pRtlInitString(NULL, teststring); */
}


static void test_RtlInitUnicodeString(void)
{
#define STRINGW {'S','o','m','e',' ','W','i','l','d',' ','S','t','r','i','n','g',0}
    static const WCHAR teststring[] = STRINGW;
    static const WCHAR originalstring[] = STRINGW;
#undef STRINGW
    UNICODE_STRING uni;

    uni.Length = 0;
    uni.MaximumLength = 0;
    uni.Buffer = (void *)0xdeadbeef;
    pRtlInitUnicodeString(&uni, teststring);
    ok(uni.Length == sizeof(teststring) - sizeof(WCHAR), "Length uninitialized\n");
    ok(uni.MaximumLength == sizeof(teststring), "MaximumLength uninitialized\n");
    ok(uni.Buffer == teststring, "Buffer not equal to teststring\n");
    ok(lstrcmpW(uni.Buffer, originalstring) == 0, "Buffer written to\n");
    pRtlInitUnicodeString(&uni, NULL);
    ok(uni.Length == 0, "Length uninitialized\n");
    ok(uni.MaximumLength == 0, "MaximumLength uninitialized\n");
    ok(uni.Buffer == NULL, "Buffer not equal to NULL\n");
/*  pRtlInitUnicodeString(NULL, teststring); */
}


#define TESTSTRING2_LEN 1000000
/* #define TESTSTRING2_LEN 32766 */


static void test_RtlInitUnicodeStringEx(void)
{
    static const WCHAR teststring[] = {'S','o','m','e',' ','W','i','l','d',' ','S','t','r','i','n','g',0};
    WCHAR *teststring2;
    UNICODE_STRING uni;
    NTSTATUS result;

    if (!pRtlInitUnicodeStringEx)
    {
        win_skip("RtlInitUnicodeStringEx is not available\n");
        return;
    }

    teststring2 = HeapAlloc(GetProcessHeap(), 0, (TESTSTRING2_LEN + 1) * sizeof(WCHAR));
    memset(teststring2, 'X', TESTSTRING2_LEN * sizeof(WCHAR));
    teststring2[TESTSTRING2_LEN] = '\0';

    uni.Length = 12345;
    uni.MaximumLength = 12345;
    uni.Buffer = (void *) 0xdeadbeef;
    result = pRtlInitUnicodeStringEx(&uni, teststring);
    ok(result == STATUS_SUCCESS,
       "pRtlInitUnicodeStringEx(&uni, 0) returns %lx, expected 0\n",
       result);
    ok(uni.Length == 32,
       "pRtlInitUnicodeStringEx(&uni, 0) sets Length to %u, expected %u\n",
       uni.Length, 32);
    ok(uni.MaximumLength == 34,
       "pRtlInitUnicodeStringEx(&uni, 0) sets MaximumLength to %u, expected %u\n",
       uni.MaximumLength, 34);
    ok(uni.Buffer == teststring,
       "pRtlInitUnicodeStringEx(&uni, 0) sets Buffer to %p, expected %p\n",
       uni.Buffer, teststring);

    uni.Length = 12345;
    uni.MaximumLength = 12345;
    uni.Buffer = (void *) 0xdeadbeef;
    pRtlInitUnicodeString(&uni, teststring);
    ok(uni.Length == 32,
       "pRtlInitUnicodeString(&uni, 0) sets Length to %u, expected %u\n",
       uni.Length, 32);
    ok(uni.MaximumLength == 34,
       "pRtlInitUnicodeString(&uni, 0) sets MaximumLength to %u, expected %u\n",
       uni.MaximumLength, 34);
    ok(uni.Buffer == teststring,
       "pRtlInitUnicodeString(&uni, 0) sets Buffer to %p, expected %p\n",
       uni.Buffer, teststring);

    uni.Length = 12345;
    uni.MaximumLength = 12345;
    uni.Buffer = (void *) 0xdeadbeef;
    result = pRtlInitUnicodeStringEx(&uni, teststring2);
    ok(result == STATUS_NAME_TOO_LONG,
       "pRtlInitUnicodeStringEx(&uni, 0) returns %lx, expected %lx\n",
       result, STATUS_NAME_TOO_LONG);
    ok(uni.Length == 12345 ||
       uni.Length == 0, /* win2k3 */
       "pRtlInitUnicodeStringEx(&uni, 0) sets Length to %u, expected 12345 or 0\n",
       uni.Length);
    ok(uni.MaximumLength == 12345 ||
       uni.MaximumLength == 0, /* win2k3 */
       "pRtlInitUnicodeStringEx(&uni, 0) sets MaximumLength to %u, expected 12345 or 0\n",
       uni.MaximumLength);
    ok(uni.Buffer == (void *) 0xdeadbeef ||
       uni.Buffer == teststring2, /* win2k3 */
       "pRtlInitUnicodeStringEx(&uni, 0) sets Buffer to %p, expected %x or %p\n",
       uni.Buffer, 0xdeadbeef, teststring2);

    uni.Length = 12345;
    uni.MaximumLength = 12345;
    uni.Buffer = (void *) 0xdeadbeef;
    pRtlInitUnicodeString(&uni, teststring2);
    ok(uni.Length == 33920 /* <= Win2000 */ || uni.Length == 65532 /* >= Win XP */,
       "pRtlInitUnicodeString(&uni, 0) sets Length to %u, expected %u\n",
       uni.Length, 65532);
    ok(uni.MaximumLength == 33922 /* <= Win2000 */ || uni.MaximumLength == 65534 /* >= Win XP */,
       "pRtlInitUnicodeString(&uni, 0) sets MaximumLength to %u, expected %u\n",
       uni.MaximumLength, 65534);
    ok(uni.Buffer == teststring2,
       "pRtlInitUnicodeString(&uni, 0) sets Buffer to %p, expected %p\n",
       uni.Buffer, teststring2);
    ok(memcmp(uni.Buffer, teststring2, (TESTSTRING2_LEN + 1) * sizeof(WCHAR)) == 0,
       "pRtlInitUnicodeString(&uni, 0) changes Buffer\n");

    uni.Length = 12345;
    uni.MaximumLength = 12345;
    uni.Buffer = (void *) 0xdeadbeef;
    result = pRtlInitUnicodeStringEx(&uni, 0);
    ok(result == STATUS_SUCCESS,
       "pRtlInitUnicodeStringEx(&uni, 0) returns %lx, expected 0\n",
       result);
    ok(uni.Length == 0,
       "pRtlInitUnicodeStringEx(&uni, 0) sets Length to %u, expected %u\n",
       uni.Length, 0);
    ok(uni.MaximumLength == 0,
       "pRtlInitUnicodeStringEx(&uni, 0) sets MaximumLength to %u, expected %u\n",
       uni.MaximumLength, 0);
    ok(uni.Buffer == NULL,
       "pRtlInitUnicodeStringEx(&uni, 0) sets Buffer to %p, expected %p\n",
       uni.Buffer, NULL);

    uni.Length = 12345;
    uni.MaximumLength = 12345;
    uni.Buffer = (void *) 0xdeadbeef;
    pRtlInitUnicodeString(&uni, 0);
    ok(uni.Length == 0,
       "pRtlInitUnicodeString(&uni, 0) sets Length to %u, expected %u\n",
       uni.Length, 0);
    ok(uni.MaximumLength == 0,
       "pRtlInitUnicodeString(&uni, 0) sets MaximumLength to %u, expected %u\n",
       uni.MaximumLength, 0);
    ok(uni.Buffer == NULL,
       "pRtlInitUnicodeString(&uni, 0) sets Buffer to %p, expected %p\n",
       uni.Buffer, NULL);

    HeapFree(GetProcessHeap(), 0, teststring2);
}


typedef struct {
    int add_nul;
    int source_Length;
    int source_MaximumLength;
    int source_buf_size;
    const char *source_buf;
    int dest_Length;
    int dest_MaximumLength;
    int dest_buf_size;
    const char *dest_buf;
    int res_Length;
    int res_MaximumLength;
    int res_buf_size;
    const char *res_buf;
    NTSTATUS result;
} dupl_ustr_t;

static const dupl_ustr_t dupl_ustr[] = {
    { 0, 32, 34, 34, "This is a string", 40, 42, 42, "--------------------", 32, 32, 32, "This is a string",     STATUS_SUCCESS},
    { 0, 32, 32, 32, "This is a string", 40, 42, 42, "--------------------", 32, 32, 32, "This is a string",     STATUS_SUCCESS},
    { 0, 32, 30, 34, "This is a string", 40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    { 0, 32, 34, 34, "This is a string", 40, 42, 42, NULL,                   32, 32, 32, "This is a string",     STATUS_SUCCESS},
    { 0, 32, 32, 32, "This is a string", 40, 42, 42, NULL,                   32, 32, 32, "This is a string",     STATUS_SUCCESS},
    { 0, 32, 30, 34, "This is a string", 40, 42, 42, NULL,                   40, 42,  0, NULL,                   STATUS_INVALID_PARAMETER},
    { 1, 32, 34, 34, "This is a string", 40, 42, 42, "--------------------", 32, 34, 34, "This is a string",     STATUS_SUCCESS},
    { 1, 32, 32, 32, "This is a string", 40, 42, 42, "--------------------", 32, 34, 34, "This is a string",     STATUS_SUCCESS},
    { 1, 32, 30, 34, "This is a string", 40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    { 1, 32, 34, 34, "This is a string", 40, 42, 42, NULL,                   32, 34, 34, "This is a string",     STATUS_SUCCESS},
    { 1, 32, 32, 32, "This is a string", 40, 42, 42, NULL,                   32, 34, 34, "This is a string",     STATUS_SUCCESS},
    { 1, 32, 30, 34, "This is a string", 40, 42, 42, NULL,                   40, 42,  0, NULL,                   STATUS_INVALID_PARAMETER},
    { 2, 32, 34, 34, "This is a string", 40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    { 2, 32, 32, 32, "This is a string", 40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    { 2, 32, 30, 34, "This is a string", 40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    { 2, 32, 34, 34, "This is a string", 40, 42, 42, NULL,                   40, 42,  0, NULL,                   STATUS_INVALID_PARAMETER},
    { 2, 32, 32, 32, "This is a string", 40, 42, 42, NULL,                   40, 42,  0, NULL,                   STATUS_INVALID_PARAMETER},
    { 2, 32, 30, 34, "This is a string", 40, 42, 42, NULL,                   40, 42,  0, NULL,                   STATUS_INVALID_PARAMETER},
    { 3, 32, 34, 34, "This is a string", 40, 42, 42, "--------------------", 32, 34, 34, "This is a string",     STATUS_SUCCESS},
    { 3, 32, 32, 32, "This is a string", 40, 42, 42, "--------------------", 32, 34, 34, "This is a string",     STATUS_SUCCESS},
    { 3, 32, 30, 32, "This is a string", 40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    { 3, 32, 34, 34, "This is a string", 40, 42, 42, NULL,                   32, 34, 34, "This is a string",     STATUS_SUCCESS},
    { 3, 32, 32, 32, "This is a string", 40, 42, 42, NULL,                   32, 34, 34, "This is a string",     STATUS_SUCCESS},
    { 3, 32, 30, 32, "This is a string", 40, 42, 42, NULL,                   40, 42,  0, NULL,                   STATUS_INVALID_PARAMETER},
    { 4, 32, 34, 34, "This is a string", 40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    { 5, 32, 34, 34, "This is a string", 40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    { 6, 32, 34, 34, "This is a string", 40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    { 7, 32, 34, 34, "This is a string", 40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    { 8, 32, 34, 34, "This is a string", 40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    { 9, 32, 34, 34, "This is a string", 40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    {10, 32, 34, 34, "This is a string", 40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    {11, 32, 34, 34, "This is a string", 40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    {12, 32, 34, 34, "This is a string", 40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    {13, 32, 34, 34, "This is a string", 40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    {14, 32, 34, 34, "This is a string", 40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    {15, 32, 34, 34, "This is a string", 40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    {16, 32, 34, 34, "This is a string", 40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    {-1, 32, 34, 34, "This is a string", 40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    {-5, 32, 34, 34, "This is a string", 40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    {-9, 32, 34, 34, "This is a string", 40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    { 0,  0,  2,  2, "",                 40, 42, 42, "--------------------",  0,  0,  0, NULL,                   STATUS_SUCCESS},
    { 0,  0,  0,  0, "",                 40, 42, 42, "--------------------",  0,  0,  0, NULL,                   STATUS_SUCCESS},
    { 0,  0,  2,  2, "",                 40, 42, 42, NULL,                    0,  0,  0, NULL,                   STATUS_SUCCESS},
    { 0,  0,  0,  0, "",                 40, 42, 42, NULL,                    0,  0,  0, NULL,                   STATUS_SUCCESS},
    { 0,  0,  2,  2, NULL,               40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    { 0,  0,  0,  0, NULL,               40, 42, 42, "--------------------",  0,  0,  0, NULL,                   STATUS_SUCCESS},
    { 0,  0,  2,  2, NULL,               40, 42, 42, NULL,                   40, 42,  0, NULL,                   STATUS_INVALID_PARAMETER},
    { 0,  0,  0,  0, NULL,               40, 42, 42, NULL,                    0,  0,  0, NULL,                   STATUS_SUCCESS},
    { 1,  0,  2,  2, "",                 40, 42, 42, "--------------------",  0,  0,  0, NULL,                   STATUS_SUCCESS},
    { 1,  0,  0,  0, "",                 40, 42, 42, "--------------------",  0,  0,  0, NULL,                   STATUS_SUCCESS},
    { 1,  0,  2,  2, "",                 40, 42, 42, NULL,                    0,  0,  0, NULL,                   STATUS_SUCCESS},
    { 1,  0,  0,  0, "",                 40, 42, 42, NULL,                    0,  0,  0, NULL,                   STATUS_SUCCESS},
    { 1,  0,  2,  2, NULL,               40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    { 1,  0,  0,  0, NULL,               40, 42, 42, "--------------------",  0,  0,  0, NULL,                   STATUS_SUCCESS},
    { 1,  0,  2,  2, NULL,               40, 42, 42, NULL,                   40, 42,  0, NULL,                   STATUS_INVALID_PARAMETER},
    { 1,  0,  0,  0, NULL,               40, 42, 42, NULL,                    0,  0,  0, NULL,                   STATUS_SUCCESS},
    { 2,  0,  2,  2, "",                 40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    { 2,  0,  0,  0, "",                 40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    { 2,  0,  2,  2, "",                 40, 42, 42, NULL,                   40, 42,  0, NULL,                   STATUS_INVALID_PARAMETER},
    { 2,  0,  0,  0, "",                 40, 42, 42, NULL,                   40, 42,  0, NULL,                   STATUS_INVALID_PARAMETER},
    { 2,  0,  2,  2, NULL,               40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    { 2,  0,  0,  0, NULL,               40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    { 2,  0,  2,  2, NULL,               40, 42, 42, NULL,                   40, 42,  0, NULL,                   STATUS_INVALID_PARAMETER},
    { 2,  0,  0,  0, NULL,               40, 42, 42, NULL,                   40, 42,  0, NULL,                   STATUS_INVALID_PARAMETER},
    { 3,  0,  2,  2, "",                 40, 42, 42, "--------------------",  0,  2,  2, "",                     STATUS_SUCCESS},
    { 3,  0,  0,  0, "",                 40, 42, 42, "--------------------",  0,  2,  2, "",                     STATUS_SUCCESS},
    { 3,  0,  2,  2, "",                 40, 42, 42, NULL,                    0,  2,  2, "",                     STATUS_SUCCESS},
    { 3,  0,  0,  0, "",                 40, 42, 42, NULL,                    0,  2,  2, "",                     STATUS_SUCCESS},
    { 3,  0,  2,  2, NULL,               40, 42, 42, "--------------------", 40, 42, 42, "--------------------", STATUS_INVALID_PARAMETER},
    { 3,  0,  0,  0, NULL,               40, 42, 42, "--------------------",  0,  2,  2, "",                     STATUS_SUCCESS},
    { 3,  0,  2,  2, NULL,               40, 42, 42, NULL,                   40, 42,  0, NULL,                   STATUS_INVALID_PARAMETER},
    { 3,  0,  0,  0, NULL,               40, 42, 42, NULL,                    0,  2,  2, "",                     STATUS_SUCCESS},
};


static void test_RtlDuplicateUnicodeString(void)
{
    size_t pos;
    WCHAR source_buf[257];
    WCHAR dest_buf[257];
    WCHAR res_buf[257];
    UNICODE_STRING source_str;
    UNICODE_STRING dest_str;
    UNICODE_STRING res_str;
    CHAR dest_ansi_buf[257];
    STRING dest_ansi_str;
    NTSTATUS result;
    unsigned int test_num;

    if (!pRtlDuplicateUnicodeString)
    {
        win_skip("RtlDuplicateUnicodeString is not available\n");
        return;
    }

    for (test_num = 0; test_num < ARRAY_SIZE(dupl_ustr); test_num++) {
	source_str.Length        = dupl_ustr[test_num].source_Length;
	source_str.MaximumLength = dupl_ustr[test_num].source_MaximumLength;
	if (dupl_ustr[test_num].source_buf != NULL) {
	    for (pos = 0; pos < dupl_ustr[test_num].source_buf_size/sizeof(WCHAR); pos++) {
		source_buf[pos] = dupl_ustr[test_num].source_buf[pos];
	    }
	    source_str.Buffer = source_buf;
	} else {
	    source_str.Buffer = NULL;
	}
	dest_str.Length        = dupl_ustr[test_num].dest_Length;
	dest_str.MaximumLength = dupl_ustr[test_num].dest_MaximumLength;
	if (dupl_ustr[test_num].dest_buf != NULL) {
	    for (pos = 0; pos < dupl_ustr[test_num].dest_buf_size/sizeof(WCHAR); pos++) {
		dest_buf[pos] = dupl_ustr[test_num].dest_buf[pos];
	    }
	    dest_str.Buffer = dest_buf;
	} else {
	    dest_str.Buffer = NULL;
	}
	res_str.Length        = dupl_ustr[test_num].res_Length;
	res_str.MaximumLength = dupl_ustr[test_num].res_MaximumLength;
	if (dupl_ustr[test_num].res_buf != NULL) {
	    for (pos = 0; pos < dupl_ustr[test_num].res_buf_size/sizeof(WCHAR); pos++) {
		res_buf[pos] = dupl_ustr[test_num].res_buf[pos];
	    }
	    res_str.Buffer = res_buf;
	} else {
	    res_str.Buffer = NULL;
	}
	result = pRtlDuplicateUnicodeString(dupl_ustr[test_num].add_nul, &source_str, &dest_str);
        dest_ansi_str.Length = dest_str.Length / sizeof(WCHAR);
        dest_ansi_str.MaximumLength = dest_ansi_str.Length + 1;
        for (pos = 0; pos < dest_ansi_str.Length; pos++) {
       	    dest_ansi_buf[pos] = (char)dest_buf[pos];
        }
        dest_ansi_buf[dest_ansi_str.Length] = '\0';
        dest_ansi_str.Buffer = dest_ansi_buf;
	ok(result == dupl_ustr[test_num].result,
           "(test %d): RtlDuplicateUnicodeString(%d, source, dest) has result %lx, expected %lx\n",
	   test_num, dupl_ustr[test_num].add_nul, result, dupl_ustr[test_num].result);
	ok(dest_str.Length == dupl_ustr[test_num].res_Length,
	   "(test %d): RtlDuplicateUnicodeString(%d, source, dest) destination has Length %d, expected %d\n",
	   test_num, dupl_ustr[test_num].add_nul, dest_str.Length, dupl_ustr[test_num].res_Length);
	ok(dest_str.MaximumLength == dupl_ustr[test_num].res_MaximumLength,
	   "(test %d): RtlDuplicateUnicodeString(%d, source, dest) destination has MaximumLength %d, expected %d\n",
	   test_num, dupl_ustr[test_num].add_nul, dest_str.MaximumLength, dupl_ustr[test_num].res_MaximumLength);
        if (result == STATUS_INVALID_PARAMETER) {
	    ok((dest_str.Buffer == NULL && res_str.Buffer == NULL) ||
               dest_str.Buffer == dest_buf,
	       "(test %d): RtlDuplicateUnicodeString(%d, source, dest) destination buffer changed %p expected %p\n",
	       test_num, dupl_ustr[test_num].add_nul, dest_str.Buffer, dest_buf);
        } else {
	    ok(dest_str.Buffer != dest_buf,
	       "(test %d): RtlDuplicateUnicodeString(%d, source, dest) has destination buffer unchanged %p\n",
	       test_num, dupl_ustr[test_num].add_nul, dest_str.Buffer);
        }
        if (dest_str.Buffer != NULL && dupl_ustr[test_num].res_buf != NULL) {
	    ok(memcmp(dest_str.Buffer, res_str.Buffer, dupl_ustr[test_num].res_buf_size) == 0,
	       "(test %d): RtlDuplicateUnicodeString(%d, source, dest) has destination \"%s\" expected \"%s\"\n",
	       test_num, dupl_ustr[test_num].add_nul, dest_ansi_str.Buffer, dupl_ustr[test_num].res_buf);
            if(result == STATUS_SUCCESS) pRtlFreeUnicodeString(&dest_str);
        } else {
	    ok(dest_str.Buffer == NULL && dupl_ustr[test_num].res_buf == NULL,
	       "(test %d): RtlDuplicateUnicodeString(%d, source, dest) has destination %p expected %p\n",
	       test_num, dupl_ustr[test_num].add_nul, dest_str.Buffer, dupl_ustr[test_num].res_buf);
        }
    }
}


static void test_RtlCopyString(void)
{
    static const char teststring[] = "Some Wild String";
    char deststring[] = "                    ";
    STRING str;
    STRING deststr;

    pRtlInitString(&str, teststring);
    pRtlInitString(&deststr, deststring);
    pRtlCopyString(&deststr, &str);
    ok(strncmp(str.Buffer, deststring, str.Length) == 0, "String not copied\n");
}


static void test_RtlUpperChar(void)
{
    int ch;
    int upper_ch;
    int expected_upper_ch;
    int byte_ch;

    for (ch = -1; ch <= 1024; ch++) {
	upper_ch = pRtlUpperChar(ch);
	byte_ch = ch & 0xff;
	if (byte_ch >= 'a' && byte_ch <= 'z') {
	    expected_upper_ch = (CHAR) (byte_ch - 'a' + 'A');
	} else {
	    expected_upper_ch = (CHAR) byte_ch;
	}
	ok(upper_ch == expected_upper_ch,
	   "RtlUpperChar('%c'[=0x%x]) has result '%c'[=0x%x], expected '%c'[=0x%x]\n",
	   ch, ch, upper_ch, upper_ch, expected_upper_ch, expected_upper_ch);
    }
}


static void test_RtlUpperString(void)
{
    int i;
    CHAR ch;
    CHAR upper_ch;
    char ascii_buf[257];
    char result_buf[257];
    char upper_buf[257];
    STRING ascii_str;
    STRING result_str;
    STRING upper_str;

    for (i = 0; i <= 255; i++) {
	ch = (CHAR) i;
	if (ch >= 'a' && ch <= 'z') {
	    upper_ch = ch - 'a' + 'A';
	} else {
	    upper_ch = ch;
	}
	ascii_buf[i] = ch;
	result_buf[i] = '\0';
	upper_buf[i] = upper_ch;
    }
    ascii_buf[i] = '\0';
    result_buf[i] = '\0';
    upper_buf[i] = '\0';
    ascii_str.Length = 256;
    ascii_str.MaximumLength = 256;
    ascii_str.Buffer = ascii_buf;
    result_str.Length = 256;
    result_str.MaximumLength = 256;
    result_str.Buffer = result_buf;
    upper_str.Length = 256;
    upper_str.MaximumLength = 256;
    upper_str.Buffer = upper_buf;

    pRtlUpperString(&result_str, &ascii_str);
    ok(memcmp(result_str.Buffer, upper_str.Buffer, 256) == 0,
       "RtlUpperString does not work as expected\n");
}


static void test_RtlUpcaseUnicodeChar(void)
{
    int i;
    WCHAR ch;
    WCHAR upper_ch;
    WCHAR expected_upper_ch;

    for (i = 0; i <= 255; i++) {
	ch = (WCHAR) i;
	upper_ch = pRtlUpcaseUnicodeChar(ch);
	if (ch >= 'a' && ch <= 'z') {
	    expected_upper_ch = ch - 'a' + 'A';
	} else if (ch >= 0xe0 && ch <= 0xfe && ch != 0xf7) {
	    expected_upper_ch = ch - 0x20;
	} else if (ch == 0xff) {
	    expected_upper_ch = 0x178;
	} else {
	    expected_upper_ch = ch;
	}
	ok(upper_ch == expected_upper_ch,
	   "RtlUpcaseUnicodeChar('%c'[=0x%x]) has result '%c'[=0x%x], expected: '%c'[=0x%x]\n",
	   ch, ch, upper_ch, upper_ch, expected_upper_ch, expected_upper_ch);
    }
}


static void test_RtlUpcaseUnicodeString(void)
{
    int i, j;
    WCHAR ch;
    WCHAR upper_ch;
    WCHAR ascii_buf[257];
    WCHAR result_buf[257];
    WCHAR upper_buf[257];
    UNICODE_STRING ascii_str;
    UNICODE_STRING result_str;
    UNICODE_STRING upper_str;

    for (i = 0; i <= 255; i++) {
	ch = (WCHAR) i;
	if (ch >= 'a' && ch <= 'z') {
	    upper_ch = ch - 'a' + 'A';
	} else if (ch >= 0xe0 && ch <= 0xfe && ch != 0xf7) {
	    upper_ch = ch - 0x20;
	} else if (ch == 0xff) {
	    upper_ch = 0x178;
	} else {
	    upper_ch = ch;
	}
	ascii_buf[i] = ch;
	result_buf[i] = '\0';
	upper_buf[i] = upper_ch;
    }
    ascii_buf[i] = '\0';
    result_buf[i] = '\0';
    upper_buf[i] = '\0';
    ascii_str.Length = 512;
    ascii_str.MaximumLength = 512;
    ascii_str.Buffer = ascii_buf;
    result_str.Length = 512;
    result_str.MaximumLength = 512;
    result_str.Buffer = result_buf;
    upper_str.Length = 512;
    upper_str.MaximumLength = 512;
    upper_str.Buffer = upper_buf;

    pRtlUpcaseUnicodeString(&result_str, &ascii_str, 0);
    for (i = 0; i <= 255; i++) {
	ok(result_str.Buffer[i] == upper_str.Buffer[i],
	   "RtlUpcaseUnicodeString works wrong: '%c'[=0x%x] is converted to '%c'[=0x%x], expected: '%c'[=0x%x]\n",
	   ascii_str.Buffer[i], ascii_str.Buffer[i],
	   result_str.Buffer[i], result_str.Buffer[i],
	   upper_str.Buffer[i], upper_str.Buffer[i]);
    }

    /* test surrogates */
    for (i = 0x100; i < 0x1100; i++)
    {
        WCHAR src[512], dst[512];
        for (j = 0; j < 256; j++)
        {
            unsigned int ch = ((i << 8) + j) - 0x10000;
            src[2 * j] = 0xd800 | (ch >> 10);
            src[2 * j + 1] = 0xdc00 | (ch & 0x3ff);
        }
        upper_str.Length = upper_str.MaximumLength = 512 * sizeof(WCHAR);
        upper_str.Buffer = src;
        result_str.Length = result_str.MaximumLength = 512 * sizeof(WCHAR);
        result_str.Buffer = dst;
        pRtlUpcaseUnicodeString(&result_str, &upper_str, 0);
        ok( !memcmp(src, dst, sizeof(dst)),
            "string compare mismatch in %04x-%04x\n", i << 8, (i << 8) + 255 );
    }
}


static void test_RtlDowncaseUnicodeString(void)
{
    int i;
    WCHAR ch;
    WCHAR lower_ch;
    WCHAR source_buf[1025];
    WCHAR result_buf[1025];
    WCHAR lower_buf[1025];
    UNICODE_STRING source_str;
    UNICODE_STRING result_str;
    UNICODE_STRING lower_str;

    for (i = 0; i < 1024; i++) {
	ch = (WCHAR) i;
	if (ch >= 'A' && ch <= 'Z') {
	    lower_ch = ch - 'A' + 'a';
	} else if (ch >= 0xc0 && ch <= 0xde && ch != 0xd7) {
	    lower_ch = ch + 0x20;
	} else if (ch >= 0x391 && ch <= 0x3ab && ch != 0x3a2) {
	    lower_ch = ch + 0x20;
	} else {
	    switch (ch) {
		case 0x178: lower_ch = 0xff; break;
		case 0x181: lower_ch = 0x253; break;
		case 0x186: lower_ch = 0x254; break;
		case 0x189: lower_ch = 0x256; break;
		case 0x18a: lower_ch = 0x257; break;
		case 0x18e: lower_ch = 0x1dd; break;
		case 0x18f: lower_ch = 0x259; break;
		case 0x190: lower_ch = 0x25b; break;
		case 0x193: lower_ch = 0x260; break;
		case 0x194: lower_ch = 0x263; break;
		case 0x196: lower_ch = 0x269; break;
		case 0x197: lower_ch = 0x268; break;
		case 0x19c: lower_ch = 0x26f; break;
		case 0x19d: lower_ch = 0x272; break;
		case 0x19f: lower_ch = 0x275; break;
		case 0x1a9: lower_ch = 0x283; break;
		case 0x1a6: lower_ch = 0x280; break;
		case 0x1ae: lower_ch = 0x288; break;
		case 0x1b1: lower_ch = 0x28a; break;
		case 0x1b2: lower_ch = 0x28b; break;
		case 0x1b7: lower_ch = 0x292; break;
		case 0x1c4: lower_ch = 0x1c6; break;
		case 0x1c7: lower_ch = 0x1c9; break;
		case 0x1ca: lower_ch = 0x1cc; break;
		case 0x1f1: lower_ch = 0x1f3; break;
		case 0x1f6: lower_ch = 0x195; break;
		case 0x1f7: lower_ch = 0x1bf; break;
		case 0x220: lower_ch = 0x19e; break;
		case 0x23a: lower_ch = 0x2c65; break;
		case 0x23d: lower_ch = 0x19a; break;
		case 0x23e: lower_ch = 0x2c66; break;
		case 0x243: lower_ch = 0x180; break;
		case 0x244: lower_ch = 0x289; break;
		case 0x245: lower_ch = 0x28c; break;
		case 0x37f: lower_ch = 0x3f3; break;
		case 0x386: lower_ch = 0x3ac; break;
		case 0x388: lower_ch = 0x3ad; break;
		case 0x389: lower_ch = 0x3ae; break;
		case 0x38a: lower_ch = 0x3af; break;
		case 0x38c: lower_ch = 0x3cc; break;
		case 0x38e: lower_ch = 0x3cd; break;
		case 0x38f: lower_ch = 0x3ce; break;
		case 0x3cf: lower_ch = 0x3d7; break;
		case 0x3f9: lower_ch = 0x3f2; break;
		case 0x3fd: lower_ch = 0x37b; break;
		case 0x3fe: lower_ch = 0x37c; break;
		case 0x3ff: lower_ch = 0x37d; break;
		default: lower_ch = ch; break;
	    } /* switch */
	}
	source_buf[i] = ch;
	result_buf[i] = '\0';
	lower_buf[i] = lower_ch;
    }
    source_buf[i] = '\0';
    result_buf[i] = '\0';
    lower_buf[i] = '\0';
    source_str.Length = 2048;
    source_str.MaximumLength = 2048;
    source_str.Buffer = source_buf;
    result_str.Length = 2048;
    result_str.MaximumLength = 2048;
    result_str.Buffer = result_buf;
    lower_str.Length = 2048;
    lower_str.MaximumLength = 2048;
    lower_str.Buffer = lower_buf;

    pRtlDowncaseUnicodeString(&result_str, &source_str, 0);
    for (i = 0; i <= 1024; i++) {
	ok(result_str.Buffer[i] == lower_str.Buffer[i] || result_str.Buffer[i] == source_str.Buffer[i] + 1 ||
           broken( result_str.Buffer[i] == source_str.Buffer[i] ),
	   "RtlDowncaseUnicodeString works wrong: '%c'[=0x%x] is converted to '%c'[=0x%x], expected: '%c'[=0x%x]\n",
	   source_str.Buffer[i], source_str.Buffer[i],
	   result_str.Buffer[i], result_str.Buffer[i],
	   lower_str.Buffer[i], lower_str.Buffer[i]);
    }
}


typedef struct {
    int ansi_Length;
    int ansi_MaximumLength;
    int ansi_buf_size;
    const char *ansi_buf;
    int uni_Length;
    int uni_MaximumLength;
    int uni_buf_size;
    const char *uni_buf;
    BOOLEAN doalloc;
    int res_Length;
    int res_MaximumLength;
    int res_buf_size;
    const char *res_buf;
    NTSTATUS result;
    int broken_len;
} ustr2astr_t;

static const ustr2astr_t ustr2astr[] = {
    { 10, 12, 12, "------------",  0,  0,  0, "",       TRUE,  0, 1, 1, "",       STATUS_SUCCESS},
    { 10, 12, 12, "------------", 12, 12, 12, "abcdef", TRUE,  6, 7, 7, "abcdef", STATUS_SUCCESS},
    {  0,  2, 12, "------------", 12, 12, 12, "abcdef", TRUE,  6, 7, 7, "abcdef", STATUS_SUCCESS},
    { 10, 12, 12, NULL,           12, 12, 12, "abcdef", TRUE,  6, 7, 7, "abcdef", STATUS_SUCCESS},
    {  0,  0, 12, "------------", 12, 12, 12, "abcdef", FALSE, 6, 0, 0, "",       STATUS_BUFFER_OVERFLOW, 1},
    {  0,  1, 12, "------------", 12, 12, 12, "abcdef", FALSE, 0, 1, 1, "",       STATUS_BUFFER_OVERFLOW},
    {  0,  2, 12, "------------", 12, 12, 12, "abcdef", FALSE, 1, 2, 2, "a",      STATUS_BUFFER_OVERFLOW},
    {  0,  3, 12, "------------", 12, 12, 12, "abcdef", FALSE, 2, 3, 3, "ab",     STATUS_BUFFER_OVERFLOW},
    {  0,  5, 12, "------------", 12, 12, 12, "abcdef", FALSE, 4, 5, 5, "abcd",   STATUS_BUFFER_OVERFLOW},
    {  8,  5, 12, "------------", 12, 12, 12, "abcdef", FALSE, 4, 5, 5, "abcd",   STATUS_BUFFER_OVERFLOW},
    {  8,  6, 12, "------------", 12, 12, 12, "abcdef", FALSE, 5, 6, 6, "abcde",  STATUS_BUFFER_OVERFLOW},
    {  8,  7, 12, "------------", 12, 12, 12, "abcdef", FALSE, 6, 7, 7, "abcdef", STATUS_SUCCESS},
    {  8,  7, 12, "------------",  0, 12, 12,  NULL,    FALSE, 0, 7, 0, "",       STATUS_SUCCESS},
#if 0
    /* crashes on Japanese and Chinese XP */
    {  0,  0, 12, NULL,           10, 10, 12,  NULL,    FALSE, 5, 0, 0, NULL,     STATUS_BUFFER_OVERFLOW},
#endif
};


static void test_RtlUnicodeStringToAnsiString(void)
{
    size_t pos;
    CHAR ansi_buf[257];
    WCHAR uni_buf[257];
    STRING ansi_str;
    UNICODE_STRING uni_str;
    NTSTATUS result;
    unsigned int test_num;

    for (test_num = 0; test_num < ARRAY_SIZE(ustr2astr); test_num++) {
	ansi_str.Length        = ustr2astr[test_num].ansi_Length;
	ansi_str.MaximumLength = ustr2astr[test_num].ansi_MaximumLength;
	if (ustr2astr[test_num].ansi_buf != NULL) {
	    memcpy(ansi_buf, ustr2astr[test_num].ansi_buf, ustr2astr[test_num].ansi_buf_size);
	    ansi_buf[ustr2astr[test_num].ansi_buf_size] = '\0';
	    ansi_str.Buffer = ansi_buf;
	} else {
	    ansi_str.Buffer = NULL;
	}
	uni_str.Length        = ustr2astr[test_num].uni_Length;
	uni_str.MaximumLength = ustr2astr[test_num].uni_MaximumLength;
	if (ustr2astr[test_num].uni_buf != NULL) {
	    for (pos = 0; pos < ustr2astr[test_num].uni_buf_size/sizeof(WCHAR); pos++) {
		uni_buf[pos] = ustr2astr[test_num].uni_buf[pos];
	    }
	    uni_str.Buffer = uni_buf;
	} else {
	    uni_str.Buffer = NULL;
	}
	result = pRtlUnicodeStringToAnsiString(&ansi_str, &uni_str, ustr2astr[test_num].doalloc);
	ok(result == ustr2astr[test_num].result,
           "(test %d): RtlUnicodeStringToAnsiString(ansi, uni, %d) has result %lx, expected %lx\n",
	   test_num, ustr2astr[test_num].doalloc, result, ustr2astr[test_num].result);
	ok(ansi_str.Length == ustr2astr[test_num].res_Length ||
       broken(ustr2astr[test_num].broken_len && !ansi_str.Length) /* win11 */,
	   "(test %d): RtlUnicodeStringToAnsiString(ansi, uni, %d) ansi has Length %d, expected %d\n",
	   test_num, ustr2astr[test_num].doalloc, ansi_str.Length, ustr2astr[test_num].res_Length);
	ok(ansi_str.MaximumLength == ustr2astr[test_num].res_MaximumLength,
	   "(test %d): RtlUnicodeStringToAnsiString(ansi, uni, %d) ansi has MaximumLength %d, expected %d\n",
	   test_num, ustr2astr[test_num].doalloc, ansi_str.MaximumLength, ustr2astr[test_num].res_MaximumLength);
	ok(memcmp(ansi_str.Buffer, ustr2astr[test_num].res_buf, ustr2astr[test_num].res_buf_size) == 0,
	   "(test %d): RtlUnicodeStringToAnsiString(ansi, uni, %d) has ansi \"%s\" expected \"%s\"\n",
	   test_num, ustr2astr[test_num].doalloc, ansi_str.Buffer, ustr2astr[test_num].res_buf);
        if(result == STATUS_SUCCESS && ustr2astr[test_num].doalloc)
            pRtlFreeAnsiString(&ansi_str);
    }
}


typedef struct {
    int dest_Length;
    int dest_MaximumLength;
    int dest_buf_size;
    const char *dest_buf;
    const char *src;
    int res_Length;
    int res_MaximumLength;
    int res_buf_size;
    const char *res_buf;
    NTSTATUS result;
} app_asc2str_t;

static const app_asc2str_t app_asc2str[] = {
    { 5, 12, 15,  "TestS01234abcde", "tring", 10, 12, 15,  "TestStringabcde", STATUS_SUCCESS},
    { 5, 11, 15,  "TestS01234abcde", "tring", 10, 11, 15,  "TestStringabcde", STATUS_SUCCESS},
    { 5, 10, 15,  "TestS01234abcde", "tring", 10, 10, 15,  "TestStringabcde", STATUS_SUCCESS},
    { 5,  9, 15,  "TestS01234abcde", "tring",  5,  9, 15,  "TestS01234abcde", STATUS_BUFFER_TOO_SMALL},
    { 5,  0, 15,  "TestS01234abcde", "tring",  5,  0, 15,  "TestS01234abcde", STATUS_BUFFER_TOO_SMALL},
    { 5, 14, 15,  "TestS01234abcde", "tring", 10, 14, 15,  "TestStringabcde", STATUS_SUCCESS},
    { 5, 14, 15,  "TestS01234abcde",    NULL,  5, 14, 15,  "TestS01234abcde", STATUS_SUCCESS},
    { 5, 14, 15,               NULL,    NULL,  5, 14, 15,               NULL, STATUS_SUCCESS},
    { 5, 12, 15, "Tst\0S01234abcde", "tr\0i",  7, 12, 15, "Tst\0Str234abcde", STATUS_SUCCESS},
};


static void test_RtlAppendAsciizToString(void)
{
    CHAR dest_buf[257];
    STRING dest_str;
    NTSTATUS result;
    unsigned int test_num;

    for (test_num = 0; test_num < ARRAY_SIZE(app_asc2str); test_num++) {
	dest_str.Length        = app_asc2str[test_num].dest_Length;
	dest_str.MaximumLength = app_asc2str[test_num].dest_MaximumLength;
	if (app_asc2str[test_num].dest_buf != NULL) {
	    memcpy(dest_buf, app_asc2str[test_num].dest_buf, app_asc2str[test_num].dest_buf_size);
	    dest_buf[app_asc2str[test_num].dest_buf_size] = '\0';
	    dest_str.Buffer = dest_buf;
	} else {
	    dest_str.Buffer = NULL;
	}
	result = pRtlAppendAsciizToString(&dest_str, app_asc2str[test_num].src);
	ok(result == app_asc2str[test_num].result,
           "(test %d): RtlAppendAsciizToString(dest, src) has result %lx, expected %lx\n",
	   test_num, result, app_asc2str[test_num].result);
	ok(dest_str.Length == app_asc2str[test_num].res_Length,
	   "(test %d): RtlAppendAsciizToString(dest, src) dest has Length %d, expected %d\n",
	   test_num, dest_str.Length, app_asc2str[test_num].res_Length);
	ok(dest_str.MaximumLength == app_asc2str[test_num].res_MaximumLength,
	   "(test %d): RtlAppendAsciizToString(dest, src) dest has MaximumLength %d, expected %d\n",
	   test_num, dest_str.MaximumLength, app_asc2str[test_num].res_MaximumLength);
	if (dest_str.Buffer == dest_buf) {
	    ok(memcmp(dest_buf, app_asc2str[test_num].res_buf, app_asc2str[test_num].res_buf_size) == 0,
	       "(test %d): RtlAppendAsciizToString(dest, src) has dest \"%s\" expected \"%s\"\n",
	       test_num, dest_buf, app_asc2str[test_num].res_buf);
	} else {
	    ok(dest_str.Buffer == app_asc2str[test_num].res_buf,
	       "(test %d): RtlAppendAsciizToString(dest, src) dest has Buffer %p expected %p\n",
	       test_num, dest_str.Buffer, app_asc2str[test_num].res_buf);
	}
    }
}


typedef struct {
    int dest_Length;
    int dest_MaximumLength;
    int dest_buf_size;
    const char *dest_buf;
    int src_Length;
    int src_MaximumLength;
    int src_buf_size;
    const char *src_buf;
    int res_Length;
    int res_MaximumLength;
    int res_buf_size;
    const char *res_buf;
    NTSTATUS result;
} app_str2str_t;

static const app_str2str_t app_str2str[] = {
    { 5, 12, 15,  "TestS01234abcde", 5, 5, 7, "tringZY", 10, 12, 15,   "TestStringabcde", STATUS_SUCCESS},
    { 5, 11, 15,  "TestS01234abcde", 5, 5, 7, "tringZY", 10, 11, 15,   "TestStringabcde", STATUS_SUCCESS},
    { 5, 10, 15,  "TestS01234abcde", 5, 5, 7, "tringZY", 10, 10, 15,   "TestStringabcde", STATUS_SUCCESS},
    { 5,  9, 15,  "TestS01234abcde", 5, 5, 7, "tringZY",  5,  9, 15,   "TestS01234abcde", STATUS_BUFFER_TOO_SMALL},
    { 5,  0, 15,  "TestS01234abcde", 0, 0, 7, "tringZY",  5,  0, 15,   "TestS01234abcde", STATUS_SUCCESS},
    { 5, 14, 15,  "TestS01234abcde", 0, 0, 7, "tringZY",  5, 14, 15,   "TestS01234abcde", STATUS_SUCCESS},
    { 5, 14, 15,  "TestS01234abcde", 0, 0, 7,      NULL,  5, 14, 15,   "TestS01234abcde", STATUS_SUCCESS},
    { 5, 14, 15,               NULL, 0, 0, 7,      NULL,  5, 14, 15,                NULL, STATUS_SUCCESS},
    { 5, 12, 15, "Tst\0S01234abcde", 4, 4, 7, "tr\0iZY",  9, 12, 15, "Tst\0Str\0i4abcde", STATUS_SUCCESS},
};


static void test_RtlAppendStringToString(void)
{
    CHAR dest_buf[257];
    CHAR src_buf[257];
    STRING dest_str;
    STRING src_str;
    NTSTATUS result;
    unsigned int test_num;

    for (test_num = 0; test_num < ARRAY_SIZE(app_str2str); test_num++) {
	dest_str.Length        = app_str2str[test_num].dest_Length;
	dest_str.MaximumLength = app_str2str[test_num].dest_MaximumLength;
	if (app_str2str[test_num].dest_buf != NULL) {
	    memcpy(dest_buf, app_str2str[test_num].dest_buf, app_str2str[test_num].dest_buf_size);
	    dest_buf[app_str2str[test_num].dest_buf_size] = '\0';
	    dest_str.Buffer = dest_buf;
	} else {
	    dest_str.Buffer = NULL;
	}
	src_str.Length         = app_str2str[test_num].src_Length;
	src_str.MaximumLength  = app_str2str[test_num].src_MaximumLength;
	if (app_str2str[test_num].src_buf != NULL) {
	    memcpy(src_buf, app_str2str[test_num].src_buf, app_str2str[test_num].src_buf_size);
	    src_buf[app_str2str[test_num].src_buf_size] = '\0';
	    src_str.Buffer = src_buf;
	} else {
	    src_str.Buffer = NULL;
	}
	result = pRtlAppendStringToString(&dest_str, &src_str);
	ok(result == app_str2str[test_num].result,
           "(test %d): RtlAppendStringToString(dest, src) has result %lx, expected %lx\n",
	   test_num, result, app_str2str[test_num].result);
	ok(dest_str.Length == app_str2str[test_num].res_Length,
	   "(test %d): RtlAppendStringToString(dest, src) dest has Length %d, expected %d\n",
	   test_num, dest_str.Length, app_str2str[test_num].res_Length);
	ok(dest_str.MaximumLength == app_str2str[test_num].res_MaximumLength,
	   "(test %d): RtlAppendStringToString(dest, src) dest has MaximumLength %d, expected %d\n",
	   test_num, dest_str.MaximumLength, app_str2str[test_num].res_MaximumLength);
	if (dest_str.Buffer == dest_buf) {
	    ok(memcmp(dest_buf, app_str2str[test_num].res_buf, app_str2str[test_num].res_buf_size) == 0,
	       "(test %d): RtlAppendStringToString(dest, src) has dest \"%s\" expected \"%s\"\n",
	       test_num, dest_buf, app_str2str[test_num].res_buf);
	} else {
	    ok(dest_str.Buffer == app_str2str[test_num].res_buf,
	       "(test %d): RtlAppendStringToString(dest, src) dest has Buffer %p expected %p\n",
	       test_num, dest_str.Buffer, app_str2str[test_num].res_buf);
	}
    }
}


typedef struct {
    int dest_Length;
    int dest_MaximumLength;
    int dest_buf_size;
    const char *dest_buf;
    const char *src;
    int res_Length;
    int res_MaximumLength;
    int res_buf_size;
    const char *res_buf;
    NTSTATUS result;
} app_uni2str_t;

static const app_uni2str_t app_uni2str[] = {
    { 4, 12, 14,     "Fake0123abcdef",    "Ustr\0",  8, 12, 14,  "FakeUstr\0\0cdef", STATUS_SUCCESS},
    { 4, 11, 14,     "Fake0123abcdef",    "Ustr\0",  8, 11, 14,  "FakeUstr\0\0cdef", STATUS_SUCCESS},
    { 4, 10, 14,     "Fake0123abcdef",    "Ustr\0",  8, 10, 14,  "FakeUstr\0\0cdef", STATUS_SUCCESS},
/* In the following test the native function writes beyond MaximumLength
 *  { 4,  9, 14,     "Fake0123abcdef",    "Ustr\0",  8,  9, 14,    "FakeUstrabcdef", STATUS_SUCCESS},
 */
    { 4,  8, 14,     "Fake0123abcdef",    "Ustr\0",  8,  8, 14,    "FakeUstrabcdef", STATUS_SUCCESS},
    { 4,  7, 14,     "Fake0123abcdef",    "Ustr\0",  4,  7, 14,    "Fake0123abcdef", STATUS_BUFFER_TOO_SMALL},
    { 4,  0, 14,     "Fake0123abcdef",    "Ustr\0",  4,  0, 14,    "Fake0123abcdef", STATUS_BUFFER_TOO_SMALL},
    { 4, 14, 14,     "Fake0123abcdef",    "Ustr\0",  8, 14, 14,  "FakeUstr\0\0cdef", STATUS_SUCCESS},
    { 4, 14, 14,     "Fake0123abcdef",        NULL,  4, 14, 14,    "Fake0123abcdef", STATUS_SUCCESS},
    { 4, 14, 14,                 NULL,        NULL,  4, 14, 14,                NULL, STATUS_SUCCESS},
    { 4, 14, 14,     "Fake0123abcdef", "U\0stri\0", 10, 14, 14, "FakeU\0stri\0\0ef", STATUS_SUCCESS},
    { 6, 14, 16, "Te\0\0stabcdefghij",  "St\0\0ri",  8, 14, 16, "Te\0\0stSt\0\0efghij", STATUS_SUCCESS},
};


static void test_RtlAppendUnicodeToString(void)
{
    WCHAR dest_buf[257];
    UNICODE_STRING dest_str;
    NTSTATUS result;
    unsigned int test_num;

    for (test_num = 0; test_num < ARRAY_SIZE(app_uni2str); test_num++) {
	dest_str.Length        = app_uni2str[test_num].dest_Length;
	dest_str.MaximumLength = app_uni2str[test_num].dest_MaximumLength;
	if (app_uni2str[test_num].dest_buf != NULL) {
	    memcpy(dest_buf, app_uni2str[test_num].dest_buf, app_uni2str[test_num].dest_buf_size);
	    dest_buf[app_uni2str[test_num].dest_buf_size/sizeof(WCHAR)] = '\0';
	    dest_str.Buffer = dest_buf;
	} else {
	    dest_str.Buffer = NULL;
	}
	result = pRtlAppendUnicodeToString(&dest_str, (LPCWSTR) app_uni2str[test_num].src);
	ok(result == app_uni2str[test_num].result,
           "(test %d): RtlAppendUnicodeToString(dest, src) has result %lx, expected %lx\n",
	   test_num, result, app_uni2str[test_num].result);
	ok(dest_str.Length == app_uni2str[test_num].res_Length,
	   "(test %d): RtlAppendUnicodeToString(dest, src) dest has Length %d, expected %d\n",
	   test_num, dest_str.Length, app_uni2str[test_num].res_Length);
	ok(dest_str.MaximumLength == app_uni2str[test_num].res_MaximumLength,
	   "(test %d): RtlAppendUnicodeToString(dest, src) dest has MaximumLength %d, expected %d\n",
	   test_num, dest_str.MaximumLength, app_uni2str[test_num].res_MaximumLength);
	if (dest_str.Buffer == dest_buf) {
	    ok(memcmp(dest_buf, app_uni2str[test_num].res_buf, app_uni2str[test_num].res_buf_size) == 0,
	       "(test %d): RtlAppendUnicodeToString(dest, src) has dest \"%s\" expected \"%s\"\n",
	       test_num, (char *) dest_buf, app_uni2str[test_num].res_buf);
	} else {
	    ok(dest_str.Buffer == (WCHAR *) app_uni2str[test_num].res_buf,
	       "(test %d): RtlAppendUnicodeToString(dest, src) dest has Buffer %p expected %p\n",
	       test_num, dest_str.Buffer, app_uni2str[test_num].res_buf);
	}
    }
}


typedef struct {
    int dest_Length;
    int dest_MaximumLength;
    int dest_buf_size;
    const char *dest_buf;
    int src_Length;
    int src_MaximumLength;
    int src_buf_size;
    const char *src_buf;
    int res_Length;
    int res_MaximumLength;
    int res_buf_size;
    const char *res_buf;
    NTSTATUS result;
} app_ustr2str_t;

static const app_ustr2str_t app_ustr2str[] = {
    { 4, 12, 14,     "Fake0123abcdef", 4, 6, 8,   "UstrZYXW",  8, 12, 14,   "FakeUstr\0\0cdef", STATUS_SUCCESS},
    { 4, 11, 14,     "Fake0123abcdef", 4, 6, 8,   "UstrZYXW",  8, 11, 14,   "FakeUstr\0\0cdef", STATUS_SUCCESS},
    { 4, 10, 14,     "Fake0123abcdef", 4, 6, 8,   "UstrZYXW",  8, 10, 14,   "FakeUstr\0\0cdef", STATUS_SUCCESS},
/* In the following test the native function writes beyond MaximumLength 
 *  { 4,  9, 14,     "Fake0123abcdef", 4, 6, 8,   "UstrZYXW",  8,  9, 14,     "FakeUstrabcdef", STATUS_SUCCESS},
 */
    { 4,  8, 14,     "Fake0123abcdef", 4, 6, 8,   "UstrZYXW",  8,  8, 14,     "FakeUstrabcdef", STATUS_SUCCESS},
    { 4,  7, 14,     "Fake0123abcdef", 4, 6, 8,   "UstrZYXW",  4,  7, 14,     "Fake0123abcdef", STATUS_BUFFER_TOO_SMALL},
    { 4,  0, 14,     "Fake0123abcdef", 0, 0, 8,   "UstrZYXW",  4,  0, 14,     "Fake0123abcdef", STATUS_SUCCESS},
    { 4, 14, 14,     "Fake0123abcdef", 0, 0, 8,   "UstrZYXW",  4, 14, 14,     "Fake0123abcdef", STATUS_SUCCESS},
    { 4, 14, 14,     "Fake0123abcdef", 0, 0, 8,         NULL,  4, 14, 14,     "Fake0123abcdef", STATUS_SUCCESS},
    { 4, 14, 14,                 NULL, 0, 0, 8,         NULL,  4, 14, 14,                 NULL, STATUS_SUCCESS},
    { 6, 14, 16, "Te\0\0stabcdefghij", 6, 8, 8, "St\0\0riZY", 12, 14, 16, "Te\0\0stSt\0\0ri\0\0ij", STATUS_SUCCESS},
};


static void test_RtlAppendUnicodeStringToString(void)
{
    WCHAR dest_buf[257];
    WCHAR src_buf[257];
    UNICODE_STRING dest_str;
    UNICODE_STRING src_str;
    NTSTATUS result;
    unsigned int test_num;

    for (test_num = 0; test_num < ARRAY_SIZE(app_ustr2str); test_num++) {
	dest_str.Length        = app_ustr2str[test_num].dest_Length;
	dest_str.MaximumLength = app_ustr2str[test_num].dest_MaximumLength;
	if (app_ustr2str[test_num].dest_buf != NULL) {
	    memcpy(dest_buf, app_ustr2str[test_num].dest_buf, app_ustr2str[test_num].dest_buf_size);
	    dest_buf[app_ustr2str[test_num].dest_buf_size/sizeof(WCHAR)] = '\0';
	    dest_str.Buffer = dest_buf;
	} else {
	    dest_str.Buffer = NULL;
	}
	src_str.Length         = app_ustr2str[test_num].src_Length;
	src_str.MaximumLength  = app_ustr2str[test_num].src_MaximumLength;
	if (app_ustr2str[test_num].src_buf != NULL) {
	    memcpy(src_buf, app_ustr2str[test_num].src_buf, app_ustr2str[test_num].src_buf_size);
	    src_buf[app_ustr2str[test_num].src_buf_size/sizeof(WCHAR)] = '\0';
	    src_str.Buffer = src_buf;
	} else {
	    src_str.Buffer = NULL;
	}
	result = pRtlAppendUnicodeStringToString(&dest_str, &src_str);
	ok(result == app_ustr2str[test_num].result,
           "(test %d): RtlAppendStringToString(dest, src) has result %lx, expected %lx\n",
	   test_num, result, app_ustr2str[test_num].result);
	ok(dest_str.Length == app_ustr2str[test_num].res_Length,
	   "(test %d): RtlAppendStringToString(dest, src) dest has Length %d, expected %d\n",
	   test_num, dest_str.Length, app_ustr2str[test_num].res_Length);
	ok(dest_str.MaximumLength == app_ustr2str[test_num].res_MaximumLength,
	   "(test %d): RtlAppendStringToString(dest, src) dest has MaximumLength %d, expected %d\n",
	   test_num, dest_str.MaximumLength, app_ustr2str[test_num].res_MaximumLength);
	if (dest_str.Buffer == dest_buf) {
	    ok(memcmp(dest_buf, app_ustr2str[test_num].res_buf, app_ustr2str[test_num].res_buf_size) == 0,
	       "(test %d): RtlAppendStringToString(dest, src) has dest \"%s\" expected \"%s\"\n",
	       test_num, (char *) dest_buf, app_ustr2str[test_num].res_buf);
	} else {
	    ok(dest_str.Buffer == (WCHAR *) app_ustr2str[test_num].res_buf,
	       "(test %d): RtlAppendStringToString(dest, src) dest has Buffer %p expected %p\n",
	       test_num, dest_str.Buffer, app_ustr2str[test_num].res_buf);
	}
    }
}


typedef struct {
    int flags;
    const char *main_str;
    const char *search_chars;
    USHORT pos;
    NTSTATUS result;
} find_ch_in_ustr_t;

static const find_ch_in_ustr_t find_ch_in_ustr[] = {
    { 0, "Some Wild String",           "S",       2, STATUS_SUCCESS},
    { 0, "This is a String",           "String",  6, STATUS_SUCCESS},
    { 1, "This is a String",           "String", 30, STATUS_SUCCESS},
    { 2, "This is a String",           "String",  2, STATUS_SUCCESS},
    { 3, "This is a String",           "String", 18, STATUS_SUCCESS},
    { 0, "This is a String",           "Wild",    6, STATUS_SUCCESS},
    { 1, "This is a String",           "Wild",   26, STATUS_SUCCESS},
    { 2, "This is a String",           "Wild",    2, STATUS_SUCCESS},
    { 3, "This is a String",           "Wild",   30, STATUS_SUCCESS},
    { 0, "abcdefghijklmnopqrstuvwxyz", "",        0, STATUS_NOT_FOUND},
    { 0, "abcdefghijklmnopqrstuvwxyz", "123",     0, STATUS_NOT_FOUND},
    { 0, "abcdefghijklmnopqrstuvwxyz", "a",       2, STATUS_SUCCESS},
    { 0, "abcdefghijklmnopqrstuvwxyz", "12a34",   2, STATUS_SUCCESS},
    { 0, "abcdefghijklmnopqrstuvwxyz", "12b34",   4, STATUS_SUCCESS},
    { 0, "abcdefghijklmnopqrstuvwxyz", "12y34",  50, STATUS_SUCCESS},
    { 0, "abcdefghijklmnopqrstuvwxyz", "12z34",  52, STATUS_SUCCESS},
    { 0, "abcdefghijklmnopqrstuvwxyz", "rvz",    36, STATUS_SUCCESS},
    { 0, "abcdefghijklmmlkjihgfedcba", "egik",   10, STATUS_SUCCESS},
    { 1, "abcdefghijklmnopqrstuvwxyz", "",        0, STATUS_NOT_FOUND},
    { 1, "abcdefghijklmnopqrstuvwxyz", "rvz",    50, STATUS_SUCCESS},
    { 1, "abcdefghijklmnopqrstuvwxyz", "ravy",   48, STATUS_SUCCESS},
    { 1, "abcdefghijklmnopqrstuvwxyz", "raxv",   46, STATUS_SUCCESS},
    { 2, "abcdefghijklmnopqrstuvwxyz", "",        2, STATUS_SUCCESS},
    { 2, "abcdefghijklmnopqrstuvwxyz", "rvz",     2, STATUS_SUCCESS},
    { 2, "abcdefghijklmnopqrstuvwxyz", "vaz",     4, STATUS_SUCCESS},
    { 2, "abcdefghijklmnopqrstuvwxyz", "ravbz",   6, STATUS_SUCCESS},
    { 3, "abcdefghijklmnopqrstuvwxyz", "",       50, STATUS_SUCCESS},
    { 3, "abcdefghijklmnopqrstuvwxyz", "123",    50, STATUS_SUCCESS},
    { 3, "abcdefghijklmnopqrstuvwxyz", "ahp",    50, STATUS_SUCCESS},
    { 3, "abcdefghijklmnopqrstuvwxyz", "rvz",    48, STATUS_SUCCESS},
    { 0, NULL,                         "abc",     0, STATUS_NOT_FOUND},
    { 1, NULL,                         "abc",     0, STATUS_NOT_FOUND},
    { 2, NULL,                         "abc",     0, STATUS_NOT_FOUND},
    { 3, NULL,                         "abc",     0, STATUS_NOT_FOUND},
    { 0, "abcdefghijklmnopqrstuvwxyz", NULL,      0, STATUS_NOT_FOUND},
    { 1, "abcdefghijklmnopqrstuvwxyz", NULL,      0, STATUS_NOT_FOUND},
    { 2, "abcdefghijklmnopqrstuvwxyz", NULL,      2, STATUS_SUCCESS},
    { 3, "abcdefghijklmnopqrstuvwxyz", NULL,     50, STATUS_SUCCESS},
    { 0, NULL,                         NULL,      0, STATUS_NOT_FOUND},
    { 1, NULL,                         NULL,      0, STATUS_NOT_FOUND},
    { 2, NULL,                         NULL,      0, STATUS_NOT_FOUND},
    { 3, NULL,                         NULL,      0, STATUS_NOT_FOUND},
    { 0, "abcdabcdabcdabcdabcdabcd",   "abcd",    2, STATUS_SUCCESS},
    { 1, "abcdabcdabcdabcdabcdabcd",   "abcd",   46, STATUS_SUCCESS},
    { 2, "abcdabcdabcdabcdabcdabcd",   "abcd",    0, STATUS_NOT_FOUND},
    { 3, "abcdabcdabcdabcdabcdabcd",   "abcd",    0, STATUS_NOT_FOUND},
};


static void test_RtlFindCharInUnicodeString(void)
{
    WCHAR main_str_buf[257];
    WCHAR search_chars_buf[257];
    UNICODE_STRING main_str;
    UNICODE_STRING search_chars;
    USHORT pos;
    NTSTATUS result;
    unsigned int idx;
    unsigned int test_num;

    if (!pRtlFindCharInUnicodeString)
    {
        win_skip("RtlFindCharInUnicodeString is not available\n");
        return;
    }

    for (test_num = 0; test_num < ARRAY_SIZE(find_ch_in_ustr); test_num++) {
	if (find_ch_in_ustr[test_num].main_str != NULL) {
	    main_str.Length        = strlen(find_ch_in_ustr[test_num].main_str) * sizeof(WCHAR);
	    main_str.MaximumLength = main_str.Length + sizeof(WCHAR);
	    for (idx = 0; idx < main_str.Length / sizeof(WCHAR); idx++) {
		main_str_buf[idx] = find_ch_in_ustr[test_num].main_str[idx];
	    }
	    main_str.Buffer = main_str_buf;
	} else {
	    main_str.Length        = 0;
	    main_str.MaximumLength = 0;
	    main_str.Buffer        = NULL;
	}
	if (find_ch_in_ustr[test_num].search_chars != NULL) {
	    search_chars.Length        = strlen(find_ch_in_ustr[test_num].search_chars) * sizeof(WCHAR);
	    search_chars.MaximumLength = search_chars.Length + sizeof(WCHAR);
	    for (idx = 0; idx < search_chars.Length / sizeof(WCHAR); idx++) {
		search_chars_buf[idx] = find_ch_in_ustr[test_num].search_chars[idx];
	    }
	    search_chars.Buffer = search_chars_buf;
	} else {
	    search_chars.Length        = 0;
	    search_chars.MaximumLength = 0;
	    search_chars.Buffer        = NULL;
	}
	pos = 12345;
        result = pRtlFindCharInUnicodeString(find_ch_in_ustr[test_num].flags, &main_str, &search_chars, &pos);
        ok(result == find_ch_in_ustr[test_num].result,
           "(test %d): RtlFindCharInUnicodeString(%d, %s, %s, [out]) has result %lx, expected %lx\n",
           test_num, find_ch_in_ustr[test_num].flags,
           find_ch_in_ustr[test_num].main_str, find_ch_in_ustr[test_num].search_chars,
           result, find_ch_in_ustr[test_num].result);
        ok(pos == find_ch_in_ustr[test_num].pos,
           "(test %d): RtlFindCharInUnicodeString(%d, %s, %s, [out]) assigns %d to pos, expected %d\n",
           test_num, find_ch_in_ustr[test_num].flags,
           find_ch_in_ustr[test_num].main_str, find_ch_in_ustr[test_num].search_chars,
           pos, find_ch_in_ustr[test_num].pos);
    }
}


typedef struct {
    int base;
    const char *str;
    int value;
    NTSTATUS result, alternative;
} str2int_t;

static const str2int_t str2int[] = {
    { 0, "1011101100",   1011101100, STATUS_SUCCESS},
    { 0, "1234567",         1234567, STATUS_SUCCESS},
    { 0, "-214",               -214, STATUS_SUCCESS},
    { 0, "+214",                214, STATUS_SUCCESS}, /* The + sign is allowed also */
    { 0, "--214",                 0, STATUS_SUCCESS}, /* Do not accept more than one sign */
    { 0, "-+214",                 0, STATUS_SUCCESS},
    { 0, "++214",                 0, STATUS_SUCCESS},
    { 0, "+-214",                 0, STATUS_SUCCESS},
    { 0, "\001\002\003\00411",   11, STATUS_SUCCESS}, /* whitespace char  1 to  4 */
    { 0, "\005\006\007\01012",   12, STATUS_SUCCESS}, /* whitespace char  5 to  8 */
    { 0, "\011\012\013\01413",   13, STATUS_SUCCESS}, /* whitespace char  9 to 12 */
    { 0, "\015\016\017\02014",   14, STATUS_SUCCESS}, /* whitespace char 13 to 16 */
    { 0, "\021\022\023\02415",   15, STATUS_SUCCESS}, /* whitespace char 17 to 20 */
    { 0, "\025\026\027\03016",   16, STATUS_SUCCESS}, /* whitespace char 21 to 24 */
    { 0, "\031\032\033\03417",   17, STATUS_SUCCESS}, /* whitespace char 25 to 28 */
    { 0, "\035\036\037\04018",   18, STATUS_SUCCESS}, /* whitespace char 29 to 32 */
    { 0, " \n \r \t214",        214, STATUS_SUCCESS},
    { 0, " \n \r \t+214",       214, STATUS_SUCCESS}, /* Signs can be used after whitespace */
    { 0, " \n \r \t-214",      -214, STATUS_SUCCESS},
    { 0, "+214 0",              214, STATUS_SUCCESS}, /* Space terminates the number */
    { 0, " 214.01",             214, STATUS_SUCCESS}, /* Decimal point not accepted */
    { 0, " 214,01",             214, STATUS_SUCCESS}, /* Decimal comma not accepted */
    { 0, "f81",                   0, STATUS_SUCCESS},
    { 0, "0x12345",         0x12345, STATUS_SUCCESS}, /* Hex */
    { 0, "00x12345",              0, STATUS_SUCCESS},
    { 0, "0xx12345",              0, STATUS_SUCCESS},
    { 0, "1x34",                  1, STATUS_SUCCESS},
    { 0, "-9999999999", -1410065407, STATUS_SUCCESS}, /* Big negative integer */
    { 0, "-2147483649",  2147483647, STATUS_SUCCESS}, /* Too small to fit in 32 Bits */
    { 0, "-2147483648", 0x80000000L, STATUS_SUCCESS}, /* Smallest negative integer */
    { 0, "-2147483647", -2147483647, STATUS_SUCCESS},
    { 0, "-1",                   -1, STATUS_SUCCESS},
    { 0, "0",                     0, STATUS_SUCCESS},
    { 0, "1",                     1, STATUS_SUCCESS},
    { 0, "2147483646",   2147483646, STATUS_SUCCESS},
    { 0, "2147483647",   2147483647, STATUS_SUCCESS}, /* Largest signed positive integer */
    { 0, "2147483648",  0x80000000L, STATUS_SUCCESS}, /* Positive int equal to smallest negative int */
    { 0, "2147483649",  -2147483647, STATUS_SUCCESS},
    { 0, "4294967294",           -2, STATUS_SUCCESS},
    { 0, "4294967295",           -1, STATUS_SUCCESS}, /* Largest unsigned integer */
    { 0, "4294967296",            0, STATUS_SUCCESS}, /* Too big to fit in 32 Bits */
    { 0, "9999999999",   1410065407, STATUS_SUCCESS}, /* Big positive integer */
    { 0, "056789",            56789, STATUS_SUCCESS}, /* Leading zero and still decimal */
    { 0, "b1011101100",           0, STATUS_SUCCESS}, /* Binary (b-notation) */
    { 0, "-b1011101100",          0, STATUS_SUCCESS}, /* Negative Binary (b-notation) */
    { 0, "b10123456789",          0, STATUS_SUCCESS}, /* Binary with nonbinary digits (2-9) */
    { 0, "0b1011101100",        748, STATUS_SUCCESS}, /* Binary (0b-notation) */
    { 0, "-0b1011101100",      -748, STATUS_SUCCESS}, /* Negative binary (0b-notation) */
    { 0, "0b10123456789",         5, STATUS_SUCCESS}, /* Binary with nonbinary digits (2-9) */
    { 0, "-0b10123456789",       -5, STATUS_SUCCESS}, /* Negative binary with nonbinary digits (2-9) */
    { 0, "0b1",                   1, STATUS_SUCCESS}, /* one digit binary */
    { 0, "0b2",                   0, STATUS_SUCCESS}, /* empty binary */
    { 0, "0b",                    0, STATUS_SUCCESS}, /* empty binary */
    { 0, "o1234567",              0, STATUS_SUCCESS}, /* Octal (o-notation) */
    { 0, "-o1234567",             0, STATUS_SUCCESS}, /* Negative Octal (o-notation) */
    { 0, "o56789",                0, STATUS_SUCCESS}, /* Octal with nonoctal digits (8 and 9) */
    { 0, "0o1234567",      01234567, STATUS_SUCCESS}, /* Octal (0o-notation) */
    { 0, "-0o1234567",    -01234567, STATUS_SUCCESS}, /* Negative octal (0o-notation) */
    { 0, "0o56789",            0567, STATUS_SUCCESS}, /* Octal with nonoctal digits (8 and 9) */
    { 0, "-0o56789",          -0567, STATUS_SUCCESS}, /* Negative octal with nonoctal digits (8 and 9) */
    { 0, "0o7",                   7, STATUS_SUCCESS}, /* one digit octal */
    { 0, "0o8",                   0, STATUS_SUCCESS}, /* empty octal */
    { 0, "0o",                    0, STATUS_SUCCESS}, /* empty octal */
    { 0, "0d1011101100",          0, STATUS_SUCCESS}, /* explicit decimal with 0d */
    { 0, "x89abcdef",             0, STATUS_SUCCESS}, /* Hex with lower case digits a-f (x-notation) */
    { 0, "xFEDCBA00",             0, STATUS_SUCCESS}, /* Hex with upper case digits A-F (x-notation) */
    { 0, "-xFEDCBA00",            0, STATUS_SUCCESS}, /* Negative Hexadecimal (x-notation) */
    { 0, "0x89abcdef",   0x89abcdef, STATUS_SUCCESS}, /* Hex with lower case digits a-f (0x-notation) */
    { 0, "0xFEDCBA00",   0xFEDCBA00, STATUS_SUCCESS}, /* Hex with upper case digits A-F (0x-notation) */
    { 0, "-0xFEDCBA00",    19088896, STATUS_SUCCESS}, /* Negative Hexadecimal (0x-notation) */
    { 0, "0xabcdefgh",     0xabcdef, STATUS_SUCCESS}, /* Hex with illegal lower case digits (g-z) */
    { 0, "0xABCDEFGH",     0xABCDEF, STATUS_SUCCESS}, /* Hex with illegal upper case digits (G-Z) */
    { 0, "0xF",                 0xf, STATUS_SUCCESS}, /* one digit hexadecimal */
    { 0, "0xG",                   0, STATUS_SUCCESS}, /* empty hexadecimal */
    { 0, "0x",                    0, STATUS_SUCCESS}, /* empty hexadecimal */
    { 0, "",                      0, STATUS_SUCCESS, STATUS_INVALID_PARAMETER}, /* empty string */
    { 2, "1011101100",          748, STATUS_SUCCESS},
    { 2, "-1011101100",        -748, STATUS_SUCCESS},
    { 2, "2",                     0, STATUS_SUCCESS},
    { 2, "0b1011101100",          0, STATUS_SUCCESS},
    { 2, "0o1011101100",          0, STATUS_SUCCESS},
    { 2, "0d1011101100",          0, STATUS_SUCCESS},
    { 2, "0x1011101100",          0, STATUS_SUCCESS},
    { 2, "",                      0, STATUS_SUCCESS, STATUS_INVALID_PARAMETER}, /* empty string */
    { 8, "1011101100",    136610368, STATUS_SUCCESS},
    { 8, "-1011101100",  -136610368, STATUS_SUCCESS},
    { 8, "8",                     0, STATUS_SUCCESS},
    { 8, "0b1011101100",          0, STATUS_SUCCESS},
    { 8, "0o1011101100",          0, STATUS_SUCCESS},
    { 8, "0d1011101100",          0, STATUS_SUCCESS},
    { 8, "0x1011101100",          0, STATUS_SUCCESS},
    { 8, "",                      0, STATUS_SUCCESS, STATUS_INVALID_PARAMETER}, /* empty string */
    {10, "1011101100",   1011101100, STATUS_SUCCESS},
    {10, "-1011101100", -1011101100, STATUS_SUCCESS},
    {10, "0b1011101100",          0, STATUS_SUCCESS},
    {10, "0o1011101100",          0, STATUS_SUCCESS},
    {10, "0d1011101100",          0, STATUS_SUCCESS},
    {10, "0x1011101100",          0, STATUS_SUCCESS},
    {10, "o12345",                0, STATUS_SUCCESS}, /* Octal although base is 10 */
    {10, "",                      0, STATUS_SUCCESS, STATUS_INVALID_PARAMETER}, /* empty string */
    {16, "1011101100",    286265600, STATUS_SUCCESS},
    {16, "-1011101100",  -286265600, STATUS_SUCCESS},
    {16, "G",                     0, STATUS_SUCCESS},
    {16, "g",                     0, STATUS_SUCCESS},
    {16, "0b1011101100",  286265600, STATUS_SUCCESS},
    {16, "0o1011101100",          0, STATUS_SUCCESS},
    {16, "0d1011101100",  286265600, STATUS_SUCCESS},
    {16, "0x1011101100",          0, STATUS_SUCCESS},
    {16, "",                      0, STATUS_SUCCESS, STATUS_INVALID_PARAMETER}, /* empty string */
    {20, "0",                     0, STATUS_INVALID_PARAMETER}, /* illegal base */
    {-8, "0",                     0, STATUS_INVALID_PARAMETER}, /* Negative base */
/*    { 0, NULL,                    0, STATUS_SUCCESS}, */ /* NULL as string */
};


static void test_RtlUnicodeStringToInteger(void)
{
    unsigned int test_num;
    int value;
    NTSTATUS result;
    WCHAR *wstr;
    UNICODE_STRING uni;

    for (test_num = 0; test_num < ARRAY_SIZE(str2int); test_num++) {
	wstr = AtoW(str2int[test_num].str);
	value = 0xdeadbeef;
	pRtlInitUnicodeString(&uni, wstr);
	result = pRtlUnicodeStringToInteger(&uni, str2int[test_num].base, &value);
	ok(result == str2int[test_num].result ||
           (str2int[test_num].alternative && result == str2int[test_num].alternative),
           "(test %d): RtlUnicodeStringToInteger(\"%s\", %d, [out]) has result %lx, expected: %lx (%lx)\n",
	   test_num, str2int[test_num].str, str2int[test_num].base, result,
           str2int[test_num].result, str2int[test_num].alternative);
        if (result == STATUS_SUCCESS)
            ok(value == str2int[test_num].value ||
               broken(str2int[test_num].str[0] == '\0' && str2int[test_num].base == 16), /* nt4 */
               "(test %d): RtlUnicodeStringToInteger(\"%s\", %d, [out]) assigns value %d, expected: %d\n",
               test_num, str2int[test_num].str, str2int[test_num].base, value, str2int[test_num].value);
        else
            ok(value == 0xdeadbeef || value == 0 /* vista */,
               "(test %d): RtlUnicodeStringToInteger(\"%s\", %d, [out]) assigns value %d, expected 0 or deadbeef\n",
               test_num, str2int[test_num].str, str2int[test_num].base, value);
	HeapFree(GetProcessHeap(), 0, wstr);
    }

    wstr = AtoW(str2int[1].str);
    pRtlInitUnicodeString(&uni, wstr);
    result = pRtlUnicodeStringToInteger(&uni, str2int[1].base, NULL);
    ok(result == STATUS_ACCESS_VIOLATION,
       "call failed: RtlUnicodeStringToInteger(\"%s\", %d, NULL) has result %lx\n",
       str2int[1].str, str2int[1].base, result);
    result = pRtlUnicodeStringToInteger(&uni, 20, NULL);
    ok(result == STATUS_INVALID_PARAMETER || result == STATUS_ACCESS_VIOLATION,
       "call failed: RtlUnicodeStringToInteger(\"%s\", 20, NULL) has result %lx\n",
       str2int[1].str, result);

    uni.Length = 10; /* Make Length shorter (5 WCHARS instead of 7) */
    result = pRtlUnicodeStringToInteger(&uni, str2int[1].base, &value);
    ok(result == STATUS_SUCCESS,
       "call failed: RtlUnicodeStringToInteger(\"12345\", %d, [out]) has result %lx\n",
       str2int[1].base, result);
    ok(value == 12345,
       "didn't return expected value (test a): expected: %d, got: %d\n",
       12345, value);

    uni.Length = 5; /* Use odd Length (2.5 WCHARS) */
    result = pRtlUnicodeStringToInteger(&uni, str2int[1].base, &value);
    ok(result == STATUS_SUCCESS || result == STATUS_INVALID_PARAMETER /* vista */,
       "call failed: RtlUnicodeStringToInteger(\"12\", %d, [out]) has result %lx\n",
       str2int[1].base, result);
    if (result == STATUS_SUCCESS)
        ok(value == 12, "didn't return expected value (test b): expected: %d, got: %d\n", 12, value);

    uni.Length = 2;
    result = pRtlUnicodeStringToInteger(&uni, str2int[1].base, &value);
    ok(result == STATUS_SUCCESS,
       "call failed: RtlUnicodeStringToInteger(\"1\", %d, [out]) has result %lx\n",
       str2int[1].base, result);
    ok(value == 1,
       "didn't return expected value (test c): expected: %d, got: %d\n",
       1, value);
    /* w2k: uni.Length = 0 returns value 11234567 instead of 0 */
    HeapFree(GetProcessHeap(), 0, wstr);
}


static void test_RtlCharToInteger(void)
{
    unsigned int test_num;
    int value;
    NTSTATUS result;

    for (test_num = 0; test_num < ARRAY_SIZE(str2int); test_num++) {
	/* w2k skips a leading '\0' and processes the string after */
	if (str2int[test_num].str[0] != '\0') {
	    value = 0xdeadbeef;
	    result = pRtlCharToInteger(str2int[test_num].str, str2int[test_num].base, &value);
	    ok(result == str2int[test_num].result ||
               (str2int[test_num].alternative && result == str2int[test_num].alternative),
               "(test %d): call failed: RtlCharToInteger(\"%s\", %d, [out]) has result %lx, expected: %lx (%lx)\n",
	       test_num, str2int[test_num].str, str2int[test_num].base, result,
               str2int[test_num].result, str2int[test_num].alternative);
            if (result == STATUS_SUCCESS)
                ok(value == str2int[test_num].value,
                   "(test %d): call failed: RtlCharToInteger(\"%s\", %d, [out]) assigns value %d, expected: %d\n",
                   test_num, str2int[test_num].str, str2int[test_num].base, value, str2int[test_num].value);
            else
                ok(value == 0 || value == 0xdeadbeef,
                   "(test %d): call failed: RtlCharToInteger(\"%s\", %d, [out]) assigns value %d, expected 0 or deadbeef\n",
                   test_num, str2int[test_num].str, str2int[test_num].base, value);
	}
    }

    result = pRtlCharToInteger(str2int[1].str, str2int[1].base, NULL);
    ok(result == STATUS_ACCESS_VIOLATION,
       "call failed: RtlCharToInteger(\"%s\", %d, NULL) has result %lx\n",
       str2int[1].str, str2int[1].base, result);

    result = pRtlCharToInteger(str2int[1].str, 20, NULL);
    ok(result == STATUS_INVALID_PARAMETER,
       "call failed: RtlCharToInteger(\"%s\", 20, NULL) has result %lx\n",
       str2int[1].str, result);
}


#define STRI_BUFFER_LENGTH 35

typedef struct {
    int base;
    ULONG value;
    USHORT Length;
    USHORT MaximumLength;
    const char *Buffer;
    NTSTATUS result;
    int broken_len;
} int2str_t;

static const int2str_t int2str[] = {
    {10,          123,  3, 11, "123\0-------------------------------", STATUS_SUCCESS},

    { 0,  0x80000000U, 10, 11, "2147483648\0------------------------", STATUS_SUCCESS}, /* min signed int */
    { 0,  -2147483647, 10, 11, "2147483649\0------------------------", STATUS_SUCCESS},
    { 0,           -2, 10, 11, "4294967294\0------------------------", STATUS_SUCCESS},
    { 0,           -1, 10, 11, "4294967295\0------------------------", STATUS_SUCCESS},
    { 0,            0,  1, 11, "0\0---------------------------------", STATUS_SUCCESS},
    { 0,            1,  1, 11, "1\0---------------------------------", STATUS_SUCCESS},
    { 0,           12,  2, 11, "12\0--------------------------------", STATUS_SUCCESS},
    { 0,          123,  3, 11, "123\0-------------------------------", STATUS_SUCCESS},
    { 0,         1234,  4, 11, "1234\0------------------------------", STATUS_SUCCESS},
    { 0,        12345,  5, 11, "12345\0-----------------------------", STATUS_SUCCESS},
    { 0,       123456,  6, 11, "123456\0----------------------------", STATUS_SUCCESS},
    { 0,      1234567,  7, 11, "1234567\0---------------------------", STATUS_SUCCESS},
    { 0,     12345678,  8, 11, "12345678\0--------------------------", STATUS_SUCCESS},
    { 0,    123456789,  9, 11, "123456789\0-------------------------", STATUS_SUCCESS},
    { 0,   2147483646, 10, 11, "2147483646\0------------------------", STATUS_SUCCESS},
    { 0,   2147483647, 10, 11, "2147483647\0------------------------", STATUS_SUCCESS}, /* max signed int */
    { 0,  2147483648U, 10, 11, "2147483648\0------------------------", STATUS_SUCCESS}, /* uint = -max int */
    { 0,  2147483649U, 10, 11, "2147483649\0------------------------", STATUS_SUCCESS},
    { 0,  4294967294U, 10, 11, "4294967294\0------------------------", STATUS_SUCCESS},
    { 0,  4294967295U, 10, 11, "4294967295\0------------------------", STATUS_SUCCESS}, /* max unsigned int */

    { 2,  0x80000000U, 32, 33, "10000000000000000000000000000000\0--", STATUS_SUCCESS}, /* min signed int */
    { 2,  -2147483647, 32, 33, "10000000000000000000000000000001\0--", STATUS_SUCCESS},
    { 2,           -2, 32, 33, "11111111111111111111111111111110\0--", STATUS_SUCCESS},
    { 2,           -1, 32, 33, "11111111111111111111111111111111\0--", STATUS_SUCCESS},
    { 2,            0,  1, 33, "0\0---------------------------------", STATUS_SUCCESS},
    { 2,            1,  1, 33, "1\0---------------------------------", STATUS_SUCCESS},
    { 2,           10,  4, 33, "1010\0------------------------------", STATUS_SUCCESS},
    { 2,          100,  7, 33, "1100100\0---------------------------", STATUS_SUCCESS},
    { 2,         1000, 10, 33, "1111101000\0------------------------", STATUS_SUCCESS},
    { 2,        10000, 14, 33, "10011100010000\0--------------------", STATUS_SUCCESS},
    { 2,        32767, 15, 33, "111111111111111\0-------------------", STATUS_SUCCESS},
/*  { 2,        32768, 16, 33, "1000000000000000\0------------------", STATUS_SUCCESS}, broken on windows */
/*  { 2,        65535, 16, 33, "1111111111111111\0------------------", STATUS_SUCCESS}, broken on windows */
    { 2,        65536, 17, 33, "10000000000000000\0-----------------", STATUS_SUCCESS},
    { 2,       100000, 17, 33, "11000011010100000\0-----------------", STATUS_SUCCESS},
    { 2,      1000000, 20, 33, "11110100001001000000\0--------------", STATUS_SUCCESS},
    { 2,     10000000, 24, 33, "100110001001011010000000\0----------", STATUS_SUCCESS},
    { 2,    100000000, 27, 33, "101111101011110000100000000\0-------", STATUS_SUCCESS},
    { 2,   1000000000, 30, 33, "111011100110101100101000000000\0----", STATUS_SUCCESS},
    { 2,   1073741823, 30, 33, "111111111111111111111111111111\0----", STATUS_SUCCESS},
    { 2,   2147483646, 31, 33, "1111111111111111111111111111110\0---", STATUS_SUCCESS},
    { 2,   2147483647, 31, 33, "1111111111111111111111111111111\0---", STATUS_SUCCESS}, /* max signed int */
    { 2,  2147483648U, 32, 33, "10000000000000000000000000000000\0--", STATUS_SUCCESS}, /* uint = -max int */
    { 2,  2147483649U, 32, 33, "10000000000000000000000000000001\0--", STATUS_SUCCESS},
    { 2,  4294967294U, 32, 33, "11111111111111111111111111111110\0--", STATUS_SUCCESS},
    { 2,  4294967295U, 32, 33, "11111111111111111111111111111111\0--", STATUS_SUCCESS}, /* max unsigned int */

    { 8,  0x80000000U, 11, 12, "20000000000\0-----------------------", STATUS_SUCCESS}, /* min signed int */
    { 8,  -2147483647, 11, 12, "20000000001\0-----------------------", STATUS_SUCCESS},
    { 8,           -2, 11, 12, "37777777776\0-----------------------", STATUS_SUCCESS},
    { 8,           -1, 11, 12, "37777777777\0-----------------------", STATUS_SUCCESS},
    { 8,            0,  1, 12, "0\0---------------------------------", STATUS_SUCCESS},
    { 8,            1,  1, 12, "1\0---------------------------------", STATUS_SUCCESS},
    { 8,   2147483646, 11, 12, "17777777776\0-----------------------", STATUS_SUCCESS},
    { 8,   2147483647, 11, 12, "17777777777\0-----------------------", STATUS_SUCCESS}, /* max signed int */
    { 8,  2147483648U, 11, 12, "20000000000\0-----------------------", STATUS_SUCCESS}, /* uint = -max int */
    { 8,  2147483649U, 11, 12, "20000000001\0-----------------------", STATUS_SUCCESS},
    { 8,  4294967294U, 11, 12, "37777777776\0-----------------------", STATUS_SUCCESS},
    { 8,  4294967295U, 11, 12, "37777777777\0-----------------------", STATUS_SUCCESS}, /* max unsigned int */

    {10,  0x80000000U, 10, 11, "2147483648\0------------------------", STATUS_SUCCESS}, /* min signed int */
    {10,  -2147483647, 10, 11, "2147483649\0------------------------", STATUS_SUCCESS},
    {10,           -2, 10, 11, "4294967294\0------------------------", STATUS_SUCCESS},
    {10,           -1, 10, 11, "4294967295\0------------------------", STATUS_SUCCESS},
    {10,            0,  1, 11, "0\0---------------------------------", STATUS_SUCCESS},
    {10,            1,  1, 11, "1\0---------------------------------", STATUS_SUCCESS},
    {10,   2147483646, 10, 11, "2147483646\0------------------------", STATUS_SUCCESS},
    {10,   2147483647, 10, 11, "2147483647\0------------------------", STATUS_SUCCESS}, /* max signed int */
    {10,  2147483648U, 10, 11, "2147483648\0------------------------", STATUS_SUCCESS}, /* uint = -max int */
    {10,  2147483649U, 10, 11, "2147483649\0------------------------", STATUS_SUCCESS},
    {10,  4294967294U, 10, 11, "4294967294\0------------------------", STATUS_SUCCESS},
    {10,  4294967295U, 10, 11, "4294967295\0------------------------", STATUS_SUCCESS}, /* max unsigned int */

    {16,  0x80000000U,  8,  9, "80000000\0--------------------------", STATUS_SUCCESS}, /* min signed int */
    {16,  -2147483647,  8,  9, "80000001\0--------------------------", STATUS_SUCCESS},
    {16,           -2,  8,  9, "FFFFFFFE\0--------------------------", STATUS_SUCCESS},
    {16,           -1,  8,  9, "FFFFFFFF\0--------------------------", STATUS_SUCCESS},
    {16,            0,  1,  9, "0\0---------------------------------", STATUS_SUCCESS},
    {16,            1,  1,  9, "1\0---------------------------------", STATUS_SUCCESS},
    {16,   2147483646,  8,  9, "7FFFFFFE\0--------------------------", STATUS_SUCCESS},
    {16,   2147483647,  8,  9, "7FFFFFFF\0--------------------------", STATUS_SUCCESS}, /* max signed int */
    {16,  2147483648U,  8,  9, "80000000\0--------------------------", STATUS_SUCCESS}, /* uint = -max int */
    {16,  2147483649U,  8,  9, "80000001\0--------------------------", STATUS_SUCCESS},
    {16,  4294967294U,  8,  9, "FFFFFFFE\0--------------------------", STATUS_SUCCESS},
    {16,  4294967295U,  8,  9, "FFFFFFFF\0--------------------------", STATUS_SUCCESS}, /* max unsigned int */

/*  { 2,        32768, 16, 17, "1000000000000000\0------------------", STATUS_SUCCESS}, broken on windows */
/*  { 2,        32768, 16, 16, "1000000000000000-------------------",  STATUS_SUCCESS}, broken on windows */
    { 2,        65536, 17, 18, "10000000000000000\0-----------------", STATUS_SUCCESS},
    { 2,        65536, 17, 17, "10000000000000000------------------",  STATUS_SUCCESS},
    { 2,       131072, 18, 19, "100000000000000000\0----------------", STATUS_SUCCESS},
    { 2,       131072, 18, 18, "100000000000000000-----------------",  STATUS_SUCCESS},
    {16,   0xffffffff,  8,  9, "FFFFFFFF\0--------------------------", STATUS_SUCCESS},
    {16,   0xffffffff,  8,  8, "FFFFFFFF---------------------------",  STATUS_SUCCESS, 1}, /* No \0 term */
    {16,   0xffffffff,  8,  7, "-----------------------------------",  STATUS_BUFFER_OVERFLOW, 1}, /* Too short */
    {16,          0xa,  1,  2, "A\0---------------------------------", STATUS_SUCCESS},
    {16,          0xa,  1,  1, "A----------------------------------",  STATUS_SUCCESS, 1}, /* No \0 term */
    {16,            0,  1,  0, "-----------------------------------",  STATUS_BUFFER_OVERFLOW, 1},
    {20,   0xdeadbeef,  0,  9, "-----------------------------------",  STATUS_INVALID_PARAMETER}, /* ill. base */
    {-8,     07654321,  0, 12, "-----------------------------------",  STATUS_INVALID_PARAMETER}, /* neg. base */
};


static void one_RtlIntegerToUnicodeString_test(int test_num, const int2str_t *int2str)
{
    int pos;
    WCHAR expected_str_Buffer[STRI_BUFFER_LENGTH + 1];
    UNICODE_STRING expected_unicode_string;
    STRING expected_ansi_str;
    WCHAR str_Buffer[STRI_BUFFER_LENGTH + 1];
    UNICODE_STRING unicode_string;
    STRING ansi_str;
    NTSTATUS result;

    for (pos = 0; pos < STRI_BUFFER_LENGTH; pos++) {
	expected_str_Buffer[pos] = int2str->Buffer[pos];
    }
    expected_unicode_string.Length = int2str->Length * sizeof(WCHAR);
    expected_unicode_string.MaximumLength = int2str->MaximumLength * sizeof(WCHAR);
    expected_unicode_string.Buffer = expected_str_Buffer;
    pRtlUnicodeStringToAnsiString(&expected_ansi_str, &expected_unicode_string, 1);

    for (pos = 0; pos < STRI_BUFFER_LENGTH; pos++) {
	str_Buffer[pos] = '-';
    }
    unicode_string.Length = 0;
    unicode_string.MaximumLength = int2str->MaximumLength * sizeof(WCHAR);
    unicode_string.Buffer = str_Buffer;

    result = pRtlIntegerToUnicodeString(int2str->value, int2str->base, &unicode_string);
    pRtlUnicodeStringToAnsiString(&ansi_str, &unicode_string, 1);
    if (result == STATUS_BUFFER_OVERFLOW) {
	/* On BUFFER_OVERFLOW the string Buffer should be unchanged */
	for (pos = 0; pos < STRI_BUFFER_LENGTH; pos++) {
	    expected_str_Buffer[pos] = '-';
	}
	/* w2k: The native function has two reasons for BUFFER_OVERFLOW: */
	/* If the value is too large to convert: The Length is unchanged */
	/* If str is too small to hold the string: Set str->Length to the length */
	/* the string would have (which can be larger than the MaximumLength). */
	/* To allow all this in the tests we do the following: */
	if (expected_unicode_string.Length > 32 && unicode_string.Length == 0) {
	    /* The value is too large to convert only triggered when testing native */
	    expected_unicode_string.Length = 0;
	}
    } else {
	ok(result == int2str->result,
           "(test %d): RtlIntegerToUnicodeString(%lu, %d, [out]) has result %lx, expected: %lx\n",
	   test_num, int2str->value, int2str->base, result, int2str->result);
	if (result == STATUS_SUCCESS) {
	    ok(unicode_string.Buffer[unicode_string.Length/sizeof(WCHAR)] == '\0',
               "(test %d): RtlIntegerToUnicodeString(%lu, %d, [out]) string \"%s\" is not NULL terminated\n",
	       test_num, int2str->value, int2str->base, ansi_str.Buffer);
	}
    }
    ok(memcmp(unicode_string.Buffer, expected_unicode_string.Buffer, STRI_BUFFER_LENGTH * sizeof(WCHAR)) == 0,
       "(test %d): RtlIntegerToUnicodeString(%lu, %d, [out]) assigns string \"%s\", expected: \"%s\"\n",
       test_num, int2str->value, int2str->base, ansi_str.Buffer, expected_ansi_str.Buffer);
    ok(unicode_string.Length == expected_unicode_string.Length ||
       broken(int2str->broken_len && !unicode_string.Length) /* win11 */,
       "(test %d): RtlIntegerToUnicodeString(%lu, %d, [out]) string has Length %d, expected: %d\n",
       test_num, int2str->value, int2str->base, unicode_string.Length, expected_unicode_string.Length);
    ok(unicode_string.MaximumLength == expected_unicode_string.MaximumLength,
       "(test %d): RtlIntegerToUnicodeString(%lu, %d, [out]) string has MaximumLength %d, expected: %d\n",
       test_num, int2str->value, int2str->base, unicode_string.MaximumLength, expected_unicode_string.MaximumLength);
    pRtlFreeAnsiString(&expected_ansi_str);
    pRtlFreeAnsiString(&ansi_str);
}


static void test_RtlIntegerToUnicodeString(void)
{
    size_t test_num;

    for (test_num = 0; test_num < ARRAY_SIZE(int2str); test_num++)
        one_RtlIntegerToUnicodeString_test(test_num, &int2str[test_num]);
}


static void one_RtlIntegerToChar_test(int test_num, const int2str_t *int2str)
{
    NTSTATUS result;
    char dest_str[STRI_BUFFER_LENGTH + 1];

    memset(dest_str, '-', STRI_BUFFER_LENGTH);
    dest_str[STRI_BUFFER_LENGTH] = '\0';
    result = pRtlIntegerToChar(int2str->value, int2str->base, int2str->MaximumLength, dest_str);
    ok(result == int2str->result,
       "(test %d): RtlIntegerToChar(%lu, %d, %d, [out]) has result %lx, expected: %lx\n",
       test_num, int2str->value, int2str->base, int2str->MaximumLength, result, int2str->result);
    ok(memcmp(dest_str, int2str->Buffer, STRI_BUFFER_LENGTH) == 0,
       "(test %d): RtlIntegerToChar(%lu, %d, %d, [out]) assigns string \"%s\", expected: \"%s\"\n",
       test_num, int2str->value, int2str->base, int2str->MaximumLength, dest_str, int2str->Buffer);
}


static void test_RtlIntegerToChar(void)
{
    NTSTATUS result;
    size_t test_num;

    for (test_num = 0; test_num < ARRAY_SIZE(int2str); test_num++)
      one_RtlIntegerToChar_test(test_num, &int2str[test_num]);

    result = pRtlIntegerToChar(int2str[0].value, 20, int2str[0].MaximumLength, NULL);
    ok(result == STATUS_INVALID_PARAMETER,
       "(test a): RtlIntegerToChar(%lu, %d, %d, NULL) has result %lx, expected: %lx\n",
       int2str[0].value, 20, int2str[0].MaximumLength, result, STATUS_INVALID_PARAMETER);

    result = pRtlIntegerToChar(int2str[0].value, 20, 0, NULL);
    ok(result == STATUS_INVALID_PARAMETER,
       "(test b): RtlIntegerToChar(%lu, %d, %d, NULL) has result %lx, expected: %lx\n",
       int2str[0].value, 20, 0, result, STATUS_INVALID_PARAMETER);

    result = pRtlIntegerToChar(int2str[0].value, int2str[0].base, 0, NULL);
    ok(result == STATUS_BUFFER_OVERFLOW,
       "(test c): RtlIntegerToChar(%lu, %d, %d, NULL) has result %lx, expected: %lx\n",
       int2str[0].value, int2str[0].base, 0, result, STATUS_BUFFER_OVERFLOW);

    result = pRtlIntegerToChar(int2str[0].value, int2str[0].base, int2str[0].MaximumLength, NULL);
    ok(result == STATUS_ACCESS_VIOLATION,
       "(test d): RtlIntegerToChar(%lu, %d, %d, NULL) has result %lx, expected: %lx\n",
       int2str[0].value, int2str[0].base, int2str[0].MaximumLength, result, STATUS_ACCESS_VIOLATION);
}

static void test_RtlIsTextUnicode(void)
{
    char ascii[] = "A simple string";
    char false_positive[] = {0x41, 0x0a, 0x0d, 0x1d};
    WCHAR false_negative = 0x0d0a;
    WCHAR unicode[] = {'A',' ','U','n','i','c','o','d','e',' ','s','t','r','i','n','g',0};
    WCHAR unicode_no_controls[] = {'A','U','n','i','c','o','d','e','s','t','r','i','n','g',0};
    /* String with both byte-reversed and standard Unicode control characters. */
    WCHAR mixed_controls[] = {'\t',0x9000,0x0d00,'\n',0};
    WCHAR *be_unicode;
    WCHAR *be_unicode_no_controls;
    BOOLEAN res;
    int flags;
    int i;

    if (!pRtlIsTextUnicode)
    {
        win_skip("RtlIsTextUnicode is not available\n");
        return;
    }

    ok(!pRtlIsTextUnicode(ascii, sizeof(ascii), NULL), "ASCII text detected as Unicode\n");

    res = pRtlIsTextUnicode(unicode, sizeof(unicode), NULL);
    ok(res ||
       broken(res == FALSE), /* NT4 */
       "Text should be Unicode\n");

    ok(!pRtlIsTextUnicode(unicode, sizeof(unicode) - 1, NULL), "Text should be Unicode\n");

    flags =  IS_TEXT_UNICODE_UNICODE_MASK;
    ok(pRtlIsTextUnicode(unicode, sizeof(unicode), &flags), "Text should not pass a Unicode\n");
    ok(flags == (IS_TEXT_UNICODE_STATISTICS | IS_TEXT_UNICODE_CONTROLS),
       "Expected flags 0x6, obtained %x\n", flags);

    flags =  IS_TEXT_UNICODE_REVERSE_MASK;
    ok(!pRtlIsTextUnicode(unicode, sizeof(unicode), &flags), "Text should not pass reverse Unicode tests\n");
    ok(flags == 0, "Expected flags 0, obtained %x\n", flags);

    flags = IS_TEXT_UNICODE_ODD_LENGTH;
    ok(!pRtlIsTextUnicode(unicode, sizeof(unicode) - 1, &flags), "Odd length test should have passed\n");
    ok(flags == IS_TEXT_UNICODE_ODD_LENGTH, "Expected flags 0x200, obtained %x\n", flags);

    be_unicode = HeapAlloc(GetProcessHeap(), 0, sizeof(unicode) + sizeof(WCHAR));
    be_unicode[0] = 0xfffe;
    for (i = 0; i < ARRAY_SIZE(unicode); i++)
    {
        be_unicode[i + 1] = (unicode[i] >> 8) | ((unicode[i] & 0xff) << 8);
    }
    ok(!pRtlIsTextUnicode(be_unicode, sizeof(unicode) + 2, NULL), "Reverse endian should not be Unicode\n");
    ok(!pRtlIsTextUnicode(&be_unicode[1], sizeof(unicode), NULL), "Reverse endian should not be Unicode\n");

    flags = IS_TEXT_UNICODE_REVERSE_MASK;
    ok(!pRtlIsTextUnicode(&be_unicode[1], sizeof(unicode), &flags), "Reverse endian should be Unicode\n");
    todo_wine
    ok(flags == (IS_TEXT_UNICODE_REVERSE_ASCII16 | IS_TEXT_UNICODE_REVERSE_STATISTICS | IS_TEXT_UNICODE_REVERSE_CONTROLS),
       "Expected flags 0x70, obtained %x\n", flags);

    flags = IS_TEXT_UNICODE_REVERSE_MASK;
    ok(!pRtlIsTextUnicode(be_unicode, sizeof(unicode) + 2, &flags), "Reverse endian should be Unicode\n");
    ok(flags == (IS_TEXT_UNICODE_REVERSE_CONTROLS | IS_TEXT_UNICODE_REVERSE_SIGNATURE),
       "Expected flags 0xc0, obtained %x\n", flags);

    /* build byte reversed unicode string with no control chars */
    be_unicode_no_controls = HeapAlloc(GetProcessHeap(), 0, sizeof(unicode) + sizeof(WCHAR));
    ok(be_unicode_no_controls != NULL, "Expected HeapAlloc to succeed.\n");
    be_unicode_no_controls[0] = 0xfffe;
    for (i = 0; i < ARRAY_SIZE(unicode_no_controls); i++)
        be_unicode_no_controls[i + 1] = (unicode_no_controls[i] >> 8) | ((unicode_no_controls[i] & 0xff) << 8);


    /* The following tests verify that the tests for */
    /* IS_TEXT_UNICODE_CONTROLS and IS_TEXT_UNICODE_REVERSE_CONTROLS */
    /* are not mutually exclusive. Regardless of whether the strings */
    /* contain an indication of endianness, the tests are still */
    /* run if the flag is passed to (Rtl)IsTextUnicode. */

    /* Test IS_TEXT_UNICODE_CONTROLS flag */
    flags = IS_TEXT_UNICODE_CONTROLS;
    ok(!pRtlIsTextUnicode(unicode_no_controls, sizeof(unicode_no_controls), &flags), "Test should not pass on Unicode string lacking control characters.\n");
    ok(flags == 0, "Expected flags 0x0, obtained %x\n", flags);

    flags = IS_TEXT_UNICODE_CONTROLS;
    ok(!pRtlIsTextUnicode(be_unicode_no_controls, sizeof(unicode_no_controls), &flags), "Test should not pass on byte-reversed Unicode string lacking control characters.\n");
    ok(flags == 0, "Expected flags 0x0, obtained %x\n", flags);

    flags = IS_TEXT_UNICODE_CONTROLS;
    ok(pRtlIsTextUnicode(unicode, sizeof(unicode), &flags), "Test should pass on Unicode string lacking control characters.\n");
    ok(flags == IS_TEXT_UNICODE_CONTROLS, "Expected flags 0x04, obtained %x\n", flags);

    flags = IS_TEXT_UNICODE_CONTROLS;
    ok(!pRtlIsTextUnicode(be_unicode_no_controls, sizeof(unicode_no_controls) + 2, &flags),
            "Test should not pass with standard Unicode string.\n");
    ok(flags == 0, "Expected flags 0x0, obtained %x\n", flags);

    flags = IS_TEXT_UNICODE_CONTROLS;
    ok(pRtlIsTextUnicode(mixed_controls, sizeof(mixed_controls), &flags), "Test should pass on a string containing control characters.\n");
    ok(flags == IS_TEXT_UNICODE_CONTROLS, "Expected flags 0x04, obtained %x\n", flags);

    /* Test IS_TEXT_UNICODE_REVERSE_CONTROLS flag */
    flags = IS_TEXT_UNICODE_REVERSE_CONTROLS;
    ok(!pRtlIsTextUnicode(be_unicode_no_controls, sizeof(unicode_no_controls), &flags), "Test should not pass on Unicode string lacking control characters.\n");
    ok(flags == 0, "Expected flags 0x0, obtained %x\n", flags);

    flags = IS_TEXT_UNICODE_REVERSE_CONTROLS;
    ok(!pRtlIsTextUnicode(unicode_no_controls, sizeof(unicode_no_controls), &flags), "Test should not pass on Unicode string lacking control characters.\n");
    ok(flags == 0, "Expected flags 0x0, obtained %x\n", flags);

    flags = IS_TEXT_UNICODE_REVERSE_CONTROLS;
    ok(!pRtlIsTextUnicode(unicode, sizeof(unicode), &flags), "Test should not pass on Unicode string lacking control characters.\n");
    ok(flags == 0, "Expected flags 0x0, obtained %x\n", flags);

    flags = IS_TEXT_UNICODE_REVERSE_CONTROLS;
    ok(!pRtlIsTextUnicode(be_unicode, sizeof(unicode) + 2, &flags),
        "Test should pass with byte-reversed Unicode string containing control characters.\n");
    ok(flags == IS_TEXT_UNICODE_REVERSE_CONTROLS, "Expected flags 0x40, obtained %x\n", flags);

    flags = IS_TEXT_UNICODE_REVERSE_CONTROLS;
    ok(!pRtlIsTextUnicode(mixed_controls, sizeof(mixed_controls), &flags), "Test should pass on a string containing byte-reversed control characters.\n");
    ok(flags == IS_TEXT_UNICODE_REVERSE_CONTROLS, "Expected flags 0x40, obtained %x\n", flags);

    /* Test with flags for both byte-reverse and standard Unicode characters */
    flags = IS_TEXT_UNICODE_CONTROLS | IS_TEXT_UNICODE_REVERSE_CONTROLS;
    ok(!pRtlIsTextUnicode(mixed_controls, sizeof(mixed_controls), &flags), "Test should pass on string containing both byte-reversed and standard control characters.\n");
    ok(flags == (IS_TEXT_UNICODE_CONTROLS | IS_TEXT_UNICODE_REVERSE_CONTROLS), "Expected flags 0x44, obtained %x\n", flags);

    flags = IS_TEXT_UNICODE_STATISTICS;
    todo_wine ok(pRtlIsTextUnicode(false_positive, sizeof(false_positive), &flags), "Test should pass on false positive.\n");

    ok(!pRtlIsTextUnicode(&false_negative, sizeof(false_negative), NULL), "Test should fail on 0x0d0a (MALAYALAM LETTER UU).\n");

    HeapFree(GetProcessHeap(), 0, be_unicode);
    HeapFree(GetProcessHeap(), 0, be_unicode_no_controls);
}

static void test_RtlCompareUnicodeString(void)
{
    WCHAR ch1, ch2;
    UNICODE_STRING str1, str2;

    str1.Buffer = &ch1;
    str1.Length = str1.MaximumLength = sizeof(WCHAR);
    str2.Buffer = &ch2;
    str2.Length = str2.MaximumLength = sizeof(WCHAR);
    for (ch1 = 0; ch1 < 512; ch1++)
    {
        for (ch2 = 0; ch2 < 1024; ch2++)
        {
            LONG res = pRtlCompareUnicodeString( &str1, &str2, FALSE );
            ok( res == (ch1 - ch2), "wrong result %ld %04x %04x\n", res, ch1, ch2 );
            res = pRtlCompareUnicodeString( &str1, &str2, TRUE );
            ok( res == (pRtlUpcaseUnicodeChar(ch1) - pRtlUpcaseUnicodeChar(ch2)),
                "wrong result %ld %04x %04x\n", res, ch1, ch2 );
            if (pRtlCompareUnicodeStrings)
            {
                res = pRtlCompareUnicodeStrings( &ch1, 1, &ch2, 1, FALSE );
                ok( res == (ch1 - ch2), "wrong result %ld %04x %04x\n", res, ch1, ch2 );
                res = pRtlCompareUnicodeStrings( &ch1, 1, &ch2, 1, TRUE );
                ok( res == (pRtlUpcaseUnicodeChar(ch1) - pRtlUpcaseUnicodeChar(ch2)),
                    "wrong result %ld %04x %04x\n", res, ch1, ch2 );
            }
        }
    }
}

static const WCHAR szGuid[] = { '{','0','1','0','2','0','3','0','4','-',
  '0','5','0','6','-'  ,'0','7','0','8','-','0','9','0','A','-',
  '0','B','0','C','0','D','0','E','0','F','0','A','}','\0' };
static const WCHAR szGuid2[] = { '{','0','1','0','2','0','3','0','4','-',
  '0','5','0','6','-'  ,'0','7','0','8','-','0','9','0','A','-',
  '0','B','0','C','0','D','0','E','0','F','0','A',']','\0' };
DEFINE_GUID(IID_Endianness, 0x01020304, 0x0506, 0x0708, 0x09, 0x0A, 0x0B,
            0x0C, 0x0D, 0x0E, 0x0F, 0x0A);

static void test_RtlGUIDFromString(void)
{
  GUID guid;
  UNICODE_STRING str;
  NTSTATUS ret;

  if (!pRtlGUIDFromString)
  {
      win_skip("RtlGUIDFromString is not available\n");
      return;
  }

  str.Length = str.MaximumLength = sizeof(szGuid) - sizeof(WCHAR);
  str.Buffer = (LPWSTR)szGuid;

  ret = pRtlGUIDFromString(&str, &guid);
  ok(ret == 0, "expected ret=0, got 0x%0lx\n", ret);
  ok(IsEqualGUID(&guid, &IID_Endianness), "Endianness broken\n");

  str.Length = str.MaximumLength = sizeof(szGuid2) - sizeof(WCHAR);
  str.Buffer = (LPWSTR)szGuid2;

  ret = pRtlGUIDFromString(&str, &guid);
  ok(ret, "expected ret!=0\n");
}

static void test_RtlStringFromGUID(void)
{
  UNICODE_STRING str;
  NTSTATUS ret;

  if (!pRtlStringFromGUID)
  {
      win_skip("RtlStringFromGUID is not available\n");
      return;
  }

  str.Length = str.MaximumLength = 0;
  str.Buffer = NULL;

  ret = pRtlStringFromGUID(&IID_Endianness, &str);
  ok(ret == 0, "expected ret=0, got 0x%0lx\n", ret);
  ok(str.Buffer && !lstrcmpiW(str.Buffer, szGuid), "Endianness broken\n");
  pRtlFreeUnicodeString(&str);
}

struct hash_unicodestring_test {
    WCHAR str[50];
    BOOLEAN case_insensitive;
    ULONG hash;
};

static const struct hash_unicodestring_test hash_test[] = {
    { L"T",         FALSE, 0x00000054 },
    { L"Test",      FALSE, 0x766bb952 },
    { L"TeSt",      FALSE, 0x764bb172 },
    { L"test",      FALSE, 0x4745d132 },
    { L"test",      TRUE,  0x6689c132 },
    { L"TEST",      TRUE,  0x6689c132 },
    { L"TEST",      FALSE, 0x6689c132 },
    { L"t\xe9st",   FALSE, 0x8845cfb6 },
    { L"t\xe9st",   TRUE,  0xa789bfb6 },
    { L"T\xc9ST",   TRUE,  0xa789bfb6 },
    { L"T\xc9ST",   FALSE, 0xa789bfb6 },
    { L"abcdef",    FALSE, 0x971318c3 },
    { { 0 } }
};

static void test_RtlHashUnicodeString(void)
{
    static const WCHAR strW[] = {'T','e','s','t',0,'1',0};
    const struct hash_unicodestring_test *ptr;
    UNICODE_STRING str;
    NTSTATUS status;
    ULONG hash;

    if (!pRtlHashUnicodeString)
    {
        win_skip("RtlHashUnicodeString is not available\n");
        return;
    }

    status = pRtlHashUnicodeString(NULL, FALSE, HASH_STRING_ALGORITHM_X65599, &hash);
    ok(status == STATUS_INVALID_PARAMETER, "got status 0x%08lx\n", status);

    pRtlInitUnicodeString(&str, strW);
    status = pRtlHashUnicodeString(&str, FALSE, HASH_STRING_ALGORITHM_X65599, NULL);
    ok(status == STATUS_INVALID_PARAMETER, "got status 0x%08lx\n", status);

    status = pRtlHashUnicodeString(&str, FALSE, HASH_STRING_ALGORITHM_INVALID, &hash);
    ok(status == STATUS_INVALID_PARAMETER, "got status 0x%08lx\n", status);

    /* embedded null */
    str.Buffer = (PWSTR)strW;
    str.Length = sizeof(strW) - sizeof(WCHAR);
    str.MaximumLength = sizeof(strW);
    status = pRtlHashUnicodeString(&str, FALSE, HASH_STRING_ALGORITHM_X65599, &hash);
    ok(status == STATUS_SUCCESS, "got status 0x%08lx\n", status);
    ok(hash == 0x32803083, "got 0x%08lx\n", hash);

    ptr = hash_test;
    while (*ptr->str)
    {
        pRtlInitUnicodeString(&str, ptr->str);
        hash = 0;
        status = pRtlHashUnicodeString(&str, ptr->case_insensitive, HASH_STRING_ALGORITHM_X65599, &hash);
        ok(status == STATUS_SUCCESS, "got status 0x%08lx for %s\n", status, wine_dbgstr_w(ptr->str));
        ok(hash == ptr->hash, "got wrong hash 0x%08lx, expected 0x%08lx, for %s, mode %d\n", hash, ptr->hash,
            wine_dbgstr_w(ptr->str), ptr->case_insensitive);

        ptr++;
    }
}

struct unicode_to_utf8_test {
    WCHAR unicode[128];
    const char *expected;
    NTSTATUS status;
};

static const struct unicode_to_utf8_test unicode_to_utf8[] = {
    { { 0 }, "", STATUS_SUCCESS },
    { { '-',0 }, "-", STATUS_SUCCESS },
    { { 'h','e','l','l','o',0 }, "hello", STATUS_SUCCESS },
    { { '-',0x7f,'-',0x80,'-',0xff,'-',0x100,'-',0 }, "-\x7F-\xC2\x80-\xC3\xBF-\xC4\x80-", STATUS_SUCCESS },
    { { '-',0x7ff,'-',0x800,'-',0 }, "-\xDF\xBF-\xE0\xA0\x80-", STATUS_SUCCESS },
    { { '-',0xd7ff,'-',0xe000,'-',0 }, "-\xED\x9F\xBF-\xEE\x80\x80-", STATUS_SUCCESS },
                       /* 0x10000 */
    { { '-',0xffff,'-',0xd800,0xdc00,'-',0 }, "-\xEF\xBF\xBF-\xF0\x90\x80\x80-", STATUS_SUCCESS },
            /* 0x103ff */     /* 0x10400 */
    { { '-',0xd800,0xdfff,'-',0xd801,0xdc00,'-',0 }, "-\xF0\x90\x8F\xBF-\xF0\x90\x90\x80-", STATUS_SUCCESS },
            /* 0x10ffff */
    { { '-',0xdbff,0xdfff,'-',0 }, "-\xF4\x8F\xBF\xBF-", STATUS_SUCCESS },
    /* standalone lead surrogates become 0xFFFD */
    { { '-',0xd800,'-',0xdbff,'-',0 }, "-\xEF\xBF\xBD-\xEF\xBF\xBD-", STATUS_SOME_NOT_MAPPED },
    /* standalone trail surrogates become 0xFFFD */
    { { '-',0xdc00,'-',0xdfff,'-',0 }, "-\xEF\xBF\xBD-\xEF\xBF\xBD-", STATUS_SOME_NOT_MAPPED },
    /* reverse surrogate pair */
    { { '-',0xdfff,0xdbff,'-',0 }, "-\xEF\xBF\xBD\xEF\xBF\xBD-", STATUS_SOME_NOT_MAPPED },
    /* byte order marks */
    { { '-',0xfeff,'-',0xfffe,'-',0 }, "-\xEF\xBB\xBF-\xEF\xBF\xBE-", STATUS_SUCCESS },
    { { 0xfeff,'-',0 }, "\xEF\xBB\xBF-", STATUS_SUCCESS },
    { { 0xfffe,'-',0 }, "\xEF\xBF\xBE-", STATUS_SUCCESS },
    /* invalid code points */
    { { 0xfffd, '-', 0xfffe, '-', 0xffff,'-',0 }, "\xEF\xBF\xBD-\xEF\xBF\xBE-\xEF\xBF\xBF-", STATUS_SUCCESS },
    /* canonically equivalent representations -- no normalization should happen */
    { { '-',0x1e09,'-',0 }, "-\xE1\xB8\x89-", STATUS_SUCCESS },
    { { '-',0x0107,0x0327,'-',0 }, "-\xC4\x87\xCC\xA7-", STATUS_SUCCESS },
    { { '-',0x00e7,0x0301,'-',0 }, "-\xC3\xA7\xCC\x81-", STATUS_SUCCESS },
    { { '-',0x0063,0x0327,0x0301,'-',0 }, "-\x63\xCC\xA7\xCC\x81-", STATUS_SUCCESS },
    { { '-',0x0063,0x0301,0x0327,'-',0 }, "-\x63\xCC\x81\xCC\xA7-", STATUS_SUCCESS },
};

static void utf8_expect_(const unsigned char *out_string, ULONG buflen, ULONG out_bytes,
                         const WCHAR *in_string, ULONG in_bytes,
                         NTSTATUS expect_status, int line)
{
    NTSTATUS status;
    ULONG bytes_out;
    char buffer[128];
    unsigned char *buf = (unsigned char *)buffer;
    unsigned int i;

    if (buflen == (ULONG)-1)
        buflen = sizeof(buffer);
    bytes_out = 0x55555555;
    memset(buffer, 0x55, sizeof(buffer));
    status = pRtlUnicodeToUTF8N(
        out_string ? buffer : NULL, buflen, &bytes_out,
        in_string, in_bytes);
    ok_(__FILE__, line)(status == expect_status, "status 0x%lx, expected 0x%lx\n", status, expect_status);
    ok_(__FILE__, line)(bytes_out == out_bytes, "bytes_out = %lu, expected %lu\n", bytes_out, out_bytes);
    if (out_string)
    {
        for (i = 0; i < bytes_out; i++)
            ok_(__FILE__, line)(buf[i] == out_string[i],
                                "buffer[%d] = 0x%x, expected 0x%x\n",
                                i, buf[i], out_string[i]);
        for (; i < sizeof(buffer); i++)
            ok_(__FILE__, line)(buf[i] == 0x55,
                                "buffer[%d] = 0x%x, expected 0x55\n",
                                i, buf[i]);
    }
}
#define utf8_expect(out_string, buflen, out_bytes, in_string, in_bytes, expect_status) \
        utf8_expect_(out_string, buflen, out_bytes, in_string, in_bytes, expect_status, __LINE__)

static void test_RtlUnicodeToUTF8N(void)
{
    NTSTATUS status;
    ULONG bytes_out;
    ULONG bytes_out_array[2];
    void * const invalid_pointer = (void *)0x8;
    char buffer[128];
    const WCHAR empty_string[] = { 0 };
    const WCHAR test_string[] = { 'A',0,'a','b','c','d','e','f','g',0 };
    const WCHAR special_string[] = { 'X',0x80,0xd800,0 };
    const ULONG special_string_len[] = { 0, 1, 1, 3, 3, 3, 6, 7 };
    const unsigned char special_expected[] = { 'X',0xc2,0x80,0xef,0xbf,0xbd,0 };
    unsigned int input_len;
    const unsigned int test_count = ARRAY_SIZE(unicode_to_utf8);
    unsigned int i, ret;

    if (!pRtlUnicodeToUTF8N)
    {
        win_skip("RtlUnicodeToUTF8N is not available\n");
        return;
    }

    /* show that bytes_out is really ULONG */
    memset(bytes_out_array, 0x55, sizeof(bytes_out_array));
    status = pRtlUnicodeToUTF8N(NULL, 0, bytes_out_array, empty_string, 0);
    ok(status == STATUS_SUCCESS, "status = 0x%lx\n", status);
    ok(bytes_out_array[0] == 0x00000000, "Got 0x%lx\n", bytes_out_array[0]);
    ok(bytes_out_array[1] == 0x55555555, "Got 0x%lx\n", bytes_out_array[1]);

    /* parameter checks */
    status = pRtlUnicodeToUTF8N(NULL, 0, NULL, NULL, 0);
    ok(status == STATUS_INVALID_PARAMETER_4, "status = 0x%lx\n", status);

    status = pRtlUnicodeToUTF8N(NULL, 0, NULL, empty_string, 0);
    ok(status == STATUS_INVALID_PARAMETER, "status = 0x%lx\n", status);

    bytes_out = 0x55555555;
    status = pRtlUnicodeToUTF8N(NULL, 0, &bytes_out, NULL, 0);
    ok(status == STATUS_INVALID_PARAMETER_4, "status = 0x%lx\n", status);
    ok(bytes_out == 0x55555555, "bytes_out = 0x%lx\n", bytes_out);

    bytes_out = 0x55555555;
    status = pRtlUnicodeToUTF8N(NULL, 0, &bytes_out, invalid_pointer, 0);
    ok(status == STATUS_SUCCESS, "status = 0x%lx\n", status);
    ok(bytes_out == 0, "bytes_out = 0x%lx\n", bytes_out);

    bytes_out = 0x55555555;
    status = pRtlUnicodeToUTF8N(NULL, 0, &bytes_out, empty_string, 0);
    ok(status == STATUS_SUCCESS, "status = 0x%lx\n", status);
    ok(bytes_out == 0, "bytes_out = 0x%lx\n", bytes_out);

    bytes_out = 0x55555555;
    status = pRtlUnicodeToUTF8N(NULL, 0, &bytes_out, test_string, 0);
    ok(status == STATUS_SUCCESS, "status = 0x%lx\n", status);
    ok(bytes_out == 0, "bytes_out = 0x%lx\n", bytes_out);

    bytes_out = 0x55555555;
    status = pRtlUnicodeToUTF8N(NULL, 0, &bytes_out, empty_string, 1);
    ok(status == STATUS_SUCCESS, "status = 0x%lx\n", status);
    ok(bytes_out == 0, "bytes_out = 0x%lx\n", bytes_out);

    bytes_out = 0x55555555;
    status = pRtlUnicodeToUTF8N(invalid_pointer, 0, &bytes_out, empty_string, 1);
    ok(status == STATUS_INVALID_PARAMETER_5, "status = 0x%lx\n", status);
    ok(bytes_out == 0x55555555, "bytes_out = 0x%lx\n", bytes_out);

    bytes_out = 0x55555555;
    status = pRtlUnicodeToUTF8N(invalid_pointer, 8, &bytes_out, empty_string, 1);
    ok(status == STATUS_INVALID_PARAMETER_5, "status = 0x%lx\n", status);
    ok(bytes_out == 0x55555555, "bytes_out = 0x%lx\n", bytes_out);

    /* length output with special chars */
#define length_expect(in_chars, out_bytes, expect_status) \
        utf8_expect_(NULL, 0, out_bytes, \
                     special_string, in_chars * sizeof(WCHAR), \
                     expect_status, __LINE__)

    length_expect(0, 0, STATUS_SUCCESS);
    length_expect(1, 1, STATUS_SUCCESS);
    length_expect(2, 3, STATUS_SUCCESS);
    length_expect(3, 6, STATUS_SOME_NOT_MAPPED);
    length_expect(4, 7, STATUS_SOME_NOT_MAPPED);
#undef length_expect

    for (i = 0; i <= 6; i++)
    {
        memset(buffer, 0x55, sizeof(buffer));
        bytes_out = 0xdeadbeef;
        status = pRtlUnicodeToUTF8N(buffer, i, &bytes_out, special_string, sizeof(special_string));
        ok(status == STATUS_BUFFER_TOO_SMALL, "%d: status = 0x%lx\n", i, status);
        ok(bytes_out == special_string_len[i], "%d: expected %lu, got %lu\n", i, special_string_len[i], bytes_out);
        ok(memcmp(buffer, special_expected, special_string_len[i]) == 0, "%d: bad conversion\n", i);
    }

    status = pRtlUnicodeToUTF8N(buffer, 7, &bytes_out, special_string, sizeof(special_string));
    ok(status == STATUS_SOME_NOT_MAPPED, "status = 0x%lx\n", status);
    ok(bytes_out == special_string_len[7], "expected %lu, got %lu\n", special_string_len[7], bytes_out);
    ok(memcmp(buffer, special_expected, 7) == 0, "bad conversion\n");

    /* conversion behavior with varying input length */
    for (input_len = 0; input_len <= sizeof(test_string); input_len++) {
        /* no output buffer, just length */
        utf8_expect(NULL, 0, input_len / sizeof(WCHAR),
                    test_string, input_len, STATUS_SUCCESS);

        /* write output */
        bytes_out = 0x55555555;
        memset(buffer, 0x55, sizeof(buffer));
        status = pRtlUnicodeToUTF8N(
            buffer, sizeof(buffer), &bytes_out,
            test_string, input_len);
        if (input_len % sizeof(WCHAR) == 0) {
            ok(status == STATUS_SUCCESS,
               "(len %u): status = 0x%lx\n", input_len, status);
            ok(bytes_out == input_len / sizeof(WCHAR),
               "(len %u): bytes_out = 0x%lx\n", input_len, bytes_out);
            for (i = 0; i < bytes_out; i++) {
                ok(buffer[i] == test_string[i],
                   "(len %u): buffer[%d] = 0x%x, expected 0x%x\n",
                   input_len, i, buffer[i], test_string[i]);
            }
            for (; i < sizeof(buffer); i++) {
                ok(buffer[i] == 0x55,
                   "(len %u): buffer[%d] = 0x%x\n", input_len, i, buffer[i]);
            }
        } else {
            ok(status == STATUS_INVALID_PARAMETER_5,
               "(len %u): status = 0x%lx\n", input_len, status);
            ok(bytes_out == 0x55555555,
               "(len %u): bytes_out = 0x%lx\n", input_len, bytes_out);
            for (i = 0; i < sizeof(buffer); i++) {
                ok(buffer[i] == 0x55,
                   "(len %u): buffer[%d] = 0x%x\n", input_len, i, buffer[i]);
            }
        }
    }

    /* test cases for special characters */
    for (i = 0; i < test_count; i++) {
        bytes_out = 0x55555555;
        memset(buffer, 0x55, sizeof(buffer));
        status = pRtlUnicodeToUTF8N(
            buffer, sizeof(buffer), &bytes_out,
            unicode_to_utf8[i].unicode, lstrlenW(unicode_to_utf8[i].unicode) * sizeof(WCHAR));
        ok(status == unicode_to_utf8[i].status,
           "(test %d): status is 0x%lx, expected 0x%lx\n",
           i, status, unicode_to_utf8[i].status);
        ok(bytes_out == strlen(unicode_to_utf8[i].expected),
           "(test %d): bytes_out is %lu, expected %u\n",
           i, bytes_out, lstrlenA(unicode_to_utf8[i].expected));
        ok(!memcmp(buffer, unicode_to_utf8[i].expected, bytes_out),
           "(test %d): got \"%.*s\", expected \"%s\"\n",
           i, (int)bytes_out, buffer, unicode_to_utf8[i].expected);
        ok(buffer[bytes_out] == 0x55,
           "(test %d): behind string: 0x%x\n", i, buffer[bytes_out]);
        memset(buffer, 0x55, sizeof(buffer));
        ret = WideCharToMultiByte( CP_UTF8, 0, unicode_to_utf8[i].unicode, lstrlenW(unicode_to_utf8[i].unicode),
                                   buffer, sizeof(buffer), NULL, NULL );
        ok( ret == strlen(unicode_to_utf8[i].expected), "(test %d): wrong len %u\n", i, ret );
        ok(!memcmp(buffer, unicode_to_utf8[i].expected, ret),
           "(test %d): got \"%.*s\", expected \"%s\"\n",
           i, ret, buffer, unicode_to_utf8[i].expected);
        ok(buffer[ret] == 0x55, "(test %d): behind string: 0x%x\n", i, buffer[ret]);

        /* same test but include the null terminator */
        bytes_out = 0x55555555;
        memset(buffer, 0x55, sizeof(buffer));
        status = pRtlUnicodeToUTF8N(
            buffer, sizeof(buffer), &bytes_out,
            unicode_to_utf8[i].unicode, (lstrlenW(unicode_to_utf8[i].unicode) + 1) * sizeof(WCHAR));
        ok(status == unicode_to_utf8[i].status,
           "(test %d): status is 0x%lx, expected 0x%lx\n",
           i, status, unicode_to_utf8[i].status);
        ok(bytes_out == strlen(unicode_to_utf8[i].expected) + 1,
           "(test %d): bytes_out is %lu, expected %u\n",
           i, bytes_out, lstrlenA(unicode_to_utf8[i].expected) + 1);
        ok(!memcmp(buffer, unicode_to_utf8[i].expected, bytes_out),
           "(test %d): got \"%.*s\", expected \"%s\"\n",
           i, (int)bytes_out, buffer, unicode_to_utf8[i].expected);
        ok(buffer[bytes_out] == 0x55,
           "(test %d): behind string: 0x%x\n", i, buffer[bytes_out]);
        memset(buffer, 0x55, sizeof(buffer));
        ret = WideCharToMultiByte( CP_UTF8, 0, unicode_to_utf8[i].unicode, -1, buffer, sizeof(buffer), NULL, NULL );
        ok( ret == strlen(unicode_to_utf8[i].expected) + 1, "(test %d): wrong len %u\n", i, ret );
        ok(!memcmp(buffer, unicode_to_utf8[i].expected, ret),
           "(test %d): got \"%.*s\", expected \"%s\"\n",
           i, ret, buffer, unicode_to_utf8[i].expected);
        ok(buffer[ret] == 0x55, "(test %d): behind string: 0x%x\n", i, buffer[ret]);
        SetLastError( 0xdeadbeef );
        memset(buffer, 0x55, sizeof(buffer));
        ret = WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS, unicode_to_utf8[i].unicode, -1,
                                   buffer, sizeof(buffer), NULL, NULL );
        if (unicode_to_utf8[i].status == STATUS_SOME_NOT_MAPPED)
        {
            ok( ret == 0, "(test %d): wrong len %u\n", i, ret );
            ok( GetLastError() == ERROR_NO_UNICODE_TRANSLATION, "(test %d): wrong error %lu\n", i, GetLastError() );
            ret = strlen(unicode_to_utf8[i].expected) + 1;
        }
        else
            ok( ret == strlen(unicode_to_utf8[i].expected) + 1, "(test %d): wrong len %u\n", i, ret );

        ok(!memcmp(buffer, unicode_to_utf8[i].expected, ret),
           "(test %d): got \"%.*s\", expected \"%s\"\n",
           i, ret, buffer, unicode_to_utf8[i].expected);
        ok(buffer[ret] == 0x55, "(test %d): behind string: 0x%x\n", i, buffer[ret]);
    }
}

struct utf8_to_unicode_test {
    const char *utf8;
    WCHAR expected[128];
    NTSTATUS status;
};

static const struct utf8_to_unicode_test utf8_to_unicode[] = {
    { "", { 0 }, STATUS_SUCCESS },
    { "-", { '-',0 }, STATUS_SUCCESS },
    { "hello", { 'h','e','l','l','o',0 }, STATUS_SUCCESS },
    /* first and last of each range */
    { "-\x7F-\xC2\x80-\xC3\xBF-\xC4\x80-", { '-',0x7f,'-',0x80,'-',0xff,'-',0x100,'-',0 }, STATUS_SUCCESS },
    { "-\xDF\xBF-\xE0\xA0\x80-", { '-',0x7ff,'-',0x800,'-',0 }, STATUS_SUCCESS },
    { "-\xED\x9F\xBF-\xEE\x80\x80-", { '-',0xd7ff,'-',0xe000,'-',0 }, STATUS_SUCCESS },
                     /*   0x10000  */
    { "-\xEF\xBF\xBF-\xF0\x90\x80\x80-", { '-',0xffff,'-',0xd800,0xdc00,'-',0 }, STATUS_SUCCESS },
        /*   0x103ff  */ /*   0x10400  */
    { "-\xF0\x90\x8F\xBF-\xF0\x90\x90\x80-", { '-',0xd800,0xdfff,'-',0xd801,0xdc00,'-',0 }, STATUS_SUCCESS },
        /*  0x10ffff  */
    { "-\xF4\x8F\xBF\xBF-", { '-',0xdbff,0xdfff,'-',0 }, STATUS_SUCCESS },
    /* standalone surrogate code points */
        /* 0xd800 */ /* 0xdbff */
    { "-\xED\xA0\x80-\xED\xAF\xBF-", { '-',0xfffd,0xfffd,'-',0xfffd,0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
        /* 0xdc00 */ /* 0xdfff */
    { "-\xED\xB0\x80-\xED\xBF\xBF-", { '-',0xfffd,0xfffd,'-',0xfffd,0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
    /* UTF-8 encoded surrogate pair */
        /* 0xdbff *//* 0xdfff */
    { "-\xED\xAF\xBF\xED\xBF\xBF-", { '-',0xfffd,0xfffd,0xfffd,0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
    /* reverse surrogate pair */
        /* 0xdfff *//* 0xdbff */
    { "-\xED\xBF\xBF\xED\xAF\xBF-", { '-',0xfffd,0xfffd,0xfffd,0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
    /* code points outside the UTF-16 range */
        /*  0x110000  */
    { "-\xF4\x90\x80\x80-", { '-',0xfffd,0xfffd,0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
        /*  0x1fffff  */
    { "-\xF7\xBF\xBF\xBF-", { '-',0xfffd,0xfffd,0xfffd,0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
        /*     0x200000   */
    { "-\xFA\x80\x80\x80\x80-", { '-',0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
        /*    0x3ffffff   */
    { "-\xFB\xBF\xBF\xBF\xBF-", { '-',0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
        /*      0x4000000     */
    { "-\xFC\x84\x80\x80\x80\x80-", { '-',0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
        /*     0x7fffffff     */
    { "-\xFD\xBF\xBF\xBF\xBF\xBF-", { '-',0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
    /* overlong encodings of each length for -, NUL, and the highest possible value */
    { "-\xC0\xAD-\xC0\x80-\xC1\xBF-", { '-',0xfffd,0xfffd,'-',0xfffd,0xfffd,'-',0xfffd,0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
    { "-\xE0\x80\xAD-\xE0\x80\x80-\xE0\x9F\xBF-", { '-',0xfffd,0xfffd,'-',0xfffd,0xfffd,'-',0xfffd,0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
    { "-\xF0\x80\x80\xAD-", { '-',0xfffd,0xfffd,0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
    { "-\xF0\x80\x80\x80-", { '-',0xfffd,0xfffd,0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
    { "-\xF0\x8F\xBF\xBF-", { '-',0xfffd,0xfffd,0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
    { "-\xF8\x80\x80\x80\xAD-", { '-',0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
    { "-\xF8\x80\x80\x80\x80-", { '-',0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
    { "-\xF8\x87\xBF\xBF\xBF-", { '-',0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
    { "-\xFC\x80\x80\x80\x80\xAD-", { '-',0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
    { "-\xFC\x80\x80\x80\x80\x80-", { '-',0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
    { "-\xFC\x83\xBF\xBF\xBF\xBF-", { '-',0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
    /* invalid bytes */
    { "\xFE", { 0xfffd,0 }, STATUS_SOME_NOT_MAPPED },
    { "\xFF", { 0xfffd,0 }, STATUS_SOME_NOT_MAPPED },
    { "\xFE\xBF\xBF\xBF\xBF\xBF\xBF\xBF\xBF", { 0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0 }, STATUS_SOME_NOT_MAPPED },
    { "\xFF\xBF\xBF\xBF\xBF\xBF\xBF\xBF\xBF", { 0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0 }, STATUS_SOME_NOT_MAPPED },
    { "\xFF\x80\x80\x80\x80\x80\x80\x80\x80", { 0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0 }, STATUS_SOME_NOT_MAPPED },
    { "\xFF\x40\x80\x80\x80\x80\x80\x80\x80", { 0xfffd,0x40,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0 }, STATUS_SOME_NOT_MAPPED },
    /* lone continuation bytes */
    { "\x80", { 0xfffd,0 }, STATUS_SOME_NOT_MAPPED },
    { "\x80\x80", { 0xfffd,0xfffd,0 }, STATUS_SOME_NOT_MAPPED },
    { "\xBF", { 0xfffd,0 }, STATUS_SOME_NOT_MAPPED },
    { "\xBF\xBF", { 0xfffd,0xfffd,0 }, STATUS_SOME_NOT_MAPPED },
    /* incomplete sequences */
    { "\xC2-", { 0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
    { "\xE0\xA0-", { 0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
    { "\xF0\x90\x80-", { 0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
    { "\xF4\x8F\xBF-", { 0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
    { "\xFA\x80\x80\x80-", { 0xfffd,0xfffd,0xfffd,0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
    { "\xFC\x84\x80\x80\x80-", { 0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
    /* multibyte sequence followed by lone continuation byte */
    { "\xE0\xA0\x80\x80-", { 0x800,0xfffd,'-',0 }, STATUS_SOME_NOT_MAPPED },
    /* byte order marks */
    { "-\xEF\xBB\xBF-\xEF\xBF\xBE-", { '-',0xfeff,'-',0xfffe,'-',0 }, STATUS_SUCCESS },
    { "\xEF\xBB\xBF-", { 0xfeff,'-',0 }, STATUS_SUCCESS },
    { "\xEF\xBF\xBE-", { 0xfffe,'-',0 }, STATUS_SUCCESS },
    /* invalid code points */
    { "\xEF\xBF\xBD-\xEF\xBF\xBE-\xEF\xBF\xBF-", { 0xfffd,'-',0xfffe,'-',0xffff,'-',0 }, STATUS_SUCCESS },
    /* canonically equivalent representations -- no normalization should happen */
    { "-\xE1\xB8\x89-", { '-',0x1e09,'-',0 }, STATUS_SUCCESS },
    { "-\xC4\x87\xCC\xA7-", { '-',0x0107,0x0327,'-',0 }, STATUS_SUCCESS },
    { "-\xC3\xA7\xCC\x81-", { '-',0x00e7,0x0301,'-',0 }, STATUS_SUCCESS },
    { "-\x63\xCC\xA7\xCC\x81-", { '-',0x0063,0x0327,0x0301,'-',0 }, STATUS_SUCCESS },
    { "-\x63\xCC\x81\xCC\xA7-", { '-',0x0063,0x0301,0x0327,'-',0 }, STATUS_SUCCESS },
};

static void unicode_expect_(const WCHAR *out_string, ULONG buflen, ULONG out_chars,
                            const char *in_string, ULONG in_chars,
                            NTSTATUS expect_status, int line)
{
    NTSTATUS status;
    ULONG bytes_out;
    WCHAR buffer[128];
    unsigned int i;

    if (buflen == (ULONG)-1)
        buflen = sizeof(buffer);
    bytes_out = 0x55555555;
    memset(buffer, 0x55, sizeof(buffer));
    status = pRtlUTF8ToUnicodeN(
        out_string ? buffer : NULL, buflen, &bytes_out,
        in_string, in_chars);
    ok_(__FILE__, line)(status == expect_status, "status = 0x%lx\n", status);
    ok_(__FILE__, line)(bytes_out == out_chars * sizeof(WCHAR),
                        "bytes_out = %lu, expected %lu\n", bytes_out, out_chars * (ULONG)sizeof(WCHAR));
    if (out_string)
    {
        for (i = 0; i < bytes_out / sizeof(WCHAR); i++)
            ok_(__FILE__, line)(buffer[i] == out_string[i],
                                "buffer[%d] = 0x%x, expected 0x%x\n",
                                i, buffer[i], out_string[i]);
        for (; i < ARRAY_SIZE(buffer); i++)
            ok_(__FILE__, line)(buffer[i] == 0x5555,
                                "buffer[%d] = 0x%x, expected 0x5555\n",
                                i, buffer[i]);
    }
}
#define unicode_expect(out_string, buflen, out_chars, in_string, in_chars, expect_status) \
        unicode_expect_(out_string, buflen, out_chars, in_string, in_chars, expect_status, __LINE__)

static void test_RtlUTF8ToUnicodeN(void)
{
    NTSTATUS status;
    ULONG bytes_out;
    ULONG bytes_out_array[2];
    void * const invalid_pointer = (void *)0x8;
    WCHAR buffer[128];
    const char empty_string[] = "";
    const char test_string[] = "A\0abcdefg";
    const WCHAR test_stringW[] = {'A',0,'a','b','c','d','e','f','g',0 };
    const char special_string[] = { 'X',0xc2,0x80,0xF0,0x90,0x80,0x80,0 };
    const WCHAR special_expected[] = { 'X',0x80,0xd800,0xdc00,0 };
    unsigned int input_len;
    const unsigned int test_count = ARRAY_SIZE(utf8_to_unicode);
    unsigned int i, ret;

    if (!pRtlUTF8ToUnicodeN)
    {
        win_skip("RtlUTF8ToUnicodeN is not available\n");
        return;
    }

    /* show that bytes_out is really ULONG */
    memset(bytes_out_array, 0x55, sizeof(bytes_out_array));
    status = pRtlUTF8ToUnicodeN(NULL, 0, bytes_out_array, empty_string, 0);
    ok(status == STATUS_SUCCESS, "status = 0x%lx\n", status);
    ok(bytes_out_array[0] == 0x00000000, "Got 0x%lx\n", bytes_out_array[0]);
    ok(bytes_out_array[1] == 0x55555555, "Got 0x%lx\n", bytes_out_array[1]);

    /* parameter checks */
    status = pRtlUTF8ToUnicodeN(NULL, 0, NULL, NULL, 0);
    ok(status == STATUS_INVALID_PARAMETER_4, "status = 0x%lx\n", status);

    status = pRtlUTF8ToUnicodeN(NULL, 0, NULL, empty_string, 0);
    ok(status == STATUS_INVALID_PARAMETER, "status = 0x%lx\n", status);

    bytes_out = 0x55555555;
    status = pRtlUTF8ToUnicodeN(NULL, 0, &bytes_out, NULL, 0);
    ok(status == STATUS_INVALID_PARAMETER_4, "status = 0x%lx\n", status);
    ok(bytes_out == 0x55555555, "bytes_out = 0x%lx\n", bytes_out);

    bytes_out = 0x55555555;
    status = pRtlUTF8ToUnicodeN(NULL, 0, &bytes_out, invalid_pointer, 0);
    ok(status == STATUS_SUCCESS, "status = 0x%lx\n", status);
    ok(bytes_out == 0, "bytes_out = 0x%lx\n", bytes_out);

    bytes_out = 0x55555555;
    status = pRtlUTF8ToUnicodeN(NULL, 0, &bytes_out, empty_string, 0);
    ok(status == STATUS_SUCCESS, "status = 0x%lx\n", status);
    ok(bytes_out == 0, "bytes_out = 0x%lx\n", bytes_out);

    bytes_out = 0x55555555;
    status = pRtlUTF8ToUnicodeN(NULL, 0, &bytes_out, test_string, 0);
    ok(status == STATUS_SUCCESS, "status = 0x%lx\n", status);
    ok(bytes_out == 0, "bytes_out = 0x%lx\n", bytes_out);

    bytes_out = 0x55555555;
    status = pRtlUTF8ToUnicodeN(NULL, 0, &bytes_out, empty_string, 1);
    ok(status == STATUS_SUCCESS, "status = 0x%lx\n", status);
    ok(bytes_out == sizeof(WCHAR), "bytes_out = 0x%lx\n", bytes_out);

    /* length output with special chars */
#define length_expect(in_chars, out_chars, expect_status) \
        unicode_expect_(NULL, 0, out_chars, special_string, in_chars, \
                        expect_status, __LINE__)

    length_expect(0, 0, STATUS_SUCCESS);
    length_expect(1, 1, STATUS_SUCCESS);
    length_expect(2, 2, STATUS_SOME_NOT_MAPPED);
    length_expect(3, 2, STATUS_SUCCESS);
    length_expect(4, 3, STATUS_SOME_NOT_MAPPED);
    length_expect(5, 3, STATUS_SOME_NOT_MAPPED);
    length_expect(6, 3, STATUS_SOME_NOT_MAPPED);
    length_expect(7, 4, STATUS_SUCCESS);
    length_expect(8, 5, STATUS_SUCCESS);
#undef length_expect

    /* output truncation */
#define truncate_expect(buflen, out_chars, expect_status) \
        unicode_expect_(special_expected, buflen, out_chars, \
                        special_string, sizeof(special_string), \
                        expect_status, __LINE__)

    truncate_expect( 0, 0, STATUS_BUFFER_TOO_SMALL);
    truncate_expect( 1, 0, STATUS_BUFFER_TOO_SMALL);
    truncate_expect( 2, 1, STATUS_BUFFER_TOO_SMALL);
    truncate_expect( 3, 1, STATUS_BUFFER_TOO_SMALL);
    truncate_expect( 4, 2, STATUS_BUFFER_TOO_SMALL);
    truncate_expect( 5, 2, STATUS_BUFFER_TOO_SMALL);
    truncate_expect( 6, 3, STATUS_BUFFER_TOO_SMALL);
    truncate_expect( 7, 3, STATUS_BUFFER_TOO_SMALL);
    truncate_expect( 8, 4, STATUS_BUFFER_TOO_SMALL);
    truncate_expect( 9, 4, STATUS_BUFFER_TOO_SMALL);
    truncate_expect(10, 5, STATUS_SUCCESS);
#undef truncate_expect

    /* conversion behavior with varying input length */
    for (input_len = 0; input_len <= sizeof(test_string); input_len++) {
        /* no output buffer, just length */
        unicode_expect(NULL, 0, input_len,
                       test_string, input_len, STATUS_SUCCESS);

        /* write output */
        unicode_expect(test_stringW, -1, input_len,
                       test_string, input_len, STATUS_SUCCESS);
    }

    /* test cases for special characters */
    for (i = 0; i < test_count; i++) {
        bytes_out = 0x55555555;
        memset(buffer, 0x55, sizeof(buffer));
        status = pRtlUTF8ToUnicodeN(
            buffer, sizeof(buffer), &bytes_out,
            utf8_to_unicode[i].utf8, strlen(utf8_to_unicode[i].utf8));
        ok(status == utf8_to_unicode[i].status,
           "(test %d): status is 0x%lx, expected 0x%lx\n",
           i, status, utf8_to_unicode[i].status);
        ok(bytes_out == lstrlenW(utf8_to_unicode[i].expected) * sizeof(WCHAR),
           "(test %d): bytes_out is %lu, expected %lu\n",
           i, bytes_out, lstrlenW(utf8_to_unicode[i].expected) * (ULONG)sizeof(WCHAR));
        ok(!memcmp(buffer, utf8_to_unicode[i].expected, bytes_out),
           "(test %d): got %s, expected %s\n",
           i, wine_dbgstr_wn(buffer, bytes_out / sizeof(WCHAR)), wine_dbgstr_w(utf8_to_unicode[i].expected));
        ok(buffer[bytes_out / sizeof(WCHAR)] == 0x5555,
           "(test %d): behind string: 0x%x\n", i, buffer[bytes_out / sizeof(WCHAR)]);
        memset(buffer, 0x55, sizeof(buffer));
        ret = MultiByteToWideChar( CP_UTF8, 0, utf8_to_unicode[i].utf8, strlen(utf8_to_unicode[i].utf8),
                                   buffer, ARRAY_SIZE(buffer) );
        ok( ret == lstrlenW(utf8_to_unicode[i].expected), "(test %d): wrong len %u\n", i, ret );
        ok(!memcmp(buffer, utf8_to_unicode[i].expected, lstrlenW(utf8_to_unicode[i].expected) * sizeof(WCHAR)),
           "(test %d): got %s, expected %s\n",
           i, wine_dbgstr_wn(buffer, ret), wine_dbgstr_w(utf8_to_unicode[i].expected));
        ok(buffer[ret] == 0x5555,
           "(test %d): behind string: 0x%x\n", i, buffer[ret]);

        /* same test but include the null terminator */
        bytes_out = 0x55555555;
        memset(buffer, 0x55, sizeof(buffer));
        status = pRtlUTF8ToUnicodeN(
            buffer, sizeof(buffer), &bytes_out,
            utf8_to_unicode[i].utf8, strlen(utf8_to_unicode[i].utf8) + 1);
        ok(status == utf8_to_unicode[i].status,
           "(test %d): status is 0x%lx, expected 0x%lx\n",
           i, status, utf8_to_unicode[i].status);
        ok(bytes_out == (lstrlenW(utf8_to_unicode[i].expected) + 1) * sizeof(WCHAR),
           "(test %d): bytes_out is %lu, expected %lu\n",
           i, bytes_out, (lstrlenW(utf8_to_unicode[i].expected) + 1) * (ULONG)sizeof(WCHAR));
        ok(!memcmp(buffer, utf8_to_unicode[i].expected, bytes_out),
           "(test %d): got %s, expected %s\n",
           i, wine_dbgstr_wn(buffer, bytes_out / sizeof(WCHAR)), wine_dbgstr_w(utf8_to_unicode[i].expected));
        ok(buffer[bytes_out / sizeof(WCHAR)] == 0x5555,
           "(test %d): behind string: 0x%x\n", i, buffer[bytes_out / sizeof(WCHAR)]);

        memset(buffer, 0x55, sizeof(buffer));
        ret = MultiByteToWideChar( CP_UTF8, 0, utf8_to_unicode[i].utf8, -1, buffer, ARRAY_SIZE(buffer) );
        ok( ret == lstrlenW(utf8_to_unicode[i].expected) + 1, "(test %d): wrong len %u\n", i, ret );
        ok(!memcmp(buffer, utf8_to_unicode[i].expected, ret * sizeof(WCHAR)),
           "(test %d): got %s, expected %s\n",
           i, wine_dbgstr_wn(buffer, ret), wine_dbgstr_w(utf8_to_unicode[i].expected));
        ok(buffer[ret] == 0x5555,
           "(test %d): behind string: 0x%x\n", i, buffer[ret]);

        SetLastError( 0xdeadbeef );
        memset(buffer, 0x55, sizeof(buffer));
        ret = MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS,
                                   utf8_to_unicode[i].utf8, -1, buffer, ARRAY_SIZE(buffer) );
        if (utf8_to_unicode[i].status == STATUS_SOME_NOT_MAPPED)
        {
            ok( ret == 0, "(test %d): wrong len %u\n", i, ret );
            ok( GetLastError() == ERROR_NO_UNICODE_TRANSLATION, "(test %d): wrong error %lu\n", i, GetLastError() );
            ret = lstrlenW(utf8_to_unicode[i].expected) + 1;
        }
        else
            ok( ret == lstrlenW(utf8_to_unicode[i].expected) + 1, "(test %d): wrong len %u\n", i, ret );

        ok(!memcmp(buffer, utf8_to_unicode[i].expected, ret * sizeof(WCHAR)),
           "(test %d): got %s, expected %s\n",
           i, wine_dbgstr_wn(buffer, ret), wine_dbgstr_w(utf8_to_unicode[i].expected));
        ok(buffer[ret] == 0x5555,
           "(test %d): behind string: 0x%x\n", i, buffer[ret]);
    }
}

static NTSTATUS WINAPIV fmt( const WCHAR *src, ULONG width, BOOLEAN ignore_inserts, BOOLEAN ansi,
                             WCHAR *buffer, ULONG size, ULONG *retsize, ... )
{
    va_list args;
    NTSTATUS status;

    *retsize = 0xdeadbeef;
    va_start( args, retsize );
    status = pRtlFormatMessage( src, width, ignore_inserts, ansi, FALSE, &args, buffer, size, retsize );
    va_end( args );
    return status;
}

static void WINAPIV testfmt( const WCHAR *src, const WCHAR *expect, ULONG width, BOOL ansi, ... )
{
    va_list args;
    NTSTATUS status;
    WCHAR buffer[128];
    ULONG size = 0xdeadbeef;

    memset( buffer, 0xcc, sizeof(buffer) );
    va_start( args, ansi );
    status = pRtlFormatMessage( src, width, FALSE, ansi, FALSE, &args, buffer, sizeof(buffer), &size );
    va_end( args );
    ok( !status, "%s: failed %lx\n", debugstr_w(src), status );
    ok( !lstrcmpW( buffer, expect ), "%s: got %s expected %s\n", debugstr_w(src),
        debugstr_w(buffer), debugstr_w(expect) );
    ok( size == (lstrlenW(expect) + 1) * sizeof(WCHAR), "%s: wrong size %lu\n", debugstr_w(src), size );
}

static void testfmt_arg_eaten( const WCHAR *src, ... )
{
    va_list args;
    NTSTATUS status;
    WCHAR *arg, buffer[1];
    ULONG size = 0xdeadbeef;

    buffer[0] = 0xcccc;
    va_start( args, src );
    status = pRtlFormatMessage( src, 0, FALSE, FALSE, FALSE, &args, buffer, ARRAY_SIZE(buffer), &size );
    ok( status == STATUS_BUFFER_OVERFLOW, "%s: failed %lx\n", debugstr_w(src), status );
    todo_wine
    ok( buffer[0] == 0xcccc, "%s: got %x\n", debugstr_w(src), buffer[0] );
    ok( size == 0xdeadbeef, "%s: wrong size %lu\n", debugstr_w(src), size );
    arg = va_arg( args, WCHAR * );
    ok( !wcscmp( L"unused", arg ), "%s: wrong arg %s\n", debugstr_w(src), debugstr_w(arg) );
    va_end( args );
}

static void test_RtlFormatMessage(void)
{
    WCHAR buffer[128];
    NTSTATUS status;
    ULONG i, size;

    /* basic formats */
    testfmt( L"test", L"test", 0, FALSE );
    testfmt( L"", L"", 0, FALSE );
    testfmt( L"%1", L"test", 0, FALSE, L"test" );
    testfmt( L"%1!s!", L"test", 0, FALSE, L"test" );
    testfmt( L"%1!s!", L"foo", 0, TRUE, "foo" );
    testfmt( L"%1!S!", L"test", 0, FALSE, "test" );
    testfmt( L"%1!S!", L"foo", 0, TRUE, L"foo" );
    testfmt( L"%1!hs!%1!hS!", L"testtest", 0, FALSE, "test" );
    testfmt( L"%1!ls!%1!lS!%1!ws!%1!wS!", L"foofoofoofoo", 0, TRUE, L"foo" );
    testfmt( L"%1!c!", L"a", 0, FALSE, L'a' );
    testfmt( L"%1!c!", L"b", 0, TRUE, 'b' );
    testfmt( L"%1!C!", L"c", 0, FALSE, L'c' );
    testfmt( L"%1!C!", L"d", 0, TRUE, 'd' );
    testfmt( L"%1!hc!", L"e", 0, FALSE, L'e' );
    testfmt( L"%1!hC!", L"f", 0, FALSE, L'f' );
    testfmt( L"%1!lc!", L"g", 0, TRUE, 'g' );
    testfmt( L"%1!lC!", L"h", 0, TRUE, 'h' );
    testfmt( L"%1!wc!", L"i", 0, TRUE, 'i' );
    testfmt( L"%1!wC!", L"j", 0, TRUE, 'j' );
    testfmt( L"%1!04X!", L"BEEF", 0, FALSE, 0xbeef );
    testfmt( L"%1!Saa!", L"testaa", 0, FALSE, "test" );
    testfmt( L"%.%%%Z%n%t%r%!% ", L".%Z\r\n\t\r! ", 0, FALSE );
    testfmt( L"%1!*.*u!,%1!*.*u!", L"  001, 0002", 0, FALSE, 5, 3, 1, 4, 2 );
    testfmt( L"%1!*.*u!,%3!*.*u!", L"  001,  0002", 0, FALSE, 5, 3, 1, 6, 4, 2 );
    testfmt( L"%1", L"(null)", 0, FALSE, NULL );
    testfmt( L"%2", L"(null)", 0, TRUE, "abc", NULL );
    testfmt( L"ab%1!!cd", L"abcd", 0, FALSE, L"hello" );
    testfmt( L"abc%1!#.000000000000000000000000000x!", L"abc0x22", 0, FALSE, 34 );
    testfmt( L"a\r\nb\rc\r\rd\r\r\ne", L"a\r\nb\r\nc\r\n\r\nd\r\n\r\ne", 0, FALSE, NULL );
#ifdef _WIN64
    testfmt( L"%1!#I64x! %2!x!", L"0x1234 5678", 0, FALSE, (ULONG_PTR)0x1234, 0x5678, 0xbeef );
    testfmt( L"%1!x! %2!#I64x! %3!#I64x! %4!x!", L"dead 0x1111222233334444 0x5555666677778888 beef",
             0, FALSE, 0xdead, 0x1111222233334444ull, 0x5555666677778888ull, 0xbeef );
    testfmt( L"%3!#I64x! %4!#I64x! %3!x! %1!x!", L"0x3 0x4 3 1", 0, FALSE, 0xdead00000001ll, 2, 3ll, 4ll );
    testfmt( L"%2!x! %1!I64x!", L"5678 1234", 0, FALSE, (ULONG_PTR)0x1234, 0x5678, 0xbeef );
    testfmt( L"%2!*.*I64x! %1!u! %4!u! %2!u!", L"  00000000000000d 19 11 17", 0, FALSE,
             19ull, 17ull, 15ull, 13ull, 11ull, 9ull );
    {  /* argument array works differently */
        ULONG_PTR args[] = { 19, 17, 15, 13, 11, 9, 7 };
        memset( buffer, 0xcc, sizeof(buffer) );
        status = pRtlFormatMessage( L"%2!*.*I64x! %1!u! %4!u! %2!u!", 0, FALSE, FALSE, TRUE,
                                    (va_list *)args, buffer, sizeof(buffer), &size );
        ok( !lstrcmpW( buffer, L"  00000000000000d 19 13 17" ), "got %s\n", wine_dbgstr_w(buffer) );
        memset( buffer, 0xcc, sizeof(buffer) );
        status = pRtlFormatMessage( L"%1!I64u! %2!u! %4!.*I64x! %5!I64u!", 0, FALSE, FALSE, TRUE,
                                    (va_list *)args, buffer, sizeof(buffer), &size );
        ok( !lstrcmpW( buffer, L"19 17 000000000000b 11" ), "got %s\n", wine_dbgstr_w(buffer) );
    }
#else
    fmt( L"%1!#I64x! %2!x!", 0, FALSE, FALSE, buffer, sizeof(buffer), &size, 0x1234, 0x5678, 0xbeef );
    if (lstrcmpW( buffer, L"0x567800001234 5678" ))
    {
        testfmt( L"%1!#I64x! %2!x!", L"0x567800001234 beef", 0, FALSE, 0x1234, 0x5678, 0xbeef );
        testfmt( L"%1!x! %2!#I64x! %3!#I64x! %4!x!", L"dead 0x1111222233334444 0x5555666677778888 beef",
                 0, FALSE, 0xdead, 0x1111222233334444ull, 0x5555666677778888ull, 0xbeef );
        testfmt( L"%3!#I64x! %4!#I64x! %3!x! %1!x!", L"0x1111222233334444 0x5555666677778888 33334444 1",
                 0, FALSE, 1, 2, 3, 4, 0x33334444, 0x11112222, 0x77778888, 0x55556666, 0xbeef, 0xbee2 );
        testfmt( L"%2!x! %1!I64x!", L"5678 1234", 0, FALSE, 0x1234, 0x5678, 0xbeef );
        testfmt( L"%2!*.*I64x! %1!u! %4!u! %2!u!", L"  000090000000b 19 7 15", 0, FALSE,
                 19, 17, 15, 13, 11, 9, 7 );
        {  /* argument array works differently */
            ULONG_PTR args[] = { 19, 17, 15, 13, 11, 9, 7 };
            memset( buffer, 0xcc, sizeof(buffer) );
            status = pRtlFormatMessage( L"%2!*.*I64x! %1!u! %4!u! %2!u!", 0, FALSE, FALSE, TRUE,
                                        (va_list *)args, buffer, sizeof(buffer), &size );
            ok( !lstrcmpW( buffer, L"        d0000000f 19 13 17" ), "got %s\n", wine_dbgstr_w(buffer) );
            memset( buffer, 0xcc, sizeof(buffer) );
            status = pRtlFormatMessage( L"%1!I64u! %2!u! %4!.*I64x! %5!I64u!", 0, FALSE, FALSE, TRUE,
                                        (va_list *)args, buffer, sizeof(buffer), &size );
            ok( !lstrcmpW( buffer, L"19 17 0000b00000000 11" ), "got %s\n", wine_dbgstr_w(buffer) );
        }
    }
    else win_skip( "I64 support broken\n" );
#endif
    testfmt( L"%1!Ix! %2!QQ!", L"1234 QQ", 0, FALSE, (ULONG_PTR)0x1234 );
    testfmt( L"%1!#llx!%2!#x!%1!#hx!", L"0x1234560x789abc0x3456", 0, FALSE, 0x123456, 0x789abc );
    lstrcpyW( buffer, L"xxxxxxxxxx" );
    fmt( L"ab%0cd", 0, FALSE, FALSE, buffer, sizeof(buffer), &size );
    ok( !memcmp( buffer, L"ab\0xxxxxxx", 10 * sizeof(WCHAR) ), "got %s\n", wine_dbgstr_wn(buffer, 10) );

    /* max width */
    testfmt( L"%1", L"testing\r\n", 3, FALSE, L"testing" );
    testfmt( L"%1%2%3", L"testing\r\nabcdef\r\nfoobar\r\n", 4, FALSE, L"testing", L"abcdef", L"foobar");
    testfmt( L"%1%2%3%4", L"test\r\nabcd\r\nabcdef\r\n", 4, FALSE, L"test", L"abcd", L"abc", L"def" );
    testfmt( L"%1a\nb%2", L"testing\r\na\r\nbfoo bar\r\n", 3, FALSE, L"testing", L"foo bar" );
    testfmt( L"a%tb%t%t%t%c%r%r%r%r%r%rdefg", L"a\r\nb\r\n\r\n\r\nc\r\r\r\r\r\rdef\r\ng", 3, FALSE );
    testfmt( L"test abcd ", L"test\r\n\r\nabcd\r\n ", 4, FALSE );
    testfmt( L"test abcdef %1 foobar", L"tes\r\nt\r\nabc\r\ndef\r\n\r\nhello\r\nfoo\r\nbar\r\n", 3, FALSE, L"hello" );
    testfmt( L"te st\nabc d\nfoo", L"te st\r\nabc d\r\nfoo", 6, FALSE );
    testfmt( L"te st    ab    d\nfoo", L"te st\r\n  ab\r\n d foo", 7, FALSE );
    testfmt( L"te\tst\t\t\t\tab\t\t\td\nfoo", L"te\tst\t\t\r\n\t\tab\t\t\t\r\nd foo", 7, FALSE );
    testfmt( L"te st\n\n\r\n\nab    d\nfoo    ", L"te st\r\n  ab\r\n d foo\r\n   ", 7, FALSE );
    testfmt( L"te st\r\nabc d\n\nfoo\rbar", L"te st abc d  foo bar", 0xff, FALSE );
    testfmt( L"te st%r%nabc d%nfoo%rbar", L"te st\r\r\nabc d\r\nfoo\rbar", 0xff, FALSE );
    testfmt( L"\01\02\03\04\a\a\a\a\b\b\b\b\t\t\t\t\v\v\v\v\f\f\f\f\r\r\r\r    a",
             L"\01\02\r\n\03\04\r\n\a\a\r\n\a\a\r\n\b\b\r\n\b\b\r\n\t\t\r\n\t\t\r\n\v\v\r\n\v\v\r\n\f\f\r\n\f\f\r\n\r\n\r\n\r\n\r\na", 2, FALSE );

    for (i = 1; i < 0xffff; i++)
    {
        WCHAR src[] = { i, ' ', i, i, i, i, i, ' ', i, 0 };
        WCHAR expect[16];
        switch (i)
        {
        case '\t':
            lstrcpyW( expect, L"\r\n\r\n\t" );
            break;
        case '\r':
        case '\n':
        case ' ':
            lstrcpyW( expect, L"\r\n\r\n " );
            break;
        case '%':
            lstrcpyW( expect, L" %% \r\nxxxx" );
            break;
        default:
            swprintf( expect, ARRAY_SIZE(expect), L"%c\r\n%c%c%c%c\r\n%c %c", i,  i,  i,  i,  i,  i,  i );
            break;
        }
        lstrcpyW( buffer, L"xxxxxxxxxx" );
        fmt( src, 4, FALSE, FALSE, buffer, sizeof(buffer), &size );
        ok( !lstrcmpW( buffer, expect ), "%04lx: got %s\n", i, debugstr_w(buffer) );
    }

    /* args are not counted the same way with an argument array */
    {
        ULONG_PTR args[] = { 6, 4, 2, 5, 3, 1 };
        memset( buffer, 0xcc, sizeof(buffer) );
        status = pRtlFormatMessage( L"%1!*.*u!,%1!*.*u!", 0, FALSE, FALSE, TRUE, (va_list *)args,
                                    buffer, sizeof(buffer), &size );
        ok( !lstrcmpW( buffer, L"  0002, 00003" ), "got %s\n", wine_dbgstr_w(buffer) );
        memset( buffer, 0xcc, sizeof(buffer) );
        status = pRtlFormatMessage( L"%1!*.*u!,%4!*.*u!", 0, FALSE, FALSE, TRUE, (va_list *)args,
                                    buffer, sizeof(buffer), &size );
        ok( !lstrcmpW( buffer, L"  0002,  001" ), "got %s\n", wine_dbgstr_w(buffer) );
    }

    /* buffer overflows */
    lstrcpyW( buffer, L"xxxxxxxxxx" );
    status = fmt( L"testing", 0, FALSE, FALSE, buffer, 8, &size );
    ok( status == STATUS_BUFFER_OVERFLOW, "failed %lx\n", status );
    ok( !lstrcmpW( buffer, L"testxxxxxx" ) || broken(!lstrcmpW( buffer, L"tesxxxxxxx" )), /* winxp */
        "got %s\n", wine_dbgstr_w(buffer) );
    ok( size == 0xdeadbeef, "wrong size %lu\n", size );

    lstrcpyW( buffer, L"xxxxxxxxxx" );
    status = fmt( L"%1", 0, FALSE, FALSE, buffer, 8, &size, L"test" );
    ok( status == STATUS_BUFFER_OVERFLOW, "failed %lx\n", status );
    ok( !memcmp( buffer, L"tes\0xxxxxx", 10 * sizeof(WCHAR) ) || broken(!lstrcmpW( buffer, L"testxxxxxx" )), /* winxp */
        "got %s\n", wine_dbgstr_w(buffer) );
    ok( size == 0xdeadbeef, "wrong size %lu\n", size );

    lstrcpyW( buffer, L"xxxxxxxxxx" );
    status = fmt( L"%1!x!", 0, FALSE, FALSE, buffer, 8, &size, 0x12345678 );
    ok( status == STATUS_BUFFER_OVERFLOW, "failed %lx\n", status );
    ok( !memcmp( buffer, L"123\0xxxxxx", 10 * sizeof(WCHAR) ) || broken(!lstrcmpW( buffer, L"1234xxxxxx" )), /* winxp */
        "got %s\n", wine_dbgstr_w(buffer) );
    ok( size == 0xdeadbeef, "wrong size %lu\n", size );

    lstrcpyW( buffer, L"xxxxxxxxxx" );
    status = fmt( L"%1!*s!", 0, FALSE, FALSE, buffer, 10, &size, 5, L"abc" );
    ok( status == STATUS_BUFFER_OVERFLOW, "failed %lx\n", status );
    ok( !memcmp( buffer, L"  ab\0xxxxx", 10 * sizeof(WCHAR) ) || broken(!lstrcmpW( buffer, L"  abcxxxxx" )), /* winxp */
        "got %s\n", wine_dbgstr_w(buffer) );
    ok( size == 0xdeadbeef, "wrong size %lu\n", size );

    lstrcpyW( buffer, L"xxxxxxxxxx" );
    status = fmt( L"ab%n", 0, FALSE, FALSE, buffer, 6, &size );
    ok( status == STATUS_BUFFER_OVERFLOW, "failed %lx\n", status );
    ok( !memcmp( buffer, L"abxxxxxxxx", 10 * sizeof(WCHAR) ), "got %s\n", wine_dbgstr_w(buffer) );
    ok( size == 0xdeadbeef, "wrong size %lu\n", size );

    /* ignore inserts */
    lstrcpyW( buffer, L"xxxxxxxxxx" );
    status = fmt( L"%1!x!%r%%%n%t", 0, TRUE, FALSE, buffer, sizeof(buffer), &size );
    ok( !lstrcmpW( buffer, L"%1!x!\r%%\r\n\t" ), "got %s\n", wine_dbgstr_w(buffer) );

    lstrcpyW( buffer, L"xxxxxxxxxx" );
    status = fmt( L"ab%0cd", 0, TRUE, FALSE, buffer, sizeof(buffer), &size );
    ok( !status, "failed %lx\n", status );
    ok( !memcmp( buffer, L"ab\0xxxxxxx", 10 * sizeof(WCHAR) ), "got %s\n", wine_dbgstr_wn(buffer, 10) );

    /* invalid args */
    lstrcpyW( buffer, L"xxxxxxxxxx" );
    size = 0xdeadbeef;
    status = pRtlFormatMessage( L"abc%1", 0, FALSE, FALSE, FALSE, NULL, buffer, sizeof(buffer), &size );
    ok( status == STATUS_INVALID_PARAMETER, "failed %lx\n", status );
    ok( !lstrcmpW( buffer, L"abcxxxxxxx" ), "got %s\n", wine_dbgstr_w(buffer) );
    ok( size == 0xdeadbeef, "wrong size %lu\n", size );

    lstrcpyW( buffer, L"xxxxxxxxxx" );
    status = pRtlFormatMessage( L"abc%1", 0, FALSE, FALSE, TRUE, NULL, buffer, sizeof(buffer), &size );
    ok( status == STATUS_INVALID_PARAMETER, "failed %lx\n", status );
    ok( !lstrcmpW( buffer, L"abcxxxxxxx" ), "got %s\n", wine_dbgstr_w(buffer) );
    ok( size == 0xdeadbeef, "wrong size %lu\n", size );

    lstrcpyW( buffer, L"xxxxxxxxxx" );
    status = pRtlFormatMessage( L"abc%", 0, FALSE, FALSE, TRUE, NULL, buffer, sizeof(buffer), &size );
    ok( status == STATUS_INVALID_PARAMETER, "failed %lx\n", status );
    ok( !lstrcmpW( buffer, L"abcxxxxxxx" ), "got %s\n", wine_dbgstr_w(buffer) );
    ok( size == 0xdeadbeef, "wrong size %lu\n", size );

    lstrcpyW( buffer, L"xxxxxxxxxx" );
    status = fmt( L"%1!u! %2!u", 0, FALSE, FALSE, buffer, sizeof(buffer), &size, 34 );
    ok( status == STATUS_INVALID_PARAMETER, "failed %lx\n", status );
    ok( !lstrcmpW( buffer, L"34 xxxxxxx" ), "got %s\n", wine_dbgstr_w(buffer) );
    ok( size == 0xdeadbeef, "wrong size %lu\n", size );

    lstrcpyW( buffer, L"xxxxxxxxxx" );
    status = fmt( L"%1!**u!", 0, FALSE, FALSE, buffer, sizeof(buffer), &size, 34 );
    ok( status == STATUS_SUCCESS, "failed %lx\n", status );
    ok( !lstrcmpW( buffer, L"*u" ), "got %s\n", wine_dbgstr_w(buffer) );

    lstrcpyW( buffer, L"xxxxxxxxxx" );
    status = fmt( L"%1!0.3+*u!", 0, FALSE, FALSE, buffer, sizeof(buffer), &size, 34 );
    ok( status == STATUS_SUCCESS, "failed %lx\n", status );
    ok( !lstrcmpW( buffer, L"+*u" ), "got %s\n", wine_dbgstr_w(buffer) );

    lstrcpyW( buffer, L"xxxxxxxxxx" );
    status = fmt( L"aa%1!***u!", 0, FALSE, FALSE, buffer, sizeof(buffer), &size, 34 );
    ok( status == STATUS_INVALID_PARAMETER, "failed %lx\n", status );
    ok( !lstrcmpW( buffer, L"aaxxxxxxxx" ), "got %s\n", wine_dbgstr_w(buffer) );
    ok( size == 0xdeadbeef, "wrong size %lu\n", size );

    lstrcpyW( buffer, L"xxxxxxxxxx" );
    status = fmt( L"abc%1!#.000000000000000000000000000x!", 0, FALSE, FALSE, buffer, sizeof(buffer), &size, 34 );
    ok( status == STATUS_SUCCESS, "failed %lx\n", status );
    ok( !lstrcmpW( buffer, L"abc0x22" ), "got %s\n", wine_dbgstr_w(buffer) );

    lstrcpyW( buffer, L"xxxxxxxxxx" );
    status = fmt( L"abc%1!#.0000000000000000000000000000x!", 0, FALSE, FALSE, buffer, sizeof(buffer), &size, 34 );
    ok( status == STATUS_INVALID_PARAMETER, "failed %lx\n", status );
    ok( !lstrcmpW( buffer, L"abcxxxxxxx" ), "got %s\n", wine_dbgstr_w(buffer) );
    ok( size == 0xdeadbeef, "wrong size %lu\n", size );

    lstrcpyW( buffer, L"xxxxxxxxxx" );
    status = fmt( L"abc%1!hsaaaaaaaaaaaaaaaaaaaaaaaaaaaaa!", 0, FALSE, FALSE, buffer, sizeof(buffer), &size, "hello" );
    ok( status == STATUS_INVALID_PARAMETER, "failed %lx\n", status );
    ok( !lstrcmpW( buffer, L"abcxxxxxxx" ), "got %s\n", wine_dbgstr_w(buffer) );
    ok( size == 0xdeadbeef, "wrong size %lu\n", size );

    /* va_arg is eaten even in case of buffer overflow */
    testfmt_arg_eaten( L"%1!s! %2!s!", L"eaten", L"unused" );
}

START_TEST(rtlstr)
{
    InitFunctionPtrs();
    if (pRtlInitAnsiString) {
	test_RtlInitString();
	test_RtlInitUnicodeString();
	test_RtlCopyString();
	test_RtlUnicodeStringToInteger();
	test_RtlCharToInteger();
	test_RtlIntegerToUnicodeString();
	test_RtlIntegerToChar();
	test_RtlUpperChar();
	test_RtlUpperString();
	test_RtlUnicodeStringToAnsiString();
	test_RtlAppendAsciizToString();
	test_RtlAppendStringToString();
	test_RtlAppendUnicodeToString();
	test_RtlAppendUnicodeStringToString();
    }

    test_RtlInitUnicodeStringEx();
    test_RtlDuplicateUnicodeString();
    test_RtlFindCharInUnicodeString();
    test_RtlGUIDFromString();
    test_RtlStringFromGUID();
    test_RtlIsTextUnicode();
    test_RtlCompareUnicodeString();
    test_RtlUpcaseUnicodeChar();
    test_RtlUpcaseUnicodeString();
    test_RtlDowncaseUnicodeString();
    test_RtlHashUnicodeString();
    test_RtlUnicodeToUTF8N();
    test_RtlUTF8ToUnicodeN();
    test_RtlFormatMessage();
}

/*
 * PROJECT:     apphelp_apitest
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for shim database registration
 * COPYRIGHT:   Copyright 2017-2019 Mark Jansen (mark.jansen@reactos.org)
 */

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <ntndk.h>
#include <atlbase.h>
#include <strsafe.h>
#include "wine/test.h"

static const unsigned char rawDB[] =
{
    /* Header: Major,           Minor,                      'sdbf' */
    0x02, 0x00, 0x00, 0x00,     0x01, 0x00, 0x00, 0x00,     0x73, 0x64, 0x62, 0x66,

    /* TAG_DATABASE,    Length */
    0x01, 0x70,         0x22, 0x00, 0x00, 0x00,
        /* TAG_NAME,    Value */
        0x01, 0x60,     0x06, 0x00, 0x00, 0x00,

        /* TAG_DATABASE_ID, Length, Value*/
        0x07, 0x90,     0x10, 0x00, 0x00, 0x00,
        /* offset 30 */
                        0xEB, 0x75, 0xDD, 0x79, 0x98, 0xC0, 0x57, 0x47, 0x99, 0x65, 0x9E, 0x83, 0xC4, 0xCA, 0x9D, 0xA4,

        /* TAG_LIBRARY, Length */
        0x02, 0x70,     0x00, 0x00, 0x00, 0x00,

    /* TAG_STRINGTABLE, Length */
    0x01, 0x78,         0x0E, 0x00, 0x00, 0x00,
        /* TAG_STRINGTABLE_ITEM, Length, Value */
        0x01, 0x88,     0x08, 0x00, 0x00, 0x00,
                        0x49, 0x00, 0x43, 0x00, 0x53, 0x00, 0x00, 0x00
};

static BOOL WriteSdbFile(const WCHAR* FileName, const unsigned char* Data, DWORD Size, const GUID* CustomID)
{
    BOOL Success;
    DWORD dwWritten;
    HANDLE Handle = CreateFileW(FileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (Handle == INVALID_HANDLE_VALUE)
    {
        skip("Failed to create temp file %ls, error %u\n", FileName, GetLastError());
        return FALSE;
    }
    Success = WriteFile(Handle, Data, Size, &dwWritten, NULL);
    ok(Success == TRUE, "WriteFile failed with %u\n", GetLastError());
    ok(dwWritten == Size, "WriteFile wrote %u bytes instead of %u\n", dwWritten, Size);
    if (CustomID)
    {
        DWORD dwGuidSize;
        SetFilePointer(Handle, 30, NULL, FILE_BEGIN);
        Success = WriteFile(Handle, CustomID, sizeof(*CustomID), &dwGuidSize, NULL);
        ok(dwGuidSize == sizeof(GUID), "WriteFile wrote %u bytes instead of %u\n", dwGuidSize, sizeof(GUID));
    }
    CloseHandle(Handle);
    return Success && (dwWritten == Size);
}



static const GUID GUID_DATABASE_SHIM = {    0x11111111, 0x1111, 0x1111, { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 } };
static const GUID GUID_DATABASE_MSI = {     0xd8ff6d16, 0x6a3a, 0x468a, { 0x8b, 0x44, 0x01, 0x71, 0x4d, 0xdc, 0x49, 0xea } };
static const GUID GUID_DATABASE_DRIVERS = { 0xf9ab2228, 0x3312, 0x4a73, { 0xb6, 0xf9, 0x93, 0x6d, 0x70, 0xe1, 0x12, 0xef } };
static const GUID TEST_DB_GUID = {          0x79dd75eb, 0xc098, 0x4757, { 0x99, 0x65, 0x9e, 0x83, 0xc4, 0xca, 0x9d, 0xa4 } };

#define SDB_DATABASE_MAIN 0x80000000

BOOL (WINAPI *pSdbRegisterDatabase)(LPCWSTR pszDatabasePath, DWORD dwDatabaseType);
BOOL (WINAPI *pSdbRegisterDatabaseEx)(LPCWSTR pszDatabasePath, DWORD dwDatabaseType, const PULONGLONG pTimeStamp);
BOOL (WINAPI *pSdbUnregisterDatabase)(REFGUID pguidDB);


BOOL IsUserAdmin()
{
    BOOL Result;
    SID_IDENTIFIER_AUTHORITY NtAuthority = { SECURITY_NT_AUTHORITY };
    PSID AdministratorsGroup;

    Result = AllocateAndInitializeSid(&NtAuthority, 2,
                                      SECURITY_BUILTIN_DOMAIN_RID,
                                      DOMAIN_ALIAS_RID_ADMINS,
                                      0, 0, 0, 0, 0, 0,
                                      &AdministratorsGroup);
    if (Result)
    {
        if (!CheckTokenMembership( NULL, AdministratorsGroup, &Result))
            Result = FALSE;
        FreeSid(AdministratorsGroup);
    }

    return Result;
}

static DWORD g_QueryFlag = 0xffffffff;
static DWORD QueryFlag(void)
{
    if (g_QueryFlag == 0xffffffff)
    {
        ULONG_PTR wow64_ptr = 0;
        NTSTATUS status = NtQueryInformationProcess(NtCurrentProcess(), ProcessWow64Information, &wow64_ptr, sizeof(wow64_ptr), NULL);
        g_QueryFlag = (NT_SUCCESS(status) && wow64_ptr != 0) ? KEY_WOW64_64KEY : 0;
    }
    return g_QueryFlag;
}

static void FileTimeNow(ULARGE_INTEGER& Result)
{
    FILETIME TimeBuffer;

    GetSystemTimeAsFileTime(&TimeBuffer);
    Result.HighPart = TimeBuffer.dwHighDateTime;
    Result.LowPart = TimeBuffer.dwLowDateTime;
}

static void ok_keys_(REFGUID Guid, LPCWSTR DisplayName, LPCWSTR Path, DWORD Type, PULONGLONG TimeStamp)
{
    UNICODE_STRING GuidString;
    WCHAR StringBuffer[200];
    DWORD ValueBuffer;
    ULARGE_INTEGER LargeUIntBuffer;

    CRegKey key;
    LSTATUS Status = key.Open(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\InstalledSDB", KEY_READ | QueryFlag());
    winetest_ok(!Status, "Unable to open InstalledSDB key\n");
    if (Status)
        return;

    if (!SUCCEEDED(RtlStringFromGUID(Guid, &GuidString)))
    {
        ok(0, "Unable to format guid\n");
        return;
    }

    Status = key.Open(key.m_hKey, GuidString.Buffer, KEY_READ);
    winetest_ok(!Status, "Unable to open %s key (0x%x)\n", wine_dbgstr_w(GuidString.Buffer), Status);
    RtlFreeUnicodeString(&GuidString);
    if (Status)
        return;

    ULONG nChars = _countof(StringBuffer);
    Status = key.QueryStringValue(L"DatabaseDescription", StringBuffer, &nChars);
    winetest_ok(!Status, "Unable to read DatabaseDescription (0x%x)\n", Status);
    if (!Status)
        winetest_ok(!wcscmp(DisplayName, StringBuffer), "Expected DatabaseDescription to be %s, was %s\n", wine_dbgstr_w(DisplayName), wine_dbgstr_w(StringBuffer));

    nChars = _countof(StringBuffer);
    Status = key.QueryStringValue(L"DatabasePath", StringBuffer, &nChars);
    winetest_ok(!Status, "Unable to read DatabasePath (0x%x)\n", Status);
    if (!Status)
        winetest_ok(!wcscmp(Path, StringBuffer), "Expected DatabasePath to be %s, was %s\n", wine_dbgstr_w(Path), wine_dbgstr_w(StringBuffer));

    Status = key.QueryDWORDValue(L"DatabaseType", ValueBuffer);
    winetest_ok(!Status, "Unable to read DatabaseType (0x%x)\n", Status);
    if (!Status)
        winetest_ok(ValueBuffer == Type, "Expected DatabaseType to be 0x%x, was 0x%x\n", Type, ValueBuffer);

    Status = key.QueryQWORDValue(L"DatabaseInstallTimeStamp", LargeUIntBuffer.QuadPart);
    winetest_ok(!Status, "Unable to read DatabaseInstallTimeStamp (0x%x)\n", Status);
    if (!Status)
    {
        if (TimeStamp)
        {
            winetest_ok(LargeUIntBuffer.QuadPart == *TimeStamp, "Expected DatabaseInstallTimeStamp to be %s, was %s\n",
                wine_dbgstr_longlong(*TimeStamp), wine_dbgstr_longlong(LargeUIntBuffer.QuadPart));
        }
        else
        {
            ULARGE_INTEGER CurrentTime;
            FileTimeNow(CurrentTime);
            ULONG DiffMS = (ULONG)((CurrentTime.QuadPart - LargeUIntBuffer.QuadPart) / 10000);
            winetest_ok(DiffMS < 5000 , "Expected DatabaseInstallTimeStamp to be less than 5 seconds before now (was: %u)\n", DiffMS);
        }
    }
}


#define ok_keys         (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : ok_keys_


START_TEST(register)
{
    WCHAR TempPath[MAX_PATH * 2];
    BOOL Success;
    HMODULE hdll;

    SetEnvironmentVariableA("SHIM_DEBUG_LEVEL", "4");
    SetEnvironmentVariableA("SHIMENG_DEBUG_LEVEL", "4");
    SetEnvironmentVariableA("DEBUGCHANNEL", "+apphelp");

    //silence_debug_output();
    hdll = LoadLibraryA("apphelp.dll");

    *(void**)&pSdbRegisterDatabase = (void*)GetProcAddress(hdll, "SdbRegisterDatabase");
    *(void**)&pSdbRegisterDatabaseEx = (void*)GetProcAddress(hdll, "SdbRegisterDatabaseEx");
    *(void**)&pSdbUnregisterDatabase = (void*)GetProcAddress(hdll, "SdbUnregisterDatabase");

    if (!pSdbRegisterDatabase || !pSdbRegisterDatabaseEx || !pSdbUnregisterDatabase)
    {
        skip("Not all functions present: %p, %p, %p\n", pSdbRegisterDatabase, pSdbRegisterDatabaseEx, pSdbUnregisterDatabase);
        return;
    }

    /* [Err ][SdbUnregisterDatabase] Failed to open key "\Registry\Machine\Software\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\InstalledSDB\{11111111-1111-1111-1111-111111111111}" Status 0xc0000034 */
    ok_int(pSdbUnregisterDatabase(GUID_DATABASE_SHIM), FALSE);
    ok_int(pSdbUnregisterDatabase(GUID_DATABASE_MSI), FALSE);
    ok_int(pSdbUnregisterDatabase(GUID_DATABASE_DRIVERS), FALSE);


    if (!IsUserAdmin())
    {
        skip("Not running as admin, unable to install databases!\n");
        return;
    }

    GetTempPathW(_countof(TempPath), TempPath);
    StringCchCatW(TempPath, _countof(TempPath), L"\\shim_db.sdb");
    if (!WriteSdbFile(TempPath, rawDB, sizeof(rawDB), NULL))
    {
        skip("Cannot write %s\n", wine_dbgstr_w(TempPath));
        return;
    }

    /* No Type */
    Success = pSdbRegisterDatabase(TempPath, 0);
    ok_int(Success, TRUE);
    if (Success)
    {
        ok_keys(TEST_DB_GUID, L"ICS", TempPath, 0, NULL);
        Success = pSdbUnregisterDatabase(TEST_DB_GUID);
        ok_int(Success, TRUE);
    }

    /* Unknown type */
    Success = pSdbRegisterDatabase(TempPath, 1);
    ok_int(Success, TRUE);
    if (Success)
    {
        ok_keys(TEST_DB_GUID, L"ICS", TempPath, 1, NULL);
        Success = pSdbUnregisterDatabase(TEST_DB_GUID);
        ok_int(Success, TRUE);
    }

    /* System type */
    Success = pSdbRegisterDatabase(TempPath, SDB_DATABASE_MAIN);
    ok_int(Success, TRUE);
    if (Success)
    {
        ok_keys(TEST_DB_GUID, L"ICS", TempPath, SDB_DATABASE_MAIN, NULL);
        Success = pSdbUnregisterDatabase(TEST_DB_GUID);
        ok_int(Success, TRUE);
    }

    /* No type, null time */
    Success = pSdbRegisterDatabaseEx(TempPath, 0, NULL);
    ok_int(Success, TRUE);
    if (Success)
    {
        ok_keys(TEST_DB_GUID, L"ICS", TempPath, 0, NULL);
        Success = pSdbUnregisterDatabase(TEST_DB_GUID);
        ok_int(Success, TRUE);
    }

    /* Unknown type, null time */
    Success = pSdbRegisterDatabaseEx(TempPath, 1, NULL);
    ok_int(Success, TRUE);
    if (Success)
    {
        ok_keys(TEST_DB_GUID, L"ICS", TempPath, 1, NULL);
        Success = pSdbUnregisterDatabase(TEST_DB_GUID);
        ok_int(Success, TRUE);
    }


    /* System type, null time */
    Success = pSdbRegisterDatabaseEx(TempPath, SDB_DATABASE_MAIN, NULL);
    ok_int(Success, TRUE);
    if (Success)
    {
        ok_keys(TEST_DB_GUID, L"ICS", TempPath, SDB_DATABASE_MAIN, NULL);
        Success = pSdbUnregisterDatabase(TEST_DB_GUID);
        ok_int(Success, TRUE);
    }

    ULARGE_INTEGER Time;
    FileTimeNow(Time);
    Time.QuadPart ^= 0xffffffffffffffffll;
    /* No type, random time */
    Success = pSdbRegisterDatabaseEx(TempPath, 0, &Time.QuadPart);
    ok_int(Success, TRUE);
    if (Success)
    {
        ok_keys(TEST_DB_GUID, L"ICS", TempPath, 0, &Time.QuadPart);
        Success = pSdbUnregisterDatabase(TEST_DB_GUID);
        ok_int(Success, TRUE);
    }

    /* Unknown type, random time */
    Success = pSdbRegisterDatabaseEx(TempPath, 1, &Time.QuadPart);
    ok_int(Success, TRUE);
    if (Success)
    {
        ok_keys(TEST_DB_GUID, L"ICS", TempPath, 1, &Time.QuadPart);
        Success = pSdbUnregisterDatabase(TEST_DB_GUID);
        ok_int(Success, TRUE);
    }

    /* System type, random time */
    Success = pSdbRegisterDatabaseEx(TempPath, SDB_DATABASE_MAIN, &Time.QuadPart);
    ok_int(Success, TRUE);
    if (Success)
    {
        ok_keys(TEST_DB_GUID, L"ICS", TempPath, SDB_DATABASE_MAIN, &Time.QuadPart);
        Success = pSdbUnregisterDatabase(TEST_DB_GUID);
        ok_int(Success, TRUE);
    }

    /* System reserved ID's */
    if (!WriteSdbFile(TempPath, rawDB, sizeof(rawDB), &GUID_DATABASE_SHIM))
    {
        skip("Cannot write %s\n", wine_dbgstr_w(TempPath));
        DeleteFileW(TempPath);
        return;
    }

    Success = pSdbRegisterDatabase(TempPath, 0);
    ok_int(Success, TRUE);
    if (Success)
    {
        ok_keys(GUID_DATABASE_SHIM, L"ICS", TempPath, 0, NULL);
        Success = pSdbUnregisterDatabase(GUID_DATABASE_SHIM);
        ok_int(Success, TRUE);
    }

    if (!WriteSdbFile(TempPath, rawDB, sizeof(rawDB), &GUID_DATABASE_MSI))
    {
        skip("Cannot write %s\n", wine_dbgstr_w(TempPath));
        DeleteFileW(TempPath);
        return;
    }

    Success = pSdbRegisterDatabase(TempPath, 0);
    ok_int(Success, TRUE);
    if (Success)
    {
        ok_keys(GUID_DATABASE_MSI, L"ICS", TempPath, 0, NULL);
        Success = pSdbUnregisterDatabase(GUID_DATABASE_MSI);
        ok_int(Success, TRUE);
    }

    if (!WriteSdbFile(TempPath, rawDB, sizeof(rawDB), &GUID_DATABASE_DRIVERS))
    {
        skip("Cannot write %s\n", wine_dbgstr_w(TempPath));
        DeleteFileW(TempPath);
        return;
    }

    Success = pSdbRegisterDatabase(TempPath, 0);
    ok_int(Success, TRUE);
    if (Success)
    {
        ok_keys(GUID_DATABASE_DRIVERS, L"ICS", TempPath, 0, NULL);
        Success = pSdbUnregisterDatabase(GUID_DATABASE_DRIVERS);
        ok_int(Success, TRUE);
    }

    DeleteFileW(TempPath);
}

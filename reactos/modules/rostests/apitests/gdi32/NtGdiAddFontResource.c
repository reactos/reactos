/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiAddFontResourceW
 * PROGRAMMERS:     Víctor Martínez Calvo ( victor.martinez@reactos.org )
 */

#include <apitest.h>
#include <wingdi.h>
#include <ndk/rtlfuncs.h>
#include <strsafe.h>


INT
APIENTRY
NtGdiAddFontResourceW(
    _In_reads_(cwc) WCHAR *pwszFiles,
    _In_ ULONG cwc,
    _In_ ULONG cFiles,
    _In_ FLONG f,
    _In_ DWORD dwPidTid,
    _In_opt_ DESIGNVECTOR *pdv);

void Test_NtGdiAddFontResourceW()
{
    WCHAR lpszFontPath[MAX_PATH];
    WCHAR lpszFontSearch[MAX_PATH];

    INT ret;
    UNICODE_STRING NtAbsPath;
    WIN32_FIND_DATAW FindFileData;
    HANDLE hFind;
    ULONG cwc;

    // Create "Font" folder Path
    GetWindowsDirectoryW(lpszFontPath, MAX_PATH);
    StringCbCatW(lpszFontPath, sizeof(lpszFontPath), L"\\Fonts\\");

    // Search first .ttf file in Fonts Path
    StringCbCopyW(lpszFontSearch, sizeof(lpszFontSearch), lpszFontPath);
    StringCbCatW(lpszFontSearch, sizeof(lpszFontSearch), L"*.ttf");

    hFind = FindFirstFileW(lpszFontSearch, &FindFileData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        skip("Unable to find fonts in Font directory!\n");
        return;
    }

    // File found. Create FontPath to File.
    StringCbCatW(lpszFontPath, sizeof(lpszFontPath), FindFileData.cFileName);

    // Fail due "cwc" being zero.
    SetLastError(0xdeaddead);
    RtlInitUnicodeString(&NtAbsPath, NULL);
    RtlDosPathNameToNtPathName_U(lpszFontPath, &NtAbsPath, NULL, NULL);
    cwc = 0;
    ret =  NtGdiAddFontResourceW(NtAbsPath.Buffer, cwc, 1, 0, 0, 0);

    ok(ret == 0, "Expected 0 files added. Added: %d\n", ret);
    ok(GetLastError() == 0xdeaddead, "Expected 0xdeaddead. Obtained: 0x%lx\n", GetLastError());

    RtlFreeUnicodeString(&NtAbsPath);

    // "cwc" must count the null terminator. Otherwise fails.
    SetLastError(0xdeaddead);
    RtlInitUnicodeString(&NtAbsPath, NULL);
    RtlDosPathNameToNtPathName_U(lpszFontPath, &NtAbsPath, NULL, NULL);
    cwc = NtAbsPath.Length / sizeof(WCHAR);
    ret =  NtGdiAddFontResourceW(NtAbsPath.Buffer, cwc, 1, 0, 0, 0);

    ok(ret == 0, "Expected 0 files added. Added: %d\n", ret);
    ok(GetLastError() == 0xdeaddead, "Expected 0xdeaddead. Obtained: 0x%lx\n", GetLastError());

    RtlFreeUnicodeString(&NtAbsPath);

    // Correct "cwc" value.
    SetLastError(0xdeaddead);
    RtlInitUnicodeString(&NtAbsPath, NULL);
    RtlDosPathNameToNtPathName_U(lpszFontPath, &NtAbsPath, NULL, NULL);
    cwc = NtAbsPath.Length / sizeof(WCHAR) + 1;
    ret =  NtGdiAddFontResourceW(NtAbsPath.Buffer, cwc, 1, 0, 0, 0);

    ok(ret == 1, "Expected 1 files added. Added: %d\n", ret);
    ok(GetLastError() == 0xdeaddead, "Expected 0xdeaddead. Obtained: 0x%lx\n", GetLastError());

    RtlFreeUnicodeString(&NtAbsPath);

    // Test an invalid pointer.
    SetLastError(0xdeadbeef);
    ret =  NtGdiAddFontResourceW((PVOID)-4, 123, 1, 0, 0, NULL);

    ok(ret == 0, "Expected 0 files added. Added: %d\n", ret);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef. Obtained: 0x%lx\n", GetLastError());
}

START_TEST(NtGdiAddFontResource)
{
    Test_NtGdiAddFontResourceW();
}

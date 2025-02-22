/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for VerQueryValue[A/W]
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"

typedef struct
{
    WORD wLength;
    WORD wValueLength;
    WORD wType;
    WCHAR szKey[16]; // L"VS_VERSION_INFO"
    WORD Padding1;
    VS_FIXEDFILEINFO Value;

    struct _StringFileInfo
    {
        WORD wLength;
        WORD wValueLength;
        WORD wType;
        WCHAR szKey[15]; // L"StringFileInfo"

        struct _StringTable
        {
            WORD wLength;
            WORD wValueLength;
            WORD wType;
            WCHAR szKey[9]; // L"000004B"

            struct _String_CompanyName
            {
                WORD wLength;
                WORD wValueLength;
                WORD wType;
                WCHAR szKey[12]; // L"CompanyName"
                WORD Padding1;
                WCHAR Value[16]; // L"ReactOS Project"
            } CompanyName;

            struct _String_Comments
            {
                WORD wLength;
                WORD wValueLength; // 0
                WORD wType;
                WCHAR szKey[9]; // L"Comments"
            } Comments;

            struct _String_Comments2
            {
                WORD wLength;
                WORD wValueLength; // 0
                WORD wType;
                WCHAR szKey[10]; // L"Comments2"
                WORD Padding;
            } Comments2;

            struct _String_FooBar
            {
                WORD wLength;
                WORD wValueLength;
                WORD wType;
                WCHAR szKey[7]; // L"FooBar"
                WCHAR Value[4]; // L"Bar"
            } FooBar;

        } StringTable;
    } StringFileInfo;
} TEST_VERSIONINFO;

static const TEST_VERSIONINFO g_VersionInfo =
{
    .wLength = 298, // Header: 40, Value: 54, StringFileInfo: 204
    .wValueLength = sizeof(VS_FIXEDFILEINFO),
    .wType = 0,
    .szKey = L"VS_VERSION_INFO",
    .Padding1 = 0,
    .Value =
    {
        .dwSignature = VS_FFI_SIGNATURE,
        .dwStrucVersion = VS_FFI_STRUCVERSION,
        .dwFileVersionMS = 0x00050002,
        .dwFileVersionLS = 0x0ECE0000,
        .dwProductVersionMS = 0x00050002,
        .dwProductVersionLS = 0x0ECE0000,
        .dwFileFlagsMask = 0x00000002,
        .dwFileFlags = 0x00000000,
        .dwFileOS = VOS__WINDOWS32,
        .dwFileType = VFT_APP,
        .dwFileSubtype = VFT2_UNKNOWN,
        .dwFileDateMS = 0x00020074,
        .dwFileDateLS = 0x00000000,
    },
    .StringFileInfo =
    {
        .wLength = 204, // Header: 36, StringTable: 168
        .wValueLength = 0,
        .wType = 1,
        .szKey = L"StringFileInfo",
        .StringTable =
        {
            .wLength = 168, //Header: 24, Children: 144
            .wValueLength = 0,
            .wType = 1,
            .szKey = L"000004B0",

            .CompanyName =
            {
                .wLength = 64,
                .wValueLength = 16,
                .wType = 1,
                .szKey = L"CompanyName",
                .Padding1 = 0,
                .Value = L"ReactOS Project",
            },
            .Comments =
            {
                .wLength = 24,
                .wValueLength = 0,
                .wType = 1,
                .szKey = L"Comments",
            },
            .Comments2 =
            {
                .wLength = 28,
                .wValueLength = 0,
                .wType = 1,
                .szKey = L"Comments2",
                .Padding = 0xCC,
            },
            .FooBar =
            {
                .wLength = 28,
                .wValueLength = 4,
                .wType = 1,
                .szKey = L"FooBar",
                .Value = L"Bar",
            },
        },
    },
};

void
Test_VerQueryValueA(void)
{
    DWORD dwVersionInfoSizeW, dwVersionInfoSizeA;
    PVOID pvVersionInfoA, pvData = NULL;
    CHAR szSubBlock[256];
    USHORT uLanguage, uCodePage;
    UINT cbLen;
    BOOL result;

    /* Get the size for the unicode version info */
    dwVersionInfoSizeW = GetFileVersionInfoSizeW(L"kernel32.dll", NULL);
    ok(dwVersionInfoSizeW > 0, "GetFileVersionInfoSizeW failed\n");

    /* Get the size for the ANSI version info */
    dwVersionInfoSizeA = GetFileVersionInfoSizeA("kernel32.dll", NULL);
    ok(dwVersionInfoSizeA > 0, "GetFileVersionInfoSizeA failed\n");
    ok(dwVersionInfoSizeA == dwVersionInfoSizeW, "Unexpected size\n");

    /* Get the ANSI version info from kernel32 */
    pvVersionInfoA = malloc(dwVersionInfoSizeA);
    memset(pvVersionInfoA, 0xCC, dwVersionInfoSizeA);
    result = GetFileVersionInfoA("kernel32.dll", 0, dwVersionInfoSizeA, pvVersionInfoA);
    ok(result, "GetFileVersionInfoA failed\n");

    /* Query available translations */
    result = VerQueryValueA(pvVersionInfoA,
                            "\\VarFileInfo\\Translation",
                            &pvData,
                            &cbLen);
    ok(result, "VerQueryValueA failed\n");
    ok(cbLen >= 4, "Unexpected value\n");
    ok((cbLen & 0x3) == 0, "Unexpected value\n");
    uLanguage = ((USHORT*)pvData)[0];
    uCodePage = ((USHORT*)pvData)[1];

    /* Query sublock */
    sprintf(szSubBlock, "\\StringFileInfo\\%04X%04X\\CompanyName", uLanguage, uCodePage);
    result = VerQueryValueA(pvVersionInfoA,
                            szSubBlock,
                            &pvData,
                            &cbLen);
    ok(result, "VerQueryValueA failed\n");
    ok(cbLen >= 2, "Unexpected value\n");
    ok((cbLen & 0x1) == 0, "Unexpected value\n");

    free(pvVersionInfoA);
}

void
Test_VerQueryValueW(void)
{
    DWORD dwVersionInfoSizeW;
    PVOID pvVersionInfoW, pvData = NULL;
    WCHAR szSubBlock[256];
    USHORT uLanguage, uCodePage;
    UINT cbLen;
    BOOL result;

    /* Get the size for the unicode version info */
    dwVersionInfoSizeW = GetFileVersionInfoSizeW(L"kernel32.dll", NULL);
    ok(dwVersionInfoSizeW > 0, "GetFileVersionInfoSizeW failed\n");

    /* Get the unicode version info from kernel32 */
    pvVersionInfoW = malloc(dwVersionInfoSizeW);
    result = GetFileVersionInfoW(L"kernel32.dll", 0, dwVersionInfoSizeW, pvVersionInfoW);
    ok(result, "GetFileVersionInfoW failed\n");

    /* Query available translations */
    result = VerQueryValueW(pvVersionInfoW,
                            L"\\VarFileInfo\\Translation",
                            &pvData,
                            &cbLen);
    ok(result, "VerQueryValueA failed\n");
    ok(cbLen >= 4, "Unexpected value\n");
    ok((cbLen & 0x3) == 0, "Unexpected value\n");
    uLanguage = ((USHORT*)pvData)[0];
    uCodePage = ((USHORT*)pvData)[1];

    /* Query sublock */
    _swprintf(szSubBlock, L"\\StringFileInfo\\%04X%04X\\CompanyName", uLanguage, uCodePage);
    result = VerQueryValueW(pvVersionInfoW,
                            szSubBlock,
                            &pvData,
                            &cbLen);
    ok(result, "VerQueryValueA failed\n");
    ok(cbLen >= 2, "Unexpected value\n");
    ok((cbLen & 0x1) == 0, "Unexpected value\n");

    free(pvVersionInfoW);
}

void
Test_StaticVersionInfo(void)
{
    PVOID pVersionInfo;
    PVOID pvData = NULL;
    SIZE_T ExpectedOffset;
    UINT cbLen;
    BOOL result;

    /* Make a copy of the version info. Windows actually writes to it!
       We make the buffer twice as big to account for the ANSI copy,
       even if we don't use it. */
    pVersionInfo = malloc(2 * sizeof(g_VersionInfo));
    memset(pVersionInfo, 0, 2 * sizeof(g_VersionInfo));
    memcpy(pVersionInfo, &g_VersionInfo, sizeof(g_VersionInfo));

    /* Test a special static version */
    result = VerQueryValueW(pVersionInfo,
                            L"\\StringFileInfo\\000004B0\\CompanyName",
                            &pvData,
                            &cbLen);
    ok(result, "VerQueryValueW failed\n");
    ok_eq_int(cbLen, 16);
    ok_eq_wstr((WCHAR*)pvData, L"ReactOS Project");
    ExpectedOffset = FIELD_OFFSET(TEST_VERSIONINFO, StringFileInfo.StringTable.CompanyName.Value);
    ok(pvData == (PVOID)((ULONG_PTR)pVersionInfo + ExpectedOffset), "Unexpected offset\n");

    result = VerQueryValueW(pVersionInfo,
                            L"\\StringFileInfo\\000004B0\\Comments",
                            &pvData,
                            &cbLen);
    ok(result, "VerQueryValueW failed\n");
    ok_eq_int(cbLen, 0);
    ok_eq_wstr((WCHAR*)pvData, L"");
    ExpectedOffset = FIELD_OFFSET(TEST_VERSIONINFO, StringFileInfo.StringTable.Comments.szKey[8]);
    ok(pvData == (PVOID)((ULONG_PTR)pVersionInfo + ExpectedOffset), "Unexpected offset\n");

    result = VerQueryValueW(pVersionInfo,
                            L"\\StringFileInfo\\000004B0\\Comments2",
                            &pvData,
                            &cbLen);
    ok(result, "VerQueryValueW failed\n");
    ok_eq_int(cbLen, 0);
    ok_eq_wstr((WCHAR*)pvData, L"");
    ExpectedOffset = FIELD_OFFSET(TEST_VERSIONINFO, StringFileInfo.StringTable.Comments2.szKey[9]);
    ok(pvData == (PVOID)((ULONG_PTR)pVersionInfo + ExpectedOffset), "Unexpected offset\n");

    result = VerQueryValueW(pVersionInfo,
                            L"\\StringFileInfo\\000004B0\\FooBar",
                            &pvData,
                            &cbLen);
    ok(result, "VerQueryValueW failed\n");
    ok_eq_int(cbLen, 4);
    ok(wcscmp((WCHAR*)pvData, L"Bar") == 0, "Bar\n");
    ExpectedOffset = FIELD_OFFSET(TEST_VERSIONINFO, StringFileInfo.StringTable.FooBar.Value);
    ok(pvData == (PVOID)((ULONG_PTR)pVersionInfo + ExpectedOffset), "Unexpected offset\n");

    free(pVersionInfo);
}

START_TEST(VerQueryValue)
{
    Test_VerQueryValueA();
    Test_VerQueryValueW();
    Test_StaticVersionInfo();
}

/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GetTextFace
 * PROGRAMMERS:     Timo Kreuzer
 *                  Katayama Hirofumi MZ
 *                  Doug Lyons
 */

#include "precomp.h"

/* Exported by gdi32.dll but undocumented */
INT
WINAPI
GetTextFaceAliasW(
    IN HDC hdc,
    IN INT c,
    OUT LPWSTR lpAliasName);

void Test_GetTextFace(void)
{
    HDC hDC;
    INT ret;
    INT ret2;
    WCHAR Buffer[20];

    hDC = CreateCompatibleDC(NULL);
    ok(hDC != 0, "CreateCompatibleDC failed, skipping tests.\n");
    if (!hDC) return;

    /* Whether asking for the string size (NULL buffer) ignores the size argument */
    SetLastError(0xE000BEEF);
    ret = GetTextFaceW(hDC, 0, NULL);
    TEST(ret != 0);
    ok(GetLastError() == 0xE000BEEF, "GetLastError() == %ld\n", GetLastError());
    ret2 = ret;

    SetLastError(0xE000BEEF);
    ret = GetTextFaceW(hDC, -1, NULL);
    TEST(ret != 0);
    TEST(ret == ret2);
    ok(GetLastError() == 0xE000BEEF, "GetLastError() == %ld\n", GetLastError());
    ret2 = ret;

    SetLastError(0xE000BEEF);
    ret = GetTextFaceW(hDC, 10000, NULL);
    TEST(ret != 0);
    TEST(ret == ret2);
    ok(GetLastError() == 0xE000BEEF, "GetLastError() == %ld\n", GetLastError());
    ret2 = ret;

    /* Whether the buffer is correctly filled */
    SetLastError(0xE000BEEF);
    ret = GetTextFaceW(hDC, 20, Buffer);
    TEST(ret != 0);
    TEST(ret <= 20);
    TEST(Buffer[ret - 1] == 0);
    ok(GetLastError() == 0xE000BEEF, "GetLastError() == %ld\n", GetLastError());

    SetLastError(0xE000BEEF);
    ret = GetTextFaceW(hDC, 1, Buffer);
    TEST(ret == 1);
    TEST(Buffer[ret - 1] == 0);
    ok(GetLastError() == 0xE000BEEF, "GetLastError() == %ld\n", GetLastError());

    SetLastError(0xE000BEEF);
    ret = GetTextFaceW(hDC, 2, Buffer);
    TEST(ret == 2);
    TEST(Buffer[ret - 1] == 0);
    ok(GetLastError() == 0xE000BEEF, "GetLastError() == %ld\n", GetLastError());

    /* Whether invalid buffer sizes are correctly ignored */
    SetLastError(0xE000BEEF);
    ret = GetTextFaceW(hDC, 0, Buffer);
    TEST(ret == 0);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError() == %ld\n", GetLastError());

    SetLastError(0xE000BEEF);
    ret = GetTextFaceW(hDC, -1, Buffer);
    TEST(ret == 0);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError() == %ld\n", GetLastError());

    DeleteDC(hDC);
}

void Test_GetTextFaceAliasW(void)
{
    HDC hDC;
    INT ret;
    INT ret2;
    UINT i;
    LOGFONTW lf;
    HFONT hFontOld, hFont;
    WCHAR buf1[LF_FACESIZE];
    WCHAR buf2[LF_FACESIZE];

    static struct
    {
        LPCWSTR lpFaceName;
        LPCWSTR lpExpectedFaceName;
        LPCWSTR lpExpectedAlias;
    } FaceTests[] =
    {
        {L"Arial", L"Arial", L"Arial"},
        {L"Tahoma", L"Tahoma", L"Tahoma"},
        {L"Tahoma Bold", L"MS Sans Serif", L"MS Sans Serif"}, // That's what Windows 2003 and 7 returns.
        {L"Helv", L"Helv", L"Helv"},
        {L"Tms Rmn", L"Tms Rmn", L"Tms Rmn"},
        {L"Times", L"Times", L"Times"},
        {L"invalid", L"MS Sans Serif", L"MS Sans Serif"}
    };

    hDC = CreateCompatibleDC(NULL);
    ok(hDC != 0, "CreateCompatibleDC failed, skipping tests.\n");
    if (!hDC) return;

    for (i = 0; i < ARRAYSIZE(FaceTests); ++i)
    {
        ZeroMemory(&lf, sizeof(lf));
        StringCchCopyW(lf.lfFaceName, ARRAYSIZE(lf.lfFaceName), FaceTests[i].lpFaceName);

        hFont = CreateFontIndirectW(&lf);
        if (!hFont)
        {
            trace("Failed to create font '%S'!\n", lf.lfFaceName);
            continue;
        }

        hFontOld = SelectObject(hDC, hFont);

        ret = GetTextFaceW(hDC, ARRAYSIZE(buf1), buf1);
        ok(ret != 0, "%S GetTextFaceW failed.\n", FaceTests[i].lpFaceName);
        ok(wcscmp(buf1, FaceTests[i].lpExpectedFaceName) == 0, "'%S' GetTextFaceW failed, got '%S', expected '%S'.\n",
            FaceTests[i].lpFaceName, buf1, FaceTests[i].lpExpectedFaceName);

        ret2 = GetTextFaceAliasW(hDC, ARRAYSIZE(buf2), buf2);
        ok(ret2 != 0, "%S GetTextFaceAliasW failed.\n", FaceTests[i].lpFaceName);
        ok(wcscmp(buf2, FaceTests[i].lpExpectedAlias) == 0, "'%S' GetTextFaceAliasW failed, got '%S', expected '%S'.\n",
            FaceTests[i].lpFaceName, buf2, FaceTests[i].lpExpectedAlias);

        SelectObject(hDC, hFontOld);
        DeleteObject(hFont);
    }

    DeleteDC(hDC);
}

START_TEST(GetTextFace)
{
    Test_GetTextFace();
    Test_GetTextFaceAliasW();
}

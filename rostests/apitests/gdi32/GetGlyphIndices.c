/*
* PROJECT:         ReactOS api tests
* LICENSE:         GPL - See COPYING in the top level directory
* PURPOSE:         Test for GetGlyphIndices
* PROGRAMMERS:     Ged Murphy
*/

#include <apitest.h>
#include <wingdi.h>
#include <winuser.h>
#include <Strsafe.h>


#define ok_lasterrornotchanged() \
    ok_err(0x12345)

#define MAX_BMP_GLYPHS 0xFFFF

static LPVOID GetResource(LPCWSTR FontName, LPDWORD Size)
{
    HRSRC hRsrc;
    LPVOID Data;

    hRsrc = FindResourceW(GetModuleHandleW(NULL), FontName, (LPCWSTR)RT_RCDATA);
    if (!hRsrc) return NULL;

    Data = LockResource(LoadResource(GetModuleHandleW(NULL), hRsrc));
    if (!Data) return NULL;

    *Size = SizeofResource(GetModuleHandleW(NULL), hRsrc);
    if (*Size == 0) return NULL;

    return Data;
}

static BOOL ExtractTTFFile(LPCWSTR FontName, LPWSTR TempFile)
{
    WCHAR TempPath[MAX_PATH];
    HANDLE hFile;
    void *Data;
    DWORD Size;
    BOOL ret;

    Data = GetResource(FontName, &Size);
    if (!Data) return FALSE;

    GetTempPathW(MAX_PATH, TempPath);
    GetTempFileNameW(TempPath, L"ttf", 0, TempFile);

    hFile = CreateFileW(TempFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;

    ret = WriteFile(hFile, Data, Size, &Size, NULL);

    CloseHandle(hFile);
    return ret;
}

static BOOL InstallTempFont(LPWSTR TempFile)
{
    if (ExtractTTFFile(L"ReactOSTestTahoma.ttf", TempFile))
    {
        if (AddFontResourceExW(TempFile, FR_PRIVATE, 0) > 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

static VOID RemoveTempFont(LPWSTR TempFile)
{
    BOOL Success;
    Success = RemoveFontResourceExW(TempFile, FR_PRIVATE, 0);
    ok(Success, "RemoveFontResourceEx() failed, we're leaving fonts installed : %lu\n", GetLastError());
    DeleteFileW(TempFile);
}

static HFONT IntCreateFont(LPWSTR FontName)
{
    LOGFONTW Font = { 0 };
    Font.lfCharSet = DEFAULT_CHARSET;
    wcsncpy(Font.lfFaceName, FontName, sizeof(Font.lfFaceName) / sizeof(Font.lfFaceName[0]));
    return CreateFontIndirectW(&Font);
}

START_TEST(GetGlyphIndices)
{
    WCHAR Glyphs[MAX_BMP_GLYPHS];
    WORD Indices[MAX_BMP_GLYPHS];
    WCHAR Single[2] = { L' ', UNICODE_NULL };
    WCHAR TempTTFFile[MAX_PATH];
    HFONT hFont;
    HDC hdc;
    int i;

    if (!InstallTempFont(TempTTFFile))
    {
        skip("Failed to create ttf file for testing\n");
        return;
    }

    hdc = CreateCompatibleDC(NULL);
    ok(hdc != 0, "CreateCompatibleDC failed, skipping tests.\n");
    if (!hdc) return;

    hFont = IntCreateFont(L"ReactOSTestTahoma");
    ok(hFont != NULL, "Failed to open the test font");
    SelectObject(hdc, hFont);

    SetLastError(0x12345);

    /* Test NULL DC */
    ok_int(GetGlyphIndicesW(NULL, Single, 1, Indices, 0), GDI_ERROR);
    ok_lasterrornotchanged();

    /* Test invalid DC */
    ok_int(GetGlyphIndicesW((HDC)(ULONG_PTR)0x12345, Single, 1, Indices, 0), GDI_ERROR);
    ok_lasterrornotchanged();

    /* Test invalid params */
    ok_int(GetGlyphIndicesW(hdc, NULL, 0, Indices, 0), GDI_ERROR);
    ok_lasterrornotchanged();
    ok_int(GetGlyphIndicesW(hdc, NULL, 1, Indices, 0), GDI_ERROR);
    ok_lasterrornotchanged();
    ok_int(GetGlyphIndicesW(hdc, Single, 1, NULL, 0), GDI_ERROR);
    ok_lasterrornotchanged();

    /* Test a single valid char */
    Single[0] = L'a';
    ok_int(GetGlyphIndicesW(hdc, Single, 1, Indices, 0), 1);
    ok_lasterrornotchanged();
    ok_int(Indices[0], 68);

    /* Setup an array of all possible BMP glyphs */
    for (i = 0; i < 4; i++)
        Glyphs[i] = (WCHAR)i;

    /* Test a string of valid chars */
    StringCchCopyW(Glyphs, MAX_BMP_GLYPHS, L"0123");
    ok_int(GetGlyphIndicesW(hdc, Glyphs, 4, Indices, 0), 4);
    ok_lasterrornotchanged();
    ok_int(Indices[0], 19);
    ok_int(Indices[1], 20);
    ok_int(Indices[2], 21);
    ok_int(Indices[3], 22);

    /* Setup an array of all possible BMP glyphs */
    for (i = 0; i < MAX_BMP_GLYPHS; i++)
        Glyphs[i] = (WCHAR)i;

    /* Get all the glyphs */
    ok_int(GetGlyphIndicesW(hdc,
                            Glyphs,
                            MAX_BMP_GLYPHS,
                            Indices,
                            GGI_MARK_NONEXISTING_GLYPHS), MAX_BMP_GLYPHS);

    /* The first 32 are invalid and should contain 0xffff */
    for (i = 0; i < 32; i++)
        ok_int(Indices[i], 0xffff);

    /* These are the first 2 valid chars */
    ok(Indices[32] != 0xffff, "ascii char ' ' should be a valid char");
    ok(Indices[33] != 0xffff, "ascii char '!' should be a valid char");

    RemoveTempFont(TempTTFFile);
}


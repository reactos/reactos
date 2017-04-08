/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for AddFontMemResourceEx
 * PROGRAMMERS:     Mark Jansen
 *
 * PanosePitchTest by Katayama Hirofumi MZ, licensed under CC BY
 */


#include <apitest.h>
#include <wingdi.h>
#include <winuser.h>


static void test_font_caps(HDC hdc)
{
    HGDIOBJ old;
    TEXTMETRICA tm = { 0 };
    char name[64];
    BOOL ret;
    HFONT font = CreateFont(0, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("PanosePitchTest"));

    if (font)
    {
        old = SelectObject(hdc, font);

        memset(&tm, 0xaa, sizeof(tm));
        ret = GetTextMetricsA(hdc, &tm);
        ok_int(ret, TRUE);

        SetLastError(0xdeadbeef);
        ret = GetTextFaceA(hdc, sizeof(name), name);
        ok(ret, "GetTextFaceA error %lu\n", GetLastError());
        if (ret)
        {
            ok_str(name, "PanosePitchTest");
        }

        ok_int(tm.tmHeight, 11);
        ok_int(tm.tmAscent, 11);
        ok_int(tm.tmDescent, 0);
        ok_int(tm.tmInternalLeading, -5);
        ok_int(tm.tmExternalLeading, 1);
        ok_int(tm.tmAveCharWidth, 8);
        ok_int(tm.tmMaxCharWidth, 11);
        ok_int(tm.tmWeight, FW_NORMAL);
        ok_int(tm.tmOverhang, 0);
        ok_int(tm.tmDigitizedAspectX, 96);
        ok_int(tm.tmDigitizedAspectY, 96);
        ok_int(tm.tmFirstChar, 63);
        ok_int(tm.tmLastChar, 65);
        ok_int(tm.tmDefaultChar, 165);
        ok_int(tm.tmBreakChar, 65);
        ok_int(tm.tmItalic, 0);
        ok_int(tm.tmUnderlined, 0);
        ok_int(tm.tmStruckOut, 0);
        ok_hex(tm.tmPitchAndFamily, TMPF_TRUETYPE | TMPF_VECTOR);
        ok_int(tm.tmCharSet, SHIFTJIS_CHARSET);

        SelectObject(hdc, old);
        DeleteObject(font);
    }

    font = CreateFont(0, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                      OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("@PanosePitchTest"));

    if (font)
    {
        old = SelectObject(hdc, font);

        memset(&tm, 0xaa, sizeof(tm));
        ret = GetTextMetricsA(hdc, &tm);
        ok_int(ret, TRUE);

        SetLastError(0xdeadbeef);
        ret = GetTextFaceA(hdc, sizeof(name), name);
        ok(ret, "GetTextFaceA error %lu\n", GetLastError());
        if (ret)
        {
            ok_str(name, "@PanosePitchTest");
        }

        ok_int(tm.tmHeight, 11);
        ok_int(tm.tmAscent, 11);
        ok_int(tm.tmDescent, 0);
        ok_int(tm.tmInternalLeading, -5);
        ok_int(tm.tmExternalLeading, 1);
        ok_int(tm.tmAveCharWidth, 8);
        ok_int(tm.tmMaxCharWidth, 11);
        ok_int(tm.tmWeight, FW_NORMAL);
        ok_int(tm.tmOverhang, 0);
        ok_int(tm.tmDigitizedAspectX, 96);
        ok_int(tm.tmDigitizedAspectY, 96);
        ok_int(tm.tmFirstChar, 63);
        ok_int(tm.tmLastChar, 65);
        ok_int(tm.tmDefaultChar, 165);
        ok_int(tm.tmBreakChar, 65);
        ok_int(tm.tmItalic, 0);
        ok_int(tm.tmUnderlined, 0);
        ok_int(tm.tmStruckOut, 0);
        ok_hex(tm.tmPitchAndFamily, TMPF_TRUETYPE | TMPF_VECTOR);
        ok_int(tm.tmCharSet, SHIFTJIS_CHARSET);

        SelectObject(hdc, old);
        DeleteObject(font);
    }
}

/* Not working as of 2017-04-08 on ReactOS */
static BOOL is_font_available(HDC hdc, const char* fontName)
{
    char name[64];
    BOOL ret;

    HFONT font = CreateFontA(0, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, fontName);
    HGDIOBJ old = SelectObject(hdc, font);


    SetLastError(0xdeadbeef);

    ret = GetTextFaceA(hdc, sizeof(name), name);
    ok(ret, "GetTextFaceA error %lu\n", GetLastError());
    SelectObject(hdc, old);
    DeleteObject(font);

    if (ret)
    {
        return !_strcmpi(name, fontName);
    }
    return FALSE;
}



START_TEST(AddFontMemResourceEx)
{
    HMODULE mod;
    HRSRC hRsrc;

    HGLOBAL hTemplate;
    DWORD dwSize, dwNumFonts;
    LPVOID pFont;

    HANDLE hFont;

    HDC hdc = CreateCompatibleDC(NULL);
    BOOL is_font_available_broken = is_font_available(hdc, "Nonexisting font name here");

    ok(!is_font_available_broken, "Validating font is broken! (CORE-13053) !\n");

    if (is_font_available_broken || !is_font_available(hdc, "PanosePitchTest"))
    {
        mod = GetModuleHandle(NULL);
        hRsrc = FindResource(mod, TEXT("PanosePitchTest.ttf"), MAKEINTRESOURCE(RT_RCDATA));

        hTemplate = LoadResource(mod, hRsrc);
        dwSize = SizeofResource(mod, hRsrc);
        pFont = LockResource(hTemplate);

        dwNumFonts = 0;
        hFont = AddFontMemResourceEx(pFont, dwSize, NULL, &dwNumFonts);
        ok_int(dwNumFonts, 2);
        ok(hFont != NULL, "Expected valid handle\n");

        if (hFont)
        {
            test_font_caps(hdc);
            RemoveFontMemResourceEx(hFont);
            if (!is_font_available_broken)
            {
                ok (!is_font_available(hdc, "PanosePitchTest"), "Expected font to be unregistered again\n");
            }
            else
            {
                skip("Font unregister test\n");
            }
        }

        UnlockResource(hTemplate);
        FreeResource(hTemplate);
    }
    else
    {
        skip("Font PanosePitchTest already available\n");
    }
    DeleteDC(hdc);
}


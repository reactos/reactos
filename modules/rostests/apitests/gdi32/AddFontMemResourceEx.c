/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for AddFontMemResourceEx
 * PROGRAMMERS:     Mark Jansen
 *
 * PanosePitchTest + TTCTestV by Katayama Hirofumi MZ, licensed under CC BY
 * Shadows_Into_Light by Kimberly Geswein, licensed under OFL
 *                    Captured from firefox, embedded on reactos.org
 */

#include "precomp.h"

typedef struct _fnt_res
{
    const char* FontName;
    TEXTMETRICA tm;
} fnt_res;

typedef struct _fnt_test
{
    const char* ResourceName;
    int NumFaces;
    fnt_res res[4];
} fnt_test;



static fnt_test test_data[] =
{
    {
        /* .ResourceName = */ "PanosePitchTest.ttf",
        /* .NumFaces = */ 2,
        /* .res = */
        {
            {
                /* .FontName = */ "PanosePitchTest",
                {
                /* .tm.tmHeight = */ 11,
                /* .tm.tmAscent = */ 11,
                /* .tm.tmDescent = */ 0,
                /* .tm.tmInternalLeading = */ -5,
                /* .tm.tmExternalLeading = */ 1,
                /* .tm.tmAveCharWidth = */ 8,
                /* .tm.tmMaxCharWidth = */ 11,
                /* .tm.tmWeight = */ FW_NORMAL,
                /* .tm.tmOverhang = */ 0,
                /* .tm.tmDigitizedAspectX = */ 96,
                /* .tm.tmDigitizedAspectY = */ 96,
                /* .tm.tmFirstChar = */ 63,
                /* .tm.tmLastChar = */ 65,
                /* .tm.tmDefaultChar = */ 165,
                /* .tm.tmBreakChar = */ 65,
                /* .tm.tmItalic = */ 0,
                /* .tm.tmUnderlined = */ 0,
                /* .tm.tmStruckOut = */ 0,
                /* .tm.tmPitchAndFamily = */ TMPF_TRUETYPE | TMPF_VECTOR,
                /* .tm.tmCharSet = */ SHIFTJIS_CHARSET,
                }
            },
            {
                /* .FontName = */ "@PanosePitchTest",
                {
                /* .tm.tmHeight = */ 11,
                /* .tm.tmAscent = */ 11,
                /* .tm.tmDescent = */ 0,
                /* .tm.tmInternalLeading = */ -5,
                /* .tm.tmExternalLeading = */ 1,
                /* .tm.tmAveCharWidth = */ 8,
                /* .tm.tmMaxCharWidth = */ 11,
                /* .tm.tmWeight = */ FW_NORMAL,
                /* .tm.tmOverhang = */ 0,
                /* .tm.tmDigitizedAspectX = */ 96,
                /* .tm.tmDigitizedAspectY = */ 96,
                /* .tm.tmFirstChar = */ 63,
                /* .tm.tmLastChar = */ 65,
                /* .tm.tmDefaultChar = */ 165,
                /* .tm.tmBreakChar = */ 65,
                /* .tm.tmItalic = */ 0,
                /* .tm.tmUnderlined = */ 0,
                /* .tm.tmStruckOut = */ 0,
                /* .tm.tmPitchAndFamily = */ TMPF_TRUETYPE | TMPF_VECTOR,
                /* .tm.tmCharSet = */ SHIFTJIS_CHARSET,
                }
            },
        },
    },
    {
        /* .ResourceName = */ "TTCTestV.ttc",
        /* .NumFaces = */ 3,
        /* .res = */
        {
            {
                /* .FontName = */ "No1Of3in1",
                {
                /* .tm.tmHeight = */ 12,
                /* .tm.tmAscent = */ 12,
                /* .tm.tmDescent = */ 0,
                /* .tm.tmInternalLeading = */ -4,
                /* .tm.tmExternalLeading = */ 1,
                /* .tm.tmAveCharWidth = */ -525,
                /* .tm.tmMaxCharWidth = */ 6,
                /* .tm.tmWeight = */ FW_NORMAL,
                /* .tm.tmOverhang = */ 0,
                /* .tm.tmDigitizedAspectX = */ 96,
                /* .tm.tmDigitizedAspectY = */ 96,
                /* .tm.tmFirstChar = */ 63,
                /* .tm.tmLastChar = */ 65,
                /* .tm.tmDefaultChar = */ 64,
                /* .tm.tmBreakChar = */ 65,
                /* .tm.tmItalic = */ 0,
                /* .tm.tmUnderlined = */ 0,
                /* .tm.tmStruckOut = */ 0,
                /* .tm.tmPitchAndFamily = */ TMPF_TRUETYPE | TMPF_VECTOR | TMPF_FIXED_PITCH,
                /* .tm.tmCharSet = */ ANSI_CHARSET,
                }
            },
            {
                /* .FontName = */ "No2Of3in1",
                {
                /* .tm.tmHeight = */ 12,
                /* .tm.tmAscent = */ 12,
                /* .tm.tmDescent = */ 0,
                /* .tm.tmInternalLeading = */ -4,
                /* .tm.tmExternalLeading = */ 1,
                /* .tm.tmAveCharWidth = */ 8,
                /* .tm.tmMaxCharWidth = */ 7,
                /* .tm.tmWeight = */ FW_NORMAL,
                /* .tm.tmOverhang = */ 0,
                /* .tm.tmDigitizedAspectX = */ 96,
                /* .tm.tmDigitizedAspectY = */ 96,
                /* .tm.tmFirstChar = */ 63,
                /* .tm.tmLastChar = */ 65,
                /* .tm.tmDefaultChar = */ 64,
                /* .tm.tmBreakChar = */ 65,
                /* .tm.tmItalic = */ 0,
                /* .tm.tmUnderlined = */ 0,
                /* .tm.tmStruckOut = */ 0,
                /* .tm.tmPitchAndFamily = */ TMPF_TRUETYPE | TMPF_VECTOR | TMPF_FIXED_PITCH,
                /* .tm.tmCharSet = */ ANSI_CHARSET,
                }
            },
            {
                /* .FontName = */ "No3Of3in1V",
                {
                /* .tm.tmHeight = */ 12,
                /* .tm.tmAscent = */ 12,
                /* .tm.tmDescent = */ 0,
                /* .tm.tmInternalLeading = */ -4,
                /* .tm.tmExternalLeading = */ 1,
                /* .tm.tmAveCharWidth = */ 8,
                /* .tm.tmMaxCharWidth = */ 13,
                /* .tm.tmWeight = */ FW_NORMAL,
                /* .tm.tmOverhang = */ 0,
                /* .tm.tmDigitizedAspectX = */ 96,
                /* .tm.tmDigitizedAspectY = */ 96,
                /* .tm.tmFirstChar = */ 63,
                /* .tm.tmLastChar = */ 65,
                /* .tm.tmDefaultChar = */ 64,
                /* .tm.tmBreakChar = */ 65,
                /* .tm.tmItalic = */ 0,
                /* .tm.tmUnderlined = */ 0,
                /* .tm.tmStruckOut = */ 0,
                /* .tm.tmPitchAndFamily = */ FF_MODERN | TMPF_TRUETYPE | TMPF_VECTOR,
                /* .tm.tmCharSet = */ ANSI_CHARSET,
                }
            },
        },
    },
    {
        /* .ResourceName = */ "Shadows_Into_Light.ttf",
        /* .NumFaces = */ 1,
        /* .res = */
        {
            {
                /* .FontName = */ "ufaXaAlLOxCUGYJ7KN51UP2Q==",
                {
                /* .tm.tmHeight = */ 26,
                /* .tm.tmAscent = */ 19,
                /* .tm.tmDescent = */ 7,
                /* .tm.tmInternalLeading = */ 10,
                /* .tm.tmExternalLeading = */ 0,
                /* .tm.tmAveCharWidth = */ 7,
                /* .tm.tmMaxCharWidth = */ 23,
                /* .tm.tmWeight = */ FW_NORMAL,
                /* .tm.tmOverhang = */ 0,
                /* .tm.tmDigitizedAspectX = */ 96,
                /* .tm.tmDigitizedAspectY = */ 96,
                /* .tm.tmFirstChar = */ 30,
                /* .tm.tmLastChar = */ 255,
                /* .tm.tmDefaultChar = */ 31,
                /* .tm.tmBreakChar = */ 32,
                /* .tm.tmItalic = */ 0,
                /* .tm.tmUnderlined = */ 0,
                /* .tm.tmStruckOut = */ 0,
                /* .tm.tmPitchAndFamily = */ TMPF_TRUETYPE | TMPF_VECTOR | TMPF_FIXED_PITCH,
                /* .tm.tmCharSet = */ ANSI_CHARSET,
                }
            },
        },
    },
};


#define ok_int2(expression) \
    do { \
        int _value = (expression); \
        ok(_value == (res->expression), "Wrong value for '%s', expected: %d, got: %d for %s/%s\n", \
           #expression, (int)(res->expression), _value, test_name, res->FontName); \
    } while (0)

#define ok_hex2(expression) \
    do { \
        int _value = (expression); \
        ok(_value == (res->expression), "Wrong value for '%s', expected: 0x%x, got: 0x%x for %s/%s\n", \
           #expression, (int)(res->expression), _value, test_name, res->FontName); \
    } while (0)


static void test_font_caps(HDC hdc, int test_index)
{
    HGDIOBJ old;
    TEXTMETRICA tm = { 0 };
    char name[64];
    BOOL ret;
    HFONT font;
    int n;
    const char* test_name = test_data[test_index].ResourceName;

    for (n = 0; test_data[test_index].res[n].FontName; ++n)
    {
        fnt_res* res = test_data[test_index].res + n;
        font = CreateFontA(0, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                     OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, res->FontName);

        if (font)
        {
            old = SelectObject(hdc, font);

            memset(&tm, 0xaa, sizeof(tm));
            ret = GetTextMetricsA(hdc, &tm);
            ok(ret, "GetTextMetricsA() for %s/%s\n", test_name, res->FontName);

            SetLastError(0xdeadbeef);
            ret = GetTextFaceA(hdc, sizeof(name), name);
            ok(ret, "GetTextFaceA error %lu for %s/%s\n", GetLastError(), test_name, res->FontName);
            if (ret)
            {
                ok(!strcmp(name, res->FontName), "FontName was %s, expected %s for %s/%s", name, res->FontName, test_name, res->FontName);
            }

            ok_int2(tm.tmHeight);
            ok_int2(tm.tmAscent);
            ok_int2(tm.tmDescent);
            ok_int2(tm.tmInternalLeading);
            ok_int2(tm.tmExternalLeading);
            ok_int2(tm.tmAveCharWidth);
            ok_int2(tm.tmMaxCharWidth);
            ok_int2(tm.tmWeight);
            ok_int2(tm.tmOverhang);
            ok_int2(tm.tmDigitizedAspectX);
            ok_int2(tm.tmDigitizedAspectY);
            ok_int2(tm.tmFirstChar);
            ok_int2(tm.tmLastChar);
            ok_int2(tm.tmDefaultChar);
            ok_int2(tm.tmBreakChar);
            ok_int2(tm.tmItalic);
            ok_int2(tm.tmUnderlined);
            ok_int2(tm.tmStruckOut);
            ok_hex2(tm.tmPitchAndFamily);
            ok_int2(tm.tmCharSet);

            SelectObject(hdc, old);
            DeleteObject(font);
        }
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
    ok(ret, "GetTextFaceA error %lu for %s\n", GetLastError(), fontName);
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
    fnt_test* data;
    int n;

    HDC hdc = CreateCompatibleDC(NULL);
    BOOL is_font_available_broken = is_font_available(hdc, "Nonexisting font name here");

    ok(!is_font_available_broken, "Validating font is broken! (CORE-13053)!\n");

    for (n = 0; n < _countof(test_data); ++n)
    {
        data = test_data + n;

        mod = GetModuleHandle(NULL);
        hRsrc = FindResourceA(mod, data->ResourceName, MAKEINTRESOURCE(RT_RCDATA));

        hTemplate = LoadResource(mod, hRsrc);
        dwSize = SizeofResource(mod, hRsrc);
        pFont = LockResource(hTemplate);

        dwNumFonts = 0;
        hFont = AddFontMemResourceEx(pFont, dwSize, NULL, &dwNumFonts);
        ok(dwNumFonts == data->NumFaces, "dwNumFonts was %lu, expected %d for %s\n", dwNumFonts, data->NumFaces, data->ResourceName);
        ok(hFont != NULL, "Expected valid handle for %s\n", data->ResourceName);

        if (hFont)
        {
            test_font_caps(hdc, n);
            RemoveFontMemResourceEx(hFont);
            if (!is_font_available_broken)
            {
                ok (!is_font_available(hdc, data->ResourceName), "Expected font to be unregistered again for %s\n", data->ResourceName);
            }
            else
            {
                skip("Font unregister test for %s\n", data->ResourceName);
            }
        }

        UnlockResource(hTemplate);
        FreeResource(hTemplate);
    }

    DeleteDC(hdc);
}


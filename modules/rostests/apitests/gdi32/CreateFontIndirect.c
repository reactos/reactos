/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for CreateFontIndirect
 * PROGRAMMERS:     Timo Kreuzer
 *                  Katayama Hirofumi MZ
 */

#include "precomp.h"

#define trace_if(val, msg) do { if (!(val)) trace(msg); } while (0)

void
Test_CreateFontIndirectA(void)
{
    LOGFONTA logfont;
    HFONT hFont;
    ULONG ret;
    ENUMLOGFONTEXDVW elfedv2;

    logfont.lfHeight = 12;
    logfont.lfWidth = 0;
    logfont.lfEscapement = 0;
    logfont.lfOrientation = 0;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfItalic = 0;
    logfont.lfUnderline = 0;
    logfont.lfStrikeOut = 0;
    logfont.lfCharSet = DEFAULT_CHARSET;
    logfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    logfont.lfQuality = PROOF_QUALITY;
    logfont.lfPitchAndFamily = DEFAULT_PITCH;
    memset(logfont.lfFaceName, 'A', LF_FACESIZE);
    hFont = CreateFontIndirectA(&logfont);
    ok(hFont != 0, "CreateFontIndirectA failed\n");

    memset(&elfedv2, 0, sizeof(elfedv2));
    ret = GetObjectW(hFont, sizeof(elfedv2), &elfedv2);
    ok(ret == sizeof(ENUMLOGFONTEXW) + 2*sizeof(DWORD), "ret = %ld\n", ret);
    ok(elfedv2.elfEnumLogfontEx.elfLogFont.lfFaceName[LF_FACESIZE-1] == 0, "\n");
    ok(elfedv2.elfEnumLogfontEx.elfFullName[0] == 0, "\n");
}

void
Test_CreateFontIndirectW(void)
{
    LOGFONTW logfont;
    HFONT hFont;
    ULONG ret;
    ENUMLOGFONTEXDVW elfedv2;

    logfont.lfHeight = 12;
    logfont.lfWidth = 0;
    logfont.lfEscapement = 0;
    logfont.lfOrientation = 0;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfItalic = 0;
    logfont.lfUnderline = 0;
    logfont.lfStrikeOut = 0;
    logfont.lfCharSet = DEFAULT_CHARSET;
    logfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    logfont.lfQuality = PROOF_QUALITY;
    logfont.lfPitchAndFamily = DEFAULT_PITCH;
    memset(logfont.lfFaceName, 'A', LF_FACESIZE * 2);
    hFont = CreateFontIndirectW(&logfont);
    ok(hFont != 0, "CreateFontIndirectW failed\n");

    memset(&elfedv2, 0, sizeof(elfedv2));
    ret = GetObjectW(hFont, sizeof(elfedv2), &elfedv2);
    ok(ret == sizeof(ENUMLOGFONTEXW) + 2*sizeof(DWORD), "\n");
    ok(elfedv2.elfEnumLogfontEx.elfLogFont.lfFaceName[LF_FACESIZE-1] == ((WCHAR)'A' << 8) + 'A', "\n");
    ok(elfedv2.elfEnumLogfontEx.elfFullName[0] == 0, "\n");
    /* Theres a bunch of data in elfFullName ... */
}

void
Test_CreateFontIndirectExA(void)
{
    ENUMLOGFONTEXDVA elfedva, elfedva2;
    ENUMLOGFONTEXDVW elfedvw;
    ENUMLOGFONTEXA *penumlfa;
    LOGFONTA *plogfonta;
    HFONT hFont;
    ULONG ret;

    memset(&elfedva, 0, sizeof(elfedva));
    penumlfa = &elfedva.elfEnumLogfontEx;
    plogfonta = &elfedva.elfEnumLogfontEx.elfLogFont;

    plogfonta->lfHeight = 12;
    plogfonta->lfWidth = 0;
    plogfonta->lfEscapement = 0;
    plogfonta->lfOrientation = 0;
    plogfonta->lfWeight = FW_NORMAL;
    plogfonta->lfItalic = 0;
    plogfonta->lfUnderline = 0;
    plogfonta->lfStrikeOut = 0;
    plogfonta->lfCharSet = DEFAULT_CHARSET;
    plogfonta->lfOutPrecision = OUT_DEFAULT_PRECIS;
    plogfonta->lfClipPrecision = CLIP_DEFAULT_PRECIS;
    plogfonta->lfQuality = PROOF_QUALITY;
    plogfonta->lfPitchAndFamily = DEFAULT_PITCH;

    memset(plogfonta->lfFaceName, 'A', LF_FACESIZE * sizeof(WCHAR));
    memset(penumlfa->elfFullName, 'B', LF_FULLFACESIZE * sizeof(WCHAR));

    hFont = CreateFontIndirectExA(&elfedva);
    ok(hFont != 0, "CreateFontIndirectExA failed\n");

    ret = GetObjectW(hFont, sizeof(elfedvw), &elfedvw);
    ok(ret == sizeof(ENUMLOGFONTEXW) + 2*sizeof(DWORD), "\n");
    ok(elfedvw.elfEnumLogfontEx.elfLogFont.lfFaceName[LF_FACESIZE-1] == 0, "\n");
    ok(elfedvw.elfEnumLogfontEx.elfFullName[LF_FULLFACESIZE-1] == 0, "\n");

    memset(&elfedva2, 0, sizeof(elfedva2));
    ret = GetObjectA(hFont, sizeof(elfedva2), &elfedva2);
    ok(ret == sizeof(ENUMLOGFONTEXDVA), "ret = %ld\n", ret);
    ok(elfedva2.elfEnumLogfontEx.elfLogFont.lfFaceName[LF_FACESIZE-1] == 0, "\n");
    ok(elfedva2.elfEnumLogfontEx.elfFullName[LF_FULLFACESIZE-1] == 0, "\n");
}

void
Test_CreateFontIndirectExW(void)
{
    ENUMLOGFONTEXDVW elfedv, elfedv2;
    ENUMLOGFONTEXDVA elfedva;
    ENUMLOGFONTEXW *penumlfw;
    LOGFONTW *plogfontw;
    HFONT hFont;
    ULONG ret;

    memset(&elfedv, 0, sizeof(elfedv));
    penumlfw = &elfedv.elfEnumLogfontEx;
    plogfontw = &elfedv.elfEnumLogfontEx.elfLogFont;

    plogfontw->lfHeight = 12;
    plogfontw->lfWidth = 0;
    plogfontw->lfEscapement = 0;
    plogfontw->lfOrientation = 0;
    plogfontw->lfWeight = FW_NORMAL;
    plogfontw->lfItalic = 0;
    plogfontw->lfUnderline = 0;
    plogfontw->lfStrikeOut = 0;
    plogfontw->lfCharSet = DEFAULT_CHARSET;
    plogfontw->lfOutPrecision = OUT_DEFAULT_PRECIS;
    plogfontw->lfClipPrecision = CLIP_DEFAULT_PRECIS;
    plogfontw->lfQuality = PROOF_QUALITY;
    plogfontw->lfPitchAndFamily = DEFAULT_PITCH;

    memset(plogfontw->lfFaceName, 'A', LF_FACESIZE * sizeof(WCHAR));
    memset(penumlfw->elfFullName, 'B', LF_FULLFACESIZE * sizeof(WCHAR));

    hFont = CreateFontIndirectExW(&elfedv);
    ok(hFont != 0, "CreateFontIndirectExW failed\n");

    memset(&elfedv2, 0, sizeof(elfedv2));
    ret = GetObjectW(hFont, sizeof(elfedv2), &elfedv2);
    ok(ret == sizeof(ENUMLOGFONTEXW) + 2*sizeof(DWORD), "\n");
    ok(elfedv2.elfEnumLogfontEx.elfLogFont.lfFaceName[LF_FACESIZE-1] == ((WCHAR)'A' << 8) + 'A', "\n");
    ok(elfedv2.elfEnumLogfontEx.elfFullName[LF_FULLFACESIZE-1] == ((WCHAR)'B' << 8) + 'B', "\n");

    memset(&elfedva, 0, sizeof(elfedva));
    ret = GetObjectA(hFont, sizeof(elfedva), &elfedva);
    ok(ret == sizeof(ENUMLOGFONTEXDVA), "\n");
    ok(elfedva.elfEnumLogfontEx.elfLogFont.lfFaceName[LF_FACESIZE-1] == '?', "\n");
    ok(elfedva.elfEnumLogfontEx.elfFullName[LF_FULLFACESIZE-1] == 0, "\n");
}

static INT CALLBACK
is_truetype_font_proc(const LOGFONTA *elf, const TEXTMETRICA *ntm,
                      DWORD type, LPARAM lParam)
{
    if (type != TRUETYPE_FONTTYPE) return 1;

    return 0;
}

static BOOL is_truetype_font_installed(HDC hDC, const char *name)
{
    LOGFONT lf;
    ZeroMemory(&lf, sizeof(lf));
    lf.lfCharSet = DEFAULT_CHARSET;
    lstrcpy(lf.lfFaceName, name);
    if (!EnumFontFamiliesExA(hDC, &lf, is_truetype_font_proc, 0, 0))
        return TRUE;
    return FALSE;
}

static INT CALLBACK
is_charset_font_proc(const LOGFONTA *elf, const TEXTMETRICA *ntm,
                               DWORD type, LPARAM lParam)
{
    if (ntm->tmCharSet == (BYTE)lParam)
        return 0;
    return 1;
}

static BOOL is_charset_font_installed(HDC hDC, BYTE CharSet)
{
    LOGFONT lf;
    ZeroMemory(&lf, sizeof(lf));
    lf.lfCharSet = DEFAULT_CHARSET;
    if (!EnumFontFamiliesExA(hDC, &lf, is_charset_font_proc, CharSet, 0))
        return TRUE;
    return FALSE;
}

/* TMPF_FIXED_PITCH is confusing. brain-damaged api */
#define _TMPF_VAR_PITCH     TMPF_FIXED_PITCH

static INT CALLBACK
is_fixed_charset_font_proc(const LOGFONTA *elf, const TEXTMETRICA *ntm,
                                     DWORD type, LPARAM lParam)
{
    if (ntm->tmCharSet == (BYTE)lParam && !(ntm->tmPitchAndFamily & _TMPF_VAR_PITCH))
        return 0;
    return 1;
}

static BOOL
is_fixed_charset_font_installed(HDC hDC, BYTE CharSet)
{
    LOGFONT lf;
    ZeroMemory(&lf, sizeof(lf));
    lf.lfCharSet = DEFAULT_CHARSET;
    if (!EnumFontFamiliesExA(hDC, &lf, is_fixed_charset_font_proc, CharSet, 0))
        return TRUE;
    return FALSE;
}

static void
Test_FontPresence(void)
{
    HDC hDC;

    hDC = CreateCompatibleDC(NULL);

    ok(is_truetype_font_installed(hDC, "Arial"), "'Arial' is not found\n");
    ok(is_truetype_font_installed(hDC, "Courier New"), "'Courier New' is not found\n");
    ok(is_truetype_font_installed(hDC, "Marlett"), "'Marlett' is not found\n");
    ok(is_truetype_font_installed(hDC, "MS Shell Dlg"), "'MS Shell Dlg' is not found\n");
    ok(is_truetype_font_installed(hDC, "Tahoma"), "'Tahoma' is not found\n");
    ok(is_truetype_font_installed(hDC, "Times New Roman"), "'Times New Roman' is not found\n");

    ok(is_charset_font_installed(hDC, ANSI_CHARSET), "ANSI_CHARSET fonts are not found\n");
    ok(is_charset_font_installed(hDC, SYMBOL_CHARSET), "SYMBOL_CHARSET fonts are not found\n");
    trace_if(is_charset_font_installed(hDC, SHIFTJIS_CHARSET), "SHIFTJIS_CHARSET fonts are not found\n");
    trace_if(is_charset_font_installed(hDC, HANGUL_CHARSET), "HANGUL_CHARSET fonts are not found\n");
    trace_if(is_charset_font_installed(hDC, GB2312_CHARSET), "GB2312_CHARSET fonts are not found\n");
    trace_if(is_charset_font_installed(hDC, CHINESEBIG5_CHARSET), "CHINESEBIG5_CHARSET fonts are not found\n");
    ok(is_charset_font_installed(hDC, OEM_CHARSET), "OEM_CHARSET fonts are not found\n");
    trace_if(is_charset_font_installed(hDC, JOHAB_CHARSET), "JOHAB_CHARSET fonts are not found\n");
    trace_if(is_charset_font_installed(hDC, HEBREW_CHARSET), "HEBREW_CHARSET fonts are not found\n");
    trace_if(is_charset_font_installed(hDC, ARABIC_CHARSET), "ARABIC_CHARSET fonts are not found\n");
    trace_if(is_charset_font_installed(hDC, GREEK_CHARSET), "GREEK_CHARSET fonts are not found\n");
    trace_if(is_charset_font_installed(hDC, TURKISH_CHARSET), "TURKISH_CHARSET fonts are not found\n");
    trace_if(is_charset_font_installed(hDC, VIETNAMESE_CHARSET), "VIETNAMESE_CHARSET fonts are not found\n");
    trace_if(is_charset_font_installed(hDC, THAI_CHARSET), "THAI_CHARSET fonts are not found\n");
    trace_if(is_charset_font_installed(hDC, EASTEUROPE_CHARSET), "EASTEUROPE_CHARSET fonts are not found\n");
    trace_if(is_charset_font_installed(hDC, RUSSIAN_CHARSET), "RUSSIAN_CHARSET fonts are not found\n");
    trace_if(is_charset_font_installed(hDC, MAC_CHARSET), "MAC_CHARSET fonts are not found\n");
    trace_if(is_charset_font_installed(hDC, BALTIC_CHARSET), "BALTIC_CHARSET fonts are not found\n");

    ok(is_fixed_charset_font_installed(hDC, ANSI_CHARSET), "fixed ANSI_CHARSET fonts are not found\n");
    trace_if(is_fixed_charset_font_installed(hDC, SHIFTJIS_CHARSET), "fixed SHIFTJIS_CHARSET fonts are not found\n");
    trace_if(is_fixed_charset_font_installed(hDC, HANGUL_CHARSET), "fixed HANGUL_CHARSET fonts are not found\n");
    trace_if(is_fixed_charset_font_installed(hDC, GB2312_CHARSET), "fixed GB2312_CHARSET fonts are not found\n");
    trace_if(is_fixed_charset_font_installed(hDC, CHINESEBIG5_CHARSET), "fixed CHINESEBIG5_CHARSET fonts are not found\n");
    ok(is_fixed_charset_font_installed(hDC, OEM_CHARSET), "fixed OEM_CHARSET fonts are not found\n");
    trace_if(is_fixed_charset_font_installed(hDC, JOHAB_CHARSET), "fixed JOHAB_CHARSET fonts are not found\n");
    trace_if(is_fixed_charset_font_installed(hDC, HEBREW_CHARSET), "fixed HEBREW_CHARSET fonts are not found\n");
    trace_if(is_fixed_charset_font_installed(hDC, ARABIC_CHARSET), "fixed ARABIC_CHARSET fonts are not found\n");
    trace_if(is_fixed_charset_font_installed(hDC, GREEK_CHARSET), "fixed GREEK_CHARSET fonts are not found\n");
    trace_if(is_fixed_charset_font_installed(hDC, TURKISH_CHARSET), "fixed TURKISH_CHARSET fonts are not found\n");
    trace_if(is_fixed_charset_font_installed(hDC, VIETNAMESE_CHARSET), "fixed VIETNAMESE_CHARSET fonts are not found\n");
    trace_if(is_fixed_charset_font_installed(hDC, THAI_CHARSET), "fixed THAI_CHARSET fonts are not found\n");
    trace_if(is_fixed_charset_font_installed(hDC, EASTEUROPE_CHARSET), "fixed EASTEUROPE_CHARSET fonts are not found\n");
    trace_if(is_fixed_charset_font_installed(hDC, RUSSIAN_CHARSET), "fixed RUSSIAN_CHARSET fonts are not found\n");
    trace_if(is_fixed_charset_font_installed(hDC, MAC_CHARSET), "fixed MAC_CHARSET fonts are not found\n");
    trace_if(is_fixed_charset_font_installed(hDC, BALTIC_CHARSET), "fixed BALTIC_CHARSET fonts are not found\n");

    DeleteDC(hDC);
}

/* NOTE: TMPF_FIXED_PITCH is confusing and brain-dead. */
#define _TMPF_VAR_PITCH TMPF_FIXED_PITCH

typedef enum TRISTATE {
    TS_UNKNOWN,
    TS_TRUE,
    TS_FALSE
} TRISTATE;

typedef struct FONT_SEL_TEST {
    CHAR        FaceNameBefore[LF_FACESIZE];

    BYTE        CharSetBefore;
    BYTE        CharSetAfter;

    TRISTATE    BoldBefore;
    TRISTATE    BoldAfter;

    BYTE        ItalicBefore;
    TRISTATE    ItalicAfter;

    BYTE        UnderlineBefore;
    TRISTATE    UnderlineAfter;

    BYTE        StruckOutBefore;
    TRISTATE    StruckOutAfter;

    TRISTATE    FixedPitchBefore;
    TRISTATE    FixedPitchAfter;
} FONT_SEL_TEST;

static FONT_SEL_TEST g_Entries[] =
{
    /* Entry #0: default */
    {
        "",
        DEFAULT_CHARSET, DEFAULT_CHARSET,
        TS_UNKNOWN, TS_FALSE,
        TS_UNKNOWN, TS_FALSE,
        TS_UNKNOWN, TS_FALSE,
        TS_UNKNOWN, TS_FALSE,
        TS_UNKNOWN, TS_FALSE
    },
    /* Entry #1: symbol font*/
    {
        "",
        SYMBOL_CHARSET, SYMBOL_CHARSET
    },
    /* Entry #2: non-bold */
    {
        "",
        DEFAULT_CHARSET, DEFAULT_CHARSET,
        TS_FALSE, TS_FALSE
    },
    /* Entry #3: bold */
    {
        "",
        DEFAULT_CHARSET, DEFAULT_CHARSET,
        TS_TRUE, TS_TRUE
    },
    /* Entry #4: non-italic (without specifying bold) */
    {
        "",
        DEFAULT_CHARSET, DEFAULT_CHARSET,
        TS_UNKNOWN, TS_FALSE,
        FALSE, TS_FALSE
    },
    /* Entry #5: italic (without specifying bold) */
    {
        "",
        DEFAULT_CHARSET, DEFAULT_CHARSET,
        TS_UNKNOWN, TS_FALSE,
        TRUE, TS_TRUE
    },
    /* Entry #6: non-underline (without specifying bold) */
    {
        "",
        DEFAULT_CHARSET, DEFAULT_CHARSET,
        TS_UNKNOWN, TS_FALSE,
        FALSE, TS_UNKNOWN,
        FALSE, TS_FALSE
    },
    /* Entry #7: underline (without specifying bold) */
    {
        "",
        DEFAULT_CHARSET, DEFAULT_CHARSET,
        TS_UNKNOWN, TS_FALSE,
        FALSE, TS_UNKNOWN,
        TRUE, TS_TRUE
    },
    /* Entry #8: struck-out (without specifying bold) */
    {
        "",
        DEFAULT_CHARSET, DEFAULT_CHARSET,
        TS_UNKNOWN, TS_FALSE,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        TRUE, TS_TRUE
    },
    /* Entry #9: non-struck-out (without specifying bold) */
    {
        "",
        DEFAULT_CHARSET, DEFAULT_CHARSET,
        TS_UNKNOWN, TS_FALSE,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        FALSE, TS_FALSE
    },
    /* Entry #10: fixed-pitch (without specifying bold) */
    {
        "",
        DEFAULT_CHARSET, DEFAULT_CHARSET,
        TS_UNKNOWN, TS_FALSE,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        TS_TRUE, TS_TRUE
    },
    /* Entry #11: non-fixed-pitch (without specifying bold) */
    {
        "",
        DEFAULT_CHARSET, DEFAULT_CHARSET,
        TS_UNKNOWN, TS_FALSE,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        TS_FALSE, TS_FALSE
    },
    /* Entry #12: fixed-pitch and bold */
    {
        "",
        DEFAULT_CHARSET, DEFAULT_CHARSET,
        TS_TRUE, TS_TRUE,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        TS_TRUE, TS_TRUE
    },
    /* Entry #13: non-fixed-pitch and bold */
    {
        "",
        DEFAULT_CHARSET, DEFAULT_CHARSET,
        TS_TRUE, TS_TRUE,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        TS_FALSE, TS_FALSE
    },
    /* Entry #14: OEM_CHARSET */
    {
        "",
        OEM_CHARSET, OEM_CHARSET,
        TS_UNKNOWN, TS_FALSE
    },
    /* Entry #15: OEM_CHARSET and bold */
    {
        "",
        OEM_CHARSET, OEM_CHARSET,
        TS_TRUE, TS_TRUE
    },
    /* Entry #16: OEM_CHARSET and fixed-pitch */
    {
        "",
        OEM_CHARSET, OEM_CHARSET,
        TS_UNKNOWN, TS_FALSE,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        TS_TRUE, TS_TRUE
    },
    /* Entry #17: CHINESEBIG5_CHARSET (Chinese) */
    {
        "",
        CHINESEBIG5_CHARSET, CHINESEBIG5_CHARSET,
        TS_UNKNOWN, TS_FALSE
    },
    /* Entry #18: CHINESEBIG5_CHARSET and bold */
    {
        "",
        CHINESEBIG5_CHARSET, CHINESEBIG5_CHARSET,
        TS_TRUE, TS_TRUE
    },
    /* Entry #19: CHINESEBIG5_CHARSET and fixed-pitch */
    {
        "",
        CHINESEBIG5_CHARSET, CHINESEBIG5_CHARSET,
        TS_UNKNOWN, TS_FALSE,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        TS_TRUE, TS_TRUE
    },
    /* Entry #20: GB2312_CHARSET (Chinese) */
    {
        "",
        GB2312_CHARSET, GB2312_CHARSET,
        TS_UNKNOWN, TS_FALSE
    },
    /* Entry #21: GB2312_CHARSET and bold */
    {
        "",
        GB2312_CHARSET, GB2312_CHARSET,
        TS_TRUE, TS_TRUE
    },
    /* Entry #22: GB2312_CHARSET and fixed-pitch */
    {
        "",
        GB2312_CHARSET, GB2312_CHARSET,
        TS_UNKNOWN, TS_FALSE,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        TS_TRUE, TS_TRUE
    },
    /* Entry #23: RUSSIAN_CHARSET (Russian) */
    {
        "",
        RUSSIAN_CHARSET, RUSSIAN_CHARSET,
        TS_UNKNOWN, TS_FALSE
    },
    /* Entry #24: RUSSIAN_CHARSET and bold */
    {
        "",
        RUSSIAN_CHARSET, RUSSIAN_CHARSET,
        TS_TRUE, TS_TRUE
    },
    /* Entry #25: RUSSIAN_CHARSET and italic */
    {
        "",
        RUSSIAN_CHARSET, RUSSIAN_CHARSET,
        TS_UNKNOWN, TS_FALSE,
        TRUE, TS_TRUE,
    },
    /* Entry #26: RUSSIAN_CHARSET and fixed-pitch */
    {
        "",
        RUSSIAN_CHARSET, RUSSIAN_CHARSET,
        TS_UNKNOWN, TS_FALSE,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        TS_TRUE, TS_TRUE
    },
    /* Entry #27: SHIFTJIS_CHARSET (Japanese) */
    {
        "",
        SHIFTJIS_CHARSET, SHIFTJIS_CHARSET,
        TS_UNKNOWN, TS_FALSE
    },
    /* Entry #28: SHIFTJIS_CHARSET and bold */
    {
        "",
        SHIFTJIS_CHARSET, SHIFTJIS_CHARSET,
        TS_TRUE, TS_TRUE
    },
    /* Entry #29: SHIFTJIS_CHARSET and fixed-pitch */
    {
        "",
        SHIFTJIS_CHARSET, SHIFTJIS_CHARSET,
        TS_UNKNOWN, TS_FALSE,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        TS_TRUE, TS_TRUE
    },
    /* Entry #30: HANGUL_CHARSET (Korean) */
    {
        "",
        HANGUL_CHARSET, HANGUL_CHARSET,
        TS_UNKNOWN, TS_FALSE
    },
    /* Entry #31: HANGUL_CHARSET and bold */
    {
        "",
        HANGUL_CHARSET, HANGUL_CHARSET,
        TS_TRUE, TS_TRUE
    },
    /* Entry #32: HANGUL_CHARSET and fixed-pitch */
    {
        "",
        HANGUL_CHARSET, HANGUL_CHARSET,
        TS_UNKNOWN, TS_FALSE,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        TS_TRUE, TS_TRUE
    },
    /* Entry #33: JOHAB_CHARSET (Korean) */
    {
        "",
        JOHAB_CHARSET, JOHAB_CHARSET,
        TS_UNKNOWN, TS_UNKNOWN
    },
    /* Entry #34: JOHAB_CHARSET and bold */
    {
        "",
        JOHAB_CHARSET, JOHAB_CHARSET,
        TS_TRUE, TS_TRUE
    },
    /* Entry #35: JOHAB_CHARSET and fixed-pitch */
    {
        "",
        JOHAB_CHARSET, JOHAB_CHARSET,
        TS_UNKNOWN, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        TS_TRUE, TS_TRUE
    },
    /* Entry #36: THAI_CHARSET (Thai) */
    {
        "",
        THAI_CHARSET, THAI_CHARSET,
        TS_UNKNOWN, TS_FALSE
    },
    /* Entry #37: THAI_CHARSET and bold */
    {
        "",
        THAI_CHARSET, THAI_CHARSET,
        TS_TRUE, TS_TRUE
    },
    /* Entry #38: THAI_CHARSET and fixed-pitch */
    {
        "",
        THAI_CHARSET, THAI_CHARSET,
        TS_UNKNOWN, TS_FALSE,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        TS_TRUE, TS_TRUE
    },
    /* Entry #39: GREEK_CHARSET (Greek) */
    {
        "",
        GREEK_CHARSET, GREEK_CHARSET,
        TS_UNKNOWN, TS_FALSE
    },
    /* Entry #40: GREEK_CHARSET and bold */
    {
        "",
        GREEK_CHARSET, GREEK_CHARSET,
        TS_TRUE, TS_TRUE
    },
    /* Entry #41: GREEK_CHARSET and italic */
    {
        "",
        GREEK_CHARSET, GREEK_CHARSET,
        TS_UNKNOWN, TS_FALSE,
        TRUE, TS_TRUE
    },
    /* Entry #42: GREEK_CHARSET and fixed-pitch */
    {
        "",
        GREEK_CHARSET, GREEK_CHARSET,
        TS_UNKNOWN, TS_FALSE,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        TS_TRUE, TS_TRUE
    },
    /* Entry #43: "Marlett" */
    {
        "Marlett",
        DEFAULT_CHARSET, SYMBOL_CHARSET
    },
    /* Entry #43: "Arial" */
    {
        "Arial",
        DEFAULT_CHARSET, DEFAULT_CHARSET,
        TS_UNKNOWN, TS_FALSE,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        TS_UNKNOWN, TS_FALSE
    },
    /* Entry #44: "Courier" */
    {
        "Courier",
        DEFAULT_CHARSET, DEFAULT_CHARSET,
        TS_UNKNOWN, TS_FALSE,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        FALSE, TS_UNKNOWN,
        TS_UNKNOWN, TS_TRUE
    }
};

static void
Test_FontSelectionEntry(HDC hDC, UINT nIndex, FONT_SEL_TEST *Entry)
{
    LOGFONTA        lf;
    HFONT           hFont;
    HGDIOBJ         hFontOld;
    TEXTMETRICA     tm;

    ZeroMemory(&lf, sizeof(lf));

    if (Entry->FaceNameBefore[0])
        lstrcpynA(lf.lfFaceName, Entry->FaceNameBefore, _countof(lf.lfFaceName));

    lf.lfCharSet = Entry->CharSetBefore;

    if (Entry->BoldBefore == TS_TRUE)
        lf.lfWeight = FW_BOLD;
    else if (Entry->BoldBefore == TS_FALSE)
        lf.lfWeight = FW_NORMAL;
    else
        lf.lfWeight = FW_DONTCARE;

    lf.lfItalic = Entry->ItalicBefore;
    lf.lfUnderline = Entry->UnderlineBefore;
    lf.lfStrikeOut = Entry->StruckOutBefore;

    if (Entry->FixedPitchBefore == TS_TRUE)
        lf.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
    else if (Entry->FixedPitchBefore == TS_FALSE)
        lf.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE;
    else
        lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

    hFont = CreateFontIndirectA(&lf);
    ok(hFont != NULL, "Entry #%u: hFont failed\n", nIndex);

    hFontOld = SelectObject(hDC, hFont);
    {
        ok(GetTextMetricsA(hDC, &tm), "Entry #%u: GetTextMetricsA failed\n", nIndex);

        if (Entry->CharSetAfter != DEFAULT_CHARSET)
            ok(tm.tmCharSet == Entry->CharSetAfter, "Entry #%u: CharSet mismatched, it was %u\n", nIndex, tm.tmCharSet);

        if (Entry->BoldAfter == TS_TRUE)
            ok(tm.tmWeight >= FW_BOLD, "Entry #%u: Weight was non-bold\n", nIndex);
        else if (Entry->BoldAfter == TS_FALSE)
            ok(tm.tmWeight <= FW_MEDIUM, "Entry #%u: Weight was bold\n", nIndex);

        if (Entry->ItalicAfter == TS_TRUE)
            ok(tm.tmItalic, "Entry #%u: Italic was non-italic\n", nIndex);
        else if (Entry->ItalicAfter == TS_FALSE)
            ok(!tm.tmItalic, "Entry #%u: Italic was italic\n", nIndex);

        if (Entry->UnderlineAfter == TS_TRUE)
            ok(tm.tmUnderlined, "Entry #%u: Underlined was FALSE\n", nIndex);
        else if (Entry->UnderlineAfter == TS_FALSE)
            ok(!tm.tmUnderlined, "Entry #%u: Underlined was TRUE\n", nIndex);

        if (Entry->StruckOutAfter == TS_TRUE)
            ok(tm.tmStruckOut, "Entry #%u: Struck-out was FALSE\n", nIndex);
        else if (Entry->StruckOutAfter == TS_FALSE)
            ok(!tm.tmStruckOut, "Entry #%u: Struck-out was TRUE\n", nIndex);
#if 0 // FIXME: fails on WHS testbot
        if (Entry->FixedPitchAfter == TS_TRUE)
            ok(!(tm.tmPitchAndFamily & _TMPF_VAR_PITCH), "Entry #%u: Pitch mismatched, it was non-fixed-pitch\n", nIndex);
        else if (Entry->FixedPitchAfter == TS_FALSE)
            ok((tm.tmPitchAndFamily & _TMPF_VAR_PITCH), "Entry #%u: Pitch mismatched, it was fixed-pitch\n", nIndex);
#endif
    }
    SelectObject(hDC, hFontOld);
    DeleteObject(hFont);
}

static void
Test_FontSelection(void)
{
    UINT nIndex;
    HDC hDC;

    hDC = CreateCompatibleDC(NULL);
    for (nIndex = 0; nIndex < _countof(g_Entries); ++nIndex)
    {
        if (!is_charset_font_installed(hDC, g_Entries[nIndex].CharSetBefore))
            skip("charset not available: 0x%x\n", g_Entries[nIndex].CharSetBefore);
        else
            Test_FontSelectionEntry(hDC, nIndex, g_Entries + nIndex);
    }
    DeleteDC(hDC);
}


START_TEST(CreateFontIndirect)
{
    Test_CreateFontIndirectA();
    Test_CreateFontIndirectW();
    Test_CreateFontIndirectExA();
    Test_CreateFontIndirectExW();
    Test_FontPresence();
    Test_FontSelection();
}


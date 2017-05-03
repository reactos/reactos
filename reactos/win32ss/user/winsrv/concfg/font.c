/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/concfg/font.c
 * PURPOSE:         Console Fonts Management
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *                  Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"
#include <winuser.h>

#include "settings.h"
#include "font.h"
// #include "concfg.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ******************************************************************/

/* Retrieves the character set associated with a given code page */
BYTE
CodePageToCharSet(
    IN UINT CodePage)
{
    CHARSETINFO CharInfo;
    if (TranslateCharsetInfo((LPDWORD)CodePage, &CharInfo, TCI_SRCCODEPAGE))
        return CharInfo.ciCharset;
    else
        return DEFAULT_CHARSET;
}

HFONT
CreateConsoleFontEx(
    IN LONG Height,
    IN LONG Width OPTIONAL,
    IN OUT LPWSTR FaceName, // Points to a WCHAR array of LF_FACESIZE elements
    IN ULONG FontFamily,
    IN ULONG FontWeight,
    IN UINT  CodePage)
{
    LOGFONTW lf;

    RtlZeroMemory(&lf, sizeof(lf));

    lf.lfHeight = Height;
    lf.lfWidth  = Width;

    lf.lfEscapement  = 0;
    lf.lfOrientation = 0; // TA_BASELINE; // TA_RTLREADING; when the console supports RTL?
    // lf.lfItalic = lf.lfUnderline = lf.lfStrikeOut = FALSE;
    lf.lfWeight  = FontWeight;
    lf.lfCharSet = CodePageToCharSet(CodePage);
    lf.lfOutPrecision  = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = (BYTE)(FIXED_PITCH | FontFamily);

    if (!IsValidConsoleFont(FaceName, CodePage))
        StringCchCopyW(FaceName, LF_FACESIZE, L"Terminal");

    StringCchCopyNW(lf.lfFaceName, ARRAYSIZE(lf.lfFaceName),
                    FaceName, LF_FACESIZE);

    return CreateFontIndirectW(&lf);
}

HFONT
CreateConsoleFont2(
    IN LONG Height,
    IN LONG Width OPTIONAL,
    IN OUT PCONSOLE_STATE_INFO ConsoleInfo)
{
    return CreateConsoleFontEx(Height,
                               Width,
                               ConsoleInfo->FaceName,
                               ConsoleInfo->FontFamily,
                               ConsoleInfo->FontWeight,
                               ConsoleInfo->CodePage);
}

HFONT
CreateConsoleFont(
    IN OUT PCONSOLE_STATE_INFO ConsoleInfo)
{
    /*
     * Format:
     * Width  = FontSize.X = LOWORD(FontSize);
     * Height = FontSize.Y = HIWORD(FontSize);
     */
    /* NOTE: FontSize is always in cell height/width units (pixels) */
    return CreateConsoleFontEx((LONG)(ULONG)ConsoleInfo->FontSize.Y,
                               (LONG)(ULONG)ConsoleInfo->FontSize.X,
                               ConsoleInfo->FaceName,
                               ConsoleInfo->FontFamily,
                               ConsoleInfo->FontWeight,
                               ConsoleInfo->CodePage);
}

BOOL
GetFontCellSize(
    IN HDC hDC OPTIONAL,
    IN HFONT hFont,
    OUT PUINT Height,
    OUT PUINT Width)
{
    BOOL Success = FALSE;
    HDC hOrgDC = hDC;
    HFONT hOldFont;
    // LONG LogSize, PointSize;
    LONG CharWidth, CharHeight;
    TEXTMETRICW tm;
    // SIZE CharSize;

    if (!hDC)
        hDC = GetDC(NULL);

    hOldFont = SelectObject(hDC, hFont);
    if (hOldFont == NULL)
    {
        DPRINT1("GetFontCellSize: SelectObject failed\n");
        goto Quit;
    }

/*
 * See also: Display_SetTypeFace in applications/fontview/display.c
 */

    /*
     * Note that the method with GetObjectW just returns
     * the original parameters with which the font was created.
     */
    if (!GetTextMetricsW(hDC, &tm))
    {
        DPRINT1("GetFontCellSize: GetTextMetrics failed\n");
        goto Cleanup;
    }

    CharHeight = tm.tmHeight + tm.tmExternalLeading;

#if 0
    /* Measure real char width more precisely if possible */
    if (GetTextExtentPoint32W(hDC, L"R", 1, &CharSize))
        CharWidth = CharSize.cx;
#else
    CharWidth = tm.tmAveCharWidth; // tm.tmMaxCharWidth;
#endif

#if 0
    /*** Logical to Point size ***/
    LogSize   = tm.tmHeight - tm.tmInternalLeading;
    PointSize = MulDiv(LogSize, 72, GetDeviceCaps(hDC, LOGPIXELSY));
    /*****************************/
#endif

    *Height = (UINT)CharHeight;
    *Width  = (UINT)CharWidth;
    Success = TRUE;

Cleanup:
    SelectObject(hDC, hOldFont);
Quit:
    if (!hOrgDC)
        ReleaseDC(NULL, hDC);

    return Success;
}

BOOL
IsValidConsoleFont2(
    IN PLOGFONTW lplf,
    IN PNEWTEXTMETRICW lpntm,
    IN DWORD FontType,
    IN UINT CodePage)
{
    LPCWSTR FaceName = lplf->lfFaceName;

    /* Record the font's attributes (Fixedwidth and Truetype) */
    // BOOL fFixed    = ((lplf->lfPitchAndFamily & 0x03) == FIXED_PITCH);
    // BOOL fTrueType = (lplf->lfOutPrecision == OUT_STROKE_PRECIS);

    /*
     * According to: http://support.microsoft.com/kb/247815
     * the criteria for console-eligible fonts are:
     * - The font must be a fixed-pitch font.
     * - The font cannot be an italic font.
     * - The font cannot have a negative A or C space.
     * - If it is a TrueType font, it must be FF_MODERN.
     * - If it is not a TrueType font, it must be OEM_CHARSET.
     *
     * Non documented: vertical fonts are forbidden (their name start with a '@').
     *
     * Additional criteria for Asian installations:
     * - If it is not a TrueType font, the face name must be "Terminal".
     * - If it is an Asian TrueType font, it must also be an Asian character set.
     *
     * To install additional TrueType fonts to be available for the console,
     * add entries of type REG_SZ named "0", "00" etc... in:
     * HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\Console\TrueTypeFont
     * The names of the fonts listed there should match those in:
     * HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\Fonts
     */

     /*
      * In ReactOS we relax some of the criteria:
      * - We allow fixed-pitch FF_MODERN (Monospace) TrueType fonts
      *   that can be italic or have negative A or C space.
      * - If it is not a TrueType font, it can be from another character set
      *   than OEM_CHARSET.
      * - We do not look into the magic registry key mentioned above.
      */

    /* Reject variable width fonts */
    if (((lplf->lfPitchAndFamily & 0x03) != FIXED_PITCH)
#if 0 /* Reject italic and TrueType fonts with negative A or C space */
        || (lplf->lfItalic)
        || !(lpntm->ntmFlags & NTM_NONNEGATIVE_AC)
#endif
        )
    {
        DPRINT1("Font '%S' rejected because it%s (lfPitchAndFamily = %d).\n",
                FaceName, !(lplf->lfPitchAndFamily & FIXED_PITCH) ? "'s not FIXED_PITCH"
                                                                  : (!(lpntm->ntmFlags & NTM_NONNEGATIVE_AC) ? " has negative A or C space"
                                                                                                             : " is broken"),
                lplf->lfPitchAndFamily);
        return FALSE;
    }

    /* Reject TrueType fonts that are not FF_MODERN */
    if ((FontType == TRUETYPE_FONTTYPE) && ((lplf->lfPitchAndFamily & 0xF0) != FF_MODERN))
    {
        DPRINT1("TrueType font '%S' rejected because it's not FF_MODERN (lfPitchAndFamily = %d)\n",
                FaceName, lplf->lfPitchAndFamily);
        return FALSE;
    }

    /* Is the current code page Chinese, Japanese or Korean? */
    if (IsCJKCodePage(CodePage))
    {
        /* It's Asian */
        if (FontType == TRUETYPE_FONTTYPE)
        {
            if (lplf->lfCharSet != CodePageToCharSet(CodePage))
            {
                DPRINT1("TrueType font '%S' rejected because it's not user Asian charset (lfCharSet = %d)\n",
                        FaceName, lplf->lfCharSet);
                return FALSE;
            }
        }
        else
        {
            /* Reject non-TrueType fonts that are not Terminal */
            if (wcscmp(FaceName, L"Terminal") != 0)
            {
                DPRINT1("Non-TrueType font '%S' rejected because it's not Terminal\n", FaceName);
                return FALSE;
            }
        }
    }
    else
    {
        /* Not CJK */
        if ((FontType != TRUETYPE_FONTTYPE) &&
            (lplf->lfCharSet != ANSI_CHARSET) &&
            (lplf->lfCharSet != DEFAULT_CHARSET) &&
            (lplf->lfCharSet != OEM_CHARSET))
        {
            DPRINT1("Non-TrueType font '%S' rejected because it's not ANSI_CHARSET or DEFAULT_CHARSET or OEM_CHARSET (lfCharSet = %d)\n",
                    FaceName, lplf->lfCharSet);
            return FALSE;
        }
    }

    /* Reject fonts that are vertical (tategaki) */
    if (FaceName[0] == L'@')
    {
        DPRINT1("Font '%S' rejected because it's vertical\n", FaceName);
        return FALSE;
    }

    /* All good */
    return TRUE;
}

typedef struct _IS_VALID_CONSOLE_FONT_PARAM
{
    BOOL IsValidFont;
    UINT CodePage;
} IS_VALID_CONSOLE_FONT_PARAM, *PIS_VALID_CONSOLE_FONT_PARAM;

static BOOL CALLBACK
IsValidConsoleFontProc(
    IN PLOGFONTW lplf,
    IN PNEWTEXTMETRICW lpntm,
    IN DWORD  FontType,
    IN LPARAM lParam)
{
    PIS_VALID_CONSOLE_FONT_PARAM Param = (PIS_VALID_CONSOLE_FONT_PARAM)lParam;
    Param->IsValidFont = IsValidConsoleFont2(lplf, lpntm, FontType, Param->CodePage);

    /* Stop the enumeration now */
    return FALSE;
}

BOOL
IsValidConsoleFont(
    IN LPCWSTR FaceName,
    IN UINT CodePage)
{
    IS_VALID_CONSOLE_FONT_PARAM Param;
    HDC hDC;
    LOGFONTW lf;

    Param.IsValidFont = FALSE;
    Param.CodePage = CodePage;

    RtlZeroMemory(&lf, sizeof(lf));
    lf.lfCharSet = DEFAULT_CHARSET; // CodePageToCharSet(CodePage);
    // lf.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
    StringCchCopyW(lf.lfFaceName, ARRAYSIZE(lf.lfFaceName), FaceName);

    hDC = GetDC(NULL);
    EnumFontFamiliesExW(hDC, &lf, (FONTENUMPROCW)IsValidConsoleFontProc, (LPARAM)&Param, 0);
    ReleaseDC(NULL, hDC);

    return Param.IsValidFont;
}

/* EOF */

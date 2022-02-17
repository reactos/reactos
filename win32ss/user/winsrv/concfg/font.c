/*
 * PROJECT:     ReactOS Console Server DLL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Console GDI Fonts Management.
 * COPYRIGHT:   Copyright 2017-2022 Hermès Bélusca-Maïto
 *              Copyright 2017 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"
#include <winuser.h>

#include "settings.h"
#include "font.h"
// #include "concfg.h"

#define NDEBUG
#include <debug.h>

#define DBGFNT  DPRINT
#define DBGFNT1 DPRINT1


/* GLOBALS ********************************************************************/

#define TERMINAL_FACENAME           L"Terminal"
#define DEFAULT_NON_DBCS_FONTFACE   L"Lucida Console" // L"Consolas"
#define DEFAULT_TT_FONT_FACENAME    L"__DefaultTTFont__"

/* TrueType font list cache */
SINGLE_LIST_ENTRY TTFontCache = { NULL };

// NOTE: Used to tag code that makes sense only with a font cache.
// #define FONT_CACHE_PRESENT


/* FUNCTIONS ******************************************************************/

/**
 * @brief
 * Retrieves the character set associated with a given code page.
 *
 * @param[in]   CodePage
 * The code page to convert.
 *
 * @return
 * The character set corresponding to the code page, or @b DEFAULT_CHARSET.
 **/
BYTE
CodePageToCharSet(
    _In_ UINT CodePage)
{
    CHARSETINFO CharInfo;
    if (TranslateCharsetInfo(UlongToPtr(CodePage), &CharInfo, TCI_SRCCODEPAGE))
        return (BYTE)CharInfo.ciCharset;
    else
        return DEFAULT_CHARSET;
}

/*****************************************************************************/

typedef struct _FIND_SUITABLE_FONT_PROC_PARAM
{
    /* Search criteria */
    _In_reads_or_z_(LF_FACESIZE) PCWSTR AltFaceName;
    FONT_DATA SearchFont;
    UINT CodePage;
    BOOL StrictSearch; // TRUE to do strict search; FALSE for relaxed criteria.

    /* Candidate font data */
    BOOL FontFound;    // TRUE/FALSE if we have/haven't found a suitable font.
    FONT_DATA CandidateFont;
    WCHAR CandidateFaceName[LF_FACESIZE];
} FIND_SUITABLE_FONT_PROC_PARAM, *PFIND_SUITABLE_FONT_PROC_PARAM;

#define TM_IS_TT_FONT(x)    (((x) & TMPF_TRUETYPE) == TMPF_TRUETYPE)
#define SIZE_EQUAL(s1, s2)  (((s1).X == (s2).X) && ((s1).Y == (s2).Y))

/**
 * @brief   EnumFontFamiliesEx() callback helper for FindSuitableFont().
 *
 * @remark
 * It implements a nearly-identical console-suitable font search
 * algorithm based on the one from FindCreateFont()
 * https://github.com/microsoft/terminal/blob/main/src/propsheet/fontdlg.cpp#L1113
 * excepting that for now, it does not support an internal font cache.
 **/
static BOOL CALLBACK
FindSuitableFontProc(
    _In_ PLOGFONTW lplf,
    _In_ PNEWTEXTMETRICW lpntm,
    _In_ DWORD  FontType,
    _In_ LPARAM lParam)
{
    PFIND_SUITABLE_FONT_PROC_PARAM Param = (PFIND_SUITABLE_FONT_PROC_PARAM)lParam;
    PFONT_DATA SearchFont = &Param->SearchFont;

    if (!IsValidConsoleFont2(lplf, lpntm, FontType, Param->CodePage))
    {
        /* This font does not suit us; continue enumeration */
        return TRUE;
    }

#ifndef FONT_CACHE_PRESENT
    /*
     * Since we don't cache all the possible font sizes for TrueType fonts,
     * we cannot check our requested size (and weight) against the enumerated
     * one; therefore reset the enumerated values to the requested ones.
     * On the contrary, Raster fonts get their specific font sizes (and weights)
     * enumerated separately, so for them we can keep the enumerated values.
     */
    if (FontType == TRUETYPE_FONTTYPE)
    {
        lplf->lfHeight = SearchFont->Size.Y;
        lplf->lfWidth  = 0; // SearchFont->Size.X;
        lplf->lfWeight = FW_NORMAL;
    }
#endif

    if (Param->StrictSearch)
    {
        /*
         * Find whether this is an exact match.
         */

        /* If looking for a particular family, skip non-matches */
        if ((SearchFont->Family != 0) &&
            ((BYTE)SearchFont->Family != (lplf->lfPitchAndFamily & 0xF0)))
        {
            /* Continue enumeration */
            return TRUE;
        }

        /* Skip non-matching sizes */
#if 0
        if ((FontInfo[i].SizeWant.Y != Size.Y) &&
            !SIZE_EQUAL(FontInfo[i].Size, Size))
#endif
        if ((lplf->lfHeight != SearchFont->Size.Y) &&
            !(lplf->lfWidth == SearchFont->Size.X &&
              lplf->lfHeight == SearchFont->Size.Y))
        {
            /* Continue enumeration */
            return TRUE;
        }

        /* Skip non-matching weights */
        if ((SearchFont->Weight != 0) &&
            (SearchFont->Weight != lplf->lfWeight))
        {
            /* Continue enumeration */
            return TRUE;
        }

        /* NOTE: We are making the font enumeration at fixed CharSet,
         * with the one specified in the parameter block. */
        ASSERT(lplf->lfCharSet == SearchFont->CharSet);

        if ((FontType != TRUETYPE_FONTTYPE) && // !TM_IS_TT_FONT(lpntm->tmPitchAndFamily)
            (lplf->lfCharSet != SearchFont->CharSet) &&
            !(lplf->lfCharSet == OEM_CHARSET && IsCJKCodePage(Param->CodePage))) // g_fEastAsianSystem
        {
            /* Continue enumeration */
            return TRUE;
        }

        /*
         * Size (and maybe family) match. If we don't care about the name or
         * if it matches, use this font. Otherwise, if name doesn't match and
         * it is a raster font, consider it.
         *
         * NOTE: The font face names are case-sensitive.
         */
        if (!SearchFont->FaceName || !*(SearchFont->FaceName) ||
            (wcscmp(lplf->lfFaceName, SearchFont->FaceName) == 0) ||
            (wcscmp(lplf->lfFaceName, Param->AltFaceName) == 0))
        {
            // FontIndex = i;

            PFONT_DATA CandidateFont = &Param->CandidateFont;

            CandidateFont->FaceName = Param->CandidateFaceName;
            StringCchCopyNW(Param->CandidateFaceName,
                            ARRAYSIZE(Param->CandidateFaceName),
                            lplf->lfFaceName, ARRAYSIZE(lplf->lfFaceName));

            CandidateFont->Weight = lplf->lfWeight;
            CandidateFont->Family = (lplf->lfPitchAndFamily & 0xF0);

            CandidateFont->Size.X = lplf->lfWidth;
            CandidateFont->Size.Y = lplf->lfHeight;

            CandidateFont->CharSet = lplf->lfCharSet;

            /* The font is found, stop enumeration */
            Param->FontFound = TRUE;
            return FALSE;
        }
        else if (FontType != TRUETYPE_FONTTYPE) // !TM_IS_TT_FONT(lpntm->tmPitchAndFamily)
        {
            // FontIndex = i;

            PFONT_DATA CandidateFont = &Param->CandidateFont;

            CandidateFont->FaceName = Param->CandidateFaceName;
            StringCchCopyNW(Param->CandidateFaceName,
                            ARRAYSIZE(Param->CandidateFaceName),
                            lplf->lfFaceName, ARRAYSIZE(lplf->lfFaceName));

            CandidateFont->Weight = lplf->lfWeight;
            CandidateFont->Family = (lplf->lfPitchAndFamily & 0xF0);

            CandidateFont->Size.X = lplf->lfWidth;
            CandidateFont->Size.Y = lplf->lfHeight;

            CandidateFont->CharSet = lplf->lfCharSet;

            /* A close Raster Font fit was found; only the name doesn't match.
             * Continue enumeration to see whether we can find better. */
            Param->FontFound = TRUE;
        }
    }
    else // !Param->StrictSearch
    {
        /*
         * Failed to find exact match, even after enumeration, so now
         * try to find a font of same family and same size or bigger.
         */

        if (IsCJKCodePage(Param->CodePage)) // g_fEastAsianSystem
        {
            if ((SearchFont->Family != 0) &&
                ((BYTE)SearchFont->Family != (lplf->lfPitchAndFamily & 0xF0)))
            {
                /* Continue enumeration */
                return TRUE;
            }

            if ((FontType != TRUETYPE_FONTTYPE) && // !TM_IS_TT_FONT(lpntm->tmPitchAndFamily)
                (lplf->lfCharSet != SearchFont->CharSet))
            {
                /* Continue enumeration */
                return TRUE;
            }
        }
        else
        {
            if (// (SearchFont->Family != 0) &&
                ((BYTE)SearchFont->Family != (lplf->lfPitchAndFamily & 0xF0)))
            {
                /* Continue enumeration */
                return TRUE;
            }
        }

        if ((lplf->lfHeight >= SearchFont->Size.Y) &&
            (lplf->lfWidth  >= SearchFont->Size.X))
        {
            /* Same family, size >= desired */
            // FontIndex = i;

            PFONT_DATA CandidateFont = &Param->CandidateFont;

            CandidateFont->FaceName = Param->CandidateFaceName;
            StringCchCopyNW(Param->CandidateFaceName,
                            ARRAYSIZE(Param->CandidateFaceName),
                            lplf->lfFaceName, ARRAYSIZE(lplf->lfFaceName));

            CandidateFont->Weight = lplf->lfWeight;
            CandidateFont->Family = (lplf->lfPitchAndFamily & 0xF0);

            CandidateFont->Size.X = lplf->lfWidth;
            CandidateFont->Size.Y = lplf->lfHeight;

            CandidateFont->CharSet = lplf->lfCharSet;

            /* The font is found, stop enumeration */
            Param->FontFound = TRUE;
            return FALSE;
        }
    }

    /* Continue enumeration */
    return TRUE;
}

/**
 * @brief
 * Finds a font suitable for the given code page, based on the current font
 * and its characteristics provided in input.
 *
 * @param[in,out]   FontData
 * In input: The face name and characteristics of the font to search for,
 * possibly getting a best match.
 * In output: The face name and characteristics of the suitable font,
 * in case of success.
 *
 * @param[in]   CodePage
 * The code page the font has to support.
 *
 * @return
 * @b TRUE in case a suitable font has been found. Its name and characteristics
 * are returned in @b FontData. @b FALSE if no suitable font has been found.
 **/
static BOOL
FindSuitableFont(
    _Inout_ PFONT_DATA FontData,
    _In_ UINT CodePage)
{
    FIND_SUITABLE_FONT_PROC_PARAM Param;
    _Inout_updates_z_(LF_FACESIZE) PWSTR FaceName;
    HDC hDC;
    LOGFONTW lf;
    PTT_FONT_ENTRY FontEntry;

    /* Save the original FaceName pointer */
    FaceName = FontData->FaceName;

    /* Save our current search criteria */
    RtlZeroMemory(&Param, sizeof(Param));
    Param.SearchFont = *FontData;

    Param.SearchFont.CharSet = CodePageToCharSet(CodePage);
    Param.CodePage = CodePage;

    if (/* !FaceName || */ !*FaceName)
    {
        /* Find and use a default Raster font */

        /* Use "Terminal" as the fallback */
        StringCchCopyW(FaceName, LF_FACESIZE, TERMINAL_FACENAME);
#if 0
        // FIXME: CJK font choose workaround: Don't choose Asian
        // charset font if there is no preferred font for CJK.
        if (IsCJKCodePage(CodePage))
            FontData->CharSet = ANSI_CHARSET;
#endif
        FontData->Family &= ~TMPF_TRUETYPE;
    }
    else if (wcscmp(FaceName, DEFAULT_TT_FONT_FACENAME) == 0)
    {
        /* Find and use a default TrueType font */
        FontEntry = FindCachedTTFont(NULL, CodePage);
        if (FontEntry)
        {
            StringCchCopyW(FaceName, LF_FACESIZE, FontEntry->FaceName);
        }
        else
        {
            StringCchCopyW(FaceName, LF_FACESIZE, DEFAULT_NON_DBCS_FONTFACE);
        }
        FontData->Family |= TMPF_TRUETYPE;
    }

    /* Search for a TrueType alternative face name */
    FontEntry = FindCachedTTFont(FaceName, CodePage);
    if (FontEntry)
    {
        /* NOTE: The font face names are case-sensitive */
        if (wcscmp(FontEntry->FaceName, FaceName) == 0)
            Param.AltFaceName = FontEntry->FaceNameAlt;
        else if (wcscmp(FontEntry->FaceNameAlt, FaceName) == 0)
            Param.AltFaceName = FontEntry->FaceName;
    }
    else
    {
        Param.AltFaceName = FaceName;
    }

    /* Initialize the search: start with a strict search, then a relaxed one */
    Param.FontFound = FALSE;

    Param.StrictSearch = TRUE;
SearchAgain:
    /*
     * Enumerate all fonts with the given character set.
     * We will match them with the search criteria.
     */
    RtlZeroMemory(&lf, sizeof(lf));
    lf.lfCharSet = Param.SearchFont.CharSet;
    // lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;

    hDC = GetDC(NULL);
    EnumFontFamiliesExW(hDC, &lf, (FONTENUMPROCW)FindSuitableFontProc, (LPARAM)&Param, 0);
    ReleaseDC(NULL, hDC);

    /* If we failed to find any font, search again with relaxed criteria */
    if (Param.StrictSearch && !Param.FontFound)
    {
        Param.StrictSearch = FALSE;
        goto SearchAgain;
    }

    /* If no font was found again, return failure */
    if (!Param.FontFound)
        return FALSE;

    /* Return the font details */
    *FontData = Param.CandidateFont;
    FontData->FaceName = FaceName; // Restore the original FaceName pointer.
    StringCchCopyNW(FaceName, LF_FACESIZE,
                    Param.CandidateFaceName,
                    ARRAYSIZE(Param.CandidateFaceName));

    return TRUE;
}

/**
 * @brief
 * Validates and creates a suitable console font based on the font
 * characteristics given in input.
 *
 * @param[in]   FontData
 * The face name and characteristics of the font to create.
 *
 * @param[in]   CodePage
 * The code page the font has to support.
 *
 * @return
 * A GDI handle to the created font, or @b NULL in case of failure.
 **/
static HFONT
CreateConsoleFontWorker(
    _In_ PFONT_DATA FontData,
    _In_ UINT CodePage)
{
    LOGFONTW lf;

    RtlZeroMemory(&lf, sizeof(lf));

    lf.lfHeight = (LONG)(ULONG)FontData->Size.Y;
    lf.lfWidth  = (LONG)(ULONG)FontData->Size.X;

    lf.lfEscapement  = 0;
    lf.lfOrientation = 0; // TA_BASELINE; // TA_RTLREADING; when the console supports RTL?
    // lf.lfItalic = lf.lfUnderline = lf.lfStrikeOut = FALSE;
    lf.lfWeight  = FontData->Weight;
    lf.lfCharSet = CodePageToCharSet(CodePage);
    lf.lfOutPrecision  = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;

    /* Set the mandatory flags and remove those that we do not support */
    lf.lfPitchAndFamily = (BYTE)( (FIXED_PITCH | FF_MODERN | FontData->Family) &
                                 ~(VARIABLE_PITCH | FF_DECORATIVE | FF_ROMAN | FF_SCRIPT | FF_SWISS));

    if (!IsValidConsoleFont(FontData->FaceName, CodePage))
        return NULL;

    StringCchCopyNW(lf.lfFaceName, ARRAYSIZE(lf.lfFaceName),
                    FontData->FaceName, LF_FACESIZE);

    return CreateFontIndirectW(&lf);
}

/*****************************************************************************/

/**
 * @brief
 * Validates and creates a suitable console font based on the font
 * characteristics given in input.
 *
 * @param[in]   Height
 * The font height in cell units (pixels).
 *
 * @param[in,opt]   Width
 * The font width in cell units (pixels).
 *
 * @param[in,out]   FaceName
 * A pointer to a maximally @b LF_FACESIZE-sized buffer.
 * In input: The buffer contains the face name of the font to try to create.
 * In output: The buffer receives the face name of the font that has been
 * created, in case of success. It may, or may not be, identical to the face
 * name provided in input, in case a substitute font has been chosen.
 *
 * @param[in]   FontWeight
 * The font weight.
 *
 * @param[in]   FontFamily
 * The font family.
 *
 * @param[in]   CodePage
 * The code page the font has to support.
 *
 * @param[in]   UseDefaultFallback
 * Whether (@b TRUE) or not (@b FALSE) to use a default fallback font in case
 * neither the specified font nor any substitute font could be found and
 * created for the specified code page.
 *
 * @param[out]  FontData
 * The face name and characteristics of the created font.
 *
 * @return
 * A GDI handle to the created font, or @b NULL in case of failure.
 *
 * @remark
 * Similar to FindCreateFont()
 * https://github.com/microsoft/terminal/blob/main/src/propsheet/fontdlg.cpp#L1113
 * but:
 * - does not support an internal font cache for now;
 * - returns a font handle (and not a font index to the cache).
 **/
HFONT
CreateConsoleFontEx(
    _In_     LONG Height,
    _In_opt_ LONG Width,
    _Inout_updates_z_(LF_FACESIZE)
         PWSTR FaceName,
    _In_ ULONG FontWeight,
    _In_ ULONG FontFamily,
    _In_ UINT  CodePage,
    _In_ BOOL  UseDefaultFallback,
    _Out_ PFONT_DATA FontData)
{
    HFONT hFont;

    FontData->FaceName = FaceName;
    FontData->Weight = FontWeight;
    FontData->Family = FontFamily;
    /* NOTE: FontSize is always in cell height/width units (pixels) */
    FontData->Size.X = Width;
    FontData->Size.Y = Height;
    FontData->CharSet = 0; // CodePageToCharSet(CodePage);

    if (/* !FaceName || */ !*FaceName || wcscmp(FaceName, DEFAULT_TT_FONT_FACENAME) == 0)
    {
        /* We do not have an actual font face name yet and should find one.
         * Call FindSuitableFont() to determine the default font to use. */
    }
    else
    {
        hFont = CreateConsoleFontWorker(FontData, CodePage);
        if (hFont)
            return hFont;

        DBGFNT1("CreateConsoleFont('%S') failed - Try to find a suitable font...\n",
                FaceName);
    }

    /*
     * We could not create a font with the default settings.
     * Try to find a suitable font and retry.
     */
    if (!FindSuitableFont(FontData, CodePage))
    {
        /* We could not find any suitable font, fall back
         * to some default one if required to do so. */
        DBGFNT1("FindSuitableFont could not find anything - %s\n",
                UseDefaultFallback ? "Falling back to 'Terminal'"
                                   : "Bailing out");

        /* No fallback: no font! */
        if (!UseDefaultFallback)
            return NULL;

        //
        // FIXME: See also !*FaceName case in FindSuitableFont().
        //
        /* Use "Terminal" as the fallback */
        StringCchCopyW(FaceName, LF_FACESIZE, TERMINAL_FACENAME);
#if 0
        // FIXME: CJK font choose workaround: Don't choose Asian
        // charset font if there is no preferred font for CJK.
        if (IsCJKCodePage(CodePage))
            FontData->CharSet = ANSI_CHARSET;
#endif
        FontData->Family &= ~TMPF_TRUETYPE;
    }
    else
    {
        DBGFNT1("FindSuitableFont found: '%S', size (%d x %d)\n",
                FaceName, FontData->Size.X, FontData->Size.Y);
    }

    /* Retry creating the font */
    hFont = CreateConsoleFontWorker(FontData, CodePage);
    if (!hFont)
        DBGFNT1("CreateConsoleFont('%S') failed\n", FaceName);

    return hFont;
}

/**
 * @brief
 * A wrapper for CreateConsoleFontEx().
 *
 * @param[in]   Height
 * The font height in cell units (pixels).
 *
 * @param[in,opt]   Width
 * The font width in cell units (pixels).
 *
 * @param[in,out]   ConsoleInfo
 * A pointer to console settings information, containing in particular
 * (in input) the face name and characteristics of the font to create
 * with the current console code page.
 * In output, the font information gets updated.
 * Note that a default fallback font is always being used in case neither
 * the specified font nor any substitute font could be found and created
 * for the specified code page.
 *
 * @return
 * A GDI handle to the created font, or @b NULL in case of failure.
 *
 * @see CreateConsoleFontEx(), CreateConsoleFont()
 **/
HFONT
CreateConsoleFont2(
    _In_     LONG Height,
    _In_opt_ LONG Width,
    _Inout_  PCONSOLE_STATE_INFO ConsoleInfo)
{
    FONT_DATA FontData;
    HFONT hFont;

    hFont = CreateConsoleFontEx(Height,
                                Width,
                                ConsoleInfo->FaceName,
                                ConsoleInfo->FontWeight,
                                ConsoleInfo->FontFamily,
                                ConsoleInfo->CodePage,
                                TRUE, // UseDefaultFallback
                                &FontData);
    if (hFont)
    {
        ConsoleInfo->FontWeight = FontData.Weight;
        ConsoleInfo->FontFamily = FontData.Family;
    }

    return hFont;
}

/**
 * @brief
 * A wrapper for CreateConsoleFontEx().
 *
 * @param[in,out]   ConsoleInfo
 * A pointer to console settings information, containing in particular
 * (in input) the face name and characteristics of the font to create
 * with the current console code page.
 * In output, the font information gets updated.
 * Note that a default fallback font is always being used in case neither
 * the specified font nor any substitute font could be found and created
 * for the specified code page.
 *
 * @return
 * A GDI handle to the created font, or @b NULL in case of failure.
 *
 * @see CreateConsoleFontEx(), CreateConsoleFont2()
 **/
HFONT
CreateConsoleFont(
    _Inout_ PCONSOLE_STATE_INFO ConsoleInfo)
{
    /*
     * Format:
     * Width  = FontSize.X = LOWORD(FontSize);
     * Height = FontSize.Y = HIWORD(FontSize);
     */
    /* NOTE: FontSize is always in cell height/width units (pixels) */
    return CreateConsoleFont2((LONG)(ULONG)ConsoleInfo->FontSize.Y,
                              (LONG)(ULONG)ConsoleInfo->FontSize.X,
                              ConsoleInfo);
}

/**
 * @brief
 * Retrieves the cell size for a console font.
 *
 * @param[in,opt]   hDC
 * An optional GDI device context handle.
 *
 * @param[in]   hFont
 * The GDI handle to the font.
 *
 * @param[out]  Height
 * In case of success, receives the cell height size (in pixels).
 *
 * @param[out]  Width
 * In case of success, receives the cell height size (in pixels).
 *
 * @return
 * @b TRUE if success, @b FALSE in case of failure.
 **/
_Success_(return)
BOOL
GetFontCellSize(
    _In_opt_ HDC hDC,
    _In_  HFONT hFont,
    _Out_ PUINT Height,
    _Out_ PUINT Width)
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
        DBGFNT1("GetFontCellSize: SelectObject failed\n");
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
        DBGFNT1("GetFontCellSize: GetTextMetrics failed\n");
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

/**
 * @brief
 * Validates whether a given font can be supported in the console,
 * under the specified code page.
 *
 * @param[in]   lplf
 * @param[in]   lpntm
 * @param[in]   FontType
 * The GDI font characteristics of the font to validate.
 *
 * @param[in]   CodePage
 * The code page the font has to support.
 *
 * @return
 * @b TRUE if the font is valid and supported in the console,
 * @b FALSE if not.
 *
 * @remark
 * Equivalent of the font validation tests in FontEnumForV2Console()
 * (or the more restrictive ones in FontEnum())
 * https://github.com/microsoft/terminal/blob/main/src/propsheet/misc.cpp#L465
 * https://github.com/microsoft/terminal/blob/main/src/propsheet/misc.cpp#L607
 *
 * @see IsValidConsoleFont()
 **/
BOOL
IsValidConsoleFont2(
    _In_ PLOGFONTW lplf,
    _In_ PNEWTEXTMETRICW lpntm,
    _In_ DWORD FontType,
    _In_ UINT  CodePage)
{
    LPCWSTR FaceName = lplf->lfFaceName;

    /*
     * According to: https://web.archive.org/web/20140901124501/http://support.microsoft.com/kb/247815
     * "Necessary criteria for fonts to be available in a command window",
     * the criteria for console-eligible fonts are as follows:
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
     * See also Raymond Chen's blog: https://devblogs.microsoft.com/oldnewthing/?p=26843
     * and MIT-licensed Microsoft Terminal source code: https://github.com/microsoft/terminal/blob/main/src/propsheet/misc.cpp
     * for other details.
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
     *   than OEM_CHARSET. When an Asian codepage is active however, we require
     *   that this non-TrueType font has an Asian character set.
     */

    /* Reject variable-width fonts ... */
    if ( ( ((lplf->lfPitchAndFamily & 0x03) != FIXED_PITCH)
#if 0 /* Reject italic and TrueType fonts with negative A or C space ... */
           || (lplf->lfItalic)
           || !(lpntm->ntmFlags & NTM_NONNEGATIVE_AC)
#endif
         ) &&
        /* ... if they are not in the list of additional TrueType fonts to include */
         !IsAdditionalTTFont(FaceName) )
    {
        DBGFNT("Font '%S' rejected because it%s (lfPitchAndFamily = %d)\n",
               FaceName,
               !(lplf->lfPitchAndFamily & FIXED_PITCH) ? "'s not FIXED_PITCH"
                   : (!(lpntm->ntmFlags & NTM_NONNEGATIVE_AC) ? " has negative A or C space"
                                                              : " is broken"),
               lplf->lfPitchAndFamily);
        return FALSE;
    }

    /* Reject TrueType fonts that are not FF_MODERN */
    if ((FontType == TRUETYPE_FONTTYPE) && ((lplf->lfPitchAndFamily & 0xF0) != FF_MODERN))
    {
        DBGFNT("TrueType font '%S' rejected because it's not FF_MODERN (lfPitchAndFamily = %d)\n",
               FaceName, lplf->lfPitchAndFamily);
        return FALSE;
    }

    /* Reject vertical fonts (tategaki) */
    if (FaceName[0] == L'@')
    {
        DBGFNT("Font '%S' rejected because it's vertical\n", FaceName);
        return FALSE;
    }

    /* Is the current code page Chinese, Japanese or Korean? */
    if (IsCJKCodePage(CodePage))
    {
        /* It's CJK */

        if (FontType == TRUETYPE_FONTTYPE)
        {
            /*
             * Here we are inclusive and check for any CJK character set,
             * instead of looking just at the current one via CodePageToCharSet().
             */
            if (!IsCJKCharSet(lplf->lfCharSet))
            {
                DBGFNT("TrueType font '%S' rejected because it's not Asian charset (lfCharSet = %d)\n",
                       FaceName, lplf->lfCharSet);
                return FALSE;
            }

            /*
             * If this is a cached TrueType font that is used only for certain
             * code pages, verify that the charset it claims is the correct one.
             *
             * Since there may be multiple entries for a cached TrueType font,
             * a general one (code page == 0) and one or more for explicit
             * code pages, we need to perform two search queries instead of
             * just one and retrieving the code page for this entry.
             */
            if (IsAdditionalTTFont(FaceName) && !IsAdditionalTTFontCP(FaceName, 0) &&
                !IsCJKCharSet(lplf->lfCharSet))
            {
                DBGFNT("Cached TrueType font '%S' rejected because it claims a code page that is not Asian charset (lfCharSet = %d)\n",
                       FaceName, lplf->lfCharSet);
                return FALSE;
            }
        }
        else
        {
            /* Reject non-TrueType fonts that do not have an Asian character set */
            if (!IsCJKCharSet(lplf->lfCharSet) && (lplf->lfCharSet != OEM_CHARSET))
            {
                DBGFNT("Non-TrueType font '%S' rejected because it's not Asian charset or OEM_CHARSET (lfCharSet = %d)\n",
                       FaceName, lplf->lfCharSet);
                return FALSE;
            }

            /* Reject non-TrueType fonts that are not Terminal */
            if (wcscmp(FaceName, TERMINAL_FACENAME) != 0)
            {
                DBGFNT("Non-TrueType font '%S' rejected because it's not 'Terminal'\n", FaceName);
                return FALSE;
            }
        }
    }
    else
    {
        /* Not CJK */

        /* Reject non-TrueType fonts that are not OEM or similar */
        if ((FontType != TRUETYPE_FONTTYPE) &&
            (lplf->lfCharSet != ANSI_CHARSET) &&
            (lplf->lfCharSet != DEFAULT_CHARSET) &&
            (lplf->lfCharSet != OEM_CHARSET))
        {
            DBGFNT("Non-TrueType font '%S' rejected because it's not ANSI_CHARSET or DEFAULT_CHARSET or OEM_CHARSET (lfCharSet = %d)\n",
                   FaceName, lplf->lfCharSet);
            return FALSE;
        }
    }

    /* All good */
    return TRUE;
}

typedef struct _IS_VALID_CONSOLE_FONT_PARAM
{
    BOOL IsValidFont;
    UINT CodePage;
} IS_VALID_CONSOLE_FONT_PARAM, *PIS_VALID_CONSOLE_FONT_PARAM;

/**
 * @brief   EnumFontFamiliesEx() callback helper for IsValidConsoleFont().
 **/
static BOOL CALLBACK
IsValidConsoleFontProc(
    _In_ PLOGFONTW lplf,
    _In_ PNEWTEXTMETRICW lpntm,
    _In_ DWORD  FontType,
    _In_ LPARAM lParam)
{
    PIS_VALID_CONSOLE_FONT_PARAM Param = (PIS_VALID_CONSOLE_FONT_PARAM)lParam;
    Param->IsValidFont = IsValidConsoleFont2(lplf, lpntm, FontType, Param->CodePage);

    /* Stop the enumeration now */
    return FALSE;
}

/**
 * @brief
 * Validates whether a given font can be supported in the console,
 * under the specified code page.
 *
 * @param[in]   FaceName
 * The face name of the font to validate.
 *
 * @param[in]   CodePage
 * The code page the font has to support.
 *
 * @return
 * @b TRUE if the font is valid and supported in the console,
 * @b FALSE if not.
 *
 * @see IsValidConsoleFont2()
 **/
BOOL
IsValidConsoleFont(
    // _In_reads_or_z_(LF_FACESIZE)
    _In_ PCWSTR FaceName,
    _In_ UINT CodePage)
{
    IS_VALID_CONSOLE_FONT_PARAM Param;
    HDC hDC;
    LOGFONTW lf;

    Param.IsValidFont = FALSE;
    Param.CodePage = CodePage;

    RtlZeroMemory(&lf, sizeof(lf));
    lf.lfCharSet = CodePageToCharSet(CodePage);
    // lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    StringCchCopyW(lf.lfFaceName, ARRAYSIZE(lf.lfFaceName), FaceName);

    hDC = GetDC(NULL);
    EnumFontFamiliesExW(hDC, &lf, (FONTENUMPROCW)IsValidConsoleFontProc, (LPARAM)&Param, 0);
    ReleaseDC(NULL, hDC);

    return Param.IsValidFont;
}


/**
 * @brief
 * Initializes the console TrueType font cache.
 *
 * @remark
 * To install additional TrueType fonts to be available for the console,
 * add entries of type REG_SZ named "0", "00" etc... in:
 * HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\Console\TrueTypeFont
 * The names of the fonts listed there should match those in:
 * HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\Fonts
 *
 * @return None.
 **/
VOID
InitTTFontCache(VOID)
{
    LRESULT lResult;
    HKEY hKey;
    DWORD dwIndex, dwType;
    WCHAR szValueName[MAX_PATH];
    DWORD cchValueName;
    WCHAR szValue[LF_FACESIZE] = L"";
    DWORD cbValue;
    UINT CodePage;
    PTT_FONT_ENTRY FontEntry;
    PWCHAR pszNext;

    if (TTFontCache.Next != NULL)
        return;
    // TTFontCache.Next = NULL;

    /* Open the Console\TrueTypeFont key */
    // "\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Console\\TrueTypeFont"
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Console\\TrueTypeFont",
                      0,
                      KEY_QUERY_VALUE,
                      &hKey) != ERROR_SUCCESS)
    {
        return;
    }

    /* Enumerate all the available TrueType console fonts */
    for (dwIndex = 0, cchValueName = ARRAYSIZE(szValueName),
                      cbValue = sizeof(szValue);
         (lResult = RegEnumValueW(hKey, dwIndex,
                                  szValueName, &cchValueName,
                                  NULL, &dwType,
                                  (PBYTE)szValue, &cbValue)) != ERROR_NO_MORE_ITEMS;
         ++dwIndex, cchValueName = ARRAYSIZE(szValueName),
                    cbValue = sizeof(szValue))
    {
        /* Ignore if we failed for another reason, e.g. because
         * the value name is too long (and thus, invalid). */
        if (lResult != ERROR_SUCCESS)
            continue;

        /* Validate the value name (exclude the unnamed value) */
        if (!cchValueName || (*szValueName == UNICODE_NULL))
            continue;
        /* Too large value names have already been handled with ERROR_MORE_DATA */
        ASSERT((cchValueName < ARRAYSIZE(szValueName)) &&
               (szValueName[cchValueName] == UNICODE_NULL));

        /* Only (multi-)string values are supported */
        if ((dwType != REG_SZ) && (dwType != REG_MULTI_SZ))
            continue;

        /* The value name is a code page (in decimal), validate it */
        CodePage = wcstoul(szValueName, &pszNext, 10);
        if (*pszNext)
            continue; // Non-numerical garbage followed...
        // IsValidCodePage(CodePage);

        FontEntry = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*FontEntry));
        if (!FontEntry)
        {
            DBGFNT1("InitTTFontCache: Failed to allocate memory, continuing...\n");
            continue;
        }

        FontEntry->CodePage = CodePage;

        pszNext = szValue;

        /* Check whether bold is disabled for this font */
        if (*pszNext == BOLD_MARK)
        {
            FontEntry->DisableBold = TRUE;
            ++pszNext;
        }
        else
        {
            FontEntry->DisableBold = FALSE;
        }

        /* Copy the font name */
        StringCchCopyNW(FontEntry->FaceName, ARRAYSIZE(FontEntry->FaceName),
                        pszNext, wcslen(pszNext));

        if (dwType == REG_MULTI_SZ)
        {
            /* There may be an alternate face name as the second string */
            pszNext += wcslen(pszNext) + 1;

            /* Check whether bold is disabled for this font */
            if (*pszNext == BOLD_MARK)
            {
                FontEntry->DisableBold = TRUE;
                ++pszNext;
            }
            // else, keep the original setting.

            /* Copy the alternate font name */
            StringCchCopyNW(FontEntry->FaceNameAlt, ARRAYSIZE(FontEntry->FaceNameAlt),
                            pszNext, wcslen(pszNext));
        }

        PushEntryList(&TTFontCache, &FontEntry->Entry);
    }

    /* Close the key and quit */
    RegCloseKey(hKey);
}

/**
 * @brief
 * Clears the console TrueType font cache.
 *
 * @return None.
 **/
VOID
ClearTTFontCache(VOID)
{
    PSINGLE_LIST_ENTRY Entry;
    PTT_FONT_ENTRY FontEntry;

    while (TTFontCache.Next != NULL)
    {
        Entry = PopEntryList(&TTFontCache);
        FontEntry = CONTAINING_RECORD(Entry, TT_FONT_ENTRY, Entry);
        RtlFreeHeap(RtlGetProcessHeap(), 0, FontEntry);
    }
    TTFontCache.Next = NULL;
}

/**
 * @brief
 * Refreshes the console TrueType font cache,
 * by clearing and re-initializing it.
 *
 * @return None.
 **/
VOID
RefreshTTFontCache(VOID)
{
    ClearTTFontCache();
    InitTTFontCache();
}

/**
 * @brief
 * Searches for a font in the console TrueType font cache,
 * with the specified code page.
 *
 * @param[in,opt]   FaceName
 * An optional pointer to a maximally @b LF_FACESIZE-sized buffer.
 * The buffer contains the face name of the font to search for.
 *
 * - If FaceName != NULL, search for the named font that should
 *   match the provided code page (when CodePage != INVALID_CP).
 *
 * - If FaceName == NULL, search for a font with the provided
 *   code page. In this case, CodePage cannot be == INVALID_CP,
 *   otherwise the search fails.
 *
 * @param[in]   CodePage
 * The code page the font has to support, or @b INVALID_CP when
 * searching a font by face name only.
 *
 * @return
 * A pointer to the cache entry for the font, or @b NULL if not found.
 **/
PTT_FONT_ENTRY
FindCachedTTFont(
    _In_reads_or_z_opt_(LF_FACESIZE)
         PCWSTR FaceName,
    _In_ UINT CodePage)
{
    PSINGLE_LIST_ENTRY Entry;
    PTT_FONT_ENTRY FontEntry;

    if (FaceName)
    {
        /* Search for the named font */
        for (Entry = TTFontCache.Next;
             Entry != NULL;
             Entry = Entry->Next)
        {
            FontEntry = CONTAINING_RECORD(Entry, TT_FONT_ENTRY, Entry);

            /* NOTE: The font face names are case-sensitive */
            if ((wcscmp(FontEntry->FaceName   , FaceName) == 0) ||
                (wcscmp(FontEntry->FaceNameAlt, FaceName) == 0))
            {
                /* Return the font if we don't search by code page, or when they match */
                if ((CodePage == INVALID_CP) || (CodePage == FontEntry->CodePage))
                {
                    return FontEntry;
                }
            }
        }
    }
    else if (CodePage != INVALID_CP)
    {
        /* Search for a font with the specified code page */
        for (Entry = TTFontCache.Next;
             Entry != NULL;
             Entry = Entry->Next)
        {
            FontEntry = CONTAINING_RECORD(Entry, TT_FONT_ENTRY, Entry);

            /* Return the font if the code pages match */
            if (CodePage == FontEntry->CodePage)
                return FontEntry;
        }
    }

    return NULL;
}

/* EOF */

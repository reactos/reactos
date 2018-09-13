/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    misc.c

Abstract:

        This file implements the NT console server font routines.

Author:

    Therese Stowell (thereses) 22-Jan-1991

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef DEBUG_PRINT
ULONG gDebugFlag;
//ULONG gDebugFlag = _DBGOUTPUT | _DBGCHARS | _DBGFONTS | _DBGFONTS2 ;
#endif

ULONG NumberOfMouseButtons;

PFONT_INFO FontInfo;
ULONG FontInfoLength;
ULONG NumberOfFonts;

WCHAR DefaultFaceName[LF_FACESIZE];
COORD DefaultFontSize;
BYTE  DefaultFontFamily;
ULONG DefaultFontIndex = 0;

typedef struct _FONTENUMDC {
    HDC hDC;
    BOOL bFindFaces;
    SHORT TTPointSize;
    ULONG ulFE;
} FONTENUMDC, *PFONTENUMDC;

/*
 * Custom CP for glyph translations
 */
CPTABLEINFO GlyphCP;
USHORT GlyphTable[256];


#define FONT_BUFFER_SIZE 12

#define FE_ABANDONFONT 1
#define FE_FONTOK      2

/*
 * Initial default fonts and face names
 */
PFACENODE gpFaceNames;


NTSTATUS
GetMouseButtons(
    PULONG NumButtons
    )
{
    *NumButtons = NumberOfMouseButtons;
    return STATUS_SUCCESS;
}

VOID
InitializeMouseButtons( VOID )
{
    NumberOfMouseButtons = GetSystemMetrics(SM_CMOUSEBUTTONS);
}

PFACENODE AddFaceNode(PFACENODE *ppStart, LPWSTR pwsz) {
    PFACENODE pNew;
    PFACENODE *ppTmp;
    int cb;

    /*
     * Is it already here?
     */
    for (ppTmp = ppStart; *ppTmp; ppTmp = &((*ppTmp)->pNext)) {
        if (wcscmp(((*ppTmp)->awch), pwsz) == 0) {
            // already there !
            return *ppTmp;
        }
    }

    cb = (wcslen(pwsz) + 1) * sizeof(WCHAR);
    pNew = (PFACENODE)ConsoleHeapAlloc(MAKE_TAG( FONT_TAG ),sizeof(FACENODE) + cb);
    if (pNew == NULL) {
        return NULL;
    }

    pNew->pNext = NULL;
    pNew->dwFlag = 0;
    wcscpy(pNew->awch, pwsz);
    *ppTmp = pNew;
    return pNew;
}

VOID
InitializeFonts( VOID )
{
    WCHAR FontName[CONSOLE_MAX_FONT_NAME_LENGTH];
    int i;
    static CONST LPWSTR FontList[] = {L"woafont",
                                      L"ega80woa.fon",
                                      L"ega40woa.fon",
                                      L"cga80woa.fon",
                                      L"cga40woa.fon"};

    //
    // Read software.ini to get the values for "woafont",
    // "ega80woa.fon", "ega40woa.fon", "cga80woa.fon", and
    // "cga40woa.fon", respectively, to pass to AddFontResource.
    //
    // If any of the entries are empty or non-existent,
    // GetPrivateProfileString will return a NULL (empty) string.
    // If such is the case, the call to AddPermanentFontResource will
    // simply fail.
    //

    OpenProfileUserMapping();

    for (i = 0; i < NELEM(FontList); i++) {
        GetPrivateProfileString(L"386enh", FontList[i], L"",
                FontName, NELEM(FontName), L"system.ini");
        GdiAddFontResourceW(FontName, AFRW_ADD_LOCAL_FONT,NULL);
    }

    CloseProfileUserMapping();
}

/*
 * Returns bit combination
 *  FE_ABANDONFONT  - do not continue enumerating this font
 *  FE_FONTOK       - font was created and added to cache or already there
 */

/*


*/
int CALLBACK
FontEnum(
    LPENUMLOGFONTW lpLogFont,
    LPNEWTEXTMETRICW lpTextMetric,
    int nFontType,
    LPARAM lParam
    )

/*++

    Is called exactly once by GDI for each font in the system.  This
    routine is used to store the FONT_INFO structure.

--*/

{
    PFONTENUMDC pfed = (PFONTENUMDC)lParam;
    HDC hDC = pfed->hDC;
    BOOL bFindFaces = pfed->bFindFaces;
    HFONT hFont;
    TEXTMETRICW tmi;
    LONG nFont;
    LONG nFontNew;
    COORD SizeToShow;
    COORD SizeActual;
    COORD SizeWant;
    BYTE tmFamily;
    SIZE Size;
    LPWSTR pwszFace = lpLogFont->elfLogFont.lfFaceName;
    PFACENODE pFN;

    DBGFONTS(("  FontEnum \"%ls\" (%d,%d) weight 0x%lx(%d) -- %s\n",
            pwszFace,
            lpLogFont->elfLogFont.lfWidth, lpLogFont->elfLogFont.lfHeight,
            lpLogFont->elfLogFont.lfWeight, lpLogFont->elfLogFont.lfWeight,
            bFindFaces ? "Finding Faces" : "Creating Fonts"));

    //
    // reject variable width and italic fonts, also tt fonts with neg ac
    //

    if
    (
      !(lpLogFont->elfLogFont.lfPitchAndFamily & FIXED_PITCH) ||
      (lpLogFont->elfLogFont.lfItalic)                        ||
      !(lpTextMetric->ntmFlags & NTM_NONNEGATIVE_AC)
    )
    {
        if (!IsAvailableTTFont(pwszFace))
        {
            DBGFONTS(("    REJECT  face (variable pitch, italic, or neg a&c)\n"));
            return bFindFaces ? TRUE : FALSE;  // unsuitable font
        }
    }

    if (nFontType == TRUETYPE_FONTTYPE) {
        lpLogFont->elfLogFont.lfHeight = pfed->TTPointSize;
        lpLogFont->elfLogFont.lfWidth  = 0;
        lpLogFont->elfLogFont.lfWeight = FW_NORMAL;
    }

    /*
     * reject TT fonts for whoom family is not modern, that is do not use
     * FF_DONTCARE    // may be surprised unpleasantly
     * FF_DECORATIVE  // likely to be symbol fonts
     * FF_SCRIPT      // cursive, inappropriate for console
     * FF_SWISS OR FF_ROMAN // variable pitch
     */

    if ((nFontType == TRUETYPE_FONTTYPE) &&
            ((lpLogFont->elfLogFont.lfPitchAndFamily & 0xf0) != FF_MODERN)) {
        DBGFONTS(("    REJECT  face (TT but not FF_MODERN)\n"));
        return bFindFaces ? TRUE : FALSE;  // unsuitable font
    }

    /*
     * reject non-TT fonts that aren't OEM
     */
    if ((nFontType != TRUETYPE_FONTTYPE) &&
#if defined(FE_SB)
            (!CONSOLE_IS_DBCS_ENABLED() ||
            !IS_ANY_DBCS_CHARSET(lpLogFont->elfLogFont.lfCharSet)) &&
#endif
            (lpLogFont->elfLogFont.lfCharSet != OEM_CHARSET)) {
        DBGFONTS(("    REJECT  face (not TT nor OEM)\n"));
        return bFindFaces ? TRUE : FALSE;  // unsuitable font
    }

    /*
     * reject non-TT fonts that are virtical font
     */
    if ((nFontType != TRUETYPE_FONTTYPE) &&
            (pwszFace[0] == L'@')) {
        DBGFONTS(("    REJECT  face (not TT and TATEGAKI)\n"));
        return bFindFaces ? TRUE : FALSE;  // unsuitable font
    }

    /*
     * reject non-TT fonts that aren't Terminal
     */
    if (CONSOLE_IS_DBCS_ENABLED() &&
        (nFontType != TRUETYPE_FONTTYPE) &&
            (wcscmp(pwszFace, L"Terminal") != 0)) {
        DBGFONTS(("    REJECT  face (not TT nor Terminal)\n"));
        return bFindFaces ? TRUE : FALSE;  // unsuitable font
    }

    /*
     * reject Far East TT fonts that aren't Far East charset.
     */
    if (IsAvailableTTFont(pwszFace) &&
        !IS_ANY_DBCS_CHARSET(lpLogFont->elfLogFont.lfCharSet) &&
        !IsAvailableTTFontCP(pwszFace,0)
       ) {
        DBGFONTS(("    REJECT  face (Far East TT and not Far East charset)\n"));
        return TRUE;    // should be enumerate next charset.
    }

    /*
     * Add or find the facename
     */
    pFN = AddFaceNode(&gpFaceNames, pwszFace);
    if (pFN == NULL) {
        return FALSE;
    }

    if (bFindFaces) {
        if (nFontType == TRUETYPE_FONTTYPE) {
            DBGFONTS(("NEW TT FACE %ls\n", pwszFace));
            pFN->dwFlag |= EF_TTFONT;
        } else if (nFontType == RASTER_FONTTYPE) {
            DBGFONTS(("NEW OEM FACE %ls\n",pwszFace));
            pFN->dwFlag |= EF_OEMFONT;
        }
        return 0;
    }


    if (IS_BOLD(lpLogFont->elfLogFont.lfWeight)) {
        DBGFONTS2(("    A bold font (weight %d)\n", lpLogFont->elfLogFont.lfWeight));
        // return 0;
    }

    /* get font info */
    SizeWant.Y = (SHORT)lpLogFont->elfLogFont.lfHeight;
    SizeWant.X = (SHORT)lpLogFont->elfLogFont.lfWidth;
CreateBoldFont:
    lpLogFont->elfLogFont.lfQuality = NONANTIALIASED_QUALITY;
    hFont = CreateFontIndirectW(&lpLogFont->elfLogFont);
    if (!hFont) {
        DBGFONTS(("    REJECT  font (can't create)\n"));
        RIPMSG0(RIP_WARNING, "FontEnum: CreateFontIndirectW returned NULL hFont.");
        return 0;  // same font in other sizes may still be suitable
    }

    DBGFONTS2(("    hFont = %lx\n", hFont));

    //
    // for reasons unbeknownst to me, removing this code causes GDI
    // to yack, claiming that the font is owned by another process.
    //

    SelectObject(hDC,hFont);
    if (!GetTextMetricsW(hDC, &tmi)) {
        tmi = *((LPTEXTMETRICW)lpTextMetric);
    }

    if (GetTextExtentPoint32W(hDC, L"0", 1, &Size)) {
        SizeActual.X = (SHORT)Size.cx;
    } else {
        SizeActual.X = (SHORT)(tmi.tmMaxCharWidth);
    }
    SizeActual.Y = (SHORT)(tmi.tmHeight + tmi.tmExternalLeading);
    DBGFONTS2(("    actual size %d,%d\n", SizeActual.X, SizeActual.Y));
    tmFamily = tmi.tmPitchAndFamily;
    if (TM_IS_TT_FONT(tmFamily) && (SizeWant.Y >= 0)) {
        SizeToShow = SizeWant;
        if (SizeWant.X == 0) {
            // Asking for zero width height gets a default aspect-ratio width
            // It's better to show that width rather than 0.
            SizeToShow.X = SizeActual.X;
        }
    } else {
        SizeToShow = SizeActual;
    }
    DBGFONTS2(("    SizeToShow = (%d,%d), SizeActual = (%d,%d)\n",
            SizeToShow.X, SizeToShow.Y, SizeActual.X, SizeActual.Y));

    // there's a GDI bug - this assert fails occasionally
    //ASSERT (tmi.tmw.tmMaxCharWidth == lpTextMetric->tmMaxCharWidth);

    /*
     * NOW, determine whether this font entry has already been cached
     * LATER : it may be possible to do this before creating the font, if
     * we can trust the dimensions & other info from lpTextMetric.
     * Sort by size:
     *  1) By pixelheight (negative Y values)
     *  2) By height (as shown)
     *  3) By width (as shown)
     */
    for (nFont = 0; nFont < (LONG)NumberOfFonts; ++nFont) {
        COORD SizeShown;

        if (FontInfo[nFont].hFont == NULL) {
            DBGFONTS(("!   Font %x has a NULL hFont\n", nFont));
            continue;
        }


        if (FontInfo[nFont].SizeWant.X > 0) {
            SizeShown.X = FontInfo[nFont].SizeWant.X;
        } else {
            SizeShown.X = FontInfo[nFont].Size.X;
        }

        if (FontInfo[nFont].SizeWant.Y > 0) {
            // This is a font specified by cell height.
            SizeShown.Y = FontInfo[nFont].SizeWant.Y;
        } else {
            SizeShown.Y = FontInfo[nFont].Size.Y;
            if (FontInfo[nFont].SizeWant.Y < 0) {
                // This is a TT font specified by character height.
                if (SizeWant.Y < 0 && SizeWant.Y > FontInfo[nFont].SizeWant.Y) {
                    // Requested pixelheight is smaller than this one.
                    DBGFONTS(("INSERT %d pt at %x, before %d pt\n",
                            -SizeWant.Y, nFont, -FontInfo[nFont].SizeWant.Y));
                    nFontNew = nFont;
                    goto InsertNewFont;
                }
            }
        }

        // DBGFONTS(("    SizeShown(%x) = (%d,%d)\n",nFont,SizeShown.X,SizeShown.Y));

        if (SIZE_EQUAL(SizeShown, SizeToShow) &&
                FontInfo[nFont].Family == tmFamily &&
                FontInfo[nFont].Weight == tmi.tmWeight &&
                wcscmp(FontInfo[nFont].FaceName, pwszFace) == 0) {
            /*
             * Already have this font
             */
            DBGFONTS2(("    Already have the font\n"));
            DeleteObject(hFont);
            pfed->ulFE |= FE_FONTOK;
            return TRUE;
        }


        if ((SizeToShow.Y < SizeShown.Y) ||
                (SizeToShow.Y == SizeShown.Y && SizeToShow.X < SizeShown.X)) {
            /*
             * This new font is smaller than nFont
             */
            DBGFONTS(("INSERT at %x, SizeToShow = (%d,%d)\n", nFont,
                    SizeToShow.X,SizeToShow.Y));
            nFontNew = nFont;
            goto InsertNewFont;
        }
    }

    /*
     * The font we are adding should be appended to the list,
     * since it is bigger (or equal) to the last one.
     */
    nFontNew = (LONG)NumberOfFonts;

InsertNewFont: // at nFontNew

//  ASSERT ((lpTextMetric->tmPitchAndFamily & 1) == 0);
    /* If we have to grow our font table, do it */

    if (NumberOfFonts == FontInfoLength) {
        PFONT_INFO Temp;

        FontInfoLength += FONT_INCREMENT;
        Temp = (PFONT_INFO)ConsoleHeapReAlloc(MAKE_TAG( FONT_TAG ),FontInfo,sizeof(FONT_INFO) * FontInfoLength);
        if (Temp == NULL) {
            RIPMSG0(RIP_WARNING, "FontEnum: failed to allocate PFONT_INFO");
            FontInfoLength -= FONT_INCREMENT;
            return FALSE;
        }
        FontInfo = Temp;
    }

    if (nFontNew < (LONG)NumberOfFonts) {
        RtlMoveMemory(&FontInfo[nFontNew+1],
                &FontInfo[nFontNew],
                sizeof(FONT_INFO)*(NumberOfFonts - nFontNew));
        //
        // Fix up DefaultFontIndex if nFontNew less than DefaultFontIndex.
        //
        if (nFontNew < (LONG)DefaultFontIndex &&
            DefaultFontIndex+1 < NumberOfFonts) {
            DefaultFontIndex++;
        }
    }

    /*
     * Store the font info
     */
    FontInfo[nFontNew].hFont = hFont;
    FontInfo[nFontNew].Family = tmFamily;
    FontInfo[nFontNew].Size = SizeActual;
    if (TM_IS_TT_FONT(tmFamily)) {
        FontInfo[nFontNew].SizeWant = SizeWant;
    } else {
        FontInfo[nFontNew].SizeWant.X = 0;
        FontInfo[nFontNew].SizeWant.Y = 0;
    }
    FontInfo[nFontNew].Weight = tmi.tmWeight;
    FontInfo[nFont].FaceName = pFN->awch;
#if defined(FE_SB)
    FontInfo[nFontNew].tmCharSet = tmi.tmCharSet;
#endif

    ++NumberOfFonts;

    if (nFontType == TRUETYPE_FONTTYPE && !IS_BOLD(FontInfo[nFontNew].Weight)) {
          lpLogFont->elfLogFont.lfWeight = FW_BOLD;
          goto CreateBoldFont;
    }

    pfed->ulFE |= FE_FONTOK;  // and continue enumeration
    return TRUE;
}

BOOL
DoFontEnum(
    HDC hDC,
    LPWSTR pwszFace,
    SHORT TTPointSize)
{
    ULONG ulFE = 0;
    BOOL bDeleteDC = FALSE;
    BOOL bFindFaces = (pwszFace == NULL);
    FONTENUMDC fed;
    LOGFONTW LogFont;

    DBGFONTS(("DoFontEnum \"%ls\"\n", pwszFace));
    if (hDC == NULL) {
        hDC = CreateDCW(L"DISPLAY",NULL,NULL,NULL);
        bDeleteDC = TRUE;
    }

    fed.hDC = hDC;
    fed.bFindFaces = bFindFaces;
    fed.ulFE = 0;
    fed.TTPointSize = TTPointSize;
    RtlZeroMemory(&LogFont, sizeof(LOGFONT));
    LogFont.lfCharSet = DEFAULT_CHARSET;
    if (pwszFace)
        wcscpy(LogFont.lfFaceName, pwszFace);
    /*
     * EnumFontFamiliesEx function enumerates one font in every face in every character set.
     */
    EnumFontFamiliesExW(hDC, &LogFont, (FONTENUMPROC)FontEnum, (LPARAM)&fed, 0);
    if (bDeleteDC) {
        DeleteDC(hDC);
    }
    return (fed.ulFE & FE_FONTOK) != 0;
}


NTSTATUS
EnumerateFonts(
    DWORD Flags)
{
    TEXTMETRIC tmi;
    HDC hDC;
    PFACENODE pFN;
    ULONG ulOldEnumFilter;
    DWORD FontIndex;
    DWORD dwFontType = 0;

    DBGFONTS(("EnumerateFonts %lx\n", Flags));

    dwFontType = (EF_TTFONT|EF_OEMFONT|EF_DEFFACE) & Flags;

    if (FontInfo == NULL) {
        //
        // allocate memory for the font array
        //
        NumberOfFonts = 0;

        FontInfo = (PFONT_INFO)ConsoleHeapAlloc(MAKE_TAG( FONT_TAG ),sizeof(FONT_INFO) * INITIAL_FONTS);
        if (FontInfo == NULL)
            return STATUS_NO_MEMORY;
        FontInfoLength = INITIAL_FONTS;
    }

    hDC = CreateDCW(L"DISPLAY",NULL,NULL,NULL);

    // Before enumeration, turn off font enumeration filters.
    ulOldEnumFilter = SetFontEnumeration(0);
    // restore all the other flags
    SetFontEnumeration(ulOldEnumFilter & ~FE_FILTER_TRUETYPE);

    if (Flags & EF_DEFFACE) {
        SelectObject(hDC,GetStockObject(OEM_FIXED_FONT));

        if (GetTextMetricsW(hDC, &tmi)) {
            DefaultFontSize.X = (SHORT)(tmi.tmMaxCharWidth);
            DefaultFontSize.Y = (SHORT)(tmi.tmHeight+tmi.tmExternalLeading);
            DefaultFontFamily = tmi.tmPitchAndFamily;
#if defined(FE_SB)
            if (IS_ANY_DBCS_CHARSET(tmi.tmCharSet))
                DefaultFontSize.X /= 2;
#endif
        }
        GetTextFaceW(hDC, LF_FACESIZE, DefaultFaceName);
#if defined(FE_SB)
        DBGFONTS(("Default (OEM) Font %ls (%d,%d) CharSet 0x%02X\n", DefaultFaceName,
                DefaultFontSize.X, DefaultFontSize.Y,
                tmi.tmCharSet));
#else
        DBGFONTS(("Default (OEM) Font %ls (%d,%d)\n", DefaultFaceName,
                DefaultFontSize.X, DefaultFontSize.Y));
#endif

        // Make sure we are going to enumerate the OEM face.
        pFN = AddFaceNode(&gpFaceNames, DefaultFaceName);
        pFN->dwFlag |= EF_DEFFACE | EF_OEMFONT;
    }

    // Use DoFontEnum to get all fonts from the system.  Our FontEnum
    // proc puts just the ones we want into an array
    //
    for (pFN = gpFaceNames; pFN; pFN = pFN->pNext) {
        DBGFONTS(("\"%ls\" is %s%s%s%s%s%s\n", pFN->awch,
            pFN->dwFlag & EF_NEW        ? "NEW "        : " ",
            pFN->dwFlag & EF_OLD        ? "OLD "        : " ",
            pFN->dwFlag & EF_ENUMERATED ? "ENUMERATED " : " ",
            pFN->dwFlag & EF_OEMFONT    ? "OEMFONT "    : " ",
            pFN->dwFlag & EF_TTFONT     ? "TTFONT "     : " ",
            pFN->dwFlag & EF_DEFFACE    ? "DEFFACE "    : " "));

        if ((pFN->dwFlag & dwFontType) == 0) {
            // not the kind of face we want
            continue;
        }
        if (pFN->dwFlag & EF_ENUMERATED) {
            // we already enumerated this face
            continue;
        }

        DoFontEnum(hDC, pFN->awch, DefaultFontSize.Y);
        pFN->dwFlag |= EF_ENUMERATED;
    }


    // After enumerating fonts, restore the font enumeration filter.
    SetFontEnumeration(ulOldEnumFilter);

    DeleteDC(hDC);

    // Make sure the default font is set correctly
    if (NumberOfFonts > 0 && DefaultFontSize.X == 0 && DefaultFontSize.Y == 0) {
        DefaultFontSize.X = FontInfo[0].Size.X;
        DefaultFontSize.Y = FontInfo[0].Size.Y;
        DefaultFontFamily = FontInfo[0].Family;
    }

    for (FontIndex = 0; FontIndex < NumberOfFonts; FontIndex++) {
        if (FontInfo[FontIndex].Size.X == DefaultFontSize.X &&
            FontInfo[FontIndex].Size.Y == DefaultFontSize.Y &&
            FontInfo[FontIndex].Family == DefaultFontFamily) {
#if defined(FE_SB)
            if (CONSOLE_IS_DBCS_ENABLED() &&
                !IS_ANY_DBCS_CHARSET(FontInfo[FontIndex].tmCharSet))
            {
                continue ;
            }
#endif
            break;
        }
    }
    ASSERT(FontIndex < NumberOfFonts);
    if (FontIndex < NumberOfFonts) {
        DefaultFontIndex = FontIndex;
    } else {
        DefaultFontIndex = 0;
    }
    DBGFONTS(("EnumerateFonts : DefaultFontIndex = %ld\n", DefaultFontIndex));

    return STATUS_SUCCESS;
}


/*
 * Get the font index for a new font
 * If necessary, attempt to create the font.
 * Always return a valid FontIndex (even if not correct)
 * Family:   Find/Create a font with of this Family
 *           0    - don't care
 * pwszFace: Find/Create a font with this face name.
 *           NULL or L""  - use DefaultFaceName
 * Size:     Must match SizeWant or actual Size.
 */
int
FindCreateFont(
    DWORD Family,
    LPWSTR pwszFace,
    COORD Size,
    LONG Weight,
    UINT CodePage)
{
#define NOT_CREATED_NOR_FOUND -1
#define CREATED_BUT_NOT_FOUND -2

    int i;
    int FontIndex = NOT_CREATED_NOR_FOUND;
    int BestMatch = NOT_CREATED_NOR_FOUND;
    BOOL bFontOK;
    WCHAR AltFaceName[LF_FACESIZE];
    COORD AltFontSize;
    BYTE  AltFontFamily;
    ULONG AltFontIndex = 0;
    LPWSTR pwszAltFace = NULL;

    BYTE CharSet = CodePageToCharSet(CodePage);

    DBGFONTS(("FindCreateFont Family=%x %ls (%d,%d) %d %d %x\n",
            Family, pwszFace, Size.X, Size.Y, Weight, CodePage, CharSet));

    if (CONSOLE_IS_DBCS_ENABLED() &&
        !IS_ANY_DBCS_CHARSET(CharSet))
    {
        MakeAltRasterFont(CodePage, FontInfo[DefaultFontIndex].Size,
                          &AltFontSize, &AltFontFamily, &AltFontIndex, AltFaceName);

        if (pwszFace == NULL || *pwszFace == L'\0') {
            pwszFace = AltFaceName;
        }
        if (Size.Y == 0) {
            Size.X = AltFontSize.X;
            Size.Y = AltFontSize.Y;
        }
    }
    else {
        if (pwszFace == NULL || *pwszFace == L'\0') {
            pwszFace = DefaultFaceName;
        }
        if (Size.Y == 0) {
            Size.X = DefaultFontSize.X;
            Size.Y = DefaultFontSize.Y;
        }
    }

    if (IsAvailableTTFont(pwszFace)) {
        pwszAltFace = GetAltFaceName(pwszFace);
    }
    else {
        pwszAltFace = pwszFace;
    }

    /*
     * Try to find the exact font
     */
TryFindExactFont:
    for (i=0; i < (int)NumberOfFonts; i++) {
        /*
         * If looking for a particular Family, skip non-matches
         */
        if ((Family != 0) &&
                ((BYTE)Family != FontInfo[i].Family)) {
            continue;
        }

        /*
         * Skip non-matching sizes
         */
        if ((FontInfo[i].SizeWant.Y != Size.Y) &&
             !SIZE_EQUAL(FontInfo[i].Size, Size)) {
            continue;
        }

        /*
         * Skip non-matching weights
         */
        if ((Weight != 0) && (Weight != FontInfo[i].Weight)) {
            continue;
        }
#if defined(FE_SB)
        if (!TM_IS_TT_FONT(FontInfo[i].Family) &&
                FontInfo[i].tmCharSet != CharSet) {
            continue;
        }
#endif

        /*
         * Size (and maybe Family) match.
         *  If we don't care about the name, or if it matches, use this font.
         *  Else if name doesn't match and it is a raster font, consider it.
         */
        if ((pwszFace == NULL) || (pwszFace[0] == L'\0') ||
                wcscmp(FontInfo[i].FaceName, pwszFace) == 0 ||
                wcscmp(FontInfo[i].FaceName, pwszAltFace) == 0
           ) {
            FontIndex = i;
            goto FoundFont;
        } else if (!TM_IS_TT_FONT(FontInfo[i].Family)) {
            BestMatch = i;
        }
    }

    /*
     * Didn't find the exact font, so try to create it
     */
    if (FontIndex == NOT_CREATED_NOR_FOUND) {
        ULONG ulOldEnumFilter;
        ulOldEnumFilter = SetFontEnumeration(0);
        // restore all the other flags
        SetFontEnumeration(ulOldEnumFilter & ~FE_FILTER_TRUETYPE);
        if (Size.Y < 0) {
            Size.Y = -Size.Y;
        }
        bFontOK = DoFontEnum(NULL, pwszFace, Size.Y);
        SetFontEnumeration(ulOldEnumFilter);
        if (bFontOK) {
            DBGFONTS(("FindCreateFont created font!\n"));
            FontIndex = CREATED_BUT_NOT_FOUND;
            goto TryFindExactFont;
        } else {
            DBGFONTS(("FindCreateFont failed to create font!\n"));
        }
    }

    /*
     * Failed to find exact match, but we have a close Raster Font
     * fit - only the name doesn't match.
     */
    if (BestMatch >= 0) {
        FontIndex = BestMatch;
        goto FoundFont;
    }

    /*
     * Failed to find exact match, even after enumeration, so now try
     * to find a font of same family and same size or bigger
     */
    for (i=0; i < (int)NumberOfFonts; i++) {
#if defined(FE_SB)
        if (CONSOLE_IS_DBCS_ENABLED()) {
            if ((Family != 0) &&
                    ((BYTE)Family != FontInfo[i].Family)) {
                continue;
            }

            if (!TM_IS_TT_FONT(FontInfo[i].Family) &&
                    FontInfo[i].tmCharSet != CharSet) {
                continue;
            }
        }
        else {
#endif
        if ((BYTE)Family != FontInfo[i].Family) {
            continue;
        }
#if defined(FE_SB)
        }
#endif

        if (FontInfo[i].Size.Y >= Size.Y &&
                FontInfo[i].Size.X >= Size.X) {
            // Same family, size >= desired.
            FontIndex = i;
            break;
        }
    }

    if (FontIndex < 0) {
        DBGFONTS(("FindCreateFont defaults!\n"));
#if defined(FE_SB)
        if (CONSOLE_IS_DBCS_ENABLED() &&
            !IsAvailableFarEastCodePage(CodePage))
        {
            FontIndex = AltFontIndex;
        }
        else
#endif
        FontIndex = DefaultFontIndex;
    }

FoundFont:
    DBGFONTS(("FindCreateFont returns %x : %ls (%d,%d)\n", FontIndex,
            FontInfo[FontIndex].FaceName,
            FontInfo[FontIndex].Size.X, FontInfo[FontIndex].Size.Y));
    return FontIndex;

#undef NOT_CREATED_NOR_FOUND
#undef CREATED_BUT_NOT_FOUND
}


NTSTATUS
FindTextBufferFontInfo(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN UINT CodePage,
    OUT PTEXT_BUFFER_FONT_INFO TextFontInfo
    )

/*++

Routine Description:

    This routine find a font information which correspond to code page value.

Arguments:

Return Value:

--*/

{
    PTEXT_BUFFER_FONT_INFO CurrentFont;

    CurrentFont = ScreenInfo->BufferInfo.TextInfo.ListOfTextBufferFont;

    while (CurrentFont != NULL) {
        if (CurrentFont->FontCodePage == CodePage) {
            *TextFontInfo = *CurrentFont;
            return STATUS_SUCCESS;
        }
        CurrentFont = CurrentFont->NextTextBufferFont;
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
StoreTextBufferFontInfo(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN ULONG FontIndex,
    IN COORD FontSize,
    IN BYTE  FontFamily,
    IN LONG  FontWeight,
    IN LPWSTR FaceName,
    IN UINT CodePage
    )

/*++

Routine Description:

    This routine store a font information in CurrentTextBufferFont and ListOfTextBufferFont.
    If specified code page does not exist in ListOfTextBufferFont, then create new list.

Arguments:

Return Value:

--*/

{
    PTEXT_BUFFER_FONT_INFO CurrentFont, PrevFont;

    CurrentFont = ScreenInfo->BufferInfo.TextInfo.ListOfTextBufferFont;

    while (CurrentFont != NULL) {
        if (CurrentFont->FontCodePage == CodePage) {
            CurrentFont->FontNumber   = FontIndex;
            CurrentFont->FontSize     = FontSize;
            CurrentFont->Family       = FontFamily;
            CurrentFont->Weight       = FontWeight;
            // CurrentFont->FontCodePage = CodePage; // Redundant
            wcscpy(CurrentFont->FaceName, FaceName);
            break;
        }
        PrevFont    = CurrentFont;
        CurrentFont = CurrentFont->NextTextBufferFont;
    }

    if (CurrentFont == NULL) {
        CurrentFont = ConsoleHeapAlloc(MAKE_TAG( FONT_TAG ), sizeof(TEXT_BUFFER_FONT_INFO));
        if (CurrentFont == NULL) {
            return STATUS_NO_MEMORY;
        }

        CurrentFont->NextTextBufferFont = NULL;
        CurrentFont->FontNumber   = FontIndex;
        CurrentFont->FontSize     = FontSize;
        CurrentFont->Family       = FontFamily;
        CurrentFont->Weight       = FontWeight;
        CurrentFont->FontCodePage = CodePage;
        wcscpy(CurrentFont->FaceName, FaceName);

        if (ScreenInfo->BufferInfo.TextInfo.ListOfTextBufferFont == NULL) {
            ScreenInfo->BufferInfo.TextInfo.ListOfTextBufferFont = CurrentFont;
        }
        else {
            PrevFont->NextTextBufferFont = CurrentFont;
        }
    }

    ScreenInfo->BufferInfo.TextInfo.CurrentTextBufferFont = *CurrentFont;
    ScreenInfo->BufferInfo.TextInfo.CurrentTextBufferFont.NextTextBufferFont = NULL;

    return STATUS_SUCCESS;
}

NTSTATUS
RemoveTextBufferFontInfo(
    IN PSCREEN_INFORMATION ScreenInfo
    )

/*++

Routine Description:

    This routine all remove a font information in ListOfTextBufferFont.

Arguments:

Return Value:

--*/

{
    PTEXT_BUFFER_FONT_INFO CurrentFont;

    CurrentFont = ScreenInfo->BufferInfo.TextInfo.ListOfTextBufferFont;

    while (CurrentFont != NULL) {
        PTEXT_BUFFER_FONT_INFO NextFont;

        NextFont = CurrentFont->NextTextBufferFont;
        ConsoleHeapFree(CurrentFont);

        CurrentFont = NextFont;
    }

    ScreenInfo->BufferInfo.TextInfo.ListOfTextBufferFont = NULL;

    return STATUS_SUCCESS;
}

NTSTATUS
GetNumFonts(
    OUT PULONG NumFonts
    )
{
    *NumFonts = NumberOfFonts;
    return STATUS_SUCCESS;
}


NTSTATUS
GetAvailableFonts(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN BOOLEAN MaximumWindow,
    OUT PVOID Buffer,
    IN OUT PULONG NumFonts
    )
{
    PCONSOLE_FONT_INFO BufPtr;
    ULONG i;
    COORD WindowSize;
    WINDOW_LIMITS WindowLimits;

    //
    // if the buffer is too small to return all the fonts, return
    // the number that will fit.
    //

    *NumFonts = (*NumFonts > NumberOfFonts) ? NumberOfFonts : *NumFonts;

    //
    // convert font size in pixels to font size in rows/columns
    //

    BufPtr = (PCONSOLE_FONT_INFO)Buffer;

    if (MaximumWindow) {
        GetWindowLimits(ScreenInfo, &WindowLimits);
        WindowSize = WindowLimits.MaximumWindowSize;
    }
    else {
        WindowSize.X = (SHORT)CONSOLE_WINDOW_SIZE_X(ScreenInfo);
        WindowSize.Y = (SHORT)CONSOLE_WINDOW_SIZE_Y(ScreenInfo);
    }
    for (i=0;i<*NumFonts;i++,BufPtr++) {
        BufPtr->nFont = i;
        BufPtr->dwFontSize.X = WindowSize.X * SCR_FONTSIZE(ScreenInfo).X / FontInfo[i].Size.X;
        BufPtr->dwFontSize.Y = WindowSize.Y * SCR_FONTSIZE(ScreenInfo).Y / FontInfo[i].Size.Y;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
GetFontSize(
    IN DWORD  FontIndex,
    OUT PCOORD FontSize
    )
{
    if (FontIndex >= NumberOfFonts)
        return STATUS_INVALID_PARAMETER;
    *FontSize = FontInfo[FontIndex].Size;
    return STATUS_SUCCESS;
}

NTSTATUS
GetCurrentFont(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN BOOLEAN MaximumWindow,
    OUT PULONG FontIndex,
    OUT PCOORD FontSize
    )
{
    COORD WindowSize;
    WINDOW_LIMITS WindowLimits;

    if (MaximumWindow) {
        GetWindowLimits(ScreenInfo, &WindowLimits);
        WindowSize = WindowLimits.MaximumWindowSize;
    }
    else {
        WindowSize.X = (SHORT)CONSOLE_WINDOW_SIZE_X(ScreenInfo);
        WindowSize.Y = (SHORT)CONSOLE_WINDOW_SIZE_Y(ScreenInfo);
    }
    *FontIndex = SCR_FONTNUMBER(ScreenInfo);
    *FontSize = WindowSize;
    return STATUS_SUCCESS;
}

NTSTATUS
SetScreenBufferFont(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN ULONG FontIndex,
    IN UINT CodePage
    )
{
    COORD FontSize;
    WINDOW_LIMITS WindowLimits;
    NTSTATUS Status;
    ULONG ulFlagPrev;
    DBGFONTS(("SetScreenBufferFont %lx %x\n", ScreenInfo, FontIndex));

    if (ScreenInfo == NULL) {
        /* If shutdown occurs with font dlg up */
        return STATUS_SUCCESS;
    }

    /*
     * Don't try to set the font if we're not in text mode
     */
    if (!(ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER)) {
        return STATUS_UNSUCCESSFUL;
    }

    Status = GetFontSize(FontIndex, &FontSize);
    if (!NT_SUCCESS(Status)) {
        return((ULONG) Status);
    }

    ulFlagPrev = ScreenInfo->Flags;
    if (TM_IS_TT_FONT(FontInfo[FontIndex].Family)) {
        ScreenInfo->Flags &= ~CONSOLE_OEMFONT_DISPLAY;
    } else {
        ScreenInfo->Flags |= CONSOLE_OEMFONT_DISPLAY;
    }

    /*
     * Convert from UnicodeOem to Unicode or vice-versa if necessary
     */
    if ((ulFlagPrev & CONSOLE_OEMFONT_DISPLAY) != (ScreenInfo->Flags & CONSOLE_OEMFONT_DISPLAY)) {
        if (ulFlagPrev & CONSOLE_OEMFONT_DISPLAY) {
            /*
             * Must convert from UnicodeOem to real Unicode
             */
            DBGCHARS(("SetScreenBufferFont converts UnicodeOem to Unicode\n"));
            FalseUnicodeToRealUnicode(
                    ScreenInfo->BufferInfo.TextInfo.TextRows,
                    ScreenInfo->ScreenBufferSize.X * ScreenInfo->ScreenBufferSize.Y,
                    ScreenInfo->Console->OutputCP);
        } else {
            /*
             * Must convert from real Unicode to UnicodeOem
             */
            DBGCHARS(("SetScreenBufferFont converts Unicode to UnicodeOem\n"));
            RealUnicodeToFalseUnicode(
                    ScreenInfo->BufferInfo.TextInfo.TextRows,
                    ScreenInfo->ScreenBufferSize.X * ScreenInfo->ScreenBufferSize.Y,
                    ScreenInfo->Console->OutputCP);
        }
    }

    /*
     * Store font properties
     */
    Status = StoreTextBufferFontInfo(ScreenInfo,
                                     FontIndex,
                                     FontSize,
                                     FontInfo[FontIndex].Family,
                                     FontInfo[FontIndex].Weight,
                                     FontInfo[FontIndex].FaceName,
                                     CodePage);
    if (!NT_SUCCESS(Status)) {
        return((ULONG) Status);
    }

    //
    // set font
    //
    Status = SetFont(ScreenInfo);
    if (!NT_SUCCESS(Status)) {
        return((ULONG) Status);
    }

    //
    // if window is growing, make sure it's not bigger than the screen.
    //

    GetWindowLimits(ScreenInfo, &WindowLimits);
    if (WindowLimits.MaximumWindowSize.X < CONSOLE_WINDOW_SIZE_X(ScreenInfo)) {
        ScreenInfo->Window.Right -= CONSOLE_WINDOW_SIZE_X(ScreenInfo) - WindowLimits.MaximumWindowSize.X;
        ScreenInfo->WindowMaximizedX = (ScreenInfo->Window.Left == 0 &&
                                        (SHORT)(ScreenInfo->Window.Right+1) == ScreenInfo->ScreenBufferSize.X);
    }
    if (WindowLimits.MaximumWindowSize.Y < CONSOLE_WINDOW_SIZE_Y(ScreenInfo)) {
        ScreenInfo->Window.Bottom -= CONSOLE_WINDOW_SIZE_Y(ScreenInfo) - WindowLimits.MaximumWindowSize.Y;
        if (ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y > ScreenInfo->Window.Bottom) {
            ScreenInfo->Window.Top += ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y - ScreenInfo->Window.Bottom;
            ScreenInfo->Window.Bottom += ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y - ScreenInfo->Window.Bottom;
        }
        ScreenInfo->WindowMaximizedY = (ScreenInfo->Window.Top == 0 &&
                                        (SHORT)(ScreenInfo->Window.Bottom+1) == ScreenInfo->ScreenBufferSize.Y);
    }
    if (WindowLimits.MinimumWindowSize.X > CONSOLE_WINDOW_SIZE_X(ScreenInfo)) {
        if (WindowLimits.MinimumWindowSize.X > ScreenInfo->ScreenBufferSize.X) {
            COORD NewBufferSize;

            NewBufferSize.X = WindowLimits.MinimumWindowSize.X;
            NewBufferSize.Y = ScreenInfo->ScreenBufferSize.Y;
            ResizeScreenBuffer(ScreenInfo,
                               NewBufferSize,
                               FALSE
                              );
        }
        if ((ScreenInfo->Window.Left+WindowLimits.MinimumWindowSize.X) > ScreenInfo->ScreenBufferSize.X) {
            ScreenInfo->Window.Left = 0;
            ScreenInfo->Window.Right = WindowLimits.MinimumWindowSize.X-1;
        } else {
            ScreenInfo->Window.Right = ScreenInfo->Window.Left+WindowLimits.MinimumWindowSize.X-1;
        }
        ScreenInfo->WindowMaximizedX = (ScreenInfo->Window.Left == 0 &&
                                        (SHORT)(ScreenInfo->Window.Right+1) == ScreenInfo->ScreenBufferSize.X);
    }

    SetLineChar(ScreenInfo);
    {
        COORD WindowedWindowSize;

        WindowedWindowSize.X = CONSOLE_WINDOW_SIZE_X(ScreenInfo);
        WindowedWindowSize.Y = CONSOLE_WINDOW_SIZE_Y(ScreenInfo);


#if defined(FE_IME)
        if (CONSOLE_IS_DBCS_OUTPUTCP(ScreenInfo->Console))
        {
            PCONVERSIONAREA_INFORMATION ConvAreaInfo;

            ConvAreaInfo = ScreenInfo->Console->ConsoleIme.ConvAreaRoot;
            while (ConvAreaInfo) {

                Status = StoreTextBufferFontInfo(ConvAreaInfo->ScreenBuffer,
                                                 SCR_FONTNUMBER(ScreenInfo),
                                                 SCR_FONTSIZE(ScreenInfo),
                                                 SCR_FAMILY(ScreenInfo),
                                                 SCR_FONTWEIGHT(ScreenInfo),
                                                 SCR_FACENAME(ScreenInfo),
                                                 SCR_FONTCODEPAGE(ScreenInfo));
                if (!NT_SUCCESS(Status)) {
                    return((ULONG) Status);
                }

                ConvAreaInfo->ScreenBuffer->Window = ScreenInfo->Window;
                ConvAreaInfo->ScreenBuffer->BufferInfo.TextInfo.ModeIndex = ScreenInfo->BufferInfo.TextInfo.ModeIndex;

                ConvAreaInfo = ConvAreaInfo->ConvAreaNext;
            }
        }
#endif // FE_IME
    }

    //
    // resize window.  this will take care of the scroll bars too.
    //

    if (ACTIVE_SCREEN_BUFFER(ScreenInfo)) {
        SetWindowSize(ScreenInfo);
    }

    //
    // adjust cursor size.
    //

    SetCursorInformation(ScreenInfo,
                         ScreenInfo->BufferInfo.TextInfo.CursorSize,
                         (BOOLEAN)ScreenInfo->BufferInfo.TextInfo.CursorVisible
                        );

    WriteToScreen(ScreenInfo,
                  &ScreenInfo->Window);
    return STATUS_SUCCESS;
}


NTSTATUS
SetFont(
    IN OUT PSCREEN_INFORMATION ScreenInfo
    )
{
    if (ACTIVE_SCREEN_BUFFER(ScreenInfo)) {
        int FontIndex = FindCreateFont(SCR_FAMILY(ScreenInfo),
                                       SCR_FACENAME(ScreenInfo),
                                       SCR_FONTSIZE(ScreenInfo),
                                       SCR_FONTWEIGHT(ScreenInfo),
                                       SCR_FONTCODEPAGE(ScreenInfo));
        if (SelectObject(ScreenInfo->Console->hDC,FontInfo[FontIndex].hFont)==0)
            return STATUS_INVALID_PARAMETER;

        if ((DWORD)FontIndex != SCR_FONTNUMBER(ScreenInfo)) {
            NTSTATUS Status;
            Status = StoreTextBufferFontInfo(ScreenInfo,
                                             FontIndex,
                                             FontInfo[FontIndex].Size,
                                             FontInfo[FontIndex].Family,
                                             FontInfo[FontIndex].Weight,
                                             FontInfo[FontIndex].FaceName,
                                             ScreenInfo->Console->OutputCP);
            if (!NT_SUCCESS(Status)) {
                return((ULONG) Status);
            }
        }

        // hack to get text realized into DC.  this is to force the
        // attribute cache to get flushed to the server side, since
        // we select the font with a client side DC and call ExtTextOut
        // with a server side DC.
        // we then need to reset the text color, since the incorrect
        // client side color has been flushed to the server.
        {
        TEXTMETRIC tmi;

        GetTextMetricsW( ScreenInfo->Console->hDC, &tmi);
        ASSERT ((tmi.tmPitchAndFamily & 1) == 0);
        ScreenInfo->Console->LastAttributes = ScreenInfo->Attributes;
        SetTextColor(ScreenInfo->Console->hDC,ConvertAttrToRGB(ScreenInfo->Console, LOBYTE(ScreenInfo->Attributes)));
        SetBkColor(ScreenInfo->Console->hDC,ConvertAttrToRGB(ScreenInfo->Console, LOBYTE(ScreenInfo->Attributes >> 4)));
        }
    }
    return STATUS_SUCCESS;
}

int
ConvertToOem(
    IN UINT Codepage,
    IN LPWSTR Source,
    IN int SourceLength,    // in chars
    OUT LPSTR Target,
    IN int TargetLength     // in chars
    )
{
    DBGCHARS(("ConvertToOem U->%d %.*ls\n", Codepage,
            SourceLength > 10 ? 10 : SourceLength, Source));
    if (Codepage == OEMCP) {
        ULONG Length;
        NTSTATUS Status;

        Status = RtlUnicodeToOemN(Target,
                                  TargetLength,
                                  &Length,
                                  Source,
                                  SourceLength * sizeof(WCHAR)
                                 );
        if (!NT_SUCCESS(Status)) {
            return 0;
        } else {
            return Length;
        }
    } else {
        return WideCharToMultiByte(Codepage,
                                   0,
                                   Source,
                                   SourceLength,
                                   Target,
                                   TargetLength,
                                   NULL,
                                   NULL);
    }
}

int
ConvertInputToUnicode(
    IN UINT Codepage,
    IN LPSTR Source,
    IN int SourceLength,    // in chars
    OUT LPWSTR Target,
    IN int TargetLength     // in chars
    )
/*
    data in the output buffer is the true unicode value
*/
{
    DBGCHARS(("ConvertInputToUnicode %d->U %.*s\n", Codepage,
            SourceLength > 10 ? 10 : SourceLength, Source));
    if (Codepage == OEMCP) {
        ULONG Length;
        NTSTATUS Status;

        Status = RtlOemToUnicodeN(Target,
                                  TargetLength * sizeof(WCHAR),
                                  &Length,
                                  Source,
                                  SourceLength
                                 );
        if (!NT_SUCCESS(Status)) {
            return 0;
        } else {
            return Length / sizeof(WCHAR);
        }
    } else {
        return MultiByteToWideChar(Codepage,
                                   0,
                                   Source,
                                   SourceLength,
                                   Target,
                                   TargetLength);
    }
}

int
ConvertOutputToUnicode(
    IN UINT Codepage,
    IN LPSTR Source,
    IN int SourceLength,    // in chars
    OUT LPWSTR Target,
    IN int TargetLength     // in chars
    )
/*
    output data is always translated via the ansi codepage
    so glyph translation works.
*/

{
    NTSTATUS Status;
    ULONG Length;
    CHAR StackBuffer[STACK_BUFFER_SIZE];
    LPSTR pszT;

    DBGCHARS(("ConvertOutputToUnicode %d->U %.*s\n", Codepage,
            SourceLength > 10 ? 10 : SourceLength, Source));
    if (Codepage == OEMCP) {
        Status = RtlCustomCPToUnicodeN(&GlyphCP,
                           Target,
                           TargetLength * sizeof(WCHAR),
                           &Length,
                           Source,
                           SourceLength
                          );
        if (!NT_SUCCESS(Status)) {
            return 0;
        } else {
            return Length / sizeof(WCHAR);
        }
    }

    if (TargetLength > STACK_BUFFER_SIZE) {
        pszT = (LPSTR)ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),SourceLength);
        if (pszT == NULL) {
            return 0;
        }
    } else {
        pszT = StackBuffer;
    }
    RtlCopyMemory(pszT, Source, SourceLength);
    Length = MultiByteToWideChar(Codepage, MB_USEGLYPHCHARS,
            pszT, SourceLength, Target, TargetLength);
    if (pszT != StackBuffer) {
        ConsoleHeapFree(pszT);
    }
    return Length;
}

#if defined(FE_SB)
WCHAR
SB_CharToWcharGlyph(
    IN UINT Codepage,
    IN char Ch)
#else
WCHAR
CharToWcharGlyph(
    IN UINT Codepage,
    IN char Ch)
#endif
{
    WCHAR wch;
    if (Codepage == OEMCP) {
        RtlCustomCPToUnicodeN(&GlyphCP, &wch, sizeof(wch), NULL, &Ch, sizeof(Ch));
    } else {
        MultiByteToWideChar(Codepage, MB_USEGLYPHCHARS, &Ch, 1, &wch, 1);
    }
#ifdef DEBUG_PRINT
    if (Ch > 0x7F) {
        DBGCHARS(("CharToWcharGlyph %d 0x%02x -> 0x%04x\n",Codepage,(UCHAR)Ch,wch));
    }
#endif
    return wch;
}

#if defined(FE_SB)
WCHAR
SB_CharToWchar(
    IN UINT Codepage,
    IN char Ch)
#else
WCHAR
CharToWchar(
    IN UINT Codepage,
    IN char Ch)
#endif
{
    WCHAR wch;
    if (Codepage == OEMCP) {
        RtlOemToUnicodeN(&wch, sizeof(wch), NULL, &Ch, sizeof(Ch));
    } else {
        MultiByteToWideChar(Codepage, 0, &Ch, 1, &wch, 1);
    }
#ifdef DEBUG_PRINT
    if (Ch > 0x7F) {
        DBGCHARS(("CharToWchar %d 0x%02x -> 0x%04x\n",Codepage,(UCHAR)Ch,wch));
    }
#endif
    return wch;
}

char
WcharToChar(
    IN UINT Codepage,
    IN WCHAR Wchar)
{
    char ch;
    if (Codepage == OEMCP) {
        RtlUnicodeToOemN(&ch, sizeof(ch), NULL, &Wchar, sizeof(Wchar));
    } else {
        WideCharToMultiByte(Codepage, 0, &Wchar, 1, &ch, 1, NULL, NULL);
    }
#ifdef DEBUG_PRINT
    if (Wchar > 0x007F) {
        DBGCHARS(("WcharToChar %d 0x%04x -> 0x%02x\n",Codepage,Wchar,(UCHAR)ch));
    }
#endif
    return ch;
}

int
ConvertOutputToOem(
    IN UINT Codepage,
    IN LPWSTR Source,
    IN int SourceLength,    // in chars
    OUT LPSTR Target,
    IN int TargetLength     // in chars
    )
/*
    Converts SourceLength Unicode characters from Source into
    not more than TargetLength Codepage characters at Target.
    Returns the number characters put in Target. (0 if failure)
*/

{
    if (Codepage == OEMCP) {
        NTSTATUS Status;
        ULONG Length;
        // Can do this in place
        Status = RtlUnicodeToOemN(Target,
                                  TargetLength,
                                  &Length,
                                  Source,
                                  SourceLength * sizeof(WCHAR)
                                 );
        if (NT_SUCCESS(Status)) {
            return Length;
        } else {
            return 0;
        }
    } else {
        ASSERT (Source != (LPWSTR)Target);
#ifdef SOURCE_EQ_TARGET
        LPSTR pszDestTmp;
        CHAR StackBuffer[STACK_BUFFER_SIZE];

        DBGCHARS(("ConvertOutputToOem U->%d %.*ls\n", Codepage,
                SourceLength > 10 ? 10 : SourceLength, Source));

        if (TargetLength > STACK_BUFFER_SIZE) {
            pszDestTmp = (LPSTR)ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),TargetLength);
            if (pszDestTmp == NULL) {
                return 0;
            }
        } else {
            pszDestTmp = StackBuffer;
        }
        TargetLength = WideCharToMultiByte(Codepage, 0,
                Source, SourceLength,
                pszDestTmp, TargetLength, NULL, NULL);

        RtlCopyMemory(Target, pszDestTmp, TargetLength);
        if (pszDestTmp != StackBuffer) {
            ConsoleHeapFree(pszDestTmp);
        }
        return TargetLength;
#else
        DBGCHARS(("ConvertOutputToOem U->%d %.*ls\n", Codepage,
                SourceLength > 10 ? 10 : SourceLength, Source));
        return WideCharToMultiByte(Codepage, 0,
                Source, SourceLength, Target, TargetLength, NULL, NULL);
#endif
    }
}

NTSTATUS
RealUnicodeToFalseUnicode(
    IN OUT LPWSTR Source,
    IN int SourceLength,     // in chars
    IN UINT Codepage
    )

/*

    this routine converts a unicode string into the correct characters
    for an OEM (cp 437) font.  this code is needed because the gdi glyph
    mapper converts unicode to ansi using codepage 1252 to index
    font.  this is how the data is stored internally.

*/

{
    NTSTATUS Status;
    LPSTR Temp;
    ULONG TempLength;
    ULONG Length;
    CHAR StackBuffer[STACK_BUFFER_SIZE];
    BOOL NormalChars;
    int i;

    DBGCHARS(("RealUnicodeToFalseUnicode U->%d:ACP->U %.*ls\n", Codepage,
            SourceLength > 10 ? 10 : SourceLength, Source));
#if defined(FE_SB)
    if (OEMCP == WINDOWSCP && Codepage == WINDOWSCP)
        return STATUS_SUCCESS;
    if (SourceLength == 0 )
        return STATUS_SUCCESS;
#endif
    NormalChars = TRUE;
    for (i=0;i<SourceLength;i++) {
        if (Source[i] > 0x7f) {
            NormalChars = FALSE;
            break;
        }
    }
    if (NormalChars) {
        return STATUS_SUCCESS;
    }
    TempLength = SourceLength;
    if (TempLength > STACK_BUFFER_SIZE) {
        Temp = (LPSTR)ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),TempLength);
        if (Temp == NULL) {
            return STATUS_NO_MEMORY;
        }
    } else {
        Temp = StackBuffer;
    }
    if (Codepage == OEMCP) {
        Status = RtlUnicodeToOemN(Temp,
                                  TempLength,
                                  &Length,
                                  Source,
                                  SourceLength * sizeof(WCHAR)
                                 );
    } else {
        Status = WideCharToMultiByte(Codepage,
                                   0,
                                   Source,
                                   SourceLength,
                                   Temp,
                                   TempLength,
                                   NULL,
                                   NULL);
    }
    if (!NT_SUCCESS(Status)) {
        if (TempLength > STACK_BUFFER_SIZE) {
            ConsoleHeapFree(Temp);
        }
        return Status;
    }

    if (CONSOLE_IS_DBCS_ENABLED()) {
        MultiByteToWideChar(USACP,
                        0,
                        Temp,
                        TempLength,
                        Source,
                        SourceLength
                       );
    } else {
        Status = RtlMultiByteToUnicodeN(Source,
                           SourceLength * sizeof(WCHAR),
                           &Length,
                           Temp,
                           TempLength
                          );
    }

    if (TempLength > STACK_BUFFER_SIZE) {
        ConsoleHeapFree(Temp);
    }
    if (!NT_SUCCESS(Status)) {
        return Status;
    } else {
        return STATUS_SUCCESS;
    }
}

NTSTATUS
FalseUnicodeToRealUnicode(
    IN OUT LPWSTR Source,
    IN int SourceLength,     // in chars
    IN UINT Codepage
    )

/*

    this routine converts a unicode string from the internally stored
    unicode characters into the real unicode characters.

*/

{
    NTSTATUS Status;
    LPSTR Temp;
    ULONG TempLength;
    ULONG Length;
    CHAR StackBuffer[STACK_BUFFER_SIZE];
    BOOL NormalChars;
    int i;

    DBGCHARS(("UnicodeAnsiToUnicodeAnsi U->ACP:%d->U %.*ls\n", Codepage,
            SourceLength > 10 ? 10 : SourceLength, Source));
#if defined(FE_SB)
    if (OEMCP == WINDOWSCP && Codepage == WINDOWSCP)
        return STATUS_SUCCESS;
    if (SourceLength == 0 )
        return STATUS_SUCCESS;
#endif
    NormalChars = TRUE;
    /*
     * Test for characters < 0x20 or >= 0x7F.  If none are found, we don't have
     * any conversion to do!
     */
    for (i=0;i<SourceLength;i++) {
        if ((USHORT)(Source[i] - 0x20) > 0x5e) {
            NormalChars = FALSE;
            break;
        }
    }
    if (NormalChars) {
        return STATUS_SUCCESS;
    }

    TempLength = SourceLength;
    if (TempLength > STACK_BUFFER_SIZE) {
        Temp = (LPSTR)ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),TempLength);
        if (Temp == NULL) {
            return STATUS_NO_MEMORY;
        }
    } else {
        Temp = StackBuffer;
    }
    if (CONSOLE_IS_DBCS_ENABLED()) {
        Status = WideCharToMultiByte(USACP,
                                 0,
                                 Source,
                                 SourceLength,
                                 Temp,
                                 TempLength,
                                 NULL,
                                 NULL);
    } else {
        Status = RtlUnicodeToMultiByteN(Temp,
                                    TempLength,
                                    &Length,
                                    Source,
                                    SourceLength * sizeof(WCHAR)
                                   );
    }

    if (!NT_SUCCESS(Status)) {
        if (TempLength > STACK_BUFFER_SIZE) {
            ConsoleHeapFree(Temp);
        }
        return Status;
    }
    if (Codepage == OEMCP) {
        Status = RtlCustomCPToUnicodeN(&GlyphCP,
                                  Source,
                                  SourceLength * sizeof(WCHAR),
                                  &Length,
                                  Temp,
                                  TempLength
                                 );
    } else {
        Status = MultiByteToWideChar(Codepage,
                                   MB_USEGLYPHCHARS,
                                   Temp,
                                   TempLength*sizeof(WCHAR),
                                   Source,
                                   SourceLength);
    }
#if defined(FE_SB)
    if (SourceLength > STACK_BUFFER_SIZE) {
        ConsoleHeapFree(Temp);
    }
#else
    if (TempLength > STACK_BUFFER_SIZE) {
        ConsoleHeapFree(Temp);
    }
#endif
    if (!NT_SUCCESS(Status)) {
        return Status;
    } else {
        return STATUS_SUCCESS;
    }
}


BOOL InitializeCustomCP() {
    PPEB pPeb;

    pPeb = NtCurrentPeb();
    if ((pPeb == NULL) || (pPeb->OemCodePageData == NULL)) {
        return FALSE;
    }

    /*
     * Fill in the CPTABLEINFO struct
     */
    RtlInitCodePageTable(pPeb->OemCodePageData, &GlyphCP);

    /*
     * Make a copy of the MultiByteToWideChar table
     */
    RtlCopyMemory(GlyphTable, GlyphCP.MultiByteTable, 256 * sizeof(USHORT));

    /*
     * Modify the first 0x20 bytes so that they are glyphs.
     */
    MultiByteToWideChar(CP_OEMCP, MB_USEGLYPHCHARS,
            "\x20\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"
            "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F",
            0x20, GlyphTable, 0x20);
    MultiByteToWideChar(CP_OEMCP, MB_USEGLYPHCHARS,
            "\x7f", 1, &GlyphTable[0x7f], 1);


    /*
     * Point the Custom CP at the glyph table
     */
    GlyphCP.MultiByteTable = GlyphTable;

#if defined(FE_SB) && defined(i386)
    if (ISNECPC98(gdwMachineId)) {
        InitializeNEC_OS2_CP();
    }
#endif
    return TRUE;
}

#if defined(FE_SB)
VOID
SetConsoleCPInfo(
    IN PCONSOLE_INFORMATION Console,
    IN BOOL Output
    )
{
    if (Output) {
        if (! GetCPInfo(Console->OutputCP,
                        &Console->OutputCPInfo)) {
            Console->OutputCPInfo.LeadByte[0] = 0;
        }
    }
    else {
        if (! GetCPInfo(Console->CP,
                        &Console->CPInfo)) {
            Console->CPInfo.LeadByte[0] = 0;
        }
    }
}

BOOL
CheckBisectStringW(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN DWORD CodePage,
    IN PWCHAR Buffer,
    IN DWORD NumWords,
    IN DWORD NumBytes
    )

/*++

Routine Description:

    This routine check bisected on Unicode string end.

Arguments:

    ScreenInfo - Pointer to screen information structure.

    CodePage - Value of code page.

    Buffer - Pointer to Unicode string buffer.

    NumWords - Number of Unicode string.

    NumBytes - Number of bisect position by byte counts.

Return Value:

    TRUE - Bisected character.

    FALSE - Correctly.

--*/

{
    while(NumWords && NumBytes) {
        if (IsConsoleFullWidth(ScreenInfo->Console->hDC,CodePage,*Buffer)) {
            if (NumBytes < 2)
                return TRUE;
            else {
                NumWords--;
                NumBytes -= 2;
                Buffer++;
            }
        }
        else {
            NumWords--;
            NumBytes--;
            Buffer++;
        }
    }
    return FALSE;
}

BOOL
CheckBisectProcessW(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN DWORD CodePage,
    IN PWCHAR Buffer,
    IN DWORD NumWords,
    IN DWORD NumBytes,
    IN SHORT OriginalXPosition,
    IN BOOL Echo
    )

/*++

Routine Description:

    This routine check bisected on Unicode string end.

Arguments:

    ScreenInfo - Pointer to screen information structure.

    CodePage - Value of code page.

    Buffer - Pointer to Unicode string buffer.

    NumWords - Number of Unicode string.

    NumBytes - Number of bisect position by byte counts.

    Echo - TRUE if called by Read (echoing characters)

Return Value:

    TRUE - Bisected character.

    FALSE - Correctly.

--*/

{
    WCHAR Char;
    ULONG TabSize;

    if (ScreenInfo->OutputMode & ENABLE_PROCESSED_OUTPUT) {
        while(NumWords && NumBytes) {
            Char = *Buffer;
            if (Char >= (WCHAR)' ') {
                if (IsConsoleFullWidth(ScreenInfo->Console->hDC,CodePage,Char)) {
                    if (NumBytes < 2)
                        return TRUE;
                    else {
                        NumWords--;
                        NumBytes -= 2;
                        Buffer++;
                        OriginalXPosition += 2;
                    }
                }
                else {
                    NumWords--;
                    NumBytes--;
                    Buffer++;
                    OriginalXPosition++;
                }
            }
            else {
                NumWords--;
                Buffer++;
                switch (Char) {
                    case UNICODE_BELL:
                        if (Echo)
                            goto CtrlChar;
                        break;
                    case UNICODE_BACKSPACE:
                    case UNICODE_LINEFEED:
                    case UNICODE_CARRIAGERETURN:
                        break;
                    case UNICODE_TAB:
                        TabSize = NUMBER_OF_SPACES_IN_TAB(OriginalXPosition);
                        OriginalXPosition = (SHORT)(OriginalXPosition + TabSize);
                        if (NumBytes < TabSize)
                            return TRUE;
                        NumBytes -= TabSize;
                        break;
                    default:
                        if (Echo) {
                    CtrlChar:
                            if (NumBytes < 2)
                                return TRUE;
                            NumBytes -= 2;
                            OriginalXPosition += 2;
                        } else {
                            NumBytes--;
                            OriginalXPosition++;
                        }
                }
            }
        }
        return FALSE;
    }
    else {
        return CheckBisectStringW(ScreenInfo,
                                  CodePage,
                                  Buffer,
                                  NumWords,
                                  NumBytes);
    }
}
#endif // FE_SB

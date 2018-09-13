/*++

Copyright (c) 1990  Microsoft Corporation

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
ULONG gDebugFlag = 0 ;
// ULONG gDebugFlag = _DBGOUTPUT | _DBGCHARS | _DBGFONTS | _DBGFONTS2 ;
#endif

PFONT_INFO FontInfo = NULL;
ULONG FontInfoLength;
ULONG NumberOfFonts;
BOOL gbEnumerateFaces = FALSE;


#define FE_ABANDONFONT 0
#define FE_SKIPFONT    1
#define FE_FONTOK      2

/*
 * Initial default fonts and face names
 */
PFACENODE gpFaceNames = NULL;

/*
 * TTPoints -- Initial font pixel heights for TT fonts
 */
SHORT TTPoints[] = {
    5, 6, 7, 8, 10, 12, 14, 16, 18, 20, 24, 28, 36, 72
};
#if defined(FE_SB)
/*
 * TTPointsDbcs -- Initial font pixel heights for TT fonts of DBCS.
 * So, This list except odd point size because font width is (SBCS:DBCS != 1:2).
 */
SHORT TTPointsDbcs[] = {
    6, 8, 10, 12, 14, 16, 18, 20, 24, 28, 36, 72
};
#endif


typedef struct _FONTENUMDATA {
    HDC hDC;
    BOOL bFindFaces;
    ULONG ulFE;
    PSHORT pTTPoints;
    UINT nTTPoints;
} FONTENUMDATA, *PFONTENUMDATA;


PFACENODE
AddFaceNode(PFACENODE *ppStart, LPTSTR ptsz) {
    PFACENODE pNew;
    PFACENODE *ppTmp;
    int cb;

    /*
     * Is it already here?
     */
    for (ppTmp = ppStart; *ppTmp; ppTmp = &((*ppTmp)->pNext)) {
        if (_tcscmp(((*ppTmp)->atch), ptsz) == 0) {
            // already there !
            return *ppTmp;
        }
    }

    cb = (_tcslen(ptsz) + 1) * sizeof(TCHAR);
    pNew = (PFACENODE)HeapAlloc(RtlProcessHeap(),0,sizeof(FACENODE) + cb);
    if (pNew == NULL) {
        return NULL;
    }

    pNew->pNext = NULL;
    pNew->dwFlag = 0;
    _tcscpy(pNew->atch, ptsz);
    *ppTmp = pNew;
    return pNew;
}


VOID
DestroyFaceNodes( VOID ) {
    PFACENODE pNext;
    PFACENODE pTmp;

    pTmp = gpFaceNames;
    while (pTmp != NULL) {
        pNext = pTmp->pNext;
        HeapFree(RtlProcessHeap(), 0, pTmp);
        pTmp = pNext;
    }
    gpFaceNames = NULL;
}


int
AddFont(
    ENUMLOGFONT *pelf,
    NEWTEXTMETRIC *pntm,
    int nFontType,
    HDC hDC,
    PFACENODE pFN
    )

/*++

    Add the font desribed by the LOGFONT structure to the font table if
    it's not already there.

--*/

{
    HFONT hFont;
    TEXTMETRIC tm;
    LONG nFont;
    COORD SizeToShow;
    COORD SizeActual;
    COORD SizeWant;
    BYTE tmFamily;
    SIZE Size;
    LPTSTR ptszFace = pelf->elfLogFont.lfFaceName;

    /* get font info */
    SizeWant.Y = (SHORT)pelf->elfLogFont.lfHeight;
    SizeWant.X = (SHORT)pelf->elfLogFont.lfWidth;
CreateBoldFont:
    pelf->elfLogFont.lfQuality = NONANTIALIASED_QUALITY;
    hFont = CreateFontIndirect(&pelf->elfLogFont);
    ASSERT(hFont);
    if (!hFont) {
        DBGFONTS(("    REJECT  font (can't create)\n"));
        return FE_SKIPFONT;  // same font in other sizes may still be suitable
    }

    DBGFONTS2(("    hFont = %lx\n", hFont));

    //
    // for reasons unbeknownst to me, removing this code causes GDI
    // to yack, claiming that the font is owned by another process.
    //

    SelectObject(hDC, hFont);
    GetTextMetrics(hDC, &tm);

    GetTextExtentPoint32(hDC, TEXT("0"), 1, &Size);
    SizeActual.X = (SHORT)Size.cx;
    SizeActual.Y = (SHORT)(tm.tmHeight + tm.tmExternalLeading);
    DBGFONTS2(("    actual size %d,%d\n", SizeActual.X, SizeActual.Y));
    tmFamily = tm.tmPitchAndFamily;
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
    //ASSERT (tm.tmMaxCharWidth == pntm->tmMaxCharWidth);

    /*
     * NOW, determine whether this font entry has already been cached
     * LATER : it may be possible to do this before creating the font, if
     * we can trust the dimensions & other info from pntm.
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
                    break;
                }
            }
        }

        // DBGFONTS(("    SizeShown(%x) = (%d,%d)\n",nFont,SizeShown.X,SizeShown.Y));

        if (SIZE_EQUAL(SizeShown, SizeToShow) &&
                FontInfo[nFont].Family == tmFamily &&
                FontInfo[nFont].Weight == tm.tmWeight &&
                _tcscmp(FontInfo[nFont].FaceName, ptszFace) == 0) {
            /*
             * Already have this font
             */
            DBGFONTS2(("    Already have the font\n"));
            DeleteObject(hFont);
            return FE_FONTOK;
        }


        if ((SizeToShow.Y < SizeShown.Y) ||
                (SizeToShow.Y == SizeShown.Y && SizeToShow.X < SizeShown.X)) {
            /*
             * This new font is smaller than nFont
             */
            DBGFONTS(("INSERT at %x, SizeToShow = (%d,%d)\n", nFont,
                    SizeToShow.X,SizeToShow.Y));
            break;
        }
    }

    /*
     * If we have to grow our font table, do it
     */
    if (NumberOfFonts == FontInfoLength) {
        PFONT_INFO Temp;

        FontInfoLength += FONT_INCREMENT;
        Temp = (PFONT_INFO)HeapReAlloc(RtlProcessHeap(), 0, FontInfo,
                                       sizeof(FONT_INFO) * FontInfoLength);
        ASSERT(Temp);
        if (Temp == NULL) {
            FontInfoLength -= FONT_INCREMENT;
            return FE_ABANDONFONT;  // no point enumerating more - no memory!
        }
        FontInfo = Temp;
    }

    /*
     * The font we are adding should be inserted into the list,
     * if it is smaller than the last one.
     */
    if (nFont < (LONG)NumberOfFonts) {
        RtlMoveMemory(&FontInfo[nFont+1],
                      &FontInfo[nFont],
                      sizeof(FONT_INFO) * (NumberOfFonts - nFont));
    }

    /*
     * Store the font info
     */
    FontInfo[nFont].hFont = hFont;
    FontInfo[nFont].Family = tmFamily;
    FontInfo[nFont].Size = SizeActual;
    if (TM_IS_TT_FONT(tmFamily)) {
        FontInfo[nFont].SizeWant = SizeWant;
    } else {
        FontInfo[nFont].SizeWant.X = 0;
        FontInfo[nFont].SizeWant.Y = 0;
    }
    FontInfo[nFont].Weight = tm.tmWeight;
    FontInfo[nFont].FaceName = pFN->atch;
#if defined(FE_SB)
    FontInfo[nFont].tmCharSet = tm.tmCharSet;
#endif

    ++NumberOfFonts;

    /*
     * If this is a true type font, create a bold version too.
     */
    if (nFontType == TRUETYPE_FONTTYPE && !IS_BOLD(FontInfo[nFont].Weight)) {
          pelf->elfLogFont.lfWeight = FW_BOLD;
          goto CreateBoldFont;
    }

    return FE_FONTOK;  // and continue enumeration
}


VOID
InitializeFonts( VOID )
{
    EnumerateFonts(EF_DEFFACE);  // Just the Default font
}


VOID
DestroyFonts( VOID )
{
    ULONG FontIndex;

    if (FontInfo != NULL) {
        for (FontIndex = 0; FontIndex < NumberOfFonts; FontIndex++) {
            DeleteObject(FontInfo[FontIndex].hFont);
        }
        HeapFree(RtlProcessHeap(), 0, FontInfo);
        FontInfo = NULL;
        NumberOfFonts = 0;
    }

    DestroyFaceNodes();
}


/*
 * Returns bit combination
 *  FE_ABANDONFONT  - do not continue enumerating this font
 *  FE_SKIPFONT     - skip this font but keep enumerating
 *  FE_FONTOK       - font was created and added to cache or already there
 */
int
FontEnum(
    ENUMLOGFONT *pelf,
    NEWTEXTMETRIC *pntm,
    int nFontType,
    PFONTENUMDATA pfed
    )

/*++

    Is called exactly once by GDI for each font in the system.  This
    routine is used to store the FONT_INFO structure.

--*/

{
    UINT i;
    LPTSTR ptszFace = pelf->elfLogFont.lfFaceName;
    PFACENODE pFN;

    DBGFONTS(("  FontEnum \"%ls\" (%d,%d) weight 0x%lx(%d) %x -- %s\n",
            ptszFace,
            pelf->elfLogFont.lfWidth, pelf->elfLogFont.lfHeight,
            pelf->elfLogFont.lfWeight, pelf->elfLogFont.lfWeight,
            pelf->elfLogFont.lfCharSet,
            pfed->bFindFaces ? "Finding Faces" : "Creating Fonts"));

    //
    // reject variable width and italic fonts, also tt fonts with neg ac
    //


    if
    (
      !(pelf->elfLogFont.lfPitchAndFamily & FIXED_PITCH) ||
      (pelf->elfLogFont.lfItalic)                        ||
      !(pntm->ntmFlags & NTM_NONNEGATIVE_AC)
    )
    {
        if (! IsAvailableTTFont(ptszFace)) {
            DBGFONTS(("    REJECT  face (dbcs, variable pitch, italic, or neg a&c)\n"));
            return pfed->bFindFaces ? FE_SKIPFONT : FE_ABANDONFONT;
        }
    }

    /*
     * reject TT fonts for whoom family is not modern, that is do not use
     * FF_DONTCARE    // may be surprised unpleasantly
     * FF_DECORATIVE  // likely to be symbol fonts
     * FF_SCRIPT      // cursive, inappropriate for console
     * FF_SWISS OR FF_ROMAN // variable pitch
     */

    if ((nFontType == TRUETYPE_FONTTYPE) &&
            ((pelf->elfLogFont.lfPitchAndFamily & 0xf0) != FF_MODERN)) {
        DBGFONTS(("    REJECT  face (TT but not FF_MODERN)\n"));
        return pfed->bFindFaces ? FE_SKIPFONT : FE_ABANDONFONT;
    }

    /*
     * reject non-TT fonts that aren't OEM
     */
    if ((nFontType != TRUETYPE_FONTTYPE) &&
#if defined(FE_SB)
            (!gfFESystem || !IS_ANY_DBCS_CHARSET(pelf->elfLogFont.lfCharSet)) &&
#endif
            (pelf->elfLogFont.lfCharSet != OEM_CHARSET)) {
        DBGFONTS(("    REJECT  face (not TT nor OEM)\n"));
        return pfed->bFindFaces ? FE_SKIPFONT : FE_ABANDONFONT;
    }

    /*
     * reject non-TT fonts that are virtical font
     */
    if ((nFontType != TRUETYPE_FONTTYPE) &&
            (ptszFace[0] == TEXT('@'))) {
        DBGFONTS(("    REJECT  face (not TT and TATEGAKI)\n"));
        return pfed->bFindFaces ? FE_SKIPFONT : FE_ABANDONFONT;
    }

    /*
     * reject non-TT fonts that aren't Terminal
     */
    if (gfFESystem && (nFontType != TRUETYPE_FONTTYPE) &&
            (_tcscmp(ptszFace, TEXT("Terminal")) != 0)) {
        DBGFONTS(("    REJECT  face (not TT nor Terminal)\n"));
        return pfed->bFindFaces ? FE_SKIPFONT : FE_ABANDONFONT;
    }

    /*
     * reject Far East TT fonts that aren't Far East charset.
     */
    if (IsAvailableTTFont(ptszFace) &&
        !IS_ANY_DBCS_CHARSET(pelf->elfLogFont.lfCharSet) &&
        !IsAvailableTTFontCP(ptszFace,0)
       ) {
        DBGFONTS(("    REJECT  face (Far East TT and not Far East charset)\n"));
        return FE_SKIPFONT;    // should be enumerate next charset.
    }

    /*
     * Add or find the facename
     */
    pFN = AddFaceNode(&gpFaceNames, ptszFace);
    if (pFN == NULL) {
        return FE_ABANDONFONT;
    }

    if (pfed->bFindFaces) {
        DWORD dwFontType;
        if (nFontType == TRUETYPE_FONTTYPE) {
            DBGFONTS(("NEW TT FACE %ls\n", ptszFace));
            dwFontType = EF_TTFONT;
        } else if (nFontType == RASTER_FONTTYPE) {
            DBGFONTS(("NEW OEM FACE %ls\n",ptszFace));
            dwFontType = EF_OEMFONT;
        }
        pFN->dwFlag |= dwFontType | EF_NEW;
#if defined(FE_SB)
        if (IS_ANY_DBCS_CHARSET(pelf->elfLogFont.lfCharSet))
            pFN->dwFlag |= EF_DBCSFONT;
#endif
        return FE_SKIPFONT;
    }


    if (IS_BOLD(pelf->elfLogFont.lfWeight)) {
        DBGFONTS2(("    A bold font (weight %d)\n", pelf->elfLogFont.lfWeight));
        // return FE_SKIPFONT;
    }

    /*
     * Add the font to the table. If this is a true type font, add the
     * sizes from the array. Otherwise, just add the size we got.
     */
    if (nFontType & TRUETYPE_FONTTYPE) {
        for (i = 0; i < pfed->nTTPoints; i++) {
            pelf->elfLogFont.lfHeight = pfed->pTTPoints[i];
            pelf->elfLogFont.lfWidth  = 0;
            pelf->elfLogFont.lfWeight = 400;
            pfed->ulFE |= AddFont(pelf, pntm, nFontType, pfed->hDC, pFN);
            if (pfed->ulFE & FE_ABANDONFONT) {
                return FE_ABANDONFONT;
            }
        }
    } else {
            pfed->ulFE |= AddFont(pelf, pntm, nFontType, pfed->hDC, pFN);
            if (pfed->ulFE & FE_ABANDONFONT) {
                return FE_ABANDONFONT;
            }
    }

    return FE_FONTOK;  // and continue enumeration
}

BOOL
DoFontEnum(
    HDC hDC,
    LPTSTR ptszFace,
    PSHORT pTTPoints,
    UINT nTTPoints)
{
    BOOL bDeleteDC = FALSE;
    FONTENUMDATA fed;
    LOGFONT LogFont;

    DBGFONTS(("DoFontEnum \"%ls\"\n", ptszFace));
    if (hDC == NULL) {
        hDC = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
        bDeleteDC = TRUE;
    }

    fed.hDC = hDC;
    fed.bFindFaces = (ptszFace == NULL);
    fed.ulFE = 0;
    fed.pTTPoints = pTTPoints;
    fed.nTTPoints = nTTPoints;
    RtlZeroMemory(&LogFont, sizeof(LOGFONT));
    LogFont.lfCharSet = DEFAULT_CHARSET;
    if (ptszFace)
        _tcscpy(LogFont.lfFaceName, ptszFace);
    /*
     * EnumFontFamiliesEx function enumerates one font in every face in every character set. 
     */
    EnumFontFamiliesEx(hDC, &LogFont, (FONTENUMPROC)FontEnum, (LPARAM)&fed, 0);
    if (bDeleteDC) {
        DeleteDC(hDC);
    }
    return (fed.ulFE & FE_FONTOK) != 0;
}


VOID
RemoveFace(LPTSTR ptszFace)
{
    DWORD i;
    int nToRemove = 0;

    DBGFONTS(("RemoveFace %ls\n", ptszFace));
    //
    // Delete & Remove fonts with Face Name == ptszFace
    //
    for (i = 0; i < NumberOfFonts; i++) {
        if (_tcscmp(FontInfo[i].FaceName, ptszFace) == 0) {
            BOOL bDeleted = DeleteObject(FontInfo[i].hFont);
            DBGPRINT(("RemoveFace: hFont %lx was %sdeleted\n",
                    FontInfo[i].hFont, bDeleted ? "" : "NOT "));
            FontInfo[i].hFont = NULL;
            nToRemove++;
        } else if (nToRemove > 0) {
            /*
             * Shuffle from FontInfo[i] down nToRemove slots.
             */
            RtlMoveMemory(&FontInfo[i - nToRemove],
                    &FontInfo[i],
                    sizeof(FONT_INFO)*(NumberOfFonts - i));
            NumberOfFonts -= nToRemove;
            i -= nToRemove;
            nToRemove = 0;
        }
    }
    NumberOfFonts -= nToRemove;
}

TCHAR DefaultFaceName[LF_FACESIZE];
COORD DefaultFontSize;
BYTE  DefaultFontFamily;
ULONG DefaultFontIndex = 0;
ULONG CurrentFontIndex = 0;

NTSTATUS
EnumerateFonts(
    DWORD Flags)
{
    TEXTMETRIC tm;
    HDC hDC;
    PFACENODE pFN;
    ULONG ulOldEnumFilter;
    BOOL  bEnumOEMFace = TRUE;
    DWORD FontIndex;
    DWORD dwFontType = 0;

    DBGFONTS(("EnumerateFonts %lx\n", Flags));

    dwFontType = (EF_TTFONT|EF_OEMFONT|EF_DEFFACE) & Flags;

    if (FontInfo == NULL) {
        //
        // allocate memory for the font array
        //
        NumberOfFonts = 0;

        FontInfo = (PFONT_INFO)HeapAlloc(RtlProcessHeap(),0,sizeof(FONT_INFO) * INITIAL_FONTS);
        if (FontInfo == NULL)
            return STATUS_NO_MEMORY;
        FontInfoLength = INITIAL_FONTS;
    }

    hDC = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);

    // Before enumeration, turn off font enumeration filters.
    ulOldEnumFilter = SetFontEnumeration(0);
    SetFontEnumeration(ulOldEnumFilter & ~FE_FILTER_TRUETYPE);

    if (Flags & EF_DEFFACE) {
        SelectObject(hDC, GetStockObject(OEM_FIXED_FONT));
        GetTextMetrics(hDC, &tm);
        GetTextFace(hDC, LF_FACESIZE, DefaultFaceName);

        DefaultFontSize.X = (SHORT)(tm.tmMaxCharWidth);
        DefaultFontSize.Y = (SHORT)(tm.tmHeight+tm.tmExternalLeading);
        DefaultFontFamily = tm.tmPitchAndFamily;
#if !defined(FE_SB)
        DBGFONTS(("Default (OEM) Font %ls (%d,%d)\n", DefaultFaceName,
                DefaultFontSize.X, DefaultFontSize.Y));
#else
        if (IS_ANY_DBCS_CHARSET(tm.tmCharSet))
            DefaultFontSize.X /= 2;
        DBGFONTS(("Default (OEM) Font %ls (%d,%d) CharSet 0x%02X\n", DefaultFaceName,
                DefaultFontSize.X, DefaultFontSize.Y,
                tm.tmCharSet));
#endif

        // Make sure we are going to enumerate the OEM face.
        pFN = AddFaceNode(&gpFaceNames, DefaultFaceName);
        pFN->dwFlag |= EF_DEFFACE | EF_OEMFONT;
    }

    if (gbEnumerateFaces) {
        /*
         * Set the EF_OLD bit and clear the EF_NEW bit
         * for all previously available faces
         */
        for (pFN = gpFaceNames; pFN; pFN = pFN->pNext) {
            pFN->dwFlag |= EF_OLD;
            pFN->dwFlag &= ~EF_NEW;
        }

        //
        // Use DoFontEnum to get the names of all the suitable Faces
        // All facenames found will be put in gpFaceNames with
        // the EF_NEW bit set.
        //
        DoFontEnum(hDC, NULL, TTPoints, 1);
        gbEnumerateFaces = FALSE;
    }

    // Use DoFontEnum to get all fonts from the system.  Our FontEnum
    // proc puts just the ones we want into an array
    //
    for (pFN = gpFaceNames; pFN; pFN = pFN->pNext) {
        DBGFONTS(("\"%ls\" is %s%s%s%s%s%s\n", pFN->atch,
            pFN->dwFlag & EF_NEW        ? "NEW "        : " ",
            pFN->dwFlag & EF_OLD        ? "OLD "        : " ",
            pFN->dwFlag & EF_ENUMERATED ? "ENUMERATED " : " ",
            pFN->dwFlag & EF_OEMFONT    ? "OEMFONT "    : " ",
            pFN->dwFlag & EF_TTFONT     ? "TTFONT "     : " ",
            pFN->dwFlag & EF_DEFFACE    ? "DEFFACE "    : " "));

        if ((pFN->dwFlag & (EF_OLD|EF_NEW)) == EF_OLD) {
            // The face is no longer available
            RemoveFace(pFN->atch);
            pFN->dwFlag &= ~EF_ENUMERATED;
            continue;
        }
        if ((pFN->dwFlag & dwFontType) == 0) {
            // not the kind of face we want
            continue;
        }
        if (pFN->dwFlag & EF_ENUMERATED) {
            // we already enumerated this face
            continue;
        }

        if (pFN->dwFlag & EF_TTFONT) {
#if defined(FE_SB)
            if (gfFESystem && !IsAvailableTTFontCP(pFN->atch,0))
                DoFontEnum(hDC, pFN->atch, TTPointsDbcs, NELEM(TTPointsDbcs));
            else
#endif
                DoFontEnum(hDC, pFN->atch, TTPoints, NELEM(TTPoints));
        } else {
            DoFontEnum(hDC, pFN->atch, NULL, 0);

            // If we find that the face just enumerated is the same as OEM,
            // reset flag so we don't try to enumerate it again.

            if (!_tcsncmp(pFN->atch, DefaultFaceName, LF_FACESIZE)) {
                bEnumOEMFace = FALSE;
            }
        }
        pFN->dwFlag |= EF_ENUMERATED;
    }


    // After enumerating fonts, restore the font enumeration filter.
    SetFontEnumeration(ulOldEnumFilter);

    DeleteDC(hDC);

#if defined(FE_SB)
    if (gfFESystem )
    {
        for (FontIndex = 0; FontIndex < NumberOfFonts; FontIndex++) {
            if (FontInfo[FontIndex].Size.X == DefaultFontSize.X &&
                FontInfo[FontIndex].Size.Y == DefaultFontSize.Y &&
                IS_ANY_DBCS_CHARSET(FontInfo[FontIndex].tmCharSet) &&
                FontInfo[FontIndex].Family == DefaultFontFamily) {
                break;
            }
        }
    }
    else
    {
#endif
    for (FontIndex = 0; FontIndex < NumberOfFonts; FontIndex++) {
        if (FontInfo[FontIndex].Size.X == DefaultFontSize.X &&
            FontInfo[FontIndex].Size.Y == DefaultFontSize.Y &&
            FontInfo[FontIndex].Family == DefaultFontFamily) {
            break;
        }
    }
#if defined(FE_SB)
    }
#endif
    ASSERT(FontIndex < NumberOfFonts);
    if (FontIndex < NumberOfFonts) {
        DefaultFontIndex = FontIndex;
    } else {
        DefaultFontIndex = 0;
    }
    DBGFONTS(("EnumerateFonts : DefaultFontIndex = %ld\n", DefaultFontIndex));

    return STATUS_SUCCESS;
}


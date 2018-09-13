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

#include "shellprv.h"
#pragma hdrstop

#include "lnkcon.h"

#define CONSOLE_REGISTRY_STRING       (TEXT("Console"))
#define CONSOLE_REGISTRY_FONTSIZE     (TEXT("FontSize"))
#define CONSOLE_REGISTRY_FONTFAMILY   (TEXT("FontFamily"))
#define CONSOLE_REGISTRY_BUFFERSIZE   (TEXT("ScreenBufferSize"))
#define CONSOLE_REGISTRY_CURSORSIZE   (TEXT("CursorSize"))
#define CONSOLE_REGISTRY_WINDOWSIZE   (TEXT("WindowSize"))
#define CONSOLE_REGISTRY_WINDOWPOS    (TEXT("WindowPosition"))
#define CONSOLE_REGISTRY_FILLATTR     (TEXT("ScreenColors"))
#define CONSOLE_REGISTRY_POPUPATTR    (TEXT("PopupColors"))
#define CONSOLE_REGISTRY_FULLSCR      (TEXT("FullScreen"))
#define CONSOLE_REGISTRY_QUICKEDIT    (TEXT("QuickEdit"))
#define CONSOLE_REGISTRY_FACENAME     (TEXT("FaceName"))
#define CONSOLE_REGISTRY_FONTWEIGHT   (TEXT("FontWeight"))
#define CONSOLE_REGISTRY_INSERTMODE   (TEXT("InsertMode"))
#define CONSOLE_REGISTRY_HISTORYSIZE  (TEXT("HistoryBufferSize"))
#define CONSOLE_REGISTRY_HISTORYBUFS  (TEXT("NumberOfHistoryBuffers"))
#define CONSOLE_REGISTRY_HISTORYNODUP (TEXT("HistoryNoDup"))
#define CONSOLE_REGISTRY_COLORTABLE   (TEXT("ColorTable%02u"))
#define CONSOLE_REGISTRY_CODEPAGE     (TEXT("CodePage"))


/*
 * Initial default fonts and face names
 */

/*
 * TTPoints -- Initial font pixel heights for TT fonts
 */
SHORT TTPoints[] = {
    5, 6, 7, 8, 10, 12, 14, 16, 18, 20, 24, 28, 36, 72
};
/*
 * TTPointsDbcs -- Initial font pixel heights for TT fonts of DBCS.
 */
SHORT TTPointsDbcs[] = {
    6, 8, 10, 12, 14, 16, 18, 20, 24, 28, 36, 72
};


typedef struct _FONTENUMDATA {
    LPCONSOLEPROP_DATA pcpd;
    HDC hDC;
    BOOL bFindFaces;
    ULONG ulFE;
    PSHORT pTTPoints;
    UINT nTTPoints;
    UINT uDefCP;
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
        if (lstrcmp(((*ppTmp)->atch), ptsz) == 0) {
            // already there !
            return *ppTmp;
        }
    }

    cb = (lstrlen(ptsz) + 1) * SIZEOF(TCHAR);
    pNew = (PFACENODE)LocalAlloc(LPTR ,SIZEOF(FACENODE) + cb);
    if (pNew == NULL) {
        return NULL;
    }

    pNew->pNext = NULL;
    pNew->dwFlag = 0;
    lstrcpy(pNew->atch, ptsz);
    *ppTmp = pNew;
    return pNew;
}


VOID
DestroyFaceNodes( LPCONSOLEPROP_DATA pcpd ) {
    PFACENODE pNext;
    PFACENODE pTmp;

    pTmp = pcpd->gpFaceNames;
    while (pTmp != NULL) {
        pNext = pTmp->pNext;
        LocalFree(pTmp);
        pTmp = pNext;
    }
    pcpd->gpFaceNames = NULL;
}


int
AddFont(
    LPCONSOLEPROP_DATA pcpd,
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
    hFont = CreateFontIndirect(&pelf->elfLogFont);
    ASSERT(hFont);
    if (!hFont) {
        return FE_SKIPFONT;  // same font in other sizes may still be suitable
    }

    //
    // for reasons unbeknownst to me, removing this code causes GDI
    // to yack, claiming that the font is owned by another process.
    //

    SelectObject(hDC, hFont);
    GetTextMetrics(hDC, &tm);

    GetTextExtentPoint32(hDC, TEXT("0"), 1, &Size);
    SizeActual.X = (SHORT)Size.cx;
    SizeActual.Y = (SHORT)(tm.tmHeight + tm.tmExternalLeading);
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
    for (nFont = 0; nFont < (LONG)pcpd->NumberOfFonts; ++nFont) {
        COORD SizeShown;

        if (pcpd->FontInfo[nFont].hFont == NULL) {
            continue;
        }

        if (pcpd->FontInfo[nFont].SizeWant.X > 0) {
            SizeShown.X = pcpd->FontInfo[nFont].SizeWant.X;
        } else {
            SizeShown.X = pcpd->FontInfo[nFont].Size.X;
        }

        if (pcpd->FontInfo[nFont].SizeWant.Y > 0) {
            // This is a font specified by cell height.
            SizeShown.Y = pcpd->FontInfo[nFont].SizeWant.Y;
        } else {
            SizeShown.Y = pcpd->FontInfo[nFont].Size.Y;
            if (pcpd->FontInfo[nFont].SizeWant.Y < 0) {
                // This is a TT font specified by character height.
                if (SizeWant.Y < 0 && SizeWant.Y > pcpd->FontInfo[nFont].SizeWant.Y) {
                    // Requested pixelheight is smaller than this one.
                    break;
                }
            }
        }


        if (SIZE_EQUAL(SizeShown, SizeToShow) &&
                pcpd->FontInfo[nFont].Family == tmFamily &&
                pcpd->FontInfo[nFont].Weight == tm.tmWeight &&
                lstrcmp(pcpd->FontInfo[nFont].FaceName, ptszFace) == 0) {
            /*
             * Already have this font
             */
            DeleteObject(hFont);
            return FE_FONTOK;
        }


        if ((SizeToShow.Y < SizeShown.Y) ||
                (SizeToShow.Y == SizeShown.Y && SizeToShow.X < SizeShown.X)) {
            /*
             * This new font is smaller than nFont
             */
            break;
        }
    }

    /*
     * If we have to grow our font table, do it
     */
    if (pcpd->NumberOfFonts == pcpd->FontInfoLength) {
        PFONT_INFO Temp;

        pcpd->FontInfoLength += FONT_INCREMENT;
        Temp = (PFONT_INFO)LocalReAlloc(pcpd->FontInfo,
                                       SIZEOF(FONT_INFO) * pcpd->FontInfoLength, LMEM_MOVEABLE|LMEM_ZEROINIT);
        ASSERT(Temp);
        if (Temp == NULL) {
            pcpd->FontInfoLength -= FONT_INCREMENT;
            return FE_ABANDONFONT;  // no point enumerating more - no memory!
        }
        pcpd->FontInfo = Temp;
    }

    /*
     * The font we are adding should be inserted into the list,
     * if it is smaller than the last one.
     */
    if (nFont < (LONG)pcpd->NumberOfFonts) {
        MoveMemory( &pcpd->FontInfo[nFont+1],
                    &pcpd->FontInfo[nFont],
                    SIZEOF(FONT_INFO) * (pcpd->NumberOfFonts - nFont)
                   );
    }

    /*
     * Store the font info
     */
    pcpd->FontInfo[nFont].hFont = hFont;
    pcpd->FontInfo[nFont].Family = tmFamily;
    pcpd->FontInfo[nFont].Size = SizeActual;
    if (TM_IS_TT_FONT(tmFamily)) {
        pcpd->FontInfo[nFont].SizeWant = SizeWant;
    } else {
        pcpd->FontInfo[nFont].SizeWant.X = 0;
        pcpd->FontInfo[nFont].SizeWant.Y = 0;
    }
    pcpd->FontInfo[nFont].Weight = tm.tmWeight;
    pcpd->FontInfo[nFont].FaceName = pFN->atch;
    pcpd->FontInfo[nFont].tmCharSet = tm.tmCharSet;

    ++pcpd->NumberOfFonts;

    /*
     * If this is a true type font, create a bold version too.
     */
    if (nFontType == TRUETYPE_FONTTYPE && !IS_BOLD(pcpd->FontInfo[nFont].Weight)) {
          pelf->elfLogFont.lfWeight = FW_BOLD;
          goto CreateBoldFont;
    }

    return FE_FONTOK;  // and continue enumeration
}


VOID
InitializeFonts( LPCONSOLEPROP_DATA pcpd )
{
    EnumerateFonts( pcpd, EF_DEFFACE);  // Just the Default font
}


VOID
DestroyFonts( LPCONSOLEPROP_DATA pcpd )
{
    ULONG FontIndex;

    if (pcpd->FontInfo != NULL) {
        for (FontIndex = 0; FontIndex < pcpd->NumberOfFonts; FontIndex++) {
            DeleteObject(pcpd->FontInfo[FontIndex].hFont);
        }
        LocalFree(pcpd->FontInfo);
        pcpd->FontInfo = NULL;
        pcpd->NumberOfFonts = 0;
    }

    DestroyFaceNodes( pcpd );
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

    BOOL      bNegAC;

    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);

    // NTMW_STRUCTURE is different on 5.0+ platforms and the flag for 5.0+
    // platforms now lives in NEWTEXTMETRIC structure.

    if (osvi.dwMajorVersion <= 4)
    {
        bNegAC = !(((NTMW_INTERNAL *)pntm)->tmd.fl & TMD_NONNEGATIVE_AC);
    }
    else
    {
        bNegAC = !(pntm->ntmFlags & NTM_NONNEGATIVE_AC);
    }

    //
    // reject variable width and italic fonts, also tt fonts with neg ac
    //

    if
    (
      !(pelf->elfLogFont.lfPitchAndFamily & FIXED_PITCH) ||
      (pelf->elfLogFont.lfItalic)                        ||
      bNegAC
    )
    {
        if (!IsAvailableTTFont(pfed->pcpd,ptszFace))
            return pfed->bFindFaces ? FE_SKIPFONT : FE_ABANDONFONT;
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
        return pfed->bFindFaces ? FE_SKIPFONT : FE_ABANDONFONT;
    }

    /*
     * reject non-TT fonts that aren't OEM
     */
    if ((nFontType != TRUETYPE_FONTTYPE) &&
         (!IsFarEastCP(pfed->uDefCP) || !IS_ANY_DBCS_CHARSET(pelf->elfLogFont.lfCharSet)) &&
         (pelf->elfLogFont.lfCharSet != OEM_CHARSET)) {
        return pfed->bFindFaces ? FE_SKIPFONT : FE_ABANDONFONT;
    }

    /*
     * reject non-TT vertical/non-Terminal Font for FE
     */
    if (IsFarEastCP(pfed->uDefCP))
    {
        if ((nFontType != TRUETYPE_FONTTYPE) &&
            ((ptszFace[0] == TEXT('@')) ||
             (lstrcmp(ptszFace, TEXT("Terminal")) != 0)))
        {
            return pfed->bFindFaces ? FE_SKIPFONT : FE_ABANDONFONT;
        }
    }

    /*
     * reject Far East TT fonts that aren't Far East charset.
     */
    if (IsAvailableTTFont(pfed->pcpd, ptszFace) &&
        !IS_ANY_DBCS_CHARSET(pelf->elfLogFont.lfCharSet) &&
        !IsAvailableTTFontCP(pfed->pcpd, ptszFace,0)
       ) {
        return FE_SKIPFONT;    // should be enumerate next charset.
    }

    /*
     * Add or find the facename
     */
    pFN = AddFaceNode(&pfed->pcpd->gpFaceNames, ptszFace);
    if (pFN == NULL) {
        return FE_ABANDONFONT;
    }

    if (pfed->bFindFaces) {
        DWORD dwFontType = 0;
        if (nFontType == TRUETYPE_FONTTYPE) {
            dwFontType = EF_TTFONT;
        } else if (nFontType == RASTER_FONTTYPE) {
            dwFontType = EF_OEMFONT;
        }
        pFN->dwFlag |= dwFontType | EF_NEW;
        
        if (IS_ANY_DBCS_CHARSET(pelf->elfLogFont.lfCharSet))
            pFN->dwFlag |= EF_DBCSFONT;
            
        return FE_SKIPFONT;
    }


    if (IS_BOLD(pelf->elfLogFont.lfWeight)) {
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
            pfed->ulFE |= AddFont(pfed->pcpd, pelf, pntm, nFontType, pfed->hDC, pFN);
            if (pfed->ulFE & FE_ABANDONFONT) {
                return FE_ABANDONFONT;
            }
        }
    } else {
            pfed->ulFE |= AddFont(pfed->pcpd, pelf, pntm, nFontType, pfed->hDC, pFN);
            if (pfed->ulFE & FE_ABANDONFONT) {
                return FE_ABANDONFONT;
            }
    }

    return FE_FONTOK;  // and continue enumeration
}

BOOL
DoFontEnum(
    LPCONSOLEPROP_DATA pcpd,
    HDC hDC,
    LPTSTR ptszFace,
    PSHORT pTTPoints,
    UINT nTTPoints)
{
    BOOL bDeleteDC = FALSE;
    FONTENUMDATA fed;
    LOGFONT LogFont;

    if (hDC == NULL) {
        hDC = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
        bDeleteDC = TRUE;
    }

    fed.pcpd = pcpd;
    fed.hDC = hDC;
    fed.bFindFaces = (ptszFace == NULL);
    fed.ulFE = 0;
    fed.pTTPoints = pTTPoints;
    fed.nTTPoints = nTTPoints;
    fed.uDefCP = pcpd->uOEMCP;
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
RemoveFace(LPCONSOLEPROP_DATA pcpd, LPTSTR ptszFace)
{
    DWORD i;
    int nToRemove = 0;

    //
    // Delete & Remove fonts with Face Name == ptszFace
    //
    for (i = 0; i < pcpd->NumberOfFonts; i++) {
        if (lstrcmp(pcpd->FontInfo[i].FaceName, ptszFace) == 0) {
            BOOL bDeleted = DeleteObject(pcpd->FontInfo[i].hFont);
            pcpd->FontInfo[i].hFont = NULL;
            nToRemove++;
        } else if (nToRemove > 0) {
            /*
             * Shuffle from FontInfo[i] down nToRemove slots.
             */
            MoveMemory( &pcpd->FontInfo[i - nToRemove],
                        &pcpd->FontInfo[i],
                        SIZEOF(FONT_INFO)*(pcpd->NumberOfFonts - i)
                       );
            pcpd->NumberOfFonts -= nToRemove;
            i -= nToRemove;
            nToRemove = 0;
        }
    }
    pcpd->NumberOfFonts -= nToRemove;
}


NTSTATUS
EnumerateFonts(
    LPCONSOLEPROP_DATA pcpd,
    DWORD Flags)
{
    TEXTMETRIC tm;
    HDC hDC;
    PFACENODE pFN;
    ULONG ulOldEnumFilter;
    BOOL  bEnumOEMFace = TRUE;
    DWORD FontIndex;
    DWORD dwFontType = 0;


    dwFontType = (EF_TTFONT|EF_OEMFONT|EF_DEFFACE) & Flags;

    if (pcpd->FontInfo == NULL) {
        //
        // allocate memory for the font array
        //
        pcpd->NumberOfFonts = 0;

        pcpd->FontInfo = (PFONT_INFO)LocalAlloc(LPTR, SIZEOF(FONT_INFO) * INITIAL_FONTS);
        if (pcpd->FontInfo == NULL)
            return STATUS_NO_MEMORY;
        pcpd->FontInfoLength = INITIAL_FONTS;
    }

    hDC = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);

    // Before enumeration, turn off font enumeration filters.
    ulOldEnumFilter = SetFontEnumeration(FE_FILTER_NONE);

    if (Flags & EF_DEFFACE) {
        SelectObject(hDC, GetStockObject(OEM_FIXED_FONT));
        GetTextMetrics(hDC, &tm);
        GetTextFace(hDC, LF_FACESIZE, pcpd->DefaultFaceName);

        pcpd->DefaultFontSize.X = (SHORT)(tm.tmMaxCharWidth);
        pcpd->DefaultFontSize.Y = (SHORT)(tm.tmHeight+tm.tmExternalLeading);
        pcpd->DefaultFontFamily = tm.tmPitchAndFamily;
        
        if (IS_ANY_DBCS_CHARSET(tm.tmCharSet))
            pcpd->DefaultFontSize.X /= 2;

        // Make sure we are going to enumerate the OEM face.
        pFN = AddFaceNode(&pcpd->gpFaceNames, pcpd->DefaultFaceName);
        pFN->dwFlag |= EF_DEFFACE | EF_OEMFONT;
    }

    if (pcpd->gbEnumerateFaces) {
        /*
         * Set the EF_OLD bit and clear the EF_NEW bit
         * for all previously available faces
         */
        for (pFN = pcpd->gpFaceNames; pFN; pFN = pFN->pNext) {
            pFN->dwFlag |= EF_OLD;
            pFN->dwFlag &= ~EF_NEW;
        }

        //
        // Use DoFontEnum to get the names of all the suitable Faces
        // All facenames found will be put in gpFaceNames with
        // the EF_NEW bit set.
        //
        DoFontEnum(pcpd, hDC, NULL, TTPoints, 1);
        pcpd->gbEnumerateFaces = FALSE;
    }

    // Use DoFontEnum to get all fonts from the system.  Our FontEnum
    // proc puts just the ones we want into an array
    //
    for (pFN = pcpd->gpFaceNames; pFN; pFN = pFN->pNext) {

        if ((pFN->dwFlag & (EF_OLD|EF_NEW)) == EF_OLD) {
            // The face is no longer available
            RemoveFace(pcpd, pFN->atch);
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
            if (IsFarEastCP(pcpd->uOEMCP) && !IsAvailableTTFontCP(pcpd, pFN->atch, 0))
                DoFontEnum(pcpd, hDC, pFN->atch, TTPointsDbcs, NELEM(TTPointsDbcs));
            else
                DoFontEnum(pcpd, hDC, pFN->atch, TTPoints, NELEM(TTPoints));
        } else {
            DoFontEnum(pcpd, hDC, pFN->atch, NULL, 0);

            // If we find that the face just enumerated is the same as OEM,
            // reset flag so we don't try to enumerate it again.

            if (lstrcmpi(pFN->atch, pcpd->DefaultFaceName) == 0)
            {
                bEnumOEMFace = FALSE;
            }
        }
        pFN->dwFlag |= EF_ENUMERATED;
    }


    // After enumerating fonts, restore the font enumeration filter.
    SetFontEnumeration(ulOldEnumFilter);

    DeleteDC(hDC);

    for (FontIndex = 0; FontIndex < pcpd->NumberOfFonts; FontIndex++) {
        if (pcpd->FontInfo[FontIndex].Size.X == pcpd->DefaultFontSize.X &&
            pcpd->FontInfo[FontIndex].Size.Y == pcpd->DefaultFontSize.Y &&
            pcpd->FontInfo[FontIndex].Family == pcpd->DefaultFontFamily) {
            break;
        }
    }
    ASSERT(FontIndex < pcpd->NumberOfFonts);
    if (FontIndex < pcpd->NumberOfFonts) {
        pcpd->DefaultFontIndex = FontIndex;
    } else {
        pcpd->DefaultFontIndex = 0;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
GetNumFonts(
    LPCONSOLEPROP_DATA pcpd,
    OUT PULONG NumFonts
    )
{
    *NumFonts = pcpd->NumberOfFonts;
    return STATUS_SUCCESS;
}


NTSTATUS
GetFontSize(
    LPCONSOLEPROP_DATA pcpd,
    IN DWORD  FontIndex,
    OUT PCOORD FontSize
    )
{
    if (FontIndex >= pcpd->NumberOfFonts)
        return STATUS_INVALID_PARAMETER;
    *FontSize = pcpd->FontInfo[FontIndex].Size;
    return STATUS_SUCCESS;
}

/*
 * Get the font index for a new font
 * If necessary, attempt to create the font.
 * Always return a valid FontIndex (even if not correct)
 * Family:   Find/Create a font with of this Family
 *           0    - don't care
 * ptszFace: Find/Create a font with this face name.
 *           NULL or TEXT("")  - use DefaultFaceName
 * Size:     Must match SizeWant or actual Size.
 */
int
FindCreateFont(
    LPCONSOLEPROP_DATA pcpd,
    DWORD Family,
    LPTSTR ptszFace,
    COORD Size,
    LONG Weight)
{
#define NOT_CREATED_NOR_FOUND -1
#define CREATED_BUT_NOT_FOUND -2

    int i;
    int FontIndex = NOT_CREATED_NOR_FOUND;
    BOOL bFontOK;
    TCHAR AltFaceName[LF_FACESIZE];
    COORD AltFontSize;
    BYTE  AltFontFamily;
    ULONG AltFontIndex = 0;
    LPTSTR ptszAltFace = NULL;
    UINT  uCurrentCP = pcpd->lpFEConsole->uCodePage;
    UINT  uDefaultCP = pcpd->uOEMCP;
    
    BYTE CharSet = CodePageToCharSet(uCurrentCP);

    if (!IsFarEastCP(uDefaultCP) || IS_ANY_DBCS_CHARSET(CharSet))
    {
        if (ptszFace == NULL || *ptszFace == TEXT('\0')) {
            ptszFace = pcpd->DefaultFaceName;
        }
        if (Size.Y == 0) {
            Size = pcpd->DefaultFontSize;
        }
    }
    else 
    {
        MakeAltRasterFont(pcpd, uCurrentCP, &AltFontSize, &AltFontFamily, &AltFontIndex, AltFaceName);

        if (ptszFace == NULL || *ptszFace == L'\0') {
            ptszFace = AltFaceName;
        }
        if (Size.Y == 0) {
            Size.X = AltFontSize.X;
            Size.Y = AltFontSize.Y;
        }
    }

    if (IsAvailableTTFont(pcpd, ptszFace)) {
        ptszAltFace = GetAltFaceName(pcpd, ptszFace);
    }
    else {
        ptszAltFace = ptszFace;
    }

    /*
     * Try to find the exact font
     */
TryFindExactFont:
    for (i=0; i < (int)pcpd->NumberOfFonts; i++) {
        /*
         * If looking for a particular Family, skip non-matches
         */
        if ((Family != 0) &&
                ((BYTE)Family != pcpd->FontInfo[i].Family)) {
            continue;
        }

        /*
         * Skip non-matching sizes
         */
        if ((!SIZE_EQUAL(pcpd->FontInfo[i].SizeWant, Size) &&
             !SIZE_EQUAL(pcpd->FontInfo[i].Size, Size))) {
            continue;
        }

        /*
         * Skip non-matching weights
         */
        if ((Weight != 0) && (Weight != pcpd->FontInfo[i].Weight)) {
            continue;
        }

        /*
         * Skip fonts that have unmatched charset
         */
        if (!TM_IS_TT_FONT(pcpd->FontInfo[i].Family) &&
                pcpd->FontInfo[i].tmCharSet != CharSet) {
            continue;
        }
        
        /*
         * Size (and maybe Family) match.
         *  If we don't care about the name, or if it matches, use this font.
         *  Else if name doesn't match and it is a raster font, consider it.
         */
        if ((ptszFace == NULL) || (ptszFace[0] == TEXT('\0')) ||
                (lstrcmp(pcpd->FontInfo[i].FaceName, ptszFace) == 0) ||
                (lstrcmp(pcpd->FontInfo[i].FaceName, ptszAltFace) == 0) ) {
            FontIndex = i;
            goto FoundFont;
        } else if (!TM_IS_TT_FONT(pcpd->FontInfo[i].Family)) {
            FontIndex = i;
        }
    }

    if (FontIndex == NOT_CREATED_NOR_FOUND) {
        /*
         * Didn't find the exact font, so try to create it
         */
        ULONG ulOldEnumFilter;
        ulOldEnumFilter = SetFontEnumeration(FE_FILTER_NONE);
        if (Size.Y < 0) {
            Size.Y = -Size.Y;
        }
        bFontOK = DoFontEnum(pcpd, NULL, ptszFace, &Size.Y, 1);
        SetFontEnumeration(ulOldEnumFilter);
        if (bFontOK) {
            FontIndex = CREATED_BUT_NOT_FOUND;
            goto TryFindExactFont;
        } else {
        }
    } else if (FontIndex >= 0) {
        // a close Raster Font fit - only the name doesn't match.
        goto FoundFont;
    }

    /*
     * Failed to find exact match, even after enumeration, so now try
     * to find a font of same family and same size or bigger
     */
    for (i=0; i < (int)pcpd->NumberOfFonts; i++) {
    
        if ((Family != 0) &&
                ((BYTE)Family != pcpd->FontInfo[i].Family)) {
            continue;
        }

        if (!TM_IS_TT_FONT(pcpd->FontInfo[i].Family) &&
            pcpd->FontInfo[i].tmCharSet != CharSet) {
            continue;
        }

        if (pcpd->FontInfo[i].Size.Y >= Size.Y &&
                pcpd->FontInfo[i].Size.X >= Size.X) {
            // Same family, size >= desired.
            FontIndex = i;
            break;
        }
    }

    if (FontIndex < 0) {
        if (uCurrentCP == uDefaultCP) 
        {
            FontIndex = pcpd->DefaultFontIndex;
        }
        else 
        {
            FontIndex = AltFontIndex;
        }
    }

FoundFont:
    return FontIndex;

#undef NOT_CREATED_NOR_FOUND
#undef CREATED_BUT_NOT_FOUND
}


LPTSTR
TranslateConsoleTitle(
    LPTSTR ConsoleTitle
    )
/*++

    this routine translates path characters into '_' characters because
    the NT registry apis do not allow the creation of keys with
    names that contain path characters.  it allocates a buffer that
    must be freed.

--*/
{
    int ConsoleTitleLength, i;
    LPTSTR TranslatedTitle;

    ConsoleTitleLength = lstrlen(ConsoleTitle) + 1;
    TranslatedTitle = LocalAlloc(LPTR,
                                ConsoleTitleLength * SIZEOF(TCHAR));
    if (TranslatedTitle == NULL) {
        return NULL;
    }
    for (i = 0; i < ConsoleTitleLength; i++) {
        if (ConsoleTitle[i] == TEXT('\\')) {
            TranslatedTitle[i] = TEXT('_');
        } else {
            TranslatedTitle[i] = ConsoleTitle[i];
        }
    }
    return TranslatedTitle;
}



void
InitRegistryValues( LPCONSOLEPROP_DATA pcpd )

/*++

Routine Description:

    This routine allocates a state info structure and fill it in with
    default values.  It then tries to load the default settings for
    console from the registry.

Arguments:

    none

Return Value:

    pStateInfo - pointer to structure to receive information

--*/

{
    TCHAR chSave;


    pcpd->lpConsole->wFillAttribute = 0x07;            // white on black
    pcpd->lpConsole->wPopupFillAttribute = 0xf5;      // purple on white
    pcpd->lpConsole->bInsertMode = FALSE;
    pcpd->lpConsole->bQuickEdit = FALSE;
    pcpd->lpConsole->bFullScreen = FALSE;
    pcpd->lpConsole->dwScreenBufferSize.X = 80;
    pcpd->lpConsole->dwScreenBufferSize.Y = 25;
    pcpd->lpConsole->dwWindowSize.X = 80;
    pcpd->lpConsole->dwWindowSize.Y = 25;
    pcpd->lpConsole->dwWindowOrigin.X = 0;
    pcpd->lpConsole->dwWindowOrigin.Y = 0;
    pcpd->lpConsole->bAutoPosition = TRUE;
    pcpd->lpConsole->dwFontSize.X = 0;
    pcpd->lpConsole->dwFontSize.Y = 0;
    pcpd->lpConsole->uFontFamily = 0;
    pcpd->lpConsole->uFontWeight = 0;
#ifdef UNICODE
    FillMemory( pcpd->lpConsole->FaceName, SIZEOF(pcpd->lpConsole->FaceName), 0 );
    pcpd->lpFaceName = (LPTSTR)pcpd->lpConsole->FaceName;
#else
    FillMemory( pcpd->szFaceName, SIZEOF(pcpd->szFaceName), 0 );
    pcpd->lpFaceName = pcpd->szFaceName;
#endif
    pcpd->lpConsole->uCursorSize = 25;
    pcpd->lpConsole->uHistoryBufferSize = 25;
    pcpd->lpConsole->uNumberOfHistoryBuffers = 4;
    pcpd->lpConsole->bHistoryNoDup = 0;
    pcpd->lpConsole->ColorTable[ 0] = RGB(0,   0,   0   );
    pcpd->lpConsole->ColorTable[ 1] = RGB(0,   0,   0x80);
    pcpd->lpConsole->ColorTable[ 2] = RGB(0,   0x80,0   );
    pcpd->lpConsole->ColorTable[ 3] = RGB(0,   0x80,0x80);
    pcpd->lpConsole->ColorTable[ 4] = RGB(0x80,0,   0   );
    pcpd->lpConsole->ColorTable[ 5] = RGB(0x80,0,   0x80);
    pcpd->lpConsole->ColorTable[ 6] = RGB(0x80,0x80,0   );
    pcpd->lpConsole->ColorTable[ 7] = RGB(0xC0,0xC0,0xC0);
    pcpd->lpConsole->ColorTable[ 8] = RGB(0x80,0x80,0x80);
    pcpd->lpConsole->ColorTable[ 9] = RGB(0,   0,   0xFF);
    pcpd->lpConsole->ColorTable[10] = RGB(0,   0xFF,0   );
    pcpd->lpConsole->ColorTable[11] = RGB(0,   0xFF,0xFF);
    pcpd->lpConsole->ColorTable[12] = RGB(0xFF,0,   0   );
    pcpd->lpConsole->ColorTable[13] = RGB(0xFF,0,   0xFF);
    pcpd->lpConsole->ColorTable[14] = RGB(0xFF,0xFF,0   );
    pcpd->lpConsole->ColorTable[15] = RGB(0xFF,0xFF,0xFF);
    pcpd->lpFEConsole->uCodePage    = pcpd->uOEMCP;
    
    // make console title NULL so we load the default settings for the console
    chSave = pcpd->ConsoleTitle[0];
    pcpd->ConsoleTitle[0] = TEXT('\0');
    GetRegistryValues( pcpd );

    // restore the console title
    pcpd->ConsoleTitle[0] = chSave;

}


VOID
GetTitleFromLinkName(
    LPTSTR szLinkName,
    LPTSTR szTitle
    )
{
    DWORD dwLen;
    LPTSTR pLnk, pDot;
    LPTSTR pPath = szLinkName;

    // Error checking
    if (!szTitle)
        return;

    if (!szLinkName)
    {
        szTitle[0] = TEXT('\0');
        return;
    }


    // find filename at end of fully qualified link name and point pLnk to it
    for (pLnk = pPath; *pPath; pPath++)
    {
        if ( (pPath[0] == TEXT('\\') || pPath[0] == TEXT(':')) &&
              pPath[1] &&
             (pPath[1] != TEXT('\\'))
            )
            pLnk = pPath + 1;
    }

    // find extension (.lnk)
    pPath = pLnk;
    for (pDot = NULL; *pPath; pPath++)
    {
        switch (*pPath) {
        case TEXT('.'):
            pDot = pPath;       // remember the last dot
            break;
        case TEXT('\\'):
        case TEXT(' '):              // extensions can't have spaces
            pDot = NULL;        // forget last dot, it was in a directory
            break;
        }
    }

    // if we found the extension, pDot points to it, if not, pDot
    // is NULL.

    if (pDot)
    {
        dwLen = min( (DWORD) (pDot - pLnk), (MAX_TITLE_LEN-1) );
    }
    else
    {
        dwLen = min( lstrlen(pLnk), (MAX_TITLE_LEN-1) );
    }

    CopyMemory(szTitle, pLnk, dwLen*SIZEOF(TCHAR));
    szTitle[ dwLen ] = TEXT('\0');

}



VOID
GetRegistryValues(
    LPCONSOLEPROP_DATA pcpd
    )

/*++

Routine Description:

    This routine reads in values from the registry and places them
    in the supplied structure.

Arguments:

    pStateInfo - optional pointer to structure to receive information

Return Value:

    current page number

--*/

{
    HKEY hCurrentUserKey;
    HKEY hConsoleKey;
    HKEY hTitleKey;
    LPTSTR TranslatedTitle;
    DWORD dwValue, dwSize;
    DWORD dwRet = 0;
    DWORD i;
    WCHAR awchBuffer[LF_FACESIZE];

    //
    // Open the current user registry key
    //

    if (RegOpenKey( HKEY_CURRENT_USER, NULL, &hCurrentUserKey)!=ERROR_SUCCESS)
        return;

    //
    // Open the console registry key
    //

    if (RegOpenKey(hCurrentUserKey,CONSOLE_REGISTRY_STRING,&hConsoleKey)!=ERROR_SUCCESS)
    {
        RegCloseKey(hCurrentUserKey);
        return;
    }

    //
    // If there is no structure to fill out, just bail out
    //

    if ((!pcpd) || (!pcpd->lpConsole))
        goto CloseKeys;

    //
    // Open the console title subkey, if there is one
    //

    if (pcpd->ConsoleTitle[0] != TEXT('\0'))
    {
        TranslatedTitle = TranslateConsoleTitle(pcpd->ConsoleTitle);
        if (TranslatedTitle == NULL)
            goto GetDefaultConsole;
        dwValue = RegOpenKey( hConsoleKey,
                              TranslatedTitle,
                              &hTitleKey);
        LocalFree(TranslatedTitle);
        if (dwValue!=ERROR_SUCCESS)
            goto GetDefaultConsole;
    } else {

GetDefaultConsole:
        hTitleKey = hConsoleKey;
    }

    //
    // Initial screen fill
    //

    dwSize = SIZEOF(dwValue);
    if (SHQueryValueEx( hTitleKey,
                         CONSOLE_REGISTRY_FILLATTR,
                         NULL,
                         NULL,
                         (LPBYTE)&dwValue,
                         &dwSize
                        ) == ERROR_SUCCESS)
    {
        pcpd->lpConsole->wFillAttribute = (WORD)dwValue;
    }

    //
    // Initial popup fill
    //

    dwSize = SIZEOF(dwValue);
    if (SHQueryValueEx( hTitleKey,
                         CONSOLE_REGISTRY_POPUPATTR,
                         NULL,
                         NULL,
                         (LPBYTE)&dwValue,
                         &dwSize
                        ) == ERROR_SUCCESS)
    {
        pcpd->lpConsole->wPopupFillAttribute = (WORD)dwValue;
    }

    //
    // Initial color table
    //

    for (i = 0; i < 16; i++)
    {
        wsprintf((LPTSTR)awchBuffer, CONSOLE_REGISTRY_COLORTABLE, i);
        dwSize = SIZEOF(dwValue);
        if (SHQueryValueEx( hTitleKey,
                             (LPTSTR)awchBuffer,
                             NULL,
                             NULL,
                             (LPBYTE)&dwValue,
                             &dwSize
                            ) == ERROR_SUCCESS)
        {
            pcpd->lpConsole->ColorTable[i] = dwValue;
        }
    }

    //
    // Initial insert mode
    //

    dwSize = SIZEOF(dwValue);
    if (SHQueryValueEx( hTitleKey,
                         CONSOLE_REGISTRY_INSERTMODE,
                         NULL,
                         NULL,
                         (LPBYTE)&dwValue,
                         &dwSize
                        ) == ERROR_SUCCESS)
    {
        pcpd->lpConsole->bInsertMode = !!dwValue;
    }

    //
    // Initial quick edit mode
    //

    dwSize = SIZEOF(dwValue);
    if (SHQueryValueEx( hTitleKey,
                         CONSOLE_REGISTRY_QUICKEDIT,
                         NULL,
                         NULL,
                         (LPBYTE)&dwValue,
                         &dwSize
                        ) == ERROR_SUCCESS)
    {
        pcpd->lpConsole->bQuickEdit = !!dwValue;
    }

#ifdef i386
    //
    // Initial full screen mode
    //

    dwSize = SIZEOF(dwValue);
    if (SHQueryValueEx( hTitleKey,
                         CONSOLE_REGISTRY_FULLSCR,
                         NULL,
                         NULL,
                         (LPBYTE)&dwValue,
                         &dwSize
                        ) == ERROR_SUCCESS)
    {
        pcpd->lpConsole->bFullScreen = !!dwValue;
    }
#endif

    //
    // Initial code page
    //

    dwSize = SIZEOF(dwValue);
    if (SHQueryValueEx( hTitleKey,
                         CONSOLE_REGISTRY_CODEPAGE,
                         NULL,
                         NULL,
                         (LPBYTE)&dwValue,
                         &dwSize
                        ) == ERROR_SUCCESS)
    {
        pcpd->lpFEConsole->uCodePage = (UINT)dwValue;
    }
    
    //
    // Initial screen buffer size
    //

    dwSize = SIZEOF(dwValue);
    if (SHQueryValueEx( hTitleKey,
                         CONSOLE_REGISTRY_BUFFERSIZE,
                         NULL,
                         NULL,
                         (LPBYTE)&dwValue,
                         &dwSize
                        ) == ERROR_SUCCESS)
    {
        pcpd->lpConsole->dwScreenBufferSize.X = LOWORD(dwValue);
        pcpd->lpConsole->dwScreenBufferSize.Y = HIWORD(dwValue);
    }

    //
    // Initial window size
    //

    dwSize = SIZEOF(dwValue);
    if (SHQueryValueEx( hTitleKey,
                         CONSOLE_REGISTRY_WINDOWSIZE,
                         NULL,
                         NULL,
                         (LPBYTE)&dwValue,
                         &dwSize
                        ) == ERROR_SUCCESS)
    {
        pcpd->lpConsole->dwWindowSize.X = LOWORD(dwValue);
        pcpd->lpConsole->dwWindowSize.Y = HIWORD(dwValue);
    }

    //
    // Initial window position
    //

    dwSize = SIZEOF(dwValue);
    if (SHQueryValueEx( hTitleKey,
                         CONSOLE_REGISTRY_WINDOWPOS,
                         NULL,
                         NULL,
                         (LPBYTE)&dwValue,
                         &dwSize
                        ) == ERROR_SUCCESS)
    {
        pcpd->lpConsole->dwWindowOrigin.X = (SHORT)LOWORD(dwValue);
        pcpd->lpConsole->dwWindowOrigin.Y = (SHORT)HIWORD(dwValue);
        pcpd->lpConsole->bAutoPosition = FALSE;
    }

    //
    // Initial font size
    //

    dwSize = SIZEOF(dwValue);
    if (SHQueryValueEx( hTitleKey,
                         CONSOLE_REGISTRY_FONTSIZE,
                         NULL,
                         NULL,
                         (LPBYTE)&dwValue,
                         &dwSize
                        ) == ERROR_SUCCESS)
    {
        pcpd->lpConsole->dwFontSize.X = LOWORD(dwValue);
        pcpd->lpConsole->dwFontSize.Y = HIWORD(dwValue);
    }

    //
    // Initial font family
    //

    dwSize = SIZEOF(dwValue);
    if (SHQueryValueEx( hTitleKey,
                         CONSOLE_REGISTRY_FONTFAMILY,
                         NULL,
                         NULL,
                         (LPBYTE)&dwValue,
                         &dwSize
                        ) == ERROR_SUCCESS)
    {
        pcpd->lpConsole->uFontFamily = dwValue;
    }

    //
    // Initial font weight
    //

    dwSize = SIZEOF(dwValue);
    if (SHQueryValueEx( hTitleKey,
                         CONSOLE_REGISTRY_FONTWEIGHT,
                         NULL,
                         NULL,
                         (LPBYTE)&dwValue,
                         &dwSize
                        ) == ERROR_SUCCESS)
    {
        pcpd->lpConsole->uFontWeight = dwValue;
    }

    //
    // Initial font face name
    //

    dwSize = SIZEOF(awchBuffer);
    if (SHQueryValueEx( hTitleKey,
                         CONSOLE_REGISTRY_FACENAME,
                         NULL,
                         NULL,
                         (LPBYTE)awchBuffer,
                         &dwSize
                        ) == ERROR_SUCCESS)
    {
        CopyMemory((LPBYTE)pcpd->lpFaceName, (LPBYTE)awchBuffer, LF_FACESIZE*SIZEOF(TCHAR));
    }

    //
    // Initial cursor size
    //

    dwSize = SIZEOF(dwValue);
    if (SHQueryValueEx( hTitleKey,
                         CONSOLE_REGISTRY_CURSORSIZE,
                         NULL,
                         NULL,
                         (LPBYTE)&dwValue,
                         &dwSize
                        ) == ERROR_SUCCESS)
    {
        pcpd->lpConsole->uCursorSize = dwValue;
    }

    //
    // Initial history buffer size
    //

    dwSize = SIZEOF(dwValue);
    if (SHQueryValueEx( hTitleKey,
                         CONSOLE_REGISTRY_HISTORYSIZE,
                         NULL,
                         NULL,
                         (LPBYTE)&dwValue,
                         &dwSize
                        ) == ERROR_SUCCESS)
    {
        pcpd->lpConsole->uHistoryBufferSize = dwValue;
    }

    //
    // Initial number of history buffers
    //

    dwSize = SIZEOF(dwValue);
    if (SHQueryValueEx( hTitleKey,
                         CONSOLE_REGISTRY_HISTORYBUFS,
                         NULL,
                         NULL,
                         (LPBYTE)&dwValue,
                         &dwSize
                        ) == ERROR_SUCCESS)
    {
        pcpd->lpConsole->uNumberOfHistoryBuffers = dwValue;
    }

    //
    // Initial history duplication mode
    //

    dwSize = SIZEOF(dwValue);
    if (SHQueryValueEx( hTitleKey,
                         CONSOLE_REGISTRY_HISTORYNODUP,
                         NULL,
                         NULL,
                         (LPBYTE)&dwValue,
                         &dwSize
                        ) == ERROR_SUCCESS)
    {
        pcpd->lpConsole->bHistoryNoDup = dwValue;
    }

    //
    // Close the registry keys
    //

    if (hTitleKey != hConsoleKey) {
        RegCloseKey(hTitleKey);
    }

CloseKeys:
    RegCloseKey(hConsoleKey);
    RegCloseKey(hCurrentUserKey);

}


VOID
SetRegistryValues(
    LPCONSOLEPROP_DATA pcpd
    )

/*++

Routine Description:

    This routine writes values to the registry from the supplied
    structure.

Arguments:

    pStateInfo - optional pointer to structure containing information
    dwPage     - current page number

Return Value:

    none

--*/

{
    HKEY hCurrentUserKey;
    HKEY hConsoleKey;
    HKEY hTitleKey;
    LPTSTR TranslatedTitle;
    DWORD dwValue;
    DWORD i;
    WCHAR awchBuffer[LF_FACESIZE];

    //
    // Open the current user registry key
    //

    if (RegOpenKey( HKEY_CURRENT_USER, NULL, &hCurrentUserKey )!=ERROR_SUCCESS)
    {
        return;
    }

    //
    // Open the console registry key
    //

    if (RegCreateKey( hCurrentUserKey, CONSOLE_REGISTRY_STRING, &hConsoleKey )!=ERROR_SUCCESS)
    {
        RegCloseKey(hCurrentUserKey);
        return;
    }

    //
    // If we only want to save the current page, bail out
    //

    if (pcpd == NULL)
    {
        goto CloseKeys;
    }

    //
    // Open the console title subkey, if there is one
    //

    if (pcpd->ConsoleTitle[0] != TEXT('\0'))
    {
        TranslatedTitle = TranslateConsoleTitle(pcpd->ConsoleTitle);
        if (TranslatedTitle == NULL)
        {
            RegCloseKey(hConsoleKey);
            RegCloseKey(hCurrentUserKey);
            return;
        }
        dwValue = RegCreateKey( hConsoleKey,
                                TranslatedTitle,
                                &hTitleKey);
        LocalFree(TranslatedTitle);
        if (dwValue!=ERROR_SUCCESS)
        {
            RegCloseKey(hConsoleKey);
            RegCloseKey(hCurrentUserKey);
            return;
        }
    } else {
        hTitleKey = hConsoleKey;
    }

    //
    // Save screen and popup colors and color table
    //

    dwValue = pcpd->lpConsole->wFillAttribute;
    RegSetValueEx( hTitleKey,
                   CONSOLE_REGISTRY_FILLATTR,
                   0,
                   REG_DWORD,
                   (LPBYTE)&dwValue,
                   SIZEOF(dwValue)
                  );
    dwValue = pcpd->lpConsole->wPopupFillAttribute;
    RegSetValueEx( hTitleKey,
                   CONSOLE_REGISTRY_POPUPATTR,
                   0,
                   REG_DWORD,
                   (LPBYTE)&dwValue,
                   SIZEOF(dwValue)
                  );
    for (i = 0; i < 16; i++)
    {
        dwValue = pcpd->lpConsole->ColorTable[i];
        wsprintf((LPTSTR)awchBuffer, CONSOLE_REGISTRY_COLORTABLE, i);
        RegSetValueEx( hTitleKey,
                       (LPTSTR)awchBuffer,
                       0,
                       REG_DWORD,
                       (LPBYTE)&dwValue,
                       SIZEOF(dwValue)
                      );
    }

    //
    // Save insert, quickedit, and fullscreen mode settings
    //

    dwValue = pcpd->lpConsole->bInsertMode;
    RegSetValueEx( hTitleKey,
                   CONSOLE_REGISTRY_INSERTMODE,
                   0,
                   REG_DWORD,
                   (LPBYTE)&dwValue,
                   SIZEOF(dwValue)
                  );
    dwValue = pcpd->lpConsole->bQuickEdit;
    RegSetValueEx( hTitleKey,
                   CONSOLE_REGISTRY_QUICKEDIT,
                   0,
                   REG_DWORD,
                   (LPBYTE)&dwValue,
                   SIZEOF(dwValue)
                  );
#ifdef i386
    dwValue = pcpd->lpConsole->bFullScreen;
    RegSetValueEx( hTitleKey,
                   CONSOLE_REGISTRY_FULLSCR,
                   0,
                   REG_DWORD,
                   (LPBYTE)&dwValue,
                   SIZEOF(dwValue)
                  );
#endif

    //
    // Save screen buffer size
    //

    dwValue = MAKELONG(pcpd->lpConsole->dwScreenBufferSize.X,
                       pcpd->lpConsole->dwScreenBufferSize.Y);
    RegSetValueEx( hTitleKey,
                   CONSOLE_REGISTRY_BUFFERSIZE,
                   0,
                   REG_DWORD,
                   (LPBYTE)&dwValue,
                   SIZEOF(dwValue)
                  );

    //
    // Save window size
    //

    dwValue = MAKELONG(pcpd->lpConsole->dwWindowSize.X,
                       pcpd->lpConsole->dwWindowSize.Y);
    RegSetValueEx( hTitleKey,
                   CONSOLE_REGISTRY_WINDOWSIZE,
                   0,
                   REG_DWORD,
                   (LPBYTE)&dwValue,
                   SIZEOF(dwValue)
                  );

    //
    // Save window position
    //

    if (pcpd->lpConsole->bAutoPosition) {
        RegDeleteKey(hTitleKey, CONSOLE_REGISTRY_WINDOWPOS);
    } else {
        dwValue = MAKELONG(pcpd->lpConsole->dwWindowOrigin.X,
                           pcpd->lpConsole->dwWindowOrigin.Y);
        RegSetValueEx( hTitleKey,
                       CONSOLE_REGISTRY_WINDOWPOS,
                       0,
                       REG_DWORD,
                       (LPBYTE)&dwValue,
                       SIZEOF(dwValue)
                      );
    }

    //
    // Save font size, family, weight, and face name
    //

    dwValue = MAKELONG(pcpd->lpConsole->dwFontSize.X,
                       pcpd->lpConsole->dwFontSize.Y);
    RegSetValueEx( hTitleKey,
                   CONSOLE_REGISTRY_FONTSIZE,
                   0,
                   REG_DWORD,
                   (LPBYTE)&dwValue,
                   SIZEOF(dwValue)
                  );
    dwValue = pcpd->lpConsole->uFontFamily;
    RegSetValueEx( hTitleKey,
                   CONSOLE_REGISTRY_FONTFAMILY,
                   0,
                   REG_DWORD,
                   (LPBYTE)&dwValue,
                   SIZEOF(dwValue)
                  );
    dwValue = pcpd->lpConsole->uFontWeight;
    RegSetValueEx( hTitleKey,
                   CONSOLE_REGISTRY_FONTWEIGHT,
                   0,
                   REG_DWORD,
                   (LPBYTE)&dwValue,
                   SIZEOF(dwValue)
                  );
    RegSetValueEx( hTitleKey,
                   CONSOLE_REGISTRY_FACENAME,
                   0,
                   REG_SZ,
                   (LPBYTE)pcpd->lpFaceName,
                   (lstrlen(pcpd->lpFaceName) + 1) * SIZEOF(TCHAR)
                 );

    //
    // Save cursor size
    //

    dwValue = pcpd->lpConsole->uCursorSize;
    RegSetValueEx( hTitleKey,
                   CONSOLE_REGISTRY_CURSORSIZE,
                   0,
                   REG_DWORD,
                   (LPBYTE)&dwValue,
                   SIZEOF(dwValue)
                  );

    //
    // Save history buffer size and number
    //

    dwValue = pcpd->lpConsole->uHistoryBufferSize;
    RegSetValueEx( hTitleKey,
                   CONSOLE_REGISTRY_HISTORYSIZE,
                   0,
                   REG_DWORD,
                   (LPBYTE)&dwValue,
                   SIZEOF(dwValue)
                  );
    dwValue = pcpd->lpConsole->uNumberOfHistoryBuffers;
    RegSetValueEx( hTitleKey,
                   CONSOLE_REGISTRY_HISTORYBUFS,
                   0,
                   REG_DWORD,
                   (LPBYTE)&dwValue,
                   SIZEOF(dwValue)
                  );
    dwValue = pcpd->lpConsole->bHistoryNoDup;
    RegSetValueEx( hTitleKey,
                   CONSOLE_REGISTRY_HISTORYNODUP,
                   0,
                   REG_DWORD,
                   (LPBYTE)&dwValue,
                   SIZEOF(dwValue)
                  );

    //
    // Close the registry keys
    //

    if (hTitleKey != hConsoleKey) {
        RegCloseKey(hTitleKey);
    }

CloseKeys:
    RegCloseKey(hConsoleKey);
    RegCloseKey(hCurrentUserKey);
}

void
InitFERegistryValues( LPCONSOLEPROP_DATA pcpd )

/*++

Routine Description:

    This routine allocates a state info structure and fill it in with
    default values.  It then tries to load the default settings for
    console from the registry.

Arguments:

    none

Return Value:

    pStateInfo - pointer to structure to receive information

--*/

{
    /*
     * In this case: console reads a property of US version.
     * It doesn't have code page information.
     * Console should sets some code page as default.
     * However, I don't know right value. 437 is temporary value.
     */
    pcpd->lpFEConsole->uCodePage = 437;

    GetFERegistryValues( pcpd );
}


VOID
GetFERegistryValues(
    LPCONSOLEPROP_DATA pcpd
    )

/*++

Routine Description:

    This routine reads in values from the registry and places them
    in the supplied structure.

Arguments:

    pStateInfo - optional pointer to structure to receive information

Return Value:

    current page number

--*/

{
    HKEY hCurrentUserKey;
    HKEY hConsoleKey;
    HKEY hTitleKey;
    LPTSTR TranslatedTitle;
    DWORD dwValue, dwSize;
    DWORD dwRet = 0;

    //
    // Open the current user registry key
    //

    if (RegOpenKey( HKEY_CURRENT_USER, NULL, &hCurrentUserKey)!=ERROR_SUCCESS)
        return;

    //
    // Open the console registry key
    //

    if (RegOpenKey(hCurrentUserKey,CONSOLE_REGISTRY_STRING,&hConsoleKey)!=ERROR_SUCCESS)
    {
        NtClose(hCurrentUserKey);
        return;
    }

    //
    // If there is no structure to fill out, just bail out
    //

    if ((!pcpd) || (!pcpd->lpFEConsole))
        goto CloseKeys;

    //
    // Open the console title subkey, if there is one
    //

    if (pcpd->ConsoleTitle[0] != TEXT('\0'))
    {
        TranslatedTitle = TranslateConsoleTitle(pcpd->ConsoleTitle);
        if (TranslatedTitle == NULL)
            goto CloseKeys;
        dwValue = RegOpenKey( hConsoleKey,
                              TranslatedTitle,
                              &hTitleKey);
        LocalFree(TranslatedTitle);
        if (dwValue!=ERROR_SUCCESS)
            goto CloseKeys;
    } else {
            goto CloseKeys;
    }

    //
    // Initial code page
    //

    dwSize = SIZEOF(dwValue);
    if (SHQueryValueEx( hTitleKey,
                         CONSOLE_REGISTRY_CODEPAGE,
                         NULL,
                         NULL,
                         (LPBYTE)&dwValue,
                         &dwSize
                        ) == ERROR_SUCCESS)
    {
        pcpd->lpFEConsole->uCodePage = (UINT)dwValue;
    }

    //
    // Close the registry keys
    //

    if (hTitleKey != hConsoleKey) {
        RegCloseKey(hTitleKey);
    }

CloseKeys:
    RegCloseKey(hConsoleKey);
    RegCloseKey(hCurrentUserKey);

}


VOID
SetFERegistryValues(
    LPCONSOLEPROP_DATA pcpd
    )

/*++

Routine Description:

    This routine writes values to the registry from the supplied
    structure.

Arguments:

    pStateInfo - optional pointer to structure containing information
    dwPage     - current page number

Return Value:

    none

--*/

{
    HKEY hCurrentUserKey;
    HKEY hConsoleKey;
    HKEY hTitleKey;
    LPTSTR TranslatedTitle;
    DWORD dwValue;

    //
    // Open the current user registry key
    //

    if (RegOpenKey( HKEY_CURRENT_USER, NULL, &hCurrentUserKey )!=ERROR_SUCCESS)
    {
        return;
    }

    //
    // Open the console registry key
    //

    if (RegCreateKey( hCurrentUserKey, CONSOLE_REGISTRY_STRING, &hConsoleKey )!=ERROR_SUCCESS)
    {
        RegCloseKey(hCurrentUserKey);
        return;
    }

    //
    // If we only want to save the current page, bail out
    //

    if (pcpd == NULL)
    {
        goto CloseKeys;
    }

    //
    // Open the console title subkey, if there is one
    //

    if (pcpd->ConsoleTitle[0] != TEXT('\0'))
    {
        TranslatedTitle = TranslateConsoleTitle(pcpd->ConsoleTitle);
        if (TranslatedTitle == NULL)
        {
            RegCloseKey(hConsoleKey);
            RegCloseKey(hCurrentUserKey);
            return;
        }
        dwValue = RegCreateKey( hConsoleKey,
                                TranslatedTitle,
                                &hTitleKey);
        LocalFree(TranslatedTitle);
        if (dwValue!=ERROR_SUCCESS)
        {
            RegCloseKey(hConsoleKey);
            RegCloseKey(hCurrentUserKey);
            return;
        }
    } else {
        hTitleKey = hConsoleKey;
    }

    // scotthsu
    dwValue = pcpd->lpFEConsole->uCodePage;
    RegSetValueEx( hTitleKey,
                   CONSOLE_REGISTRY_CODEPAGE,
                   0,
                   REG_DWORD,
                   (LPBYTE)&dwValue,
                   SIZEOF(dwValue)
                  );

    //
    // Close the registry keys
    //

    if (hTitleKey != hConsoleKey) {
        RegCloseKey(hTitleKey);
    }

CloseKeys:
    RegCloseKey(hConsoleKey);
    RegCloseKey(hCurrentUserKey);
}

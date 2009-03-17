/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         GDI font driver for bitmap fonts
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include "bmfd.h"

static
BOOLEAN
IsValidPtr(
    PVOID p,
    ULONG cjSize,
    PVOID pStart,
    PVOID pEnd,
    ULONG cjAlign)
{
    if ((ULONG_PTR)p < (ULONG_PTR)pStart ||
        (ULONG_PTR)p + cjSize >= (ULONG_PTR)pEnd ||
        (ULONG_PTR)p & (cjAlign -1))
    {
        return FALSE;
    }
    return TRUE;
}

static
BOOL
FillFaceInfo(
    PBMFD_FACE pface,
    PFONTINFO16 pFontInfo)
{
    CHAR ansi[4];
    WCHAR unicode[4];
    ULONG written;
    DWORD dfFlags;

    pface->pFontInfo = pFontInfo;
    pface->ulVersion = GETVAL(pFontInfo->dfVersion);
    pface->cGlyphs = pFontInfo->dfLastChar - pFontInfo->dfFirstChar + 1;

    /* Convert chars to unicode */
    ansi[0] = pFontInfo->dfFirstChar;
    ansi[1] = pFontInfo->dfLastChar;
    ansi[2] = pFontInfo->dfFirstChar + pFontInfo->dfDefaultChar;
    ansi[3] = pFontInfo->dfFirstChar + pFontInfo->dfBreakChar;
    EngMultiByteToUnicodeN(unicode, 4 * sizeof(WCHAR), &written, ansi, 4);
    pface->wcFirstChar = unicode[0];
    pface->wcLastChar = unicode[1];
    pface->wcDefaultChar = unicode[2];
    pface->wcBreakChar = unicode[3];

    /* Copy some values */
    pface->wPixHeight = GETVAL(pFontInfo->dfPixHeight);
    pface->wPixWidth = GETVAL(pFontInfo->dfPixWidth);
    pface->wWidthBytes = GETVAL(pFontInfo->dfWidthBytes);
    pface->wAscent = GETVAL(pFontInfo->dfAscent);
    pface->wDescent = pface->wPixHeight - pface->wAscent;

    /* Some version specific members */
    if (pface->ulVersion >= 0x300)
    {
        dfFlags = GETVAL(pFontInfo->dfFlags);
        pface->wA = GETVAL(pFontInfo->dfAspace);
        pface->wB = GETVAL(pFontInfo->dfBspace);
        pface->wC = GETVAL(pFontInfo->dfCspace);
        pface->pCharTable = pface->pFontInfo->dfCharTable;
        pface->cjEntrySize = sizeof(GLYPHENTRY30);
    }
    else
    {
        dfFlags = DFF_1COLOR;
        pface->wA = 0;
        pface->wB = 0;
        pface->wC = 0;
        pface->pCharTable = &pface->pFontInfo->dfReserved + 1;
        pface->cjEntrySize = sizeof(GLYPHENTRY20);
    }

    pface->flInfo = FM_INFO_MASK;

    /* If dfWidth is non-null, we have a fixed width font */
    if (dfFlags & DFF_FIXED || pface->wPixWidth)
        pface->flInfo |= FM_INFO_CONSTANT_WIDTH;

    /* Initialize color depth flags */
    if (dfFlags & DFF_1COLOR)
        pface->flInfo |= FM_INFO_1BPP;
    else if (dfFlags & DFF_16COLOR)
        pface->flInfo |= FM_INFO_4BPP;
    else if (dfFlags & DFF_256COLOR)
        pface->flInfo |= FM_INFO_8BPP;
    else if (dfFlags & DFF_RGBCOLOR)
        pface->flInfo |= FM_INFO_24BPP;

    // TODO: walk through all glyphs and veryfy them and calculate max values

    // FIXME: After this point, the whole font data should be verified!

    return TRUE;
}

static
PVOID
ParseFntFile(
    PVOID pvView,
    ULONG cjView)
{
    /* unimplemented */
    return NULL;
}


static
PVOID
ParseFonFile(
    PVOID pvView,
    ULONG cjView)
{
    PIMAGE_DOS_HEADER pDosHeader = pvView;
    PIMAGE_OS2_HEADER pOs2Header;
    PNE_RESTABLE pResTable;
    PNE_TYPEINFO pTInfo;
    PFONTINFO16 pFontInfo;
    PCHAR pStart, pEnd;
    PBMFD_FILE pfile = NULL;
    WORD wShift;
    ULONG i, cjOffset, cjLength;
    ULONG type_id, count;

    /* Initial margins for valid pointers */
    pStart = pvView;
    pEnd = pStart + cjView;

    /* Check for image dos header */
    if (GETVAL(pDosHeader->e_magic) != IMAGE_DOS_MAGIC)
    {
        return NULL;
    }

    /* Get pointer to OS2 header and veryfy it is valid */
    pOs2Header = (PVOID)((PCHAR)pDosHeader + GETVAL(pDosHeader->e_lfanew));
    pStart += sizeof(IMAGE_DOS_HEADER);
    if (!IsValidPtr(pOs2Header, sizeof(IMAGE_OS2_HEADER), pStart, pEnd, 4))
    {
        DbgPrint("e_lfanew is invalid: 0x%lx\n", pDosHeader->e_lfanew);
        return NULL;
    }

    /* Get pointer to resource table and verify it is valid */
    pResTable = (PVOID)((PCHAR)pOs2Header + GETVAL(pOs2Header->ne_rsrctab));
    pStart = (PCHAR)pOs2Header;
    if (!IsValidPtr(pResTable, sizeof(NE_RESTABLE), pStart, pEnd, 1))
    {
        DbgPrint("pTInfo is invalid: 0x%p\n", pResTable);
        return NULL;
    }

    wShift = GETVAL(pResTable->size_shift);
    pTInfo = pResTable->typeinfo;
    type_id = GETVAL(pTInfo->type_id);

    /* Loop the resource table to find a font resource */
    while (type_id)
    {
        /* Get number of nameinfo entries */
        count = GETVAL(pTInfo->count);

        /* Look for a font resource */
        if (type_id == NE_RSCTYPE_FONT && count > 0)
        {
            DbgPrint("Found NE_RSCTYPE_FONT\n");

            /* Allocate an info structure for this font and all faces */
            cjLength = sizeof(BMFD_FILE) + (count-1) * sizeof(BMFD_FACE);
            pfile = EngAllocMem(0, cjLength, TAG_FONTINFO);
            if (!pfile)
            {
                DbgPrint("Not enough memory: %ld\n", cjLength);
                return NULL;
            }

            pfile->cNumFaces = count;

            /* Fill all face info structures */
            for (i = 0; i < count; i++)
            {
                cjOffset = GETVAL(pTInfo->nameinfo[i].offset) << wShift;
                cjLength = GETVAL(pTInfo->nameinfo[i].length) << wShift;
                pFontInfo = (PVOID)((PCHAR)pDosHeader + cjOffset);

                if (!IsValidPtr(pFontInfo, cjLength, pStart, pEnd, 1))
                {
                    DbgPrint("pFontInfo is invalid: 0x%p\n", pFontInfo);
                    EngFreeMem(pfile);
                    return NULL;
                }

                /* Validate FONTINFO and fill face info */
                if (!FillFaceInfo(&pfile->aface[i], pFontInfo))
                {
                    DbgPrint("pFontInfo is invalid: 0x%p\n", pFontInfo);
                    EngFreeMem(pfile);
                    return NULL;
                }
            }

            /* Break out of the loop */
            break;
        }

        /* Following pointers must be bigger than this */
        pStart = (PCHAR)pTInfo;

        /* Goto next entry in resource table */
        pTInfo = (PVOID)&pTInfo->nameinfo[count];

        /* Verify that the new pTInfo pointer is valid */
        if (!IsValidPtr(pTInfo, sizeof(NE_TYPEINFO), pStart, pEnd, 1))
        {
            DbgPrint("pTInfo is invalid: 0x%p\n", pTInfo);
            return NULL;
        }

        type_id = GETVAL(pTInfo->type_id);
    }

    return pfile;
}

/** Public Interface **********************************************************/

ULONG_PTR
APIENTRY
BmfdLoadFontFile(
    ULONG cFiles,
    ULONG_PTR *piFile,
    PVOID *ppvView,
    ULONG *pcjView,
    DESIGNVECTOR *pdv,
    ULONG ulLangID,
    ULONG ulFastCheckSum)
{
    PBMFD_FILE pfile = NULL;
    PVOID pvView;
    ULONG cjView;

    DbgPrint("BmfdLoadFontFile()\n");
    DbgBreakPoint();

    /* Check parameters */
    if (cFiles != 1)
    {
        DbgPrint("Only 1 File is allowed, got %ld!\n", cFiles);
        return HFF_INVALID;
    }

    /* Map the font file */
    if (!EngMapFontFileFD(*piFile, (PULONG*)&pvView, &cjView))
    {
        DbgPrint("Could not map font file!\n", cFiles);
        return HFF_INVALID;
    }

    DbgPrint("mapped font file to %p, site if %ld\n", pvView, cjView);

    /* Try to parse a .fon file */
    pfile = ParseFonFile(pvView, cjView);

    if (!pfile)
    {
        /* Could be a .fnt file */
        pfile = ParseFntFile(pvView, cjView);
    }

    /* Check whether we succeeded finding a font */
    if (!pfile)
    {
        DbgPrint("No font data found\n");

        /* Unmap the file */
        EngUnmapFontFileFD(*piFile);

        /* Failure! */
        return HFF_INVALID;
    }

    pfile->iFile = *piFile;
    pfile->pvView = pvView;

    /* Success, return the pointer to font info structure */
    return (ULONG_PTR)pfile;
}

BOOL
APIENTRY
BmfdUnloadFontFile(
    IN ULONG_PTR iFile)
{
    PBMFD_FILE pfile = (PBMFD_FILE)iFile;

    DbgPrint("BmfdUnloadFontFile()\n");

    /* Unmap the font file */
    EngUnmapFontFileFD(pfile->iFile);

    /* Free the memory that was allocated for the font */
    EngFreeMem(pfile);

    return TRUE;
}


LONG
APIENTRY
BmfdQueryFontFile(
    ULONG_PTR iFile,
    ULONG ulMode,
    ULONG cjBuf,
    ULONG *pulBuf)
{
    PBMFD_FILE pfile = (PBMFD_FILE)iFile;

    DbgPrint("BmfdQueryFontFile()\n");
//    DbgBreakPoint();

    switch (ulMode)
    {
        case QFF_DESCRIPTION:
        {
            /* We copy the face name of the 1st face */
            PCHAR pDesc = pfile->aface[0].pszFaceName;
            ULONG cOutSize;
            if (pulBuf)
            {
                EngMultiByteToUnicodeN((LPWSTR)pulBuf,
                                       cjBuf,
                                       &cOutSize,
                                       pDesc,
                                       strnlen(pDesc, LF_FACESIZE));
            }
            else
            {
                cOutSize = (strnlen(pDesc, LF_FACESIZE) + 1) * sizeof(WCHAR);
            }
            return cOutSize;
        }

        case QFF_NUMFACES:
            /* return the number of faces in the file */
            return pfile->cNumFaces;

        default:
            return FD_ERROR;
    }
}

LONG
APIENTRY
BmfdQueryFontCaps(
    ULONG culCaps,
    ULONG *pulCaps)
{
    DbgPrint("BmfdQueryFontCaps()\n");

    /* We need room for 2 ULONGs */
    if (culCaps < 2)
    {
        return FD_ERROR;
    }

    /* We only support 1 bpp */
    pulCaps[0] = 2;
    pulCaps[1] = QC_1BIT;

    return 2;
}


PVOID
APIENTRY
BmfdQueryFontTree(
    DHPDEV dhpdev,
    ULONG_PTR iFile,
    ULONG iFace,
    ULONG iMode,
    ULONG_PTR *pid)
{
    PBMFD_FILE pfile = (PBMFD_FILE)iFile;
    PBMFD_FACE pface;
    ULONG i, j, cjOffset, cjSize, cGlyphs, cRuns;
    CHAR ch, chFirst, ach[256];
    WCHAR wc, awc[256];
    PFD_GLYPHSET pGlyphSet;
    WCRUN *pwcrun;
    HGLYPH * phglyphs;

    DbgPrint("DrvQueryFontTree(iMode=%ld)\n", iMode);
//    DbgBreakPoint();

    /* Check parameters, we only support QFT_GLYPHSET */
    if (!iFace || iFace > pfile->cNumFaces || iMode != QFT_GLYPHSET)
    {
        DbgPrint("iFace = %ld, cNumFaces = %ld\n", iFace, pfile->cNumFaces);
        return NULL;
    }

    /* Get a pointer to the face data */
    pface = &pfile->aface[iFace - 1];

    /* Get the number of characters in the face */
    cGlyphs = pface->cGlyphs;

    chFirst = pface->pFontInfo->dfFirstChar;

    /* Build array of supported chars */
    for (i = 0; i < cGlyphs; i++)
    {
        ach[i] = chFirst + i;
    }

    /* Convert the chars to unicode */
    EngMultiByteToUnicodeN(awc, sizeof(awc), NULL, ach, cGlyphs);

    /* Sort both arrays in wchar order */
    for (i = 0; i < cGlyphs - 1; i++)
    {
        wc = awc[i];
        for (j = i + 1; j < cGlyphs; j++)
        {
            if (awc[j] < wc)
            {
                awc[i] = awc[j];
                awc[j] = wc;
                wc = awc[i];
                ch = ach[i];
                ach[i] = ach[j];
                ach[j] = ch;
            }
        }
    }

    /* Find number of WCRUNs */
    cRuns = 1;
    for (i = 1; i < cGlyphs; i++)
    {
        if (awc[i] != awc[i - 1] + 1)
        {
            cRuns++;
        }
    }

    /* Calculate FD_GLYPHSET size */
    cjSize = sizeof(FD_GLYPHSET)
             + (cRuns - 1) * sizeof(WCRUN)
             + cGlyphs * sizeof(HGLYPH);

    /* Allocate the FD_GLYPHSET structure */
    pGlyphSet = EngAllocMem(0, cjSize, TAG_GLYPHSET);
    if (!pGlyphSet)
    {
        return NULL;
    }

    /* Initialize FD_GLYPHSET */
    pGlyphSet->cjThis = cjSize;
    pGlyphSet->flAccel = 0;
    pGlyphSet->cGlyphsSupported = cGlyphs;
    pGlyphSet->cRuns = cRuns;

    /* Initialize 1st WCRUN */
    pwcrun = pGlyphSet->awcrun;
    phglyphs = (PHGLYPH)&pGlyphSet->awcrun[cRuns];
    pwcrun[0].wcLow = awc[0];
    pwcrun[0].cGlyphs = 1;
    pwcrun[0].phg = phglyphs;
    phglyphs[0] = (HGLYPH)pface->pCharTable;

    /* Walk through all supported chars */
    for (i = 1, j = 0; i < cGlyphs; i++)
    {
        /* Use pointer to glyph entry as hglyph */
        cjOffset = (ach[i] - chFirst) * pface->cjEntrySize;
        phglyphs[i] = (HGLYPH)(pface->pCharTable + cjOffset);

        /* Check whether we can append the wchar to a run */
        if (awc[i] == awc[i - 1] + 1)
        {
            /* Append to current WCRUN */
            pwcrun[j].cGlyphs++;
        }
        else
        {
            /* Add a new WCRUN */
            j++;
            pwcrun[j].wcLow = awc[i];
            pwcrun[j].cGlyphs = 1;
            pwcrun[j].phg = &phglyphs[i];
        }
    }

    /* Set *pid to the allocated structure for use in BmfdFree */
    *pid = (ULONG_PTR)pGlyphSet;

    return pGlyphSet;
}

PIFIMETRICS
APIENTRY
BmfdQueryFont(
    IN DHPDEV dhpdev,
    IN ULONG_PTR iFile,
    IN ULONG iFace,
    IN ULONG_PTR *pid)
{
    PBMFD_FILE pfile = (PBMFD_FILE)iFile;
    PBMFD_FACE pface;
    PFONTINFO16 pFontInfo;
    PIFIMETRICS pifi;
    PBMFD_IFIMETRICS pifiX;
    PANOSE panose = {0};

    DbgPrint("BmfdQueryFont()\n");
//    DbgBreakPoint();

    /* Validate parameters */
    if (iFace > pfile->cNumFaces || !pid)
    {
        return NULL;
    }

    pface = &pfile->aface[iFace - 1];
    pFontInfo = pface->pFontInfo;

    /* Allocate the structure */
    pifiX = EngAllocMem(FL_ZERO_MEMORY, sizeof(BMFD_IFIMETRICS), TAG_IFIMETRICS);
    if (!pifiX)
    {
        return NULL;
    }

    /* Return a pointer to free it later */
    *pid = (ULONG_PTR)pifiX;

    /* Fill IFIMETRICS */
    pifi = &pifiX->ifim;
    pifi->cjThis = sizeof(BMFD_IFIMETRICS);
    pifi->cjIfiExtra = 0;
    pifi->dpwszFamilyName = FIELD_OFFSET(BMFD_IFIMETRICS, wszFamilyName);
    pifi->dpwszStyleName = FIELD_OFFSET(BMFD_IFIMETRICS, wszFamilyName);
    pifi->dpwszFaceName = FIELD_OFFSET(BMFD_IFIMETRICS, wszFaceName);
    pifi->dpwszUniqueName = FIELD_OFFSET(BMFD_IFIMETRICS, wszFaceName);
    pifi->dpFontSim = 0;
    pifi->lEmbedId = 0;
    pifi->lItalicAngle = 0;
    pifi->lCharBias = 0;
    pifi->dpCharSets = 0;
    pifi->jWinCharSet = pFontInfo->dfCharSet;
    pifi->jWinPitchAndFamily = pFontInfo->dfPitchAndFamily;
    pifi->usWinWeight = GETVAL(pFontInfo->dfWeight);
    pifi->flInfo = pface->flInfo;
    pifi->fsSelection = 0;
    pifi->fsType = 0;
    pifi->fwdUnitsPerEm = GETVAL(pFontInfo->dfPixHeight);
    pifi->fwdLowestPPEm = 0;
    pifi->fwdWinAscender = GETVAL(pFontInfo->dfAscent);
    pifi->fwdWinDescender = pifi->fwdUnitsPerEm - pifi->fwdWinAscender;
    pifi->fwdMacAscender = pifi->fwdWinAscender;
    pifi->fwdMacDescender = - pifi->fwdWinDescender;
    pifi->fwdMacLineGap = 0;
    pifi->fwdTypoAscender = pifi->fwdWinAscender;
    pifi->fwdTypoDescender = - pifi->fwdWinDescender;
    pifi->fwdTypoLineGap = 0;
    pifi->fwdAveCharWidth = GETVAL(pFontInfo->dfAvgWidth);
    pifi->fwdMaxCharInc =  GETVAL(pFontInfo->dfMaxWidth);
    pifi->fwdCapHeight = pifi->fwdUnitsPerEm / 2;
    pifi->fwdXHeight = pifi->fwdUnitsPerEm / 4;
    pifi->fwdSubscriptXSize = 0;
    pifi->fwdSubscriptYSize = 0;
    pifi->fwdSubscriptXOffset = 0;
    pifi->fwdSubscriptYOffset = 0;
    pifi->fwdSuperscriptXSize = 0;
    pifi->fwdSuperscriptYSize = 0;
    pifi->fwdSuperscriptXOffset = 0;
    pifi->fwdSuperscriptYOffset = 0;
    pifi->fwdUnderscoreSize = 01;
    pifi->fwdUnderscorePosition = -1;
    pifi->fwdStrikeoutSize = 1;
    pifi->fwdStrikeoutPosition = pifi->fwdXHeight + 1;
    pifi->chFirstChar = pFontInfo->dfFirstChar;
    pifi->chLastChar = pFontInfo->dfLastChar;
    pifi->chDefaultChar = pFontInfo->dfFirstChar + pFontInfo->dfDefaultChar;
    pifi->chBreakChar = pFontInfo->dfFirstChar + pFontInfo->dfBreakChar;
    pifi->wcFirstChar = pface->wcFirstChar;
    pifi->wcLastChar = pface->wcLastChar;
    pifi->wcDefaultChar = pface->wcDefaultChar;
    pifi->wcBreakChar = pface->wcBreakChar;
    pifi->ptlBaseline.x = 1;
    pifi->ptlBaseline.y = 0;
    pifi->ptlAspect.x = pFontInfo->dfVertRes; // CHECKME
    pifi->ptlAspect.y = pFontInfo->dfHorizRes;
    pifi->ptlCaret.x = 0;
    pifi->ptlCaret.y = 1;
    pifi->rclFontBox.left = 0;
    pifi->rclFontBox.right = pifi->fwdAveCharWidth;
    pifi->rclFontBox.top = pifi->fwdWinAscender;
    pifi->rclFontBox.bottom = - pifi->fwdWinDescender;
    *(DWORD*)&pifi->achVendId = 0x30303030; // FIXME
    pifi->cKerningPairs = 0;
    pifi->ulPanoseCulture = FM_PANOSE_CULTURE_LATIN;
    pifi->panose = panose;

    /* Set char sets */
    pifiX->ajCharSet[0] = pifi->jWinCharSet;
    pifiX->ajCharSet[1] = DEFAULT_CHARSET;

    if (pface->flInfo & FM_INFO_CONSTANT_WIDTH)
        pifi->jWinPitchAndFamily |= FIXED_PITCH;

#if 0
    EngMultiByteToUnicodeN(pifiX->wszFaceName,
                           LF_FACESIZE * sizeof(WCHAR),
                           NULL,
                           pFontInfo->,
                           strnlen(pDesc, LF_FACESIZE));
#endif
    wcscpy(pifiX->wszFaceName, L"Courier-X");
    wcscpy(pifiX->wszFamilyName, L"Courier-X");

    /* Initialize font weight style flags and string */
    if (pifi->usWinWeight == FW_REGULAR)
    {
   //     pifi->fsSelection |= FM_SEL_REGULAR;
    }
    else if (pifi->usWinWeight > FW_SEMIBOLD)
    {
        pifi->fsSelection |= FM_SEL_BOLD;
        wcscat(pifiX->wszStyleName, L"Bold ");
    }
    else if (pifi->usWinWeight <= FW_LIGHT)
    {
        wcscat(pifiX->wszStyleName, L"Light ");
    }

    if (pFontInfo->dfItalic)
    {
        pifi->fsSelection |= FM_SEL_ITALIC;
        wcscat(pifiX->wszStyleName, L"Italic ");
    }

    if (pFontInfo->dfUnderline)
    {
        pifi->fsSelection |= FM_SEL_UNDERSCORE;
        wcscat(pifiX->wszStyleName, L"Underscore ");
    }

    if (pFontInfo->dfStrikeOut)
    {
        pifi->fsSelection |= FM_SEL_STRIKEOUT;
        wcscat(pifiX->wszStyleName, L"Strikeout ");
    }

    return pifi;
}


VOID
APIENTRY
BmfdFree(
    PVOID pv,
    ULONG_PTR id)
{
    DbgPrint("BmfdFree()\n");
    if (id)
    {
        EngFreeMem((PVOID)id);
    }
}


VOID
APIENTRY
BmfdDestroyFont(
    IN FONTOBJ *pfo)
{
    /* Free the font realization info */
    EngFreeMem(pfo->pvProducer);
    pfo->pvProducer = NULL;
}

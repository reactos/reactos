/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         GDI font driver for bitmap fonts
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include "ftfd.h"

PVOID
HackFixup(
    PVOID pvView,
    ULONG cjView)
{
    CHAR *pc;
    CHAR c;

    pc = EngAllocMem(0, cjView, 'tmp ');
    memcpy(pc, pvView, cjView);

    c = *pc;
    *pc = 0;

    return pc;
}

/** Public Interface **********************************************************/

ULONG_PTR
APIENTRY
FtfdLoadFontFile(
    ULONG cFiles,
    ULONG_PTR *piFile,
    PVOID *ppvView,
    ULONG *pcjView,
    DESIGNVECTOR *pdv,
    ULONG ulLangID,
    ULONG ulFastCheckSum)
{
    PVOID pvView;
    ULONG cjView, i;
    FT_Error fterror;
    FT_Face ftface;
    PFTFD_FILE pfile;
    ULONG cjSize, cNumFaces;

    DbgPrint("FtfdLoadFontFile()\n");

    /* Check parameters */
    if (cFiles != 1)
    {
        DbgPrint("Only 1 File is allowed, got %ld!\n", cFiles);
        return HFF_INVALID;
    }

    /* Map the font file */
    if (!EngMapFontFileFD(*piFile, (PULONG*)&pvView, &cjView))
    {
        DbgPrint("Could not map font file!\n");
        return HFF_INVALID;
    }

    // HACK!!!
    pvView = HackFixup(pvView, cjView);

    fterror = FT_New_Memory_Face(gftlibrary, pvView, cjView, 0, &ftface);
    if (fterror)
    {
        DbgPrint("No faces found in file\n");

        /* Unmap the file */
        EngUnmapFontFileFD(*piFile);

        /* Failure! */
        return HFF_INVALID;
    }

    /* Get number of faces from the first face */
    cNumFaces = ftface->num_faces;

    cjSize = sizeof(FTFD_FILE) + cNumFaces * sizeof(FT_Face);
    pfile = EngAllocMem(0, cjSize, 'dftF');
    if (!pfile)
    {
        DbgPrint("EngAllocMem() failed.\n");

        /* Unmap the file */
        EngUnmapFontFileFD(*piFile);

        /* Failure! */
        return HFF_INVALID;
    }

    pfile->cNumFaces = cNumFaces;
    pfile->iFile = *piFile;
    pfile->pvView = pvView;
    pfile->cjView = cjView;
        
    for (i = 0; i < pfile->cNumFaces; i++)
    {
        pfile->aftface[i] = ftface;
        FT_Select_Charmap(ftface, FT_ENCODING_UNICODE);
    }

    DbgPrint("Success! Returning %ld faces\n", cNumFaces);

    return (ULONG_PTR)pfile;
}

BOOL
APIENTRY
FtfdUnloadFontFile(
    IN ULONG_PTR iFile)
{
    PFTFD_FILE pfile = (PFTFD_FILE)iFile;
    ULONG i;

    DbgPrint("FtfdUnloadFontFile()\n");

    // HACK!!!
    EngFreeMem(pfile->pvView);

    /* Cleanup faces */
    for (i = 0; i < pfile->cNumFaces; i++)
    {
        FT_Done_Face(pfile->aftface[i]);
    }

    /* Unmap the font file */
    EngUnmapFontFileFD(pfile->iFile);

    /* Free the memory that was allocated for the font */
    EngFreeMem(pfile);

    return TRUE;
}


LONG
APIENTRY
FtfdQueryFontFile(
    ULONG_PTR iFile,
    ULONG ulMode,
    ULONG cjBuf,
    ULONG *pulBuf)
{
    PFTFD_FILE pfile = (PFTFD_FILE)iFile;

    DbgPrint("FtfdQueryFontFile(ulMode=%ld)\n", ulMode);
//    DbgBreakPoint();

    switch (ulMode)
    {
        case QFF_DESCRIPTION:
        {
            return 0;
        }

        case QFF_NUMFACES:
            /* return the number of faces in the file */
            return pfile->cNumFaces;

    }

    return FD_ERROR;
}


PIFIMETRICS
APIENTRY
FtfdQueryFont(
    IN DHPDEV dhpdev,
    IN ULONG_PTR iFile,
    IN ULONG iFace,
    IN ULONG_PTR *pid)
{
    PFTFD_FILE pfile = (PFTFD_FILE)iFile;
    PFTFD_IFIMETRICS pifiX;
    PIFIMETRICS pifi;
    FT_Face ftface;
    FT_Error fterror;

    DbgPrint("FtfdQueryFont()\n");

    /* Validate parameters */
    if (iFace > pfile->cNumFaces || !pid)
    {
        return NULL;
    }

    fterror = FT_New_Memory_Face(gftlibrary,
                                 pfile->pvView,
                                 pfile->cjView,
                                 iFace - 1,
                                 &ftface);
    if (fterror)
    {
        return NULL;
    }

    /* Allocate the ifi metrics structure */
    pifiX = EngAllocMem(FL_ZERO_MEMORY, sizeof(FTFD_IFIMETRICS), TAG_IFIMETRICS);
    if (!pifiX)
    {
        DbgPrint("EngAllocMem() failed.\n");
        FT_Done_Face(ftface);
        return NULL;
    }

    /* Fill IFIMETRICS */
    pifi = &pifiX->ifim;
    pifi->cjThis = sizeof(FTFD_IFIMETRICS);
    pifi->cjIfiExtra = 0;
    pifi->dpwszFamilyName = FIELD_OFFSET(FTFD_IFIMETRICS, wszFamilyName);
    pifi->dpwszStyleName = FIELD_OFFSET(FTFD_IFIMETRICS, wszFamilyName);
    pifi->dpwszFaceName = FIELD_OFFSET(FTFD_IFIMETRICS, wszFaceName);
    pifi->dpwszUniqueName = FIELD_OFFSET(FTFD_IFIMETRICS, wszFaceName);
    pifi->dpFontSim = 0;
    pifi->lEmbedId = 0;
    pifi->lItalicAngle = 0;
    pifi->lCharBias = 0;
    pifi->dpCharSets = 0;
//    pifi->jWinCharSet = pFontInfo->dfCharSet;
//    pifi->jWinPitchAndFamily = pFontInfo->dfPitchAndFamily;
//    pifi->usWinWeight = GETVAL(pFontInfo->dfWeight);
//    pifi->flInfo = pface->flInfo;
    pifi->fsSelection = 0;
    pifi->fsType = 0;
//    pifi->fwdUnitsPerEm = GETVAL(pFontInfo->dfPixHeight);
    pifi->fwdLowestPPEm = 0;
//    pifi->fwdWinAscender = GETVAL(pFontInfo->dfAscent);
    pifi->fwdWinDescender = pifi->fwdUnitsPerEm - pifi->fwdWinAscender;
    pifi->fwdMacAscender = pifi->fwdWinAscender;
    pifi->fwdMacDescender = - pifi->fwdWinDescender;
    pifi->fwdMacLineGap = 0;
    pifi->fwdTypoAscender = pifi->fwdWinAscender;
    pifi->fwdTypoDescender = - pifi->fwdWinDescender;
    pifi->fwdTypoLineGap = 0;
//    pifi->fwdAveCharWidth = GETVAL(pFontInfo->dfAvgWidth);
//    pifi->fwdMaxCharInc =  GETVAL(pFontInfo->dfMaxWidth);
//    pifi->fwdCapHeight = pifi->fwdUnitsPerEm / 2;
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
//    pifi->chFirstChar = pFontInfo->dfFirstChar;
//    pifi->chLastChar = pFontInfo->dfLastChar;
//    pifi->chDefaultChar = pFontInfo->dfFirstChar + pFontInfo->dfDefaultChar;
//    pifi->chBreakChar = pFontInfo->dfFirstChar + pFontInfo->dfBreakChar;
//    pifi->wcFirstChar = pface->wcFirstChar;
//    pifi->wcLastChar = pface->wcLastChar;
//    pifi->wcDefaultChar = pface->wcDefaultChar;
//    pifi->wcBreakChar = pface->wcBreakChar;
    pifi->ptlBaseline.x = 1;
    pifi->ptlBaseline.y = 0;
//    pifi->ptlAspect.x = pFontInfo->dfVertRes; // CHECKME
//    pifi->ptlAspect.y = pFontInfo->dfHorizRes;
    pifi->ptlCaret.x = 0;
    pifi->ptlCaret.y = 1;
    pifi->rclFontBox.left = 0;
    pifi->rclFontBox.right = pifi->fwdAveCharWidth;
    pifi->rclFontBox.top = pifi->fwdWinAscender;
    pifi->rclFontBox.bottom = - pifi->fwdWinDescender;
    *(DWORD*)&pifi->achVendId = 0x30303030; // FIXME
    pifi->cKerningPairs = 0;
    pifi->ulPanoseCulture = FM_PANOSE_CULTURE_LATIN;
//    pifi->panose = panose;

    /* Set char sets */
    pifiX->ajCharSet[0] = pifi->jWinCharSet;
    pifiX->ajCharSet[1] = DEFAULT_CHARSET;

    FT_Done_Face(ftface);


    return 0;
}


LONG
APIENTRY
FtfdQueryFontCaps(
    ULONG culCaps,
    ULONG *pulCaps)
{
    DbgPrint("FtfdQueryFontCaps()\n");

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
FtfdQueryFontTree(
    DHPDEV dhpdev,
    ULONG_PTR iFile,
    ULONG iFace,
    ULONG iMode,
    ULONG_PTR *pid)
{
    PFTFD_FILE pfile = (PFTFD_FILE)iFile;
    FT_Face ftface;
    FT_Error fterror;
    FTFD_CHARPAIR *pcp;
    FD_GLYPHSET *pGlyphSet;
    FT_ULong charcode;
    ULONG i, j, cGlyphs, cRuns, cjSize;
    WCRUN *pwcrun;
    HGLYPH * phglyphs;

    DbgPrint("FtfdQueryFontTree()\n");

    fterror = FT_New_Memory_Face(gftlibrary,
                                 pfile->pvView,
                                 pfile->cjView,
                                 iFace - 1,
                                 &ftface);
    if (fterror)
    {
        return NULL;
    }

    cGlyphs = ftface->num_glyphs;
    cRuns = 1;

    pcp = EngAllocMem(0, cGlyphs * sizeof(FTFD_CHARPAIR), 'pcp ');
    if (!pcp)
    {
        return NULL;
    }

    charcode = FT_Get_First_Char(ftface, &pcp[0].index);
    for (i = 0; i < cGlyphs && charcode != 0; i++)
    {
        pcp[i].charcode = charcode;
        charcode = FT_Get_Next_Char(ftface, charcode, &pcp[i].index);
        if (charcode != pcp[i - 1].charcode + 1)
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
    pwcrun[0].wcLow = pcp[0].charcode;
    pwcrun[0].cGlyphs = 1;
    pwcrun[0].phg = phglyphs;
    phglyphs[0] = pcp[0].index;

    /* Walk through all supported chars */
    for (i = 1, j = 0; i < cGlyphs; i++)
    {
        /* Use glyph index as HGLYPH */
        phglyphs[i] = pcp[i].index;

        /* Check whether we can append the wchar to a run */
        if (pcp[i].charcode == pcp[i - 1].charcode + 1)
        {
            /* Append to current WCRUN */
            pwcrun[j].cGlyphs++;
        }
        else
        {
            /* Add a new WCRUN */
            j++;
            pwcrun[j].wcLow = pcp[i].charcode;
            pwcrun[j].cGlyphs = 1;
            pwcrun[j].phg = &phglyphs[i];
        }
    }

    return NULL;
}

VOID
APIENTRY
FtfdFree(
    PVOID pv,
    ULONG_PTR id)
{
    DbgPrint("FtfdFree()\n");
    if (id)
    {
        EngFreeMem((PVOID)id);
    }
}




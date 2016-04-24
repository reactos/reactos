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

    pc = EngAllocMem(0, cjView, 'tmp ');
    memcpy(pc, pvView, cjView);

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
//    __debugbreak();

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
    ULONG i;

    DbgPrint("FtfdQueryFont()\n");

    /* Validate parameters */
    if (iFace > pfile->cNumFaces || !pid)
    {
        DbgPrint("iFace > pfile->cNumFaces || !pid\n");
        return NULL;
    }

    fterror = FT_New_Memory_Face(gftlibrary,
                                 pfile->pvView,
                                 pfile->cjView,
                                 iFace - 1,
                                 &ftface);
    if (fterror)
    {
        DbgPrint("FT_New_Memory_Face failed\n");
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

    /* Relative offsets */
    pifi->dpwszFamilyName = FIELD_OFFSET(FTFD_IFIMETRICS, wszFamilyName);
    pifi->dpwszStyleName = FIELD_OFFSET(FTFD_IFIMETRICS, wszStyleName);
    pifi->dpwszFaceName = FIELD_OFFSET(FTFD_IFIMETRICS, wszFaceName);
    pifi->dpwszUniqueName = FIELD_OFFSET(FTFD_IFIMETRICS, wszFaceName);
    pifi->dpCharSets = FIELD_OFFSET(FTFD_IFIMETRICS, ajCharSet);
    pifi->dpFontSim = 0;

    /* Charsets */
    pifi->jWinCharSet = ANSI_CHARSET;
    pifiX->ajCharSet[0] = pifi->jWinCharSet;
    for (i = 1; i < 16; i++)
    {
        pifiX->ajCharSet[i] = DEFAULT_CHARSET;
    }

    pifi->lEmbedId = 0;
    pifi->lItalicAngle = 0;
    pifi->lCharBias = 0;
    pifi->jWinPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE; // FIXME
    pifi->usWinWeight = FW_MEDIUM; // FIXME
    pifi->flInfo = FM_INFO_TECH_TRUETYPE | FM_INFO_ARB_XFORMS |
                   FM_INFO_1BPP | FM_INFO_4BPP |
                   FM_INFO_RETURNS_OUTLINES |
                   FM_INFO_RETURNS_BITMAPS |
                   FM_INFO_RIGHT_HANDED;
    pifi->fsSelection = 0;
    pifi->fsType = 0;

    /* Font resolution */
    pifi->fwdUnitsPerEm = ftface->units_per_EM;
    pifi->fwdLowestPPEm = 8; // FIXME

    /* Font metrics */
    pifi->fwdWinAscender = ftface->ascender;
    pifi->fwdWinDescender = - ftface->descender;
    pifi->fwdMacAscender = pifi->fwdWinAscender;
    pifi->fwdMacDescender = - pifi->fwdWinDescender;
    pifi->fwdMacLineGap = 0;
    pifi->fwdTypoAscender = pifi->fwdWinAscender;
    pifi->fwdTypoDescender = 0; // FIXME!!! - pifi->fwdWinDescender;
    pifi->fwdTypoLineGap = 0;
    pifi->fwdAveCharWidth = 1085; // FIXME
    pifi->fwdMaxCharInc =  ftface->max_advance_width;
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
    pifi->fwdUnderscoreSize = 1;
    pifi->fwdUnderscorePosition = -1;
    pifi->fwdStrikeoutSize = 1;
    pifi->fwdStrikeoutPosition = pifi->fwdXHeight + 1;

    pifi->ptlBaseline.x = 1;
    pifi->ptlBaseline.y = 0;
    pifi->ptlAspect.x = 1;
    pifi->ptlAspect.y = 1;
    pifi->ptlCaret.x = 0;
    pifi->ptlCaret.y = 1;

    /* Set the biggest characters bounding box */
    pifi->rclFontBox.left = ftface->bbox.xMin;
    pifi->rclFontBox.right = ftface->bbox.xMax;
    pifi->rclFontBox.top = ftface->bbox.yMax;
    pifi->rclFontBox.bottom = ftface->bbox.yMin;

    /* Special characters */
    pifi->chFirstChar = 0x1c; // FIXME
    pifi->chLastChar = 0x79;
    pifi->chDefaultChar = 0x1d;
    pifi->chBreakChar = 0x1e;
    pifi->wcFirstChar = 0x1e;
    pifi->wcLastChar = 0x79;
    pifi->wcDefaultChar = 0x1d;
    pifi->wcBreakChar = 0x1e;


    *(DWORD*)&pifi->achVendId = 0x30303030; // FIXME
    pifi->cKerningPairs = 0;
    pifi->ulPanoseCulture = FM_PANOSE_CULTURE_LATIN;
//    pifi->panose = panose;

    EngMultiByteToUnicodeN(pifiX->wszFamilyName,
                           LF_FACESIZE,
                           NULL,
                           ftface->family_name,
                           strnlen(ftface->family_name, MAX_PATH));

    EngMultiByteToUnicodeN(pifiX->wszStyleName,
                           LF_FACESIZE,
                           NULL,
                           ftface->style_name,
                           strnlen(ftface->style_name, MAX_PATH));

    EngMultiByteToUnicodeN(pifiX->wszFaceName,
                           LF_FACESIZE,
                           NULL,
                           ftface->family_name,
                           strnlen(ftface->family_name, MAX_PATH));

    FT_Done_Face(ftface);

    DbgPrint("Finished with the ifi: %p\n", pifiX);
    __debugbreak();

    return pifi;
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
        DbgPrint("FT_New_Memory_Face() failed.\n");
        return NULL;
    }

    /* Get initial value for cGlyphs from ftface */
    cGlyphs = ftface->num_glyphs + 1;

    /* Allocate a buffer for the char codes and glyph indexes */
    pcp = EngAllocMem(0, cGlyphs * sizeof(FTFD_CHARPAIR), 'pcp ');
    if (!pcp)
    {
        DbgPrint("EngAllocMem() failed.\n");
        return NULL;
    }

    /* Gather char codes and indexes and count WCRUNs */
    pcp[0].code = FT_Get_First_Char(ftface, &pcp[0].index);
    charcode = pcp[0].code;
    for (i = 1, cRuns = 1; charcode && i < cGlyphs; i++)
    {
        charcode = FT_Get_Next_Char(ftface, charcode, &pcp[i].index);
        DbgPrint("charcode=0x%lx, index=0x%lx\n", charcode, pcp[i].index);
        pcp[i].code = charcode;
        if (charcode != pcp[i - 1].code + 1)
        {
            cRuns++;
        }
    }

    /* Update cGlyphs to real value */
    cGlyphs = i - 1;

    /* Calculate FD_GLYPHSET size */
    cjSize = sizeof(FD_GLYPHSET)
             + (cRuns - 1) * sizeof(WCRUN)
             + cGlyphs * sizeof(HGLYPH);

    /* Allocate the FD_GLYPHSET structure */
    pGlyphSet = EngAllocMem(0, cjSize, TAG_GLYPHSET);
    if (!pGlyphSet)
    {
        DbgPrint("EngAllocMem() failed.\n");
        EngFreeMem(pcp);
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
    pwcrun[0].wcLow = pcp[0].code;
    pwcrun[0].cGlyphs = 1;
    pwcrun[0].phg = &phglyphs[0];
    phglyphs[0] = pcp[0].index;

DbgPrint("pcp[0].index = 0x%lx\n", pcp[0].index);

    /* Walk through all supported chars */
    for (i = 1, j = 0; i < cGlyphs; i++)
    {
        /* Use glyph index as HGLYPH */
        phglyphs[i] = pcp[i].index;

        /* Check whether we can append the wchar to a run */
        if (pcp[i].code == pcp[i - 1].code + 1)
        {
            /* Append to current WCRUN */
            pwcrun[j].cGlyphs++;
        }
        else
        {
            /* Add a new WCRUN */
            DbgPrint("adding new run\n");
            j++;
            pwcrun[j].wcLow = pcp[i].code;
            pwcrun[j].cGlyphs = 1;
            pwcrun[j].phg = &phglyphs[i];
        }
    }

    /* Free the temporary buffer */
    EngFreeMem(pcp);

    /* Set *pid to the allocated structure for use in FtfdFree */
    *pid = (ULONG_PTR)pGlyphSet;

DbgPrint("pGlyphSet=%p\n", pGlyphSet);
__debugbreak();

    return pGlyphSet;
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




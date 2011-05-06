/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         GDI font driver based on freetype
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include "ftfd.h"

FWORD
CalculateAveCharWidth(
    FT_Face ftface)
{
    ULONG cGlyphs = 0, ulAccumCharWidth = 0;
    WCHAR wc;
    FT_UInt index;
    FT_Error fterror;

    /* Loop all glyphs from 'a' to 'z' */
    for (wc = L'a'; wc <= L'z'; wc++)
    {
        /* Load the glyph into the glyph slot */
        fterror = FT_Load_Char(ftface, wc, FT_LOAD_NO_SCALE|FT_LOAD_NO_BITMAP);
        if (fterror) goto allglyphs;

        /* Calculate accumulative char width */
        ulAccumCharWidth += ftface->glyph->metrics.width;
        cGlyphs++;
    }
    goto done;

allglyphs:
    DbgPrint("using all glyphs\n");

    /* Start over */
    ulAccumCharWidth = 0;
    cGlyphs = 0;

    /* Loop all glyphs in the font */
    for (index = 0; index <= (UINT)ftface->num_glyphs; index++)
    {
        /* Load the glyph into the glyph slot */
        fterror = FT_Load_Glyph(ftface, index, FT_LOAD_NO_SCALE|FT_LOAD_NO_BITMAP);
        if (fterror) continue;

        /* Calculate accumulative char width */
        ulAccumCharWidth += ftface->glyph->metrics.width; // fIXME: weighted
        cGlyphs++;
    }

done:
    return (FWORD)(ulAccumCharWidth / cGlyphs);
}

BOOL
NTAPI
FtfdInitIfiMetrics(
    PFTFD_FACE pface)
{
    PFTFD_IFIMETRICS pifiex;
    PIFIMETRICS pifi;
    FT_Face ftface;
    ULONG i;

    DbgPrint("FtfdInitIfiMetrics()\n");

    /* Get the freetype face pointer */
    ftface = pface->ftface;

    /* Fill IFIMETRICS */
    pifiex = &pface->ifiex;
    pifi = &pface->ifiex.ifi;
    pifi->cjThis = sizeof(FTFD_IFIMETRICS);
    pifi->cjIfiExtra = 0;

    /* Set relative offsets */
    pifi->dpwszFamilyName = FIELD_OFFSET(FTFD_IFIMETRICS, awcFamilyName);
    pifi->dpwszStyleName = FIELD_OFFSET(FTFD_IFIMETRICS, awcStyleName);
    pifi->dpwszFaceName = FIELD_OFFSET(FTFD_IFIMETRICS, awcFaceName);
    pifi->dpwszUniqueName = FIELD_OFFSET(FTFD_IFIMETRICS, awcUniqueName);
    pifi->dpCharSets = FIELD_OFFSET(FTFD_IFIMETRICS, ajCharSet);
    pifi->dpFontSim = 0;

    /* Charsets */
    pifi->jWinCharSet = ANSI_CHARSET;
    pifiex->ajCharSet[0] = pifi->jWinCharSet;
    for (i = 1; i < 16; i++)
    {
        pifiex->ajCharSet[i] = DEFAULT_CHARSET;
    }

    pifi->lEmbedId = 0;
    pifi->lItalicAngle = 0;
    pifi->lCharBias = 0;
    pifi->jWinPitchAndFamily = 0; // FIXME: generic way to get this?

    /* Set flags */
    pifi->flInfo = FM_INFO_RETURNS_BITMAPS | FM_INFO_1BPP | FM_INFO_4BPP;
    if (pface->ulFontFormat == FMT_TYPE1)
        pifi->flInfo |= FM_INFO_TECH_TYPE1;
    if (pface->ulFontFormat == FMT_CFF)
        pifi->flInfo |= FM_INFO_TECH_TYPE1 | FM_INFO_TECH_CFF;
    if (pface->ulFontFormat == FMT_TRUETYPE)
        pifi->flInfo |= FM_INFO_TECH_TRUETYPE;
    else
        pifi->flInfo |= FM_INFO_TECH_OUTLINE_NOT_TRUETYPE;
    if (pface->cRuns > 1)
        pifi->flInfo |= FM_INFO_NOT_CONTIGUOUS;
    if (pface->ulFontFormat != FMT_FNT)
        pifi->flInfo |= /*FM_INFO_RETURNS_OUTLINES |*/ FM_INFO_ARB_XFORMS;
    pifi->flInfo |= FM_INFO_RIGHT_HANDED; // FIXME: how to determine?

    /* Font style */
    pifi->fsSelection = FM_SEL_REGULAR;
    pifi->usWinWeight = FW_REGULAR;
    if (ftface->style_flags & FT_STYLE_FLAG_BOLD)
    {
        pifi->fsSelection &= ~FM_SEL_REGULAR;
        pifi->fsSelection |= FM_SEL_BOLD;
        pifi->usWinWeight = FW_BOLD;
    }
    if (ftface->style_flags & FT_STYLE_FLAG_ITALIC)
    {
        pifi->fsSelection &= ~FM_SEL_REGULAR; // ??? remove it?
        pifi->fsSelection |= FM_SEL_ITALIC;
    }

    pifi->fsType = 0;

    /* Font resolution */
    pifi->fwdUnitsPerEm = ftface->units_per_EM;
    pifi->fwdLowestPPEm = 3; // FIXME

    /* Font metrics */
    pifi->fwdWinAscender = (ftface->ascender * 213) / 170;
    pifi->fwdWinDescender = -(ftface->descender * 213) / 170;
    pifi->fwdMacAscender = ftface->ascender;
    pifi->fwdMacDescender = ftface->descender;
    pifi->fwdMacLineGap = 0;
    pifi->fwdAveCharWidth = 0;
    pifi->fwdTypoAscender = pifi->fwdMacAscender;
    pifi->fwdTypoDescender = pifi->fwdMacDescender;
    pifi->fwdTypoLineGap = 0;
    pifi->fwdMaxCharInc = ftface->max_advance_width;
    pifi->fwdCapHeight = 0;
    pifi->fwdXHeight = 0;
    pifi->fwdSubscriptXSize = 0;
    pifi->fwdSubscriptYSize = 0;
    pifi->fwdSubscriptXOffset = 0;
    pifi->fwdSubscriptYOffset = 0;
    pifi->fwdSuperscriptXSize = 0;
    pifi->fwdSuperscriptYSize = 0;
    pifi->fwdSuperscriptXOffset = 0;
    pifi->fwdSuperscriptYOffset = 0;
    pifi->fwdUnderscoreSize = ftface->underline_thickness;
    pifi->fwdUnderscorePosition = ftface->underline_position; // FIXME: off by 10
    pifi->fwdStrikeoutSize = pifi->fwdUnitsPerEm / 20;
    pifi->fwdStrikeoutPosition = pifi->fwdUnitsPerEm / 4;

    pifi->ptlBaseline.x = 1; // FIXME
    pifi->ptlBaseline.y = 0; // FIXME
    pifi->ptlAspect.x = 0x3e9; // FIXME
    pifi->ptlAspect.y = 0x3e9; // FIXME
    pifi->ptlCaret.x = 0; // FIXME
    pifi->ptlCaret.y = 1; // FIXME

    /* Set the biggest characters bounding box */
    pifi->rclFontBox.left = ftface->bbox.xMin;
    pifi->rclFontBox.right = ftface->bbox.xMax;
    pifi->rclFontBox.top = ftface->bbox.yMax;
    pifi->rclFontBox.bottom = ftface->bbox.yMin;

    /* Special characters */
    pifi->chFirstChar = 0x00;
    pifi->chLastChar = 0xff;
    pifi->chDefaultChar = 0x20;
    pifi->chBreakChar = 0x20;
    //pifi->wcFirstChar = 0;
    //pifi->wcLastChar = 0x00ff;
    pifi->wcDefaultChar = 0x0020;
    pifi->wcBreakChar = 0x0020;

    *(DWORD*)&pifi->achVendId = '0000';
    pifi->cKerningPairs = 0;
    pifi->ulPanoseCulture = FM_PANOSE_CULTURE_LATIN;

    pifi->panose.bFamilyType = PAN_FAMILY_TEXT_DISPLAY;
    pifi->panose.bSerifStyle = PAN_ANY;
    pifi->panose.bWeight = PAN_ANY;
    pifi->panose.bProportion = PAN_ANY;
    pifi->panose.bContrast = PAN_ANY;
    pifi->panose.bStrokeVariation = PAN_ANY;
    pifi->panose.bArmStyle = PAN_ANY;
    pifi->panose.bLetterform = PAN_ANY;
    pifi->panose.bMidline = PAN_ANY;
    pifi->panose.bXHeight = PAN_ANY;

    EngMultiByteToUnicodeN(pifiex->awcFamilyName,
                           LF_FACESIZE,
                           NULL,
                           ftface->family_name,
                           strnlen(ftface->family_name, MAX_PATH));

    EngMultiByteToUnicodeN(pifiex->awcStyleName,
                           LF_FACESIZE,
                           NULL,
                           ftface->style_name,
                           strnlen(ftface->style_name, MAX_PATH));

    EngMultiByteToUnicodeN(pifiex->awcFaceName,
                           LF_FACESIZE,
                           NULL,
                           ftface->family_name,
                           strnlen(ftface->family_name, MAX_PATH));

    /* Use OS/2 TrueType or OpenType tables */
    OtfGetIfiMetrics(pface, pifi);

    if (pifi->fwdAveCharWidth == 0)
        pifi->fwdAveCharWidth = CalculateAveCharWidth(ftface);

    /* Create a unique name */
    wcscpy(pifiex->awcUniqueName, L"1.000;ABCD;");
    wcsncat(pifiex->awcUniqueName, pifiex->awcFamilyName, LF_FACESIZE);
    pifiex->awcUniqueName[0] = L'1'; // + version?
    //pifiex->awcUniqueName[2] = L'0'; // + version?
    EngMultiByteToUnicodeN(pifiex->awcUniqueName + 6,
                           4,
                           NULL,
                           pifi->achVendId,
                           4);


    DbgPrint("Finished with the ifi: %p\n", pifi);
    //__debugbreak();

    return TRUE;
}

PVOID
APIENTRY
FtfdInitGlyphSet(
    PFTFD_FACE pface)
{
    FT_Face ftface = pface->ftface;
    FD_GLYPHSET *pGlyphSet;
    FT_UInt index;
    ULONG i, cRuns, cjSize;
    HGLYPH * phglyphs;
    WCHAR wcCurrent, wcPrev;

    DbgPrint("FtfdInitGlyphSet()\n");

    /* Calculate FD_GLYPHSET size (incl. HGLYPH array!) */
    cjSize = FIELD_OFFSET(FD_GLYPHSET, awcrun)
             + pface->cRuns * sizeof(WCRUN)
             + pface->cMappings * sizeof(HGLYPH);

    /* Allocate the FD_GLYPHSET structure plus an array of HGLYPHs */
    pGlyphSet = EngAllocMem(0, cjSize, TAG_GLYPHSET);
    if (!pGlyphSet)
    {
        DbgPrint("EngAllocMem() failed.\n");
        return NULL;
    }

    /* Get a pointer to the HGLYPH array */
    phglyphs = (PHGLYPH)&pGlyphSet->awcrun[pface->cRuns];

    /* Initialize FD_GLYPHSET */
    pGlyphSet->cjThis = cjSize;
    pGlyphSet->flAccel = 0;
    pGlyphSet->cGlyphsSupported = pface->cMappings;
    pGlyphSet->cRuns = pface->cRuns;

    /* Loop through all character mappings */
    wcPrev = wcCurrent = (WCHAR)FT_Get_First_Char(ftface, &index);
    for (i = 0, cRuns = 0; i < pface->cMappings && index; i++)
    {
        /* Use index as glyph handle */
        phglyphs[i] = (HGLYPH)index;

        /* Check whether we can append the wchar to a run */
        if (wcCurrent == wcPrev + 1)
        {
            /* Append to current WCRUN */
            pGlyphSet->awcrun[cRuns - 1].cGlyphs++;
        }
        else
        {
            /* Add a new WCRUN */
            cRuns++;
            pGlyphSet->awcrun[cRuns - 1].wcLow = wcCurrent;
            pGlyphSet->awcrun[cRuns - 1].cGlyphs = 1;
            pGlyphSet->awcrun[cRuns - 1].phg = &phglyphs[i];
            //DbgPrint("adding new run i=%ld, cRuns=%ld, wc=%x\n", i, cRuns, wcCurrent);
        }

        /* Get the next charcode and index */
        wcPrev = wcCurrent;
        wcCurrent = (WCHAR)FT_Get_Next_Char(ftface, wcCurrent, &index);
    }

    DbgPrint("Done with font tree, %d runs\n", pGlyphSet->cRuns);
    pface->pGlyphSet = pGlyphSet;
    return pGlyphSet;
}

VOID
FtfdInitKerningPairs(
    PFTFD_FACE pface)
{
    //DbgPrint("unimplemented\n");
}

static
ULONG
FtfdGetFontFormat(FT_Face ftface)
{
    const char *pstrFormat;

    /* FreeType only provides a string :-/ */
    pstrFormat = FT_Get_X11_Font_Format(ftface);
    if (strcmp(pstrFormat, "TrueType") == 0) return FMT_TRUETYPE;
    if (strcmp(pstrFormat, "Type 1") == 0) return FMT_TYPE1;
    if (strcmp(pstrFormat, "CFF") == 0) return FMT_CFF;
    if (strcmp(pstrFormat, "Windows FNT") == 0) return FMT_FNT;
    if (strcmp(pstrFormat, "BDF") == 0) return FMT_BDF;
    if (strcmp(pstrFormat, "PCF") == 0) return FMT_PCF;
    if (strcmp(pstrFormat, "Type 42") == 0) return FMT_TYPE42;
    if (strcmp(pstrFormat, "CID Type 1") == 0) return FMT_CIDTYPE1;
    if (strcmp(pstrFormat, "PFR") == 0) return FMT_PFR;
    return FMT_UNKNOWN;
}

FtfdGetFileFormat(
    PFTFD_FILE pfile)
{
    ULONG ulFontFormat = pfile->apface[0]->ulFontFormat;

    if (ulFontFormat == FMT_CFF) return FILEFMT_OTF;
    if (ulFontFormat == FMT_FNT) return FILEFMT_FNT;
    if (ulFontFormat == FMT_TRUETYPE)
    {
        if (*(DWORD*)pfile->pvView == 'OTTO') return FILEFMT_OTF;
        return FILEFMT_TTF;
    }

    __debugbreak();
    return 0;
}

PFTFD_FACE
NTAPI
FtfdCreateFace(
    PFTFD_FILE pfile,
    FT_Face ftface)
{
    PFTFD_FACE pface;
    FT_Error fterror;
    ULONG ulAccumCharWidth = 0;
    WCHAR wcCurrent, wcPrev;
    FT_UInt index;

    pface = EngAllocMem(FL_ZERO_MEMORY, sizeof(FTFD_FACE), 'dftF');
    if (!pface)
    {
        DbgPrint("Couldn't allcate a face\n");
        return NULL;
    }

    pface->pfile = pfile;
    pface->ftface = ftface;
    pface->cGlyphs = ftface->num_glyphs;

    /* Get the font format */
    pface->ulFontFormat = FtfdGetFontFormat(ftface);

    /* Load a unicode charmap */
    fterror = FT_Select_Charmap(ftface, FT_ENCODING_UNICODE);
    if (fterror)
    {
        DbgPrint("ERROR: Could not load unicode charmap\n");
        return NULL;
    }

    /* Start with 0 runs and 0 mappings */
    pface->cMappings = 0;
    pface->cRuns = 0;

    /* Loop through all character mappings */
    wcPrev = wcCurrent = (WCHAR)FT_Get_First_Char(ftface, &index);
    pface->ifiex.ifi.wcFirstChar = wcCurrent;
    while (index)
    {
        /* Count the mapping */
        pface->cMappings++;

        /* If character is not subsequent, count a new run */
        if (wcCurrent != wcPrev + 1) pface->cRuns++;
        wcPrev = wcCurrent;

        /* Get the next charcode and index */
        wcCurrent = (WCHAR)FT_Get_Next_Char(ftface, wcCurrent, &index);
    }

    pface->ifiex.ifi.wcLastChar = wcPrev;

    /* Initialize IFIMETRICS */
    FtfdInitIfiMetrics(pface);

    /* Initialize glyphset */
    FtfdInitGlyphSet(pface);

    /* Initialize kerning pairs */
    FtfdInitKerningPairs(pface);

    return pface;
}

static
VOID
FtfdDestroyFace(
    PFTFD_FACE pface)
{
    /* Cleanup the freetype face */
    FT_Done_Face(pface->ftface);

    /* Free the glyphset structure */
    EngFreeMem(pface->pGlyphSet);

    /* Free the kerning pairs structure */
    if (pface->pKerningPairs) EngFreeMem(pface->pKerningPairs);

    /* Finally free the face structure */
    EngFreeMem(pface);
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
    ULONG cjView, cjSize, cNumFaces, i;
    FT_Error fterror;
    FT_Face ftface;
    PFTFD_FILE pfile = NULL;

    DbgPrint("FtfdLoadFontFile()\n");

    /* Check parameters */
    if (cFiles != 1)
    {
        DbgPrint("ERROR: Only 1 File is allowed, got %ld!\n", cFiles);
        return HFF_INVALID;
    }

    /* Map the font file */
    if (!EngMapFontFileFD(*piFile, (PULONG*)&pvView, &cjView))
    {
        DbgPrint("ERROR: Could not map font file!\n");
        return HFF_INVALID;
    }

    /* Load the first face */
    fterror = FT_New_Memory_Face(gftlibrary, pvView, cjView, 0, &ftface);
    if (fterror)
    {
        /* Failure! */
        DbgPrint("ERROR: No faces found in file\n");
        goto error;
    }

    /* Get number of faces from the first face */
    cNumFaces = ftface->num_faces;

    /* Allocate the file structure */
    cjSize = sizeof(FTFD_FILE) + cNumFaces * sizeof(PVOID);
    pfile = EngAllocMem(0, cjSize, 'dftF');
    if (!pfile)
    {
        /* Failure! */
        DbgPrint("ERROR: EngAllocMem() failed.\n");
        goto error;
    }

    /* Initialize the file structure */
    pfile->cNumFaces = cNumFaces;
    pfile->iFile = *piFile;
    pfile->pvView = *ppvView;
    pfile->cjView = *pcjView;
    pfile->ulFastCheckSum = ulFastCheckSum;

    /* Create a face */
    pfile->apface[0] = FtfdCreateFace(pfile, ftface);
    if (!pfile->apface[0])
    {
        DbgPrint("ERROR: FtfdCreateFace() failed.\n");
        goto error;
    }

    /* Get the file format */
    pfile->ulFileFormat = FtfdGetFileFormat(pfile);

    /* Loop all additional faces in this file */
    for (i = 1; i < cNumFaces; i++)
    {
        /* Load the face */
        fterror = FT_New_Memory_Face(gftlibrary, *ppvView, *pcjView, i, &ftface);
        if (fterror)
        {
            DbgPrint("error\n");
            __debugbreak();
            goto error;
        }

        /* Store the face in the file structure */
        pfile->apface[i] = FtfdCreateFace(pfile, ftface);
    }

    DbgPrint("Success! Returning %ld faces\n", cNumFaces);
    return (ULONG_PTR)pfile;

error:
    if (pfile) EngFreeMem(pfile);

    /* Unmap the file */
    EngUnmapFontFileFD(*piFile);

    /* Failure! */
    return HFF_INVALID;
}

PIFIMETRICS
APIENTRY
FtfdQueryFont(
    IN DHPDEV dhpdev,
    IN ULONG_PTR diFile,
    IN ULONG iFace,
    IN ULONG_PTR *pid)
{
    PFTFD_FILE pfile = (PFTFD_FILE)diFile;
    PFTFD_FACE pface = pfile->apface[iFace - 1];

    DbgPrint("FtfdQueryFont()\n");

    /* Validate parameters */
    if (iFace > pfile->cNumFaces || !pid)
    {
        DbgPrint("ERROR: iFace > pfile->cNumFaces || !pid\n");
        return NULL;
    }

    /* Nothing to free */
    *pid = 0;

    /* Return pointer to the IFIMETRICS */
    return &pface->ifiex.ifi;
}

PVOID
APIENTRY
FtfdQueryFontTree(
    DHPDEV dhpdev,
    ULONG_PTR diFile,
    ULONG iFace,
    ULONG iMode,
    ULONG_PTR *pid)
{
    PFTFD_FILE pfile = (PFTFD_FILE)diFile;
    PFTFD_FACE pface;

    DbgPrint("FtfdQueryFontTree(iMode=%ld)\n", iMode);

    /* Validate parameters */
    if (iFace > pfile->cNumFaces || !pid)
    {
        DbgPrint("ERROR: iFace > pfile->cNumFaces || !pid\n");
        return NULL;
    }

    /* get pointer to the requested face */
    pface = pfile->apface[iFace - 1];

    switch (iMode)
    {
        case QFT_GLYPHSET:
            *pid = 0;
            return pface->pGlyphSet;

        case QFT_KERNPAIRS:
            *pid = 0;
            return pface->pKerningPairs;

        default:
            DbgPrint("ERROR: invalid iMode: %ld\n", iMode);
    }

    return NULL;
}

BOOL
APIENTRY
FtfdUnloadFontFile(
    IN ULONG_PTR diFile)
{
    PFTFD_FILE pfile = (PFTFD_FILE)diFile;
    ULONG i;

    DbgPrint("FtfdUnloadFontFile()\n");

    /* Cleanup faces */
    for (i = 0; i < pfile->cNumFaces; i++)
    {
        FtfdDestroyFace(pfile->apface[i]);
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
    ULONG_PTR diFile,
    ULONG ulMode,
    ULONG cjBuf,
    ULONG *pulBuf)
{
    PFTFD_FILE pfile = (PFTFD_FILE)diFile;

    DbgPrint("FtfdQueryFontFile(ulMode=%ld)\n", ulMode);

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

    /* We only support bitmaps for now */
    pulCaps[0] = 2;
    pulCaps[1] = QC_1BIT | QC_4BIT;

    return 2;
}

LONG
APIENTRY
FtfdQueryTrueTypeTable(
    ULONG_PTR diFile,
    ULONG ulFont,
    ULONG ulTag,
    PTRDIFF dpStart,
    ULONG cjBuf,
    BYTE *pjBuf,
    PBYTE *ppjTable,
    ULONG *pcjTable)
{
    PFTFD_FILE pfile = (PFTFD_FILE)diFile;
    PBYTE pjTable;
    ULONG cjTable;

    DbgPrint("FtfdQueryTrueTypeTable\n");

    /* Check if this file supports TrueType tables */
    if (pfile->ulFileFormat != FILEFMT_TTF &&
        pfile->ulFileFormat != FILEFMT_OTF)
    {
        DbgPrint("File format doesn't support true type tables\n");
        return FD_ERROR;
    }

    // FIXME: handle ulFont

    /* Check if the whole file is requested */
    if (ulTag == 0)
    {
        /* Requested the whole file */
        pjTable = pfile->pvView;
        cjTable = pfile->cjView;
    }
    else
    {
        /* Search for the table */
        pjTable = OtfFindTable(pfile->pvView, pfile->cjView, ulTag, &cjTable);
        if (!pjTable)
        {
            DbgPrint("Couldn't find table '%.4s'\n", (char*)&ulTag);
            return FD_ERROR;
        }
    }

    /* Return requested pointers */
    if (ppjTable) *ppjTable = pjTable;
    if (pcjTable) *pcjTable = cjTable;

    /* Check if we shall copy data */
    if (pjBuf)
    {
        /* Check if the offset is inside the table */
        if (dpStart < 0 || (ULONG_PTR)dpStart >= cjTable)
        {
            DbgPrint("dpStart outside the table: %p\n", dpStart);
            return FD_ERROR;
        }

        /* Don't copy beyond the table end */
        cjTable -= dpStart;

        /* Don't copy more then the buffer can hold */
        if (cjBuf < cjTable) cjTable = cjBuf;

        /* Copy the data to the buffer */
        RtlCopyMemory(pjBuf, pjTable + dpStart, cjTable);
    }

    return cjTable;
}

PVOID
APIENTRY
FtfdGetTrueTypeFile(
    ULONG_PTR diFile,
    ULONG *pcj)
{
    PFTFD_FILE pfile = (PFTFD_FILE)diFile;

    DbgPrint("FtfdGetTrueTypeFile\n");

    /* Check if this file is TrueType */
    if (pfile->ulFileFormat != FILEFMT_TTF &&
        pfile->ulFileFormat != FILEFMT_OTF)
    {
        DbgPrint("File format is not TrueType or Opentype\n");
        return NULL;
    }

    /* Return the pointer and size */
    if (pcj) *pcj = pfile->cjView;
    return pfile->pvView;
}

#if 0 // not needed atm
VOID
APIENTRY
FtfdFree(
    PVOID pv,
    ULONG_PTR id)
{
    DbgPrint("FtfdFree()\n");
    EngFreeMem(pv);
}
#endif



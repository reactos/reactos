/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         GDI font driver based on freetype
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include "ftfd.h"

static
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
        if (fterror) break;

        /* Calculate accumulative char width */
        ulAccumCharWidth += ftface->glyph->metrics.width;
        cGlyphs++;
    }

    /* Check if an error occured */
    if (wc <= L'z')
    {
        TRACE("using all glyphs\n");

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
            ulAccumCharWidth += ftface->glyph->metrics.width; // FIXME: weighted
            cGlyphs++;
        }
    }

    /* Return the average glyph width */
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
    FT_Error fterror;
    PS_FontInfoRec fontinfo;
    ULONG i;

    TRACE("FtfdInitIfiMetrics()\n");

    /* Get the freetype face pointer */
    ftface = pface->ftface;

    /* Init header */
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

    /* Initialize charsets */
    pifi->jWinCharSet = ANSI_CHARSET;
    pifiex->ajCharSet[0] = pifi->jWinCharSet;
    for (i = 1; i < 16; i++)
    {
        pifiex->ajCharSet[i] = DEFAULT_CHARSET;
    }

    pifi->lEmbedId = 0;
    pifi->lCharBias = 0;

    /* Feature flags */
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

    /* Font resolution */
    pifi->fwdUnitsPerEm = ftface->units_per_EM;
    pifi->fwdLowestPPEm = 3;

    /* Font metrics */
    pifi->fwdMacAscender = ftface->ascender;
    pifi->fwdMacDescender = ftface->descender;
    pifi->fwdMacLineGap = 0;
    pifi->fwdMaxCharInc = ftface->max_advance_width;
    pifi->fwdUnderscoreSize = ftface->underline_thickness;
    pifi->fwdUnderscorePosition = ftface->underline_position;

    pifi->ptlBaseline.x = 1; // FIXME
    pifi->ptlBaseline.y = 0; // FIXME
    pifi->ptlAspect.x = 1;
    pifi->ptlAspect.y = 1;
    pifi->ptlCaret.x = 0; // FIXME
    pifi->ptlCaret.y = 1; // FIXME

    /* Set the biggest characters bounding box */
    pifi->rclFontBox.left = ftface->bbox.xMin;
    pifi->rclFontBox.right = ftface->bbox.xMax;
    pifi->rclFontBox.top = ftface->bbox.yMax;
    pifi->rclFontBox.bottom = ftface->bbox.yMin;

    pifi->cKerningPairs = 0;
    pifi->ulPanoseCulture = FM_PANOSE_CULTURE_LATIN;

    /* Try to get OS/2 TrueType or OpenType metrics */
    if (!FtfdGetWinMetrics(pface, pifi))
    {
        /* No success, use fallback */

        /* Font style flags */
        pifi->fsType = 0;
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
            pifi->fsSelection &= ~FM_SEL_REGULAR;
            pifi->fsSelection |= FM_SEL_ITALIC;
        }

        /* Metrics */
        pifi->fwdWinAscender = (ftface->ascender * 213) / 170;
        pifi->fwdWinDescender = -(ftface->descender * 213) / 170;
        pifi->fwdTypoAscender = ftface->ascender;
        pifi->fwdTypoDescender = ftface->descender;
        pifi->fwdTypoLineGap = pifi->fwdUnitsPerEm / 10;
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
        pifi->fwdStrikeoutSize = pifi->fwdUnderscoreSize;
        pifi->fwdStrikeoutPosition = pifi->fwdMacAscender / 3;
        pifi->fwdAveCharWidth = CalculateAveCharWidth(ftface);

        /* Special characters (first and last char are already enumerated) */
        pifi->wcDefaultChar = 0x0020;
        pifi->wcBreakChar = 0x0020;

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

        *(DWORD*)&pifi->achVendId = 'nknU';
    }

    /* Try to get type1 info from freetype */
    fterror = FT_Get_PS_Font_Info(pface->ftface, &fontinfo);
    if (fterror == 0)
    {
        /* Set italic angle */
        pifi->lItalicAngle = fontinfo.italic_angle;
    }
    else
    {
        /* Set fallback values */
        pifi->lItalicAngle = 0;
    }

    /* Get the win family */
    if (pifi->panose.bFamilyType == PAN_FAMILY_SCRIPT)
        pifi->jWinPitchAndFamily = FF_SCRIPT;
    else if (pifi->panose.bFamilyType == PAN_FAMILY_DECORATIVE)
        pifi->jWinPitchAndFamily = FF_DECORATIVE;
    else if (pifi->panose.bProportion == PAN_PROP_MODERN)
        pifi->jWinPitchAndFamily = FF_MODERN;
    else if (pifi->panose.bSerifStyle <= PAN_SERIF_ROUNDED)
        pifi->jWinPitchAndFamily = FF_SWISS;
    else
        pifi->jWinPitchAndFamily = FF_ROMAN;

    /* Set pitch */
    pifi->jWinPitchAndFamily |= FT_IS_FIXED_WIDTH(ftface) ? FIXED_PITCH :
                                                            VARIABLE_PITCH;

    /* Convert the special characters from unicode to ansi */
    EngUnicodeToMultiByteN(&pifi->chFirstChar, 4, NULL, &pifi->wcFirstChar, 8);

    /* This one seems to be hardcoded to 0xff */
    pifi->chLastChar = 0xff;

    /* Convert names to unicode */
    EngMultiByteToUnicodeN(pifiex->awcFamilyName,
                           sizeof(pifiex->awcFamilyName),
                           NULL,
                           ftface->family_name,
                           strnlen(ftface->family_name, MAX_PATH));

    EngMultiByteToUnicodeN(pifiex->awcStyleName,
                           sizeof(pifiex->awcStyleName),
                           NULL,
                           ftface->style_name,
                           strnlen(ftface->style_name, MAX_PATH));

    EngMultiByteToUnicodeN(pifiex->awcFaceName,
                           sizeof(pifiex->awcFaceName),
                           NULL,
                           ftface->family_name,
                           strnlen(ftface->family_name, MAX_PATH));

    /* Create a unique name */
    wcscpy(pifiex->awcUniqueName, L"1.000;????;");
    wcsncat(pifiex->awcUniqueName, pifiex->awcFamilyName, LF_FACESIZE);
    pifiex->awcUniqueName[0] = L'0' +  HIWORD(pface->ulFontRevision) % 10;
    pifiex->awcUniqueName[2] = L'0' + (LOWORD(pface->ulFontRevision) / 100) % 10;
    pifiex->awcUniqueName[3] = L'0' + (LOWORD(pface->ulFontRevision) / 10) % 10;
    pifiex->awcUniqueName[4] = L'0' +  LOWORD(pface->ulFontRevision) % 10;
    EngMultiByteToUnicodeN(pifiex->awcUniqueName+6, 8, NULL, pifi->achVendId, 4);

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
    PWCHAR pwcReverseTable;

    TRACE("FtfdInitGlyphSet()\n");

    /* Allocate an array of WCHARs */
    cjSize = pface->cGlyphs * sizeof(WCHAR);
    pwcReverseTable = EngAllocMem(0, cjSize, 'dftF');
    if (!pwcReverseTable)
    {
        WARN("EngAllocMem() failed.\n");
        return NULL;
    }

    /* Calculate FD_GLYPHSET size (incl. HGLYPH array!) */
    cjSize = FIELD_OFFSET(FD_GLYPHSET, awcrun)
             + pface->cRuns * sizeof(WCRUN)
             + pface->cMappings * sizeof(HGLYPH);

    /* Allocate the FD_GLYPHSET structure plus an array of HGLYPHs */
    pGlyphSet = EngAllocMem(0, cjSize, TAG_GLYPHSET);
    if (!pGlyphSet)
    {
        WARN("EngAllocMem() failed.\n");
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
        /* Create an entry in the reverse lookup table */
        ASSERT(index < pface->cGlyphs);
        pwcReverseTable[index] = wcCurrent;

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
        }

        /* Get the next charcode and index */
        wcPrev = wcCurrent;
        wcCurrent = (WCHAR)FT_Get_Next_Char(ftface, wcCurrent, &index);
    }

    TRACE("Done with font tree, %d runs\n", pGlyphSet->cRuns);
    pface->pGlyphSet = pGlyphSet;
    pface->pwcReverseTable = pwcReverseTable;
    return pGlyphSet;
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

static
ULONG
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
    ULONG iFace,
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
        WARN("Couldn't allcate a face\n");
        return NULL;
    }

    /* Set basic fields */
    pface->pfile = pfile;
    pface->iFace = iFace;
    pface->ftface = ftface;
    pface->cGlyphs = ftface->num_glyphs;

    /* Get the font format */
    pface->ulFontFormat = FtfdGetFontFormat(ftface);

    /* Load a unicode charmap */
    fterror = FT_Select_Charmap(ftface, FT_ENCODING_UNICODE);
    if (fterror)
    {
        WARN("Could not load unicode charmap\n");
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

    /* Save the last character */
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

    TRACE("FtfdLoadFontFile()\n");
//__debugbreak();
//return 0;

    /* Check parameters */
    if (cFiles != 1)
    {
        WARN("Only 1 File is allowed, got %ld!\n", cFiles);
        return HFF_INVALID;
    }

    /* Map the font file */
    if (!EngMapFontFileFD(*piFile, (PULONG*)&pvView, &cjView))
    {
        WARN("Could not map font file!\n");
        return HFF_INVALID;
    }

    /* Load the first face */
    fterror = FT_New_Memory_Face(gftlibrary, pvView, cjView, 0, &ftface);
    if (fterror)
    {
        /* Failure! */
        WARN("No faces found in file\n");
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
        WARN("EngAllocMem() failed.\n");
        goto error;
    }

    /* Initialize the file structure */
    pfile->cNumFaces = cNumFaces;
    pfile->iFile = *piFile;
    pfile->pvView = *ppvView;
    pfile->cjView = *pcjView;
    pfile->ulFastCheckSum = ulFastCheckSum;

    /* Create a face */
    pfile->apface[0] = FtfdCreateFace(pfile, 1, ftface);
    if (!pfile->apface[0])
    {
        WARN("FtfdCreateFace() failed.\n");
        goto error;
    }

    /* Get the file format */
    pfile->ulFileFormat = FtfdGetFileFormat(pfile);

    /* Check for design vector */
    if (pdv)
    {
        /* Check if the font format supports it */
        if (pfile->apface[0]->ulFontFormat != FMT_TYPE1)
        {
            WARN("Design vector is not supported\n");
            goto error;
        }

        /* Verify the design vector, just in case ... */
        if (pdv->dvReserved != STAMP_DESIGNVECTOR ||
            pdv->dvNumAxes > MM_MAX_NUMAXES)
        {
            WARN("Design vector is invalid\n");
            goto error;
        }

        /* Copy design vector */
        pfile->dv = *pdv;
    }
    else
    {
        /* Mark as not present */
        pfile->dv.dvReserved = 0;
    }

    /* Loop all additional faces in this file */
    for (i = 1; i < cNumFaces; i++)
    {
        /* Load the face */
        fterror = FT_New_Memory_Face(gftlibrary, *ppvView, *pcjView, i, &ftface);
        if (fterror)
        {
            WARN("error\n");
            __debugbreak();
            goto error;
        }

        /* Store the face in the file structure */
        pfile->apface[i] = FtfdCreateFace(pfile, i + 1, ftface);
    }

    TRACE("Success! Returning %ld faces\n", cNumFaces);
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

    TRACE("FtfdQueryFont()\n");

    /* Validate parameters */
    if (iFace > pfile->cNumFaces || !pid)
    {
        WARN("iFace > pfile->cNumFaces || !pid\n");
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

    TRACE("FtfdQueryFontTree(iMode=%ld)\n", iMode);

    /* Validate parameters */
    if (iFace > pfile->cNumFaces || !pid)
    {
        WARN("iFace > pfile->cNumFaces || !pid\n");
        return NULL;
    }

    /* Get pointer to the requested face */
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
            WARN("Invalid iMode: %ld\n", iMode);
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

    TRACE("FtfdUnloadFontFile()\n");

    /* Cleanup faces */
    for (i = 0; i < pfile->cNumFaces; i++)
    {
        FtfdDestroyFace(pfile->apface[i]);
    }

    /* Unmap the font file */
    EngUnmapFontFileFD(pfile->iFile);

    /* Free the memory that was allocated for the file */
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

    TRACE("FtfdQueryFontFile(ulMode=%ld)\n", ulMode);

    switch (ulMode)
    {
        case QFF_DESCRIPTION:
        {
            WARN("QFF_DESCRIPTION unimplemented\n");
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
    TRACE("FtfdQueryFontCaps()\n");

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


#if 0 // not needed atm
VOID
APIENTRY
FtfdFree(
    PVOID pv,
    ULONG_PTR id)
{
    TRACE("FtfdFree()\n");
    EngFreeMem(pv);
}
#endif



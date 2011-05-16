/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         OpenType support for GDI font driver based on freetype
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 * REFERENCES:      http://www.microsoft.com/typography/otspec/
 *                  http://www.microsoft.com/typography/otspec/os2.htm
 */

#include "ftfd.h"
#include "tttables.h"

ULONG test_GETD(PVOID pv)
{
    PUSHORT pus = pv;
    return (ULONG)_byteswap_ushort(pus[0]) << 16 | _byteswap_ushort(pus[1]);
}

static
ULONG
CalcTableChecksum(PVOID pvTable, ULONG cjTable)
{
    PULONG pul = pvTable, pulEnd;
    ULONG ulCheckSum = 0L;
    ASSERT(!((ULONG_PTR)pvTable & 3));

    pulEnd = (PULONG)pvTable + (cjTable + 3) / sizeof(ULONG);

    while (pul < pulEnd) ulCheckSum += GETD(pul++);

    return ulCheckSum;
}

/*! \name FtfdFindTrueTypeTable
 *  \brief Searches for a specific table in TrueType and OpenType font files
 *  \param pvView - The address where the font file is mapped
 *  \param cjView - Size of the mapped font file
 *  \param iFace - 1-based index of the font in the font file.
 *  \param ulTag - Identifier tag of the table to search
 *  \param pulLength - Pointer to an ULONG that recieves the table length,
 *                     Can be NULL.
 *  \return Pointer to the table if successful, NULL if unsuccessful.
 */
PVOID
NTAPI
FtfdFindTrueTypeTable(
    PVOID pvView,
    ULONG cjView,
    ULONG ulFont,
    ULONG ulTag,
    PULONG pulLength)
{
    PTT_FILE_HEADER pFontHeader;
    PTT_COLLECTION pCollection;
    ULONG i, ulOffset, ulLength, ulNumTables, ulCheckSum;
    ASSERT(ulFont > 0);

    /* Check if this is a font collection */
    pCollection = pvView;
    if (pCollection->ulTTCTag == 'fctt')
    {
        /* Check if we have enough fonts in the file */
        if (ulFont > GETD(&pCollection->ulNumFonts))
        {
            WARN("ulFont too big: %ld\n", ulFont);
            return NULL;
        }

        /* Get the offset of the font we want */
        ulOffset = GETD(&pCollection->aulOffsetTable[ulFont - 1]);
        if (ulOffset >= cjView)
        {
            WARN("Font %ld is not inside the mapped region\n", ulFont);
            return NULL;
        }

        /* Update the font position and remaining view size */
        pvView = (PUCHAR)pvView + ulOffset;
        cjView -= ulOffset;
    }
    else if (ulFont > 1)
    {
        // Shouldn't happen
        __debugbreak();
    }

    /* Verify the font header */
    pFontHeader = pvView;
    if (pFontHeader->ulIdentifier != 'OTTO' &&
        pFontHeader->ulIdentifier != 0x00000100)
    {
        WARN("Bad font header: 0x%lx\n",
                 pFontHeader->ulIdentifier);
        return NULL;
    }

    /* Check if number of tables is ok */
    ulNumTables = GETW(&pFontHeader->usNumTables);
    ulLength = ulNumTables * sizeof(TT_TABLE_ENTRY);
    if (ulLength + sizeof(TT_FILE_HEADER) > cjView)
    {
        WARN("Too many tables (%ld)\n", ulNumTables);
        return NULL;
    }

    /* Loop all tables */
    for (i = 0; i < ulNumTables; i++)
    {
        /* Compare the tag */
        if (pFontHeader->aTableEntries[i].ulTag == ulTag)
        {
            /* Get table offset and length */
            ulOffset = GETD(&pFontHeader->aTableEntries[i].ulOffset);
            ulLength = GETD(&pFontHeader->aTableEntries[i].ulLength);

            /* Check if this is inside the file */
            if (ulOffset + ulLength > cjView ||
                ulOffset + ulLength < ulOffset)
            {
                WARN("Invalid table entry. %ld, %ld \n", ulOffset, ulLength);
                return NULL;
            }

            /* Check the table's checksum */
            ulCheckSum = CalcTableChecksum((PUCHAR)pvView + ulOffset, ulLength);
            if (ulCheckSum != GETD(&pFontHeader->aTableEntries[i].ulCheckSum))
            {
                WARN("Checksum mitmatch! %ld, %ld \n", ulOffset, ulLength);
                return NULL;
            }

            /* Return size and pointer to the table */
            if (pulLength) *pulLength = ulLength;
            return (PUCHAR)pvView + ulOffset;
        }
    }

    /* Not found */
    return NULL;
}

/*! \name GetWinFamily
 *  \brief Translates IBM font class IDs into a Windows family bitfield
 *  \param jClassId
 *  \param jSubclassId
 *  \ref http://www.microsoft.com/typography/otspec/ibmfc.htm
 */
static
BYTE
GetWinFamily(BYTE jClassId, BYTE jSubclassId)
{
    switch (jClassId)
    {
        case 0: // Class ID = 0 No Classification
            return FF_ROMAN;//FF_SWISS;

        case 1: // Class ID = 1 Oldstyle Serifs
            switch (jSubclassId)
            {
                case 0: // Subclass ID = 0 : No Classification
                case 1: // Subclass ID = 1 : IBM Rounded Legibility
                case 2: // Subclass ID = 2 : Garalde
                case 3: // Subclass ID = 3 : Venetian
                case 4: // Subclass ID = 4 : Modified Venetian
                case 5: // Subclass ID = 5 : Dutch Modern
                case 6: // Subclass ID = 6 : Dutch Traditional
                case 7: // Subclass ID = 7 : Contemporary
                case 8: // Subclass ID = 8 : Calligraphic
                case 15: // Subclass ID = 15 : Miscellaneous
                default: // Subclass ID = 9-14 : (reserved for future use)
                    break;
            }

        case 2: // Class ID = 2 Transitional Serifs
            switch (jSubclassId)
            {
                case 15: return FF_ROMAN; // 15: Miscellaneous
                case 0: // Subclass ID = 0 : No Classification
                case 1: // Subclass ID = 1 : Direct Line
                case 2: // Subclass ID = 2 : Script
                default: // Subclass ID = 3-14 : (reserved for future use)
                    break;
            }

        case 3: // Class ID = 3 Modern Serifs
            switch (jSubclassId)
            {
                case 0: // Subclass ID = 0 : No Classification
                case 1: // Subclass ID = 1 : Italian
                case 2: // Subclass ID = 2 : Script
                case 15: // Subclass ID = 15 : Miscellaneous
                default: // Subclass ID = 3-14 : (reserved for future use)
                    break;
            }

        case 4: // Class ID = 4 Clarendon Serifs
            switch (jSubclassId)
            {
                case 0: // Subclass ID = 0 : No Classification
                case 1: // Subclass ID = 1 : Clarendon
                case 2: // Subclass ID = 2 : Modern
                case 3: // Subclass ID = 3 : Traditional
                case 4: // Subclass ID = 4 : Newspaper
                case 5: // Subclass ID = 5 : Stub Serif
                case 6: // Subclass ID = 6 : Monotone
                case 7: // Subclass ID = 7 : Typewriter
                case 15: // Subclass ID = 15 : Miscellaneous
                default: // Subclass ID = 8-14: (reserved for future use)
                    break;
            }

        case 5: // Class ID = 5 Slab Serifs
            switch (jSubclassId)
            {
                case 0: // Subclass ID = 0 : No Classification
                case 1: // Subclass ID = 1 : Monotone
                case 2: // Subclass ID = 2 : Humanist
                case 3: // Subclass ID = 3 : Geometric
                case 4: // Subclass ID = 4 : Swiss
                case 5: // Subclass ID = 5 : Typewriter
                case 15: // Subclass ID = 15 : Miscellaneous
                default: // Subclass ID = 6-14 : (reserved for future use)
                    break;
            }

        case 7: // Class ID = 7 Freeform Serifs
            switch (jSubclassId)
            {
                case 0: // Subclass ID = 0 : No Classification
                case 1: // Subclass ID = 1 : Modern
                case 15: // Subclass ID = 15 : Miscellaneous
                default: // Subclass ID = 2-14 : (reserved for future use)
                    break;
            }

        case 8: // Class ID = 8 Sans Serif
            switch (jSubclassId)
            {
                case 0: return FF_SWISS; // 0: No Classification
                case 5: return FF_SWISS; // 5: Neo-grotesque Gothic
                case 15: return FF_SWISS|FF_ROMAN; // 15: Miscellaneous

                case 1: // Subclass ID = 1 : IBM Neo-grotesque Gothic
                case 2: // Subclass ID = 2 : Humanist
                case 3: // Subclass ID = 3 : Low-x Round Geometric
                case 4: // Subclass ID = 4 : High-x Round Geometric
                case 6: // Subclass ID = 6 : Modified Neo-grotesque Gothic
                case 9: // Subclass ID = 9 : Typewriter Gothic
                case 10: // Subclass ID = 10 : Matrix
                default: // Subclass ID = 7-8, 11-14 : (reserved for future use)
                    break;
            }

        case 9: // Class ID = 9 Ornamentals
            switch (jSubclassId)
            {
                case 0: // Subclass ID = 0 : No Classification
                case 1: // Subclass ID = 1 : Engraver
                case 2: // Subclass ID = 2 : Black Letter
                case 3: // Subclass ID = 3 : Decorative
                case 4: // Subclass ID = 4 : Three Dimensional
                case 15: // Subclass ID = 15 : Miscellaneous
                default: // Subclass ID = 5-14 : (reserved for future use)
                    break;
            }

        case 10: // Class ID = 10 Scripts
            switch (jSubclassId)
            {
                case 0: // Subclass ID = 0 : No Classification
                case 1: // Subclass ID = 1 : Uncial
                case 2: // Subclass ID = 2 : Brush Joined
                case 3: // Subclass ID = 3 : Formal Joined
                case 4: // Subclass ID = 4 : Monotone Joined
                case 5: // Subclass ID = 5 : Calligraphic
                case 6: // Subclass ID = 6 : Brush Unjoined
                case 7: // Subclass ID = 7 : Formal Unjoined
                case 8: // Subclass ID = 8 : Monotone Unjoined
                case 15: // Subclass ID = 15 : Miscellaneous
                default: // Subclass ID = 9-14 : (reserved for future use)
                    break;
            }

        case 12: // Class ID = 12 Symbolic
            switch (jSubclassId)
            {
                case 0: // Subclass ID = 0 : No Classification
                case 3: // Subclass ID = 3 : Mixed Serif
                case 6: // Subclass ID = 6 : Oldstyle Serif
                case 7: // Subclass ID = 7 : Neo-grotesque Sans Serif
                case 15: // Subclass ID = 15 : Miscellaneous
                default: // Subclass ID = 1-2,4-5,8-14  : (reserved for future use)
                    break;
            }

        case 13: // Class ID = 13 Reserved
        case 14: // Class ID = 14 Reserved
        default: // Class ID = 6,11 (reserved for future use)
            break;
    }

    WARN("Unhandled class: jClassId=%d, jSubclassId=%d\n", jClassId, jSubclassId);
//__debugbreak();
    return FF_SWISS;
}

BOOL
NTAPI
FtfdGetWinMetrics(
    PFTFD_FACE pface,
    PIFIMETRICS pifi)
{
    PFTFD_FILE pfile = pface->pfile;
    PVOID pvView = pfile->pvView;
    PTT_TABLE_OS2 pOs2;
    PTT_TABLE_HEAD pHead;

    /* Get the head table for the face */
    pHead = FtfdFindTrueTypeTable(pvView, pfile->cjView, pface->iFace, 'daeh', NULL);
    if (!pHead)
    {
        WARN("Couldn't find 'head' table\n");
        return FALSE;
    }

    /* Get lowest PPEm */
    pifi->fwdLowestPPEm = GETW(&pHead->lowestRecPPEM);
    pface->ulFontRevision = GETD(&pHead->fontRevision);

    /* Get the OS/2 table for the face */
    pOs2 = FtfdFindTrueTypeTable(pvView, pfile->cjView, pface->iFace, '2/SO', NULL);
    if (!pOs2)
    {
        WARN("Couldn't find 'OS/2' table\n");
        return FALSE;
    }

    //pifi->lEmbedId;
    //pifi->lCharBias;
    //pifi->jWinCharSet;
    pifi->jWinPitchAndFamily |= GetWinFamily(pOs2->jClassId, pOs2->jSubClassId);
    pifi->usWinWeight = GETW(&pOs2->usWeightClass);
    pifi->fsSelection = GETW(&pOs2->fsSelection);
    pifi->fsType = GETW(&pOs2->fsType);
    pifi->fwdWinAscender = GETW(&pOs2->usWinAscent);
    pifi->fwdWinDescender = GETW(&pOs2->usWinDescent);
    //pifi->fwdMacAscender;
    //pifi->fwdMacDescender;
    //pifi->fwdMacLineGap;
    pifi->fwdTypoAscender = GETW(&pOs2->sTypoAscender);
    pifi->fwdTypoDescender = GETW(&pOs2->sTypoDescender);
    pifi->fwdTypoLineGap = GETW(&pOs2->sTypoLineGap);
    pifi->fwdAveCharWidth = GETW(&pOs2->xAvgCharWidth);
    pifi->fwdCapHeight = GETW(&pOs2->sCapHeight);
    pifi->fwdXHeight = GETW(&pOs2->sxHeight);
    pifi->fwdSubscriptXSize = GETW(&pOs2->ySubscriptXSize);
    pifi->fwdSubscriptYSize = GETW(&pOs2->ySubscriptYSize);
    pifi->fwdSubscriptXOffset = GETW(&pOs2->ySubscriptXOffset);
    pifi->fwdSubscriptYOffset = GETW(&pOs2->ySubscriptYOffset);
    pifi->fwdSuperscriptXSize = GETW(&pOs2->ySuperscriptXSize);
    pifi->fwdSuperscriptYSize = GETW(&pOs2->ySuperscriptYSize);
    pifi->fwdSuperscriptXOffset = GETW(&pOs2->ySuperscriptXOffset);
    pifi->fwdSuperscriptYOffset = GETW(&pOs2->ySuperscriptYOffset);
    //pifi->fwdUnderscoreSize;
    //pifi->fwdUnderscorePosition;
    pifi->fwdStrikeoutSize = GETW(&pOs2->yStrikeoutSize);
    pifi->fwdStrikeoutPosition = GETW(&pOs2->yStrikeoutPosition);
    pifi->wcFirstChar = GETW(&pOs2->usFirstCharIndex);
    pifi->wcLastChar = GETW(&pOs2->usLastCharIndex);
    pifi->wcDefaultChar = GETW(&pOs2->usDefaultChar);
    pifi->wcBreakChar = GETW(&pOs2->usBreakChar);
    *(DWORD*)pifi->achVendId = *(DWORD*)pOs2->achVendID;
    //pifi->ulPanoseCulture;
    pifi->panose = *(PANOSE*)pOs2->panose;

    return TRUE;
}

INT
__cdecl
CompareKernPair(
    FD_KERNINGPAIR *pkp1,
    FD_KERNINGPAIR *pkp2)
{
    ULONG ul1, ul2;
    ul1 = pkp1->wcFirst + 65536 * pkp1->wcSecond;
    ul2 = pkp2->wcFirst + 65536 * pkp2->wcSecond;
    if (ul1 < ul2) return -1;
    if (ul1 > ul2) return 1;
    return 0;
}


VOID
NTAPI
FtfdInitKerningPairs(
    PFTFD_FACE pface)
{
    PFTFD_FILE pfile = pface->pfile;
    PTT_KERNING_TABLE pKernTable;
    PTT_KERNING_SUBTABLE pSubTable;
    ULONG cjSize, i, j, cPairs = 0;
    FD_KERNINGPAIR *pKernPair;
    HGLYPH hgLeft, hgRight;
    USHORT nTables;
    ULONG_PTR ulLastAddress;

//__debugbreak();

    /* Get the kern table for the face */
    pKernTable = FtfdFindTrueTypeTable(pfile->pvView,
                                       pfile->cjView,
                                       pface->iFace,
                                       'nrek',
                                       &cjSize);

    if (!pKernTable || cjSize < sizeof(TT_KERNING_TABLE) ||
         pKernTable->usVersion != 0)
    {
        TRACE("Couldn't find a valid kerning table\n");
        return;
    }

    nTables = GETW(&pKernTable->nTables);
    ulLastAddress = (ULONG_PTR)pKernTable + cjSize;


    /* Loop all subtables */
    pSubTable = &pKernTable->subtable;
    for (i = 0; i < nTables; i++)
    {
        /* Check if the subtable is accessible */
        if ((ULONG_PTR)pSubTable + sizeof(TT_KERNING_SUBTABLE) > ulLastAddress)
        {
            __debugbreak();
            return;
        }

        /* Get the table size and check if its valid */
        cjSize = GETW(&pSubTable->usLength);
        if ((ULONG_PTR)pSubTable + cjSize > ulLastAddress)
        {
            __debugbreak();
            return;
        }

        /* Check version */
        if (GETW(&pSubTable->usVersion) == 0)
        {
            /* Get number of kerning pairs and check id its valid */
            cPairs += GETW(&pSubTable->format0.nPairs);
            if ((ULONG_PTR)&pSubTable->format0.akernpair[cPairs] > ulLastAddress)
            {
                __debugbreak();
                return;
            }
        }

        /* Go to next subtable */
        pSubTable = (PVOID)((PCHAR)pSubTable + cjSize);
    }

    if (cPairs == 0)
    {
        return;
    }

    /* Allocate an FD_KERNINGPAIR array */
    pKernPair = EngAllocMem(0, (cPairs + 1) * sizeof(FD_KERNINGPAIR), 'dftF');
    pface->pKerningPairs = pKernPair;
    if (!pKernPair)
    {
        WARN("EngAllocMem failed\n");
        return;
    }

    /* Loop all subtables again */
    pSubTable = &pKernTable->subtable;
    for (i = 0; i < nTables; i++)
    {
        /* Check version */
        if (GETW(&pSubTable->usVersion) == 0)
        {
            /* Loop all kern pairs in the table */
            for (j = 0; j < GETW(&pSubTable->format0.nPairs); j++)
            {
                /* Get the glyph handles for the kerning */
                hgLeft = GETW(&pSubTable->format0.akernpair[j].usLeft);
                hgRight = GETW(&pSubTable->format0.akernpair[j].usRight);

                /* Make sure we are inside the range */
                if (hgLeft >= pface->cGlyphs) hgLeft = 0;
                if (hgRight >= pface->cGlyphs) hgRight = 0;

                /* Windows wants WCHARs, convert them */
                pKernPair->wcFirst = pface->pwcReverseTable[hgLeft];
                pKernPair->wcSecond = pface->pwcReverseTable[hgLeft];
                pKernPair->fwdKern = GETW(&pSubTable->format0.akernpair[j].fwdValue);
                pKernPair++;
            }
        }

        /* Go to next subtable */
        pSubTable = (PVOID)((PCHAR)pSubTable + GETW(&pSubTable->usLength));
    }

    /* Zero terminate array */
    pKernPair->wcFirst = 0;
    pKernPair->wcSecond = 0;
    pKernPair->fwdKern = 0;

    /* Sort the array */
    EngSort((PBYTE)pface->pKerningPairs,
            sizeof(FD_KERNINGPAIR),
            cPairs,
            (SORTCOMP)CompareKernPair);

    /* Set the number of kernpairs in the IFIMETRICS */
    pface->ifiex.ifi.cKerningPairs = cPairs;
}

/** Public Interface **********************************************************/

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

    TRACE("FtfdQueryTrueTypeTable\n");

    /* Check if this file supports TrueType tables */
    if (pfile->ulFileFormat != FILEFMT_TTF &&
        pfile->ulFileFormat != FILEFMT_OTF)
    {
        WARN("File format doesn't support true type tables\n");
        return FD_ERROR;
    }

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
        pjTable = FtfdFindTrueTypeTable(pfile->pvView,
                                        pfile->cjView,
                                        ulFont,
                                        ulTag,
                                        &cjTable);
        if (!pjTable)
        {
            WARN("Couldn't find table '%.4s'\n", (char*)&ulTag);
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
            WARN("dpStart outside the table: %p\n", dpStart);
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

    TRACE("FtfdGetTrueTypeFile\n");

    /* Check if this file is TrueType */
    if (pfile->ulFileFormat != FILEFMT_TTF &&
        pfile->ulFileFormat != FILEFMT_OTF)
    {
        WARN("File format is not TrueType or Opentype\n");
        return NULL;
    }

    /* Return the pointer and size */
    if (pcj) *pcj = pfile->cjView;
    return pfile->pvView;
}


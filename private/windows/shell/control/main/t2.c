/** FILE: t2.c ************* Module Header ********************************
 *
 *  Control panel applet for Font configuration.  This file holds code for
 *  the items concerning truetype fonts.
 *
 * History:
 *  12:30 on Tues  23 Apr 1991  -by-  Steve Cathcart   [stevecat]
 *        Took base code from Win 3.1 source
 *  10:30 on Tues  04 Feb 1992  -by-  Steve Cathcart   [stevecat]
 *           Updated code to latest Win 3.1 sources
 *
 *  Copyright (C) 1990-1992 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                                Include files
//==========================================================================
// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Application specific
#include "main.h"

#include <lzexpand.h>

//#include "fontdefs.h"
//#include "newexe.h"

//==========================================================================
//                            Local Definitions
//==========================================================================

// void FAR PASCAL UnicodeToAnsi(WORD FAR *lpwName, LPSTR szName);

// #define SFNT_MAGIC        0x5F0F3CF5
#define SFNT_MAGIC          0xf53C0F5f
#define TAG_FONTHEADER      0x64616568      /* 'head' */
#define TAG_NAMINGTABLE     0x656d616e      /* 'name' */
#define TAG_CHARTOINDEXMAP  0x70616d63      /* 'cmap' */

#define FAMILY_ID           0x0100
#define SUBFAMILY_ID        0x0200
#define FACENAME_ID         0x0400        // in mac order
#define DEFAULT_LANG        0x0904        // US (1033) (in mac order)


typedef struct {
    WORD  platformID;
    WORD  specificID;
    DWORD  offset;
} sfnt_platformEntry;

typedef struct {
    WORD  version;
    WORD  numTables;
    // sfnt_platformEntry platform[1]; /* platform[numTables] */
} sfnt_char2IndexDirectory;

typedef struct {
    WORD platformID;
    WORD specificID;
    WORD languageID;
    WORD nameID;
    WORD length;
    WORD offset;
} sfnt_NameRecord;

typedef struct {
    WORD format;
    WORD count;
    WORD stringOffset;
/*  sfnt_NameRecord[count]  */
} sfnt_NamingTable;

typedef struct tt_head_tag {
  DWORD dwVersion;
  DWORD dwRevision;
  DWORD dwChecksum;
  DWORD dwMagic;
  WORD  wFlags;
  WORD  wUnitsPerEm;
  DWORD dwCreated1;
  DWORD dwCreated2;
  DWORD dwModified1;
  DWORD dwModified2;
  WORD  wXMin;
  WORD  wYMin;
  WORD  wXMax;
  WORD  wYMax;
  WORD  wStyle;
  WORD  wMinReadableSize;
  short iDirectionHint;
  short iIndexToLocFormat;
  short iGlyphDataFormat;
  } tt_head;

typedef struct tttag_tag {
  DWORD dwTag;
  DWORD dwCheckSum;
  DWORD dwOffset;
  DWORD dwLength;
  } tttag;

typedef struct tt_hdr_tag {
  DWORD dwVersion;
  WORD  uNumTables;
  WORD  uSearchRange;
  WORD  uEntrySelector;
  WORD  uRangeShift;
  } tt_hdr;


WORD SWAPW(WORD w)
{
     return MAKEWORD(HIBYTE(w), LOBYTE(w));

//    _asm xchg al, ah
//    if(0)return(0);
}

DWORD SWAPL(DWORD dw)
{
     return MAKELONG(SWAPW(HIWORD(dw)), SWAPW(LOWORD(dw)));

//    _asm {
//        xchg    al, ah
//        xchg    dl, dh
//    xchg    ax, dx
//    }
//    if(0)return(0L);
}


#define Mac2Ansi(a)    a    // do this right later!

void FillName (LPTSTR szName, unsigned PlatformID, WORD igi, WORD UNALIGNED *pStringArea)
{
    WORD wName[64];
//    WORD i, size;

    if (PlatformID == 0x300)
    {
        // Windows font?

        if (igi >= sizeof(wName))
            igi = sizeof(wName) - sizeof(wName[0]);

        igi /= sizeof(wName[0]);
        wName[igi] = 0;
        szName[igi] = TEXT('\0');

//        size = igi;
        for ( ; igi>0; )
        {
//
//  Original statement - has ambiguity in [--igi] expression.  Current
//  x86 & MIPS compilers calculate the side effect of --igi before the
//  statement is executed.  PowerPC does the opposite.  Remove ambiguity
//  by separating statement with predecrement first.
//      [stevecat]  4/12/94  Windows NT Daytona 
//
//            wName[--igi] = SWAPW(pStringArea[igi]);
//

            igi--;

            wName[igi] = SWAPW(pStringArea[igi]);

// FIX FIX FIX [stevecat] - Temp conversion routine
#ifdef UNICODE
            szName[igi] = wName[igi];
#else
            szName[igi] = LOBYTE(wName[igi]);
#endif
        }

        // UnicodeToAnsi(wName, szName);
        //for (i = 0; i < size; i++)
        //{
        //    szName[i] = LOBYTE(wName[i]);
        //    if (wName[i] == 0)
        //        break;
        //}
    }
    else
    {
        // Mac font

        szName[igi] = (TCHAR) 0;

        while (igi--)
            szName[igi] = Mac2Ansi (((LPBYTE)pStringArea)[igi]);
    }
}

// find a TT name matching the platform specific and language from
// the name table
//
// in:
//    pNames        name table to search
//    PlatformID    search for this
//    SpecificID    and this
//    uLanguageID    and this
//    NameID        this is the name type
//
// out:
//    szName        name if found
//
// returns:
//    TRUE    name found, szName contains the name
//    FALSE    name not found, szName is NULL

BOOL FindNameThing(sfnt_NamingTable *pNames, WORD PlatformID, WORD SpecificID, WORD uLanguageID, WORD NameID, LPTSTR szName)
{
    int cNames;
    sfnt_NameRecord *pNameRecord;
    LPBYTE pStringArea;

    szName[0] = (TCHAR) 0;

    cNames = SWAPW(pNames->count);
    pNameRecord = (sfnt_NameRecord *)((LPBYTE)pNames + sizeof(sfnt_NamingTable));
    pStringArea = (LPBYTE) pNames + SWAPW(pNames->stringOffset);

    for (; cNames--; pNameRecord++)
    {
        if (pNameRecord->platformID == PlatformID &&
              pNameRecord->specificID == SpecificID &&
              pNameRecord->languageID == uLanguageID &&
            pNameRecord->nameID     == NameID)
        {

            FillName (szName, PlatformID, SWAPW(pNameRecord->length), (PWORD)(pStringArea + SWAPW(pNameRecord->offset)));
            return TRUE;
        }
    }
    return FALSE;
}


BOOL GetName(int fh, tttag *pTTTag, WORD PlatformID, WORD SpecificID, WORD uLanguageID, LPTSTR szName)
{
    sfnt_NamingTable *pNames;
    WORD size;
    TCHAR szSubFamily[64];
    WORD uDefaultLanguageID;

    size = (WORD)SWAPL(pTTTag->dwLength);

    uLanguageID = SWAPW(uLanguageID);

    pNames = (sfnt_NamingTable *) LocalAlloc (LPTR, size);
    if (!pNames)
        return FALSE;

    szName[0] = (TCHAR) 0;

    if (LZRead(fh, (LPSTR) pNames, (int)size) == (int)size)
    {
        uDefaultLanguageID = (PlatformID == 0x300) ? DEFAULT_LANG : 0;

        if (!FindNameThing(pNames, PlatformID, SpecificID, uLanguageID, FACENAME_ID, szName))
        {
            if (FindNameThing(pNames, PlatformID, SpecificID, uLanguageID, SUBFAMILY_ID, szSubFamily) &&
                (FindNameThing(pNames, PlatformID, SpecificID, uLanguageID, FAMILY_ID, szName) ||
                 FindNameThing(pNames, PlatformID, SpecificID, uDefaultLanguageID, FAMILY_ID, szName)))
            {
                lstrcat(szName, TEXT(" "));
                lstrcat(szName, szSubFamily);
            }
            else
            {
                FindNameThing(pNames, PlatformID, SpecificID, uDefaultLanguageID, FACENAME_ID, szName);
            }
        }
    }

    LocalFree((HANDLE)pNames);

    return (BOOL)szName[0];
}


BOOL IsTrueType(LPBUFTYPE lpFile)
{
    int fh;
    OFSTRUCT ofstruct;
    short i, nTables;
    WORD wSize;
    RASTERIZER_STATUS info;

    tt_hdr TTHeader;
    tttag *pTags;

    tt_head TTFontHeader;
    sfnt_platformEntry FAR *lpPlat;
    struct cmap_thing
    {
        sfnt_char2IndexDirectory DirCmap;
        sfnt_platformEntry Plat[2];
    } Cmap;

    unsigned cTables, ncTables;
    WORD SpecificID;
    WORD PlatformID = (WORD)-1;
    int result = FALSE;


    GetRasterizerCaps(&info, sizeof(info));

    if ((fh=LZOpenFile(lpFile->name, &ofstruct, 0)) <= -1)
        return(FALSE);

    if (LZRead(fh, (LPSTR) &TTHeader, sizeof(TTHeader)) != sizeof(TTHeader))
        goto IsTrueType_closefile;

    // If number of tables is so large that LocalAlloc fails, then the
    // font will be blown off.

    if ((nTables = SWAPW(TTHeader.uNumTables)) > 0x7fff/sizeof(tttag))
        goto IsTrueType_closefile;

    i = nTables * sizeof(tttag);

    if (!(pTags = (tttag *) LocalAlloc (LPTR, i)))
        goto IsTrueType_closefile;

    if (LZRead(fh, (LPSTR) pTags, i) != i)
        goto FailAndFree;

    // the tables are in sorted order, so we should find 'cmap'
    // before 'head', then 'name'

    // first we find the cmap table so we can find out what PlatformID
    // this font uses

    for (i=0; i<nTables; i++)
    {
        if (pTags[i].dwTag == TAG_CHARTOINDEXMAP)
        {
            // get platform stuff

            LZSeek(fh, SWAPL(pTags[i].dwOffset), SEEK_BEG);
            wSize = LZRead(fh, (LPSTR) &Cmap, sizeof(Cmap));
            if ((ncTables=SWAPW(Cmap.DirCmap.numTables)) == 1)
            {
                if (wSize < sizeof(Cmap)-sizeof(Cmap.Plat[1]))
                    break;
            }
            else if (wSize != sizeof(Cmap))
                break;

            for (cTables=0; cTables<ncTables; cTables++)
            {
                // we read 2 platform entries at a time
                if (cTables >= 2 && !(cTables & 1))
                {
                    wSize = ncTables-cTables>1 ? sizeof(Cmap.Plat)
                                               : sizeof(Cmap.Plat[0]);
                    if ((WORD)LZRead(fh, (LPSTR) Cmap.Plat, wSize) != wSize)
                        break;
                }

                lpPlat = &Cmap.Plat[cTables & 01];

                // Unicode: get this and exit
                if (lpPlat->platformID == 0x300)
                {
                    PlatformID = lpPlat->platformID;
                    SpecificID = lpPlat->specificID;
                    break;
                }
                // Mac: get it, hope the Unicode platform will come
                if (lpPlat->platformID == 0x100 && lpPlat->specificID == 0)
                {
                    PlatformID = 0x100;
                    SpecificID = 0;
                }
            }
            break;    // found continue below
        }
    }

    if (PlatformID == (WORD)-1)
        goto FailAndFree;

    // we found 'cmap' with the PlatformID now look for 'head'
    // then 'name'

    while (++i < nTables)
    {
        if (pTags[i].dwTag == TAG_FONTHEADER)
        {
            LZSeek(fh, SWAPL(pTags[i].dwOffset), SEEK_BEG);
            if (LZRead(fh, (LPSTR) &TTFontHeader, sizeof(TTFontHeader))
                    != sizeof(TTFontHeader) ||
                    TTFontHeader.dwMagic != SFNT_MAGIC)
                goto FailAndFree;
            break;
        }
    }

    while (++i < nTables)
    {
        if (pTags[i].dwTag == TAG_NAMINGTABLE)
        {
            LZSeek(fh, SWAPL(pTags[i].dwOffset), SEEK_BEG);
            result = GetName(fh, &pTags[i], PlatformID, SpecificID,
                                        info.nLanguageID, lpFile->desc);
            break;
        }
    }

FailAndFree:
    LocalFree((HANDLE)pTags);

IsTrueType_closefile:
    LZClose(fh);
    return result;
}


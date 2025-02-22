/*
 * GDI BiDirectional handling
 *
 * Copyright 2003 Shachar Shemesh
 * Copyright 2007 Maarten Lankhorst
 * Copyright 2010 CodeWeavers, Aric Stewart
 * 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * Code derived from the modified reference implementation
 * that was found in revision 17 of http://unicode.org/reports/tr9/
 * "Unicode Standard Annex #9: THE BIDIRECTIONAL ALGORITHM"
 *
 * -- Copyright (C) 1999-2005, ASMUS, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of the Unicode data files and any associated documentation (the
 * "Data Files") or Unicode software and any associated documentation (the
 * "Software") to deal in the Data Files or Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, and/or sell copies of the Data Files or Software,
 * and to permit persons to whom the Data Files or Software are furnished
 * to do so, provided that (a) the above copyright notice(s) and this
 * permission notice appear with all copies of the Data Files or Software,
 * (b) both the above copyright notice(s) and this permission notice appear
 * in associated documentation, and (c) there is clear notice in each
 * modified Data File or in the Software as well as in the documentation
 * associated with the Data File(s) or Software that the data or software
 * has been modified.
 */
 
#include "ros_lpk.h"

WINE_DEFAULT_DEBUG_CHANNEL(bidi);

/* HELPER FUNCTIONS AND DECLARATIONS */

#define odd(x) ((x) & 1)

extern const unsigned short bidi_direction_table[] DECLSPEC_HIDDEN;

/*------------------------------------------------------------------------
    Bidirectional Character Types

    as defined by the Unicode Bidirectional Algorithm Table 3-7.

    Note:

      The list of bidirectional character types here is not grouped the
      same way as the table 3-7, since the numberic values for the types
      are chosen to keep the state and action tables compact.
------------------------------------------------------------------------*/
enum directions
{
    /* input types */
             /* ON MUST be zero, code relies on ON = N = 0 */
    ON = 0,  /* Other Neutral */
    L,       /* Left Letter */
    R,       /* Right Letter */
    AN,      /* Arabic Number */
    EN,      /* European Number */
    AL,      /* Arabic Letter (Right-to-left) */
    NSM,     /* Non-spacing Mark */
    CS,      /* Common Separator */
    ES,      /* European Separator */
    ET,      /* European Terminator (post/prefix e.g. $ and %) */

    /* resolved types */
    BN,      /* Boundary neutral (type of RLE etc after explicit levels) */

    /* input types, */
    S,       /* Segment Separator (TAB)        // used only in L1 */
    WS,      /* White space                    // used only in L1 */
    B,       /* Paragraph Separator (aka as PS) */

    /* types for explicit controls */
    RLO,     /* these are used only in X1-X9 */
    RLE,
    LRO,
    LRE,
    PDF,

    LRI, /* Isolate formatting characters new with 6.3 */
    RLI,
    FSI,
    PDI,

    /* resolved types, also resolved directions */
    NI = ON,  /* alias, where ON, WS and S are treated the same */
};

/* HELPER FUNCTIONS */

static inline unsigned short get_table_entry(const unsigned short *table, WCHAR ch)
{
    return table[table[table[ch >> 8] + ((ch >> 4) & 0x0f)] + (ch & 0xf)];
}

/* Convert the libwine information to the direction enum */
static void classify(LPCWSTR lpString, WORD *chartype, DWORD uCount)
{
    unsigned i;

    for (i = 0; i < uCount; ++i)
        chartype[i] = get_table_entry( bidi_direction_table, lpString[i] );
}

/* Set a run of cval values at locations all prior to, but not including */
/* iStart, to the new value nval. */
static void SetDeferredRun(BYTE *pval, int cval, int iStart, int nval)
{
    int i = iStart - 1;
    for (; i >= iStart - cval; i--)
    {
        pval[i] = nval;
    }
}

/* THE PARAGRAPH LEVEL */

/*------------------------------------------------------------------------
    Function: resolveParagraphs

    Resolves the input strings into blocks over which the algorithm
    is then applied.

    Implements Rule P1 of the Unicode Bidi Algorithm

    Input: Text string
           Character count

    Output: revised character count

    Note:    This is a very simplistic function. In effect it restricts
            the action of the algorithm to the first paragraph in the input
            where a paragraph ends at the end of the first block separator
            or at the end of the input text.

------------------------------------------------------------------------*/

static int resolveParagraphs(WORD *types, int cch)
{
    /* skip characters not of type B */
    int ich = 0;
    for(; ich < cch && types[ich] != B; ich++);
    /* stop after first B, make it a BN for use in the next steps */
    if (ich < cch && types[ich] == B)
        types[ich++] = BN;
    return ich;
}

/* REORDER */
/*------------------------------------------------------------------------
    Function: resolveLines

    Breaks a paragraph into lines

    Input:  Array of line break flags
            Character count
    In/Out: Array of characters

    Returns the count of characters on the first line

    Note: This function only breaks lines at hard line breaks. Other
    line breaks can be passed in. If pbrk[n] is TRUE, then a break
    occurs after the character in pszInput[n]. Breaks before the first
    character are not allowed.
------------------------------------------------------------------------*/
static int resolveLines(LPCWSTR pszInput, const BOOL * pbrk, int cch)
{
    /* skip characters not of type LS */
    int ich = 0;
    for(; ich < cch; ich++)
    {
        if (pszInput[ich] == (WCHAR)'\n' || (pbrk && pbrk[ich]))
        {
            ich++;
            break;
        }
    }

    return ich;
}

/*------------------------------------------------------------------------
    Function: resolveWhiteSpace

    Resolves levels for WS and S
    Implements rule L1 of the Unicode bidi Algorithm.

    Input:  Base embedding level
            Character count
            Array of direction classes (for one line of text)

    In/Out: Array of embedding levels (for one line of text)

    Note: this should be applied a line at a time. The default driver
          code supplied in this file assumes a single line of text; for
          a real implementation, cch and the initial pointer values
          would have to be adjusted.
------------------------------------------------------------------------*/
static void resolveWhitespace(int baselevel, const WORD *pcls, BYTE *plevel, int cch)
{
    int cchrun = 0;
    BYTE oldlevel = baselevel;

    int ich = 0;
    for (; ich < cch; ich++)
    {
        switch(pcls[ich])
        {
        default:
            cchrun = 0; /* any other character breaks the run */
            break;
        case WS:
            cchrun++;
            break;

        case RLE:
        case LRE:
        case LRO:
        case RLO:
        case PDF:
        case LRI:
        case RLI:
        case FSI:
        case PDI:
        case BN:
            plevel[ich] = oldlevel;
            cchrun++;
            break;

        case S:
        case B:
            /* reset levels for WS before eot */
            SetDeferredRun(plevel, cchrun, ich, baselevel);
            cchrun = 0;
            plevel[ich] = baselevel;
            break;
        }
        oldlevel = plevel[ich];
    }
    /* reset level before eot */
    SetDeferredRun(plevel, cchrun, ich, baselevel);
}

/*------------------------------------------------------------------------
    Function: BidiLines

    Implements the Line-by-Line phases of the Unicode Bidi Algorithm

      Input:     Count of characters
                 Array of character directions

    Inp/Out: Input text
             Array of levels

------------------------------------------------------------------------*/
static void BidiLines(int baselevel, LPWSTR pszOutLine, LPCWSTR pszLine, const WORD * pclsLine,
                      BYTE * plevelLine, int cchPara, const BOOL * pbrk)
{
    int cchLine = 0;
    int done = 0;
    int *run;

    run = HeapAlloc(GetProcessHeap(), 0, cchPara * sizeof(int));
    if (!run)
    {
        WARN("Out of memory\n");
        return;
    }

    do
    {
        /* break lines at LS */
        cchLine = resolveLines(pszLine, pbrk, cchPara);

        /* resolve whitespace */
        resolveWhitespace(baselevel, pclsLine, plevelLine, cchLine);

        if (pszOutLine)
        {
            int i;
            /* reorder each line in place */
            ScriptLayout(cchLine, plevelLine, NULL, run);
            for (i = 0; i < cchLine; i++)
                pszOutLine[done+run[i]] = pszLine[i];
        }

        pszLine += cchLine;
        plevelLine += cchLine;
        pbrk += pbrk ? cchLine : 0;
        pclsLine += cchLine;
        cchPara -= cchLine;
        done += cchLine;

    } while (cchPara);

    HeapFree(GetProcessHeap(), 0, run);
}

/*************************************************************
 *    BIDI_Reorder
 *
 *     Returns TRUE if reordering was required and done.
 */
BOOL BIDI_Reorder(
                HDC hDC,        /*[in] Display DC */
                LPCWSTR lpString,       /* [in] The string for which information is to be returned */
                INT uCount,     /* [in] Number of WCHARs in string. */
                DWORD dwFlags,  /* [in] GetCharacterPlacement compatible flags specifying how to process the string */
                DWORD dwWineGCP_Flags,       /* [in] Wine internal flags - Force paragraph direction */
                LPWSTR lpOutString, /* [out] Reordered string */
                INT uCountOut,  /* [in] Size of output buffer */
                UINT *lpOrder, /* [out] Logical -> Visual order map */
                WORD **lpGlyphs, /* [out] reordered, mirrored, shaped glyphs to display */
                INT *cGlyphs /* [out] number of glyphs generated */
    )
{
    WORD *chartype;
    BYTE *levels;
    INT i, done;
    unsigned glyph_i;
    BOOL is_complex;

    int maxItems;
    int nItems;
    SCRIPT_CONTROL Control;
    SCRIPT_STATE State;
    SCRIPT_ITEM *pItems;
    HRESULT res;
    SCRIPT_CACHE psc = NULL;
    WORD *run_glyphs = NULL;
    WORD *pwLogClust = NULL;
    SCRIPT_VISATTR *psva = NULL;
    DWORD cMaxGlyphs = 0;
    BOOL  doGlyphs = TRUE;

    TRACE("%s, %d, 0x%08x lpOutString=%p, lpOrder=%p\n",
          debugstr_wn(lpString, uCount), uCount, dwFlags,
          lpOutString, lpOrder);

    memset(&Control, 0, sizeof(Control));
    memset(&State, 0, sizeof(State));
    if (lpGlyphs)
        *lpGlyphs = NULL;

    if (!(dwFlags & GCP_REORDER))
    {
        FIXME("Asked to reorder without reorder flag set\n");
        return FALSE;
    }

    if (lpOutString && uCountOut < uCount)
    {
        FIXME("lpOutString too small\n");
        return FALSE;
    }

    chartype = HeapAlloc(GetProcessHeap(), 0, uCount * sizeof(WORD));
    if (!chartype)
    {
        WARN("Out of memory\n");
        return FALSE;
    }

    if (lpOutString)
        memcpy(lpOutString, lpString, uCount * sizeof(WCHAR));

    is_complex = FALSE;
    for (i = 0; i < uCount && !is_complex; i++)
    {
        if ((lpString[i] >= 0x900 && lpString[i] <= 0xfff) ||
            (lpString[i] >= 0x1cd0 && lpString[i] <= 0x1cff) ||
            (lpString[i] >= 0xa840 && lpString[i] <= 0xa8ff))
            is_complex = TRUE;
    }

    /* Verify reordering will be required */
    if ((WINE_GCPW_FORCE_RTL == (dwWineGCP_Flags&WINE_GCPW_DIR_MASK)) ||
        ((dwWineGCP_Flags&WINE_GCPW_DIR_MASK) == WINE_GCPW_LOOSE_RTL))
        State.uBidiLevel = 1;
    else if (!is_complex)
    {
        done = 1;
        classify(lpString, chartype, uCount);
        for (i = 0; i < uCount; i++)
            switch (chartype[i])
            {
                case R:
                case AL:
                case RLE:
                case RLO:
                    done = 0;
                    break;
            }
        if (done)
        {
            HeapFree(GetProcessHeap(), 0, chartype);
            if (lpOrder)
            {
                for (i = 0; i < uCount; i++)
                    lpOrder[i] = i;
            }
            return TRUE;
        }
    }

    levels = HeapAlloc(GetProcessHeap(), 0, uCount * sizeof(BYTE));
    if (!levels)
    {
        WARN("Out of memory\n");
        HeapFree(GetProcessHeap(), 0, chartype);
        return FALSE;
    }

    maxItems = 5;
    pItems = HeapAlloc(GetProcessHeap(),0, maxItems * sizeof(SCRIPT_ITEM));
    if (!pItems)
    {
        WARN("Out of memory\n");
        HeapFree(GetProcessHeap(), 0, chartype);
        HeapFree(GetProcessHeap(), 0, levels);
        return FALSE;
    }

    if (lpGlyphs)
    {
#ifdef __REACTOS__
        /* ReactOS r57677 and r57679 */
        cMaxGlyphs = 3 * uCount / 2 + 16;
#else
        cMaxGlyphs = 1.5 * uCount + 16;
#endif
        run_glyphs = HeapAlloc(GetProcessHeap(),0,sizeof(WORD) * cMaxGlyphs);
        if (!run_glyphs)
        {
            WARN("Out of memory\n");
            HeapFree(GetProcessHeap(), 0, chartype);
            HeapFree(GetProcessHeap(), 0, levels);
            HeapFree(GetProcessHeap(), 0, pItems);
            return FALSE;
        }
        pwLogClust = HeapAlloc(GetProcessHeap(),0,sizeof(WORD) * uCount);
        if (!pwLogClust)
        {
            WARN("Out of memory\n");
            HeapFree(GetProcessHeap(), 0, chartype);
            HeapFree(GetProcessHeap(), 0, levels);
            HeapFree(GetProcessHeap(), 0, pItems);
            HeapFree(GetProcessHeap(), 0, run_glyphs);
            return FALSE;
        }
        psva = HeapAlloc(GetProcessHeap(),0,sizeof(SCRIPT_VISATTR) * uCount);
        if (!psva)
        {
            WARN("Out of memory\n");
            HeapFree(GetProcessHeap(), 0, chartype);
            HeapFree(GetProcessHeap(), 0, levels);
            HeapFree(GetProcessHeap(), 0, pItems);
            HeapFree(GetProcessHeap(), 0, run_glyphs);
            HeapFree(GetProcessHeap(), 0, pwLogClust);
            return FALSE;
        }
    }

    done = 0;
    glyph_i = 0;
    while (done < uCount)
    {
        INT j;
        classify(lpString + done, chartype, uCount - done);
        /* limit text to first block */
        i = resolveParagraphs(chartype, uCount - done);
        for (j = 0; j < i; ++j)
            switch(chartype[j])
            {
                case B:
                case S:
                case WS:
                case ON: chartype[j] = NI;
                default: continue;
            }

        if ((dwWineGCP_Flags&WINE_GCPW_DIR_MASK) == WINE_GCPW_LOOSE_RTL)
            State.uBidiLevel = 1;
        else if ((dwWineGCP_Flags&WINE_GCPW_DIR_MASK) == WINE_GCPW_LOOSE_LTR)
            State.uBidiLevel = 0;

        if (dwWineGCP_Flags & WINE_GCPW_LOOSE_MASK)
        {
            for (j = 0; j < i; ++j)
                if (chartype[j] == L)
                {
                    State.uBidiLevel = 0;
                    break;
                }
                else if (chartype[j] == R || chartype[j] == AL)
                {
                    State.uBidiLevel = 1;
                    break;
                }
        }

        res = ScriptItemize(lpString + done, i, maxItems, &Control, &State, pItems, &nItems);
        while (res == E_OUTOFMEMORY)
        {
            maxItems = maxItems * 2;
            pItems = HeapReAlloc(GetProcessHeap(), 0, pItems, sizeof(SCRIPT_ITEM) * maxItems);
            if (!pItems)
            {
                WARN("Out of memory\n");
                HeapFree(GetProcessHeap(), 0, chartype);
                HeapFree(GetProcessHeap(), 0, levels);
                HeapFree(GetProcessHeap(), 0, run_glyphs);
                HeapFree(GetProcessHeap(), 0, pwLogClust);
                HeapFree(GetProcessHeap(), 0, psva);
                return FALSE;
            }
            res = ScriptItemize(lpString + done, i, maxItems, &Control, &State, pItems, &nItems);
        }

        if (lpOutString || lpOrder)
            for (j = 0; j < nItems; j++)
            {
                int k;
                for (k = pItems[j].iCharPos; k < pItems[j+1].iCharPos; k++)
                    levels[k] = pItems[j].a.s.uBidiLevel;
            }

        if (lpOutString)
        {
            /* assign directional types again, but for WS, S this time */
            classify(lpString + done, chartype, i);

            BidiLines(State.uBidiLevel, lpOutString + done, lpString + done,
                        chartype, levels, i, 0);
        }

        if (lpOrder)
        {
            int k, lastgood;
            for (j = lastgood = 0; j < i; ++j)
                if (levels[j] != levels[lastgood])
                {
                    --j;
                    if (odd(levels[lastgood]))
                        for (k = j; k >= lastgood; --k)
                            lpOrder[done + k] = done + j - k;
                    else
                        for (k = lastgood; k <= j; ++k)
                            lpOrder[done + k] = done + k;
                    lastgood = ++j;
                }
            if (odd(levels[lastgood]))
                for (k = j - 1; k >= lastgood; --k)
                    lpOrder[done + k] = done + j - 1 - k;
            else
                for (k = lastgood; k < j; ++k)
                    lpOrder[done + k] = done + k;
        }

        if (lpGlyphs && doGlyphs)
        {
            BYTE *runOrder;
            int *visOrder;
            SCRIPT_ITEM *curItem;

            runOrder = HeapAlloc(GetProcessHeap(), 0, maxItems * sizeof(*runOrder));
            visOrder = HeapAlloc(GetProcessHeap(), 0, maxItems * sizeof(*visOrder));
            if (!runOrder || !visOrder)
            {
                WARN("Out of memory\n");
                HeapFree(GetProcessHeap(), 0, runOrder);
                HeapFree(GetProcessHeap(), 0, visOrder);
                HeapFree(GetProcessHeap(), 0, chartype);
                HeapFree(GetProcessHeap(), 0, levels);
                HeapFree(GetProcessHeap(), 0, pItems);
                HeapFree(GetProcessHeap(), 0, psva);
                HeapFree(GetProcessHeap(), 0, pwLogClust);
                return FALSE;
            }

            for (j = 0; j < nItems; j++)
                runOrder[j] = pItems[j].a.s.uBidiLevel;

            ScriptLayout(nItems, runOrder, visOrder, NULL);

            for (j = 0; j < nItems; j++)
            {
                int k;
                int cChars,cOutGlyphs;
                curItem = &pItems[visOrder[j]];

                cChars = pItems[visOrder[j]+1].iCharPos - curItem->iCharPos;

                res = ScriptShape(hDC, &psc, lpString + done + curItem->iCharPos, cChars, cMaxGlyphs, &curItem->a, run_glyphs, pwLogClust, psva, &cOutGlyphs);
                while (res == E_OUTOFMEMORY)
                {
                    cMaxGlyphs *= 2;
                    run_glyphs = HeapReAlloc(GetProcessHeap(), 0, run_glyphs, sizeof(WORD) * cMaxGlyphs);
                    if (!run_glyphs)
                    {
                        WARN("Out of memory\n");
                        HeapFree(GetProcessHeap(), 0, runOrder);
                        HeapFree(GetProcessHeap(), 0, visOrder);
                        HeapFree(GetProcessHeap(), 0, chartype);
                        HeapFree(GetProcessHeap(), 0, levels);
                        HeapFree(GetProcessHeap(), 0, pItems);
                        HeapFree(GetProcessHeap(), 0, psva);
                        HeapFree(GetProcessHeap(), 0, pwLogClust);
                        HeapFree(GetProcessHeap(), 0, *lpGlyphs);
                        ScriptFreeCache(&psc);
                        *lpGlyphs = NULL;
                        return FALSE;
                    }
                    res = ScriptShape(hDC, &psc, lpString + done + curItem->iCharPos, cChars, cMaxGlyphs, &curItem->a, run_glyphs, pwLogClust, psva, &cOutGlyphs);
                }
                if (res)
                {
                    if (res == USP_E_SCRIPT_NOT_IN_FONT)
                        TRACE("Unable to shape with currently selected font\n");
                    else
                        FIXME("Unable to shape string (%x)\n",res);
                    j = nItems;
                    doGlyphs = FALSE;
                    HeapFree(GetProcessHeap(), 0, *lpGlyphs);
                    *lpGlyphs = NULL;
                }
                else
                {
                    if (*lpGlyphs)
                        *lpGlyphs = HeapReAlloc(GetProcessHeap(), 0, *lpGlyphs, sizeof(WORD) * (glyph_i + cOutGlyphs));
                   else
                        *lpGlyphs = HeapAlloc(GetProcessHeap(), 0, sizeof(WORD) * (glyph_i + cOutGlyphs));
                    for (k = 0; k < cOutGlyphs; k++)
                        (*lpGlyphs)[glyph_i+k] = run_glyphs[k];
                    glyph_i += cOutGlyphs;
                }
            }
            HeapFree(GetProcessHeap(), 0, runOrder);
            HeapFree(GetProcessHeap(), 0, visOrder);
        }

        done += i;
    }
    if (cGlyphs)
        *cGlyphs = glyph_i;

    HeapFree(GetProcessHeap(), 0, chartype);
    HeapFree(GetProcessHeap(), 0, levels);
    HeapFree(GetProcessHeap(), 0, pItems);
    HeapFree(GetProcessHeap(), 0, run_glyphs);
    HeapFree(GetProcessHeap(), 0, pwLogClust);
    HeapFree(GetProcessHeap(), 0, psva);
    ScriptFreeCache(&psc);
    return TRUE;
}

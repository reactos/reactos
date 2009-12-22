/*
 * GDI BiDirectional handling
 *
 * Copyright 2003 Shachar Shemesh
 * Copyright 2007 Maarten Lankhorst
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

#include "config.h"

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "wine/debug.h"
#include "gdi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(bidi);

#define ASSERT(x) do { if (!(x)) FIXME("assert failed: %s\n", #x); } while(0)
#define MAX_LEVEL 61

/* HELPER FUNCTIONS AND DECLARATIONS */

#define odd(x) ((x) & 1)

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

    /* resolved types, also resolved directions */
    N = ON,  /* alias, where ON, WS and S are treated the same */
};

/* HELPER FUNCTIONS */

/* grep -r ';BN;' data.txt  | grep -v [0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F] | sed -e s@\;.*@@ -e s/^..../0x\&,\ / | xargs echo */
static const WCHAR BNs[] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008,
    0x000E, 0x000F, 0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016,
    0x0017, 0x0018, 0x0019, 0x001A, 0x001B, 0x007F, 0x0080, 0x0081, 0x0082,
    0x0083, 0x0084, 0x0086, 0x0087, 0x0088, 0x0089, 0x008A, 0x008B, 0x008C,
    0x008D, 0x008E, 0x008F, 0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095,
    0x0096, 0x0097, 0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D, 0x009E,
    0x009F, 0x00AD, 0x070F, 0x200B, 0x200C, 0x200D, 0x2060, 0x2061, 0x2062,
    0x2063, 0x206A, 0x206B, 0x206C, 0x206D, 0x206E, 0x206F, 0xFEFF
};

/* Idem, but with ';R;' instead of ';BN;' */
static const WCHAR Rs[] = {
    0x05BE, 0x05C0, 0x05C3, 0x05C6, 0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4,
    0x05D5, 0x05D6, 0x05D7, 0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD,
    0x05DE, 0x05DF, 0x05E0, 0x05E1, 0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6,
    0x05E7, 0x05E8, 0x05E9, 0x05EA, 0x05F0, 0x05F1, 0x05F2, 0x05F3, 0x05F4,
    0x07C0, 0x07C1, 0x07C2, 0x07C3, 0x07C4, 0x07C5, 0x07C6, 0x07C7, 0x07C8,
    0x07C9, 0x07CA, 0x07CB, 0x07CC, 0x07CD, 0x07CE, 0x07CF, 0x07D0, 0x07D1,
    0x07D2, 0x07D3, 0x07D4, 0x07D5, 0x07D6, 0x07D7, 0x07D8, 0x07D9, 0x07DA,
    0x07DB, 0x07DC, 0x07DD, 0x07DE, 0x07DF, 0x07E0, 0x07E1, 0x07E2, 0x07E3,
    0x07E4, 0x07E5, 0x07E6, 0x07E7, 0x07E8, 0x07E9, 0x07EA, 0x07F4, 0x07F5,
    0x07FA, 0x200F, 0xFB1D, 0xFB1F, 0xFB20, 0xFB21, 0xFB22, 0xFB23, 0xFB24,
    0xFB25, 0xFB26, 0xFB27, 0xFB28, 0xFB2A, 0xFB2B, 0xFB2C, 0xFB2D, 0xFB2E,
    0xFB2F, 0xFB30, 0xFB31, 0xFB32, 0xFB33, 0xFB34, 0xFB35, 0xFB36, 0xFB38,
    0xFB39, 0xFB3A, 0xFB3B, 0xFB3C, 0xFB3E, 0xFB40, 0xFB41, 0xFB43, 0xFB44,
    0xFB46, 0xFB47, 0xFB48, 0xFB49, 0xFB4A, 0xFB4B, 0xFB4C, 0xFB4D, 0xFB4E,
    0xFB4F
};

/* Convert the incomplete win32 table to some slightly more useful data */
static void classify(LPCWSTR lpString, WORD *chartype, DWORD uCount)
{
    unsigned i, j;
    GetStringTypeW(CT_CTYPE2, lpString, uCount, chartype);
    for (i = 0; i < uCount; ++i)
        switch (chartype[i])
        {
            case C2_LEFTTORIGHT: chartype[i] = L; break;
            case C2_RIGHTTOLEFT:
                chartype[i] = AL;
                for (j = 0; j < sizeof(Rs)/sizeof(WCHAR); ++j)
                    if (Rs[j] == lpString[i])
                    {
                        chartype[i] = R;
                        break;
                    }
                break;

            case C2_EUROPENUMBER: chartype[i] = EN; break;
            case C2_EUROPESEPARATOR: chartype[i] = ES; break;
            case C2_EUROPETERMINATOR: chartype[i] = ET; break;
            case C2_ARABICNUMBER: chartype[i] = AN; break;
            case C2_COMMONSEPARATOR: chartype[i] = CS; break;
            case C2_BLOCKSEPARATOR: chartype[i] = B; break;
            case C2_SEGMENTSEPARATOR: chartype[i] = S; break;
            case C2_WHITESPACE: chartype[i] = WS; break;
            case C2_OTHERNEUTRAL:
                switch (lpString[i])
                {
                    case 0x202A: chartype[i] = LRE; break;
                    case 0x202B: chartype[i] = RLE; break;
                    case 0x202C: chartype[i] = PDF; break;
                    case 0x202D: chartype[i] = LRO; break;
                    case 0x202E: chartype[i] = RLO; break;
                    default: chartype[i] = ON; break;
                }
                break;
            case C2_NOTAPPLICABLE:
                chartype[i] = NSM;
                for (j = 0; j < sizeof(BNs)/sizeof(WCHAR); ++j)
                    if (BNs[j] == lpString[i])
                    {
                        chartype[i] = BN;
                        break;
                    }
                break;

            default:
                /* According to BiDi spec, unassigned characters default to L */
                FIXME("Unhandled character type: %04x\n", chartype[i]);
                chartype[i] = L;
                break;
        }
}

/* reverse cch characters */
static void reverse(LPWSTR psz, int cch)
{
    WCHAR chTemp;
    int ich = 0;
    for (; ich < --cch; ich++)
    {
        chTemp = psz[ich];
        psz[ich] = psz[cch];
        psz[cch] = chTemp;
    }
}

/* Set a run of cval values at locations all prior to, but not including */
/* iStart, to the new value nval. */
static void SetDeferredRun(WORD *pval, int cval, int iStart, int nval)
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

/* RESOLVE EXPLICIT */

static WORD GreaterEven(int i)
{
    return odd(i) ? i + 1 : i + 2;
}

static WORD GreaterOdd(int i)
{
    return odd(i) ? i + 2 : i + 1;
}

static WORD EmbeddingDirection(int level)
{
    return odd(level) ? R : L;
}

/*------------------------------------------------------------------------
    Function: resolveExplicit

    Recursively resolves explicit embedding levels and overrides.
    Implements rules X1-X9, of the Unicode Bidirectional Algorithm.

    Input: Base embedding level and direction
           Character count

    Output: Array of embedding levels

    In/Out: Array of direction classes


    Note: The function uses two simple counters to keep track of
          matching explicit codes and PDF. Use the default argument for
          the outermost call. The nesting counter counts the recursion
          depth and not the embedding level.
------------------------------------------------------------------------*/

static int resolveExplicit(int level, int dir, WORD *pcls, WORD *plevel, int cch, int nNest)
{
    /* always called with a valid nesting level
       nesting levels are != embedding levels */
    int nLastValid = nNest;
    int ich = 0;

    /* check input values */
    ASSERT(nNest >= 0 && level >= 0 && level <= MAX_LEVEL);

    /* process the text */
    for (; ich < cch; ich++)
    {
        WORD cls = pcls[ich];
        switch (cls)
        {
        case LRO:
        case LRE:
            nNest++;
            if (GreaterEven(level) <= MAX_LEVEL - (cls == LRO ? 2 : 0))
            {
                plevel[ich] = GreaterEven(level);
                pcls[ich] = BN;
                ich += resolveExplicit(plevel[ich], (cls == LRE ? N : L),
                            &pcls[ich+1], &plevel[ich+1],
                             cch - (ich+1), nNest);
                nNest--;
                continue;
            }
            cls = pcls[ich] = BN;
            break;

        case RLO:
        case RLE:
            nNest++;
            if (GreaterOdd(level) <= MAX_LEVEL - (cls == RLO ? 2 : 0))
            {
                plevel[ich] = GreaterOdd(level);
                pcls[ich] = BN;
                ich += resolveExplicit(plevel[ich], (cls == RLE ? N : R),
                                &pcls[ich+1], &plevel[ich+1],
                                 cch - (ich+1), nNest);
                nNest--;
                continue;
            }
            cls = pcls[ich] = BN;
            break;

        case PDF:
            cls = pcls[ich] = BN;
            if (nNest)
            {
                if (nLastValid < nNest)
                {
                    nNest--;
                }
                else
                {
                    cch = ich; /* break the loop, but complete body */
                }
            }
        }

        /* Apply the override */
        if (dir != N)
        {
            cls = dir;
        }
        plevel[ich] = level;
        if (pcls[ich] != BN)
            pcls[ich] = cls;
    }

    return ich;
}

/* RESOLVE WEAK TYPES */

enum states /* possible states */
{
    xa,        /*  arabic letter */
    xr,        /*  right letter */
    xl,        /*  left letter */

    ao,        /*  arabic lett. foll by ON */
    ro,        /*  right lett. foll by ON */
    lo,        /*  left lett. foll by ON */

    rt,        /*  ET following R */
    lt,        /*  ET following L */

    cn,        /*  EN, AN following AL */
    ra,        /*  arabic number foll R */
    re,        /*  european number foll R */
    la,        /*  arabic number foll L */
    le,        /*  european number foll L */

    ac,        /*  CS following cn */
    rc,        /*  CS following ra */
    rs,        /*  CS,ES following re */
    lc,        /*  CS following la */
    ls,        /*  CS,ES following le */

    ret,    /*  ET following re */
    let,    /*  ET following le */
} ;

static const int stateWeak[][10] =
{
    /*    N,  L,  R, AN, EN, AL,NSM, CS, ES, ET */
/*xa*/ { ao, xl, xr, cn, cn, xa, xa, ao, ao, ao }, /* arabic letter          */
/*xr*/ { ro, xl, xr, ra, re, xa, xr, ro, ro, rt }, /* right letter           */
/*xl*/ { lo, xl, xr, la, le, xa, xl, lo, lo, lt }, /* left letter            */

/*ao*/ { ao, xl, xr, cn, cn, xa, ao, ao, ao, ao }, /* arabic lett. foll by ON*/
/*ro*/ { ro, xl, xr, ra, re, xa, ro, ro, ro, rt }, /* right lett. foll by ON */
/*lo*/ { lo, xl, xr, la, le, xa, lo, lo, lo, lt }, /* left lett. foll by ON  */

/*rt*/ { ro, xl, xr, ra, re, xa, rt, ro, ro, rt }, /* ET following R         */
/*lt*/ { lo, xl, xr, la, le, xa, lt, lo, lo, lt }, /* ET following L         */

/*cn*/ { ao, xl, xr, cn, cn, xa, cn, ac, ao, ao }, /* EN, AN following AL    */
/*ra*/ { ro, xl, xr, ra, re, xa, ra, rc, ro, rt }, /* arabic number foll R   */
/*re*/ { ro, xl, xr, ra, re, xa, re, rs, rs,ret }, /* european number foll R */
/*la*/ { lo, xl, xr, la, le, xa, la, lc, lo, lt }, /* arabic number foll L   */
/*le*/ { lo, xl, xr, la, le, xa, le, ls, ls,let }, /* european number foll L */

/*ac*/ { ao, xl, xr, cn, cn, xa, ao, ao, ao, ao }, /* CS following cn        */
/*rc*/ { ro, xl, xr, ra, re, xa, ro, ro, ro, rt }, /* CS following ra        */
/*rs*/ { ro, xl, xr, ra, re, xa, ro, ro, ro, rt }, /* CS,ES following re     */
/*lc*/ { lo, xl, xr, la, le, xa, lo, lo, lo, lt }, /* CS following la        */
/*ls*/ { lo, xl, xr, la, le, xa, lo, lo, lo, lt }, /* CS,ES following le     */

/*ret*/{ ro, xl, xr, ra, re, xa,ret, ro, ro,ret }, /* ET following re        */
/*let*/{ lo, xl, xr, la, le, xa,let, lo, lo,let }, /* ET following le        */
};

enum actions /* possible actions */
{
    /* primitives */
    IX = 0x100,                    /* increment */
    XX = 0xF,                    /* no-op */

    /* actions */
    xxx = (XX << 4) + XX,        /* no-op */
    xIx = IX + xxx,                /* increment run */
    xxN = (XX << 4) + ON,        /* set current to N */
    xxE = (XX << 4) + EN,        /* set current to EN */
    xxA = (XX << 4) + AN,        /* set current to AN */
    xxR = (XX << 4) + R,        /* set current to R */
    xxL = (XX << 4) + L,        /* set current to L */
    Nxx = (ON << 4) + 0xF,        /* set run to neutral */
    Axx = (AN << 4) + 0xF,        /* set run to AN */
    ExE = (EN << 4) + EN,        /* set run to EN, set current to EN */
    NIx = (ON << 4) + 0xF + IX, /* set run to N, increment */
    NxN = (ON << 4) + ON,        /* set run to N, set current to N */
    NxR = (ON << 4) + R,        /* set run to N, set current to R */
    NxE = (ON << 4) + EN,        /* set run to N, set current to EN */

    AxA = (AN << 4) + AN,        /* set run to AN, set current to AN */
    NxL = (ON << 4) + L,        /* set run to N, set current to L */
    LxL = (L << 4) + L,            /* set run to L, set current to L */
}  ;

static const int actionWeak[][10] =
{
       /*  N,   L,   R,  AN,  EN,  AL, NSM,  CS,  ES,  ET */
/*xa*/ { xxx, xxx, xxx, xxx, xxA, xxR, xxR, xxN, xxN, xxN }, /* arabic letter           */
/*xr*/ { xxx, xxx, xxx, xxx, xxE, xxR, xxR, xxN, xxN, xIx }, /* right letter            */
/*xl*/ { xxx, xxx, xxx, xxx, xxL, xxR, xxL, xxN, xxN, xIx }, /* left letter             */

/*ao*/ { xxx, xxx, xxx, xxx, xxA, xxR, xxN, xxN, xxN, xxN }, /* arabic lett. foll by ON */
/*ro*/ { xxx, xxx, xxx, xxx, xxE, xxR, xxN, xxN, xxN, xIx }, /* right lett. foll by ON  */
/*lo*/ { xxx, xxx, xxx, xxx, xxL, xxR, xxN, xxN, xxN, xIx }, /* left lett. foll by ON   */

/*rt*/ { Nxx, Nxx, Nxx, Nxx, ExE, NxR, xIx, NxN, NxN, xIx }, /* ET following R         */
/*lt*/ { Nxx, Nxx, Nxx, Nxx, LxL, NxR, xIx, NxN, NxN, xIx }, /* ET following L         */

/*cn*/ { xxx, xxx, xxx, xxx, xxA, xxR, xxA, xIx, xxN, xxN }, /* EN, AN following  AL    */
/*ra*/ { xxx, xxx, xxx, xxx, xxE, xxR, xxA, xIx, xxN, xIx }, /* arabic number foll R   */
/*re*/ { xxx, xxx, xxx, xxx, xxE, xxR, xxE, xIx, xIx, xxE }, /* european number foll R */
/*la*/ { xxx, xxx, xxx, xxx, xxL, xxR, xxA, xIx, xxN, xIx }, /* arabic number foll L   */
/*le*/ { xxx, xxx, xxx, xxx, xxL, xxR, xxL, xIx, xIx, xxL }, /* european number foll L */

/*ac*/ { Nxx, Nxx, Nxx, Axx, AxA, NxR, NxN, NxN, NxN, NxN }, /* CS following cn         */
/*rc*/ { Nxx, Nxx, Nxx, Axx, NxE, NxR, NxN, NxN, NxN, NIx }, /* CS following ra         */
/*rs*/ { Nxx, Nxx, Nxx, Nxx, ExE, NxR, NxN, NxN, NxN, NIx }, /* CS,ES following re      */
/*lc*/ { Nxx, Nxx, Nxx, Axx, NxL, NxR, NxN, NxN, NxN, NIx }, /* CS following la         */
/*ls*/ { Nxx, Nxx, Nxx, Nxx, LxL, NxR, NxN, NxN, NxN, NIx }, /* CS,ES following le      */

/*ret*/{ xxx, xxx, xxx, xxx, xxE, xxR, xxE, xxN, xxN, xxE }, /* ET following re            */
/*let*/{ xxx, xxx, xxx, xxx, xxL, xxR, xxL, xxN, xxN, xxL }, /* ET following le            */
};

static int GetDeferredType(int action)
{
    return (action >> 4) & 0xF;
}

static int GetResolvedType(int action)
{
    return action & 0xF;
}

/* Note on action table:

  States can be of two kinds:
     - Immediate Resolution State, where each input token
       is resolved as soon as it is seen. These states have
       only single action codes (xxN) or the no-op (xxx)
       for static input tokens.
     - Deferred Resolution State, where input tokens either
       either extend the run (xIx) or resolve its Type (e.g. Nxx).

   Input classes are of three kinds
     - Static Input Token, where the class of the token remains
       unchanged on output (AN, L, N, R)
     - Replaced Input Token, where the class of the token is
       always replaced on output (AL, BN, NSM, CS, ES, ET)
     - Conditional Input Token, where the class of the token is
       changed on output in some, but not all, cases (EN)

     Where tokens are subject to change, a double action
     (e.g. NxA, or NxN) is _required_ after deferred states,
     resolving both the deferred state and changing the current token.
*/

/*------------------------------------------------------------------------
    Function: resolveWeak

    Resolves the directionality of numeric and other weak character types

    Implements rules X10 and W1-W6 of the Unicode Bidirectional Algorithm.

    Input: Array of embedding levels
           Character count

    In/Out: Array of directional classes

    Note: On input only these directional classes are expected
          AL, HL, R, L,  ON, BN, NSM, AN, EN, ES, ET, CS,
------------------------------------------------------------------------*/
static void resolveWeak(int baselevel, WORD *pcls, WORD *plevel, int cch)
{
    int state = odd(baselevel) ? xr : xl;
    int cls;

    int level = baselevel;
    int action, clsRun, clsNew;
    int cchRun = 0;
    int ich = 0;

    for (; ich < cch; ich++)
    {
        /* ignore boundary neutrals */
        if (pcls[ich] == BN)
        {
            /* must flatten levels unless at a level change; */
            plevel[ich] = level;

            /* lookahead for level changes */
            if (ich + 1 == cch && level != baselevel)
            {
                /* have to fixup last BN before end of the loop, since
                 * its fix-upped value will be needed below the assert */
                pcls[ich] = EmbeddingDirection(level);
            }
            else if (ich + 1 < cch && level != plevel[ich+1] && pcls[ich+1] != BN)
            {
                /* fixup LAST BN in front / after a level run to make
                 * it act like the SOR/EOR in rule X10 */
                int newlevel = plevel[ich+1];
                if (level > newlevel) {
                    newlevel = level;
                }
                plevel[ich] = newlevel;

                /* must match assigned level */
                pcls[ich] = EmbeddingDirection(newlevel);
                level = plevel[ich+1];
            }
            else
            {
                /* don't interrupt runs */
                if (cchRun)
                {
                    cchRun++;
                }
                continue;
            }
        }

        ASSERT(pcls[ich] <= BN);
        cls = pcls[ich];

        action = actionWeak[state][cls];

        /* resolve the directionality for deferred runs */
        clsRun = GetDeferredType(action);
        if (clsRun != XX)
        {
            SetDeferredRun(pcls, cchRun, ich, clsRun);
            cchRun = 0;
        }

        /* resolve the directionality class at the current location */
        clsNew = GetResolvedType(action);
        if (clsNew != XX)
            pcls[ich] = clsNew;

        /* increment a deferred run */
        if (IX & action)
            cchRun++;

        state = stateWeak[state][cls];
    }

    /* resolve any deferred runs
     * use the direction of the current level to emulate PDF */
    cls = EmbeddingDirection(level);

    /* resolve the directionality for deferred runs */
    clsRun = GetDeferredType(actionWeak[state][cls]);
    if (clsRun != XX)
        SetDeferredRun(pcls, cchRun, ich, clsRun);
}

/* RESOLVE NEUTRAL TYPES */

/* action values */
enum neutralactions
{
    /* action to resolve previous input */
    nL = L,         /* resolve EN to L */
    En = 3 << 4,    /* resolve neutrals run to embedding level direction */
    Rn = R << 4,    /* resolve neutrals run to strong right */
    Ln = L << 4,    /* resolved neutrals run to strong left */
    In = (1<<8),    /* increment count of deferred neutrals */
    LnL = (1<<4)+L, /* set run and EN to L */
};

static int GetDeferredNeutrals(int action, int level)
{
    action = (action >> 4) & 0xF;
    if (action == (En >> 4))
        return EmbeddingDirection(level);
    else
        return action;
}

static int GetResolvedNeutrals(int action)
{
    action = action & 0xF;
    if (action == In)
        return 0;
    else
        return action;
}

/* state values */
enum resolvestates
{
    /* new temporary class */
    r,  /* R and characters resolved to R */
    l,  /* L and characters resolved to L */
    rn, /* N preceded by right */
    ln, /* N preceded by left */
    a,  /* AN preceded by left (the abbreviation 'la' is used up above) */
    na, /* N preceded by a */
} ;


/*------------------------------------------------------------------------
  Notes:

  By rule W7, whenever a EN is 'dominated' by an L (including start of
  run with embedding direction = L) it is resolved to, and further treated
  as L.

  This leads to the need for 'a' and 'na' states.
------------------------------------------------------------------------*/

static const int actionNeutrals[][5] =
{
/*   N,  L,  R,  AN, EN = cls */
  { In,  0,  0,  0,  0 }, /* r    right */
  { In,  0,  0,  0,  L }, /* l    left */

  { In, En, Rn, Rn, Rn }, /* rn   N preceded by right */
  { In, Ln, En, En, LnL}, /* ln   N preceded by left */

  { In,  0,  0,  0,  L }, /* a   AN preceded by left */
  { In, En, Rn, Rn, En }, /* na   N  preceded by a */
} ;

static const int stateNeutrals[][5] =
{
/*   N, L,  R, AN, EN */
  { rn, l,  r,  r,  r }, /* r   right */
  { ln, l,  r,  a,  l }, /* l   left */

  { rn, l,  r,  r,  r }, /* rn  N preceded by right */
  { ln, l,  r,  a,  l }, /* ln  N preceded by left */

  { na, l,  r,  a,  l }, /* a  AN preceded by left */
  { na, l,  r,  a,  l }, /* na  N preceded by la */
} ;

/*------------------------------------------------------------------------
    Function: resolveNeutrals

    Resolves the directionality of neutral character types.

    Implements rules W7, N1 and N2 of the Unicode Bidi Algorithm.

    Input: Array of embedding levels
           Character count
           Baselevel

    In/Out: Array of directional classes

    Note: On input only these directional classes are expected
          R,  L,  N, AN, EN and BN

          W8 resolves a number of ENs to L
------------------------------------------------------------------------*/
static void resolveNeutrals(int baselevel, WORD *pcls, const WORD *plevel, int cch)
{
    /* the state at the start of text depends on the base level */
    int state = odd(baselevel) ? r : l;
    int cls;

    int cchRun = 0;
    int level = baselevel;

    int action, clsRun, clsNew;
    int ich = 0;
    for (; ich < cch; ich++)
    {
        /* ignore boundary neutrals */
        if (pcls[ich] == BN)
        {
            /* include in the count for a deferred run */
            if (cchRun)
                cchRun++;

            /* skip any further processing */
            continue;
        }

        ASSERT(pcls[ich] < 5); /* "Only N, L, R,  AN, EN are allowed" */
        cls = pcls[ich];

        action = actionNeutrals[state][cls];

        /* resolve the directionality for deferred runs */
        clsRun = GetDeferredNeutrals(action, level);
        if (clsRun != N)
        {
            SetDeferredRun(pcls, cchRun, ich, clsRun);
            cchRun = 0;
        }

        /* resolve the directionality class at the current location */
        clsNew = GetResolvedNeutrals(action);
        if (clsNew != N)
            pcls[ich] = clsNew;

        if (In & action)
            cchRun++;

        state = stateNeutrals[state][cls];
        level = plevel[ich];
    }

    /* resolve any deferred runs */
    cls = EmbeddingDirection(level);    /* eor has type of current level */

    /* resolve the directionality for deferred runs */
    clsRun = GetDeferredNeutrals(actionNeutrals[state][cls], level);
    if (clsRun != N)
        SetDeferredRun(pcls, cchRun, ich, clsRun);
}

/* RESOLVE IMPLICIT */

/*------------------------------------------------------------------------
    Function: resolveImplicit

    Recursively resolves implicit embedding levels.
    Implements rules I1 and I2 of the Unicode Bidirectional Algorithm.

    Input: Array of direction classes
           Character count
           Base level

    In/Out: Array of embedding levels

    Note: levels may exceed 15 on output.
          Accepted subset of direction classes
          R, L, AN, EN
------------------------------------------------------------------------*/
static const WORD addLevel[][4] =
{
          /* L,  R, AN, EN */
/* even */ { 0,  1,  2,  2, },
/* odd  */ { 1,  0,  1,  1, }

};

static void resolveImplicit(const WORD * pcls, WORD *plevel, int cch)
{
    int ich = 0;
    for (; ich < cch; ich++)
    {
        /* cannot resolve bn here, since some bn were resolved to strong
         * types in resolveWeak. To remove these we need the original
         * types, which are available again in resolveWhiteSpace */
        if (pcls[ich] == BN)
        {
            continue;
        }
        ASSERT(pcls[ich] > 0); /* "No Neutrals allowed to survive here." */
        ASSERT(pcls[ich] < 5); /* "Out of range." */
        plevel[ich] += addLevel[odd(plevel[ich])][pcls[ich] - 1];
    }
}

/* REORDER */
/*------------------------------------------------------------------------
    Function: resolveLines

    Breaks a paragraph into lines

    Input:  Character count
    In/Out: Array of characters
            Array of line break flags

    Returns the count of characters on the first line

    Note: This function only breaks lines at hard line breaks. Other
    line breaks can be passed in. If pbrk[n] is TRUE, then a break
    occurs after the character in pszInput[n]. Breaks before the first
    character are not allowed.
------------------------------------------------------------------------*/
static int resolveLines(LPCWSTR pszInput, BOOL * pbrk, int cch)
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
static void resolveWhitespace(int baselevel, const WORD *pcls, WORD *plevel, int cch)
{
    int cchrun = 0;
    int oldlevel = baselevel;

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
    Functions: reorder/reorderLevel

    Recursively reorders the display string
    "From the highest level down, reverse all characters at that level and
    higher, down to the lowest odd level"

    Implements rule L2 of the Unicode bidi Algorithm.

    Input: Array of embedding levels
           Character count
           Flag enabling reversal (set to false by initial caller)

    In/Out: Text to reorder

    Note: levels may exceed 15 resp. 61 on input.

    Rule L3 - reorder combining marks is not implemented here
    Rule L4 - glyph mirroring is implemented as a display option below

    Note: this should be applied a line at a time
-------------------------------------------------------------------------*/
static int reorderLevel(int level, LPWSTR pszText, const WORD* plevel, int cch, BOOL fReverse)
{
    int ich = 0;

    /* true as soon as first odd level encountered */
    fReverse = fReverse || odd(level);

    for (; ich < cch; ich++)
    {
        if (plevel[ich] < level)
        {
            break;
        }
        else if (plevel[ich] > level)
        {
            ich += reorderLevel(level + 1, pszText + ich, plevel + ich,
                cch - ich, fReverse) - 1;
        }
    }
    if (fReverse)
    {
        reverse(pszText, ich);
    }
    return ich;
}

static int reorder(int baselevel, LPWSTR pszText, const WORD* plevel, int cch)
{
    int ich = 0;

    while (ich < cch)
    {
        ich += reorderLevel(baselevel, pszText + ich, plevel + ich,
            cch - ich, FALSE);
    }
    return ich;
}

/* DISPLAY OPTIONS */
/*-----------------------------------------------------------------------
   Function:    mirror

    Crudely implements rule L4 of the Unicode Bidirectional Algorithm
    Demonstrate mirrored brackets, braces and parens


    Input:    Array of levels
            Count of characters

    In/Out:    Array of characters (should be array of glyph ids)

    Note;
    A full implementation would need to substitute mirrored glyphs even
    for characters that are not paired (e.g. integral sign).
-----------------------------------------------------------------------*/
static void mirror(LPWSTR pszInput, const WORD* plevel, int cch)
{
    static int warn_once;
    int i;

    for (i = 0; i < cch; ++i)
    {
        if (!odd(plevel[i]))
            continue;
        /* This needs the data from http://www.unicode.org/Public/UNIDATA/BidiMirroring.txt */
        if (!warn_once++)
            FIXME("stub: mirroring of characters not yet implemented\n");
        break;
    }
}

/*------------------------------------------------------------------------
    Function: BidiLines

    Implements the Line-by-Line phases of the Unicode Bidi Algorithm

      Input:     Count of characters
             flag whether to mirror

    Inp/Out: Input text
             Array of character directions
             Array of levels

------------------------------------------------------------------------*/
static void BidiLines(int baselevel, LPWSTR pszOutLine, LPCWSTR pszLine, WORD * pclsLine,
                      WORD * plevelLine, int cchPara, int fMirror, BOOL * pbrk)
{
    int cchLine = 0;

    do
    {
        /* break lines at LS */
        cchLine = resolveLines(pszLine, pbrk, cchPara);

        /* resolve whitespace */
        resolveWhitespace(baselevel, pclsLine, plevelLine, cchLine);

        if (pszOutLine)
        {
            if (fMirror)
                mirror(pszOutLine, plevelLine, cchLine);

            /* reorder each line in place */
            reorder(baselevel, pszOutLine, plevelLine, cchLine);
        }

        pszLine += cchLine;
        plevelLine += cchLine;
        pbrk += pbrk ? cchLine : 0;
        pclsLine += cchLine;
        cchPara -= cchLine;

    } while (cchPara);
}

/*************************************************************
 *    BIDI_Reorder
 */
BOOL BIDI_Reorder(
                LPCWSTR lpString,       /* [in] The string for which information is to be returned */
                INT uCount,     /* [in] Number of WCHARs in string. */
                DWORD dwFlags,  /* [in] GetCharacterPlacement compatible flags specifying how to process the string */
                DWORD dwWineGCP_Flags,       /* [in] Wine internal flags - Force paragraph direction */
                LPWSTR lpOutString, /* [out] Reordered string */
                INT uCountOut,  /* [in] Size of output buffer */
                UINT *lpOrder /* [out] Logical -> Visual order map */
    )
{
    WORD *levels;
    WORD *chartype;
    unsigned i, baselevel = 0, done;
    TRACE("%s, %d, 0x%08x lpOutString=%p, lpOrder=%p\n",
          debugstr_wn(lpString, uCount), uCount, dwFlags,
          lpOutString, lpOrder);

    if (!(dwFlags & GCP_REORDER))
    {
        FIXME("Asked to reorder without reorder flag set\n");
        return FALSE;
    }

    if (uCountOut < uCount)
    {
        FIXME("lpOutString too small\n");
        return FALSE;
    }

    chartype = HeapAlloc(GetProcessHeap(), 0, uCount * 2 * sizeof(WORD));
    levels = chartype + uCount;
    if (!chartype)
    {
        WARN("Out of memory\n");
        return FALSE;
    }

    if (lpOutString)
        memcpy(lpOutString, lpString, uCount * sizeof(WCHAR));

    if (WINE_GCPW_FORCE_RTL == (dwWineGCP_Flags&WINE_GCPW_DIR_MASK))
        baselevel = 1;

    done = 0;
    while (done < uCount)
    {
        unsigned j;
        classify(lpString + done, chartype, uCount - done);
        /* limit text to first block */
        i = resolveParagraphs(chartype, uCount - done);
        for (j = 0; j < i; ++j)
            switch(chartype[j])
            {
                case B:
                case S:
                case WS:
                case ON: chartype[j] = N;
                default: continue;
            }

        if ((dwWineGCP_Flags&WINE_GCPW_DIR_MASK) == WINE_GCPW_LOOSE_RTL)
            baselevel = 1;
        else if ((dwWineGCP_Flags&WINE_GCPW_DIR_MASK) == WINE_GCPW_LOOSE_LTR)
            baselevel = 0;

        if (dwWineGCP_Flags & WINE_GCPW_LOOSE_MASK)
        {
            for (j = 0; j < i; ++j)
                if (chartype[j] == L)
                {
                    baselevel = 0;
                    break;
                }
                else if (chartype[j] == R || chartype[j] == AL)
                {
                    baselevel = 1;
                    break;
                }
        }

        /* resolve explicit */
        resolveExplicit(baselevel, N, chartype, levels, i, 0);

        /* resolve weak */
        resolveWeak(baselevel, chartype, levels, i);

        /* resolve neutrals */
        resolveNeutrals(baselevel, chartype, levels, i);

        /* resolveImplicit */
        resolveImplicit(chartype, levels, i);

        /* assign directional types again, but for WS, S this time */
        classify(lpString + done, chartype, i);

        BidiLines(baselevel, lpOutString ? lpOutString + done : NULL, lpString + done,
                    chartype, levels, i, !(dwFlags & GCP_SYMSWAPOFF), 0);

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
        done += i;
    }

    HeapFree(GetProcessHeap(), 0, chartype);
    return TRUE;
}

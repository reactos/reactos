/*
 * Uniscribe BiDirectional handling
 *
 * Copyright 2003 Shachar Shemesh
 * Copyright 2007 Maarten Lankhorst
 * Copyright 2010 CodeWeavers, Aric Stewart
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

#include <stdarg.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "usp10.h"
#include "wine/debug.h"
#include "wine/list.h"

#include "usp10_internal.h"

extern const unsigned short bidi_bracket_table[];
extern const unsigned short bidi_direction_table[];

WINE_DEFAULT_DEBUG_CHANNEL(bidi);

#define ASSERT(x) do { if (!(x)) FIXME("assert failed: %s\n", #x); } while(0)
#define MAX_DEPTH 125

/* HELPER FUNCTIONS AND DECLARATIONS */

/*------------------------------------------------------------------------
    Bidirectional Character Types

    as defined by the Unicode Bidirectional Algorithm Table 3-7.

    Note:

      The list of bidirectional character types here is not grouped the
      same way as the table 3-7, since the numeric values for the types
      are chosen to keep the state and action tables compact.
------------------------------------------------------------------------*/
enum directions
{
    /* input types */
             /* ON MUST be zero, code relies on ON = NI = 0 */
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
    NI = ON,  /* alias, where ON, WS, S  and Isolates are treated the same */
};

static const char debug_type[][4] =
{
    "ON",      /* Other Neutral */
    "L",       /* Left Letter */
    "R",       /* Right Letter */
    "AN",      /* Arabic Number */
    "EN",      /* European Number */
    "AL",      /* Arabic Letter (Right-to-left) */
    "NSM",     /* Non-spacing Mark */
    "CS",      /* Common Separator */
    "ES",      /* European Separator */
    "ET",      /* European Terminator (post/prefix e.g. $ and %) */
    "BN",      /* Boundary neutral (type of RLE etc after explicit levels) */
    "S",       /* Segment Separator (TAB)        // used only in L1 */
    "WS",      /* White space                    // used only in L1 */
    "B",       /* Paragraph Separator (aka as PS) */
    "RLO",     /* these are used only in X1-X9 */
    "RLE",
    "LRO",
    "LRE",
    "PDF",
    "LRI",     /* Isolate formatting characters new with 6.3 */
    "RLI",
    "FSI",
    "PDI",
};

/* HELPER FUNCTIONS */

static inline void dump_types(const char* header, WORD *types, int start, int end)
{
    int i, len = 0;
    TRACE("%s:",header);
    for (i = start; i < end && len < 200; i++)
    {
        TRACE(" %s",debug_type[types[i]]);
        len += strlen(debug_type[types[i]])+1;
    }
    if (i != end)
        TRACE("...");
    TRACE("\n");
}

/* Convert the libwine information to the direction enum */
static void classify(const WCHAR *string, WORD *chartype, DWORD count, const SCRIPT_CONTROL *c)
{
    unsigned i;

    for (i = 0; i < count; ++i)
    {
        chartype[i] = get_table_entry_32( bidi_direction_table, string[i] );
        if (c->fLegacyBidiClass && chartype[i] == ES)
        {
            if (string[i] == '+' || string[i] == '-') chartype[i] = NI;
        }
    }
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
typedef struct tagStackItem {
    int level;
    int override;
    BOOL isolate;
} StackItem;

#define push_stack(l,o,i)  \
  do { stack_top--; \
  stack[stack_top].level = l; \
  stack[stack_top].override = o; \
  stack[stack_top].isolate = i;} while(0)

#define pop_stack() do { stack_top++; } while(0)

#define valid_level(x) (x <= MAX_DEPTH && overflow_isolate_count == 0 && overflow_embedding_count == 0)

static void resolveExplicit(int level, WORD *pclass, WORD *poutLevel, WORD *poutOverrides, int count, BOOL initialOverride)
{
    /* X1 */
    int overflow_isolate_count = 0;
    int overflow_embedding_count = 0;
    int valid_isolate_count = 0;
    int i;

    StackItem stack[MAX_DEPTH+2];
    int stack_top = MAX_DEPTH+1;

    stack[stack_top].level = level;
    stack[stack_top].override = NI;
    stack[stack_top].isolate = FALSE;

    if (initialOverride)
    {
        if (odd(level))
            push_stack(level, R, FALSE);
        else
            push_stack(level, L, FALSE);
    }

    for (i = 0; i < count; i++)
    {
        poutOverrides[i] = stack[stack_top].override;

        /* X2 */
        if (pclass[i] == RLE)
        {
            int least_odd = GreaterOdd(stack[stack_top].level);
            poutLevel[i] = stack[stack_top].level;
            if (valid_level(least_odd))
                push_stack(least_odd, NI, FALSE);
            else if (overflow_isolate_count == 0)
                overflow_embedding_count++;
        }
        /* X3 */
        else if (pclass[i] == LRE)
        {
            int least_even = GreaterEven(stack[stack_top].level);
            poutLevel[i] = stack[stack_top].level;
            if (valid_level(least_even))
                push_stack(least_even, NI, FALSE);
            else if (overflow_isolate_count == 0)
                overflow_embedding_count++;
        }
        /* X4 */
        else if (pclass[i] == RLO)
        {
            int least_odd = GreaterOdd(stack[stack_top].level);
            poutLevel[i] = stack[stack_top].level;
            if (valid_level(least_odd))
                push_stack(least_odd, R, FALSE);
            else if (overflow_isolate_count == 0)
                overflow_embedding_count++;
        }
        /* X5 */
        else if (pclass[i] == LRO)
        {
            int least_even = GreaterEven(stack[stack_top].level);
            poutLevel[i] = stack[stack_top].level;
            if (valid_level(least_even))
                push_stack(least_even, L, FALSE);
            else if (overflow_isolate_count == 0)
                overflow_embedding_count++;
        }
        /* X5a */
        else if (pclass[i] == RLI)
        {
            int least_odd = GreaterOdd(stack[stack_top].level);
            poutLevel[i] = stack[stack_top].level;
            if (valid_level(least_odd))
            {
                valid_isolate_count++;
                push_stack(least_odd, NI, TRUE);
            }
            else
                overflow_isolate_count++;
        }
        /* X5b */
        else if (pclass[i] == LRI)
        {
            int least_even = GreaterEven(stack[stack_top].level);
            poutLevel[i] = stack[stack_top].level;
            if (valid_level(least_even))
            {
                valid_isolate_count++;
                push_stack(least_even, NI, TRUE);
            }
            else
                overflow_isolate_count++;
        }
        /* X5c */
        else if (pclass[i] == FSI)
        {
            int j;
            int new_level = 0;
            int skipping = 0;
            poutLevel[i] = stack[stack_top].level;
            for (j = i+1; j < count; j++)
            {
                if (pclass[j] == LRI || pclass[j] == RLI || pclass[j] == FSI)
                {
                    skipping++;
                    continue;
                }
                else if (pclass[j] == PDI)
                {
                    if (skipping)
                        skipping --;
                    else
                        break;
                    continue;
                }

                if (skipping) continue;

                if (pclass[j] == L)
                {
                    new_level = 0;
                    break;
                }
                else if (pclass[j] == R || pclass[j] == AL)
                {
                    new_level = 1;
                    break;
                }
            }
            if (odd(new_level))
            {
                int least_odd = GreaterOdd(stack[stack_top].level);
                if (valid_level(least_odd))
                {
                    valid_isolate_count++;
                    push_stack(least_odd, NI, TRUE);
                }
                else
                    overflow_isolate_count++;
            }
            else
            {
                int least_even = GreaterEven(stack[stack_top].level);
                if (valid_level(least_even))
                {
                    valid_isolate_count++;
                    push_stack(least_even, NI, TRUE);
                }
                else
                    overflow_isolate_count++;
            }
        }
        /* X6 */
        else if (pclass[i] != B && pclass[i] != BN && pclass[i] != PDI && pclass[i] != PDF)
        {
            poutLevel[i] = stack[stack_top].level;
            if (stack[stack_top].override != NI)
                pclass[i] = stack[stack_top].override;
        }
        /* X6a */
        else if (pclass[i] == PDI)
        {
            if (overflow_isolate_count) overflow_isolate_count--;
            else if (!valid_isolate_count) {/* do nothing */}
            else
            {
                overflow_embedding_count = 0;
                while (!stack[stack_top].isolate) pop_stack();
                pop_stack();
                valid_isolate_count --;
            }
            poutLevel[i] = stack[stack_top].level;
        }
        /* X7 */
        else if (pclass[i] == PDF)
        {
            poutLevel[i] = stack[stack_top].level;
            if (overflow_isolate_count) {/* do nothing */}
            else if (overflow_embedding_count) overflow_embedding_count--;
            else if (!stack[stack_top].isolate && stack_top < (MAX_DEPTH+1))
                pop_stack();
        }
        /* X8: Nothing */
    }
    /* X9: Based on 5.2 Retaining Explicit Formatting Characters */
    for (i = 0; i < count ; i++)
        if (pclass[i] == RLE || pclass[i] == LRE || pclass[i] == RLO || pclass[i] == LRO || pclass[i] == PDF)
            pclass[i] = BN;
}

static inline int previousValidChar(const WORD *pcls, int index, int back_fence)
{
    if (index == -1 || index == back_fence) return index;
    index --;
    while (index > back_fence && pcls[index] == BN) index --;
    return index;
}

static inline int nextValidChar(const WORD *pcls, int index, int front_fence)
{
    if (index == front_fence) return index;
    index ++;
    while (index < front_fence && pcls[index] == BN) index ++;
    return index;
}

typedef struct tagRun
{
    int start;
    int end;
    WORD e;
} Run;

typedef struct tagRunChar
{
    WCHAR ch;
    WORD *pcls;
} RunChar;

typedef struct tagIsolatedRun
{
    struct list entry;
    int length;
    WORD sos;
    WORD eos;
    WORD e;

    RunChar item[1];
} IsolatedRun;

static inline int iso_nextValidChar(IsolatedRun *iso_run, int index)
{
    if (index >= (iso_run->length-1)) return -1;
    index ++;
    while (index < iso_run->length && *iso_run->item[index].pcls == BN) index++;
    if (index == iso_run->length) return -1;
    return index;
}

static inline int iso_previousValidChar(IsolatedRun *iso_run, int index)
{

    if (index <= 0) return -1;
    index --;
    while (index > -1 && *iso_run->item[index].pcls == BN) index--;
    return index;
}

static inline void iso_dump_types(const char* header, IsolatedRun *iso_run)
{
    int i, len = 0;
    TRACE("%s:",header);
    TRACE("[ ");
    for (i = 0; i < iso_run->length && len < 200; i++)
    {
        TRACE(" %s",debug_type[*iso_run->item[i].pcls]);
        len += strlen(debug_type[*iso_run->item[i].pcls])+1;
    }
    if (i != iso_run->length)
        TRACE("...");
    TRACE(" ]\n");
}

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

static void resolveWeak(IsolatedRun * iso_run)
{
    int i;

    /* W1 */
    for (i=0; i < iso_run->length; i++)
    {
        if (*iso_run->item[i].pcls == NSM)
        {
            int j = iso_previousValidChar(iso_run, i);
            if (j == -1)
                *iso_run->item[i].pcls = iso_run->sos;
            else if (*iso_run->item[j].pcls >= LRI)
                *iso_run->item[i].pcls = ON;
            else
                *iso_run->item[i].pcls = *iso_run->item[j].pcls;
        }
    }

    /* W2 */
    for (i = 0; i < iso_run->length; i++)
    {
        if (*iso_run->item[i].pcls == EN)
        {
            int j = iso_previousValidChar(iso_run, i);
            while (j > -1)
            {
                if (*iso_run->item[j].pcls == R || *iso_run->item[j].pcls == L || *iso_run->item[j].pcls == AL)
                {
                    if (*iso_run->item[j].pcls == AL)
                        *iso_run->item[i].pcls = AN;
                    break;
                }
                j = iso_previousValidChar(iso_run, j);
            }
        }
    }

    /* W3 */
    for (i = 0; i < iso_run->length; i++)
    {
        if (*iso_run->item[i].pcls == AL)
            *iso_run->item[i].pcls = R;
    }

    /* W4 */
    for (i = 0; i < iso_run->length; i++)
    {
        if (*iso_run->item[i].pcls == ES)
        {
            int b = iso_previousValidChar(iso_run, i);
            int f = iso_nextValidChar(iso_run, i);

            if (b > -1 && f > -1 && *iso_run->item[b].pcls == EN && *iso_run->item[f].pcls == EN)
                *iso_run->item[i].pcls = EN;
        }
        else if (*iso_run->item[i].pcls == CS)
        {
            int b = iso_previousValidChar(iso_run, i);
            int f = iso_nextValidChar(iso_run, i);

            if (b > -1 && f > -1 && *iso_run->item[b].pcls == EN && *iso_run->item[f].pcls == EN)
                *iso_run->item[i].pcls = EN;
            else if (b > -1 && f > -1 && *iso_run->item[b].pcls == AN && *iso_run->item[f].pcls == AN)
                *iso_run->item[i].pcls = AN;
        }
    }

    /* W5 */
    for (i = 0; i < iso_run->length; i++)
    {
        if (*iso_run->item[i].pcls == ET)
        {
            int j;
            for (j = i-1 ; j > -1; j--)
            {
                if (*iso_run->item[j].pcls == BN) continue;
                if (*iso_run->item[j].pcls == ET) continue;
                else if (*iso_run->item[j].pcls == EN) *iso_run->item[i].pcls = EN;
                else break;
            }
            if (*iso_run->item[i].pcls == ET)
            {
                for (j = i+1; j < iso_run->length; j++)
                {
                    if (*iso_run->item[j].pcls == BN) continue;
                    if (*iso_run->item[j].pcls == ET) continue;
                    else if (*iso_run->item[j].pcls == EN) *iso_run->item[i].pcls = EN;
                    else break;
                }
            }
        }
    }

    /* W6 */
    for (i = 0; i < iso_run->length; i++)
    {
        if (*iso_run->item[i].pcls == ET || *iso_run->item[i].pcls == ES || *iso_run->item[i].pcls == CS || *iso_run->item[i].pcls == ON)
        {
            int b = i-1;
            int f = i+1;
            if (b > -1 && *iso_run->item[b].pcls == BN)
                *iso_run->item[b].pcls = ON;
            if (f < iso_run->length && *iso_run->item[f].pcls == BN)
                *iso_run->item[f].pcls = ON;

            *iso_run->item[i].pcls = ON;
        }
    }

    /* W7 */
    for (i = 0; i < iso_run->length; i++)
    {
        if (*iso_run->item[i].pcls == EN)
        {
            int j;
            for (j = iso_previousValidChar(iso_run, i); j > -1; j = iso_previousValidChar(iso_run, j))
                if (*iso_run->item[j].pcls == R || *iso_run->item[j].pcls == L)
                {
                    if (*iso_run->item[j].pcls == L)
                        *iso_run->item[i].pcls = L;
                    break;
                }
            if (iso_run->sos == L &&  j == -1)
                *iso_run->item[i].pcls = L;
        }
    }
}

typedef struct tagBracketPair
{
    int start;
    int end;
} BracketPair;

static int __cdecl compr(const void *a, const void* b)
{
    return ((BracketPair*)a)->start - ((BracketPair*)b)->start;
}

static BracketPair *computeBracketPairs(IsolatedRun *iso_run)
{
    WCHAR *open_stack;
    int *stack_index;
    int stack_top = iso_run->length;
    unsigned int pair_count = 0;
    BracketPair *out = NULL;
    SIZE_T out_size = 0;
    int i;

    open_stack = malloc(iso_run->length * sizeof(*open_stack));
    stack_index = malloc(iso_run->length * sizeof(*stack_index));

    for (i = 0; i < iso_run->length; i++)
    {
        unsigned short ubv = get_table_entry_16(bidi_bracket_table, iso_run->item[i].ch);

        if (!ubv)
            continue;

        if ((ubv >> 8) == 0)
        {
            --stack_top;
            open_stack[stack_top] = iso_run->item[i].ch + (signed char)(ubv & 0xff);
            /* Deal with canonical equivalent U+2329/232A and U+3008/3009. */
            if (open_stack[stack_top] == 0x232a)
                open_stack[stack_top] = 0x3009;
            stack_index[stack_top] = i;
        }
        else if ((ubv >> 8) == 1)
        {
            unsigned int j;

            for (j = stack_top; j < iso_run->length; ++j)
            {
                WCHAR c = iso_run->item[i].ch;

                if (c == 0x232a)
                    c = 0x3009;

                if (c != open_stack[j])
                    continue;

                if (!(usp10_array_reserve((void **)&out, &out_size, pair_count + 2, sizeof(*out))))
                    ERR("Failed to grow output array.\n");

                out[pair_count].start = stack_index[j];
                out[pair_count].end = i;
                ++pair_count;

                out[pair_count].start = -1;
                stack_top = j + 1;
                break;
            }
        }
    }

    free(open_stack);
    free(stack_index);

    if (!pair_count)
        return NULL;

    qsort(out, pair_count, sizeof(*out), compr);

    return out;
}

#define N0_TYPE(a) ((a == AN || a == EN)?R:a)

/*------------------------------------------------------------------------
    Function: resolveNeutrals

    Resolves the directionality of neutral character types.

    Implements rules N1 and N2 of the Unicode Bidi Algorithm.

    Input: Array of embedding levels
           Character count
           Baselevel

    In/Out: Array of directional classes

    Note: On input only these directional classes are expected
          R,  L,  NI, AN, EN and BN

          W8 resolves a number of ENs to L
------------------------------------------------------------------------*/
static void resolveNeutrals(IsolatedRun *iso_run)
{
    int i;
    BracketPair *pairs = NULL;

    /* Translate isolates into NI */
    for (i = 0; i < iso_run->length; i++)
    {
        if (*iso_run->item[i].pcls >= LRI)
            *iso_run->item[i].pcls = NI;

        switch(*iso_run->item[i].pcls)
        {
            case B:
            case S:
            case WS: *iso_run->item[i].pcls = NI;
        }

        ASSERT(*iso_run->item[i].pcls < 5 || *iso_run->item[i].pcls == BN); /* "Only NI, L, R,  AN, EN and BN are allowed" */
    }

    /* N0: Skipping bracketed pairs for now */
    pairs = computeBracketPairs(iso_run);
    if (pairs)
    {
        BracketPair *p = &pairs[0];
        int i = 0;
        while (p->start >= 0)
        {
            int j;
            int e = EmbeddingDirection(iso_run->e);
            int o = EmbeddingDirection(iso_run->e+1);
            BOOL flag_o = FALSE;
            TRACE("Bracket Pair [%i - %i]\n",p->start, p->end);

            /* N0.b */
            for (j = p->start+1; j < p->end; j++)
            {
                if (N0_TYPE(*iso_run->item[j].pcls) == e)
                {
                    *iso_run->item[p->start].pcls = e;
                    *iso_run->item[p->end].pcls = e;
                    break;
                }
                else if (N0_TYPE(*iso_run->item[j].pcls) == o)
                    flag_o = TRUE;
            }
            /* N0.c */
            if (j == p->end && flag_o)
            {
                for (j = p->start; j >= 0; j--)
                {
                    if (N0_TYPE(*iso_run->item[j].pcls) == o)
                    {
                        *iso_run->item[p->start].pcls = o;
                        *iso_run->item[p->end].pcls = o;
                        break;
                    }
                    else if (N0_TYPE(*iso_run->item[j].pcls) == e)
                    {
                        *iso_run->item[p->start].pcls = e;
                        *iso_run->item[p->end].pcls = e;
                        break;
                    }
                }
                if ( j < 0 )
                {
                    *iso_run->item[p->start].pcls = iso_run->sos;
                    *iso_run->item[p->end].pcls = iso_run->sos;
                }
            }

            i++;
            p = &pairs[i];
        }
        free(pairs);
    }

    /* N1 */
    for (i = 0; i < iso_run->length; i++)
    {
        WORD l,r;

        if (*iso_run->item[i].pcls == NI)
        {
            int j;
            int b = iso_previousValidChar(iso_run, i);

            if (b == -1)
            {
                l = iso_run->sos;
                b = 0;
            }
            else
            {
                if (*iso_run->item[b].pcls == R || *iso_run->item[b].pcls == AN || *iso_run->item[b].pcls == EN)
                    l = R;
                else if (*iso_run->item[b].pcls == L)
                    l = L;
                else /* No string type */
                    continue;
            }
            j = iso_nextValidChar(iso_run, i);
            while (j > -1 && *iso_run->item[j].pcls == NI) j = iso_nextValidChar(iso_run, j);

            if (j == -1)
            {
                r = iso_run->eos;
                j = iso_run->length;
            }
            else if (*iso_run->item[j].pcls == R || *iso_run->item[j].pcls == AN || *iso_run->item[j].pcls == EN)
                r = R;
            else if (*iso_run->item[j].pcls == L)
                r = L;
            else /* No string type */
                continue;

            if (r == l)
            {
                for (b = i; b < j && b < iso_run->length; b++)
                    *iso_run->item[b].pcls = r;
            }
        }
    }

    /* N2 */
    for (i = 0; i < iso_run->length; i++)
    {
        if (*iso_run->item[i].pcls == NI)
        {
            int b = i-1;
            int f = i+1;
            *iso_run->item[i].pcls = EmbeddingDirection(iso_run->e);
            if (b > -1 && *iso_run->item[b].pcls == BN)
                *iso_run->item[b].pcls = EmbeddingDirection(iso_run->e);
            if (f < iso_run->length && *iso_run->item[f].pcls == BN)
                *iso_run->item[f].pcls = EmbeddingDirection(iso_run->e);
        }
    }
}

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
static void resolveImplicit(const WORD * pcls, WORD *plevel, int sos, int eos)
{
    int i;

    /* I1/2 */
    for (i = sos; i <= eos; i++)
    {
        if (pcls[i] == BN)
            continue;

        ASSERT(pcls[i] > 0); /* "No Neutrals allowed to survive here." */
        ASSERT(pcls[i] < 5); /* "Out of range." */

        if (odd(plevel[i]) && (pcls[i] == L || pcls[i] == EN || pcls [i] == AN))
            plevel[i]++;
        else if (!odd(plevel[i]) && pcls[i] == R)
            plevel[i]++;
        else if (!odd(plevel[i]) && (pcls[i] == EN || pcls [i] == AN))
            plevel[i]+=2;
    }
}

static void resolveResolved(unsigned baselevel, const WORD * pcls, WORD *plevel, int sos, int eos)
{
    int i;

    /* L1 */
    for (i = sos; i <= eos; i++)
    {
        if (pcls[i] == B || pcls[i] == S)
        {
            int j = i -1;
            while (i > sos  && j >= sos &&
                   (pcls[j] == WS || pcls[j] == FSI || pcls[j] == LRI || pcls[j] == RLI ||
                    pcls[j] == PDI || pcls[j] == LRE || pcls[j] == RLE || pcls[j] == LRO ||
                    pcls[j] == RLO || pcls[j] == PDF || pcls[j] == BN))
                plevel[j--] = baselevel;
            plevel[i] = baselevel;
        }
        else if (pcls[i] == LRE || pcls[i] == RLE || pcls[i] == LRO || pcls[i] == RLO ||
                 pcls[i] == PDF || pcls[i] == BN)
        {
            plevel[i] = i ? plevel[i - 1] : baselevel;
        }
        if (i == eos &&
            (pcls[i] == WS || pcls[i] == FSI || pcls[i] == LRI || pcls[i] == RLI ||
             pcls[i] == PDI || pcls[i] == LRE || pcls[i] == RLE || pcls[i] == LRO ||
             pcls[i] == RLO || pcls[i] == PDF || pcls[i] == BN ))
        {
            int j = i;
            while (j >= sos && (pcls[j] == WS || pcls[j] == FSI || pcls[j] == LRI || pcls[j] == RLI ||
                                pcls[j] == PDI || pcls[j] == LRE || pcls[j] == RLE || pcls[j] == LRO ||
                                pcls[j] == RLO || pcls[j] == PDF || pcls[j] == BN))
                plevel[j--] = baselevel;
        }
    }
}

static void computeIsolatingRunsSet(unsigned baselevel, WORD *pcls, const WORD *pLevel,
        const WCHAR *string, unsigned int uCount, struct list *set)
{
    int run_start, run_end, i;
    int run_count = 0;
    Run *runs;
    IsolatedRun *current_isolated;

    if (!(runs = calloc(uCount, sizeof(*runs))))
        return;

    list_init(set);

    /* Build Runs */
    run_start = 0;
    while (run_start < uCount)
    {
        run_end = nextValidChar(pcls, run_start, uCount);
        while (run_end < uCount && pLevel[run_end] == pLevel[run_start]) run_end = nextValidChar(pcls, run_end, uCount);
        run_end --;
        runs[run_count].start = run_start;
        runs[run_count].end = run_end;
        runs[run_count].e = pLevel[run_start];
        run_start = nextValidChar(pcls, run_end, uCount);
        run_count++;
    }

    /* Build Isolating Runs */
    i = 0;
    while (i < run_count)
    {
        int k = i;
        if (runs[k].start >= 0)
        {
            int type_fence, real_end;
            int j;

            if (!(current_isolated = malloc(offsetof(IsolatedRun, item[uCount]))))
                break;

            run_start = runs[k].start;
            current_isolated->e = runs[k].e;
            current_isolated->length = (runs[k].end - runs[k].start)+1;

            for (j = 0; j < current_isolated->length;  j++)
            {
                current_isolated->item[j].pcls = &pcls[runs[k].start+j];
                current_isolated->item[j].ch = string[runs[k].start + j];
            }

            run_end = runs[k].end;

            TRACE("{ [%i -- %i]",run_start, run_end);

            if (pcls[run_end] == BN)
                run_end = previousValidChar(pcls, run_end, runs[k].start);

            while (run_end < uCount && (pcls[run_end] == RLI || pcls[run_end] == LRI || pcls[run_end] == FSI))
            {
                j = k+1;
search:
                while (j < run_count && pcls[runs[j].start] != PDI) j++;
                if (j < run_count && runs[i].e != runs[j].e)
                {
                    j++;
                    goto search;
                }

                if (j != run_count)
                {
                    int m;
                    int l = current_isolated->length;

                    current_isolated->length += (runs[j].end - runs[j].start)+1;
                    for (m = 0; l < current_isolated->length; l++, m++)
                    {
                        current_isolated->item[l].pcls = &pcls[runs[j].start+m];
                        current_isolated->item[l].ch = string[runs[j].start + m];
                    }

                    TRACE("[%i -- %i]",runs[j].start, runs[j].end);

                    run_end = runs[j].end;
                    if (pcls[run_end] == BN)
                        run_end = previousValidChar(pcls, run_end, runs[i].start);
                    runs[j].start = -1;
                    k = j;
                }
                else
                {
                    run_end = uCount;
                    break;
                }
            }

            type_fence = previousValidChar(pcls, run_start, -1);

            if (type_fence == -1)
                current_isolated->sos = (baselevel > pLevel[run_start])?baselevel:pLevel[run_start];
            else
                current_isolated->sos = (pLevel[type_fence] > pLevel[run_start])?pLevel[type_fence]:pLevel[run_start];

            current_isolated->sos = EmbeddingDirection(current_isolated->sos);

            if (run_end == uCount)
                current_isolated->eos = current_isolated->sos;
            else
            {
                /* eos could be an BN */
                if ( pcls[run_end] == BN )
                {
                    real_end = previousValidChar(pcls, run_end, run_start-1);
                    if (real_end < run_start)
                        real_end = run_start;
                }
                else
                    real_end = run_end;

                type_fence = nextValidChar(pcls, run_end, uCount);
                if (type_fence == uCount)
                    current_isolated->eos = (baselevel > pLevel[real_end])?baselevel:pLevel[real_end];
                else
                    current_isolated->eos = (pLevel[type_fence] > pLevel[real_end])?pLevel[type_fence]:pLevel[real_end];

                current_isolated->eos = EmbeddingDirection(current_isolated->eos);
            }

            list_add_tail(set, &current_isolated->entry);
            TRACE(" } level %i {%s <--> %s}\n",current_isolated->e, debug_type[current_isolated->sos], debug_type[current_isolated->eos]);
        }
        i++;
    }

    free(runs);
}

/*************************************************************
 *    BIDI_DetermineLevels
 */
BOOL BIDI_DetermineLevels(
                const WCHAR *lpString,  /* [in] The string for which information is to be returned */
                unsigned int uCount,    /* [in] Number of WCHARs in string. */
                const SCRIPT_STATE *s,
                const SCRIPT_CONTROL *c,
                WORD *lpOutLevels, /* [out] final string levels */
                WORD *lpOutOverrides /* [out] final string overrides */
    )
{
    WORD *chartype;
    unsigned baselevel = 0;
    struct list IsolatingRuns;
    IsolatedRun *iso_run, *next;

    TRACE("%s, %d\n", debugstr_wn(lpString, uCount), uCount);

    if (!(chartype = malloc(uCount * sizeof(*chartype))))
    {
        WARN("Out of memory\n");
        return FALSE;
    }

    baselevel = s->uBidiLevel;

    classify(lpString, chartype, uCount, c);
    if (TRACE_ON(bidi)) dump_types("Start ", chartype, 0, uCount);

    memset(lpOutOverrides, 0, sizeof(WORD) * uCount);

    /* resolve explicit */
    resolveExplicit(baselevel, chartype, lpOutLevels, lpOutOverrides, uCount, s->fOverrideDirection);
    if (TRACE_ON(bidi)) dump_types("After Explicit", chartype, 0, uCount);

    /* X10/BD13: Computer Isolating runs */
    computeIsolatingRunsSet(baselevel, chartype, lpOutLevels, lpString, uCount, &IsolatingRuns);

    LIST_FOR_EACH_ENTRY_SAFE(iso_run, next, &IsolatingRuns, IsolatedRun, entry)
    {
        if (TRACE_ON(bidi)) iso_dump_types("Run", iso_run);

        /* resolve weak */
        resolveWeak(iso_run);
        if (TRACE_ON(bidi)) iso_dump_types("After Weak", iso_run);

        /* resolve neutrals */
        resolveNeutrals(iso_run);
        if (TRACE_ON(bidi)) iso_dump_types("After Neutrals", iso_run);

        list_remove(&iso_run->entry);
        free(iso_run);
    }

    if (TRACE_ON(bidi)) dump_types("Before Implicit", chartype, 0, uCount);
    /* resolveImplicit */
    resolveImplicit(chartype, lpOutLevels, 0, uCount-1);

    /* resolveResolvedLevels*/
    classify(lpString, chartype, uCount, c);
    resolveResolved(baselevel, chartype, lpOutLevels, 0, uCount-1);

    free(chartype);
    return TRUE;
}

/* reverse cch indices */
static void reverse(int *pidx, int cch)
{
    int temp;
    int ich = 0;
    for (; ich < --cch; ich++)
    {
        temp = pidx[ich];
        pidx[ich] = pidx[cch];
        pidx[cch] = temp;
    }
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
int BIDI_ReorderV2lLevel(int level, int *pIndices, const BYTE* plevel, int cch, BOOL fReverse)
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
            ich += BIDI_ReorderV2lLevel(level + 1, pIndices + ich, plevel + ich,
                cch - ich, fReverse) - 1;
        }
    }
    if (fReverse)
    {
        reverse(pIndices, ich);
    }
    return ich;
}

/* Applies the reorder in reverse. Taking an already reordered string and returning the original */
int BIDI_ReorderL2vLevel(int level, int *pIndices, const BYTE* plevel, int cch, BOOL fReverse)
{
    int ich = 0;
    int newlevel = -1;

    /* true as soon as first odd level encountered */
    fReverse = fReverse || odd(level);

    for (; ich < cch; ich++)
    {
        if (plevel[ich] < level)
            break;
        else if (plevel[ich] > level)
            newlevel = ich;
    }
    if (fReverse)
    {
        reverse(pIndices, ich);
    }

    if (newlevel >= 0)
    {
        ich = 0;
        for (; ich < cch; ich++)
            if (plevel[ich] < level)
                break;
            else if (plevel[ich] > level)
                ich += BIDI_ReorderL2vLevel(level + 1, pIndices + ich, plevel + ich,
                cch - ich, fReverse) - 1;
    }

    return ich;
}

BOOL BIDI_GetStrengths(const WCHAR *string, unsigned int count, const SCRIPT_CONTROL *c, WORD *strength)
{
    unsigned int i;

    classify(string, strength, count, c);
    for (i = 0; i < count; i++)
    {
        switch (strength[i])
        {
            case L:
            case LRE:
            case LRO:
            case R:
            case AL:
            case RLE:
            case RLO:
                strength[i] = BIDI_STRONG;
                break;
            case PDF:
            case EN:
            case ES:
            case ET:
            case AN:
            case CS:
            case BN:
                strength[i] = BIDI_WEAK;
                break;
            case B:
            case S:
            case WS:
            case ON:
            default: /* Neutrals and NSM */
                strength[i] = BIDI_NEUTRAL;
        }
    }
    return TRUE;
}

/*
 * Unicode Bidirectional Algorithm implementation
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

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

#include "dwrite_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(bidi);

extern const unsigned short bidi_bracket_table[];
extern const unsigned short bidi_direction_table[];

#define ASSERT(x) do { if (!(x)) FIXME("assert failed: %s\n", #x); } while(0)
#define MAX_DEPTH 125

#define odd(x) ((x) & 1)

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
    "S",       /* Segment Separator (TAB)         used only in L1 */
    "WS",      /* White space                     used only in L1 */
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

static inline void bidi_dump_types(const char* header, const struct bidi_char *chars, UINT32 start, UINT32 end)
{
    int i, len = 0;
    TRACE("%s:", header);
    for (i = start; i < end && len < 200; i++) {
        TRACE(" %s", debug_type[chars[i].bidi_class]);
        len += strlen(debug_type[chars[i].bidi_class]) + 1;
    }
    if (i != end)
        TRACE("...");
    TRACE("\n");
}

/* RESOLVE EXPLICIT */

static inline UINT8 get_greater_even_level(UINT8 level)
{
    return odd(level) ? level + 1 : level + 2;
}

static inline UINT8 get_greater_odd_level(UINT8 level)
{
    return odd(level) ? level + 2 : level + 1;
}

static inline UINT8 get_embedding_direction(UINT8 level)
{
    return odd(level) ? R : L;
}

/*------------------------------------------------------------------------
    Function: bidi_resolve_explicit

    Recursively resolves explicit embedding levels and overrides.
    Implements rules X1-X9, of the Unicode Bidirectional Algorithm.

    Note: The function uses two simple counters to keep track of
          matching explicit codes and PDF. Use the default argument for
          the outermost call. The nesting counter counts the recursion
          depth and not the embedding level.
------------------------------------------------------------------------*/
typedef struct tagStackItem
{
    UINT8 level;
    UINT8 override;
    BOOL isolate;
} StackItem;

#define push_stack(l,o,i)  \
  do { stack_top--; \
  stack[stack_top].level = l; \
  stack[stack_top].override = o; \
  stack[stack_top].isolate = i;} while(0)

#define pop_stack() do { stack_top++; } while(0)

#define valid_level(x) (x <= MAX_DEPTH && overflow_isolate_count == 0 && overflow_embedding_count == 0)

static void bidi_resolve_explicit(struct bidi_char *chars, unsigned int count, UINT8 baselevel)
{
    /* X1 */
    int overflow_isolate_count = 0;
    int overflow_embedding_count = 0;
    int valid_isolate_count = 0;
    unsigned int i;

    StackItem stack[MAX_DEPTH+2];
    int stack_top = MAX_DEPTH+1;

    stack[stack_top].level = baselevel;
    stack[stack_top].override = NI;
    stack[stack_top].isolate = FALSE;

    for (i = 0; i < count; ++i)
    {
        struct bidi_char *c = &chars[i];
        UINT8 least_odd, least_even;

        switch (c->bidi_class)
        {

        /* X2 */
        case RLE:
            least_odd = get_greater_odd_level(stack[stack_top].level);
            c->resolved = valid_level(least_odd) ? least_odd : stack[stack_top].level;
            if (valid_level(least_odd))
                push_stack(least_odd, NI, FALSE);
            else if (overflow_isolate_count == 0)
                overflow_embedding_count++;
            break;

        /* X3 */
        case LRE:
            least_even = get_greater_even_level(stack[stack_top].level);
            c->resolved = valid_level(least_even) ? least_even : stack[stack_top].level;
            if (valid_level(least_even))
                push_stack(least_even, NI, FALSE);
            else if (overflow_isolate_count == 0)
                overflow_embedding_count++;
            break;

        /* X4 */
        case RLO:
            least_odd = get_greater_odd_level(stack[stack_top].level);
            c->resolved = stack[stack_top].level;
            if (valid_level(least_odd))
                push_stack(least_odd, R, FALSE);
            else if (overflow_isolate_count == 0)
                overflow_embedding_count++;
            break;

        /* X5 */
        case LRO:
            least_even = get_greater_even_level(stack[stack_top].level);
            c->resolved = stack[stack_top].level;
            if (valid_level(least_even))
                push_stack(least_even, L, FALSE);
            else if (overflow_isolate_count == 0)
                overflow_embedding_count++;
            break;

        /* X5a */
        case RLI:
            least_odd = get_greater_odd_level(stack[stack_top].level);
            c->resolved = stack[stack_top].level;
            if (valid_level(least_odd))
            {
                valid_isolate_count++;
                push_stack(least_odd, NI, TRUE);
            }
            else
                overflow_isolate_count++;
            break;

        /* X5b */
        case LRI:
            least_even = get_greater_even_level(stack[stack_top].level);
            c->resolved = stack[stack_top].level;
            if (valid_level(least_even))
            {
                valid_isolate_count++;
                push_stack(least_even, NI, TRUE);
            }
            else
                overflow_isolate_count++;
            break;

        /* X5c */
        case FSI:
        {
            UINT8 new_level = 0;
            int skipping = 0;
            int j;

            c->resolved = stack[stack_top].level;
            for (j = i+1; j < count; j++)
            {
                const struct bidi_char *p = &chars[j];

                if (p->bidi_class == LRI || p->bidi_class == RLI || p->bidi_class == FSI)
                {
                    skipping++;
                    continue;
                }
                else if (p->bidi_class == PDI)
                {
                    if (skipping)
                        skipping --;
                    else
                        break;
                    continue;
                }

                if (skipping) continue;

                if (p->bidi_class == L)
                {
                    new_level = 0;
                    break;
                }
                else if (p->bidi_class == R || p->bidi_class == AL)
                {
                    new_level = 1;
                    break;
                }
            }
            if (odd(new_level))
            {
                least_odd = get_greater_odd_level(stack[stack_top].level);
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
                least_even = get_greater_even_level(stack[stack_top].level);
                if (valid_level(least_even))
                {
                    valid_isolate_count++;
                    push_stack(least_even, NI, TRUE);
                }
                else
                    overflow_isolate_count++;
            }
            break;
        }

        /* X6 */
        case ON:
        case L:
        case R:
        case AN:
        case EN:
        case AL:
        case NSM:
        case CS:
        case ES:
        case ET:
        case S:
        case WS:
            c->resolved = stack[stack_top].level;
            if (stack[stack_top].override != NI)
                c->resolved = stack[stack_top].override;
            break;

        /* X6a */
        case PDI:
            if (overflow_isolate_count) overflow_isolate_count--;
            else if (!valid_isolate_count) {/* do nothing */}
            else
            {
                overflow_embedding_count = 0;
                while (!stack[stack_top].isolate) pop_stack();
                pop_stack();
                valid_isolate_count--;
            }
            c->resolved = stack[stack_top].level;
            break;

        /* X7 */
        case PDF:
            c->resolved = stack[stack_top].level;
            if (overflow_isolate_count) {/* do nothing */}
            else if (overflow_embedding_count) overflow_embedding_count--;
            else if (!stack[stack_top].isolate && stack_top < (MAX_DEPTH+1))
                pop_stack();
            break;

        /* X8 */
        default:
            c->resolved = baselevel;
            break;
        }

        c->explicit = c->resolved;
    }

    /* X9: Based on 5.2 Retaining Explicit Formatting Characters */
    for (i = 0; i < count; ++i)
    {
        switch (chars[i].bidi_class)
        {
            case RLE:
            case LRE:
            case RLO:
            case LRO:
            case PDF:
                chars[i].bidi_class = BN;
                break;
            default:
                ;
        }
    }
}

static inline int get_prev_valid_char_index(const struct bidi_char *chars, int index, int back_fence)
{
    if (index == -1 || index == back_fence) return index;
    index--;
    while (index > back_fence && chars[index].bidi_class == BN) index--;
    return index;
}

static inline int get_next_valid_char_index(const struct bidi_char *chars, int index, int front_fence)
{
    if (index == front_fence) return index;
    index++;
    while (index < front_fence && chars[index].bidi_class == BN) index++;
    return index;
}

typedef struct tagRun
{
    int start;
    int end;
    UINT8 e;
} Run;

typedef struct tagRunChar
{
    WCHAR  ch;
    UINT8 *class;
} RunChar;

typedef struct tagIsolatedRun
{
    struct list entry;
    int length;
    UINT8 sos;
    UINT8 eos;
    UINT8 e;

    RunChar item[1];
} IsolatedRun;

static inline int get_next_valid_char_from_run(IsolatedRun *run, int index)
{
    if (index >= (run->length-1)) return -1;
    index++;
    while (index < run->length && *run->item[index].class == BN) index++;
    if (index == run->length) return -1;
    return index;
}

static inline int get_prev_valid_char_from_run(IsolatedRun *run, int index)
{
    if (index <= 0) return -1;
    index--;
    while (index > -1 && *run->item[index].class == BN) index--;
    return index;
}

static inline void iso_dump_types(const char* header, IsolatedRun *run)
{
    int i, len = 0;
    TRACE("%s:",header);
    TRACE("[ ");
    for (i = 0; i < run->length && len < 200; i++) {
        TRACE(" %s", debug_type[*run->item[i].class]);
        len += strlen(debug_type[*run->item[i].class])+1;
    }
    if (i != run->length)
        TRACE("...");
    TRACE(" ]\n");
}

/*------------------------------------------------------------------------
    Function: bidi_resolve_weak

    Resolves the directionality of numeric and other weak character types

    Implements rules X10 and W1-W6 of the Unicode Bidirectional Algorithm.

    Input: Array of embedding levels
           Character count

    In/Out: Array of directional classes

    Note: On input only these directional classes are expected
          AL, HL, R, L,  ON, BN, NSM, AN, EN, ES, ET, CS,
------------------------------------------------------------------------*/
static BOOL bidi_is_isolate(UINT8 class)
{
    return class == LRI || class == RLI || class == FSI || class == PDI;
}

static void bidi_resolve_weak(IsolatedRun *iso_run)
{
    int i;

    /* W1 */
    for (i=0; i < iso_run->length; i++) {
        if (*iso_run->item[i].class == NSM) {
            int j = get_prev_valid_char_from_run(iso_run, i);
            if (j == -1)
                *iso_run->item[i].class = iso_run->sos;
            else if (bidi_is_isolate(*iso_run->item[j].class))
                *iso_run->item[i].class = ON;
            else
                *iso_run->item[i].class = *iso_run->item[j].class;
        }
    }

    /* W2 */
    for (i = 0; i < iso_run->length; i++) {
        if (*iso_run->item[i].class == EN) {
            int j = get_prev_valid_char_from_run(iso_run, i);
            while (j > -1) {
                if (*iso_run->item[j].class == R || *iso_run->item[j].class == L || *iso_run->item[j].class == AL) {
                    if (*iso_run->item[j].class == AL)
                        *iso_run->item[i].class = AN;
                    break;
                }
                j = get_prev_valid_char_from_run(iso_run, j);
            }
        }
    }

    /* W3 */
    for (i = 0; i < iso_run->length; i++) {
        if (*iso_run->item[i].class == AL)
            *iso_run->item[i].class = R;
    }

    /* W4 */
    for (i = 0; i < iso_run->length; i++) {
        if (*iso_run->item[i].class == ES) {
            int b = get_prev_valid_char_from_run(iso_run, i);
            int f = get_next_valid_char_from_run(iso_run, i);

            if (b > -1 && f > -1 && *iso_run->item[b].class == EN && *iso_run->item[f].class == EN)
                *iso_run->item[i].class = EN;
        }
        else if (*iso_run->item[i].class == CS) {
            int b = get_prev_valid_char_from_run(iso_run, i);
            int f = get_next_valid_char_from_run(iso_run, i);

            if (b > -1 && f > -1 && *iso_run->item[b].class == EN && *iso_run->item[f].class == EN)
                *iso_run->item[i].class = EN;
            else if (b > -1 && f > -1 && *iso_run->item[b].class == AN && *iso_run->item[f].class == AN)
                *iso_run->item[i].class = AN;
        }
    }

    /* W5 */
    for (i = 0; i < iso_run->length; i++) {
        if (*iso_run->item[i].class == ET) {
            int j;
            for (j = i-1 ; j > -1; j--) {
                if (*iso_run->item[j].class == BN) continue;
                if (*iso_run->item[j].class == ET) continue;
                else if (*iso_run->item[j].class == EN) *iso_run->item[i].class = EN;
                else break;
            }
            if (*iso_run->item[i].class == ET) {
                for (j = i+1; j < iso_run->length; j++) {
                    if (*iso_run->item[j].class == BN) continue;
                    if (*iso_run->item[j].class == ET) continue;
                    else if (*iso_run->item[j].class == EN) *iso_run->item[i].class = EN;
                    else break;
                }
            }
        }
    }

    /* W6 */
    for (i = 0; i < iso_run->length; i++) {
        if (*iso_run->item[i].class == ET || *iso_run->item[i].class == ES || *iso_run->item[i].class == CS || *iso_run->item[i].class == ON)
        {
            int b = i-1;
            int f = i+1;
            if (b > -1 && *iso_run->item[b].class == BN)
                *iso_run->item[b].class = ON;
            if (f < iso_run->length && *iso_run->item[f].class == BN)
                *iso_run->item[f].class = ON;

            *iso_run->item[i].class = ON;
        }
    }

    /* W7 */
    for (i = 0; i < iso_run->length; i++) {
        if (*iso_run->item[i].class == EN) {
            int j;
            for (j = get_prev_valid_char_from_run(iso_run, i); j > -1; j = get_prev_valid_char_from_run(iso_run, j))
                if (*iso_run->item[j].class == R || *iso_run->item[j].class == L) {
                    if (*iso_run->item[j].class == L)
                        *iso_run->item[i].class = L;
                    break;
                }
            if (iso_run->sos == L && j == -1)
                *iso_run->item[i].class = L;
        }
    }
}

typedef struct tagBracketPair
{
    int start;
    int end;
} BracketPair;

static int __cdecl bracketpair_compr(const void *a, const void* b)
{
    return ((BracketPair*)a)->start - ((BracketPair*)b)->start;
}

static BracketPair *bidi_compute_bracket_pairs(IsolatedRun *iso_run)
{
    WCHAR *open_stack;
    int *stack_index;
    int stack_top = iso_run->length;
    BracketPair *out;
    int pair_count = 0;
    int i;

    open_stack = malloc(sizeof(WCHAR) * iso_run->length);
    stack_index = malloc(sizeof(int) * iso_run->length);
    out = malloc(sizeof(BracketPair) * iso_run->length);

    if (!open_stack || !stack_index || !out) {
        free(open_stack);
        free(stack_index);
        free(out);
        return NULL;
    }

    out[0].start = -1;

    for (i = 0; i < iso_run->length; i++) {
        unsigned short ubv = get_table_entry_16(bidi_bracket_table, iso_run->item[i].ch);
        if (ubv)
        {
            if ((ubv >> 8) == 0) {
                stack_top--;
                open_stack[stack_top] = iso_run->item[i].ch + (signed char)(ubv & 0xff);
                /* deal with canonical equivalent U+2329/232A and U+3008/3009 */
                if (open_stack[stack_top] == 0x232A)
                    open_stack[stack_top] = 0x3009;
                stack_index[stack_top] = i;
            }
            else if ((ubv >> 8) == 1) {
                int j;

                if (stack_top == iso_run->length) continue;
                for (j = stack_top; j < iso_run->length; j++) {
                    WCHAR c = iso_run->item[i].ch;
                    if (c == 0x232A) c = 0x3009;
                    if (c == open_stack[j]) {
                        out[pair_count].start = stack_index[j];
                        out[pair_count].end = i;
                        pair_count++;
                        out[pair_count].start = -1;
                        stack_top = j+1;
                        break;
                    }
                }
            }
        }
    }
    if (pair_count == 0)
    {
        free(out);
        out = NULL;
    }
    else if (pair_count > 1)
        qsort(out, pair_count, sizeof(BracketPair), bracketpair_compr);

    free(open_stack);
    free(stack_index);
    return out;
}

static inline UINT8 get_rule_N0_class(UINT8 class)
{
    return (class == AN || class == EN) ? R : class;
}

/*------------------------------------------------------------------------
    Function: bidi_resolve_neutrals

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
static void bidi_resolve_neutrals(IsolatedRun *run)
{
    BracketPair *pairs;
    int i;

    /* Translate isolates into NI */
    for (i = 0; i < run->length; i++) {
        switch (*run->item[i].class) {
            case B:
            case S:
            case WS:
            case FSI:
            case LRI:
            case RLI:
            case PDI: *run->item[i].class = NI;
        }

        /* "Only NI, L, R, AN, EN and BN are allowed" */
        ASSERT(*run->item[i].class <= EN || *run->item[i].class == BN);
    }

    /* N0: Skipping bracketed pairs for now */
    pairs = bidi_compute_bracket_pairs(run);
    if (pairs) {
        BracketPair *p = pairs;
        int i = 0;
        while (p->start >= 0) {
            UINT8 e = get_embedding_direction(run->e);
            UINT8 o = get_embedding_direction(run->e + 1);
            BOOL flag_o = FALSE;
            int j;

            TRACE("Bracket Pair [%i - %i]\n", p->start, p->end);

            /* N0.b */
            for (j = p->start+1; j < p->end; j++) {
                if (get_rule_N0_class(*run->item[j].class) == e) {
                    *run->item[p->start].class = e;
                    *run->item[p->end].class = e;
                    break;
                }
                else if (get_rule_N0_class(*run->item[j].class) == o)
                    flag_o = TRUE;
            }
            /* N0.c */
            if (j == p->end && flag_o) {
                for (j = p->start; j >= 0; j--) {
                    if (get_rule_N0_class(*run->item[j].class) == o) {
                        *run->item[p->start].class = o;
                        *run->item[p->end].class = o;
                        break;
                    }
                    else if (get_rule_N0_class(*run->item[j].class) == e) {
                        *run->item[p->start].class = e;
                        *run->item[p->end].class = e;
                        break;
                    }
                }
                if (j < 0) {
                    *run->item[p->start].class = run->sos;
                    *run->item[p->end].class = run->sos;
                }
            }

            i++;
            p = &pairs[i];
        }
        free(pairs);
    }

    /* N1 */
    for (i = 0; i < run->length; i++) {
        UINT8 l, r;

        if (*run->item[i].class == NI) {
            int b = get_prev_valid_char_from_run(run, i);
            int j;

            if (b == -1) {
                l = run->sos;
                b = 0;
            }
            else {
                if (*run->item[b].class == R || *run->item[b].class == AN || *run->item[b].class == EN)
                    l = R;
                else if (*run->item[b].class == L)
                    l = L;
                else /* No string type */
                    continue;
            }
            j = get_next_valid_char_from_run(run, i);
            while (j > -1 && *run->item[j].class == NI) j = get_next_valid_char_from_run(run, j);
            if (j == -1) {
                r = run->eos;
                j = run->length;
            }
            else if (*run->item[j].class == R || *run->item[j].class == AN || *run->item[j].class == EN)
                r = R;
            else if (*run->item[j].class == L)
                r = L;
            else /* No string type */
                continue;

            if (r == l) {
                for (b = i; b < j && b < run->length; b++)
                    *run->item[b].class = r;
            }
        }
    }

    /* N2 */
    for (i = 0; i < run->length; i++) {
        if (*run->item[i].class == NI) {
            int b = i-1;
            int f = i+1;

            *run->item[i].class = get_embedding_direction(run->e);
            if (b > -1 && *run->item[b].class == BN)
                *run->item[b].class = get_embedding_direction(run->e);
            if (f < run->length && *run->item[f].class == BN)
                *run->item[f].class = get_embedding_direction(run->e);
        }
    }
}

/*------------------------------------------------------------------------
    Function: bidi_resolve_implicit

    Recursively resolves implicit embedding levels.
    Implements rules I1 and I2 of the Unicode Bidirectional Algorithm.

    Note: levels may exceed 15 on output.
          Accepted subset of direction classes
          R, L, AN, EN
------------------------------------------------------------------------*/
static void bidi_resolve_implicit(struct bidi_char *chars, unsigned int count)
{
    unsigned int i;

    /* I1/2 */
    for (i = 0; i < count; ++i)
    {
        struct bidi_char *c = &chars[i];

        if (c->bidi_class == BN)
            continue;

        ASSERT(c->bidi_class != ON); /* "No Neutrals allowed to survive here." */
        ASSERT(c->bidi_class <= EN); /* "Out of range." */

        if (odd(c->resolved) && (c->bidi_class == L || c->bidi_class == EN || c->bidi_class == AN))
            c->resolved++;
        else if (!odd(c->resolved) && c->bidi_class == R)
            c->resolved++;
        else if (!odd(c->resolved) && (c->bidi_class == EN || c->bidi_class == AN))
            c->resolved += 2;
    }
}

static inline BOOL is_rule_L1_reset_class(UINT8 class)
{
    switch (class) {
    case WS:
    case FSI:
    case LRI:
    case RLI:
    case PDI:
    case LRE:
    case RLE:
    case LRO:
    case RLO:
    case PDF:
    case BN:
        return TRUE;
    default:
        return FALSE;
    }
}

static void bidi_resolve_resolved(struct bidi_char *chars, unsigned int count, UINT8 baselevel)
{
    int i, sos = 0, eos = count - 1;

    /* L1 */
    for (i = sos; i <= eos; i++)
    {
        switch (chars[i].nominal_bidi_class)
        {
            case B:
            case S:
                {
                    int j = i - 1;
                    while (i > sos && j >= sos && is_rule_L1_reset_class(chars[j].nominal_bidi_class))
                        chars[j--].resolved = baselevel;
                    chars[i].resolved = baselevel;
                }
                break;
            case LRE: case RLE: case LRO: case RLO: case PDF: case BN:
                chars[i].resolved = i ? chars[i - 1].resolved : baselevel;
                break;
            default:
                ;
        }

        if (i == eos && is_rule_L1_reset_class(chars[i].nominal_bidi_class))
        {
            int j = i;
            while (j >= sos && is_rule_L1_reset_class(chars[j].nominal_bidi_class))
                chars[j--].resolved = baselevel;
        }
    }
}

static HRESULT bidi_compute_isolating_runs_set(struct bidi_char *chars, unsigned int count, UINT8 baselevel, struct list *set)
{
    int run_start, run_end, i;
    int run_count = 0;
    HRESULT hr = S_OK;
    Run *runs;

    if (!(runs = calloc(count, sizeof(*runs))))
        return E_OUTOFMEMORY;

    list_init(set);

    /* Build Runs */
    run_start = 0;
    while (run_start < count)
    {
        run_end = get_next_valid_char_index(chars, run_start, count);
        while (run_end < count && chars[run_end].resolved == chars[run_start].resolved)
            run_end = get_next_valid_char_index(chars, run_end, count);
        run_end--;
        runs[run_count].start = run_start;
        runs[run_count].end = run_end;
        runs[run_count].e = chars[run_start].resolved;
        run_start = get_next_valid_char_index(chars, run_end, count);
        run_count++;
    }

    /* Build Isolating Runs */
    i = 0;
    while (i < run_count)
    {
        int k = i;
        if (runs[k].start >= 0)
        {
            IsolatedRun *current_isolated;
            int type_fence, real_end;
            int j;

            if (!(current_isolated = malloc(sizeof(IsolatedRun) + sizeof(RunChar)*count)))
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            run_start = runs[k].start;
            current_isolated->e = runs[k].e;
            current_isolated->length = (runs[k].end - runs[k].start)+1;

            for (j = 0; j < current_isolated->length; ++j)
            {
                current_isolated->item[j].class = &chars[runs[k].start+j].bidi_class;
                current_isolated->item[j].ch = chars[runs[k].start+j].ch;
            }

            run_end = runs[k].end;

            TRACE("{ [%i -- %i]",run_start, run_end);

            if (chars[run_end].bidi_class == BN)
                run_end = get_prev_valid_char_index(chars, run_end, runs[k].start);

            while (run_end < count && (chars[run_end].bidi_class == RLI || chars[run_end].bidi_class == LRI || chars[run_end].bidi_class == FSI))
            {
                j = k+1;
search:
                while (j < run_count && chars[runs[j].start].bidi_class != PDI) j++;
                if (j < run_count && runs[i].e != runs[j].e) {
                    j++;
                    goto search;
                }

                if (j != run_count)
                {
                    int l = current_isolated->length;
                    int m;

                    current_isolated->length += (runs[j].end - runs[j].start)+1;
                    for (m = 0; l < current_isolated->length; l++, m++) {
                        current_isolated->item[l].class = &chars[runs[j].start + m].bidi_class;
                        current_isolated->item[l].ch = chars[runs[j].start + m].ch;
                    }

                    TRACE("[%i -- %i]", runs[j].start, runs[j].end);

                    run_end = runs[j].end;
                    if (chars[run_end].bidi_class == BN)
                        run_end = get_prev_valid_char_index(chars, run_end, runs[i].start);
                    runs[j].start = -1;
                    k = j;
                }
                else {
                    run_end = count;
                    break;
                }
            }

            type_fence = get_prev_valid_char_index(chars, run_start, -1);

            if (type_fence == -1)
                current_isolated->sos = max(baselevel, chars[run_start].resolved);
            else
                current_isolated->sos = max(chars[type_fence].resolved, chars[run_start].resolved);

            current_isolated->sos = get_embedding_direction(current_isolated->sos);

            if (run_end == count)
                current_isolated->eos = current_isolated->sos;
            else
            {
                /* eos could be an BN */
                if (chars[run_end].resolved == BN)
                {
                    real_end = get_prev_valid_char_index(chars, run_end, run_start - 1);
                    if (real_end < run_start)
                        real_end = run_start;
                }
                else
                    real_end = run_end;

                type_fence = get_next_valid_char_index(chars, run_end, count);
                if (type_fence == count)
                    current_isolated->eos = max(baselevel, chars[real_end].resolved);
                else
                    current_isolated->eos = max(chars[type_fence].resolved, chars[real_end].resolved);

                current_isolated->eos = get_embedding_direction(current_isolated->eos);
            }

            list_add_tail(set, &current_isolated->entry);
            TRACE(" } level %i {%s <--> %s}\n", current_isolated->e, debug_type[current_isolated->sos], debug_type[current_isolated->eos]);
        }
        i++;
    }

    free(runs);
    return hr;
}

HRESULT bidi_computelevels(struct bidi_char *chars, unsigned int count, UINT8 baselevel)
{
    IsolatedRun *iso_run, *next;
    struct list IsolatingRuns;
    HRESULT hr;

    if (TRACE_ON(bidi)) bidi_dump_types("start ", chars, 0, count);

    bidi_resolve_explicit(chars, count, baselevel);

    if (TRACE_ON(bidi)) bidi_dump_types("after explicit", chars, 0, count);

    /* X10/BD13: Compute Isolating runs */
    if (FAILED(hr = bidi_compute_isolating_runs_set(chars, count, baselevel, &IsolatingRuns)))
    {
        WARN("Failed to compute isolating runs set, hr %#lx.\n", hr);
        return hr;
    }

    LIST_FOR_EACH_ENTRY_SAFE(iso_run, next, &IsolatingRuns, IsolatedRun, entry)
    {
        if (TRACE_ON(bidi)) iso_dump_types("run", iso_run);

        bidi_resolve_weak(iso_run);
        if (TRACE_ON(bidi)) iso_dump_types("after weak", iso_run);

        bidi_resolve_neutrals(iso_run);
        if (TRACE_ON(bidi)) iso_dump_types("after neutrals", iso_run);

        list_remove(&iso_run->entry);
        free(iso_run);
    }

    if (TRACE_ON(bidi)) bidi_dump_types("before implicit", chars, 0, count);
    bidi_resolve_implicit(chars, count);

    bidi_resolve_resolved(chars, count, baselevel);

    return S_OK;
}

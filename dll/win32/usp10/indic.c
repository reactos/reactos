/*
 * Implementation of Indic Syllables for the Uniscribe Script Processor
 *
 * Copyright 2011 CodeWeavers, Aric Stewart
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
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "winnls.h"
#include "usp10.h"
#include "winternl.h"

#include "wine/debug.h"
#include "wine/heap.h"
#include "usp10_internal.h"

WINE_DEFAULT_DEBUG_CHANNEL(uniscribe);

static void debug_output_string(const WCHAR *str, unsigned int char_count, lexical_function f)
{
    int i;
    if (TRACE_ON(uniscribe))
    {
        for (i = 0; i < char_count; ++i)
        {
            switch (f(str[i]))
            {
                case lex_Consonant: TRACE("C"); break;
                case lex_Ra: TRACE("Ra"); break;
                case lex_Vowel: TRACE("V"); break;
                case lex_Nukta: TRACE("N"); break;
                case lex_Halant: TRACE("H"); break;
                case lex_ZWNJ: TRACE("Zwnj"); break;
                case lex_ZWJ: TRACE("Zwj"); break;
                case lex_Matra_post: TRACE("Mp");break;
                case lex_Matra_above: TRACE("Ma");break;
                case lex_Matra_below: TRACE("Mb");break;
                case lex_Matra_pre: TRACE("Mm");break;
                case lex_Modifier: TRACE("Sm"); break;
                case lex_Vedic: TRACE("Vd"); break;
                case lex_Anudatta: TRACE("A"); break;
                case lex_Composed_Vowel: TRACE("t"); break;
                default:
                    TRACE("X"); break;
            }
        }
        TRACE("\n");
    }
}

static inline BOOL is_matra( int type )
{
    return (type == lex_Matra_above || type == lex_Matra_below ||
            type == lex_Matra_pre || type == lex_Matra_post ||
            type == lex_Composed_Vowel);
}

static inline BOOL is_joiner( int type )
{
    return (type == lex_ZWJ || type == lex_ZWNJ);
}

static int consonant_header(const WCHAR *input, unsigned int cChar,
        unsigned int start, unsigned int next, lexical_function lex)
{
    if (!is_consonant( lex(input[next]) )) return -1;
    next++;
    if ((next < cChar) && lex(input[next]) == lex_Nukta)
            next++;
    if ((next < cChar) && lex(input[next])==lex_Halant)
    {
        next++;
        if((next < cChar) && is_joiner( lex(input[next]) ))
            next++;
        if ((next < cChar) && is_consonant( lex(input[next]) ))
            return next;
    }
    else if ((next < cChar) && is_joiner( lex(input[next]) ) && lex(input[next+1])==lex_Halant)
    {
        next+=2;
        if ((next < cChar) && is_consonant( lex(input[next]) ))
            return next;
    }
    return -1;
}

static int parse_consonant_syllable(const WCHAR *input, unsigned int cChar,
        unsigned int start, unsigned int *main, unsigned int next, lexical_function lex)
{
    int check;
    int headers = 0;
    do
    {
        check = consonant_header(input,cChar,start,next,lex);
        if (check != -1)
        {
            next = check;
            headers++;
        }
    } while (check != -1);
    if (headers || is_consonant( lex(input[next]) ))
    {
        *main = next;
        next++;
    }
    else
        return -1;
    if ((next < cChar) && lex(input[next]) == lex_Nukta)
            next++;
    if ((next < cChar) && lex(input[next]) == lex_Anudatta)
            next++;

    if ((next < cChar) && lex(input[next]) == lex_Halant)
    {
        next++;
        if((next < cChar) && is_joiner( lex(input[next]) ))
            next++;
    }
    else if (next < cChar)
    {
        while((next < cChar) && is_matra( lex(input[next]) ))
            next++;
        if ((next < cChar) && lex(input[next]) == lex_Nukta)
            next++;
        if ((next < cChar) && lex(input[next]) == lex_Halant)
            next++;
    }
    if ((next < cChar) && lex(input[next]) == lex_Modifier)
            next++;
    if ((next < cChar) && lex(input[next]) == lex_Vedic)
            next++;
    return next;
}

static int parse_vowel_syllable(const WCHAR *input, unsigned int cChar,
        unsigned int start, unsigned int next, lexical_function lex)
{
    if ((next < cChar) && lex(input[next]) == lex_Nukta)
        next++;
    if ((next < cChar) && is_joiner( lex(input[next]) ) && lex(input[next+1])==lex_Halant && is_consonant( lex(input[next+2]) ))
        next+=3;
    else if ((next < cChar) && lex(input[next])==lex_Halant && is_consonant( lex(input[next+1]) ))
        next+=2;
    else if ((next < cChar) && lex(input[next])==lex_ZWJ && is_consonant( lex(input[next+1]) ))
        next+=2;

    if ((next < cChar) && is_matra( lex(input[next]) ))
    {
        while((next < cChar) && is_matra( lex(input[next]) ))
            next++;
        if ((next < cChar) && lex(input[next]) == lex_Nukta)
            next++;
        if ((next < cChar) && lex(input[next]) == lex_Halant)
            next++;
    }

    if ((next < cChar) && lex(input[next]) == lex_Modifier)
        next++;
    if ((next < cChar) && lex(input[next]) == lex_Vedic)
        next++;
    return next;
}

static int Indic_process_next_syllable(const WCHAR *input, unsigned int cChar,
        unsigned int start, unsigned int *main, unsigned int next, lexical_function lex)
{
    if (lex(input[next])==lex_Vowel)
    {
        *main = next;
        return parse_vowel_syllable(input, cChar, start, next+1, lex);
    }
    else if ((cChar > next+3) && lex(input[next]) == lex_Ra && lex(input[next+1]) == lex_Halant && lex(input[next+2]) == lex_Vowel)
    {
        *main = next+2;
        return parse_vowel_syllable(input, cChar, start, next+3, lex);
    }

    else if (start == next && lex(input[next])==lex_NBSP)
    {
        *main = next;
        return parse_vowel_syllable(input, cChar, start, next+1, lex);
    }
    else if (start == next && (cChar > next+3) && lex(input[next]) == lex_Ra && lex(input[next+1]) == lex_Halant && lex(input[next+2]) == lex_NBSP)
    {
        *main = next+2;
        return parse_vowel_syllable(input, cChar, start, next+3, lex);
    }

    return parse_consonant_syllable(input, cChar, start, main, next, lex);
}

static BOOL Consonant_is_post_base_form(HDC hdc, SCRIPT_ANALYSIS *psa, ScriptCache *psc,
        const WCHAR *pwChar, const IndicSyllable *s, lexical_function lexical, BOOL modern)
{
    if (is_consonant(lexical(pwChar[s->base])) && s->base > s->start && lexical(pwChar[s->base-1]) == lex_Halant)
    {
        if (modern)
            return (SHAPE_does_GSUB_feature_apply_to_chars(hdc, psa, psc, &pwChar[s->base-1], 1, 2, "pstf") > 0);
        else
        {
            WCHAR cc[2];
            cc[0] = pwChar[s->base];
            cc[1] = pwChar[s->base-1];
            return (SHAPE_does_GSUB_feature_apply_to_chars(hdc, psa, psc, cc, 1, 2, "pstf") > 0);
        }
    }
    return FALSE;
}

static BOOL Consonant_is_below_base_form(HDC hdc, SCRIPT_ANALYSIS *psa, ScriptCache *psc,
        const WCHAR *pwChar, const IndicSyllable *s, lexical_function lexical, BOOL modern)
{
    if (is_consonant(lexical(pwChar[s->base])) && s->base > s->start && lexical(pwChar[s->base-1]) == lex_Halant)
    {
        if (modern)
            return (SHAPE_does_GSUB_feature_apply_to_chars(hdc, psa, psc, &pwChar[s->base-1], 1, 2, "blwf") > 0);
        else
        {
            WCHAR cc[2];
            cc[0] = pwChar[s->base];
            cc[1] = pwChar[s->base-1];
            return (SHAPE_does_GSUB_feature_apply_to_chars(hdc, psa, psc, cc, 1, 2, "blwf") > 0);
        }
    }
    return FALSE;
}

static BOOL Consonant_is_pre_base_form(HDC hdc, SCRIPT_ANALYSIS *psa, ScriptCache *psc,
        const WCHAR *pwChar, const IndicSyllable *s, lexical_function lexical, BOOL modern)
{
    if (is_consonant(lexical(pwChar[s->base])) && s->base > s->start && lexical(pwChar[s->base-1]) == lex_Halant)
    {
        if (modern)
            return (SHAPE_does_GSUB_feature_apply_to_chars(hdc, psa, psc, &pwChar[s->base-1], 1, 2, "pref") > 0);
        else
        {
            WCHAR cc[2];
            cc[0] = pwChar[s->base];
            cc[1] = pwChar[s->base-1];
            return (SHAPE_does_GSUB_feature_apply_to_chars(hdc, psa, psc, cc, 1, 2, "pref") > 0);
        }
    }
    return FALSE;
}

static BOOL Consonant_is_ralf(HDC hdc, SCRIPT_ANALYSIS *psa, ScriptCache *psc,
        const WCHAR *pwChar, const IndicSyllable *s, lexical_function lexical)
{
    if ((lexical(pwChar[s->start])==lex_Ra) && s->end > s->start && lexical(pwChar[s->start+1]) == lex_Halant)
        return (SHAPE_does_GSUB_feature_apply_to_chars(hdc, psa, psc, &pwChar[s->start], 1, 2, "rphf") > 0);
    return FALSE;
}

static int FindBaseConsonant(HDC hdc, SCRIPT_ANALYSIS *psa, ScriptCache *psc,
        const WCHAR *input, IndicSyllable *s, lexical_function lex, BOOL modern)
{
    int i;
    BOOL blwf = FALSE;
    BOOL pref = FALSE;

    /* remove ralf from consideration */
    if (Consonant_is_ralf(hdc, psa, psc, input, s, lex))
    {
        s->ralf = s->start;
        s->start+=2;
    }

    /* try to find a base consonant */
    if (!is_consonant( lex(input[s->base]) ))
    {
        for (i = s->end; i >= s->start; i--)
            if (is_consonant( lex(input[i]) ))
            {
                s->base = i;
                break;
            }
    }

    while ((blwf = Consonant_is_below_base_form(hdc, psa, psc, input, s, lex, modern)) || Consonant_is_post_base_form(hdc, psa, psc, input, s, lex, modern) || (pref = Consonant_is_pre_base_form(hdc, psa, psc, input, s, lex, modern)))
    {
        if (blwf && s->blwf == -1)
            s->blwf = s->base - 1;
        if (pref && s->pref == -1)
            s->pref = s->base - 1;

        for (i = s->base-1; i >= s->start; i--)
            if (is_consonant( lex(input[i]) ))
            {
                s->base = i;
                break;
            }
    }

    if (s->ralf >= 0)
        s->start = s->ralf;

    if (s->ralf == s->base)
        s->ralf = -1;

    return s->base;
}

void Indic_ParseSyllables(HDC hdc, SCRIPT_ANALYSIS *psa, ScriptCache *psc, const WCHAR *input, unsigned int cChar,
        IndicSyllable **syllables, int *syllable_count, lexical_function lex, BOOL modern)
{
    unsigned int center = 0;
    int index = 0;
    int next = 0;

    *syllable_count = 0;

    if (!lex)
    {
        ERR("Failure to have required functions\n");
        return;
    }

    debug_output_string(input, cChar, lex);
    while (next != -1)
    {
        while((next < cChar) && lex(input[next]) == lex_Generic)
            next++;
        index = next;
        if (next >= cChar)
            break;
        next = Indic_process_next_syllable(input, cChar, 0, &center, index, lex);
        if (next != -1)
        {
            if (*syllable_count)
                *syllables = HeapReAlloc(GetProcessHeap(),0,*syllables, sizeof(IndicSyllable)*(*syllable_count+1));
            else
                *syllables = heap_alloc(sizeof(**syllables));
            (*syllables)[*syllable_count].start = index;
            (*syllables)[*syllable_count].base = center;
            (*syllables)[*syllable_count].ralf = -1;
            (*syllables)[*syllable_count].blwf = -1;
            (*syllables)[*syllable_count].pref = -1;
            (*syllables)[*syllable_count].end = next-1;
            FindBaseConsonant(hdc, psa, psc, input, &(*syllables)[*syllable_count], lex, modern);
            index = next;
            *syllable_count = (*syllable_count)+1;
        }
        else if (index < cChar)
        {
            TRACE("Processing failed at %i\n",index);
            next = ++index;
        }
    }
    TRACE("Processed %i of %i characters into %i syllables\n",index,cChar,*syllable_count);
}

void Indic_ReorderCharacters(HDC hdc, SCRIPT_ANALYSIS *psa, ScriptCache *psc, WCHAR *input, unsigned int cChar,
        IndicSyllable **syllables, int *syllable_count, lexical_function lex, reorder_function reorder_f, BOOL modern)
{
    int i;

    if (!reorder_f)
    {
        ERR("Failure to have required functions\n");
        return;
    }

    Indic_ParseSyllables(hdc, psa, psc, input, cChar, syllables, syllable_count, lex, modern);
    for (i = 0; i < *syllable_count; i++)
        reorder_f(input, &(*syllables)[i], lex);
}

/*
 * Implementation of line breaking algorithm for the Uniscribe Script Processor
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

#include "usp10_internal.h"

WINE_DEFAULT_DEBUG_CHANNEL(uniscribe);

extern const unsigned short wine_linebreak_table[] DECLSPEC_HIDDEN;

enum breaking_types {
    b_BK=1, b_CR, b_LF, b_CM, b_SG, b_GL, b_CB, b_SP, b_ZW, b_NL, b_WJ, b_JL, b_JV, b_JT, b_H2, b_H3, b_XX, b_OP, b_CL,
    b_CP, b_QU, b_NS, b_EX, b_SY, b_IS, b_PR, b_PO, b_NU, b_AL, b_ID, b_IN, b_HY, b_BB, b_BA, b_SA, b_AI, b_B2, b_HL,
    b_CJ, b_RI, b_EB, b_EM, b_ZWJ
};

enum breaking_class {b_r=1, b_s, b_x};

static void debug_output_breaks(const short* breaks, int count)
{
    if (TRACE_ON(uniscribe))
    {
        int i;
        TRACE("[");
        for (i = 0; i < count && i < 200; i++)
        {
            switch (breaks[i])
            {
                case b_x: TRACE("x"); break;
                case b_r: TRACE("!"); break;
                case b_s: TRACE("+"); break;
                default: TRACE("*");
            }
        }
        if (i == 200)
            TRACE("...");
        TRACE("]\n");
    }
}

static inline void else_break(short* before, short class)
{
    if (*before == 0)  *before = class;
}

void BREAK_line(const WCHAR *chars, int count, const SCRIPT_ANALYSIS *sa, SCRIPT_LOGATTR *la)
{
    int i,j;
    short *break_class;
    short *break_before;

    TRACE("In      %s\n",debugstr_wn(chars,count));

    break_class = HeapAlloc(GetProcessHeap(),0, count * sizeof(short));
    break_before = HeapAlloc(GetProcessHeap(),0, count * sizeof(short));

    for (i = 0; i < count; i++)
    {
        break_class[i] = get_table_entry( wine_linebreak_table, chars[i] );
        break_before[i] = 0;

        memset(&la[i],0,sizeof(SCRIPT_LOGATTR));

        la[i].fCharStop = TRUE;
        switch (break_class[i])
        {
            case b_BK:
            case b_ZW:
            case b_SP:
                la[i].fWhiteSpace = TRUE;
                break;
            case b_CM:
                la[i].fCharStop = FALSE;
                break;
        }
    }

    /* LB1 */
    /* TODO: Have outside algorithms for these scripts */
    for (i = 0; i < count; i++)
    {
        switch(break_class[i])
        {
            case b_AI:
            case b_SA:
            case b_SG:
            case b_XX:
                break_class[i] = b_AL;
                break;
            case b_CJ:
                break_class[i] = b_NS;
                break;
        }
    }

    /* LB2 - LB3 */
    break_before[0] = b_x;
    for (i = 0; i < count; i++)
    {
        switch(break_class[i])
        {
            /* LB4 - LB6 */
            case b_CR:
                if (i < count-1 && break_class[i+1] == b_LF)
                {
                    else_break(&break_before[i],b_x);
                    else_break(&break_before[i+1],b_x);
                    break;
                }
            case b_LF:
            case b_NL:
            case b_BK:
                if (i < count-1) else_break(&break_before[i+1],b_r);
                else_break(&break_before[i],b_x);
                break;
            /* LB7 */
            case b_SP:
                else_break(&break_before[i],b_x);
                break;
            case b_ZW:
                else_break(&break_before[i],b_x);
            /* LB8 */
                while (i < count-1 && break_class[i+1] == b_SP)
                    i++;
                else_break(&break_before[i],b_s);
                break;
        }
    }

    debug_output_breaks(break_before,count);

    /* LB9 - LB10 */
    for (i = 0; i < count; i++)
    {
        if (break_class[i] == b_CM)
        {
            if (i > 0)
            {
                switch (break_class[i-1])
                {
                    case b_SP:
                    case b_BK:
                    case b_CR:
                    case b_LF:
                    case b_NL:
                    case b_ZW:
                        break_class[i] = b_AL;
                        break;
                    default:
                        break_class[i] = break_class[i-1];
                }
            }
            else break_class[i] = b_AL;
        }
    }

    for (i = 0; i < count; i++)
    {
        switch(break_class[i])
        {
            /* LB11 */
            case b_WJ:
                else_break(&break_before[i],b_x);
                if (i < count-1)
                    else_break(&break_before[i+1],b_x);
                break;
            /* LB12 */
            case b_GL:
                if (i < count-1)
                    else_break(&break_before[i+1],b_x);
            /* LB12a */
                if (i > 0)
                {
                    if (break_class[i-1] != b_SP &&
                        break_class[i-1] != b_BA &&
                        break_class[i-1] != b_HY)
                        else_break(&break_before[i],b_x);
                }
                break;
            /* LB13 */
            case b_CL:
            case b_CP:
            case b_EX:
            case b_IS:
            case b_SY:
                else_break(&break_before[i],b_x);
                break;
            /* LB14 */
            case b_OP:
                while (i < count-1 && break_class[i+1] == b_SP)
                {
                    else_break(&break_before[i+1],b_x);
                    i++;
                }
                else_break(&break_before[i+1],b_x);
                break;
            /* LB15 */
            case b_QU:
                j = i+1;
                while (j < count-1 && break_class[j] == b_SP)
                    j++;
                if (break_class[j] == b_OP)
                {
                    for (; j > i; j--)
                        else_break(&break_before[j],b_x);
                }
                break;
            /* LB16 */
            case b_NS:
                j = i-1;
                while(j > 0 && break_class[j] == b_SP)
                    j--;
                if (break_class[j] == b_CL || break_class[j] == b_CP)
                {
                    for (j++; j <= i; j++)
                        else_break(&break_before[j],b_x);
                }
                break;
            /* LB17 */
            case b_B2:
                j = i+1;
                while (j < count && break_class[j] == b_SP)
                    j++;
                if (break_class[j] == b_B2)
                {
                    for (; j > i; j--)
                        else_break(&break_before[j],b_x);
                }
                break;
        }
    }

    debug_output_breaks(break_before,count);

    for (i = 0; i < count; i++)
    {
        switch(break_class[i])
        {
            /* LB18 */
            case b_SP:
                if (i < count-1)
                    else_break(&break_before[i+1],b_s);
                break;
            /* LB19 */
            case b_QU:
                else_break(&break_before[i],b_x);
                if (i < count-1)
                    else_break(&break_before[i+1],b_x);
                break;
            /* LB20 */
            case b_CB:
                else_break(&break_before[i],b_s);
                if (i < count-1)
                    else_break(&break_before[i+1],b_s);
                break;
            /* LB21 */
            case b_BA:
            case b_HY:
            case b_NS:
                else_break(&break_before[i],b_x);
                break;
            case b_BB:
                if (i < count-1)
                    else_break(&break_before[i+1],b_x);
                break;
            /* LB21a */
            case b_HL:
                if (i < count-2)
                    switch (break_class[i+1])
                    {
                    case b_HY:
                    case b_BA:
                        else_break(&break_before[i+2], b_x);
                    }
                break;
            /* LB22 */
            case b_IN:
                if (i > 0)
                {
                    switch (break_class[i-1])
                    {
                        case b_AL:
                        case b_HL:
                        case b_ID:
                        case b_IN:
                        case b_NU:
                            else_break(&break_before[i], b_x);
                    }
                }
                break;
        }

        if (i < count-1)
        {
            /* LB23 */
            if ((break_class[i] == b_ID && break_class[i+1] == b_PO) ||
                (break_class[i] == b_AL && break_class[i+1] == b_NU) ||
                (break_class[i] == b_HL && break_class[i+1] == b_NU) ||
                (break_class[i] == b_NU && break_class[i+1] == b_AL) ||
                (break_class[i] == b_NU && break_class[i+1] == b_HL))
                    else_break(&break_before[i+1],b_x);
            /* LB24 */
            if ((break_class[i] == b_PR && break_class[i+1] == b_ID) ||
                (break_class[i] == b_PR && break_class[i+1] == b_AL) ||
                (break_class[i] == b_PR && break_class[i+1] == b_HL) ||
                (break_class[i] == b_PO && break_class[i+1] == b_AL) ||
                (break_class[i] == b_PO && break_class[i+1] == b_HL))
                    else_break(&break_before[i+1],b_x);

            /* LB25 */
            if ((break_class[i] == b_CL && break_class[i+1] == b_PO) ||
                (break_class[i] == b_CP && break_class[i+1] == b_PO) ||
                (break_class[i] == b_CL && break_class[i+1] == b_PR) ||
                (break_class[i] == b_CP && break_class[i+1] == b_PR) ||
                (break_class[i] == b_NU && break_class[i+1] == b_PO) ||
                (break_class[i] == b_NU && break_class[i+1] == b_PR) ||
                (break_class[i] == b_PO && break_class[i+1] == b_OP) ||
                (break_class[i] == b_PO && break_class[i+1] == b_NU) ||
                (break_class[i] == b_PR && break_class[i+1] == b_OP) ||
                (break_class[i] == b_PR && break_class[i+1] == b_NU) ||
                (break_class[i] == b_HY && break_class[i+1] == b_NU) ||
                (break_class[i] == b_IS && break_class[i+1] == b_NU) ||
                (break_class[i] == b_NU && break_class[i+1] == b_NU) ||
                (break_class[i] == b_SY && break_class[i+1] == b_NU))
                    else_break(&break_before[i+1],b_x);

            /* LB26 */
            if (break_class[i] == b_JL)
            {
                switch (break_class[i+1])
                {
                    case b_JL:
                    case b_JV:
                    case b_H2:
                    case b_H3:
                        else_break(&break_before[i+1],b_x);
                }
            }
            if ((break_class[i] == b_JV || break_class[i] == b_H2) &&
                (break_class[i+1] == b_JV || break_class[i+1] == b_JT))
                    else_break(&break_before[i+1],b_x);
            if ((break_class[i] == b_JT || break_class[i] == b_H3) &&
                 break_class[i+1] == b_JT)
                    else_break(&break_before[i+1],b_x);

            /* LB27 */
            switch (break_class[i])
            {
                case b_JL:
                case b_JV:
                case b_JT:
                case b_H2:
                case b_H3:
                    if (break_class[i+1] == b_IN || break_class[i+1] == b_PO)
                        else_break(&break_before[i+1],b_x);
            }
            if (break_class[i] == b_PR)
            {
                switch (break_class[i+1])
                {
                    case b_JL:
                    case b_JV:
                    case b_JT:
                    case b_H2:
                    case b_H3:
                        else_break(&break_before[i+1],b_x);
                }
            }

            /* LB28 */
            if ((break_class[i] == b_AL && break_class[i+1] == b_AL) ||
                (break_class[i] == b_AL && break_class[i+1] == b_HL) ||
                (break_class[i] == b_HL && break_class[i+1] == b_AL) ||
                (break_class[i] == b_HL && break_class[i+1] == b_HL))
                else_break(&break_before[i+1],b_x);

            /* LB29 */
            if ((break_class[i] == b_IS && break_class[i+1] == b_AL) ||
                (break_class[i] == b_IS && break_class[i+1] == b_HL))
                else_break(&break_before[i+1],b_x);

            /* LB30 */
            if ((break_class[i] == b_AL || break_class[i] == b_HL || break_class[i] == b_NU) &&
                 break_class[i+1] == b_OP)
                else_break(&break_before[i+1],b_x);
            if (break_class[i] == b_CP &&
                (break_class[i+1] == b_AL || break_class[i+1] == b_HL || break_class[i+1] == b_NU))
                else_break(&break_before[i+1],b_x);

            /* LB30a */
            if (break_class[i] == b_RI && break_class[i+1] == b_RI)
                else_break(&break_before[i+1],b_x);
        }
    }
    debug_output_breaks(break_before,count);

    /* LB31 */
    for (i = 0; i < count-1; i++)
        else_break(&break_before[i+1],b_s);

    debug_output_breaks(break_before,count);
    for (i = 0; i < count; i++)
    {
        if (break_before[i] != b_x)
        {
            la[i].fSoftBreak = TRUE;
            la[i].fWordStop = TRUE;
        }
    }

    HeapFree(GetProcessHeap(), 0, break_before);
    HeapFree(GetProcessHeap(), 0, break_class);
}

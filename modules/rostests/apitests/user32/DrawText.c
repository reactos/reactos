/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Test for DrawText
 * COPYRIGHT:   Copyright 2019-2020 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

typedef struct YY
{
    LONG top;
    LONG bottom;
} YY;

typedef struct TEST_ENTRY
{
    INT line;
    UINT uFormat;
    LPCSTR pszText;
    INT ret;
    LONG font_height;
    YY input;
    YY output;
} TEST_ENTRY;

#define DT_1 (DT_SINGLELINE | DT_LEFT | DT_TOP | DT_CALCRECT)
#define DT_2 (DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_CALCRECT)
#define DT_3 (DT_LEFT | DT_TOP | DT_CALCRECT)
#define DT_4 (DT_LEFT | DT_VCENTER | DT_CALCRECT)
#define DT_5 (DT_LEFT | DT_TOP | DT_CALCRECT | DT_EDITCONTROL)

static const TEST_ENTRY s_entries[] =
{
    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 0, -2 }, { 0, 12 } },
    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 0, -1 }, { 0, 12 } },
    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 0, 0 }, { 0, 12 } },
    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 0, 1 }, { 0, 12 } },
    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 0, 2 }, { 0, 12 } },
    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 0, 3 }, { 0, 12 } },
    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 0, 4 }, { 0, 12 } },
    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 0, 5 }, { 0, 12 } },
    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 0, 6 }, { 0, 12 } },

    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 1, -2 }, { 1, 13 } },
    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 1, -1 }, { 1, 13 } },
    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 1, 0 }, { 1, 13 } },
    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 1, 1 }, { 1, 13 } },
    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 1, 2 }, { 1, 13 } },
    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 1, 3 }, { 1, 13 } },
    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 1, 4 }, { 1, 13 } },
    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 1, 5 }, { 1, 13 } },
    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 1, 6 }, { 1, 13 } },

    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 0, -2 }, { 0, 13 } },
    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 0, -1 }, { 0, 13 } },
    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 0, 0 }, { 0, 13 } },
    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 0, 1 }, { 0, 13 } },
    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 0, 2 }, { 0, 13 } },
    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 0, 3 }, { 0, 13 } },
    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 0, 4 }, { 0, 13 } },
    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 0, 5 }, { 0, 13 } },
    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 0, 6 }, { 0, 13 } },

    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 1, -2 }, { 1, 14 } },
    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 1, -1 }, { 1, 14 } },
    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 1, 0 }, { 1, 14 } },
    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 1, 1 }, { 1, 14 } },
    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 1, 2 }, { 1, 14 } },
    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 1, 3 }, { 1, 14 } },
    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 1, 4 }, { 1, 14 } },
    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 1, 5 }, { 1, 14 } },
    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 1, 6 }, { 1, 14 } },

    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 0, -2 }, { 0, 14 } },
    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 0, -1 }, { 0, 14 } },
    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 0, 0 }, { 0, 14 } },
    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 0, 1 }, { 0, 14 } },
    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 0, 2 }, { 0, 14 } },
    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 0, 3 }, { 0, 14 } },
    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 0, 4 }, { 0, 14 } },
    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 0, 5 }, { 0, 14 } },
    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 0, 6 }, { 0, 14 } },

    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 1, -2 }, { 1, 15 } },
    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 1, -1 }, { 1, 15 } },
    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 1, 0 }, { 1, 15 } },
    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 1, 1 }, { 1, 15 } },
    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 1, 2 }, { 1, 15 } },
    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 1, 3 }, { 1, 15 } },
    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 1, 4 }, { 1, 15 } },
    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 1, 5 }, { 1, 15 } },
    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 1, 6 }, { 1, 15 } },

    { __LINE__, DT_2, "ABCabc123g", 5, -10, { 0, -2 }, { 0, 5 } },
    { __LINE__, DT_2, "ABCabc123g", 6, -10, { 0, -1 }, { 0, 6 } },
    { __LINE__, DT_2, "ABCabc123g", 6, -10, { 0, 0 }, { 0, 6 } },
    { __LINE__, DT_2, "ABCabc123g", 7, -10, { 0, 1 }, { 0, 7 } },
    { __LINE__, DT_2, "ABCabc123g", 7, -10, { 0, 2 }, { 0, 7 } },
    { __LINE__, DT_2, "ABCabc123g", 8, -10, { 0, 3 }, { 0, 8 } },
    { __LINE__, DT_2, "ABCabc123g", 8, -10, { 0, 4 }, { 0, 8 } },
    { __LINE__, DT_2, "ABCabc123g", 9, -10, { 0, 5 }, { 0, 9 } },
    { __LINE__, DT_2, "ABCabc123g", 9, -10, { 0, 6 }, { 0, 9 } },

    { __LINE__, DT_2, "ABCabc123g", 5, -10, { 1, -2 }, { 1, 6 } },
    { __LINE__, DT_2, "ABCabc123g", 5, -10, { 1, -1 }, { 1, 6 } },
    { __LINE__, DT_2, "ABCabc123g", 6, -10, { 1, 0 }, { 1, 7 } },
    { __LINE__, DT_2, "ABCabc123g", 6, -10, { 1, 1 }, { 1, 7 } },
    { __LINE__, DT_2, "ABCabc123g", 7, -10, { 1, 2 }, { 1, 8 } },
    { __LINE__, DT_2, "ABCabc123g", 7, -10, { 1, 3 }, { 1, 8 } },
    { __LINE__, DT_2, "ABCabc123g", 8, -10, { 1, 4 }, { 1, 9 } },
    { __LINE__, DT_2, "ABCabc123g", 8, -10, { 1, 5 }, { 1, 9 } },
    { __LINE__, DT_2, "ABCabc123g", 9, -10, { 1, 6 }, { 1, 10 } },

    { __LINE__, DT_2, "ABCabc123g", 6, -11, { 0, -2 }, { 0, 6 } },
    { __LINE__, DT_2, "ABCabc123g", 6, -11, { 0, -1 }, { 0, 6 } },
    { __LINE__, DT_2, "ABCabc123g", 7, -11, { 0, 0 }, { 0, 7 } },
    { __LINE__, DT_2, "ABCabc123g", 7, -11, { 0, 1 }, { 0, 7 } },
    { __LINE__, DT_2, "ABCabc123g", 8, -11, { 0, 2 }, { 0, 8 } },
    { __LINE__, DT_2, "ABCabc123g", 8, -11, { 0, 3 }, { 0, 8 } },
    { __LINE__, DT_2, "ABCabc123g", 9, -11, { 0, 4 }, { 0, 9 } },
    { __LINE__, DT_2, "ABCabc123g", 9, -11, { 0, 5 }, { 0, 9 } },
    { __LINE__, DT_2, "ABCabc123g", 10, -11, { 0, 6 }, { 0, 10 } },

    { __LINE__, DT_2, "ABCabc123g", 5, -11, { 1, -2 }, { 1, 6 } },
    { __LINE__, DT_2, "ABCabc123g", 6, -11, { 1, -1 }, { 1, 7 } },
    { __LINE__, DT_2, "ABCabc123g", 6, -11, { 1, 0 }, { 1, 7 } },
    { __LINE__, DT_2, "ABCabc123g", 7, -11, { 1, 1 }, { 1, 8 } },
    { __LINE__, DT_2, "ABCabc123g", 7, -11, { 1, 2 }, { 1, 8 } },
    { __LINE__, DT_2, "ABCabc123g", 8, -11, { 1, 3 }, { 1, 9 } },
    { __LINE__, DT_2, "ABCabc123g", 8, -11, { 1, 4 }, { 1, 9 } },
    { __LINE__, DT_2, "ABCabc123g", 9, -11, { 1, 5 }, { 1, 10 } },
    { __LINE__, DT_2, "ABCabc123g", 9, -11, { 1, 6 }, { 1, 10 } },

    { __LINE__, DT_2, "ABCabc123g", 6, -12, { 0, -2 }, { 0, 6 } },
    { __LINE__, DT_2, "ABCabc123g", 7, -12, { 0, -1 }, { 0, 7 } },
    { __LINE__, DT_2, "ABCabc123g", 7, -12, { 0, 0 }, { 0, 7 } },
    { __LINE__, DT_2, "ABCabc123g", 8, -12, { 0, 1 }, { 0, 8 } },
    { __LINE__, DT_2, "ABCabc123g", 8, -12, { 0, 2 }, { 0, 8 } },
    { __LINE__, DT_2, "ABCabc123g", 9, -12, { 0, 3 }, { 0, 9 } },
    { __LINE__, DT_2, "ABCabc123g", 9, -12, { 0, 4 }, { 0, 9 } },
    { __LINE__, DT_2, "ABCabc123g", 10, -12, { 0, 5 }, { 0, 10 } },
    { __LINE__, DT_2, "ABCabc123g", 10, -12, { 0, 6 }, { 0, 10 } },

    { __LINE__, DT_2, "ABCabc123g", 6, -12, { 1, -2 }, { 1, 7 } },
    { __LINE__, DT_2, "ABCabc123g", 6, -12, { 1, -1 }, { 1, 7 } },
    { __LINE__, DT_2, "ABCabc123g", 7, -12, { 1, 0 }, { 1, 8 } },
    { __LINE__, DT_2, "ABCabc123g", 7, -12, { 1, 1 }, { 1, 8 } },
    { __LINE__, DT_2, "ABCabc123g", 8, -12, { 1, 2 }, { 1, 9 } },
    { __LINE__, DT_2, "ABCabc123g", 8, -12, { 1, 3 }, { 1, 9 } },
    { __LINE__, DT_2, "ABCabc123g", 9, -12, { 1, 4 }, { 1, 10 } },
    { __LINE__, DT_2, "ABCabc123g", 9, -12, { 1, 5 }, { 1, 10 } },
    { __LINE__, DT_2, "ABCabc123g", 10, -12, { 1, 6 }, { 1, 11 } },

    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 0, -2 }, { 0, 24 } },
    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 0, -1 }, { 0, 24 } },
    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 0, 0 }, { 0, 24 } },
    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 0, 1 }, { 0, 24 } },
    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 0, 2 }, { 0, 24 } },
    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 0, 3 }, { 0, 24 } },
    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 0, 4 }, { 0, 24 } },
    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 0, 5 }, { 0, 24 } },
    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 0, 6 }, { 0, 24 } },

    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 1, -2 }, { 1, 25 } },
    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 1, -1 }, { 1, 25 } },
    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 1, 0 }, { 1, 25 } },
    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 1, 1 }, { 1, 25 } },
    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 1, 2 }, { 1, 25 } },
    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 1, 3 }, { 1, 25 } },
    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 1, 4 }, { 1, 25 } },
    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 1, 5 }, { 1, 25 } },
    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 1, 6 }, { 1, 25 } },

    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 0, -2 }, { 0, 26 } },
    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 0, -1 }, { 0, 26 } },
    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 0, 0 }, { 0, 26 } },
    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 0, 1 }, { 0, 26 } },
    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 0, 2 }, { 0, 26 } },
    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 0, 3 }, { 0, 26 } },
    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 0, 4 }, { 0, 26 } },
    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 0, 5 }, { 0, 26 } },
    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 0, 6 }, { 0, 26 } },

    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 1, -2 }, { 1, 27 } },
    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 1, -1 }, { 1, 27 } },
    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 1, 0 }, { 1, 27 } },
    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 1, 1 }, { 1, 27 } },
    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 1, 2 }, { 1, 27 } },
    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 1, 3 }, { 1, 27 } },
    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 1, 4 }, { 1, 27 } },
    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 1, 5 }, { 1, 27 } },
    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 1, 6 }, { 1, 27 } },

    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 0, -2 }, { 0, 28 } },
    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 0, -1 }, { 0, 28 } },
    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 0, 0 }, { 0, 28 } },
    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 0, 1 }, { 0, 28 } },
    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 0, 2 }, { 0, 28 } },
    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 0, 3 }, { 0, 28 } },
    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 0, 4 }, { 0, 28 } },
    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 0, 5 }, { 0, 28 } },
    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 0, 6 }, { 0, 28 } },

    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 1, -2 }, { 1, 29 } },
    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 1, -1 }, { 1, 29 } },
    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 1, 0 }, { 1, 29 } },
    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 1, 1 }, { 1, 29 } },
    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 1, 2 }, { 1, 29 } },
    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 1, 3 }, { 1, 29 } },
    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 1, 4 }, { 1, 29 } },
    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 1, 5 }, { 1, 29 } },
    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 1, 6 }, { 1, 29 } },

    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 0, -2 }, { 0, 24 } },
    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 0, -1 }, { 0, 24 } },
    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 0, 0 }, { 0, 24 } },
    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 0, 1 }, { 0, 24 } },
    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 0, 2 }, { 0, 24 } },
    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 0, 3 }, { 0, 24 } },
    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 0, 4 }, { 0, 24 } },
    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 0, 5 }, { 0, 24 } },
    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 0, 6 }, { 0, 24 } },

    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 1, -2 }, { 1, 25 } },
    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 1, -1 }, { 1, 25 } },
    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 1, 0 }, { 1, 25 } },
    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 1, 1 }, { 1, 25 } },
    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 1, 2 }, { 1, 25 } },
    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 1, 3 }, { 1, 25 } },
    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 1, 4 }, { 1, 25 } },
    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 1, 5 }, { 1, 25 } },
    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 1, 6 }, { 1, 25 } },

    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 0, -2 }, { 0, 26 } },
    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 0, -1 }, { 0, 26 } },
    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 0, 0 }, { 0, 26 } },
    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 0, 1 }, { 0, 26 } },
    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 0, 2 }, { 0, 26 } },
    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 0, 3 }, { 0, 26 } },
    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 0, 4 }, { 0, 26 } },
    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 0, 5 }, { 0, 26 } },
    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 0, 6 }, { 0, 26 } },

    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 1, -2 }, { 1, 27 } },
    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 1, -1 }, { 1, 27 } },
    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 1, 0 }, { 1, 27 } },
    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 1, 1 }, { 1, 27 } },
    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 1, 2 }, { 1, 27 } },
    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 1, 3 }, { 1, 27 } },
    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 1, 4 }, { 1, 27 } },
    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 1, 5 }, { 1, 27 } },
    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 1, 6 }, { 1, 27 } },

    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 0, -2 }, { 0, 28 } },
    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 0, -1 }, { 0, 28 } },
    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 0, 0 }, { 0, 28 } },
    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 0, 1 }, { 0, 28 } },
    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 0, 2 }, { 0, 28 } },
    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 0, 3 }, { 0, 28 } },
    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 0, 4 }, { 0, 28 } },
    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 0, 5 }, { 0, 28 } },
    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 0, 6 }, { 0, 28 } },

    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 1, -2 }, { 1, 29 } },
    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 1, -1 }, { 1, 29 } },
    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 1, 0 }, { 1, 29 } },
    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 1, 1 }, { 1, 29 } },
    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 1, 2 }, { 1, 29 } },
    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 1, 3 }, { 1, 29 } },
    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 1, 4 }, { 1, 29 } },
    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 1, 5 }, { 1, 29 } },
    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 1, 6 }, { 1, 29 } },

    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 0, -2 }, { 0, 24 } },
    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 0, -1 }, { 0, 24 } },
    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 0, 0 }, { 0, 24 } },
    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 0, 1 }, { 0, 24 } },
    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 0, 2 }, { 0, 24 } },
    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 0, 3 }, { 0, 24 } },
    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 0, 4 }, { 0, 24 } },
    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 0, 5 }, { 0, 24 } },
    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 0, 6 }, { 0, 24 } },

    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 1, -2 }, { 1, 25 } },
    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 1, -1 }, { 1, 25 } },
    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 1, 0 }, { 1, 25 } },
    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 1, 1 }, { 1, 25 } },
    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 1, 2 }, { 1, 25 } },
    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 1, 3 }, { 1, 25 } },
    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 1, 4 }, { 1, 25 } },
    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 1, 5 }, { 1, 25 } },
    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 1, 6 }, { 1, 25 } },

    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 0, -2 }, { 0, 26 } },
    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 0, -1 }, { 0, 26 } },
    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 0, 0 }, { 0, 26 } },
    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 0, 1 }, { 0, 26 } },
    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 0, 2 }, { 0, 26 } },
    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 0, 3 }, { 0, 26 } },
    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 0, 4 }, { 0, 26 } },
    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 0, 5 }, { 0, 26 } },
    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 0, 6 }, { 0, 26 } },

    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 1, -2 }, { 1, 27 } },
    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 1, -1 }, { 1, 27 } },
    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 1, 0 }, { 1, 27 } },
    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 1, 1 }, { 1, 27 } },
    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 1, 2 }, { 1, 27 } },
    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 1, 3 }, { 1, 27 } },
    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 1, 4 }, { 1, 27 } },
    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 1, 5 }, { 1, 27 } },
    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 1, 6 }, { 1, 27 } },

    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 0, -2 }, { 0, 28 } },
    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 0, -1 }, { 0, 28 } },
    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 0, 0 }, { 0, 28 } },
    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 0, 1 }, { 0, 28 } },
    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 0, 2 }, { 0, 28 } },
    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 0, 3 }, { 0, 28 } },
    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 0, 4 }, { 0, 28 } },
    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 0, 5 }, { 0, 28 } },
    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 0, 6 }, { 0, 28 } },

    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 1, -2 }, { 1, 29 } },
    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 1, -1 }, { 1, 29 } },
    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 1, 0 }, { 1, 29 } },
    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 1, 1 }, { 1, 29 } },
    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 1, 2 }, { 1, 29 } },
    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 1, 3 }, { 1, 29 } },
    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 1, 4 }, { 1, 29 } },
    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 1, 5 }, { 1, 29 } },
    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 1, 6 }, { 1, 29 } },
};

static void DoEntry(HDC hdc, const TEST_ENTRY *pEntry)
{
    RECT rc;
    INT ret;
    HFONT hFont;
    HGDIOBJ hFontOld;
    LOGFONTW lf = { pEntry->font_height };
    lstrcpyW(lf.lfFaceName, L"Tahoma");

    hFont = CreateFontIndirectW(&lf);
    ok(hFont != NULL, "hFont is NULL\n");

    hFontOld = SelectObject(hdc, hFont);
    {
        SetRect(&rc, 0, pEntry->input.top, 0, pEntry->input.bottom);

        ret = DrawTextA(hdc, pEntry->pszText, -1, &rc, pEntry->uFormat);
        ok(ret == pEntry->ret,
           "Line %d: ret %d vs %d\n", pEntry->line, ret, pEntry->ret);

        ok(rc.top == pEntry->output.top,
           "Line %d: top %ld vs %ld\n", pEntry->line, rc.top, pEntry->output.top);

        ok(rc.bottom == pEntry->output.bottom,
           "Line %d: bottom %ld vs %ld\n", pEntry->line, rc.bottom, pEntry->output.bottom);
    }
    SelectObject(hdc, hFontOld);
    DeleteObject(hFont);
}

START_TEST(DrawText)
{
    SIZE_T i;
    HBITMAP hbm;
    HDC hdc = CreateCompatibleDC(NULL);
    ok(hdc != NULL, "hdc was NULL\n");

    for (i = 0; i < ARRAYSIZE(s_entries); ++i)
    {
        DoEntry(hdc, &s_entries[i]);
    }

    DeleteDC(hdc);
}

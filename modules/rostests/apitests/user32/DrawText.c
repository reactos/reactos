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
    RECT input;
    YY output;
} TEST_ENTRY;

#define DT_1 (DT_CALCRECT | DT_SINGLELINE | DT_LEFT | DT_TOP)
#define DT_2 (DT_CALCRECT | DT_SINGLELINE | DT_LEFT | DT_VCENTER)
#define DT_3 (DT_CALCRECT | DT_LEFT | DT_TOP)
#define DT_4 (DT_CALCRECT | DT_LEFT | DT_VCENTER)
#define DT_5 (DT_CALCRECT | DT_LEFT | DT_TOP | DT_EDITCONTROL)

#define DT_6 (DT_SINGLELINE | DT_LEFT | DT_TOP)
#define DT_7 (DT_SINGLELINE | DT_LEFT | DT_VCENTER)
#define DT_8 (DT_LEFT | DT_TOP)
#define DT_9 (DT_LEFT | DT_VCENTER)
#define DT_10 (DT_LEFT | DT_TOP | DT_EDITCONTROL)

static const TEST_ENTRY s_entries[] =
{
    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 0, 0, 0, -2 }, { 0, 12 } },
    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 0, 0, 0, -1 }, { 0, 12 } },
    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 0, 0, 0, 0 }, { 0, 12 } },
    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 0, 0, 0, 1 }, { 0, 12 } },
    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 0, 0, 0, 2 }, { 0, 12 } },
    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 0, 0, 0, 3 }, { 0, 12 } },
    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 0, 0, 0, 4 }, { 0, 12 } },
    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 0, 0, 0, 5 }, { 0, 12 } },
    { __LINE__, DT_1, "ABCabc123g", 12, -10, { 0, 0, 0, 6 }, { 0, 12 } },

    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 0, 0, 0, -2 }, { 0, 13 } },
    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 0, 0, 0, -1 }, { 0, 13 } },
    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 0, 0, 0, 0 }, { 0, 13 } },
    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 0, 0, 0, 1 }, { 0, 13 } },
    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 0, 0, 0, 2 }, { 0, 13 } },
    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 0, 0, 0, 3 }, { 0, 13 } },
    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 0, 0, 0, 4 }, { 0, 13 } },
    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 0, 0, 0, 5 }, { 0, 13 } },
    { __LINE__, DT_1, "ABCabc123g", 13, -11, { 0, 0, 0, 6 }, { 0, 13 } },

    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 0, 0, 0, -2 }, { 0, 14 } },
    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 0, 0, 0, -1 }, { 0, 14 } },
    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 0, 0, 0, 0 }, { 0, 14 } },
    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 0, 0, 0, 1 }, { 0, 14 } },
    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 0, 0, 0, 2 }, { 0, 14 } },
    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 0, 0, 0, 3 }, { 0, 14 } },
    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 0, 0, 0, 4 }, { 0, 14 } },
    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 0, 0, 0, 5 }, { 0, 14 } },
    { __LINE__, DT_1, "ABCabc123g", 14, -12, { 0, 0, 0, 6 }, { 0, 14 } },

    { __LINE__, DT_2, "ABCabc123g", 5, -10, { 0, 0, 0, -2 }, { 0, 5 } },
    { __LINE__, DT_2, "ABCabc123g", 6, -10, { 0, 0, 0, -1 }, { 0, 6 } },
    { __LINE__, DT_2, "ABCabc123g", 6, -10, { 0, 0, 0, 0 }, { 0, 6 } },
    { __LINE__, DT_2, "ABCabc123g", 7, -10, { 0, 0, 0, 1 }, { 0, 7 } },
    { __LINE__, DT_2, "ABCabc123g", 7, -10, { 0, 0, 0, 2 }, { 0, 7 } },
    { __LINE__, DT_2, "ABCabc123g", 8, -10, { 0, 0, 0, 3 }, { 0, 8 } },
    { __LINE__, DT_2, "ABCabc123g", 8, -10, { 0, 0, 0, 4 }, { 0, 8 } },
    { __LINE__, DT_2, "ABCabc123g", 9, -10, { 0, 0, 0, 5 }, { 0, 9 } },
    { __LINE__, DT_2, "ABCabc123g", 9, -10, { 0, 0, 0, 6 }, { 0, 9 } },

    { __LINE__, DT_2, "ABCabc123g", 6, -11, { 0, 0, 0, -2 }, { 0, 6 } },
    { __LINE__, DT_2, "ABCabc123g", 6, -11, { 0, 0, 0, -1 }, { 0, 6 } },
    { __LINE__, DT_2, "ABCabc123g", 7, -11, { 0, 0, 0, 0 }, { 0, 7 } },
    { __LINE__, DT_2, "ABCabc123g", 7, -11, { 0, 0, 0, 1 }, { 0, 7 } },
    { __LINE__, DT_2, "ABCabc123g", 8, -11, { 0, 0, 0, 2 }, { 0, 8 } },
    { __LINE__, DT_2, "ABCabc123g", 8, -11, { 0, 0, 0, 3 }, { 0, 8 } },
    { __LINE__, DT_2, "ABCabc123g", 9, -11, { 0, 0, 0, 4 }, { 0, 9 } },
    { __LINE__, DT_2, "ABCabc123g", 9, -11, { 0, 0, 0, 5 }, { 0, 9 } },
    { __LINE__, DT_2, "ABCabc123g", 10, -11, { 0, 0, 0, 6 }, { 0, 10 } },

    { __LINE__, DT_2, "ABCabc123g", 6, -12, { 0, 0, 0, -2 }, { 0, 6 } },
    { __LINE__, DT_2, "ABCabc123g", 7, -12, { 0, 0, 0, -1 }, { 0, 7 } },
    { __LINE__, DT_2, "ABCabc123g", 7, -12, { 0, 0, 0, 0 }, { 0, 7 } },
    { __LINE__, DT_2, "ABCabc123g", 8, -12, { 0, 0, 0, 1 }, { 0, 8 } },
    { __LINE__, DT_2, "ABCabc123g", 8, -12, { 0, 0, 0, 2 }, { 0, 8 } },
    { __LINE__, DT_2, "ABCabc123g", 9, -12, { 0, 0, 0, 3 }, { 0, 9 } },
    { __LINE__, DT_2, "ABCabc123g", 9, -12, { 0, 0, 0, 4 }, { 0, 9 } },
    { __LINE__, DT_2, "ABCabc123g", 10, -12, { 0, 0, 0, 5 }, { 0, 10 } },
    { __LINE__, DT_2, "ABCabc123g", 10, -12, { 0, 0, 0, 6 }, { 0, 10 } },

    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 0, 0, 0, -2 }, { 0, 24 } },
    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 0, 0, 0, -1 }, { 0, 24 } },
    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 0, 0, 0, 0 }, { 0, 24 } },
    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 0, 0, 0, 1 }, { 0, 24 } },
    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 0, 0, 0, 2 }, { 0, 24 } },
    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 0, 0, 0, 3 }, { 0, 24 } },
    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 0, 0, 0, 4 }, { 0, 24 } },
    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 0, 0, 0, 5 }, { 0, 24 } },
    { __LINE__, DT_3, "ABCabc\n123g", 24, -10, { 0, 0, 0, 6 }, { 0, 24 } },

    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 0, 0, 0, -2 }, { 0, 26 } },
    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 0, 0, 0, -1 }, { 0, 26 } },
    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 0, 0, 0, 0 }, { 0, 26 } },
    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 0, 0, 0, 1 }, { 0, 26 } },
    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 0, 0, 0, 2 }, { 0, 26 } },
    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 0, 0, 0, 3 }, { 0, 26 } },
    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 0, 0, 0, 4 }, { 0, 26 } },
    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 0, 0, 0, 5 }, { 0, 26 } },
    { __LINE__, DT_3, "ABCabc\n123g", 26, -11, { 0, 0, 0, 6 }, { 0, 26 } },

    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 0, 0, 0, -2 }, { 0, 28 } },
    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 0, 0, 0, -1 }, { 0, 28 } },
    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 0, 0, 0, 0 }, { 0, 28 } },
    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 0, 0, 0, 1 }, { 0, 28 } },
    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 0, 0, 0, 2 }, { 0, 28 } },
    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 0, 0, 0, 3 }, { 0, 28 } },
    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 0, 0, 0, 4 }, { 0, 28 } },
    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 0, 0, 0, 5 }, { 0, 28 } },
    { __LINE__, DT_3, "ABCabc\n123g", 28, -12, { 0, 0, 0, 6 }, { 0, 28 } },

    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 0, 0, 0, -2 }, { 0, 24 } },
    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 0, 0, 0, -1 }, { 0, 24 } },
    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 0, 0, 0, 0 }, { 0, 24 } },
    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 0, 0, 0, 1 }, { 0, 24 } },
    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 0, 0, 0, 2 }, { 0, 24 } },
    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 0, 0, 0, 3 }, { 0, 24 } },
    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 0, 0, 0, 4 }, { 0, 24 } },
    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 0, 0, 0, 5 }, { 0, 24 } },
    { __LINE__, DT_4, "ABCabc\n123g", 24, -10, { 0, 0, 0, 6 }, { 0, 24 } },

    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 0, 0, 0, -2 }, { 0, 26 } },
    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 0, 0, 0, -1 }, { 0, 26 } },
    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 0, 0, 0, 0 }, { 0, 26 } },
    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 0, 0, 0, 1 }, { 0, 26 } },
    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 0, 0, 0, 2 }, { 0, 26 } },
    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 0, 0, 0, 3 }, { 0, 26 } },
    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 0, 0, 0, 4 }, { 0, 26 } },
    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 0, 0, 0, 5 }, { 0, 26 } },
    { __LINE__, DT_4, "ABCabc\n123g", 26, -11, { 0, 0, 0, 6 }, { 0, 26 } },

    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 0, 0, 0, -2 }, { 0, 28 } },
    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 0, 0, 0, -1 }, { 0, 28 } },
    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 0, 0, 0, 0 }, { 0, 28 } },
    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 0, 0, 0, 1 }, { 0, 28 } },
    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 0, 0, 0, 2 }, { 0, 28 } },
    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 0, 0, 0, 3 }, { 0, 28 } },
    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 0, 0, 0, 4 }, { 0, 28 } },
    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 0, 0, 0, 5 }, { 0, 28 } },
    { __LINE__, DT_4, "ABCabc\n123g", 28, -12, { 0, 0, 0, 6 }, { 0, 28 } },

    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 0, 0, 0, -2 }, { 0, 24 } },
    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 0, 0, 0, -1 }, { 0, 24 } },
    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 0, 0, 0, 0 }, { 0, 24 } },
    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 0, 0, 0, 1 }, { 0, 24 } },
    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 0, 0, 0, 2 }, { 0, 24 } },
    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 0, 0, 0, 3 }, { 0, 24 } },
    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 0, 0, 0, 4 }, { 0, 24 } },
    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 0, 0, 0, 5 }, { 0, 24 } },
    { __LINE__, DT_5, "ABCabc\n123g", 24, -10, { 0, 0, 0, 6 }, { 0, 24 } },

    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 0, 0, 0, -2 }, { 0, 26 } },
    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 0, 0, 0, -1 }, { 0, 26 } },
    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 0, 0, 0, 0 }, { 0, 26 } },
    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 0, 0, 0, 1 }, { 0, 26 } },
    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 0, 0, 0, 2 }, { 0, 26 } },
    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 0, 0, 0, 3 }, { 0, 26 } },
    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 0, 0, 0, 4 }, { 0, 26 } },
    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 0, 0, 0, 5 }, { 0, 26 } },
    { __LINE__, DT_5, "ABCabc\n123g", 26, -11, { 0, 0, 0, 6 }, { 0, 26 } },

    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 0, 0, 0, -2 }, { 0, 28 } },
    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 0, 0, 0, -1 }, { 0, 28 } },
    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 0, 0, 0, 0 }, { 0, 28 } },
    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 0, 0, 0, 1 }, { 0, 28 } },
    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 0, 0, 0, 2 }, { 0, 28 } },
    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 0, 0, 0, 3 }, { 0, 28 } },
    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 0, 0, 0, 4 }, { 0, 28 } },
    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 0, 0, 0, 5 }, { 0, 28 } },
    { __LINE__, DT_5, "ABCabc\n123g", 28, -12, { 0, 0, 0, 6 }, { 0, 28 } },

    { __LINE__, DT_6, "ABCabc123g", 10, -8, { 0, 1, 30, 15 - 2 }, { 1, 12 } },
    { __LINE__, DT_6, "ABCabc123g", 10, -8, { 0, 1, 30, 15 - 1 }, { 1, 13 } },
    { __LINE__, DT_6, "ABCabc123g", 10, -8, { 0, 1, 30, 15 + 0 }, { 1, 14 } },
    { __LINE__, DT_6, "ABCabc123g", 10, -8, { 0, 1, 30, 15 + 1 }, { 1, 15 } },
    { __LINE__, DT_6, "ABCabc123g", 10, -8, { 0, 1, 30, 15 + 2 }, { 1, 16 } },
    { __LINE__, DT_6, "ABCabc123g", 10, -8, { 0, 1, 30, 15 + 3 }, { 1, 17 } },
    { __LINE__, DT_6, "ABCabc123g", 10, -8, { 0, 1, 30, 15 + 4 }, { 1, 18 } },
    { __LINE__, DT_6, "ABCabc123g", 10, -8, { 0, 1, 30, 15 + 5 }, { 1, 19 } },
    { __LINE__, DT_6, "ABCabc123g", 10, -8, { 0, 1, 30, 15 + 6 }, { 1, 20 } },

    { __LINE__, DT_6, "ABCabc123g", 11, -9, { 0, 1, 30, 15 - 2 }, { 1, 12 } },
    { __LINE__, DT_6, "ABCabc123g", 11, -9, { 0, 1, 30, 15 - 1 }, { 1, 13 } },
    { __LINE__, DT_6, "ABCabc123g", 11, -9, { 0, 1, 30, 15 + 0 }, { 1, 14 } },
    { __LINE__, DT_6, "ABCabc123g", 11, -9, { 0, 1, 30, 15 + 1 }, { 1, 15 } },
    { __LINE__, DT_6, "ABCabc123g", 11, -9, { 0, 1, 30, 15 + 2 }, { 1, 16 } },
    { __LINE__, DT_6, "ABCabc123g", 11, -9, { 0, 1, 30, 15 + 3 }, { 1, 17 } },
    { __LINE__, DT_6, "ABCabc123g", 11, -9, { 0, 1, 30, 15 + 4 }, { 1, 18 } },
    { __LINE__, DT_6, "ABCabc123g", 11, -9, { 0, 1, 30, 15 + 5 }, { 1, 19 } },
    { __LINE__, DT_6, "ABCabc123g", 11, -9, { 0, 1, 30, 15 + 6 }, { 1, 20 } },

    { __LINE__, DT_6, "ABCabc123g", 12, -10, { 0, 1, 30, 15 - 2 }, { 1, 12 } },
    { __LINE__, DT_6, "ABCabc123g", 12, -10, { 0, 1, 30, 15 - 1 }, { 1, 13 } },
    { __LINE__, DT_6, "ABCabc123g", 12, -10, { 0, 1, 30, 15 + 0 }, { 1, 14 } },
    { __LINE__, DT_6, "ABCabc123g", 12, -10, { 0, 1, 30, 15 + 1 }, { 1, 15 } },
    { __LINE__, DT_6, "ABCabc123g", 12, -10, { 0, 1, 30, 15 + 2 }, { 1, 16 } },
    { __LINE__, DT_6, "ABCabc123g", 12, -10, { 0, 1, 30, 15 + 3 }, { 1, 17 } },
    { __LINE__, DT_6, "ABCabc123g", 12, -10, { 0, 1, 30, 15 + 4 }, { 1, 18 } },
    { __LINE__, DT_6, "ABCabc123g", 12, -10, { 0, 1, 30, 15 + 5 }, { 1, 19 } },
    { __LINE__, DT_6, "ABCabc123g", 12, -10, { 0, 1, 30, 15 + 6 }, { 1, 20 } },

    { __LINE__, DT_7, "ABCabc123g", 11, -8, { 0, 1, 30, 15 - 2 }, { 1, 12 } },
    { __LINE__, DT_7, "ABCabc123g", 11, -8, { 0, 1, 30, 15 - 1 }, { 1, 13 } },
    { __LINE__, DT_7, "ABCabc123g", 12, -8, { 0, 1, 30, 15 + 0 }, { 1, 14 } },
    { __LINE__, DT_7, "ABCabc123g", 12, -8, { 0, 1, 30, 15 + 1 }, { 1, 15 } },
    { __LINE__, DT_7, "ABCabc123g", 13, -8, { 0, 1, 30, 15 + 2 }, { 1, 16 } },
    { __LINE__, DT_7, "ABCabc123g", 13, -8, { 0, 1, 30, 15 + 3 }, { 1, 17 } },
    { __LINE__, DT_7, "ABCabc123g", 14, -8, { 0, 1, 30, 15 + 4 }, { 1, 18 } },
    { __LINE__, DT_7, "ABCabc123g", 14, -8, { 0, 1, 30, 15 + 5 }, { 1, 19 } },
    { __LINE__, DT_7, "ABCabc123g", 15, -8, { 0, 1, 30, 15 + 6 }, { 1, 20 } },

    { __LINE__, DT_7, "ABCabc123g", 11, -9, { 0, 1, 30, 15 - 2 }, { 1, 12 } },
    { __LINE__, DT_7, "ABCabc123g", 12, -9, { 0, 1, 30, 15 - 1 }, { 1, 13 } },
    { __LINE__, DT_7, "ABCabc123g", 12, -9, { 0, 1, 30, 15 + 0 }, { 1, 14 } },
    { __LINE__, DT_7, "ABCabc123g", 13, -9, { 0, 1, 30, 15 + 1 }, { 1, 15 } },
    { __LINE__, DT_7, "ABCabc123g", 13, -9, { 0, 1, 30, 15 + 2 }, { 1, 16 } },
    { __LINE__, DT_7, "ABCabc123g", 14, -9, { 0, 1, 30, 15 + 3 }, { 1, 17 } },
    { __LINE__, DT_7, "ABCabc123g", 14, -9, { 0, 1, 30, 15 + 4 }, { 1, 18 } },
    { __LINE__, DT_7, "ABCabc123g", 15, -9, { 0, 1, 30, 15 + 5 }, { 1, 19 } },
    { __LINE__, DT_7, "ABCabc123g", 15, -9, { 0, 1, 30, 15 + 6 }, { 1, 20 } },

    { __LINE__, DT_7, "ABCabc123g", 12, -10, { 0, 1, 30, 15 - 2 }, { 1, 12 } },
    { __LINE__, DT_7, "ABCabc123g", 12, -10, { 0, 1, 30, 15 - 1 }, { 1, 13 } },
    { __LINE__, DT_7, "ABCabc123g", 13, -10, { 0, 1, 30, 15 + 0 }, { 1, 14 } },
    { __LINE__, DT_7, "ABCabc123g", 13, -10, { 0, 1, 30, 15 + 1 }, { 1, 15 } },
    { __LINE__, DT_7, "ABCabc123g", 14, -10, { 0, 1, 30, 15 + 2 }, { 1, 16 } },
    { __LINE__, DT_7, "ABCabc123g", 14, -10, { 0, 1, 30, 15 + 3 }, { 1, 17 } },
    { __LINE__, DT_7, "ABCabc123g", 15, -10, { 0, 1, 30, 15 + 4 }, { 1, 18 } },
    { __LINE__, DT_7, "ABCabc123g", 15, -10, { 0, 1, 30, 15 + 5 }, { 1, 19 } },
    { __LINE__, DT_7, "ABCabc123g", 16, -10, { 0, 1, 30, 15 + 6 }, { 1, 20 } },

    { __LINE__, DT_8, "ABCabc\n123g", 20, -8, { 0, 1, 30, 15 - 2 }, { 1, 12 } },
    { __LINE__, DT_8, "ABCabc\n123g", 20, -8, { 0, 1, 30, 15 - 1 }, { 1, 12 } },
    { __LINE__, DT_8, "ABCabc\n123g", 20, -8, { 0, 1, 30, 15 + 0 }, { 1, 12 } },
    { __LINE__, DT_8, "ABCabc\n123g", 20, -8, { 0, 1, 30, 15 + 1 }, { 1, 12 } },
    { __LINE__, DT_8, "ABCabc\n123g", 20, -8, { 0, 1, 30, 15 + 2 }, { 1, 12 } },
    { __LINE__, DT_8, "ABCabc\n123g", 20, -8, { 0, 1, 30, 15 + 3 }, { 1, 12 } },
    { __LINE__, DT_8, "ABCabc\n123g", 20, -8, { 0, 1, 30, 15 + 4 }, { 1, 12 } },
    { __LINE__, DT_8, "ABCabc\n123g", 20, -8, { 0, 1, 30, 15 + 5 }, { 1, 12 } },
    { __LINE__, DT_8, "ABCabc\n123g", 20, -8, { 0, 1, 30, 15 + 6 }, { 1, 12 } },

    { __LINE__, DT_8, "ABCabc\n123g", 22, -9, { 0, 1, 30, 15 - 2 }, { 1, 12 } },
    { __LINE__, DT_8, "ABCabc\n123g", 22, -9, { 0, 1, 30, 15 - 1 }, { 1, 13 } },
    { __LINE__, DT_8, "ABCabc\n123g", 22, -9, { 0, 1, 30, 15 + 0 }, { 1, 13 } },
    { __LINE__, DT_8, "ABCabc\n123g", 22, -9, { 0, 1, 30, 15 + 1 }, { 1, 13 } },
    { __LINE__, DT_8, "ABCabc\n123g", 22, -9, { 0, 1, 30, 15 + 2 }, { 1, 13 } },
    { __LINE__, DT_8, "ABCabc\n123g", 22, -9, { 0, 1, 30, 15 + 3 }, { 1, 13 } },
    { __LINE__, DT_8, "ABCabc\n123g", 22, -9, { 0, 1, 30, 15 + 4 }, { 1, 13 } },
    { __LINE__, DT_8, "ABCabc\n123g", 22, -9, { 0, 1, 30, 15 + 5 }, { 1, 13 } },
    { __LINE__, DT_8, "ABCabc\n123g", 22, -9, { 0, 1, 30, 15 + 6 }, { 1, 13 } },

    { __LINE__, DT_8, "ABCabc\n123g", 24, -10, { 0, 1, 30, 15 - 2 }, { 1, 12 } },
    { __LINE__, DT_8, "ABCabc\n123g", 24, -10, { 0, 1, 30, 15 - 1 }, { 1, 13 } },
    { __LINE__, DT_8, "ABCabc\n123g", 24, -10, { 0, 1, 30, 15 + 0 }, { 1, 14 } },
    { __LINE__, DT_8, "ABCabc\n123g", 24, -10, { 0, 1, 30, 15 + 1 }, { 1, 15 } },
    { __LINE__, DT_8, "ABCabc\n123g", 24, -10, { 0, 1, 30, 15 + 2 }, { 1, 15 } },
    { __LINE__, DT_8, "ABCabc\n123g", 24, -10, { 0, 1, 30, 15 + 3 }, { 1, 15 } },
    { __LINE__, DT_8, "ABCabc\n123g", 24, -10, { 0, 1, 30, 15 + 4 }, { 1, 15 } },
    { __LINE__, DT_8, "ABCabc\n123g", 24, -10, { 0, 1, 30, 15 + 5 }, { 1, 15 } },
    { __LINE__, DT_8, "ABCabc\n123g", 24, -10, { 0, 1, 30, 15 + 6 }, { 1, 15 } },

    { __LINE__, DT_9, "ABCabc\n123g", 20, -8, { 0, 1, 30, 15 - 2 }, { 1, 12 } },
    { __LINE__, DT_9, "ABCabc\n123g", 20, -8, { 0, 1, 30, 15 - 1 }, { 1, 12 } },
    { __LINE__, DT_9, "ABCabc\n123g", 20, -8, { 0, 1, 30, 15 + 0 }, { 1, 12 } },
    { __LINE__, DT_9, "ABCabc\n123g", 20, -8, { 0, 1, 30, 15 + 1 }, { 1, 12 } },
    { __LINE__, DT_9, "ABCabc\n123g", 20, -8, { 0, 1, 30, 15 + 2 }, { 1, 12 } },
    { __LINE__, DT_9, "ABCabc\n123g", 20, -8, { 0, 1, 30, 15 + 3 }, { 1, 12 } },
    { __LINE__, DT_9, "ABCabc\n123g", 20, -8, { 0, 1, 30, 15 + 4 }, { 1, 12 } },
    { __LINE__, DT_9, "ABCabc\n123g", 20, -8, { 0, 1, 30, 15 + 5 }, { 1, 12 } },
    { __LINE__, DT_9, "ABCabc\n123g", 20, -8, { 0, 1, 30, 15 + 6 }, { 1, 12 } },

    { __LINE__, DT_9, "ABCabc\n123g", 22, -9, { 0, 1, 30, 15 - 2 }, { 1, 12 } },
    { __LINE__, DT_9, "ABCabc\n123g", 22, -9, { 0, 1, 30, 15 - 1 }, { 1, 13 } },
    { __LINE__, DT_9, "ABCabc\n123g", 22, -9, { 0, 1, 30, 15 + 0 }, { 1, 13 } },
    { __LINE__, DT_9, "ABCabc\n123g", 22, -9, { 0, 1, 30, 15 + 1 }, { 1, 13 } },
    { __LINE__, DT_9, "ABCabc\n123g", 22, -9, { 0, 1, 30, 15 + 2 }, { 1, 13 } },
    { __LINE__, DT_9, "ABCabc\n123g", 22, -9, { 0, 1, 30, 15 + 3 }, { 1, 13 } },
    { __LINE__, DT_9, "ABCabc\n123g", 22, -9, { 0, 1, 30, 15 + 4 }, { 1, 13 } },
    { __LINE__, DT_9, "ABCabc\n123g", 22, -9, { 0, 1, 30, 15 + 5 }, { 1, 13 } },
    { __LINE__, DT_9, "ABCabc\n123g", 22, -9, { 0, 1, 30, 15 + 6 }, { 1, 13 } },

    { __LINE__, DT_9, "ABCabc\n123g", 24, -10, { 0, 1, 30, 15 - 2 }, { 1, 12 } },
    { __LINE__, DT_9, "ABCabc\n123g", 24, -10, { 0, 1, 30, 15 - 1 }, { 1, 13 } },
    { __LINE__, DT_9, "ABCabc\n123g", 24, -10, { 0, 1, 30, 15 + 0 }, { 1, 14 } },
    { __LINE__, DT_9, "ABCabc\n123g", 24, -10, { 0, 1, 30, 15 + 1 }, { 1, 15 } },
    { __LINE__, DT_9, "ABCabc\n123g", 24, -10, { 0, 1, 30, 15 + 2 }, { 1, 15 } },
    { __LINE__, DT_9, "ABCabc\n123g", 24, -10, { 0, 1, 30, 15 + 3 }, { 1, 15 } },
    { __LINE__, DT_9, "ABCabc\n123g", 24, -10, { 0, 1, 30, 15 + 4 }, { 1, 15 } },
    { __LINE__, DT_9, "ABCabc\n123g", 24, -10, { 0, 1, 30, 15 + 5 }, { 1, 15 } },
    { __LINE__, DT_9, "ABCabc\n123g", 24, -10, { 0, 1, 30, 15 + 6 }, { 1, 15 } },

    { __LINE__, DT_10, "ABCabc\n123g", 10, -8, { 0, 1, 30, 15 - 2 }, { 1, 12 } },
    { __LINE__, DT_10, "ABCabc\n123g", 10, -8, { 0, 1, 30, 15 - 1 }, { 1, 13 } },
    { __LINE__, DT_10, "ABCabc\n123g", 10, -8, { 0, 1, 30, 15 + 0 }, { 1, 14 } },
    { __LINE__, DT_10, "ABCabc\n123g", 10, -8, { 0, 1, 30, 15 + 1 }, { 1, 15 } },
    { __LINE__, DT_10, "ABCabc\n123g", 10, -8, { 0, 1, 30, 15 + 2 }, { 1, 16 } },
    { __LINE__, DT_10, "ABCabc\n123g", 10, -8, { 0, 1, 30, 15 + 3 }, { 1, 17 } },
    { __LINE__, DT_10, "ABCabc\n123g", 10, -8, { 0, 1, 30, 15 + 4 }, { 1, 18 } },
    { __LINE__, DT_10, "ABCabc\n123g", 10, -8, { 0, 1, 30, 15 + 5 }, { 1, 19 } },
    { __LINE__, DT_10, "ABCabc\n123g", 20, -8, { 0, 1, 30, 15 + 6 }, { 1, 12 } },

    { __LINE__, DT_10, "ABCabc\n123g", 11, -9, { 0, 1, 30, 15 - 2 }, { 1, 12 } },
    { __LINE__, DT_10, "ABCabc\n123g", 11, -9, { 0, 1, 30, 15 - 1 }, { 1, 13 } },
    { __LINE__, DT_10, "ABCabc\n123g", 11, -9, { 0, 1, 30, 15 + 0 }, { 1, 14 } },
    { __LINE__, DT_10, "ABCabc\n123g", 11, -9, { 0, 1, 30, 15 + 1 }, { 1, 15 } },
    { __LINE__, DT_10, "ABCabc\n123g", 11, -9, { 0, 1, 30, 15 + 2 }, { 1, 16 } },
    { __LINE__, DT_10, "ABCabc\n123g", 11, -9, { 0, 1, 30, 15 + 3 }, { 1, 17 } },
    { __LINE__, DT_10, "ABCabc\n123g", 11, -9, { 0, 1, 30, 15 + 4 }, { 1, 18 } },
    { __LINE__, DT_10, "ABCabc\n123g", 11, -9, { 0, 1, 30, 15 + 5 }, { 1, 19 } },
    { __LINE__, DT_10, "ABCabc\n123g", 11, -9, { 0, 1, 30, 15 + 6 }, { 1, 20 } },

    { __LINE__, DT_10, "ABCabc\n123g", 12, -10, { 0, 1, 30, 15 - 2 }, { 1, 12 } },
    { __LINE__, DT_10, "ABCabc\n123g", 12, -10, { 0, 1, 30, 15 - 1 }, { 1, 13 } },
    { __LINE__, DT_10, "ABCabc\n123g", 12, -10, { 0, 1, 30, 15 + 0 }, { 1, 14 } },
    { __LINE__, DT_10, "ABCabc\n123g", 12, -10, { 0, 1, 30, 15 + 1 }, { 1, 15 } },
    { __LINE__, DT_10, "ABCabc\n123g", 12, -10, { 0, 1, 30, 15 + 2 }, { 1, 16 } },
    { __LINE__, DT_10, "ABCabc\n123g", 12, -10, { 0, 1, 30, 15 + 3 }, { 1, 17 } },
    { __LINE__, DT_10, "ABCabc\n123g", 12, -10, { 0, 1, 30, 15 + 4 }, { 1, 18 } },
    { __LINE__, DT_10, "ABCabc\n123g", 12, -10, { 0, 1, 30, 15 + 5 }, { 1, 19 } },
    { __LINE__, DT_10, "ABCabc\n123g", 12, -10, { 0, 1, 30, 15 + 6 }, { 1, 20 } },
};

static void DoEntry(HDC hdc, const TEST_ENTRY *pEntry)
{
    RECT rc;
    INT ret;
    HFONT hFont;
    HGDIOBJ hFontOld;
    LONG x, y, yMin, yMax;
    LOGFONTW lf = { pEntry->font_height };
    lstrcpyW(lf.lfFaceName, L"Tahoma");

    hFont = CreateFontIndirectW(&lf);
    ok(hFont != NULL, "hFont is NULL\n");

    hFontOld = SelectObject(hdc, hFont);
    {
        rc = pEntry->input;

        if (!(pEntry->uFormat & DT_CALCRECT))
            FillRect(hdc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));

        ret = DrawTextA(hdc, pEntry->pszText, -1, &rc, pEntry->uFormat);
        ok(ret == pEntry->ret,
           "Line %d: ret %d vs %d\n", pEntry->line, ret, pEntry->ret);

        if (!(pEntry->uFormat & DT_CALCRECT))
        {
            yMin = rc.bottom;
            yMax = rc.top;
            for (y = rc.top; y < rc.bottom; ++y)
            {
                BOOL bNonWhiteFound = FALSE;
                for (x = rc.left; x < rc.right; ++x)
                {
                    if (GetPixel(hdc, x, y) != RGB(255, 255, 255))
                    {
                        bNonWhiteFound = TRUE;
                        break;
                    }
                }
                if (!bNonWhiteFound)
                {
                    if (yMin > y)
                        yMin = y;
                    if (yMax < y)
                        yMax = y;
                }
            }
            rc.top = yMin;
            rc.bottom = yMax;
        }

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
    HGDIOBJ hbmOld;
    HDC hdc = CreateCompatibleDC(NULL);
    ok(hdc != NULL, "hdc was NULL\n");

    hbm = CreateCompatibleBitmap(hdc, 100, 100);
    hbmOld = SelectObject(hdc, hbm);

    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkColor(hdc, RGB(255, 255, 255));
    SetBkMode(hdc, OPAQUE);
    for (i = 0; i < ARRAYSIZE(s_entries); ++i)
    {
        DoEntry(hdc, &s_entries[i]);
    }

    SelectObject(hdc, hbmOld);
    DeleteObject(hbm);
    DeleteDC(hdc);
}

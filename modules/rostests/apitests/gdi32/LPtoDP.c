/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for LPtoDP
 * PROGRAMMERS:     Katayama Hirofumi MZ
 */

#include "precomp.h"

#define INVALID_POINTER ((PVOID)(ULONG_PTR)0xdeadbeefdeadbeefULL)
#define NO_CHECK 0xFACECAFE
#define CALC_VALUE_1 0xBADACE01
#define CALC_VALUE_2 0xFEEDF00D
#define CALC_VALUE_3 0xDEADCAD1
#define DISPLAY_SIZE 0xDEADBEEF
#define NEGA_DISPLAY_SIZE 0xBEEFDEAD

typedef struct PRESET
{
    LONG xWnd;
    LONG yWnd;
    LONG cxWnd;
    LONG cyWnd;

    LONG xView;
    LONG yView;
    LONG cxView;
    LONG cyView;
} PRESET;

typedef struct TEST_ENTRY
{
    INT lineno;
    BOOL ret;
    DWORD error;

    BOOL bWndExt;
    INT nMapMode;

    PRESET preset;

    LONG xWndOut;
    LONG yWndOut;
    LONG cxWndOut;
    LONG cyWndOut;

    LONG xViewOut;
    LONG yViewOut;
    LONG cxViewOut;
    LONG cyViewOut;

    POINT ptSrc;
    POINT ptDest;
} TEST_ENTRY;

#define PRESET0     { 0,  0,   0,   0,   0,   0,  0,    0   }
#define PRESET1     { 0,  0,   1,   1,   0,   0,  1,    1   }
#define PRESET2     { 75, 0,   1,   1,   0,   0,  10,   10  }
#define PRESET3     { 0,  180, 1,   1,   120, 0,  1,    1   }
#define PRESET4     { 0,  0,   200, 1,   50,  0,  1,    1   }
#define PRESET5     { 0,  0,   1,   200, 0,   0,  -100, 1   }
#define PRESET6     { 50, 0,   1,   200, 10,  0,  100,  200 }

static const TEST_ENTRY s_entries[] =
{
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TEXT, PRESET0, 0, 0, 1, 1, 0, 0, 1, 1, { 100, 150 }, { 100, 150 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TEXT, PRESET1, 0, 0, 1, 1, 0, 0, 1, 1, { 100, 150 }, { 100, 150 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TEXT, PRESET2, 75, 0, 1, 1, 0, 0, 1, 1, { 100, 150 }, { 25, 150 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TEXT, PRESET3, 0, 180, 1, 1, 120, 0, 1, 1, { 100, 150 }, { 220, -30 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TEXT, PRESET4, 0, 0, 1, 1, 50, 0, 1, 1, { 100, 150 }, { 150, 150 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TEXT, PRESET5, 0, 0, 1, 1, 0, 0, 1, 1, { 100, 150 }, { 100, 150 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TEXT, PRESET6, 50, 0, 1, 1, 10, 0, 1, 1, { 100, 150 }, { 60, 150 } },

    { __LINE__, TRUE, 0xDEADBEEF, FALSE, MM_ANISOTROPIC, PRESET0, 0, 0, NO_CHECK, NO_CHECK, 0, 0, NO_CHECK, NO_CHECK, { 100, 150 }, { NO_CHECK, NO_CHECK } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ANISOTROPIC, PRESET1, 0, 0, 1, 1, 0, 0, 1, 1, { 100, 150 }, { 100, 150 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ANISOTROPIC, PRESET2, 75, 0, 1, 1, 0, 0, 10, 10, { 100, 150 }, { 250, 1500 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ANISOTROPIC, PRESET3, 0, 180, 1, 1, 120, 0, 1, 1, { 100, 150 }, { 220, -30 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ANISOTROPIC, PRESET4, 0, 0, 200, 1, 50, 0, 1, 1, { 100, 150 }, { 51, 150 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ANISOTROPIC, PRESET5, 0, 0, 1, 200, 0, 0, -100, 1, { 100, 150 }, { -10000, 1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ANISOTROPIC, PRESET6, 50, 0, 1, 200, 10, 0, 100, 200, { 100, 150 }, { 5010, 150 } },

    { __LINE__, TRUE, 0xDEADBEEF, FALSE, MM_ISOTROPIC, PRESET0, 0, 0, NO_CHECK, NO_CHECK, 0, 0, NO_CHECK, NO_CHECK, { 100, 150 }, { NO_CHECK, NO_CHECK } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ISOTROPIC, PRESET1, 0, 0, 1, 1, 0, 0, 1, 1, { 100, 150 }, { 100, 150 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ISOTROPIC, PRESET2, 75, 0, 1, 1, 0, 0, 10, 10, { 100, 150 }, { 250, 1500 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ISOTROPIC, PRESET3, 0, 180, 1, 1, 120, 0, 1, 1, { 100, 150 }, { 220, -30 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ISOTROPIC, PRESET4, 0, 0, 200, 1, 50, 0, 1, 0, { 100, 150 }, { 51, 0 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ISOTROPIC, PRESET5, 0, 0, 1, 200, 0, 0, 0, 1, { 100, 150 }, { 0, 0 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ISOTROPIC, PRESET6, 50, 0, 1, 200, 10, 0, 1, 200, { 100, 150 }, { 60, 150 } },

    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOMETRIC, PRESET0, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOMETRIC, PRESET1, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOMETRIC, PRESET2, 75, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOMETRIC, PRESET3, 0, 180, NO_CHECK, NO_CHECK, 120, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOMETRIC, PRESET4, 0, 0, NO_CHECK, NO_CHECK, 50, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOMETRIC, PRESET5, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOMETRIC, PRESET6, 50, 0, NO_CHECK, NO_CHECK, 10, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },

    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIMETRIC, PRESET0, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIMETRIC, PRESET1, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIMETRIC, PRESET2, 75, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIMETRIC, PRESET3, 0, 180, NO_CHECK, NO_CHECK, 120, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIMETRIC, PRESET4, 0, 0, NO_CHECK, NO_CHECK, 50, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIMETRIC, PRESET5, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIMETRIC, PRESET6, 50, 0, NO_CHECK, NO_CHECK, 10, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },

    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOENGLISH, PRESET0, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOENGLISH, PRESET1, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOENGLISH, PRESET2, 75, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOENGLISH, PRESET3, 0, 180, NO_CHECK, NO_CHECK, 120, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOENGLISH, PRESET4, 0, 0, NO_CHECK, NO_CHECK, 50, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOENGLISH, PRESET5, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOENGLISH, PRESET6, 50, 0, NO_CHECK, NO_CHECK, 10, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },

    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIENGLISH, PRESET0, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIENGLISH, PRESET1, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIENGLISH, PRESET2, 75, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIENGLISH, PRESET3, 0, 180, NO_CHECK, NO_CHECK, 120, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIENGLISH, PRESET4, 0, 0, NO_CHECK, NO_CHECK, 50, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIENGLISH, PRESET5, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIENGLISH, PRESET6, 50, 0, NO_CHECK, NO_CHECK, 10, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },

    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TWIPS, PRESET0, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TWIPS, PRESET1, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TWIPS, PRESET2, 75, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TWIPS, PRESET3, 0, 180, NO_CHECK, NO_CHECK, 120, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TWIPS, PRESET4, 0, 0, NO_CHECK, NO_CHECK, 50, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TWIPS, PRESET5, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TWIPS, PRESET6, 50, 0, NO_CHECK, NO_CHECK, 10, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_1 , CALC_VALUE_1 } },
};
static const size_t s_entries_count = _countof(s_entries);

static const TEST_ENTRY s_shifted_entries[] =
{
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TEXT, PRESET0, 0, 0, 1, 1, 0, 0, 1, 1, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TEXT, PRESET1, 0, 0, 1, 1, 0, 0, 1, 1, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TEXT, PRESET2, 75, 0, 1, 1, 0, 0, 1, 1, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TEXT, PRESET3, 0, 180, 1, 1, 120, 0, 1, 1, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TEXT, PRESET4, 0, 0, 1, 1, 50, 0, 1, 1, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TEXT, PRESET5, 0, 0, 1, 1, 0, 0, 1, 1, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TEXT, PRESET6, 50, 0, 1, 1, 10, 0, 1, 1, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },

    { __LINE__, TRUE, 0xDEADBEEF, FALSE, MM_ANISOTROPIC, PRESET0, 0, 0, NO_CHECK, NO_CHECK, 0, 0, NO_CHECK, NO_CHECK, { 100, 150 }, { NO_CHECK, NO_CHECK } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ANISOTROPIC, PRESET1, 0, 0, 1, 1, 0, 0, 1, 1, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ANISOTROPIC, PRESET2, 75, 0, 1, 1, 0, 0, 10, 10, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ANISOTROPIC, PRESET3, 0, 180, 1, 1, 120, 0, 1, 1, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ANISOTROPIC, PRESET4, 0, 0, 200, 1, 50, 0, 1, 1, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ANISOTROPIC, PRESET5, 0, 0, 1, 200, 0, 0, -100, 1, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ANISOTROPIC, PRESET6, 50, 0, 1, 200, 10, 0, 100, 200, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },

    { __LINE__, TRUE, 0xDEADBEEF, FALSE, MM_ISOTROPIC, PRESET0, 0, 0, NO_CHECK, NO_CHECK, 0, 0, NO_CHECK, NO_CHECK, { 100, 150 }, { NO_CHECK, NO_CHECK } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ISOTROPIC, PRESET1, 0, 0, 1, 1, 0, 0, 1, 1, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ISOTROPIC, PRESET2, 75, 0, 1, 1, 0, 0, 10, 10, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ISOTROPIC, PRESET3, 0, 180, 1, 1, 120, 0, 1, 1, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ISOTROPIC, PRESET4, 0, 0, 200, 1, 50, 0, 1, 0, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ISOTROPIC, PRESET5, 0, 0, 1, 200, 0, 0, 0, 1, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ISOTROPIC, PRESET6, 50, 0, 1, 200, 10, 0, 1, 200, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },

    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOMETRIC, PRESET0, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOMETRIC, PRESET1, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOMETRIC, PRESET2, 75, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOMETRIC, PRESET3, 0, 180, NO_CHECK, NO_CHECK, 120, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOMETRIC, PRESET4, 0, 0, NO_CHECK, NO_CHECK, 50, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOMETRIC, PRESET5, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOMETRIC, PRESET6, 50, 0, NO_CHECK, NO_CHECK, 10, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },

    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIMETRIC, PRESET0, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIMETRIC, PRESET1, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIMETRIC, PRESET2, 75, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIMETRIC, PRESET3, 0, 180, NO_CHECK, NO_CHECK, 120, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIMETRIC, PRESET4, 0, 0, NO_CHECK, NO_CHECK, 50, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIMETRIC, PRESET5, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIMETRIC, PRESET6, 50, 0, NO_CHECK, NO_CHECK, 10, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },

    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOENGLISH, PRESET0, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOENGLISH, PRESET1, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOENGLISH, PRESET2, 75, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOENGLISH, PRESET3, 0, 180, NO_CHECK, NO_CHECK, 120, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOENGLISH, PRESET4, 0, 0, NO_CHECK, NO_CHECK, 50, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOENGLISH, PRESET5, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOENGLISH, PRESET6, 50, 0, NO_CHECK, NO_CHECK, 10, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },

    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIENGLISH, PRESET0, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIENGLISH, PRESET1, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIENGLISH, PRESET2, 75, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIENGLISH, PRESET3, 0, 180, NO_CHECK, NO_CHECK, 120, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIENGLISH, PRESET4, 0, 0, NO_CHECK, NO_CHECK, 50, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIENGLISH, PRESET5, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIENGLISH, PRESET6, 50, 0, NO_CHECK, NO_CHECK, 10, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },

    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TWIPS, PRESET0, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TWIPS, PRESET1, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TWIPS, PRESET2, 75, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TWIPS, PRESET3, 0, 180, NO_CHECK, NO_CHECK, 120, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TWIPS, PRESET4, 0, 0, NO_CHECK, NO_CHECK, 50, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TWIPS, PRESET5, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TWIPS, PRESET6, 50, 0, NO_CHECK, NO_CHECK, 10, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_2, CALC_VALUE_2 } },
};
static const size_t s_shifted_entries_count = _countof(s_shifted_entries);

static const TEST_ENTRY s_transformed_entries[] =
{
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TEXT, PRESET0, 0, 0, 1, 1, 0, 0, 1, 1, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TEXT, PRESET1, 0, 0, 1, 1, 0, 0, 1, 1, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TEXT, PRESET2, 75, 0, 1, 1, 0, 0, 1, 1, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TEXT, PRESET3, 0, 180, 1, 1, 120, 0, 1, 1, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TEXT, PRESET4, 0, 0, 1, 1, 50, 0, 1, 1, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TEXT, PRESET5, 0, 0, 1, 1, 0, 0, 1, 1, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TEXT, PRESET6, 50, 0, 1, 1, 10, 0, 1, 1, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },

    { __LINE__, TRUE, 0xDEADBEEF, FALSE, MM_ANISOTROPIC, PRESET0, 0, 0, NO_CHECK, NO_CHECK, 0, 0, NO_CHECK, NO_CHECK, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ANISOTROPIC, PRESET1, 0, 0, 1, 1, 0, 0, 1, 1, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ANISOTROPIC, PRESET2, 75, 0, 1, 1, 0, 0, 10, 10, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ANISOTROPIC, PRESET3, 0, 180, 1, 1, 120, 0, 1, 1, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ANISOTROPIC, PRESET4, 0, 0, 200, 1, 50, 0, 1, 1, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ANISOTROPIC, PRESET5, 0, 0, 1, 200, 0, 0, -100, 1, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ANISOTROPIC, PRESET6, 50, 0, 1, 200, 10, 0, 100, 200, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },

    { __LINE__, TRUE, 0xDEADBEEF, FALSE, MM_ISOTROPIC, PRESET0, 0, 0, NO_CHECK, NO_CHECK, 0, 0, NO_CHECK, NO_CHECK, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ISOTROPIC, PRESET1, 0, 0, 1, 1, 0, 0, 1, 1, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ISOTROPIC, PRESET2, 75, 0, 1, 1, 0, 0, 10, 10, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ISOTROPIC, PRESET3, 0, 180, 1, 1, 120, 0, 1, 1, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ISOTROPIC, PRESET4, 0, 0, 200, 1, 50, 0, 1, 0, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ISOTROPIC, PRESET5, 0, 0, 1, 200, 0, 0, 0, 1, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_ISOTROPIC, PRESET6, 50, 0, 1, 200, 10, 0, 1, 200, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },

    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOMETRIC, PRESET0, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOMETRIC, PRESET1, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOMETRIC, PRESET2, 75, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOMETRIC, PRESET3, 0, 180, NO_CHECK, NO_CHECK, 120, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOMETRIC, PRESET4, 0, 0, NO_CHECK, NO_CHECK, 50, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOMETRIC, PRESET5, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOMETRIC, PRESET6, 50, 0, NO_CHECK, NO_CHECK, 10, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },

    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIMETRIC, PRESET0, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIMETRIC, PRESET1, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIMETRIC, PRESET2, 75, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIMETRIC, PRESET3, 0, 180, NO_CHECK, NO_CHECK, 120, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIMETRIC, PRESET4, 0, 0, NO_CHECK, NO_CHECK, 50, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIMETRIC, PRESET5, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIMETRIC, PRESET6, 50, 0, NO_CHECK, NO_CHECK, 10, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },

    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOENGLISH, PRESET0, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOENGLISH, PRESET1, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOENGLISH, PRESET2, 75, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOENGLISH, PRESET3, 0, 180, NO_CHECK, NO_CHECK, 120, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOENGLISH, PRESET4, 0, 0, NO_CHECK, NO_CHECK, 50, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOENGLISH, PRESET5, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_LOENGLISH, PRESET6, 50, 0, NO_CHECK, NO_CHECK, 10, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },

    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIENGLISH, PRESET0, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIENGLISH, PRESET1, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIENGLISH, PRESET2, 75, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIENGLISH, PRESET3, 0, 180, NO_CHECK, NO_CHECK, 120, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIENGLISH, PRESET4, 0, 0, NO_CHECK, NO_CHECK, 50, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIENGLISH, PRESET5, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_HIENGLISH, PRESET6, 50, 0, NO_CHECK, NO_CHECK, 10, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },

    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TWIPS, PRESET0, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TWIPS, PRESET1, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TWIPS, PRESET2, 75, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TWIPS, PRESET3, 0, 180, NO_CHECK, NO_CHECK, 120, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TWIPS, PRESET4, 0, 0, NO_CHECK, NO_CHECK, 50, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TWIPS, PRESET5, 0, 0, NO_CHECK, NO_CHECK, 0, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
    { __LINE__, TRUE, 0xDEADBEEF, TRUE, MM_TWIPS, PRESET6, 50, 0, NO_CHECK, NO_CHECK, 10, 0, DISPLAY_SIZE, NEGA_DISPLAY_SIZE, { 100, 150 }, { CALC_VALUE_3, CALC_VALUE_3 } },
};
static const size_t s_transformed_entries_count = _countof(s_transformed_entries);

static void SetXForm1(XFORM *pxform)
{
    pxform->eM11 = 1;
    pxform->eM12 = 0;
    pxform->eM21 = 0;
    pxform->eM22 = 1;
    pxform->eDx = 314;
    pxform->eDy = -99;
}

static void SetXForm2(XFORM *pxform)
{
    pxform->eM11 = 2;
    pxform->eM12 = 0;
    pxform->eM21 = 0;
    pxform->eM22 = 3;
    pxform->eDx = 0;
    pxform->eDy = 0;
}

static void DoTestEntry(HDC hDC, const TEST_ENTRY *entry)
{
    POINT pt, ptWnd, ptView;
    SIZE siz, sizWnd, sizView;
    INT ret;

    SetMapMode(hDC, entry->nMapMode);

    ret = SetWindowOrgEx(hDC, entry->preset.xWnd, entry->preset.yWnd, NULL);
    ok(ret == TRUE, "Line %d: SetWindowOrgEx failed\n", entry->lineno);

    ret = SetWindowExtEx(hDC, entry->preset.cxWnd, entry->preset.cyWnd, NULL);
    ok(ret == entry->bWndExt, "Line %d: SetWindowExtEx() expected %d, was %d\n", entry->lineno, entry->bWndExt, ret);

    ret = SetViewportOrgEx(hDC, entry->preset.xView, entry->preset.yView, NULL);
    ok(ret == TRUE, "Line %d: SetViewportOrgEx failed\n", entry->lineno);

    ret = SetViewportExtEx(hDC, entry->preset.cxView, entry->preset.cyView, NULL);
    ok(ret == TRUE, "Line %d: SetViewportExtEx failed\n", entry->lineno);

    ok(GetWindowOrgEx(hDC, &pt) == TRUE, "Line %d: GetWindowOrgEx failed\n", entry->lineno);
    ptWnd = pt;
    ok(GetWindowExtEx(hDC, &siz) == TRUE, "Line %d: GetWindowExtEx failed\n", entry->lineno);
    sizWnd = siz;

    ok(pt.x == entry->xWndOut && pt.y == entry->yWndOut,
       "Line %d: Window org expected (%ld, %ld), was (%ld, %ld)\n",
        entry->lineno, entry->xWndOut, entry->yWndOut, pt.x, pt.y);

    if (entry->cxWndOut == DISPLAY_SIZE || entry->cxWndOut == NEGA_DISPLAY_SIZE)
    {
        LONG cx = GetDeviceCaps(hDC, HORZRES);
        LONG cy = GetDeviceCaps(hDC, VERTRES);
        if (entry->cxWndOut == NEGA_DISPLAY_SIZE)
            cx = -cx;
        if (entry->cyWndOut == NEGA_DISPLAY_SIZE)
            cy = -cy;
        ok(siz.cx == cx && siz.cy == cy,
           "Line %d: Window ext expected display size (%ld, %ld), was (%ld, %ld)\n",
            entry->lineno, cx, cy, siz.cx, siz.cy);
    }
    else if (entry->cxWndOut != NO_CHECK)
    {
        ok(siz.cx == entry->cxWndOut && siz.cy == entry->cyWndOut,
           "Line %d: Window ext expected (%ld, %ld), was (%ld, %ld)\n",
            entry->lineno, entry->cxWndOut, entry->cyWndOut, siz.cx, siz.cy);
    }

    ok(GetViewportOrgEx(hDC, &pt) == TRUE, "Line %d: GetViewportOrgEx failed\n", entry->lineno);
    ptView = pt;
    ok(GetViewportExtEx(hDC, &siz) == TRUE, "Line %d: GetViewportExtEx failed\n", entry->lineno);
    sizView = siz;

    ok(pt.x == entry->xViewOut && pt.y == entry->yViewOut,
       "Line %d: Viewport org expected (%ld, %ld), was (%ld, %ld)\n",
        entry->lineno, entry->xViewOut , entry->yViewOut, pt.x, pt.y);

    if (entry->cxViewOut == DISPLAY_SIZE || entry->cxViewOut == NEGA_DISPLAY_SIZE)
    {
        LONG cx = GetDeviceCaps(hDC, HORZRES);
        LONG cy = GetDeviceCaps(hDC, VERTRES);
        if (entry->cxViewOut == NEGA_DISPLAY_SIZE)
            cx = -cx;
        if (entry->cyViewOut == NEGA_DISPLAY_SIZE)
            cy = -cy;
        ok(siz.cx == cx && siz.cy == cy,
           "Line %d: Viewport ext expected display size (%ld, %ld), was (%ld, %ld)\n",
            entry->lineno, cx, cy, siz.cx, siz.cy);
    }
    else if (entry->cxViewOut != NO_CHECK)
    {
        ok(siz.cx == entry->cxViewOut && siz.cy == entry->cyViewOut,
           "Line %d: Viewport ext expected (%ld, %ld), was (%ld, %ld)\n",
            entry->lineno, entry->cxViewOut, entry->cyViewOut, siz.cx, siz.cy);
    }

    pt = entry->ptSrc;

    SetLastError(0xDEADBEEF);
    ret = LPtoDP(hDC, &pt, 1);
    ok(ret == entry->ret, "Line %d: LPtoDP() expected %d, was %d\n", entry->lineno, entry->ret, ret);

    ok(GetLastError() == entry->error, "Line %d: GetLastError() expected %ld, was %ld\n",
       entry->lineno, entry->error, GetLastError());

    if (entry->ptDest.x == CALC_VALUE_1)
    {
        LONG x = MulDiv(entry->ptSrc.x - ptWnd.x, sizView.cx, sizWnd.cx) + ptView.x;
        LONG y = MulDiv(entry->ptSrc.y - ptWnd.y, sizView.cy, sizWnd.cy) + ptView.y;
        // TODO: make more accurate
        ok(labs(pt.x - x) <= 1 && labs(pt.y - y) <= 1,
           "Line %d: Dest expected (%ld, %ld), was (%ld, %ld)\n",
           entry->lineno, x, y, pt.x, pt.y);
    }
    else if (entry->ptDest.x == CALC_VALUE_2)
    {
        XFORM xform;
        LONG x, y;
        SetXForm1(&xform);
        x = (LONG)((xform.eM11 * entry->ptSrc.x + xform.eM12 * entry->ptSrc.y) + xform.eDx);
        y = (LONG)((xform.eM21 * entry->ptSrc.x + xform.eM22 * entry->ptSrc.y) + xform.eDy);
        x = MulDiv(x - ptWnd.x, sizView.cx, sizWnd.cx) + ptView.x;
        y = MulDiv(y - ptWnd.y, sizView.cy, sizWnd.cy) + ptView.y;
        // TODO: make more accurate
        ok(labs(pt.x - x) <= 1 && labs(pt.y - y) <= 1,
           "Line %d: Dest expected (%ld, %ld), was (%ld, %ld)\n",
           entry->lineno, x, y, pt.x, pt.y);
    }
    else if (entry->ptDest.x == CALC_VALUE_3)
    {
        XFORM xform;
        LONG x, y;
        SetXForm2(&xform);
        x = (LONG)((xform.eM11 * entry->ptSrc.x + xform.eM12 * entry->ptSrc.y) + xform.eDx);
        y = (LONG)((xform.eM21 * entry->ptSrc.x + xform.eM22 * entry->ptSrc.y) + xform.eDy);
        x = MulDiv(x - ptWnd.x, sizView.cx, sizWnd.cx) + ptView.x;
        y = MulDiv(y - ptWnd.y, sizView.cy, sizWnd.cy) + ptView.y;
        // TODO: make more accurate
        ok(labs(pt.x - x) <= 2 && labs(pt.y - y) <= 2,
           "Line %d: Dest expected (%ld, %ld), was (%ld, %ld)\n",
           entry->lineno, x, y, pt.x, pt.y);
    }
    else if (entry->ptDest.x != NO_CHECK)
    {
        ok(pt.x == entry->ptDest.x && pt.y == entry->ptDest.y,
           "Line %d: Dest expected (%ld, %ld), was (%ld, %ld)\n",
           entry->lineno, entry->ptDest.x, entry->ptDest.y, pt.x, pt.y);
    }
}

START_TEST(LPtoDP)
{
    size_t i;
    POINT apt[2];
    XFORM xform;

    HDC hDC = CreateCompatibleDC(NULL);
    ok(hDC != NULL, "hDC is NULL\n");

    SetLastError(0xDEADBEEF);
    ok_int(LPtoDP(NULL, NULL, 0), 1);
    ok_err(0xDEADBEEF);

    ok_int(LPtoDP(NULL, NULL, -1), 1);
    ok_err(0xDEADBEEF);

    ok_int(LPtoDP(NULL, INVALID_POINTER, -1), 1);
    ok_err(0xDEADBEEF);

    ok_int(LPtoDP(NULL, NULL, 2), 0);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xDEADBEEF);
    ok_int(LPtoDP(NULL, apt, 2), 0);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xDEADBEEF);
    ok_int(LPtoDP(NULL, apt, 0), 1);
    ok_err(0xDEADBEEF);

    SetLastError(0xDEADBEEF);
    ok_int(LPtoDP(NULL, apt, -2), 1);
    ok_err(0xDEADBEEF);

    SetLastError(0xDEADBEEF);
    ok_int(LPtoDP((HDC)-4, apt, -2), 1);
    ok_err(0xDEADBEEF);

    SetLastError(0xDEADBEEF);
    ok_int(LPtoDP(hDC, NULL, 2), 1);
    ok_err(0xDEADBEEF);

    SetLastError(0xDEADBEEF);
    ok_int(LPtoDP(hDC, INVALID_POINTER, 2), 1);
    ok_err(0xDEADBEEF);

    SetGraphicsMode(hDC, GM_COMPATIBLE);
    ModifyWorldTransform(hDC, NULL, MWT_IDENTITY);

    for (i = 0; i < s_entries_count; ++i)
    {
        DoTestEntry(hDC, &s_entries[i]);
    }

    SetGraphicsMode(hDC, GM_ADVANCED);
    ModifyWorldTransform(hDC, NULL, MWT_IDENTITY);

    for (i = 0; i < s_entries_count; ++i)
    {
        DoTestEntry(hDC, &s_entries[i]);
    }

    SetXForm1(&xform);
    SetGraphicsMode(hDC, GM_ADVANCED);
    SetWorldTransform(hDC, &xform);
    for (i = 0; i < s_shifted_entries_count; ++i)
    {
        DoTestEntry(hDC, &s_shifted_entries[i]);
    }

    SetXForm2(&xform);
    SetGraphicsMode(hDC, GM_ADVANCED);
    SetWorldTransform(hDC, &xform);
    for (i = 0; i < s_transformed_entries_count; ++i)
    {
        DoTestEntry(hDC, &s_transformed_entries[i]);
    }

    DeleteDC(hDC);
}

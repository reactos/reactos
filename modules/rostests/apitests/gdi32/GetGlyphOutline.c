/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GetGlyphOutline
 * PROGRAMMERS:     Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

typedef struct TEST_ENTRY
{
    INT line;
    LPCWSTR lfFaceName;
    LONG lfHeight;
    DWORD dwRet;
    DWORD dwError;
    WCHAR wch;
    UINT uFormat;
    BOOL bMetrics;
    GLYPHMETRICS gm;
    DWORD cbBuffer;
    LPVOID lpvBuffer;
} TEST_ENTRY;

static const MAT2 s_mat = { {0,1}, {0,0}, {0,0}, {0,1} };

static BYTE s_ab[512];

#define WCH0 0
#define WCH1 0xFFFF
#define WCH2 L'A'
#define WCH3 L'T'
#define WCH4 L'g'
#define WCH5 L'.'

static const TEST_ENTRY s_entries[] =
{
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH0, GGO_METRICS, FALSE, { 0 } },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH1, GGO_METRICS, FALSE, { 0 } },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH2, GGO_METRICS, FALSE, { 0 } },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH3, GGO_METRICS, FALSE, { 0 } },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH4, GGO_METRICS, FALSE, { 0 } },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH5, GGO_METRICS, FALSE, { 0 } },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH0, GGO_METRICS, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH1, GGO_METRICS, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH2, GGO_METRICS, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH3, GGO_METRICS, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH4, GGO_METRICS, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH5, GGO_METRICS, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH0, GGO_METRICS, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH1, GGO_METRICS, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH2, GGO_METRICS, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH3, GGO_METRICS, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH4, GGO_METRICS, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH5, GGO_METRICS, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH0, GGO_BITMAP, FALSE, { 0 }, },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH1, GGO_BITMAP, FALSE, { 0 }, },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH2, GGO_BITMAP, FALSE, { 0 }, },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH3, GGO_BITMAP, FALSE, { 0 }, },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH4, GGO_BITMAP, FALSE, { 0 }, },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH5, GGO_BITMAP, FALSE, { 0 }, },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH0, GGO_BITMAP, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH1, GGO_BITMAP, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH2, GGO_BITMAP, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH3, GGO_BITMAP, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH4, GGO_BITMAP, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH5, GGO_BITMAP, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH0, GGO_BITMAP, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH1, GGO_BITMAP, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH2, GGO_BITMAP, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH3, GGO_BITMAP, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH4, GGO_BITMAP, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -12, GDI_ERROR, 0xDEADBEEF, WCH5, GGO_BITMAP, FALSE, { 0 }, 512, s_ab },

    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH0, GGO_METRICS, FALSE, { 0 } },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH1, GGO_METRICS, FALSE, { 0 } },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH2, GGO_METRICS, FALSE, { 0 } },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH3, GGO_METRICS, FALSE, { 0 } },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH4, GGO_METRICS, FALSE, { 0 } },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH5, GGO_METRICS, FALSE, { 0 } },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH0, GGO_METRICS, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH1, GGO_METRICS, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH2, GGO_METRICS, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH3, GGO_METRICS, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH4, GGO_METRICS, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH5, GGO_METRICS, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH0, GGO_METRICS, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH1, GGO_METRICS, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH2, GGO_METRICS, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH3, GGO_METRICS, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH4, GGO_METRICS, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH5, GGO_METRICS, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH0, GGO_BITMAP, FALSE, { 0 }, },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH1, GGO_BITMAP, FALSE, { 0 }, },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH2, GGO_BITMAP, FALSE, { 0 }, },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH3, GGO_BITMAP, FALSE, { 0 }, },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH4, GGO_BITMAP, FALSE, { 0 }, },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH5, GGO_BITMAP, FALSE, { 0 }, },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH0, GGO_BITMAP, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH1, GGO_BITMAP, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH2, GGO_BITMAP, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH3, GGO_BITMAP, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH4, GGO_BITMAP, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH5, GGO_BITMAP, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH0, GGO_BITMAP, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH1, GGO_BITMAP, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH2, GGO_BITMAP, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH3, GGO_BITMAP, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH4, GGO_BITMAP, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -14, GDI_ERROR, 0xDEADBEEF, WCH5, GGO_BITMAP, FALSE, { 0 }, 512, s_ab },

    { __LINE__, L"Tahoma", -12, 36, 0xDEADBEEF, WCH1, GGO_METRICS, TRUE, { 9, 9, { 2, 9 }, 12, 0 }, },
    { __LINE__, L"Tahoma", -12, 28, 0xDEADBEEF, WCH2, GGO_METRICS, TRUE, { 7, 9, { 0, 9 }, 8, 0 }, },
    { __LINE__, L"Tahoma", -12, 28, 0xDEADBEEF, WCH3, GGO_METRICS, TRUE, { 7, 9, { 0, 9 }, 8, 0 }, },
    { __LINE__, L"Tahoma", -12, 28, 0xDEADBEEF, WCH4, GGO_METRICS, TRUE, { 6, 9, { 0, 7 }, 7, 0 }, },
    { __LINE__, L"Tahoma", -12, 20, 0xDEADBEEF, WCH5, GGO_METRICS, TRUE, { 1, 2, { 1, 2 }, 4, 0 }, },
    { __LINE__, L"Tahoma", -12, 36, 0xDEADBEEF, WCH1, GGO_METRICS, TRUE, { 9, 9, { 2, 9 }, 12, 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -12, 28, 0xDEADBEEF, WCH2, GGO_METRICS, TRUE, { 7, 9, { 0, 9 }, 8, 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -12, 28, 0xDEADBEEF, WCH3, GGO_METRICS, TRUE, { 7, 9, { 0, 9 }, 8, 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -12, 28, 0xDEADBEEF, WCH4, GGO_METRICS, TRUE, { 6, 9, { 0, 7 }, 7, 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -12, 20, 0xDEADBEEF, WCH5, GGO_METRICS, TRUE, { 1, 2, { 1, 2 }, 4, 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -12, 36, 0xDEADBEEF, WCH1, GGO_METRICS, TRUE, { 9, 9, { 2, 9 }, 12, 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -12, 28, 0xDEADBEEF, WCH2, GGO_METRICS, TRUE, { 7, 9, { 0, 9 }, 8, 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -12, 28, 0xDEADBEEF, WCH3, GGO_METRICS, TRUE, { 7, 9, { 0, 9 }, 8, 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -12, 28, 0xDEADBEEF, WCH4, GGO_METRICS, TRUE, { 6, 9, { 0, 7 }, 7, 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -12, 20, 0xDEADBEEF, WCH5, GGO_METRICS, TRUE, { 1, 2, { 1, 2 }, 4, 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -12, 36, 0xDEADBEEF, WCH1, GGO_BITMAP, TRUE, { 9, 9, { 2, 9 }, 12, 0 }, },
    { __LINE__, L"Tahoma", -12, 36, 0xDEADBEEF, WCH2, GGO_BITMAP, TRUE, { 7, 9, { 0, 9 }, 8, 0 }, },
    { __LINE__, L"Tahoma", -12, 36, 0xDEADBEEF, WCH3, GGO_BITMAP, TRUE, { 7, 9, { 0, 9 }, 8, 0 }, },
    { __LINE__, L"Tahoma", -12, 36, 0xDEADBEEF, WCH4, GGO_BITMAP, TRUE, { 6, 9, { 0, 7 }, 7, 0 }, },
    { __LINE__, L"Tahoma", -12, 8, 0xDEADBEEF, WCH5, GGO_BITMAP, TRUE, { 1, 2, { 1, 2 }, 4, 0 }, },
    { __LINE__, L"Tahoma", -12, 36, 0xDEADBEEF, WCH1, GGO_BITMAP, TRUE, { 9, 9, { 2, 9 }, 12, 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -12, 36, 0xDEADBEEF, WCH2, GGO_BITMAP, TRUE, { 7, 9, { 0, 9 }, 8, 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -12, 36, 0xDEADBEEF, WCH3, GGO_BITMAP, TRUE, { 7, 9, { 0, 9 }, 8, 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -12, 36, 0xDEADBEEF, WCH4, GGO_BITMAP, TRUE, { 6, 9, { 0, 7 }, 7, 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -12, 8, 0xDEADBEEF, WCH5, GGO_BITMAP, TRUE, { 1, 2, { 1, 2 }, 4, 0 }, 512, s_ab },

    { __LINE__, L"Tahoma", -14, 40, 0xDEADBEEF, WCH1, GGO_METRICS, TRUE, { 11, 11, { 2, 11 }, 14, 0 }, },
    { __LINE__, L"Tahoma", -14, 28, 0xDEADBEEF, WCH2, GGO_METRICS, TRUE, { 7, 10, { 0, 10 }, 8, 0 }, },
    { __LINE__, L"Tahoma", -14, 36, 0xDEADBEEF, WCH3, GGO_METRICS, TRUE, { 9, 10, { -1, 10 }, 8, 0 }, },
    { __LINE__, L"Tahoma", -14, 28, 0xDEADBEEF, WCH4, GGO_METRICS, TRUE, { 7, 11, { 0, 8 }, 8, 0 }, },
    { __LINE__, L"Tahoma", -14, 20, 0xDEADBEEF, WCH5, GGO_METRICS, TRUE, { 1, 2, { 1, 2 }, 4, 0 }, },
    { __LINE__, L"Tahoma", -14, 40, 0xDEADBEEF, WCH1, GGO_METRICS, TRUE, { 11, 11, { 2, 11 }, 14, 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -14, 28, 0xDEADBEEF, WCH2, GGO_METRICS, TRUE, { 7, 10, { 0, 10 }, 8, 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -14, 36, 0xDEADBEEF, WCH3, GGO_METRICS, TRUE, { 9, 10, { -1, 10 }, 8, 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -14, 28, 0xDEADBEEF, WCH4, GGO_METRICS, TRUE, { 7, 11, { 0, 8 }, 8, 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -14, 20, 0xDEADBEEF, WCH5, GGO_METRICS, TRUE, { 1, 2, { 1, 2 }, 4, 0 }, 1, s_ab },
    { __LINE__, L"Tahoma", -14, 40, 0xDEADBEEF, WCH1, GGO_METRICS, TRUE, { 11, 11, { 2, 11 }, 14, 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -14, 28, 0xDEADBEEF, WCH2, GGO_METRICS, TRUE, { 7, 10, { 0, 10 }, 8, 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -14, 36, 0xDEADBEEF, WCH3, GGO_METRICS, TRUE, { 9, 10, { -1, 10 }, 8, 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -14, 28, 0xDEADBEEF, WCH4, GGO_METRICS, TRUE, { 7, 11, { 0, 8 }, 8, 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -14, 20, 0xDEADBEEF, WCH5, GGO_METRICS, TRUE, { 1, 2, { 1, 2 }, 4, 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -14, 44, 0xDEADBEEF, WCH1, GGO_BITMAP, TRUE, { 11, 11, { 2, 11 }, 14, 0 }, },
    { __LINE__, L"Tahoma", -14, 40, 0xDEADBEEF, WCH2, GGO_BITMAP, TRUE, { 7, 10, { 0, 10 }, 8, 0 }, },
    { __LINE__, L"Tahoma", -14, 40, 0xDEADBEEF, WCH3, GGO_BITMAP, TRUE, { 9, 10, { -1, 10 }, 8, 0 }, },
    { __LINE__, L"Tahoma", -14, 0x2C, 0xDEADBEEF, WCH4, GGO_BITMAP, TRUE, { 7, 11, { 0, 8 }, 8, 0 }, },
    { __LINE__, L"Tahoma", -14, 0x8, 0xDEADBEEF, WCH5, GGO_BITMAP, TRUE, { 1, 2, { 1, 2 }, 4, 0 }, },
    { __LINE__, L"Tahoma", -14, 44, 0xDEADBEEF, WCH1, GGO_BITMAP, TRUE, { 11, 11, { 2, 11 }, 14, 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -14, 40, 0xDEADBEEF, WCH2, GGO_BITMAP, TRUE, { 7, 10, { 0, 10 }, 8, 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -14, 40, 0xDEADBEEF, WCH3, GGO_BITMAP, TRUE, { 9, 10, { -1, 10 }, 8, 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -14, 0x2C, 0xDEADBEEF, WCH4, GGO_BITMAP, TRUE, { 7, 11, { 0, 8 }, 8, 0 }, 512, s_ab },
    { __LINE__, L"Tahoma", -14, 0x8, 0xDEADBEEF, WCH5, GGO_BITMAP, TRUE, { 1, 2, { 1, 2 }, 4, 0 }, 512, s_ab },

    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH0, GGO_METRICS, FALSE, { 0 }, },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH1, GGO_METRICS, FALSE, { 0 }, },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH2, GGO_METRICS, FALSE, { 0 }, },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH3, GGO_METRICS, FALSE, { 0 }, },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH4, GGO_METRICS, FALSE, { 0 }, },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH5, GGO_METRICS, FALSE, { 0 }, },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH0, GGO_METRICS, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH1, GGO_METRICS, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH2, GGO_METRICS, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH3, GGO_METRICS, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH4, GGO_METRICS, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH5, GGO_METRICS, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH0, GGO_METRICS, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH1, GGO_METRICS, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH2, GGO_METRICS, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH3, GGO_METRICS, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH4, GGO_METRICS, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH5, GGO_METRICS, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH0, GGO_BITMAP, FALSE, { 0 }, },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH1, GGO_BITMAP, FALSE, { 0 }, },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH2, GGO_BITMAP, FALSE, { 0 }, },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH3, GGO_BITMAP, FALSE, { 0 }, },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH4, GGO_BITMAP, FALSE, { 0 }, },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH5, GGO_BITMAP, FALSE, { 0 }, },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH0, GGO_BITMAP, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH1, GGO_BITMAP, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH2, GGO_BITMAP, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH3, GGO_BITMAP, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH4, GGO_BITMAP, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH5, GGO_BITMAP, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH0, GGO_BITMAP, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH1, GGO_BITMAP, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH2, GGO_BITMAP, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH3, GGO_BITMAP, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH4, GGO_BITMAP, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -12, GDI_ERROR, 0xDEADBEEF, WCH5, GGO_BITMAP, FALSE, { 0 }, 512, s_ab },

    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH0, GGO_METRICS, FALSE, { 0 }, },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH1, GGO_METRICS, FALSE, { 0 }, },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH2, GGO_METRICS, FALSE, { 0 }, },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH3, GGO_METRICS, FALSE, { 0 }, },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH4, GGO_METRICS, FALSE, { 0 }, },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH5, GGO_METRICS, FALSE, { 0 }, },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH0, GGO_METRICS, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH1, GGO_METRICS, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH2, GGO_METRICS, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH3, GGO_METRICS, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH4, GGO_METRICS, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH5, GGO_METRICS, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH0, GGO_METRICS, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH1, GGO_METRICS, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH2, GGO_METRICS, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH3, GGO_METRICS, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH4, GGO_METRICS, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH5, GGO_METRICS, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH0, GGO_BITMAP, FALSE, { 0 }, },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH1, GGO_BITMAP, FALSE, { 0 }, },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH2, GGO_BITMAP, FALSE, { 0 }, },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH3, GGO_BITMAP, FALSE, { 0 }, },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH4, GGO_BITMAP, FALSE, { 0 }, },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH5, GGO_BITMAP, FALSE, { 0 }, },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH0, GGO_BITMAP, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH1, GGO_BITMAP, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH2, GGO_BITMAP, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH3, GGO_BITMAP, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH4, GGO_BITMAP, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH5, GGO_BITMAP, FALSE, { 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH0, GGO_BITMAP, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH1, GGO_BITMAP, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH2, GGO_BITMAP, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH3, GGO_BITMAP, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH4, GGO_BITMAP, FALSE, { 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -14, GDI_ERROR, 0xDEADBEEF, WCH5, GGO_BITMAP, FALSE, { 0 }, 512, s_ab },

    { __LINE__, L"Marlett", -12, 24, 0xDEADBEEF, WCH0, GGO_METRICS, TRUE, { 5, 8, { 0, 8 }, 6, 0 }, },
    { __LINE__, L"Marlett", -12, 24, 0xDEADBEEF, WCH1, GGO_METRICS, TRUE, { 5, 8, { 0, 8 }, 6, 0 }, },
    { __LINE__, L"Marlett", -12, 24, 0xDEADBEEF, WCH2, GGO_METRICS, TRUE, { 5, 8, { 0, 8 }, 6, 0 }, },
    { __LINE__, L"Marlett", -12, 24, 0xDEADBEEF, WCH3, GGO_METRICS, TRUE, { 5, 8, { 0, 8 }, 6, 0 }, },
    { __LINE__, L"Marlett", -12, 28, 0xDEADBEEF, WCH4, GGO_METRICS, TRUE, { 8, 9, { 2, 11 }, 12, 0 }, },
    { __LINE__, L"Marlett", -12, 24, 0xDEADBEEF, WCH5, GGO_METRICS, TRUE, { 5, 8, { 0, 8 }, 6, 0 }, },
    { __LINE__, L"Marlett", -12, 24, 0xDEADBEEF, WCH0, GGO_METRICS, TRUE, { 5, 8, { 0, 8 }, 6, 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -12, 24, 0xDEADBEEF, WCH1, GGO_METRICS, TRUE, { 5, 8, { 0, 8 }, 6, 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -12, 24, 0xDEADBEEF, WCH2, GGO_METRICS, TRUE, { 5, 8, { 0, 8 }, 6, 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -12, 24, 0xDEADBEEF, WCH3, GGO_METRICS, TRUE, { 5, 8, { 0, 8 }, 6, 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -12, 28, 0xDEADBEEF, WCH4, GGO_METRICS, TRUE, { 8, 9, { 2, 11 }, 12, 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -12, 24, 0xDEADBEEF, WCH5, GGO_METRICS, TRUE, { 5, 8, { 0, 8 }, 6, 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -12, 24, 0xDEADBEEF, WCH0, GGO_METRICS, TRUE, { 5, 8, { 0, 8 }, 6, 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -12, 24, 0xDEADBEEF, WCH1, GGO_METRICS, TRUE, { 5, 8, { 0, 8 }, 6, 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -12, 24, 0xDEADBEEF, WCH2, GGO_METRICS, TRUE, { 5, 8, { 0, 8 }, 6, 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -12, 24, 0xDEADBEEF, WCH3, GGO_METRICS, TRUE, { 5, 8, { 0, 8 }, 6, 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -12, 28, 0xDEADBEEF, WCH4, GGO_METRICS, TRUE, { 8, 9, { 2, 11 }, 12, 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -12, 24, 0xDEADBEEF, WCH5, GGO_METRICS, TRUE, { 5, 8, { 0, 8 }, 6, 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -12, 32, 0xDEADBEEF, WCH0, GGO_BITMAP, TRUE, { 5, 8, { 0, 8 }, 6, 0 }, },
    { __LINE__, L"Marlett", -12, 32, 0xDEADBEEF, WCH1, GGO_BITMAP, TRUE, { 5, 8, { 0, 8 }, 6, 0 }, },
    { __LINE__, L"Marlett", -12, 32, 0xDEADBEEF, WCH2, GGO_BITMAP, TRUE, { 5, 8, { 0, 8 }, 6, 0 }, },
    { __LINE__, L"Marlett", -12, 32, 0xDEADBEEF, WCH3, GGO_BITMAP, TRUE, { 5, 8, { 0, 8 }, 6, 0 }, },
    { __LINE__, L"Marlett", -12, 36, 0xDEADBEEF, WCH4, GGO_BITMAP, TRUE, { 8, 9, { 2, 11 }, 12, 0 }, },
    { __LINE__, L"Marlett", -12, 32, 0xDEADBEEF, WCH5, GGO_BITMAP, TRUE, { 5, 8, { 0, 8 }, 6, 0 }, },
    { __LINE__, L"Marlett", -12, 32, 0xDEADBEEF, WCH0, GGO_BITMAP, TRUE, { 5, 8, { 0, 8 }, 6, 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -12, 32, 0xDEADBEEF, WCH1, GGO_BITMAP, TRUE, { 5, 8, { 0, 8 }, 6, 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -12, 32, 0xDEADBEEF, WCH2, GGO_BITMAP, TRUE, { 5, 8, { 0, 8 }, 6, 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -12, 32, 0xDEADBEEF, WCH3, GGO_BITMAP, TRUE, { 5, 8, { 0, 8 }, 6, 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -12, 36, 0xDEADBEEF, WCH4, GGO_BITMAP, TRUE, { 8, 9, { 2, 11 }, 12, 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -12, 32, 0xDEADBEEF, WCH5, GGO_BITMAP, TRUE, { 5, 8, { 0, 8 }, 6, 0 }, 512, s_ab },

    { __LINE__, L"Marlett", -14, 28, 0xDEADBEEF, WCH0, GGO_METRICS, TRUE, { 6, 9, { 0, 9 }, 7, 0 }, },
    { __LINE__, L"Marlett", -14, 28, 0xDEADBEEF, WCH1, GGO_METRICS, TRUE, { 6, 9, { 0, 9 }, 7, 0 }, },
    { __LINE__, L"Marlett", -14, 28, 0xDEADBEEF, WCH2, GGO_METRICS, TRUE, { 6, 9, { 0, 9 }, 7, 0 }, },
    { __LINE__, L"Marlett", -14, 28, 0xDEADBEEF, WCH3, GGO_METRICS, TRUE, { 6, 9, { 0, 9 }, 7, 0 }, },
    { __LINE__, L"Marlett", -14, 36, 0xDEADBEEF, WCH4, GGO_METRICS, TRUE, { 10, 10, { 2, 12 }, 14, 0 }, },
    { __LINE__, L"Marlett", -14, 28, 0xDEADBEEF, WCH5, GGO_METRICS, TRUE, { 6, 9, { 0, 9 }, 7, 0 }, },
    { __LINE__, L"Marlett", -14, 28, 0xDEADBEEF, WCH0, GGO_METRICS, TRUE, { 6, 9, { 0, 9 }, 7, 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -14, 28, 0xDEADBEEF, WCH1, GGO_METRICS, TRUE, { 6, 9, { 0, 9 }, 7, 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -14, 28, 0xDEADBEEF, WCH2, GGO_METRICS, TRUE, { 6, 9, { 0, 9 }, 7, 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -14, 28, 0xDEADBEEF, WCH3, GGO_METRICS, TRUE, { 6, 9, { 0, 9 }, 7, 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -14, 36, 0xDEADBEEF, WCH4, GGO_METRICS, TRUE, { 10, 10, { 2, 12 }, 14, 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -14, 28, 0xDEADBEEF, WCH5, GGO_METRICS, TRUE, { 6, 9, { 0, 9 }, 7, 0 }, 1, s_ab },
    { __LINE__, L"Marlett", -14, 28, 0xDEADBEEF, WCH0, GGO_METRICS, TRUE, { 6, 9, { 0, 9 }, 7, 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -14, 28, 0xDEADBEEF, WCH1, GGO_METRICS, TRUE, { 6, 9, { 0, 9 }, 7, 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -14, 28, 0xDEADBEEF, WCH2, GGO_METRICS, TRUE, { 6, 9, { 0, 9 }, 7, 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -14, 28, 0xDEADBEEF, WCH3, GGO_METRICS, TRUE, { 6, 9, { 0, 9 }, 7, 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -14, 36, 0xDEADBEEF, WCH4, GGO_METRICS, TRUE, { 10, 10, { 2, 12 }, 14, 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -14, 28, 0xDEADBEEF, WCH5, GGO_METRICS, TRUE, { 6, 9, { 0, 9 }, 7, 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -14, 36, 0xDEADBEEF, WCH0, GGO_BITMAP, TRUE, { 6, 9, { 0, 9 }, 7, 0 }, },
    { __LINE__, L"Marlett", -14, 36, 0xDEADBEEF, WCH1, GGO_BITMAP, TRUE, { 6, 9, { 0, 9 }, 7, 0 }, },
    { __LINE__, L"Marlett", -14, 36, 0xDEADBEEF, WCH2, GGO_BITMAP, TRUE, { 6, 9, { 0, 9 }, 7, 0 }, },
    { __LINE__, L"Marlett", -14, 36, 0xDEADBEEF, WCH3, GGO_BITMAP, TRUE, { 6, 9, { 0, 9 }, 7, 0 }, },
    { __LINE__, L"Marlett", -14, 40, 0xDEADBEEF, WCH4, GGO_BITMAP, TRUE, { 10, 10, { 2, 12 }, 14, 0 }, },
    { __LINE__, L"Marlett", -14, 36, 0xDEADBEEF, WCH5, GGO_BITMAP, TRUE, { 6, 9, { 0, 9 }, 7, 0 }, },
    { __LINE__, L"Marlett", -14, 36, 0xDEADBEEF, WCH0, GGO_BITMAP, TRUE, { 6, 9, { 0, 9 }, 7, 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -14, 36, 0xDEADBEEF, WCH1, GGO_BITMAP, TRUE, { 6, 9, { 0, 9 }, 7, 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -14, 36, 0xDEADBEEF, WCH2, GGO_BITMAP, TRUE, { 6, 9, { 0, 9 }, 7, 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -14, 36, 0xDEADBEEF, WCH3, GGO_BITMAP, TRUE, { 6, 9, { 0, 9 }, 7, 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -14, 40, 0xDEADBEEF, WCH4, GGO_BITMAP, TRUE, { 10, 10, { 2, 12 }, 14, 0 }, 512, s_ab },
    { __LINE__, L"Marlett", -14, 36, 0xDEADBEEF, WCH5, GGO_BITMAP, TRUE, { 6, 9, { 0, 9 }, 7, 0 }, 512, s_ab },
};

void DoEntry(const TEST_ENTRY *pEntry)
{
    LOGFONTW lf;
    HFONT hFont;
    HDC hDC;
    HGDIOBJ hFontOld;

    ZeroMemory(&lf, sizeof(lf));

    lf.lfHeight = pEntry->lfHeight;
    lf.lfCharSet = DEFAULT_CHARSET;
    lstrcpyW(lf.lfFaceName, pEntry->lfFaceName);

    hFont = CreateFontIndirectW(&lf);
    ok(hFont != NULL, "hFont was NULL\n");
    if (hFont == NULL)
    {
        skip("Line %d: skipped because hFont == NULL\n", pEntry->line);
        return;
    }

    hDC = CreateCompatibleDC(NULL);
    ok(hDC != NULL, "hDC was NULL\n");
    if (hDC == NULL)
    {
        skip("Line %d: skipped because hDC == NULL\n", pEntry->line);
        DeleteObject(hFont);
        return;
    }

    hFontOld = SelectObject(hDC, hFont);
    ok(hFontOld != NULL, "SelectObject failed\n");
    if (hFontOld == NULL)
    {
        skip("Line %d: skipped because SelectObject failed\n", pEntry->line);
    }
    else
    {
        DWORD dwRet, dwError;
        GLYPHMETRICS gm;

        SetLastError(0xDEADBEEF);

        if (pEntry->bMetrics)
        {
            FillMemory(&gm, sizeof(gm), 0xCD);
            dwRet = GetGlyphOutlineW(hDC, pEntry->wch, pEntry->uFormat, &gm, pEntry->cbBuffer, pEntry->lpvBuffer, &s_mat);
        }
        else
        {
            dwRet = GetGlyphOutlineW(hDC, pEntry->wch, pEntry->uFormat, NULL, pEntry->cbBuffer, pEntry->lpvBuffer, &s_mat);
        }
        dwError = GetLastError();

        ok(dwRet == pEntry->dwRet, "Line %d: dwRet expected 0x%lX, was 0x%lX\n", pEntry->line, pEntry->dwRet, dwRet);
        ok(dwError == pEntry->dwError, "Line %d: dwError expected 0x%lX, was 0x%lX\n", pEntry->line, pEntry->dwError, dwError);

        if (pEntry->bMetrics)
        {
            ok(gm.gmBlackBoxX == pEntry->gm.gmBlackBoxX, "Line %d: gm.gmBlackBoxX expected 0x%X, was 0x%X\n", pEntry->line, pEntry->gm.gmBlackBoxX, gm.gmBlackBoxX);
            ok(gm.gmBlackBoxY == pEntry->gm.gmBlackBoxY, "Line %d: gm.gmBlackBoxY expected 0x%X, was 0x%X\n", pEntry->line, pEntry->gm.gmBlackBoxY, gm.gmBlackBoxY);
            ok(gm.gmptGlyphOrigin.x == pEntry->gm.gmptGlyphOrigin.x, "Line %d: gm.gmptGlyphOrigin.x expected %ld, was %ld\n", pEntry->line, pEntry->gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.x);
            ok(gm.gmptGlyphOrigin.y == pEntry->gm.gmptGlyphOrigin.y, "Line %d: gm.gmptGlyphOrigin.y expected %ld, was %ld\n", pEntry->line, pEntry->gm.gmptGlyphOrigin.y, gm.gmptGlyphOrigin.y);
            ok(gm.gmCellIncX == pEntry->gm.gmCellIncX, "Line %d: gm.gmCellIncX expected %d, was %d\n", pEntry->line, pEntry->gm.gmCellIncX, gm.gmCellIncX);
            ok(gm.gmCellIncY == pEntry->gm.gmCellIncY, "Line %d: gm.gmCellIncY expected %d, was %d\n", pEntry->line, pEntry->gm.gmCellIncY, gm.gmCellIncY);
        }
        SelectObject(hDC, hFontOld);
    }

    DeleteObject(hFont);
    DeleteDC(hDC);
}

START_TEST(GetGlyphOutline)
{
    SIZE_T i, count = ARRAYSIZE(s_entries);
    for (i = 0; i < count; ++i)
    {
        DoEntry(&s_entries[i]);
    }
}

/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for World Transformation and font rendering
 * PROGRAMMERS:     Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

typedef struct tagBITMAPINFOEX
{
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD          bmiColors[256];
} BITMAPINFOEX, FAR *LPBITMAPINFOEX;

#if 1
    #define SaveBitmapToFile(f, h)
#else
static BOOL SaveBitmapToFile(LPCWSTR pszFileName, HBITMAP hbm)
{
    BOOL f;
    BITMAPFILEHEADER bf;
    BITMAPINFOEX bmi;
    BITMAPINFOHEADER *pbmih;
    DWORD cb, cbColors;
    HDC hDC;
    HANDLE hFile;
    LPVOID pBits;
    BITMAP bm;

    if (!GetObjectW(hbm, sizeof(BITMAP), &bm))
        return FALSE;

    pbmih = &bmi.bmiHeader;
    ZeroMemory(pbmih, sizeof(BITMAPINFOHEADER));
    pbmih->biSize             = sizeof(BITMAPINFOHEADER);
    pbmih->biWidth            = bm.bmWidth;
    pbmih->biHeight           = bm.bmHeight;
    pbmih->biPlanes           = 1;
    pbmih->biBitCount         = bm.bmBitsPixel;
    pbmih->biCompression      = BI_RGB;
    pbmih->biSizeImage        = bm.bmWidthBytes * bm.bmHeight;

    if (bm.bmBitsPixel < 16)
        cbColors = (1 << bm.bmBitsPixel) * sizeof(RGBQUAD);
    else
        cbColors = 0;

    bf.bfType = 0x4d42;
    bf.bfReserved1 = 0;
    bf.bfReserved2 = 0;
    cb = sizeof(BITMAPFILEHEADER) + pbmih->biSize + cbColors;
    bf.bfOffBits = cb;
    bf.bfSize = cb + pbmih->biSizeImage;

    pBits = HeapAlloc(GetProcessHeap(), 0, pbmih->biSizeImage);
    if (pBits == NULL)
        return FALSE;

    f = FALSE;
    hDC = CreateCompatibleDC(NULL);
    if (hDC)
    {
        if (GetDIBits(hDC, hbm, 0, bm.bmHeight, pBits, (BITMAPINFO *)&bmi,
                      DIB_RGB_COLORS))
        {
            hFile = CreateFileW(pszFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL |
                                FILE_FLAG_WRITE_THROUGH, NULL);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                f = WriteFile(hFile, &bf, sizeof(BITMAPFILEHEADER), &cb, NULL) &&
                    WriteFile(hFile, &bmi, sizeof(BITMAPINFOHEADER), &cb, NULL) &&
                    WriteFile(hFile, bmi.bmiColors, cbColors, &cb, NULL) &&
                    WriteFile(hFile, pBits, pbmih->biSizeImage, &cb, NULL);
                CloseHandle(hFile);

                if (!f)
                    DeleteFileW(pszFileName);
            }
        }
        DeleteDC(hDC);
    }
    HeapFree(GetProcessHeap(), 0, pBits);
    return f;
}
#endif

typedef struct TEST_ENTRY
{
    INT line;               // line number
    INT GraphicsMode;       // GM_COMPATIBLE or GM_ADVANCED
    POINT ptRef;            // reference point
    INT TextAlign;
    XFORM xform;
    BOOL xform_ok;
    LPCWSTR filename;
    INT cWhitePoints;       // number of white points
    POINT WhitePoints[4];
    INT cBlackPoints;       // number of black points
    POINT BlackPoints[4];
} TEST_ENTRY;

#define WIDTH 400
#define HEIGHT 400
#define XCENTER (WIDTH / 2)
#define YCENTER (HEIGHT / 2)
#define BLACK RGB(0, 0, 0)
#define WHITE RGB(255, 255, 255)
#define LFHEIGHT -100

static const RECT s_rc = {0, 0, WIDTH, HEIGHT};
static HBRUSH s_hWhiteBrush = NULL;
static HPEN s_hRedPen = NULL;

#define POS(ix, iy) {XCENTER + (ix) * WIDTH/8, YCENTER + (iy) * HEIGHT/8}

#define NO_TRANS_1 \
    3, {POS(1, 1), POS(-1, 1), POS(-1, -1)}, 1, {POS(1, -1)}

#define NO_TRANS_2 \
    3, {POS(1, -1), POS(-1, 1), POS(-1, -1)}, 1, {POS(1, 1)}

static const TEST_ENTRY s_entries[] =
{
    // GM_COMPATIBLE TA_BOTTOM
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 0, 0, 1, 0, 0}, FALSE, L"000.bmp", NO_TRANS_1},
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1.5, 0, 0, 1, 0, 0}, FALSE, L"001.bmp", NO_TRANS_1},
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 0, 0, 1.5, 0, 0}, FALSE, L"002.bmp", NO_TRANS_1},
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {-1, 0, 0, 1, 0, 0}, FALSE, L"003.bmp", NO_TRANS_1},
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 0, 0, -1, 0, 0}, FALSE, L"004.bmp", NO_TRANS_1},
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 1, 1, 0, 0, 0}, FALSE, L"005.bmp", NO_TRANS_1},
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, -1, 1, 0, 0, 0}, FALSE, L"006.bmp", NO_TRANS_1},
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 1, -1, 0, 0, 0}, FALSE, L"007.bmp", NO_TRANS_1},
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, -1, -1, 0, 0, 0}, FALSE, L"009.bmp", NO_TRANS_1},
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 1, 0, 1, 0, 0}, FALSE, L"009.bmp", NO_TRANS_1},
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 1, 1, 1, 0, 0}, FALSE, L"010.bmp", NO_TRANS_1},
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 0, 0, 0, 0, 0}, FALSE, L"011.bmp", NO_TRANS_1},
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 0, 0, 0, 1000, 0}, FALSE, L"012.bmp", NO_TRANS_1},
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 0, 0, 0, 0, 1000}, FALSE, L"013.bmp", NO_TRANS_1},

    // GM_COMPATIBLE TA_TOP
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {1, 0, 0, 1, 0, 0}, FALSE, L"100.bmp", NO_TRANS_2},
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {1.5, 0, 0, 1, 0, 0}, FALSE, L"101.bmp", NO_TRANS_2},
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {1, 0, 0, 1.5, 0, 0}, FALSE, L"102.bmp", NO_TRANS_2},
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {-1, 0, 0, 1, 0, 0}, FALSE, L"103.bmp", NO_TRANS_2},
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {1, 0, 0, -1, 0, 0}, FALSE, L"104.bmp", NO_TRANS_2},
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {0, 1, 1, 0, 0, 0}, FALSE, L"105.bmp", NO_TRANS_2},
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {0, -1, 1, 0, 0, 0}, FALSE, L"106.bmp", NO_TRANS_2},
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {0, 1, -1, 0, 0, 0}, FALSE, L"107.bmp", NO_TRANS_2},
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {0, -1, -1, 0, 0, 0}, FALSE, L"109.bmp", NO_TRANS_2},
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {0, 1, 0, 1, 0, 0}, FALSE, L"109.bmp", NO_TRANS_2},
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {1, 1, 1, 1, 0, 0}, FALSE, L"110.bmp", NO_TRANS_2},
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {0, 0, 0, 0, 0, 0}, FALSE, L"111.bmp", NO_TRANS_2},
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {0, 0, 0, 0, 1000, 0}, FALSE, L"112.bmp", NO_TRANS_2},
    {__LINE__, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {0, 0, 0, 0, 0, 1000}, FALSE, L"113.bmp", NO_TRANS_2},

    // GM_ADVANCED TA_LEFT TA_BOTTOM
    {__LINE__, GM_ADVANCED, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 0, 0, 1, 0, 0}, TRUE, L"200.bmp", NO_TRANS_1},
    {__LINE__, GM_ADVANCED, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1.5, 0, 0, 1, 0, 0}, TRUE, L"201.bmp",
        4, {POS(-1, -1), POS(1, -1), POS(-1, 1), POS(1, 1)}, 1, {POS(3, -1)}},
    {__LINE__, GM_ADVANCED, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 0, 0, 1.5, 0, 0}, TRUE, L"202.bmp",
        3, {POS(-1, -1), POS(-1, 1), POS(1, -1)}, 1, {POS(1, 1)}},
    {__LINE__, GM_ADVANCED, {-XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {-1, 0, 0, 1, 0, 0}, TRUE, L"203.bmp",
        3, {POS(1, -1), POS(-1, 1), POS(1, -1)}, 1, {POS(-1, -1)}},
    {__LINE__, GM_ADVANCED, {XCENTER, -YCENTER}, TA_LEFT | TA_BOTTOM, {1, 0, 0, -1, 0, 0}, TRUE, L"204.bmp",
        3, {POS(-1, -1), POS(-1, 1), POS(1, -1)}, 1, {POS(1, 1)}},
    {__LINE__, GM_ADVANCED, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 1, 1, 0, 0, 0}, TRUE, L"205.bmp",
        3, {POS(-1, -1), POS(1, 1), POS(1, -1)}, 1, {POS(-1, 1)}},
    {__LINE__, GM_ADVANCED, {-XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, -1, 1, 0, 0, 0}, TRUE, L"206.bmp",
        3, {POS(-1, 1), POS(1, 1), POS(1, -1)}, 1, {POS(-1, -1)}},
    {__LINE__, GM_ADVANCED, {XCENTER, -YCENTER}, TA_LEFT | TA_BOTTOM, {0, 1, -1, 0, 0, 0}, TRUE, L"207.bmp",
        3, {POS(-1, 1), POS(-1, -1), POS(1, -1)}, 1, {POS(1, 1)}},
    {__LINE__, GM_ADVANCED, {-XCENTER, -YCENTER}, TA_LEFT | TA_BOTTOM, {0, -1, -1, 0, 0, 0}, TRUE, L"208.bmp",
        3, {POS(-1, 1), POS(-1, -1), POS(1, 1)}, 1, {POS(1, -1)}},
    {__LINE__, GM_ADVANCED, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 1, 0, 1, 0, 0}, FALSE, L"209.bmp", NO_TRANS_1},
    {__LINE__, GM_ADVANCED, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 1, 1, 1, 0, 0}, FALSE, L"210.bmp", NO_TRANS_1},
    {__LINE__, GM_ADVANCED, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 0, 0, 0, 0, 0}, FALSE, L"211.bmp", NO_TRANS_1},
    {__LINE__, GM_ADVANCED, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 0, 0, 0, 1000, 0}, FALSE, L"212.bmp", NO_TRANS_1},
    {__LINE__, GM_ADVANCED, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 0, 0, 0, 0, 1000}, FALSE, L"213.bmp", NO_TRANS_1},

    // GM_ADVANCED TA_LEFT TA_TOP
    {__LINE__, GM_ADVANCED, {0, 0}, TA_LEFT | TA_TOP, {2, 0, 0, 1, 0, 0}, TRUE, L"300.bmp",
        4, {POS(-1, -1), POS(-1, 1), POS(1, -1), POS(1, 1)}, 2, {POS(-3, -3), POS(-1, -3)}},
    {__LINE__, GM_ADVANCED, {0, 0}, TA_LEFT | TA_TOP, {1, 0, 0, 2, 0, 0}, TRUE, L"301.bmp",
        4, {POS(-1, -1), POS(-1, 1), POS(1, -1), POS(1, 1)}, 2, {POS(-3, -3), POS(-3, -1)}},
    {__LINE__, GM_ADVANCED, {0, 0}, TA_LEFT | TA_TOP, {2, 0, 0, 1, WIDTH/4, 0}, TRUE, L"302.bmp",
        3, {POS(-1, 1), POS(1, -1), POS(1, 1)}, 2, {POS(-1, -3), POS(1, -3)}},
    {__LINE__, GM_ADVANCED, {0, 0}, TA_LEFT | TA_TOP, {1, 0, 0, 2, WIDTH/4, 0}, TRUE, L"303.bmp",
        3, {POS(-1, 1), POS(1, -1), POS(1, 1)}, 2, {POS(-1, -3), POS(-1, -1)}},
    {__LINE__, GM_ADVANCED, {0, 0}, TA_LEFT | TA_TOP, {2, 0, 0, 1, 0, HEIGHT/4}, TRUE, L"304.bmp",
        3, {POS(-1, 1), POS(1, -1), POS(1, 1)}, 2, {POS(-3, -1), POS(-1, -1)}},
    {__LINE__, GM_ADVANCED, {0, 0}, TA_LEFT | TA_TOP, {1, 0, 0, 2, 0, HEIGHT/4}, TRUE, L"305.bmp",
        4, {POS(-1, -1), POS(-1, 1), POS(1, -1), POS(1, 1)}, 2, {POS(-3, -1), POS(-3, 1)}},
    {__LINE__, GM_ADVANCED, {0, YCENTER}, TA_LEFT | TA_TOP, {2, 0, 0, 1, 0, 0}, TRUE, L"306.bmp",
        3, {POS(-1, -1), POS(1, -1), POS(1, 1)}, 2, {POS(-3, 1), POS(-1, 1)}},
    {__LINE__, GM_ADVANCED, {0, YCENTER}, TA_LEFT | TA_TOP, {1, 0, 0, 2, 0, -HEIGHT/2}, TRUE, L"307.bmp",
        4, {POS(-1, -1), POS(-1, 1), POS(1, -1), POS(1, 1)}, 2, {POS(-3, 3), POS(-3, 1)}},
    {__LINE__, GM_ADVANCED, {0, YCENTER}, TA_LEFT | TA_TOP, {2, 0, 0, 1, WIDTH/4, 0}, TRUE, L"308.bmp",
        2, {POS(-1, -1), POS(1, -1)}, 2, {POS(-1, 1), POS(1, 1)}},
    {__LINE__, GM_ADVANCED, {0, YCENTER}, TA_LEFT | TA_TOP, {1, 0, 0, 2, WIDTH/4, -HEIGHT/2}, TRUE, L"309.bmp",
        3, {POS(-1, -1), POS(1, -1), POS(1, 1)}, 2, {POS(-1, 1), POS(-1, 3)}},
    {__LINE__, GM_ADVANCED, {0, YCENTER}, TA_LEFT | TA_TOP, {2, 0, 0, 1, 0, HEIGHT/4}, TRUE, L"310.bmp",
        4, {POS(-1, -1), POS(1, -1), POS(1, -1), POS(1, 1)}, 2, {POS(-3, 3), POS(-1, 3)}},
    {__LINE__, GM_ADVANCED, {0, YCENTER}, TA_LEFT | TA_TOP, {1, 0, 0, 2, 0, -HEIGHT/2}, TRUE, L"311.bmp",
        4, {POS(-1, -1), POS(1, -1), POS(1, -1), POS(1, 1)}, 2, {POS(-3, 3), POS(-3, 1)}},

    // GM_ADVANCED TA_CENTER TA_TOP
    {__LINE__, GM_ADVANCED, {XCENTER, 0}, TA_CENTER | TA_TOP, {2, 0, 0, 1, -WIDTH/4, 0}, TRUE, L"400.bmp",
        4, {POS(-1, -1), POS(-1, 1), POS(1, -1), POS(1, 1)}, 2, {POS(1, -3), POS(3, -3)}},
    {__LINE__, GM_ADVANCED, {XCENTER, 0}, TA_CENTER | TA_TOP, {1, 0, 0, 2, WIDTH/8, 0}, TRUE, L"401.bmp",
        3, {POS(-1, -1), POS(-1, 1), POS(1, 1)}, 2, {POS(1, -3), POS(1, -1)}},
    {__LINE__, GM_ADVANCED, {XCENTER, 0}, TA_CENTER | TA_TOP, {2, 0, 0, 2, -WIDTH/4, HEIGHT/4}, TRUE, L"402.bmp",
        2, {POS(-1, 1), POS(-1, -1)}, 4, {POS(1, -1), POS(3, -1), POS(1, 1), POS(3, 1)}},
    {__LINE__, GM_ADVANCED, {XCENTER, 0}, TA_CENTER | TA_TOP, {1, 0, 0, 2, WIDTH/8, 0}, TRUE, L"403.bmp",
        3, {POS(-1, -1), POS(-1, 1), POS(1, 1)}, 2, {POS(1, -1), POS(1, -3)}},
    {__LINE__, GM_ADVANCED, {XCENTER, 0}, TA_CENTER | TA_TOP, {2, 0, 0, 1, -WIDTH/4, HEIGHT/4}, TRUE, L"404.bmp",
        2, {POS(-1, 1), POS(1, 1)}, 2, {POS(1, -1), POS(3, -1)}},
    {__LINE__, GM_ADVANCED, {XCENTER, 0}, TA_CENTER | TA_TOP, {1, 0, 0, 2, WIDTH/8, HEIGHT/4}, TRUE, L"405.bmp",
        2, {POS(-1, -1), POS(-1, 1)}, 2, {POS(1, -1), POS(1, 1)}},
    {__LINE__, GM_ADVANCED, {0, YCENTER}, TA_CENTER | TA_TOP, {1, 0, 0, 2, WIDTH/8, -HEIGHT/2}, TRUE, L"406.bmp",
        4, {POS(-1, -1), POS(1, -1), POS(-1, 1), POS(1, 1)}, 2, {POS(-3, 1), POS(-3, 3)}},
    {__LINE__, GM_ADVANCED, {0, YCENTER}, TA_CENTER | TA_TOP, {2, 0, 0, 1, WIDTH/4, -HEIGHT/2}, TRUE, L"407.bmp",
        4, {POS(-1, -1), POS(1, -1), POS(-1, 1), POS(1, 1)}, 2, {POS(-3, -3), POS(-1, -3)}},
    {__LINE__, GM_ADVANCED, {0, YCENTER}, TA_CENTER | TA_TOP, {2, 0, 0, 2, WIDTH/4, -HEIGHT/2}, TRUE, L"408.bmp",
        3, {POS(-1, -1), POS(1, -1), POS(1, 1)}, 4, {POS(-3, 3), POS(-3, 1), POS(-3, 1), POS(-1, 1)}},
    {__LINE__, GM_ADVANCED, {0, YCENTER}, TA_CENTER | TA_TOP, {1, 0, 0, 2, WIDTH/8, -HEIGHT/2}, TRUE, L"409.bmp",
        4, {POS(-1, -1), POS(-1, 1), POS(1, -1), POS(1, 1)}, 2, {POS(-3, 3), POS(-3, 1)}},
    {__LINE__, GM_ADVANCED, {0, YCENTER}, TA_CENTER | TA_TOP, {2, 0, 0, 1, WIDTH/4, -HEIGHT/2}, TRUE, L"410.bmp",
        4, {POS(-1, -1), POS(-1, 1), POS(1, -1), POS(1, 1)}, 2, {POS(-3, -3), POS(-1, -3)}},
    {__LINE__, GM_ADVANCED, {0, YCENTER}, TA_CENTER | TA_TOP, {1, 0, 0, 2, WIDTH/8, -HEIGHT/2}, TRUE, L"411.bmp",
        4, {POS(-1, -1), POS(-1, 1), POS(1, -1), POS(1, 1)}, 2, {POS(-3, 1), POS(-3, 3)}},

    // GM_ADVANCED TA_RIGHT TA_TOP
    {__LINE__, GM_ADVANCED, {XCENTER, 0}, TA_RIGHT | TA_TOP, {2, 0, 0, 1, -WIDTH/4, 0}, TRUE, L"500.bmp",
        4, {POS(-1, -1), POS(-1, 1), POS(1, -1), POS(1, 1)}, 2, {POS(-1, -3), POS(1, -3)}},
    {__LINE__, GM_ADVANCED, {XCENTER, 0}, TA_RIGHT | TA_TOP, {1, 0, 0, 2, WIDTH/4, 0}, TRUE, L"501.bmp",
        3, {POS(-1, -1), POS(-1, 1), POS(1, 1)}, 2, {POS(1, -3), POS(1, -1)}},
    {__LINE__, GM_ADVANCED, {XCENTER, 0}, TA_RIGHT | TA_TOP, {2, 0, 0, 2, -WIDTH/4, HEIGHT/4}, TRUE, L"502.bmp",
        0, {POS(0, 0)}, 4, {POS(-1, -1), POS(1, -1), POS(1, -1), POS(-1, -1)}},
    {__LINE__, GM_ADVANCED, {XCENTER, 0}, TA_RIGHT | TA_TOP, {1, 0, 0, 2, WIDTH/4, 0}, TRUE, L"503.bmp",
        3, {POS(-1, 1), POS(1, 1), POS(-1, -1)}, 2, {POS(1, -1), POS(1, -3)}},
    {__LINE__, GM_ADVANCED, {XCENTER, 0}, TA_RIGHT | TA_TOP, {2, 0, 0, 1, -WIDTH/4, HEIGHT/4}, TRUE, L"504.bmp",
        2, {POS(-1, 1), POS(1, 1)}, 2, {POS(-1, -1), POS(1, -1)}},
    {__LINE__, GM_ADVANCED, {XCENTER, 0}, TA_RIGHT | TA_TOP, {1, 0, 0, 2, WIDTH/4, HEIGHT/4}, TRUE, L"505.bmp",
        2, {POS(-1, -1), POS(-1, 1)}, 2, {POS(1, -1), POS(1, 1)}},
    {__LINE__, GM_ADVANCED, {0, YCENTER}, TA_RIGHT | TA_TOP, {2, 0, 0, 1, WIDTH/2, -HEIGHT/2}, TRUE, L"506.bmp",
        4, {POS(-1, -1), POS(-1, 1), POS(1, -1), POS(1, 1)}, 2, {POS(-3, -3), POS(-1, -3)}},
    {__LINE__, GM_ADVANCED, {0, YCENTER}, TA_RIGHT | TA_TOP, {1, 0, 0, 2, WIDTH/2, -HEIGHT/2}, TRUE, L"507.bmp",
        3, {POS(-1, -1), POS(1, -1), POS(1, 1)}, 2, {POS(-1, 3), POS(-1, 1)}},
    {__LINE__, GM_ADVANCED, {0, YCENTER}, TA_RIGHT | TA_TOP, {2, 0, 0, 2, WIDTH, -HEIGHT/2}, TRUE, L"508.bmp",
        3, {POS(-1, -1), POS(-1, 1), POS(1, -1)}, 4, {POS(1, 1), POS(3, 1), POS(1, 3), POS(3, 3)}},
    {__LINE__, GM_ADVANCED, {0, YCENTER}, TA_RIGHT | TA_TOP, {1, 0, 0, 2, WIDTH/2, -HEIGHT/2}, TRUE, L"509.bmp",
        3, {POS(-1, -1), POS(1, -1), POS(1, 1)}, 2, {POS(-1, 1), POS(-1, 3)}},
    {__LINE__, GM_ADVANCED, {0, YCENTER}, TA_RIGHT | TA_TOP, {2, 0, 0, 1, WIDTH/2, -HEIGHT/4}, TRUE, L"510.bmp",
        3, {POS(-1, 1), POS(1, -1), POS(1, 1)}, 2, {POS(-3, -1), POS(-1, -1)}},
    {__LINE__, GM_ADVANCED, {0, YCENTER}, TA_RIGHT | TA_TOP, {1, 0, 0, 2, WIDTH/2, -HEIGHT/2}, TRUE, L"511.bmp",
        3, {POS(-1, -1), POS(1, -1), POS(1, 1)}, 2, {POS(-1, 1), POS(-1, 3)}},
};
static const INT s_entry_count = (INT)(sizeof(s_entries) / sizeof(s_entries[0]));

static void DoTestEntry(const TEST_ENTRY *entry, HDC hDC, HBITMAP hbm)
{
    HGDIOBJ hbmOld, hPenOld;
    INT i;
    COLORREF rgb;
    BOOL ret;
    static const WCHAR s_chBlackBox = L'g';

    SetGraphicsMode(hDC, entry->GraphicsMode);

    hbmOld = SelectObject(hDC, hbm);
    {
        ModifyWorldTransform(hDC, NULL, MWT_IDENTITY);

        FillRect(hDC, &s_rc, s_hWhiteBrush);

        hPenOld = SelectObject(hDC, s_hRedPen);
        {
            MoveToEx(hDC, XCENTER / 2, 0, NULL);
            LineTo(hDC, XCENTER / 2, HEIGHT);

            MoveToEx(hDC, XCENTER, 0, NULL);
            LineTo(hDC, XCENTER, HEIGHT);

            MoveToEx(hDC, XCENTER * 3 / 2, 0, NULL);
            LineTo(hDC, XCENTER * 3 / 2, HEIGHT);

            MoveToEx(hDC, 0, YCENTER / 2, NULL);
            LineTo(hDC, WIDTH, YCENTER / 2);

            MoveToEx(hDC, 0, YCENTER, NULL);
            LineTo(hDC, WIDTH, YCENTER);

            MoveToEx(hDC, 0, YCENTER * 3 / 2, NULL);
            LineTo(hDC, WIDTH, YCENTER * 3 / 2);
        }
        SelectObject(hDC, hPenOld);

        ret = SetWorldTransform(hDC, &entry->xform);
        ok(ret == entry->xform_ok, "Line %d: SetWorldTransform returned %d\n", entry->line, ret);

        SetTextAlign(hDC, entry->TextAlign);

        TextOutW(hDC, entry->ptRef.x, entry->ptRef.y, &s_chBlackBox, 1);

        ModifyWorldTransform(hDC, NULL, MWT_IDENTITY);

        for (i = 0; i < entry->cWhitePoints; ++i)
        {
            rgb = GetPixel(hDC, entry->WhitePoints[i].x, entry->WhitePoints[i].y);
            ok(rgb == WHITE, "Line %d: %d: (%ld, %ld) is not white\n", entry->line, i,
               entry->WhitePoints[i].x, entry->WhitePoints[i].y);
        }

        for (i = 0; i < entry->cBlackPoints; ++i)
        {
            rgb = GetPixel(hDC, entry->BlackPoints[i].x, entry->BlackPoints[i].y);
            ok(rgb == BLACK, "Line %d: %d: (%ld, %ld) is not black\n", entry->line, i,
               entry->BlackPoints[i].x, entry->BlackPoints[i].y);
        }
    }
    SelectObject(hDC, hbmOld);

    SaveBitmapToFile(entry->filename, hbm);
}

START_TEST(TextTransform)
{
    HDC hDC;
    BITMAPINFO bmi;
    LPVOID pvBits;
    HBITMAP hbm;
    LOGFONTA lf;
    HFONT hFont;
    HGDIOBJ hFontOld;
    INT i;

    s_hWhiteBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
    s_hRedPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));

    hDC = CreateCompatibleDC(NULL);
    ok(hDC != NULL, "hDC was NULL.\n");

    SetBkMode(hDC, TRANSPARENT);
    SetMapMode(hDC, MM_ANISOTROPIC);

    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = WIDTH;
    bmi.bmiHeader.biHeight = HEIGHT;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    hbm = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    ok(hbm != NULL, "hbm was NULL.\n");

    ZeroMemory(&lf, sizeof(lf));
    lf.lfHeight = LFHEIGHT;
    lf.lfCharSet = DEFAULT_CHARSET;
    lstrcpyA(lf.lfFaceName, "Marlett");
    hFont = CreateFontIndirectA(&lf);
    ok(hFont != NULL, "hFont was NULL.\n");

    hFontOld = SelectObject(hDC, hFont);
    for (i = 0; i < s_entry_count; ++i)
    {
        DoTestEntry(&s_entries[i], hDC, hbm);
    }
    SelectObject(hDC, hFontOld);

    DeleteObject(hFont);
    DeleteObject(hbm);

    DeleteObject(s_hWhiteBrush);
    DeleteObject(s_hRedPen);

    DeleteDC(hDC);
}

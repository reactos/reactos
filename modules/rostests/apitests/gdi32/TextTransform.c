/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for World Transformation and font rendering
 * PROGRAMMERS:     Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

//#define DEBUGGING

typedef struct tagBITMAPINFOEX
{
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD          bmiColors[256];
} BITMAPINFOEX, FAR *LPBITMAPINFOEX;

#ifdef DEBUGGING
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
    LONG lfWidth;           // LOGFONT.lfWidth
    LONG lfEscapement;      // LOGFONT.lfEscapement
    INT GraphicsMode;       // GM_COMPATIBLE or GM_ADVANCED
    POINT ptRef;            // reference point
    INT TextAlign;
    XFORM xform;
    BOOL xform_ok;
    BYTE aBlacks[16];
} TEST_ENTRY;

#define WIDTH 200
#define HEIGHT 200
#define XCENTER (WIDTH / 2)
#define YCENTER (HEIGHT / 2)
#define BLACK RGB(0, 0, 0)
#define WHITE RGB(255, 255, 255)
#define LFHEIGHT -50
#define LFWIDTH1 100
#define LFESCAPE1 (-90 * 10)

static const RECT s_rc = {0, 0, WIDTH, HEIGHT};
static HBRUSH s_hWhiteBrush = NULL;
static HPEN s_hRedPen = NULL;

static const TEST_ENTRY s_entries[] =
{
    // GM_COMPATIBLE TA_BOTTOM
    {__LINE__, 0, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 0, 0, 1, 0, 0}, FALSE, {7, 8}},
    {__LINE__, 0, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {-1, 0, 0, 1, 0, 0}, FALSE, {7, 8}},
    {__LINE__, 0, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 0, 0, -1, 0, 0}, FALSE, {7, 8}},
    {__LINE__, 0, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 1, 1, 0, 0, 0}, FALSE, {7, 8}},
    {__LINE__, 0, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, -1, -1, 0, 0, 0}, FALSE, {7, 8}},
    {__LINE__, 0, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 1, 1, 1, 0, 0}, FALSE, {7, 8}},
    {__LINE__, 0, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 0, 0, 0, 0, 0}, FALSE, {7, 8}},
    {__LINE__, LFWIDTH1, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 0, 0, 1, 0, 0}, FALSE, {7, 8}},
    {__LINE__, LFWIDTH1, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {-1, 0, 0, 1, 0, 0}, FALSE, {7, 8}},
    {__LINE__, LFWIDTH1, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 0, 0, -1, 0, 0}, FALSE, {7, 8}},
    {__LINE__, LFWIDTH1, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 1, 1, 0, 0, 0}, FALSE, {7, 8}},
    {__LINE__, LFWIDTH1, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, -1, -1, 0, 0, 0}, FALSE, {7, 8}},
    {__LINE__, LFWIDTH1, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 1, 1, 1, 0, 0}, FALSE, {7, 8}},
    {__LINE__, LFWIDTH1, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 0, 0, 0, 0, 0}, FALSE, {7, 8}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 0, 0, 1, 0, 0}, FALSE, {11, 15}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {-1, 0, 0, 1, 0, 0}, FALSE, {11, 15}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 0, 0, -1, 0, 0}, FALSE, {11, 15}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 1, 1, 0, 0, 0}, FALSE, {11, 15}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, -1, -1, 0, 0, 0}, FALSE, {11, 15}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 1, 1, 1, 0, 0}, FALSE, {11, 15}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 0, 0, 0, 0, 0}, FALSE, {11, 15}},
    {__LINE__, 0, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 0, 0, 1, 0, 0}, FALSE, {11, 15}},
    {__LINE__, 0, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {-1, 0, 0, 1, 0, 0}, FALSE, {11, 15}},
    {__LINE__, 0, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 0, 0, -1, 0, 0}, FALSE, {11, 15}},
    {__LINE__, 0, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 1, 1, 0, 0, 0}, FALSE, {11, 15}},
    {__LINE__, 0, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, -1, -1, 0, 0, 0}, FALSE, {11, 15}},
    {__LINE__, 0, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 1, 1, 1, 0, 0}, FALSE, {11, 15}},
    {__LINE__, 0, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 0, 0, 0, 0, 0}, FALSE, {11, 15}},

    // GM_COMPATIBLE TA_TOP
    {__LINE__, 0, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {1, 0, 0, 1, 0, 0}, FALSE, {11, 12}},
    {__LINE__, 0, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {-1, 0, 0, 1, 0, 0}, FALSE, {11, 12}},
    {__LINE__, 0, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {1, 0, 0, -1, 0, 0}, FALSE, {11, 12}},
    {__LINE__, 0, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {0, 1, 1, 0, 0, 0}, FALSE, {11, 12}},
    {__LINE__, 0, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {0, -1, -1, 0, 0, 0}, FALSE, {11, 12}},
    {__LINE__, 0, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {1, 1, 1, 1, 0, 0}, FALSE, {11, 12}},
    {__LINE__, 0, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {0, 0, 0, 0, 0, 0}, FALSE, {11, 12}},
    {__LINE__, LFWIDTH1, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {1, 0, 0, 1, 0, 0}, FALSE, {11, 12}},
    {__LINE__, LFWIDTH1, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {-1, 0, 0, 1, 0, 0}, FALSE, {11, 12}},
    {__LINE__, LFWIDTH1, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {1, 0, 0, -1, 0, 0}, FALSE, {11, 12}},
    {__LINE__, LFWIDTH1, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {0, 1, 1, 0, 0, 0}, FALSE, {11, 12}},
    {__LINE__, LFWIDTH1, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {0, -1, -1, 0, 0, 0}, FALSE, {11, 12}},
    {__LINE__, LFWIDTH1, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {1, 1, 1, 1, 0, 0}, FALSE, {11, 12}},
    {__LINE__, LFWIDTH1, 0, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {0, 0, 0, 0, 0, 0}, FALSE, {11, 12}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {1, 0, 0, 1, 0, 0}, FALSE, {10, 14}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {-1, 0, 0, 1, 0, 0}, FALSE, {10, 14}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {1, 0, 0, -1, 0, 0}, FALSE, {10, 14}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {0, 1, 1, 0, 0, 0}, FALSE, {10, 14}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {0, -1, -1, 0, 0, 0}, FALSE, {10, 14}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {1, 1, 1, 1, 0, 0}, FALSE, {10, 14}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {0, 0, 0, 0, 0, 0}, FALSE, {10, 14}},
    {__LINE__, 0, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {1, 0, 0, 1, 0, 0}, FALSE, {10, 14}},
    {__LINE__, 0, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {-1, 0, 0, 1, 0, 0}, FALSE, {10, 14}},
    {__LINE__, 0, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {1, 0, 0, -1, 0, 0}, FALSE, {10, 14}},
    {__LINE__, 0, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {0, 1, 1, 0, 0, 0}, FALSE, {10, 14}},
    {__LINE__, 0, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {0, -1, -1, 0, 0, 0}, FALSE, {10, 14}},
    {__LINE__, 0, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {1, 1, 1, 1, 0, 0}, FALSE, {10, 14}},
    {__LINE__, 0, LFESCAPE1, GM_COMPATIBLE, {XCENTER, YCENTER}, TA_LEFT | TA_TOP, {0, 0, 0, 0, 0, 0}, FALSE, {10, 14}},

    // GM_ADVANCED TA_LEFT TA_BOTTOM
    {__LINE__, 0, 0, GM_ADVANCED, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 0, 0, 1, 0, 0}, TRUE, {7, 8}},
    {__LINE__, 0, 0, GM_ADVANCED, {-XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {-1, 0, 0, 1, 0, 0}, TRUE, {5, 6}},
    {__LINE__, 0, 0, GM_ADVANCED, {XCENTER, -YCENTER}, TA_LEFT | TA_BOTTOM, {1, 0, 0, -1, 0, 0}, TRUE, {11, 12}},
    {__LINE__, 0, 0, GM_ADVANCED, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 1, 1, 0, 0, 0}, TRUE, {10, 14}},
    {__LINE__, 0, 0, GM_ADVANCED, {-XCENTER, -YCENTER}, TA_LEFT | TA_BOTTOM, {0, -1, -1, 0, 0, 0}, TRUE, {3, 7}},
    {__LINE__, 0, 0, GM_ADVANCED, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 1, 1, 1, 0, 0}, FALSE, {7, 8}},
    {__LINE__, 0, 0, GM_ADVANCED, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 0, 0, 0, 0, 0}, FALSE, {7, 8}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 0, 0, 1, 0, 0}, TRUE, {7, 8}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {-XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {-1, 0, 0, 1, 0, 0}, TRUE, {5, 6}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {XCENTER, -YCENTER}, TA_LEFT | TA_BOTTOM, {1, 0, 0, -1, 0, 0}, TRUE, {11, 12}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 1, 1, 0, 0, 0}, TRUE, {10, 14}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {-XCENTER, -YCENTER}, TA_LEFT | TA_BOTTOM, {0, -1, -1, 0, 0, 0}, TRUE, {3, 7}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 1, 1, 1, 0, 0}, FALSE, {7, 8}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 0, 0, 0, 0, 0}, FALSE, {7, 8}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 0, 0, 1, 0, 0}, TRUE, {10, 11, 14, 15}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {-XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {-1, 0, 0, 1, 0, 0}, TRUE, {10, 11, 14, 15}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {XCENTER, -YCENTER}, TA_LEFT | TA_BOTTOM, {1, 0, 0, -1, 0, 0}, TRUE, {2, 3, 6, 7}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 1, 1, 0, 0, 0}, TRUE, {7, 8, 11, 12}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {-XCENTER, -YCENTER}, TA_LEFT | TA_BOTTOM, {0, -1, -1, 0, 0, 0}, TRUE, {5, 6, 9, 10}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 1, 1, 1, 0, 0}, FALSE, {10, 11, 14, 15}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 0, 0, 0, 0, 0}, FALSE, {10, 11, 14, 15}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 0, 0, 1, WIDTH/8, 0}, TRUE, {11, 15}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {-XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {-1, 0, 0, 1, WIDTH/8, 0}, TRUE, {11, 15}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {XCENTER, -YCENTER}, TA_LEFT | TA_BOTTOM, {1, 0, 0, -1, WIDTH/8, 0}, TRUE, {3, 7}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {XCENTER, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 1, 1, 0, 0, HEIGHT/8}, TRUE, {11, 12}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {XCENTER, -YCENTER}, TA_LEFT | TA_BOTTOM, {0, 1, -1, 0, 0, HEIGHT/8}, TRUE, {9, 10}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {-XCENTER, -YCENTER}, TA_LEFT | TA_BOTTOM, {0, -1, -1, 0, 0, HEIGHT/8}, TRUE, {9, 10}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {XCENTER + WIDTH/8, YCENTER}, TA_LEFT | TA_BOTTOM, {1, 1, 1, 1, 0, 0}, FALSE, {11, 15}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {XCENTER - WIDTH/8, YCENTER}, TA_LEFT | TA_BOTTOM, {0, 0, 0, 0, 0, 0}, FALSE, {10, 14}},

    // GM_ADVANCED TA_LEFT TA_TOP
    {__LINE__, 0, 0, GM_ADVANCED, {0, 0}, TA_LEFT | TA_TOP, {2, 0, 0, 1, 0, 0}, TRUE, {1, 2, 3, 4}},
    {__LINE__, 0, 0, GM_ADVANCED, {0, 0}, TA_LEFT | TA_TOP, {1, 0, 0, 2, 0, 0}, TRUE, {1, 2, 5, 6}},
    {__LINE__, 0, 0, GM_ADVANCED, {0, 0}, TA_LEFT | TA_TOP, {2, 0, 0, 1, WIDTH/4, 0}, TRUE, {2, 3, 4}},
    {__LINE__, 0, 0, GM_ADVANCED, {0, 0}, TA_LEFT | TA_TOP, {2, 0, 0, 1, 0, HEIGHT/4}, TRUE, {5, 6, 7, 8}},
    {__LINE__, 0, 0, GM_ADVANCED, {0, 0}, TA_LEFT | TA_TOP, {1, 0, 0, 2, 0, HEIGHT/4}, TRUE, {5, 6, 9, 10}},
    {__LINE__, 0, 0, GM_ADVANCED, {0, YCENTER}, TA_LEFT | TA_TOP, {2, 0, 0, 1, 0, 0}, TRUE, {9, 10, 11, 12}},
    {__LINE__, 0, 0, GM_ADVANCED, {0, YCENTER}, TA_LEFT | TA_TOP, {2, 0, 0, 1, WIDTH/4, 0}, TRUE, {10, 11, 12}},
    {__LINE__, 0, 0, GM_ADVANCED, {0, YCENTER}, TA_LEFT | TA_TOP, {1, 0, 0, 2, WIDTH/4, -HEIGHT/2}, TRUE, {10, 11, 14, 15}},
    {__LINE__, 0, 0, GM_ADVANCED, {0, YCENTER}, TA_LEFT | TA_TOP, {2, 0, 0, 1, 0, HEIGHT/4}, TRUE, {13, 14, 15, 16}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {0, 0}, TA_LEFT | TA_TOP, {1.25, 0, 0, 1, 0, 0}, TRUE, {1, 2, 4}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {0, 0}, TA_LEFT | TA_TOP, {1, 0, 0, 2, 0, 0}, TRUE, {1, 2, 3, 4, 5, 6, 7, 8}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {0, 0}, TA_LEFT | TA_TOP, {1.25, 0, 0, 1, WIDTH/4, 0}, TRUE, {2, 3}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {0, 0}, TA_LEFT | TA_TOP, {1.25, 0, 0, 1, 0, HEIGHT/4}, TRUE, {5, 6, 8}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {0, 0}, TA_LEFT | TA_TOP, {1, 0, 0, 2, 0, HEIGHT/4}, TRUE, {5, 6, 7, 8, 9, 10, 11, 12}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {0, YCENTER}, TA_LEFT | TA_TOP, {1.25, 0, 0, 1, 0, 0}, TRUE, {9, 10, 12}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {0, YCENTER}, TA_LEFT | TA_TOP, {1.25, 0, 0, 1, WIDTH/4, 0}, TRUE, {10, 11}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {0, YCENTER}, TA_LEFT | TA_TOP, {1, 0, 0, 2, WIDTH/4, -HEIGHT/2}, TRUE, {10, 11, 12, 14, 15, 16}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {0, YCENTER}, TA_LEFT | TA_TOP, {1.25, 0, 0, 1, 0, HEIGHT/4}, TRUE, {13, 14, 16}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {0, 0}, TA_LEFT | TA_TOP, {1.25, 0, 0, 1, 0, 0}, TRUE, {5, 9}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {0, 0}, TA_LEFT | TA_TOP, {1, 0, 0, 2, 0, 0}, TRUE, {9, 13}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {0, 0}, TA_LEFT | TA_TOP, {1.25, 0, 0, 1, WIDTH/4, 0}, TRUE, {5, 6, 9, 10}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {0, 0}, TA_LEFT | TA_TOP, {1.25, 0, 0, 1, 0, HEIGHT/4}, TRUE, {9, 13}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {0, 0}, TA_LEFT | TA_TOP, {1, 0, 0, 2, 0, HEIGHT/4}, TRUE, {13}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {0, YCENTER}, TA_LEFT | TA_TOP, {1.25, 0, 0, 1, 0, 0}, TRUE, {13}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {0, YCENTER}, TA_LEFT | TA_TOP, {1.25, 0, 0, 1, WIDTH/4, 0}, TRUE, {13, 14}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {0, 0}, TA_LEFT | TA_TOP, {2, 0, 0, 1, 0, 0}, TRUE, {5, 9}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {0, 0}, TA_LEFT | TA_TOP, {2, 0, 0, 1, WIDTH/4, 0}, TRUE, {5, 6, 9, 10}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {0, 0}, TA_LEFT | TA_TOP, {2, 0, 0, 1, 0, HEIGHT/4}, TRUE, {9, 13}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {0, 0}, TA_LEFT | TA_TOP, {1, 0, 0, 2, WIDTH/8, HEIGHT/4}, TRUE, {13}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {0, YCENTER}, TA_LEFT | TA_TOP, {2, 0, 0, 1, WIDTH/4, 0}, TRUE, {13, 14}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {0, YCENTER}, TA_LEFT | TA_TOP, {1.25, 0, 0, 1, WIDTH/8, 0}, TRUE, {13}},

    // GM_ADVANCED TA_CENTER TA_TOP
    {__LINE__, 0, 0, GM_ADVANCED, {XCENTER, 0}, TA_CENTER | TA_TOP, {2, 0, 0, 1, -WIDTH/4, 0}, TRUE, {2, 3, 4}},
    {__LINE__, 0, 0, GM_ADVANCED, {XCENTER, 0}, TA_CENTER | TA_TOP, {1, 0, 0, 2, -WIDTH/4, 0}, TRUE, {1, 2, 5, 6}},
    {__LINE__, 0, 0, GM_ADVANCED, {XCENTER, 0}, TA_CENTER | TA_TOP, {1, 0, 0, 2, -WIDTH/4, HEIGHT/4}, TRUE, {5, 6, 9, 10}},
    {__LINE__, 0, 0, GM_ADVANCED, {XCENTER, 0}, TA_CENTER | TA_TOP, {2, 0, 0, 1, -WIDTH/4, HEIGHT/4}, TRUE, {6, 7, 8}},
    {__LINE__, 0, 0, GM_ADVANCED, {XCENTER, 0}, TA_CENTER | TA_TOP, {1, 0, 0, 2, -WIDTH/4, HEIGHT/8}, TRUE, {5, 6}},
    {__LINE__, 0, 0, GM_ADVANCED, {XCENTER/2, YCENTER}, TA_CENTER | TA_TOP, {2, 0, 0, 2, WIDTH/4, -HEIGHT/2}, TRUE, {10, 11, 12, 14, 15, 16}},
    {__LINE__, 0, 0, GM_ADVANCED, {XCENTER/2, YCENTER/2}, TA_CENTER | TA_TOP, {1, 0, 0, 2, 0, -HEIGHT/2}, TRUE, {1, 2, 5, 6}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {XCENTER, 0}, TA_CENTER | TA_TOP, {2, 0, 0, 1, WIDTH/8, 0}, TRUE, {2, 3, 4}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {XCENTER, 0}, TA_CENTER | TA_TOP, {1, 0, 0, 2, WIDTH/8, 0}, TRUE, {2, 4, 6, 8}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {XCENTER, 0}, TA_CENTER | TA_TOP, {1.25, 0, 0, 1, -WIDTH/4, HEIGHT/4}, TRUE, {5, 7, 8}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {XCENTER, 0}, TA_CENTER | TA_TOP, {1, 0, 0, 2, WIDTH/8, HEIGHT/4}, TRUE, {6, 8, 10, 12}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {0, YCENTER}, TA_CENTER | TA_TOP, {1.25, 0, 0, 2, WIDTH/8, -HEIGHT/2}, TRUE, {10, 11, 14, 15}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {0, YCENTER}, TA_CENTER | TA_TOP, {1, 0, 0, 2, WIDTH/4, -HEIGHT/2}, TRUE, {9, 10, 11, 13, 14, 15}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {XCENTER, 0}, TA_CENTER | TA_TOP, {2, 0, 0, 1, -WIDTH/8, 0}, TRUE, {3, 4, 7, 8}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {XCENTER, 0}, TA_CENTER | TA_TOP, {1, 0, 0, 1.25, 0, 0}, TRUE, {2, 3, 6, 7}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {XCENTER, 0}, TA_CENTER | TA_TOP, {1, 0, 0, 2, 0, 0}, TRUE, {2, 3, 6, 7, 10, 11, 14, 15}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {XCENTER, 0}, TA_CENTER | TA_TOP, {1.25, 0, 0, 1, -WIDTH/8, HEIGHT/4}, TRUE, {6, 7, 10, 11}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {XCENTER, 0}, TA_CENTER | TA_TOP, {1, 0, 0, 1.25, WIDTH/8, HEIGHT/4}, TRUE, {7, 11}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {XCENTER, 0}, TA_CENTER | TA_TOP, {2, 0, 0, 1, -WIDTH/4, 0}, TRUE, {3, 4, 7, 8}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {XCENTER, 0}, TA_CENTER | TA_TOP, {1, 0, 0, 2, WIDTH/8, HEIGHT/8}, TRUE, {7, 15}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {XCENTER, 0}, TA_CENTER | TA_TOP, {1, 0, 0, 2, WIDTH/8, -HEIGHT/8}, TRUE, {3, 11}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {XCENTER, 0}, TA_CENTER | TA_TOP, {2, 0, 0, 1, -WIDTH/4, HEIGHT/4}, TRUE, {7, 8, 11, 12}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {XCENTER, 0}, TA_CENTER | TA_TOP, {1, 0, 0, 1, WIDTH/8, HEIGHT/4}, TRUE, {7, 11}},

    // GM_ADVANCED TA_RIGHT TA_TOP
    {__LINE__, 0, 0, GM_ADVANCED, {XCENTER, 0}, TA_RIGHT | TA_TOP, {2, 0, 0, 1, -WIDTH/4, 0}, TRUE, {1, 2, 3}},
    {__LINE__, 0, 0, GM_ADVANCED, {XCENTER, 0}, TA_RIGHT | TA_TOP, {2, 0, 0, 1, -WIDTH/4, HEIGHT/4}, TRUE, {5, 6, 7}},
    {__LINE__, 0, 0, GM_ADVANCED, {XCENTER, 0}, TA_RIGHT | TA_TOP, {1, 0, 0, 2, WIDTH/4, HEIGHT/4}, TRUE, {6, 7, 10, 11}},
    {__LINE__, 0, 0, GM_ADVANCED, {0, YCENTER}, TA_RIGHT | TA_TOP, {2, 0, 0, 1, WIDTH/2, -HEIGHT/2}, TRUE, {1, 2}},
    {__LINE__, 0, 0, GM_ADVANCED, {0, YCENTER}, TA_RIGHT | TA_TOP, {2, 0, 0, 2, WIDTH, -HEIGHT/2}, TRUE, {9, 10, 11, 12, 13, 14, 15, 16}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {XCENTER, 0}, TA_RIGHT | TA_TOP, {1.25, 0, 0, 1, -WIDTH/4, 0}, TRUE, {1}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {XCENTER, 0}, TA_RIGHT | TA_TOP, {1.25, 0, 0, 1, -WIDTH/4, HEIGHT/4}, TRUE, {5}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {XCENTER, 0}, TA_RIGHT | TA_TOP, {1, 0, 0, 2, WIDTH/4, HEIGHT/4}, TRUE, {5, 6, 7, 9, 10, 11}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {0, YCENTER}, TA_RIGHT | TA_TOP, {1.25, 0, 0, 1, WIDTH/2, -HEIGHT/2}, TRUE, {1, 2}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {0, YCENTER}, TA_RIGHT | TA_TOP, {1.25, 0, 0, 2, WIDTH, -HEIGHT/2}, TRUE, {9, 11, 12, 13, 15, 16}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {XCENTER, 0}, TA_RIGHT | TA_TOP, {1.25, 0, 0, 1, -WIDTH/8, 0}, TRUE, {2, 3}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {XCENTER, 0}, TA_RIGHT | TA_TOP, {1.25, 0, 0, 1, -WIDTH/8, HEIGHT/4}, TRUE, {2, 3, 6, 7}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {XCENTER, 0}, TA_RIGHT | TA_TOP, {1, 0, 0, 1.25, WIDTH/4, HEIGHT/4}, TRUE, {3, 4, 7, 8}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {0, YCENTER}, TA_RIGHT | TA_TOP, {1.25, 0, 0, 1, WIDTH/2, -HEIGHT/4}, TRUE, {2, 3, 6, 7}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {0, YCENTER}, TA_RIGHT | TA_TOP, {1.25, 0, 0, 1.25, WIDTH, -HEIGHT/8}, TRUE, {8, 12}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {XCENTER, 0}, TA_RIGHT | TA_TOP, {2, 0, 0, 1, -WIDTH/4, 0}, TRUE, {3, 4}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {XCENTER, 0}, TA_RIGHT | TA_TOP, {2, 0, 0, 1, -WIDTH/4, HEIGHT/4}, TRUE, {3, 4, 7, 8}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {XCENTER, 0}, TA_RIGHT | TA_TOP, {1, 0, 0, 1.25, WIDTH/8, HEIGHT/4}, TRUE, {3, 7}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {0, YCENTER}, TA_RIGHT | TA_TOP, {2, 0, 0, 1, WIDTH/2, -HEIGHT/2}, TRUE, {2, 3}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {0, YCENTER}, TA_RIGHT | TA_TOP, {2, 0, 0, 1.25, WIDTH, -HEIGHT/8}, TRUE, {8, 12}},

    // GM_ADVANCED TA_BASELINE TA_BASELINE
    {__LINE__, 0, 0, GM_ADVANCED, {XCENTER, 0}, TA_RIGHT | TA_BASELINE, {2, 0, 0, 1, -WIDTH/4, HEIGHT/4}, TRUE, {1, 2, 3}},
    {__LINE__, 0, 0, GM_ADVANCED, {XCENTER, 0}, TA_RIGHT | TA_BASELINE, {1, 0, 0, 2, WIDTH/4, HEIGHT/4}, TRUE, {2, 3}},
    {__LINE__, 0, 0, GM_ADVANCED, {0, YCENTER}, TA_RIGHT | TA_BASELINE, {2, 0, 0, 2, WIDTH, -HEIGHT/2}, TRUE, {1, 2, 3, 4, 5, 6, 7, 8}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {XCENTER, 0}, TA_RIGHT | TA_BASELINE, {1.25, 0, 0, 1, -WIDTH/4, HEIGHT/4}, TRUE, {1}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {XCENTER, 0}, TA_RIGHT | TA_BASELINE, {1, 0, 0, 2, WIDTH/4, HEIGHT/4}, TRUE, {1, 2, 3}},
    {__LINE__, LFWIDTH1, 0, GM_ADVANCED, {0, YCENTER}, TA_RIGHT | TA_BASELINE, {1.25, 0, 0, 2, WIDTH, -HEIGHT/2}, TRUE, {1, 3, 4, 5, 7, 8}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {XCENTER, 0}, TA_RIGHT | TA_BASELINE, {2, 0, 0, 1, -WIDTH/8, HEIGHT/4}, TRUE, {3, 4}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {XCENTER, 0}, TA_RIGHT | TA_BASELINE, {1, 0, 0, 1.25, WIDTH/8, HEIGHT/4}, TRUE, {3}},
    {__LINE__, LFWIDTH1, LFESCAPE1, GM_ADVANCED, {0, YCENTER}, TA_RIGHT | TA_BASELINE, {1.25, 0, 0, 2, WIDTH, -HEIGHT/2}, TRUE, {4, 8}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {XCENTER, 0}, TA_RIGHT | TA_BASELINE, {2, 0, 0, 1, -WIDTH/4, HEIGHT/4}, TRUE, {3, 4}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {0, YCENTER}, TA_RIGHT | TA_BASELINE, {1.25, 0, 0, 1, WIDTH/8, HEIGHT/4}, TRUE, {5, 9}},
    {__LINE__, 0, LFESCAPE1, GM_ADVANCED, {0, YCENTER}, TA_RIGHT | TA_BASELINE, {2, 0, 0, 2, 0, -HEIGHT/8}, TRUE, {1, 9}},
};

static void DoTestEntry(const TEST_ENTRY *entry, HDC hDC, HBITMAP hbm)
{
    HGDIOBJ hbmOld, hPenOld;
    INT i, j;
    BOOL ret;
    static const WCHAR s_szBlackBoxes[] = L"gg";
    LOGFONTA lf;
    HFONT hFont;
    HGDIOBJ hFontOld;

    ZeroMemory(&lf, sizeof(lf));
    lf.lfHeight = LFHEIGHT;
    lf.lfWidth = entry->lfWidth;
    lf.lfEscapement = entry->lfEscapement;
    lf.lfCharSet = DEFAULT_CHARSET;
    lstrcpyA(lf.lfFaceName, "Marlett");
    hFont = CreateFontIndirectA(&lf);
    ok(hFont != NULL, "hFont was NULL.\n");
    hFontOld = SelectObject(hDC, hFont);

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

        TextOutW(hDC, entry->ptRef.x, entry->ptRef.y, s_szBlackBoxes, lstrlenW(s_szBlackBoxes));

        ModifyWorldTransform(hDC, NULL, MWT_IDENTITY);

#define POS(ix, iy) {(ix) * WIDTH/4 + WIDTH/8, (iy) * HEIGHT/4 + HEIGHT/8}

        for (j = 0; j < 4; ++j)
        {
            for (i = 0; i < 4; ++i)
            {
                POINT pt = POS(i, j);
                INT k = j * 4 + i + 1;
                COLORREF rgb = GetPixel(hDC, pt.x, pt.y);
                BOOL bFound = FALSE;
                INT m;
                for (m = 0; m < _countof(entry->aBlacks); ++m)
                {
                    if (entry->aBlacks[m] == 0)
                        break;
                    if (entry->aBlacks[m] == k)
                    {
                        bFound = TRUE;
                        break;
                    }
                }

                if (bFound)
                {
                    ok(rgb == RGB(0, 0, 0), "Line %d: It was not black (k: %d)\n", entry->line, k);
                }
                else
                {
                    ok(rgb != RGB(0, 0, 0), "Line %d: It was black (k: %d)\n", entry->line, k);
                }

#ifdef DEBUGGING
                {
                    WCHAR sz[8];
                    RECT rc;
                    COLORREF colorOld;
                    HGDIOBJ hFontOld = SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
                    if (rgb == RGB(0, 0, 0))
                    {
                        if (!bFound)
                            printf("Line %d: k: %d\n", entry->line, k);
                    }
                    Ellipse(hDC, pt.x - 10, pt.y - 10, pt.x + 10, pt.y + 10);
                    SetRect(&rc, pt.x - 10, pt.y - 10, pt.x + 10, pt.y + 10);
                    StringCchPrintfW(sz, _countof(sz), L"%d", k);
                    colorOld = SetTextColor(hDC, RGB(255, 0, 0));
                    SetTextAlign(hDC, TA_LEFT | TA_TOP | TA_NOUPDATECP);
                    DrawTextW(hDC, sz, -1, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
                    SetTextColor(hDC, colorOld);
                    SelectObject(hDC, hFontOld);
                }
#endif
            }
        }
    }
    SelectObject(hDC, hbmOld);

#ifdef DEBUGGING
    {
        WCHAR szFileName[MAX_PATH];
        StringCchPrintfW(szFileName, _countof(szFileName), L"Line%04u.bmp", entry->line);
        SaveBitmapToFile(szFileName, hbm);
    }
#endif

    SelectObject(hDC, hFontOld);
    DeleteObject(hFont);
}

START_TEST(TextTransform)
{
    HDC hDC;
    BITMAPINFO bmi;
    LPVOID pvBits;
    HBITMAP hbm;
    UINT i;

    s_hWhiteBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
    s_hRedPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));

    hDC = CreateCompatibleDC(NULL);
    ok(hDC != NULL, "hDC was NULL.\n");

    SetBkMode(hDC, TRANSPARENT);

    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = WIDTH;
    bmi.bmiHeader.biHeight = HEIGHT;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    hbm = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    ok(hbm != NULL, "hbm was NULL.\n");

    for (i = 0; i < _countof(s_entries); ++i)
    {
        DoTestEntry(&s_entries[i], hDC, hbm);
    }

    DeleteObject(hbm);

    DeleteObject(s_hWhiteBrush);
    DeleteObject(s_hRedPen);

    DeleteDC(hDC);
}

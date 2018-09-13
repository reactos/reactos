#include <windows.h>
#include <win32.h>  //This is needed for both win16/32 versions
#include "ddsucks.h"
#include "drawdibi.h"
#ifdef WIN32
#include "mmddk.h"
#endif

//#ifdef DEBUG
//static void cdecl dprintf(LPSTR szFormat, ...);
//#define DPF(x) dprintf x
//#else
//#define DPF(x)
//#endif

/**************************************************************************
**************************************************************************/

/* CreateExplicitPalette()
 */
static HPALETTE CreateExplicitPalette()
{
    int i;

    struct {
	UINT         palVersion;
	UINT         palNumEntries;
	PALETTEENTRY palPalEntry[256];
    }   pal;

    pal.palVersion = 0x300;
    pal.palNumEntries = 256;

    for (i = 0; i < 256; i++)
    {
        pal.palPalEntry[i].peRed   = (BYTE)i;
        pal.palPalEntry[i].peGreen = 0;
        pal.palPalEntry[i].peBlue  = 0;
        pal.palPalEntry[i].peFlags = PC_EXPLICIT;
    }

    return CreatePalette((LPLOGPALETTE)&pal);
}

/***************************************************************************
 *
 * DoesDisplayDriverSuck(void)
 *
 * times the display driver and determines if it "sucks" drawing 8bpp DIBs to
 * the screen.
 *
 * it "sucks" if it can BitBlt() bitmaps *much* faster than it can
 * StretchDIBits() them.
 *
 ***************************************************************************/

#define TEST_SIZE_X     160     // !!!magic numbers....
#define TEST_SIZE_Y     120
#define TEST_COUNT      10

static BOOL NEAR PASCAL DoesDisplayDriverSuck(int bits)
{
    int                 i;
    HDC                 hdc;
    HDC                 hdcMem;
    HBITMAP             hbm;
    HBITMAP             hbmT;
    HPALETTE            hpal=NULL;
    HPALETTE            hpalT;
    LPBITMAPINFOHEADER  lpbi;
    LPVOID              lpBits;
    LPWORD              lpw;
    DWORD               time;
    DWORD               timeDib;
    DWORD               timeBlt;
    DWORD               timeDibBlt;

    extern DWORD FAR PASCAL timeGetTime(void);

    hdc = GetDC(NULL);

    if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
    {
        hpal = CreateExplicitPalette();

        hpalT = SelectPalette(hdc, hpal, FALSE);
        RealizePalette(hdc);
    }

    hdcMem = CreateCompatibleDC(hdc);
    hbm = CreateCompatibleBitmap(hdc, TEST_SIZE_X, TEST_SIZE_Y);
    hbmT = SelectObject(hdcMem, hbm);

    lpbi = (LPVOID)GlobalLock(GlobalAlloc(GHND, sizeof(BITMAPINFOHEADER) + 256*sizeof(RGBQUAD) + (DWORD)TEST_SIZE_X*TEST_SIZE_Y));

    lpbi->biSize            = sizeof(BITMAPINFOHEADER);
    lpbi->biWidth           = TEST_SIZE_X;
    lpbi->biHeight          = TEST_SIZE_Y;
    lpbi->biPlanes          = 1;
    lpbi->biBitCount        = bits;
    lpbi->biCompression     = BI_RGB;
    lpbi->biSizeImage       = (DWORD)TEST_SIZE_X*TEST_SIZE_Y;
    lpbi->biXPelsPerMeter   = 0;
    lpbi->biYPelsPerMeter   = 0;
    lpbi->biClrUsed         = 256;
    lpbi->biClrImportant    = 0;

    lpw    = (LPWORD)(lpbi+1);
    lpBits = (LPBYTE)(lpbi+1) + 256*sizeof(RGBQUAD);

    for (i=0; i<256; i++)
        lpw[i] = i;

    //
    //  grab the screen as the blit test
    //
    BitBlt(hdcMem, 0, 0, TEST_SIZE_X, TEST_SIZE_Y, hdc, 0, 0, SRCCOPY);

    GetDIBits(hdc, hbm, 0, TEST_SIZE_Y,
        lpBits, (LPBITMAPINFO)lpbi, DIB_PAL_COLORS);

#ifdef DEBUG
    //
    //  blt the bitmap back to the screen N times
    //  and time it!
    //
    time = timeGetTime();

    for (i=0; i<TEST_COUNT; i++)
        BitBlt(hdc, 0, 0, TEST_SIZE_X, TEST_SIZE_Y, hdcMem, 0, 0, SRCCOPY);

    timeBlt = timeGetTime() - time;
#endif

    //
    //  draw the DIB directly to the screen
    //
    time = timeGetTime();

    for (i=0; i<TEST_COUNT; i++)
        StretchDIBits(hdc, 0, 0, TEST_SIZE_X, TEST_SIZE_Y,
                    0, 0, TEST_SIZE_X, TEST_SIZE_Y,
                    lpBits, (LPBITMAPINFO)lpbi, DIB_PAL_COLORS, SRCCOPY);

    timeDib = timeGetTime() - time;

    //
    //  convert the DIB to a BITMAP then draw
    //
    time = timeGetTime();

    for (i=0; i<TEST_COUNT; i++)
    {
        SetDIBits(hdc,hbm,0,TEST_SIZE_Y,lpBits,(LPBITMAPINFO)lpbi,DIB_PAL_COLORS);
        BitBlt(hdc, 0, 0, TEST_SIZE_X, TEST_SIZE_Y, hdcMem, 0, 0, SRCCOPY);
    }

    timeDibBlt = timeGetTime() - time;

    //
    //  print the results.
    //

    DPF(("DIB BLT PERFORMANCE (%d bit):", bits));
    DPF(("    BitBlt:             %ldms", timeBlt));
    DPF(("    StretchDIBits:      %ldms", timeDib));
    DPF(("    SetDIBits + BitBlt: %ldms", timeDibBlt));

    if (hpal)
    {
        SelectPalette(hdc, hpalT, FALSE);
        DeleteObject(hpal);
    }

    SelectObject(hdcMem, hbmT);
    DeleteObject(hbm);
    DeleteObject(hdcMem);
    ReleaseDC(NULL, hdc);

    GlobalFree((HGLOBAL)HIWORD((DWORD)lpbi));

    return timeDib > timeDibBlt;
}

/***************************************************************************
 *
 * DoesDisplaySuck(void)
 *
 * times the display driver and determines if it "sucks" drawing 8bpp DIBs to
 * the screen.
 *
 * it "sucks" if it can BitBlt() bitmaps *much* faster than it can
 * StretchDIBits() them.
 *
 ***************************************************************************/

BOOL FAR PASCAL DoesDisplaySuck(void)
{
    char  achDisplay[80];
    char  ach[80];

    static BOOL fDisplayDriverSucks = -1;

    static TCHAR szDisplayDriverSucks[] = TEXT("DisplayDriverBad");
#ifdef WIN16
    static TCHAR szDisplayDrv[]         = TEXT("display.drv");
#else
    static TCHAR szDisplayDrv[]         = TEXT("display.dll");
#endif
    static TCHAR szSystemIni[]          = TEXT("system.ini");
    static TCHAR szMCIAVI[]             = MCIAVI_SECTION;
    static TCHAR szBoot[]               = TEXT("boot");
    static TCHAR szNull[]               = TEXT("");

    if (fDisplayDriverSucks != -1)
        return fDisplayDriverSucks;

    GetPrivateProfileString(szBoot, szDisplayDrv, szNull, achDisplay,
        sizeof(achDisplay), szSystemIni);

    GetProfileString(szMCIAVI, szDisplayDrv, szNull, ach, sizeof(ach));

    if (lstrcmpi(ach, achDisplay) != 0) {
        WriteProfileString(szMCIAVI, szDisplayDrv, achDisplay);
        fDisplayDriverSucks = -1;
    }
    else {
        fDisplayDriverSucks = GetProfileInt(szMCIAVI, szDisplayDriverSucks, -1);
    }

    if (fDisplayDriverSucks == -1) {
#ifdef DEBUG
        DoesDisplayDriverSuck(4);
#endif
        fDisplayDriverSucks = DoesDisplayDriverSuck(8);
        wsprintf(ach, "%d", fDisplayDriverSucks);
        WriteProfileString(szMCIAVI, szDisplayDriverSucks, ach);

        if (fDisplayDriverSucks)
        {
            DPF(("*****************************************************"));
            DPF(("**                                                 **"));
            DPF(("** WARNING YOUR DISPLAY DRIVER SUCKS DOING DIBS    **"));
            DPF(("** TRYING TO DEAL WITH IT.                         **"));
            DPF(("**                                                 **"));
            DPF(("*****************************************************"));
        }
    }

    return fDisplayDriverSucks;
}

#if 0
#ifdef DEBUG

static void cdecl dprintf(LPSTR szFormat, ...)
{
    char ach[128];

#ifdef WIN32
    va_list va;
    va_start(szFormat, va);
    wvsprintfA (ach,szFormat,va);
    va_end(va);

    OutputDebugStringA("DDSUCKS: ");
    OutputDebugStringA(ach);
    OutputDebugStringA("\r\n");
#else
    wvsprintfA (ach,szFormat,(LPSTR)(&szFormat+1));

    OutputDebugStringA("DDSUCKS: ");
    OutputDebugStringA(ach);
    OutputDebugStringA("\r\n");
#endif
}

#endif
#endif

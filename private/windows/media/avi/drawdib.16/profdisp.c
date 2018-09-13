#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>           // for timeGetTime()
#include "drawdibi.h"
#include "profdisp.h"

//#include "msvideo.h"
//#include "lockbm.h"
//#include "setdi.h"

// Remove inline assembly warning
#pragma warning(disable:4704)

//
//  Set+Blt must be N% faster in order to say a driver sucks
//
#define PROFDISP_FUDGE      110

#ifndef WIN32
    #define GdiFlush()
#endif

DWORD VFWAPI DrawDibProfileDisplay(LPBITMAPINFOHEADER lpbi);
static UINT NEAR PASCAL ProfDispCanDrawDib(LPBITMAPINFOHEADER lpbi);

static HPALETTE CreateTestPalette(BOOL);

#ifndef WIN32

//C6 will die if we dont redefine this.
#undef  GlobalFreePtr
#define GlobalFreePtr(p)    GlobalFree(GlobalPtrHandle(p))

#pragma alloc_text(DRAWDIB, DrawDibProfileDisplay)
#pragma alloc_text(DRAWDIB, ProfDispCanDrawDib)
#endif

#ifndef QUERYDIBSUPPORT
    #define QUERYDIBSUPPORT     3073
#endif

#ifndef QDI_SETDIBITS
    #define QDI_SETDIBITS       0x0001
    #define QDI_GETDIBITS       0x0002
    #define QDI_DIBTOSCREEN     0x0004
    #define QDI_STRETCHDIB      0x0008
#endif

/*
** ProfDisp - profile the display driver
*/

#define BITMAP_X    320
#define BITMAP_Y    240
#define N_FRAMES    10
#define STRETCH_N   190         // 1.90 times

#ifdef DEBUG
    #define FIRST_N 0           // do four bit
#else
    #define FIRST_N 1
#endif

#define BITBLTINDEX 5
#define BACKINDEX 6

// Internal return codes from DrawDibTest

#define CANT_DO_THESE_BITS      1
#define CANT_DO_STRETCHDIBITS   2
#define STRETCHDI_FASTER        3
#define OTHER_FASTER            4

static HWND     ghwnd ;

#ifndef WIN32
#define CODE  _based(_segname("_CODE"))
#define STACK _based(_segname("_STACK"))
#else
#define CODE
#define STACK
#endif

static UINT ProfileDisplay(HDC hdc, UINT wBitsToTest, int dx, int dy) ;
static BOOL IsDisplay16Bit(HDC hdc ) ;
static BOOL IsDisplay32Bit(HDC hdc ) ;
static UINT DrawDibTest(HDC hdc, LPBITMAPINFOHEADER FAR *alpbi, UINT wFrames,UINT wStretch ) ;
static void FreeFrames( LPBITMAPINFOHEADER FAR *alpbi) ;
static void MakeFrames(LPBITMAPINFOHEADER FAR *alpbi, UINT bits, UINT wXSize,UINT wYSize ) ;
static HANDLE MakeDib( HBITMAP hbitmap, UINT bits ) ;

static TCHAR CODE szBoot[]        = TEXT("boot" );
static TCHAR CODE szDisplay[]     = TEXT("display.drv" );
static TCHAR CODE szNull[]        = TEXT("") ;
static TCHAR CODE szDrawdib[]     = TEXT("drawdib" );
static TCHAR CODE szSystemIni[]   = TEXT("system.ini") ;
static TCHAR CODE szNxNxNxType[]  = TEXT(" %dx%dx%d(%s%u)");
static TCHAR CODE szEntryFormat[] = TEXT("%d,%d,%d,%d");
static TCHAR CODE szU[]           = TEXT("%u");
static TCHAR CODE sz02U[]         = TEXT("%02u");
static TCHAR CODE sz565[]         = TEXT("565 ");
static TCHAR CODE sz555[]         = TEXT("555 ");
static TCHAR CODE szRGB[]         = TEXT("RGB ");
static TCHAR CODE szBGR[]         = TEXT("BGR ");

// The following two strings are loaded from MSVIDEO.DLL - defined in
// video\video.rc.  If they cannot be found there, use these definitions
static TCHAR CODE szProfilingDefault[]  = TEXT("Profiling Display");
static TCHAR CODE szListbox[]           = TEXT("ListBox");

#if 0   // dont warn any-more
#ifndef WIN32
static TCHAR CODE szWarning[]     = TEXT("Warning!");

static TCHAR CODE szDisplaySucks[]=
            TEXT("You may encounter display performance problems; ")
            TEXT("please contact the manufacturer of your video ")
            TEXT("board to see if an updated driver is available.");
#endif
#endif

#define ARRAYLEN(array)	    (sizeof(array)/sizeof(array[0]))

static int result[5] = {
     -1,
     -1,
     -1,
     -1,
     -1};

//
// UINT displayFPS[7][3][2]     [test dib][stretch][method]
//
// this array contains fps numbers times 10, ie 10 == 1fps
// zero means the test was not run.
//
// testdib:
//      0       = 4bpp  DIB (debug only)
//      1       = 8bpp  DIB
//      2       = 16bpp DIB
//      3       = 24bpp DIB
//      4       = 32bpp DIB
//      5       = BitBlt
//      6       = 8bpp  DIB (with non identity palette)
//
//  stretch:
//      0       = 1:1
//      1       = 1:2
//      2       = 1:N  (realy 2:3)
//
//  method (for stretch == 1:1)
//      0       = StretchDIBits()
//      1       = SetDIBits() + BitBlt  *
//
//  method (for stretch != 1:1)
//      0       = StretchDIBits()
//      1       = StretchDIB() + StretchDIBits() **
//
//  method (for testdib == 5, bitblt)
//      0       = BitBlt foreground palette
//      1       = BitBlt background palette
//
//  NOTE high color dibs (> 8) are not tested on devices with bitdepths <= 8
//
//  NOTE stretching tests are not run unless the device does stretching.
//  (RasterCaps has RC_STRETCHBLT or RC_STRETCHDIBITS set)
//
//  * NOTE if we can access bitmaps, we dont use SetDIBits() we use direct
//    code.
//
//  ** NOTE (StretchDIB is not a GDI api...)
//
//  EXAMPLE:
//      displayFPS[1][0][0] is the FPS of 1:1 StretchDIBits() on a 8bpp DIB
//      displayFPS[1][0][1] is the FPS of Set+BitBlt() on a 8bpp DIB
//      displayFPS[1][1][0] is the FPS of 1:2 StretchDIBits() on a 8bpp DIB
//
//  how the ResultN flags get set:
//
//      PD_CAN_DRAW_DIB (can draw this dib 1:1 using some method...)
//          displayFPS[N][0][0] != 0 or displayFPS[N][0][1] != 0
//
//      PD_CAN_STRETCHDIB (can stretch this dib using StretchDIBits)
//          displayFPS[N][1][0] > displayFPS[N][1][1] or
//          displayFPS[N][2][0] > displayFPS[N][2][1]
//
//      PD_STRETCHDIB_1_1_OK (StretchDIBits faster than Set+BitBlt)
//          displayFPS[N][0][0] > displayFPS[N][0][1]
//
//      PD_STRETCHDIB_1_2_OK (StretchDIBits 1:2 is faster the doing it our self)
//          displayFPS[N][1][0] > displayFPS[N][1][1]
//
//      PD_STRETCHDIB_1_N_OK (StretchDIBits 1:N is faster the doing it our self)
//          displayFPS[N][2][0] > displayFPS[N][2][1]
//

static UINT displayFPS[7]   // 0=4bbp, 1=8bpp, 2=16bpp, 3=24bpp, 4=32bit, 5=BitBlt, 6=Dib ~1:1
                      [3]   // 0=1:1,  1=1:2,  2=1:N
                      [2];  // 0=DrawDib, 1=Set+Blt (or ~1:1 for BitBlt)

/***************************************************************************
 *
 * @doc INTERNAL
 *
 * @api LONG | atoi | local version of atoi
 *
 ***************************************************************************/
 
static int NEAR PASCAL atoi(char FAR *sz)
{
    int i = 0;
    
    while (*sz && *sz >= '0' && *sz <= '9')
    	i = i*10 + *sz++ - '0';
    	
    return i;    	
}	

static void FAR InitProfDisp(BOOL fForceMe)
{
    TCHAR   ach[80];
    TCHAR   achDisplay[80];
    HDC     hdc;
    int     i;
    int     n;
    int     BitDepth;

    GetPrivateProfileString(szBoot, szDisplay, szNull,
			    achDisplay, ARRAYLEN(achDisplay), szSystemIni);

    hdc = GetDC(NULL);

    BitDepth = GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL);

    n = GetBitmapType();

    switch(n & BM_TYPE)
    {
        default:
        case BM_8BIT:    ach[0] = 0;             break;
        case BM_16555:   lstrcpy(ach, sz555);    break;
        case BM_16565:   lstrcpy(ach, sz565);    break;
        case BM_24BGR:
        case BM_32BGR:   lstrcpy(ach, szBGR);    break;
        case BM_24RGB:
        case BM_32RGB:   lstrcpy(ach, szRGB);    break;
    }

    wsprintf(achDisplay + lstrlen(achDisplay), szNxNxNxType,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        BitDepth,(LPSTR)ach, n >> 4);
    ReleaseDC(NULL, hdc);

    GetProfileString(szDrawdib, achDisplay, szNull, ach, ARRAYLEN(ach));
    
    for (i=0,n=1; n<5; n++)
    {
        if (ach[i] == '\0')
        {
            result[n] = -1;
        }
        else
        {
            result[n] = atoi(ach+i);
            while (ach[i] != 0 && ach[i] != ',')
                i++;
            if (ach[i] != 0)
                i++;
	}
    }

    if (fForceMe ||
        result[1] == -1 ||
        result[2] == -1 ||
        result[3] == -1 ||
        result[4] == -1)
    {
        TestDibFormats(BITMAP_X,BITMAP_Y);

	wsprintf(ach, szEntryFormat,
		 result[1], result[2], result[3], result[4]);
        WriteProfileString(szDrawdib, achDisplay, ach);

#if 0
        //
        // if the DISPLAY driver sucks drawing DIBs then warn the user.
        //
        // we will only warn if the device is at least 8bpp
        //
        if (BitDepth >= 8 && !(result[1] & PD_STRETCHDIB_1_1_OK))
        {
#ifndef WIN32
            MessageBox(NULL, szDisplaySucks, szWarning,
#ifdef BIDI
		MB_RTL_READING |
#endif

                MB_OK | MB_SYSTEMMODAL | MB_ICONEXCLAMATION);
#else
            RPF(("Display driver probably too slow for AVI"));
#endif
        }
#endif
    }
}

/****************************************************************
*
*****************************************************************/

static UINT NEAR PASCAL ProfDispCanDrawDib(LPBITMAPINFOHEADER lpbi)
{
    int     n;
    LONG    l;
    HDC     hdc;
    UINT    w;

    if (result[1] == -1 || lpbi==NULL)
        InitProfDisp(FALSE);

    if (lpbi == NULL)
        return 0;

    switch (lpbi->biCompression)
    {
        //
        //  standard format use our pre-computed performance numbers.
        //
        case BI_RGB:
            n = (int)lpbi->biBitCount / 8;
            return result[n];

        case BI_RLE4:
        case BI_RLE8:
            //
            // return the un-rle results *but* RLE can't stretch
            //
            return result[1] & PD_CAN_DRAW_DIB|PD_STRETCHDIB_1_1_OK;

        //
        //  custom format, ask the DISPLAY driver
        //
        default:
            l = 0;
            w = 0;

            hdc = GetDC(NULL);

            if (Escape(hdc, QUERYDIBSUPPORT, (int)lpbi->biSize, (LPVOID)lpbi, (LPVOID)&l) > 0)
            {
                // make sure the driver realy realy gave us back flags.
                if (l & ~(0x00FF))
                    l = 0;

                if (l & QDI_DIBTOSCREEN)
                    w |= PD_STRETCHDIB_1_1_OK | PD_CAN_DRAW_DIB;

                if (l & QDI_STRETCHDIB)
                    w |= PD_CAN_STRETCHDIB;

                /* what about stretching? fast? */
            }

            ReleaseDC(NULL, hdc);
            return w;
    }
}

/****************************************************************
* @doc EXTERNAL DrawDib
*
* @api void | DrawDibProfileDisplay | Profiles the display for DrawDib.
*
* @parm LPBITMAPINFOHEADER | parms | Specifies bitmap information. 
*       Set to null if no information is available.
*
*****************************************************************/

DWORD VFWAPI DrawDibProfileDisplay(LPBITMAPINFOHEADER lpbi)
{
    if (lpbi == NULL)
    {
        InitProfDisp(TRUE) ;
        return (DWORD)(LPVOID)displayFPS;
    }
    else
        return ProfDispCanDrawDib(lpbi);
}

LPVOID FAR TestDibFormats(int dx, int dy)
{
    int         dxScreen,dyScreen;
    RECT        rc;
    HWND        hwnd;
    HDC         hdc;
    int         n;
    int         i;
    HCURSOR     hcur;
    HPALETTE    hpal;
#ifdef DEBUG
    HPALETTE    hpalT;
#endif
    HWND        hwndActive;
    TCHAR       szProfiling[80];

    // dont change this without changing MSVIDEO.RC
    #define IDS_PROFILING       4000

    extern HMODULE ghInst;      // in MSVIDEO\init.c

    if (!LoadString(ghInst, IDS_PROFILING, szProfiling, sizeof(szProfiling)/sizeof(TCHAR)))
        lstrcpy(szProfiling, szProfilingDefault);

#ifdef WIN32
    #define GetCurrentInstance() GetModuleHandle(NULL)
#else
    #define GetCurrentInstance() NULL
#endif

    dxScreen = GetSystemMetrics(SM_CXSCREEN);
    dyScreen = GetSystemMetrics(SM_CYSCREEN);

    // fill in displayFPS[7][3][2];

    for (n=0; n<7; n++)
        for (i=0; i<3; i++)
            displayFPS[n][i][0] =
            displayFPS[n][i][1] = 0;

    SetRect(&rc, 0, 0, dx, dy);
    AdjustWindowRect(&rc, (WS_OVERLAPPED | WS_CAPTION | WS_BORDER), FALSE);
    OffsetRect(&rc, -rc.left, -rc.top);

    hwnd =
#ifdef BIDI
	CreateWindowEx(WS_EX_BIDI_SCROLL |  WS_EX_BIDI_MENU |WS_EX_BIDI_NOICON,
			szListbox,     // Class name
                         szProfiling,   // Caption
                          LBS_NOINTEGRALHEIGHT|
                          (WS_OVERLAPPED | WS_CAPTION | WS_BORDER),
                        (dxScreen - rc.right) / 2,
                        (dyScreen - rc.bottom) / 2,
                          rc.right,
                          rc.bottom,
                          (HWND)NULL,             // Parent window (no parent)
                          (HMENU)NULL,            // use class menu
                          GetCurrentInstance(),   // handle to window instance
                        (LPTSTR)NULL            // no params to pass on
                         );

#else
	CreateWindow (
			szListbox,     // Class name
                         szProfiling,   // Caption
                          LBS_NOINTEGRALHEIGHT|
                          (WS_OVERLAPPED | WS_CAPTION | WS_BORDER),
                        (dxScreen - rc.right) / 2,
                        (dyScreen - rc.bottom) / 2,
                          rc.right,
                          rc.bottom,
                          (HWND)NULL,             // Parent window (no parent)
                          (HMENU)NULL,            // use class menu
                          GetCurrentInstance(),   // handle to window instance
                        (LPTSTR)NULL            // no params to pass on
                         );
#endif

    // make the window top most
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);

    // and show it.
    SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
        SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_SHOWWINDOW);

    // and activate it.
    hwndActive = GetActiveWindow();
    SetActiveWindow(hwnd);

    hdc = GetDC(hwnd);
    hcur = SetCursor(NULL);

    if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
    {
        hpal = CreateTestPalette(TRUE);
        SelectPalette(hdc, hpal, FALSE);
        RealizePalette(hdc);
    }

    Yield();
    Yield();
    Yield();

#ifndef WIN32
    //
    //      make sure no junk is around in the SmartDrv cache, this will
    //      mess with the timings
    //
    _asm {
        mov     ax,4A10h        ; tell Bambi to flush the cache
        mov     bx,0001h
        int     2fh

        mov     ah,0Dh          ; tell other people to commit...
        int     21h
    }
#else
    GdiFlush();
#endif

    for (n=FIRST_N; n<5; n++)
        result[n] = ProfileDisplay(hdc, n==0 ? 4 : n*8, dx, dy);

#ifdef DEBUG
    if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
    {
        //
        // re-run the 8bit tests with a background palette
        //
        SelectPalette(hdc, GetStockObject(DEFAULT_PALETTE), FALSE);
        RealizePalette(hdc);

	hpalT = CreateTestPalette(FALSE);
	SelectPalette(hdc, hpalT, TRUE);
        RealizePalette(hdc);

        Yield();
        Yield();
        Yield();

        ProfileDisplay(hdc, 8, dx, dy);

        SelectPalette(hdc, GetStockObject(DEFAULT_PALETTE), FALSE);
        RealizePalette(hdc);
	DeleteObject(hpalT);
        DeleteObject(hpal);
    }
#endif

    SetCursor(hcur);
    ReleaseDC(hwnd, hdc);

    if (hwndActive)
        SetActiveWindow(hwndActive);

    DestroyWindow(hwnd) ;

    return (LPVOID)displayFPS;
}

static UINT ProfileDisplay(HDC hdc, UINT wBitsToTest, int dx, int dy)
{
    LPBITMAPINFOHEADER alpbi[N_FRAMES];
    UINT                wRetval;

    if (GetDeviceCaps(hdc, BITSPIXEL) *
        GetDeviceCaps(hdc, PLANES) <= 8 && wBitsToTest > 8)
        return 0;

    alpbi[0] = NULL;

    MakeFrames(alpbi,wBitsToTest,dx,dy);

    wRetval = 0 ;

    if (!alpbi[0])
        return (UINT)-1 ;

    switch(DrawDibTest(hdc,alpbi,N_FRAMES,100))
    {
        case CANT_DO_THESE_BITS:
            goto done ;

        case CANT_DO_STRETCHDIBITS:
            wRetval = PD_CAN_DRAW_DIB ;
            goto done ;

        case STRETCHDI_FASTER:
            wRetval = PD_STRETCHDIB_1_1_OK ;
            /* Falling through */

        case OTHER_FASTER:
            wRetval |= PD_CAN_DRAW_DIB;
    }

    if (DrawDibTest(hdc,alpbi,N_FRAMES,STRETCH_N) == STRETCHDI_FASTER)
        wRetval |= PD_STRETCHDIB_1_N_OK|PD_CAN_STRETCHDIB;

    if (DrawDibTest(hdc,alpbi,N_FRAMES,200) == STRETCHDI_FASTER)
        wRetval |= PD_STRETCHDIB_1_2_OK|PD_CAN_STRETCHDIB;

done:
    FreeFrames(alpbi);

    return wRetval;
}

static UINT DrawDibTest(HDC hdc,LPBITMAPINFOHEADER FAR *alpbi,UINT wFrames,UINT wStretch)
{
    HDC                 hdcMem ;
    HBITMAP             hbitmap ;
    HBITMAP             hbitmapOld ;

    UINT                wBits ;
    DWORD               dwSize;
    DWORD               wSizeColors ;
    DWORD               dwSizeImage;

    volatile LPBITMAPINFOHEADER  lpbi ;
    LPBYTE              bits ;
    LPBITMAPINFOHEADER  lpbiStretch ;
    LPBYTE              bitsStretch ;
    DWORD               time0 = 0;
    DWORD               time1 = 0;
    DWORD               time2 = 0;
    RECT                rc ;
    int                 XDest,YDest,cXDest,cYDest ;
    int                 cXSrc,cYSrc ;
    int                 i ;
    int                 n ;
    int                 q ;
    UINT                DibUsage;
    HPALETTE            hpal;
    BOOL                fBack;
    BOOL                f;

    lpbi = alpbi[0];

    /*
    ** Get stuff common to all frames
    */
    wBits = lpbi->biBitCount ;
    //cXSrc = (int)lpbi->biWidth ;
    //cYSrc = (int)lpbi->biHeight ;
    //cXDest = wStretch*(int)lpbi->biWidth/100 ;
    //cYDest = wStretch*(int)lpbi->biHeight/100 ;

    cXSrc = 100*(int)lpbi->biWidth/wStretch ;
    cYSrc = 100*(int)lpbi->biHeight/wStretch ;
    cXDest = (int)lpbi->biWidth ;
    cYDest = (int)lpbi->biHeight ;

    // are we background'ed
    n = wStretch == 100 ? 0 : wStretch == 200 ? 1 : 2;
    fBack = wBits==8 && displayFPS[1][n][0] != 0;

    if (lpbi->biBitCount <= 8)
        wSizeColors = sizeof(RGBQUAD) * (int)(lpbi->biClrUsed ? lpbi->biClrUsed : (1 << (int)lpbi->biBitCount));
    else
        wSizeColors = 0 ;

    bits = (LPBYTE)lpbi + (int)lpbi->biSize + wSizeColors ;

    if (GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES) <= 8 && wBits > 8)
        return CANT_DO_STRETCHDIBITS;

////if (wStretch != 100 && !(GetDeviceCaps(hdc,RASTERCAPS)&(RC_STRETCHDIB|RC_STRETCHBLT)))

    if (wStretch != 100 && !(GetDeviceCaps(hdc,RASTERCAPS)&(RC_STRETCHDIB)))
        return CANT_DO_STRETCHDIBITS ;

    if (wStretch != 100 && wBits == 4)
        return CANT_DO_STRETCHDIBITS ;

    if (wStretch != 100 && (GetWinFlags() & WF_CPU286))
        return STRETCHDI_FASTER;

//  if (wStretch != 100 && wBits > 8) //!!!
//      wFrames = 4;

    lpbi->biWidth  = cXSrc;
    lpbi->biHeight = cYSrc;

    // get current palette
    hpal = SelectPalette(hdc, GetStockObject(DEFAULT_PALETTE), FALSE);
    SelectPalette(hdc, hpal, fBack);
    RealizePalette(hdc);

    if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
    {
        DibUsage = DIB_PAL_COLORS;
    }
    else
    {
        DibUsage = DIB_RGB_COLORS;
    }

////GetClientRect(hwnd,&rc) ;
    GetClipBox(hdc,&rc) ;
    XDest = (rc.right - cXDest)/2 ;
    YDest = (rc.bottom - cYDest)/2 ;

    time0 = 0;

    if (wBits == 16 && !IsDisplay16Bit(hdc))
        goto test_bitmap;

    if (wBits == 32 && !IsDisplay32Bit(hdc))
        goto test_bitmap;

    time0 = timeGetTime() ;

    for (i=0; i<(int)wFrames; i++)
    {
        lpbi = alpbi[i%N_FRAMES];
        bits = ((LPBYTE)lpbi) + (int)lpbi->biSize + wSizeColors ;

#ifdef WIN32
	/*
	 * to correctly model the behaviour of DrawDibDraw, we
	 * use SetDIBitsToDevice if 1:1 (source rect == dest rect).
	 */
	if ( (cXSrc == cXDest) && (cYSrc == cYDest)) {
            f = SetDIBitsToDevice(hdc, XDest, YDest, cXDest, cYDest,
				0, 0, 0, cYSrc,
                                bits, (LPBITMAPINFO)lpbi, DibUsage);
	} else
#endif
        {
            f = StretchDIBits(
                    hdc,
                    XDest,YDest,cXDest,cYDest,
                    0,0,cXSrc, cYSrc,
                    bits,(LPBITMAPINFO)lpbi,DibUsage,SRCCOPY) ;
        }
    }

    GdiFlush();

    time0 = timeGetTime() - time0 ;

    if (f == 0)
        time0 = 0;

test_bitmap:
    time1 = 0;

    if (wStretch == 100)
    {
        PSETDI psd;

        psd = (PSETDI)LocalAlloc(LPTR, sizeof(SETDI));

        if (psd == NULL)
            goto done;

        hbitmap = CreateCompatibleBitmap(hdc,cXDest,cYDest) ;
        hdcMem = CreateCompatibleDC(NULL) ;
        hbitmapOld = SelectObject(hdcMem,hbitmap) ;

        f = SetBitmapBegin(
                    psd,            //  structure
                    hdc,            //  device
                    hbitmap,        //  bitmap to set into
                    lpbi,           //  --> BITMAPINFO of source
                    DibUsage);

        psd->hdc = hdc;

        if (f)
            f = SetBitmap(psd, 0, 0, cXDest, cYDest, bits, 0, 0, cXDest, cYDest);

        if (f)
        {
//          SelectPalette(hdc, GetStockObject(DEFAULT_PALETTE), FALSE);
//          RealizePalette(hdc);

            time1 = timeGetTime();

            for (i=0; i<(int)wFrames; i++)
            {
                SetBitmap(psd, 0, 0, cXDest, cYDest, bits, 0, 0, cXDest, cYDest);
                BitBlt(hdc,XDest,YDest,cXDest,cYDest,hdcMem,0,0,SRCCOPY);
            }

            GdiFlush();

            time1 = timeGetTime() - time1 ;

            SetBitmapEnd(psd);

//          SelectPalette(hdc, hpal, fBack);
//          RealizePalette(hdc);
        }

#ifdef DEBUG
        if ((int)lpbi->biBitCount == GetDeviceCaps(hdc,BITSPIXEL)*GetDeviceCaps(hdc,PLANES))
        {
            time2 = timeGetTime() ;

            for (i=0; i<(int)wFrames; i++)
                BitBlt(hdc,XDest,YDest,cXDest,cYDest,hdcMem,0,0,SRCCOPY) ;

            GdiFlush();

            time2 = timeGetTime() - time2 ;
        }
#endif

        LocalFree((HLOCAL)psd);
        SelectObject(hdcMem,hbitmapOld) ;
        DeleteObject(hbitmap) ;
        DeleteDC(hdcMem) ;
    }
    else
    {
        if (wBits == 16 && !IsDisplay16Bit(hdc))
            goto done;

        if (wBits == 32 && !IsDisplay32Bit(hdc))
            goto done;

#ifdef NOSTRETCH
	/*
         * StretchDIB not ported from ASM yet - so StretchDIBits must win.
         */
        time1 = time0 + 1;
#else
        // Calc size we need to allocate for stretched bits

        dwSizeImage = (DWORD)(UINT)cYDest*(DWORD)(((UINT)cXDest*(UINT)lpbi->biBitCount+31)/32*4);
        dwSize = (int)lpbi->biSize + (int)lpbi->biClrUsed*sizeof(RGBQUAD);

        if ((lpbiStretch = (LPVOID)GlobalAllocPtr(GHND,dwSize + dwSizeImage)) != NULL)
        {
            hmemcpy(lpbiStretch, lpbi, dwSize);
            lpbiStretch->biWidth  = cXDest;
            lpbiStretch->biHeight = cYDest;
            lpbiStretch->biSizeImage = dwSizeImage;
            bitsStretch = (LPBYTE)lpbiStretch + (UINT)dwSize;

            time1 = timeGetTime() ;

            for (i=0; i<(int)wFrames; i++)
            {
                lpbi = alpbi[i%N_FRAMES];
                bits = ((LPBYTE)lpbi) + (int)lpbi->biSize + wSizeColors;

                StretchDIB(
                    lpbiStretch,bitsStretch,
                    0,0,cXDest,cYDest,
                    lpbi,bits,
                    0,0,cXSrc,cYSrc);
#ifdef WIN32
		/*
		 * to correctly model the behaviour of DrawDibDraw, we
		 * use SetDIBitsToDevice if 1:1 (source rect == dest rect).
		 */
                f = SetDIBitsToDevice(hdc, XDest, YDest, cXDest, cYDest,
                                    0, 0, 0, cYSrc,
                                    bits, (LPBITMAPINFO)lpbi, DibUsage);
#else
                f = StretchDIBits(
                    hdc,
                    XDest,YDest,cXDest,cYDest,
                    0,0,cXDest,cYDest,
                    bitsStretch,(LPBITMAPINFO)lpbiStretch,DibUsage,SRCCOPY);
#endif
            }

            GdiFlush();

            time1 = timeGetTime() - time1 ;

            GlobalFreePtr(lpbiStretch);

            if (f == 0)
                time1 = 0;
#endif
        }

#ifdef DEBUG
        if ((int)lpbi->biBitCount == GetDeviceCaps(hdc,BITSPIXEL)*GetDeviceCaps(hdc,PLANES))
        {
            hbitmap = CreateCompatibleBitmap(hdc,cXSrc,cYSrc) ;
            hdcMem = CreateCompatibleDC(NULL) ;
            hbitmapOld = SelectObject(hdcMem,hbitmap) ;
	    lpbi = alpbi[0];
	    bits = ((LPBYTE)lpbi) + (int)lpbi->biSize + wSizeColors;
            SetDIBits(hdc,hbitmap,0,cYSrc,bits,(LPBITMAPINFO)lpbi,DibUsage);

            SelectPalette(hdcMem, hpal, FALSE);
//          RealizePalette(hdcMem);

            time2 = timeGetTime() ;

            for (i=0; i<(int)wFrames; i++)
                StretchBlt(hdc,XDest,YDest,cXDest,cYDest,hdcMem,0,0,cXSrc,cYSrc,SRCCOPY) ;

            GdiFlush();

            time2 = timeGetTime() - time2 ;

            SelectObject(hdcMem,hbitmapOld) ;
            DeleteObject(hbitmap) ;
            DeleteDC(hdcMem) ;
        }
#endif
    }

done:
    /* time0 is the time required to do StretchDIBits */
    /* time1 is the time required to do Set + BitBlt */
    /* time2 is the time required to do a BitBlt */

    //
    // compute the FPS * 10 and store for later use.
    //
    n = wStretch == 100 ? 0 : wStretch == 200 ? 1 : 2;
    q = fBack ? BACKINDEX : wBits/8;

    time1 = (DWORD)MulDiv((int)time1,PROFDISP_FUDGE,100);

    displayFPS[q][n][0] = time0 ? (UINT)MulDiv(wFrames,10000,(int)time0) : 0;
    displayFPS[q][n][1] = time1 ? (UINT)MulDiv(wFrames,10000,(int)time1) : 0;

    if (time2)
        displayFPS[BITBLTINDEX][n][fBack] = (UINT)MulDiv(wFrames,10000,(int)time2);

    RPF(("DrawDibTest %dx%dx%d %d StretchDIBits=%04lu SetDI+BitBlt=%04lu BitBlt=%04lu %ls",cXDest,cYDest,wBits,wStretch,time0,time1,time2,(LPSTR)(time0 < time1 ? TEXT("") : TEXT("SUCKS!"))));

    lpbi->biWidth  = cXDest;
    lpbi->biHeight = cYDest;

    if (time0 == 0)
    {
        return time1 ? OTHER_FASTER : CANT_DO_THESE_BITS;
    }
    else
    {
        if (time1)
            return (time0 < time1) ? STRETCHDI_FASTER : OTHER_FASTER;
        else
            return STRETCHDI_FASTER;
    }
}

static void MakeFrames(LPBITMAPINFOHEADER FAR *alpbi, UINT bits, UINT wXSize,UINT wYSize )
{
    int         i ;
    int         x ;
    int         y ;
    LPBITMAPINFOHEADER lpbi ;
    DWORD       dwSizeImage;
    BYTE _huge *pb;
    WORD FAR   *pw;
    DWORD FAR  *pdw;
    UINT        rc;
    HDC         hdc;

    hdc = GetDC(NULL);
    rc = GetDeviceCaps(hdc, RASTERCAPS);
    ReleaseDC(NULL,hdc);

    FreeFrames(alpbi);

    dwSizeImage = wYSize*(DWORD)((wXSize*bits/8+3)&~3);

    lpbi = (LPVOID)GlobalAllocPtr(GHND,sizeof(BITMAPINFOHEADER)+dwSizeImage + 1024);
    lpbi->biSize            = sizeof(BITMAPINFOHEADER) ;
    lpbi->biWidth           = wXSize ;
    lpbi->biHeight          = wYSize ;
    lpbi->biPlanes          = 1 ;
    lpbi->biBitCount        = bits ;
    lpbi->biCompression     = BI_RGB ;
    lpbi->biSizeImage       = dwSizeImage;
    lpbi->biXPelsPerMeter   = 0 ;
    lpbi->biYPelsPerMeter   = 0 ;
    lpbi->biClrUsed         = 0 ;
    lpbi->biClrImportant    = 0 ;

    // !!! These should be RGB DIBs if the device isn't a palette device!

    if (bits == 4)
    {
        lpbi->biClrUsed = 16;
    }
    else if (bits == 8)
    {
	lpbi->biClrUsed = 256;
    }

    pb = (BYTE _huge *)lpbi+lpbi->biSize+lpbi->biClrUsed * sizeof(RGBQUAD);

    if (bits == 4)
    {
        for (y=0; y<(int)wYSize; y++)
            for (x=0; x<(int)wXSize; x += 2)
	    {
                i = ((x / (wXSize / 4)) + 4 * (y / (wYSize / 4)));
		i += i * 16;
		*pb++ = i;
            }

        if (rc & RC_PALETTE)
        {
            pw = (LPVOID)((LPBYTE)lpbi+(int)lpbi->biSize);

            for (i=0; i<8; i++)
                *pw++ = i;

            for (i=0; i<8; i++)
                *pw++ = 248+i;
        }
        else
        {
            pdw = (LPVOID)((LPBYTE)lpbi+(int)lpbi->biSize);

            *pdw++ = 0x00000000;    // 0000  black
            *pdw++ = 0x00800000;    // 0001  dark red
            *pdw++ = 0x00008000;    // 0010  dark green
            *pdw++ = 0x00808000;    // 0011  mustard
            *pdw++ = 0x00000080;    // 0100  dark blue
            *pdw++ = 0x00800080;    // 0101  purple
            *pdw++ = 0x00008080;    // 0110  dark turquoise
            *pdw++ = 0x00C0C0C0;    // 1000  gray
            *pdw++ = 0x00808080;    // 0111  dark gray
            *pdw++ = 0x00FF0000;    // 1001  red
            *pdw++ = 0x0000FF00;    // 1010  green
            *pdw++ = 0x00FFFF00;    // 1011  yellow
            *pdw++ = 0x000000FF;    // 1100  blue
            *pdw++ = 0x00FF00FF;    // 1101  pink (magenta)
            *pdw++ = 0x0000FFFF;    // 1110  cyan
            *pdw++ = 0x00FFFFFF;    // 1111  white
        }
    }
    else if (bits == 8)
    {
        for (y=0; y<(int)wYSize; y++)
            for (x=0; x<(int)wXSize; x++)
            {
                *pb++ = 10 + y * 236 / (int)wYSize;
            }

        if (rc & RC_PALETTE)
        {
            pw = (LPVOID)((LPBYTE)lpbi+(int)lpbi->biSize);

            for (i=0; i<256; i++)
                *pw++ = i;
        }
        else
        {
            pdw = (LPVOID)((LPBYTE)lpbi+(int)lpbi->biSize);

            for (i=0; i<256; i++)
                *pdw++ = RGB(i,0,0);
        }
    }
    else if (bits == 16)
    {
        for (y=0; y<(int)wYSize; y++)
            for (x=0; x<(int)wXSize; x++)
            {
                *pb++ = (BYTE) ((UINT)y * 32u / wYSize);
                *pb++ = (BYTE)(((UINT)x * 32u / wXSize) << 2);
	    }
    }
    else if (bits == 24)
    {
        for (y=0; y<(int)wYSize; y++)
            for (x=0; x<(int)wXSize; x++)
	    {
                *pb++ = (BYTE) (y * 256l / wYSize);
                *pb++ = (BYTE)~(x * 256l / wXSize);
                *pb++ = (BYTE) (x * 256l / wXSize);
	    }
    }
    else if (bits == 32)
    {
        for (y=0; y<(int)wYSize; y++)
            for (x=0; x<(int)wXSize; x++)
	    {
                *pb++ = (BYTE)~(x * 256l / wXSize);
                *pb++ = (BYTE) (y * 256l / wYSize);
                *pb++ = (BYTE) (x * 256l / wXSize);
                *pb++ = 0;
	    }
    }

    for ( i=0; i<N_FRAMES; i++ )
        alpbi[i] = lpbi;
}

static void FreeFrames(LPBITMAPINFOHEADER FAR *alpbi)
{
    UINT        w ;

    if (!alpbi[0])
        return ;

    for (w=0; w<N_FRAMES; w++)
	if (alpbi[w] && (w == 0 || alpbi[w] != alpbi[w-1]))
            GlobalFreePtr(alpbi[w]);

    for (w=0; w<N_FRAMES; w++)
        alpbi[w] = NULL;
}

#if 0
/*
 *  CreateTestPalette()
 *
 */
static HPALETTE CreateTestPalette(BOOL f)
{
    HDC hdc;
    int i;

    struct {
        WORD         palVersion;
        WORD         palNumEntries;
        PALETTEENTRY palPalEntry[256];
    }   pal;

    pal.palNumEntries = 256;
    pal.palVersion    = 0x0300;

    hdc = GetDC(NULL);
    GetSystemPaletteEntries(hdc, 0, 256, &pal.palPalEntry[0]);
    ReleaseDC(NULL,hdc);

    for (i = 10; i < 246; i++)
        pal.palPalEntry[i].peFlags = PC_NOCOLLAPSE;

    if (!f)
        pal.palPalEntry[0].peRed = 255;

    return CreatePalette((LPLOGPALETTE)&pal);
}

#else

/*
 *  CreateTestPalette()
 *
 */
static HPALETTE CreateTestPalette(BOOL fUp)
{
    int i;
    HDC hdc;

    struct {
        WORD         palVersion;
        WORD         palNumEntries;
        PALETTEENTRY palPalEntry[256];
    }   pal;

    pal.palNumEntries = 256;
    pal.palVersion    = 0x0300;

    for (i = 0; i < 256; i++)
    {
        pal.palPalEntry[i].peRed   = 0;
        pal.palPalEntry[i].peGreen = 0;
        pal.palPalEntry[i].peBlue  = (BYTE)(fUp ? i : 255 - i);
        pal.palPalEntry[i].peFlags = PC_NOCOLLAPSE;
    }

    hdc = GetDC(NULL);
    GetSystemPaletteEntries(hdc, 0,   10, &pal.palPalEntry[0]);
    GetSystemPaletteEntries(hdc, 246, 10, &pal.palPalEntry[246]);
    ReleaseDC(NULL,hdc);

    return CreatePalette((LPLOGPALETTE)&pal);
}
#endif

#define RGB555_RED      0x7C00
#define RGB555_GREEN    0x03E0
#define RGB555_BLUE     0x001F

static BOOL IsDisplay16Bit( HDC hdc )
{
    struct {
        BITMAPINFOHEADER    bi;
        RGBQUAD             rgbq[256];
    }   dib;
    int                     w ;
    LONG                    l=0;
    WORD                    bits[2];
    COLORREF                cref ;

    w = GetDeviceCaps(hdc,BITSPIXEL)*GetDeviceCaps(hdc,PLANES) ;

    if ( w < 15 )
        return FALSE;

    /*
    ** OK, the hardware is at least 16 bits - now test to see
    ** if they impelement 5-5-5 RGB
    */

    dib.bi.biSize = sizeof(BITMAPINFOHEADER);
    dib.bi.biWidth = 1;
    dib.bi.biHeight = 1;
    dib.bi.biPlanes = 1;
    dib.bi.biBitCount = 16;
    dib.bi.biCompression = BI_RGB;
    dib.bi.biSizeImage = 4;
    dib.bi.biXPelsPerMeter = 0;
    dib.bi.biYPelsPerMeter = 0;
    dib.bi.biClrUsed = 1;
    dib.bi.biClrImportant = 0;

    //
    // just in case they try to decode it as rle
    //
    bits[0] = 0x0000;           // this is RLE EOL
    bits[1] = 0x0100;           // this is RLE EOF

    //
    // send the Escape to see if they support 16bpp DIBs
    //
    if (Escape(hdc, QUERYDIBSUPPORT, (int)dib.bi.biSize, (LPVOID)&dib, (LPVOID)&l) > 0)
    {
        // make sure the driver realy realy gave us back flags.
	if (l & ~(0x00FF))
		l = 0;

        if (l & (QDI_DIBTOSCREEN|QDI_STRETCHDIB))
            return TRUE;
    }

    //
    // they dont support the QUERYDIBSUPPORT Escape, try to draw DIBs and see
    // what they do!
    //

    if ( !StretchDIBits(hdc,0,0,1,1,0,0,1,1,bits,(LPBITMAPINFO)&dib,DIB_RGB_COLORS,SRCCOPY))
        return FALSE;

    cref = GetPixel(hdc,0,0) ;

    if (cref != RGB(0,0,0))
        return FALSE;

    /*
    ** Display a red pixel of the max value and get it back with
    ** GetPixel(). Verify that red has the max value in the RGB
    ** triplet and green and blue are nothing.
    */
    bits[0] = RGB555_RED ;
    if ( !StretchDIBits(hdc,0,0,1,1,0,0,1,1,bits,(LPBITMAPINFO)&dib,DIB_RGB_COLORS,SRCCOPY))
        return FALSE;

    cref = GetPixel(hdc,0,0) & 0x00F8F8F8;

    if (cref != RGB(0xF8,0,0))
        return FALSE;

    /*
    ** Ditto green. Note that if the driver is implementing 5-6-5, then
    ** green will read back as less than full scale and we will catch
    ** it here.
    */
    bits[0] = RGB555_GREEN ;
    if ( !StretchDIBits(hdc,0,0,1,1,0,0,1,1,bits,(LPBITMAPINFO)&dib,DIB_RGB_COLORS,SRCCOPY))
        return FALSE;

    cref = GetPixel(hdc,0,0) & 0x00F8F8F8;

    if (cref != RGB(0,0xF8,0))
        return FALSE;

    /*
    ** Ditto blue.
    */
    bits[0] = RGB555_BLUE ;
    if ( !StretchDIBits(hdc,0,0,1,1,0,0,1,1,bits,(LPBITMAPINFO)&dib,DIB_RGB_COLORS,SRCCOPY))
        return FALSE;

    cref = GetPixel(hdc,0,0) & 0x00F8F8F8;

    if (cref != RGB(0,0,0xF8))
        return FALSE;

    return TRUE;
}

static BOOL IsDisplay32Bit( HDC hdc )
{
    struct {
        BITMAPINFOHEADER    bi;
        RGBQUAD             rgbq[256];
    }   dib;
    int                     w ;
    LONG                    l=0;
    DWORD                   bits[2];

    w = GetDeviceCaps(hdc,BITSPIXEL)*GetDeviceCaps(hdc,PLANES) ;

    if ( w < 15 )
        return FALSE;

    /*
    ** OK, the hardware is at least 16 bits - now test to see
    ** if they impelement a 32 bit DIB
    */

    dib.bi.biSize = sizeof(BITMAPINFOHEADER);
    dib.bi.biWidth = 2;
    dib.bi.biHeight = 1;
    dib.bi.biPlanes = 1;
    dib.bi.biBitCount = 32;
    dib.bi.biCompression = BI_RGB;
    dib.bi.biSizeImage = 4;
    dib.bi.biXPelsPerMeter = 0;
    dib.bi.biYPelsPerMeter = 0;
    dib.bi.biClrUsed = 1;
    dib.bi.biClrImportant = 0;

    //
    // send the Escape to see if they support 32bpp DIBs
    //
    if (Escape(hdc, QUERYDIBSUPPORT, (int)dib.bi.biSize, (LPVOID)&dib, (LPVOID)&l) > 0)
    {
        // make sure the driver realy realy gave us back flags.
	if (l & ~(0x00FF))
		l = 0;

        if (l & (QDI_DIBTOSCREEN|QDI_STRETCHDIB))
            return TRUE;
    }

    bits[0] = 0x00000000;
    bits[1] = 0x00FFFFFF;

    //
    // they dont support the QUERYDIBSUPPORT Escape, try to draw DIBs and see
    // what they do!
    //
    if (!StretchDIBits(hdc,0,0,2,1,0,0,2,1,bits,(LPBITMAPINFO)&dib,DIB_RGB_COLORS,SRCCOPY))
        return FALSE;

    if (GetPixel(hdc,0,0) != 0)
        return FALSE;

    if ((GetPixel(hdc,1,0) & 0x00F8F8F8) != 0x00F8F8F8)
        return FALSE;

    return TRUE;
}

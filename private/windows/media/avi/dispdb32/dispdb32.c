/******************************Module*Header*******************************\
* Module Name: dispdib32.c
*
* Fakes the display of full screen videos.
*
*
* Created: 23-03-94
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1994 Microsoft Corporation
\**************************************************************************/
#include <windows.h>
#include "dispdib.h"
#include "drawdib.h"

/* -------------------------------------------------------------------------
** Private constants
** -------------------------------------------------------------------------
*/
#define CX_MAX_MOVIE_DEFAULT  640
#define CY_MAX_MOVIE_DEFAULT  480

/* -------------------------------------------------------------------------
** Private functions prototypes
** -------------------------------------------------------------------------
*/
LRESULT CALLBACK
FullScreenWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT CALLBACK
KeyboardHookProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam
    );

UINT
DisplayDibEnter(
    LPBITMAPINFOHEADER lpbi,
    UINT wFlags
    );

void
DisplayDibLeave(
    UINT wFlags
    );

int
DisplayCalcMovieMultiplier(
    int cxOriginal,
    int cyOriginal,
    DWORD dwCompression
    );


/* -------------------------------------------------------------------------
** Debugging stuff
** -------------------------------------------------------------------------
*/
#if DBG

void
dprintf(
    LPSTR lpszFormat,
    ...
    );

int DebugLevel = 1;

#define  dpf( _x_ )                       dprintf _x_
#define dpf1( _x_ ) if (DebugLevel >= 1) {dprintf _x_ ;} else
#define dpf2( _x_ ) if (DebugLevel >= 2) {dprintf _x_ ;} else
#define dpf3( _x_ ) if (DebugLevel >= 3) {dprintf _x_ ;} else
#define dpf4( _x_ ) if (DebugLevel >= 4) {dprintf _x_ ;} else
#define dpf5( _x_ ) if (DebugLevel >= 5) {dprintf _x_ ;} else

#else

#define  dpf( _x_ )
#define dpf1( _x_ )
#define dpf2( _x_ )
#define dpf3( _x_ )
#define dpf4( _x_ )
#define dpf5( _x_ )

#endif



/* -------------------------------------------------------------------------
** Private Globals
** These are only valid in the process that started playing the movie.
** -------------------------------------------------------------------------
*/
HWND        hwndFullScreen;
HDC         hdcFullScreen;
HDRAWDIB    hdd;
BOOL        fClassRegistered;
int         dxScreen;
int         dyScreen;
int         iMovieSizeMultiplier;



/* -------------------------------------------------------------------------
** Global data shared between all processes that attach to this library.
** This is required to make the keyboard hook work correctly.
** -------------------------------------------------------------------------
*/
#pragma data_seg( ".sdata" )
BOOL    fStop;
HHOOK   hHookK;
#pragma data_seg()



/******************************Public*Routine******************************\
* DisplayDib
*
* Just call DisplayDibEx
*
* History:
* 23-03-94 - StephenE - Created
*
\**************************************************************************/
UINT FAR PASCAL
DisplayDib(
    LPBITMAPINFOHEADER lpbi,
    LPSTR lpBits,
    UINT wFlags
    )
{
    dpf(( "Down level api, use DisplayDibEx" ));
    return DisplayDibEx( lpbi, 0, 0, lpBits, wFlags );
}




/******************************Public*Routine******************************\
* @doc EXTERNAL DISPDIB
*
* @api UINT | DisplayDibEx | This function displays a 256-color bitmap on a
*    standard VGA display. It reduces the display resolution to 320-by-200
*    or 320-by-240 and uses the full screen to display the bitmap, clipping
*    and centering it as necessary. The function normally does not return to
*    the application until the user presses a key or clicks a mouse button.
*
*    To call <f DisplayDibEx>, an application must be the active
*    application. All inactive applications and GDI screen updates
*    are suspended while <f DisplayDib> temporarily reconfigures
*    the display.
*
* @parm LPBITMAPINFO | lpbi | Specifies a pointer to a <t BITMAPINFO>
*    header describing the bitmap to be displayed.
*
* @parm int | x | x position to place DIB iff DISPLAYDIB_NOCENTER flags is set
*      the lower left is (0,0)
*
* @parm int | y | y position to place DIB iff DISPLAYDIB_NOCENTER flags is set
*      the lower left is (0,0)
*
* @parm LPSTR | lpBits | Specifies a pointer to the bitmap bits. If this
*     parameter is NULL, the bits are assumed to follow the
*     <t BITMAPINFO> structure pointed to by <p lpbi>.
*
* @parm UINT | wFlags | Specifies options for displaying the bitmap. Use
*  the following flags:
*
* @flag  DISPLAYDIB_MODE_DEFAULT | Use the default mode (320 by 240)
*    to display the bitmap.
* @flag  DISPLAYDIB_MODE_320x200x8 | Use 320-by-200 mode to display
*    the bitmap.
* @flag  DISPLAYDIB_MODE_320x240x8 | Use 320-by-240 mode to display
*    the bitmap. This is the default.
* @flag  DISPLAYDIB_NOWAIT | Return immediately after displaying the
*  bitmap; don't wait for a key press or mouse click before returning.
* @flag  DISPLAYDIB_NOPALETTE      | Ignore the palette associated
*    with the bitmap. You can use this flag when displaying a series
*    of bitmaps that use a common palette.
* @flag  DISPLAYDIB_NOCENTER       | Don't center the image. The function
*  displays the bitmap in the lower-left corner of the display.
* @flag  DISPLAYDIB_NOIMAGE        | Don't draw image
* @flag  DISPLAYDIB_ZOOM2          | Stretch image by 2
* @flag  DISPLAYDIB_DONTLOCKTASK   | dont lock out other tasks
* @flag  DISPLAYDIB_TEST           | dont do any thing just test for support
* @flag  DISPLAYDIB_BEGIN          | Switch to the low-resolution
*    display mode and set the palette. The bitmap is not displayed.
*
*    If you are displaying a series of images that use the same palette,
*    you can call <f DisplayDib> with this flag to prepare the display for
*    the bitmaps, then make a series of <f DisplayDib> calls with the
*    DISPLAYDIB_NOPALETTE flag. This technique
*    eliminates the screen flicker that occurs when the display is
*    switched between the low-resolution and standard VGA modes.
*    To return the display to standard VGA mode, subsequently
*    call <f DisplayDib> with the DISPLAYDIB_END flag.
*
* @flag  DISPLAYDIB_END            | Switch back to standard VGA mode
*    and return without displaying a bitmap. Signifies the end of multiple
*    calls to <f DisplayDib>. With this flag, you can specify
*    NULL for the <p lpbi> and <p lpBits> parameters.
*
* @rdesc Returns zero if successful, otherwise returns an error code.
*  Error codes are as follows:
*
* @flag  DISPLAYDIB_NOTSUPPORTED   | <f DisplayDib> is not supported
*   in the current mode.
* @flag  DISPLAYDIB_INVALIDDIB     | The bitmap specified by
*   <p lpbi> is not a valid bitmap.
* @flag  DISPLAYDIB_INVALIDFORMAT  | The bitmap specified by
*   <p lpbi> specifes a type of bitmap that is not supported.
* @flag  DISPLAYDIB_INVALIDTASK    | The caller is an inactive application.
*   <f DisplayDib> can only be called by an active application.
*
* @comm The <f DisplayDib> function displays bitmaps described with
*    the Windows 3.0 <t BITMAPINFO> data structure in either BI_RGB
*    or BI_RLE8 format; it does not support bitmaps described with
*    the OS/2 <t BITMAPCOREHEADER> data structure.
*
*    When <f DisplayDib> switches to a low-resolution display, it
*    disables the current display driver. As a result, you cannot use GDI
*    functions to update the display while <f DisplayDib> is displaying a
*    bitmap.
*
*
* History:
* 23-03-94 - StephenE - Created
*
\**************************************************************************/
UINT FAR PASCAL
DisplayDibEx(
    LPBITMAPINFOHEADER lpbi,
    int x,
    int y,
    LPSTR lpBits,
    UINT wFlags
    )
{
    DWORD       wNumColors;
    LONG        yExt;
    LONG        xExt;
    int         xScreen,yScreen;

    /*
    ** If not already done so:
    **      Register our class and Create our window "fullscreen"
    */
    if (wFlags & DISPLAYDIB_BEGIN) {

        dpf4(( "DISPLAYDIB_BEGIN..." ));

        return DisplayDibEnter( lpbi, wFlags );
    }

    /*
    ** Just testing return OK
    */
    else if (wFlags & DISPLAYDIB_TEST) {

        dpf1(( "lpbi->biCompression = 0x%X = %c%c%c%c",
                lpbi->biCompression,
                *((LPSTR)&lpbi->biCompression + 0),
                *((LPSTR)&lpbi->biCompression + 1),
                *((LPSTR)&lpbi->biCompression + 2),
                *((LPSTR)&lpbi->biCompression + 3) ));

        dpf4(( "DISPLAYDIB_TEST... returning OK" ));
        return DISPLAYDIB_NOERROR;
    }

    /*
    ** Palette change message
    */
    else if ( (wFlags & (DISPLAYDIB_NOWAIT | DISPLAYDIB_NOIMAGE)) ==
              (DISPLAYDIB_NOWAIT | DISPLAYDIB_NOIMAGE) ) {

        PALETTEENTRY    ape[256];
        LPRGBQUAD       lprgb;
        int             i;

        lprgb = (LPRGBQUAD) ((LPBYTE) lpbi + lpbi->biSize);

        for (i = 0; i < (int) lpbi->biClrUsed; i++) {
            ape[i].peRed = lprgb[i].rgbRed;
            ape[i].peGreen = lprgb[i].rgbGreen;
            ape[i].peBlue = lprgb[i].rgbBlue;
            ape[i].peFlags = 0;
        }

        DrawDibChangePalette(hdd, 0, (int)lpbi->biClrUsed, (LPPALETTEENTRY)ape);

        return DISPLAYDIB_NOERROR;
    }

    /*
    ** Time to kill the window and the class
    */
    else if (wFlags & DISPLAYDIB_END) {

        dpf4(( "DISPLAYDIB_END..." ));
        DisplayDibLeave( wFlags );
        return DISPLAYDIB_NOERROR;
    }

    /*
    ** Do the drawing here !!
    */
    else if ( !fStop ) {

        /*
        ** If we were'nt asked to draw anything just return.
        */
        if ( wFlags & DISPLAYDIB_NOIMAGE ) {
            return DISPLAYDIB_NOERROR;
        }

        xExt = lpbi->biWidth;
        yExt = lpbi->biHeight;

        if ( wFlags & DISPLAYDIB_ZOOM2 ) {

            xExt <<= 1;
            yExt <<= 1;
        }
        else if ( iMovieSizeMultiplier ) {

            xExt <<= iMovieSizeMultiplier;
            yExt <<= iMovieSizeMultiplier;
        }

        wNumColors  = lpbi->biClrUsed;
        if (wNumColors == 0 && lpbi->biBitCount <= 8) {
            wNumColors = 1 << (UINT)lpbi->biBitCount;
        }

        /*
        ** setup pointers
        */
        if (lpBits == NULL) {
            lpBits = (LPBYTE)lpbi + lpbi->biSize + wNumColors * sizeof(RGBQUAD);
        }

        /*
        **  center the image
        */
        if (!(wFlags & DISPLAYDIB_NOCENTER)) {

            xScreen = ((int)dxScreen - xExt) / 2;
            yScreen = ((int)dyScreen - yExt) / 2;
        }
        else {

            xScreen = 0;
            yScreen = 0;
        }

        dpf5(( "Drawing to the screen..." ));
        DrawDibDraw( hdd, hdcFullScreen,
                     xScreen, yScreen, xExt, yExt,
                     lpbi, lpBits,
                     0, 0, lpbi->biWidth, lpbi->biHeight,
                     DDF_SAME_HDC | DDF_SAME_DRAW );

        return DISPLAYDIB_NOERROR;
    }

    /*
    ** The user pressed a key... time to stop
    */
    else {

        dpf4(( "The keyboard hook is telling us to stop..." ));
        DisplayDibLeave( wFlags );
        return DISPLAYDIB_NOTSUPPORTED;
    }

}



/*****************************Private*Routine******************************\
* DisplayDibEnter
*
*
*
* History:
* 23-03-94 - StephenE - Created
*
\**************************************************************************/
UINT
DisplayDibEnter(
    LPBITMAPINFOHEADER lpbi,
    UINT wFlags
    )
{
    WNDCLASS    wc;
    HINSTANCE   hInst = GetModuleHandle( NULL );


    /*
    ** If our class isn't already registered with windows register it
    */
    fClassRegistered = GetClassInfo( hInst, TEXT("SJE_FULLSCREEN"), &wc );
    if ( fClassRegistered == FALSE ) {

        ZeroMemory( &wc, sizeof(wc) );

        wc.style         = CS_OWNDC;
        wc.lpfnWndProc   = FullScreenWndProc;
        wc.hInstance     = hInst;
        wc.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
        wc.lpszClassName = TEXT("SJE_FULLSCREEN");
        fClassRegistered = RegisterClass( &wc );
        dpf4(( "Class registered... %s", fClassRegistered ? "OK" : "FAILED" ));
    }


    if ( fClassRegistered ) {

        /*
        ** Do we already have a window ??
        */
        if ( hwndFullScreen == NULL ) {

            hwndFullScreen = CreateWindowEx(WS_EX_TOPMOST,
                                            TEXT("SJE_FULLSCREEN"),
                                            NULL,
                                            WS_POPUP,
                                            0, 0, 0, 0,
                                            NULL, NULL,
                                            hInst, NULL );

            dpf4(( "Window created... %s", hwndFullScreen ? "OK" : "FAILED" ));
        }

        if ( hwndFullScreen ) {

            LONG    yExt;
            LONG    xExt;
            MSG     msg;

            /*
            ** purge the queue of keyboard messages before installing
            ** the hook.
            */
            while ( PeekMessage( &msg, NULL, WM_KEYFIRST, WM_KEYLAST,
                                 PM_REMOVE | PM_NOYIELD ) );

            hHookK = SetWindowsHookEx( WH_KEYBOARD, KeyboardHookProc,
                                       GetModuleHandle(TEXT("DISPDB32.DLL")),
                                       0 );
            dpf4(( "Hook created... %s", hHookK ? "OK" : "FAILED" ));


            dxScreen = GetSystemMetrics( SM_CXSCREEN );
            dyScreen = GetSystemMetrics( SM_CYSCREEN );

            hdcFullScreen = GetDC( hwndFullScreen );
            hdd = DrawDibOpen();

            xExt = lpbi->biWidth;
            yExt = lpbi->biHeight;

            iMovieSizeMultiplier =
                DisplayCalcMovieMultiplier( xExt, yExt, lpbi->biCompression );

            if ( wFlags & DISPLAYDIB_ZOOM2 ) {

                xExt <<= 1;
                yExt <<= 1;
            }
            else if ( iMovieSizeMultiplier ) {

                xExt <<= iMovieSizeMultiplier;
                yExt <<= iMovieSizeMultiplier;
            }

            dpf1(( "Drawing at %d by %d... Flags = 0x%X", xExt, yExt, wFlags ));
            DrawDibBegin( hdd, hdcFullScreen, xExt, yExt,
                          lpbi, lpbi->biWidth, lpbi->biHeight, 0 );

            MoveWindow( hwndFullScreen, 0, 0, dxScreen, dyScreen, FALSE );
            ShowWindow( hwndFullScreen, SW_SHOW );
            UpdateWindow( hwndFullScreen );

            ShowCursor( FALSE );
            SetFocus( hwndFullScreen );
        }
    }

    fStop = FALSE;
    return hwndFullScreen != NULL ? DISPLAYDIB_NOERROR : DISPLAYDIB_NOTSUPPORTED;
}



/*****************************Private*Routine******************************\
* DisplayDibLeave
*
*
*
* History:
* 23-03-94 - StephenE - Created
*
\**************************************************************************/
void
DisplayDibLeave(
    UINT wFlags
    )
{
    if (hwndFullScreen) {
        DestroyWindow( hwndFullScreen );
        hwndFullScreen = NULL;
    }

}

/*****************************Private*Routine******************************\
* DisplayCalcMovieMultiplier
*
* Determines the largest movie that the display is capable of displaying.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
int
DisplayCalcMovieMultiplier(
    int cxOriginal,
    int cyOriginal,
    DWORD dwCompression
    )
{
    SYSTEM_INFO     SysInfo;
    int             iMult;
    int             iMultTemp;
    int             cxOriginalSave, cyOriginalSave;


    /*
    ** For now, don't try to stretch 'cvid' compressed movies.  This is
    ** because it looks crap!!
    */
    if ( *((LPSTR)&dwCompression + 0) == 'c'
      && *((LPSTR)&dwCompression + 1) == 'v'
      && *((LPSTR)&dwCompression + 2) == 'i'
      && *((LPSTR)&dwCompression + 3) == 'd' ) {

        return 0;
    }


    GetSystemInfo( &SysInfo );
    iMultTemp = iMult = 0;
    cxOriginalSave = cxOriginal;
    cyOriginalSave = cyOriginal;

    switch ( SysInfo.wProcessorArchitecture ) {

    case PROCESSOR_ARCHITECTURE_INTEL:
        if ( SysInfo.wProcessorLevel <= 3 ) {
            break;
        }

        /*
        ** maybe later we will do something different for i486's
        ** for now they just fall through to the RISC / Pentium default
        ** case below.
        */

    default:

        while ( (cxOriginal <= CX_MAX_MOVIE_DEFAULT)
             && (cyOriginal <= CY_MAX_MOVIE_DEFAULT) ) {

            iMult = iMultTemp;
            iMultTemp++;

            cxOriginal = cxOriginalSave << iMultTemp;
            cyOriginal = cyOriginalSave << iMultTemp;
        }
        break;
    }

    return iMult;
}


/******************************Public*Routine******************************\
* FullScreenWndProc
*
*
*
* History:
* 23-03-94 - StephenE - Created
*
\**************************************************************************/
LRESULT CALLBACK
FullScreenWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch ( message ) {

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            RECT        rc;

            dpf4(( "Window needs painting" ));
            BeginPaint( hwnd, &ps );
            GetUpdateRect( hwnd, &rc, FALSE );
            FillRect( hdcFullScreen, &rc, GetStockObject( BLACK_BRUSH ) );
            EndPaint( hwnd, &ps );
        }
        break;

    case WM_PALETTECHANGED:
        if ( (HWND)wParam == hwnd ) {
            break;
        }

        /* fall thru */

    case WM_QUERYNEWPALETTE:
        if ( DrawDibRealize( hdd, hdcFullScreen, FALSE ) > 0 ) {
            InvalidateRect( hwnd, NULL, TRUE );
        }
        break;

    case WM_DESTROY:
        dpf4(( "Window destroyed releasing DC" ));
        ReleaseDC( hwnd, hdcFullScreen );
        DrawDibEnd( hdd );
        DrawDibClose( hdd );

        UnregisterClass( TEXT("SJE_FULLSCREEN"), GetModuleHandle( NULL ) );

        fClassRegistered = FALSE;
        fStop = FALSE;

        ShowCursor( TRUE );
        UnhookWindowsHookEx( hHookK );
        break;

    default:
        return DefWindowProc( hwnd, message, wParam, lParam );
    }

    return (LRESULT)FALSE;
}



/******************************Public*Routine******************************\
* KeyboardHookProc
*
*
*
* History:
* 23-03-94 - StephenE - Created
*
\**************************************************************************/
LRESULT CALLBACK
KeyboardHookProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam
    )
{
    if ( nCode == HC_ACTION) {

        /*
        ** Only interested in keydowns
        */
        if ( !(lParam & 0x80000000) ) {
            dpf4(( "Stop requested from the keyboard hook" ));
            fStop = TRUE;
        }
    }

    return CallNextHookEx( hHookK, nCode, wParam, lParam );
}


#if DBG
/*****************************Private*Routine******************************\
* dprintf
*
* Standard debug out stuff
*
* History:
* 23-03-94 - StephenE - Created
*
\**************************************************************************/
void
dprintf(
    LPSTR lpszFormat,
    ...
    )
{
    char buf[512];
    UINT n;
    va_list va;

    n = wsprintfA(buf, "DISPDB32: (tid %x) ", GetCurrentThreadId());

    va_start(va, lpszFormat);
    n += wvsprintfA(buf+n, lpszFormat, va);
    va_end(va);

    buf[n++] = '\n';
    buf[n] = 0;
    OutputDebugStringA(buf);
}
#endif

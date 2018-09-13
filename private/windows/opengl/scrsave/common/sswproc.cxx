/******************************Module*Header*******************************\
* Module Name: sswproc.cxx
*
* Window procedure functions.
*
* Copyright (c) 1996 Microsoft Corporation
\**************************************************************************/

#include <windows.h>
#include <commdlg.h>
#include <scrnsave.h>
#include <GL\gl.h>
#include "tk.h"
#include <math.h>
#include <memory.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <time.h>

#include "ssintrnl.hxx"
#include "sswproc.hxx"
#include "palette.hxx"
#include "clear.hxx"

// forward declarations of internal functions

static void ss_TimerProc();
LRESULT SS_ScreenSaverProc(HWND, UINT, WPARAM, LPARAM);

LONG 
FullScreenPaletteManageProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void 
ssw_RelayMessageToChildren( PSSW pssw, UINT msg, WPARAM wParam, LPARAM lParam);

static void ssw_RealizePalette( PSSW pssw, BOOL bBackground );
static void ssw_DeletePalette( PSSW pssw );


/**************************************************************************\
* ScreenSaverProc
*
* Processes messages for the top level screen saver window.
*
* Unhandled msgs are sent to DefScreenSaverProc
\**************************************************************************/

LRESULT
ScreenSaverProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    static BOOL bInited = FALSE;
    static UINT idTimer = 0;
    // Draw timer time-out interval
#ifdef SS_DEBUG  
    static UINT uiTimeOut = 2;  // Let it rip !
#else
    static UINT uiTimeOut = 16; // Cap at ~60 fps
#endif
    static BOOL bSuspend = FALSE;
#ifdef SS_WIN95_TIMER_HACK
    static BOOL bIdle = FALSE;
#endif
    PSSW pssw;

    switch (message)
    {
        case WM_CREATE:
        case WM_ERASEBKGND:
        case SS_WM_INITGL:
            return SS_ScreenSaverProc( hwnd, message, wParam, lParam);

        case WM_ACTIVATE:
            if ( LOWORD(wParam) == WA_INACTIVE ) {
                SS_DBGMSG( "Main_Proc: WM_ACTIVATE inactive\n" );
                gpss->bInForeground = FALSE;
            } else {
                SS_DBGMSG( "Main_Proc: WM_ACTIVATE active\n" );
                gpss->bInForeground = TRUE;
            }

            // fall thru

        case WM_QUERYNEWPALETTE:
        case WM_PALETTECHANGED:
        case WM_SYSCOLORCHANGE:
        case SS_WM_PALETTE:
            return( MainPaletteManageProc( hwnd, message, wParam, lParam ) );

        case SS_WM_START:
            SS_DBGMSG( "Main_Proc: SS_WM_START\n" );
            // This is the main GL startup point. The global animation timer
            // is started, and SS_WM_START is relayed to the window chain.

//mf: kluge for 'delayed background paint' problem in preview mode - in
// floater type ss's, the main window gets this delayed bg paint thing that
// makes the floater obvious.  By relenquishing our time slice here, the
// problem goes away...
#if 1
            if( gpss->type == SS_TYPE_PREVIEW ) {
                Sleep(0);
            }
#endif

            // Initialize the animation timer - it should start up once we
            // return from here

            idTimer = 1;
            SetTimer(hwnd, idTimer, uiTimeOut, 0);

#ifdef SS_DEBUG
            // Start timer for calculating update rate
            gpss->timer.Start();
#endif
            if( !bInited )
                bInited = TRUE;

            // Process SS_WM_START for the window chain
            return SS_ScreenSaverProc( hwnd, message, wParam, lParam);

#ifdef SS_WIN95_TIMER_HACK
        case SS_WM_IDLE :
            if( wParam == SS_IDLE_ON )
                bIdle = TRUE;
            else if( wParam == SS_IDLE_OFF )
                bIdle = FALSE;
            break;
#endif

        case WM_DESTROY:
            if (idTimer) {
                KillTimer(hwnd, idTimer);
                idTimer = 0;
            }

            // Destroy any children of the top level window

            pssw = gpss->sswTable.PsswFromHwnd( hwnd );

            PSSW psswChild;
            psswChild = pssw == NULL ? NULL : pssw->psswChildren;

            while( psswChild ) {
                if( psswChild->hwnd )
                    DestroyWindow( psswChild->hwnd );
                psswChild = pssw->psswSibling;
            }

            // Handle any palette stuff

//mf: Before deleting this top level window, we need to use its
// still-valid dc to things like restore SystemPaletteUse mode.  Ideally this
// should be done after ~SSW has dumped GL, but ~SSW also releases the DC.
// If this is a problem, we can create a new function SSW::DeleteGL that just
// gets rid of the GL part

            if( gpss->pssPal && pssw)
                ssw_DeletePalette( pssw );

            // Dump the main pssw
            delete pssw;

            // All pssw's have now been deleted - remove global ptr to top of
            // window chain.
            gpss->psswMain = NULL;

            PostQuitMessage(0);
            return 0;

        case SS_WM_CLOSING:
            // mf:This message is sent when the screen saver receives a WM_CLOSE
            // msg, *after* any password protection routines.
            // For now, only sent in /s mode
            SS_DBGMSG( "Main_Proc: SS_WM_CLOSING\n" );

            if( gpss->bResSwitch ) {
                // pssw->GdiClear();
                // mf: untested for child window case
                // Restore previous display settings
                // Note that this is also checked for in ~SCRNSAVE, in
                // case this message is not hit.
                ChangeDisplaySettings(NULL, CDS_FULLSCREEN);
                gpss->bResSwitch = FALSE;
            }
            return 0;

        case WM_SETFOCUS:
            SS_DBGMSG( "Main_Proc: WM_FOCUS\n" );
            //mf: this catches some of the win95 passwd dialog problems, where
            // we don't get repaint msgs when dialogs end
            if( ss_fOnWin95() && ss_fFullScreenMode() ) {
                pssw = gpss->sswTable.PsswFromHwnd( hwnd );
                if (pssw != NULL)
                    pssw->Repaint( FALSE );
            }
            break;

#if DBG
        case WM_SHOWWINDOW:
            SS_DBGMSG( "Main_Proc: WM_SHOWWINDOW\n" );
            break;
#endif

        case WM_PAINT:
            SS_DBGMSG( "Main_Proc: WM_PAINT\n" );

            pssw = gpss->sswTable.PsswFromHwnd( hwnd );
            if (pssw != NULL)
                pssw->Repaint( TRUE );

#ifdef SS_DO_PAINT
            // We do the painting rather than letting the system do it
            hdc = BeginPaint(hwnd, &ps);

            // This is case where bg brush is NULL and we have to do repaint
            // We only do it after bInited, as this will avoid the first
            // WM_PAINT for the entire window.
            if( bInited )
                DrawGdiRect( hdc, gpss->hbrBg, &ps.rcPaint );
            EndPaint(hwnd, &ps);
#endif // SS_DO_PAINT

#ifdef SS_DELAYED_START_KLUGE
            if( !bInited && SS_DELAY_START(gpss->type) ) {
                bInited = TRUE;

                // Do initial GL configuration
                PostMessage( hwnd, SS_WM_INITGL, 0, 0 );
                // Start drawing
                PostMessage( hwnd, SS_WM_START, 0, 0 );
            }
#endif // SS_DELAYED_START_KLUGE

            if( pssw->iSubWindow ) {
                // If this window has sub windows, mark the bg for validation,
                // since for hardware double buffered schemes, Swapbuffers
                // may swap in ugly garbage.
                pssw->bValidateBg = TRUE;
            }

#ifdef SS_DO_PAINT
            return 0; // painting has been handled by us
#endif // SS_DO_PAINT

            break;

        case WM_SIZE:
            // Suspend drawing if minimized
            if( wParam == SIZE_MINIMIZED )
                bSuspend = TRUE;
            else  // either SIZE_RESTORED or SIZE_MAXIMIZED
                bSuspend = FALSE;

            return SS_ScreenSaverProc( hwnd, message, wParam, lParam);

        case WM_MOVE:
            SS_DBGMSG( "Main_Proc: WM_MOVE\n" );
            // See note for WM_PAINT for subWindows
            pssw = gpss->sswTable.PsswFromHwnd( hwnd );
            if (pssw == NULL)
                break;

            //mf: kluge for non-floater workaround : see above
            if( pssw->iSubWindow ) 
                pssw->bValidateBg = TRUE;

            break;

        case WM_TIMER:
            if( bSuspend )
                return 0;

#ifdef SS_WIN95_TIMER_HACK
            if( bIdle ) {
                // We are in an idle state, and don't want to flood the queue
                // with WM_TIMER mesages.  So we kill the timer, do our
                // drawing, then start another timer.

                // Kill current timer
                if (idTimer)
                    KillTimer(hwnd, idTimer);
                else
                    // unlikely, but what the hay
                    return 0;
            }
#endif

            ss_TimerProc();

#ifdef SS_WIN95_TIMER_HACK
            if( bIdle ) {
                // Start another animation timer after we've done drawing
                SetTimer(hwnd, idTimer, uiTimeOut, 0);
            }
#endif
            return 0;
    }

    return DefScreenSaverProc(hwnd, message, wParam, lParam);
}

/**************************************************************************\
* SS_ScreenSaverProc
*
* Wndproc for child windows, and some messages from top-level window
*
* Unhandled msgs are sent to DefWindowProc
\**************************************************************************/

LRESULT 
SS_ScreenSaverProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    int i;
    int retVal;
    PSSW pssw;

    switch (message)
    {
        case WM_CREATE:
            SS_DBGMSG1( "SS_Proc: WM_CREATE for 0x%x\n", hwnd );

            pssw = (PSSW) ( ((LPCREATESTRUCT)lParam)->lpCreateParams ); 
            gpss->sswTable.Register( hwnd, pssw );
  
            pssw->size.width  = ((LPCREATESTRUCT)lParam)->cx;
            pssw->size.height = ((LPCREATESTRUCT)lParam)->cy;
            pssw->hwnd = hwnd;
            break;

        case SS_WM_INITGL:
            SS_DBGMSG1( "SS_Proc: SS_WM_INITGL for 0x%x\n", hwnd );

            pssw = gpss->sswTable.PsswFromHwnd( hwnd );
            pssw->InitGL();

            break;

#ifdef SS_MULTI_WINDOW_TIMERS
// Enable this section and fill it in if windows start their own animation
// timers.  For now, there is just one timer on the main window.
        case SS_WM_START:
            SS_DBGMSG1( "SS_Proc: SS_WM_START for 0x%x\n", hwnd );

            pssw = gpss->sswTable.PsswFromHwnd( hwnd );

            // Send SS_WM_START to any children of this window
            ssw_RelayMessageToChildren( pssw, SS_WM_START, 0, 0 ); 

            // Nothing really to do here yet...

            break;
#endif

        case SS_WM_PALETTE:
            return( MainPaletteManageProc( hwnd, message, wParam, lParam ) );

        case WM_DESTROY:
            pssw = gpss->sswTable.PsswFromHwnd( hwnd );

            SS_DBGMSG1( "SS_Proc: WM_DESTROY for 0x%x\n", hwnd );
            // Kill off any children of this window first

            PSSW psswChild;
            psswChild = pssw == NULL ? NULL : pssw->psswChildren;

            while( psswChild ) {
                if( psswChild->hwnd )
                    DestroyWindow( psswChild->hwnd );
                else
                    delete psswChild;
                psswChild = pssw->psswSibling;
            }
            // Delete the pssw - this does all necessary cleanup

            delete pssw;
            break;

        case WM_ERASEBKGND:
            SS_DBGMSG1( "SS_Proc: WM_ERASEBKGRND for 0x%x\n", hwnd );
#if 0
            // If eventually we want to control bg erasing...
          {
            BOOL bEraseNow = TRUE; // ! 0 or 1 have same effect !
            if( bEraseNow ) {
                // If bg for the window is NULL, ? should erase here ?
                return TRUE;
            }
            else
                return 0; // window remains marked for erasing
          }
#else
            pssw = gpss->sswTable.PsswFromHwnd( hwnd );
            if (pssw != NULL)
                pssw->Repaint( FALSE );
            // Don't process this message
            return DefWindowProc(hwnd, message, wParam, lParam);
#endif

        case WM_PAINT:
            // We get this msg every time window moves, since SWP_NOCOPYBITS is
            // specified with the window move.
            hdc = BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            break;

        case WM_SIZE:
            pssw = gpss->sswTable.PsswFromHwnd( hwnd );
            if (pssw != NULL)
                pssw->Resize( LOWORD(lParam), HIWORD(lParam) );
            break;

        // these msg's are never received by the child window ?
        case WM_ACTIVATE:
        case WM_QUERYNEWPALETTE:
        case WM_PALETTECHANGED:
            return( MainPaletteManageProc( hwnd, message, wParam, lParam ) );


        case WM_SYSCOMMAND:
        case WM_SETCURSOR:
        case WM_ACTIVATEAPP:
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_POWERBROADCAST:
        case WM_POWER:
            pssw = gpss->sswTable.PsswFromHwnd( hwnd );
            if (pssw == NULL)
                return DefScreenSaverProc(hwnd, message, wParam, lParam);
            else
                return DefScreenSaverProc(pssw->psswParent->hwnd, message, wParam, lParam);

        default: 
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}


#ifdef SS_DEBUG
/**************************************************************************\
* PrintUpdateRate
*
* Print number of updates per second in the title bar
*
\**************************************************************************/

static void
PrintUpdateRate( double elapsed, long updateCount )
{
    char buf[100];
    double updateRate;

    if( elapsed == 0.0 )
        updateRate = 0.0;
    else
        updateRate = updateCount / elapsed;

    sprintf( buf, "Updates per second = %4.1f", updateRate );
    SendMessage(gpss->psswMain->hwnd, WM_SETTEXT, 0, (LPARAM)buf);
}
#endif

/**************************************************************************\
* ss_TimerProc
*
* Every time a timer event fires off, update all active windows
*
\**************************************************************************/

static void 
ss_TimerProc()
{
    static int busy = FALSE;
    int i;
#ifdef SS_DEBUG
    static long updateCount = 0;
    static double updateInterval = 2.0;
    SS_TIMER *pTimer = &gpss->timer;
#endif

    if (busy)
        return;
    busy = TRUE;

    gpss->psswMain->UpdateWindow();

#ifdef SS_DEBUG
    updateCount++;
    if( gpss->bDoTiming && 
        (( (double) pTimer->ElapsedTime() ) >= updateInterval) ) 
    {
        double elapsed = pTimer->Stop();
        PrintUpdateRate( elapsed, updateCount );
        updateCount = 0;
        pTimer->Start();
    }
#endif

    busy = FALSE;
}

/**************************************************************************\
* RelayMessageToChildren
*
* Pass along the message to any child windows
\**************************************************************************/

static void
ssw_RelayMessageToChildren( PSSW pssw, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PSSW psswChild = pssw->psswChildren;

    while( psswChild ) {
        if( psswChild->hwnd )
            SendMessage( psswChild->hwnd, msg, wParam, lParam );
        psswChild = psswChild->psswSibling;
    }
}


/**************************************************************************\
* UpdateDIBColorTable
*
* Wrapper for SSDIB_UpdateColorTable.  
*
* This controls the hPal parameter for SSDIB_UpdateColorTable.
*
\**************************************************************************/

void
ssw_UpdateDIBColorTable( HDC hdcbm, HDC hdcwin )
{
    SS_PAL *pssPal = gpss->pssPal;

    if( !pssPal )
        return;
#if 0
    HPALETTE hpal = pssPal->bTakeOver ? pssPal->hPal : NULL;
#else
    HPALETTE hpal =  pssPal->hPal;
#endif

    SSDIB_UpdateColorTable( hdcbm, hdcwin, hpal );
}


/**************************************************************************\
* RealizePalette
*
\**************************************************************************/

static void
ssw_RealizePalette( PSSW pssw, BOOL bBackground )
{
    // assumed pssPal valid if get here
    SS_PAL *pssPal = gpss->pssPal;

    if( !pssw->hrc ) {
        // Can assume this window doesn't need to worry about palettes, but
        // if any of its children are subWindows, it will have to take care
        // of it *for* them.
        if( ! pssw->iSubWindow )
            return; // no hrc and no subWindow children
    }
    pssPal->Realize( pssw->hwnd, pssw->hdc, bBackground );

    if( pssw->pStretch && pssw->pStretch->ssbm.hdc ) {
        SS_BITMAP *pssbm = &pssw->pStretch->ssbm;
        ssw_UpdateDIBColorTable( pssbm->hdc, pssw->hdc );
    }
}

/**************************************************************************\
* ssw_DeletePalette
*
\**************************************************************************/

static void
ssw_DeletePalette( PSSW pssw )
{
    SS_PAL *pssPal = gpss->pssPal;

    if( pssPal->bTakeOver ) {
        // We took over the system palette - make a note of this
        // for any special ss termination conditions.
        gpss->flags |= SS_PALETTE_TAKEOVER;
    }
    pssPal->SetDC( pssw->hdc );
    delete pssPal;
    gpss->pssPal = NULL;
}

/**************************************************************************\
* PaletteManage Procs
\**************************************************************************/

/* palette related msgs's:
    - WM_ACTIVATE:
        The WM_ACTIVATE message is sent when a window is being activated or 
        deactivated. This message is sent first to the window procedure of 
        the top-level window being deactivated; it is then sent to the 
        window procedure of the top-level window being activated. 

    - WM_QUERYNEWPALETTE:
        The WM_QUERYNEWPALETTE message informs a window that it is about 
        to receive the keyboard focus, giving the window the opportunity 
        to realize its logical palette when it receives the focus. 

        If the window realizes its logical palette, it must return TRUE; 
        otherwise, it must return FALSE. 

    - WM_PALETTECHANGED:
        The WM_PALETTECHANGED message is sent to all top-level and overlapped 
        windows after the window with the keyboard focus has realized its 
        logical palette, thereby changing the system palette. This message 
        enables a window that uses a color palette but does not have the 
        keyboard focus to realize its logical palette and update its client 
        area. 

        This message must be sent to all top-level and overlapped windows, 
        including the one that changed the system palette. If any child 
        windows use a color palette, this message must be passed on to them 
        as well. 
        To avoid creating an infinite loop, a window that receives this 
        message must not realize its palette, unless it determines that 
        wParam does not contain its own window handle. 

    - WM_SYSCOLORCHANGE:
        The WM_SYSCOLORCHANGE message is sent to all top-level windows when 
        a change is made to a system color setting. 

    - SS_WM_PALETTE:
        Internal msg.  Uses:
        - In fullscreen mode, we send this from Main wndproc to main 
          window's children on WM_ACTIVATE.
        - When this is received in SS_ScreenSaverProc, if fullscreen,
          it does:
                    UnrealizeObject( pssPal->hPal );
                    RealizePalette( hdc );
          otherwise, it is passed to PaletteManageProc, where
          Realize is called (for 'floater' windows to realize 
          their palettes).
        - It is also sent by DelayPaletteRealization() when it can't get
          the system palette.

*/


/**************************************************************************\
* MainPaletteManageProc
*
* Top-level palette management proc.

* Returns immediately if no palette set - otherwise calls through
* paletteManageProc function pointer
*
\**************************************************************************/

LONG 
MainPaletteManageProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if( !gpss->pssPal )
        // No palette management required
        return 0;

    // else call approppriate palette manage proc
    return (*gpss->pssPal->paletteManageProc)(hwnd, message, wParam, lParam);
}

LONG 
NullPaletteManageProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

/**************************************************************************\
* FullScreenPaletteManageProc
*
* Processes messages relating to palette management in full screen mode.
*
\**************************************************************************/

LONG 
FullScreenPaletteManageProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    SS_PAL *pssPal;
    PSSW pssw;

    switch (message)
    {
        case WM_ACTIVATE:

#if SS_DEBUG
            if ( LOWORD(wParam) == WA_INACTIVE )
                SS_DBGMSG1( "FullScreen_PMProc: WM_ACTIVATE : inactive for 0x%x\n",
                           hwnd );
            else
                SS_DBGMSG1( "FullScreen_PMProc: WM_ACTIVATE : active for 0x%x\n",
                           hwnd );
#endif

            // !! This msg is only sent to main top level window
            pssw = gpss->sswTable.PsswFromHwnd( hwnd );

            pssPal = gpss->pssPal;
            if ( pssPal->bUseStatic ) {
                HDC hdc = pssw->hdc; // hdc *always* valid for top-level pssw
                // Note: wParam = 0 when window going *inactive*
                SetSystemPaletteUse( hdc, wParam ? SYSPAL_NOSTATIC
                                                : pssPal->uiOldStaticUse);
            }

            // Send SS_WM_PALETTE msg to main window
            SendMessage( hwnd, SS_WM_PALETTE, wParam, 0);
            break;

        case SS_WM_PALETTE:

            SS_DBGMSG1( "FullScreen_PMProc: SS_WM_PALETTE for 0x%x\n", hwnd );

            pssw = gpss->sswTable.PsswFromHwnd( hwnd );

            ssw_RelayMessageToChildren( pssw, SS_WM_PALETTE, wParam, 0 );

            HDC hdc;
            if( hdc = pssw->hdc )
            {
                pssPal = gpss->pssPal;

//mf: this should call thru ssw_RealizePalette for bitmap case ? (for now
// don't need to, since we take over palette...)
                // This resets the logical palette, causing remapping of
                // logical palette to system palette
           // mf: !!! ?? how come no dc with UnrealizeObject ?  does that
                // mean its done once per app, not per child window ?
            // yeah, we should move this up...
                UnrealizeObject( pssPal->hPal );
                RealizePalette( hdc );
            }
            break;
    }
    return 0;
}

/**************************************************************************\
* PaletteManageProc
*
* Processes messages relating to palette management for the general case.
*
* Note: this msg handling strategy is based loosely on the tk, so any changes 
* there should be reflected here
\**************************************************************************/

LONG 
PaletteManageProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // One global palette for all windows
    if( !gpss->pssPal )
        return 0;

    switch (message)
    {
      case WM_ACTIVATE:

        SendMessage( hwnd, SS_WM_PALETTE, gpss->bInBackground(), 0);

        // Allow DefWindowProc() to finish the default processing (which 
        // includes changing the keyboard focus).

        break;

      case WM_QUERYNEWPALETTE:

        SS_DBGMSG1( "Palette_Proc: WM_QUERYNEWPALETTE for 0x%x\n", hwnd );
        // We don't actually realize palette here (we do it at WM_ACTIVATE
        // time), but we need the system to think that we have so that a
        // WM_PALETTECHANGED message is generated.
//mf: why can't we just realize here ? and who wants even more messages to
// be generated !!! :)

        // This is the only msg preview mode gets wrt palettes !
        if( !ss_fPreviewMode() )
            return (1);

        // We are in preview mode - realize the palette
        SendMessage( hwnd, SS_WM_PALETTE, gpss->bInBackground(), 0);
        break;

      case WM_PALETTECHANGED:

        SS_DBGMSG1( "Palette_Proc: WM_PALETTECHANGED for 0x%x\n", hwnd );
        // Respond to this message only if the window that changed the palette
        // is not this app's window.

        // We are not the foreground window, so realize palette in the
        // background.  We cannot call Realize to do this because
        // we should not do any of the gbUseStaticColors processing while
        // in background.

        // Actually, we *can* be the fg window, so don't realize if
        // we're in foreground

        if( (hwnd != (HWND) wParam) && gpss->bInBackground() )
            SendMessage( hwnd, SS_WM_PALETTE, TRUE, 0);

        break;

      case WM_SYSCOLORCHANGE:

        // If the system colors have changed and we have a palette
        // for an RGB surface then we need to recompute the static
        // color mapping because they might have been changed in
        // the process of changing the system colors.

          SS_DBGMSG1( "Palette_Proc: WM_SYSCOLORCHANGE for 0x%x\n", hwnd );
          gpss->pssPal->ReCreateRGBPalette();
          SendMessage( hwnd, SS_WM_PALETTE, gpss->bInBackground(), 0);
          break;
            
      case SS_WM_PALETTE:

          SS_DBGMSG2( "Palette_Proc: SS_WM_PALETTE for 0x%x, bg = %d\n", 
                          hwnd, wParam );


          // Realize palette for this window and its children
          // wParam = TRUE if realize as bg

          PSSW pssw = gpss->sswTable.PsswFromHwnd( hwnd );
          ssw_RelayMessageToChildren( pssw, message, wParam, lParam ); 
          ssw_RealizePalette( pssw, (BOOL)wParam );
          break;
    }
    return 0;
}

/******************************Public*Routine******************************\
* GLScreenSaverConfigureDialog
*
* This is a wrapper for ScreenSaverConfigureDialog, which is the main dialog
* proc for all the GL screen savers in config mode.
* 
* We call the client's ss_ConfigInit() routine on the first WM_PAINT, since
* the dialog will have focus at this point (can realize palette) and all
* buttons should have been created.
\**************************************************************************/

BOOL
GLScreenSaverConfigureDialog( HWND hDlg, UINT msg, WPARAM wParam,
                              LPARAM lParam )
{
    static BOOL bInited = 0;

    switch( msg ) {
        case WM_INITDIALOG :
          {
            SS_DBGMSG( "GLScreenSaverConfigureDialog: WM_INITDIALOG\n" );
            // Create wrapper pssw for the dialog box

            PSSW pssw;
            pssw = new SSW( NULL,       // ssw parent
                            hDlg
                          );
            SS_ASSERT( pssw, "GLScreenSaverConfigureDialog : alloc failure for psswMain\n" );

            gpss->psswMain = pssw;

            // Load any resource strings common to all the dialogs
            BOOL bStringsLoaded = ss_LoadTextureResourceStrings();
            // If this doesn't work, things are seriously wrong and we
            // shouldn't continue
            SS_ASSERT( bStringsLoaded, "GLScreenSaverConfigureDialog : failure loading common resource strings\n" );
          }
          break;

        case WM_PAINT:

            if( !bInited ) {
                // Call client's ss_ConfigInit()
                if( !ss_ConfigInit( hDlg ) ) {
                    SS_WARNING( "ConfigInit failed\n" );
                    // Send WM_CLOSE to the dialog - this will enable any
                    // cleanup code to be called by the client
                    SendMessage( hDlg, WM_CLOSE, 0, 0l );
                }
                bInited = TRUE;
            }
            break;

        case WM_ACTIVATE:
        case WM_QUERYNEWPALETTE:
        case WM_PALETTECHANGED:
        case SS_WM_PALETTE:
            return( MainPaletteManageProc( hDlg, msg, wParam, lParam ) );

    }
    return ScreenSaverConfigureDialog( hDlg, msg, wParam, lParam );
}

/******************************Public*Routine******************************\
* SSW_TABLE constructor
*
\**************************************************************************/

SSW_TABLE::SSW_TABLE()
{
    nEntries = 0;
}

/******************************Public*Routine******************************\
* Register
*
* Register a HWND/PSSW pair.
\**************************************************************************/

void
SSW_TABLE::Register( HWND hwnd, PSSW pssw )
{
    SSW_TABLE_ENTRY *pEntry;

    // Check if already in table
    if( PsswFromHwnd( hwnd ) )
        return;

    // put hwnd/pssw pair in the table
    pEntry = &sswTable[nEntries];
    pEntry->hwnd = hwnd;
    pEntry->pssw = pssw;
    nEntries++;
}

/******************************Public*Routine******************************\
* PsswFromHwnd
*
* Return PSSW for the HWND
\**************************************************************************/

PSSW
SSW_TABLE::PsswFromHwnd( HWND hwnd )
{
    int count = nEntries;
    SSW_TABLE_ENTRY *pEntry = sswTable;

    while( count-- ) {
        if( pEntry->hwnd == hwnd )
            return pEntry->pssw;
        pEntry++;
    }
    return NULL;
}

/******************************Public*Routine******************************\
* Remove
*
* Remove HWND/PSSW entry from table
\**************************************************************************/

BOOL
SSW_TABLE::Remove( HWND hwnd )
{
    SSW_TABLE_ENTRY *pEntry = sswTable;

    // Locate the hwnd/pssw pair

    for( int count = 0 ; count < nEntries ; count++, pEntry++ ) {
        if( pEntry->hwnd == hwnd )
            break;
    }

    if( count == nEntries )
        // couldn't find it in the table
        return FALSE;

    // Remove entry / shuffle up other entries
    for( int i = count; i < nEntries-1; i ++ ) {
        sswTable[i] = sswTable[i+1];
    }

    nEntries--;
    return TRUE;
}

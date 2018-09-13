//----------------------------------------------------------------------------
//
// SCRNSAVE.C      --    skeleton for screen saver application
//
//    4/5/94 francish   merged NT and Win4 saver code, folded in SCRNSAVE.SCR
//
//  - 3/14/96: marcfo   Pulled this file in from shell\control\scrnsave\common.
//             All changes marked with GL_SCRNSAVE.
//----------------------------------------------------------------------------

#define GL_SCRNSAVE 1

#define WIN31
#include <windows.h>
#include <windowsx.h>
#include "scrnsave.h"
#include <regstr.h>
#include    <imm.h>
#ifdef GL_SCRNSAVE
#include "glscrnsv.h"
#endif

#define DBG_MSGS    0

const TCHAR szScreenSaverKey[] = REGSTR_PATH_SCREENSAVE;
TCHAR szPasswordActiveValue[] = REGSTR_VALUE_USESCRPASSWORD;
const TCHAR szPasswordValue[] = REGSTR_VALUE_SCRPASSWORD;
TCHAR szPwdDLL[] = TEXT("PASSWORD.CPL");
CHAR szFnName[] = "VerifyScreenSavePwd";        // Proc name, must be ANSI
TCHAR szImmDLL[] = TEXT("IMM32.DLL");
CHAR szImmFnc[] = "ImmAssociateContext";        // Proc name, must be ANSI
#if 0
TCHAR szCoolSaverHacks[] = REGSTR_PATH_SETUP TEXT("\\Screen Savers");
TCHAR szMouseThreshold[] = TEXT("Mouse Threshold");
TCHAR szPasswordDelay[] = TEXT("Password Delay");
#endif

typedef BOOL (FAR PASCAL * VERIFYPWDPROC) (HWND);
typedef HIMC (FAR PASCAL * IMMASSOCPROC) (HWND,HIMC);


//----------------------------------------------------------------------------
// variables declared in SCRNSAVE.H
HINSTANCE hMainInstance = 0;
HWND hMainWindow = 0;
BOOL fChildPreview = FALSE;



//----------------------------------------------------------------------------
// other globals
POINT ptMouse;
BOOL fClosing = FALSE;
BOOL fCheckingPassword = FALSE;
HINSTANCE hInstPwdDLL = NULL;
VERIFYPWDPROC VerifyPassword = NULL;
static BOOL preview_like_fullscreen = FALSE;
static UINT uShellAutoPlayQueryMessage = 0;
HINSTANCE hInstImm = NULL;
IMMASSOCPROC ImmFnc = NULL;
HIMC hPrevImc = (HIMC)0L;


static BOOL fOnWin95 = FALSE;  //TRUE if on Chicago, FALSE if on Cairo

//----------------------------------------------------------------------------
// random junk
DWORD dwWakeThreshold = 4;  //default to slight movement
DWORD dwPasswordDelay = 0;
DWORD dwBlankTime = 0;
#define MAX_PASSWORD_DELAY_IN_SECONDS (60)

//----------------------------------------------------------------------------
// forward declarations of internal fns
#ifndef GL_SCRNSAVE
// These are hooked out to glscrnsv.cxx
static INT_PTR DoScreenSave( HWND hParent );
static INT_PTR DoConfigBox( HWND hParent );
#endif
static INT_PTR DoSaverPreview( LPCTSTR szUINTHandle );
static INT_PTR DoChangePw( LPCTSTR szUINTHandle );
static BOOL DoPasswordCheck( HWND hParent );
VOID LoadPwdDLL(VOID);
VOID UnloadPwdDLL(VOID);


//----------------------------------------------------------------------------
// helper for time
static DWORD
GetElapsedTime(DWORD from, DWORD to)
{
    return (to >= from)? (to - from) : (1 + to + (((DWORD)-1) - from));
}

//----------------------------------------------------------------------------
// helper to convert text to unsigned int
static UINT_PTR
atoui( LPCTSTR szUINT )
{
   UINT_PTR uValue = 0;

   while( ( *szUINT >= TEXT('0') ) && ( *szUINT <= TEXT('9') ) )
      uValue = ( ( uValue * 10 ) + ( *szUINT++ - TEXT('0') ) );

   return uValue;
}


//----------------------------------------------------------------------------
// Local reboot and hotkey control (on Win95)
static void
HogMachine( BOOL value )
{
    BOOL dummy;

    //
    // NT is always secure, therefore we don't need to call this on Cairo/NT
    //
    if (fOnWin95) {
        SystemParametersInfo( SPI_SCREENSAVERRUNNING, value, &dummy, 0 );
    }
}


//----------------------------------------------------------------------------
// entry point (duh)
INT_PTR PASCAL
WinMainN( HINSTANCE hInst, HINSTANCE hPrev, LPTSTR szCmdLine, int nCmdShow )
{
   LPCTSTR pch = szCmdLine;
   HWND hParent = 0;
   OSVERSIONINFO osvi;

   hMainInstance = hInst;

   osvi.dwOSVersionInfoSize = sizeof(osvi);
   fOnWin95 = (GetVersionEx(&osvi) &&
                osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);

#ifdef GL_SCRNSAVE
   //
   // the shell sends this message to the foreground window before running an
   // AutoPlay app. we return 1 to cancel autoplay if we are password protected
   //
   if (fOnWin95) {
        uShellAutoPlayQueryMessage = RegisterWindowMessage(TEXT("QueryCancelAutoPlay"));
   } else {
        uShellAutoPlayQueryMessage = 0;
   }
#endif

   _try
   {
      for(;;) switch( *pch )
      {
          case TEXT('S'):
          case TEXT('s'):
              return DoScreenSave( NULL );

#ifdef GL_SCRNSAVE
          case TEXT('W'):
          case TEXT('w'):
              do pch++; while( *pch == TEXT(' ') );  // size parameters
              return DoWindowedScreenSave( pch );
#endif

          case TEXT('L'):
          case TEXT('l'):
              // special switch for tests such as WinBench
              // this is NOT a hack to make bechmarks look good
              // it's a hack to allow you to benchmark a screen saver
              // many bechmarking apps require the whole screen in foreground
              // which makes it hard to measure how a screensaver adds CPU load
              // you must provide a parent window (just like preview mode)
              preview_like_fullscreen = TRUE;
          case TEXT('P'):
          case TEXT('p'):
              do pch++; while( *pch == TEXT(' ') );  // skip to the good stuff
              return DoSaverPreview( pch );

          case TEXT('A'):
          case TEXT('a'):
              if (!fOnWin95)
                  return -1;
              do pch++; while( *pch == TEXT(' ') );  // skip to the good stuff
              return DoChangePw( pch );

          case TEXT('C'):
          case TEXT('c'): {
              HWND hwndParent = NULL
              ;
              // Look for optional parent window after the "C",
              // syntax is "C:hwnd_value"
              if (*(++pch) == TEXT(':')) {
                   hwndParent = (HWND)atoui( ++pch );
              }

              if (hwndParent == NULL || !IsWindow(hwndParent))
                   hwndParent = GetForegroundWindow();

              return DoConfigBox( hwndParent );
          }

          case TEXT('\0'):
              return DoConfigBox( NULL );

          case TEXT(' '):
          case TEXT('-'):
          case TEXT('/'):
              pch++;   // skip spaces and common switch prefixes
              break;

          default:
              return -1;
      }
   }
   _except(UnhandledExceptionFilter(GetExceptionInformation()))
   {
      // don't leave local reboot and hotkeys disabled on Win95
      HogMachine( FALSE );
   }

}


//----------------------------------------------------------------------------
// default screen-saver proc, declared in SCRNSAVE.H
// intended to be called by the consumer's ScreenSaverProc where
// DefWindowProc would normally be called
LRESULT WINAPI
DefScreenSaverProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
#if DBG_MSGS
    TCHAR szBuff[80];

    wsprintf( szBuff, TEXT("*** DefSSP received:\t0x%04lx 0x%08lx 0x%08lx\n"), uMsg, wParam, lParam );
    OutputDebugString(szBuff);
#endif

   if( !fChildPreview && !fClosing )
   {
      switch( uMsg )
      {
         case WM_CLOSE:
            //
            // Only do password check if on Windows 95.  WinNT (Cairo) has
            // the password check built into the security desktop for
            // C2 compliance.
            //
            if (fOnWin95) {
                if( !DoPasswordCheck( hWnd ) )
                {
                    GetCursorPos( &ptMouse );  // re-establish
                    return FALSE;
                }
            }
#ifdef GL_SCRNSAVE
            // We need to know when we're being terminated, so we can do
            // various clean-up stuff
            SendMessage( hWnd, SS_WM_CLOSING, 0, 0 );
#endif
            break;

         case SCRM_VERIFYPW:
            if (fOnWin95)
                return ( VerifyPassword? (LRESULT)VerifyPassword( hWnd ) : 1L );
            break;

         default:
         {
            POINT ptMove, ptCheck;

            if( fCheckingPassword )
                break;

            switch( uMsg )
            {
                case WM_SHOWWINDOW:
                    if( (BOOL)wParam )
                    SetCursor( NULL );
                    break;

                case WM_SETCURSOR:
                    SetCursor( NULL );
                    return TRUE;

                case WM_MOUSEMOVE:
                    GetCursorPos( &ptCheck );
                    if( ( ptMove.x = ptCheck.x - ptMouse.x ) && ( ptMove.x < 0 ) )
                        ptMove.x *= -1;
                    if( ( ptMove.y = ptCheck.y - ptMouse.y ) && ( ptMove.y < 0 ) )
                        ptMove.y *= -1;
                    if( ((DWORD)ptMove.x + (DWORD)ptMove.y) > dwWakeThreshold )
                    {
                        PostMessage( hWnd, WM_CLOSE, 0, 0l );
                        ptMouse = ptCheck;
                    }
                    break;

                case WM_POWERBROADCAST:
                    switch (wParam)
                    {
                    case PBT_APMRESUMECRITICAL:
                    case PBT_APMRESUMESUSPEND:
                    case PBT_APMRESUMESTANDBY:
                    case PBT_APMRESUMEAUTOMATIC:
                        // If the system is resuming from a real suspend
                        // (as opposed to a failed suspend) deactivate
                        // the screensaver.
                        if ((lParam & PBTF_APMRESUMEFROMFAILURE) == 0)
                            goto PostClose;
                        break;
		    default:
			// The standard screensaver code shuts down on
			// all power broadcast messages.  This doesn't
			// make much sense, but match the behavior so
			// that all screensavers operate the same way.
			goto PostClose;
                    }
                    break;

                case WM_POWER:
                    //
                    // a critical resume does not generate a WM_POWERBROADCAST
                    // to windows for some reason, but it does generate an old
                    // WM_POWER message.
                    //
                    if (wParam == PWR_CRITICALRESUME)
                        goto PostClose;
                    break;

                case WM_ACTIVATEAPP:
                    if( wParam ) break;
                case WM_LBUTTONDOWN:
                case WM_MBUTTONDOWN:
                case WM_RBUTTONDOWN:
                case WM_KEYDOWN:
                case WM_SYSKEYDOWN:
PostClose:
                    PostMessage( hWnd, WM_CLOSE, 0, 0l );
                    break;
            }
         }
      }
   }

   //
   // the shell sends this message to the foreground window before running an
   // AutoPlay app. On Win95, we return 1 to cancel autoplay if we are password protected
   //
   // On WinNT, secure screen savers run on a secure separate desktop, and will never see
   // this message, therefore, this code will never get executed.
   //
   //
   // BUGBUG -
   // On NT we don't want to take down the screen saver unless it is running
   // on the same desktop as the autoplay shell.  There is code in the
   // NT autoplay shell that looks for this and does not run the app if
   // that is the case; however, I not positive that the uShellAutoPlayQueryMessage
   // will not go between desktops.  (BradG assures me that it will not, but you
   // never know.)  If secure screensavers on NT randomly close when you put
   // an autoplay cd in the drive, then this code should be examined closely.
   //
   if ((uMsg == uShellAutoPlayQueryMessage) && uMsg)
   {
      PostMessage(hWnd, WM_CLOSE, 0, 0L);
      return (VerifyPassword != NULL);
   }

   return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

//----------------------------------------------------------------------------
// This window procedure takes care of important stuff before calling the
// consumer's ScreenSaverProc.  This helps to prevent us from getting hosed
// by wacky consumer code.
LRESULT WINAPI
RealScreenSaverProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch( uMsg )
    {
    case WM_CREATE:
        // screen saver does not need the IME
        if ((hInstImm = GetModuleHandle(szImmDLL)) &&
            (ImmFnc = (IMMASSOCPROC)GetProcAddress(hInstImm,szImmFnc)))
             hPrevImc = ImmFnc(hWnd, (HIMC)0);

        // establish the mouse position
        GetCursorPos( &ptMouse );

        if( !fChildPreview )
            SetCursor( NULL );
        
        break;

    case WM_DESTROY:
        // screen saver does not need the IME
        if( hInstImm && ImmFnc && hPrevImc )
            ImmFnc(hWnd, hPrevImc);

        PostQuitMessage( 0 );
        break;

    case WM_SETTEXT:
        // don't let some fool change our title
        // we need to be able to use FindWindow() to find running instances
        // of full-screen windows screen savers
        // NOTE: USER slams our title in during WM_NCCREATE by calling the
        // defproc for WM_SETTEXT directly, so the initial title will get
        // there.  If this ever changes, we can simply set a bypass flag
        // during WM_NCCREATE processing.
        return FALSE;

    case WM_SYSCOMMAND:
        if (!fChildPreview)
        {
            switch (wParam)
            {
            case SC_NEXTWINDOW:       // no Alt-tabs
            case SC_PREVWINDOW:       // no shift-alt-tabs
            case SC_SCREENSAVE:       // no more screensavers
                return FALSE;
            }
        }
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:
        if( fChildPreview )
        {
            // if we're in preview mode, pump the help stuff to our owner
            HWND hParent = GetParent( hWnd );

            if( hParent && IsWindow( hParent ) )
                PostMessage( hParent, uMsg, (WPARAM)hParent, lParam );
            return TRUE;
        }
        break;

    case WM_TIMER:
        if( fClosing )
            return FALSE;
        Sleep( 0 );
        break;
       
    case WM_IME_NOTIFY:
        // Eat IMN_OPENSTATUSWINDOW so that the status window
        // isn't displayed.
        if (wParam == IMN_OPENSTATUSWINDOW)
        {
            return 0;
        }
        break;
       
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if( fClosing )
            return DefWindowProc( hWnd, uMsg, wParam, lParam );
        break;

    case WM_PAINT:
        if( fClosing )
            return DefWindowProc( hWnd, uMsg, wParam, lParam );
        if( !fChildPreview )
            SetCursor( NULL );
        break;
    }

    return ScreenSaverProc( hWnd, uMsg, wParam, lParam );
}

#ifdef GL_SCRNSAVE
void
#else
static void
#endif
InitRealScreenSave()
{
#if 0
   HKEY hkey;

   if (RegOpenKey(HKEY_CURRENT_USER, szCoolSaverHacks, &hkey) ==
      ERROR_SUCCESS)
   {
      DWORD data, len, type;

      len = sizeof(data);
      if ((RegQueryValueEx(hkey, szMouseThreshold, NULL, &type,
         (LPBYTE)&data, &len) == ERROR_SUCCESS) && (type == REG_DWORD))
      {
         dwWakeThreshold = max(dwWakeThreshold, data);
      }

      len = sizeof(data);
      if ((RegQueryValueEx(hkey, szPasswordDelay, NULL, &type,
         (LPBYTE)&data, &len) == ERROR_SUCCESS) && (type == REG_DWORD) && data)
      {
         data = min(MAX_PASSWORD_DELAY_IN_SECONDS, data);
         dwPasswordDelay = data * 1000;
         dwBlankTime = GetTickCount();
      }
   }
#endif

   LoadPwdDLL();
}

//----------------------------------------------------------------------------

#ifndef GL_SCRNSAVE

static INT_PTR
DoScreenSave( HWND hParent )
{
   LPCTSTR pszWindowClass = TEXT("WindowsScreenSaverClass");
   LPCTSTR pszWindowTitle;

   WNDCLASS cls;
   MSG      msg;
   UINT     uStyle;
   UINT     uExStyle;
   int      ncx, ncy;
   int      nx, ny;

   cls.hCursor        = NULL;
   cls.hIcon          = LoadIcon( hMainInstance, MAKEINTATOM( ID_APP ) );
   cls.lpszMenuName   = NULL;
   cls.lpszClassName  = pszWindowClass;
   cls.hbrBackground  = GetStockObject( BLACK_BRUSH );
   cls.hInstance      = hMainInstance;
   cls.style          = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS | CS_OWNDC;
   cls.lpfnWndProc    = RealScreenSaverProc;
   cls.cbWndExtra     = 0;
   cls.cbClsExtra     = 0;

   if( hParent )
   {
      RECT rcParent;
      GetClientRect( hParent, &rcParent );
      ncx = rcParent.right;
      ncy = rcParent.bottom;
      nx  = 0;
      ny  = 0;
      uStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN;
      uExStyle = 0;

      fChildPreview = TRUE;
      pszWindowTitle = TEXT("Preview");      // MUST differ from full screen
   }
   else
   {
      HWND hOther;

#ifdef SM_CXVIRTUALSCREEN
      nx  = GetSystemMetrics( SM_XVIRTUALSCREEN );
      ny  = GetSystemMetrics( SM_YVIRTUALSCREEN );
      ncx = GetSystemMetrics( SM_CXVIRTUALSCREEN );
      ncy = GetSystemMetrics( SM_CYVIRTUALSCREEN );

      if (ncx == 0 || ncy == 0)
#endif
      {
        RECT rc;
        HDC hdc = GetDC(NULL);
        GetClipBox(hdc, &rc);
        ReleaseDC(NULL, hdc);
        nx = rc.left;
        ny = rc.top;
        ncx = rc.right  - rc.left;
        ncy = rc.bottom - rc.top;
      }

      uStyle = WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
      uExStyle = WS_EX_TOPMOST;

      pszWindowTitle = TEXT("Screen Saver"); // MUST differ from preview

      // if there is another NORMAL screen save instance, switch to it
      hOther = FindWindow( pszWindowClass, pszWindowTitle );

      if( hOther && IsWindow( hOther ) )
      {
         SetForegroundWindow( hOther );
         return 0;
      }

      InitRealScreenSave();
   }

   //
   // the shell sends this message to the foreground window before running an
   // AutoPlay app. we return 1 to cancel autoplay if we are password protected
   //
   if (fOnWin95) {
        uShellAutoPlayQueryMessage = RegisterWindowMessage(TEXT("QueryCancelAutoPlay"));
   } else {
        uShellAutoPlayQueryMessage = 0;
   }

   if( RegisterClass( &cls ) )
   {
      hMainWindow = CreateWindowEx( uExStyle, pszWindowClass, pszWindowTitle,
                        uStyle, nx, ny, ncx, ncy, hParent, (HMENU)NULL,
                        hMainInstance, (LPVOID)NULL );
   }

   if( hMainWindow )
   {
      if( !fChildPreview )
         SetForegroundWindow( hMainWindow );

      while( GetMessage( &msg, NULL, 0, 0 ) )
      {
         TranslateMessage( &msg );
         DispatchMessage( &msg );
      }
   }

   // free password-handling DLL if loaded
   UnloadPwdDLL();

   return msg.wParam;
}
#endif


//----------------------------------------------------------------------------

static INT_PTR
DoSaverPreview( LPCTSTR szUINTHandle )
{
   // get parent handle from string
   HWND hParent = (HWND)atoui( szUINTHandle );

   // only preview on a valid parent window (NOT full screen)
   return ( (hParent && IsWindow( hParent ))? DoScreenSave( hParent ) : -1 );
}


//----------------------------------------------------------------------------

#ifndef GL_SCRNSAVE

static INT_PTR
DoConfigBox( HWND hParent )
{
   // let the consumer register any special controls for the dialog
   if( !RegisterDialogClasses( hMainInstance ) )
      return FALSE;

   return DialogBox( hMainInstance, MAKEINTRESOURCE( DLG_SCRNSAVECONFIGURE ),
                     hParent, (DLGPROC)ScreenSaverConfigureDialog );
}
#endif


//----------------------------------------------------------------------------

static INT_PTR
DoChangePw( LPCTSTR szUINTHandle )
{
   // get parent handle from string
   HWND hParent = (HWND)atoui( szUINTHandle );

   if( !hParent || !IsWindow( hParent ) )
      hParent = GetForegroundWindow();

   // allow the library to be hooked
   ScreenSaverChangePassword( hParent );
   return 0;
}

static const TCHAR szMprDll[] = TEXT("MPR.DLL");       // not to be localized
static const TCHAR szProviderName[] = TEXT("SCRSAVE"); // not to be localized

#ifdef UNICODE
static const CHAR szPwdChangePW[] = "PwdChangePasswordW"; // not to be localized
#else
static const CHAR szPwdChangePW[] = "PwdChangePasswordA"; // not to be localized
#endif

// bogus prototype
typedef DWORD (FAR PASCAL *PWCHGPROC)( LPCTSTR, HWND, DWORD, LPVOID );

void WINAPI
ScreenSaverChangePassword( HWND hParent )
{
   HINSTANCE mpr = LoadLibrary( szMprDll );

   if( mpr )
   {
      // netland hasn't cracked MNRENTRY yet
      PWCHGPROC pwd = (PWCHGPROC)GetProcAddress( mpr, szPwdChangePW );

      if( pwd )
         pwd( szProviderName, hParent, 0, NULL );

      FreeLibrary( mpr );
   }
}


//----------------------------------------------------------------------------

static BOOL
DoPasswordCheck( HWND hParent )
{
   // don't reenter and don't check when we've already decided
   if( fCheckingPassword || fClosing )
      return FALSE;

   if( VerifyPassword )
   {
      static DWORD lastcheck = (DWORD)-1;
      DWORD curtime = GetTickCount();
      MSG msg;

      if (dwPasswordDelay &&
         (GetElapsedTime(dwBlankTime, curtime) < dwPasswordDelay))
      {
         fClosing = TRUE;
         goto _didcheck;
      }

      // no rapid checking...
      if ((lastcheck != (DWORD)-1) &&
         (GetElapsedTime(lastcheck, curtime) < 200))
      {
         goto _didcheck;
      }

      // do the check
      fCheckingPassword = TRUE;

#ifdef GL_SCRNSAVE
      // Put ss in idle mode during password dialog processing
      SendMessage( hParent, SS_WM_IDLE, SS_IDLE_ON, 0L );
#endif

      // flush WM_TIMER messages before putting up the dialog
      PeekMessage( &msg, hParent, WM_TIMER, WM_TIMER, PM_REMOVE | PM_NOYIELD );
      PeekMessage( &msg, hParent, WM_TIMER, WM_TIMER, PM_REMOVE | PM_NOYIELD );

      // call the password verify proc
      fClosing = (BOOL)SendMessage( hParent, SCRM_VERIFYPW, 0, 0L );

      fCheckingPassword = FALSE;

#ifdef GL_SCRNSAVE
      // Restore normal display mode
      SendMessage( hParent, SS_WM_IDLE, SS_IDLE_OFF, 0L );
#endif

      if (!fClosing)
         SetCursor(NULL);

      // curtime may be outdated by now
      lastcheck = GetTickCount();
   }
   else
   {
      // passwords disabled or unable to load handler DLL, always allow exit
      fClosing = TRUE;
   }

_didcheck:
   return fClosing;
}

//----------------------------------------------------------------------------
// stolen from the CRT, used to shirink our code

int _stdcall
DummyEntry( void )
{
    int i;
    STARTUPINFO si;
    LPTSTR pszCmdLine = GetCommandLine();

    if ( *pszCmdLine == TEXT('\"')) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while (*(pszCmdLine = CharNext(pszCmdLine)) &&
              (*pszCmdLine != TEXT('\"')) );
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == TEXT('\"') )
            pszCmdLine++;
    }
    else {
        while ((UINT)*pszCmdLine > (UINT)TEXT(' '))
            pszCmdLine = CharNext(pszCmdLine);
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && ((UINT)*pszCmdLine <= (UINT)TEXT(' '))) {
        pszCmdLine = CharNext(pszCmdLine);
    }

    si.dwFlags = 0;
    GetStartupInfo(&si);

    i = (int)WinMainN(GetModuleHandle(NULL), NULL, pszCmdLine,
        si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);

    ExitProcess(i);
    return i;   // We never comes here.
}

//----------------------------------------------------------------------------
// main() entry point to satisfy old NT screen savers
void _cdecl main( int argc, char *argv[] ) {
    DummyEntry();
}

//----------------------------------------------------------------------------
// WinMain() entry point to satisfy old NT screen savers
int PASCAL WinMain( HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int nCmdShow ) {
    DummyEntry();
    return 0;

    // reference unreferenced parameters
    (void)hInst;
    (void)hPrev;
    (void)szCmdLine;
    (void)nCmdShow;
}


VOID LoadPwdDLL(VOID)
{
    HKEY hKey;

    if (!fOnWin95)
        return;

    if (hInstPwdDLL)
        UnloadPwdDLL();

    // look in registry to see if password turned on, otherwise don't
    // bother to load password handler DLL
    if (RegOpenKey(HKEY_CURRENT_USER,szScreenSaverKey,&hKey) ==
        ERROR_SUCCESS)
    {
        DWORD dwVal,dwSize=sizeof(dwVal);

        if ((RegQueryValueEx(hKey,szPasswordActiveValue,
            NULL,NULL,(BYTE *) &dwVal,&dwSize) == ERROR_SUCCESS)
            && dwVal)
        {

            // try to load the DLL that contains password proc.
            hInstPwdDLL = LoadLibrary(szPwdDLL);
            if (hInstPwdDLL)
            {
                VerifyPassword = (VERIFYPWDPROC) GetProcAddress(hInstPwdDLL,
                    szFnName);

                if( VerifyPassword )
                    HogMachine( TRUE );
                else
                    UnloadPwdDLL();
            }
        }

        RegCloseKey(hKey);
    }

}

VOID UnloadPwdDLL(VOID)
{
    if (!fOnWin95)
        return;

    if (hInstPwdDLL)
    {
        FreeLibrary(hInstPwdDLL);
        hInstPwdDLL = NULL;

        if( VerifyPassword )
        {
            VerifyPassword = NULL;
            HogMachine( FALSE );
        }
    }
}

//----------------------------------------------------------------------------
// compatbility stuff  (to make porting easier)
TCHAR szAppName[ APPNAMEBUFFERLEN ];
TCHAR szName[ TITLEBARNAMELEN ];
TCHAR szIniFile[ MAXFILELEN ];
TCHAR szScreenSaver[ 22 ];
TCHAR szHelpFile[ MAXFILELEN ];
TCHAR szNoHelpMemory[ BUFFLEN ];

// Quick fix for old screen savers that don't know about context
// sensitive help
UINT  MyHelpMessage = WM_HELP;

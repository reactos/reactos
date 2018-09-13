/*
 *  SCRNSAVE.C - default screen saver.
 *
 *  this app makes a IdleWild screen saver compatible with the windows 3.1
 *  screen saver interface.
 *
 *  Usage:      SCRNSAVE.EXE saver.iw [/s] [/c]
 *
 *      the IdleWild screen saver 'saver.iw' will be loaded and told to
 *      screen save.  if '/c' is specifed the savers configure dialog will
 *      be shown.
 *
 *      when the screen saver terminates SCRNSAVE.EXE will terminate too.
 *
 *      if the saver.iw is not specifed or refuses to load then a
 *      builtin 'blackness' screen saver will be used.
 *
 *  Restrictions:
 *
 *      because only one screen saver is loaded, (not all the screen savers
 *      like IdleWild.exe does) the random screen saver will not work correctly
 *
 *  History:
 *      10/15/90        ToddLa      stolen from SOS.C by BradCh
 *       6/17/91        stevecat    ported to NT Windows
 *
 */
#include <string.h>

#define WIN31 /* For topmost windows */
#include <windows.h>
#include "strings.h"

#define BUFFER_LEN  255

CHAR szAppName[BUFFER_LEN];
CHAR szNoConfigure[BUFFER_LEN];

#define     THRESHOLD   3

#define abs(x)      ( (x)<0 ? -(x) : (x) )

//
// private stuff in IWLIB.DLL
//
HANDLE  hIdleWildDll;
CHAR    szIdleWildDll[] = "IWLIB.DLL";

SHORT (*FInitScrSave) (HANDLE, HWND);
VOID  (*TermScrSave) (VOID);
VOID  (*ScrBlank) (SHORT);
VOID  (*ScrSetIgnore) (SHORT);
SHORT (*ScrLoadServer) (CHAR *);
SHORT (*ScrSetServer) (CHAR *);
VOID  (*ScrInvokeDlg) (HANDLE, HWND);

HANDLE  hMainInstance   = NULL;
HWND    hwndApp         = NULL;
HWND    hwndActive      = NULL;
BOOL    fBlankNow       = FALSE;
BOOL    fIdleWild       = FALSE;
//SHORT   wmScrSave       = -1;
// changed to what I believe it should be
UINT wmScrSave  = 0xffffffff;

typedef LONG (*LPWNDPROC)(); // pointer to a window procedure

BOOL FInitIdleWild (LPSTR szCmdLine);
BOOL FTermIdleWild (VOID);
BOOL FInitApp      (HANDLE hInstance, LPSTR szCmdLine, WORD sw);
BOOL FTermApp      (VOID);
BOOL FInitDefault  (VOID);
LRESULT DefaultProc   (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int __cdecl main (USHORT argc, CHAR **argv)
{
        HANDLE   hInstance;
        HANDLE   hPrev     = NULL;
        LPSTR    szCmdLine = GetCommandLine();
        WORD     sw        = SW_SHOWNORMAL;
        MSG      msg;

    hInstance = GetModuleHandle (NULL);

        hMainInstance = hInstance;

        // If we're already running another instance, get out
        if (hPrev != NULL)
                return FALSE;

        if (!FInitApp (hInstance, szCmdLine, sw))
        {
                //MessageBox (NULL, "Cannot initialize!", szAppName, MB_OK);
                return FALSE;
        }

        while (GetMessage (&msg, NULL, 0, 0))
        {
                //
        // IWLIB.DLL will brodcast a message when the screen saving
                // is done.
        //
                if (msg.message == wmScrSave && msg.wParam == FALSE)
                        break;

                TranslateMessage (&msg);
                DispatchMessage (&msg);
        }

        return FTermApp ();
}


BOOL FTermApp (VOID)
{
        ////FTermDefault ();

        FTermIdleWild ();
        return TRUE;
}


BOOL FInitApp (HANDLE hInstance, LPSTR szCmdLine, WORD sw)
{
        LPSTR lpch, lpT;

    LoadString(hMainInstance, idsAppName, szAppName, BUFFER_LEN);
    LoadString(hMainInstance, idsNoConfigure, szNoConfigure, BUFFER_LEN);

//=================================================================

    // on NT, szCmdLine's first string includes its own name, remove this
    // to make it exactly like the windows command line.

    if (*szCmdLine)
        {
        lpT = strchr(szCmdLine, ' ');   // skip self name
        if (lpT)
                {
            szCmdLine = lpT;
            while (*szCmdLine == ' ')
                szCmdLine++;            // skip spaces to end or first cmd
        }
                else
                {
            szCmdLine += strlen(szCmdLine);   // point to NULL
        }
    }
//=====================================================================
        //
    //  parse command line looking for switches
        //

        for (lpch = szCmdLine; *lpch != '\0'; lpch += 1)
        {
                if (*lpch == '/' || *lpch == '-')
                {
                        if (lpch[1] == 's' || lpch[1] == 'S')
                                fBlankNow = TRUE;
                        if (lpch[1] == 'c' || lpch[1] == 'C')
                                hwndActive = GetActiveWindow ();
                        lpch[0] = ' ';
                        lpch[1] = ' ';
                }
        }

        //
    //  try to load the IdleWild screen saver, if none specifed or
        //  we are unable to load it then use the default one.
    //
        if (FInitIdleWild (szCmdLine))
        {
                if (fBlankNow)
                {
                        ScrSetIgnore (1);
                        ScrBlank (TRUE);
                }
                else
                {
                        ScrInvokeDlg (hMainInstance, hwndActive);
                        PostQuitMessage (0);
                }
        }
        else if (!fBlankNow || !FInitDefault ())
        {
                MessageBox (hwndActive, szNoConfigure, szAppName, MB_OK | MB_ICONEXCLAMATION);
                PostQuitMessage (0);
        }

        return TRUE;
}


//
//  run-time link to IWLIB.DLL
//
BOOL FInitIdleWild (LPSTR szCmdLine)
{
        OFSTRUCT of;

        while (*szCmdLine == ' ')
                szCmdLine++;

        if (*szCmdLine == 0)
                return FALSE;

        if (-1 == OpenFile(szIdleWildDll, &of, OF_EXIST | OF_SHARE_DENY_NONE) ||
            -1 == OpenFile(szCmdLine, &of, OF_EXIST | OF_SHARE_DENY_NONE))
                return FALSE;

        if ((hIdleWildDll = LoadLibrary (szIdleWildDll)) == NULL)
                return FALSE;

        FInitScrSave  = (SHORT (*) (HANDLE, HWND))GetProcAddress (hIdleWildDll, "FInitScrSave" );
        TermScrSave   = (VOID (*) (VOID))         GetProcAddress (hIdleWildDll, "TermScrSave"  );
        ScrBlank      = (VOID (*) (SHORT))        GetProcAddress (hIdleWildDll, "ScrBlank"     );
        ScrSetIgnore  = (VOID (*) (SHORT))        GetProcAddress (hIdleWildDll, "ScrSetIgnore" );
        ScrLoadServer = (SHORT (*) (CHAR *))      GetProcAddress (hIdleWildDll, "ScrLoadServer");
        ScrSetServer  = (SHORT (*) (CHAR *))      GetProcAddress (hIdleWildDll, "ScrSetServer" );
        ScrInvokeDlg  = (VOID (*) (HANDLE, HWND)) GetProcAddress (hIdleWildDll, "ScrInvokeDlg" );

        //
    // must be a invalid dll?
        //
    if (!FInitScrSave || !TermScrSave)
        {
                FreeLibrary (hIdleWildDll);
                return FALSE;
        }

        //
    // init iwlib.dll
        //
    if (!FInitScrSave (hMainInstance, NULL))     // NULL hwnd???
        {
                FreeLibrary (hIdleWildDll);
                return FALSE;
        }

        //
    //  load the screen saver on the command line.
        //  if the load fails, abort
    //
        if (!ScrLoadServer (szCmdLine))
        {
                TermScrSave ();
                FreeLibrary (hIdleWildDll);
                return FALSE;
        }

        wmScrSave = RegisterWindowMessage ("SCRSAVE"); // REVIEW: for win 3.1

        fIdleWild = TRUE;

        return TRUE;
}


BOOL FTermIdleWild (VOID)
{
        if (fIdleWild)
        {
                TermScrSave ();
                FreeLibrary (hIdleWildDll);
        }
        return TRUE;
}


//
//  init the default screen saver
//
BOOL FInitDefault (VOID)
{
        WNDCLASS    cls;
        HWND        hwnd;
        HDC         hdc;
        RECT rc;
        OSVERSIONINFO osvi;
        BOOL bWin2000 =  FALSE;

        cls.style           = 0;
        cls.lpfnWndProc     = (WNDPROC) DefaultProc;
        cls.cbClsExtra      = 0;
        cls.cbWndExtra      = 0;
        cls.hInstance       = hMainInstance;
        cls.hIcon           = NULL;
        cls.hCursor         = NULL;
        cls.hbrBackground   = GetStockObject (BLACK_BRUSH);
        cls.lpszMenuName    = NULL;
        cls.lpszClassName   = szAppName;

        if (!RegisterClass (&cls))
                return FALSE;

        //
        // Make sure we use the entire virtual desktop size for multiple
        // displays
        //

        hdc = GetDC(NULL);
        GetClipBox(hdc, &rc);
        ReleaseDC(NULL, hdc);

        // On Win2000 Terminal Services we must detect the case where a remotte session
        // is on the disconnected desktop, because in this case GetClipBox() returns
        // an empty rect.

        if (IsRectEmpty(&rc)) {
            osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
            if (GetVersionEx (&osvi)){
                if ((osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) && (osvi.dwMajorVersion >= 5)) {
                    bWin2000 = TRUE;
                }
            }
            if (bWin2000 && GetSystemMetrics(SM_REMOTESESSION)) {
                rc.left = 0;
                rc.top = 0;
                rc.right = GetSystemMetrics(SM_CXVIRTUALSCREEN);
                rc.bottom = GetSystemMetrics(SM_CYVIRTUALSCREEN);

            }

        }


        hwnd = CreateWindowEx (WS_EX_TOPMOST, szAppName, szAppName,
                                                        WS_POPUP | WS_VISIBLE,
                                                        rc.left,
                                                        rc.top,
                                                        rc.right  - rc.left,
                                                        rc.bottom - rc.top,
                                                        NULL, NULL,
                                                        hMainInstance, NULL);

        return hwnd != NULL;
}


LRESULT DefaultProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
        static POINT  ptLast;
        POINT  ptMouse;

        switch (msg)
        {
        case WM_CREATE:
                GetCursorPos (&ptLast);
                break;

        case WM_DESTROY:
                PostQuitMessage (0);
                break;

        case WM_ACTIVATE:
        case WM_ACTIVATEAPP:
                if (wParam)
                        break;
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_KEYDOWN:
        case WM_CHAR:
                PostMessage (hwnd, WM_CLOSE, 0, 0L);
                break;

        case WM_MOUSEMOVE:
                GetCursorPos (&ptMouse);
                if (abs (ptMouse.x - ptLast.x) + abs (ptMouse.y - ptLast.y) > THRESHOLD)
                        PostMessage (hwnd, WM_CLOSE, 0, 0L);
                break;

        case WM_SETCURSOR:
                SetCursor (NULL);
                return 0L;
        }

        return DefWindowProc (hwnd, msg, wParam, lParam);
}

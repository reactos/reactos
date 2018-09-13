/*--------------------------------------------------------------------------*\
|   qmci.c - Quick MDI App                                                  |
|                                                                           |
|   Usage:                                                                  |
|	To make a quick MDI windows application, modify this source	    |
|                                                                           |
|   History:                                                                |
|	12/15/87 toddla     Created					    |
|                                                                           |
\*-------------------------------------------------------------------------*/

#include <windows.h>
#include <commdlg.h>
#include "movplay.h"

#include "mciwnd.h"
#include "preview.h"    //!!! in mciwnd.h???

typedef LONG (FAR PASCAL *LPWNDPROC)(); // pointer to a window procedure

/*-------------------------------------------------------------------------*\
|                                                                          |
|   g l o b a l   v a r i a b l e s                                        |
|                                                                          |
\*------------------------------------------------------------------------*/

// We have our own copy of the MCIWND.LIB so we better make our own class
// names or we'll conflict and blow up!
extern char	aszMCIWndClassName[];
extern char	aszToolbarClassName[];
extern char	aszTrackbarClassName[];

char    szAppName[]  = "MovPlay";   /* change this to your app's name */

char    szOpenFilter[] = "Video Files\0*.avi\0"
                         "Wave Files\0*.wav\0"
                         "Midi Files\0*.mid; *.rmi\0"
                         "All Files\0*.*\0";

HANDLE  hInstApp;                   /* Instance handle */
HACCEL  hAccelApp;
HWND    hwndApp;                    /* Handle to parent window */
HWND    hwndMdi;                    /* Handle to MCI client window */

OFSTRUCT     of;
OPENFILENAME ofn;
char         achFileName[128];

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

long FAR PASCAL _export AppWndProc(HWND, UINT, WPARAM, LPARAM);
long FAR PASCAL _export mdiDocWndProc(HWND, unsigned, WORD, LONG);
int ErrMsg (LPSTR sz,...);

HWND mdiCreateDoc(LPSTR szClass, LPSTR szTitle, LPARAM l);

/*----------------------------------------------------------------------------*\
|   AppAbout( hDlg, msg, wParam, lParam )                                      |
|                                                                              |
|   Description:                                                               |
|       This function handles messages belonging to the "About" dialog box.    |
|       The only message that it looks for is WM_COMMAND, indicating the use   |
|       has pressed the "OK" button.  When this happens, it takes down         |
|       the dialog box.                                                        |
|                                                                              |
|   Arguments:                                                                 |
|       hDlg            window handle of about dialog window                   |
|       msg             message number                                         |
|       wParam          message-dependent                                      |
|       lParam          message-dependent                                      |
|                                                                              |
|   Returns:                                                                   |
|       TRUE if message has been processed, else FALSE                         |
|                                                                              |
\*----------------------------------------------------------------------------*/
BOOL FAR PASCAL _export AppAbout(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_COMMAND:
            EndDialog(hwnd,TRUE);
            return TRUE;

        case WM_INITDIALOG:
	    return TRUE;
    }
    return FALSE;
}

/*----------------------------------------------------------------------------*\
|   AppInit ( hInstance, hPrevInstance )				       |
|                                                                              |
|   Description:                                                               |
|       This is called when the application is first loaded into               |
|       memory.  It performs all initialization that doesn't need to be done   |
|       once per instance.                                                     |
|                                                                              |
|   Arguments:                                                                 |
|	hPrevInstance	instance handle of previous instance		       |
|       hInstance       instance handle of current instance                    |
|                                                                              |
|   Returns:                                                                   |
|       TRUE if successful, FALSE if not                                       |
|                                                                              |
\*----------------------------------------------------------------------------*/
BOOL AppInit(HANDLE hInst, HANDLE hPrev, LPSTR szCmd, int sw)
{
    WNDCLASS    cls;

    /* Save instance handle for DialogBox */
    hInstApp = hInst;

    hAccelApp = LoadAccelerators(hInstApp, "AppAccel");

    if (!hPrev) {
        cls.hCursor        = LoadCursor(NULL,IDC_ARROW);
        cls.hIcon          = LoadIcon(hInst,"AppIcon");
        cls.lpszMenuName   = "AppMenu";
        cls.lpszClassName  = szAppName;
        cls.hbrBackground  = (HBRUSH)COLOR_APPWORKSPACE+1;
        cls.hInstance      = hInst;
        cls.style          = 0;
        cls.lpfnWndProc    = (WNDPROC)AppWndProc;
        cls.cbClsExtra     = 0;
	cls.cbWndExtra	   = 0;

        if (!RegisterClass(&cls))
            return FALSE;
    }

    // This app has its own copy of the MCIWnd stuff, and doesn't use
    // the copy found in MSVIDEO.DLL  We better also have different
    // class names or else we'll conflict and blow up.
    // !!! Warning - The variable is not too long!
    lstrcpy(aszMCIWndClassName, "MCIWndMov");
    lstrcpy(aszTrackbarClassName, "TrackMov");
    lstrcpy(aszToolbarClassName, "ToolMov");

    MCIWndRegisterClass();

    hwndApp =
#ifdef BIDI
	CreateWindowEx(WS_EX_BIDI_SCROLL |  WS_EX_BIDI_MENU |WS_EX_BIDI_NOICON,
#else
	CreateWindow (
#endif
	       szAppName,szAppName,
	       WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
	       CW_USEDEFAULT,0,
	       CW_USEDEFAULT,0,
	       (HWND)NULL,	  /* no parent */
	       (HMENU)NULL,	  /* use class menu */
               (HANDLE)hInst,     /* handle to window instance */
	       (LPSTR)NULL	  /* no params to pass on */
	     );

    /* Make window visible according to the way the app is activated */
    ShowWindow(hwndApp,sw);

    if (szCmd && szCmd[0])
        mdiCreateDoc(aszMCIWndClassName, 0, (LPARAM)(LPSTR)szCmd);

    return TRUE;
}

/*----------------------------------------------------------------------------*\
|   WinMain( hInstance, hPrevInstance, lpszCmdLine, cmdShow )                  |
|                                                                              |
|   Description:                                                               |
|       The main procedure for the App.  After initializing, it just goes      |
|       into a message-processing loop until it gets a WM_QUIT message         |
|       (meaning the app was closed).                                          |
|                                                                              |
|   Arguments:                                                                 |
|       hInstance       instance handle of this instance of the app            |
|       hPrevInstance   instance handle of previous instance, NULL if first    |
|       lpszCmdLine     ->null-terminated command line                         |
|       cmdShow         specifies how the window is initially displayed        |
|                                                                              |
|   Returns:                                                                   |
|       The exit code as specified in the WM_QUIT message.                     |
|                                                                              |
\*----------------------------------------------------------------------------*/
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int sw)
{
    MSG     msg;

    if (!AppInit(hInstance,hPrevInstance,szCmdLine,sw))
       return FALSE;

    /*
     * Polling messages from event queue
     */
    for (;;)
    {
        if (PeekMessage(&msg, NULL, 0, 0,PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                break;

            if (hAccelApp && hwndApp &&
			TranslateAccelerator(hwndApp, hAccelApp, &msg))
                continue;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // idle time here, DONT BE A PIG!
            WaitMessage();
        }
    }

    return msg.wParam;
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

BOOL fDialog(HWND hwnd,int id,FARPROC fpfn)
{
    BOOL	f;
    HANDLE	hInst;

    hInst = (HINSTANCE)GetWindowWord(hwnd,GWW_HINSTANCE);
    fpfn  = MakeProcInstance(fpfn,hInst);
    f = DialogBox(hInst,MAKEINTRESOURCE(id),hwnd,(DLGPROC)fpfn);
    FreeProcInstance (fpfn);
    return f;
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

#define mdiGetCreateParam(lParam) \
    (((LPMDICREATESTRUCT)(((LPCREATESTRUCT)lParam)->lpCreateParams))->lParam)

/*----------------------------------------------------------------------------*\
|   mdiCreateChild()							       |
|									       |
|   Description:                                                               |
|                                                                              |
|   Arguments:                                                                 |
|                                                                              |
|   Returns:                                                                   |
|	HWND if successful, NULL otherwise				       |
|									       |
\*----------------------------------------------------------------------------*/

HWND mdiCreateChild(
    HWND  hwndMdi,
    LPSTR szClass,
    LPSTR szTitle,
    DWORD dwStyle,
    int   x,
    int   y,
    int   dx,
    int   dy,
    WORD  sw,
    HMENU hmenu,
    LPARAM l)
{
    MDICREATESTRUCT mdics;

    mdics.szClass   = szClass;
    mdics.szTitle   = szTitle;
    mdics.hOwner    = (HINSTANCE)GetWindowWord(hwndMdi, GWW_HINSTANCE);
    mdics.x         = x;
    mdics.y         = y;
    mdics.cx        = dx;
    mdics.cy        = dy;
    mdics.style     = dwStyle;
    mdics.lParam    = l;

    return (HWND)SendMessage(hwndMdi,WM_MDICREATE,0,(LONG)(LPVOID)&mdics);
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

HWND mdiCreateDoc(LPSTR szClass, LPSTR szTitle, LPARAM l)
{
    return mdiCreateChild(hwndMdi,szClass,szTitle,
	WS_CLIPCHILDREN | WS_CLIPSIBLINGS | MCIWNDF_SHOWALL,
        CW_USEDEFAULT,0,CW_USEDEFAULT,0,SW_NORMAL,NULL,l);
}

/*----------------------------------------------------------------------------*\
|   mdiCreateClient()                                                           |
|									       |
|   Description:                                                               |
|                                                                              |
|   Arguments:                                                                 |
|                                                                              |
|   Returns:                                                                   |
|	HWND if successful, NULL otherwise				       |
|									       |
\*----------------------------------------------------------------------------*/
HWND FAR PASCAL mdiCreateClient(HWND hwndP, HMENU hmenuWindow)
{
    CLIENTCREATESTRUCT ccs;

    ccs.hWindowMenu = hmenuWindow;
    ccs.idFirstChild = 100;

    return
#ifdef BIDI
	CreateWindowEx(WS_EX_BIDI_SCROLL |  WS_EX_BIDI_MENU |WS_EX_BIDI_NOICON,
#else
	CreateWindow (
#endif
		"MDICLIENT",NULL,
                WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE,
                0,0,0,0,
                hwndP, 0, (HINSTANCE)GetWindowWord(hwndP,GWW_HINSTANCE),
                (LPVOID)&ccs);
}

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

#define mdiActiveDoc() \
    (HWND)SendMessage(hwndMdi,WM_MDIGETACTIVE,0,0L)

/*----------------------------------------------------------------------------*\
\*----------------------------------------------------------------------------*/

LONG NEAR PASCAL mdiSendMessage(HWND hwndMdi, HWND hwnd, unsigned msg, WORD wParam, LONG lParam)
{
    if (hwnd == (HWND)-1)
    {
        for (hwnd = GetWindow(hwndMdi, GW_CHILD); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT))
            SendMessage(hwnd, msg, wParam, lParam);

        return 0L;
    }
    else
    {
        if (hwnd == NULL)
            hwnd = (HWND)SendMessage(hwndMdi,WM_MDIGETACTIVE,0,0L);

        if (hwnd)
            return SendMessage(hwnd, msg, wParam, lParam);
    }
}

/*----------------------------------------------------------------------------*\
|   AppWndProc( hwnd, msg, wParam, lParam )                                    |
|                                                                              |
|   Description:                                                               |
|       The window proc for the app's main (tiled) window.  This processes all |
|       of the parent window's messages.                                       |
|									       |
|   Arguments:                                                                 |
|       hwnd            window handle for the parent window                    |
|       msg             message number                                         |
|       wParam          message-dependent                                      |
|       lParam          message-dependent                                      |
|                                                                              |
|   Returns:                                                                   |
|       0 if processed, nonzero if ignored                                     |
|                                                                              |
\*----------------------------------------------------------------------------*/
long FAR PASCAL _export AppWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    UINT            f;
    PAINTSTRUCT     ps;
    HDC             hdc;
    HMENU           hmenu;
    HWND            hwndMovie;

    switch (msg) {
        case WM_COMMAND:
            hwndMovie = mdiActiveDoc();

	    switch(wParam) {
		case MENU_ABOUT:
                    fDialog(hwnd,ABOUTBOX,(FARPROC)AppAbout);
		    break;

		case MENU_EXIT:
                    PostMessage(hwnd,WM_CLOSE,0,0L);
                    break;

                case MENU_CLOSE:
                    //PostMessage(hwndMdi, WM_MDIDESTROY, (WPARAM)hwndMovie, 0);
                    PostMessage(hwndMovie, WM_CLOSE, 0, 0L);
                    break;

                case MENU_CLOSEALL:
                    mdiSendMessage(hwndMdi,(HWND)-1,WM_CLOSE,0,0);
                    break;

                case MENU_NEW:
                    mdiCreateDoc(aszMCIWndClassName, "Untitled", 0);
                    break;

                case MENU_OPEN:
                    /* prompt user for file to open */
                    ofn.lStructSize = sizeof(OPENFILENAME);
                    ofn.hwndOwner = hwnd;
                    ofn.hInstance = NULL;
                    ofn.lpstrFilter = szOpenFilter;
                    ofn.lpstrCustomFilter = NULL;
                    ofn.nMaxCustFilter = 0;
                    ofn.nFilterIndex = 0;
                    ofn.lpstrFile = achFileName;
                    ofn.nMaxFile = sizeof(achFileName);
                    ofn.lpstrFileTitle = NULL;
                    ofn.nMaxFileTitle = 0;
                    ofn.lpstrInitialDir = NULL;
                    ofn.lpstrTitle = "Open";
		    ofn.Flags =
#ifdef BIDI
		OFN_BIDIDIALOG |
#endif
		    OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
                    ofn.nFileOffset = 0;
                    ofn.nFileExtension = 0;
                    ofn.lpstrDefExt = NULL;
                    ofn.lCustData = 0;
                    ofn.lpfnHook = NULL;
                    ofn.lpTemplateName = NULL;

                    if (GetOpenFileNamePreview(&ofn))
                    {
                        mdiCreateDoc(aszMCIWndClassName, 0,
				(LPARAM)(LPSTR)achFileName);
                    }
                    break;

                case WM_MDITILE:
                case WM_MDICASCADE:
                case WM_MDIICONARRANGE:
                    SendMessage(hwndMdi, (UINT)wParam, 0, 0);
                    break;

                /* Movie Menu */
                case IDM_PLAY:
                    MCIWndPlay(hwndMovie);
                    break;
                case IDM_RPLAY:
                    MCIWndPlayReverse(hwndMovie);
                    break;
                case IDM_STOP:
                    MCIWndStop(hwndMovie);
                    break;
                case IDM_HOME:
                    MCIWndHome(hwndMovie);
                    break;
                case IDM_END:
                    MCIWndEnd(hwndMovie);
                    break;
                case IDM_STEP:
                    MCIWndStep(hwndMovie, 1);
                    break;
                case IDM_RSTEP:
                    MCIWndStep(hwndMovie, -1);
                    break;

		/* Styles POPUP */

#define ISCHECKED() (BOOL)(GetMenuState(GetMenu(hwnd), wParam, 0) & MF_CHECKED)

		case IDM_SRepeat:
		    MCIWndSetRepeat(hwndMovie, !ISCHECKED());
		    break;

		case IDM_SAutosizeWindow:
		    MCIWndChangeStyles(hwndMovie, MCIWNDF_NOAUTOSIZEWINDOW,
			    ISCHECKED() ? MCIWNDF_NOAUTOSIZEWINDOW : 0);
		    break;

		case IDM_SAutosizeMovie:
		    MCIWndChangeStyles(hwndMovie, MCIWNDF_NOAUTOSIZEMOVIE,
			    ISCHECKED() ? MCIWNDF_NOAUTOSIZEMOVIE : 0);
		    break;

		case IDM_SPlaybar:
		    MCIWndChangeStyles(hwndMovie, MCIWNDF_NOPLAYBAR,
			    ISCHECKED() ? MCIWNDF_NOPLAYBAR : 0);
		    break;

		case IDM_SRecord:
		    MCIWndChangeStyles(hwndMovie, MCIWNDF_RECORD,
			    ISCHECKED() ? 0 : MCIWNDF_RECORD);
		    break;

		case IDM_SMenu:
		    MCIWndChangeStyles(hwndMovie, MCIWNDF_NOMENU,
			    ISCHECKED() ? MCIWNDF_NOMENU : 0);
		    break;

		case IDM_SErrorDlg:
		    MCIWndChangeStyles(hwndMovie, MCIWNDF_NOERRORDLG,
			    ISCHECKED() ? MCIWNDF_NOERRORDLG : 0);
		    break;

		case IDM_SShowName:
		    MCIWndChangeStyles(hwndMovie, MCIWNDF_SHOWNAME,
			    ISCHECKED() ? 0 : MCIWNDF_SHOWNAME);
		    break;

		case IDM_SShowMode:
		    MCIWndChangeStyles(hwndMovie, MCIWNDF_SHOWMODE,
			    ISCHECKED() ? 0 : MCIWNDF_SHOWMODE);
		    break;

		case IDM_SShowPos:
		    MCIWndChangeStyles(hwndMovie, MCIWNDF_SHOWPOS,
			    ISCHECKED() ? 0 : MCIWNDF_SHOWPOS);
		    break;

		case IDM_SNotifyMedia:
		    MCIWndChangeStyles(hwndMovie, MCIWNDF_NOTIFYMEDIA,
			    ISCHECKED() ? 0 : MCIWNDF_NOTIFYMEDIA);
		    break;

		case IDM_SNotifyMode:
		    MCIWndChangeStyles(hwndMovie, MCIWNDF_NOTIFYMODE,
			    ISCHECKED() ? 0 : MCIWNDF_NOTIFYMODE);
		    break;

		case IDM_SNotifyPos:
		    MCIWndChangeStyles(hwndMovie, MCIWNDF_NOTIFYPOS,
			    ISCHECKED() ? 0 : MCIWNDF_NOTIFYPOS);
		    break;

		case IDM_SNotifySize:
		    MCIWndChangeStyles(hwndMovie, MCIWNDF_NOTIFYSIZE,
			    ISCHECKED() ? 0 : MCIWNDF_NOTIFYSIZE);
		    break;

                default:
                    mdiSendMessage(hwndMdi,NULL,msg,wParam,lParam);
                    break;
	    }
            break;

        case WM_PALETTECHANGED:
            mdiSendMessage(hwndMdi, (HWND)-1, msg, wParam, lParam);
            break;

        case WM_QUERYNEWPALETTE:
            return mdiSendMessage(hwndMdi, NULL, msg, wParam, lParam);

        case WM_INITMENUPOPUP:
            hwndMovie = mdiActiveDoc();

	    //
	    // Check the styles properly when styles is chosen
	    // !!! Make sure position constants don't change!
	    //
  	    hmenu = GetSubMenu(GetSubMenu(GetMenu(hwnd), 1), 10);
	    if (((HMENU)wParam == hmenu) && hwndMovie) {
		WORD  wStyles = MCIWndGetStyles(hwndMovie);

		CheckMenuItem(hmenu, IDM_SRepeat,
		    MCIWndGetRepeat(hwndMovie) ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(hmenu, IDM_SAutosizeWindow,
		    (wStyles & MCIWNDF_NOAUTOSIZEWINDOW) ? MF_UNCHECKED :
			MF_CHECKED);
		CheckMenuItem(hmenu, IDM_SAutosizeMovie,
		    (wStyles & MCIWNDF_NOAUTOSIZEMOVIE) ? MF_UNCHECKED :
			MF_CHECKED);
		CheckMenuItem(hmenu, IDM_SPlaybar,
		    (wStyles & MCIWNDF_NOPLAYBAR) ? MF_UNCHECKED : MF_CHECKED);
		CheckMenuItem(hmenu, IDM_SRecord,
		    (wStyles & MCIWNDF_RECORD) ? MF_CHECKED :MF_UNCHECKED);
		CheckMenuItem(hmenu, IDM_SMenu,
		    (wStyles & MCIWNDF_NOMENU) ? MF_UNCHECKED :MF_CHECKED);
		CheckMenuItem(hmenu, IDM_SErrorDlg,
		    (wStyles & MCIWNDF_NOERRORDLG) ? MF_UNCHECKED :MF_CHECKED);
		CheckMenuItem(hmenu, IDM_SShowName,
		    (wStyles & MCIWNDF_SHOWNAME) ? MF_CHECKED :MF_UNCHECKED);
		CheckMenuItem(hmenu, IDM_SShowMode,
		    (wStyles & MCIWNDF_SHOWMODE) ? MF_CHECKED :MF_UNCHECKED);
		CheckMenuItem(hmenu, IDM_SShowPos,
		    (wStyles & MCIWNDF_SHOWPOS) ? MF_CHECKED :MF_UNCHECKED);
		CheckMenuItem(hmenu, IDM_SNotifyMedia,
		    (wStyles & MCIWNDF_NOTIFYMEDIA) ? MF_CHECKED :MF_UNCHECKED);
		CheckMenuItem(hmenu, IDM_SNotifyMode,
		    (wStyles & MCIWNDF_NOTIFYMODE) ? MF_CHECKED :MF_UNCHECKED);
		CheckMenuItem(hmenu, IDM_SNotifyPos,
		    (wStyles & MCIWNDF_NOTIFYPOS) ? MF_CHECKED :MF_UNCHECKED);
		CheckMenuItem(hmenu, IDM_SNotifySize,
		    (wStyles & MCIWNDF_NOTIFYSIZE) ? MF_CHECKED :MF_UNCHECKED);
	    }

	    //
	    // Enable/Disable the stuff under the MOVIE popup
	    // !!! Make sure position constants don't change!
	    //
	    if ((HMENU)wParam == GetSubMenu(GetMenu(hwnd), 1)) {

                EnableMenuItem((HMENU)wParam, 10,
		    MF_BYPOSITION | (hwndMovie ? MF_ENABLED : MF_GRAYED));
		
                if (!hwndMovie || MCIWndGetMode(hwndMovie, NULL, 0) ==
		    	    MCI_MODE_NOT_READY) {
		    f = hwndMovie ? MF_ENABLED : MF_GRAYED;
                    EnableMenuItem((HMENU)wParam, MENU_CLOSE, f);
                    EnableMenuItem((HMENU)wParam, MENU_CLOSEALL, f);

                    EnableMenuItem((HMENU)wParam, IDM_STOP, MF_GRAYED);
                    EnableMenuItem((HMENU)wParam, IDM_PLAY, MF_GRAYED);
                    EnableMenuItem((HMENU)wParam, IDM_RPLAY, MF_GRAYED);
                    EnableMenuItem((HMENU)wParam, IDM_HOME, MF_GRAYED);
                    EnableMenuItem((HMENU)wParam, IDM_END, MF_GRAYED);
                    EnableMenuItem((HMENU)wParam, IDM_STEP, MF_GRAYED);
                    EnableMenuItem((HMENU)wParam, IDM_RSTEP, MF_GRAYED);
                } else {
                   EnableMenuItem((HMENU)wParam, MENU_CLOSE, MF_ENABLED);
                   EnableMenuItem((HMENU)wParam, MENU_CLOSEALL, MF_ENABLED);
   
                   f = MCIWndGetMode(hwndMovie, NULL, 0) != MCI_MODE_STOP;
                   EnableMenuItem((HMENU)wParam, IDM_PLAY,
				!f ? MF_ENABLED : MF_GRAYED);
                   EnableMenuItem((HMENU)wParam, IDM_RPLAY,
				!f ? MF_ENABLED : MF_GRAYED);
                   EnableMenuItem((HMENU)wParam, IDM_STOP,
				 f ? MF_ENABLED : MF_GRAYED);
                   EnableMenuItem((HMENU)wParam, IDM_HOME, MF_ENABLED);
                   EnableMenuItem((HMENU)wParam, IDM_END,  MF_ENABLED);
                   EnableMenuItem((HMENU)wParam, IDM_STEP, MF_ENABLED);
                   EnableMenuItem((HMENU)wParam, IDM_RSTEP,MF_ENABLED);
               }
	    }

            return mdiSendMessage(hwndMdi, NULL, msg, wParam, lParam);
            break;

       case WM_CREATE:
            hmenu = GetMenu(hwnd);
            hwndMdi = mdiCreateClient(hwnd, GetSubMenu(hmenu, GetMenuItemCount(hmenu)-1));
            break;

       case WM_SIZE:
            MoveWindow(hwndMdi,0,0,LOWORD(lParam),HIWORD(lParam),TRUE);
            break;

       case WM_DESTROY:
	    hwndApp = NULL;
	    PostQuitMessage(0);
	    break;

       case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            return 0;
    }
    return DefFrameProc(hwnd,hwndMdi,msg,wParam,lParam);
}

/*----------------------------------------------------------------------------*\
|   ErrMsg - Opens a Message box with a error message in it.  The user can     |
|	     select the OK button to continue or the CANCEL button to kill     |
|	     the parent application.					       |
\*----------------------------------------------------------------------------*/
int ErrMsg (LPSTR sz,...)
{
    char ach[128];
    wvsprintf(ach,sz,(LPSTR)(&sz+1));   /* Format the string */
    MessageBox (NULL,ach,NULL,
#ifdef BIDI
		MB_RTL_READING |
#endif
    MB_OK|MB_ICONEXCLAMATION);
    return FALSE;
}

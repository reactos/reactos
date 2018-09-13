/****************************************************************************/
/*                                                                          */
/*       Touched by      :       Diane K. Oh                                */
/*       On Date         :       June 11, 1992                              */
/*       Revision remarks by Diane K. Oh ext #15201                         */
/*       This file has been changed to comply with the Unicode standard     */
/*       Following is a quick overview of what I have done.                 */
/*                                                                          */
/*       Was               Changed it into   Remark                         */
/*       ===               ===============   ======                         */
/*       CHAR              TCHAR             if it refers to text           */
/*       LPCHAR & LPSTR    LPTSTR            if it refers to text           */
/*       PSTR & NPSTR      LPTSTR            if it refers to text           */
/*       "..."             TEXT("...")       compile time macro resolves it */
/*       '...'             TEXT('...')       same                           */
/*                                                                          */
/*       strlen            lstrlen           compile time macro resolves it */
/*                                                                          */
/*  Notes:                                                                  */
/*                                                                          */
/*    1. Added LPTSTR typecast before MAKEINTRESOURCE to remove warning     */
/*    2. Size was multiplied by sizeof(TCHAR) when allocating strings.      */
/*    3. WinMain has not been Unicode enabled so lpCmdLine parameter        */
/*         remains as LPSTR type.                                           */
/*    4. Changed typecast from FARPROC to LPCFHOOKPROC to remove warning    */
/*                                                                          */
/****************************************************************************/

#include <windows.h>               /* required for all Windows applications */
#include <stdlib.h>
#include "commdlg.h"
#include "shellapi.h"
#include "clock.h"                 /* specific to this program              */

#if defined(JAPAN)
#include <dlgs.h>
#endif


BOOL InitApplication (HANDLE);
HRGN CreateEllipticWndRgn(HWND,LPRECT);

LRESULT FAR PASCAL MainWndProc (HWND, UINT, WPARAM, LPARAM);

#if defined(JAPAN)
#define NATIVE_CHARSET  SHIFTJIS_CHARSET

UINT APIENTRY ExceptVerticalFont(HWND, UINT, UINT, LONG);
#endif

HANDLE hInst;                       /* current instance                      */
void  ParseSavedWindow(LPTSTR szBuf, PRECT pRect );
void NEAR PASCAL DeleteTools (void);
void NEAR PASCAL PrintShadowText (register HDC hDC, int nx, int ny, int sizex, int sizey, LPTSTR pszStr, int nStrLen, HDC hOffScrnDC);
void NEAR PASCAL ClockSize (register HWND hWnd,int newWidth,int newHeight,WORD SizeWord);
void NEAR PASCAL ClockTimerInterval (HWND hWnd);
void NEAR PASCAL CompClockDim (void);
void NEAR PASCAL ClockTimer (HWND hWnd);
void NEAR PASCAL FormatTimeStr (void);
void NEAR PASCAL ClockPaint (HWND hWnd, register HDC hDC, int hint);
void NEAR PASCAL DrawFace (HDC hDC);
void NEAR PASCAL DrawFatHand (register HDC hDC, int pos, HPEN hPen, BOOL hHand);
void NEAR PASCAL DrawHand (register HDC hDC, int pos, HPEN hPen, int scale, int patMode);
void NEAR PASCAL DrawIconBorder (HWND hWnd, register HDC hDC);
void NEAR PASCAL FormatDateStr (xDATE *pDate, BOOL bRealShort);
void NEAR PASCAL SizeFont (HWND hWnd, int newHeight, int newWidth);
void NEAR PASCAL SetMenuBar (HWND hWnd);
void NEAR PASCAL FormatInit (VOID);
VOID NEAR PASCAL SaveClockOptions (HWND hWnd);
void NEAR PASCAL ResetWinTitle (HWND hWnd);
void NEAR PASCAL CreateTools (void);
void NEAR PASCAL ClockCreate (HWND hWnd);

TIME    oTime;
xDATE   oDate;

CLOCKDISPSTRUCT ClockDisp;

LOGFONT    FontStruct;
typedef struct
{
    SHORT x;
    SHORT y;

} TRIG;


#define FNOSHAD_MONOCHROME 1
#define FNOSHAD_USERSAYSNO 2

short   fNoShadow = 0;

BOOL    bUtc       = FALSE;
BOOL    bFirst     = TRUE;
BOOL    bColor     = TRUE;
BOOL    fShadedHands = FALSE;
BOOL    bNewFont   = TRUE;
BOOL    bCantHide = FALSE;
BOOL    bTmpHide = FALSE;
BOOL    fDisplay = FALSE;
TCHAR   szBuffer[BUFLEN];    /* buffer for stringtable stuff */
TCHAR   szAppName[BUFLEN];
TCHAR   szIniFile[20];
TCHAR   szDfltFontFile[20];
TCHAR   szSection[30];
TCHAR   szFontFileKey[20];
HBRUSH  hbrColorWindow;
HBRUSH  hbrBtnHighlight;
HBRUSH  hbrForeground;
HBRUSH  hbrBlobColor;
HFONT   hFont = NULL;
HFONT   hFontDate = NULL;
#define bDisplayDate    (!ClockDisp.bNoDate && !ClockDisp.bIconic)

RECT    clockRect;
RECT    rCoordRect;
TRIG    clockCenter;

HPEN    hpenForeground;
HPEN    hpenShadow;
HPEN    hpenBackground;
HPEN    hpenBlobHlt;
HPEN    hpenRed;
HCURSOR hCurWait;


/* win.ini strings...   Don't internationalize */
TCHAR   szMaximized[] = TEXT("Maximized");
TCHAR   szOptions[]   = TEXT("Options");
TCHAR   szPosition[]  = TEXT("Position");
TCHAR   szNoShadow[]  = TEXT("NoShadow");

#if defined(JAPAN)
TCHAR   szCharSet[]   = TEXT("Charset");
#endif

int     TimerID = 1;    /* number used for timer-id */
int     clockRadius;
int     HorzRes;
int     VertRes;

int     aspectD;
int     aspectN;

extern TRIG CirTab[];
TRIG FAR *lpcirTab = CirTab;

#if defined(JAPAN) || defined(KOREA)
UINT FAR PASCAL  ExceptVerticalFont(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
#endif


void MoveTo (HDC hdc, int x, int y)
{
   MoveToEx (hdc, x, y, NULL);
}

INT MyAtoi (LPTSTR  Str)
{
  CHAR    szAnsi [160];
  BOOL    fDefCharUsed;

#ifdef UNICODE
   WideCharToMultiByte (CP_ACP, 0, Str, -1, szAnsi, 160, NULL, &fDefCharUsed);
   return (atoi (szAnsi));
#else
   return (atoi (Str));
#endif

}

/****************************************************************************

    FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)

    PURPOSE: calls initialization function, processes message loop

    COMMENTS:

        Windows recognizes this function by name as the initial entry point
        for the program.  This function calls the application initialization
        routine, if no other instance of the program is running, and always
        calls the instance initialization routine.  It then executes a message
        retrieval and dispatch loop that is the top-level control structure
        for the remainder of execution.  The loop is terminated when a WM_QUIT
        message is received, at which time this function exits the application
        instance by returning the value passed by PostQuitMessage().

        If this function must abort before entering the message loop, it
        returns the conventional value NULL.

****************************************************************************/

int APIENTRY WinMain (HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPSTR lpCmdLine,
                      int nCmdShow)
{

    MSG        msg;                        /* message */
    HDC        hDC;
    HWND       hWnd;
    HMENU      hMenu;
    int        iMaximized;
    TCHAR      szParseStr [80];
    TIME       nTime;
    LPTSTR     szTooMany;
    TCHAR      szTopmost [80];
#ifdef JAPAN
    int        i;
#endif
    UNREFERENCED_PARAMETER( lpCmdLine );


    hInst = hInstance;
    FormatInit ();
    if (!InitApplication (hInstance)) /* Initialize shared things       */
       return (FALSE);                /* Exits if unable to initialize  */

    /* Perform initializations that apply to a specific instance */
    LoadString (hInstance, IDS_APPNAME, szAppName, BUFLEN);
    LoadString (hInstance, IDS_USNAME, szSection, 30);
    LoadString (hInstance, IDS_INIFILE, szIniFile, 20);
    ClockCreate ((HWND)NULL);


    hDC = GetDC (NULL);
    if ((UINT)GetDeviceCaps (hDC, NUMCOLORS) <= 2)
        fNoShadow = FNOSHAD_MONOCHROME;
    ReleaseDC (NULL, hDC);
    if (GetPrivateProfileInt (szSection, szNoShadow, 0, szIniFile))
        fNoShadow |= FNOSHAD_USERSAYSNO;

    /* get window position and size from ini file */
    GetPrivateProfileString (szSection, szPosition, TEXT(""), szParseStr,
                             80, szIniFile);
    ParseSavedWindow (szParseStr, &rCoordRect);


    hWnd = CreateWindow (szSection,      /* The class name.             */
                         szSection,      /* The window instance name.   */
                         WS_TILEDWINDOW,
                         rCoordRect.left, rCoordRect.top,
                         rCoordRect.right, rCoordRect.bottom,
                         NULL,
                         NULL,
                         hInstance,
                         NULL);

    // Loop if control panel time being changed.
    GetTime (&oTime);
    do
    {
        GetTime (&nTime);
    }
    while (nTime.second == oTime.second && nTime.minute == oTime.minute &&
           nTime.hour24 == oTime.hour24);

    GetDate (&oDate);

    if (!SetTimer (hWnd, TimerID, OPEN_TLEN, 0L))
    {
        /* Windows only supports 16 public timers */
        szTooMany = (LPTSTR) LocalAlloc (LPTR, 160 * sizeof(TCHAR));
        LoadString (hInstance, IDS_TOOMANY, szTooMany, 160);
        MessageBox ((HWND)NULL, szTooMany, szBuffer, MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);
        DeleteTools ();
        return (FALSE);
    }

    /* get font choice from ini file */
#ifdef JAPAN
    lstrcpy (szDfltFontFile, TEXT("System"));
#else
    LoadString (hInstance, IDS_FONTFILE, szDfltFontFile, 20);
#endif
    LoadString (hInstance, IDS_FONTCHOICE, szFontFileKey, 20);
    GetPrivateProfileString (szSection, szFontFileKey, szDfltFontFile,
                             FontStruct.lfFaceName, 20, szIniFile);
    FontStruct.lfPitchAndFamily = FF_SWISS | VARIABLE_PITCH;
#ifdef JAPAN
    if (!(i = GetPrivateProfileInt(szSection, szCharSet,
        ANSI_CHARSET, szIniFile)))
        FontStruct.lfCharSet = ANSI_CHARSET;
    else
        FontStruct.lfCharSet = i;
#else
    FontStruct.lfCharSet = ANSI_CHARSET;
#endif
    FontStruct.lfWeight = FW_NORMAL;
    FontStruct.lfQuality = DEFAULT_QUALITY;
    FontStruct.lfUnderline = FALSE;
    FontStruct.lfStrikeOut = FALSE;
    FontStruct.lfEscapement = 0;
    FontStruct.lfOrientation = 0;
    FontStruct.lfOutPrecision = OUT_DEFAULT_PRECIS;
    FontStruct.lfClipPrecision = CLIP_DEFAULT_PRECIS;

#ifdef JAPAN
    /* We can use only NATIVE_CHARSET */
    if (FontStruct.lfCharSet != NATIVE_CHARSET)
    {
        FontStruct.lfCharSet = NATIVE_CHARSET;
        lstrcpy (FontStruct.lfFaceName, TEXT("System"));
    }
#endif


    /* get clock options from ini file */
    GetPrivateProfileString (szSection, szOptions, TEXT(""), szParseStr, 80, szIniFile);
    ParseSavedFlags (szParseStr, &ClockDisp);

    FormatTimeStr ();
    FormatDateStr (&oDate, ClockDisp.bIconic);

    hMenu = GetMenu (hWnd);
    /* Check the default menu item either analog or digital */
    CheckMenuItem (hMenu, ClockDisp.wFormat, MF_BYCOMMAND | MF_CHECKED);
    if (ClockDisp.wFormat == IDM_ANALOG)
        EnableMenuItem( hMenu, IDM_SETFONT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

    if (!ClockDisp.bNoSeconds)
    {
        CheckMenuItem (hMenu, IDM_SECONDS, MF_BYCOMMAND | MF_CHECKED);
        ClockTimerInterval (hWnd);
    }
    if (!ClockDisp.bNoDate)
    {
        CheckMenuItem (hMenu, IDM_DATE, MF_BYCOMMAND | MF_CHECKED);
        ResetWinTitle (hWnd);
    }
    hMenu = GetSystemMenu (hWnd, FALSE);
    AppendMenu (hMenu, MF_SEPARATOR, 0, NULL);

    LoadString (hInstance, IDS_TOPMOST, szTopmost, 79);

    if (ClockDisp.bTopMost)
    {
        AppendMenu (hMenu, MF_ENABLED | MF_CHECKED | MF_STRING, IDM_TOPMOST,
                    szTopmost);
        SetWindowPos (hWnd, (HWND)-1, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    else
        AppendMenu (hMenu, MF_ENABLED | MF_UNCHECKED | MF_STRING, IDM_TOPMOST,
                    szTopmost);


    if (ClockDisp.bNoTitle)
       SetMenuBar (hWnd);


    if (!ClockDisp.bIconic)
    {
        iMaximized = GetPrivateProfileInt (szSection, szMaximized, 0, szIniFile);
        if (iMaximized)
            ShowWindow (hWnd, SW_MAXIMIZE);
        else
        {
            ShowWindow (hWnd, nCmdShow);
            GetWindowRect (hWnd, &rCoordRect);
        }
    }
    else
        ShowWindow (hWnd, SW_MINIMIZE);


    /* Acquire and dispatch messages until a WM_QUIT message is received. */
    while (GetMessage (&msg,        /* message structure                      */
                       NULL,        /* handle of window receiving the message */
                       0,        /* lowest message to examine              */
                       0))       /* highest message to examine             */
    {
        TranslateMessage (&msg);    /* Translates virtual key codes           */
        DispatchMessage (&msg);     /* Dispatches message to window           */
    }
    return (int)(msg.wParam);       /* Returns the value from PostQuitMessage */

    hPrevInstance; //UNUSED
    nCmdShow; //UNUSED
}


/****************************************************************************

    FUNCTION: InitApplication(HANDLE)

    PURPOSE: Initializes window data and registers window class

    COMMENTS:

        This function is called at initialization time only if no other
        instances of the application are running.  This function performs
        initialization tasks that can be done once for any number of running
        instances.

        In this case, we initialize a window class by filling out a data
        structure of type WNDCLASS and calling the Windows RegisterClass()
        function.  Since all instances of this application use the same window
        class, we only need to do this when the first instance is initialized.


****************************************************************************/

BOOL InitApplication (HANDLE hInstance)       /* current instance */
{
   WNDCLASS  ClockClass;

    /* Fill in window class structure with parameters that describe the       */
    /* main window.                                                           */
    ClockClass.cbClsExtra = ClockClass.cbWndExtra = 0;

    ClockClass.lpszClassName = TEXT("Clock");
    ClockClass.lpszMenuName  = TEXT("Clock"); //szSection;
    ClockClass.hbrBackground = GetStockObject(LTGRAY_BRUSH);
    ClockClass.style         = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
    ClockClass.hInstance     = hInstance;
    ClockClass.lpfnWndProc   = MainWndProc;

    ClockClass.hCursor       = LoadCursor (NULL, IDC_ARROW);
    ClockClass.hIcon         = NULL;

    /* Register the window class and return success/failure code. */
    return (RegisterClass (&ClockClass));
}



/****************************************************************************

    FUNCTION: MainWndProc(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages

    MESSAGES:

        WM_COMMAND    - application menu (About dialog box)
        WM_DESTROY    - destroy window

****************************************************************************/

LRESULT APIENTRY MainWndProc (HWND hWnd,         /* window handle          */
                              UINT message,      /* type of message        */
                              WPARAM wParam,     /* additional information */
                              LPARAM lParam)     /* additional information */
{
  HMENU       hMenu;
  PAINTSTRUCT ps;

    switch (message)
    {
       case WM_COMMAND:
           switch (LOWORD (wParam))
           {
               case IDM_ANALOG:
               case IDM_DIGITAL:
                   if (LOWORD (wParam) != ClockDisp.wFormat)
                   {
                       /* Switch flag to other choice */
                       hMenu = GetMenu(hWnd);
                       CheckMenuItem(hMenu, ClockDisp.wFormat, MF_BYCOMMAND | MF_UNCHECKED);
                       CheckMenuItem(hMenu, ClockDisp.wFormat = (WORD)wParam, MF_BYCOMMAND | MF_CHECKED);
                       if (wParam == IDM_ANALOG)
                           EnableMenuItem (hMenu, IDM_SETFONT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
                       else
                           EnableMenuItem (hMenu, IDM_SETFONT, MF_BYCOMMAND | MF_ENABLED);
                       ResetWinTitle (hWnd);
                       InvalidateRect (hWnd, (LPRECT)NULL, TRUE);
                   }
                   break;


               case IDM_SETFONT:
               {
                   CHOOSEFONT  cf;
                   short cy, nOldFontHeight;
#ifdef JAPAN
                   /* Win 3.1 */
                   LOGFONT OldFontStruct;
#endif

                   /* calls the font chooser (in commdlg)
                    */
                   cf.lStructSize = sizeof (CHOOSEFONT);
                   cf.hwndOwner = hWnd;
                   cf.hInstance = hInst;
                   cf.lpTemplateName = (LPTSTR) MAKEINTRESOURCE(IDD_FONT);
                   cf.hDC = NULL;
                   cf.lpLogFont = &FontStruct;
                   FontStruct.lfItalic = 0;
                   FontStruct.lfWeight = FW_NORMAL;
                   cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT |
#ifdef JAPAN
                              CF_NOSIMULATIONS | CF_ENABLETEMPLATE |
#else
                              CF_NOSIMULATIONS | CF_ANSIONLY | CF_ENABLETEMPLATE |
#endif
                              CF_NOVECTORFONTS;
#if defined(JAPAN)
                   /* except vertical font from list */
                   cf.Flags |= CF_ENABLEHOOK;
                   cf.lpfnHook = (LPCFHOOKPROC)ExceptVerticalFont;
#else
                   cf.lpfnHook = (LPCFHOOKPROC) NULL;
#endif

#ifdef JAPAN
                   /* Win 3.1 */
                   OldFontStruct = FontStruct;
#endif
                   nOldFontHeight = (short) FontStruct.lfHeight;
                   cy = HIWORD (GetDialogBaseUnits ());

                   /* 36 is the (height - 1) of the stc5 static control in font.dlg template */
                   if (((36 * cy) / 8) < -FontStruct.lfHeight)
                      FontStruct.lfHeight = -((36 * cy) / 8);

                   if (ChooseFont (&cf))
                   {
#if defined(JAPAN) || defined(KOREA)
                    /* We can use only NATIVE_CHARSET */
                      if (FontStruct.lfCharSet != NATIVE_CHARSET)
                      {
                         FontStruct = OldFontStruct;
                         break;
                      }
#endif
                      /* write new font info to ini file */
                       WritePrivateProfileString (szSection, szFontFileKey,
                                                  FontStruct.lfFaceName, szIniFile);
                       bNewFont = TRUE;
                       InvalidateRect(hWnd, NULL, TRUE);
                   }
                   else  /* restore old height */
                      FontStruct.lfHeight = nOldFontHeight;
                   break;
               }

               case IDM_UTC:
                   hMenu = GetMenu(hWnd);
                   if (!bUtc)
                   {
                       bUtc = TRUE;
                       CheckMenuItem(hMenu, IDM_UTC, MF_BYCOMMAND | MF_CHECKED);
                   }
                   else
                   {
                       bUtc = FALSE;
                       CheckMenuItem (hMenu, IDM_UTC, MF_BYCOMMAND | MF_UNCHECKED);
                   }
                   // call FormatInit to add or remove am/pm string (utc
                   // doesn't have it, non-utc does)
                   FormatInit();
                   ResetWinTitle (hWnd);
                   FormatTimeStr ();
                   bNewFont = TRUE;
                   InvalidateRect (hWnd, (LPRECT)NULL, TRUE);
                   ClockTimer (hWnd);
                   break;

               case IDM_NOTITLE:
                   goto toggle_title;

               case IDM_SECONDS:

                   /* toggle seconds option
                    */
                   hMenu = GetMenu (hWnd);
                   if (ClockDisp.bNoSeconds)
                   {
                       CheckMenuItem (hMenu, IDM_SECONDS, MF_BYCOMMAND | MF_CHECKED);
                       ClockDisp.bNoSeconds = FALSE;
                   }
                   else
                   {
                       CheckMenuItem (hMenu, IDM_SECONDS, MF_BYCOMMAND | MF_UNCHECKED);
                       ClockDisp.bNoSeconds = TRUE;
                   }
                   ClockTimerInterval (hWnd);
                   FormatTimeStr ();
                   bNewFont = TRUE;
                   InvalidateRect (hWnd, (LPRECT)NULL, TRUE);

                   break;

               case IDM_DATE:
                   /* toggles date option
                    */
                   hMenu = GetMenu (hWnd);
                   if (ClockDisp.bNoDate)
                   {
                       CheckMenuItem (hMenu, IDM_DATE, MF_BYCOMMAND | MF_CHECKED);
                       ClockDisp.bNoDate = FALSE;
                   }
                   else
                   {
                       CheckMenuItem (hMenu, IDM_DATE, MF_BYCOMMAND | MF_UNCHECKED);
                       ClockDisp.bNoDate = TRUE;
                   }
                   bNewFont = TRUE;
                   if (ClockDisp.wFormat == IDM_DIGITAL)
                       InvalidateRect(hWnd, (LPRECT)NULL, TRUE);
                   else
                       ResetWinTitle (hWnd);
                   break;

               case IDM_ABOUT:
                   ShellAbout(hWnd, szAppName, TEXT(""), LoadIcon(hInst, TEXT("cckk")));
                   break;

               default:
                   goto defproc;
           }
           break;

       case WM_MOUSEACTIVATE:
           /* right button temporarily hides the window if topmost is
            * enabled (window re-appears when right button is released).
            * When this happens, we don't want to activate the clock window
            * just before hiding it (it would look really bad), so we
            * intercept the activate message.
            */
           if (GetAsyncKeyState (VK_RBUTTON) & 0x8000)
               return (MA_NOACTIVATE);
           else
               goto defproc;
           break;

       case WM_INITMENU:
           bCantHide = TRUE;
           goto defproc;

       case WM_MENUSELECT:
           if (LOWORD (lParam) == -1  && HIWORD(lParam) == 0)
               bCantHide = FALSE;
           goto defproc;

       case WM_RBUTTONDOWN:
       case WM_NCRBUTTONDOWN:
           /* right button temporarily hides the window, if the window
            * is topmost, and if no menu is currently "active"
            */
           if (!bTmpHide && ClockDisp.bTopMost && !bCantHide)
           {
               ShowWindow (hWnd, SW_HIDE);
               SetCapture (hWnd);
               bTmpHide = TRUE;
           }
           break;

       case WM_RBUTTONUP:
       case WM_NCRBUTTONUP:
           /* if window is currently hidden, right button up brings it
            * back. Must make sure we show it in its previous state - ie:
            * minimized, maximized or normal.
            */
           if (bTmpHide)
           {
               ReleaseCapture ();
               if (ClockDisp.bIconic)
                   ShowWindow (hWnd, SW_SHOWMINNOACTIVE);
               else if (IsZoomed (hWnd))
                   ShowWindow (hWnd, SW_SHOWMAXIMIZED);
               else
                   ShowWindow (hWnd, SW_SHOWNOACTIVATE);
               bTmpHide = FALSE;
           }
           break;


       case WM_KEYDOWN:
           /* ESC key toggles the menu/title bar (just like a double click
            * on the client area of the window.
            */
           if ((wParam == VK_ESCAPE) && !(HIWORD (lParam) & 0x4000))
               goto toggle_title;
           break;

       case WM_NCLBUTTONDBLCLK:
           if (!ClockDisp.bNoTitle)
               /* if we have title bars etc. let the normal sutff take place */
               goto defproc;
           /* else: no title bars, then this is actually a request to bring
            * the title bars back...
            */
           /* fall through */

       case WM_LBUTTONDBLCLK:
toggle_title:
           fDisplay = FALSE;
           ClockDisp.bNoTitle = (ClockDisp.bNoTitle ? FALSE : TRUE);
           SetMenuBar (hWnd);
           break;

       case WM_NCHITTEST:
           /* if we have no title/menu bar, clicking and dragging the client
            * area moves the window. To do this, return HTCAPTION.
            * Note dragging not allowed if window maximized, or if caption
            * bar is present.
            */
           wParam = DefWindowProc(hWnd, message, wParam, lParam);
           if (ClockDisp.bNoTitle && (wParam == HTCLIENT) && !IsZoomed(hWnd))
               return HTCAPTION;
           else
               return wParam;

       case WM_SIZE:
           bNewFont = TRUE;
           ClockSize(hWnd, (int) LOWORD (lParam), (int) HIWORD (lParam), (WORD) wParam);
           UpdateWindow(hWnd);
           break;

       case WM_QUERYDRAGICON:
           return (LRESULT) LoadIcon (hInst, TEXT("cckk"));

       case WM_DESTROY:
       {
         HCURSOR   hTempCursor;

           KillTimer (hWnd, TimerID);
           DeleteTools ();
           if (hFont)
               DeleteObject (hFont);
           if (hFontDate)
               DeleteObject (hFontDate);
           if (ClockDisp.hBitmap)
               DeleteObject (ClockDisp.hBitmap);

           SetCapture (hWnd);
           hTempCursor = SetCursor (LoadCursor (NULL, IDC_WAIT));

           SaveClockOptions (hWnd);

           PostQuitMessage (0);
           break;
       }

       case WM_WININICHANGE:
           FormatInit ();
           FormatDateStr (&oDate, ClockDisp.bIconic);
           ResetWinTitle (hWnd);
           FormatTimeStr ();
           bNewFont = TRUE;
           InvalidateRect (hWnd, (LPRECT)NULL, TRUE);
           break;

       case WM_PAINT:

           /* 25-Mar-1987. Added to force total repaint to solve
            * problem of garbage under second hand when hidden
            * by menu or popup.
            */
           InvalidateRect (hWnd, (LPRECT)NULL, TRUE);
           fDisplay = FALSE;
           BeginPaint (hWnd, &ps);
           ClockPaint (hWnd, ps.hdc, REPAINT);
           EndPaint (hWnd, &ps);
           break;

       case WM_TIMECHANGE:
           /* Redraw. */
           oDate.day = 0;
           InvalidateRect (hWnd, (LPRECT)NULL, TRUE);
           /* fall through */

       case WM_TIMER:
           ClockTimer (hWnd);
           break;

       case WM_SYSCOMMAND:
           switch (wParam)
           {
               case SC_MINIMIZE:
                   if (!IsZoomed (hWnd))
                       GetWindowRect (hWnd, &rCoordRect);
                   if (ClockDisp.bTopMost)
                   {
                       ClockDisp.bTopMost = FALSE;
                       PostMessage (hWnd, WM_SYSCOMMAND, IDM_TOPMOST, 0L);
                   }
                   break;

               case SC_MAXIMIZE:
                   if (!IsIconic (hWnd))
                       GetWindowRect (hWnd, &rCoordRect);
                   break;

               case IDM_TOPMOST:
               {
                   /* toggles topmost option
                    */
                   hMenu = GetSystemMenu (hWnd, FALSE);
                   if (ClockDisp.bTopMost)
                   {
                       CheckMenuItem (hMenu, IDM_TOPMOST, MF_BYCOMMAND | MF_UNCHECKED);
                       SetWindowPos (hWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                       ClockDisp.bTopMost = FALSE;
                   }
                   else
                   {
                       CheckMenuItem (hMenu, IDM_TOPMOST, MF_BYCOMMAND | MF_CHECKED);
                       SetWindowPos (hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                       ClockDisp.bTopMost = TRUE;
                   }
                   break;
               }
           }
           return (DefWindowProc(hWnd, message, wParam, lParam));
           break;

       case WM_SYSCOLORCHANGE:
           DeleteTools ();
           CreateTools ();
           break;

       case WM_ERASEBKGND:
       {
         RECT rect;
         HBRUSH  hBr;
         DWORD   rgbCol;
         HDC     hDC;

           GetClientRect (hWnd, &rect);
           hDC = GetDC (hWnd);
           SetBkMode (hDC, OPAQUE);

           /* Make a temp brush to color the background.  This is to
            * force use of a solid color so the hand motion is painted
            * correctly.
            */
           rgbCol = GetNearestColor(hDC, GetSysColor(COLOR_BTNFACE));


           /* CHECK RETURN VALUE!! */
           hBr = CreateSolidBrush(rgbCol);

           FillRect((HDC)wParam, &rect, hBr);
           ReleaseDC (hWnd, hDC);
           DeleteObject(hBr);
           break;
       }

       case WM_ENDSESSION:
           if (wParam)
               SaveClockOptions (hWnd);
           break;

       default:                          /* Passes it on if unproccessed    */
defproc:
           return (DefWindowProc (hWnd, message, wParam, lParam));
    }
    return (0);
}


/*
 *  ResetWinTitle() - Sets the window title with the date if appropriate
 *
 *  The date is part of the window text when the clock is minimized, or when
 *  it is analog, and the date option is selected.
 */

void NEAR PASCAL ResetWinTitle (HWND hWnd)
{
TCHAR    szNewTitle[BUFLEN+20];

    if (((ClockDisp.wFormat == IDM_DIGITAL) && !ClockDisp.bIconic)
        || ClockDisp.bNoDate)
        SetWindowText (hWnd, szAppName);   /* no date in title if digital */
    else
    {
        wsprintf (szNewTitle, TEXT("%s - %s"), szAppName, ClockDisp.szDate);
        SetWindowText (hWnd, szNewTitle);
    }
}


void NEAR PASCAL CreateTools (void)
{
#define BLOB_COLOR  RGB (0,128,128)

    hbrForeground  = CreateSolidBrush (GetSysColor (COLOR_BTNSHADOW));
    hbrColorWindow = CreateSolidBrush (GetSysColor (COLOR_BTNFACE));
    hbrBtnHighlight= CreateSolidBrush (GetSysColor (COLOR_BTNHIGHLIGHT));
    hbrBlobColor   = CreateSolidBrush (BLOB_COLOR);
    hpenForeground = CreatePen (0, 1, GetSysColor (COLOR_WINDOWTEXT));
    hpenShadow     = CreatePen (0, 1, GetSysColor (COLOR_BTNSHADOW));
    hpenBackground = CreatePen (0, 1, GetSysColor (COLOR_BTNFACE));
    hpenBlobHlt    = CreatePen (0, 1, RGB (0,255,255));
    hpenRed        = CreatePen (0, 1, RGB (255, 0, 0));
    hCurWait       = LoadCursor (NULL, IDC_WAIT);
}


void NEAR PASCAL ClockCreate (HWND hWnd)
{
  register HDC    hDC;
  int             HorzSize;
  int             VertSize;


    hDC = GetDC (hWnd);
    VertRes = GetDeviceCaps (hDC, VERTRES);
    HorzRes = GetDeviceCaps (hDC, HORZRES);
    VertSize= GetDeviceCaps (hDC, VERTSIZE);
    HorzSize= GetDeviceCaps (hDC, HORZSIZE);
    ReleaseDC (hWnd, hDC);

    aspectN = MulDiv (VertRes, 100, VertSize);
    aspectD = MulDiv (HorzRes, 100, HorzSize);

    CreateTools ();

}


/*
 *  ClockTimerInterval()
 *
 *  Sets the timer interval. Two things affect this interval:
 *    1) if the window is iconic, or
 *    2) if Seconds option has been disabled
 *  In both cases, timer ticks occur every half-minute. Otherwise, timer
 *  every half-second.
 *  Gets the flags from ClockDisp structure.
 */

void  PASCAL ClockTimerInterval (HWND hWnd)
{

    if (ClockDisp.bIconic || ClockDisp.bNoSeconds)
    {
        /* Update once every 1/2 minute in the iconic state, or if
         * seconds display suppressed
         */
        KillTimer (hWnd, TimerID);
        SetTimer (hWnd, TimerID, (WORD)ICON_TLEN, 0L);
    }
    else
    {
        /* Update every 1/2 second in the opened state. */
        KillTimer (hWnd, TimerID);
        SetTimer (hWnd, TimerID, OPEN_TLEN, 0L);
    }
}


/*
 *  ClockTimer()
 *
 *      msg - timer ID
 *
 * Called by windows to tell CLOCK there has been a time change.
 *
 */

void NEAR PASCAL ClockTimer (HWND hWnd)
{
  HDC     hDC;
  TIME    nTime;
  xDATE   nDate;


    GetTime (&nTime);
    GetDate (&nDate);
    if (fDisplay)
       return;

    if (ClockDisp.bNoSeconds || ClockDisp.bIconic)
    {
        KillTimer (hWnd, TimerID);
        SetTimer (hWnd, TimerID, (WORD)(61 - nTime.second) * 100, 0L);
    }
    /* It's possible to change any part of the system at any time
     * through the Control Panel.  So we check everything.
     */
    if (((nTime.second == oTime.second) || ClockDisp.bIconic || ClockDisp.bNoSeconds) &&
        (nTime.minute == oTime.minute)          &&
        (nTime.hour24 == oTime.hour24)          &&
        ((nDate.day == oDate.day    &&
         nDate.month == oDate.month &&
         nDate.year == oDate.year ) || ClockDisp.bNoDate))
        return;

    hDC = GetDC (hWnd);
    ClockPaint (hWnd, hDC, HANDPAINT);
    ReleaseDC (hWnd, hDC);
}


VOID GetTime (TIME * pTime)
{
  SYSTEMTIME st;

    if  (bUtc)
       GetSystemTime (&st);
    else
       GetLocalTime (&st);

    if (st.wHour > 11)
       pTime->ampm = 1;
    else
       pTime->ampm = 0;

    if (st.wHour == 0 && !ClockDisp.wTimeFormat)
       st.wHour = 12;

    pTime->hour = st.wHour;
    pTime->hour24 = pTime->hour12 = st.wHour;
    if (pTime->hour12 > 12)
        pTime->hour12 -= 12;

    pTime->minute = st.wMinute;
    pTime->second = st.wSecond;
}

void NEAR GetDate (xDATE * pDate)
{
  SYSTEMTIME st;

    if (bUtc)
       GetSystemTime(&st);
    else
       GetLocalTime(&st);

    pDate->day= st.wDay;
    pDate->month= st.wMonth;
    pDate->year= st.wYear;
}


/*
 * FormatTimeStr()
 *
 * Creates a template for the time, based on options in the ClockDisp
 * structure. ie: seconds or no seconds, AM/PM string, etc.
 * The result is used in ExtTextOut calls to determine the size of the
 * time string.
 * Note that this is based on the assumption that digits are fixed pitch
 * (and so '8' is used as a dummy digit).
 * Bad things will happen if the selected font has proportional digits!!!
 */

void NEAR PASCAL FormatTimeStr (void)
{
    LPTSTR  pstr;
    int     i;
    TCHAR   szAMPM[MAX_AMPM_LEN];
    TCHAR   szTime[MAX_TIME_SEP + MAX_AMPM_LEN + 8];
    int     iAMPMLen;


    pstr = ClockDisp.szTimeFmt;

    if (!ClockDisp.bIconic)
    {
        /*
         *  Build the widest possible AM/PM string.
         *  This assumes the widest proportional character is M.
         */
        iAMPMLen = ClockDisp.nMaxAMPMLen;
        for (i = 0; i < iAMPMLen; i++)
        {
            szAMPM[i] = TEXT('M');
        }

        if (ClockDisp.wAMPMPosition && iAMPMLen)
        {
            /*
             *  Length includes a space, so write the space to the
             *  buffer instead of the M.
             */
            szAMPM[i - 1] = TEXT(' ');
        }
        szAMPM[i] = TEXT('\0');

        /*
         *  Get the Time string.
         */
        wsprintf (szTime, TEXT("88%c88"), ClockDisp.szTimeSep[0]);
        if (!ClockDisp.bNoSeconds)
        {
            wsprintf (szTime + 5, TEXT("%c88"), ClockDisp.szTimeSep[0]);
        }
        lstrcat (szTime, TEXT(" "));

        /*
         *  Build the actual format string in the proper order.
         */
        if (ClockDisp.wAMPMPosition)
        {
            lstrcpy (pstr, szAMPM);
            lstrcat (pstr, szTime);
        }
        else
        {
            lstrcpy (pstr, szTime);
            lstrcat (pstr, szAMPM);
        }
    }
    else
    {
        /*
         *  Icon - only show hours and minutes.
         */
        wsprintf (pstr, TEXT("88%c88"), ClockDisp.szTimeSep[0] );
    }

    /*
     *  Remove trailing space, if necessary.
     */
    ClockDisp.nTimeLen = lstrlen (pstr);
    if (pstr[ClockDisp.nTimeLen - 1] == TEXT(' '))
    {
       ClockDisp.nTimeLen--;
       pstr[ClockDisp.nTimeLen] = TEXT('\0');
    }
}


/*
 *  ClockPaint()
 */

void NEAR PASCAL ClockPaint (HWND hWnd, register HDC hDC, int hint)
{
  int     hour;
  RECT    Rect;
  TIME    nTime;
  xDATE   nDate;
  TCHAR   szTime[6];
  HDC     hTmpDC = NULL;
  HBITMAP hOldBM;
  HFONT   h;

    GetTime (&nTime);
    GetDate (&nDate);

    if ((nDate.day != oDate.day)      ||
        (nDate.month != oDate.month)  ||
        (nDate.year != oDate.year))
    {
        /* new date - so reformat date string */
        FormatDateStr (&nDate, ClockDisp.bIconic);
        ResetWinTitle (hWnd);
        bNewFont = TRUE;
    }

    if (ClockDisp.wFormat == IDM_DIGITAL)
    {
        /*******************/
        /* Digital Display */
        /*******************/

        /* Is the font ready yet? */
        if (!hFont || bNewFont)
        {
            GetClientRect (hWnd, &Rect);
            /* Create a suitable font */
            SizeFont (hWnd, Rect.bottom - Rect.top, Rect.right - Rect.left);
            bNewFont = FALSE;
        }

        if (hFont)
            h = SelectObject (hDC, hFont);
        else
            h = 0;
        SetBkColor (hDC, GetSysColor (COLOR_BTNFACE));
        SetTextAlign (hDC, TA_LEFT);

        if (ClockDisp.wShdwOff && (hTmpDC = CreateCompatibleDC (hDC)))
        {
            /* we will want to print shadowed text. Attempt to allocate
             * the offscreen DC and bitmap for this.
             */
            if (!(hOldBM = SelectObject (hTmpDC, ClockDisp.hBitmap)))
            {
                /* select failed. Perhaps the bitmap was discarded.
                 * So attempt to reallocate a bitmap
                 */
                DeleteObject (ClockDisp.hBitmap);
                ClockDisp.hBitmap = CreateDiscardableBitmap (hDC,
                                                             2*ClockDisp.nSizeChar + 2*ClockDisp.wShdwOff,
                                                             ClockDisp.nSizeY);
                if (!ClockDisp.hBitmap ||
                    !(hOldBM = SelectObject (hTmpDC, ClockDisp.hBitmap)))
                {
                    /* either we couldn't re-allocate the bitmap, or the
                     * select failed again (ie: not because the bitmap
                     * was discarded). Either way, we can't use an offscreen.
                     */
                    DeleteDC (hTmpDC);
                    hTmpDC = NULL;
                }
            }
            if (hTmpDC)
            {
                /* we can use our offscreen. Prepare it for use.
                 */
                if (hFont)
                    SelectObject (hTmpDC, hFont);
                SetBkColor (hTmpDC, GetSysColor (COLOR_BTNFACE));
                SetTextAlign (hTmpDC, TA_LEFT);
            }
        }

        if (hint == REPAINT || ClockDisp.bIconic)
        {
            SelectObject (hDC, hbrColorWindow);
            /* Set old values as undefined, so entire clock updated. */
            oTime.hour24 = 25;
            oTime.minute = 60;
            oTime.ampm   = 2;
            oDate.day = 0;

            /* paint the separators */
            PrintShadowText (hDC,
                             ClockDisp.nPosSep1,
                             ClockDisp.nPosY,
                             ClockDisp.nSizeSep,
                             ClockDisp.nSizeY,
                             &ClockDisp.szTimeSep[0],
                             1,
                             hTmpDC);
            if (!ClockDisp.bIconic && !ClockDisp.bNoSeconds)
                PrintShadowText (hDC,
                                 ClockDisp.nPosSep2,
                                 ClockDisp.nPosY,
                                 ClockDisp.nSizeSep,
                                 ClockDisp.nSizeY,
                                 &ClockDisp.szTimeSep[0],
                                 1,
                                 hTmpDC);
        }

        if (!ClockDisp.bIconic)
        {
            if (ClockDisp.wAMPMPosition)
            {
                /*
                 *  AM/PM - Prefix.
                 */
                if ((oTime.ampm != nTime.ampm) || (oTime.hour24 != nTime.hour24))
                {
                    Rect.left = ClockDisp.nPosAMPM;
                    Rect.top = ClockDisp.nPosY;
                    Rect.right = ClockDisp.nPosHr + ClockDisp.nSizeChar;
                    Rect.bottom = ClockDisp.nPosY + ClockDisp.nSizeY;
                    FillRect (hDC, &Rect, hbrColorWindow);
                }
            }
            else
            {
                /*
                 *  AM/PM - Suffix.
                 */
                if (oTime.ampm != nTime.ampm)
                {
                    PrintShadowText (hDC,
                                     ClockDisp.nPosAMPM,
                                     ClockDisp.nPosY,
                                     ClockDisp.nSizeAMPM,
                                     ClockDisp.nSizeY,
                                     ClockDisp.szAMPM[nTime.ampm],
                                     lstrlen (ClockDisp.szAMPM[nTime.ampm]),
                                     hTmpDC);
                }
            }
        }

        if (!ClockDisp.bIconic && !ClockDisp.bNoSeconds)
        {
            /* paint the seconds */
            szTime[0] = TEXT('0') + nTime.second / 10;
            szTime[1] = TEXT('0') + nTime.second % 10;
            PrintShadowText (hDC,
                             ClockDisp.nPosSec,
                             ClockDisp.nPosY,
                             2 * ClockDisp.nSizeChar,
                             ClockDisp.nSizeY,
                             szTime,
                             2,
                             hTmpDC);
        }

        if (oTime.minute != nTime.minute)
        {
            /* paint the minutes */
            szTime[0]  = TEXT('0') + nTime.minute / 10;
            szTime[1]  = TEXT('0') + nTime.minute % 10;
            PrintShadowText (hDC,
                             ClockDisp.nPosMin,
                             ClockDisp.nPosY,
                             2 * ClockDisp.nSizeChar,
                             ClockDisp.nSizeY,
                             szTime,
                             2,
                             hTmpDC);
        }

        if (oTime.hour24 != nTime.hour24)
        {
            /* paint the hours */
            if (ClockDisp.wTimeFormat)
                hour = nTime.hour24;
            else
                hour = nTime.hour12;

            szTime[0] = TEXT('0') + hour / 10;
            szTime[1] = TEXT('0') + hour % 10;

            /* Kill Leading zero if needed. */
            if (ClockDisp.wTimeLZero == 0 && szTime[0] == TEXT('0'))
            {
                PrintShadowText (hDC,
                                 ClockDisp.nPosHr + ClockDisp.nSizeChar,
                                 ClockDisp.nPosY,
                                 ClockDisp.nSizeChar,
                                 ClockDisp.nSizeY,
                                 szTime + 1,
                                 1,
                                 hTmpDC);
                if ((oTime.hour12 > 9) ||
                    (ClockDisp.wTimeFormat && (oTime.hour24 > 9)))
                {
                    /* if we just switched from 12 to 1 (or 11 to 0),
                     * erase leading 1
                     */
                    Rect.left = ClockDisp.nPosHr;
                    Rect.top = ClockDisp.nPosY;
                    Rect.right = ClockDisp.nPosHr + ClockDisp.nSizeChar;
                    Rect.bottom = ClockDisp.nPosY + ClockDisp.nSizeY;
                    FillRect (hDC, &Rect, hbrColorWindow);
                }
            }
            else
            {
                PrintShadowText (hDC,
                                 ClockDisp.nPosHr,
                                 ClockDisp.nPosY,
                                 2 * ClockDisp.nSizeChar,
                                 ClockDisp.nSizeY,
                                 szTime,
                                 2,
                                 hTmpDC);
            }
        }

        if (ClockDisp.wAMPMPosition && !ClockDisp.bIconic)
        {
            if ((oTime.ampm != nTime.ampm) || (oTime.hour24 != nTime.hour24))
            {
                int hh;

                if (ClockDisp.wTimeFormat)
                    hour = nTime.hour24;
                else
                    hour = nTime.hour12;

                szTime[0] = TEXT('0') + hour / 10;

                /* Kill Leading zero if needed. */
                if (ClockDisp.wTimeLZero == 0 && szTime[0] == TEXT('0'))
                    hh = ClockDisp.nSizeChar;
                else
                    hh = 0;

                PrintShadowText (hDC,
                                 ClockDisp.nPosAMPM + hh,
                                 ClockDisp.nPosY,
                                 ClockDisp.nSizeAMPM,
                                 ClockDisp.nSizeY,
                                 ClockDisp.szAMPM[nTime.ampm],
                                 lstrlen (ClockDisp.szAMPM[nTime.ampm]),
                                 hTmpDC);
            }
        }

        if (hTmpDC)
        {
            SelectObject (hDC, hOldBM);
            DeleteDC (hTmpDC);
            hTmpDC = NULL;
        }

        if ((oDate.day != nDate.day     ||
             oDate.month != nDate.month ||
             oDate.year != nDate.year) && bDisplayDate)
        {
            /* paint the date - first erase old string, 'cause new string
             * may be shorter. Problem: we no longer know the extent of
             * the old string, so paint the whole x-extent of the clock
             * window. Don't worry, this doesn't happen too often. */
            GetClientRect (hWnd, &Rect);
            Rect.top = ClockDisp.nPosDateY;
            Rect.bottom = ClockDisp.nPosDateY + ClockDisp.nSizeDateY;
            FillRect (hDC, &Rect, hbrColorWindow);

            if (hFontDate)
                h = SelectObject (hDC, hFontDate);
            else
                h = 0;
            PrintShadowText (hDC,
                             ClockDisp.nPosDateX,
                             ClockDisp.nPosDateY,
                             ClockDisp.nSizeDateX,
                             ClockDisp.nSizeDateY,
                             ClockDisp.szDate,
                             ClockDisp.nDateLen,
                             hTmpDC);
        }

        if (h)
            SelectObject(hDC, h);
    }
    else
    {
        /******************/
        /* Analog display */
        /******************/

        if (hint == REPAINT)
        {
            SetBkMode (hDC, TRANSPARENT);

            DrawFace (hDC);
            if (!ClockDisp.bIconic)
            {
                DrawFatHand (hDC, oTime.hour * 5 + (oTime.minute / 12), hpenForeground, HHAND);
                DrawFatHand (hDC, oTime.minute, hpenForeground, MHAND);
            }
            else
            {
                DrawHand (hDC, oTime.hour * 5 + (oTime.minute / 12), hpenForeground, HOURSCALE, R2_COPYPEN);
                DrawHand (hDC, oTime.minute, hpenForeground, MINUTESCALE, R2_COPYPEN);
            }

            if (!ClockDisp.bIconic && !ClockDisp.bNoSeconds)
                /* Draw the second hand. */
                DrawHand (hDC, oTime.second, hpenBackground, SECONDSCALE, R2_NOT);

            /* NOTE: Don't update oTime in this case! */

            if (ClockDisp.bIconic)
                DrawIconBorder (hWnd, hDC);
            return;
        }
        else if (hint == HANDPAINT)
        {
            SetBkMode(hDC, TRANSPARENT);
            if ((!ClockDisp.bIconic && !ClockDisp.bNoSeconds) && nTime.second != oTime.second)
                /* Erase the old second hand. */
                DrawHand (hDC, oTime.second, hpenBackground, SECONDSCALE, R2_NOT);

            if (nTime.minute != oTime.minute || nTime.hour != oTime.hour)
            {
                if (ClockDisp.bIconic)
                {
                    DrawHand (hDC, oTime.minute, hpenBackground, MINUTESCALE, R2_COPYPEN);
                    DrawHand (hDC, oTime.hour * 5 + (oTime.minute / 12), hpenBackground, HOURSCALE, R2_COPYPEN);
                    DrawHand (hDC, nTime.minute, hpenForeground, MINUTESCALE, R2_COPYPEN);
                    DrawHand (hDC, nTime.hour * 5 + (nTime.minute / 12), hpenForeground, HOURSCALE, R2_COPYPEN);
                }
                else
                {
                    DrawFatHand (hDC, oTime.hour * 5 + (oTime.minute/12), hpenBackground, HHAND);
                    DrawFatHand (hDC, oTime.minute, hpenBackground, MHAND);

                    DrawFatHand (hDC, (nTime.hour) * 5 + (nTime.minute / 12), hpenForeground, HHAND);
                    DrawFatHand (hDC, nTime.minute, hpenForeground, MHAND);
                }
            }

            if ((!ClockDisp.bIconic && !ClockDisp.bNoSeconds) && nTime.second != oTime.second)
                /* Draw new second hand */
                DrawHand (hDC, nTime.second, hpenBackground, SECONDSCALE, R2_NOT);
        }

    }

    if (ClockDisp.bIconic)
        DrawIconBorder (hWnd, hDC);

    oTime = nTime;
    oDate = nDate;
}

void Adjust (POINT * rgpt, INT cPoint, INT iDelta)
{
   INT i;

     for (i = 0; i < cPoint; i++)
     {
        rgpt[i].x += iDelta;
        rgpt[i].y += iDelta;
     }
}


/*
 *  DrawFatHand() - Draws either hour or minute hand.
 */

void NEAR PASCAL DrawFatHand (register HDC hDC, int pos, HPEN hPen, BOOL hHand)
{
  register int  m;
  int           n;
  int           scale;

  TRIG     tip;
  TRIG     stip;
  BOOL     fErase;

    SetROP2 (hDC, 13);
    fErase = (hPen == hpenBackground);

    SelectObject (hDC, hPen);

    scale = hHand ? 7 : 5;

    n = (pos + 15) % 60;
    m = MulDiv (clockRadius, scale, 100);
    stip.y = (SHORT) MulDiv (lpcirTab[n].y, m, 8000);
    stip.x = (SHORT) MulDiv (lpcirTab[n].x, m, 8000);

    scale = hHand ? 65 : 80;
    tip.y = (SHORT) MulDiv (lpcirTab[pos % 60].y, MulDiv (clockRadius, scale, 100), 8000);
    tip.x = (SHORT) MulDiv (lpcirTab[pos % 60].x, MulDiv (clockRadius, scale, 100), 8000);
    {
      POINT  rgpt[4];
      HBRUSH hbrInit;

        rgpt[0].x = clockCenter.x + stip.x;
        rgpt[0].y = clockCenter.y + stip.y;
        rgpt[1].x = clockCenter.x + tip.x;
        rgpt[1].y = clockCenter.y + tip.y;
        rgpt[2].x = clockCenter.x - stip.x;
        rgpt[2].y = clockCenter.y - stip.y;

        scale = hHand ? 15 : 20;

        n = (pos + 30) % 60;
        m = MulDiv (clockRadius, scale, 100);
        tip.y = (SHORT) MulDiv (lpcirTab[n].y, m, 8000);
        tip.x = (SHORT) MulDiv (lpcirTab[n].x, m, 8000);

        rgpt[3].x = clockCenter.x + tip.x;
        rgpt[3].y = clockCenter.y + tip.y;

        SelectObject (hDC, GetStockObject (NULL_PEN));
        if (fErase)
        {
            hbrInit = SelectObject (hDC, hbrColorWindow);
        }
        if (fShadedHands)
        {
            Adjust (rgpt, 4, -2);
            if (!fErase)
            {
               hbrInit = SelectObject (hDC, hbrBtnHighlight);
            }
            Polygon(hDC, rgpt, 4);

            if (!fErase)
            {
               SelectObject (hDC, hbrForeground);
            }
            Adjust (rgpt, 4, 4);
            Polygon (hDC, rgpt, 4);

            Adjust (rgpt, 4, -2);
        }
        if (!fErase)
        {
           SelectObject (hDC, hbrBlobColor);
        }
        Polygon (hDC, rgpt, 4);
        /*
         * if we selected a brush in, reset it now.
         */
        if (fErase || fShadedHands) {
           SelectObject(hDC, hbrInit);
        }
    }
}


/*
 *  DrawFace()
 */

void NEAR PASCAL DrawFace(HDC hDC)
{
  int      i;
  RECT     tRect;
  TRIG *   ppt;
  int      blobHeight, blobWidth;

    blobWidth = MulDiv (MAXBLOBWIDTH, (clockRect.right - clockRect.left), HorzRes);
    blobHeight = MulDiv (blobWidth, aspectN, aspectD);

    if (blobHeight < 2)
        blobHeight = 1;

    if (blobWidth < 2)
        blobWidth = 2;

    InflateRect (&clockRect, -(blobHeight >> 1), -(blobWidth >> 1));


    clockRadius = (int) ((clockRect.right - clockRect.left-8) >> 1);
    clockCenter.y = (SHORT) (clockRect.top + ((clockRect.bottom - clockRect.top) >> 1) - 1);
    clockCenter.x = (SHORT) (clockRect.left + clockRadius+3);

    for (i = 0; i < 60; i++)
    {
        ppt = lpcirTab + i;

        tRect.top  = MulDiv (ppt->y, clockRadius, 8000) + clockCenter.y;
        tRect.left = MulDiv (ppt->x, clockRadius, 8000) + clockCenter.x;

        fShadedHands = FALSE;

        if (i % 5)
        {
            /* Draw a dot. */
            if (blobWidth > 2 && blobHeight >= 2)
            {
                fShadedHands = TRUE;
                tRect.right = tRect.left + 2;
                tRect.bottom = tRect.top + 2;
                FillRect (hDC, &tRect, GetStockObject (WHITE_BRUSH));
                OffsetRect (&tRect, -1, -1);
                FillRect (hDC, &tRect, hbrForeground);
                tRect.left++;
                tRect.top++;
                FillRect (hDC, &tRect, hbrColorWindow);
            }
        }
        else if (!ClockDisp.bIconic)
        {
            tRect.right = tRect.left + blobWidth;
            tRect.bottom = tRect.top + blobHeight;
            OffsetRect (&tRect, -(blobWidth >> 1) , -(blobHeight >> 1));

            SelectObject (hDC, GetStockObject (BLACK_PEN));
            SelectObject (hDC, hbrBlobColor);

            Rectangle (hDC, tRect.left, tRect.top, tRect.right, tRect.bottom);
            SelectObject (hDC, hpenBlobHlt);
            MoveTo (hDC, tRect.left, tRect.bottom-1);
            LineTo (hDC, tRect.left, tRect.top);
            LineTo (hDC, tRect.right-1, tRect.top);
        }
        else
        {
            PatBlt (hDC, tRect.left, tRect.top, 2, 2, BLACKNESS);
            PatBlt (hDC, tRect.left, tRect.top, 1, 1, WHITENESS);
        }
    }
    InflateRect (&clockRect, (blobHeight >> 1), (blobWidth >> 1));
}

/*
 *  DrawIconBorder() - Draws a Border around either icon-clock.
 */

void NEAR PASCAL DrawIconBorder (HWND hWnd, register HDC hDC)
{
  RECT Rect;

    /* draws a "sunk-in" border, ie: double black outline top and left,
     * single white outline bottom and right.
     */
    GetClientRect (hWnd, &Rect);

    SelectObject (hDC, GetStockObject (BLACK_PEN));
    MoveTo (hDC, Rect.left, Rect.top + 1);
    LineTo (hDC, Rect.left, Rect.bottom - 1);
    MoveTo (hDC, Rect.left+1, Rect.bottom - 1);
    LineTo (hDC, Rect.right - 1, Rect.bottom - 1);
    MoveTo (hDC, Rect.right - 1, Rect.bottom - 2);
    LineTo (hDC, Rect.right - 1, Rect.top);
    MoveTo (hDC, Rect.right - 2, Rect.top);
    LineTo (hDC, Rect.left, Rect.top);

    MoveTo (hDC, Rect.left + 2, Rect.top + 2);
    LineTo (hDC, Rect.left + 2, Rect.bottom - 3);
    LineTo (hDC, Rect.right - 3, Rect.bottom - 3);
    LineTo (hDC, Rect.right - 3, Rect.top + 2);
    LineTo (hDC, Rect.left + 2, Rect.top + 2);

    SelectObject (hDC, hpenRed);
    MoveTo (hDC, Rect.left + 1, Rect.top + 1);
    LineTo (hDC, Rect.left + 1, Rect.bottom - 2);
    LineTo (hDC, Rect.right - 2, Rect.bottom - 2);
    LineTo (hDC, Rect.right - 2, Rect.top + 1);
    LineTo (hDC, Rect.left + 1, Rect.top + 1);
}


/*
 * FormatDateStr() - prepare the formatted date string
 *
 * parameters:
 *  xDATE *pDate - structure contains current date
 *  BOOL bRealShort - if TRUE, the year is not added to the string (ie:
 *                    only use the day and the month)
 *
 * This will format the given date according to the format specified
 * by the current Locale, and place the result in ClockDisp.szDate.
 */

void NEAR PASCAL FormatDateStr (xDATE * pDate, BOOL bRealShort)
{
  register int i = 0, j = 0;
  BOOL         bLead;
  TCHAR        cSep;

    while (ClockDisp.szDateFmt[i] && (j < MAX_DATE_LEN - 1))
    {
        bLead = FALSE;
        switch (cSep = ClockDisp.szDateFmt[i++])
        {
            case TEXT('d'):
                while (ClockDisp.szDateFmt[i] == TEXT('d'))
                {
                    bLead = TRUE;
                    i++;
                }
                if (bLead || (pDate->day / 10))
                    ClockDisp.szDate[j++] = TEXT('0') + pDate->day / 10;
                ClockDisp.szDate[j++] = TEXT('0') + pDate->day % 10;
                break;

            case TEXT('M'):
                while (ClockDisp.szDateFmt[i] == TEXT('M'))
                {
                    bLead = TRUE;
                    i++;
                }
                if (bLead || (pDate->month / 10))
                    ClockDisp.szDate[j++] = TEXT('0') + pDate->month / 10;
                ClockDisp.szDate[j++] = TEXT('0') + pDate->month % 10;
                break;

            case TEXT('y'):
                i++;
                if (ClockDisp.szDateFmt[i] == TEXT('y'))
                {
                    bLead = TRUE;
                    i+=2;
                }
                if (bLead && !bRealShort)
                {
                    ClockDisp.szDate[j++] = (pDate->year < 2000 ? TEXT('1') : TEXT('2'));
                    ClockDisp.szDate[j++] = (pDate->year < 2000 ? TEXT('9') : TEXT('0'));
                }
                if (!bRealShort)
                {
                    ClockDisp.szDate[j++] = TEXT('0') + (pDate->year % 100) / 10;
                    ClockDisp.szDate[j++] = TEXT('0') + (pDate->year % 100) % 10;
                }
                break;

            default:
                /* copy the current character into the formatted string - it
                 * is a separator. BUT: don't copy a separator into the
                 * very first position (could happen if the year comes first,
                 * but we're not using the year)
                 */
                if (j)
                    ClockDisp.szDate[j++] = cSep;
                break;
        }
    }
    while ((ClockDisp.szDate[j-1] < TEXT('0')) || (ClockDisp.szDate[j-1] > TEXT('9')))
        j--;
    ClockDisp.szDate[j] = TEXT('\0');
    ClockDisp.nDateLen = j;
}


/*
 *  PrintShadowText()
 *
 *  Parameters:
 *      HDC hdc - the window DC
 *      int nx, ny, sizex, sizey - position and extent of bounding box
 *      PSTR pszStr - actual string to print
 *      int nStrLen - length of pszStr
 *      HDC hOffScrnDC - an offscreen DC used to paint the shadowed text.
 *                       If this is NULL, the text is not shadowed.
 *                       Assumes the bitmap associated with this DC can
 *                       contain 2 digits. If the specified bounding box
 *                       does not fit in the bitmap, the shadowed text is
 *                       painted directly to the window DC (may cause flashing)
 *
 *  Shadowed text is: a white highlight top and left, dark highlight bottom
 *  and right. The character itself is painted the same color as the background
 *  (so only noticeable if both highlights are noticeable).
 */


void NEAR PASCAL PrintShadowText (register HDC hDC, int nx, int ny,
                                  int sizex, int sizey, LPTSTR pszStr,
                                  int nStrLen, HDC hOffScrnDC)
{
    RECT    Rect;
    register WORD    Shadow;

    if (!hOffScrnDC || fNoShadow)
    {
        /* If no valid offscreen DC is provided, then we can NOT print
         * shadowed text. Simply print the normal font.
         */
        SetTextColor (hDC, GetSysColor (COLOR_BTNTEXT));

        Rect.top = ny;
        Rect.bottom = ny + sizey;
        Rect.left = nx;
        Rect.right = nx + sizex;
        ExtTextOut (hDC, nx, ny, ETO_OPAQUE | ETO_CLIPPED, &Rect,
                    pszStr, nStrLen, NULL);
    }
    else if (sizex > 2 * ClockDisp.nSizeChar)
    {
        /* print the shadowed text (but don't use offscreen bitmap because
         * the string is too long to fit)
         */
        Shadow = ClockDisp.wShdwOff;
        SetTextColor (hDC, GetSysColor (COLOR_BTNHIGHLIGHT));

        Rect.top = ny - Shadow;
        Rect.bottom = ny + sizey + Shadow;
        Rect.left = nx - Shadow;
        Rect.right = nx + sizex + Shadow;

        ExtTextOut (hDC, nx - Shadow, ny - Shadow,
                    ETO_OPAQUE | ETO_CLIPPED, &Rect, pszStr, nStrLen, NULL);

        SetBkMode (hDC, TRANSPARENT);
        SetTextColor (hDC, GetSysColor (COLOR_BTNSHADOW));
        ExtTextOut (hDC, nx + Shadow, ny + Shadow,
                    ETO_CLIPPED, &Rect, pszStr, nStrLen, NULL);

        SetTextColor (hDC, GetSysColor (COLOR_BTNFACE));
        ExtTextOut (hDC, nx, ny, ETO_CLIPPED, &Rect, pszStr, nStrLen, NULL);
    }
    else
    {
        /* use the off-screen bitmap, and print shadowed text. */
        Shadow = ClockDisp.wShdwOff;

        Rect.left = 0;
        Rect.right = sizex + 2* Shadow;
        Rect.top = 0;
        Rect.bottom = sizey + 2* Shadow;

        SetTextColor (hOffScrnDC, GetSysColor (COLOR_BTNHIGHLIGHT));
        ExtTextOut (hOffScrnDC, 0, 0, ETO_OPAQUE | ETO_CLIPPED, &Rect, pszStr,
                    nStrLen, NULL);
        SetBkMode (hOffScrnDC, TRANSPARENT);
        SetTextColor (hOffScrnDC, GetSysColor (COLOR_BTNSHADOW));
        ExtTextOut (hOffScrnDC, 2 * Shadow, 2* Shadow,
                    ETO_CLIPPED, &Rect, pszStr, nStrLen, NULL);

        SetTextColor (hOffScrnDC, GetSysColor (COLOR_BTNFACE));
        ExtTextOut (hOffScrnDC, Shadow, Shadow,
                    ETO_CLIPPED, &Rect, pszStr, nStrLen, NULL);

        BitBlt (hDC, nx-Shadow, ny-Shadow,
                Rect.right, Rect.bottom, hOffScrnDC, 0, 0, SRCCOPY);
    }
}

/*
 * SizeFont() - size font according to window size
 *
 * Only useful in digital mode.
 * Create a font that will fit the current time format (based on the
 * time template). Center the string, set up the positions for the
 * different components of the time string.
 * Do the same for the date, unless that option is disabled.
 * 2 results:
 *  fonts are created. The font handles are stored in global variables
 *  string is positioned. Results are stored in the ClockDisp structure
 *    (ie: starting positions and extents)
 */

void NEAR PASCAL SizeFont(HWND hWnd, int newHeight, int newWidth)
{
  register HDC    hDC;
  SIZE       sizeTimeExt;
  SIZE       sizeDateExt;

  int        nzExts[ 15 ];
#ifdef JAPAN    // #1208:6/1/93:
  int        bFit;
  LOGFONT    wkFontStruct;
#endif
  int        nOldFit, nFit, i;
  int        nAMPMIndx = 0;
  int        iAMPMLen;
  int        DesiredWidth;
  int        DesiredHeight;
  int        InitialHeight;
  HCURSOR    hOldCur;
  HFONT h;

    if (ClockDisp.wFormat == IDM_DIGITAL)
    {
        hOldCur = SetCursor (hCurWait);

        hDC = GetDC(hWnd);
        if (hFont != NULL)
            DeleteObject (hFont);

        if (ClockDisp.bIconic)  /* Adjust for fat border */
        {
            newWidth -= 4;
            newHeight -= 4;
        }

        /* use up 7/8 of the x-extent of the window */
        DesiredWidth = (newWidth * 7) / 8;

        if (bDisplayDate)
            /* display the date with the time - time only occupied a third
             * of the screen height
             */
            DesiredHeight = 3 * newHeight / 8;
        else
            /* no date - time occupies two-thirds the screen height */
            DesiredHeight = 7 * newHeight / 8;

        if (ClockDisp.bIconic)  /* Readjust for fat border */
        {
            newWidth += 4;
            newHeight += 4;
        }

        /* create initial font real big, for more precision in calculations.
         * This will give us an extent for out string, we can then figure
         * out the correct font height to fit the string (using a ratio).
         */
        InitialHeight = 1000;

        FontStruct.lfWidth = 0;                 /* don't stretch fonts */
        FontStruct.lfHeight = -InitialHeight;

#ifdef JAPAN
        /* In Win 3.0 if the following were not done some unexpected font
         * maybe selected in special cases.  Especially in Japan with
         * vertical writing.
         */
        FontStruct.lfEscapement  = 0;
        FontStruct.lfOrientation = 0;
#endif

#if defined(JAPAN) || defined(KOREA)
        /* We can use only NATIVE_CHARSET */
        if (FontStruct.lfCharSet != NATIVE_CHARSET)
        {
            FontStruct.lfCharSet = NATIVE_CHARSET;
            lstrcpy (FontStruct.lfFaceName, TEXT("System"));
        }
#endif

        hFont = CreateFontIndirect(&FontStruct);
        if (hFont)
            h = SelectObject(hDC, hFont);
        else
            h = 0;

        GetTextExtentPoint (hDC, ClockDisp.szTimeFmt, ClockDisp.nTimeLen, &sizeTimeExt);
        if (bDisplayDate)
            GetTextExtentPoint (hDC, ClockDisp.szDate, ClockDisp.nDateLen, &sizeDateExt);

        if (h)
            SelectObject(hDC, h);

        /* compute appropriate font for time string:
         * establish a ratio current size to required size.
         * This may yield a font too tall to fit the window, so limit
         * the font height.
         */
        FontStruct.lfHeight = -MulDiv (InitialHeight, DesiredWidth,
                                       sizeTimeExt.cx);
        if (-FontStruct.lfHeight > DesiredHeight)
            FontStruct.lfHeight = -DesiredHeight;


        // pre-initialize nzExts in case this for loop doesn't execute
        // (in really small cy cases).
        for (i = 0; i < 15; nzExts[i] = 0, i++);
#ifdef JAPAN    // #1208:6/1/93:
        bFit = FALSE;
        wkFontStruct = FontStruct;
#endif

        for (nOldFit = 0, nFit = -1; FontStruct.lfHeight < 0;)
        {
            nOldFit = nFit;

            /* little loop to take care of round-off errors, which may
             * cause our initial "guess" to be slightly too large. So
             * check that the string fits (in most cases it will),
             * otherwise, re-create the font in a slightly smaller size.
             */
            if (hFont)
                DeleteObject (hFont);
            hFont = CreateFontIndirect(&FontStruct);

            if (hFont)
                h = SelectObject(hDC, hFont);
            else
                h = 0;

            GetTextExtentExPoint (hDC, ClockDisp.szTimeFmt, ClockDisp.nTimeLen,
                                  DesiredWidth, &nFit, nzExts, &sizeTimeExt);
            if (h)
                SelectObject(hDC, h);

            if (nFit == ClockDisp.nTimeLen) /* if string fits, exit loop */
#ifdef JAPAN    // #1208:6/1/93:
            {
                bFit = TRUE;
                break;
            }
#else
               break;
#endif

            FontStruct.lfHeight += 2;       /* remember height is negative! */
        }

#ifdef JAPAN    // #1208:6/1/93: Change display font fit iconic
        if (!bFit && ClockDisp.bIconic)
        {
            wkFontStruct.lfPitchAndFamily = FF_SWISS | VARIABLE_PITCH;
            lstrcpy (wkFontStruct.lfFaceName, TEXT(" "));
            for (nOldFit = 0, nFit = -1; wkFontStruct.lfHeight < 0;)
            {
                nOldFit = nFit;

                if (hFont)
                    DeleteObject (hFont);
                hFont = CreateFontIndirect(&wkFontStruct);

                if (hFont)
                    h = SelectObject(hDC, hFont);
                else
                    h = 0;

                GetTextExtentExPoint( hDC, ClockDisp.szTimeFmt,
                ClockDisp.nTimeLen, DesiredWidth, &nFit, nzExts, &sizeTimeExt );
                if (h)
                    SelectObject(hDC, h);

                if (nFit == ClockDisp.nTimeLen)
                    break;

                wkFontStruct.lfHeight += 2;
            }
        }
#endif


        /* compute placement and extents */
        iAMPMLen = ClockDisp.nMaxAMPMLen;
        if (!ClockDisp.bIconic && ClockDisp.wAMPMPosition && iAMPMLen)
        {
            ClockDisp.nSizeChar = nzExts[iAMPMLen] - nzExts[iAMPMLen-1];
            ClockDisp.nPosAMPM = (newWidth - sizeTimeExt.cx) / 2;
            ClockDisp.nSizeAMPM = nzExts[iAMPMLen-2];
            ClockDisp.nSizeSep = nzExts[iAMPMLen+2] - nzExts[iAMPMLen+1];
            ClockDisp.nPosHr = ClockDisp.nPosAMPM + nzExts[iAMPMLen-1];
            ClockDisp.nPosSep1 = ClockDisp.nPosAMPM + nzExts[iAMPMLen+1];
            ClockDisp.nPosMin = ClockDisp.nPosAMPM + nzExts[iAMPMLen+2];
            if (!ClockDisp.bNoSeconds)
            {
                ClockDisp.nPosSep2 = ClockDisp.nPosAMPM + nzExts[iAMPMLen+4];
                ClockDisp.nPosSec = ClockDisp.nPosAMPM + nzExts[iAMPMLen+5];
            }
        }
        else
        {
            ClockDisp.nSizeChar = nzExts[0];
            ClockDisp.nSizeSep = nzExts[2] - nzExts[1];
            ClockDisp.nPosHr = (newWidth - sizeTimeExt.cx) / 2;
            ClockDisp.nPosSep1 = ClockDisp.nPosHr + nzExts[ 1 ];
            ClockDisp.nPosMin = ClockDisp.nPosHr + nzExts[2];
            if (!ClockDisp.bIconic)
            {
                if (!ClockDisp.bNoSeconds)
                {
                    ClockDisp.nPosSep2 = ClockDisp.nPosHr + nzExts[4];
                    ClockDisp.nPosSec = ClockDisp.nPosHr + nzExts[5];
                    if (ClockDisp.nTimeLen > 8)
                        nAMPMIndx = 8;
                }
                else if (ClockDisp.nTimeLen > 5)
                    nAMPMIndx = 5;
                if (nAMPMIndx)
                {
                    ClockDisp.nPosAMPM = ClockDisp.nPosHr + nzExts[nAMPMIndx];
                    ClockDisp.nSizeAMPM = sizeTimeExt.cx - nzExts[nAMPMIndx];
                }
                else
                {
                    ClockDisp.nPosAMPM = 0;
                    ClockDisp.nSizeAMPM = 0;
                }
            }
        }

        ClockDisp.nSizeY  = sizeTimeExt.cy;

        /* compute size of shadow offset - if the font is too small, no
         * shadow (offset = 0)
         */

        ClockDisp.wShdwOff = (WORD) (((ClockDisp.nSizeChar + ClockDisp.nSizeY) < 90) ?
                                0 : 2 * GetSystemMetrics (SM_CXBORDER));

        /* allocate a bitmap for 2 digits */
        if (ClockDisp.hBitmap != NULL)
            DeleteObject (ClockDisp.hBitmap);

        ClockDisp.hBitmap = CreateDiscardableBitmap (hDC, 2*ClockDisp.nSizeChar
                                                     + 2*ClockDisp.wShdwOff, ClockDisp.nSizeY);

        /* prepare a font for the date */
        if (bDisplayDate)
        {
            DesiredHeight = -(FontStruct.lfHeight * 3) / 4;

            /* compute appropriate font for date - same algorithm as time */
            FontStruct.lfHeight = -MulDiv (InitialHeight, DesiredWidth,
                sizeDateExt.cx);
            if (-FontStruct.lfHeight > DesiredHeight)
                FontStruct.lfHeight = -DesiredHeight;

            for (; ;)
            {
                if (hFontDate != NULL)
                    DeleteObject (hFontDate);
                hFontDate = CreateFontIndirect (&FontStruct);

                if (hFontDate)
                    h = SelectObject (hDC, hFontDate);
                else
                    h = 0;
                GetTextExtentPoint (hDC, ClockDisp.szDate,
                    ClockDisp.nDateLen, &sizeDateExt);
                if (h)
                    SelectObject (hDC, h);
                if (sizeDateExt.cx < DesiredWidth)
                   break;
                FontStruct.lfHeight += 2;
            }

            /* compute date placement and extents */
            ClockDisp.nPosDateX = (newWidth - sizeDateExt.cx) / 2;
            ClockDisp.nSizeDateX = sizeDateExt.cx;
            ClockDisp.nSizeDateY = sizeDateExt.cy;

            ClockDisp.nPosY = (newHeight - (ClockDisp.nSizeY +
                               ClockDisp.nSizeDateY)) / 2;
            ClockDisp.nPosDateY = ClockDisp.nPosY + ClockDisp.nSizeY;
        }
        else
        {
            /* no date, so center the time */
            ClockDisp.nPosY = (newHeight - ClockDisp.nSizeY) / 2;
        }
        ReleaseDC (hWnd, hDC);

        SetCursor (hOldCur);
    }
}


/*
 *  DrawHand() - Draw the second hand using XOR mode.
 */

void NEAR PASCAL DrawHand (register HDC hDC,
                           int          pos,
                           HPEN         hPen,
                           int          scale,
                           int          patMode)
{
  TRIG *   lppt;
  int      radius;

    MoveTo (hDC, clockCenter.x, clockCenter.y);
    radius = MulDiv (clockRadius, scale, 100);
    lppt = lpcirTab + (pos % 60);
    SetROP2 (hDC, patMode);
    SelectObject (hDC, hPen);

    LineTo (hDC, clockCenter.x + MulDiv (lppt->x, radius, 8000),
            clockCenter.y + MulDiv (lppt->y, radius, 8000));
}


/*
 *  SetMenuBar() - places or removes the menu bar, etc.
 *
 *  Based on the flags in ClockDisp structure (ie: do we want a menu/title
 *  bar or not?), adds or removes the window title and menu bar:
 *    Gets current style, toggles the bits, and re-sets the style.
 *    Must then resize the window frame and show it.
 */

void NEAR PASCAL SetMenuBar (HWND hWnd)
{
    static DWORD  wID;
    DWORD         dwStyle;

    dwStyle = GetWindowLong (hWnd, GWL_STYLE);
    if (ClockDisp.bNoTitle)
    {
        /* remove caption & menu bar, etc. */
        dwStyle &= ~(WS_DLGFRAME | WS_SYSMENU |
                   WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
        wID = SetWindowLong (hWnd, GWL_ID, 0);

    }
    else
    {
        /* put menu bar & caption back in */
        dwStyle = WS_TILEDWINDOW | dwStyle;
        SetWindowLong (hWnd, GWL_ID, wID);
        SetWindowRgn(hWnd, NULL, TRUE);
    }
    SetWindowLong (hWnd, GWL_STYLE, dwStyle);
    SetWindowPos (hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE |
                  SWP_NOZORDER | SWP_FRAMECHANGED);

    if (ClockDisp.wFormat == IDM_ANALOG && ClockDisp.bNoTitle)
    {
        HRGN hrgn;
        RECT rc;

        GetClientRect(hWnd,&rc);

        if(hrgn = CreateEllipticWndRgn(hWnd,&rc))
            SetWindowRgn(hWnd,hrgn,TRUE);
    }

    ShowWindow (hWnd, SW_SHOW);
}


/*
 *  FormatInit() -  Retrieve current locale information.
 */

void NEAR PASCAL FormatInit (VOID)
{
    LCID   lcid;
    TCHAR  szBuf[3];


    lcid = GetUserDefaultLCID ();

    /*
     *  Get time format:
     *      0 = 12 hour format
     *      1 = 24 hour format
     */
    if (!bUtc)
    {
        GetLocaleInfoW (lcid, LOCALE_ITIME, (LPWSTR) szBuf, 3);
        ClockDisp.wTimeFormat = (WORD)MyAtoi (szBuf);
    }
    else
    {
        /*
         *  GMT - use 24 hour clock format.
         */
        ClockDisp.wTimeFormat = 1;
    }

    /*
     *  Get time leading zero:
     *      0 = no leading zero
     *      1 = leading zero
     */
    GetLocaleInfoW (lcid, LOCALE_ITLZERO, (LPWSTR) szBuf, 3);
    ClockDisp.wTimeLZero = (WORD)MyAtoi (szBuf);

    /*
     *  Get time marker position:
     *      0 = suffix
     *      1 = prefix
     */
    GetLocaleInfoW (lcid, LOCALE_ITIMEMARKPOSN, (LPWSTR) szBuf, 3);
    ClockDisp.wAMPMPosition = (WORD)MyAtoi (szBuf);

    /*
     *  Get AM/PM designators.
     */
    if (!bUtc)
    {
        GetLocaleInfoW (lcid, LOCALE_S1159, (LPWSTR) ClockDisp.szAMPM[0], MAX_AMPM_LEN);
        GetLocaleInfoW (lcid, LOCALE_S2359, (LPWSTR) ClockDisp.szAMPM[1], MAX_AMPM_LEN);

        ClockDisp.nMaxAMPMLen = max( lstrlen(ClockDisp.szAMPM[0]),
                                     lstrlen(ClockDisp.szAMPM[1]) );
        if ((ClockDisp.wAMPMPosition) && (ClockDisp.nMaxAMPMLen))
        {
            /*
             *  AM/PM is a Prefix, so need to add one to the length
             *  for the space between the time marker and the time string.
             */
            (ClockDisp.nMaxAMPMLen)++;
        }
    }
    else
    {
        *(ClockDisp.szAMPM[0]) = *(ClockDisp.szAMPM[1]) = TEXT('\0');
        ClockDisp.nMaxAMPMLen = 0;
    }

    /*
     *  Get time separator string.
     */
    GetLocaleInfoW (lcid, LOCALE_STIME, (LPWSTR) ClockDisp.szTimeSep, MAX_TIME_SEP);

    /*
     *  Get short date format.
     */
    GetLocaleInfoW (lcid, LOCALE_SSHORTDATE, (LPWSTR) ClockDisp.szDateFmt, MAX_DATE_LEN);
}


/*
 *  ClockSize()
 */

void NEAR PASCAL ClockSize (register HWND hWnd,
                            int           newWidth,
                            int           newHeight,
                            WORD          SizeWord)
{
  BOOL    bChanged = FALSE;

    SetRect (&clockRect, 0, 0, newWidth, newHeight);
    CompClockDim ();

    if (SizeWord == SIZEICONIC)
    {
        ClockDisp.bIconic = TRUE;
        bChanged = TRUE;
    }
    else if (ClockDisp.bIconic)
    {
        ClockDisp.bIconic = FALSE;
        bChanged = TRUE;
    }

    if (bChanged)
    {
        ClockTimerInterval (hWnd);
        FormatTimeStr ();
        if (!ClockDisp.bNoDate)
        {
            FormatDateStr (&oDate, ClockDisp.bIconic);
            ResetWinTitle (hWnd);    /* date has changed */
        }
    }

    if (ClockDisp.wFormat == IDM_ANALOG && ClockDisp.bNoTitle)
    {
        HRGN hrgn;
        RECT rc;

        GetClientRect(hWnd,&rc);

        if(hrgn = CreateEllipticWndRgn(hWnd,&rc))
            SetWindowRgn(hWnd,hrgn,TRUE);
    }
}


/*
 *  CompClockDim() - Recompute the clock's dimensions.
 */

void NEAR PASCAL CompClockDim (void)

{
  int             i;
  register int    tWidth;
  register int    tHeight;

    tWidth = clockRect.right - clockRect.left;
    tHeight = clockRect.bottom - clockRect.top;

    if (tWidth > MulDiv (tHeight,aspectD,aspectN))
    {
        i = MulDiv (tHeight, aspectD, aspectN);
        clockRect.left += (tWidth - i) >> 1;
        clockRect.right = clockRect.left + i;
    }
    else
    {
        i = MulDiv (tWidth, aspectN, aspectD);
        clockRect.top += (tHeight - i) >> 1;
        clockRect.bottom = clockRect.top + i;
    }
}


/*
 *  DeleteTools()
 */

void NEAR PASCAL DeleteTools (void)
{
    DeleteObject (hbrForeground);
    DeleteObject (hbrColorWindow);
    DeleteObject (hbrBtnHighlight);
    DeleteObject (hbrBlobColor);
    DeleteObject (hpenForeground);
    DeleteObject (hpenShadow);
    DeleteObject (hpenBackground);
    DeleteObject (hpenBlobHlt);
    DeleteObject (hpenRed);
}


void ParseSavedWindow (LPTSTR szBuf, PRECT pRect)
{
  PINT  pint;
  int   count;

    short cxFrame  = (short) GetSystemMetrics (SM_CXFRAME);
    short cxSize   = (short) GetSystemMetrics (SM_CXSIZE);
    short cyFrame  = (short) GetSystemMetrics (SM_CYFRAME);
    short cySize   = (short) GetSystemMetrics (SM_CYSIZE);

    count = 0;
    pint = (PINT) pRect;

    while (*szBuf && count < 4)
    {
        *pint = (int) MyAtoi (szBuf);
        pint++;         // advance to next field

        while (*szBuf && *szBuf != TEXT(','))
            szBuf++;

        while (*szBuf && *szBuf == TEXT(','))
            szBuf++;

        count++;
    }
    if ((count < 4) ||
        (pRect->left >= pRect->right) || (pRect->top >= pRect->bottom))
    {
        HDC hDC = GetDC(NULL);
        int nPixMMX = GetDeviceCaps (hDC, HORZRES) / GetDeviceCaps (hDC, HORZSIZE);
        int nPixMMY = GetDeviceCaps (hDC, VERTRES) / GetDeviceCaps (hDC, VERTSIZE);
        ReleaseDC (NULL, hDC);

/* Bug #14014:  These sizes chosen for showing date in title bar as well as
 * instruction speed.    24 September 1991    Clark Cyr
 */
        pRect->left   = (LONG)CW_USEDEFAULT;
        pRect->top    = SW_SHOWNORMAL;
        pRect->right  = 64 * nPixMMX + 4 * cxFrame;
        pRect->bottom = 64 * nPixMMY + 4 * cyFrame + cySize;
    }
    else
    {
        short cxScreen = (short) GetSystemMetrics (SM_CXSCREEN);
        short cyScreen = (short) GetSystemMetrics (SM_CYSCREEN);

        pRect->right -= pRect->left;  /* right is now width   */
        pRect->bottom -= pRect->top;  /* bottom is now height */

        if (pRect->left > cxScreen - cxFrame - cxSize)
            pRect->left = cxScreen - cxFrame - cxSize;
        else if (pRect->left < cxFrame + cxSize - pRect->right)
            pRect->left = cxFrame + cxSize - pRect->right;

        if (pRect->top > cyScreen - cyFrame - cySize)
            pRect->top = cyScreen - cyFrame - cySize;
        else if (pRect->top < cxFrame + cxSize - pRect->bottom)
            pRect->top = cxFrame + cxSize - pRect->bottom;
    }

}


#define ADVANCE(sz)  while (*sz && *sz != TEXT(',')) sz++; \
                     while (*sz && *sz == TEXT(',')) sz++; \
                     if (!*sz) return;

void NEAR PASCAL ParseSavedFlags (LPTSTR szBuf, PCLOCKDISPSTRUCT pClck)
{
    pClck->wFormat = IDM_ANALOG;
    pClck->bIconic = FALSE;
    pClck->bNoSeconds = FALSE;
    pClck->bNoTitle = FALSE;
    pClck->bTopMost = FALSE;
    pClck->bNoDate = FALSE;

    if (!szBuf)
       return;

    pClck->wFormat = (WORD)(MyAtoi(szBuf) ? IDM_ANALOG : IDM_DIGITAL);
    ADVANCE (szBuf);
    pClck->bIconic = (MyAtoi(szBuf) ? TRUE : FALSE);
    ADVANCE (szBuf);
    pClck->bNoSeconds = (MyAtoi(szBuf) ? TRUE : FALSE);
    ADVANCE (szBuf);
    pClck->bNoTitle = (MyAtoi(szBuf) ? TRUE : FALSE);
    ADVANCE (szBuf);
    pClck->bTopMost = (MyAtoi(szBuf) ? TRUE : FALSE);
    ADVANCE (szBuf);
    pClck->bNoDate = (MyAtoi(szBuf) ? TRUE : FALSE);
}


VOID NEAR PASCAL PrepareSavedWindow (LPTSTR szBuf, PRECT pRect)
{
    wsprintf (szBuf, TEXT("%i,%i,%i,%i"), pRect->left, pRect->top,
              pRect->right, pRect->bottom);
}

TRIG CirTab[] = {       // circle sin, cos, table
    { 0,     -7999  },
    { 836,   -7956  },
    { 1663,  -7825  },
    { 2472,  -7608  },
    { 3253,  -7308  },
    { 3999,  -6928  },
    { 4702,  -6472  },
    { 5353,  -5945  },
    { 5945,  -5353  },
    { 6472,  -4702  },
    { 6928,  -4000  },
    { 7308,  -3253  },
    { 7608,  -2472  },
    { 7825,  -1663  },
    { 7956,  -836   },

    { 8000,  0      },
    { 7956,  836    },
    { 7825,  1663   },
    { 7608,  2472   },
    { 7308,  3253   },
    { 6928,  4000   },
    { 6472,  4702   },
    { 5945,  5353   },
    { 5353,  5945   },
    { 4702,  6472   },
    { 3999,  6928   },
    { 3253,  7308   },
    { 2472,  7608   },
    { 1663,  7825   },
    { 836,   7956   },

    { 0,     7999   },
    { -836,  7956   },
    { -1663, 7825   },
    { -2472, 7608   },
    { -3253, 7308   },
    { -4000, 6928   },
    { -4702, 6472   },
    { -5353, 5945   },
    { -5945, 5353   },
    { -6472, 4702   },
    { -6928, 3999   },
    { -7308, 3253   },
    { -7608, 2472   },
    { -7825, 1663   },
    { -7956, 836    },

    { -7999, -0     },
    { -7956, -836   },
    { -7825, -1663  },
    { -7608, -2472  },
    { -7308, -3253  },
    { -6928, -4000  },
    { -6472, -4702  },
    { -5945, -5353  },
    { -5353, -5945  },
    { -4702, -6472  },
    { -3999, -6928  },
    { -3253, -7308  },
    { -2472, -7608  },
    { -1663, -7825  },
    { -836 , -7956  }
};

VOID NEAR PASCAL PrepareSavedFlags (LPTSTR szBuf, PCLOCKDISPSTRUCT pClck)
{
    wsprintf (szBuf, TEXT("%i,%i,%i,%i,%i,%i"),
              (pClck->wFormat == IDM_ANALOG ? 1 : 0),
              (pClck->bIconic ? 1 : 0), (pClck->bNoSeconds ? 1 : 0),
              (pClck->bNoTitle ? 1 : 0), (pClck->bTopMost ? 1 : 0),
              (pClck->bNoDate ? 1 : 0));
}

/*
 *  SaveClockOptions()
 */

VOID NEAR PASCAL SaveClockOptions (HWND hWnd)
{
    TCHAR    szInt[80];
    INT      i = (INT) IsZoomed (hWnd);

/* Bug 15058: Don't save the rectangle if we're maximized, assume it
 * has already been saved for restoration when maximization took place.
 *       18 October 1991           Clark Cyr
 */
    if (!ClockDisp.bIconic && !i)
        GetWindowRect (hWnd, &rCoordRect);

    wsprintf (szInt, TEXT("%i"), i);
    WritePrivateProfileString(szSection, szMaximized, szInt, szIniFile);

    /* write current clock options */
    PrepareSavedFlags (szInt, &ClockDisp);
    WritePrivateProfileString (szSection, szOptions, szInt, szIniFile);

    /* write window position and size */
    PrepareSavedWindow (szInt, &rCoordRect);
    WritePrivateProfileString (szSection, szPosition, szInt, szIniFile);

#ifdef JAPAN
    WritePrivateProfileString(
            szSection, szFontFileKey, FontStruct.lfFaceName, szIniFile );

    wsprintf (szInt, TEXT("%i"), (INT)FontStruct.lfCharSet);
    WritePrivateProfileString(szSection, szCharSet, szInt, szIniFile);
#endif
}


#if defined(JAPAN) || defined(KOREA)
/*
 * ExceptVerticalFont()
 */

UINT FAR PASCAL  ExceptVerticalFont(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LOGFONT lf;
    TCHAR szFaceName[LF_FACESIZE];
    UINT nId, count;

    switch (message)
    {
      case WM_INITDIALOG:
        count = SendDlgItemMessage(hwnd, cmb1, CB_GETCOUNT, 0, 0L);
        /* except vertical font */
        for (nId = 0; nId < count; nId++)
        {
            SendDlgItemMessage(hwnd, cmb1, CB_GETLBTEXT,
                               nId, (LONG) szFaceName);
            if (szFaceName[0] == TEXT('@'))
            {
                SendDlgItemMessage(hwnd, cmb1, CB_DELETESTRING, nId, 0L);
                nId--;
                count--;
            }
        }
        /* set selection current selected facename */
        SendMessage(hwnd, WM_CHOOSEFONT_GETLOGFONT, 0, (LONG) (LPTSTR) &lf);
        nId = SendDlgItemMessage(hwnd, cmb1, CB_FINDSTRING,
                                 0, (LONG) lf.lfFaceName);
        SendDlgItemMessage(hwnd, cmb1, CB_SETCURSEL, nId, 0L);

                // KKBUGFIX #1364: 12/10/92: Set focus on face name
        return(TRUE);

      default:
        return(FALSE);
    }
}
#endif


HRGN CreateEllipticWndRgn(HWND hWnd, LPRECT lprc)
{
    int  cSide;
    int  xOffset;
    int  yOffset;
    HRGN hRgn = NULL;


    if(lprc)
    {
        cSide = min(lprc->right, lprc->bottom);

        xOffset = GetSystemMetrics(SM_CXFRAME) + ((lprc->right  - cSide) >> 1);
        yOffset = GetSystemMetrics(SM_CYFRAME) + ((lprc->bottom - cSide) >> 1);

        hRgn = CreateEllipticRgn(xOffset, yOffset, xOffset+cSide, yOffset+cSide);
    }

    return(hRgn);
}


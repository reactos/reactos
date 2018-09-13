/****************************************************************************

    PROGRAM:    Access.c

    PURPOSE:    Provide Access for individuals with physical impairments

    FUNCTIONS:

    WinMain()         - calls initialization function, processes message loop
    AccessWndProc()   - processes messages
    About()           - processes messages for "About" dialog box

    COMMENTS:

    Windows can have several copies of your application running at the
    same time.  The variable hInst keeps track of which instance this
    application is so that processing will be to the correct window.

    You only need to initialize the application once.  After it is
    initialized, all other copies of the application will use the same
    window class, and do not need to be separately initialized.

****************************************************************************/

#include "windows.h"              /* required for all Windows applications */
#include "Access.h"               /* specific to this program */
#include "Menu.h"
#include "accessid.h"
#include "dialogs.h"
#include <string.h>

int    testnumber = 10;
char   lpReturn[11];
int    monitor_type = 0;
int    string_result = 1;

extern HWND    FilterhWnd;     /* from Dialogs.c */
extern HWND    StickyhWnd;
extern HWND    MousehWnd;
extern HWND    TogglehWnd;
extern HWND    SerialhWnd;
extern HWND    TimeOuthWnd;
extern HWND    ShowSoundshWnd;
extern HWND    SoundSentryhWnd;
extern BOOL    userpainthidden;
extern HBRUSH  hBrush;

HMENU hmenuaccess,hmenusubaccess1;
WORD  wpopupmenu1 = FALSE;
WORD  wpopuptemp = FALSE;

HANDLE  hInst;                                /* current instance */
BOOL    bmessage = TRUE;
HOOKPROC lpfnNewDialogHook = NULL;
HHOOK	lpfnOldDialogHook = NULL;
BOOL    bHelp = FALSE;                        /* Help mode flag; TRUE = "ON" */
int     iCN,iBR,iCCN,iCBR;
BOOL    bCO;
int     iPlatform;

char    szHelpFileName[EXE_NAME_MAX_SIZE+1];     /* Help file name */


/****************************************************************************/

short OkAccessMessage (HWND hWnd, WORD wnumber)

{
    int  ianswer;
    char sreadbuf[255];
    char sbuffer[45];
    {

    LoadString (hInst,wnumber,(LPSTR)sreadbuf,245);
    LoadString (hInst,IDS_TITLE,(LPSTR)sbuffer,35);
    if (bmessage)
       ianswer = MessageBox (hWnd, (LPSTR)sreadbuf,(LPSTR)sbuffer, MB_YESNO|MB_ICONHAND);
    else
       ianswer = MessageBox (hWnd, (LPSTR)sreadbuf,(LPSTR)sbuffer, MB_OK|MB_ICONHAND);
    }

	return ((short)ianswer);
}

short AccessMessageBox (HWND hWnd, WORD wnumber, UINT iFlags )

{
    int  ianswer;
    char sreadbuf[255];
    char sbuffer[45];
    {

    LoadString (hInst,wnumber,(LPSTR)sreadbuf,245);
    LoadString (hInst,IDS_TITLE,(LPSTR)sbuffer,35);
    ianswer = MessageBox (hWnd, (LPSTR)sreadbuf,(LPSTR)sbuffer, iFlags|MB_ICONHAND);
    }

	return ((short)ianswer);
}

void OkAccessMsg( HWND hWnd, WORD wMsg,...)
    {
    char szBuffer[256];
    char szFormat[256];
    char szTitle[48];
    va_list vaList;

    LoadString (hInst,IDS_TITLE,(LPSTR)szTitle,sizeof szTitle);
    LoadString (hInst,wMsg,(LPSTR)szFormat,sizeof szFormat);
    //
    // Use va_start, va_end to manage vaList
    //
    va_start(vaList, wMsg);
    wvsprintf( szBuffer, szFormat, vaList );
    va_end(vaList);

    MessageBox( hWnd, (LPSTR)szBuffer, (LPSTR)szTitle, MB_OK );
    }

/*************************************************************************

    FUNCTION:    WinMain(HANDLE, HANDLE, LPSTR, int)

    PURPOSE:    processes message loop

    COMMENTS:

    This will initialize the window class if it is the first time this
    application is run.  It then creates the window, and processes the
    message loop until a PostQuitMessage is received.  It exits the
    application by returning the value passed by the PostQuitMessage.

****************************************************************************/

int PASCAL WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
HINSTANCE hInstance;                            /* current instance */
HINSTANCE hPrevInstance;                        /* previous instance */
LPSTR lpCmdLine;                                /* command line */
int nCmdShow;                                   /* show-window type (open/icon)    */

{
    static    char szAppName[] = "Access";
    static    char szAppTitle[] = "Access Utility";
    static    char szAppMenu[] = "ACCESSMENU";

    HWND hWnd;                                  /* window handle */
    MSG msg;                                    /* message */
    WNDCLASS    wc;

    DWORD dwOS;
    WORD wWinVer;
    BOOL fNT;

    // abort if running on WinNT versions < 3.5 or > 3.99
    dwOS = GetVersion();
    fNT = (HIWORD(dwOS) & 0x8000) == 0;
    wWinVer = LOWORD(GetVersion());
    if( ( fNT && ( LOBYTE(wWinVer) != 3 || HIBYTE(wWinVer) < 0x32 ) )
        || ( !fNT && LOBYTE(wWinVer) < 4 ) )
        {
        OkFiltersMessage (NULL,IDS_BAD_OS_VER);
        return (FALSE);
        }

    // The following test is good only for Win16 but harmless under Win32
    if (hPrevInstance)                          /* Has application been initialized? */
        return (FALSE);                         /*  yes, don't start again */

    // The following test is good only for Win32
    hWnd = FindWindow( szAppName, szAppTitle ); /* If another copy of us is running */
    if( hWnd != NULL )
        {
        SetForegroundWindow( hWnd );            /* Bring IT to the foreground */
        return (FALSE);                         /* and exit ourselves */
        }

    wc.style            = CS_HREDRAW | CS_VREDRAW ;
    wc.lpfnWndProc      = AccessWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = hInstance;
    wc.hIcon            = LoadIcon(hInstance, (LPSTR) "icon");
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = GetStockObject(WHITE_BRUSH);
   //    wc.hbrBackground = COLOR_WINDOW +1 ;
    wc.lpszMenuName     = szAppMenu;
    wc.lpszClassName    = szAppName;

    RegisterClass(&wc);

    hInst = hInstance;                          /* Saves the current instance */

    hWnd = CreateWindow(szAppName,              /* window class */
        szAppTitle,                     /* window name */
        WS_OVERLAPPEDWINDOW,            /* window style */
        30,                             /* x position */
        30,                             /* y position */
        308,                            /* width */
    //  47,                             /* height */
        GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYMENU) +
        GetSystemMetrics(SM_CYFRAME) * 2 +
        GetSystemMetrics(SM_CYBORDER) * 4,
        NULL,                           /* parent handle */
        NULL,                           /* menu or child ID */
        hInstance,                      /* instance */
        NULL);                          /* additional info */

   /* CreateWindow sends a WM_CREATE message ****************************************/


    if (!hWnd)                          /* Was the window created? */
    {
        OkAccessMsg(NULL,IDS_UNABLE_TO_START);
        return (0);
    }

   //    hdcWndMain = GetDC(hWnd);
   //    SetBkColor (hdcWndMain,GetSysColor(COLOR_WINDOW));
   //    SetTextColor(hdcWndMain,GetSysColor(COLOR_WINDOWTEXT));
   //    ReleaseDC(hWnd,hdcWndMain);

    ShowWindow(hWnd, nCmdShow);  /* Shows the window and send WM_SIZE message */
    UpdateWindow(hWnd);          /* Sends WM_PAINT message */


   /*
    * UpdateWindow() call forces Windows to send WM_PAINT immediately
    * instead of taking it's time by posting a WM_PAINT message in the
    * message queue.  Windows exits to WM_PAINT message to process it
    * and returns to next instruction after UpDateWindows when it finishes.
    */

    MakeHelpPathName(szHelpFileName);

    while (GetMessage(&msg,        /* message structure */
               NULL,               /* handle of window receiving the message */
               0,                  /* lowest message to examine */
               0))                 /* highest message to examine */
    {
        TranslateMessage(&msg);    /* Translates virtual key codes */
        DispatchMessage(&msg);     /* Dispatches message to window */
    }

    return ((int)msg.wParam);      /* Returns the value from PostQuitMessage */
}

/****************************************************************************

    FUNCTION:    AccessWndProc(HWND, unsigned, WORD, LONG)

    PURPOSE:    Processes messages

    WM_COMMAND processing:

        IDM_ABOUT               - display "About" box.
        IDM_SAVE_TO_WIN_INI     - display message box before saving user parameters
        IDM_ADJUST_SERIALKEYS   - displag dialog box...
        IDM_ADJUST_TOGGLEKEYS   -
        IDM_ADJUST_STICKEYS     -
        IDM_ADJUST_FILTERKEYS   -
        IDM_ADJUST_MOUSEKEYS    -
        IDM_ADJUST_SHOWSOUNDS   -
        IDM_ADJUST_TIMEOUT      -


****************************************************************************/

LRESULT APIENTRY AccessWndProc(HWND hWnd, UINT message,
                               WPARAM wParam, LPARAM lParam)
{

    int     MessageReturn;
    DWORD   dwHelpContextId;
    extern  int        fQuestion_Save;

#ifdef NOTUSED
    lp_kybdinfoparam   = &KybdInfoParam;
    lp_mouinfoparam    = &MouInfoParam;
    lpa_passthecomvars = &passthecomvars;
#endif

    switch (message) {

    case WM_ENTERIDLE:
        if ((wParam == MSGF_MENU) && (GetKeyState(VK_F1) & 0x8000))
        {
            bHelp = TRUE;
            PostMessage(hWnd, WM_KEYDOWN, VK_RETURN, 0L);
            break;
        }
        if ((wParam == MSGF_DIALOGBOX) && (userpainthidden))
        {
            SendMessage(FilterhWnd,WM_HSCROLL,SB_LINEUP,0L);
            break;
        }

    case WM_COMMAND:
      /* Was F1 just pressed in a menu, or are we in help mode */
      /* (Shift-F1)? */

      if (bHelp)
       {
          dwHelpContextId =
              (wParam == IDM_SAVE_TO_WIN_INI)    ? (DWORD) HELPID_SAVE_TO_WIN_INI:
              (wParam == IDM_SAVE_DEFAULT)       ? (DWORD) HELPID_SAVE_DEFAULT:
              (wParam == IDM_EXIT)               ? (DWORD) HELPID_EXIT  :
              (wParam == IDM_ABOUT)              ? (DWORD) HELPID_ABOUT :
              (wParam == IDM_ADJUST_STICKEYS)    ? (DWORD) IDM_HELP_STICKYKEYS :
              (wParam == IDM_ADJUST_FILTERKEYS)  ? (DWORD) IDM_HELP_FILTERKEYS :
              (wParam == IDM_ADJUST_MOUSEKEYS)   ? (DWORD) IDM_HELP_MOUSEKEYS  :
              (wParam == IDM_ADJUST_TIMEOUT)     ? (DWORD) IDM_HELP_TIMEOUT    :
              (wParam == IDM_ADJUST_SERIALKEYS)  ? (DWORD) IDM_HELP_SERIALKEYS :
              (wParam == IDM_ADJUST_TOGGLEKEYS)  ? (DWORD) IDM_HELP_TOGGLEKEYS :
              (wParam == IDM_ADJUST_SHOWSOUNDS)  ? (DWORD) IDM_HELP_SHOWSOUNDS :
              (wParam == IDM_ADJUST_SOUNDSENTRY) ? (DWORD) IDM_HELP_SOUNDSENTRY:
                                       (DWORD) 0L;

          if (!dwHelpContextId)
           {
               bHelp = FALSE;
               OkFiltersMessage (hWnd,IDS_HELP_MESSAGE);
               return (DefWindowProc(hWnd, message, wParam, lParam));
           }

          bHelp = FALSE;
          WinHelp(hWnd,szHelpFileName,HELP_CONTEXT,dwHelpContextId);
          break;
       }

       switch (wParam)
           {

           case IDM_EXIT:
               SendMessage (hWnd,WM_CLOSE,0,0L);
               return (0L);
               break;

           case IDM_HELP:
               dwHelpContextId = (DWORD) 0xFFFF ;
               bHelp = FALSE;
               WinHelp(hWnd,szHelpFileName,HELP_CONTEXT,dwHelpContextId);
               break;

           case IDM_HELP_START:
               dwHelpContextId = (DWORD) HELPID_HELP ;
               bHelp = FALSE;
               WinHelp(hWnd,szHelpFileName,HELP_CONTEXT,dwHelpContextId);
               break;

           case IDM_HELP_FILTERKEYS:
               dwHelpContextId = (DWORD) IDM_HELP_FILTERKEYS;
               bHelp = FALSE;
               WinHelp(hWnd,szHelpFileName,HELP_CONTEXT,dwHelpContextId);
               break;

           case IDM_HELP_STICKYKEYS:
               dwHelpContextId = (DWORD) IDM_HELP_STICKYKEYS;
               bHelp = FALSE;
               WinHelp(hWnd,szHelpFileName,HELP_CONTEXT,dwHelpContextId);
               break;

           case IDM_HELP_MOUSEKEYS:
               dwHelpContextId = (DWORD) IDM_HELP_MOUSEKEYS;
               bHelp = FALSE;
               WinHelp(hWnd,szHelpFileName,HELP_CONTEXT,dwHelpContextId);
               break;

           case IDM_HELP_TOGGLEKEYS:
               dwHelpContextId = (DWORD) IDM_HELP_TOGGLEKEYS;
               bHelp = FALSE;
               WinHelp(hWnd,szHelpFileName,HELP_CONTEXT,dwHelpContextId);
               break;

           case IDM_HELP_SERIALKEYS:
               dwHelpContextId = (DWORD) IDM_HELP_SERIALKEYS;
               bHelp = FALSE;
               WinHelp(hWnd,szHelpFileName,HELP_CONTEXT,dwHelpContextId);
               break;

           case IDM_HELP_TIMEOUT:
               dwHelpContextId = (DWORD) IDM_HELP_TIMEOUT;
               bHelp = FALSE;
               WinHelp(hWnd,szHelpFileName,HELP_CONTEXT,dwHelpContextId);
               break;

           case IDM_HELP_SHOWSOUNDS:
               dwHelpContextId = (DWORD) IDM_HELP_SHOWSOUNDS;
               bHelp = FALSE;
               WinHelp(hWnd,szHelpFileName,HELP_CONTEXT,dwHelpContextId);
               break;

           case IDM_HELP_SOUNDSENTRY:
               dwHelpContextId = (DWORD) IDM_HELP_SOUNDSENTRY;
               bHelp = FALSE;
               WinHelp(hWnd,szHelpFileName,HELP_CONTEXT,dwHelpContextId);
               break;

           case IDM_ABOUT:
               if( (HIWORD(GetVersion()) & 0x8000) == 0 )
                    {
                    // windows nt dialog
                    DialogBox(hInst,             /* current instance */
                        MAKEINTRESOURCE(808),    /* resource to use */
                        hWnd,                    /* parent handle */
                        About);                  /* About() instance address */
                    }
                else
                    {
                    // windows chicago dialog
                    DialogBox(hInst,             /* current instance */
                        MAKEINTRESOURCE(807),    /* resource to use */
                        hWnd,                    /* parent handle */
                        About);                  /* About() instance address */
                    }
               break;

           case IDM_SAVE_DEFAULT:
               if( fQuestion_Save == 1 )
                    {
                    MessageReturn = AccessMessageBox( hWnd, IDS_SAVE_FIRST, MB_YESNOCANCEL );
                    if( MessageReturn == IDYES )
                        {
                        SaveFeatures();
                        fQuestion_Save =0;
                        }
                    else if( MessageReturn == IDCANCEL )
                        {
                        break; // cancel the save procedure
                        }
                    else if( MessageReturn == IDNO )
                        ; // fall through to normal processing
                    else
                        ; // we should print an error message but what the heck
                    }

               MessageReturn = OkAccessMessage (hWnd,IDS_SAVE_DEFAULT);
               if (MessageReturn == IDYES )
                   {
                   DWORD iStatus;
                   iStatus = SaveDefaultSettings();
                   switch( iStatus )
                        {
                        case ERROR_SUCCESS:
                                break;
                        case ERROR_ACCESS_DENIED:
                                OkFiltersMessage (hWnd,IDS_ACCESS_DENIED);
                                break;
                    //    case ERROR_BADDB:
                    //    case ERROR_CANT_OPEN:
                    //    case ERROR_CANT_READ:
                    //    case ERROR_INVALID_PARAMETER:
                    //    case ERROR_OUT_OF_MEMORY:
                    //    case ERROR_PRIVILEGE_NOT_HELD:
                    //    case ERROR_KEY_DELETED:
                    //    case ERROR_FILE_NOT_FOUND:
                    //    case ERROR_CHILD_MUST_BE_VOLATILE:
                        default:
                                OkAccessMsg(hWnd,IDS_ERROR_CODE,iStatus);
                                break;
                        }
                   // note we don't reset fQuestion_Save so that, if we saved
                   // as default without saving locally, we will still be
                   // prompted to save for this account on exit
                   }
               break;

           case IDM_SAVE_TO_WIN_INI:
#ifdef NOSAVE
               bmessage = FALSE;
               OkAccessMessage (hWnd,IDS_SAVE_DISABLED);
               bmessage = TRUE;
               break;
#else
               MessageReturn = OkAccessMessage (hWnd,IDS_SAVE_TO_WIN_INI);

               if (MessageReturn == IDYES )
                   {
                   SaveFeatures();
                   fQuestion_Save =0;
                   }
               break;
#endif // else if not def nosave

            case IDM_ADJUST_STICKEYS:
                DialogBox(hInst,           /* current instance */
                    MAKEINTRESOURCE(800),  /* resource to use */
                    hWnd,                  /* parent handle */
                    AdjustSticKeys);       /* dialog instance address */

                break;

            case IDM_ADJUST_FILTERKEYS:
                DialogBox(hInst,           /* current instance */
                    MAKEINTRESOURCE(801),  /* resource to use */
                    hWnd,                  /* parent handle */
                    AdjustFilterKeys);     /* dialog instance address */

                break;

            case IDM_ADJUST_MOUSEKEYS:
                DialogBox(hInst,           /* current instance */
                    MAKEINTRESOURCE(802),  /* resource to use */
                    hWnd,                  /* parent handle */
                    AdjustMouseKeys);      /* dialog instance address */

                break;

           case IDM_ADJUST_SERIALKEYS:
               DialogBox(hInst,             /* current instance */
                    MAKEINTRESOURCE(803),    /* resource to use */
                    hWnd,                    /* parent handle */
                    AdjustSerialKeys);       /* dialog instance address */
                break;

           case IDM_ADJUST_TIMEOUT:
                DialogBox(hInst,           /* current instance */
                    MAKEINTRESOURCE(804),  /* resource to use */
                    hWnd,                  /* parent handle */
                    AdjustTimeOut);        /* dialog instance address */

                break;


           case IDM_ADJUST_TOGGLEKEYS:
                DialogBox(hInst,           /* current instance */
                    MAKEINTRESOURCE(805),  /* resource to use */
                    hWnd,                  /* parent handle */
                    AdjustToggleKeys);     /* dialog instance address */

                break;


           case IDM_ADJUST_SHOWSOUNDS:
                if (fShowSoundsOn) {
                    CheckMenuItem(GetMenu(hWnd), LOWORD(wParam), MF_UNCHECKED);
                    fShowSoundsOn = 0;
                } else {
                    CheckMenuItem(GetMenu(hWnd), LOWORD(wParam), MF_CHECKED);
                    fShowSoundsOn = 1;
                }
                SystemParametersInfo(
                    SPI_SETSHOWSOUNDS,
                    fShowSoundsOn,
                    0,
                    0);
                fQuestion_Save = 1;
                break;


           //
           // Full screen text and graphics modes are only available on the
           // chicago platform so we use separate dialogs.
           // also won't support them on Cairo non-x86 platforms; but right
           // now we'll just skip on any non-Chicago.
           //
           case IDM_ADJUST_SOUNDSENTRY:
               if( (HIWORD(GetVersion()) & 0x8000) == 0 )
                    {
                    // windows nt dialog
                    DialogBox(hInst,             /* current instance */
                        MAKEINTRESOURCE(809),    /* resource to use */
                        hWnd,                    /* parent handle */
                        AdjustSoundSentry);      /* dialog instance address */
                    }
                else
                    {
                    // windows chicago dialog
                    DialogBox(hInst,             /* current instance */
                        MAKEINTRESOURCE(806),    /* resource to use */
                        hWnd,                    /* parent handle */
                        AdjustSoundSentry);      /* dialog instance address */
                    }
                break;

           default:
                return (DefWindowProc(hWnd, message, wParam, lParam));
                break;

            }

    break;


    case WM_KEYDOWN:

    /******* Decided not to support shift+F1 support, just F1 support.
             See Windows Programming Tools, p18-17 for other ideas if
             want to add some day */

         switch (wParam)
             {
             case VK_F1:

                 dwHelpContextId = (DWORD) 0xFFFF;
                 bHelp = FALSE;
                 WinHelp(hWnd,szHelpFileName,HELP_CONTEXT,dwHelpContextId);
                 break;

             default:
                 return (DefWindowProc(hWnd, message, wParam, lParam));
                 break;
             }
         return(0);

    case WM_CREATE:

      {
	  
      InitializeUserRegIfNeeded();
      if( (HIWORD(GetVersion()) & 0x8000) != 0 )
           {
           HMENU hMenu;
           // chicago -- remove the Save Default command
           hMenu = GetMenu(hWnd);
           DeleteMenu(hMenu, IDM_SAVE_DEFAULT, MF_BYCOMMAND );
           DrawMenuBar(hWnd);
           }

      if (!lpfnNewDialogHook)
         {
             DWORD dwThread;
             dwThread = GetCurrentThreadId();
             lpfnNewDialogHook = hookDialogBoxMsg;
             if (lpfnNewDialogHook)
                 lpfnOldDialogHook = SetWindowsHookEx(WH_MSGFILTER,lpfnNewDialogHook,NULL,dwThread);
         }

      InitFeatures(hWnd,hInst);

      // initialize the ShowSounds menu item correctly checked or unchecked
      if (fShowSoundsOn) {
          CheckMenuItem(GetMenu(hWnd), IDM_ADJUST_SHOWSOUNDS, MF_CHECKED);
      } else {
          CheckMenuItem(GetMenu(hWnd), IDM_ADJUST_SHOWSOUNDS, MF_UNCHECKED);
      }

///   ShowSoundsParam.fvideo_flash = FALSE;
///   Set_ShowSounds_Param(&ShowSoundsParam);

// WHAT IS THIS CODE ABOUT CHECKING FOR ALTERED MENUS ALL ABOUT???
      hmenuaccess = GetMenu(hWnd);
      hmenusubaccess1 = GetSubMenu(hmenuaccess,1);  /* &Adjust popup */
      wpopupmenu1 = (WORD)GetMenuItemCount(hmenusubaccess1);
      }


      return(0);
      break;

      case WM_QUERYENDSESSION:
      case WM_CLOSE:

         /*  Both EXIT and CLOSE get here, as selecting close from the menu
             horizontal bar will cause WM_CLOSE and exit is programmmed to
             send a WM_CLOSE also */

         // check to see if anyone appended or changed our menu but onlu if AU Windows was created??

        // WHAT IS THIS CODE ABOUT CHECKING FOR ALTERED MENUS ALL ABOUT???
         if (wpopupmenu1)
         {
            wpopuptemp = (WORD)GetMenuItemCount(hmenusubaccess1);
            if (wpopupmenu1 != -1)
               {
               if (wpopupmenu1 != wpopuptemp)
                  {
                  bmessage = FALSE;
                  MessageReturn = OkAccessMessage (hWnd,IDS_MENU_MESSAGE);
                  bmessage = TRUE;
                  return(0L);
                  }
               }
         }

         if (fQuestion_Save == 1)
            {
#ifdef NOSAVE
            bmessage = FALSE;
            OkAccessMessage (hWnd,IDS_SAVE_DISABLED);
            bmessage = TRUE;
#else
            fQuestion_Save =0;
            MessageReturn = OkAccessMessage (hWnd,IDS_CLOSE_MESSAGE);

            if (MessageReturn == IDYES )
               {
               SaveFeatures();
               }
#endif
         }


         if (message == WM_QUERYENDSESSION)
            return (TRUE);
         else
            DestroyWindow(hWnd);
         return (0L);
         break;

     case WM_DESTROY:

         WinHelp(hWnd,szHelpFileName,HELP_QUIT,0L);

         // check to see if anyone appended or changed our menu but only if AU Windows was created??

         if (wpopupmenu1)
         {
         wpopuptemp = (WORD)GetMenuItemCount(hmenusubaccess1);
         if (wpopupmenu1 != -1)
            {
            if (wpopupmenu1 != wpopuptemp)
               {
               bmessage = FALSE;
               MessageReturn = OkAccessMessage (hWnd,IDS_MENU_MESSAGE);
               bmessage = TRUE;
               return(0L);
               }
            }
         }

         if (hBrush)
            DeleteObject(hBrush);

         if (lpfnNewDialogHook)
            {
                UnhookWindowsHookEx (lpfnOldDialogHook);
                lpfnNewDialogHook = NULL;
                lpfnOldDialogHook = NULL;
            }

         PostQuitMessage(0);
         return (0);

      /* Passes it on if unproccessed */
      default:
         return (DefWindowProc(hWnd, message, wParam, lParam));
         break;

    }
    return (0);
}


/****************************************************************************

    FUNCTION:    About(HWND, unsigned, WORD, LONG)

    PURPOSE:    Processes messages for "About" dialog box

    MESSAGES:

    WM_INITDIALOG    - initialize dialog box
    WM_COMMAND       - Input received

    COMMENTS:

    No initialization is needed for this particular dialog box, but TRUE
    must be returned to Windows.

    Wait for user to click on "Ok" button, then close the dialog box.

****************************************************************************/

LRESULT APIENTRY About(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)

    {

    switch (message) {
    case WM_INITDIALOG:                     /* message: initialize dialog box */
        return (TRUE);

    case WM_COMMAND:                        /* message: received a command */
        if (wParam == IDOK) {               /* "OK" box selected? */
        EndDialog(hDlg, 0);                 /* Exits the dialog box */
        return (TRUE);
        }
        break;
    }
    return (FALSE);                            /* Didn't process a message */
}

/****************************************************************************

   FUNCTION:   MakeHelpPathName

   PURPOSE:    HelpEx assumes that the .HLP help file is in the same
               directory as the HelpEx executable.  This function derives
               the full path name of the help file from the path of the
               executable.

****************************************************************************/

void MakeHelpPathName(szFileName)
char * szFileName;
{
   char *  pcFileName;
   int     nFileNameLen;
   static    char szAppHelp[8];
   DWORD dwOS;
   WORD  wWinVer;

   dwOS = GetVersion();
   /*
    * If the high bit of the high word is 1, the OS platform is either
    * Win32s or Win3.1.  If the high bit of the high word is 0, the OS
    * platform is NT.
    */
   if (HIWORD(dwOS) & 0x8000) {
       wWinVer = LOWORD(GetVersion());
       if ((LOBYTE(wWinVer) == 3) && (HIBYTE(wWinVer) == 0))
          strcpy(szAppHelp,"access30.hlp");
       else if((LOBYTE(wWinVer) == 3) && (HIBYTE(wWinVer) > 0))
          strcpy(szAppHelp,"access31.hlp");
       else if((LOBYTE(wWinVer) == 4) && (HIBYTE(wWinVer) == 0))
          strcpy(szAppHelp,"access40.hlp");
   } else {
          strcpy(szAppHelp,"access35.hlp");
   }

   nFileNameLen = GetModuleFileName(hInst,szFileName,EXE_NAME_MAX_SIZE);
   pcFileName = szFileName + nFileNameLen;

   while (pcFileName > szFileName)
      {
      if (*pcFileName == '\\' || *pcFileName == ':')
          {
          *(++pcFileName) = '\0';
          break;
          }
      nFileNameLen--;
      pcFileName--;
      }

   if ((nFileNameLen+13) < EXE_NAME_MAX_SIZE)
      {
      lstrcat(szFileName, szAppHelp);
      }
   else
      {
      lstrcat(szFileName, "?");
      }

   return;
}

/****************************************************************************

   FUNCTION:   hookDialogBoxMsg

   PURPOSE:    To hook dialog box message and watch for F1 or help request.
               This is down by processing WM_KEYDOWN and WM_KEYUP messages.

****************************************************************************/
LRESULT APIENTRY hookDialogBoxMsg(nCode,wParam,lParam)    /* Function to trap for F1 in dialog boxes */
//HOOKPROC hookDialogBoxMsg(nCode,wParam,lParam)    /* Function to trap for F1 in dialog boxes */
int    nCode;
WPARAM wParam;
LPARAM lParam;
{
    LRESULT iResult;
    static MSG FAR *msgdialog;
    HWND hWndlocal;
    WORD localcontext;

    msgdialog = (MSG FAR *) lParam;
    iResult = TRUE;
    hWndlocal = NULL;


    /* first check if message is of interest */

    if (nCode == MSGF_DIALOGBOX)
    {
        if (msgdialog->message == WM_KEYDOWN)
        {
        /*  process virtual key code   */

        switch (msgdialog->wParam)
            {
            case VK_F1:

/***************************************************************************
    The following SendMessage will work, but it requires a global variable
    called "hWndParent".  I can get the same results if I query the message
    that the Filter function caught by sending the code here by getting a
    handle to the window the message was intended for, and then getting the
    parent of that handle, and then sending the message this way.

    ex. SendMessage(hWndParent,WM_COMMAND,IDM_HELP_FILTERKEYS,0L);

    ex. hWndlocal = GetParent(GetWindowWord(msgdialog->hwnd,GWW_HWNDPARENT));

*****************************************************************************/

                hWndlocal = GetParent(GetParent(msgdialog->hwnd));

                if (GetParent(msgdialog->hwnd) == FilterhWnd)
                        localcontext = IDM_HELP_FILTERKEYS;

                else if (GetParent(msgdialog->hwnd) == StickyhWnd)
                        localcontext = IDM_HELP_STICKYKEYS;

                else if (GetParent(msgdialog->hwnd) == MousehWnd)
                        localcontext = IDM_HELP_MOUSEKEYS;

                else if (GetParent(msgdialog->hwnd) == TogglehWnd)
                        localcontext = IDM_HELP_TOGGLEKEYS;

                else if (GetParent(msgdialog->hwnd) == SerialhWnd)
                        localcontext = IDM_HELP_SERIALKEYS;

                else if (GetParent(msgdialog->hwnd) == TimeOuthWnd)
                        localcontext = IDM_HELP_TIMEOUT;

                else if (GetParent(msgdialog->hwnd) == ShowSoundshWnd)
                        localcontext = IDM_HELP_SHOWSOUNDS;

                else if (GetParent(msgdialog->hwnd) == SoundSentryhWnd)
                        localcontext = IDM_HELP_SOUNDSENTRY;

                else
                    {
                    localcontext = IDM_HELP_START;
                    }

                SendMessage(hWndlocal,WM_COMMAND,localcontext,0L);
                break;

            default:

                iResult = CallNextHookEx(lpfnOldDialogHook,nCode,wParam,lParam);
                return (iResult);
                break;

            }

        }else if (msgdialog->message != WM_KEYUP)
            {
            iResult = CallNextHookEx(lpfnOldDialogHook,nCode,wParam,lParam);
            return (iResult);
            }

    }

    iResult = CallNextHookEx(lpfnOldDialogHook,nCode,wParam,lParam);
    return (iResult);
}

/****************************************************************************/

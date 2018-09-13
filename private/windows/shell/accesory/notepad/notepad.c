/*
 *   Notepad application
 *   Copyright (C) 1984-1995 Microsoft Inc.
 */


#include "precomp.h"
#include <htmlhelp.h>

#define DeepTrouble() MessageBox(hwndNP, szErrSpace, szNN, MB_SYSTEMMODAL|MB_OK|MB_ICONHAND);

UINT     lGotoLine;                  /* line number to goto to */

TCHAR    chMerge;
HWND     hwndNP = 0;                 /* handle to notepad parent window    */
HWND     hwndEdit = 0;               /* handle to main text control item  */
HANDLE   hEdit;                      /* Handle to storage for edit item   */
HWND     hDlgFind = NULL;            /* handle to modeless FindText window */
HANDLE   hStdCursor;                 /* handle to arrow or beam cursor    */
HANDLE   hWaitCursor;                /* handle to hour glass cursor       */
HANDLE   hInstanceNP;                /* Module instance handle            */
HANDLE   hFont;                      /* handle to Unicode font            */
LOGFONT  FontStruct;                 /* font dialog structure             */
INT      iPointSize=120;             /* current point size unit=1/10 pts  */
TCHAR    szFileName[MAX_PATH+1];     /* Current notepad filename          */
TCHAR    szSearch[CCHKEYMAX];        /* Search string                     */
TCHAR    szReplace[CCHKEYMAX];       /* replace string                    */

BOOL     fUntitled = TRUE;           /* TRUE iff notepad has no title      */

HMENU hSysMenuSetup;                 /* Save Away for disabled Minimize   */

DWORD     dwEmSetHandle = 0;          /* Is EM_SETHANDLE in process?       */
HANDLE   hAccel;                     /* Handle to accelerator table       */
BOOL     fRunBySetup = FALSE;        /* Did SlipUp WinExec us??           */
BOOL     fWrap = 0;                  /* Flag for word wrap                */
TCHAR    szNotepad[] = TEXT("Notepad");/* Name of notepad window class       */

BOOL fInSaveAsDlg = FALSE;

/* variables for the new File/Open, File/Saveas,Find Text and Print dialogs */
OPENFILENAME OFN;                     /* passed to the File Open/save APIs */
TCHAR szOpenFilterSpec[CCHFILTERMAX]; /* default open filter spec          */
TCHAR szSaveFilterSpec[CCHFILTERMAX]; /* default save filter spec          */
NP_FILETYPE g_ftOpenedAs=FT_UNKNOWN;  /* current file was opened           */
NP_FILETYPE g_ftSaveAs;               /* current file was opened           */

FINDREPLACE FR;                       /* Passed to FindText()              */
PAGESETUPDLG g_PageSetupDlg;
UINT wFRMsg;                          /* message used in communicating     */
                                      /* with Find/Replace dialog          */

DWORD dwCurrentSelectionStart = 0L;      /* WM_ACTIVATEAPP selection pos */
DWORD dwCurrentSelectionEnd   = 0L;      /* WM_ACTIVATEAPP selection pos */
UINT wHlpMsg;                        /* message used in invoking help     */

/* Strings loaded from resource file passed to LoadString at initialization time */
/* To add resource string:
 * 1) create IDS_ macro definition in notepad.h
 * 2) create string in resource file
 * 3) create 'TCHAR*' variable directly below and in notepad.h file
 * 4) add &variable to rgsz
 * 5) increment CSTRINGS
 */
TCHAR *szDiskError =(TCHAR *)IDS_DISKERROR;  /* Can't open File, check disk  */
TCHAR *szFNF       =(TCHAR *)IDS_FNF;        /* File not found               */
TCHAR *szFAE       =(TCHAR *)IDS_FAE;        /* File already exists          */
TCHAR *szSCBC      =(TCHAR *)IDS_SCBC;       /* Save changes before closing? */
TCHAR *szUntitled  =(TCHAR *)IDS_UNTITLED;   /* untitled                     */
TCHAR *szCFS       =(TCHAR *)IDS_CFS;        /* Can't find string            */
TCHAR *szErrSpace  =(TCHAR *)IDS_ERRSPACE;   /* Memory space exhausted       */
TCHAR *szNpTitle   =(TCHAR *)IDS_NOTEPAD;    /* Notepad -                    */
TCHAR *szFTL       =(TCHAR *)IDS_FTL;        /* File too large for notepad   */
TCHAR *szNN        =(TCHAR *)IDS_NN;         /* Notepad Note!                */
TCHAR *szPDIE      =(TCHAR *)IDS_PRINTDLGINIT; /* Print dialog init error    */
TCHAR *szCP        =(TCHAR *)IDS_CANTPRINT;  /* Can't print                  */
TCHAR *szNVF       =(TCHAR *)IDS_NVF;        /* Not a valid filename.        */
TCHAR *szCREATEERR =(TCHAR *)IDS_CREATEERR;  /* cannot create file           */
TCHAR *szNoWW      =(TCHAR *)IDS_NOWW;       /* Too much text to word wrap   */
TCHAR *szMerge     =(TCHAR *)IDS_MERGE1;     /* search string for merge      */

TCHAR *szHelpFile  =(TCHAR *)IDS_HELPFILE;   /* Name of helpfile.            */
TCHAR *szBadMarg   =(TCHAR *)IDS_BADMARG;    /* Bad margins.                 */
TCHAR *szFileOpenFail =(TCHAR *)IDS_FILEOPENFAIL;  /* Can't open File */
TCHAR *szAnsiText  =(TCHAR *)IDS_ANSITEXT; /* File/Open ANSI filter spec. string */
TCHAR *szAllFiles  =(TCHAR *)IDS_ALLFILES;   /* File/Open Filter spec. string */
TCHAR *szOpenCaption = (TCHAR *)IDS_OPENCAPTION; /* caption for File/Open dlg */
TCHAR *szSaveCaption = (TCHAR *)IDS_SAVECAPTION; /* caption for File/Save dlg */
TCHAR *szCannotQuit = (TCHAR *)IDS_CANNOTQUIT;  /* cannot quit during a WM_QUERYENDSESSION */
TCHAR *szLoadDrvFail = (TCHAR *)IDS_LOADDRVFAIL;  /* LOADDRVFAIL from PrintDlg */
TCHAR *szErrUnicode =(TCHAR *)IDS_ERRUNICODE;  /* Unicode character existence error */
TCHAR *szACCESSDENY = (TCHAR *)IDS_ACCESSDENY; /* Access denied on Open */
TCHAR *szFontTooBig = (TCHAR *)IDS_FONTTOOBIG; /* font too big or page too small */
TCHAR *szCommDlgErr = (TCHAR *)IDS_COMMDLGERR; /* common dialog error %x */
TCHAR *szCommDlgInitErr = (TCHAR *)IDS_COMMDLGINIT; /* common dialog error %x */


TCHAR *szLineError    = (TCHAR*)IDS_LINEERROR;   /* line number error        */
TCHAR *szLineTooLarge = (TCHAR*)IDS_LINETOOLARGE;/* line number out of range */

TCHAR *szFtAnsi       = (TCHAR*) IDS_FT_ANSI;
TCHAR *szFtUnicode    = (TCHAR*) IDS_FT_UNICODE;
TCHAR *szFtUnicodeBe  = (TCHAR*) IDS_FT_UNICODEBE;
TCHAR *szFtUtf8       = (TCHAR*) IDS_FT_UTF8;


TCHAR **rgsz[CSTRINGS-6] = {
        &szDiskError,
        &szFNF,
        &szFAE,
        &szSCBC,
        &szUntitled,
        &szErrSpace,
        &szCFS,
        &szNpTitle,
        &szFTL,
        &szNN,
        &szCommDlgInitErr,
        &szPDIE,
        &szCP,
        &szNVF,
        &szCREATEERR,
        &szNoWW,
        &szMerge,
        &szHelpFile,
        &szBadMarg,
        &szFileOpenFail,
        &szAnsiText,
        &szAllFiles,
        &szOpenCaption,
        &szSaveCaption,
        &szCannotQuit,
        &szLoadDrvFail,
        &szACCESSDENY,
        &szErrUnicode,
        &szCommDlgErr,
        &szFontTooBig,
        &szLineError,
        &szLineTooLarge,
        &szFtAnsi,
        &szFtUnicode,
        &szFtUnicodeBe,
        &szFtUtf8,
};


HANDLE   fp;          /* file pointer */


#if 0
VOID DisplayFont( LOGFONT* pf )
{
    TCHAR dbuf[100];

    ODS(TEXT("-----------------------\n"));
    wsprintf(dbuf,TEXT("lfHeight          %d\n"),pf->lfHeight); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfWidth           %d\n"),pf->lfWidth ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfEscapement      %d\n"),pf->lfEscapement); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfOrientation     %d\n"),pf->lfOrientation); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfWeight          %d\n"),pf->lfWeight); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfItalic          %d\n"),pf->lfItalic); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfUnderLine       %d\n"),pf->lfUnderline); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfStrikeOut       %d\n"),pf->lfStrikeOut); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfCharSet         %d\n"),pf->lfCharSet); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfOutPrecision    %d\n"),pf->lfOutPrecision); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfClipPrecision   %d\n"),pf->lfClipPrecision); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfQuality         %d\n"),pf->lfQuality); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfPitchAndFamily  %d\n"),pf->lfPitchAndFamily); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfFaceName        %s\n"),pf->lfFaceName); ODS(dbuf);

}
#endif

static TCHAR  szPath[MAX_PATH];

void FileDragOpen(void);
VOID NpResetMenu(HWND hWnd);
BOOL SignalCommDlgError(VOID);
VOID ReplaceSel( BOOL bView );

/* FreeGlobal, frees  all global memory allocated. */

void NEAR PASCAL FreeGlobal()
{
    if(g_PageSetupDlg.hDevMode)
    {
        GlobalFree(g_PageSetupDlg.hDevMode);
    }

    if(g_PageSetupDlg.hDevNames)
    {
        GlobalFree(g_PageSetupDlg.hDevNames);
    }

    g_PageSetupDlg.hDevMode=  NULL; // make sure they are zero for PrintDlg
    g_PageSetupDlg.hDevNames= NULL;
}

VOID PASCAL SetPageSetupDefaults( VOID )
{
    TCHAR szIMeasure[ 2 ];

    g_PageSetupDlg.lpfnPageSetupHook= PageSetupHookProc;
    g_PageSetupDlg.lpPageSetupTemplateName= MAKEINTRESOURCE(IDD_PAGESETUP);

    GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_IMEASURE, szIMeasure, 2 );

    g_PageSetupDlg.Flags= PSD_MARGINS  |
            PSD_ENABLEPAGESETUPHOOK | PSD_ENABLEPAGESETUPTEMPLATE;

    if (szIMeasure[ 0 ] == TEXT( '1' ))
    {
        //  English measure (in thousandths of inches).
        g_PageSetupDlg.Flags |= PSD_INTHOUSANDTHSOFINCHES;
        g_PageSetupDlg.rtMargin.top    = 1000;
        g_PageSetupDlg.rtMargin.bottom = 1000;
        g_PageSetupDlg.rtMargin.left   = 750;
        g_PageSetupDlg.rtMargin.right  = 750;
    }
    else
    {
        //  Metric measure (in hundreths of millimeters).
        g_PageSetupDlg.Flags |= PSD_INHUNDREDTHSOFMILLIMETERS;
        g_PageSetupDlg.rtMargin.top    = 2500;
        g_PageSetupDlg.rtMargin.bottom = 2500;
        g_PageSetupDlg.rtMargin.left   = 2000;
        g_PageSetupDlg.rtMargin.right  = 2000;
    }

}

/* Standard window size proc */
void NPSize (int cxNew, int cyNew)
{
    /* Invalidate the edit control window so that it is redrawn with the new
     * margins. Needed when comming up from iconic and when doing word wrap so
     * the new margins are accounted for.
     */
    InvalidateRect(hwndEdit, (LPRECT)NULL, TRUE);
    MoveWindow( hwndEdit, 0, 0, cxNew, cyNew, TRUE );
}

// NpSaveDialogHookProc
//
// Common dialog hook procedure for handling
// the file type while saving.
//

const DWORD s_SaveAsHelpIDs[]=
    {
        IDC_FILETYPE, IDH_FILETYPE,
        IDC_ENCODING, IDH_FILETYPE,
        0, 0
    };

UINT_PTR APIENTRY NpSaveDialogHookProc(
    HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    INT id;
    POINT pt;
    TCHAR* szSelect;        // selected type

    switch( msg )
    {
        case WM_INITDIALOG:
            // Warning: the order here must be the same as NP_FILETYPE

            SendDlgItemMessage(hWnd, IDC_FILETYPE,CB_ADDSTRING, 0, (LPARAM) szFtAnsi );
            SendDlgItemMessage(hWnd, IDC_FILETYPE,CB_ADDSTRING, 0, (LPARAM) szFtUnicode );
            SendDlgItemMessage(hWnd, IDC_FILETYPE,CB_ADDSTRING, 0, (LPARAM) szFtUnicodeBe );
            SendDlgItemMessage(hWnd, IDC_FILETYPE,CB_ADDSTRING, 0, (LPARAM) szFtUtf8 );

            szSelect= szFtAnsi;         // default
            g_ftSaveAs= g_ftOpenedAs;   // default: save as same type as opened
            switch( g_ftSaveAs )
            {
                case FT_UNICODE:   szSelect= szFtUnicode;   break;
                case FT_UNICODEBE: szSelect= szFtUnicodeBe; break;
                case FT_UTF8:      szSelect= szFtUtf8;      break;
                default: break;
            }

            SendDlgItemMessage( hWnd, IDC_FILETYPE, CB_SELECTSTRING, (WPARAM) -1, (LPARAM)szSelect );
            break;

        case WM_COMMAND:
            g_ftSaveAs= (NP_FILETYPE) SendDlgItemMessage( hWnd, IDC_FILETYPE, CB_GETCURSEL, 0, 0 );
            break;

        case WM_HELP:
            //
            //  We only want to intercept help messages for controls that we are
            //  responsible for.
            //

            id = GetDlgCtrlID(((LPHELPINFO) lParam)-> hItemHandle);

            if ( id != IDC_FILETYPE && id != IDC_ENCODING)
                break;

            WinHelp(((LPHELPINFO) lParam)-> hItemHandle,
                      szHelpFile,
                      HELP_WM_HELP,
                      (ULONG_PTR) s_SaveAsHelpIDs);
            return TRUE;

        case WM_CONTEXTMENU:
            //
            //  If the user clicks on any of our labels, then the wParam will
            //  be the hwnd of the dialog, not the static control.  WinHelp()
            //  handles this, but because we hook the dialog, we must catch it
            //  first.
            //
            if( hWnd == (HWND) wParam )
            {
                GetCursorPos(&pt);
                ScreenToClient(hWnd, &pt);
                wParam = (WPARAM) ChildWindowFromPoint(hWnd, pt);
            }

            //
            //  We only want to intercept help messages for controls that we are
            //  responsible for.
            //

            id = GetDlgCtrlID((HWND) wParam);

            if ( id != IDC_FILETYPE && id != IDC_ENCODING)
                break;

            WinHelp( (HWND)   wParam,
                              szHelpFile,
                              HELP_CONTEXTMENU,
                      (ULONG_PTR) s_SaveAsHelpIDs);
            return TRUE;
    }
    return( FALSE );
}
/* ** Notepad command proc - called whenever notepad gets WM_COMMAND
      message.  wParam passed as cmd */
INT NPCommand(
    HWND     hwnd,
    WPARAM   wParam,
    LPARAM   lParam )
{
    HWND     hwndFocus;
    LONG     lSel;
    TCHAR    szNewName[MAX_PATH] = TEXT("");      /* New file name */
    FARPROC  lpfn;
    LONG     style;
    DWORD    rc;

    switch (LOWORD(wParam))
    {
        case M_EXIT:
            PostMessage(hwnd, WM_CLOSE, 0, 0L);
            break;

        case M_NEW:
            New(TRUE);
            break;

        case M_OPEN:
            if (CheckSave(FALSE))
            {
                /* set up the variable fields of the OPENFILENAME struct.
                 * (the constant fields have been set in NPInit()
                 */
                OFN.lpstrFile         = szNewName;
                lstrcpy(szNewName, TEXT("*.txt") ); /* set default selection */
                OFN.lpstrTitle        = szOpenCaption;

                /* ALL non-zero long pointers must be defined immediately
                 * before the call, as the DS might move otherwise.
                 * 12 February 1991    clarkc
                 */
                OFN.lpstrFilter       = szOpenFilterSpec;
                OFN.lpstrDefExt       = TEXT("txt");
                /* Added OFN_FILEMUSTEXIST to eliminate problems in LoadFile.
                 * 12 February 1991    clarkc
                 */
                OFN.Flags          = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
                OFN.nFilterIndex   = FILE_TEXT;
                OFN.lpTemplateName = NULL;
                OFN.lpfnHook       = NULL;

                if( GetOpenFileName( (LPOPENFILENAME)&OFN ) )
                {
                   HANDLE oldfp= fp;

                   fp= CreateFile( szNewName,            // filename
                                   GENERIC_READ,         // access mode
                                   FILE_SHARE_READ|FILE_SHARE_WRITE,
                                   NULL,                 // security descriptor
                                   OPEN_EXISTING,        // how to create
                                   FILE_ATTRIBUTE_NORMAL,// file attributes
                                   NULL);                // hnd to file attrs
                   /* Try to load the file and reset fp if failed */
                   if( !LoadFile( szNewName, FT_UNKNOWN ) )
                   {
                      fp= oldfp;
                   }
                }
                else
                {
                    SignalCommDlgError();
                }
            }
            break;

        case M_SAVE:
            /* set up the variable fields of the OPENFILENAME struct.
             * (the constant fields have been sel in NPInit()
             */
            g_ftSaveAs = g_ftOpenedAs;
            if( !fUntitled && SaveFile( hwndNP, szFileName, FALSE ) )
               break;

            /* fall through */

        case M_SAVEAS:

            OFN.lpstrFile       = szNewName;
            OFN.lpstrTitle      = szSaveCaption;
            /* Added OFN_PATHMUSTEXIST to eliminate problems in SaveFile.
             * 12 February 1991    clarkc
             */
            OFN.Flags = OFN_HIDEREADONLY     | OFN_OVERWRITEPROMPT |
                        OFN_NOREADONLYRETURN | OFN_PATHMUSTEXIST   |
                        OFN_EXPLORER         |
                        OFN_ENABLESIZING     |
                        OFN_ENABLETEMPLATE   | OFN_ENABLEHOOK;

            OFN.lpTemplateName= TEXT("NpSaveDialog");
            OFN.lpfnHook= NpSaveDialogHookProc;

            /* ALL non-zero long pointers must be defined immediately
             * before the call, as the DS might move otherwise.
             * 12 February 1991    clarkc
             */
            OFN.lpstrFilter       = szSaveFilterSpec;
            OFN.lpstrDefExt       = TEXT("txt");

            if (!fUntitled)
            {
                lstrcpyn(szNewName, szFileName, MAX_PATH); /* set default selection */
                szNewName[MAX_PATH]= TEXT('\0');
            }
            else
            {
                lstrcpy (szNewName, TEXT("*.txt") );
            }

            fInSaveAsDlg = TRUE;

            OFN.nFilterIndex= FILE_TEXT;

            //
            // Do common dialog to save file
            //


            if (GetSaveFileName(&OFN))
            {

               if( SaveFile(hwnd, szNewName, TRUE) )
               {
                  lstrcpyn( szFileName, szNewName, MAX_PATH);
                  szFileName[MAX_PATH]= TEXT('\0');
                  g_ftOpenedAs= g_ftSaveAs;
               }
            }
            else
            {
                SignalCommDlgError();
            }

            fInSaveAsDlg = FALSE;
            break;

        case M_SELECTALL:
            {
                HMENU    hMenu;

                hMenu = GetMenu(hwndNP);
                lSel = (LONG) SendMessage (hwndEdit, WM_GETTEXTLENGTH, 0, 0L);
                SendMessage (hwndEdit, EM_SETSEL, 0, lSel );
                SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);
                EnableMenuItem(GetSubMenu(hMenu, 1), M_SELECTALL, MF_GRAYED);
                break;
            }

        case M_REPLACE:
            if( hDlgFind )
            {
               SetFocus( hDlgFind );
            }
            else
            {
               FR.Flags= FR_HIDEWHOLEWORD | FR_REPLACE;
               FR.lpstrReplaceWith= szReplace;
               FR.wReplaceWithLen= CCHKEYMAX;
               FR.lpstrFindWhat = szSearch;
               FR.wFindWhatLen  = CCHKEYMAX;
               hDlgFind = ReplaceText( &FR );
            }
            break;

        case M_FINDNEXT:
            if (szSearch[0])
            {
               Search(szSearch);
               break;
            }
            /* else fall thro' a,d bring up "find" dialog */

        case M_FIND:
            if (hDlgFind)
            {
               SetFocus(hDlgFind);
            }
            else
            {
               FR.Flags= FR_DOWN | FR_HIDEWHOLEWORD;
               FR.lpstrReplaceWith= NULL;
               FR.wReplaceWithLen= 0;
               FR.lpstrFindWhat = szSearch;
               FR.wFindWhatLen  = CCHKEYMAX;
               hDlgFind = FindText((LPFINDREPLACE)&FR);
            }
            break;

        case M_GOTO:
            {
                INT  Result;

                Result= (INT)DialogBox( hInstanceNP,
                                        MAKEINTRESOURCE(IDD_GOTODIALOG),
                                        hwndNP,
                                        GotoDlgProc );

                //
                // move cursor only if ok pressed and line number ok
                //

                if( Result == 0 )
                {
                    UINT CharIndex;
                    CharIndex= (UINT) SendMessage( hwndEdit,
                                                   EM_LINEINDEX,
                                                   lGotoLine-1,
                                                   0 );
                    if( CharIndex != (UINT) -1 )
                    {
                        SendMessage( hwndEdit, EM_SETSEL, CharIndex, CharIndex);
                        SendMessage( hwndEdit, EM_SCROLLCARET, 0, 0 );
                    }
                }
            }
            break;

        case M_ABOUT:
            ShellAbout(hwndNP,
                       szNN,
                       TEXT(""),
                       LoadIcon(hInstanceNP,
                                (LPTSTR)MAKEINTRESOURCE(ID_ICON)));

            break;

        case M_HELP:
            HtmlHelpA(GetDesktopWindow(), "notepad.chm", HH_DISPLAY_TOPIC, 0L);
            break;

        case M_CUT:
        case M_COPY:
        case M_CLEAR:
            lSel = (LONG)SendMessage (hwndEdit, EM_GETSEL, 0, 0L);
            if (LOWORD(lSel) == HIWORD(lSel))
               break;

        case M_PASTE:
            /* If notepad parent or edit window has the focus,
               pass command to edit window.
               make sure line resulting from paste will not be too long. */
            hwndFocus = GetFocus();
            if (hwndFocus == hwndEdit || hwndFocus == hwndNP)
            {
                PostMessage(hwndEdit, LOWORD(wParam), 0, 0);
            }
            break;

        case M_DATETIME:
            InsertDateTime(FALSE);
            break;

        case M_UNDO:
            SendMessage (hwndEdit, EM_UNDO, 0, 0L);
            break;

        case M_WW:
            style= (!fWrap) ? ES_STD : (ES_STD | WS_HSCROLL);
            if( NpReCreate( style ) )
            {
                fWrap= !fWrap;
            }
            else
            {
                MessageBox(hwndNP, szNoWW, szNN,
                           MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
            }
            break;

        case ID_EDIT:
            break;

        case M_PRINT:
            PrintIt( UseDialog );
            break;

        case M_PAGESETUP:
            TryPrintDlgAgain:
            
            if( PageSetupDlg(&g_PageSetupDlg) )
            {
                //  We know it's okay to copy these strings over...
                lstrcpy(chPageText[HEADER], chPageTextTemp[HEADER]);
                lstrcpy(chPageText[FOOTER], chPageTextTemp[FOOTER]);
            }
            else
            {
                rc= CommDlgExtendedError();

                if( rc == PDERR_PRINTERNOTFOUND ||
                    rc == PDERR_DNDMMISMATCH    ||
                    rc == PDERR_DEFAULTDIFFERENT )
                  {
                      FreeGlobal();
                      g_PageSetupDlg.hDevMode= g_PageSetupDlg.hDevNames= 0;
                      goto TryPrintDlgAgain;
                  }

                // Check for Dialog Failure

                SignalCommDlgError( );

            }
            break;

        case M_SETFONT:
        {
            CHOOSEFONT  cf;
            HFONT       hFontNew;
            HDC         hDisplayDC;     // display DC

            hDisplayDC= GetDC(NULL);    // try to get display DC
            if( !hDisplayDC )
                break;

            /* calls the font chooser (in commdlg)
             * We set lfHeight; choosefont returns ipointsize
             */
            cf.lStructSize = sizeof(CHOOSEFONT);
            cf.hwndOwner = hwnd;
            cf.lpLogFont = &FontStruct;         // filled in by init
            FontStruct.lfHeight= -MulDiv(iPointSize,GetDeviceCaps(hDisplayDC,LOGPIXELSY),720);
            cf.Flags = CF_INITTOLOGFONTSTRUCT |
                       CF_SCREENFONTS         | 
                       CF_NOVERTFONTS         |
                       0;
            cf.rgbColors = 0;                   // only if cf_effects
            cf.lCustData = 0;                   // for hook function
            cf.lpfnHook = (LPCFHOOKPROC) NULL;
            cf.lpTemplateName = (LPTSTR) NULL;
            cf.hInstance = NULL;
            cf.lpszStyle = NULL;                // iff cf_usestyle
            cf.nFontType = SCREEN_FONTTYPE;
            cf.nSizeMin  = 0;  // iff cf_limitsize
            cf.nSizeMax  = 0;  // iff cf_limitsize
            ReleaseDC( NULL, hDisplayDC );

            if( ChooseFont(&cf) )
            {
                SetCursor( hWaitCursor );        // may take some time
                
                hFontNew= CreateFontIndirect(&FontStruct);
                if( hFontNew )
                {
                   DeleteObject( hFont );
                   hFont= hFontNew;
                   SendMessage( hwndEdit, WM_SETFONT,
                               (WPARAM)hFont, MAKELPARAM(TRUE, 0));
                   iPointSize= cf.iPointSize;  // remember for printer
                }
                SetCursor( hStdCursor );
            }
            break;
        }

        default:
            return FALSE;
    }
    return TRUE;
}


// bugbug
// for some reason, this procedure tries to maintain
// a valid 'fp' even though I believe it does not need
// to be.
// bugbug

void FileDragOpen(void)
{
    HANDLE oldfp;

    oldfp= fp;       // remember in case of error

    if( CheckSave(FALSE) )
    {

         fp= CreateFile( szPath,               // filename
                         GENERIC_READ,         // access mode
                         FILE_SHARE_READ|FILE_SHARE_WRITE,
                         NULL,                 // security descriptor
                         OPEN_EXISTING,        // how to create
                         FILE_ATTRIBUTE_NORMAL,// file attributes
                         NULL);                // hnd to file attrs

       if( fp == INVALID_HANDLE_VALUE )
       {
          AlertUser_FileFail( szPath );

          // Restore fp to original file.
          fp= oldfp;
       }
       /* Try to load the file and reset fp if failed */
       else if( !LoadFile( szPath, FT_UNKNOWN ) )
       {
           fp= oldfp;
       }
    }
}


/* Proccess file drop/drag options. */
void doDrop (WPARAM wParam, HWND hwnd)
{
   /* If user dragged/dropped a file regardless of keys pressed
    * at the time, open the first selected file from file manager. */

    if (DragQueryFile ((HANDLE)wParam, 0xFFFFFFFF, NULL, 0)) /* # of files dropped */
    {
       DragQueryFile ((HANDLE)wParam, 0, szPath, CharSizeOf(szPath));
       SetActiveWindow (hwnd);
       FileDragOpen();
    }
    DragFinish ((HANDLE)wParam);  /* Delete structure alocated for WM_DROPFILES*/
}

/* ** if notepad is dirty, check to see if user wants to save contents */
BOOL FAR CheckSave (BOOL fSysModal)
{
    INT    mdResult = IDOK;
    TCHAR  szNewName[MAX_PATH] = TEXT("");      /* New file name */
    TCHAR *pszFileName;

/* If it's untitled and there's no text, don't worry about it */
    if (fUntitled && !SendMessage (hwndEdit, WM_GETTEXTLENGTH, 0, (LPARAM)0))
        return (TRUE);

    if (SendMessage (hwndEdit, EM_GETMODIFY, 0, 0L))
    {
       if( fUntitled )
           pszFileName= szUntitled;
       else
           pszFileName= szFileName;

       mdResult= AlertBox( hwndNP, szNN, szSCBC, pszFileName,
       (WORD)((fSysModal ? MB_SYSTEMMODAL :
                           MB_APPLMODAL)|MB_YESNOCANCEL|MB_ICONEXCLAMATION));
       if( mdResult == IDYES )
       {
          if( fUntitled )
          {
             lstrcpy( szNewName, TEXT("*.txt") );
SaveFilePrompt:
             OFN.lpstrFile        = szNewName;
             OFN.lpstrTitle       = szSaveCaption;

            /* Added OFN_PATHMUSTEXIST to eliminate problems in SaveFile.
             * 12 February 1991    clarkc
             */

            OFN.Flags = OFN_HIDEREADONLY     | OFN_OVERWRITEPROMPT |
                        OFN_NOREADONLYRETURN | OFN_PATHMUSTEXIST   |
                        OFN_EXPLORER         |
                        OFN_ENABLESIZING     |
                        OFN_ENABLETEMPLATE   | OFN_ENABLEHOOK;

            OFN.lpTemplateName= TEXT("NpSaveDialog");
            OFN.lpfnHook= NpSaveDialogHookProc;

            /* ALL non-zero long pointers must be defined immediately
             * before the call, as the DS might move otherwise.
             * 12 February 1991    clarkc
             */
             OFN.lpstrFilter       = szSaveFilterSpec;
             OFN.lpstrDefExt       = TEXT("txt");

             OFN.nFilterIndex= FILE_TEXT;

             //
             // Set dialog checkmark by current file type
             //

             fInSaveAsDlg = TRUE;
             if (GetSaveFileName(&OFN))
             {
                // since SaveFile() uses near ptr to name (obs.)
                lstrcpy(szNewName, OFN.lpstrFile);
                if( SaveFile(hwndNP, szNewName, TRUE) )
                {
                   lstrcpy(szFileName, szNewName);
                   g_ftOpenedAs= g_ftSaveAs;
                }
                else
                {      // Fixing close without saving file when disk-full
                   lstrcpy(szNewName, szFileName);
                   goto SaveFilePrompt;
                }
             }
             else
             {
                mdResult= IDCANCEL;       /* Don't exit Program */
                if( CommDlgExtendedError() )/* Dialog box failed, Lo-mem*/
                   DeepTrouble();
             }

             fInSaveAsDlg = FALSE;
          }
          else
          {
            // initialize the save type.
             g_ftSaveAs = g_ftOpenedAs;
             if( SaveFile(hwndNP, szFileName, FALSE) )
                return(TRUE);
             lstrcpy(szNewName, szFileName);
             goto SaveFilePrompt;
          }
       }
    }
    return (mdResult != IDCANCEL);
}


/* Notepad window class procedure */
LRESULT FAR NPWndProc(
        HWND       hwnd,
        UINT       message,
        WPARAM     wParam,
        LPARAM     lParam)
{
    RECT rc;
    LPFINDREPLACE lpfr;
    DWORD dwFlags;
    HANDLE hMenu;



    switch (message)
    {
/* If we're being run by Setup and it's the system menu, be certain that
 * the minimize menu item is disabled.  Note that hSysMenuSetup is only
 * initialized if Notepad is being run by Setup.  Don't use it outside
 * the fRunBySetup conditional!    28 June 1991    Clark Cyr
 */
        case WM_INITMENUPOPUP:
            if (fRunBySetup && HIWORD(lParam))
               EnableMenuItem(hSysMenuSetup,SC_MINIMIZE,MF_GRAYED|MF_DISABLED);
            break;

        case WM_SYSCOMMAND:
            if (fRunBySetup)
            {
                /* If we have been spawned by SlipUp we need to make sure the
                 * user doesn't minimize us or alt tab/esc away.
                 */
                if (wParam == SC_MINIMIZE ||
                    wParam == SC_NEXTWINDOW ||
                    wParam == SC_PREVWINDOW)
                    break;
            }
            DefWindowProc(hwnd, message, wParam, lParam);
            break;

        case WM_SETFOCUS:
            if (!IsIconic(hwndNP))
            {
               SetFocus(hwndEdit);
            }
            break;

        case WM_KILLFOCUS:
            SendMessage (hwndEdit, message, wParam, lParam);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_CLOSE:

            // Save any globals in the registry if need be

            SaveGlobals();

            if (CheckSave(FALSE))
            {
                /* Exit help */
                if(!WinHelp(hwndNP, (LPTSTR)szHelpFile, HELP_QUIT, 0))
                    DeepTrouble();

                DestroyWindow(hwndNP);
                DeleteObject(hFont);
            }

            break;

        case WM_QUERYENDSESSION:
            if (fInSaveAsDlg)
            {
                MessageBeep (0);
                MessageBeep (0);
                MessageBox (hwndNP, szCannotQuit, szNN, MB_OK|MB_SYSTEMMODAL);
                return FALSE;
            }
            else
                return (CheckSave(TRUE));
            break;


        case WM_ACTIVATEAPP:
            if (wParam)
            {
            /* This causes the caret position to be at the end of the selection
             * but there's no way to ask where it was or set it if known.  This
             * will cause a caret change when the selection is made from bottom
             * to top.
             */
                if( dwCurrentSelectionStart != 0 || dwCurrentSelectionEnd != 0 )
                {
                   SendMessage( hwndEdit, EM_SETSEL,
                                dwCurrentSelectionStart,
                                dwCurrentSelectionEnd );
                   SendMessage( hwndEdit, EM_SCROLLCARET, 0, 0 );
                }
            }
            else
            {
                SendMessage( hwndEdit, EM_GETSEL,
                            (WPARAM) &dwCurrentSelectionStart,
                            (LPARAM) &dwCurrentSelectionEnd);
                if (dwCurrentSelectionStart == dwCurrentSelectionEnd)
                {
                    dwCurrentSelectionStart = 0L;
                    dwCurrentSelectionEnd = 0L;
                }
                else
                {
                   SendMessage (hwndEdit, EM_SETSEL, dwCurrentSelectionStart,
                                dwCurrentSelectionEnd);
                   SendMessage (hwndEdit, EM_SCROLLCARET, 0, 0);
                }
            }
            break;

        case WM_ACTIVATE:
            if ((LOWORD(wParam) == WA_ACTIVE       ||
                 LOWORD(wParam) == WA_CLICKACTIVE) &&
                !IsIconic(hwndNP)
               )
               {
                   // active doesn't always mean foreground (ntbug# 53048)
                   if( GetForegroundWindow() == hwndNP )
                   {
                       SetFocus(GetForegroundWindow());
                   }
               }
            break;

        case WM_SIZE:
            switch (wParam)
            {
                case SIZENORMAL:
                case SIZEFULLSCREEN:
                    NPSize(MAKEPOINTS(lParam).x, MAKEPOINTS(lParam).y);
                    break;

                case SIZEICONIC:
                    return (DefWindowProc(hwnd, message, wParam, lParam));
                    break;
                }
            break;

        case WM_INITMENU:
            NpResetMenu( hwnd );
            break;

        //
        // Some keyboards come with a "Search" button which the shell team
        // wanted us to handle.  See ntbug# 380067
        //

        case WM_APPCOMMAND:

            if( ( GET_APPCOMMAND_LPARAM(lParam) == APPCOMMAND_BROWSER_SEARCH ) ) 
            {
                NPCommand(hwnd, M_FIND, 0);
                break;
            }
            // otherwise fall through
 
        case WM_COMMAND:
            if ((HWND)(lParam) == hwndEdit &&
                (HIWORD(wParam) == EN_ERRSPACE ||
                 HIWORD(wParam) == EN_MAXTEXT))
            {
                if (dwEmSetHandle == SETHANDLEINPROGRESS)
                    dwEmSetHandle = SETHANDLEFAILED;
                else
                    DeepTrouble();
                return 0L;
            }

            if (!NPCommand(hwnd, wParam, lParam))
               return (DefWindowProc(hwnd, message, wParam, lParam));
            break;

        case WM_WININICHANGE:
            // Ignore for now.
            // If you put this back in, be sure it handles both
            // the metric change and the decimal change.
            //NpWinIniChange ();
            break;

        case WM_DROPFILES: /*case added 03/26/91 for file drag/drop support*/
            doDrop (wParam,hwnd);
            break;

        case PWM_CHECK_HKL: /* private message: corresponding to HKL change message */
            {
                LPARAM lParam = 0;

                if (PRIMARYLANGID(LOWORD((DWORD) (INT_PTR) GetKeyboardLayout(0))) == LANG_JAPANESE) {
                    /*
                     * If new current HKL is Japanese, handle the result string at once.
                     */
                    lParam = EIMES_GETCOMPSTRATONCE;
                }
                SendMessage(hwndEdit, EM_SETIMESTATUS, EMSIS_COMPOSITIONSTRING, lParam);
            }
            break;

        default:
            /* this can be a message from the modeless Find Text window */
            if (message == wFRMsg)
            {
                BOOL bStatus;    // true if found text

                lpfr = (LPFINDREPLACE)lParam;
                dwFlags = lpfr->Flags;

                fReverse = (dwFlags & FR_DOWN      ? FALSE : TRUE);
                fCase    = (dwFlags & FR_MATCHCASE ? TRUE  : FALSE);

                if( dwFlags & FR_FINDNEXT )
                {
                    SetCursor( hWaitCursor );
                    Search( szSearch );
                    SetCursor( hStdCursor );
                }
                else if( dwFlags & FR_REPLACE )
                {
                    //
                    // Replace current selection if it matches
                    // then highlight the next occurence of the string.
                    //

                    SetCursor( hWaitCursor );
                    ReplaceSel( TRUE );
                    Search( szSearch );
                    SetCursor( hStdCursor );
                }
                else if( dwFlags & FR_REPLACEALL )
                {
                   //
                   // The replace dialog doesn't allow reverse searches
                   // but just it cases it changes, for it to false.
                   //
                   if( fReverse )
                   {
                       fReverse= FALSE;
                   }

                   //
                   // Replace all occurances of text in the file
                   // starting from the top.  Reset the selection
                   // to the top of the file.
                   //
                   SetCursor( hWaitCursor );
                   SendMessage( hwndEdit, EM_SETSEL, 0, 0 );
                   do
                   {
                      ReplaceSel( FALSE );
                      bStatus= Search( szSearch );
                   }
                   while( bStatus );
                   SetCursor( hStdCursor );
                   //
                   // back to the top of the file.
                   //
                   SendMessage( hwndEdit, EM_SETSEL, 0, 0 );
                   SendMessage( hwndEdit, EM_SCROLLCARET, 0, 0);

                }
                else if (dwFlags & FR_DIALOGTERM)
                    hDlgFind = NULL;   /* invalidate modeless window handle */
                break;
            }
            return (DefWindowProc(hwnd, message, wParam, lParam));
    }
    return (0L);
}

LPTSTR SkipProgramName (LPTSTR lpCmdLine)
{
    LPTSTR  p = lpCmdLine;
    BOOL    bInQuotes = FALSE;

    //
    // Skip executable name
    //
    for (p; *p; p = CharNext(p))
    {
       if ((*p == TEXT(' ') || *p == TEXT('\t')) && !bInQuotes)
          break;

       if (*p == TEXT('\"'))
          bInQuotes = !bInQuotes;
    }

    while (*p == TEXT(' ') || *p == TEXT('\t'))
       p++;

    return (p);
}

/* ** Main loop */

INT WINAPI WinMain(
   HINSTANCE hInstance,
   HINSTANCE hPrevInstance,
   LPSTR lpAnsiCmdLine,
   INT cmdShow)
{
    MSG msg;
    VOID (FAR PASCAL *lpfnRegisterPenApp)(WORD, BOOL) = NULL;
    LPTSTR lpCmdLine = GetCommandLine ();


/* PenWindow registration must be before creating an edit class window.
 * Moved here, along with goto statement below for appropriate cleanup.
 *                 10 July 1991    ClarkC
 */
    if ((FARPROC) lpfnRegisterPenApp = GetProcAddress((HINSTANCE)(INT_PTR)(GetSystemMetrics(SM_PENWINDOWS)),
        "RegisterPenApp"))
        (*lpfnRegisterPenApp)(1, TRUE);

    if (!NPInit(hInstance, hPrevInstance, SkipProgramName (lpCmdLine), cmdShow))
    {
       msg.wParam = FALSE;
       goto UnRegisterPenWindows;
    }

    while (GetMessage((LPMSG)&msg, (HWND)NULL, 0, 0))
    {
        //
        // To handle IME status when active KL is changed.
        //
        if (msg.message == WM_INPUTLANGCHANGEREQUEST) {
            //
            // WM_INPUTLANGCHANGE will be *sent* to WndProc,
            // so there's no chance to catch WM_INPUTLANGCHANGE from the frame window.
            // Instead, we post the private message to check the active HKL later.
            //
            PostMessage(hwndNP, PWM_CHECK_HKL, 0, 0);
        }

        if (!hDlgFind || !IsDialogMessage(hDlgFind, &msg))
        {
            if (TranslateAccelerator(hwndNP, hAccel, (LPMSG)&msg) == 0)
            {
               TranslateMessage ((LPMSG)&msg);
               DispatchMessage ((LPMSG)&msg);
            }
        }
    }

    /* Clean up any global allocations */

    FreeGlobal();

    LocalFree( hEdit );

UnRegisterPenWindows:

    if (lpfnRegisterPenApp)
        (*lpfnRegisterPenApp)(1, FALSE);

    return (int)(msg.wParam);
}

/* ** Set Window caption text */
void FAR SetTitle( TCHAR  *sz )
{
    TCHAR    szWindowText[MAX_PATH+50];
    TCHAR    szFileName[MAX_PATH];
    HANDLE   hFindFile;
    WIN32_FIND_DATA info;

    // if "untitled" then don't do all this work...
    if( lstrcmp( sz, szUntitled ) == 0 )
       lstrcpy( szWindowText, sz );
    else
    {
       // Get real(file system) name for the file.
       hFindFile= FindFirstFile( sz, &info );

       if( hFindFile != INVALID_HANDLE_VALUE )
       {
          lstrcpy( szFileName, info.cFileName );
          FindClose( hFindFile );
       }
       else
          lstrcpy( szFileName, sz );

       GetFileTitle(szFileName, szWindowText, MAX_PATH);
    }

    lstrcat(szWindowText, szNpTitle);
    SetWindowText(hwndNP, (LPTSTR)szWindowText);
}

/* ** Given filename which may or maynot include path, return pointer to
      filename (not including path part.) */
LPTSTR PASCAL far PFileInPath(
    LPTSTR sz)
{
    LPTSTR pch = sz;
    LPTSTR psz;

    /* Strip path/drive specification from name if there is one */
    /* Ripped out AnsiPrev calls.     21 March 1991  clarkc     */
    for (psz = sz; *psz; psz = CharNext(psz))
      {
        if ((*psz == TEXT(':')) || (*psz == TEXT('\\')))
            pch = psz;
      }

    if (pch != sz)   /* If found slash or colon, return the next character */
        pch++;       /* increment OK, pch not pointing to DB character     */

    return(pch);
}

/* ** Enable or disable menu items according to selection state
      This routine is called when user tries to pull down a menu. */

VOID NpResetMenu( HWND hwnd )
{
    LONG    lsel;
    INT     mfcc;   /* menuflag for cut, copy */
    BOOL    fCanUndo;
    HANDLE  hMenu;
    TCHAR   msgbuf[20];
    BOOL    fPaste= FALSE;
    UINT    uSelState;

    hMenu = GetMenu(hwndNP);

    // cut, copy and delete only get enabled if there is text selected.

    lsel = (LONG)SendMessage(hwndEdit, EM_GETSEL, 0, 0L);
    mfcc = LOWORD(lsel) == HIWORD(lsel) ? MF_GRAYED : MF_ENABLED;
    EnableMenuItem(GetSubMenu(hMenu, 1), M_CUT, mfcc);
    EnableMenuItem(GetSubMenu(hMenu, 1), M_COPY, mfcc);
    EnableMenuItem(GetSubMenu(hMenu, 1), M_CLEAR, mfcc);

    // check if the selectall is gray (that means the user has already
    // done select-all) and it the user has deselected - if so, time
    // to re-enable selectall menu.

    uSelState = GetMenuState(GetSubMenu(hMenu, 1), M_SELECTALL, MF_BYCOMMAND);
    if ((uSelState == MF_GRAYED) && (mfcc == MF_GRAYED))
    {
        EnableMenuItem(GetSubMenu(hMenu, 1), M_SELECTALL, MF_ENABLED);
    }

    // paste is enabled if there is text in the clipboard

    if( OpenClipboard(hwnd) )
    {
        fPaste= IsClipboardFormatAvailable(CF_TEXT);
        CloseClipboard();
    }
    EnableMenuItem(GetSubMenu(hMenu, 1), M_PASTE, fPaste ? MF_ENABLED : MF_GRAYED);

    // enable Undo only if editcontrol says we can do it.

    fCanUndo = (BOOL) SendMessage(hwndEdit, EM_CANUNDO, 0, 0L);
    EnableMenuItem(GetSubMenu(hMenu, 1), M_UNDO, fCanUndo ? MF_ENABLED : MF_GRAYED);

    // check the word wrap item correctly

    CheckMenuItem(GetSubMenu(hMenu, 2), M_WW, fWrap ? MF_CHECKED : MF_UNCHECKED);

}


void FAR NpWinIniChange(VOID)
{
   InitLocale ();
}

/* ** Scan sz1 for merge spec.    If found, insert string sz2 at that point.
      Then append rest of sz1 NOTE! Merge spec guaranteed to be two chars.
      returns TRUE if it does a merge, false otherwise. */
BOOL MergeStrings(
    TCHAR    *szSrc,
    TCHAR    *szMerge,
    TCHAR    *szDst)
    {
    register    TCHAR *pchSrc;
    register    TCHAR *pchDst;

    pchSrc = szSrc;
    pchDst = szDst;

    /* Find merge spec if there is one. */
    while ( *pchSrc != chMerge)
        {
        *pchDst++ = *pchSrc;

        /* If we reach end of string before merge spec, just return. */
        if (!*pchSrc++)
            return FALSE;

        }

    /* If merge spec found, insert sz2 there. (check for null merge string */
    if (szMerge)
        {
        while (*szMerge)
            *pchDst++ = *szMerge++;
        }

    /* Jump over merge spec */
    pchSrc++,pchSrc++;

    /* Now append rest of Src String */
    while (*pchDst++ = *pchSrc++);
    return TRUE;

    }

/* ** Post a message box */
INT FAR AlertBox(
    HWND    hwndParent,
    TCHAR    *szCaption,
    TCHAR    *szText1,
    TCHAR    *szText2,
    UINT     style)
{
    INT iResult;                      // result of function
    INT iAllocSize;                   // size needed for message
    TCHAR*  pszMessage;               // combined message

    // Allocate a message buffer assuming there will be a merge.
    // If we cannot do the allocation, tell the user something
    // related to the original problem. (not the allocation failure)
    // Then pray that MessageBox can get enough memory to actually work.

    iAllocSize= (lstrlen(szText1) + (szText2 ? lstrlen(szText2) : 0) + 1 ) * sizeof(TCHAR);

    pszMessage= (TCHAR*) LocalAlloc( LPTR, iAllocSize );

    if( pszMessage )
    {
        MergeStrings( szText1, szText2, pszMessage );
        iResult= MessageBox( hwndParent, pszMessage, szCaption, style );
        LocalFree( (HLOCAL) pszMessage );
    }
    else
    {
        iResult= MessageBox( hwndParent, szText1, szCaption, style );
    }

    return( iResult );
}

// SignalCommDlgError
//
// If a common dialog error occurred, put up reasonable message box.
//
// returns: TRUE if error occurred, FALSE if no error.
//

typedef struct tagMAPERROR
{
    DWORD   rc;            // return code from CommDlgExtendedError()
    PTCHAR* ppszMsg;       // text of message pointer
} MAPERROR;

// errors not in this list get generic "common dialog error %x" message.
static TCHAR* szNull= TEXT("");

MAPERROR maperror[]=
{
    CDERR_DIALOGFAILURE,  &szErrSpace,
    CDERR_INITIALIZATION, &szCommDlgInitErr,
    CDERR_MEMLOCKFAILURE, &szPDIE,
    CDERR_LOADSTRFAILURE, &szErrSpace,
    CDERR_FINDRESFAILURE, &szErrSpace,
    PDERR_LOADDRVFAILURE, &szLoadDrvFail,
    PDERR_GETDEVMODEFAIL, &szErrSpace,
    PDERR_NODEFAULTPRN,   &szNull,          // don't report; common dialog does already
};

BOOL SignalCommDlgError(VOID)
{
    DWORD rc;               // return code
    TCHAR* pszMsg;          // message
    INT    i;
    TCHAR  szBuf[200];      // just for common dialog failure

    rc= CommDlgExtendedError();

    // no failure - just return

    if( rc == 0 )
    {
        return FALSE;
    }

    // some sort of error - pick up message

    pszMsg= NULL;
    for( i=0; i< sizeof(maperror)/sizeof(maperror[0]); i++ )
    {
        if( rc == maperror[i].rc )
        {
            pszMsg= *maperror[i].ppszMsg;
        }
    }

    // if no known mapping - tell user the actual return code
    // this may be a bit confusing, but rare hopefully.

    if( !pszMsg )
    {
        wsprintf( szBuf, szCommDlgErr, rc );   // fill in error code
        pszMsg= szBuf;
    }

    // popup if there is any message to give user

    if( *pszMsg )
    {
        MessageBox(hwndNP, pszMsg, szNN, MB_SYSTEMMODAL|MB_OK|MB_ICONHAND);
    }

    return TRUE;

}

// ReplaceSel
//
// Replace the current selection with string from FR struct
// if the current selection matches our search string.
//
// MLE will show selection if bView is true.
//


VOID ReplaceSel( BOOL bView )
{
    DWORD StartSel;    // start of selected text
    DWORD EndSel;      // end of selected text

    HANDLE hEText;
    TCHAR* pStart;
    DWORD  ReplaceWithLength;  // length of replacement string
    DWORD  FindWhatLength;

    ReplaceWithLength= lstrlen(FR.lpstrReplaceWith);
    FindWhatLength= lstrlen(FR.lpstrFindWhat);

    SendMessage( hwndEdit, EM_GETSEL, (WPARAM) &StartSel, (LPARAM) &EndSel );
    hEText= (HANDLE) SendMessage( hwndEdit, EM_GETHANDLE, 0, 0 );
    if( !hEText )  // silently return if we can't get it
    {
        return;
    }

    pStart= LocalLock( hEText );
    if( !pStart )
    {
        return;
    }

    if(  (EndSel-StartSel) == FindWhatLength )
    {
       if( (fCase &&
            !_tcsncmp(  FR.lpstrFindWhat, pStart+StartSel, FindWhatLength) ) ||
           (!fCase &&
           ( 2 == CompareString(LOCALE_USER_DEFAULT,
                  NORM_IGNORECASE | SORT_STRINGSORT | NORM_STOP_ON_NULL,
                  FR.lpstrFindWhat, FindWhatLength,
                  pStart+StartSel,  FindWhatLength ) ) ) )
        {
            SendMessage( hwndEdit, EM_REPLACESEL,
                         TRUE, (LPARAM) FR.lpstrReplaceWith);
            SendMessage( hwndEdit, EM_SETSEL,
                         StartSel, StartSel+ReplaceWithLength );

            if( bView )
            {
                SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);
            }
        }
    }

    LocalUnlock( hEText );
}

// GotoDlgProc
//
// Handle the Goto Dialog window processing
//
// Returns:
//
// 1 if successfull
// 0 if not (cancelled)
//
// Modifies global lGotoLine
//

const DWORD s_GotoHelpIDs[] = {
    IDC_GOTO, IDH_GOTO,
    0, 0
};

#define GOTOBUFSIZE 100
INT_PTR CALLBACK GotoDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    TCHAR szBuf[GOTOBUFSIZE];
    UINT LineNum;
    DWORD SelStart, SelEnd;
    POINT pt;
    INT id;

    switch (message)
    {
        //
        // initialize input field to size of file
        //
        case WM_INITDIALOG:
            SendMessage(hwndEdit,EM_GETSEL,(WPARAM) &SelStart,(WPARAM)&SelEnd);

            // the line numbers are 1 based instead 0 based. hence add 1.
            LineNum= (UINT)SendMessage( hwndEdit, EM_LINEFROMCHAR, SelStart, 0 ) + 1;
            wsprintf(szBuf, TEXT("%d"), LineNum);
            SetDlgItemText( hDlg, IDC_GOTO, szBuf );
            SetFocus( hDlg );
            return TRUE;
            break;

        // context sensitive help.
        case WM_HELP:
            WinHelp(((LPHELPINFO) lParam)-> hItemHandle, szHelpFile,
                HELP_WM_HELP, (ULONG_PTR) (LPVOID) s_GotoHelpIDs);
            break;

        case WM_CONTEXTMENU:

            WinHelp((HWND) wParam, szHelpFile, HELP_CONTEXTMENU,
                (ULONG_PTR) (LPVOID) s_GotoHelpIDs);
            break;

        case WM_COMMAND:

            switch (LOWORD(wParam))
            {
                UINT CharIndex;

                case IDC_GOTO:
                    return TRUE;
                    break;

                case IDOK:
                    GetDlgItemText( hDlg, IDC_GOTO, szBuf, GOTOBUFSIZE );

                    // convert all unicode numbers to range L'0' to L'9'

                    FoldString( MAP_FOLDDIGITS, szBuf, -1, szBuf, GOTOBUFSIZE);
                    lGotoLine= _ttol( szBuf );

                    //
                    // see if valid line number
                    //

                    CharIndex= (UINT)SendMessage( hwndEdit,
                                            EM_LINEINDEX,
                                            lGotoLine-1,
                                            0);
                    if( lGotoLine > 0 && CharIndex != -1 )
                    {
                        EndDialog(hDlg, 0);  // successfull
                        return TRUE;
                    }

                    //
                    // Invalid line number
                    // warning user and set to reasonable value
                    //

                    MessageBox( hDlg, szLineTooLarge, szLineError, MB_OK );

                    LineNum= (UINT)SendMessage( hwndEdit, EM_GETLINECOUNT, 0, 0 );
                    wsprintf(szBuf, TEXT("%d"), LineNum);
                    SetDlgItemText( hDlg, IDC_GOTO, szBuf );
                    SetFocus( hDlg );
                    break;

                case IDCANCEL :
                    EndDialog(hDlg, 1 );   // cancelled
                    return TRUE;
                    break;

                default:

                    break;

            } // switch (wParam)

            break;

       default:

             break;

    } // switch (message)


    return FALSE;     // Didn't process a message


} // GotoDlgProc()

/*******************************************************************************

Module Name:

    UCE.c

Abstract:

    This module contains the main routines for the Universal Character Explorer,
    an new interface for selecting special characters.


    Copyright (c) 1997-1999 Microsoft Corporation.
********************************************************************************/

//
//  Include Files.
//

#define WIN31
#include "windows.h"
#include <stdio.h>
#include "ole2.h"
#include <richedit.h>
#include <port1632.h>
#include "stdlib.h"
#include "tchar.h"
#ifdef UNICODE
  #include "wchar.h"
#else
  #include "stdio.h"
#endif
#include "commctrl.h"
#include "htmlhelp.h"

#include "getuname.h"
#include "winnls.h"
#include "olecomon.h"
#include "cdropsrc.h"
#include "cdataobj.h"
#include "UCE.h"
#include "ucefile.h"
//#include "oleedit.h"
#include "helpids.h"
#include "resource.h"

//
//  Macros.
//

#define FMagData(psycm)      ((psycm)->xpMagCurr != 0)
#define abs(x)               (((x) >= 0) ? (x) : (-(x)))

//
//  Constant Declarations.
//

#define STATUSPOINTSIZE      9              // point size of status bar font
#define FE_STATUSPOINTSIZE   10             // FE point size of status bar font
#define DX_BITMAP            20             // width of TT bitmap
#define DY_BITMAP            12             // height of TT bitmap

#define SEARCH_WORD_MAX      8
#define SEARCH_WORD_LEN      32

#define BACKGROUND           0x000000FF     // bright blue
#define BACKGROUNDSEL        0x00FF00FF     // bright purple

TCHAR ChmHelpPath[] = TEXT("charmap.chm");

//
//  Timer IDs
//
#define ID_SCROLLTIMER      8
#define ID_DRAGTIMER        1

// defines for hex edit control
#define ZERO        0x0030
#define MAX_CHARS   4

//
//  Debug Print Code.
//

#if 0
  TCHAR szDOUT[3] = TEXT("A\n");
  TCHAR szDbgBuf[256];
  #define DOUTL(p)     OutputDebugString(TEXT(p))
  #define DOUTCHN(ch)  if(0){}else {szDOUT[0] = ch; OutputDebugString(szDOUT);}
  #define DPRINT(p)    if(0){}else {wsprintf p; OutputDebugString(szDbgBuf);}
#else
  #define DOUTL(p)
  #define DOUTCHN(ch)
  #define DPRINT(p)
#endif

//
//  Global Variables.
//

HINSTANCE hInst;
HINSTANCE ghRichEditLib=NULL; // Module handle for rich edit library

INT    cchSymRow    = 20;     // number of characters across the character grid
INT    cchSymCol    = 10;     // number of rows in the character grid
UTCHAR chSymFirst   = 0;
UTCHAR chSymLast    = 200;
UTCHAR chRangeFirst = 0;
UTCHAR chRangeLast  = 200;
INT    chPos;
INT    chCurrPos;
BOOL   fSearched    = FALSE;
BOOL   fNeedReset   = FALSE;

SYCM   sycm;                       // tons of data need to do char grid painting
WORD   wCFRichText = 0;            // private clipboard format, rich text format
HFONT  hFontClipboard = NULL;      // tells us which font is in the clipboard
HANDLE hstrClipboard = NULL;       // contains the string which is in the clipboard
BOOL   fDelClipboardFont = FALSE;  // the clipboard font needs to be deleted

HBITMAP hbmFont = NULL;            // TT bitmap drawn before font facenames in combo

LONG   lEditSel = 0;               // contains the selection range of the EC
HBRUSH hStaticBrush;               // used for static controls during WM_CTLCOLOR
BOOL   fDisplayAdvControls = TRUE; // flag which decides whether advanced
                                   // are to be displayed
DWORD  gwExpandedHeight,           // dialog height in expanded state
       gwNormalHeight;             // dialog height when advanced controls are
                                   // hidden
BOOL   fScrolled;
WPARAM prevKeys;                   // Used in drag scrolling
LPARAM ptPrevMouse;                // Used in drag scrolling

BOOL   fSURChanged=FALSE;          // used to automatically change unicode
                                   // range when MAX_CHARS chars are typed
BOOL   fSURNeedsReset=FALSE;       // Unicode start pt has changed and reset
                                   // is needed

DWORD  gFontType;                  // for the current selected font
UINT   gKBD_CP = 0;                // codepage associated with the active keyboard
BOOL   gDisplayFontChgMsg = true;

//
//  Variables required for Drag and Drop
//
BOOL   fPendingDrag = FALSE;        // is a drag and drop operation pending
int    nDragDelay = 0;              // delay after LBUTTONDOWN after which
                                    // drag and drop starts
int    nDragMinDist = 0;            // minimum distance by which cursor has to
                                    // move before drag and drop is considered
int    nScrollInterval = 0;         // Scroll interval
int    iFromPrev=0x21, iToPrev=0xFFFD;

//
//  Useful window handles.
//
HWND hwndDialog;
HWND hwndCharGrid;
HWND hwndTT;

//
//  Data used to draw the status bar.
//
RECT rcStatusLine;                // bounding rect for status bar
INT  dyStatus;                    // height of status bar

INT   dxHelpField;                // width of help window
TCHAR szKeystrokeText[MAX_PATH];  // buffer for keystroke text
TCHAR szAlt[MAX_PATH];            // buffer for Alt+
HFONT hfontStatus;                // font used for text of status bar

TCHAR szTipText[MAX_PATH];        // Tip text


static const DWORD aHelpIDs[] = {
    ID_FONT,            IDH_UCE_FONT,
    ID_FONTLB,          IDH_UCE_FONT,
    ID_HELP,            IDH_UCE_HELPBUTTON,
    ID_CHARGRID,        IDH_UCE_GRIDCHAR,
    ID_TOPLEFT,         IDH_UCE_COPYCHAR,
    ID_STRING,          IDH_UCE_COPYCHAR,
    ID_SELECT,          IDH_UCE_SELECT,
    ID_COPY,            IDH_UCE_COPY,
    ID_ADVANCED,        IDH_UCE_ADVANCED,
    ID_VIEWLB,          IDH_UCE_CHARSET,
    ID_VIEW,            IDH_UCE_CHARSET,
    ID_URANGE,          IDH_UCE_GOTOUNICODE,
    ID_FROM,            IDH_UCE_GOTOUNICODE,
    ID_SUBSETLB,        IDH_UCE_SEARCHBYGROUP,
    ID_UNICODESUBSET,   IDH_UCE_SEARCHBYGROUP,
    ID_SEARCHNAME,      IDH_UCE_SEARCHBYNAME,
    ID_SEARCHINPUT,     IDH_UCE_SEARCHBYNAME,
    ID_SEARCH,          IDH_UCE_SEARCHRESET,
0,  0
};



static ValidateData validData[] = {
    ID_FROM, 0x0021, 0xFFFD, 0x0021, 4,
    ID_TO,   0x0021, 0xFFFD, 0xFFFD, 4
};

//
// as a display buffer
//
LPWSTR pCode=NULL;

////////////////////////////////////////////////////////////////////////////
//
//  WinMain
//
//  Calls initialization function, processes message loop, cleanup.
//
////////////////////////////////////////////////////////////////////////////

INT WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    MSG msg;

    if (!InitApplication(hInstance))
    {
        return (FALSE);
    }

    //
    //  Initialize the OLE library
    //
    if (OleInitialize(NULL) != NOERROR)
        return FALSE;

    //
    //  Load the rich edit control library
    //

    if ((ghRichEditLib = LoadLibrary(L"RICHED20.DLL")) == NULL)
        return FALSE;   // Rich edit control initialisation failed


    InitCommonControls();


    //
    //  Perform initialization for this instance.
    //
    if (!InitInstance(hInstance, nCmdShow))
    {
        return (FALSE);
    }

    while (GetMessage(&msg, NULL, 0, 0))
    {
        //
        //  Main message loop.
        //
        if (!IsDialogMessage(hwndDialog, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    //
    //  Free up some stuff.
    //
    if (hfontStatus)
    {
        DeleteObject(hfontStatus);
    }

    //
    //  Free the rich edit control library
    //
    FreeLibrary(ghRichEditLib);

    //
    //  Close the OLE library and free any resources that it maintains
    //
    OleUninitialize();

    return (INT)(msg.wParam);
}

////////////////////////////////////////////////////////////////////////////
//
//  InitApplication
//
//  Initializes window data and registers window class.
//
////////////////////////////////////////////////////////////////////////////

BOOL InitApplication(
    HINSTANCE hInstance)
{
    WNDCLASS wc;

    //
    //  Register a window class that we will use to draw the character
    //  grid into.
    //
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = CharGridWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName =  NULL;
    wc.lpszClassName = TEXT("CharGridWClass");

    if (!RegisterClass(&wc))
    {
        return (FALSE);
    }

    wc.style = 0;
    wc.lpfnWndProc = DefDlgProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = DLGWINDOWEXTRA;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDIC_UCE));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName =  NULL;
    wc.lpszClassName = TEXT("MyDlgClass");

    return RegisterClass(&wc);
}


////////////////////////////////////////////////////////////////////////////
//
//  InitInstance
//
//  Does some initialization and creates main window which is a dialog.
//
////////////////////////////////////////////////////////////////////////////

BOOL InitInstance(
    HINSTANCE   hInstance,
    INT         nCmdShow)
{
    CHARSETINFO csi;
    DWORD dw = GetACP();
/*
    LANGID PrimaryLangId = (PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale())));
    BOOL bFE = ((PrimaryLangId == LANG_JAPANESE) ||
                (PrimaryLangId == LANG_KOREAN)   ||
                (PrimaryLangId == LANG_CHINESE));
*/
    //
    //  Save the instance handle in a global variable.
    //
    hInst = hInstance;

    //
    //  This font will be used to paint the status line.
    //
    if (!TranslateCharsetInfo((DWORD*)dw, &csi, TCI_SRCCODEPAGE))
    {
        csi.ciCharset = ANSI_CHARSET;
    }
/*
    hfontStatus = CreateFont( -PointsToHeight(bFE
                                                ? FE_STATUSPOINTSIZE
                                                : STATUSPOINTSIZE),
*/
    hfontStatus = CreateFont( -PointsToHeight(STATUSPOINTSIZE),
                              0, 0, 0, 400, 0, 0, 0,
                              csi.ciCharset,
                              OUT_DEFAULT_PRECIS,
                              CLIP_DEFAULT_PRECIS,
                              DEFAULT_QUALITY,
                              VARIABLE_PITCH,
                              TEXT("MS Shell Dlg") );

    dyStatus = 2 * PointsToHeight(STATUSPOINTSIZE);

    //
    //  Create a main window for this application instance.
    //
    if (!(hwndDialog = CreateDialog( hInstance,
                                     TEXT("UCE"),
                                     NULL,
                                     (DLGPROC)UCEWndProc )))
    {
        return (FALSE);
    }

    // Drag action starts after nDragDelay time
    nDragDelay = GetProfileInt(
        TEXT("windows"),
        TEXT("DragDelay"),
        DD_DEFDRAGDELAY
        );

    // If the mouse moves nDragMinDist from the place of LBUTTONDOWN
    // then Drag starts
    nDragMinDist = GetProfileInt(
        TEXT("windows"),
        TEXT("DragMinDist"),
        DD_DEFDRAGMINDIST
        );

    // Scroll Interval
    nScrollInterval = GetProfileInt(
        TEXT("windows"),
        TEXT("DragScrollInterval"),
        DD_DEFSCROLLINTERVAL
        );

    //
    //  Initialize keystroke text, make the window visible,
    //  update its client area, and return "success".
    //
    UpdateKeystrokeText(NULL, sycm.chCurr, FALSE);


    ShowWindow(hwndDialog, nCmdShow);
    UpdateWindow(hwndDialog);

                // Potential for Bug#187822, tab order problem.
                // If the initial focus should be on ID_FONT, add this code,
                // otherwise initial focus will be on ID_CHARGRID
                {
                        HWND hTmp;
                        hTmp = GetDlgItem(hwndDialog, ID_FONT);
                        SetFocus(hTmp);
                }


    return (TRUE);

}

////////////////////////////////////////////////////////////////////////////
//
//  EnumChildProc
//
//  Gets called during init for each child window.
//
////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK EnumChildProc(
    HWND hwnd,
    LPARAM lParam)
{
    LONG st;
    TCHAR szClass[MAX_PATH];

    //
    //  Get control class.
    //
    GetClassName(hwnd, szClass, MAX_PATH);
    if (lstrcmpi(szClass, TEXT("button")) == 0 )
    {
        //
        //  If it is a button, set the ex style to NOTIFYPARENT.
        //
        st = GetWindowLong(hwnd, GWL_EXSTYLE);
        st = st & ~WS_EX_NOPARENTNOTIFY;
        SetWindowLong(hwnd, GWL_EXSTYLE, st);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  UCEWndProc
//
//  Processes messages for the main window.  This window is a dialog box.
//
////////////////////////////////////////////////////////////////////////////

LRESULT APIENTRY UCEWndProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
        case ( WM_KEYDOWN ) :
        {
            if(wParam == VK_F6)
            {
                if(ghwndList)
                    SetFocus(ghwndList);
                else if(ghwndGrid)
                    SetFocus(ghwndGrid);
                return 0L;
            }
            break;
        }

        case ( WM_CTLCOLORSTATIC ) :
        {
            POINT point;

            SetBkColor((HDC)wParam, GetSysColor(COLOR_BTNFACE));
            UnrealizeObject(hStaticBrush);
            point.x = point.y = 0;
            ClientToScreen(hWnd, &point);

            return ((LRESULT)hStaticBrush);
            break;
        }

        case ( WM_INITDIALOG ) :
        {
            int   right_adjust;
            int   bottom_adjust;
            RECT  rectParent, rect;
            POINT pt;
            HWND  hwndCMSB;

            //
            //  Set buttons to send WM_PARENTNOTIFY.
            //
            EnumChildWindows(hWnd, (WNDENUMPROC)EnumChildProc, (LPARAM)NULL );

            // Create the scroll bar first and then resize it in the Chargrid
            // window create routine
            hwndCMSB = CreateWindowEx(0L,
                                      TEXT("SCROLLBAR"),
                                      NULL,
                                      WS_CHILD | SBS_VERT | WS_VISIBLE,
                                      0,
                                      0,
                                      0,
                                      0,
                                      hWnd,
                                      (HMENU)ID_MAPSCROLL,
                                      hInst,
                                      NULL );

            //
            //  Create the character grid with dimensions which just fit
            //  inside the space allowed in the dialog.  When it processes
            //  the WM_CREATE message it will be sized and centered more
            //  accurately.
            //
            GetClientRect(hWnd, &rectParent);

            right_adjust  = (int)((rectParent.right/360) *9);
            bottom_adjust = (int)((rectParent.bottom/10) *3);

            if (!(hwndCharGrid =
                  CreateWindow( TEXT("CharGridWClass"),
                                NULL,
                                WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                0, 0,
                                rectParent.right  - right_adjust,
                                rectParent.bottom - bottom_adjust,
                                hWnd,
                                (HMENU) ID_CHARGRID,
                                hInst,
                                NULL )))
            {
                DestroyWindow(hWnd);
                break;
            }

            GetWindowRect( hwndCharGrid, &rect );
            pt.x = rect.right;
            pt.y = rect.top;

            ScreenToClient(hWnd, &pt);

//pliu            hStaticBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

            // Compute the heights of the dialog in expanded and shrunk state
            ComputeExpandedAndShrunkHeight(hWnd);

            // Load registry setting to find if start state is expanded or shrunk
            LoadAdvancedSelection(hWnd, ID_ADVANCED, SZ_ADVANCED);

            // Resize the Dialog to correspond to expanded or shrunk state
            ResizeDialog(hWnd);

            // Compute client rect of the resized dialog
            GetClientRect(hWnd, &rectParent);

            //
            //  Initialize the status line data.
            //
            dxHelpField = 23 * rectParent.right / 32;
            rcStatusLine = rectParent;
            rcStatusLine.top = rcStatusLine.bottom - dyStatus;

            //
            //  Disable Copy & Search buttons.
            //
            EnableWindow(GetDlgItem(hWnd, ID_COPY),   FALSE);
            EnableWindow(GetDlgItem(hWnd, ID_SEARCH), FALSE);

            if (Display_InitList())
            {
                pCode = NULL;
            }

            if (Font_InitList(hWnd))
            {
                Font_FillToComboBox(hWnd,ID_FONT);
                LoadCurrentSelection(hWnd,ID_FONT,SZ_FONT,SZ_FONT_DEFAULT);
            }

            if (CodePage_InitList())
            {
                CodePage_FillToComboBox(hWnd,ID_VIEW);
                LoadCurrentSelection(hWnd,ID_VIEW,SZ_CODEPAGE,SZ_CODEPAGE_DEFAULT);
            }

            CreateResources(hInst, hWnd);  // init gui stuff
            if( UCE_EnumFiles() )
            {
                Subset_FillComboBox( hWnd , ID_UNICODESUBSET );
            }

            // Initialise the ID_ADVANCED check box
            SendMessage(GetDlgItem(hWnd, ID_ADVANCED),
                BM_SETCHECK,
                (WPARAM)((fDisplayAdvControls==TRUE)? BST_CHECKED: BST_UNCHECKED),
                (LPARAM)0L);

#ifndef DISABLE_RICHEDIT
            // Some richedit's initialization

            // Set and event mask for the rich edit control so that we
            // get a notification when text changes
            SendMessage(GetDlgItem(hWnd, ID_STRING), EM_SETEVENTMASK, 0, ENM_SELCHANGE);

            // Set necessary lang options
/* this is supposed to fix 374706, however, looks like you have to turn on IMF_AUTOFONT
   for RichEdit to change cursor correctly, but this flag cause RichEdit to lose font info.
            DWORD   dwOptions = SendMessage(GetDlgItem(hWnd, ID_STRING), 
                                EM_GETLANGOPTIONS, 0, 0);
            dwOptions |= IMF_IMECANCELCOMPLETE | IMF_UIFONTS;
            SendMessage(GetDlgItem(hWnd, ID_STRING), EM_SETLANGOPTIONS, 0,
                        dwOptions);
*/
            SendMessage(GetDlgItem(hWnd, ID_STRING), EM_SETLANGOPTIONS, 0,
                        IMF_IMECANCELCOMPLETE | IMF_UIFONTS);

#else
            SendMessage(GetDlgItem(hWnd, IDC_EDIT), EM_SETEVENTMASK, 0,
                ENM_SELCHANGE);
#endif

            // Enable/diable SUR controls depending on the range selected
            EnableSURControls(hWnd);

            // Initialise the text in the two UR controls
            SetHexEditProc(GetDlgItem(hWnd, ID_FROM));
            SendMessage(GetDlgItem(hWnd, ID_FROM), EM_LIMITTEXT,
                (WPARAM)MAX_CHARS, (LPARAM)0L);

            // subclass the search edit box to handle the Enter key
            SetSearchEditProc(GetDlgItem(hWnd, ID_SEARCHINPUT));

            // Initialise codepage associated with the active keyboard
            {
                WCHAR wcBuf[8];

                if(GetLocaleInfo(LOWORD(GetKeyboardLayout(0)),
                                 LOCALE_IDEFAULTANSICODEPAGE, wcBuf, 8))
                {
                    gKBD_CP = (UINT) _wtol(wcBuf);
                }
            }

            //
            //  Fall through to WM_FONTCHANGE...
            //
        }

        case ( WM_FONTCHANGE ) :
        {
            //
            //  Get the fonts from the system and put them in the font
            //  selection combo box.
            //

            if (message == WM_FONTCHANGE)
            {
                SaveCurrentSelection(hWnd,ID_FONT,SZ_FONT);

                if (Font_InitList(hWnd))
                {
                    Font_FillToComboBox(hWnd,ID_FONT);
                }
                LoadCurrentSelection(hWnd,ID_FONT,SZ_FONT,SZ_FONT_DEFAULT);
            }

            SendMessage(hWnd, WM_COMMAND, MAKELONG(ID_FONT,CBN_SELCHANGE), 0L);

            if (message == WM_INITDIALOG)
            {
                SetFocus(hwndCharGrid);

                //
                //  Fall through to WM_SYSCOLORCHANGE...
                //
            }
            else
            {
                break;
            }
        }

        case ( WM_SYSCOLORCHANGE ) :
        {
            if (hbmFont)
            {
                DeleteObject(hbmFont);
            }
            hbmFont = LoadBitmaps(IDBM_TT);
//pliu            DeleteObject(hStaticBrush);
//pliu            hStaticBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
            break;
        }

        case ( WM_NOTIFY ):
        {
            int        iTextLength;
            LPNMHDR pnmh = (LPNMHDR)lParam;

#ifndef DISABLE_RICHEDIT
            if ((wParam == ID_STRING) && (pnmh->code == EN_SELCHANGE))
            {
                // If there is no text in the rich edit control then
                // disable the Copy button
                iTextLength = GetWindowTextLength(GetDlgItem(hWnd, ID_STRING));
                EnableWindow(GetDlgItem(hWnd, ID_COPY), (BOOL)iTextLength);
            }
#else
            if ((wParam == IDC_EDIT) && (pnmh->code == EN_SELCHANGE))
            {
                // If there is no text in the rich edit control then
                // disable the Copy button
                iTextLength = GetWindowTextLength(GetDlgItem(hWnd, IDC_EDIT));
                EnableWindow(GetDlgItem(hWnd, ID_COPY), (BOOL)iTextLength);
            }
#endif
            break;
        }

        case ( WM_PARENTNOTIFY ) :
        {
            POINTS points;
            DWORD  dwMsgPos;
            POINT  point;

            DPRINT(( szDbgBuf,
                    TEXT("WM_PARENTNOTIFY: lParam:0x%08lX, wParam:0x%08lX\n"),
                    (DWORD)lParam,
                    (DWORD)wParam ));

            //
            //  We process this message to implement the context sensitive
            //  help.  Downclicks to controls are found here, the help
            //  message is updated in the status bar.
            //
            //  The parameters with this message are unreliable!
            //
            if (LOWORD(wParam) == WM_LBUTTONDOWN)
            {
                dwMsgPos = GetMessagePos();
                points = MAKEPOINTS(dwMsgPos);
                point.x = points.x;
                point.y = points.y;
            }

            break;
        }

        case ( WM_VSCROLL ) :
        {
            ProcessScrollMsg(hWnd, LOWORD(wParam), HIWORD(wParam));
            return TRUE;
        }

        case ( WM_PAINT ) :
        {
            HBRUSH hBrush;
            RECT rcTemp;
            INT dyBorder, dxBorder;
            PAINTSTRUCT ps;
            HDC hdc;

            //
            //  This code implements painting of the status bar.
            //
            hdc = BeginPaint(hWnd, &ps);

            rcTemp = rcStatusLine;

            dyBorder = GetSystemMetrics(SM_CYBORDER);
            dxBorder = GetSystemMetrics(SM_CXBORDER);

            if (hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNSHADOW)))
            {
                //
                //  Status line top.
                //
                rcTemp.left   = 6 * dyBorder;
                rcTemp.right  = rcStatusLine.right - 8 * dyBorder;
                rcTemp.top    = rcStatusLine.top + dyBorder * 2;
                rcTemp.bottom = rcTemp.top + dyBorder;
                FillRect(hdc, &rcTemp, hBrush);

                //
                //  Status line left side.
                //
                rcTemp = rcStatusLine;
                rcTemp.left = 6 * dyBorder;
                rcTemp.right = rcTemp.left + dyBorder;
                rcTemp.top += dyBorder * 2;
                rcTemp.bottom -= dyBorder * 2;
                FillRect(hdc, &rcTemp, hBrush);

                DeleteObject(hBrush);
            }

            if (hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNHIGHLIGHT)))
            {
                //
                //  Status line bottom.
                //
                rcTemp.left   = 8 * dyBorder;
                rcTemp.right  = rcStatusLine.right - 8 * dyBorder;
                rcTemp.top    = rcStatusLine.bottom - 3 * dyBorder;
                rcTemp.bottom = rcTemp.top + dyBorder;
                FillRect(hdc, &rcTemp, hBrush);

                //
                //  Status line right side.
                //
                rcTemp = rcStatusLine;
                rcTemp.left = rcStatusLine.right - 8 * dyBorder;
                rcTemp.right = rcTemp.left + dyBorder;
                rcTemp.top += dyBorder * 2;
                rcTemp.bottom -= dyBorder * 2;
                FillRect(hdc, &rcTemp, hBrush);

                DeleteObject(hBrush);
            }

            PaintStatusLine(hdc, TRUE, TRUE);

            EndPaint(hWnd, &ps);
            return (TRUE);
            break;
        }

        case ( WM_MEASUREITEM ) :
        {
            HDC hDC;
            HFONT hFont;
            TEXTMETRIC tm;

            hDC = GetDC(NULL);
            hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0L);
            if (hFont)
            {
                hFont = (HFONT)SelectObject(hDC, hFont);
            }
            GetTextMetrics(hDC, &tm);
            if (hFont)
            {
                SelectObject(hDC, hFont);
            }
            ReleaseDC(NULL, hDC);

            ((LPMEASUREITEMSTRUCT)lParam)->itemHeight =
                                  max(tm.tmHeight, DY_BITMAP);

            break;
        }

        case ( WM_DRAWITEM ) :
        {
            if (((LPDRAWITEMSTRUCT)lParam)->itemID != -1)
            {
                DrawFamilyComboItem((LPDRAWITEMSTRUCT)lParam);
            }
            break;
        }

        case ( WM_ASKCBFORMATNAME ) :
        {
            LoadString(hInst, IDS_RTF, (LPTSTR)lParam, (int)wParam);
            //should hardcode clipboard format, we'll do it after Win2K
            //lstrcpy((LPTSTR)lParam, RTFFMT);
            return (TRUE);
        }

        case ( WM_PAINTCLIPBOARD ) :
        {
            LPPAINTSTRUCT lpPS;
            HANDLE hFont;
            LPTSTR lpstrText;

            if (hstrClipboard)
            {
                //
                //  Setup.
                //
                lpPS = (LPPAINTSTRUCT)GlobalLock((HANDLE)lParam);
                lpstrText = (LPTSTR)GlobalLock(hstrClipboard);

                //
                //  Paint.
                //
                hFont = SelectObject(lpPS->hdc, hFontClipboard);
                TextOut(lpPS->hdc, 0, 0, lpstrText, lstrlen(lpstrText));
                SelectObject(lpPS->hdc, hFont);

                //
                //  Cleanup.
                //
                GlobalUnlock(hstrClipboard);
                GlobalUnlock((HANDLE)lParam);
            }
            return (TRUE);
        }

        case ( WM_SYSCOMMAND):
        {
            switch (wParam)
            {
                case (SC_CLOSE):
                  DestroyWindow(hWnd);
                  return (TRUE);
            }
            break;
        }

        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                //case ( IDCANCEL ) :
                case ( ID_CLOSE ) :
                {
                    DestroyWindow(hWnd);
                    return (TRUE);
                }

                case ( ID_SELECT ) :
                {
                  WCHAR wc = (WCHAR)pCode[sycm.chCurr];
                  ConvertAnsifontToUnicode(hWnd, (char*)&pCode[sycm.chCurr], &wc);
#ifndef DISABLE_RICHEDIT
                SetRichEditFont(hWnd, ID_STRING, sycm.hFont);
                //richedit screws up symbol font display
                //use ansi code so that it can display symbols correctly.
                if (gFontType & SYMBOL_FONTTYPE)
                {
                  if ((wc >= 0xf000) && (wc <= 0xf0ff))
                    wc = (WCHAR) (BYTE)pCode[sycm.chCurr];
                }
                SendDlgItemMessage(hWnd, ID_STRING, WM_CHAR, wc, 0L);
                CopyTextToClipboard(hWnd);
#else
                SendDlgItemMessage(hWnd, IDC_EDIT, WM_SETFONT, (WPARAM)(sycm.hFont), MAKELPARAM(true,0));
                SendDlgItemMessage(hWnd, IDC_EDIT, WM_CHAR, wc, 0L);
#endif
                    break;
                }

                case ( ID_COPY ) :
                {
                    CopyTextToClipboard(hWnd);
                    return (TRUE);
                }

                case ( ID_FONT ) :
                {
                    static int preItem;
#ifdef DISABLE_RICHEDIT
                    if (HIWORD(wParam) == CBN_DROPDOWN)
                    {
                        preItem = SendDlgItemMessage(hWnd, ID_FONT, CB_GETCURSEL, 0, 0);
                    }
#endif
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        int nItem = (int)SendDlgItemMessage(hWnd, ID_FONT, CB_GETCURSEL,       0, 0);
#ifdef DISABLE_RICHEDIT
                        if ((gDisplayFontChgMsg) &&
                            (preItem != nItem) &&
                            GetWindowTextLength(GetDlgItem(hWnd, IDC_EDIT)))
                        {
                          int dlgRet = DialogBox(hInst, MAKEINTRESOURCE(IDD_FONTCHANGEMSG), hwndDialog, (DLGPROC)MsgProc);
                          if (dlgRet == IDCANCEL)
                          {
                            SendDlgItemMessage(hWnd, ID_FONT, CB_SETCURSEL, (WPARAM)preItem, 0);
                            return TRUE;
                          }
                        }
#endif
                        nItem = (int)SendDlgItemMessage(hWnd, ID_FONT, CB_GETITEMDATA, nItem, 0);
                        gFontType = Font_pList[nItem].FontType;

                        ExitMagnify(hwndCharGrid, &sycm);
                        sycm.chCurr = 0;

                        if((ghwndList == NULL) && (ghwndGrid == NULL))
                        {
                            if(gFontType & SYMBOL_FONTTYPE)
                            {
                                if(CodePage_GetCurCodePageVal() != UNICODE_CODEPAGE)
                                {

                                    SendMessage( GetDlgItem(hWnd, ID_VIEW),
                                                 CB_SETCURSEL,
                                                 (WPARAM) 0,
                                                 (LPARAM) 0);
                                }
                                EnableWindow(GetDlgItem(hWnd, ID_VIEW),          FALSE);
                                EnableWindow(GetDlgItem(hWnd, ID_FROM),          FALSE);
                                EnableWindow(GetDlgItem(hWnd, ID_SEARCHINPUT),   FALSE);
                                EnableWindow(GetDlgItem(hWnd, ID_UNICODESUBSET), FALSE);
                            }
                            else
                            {
                                EnableWindow(GetDlgItem(hWnd, ID_VIEW),          TRUE);
                                EnableWindow(GetDlgItem(hWnd, ID_SEARCHINPUT),   TRUE);
                                EnableWindow(GetDlgItem(hWnd, ID_UNICODESUBSET), TRUE);
                                if(UNICODE_CODEPAGE == CodePage_GetCurCodePageVal())
                                {
                                    EnableWindow(GetDlgItem(hWnd, ID_FROM),      TRUE);
                                }
                                else
                                {
                                    EnableWindow(GetDlgItem(hWnd, ID_FROM),      FALSE);
                                }
                            }
                        }
                        else //GroupBy windows present
                        {
                          if(gFontType & SYMBOL_FONTTYPE) //disable GroupBy
                          {
                            SendMessage(GetDlgItem(hWnd, ID_UNICODESUBSET), CB_SETCURSEL,
                                                   (WPARAM)0,(LPARAM) 0);
                            SendMessage(GetDlgItem(hWnd, ID_VIEW), CB_SETCURSEL,
                                                   (WPARAM)0,(LPARAM) 0);

                            // same as ID_VIEW changed
                            //--------------------------------
                            WCHAR buffer[256];

                            ExitMagnify(hwndCharGrid, &sycm);
                            sycm.chCurr = 0;

                            Subset_OnSelChange(hWnd , ID_UNICODESUBSET );
                            // Then say that subset has changed
                            SubSetChanged( hWnd );

                            fSearched  = FALSE;
                            fNeedReset = FALSE;
                            SetDlgItemText(hWnd, ID_SEARCHINPUT, L"");
                            LoadString(hInst, IDS_SEARCH, buffer, 255);
                            SetDlgItemText(hWnd, ID_SEARCH, buffer);
                            EnableWindow(GetDlgItem(hWnd, ID_SEARCH), FALSE);

                            // Enable or disable SUR controls
                            EnableSURControls(hWnd);
                            //--------------------------------

                            EnableWindow(GetDlgItem(hWnd, ID_VIEW),          FALSE);
                            EnableWindow(GetDlgItem(hWnd, ID_FROM),          FALSE);
                            EnableWindow(GetDlgItem(hWnd, ID_SEARCHINPUT),   FALSE);
                            EnableWindow(GetDlgItem(hWnd, ID_UNICODESUBSET), FALSE);
                          }
                          else
                          {
                            EnableWindow(GetDlgItem(hWnd, ID_UNICODESUBSET), TRUE);
                          }
                        }

                        RecalcUCE( hWnd,
                                   &sycm,
                                   nItem,
                                   TRUE );
#ifndef DISABLE_RICHEDIT
                        SetRichEditFont(hWnd, ID_STRING, sycm.hFont);
#else
                        SendDlgItemMessage(hWnd, IDC_EDIT, WM_SETFONT, WPARAM(sycm.hFont), MAKELPARAM(true,0));
#endif

                        if(fNeedReset || fSearched)
                        {
                            SendMessage(hwndDialog, WM_COMMAND, ID_SEARCH, 0L);
                        }
                    }
                    else if (HIWORD(wParam) == CBN_SETFOCUS)
                    {
                        //
                        //  Necessary if hotkey is used to get to the CB.
                        //
                        // UpdateHelpText(NULL, (HWND)lParam);
                    }

                    return (TRUE);
                }

                case ID_SEARCHINPUT:
                {
                    WCHAR buffer1[256];
                    WCHAR buffer2[256];

                    if( (HWND)lParam != GetFocus() )
                        break;

                    if(fSearched == TRUE) fNeedReset= TRUE;

                    GetWindowText((HWND)lParam, buffer1, 256);
                    if(swscanf(buffer1, L"%s", buffer2) > 0 &&
                       fSearched == FALSE)
                    {
                        LoadString(hInst, IDS_SEARCH, buffer1, 255);
                        SetDlgItemText(hWnd, ID_SEARCH, buffer1);
                        EnableWindow(GetDlgItem(hWnd, ID_SEARCH), TRUE);
                        EnableSURControls(hWnd, TRUE);
                    }
                    else
                    {
                        if(fNeedReset == FALSE)
                        {
                            LoadString(hInst, IDS_SEARCH, buffer1, 255);
                            SetDlgItemText(hWnd, ID_SEARCH, buffer1);
                            EnableWindow(GetDlgItem(hWnd, ID_SEARCH), FALSE);
                            fSearched = FALSE;
                            EnableSURControls(hWnd);
                        }
                        else
                        {
                            LoadString(hInst, IDS_RESET,  buffer2, 255);
                            SetDlgItemText(hWnd, ID_SEARCH, buffer2);
                            EnableWindow(GetDlgItem(hWnd, ID_SEARCH), TRUE);
                            fNeedReset = FALSE;
                            fSearched  = TRUE;
                        }
                    }
                    break;
                }

                case ID_SEARCH:
                {
                    WCHAR buffer[256];
                    WCHAR KeyWord[SEARCH_WORD_MAX][SEARCH_WORD_LEN];
                    WCHAR *ptr1;
                    int   i, j, k, word;
                    char  buffer3[256];
                    WCHAR wbuffer[256];
                    HDC   hdc;

                    if(fSearched == TRUE)
                    {
                        fSearched  = FALSE;
                        fNeedReset = FALSE;
                        SetDlgItemText(hWnd, ID_SEARCHINPUT, L"");
                        LoadString(hInst, IDS_SEARCH, buffer, 255);
                        SetDlgItemText(hWnd, ID_SEARCH, buffer);
                        EnableWindow(GetDlgItem(hWnd, ID_SEARCH), FALSE);
                        SendMessage(hWnd,
                                    WM_COMMAND,
                                    MAKELONG(ID_VIEW,CBN_SELCHANGE),
                                    0L);
                        EnableSURControls(hWnd);
                        break;
                    }

                    GetDlgItemText(hWnd, ID_SEARCHINPUT, buffer, 256);
                    memset(KeyWord, 0, sizeof(KeyWord));
                    ptr1 = buffer;
                    word = 1;

                    do
                    {
                        if(*ptr1 !=  ' ')
                        {
                            j = 0;

                            if(*ptr1 == '\"')   // this is a quoted string
                            {
                                while(*++ptr1 && *ptr1 != '\"')
                                {
                                    KeyWord[word][j++] = *ptr1;
                                }
                            }
                            else                // this is an unquoted string
                            {
                                do
                                {
                                    KeyWord[word][j++] = *ptr1++;
                                }
                                while(*ptr1 && *ptr1 != ' ');

                            }
                        }
                        _wcsupr(KeyWord[word]);
                        if(++word == SEARCH_WORD_MAX)
                            break;
                    }
                    while(*ptr1++ && *ptr1);

                    j = 0;
                    for(i = 0; i <= chRangeLast && pCode[i]; i++)
                    {
                        GetUName(pCode[i], wbuffer);
                        /*
                        MultiByteToWideChar(
                                CP_ACP,
                                MB_PRECOMPOSED,
                                buffer3, -1,
                                wbuffer, 256); */
                        _wcsupr(wbuffer);

                        for(k = 0; k < word; k++)
                        {
                            if(wcsstr(wbuffer, KeyWord[k]) == NULL)
                                break;
                        }

                        if(k < word)
                            continue;

                        pCode[j++] = pCode[i];
                    }

                    pCode[j] = '\0';
                    chRangeLast = j - 1;
                    fSearched  = TRUE;
                    fNeedReset = TRUE;
                    LoadString(hInst, IDS_RESET,  buffer, 255);
                    SetDlgItemText(hwndDialog, ID_SEARCH, buffer);
                    EnableWindow(GetDlgItem(hWnd, ID_SEARCH), TRUE);

                    ExitMagnify(hwndCharGrid, &sycm);
                    sycm.chCurr = 0;
                    UpdateSymbolSelection(hWnd, FALSE);

                    hdc = GetDC(hwndDialog);
                    UpdateKeystrokeText(hdc, sycm.chCurr, TRUE);

                    InvalidateRect(hwndCharGrid, NULL, TRUE);
                    ReleaseDC(hwndDialog, hdc);
                    break;
                }

                case ( ID_UNICODESUBSET ) :
                {
                    WCHAR buffer[256];

                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        ExitMagnify(hwndCharGrid, &sycm);
                        sycm.chCurr = 0;

                        Subset_OnSelChange(hWnd , ID_UNICODESUBSET );
                        // Then say that subset has changed
                        SubSetChanged( hWnd );

                        fSearched  = FALSE;
                        fNeedReset = FALSE;
                        SetDlgItemText(hWnd, ID_SEARCHINPUT, L"");
                        LoadString(hInst, IDS_SEARCH, buffer, 255);
                        SetDlgItemText(hWnd, ID_SEARCH, buffer);
                        EnableWindow(GetDlgItem(hWnd, ID_SEARCH), FALSE);

                        // Enable or disable SUR controls
                        EnableSURControls(hWnd);
                    }
                    else if (HIWORD(wParam) == CBN_SETFOCUS)
                    {
                        //
                        //  Necessary if hotkey is used to get to the CB.
                        //
                    }

                    return (0L);
                }
#ifndef DISABLE_RICHEDIT
                case ( ID_STRING ) :
                {
                    if (HIWORD(wParam) == EN_SETFOCUS)
                    {
                        //
                        //  Necessary if hotkey is used to get to the EC.
                        //
                    }
                    else if (HIWORD(wParam) == EN_CHANGE)
                    {
                        //
                        //  Disable Copy button if there are no chars in EC.
                        //
                        INT iLength;

                        iLength = GetWindowTextLength((HWND)lParam);
                        EnableWindow(GetDlgItem(hWnd, ID_COPY), (BOOL)iLength);
                    }
                    break;
                }
#else
                case ( IDC_EDIT ) :
                {
                    if (HIWORD(wParam) == EN_SETFOCUS)
                    {
                        //
                        //  Necessary if hotkey is used to get to the EC.
                        //
                    }
                    else if (HIWORD(wParam) == EN_CHANGE)
                    {
                        //
                        //  Disable Copy button if there are no chars in EC.
                        //
                        INT iLength;

                        iLength = GetWindowTextLength((HWND)lParam);
                        EnableWindow(GetDlgItem(hWnd, ID_COPY), (BOOL)iLength);
                    }
                    break;
                }
#endif
                case ( ID_HELP ) :
                {
                    DoHelp(hWnd, TRUE);
                    break;
                }

                case (ID_VIEW) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        LONG lCodePage;
                        long lNumWChar;

                        ExitMagnify(hwndCharGrid, &sycm);
                        sycm.chCurr = 0;

                        lCodePage = (LONG)CodePage_GetCurSelCodePage(hWnd,ID_VIEW);
                        if(lCodePage == 0)
                            break;
                        lNumWChar = WCharCP(lCodePage, NULL);
                        if(lNumWChar == 0)
                            break;
                        pCode = Display_CreateDispBuffer(NULL,
                                               lNumWChar, NULL, 0,FALSE);
                        if (pCode == NULL)
                            break;
                        WCharCP(lCodePage, pCode);
                        chRangeLast = lstrlen(pCode)-1;
                        sycm.chCurr = 0;

                        SendMessage(
                            hWnd,
                            WM_COMMAND,
                            MAKELONG(ID_FONT,CBN_SELCHANGE),
                            0L);


                        /* auto font selection according to the codepage.

                        if (lCodePage == UNICODE_CODEPAGE)
                        {
                            SendMessage(
                                hWnd,
                                WM_COMMAND,
                                MAKELONG(ID_FONT,CBN_SELCHANGE),
                                0L);
                        }
                        else
                        {
                            CPINFO      cpinfo;

                            GetCPInfo(lCodePage, &cpinfo);
                            if(cpinfo.MaxCharSize > 1)
                            {
                                CHARSETINFO csi;

                                if(!TranslateCharsetInfo((DWORD*)lCodePage,
                                                &csi, TCI_SRCCODEPAGE))
                                {
                                    csi.ciCharset = Font_GetSelFontCharSet(
                                                     hWnd,
                                                     ID_FONT,
                                                     (INT)SendDlgItemMessage(
                                                                hWnd,
                                                                ID_FONT,
                                                                CB_GETCURSEL,
                                                                0,
                                                                0L));
                                }

                                Font_SelectByCharSet(hWnd,ID_FONT,csi.ciCharset);
                            }
                            SendMessage(
                                hWnd,
                                WM_COMMAND,
                                MAKELONG(ID_FONT,CBN_SELCHANGE),
                                0L);
                        }
                        */

                        // Enable or disable SUR controls
                        EnableSURControls(hWnd);

                        if(fNeedReset || fSearched)
                        {
                            SendMessage(hwndDialog, WM_COMMAND, ID_SEARCH, 0L);
                        }
                     }
                    break;
                }

                case (ID_SUBFUNCCHANGED):
                    break;

                case (ID_ADVANCED):
                {
                    switch (HIWORD(wParam)) {
                    case BN_CLICKED:
                        ShowHideAdvancedControls( hWnd, wParam, lParam );
                        break;
                    }
                }
                break;

                case ( ID_FROM ) :
                {
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        HWND hWndEdit = (HWND)lParam;
                        int nLen = GetWindowTextLength(hWndEdit);
                        if ((nLen == MAX_CHARS) && (fSURChanged == FALSE))
                        {
                            fSURChanged = TRUE;
                            ValidateValues(hWndEdit);
                        }
                        else
                        {
                            fSURChanged = FALSE;
                        }
                    }

                    break;
                }
            }
            break;
        }

        case ( WM_DESTROY ) :
        {
            SaveAdvancedSelection(hWnd,ID_ADVANCED,SZ_ADVANCED);
            SaveCurrentSelection(hWnd,ID_VIEW,SZ_CODEPAGE);
            SaveCurrentSelection(hWnd,ID_FONT,SZ_FONT);
            CodePage_DeleteList();
            Font_DeleteList();
            UCE_CloseFiles();
            DeleteResources();
            Display_DeleteList();

            DoHelp(hWnd, FALSE);

            DeleteObject(hStaticBrush);
            PostQuitMessage(0);
            break;
        }

        case ( WM_ACTIVATEAPP ) :
        {
#if 0
// (#326752) Postponed by richedit so we work around it here (wchao, 6-16-99).
#ifndef DISABLE_RICHEDIT
            if (wParam)
            {
                SendDlgItemMessage( hWnd,
                                    ID_STRING,
                                    EM_SETSEL,
                                    LOWORD(lEditSel),
                                    HIWORD(lEditSel) );
            }
            else
            {
                lEditSel = (LONG)SendDlgItemMessage(hWnd, ID_STRING, EM_GETSEL, 0, 0L);
                SendDlgItemMessage(hWnd, ID_STRING, EM_SETSEL, 0, 0L);
            }
#else
            if (wParam)
            {
                SendDlgItemMessage( hWnd,
                                    IDC_EDIT,
                                    EM_SETSEL,
                                    LOWORD(lEditSel),
                                    HIWORD(lEditSel) );
            }
            else
            {
                lEditSel = SendDlgItemMessage(hWnd, IDC_EDIT, EM_GETSEL, 0, 0L);
                SendDlgItemMessage(hWnd, IDC_EDIT, EM_SETSEL, 0, 0L);
            }
#endif
#endif
            break;
        }

        case ( WM_CONTEXTMENU ) :
        {
            TCHAR    HelpPath[MAX_PATH];
            if( !GetWindowsDirectory( HelpPath, MAX_PATH))
                 return FALSE;
            wcscat((TCHAR *)HelpPath, TEXT("\\HELP\\CHARMAP.HLP"));
            WinHelp((HWND)(wParam),
                   HelpPath, HELP_CONTEXTMENU, (DWORD_PTR)(LPVOID)aHelpIDs);
            return TRUE;
        }

        case ( WM_HELP ) :
        {
            DoHelp(hWnd, TRUE);
            return TRUE;
        }
    }
    return (0L);
}

////////////////////////////////////////////////////////////////////////////
//
//  CharGridWndProc
//
//  Processes messages for the character grid window.
//
////////////////////////////////////////////////////////////////////////////

LRESULT APIENTRY CharGridWndProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    static LPDROPSOURCE pDropSource;
    static POINT        ptDragStart;

    switch (message)
    {
        case ( WM_CREATE ) :
        {
            RECT rect;
            HDC hdcScrn;
            POINT point1, point2;
            HWND hWndScroll;

            //
            //  Setup global.
            //
            hwndCharGrid = hWnd;

            GetClientRect(hWnd, &rect);

            //
            //  Calculate metrics for the character grid and the
            //  magnify window.
            //
            sycm.dxpBox = (rect.right  - 1) / (cchSymRow + 2);
            sycm.dypBox = (rect.bottom    ) / (cchSymCol + 3);
            sycm.dxpCM  = sycm.dxpBox * cchSymRow+1;
            sycm.dypCM  = sycm.dypBox * cchSymCol+1;  // space inside for border

                                                /*
            if ((PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale())) == LANG_CHINESE))
            {
                sycm.dxpMag = sycm.dxpBox * 3 + 5;
            }
                                                else if ((PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale())) == LANG_HINDI) ||
                                                                                        (PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale())) == LANG_TAMIL))
            {*/
                sycm.dxpMag = sycm.dxpBox * 3 + 15 ; //Indic characters are wide (#178112)
            /*}
            else
            {
                sycm.dxpMag = sycm.dxpBox * 2 + 4; // twice the size + 2 bit border
            }*/
            sycm.dypMag = sycm.dypBox * 2 + 4;

            sycm.chCurr   = chSymFirst;
            sycm.hFontMag = NULL;
            sycm.hFont    = NULL;
            sycm.hdcMag   = NULL;
            sycm.hbmMag   = NULL;
            sycm.ypDest   = 0;

            sycm.fFocusState = sycm.fMouseDn = sycm.fCursorOff = FALSE;
            sycm.fMagnify = FALSE;

            //
            //  Size the window precisely so the grid fits and is centered.
            //
            MoveWindow( hWnd,
                        (rect.right  - sycm.dxpCM) / 2 + 2,  //- 4,
                        (rect.bottom - sycm.dypCM) / 2 - 18 + 20,
                        sycm.dxpCM + 2,
                        sycm.dypCM + 2,
                        FALSE );

            //
            //  Figure out what the offsets are between the dialog
            //  and the character grid window.
            //
            point1.x = point1.y = point2.x = point2.y = 0;
            ClientToScreen(hWnd, &point1);
            ClientToScreen(((LPCREATESTRUCT)lParam)->hwndParent, &point2);
            //
            // In a mirrored charmap point1.x - point2.x will be a -ve value
            //
            sycm.xpCM = abs(point1.x - point2.x) - (sycm.dxpMag - sycm.dxpBox) / 2;
            sycm.ypCM = (point1.y - point2.y) - (sycm.dypMag - sycm.dypBox) / 2;


            //
            //  Create dc and bitmap for the magnify window.
            //
            if ((hdcScrn = GetWindowDC(hWnd)) != NULL)
            {
                if ((sycm.hdcMag = CreateCompatibleDC(hdcScrn)) != NULL)
                {
                    SetTextColor( sycm.hdcMag,
                                  GetSysColor(COLOR_WINDOWTEXT) );
                    SetBkColor( sycm.hdcMag,
                                GetSysColor(COLOR_WINDOW) );
                    SetBkMode(sycm.hdcMag, OPAQUE);

                    if ((sycm.hbmMag =
                         CreateCompatibleBitmap( hdcScrn,
                                                 sycm.dxpMag,
                                                 sycm.dypMag * 2 )) == NULL)
                    {
                        ;
                        //DeleteDC(sycm.hdcMag);
                        //DeleteObject(sycm.hdcMag);
                    }
                    else
                    {
                        SelectObject(sycm.hdcMag, sycm.hbmMag);
                    }
                }
                ReleaseDC(hWnd, hdcScrn);
            }

            // Create an instance of CDropSource for Drag and Drop
            pDropSource = new CDropSource;

            if (pDropSource == NULL) {
                // Show error message and prevent window from being shown
                return -1;
            }

            hWndScroll = GetDlgItem(GetParent(hWnd), ID_MAPSCROLL);

            GetWindowRect( hwndCharGrid, &rect );
            // Use MapWindowPoint instead of ScreenToClient to map the entire rect,
            // to swap the left and right in a mirrored window.
            MapWindowPoints(NULL, GetParent(hWnd), (LPPOINT) &rect, 2);

            MoveWindow( hWndScroll,
                        rect.right+1,
                        rect.top+1,
                        sycm.dxpBox,
                        sycm.dypCM,
                        FALSE );
            break;
        }

        case ( WM_DESTROY ) :
        {
            if (sycm.fMouseDn)
            {
                ExitMagnify(hWnd, &sycm);
            }
            if (fDelClipboardFont)
            {
                DeleteObject(hFontClipboard);
            }
            if (sycm.hFont != NULL)
            {
                DeleteObject(sycm.hFont);
            }
            if (sycm.hFontMag != NULL)
            {
                DeleteObject(sycm.hFontMag);
            }
            if (sycm.hdcMag != NULL)
            {
                DeleteDC(sycm.hdcMag);
            }
            if (sycm.hbmMag != NULL)
            {
                DeleteObject(sycm.hbmMag);
            }

            // Release pDropSource it will automatically destroy itself
            pDropSource->Release();

            break;
        }

        case ( WM_SETFOCUS )  :
        {
            WCHAR wcBuf[8];
            UINT  cp;

            if(GetLocaleInfo(LOWORD(GetKeyboardLayout(0)),
                             LOCALE_IDEFAULTANSICODEPAGE, wcBuf, 8))
            {
                cp = _wtol(wcBuf);
                if(gKBD_CP != cp)
                {
                    gKBD_CP = cp;
                    HDC hDC = GetDC(hwndDialog);
                    UpdateKeystrokeText(hDC, sycm.chCurr, TRUE);
                    ReleaseDC(hwndDialog, hDC);
                }
            }
            //
            // fall through ...
            //
        }

        case ( WM_KILLFOCUS ) :
        {
            DOUTL("Focus\n");
            if (sycm.fMagnify == FALSE)
                DrawSymChOutlineHwnd( &sycm,
                                  hWnd,
                                  sycm.chCurr,
                                  TRUE,
                                  message == WM_SETFOCUS );
            sycm.fFocusState = (message == WM_SETFOCUS);

            break;
        }

        case ( WM_TIMER ) :
        {
            switch (wParam)
            {
                case ( ID_DRAGTIMER ) :
                {
                    // If the user has kept LBUTTON down for long then
                    // start drag and drop operation
                    ReleaseCapture();
                    KillTimer(hWnd, 1);
                    fPendingDrag = FALSE;

                    // Come out of magnify mode
                    if (sycm.fMouseDn)
                    {
                        ExitMagnify(hWnd, &sycm);
                    }

                    // perform the modal drag/drop operation.
                    DoDragAndDrop(GetParent(hWnd), pDropSource);
                    break;
                }

                case ( ID_SCROLLTIMER ) :
                {
                    DoDragScroll(hWnd, (WPARAM)prevKeys, (LPARAM)ptPrevMouse);
                    break;
                }
            }

            break;
        }

        case ( WM_LBUTTONDOWN ) :
        {
            RECT    rect;
            UINT    chMouseSymbol;

            DOUTL("WM_LBUTTONDOWN: In\n");

            chMouseSymbol = (UINT)ChFromSymLParam(&sycm, lParam);

            // If cursor is off (magnified) and lbutton is clicked
            // exit magnify and go into drag mode ELSE magnify
            if ((sycm.fMagnify)&&(chMouseSymbol == sycm.chCurr))
            {
                // Store the point at which LBUTTON was down
                ptDragStart.x = (int)(short)LOWORD (lParam);
                ptDragStart.y = (int)(short)HIWORD (lParam);

                // Go into dragging mode
                fPendingDrag = TRUE;

                // Start timer
                SetTimer(hWnd, 1, nDragDelay, NULL);
                SetCapture(hWnd);
            }
            else
            {
                if(!pCode || *pCode == '\0')
                    return (0L);

                //
                //  Don't draw anything if there's an update region pending.
                //
                if (GetUpdateRect(hWnd, (LPRECT)&rect, FALSE) != 0)
                {
                    DOUTL("WM_LBUTTONDOWN: No upd rect\n");
                    break;
                }

                SetFocus(hWnd);
                SetCapture(hWnd);

                sycm.fMouseDn = TRUE;

                if ((!FMagData(&sycm))&&(sycm.fMagnify==FALSE))
                {
                    DOUTL("WM_LBUTTONDOWN: Drawing sym outline\n");
                    DrawSymChOutlineHwnd(&sycm, hWnd, sycm.chCurr, FALSE, FALSE);
                }
            }
            ShowWindow(hwndTT, SW_HIDE);

            //
            //  Fall through to WM_MOUSEMOVE...
            //
        }

        case ( WM_MOUSEMOVE ) :
        {
            int x, y;

            if (sycm.fCursorOff)
            {
                sycm.fCursorOff = FALSE;
                ShowCursor(TRUE);
            }
            // This part is for Drag and Drop
            // If a drag is pending and mouse moves beyond threshold
            // then start our drag and drop operation
            if (fPendingDrag)
            {
                x = (int)(short)LOWORD (lParam);
                y = (int)(short)HIWORD (lParam);

                // Find if the point at which the mouse is is beyond the
                // min rectangle enclosing the point at which LBUTTON
                // was down
                if (! (((ptDragStart.x - nDragMinDist) <= x)
                    && (x <= (ptDragStart.x + nDragMinDist))
                    && ((ptDragStart.y - nDragMinDist) <= y)
                    && (y <= (ptDragStart.y + nDragMinDist))) )
                {
                    // mouse moved beyond threshhold to start drag
                    ReleaseCapture();
                    KillTimer(hWnd, 1);
                    fPendingDrag = FALSE;

                    // perform the modal drag/drop operation.
                    DoDragAndDrop(GetParent(hWnd), pDropSource);
                }
                break;
            }

            // This is the normal code
            if (sycm.fMouseDn)
            {
                POINT pt;
                UINT chMouseSymbol;

                DOUTL("WM_MOUSEMOVE: mouse is down\n");

                pt.x = LOWORD(lParam);
                pt.y = HIWORD(lParam);


                ClientToScreen(hWnd, (LPPOINT)&pt);

                if (WindowFromPoint(pt) == hWnd)
                {
                    // Kill the out of client are drag timer
                    KillTimer(hWnd, ID_SCROLLTIMER);

                    ScreenToClient(hWnd, (LPPOINT)&pt);
                    //
                    //  Convert back to a 'points'-like thing.
                    //
                    lParam = MAKELONG((WORD)pt.x, (WORD)pt.y);

                    chMouseSymbol = (UINT)ChFromSymLParam(&sycm, lParam);
                    chPos = chMouseSymbol;
                    if(chPos + chSymFirst <= chRangeLast)
                    {
                        chCurrPos = chPos;
                    }
                    else
                    {
                        chPos = chCurrPos;
                    }

                    chMouseSymbol = (UINT)ChFromSymLParam(&sycm, lParam);

                    // disable space display
                    if ( (chMouseSymbol > (UINT)chSymLast) ||
                         (IsAnyListWindow() &&
                          pCode &&
                          (pCode[chMouseSymbol] == (WCHAR)' ')) )
                    {
                        //
                        //  We're outside of current character range (but
                        //  still within the grid).  Restore cursor and
                        //  leave magnified character.
                        //
                        if (sycm.fCursorOff)
                        {
                            sycm.fCursorOff = FALSE;
                            ShowCursor(TRUE);
                        }
                    }
                    else
                    {
                        //
                        //  We're in the grid and within the range of currently
                        //  displayed characters, display magnified character.
                        //
                        DOUTL("WM_MOUSEMOVE: in grid and subrange\n");

                        if (!sycm.fCursorOff)
                        {
                            sycm.fCursorOff = TRUE;
                            ShowCursor(FALSE);
                        }
                        DOUTL("WM_MOUSEMOVE: movsymsel ");
                        DOUTCHN( (UTCHAR)chMouseSymbol );
                        MoveSymbolSel(&sycm, (UTCHAR)chMouseSymbol);
                    }
                }
                else
                {
                    //
                    //  Left grid, leave magnified character and restore
                    //  cursor.
                    //
                    DOUTL("In Drag Scroll\n");
                    if (sycm.fCursorOff)
                    {
                        sycm.fCursorOff = FALSE;
                        ShowCursor(TRUE);
                    }

                    // Here comes the scroll code for scrolling when we are
                    // outside the client area
                    DoDragScroll(hWnd, wParam, lParam);
                }
            }
            else
            {
                MSG   msg;

                //we need to fill out a message structure and pass
                //it to the tooltip with the TTM_RELAYEVENT message

                msg.hwnd    = hWnd;
                msg.message = message;
                msg.wParam  = wParam;
                msg.lParam  = lParam;
                GetCursorPos(&msg.pt);
                msg.time = GetMessageTime();

                SendMessage(hwndTT, TTM_RELAYEVENT, 0, (LPARAM)&msg);
            }

            DOUTL("WM_MOUSEMOVE: Leaving\n");
            break;
        }

        case ( WM_CANCELMODE ) :
        case ( WM_LBUTTONUP )  :
        {
            //ShowWindow(hwndTT, SW_SHOWNA);

            if (sycm.fMouseDn)
            {
                KillTimer(hWnd, ID_SCROLLTIMER);
            }
            sycm.fMouseDn = FALSE;
            ReleaseCapture();

            // This part is for drag and drop
            // Button came up before starting drag so clear flags and timer
            if (fPendingDrag)
            {
                ReleaseCapture();
                KillTimer(hWnd, 1);
                fPendingDrag = FALSE;
                ExitMagnify(hWnd, &sycm);
            }
            break;
        }

        case ( WM_LBUTTONDBLCLK ) :
        {
            UINT chMouseSymbol = (UINT)ChFromSymLParam(&sycm, lParam);

            if (chMouseSymbol <= chRangeLast &&
                pCode[chMouseSymbol] &&
                pCode[chMouseSymbol] != (WCHAR)' ' )
            {
                WCHAR wc = (WCHAR)pCode[sycm.chCurr];
                ConvertAnsifontToUnicode(hWnd, (char*)&pCode[sycm.chCurr], &wc);
#ifndef DISABLE_RICHEDIT
                SetRichEditFont(hwndDialog, ID_STRING, sycm.hFont);
                //richedit screws up symbol font display
                //we have to convert to ansi to display symbol font
                if (gFontType & SYMBOL_FONTTYPE)
                {
                  if ((wc >= 0xf000) && (wc <= 0xf0ff))
                    wc = (WCHAR)(BYTE)pCode[sycm.chCurr];
                }
                SendDlgItemMessage(hwndDialog, ID_STRING, WM_CHAR, wc, 0L);
                CopyTextToClipboard(hwndDialog);
#else
                SendDlgItemMessage(hWnd, IDC_EDIT, WM_SETFONT, (WPARAM)(sycm.hFont), MAKELPARAM(true,0));
                SendDlgItemMessage(hwndDialog, IDC_EDIT, WM_CHAR, wc, 0L);
#endif
            }
            if(sycm.fMagnify)
            {
               ExitMagnify(hwndCharGrid, &sycm);
            }
            break;
        }

        case ( WM_GETDLGCODE ) :
        {
            //
            //  Necessary to obtain arrow and tab messages.
            //
            return (DLGC_WANTARROWS | DLGC_WANTCHARS);
            break;
        }

        case ( WM_KEYDOWN ) :
        {
            UTCHAR chNew = sycm.chCurr;
            INT    cchMoved;

            fScrolled = FALSE;

            // If hWnd is mirrored swap left and right keys.
            if (GetWindowLongPtr(hWnd, GWL_EXSTYLE) & WS_EX_LAYOUTRTL) {
                if (wParam == VK_LEFT) {
                    wParam = VK_RIGHT;
                } else if (wParam == VK_RIGHT) {
                    wParam = VK_LEFT;
                }
            }

            switch (wParam)
            {
                case ( VK_F6 ) :
                {
                    if(ghwndList)
                        SetFocus(ghwndList);
                    else if(ghwndGrid)
                        SetFocus(ghwndGrid);
                    return 0L;
                }

                case ( VK_LEFT ) :
                {
                    if(chNew == 0 || --chNew >= chSymFirst)
                        break;

                    SendMessage(hWnd, WM_KEYDOWN, VK_UP, 0L);

                    while(pCode[chNew] == L' ')
                        --chNew;

                    sycm.chCurr = chNew;
                    break;
                }

                case ( VK_UP ) :
                {
                    if ((chNew -  cchSymRow) >= chRangeFirst &&
                        (chNew -= (UTCHAR)cchSymRow) < chSymFirst)
                    {
                        if (!ScrollMap(GetParent(hWnd), -cchSymRow, TRUE))
                            return (0L);

                        while(pCode[chNew] == L' ')
                            --chNew;
                        sycm.chCurr = chNew;
                        RestoreSymMag(hWnd, &sycm);
                        fScrolled = TRUE;
                    }
                    break;
                }

                case ( VK_RIGHT ) :
                {
                    if ((chNew+1) > chRangeLast)
                        break;
                    if (++chNew <= chSymLast && pCode[chNew] != L' ')
                        break;

                    if (!FMagData(&sycm))
                        DrawSymChOutlineHwnd(&sycm, hWnd, sycm.chCurr, FALSE, FALSE);

                    sycm.chCurr = ((chNew-1)/cchSymRow)*cchSymRow;
                    SendMessage(hWnd, WM_KEYDOWN, VK_DOWN, 0L);
                    return (0L);
                }

                case ( VK_DOWN ) :
                {
                                                                          if ((chNew +  cchSymRow) <= chRangeLast &&
                        (chNew += (UTCHAR)cchSymRow) <= chSymLast)
                              break;

                    if (!ScrollMap(GetParent(hWnd), cchSymRow, TRUE))
                        return (0L);

                    while(pCode[chNew] == L' ')
                        --chNew;
                    sycm.chCurr = chNew;
                    RestoreSymMag(hWnd, &sycm);
                    fScrolled = TRUE;
                    break;
                }

                case ( VK_NEXT ) :
                {
                    if(chRangeLast <= chSymLast)
                    {
                        while(chNew+cchSymRow <= chRangeLast)
                            chNew += (UTCHAR)cchSymRow;
                        break;
                    }

                    if((cchMoved =
                        ScrollMapPage(GetParent(hWnd), FALSE, TRUE)) == 0)
                    {
                        ValidateRect(hWnd, NULL);
                        return (0L);
                    }
                    //
                    //  We scrolled the map!  Bump the char so it is
                    //  still in the window.
                    //
                    RestoreSymMag(hWnd, &sycm);
                    fScrolled = TRUE;
                    chNew += (UTCHAR)cchMoved;
                    if(chNew > chRangeLast)
                       sycm.chCurr = chNew = chNew - cchSymRow;

                    while(pCode[chNew] == L' ')
                        --chNew;

                    sycm.chCurr = chNew;
                    break;
                }

                case ( VK_PRIOR ) :
                {
                    if(chSymLast < cchSymRow*cchSymCol)
                    {
                        while(chNew-cchSymRow >= 0)
                            chNew -= (UTCHAR)cchSymRow;
                        break;
                    }

                    if ((cchMoved =
                        ScrollMapPage( GetParent(hWnd), TRUE, TRUE )) == 0)
                    {
                        ValidateRect(hWnd, NULL);
                        return (0L);
                    }

                    //
                    //  We scrolled the map!  Bump the char so it is
                    //  still in the window.
                    //
                    RestoreSymMag(hWnd, &sycm);
                    fScrolled = TRUE;
                    chNew += (UTCHAR)cchMoved;

                    while(pCode[chNew] == L' ')
                        --chNew;

                    sycm.chCurr = chNew;
                    break;
                }

                case ( VK_HOME ) :
                {
                    if(GetAsyncKeyState(VK_CONTROL))
                    {
                        chNew = 0;
                        if(chSymLast > cchSymRow*cchSymCol)
                        {
                            sycm.chCurr = 0;
                            ScrollMap(GetParent(hWnd),
                                      cchSymRow*cchSymCol -chSymLast - 1,
                                      TRUE);
                            RestoreSymMag(hWnd, &sycm);
                            fScrolled = TRUE;
                        }
                    }
                    else
                    {
                        chNew = (chNew/cchSymRow)*cchSymRow;
                    }
                    break;
                }

                case ( VK_END ) :
                {
                    if(GetAsyncKeyState(VK_CONTROL))
                    {
                        chNew = chRangeLast;
                        if(chSymLast < chRangeLast)
                        {
                            int iTemp = ((chRangeLast + cchSymRow)/cchSymRow)
                                        *cchSymRow - chSymLast - 1;

                            ScrollMap(GetParent(hWnd), iTemp, TRUE);
                            RestoreSymMag(hWnd, &sycm);
                            sycm.chCurr = chRangeLast;
                            fScrolled = TRUE;
                        }
                    }
                    else
                    {
                        chNew = ((chNew+cchSymRow)/cchSymRow)*cchSymRow-1;

                        while(pCode[chNew] == L' ')
                            --chNew;

                        if(chNew > chRangeLast)
                            chNew = chRangeLast;
                    }
                    break;
                }

                            default :
                {
                   return (0L);
                }
            }

            if (!FMagData(&sycm))
                DrawSymChOutlineHwnd(&sycm, hWnd, sycm.chCurr, FALSE, FALSE);

            while ((pCode[chNew] == L' ') && (chNew > chSymFirst))
                --chNew;

            // If scrolled and magnifed draw magnified character
            if ((fScrolled == FALSE) && (sycm.fMagnify == TRUE))
               MoveSymbolSel(&sycm, chNew);
            else
            {
                HDC  hdc;

                // else set focus to true and draw normal character
                BOOL fFocus;
                sycm.fFocusState = TRUE;
                if (fFocus = sycm.fFocusState)
                {
                    sycm.fFocusState = FALSE;
                }

                hdc = GetDC(hwndDialog);
                UpdateKeystrokeText(hdc, chNew, TRUE);
                ReleaseDC(hwndDialog, hdc);

                DrawSymChOutlineHwnd(&sycm, hWnd, chNew, TRUE, fFocus);
                sycm.chCurr = chNew;
            }

                                                {       //#bug 234106 refresh
                                                        HWND hwndCharGrid, hwndSB;
                                                        hwndCharGrid = GetDlgItem(GetParent(hWnd), ID_CHARGRID);
                                                        hwndSB = GetDlgItem(GetParent(hWnd), ID_MAPSCROLL);
                                                        InvalidateRect(hwndCharGrid, NULL, TRUE);
                                                        InvalidateRect(hwndSB, NULL, TRUE);
                                                }

            break;
        }

        case ( WM_CHAR ) :
        {
            WCHAR wch = (WCHAR)wParam;

            if (sycm.fMouseDn)
                break;

        // If space is typed in and user is in Magnify mode exit magnify
        // else enter magnify
            if (wch == ' ')
            {
                if (sycm.fMagnify)
                {
                    ExitMagnify(hwndCharGrid, &sycm);
                }
                else
                {
                    DrawSymChOutlineHwnd(&sycm, hWnd, sycm.chCurr, FALSE, FALSE);
                    MoveSymbolSel(&sycm, (UTCHAR)sycm.chCurr, TRUE);
                }
            }

            break;
        }

        case ( WM_PAINT ) :
        {
            HDC hdc;
            PAINTSTRUCT ps;

            DOUTL("WM_PAINT: In\n");

            hdc = BeginPaint(hWnd, &ps);
            DOUTL("WM_PAINT: drawing map\n");
            DrawSymbolMap(&sycm, hdc);

            if(fScrolled)
            {
               DOUTL("WM_PAINT: drawing scroll\n");
               // Drawing correct outline is handled in DrawSymbolMap so no need
               // to do it again here

                // IF the character is magnified draw magnified character
                if (sycm.fMagnify)
                {
                    MoveSymbolSel(&sycm, sycm.chCurr, TRUE);
                }

                fScrolled = FALSE;
            }
            else
            {
                if (sycm.fMagnify)
                {
                    DOUTL("WM_PAINT: drawing magnify\n");
                    MoveSymbolSel(&sycm, sycm.chCurr, TRUE);
                }
            }

            EndPaint(hWnd, &ps);

            DOUTL("WM_PAINT: Leaving\n");
            return (TRUE);
        }

        case ( WM_NOTIFY ) :
        {
            // Tooltip sends a notification to get text to be displayed

            LPTOOLTIPTEXT lptt = (LPTOOLTIPTEXT)lParam;
            POINT pt;
            WCHAR wc;
            INT   x, y, ch;
            DWORD dwPos = GetMessagePos();
            char  szMessageA[MAX_PATH*2];
            WCHAR szMessageW[MAX_PATH];
            LONG  lCodePage;

            if(pCode == NULL || *pCode == 0) break;

            switch (lptt->hdr.code)
            {
                case TTN_NEEDTEXT:
                {
                    pt.x = LOWORD(dwPos);
                    pt.y = HIWORD(dwPos);

                    // Convert the point from screen coordinates to our
                    // window coordinates
                    MapWindowPoints(HWND_DESKTOP, hWnd, &pt, 1);

                    // Now given the point find which rectangle it lies in
                    x = pt.x / sycm.dxpBox;
                    y = pt.y / sycm.dypBox;

                    ch = chSymFirst + y*cchSymRow + x;

                    // If in magnify mode and mouse is in magnify rect then show
                    // tip for the magnified character
                    if (sycm.fMagnify == TRUE)
                    {
                        RECT  rcMagnify;
                        POINT ptLeftTop;

                        ptLeftTop.x = sycm.xpMagCurr;
                        ptLeftTop.y = sycm.ypMagCurr;

                        MapWindowPoints(hwndDialog, hwndCharGrid, &ptLeftTop, 1);

                        rcMagnify.left   = ptLeftTop.x;
                        rcMagnify.right  = rcMagnify.left + sycm.dxpMag;
                        rcMagnify.top    = ptLeftTop.y;
                        rcMagnify.bottom = rcMagnify.top + sycm.dypMag;

                        if (PtInRect(&rcMagnify, pt))
                        {
                            ch = sycm.chCurr;
                        }
                    }

                    if ((ch > chSymLast) || (ch > chRangeLast))
                        break;

                    wc = pCode[ch];

                    // Dont show tips for filler characters
                    if (wc == 0x20)
                        break;

                    if(gFontType & SYMBOL_FONTTYPE)
                    {
                        LoadString(hInst, IDS_SYMBOLSET, szMessageW, MAX_PATH);
                        wsprintf(szTipText, TEXT("%s : 0x%2X"),szMessageW, wc);
                    }
                    else
                    {
                        GetUName(wc, szMessageW);
                        /*
                        MultiByteToWideChar(
                            CP_ACP, 0,
                            szMessageA, -1,
                            szMessageW, MAX_PATH);*/

                        if((lCodePage = CodePage_GetCurCodePageVal())
                            != UNICODE_CODEPAGE)
                        {
                            BYTE mb[2];
                            WORD wCharCode;

                            if (WideCharToMultiByte(
                                lCodePage, WC_NO_BEST_FIT_CHARS,
                                &wc, 1,
                                (char*)mb, 2,
                                NULL, NULL ) == 1)
                            {
                                wCharCode = mb[0];     // single-byte
                            }
                            else
                            {
                                wCharCode = (mb[0]<<8) | mb[1];
                            }

                            wsprintf(szTipText, TEXT("U+%04X (0x%2X): %s"),
                                     wc, wCharCode, szMessageW);
                        }
                        else
                        {
                            wsprintf(szTipText, TEXT("U+%04X: %s"),
                                     wc, szMessageW);
                        }
                    }
                    lstrcpyn(lptt->szText, szTipText, 80);
                }

                default:
                    break;
            }
        }
          break;

        case ( WM_INPUTLANGCHANGE ):
        {
            WCHAR wcBuf[8];
            UINT  cp;

            if(GetLocaleInfo(LOWORD(lParam), LOCALE_IDEFAULTANSICODEPAGE, wcBuf, 8))
            {
                cp = _wtol(wcBuf);
                if(gKBD_CP != cp)
                {
                    gKBD_CP = cp;
                    HDC hDC = GetDC(hwndDialog);
                    UpdateKeystrokeText(hDC, sycm.chCurr, TRUE);
                    ReleaseDC(hwndDialog, hDC);
                }
            }
            return TRUE;
        }

              default :
        {
            return (DefWindowProc(hWnd, message, wParam, lParam));
        }

    }
    return (0L);
}

///////////////////////////////////////////////////////////////////////////
//
// When enumerate Bitmap font, Symbol font, OEM font, we got the ANSI code
// points instead of Unicode. So when these fonts are going to be displayed
// we have to convert them to Unicode. (We can use TextOutA, but in
// SendDlgItemMessage, it has to be Unicode.)
//
// We can not convert it using the charset information given by the LOGFONT
// during font enumeration. Those charset information are not accurate. For
// example, in Thai system, "Small fonts" contains Thai characters, but the
// CharSet in LOGFONT is 0. However, the CharSet information in TextMetrics
// is correct.
////////////////////////////////////////////////////////////////////////////
DWORD GetCurFontCharSet(HWND hWnd)
{
  DWORD cs;
  HDC hdc;
  HFONT hfOld;
  TEXTMETRIC tm;

  hdc = GetDC(hWnd);
  hfOld = (HFONT)SelectObject(hdc, sycm.hFont);
  GetTextMetrics(hdc, &tm);
  cs = MAKELONG(tm.tmCharSet, 0);
  SelectObject(hdc, hfOld);
  ReleaseDC(hWnd, hdc);
  return cs;
}

int ConvertAnsifontToUnicode(HWND hWnd, char* mb, WCHAR* wc)
{
  CHARSETINFO csi;
  WORD cp = CP_ACP;
  int  ret = 1;
  DWORD cs;

  if (gFontType & SYMBOL_FONTTYPE)
  {
    cs = GetCurFontCharSet(hWnd);
    if (TranslateCharsetInfo((DWORD *)cs, &csi, TCI_SRCCHARSET))
      cp = (WORD)(csi.ciACP);
    if ((ret = MultiByteToWideChar(cp, 0, mb, 1, wc, 1)) != 1)
      if ((ret = MultiByteToWideChar(CP_ACP, 0, mb, 1, wc, 1)) != 1)
        *wc = (WCHAR)(BYTE)(*mb);
  }
  return ret;
}

int ConvertUnicodeToAnsiFont(HWND hWnd, WCHAR* wc, char* mb)
{
  CHARSETINFO csi;
  WORD cp = CP_ACP;
  int ret = 1;
  DWORD cs;

  cs = GetCurFontCharSet(hWnd);
  if (TranslateCharsetInfo((DWORD *)cs, &csi, TCI_SRCCHARSET))
    cp = (WORD)(csi.ciACP);
  if ((ret = WideCharToMultiByte(cp, 0, wc, 1, mb, 1, NULL, NULL)) == 0)
    if ((ret = WideCharToMultiByte(CP_ACP, 0, wc, 1, mb, 1, NULL, NULL)) == 0)
      *mb = (char)(BYTE)(*wc);
  return ret;
}

////////////////////////////////////////////////////////////////////////////
//
//  ProcessScrollMsg
//
////////////////////////////////////////////////////////////////////////////

VOID ProcessScrollMsg(
    HWND hwndDlg,
    int nCode,
    int nPos)
{
    UTCHAR chNew = sycm.chCurr;
    HWND hwndGrid = GetDlgItem(hwndDlg, ID_CHARGRID);
    int cchScroll;

    switch( nCode )
    {
        case ( SB_LINEUP ) :
        {
            cchScroll = -cchSymRow;
            break;
        }
        case ( SB_LINEDOWN ) :
        {
            cchScroll = cchSymRow;
            break;
        }
        case ( SB_PAGEUP ) :
        {
            cchScroll = (int)TRUE;
            break;
        }
        case ( SB_PAGEDOWN ) :
        {
            cchScroll = (int)FALSE;
            break;
        }
        case ( SB_THUMBTRACK ) :
        case ( SB_THUMBPOSITION ) :
        {
            cchScroll = (nPos * cchSymRow + chRangeFirst) - chSymFirst;
            break;
        }
        default :
        {
            return;
        }
    }

    if (nCode == SB_PAGEUP || nCode == SB_PAGEDOWN)
    {
        if (!ScrollMapPage(hwndDlg, (BOOL)cchScroll, TRUE))
        {
            return;
        }

        //
        //  ScrollMapPage will do the right thing to sycm.chCurr.
        //
        chNew = sycm.chCurr;
        fScrolled = TRUE;
        while(pCode[chNew] == L' ')
            --chNew;
    }
    else
    {
        if (cchScroll == 0 || !ScrollMap(hwndDlg, cchScroll, TRUE))
        {
            return;
        }

        //
        //  Keep the current symbol inside the window.
        //
        while (chNew > chSymLast)
        {
            chNew -= (UTCHAR)cchSymRow;
        }

        while (chNew < chSymFirst)
        {
            chNew += (UTCHAR)cchSymRow;
        }
        while(pCode[chNew] == L' ')
            --chNew;
    }

    sycm.chCurr = chNew;
    RestoreSymMag(hwndGrid, &sycm);
}

////////////////////////////////////////////////////////////////////////////
//
//  ScrollMapPage
//
//  Scrolls the map up or down by a page.  See ScrollMap().
//
////////////////////////////////////////////////////////////////////////////

INT ScrollMapPage(
    HWND hwndDlg,
    BOOL fUp,
    BOOL fRePaint)
{
    INT cchScroll = cchFullMap;

    if (fUp)
    {
        cchScroll = -cchScroll;
    }

    if ((chSymFirst + cchScroll) < chRangeFirst)
    {
        cchScroll = (chRangeFirst - chSymFirst);
    }
    else if ((chSymLast + cchScroll) > chRangeLast)
    {
        cchScroll = (1+(chRangeLast-chSymLast-1)/cchSymRow)*cchSymRow;
    }

    return (ScrollMap(hwndDlg, cchScroll, fRePaint) ? cchScroll : 0);
}


////////////////////////////////////////////////////////////////////////////
//
//  ScrollMap
//
//  Scrolls the map up or down if there are too many chars to fit in the
//  chargrid.
//
////////////////////////////////////////////////////////////////////////////

BOOL ScrollMap(
    HWND hwndDlg,
    INT cchScroll,
    BOOL fRePaint)
{
    HWND hwndSB, hwndCharGrid;
    INT chFirst = chSymFirst + cchScroll;
    INT chLast = chSymLast + cchScroll;
    HDC hdc;

    if ((chFirst < chRangeFirst) || (chLast > chRangeLast + cchSymRow - 1))
    {
        return (FALSE);
    }
    hwndCharGrid = GetDlgItem(hwndDlg, ID_CHARGRID);
    hwndSB = GetDlgItem(hwndDlg, ID_MAPSCROLL);
    SetScrollPos(hwndSB, SB_CTL, (chFirst - chRangeFirst) / cchSymRow, TRUE);

    UpdateSymbolRange(hwndDlg, chFirst, chLast);

    if ((hwndDlg != NULL) && ((hdc = GetDC(hwndDlg)) != NULL))
    {
        LPINT lpdxp;
        HFONT hFont;
        UINT ch;

        hFont = (HFONT)SelectObject(hdc, sycm.hFont);
        lpdxp = (LPINT)sycm.rgdxp;

        Font_GetCharWidth32(hdc, chSymFirst, chSymLast, lpdxp,pCode);

        SelectObject(hdc, hFont);

        for (ch = (UINT) chSymFirst; ch <= (UINT) chSymLast; ch++, lpdxp++)
        {
            *lpdxp = (sycm.dxpBox - *lpdxp) / 2;
        }
        ReleaseDC(hwndDlg, hdc);
    }

    if (fRePaint)
    {
        InvalidateRect(hwndSB, NULL, TRUE);
        InvalidateRect(hwndCharGrid, NULL, TRUE);
    }

    return (TRUE);
}

////////////////////////////////////////////////////////////////////////////
//
//  ChFromSymLParam
//
//  Determines the character to select from the mouse position (lParam).
//
////////////////////////////////////////////////////////////////////////////

UINT ChFromSymLParam(
    PSYCM psycm,
    LPARAM lParam)
{
    return (min( cchSymRow - 1,
                 max(0, ((INT)LOWORD(lParam) - 1) / psycm->dxpBox) ) +
            min( cchSymCol - 1,
                 max(0, ((INT)HIWORD(lParam) - 1) / psycm->dypBox) ) *
            cchSymRow + chSymFirst);
}


////////////////////////////////////////////////////////////////////////////
//
//  DrawSymChOutlineHwnd
//
//  Gets a DC for hwnd, calls DrawSymChOutline.
//
////////////////////////////////////////////////////////////////////////////

VOID DrawSymChOutlineHwnd(
    PSYCM psycm,
    HWND hwnd,
    UTCHAR ch,
    BOOL fVisible,
    BOOL fFocus)
{
    HDC hdc = GetDC(hwnd);

    DrawSymChOutline(psycm, hdc, ch, fVisible, fFocus);
    ReleaseDC(hwnd, hdc);
}


//
// Get physical font dimension from the given logical font.
// This routine may scale down the logical font size to match the best
// possible viewable size of the given font (wchao).
//
BOOL GetViewableFontDimensions (
    HDC             hdc,
    int             dypInch,        // presentation device's logical pixel per inch
    LOGFONT*        plf,            // (in/out) requested logical font
    HFONT*          phFontOld,      // (out) -optional- original font in the dc
    HFONT*          phFontNew,      // (out) new created font
    TEXTMETRIC*     ptm)            // (out) physical dimensions of *phFontNew
{
    if (!phFontNew || !ptm)
        return FALSE;

    LONG    iHeightSave = plf->lfHeight;
    LONG    iDir = plf->lfHeight < 0 ? -1 : 1;
    HFONT   hFont = CreateFontIndirect(plf);
    HFONT   hFontOld = (HFONT)SelectObject(hdc, hFont);

    GetTextMetrics(hdc, ptm);


    // This loop is only needed by bitmap font as it was digitized (either scaling up or down)
    // by GDI to a greater resolution. This makes the character looks chunky (imagine how bad
    // it would be for "Small Fonts" scaled up to 13pt). We scale down the font until it
    // matches screen resolution.

    while (plf->lfHeight && ptm->tmDigitizedAspectY != dypInch)
    {
        plf->lfHeight -= iDir;      // scaling down
        DeleteObject(hFont);
        hFont = CreateFontIndirect(plf);
        SelectObject(hdc, hFont);
        GetTextMetrics(hdc, ptm);
    }

    if (ptm->tmDigitizedAspectY != dypInch || !ptm->tmHeight)
    {
        // If we get here. GDI is really screwed up.
        //
        plf->lfHeight = iHeightSave;
        DeleteObject(hFont);
        hFont = CreateFontIndirect(plf);
        SelectObject(hdc, hFont);
        GetTextMetrics(hdc, ptm);
    }

    *phFontNew = hFont;

    if (phFontOld)
        *phFontOld = hFontOld;

    return TRUE;
}



////////////////////////////////////////////////////////////////////////////
//
//  RecalcUCE
//
//  Recalculates fixed character map data (font info, sizes, etc.).
//
////////////////////////////////////////////////////////////////////////////

VOID RecalcUCE(
    HWND hwndDlg,
    PSYCM psycm,
    INT iCombo,
    BOOL fRedraw)
{
    HDC          hdc;
    TEXTMETRIC   tm;
    UINT ch;
    LPINT lpdxp;
    HFONT        hFont;
    LOGFONT      LogFont;
    LONG         iCurSel;
    int          dypInch;

    //
    //  Get rid of the old font handles.
    //
    if (hFontClipboard && (hFontClipboard == psycm->hFont))
    {
        fDelClipboardFont = TRUE;
    }
    if (psycm->hFont && (hFontClipboard != psycm->hFont))
    {
        DeleteObject(psycm->hFont);
    }
    if (psycm->hFontMag)
    {
        DeleteObject(psycm->hFontMag);
    }

    hdc = GetDC(hwndCharGrid);

    //  Get device y logical pixel per inch
    dypInch = GetDeviceCaps(hdc, LOGPIXELSY);

    //
    //  Set these to zero.
    //
    LogFont.lfWidth = LogFont.lfEscapement = LogFont.lfOrientation =
                      LogFont.lfWeight = 0;
    LogFont.lfItalic = LogFont.lfUnderline = LogFont.lfStrikeOut =
                       LogFont.lfOutPrecision = LogFont.lfClipPrecision =
                       LogFont.lfQuality = LogFont.lfPitchAndFamily = 0;

    //
    //  Let the facename and size define the font.
    //
    //  LogFont.lfCharSet = DEFAULT_CHARSET;

    //  Work around the GDI bug that assumes the font's default charset
    //  is always the system default locale.
    //
    LogFont.lfCharSet = Font_pList[iCombo].CharSet;

    //
    //  Get the facename from the combo box.
    //
    if(lstrcmpi(Font_pList[iCombo].szFaceName, L"SystemDefaultEUDCFont") == 0)
      lstrcpy(LogFont.lfFaceName, _T("MS Sans Serif"));
    else
      lstrcpy(LogFont.lfFaceName, Font_pList[iCombo].szFaceName);


    //
    //  Enable Block listbox and set defaults appropriately.
    //
    iCurSel = (LONG)SendDlgItemMessage( hwndDlg,
                                        ID_UNICODESUBSET,
                                        CB_GETCURSEL,
                                        0,
                                        0L );
    UpdateSymbolSelection( hwndDlg, TRUE);

    //
    //  Create the magnify font.
    //
    LogFont.lfHeight = psycm->dypMag - 5;        // Allow for whitespace.
    psycm->hFontMag = CreateFontIndirect(&LogFont);


    //
    //  Create the grid font
    //
    if (PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale())) == LANG_CHINESE)
    {
        LogFont.lfHeight = 16;
    }
    else
    {
        LogFont.lfHeight = psycm->dypBox - 6;    // 3-pixel top/bottom barring.
    }


    GetViewableFontDimensions(hdc, dypInch, &LogFont, &hFont, &psycm->hFont, &tm);


    //  Font with significant descent-ascent (round) ratio
    //  e.g. most Thai fonts, Traditional Arabic.

    if (tm.tmAscent && MulDiv(tm.tmDescent, 10, tm.tmAscent) > 4)
    {
        DeleteObject(psycm->hFont);
        LogFont.lfHeight = -LogFont.lfHeight;       // request it as character height
        GetViewableFontDimensions(hdc, dypInch, &LogFont, NULL, &psycm->hFont, &tm);
        LogFont.lfHeight = tm.tmHeight;             // now take the actual cell height
    }


    //  Maximum avg width allowed inside a grid box
    LONG    iMaxAveBoxWidth = MulDiv(tm.tmHeight, psycm->dxpBox, psycm->dypBox) - 2;   // 1-pixel side barrings
    LONG    tmHeightLast = tm.tmHeight;


    //  Narrow down allowance for non-symbol proportional font
    if (tm.tmCharSet != SYMBOL_CHARSET &&
        tm.tmAveCharWidth != tm.tmMaxCharWidth)
        iMaxAveBoxWidth = MulDiv(iMaxAveBoxWidth, 70, 100); // 70% looks good for most fonts specially FE's.


    //  Try & Fit the font in grid box
    while (tm.tmAveCharWidth > iMaxAveBoxWidth)
    {
        DeleteObject(psycm->hFont);
        LogFont.lfHeight--;
        GetViewableFontDimensions(hdc, dypInch, &LogFont, NULL, &psycm->hFont, &tm);

        // Nothing better, let's get out of here.
        if (tm.tmHeight == tmHeightLast)
            break;

        tmHeightLast = tm.tmHeight;
    }

    psycm->xpCh = 2;

    psycm->ypCh = (psycm->dypBox - tm.tmHeight + 1) / 2;    // y offset display position in a grid box

    lpdxp = (LPINT) psycm->rgdxp;

    Font_GetCharWidth32(hdc, chSymFirst, chSymLast, lpdxp,pCode);

    SelectObject(hdc, hFont);

    for (ch = (UINT) chSymFirst; ch <= (UINT) chSymLast; ch++, lpdxp++)
    {
        *lpdxp = (psycm->dxpBox - *lpdxp) / 2;
    }

    ReleaseDC(hwndCharGrid, hdc);

    psycm->xpMagCurr = 0;              // No magnification data

    // Set the tooltip window for the character grid
    ResizeTipsArea(hwndCharGrid, &sycm);

    if (fRedraw)
    {
        InvalidateRect(hwndCharGrid, NULL, TRUE);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  DrawSymbolMap
//
//  Draws all of the pieces of the symbol character map.
//
////////////////////////////////////////////////////////////////////////////

VOID DrawSymbolMap(
    PSYCM psycm,
    HDC hdc)
{
    BOOL fFocus;

    DrawSymbolGrid(psycm, hdc);
    DrawSymbolChars(psycm, hdc);
    //
    //  We need to force the focus rect to paint if we have the focus
    //  since the old focus rect has been drawn over already.
    //
    if (fFocus = psycm->fFocusState)
    {
        psycm->fFocusState = FALSE;
    }

    if (psycm->fMagnify == FALSE)
    {
        DrawSymChOutline(psycm, hdc, psycm->chCurr, TRUE, fFocus);
    }
}

////////////////////////////////////////////////////////////////////////////
//
//  DrawSymbolGrid
//
//  Draws the symbol character map grid.
//
////////////////////////////////////////////////////////////////////////////

VOID DrawSymbolGrid(
    PSYCM psycm,
    HDC hdc)

{
    INT cli;                // count of lines
    INT xp, yp;
    INT dxpBox = psycm->dxpBox;
    INT dypBox = psycm->dypBox;
    HPEN hpenOld;
    INT xpos, ypos;

    hpenOld = (HPEN) SelectObject(hdc, CreatePen(PS_SOLID,
                                                 1,
                                                 GetSysColor(COLOR_WINDOWFRAME)));
    //
    //  Draw horizontal lines.
    //
    xp = psycm->dxpCM + 1;
    yp = 1;
    ypos = 0;
    cli = cchSymCol+1;
    while (cli--)
    {
        MoveToEx(hdc, 1, yp, NULL);
        LineTo(hdc, xp, yp);
        yp += dypBox;
        ypos += 1;
     }

    //
    //  Draw vertical lines.
    //
    yp = psycm->dypCM;
    xp = 1;
    xpos = 0;
    cli = cchSymRow+1;
    while (cli--)
    {
        MoveToEx(hdc, xp, 1, NULL);
        LineTo(hdc, xp, yp);
        xp += dxpBox;
        xpos += 1;
    }

    DeleteObject(SelectObject(hdc, hpenOld));
}

////////////////////////////////////////////////////////////////////////////
//
//  DrawSymbolChars
//
//  Draws the symbol character map.
//
////////////////////////////////////////////////////////////////////////////

VOID DrawSymbolChars(
    PSYCM psycm,
    HDC hdc)
{
    INT    dxpBox  = psycm->dxpBox;
    INT    dypBox  = psycm->dypBox;

    INT    cch;
    INT    x, y;
    INT    yp;
    TCHAR   ch;

    HFONT  hFontOld;
    RECT   rect;
    LPRECT lprect = (LPRECT) &rect;
    LPINT lpdxp;

    if ((pCode == NULL) || (*pCode == '\0'))
    {
        return;
    }

    //
    // Setup the font and colors.
    //
    hFontOld = (HFONT) SelectObject(hdc, psycm->hFont);
    SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
    SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
    SetBkMode(hdc, OPAQUE);

    //
    // Draw characters.
    //
    cch = 1;
    ch = chSymFirst;

    lpdxp = (LPINT) psycm->rgdxp;

    rect.top = 2;
    yp = psycm->ypCh;
    rect.bottom = rect.top + dypBox - 1;

    for (y = 0; y++ < cchSymCol;)
    {
        rect.left  = psycm->xpCh;
        rect.right = rect.left + dxpBox - 1;
        for (x = 0; (x++ < cchSymRow) && (ch <= chSymLast);)
        {
          WCHAR wc = pCode[ch];
          ConvertAnsifontToUnicode(hwndCharGrid, (char*)&pCode[ch], &wc);
          ExtTextOutW( hdc,
                       rect.left + (*lpdxp++),
                       yp,
                       ETO_OPAQUE | ETO_CLIPPED,
                       lprect,
                       &wc,
                       1,
                       NULL );
/*
// Unify display using ExtTextOutW
            if(gFontType & SYMBOL_FONTTYPE)
            {
                char mb = (char)pCode[ch];
                ExtTextOutA( hdc,
                             rect.left + (*lpdxp++),
                             yp,
                             ETO_OPAQUE | ETO_CLIPPED,
                             lprect,
                             &mb,
                             1,
                             NULL );
            }
            else if(gFontType & OEM_FONTTYPE)
            {
                char mb;
                WideCharToMultiByte(CP_OEMCP, WC_NO_BEST_FIT_CHARS,
                    &pCode[ch], 1, &mb, 1, NULL, NULL);
                ExtTextOutA( hdc,
                             rect.left + (*lpdxp++),
                             yp,
                             ETO_OPAQUE | ETO_CLIPPED,
                             lprect,
                             &mb,
                             1,
                             NULL );
            }
            else
            {
                ExtTextOutW( hdc,
                             rect.left + (*lpdxp++),
                             yp,
                             ETO_OPAQUE | ETO_CLIPPED,
                             lprect,
                             &(pCode[ch]),
                             1,
                             NULL );
            }
*/
           ch++;
           if(ch > chRangeLast)
           {
              return;
           }
           rect.left  += dxpBox;
           rect.right += dxpBox;
        }
        yp += dypBox;
        rect.top += dypBox;
        rect.bottom += dypBox;
    }

    SelectObject(hdc, hFontOld);
}

////////////////////////////////////////////////////////////////////////////
//
//  DrawSymChOutline
//
//  Draws an outline around the symbol in the character map.  If fVisible,
//  then it draws the outline, otherwise it erases it.
//
////////////////////////////////////////////////////////////////////////////

VOID DrawSymChOutline(
    PSYCM psycm,
    HDC hdc,
    UTCHAR ch,
    BOOL fVisible,
    BOOL fFocus)
{
    HBRUSH hbrOld;
    RECT rc;
    INT dxpBox = psycm->dxpBox;
    INT dypBox = psycm->dypBox;

    hbrOld = (HBRUSH)SelectObject( hdc,
                           CreateSolidBrush(GetSysColor( fVisible
                                                           ? COLOR_WINDOWFRAME
                                                           : COLOR_WINDOW )) );
    ch -= chSymFirst;

    rc.left   = (ch % cchSymRow) * dxpBox + 2;
    rc.right  = rc.left + dxpBox - 1;
    rc.top    = (ch / cchSymRow) * dypBox + 2;
    rc.bottom = rc.top  + dypBox - 1;

    //
    //  Draw selection rectangle.
    //
    PatBlt(hdc, rc.left,      rc.top - 2,    dxpBox - 1, 1,          PATCOPY);
    PatBlt(hdc, rc.left,      rc.bottom + 1, dxpBox - 1, 1,          PATCOPY);
    PatBlt(hdc, rc.left - 2,  rc.top,        1,          dypBox - 1, PATCOPY);
    PatBlt(hdc, rc.right + 1, rc.top,        1,          dypBox - 1, PATCOPY);

    DeleteObject(SelectObject(hdc, GetStockObject(NULL_BRUSH)));

    //
    //  Deal with the focus rectangle.
    //
    if (fFocus != psycm->fFocusState)
    {
        DrawFocusRect(hdc, &rc);
        psycm->fFocusState = fFocus;
    }

    SelectObject(hdc, hbrOld);
}

////////////////////////////////////////////////////////////////////////////
//
//  MoveSymbolSel
//
//  Changes the current symbol selection.  Handles drawing of magnified
//  characters.
//
////////////////////////////////////////////////////////////////////////////

VOID MoveSymbolSel(
    PSYCM psycm,
    UTCHAR chNew,
    BOOL fRepaint)
{
    HDC hdc;
    HDC hdcMag = psycm->hdcMag;
    RECT rc;
    HFONT hFontOld;
    HFONT hFontMag;                    // old font in memory dc
    HPEN hpenOld;

    UTCHAR chNorm = chNew - chSymFirst + cchSymRow;

    INT dxpMag = psycm->dxpMag;        // for quick reference
    INT dypMag = psycm->dypMag;
    INT ypMemSrc  = psycm->ypDest;
    INT ypMemDest = ypMemSrc ^ dypMag;
    INT xpCurr  = psycm->xpMagCurr;
    INT ypCurr  = psycm->ypMagCurr;
    INT xpNew   = psycm->xpCM + (psycm->dxpBox *  (chNorm % cchSymRow));
    INT ypNew   = psycm->ypCM + (psycm->dypBox * ((chNorm / cchSymRow) - 1));
    INT dxpCh;  // width of extra character space (used to center char in box)
    INT dypCh;
    SIZE sz;

    DOUTL("MoveSymbolSel: In\n");

    if(chNew > chRangeLast)
    {
        return;
    }

    if ((chNew == (UTCHAR)psycm->chCurr) && FMagData(psycm)
        && (fRepaint == FALSE))
    {
        DOUTL("MoveSymbolSel: ch == cur && fMag... exiting\n");
        return;
    }

    hdc = GetDC(hwndDialog);

    //
    //  Setup the magnified font character.
    //
    hFontMag = (HFONT)SelectObject(hdcMag, psycm->hFontMag);

    if (pCode != NULL   &&  chNew <= chRangeLast) {
        GetTextExtentPointW(hdcMag, &pCode[chNew], 1, &sz);
    }

    if (PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale())) == LANG_CHINESE)
    {
        dxpCh = (dxpMag - (INT)sz.cx) / 2 - 2;
        dypCh = (dypMag - (INT)sz.cy) / 2 - 2;
    }
    else
    {
        dxpCh = (dxpMag - (INT)sz.cx) / 2 - 1;
        dypCh = (dypMag - (INT)sz.cy) / 2 - 1;
    }
    hpenOld = (HPEN)SelectObject(hdc, CreatePen( PS_SOLID,
                                           1,
                                           GetSysColor(COLOR_WINDOWFRAME) ));
    hFontOld = (HFONT)SelectObject(hdc, psycm->hFontMag);

    //
    //  Copy screen data to offscreen bitmap.
    //
    BitBlt(hdcMag, 0, ypMemDest, dxpMag, dypMag, hdc, xpNew, ypNew, SRCCOPY);

    //
    //  Setup DC.
    //
    SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
    SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
    SetBkMode(hdc, OPAQUE);


    if (FMagData(psycm))
    {
        INT xpT  = xpNew - xpCurr;     // point of overlap in offscreen data
        INT ypT  = ypNew - ypCurr;
        INT dxpT = dxpMag - abs(xpT);  // size of overlap
        INT dypT = dypMag - abs(ypT);


        DOUTL("MoveSymbolSel: FMagData\n");

        if ((dxpT > 0) && (dypT > 0))
        {
            INT xpTmax,  ypTmax;   // max(0, xpT);
            INT xpTmin,  ypTmin;   // min(0, xpT);
            INT xpTnmin, ypTnmin;  // min(0, -xpT);

            DOUTL("MoveSymbolSel: dxpT > 0 && dypT > 0\n");

            if (xpT < 0)
            {
                xpTnmin = - (xpTmin = xpT);
                xpTmax  = 0;
            }
            else
            {
                xpTmax  = xpT;
                xpTnmin = xpTmin = 0;
            }
            if (ypT < 0)
            {
                ypTnmin = - (ypTmin = ypT);
                ypTmax  = 0;
            }
            else
            {
                ypTmax  = ypT;
                ypTnmin = ypTmin = 0;
            }

            rc.left  = xpTmax;
            rc.right = xpTmin + dxpMag;
            rc.top   = ypTmax + ypMemSrc;
            rc.bottom= ypTmin + dypMag + ypMemSrc;

            //
            //  Copy overlapping offscreen data.
            //
            BitBlt( hdcMag,
                    xpTnmin,
                    ypTnmin + ypMemDest,
                    dxpT,
                    dypT,
                    hdcMag,
                    xpTmax,
                    ypTmax  + ypMemSrc,
                    SRCCOPY );

            //
            //  Print part of char over old screen data.
            //
            WCHAR wc = pCode[chNew];
//            ConvertAnsifontToUnicode(hwndDialog, psycm->hFontMag, (char*)&pCode[chNew], &wc);
            ConvertAnsifontToUnicode(hwndDialog, (char*)&pCode[chNew], &wc);
            ExtTextOutW( hdcMag,
                         xpT + dxpCh,
                         ypT + dypCh + ypMemSrc,
                         ETO_OPAQUE | ETO_CLIPPED,
                         (LPRECT)&rc,
                         &wc,
                         1,
                         NULL );

        }
        //
        //  Restore old screen data.
        //
        BitBlt(hdc, xpCurr, ypCurr, dxpMag, dypMag, hdcMag, 0, ypMemSrc, SRCCOPY);
    }

    rc.right  = (psycm->xpMagCurr = rc.left = xpNew) + dxpMag - 2;
    rc.bottom = (psycm->ypMagCurr = rc.top  = ypNew) + dypMag - 2;

    //
    //  The rectangle.
    //
    MoveToEx(hdc, rc.left, rc.top, NULL);
    LineTo(hdc, rc.left, rc.bottom - 1);
    LineTo(hdc, rc.right - 1, rc.bottom - 1);
    LineTo(hdc, rc.right - 1, rc.top);
    LineTo(hdc, rc.left, rc.top);

    //
    //  The shadow.
    //
    MoveToEx(hdc, rc.right, rc.top + 1, NULL);
    LineTo(hdc, rc.right, rc.bottom);
    LineTo(hdc, rc.left, rc.bottom);
    MoveToEx(hdc, rc.right + 1, rc.top + 2, NULL);
    LineTo(hdc, rc.right + 1, rc.bottom + 1);
    LineTo(hdc, rc.left + 1, rc.bottom + 1);

    rc.left++;
    rc.top++;
    rc.right--;
    rc.bottom--;

    //
    //  Draw magnified character on screen.
    //
    WCHAR wc = pCode[chNew];
    ConvertAnsifontToUnicode(hwndCharGrid, (char*)&pCode[chNew], &wc);
    ExtTextOutW( hdc,
                 xpNew + dxpCh,
                 ypNew + dypCh,
                 ETO_OPAQUE | ETO_CLIPPED,
                 (LPRECT)&rc,
                 &(wc),
                 1,
                 NULL );

    psycm->ypDest = ypMemDest;

    DeleteObject(SelectObject(hdc, hpenOld));
    SelectObject(hdc, hFontOld);
    SelectObject(hdcMag, hFontMag);

    UpdateKeystrokeText(hdc, chNew, TRUE);

    ReleaseDC(hwndDialog, hdc);

    psycm->chCurr = chNew;
    psycm->fMagnify = TRUE;
    DOUTL("MoveSymbolSel: Leaving\n");
}

////////////////////////////////////////////////////////////////////////////
//
//  RestoreSymMag
//
//  Restores the screen data under the magnifier.
//
////////////////////////////////////////////////////////////////////////////

VOID RestoreSymMag(
    HWND  hWndGrid,
    PSYCM psycm)
{
    if (FMagData(psycm))
    {
        HDC hdc = GetDC(hwndDialog);

        // Does not overwrite scroll bar
        BitBlt( hdc,
                psycm->xpMagCurr,
                psycm->ypMagCurr,
                psycm->dxpMag,
                psycm->dypMag,
                psycm->hdcMag,
                0,
                psycm->ypDest,
                SRCCOPY );

        ReleaseDC(hwndDialog, hdc);

        psycm->xpMagCurr = 0;     // flag - no data offscreen (see FMagData)
    }
}

////////////////////////////////////////////////////////////////////////////
//
//  GetEditText
//
//  Returns HANDLE containing the text in the edit control.
//
//  NOTE: Caller is responsible for freeing this handle!
//
////////////////////////////////////////////////////////////////////////////

HANDLE GetEditText(
    HWND hwndDlg)
{
    INT cchText;
    HWND hwndEditCtl;
    HANDLE hmem;
    LPTSTR lpstrText;
    DWORD dwSel;

#ifndef DISABLE_RICHEDIT
    hwndEditCtl = GetDlgItem(hwndDlg, ID_STRING);
#else
    hwndEditCtl = GetDlgItem(hwndDlg, IDC_EDIT);
#endif
    cchText = GetWindowTextLength(hwndEditCtl);

    hmem = GlobalAlloc(0, CTOB((cchText + 1)));

    lpstrText = (LPTSTR)GlobalLock(hmem);

    cchText = GetWindowText(hwndEditCtl, lpstrText, cchText+1);

    dwSel = (DWORD)SendMessage(hwndEditCtl, EM_GETSEL, 0, 0L);

    if (LOWORD(dwSel) != HIWORD(dwSel))
    {
        //
        //  If there is a selection, then only get the selected text.
        //
        *(lpstrText + HIWORD(dwSel)) = TEXT('\0');
        lstrcpy(lpstrText, lpstrText + LOWORD(dwSel));
    }

    GlobalUnlock(hmem);

    if (cchText == 0)
    {
        hmem = GlobalFree(hmem);
    }

    return (hmem);
}

////////////////////////////////////////////////////////////////////////////
//
//  PointsToHeight
//
//  Calculates the height in pixels of the specified point size for the
//  current display.
//
////////////////////////////////////////////////////////////////////////////

INT PointsToHeight(
    INT iPoints)
{
    HDC hdc;
    INT iHeight;

    hdc = GetDC(HWND_DESKTOP);
    iHeight = MulDiv(iPoints, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(HWND_DESKTOP, hdc);
    return (iHeight);
}

////////////////////////////////////////////////////////////////////////////
//
//  UpdateKeystrokeText
//
//  Calculates and updates the text string displayed in the Keystroke
//  field of the status bar.  It repaints the status field if fRedraw is
//  TRUE.
//
////////////////////////////////////////////////////////////////////////////

VOID UpdateKeystrokeText(
    HDC hdc,
    UTCHAR chNew,
    BOOL fRedraw)
{
    if (!pCode || *pCode == '\0')
    {
       LoadString(hInst,
                  IDS_NOCHARFOUND,
                  szKeystrokeText,
                  sizeof(szKeystrokeText));
    }
    else
    {
        char  szMessageA[MAX_PATH*2];
        WCHAR szMessageW[MAX_PATH];
        LONG  lCodePage;
        BYTE  mb[2];
        WORD  wCharCode;

        szAlt[0] = 0;

        if(gFontType & SYMBOL_FONTTYPE)
        {
           LoadString(hInst, IDS_SYMBOLSET, szMessageW, MAX_PATH);
           wsprintf(szKeystrokeText,TEXT("%s : 0x%2X"), szMessageW, pCode[chNew]);
        }
        else
        {
            GetUName(pCode[chNew],szMessageW);
            /* MultiByteToWideChar(CP_ACP, 0, szMessageA, -1, szMessageW, MAX_PATH);*/

            if((lCodePage = CodePage_GetCurCodePageVal()) != UNICODE_CODEPAGE)
            {
                int i;

                i = WideCharToMultiByte(
                        lCodePage, WC_NO_BEST_FIT_CHARS,
                        &pCode[chNew], 1,
                        (char*)mb, 2,
                        NULL, NULL );

                if(i == 1)
                {
                    wCharCode = mb[0];               // single-byte
                }
                else
                {
                    wCharCode = (mb[0]<<8) | mb[1];  // double-byte
                }

                wsprintf(szKeystrokeText,TEXT("U+%04X (0x%2X): %s"),
                         pCode[chNew], wCharCode,szMessageW);
            }
            else
            {
                wsprintf(szKeystrokeText, TEXT("U+%04X: %s"),
                         pCode[chNew], szMessageW);
            }

            // add keystroke info to high ANSI char
            // exclude Control chars
            if((pCode[chNew] > 0x009F) &&
               (WideCharToMultiByte(gKBD_CP, WC_NO_BEST_FIT_CHARS,
                                    &pCode[chNew],1,
                                    (char*)mb,2,NULL,NULL) == 1)
               && (*mb > 0x7F))
            {
               LoadString(hInst, IDS_ALT, szMessageW, MAX_PATH);
                           wsprintf(szAlt, TEXT("%s: Alt+0%d"), szMessageW, *mb);
            }
        }

    }

    if (fRedraw)
    {
        PaintStatusLine(hdc, FALSE, TRUE);
    }
}

////////////////////////////////////////////////////////////////////////////
//
//  SubSetChanged
//
//  Sets the ANSI bit if appropriate and then calls UpdateSymbolSelection
//  and then repaints the window.
//
//  Repaints Keystroke field if HWND != NULL.
//
//  Redraws the char grid.
//
////////////////////////////////////////////////////////////////////////////

VOID SubSetChanged(
    HWND hwnd)
{
    HDC hdc;

    ExitMagnify(hwndCharGrid, &sycm);
    UpdateSymbolSelection(hwnd, TRUE);
    if ((hwnd != NULL) && ((hdc = GetDC(hwnd)) != NULL))
    {
        LPINT lpdxp;
        HFONT hFont;
        UINT ch;

        hFont = (HFONT)SelectObject(hdc, sycm.hFont);
        lpdxp = (LPINT)sycm.rgdxp;

        if (pCode)
        {
            Font_GetCharWidth32(hdc, chSymFirst, chSymLast, lpdxp,pCode);
        }

        SelectObject(hdc, hFont);

        for (ch = (UINT) chSymFirst; ch <= (UINT) chSymLast; ch++, lpdxp++)
        {
            *lpdxp = (sycm.dxpBox - *lpdxp) / 2;
        }
        ReleaseDC(hwnd, hdc);
    }

    if(sycm.chCurr != 0)
    {
        sycm.chCurr = 0;
        HDC hDC = GetDC(hwndDialog);
        UpdateKeystrokeText(hDC, sycm.chCurr, FALSE);
        ReleaseDC(hwndDialog, hDC);
        InvalidateRect(hwndDialog, NULL, TRUE);
    }
    InvalidateRect(hwndCharGrid, NULL, TRUE);
}

////////////////////////////////////////////////////////////////////////////
//
//  UpdateSymbolSelection
//
//  Updates the values of the following global values:
//      chRangeFirst
//      chRangeLast
//  Subsets in the Unicode character set have different numbers of
//  characters.  We have to do some bounds checking in order to set an
//  appropriate sycm.chCurr value.  The "Keystroke" status field is
//  updated.
//
//  Repaints Keystroke field if HWND != NULL.
//
////////////////////////////////////////////////////////////////////////////

VOID UpdateSymbolSelection(
    HWND hwnd,
    BOOL bUpdateRange)
{
    int iCmd = SW_HIDE;
    HWND hwndSB;
    UINT chFirst, chLast;

    chRangeFirst = 0;

    if(bUpdateRange)
    {
        LPWSTR lpszSubsetChar=NULL;
        INT    nSubsetChar = 0;
        BOOL   bLineBreak=FALSE;

        URANGE *pCMapTab = NULL;
        INT    nNumofCMapTab = 0;

        LONG lCodePage;

        lCodePage = CodePage_GetCurSelCodePage(hwnd,ID_VIEW);

        Subset_GetUnicodeCharsToDisplay(
                    hwnd,
                    ID_UNICODESUBSET,
                    lCodePage,
                    &lpszSubsetChar,
                    (unsigned int*)&nSubsetChar,
                    &bLineBreak);

        Font_GetCurCMapTable(hwnd,ID_FONT,&pCMapTab,(unsigned int*)&nNumofCMapTab);

        pCode = Display_CreateDispBuffer(
                        lpszSubsetChar,
                        nSubsetChar,
                        pCMapTab,
                        nNumofCMapTab,
                        bLineBreak);
    }

    if(pCode == NULL || *pCode == 0)
    {
           chRangeLast = 0;
    }
    else
    {
           chRangeLast = lstrlen(pCode)-1;
    }

    chFirst = chRangeFirst;

    chLast = chFirst + cchFullMap - 1;
    chLast = min(chLast, chRangeLast);

    hwndSB = GetDlgItem(hwnd, ID_MAPSCROLL);

    if (chLast != chRangeLast)
    {
        int i;

        iCmd = SW_SHOW;
        SetScrollPos(hwndSB, SB_CTL, 0, FALSE);
        i = (chRangeLast - chRangeFirst + 1) - cchFullMap;
        if (i < cchSymRow)
        {
            i = 1;
        }
        else
        {
            i = (i+cchSymRow-1) / cchSymRow;
        }

        SetScrollRange(hwndSB, SB_CTL, 0, i, FALSE);
        InvalidateRect(hwndSB, NULL, FALSE);
    }

    ShowWindow(hwndSB, iCmd);
    UpdateSymbolRange(hwnd, chFirst, chLast);

    //adjust character position
    {
        LPINT lpdxp;
        HFONT hFont;
        UINT ch;
        HDC hdc = GetDC(hwnd);

        if (hdc)
        {
          hFont = (HFONT)SelectObject(hdc, sycm.hFont);
          lpdxp = (LPINT)sycm.rgdxp;

          if (pCode)
          {
            Font_GetCharWidth32(hdc, chFirst, chLast, lpdxp,pCode);
          }

          SelectObject(hdc, hFont);

          for (ch = (UINT) chFirst; ch <= (UINT) chLast; ch++, lpdxp++)
          {
            *lpdxp = (sycm.dxpBox - *lpdxp) / 2;
          }
          ReleaseDC(hwnd, hdc);
        }
    }
}

VOID SetSpecificSelection(HWND hwnd,
                          int iFrom,
                          int iTo)
{
    int iCmd = SW_HIDE;
    HWND hwndSB;
    UINT chFirst, chLast;

    chRangeFirst = 0;

    LPWSTR lpszSubsetChar=NULL;
    INT    nSubsetChar = 0;
    BOOL   bLineBreak=FALSE;

    URANGE *pCMapTab = NULL;
    INT    nNumofCMapTab = 0;

    LONG lCodePage;

    lCodePage = CodePage_GetCurSelCodePage(hwnd,ID_VIEW);

    Subset_GetUnicodeCharsToDisplay(
                hwnd,
                ID_UNICODESUBSET,
                lCodePage,
                &lpszSubsetChar,
                (unsigned int*)&nSubsetChar,
                &bLineBreak);

    Font_GetCurCMapTable(hwnd,ID_FONT,&pCMapTab,(unsigned int*)&nNumofCMapTab);

    pCode = Display_CreateSubsetDispBuffer(
                    lpszSubsetChar,
                    nSubsetChar,
                    pCMapTab,
                    nNumofCMapTab,
                    bLineBreak,
                    iFrom,
                    iTo);

    if (pCode && pCode[0])
    {
        chRangeLast = lstrlen(pCode)-1;
    }

    chFirst = chRangeFirst;

    chLast = chFirst + cchFullMap - 1;
    chLast = min(chLast, chRangeLast);

    hwndSB = GetDlgItem(hwnd, ID_MAPSCROLL);

    if (chLast != chRangeLast)
    {
        int i;

        iCmd = SW_SHOW;
        SetScrollPos(hwndSB, SB_CTL, 0, FALSE);
        i = (chRangeLast - chRangeFirst + 1) - cchFullMap;
        if (i < cchSymRow)
        {
            i = 1;
        }
        else
        {
            i = (i+cchSymRow-1) / cchSymRow;
        }

        SetScrollRange(hwndSB, SB_CTL, 0, i, FALSE);
        InvalidateRect(hwndSB, NULL, FALSE);
    }

    ShowWindow(hwndSB, iCmd);
    UpdateSymbolRange(hwnd, chFirst, chLast);

    //adjust character position
    {
        LPINT lpdxp;
        HFONT hFont;
        UINT ch;
        HDC hdc = GetDC(hwnd);

        if (hdc)
        {
          hFont = (HFONT)SelectObject(hdc, sycm.hFont);
          lpdxp = (LPINT)sycm.rgdxp;

          if (pCode)
          {
            Font_GetCharWidth32(hdc, chFirst, chLast, lpdxp,pCode);
          }

          SelectObject(hdc, hFont);

          for (ch = (UINT) chFirst; ch <= (UINT) chLast; ch++, lpdxp++)
          {
            *lpdxp = (sycm.dxpBox - *lpdxp) / 2;
          }
          ReleaseDC(hwnd, hdc);
        }
    }

}

////////////////////////////////////////////////////////////////////////////
//
//  UpdateSymbolRange
//
//  Updates the values of the following global values:
//      chSymFirst
//      chSymLast
//      sycm.chCurr
//  Subsets in the Unicode character set have different numbers of
//  characters.  We have to do some bounds checking in order to set an
//  appropriate sycm.chCurr value.  The "Keystroke" status field is
//  updated.
//
//  Repaints Keystroke field if HWND != NULL.
//
////////////////////////////////////////////////////////////////////////////

VOID UpdateSymbolRange(
    HWND hwnd,
    INT FirstChar,
    INT LastChar)
{
    UTCHAR chSymOffset;

    chSymOffset = sycm.chCurr - chSymFirst;

    chSymFirst = (UTCHAR)FirstChar;
    chSymLast = (UTCHAR)LastChar;

    sycm.chCurr = chSymOffset + chSymFirst;

    if (sycm.chCurr > chSymLast)
    {
        sycm.chCurr = chSymFirst;
    }
    while(sycm.chCurr>chSymFirst && pCode[sycm.chCurr] == L' ')
             --sycm.chCurr;

    if (hwnd != NULL)
    {
        HDC hdc;

        hdc = GetDC(hwnd);
        UpdateKeystrokeText(hdc, sycm.chCurr, TRUE);
        ReleaseDC(hwnd, hdc);
    }
    else
    {
        UpdateKeystrokeText(NULL, sycm.chCurr, FALSE);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  PaintStatusLine
//
//  Paints the Help and Keystroke fields in the status bar.
//
//  Repaints Help field if fHelp == TRUE.
//  Repaints Keystroke field if fKeystroke == TRUE.
//
////////////////////////////////////////////////////////////////////////////

VOID PaintStatusLine(
    HDC hdc,
    BOOL fHelp,
    BOOL fKeystroke)
{
    HFONT hfontOld = NULL;
    RECT rect;
    INT dyBorder;

    dyBorder = GetSystemMetrics(SM_CYBORDER);

    if (hfontStatus)
    {
        hfontOld = (HFONT)SelectObject(hdc, hfontStatus);
    }

    //
    //  Set the text and background colors.
    //
    SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
    SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));

    if (fKeystroke)
    {
        //
        //  Now the character text, with a gray background.
        //
        rect.top    = rcStatusLine.top + 3 * dyBorder;
        rect.bottom = rcStatusLine.bottom - 3 * dyBorder;
        rect.left   = 9 * dyBorder;
        rect.right  = rcStatusLine.right - 9 * dyBorder;

        ExtTextOut( hdc,
                    rect.left + dyBorder * 2,
                    rect.top,
                    ETO_OPAQUE | ETO_CLIPPED,
                    &rect,
                    szKeystrokeText,
                    lstrlen(szKeystrokeText),
                    NULL );

        //
        //  Now the keystroke Alt+ text, with a gray background.
        //
        if(szAlt[0])
        {
          rect.left   += dxHelpField;
/*
          ExtTextOut( hdc,
                    rect.left + dyBorder * 2,
                    rect.top,
                    ETO_OPAQUE | ETO_CLIPPED,
                    &rect,
                    szAlt,
                    lstrlen(szAlt),
                    NULL );
*/
          UINT ta = GetTextAlign(hdc);
          UINT tb = ta | TA_RIGHT;
          SetTextAlign(hdc, tb);
          ExtTextOut( hdc,
                    rect.right - dyBorder * 2,
                    rect.top,
                    ETO_OPAQUE,
                    &rect,
                    szAlt,
                    lstrlen(szAlt),
                    NULL );
          SetTextAlign(hdc, ta); 
        }
    }

    if (hfontOld)
    {
        SelectObject(hdc, hfontOld);
    }
}

////////////////////////////////////////////////////////////////////////////
//
//  DoHelp
//
//  Invokes help if fInvokeHelp is true, or dismisses help if fInvokeHelp
//  is FALSE.
//
////////////////////////////////////////////////////////////////////////////

VOID DoHelp(
    HWND hWnd,
    BOOL fInvokeHelp)
{
        if (fInvokeHelp)
        {
            HtmlHelpA(GetDesktopWindow(), "charmap.chm", HH_DISPLAY_TOPIC, 0L);
//BUGBUG    HtmlHelp(hWnd, ChmHelpPath, HH_DISPLAY_TOPIC, 0L);
        }
        else
        {
            HtmlHelpA(GetDesktopWindow(), "charmap.chm", HHWIN_PROP_POST_QUIT, 0L);
//BUGBUG    HtmlHelp(hWnd, ChmHelpPath, HHWIN_PROP_POST_QUIT, 0L);
        }
}

////////////////////////////////////////////////////////////////////////////
//
//  ExitMagnify
//
//  Releases mouse capture, exits magnify mode, and restores the cursor.
//
////////////////////////////////////////////////////////////////////////////

VOID ExitMagnify(
    HWND hWnd,
    PSYCM psycm)
{
    BOOL fFocus;
    //
    //  Release capture, remove magnified character, restore cursor.
    //
    ReleaseCapture();
    RestoreSymMag(hWnd, psycm);
    psycm->fFocusState = TRUE;
    if (fFocus = psycm->fFocusState)
    {
        psycm->fFocusState = FALSE;
    }
    DrawSymChOutlineHwnd(psycm, hWnd, psycm->chCurr, TRUE, fFocus);
    if (psycm->fCursorOff)
    {
        ShowCursor(TRUE);
    }
    psycm->fMouseDn = psycm->fCursorOff = FALSE;
    psycm->fMagnify = FALSE;
}

////////////////////////////////////////////////////////////////////////////
//
//  SetEditCtlFont
//
//  Creates a font for the Edit control that visually matches the handle
//  given, but is guaranteed not to be bigger than the size of the edit
//  control.
//
////////////////////////////////////////////////////////////////////////////

void SetEditCtlFont(
    HWND hwndDlg,
    int idCtl,
    HFONT hfont)
{
    static HFONT hfNew = NULL;
    LOGFONT lfNew;
    HWND hwndCtl = GetDlgItem(hwndDlg, idCtl);
    RECT rc;

    if (hfNew != NULL)
    {
        DeleteObject(hfNew);
    }

    GetWindowRect(hwndCtl, &rc);

    if (GetObject(hfont, sizeof(lfNew), &lfNew) != 0)
    {
        lfNew.lfHeight = rc.bottom - rc.top - 8;
        lfNew.lfWidth  = lfNew.lfEscapement = lfNew.lfOrientation =
        lfNew.lfWeight = 0;

        hfNew = CreateFontIndirect(&lfNew);
    }
    else
    {
        hfNew = hfont;
    }

    SendMessage(hwndCtl, WM_SETFONT, (WPARAM)hfNew, (LPARAM)TRUE);

    if (hfNew == hfont)
    {
        hfNew = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////
//
//  SetSearched
//
////////////////////////////////////////////////////////////////////////////
void SetSearched(void)
{
  fSearched = TRUE;
}

////////////////////////////////////////////////////////////////////////////
//
//  ComputeExpandedAndShrunkHeight
//  Computes the height of the dialog in expanded and shrunk state
//
////////////////////////////////////////////////////////////////////////////
void ComputeExpandedAndShrunkHeight(HWND hDlg)
{
    RECT rDlg, rButton;

    // Initially dialog is created in expanded state so expanded height is
    // same as current dialog height
    GetWindowRect(hDlg, &rDlg);
    gwExpandedHeight = rDlg.bottom - rDlg.top;

    // Shrunk height of dialog is y position of Advanced checkbox plus
    // height of status line plus some separator
    GetWindowRect(GetDlgItem(hDlg, ID_ADVANCED), &rButton);
    gwNormalHeight = rButton.bottom - rDlg.top + dyStatus
        + GetSystemMetrics(SM_CYFRAME);
}

////////////////////////////////////////////////////////////////////////////
//
//  ShowHideAdvancedControls
//  Toggles ID_ADVANCED checkbox and resizes the dialog accordingly
//
////////////////////////////////////////////////////////////////////////////
int ShowHideAdvancedControls(
                HWND hWnd,
                WPARAM wParam,
                LPARAM lParam)
{
    RECT    rectParent;
    int     iCheckState;

    // Get Current Check State
    iCheckState = (int)SendMessage(GetDlgItem(hWnd, ID_ADVANCED),
        BM_GETCHECK,
        (WPARAM)0L,
        (LPARAM)0L);

    // Toggle fDisplayAdvancedControls
    fDisplayAdvControls = (iCheckState == BST_CHECKED)? FALSE: TRUE;

    SendMessage( GetDlgItem(hwndDialog, ID_UNICODESUBSET),
                 CB_SETCURSEL,
                 (WPARAM) 0,
                 (LPARAM) 0);

    if(fDisplayAdvControls == FALSE)
    {
        // Since we are going back to a simple window close the list
        // which are already open
         if(ghwndList || ghwndGrid)
         {
             fSearched = TRUE;
             DestroyAllListWindows();
         }

         if(CodePage_GetCurSelCodePage(hWnd,ID_VIEW) != UNICODE_CODEPAGE)
         {
             SendMessage( GetDlgItem(hwndDialog, ID_VIEW),
                          CB_SETCURSEL,
                          (WPARAM) 0,
                          (LPARAM) 0);
             fSearched = TRUE;
         }
    }
    else
    {
         // Getting ready to show advanced controls
         WCHAR buffer[256];

         fSearched  = FALSE;
         fNeedReset = FALSE;

         SetDlgItemText(hWnd, ID_SEARCHINPUT, L"");
         LoadString(hInst, IDS_SEARCH, buffer, 255);
         SetDlgItemText(hWnd, ID_SEARCH, buffer);
         EnableWindow(GetDlgItem(hWnd, ID_SEARCH), FALSE);

         // Initialise and enable SUR controls
         EnableSURControls(hWnd);
    }

    // Change Dialog Size and enable/diable advanced controls
    ResizeDialog(hWnd);

    // Set the status line size
    GetClientRect(hWnd, &rectParent);
    rcStatusLine = rectParent;
    rcStatusLine.top = rcStatusLine.bottom - dyStatus;

    // Toggle ID_ADVANCED check
    SendMessage(GetDlgItem(hWnd, ID_ADVANCED),
        BM_SETCHECK,
        (WPARAM)((fDisplayAdvControls==TRUE)? BST_CHECKED: BST_UNCHECKED),
        (LPARAM)0L
    );

    // Force a paint message
    InvalidateRect(hWnd, NULL, TRUE);

    if(fSearched || fNeedReset)
    {
        SendMessage( hWnd,
                     WM_COMMAND,
                     MAKELONG(ID_VIEW,CBN_SELCHANGE),
                     0L);
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////
//
//  ResizeDialog - Sets the dialog height to correspond to initial start state
//
////////////////////////////////////////////////////////////////////////////
void ResizeDialog(HWND hWnd)
{
    RECT rDlg;
    int  iDisplay;

    // Get size of dialog
    GetWindowRect(hWnd, &rDlg);

    // Change Dialog Size
    if (fDisplayAdvControls == FALSE)
    {
        MoveWindow(hWnd, rDlg.left, rDlg.top,
            rDlg.right - rDlg.left, gwNormalHeight, TRUE);
        iDisplay = SW_HIDE;
    }
    else
    {
        MoveWindow(hWnd, rDlg.left, rDlg.top, rDlg.right - rDlg.left,
            gwExpandedHeight, TRUE);
        iDisplay = SW_NORMAL;
    }

    // Show or hide controls depending on whether expanded dialog is
    // being shown or normal dialog is shown
    ShowWindow(GetDlgItem(hWnd, ID_VIEWLB), iDisplay);
    ShowWindow(GetDlgItem(hWnd, ID_VIEW), iDisplay);
    ShowWindow(GetDlgItem(hWnd, ID_SUBSETLB), iDisplay);
    ShowWindow(GetDlgItem(hWnd, ID_UNICODESUBSET), iDisplay);
    ShowWindow(GetDlgItem(hWnd, ID_SEARCHNAME), iDisplay);
    ShowWindow(GetDlgItem(hWnd, ID_SEARCHINPUT), iDisplay);
    ShowWindow(GetDlgItem(hWnd, ID_SEARCH), iDisplay);

    // All SUR controls
    ShowWindow(GetDlgItem(hWnd, ID_URANGE), iDisplay);
    ShowWindow(GetDlgItem(hWnd, ID_FROM), iDisplay);
}

////////////////////////////////////////////////////////////////////////////
//
//  DoDragAndDrop - Calls the modal drag and drop proc for CharGrid
//
////////////////////////////////////////////////////////////////////////////
int DoDragAndDrop(HWND hWnd, LPDROPSOURCE pDropSource)
{
    PCImpIDataObject    pDataObject;
    DWORD               dwEffect;
    TCHAR               szCurrSelection[2];

    SetRichEditFont(hwndDialog, ID_STRING, sycm.hFont);
    WCHAR wc = (WCHAR)pCode[sycm.chCurr];
    ConvertAnsifontToUnicode(hWnd, (char*)&pCode[sycm.chCurr], &wc);
    //rich edit screws up symbol fonts
    if (gFontType & SYMBOL_FONTTYPE)
    {
      if ((wc >= 0xf000) && (wc <= 0xf0ff))
        wc = (WCHAR) (BYTE)pCode[sycm.chCurr];
    }
    szCurrSelection[0] = wc;
    szCurrSelection[1] = 0;

    // Create an instance of the DataObject
    pDataObject = new CImpIDataObject(hWnd);

    pDataObject->SetText(szCurrSelection);

    // Do drag and drop with copy
    DoDragDrop(pDataObject, pDropSource, DROPEFFECT_COPY, &dwEffect);

    // Free instance of DataObject
    pDataObject->Release();

    return 0;
}

////////////////////////////////////////////////////////////////////////////
//
//  CopyTextToClipboard - Copies text from the rich edit control to clipboard
//
////////////////////////////////////////////////////////////////////////////
static void CopyTextToClipboard(HWND hWndDlg)
{
    DWORD   dwSelStart, dwSelEnd;
    HWND    hWndRichEdit;

#ifndef DISABLE_RICHEDIT
    hWndRichEdit = GetDlgItem(hWndDlg, ID_STRING);
#else
    hWndRichEdit = GetDlgItem(hWndDlg, IDC_EDIT);
#endif

    SendMessage(hWndRichEdit, EM_GETSEL, (WPARAM)&dwSelStart,
        (LPARAM)&dwSelEnd);

    if (dwSelStart == dwSelEnd)
    {
        // There is no text that has been selected currently
        // So select the entire text and copy it to clipboard

        // Temporarily hide selection
        SendMessage(hWndRichEdit, EM_HIDESELECTION, (WPARAM)TRUE, (LPARAM)0);
        // Select entire text
        SendMessage(hWndRichEdit, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
        // Copy text to clipboard
        SendMessage(hWndRichEdit, WM_COPY, 0L, 0L);
        // Restore the hidden selection
        SendMessage(hWndRichEdit, EM_SETSEL, (WPARAM)dwSelStart,
            (LPARAM)dwSelEnd);
        SendMessage(hWndRichEdit, EM_HIDESELECTION, (WPARAM)FALSE, (LPARAM)0);
    }
    else
    {
        SendMessage(hWndRichEdit, WM_COPY, 0L, 0L);
    }

}

////////////////////////////////////////////////////////////////////////////
//
//  CompareCharFormats - Compares two CHARFORMATS
//  returns 1 if they are equal
//          0 if not equal
//
////////////////////////////////////////////////////////////////////////////
static int CompareCharFormats(CHARFORMAT* pcf1, CHARFORMAT* pcf2)
{
        /*
    return ( (pcf1->bPitchAndFamily == pcf2->bPitchAndFamily)
        && (lstrcmp(pcf1->szFaceName, pcf2->szFaceName) == 0)
        && (pcf1->bCharSet == pcf2->bCharSet)
        );
        */
  return ((lstrcmp(pcf1->szFaceName, pcf2->szFaceName) == 0));
}

////////////////////////////////////////////////////////////////////////////
//
//  SetRichEditFont - Compares the charformat of the current selection
//  with the charformat for the font in the chargrid. If they are not
//  equal then it sets a new charformat
//
////////////////////////////////////////////////////////////////////////////
static void SetRichEditFont(HWND hWndDlg, int idCtl, HFONT hFont)
{
    CHARFORMAT  cf, cfCurr;
    LOGFONT     lf;
    HWND        hWndRichEdit;
    RECT        rc;
    HDC         hDC;
    int         iLogPixelsYDisplay, iFontDeviceHt;
    DWORD       dwSelStart, dwSelEnd;
    HFONT       hFontEdit, hFontOld;
    TEXTMETRIC  tm;

    hWndRichEdit = GetDlgItem(hWndDlg, idCtl);
    SendMessage(hWndRichEdit, EM_GETRECT, 0, (LPARAM)&rc);
    hDC = GetDC(hWndDlg);

    hWndRichEdit = GetDlgItem(hWndDlg, ID_STRING);

    SendMessage(hWndRichEdit, EM_GETSEL, (WPARAM)&dwSelStart,
        (LPARAM)&dwSelEnd);

    // Font cell height (logical units)
    iFontDeviceHt = rc.bottom - rc.top - 2;

    // Convert font height from logical units to twips for rich edit
    // Return logical units per inch
    iLogPixelsYDisplay = GetDeviceCaps(hDC, LOGPIXELSY);

    // Get the CharFormat of the current selection
    cfCurr.cbSize = sizeof(cfCurr);
    SendMessage(hWndRichEdit, EM_GETCHARFORMAT, TRUE, (LPARAM)&cfCurr);

    // Get the Log font structure from the current font
    if (GetObject(hFont, sizeof(lf), &lf) == 0)
    {
        ReleaseDC(hWndDlg, hDC);
        return;
    }

    // Logical cell height
    lf.lfHeight = iFontDeviceHt;

    cf.bCharSet = (BYTE) GetCurFontCharSet(hWndRichEdit);

    // Fill out the CHARFORMAT structure to get the character effects.
    cf.cbSize = sizeof (cf);
    cf.dwMask = CFM_BOLD | CFM_FACE | CFM_ITALIC |
                    CFM_SIZE | CFM_UNDERLINE | CFM_OFFSET | CFM_CHARSET;
    cf.dwEffects = 0;

    // Set attributes
    if (lf.lfWeight >= FW_BOLD)
        cf.dwEffects |= CFE_BOLD;
    if (lf.lfItalic)
        cf.dwEffects |= CFE_ITALIC;

    if (lf.lfUnderline)
        cf.dwEffects |= CFE_UNDERLINE;

    cf.bPitchAndFamily = lf.lfPitchAndFamily;

    // Figure physical font internal leading

    GetViewableFontDimensions(hDC, iLogPixelsYDisplay, &lf, &hFontOld, &hFontEdit, &tm);


    // rich edit needs character height not cell height.
    // character height = cell height - internal leading
    //
    // logical units * points/inch * twips/point
    // ------------------------------------------
    // logical units / inch
    //
    cf.yHeight = MulDiv(tm.tmHeight - tm.tmInternalLeading, 1440, iLogPixelsYDisplay);

    SelectObject(hDC, hFontOld);
    DeleteObject(hFontEdit);

    cf.yOffset = 0;

    // Set the new typeface, preserving the previous effects.
    wcscpy(cf.szFaceName, lf.lfFaceName);

    if (CompareCharFormats(&cf, &cfCurr) == 0)
        SendMessage (hWndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

    ReleaseDC(hWndDlg, hDC);

}

////////////////////////////////////////////////////////////////////////////
//
//  DoDragScroll - Routine for scrolling the map when the mouse button is
//  down and mouse pointer goes outside the character grid
//
////////////////////////////////////////////////////////////////////////////
int DoDragScroll(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    RECT    rcClient;
    int     yMousePosn, iyDistance, iScrollTimer;
    DWORD   dwScrollDir;
    POINT   pt;

    // Store the current wParam and lParam so that they can be used for
    // next dragscroll
    prevKeys = wParam;
    ptPrevMouse = lParam;
    POINTSTOPOINT(pt, lParam);

    yMousePosn = pt.y;
    GetClientRect(hWnd, &rcClient);

    // Verify that we are out of the client area if not return
    if ((yMousePosn >= rcClient.top) && (yMousePosn <= rcClient.bottom))
    {
        KillTimer(hWnd, ID_SCROLLTIMER);
        return 0;
    }

    dwScrollDir = (yMousePosn < 0) ? VK_UP : VK_DOWN;

    iyDistance = (yMousePosn < 0) ?
        -yMousePosn : yMousePosn - rcClient.bottom;

    // Thus if iyDistance is less then then the scroll interval will be less
    iScrollTimer = 750 - ((UINT)iyDistance << 4);

    // Min value should be one
    if (iScrollTimer < 1)
        iScrollTimer = 1;

    // Scroll
    SendMessage(hWnd, WM_KEYDOWN, (WPARAM)dwScrollDir, (LPARAM)0L);

    // Set timer for next scroll
    SetTimer(hWnd, ID_SCROLLTIMER, iScrollTimer, NULL);

    return 0;
}

////////////////////////////////////////////////////////////////////////////
//
//  FormatHex - Puts leading zeros and converts lower case to upper
//
////////////////////////////////////////////////////////////////////////////
int FormatHex(LPTSTR lpszText, LPTSTR lpszHexText, int nLen)
{
    int nChars, i, j;

    nChars = wcslen(lpszText);

    for (i=0; i<nLen-nChars; i++)
    {
        // Put the leading 0's
        lpszHexText[i] = ZERO;
    }

    for (j=0; j<nChars; j++)
    {
        // Convert all characters to upper
        lpszHexText[i+j] = towupper(lpszText[j]);
    }

    // Null terminate
    lpszHexText[nLen] = 0;

    return 0;
}

////////////////////////////////////////////////////////////////////////////
//
//  InitSURControl - Initialises the default values for the SUR edit box
//                   with ID nControlId
//
////////////////////////////////////////////////////////////////////////////
int InitSURControl(HWND hWnd, int nControlId)
{
    int     i;
    TCHAR   szValue[256];

    for (i=0; i<sizeof(validData) / sizeof(validData[0]); i++)
    {
        if (validData[i].nControlId == nControlId)
        {
            // Set inital value
            wsprintf(szValue, L"%04X", validData[i].iDefaultValue);
            //SetDlgItemText(hWnd, nControlId, szValue);
            SetDlgItemText(hWnd, nControlId, L"");
        }
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////
//
//  InitSURControl - Initialises the default values for the SUR edit box
//                   with ID nControlId
//
////////////////////////////////////////////////////////////////////////////
int GetDefaultValue(int nControlId)
{
    int     i;

    for (i=0; i<sizeof(validData) / sizeof(validData[0]); i++)
    {
        if (validData[i].nControlId == nControlId)
        {
            return (validData[i].iDefaultValue);
        }
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////
//
//  InitSURControls - Initialises the default values for all SUR edit boxes
//
////////////////////////////////////////////////////////////////////////////
int InitSURControls(HWND hWnd)
{
    InitSURControl(hWnd, ID_FROM);

    return 0;
}

////////////////////////////////////////////////////////////////////////////
//
//  UnicodeRangeChecked - Called when Unicode Range check box is checked
//
////////////////////////////////////////////////////////////////////////////
int UnicodeRangeChecked(HWND hWnd)
{
    if (IsDlgButtonChecked(hWnd, ID_URANGE) == BST_CHECKED)
    {
        EnableWindow(GetDlgItem(hWnd, ID_FROM), TRUE);
        EnableWindow(GetDlgItem(hWnd, ID_TO), TRUE);
        InitSURControls(hWnd);

        // Show the current range
        SURangeChanged(hWnd);
    }
    else
    {
        EnableWindow(GetDlgItem(hWnd, ID_FROM), FALSE);
        EnableWindow(GetDlgItem(hWnd, ID_TO), FALSE);
        InitSURControls(hWnd);

        // Show the whole unicode range
        SURangeChanged(hWnd);
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////
//
//  EnableSURControls - Enables or Disables SUR controls depending on
//                      currently selected CodePage and View
//
////////////////////////////////////////////////////////////////////////////
int EnableSURControls(HWND hWnd, BOOL fForceDisable)
{
    LONG    lCodePage;
    int     nIndex;
    BOOL    bEnabled=TRUE;


    if (fForceDisable || !IsWindowEnabled(GetDlgItem(hWnd, ID_VIEW)))
    {
        // Called when controls are to be forcibly disabled
        // irrespective of codepage and subset
        InitSURControls(hWnd);
        EnableWindow(GetDlgItem(hWnd, ID_URANGE), FALSE);
        EnableWindow(GetDlgItem(hWnd, ID_FROM), FALSE);
        iFromPrev = 0x21; iToPrev = 0xFFFD;
        return 0;
    }

    // SUR controls are disabled if Character Set is not Unicode
    // or SearchByGroup is not all
    lCodePage = (LONG)CodePage_GetCurSelCodePage(hWnd, ID_VIEW);

    nIndex = (int)SendDlgItemMessage(hWnd, ID_UNICODESUBSET, CB_GETCURSEL,
        (WPARAM) 0, (LPARAM) 0L);

    if ((lCodePage != UNICODE_CODEPAGE) || (nIndex != 0))
    {
        bEnabled = FALSE;
    }

    EnableWindow(GetDlgItem(hWnd, ID_URANGE), bEnabled);

    if (bEnabled == FALSE)
    {
        // Reset all the SUR controls
        InitSURControls(hWnd);
        EnableWindow(GetDlgItem(hWnd, ID_FROM), bEnabled);
    }
    else
    {
        EnableWindow(GetDlgItem(hWnd, ID_FROM), bEnabled);
        InitSURControls(hWnd);
    }

    // Reset values of iFromPrev and iToPrev
    iFromPrev = 0x21; iToPrev = 0xFFFD;

    return 0;
}

////////////////////////////////////////////////////////////////////////////
//
//  SURangeChanged - Called when contents of one of the SUR edit boxes
//  changes. This proc updates the chargrid if the contents of the edit
//  boxes are valid
//
////////////////////////////////////////////////////////////////////////////
int SURangeChanged(HWND hWnd)
{
    int     iFrom, iTo=0xFFFD, nLen;
    TCHAR   chBuffer[MAX_CHARS+1];
    WCHAR   buffer2[256];

    nLen = GetDlgItemText(hWnd, ID_FROM, chBuffer, MAX_CHARS+1);
    if (nLen != 0)
        swscanf(chBuffer, L"%x", &iFrom);
    else
        iFrom = 0x21;

    // Check limits
    if ((iFrom < 0x0021) || (iFrom > 0xFFFD))
    {
        iFrom = 0x0021;
        //InitSURControl(hWnd, ID_FROM);
    }

    // Check if the from and to values have changed
    if ((iFromPrev == iFrom) && (iToPrev == iTo))
    {
        // No change
        return 0;
    }

    // Selection should be the first one
    ExitMagnify(hwndCharGrid, &sycm);
    sycm.chCurr = 0;

    // Reload the buffer and set min and max range
    SetSpecificSelection(hWnd, iFrom, iTo);

    iFromPrev = iFrom;
    iToPrev = iTo;

    // Since the range has changed enable the reset button
    if (nLen != 0)
    {
        LoadString(hInst, IDS_RESET,  buffer2, 255);
        SetDlgItemText(hWnd, ID_SEARCH, buffer2);
        EnableWindow(GetDlgItem(hWnd, ID_SEARCH), TRUE);
        fNeedReset = FALSE;
        fSearched  = TRUE;
    }
    else
    {
        LoadString(hInst, IDS_SEARCH,  buffer2, 255);
        SetDlgItemText(hWnd, ID_SEARCH, buffer2);
        EnableWindow(GetDlgItem(hWnd, ID_SEARCH), FALSE);
        fNeedReset = FALSE;
        fSearched  = FALSE;
    }

    // Invalidate the charmap
    InvalidateRect(hwndCharGrid, NULL, TRUE);

    return 0;
}

////////////////////////////////////////////////////////////////////////////
//
//  ValidateValues - Validates the values in SUR edit boxes. Also formats
//  edit box text in case of valid values
//
////////////////////////////////////////////////////////////////////////////
int ValidateValues(HWND hWnd)
{
    UINT    uCtrlID;
    int     iValue1, iValue2=0xFFFD, iDefault1;
    TCHAR   szText[MAX_CHARS+1],
            szHexText[MAX_CHARS+1];

    // Subset has changed so validate upper and lower limit
    // and redisplay grid
    GetWindowText(hWnd, szText, MAX_CHARS+1);
    swscanf(szText, L"%x", &iValue1);

    uCtrlID = GetDlgCtrlID(hWnd);
    iDefault1 = GetDefaultValue(uCtrlID);

    // This code was for validation of default values
    if (uCtrlID == ID_FROM)
    {
        if ((iValue1 < iDefault1) || (iValue1 > iValue2))
        {
            SURangeChanged(GetParent(hWnd));
            return 0;
        }
    }

    SURangeChanged(GetParent(hWnd));
    FormatHex(szText, szHexText, MAX_CHARS);
    SetWindowText(hWnd, szHexText);

    return 0;
}

static WNDPROC fnEditProc = NULL;

//**********************************************************************
// SetHexEditProc
//
// Purpose:
//      Replaces current Edit window procedure with a new procedure
//      capable of validating hex text
//
// Parameters:
//      HWND  hWndEdit      -   Edit Control Handle
//
// Return Value:
//      TRUE                -   Success
//**********************************************************************
static BOOL SetHexEditProc(HWND hWndEdit)
{
    fnEditProc = (WNDPROC)SetWindowLongPtr (hWndEdit, GWLP_WNDPROC,
        (LPARAM)HexEditControlProc);

    if (fnEditProc != NULL)
        return TRUE;
    else
        return FALSE;
}


//**********************************************************************
// HexEditControlProc
//
// Purpose:
//      Edit control procedure capable of handling Hex characters
//      Allows 0-9, a-f and converts a-f to A-F
//      Enter or space - cause refresh of grid with characters
//                       starting from text entered in ID_FROM
//      Escape         - if something is present in textbox clear box
//                     - if nothing is present in textbox close app
//
//**********************************************************************
static LRESULT CALLBACK HexEditControlProc(HWND    hWnd,
                                           UINT    uMessage,
                                           WPARAM  wParam,
                                           LPARAM  lParam)
{
    switch(uMessage)
    {
        case WM_GETDLGCODE:
        {
            return (DLGC_WANTALLKEYS);
            break;
        }

        case WM_KEYDOWN:
        {
            int iLen;

            switch (wParam)
            {
                case VK_ESCAPE:
                {
                    iLen = GetWindowTextLength(hWnd);
                    if (iLen)
                    {
                        InitSURControls(GetParent(hWnd));
                        ValidateValues(hWnd);
                    }
                    else
                    {
                        SendMessage(GetParent(hWnd), WM_CLOSE, 0, 0L);
                    }
                    break;
                }

                case VK_F6:
                {

                    SetFocus(hwndCharGrid);
                    return 0L;
                }

                case VK_TAB:
                {
                    PostMessage(GetParent(hWnd), WM_NEXTDLGCTL, 0, 0L);
                    break;
                }
            }

            break;
        }

        case WM_CHAR:
        {
            TCHAR   chCharCode;

            chCharCode = (TCHAR) wParam;

            // The only characters allowed are 0-9, a-f, A-F, ' ' and '\b'
            if ((chCharCode == ' ') || (chCharCode == VK_RETURN))
            {
                fSURChanged = TRUE;
                return ValidateValues(hWnd);
            }
            else
            {
                if (chCharCode == '\b')
                    break;

                // Validate chars
                if ((chCharCode >= '0')&&(chCharCode <= '9')
                    || (chCharCode >= 'A')&&(chCharCode <= 'F')
                    || (chCharCode >= 'a')&&(chCharCode <= 'f')
                    || (chCharCode == '\b'))
                {
                    // Char is ok
                    if ((chCharCode >= 'a')&&(chCharCode <= 'f'))
                        wParam = towupper(chCharCode);
                }
            }

            break;
        }

        case WM_KILLFOCUS:
        {
            ValidateValues(hWnd);
            break;
        }
    }

    return CallWindowProc(fnEditProc, hWnd, uMessage, wParam, lParam);
}

//**********************************************************************
// CreateTooltipWindow
//
// Purpose:  Creates a new tooltip window
//
// Parameters:  hWndParent   -  App Window handle
//
// Return Value:  Window handle of the tooltip window
//**********************************************************************
HWND CreateTooltipWindow(HWND hWndParent)
{
  hwndTT = CreateWindowEx( 0,
      TOOLTIPS_CLASS,
      NULL,
      WS_POPUP | TTS_ALWAYSTIP,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      10,
      10,
      hWndParent,
      NULL,
      hInst,
      NULL);

  if (hwndTT == (HWND) NULL)
    return (HWND) NULL;

  return hwndTT;
}

//**********************************************************************
// ResizeTipsArea
//
// Purpose:  Creates a new tooltip window and sets the rectangles for
//           each tooltip
//
// Parameters:  hWnd         -  App Window handle
//
// Return Value:  None
//**********************************************************************
void ResizeTipsArea(HWND hWndGrid, PSYCM psycm)
{
    TOOLINFO ti;    // tool information
    RECT     rcKey; // tool rectangle
    int      x, y;

    if (hwndTT != NULL)
        DestroyWindow(hwndTT);

    CreateTooltipWindow(hWndGrid);

    for (x=0; x<cchSymCol; x++)
    {
        for (y=0; y<cchSymRow; y++)
        {
             ZeroMemory(&ti, sizeof(ti));
             ti.cbSize   = sizeof(TOOLINFO);
             ti.uFlags   = 0;
             ti.hwnd     = hWndGrid;
             ti.lpszText = LPSTR_TEXTCALLBACK;

             rcKey.left    = (int)(y*psycm->dxpBox);
             rcKey.right   = (int)((y+1)*psycm->dxpBox);
             rcKey.top     = (int)(x*psycm->dypBox);
             rcKey.bottom  = (int)((x+1)*psycm->dypBox);

             ti.rect = rcKey;
             SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
        }
    }
}

////////////////////////////////////////////////////////////////////////////
//
//  DrawFamilyComboItem
//
//  Paints the font facenames and TT bitmap in the font combo box.
//
////////////////////////////////////////////////////////////////////////////

BOOL DrawFamilyComboItem(
    LPDRAWITEMSTRUCT lpdis)
{
    HDC hDC, hdcMem;
    DWORD rgbBack, rgbText;
    TCHAR szFace[LF_EUDCFACESIZE];
    HBITMAP hOld;
    INT dy;
    DWORD   FontType;

    hDC = lpdis->hDC;

    if (lpdis->itemState & ODS_SELECTED)
    {
        rgbBack = SetBkColor(hDC, GetSysColor(COLOR_HIGHLIGHT));
        rgbText = SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
    }
    else
    {
        rgbBack = SetBkColor(hDC, GetSysColor(COLOR_WINDOW));
        rgbText = SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
    }

    SendMessage( lpdis->hwndItem,
                 CB_GETLBTEXT,
                 lpdis->itemID,
                 (LPARAM)(LPTSTR)szFace );
    ExtTextOut( hDC,
                lpdis->rcItem.left + DX_BITMAP,
                lpdis->rcItem.top,
                ETO_OPAQUE | ETO_CLIPPED,
                &lpdis->rcItem,
                szFace,
                lstrlen(szFace),
                NULL );

    hdcMem = CreateCompatibleDC(hDC);
    if (hdcMem)
    {
        int i;

        if (hbmFont)
        {
            hOld = (HBITMAP) SelectObject(hdcMem, hbmFont);

            i = (int)SendMessage(lpdis->hwndItem, CB_GETITEMDATA, (WPARAM) lpdis->itemID, (LPARAM) 0);

            FontType = Font_pList[i].FontType;
            if (FontType)
            {
                int xSrc;
                dy = ((lpdis->rcItem.bottom - lpdis->rcItem.top) - DY_BITMAP) / 2;

                if      (FontType & TRUETYPE_FONT ||
                         FontType & EUDC_FONTTYPE )
                    xSrc = 0;
                else if (FontType & TT_OPENTYPE_FONTTYPE)
                    xSrc = 2;
                else if (FontType & PS_OPENTYPE_FONTTYPE)
                    xSrc = 3;
                else if (FontType & TYPE1_FONTTYPE)
                    xSrc = 4;
                else
                    xSrc = -1;

                if (xSrc != -1)
                BitBlt( hDC,
                        lpdis->rcItem.left,
                        lpdis->rcItem.top + dy,
                        DX_BITMAP,
                        DY_BITMAP,
                        hdcMem,
                        xSrc * DX_BITMAP,
                        lpdis->itemState & ODS_SELECTED ? DY_BITMAP : 0,
                        SRCCOPY );
            }
            SelectObject(hdcMem, hOld);
        }
        DeleteDC(hdcMem);
    }

    SetTextColor(hDC, rgbText);
    SetBkColor(hDC, rgbBack);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  LoadBitmaps
//
//  Loads DIB bitmaps and "fixes up" their color tables so that we get the
//  desired result for the device we are on.
//
//  This routine requires:
//    - the DIB is a 16 color DIB authored with the standard windows colors
//    - bright blue (00 00 FF) is converted to the background color
//    - light grey  (C0 C0 C0) is replaced with the button face color
//    - dark grey   (80 80 80) is replaced with the button shadow color
//
//  This means you can't have any of these colors in your bitmap.
//
////////////////////////////////////////////////////////////////////////////

HBITMAP LoadBitmaps(
    INT id)
{
    HDC hdc;
    HANDLE h, hRes;
    DWORD *p;
    LPBYTE lpBits;
    LPBITMAPINFOHEADER lpBitmapInfo;
    INT numcolors;
    DWORD rgbSelected, rgbUnselected;
    HBITMAP hbm;

    rgbSelected = GetSysColor(COLOR_HIGHLIGHT);
    //
    //  Flip the colors.
    //
    rgbSelected = RGB( GetBValue(rgbSelected),
                       GetGValue(rgbSelected),
                       GetRValue(rgbSelected) );
    rgbUnselected = GetSysColor(COLOR_WINDOW);
    //
    //  Flip the colors.
    //
    rgbUnselected = RGB( GetBValue(rgbUnselected),
                         GetGValue(rgbUnselected),
                         GetRValue(rgbUnselected) );

    h = FindResource(hInst, MAKEINTRESOURCE(id), RT_BITMAP);
    hRes = LoadResource(hInst, (HRSRC) h);

    //
    //  Lock the bitmap and get a pointer to the color table.
    //
    lpBitmapInfo = (LPBITMAPINFOHEADER)LockResource(hRes);

    if (!lpBitmapInfo)
    {
        return (FALSE);
    }

    p = (DWORD *)((LPSTR)(lpBitmapInfo) + lpBitmapInfo->biSize);

    //
    //  Search for the Solid Blue entry and replace it with the current
    //  background RGB.
    //

                numcolors = 16;

    while (numcolors-- > 0)
    {
        if (*p == BACKGROUND)
        {
            *p = rgbUnselected;
        }
        else if (*p == BACKGROUNDSEL)
        {
            *p = rgbSelected;
        }
        p++;
    }
    UnlockResource(hRes);

    //
    //  Now create the DIB.
    //
    lpBitmapInfo = (LPBITMAPINFOHEADER)LockResource(hRes);

    //
    //  First skip over the header structure.
    //
    lpBits = (LPBYTE)(lpBitmapInfo + 1);

    //
    //  Skip the color table entries, if any.
    //
    lpBits += (1 << (lpBitmapInfo->biBitCount)) * sizeof(RGBQUAD);

    //
    //  Create a color bitmap compatible with the display device.
    //
    hdc = GetDC(NULL);
    hbm = CreateDIBitmap( hdc,
                          lpBitmapInfo,
                          (DWORD)CBM_INIT,
                          lpBits,
                          (LPBITMAPINFO)lpBitmapInfo,
                          DIB_RGB_COLORS );
    ReleaseDC(NULL, hdc);

    GlobalUnlock(hRes);
    FreeResource(hRes);

    return (hbm);
}

static WNDPROC fnSearchEditProc = NULL;

//**********************************************************************
// SetSearchEditProc
//
// Purpose:
//      Replaces current Search Edit box procedure with a new procedure
//      to handle the Enter key
//
// Parameters:
//      HWND  hWndEdit      -   Edit Control Handle
//
// Return Value:
//      TRUE                -   Success
//**********************************************************************
static BOOL SetSearchEditProc(HWND hWndEdit)
{
    fnSearchEditProc = (WNDPROC)SetWindowLongPtr (hWndEdit, GWLP_WNDPROC,
        (LPARAM)SearchEditControlProc);

    if (fnSearchEditProc != NULL)
        return TRUE;
    else
        return FALSE;
}

//**********************************************************************
// SearchEditControlProc
//
// Purpose:
//      to handle :
//         Enter     - click the search/reset button
//
//**********************************************************************
static LRESULT CALLBACK SearchEditControlProc(HWND    hWnd,
                                              UINT    uMessage,
                                              WPARAM  wParam,
                                              LPARAM  lParam)
{
    if(uMessage == WM_KEYDOWN)
    {
        if(wParam   == VK_RETURN)
            SendMessage(hwndDialog, WM_COMMAND, ID_SEARCH, 0L);
        else if(wParam   == VK_F6)
            SetFocus(hwndCharGrid);
    }

    return CallWindowProc(fnSearchEditProc, hWnd, uMessage, wParam, lParam);
}

BOOL CALLBACK MsgProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
      case ( WM_INITDIALOG ):
      {
        if (gDisplayFontChgMsg)
          SendMessage(GetDlgItem(hWnd, IDC_CHECKNOMSG),
                      BM_SETCHECK,(WPARAM)BST_UNCHECKED,(LPARAM)0L);
        else
          SendMessage(GetDlgItem(hWnd, IDC_CHECKNOMSG),
                      BM_SETCHECK,(WPARAM)BST_CHECKED,(LPARAM)0L);
        return TRUE;
      }
      case ( WM_COMMAND ) :
      {
        switch (LOWORD(wParam))
        {
          case (IDOK):
            EndDialog(hWnd, IDOK);
            return TRUE;
          case (IDCANCEL):
            EndDialog(hWnd, IDCANCEL);
            return TRUE;
          case (IDC_CHECKNOMSG):
            LRESULT iCheckState;
            iCheckState = SendMessage(GetDlgItem(hWnd, IDC_CHECKNOMSG),
                                      BM_GETCHECK,(WPARAM)0L,(LPARAM)0L);
            gDisplayFontChgMsg = (iCheckState == BST_CHECKED)? FALSE: TRUE;
            return TRUE;
        }
      }
      case ( WM_SYSCOMMAND):
      {
        switch (wParam)
        {
          case ((WPARAM)SC_CLOSE):
            EndDialog(hWnd, IDCANCEL);
            return TRUE;
        }
        break;
      }
    }
    return FALSE;
}

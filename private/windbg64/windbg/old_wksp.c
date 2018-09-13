/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    old_wksp.c

Abstract:

    This module contains the old support for Windbg's workspaces.

Author:

    Ramon J. San Andres (ramonsa)       07-July-1992
    Griffith Wm. Kadnier (v-griffk)     15-Jan-1993
    Carlos Klapp (a-caklap)             07-Oct-1999

Environment:

    Win32, User Mode

--*/

#include "precomp.h"
#pragma hdrstop

#include "dbugexcp.h"


//
// BUGBUG - kcarlos
// HAck to be replaced by new code
// Some of the registry entries are longer than MAX_PATH
//
#define BIG_BUFFER_HACK (1024 * 8)


//
//  The following constants determine the location of the
//  debugger information in the registry.
//
/*
#define WINDBG_KEY          "Software\\Microsoft\\"
#define PROGRAMS            "Programs"
#define OPTIONS             "Options"
#define DEFAULT_WORKSPACE   "Default Workspace"
#define COMMON_WORKSPACE    "Common Workspace"
#define WORKSPACE_PREFIX    "WorkSpace_"
*/


//
//  The following strings identify key/values in the registry.
//
/*
#define WS_STR_MRU_LIST             "MRU List"
#define WS_STR_MISC                 "Misc"
#define WS_STR_PATH                 "Path"
#define WS_STR_COMLINE              "Command Line"
#define WS_STR_FRAME_WINDOW         "Frame Window"
#define WS_STR_DEFFONT              "Default font"
#define WS_STR_FILEMRU              "File MRU List"
#define WS_STR_TOOLBAR              "Tool Bar"
#define WS_STR_STATUSBAR            "Status Bar"
// Path were the last source file was opened using the file open dlg.
#define WS_STR_MRU_SRC_FILE_PATH    "MRU Used Src Path"
#define WS_STR_SRCMODE              "Source Stepping"
#define WS_STR_BRKPTS               "Breakpoints"
#define WS_STR_WNDPROCHIST          "WndProc History"
#define WS_STR_OPTIONS              "Options"
#define WS_STR_DBGCHLD              "Debug Child Process"
#define WS_STR_CHILDGO              "Go on Child Start"
#define WS_STR_ATTACHGO             "Go on Attach"
#define WS_STR_EXITGO               "Go on Thread Exit"
#define WS_STR_EPSTEP               "Entry Point is First Step"
#define WS_STR_COMMANDREPEAT        "Command Repeat"
#define WS_STR_NOVERSION            "NoVersion"
#define WS_STR_EXTENSION_NAMES      "Extension Names"
#define WS_STR_MASMEVAL             "Masm Evaluation"
#define WS_STR_BACKGROUND           "Background Symbol Loads"
#define WS_STR_ALTSS                "Alternate SS"
#define WS_STR_WOWVDM               "WOW Vdm"
#define WS_STR_DISCONNECT           "Disconnect Exit"
#define WS_STR_IGNOREALL            "Ignore All"
#define WS_STR_BROWSEFORSYMBOLS     "Browse For Syms On Sym Load Errors"
#define WS_STR_VERBOSE              "Verbose"
#define WS_STR_CONTEXT              "Abbreviated Context"
#define WS_STR_INHERITHANDLES       "Inherit Handles"
#define WS_STR_LFOPT_APPEND         "Logfile Append"
#define WS_STR_LFOPT_AUTO           "Logfile Auto"
#define WS_STR_LFOPT_FNAME          "Logfile Name"
#define WS_STR_KERNELDEBUGGER       "Kernel Debugger"
#define WS_STR_KD_INITIALBP         "Initial Break Point"
#define WS_STR_KD_MODEM             "Use Modem Controls"
#define WS_STR_KD_PORT              "Port"
#define WS_STR_KD_BAUDRATE          "Baudrate"
#define WS_STR_KD_CACHE             "Cache Size"
#define WS_STR_KD_PLATFORM          "Platform"
#define WS_STR_KD_ENABLE            "Enable"
#define WS_STR_KD_GOEXIT            "Go On Exit"
#define WS_STR_KD_CRASH             "Crash Dump"
#define WS_STR_KD_KERNEL_NAME       "Kernel Name"
#define WS_STR_KD_HAL_NAME          "HAL Name"
#define WS_STR_RADIX                "Radix"
#define WS_STR_REG                  "Registers"
#define WS_STR_REGULAR              "Regular"
#define WS_STR_EXTENDED             "Extended"
#define WS_STR_REGMMU               "Register MMU"
#define WS_STR_DISPSEG              "Display Segment"
#define WS_STR_IGNCASE              "Ignore Case"
#define WS_STR_SUFFIX               "Suffix"
#define WS_STR_ENV                  "Environment"
#define WS_STR_ASMOPT               "Disassembler Options"
#define WS_STR_SHOWSEG              "Show Segment"
#define WS_STR_SHOWRAW              "Show Raw Bytes"
#define WS_STR_UPPERCASE            "Uppercase"
#define WS_STR_SHOWSRC              "Show Source"
#define WS_STR_SHOWSYM              "Show Symbols"
#define WS_STR_DEMAND               "Never Open DisAsm Wnd Automatically"
#define WS_STR_COLORS               "Colors"
#define WS_STR_DBGDLL               "Debugger DLLs"
#define WS_STR_SYMHAN               "Symbol Handler"
#define WS_STR_EXPEVAL              "Expression Evaluator"
#define WS_STR_TRANSLAY             "Transport Layer"
#define WS_STR_EXECMOD              "Execution model"
#define WS_STR_VIEWS                "Views"
#define WS_STR_TYPE                 "pIndWinLayout->m_nType"
#define WS_STR_PANE                 "Pane Data"
#define WS_STR_PLACEMENT            "Placement"
#define WS_STR_FONT                 "Font"
#define WS_STR_FILENAME             "File Name"
#define WS_STR_LINE                 "Line"
#define WS_STR_COLUMN               "Column"
#define WS_STR_READONLY             "ReadOnly"
#define WS_STR_EXPRESSION           "Expression"
#define WS_STR_FORMAT               "Format"
#define WS_STR_LIVE                 "Live"
#define WS_STR_X                    "X"
#define WS_STR_Y                    "Y"
#define WS_STR_WIDTH                "Width"
#define WS_STR_HEIGHT               "Height"
#define WS_STR_STATE                "State"
#define WS_STR_ICONIC               "Iconized"
#define WS_STR_NORMAL               "Normal"
#define WS_STR_MAXIMIZED            "Maximized"
#define WS_STR_ORDER                "Order"
#define WS_STR_TABSTOPS             "Tab Stops"
#define WS_STR_KEEPTABS             "Keep tabs"
#define WS_STR_HORSCROLL            "Horizontal Scroll Bar"
#define WS_STR_VERSCROLL            "Vertical Scroll Bar"
#define WS_STR_SRCHPATH             "PATH search"
#define WS_STR_REDOSIZE             "Undo-Redo buffer size"
#define WS_STR_SRCPATH              "Source search path"
#define WS_STR_ROOTPATH             "Root Mapping Pairs"
#define WS_STR_USERDLL              "User DLLs"
#define WS_STR_USERDLLNAME          "Name"
#define WS_STR_USERDLL_TEMPLATE     "UserDll_%.4d"
#define WS_STR_LOADTIME             "Load time"
#define WS_STR_DEFLOCATION          "Default location"
#define WS_STR_DEFLOADTIME          "Default load time"
#define WS_STR_DEFSRCHPATH          "Default search path"
#define WS_STR_EXCPT                "Exceptions"
#define WS_STR_ASKSAVE              "Ask to save"
#define WS_STR_CALLS                "Calls Options"
#define WS_STR_TITLE                "Window Title"
#define WS_STR_REMOTE_PIPE          "Remote Pipe"
*/

//
//  When to load the symbols
//
typedef enum _LOADTIME {
    LOAD_SYMBOLS_IGNORE,        //  Ignore
    LOAD_SYMBOLS_NOW,           //  Load symbols immediately after loading DLL
    LOAD_SYMBOLS_LATER,         //  Defer symbol loading
    LOAD_SYMBOLS_NEVER          //  Suppress symbol loading
} LOADTIME, *PLOADTIME;


//
//  View ordering structure
typedef struct _VIEW_ORDER {
    int                     View;       //  View index
    int                     Order;      //  Order
    CIndivWinLayout_WKSP    *pIndWinLayout;
} VIEW_ORDER, *PVIEW_ORDER;





//
//  External variables
//
extern char     DebuggerName[];
extern BOOL     AskToSave;

//
//  External functions
//
extern HWND     GetWatchHWND(void);
extern LRESULT  SendMessageNZ (HWND,UINT,WPARAM,LPARAM);

//
//  Global variables
//
/*
BOOL                StateCurrent    = FALSE;
BOOL                ProgramLoaded   = FALSE;
char                CurrentWorkSpaceName[ MAX_PATH ];
char                CurrentProgramName[ MAX_PATH ];
char                UntitledProgramName[]           = UNTITLED_PROGRAM;
char                UntitledWorkSpaceName[]         = UNTITLED_WORKSPACE;
VS_FIXEDFILEINFO   *FixedFileInfo   = NULL;
DWORD               WorkspaceOverride;
*/


//EXCEPTION_LIST *DefaultExceptionList = NULL;


//
//  Local prototypes
//

/*
//HKEY    OpenRegistryKey( HKEY, LPCSTR, BOOL    );

//HKEY    GetWorkSpaceKey( HKEY, LPSTR , LPSTR , BOOL   , LPSTR  );
BOOL    LoadWorkSpaceFromKey( HKEY, BOOL    );
BOOL    SaveWorkSpaceToKey( HKEY, BOOL    );
BOOL    SetDefaultWorkSpace( HKEY, LPSTR , LPSTR  );

BOOL    LoadWorkSpaceItem ( HKEY, WORKSPACE_ITEM, BOOL    );
BOOL    SaveWorkSpaceItem ( HKEY, WORKSPACE_ITEM, BOOL    );

BOOL    LoadWorkSpaceWindowItem ( HKEY, WORKSPACE_ITEM, BOOL   , BOOL     );
BOOL    SaveWorkSpaceWindowItem ( HKEY, WORKSPACE_ITEM, BOOL   , BOOL     );

*/

BOOL    LoadAllWindows(VOID);
BOOL    SaveAllWindows(VOID);

BOOL    LoadView ( int, CIndivWinLayout_WKSP * );
BOOL    SaveView ( int, int, CIndivWinLayout_WKSP * );

BOOL    GetWindowMetrics ( HWND, CIndivWinLayout_WKSP *, BOOL );

/*
BOOL    LoadWindowData ( HKEY, char*, PWINDOW_DATA );
BOOL    SaveWindowData ( HKEY, char*, PWINDOW_DATA );

BOOL    LoadFont( HKEY, char*, LPLOGFONT );
BOOL    SaveFont( HKEY, char*, LPLOGFONT );

BOOL    LoadMRUList ( HKEY, char*, DWORD, DWORD, DWORD );
BOOL    SaveMRUList( HKEY, char*, DWORD );

BOOL    DeleteKeyRecursive( HKEY, LPSTR  );

LPSTR   LoadMultiString( HKEY, LPSTR , DWORD * );

BOOL    GetProgramPath( HKEY, LPSTR, LPSTR );
BOOL    SetProgramPath( HKEY, LPSTR );

*/

int WINAPIV CompareViewOrder(const void *, const void *);

#define WS_STR_VIEWKEY_TEMPLATE     "%.3d"


/*
SHE         LoadTimeToShe ( LOADTIME );
LOADTIME    SheToLoadTime( SHE );
LPSTR       GetExtensionDllNames(LPDWORD);
VOID        SetExtensionDllNames(LPSTR);
*/



// **********************************************************
//                   EXPORTED FUNCTIONS
// **********************************************************

// **********************************************************
//                   WORKSPACE FUNCTIONS
// **********************************************************



BOOL
LoadAllWindows(
    VOID
    )
/*++

Routine Description:

    Loads all windows

Arguments:

Return Value:

    BOOL    - TRUE if windows loaded

--*/
{
    int         View;
    HKEY        hKey;
    DWORD       Dword;
    DWORD       DataSize;
    FILETIME    FileTime;
    DWORD       SubKeys;
    DWORD       Error;
    int         Order;
    char        Buffer[ BIG_BUFFER_HACK ];
    DWORD       i   = 0;
    BOOL        Ok  = TRUE;
    VIEW_ORDER  ViewOrder[ MAX_VIEWS ];

    CIndivWinLayout_WKSP * pIndWinLayout;

    //
    // Reposition the main window
    //
    {
        WINDOWPLACEMENT wndpl = {0};

        wndpl.length = sizeof(wndpl);
        wndpl.rcNormalPosition.top = g_contFrameWindow.m_nX;
        wndpl.rcNormalPosition.left = g_contFrameWindow.m_nY;
        wndpl.rcNormalPosition.right = g_contFrameWindow.m_nX + g_contFrameWindow.m_nWidth;
        wndpl.rcNormalPosition.bottom = g_contFrameWindow.m_nY + g_contFrameWindow.m_nHeight;
        wndpl.showCmd = g_contFrameWindow.m_nWindowState;

        SetWindowPlacement(hwndFrame, &wndpl);
    }

    /*
    MoveWindow(hwndFrame,
               g_contFrameWindow.m_nX,
               g_contFrameWindow.m_nY,
               g_contFrameWindow.m_nWidth,
               g_contFrameWindow.m_nHeight,
               TRUE
               );
    ShowWindow(hwndFrame, g_contFrameWindow.m_nWindowState);
    */

    g_logfontDefault = g_contFrameWindow.m_logfont;


    //
    // Load the child windows
    //

    TList<CIndivWinLayout_WKSP *> &list = g_dynacontAllChildWindows.m_listConts;

    //
    //  If replacing, get rid of all the current windows of this particular
    //  type.
    //
    for ( View = 0; View < MAX_VIEWS; View++ ) {
        int Doc = Views[View].Doc;
        if ( Doc != -1 ) {
            SendMessage( Views[ View ].hwndFrame,
                         WM_SYSCOMMAND,
                         SC_CLOSE,
                         0L 
                         );
        }
    }

    //
    //  Load the views from the registry
    //

    //
    //  If loading all, determine the view ordering and load the views
    //  in the appropriate order.
    //

    //
    //  Determine view ordering
    //
    for ( View = 0; View < MAX_VIEWS; View++ ) {
        ViewOrder[View].View   = -1;
        ViewOrder[View].Order  = MAX_VIEWS+1;
        ViewOrder[View].pIndWinLayout = NULL;
    }

    TListEntry<CIndivWinLayout_WKSP *> * pContEntry = list.FirstEntry();
    for (; pContEntry != list.Stop(); pContEntry = pContEntry->Flink) {

        pIndWinLayout = pContEntry->m_tData;

        View = atoi( pIndWinLayout->m_pszRegistryName );

        if ( pIndWinLayout->m_nOrder < MAX_VIEWS ) {
            Order = MAX_VIEWS - pIndWinLayout->m_nOrder;
        } else {
            Order = View;
        }


        ViewOrder[View].View  = View;
        ViewOrder[View].Order = Order;
        ViewOrder[View].pIndWinLayout = pIndWinLayout;
    }

    qsort( ViewOrder, sizeof( ViewOrder )/sizeof( ViewOrder[0] ), sizeof( ViewOrder[0] ), CompareViewOrder );

    //
    //  Now load the views in order
    //
    for ( View = 0;
          View < MAX_VIEWS && ViewOrder[View].View != -1;
          View++ ) {

        LoadView( ViewOrder[View].View, ViewOrder[View].pIndWinLayout);
    }


    //
    // Repaint the debugger
    //
/*
    RedrawWindow(hwndFrame, 
                 NULL,
                 NULL,
                 RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN
                 );
    UpdateWindow(hwndFrame);
*/

    return Ok;
}





BOOL
SaveAllWindows(
    VOID
    )
/*++

Routine Description:

    Saves all windows

Arguments:

Return Value:

    BOOL    - TRUE if windows saved

--*/
{
    int         View;
    BOOL        Ok = TRUE;
    HWND        hwnd;
    int         Order = 0;
    VIEW_ORDER  ViewOrder[ MAX_VIEWS ] = {0};


    //
    // Reposition the main window
    //
    /*
    GetWindowMetrics(hwndFrame,
                     &g_contFrameWindow,
                     FALSE
                     );
    */

    //
    // Reposition the main window
    //
    {
        WINDOWPLACEMENT wndpl = {0};

        wndpl.length = sizeof(wndpl);

        GetWindowPlacement(hwndFrame, &wndpl);

        g_contFrameWindow.m_nX = wndpl.rcNormalPosition.top;
        g_contFrameWindow.m_nY = wndpl.rcNormalPosition.left;
        g_contFrameWindow.m_nWidth = wndpl.rcNormalPosition.right - wndpl.rcNormalPosition.left;
        g_contFrameWindow.m_nHeight = wndpl.rcNormalPosition.bottom - wndpl.rcNormalPosition.top;

        if ( wndpl.showCmd == SW_SHOWMAXIMIZED ) {
            g_contFrameWindow.m_nWindowState = WSTATE_MAXIMIZED;
        } else if ( wndpl.showCmd == SW_SHOWMINIMIZED ) {
            g_contFrameWindow.m_nWindowState = WSTATE_ICONIC;
        } else {
            g_contFrameWindow.m_nWindowState = WSTATE_NORMAL;
        }
    }

    g_contFrameWindow.m_logfont = g_logfontDefault;


    //
    // Clear out the contents of the container
    //
    g_dynacontAllChildWindows.Containers_DeleteList();


    //
    //  If saving all, determine the view ordering.
    //

    for ( View = 0; View < MAX_VIEWS; View++ ) {
        ViewOrder[View].View   = View;
        ViewOrder[View].Order  = -1;
        ViewOrder[View].pIndWinLayout = NULL;
    }

    //hwnd = GetWindow( g_hwndMDIClient, GW_CHILD );

    hwnd = GetTopWindow (g_hwndMDIClient);
    while ( hwnd ) {

        View = GetWindowWord( hwnd, GWW_VIEW );

        if ( 0 <= View && View < MAX_VIEWS ) {
            ViewOrder[View].Order = Order++;
        }

        hwnd = GetNextWindow( hwnd, GW_HWNDNEXT );
    }


    for ( View = 0; Ok && View < MAX_VIEWS; View++ ) {
        int Doc = Views[View].Doc;
        if ( Doc != -1 ) {

            CIndivWinLayout_WKSP * pIndWinLayout = new CIndivWinLayout_WKSP;

            if ( !pIndWinLayout ) {
                continue; // Maybe the next one will succeed...
            }

            pIndWinLayout->Init(&g_dynacontAllChildWindows,
                                "Tmp",
                                FALSE,
                                FALSE
                                );
                               
            Ok &= SaveView(View,
                           ViewOrder[View].Order,
                           pIndWinLayout
                           );
        }
    }

    return Ok;
}



BOOL
LoadView (
    int                     WantedView,
    CIndivWinLayout_WKSP    *pIndWinLayout
    )
/*++

Routine Description:

    Loads a view

Arguments:

    WantedView  -   Supplies desired view
    pIndWinLayout -   Supplies all data necessary to open a window

Return Value:

    BOOL    - TRUE if view loaded

--*/
{
    BOOL            Ok          = TRUE;
    //WORD            Mode        = MODE_CREATE;
    char           *szFileName    = NULL;
    HFONT           Font        = 0;
    DWORD           DataSize;
    //WINDOW_DATA     WindowData;
    WININFO         WinInfo;
    int             View;
    DWORD           iFormat = MW_BYTE;
    DWORD           Live    = FALSE;
    //LPSTR           List    = NULL;
    //DWORD           Next    = 0;
    LPDOCREC        Doc;
    //DWORD           Line;
    DWORD           Column;
    //DWORD           ReadOnly = (DWORD)FALSE;
    //PLONG           Pane;
    CHOOSEFONT      Cf;
    //UINT            uSwitch;
    LPVIEWREC       v;
    HDC             hDC;
    TEXTMETRIC      tm;

    //
    //  Get window placement
    //

    WinInfo.coord.left   =   pIndWinLayout->m_nX;
    WinInfo.coord.top    =   pIndWinLayout->m_nY;
    WinInfo.coord.right  =   pIndWinLayout->m_nX + pIndWinLayout->m_nWidth;
    WinInfo.coord.bottom =   pIndWinLayout->m_nY + pIndWinLayout->m_nHeight;

    WinInfo.style = 0;

    Ok &= ( (Font = CreateFontIndirect( &pIndWinLayout->m_logfont )) != 0 );

    if (pIndWinLayout->m_nType == DOC_WIN) {

        View = AddFile( MODE_OPENCREATE,
                        (WORD)pIndWinLayout->m_nType,
                        pIndWinLayout->m_pszFileName,
                        &WinInfo,
                        Font,
                        pIndWinLayout->m_bReadOnly,
                        -1,
                        WantedView,
                        TRUE
                        );

        Ok = (BOOL)( View != -1 );

    } else {
        View = OpenDebugWindowEx(pIndWinLayout->m_nType, 
                               &WinInfo, 
                               WantedView,
                               TRUE // User activated
                               ); 
        Ok = (View != -1);
        if ( Ok ) {

            UINT uSwitch; 
            LOGFONT LogFont = pIndWinLayout->m_logfont;
            
            v = &Views[View];
            
            Cf.lStructSize      = sizeof (CHOOSEFONT);
            Cf.hwndOwner        = v->hwndClient;
            Cf.hDC              = NULL;
            Cf.lpLogFont        = &LogFont;
            
            if (v->Doc < -1) {
                uSwitch = -(v->Doc);
            } else {
                uSwitch = Docs[ v->Doc ].docType;
            }
            
            switch (uSwitch) {
                
            case WATCH_WIN:
                SendMessageNZ( GetWatchHWND(), WM_SETFONT, 0, (LPARAM)&Cf);
                break;
                
            case LOCALS_WIN:
                SendMessageNZ( GetLocalHWND(), WM_SETFONT, 0, (LPARAM)&Cf);
                break;
                
            case CALLS_WIN:
                SendMessageNZ( GetCallsHWND(), WM_SETFONT, 0, (LPARAM)&Cf);
                break;
                
            case CPU_WIN:
                SendMessageNZ( GetCpuHWND(), WM_SETFONT, 0, (LPARAM)&Cf);
                break;
                
            case FLOAT_WIN:
                SendMessageNZ( GetFloatHWND(), WM_SETFONT, 0, (LPARAM)&Cf);
                break;
                
            default:
                Dbg(hDC = GetDC(v->hwndClient));
                v->font = Font;
                Dbg(SelectObject(hDC, v->font));
                Dbg(GetTextMetrics (hDC, &tm));
                v->charHeight   = tm.tmHeight;
                v->maxCharWidth = tm.tmMaxCharWidth;
                v->aveCharWidth = tm.tmAveCharWidth;
                v->charSet      = tm.tmCharSet;
                GetCharWidth(hDC,
                             0,
                             MAX_CHARS_IN_FONT - 1,
                             (LPINT)v->charWidth
                             );
                Dbg(ReleaseDC (v->hwndClient, hDC));
                DestroyCaret ();
                CreateCaret(v->hwndClient, 0, 3, v->charHeight);
                PosXY(View, v->X, v->Y, FALSE);
                ShowCaret(v->hwndClient);
                SendMessage(v->hwndClient, WM_FONTCHANGE, 0, 0L);
                InvalidateRect(v->hwndClient, (LPRECT)NULL, FALSE);
            }
        }
    }

    if (Ok) {

        if ( pIndWinLayout->m_nType == WATCH_WIN  ||
             pIndWinLayout->m_nType == LOCALS_WIN ||
             pIndWinLayout->m_nType == CPU_WIN    ||
             pIndWinLayout->m_nType == FLOAT_WIN ) {

            LONG lLen = sizeof(INFO) + _tcslen(pIndWinLayout->m_pszExpression) +1;
            PINFO pinfo = (PINFO) calloc(1, lLen);

            pinfo->Length  = lLen;
            pinfo->Flags.Expand1st = pIndWinLayout->m_dwPaneFlags;
            pinfo->PerCent = pIndWinLayout->m_dwPerCent;
            _tcscpy(pinfo->Text, pIndWinLayout->m_pszExpression);

            SetPaneStatus( View, (PLONG) pinfo );

            free(pinfo);
        }

        ShowWindow( Views[View].hwndFrame, pIndWinLayout->m_nWindowState );

        switch ( pIndWinLayout->m_nType ) {

        case DOC_WIN:
            if ( pIndWinLayout->m_pszFileName ) {

                int nLine;

                nLine = min(
                            max(1, (int)pIndWinLayout->m_nLine), 
                            (int)Docs[Views[View].Doc].NbLines
                            ) - 1;

                PosXY(View, pIndWinLayout->m_nColumn, nLine, FALSE);
            }
            break;

        case MEMORY_WIN:
            {
                char Buffer[ BIG_BUFFER_HACK ];
            
                MemWinDesc[ View ].iFormat    = pIndWinLayout->m_nFormat;
                MemWinDesc[ View ].fLive      = pIndWinLayout->m_bLive;
                MemWinDesc[ View ].atmAddress = AddAtom( pIndWinLayout->m_pszExpression );
                strcpy( MemWinDesc[ View ].szAddress, pIndWinLayout->m_pszExpression );
            
                Doc = &Docs[ Views[ View].Doc ];
                
                Dbg(LoadString(g_hInst, 
                               SYS_MemoryWin_Title, 
                               Buffer, 
                               sizeof( Buffer )
                               ));
                
                RemoveMnemonic(Buffer, Doc->szFileName);
                lstrcat (Doc->szFileName,"(");
                lstrcat (Doc->szFileName, MemWinDesc[ View ].szAddress);
                lstrcat (Doc->szFileName,")");
            
                RefreshWindowsTitle( Views[ View].Doc );
        
                ViewMem( View, FALSE );
            }
            break;

        case WATCH_WIN:
            /*
            //
            //  NOTENOTE Ramonsa - Add watch expressions when new
            //  watch window code is on-line.
            //
            if ( Ok && (List = LoadMultiString( hKey, WS_STR_EXPRESSION, &DataSize )) ) {
                while ( Ok && (String = GetNextStringFromMultiString( List,
                                                                      DataSize,
                                                                      &Next )) ) {
                    Ok &= (AddWatchNode( String ) != NULL );
                }
                DeallocateMultiString( List );
            }
            */
            break;

        default:
            break;
        }
    }

    return Ok;
}




BOOL
SaveView (
    int             View,
    int             Order,
    CIndivWinLayout_WKSP    *pIndWinLayout
    )
/*++

Routine Description:

    Saves a view

Arguments:

    View        -   Supplies index of view
    Order       -   Supplies view order

Return Value:

    BOOL    - TRUE if view saved

--*/
{
    HKEY                hKey;
    //WINDOW_DATA         WindowData;
    LOGFONT             LogFont;
    DWORD               Dword;
    char                Buffer[ BIG_BUFFER_HACK ];
    LPSTR               List        = NULL;
    DWORD               ListLength  = 0;
    BOOL                Ok          = TRUE;


    sprintf( Buffer, WS_STR_VIEWKEY_TEMPLATE, View );
    pIndWinLayout->SetRegistryName(Buffer);


    //
    //  Save view type
    //

    if (Views[ View ].Doc < -1) {
        pIndWinLayout->m_nType = -(Views[ View ].Doc);
    } else {
        pIndWinLayout->m_nType = Docs[ Views[ View ].Doc ].docType;
    }

    Assert( pIndWinLayout->m_nType >= 0 );

    if ( pIndWinLayout->m_nType == WATCH_WIN  ||
         pIndWinLayout->m_nType == LOCALS_WIN ||
         pIndWinLayout->m_nType == CPU_WIN    ||
         pIndWinLayout->m_nType == FLOAT_WIN ) {

        PINFO pinfo = (PINFO) GetPaneStatus( View );

        if ( pinfo ) {

            pIndWinLayout->m_dwPaneFlags = pinfo->Flags.Expand1st;
            pIndWinLayout->m_dwPerCent   = pinfo->PerCent;
            FREE_STR( pIndWinLayout->m_pszExpression );
            pIndWinLayout->m_pszExpression = _tcsdup(pinfo->Text);

            FreePaneStatus( View, (PLONG) pinfo );
        }
    }

    //
    //  Save window placement
    //
    if ( GetWindowMetrics( Views[ View ].hwndFrame, pIndWinLayout, TRUE ) ) {


        GetObject(Views[ View ].font, 
                  sizeof( LogFont ), 
                  &pIndWinLayout->m_logfont
                  );

        pIndWinLayout->m_nOrder = Order;

        //
        //  Save extra stuff according to type
        //

        switch ( pIndWinLayout->m_nType ) {
            
        case DOC_WIN:
            FREE_STR(pIndWinLayout->m_pszFileName);
            
            if ( *Docs[ Views[ View].Doc ].szFileName ) {
                pIndWinLayout->m_pszFileName = _tcsdup( Docs[ Views[ View].Doc ].szFileName );
            }
            
            pIndWinLayout->m_bReadOnly = Docs[ Views[ View].Doc ].readOnly;
            
            pIndWinLayout->m_nLine = Views[ View].Y + 1;
            pIndWinLayout->m_nColumn = Views[ View].X;
            break;
            
            
        case MEMORY_WIN:
            GetAtomName( MemWinDesc[ View ].atmAddress, Buffer, sizeof( Buffer ) );
            
            FREE_STR(pIndWinLayout->m_pszExpression);
            pIndWinLayout->m_pszExpression = _tcsdup( Buffer );

            pIndWinLayout->m_nFormat = MemWinDesc[ View ].iFormat;
            
            pIndWinLayout->m_bLive = MemWinDesc[ View ].fLive;
            break;
            
            
        case WATCH_WIN:
            /*
            //
            //  NOTENOTE Ramonsa - Save watch expressions when new
            //  watch window code is on-line.
            //
            if ( !Default ) {
                //
                //  Save all the watch expressions
                //
                WatchNode = GetFirstWatchNode();
                while ( Ok && WatchNode ) {
                    Ok &= AddToMultiString( &List, &ListLength, WatchNode->Expr );
                    WatchNode = WatchNode->Next;
                }

                if ( Ok && List ) {

                    if ( RegSetValueEx( hKey,
                                        WS_STR_EXPRESSION,
                                        0,
                                        REG_MULTI_SZ,
                                        List,
                                        ListLength
                                        ) != NO_ERROR ) {

                        Ok = FALSE;
                    }
                }

                if ( List ) {
                    DeallocateMultiString( List );
                }
            }
            */
            break;
            
        default:
            break;
        }
    }

    return Ok;
}






BOOL
GetWindowMetrics (
    HWND                    Hwnd,
    CIndivWinLayout_WKSP   *pIndWinLayout,
    BOOL                    Client
    )
/*++

Routine Description:

    Fills out the WindowData structure with the data that can
    be obtained from a window handle.

Arguments:

    Hwnd            -   Supplies the window handle
    WindowData      -   Supplies window data structure
    Client          -   Supplies flag which if TRUE means that
                        we want Client metrics

Return Value:

    BOOL    - TRUE if data filled out

--*/
{
    BOOL            Ok = FALSE;
    WINDOWPLACEMENT WindowPlacement;
    RECT            Rect;


    WindowPlacement.length = sizeof(WINDOWPLACEMENT);

    if ( GetWindowPlacement( Hwnd, &WindowPlacement ) ) {

        pIndWinLayout->m_nX      = WindowPlacement.rcNormalPosition.left;
        pIndWinLayout->m_nY      = WindowPlacement.rcNormalPosition.top;
        pIndWinLayout->m_nWidth  = WindowPlacement.rcNormalPosition.right -
                                   WindowPlacement.rcNormalPosition.left;
        pIndWinLayout->m_nHeight = WindowPlacement.rcNormalPosition.bottom -
                                   WindowPlacement.rcNormalPosition.top;

        if ( WindowPlacement.showCmd == SW_SHOWMAXIMIZED ) {
            pIndWinLayout->m_nWindowState = WSTATE_MAXIMIZED;
        } else if ( WindowPlacement.showCmd == SW_SHOWMINIMIZED ) {
            pIndWinLayout->m_nWindowState = WSTATE_ICONIC;
        } else {
            pIndWinLayout->m_nWindowState = WSTATE_NORMAL;
        }

        Ok = TRUE;
    }


    return Ok;
}






int WINAPIV
CompareViewOrder (
                 const void *p1,
                 const void *p2
                 )
/*++

Routine Description:

    Compares two VIEW_ORDER structures. Called by qsort

Arguments:

    See qsort()

Return Value:

    See qsort()

--*/
{
    int Order1 = ((PVIEW_ORDER)p1)->Order;
    int Order2 = ((PVIEW_ORDER)p2)->Order;

    if ( Order1 == Order2 ) {
        return 0;
    } else if ( Order1 < Order2 ) {
        return -1;
    } else {
        return 1;
    }
}




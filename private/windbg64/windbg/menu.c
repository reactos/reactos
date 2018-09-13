/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    Menu.c

Abstract:

    This module contains the support for Windbg's menu.

Author:

    David J. Gilman (davegi) 16-May-1992

Environment:

    Win32, User Mode

--*/

#include "precomp.h"
#pragma hdrstop

HWND GetWatchHWND(void);


extern LRESULT SendMessageNZ(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);



//Keep the 4 most recently used files (editor and project)
// Being globals, automatically initialized to 0.
HANDLE hFileKept[PROJECT_FILE + 1][MAX_MRU_FILES_KEPT];
int nbFilesKept[PROJECT_FILE + 1];

//Last menu id & id state
IWORD FAR lastMenuId;
IWORD FAR lastMenuIdState;

//Window submenu
HMENU hWindowSubMenu;

//
// Handle to main window menu.
//

HMENU hMainMenu;
HMENU hMainMenuSave;



//
// EnableMenuItemTable contains the menu IDs for all menu items whose
// enabled state needs to be determined dynamically i.e. based on the state
// of Windbg.
//


UINT
EnableMenuItemTable[ ] = {
    IDM_FILE_OPEN,
    IDM_FILE_CLOSE,
    IDM_FILE_OPEN_EXECUTABLE,
    IDM_FILE_OPEN_CRASH_DUMP,
    IDM_FILE_NEW_WORKSPACE,
    IDM_FILE_SAVE_WORKSPACE,
    IDM_FILE_SAVEAS_WORKSPACE,

    IDM_EDIT_CUT,
    IDM_EDIT_COPY,
    IDM_EDIT_PASTE,
    IDM_EDIT_FIND,
    IDM_EDIT_REPLACE,
    IDM_EDIT_GOTO_ADDRESS,
    IDM_EDIT_GOTO_LINE,
    IDM_EDIT_PROPERTIES,

    IDM_VIEW_COMMAND,
    IDM_VIEW_WATCH,
    IDM_VIEW_CALLSTACK,
    IDM_VIEW_MEMORY,
    IDM_VIEW_LOCALS,
    IDM_VIEW_REGISTERS,
    IDM_VIEW_DISASM,
    IDM_VIEW_FLOAT,
    IDM_VIEW_TOGGLETAG,
    IDM_VIEW_NEXTTAG,
    IDM_VIEW_PREVIOUSTAG,
    IDM_VIEW_CLEARALLTAGS,
    IDM_VIEW_TOOLBAR,
    IDM_VIEW_STATUS,
    IDM_VIEW_OPTIONS,

    IDM_DEBUG_GO,
    IDM_DEBUG_RESTART,
    IDM_DEBUG_STOPDEBUGGING,
    IDM_DEBUG_BREAK,
    IDM_DEBUG_STEPINTO,
    IDM_DEBUG_STEPOVER,
    IDM_DEBUG_RUNTOCURSOR,
    IDM_DEBUG_QUICKWATCH,
    IDM_DEBUG_SOURCE_MODE,
    IDM_DEBUG_SOURCE_MODE_ON,
    IDM_DEBUG_SOURCE_MODE_OFF,

    IDM_DEBUG_EXCEPTIONS,
    IDM_DEBUG_SET_THREAD,
    IDM_DEBUG_SET_PROCESS,
    IDM_DEBUG_ATTACH,
    IDM_DEBUG_GO_UNHANDLED,
    IDM_DEBUG_GO_HANDLED,

    IDM_WINDOW_NEWWINDOW,
    IDM_WINDOW_CASCADE,
    IDM_WINDOW_TILE_HORZ,
    IDM_WINDOW_TILE_VERT,
    IDM_WINDOW_ARRANGE,
    IDM_WINDOW_ARRANGE_ICONS
};

#define ELEMENTS_IN_ENABLE_MENU_ITEM_TABLE          \
    ( sizeof( EnableMenuItemTable ) / sizeof( EnableMenuItemTable[ 0 ] ))


#if defined( NEW_WINDOWING_CODE )

UINT
CommandIdEnabled(
    IN UINT MenuID
    )

/*++

Routine Description:

    Determines if a menu item is enabled/disabled based on the current
    state of the debugger.

Arguments:

    MenuID - Supplies a menu id whose state is to be determined.

Return Value:

    UINT - Returns ( MF_ENABLED | MF_BYCOMMAND ) if the supplied menu ID
        is enabled, ( MF_GRAYED | MF_BYCOMMAND) otherwise.

--*/
{
    BOOL        bEnabled;
    //LPVIEWREC   lpviewrec = NULL;
    //LPDOCREC    lpdocrec;
    BOOL        bNormalWin;
    //UINT        uFormat;
    int         i;
    //PPANE       ppane;
    LPSTR       lpszList;
    DWORD       dwListLength;
    
    HWND        hwndChild = MDIGetActive(g_hwndMDIClient, NULL);
    PCOMMONWIN_DATA pCommonWinData = GetCommonWinData( hwndChild );
    WIN_TYPES   nDocType;

    if (pCommonWinData) {
        nDocType = pCommonWinData->m_enumType;
    } else {
        nDocType = MINVAL_WINDOW;
    }

    //
    // Determine the state of the debugger.
    //
    if (DbgState != ds_normal) {
        //
        // Just to be on the safe side, we wait until the debugger
        // has finished bootstrapping, before we enable them. Just
        // to make sure we don't crash the debugger.
        //
        return FALSE;
    }
    //
    // The Debugger is in a sane state.
    //


    //
    // If there is an active edit control, remember the current view,
    // document and whether or not the active window is an icon.
    //

/*
    if (curView >= 0) {
        lpviewrec = &Views[ curView ];
    }

    lpdocrec = NULL;
    ppane = NULL;
*/
    bNormalWin = FALSE;
    if (hwndChild) {
        bNormalWin = ! IsIconic( hwndChild );
    }
/*
    if ( hwndActiveEdit ) {
        if (lpviewrec->Doc > -1) {
            lpdocrec = &Docs[ lpviewrec->Doc ];
            nDocType = lpdocrec->docType;
        } else if (lpviewrec->Doc < -1) {
            ppane = (PPANE)GetWindowLongPtr( hwndActive, GWW_EDIT);
            nDocType = -(lpviewrec->Doc);
        }
        bNormalWin = ! IsIconic( hwndActive );
    }
*/

    //
    // Assume menu item is not enabled.
    //

    bEnabled = FALSE;

    switch( MenuID ) {

    case IDM_FILE_OPEN:
    case IDM_FILE_SAVE_WORKSPACE:
    case IDM_FILE_SAVEAS_WORKSPACE:
    case IDM_FILE_SAVE_AS_WINDOW_LAYOUTS:
    case IDM_FILE_MANAGE_WINDOW_LAYOUTS:
    case IDM_EDIT_BREAKPOINTS:
    case IDM_VIEW_COMMAND:
    case IDM_VIEW_WATCH:
    case IDM_VIEW_CALLSTACK:
    case IDM_VIEW_MEMORY:
    case IDM_VIEW_LOCALS:
    case IDM_VIEW_REGISTERS:
    case IDM_VIEW_DISASM:
    case IDM_VIEW_FLOAT:
    case IDM_VIEW_TOOLBAR:
    case IDM_VIEW_STATUS:
    case IDM_VIEW_OPTIONS:
    case IDM_DEBUG_EXCEPTIONS:
        // Always enabled.
        bEnabled = TRUE;
        break;

    case IDM_DEBUG_SOURCE_MODE:
    case IDM_DEBUG_SOURCE_MODE_ON:
    case IDM_DEBUG_SOURCE_MODE_OFF:
        bEnabled = TRUE;

        CheckMenuItem(hMainMenu, 
                      IDM_DEBUG_SOURCE_MODE,
                      GetSrcMode_StatusBar() ? MF_CHECKED : MF_UNCHECKED
                      );
        SendMessageNZ(GetHwnd_Toolbar(), 
                      TB_CHECKBUTTON,
                      IDM_DEBUG_SOURCE_MODE_ON, 
                      MAKELONG(GetSrcMode_StatusBar(), 0)
                      );
        SendMessageNZ(GetHwnd_Toolbar(), 
                      TB_CHECKBUTTON,
                      IDM_DEBUG_SOURCE_MODE_OFF, 
                      MAKELONG(!GetSrcMode_StatusBar(), 0)
                      );
        break;

    case IDM_FILE_CLOSE:
        bEnabled =  (NULL != hwndActive);
        break;

    case IDM_FILE_OPEN_EXECUTABLE:
    case IDM_FILE_OPEN_CRASH_DUMP:
        bEnabled = !DebuggeeActive() && !g_contWorkspace_WkSp.m_bUserCrashDump
            && !g_contKernelDbgPreferences_WkSp.m_bUseCrashDump;
        break;

    case IDM_FILE_NEW_WORKSPACE:
    case IDM_FILE_MANAGE_WORKSPACE:
        bEnabled = !(LptdCur && LptdCur->tstate == tsRunning);
        break;

    case IDM_EDIT_CUT:
        if ( pCommonWinData ) {
            bEnabled = pCommonWinData->CanCut();
        } else {
            bEnabled = FALSE;
        }

        SendMessageNZ(GetHwnd_Toolbar(), TB_ENABLEBUTTON, MenuID, MAKELONG(bEnabled, 0));
        break;

    case IDM_EDIT_COPY:
        if ( pCommonWinData ) {
            bEnabled = pCommonWinData->CanCopy();
        } else {
            bEnabled = FALSE;
        }

        SendMessageNZ(GetHwnd_Toolbar(), TB_ENABLEBUTTON, MenuID, MAKELONG(bEnabled, 0));
        break;

    case IDM_EDIT_PASTE:
        //
        // If the window is normal, is not read only and is a document
        // or cmdwin, determine if the clipboard contains pastable data
        // (i.e. clipboard format CF_TEXT).
        //

        if ( !(pCommonWinData && pCommonWinData->CanPaste()) ) {
            bEnabled = FALSE;
        } else {
            UINT uFormat = 0;
            while ( uFormat = EnumClipboardFormats( uFormat )) {
                if ( uFormat == CF_TEXT ) {
                    bEnabled = TRUE;
                    break;
                }
            }
        }
        CloseClipboard();
        
        SendMessageNZ(GetHwnd_Toolbar(), TB_ENABLEBUTTON, MenuID, MAKELONG(bEnabled, 0));
        break;

    case IDM_EDIT_FIND:
        if (pCommonWinData == NULL) {
            bEnabled = FALSE;
        } else {
            bEnabled = bNormalWin
                && ((nDocType == DOC_WINDOW) || (MDIGetActive(g_hwndMDIClient, NULL) == g_DebuggerWindows.hwndCmd))
                && frMem.hDlgFindNextWnd == 0
                && frMem.hDlgConfirmWnd == 0;
        }
        break;

    case IDM_EDIT_REPLACE:
        bEnabled = bNormalWin
            && (nDocType == DOC_WINDOW)
            && frMem.hDlgFindNextWnd == 0
            && frMem.hDlgConfirmWnd == 0;
        break;

    case IDM_EDIT_GOTO_ADDRESS:
        bEnabled = DebuggeeActive();
        break;

    case IDM_EDIT_GOTO_LINE:
        if (pCommonWinData == NULL) {
            bEnabled = FALSE;
        } else {
            bEnabled = bNormalWin                   // Not iconized
                && (DOC_WINDOW == nDocType);  // We have a text file
        }
        break;

    case IDM_EDIT_PROPERTIES:
        bEnabled = (CALLS_WINDOW == nDocType) ||
                   (DebuggeeActive() &&
                    (WATCH_WINDOW == nDocType ||
                     LOCALS_WINDOW == nDocType ||
                     MEMORY_WINDOW == nDocType));
        SendMessageNZ(GetHwnd_Toolbar(),
                      TB_ENABLEBUTTON,
                      MenuID,
                      MAKELONG(bEnabled, 0));
        break;

    case IDM_VIEW_TOGGLETAG:
    case IDM_VIEW_NEXTTAG:
    case IDM_VIEW_PREVIOUSTAG:
    case IDM_VIEW_CLEARALLTAGS:
        bEnabled = bNormalWin                           // Not iconized
            && (DOC_WINDOW == nDocType);      // We have a document & We have a text file
        break;

    case IDM_DEBUG_GO:
        if (DebuggeeActive()) {
            bEnabled = GoOK(LppdCur, LptdCur);
        } else {
            bEnabled = (GetLppdHead() == (LPPD)0);
        }
        SendMessageNZ(GetHwnd_Toolbar(),
                      TB_ENABLEBUTTON,
                      MenuID,
                      MAKELONG(bEnabled, 0));
        break;

    case IDM_DEBUG_RESTART:
        bEnabled = !IsProcRunning( LppdCur ) &&
                   !(g_contWorkspace_WkSp.m_bKernelDebugger);
        SendMessageNZ(GetHwnd_Toolbar(),
                      TB_ENABLEBUTTON,
                      MenuID,
                      MAKELONG(bEnabled, 0));
        break;

    case IDM_DEBUG_STOPDEBUGGING:
        if (g_contWorkspace_WkSp.m_bKernelDebugger) {
            bEnabled = FALSE;
        } else {
            bEnabled = DebuggeeAlive();
        }
        SendMessageNZ(GetHwnd_Toolbar(),
                      TB_ENABLEBUTTON,
                      MenuID,
                      MAKELONG(bEnabled, 0));
        break;

    case IDM_DEBUG_BREAK:
        bEnabled = DebuggeeActive() && IsProcRunning(LppdCur);
        SendMessageNZ(GetHwnd_Toolbar(),
                      TB_ENABLEBUTTON,
                      MenuID,
                      MAKELONG(bEnabled, 0));
        break;

    case IDM_DEBUG_STEPINTO:
    case IDM_DEBUG_STEPOVER:
        if (DebuggeeActive()) {
            bEnabled = StepOK(LppdCur, LptdCur);
        } else {
            bEnabled = (GetLppdHead() == (LPPD)0);
        }
        SendMessageNZ(GetHwnd_Toolbar(),
                      TB_ENABLEBUTTON,
                      MenuID,
                      MAKELONG(bEnabled, 0));
        break;

    case IDM_DEBUG_RUNTOCURSOR:
        //
        // In addition, for IDM_RUN_CONTINUE_TO_CURSOR, caret must be in a
        // document or the disassembler window.
        //
        if (!DebuggeeAlive()) {              // not started?
            bEnabled = (DOC_WIN == nDocType);
        } else if (DOC_WIN == nDocType        // src or asm win?
                  || DISASM_WIN == nDocType
                  || CALLS_WINDOW == nDocType) {
            bEnabled = GoOK(LppdCur, LptdCur);
        } else {                                    // other.
            bEnabled = FALSE;
        }
        SendMessageNZ(GetHwnd_Toolbar(),
                      TB_ENABLEBUTTON,
                      MenuID,
                      MAKELONG(bEnabled, 0));
        break;

    case IDM_DEBUG_QUICKWATCH:
        bEnabled = DebuggeeActive() && !IsProcRunning(LppdCur);
        SendMessageNZ(GetHwnd_Toolbar(),
                      TB_ENABLEBUTTON,
                      MenuID,
                      MAKELONG(bEnabled, 0));
        break;

    case IDM_DEBUG_SET_THREAD:
    case IDM_DEBUG_SET_PROCESS:
        bEnabled = DebuggeeActive();
        break;

    case IDM_DEBUG_ATTACH:
        if (OsVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
            if (g_contWorkspace_WkSp.m_bKernelDebugger) {
                bEnabled = FALSE;
            } else {
                bEnabled = TRUE;
            }
        }
        break;

    case IDM_DEBUG_GO_HANDLED:
    case IDM_DEBUG_GO_UNHANDLED:
        if (DebuggeeActive()) {
            bEnabled = GoExceptOK(LppdCur, LptdCur);
        }
        break;

/*
    case IDM_WINDOW_NEWWINDOW:
        bEnabled = pCommonWinData           // We have a doc
            && hwndActiveEdit                   // Currently active
            && nDocType == DOC_WINDOW;    // Text file
        break;
*/

    case IDM_WINDOW_CASCADE:
    case IDM_WINDOW_TILE_HORZ:
    case IDM_WINDOW_TILE_VERT:
    case IDM_WINDOW_ARRANGE:
        bEnabled = hwndActiveEdit != NULL;
        break;

    case IDM_WINDOW_ARRANGE_ICONS:
        {
            HWND hwnd;
            if ((hwnd = GetCpuHWND()) && IsIconic (hwnd)) {
                bEnabled = TRUE;
            } else if ((hwnd = GetFloatHWND()) && IsIconic (hwnd)) {
                bEnabled = TRUE;
            } else if ((hwnd = GetLocalHWND()) && IsIconic (hwnd)) {
                bEnabled = TRUE;
            } else if ((hwnd = GetWatchHWND()) && IsIconic (hwnd)) {
                bEnabled = TRUE;
            } else {
                for (i = 0; i < MAX_VIEWS; i++) {
                    if (Views[i].hwndClient &&
                        IsIconic(GetParent(Views[i].hwndClient))) {
                        bEnabled = TRUE;
                        break;
                    }
                }
            }
        }
        break;

    default:
        // We should have handled everything.
        Assert(0);
        break;
    }

    return (( bEnabled ) ? MF_ENABLED : MF_GRAYED ) | MF_BYCOMMAND;
}

#else //NEW_WINDOWING_CODE 

UINT
CommandIdEnabled(
    IN UINT MenuID
    )

/*++

Routine Description:

    Determines if a menu item is enabled/disabled based on the current
    state of the debugger.

Arguments:

    MenuID - Supplies a menu id whose state is to be determined.

Return Value:

    UINT - Returns ( MF_ENABLED | MF_BYCOMMAND ) if the supplied menu ID
        is enabled, ( MF_GRAYED | MF_BYCOMMAND) otherwise.

--*/

{
    BOOL        bEnabled;
    LPVIEWREC   lpviewrec = NULL;
    LPDOCREC    lpdocrec;
    BOOL        bNormalWin;
    UINT        uFormat;
    int         i;
    int         nDocType = -1;
    PPANE       ppane;
    LPSTR       lpszList;
    DWORD       dwListLength;
    HWND        hwnd;


    //
    // Determine the state of the debugger.
    //
    if (DbgState != ds_normal) {
        //
        // Just to be on the safe side, we wait until the debugger
        // has finished bootstrapping, before we enable them. Just
        // to make sure we don't crash the debugger.
        //
        return FALSE;
    }
    //
    // The Debugger is in a sane state.
    //


    //
    // If there is an active edit control, remember the current view,
    // document and whether or not the active window is an icon.
    //

    if (curView >= 0) {
        lpviewrec = &Views[ curView ];
    }

    lpdocrec = NULL;
    ppane = NULL;
    bNormalWin = FALSE;

    if ( hwndActiveEdit ) {
        if (lpviewrec->Doc > -1) {
            lpdocrec = &Docs[ lpviewrec->Doc ];
            nDocType = lpdocrec->docType;
        } else if (lpviewrec->Doc < -1) {
            ppane = (PPANE)GetWindowLongPtr( hwndActive, GWW_EDIT);
            nDocType = -(lpviewrec->Doc);
        }
        bNormalWin = ! IsIconic( hwndActive );
    }

    //
    // Assume menu item is not enabled.
    //

    bEnabled = FALSE;

    switch( MenuID ) {

    case IDM_FILE_OPEN:
    case IDM_FILE_SAVE_WORKSPACE:
    case IDM_FILE_SAVEAS_WORKSPACE:
    case IDM_FILE_SAVE_AS_WINDOW_LAYOUTS:
    case IDM_FILE_MANAGE_WINDOW_LAYOUTS:
    case IDM_EDIT_BREAKPOINTS:
    case IDM_VIEW_COMMAND:
    case IDM_VIEW_WATCH:
    case IDM_VIEW_CALLSTACK:
    case IDM_VIEW_MEMORY:
    case IDM_VIEW_LOCALS:
    case IDM_VIEW_REGISTERS:
    case IDM_VIEW_DISASM:
    case IDM_VIEW_FLOAT:
    case IDM_VIEW_TOOLBAR:
    case IDM_VIEW_STATUS:
    case IDM_VIEW_OPTIONS:
    case IDM_DEBUG_EXCEPTIONS:
        // Always enabled.
        bEnabled = TRUE;
        break;

    case IDM_DEBUG_SOURCE_MODE:
    case IDM_DEBUG_SOURCE_MODE_ON:
    case IDM_DEBUG_SOURCE_MODE_OFF:
        bEnabled = TRUE;

        CheckMenuItem(hMainMenu, IDM_DEBUG_SOURCE_MODE,
            GetSrcMode_StatusBar() ? MF_CHECKED : MF_UNCHECKED);
        SendMessageNZ(GetHwnd_Toolbar(), TB_CHECKBUTTON,
            IDM_DEBUG_SOURCE_MODE_ON, MAKELONG(GetSrcMode_StatusBar(), 0));
        SendMessageNZ(GetHwnd_Toolbar(), TB_CHECKBUTTON,
            IDM_DEBUG_SOURCE_MODE_OFF, MAKELONG(!GetSrcMode_StatusBar(), 0));
        break;

    case IDM_FILE_CLOSE:
        bEnabled =  (NULL != hwndActive);
        break;

    case IDM_FILE_OPEN_EXECUTABLE:
    case IDM_FILE_OPEN_CRASH_DUMP:
        bEnabled = !DebuggeeActive() && !g_contWorkspace_WkSp.m_bUserCrashDump
            && !g_contKernelDbgPreferences_WkSp.m_bUseCrashDump;
        break;

    case IDM_FILE_NEW_WORKSPACE:
    case IDM_FILE_MANAGE_WORKSPACE:
        bEnabled = !(LptdCur && LptdCur->tstate == tsRunning);
        break;

    case IDM_EDIT_CUT:
        bEnabled = (lpdocrec != NULL)               // We have a document
            && bNormalWin                           // Not iconized
            && lpviewrec->BlockStatus
            && ( lpviewrec->BlockXL != lpviewrec->BlockXR || lpviewrec->BlockYL != lpviewrec->BlockYR )
            && !lpdocrec->readOnly
            && DOC_WIN == nDocType;        // We have a text file

        SendMessageNZ(GetHwnd_Toolbar(), TB_ENABLEBUTTON, MenuID, MAKELONG(bEnabled, 0));
        break;

    case IDM_EDIT_COPY:
       {
        BOOL Enable1 = FALSE;
        BOOL Enable2 = FALSE;

        if (lpdocrec == NULL) {
            Enable1 = FALSE;
            if (ppane) {
                Enable2 = (ppane->SelLen != 0);
            }
        } else {
            Enable1 = bNormalWin &&
                  ((lpviewrec->BlockStatus &&
                    ( lpviewrec->BlockXL != lpviewrec->BlockXR || lpviewrec->BlockYL != lpviewrec->BlockYR )));

            if (ppane == NULL) {
                Enable2 = FALSE;
            } else {
                Enable2 = (ppane->SelLen != 0);
            }
        }

        bEnabled = Enable1 || Enable2;

        SendMessageNZ(GetHwnd_Toolbar(), TB_ENABLEBUTTON, MenuID, MAKELONG(bEnabled, 0));
        break;
       }

    case IDM_EDIT_PASTE:
        {
            bEnabled = FALSE;

            //
            // If the window is normal, is not read only and is a document
            // or cmdwin, determine if the clipboard contains pastable data
            // (i.e. clipboard format CF_TEXT).
            //

            if (lpdocrec == NULL) {
                if (ppane != NULL) {
                    if (!ppane->ReadOnly && (OpenClipboard( hwndFrame ))) {
                        uFormat = 0;
                        while( uFormat = EnumClipboardFormats( uFormat )) {
                            if( uFormat == CF_TEXT ) {
                                bEnabled = TRUE;
                                break;
                            }
                        }
                        CloseClipboard();
                    }
                }
            } else if (bNormalWin &&
                ((((nDocType == DOC_WIN && !lpdocrec->readOnly) ||
                    (nDocType == COMMAND_WIN && !(lpdocrec->RORegionSet &&
                      ((lpviewrec->Y < lpdocrec->RoY2) || (lpviewrec->Y == lpdocrec->RoY2
                      && lpviewrec->X < lpdocrec->RoX2))))
                  ))) && OpenClipboard( hwndFrame ))
            {
                uFormat = 0;
                while( uFormat = EnumClipboardFormats( uFormat )) {
                    if ( uFormat == CF_TEXT ) {
                        bEnabled = TRUE;
                        break;
                    }
                }
                CloseClipboard();
            } else if (ppane != NULL) {
                if (!ppane->ReadOnly && (OpenClipboard( hwndFrame ))) {
                    uFormat = 0;
                    while ( uFormat = EnumClipboardFormats( uFormat )) {
                        if ( uFormat == CF_TEXT ) {
                            bEnabled = TRUE;
                            break;
                        }
                    }
                    CloseClipboard();
                }
            }
        }
        SendMessageNZ(GetHwnd_Toolbar(), TB_ENABLEBUTTON, MenuID, MAKELONG(bEnabled, 0));
        break;

    case IDM_EDIT_FIND:
        if (lpdocrec == NULL) {
            bEnabled = FALSE;
        } else {
            bEnabled = bNormalWin
                && ((nDocType == DOC_WIN) || (nDocType == COMMAND_WIN))
                && frMem.hDlgFindNextWnd == 0
                && frMem.hDlgConfirmWnd == 0;
        }
        break;

    case IDM_EDIT_REPLACE:
        if (lpdocrec == NULL) {
            bEnabled = FALSE;
        } else {
            bEnabled = bNormalWin
                && (nDocType == DOC_WIN) && (!lpdocrec->readOnly)
                && frMem.hDlgFindNextWnd == 0
                && frMem.hDlgConfirmWnd == 0;
        }
        break;

    case IDM_EDIT_GOTO_ADDRESS:
        bEnabled = DebuggeeActive();
        break;

    case IDM_EDIT_GOTO_LINE:
        if (lpdocrec == NULL) {
            bEnabled = FALSE;
        } else {
            bEnabled = bNormalWin                   // Not iconized
                && (DOC_WIN == nDocType);  // We have a text file
        }
        break;

    case IDM_EDIT_PROPERTIES:
        bEnabled = (CALLS_WIN == nDocType) ||
                   (DebuggeeActive() &&
                    (WATCH_WIN == nDocType ||
                     LOCALS_WIN == nDocType ||
                     MEMORY_WIN == nDocType));
        SendMessageNZ(GetHwnd_Toolbar(),
                      TB_ENABLEBUTTON,
                      MenuID,
                      MAKELONG(bEnabled, 0));
        break;

    case IDM_VIEW_TOGGLETAG:
    case IDM_VIEW_NEXTTAG:
    case IDM_VIEW_PREVIOUSTAG:
    case IDM_VIEW_CLEARALLTAGS:
        bEnabled = (lpdocrec != NULL)               // We have a document
            && bNormalWin                           // Not iconized
            && (DOC_WIN == nDocType);      // We have a text file
        break;

    case IDM_DEBUG_GO:
        if (DebuggeeActive()) {
            bEnabled = GoOK(LppdCur, LptdCur);
        } else {
            bEnabled = (GetLppdHead() == (LPPD)0);
        }
        SendMessageNZ(GetHwnd_Toolbar(),
                      TB_ENABLEBUTTON,
                      MenuID,
                      MAKELONG(bEnabled, 0));
        break;

    case IDM_DEBUG_RESTART:
        bEnabled = !IsProcRunning( LppdCur ) &&
                   !(g_contWorkspace_WkSp.m_bKernelDebugger);
        SendMessageNZ(GetHwnd_Toolbar(),
                      TB_ENABLEBUTTON,
                      MenuID,
                      MAKELONG(bEnabled, 0));
        break;

    case IDM_DEBUG_STOPDEBUGGING:
        if (g_contWorkspace_WkSp.m_bKernelDebugger) {
            bEnabled = FALSE;
        } else {
            bEnabled = DebuggeeAlive();
        }
        SendMessageNZ(GetHwnd_Toolbar(),
                      TB_ENABLEBUTTON,
                      MenuID,
                      MAKELONG(bEnabled, 0));
        break;

    case IDM_DEBUG_BREAK:
        bEnabled = DebuggeeActive() && IsProcRunning(LppdCur);
        SendMessageNZ(GetHwnd_Toolbar(),
                      TB_ENABLEBUTTON,
                      MenuID,
                      MAKELONG(bEnabled, 0));
        break;

    case IDM_DEBUG_STEPINTO:
    case IDM_DEBUG_STEPOVER:
        if (DebuggeeActive()) {
            bEnabled = StepOK(LppdCur, LptdCur);
        } else {
            bEnabled = (GetLppdHead() == (LPPD)0);
        }
        SendMessageNZ(GetHwnd_Toolbar(),
                      TB_ENABLEBUTTON,
                      MenuID,
                      MAKELONG(bEnabled, 0));
        break;

    case IDM_DEBUG_RUNTOCURSOR:
        //
        // In addition, for IDM_RUN_CONTINUE_TO_CURSOR, caret must be in a
        // document or the disassembler window.
        //
        if (!DebuggeeAlive()) {              // not started?
            bEnabled = (DOC_WIN == nDocType);
        } else if (DOC_WIN == nDocType        // src or asm win?
                  || DISASM_WIN == nDocType
                  || CALLS_WIN == nDocType) {
            bEnabled = GoOK(LppdCur, LptdCur);
        } else {                                    // other.
            bEnabled = FALSE;
        }
        SendMessageNZ(GetHwnd_Toolbar(),
                      TB_ENABLEBUTTON,
                      MenuID,
                      MAKELONG(bEnabled, 0));
        break;

    case IDM_DEBUG_QUICKWATCH:
        bEnabled = DebuggeeActive() && !IsProcRunning(LppdCur);
        SendMessageNZ(GetHwnd_Toolbar(),
                      TB_ENABLEBUTTON,
                      MenuID,
                      MAKELONG(bEnabled, 0));
        break;

    case IDM_DEBUG_SET_THREAD:
    case IDM_DEBUG_SET_PROCESS:
        bEnabled = DebuggeeActive();
        break;

    case IDM_DEBUG_ATTACH:
        if (OsVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
            if (g_contWorkspace_WkSp.m_bKernelDebugger) {
                bEnabled = FALSE;
            } else {
                bEnabled = TRUE;
            }
        }
        break;

    case IDM_DEBUG_GO_HANDLED:
    case IDM_DEBUG_GO_UNHANDLED:
        if (DebuggeeActive()) {
            bEnabled = GoExceptOK(LppdCur, LptdCur);
        }
        break;

    case IDM_WINDOW_NEWWINDOW:
        bEnabled = (lpdocrec != NULL)           // We have a doc
            && hwndActiveEdit                   // Currently active
            && nDocType == DOC_WIN;    // Text file
        break;

    case IDM_WINDOW_CASCADE:
    case IDM_WINDOW_TILE_HORZ:
    case IDM_WINDOW_TILE_VERT:
    case IDM_WINDOW_ARRANGE:
        bEnabled = hwndActiveEdit != NULL;
        break;

    case IDM_WINDOW_ARRANGE_ICONS:
        if ((hwnd = GetCpuHWND()) && IsIconic (hwnd)) {
            bEnabled = TRUE;
        } else if ((hwnd = GetFloatHWND()) && IsIconic (hwnd)) {
            bEnabled = TRUE;
        } else if ((hwnd = GetLocalHWND()) && IsIconic (hwnd)) {
            bEnabled = TRUE;
        } else if ((hwnd = GetWatchHWND()) && IsIconic (hwnd)) {
            bEnabled = TRUE;
        } else {
            for (i = 0; i < MAX_VIEWS; i++) {
                if (Views[i].hwndClient &&
                    IsIconic(GetParent(Views[i].hwndClient))) {
                    bEnabled = TRUE;
                    break;
                }
            }
        }
        break;

    default:
        // We should have handled everything.
        Assert(0);
        break;
    }

    return (( bEnabled ) ? MF_ENABLED : MF_GRAYED ) | MF_BYCOMMAND;
}

#endif //NEW_WINDOWING_CODE 



UINT
GetPopUpMenuID(
    IN HMENU hMenu
    )

/*++

Routine Description:

    Map the supplied menu handle to a menu ID.

Arguments:

    hMenu - Supplies a handle to a pop-up menu.

Return Value:

    UINT - Returns the menu ID corresponsing to the supplied menu handle.

--*/

{
    INT     i;
    INT     menus;

    DAssert( hMainMenu != NULL );

    //
    // Loop through the menu bar for each pop-up menu.
    //

    menus = GetActualMenuCount( );
    DAssert( menus != -1 );

    for( i = 0; i < menus; i++ ) {

        //
        // If the current pop-up is the supplied pop-up, return its ID.
        //

        if( hMenu == GetSubMenu( hMainMenu, i )) {
            return ((( i + 1 ) * IDM_BASE ) | MENU_SIGNATURE );
        }
    }

    //
    // The supplied menu handle wasn't found, assume it was actually a
    // menu ID.
    //

    return HandleToUlong( hMenu );
}


VOID
InitializeMenu(
    IN HMENU hMenu
    )

/*++

Routine Description:

    InitializeMenu sets the enabled/disabled state of all menu items whose
    state musr be determined dynamically.

Arguments:

    hMenu - Supplies a handle to the menu bar.

Return Value:

    None.

--*/

{
    INT     i;
    UINT    checked;

    Dbg(hMenu);

    //
    // Iterate thrrough the table, enabling/disabling menu items
    // as appropriate.
    //

    for( i = 0; i < ELEMENTS_IN_ENABLE_MENU_ITEM_TABLE; i++ ) {

        EnableMenuItem(
            hMenu,
            EnableMenuItemTable[ i ],
            CommandIdEnabled( EnableMenuItemTable[ i ])
            );
    }
}


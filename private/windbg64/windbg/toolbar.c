/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Toolbar.c

Abstract:

    This module contains the support code for toolbar

Author:

    Carlos Klapp (a-caklap) 16-Sep-1997

Environment:

    Win32, User Mode

--*/


#include "precomp.h"
#pragma hdrstop


HWND    hwndToolbar;          //Handle to main toolbar window


// toolbar constants
#define NUM_BMPS_IN_TOOLBAR         27

// See docs for TBBUTTON
TBBUTTON tbButtons[] =
{
    { 0,    0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0},
    { 0,    IDM_FILE_OPEN,              TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 0,    0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0},
    { 1,    IDM_FILE_SAVE_WORKSPACE,    TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 0,    0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0},
    { 2,    IDM_EDIT_CUT,               TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 3,    IDM_EDIT_COPY,              TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 4,    IDM_EDIT_PASTE,             TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 0,    0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0},
    { 5,    IDM_DEBUG_GO,               TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 6,    IDM_DEBUG_RESTART,          TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 7,    IDM_DEBUG_STOPDEBUGGING,    TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 8,    IDM_DEBUG_BREAK,            TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 0,    0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0},
    { 9,    IDM_DEBUG_STEPINTO,         TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 10,   IDM_DEBUG_STEPOVER,         TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 11,   IDM_DEBUG_RUNTOCURSOR,      TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 0,    0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0},
    { 12,   IDM_EDIT_TOGGLEBREAKPOINT,  TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 0,    0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0},
    { 13,   IDM_DEBUG_QUICKWATCH,       TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 0,    0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0},
    { 14,   IDM_VIEW_COMMAND,           TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 15,   IDM_VIEW_WATCH,             TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 16,   IDM_VIEW_LOCALS,            TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 17,   IDM_VIEW_REGISTERS,         TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 18,   IDM_VIEW_MEMORY,            TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 19,   IDM_VIEW_CALLSTACK,         TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 20,   IDM_VIEW_DISASM,            TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 21,   IDM_VIEW_FLOAT,             TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 0,    0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0},
    { 22,   IDM_DEBUG_SOURCE_MODE_ON,   TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0},
    { 23,   IDM_DEBUG_SOURCE_MODE_OFF,  TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0},
    { 0,    0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0},
    { 24,   IDM_EDIT_PROPERTIES,        TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 0,    0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0},
    { 25,   IDM_VIEW_FONT,              TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 0,    0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0},
    { 26,   IDM_VIEW_OPTIONS,           TBSTATE_ENABLED, TBSTYLE_BUTTON, 0}
};

// Used to retrieve the tooltip text
typedef struct {
    UINT    uCmdId;     // TBBUTOON command
    int     nStrId;    // String resource ID
} TB_STR_MAP;

// Map the command id to resource string identifier.
TB_STR_MAP tb_str_map[] = {
    { IDM_FILE_OPEN,                TBR_FILE_OPEN },
    { IDM_FILE_SAVE_WORKSPACE,      TBR_FILE_SAVE_WORKSPACE },
    { IDM_EDIT_CUT,                 TBR_EDIT_CUT },
    { IDM_EDIT_COPY,                TBR_EDIT_COPY },
    { IDM_EDIT_PASTE,               TBR_EDIT_PASTE },
    { IDM_DEBUG_GO,                 TBR_DEBUG_GO },
    { IDM_DEBUG_RESTART,            TBR_DEBUG_RESTART },
    { IDM_DEBUG_STOPDEBUGGING,      TBR_DEBUG_STOPDEBUGGING },
    { IDM_DEBUG_BREAK,              TBR_DEBUG_BREAK },
    { IDM_DEBUG_STEPINTO,           TBR_DEBUG_STEPINTO },
    { IDM_DEBUG_STEPOVER,           TBR_DEBUG_STEPOVER },
    { IDM_DEBUG_RUNTOCURSOR,        TBR_DEBUG_RUNTOCURSOR },
    { IDM_EDIT_TOGGLEBREAKPOINT,    TBR_EDIT_BREAKPOINTS },
    { IDM_DEBUG_QUICKWATCH,         TBR_DEBUG_QUICKWATCH },
    { IDM_VIEW_COMMAND,             TBR_VIEW_COMMAND },
    { IDM_VIEW_WATCH,               TBR_VIEW_WATCH },
    { IDM_VIEW_LOCALS,              TBR_VIEW_LOCALS },
    { IDM_VIEW_REGISTERS,           TBR_VIEW_REGISTERS },
    { IDM_VIEW_MEMORY,              TBR_VIEW_MEMORY },
    { IDM_VIEW_CALLSTACK,           TBR_VIEW_CALLSTACK },
    { IDM_VIEW_DISASM,              TBR_VIEW_DISASM },
    { IDM_VIEW_FLOAT,               TBR_VIEW_FLOAT },
    { IDM_DEBUG_SOURCE_MODE_ON,     TBR_DEBUG_SOURCE_MODE_ON },
    { IDM_DEBUG_SOURCE_MODE_OFF,    TBR_DEBUG_SOURCE_MODE_OFF },
    { IDM_EDIT_PROPERTIES,          TBR_EDIT_PROPERTIES },
    { IDM_VIEW_FONT,                TBR_VIEW_FONT },
    { IDM_VIEW_OPTIONS,             TBR_VIEW_OPTIONS }
};

#define NUM_TOOLBAR_BUTTONS (sizeof(tbButtons)/sizeof(TBBUTTON))
#define NUM_TOOLBAR_STRINGS (sizeof(tb_str_map)/sizeof(TB_STR_MAP))





LPSTR
GetToolTipTextFor_Toolbar(UINT uToolbarId)
/*++
Routine Description:
    Given the id of the toolbar button, we retrieve the
    corresponding tooltip text from the resources.

Arguments:
    uToolbarId - The command id for the toolbar button. This is the
        value contained in the WM_COMMAND msg.

Returns:
    Returns a pointer to a static buffer that contains the tooltip text.
--*/
{
    // Display tool tip text.
    static char sz[MAX_MSG_TXT];
    int nStrId = 0, i;
    
    // Get the str id given the cmd id
    for (i=0; i<NUM_TOOLBAR_STRINGS; i++) {
        if (tb_str_map[i].uCmdId == uToolbarId) {
            nStrId = tb_str_map[i].nStrId;
            break;
        }
    }
    Dbg(nStrId);
    
    // Now that we have the string id ....
    Dbg(LoadString(g_hInst, nStrId, sz, sizeof(sz) ));

    Dbg(strlen(sz) < sizeof(sz));

    return sz;
}



void
WindbgCreate_Toolbar(HWND hwndParent)
/*++
Routine Description:
    Creates the toolbar.

Arguments:
    hwndParent - The parent window of the toolbar.
--*/
{
    hwndToolbar = CreateToolbarEx(
        hwndParent,                 // parent
        WS_CHILD | WS_BORDER 
        | WS_VISIBLE 
        | TBSTYLE_TOOLTIPS 
        | TBSTYLE_WRAPABLE
        | CCS_TOP,                  // style
        ID_TOOLBAR,                 // toolbar id
        NUM_BMPS_IN_TOOLBAR,        // number of bitmaps
        g_hInst,                      // mod instance
        IDB_BMP_TOOLBAR,            // resource id for the bitmap
        tbButtons,                  // address of buttons
        NUM_TOOLBAR_BUTTONS,        // number of buttons
        16,15,                      // width & height of the buttons
        16,15,                      // width & height of the bitmaps
        sizeof(TBBUTTON));          // structure size

    Dbg(hwndToolbar);
}


void
Show_Toolbar(BOOL bShow)
/*++
Routine Description:
    Shows/hides the toolbar.

Arguments:
    bShow - TRUE - Show the toolbar.
            FALSE - Hide the toolbar.

            Autmatically resizes the MDI Client
--*/
{
    RECT rect;
    
    // Show/Hide the toolbar
    ShowWindow(hwndToolbar, bShow ? SW_SHOW : SW_HIDE);
    
    //Ask the frame to resize, so that everything will be correctly positioned.
    GetWindowRect(hwndFrame, &rect);
    
    EnableToolbarControls();
    
    SendMessage(hwndFrame, WM_SIZE, SIZE_RESTORED,
        MAKELPARAM(rect.right - rect.left, rect.bottom - rect.top));
    
    // Ask the MDIClient to redraw itself and its children.
    // This is  done in order to fix a redraw problem where some of the
    // MDIChild window are not correctly redrawn.
    Dbg(RedrawWindow(g_hwndMDIClient, NULL, NULL, 
        RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_FRAME));
}                                       /* UpdateToolbar() */


HWND
GetHwnd_Toolbar()
{
    return hwndToolbar;
}




/***    EnableToolbarControls
**
**  Description:
**      Enables/disables the controls in the toolbar according
**      to the current state of the system.
**
*/

void
EnableToolbarControls()
{
    InitializeMenu(GetMenu(hwndFrame));
}

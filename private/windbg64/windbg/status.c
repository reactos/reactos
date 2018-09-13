/*
 *
 */

#include "precomp.h"
#pragma hdrstop



// Adjust the size as necessary.
// Obviously, if an assert that checks for mem overwrites goes off,
// don't remove the assert, increase the char array size.
//           |
//          \|/
#define MAX_TEMP_TXT 100



//Status Bar : Structure definition
typedef struct _STATUS {
    HWND    hwndStatusBar;

    // The actual text to be displayed for each item
    LPSTR   rgszItemText[nMAX_IDX_STATUSBAR];

    // The line column text is in the following format: Ln 000, Col 000
    // Where "Ln" & "Col" are loaded from the resource and since they could be
    // language dependent. This is why we have to clutter the structure with
    // these 2 additional references.
    LPSTR   lpszLinePrefix;
    LPSTR   lpszColumnPrefix;

    // Prefix help the user figure out which is the process & thread displays
    // Proc 000:000
    // Thrd 000:000
    LPSTR   lpszProcessPrefix;
    LPSTR   lpszThreadPrefix;

    // Indicates whether the text should be grayed out when displayed.
    // TRUE - grayed out
    // FALSE - normal color
    BOOL    rgbGrayItemText[nMAX_IDX_STATUSBAR];

    // Indicates which ones are OWNER_DRAW. This is done
    //      so we can gray things out.
    // TRUE - Owner draw
    // FALSE - Normal, status bar takes care of the drawing.
    int     rgbOwnerDrawItem[nMAX_IDX_STATUSBAR];


    // TRUE - we are in src code mode
    // FALSE - we are in assembly mode
    BOOL    bSrcMode;

    int     nCurProcId;                              // Current Pid index
    int     nCurThreadId;                            // Current Tid index
    BOOL    bOverType;                               // Overtype status
    BOOL    bCapsLock;                               // CapsLock status
    BOOL    bNumLock;                                // NumLock status
} STATUS, * LPSTATUS;

static STATUS status;


///////////////////////////////////////////////////////////
// protos
void RecalcItemWidths_StatusBar(void);
void Internal_SetItemText_StatusBar(nIDX_STATUSBAR_ITEMS nId, LPSTR lpszNewText);


///////////////////////////////////////////////////////////
// Init/term functions
//
void
WindbgCreate_StatusBar(
                       HWND hwndParent
                       )
/*++
Routine Description:
    Creates and initializes the status bar.

Arguments:
    hwndParent - Hwnd to the owner of the status bar
--*/
{
    char sz[MAX_MSG_TXT];

    status.hwndStatusBar = CreateStatusWindow(
        WS_CHILD | WS_BORDER
        | WS_VISIBLE | CCS_BOTTOM,  // style
        "WinDbg",                   // Start up text. will soon be overwritten
        hwndParent,                 // parent
        IDC_STATUS_BAR);            // id

    Dbg(status.hwndStatusBar);

    //
    // We recalc the sizes even though we know they are 0, because,
    // the status bar needs to know how many parts there will be.
    //
    RecalcItemWidths_StatusBar();

    //
    // These are the owner draw items.
    //
    status.rgbOwnerDrawItem[nSRCASM_IDX_STATUSBAR] = TRUE;
    status.rgbOwnerDrawItem[nOVRTYPE_IDX_STATUSBAR] = TRUE;
    status.rgbOwnerDrawItem[nCAPSLCK_IDX_STATUSBAR] = TRUE;
    status.rgbOwnerDrawItem[nNUMLCK_IDX_STATUSBAR] = TRUE;

    //
    // Load the static stuff.
    //
    Dbg(LoadString(g_hInst, STS_MESSAGE_ASM, sz, sizeof(sz)));
    Internal_SetItemText_StatusBar(nSRCASM_IDX_STATUSBAR, sz);

    Dbg(LoadString(g_hInst, STS_MESSAGE_OVERTYPE, sz, sizeof(sz)));
    Internal_SetItemText_StatusBar(nOVRTYPE_IDX_STATUSBAR, sz);

    Dbg(LoadString(g_hInst, STS_MESSAGE_CAPSLOCK, sz, sizeof(sz)));
    Internal_SetItemText_StatusBar(nCAPSLCK_IDX_STATUSBAR, sz);

    Dbg(LoadString(g_hInst, STS_MESSAGE_NUMLOCK, sz, sizeof(sz)));
    Internal_SetItemText_StatusBar(nNUMLCK_IDX_STATUSBAR, sz);

    //
    // Preload prefixes
    //
    Dbg(LoadString(g_hInst, STS_MESSAGE_CURPROCID, sz, sizeof(sz)));
    status.lpszProcessPrefix = _strdup(sz);
    Dbg(status.lpszProcessPrefix);

    Dbg(LoadString(g_hInst, STS_MESSAGE_CURTHRDID, sz, sizeof(sz)));
    status.lpszThreadPrefix = _strdup(sz);
    Dbg(status.lpszThreadPrefix);

    Dbg(LoadString(g_hInst, STS_MESSAGE_LINE, sz, sizeof(sz)));
    status.lpszLinePrefix = _strdup(sz);
    Dbg(status.lpszLinePrefix);

    Dbg(LoadString(g_hInst, STS_MESSAGE_COLUMN, sz, sizeof(sz)));
    status.lpszColumnPrefix = _strdup(sz);
    Dbg(status.lpszColumnPrefix);

    //
    // Just temporary data, initialization routines elsewhere, will
    // initialize these properly. When I say temporary, I don't
    // mean for you to remove this.
    //
    Internal_SetItemText_StatusBar(nPROCID_IDX_STATUSBAR, "000:000");
    Internal_SetItemText_StatusBar(nTHRDID_IDX_STATUSBAR, "000:000");
    Internal_SetItemText_StatusBar(nSRCLIN_IDX_STATUSBAR, "0, 0");

    // Since all the new text has been added resize.
    RecalcItemWidths_StatusBar();

    // Almost forgot to init these
    SetCapsLock_StatusBar(GetKeyState(VK_CAPITAL) & 0x0001);
    SetNumLock_StatusBar(GetKeyState(VK_NUMLOCK) & 0x0001);
    SetOverType_StatusBar(FALSE);
}


void
Terminate_StatusBar()
/*++
Routine Description:
    Just frees allocated resources.
--*/
{
    int i;

    for (i = 0; i < nMAX_IDX_STATUSBAR -1; i++) {
        if (status.rgszItemText[i]) {
            free(status.rgszItemText[i]);
            status.rgszItemText[i] = NULL;
        }
    }

    if (status.lpszLinePrefix) {
        free(status.lpszLinePrefix);
        status.lpszLinePrefix = NULL;
    }

    if (status.lpszColumnPrefix) {
        free(status.lpszColumnPrefix);
        status.lpszColumnPrefix = NULL;
    }

    if (status.lpszProcessPrefix) {
        free(status.lpszProcessPrefix);
        status.lpszProcessPrefix = NULL;
    }

    if (status.lpszThreadPrefix) {
        free(status.lpszThreadPrefix);
        status.lpszThreadPrefix = NULL;
    }
}



///////////////////////////////////////////////////////////
// Operations that affect the entire status bar.
//
void
Show_StatusBar(
               BOOL bShow
               )
/*++
Routine Description:
    Show/Hide the status bar. Automatically resizes/updates the MDI client.

Arguments:
    bShow -     TRUE - Show status bar
                FALSE - Hide status bar
--*/
{
    RECT rect;

    // Show/Hide the toolbar
    ShowWindow(status.hwndStatusBar, bShow ? SW_SHOW : SW_HIDE);

    //Ask the frame to resize, so that everything will be correctly positioned.
    GetWindowRect(hwndFrame, &rect);

    SendMessage(hwndFrame, WM_SIZE, SIZE_RESTORED,
        MAKELPARAM(rect.right - rect.left, rect.bottom - rect.top));

    // Ask the MDIClient to redraw itself and its children.
    // This is  done in order to fix a redraw problem where some of the
    // MDIChild window are not correctly redrawn.
    Dbg(RedrawWindow(g_hwndMDIClient, NULL, NULL,
        RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_FRAME));
}                                       // UpdateToolbar()


void
WM_SIZE_StatusBar(
                  WPARAM wParam,
                  LPARAM lParam
                  )
/*++
Routine Description:
    Causes the status bar to be resized. This function is meant to be
    called from the parent window, whenever a parent window receives a
    WM_SIZE message, ie:

    // parent window proc
    switch (uMsg) {
    case WM_SIZE:
        WM_SIZE_StatusBar(wParam, lParam);
        return TRUE;
        ...
        ...
        ...
    }

Arguments:
    wParam & lParam - See docs for a desciption of the WM_SIZE message.

--*/
{
    // make the status bar resize.
    SendMessage(status.hwndStatusBar, WM_SIZE, wParam, lParam);

    // Since it was resized, the widths the text items need to be recalculated.
    // The is because  of the way that status bar positions the elements on the
    // screen. See the docs for SB_SETPARTS, for more detail. The SB_SETPARTS
    // docs will enlighten you.
    RecalcItemWidths_StatusBar();
}


HWND
GetHwnd_StatusBar()
// I'm not documenting this function, everyone can figure this one out.
{
    return status.hwndStatusBar;
}



///////////////////////////////////////////////////////////
// Main text display functions
//
void
RecalcItemWidths_StatusBar(void)
/*++
Routine description:
    The function will recalculate the width of the text items.
    The calculations don't have to be exact. Status bar is very
    forgiving and pretty much needs a rough estimate.
--*/
{
    int rgnItemWidths[nMAX_IDX_STATUSBAR];
    int i, nWidth;
    HDC hdc;

    hdc = GetDC(status.hwndStatusBar);
    Dbg(hdc);

    // Get the width of the status bar's client area.
    {
        RECT rcClient;
        GetClientRect(status.hwndStatusBar, &rcClient);
        nWidth = rcClient.right;
    }

    // Calculate the right edge coordinate for each part, and
    // copy the coordinates to the array.
    for (i = nMAX_IDX_STATUSBAR -1; i >= 0; i--) {

        rgnItemWidths[i] = nWidth;

        if (NULL == status.rgszItemText[i]) {
            // We don't have any text, but we need a position anyways.
            nWidth -= 10; // Just any old number
        } else {
            LPSTR lpsz = status.rgszItemText[i];
            SIZE size;

            // Skip over tabs.
            // 1 tab is centered, 2 is right aligned.
            // See status bar docs for more info.
            if ('\t' == *lpsz) {
                lpsz++;
                if ('\t' == *lpsz) {
                    lpsz++;
                }
            }

            Dbg(GetTextExtentPoint32(hdc, lpsz, strlen(lpsz), &size));

            nWidth -= size.cx;
        }
    }

    Dbg(ReleaseDC(status.hwndStatusBar, hdc));

    // Tell the status window to create the window parts.
    Dbg(SendMessage(status.hwndStatusBar, SB_SETPARTS,
        (WPARAM) nMAX_IDX_STATUSBAR, (LPARAM) rgnItemWidths));

    // The status bar invalidates the parts that changed. So it is
    // automatically updated.
}


void
Internal_SetItemText_StatusBar(
                    nIDX_STATUSBAR_ITEMS nId,
                    LPSTR lpszNewText
                    )
/*++
Routine Description:
    Set the text for a specified item.
--*/
{
    // Leave these sanity checks in here.
    // If they go off, someone did something wrong
    // or changed some important code
    Dbg(0 <= nId);
    Dbg(nId < nMAX_IDX_STATUSBAR);
    Dbg(lpszNewText);

    // Free any previous text
    if (status.rgszItemText[nId]) {
        free(status.rgszItemText[nId]);
    }

    // duplicate the text
    status.rgszItemText[nId] = _strdup(lpszNewText);

    // Make sure it was allocated
    Assert(status.rgszItemText[nId]);

    // Do we have any text to set?
    if (status.rgszItemText[nId]) {
        int nFormat = nId;

        // Make it owner draw???
        if (status.rgbOwnerDrawItem[nId]) {
            nFormat |= SBT_OWNERDRAW;
        }

        // Set the text
        Dbg(SendMessage(status.hwndStatusBar, SB_SETTEXT,
            (WPARAM) nFormat, (LPARAM) status.rgszItemText[nId]));
    }
}


void
InvalidateItem_Statusbar(nIDX_STATUSBAR_ITEMS nIdx)
/*++
Routine description:
    Invalidates the item's rect on the status bar, so that an update
    to that region will take place.

Arguments:
    nIdx - The status bar item that is to be updated.
--*/
{
    RECT rc;

    Dbg(0 <= nIdx);
    Dbg(nIdx < nMAX_IDX_STATUSBAR);

    SendMessage(status.hwndStatusBar, SB_GETRECT,
        (WPARAM) nIdx, (LPARAM) &rc);

    InvalidateRect(status.hwndStatusBar, &rc, FALSE);
}


void
OwnerDrawItem_StatusBar(
                        LPDRAWITEMSTRUCT lpDrawItem
                        )
/*++
Routine Description:
    Called from the parent window for owner draw text items.
    Draws an actual status bar item onto the status bar.
    Depending the the flags set, it will draw the item grayed out.

Arguments:
    See docs for WM_DRAWITEM, and Status bar -> owner draw items.
--*/
{
    LPSTR lpszItemText = (LPSTR) lpDrawItem->itemData;
    COLORREF crefOldTextColor = CLR_INVALID;
    COLORREF crefOldBkColor = CLR_INVALID;

    if (NULL == lpszItemText) {
        // nothing to do
        return;
    }

    // Set background color and save the old color.
    crefOldBkColor = SetBkColor(lpDrawItem->hDC, GetSysColor(COLOR_3DFACE));
    Assert(CLR_INVALID != crefOldBkColor);

    // should the item be grayed out?
    if (status.rgbGrayItemText[lpDrawItem->itemID]) {
        crefOldTextColor = SetTextColor(lpDrawItem->hDC, GetSysColor(COLOR_GRAYTEXT));
        Assert(CLR_INVALID != crefOldTextColor);
    }

    // draw the color coded text to the screen
    {
        UINT uFormat = DT_NOPREFIX | DT_VCENTER | DT_SINGLELINE;

        // "\t" is used to center
        // '\t\t" is used to right align.
        // No, I did not make this up, this is the way the status bar works.
        if ('\t' == *lpszItemText) {
            lpszItemText++;
            if ('\t' == *lpszItemText) {
                // 2 tabs found
                lpszItemText++;
                uFormat |= DT_RIGHT;
            } else {
                // 1 tab found
                uFormat |= DT_CENTER;
            }
        }
        DrawText(lpDrawItem->hDC, lpszItemText, strlen(lpszItemText),
            &lpDrawItem->rcItem, uFormat);
    }

    // Reset the the hDC back to its old state.
    if (CLR_INVALID != crefOldTextColor) {
        Dbg(CLR_INVALID != SetTextColor(lpDrawItem->hDC, crefOldTextColor));
    }

    if (CLR_INVALID != crefOldBkColor) {
        Dbg(CLR_INVALID != SetBkColor(lpDrawItem->hDC, crefOldBkColor));
    }
}


void
SetItemText_StatusBar(
                    nIDX_STATUSBAR_ITEMS nId,
                    LPSTR lpszNewText
                    )
/*++
Routine Description:

Arguments:
    nId         -
    lpszNewText
--*/
{
    Internal_SetItemText_StatusBar(nId, lpszNewText);
    // If nId is 0, the we don't have to recalc the widths, because this
    // is the only one that doesn't affect the rest.
    if (nId > 0) {
        RecalcItemWidths_StatusBar();
    }
}


// Old code, you figure it out. It didn't want to. -kcarlos
void
CDECL
SetMessageText_StatusBar(
                         int nNewTextId,
                         WORD wMsgType,
                         ...
                         )
/*++
Routine Description:
    Set the main text item on a status bar.

Arguments:
    nNewTextId - Resource id or menu id.
    wMsgType - Indicates whether it is a menu or string resource id
--*/
{
#define BETWEEN(inf, sup) (nNewTextId >= inf && nNewTextId <= sup)

    char szStatusFormat[MAX_MSG_TXT];
    char szStatusText[MAX_VAR_MSG_TXT]; // size is as big as considered necessary
    va_list vargs;

    //Load status text format

    if (wMsgType == STATUS_MENUTEXT) {

        int menuAdjust, j = 0;

        nNewTextId = (int) GetPopUpMenuID(( HMENU ) nNewTextId );


        // See if it's a MRU file item
        if (IDM_FILE_MRU_FILE1 <= nNewTextId && nNewTextId <= IDM_FILE_MRU_FILE16) {
            Dbg(LoadString(g_hInst, STA_Open_MRU_File, (LPSTR)szStatusFormat, MAX_MSG_TXT));
            goto show;
        }

        // See if it's a MRU workspace item
        if (IDM_FILE_MRU_WORKSPACE1 <= nNewTextId && nNewTextId <= IDM_FILE_MRU_WORKSPACE16) {
            Dbg(LoadString(g_hInst, STA_Open_MRU_Project, (LPSTR)szStatusFormat, MAX_MSG_TXT));
            goto show;
        }

        if (BETWEEN(IDM_FILE_FIRST, IDM_FILE_LAST)
            || BETWEEN(IDM_EDIT_FIRST, IDM_EDIT_LAST)
            || BETWEEN(IDM_VIEW_FIRST, IDM_VIEW_LAST)
            || BETWEEN(IDM_DEBUG_FIRST, IDM_DEBUG_LAST)
            || BETWEEN(IDM_WINDOW_FIRST, IDM_WINDOW_LAST)
            || BETWEEN(IDM_HELP_FIRST, IDM_HELP_LAST)) {

            if(!LoadString(g_hInst,
                nNewTextId, (LPSTR)szStatusFormat, MAX_MSG_TXT)) {

                wsprintf(szStatusFormat,
                    "LoadString failed in %s, %ld: hInst == %ld, Id == %ld\r\n",
                    __FILE__, (LONG)__LINE__, HandleToLong(g_hInst), (LONG)nNewTextId);

                OutputDebugString(szStatusFormat);

                lstrcpy(szStatusFormat, "--Bogus Bogus--");
            }

            goto show;
        }

        //See if we have a maximized Mdi Window
        //(A system menu will be added) to standard menu bar

        if (GetMenuItemCount(hMainMenu) > NUMBER_OF_MENUS) {
            menuAdjust = 1;
        } else {
            menuAdjust = 0;
        }

        //See if it's a MDI window item

        if (nNewTextId >= IDM_WINDOWCHILD
            && nNewTextId < IDM_WINDOWCHILD + MAX_DOCUMENTS) {

            PSTR pStr;

            Dbg(LoadString(g_hInst, STA_Open_MDI_Window, (LPSTR)szStatusFormat, MAX_MSG_TXT));
            if (GetMenuString(hWindowSubMenu, nNewTextId, (LPSTR)szTmp,
                MAX_MSG_TXT, MF_BYCOMMAND) > 2) {

                //Search and get rid of accelerator

                Dbg((pStr = (PSTR) strchr( (PUCHAR) szTmp, '\t' - 1)) != NULL);
                *pStr = '\0';
                UnescapeAmpersands(szTmp + 2, sizeof(szTmp)-2);
                strcat(szStatusFormat, szTmp + 2);
            }
            goto show;
        }

        //It's system menu or unknown, Clear status Bar

        Dbg(LoadString(g_hInst, SYS_StatusClear, (LPSTR)szStatusFormat, MAX_MSG_TXT));
    } else {
        //Load Info text or Error Message
        Dbg(LoadString(g_hInst, nNewTextId, (LPSTR)szStatusFormat, MAX_MSG_TXT));
    }


show:

    //Build the status text with the var parameters

    va_start(vargs, wMsgType);
    vsprintf(szStatusText, szStatusFormat, vargs);
    va_end(vargs);

    SetItemText_StatusBar(nMESSAGE_IDX_STATUSBAR, szStatusText);
}


///////////////////////////////////////////////////////////
// Set/get specialized items on the status bar.
//
// All of the Get????_StatusBar retrieve the current value.
//
// All of the Set????_StatusBar set the new value and return the
//   previous value.
//      TRUE - Item is enabled.
//      FALSE - Item is disabled.

//
// Src/Asm mode
BOOL
GetSrcMode_StatusBar()
{
    return status.bSrcMode;
}

BOOL
SetSrcMode_StatusBar(
                     BOOL bNewValue
                     )
{
    BOOL b = status.bSrcMode;

    status.bSrcMode = bNewValue;
    status.rgbGrayItemText[nSRCASM_IDX_STATUSBAR] = bNewValue;

    InvalidateItem_Statusbar(nSRCASM_IDX_STATUSBAR);

    // Reflect the change to the menu
    InitializeMenu(GetMenu(hwndFrame));

    // Old code that was move in here.
    if ((FALSE == bNewValue) && (disasmView == -1)) {
        OpenDebugWindow(DISASM_WIN, TRUE); // User activated
    }

    return b;
}


//
// Insert/Overtype mode
BOOL
GetOverType_StatusBar()
{
    return status.bOverType;
}


BOOL
SetOverType_StatusBar(BOOL bNewValue)
{
    BOOL b = status.bOverType;

    status.bOverType = bNewValue;
    status.rgbGrayItemText[nOVRTYPE_IDX_STATUSBAR] = !bNewValue;

    InvalidateItem_Statusbar(nOVRTYPE_IDX_STATUSBAR);

    return b;
}


//
// Num lock mode
BOOL
GetNumLock_StatusBar()
{
    return status.bNumLock;
}

BOOL
SetNumLock_StatusBar(BOOL bNewValue)
{
    BOOL b = status.bNumLock;

    status.bNumLock = bNewValue;
    status.rgbGrayItemText[nNUMLCK_IDX_STATUSBAR] = !bNewValue;

    InvalidateItem_Statusbar(nNUMLCK_IDX_STATUSBAR);

    return b;
}


//
// Caps mode
BOOL
GetCapsLock_StatusBar()
{
    return status.bCapsLock;
}

BOOL
SetCapsLock_StatusBar(BOOL bNewValue)
{
    BOOL b = status.bCapsLock;

    status.bCapsLock = bNewValue;
    status.rgbGrayItemText[nCAPSLCK_IDX_STATUSBAR] = !bNewValue;

    InvalidateItem_Statusbar(nCAPSLCK_IDX_STATUSBAR);

    return b;
}


///////////////////////////////////////////////////////////
// Specialized text display functions
void
SetLineColumn_StatusBar(
                        int nNewLine,
                        int nNewColumn
                        )
/*++
Routine Description:
    Used to display the line and column values in text edit controls.
    Loads the prefixs "Ln" & "Col" from the string resource section.

Arguments:
    nNewLine - Line number in edit controls.
    nNewColumn - Column number in edit controls.
--*/
{
    char sz[MAX_TEMP_TXT];

    sprintf(sz, "%s %d, %s %d", status.lpszLinePrefix, nNewLine,
        status.lpszColumnPrefix, nNewColumn);

    Dbg(strlen(sz) < sizeof(sz));

    SetItemText_StatusBar(nSRCLIN_IDX_STATUSBAR, sz);
}


void
SetPidTid_StatusBar(
                    const LPPD lppd,
                    const LPTD lptd
                    )
/*++

Routine Description:

    Display the Process ID and Task ID in the status bar.

    Display format:

        Internal Process number:OS Process ID           Internal Task number:OS Task ID
        000:000                                         000:000

    If the OS ID cannot be retrieved:
        000:???                                         000:???

    If the OS ID is greater than 3 digits, then it is displayed in hex
        000:0xFFFFFFFF                                  000:0xFFFFFFFF

Arguments:

    lppd

    lptd

        Are not modified, but simple used to access the
        process ID (lppd->ipid) and process handle (lppd->hpid),
        task ID (lptd->itid) and task handle (lptd->htid).

--*/
{
    int ipid, itid;


    if (lppd) {
        ipid = lppd->ipid;
    } else {
        ipid = -1;
    }

    if (lptd) {
        itid = lptd->itid;
    } else {
        itid = -1;
    }

    if (ipid != status.nCurProcId) {
        char sz[MAX_TEMP_TXT];

        status.nCurProcId = ipid;

        if (ipid == -1) {
            sprintf(sz, "%s ???:???", status.lpszProcessPrefix);
        } else {
            PST pst;

            if (OSDGetProcessStatus(lppd->hpid, &pst) != xosdNone) {
                // Add the internal process number.
                sprintf(sz, "%s %03d:???", status.lpszProcessPrefix, ipid);
            } else {
                // Add the internal process number & OS thread id
                UINT uOS_Id = atoi(pst.rgchProcessID);

                // Sanity check
                Dbg(strlen(pst.rgchProcessID) < sizeof(pst.rgchProcessID));

                // Testing against 1000, is to determine how many digits we must display.
                // Win98 uses very large IDs. If it is greater than 3 digits, display
                // it in hex.
                if (uOS_Id < 1000) {
                    sprintf(sz, "%s %03d:%03u", status.lpszProcessPrefix, ipid, uOS_Id);
                } else {
                    sprintf(sz, "%s %03d:0x%x", status.lpszProcessPrefix, ipid, uOS_Id);
                }
            }

        }

        // Sanity check, should never occur.
        // Mem overwrite?
        Dbg(strlen(sz) < sizeof(sz));

        SetItemText_StatusBar(nPROCID_IDX_STATUSBAR, sz);
    }

    if (itid != status.nCurThreadId) {
        char sz[MAX_TEMP_TXT];

        status.nCurThreadId = itid;

        if (itid == -1) {
            sprintf(sz, "%s ???:???", status.lpszThreadPrefix);
        } else {
            TST tst;

            if (OSDGetThreadStatus(lppd->hpid, lptd->htid, &tst) != xosdNone) {
                // Add the internal thread number.
                sprintf(sz, "%s %03d:???", status.lpszThreadPrefix, itid);
            } else {
                // Add the internal thread number & OS thread id
                UINT uOS_Id = atoi(tst.rgchThreadID);

                // Sanity check
                Dbg(strlen(tst.rgchThreadID) < sizeof(tst.rgchThreadID));

                // Testing against 1000, is to determine how many digits we must display.
                // Win98 uses very large IDs. If it is greater than 3 digits, display
                // it in hex.
                if (uOS_Id < 1000) {
                    sprintf(sz, "%s %03d:%03u", status.lpszThreadPrefix, itid, uOS_Id);
                } else {
                    sprintf(sz, "%s %03d:0x%x", status.lpszThreadPrefix, itid, uOS_Id);
                }
            }
        }

        // Sanity check, should never occur.
        // Mem overwrite?
        Dbg(strlen(sz) < sizeof(sz));

        SetItemText_StatusBar(nTHRDID_IDX_STATUSBAR, sz);
    }
}



///////////////////////////////////////////////////////////
// Misc helper routines
//
/****************************************************************************

         FUNCTION: KeyboardHook

         PURPOSE: Check if keyboard hit is NUMLOCK, CAPSLOCK or INSERT

****************************************************************************/
LRESULT EXPORT
KeyboardHook( int iCode, WPARAM wParam, LPARAM lParam )
{
    if (iCode == HC_ACTION) {
        if (wParam == VK_NUMLOCK
            && HIWORD(lParam) & 0x8000 // Key up
            && GetKeyState(VK_CONTROL) >= 0) { //No Ctrl

            // CAPSLOCK has been hit, refresh status
            SetNumLock_StatusBar(GetKeyState(VK_NUMLOCK) & 0x0001);

        } else if (wParam == VK_CAPITAL
            && HIWORD(lParam) & 0x8000 //Key up
            && GetKeyState(VK_CONTROL) >= 0) { //No Ctrl

            // CAPSLOCK has been hit, refresh status
            SetCapsLock_StatusBar(GetKeyState(VK_CAPITAL) & 0x0001);

        } else if (wParam == VK_INSERT
            && ((HIWORD(lParam) & 0xE000) == 0x0000) //Key down was up before and No Alt
            && GetKeyState(VK_SHIFT) >= 0   //No Shift
            && GetKeyState(VK_CONTROL) >= 0) { //No Ctrl

            // INSERT has been hit and refresh status if so
            // We can't use the up down state, since there is no indicator light
            // as a referene to the user. We simple have to toggle it.
            SetOverType_StatusBar(!GetOverType_StatusBar());
        }
    }

    return CallNextHookEx( hKeyHook, iCode, wParam, lParam );
}                                       /* KeyboardHook() */




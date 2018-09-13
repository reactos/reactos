#include "cabinet.h"
#include "rcids.h"
#include <trayp.h>
#include "taskband.h"
#include "bandsite.h"
#include "mmhelper.h" // Multimonitor helper functions
#include "apithk.h"

extern TRAYSTUFF g_ts;

void RunSystemMonitor(void);

#define HSHELL_OVERFLOW (WM_USER+42)        // BUGBUG - BobDay move to WINUSER.W

#ifdef WINNT
#define USE_SYSMENU_TIMEOUT
#endif

typedef struct {
    TC_ITEM tcitem;
    DWORD   dwFlags;
} TASKTABITEM, * PTASKTABITEM;

#define RECTWIDTH(rc)   ((rc).right-(rc).left)
#define RECTHEIGHT(rc)  ((rc).bottom-(rc).top)
#define ResizeWindow(hwnd, cWidth, cHeight) \
    SetWindowPos(hwnd, 0, 0, 0, cWidth, cHeight, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER)

#define IDC_FOLDERTABS             1

#define TIF_RENDERFLASHED       0x000000001
#define TIF_SHOULDTIP           0x000000002
#define TIF_ACTIVATEALT         0x000000004
#define TIF_EVERACTIVEALT       0x000000008
#define TIF_FLASHING            0x000000010

HFONT g_hfontCapBold = NULL;
HFONT g_hfontCapNormal = NULL;

void SetWindowStyleBit(HWND hWnd, DWORD dwBit, DWORD dwValue);

CTasks g_tasks = {0};

#define IDT_SYSMENU 2
#define IDT_RECHECKRUDEAPP 3

#define TIMEOUT_SYSMENU         2000
#define TIMEOUT_SYSMENU_HUNG    125

void UpdateButtonSize(PTasks ptasks);
BOOL Task_SwitchToSelection(PTasks ptasks);
LRESULT Task_HandleWinIniChange(PTasks ptasks, WPARAM wParam, LPARAM lParam, BOOL fDeferBandsiteChange);
void Task_VerifyButtonHeight(PTasks ptasks);
void Cabinet_InitGlobalMetrics(WPARAM, LPTSTR);
void SHAllowSetForegroundWindow(HWND hwnd);

DWORD Tray_GetStuckPlace();
void Tray_HandleFullScreenApp(HWND hwnd);
BOOL ShouldMinMax(HWND hwnd, BOOL fMin);
void LowerDesktop();
void Tray_Unhide();
void RedrawItem(PTasks ptasks, HWND hwndShell, WPARAM code );
void Task_ScrollIntoView(PTasks ptasks);

HWND Task_FindRudeApp(PTasks ptasks);

BOOL Window_IsNormal(HWND hwnd)
{
    return (hwnd != v_hwndTray) && (hwnd != v_hwndDesktop) && IsWindow(hwnd);
}

//======================================================================
//
HWND TabCtrl_GetItemHwnd(HWND hwnd, int i)
{
    TASKTABITEM item;

    item.tcitem.lParam = 0;
    item.tcitem.mask = TCIF_PARAM;

    TabCtrl_GetItem(hwnd, i, (TC_ITEM *)&item);
    return (HWND)item.tcitem.lParam;
}

DWORD TabCtrl_GetItemFlags(HWND hwnd, int i)
{
    TASKTABITEM item;

    item.tcitem.mask = TCIF_PARAM;
    TabCtrl_GetItem(hwnd, i, (TC_ITEM *)&item);
    return item.dwFlags;
}

void TabCtrl_SetItemFlags(HWND hwnd, int i, DWORD dwFlags)
{
    TASKTABITEM item;

    item.tcitem.mask = TCIF_PARAM;
    if (TabCtrl_GetItem(hwnd, i, (TC_ITEM *)&item)) {
        item.dwFlags = dwFlags;
        TabCtrl_SetItem(hwnd, i, (TC_ITEM *)&item);
    }
}

int FindItem(PTasks ptasks, HWND hwnd)
{
    int iMax;
    int i;

    iMax = TabCtrl_GetItemCount(ptasks->hwndTab);
    for ( i = 0; i < iMax; i++)
    {
        HWND hwndTask = TabCtrl_GetItemHwnd(ptasks->hwndTab, i);
        if (hwndTask == hwnd) {
            return i;
        }
    }
    return -1;
}

void CheckNeedScrollbars(PTasks ptasks, int cyRow, int cItems, int iCols, int iRows,
                                     int iItemWidth, LPRECT lprcView)
{
    DWORD dwStuck = Tray_GetStuckPlace();
    SCROLLINFO si;
    RECT rcTabs;
    int cxRow = iItemWidth + g_cxTabSpace;
    int iVisibleColumns = ((RECTWIDTH(*lprcView) + g_cxTabSpace) / cxRow);
    int iVisibleRows = ((RECTHEIGHT(*lprcView) + g_cyTabSpace) / cyRow);
    int x,y, cx,cy;

    rcTabs = *lprcView;

    if (!iVisibleColumns)
        iVisibleColumns = 1;
    if (!iVisibleRows)
        iVisibleRows = 1;

    si.cbSize = SIZEOF(SCROLLINFO);
    si.fMask = SIF_PAGE | SIF_RANGE;
    si.nMin = 0;
    si.nPage = 0;
    si.nPos = 0;

    if (STUCK_HORIZONTAL(dwStuck)) {
        // do vertical scrollbar
        // -1 because it's 0 based.
        si.nMax = (cItems + iVisibleColumns - 1) / iVisibleColumns  -1 ;
        si.nPage = iVisibleRows;

        // we're actually going to need the scrollbars
        if (si.nPage <= (UINT)si.nMax) {
            // this effects the vis columns and therefore nMax and nPage
            rcTabs.right -= g_cxVScroll;
            iVisibleColumns = ((RECTWIDTH(rcTabs) + g_cxTabSpace) / cxRow);
            if (!iVisibleColumns)
                iVisibleColumns = 1;
            si.nMax = (cItems + iVisibleColumns - 1) / iVisibleColumns  -1 ;
        }

        SetScrollInfo(ptasks->hwnd, SB_VERT, &si, TRUE);
        si.fMask = SIF_POS | SIF_PAGE | SIF_RANGE;
        GetScrollInfo(ptasks->hwnd, SB_VERT, &si);
        x = 0;
        y = -si.nPos * cyRow;
        cx = cxRow * iVisibleColumns;
        // +1 because si.nMax is zero based
        cy = cyRow * (si.nMax +1);

        // nuke the other scroll bar
        si.nMax = 0;
        si.nPos = 0;
        si.nMin = 0;
        si.nPage = 0;
        SetScrollInfo(ptasks->hwnd, SB_HORZ, &si, TRUE);

    } else {
        // do horz scrollbar
        si.nMax = iCols -1;
        si.nPage = iVisibleColumns;

        // we're actually going to need the scrollbars
        if (si.nPage <= (UINT)si.nMax) {
            // this effects the vis columns and therefore nMax and nPage
            rcTabs.bottom -= g_cyHScroll;
            iVisibleRows = ((RECTHEIGHT(rcTabs) + g_cyTabSpace) / cyRow);
            if (!iVisibleRows)
                iVisibleRows = 1;
            si.nMax = (cItems + iVisibleRows - 1) / iVisibleRows  -1 ;
        }

        SetScrollInfo(ptasks->hwnd, SB_HORZ, &si, TRUE);
        si.fMask = SIF_POS | SIF_PAGE | SIF_RANGE;
        GetScrollInfo(ptasks->hwnd, SB_HORZ, &si);
        y = 0;
        x = -si.nPos * cxRow;

        cx = cxRow * (si.nMax + 1);
        cy = cyRow * iVisibleRows;

        // nuke the other scroll bar
        si.nMax = 0;
        si.nPos = 0;
        si.nMin = 0;
        si.nPage = 0;
        SetScrollInfo(ptasks->hwnd, SB_VERT, &si, TRUE);
    }
    SetWindowPos(ptasks->hwndTab, 0, x,y, cx, cy, SWP_NOACTIVATE| SWP_NOZORDER);
}

void NukeScrollbars(PTasks ptasks)
{
    SCROLLINFO si;
    si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
    si.cbSize = SIZEOF(SCROLLINFO);
    si.nMin = 0;
    si.nMax = 0;
    si.nPage = 0;
    si.nPos = 0;

    SetScrollInfo(ptasks->hwnd, SB_VERT, &si, TRUE);
    SetScrollInfo(ptasks->hwnd, SB_HORZ, &si, TRUE);
}

//======================================================================
// this checks the size of the tab's items.  it shrinks them to
// make sure that all are visible on the window.  min size is smallicon size
// plus 3*xedge.  max size is small icons size.
void CheckSize(PTasks ptasks, BOOL fForceResize)
{
    RECT rc;
    int iWinWidth;
    int iWinHeight;
    int cItems;
    int iIdeal;
    int cyRow;
    int iMax, iMin;
    int iRows;
    int iOldWidth;
    RECT rcItem;

    GetWindowRect(ptasks->hwnd, &rc);
    if (IsRectEmpty(&rc) || !(GetWindowLong(ptasks->hwndTab, GWL_STYLE) & WS_VISIBLE))
        return;

    iWinWidth = RECTWIDTH(rc);
    iWinHeight = RECTHEIGHT(rc);
    cItems = TabCtrl_GetItemCount(ptasks->hwndTab);
    TabCtrl_GetItemRect(ptasks->hwndTab, 0, &rcItem);
    iOldWidth = RECTWIDTH(rcItem);

    // we need to add the iButtonSpace on the nominator because there are n-1 spaces.
    // we need to add the iButtonSpace in the denominator because that's the full height
    // of a row
    cyRow = RECTHEIGHT(rcItem) + g_cyTabSpace;
    iRows = (iWinHeight + g_cyTabSpace) / cyRow;
    if (iRows == 0) iRows = 1;
    if (cItems) {
        DWORD dwStuck;
        // interbutton spacing by the tabs
        int iCols;

        // We need to round up so that iCols is the smallest number such that
        // iCols*iRows >= cItems
        iCols = (cItems + iRows - 1) / iRows;
        iIdeal = (iWinWidth / iCols) - g_cxTabSpace;
        dwStuck = Tray_GetStuckPlace();

        // now check if we want to bail..
        // bail if we're increasing the width, but not by very much
        //
        // use the ideal width for this calculation
        if (STUCK_HORIZONTAL(dwStuck) && !fForceResize && (iOldWidth < iIdeal) && ((iOldWidth / (iIdeal - iOldWidth)) >= 3)) {
            return;
        }

        if (STUCK_HORIZONTAL(dwStuck))
            iMax = g_cxMinimized;
        else
            iMax = iWinWidth;

        iMin = g_cySize + 2*g_cxEdge;
        iIdeal = min(iMax, iIdeal);
        iIdeal = max(iMin, iIdeal);
        TabCtrl_SetItemSize(ptasks->hwndTab, iIdeal, g_cySize + 2 * g_cyEdge);

        // if we're forced to the minimum size, then we may need some scrollbars
        if (iIdeal == iMin) {
            CheckNeedScrollbars(ptasks, cyRow, cItems, iCols, iRows, iIdeal, &rc);
        } else {
            NukeScrollbars(ptasks);
            SetWindowPos(ptasks->hwndTab, 0, 0, 0,iWinWidth, iWinHeight, SWP_NOACTIVATE | SWP_NOZORDER);
        }
    } else {
        TabCtrl_SetItemSize(ptasks->hwndTab, g_cxMinimized, g_cySize + 2 * g_cyEdge);
    }
}

void Task_UpdateFlashingFlag(HWND hwnd)
{
    // Loop through the tab items, see if any have TIF_FLASHING
    // set, and update the global traystuff flashing flag.

    int cItems = TabCtrl_GetItemCount(hwnd);

    while (cItems--)
    {
        DWORD dwFlags = TabCtrl_GetItemFlags(hwnd, cItems);
        if (dwFlags & TIF_FLASHING)
        {
            g_ts.fFlashing = TRUE;
            return;
        }
    }

    g_ts.fFlashing = FALSE;
}

//---------------------------------------------------------------------------
// Delete an item from the listbox but resize the buttons if needed.
void Task_DeleteItem(PTasks ptasks, UINT i)
{
    //ImageList... delete icon
    TabCtrl_DeleteItem(ptasks->hwndTab, i); // delete first to avoid refresh

    // Update the global traystuff flag that says, "There is an item flashing."
    Task_UpdateFlashingFlag(ptasks->hwndTab);

    CheckSize(ptasks, FALSE);
}

void Task_RealityCheck(HWND hwndView)
{
    PTasks ptasks = GetWindowPtr(hwndView, 0);
    if (ptasks)
    {
        //
        // Delete any buttons corresponding to non-existent windows.
        //
        int iItem = TabCtrl_GetItemCount(ptasks->hwndTab);
        while (--iItem >= 0)
        {
            HWND hwnd = TabCtrl_GetItemHwnd(ptasks->hwndTab, iItem);
            if (!IsWindow(hwnd))
                Task_DeleteItem(ptasks, iItem);
        }
    }
}

//---------------------------------------------------------------------------
// Insert an item into the listbox but resize the buttons if required.
int Task_InsertItem(PTasks ptasks, HWND hwndTask)
{
    TASKTABITEM ti;
    ti.tcitem.mask = TCIF_PARAM;
    ti.tcitem.lParam = (LPARAM)hwndTask;
    ti.dwFlags = 0L;

    TabCtrl_InsertItem(ptasks->hwndTab, 0x7FFF, (TC_ITEM*)&ti);

    CheckSize(ptasks, FALSE);

    return TRUE;
}

//---------------------------------------------------------------------------
// Adds the given window to the task list.
// Returns it's position in the list or -1 of there's a problem.
// NB No check is made to see if it's already in the list.
int Task_AddWindow(PTasks ptasks, HWND hwnd)
{
    int iInsert = -1;

    if (Window_IsNormal(hwnd))
    {
        // Button.
        if (FindItem(ptasks, hwnd) != -1)
            return -1;

        if(Task_InsertItem(ptasks, hwnd) == 0)
            return -1;
    }
    return iInsert;
}

//---------------------------------------------------------------------------
// If the given window is in the task list then it is selected.
// If it's not in the list then it is added.
int Task_SelectWindow(PTasks ptasks, HWND hwnd)
{
    int i;      // Initialize to zero for the empty case
    int iCurSel;

    // Are there any items?

    // Some item has the focus, is it selected?
    iCurSel = TabCtrl_GetCurSel(ptasks->hwndTab);
    i = -1;

    // We aren't highlighting the correct task. Find it.
    if (IsWindow(hwnd)) {
        i = FindItem(ptasks, hwnd);
        if (i == -1) {

            // Didn't find it - better add it now.
            i = Task_AddWindow(ptasks, hwnd);
        } else if (i == iCurSel) {

            return i; // the current one is already selected
        }
    }

    // passing -1 is ok
    TabCtrl_SetCurSel(ptasks->hwndTab, i);
    
    return i;
}


//---------------------------------------------------------------------------
// Set the focus to the given window
// If fAutomin is set the old task will be re-minimising if it was restored
// during the last switch_to.
void Task_SwitchToWindow(PTasks ptasks, HWND hwnd)
{
    // use GetLastActivePopup (if it's a visible window) so we don't change
    // what child had focus all the time
    HWND hwndLastActive = GetLastActivePopup(hwnd);

    if (IsWindowVisible(hwndLastActive))
        hwnd = hwndLastActive;

    SwitchToThisWindow(hwnd, TRUE);
}

void CALLBACK _FakeSystemMenuCallback(HWND hwnd, UINT uiMsg,
                                ULONG_PTR dwData, LRESULT result)
{
    PTasks ptasks = (PTasks)dwData;
    KillTimer(ptasks->hwnd, IDT_SYSMENU);

    //
    // Since we fake system menu's sometimes, we can come through here
    // 1 or 2 times per system menu request (once for the real one and
    // once for the fake one).  Only decrement it down to 0. Don't go neg.
    //
    if (ptasks->iSysMenuCount)      // Decrement it if any outstanding...
        ptasks->iSysMenuCount--;

    ptasks->dwPos = 0;          // Indicates that we aren't doing a menu now
    if (ptasks->iSysMenuCount <= 0) {
        SendMessage(g_ts.hwndTrayTips, TTM_ACTIVATE, TRUE, 0L);
    }
}

#ifdef USE_SYSMENU_TIMEOUT

HWND CreateFakeWindow(HWND hwndOwner)
{
    WNDCLASSEX wc;

    if (!GetClassInfoEx(hinstCabinet, TEXT("_ExplorerFakeWindow"), &wc)) {
        ZeroMemory(&wc, sizeof(wc));
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = DefWindowProc;
        wc.hInstance = hinstCabinet;
        wc.lpszClassName = TEXT("_ExplorerFakeWindow");
        RegisterClassEx(&wc);
    }
    return CreateWindow(TEXT("_ExplorerFakeWindow"), NULL, WS_POPUP | WS_SYSMENU, 
            0, 0, 0, 0, hwndOwner, NULL, hinstCabinet, NULL);
}

void _HandleSysMenuTimeout(PTasks ptasks)
{
    HMENU   hPopup;
    HWND    hwndTask = ptasks->hwndSysMenu;
    DWORD   dwPos = ptasks->dwPos;
    HWND    hwndFake = NULL;

    KillTimer(ptasks->hwnd, IDT_SYSMENU);

    hPopup = GetSystemMenu(hwndTask, FALSE);

    // This window doesn't have the system menu. Since this window
    // is hung, let's fake one so the user can still close it.
    if (hPopup == NULL) {
        if ((hwndFake = CreateFakeWindow(ptasks->hwnd)) != NULL) {
            hPopup = GetSystemMenu(hwndFake, FALSE);
        }
    }

    if (hPopup)
    {
        //
        // Disable everything on the popup menu _except_ close
        //

        int cItems = GetMenuItemCount(hPopup);
        int iItem  = 0;
#ifdef WINNT
        BOOL fMinimize = ShouldMinMax(hwndTask, TRUE);
#endif
        for (; iItem < cItems; iItem++)
        {
            UINT ID = GetMenuItemID(hPopup, iItem);
#ifdef WINNT
            // Leave the minimize item as is. NT allows
            // hung-window minimization.

            if (ID == SC_MINIMIZE && fMinimize) {
                continue;
            }
#endif
            if (ID != SC_CLOSE)
            {
                EnableMenuItem(hPopup, iItem, MF_BYPOSITION | MF_GRAYED);
            }

        }

        // BUGBUG (RAID 10667) Until this user bug is fixed, we
        // must be the foreground window

        SetForegroundWindow(ptasks->hwnd);
        SetFocus(ptasks->hwnd);

        TrackPopupMenu(hPopup,
                       TPM_RIGHTBUTTON,
                       GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos),
                       0,
                       ptasks->hwnd,
                       NULL);
    }

    // Destroy the fake window
    if (hwndFake != NULL) {
        DestroyWindow(hwndFake);
    }

    // Turn back on tooltips
    _FakeSystemMenuCallback(hwndTask, WM_SYSMENU, (ULONG_PTR)ptasks, 0);
}

void _HandleSysMenu( PTasks ptasks, HWND hwnd )
{
    //
    // At this point, USER32 just told us that the app is now about to bring
    // up its own system menu.  We can therefore put away our fake system
    // menu.
    //
    DefWindowProc(ptasks->hwnd, WM_CANCELMODE, 0, 0);   // Close menu
    KillTimer(ptasks->hwnd, IDT_SYSMENU);
}
#endif

void Task_FakeSystemMenu(PTasks ptasks, HWND hwndTask, DWORD dwPos)
{
    DWORD   dwTimeout = TIMEOUT_SYSMENU;

    if (ptasks->iSysMenuCount <= 0) {
        SendMessage(g_ts.hwndTrayTips, TTM_ACTIVATE, FALSE, 0L);
    }

    // HACKHACK: sleep to give time to switch to them.  (user needs this... )
    Sleep(20);

#ifdef USE_SYSMENU_TIMEOUT
    //
    // ** Advanced System Menu functionality **
    //
    // If the app doesn't put up its system menu within a reasonable timeout,
    // then we popup a fake menu for it anyway.  Suppport for this is required
    // in USER32 (basically it needs to tell us when to turn off our timeout
    // timer).
    //
    // If the user-double right-clicks on the task bar, they get a really
    // short timeout.  If the app is already hung, then they get a really
    // short timeout.  Otherwise, they get the relatively long timeout.
    //
    if (ptasks->dwPos != 0)     // 2nd right-click (on a double-right click)
        dwTimeout = TIMEOUT_SYSMENU_HUNG;

    //
    // We check to see if the app in question is hung, and if so, simulate
    // speed up the timeout process.  It will happen soon enough.
    //
#ifdef WINNT
    if (IsHungAppWindow(hwndTask))
#else
    if (IsHungThread(GetWindowThreadProcessId(hwndTask, NULL)))
#endif
        dwTimeout = TIMEOUT_SYSMENU_HUNG;

    ptasks->hwndSysMenu = hwndTask;
    ptasks->dwPos = dwPos;
    SetTimer(ptasks->hwnd, IDT_SYSMENU, dwTimeout, NULL);
#endif

    ptasks->iSysMenuCount++;
    if (!SendMessageCallback(hwndTask, WM_SYSMENU, 0, dwPos, _FakeSystemMenuCallback, (ULONG_PTR)ptasks)) {
        _FakeSystemMenuCallback(hwndTask, WM_SYSMENU, (ULONG_PTR)ptasks, 0);
    }
}

int Task_GetSelectedItems(PTasks ptasks, HDSA hdsa)
{
    int iSelected = 0;
    int i = TabCtrl_GetItemCount(ptasks->hwndTab) - 1;
    TASKTABITEM item;

    item.tcitem.mask = TCIF_PARAM | TCIF_STATE;
    item.tcitem.dwStateMask = TCIS_BUTTONPRESSED;

    for ( ; i >= 0; i--) {
        TabCtrl_GetItem(ptasks->hwndTab, i, (LPTCITEM)&item);
        if (item.tcitem.dwState & TCIS_BUTTONPRESSED) {
            if (hdsa) {
                DSA_AppendItem(hdsa, &item.tcitem.lParam);
            }
            iSelected++;
        }
    }

    return iSelected;
}

// because we dont' define WIN32_WINNT 0x0500 so that we can run downlevel for now
#ifndef MDITILE_ZORDER
#define MDITILE_ZORDER         0x0004
#endif

void Task_OnCombinedCommand(PTasks ptasks, int iRet)
{
    int i;
    int idCmd;
    HDSA hdsa = DSA_Create(SIZEOF(HWND), 4);
    int iCount;
    ANIMATIONINFO ami;
    LONG iAnimate;

    if (!hdsa) {
        return;
    }

    iCount = Task_GetSelectedItems(ptasks, hdsa);

    // turn off animiations during this
    ami.cbSize = SIZEOF(ANIMATIONINFO);
    SystemParametersInfo(SPI_GETANIMATION, SIZEOF(ami), &ami, FALSE);
    iAnimate = ami.iMinAnimate;
    ami.iMinAnimate = FALSE;
    SystemParametersInfo(SPI_SETANIMATION, SIZEOF(ami), &ami, FALSE);

    switch (iRet) {
    case IDM_CASCADE:
    case IDM_VERTTILE:
    case IDM_HORIZTILE:
        //AppBarNotifyAll(ABN_WINDOWARRANGE, NULL, TRUE);

        for (i = 0; i < iCount; i++) {
            HWND hwnd;
            DSA_GetItem(hdsa, i, &hwnd);
            if (IsIconic(hwnd)) {
                // this needs to by synchronous with the arrange
                ShowWindow(hwnd, SW_RESTORE);
            }
        }
        if (iRet == IDM_CASCADE)
            CascadeWindows(GetDesktopWindow(), MDITILE_ZORDER, NULL, iCount, DSA_GetItemPtr(hdsa, 0));
        else {
            TileWindows(GetDesktopWindow(), 
                        (iRet == IDM_VERTTILE ?
                                MDITILE_VERTICAL : MDITILE_HORIZONTAL), NULL, iCount, DSA_GetItemPtr(hdsa, 0));
        }
        //AppBarNotifyAll(ABN_WINDOWARRANGE, NULL, FALSE);
        break;

    case IDM_MINIMIZE:
        idCmd = SC_MINIMIZE;
        goto DoSysCommand;

    case IDM_RESTORE:
        idCmd = SC_RESTORE;
        goto DoSysCommand;

    case IDM_MAXIMIZE:
        idCmd = SC_MAXIMIZE;
        goto DoSysCommand;

    case IDM_CLOSE:
        idCmd = SC_CLOSE;
DoSysCommand:

        for (iCount--; iCount >= 0; iCount--) {
            HWND hwnd;

            DSA_GetItem(hdsa, iCount, &hwnd);
            if (idCmd == SC_MAXIMIZE)
                if (!ShouldMinMax(hwnd, FALSE))
                    continue;
            PostMessage(hwnd, WM_SYSCOMMAND, idCmd, 0L);
        }
        break;

    }

    // restore animations  state
    ami.iMinAnimate = iAnimate;
    SystemParametersInfo(SPI_SETANIMATION, SIZEOF(ami), &ami, FALSE);

    DSA_Destroy(hdsa);
}

void Task_CombinedMenu(PTasks ptasks, DWORD dwPos)
{
    HMENU hmenu = LoadMenuPopup(MAKEINTRESOURCE(MENU_COMBINEDTASKS));
    if (hmenu) {
        HWND hwndLastForeground = GetForegroundWindow();
        int i = TabCtrl_GetItemCount(ptasks->hwndTab) - 1;
        int iRet;
        TASKTABITEM item;

        item.tcitem.mask = TCIF_PARAM | TCIF_STATE;
        item.tcitem.dwStateMask = TCIS_BUTTONPRESSED;

        for ( ; i >= 0; i--) {
            TabCtrl_GetItem(ptasks->hwndTab, i, (LPTCITEM)&item);
            if (item.tcitem.dwState & TCIS_BUTTONPRESSED) {
                HWND hwnd = (HWND)item.tcitem.lParam;
                HMENU hmenuSys;

                if (ShouldMinMax(hwnd, TRUE)) {
                    EnableMenuItem(hmenu, IDM_MINIMIZE, MF_BYCOMMAND|MF_ENABLED);
                }

                if (IsIconic(hwnd)) {
                    EnableMenuItem(hmenu, IDM_RESTORE, MF_BYCOMMAND|MF_ENABLED);
                }

                hmenuSys = GetSystemMenu(hwnd, FALSE);

                if (hmenuSys) {
                    if (!(GetMenuState(hmenuSys, SC_RESTORE, MF_BYCOMMAND) & MF_DISABLED)) {
                        EnableMenuItem(hmenu, IDM_RESTORE, MF_BYCOMMAND|MF_ENABLED);
                    }
                    if (!(GetMenuState(hmenuSys, SC_MAXIMIZE, MF_BYCOMMAND) & MF_DISABLED)) {
                        EnableMenuItem(hmenu, IDM_MAXIMIZE, MF_BYCOMMAND|MF_ENABLED);
                    }
                    if (!(GetMenuState(hmenuSys, SC_MINIMIZE, MF_BYCOMMAND) & MF_DISABLED)) {
                        EnableMenuItem(hmenu, IDM_MINIMIZE, MF_BYCOMMAND|MF_ENABLED);
                    }
                    if (!(GetMenuState(hmenuSys, SC_CLOSE, MF_BYCOMMAND) & MF_DISABLED)) {
                        EnableMenuItem(hmenu, IDM_CLOSE, MF_BYCOMMAND|MF_ENABLED);
                    }

                }
            }
        }

        g_ts.fIgnoreTaskbarActivate = TRUE;
        SetForegroundWindow(v_hwndTray);
        
        iRet = TrackPopupMenuEx(hmenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
            GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos),
            ptasks->hwnd, NULL);

        g_ts.fIgnoreTaskbarActivate = FALSE;
        if (iRet) {
            Task_OnCombinedCommand(ptasks, iRet);
        }

        SendMessage(ptasks->hwndTab, TCM_DESELECTALL, FALSE, 0);
        SetForegroundWindow(v_hwndTray);
        DestroyMenu(hmenu);
    }
}

void Task_SysMenuForItem(PTasks ptasks, int i, DWORD dwPos)
{
    if (Task_GetSelectedItems(ptasks, NULL) > 1) {

        // more than one selection... do the combined menu.
        Task_CombinedMenu(ptasks, dwPos);

    } else {
        HWND hwndTask = TabCtrl_GetItemHwnd(ptasks->hwndTab, i);
        HWND hwndProxy = hwndTask;
        if (g_fDesktopRaised) {
            LowerDesktop();
        }

        // set foreground first so that we'll switch to it.
        if (IsIconic(hwndTask) && 
            (TabCtrl_GetItemFlags(ptasks->hwndTab, i) & TIF_EVERACTIVEALT)) {
            HWND hwndProxyT = (HWND) GetWindowLongPtr(hwndTask, 0);
            if (hwndProxyT != NULL && IsWindow(hwndProxyT))
                hwndProxy = hwndProxyT;
        }

        SetForegroundWindow(GetLastActivePopup(hwndProxy));
        if (hwndProxy != hwndTask)
            SendMessage(hwndTask, WM_SYSCOMMAND, SC_RESTORE, -2);

        Task_SelectWindow(ptasks, hwndTask);
        PostMessage(ptasks->hwnd, TM_POSTEDRCLICK, (WPARAM)hwndTask, (LPARAM)dwPos);
    }
}

BOOL Task_PostFakeSystemMenu(PTasks ptasks, DWORD dwPos)
{
    int i;
    BOOL fRet;
    TC_HITTESTINFO tcht;

    if (dwPos != (DWORD)-1) {
        tcht.pt.x = GET_X_LPARAM(dwPos);
        tcht.pt.y = GET_Y_LPARAM(dwPos);

        ScreenToClient(ptasks->hwndTab, &tcht.pt);
        i = (int)SendMessage(ptasks->hwndTab, TCM_HITTEST, 0, (LPARAM)&tcht);
    } else {
        i = TabCtrl_GetCurFocus(ptasks->hwndTab);
    }
    fRet = (i != -1);
    if (fRet) {
        Task_SysMenuForItem(ptasks, i, dwPos);
    }
    return fRet;
}

LRESULT Task_HandleNotify(PTasks ptasks, LPNMHDR lpnm)
{
    switch (lpnm->code) {
        case NM_CLICK: {
            TC_HITTESTINFO hitinfo;
            int i;
            DWORD dwPos = GetMessagePos();
            hitinfo.pt.x = GET_X_LPARAM(dwPos);
            hitinfo.pt.y = GET_Y_LPARAM(dwPos);

            // did the click happen on the currently selected tab?
            // if so, tab ctrl isn't going to send us a message. so di it ourselves
            i = TabCtrl_GetCurSel(ptasks->hwndTab);
            ScreenToClient(ptasks->hwndTab, &hitinfo.pt);
            if (i == TabCtrl_HitTest(ptasks->hwndTab, &hitinfo)) {
                HWND hwnd = TabCtrl_GetItemHwnd(ptasks->hwndTab, i);

                if (TabCtrl_GetItemFlags(ptasks->hwndTab, i) & TIF_EVERACTIVEALT) {
                    PostMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, -1);
                    break;
                }

                if (hwnd == GetForegroundWindow()) {
                    if (IsIconic(hwnd))
                        ShowWindowAsync(hwnd, SW_RESTORE);
                } else {

                    if (IsIconic(hwnd))
                        Task_SwitchToSelection(ptasks);
                    else if (ShouldMinMax(hwnd, TRUE)) {
                        SHAllowSetForegroundWindow(hwnd);
                        PostMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
                        PostMessage(ptasks->hwndTab, TCM_SETCURSEL, (WPARAM)-1, 0);
                    }
                }
            }
            break;
        }

        case TCN_SELCHANGE:
            Task_SwitchToSelection(ptasks);
            break;
            
        case TCN_FOCUSCHANGE:
            Task_ScrollIntoView(ptasks);
            break;

        case TTN_SHOW:
            SetWindowPos(g_ts.hwndTrayTips,
                         HWND_TOP,
                         0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            break;

        case TTN_NEEDTEXT:
        {
            LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT)lpnm;
            HWND hwnd;
            TASKTABITEM item;

            item.tcitem.lParam = 0;
            item.tcitem.mask = TCIF_PARAM;

            TabCtrl_GetItem(ptasks->hwndTab, lpttt->hdr.idFrom, (TC_ITEM *)&item);

            hwnd = (HWND)item.tcitem.lParam;
            // do an IsWindow check because user might (does) sometimes
            // send us a redraw item as it's dying, and since we do
            // a postmessage, we don't get it till we're dead
            //DebugMsg(DM_TRACE, "NeedText for hwnd %d, dwflags = %d", hwnd, item.dwFlags);
            if ((item.dwFlags & TIF_SHOULDTIP) && IsWindow(hwnd))
            {
#if defined(WINDOWS_ME)
                DWORD    exStyle;

                exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
                if (exStyle & WS_EX_RTLREADING)
                        lpttt->uFlags |= TTF_RTLREADING;
                else
                        lpttt->uFlags &= ~TTF_RTLREADING;
#endif
        // BUGBUG If you're not unicode, you deserve to hang

#ifdef UNICODE
                InternalGetWindowText(hwnd, lpttt->szText, ARRAYSIZE(lpttt->szText));
#else
                GetWindowText(hwnd, lpttt->szText, ARRAYSIZE(lpttt->szText));
#endif

            }
            else
                lpttt->szText[0] = 0;
            break;
        }

    }
    return 0L;
}

//---------------------------------------------------------------------------
// Switch to a single task (Doesn't do anything if more than one item
// is selected.)
// Returns TRUE if a switch took place.
BOOL Task_SwitchToSelection(PTasks ptasks)
{
    int iItem = (int)SendMessage(ptasks->hwndTab,TCM_GETCURSEL, 0, 0);
    if (iItem != -1)
    {
        HWND hwndTask = TabCtrl_GetItemHwnd(ptasks->hwndTab, iItem);
        if (Window_IsNormal(hwndTask))
        {
            LowerDesktop();
            Task_SwitchToWindow(ptasks, hwndTask);
            return TRUE;
        }
        else
        {
            // Window went away?
            Task_DeleteItem(ptasks, iItem);
        }
    }
    return FALSE;
}

BOOL CALLBACK Task_BuildCallback(HWND hwnd, LPARAM lParam)
{
    PTasks ptasks = (PTasks)lParam;
    if (IsWindow(hwnd) && IsWindowVisible(hwnd) && !GetWindow(hwnd, GW_OWNER) &&
        (!(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW))) {

        Task_AddWindow(ptasks, hwnd);

    }
    return TRUE;
}


//---------------------------------------------------------------------------
LRESULT _HandleCreate(HWND hwnd, LPVOID lpCreateParams)
{
    PTasks ptasks = lpCreateParams;
    DWORD dwStyle = 0;
    
    SetWindowPtr(hwnd, 0, ptasks);

    g_ts.hwndView = hwnd;
    ptasks->hwnd = hwnd;

    // Create a listbox in the client area.
    ptasks->hwndTab = CreateWindow(WC_TABCONTROL, NULL,
                                  WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE |
                                   TCS_FOCUSNEVER | TCS_MULTISELECT | TCS_OWNERDRAWFIXED | TCS_MULTILINE | TCS_BUTTONS | TCS_FIXEDWIDTH | dwStyle,
                                  0,0,0,0, hwnd, (HMENU)IDC_FOLDERTABS, hinstCabinet, NULL);

    if (ptasks->hwndTab)
    {
        TOOLINFO ti;
        TabCtrl_SetItemExtra(ptasks->hwndTab, (SIZEOF(TASKTABITEM) - SIZEOF(TC_ITEM)) + SIZEOF(HWND));

        // initial size
        TabCtrl_SetItemSize(ptasks->hwndTab, g_cxMinimized, g_cySize + 2 * g_cyEdge);

        ti.cbSize = SIZEOF(ti);
        ti.uFlags = TTF_IDISHWND;
        ti.hwnd = ptasks->hwndTab;
        ti.uId = (UINT_PTR)ptasks->hwndTab;
        ti.lpszText = 0;
        if (g_ts.hwndTrayTips) {
            SendMessage(g_ts.hwndTrayTips, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
            SendMessage(ptasks->hwndTab, TCM_SETTOOLTIPS, (WPARAM)g_ts.hwndTrayTips, 0L);
        }

        ptasks->WM_ShellHook = RegisterWindowMessage(TEXT("SHELLHOOK"));
        RegisterShellHook(hwnd, 3); // 3 = magic flag

        // force getting of font, calc of metrics
        Task_HandleWinIniChange(ptasks, 0, 0, TRUE);

        EnumWindows(Task_BuildCallback, (LPARAM)ptasks);
        return 0;       // success
    }

    // Failure.
    return -1;
}



//---------------------------------------------------------------------------
LRESULT _HandleDestroy(PTasks ptasks)
{
    RegisterShellHook(ptasks->hwnd, FALSE);

    ptasks->hwnd = NULL;

    return 1;
}

int _HandleScroll(PTasks ptasks, UINT code, int nPos, UINT sb)
{

    SCROLLINFO si;

    si.cbSize = SIZEOF(SCROLLINFO);
    si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
    GetScrollInfo(ptasks->hwnd, sb, &si);
    si.nMax -= (si.nPage -1);

    switch (code) {
        case SB_BOTTOM:
            nPos = si.nMax;
            break;

        case SB_TOP:
            nPos = 0;
            break;

        case SB_ENDSCROLL:
            nPos = si.nPos;
            break;

        case SB_LINEDOWN:
            nPos = si.nPos + 1;
            break;

        case SB_LINEUP:
            nPos = si.nPos - 1;
            break;

        case SB_PAGEDOWN:
            nPos = si.nPos + si.nPage;
            break;

        case SB_PAGEUP:
            nPos = si.nPos - si.nPage;
            break;

        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            break;
    }
    if (nPos > (int)(si.nMax))
        nPos = si.nMax;
    if (nPos < 0 )
        nPos = 0;

    SetScrollPos(ptasks->hwnd, sb, nPos, TRUE);

    return nPos;
}

//---------------------------------------------------------------------------
LRESULT _HandleVScroll(PTasks ptasks, UINT code, int nPos)
{
    RECT rcItem;
    int cyRow;

    nPos = _HandleScroll(ptasks, code, nPos, SB_VERT);

    TabCtrl_GetItemRect(ptasks->hwndTab, 0, &rcItem);
    cyRow = RECTHEIGHT(rcItem) + g_cyTabSpace;
    SetWindowPos(ptasks->hwndTab, 0, 0, -nPos * cyRow , 0, 0, SWP_NOACTIVATE | SWP_NOSIZE |SWP_NOZORDER);

    return 0;
}

LRESULT _HandleHScroll(PTasks ptasks, UINT code, int nPos)
{
    RECT rcItem;
    int cxRow;

    nPos = _HandleScroll(ptasks, code, nPos, SB_HORZ);

    TabCtrl_GetItemRect(ptasks->hwndTab, 0, &rcItem);
    cxRow = RECTWIDTH(rcItem) + g_cxTabSpace;
    SetWindowPos(ptasks->hwndTab, 0, -nPos * cxRow, 0, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE |SWP_NOZORDER);

    return 0;
}

// after a selection is made, scroll it into view
void Task_ScrollIntoView(PTasks ptasks)
{
    DWORD dwStyle = GetWindowLong(ptasks->hwnd, GWL_STYLE);
    if (dwStyle & (WS_HSCROLL | WS_VSCROLL)) {
        int iItem = (int)SendMessage(ptasks->hwndTab,TCM_GETCURFOCUS, 0, 0);
        // loop until return call below
        while (iItem != -1) {
            RECT rc;
            RECT rcWindow;
            RECT rcIntersect;
            TabCtrl_GetItemRect(ptasks->hwndTab, iItem, &rc);
            MapWindowRect(ptasks->hwndTab, HWND_DESKTOP, &rc);
            GetWindowRect(ptasks->hwnd, &rcWindow);
            
            if (!IntersectRect(&rcIntersect, &rc, &rcWindow)) {
                // need to scroll.  selected button not in view
                if (dwStyle & WS_HSCROLL) {
                    if (rc.right < rcWindow.right)
                        _HandleHScroll(ptasks, SB_LINEUP,SB_HORZ);
                    else 
                        _HandleHScroll(ptasks, SB_LINEDOWN,SB_HORZ);
                } else {
                    if (rc.top < rcWindow.top)
                        _HandleVScroll(ptasks, SB_LINEUP, SB_VERT);
                    else
                        _HandleVScroll(ptasks, SB_LINEDOWN, SB_VERT);
                }
            } else
                return;
        }
    }
}


//---------------------------------------------------------------------------
LRESULT _HandleSize(PTasks ptasks, WPARAM fwSizeType)
{
    // Make the listbox fill the parent;
    if (fwSizeType != SIZE_MINIMIZED)
    {
        CheckSize(ptasks, TRUE);
    }
    return 0;
}

extern void Tray_RealityCheck();

//---------------------------------------------------------------------------
// Have the task list show the given window.
// NB Ignore taskman itself.
LRESULT Task_HandleActivate(PTasks ptasks, HWND hwndActive)
{
    //
    // App-window activation change is a good time to do a reality
    // check (make sure "always-on-top" agrees with actual window
    // position, make sure there are no ghost buttons, etc).
    //
    Tray_RealityCheck();

    if (Window_IsNormal(hwndActive))
    {
        int i;
        DWORD dwFlags;

        LowerDesktop();

        i = Task_SelectWindow(ptasks, hwndActive);
        dwFlags = TabCtrl_GetItemFlags(ptasks->hwndTab, i);

        // Strip off TIF_FLASHING
        dwFlags &= ~TIF_FLASHING;
        TabCtrl_SetItemFlags(ptasks->hwndTab, i, dwFlags);

        // Update the global traystuff flag that says, "There is an item flashing."
        Task_UpdateFlashingFlag(ptasks->hwndTab);

        // if it's flashed blue, turn it off.
        if (dwFlags & TIF_RENDERFLASHED)
            RedrawItem(ptasks, hwndActive, HSHELL_REDRAW);
    }
    else
    {
        if (!(g_ts.fIgnoreTaskbarActivate && GetForegroundWindow() == v_hwndTray))
            TabCtrl_SetCurSel(ptasks->hwndTab, -1);
    }

    if (hwndActive)
        g_ts.hwndLastActive = hwndActive;

    return TRUE;
}

//---------------------------------------------------------------------------
void Task_HandleOtherWindowDestroyed(PTasks ptasks, HWND hwndDestroyed)
{
    int i;
    MSG msg;
    
    if (PeekMessage(&msg, NULL, TM_WINDOWDESTROYED, TM_WINDOWDESTROYED,
        PM_NOREMOVE) && msg.hwnd==ptasks->hwnd)
    {
        // Don's use ShowWindow(SW_HIDE) since we don't want a repaint until
        // we are all done
        SetWindowStyleBit(ptasks->hwndTab, WS_VISIBLE, 0);
    }
    else
    {
        if (!(GetWindowLong(ptasks->hwndTab, GWL_STYLE) & WS_VISIBLE))
            ShowWindow(ptasks->hwndTab, SW_SHOWNORMAL);
    }

    // Look for the destoyed window.
    i = FindItem(ptasks, hwndDestroyed);
    if (i != -1)
    {
        Task_DeleteItem(ptasks, i);
    }
    else
    {
        // If the item doesn't exist in the task list, make sure it isn't part
        // of somebody's fake SDI implementation.  Otherwise Minimize All will
        // break.
        for (i = TabCtrl_GetItemCount(ptasks->hwndTab) - 1; i >= 0; i--)
        {
            if ((TabCtrl_GetItemFlags(ptasks->hwndTab, i) & TIF_EVERACTIVEALT) &&
                (HWND) GetWindowLongPtr(TabCtrl_GetItemHwnd(ptasks->hwndTab, i), 0) ==
                       hwndDestroyed)
            {
                goto NoDestroy;
            }
        }
    }

    TrayHandleWindowDestroyed(hwndDestroyed);

NoDestroy:
    // This might have been a rude app.  Figure out if we've
    // got one now and have the tray sync up.
    Tray_HandleFullScreenApp(Task_FindRudeApp(ptasks));

    if (g_ts.hwndLastActive == hwndDestroyed)
    {
        if (g_ts.hwndLastActive == hwndDestroyed)
            g_ts.hwndLastActive = NULL;
    }
}

//---------------------------------------------------------------------------
void Task_HandleOverflow(PTasks ptasks)
{
    int i;

    BOOL fFoundDestroyedItems = FALSE;

    for (i = TabCtrl_GetItemCount(ptasks->hwndTab) - 1; i >= 0; i--)
    {
        HWND hwnd = TabCtrl_GetItemHwnd(ptasks->hwndTab, i);
        if (!IsWindow(hwnd))
        {
            if (!fFoundDestroyedItems)
            {
                // Don's use ShowWindow(SW_HIDE) since we don't want a repaint until
                // we are all done
                SetWindowStyleBit(ptasks->hwndTab, WS_VISIBLE, 0);
                fFoundDestroyedItems = TRUE;
            }

            // And delete this item...
            Task_DeleteItem(ptasks, i);
        }
    }

    // If we deleted items call off to destroy stuff out of save list
    if (fFoundDestroyedItems)
    {
        ShowWindow(ptasks->hwndTab, SW_SHOWNORMAL);
        TrayHandleWindowDestroyed(NULL);
    }

}

void Task_HandleOtherWindowCreated(PTasks ptasks, HWND hwndCreated)
{
    MSG msg;

    while (PeekMessage(&msg, NULL, TM_WINDOWDESTROYED, TM_WINDOWDESTROYED, PM_REMOVE)) {
        Task_HandleOtherWindowDestroyed(ptasks, (HWND)msg.lParam);
    }
    Task_AddWindow(ptasks, hwndCreated);
}

void Task_HandleGetMinRect(PTasks ptasks, HWND hwndShell, LPPOINTS lprc)
{
    int i;
    RECT rc;
    RECT rcTask;

    i = FindItem(ptasks, hwndShell);
    if (i == -1)
        return;

    // Found it in our list.
    TabCtrl_GetItemRect(ptasks->hwndTab, i, &rc);

    //
    // If the Tab is mirrored then let's retreive the screen coordinates
    // by calculating from the left edge of the screen since screen coordinates 
    // are not mirrored so that minRect will prserve its location. [samera]
    //
    if (IS_WINDOW_RTL_MIRRORED(GetDesktopWindow())) {  
        RECT rcTab;

        GetWindowRect( ptasks->hwndTab , &rcTab );
        rc.left   += rcTab.left;
        rc.right  += rcTab.left;
        rc.top    += rcTab.top;
        rc.bottom += rcTab.top;
    }
    else
    {
        MapWindowPoints(ptasks->hwndTab, HWND_DESKTOP, (LPPOINT)&rc, 2);
    }

    lprc[0].x = (short)rc.left;
    lprc[0].y = (short)rc.top;
    lprc[1].x = (short)rc.right;
    lprc[1].y = (short)rc.bottom;

    // make sure the rect is within out client area
    GetClientRect(ptasks->hwnd, &rcTask);
    MapWindowPoints(ptasks->hwnd, HWND_DESKTOP, (LPPOINT)&rcTask, 2);
    if (lprc[0].x < rcTask.left) {
        lprc[1].x = lprc[0].x = (short)rcTask.left;
        lprc[1].x++;
    }
    if (lprc[0].x > rcTask.right) {
        lprc[1].x = lprc[0].x = (short)rcTask.right;
        lprc[1].x++;
    }
    if (lprc[0].y < rcTask.top) {
        lprc[1].y = lprc[0].y = (short)rcTask.top;
        lprc[1].y++;
    }
    if (lprc[0].y > rcTask.bottom) {
        lprc[1].y = lprc[0].y = (short)rcTask.bottom;
        lprc[1].y++;
    }
}

BOOL Task_IsItemActive(PTasks ptasks, int iItem)
{
    HWND hwnd = GetForegroundWindow();
    HWND hwndItem = TabCtrl_GetItemHwnd(ptasks->hwndTab, iItem);

    return (hwnd && hwnd == hwndItem);
}

// bugbug, move this to shellp.h
#define HSHELL_HIGHBIT 0x8000
#define HSHELL_FLASH (HSHELL_REDRAW|HSHELL_HIGHBIT)
#define HSHELL_RUDEAPPACTIVATED (HSHELL_WINDOWACTIVATED|HSHELL_HIGHBIT)

void RedrawItem(PTasks ptasks, HWND hwndShell, WPARAM code )
{
    int i = FindItem(ptasks, hwndShell);
    if (i != -1)
    {
        RECT rc;
        DWORD dwFlags;

        TOOLINFO ti;
        ti.cbSize = SIZEOF(ti);

        // set the bit saying whether we should flash or not
        dwFlags = TabCtrl_GetItemFlags(ptasks->hwndTab, i);

        if ((code == HSHELL_FLASH) != BOOLIFY(dwFlags & TIF_RENDERFLASHED))
        {
            // only do the set if this bit changed.
            if (code == HSHELL_FLASH)
            {
                // TIF_RENDERFLASHED means, "Paint the background blue."
                // TIF_FLASHING means, "This item is flashing."

                dwFlags |= TIF_RENDERFLASHED;

                // Only set TIF_FLASHING and unhide the tray if the app is inactive.
                // Some apps (e.g., freecell) flash themselves while active just for
                // fun.  It's annoying for the autohid tray to pop out in that case.

                if (!Task_IsItemActive(ptasks, i))
                {
                    dwFlags |= TIF_FLASHING;

                    // On NT5, unhide the tray whenever we get a flashing app.
                    if (g_bRunOnNT5)
                        Tray_Unhide();
                }
            }
            else
            {
                // Don't clear TIF_FLASHING.  We clear that only when the app
                // is activated.
                dwFlags &= ~TIF_RENDERFLASHED;
            }
            TabCtrl_SetItemFlags(ptasks->hwndTab, i, dwFlags);

            // Update the global traystuff flag that says, "There is an item flashing."
            Task_UpdateFlashingFlag(ptasks->hwndTab);
        }

        if (TabCtrl_GetItemRect(ptasks->hwndTab, i, &rc))
        {
            InflateRect(&rc, -g_cxEdge, -g_cyEdge);
            RedrawWindow(ptasks->hwndTab, &rc, NULL, RDW_INVALIDATE);
        }

        ti.hwnd = ptasks->hwndTab;
        ti.uId = i;
        ti.lpszText = LPSTR_TEXTCALLBACK;
        SendMessage(g_ts.hwndTrayTips, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);
    }
}


void SetActiveAlt(PTasks ptasks, HWND hwndAlt)
{
    int iMax;
    int i;

    if (!ptasks)
        return;

    iMax = TabCtrl_GetItemCount(ptasks->hwndTab);
    for ( i = 0; i < iMax; i++) {
        DWORD dwFlags = TabCtrl_GetItemFlags(ptasks->hwndTab, i);
        HWND hwndTask = TabCtrl_GetItemHwnd(ptasks->hwndTab, i);
        if (hwndTask == hwndAlt)
            dwFlags |= TIF_ACTIVATEALT | TIF_EVERACTIVEALT;
        else
            dwFlags &= ~TIF_ACTIVATEALT;
        TabCtrl_SetItemFlags(ptasks->hwndTab, i, dwFlags);
    }
}

HRESULT SHIsParentOwnerOrSelf(HWND hwndParent, HWND hwnd)
{
    while (hwnd)
    {
        if (hwnd == hwndParent)
            return S_OK;

        hwnd = GetParent(hwnd);
    }

    return E_FAIL;
}

BOOL Task_IsRudeWindowActive(HWND hwnd)
{
    // A rude window is considered "active" if it is:
    // - in the same thread as the foreground window, or
    // - in the same window hierarchy as the foreground window
    //
    HWND hwndFore = GetForegroundWindow();

    DWORD dwID = GetWindowThreadProcessId(hwnd, NULL);
    DWORD dwIDFore = GetWindowThreadProcessId(hwndFore, NULL);

    if (dwID == dwIDFore)
        return TRUE;
    else if (SHIsParentOwnerOrSelf(hwnd, hwndFore) == S_OK)
        return TRUE;

    return FALSE;
}

//***   Task_IsRudeWindow -- is given HWND 'rude' (fullscreen) on given monitor
//
BOOL Task_IsRudeWindow(HMONITOR hmon, HWND hwnd)
{
    ASSERT(hmon);
    ASSERT(hwnd);

    // Don't count the desktop as rude
    if (hwnd != v_hwndDesktop)
    {
        RECT rcMon, rcApp, rcTmp;
        DWORD dwStyle;

        //
        // NB: User32 will sometimes send us spurious HSHELL_RUDEAPPACTIVATED
        // messages.  When this happens, and we happen to have a maximized
        // app up, the old version of this code would think there was a rude app
        // up.  This mistake would break tray always-on-top and autohide.
        //
        //
        // The old logic was:
        //
        // If the app's window rect takes up the whole monitor, then it's rude.
        // (This check could mistake normal maximized apps for rude apps.)
        //
        //
        // The new logic is:
        //
        // If the app window does not have WS_DLGFRAME and WS_THICKFRAME,
        // then do the old check.  Rude apps typically lack one of these bits
        // (while normal apps usually have them), so do the old check in
        // this case to avoid potential compat issues with rude apps that
        // have non-fullscreen client areas.
        //
        // Otherwise, get the client rect rather than the window rect
        // and compare that rect against the monitor rect.
        //

        // If (mon U app) == app, then app is filling up entire monitor
        GetMonitorRect(hmon, &rcMon);

        dwStyle = GetWindowLong(hwnd, GWL_STYLE);
        if ((dwStyle & (WS_CAPTION | WS_THICKFRAME)) == (WS_CAPTION | WS_THICKFRAME))
        {
            // Doesn't match rude app profile; use client rect
            GetClientRect(hwnd, &rcApp);
            MapWindowPoints(hwnd, HWND_DESKTOP, (LPPOINT)&rcApp, 2);
        }
        else
        {
            // Matches rude app profile; use window rect
            GetWindowRect(hwnd, &rcApp);
        }
        UnionRect(&rcTmp, &rcApp, &rcMon);
        if (EqualRect(&rcTmp, &rcApp))
        {
            // Looks like a rude app.  Is it active?
            if (Task_IsRudeWindowActive(hwnd))
                return TRUE;
        }
    }

    // No, not rude
    return FALSE;
}

struct iradata {
    HMONITOR    hmon;   // IN hmon we're checking against
    HWND        hwnd;   // INOUT hwnd of 1st rude app found
};

BOOL CALLBACK Task_IsRudeCallback(HWND hwnd, LPARAM lParam)
{
    struct iradata *pira = (struct iradata *)lParam;
    HMONITOR hmon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

    if ((pira->hmon == NULL || pira->hmon == hmon))
    {
        if (Task_IsRudeWindow(hmon, hwnd))
        {
            // We're done
            pira->hwnd = hwnd;
            return FALSE;
        }
    }

    // Keep going
    return TRUE;
}

HWND Task_FindRudeApp(PTasks ptasks)
{
    struct iradata irad = { NULL, 0 };

    // First try our cache
    if (IsWindow(ptasks->hwndLastRude))
    {
        if (!Task_IsRudeCallback(ptasks->hwndLastRude, (LPARAM)&irad))
        {
            // Cache hit
            return irad.hwnd;
        }
    }

    // No luck, gotta do it the hard way
    EnumWindows(Task_IsRudeCallback, (LPARAM)&irad);

    // Cache it for next time
    ptasks->hwndLastRude = irad.hwnd;

    return irad.hwnd;
}

extern HWND g_hwndPrevFocus;

//---------------------------------------------------------------------------
// We get notification about activation etc here. This saves having
// a fine-grained timer.
LRESULT Task_HandleShellHook(PTasks ptasks, int iCode, LPARAM lParam)
{
    HWND hwndRude = NULL;
    HWND hwnd = (HWND)lParam;

    // Tell library function that we have processed this message.
    RegisterShellHook(ptasks->hwnd, 5);

    switch (iCode) {
    case HSHELL_GETMINRECT:
        {
            LPSHELLHOOKINFO pshi = (LPSHELLHOOKINFO)lParam;
            Task_HandleGetMinRect(ptasks, pshi->hwnd, (LPPOINTS)&pshi->rc);
        }
        return TRUE;

    case HSHELL_RUDEAPPACTIVATED:

        // We shouldn't need to do this but we're getting rude-app activation
        // msgs when there aren't any.

        // Also, the hwnd that user tells us about is just the foreground window --
        // Task_FindRudeApp will return the window that's actually sized fullscreen.

        hwndRude = Task_FindRudeApp(ptasks);
        if (!hwndRude)
        {
            // If we can't find a rude app, set a timer and check again in a bit

            // we check this much earlier now than in win95, there are race conditions that apps 
            // activate smaller than size up and if we check them before they're sized up, we lose.
            // so set a timer and check again in a bit if we're going to blow off user's notify
            SetTimer(ptasks->hwnd, IDT_RECHECKRUDEAPP, 1000, NULL);
        }
        goto L_HSHELL_WINDOWACTIVATED;

    case HSHELL_WINDOWACTIVATED:
    L_HSHELL_WINDOWACTIVATED:
        {

            int iItem = FindItem(ptasks, hwnd);
            if (iItem == -1)
            {
                int iMax;
                int i;
                BOOL fFoundBackup = FALSE;

                iMax = TabCtrl_GetItemCount(ptasks->hwndTab);
                for (i = 0; i < iMax; i++)
                {
                    DWORD dwFlags = TabCtrl_GetItemFlags(ptasks->hwndTab, i);
                    if ((dwFlags & TIF_ACTIVATEALT) ||
                        (!fFoundBackup && (dwFlags & TIF_EVERACTIVEALT)))
                    {
                        DWORD dwpid1, dwpid2;
                        HWND hwndNew = TabCtrl_GetItemHwnd(ptasks->hwndTab, i);

                        GetWindowThreadProcessId(hwnd, &dwpid1);
                        GetWindowThreadProcessId(hwndNew, &dwpid2);

                        // Only change if they're in the same process
                        if (dwpid1 == dwpid2)
                        {
                            hwnd = TabCtrl_GetItemHwnd(ptasks->hwndTab, i);
                            if (dwFlags & TIF_ACTIVATEALT)
                                break;
                            else
                                fFoundBackup = TRUE;
                        }
                    }
                }
            }

            Task_HandleActivate(ptasks, hwnd);
            Tray_HandleFullScreenApp(hwndRude);
        }
        break;

    case HSHELL_WINDOWCREATED:
        Task_HandleOtherWindowCreated(ptasks, hwnd);
        break;

    case HSHELL_WINDOWDESTROYED:
        if (!PostMessage(ptasks->hwnd, TM_WINDOWDESTROYED, 0, lParam))
        {
            Task_HandleOtherWindowDestroyed(ptasks, hwnd);
        }
        break;

    case HSHELL_ACTIVATESHELLWINDOW:
        SwitchToThisWindow(v_hwndTray, TRUE);
        SetForegroundWindow(v_hwndTray);
        break;

    case HSHELL_TASKMAN:

        //
        // On NT, we've arranged for winlogon/user to send a -1 lParam to indicate
        // that the real task list should be displayed (normally the lParam is
        // the hwnd)
        //

#ifdef WINNT
        if (-1 == lParam)
        {
            RunSystemMonitor();
        }
        else
        {
#endif
            // if it wasn't invoked via control escape, then it was the win key
            if (!g_ts.fStuckRudeApp && GetAsyncKeyState(VK_CONTROL) >= 0)
            {
                if (!g_hwndPrevFocus)
                {
                    HWND hwndForeground = GetForegroundWindow();
                    if (hwndForeground != v_hwndTray)
                    {
                        g_hwndPrevFocus = hwndForeground;
                    }
                }
                else if (GetForegroundWindow() == v_hwndTray)
                {
                    // g_hwndPrevFocus will be wiped out by the MPOS_FULLCANCEL
                    // so save it before we lose it
                    HWND hwndPrevFocus = g_hwndPrevFocus;

                    if (g_ts._pmpStartMenu)
                        g_ts._pmpStartMenu->lpVtbl->OnSelect(g_ts._pmpStartMenu, MPOS_FULLCANCEL);

                    // otherwise they're just hitting the key again.
                    // set focus away
                    SHAllowSetForegroundWindow(hwndPrevFocus);
                    SetForegroundWindow(hwndPrevFocus);
                    g_hwndPrevFocus = NULL;
                    return TRUE;
                }
            }

            PostMessage(v_hwndTray, TM_ACTASTASKSW, 0, 0L);

#ifdef WINNT
        }
#endif
        return TRUE;

#ifdef USE_SYSMENU_TIMEOUT
    case HSHELL_SYSMENU:
        _HandleSysMenu(ptasks, hwnd);
        break;
#endif

    case HSHELL_REDRAW:
    case HSHELL_FLASH:
        RedrawItem(ptasks, hwnd, iCode);
        break;

    case HSHELL_OVERFLOW:
        Task_HandleOverflow(ptasks);
        break;

#ifdef WINNT
    case HSHELL_ENDTASK:
        EndTask(hwnd, FALSE, FALSE);
        break;

    default:
        return Task_HandleAppCommand((WPARAM)iCode, lParam);
#endif
    }

    return 0;
}

void Task_InitGlobalFonts()
{
    HFONT hfont;
    NONCLIENTMETRICS ncm;

    ncm.cbSize = SIZEOF(ncm);
    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, SIZEOF(ncm), &ncm, 0))
    {
        // Create the bold font
        ncm.lfCaptionFont.lfWeight = FW_BOLD;
        hfont = CreateFontIndirect(&ncm.lfCaptionFont);
        if (hfont) 
        {
            if (g_hfontCapBold)
                DeleteObject(g_hfontCapBold);

            g_hfontCapBold = hfont;
        }

        // Create the normal font
        ncm.lfCaptionFont.lfWeight = FW_NORMAL;
        hfont = CreateFontIndirect(&ncm.lfCaptionFont);
        if (hfont) 
        {
            if (g_hfontCapNormal)
                DeleteObject(g_hfontCapNormal);

            g_hfontCapNormal = hfont;
        }
    }
}

//---------------------------------------------------------------------------
LRESULT Task_HandleWinIniChange(PTasks ptasks, WPARAM wParam, LPARAM lParam, BOOL fOnCreate)
{
    if (wParam == SPI_SETNONCLIENTMETRICS ||
        ((!wParam) && (!lParam || (lstrcmpi((LPTSTR)lParam, TEXT("WindowMetrics")) == 0)))) 
    {
        //
        // On creation, don't bother creating the fonts if someone else
        // (such as the clock control) has already done it for us.
        //
        if (!fOnCreate || !g_hfontCapNormal)
            Task_InitGlobalFonts();

        if (fOnCreate)
        {
            //
            // On creation, we haven't been inserted into bandsite yet,
            // so we need to defer size validation.
            //
            PostMessage(ptasks->hwnd, TBC_VERIFYBUTTONHEIGHT, 0, 0);
        }
        else
        {
            Task_VerifyButtonHeight(ptasks);
        }
    }
    return 0;
}

void Task_VerifyButtonHeight(PTasks ptasks)
{
    RECT rc;
    DWORD dwStuck;
    int cyOld;
    int cyNew;

    // verify the size of the buttons.
    TabCtrl_GetItemRect(ptasks->hwndTab, 0, &rc);
    cyOld = RECTHEIGHT(rc);
    CheckSize(ptasks, TRUE);
    BandSite_Update(g_ts.ptbs);     // _BandInfoChanged
    TabCtrl_GetItemRect(ptasks->hwndTab, 0, &rc);
    cyNew = RECTHEIGHT(rc);

    dwStuck = Tray_GetStuckPlace();
    if (STUCK_HORIZONTAL(dwStuck) &&  (cyOld != cyNew)) 
    {
        // if the size has changed, resize the tray,

        // make sure it's at least one row height
        GetWindowRect(v_hwndTray, &rc);
        if (RECTHEIGHT(rc) < cyNew) 
        {
            if (rc.top <= 0)
                rc.bottom = rc.top + cyNew;
            else
                rc.top = rc.bottom - cyNew;
        }
        SendMessage(v_hwndTray, WM_SIZING, 0, (LPARAM)&rc);
        SetWindowPos(v_hwndTray, 0, rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc),
                     SWP_NOACTIVATE | SWP_NOZORDER);
    } 
    else 
    {
        // otherwise just redraw it all
        RedrawWindow(ptasks->hwndTab, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
    }
}

LRESULT _HandleSizing(PTasks ptasks, LPRECT lpRect)
{
    int iHeight;
    int iItemHeight;
    RECT rcItem;

    lpRect->bottom += g_cyEdge*2;
    iHeight = RECTHEIGHT(*lpRect);

    // full height of one row including y inter button spacing
    TabCtrl_GetItemRect(ptasks->hwndTab, 0, &rcItem);

    iItemHeight = RECTHEIGHT(rcItem) + g_cyTabSpace;
    iHeight += iItemHeight/2;
    iHeight -= (iHeight % iItemHeight);
    if (iHeight > g_cyTabSpace)
        iHeight -= g_cyTabSpace;
    lpRect->bottom = lpRect->top + iHeight;
    return 0;
}

BOOL _HandleDrawItem(PTasks ptasks, LPDRAWITEMSTRUCT lpdis)
{
    UINT uDCFlags;
    struct TaskExtra {
        HWND hwnd;
        DWORD dwFlags;
    } *info =  (struct TaskExtra*)lpdis->itemData;
    BOOL fTruncated;
    WORD wLang;

    if (!Window_IsNormal(info->hwnd)) 
    {
        //Task_HandleOtherWindowDestroyed(ptasks, info->hwnd);
        // our shell hook should have prevented this
        //ASSERT(0);
    } 
    else 
    {

        if (info->dwFlags & TIF_RENDERFLASHED) 
        {
            uDCFlags = DC_ACTIVE;
        } 
        else 
        {
            uDCFlags =
                DC_INBUTTON  |
                    ((lpdis->itemState & ODS_SELECTED) ? DC_ACTIVE : 0);
        }

        if (lpdis->itemState & ODS_SELECTED) 
        {
            lpdis->rcItem.bottom++;
            lpdis->rcItem.top++;
        }

       // _HandleDrawItem(PTasks ptasks, LPDRAWITEMSTRUCT lpdis)
        wLang = GetUserDefaultLangID();
        if (PRIMARYLANGID(wLang) == LANG_CHINESE &&
           ((SUBLANGID(wLang) == SUBLANG_CHINESE_TRADITIONAL) ||
            (SUBLANGID(wLang) == SUBLANG_CHINESE_SIMPLIFIED)))
        {
            // Select normal weight font for chinese language
            fTruncated = !DrawCaptionTemp(info->hwnd, lpdis->hDC,&lpdis->rcItem,
                                          g_hfontCapNormal,
                                          NULL, NULL,
                                          uDCFlags | DC_TEXT | DC_ICON | DC_NOSENDMSG);
        }
        else
        {

            fTruncated = !DrawCaptionTemp(info->hwnd, lpdis->hDC, &lpdis->rcItem,
                        (lpdis->itemState & ODS_SELECTED) ? g_hfontCapBold : g_hfontCapNormal,
                        NULL, NULL, uDCFlags | DC_TEXT | DC_ICON | DC_NOSENDMSG);
        }
        // save away info on whether we should tool tip or not
        if (fTruncated)
            info->dwFlags |= TIF_SHOULDTIP;
        else
            info->dwFlags &= ~TIF_SHOULDTIP;

        if (lpdis->itemState & ODS_SELECTED) 
        {
            COLORREF clr;
            HBRUSH hbr;

            // now draw in that one line
            if (uDCFlags == DC_ACTIVE) 
            {
                clr = SetBkColor(lpdis->hDC, GetSysColor(COLOR_ACTIVECAPTION));
            } 
            else 
            {
                hbr = (HBRUSH)DefWindowProc(ptasks->hwndTab, WM_CTLCOLORSCROLLBAR, (WPARAM)lpdis->hDC, (LPARAM)ptasks->hwndTab);
                hbr = SelectObject(lpdis->hDC, hbr);
            }

            lpdis->rcItem.top--;
            lpdis->rcItem.bottom = lpdis->rcItem.top + 1;
            ExtTextOut(lpdis->hDC, 0, 0, ETO_OPAQUE, &lpdis->rcItem, NULL, 0,NULL);

            if (uDCFlags == DC_ACTIVE) 
            {
                SetBkColor(lpdis->hDC, clr);
            } 
            else 
            {
                SelectObject(lpdis->hDC, hbr);
            }
        }
    }
    return TRUE;
}

void Task_TaskTab(PTasks ptasks, int iIncr)
{
    int i;
    int iCount = TabCtrl_GetItemCount(ptasks->hwndTab);
    if (iCount) {
        if (GetFocus() != ptasks->hwndTab)
            SetFocus(ptasks->hwndTab);
        // make sure nothing is selected
        if (TabCtrl_GetCurSel(ptasks->hwndTab) != -1)
            Task_SelectWindow(ptasks, NULL);

        i = TabCtrl_GetCurFocus(ptasks->hwndTab);
        if ((INT_PTR)iIncr < 0 && i == -1)
            i = 0;
        i = (i + iIncr + iCount) % iCount;
        TabCtrl_SetCurFocus(ptasks->hwndTab, i);
    }
}

void Task_OnSetFocus(PTasks ptasks)
{
    NMHDR nmhdr;

    SetFocus(ptasks->hwndTab);

    nmhdr.hwndFrom = ptasks->hwnd;
    nmhdr.code = NM_SETFOCUS;
    SendMessage(GetParent(ptasks->hwnd), WM_NOTIFY, (WPARAM)NULL, (LPARAM)&nmhdr);
}

LRESULT CALLBACK Task_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PTasks ptasks = &g_tasks;
    LRESULT lres;

    INSTRUMENT_WNDPROC(SHCNFI_MAIN_WNDPROC, hwnd, msg, wParam, lParam);

    switch (msg) {
    case WM_CREATE:
        return _HandleCreate(hwnd, ((LPCREATESTRUCT)lParam)->lpCreateParams);

    case WM_DESTROY:
        return _HandleDestroy(ptasks);

    case WM_SIZE:
        return _HandleSize(ptasks, wParam);

    case WM_DRAWITEM:
        return _HandleDrawItem(ptasks, (LPDRAWITEMSTRUCT)lParam);

    case WM_WININICHANGE:
        Cabinet_InitGlobalMetrics(wParam, (LPTSTR)lParam);
        Task_HandleWinIniChange(ptasks, wParam, lParam, FALSE);
        return SendMessage(ptasks->hwndTab, WM_WININICHANGE, wParam, lParam);

    case WM_SIZING:
        return _HandleSizing(ptasks, (LPRECT)lParam);

    // this keeps our window from comming to the front on button down
    // instead, we activate the window on the up click
    // we only want this for the tree and the view window
    // (the view window does this itself)
    case WM_MOUSEACTIVATE: {
        POINT pt;
        RECT rc;

        GetCursorPos(&pt);
        GetWindowRect(ptasks->hwnd, &rc);

        if ((LOWORD(lParam) == HTCLIENT) && PtInRect(&rc, pt))
            return MA_NOACTIVATE;
        else
            goto DoDefault;
    }

    case WM_SETFOCUS: 
        Task_OnSetFocus(ptasks);
        break;

    case WM_VSCROLL:
        return _HandleVScroll(ptasks, LOWORD(wParam), HIWORD(wParam));

    case WM_HSCROLL:
        return _HandleHScroll(ptasks, LOWORD(wParam), HIWORD(wParam));

    case WM_NOTIFY:
        return Task_HandleNotify(ptasks, (LPNMHDR)lParam);

    case WM_NCHITTEST:
        lres = DefWindowProc(hwnd, msg, wParam, lParam);
        if (lres == HTVSCROLL || lres == HTHSCROLL)
            return lres;
        else
            return HTTRANSPARENT;

    case WM_TIMER:
        switch (wParam) {
        case IDT_RECHECKRUDEAPP:
            Tray_HandleFullScreenApp(Task_FindRudeApp(ptasks));
            KillTimer(hwnd, IDT_RECHECKRUDEAPP);
            break;
            
#ifdef USE_SYSMENU_TIMEOUT
        case IDT_SYSMENU:
            _HandleSysMenuTimeout(ptasks);
            break;
#endif
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == SC_CLOSE)
        {
            BOOL fForce = GetKeyState(VK_CONTROL) & (1 << 16) ? TRUE : FALSE;
#ifdef WINNT
            EndTask(ptasks->hwndSysMenu, FALSE , fForce);
#else
            EndTask(ptasks->hwndSysMenu, 0, NULL, fForce ? 0 : ET_TRYTOKILLNICELY);
#endif
        }
#ifdef WINNT
        else if (LOWORD(wParam) == SC_MINIMIZE) {
            ShowWindow(ptasks->hwndSysMenu, SW_FORCEMINIMIZE);
        }
#endif
        break;

    case TM_POSTEDRCLICK:
        // wparam is handle to the apps window
        Task_FakeSystemMenu(ptasks, (HWND)wParam, (DWORD)lParam);
        break;

    case TM_TASKTAB:
        Task_TaskTab(ptasks, (int)wParam);
        break;

    case WM_CONTEXTMENU:
        if (SHRestricted(REST_NOTRAYCONTEXTMENU)) {
            break;
        }

        // if we didn't find an item to put the sys menu up for, then
        // pass on the WM_CONTExTMENU message
        if (!Task_PostFakeSystemMenu(ptasks, (DWORD)lParam))
            goto DoDefault;
        DebugMsg(DM_TRACE, TEXT("Task GOT CONTEXT MENU!"));
        break;

    case TM_WINDOWDESTROYED:
        Task_HandleOtherWindowDestroyed(ptasks, (HWND)lParam);
        break;

    case TM_SYSMENUCOUNT:
        return ptasks->iSysMenuCount;

    case TBC_VERIFYBUTTONHEIGHT:
        Task_VerifyButtonHeight(ptasks);
        break;
        
    case TBC_SETACTIVEALT:
        SetActiveAlt(ptasks, (HWND) lParam);
        break;

    default:
DoDefault:
        if ((ptasks != NULL) && (msg == ptasks->WM_ShellHook))
            return Task_HandleShellHook(ptasks, (int)wParam, lParam);
        else
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}


//---------------------------------------------------------------------------

const TCHAR c_szTaskSwClass[] = TEXT("MSTaskSwWClass");


BOOL CTasks_RegisterWindowClass()
{
    WNDCLASSEX  wc;

    ZeroMemory(&wc, SIZEOF(wc));
    wc.cbSize = SIZEOF(WNDCLASSEX);

    if (GetClassInfoEx(hinstCabinet, c_szTaskSwClass, &wc))
        return TRUE;

    wc.lpszClassName    = c_szTaskSwClass;
    wc.lpfnWndProc      = Task_WndProc;
    wc.cbWndExtra       = SIZEOF(PTasks);
    wc.hInstance        = hinstCabinet;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_3DFACE + 1);

    return RegisterClassEx(&wc);
}


BOOL Tasks_Create(HWND hwndParent)
{
    if (!CTasks_RegisterWindowClass())
        return FALSE;

    // this sets this->hwnd
    return BOOLFROMPTR(CreateWindowEx(0, c_szTaskSwClass, NULL,
                                WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                0, 0, 0, 0, hwndParent, NULL, hinstCabinet, (CTasks *)&g_tasks));

}

void SetWindowStyleBit(HWND hWnd, DWORD dwBit, DWORD dwValue)
{
    DWORD dwStyle;

    dwStyle = GetWindowLong(hWnd, GWL_STYLE);
    if ((dwStyle & dwBit) != dwValue) {
        dwStyle ^= dwBit;
        SetWindowLong(hWnd, GWL_STYLE, dwStyle);
    }
}

BOOL ShouldMinMax(HWND hwnd, BOOL fMin)
{
    DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
    if (IsWindowVisible(hwnd) &&
        (fMin ? !IsMinimized(hwnd) : !IsMaximized(hwnd)) && 
        IsWindowEnabled(hwnd)) {
        if (dwStyle & (fMin ? WS_MINIMIZEBOX : WS_MAXIMIZEBOX)) {
            if ((dwStyle & (WS_CAPTION | WS_SYSMENU)) == (WS_CAPTION | WS_SYSMENU)) {
                HMENU hmenu = GetSystemMenu(hwnd, FALSE);
                if (hmenu) {
                    // is there a sys menu and is the sc_min/maximize part enabled?
                    if (!(GetMenuState(hmenu, (fMin ? SC_MINIMIZE : SC_MAXIMIZE), MF_BYCOMMAND) & MF_DISABLED)) {
                        return TRUE;
                    }
                }
            } else {
                return TRUE;
            }
        }
    }
    return FALSE;
}

void SaveWindowPositions(UINT idRes);

BOOL CanMinimizeAll(HWND hwndView)
{
    PTasks ptasks;
    int i;

    ASSERT(IS_VALID_HANDLE(hwndView, WND));

    ptasks = GetWindowPtr(hwndView, 0);

    ASSERT(IS_VALID_READ_PTR(ptasks, CTasks));

    for ( i = TabCtrl_GetItemCount(ptasks->hwndTab) -1; i >= 0; i--)
    {
        HWND hwnd = TabCtrl_GetItemHwnd(ptasks->hwndTab, i);
        if (ShouldMinMax(hwnd, TRUE) || (TabCtrl_GetItemFlags(ptasks->hwndTab, i) & TIF_EVERACTIVEALT))
            return TRUE;
    }

    return FALSE;
}

void CheckWindowPositions();

DWORD MinimizeAllThread(LPVOID lpv)
{
    HWND hwndView = (HWND)lpv;
    LONG iAnimate;
    ANIMATIONINFO ami;
    int i;
    PTasks ptasks;

    if (IsWindow(hwndView))
        ptasks = GetWindowPtr(hwndView, 0);
    else
        ptasks = 0;

    if (!ptasks)
        return 0;

    // turn off animiations during this
    ami.cbSize = SIZEOF(ANIMATIONINFO);
    SystemParametersInfo(SPI_GETANIMATION, SIZEOF(ami), &ami, FALSE);
    iAnimate = ami.iMinAnimate;
    ami.iMinAnimate = FALSE;
    SystemParametersInfo(SPI_SETANIMATION, SIZEOF(ami), &ami, FALSE);

    //
    //EnumWindows(MinimizeEnumProc, 0);
    // go through the tab control and minimize them.
    // don't do enumwindows because we only want to minimize windows
    // that are restorable via the tray

    for ( i = TabCtrl_GetItemCount(ptasks->hwndTab) -1; i >= 0; i--)
    {
        // we do the whole minimize on its own thread, so we don't do the showwindow
        // async.  this allows animation to be off for the full minimize.
        HWND hwnd = TabCtrl_GetItemHwnd(ptasks->hwndTab, i);
        if (ShouldMinMax(hwnd, TRUE)) {
            TraceMsg(TF_TRAY, "mat: hwnd=0x%x send SW_SHOWMINNOACT", hwnd);
            ShowWindow(hwnd, SW_SHOWMINNOACTIVE);
        }
        else if (TabCtrl_GetItemFlags(ptasks->hwndTab, i) & TIF_EVERACTIVEALT) {
            TraceMsg(TF_TRAY, "mat: add hwnd=0x%x send SC_MIN", hwnd);
            SHAllowSetForegroundWindow(hwnd);
            SendMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, -1);
        }
        else {
            TraceMsg(TF_TRAY, "mat: hwnd=0x%x skip min", hwnd);
        }
    }

    CheckWindowPositions();

    // restore animations  state
    ami.iMinAnimate = iAnimate;
    SystemParametersInfo(SPI_SETANIMATION, SIZEOF(ami), &ami, FALSE);
    return 0;
}

void MinimizeAll(HWND hwndView) 
{
    // might want to move this into MinimizeAllThread (to match
    // CheckWindowPositions).  but what if CreateThread fails?
    SaveWindowPositions(IDS_MINIMIZEALL);

    SHCreateThread(MinimizeAllThread, hwndView, 0, NULL);
}


int Task_HitTest(HWND hwndTask, POINTL ptl)
{
    PTasks ptasks = GetWindowPtr(hwndTask, 0);
    if (ptasks)
    {
        TC_HITTESTINFO hitinfo = { {ptl.x, ptl.y}, TCHT_ONITEM };
        ScreenToClient(ptasks->hwndTab, &hitinfo.pt);
        return TabCtrl_HitTest(ptasks->hwndTab, &hitinfo);
    }

    return -1;
}

void Task_SetCurSel(HWND hwndTask, int i)
{
    PTasks ptasks = GetWindowPtr(hwndTask, 0);
    if (ptasks)
    {
        TabCtrl_SetCurSel(ptasks->hwndTab, i);
        Task_SwitchToSelection(ptasks);
    }
}

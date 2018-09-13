//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       taskman.c
//
//  Contents:   Windows NT task manager for the shell
//
//  History:    6-05-95   davepl   Created
//
//--------------------------------------------------------------------------


#include "cabinet.h"
#pragma hdrstop

#include "taskres.h"

//
// Globals
//

extern HINSTANCE hInst;
HANDLE g_hHeap;
HWND   g_hwndTaskMan = NULL;

//
// Locally-defined prototypes
//

BOOL_PTR CALLBACK TaskManDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL            OnCommand       (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
BOOL            OnInitDialog    (HWND hwnd, HWND hwndFocus, LPARAM lParam);
void            OnShowWindow    (HWND hwnd, BOOL fShow, UINT status);
void            SetControlStates(HWND hWnd);
HRESULT         GetWindowList   (HWND hwndTasklist, HWND ** paHwnds, UINT * pcHwnds);

LRESULT CALLBACK TaskManStubWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

//+-------------------------------------------------------------------------
//
//  Function:   StartTaskMan
//
//  Synopsis:   Thread entry point for taskman creation
//
//  History:    6-05-95   davepl   Created
//
//  Notes:      Returns a success code when the taskman is subsequently
//              destroyed
//
//--------------------------------------------------------------------------

DWORD WINAPI StartTaskMan(LPVOID hWndParent)
{
    HWND hwndstub;
    WNDCLASS wndclass;
    HANDLE hThisThread;

    //
    // Make sure the taskman isn't already running
    //

    if (g_hwndTaskMan)
        return (DWORD) S_FALSE;

    g_hHeap = GetProcessHeap();             // Store away the process heap
    if (NULL == g_hHeap)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // Boost this thread's priority so that the taskman can come up
    // even when the system is heavily loaded (like under stress).
    //

    hThisThread = GetCurrentThread();
    if (NULL == hThisThread)
    {
        return (DWORD)E_FAIL;
    }

    SetThreadPriority(hThisThread, HIGH_PRIORITY_CLASS);

    if (!GetClassInfo(hinstCabinet, TEXT("TaskManStubWndClass"), &wndclass))
    {
        ZeroMemory(&wndclass, SIZEOF(wndclass));
        //wndclass.style         = 0;
        wndclass.lpfnWndProc   = TaskManStubWndProc;
        //wndclass.cbClsExtra    = 0;
        //wndclass.cbWndExtra    = 0;
        wndclass.hInstance     = hinstCabinet;
        //wndclass.hIcon         = NULL;
        wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
        wndclass.hbrBackground = GetStockObject (WHITE_BRUSH);
        //wndclass.lpszMenuName  = NULL;
        wndclass.lpszClassName = TEXT("TaskManStubWndClass");

        if (!RegisterClass(&wndclass))
            return (DWORD)HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // Create the stub window
    //

    if (hwndstub = CreateWindow(TEXT("TaskManStubWndClass"),            // class
                                TEXT("TaskManStubWndTitle"),            // title
                                WS_POPUP,                  // style
                                10, 10, 100, 100,          // pos & size
                                NULL,                      // parent
                                NULL,                      // menu
                                hinstCabinet,              // instance
                                NULL))                     // window data
    {
        MSG msg;
        HWND hDlg;
        if (0 == (hDlg = CreateDialog(hinstCabinet,
                              MAKEINTRESOURCE(IDD_TASKMAN),
                              hwndstub,
                              TaskManDlgProc)))
        {
            DWORD dwError = HRESULT_FROM_WIN32(GetLastError());
            PostMessage(hwndstub, WM_CLOSE, 0, 0);
            return dwError;
        }

        //
        // Churn, churn, churn...
        //

        while (GetMessage(&msg, NULL, 0, 0))
        {
            if (FALSE == IsDialogMessage(hDlg, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
    return (DWORD)HRESULT_FROM_WIN32(GetLastError());
}

//+-------------------------------------------------------------------------
//
//  Function:   SetDefButton
//
//  Synopsis:   Sets the default button in a dialog
//
//  Arguments:  [hwndDlg]  --  Dialog
//              [idButton] --  Button to make the default
//
//  History:    6-05-95   davepl   NT Port
//
//  Notes:      We use this to mark the SWITCHTO button as default when
//              working with the task listview, so that when enter is
//              pressed it acts as if SWITCHTO was selected (got to be
//              and easier way...)
//
//--------------------------------------------------------------------------

VOID SetDefButton(HWND hwndDlg, INT  idButton)
{
    LRESULT lr;

    if (HIWORD(lr = SendMessage(hwndDlg, DM_GETDEFID, 0, 0)) == DC_HASDEFID)
    {
        HWND hwndOldDefButton = GetDlgItem(hwndDlg, LOWORD(lr));
        if (hwndOldDefButton)
        {
            SendMessage (hwndOldDefButton,
                         BM_SETSTYLE,
                         MAKEWPARAM(BS_PUSHBUTTON, 0),
                         MAKELPARAM(TRUE, 0));
        }
    }

    SendMessage( hwndDlg, DM_SETDEFID, idButton, 0L );
    SendMessage( GetDlgItem(hwndDlg, idButton),
                 BM_SETSTYLE,
                 MAKEWPARAM( BS_DEFPUSHBUTTON, 0 ),
                 MAKELPARAM( TRUE, 0 ));
}


//+-------------------------------------------------------------------------
//
//  Function:   TaskManDlgProc
//
//  Synopsis:   Wndproc for the taskman dialog
//
//  History:    6-05-95   davepl   NT Port
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL_PTR CALLBACK TaskManDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    INSTRUMENT_WNDPROC(SHCNFI_TASKMAN_DLGPROC, hWnd, msg, wParam, lParam);

    switch(msg)
    {
        HANDLE_MSG(hWnd, WM_INITDIALOG,  OnInitDialog);
        HANDLE_MSG(hWnd, WM_COMMAND,     OnCommand);
        HANDLE_MSG(hWnd, WM_SHOWWINDOW,  OnShowWindow);
    }
    return 0;
}

//+-------------------------------------------------------------------------
//
//  Function:   OnInitDialog
//
//  Synopsis:   Caches the process heap.  This could use the shell allocator,
//              but I'm trying to keep the code as separate as possible
//              at this point
//
//  History:    6-05-95   davepl   NT Port
//
//  Notes:      Also sets the global handle to indicate that the taskman
//              is up and running.
//
//              BUGBUG (Davepl) Potential race condition here?
//
//--------------------------------------------------------------------------

BOOL OnInitDialog(HWND hWnd, HWND hwndFocus, LPARAM lParam)
{
    ENTERCRITICAL;
    if (g_hwndTaskMan)
    {
        LEAVECRITICAL;
        PostMessage(hWnd, WM_CLOSE, 0, 0);
        return FALSE;
    }
    g_hwndTaskMan = hWnd;
    LEAVECRITICAL;

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:   OnCommand
//
//  Synopsis:   Processes all WM_COMMAND messages from the dialog,
//              typically resulting from user interaction
//
//  History:    6-05-95   davepl   NT Port
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL OnCommand(HWND hWnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch(id)
    {
        case IDCANCEL:
        {
            ShowWindow(hWnd, SW_HIDE);
            break;
        }

        case IDC_SWITCHTO:
        {
            HWND * aHwnds;
            UINT   cHwnds;
            HRESULT hr = GetWindowList(GetDlgItem(hWnd, IDC_TASKLIST), &aHwnds, &cHwnds);
            if (S_OK == hr)
            {
                if (cHwnds)
                {
                    ShowWindow(aHwnds[0], SW_RESTORE);
                    ShowWindow(hWnd, SW_HIDE);
                    SetForegroundWindow(aHwnds[0]);
                    HeapFree(g_hHeap, 0, aHwnds);
                }
            }
            else if (S_FALSE == hr)
            {
                HeapFree(g_hHeap, 0, aHwnds);
            }

            break;
        }

        case IDC_REFRESH:
        {
            PostMessage(hWnd, WM_SHOWWINDOW, TRUE, 0);
            break;
        }

        // Handle tiling, minimizing, and cascading

        case IDC_MINIMIZE:
        {
            HWND * aHwnds;
            UINT   cHwnds;

            if (SUCCEEDED(GetWindowList(GetDlgItem(hWnd, IDC_TASKLIST), &aHwnds, &cHwnds) && cHwnds))
            {
                UINT i;
                for(i = 0; i < cHwnds; i++)
                {
                    ShowWindow(aHwnds[i], SW_SHOWMINNOACTIVE);
                }
                HeapFree(g_hHeap, 0, aHwnds);
                SetForegroundWindow(hWnd);
            }
            break;
        }

        case IDC_ENDTASK:
        {
            HWND * aHwnds;
            UINT   cHwnds;
            HRESULT hr = GetWindowList(GetDlgItem(hWnd, IDC_TASKLIST), &aHwnds, &cHwnds);

            if (S_OK == hr)
            {
                UINT i;
                BOOL fForce = GetKeyState(VK_CONTROL) & ( 1 << 16) ? TRUE : FALSE;
                for(i = 0; i < cHwnds; i++)
                {
                    SetActiveWindow(aHwnds[i]);
                    EndTask(aHwnds[i], FALSE, fForce);
                }

                // If we've removed an app, we need to refresh the dialog, which we can do
                // by "pretending" we've just been opened

                if (cHwnds)
                {
                    PostMessage(hWnd, WM_SHOWWINDOW, TRUE, 0);
                }

                HeapFree(g_hHeap, 0, aHwnds);
                SetForegroundWindow(hWnd);
            }
            else if (S_FALSE == hr)             // Nothing select, so free the list and
            {                                   // do nothing
                HeapFree(g_hHeap, 0, aHwnds);
            }

            break;

        }

        case IDC_CASCADE:
        case IDC_TILEVERTICALLY:
        case IDC_TILEHORIZONTALLY:
        {
            HWND * aHwnds;
            UINT   cHwnds;

            if (SUCCEEDED(GetWindowList(GetDlgItem(hWnd, IDC_TASKLIST), &aHwnds, &cHwnds) && cHwnds))
            {
                UINT i;
                for(i = 0; i < cHwnds; i++)
                {
                    ShowWindow(aHwnds[i], SW_RESTORE);
                    SetForegroundWindow(aHwnds[i]);
                }

                if (id == IDC_CASCADE)
                {
                    CascadeWindows(GetDesktopWindow(),
                                   0,
                                   NULL,
                                   cHwnds,
                                   aHwnds);
                }
                else
                {
                    TileWindows(GetDesktopWindow(),
                                id == IDC_TILEVERTICALLY ? MDITILE_VERTICAL : MDITILE_HORIZONTAL,
                                NULL,
                                cHwnds,
                                aHwnds);
                }

                HeapFree(g_hHeap, 0, aHwnds);
                SetForegroundWindow(hWnd);
            }
            break;
        }

        // Listbox messages

        case IDC_TASKLIST:
        {
            switch(codeNotify)
            {
                case LBN_SELCHANGE:
                {
                   SetControlStates(hWnd);
                   break;
                }

                case LBN_DBLCLK:
                {
                    HWND * aHwnds;
                    UINT   cHwnds;
                    HRESULT hr = GetWindowList(GetDlgItem(hWnd, IDC_TASKLIST), &aHwnds, &cHwnds);

                    if (S_OK == hr)
                    {
                        if (cHwnds)
                        {
                            ShowWindow(aHwnds[0], SW_RESTORE);
                            SetForegroundWindow(aHwnds[0]);
                            HeapFree(g_hHeap, 0, aHwnds);
                            ShowWindow(hWnd, SW_HIDE);
                        }
                    }
                    else if (S_FALSE == hr)
                    {
                       HeapFree(g_hHeap, 0, aHwnds);
                    }

                    break;
                }
            }

            // Always leave the switchto button as the default button so that if ENTER
            // is pressed, we'll do switchto

            SetDefButton(hWnd, IDC_SWITCHTO);

            break;
        }

    }

    return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Function:   SetControlStates
//
//  Synopsis:   Enables/Disables the appropriate dialog controls depending on
//              whether or not anything is currently selected in the dialog
//
//  History:    6-05-95   davepl   NT Port
//
//  Notes:
//
//--------------------------------------------------------------------------

//
// This is the list of items that need to be updated
//

static const UINT aidItems[] =
{
//    IDC_CASCADE,
//    IDC_TILEHORIZONTALLY,
//    IDC_TILEVERTICALLY,
    IDC_ENDTASK,
    IDC_IDLE,
//    IDC_MINIMIZE,
    IDC_SWITCHTO,
};

void SetControlStates(HWND hWnd)
{
    UINT cSelected;
    UINT i;
    HWND hListBox = GetDlgItem(hWnd, IDC_TASKLIST);
    if (NULL == hListBox)
    {
        return;
    }

    cSelected = ListBox_GetSelCount(hListBox);

    for (i=0; i < ARRAYSIZE(aidItems); i++)
    {
        EnableWindow(GetDlgItem(hWnd, aidItems[i]), cSelected > 0 );
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   OnShowWindow
//
//  Synopsis:   Enumerates the active toplevel windows and adds an entry for
//              each to the task list listbox
//
//  History:    6-05-95   davepl   NT Port
//
//  Notes:      Also called to refresh the listbox when tasks are killed
//
//--------------------------------------------------------------------------

void OnShowWindow(HWND hWnd, BOOL fShow, UINT status)
{
    HWND hwndNext;
    HWND hwndLB = GetDlgItem(hWnd, IDC_TASKLIST);

    //
    // Empty the listbox...
    //

    while ((int)SendMessage(hwndLB, LB_DELETESTRING, 0, 0) != LB_ERR)
        NULL;

    //
    // Enumerate top-level windows (what we call tasks around here)
    //

    hwndNext = GetWindow(hWnd, GW_HWNDFIRST);
    while (hwndNext)
    {
        TCHAR szTitle[64] = { TEXT('\0') };
        GetWindowText(hwndNext, szTitle, ARRAYSIZE(szTitle));

        //
        // Criteria for a window to be shown in the tasklist:
        //
        // 1) Must be top level
        // 2) Must be unowned
        // 3) Must not be the tasklist or "Program Manager" (Explorer)
        // 4) Must be visible
        //

        if ((hwndNext != hWnd) &&
            (IsWindowVisible(hwndNext)) &&
            (lstrcmp(szTitle, TEXT("Program Manager"))) &&
            (!GetWindow(hwndNext, GW_OWNER)))
        {
            TCHAR szWindowName[MAX_PATH];
            if (GetWindowText(hwndNext, szWindowName, ARRAYSIZE(szWindowName)))
            {
                // Add the task name to the list box, and store its associated
                // window as the item data of the next listbox entry

                int iIndex = (int)SendMessage(hwndLB, LB_ADDSTRING, 0,
                                              (LPARAM)(LPTSTR)szWindowName);
                SendMessage(hwndLB, LB_SETITEMDATA, iIndex, (LPARAM)hwndNext);
            }
        }

        hwndNext = GetWindow(hwndNext, GW_HWNDNEXT);
    }

    // Select the first item in the list box, and if that goes OK, enable
    // the buttons on the dialog (otherwise, disable them as if no selection
    // had been made)

    SendMessage(hwndLB, LB_SETCURSEL, 0, 0);
    SetControlStates(hWnd);
    SetForegroundWindow(hWnd);

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetWindowList
//
//  Synopsis:   Returns an array of HWNDS corresponding to the app selected
//              in the listbox.  Return S_OK for success, S_FALSE if nothing
//              is selected, and failure codes otherwise.  If nothing is
//              selected, we return a list of all windows.
//
//  History:    6-05-95   davepl   NT Port
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT GetWindowList(HWND hwndTasklist, HWND ** paHwnds, UINT * pcHwnds)
{
    BOOL fSucceeded;
    INT * aItems = NULL;
    UINT cSelected = ListBox_GetSelCount(hwndTasklist);

    // NULL out in case of error

    *paHwnds = NULL;
    *pcHwnds = 0;

    // If nothing is selected, we return the list of all windows

    if (0 == cSelected)
    {
        UINT i;
        UINT cItems = ListBox_GetCount(hwndTasklist);
        HWND * aHwnds;

        if (cItems == 0)
        {
            return E_FAIL;                  // Nothing even in listbox, call it an "error"
        }

        aHwnds = (HWND *) HeapAlloc(g_hHeap, 0, cItems * sizeof(HWND));
        if (NULL == aHwnds)
        {
            return E_OUTOFMEMORY;
        }

        for (i = 0; i < cItems; i++)
        {
            HWND hwnd = (HWND) ListBox_GetItemData(hwndTasklist, i);
            if (hwnd != (HWND) LB_ERR)
            {
                aHwnds[i] = hwnd;
            }
            else
            {
                HeapFree(g_hHeap, 0, aHwnds);
                return E_FAIL;
            }
        }

        *paHwnds = aHwnds;
        *pcHwnds = cItems;

        return S_FALSE;     // S_FALSE == nothing selected, but got full list OK
    }

    //
    // Grab the table of selected items and process it
    //

    fSucceeded = TRUE;

    aItems = (INT *) HeapAlloc(g_hHeap, 0, cSelected * sizeof(INT));
    if (NULL == aItems)
    {
        return E_OUTOFMEMORY;
    }

    if (LB_ERR != ListBox_GetSelItems(hwndTasklist, cSelected, aItems))
    {
        UINT i;

        // Walk through the list and replace the indexes with the
        // actual HWNDs associated with that listbox entry.  We will
        // reuse this index list and pass it back as the HWND list

        for (i = 0; i < cSelected; i++)
        {
            // BUGBUG Assumes sizeof(HWND) == sizeof(INT)

            aItems[i] = (INT) ListBox_GetItemData(hwndTasklist, aItems[i]);
            if (LB_ERR == aItems[i])
            {
                fSucceeded = FALSE;
                break;
            }
        }

        // If everything went OK, hand off this list, otherwise free it

        if (fSucceeded)
        {
            *paHwnds = (HWND *) aItems;
            *pcHwnds = cSelected;
            return S_OK;
        }
        else
        {
            HeapFree(g_hHeap, 0, aItems);
            return E_FAIL;
        }
    }
    else
    {
        HeapFree(g_hHeap, 0, aItems);
        return E_FAIL;
    }
}

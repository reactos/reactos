#include "common.h"
#include "diskutil.h"
#include "msprintf.h"
#include "drivlist.h" 
#include "miniicon.h"

#define  cxBITMAP  18
#define  cyBITMAP  18
#define  cbRESOURCE 256


/*
 * VARIABLES __________________________________________________________________
 *
 */
FARPROC lpfnComboMain;


/*
 * PROTOTYPES _________________________________________________________________
 *
 */

LONG APIENTRY 
DriveListProc(
    HWND hWnd, 
    UINT Message, 
    WPARAM wParam, 
    LPARAM lParam
    );

BOOL CALLBACK 
DriveComboProc(
    HWND hWnd, 
    UINT Message, 
    WPARAM wParam, 
    LPARAM lParam
    );

void 
GoSelectDrive(
    HWND hWnd, 
    drenum dre
    );
    
void 
DrawDriveItem(
    DRAWITEMSTRUCT *lpdi
    );

/*
 * ROUTINES ___________________________________________________________________
 *
 */

/*** RegisterDriveList - Registers the DriveList class (always do this!!!)
 *
 */
BOOL 
RegisterDriveList(
    HINSTANCE hInst
    )
{
    WNDCLASS wc;

    wc.style       = CS_NOCLOSE;
    wc.lpfnWndProc = DriveListProc;	// Standard callback function
    wc.cbClsExtra  = 0;	            // No per-class extra data
    wc.cbWndExtra  = 12;	        // We store proc addresses here
    wc.hInstance   = hInst;	        // Registering application

    wc.hIcon = NULL;
    wc.hCursor = LoadCursor (NULL, MAKEINTRESOURCE(IDC_ARROW));
    wc.hbrBackground = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

    wc.lpszMenuName  = NULL;	    // Name of menu resource in .RC file
    wc.lpszClassName = szDriveListCLASS;	// Name used in call to CreateWindow

    RegisterClass(&wc);

    return TRUE;
}

/*** ExitDriveList - Frees memory allocated for the DriveList class
 *
 */
void 
ExitDriveList(
    void
    )
{
}

/*** DriveListProc - Procedure for handing a DriveList
 *
 */
LONG APIENTRY 
DriveListProc(
    HWND hWnd, 
    UINT Message, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    MEASUREITEMSTRUCT *lpmi;
    CREATESTRUCT      *lpcs;
    HWND              hCombo;
    PAINTSTRUCT       ps;
    long              l;
    RECT              r;

    switch (Message)
    {
        case  WM_CREATE:
            lpcs = (CREATESTRUCT *)lParam;

            if (lpcs->style & STYLE_LISTBOX)
            {
                MiDebugMsg((0, "CreateWindowEx(ListBox)"));
                hCombo = CreateWindowEx (
                                //EDGE_SUNKEN | WS_EX_EDGEMASK,
                                EDGE_SUNKEN,
                                "ListBox",
                                "",
                                LBS_OWNERDRAWFIXED | LBS_HASSTRINGS |
                                WS_VSCROLL |
                                WS_VISIBLE | WS_CHILD | CBS_SIMPLE,
                                0,
                                0,
                                lpcs->cx,
                                lpcs->cy *5,
                                hWnd,
                                NULL,
                                lpcs->hInstance,
                                NULL
                                );
            }
            else
            {
                MiDebugMsg((0, "CreateWindowEx(ComboBox)"));
                hCombo = CreateWindow (
                                "ComboBox",
                                "",
                                LBS_USETABSTOPS |
                                CBS_OWNERDRAWFIXED | CBS_HASSTRINGS |
                                CBS_DROPDOWNLIST | WS_VSCROLL |
                                WS_VISIBLE | WS_CHILD | CBS_SIMPLE,
                                0,
                                0,
                                lpcs->cx,
                                lpcs->cy *4,	// NUMBER LINES in DROP DOWN
                                hWnd,
                                NULL,
                                lpcs->hInstance,
                                NULL
                                );
            }

            SetWindowLong(hWnd, DL_COMBOWND, (long)hCombo);
            SetWindowLong(hWnd, DL_UPDATES, 0L);

            if (hCombo == NULL)
                return -1;

            //
            //Subclass the combobox
            //
            lpfnComboMain = (FARPROC)GetWindowLong(hCombo, GWL_WNDPROC);
            SetWindowLong(hCombo, GWL_WNDPROC, (LONG)DriveComboProc);

            //wproc = (WNDPROC)GetWindowLong (hCombo, GWL_WNDPROC);
            //SetWindowLong(hWnd, DL_COMBOPROC, (long)wproc);
            //wproc = (WNDPROC)DriveComboProc;
            //SetWindowLong(hCombo, GWL_WNDPROC, (long)wproc);

            return 0;
            break;

        case  WM_DESTROY:  
            hCombo = (HWND)GetWindowLong(hWnd, DL_COMBOWND);

            if (hCombo != NULL)
            {
                SetWindowLong (hCombo, GWL_WNDPROC, (long)lpfnComboMain);

                DestroyWindow(hCombo);
                SetWindowLong(hWnd, DL_COMBOWND, 0L);
            }
            return 0;
            break;

        case WM_HELP:
            ((LPHELPINFO) lParam)->hItemHandle = hWnd;
            return DefWindowProc (hWnd, Message, wParam, lParam);

        case WM_CONTEXTMENU:
            return DefWindowProc (hWnd, Message, (WPARAM) hWnd, lParam);

        case  WM_PAINT:
            BeginPaint (hWnd, &ps);
            EndPaint (hWnd, &ps);
            break;

        case  WM_COMMAND:
            if ((HIWORD(lParam) == CBN_SELCHANGE) || 
                (HIWORD(lParam) == LBN_SELCHANGE))
            {
                wParam = GetWindowLong(hWnd, GWL_ID);
                SendMessage(GetParent(hWnd),Message,wParam,lParam);
            }
            break;

        case  WM_MEASUREITEM:
            lpmi = (MEASUREITEMSTRUCT *)lParam;
            lpmi->itemWidth = cxBITMAP;
            lpmi->itemHeight = cyBITMAP +2;
            break;

        case  WM_DRAWITEM:
            DrawDriveItem((DRAWITEMSTRUCT *)lParam);
            return 0;
            break;

        case  DL_UPDATESBAD:
            l = GetWindowLong(hWnd, DL_UPDATES);
            SetWindowLong(hWnd, DL_UPDATES, l+1);
            break;

        case  DL_UPDATESOKAY:
            l = GetWindowLong(hWnd, DL_UPDATES);
            SetWindowLong(hWnd, DL_UPDATES, l-1);

            if (l == 1)
            {
                GetClientRect(hWnd, &r);
                InvalidateRect(hWnd, &r, FALSE);
                UpdateWindow(hWnd);
            }
            break;
    }

    /*
    if ((hCombo = (HWND)GetWindowLong(hWnd, DL_COMBOWND)) == NULL)
        return -1;

    if ((wproc = (WNDPROC)GetWindowLong(hCombo, GWL_WNDPROC)) == NULL)
        return -1;

    if (Message > WM_USER)
    {
        return CallWindowProc(wproc, hCombo, Message, wParam, lParam);
    }

    if (Message == WM_SETFONT || Message == WM_SETFOCUS || Message == WM_KILLFOCUS)
    {
        CallWindowProc(wproc, hCombo, Message, wParam, lParam);
    }
    if (Message == WM_GETDLGCODE)
        return (DLGC_WANTCHARS | DLGC_WANTARROWS);

    if (Message == WM_KEYDOWN)
    {
        if ((wParam == VK_UP) || (wParam == VK_DOWN) ||
            (wParam == VK_LEFT) || (wParam == VK_RIGHT) ||
            (wParam == VK_HOME) || (wParam == VK_END) ||
            (wParam == VK_F4))
        {
            CallWindowProc (wproc, hCombo, Message, wParam, lParam);
            Message = WM_CHAR;
        }
    }

    if (Message == WM_CHAR)
    {
        return CallWindowProc (wproc, hCombo, Message, wParam, lParam);
    }
    */

    return DefWindowProc (hWnd, Message, wParam, lParam);
}

/*** DriveComboProc - Procedure for handing a DriveList
 *
 */
BOOL CALLBACK 
DriveComboProc(
    HWND hCombo, 
    UINT Message, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    HWND      hWnd;

    if ((hWnd = GetParent(hCombo)) == NULL)
        return -1;

    if (Message == WM_GETDLGCODE)
        return (DLGC_WANTCHARS | DLGC_WANTARROWS);

    if (Message == WM_CHAR)
    {
        if (wParam >= 'A' && wParam <= 'Z')  GoSelectDrive(hWnd, (drenum)(wParam-'A'));
        if (wParam >= 'a' && wParam <= 'z')  GoSelectDrive(hWnd, (drenum)(wParam-'a'));
        return DefWindowProc (hWnd, Message, wParam, lParam);
    }

    return CallWindowProc((WNDPROC)lpfnComboMain, hCombo, Message, wParam, lParam);
}

/*** GoSelectDrive - Choose a new drive, based on a keypress
 *
 */
void 
GoSelectDrive(
    HWND hWnd, 
    drenum dre
    )
{
    HWND      hCombo;
    USHORT    nItems;
    USHORT    n;
    DWORD     data;
    WPARAM    wParam;
    LPARAM    lParam;
    BOOL      fList;

    if ((hCombo = (HWND)GetWindowLong(hWnd, DL_COMBOWND)) == NULL)
        return;

    fList = (GetWindowLong (hWnd, GWL_STYLE) & STYLE_LISTBOX) ? TRUE : FALSE;

    if (fList)
        nItems = (USHORT)CallWindowProc((WNDPROC)lpfnComboMain, hCombo, LB_GETCOUNT, 0, 0L);
    else
        nItems = (USHORT)CallWindowProc((WNDPROC)lpfnComboMain, hCombo, CB_GETCOUNT, 0, 0L);

    for (n = 0; n < nItems; n++)
    {
        if (fList)
            data = CallWindowProc ((WNDPROC)lpfnComboMain, hCombo, LB_GETITEMDATA, (WPARAM)n, 0L);
        else
            data = CallWindowProc ((WNDPROC)lpfnComboMain, hCombo, CB_GETITEMDATA, (WPARAM)n, 0L);

        if (LOBYTE(data) == dre)
        {
            if (fList)
                CallWindowProc ((WNDPROC)lpfnComboMain, hCombo, LB_SETCURSEL, (WPARAM)n, 0L);
            else
                CallWindowProc ((WNDPROC)lpfnComboMain, hCombo, CB_SETCURSEL, (WPARAM)n, 0L);

            wParam = (WPARAM)GetWindowLong(hWnd, GWL_ID);
            lParam = (LPARAM)MAKELONG((WORD)hCombo, (WORD)CBN_SELCHANGE);
            SendMessage(GetParent(hWnd), WM_COMMAND, wParam, lParam);
            return;
        }
    }

    MessageBeep (MB_ICONEXCLAMATION);
}

void 
DrawDriveItem(
    DRAWITEMSTRUCT *lpdi
    )
{
    TCHAR    buf[cbRESOURCE] = "";
    HWND     hWnd;
    int      x, y;
    HICON    hi;
    COLORREF clrBk, clrFg;
    HBRUSH   hb;
    int      dre;
    long     l;

    if (lpdi->itemAction & ODA_FOCUS)
        return;

    dre = (int)LOBYTE(lpdi->itemData);
    MiDebugMsg((0, "DrawDriveItem for dre %d", dre));
    if ((dre != Drive_INV) && ((dre < (int)Drive_A) || (dre > (int)Drive_Z)))
        return;

    hWnd = GetParent (lpdi->hwndItem);

    if (GetWindowLong (hWnd, DL_UPDATES) != 0)
        return;

    l = GetWindowLong (hWnd, GWL_STYLE);
    if (l & STYLE_LISTBOX)
        SendMessage (hWnd, LB_GETTEXT,  (WPARAM)lpdi->itemID,(LPARAM)(LPTSTR)buf);
    else
        SendMessage (hWnd, CB_GETLBTEXT,(WPARAM)lpdi->itemID,(LPARAM)(LPTSTR)buf);

    x = lpdi->rcItem.left;
    y = lpdi->rcItem.top;

    hi = GetDriveIcon((drenum)dre);

    if (lpdi->itemState & ODS_SELECTED)
    {
        clrBk = SetBkColor   (lpdi->hDC, GetSysColor(COLOR_HIGHLIGHT));
        clrFg = SetTextColor (lpdi->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
    }
    else
    {
        clrBk = SetBkColor   (lpdi->hDC, GetSysColor(COLOR_WINDOW));
        clrFg = SetTextColor (lpdi->hDC, GetSysColor(COLOR_WINDOWTEXT));
    }

    hb = CreateSolidBrush (GetBkColor(lpdi->hDC));
    FillRect (lpdi->hDC, &lpdi->rcItem, hb);
    DeleteObject (hb);

    TabbedTextOut (lpdi->hDC, x +((hi == NULL) ? 1 : 21), y+3,
                   buf, _fstrlen(buf), 0, NULL, 0);

    SetBkColor (lpdi->hDC, clrBk);
    SetTextColor (lpdi->hDC, clrFg);

    if (hi == NULL)
        return;

    DrawIconEx(lpdi->hDC, x+1, y+1, hi,
        MINIY, MINIX, 0, NULL, DI_NORMAL);
}

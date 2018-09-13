/////////////////////////////////////////////////////////////////////////////
//  LB.C
//
//  Simple owner draw list box modified for Row major ordering for
//  displaying Control Panel ICONs across rows first before starting
//  another row.  This is different from how USER does their list
//  boxes. They go in Column major order when allowing for multiple
//  columns.
//
//  History:
//   01/15/89 toddla     Created
//  Tue Jan 15 1991 -by- MichaelE
//         Added generating WM_CHARTOITEM message.
//  10:30 on Tues  04 Feb 1992  -by-  Steve Cathcart   [stevecat]
//        Updated code to latest Win 3.1 sources
//  18:30 on Fri  24 Jun 1994  -by-  Steve Cathcart   [stevecat]
//        Changed to work better with Window Full Drag on
//
//  Copyright (C) 1990-1994 Microsoft Corporation
//
/////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include "lb.h"

#define BOUND(x,min,max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
#define SWAP(x,y)   ((x)^=(y)^=(x)^=(y))
#define MIN(x,y) (((x) <= (y)) : x ? y)
#define ALIGNB(x)   (((x) + 7) & ~0x07)

#define MAX_LB 200          // max items in a list-box for now

#define GWL_HANDLE 0

#define LONG2POINT(l, pt)   (pt.y = (int) HIWORD(l),  pt.x = (int) LOWORD(l))

#define EXPORT  FAR PASCAL _export
#define PUBLIC  FAR PASCAL

#define LBH(hwnd)        (HANDLE)(GetWindowLong(hwnd,GWL_HANDLE))
#define LBP(hwnd)        ((PLB)LBH(hwnd))

LRESULT APIENTRY lbWndProc (HWND hwnd, UINT msg, WPARAM wParam, LONG lParam);

#define SCROLLWIDTH (GetSystemMetrics(SM_CXVSCROLL) - GetSystemMetrics(SM_CXBORDER))

/*--------------------------------------------------------------------------*\
|   lbInit (hPrev,hInst )                                                     |
|                                                                            |
|   Description:                                                             |
|       This is called when the application is first loaded into             |
|    memory.  It performs all initialization.                 |
|                                                                            |
|   Arguments:                                                               |
|    hPrev       instance handle of previous instance              |
|    hInst       instance handle of current instance                 |
|                                                                            |
|   Returns:                                                                 |
|       TRUE if successful, FALSE if not                                     |
|                                                                            |
\*--------------------------------------------------------------------------*/
BOOL PUBLIC lbInit (HANDLE hInst)
{
    WNDCLASS    cls;

    cls.hCursor        = LoadCursor (NULL, IDC_ARROW);
    cls.hIcon          = NULL;
    cls.lpszMenuName   = NULL;
    cls.lpszClassName  = LBCLASS;
    cls.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    cls.hInstance      = hInst;
// [stevecat] - remove the CS_HREDRAW style for FULL Drag
//    cls.style          = CS_HREDRAW | CS_DBLCLKS; // | CS_VREDRAW;
    cls.style          = CS_DBLCLKS; // CS_HREDRAW | CS_VREDRAW;
    cls.lpfnWndProc    = lbWndProc;
    cls.cbClsExtra     = 0;
    cls.cbWndExtra     = sizeof(HANDLE);

    RegisterClass(&cls);

    return TRUE;
}

LONG lbNotifyParent (PLB plb, UINT msg, DWORD wParam, LONG lParam)
{
    if(plb->hwndOwner)
        return SendMessage (plb->hwndOwner, msg, wParam, lParam);

    return 0L;
}

void lbCmdNotify (PLB plb, WORD wNotify)
{
    lbNotifyParent (plb, WM_COMMAND, MAKELONG(plb->id, wNotify), (LONG)plb->hwnd);
}

BOOL lbDeleteItem (PLB plb, int i)
{
    DELETEITEMSTRUCT di;

    if (!plb || i < 0 || i >= plb->nItems)
        return FALSE;

    di.CtlType  = ODT_LISTBOX;
    di.CtlID    = plb->id;
    di.itemID   = i;
    di.hwndItem = plb->hwnd;
    di.itemData = plb->lData[i];

    lbNotifyParent (plb, WM_DELETEITEM, 0, (LONG)&di);

    plb->nItems--;

    for (; i < plb->nItems; i++)
        plb->lData[i] = plb->lData[i+1];

    return TRUE;
}

void lbItemRect (PLB plb, int i, RECT* prc)
{
    if (!plb || !plb->nx || i < 0 || i >= plb->nItems)
        return;

    prc->left   = (i % plb->nx) * plb->dx;
    prc->right  = prc->left + plb->dx;

    prc->top    = (i / plb->nx) * plb->dy;
    prc->top   -= plb->dyScroll * plb->dy;
    prc->bottom = prc->top + plb->dy;
}

int lbItemFromPoint (PLB plb, POINT pt)
{
    int   i;

    pt.y += plb->dyScroll * plb->dy;

    if (!plb || !plb->dx || !plb->dy)
        return -1;

    if (pt.x > plb->dx * plb->nx)
        return -1;

    i  = (pt.x / plb->dx);
    i += (pt.y / plb->dy) * plb->nx;

    if (i < 0 || i >= plb->nItems)
        i = -1;

    return i;
}

BOOL lbDrawItem (PLB plb, HDC hdc, int i, int itemAction)
{
    DRAWITEMSTRUCT di;
    BOOL fGetDC;

    if (!plb || i < 0 || i >= plb->nItems)
        return FALSE;

    if (fGetDC = !hdc)
    {
        hdc = GetDC (plb->hwnd);
        lbNotifyParent (plb, WM_CTLCOLORLISTBOX, (DWORD)hdc, (LONG)plb->hwnd);
    }

    lbItemRect (plb, i, &di.rcItem);

    if (RectVisible (hdc, &di.rcItem))
    {
        di.CtlType      = ODT_LISTBOX;
        di.CtlID        = plb->id;
        di.itemID       = i;
        di.itemAction   = itemAction;
        di.itemState    = 0;
        di.hwndItem     = plb->hwnd;
        di.hDC          = hdc;
        di.itemData     = plb->lData[i];

        if (i == plb->iCurSel)
            di.itemState |= ODS_SELECTED;

        if (plb->flb & LB_FOCUS)
            di.itemState |= ODS_FOCUS;

        lbNotifyParent (plb, WM_DRAWITEM, 0, (LONG)&di);
    }

    if (fGetDC)
        ReleaseDC (plb->hwnd,hdc);

    return TRUE;
}

void GetRealClientRect (HWND hwnd, PRECT prc)
{
    DWORD dwStyle;

    dwStyle = GetWindowLong (hwnd, GWL_STYLE);
    GetClientRect (hwnd, prc);

#if 0
    if (dwStyle & WS_HSCROLL)
        prc->bottom += GetSystemMetrics (SM_CYHSCROLL);
#endif
    if (dwStyle & WS_VSCROLL)
        prc->right  += SCROLLWIDTH;
}

void lbCalcSizes (PLB plb, RECT *prc)
{
    plb->nx = prc->right / plb->dx;

    if (plb->nx == 0)
        plb->nx = 1;

    plb->ny = (plb->nItems + plb->nx-1) / plb->nx ;

    plb->nyScroll = plb->ny - (prc->bottom / plb->dy);

    if (plb->nyScroll < 0)
        plb->nyScroll = 0;
}

void lbSize (PLB plb)
{
    RECT rc;
    int  ix;

    if (!plb || !plb->dx)
        return;

    GetRealClientRect (plb->hwnd, &rc);

    //
    //  Save current number of items per row
    //

    ix = plb->nx;

    lbCalcSizes (plb, &rc);

    if (plb->nyScroll)
    {
        rc.right -= SCROLLWIDTH;
        lbCalcSizes (plb, &rc);
    }

    SetScrollRange (plb->hwnd, SB_VERT, 0, plb->nyScroll, TRUE);

    if (plb->dyScroll > plb->nyScroll)
    {
        plb->dyScroll = plb->nyScroll;
        InvalidateRect (plb->hwnd, NULL, TRUE);
        return;
    }

    //
    //  Force a redraw of listbox contents if horizontal size changes
    //  by enough to allow more or less icons per row.
    //

    if (ix != plb->nx)
    {
        InvalidateRect (plb->hwnd, NULL, TRUE);
    }
}

void lbPaint (PLB plb, HDC hdc)
{
    int    i;
    HBRUSH hbr;
    RECT   rc;

    if (!plb)
        return;

    GetClientRect (plb->hwnd, &rc);


    hbr = (HBRUSH)lbNotifyParent (plb, WM_CTLCOLORLISTBOX, (DWORD)hdc, (LONG)plb->hwnd);

    FillRect (hdc, &rc, hbr);

    for (i=0; i < plb->nItems; i++)
        lbDrawItem (plb, hdc, i, ODA_DRAWENTIRE);
}

void lbShowItem (PLB plb, int i)
{
    RECT rc;
    int  row;
    int  rows;

    GetClientRect (plb->hwnd, &rc);

    row = (i / plb->nx);
    rows= (rc.bottom / plb->dy);

    if (row < plb->dyScroll)
        SendMessage(plb->hwnd, WM_VSCROLL, MAKELONG(SB_THUMBPOSITION, row), (LONG)plb->hwnd);
    else if (row >= plb->dyScroll+rows)
        SendMessage(plb->hwnd, WM_VSCROLL, MAKELONG(SB_THUMBPOSITION, row-rows+1), (LONG)plb->hwnd);

}

BOOL lbCreate (HWND hwnd)
{
    MEASUREITEMSTRUCT mi;
    HANDLE h;
    PLB plb;

    h = LocalAlloc (LPTR, sizeof(LB) + MAX_LB * sizeof(LONG));
    if (!h)
        return FALSE;

    SetWindowLong (hwnd, GWL_HANDLE, (LONG)h);
    plb = LBP(hwnd);

    mi.CtlType     = ODT_LISTBOX;
    mi.CtlID       = plb->id;
    mi.itemID      = (UINT) (int) -1;
    mi.itemWidth   = 1;
    mi.itemHeight  = 1;
    mi.itemData    = 0l;

    plb->hwnd      = hwnd;
    plb->hwndOwner = GetParent (hwnd);
    plb->id        = GetWindowLong (hwnd, GWL_ID);

    //plb->nx        = 0;
    //plb->nItems    = 0;
    //plb->dyScroll  = 0;
    //plb->flb       = 0;
    //plb->iCurSel   = 0;

    lbNotifyParent (plb, WM_MEASUREITEM, 0, (LONG)&mi);

    plb->dx = mi.itemWidth;
    plb->dy = mi.itemHeight;

    SetScrollRange (plb->hwnd, SB_HORZ, 0, 0, FALSE);
    SetScrollRange (plb->hwnd, SB_VERT, 0, 0, FALSE);

    return TRUE;
}

/*--------------------------------------------------------------------------*\
|   lbWndProc (hwnd, msg, wParam, lParam)
|
|   Description:
|    This is called when the application is first loaded into
|    memory.  It performs all initialization.
|
|   Arguments:
|    hPrev       instance handle of previous instance
|    hInst       instance handle of current instance
|
|   Returns:
|       TRUE if successful, FALSE if not
|
\*--------------------------------------------------------------------------*/
LRESULT APIENTRY lbWndProc (HWND hwnd, UINT msg, WPARAM wParam, LONG lParam)
{
    PAINTSTRUCT ps;
    HANDLE      h;
    PLB         plb;
    int         i;
    int         dn,iPos,iMax;
    POINT       pt;

    plb = LBP(hwnd);

    switch (msg)
    {
    case WM_CREATE:
        if (!lbCreate (hwnd))
        return -1L;
        break;

    case WM_LBUTTONDOWN:
        if (plb->flb & LB_CAPTURE)
            return 0L;

        plb->flb |= LB_CAPTURE;

        SetFocus (hwnd);
        SetCapture (hwnd);

        // fall through

    case WM_TIMER:

        // fall through

    case WM_MOUSEMOVE:
        if (plb->flb & LB_CAPTURE)
        {
            LONG2POINT(lParam, pt);
            i = lbItemFromPoint (plb, pt);

            if(i >= 0)
                SendMessage (hwnd, LB_SETCURSEL, i, 0l);
        }
        return 0l;

    case WM_LBUTTONDBLCLK:
        LONG2POINT(lParam, pt);
        if (lbItemFromPoint (plb, pt) >= 0)
            lbCmdNotify (plb, LBN_DBLCLK);
        break;

    case WM_LBUTTONUP:
        if (plb->flb & LB_CAPTURE)
        {
                ReleaseCapture ();
                plb->flb &= ~LB_CAPTURE;
        }
            return 0L;

    case WM_DESTROY:
        if (h = LBH(hwnd))
        {
            SendMessage (hwnd,LB_RESETCONTENT,0,0l);
            SetWindowLong (hwnd,GWL_HANDLE,0);
            LocalFree (h);
        }
        break;

    case WM_ERASEBKGND:
        return 0L;

    case WM_PAINT:
        BeginPaint (hwnd, &ps);
        lbPaint (plb,ps.hdc);
        EndPaint (hwnd, &ps);
        return 0L;

    case WM_VSCROLL:
        iPos = plb->dyScroll;
        iMax = plb->nyScroll;
        switch (LOWORD(wParam))
        {
            case SB_LINEDOWN:      dn =  1; break;
            case SB_LINEUP:        dn = -1; break;
            case SB_PAGEDOWN:      dn =  1; break;
            case SB_PAGEUP:        dn = -1; break;
            case SB_THUMBTRACK:
            case SB_THUMBPOSITION: dn = HIWORD(wParam)-iPos; break;
            default:               dn = 0;
        }
        if (dn = BOUND(iPos+dn,0,iMax) - iPos)
        {
            plb->dyScroll = iPos + dn;
            ScrollWindow (hwnd, 0, -dn*plb->dy, NULL, NULL);
            SetScrollPos (hwnd, SB_VERT, plb->dyScroll, TRUE);
            UpdateWindow (hwnd);
        }
        break;

    case WM_SIZE:
            lbSize(plb);
            break;

    case WM_CHAR:
        if ( GetWindowLong (hwnd, GWL_STYLE) & LBS_WANTKEYBOARDINPUT )
            if ((i = (int)lbNotifyParent (plb, WM_CHARTOITEM,
                            MAKELONG(wParam, plb->iCurSel), (LONG)hwnd)) >= 0)
            {
                SendMessage (hwnd, LB_SETCURSEL, i, 0L);
                lbShowItem (plb, plb->iCurSel);
            }
        break;

    case WM_KEYDOWN:
        switch (wParam)
        {
        default:
            i = 0;
            break;
        case VK_RETURN:
            lbCmdNotify (plb,LBN_DBLCLK);
            break;
        case VK_LEFT:
            i = -1;
            break;
        case VK_RIGHT:
            i = +1;
            break;
        case VK_UP:
            i = -plb->nx;
            break;
        case VK_DOWN:
            i = +plb->nx;
            break;
        case VK_HOME:
            i = -plb->iCurSel;
            break;
        case VK_END:
            i = plb->nItems - plb->iCurSel - 1;
            break;
        }

        if (i != 0)
        {
            SendMessage (hwnd, LB_SETCURSEL, plb->iCurSel+i, 0L);
            lbShowItem (plb, plb->iCurSel);
        }
        break;

        case WM_KILLFOCUS:
        case WM_SETFOCUS:
            lbDrawItem(plb, NULL, plb->iCurSel, ODA_FOCUS);
            break;

        case LB_RESETCONTENT:
            for (i=plb->nItems-1; i >= 0; i--)
                if (lbDeleteItem (plb, i))
                    InvalidateRect (hwnd, NULL, TRUE);

            break;

        case LB_ADDSTRING:
            i = plb->nItems++;
            plb->lData[i] = lParam;
            lbDrawItem (plb, NULL, i, ODA_DRAWENTIRE);
            return (LONG)i;

        case LB_SETCURSEL:
            if ((int)wParam < 0 || (int)wParam >= plb->nItems)
                return 0L;

            if ((int)wParam != plb->iCurSel)
            {
                i = plb->iCurSel;
                plb->iCurSel = wParam;

                lbDrawItem (plb, NULL, i, ODA_SELECT);
                lbDrawItem (plb, NULL, plb->iCurSel, ODA_SELECT);
////////////////lbShowItem (plb, plb->iCurSel);
                lbCmdNotify (plb, LBN_SELCHANGE);
            }
            break;

        case LB_GETCURSEL:
            return plb->iCurSel;
            break;

        case LB_GETCOUNT:
            return plb->nItems;
            break;

        case LB_SETCOLUMNWIDTH:
            plb->dx = wParam;
            lbSize (plb);
            break;

        case LB_GETITEMDATA:
            return plb->lData[wParam];
            break;

        case LB_SETITEMDATA:
            plb->lData[wParam] = lParam;
            break;

        case WM_SETREDRAW:
            break;
    }
    return DefWindowProc (hwnd, msg, wParam, lParam);
}


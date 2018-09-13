#include "ctlspriv.h"

typedef struct {
    short x;
    short cxy;
    short fmt;
    LPSTR pszText;
    HBITMAP hbm;
    LPARAM lParam;
} HDI;

// BUGBUG: store the style here too, set at create time
typedef struct {
    HWND hwnd;
    HWND hwndParent;
    UINT flags;
    int iRepaint;       // First item needing repaint
    int cxEllipses;
    int cxDividerSlop;
    int cyChar;
    HFONT hfont;
    HDSA hdsaHDI;       // list of HDI's
} HD;

LRESULT CALLBACK Header_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Message handler functions

BOOL NEAR Header_OnCreate(HD* phd, CREATESTRUCT FAR* lpCreateStruct);
void NEAR Header_OnDestroy(HD* phd);
void NEAR Header_OnPaint(HD* phd, HDC hdcIn);
#if 0
BOOL NEAR Header_OnEraseBkgnd(HD* phd, HDC hdc);
#endif
void NEAR Header_OnCommand(HD* phd, int id, HWND hwndCtl, UINT codeNotify);
void NEAR Header_OnEnable(HD* phd, BOOL fEnable);
UINT NEAR Header_OnGetDlgCode(HD* phd, MSG FAR* lpmsg);
void NEAR Header_OnLButtonDown(HD* phd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
BOOL NEAR Header_StartDrag(HD* phd, int i, int x, int y);
BOOL NEAR Header_IsTracking(HD* phd);
void NEAR Header_OnMouseMove(HD* phd, int x, int y, UINT keyFlags);
void NEAR Header_OnLButtonUp(HD* phd, int x, int y, UINT keyFlags);
void NEAR Header_OnCancelMode(HD* phd);
void NEAR Header_OnSetFont(HD* plv, HFONT hfont, BOOL fRedraw);
int NEAR PASCAL Header_OnHitTest(HD* phd, HD_HITTESTINFO FAR *phdht);
HFONT NEAR Header_OnGetFont(HD* plv);

BOOL NEAR Header_GetItemRect(HD* phd, int i, RECT FAR* prc);
void NEAR Header_Redraw(HD* phd, HDC hdc, RECT FAR* prcClip, int i);
void NEAR Header_RedrawItem(HD* phd, int i);

// HDM_* Message handler functions

int NEAR Header_OnInsertItem(HD* phd, int i, const HD_ITEM FAR* pitem);
BOOL NEAR Header_OnDeleteItem(HD* phd, int i);
BOOL NEAR Header_OnGetItem(HD* phd, int i, HD_ITEM FAR* pitem);
BOOL NEAR Header_OnSetItem(HD* phd, int i, const HD_ITEM FAR* pitem);
BOOL NEAR Header_OnLayout(HD* phd, HD_LAYOUT FAR* playout);
BOOL NEAR Header_OnSetCursor(HD* phd, HWND hwndCursor, UINT codeHitTest, UINT msg);


#define Header_GetItemPtr(phd, i)   (HDI FAR*)DSA_GetItemPtr((phd)->hdsaHDI, (i))


#pragma code_seg(CODESEG_INIT)

BOOL FAR PASCAL Header_Init(HINSTANCE hinst)
{
    WNDCLASS wc;

    if (!GetClassInfo(hinst, c_szHeaderClass, &wc)) {
#ifndef WIN32
#ifndef IEWIN31
	//
	// Use stab WndProc to avoid loading segment on init.
	//
	LRESULT CALLBACK _Header_WndProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
    	wc.lpfnWndProc     = _Header_WndProc;
#else
        wc.lpfnWndProc     = Header_WndProc;
#endif
#else
    	wc.lpfnWndProc	   = Header_WndProc;
#endif
    	wc.hCursor         = NULL;	// we do WM_SETCURSOR handling
    	wc.hIcon           = NULL;
    	wc.lpszMenuName    = NULL;
    	wc.hInstance       = hinst;
    	wc.lpszClassName   = c_szHeaderClass;
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    	wc.style           = CS_DBLCLKS | CS_GLOBALCLASS;
    	wc.cbWndExtra      = sizeof(HD*);
    	wc.cbClsExtra      = 0;

    	return RegisterClass(&wc);
    }

    return TRUE;
}
#pragma code_seg()


LRESULT CALLBACK Header_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HD* phd = (HD*)GetWindowInt(hwnd, 0);

    if (phd == NULL)
    {
        if (msg == WM_NCCREATE)
        {
            phd = (HD*)NearAlloc(sizeof(HD));

            if (phd == NULL)
                return 0L;

            phd->hwnd = hwnd;
            phd->hwndParent = GetParent(hwnd);
	    SetWindowInt(hwnd, 0, (int)phd);
        }
        else
        {
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }

    if (msg == WM_NCDESTROY)
    {
        //DWORD result = HANDLE_MSG(hwnd, WM_NCDESTROY, Header_OnNCDestroy);

        NearFree(phd);
        phd = NULL;
        SetWindowInt(hwnd, 0, 0);

        //return result;
    }

    switch (msg)
    {
        HANDLE_MSG(phd, WM_CREATE, Header_OnCreate);
        HANDLE_MSG(phd, WM_DESTROY, Header_OnDestroy);
#if 0
        HANDLE_MSG(phd, WM_ERASEBKGND, Header_OnEraseBkgnd);
#endif
        HANDLE_MSG(phd, WM_ENABLE, Header_OnEnable);
        HANDLE_MSG(phd, WM_SETCURSOR, Header_OnSetCursor);
        HANDLE_MSG(phd, WM_MOUSEMOVE, Header_OnMouseMove);
        HANDLE_MSG(phd, WM_LBUTTONDOWN, Header_OnLButtonDown);
        HANDLE_MSG(phd, WM_LBUTTONDBLCLK, Header_OnLButtonDown);
        HANDLE_MSG(phd, WM_LBUTTONUP, Header_OnLButtonUp);
        HANDLE_MSG(phd, WM_CANCELMODE, Header_OnCancelMode);
        HANDLE_MSG(phd, WM_GETDLGCODE, Header_OnGetDlgCode);
        HANDLE_MSG(phd, WM_SETFONT, Header_OnSetFont);
        HANDLE_MSG(phd, WM_GETFONT, Header_OnGetFont);

    case WM_PRINTCLIENT:
    case WM_PAINT:
        Header_OnPaint(phd, (HDC)wParam);
        return(0);

    case WM_RBUTTONUP:
        if (!SendNotify(phd->hwndParent, phd->hwnd, NM_RCLICK, NULL))
            goto DoDefWindowProc;
        break;

    case WM_STYLECHANGED:
        // we don't cache our style so relay out and invaidate
        InvalidateRect(phd->hwnd, NULL, TRUE);
        break;

    case HDM_GETITEMCOUNT:
        return (LPARAM)(UINT)DSA_GetItemCount(phd->hdsaHDI);

    case HDM_INSERTITEM:
        return (LPARAM)Header_OnInsertItem(phd, (int)wParam, (const HD_ITEM FAR*)lParam);

    case HDM_DELETEITEM:
        return (LPARAM)Header_OnDeleteItem(phd, (int)wParam);

    case HDM_GETITEM:
        return (LPARAM)Header_OnGetItem(phd, (int)wParam, (HD_ITEM FAR*)lParam);

    case HDM_SETITEM:
        return (LPARAM)Header_OnSetItem(phd, (int)wParam, (const HD_ITEM FAR*)lParam);

    case HDM_LAYOUT:
        return (LPARAM)Header_OnLayout(phd, (HD_LAYOUT FAR*)lParam);

    case HDM_HITTEST:
        return (LPARAM)Header_OnHitTest(phd, (HD_HITTESTINFO FAR *)lParam);

    default:
DoDefWindowProc:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

BOOL NEAR Header_SendChange(HD* phd, int i, int code, const HD_ITEM FAR* pitem)
{
    HD_NOTIFY nm;

    nm.iItem = i;
    nm.pitem = (HD_ITEM FAR*)pitem;
    nm.iButton = 0;

    return !(BOOL)SendNotify(phd->hwndParent, phd->hwnd, code, &nm.hdr);
}

void NEAR Header_Notify(HD* phd, int i, int iButton, int code)
{
    HD_NOTIFY nm;
    nm.iItem = i;
    nm.iButton = iButton;
    nm.pitem = NULL;

    SendNotify(phd->hwndParent, phd->hwnd, code, &nm.hdr);
}


void NEAR Header_NewFont(HD* phd, HFONT hfont)
{
    HDC hdc;
    SIZE siz;

    hdc = GetDC(HWND_DESKTOP);

    if (hfont)
        SelectFont(hdc, hfont);

    GetTextExtentPoint(hdc, c_szEllipses, CCHELLIPSES, &siz);

    phd->cxEllipses = siz.cx;
    phd->cyChar = siz.cy;
    phd->hfont = hfont;

    ReleaseDC(HWND_DESKTOP, hdc);
}

BOOL NEAR Header_OnCreate(HD* phd, CREATESTRUCT FAR* lpCreateStruct)
{
    phd->iRepaint = 0;
    phd->flags = 0;
    phd->hfont = NULL;

    phd->hdsaHDI = DSA_Create(sizeof(HDI), 4);
    phd->cxDividerSlop = 8 * g_cxBorder;
    Header_NewFont(phd, NULL);
    return TRUE;
}

void NEAR Header_OnDestroy(HD* phd)
{
    int j;
    HDI FAR * phdi;

    // We must walk through and destroy all of the string pointers that
    // are contained in the structures before we pass it off to the
    // DSA_Destroy function...
    for (j = DSA_GetItemCount(phd->hdsaHDI) - 1; j >= 0 ; j--) {
        phdi = Header_GetItemPtr(phd, j);
        if (phdi && phdi->pszText)
        {
            Str_Set(&phdi->pszText, NULL);
        }
    }

    DSA_Destroy(phd->hdsaHDI);
}

void NEAR Header_OnPaint(HD* phd, HDC hdc)
{
    PAINTSTRUCT ps;

    if (hdc)
    {
        GetClientRect(phd->hwnd, &ps.rcPaint);
        Header_Redraw(phd, hdc, &ps.rcPaint, 0);
    }
    else
    {
        Header_Redraw(phd, BeginPaint(phd->hwnd, &ps), &ps.rcPaint, 0);
        EndPaint(phd->hwnd, &ps);
    }
}

#if 0
BOOL NEAR Header_OnEraseBkgnd(HD* phd, HDC hdc)
{
    RECT rc;

    GetClientRect(phd->hwnd, &rc);
    FillRect(hdc, &rc, g_hbrBtnFace);
    return TRUE;
}
#endif

void NEAR Header_OnCommand(HD* phd, int id, HWND hwndCtl, UINT codeNotify)
{
}

void NEAR Header_OnEnable(HD* phd, BOOL fEnable)
{
}

UINT NEAR Header_OnGetDlgCode(HD* phd, MSG FAR* lpmsg)
{
    return DLGC_WANTTAB | DLGC_WANTARROWS;
}


int NEAR Header_HitTest(HD* phd, int x, int y, UINT FAR* pflags)
{
    UINT flags = 0;
    POINT pt;
    RECT rc;
    HDI FAR* phdi;
    int i;

    pt.x = x; pt.y = y;

    GetClientRect(phd->hwnd, &rc);

    flags = 0;
    i = -1;
    if (x < rc.left)
        flags |= HHT_TOLEFT;
    else if (x >= rc.right)
        flags |= HHT_TORIGHT;
    if (y < rc.top)
        flags |= HHT_ABOVE;
    else if (y >= rc.bottom)
        flags |= HHT_BELOW;

    if (flags == 0)
    {
        int cItems = DSA_GetItemCount(phd->hdsaHDI);
        int xPrev = 0;
        BOOL fPrevZero = FALSE;
        int xItem;
        int cxSlop;

        phdi = Header_GetItemPtr(phd, 0);
        //DebugMsg(DM_TRACE, "Hit Test begin");
        for (i = 0; i <= cItems; i++, phdi++, xPrev = xItem)
        {
            xItem = (i == cItems ? rc.right : phdi->x);

            //DebugMsg(DM_TRACE, "x = %d xItem = %d xPrev = %d fPrevZero = %d", x, xItem, xPrev, xPrev == xItem);
            if (xItem == xPrev)
            {
                // Skip zero width items...
                //
                fPrevZero = TRUE;
                continue;
            }

            cxSlop = min((xItem - xPrev) / 4, phd->cxDividerSlop);

            if (x >= xPrev && x < xItem)
            {
                flags = HHT_ONHEADER;

                if (i > 0 && x < xPrev + cxSlop)
                {
                    i--;
                    flags = HHT_ONDIVIDER;

                    if (fPrevZero && x > xPrev)
                    {
                        flags = HHT_ONDIVOPEN;
                    }
                }
                else if (x >= xItem - cxSlop)
                {
                    flags = HHT_ONDIVIDER;
                }
                break;
            }
            fPrevZero = FALSE;
        }
        if (i == cItems)
        {
            i = -1;
            flags = HHT_NOWHERE;
        }
    }
    *pflags = flags;
    return i;
}

int NEAR PASCAL Header_OnHitTest(HD* phd, HD_HITTESTINFO FAR *phdht)
{
    if (phdht) {
        phdht->iItem = Header_HitTest(phd, phdht->pt.x, phdht->pt.y, &phdht->flags);
        return phdht->iItem;
    } else
        return -1;
}

BOOL NEAR Header_OnSetCursor(HD* phd, HWND hwndCursor, UINT codeHitTest, UINT msg)
{
    POINT pt;
    UINT flags;
    DWORD dw;
    LPCSTR lpCur;
    HINSTANCE hinst;

    if (phd->hwnd != hwndCursor || codeHitTest >= 0x8000)
        return FALSE;

    dw = GetMessagePos();

    pt.x = LOWORD(dw);
    pt.y = HIWORD(dw);

    ScreenToClient(hwndCursor, &pt);

    Header_HitTest(phd, pt.x, pt.y, &flags);

    hinst = HINST_THISDLL;
    switch (flags)
    {
    case HHT_ONDIVIDER:
        lpCur = MAKEINTRESOURCE(IDC_DIVIDER);
        break;
    case HHT_ONDIVOPEN:
        lpCur = MAKEINTRESOURCE(IDC_DIVOPEN);
        break;
    default:
        lpCur = IDC_ARROW;
	hinst = NULL;
        break;
    }
    SetCursor(LoadCursor(hinst, lpCur));
    return TRUE;
}

void NEAR Header_DrawDivider(HD* phd, int x)
{
    RECT rc;
    HDC hdc = GetDC(phd->hwnd);

    GetClientRect(phd->hwnd, &rc);
    rc.left = x;
    rc.right = x + g_cxBorder;

    InvertRect(hdc, &rc);

    ReleaseDC(phd->hwnd, hdc);
}

// BUGBUG: no need for most of these globals.

HWND g_hwndTrack = NULL;
int g_iTrack = 0;
int g_bTrackPress = FALSE;		// is the button pressed?
UINT g_flagsTrack = 0;
int g_xTrack = 0;
int g_dxTrack = 0;
int g_xMinTrack = 0;

int NEAR Header_GetItemPosition(HD* phd, int i)
{
    if (i < 0)
        return 0;
    return (Header_GetItemPtr(phd, i))->x;
}

int NEAR Header_PinDividerPos(HD* phd, int x)
{
    x += g_dxTrack;
    if (x < g_xMinTrack)
        x = g_xMinTrack;
    return x;
}

void NEAR Header_OnLButtonDown(HD* phd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    HD_ITEM hd;
    int i;
    UINT flags;

    i = Header_HitTest(phd, x, y, &flags);
    if (flags & (HHT_ONDIVIDER))
    {
        if (fDoubleClick) {
            Header_SendChange(phd, i, HDN_DIVIDERDBLCLICK, NULL);
        }
    }
    if (flags & (HHT_ONDIVIDER | HHT_ONHEADER | HHT_ONDIVOPEN))
    {
        g_hwndTrack = phd->hwnd;
        g_iTrack = i;
        g_flagsTrack = flags;
        SetCapture(phd->hwnd);
    }
    if (flags & (HHT_ONDIVIDER | HHT_ONDIVOPEN))
    {
        //
        // We should first send out the HDN_BEGINTRACK notification
        //
        g_xMinTrack = Header_GetItemPosition(phd, i - 1);
        g_xTrack = Header_GetItemPosition(phd, i);
        g_dxTrack = g_xTrack - x;

        hd.mask = HDI_WIDTH;
        hd.cxy = x - g_xMinTrack;
        if (!Header_SendChange(phd, i, HDN_BEGINTRACK, &hd))
        {
            // They said no!
            ReleaseCapture();
            g_hwndTrack = NULL;
            return;
        }


        x = Header_PinDividerPos(phd, x);
        Header_DrawDivider(phd, x);
    }
    else if ((flags & HHT_ONHEADER) && (GetWindowStyle(phd->hwnd) & HDS_BUTTONS))
    {
        if (fDoubleClick) {
            Header_SendChange(phd, i, HDN_ITEMDBLCLICK, NULL);
        } else {
            g_bTrackPress = TRUE;
            Header_RedrawItem(phd, g_iTrack);
        }
    }
}

void NEAR Header_OnMouseMove(HD* phd, int x, int y, UINT keyFlags)
{
    UINT flags;
    int i;
    HD_ITEM hd;

    if (Header_IsTracking(phd))
    {
        if (g_flagsTrack & (HHT_ONDIVIDER | HHT_ONDIVOPEN))
        {
            x = Header_PinDividerPos(phd, x);

            //
            // Let the Owner have a chance to update this.
            //
            hd.mask = HDI_WIDTH;
            hd.cxy = x - g_xMinTrack;
            if (!Header_SendChange(phd, g_iTrack, HDN_TRACK, &hd))
            {
                // We need to cancel tracking
                ReleaseCapture();
                g_hwndTrack = NULL;

                // Undraw the last divider we displayed
                Header_DrawDivider(phd, g_xTrack);
                return;
            }

            // We should update our x depending on what caller did
            x = hd.cxy + g_xMinTrack;

            Header_DrawDivider(phd, g_xTrack);
            Header_DrawDivider(phd, x);
            g_xTrack = x;
        }
        else if (g_flagsTrack & HHT_ONHEADER)
        {
	    i = Header_HitTest(phd, x, y, &flags);
	    // if pressing on button and it's not pressed, press it
	    if (flags & HHT_ONHEADER && i == g_iTrack)
	    {
		if ((!g_bTrackPress) && (GetWindowStyle(phd->hwnd) & HDS_BUTTONS))
		{
		    g_bTrackPress = TRUE;
		    Header_RedrawItem(phd, g_iTrack);
		}
	    }
	    // tracked off of button.  if pressed, pop it
	    else if ((g_bTrackPress) && (GetWindowStyle(phd->hwnd) & HDS_BUTTONS))
	    {
		g_bTrackPress = FALSE;
		Header_RedrawItem(phd, g_iTrack);
	    }
        }
    }
}

void NEAR Header_OnLButtonUp(HD* phd, int x, int y, UINT keyFlags)
{
    if (Header_IsTracking(phd))
    {
        ReleaseCapture();
        g_hwndTrack = NULL;

        if (g_flagsTrack & (HHT_ONDIVIDER | HHT_ONDIVOPEN))
        {
            HD_ITEM item;

            Header_DrawDivider(phd, g_xTrack);

            item.mask = HDI_WIDTH;
            item.cxy = g_xTrack - g_xMinTrack;

            // Let the owner have a chance to say yes.


            if (Header_SendChange(phd, g_iTrack, HDN_ENDTRACK, &item))
                Header_OnSetItem(phd, g_iTrack, &item);

            RedrawWindow(phd->hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
        }
        else if ((g_flagsTrack & HHT_ONHEADER && g_bTrackPress) && (GetWindowStyle(phd->hwnd) & HDS_BUTTONS))
        {
            // Notify the owner that the item has been clicked
	    Header_Notify(phd, g_iTrack, 0, HDN_ITEMCLICK);
	    g_bTrackPress = FALSE;
	    Header_RedrawItem(phd, g_iTrack);
        }
    }
}

BOOL NEAR Header_StartDrag(HD* phd, int i, int x, int y)
{
    return FALSE;
}

BOOL NEAR Header_IsTracking(HD* phd)
{
    if (!g_hwndTrack || phd->hwnd != g_hwndTrack)
        return FALSE;

    if (GetCapture() != phd->hwnd)
    {
        g_flagsTrack = 0;
        return FALSE;
    }

    return TRUE;
}

// BUGBUG: use this to check capture loss
void NEAR Header_OnCancelMode(HD* phd)
{
}

void NEAR Header_OnSetFont(HD* phd, HFONT hfont, BOOL fRedraw)
{
    if (hfont != phd->hfont)
    {
        Header_NewFont(phd, hfont);

        if (fRedraw)
            RedrawWindow(phd->hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
    }
}

HFONT NEAR Header_OnGetFont(HD* phd)
{
    return phd->hfont;
}

//**********************************************************************

int NEAR Header_OnInsertItem(HD* phd, int i, const HD_ITEM FAR* pitem)
{
    HDI hdi;
    int x;
    HDI FAR* phdi;
    int j;

    if (!pitem)
    	return -1;
    	
    if (pitem->mask == 0)
        return -1;

    x = pitem->cxy;

    if (i > DSA_GetItemCount(phd->hdsaHDI))
        i = DSA_GetItemCount(phd->hdsaHDI);

    if (i > 0)
    {

        phdi = Header_GetItemPtr(phd, i - 1);
        if (phdi)
            x += phdi->x;

    }

    // move everything else over
    for (j = DSA_GetItemCount(phd->hdsaHDI) - 1; j >= i ; j--) {
        phdi = Header_GetItemPtr(phd, j);
        phdi->x += pitem->cxy;
    }

    hdi.x = x;
    hdi.lParam = pitem->lParam;
    hdi.fmt = pitem->fmt;
    hdi.pszText = NULL;
    hdi.cxy = pitem->cxy;
    if ((pitem->mask & HDI_TEXT) && (pitem->pszText != NULL))
    {
        if (!Str_Set(&hdi.pszText, pitem->pszText))
            return -1;

        // Unless ownerdraw make sure the text bit is on!
        if ((pitem->mask & HDF_OWNERDRAW) == 0)
            hdi.fmt |= HDF_STRING;
    }
    else
    {
        hdi.fmt &= ~(HDF_STRING);
    }

    if ((pitem->mask & HDI_BITMAP) && (pitem->hbm != NULL))
    {
        hdi.hbm = pitem->hbm;

        // Unless ownerdraw make sure the text bit is on!
        if ((pitem->mask & HDF_OWNERDRAW) == 0)
            hdi.fmt |= HDF_BITMAP;
    }
    else
    {
        hdi.hbm = NULL;
        hdi.fmt &= ~(HDF_BITMAP);
    }

    i = DSA_InsertItem(phd->hdsaHDI, i, &hdi);
    if (i == -1) {
        // failed to add
        if (hdi.pszText)
            Str_Set(&hdi.pszText, NULL);
    } else {
        // succeeded!  redraw
        RECT rc;
        GetClientRect(phd->hwnd, &rc);
        rc.left = x - pitem->cxy;
        RedrawWindow(phd->hwnd, &rc, NULL, RDW_INVALIDATE | RDW_ERASE);
    }

    return i;
}

BOOL NEAR Header_OnDeleteItem(HD* phd, int i)
{
    HDI hdi;
    RECT rc;
    int iWidth;
    HDI FAR * phdi;
    int j;

    if (!DSA_GetItem(phd->hdsaHDI, i, &hdi))
        return FALSE;

    GetClientRect(phd->hwnd, &rc);
    iWidth = rc.right;
    Header_GetItemRect(phd, i, &rc);
    InflateRect(&rc, g_cxBorder, g_cyBorder);

    if (!DSA_DeleteItem(phd->hdsaHDI, i))
        return FALSE;

    if (hdi.pszText)
        Str_Set(&hdi.pszText, NULL);

    // move everything else over
    for (j = DSA_GetItemCount(phd->hdsaHDI) - 1; j >= i ; j--) {
        phdi = Header_GetItemPtr(phd, j);
        phdi->x -= hdi.cxy;
    }

    rc.right = iWidth;
    InvalidateRect(phd->hwnd, &rc, TRUE);
    return TRUE;
}

BOOL NEAR Header_OnGetItem(HD* phd, int i, HD_ITEM FAR* pitem)
{
    HDI FAR* phdi;
    UINT mask;

    Assert(pitem);

    if (!pitem)
    	return FALSE;
    	
    mask = pitem->mask;

    phdi = Header_GetItemPtr(phd, i);
    if (!phdi)
        return FALSE;

    if (mask & HDI_WIDTH)
    {
        int cx = phdi->x;

        if (i > 0)
        {
            Assert((phdi - 1) == Header_GetItemPtr(phd, i - 1));
            cx -= (phdi - 1)->x;
        }

        pitem->cxy = cx;
    }

    if (mask & HDI_FORMAT)
    {
        pitem->fmt = phdi->fmt;
    }

    if (mask & HDI_LPARAM)
    {
        pitem->lParam = phdi->lParam;
    }

    if (mask & HDI_TEXT)
    {
        if (!Str_GetPtr(phdi->pszText, pitem->pszText, pitem->cchTextMax))
            return FALSE;
    }

    if (mask & HDI_BITMAP)
    {
        pitem->hbm = phdi->hbm;
    }

    pitem->mask = mask;
    return TRUE;
}

BOOL NEAR Header_OnSetItem(HD* phd, int i, const HD_ITEM FAR* pitem)
{
    HDI FAR* phdi;
    UINT mask;
    int xOld;
    BOOL fInvalidate;

    Assert(pitem);

    if (!pitem)
    	return FALSE;
    	
    phdi = Header_GetItemPtr(phd, i);
    if (!phdi)
        return FALSE;

    mask = pitem->mask;

    if (mask == 0)
        return TRUE;

    if (!Header_SendChange(phd, i, HDN_ITEMCHANGING, pitem))
        return FALSE;

    xOld = phdi->x;
    if (mask & HDI_WIDTH)
    {
        int j;
        HDI FAR* phdiLocal;
        int dx = pitem->cxy;

        if (dx < 0)
            dx = 0;

        if (i > 0)
        {
            Assert((phdi - 1) == Header_GetItemPtr(phd, i - 1));
            dx += (phdi - 1)->x;
        }

        dx -= phdi->x;

        phdi->cxy = pitem->cxy;
        for (phdiLocal = phdi, j = i; j < DSA_GetItemCount(phd->hdsaHDI); j++, phdiLocal++)
        {
            phdiLocal->x += dx;
        }
        fInvalidate = TRUE;
    }
    if (mask & HDI_FORMAT) {
        phdi->fmt = pitem->fmt;
        fInvalidate = TRUE;
    }
    if (mask & HDI_LPARAM)
        phdi->lParam = pitem->lParam;

    if (mask & HDI_TEXT)
    {
        if (!Str_Set(&phdi->pszText, pitem->pszText))
            return FALSE;
        fInvalidate = TRUE;
    }

    if (mask & HDI_BITMAP)
    {
        phdi->hbm = pitem->hbm;
        fInvalidate = TRUE;
    }

    Header_SendChange(phd, i, HDN_ITEMCHANGED, pitem);

    if (fInvalidate) {
        if (xOld == phdi->x) {
            // no change in x
            Header_RedrawItem(phd, i);
        } else {
            RECT rc;
            GetClientRect(phd->hwnd, &rc);

            if (i > 0) {
                HDI FAR * phdiTemp;
                phdiTemp = Header_GetItemPtr(phd, i - 1);
                if (phdiTemp) {
                    rc.left = phdi->x;
                }
            }
            RedrawWindow(phd->hwnd, &rc, NULL, RDW_INVALIDATE | RDW_ERASE);
        }
    }
    return TRUE;
}

// Compute layout for header bar, and leftover rectangle.
//
BOOL NEAR Header_OnLayout(HD* phd, HD_LAYOUT FAR* playout)
{
    int cyHeader;
    WINDOWPOS FAR* pwpos;
    RECT FAR* prc;

    Assert(playout);

    if (!(playout && phd))
    	return FALSE;

    if (!(playout->pwpos && playout->prc))
	return FALSE;

    pwpos = playout->pwpos;
    prc = playout->prc;

    cyHeader = phd->cyChar + 2 * g_cyEdge;

    // BUGBUG: we should store the style at creat  time
    // internal hack style for use with LVS_REPORT|LVS_NOCOLUMNHEADER! edh
    if (GetWindowStyle(phd->hwnd) & HDS_HIDDEN)
	cyHeader = 0;

    pwpos->hwndInsertAfter = NULL;
    pwpos->flags = SWP_NOZORDER | SWP_NOACTIVATE;

    // BUGBUG: Assert(phd->style & HDS_HORZ);

    pwpos->x  = prc->left;
    pwpos->cx = prc->right - prc->left;
    pwpos->y  = prc->top;
    pwpos->cy = cyHeader;

    prc->top += cyHeader;
    return TRUE;
}

BOOL NEAR Header_GetItemRect(HD* phd, int i, RECT FAR* prc)
{
    HDI FAR* phdi;

    phdi = Header_GetItemPtr(phd, i);
    if (!phdi)
        return FALSE;

    GetClientRect(phd->hwnd, prc);

    prc->right = phdi->x;
    prc->left = prc->right - phdi->cxy;
#ifdef DEBUG
    if (i > 0)
        Assert(prc->left == (phdi - 1)->x);
#endif
    return TRUE;
}

void NEAR Header_RedrawItem(HD* phd, int i)
{
    RECT rc;

    Header_GetItemRect(phd, i, &rc);
    InflateRect(&rc, g_cxBorder, g_cyBorder);
    RedrawWindow(phd->hwnd, &rc, NULL, RDW_INVALIDATE | RDW_ERASE);
}

void NEAR _Header_DrawBitmap(HDC hdc, HBITMAP hbm, HDC hdcMem, RECT FAR *prc,
        int fmt, UINT flags)
{
    BITMAP bm;
    RECT rc;
    int xBitmap = 0;
    int yBitmap = 0;
    HBITMAP hbmOld;

    if (GetObject(hbm, sizeof(bm), &bm) != sizeof(bm))
        return;     // could not get the info about bitmap.

    if ((hbmOld = SelectObject(hdcMem, hbm)) == ERROR)
        return;     // an error happened.

    rc = *prc;
    rc.left  += g_cxLabelMargin;
    rc.right -= g_cxLabelMargin;

    // If needed, add in a little extra margin...
    //
    if (flags & SHDT_EXTRAMARGIN)
    {
        rc.left  += g_cxLabelMargin * 2;
        rc.right -= g_cxLabelMargin * 2;
    }

    if (flags & SHDT_DEPRESSED)
        OffsetRect(&rc, g_cxBorder, g_cyBorder);


    if (fmt == LVCFMT_LEFT)
    {
        if (bm.bmWidth > (rc.right-rc.left))
        {
            bm.bmWidth = rc.right - rc.left;
        }
    }
    else if (fmt == LVCFMT_CENTER)
    {
        if (bm.bmWidth > (rc.right - rc.left))
        {
            xBitmap =  (bm.bmWidth - (rc.right - rc.left)) / 2;
            bm.bmWidth = rc.right - rc.left;
        }
        else
            rc.left = (rc.left + rc.right - bm.bmWidth) / 2;
    }
    else    // fmt == LVCFMT_RIGHT
    {

        if (bm.bmWidth > (rc.right - rc.left))
        {
            xBitmap =  bm.bmWidth - (rc.right - rc.left);
        }
        else
            rc.left = rc.right - bm.bmWidth;
    }

    // Now setup horizontally
    if (bm.bmHeight > (rc.bottom - rc.top))
    {
        yBitmap = (bm.bmHeight - (rc.bottom - rc.top)) / 2;
        bm.bmHeight = rc.bottom - rc.top;
    }
    else
        rc.top = (rc.bottom - rc.top - bm.bmHeight) / 2;

    // Last but not least we will do the bitblt.
    BitBlt(hdc, rc.left, rc.top, bm.bmWidth, bm.bmHeight,
            hdcMem, xBitmap, yBitmap, SRCCOPY);

    // Unselect our object from the DC
    SelectObject(hdcMem, hbmOld);

}

void NEAR Header_Redraw(HD* phd, HDC hdc, RECT FAR* prcClip, int i)
{
    int cItems;
    HDI FAR* phdi;
    RECT rc;
    BOOL fTracking, fItemSunken;
    UINT uDrawTextFlags;
    HFONT hfontOld = NULL;
    HDC hdcMem = NULL;
    WORD wID;

    fTracking = Header_IsTracking(phd);

    SetTextColor(hdc, g_clrBtnText);
    SetBkColor(hdc, g_clrBtnFace);
    if (phd->hfont)
        hfontOld = SelectFont(hdc, phd->hfont);

    cItems = DSA_GetItemCount(phd->hdsaHDI);

    wID = GetWindowID(phd->hwnd);

    Header_GetItemRect(phd, i, &rc);
    phdi = Header_GetItemPtr(phd, i);
    for ( ; i <= cItems; i++)
    {
        if (prcClip)
        {
            if (rc.right < prcClip->left)
                goto NextItem;
            if (rc.left >= prcClip->right)
                break;
        }

        fItemSunken = (fTracking && (g_flagsTrack & HHT_ONHEADER) &&
                        (g_iTrack == i) && g_bTrackPress);

        if (i < cItems)
        {
            if(fItemSunken)
                uDrawTextFlags = SHDT_ELLIPSES | SHDT_DEPRESSED | SHDT_EXTRAMARGIN;
            else
                uDrawTextFlags = SHDT_ELLIPSES | SHDT_EXTRAMARGIN;

            phdi = Header_GetItemPtr(phd, i);

            if (phdi->fmt & HDF_OWNERDRAW)
            {
                DRAWITEMSTRUCT dis;

                dis.CtlType = ODT_HEADER;
                dis.CtlID = wID;
                dis.itemID = i;
                dis.itemAction = ODA_DRAWENTIRE;
                dis.itemState = (uDrawTextFlags & SHDT_DEPRESSED) ? ODS_SELECTED : 0;
                dis.hwndItem = phd->hwnd;
                dis.hDC = hdc;
                dis.rcItem = rc;
                dis.itemData = phdi->lParam;

                // Now send it off to my parent...
                if (SendMessage(phd->hwndParent, WM_DRAWITEM, wID,
                        (LPARAM)(DRAWITEMSTRUCT FAR *)&dis))
                {
                    goto DrawEdges;  //Ick, but it works
                }
            }

            //
            // Now neet to handle the different combinatations of
            // text and bitmaps...
            //
            if (phdi->fmt & HDF_BITMAP)
            {
                if (hdcMem == NULL)
                    hdcMem = CreateCompatibleDC(hdc);

                if (hdcMem != NULL)
                {
                    _Header_DrawBitmap(hdc, phdi->hbm, hdcMem, &rc,
                            phdi->fmt & HDF_JUSTIFYMASK, uDrawTextFlags);

                }
                // BUGBUG: Not Implemented
                Assert (FALSE);
            }

            if (phdi->fmt & HDF_STRING)
            {
#if defined(WINDOWS_ME)
				int oldAlign;

				if (phdi->fmt & HDF_RTLREADING)
					{
					oldAlign = GetTextAlign(hdc);
					SetTextAlign(hdc, oldAlign | TA_RTLREADING);
					}
#endif
                SHDrawText(hdc, phdi->pszText, &rc,
                        phdi->fmt & HDF_JUSTIFYMASK,
                        uDrawTextFlags, phd->cyChar, phd->cxEllipses,
			            GetSysColor( COLOR_BTNTEXT ),
                        GetSysColor( COLOR_BTNFACE ) );
#if defined(WINDOWS_ME)
				if (phdi->fmt & HDF_RTLREADING)
					SetTextAlign(hdc, oldAlign);
#endif
            }
        }

DrawEdges:
        if (GetWindowStyle(phd->hwnd) & HDS_BUTTONS)
        {
            if (fItemSunken)
                DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT | BF_SOFT | BF_FLAT);
            else
                DrawEdge(hdc, &rc, EDGE_RAISED, BF_RECT | BF_SOFT);
        }
        else
        {
                DrawEdge(hdc, &rc, EDGE_ETCHED, BF_BOTTOMRIGHT);
        }


NextItem:
        rc.left = rc.right;
        if (i < cItems - 1)
        {
            phdi++;
            rc.right = phdi->x;
        }
        else
        {
            rc.right = 30000;
        }
    }

    if (fTracking && (g_flagsTrack & (HHT_ONDIVIDER | HHT_ONDIVOPEN)))
    {
        Header_DrawDivider(phd, g_xTrack);
    }

    if (hfontOld)
	SelectFont(hdc, hfontOld);

    // Also free any memory dcs we may have created
    if (hdcMem != NULL)
        DeleteDC(hdcMem);
}

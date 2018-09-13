#include "ctlspriv.h"

/////////////////////////////////////////////////////////////////////////////
//
// updown.c : A micro-scrollbar control; useful for increment/decrement.
//
/////////////////////////////////////////////////////////////////////////////

#define NUM_UDACCELS 3

#define DONTCARE    0
#define SIGNED      1
#define UNSIGNED    2

typedef struct {
    HWND    hwnd;
    LONG    style;
    HWND    hwndBuddy;
    unsigned fUp        : 1;
    unsigned fDown      : 1;
    unsigned fUnsigned  : 1;    // BUGBUG: no way to turn this on
    unsigned fSharedBorder  : 1;
    unsigned fSunkenBorder  : 1;
    unsigned fUpDownDestroyed : 1;  // This tells the buddy that updown destoryed.
    UINT     nBase;
    int      nUpper;
    int      nLower;
    int      nPos;
    HWND     hwndParent;
    UINT     uClass;
    BOOL     bDown;
    DWORD    dwStart;
    WNDPROC  lpfnDefProc;
    UINT     nAccel;
    UDACCEL  udAccel[NUM_UDACCELS];
} UDSTATE, NEAR *PUDSTATE;


// Constants:
//
#define CLASS_UNKNOWN   0
#define CLASS_EDIT  1
#define CLASS_LISTBOX   2

#define MAX_INTLENGTH   18 // big enough for all intl stuff, too

// this is the space to the left and right of the arrow (in pixels)
#define XBORDER 0

#define BASE_DECIMAL    10
#define BASE_HEX        16

// Declarations:
//
LRESULT CALLBACK ArrowKeyProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


/////////////////////////////////////////////////////////////////////////////

//
// ***** Internal workhorses *****
//


// Validates the buddy.
//
void NEAR PASCAL isgoodbuddy(PUDSTATE np)
{
    if (!np->hwndBuddy)
        return;
    if (!IsWindow(np->hwndBuddy))
    {
#if defined(DEBUG) && !defined(WIN32)
        DebugOutput(DBF_ERROR | DBF_USER,
                    "UpDown: invalid buddy handle 0x04X; "
                    "resetting to NULL", np->hwndBuddy);
#endif
        np->hwndBuddy = NULL;
        np->uClass = CLASS_UNKNOWN;
    }
    if (GetParent(np->hwndBuddy) != np->hwndParent)
    {
#if defined(DEBUG) && !defined(WIN32)
        DebugOutput(DBF_ERROR | DBF_USER,
                    "UpDown: buddy has different parent; "
                    "resetting to NULL");
#endif
        np->hwndBuddy = NULL;
        np->uClass = CLASS_UNKNOWN;
    }
}

// Picks a good buddy.
//
void NEAR PASCAL pickbuddy(PUDSTATE np)
{
    if (np->style & UDS_AUTOBUDDY)
        np->hwndBuddy = GetWindow(np->hwnd, GW_HWNDPREV);
}


void NEAR PASCAL unachor(PUDSTATE np)
{
    RECT rc;
    RECT rcBuddy;
    RECT rcUD;

    if ( np->hwndBuddy && (np->style & (UDS_ALIGNLEFT | UDS_ALIGNRIGHT))) {
        GetWindowRect(np->hwndBuddy, &rcBuddy);
        GetWindowRect(np->hwnd, &rcUD);
        UnionRect(&rc, &rcUD, &rcBuddy);
        MapWindowRect(NULL, np->hwndParent, &rc);
	MoveWindow(np->hwndBuddy, rc.left, rc.top,
				rc.right - rc.left, rc.bottom - rc.top, FALSE);

    }
}

// Anchor this control to the buddy's edge, if appropriate.
//
void NEAR PASCAL anchor(PUDSTATE np)
{
    BOOL bAlignToBuddy;
    int nOver = 0,  nHasBorder;
    RECT rc, rcBuddy;
    int nHeight, nWidth;

    np->fSharedBorder = FALSE;

    isgoodbuddy(np);
    nHasBorder = (np->style & WS_BORDER) == WS_BORDER;

    bAlignToBuddy = np->hwndBuddy && (np->style & (UDS_ALIGNLEFT | UDS_ALIGNRIGHT));

    if (bAlignToBuddy)
    {
	if ((np->uClass == CLASS_EDIT) ||
		(GetWindowLong(np->hwndBuddy, GWL_EXSTYLE) & WS_EX_CLIENTEDGE))
	{
            np->fSunkenBorder = TRUE;
	}

        GetWindowRect(np->hwndBuddy, &rc);

        if ((np->uClass == CLASS_EDIT) || (GetWindowLong(np->hwndBuddy, GWL_STYLE) & WS_BORDER))
        {
	    // BUGBUG for full generalization, should handle border AND clientedge

            nOver = g_cxBorder * (np->fSunkenBorder ? 2 : 1);
            np->fSharedBorder = TRUE;

	    // turn off border styles...
            np->style &= ~WS_BORDER;
            SetWindowLong(np->hwnd, GWL_STYLE, np->style);
            SetWindowLong(np->hwnd, GWL_EXSTYLE, GetWindowLong(np->hwnd, GWL_EXSTYLE) & ~(WS_EX_CLIENTEDGE));
        }
    }
    else
    {
        GetWindowRect(np->hwnd, &rc);
    }

    nHeight = rc.bottom - rc.top;
    nWidth = rc.right - rc.left;

    ScreenToClient(np->hwndParent, (LPPOINT)&rc.left);
    rc.right = rc.left + nWidth;

    if (bAlignToBuddy)
    {
        nWidth = g_cxVScroll - g_cxBorder + nOver;
	rcBuddy = rc;

        if (np->style & UDS_ALIGNLEFT)
        {
	    // size buddy to right
	    rcBuddy.left += nWidth - nOver;
            rc.right = rc.left + nWidth;
        }
        else
        {
	    // size buddy to left
	    rcBuddy.right -= nWidth - nOver;
            rc.left = rc.right - nWidth;
        }
	// size the buddy to fit the updown on the appropriate side
	MoveWindow(np->hwndBuddy, rcBuddy.left, rcBuddy.top,
				rcBuddy.right - rcBuddy.left, nHeight, TRUE);
    }
    else if (!(np->style & UDS_HORZ))
    {
        nWidth = g_cxVScroll + 2 * nHasBorder;
    }

    SetWindowPos(np->hwnd, NULL, rc.left, rc.top, nWidth, nHeight,
        SWP_DRAWFRAME | SWP_NOZORDER | SWP_NOACTIVATE);
}


// Use this to make any and all comparisons involving the nPos,
// nUpper or nLower fields of the PUDSTATE. It determines
// whether to do a signed or unsigned comparison and returns
//  > 0 for (x > y)
//  < 0 for (x < y)
// == 0 for (x == y).

int NEAR PASCAL compare(PUDSTATE np, int x, int y, UINT fCompareType)
{
    if ((fCompareType == UNSIGNED) || ((np->fUnsigned) && !(fCompareType == SIGNED)) )
    {
        // Do unsigned comparisons
        if ((UINT)x > (UINT)y)
            return 1;
        else if ((UINT)x < (UINT)y)
            return -1;
    }
    else
    {
        // Do signed comparisons
        if (x > y)
            return 1;
        else if (x < y)
            return -1;
    }

    return 0;
}

// Use this after any pos change to make sure pos stays in range.
// Wraps as necessary.
//
BOOL NEAR PASCAL nudge(PUDSTATE np)
{
    BOOL bOutOfRange = TRUE;
    int min = np->nUpper;
    int max = np->nLower;

    if (compare(np,max,min, DONTCARE) < 0)
    { 
        int t; 
        t = min; 
        min = max; 
        max = t; 
    }

    if (np->style & UDS_WRAP)
    {
        if ((compare(np, np->nPos, min, np->fUnsigned ? UNSIGNED : SIGNED) < 0))
            np->nPos = max;
        else if ((compare(np, np->nPos, max, np->fUnsigned ? UNSIGNED : SIGNED) > 0))
            np->nPos = min;
        else bOutOfRange = FALSE;
    }
    else
    {
        if (compare(np,np->nPos,min, DONTCARE) < 0) 
            np->nPos = min;
        else if (compare(np,np->nPos,max, DONTCARE) > 0) 
            np->nPos = max;
        else 
            bOutOfRange = FALSE;
    }

    return(bOutOfRange);
}

// Sets the state of the buttons (pushed, released).
//
void NEAR PASCAL squish(PUDSTATE np, UINT bTop, UINT bBottom)
{
    BOOL bInvalidate = FALSE;

    if (np->nUpper == np->nLower || !IsWindowEnabled(np->hwnd))
    {
        bTop = FALSE;
        bBottom = FALSE;
    }
    else
    {
        bTop = !!bTop;
        bBottom = !!bBottom;
    }

    if (np->fUp != bTop)
    {
        np->fUp = bTop;
        bInvalidate = TRUE;
    }
    if (np->fDown != bBottom)
    {
        np->fDown = bBottom;
        bInvalidate = TRUE;
    }

    if (bInvalidate)
    {
        np->dwStart = GetTickCount();
        InvalidateRect(np->hwnd, NULL, FALSE);
    }
}

// Gets the intl 1000 separator
//
void NEAR PASCAL getthousands(LPSTR pszThousand)
{
#ifdef WIN32
    if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, pszThousand, 2))
    {
        pszThousand[0] = ',';
        pszThousand[1] = 0;
    }
#else
    static DWORD uLast = 0;
    static char cThou;
    DWORD uNow;

    /* Only check the intl setting every 5 seconds.
     */
    uNow = GetTickCount();
    if (uNow - uLast > 5000)
    {
        if (!GetProfileString("intl", "sThousand", pszThousand, pszThousand, 2))
	{
            pszThousand[0] = ',';
            pszThousand[1] = 0;
	}
        cThou = pszThousand[0];
        uLast = uNow;
    }
    else
    {
        pszThousand[0] = cThou;
        pszThousand[1] = 0;
    }

#endif
}

// Gets the caption of the buddy
//
LRESULT NEAR PASCAL getint(PUDSTATE np)
{
    char szInt[MAX_INTLENGTH]; // big enough for all intl stuff, too
    char szThousand[2];
    char cTemp;
    int nPos;
    int sign = 1;
    LPSTR p = szInt;
    BOOL bInValid = TRUE;

    isgoodbuddy(np);
    if (np->hwndBuddy && np->style & UDS_SETBUDDYINT)
    {
        if (np->uClass == CLASS_LISTBOX)
        {
            np->nPos = (int)SendMessage(np->hwndBuddy, LB_GETCURSEL, 0, 0L);
            bInValid = nudge(np);
        }
        else
        {
            GetWindowText(np->hwndBuddy, szInt, sizeof(szInt));

            switch (np->nBase)
            {
                case BASE_HEX:
                    if ((*p == 'x') || (*p == 'X'))
                        // ignore first character
                        p++;
                    else if ((*p == '0') && ((*(p + 1) == 'x') || (*(p + 1) == 'X')))
                        // ignore first two characters ("0x" or "0X")
                        p += 2;

                    for (nPos = 0; *p; p++)
                    {
                        if ((*p >= 'A') && (*p <= 'F'))
                            cTemp = (char)(*p - 'A' + 10);
                        else if ((*p >= 'a') && (*p <= 'f'))
                            cTemp = (char)(*p - 'a' + 10);
                        else if ((*p >= '0') && (*p <= '9'))
                            cTemp = (char)(*p - '0');
                        else
                            goto BadValue;

                        nPos = (nPos * 16) + cTemp;
                    }
                    np->nPos = nPos;
                    break;

                case BASE_DECIMAL:
        default:
                    getthousands(szThousand);
                    if (*p == '-')
                    {
                        sign = -1;
                        ++p;
                    }

                    for (nPos=0; *p; p++)
                    {
                        cTemp = *p;

                        // If there is a thousand separator, make sure it is in the
                        // right place
                        if (cTemp == szThousand[0] && lstrlen(p)==4)
                        {
                            continue;
                        }

                        cTemp -= '0';
                        if ((UINT)cTemp > 9)
                        {
                            goto BadValue;
                        }
                        nPos = (nPos*10) + cTemp;
                    }

                    np->nPos = nPos*sign;
                    break;
            }
            bInValid = nudge(np);
        }
    }

BadValue:
    return(MAKELRESULT(np->nPos, bInValid));
}

// Sets the caption of the buddy if appropriate.
//
void NEAR PASCAL setint(PUDSTATE np)
{
    static int cReenter = 0;

    char szInt[MAX_INTLENGTH];
    char szThousand[2];
    int pos = np->nPos;
    LPSTR p = szInt;

    isgoodbuddy(np);
    if (np->hwndBuddy && np->style & UDS_SETBUDDYINT)
    {
        /* If we have reentered, then maybe the app has set up a loop.
         * Check to see if the value really needs to be set.
         */
        if (cReenter && (LRESULT)pos==getint(np))
        {
            return;
        }
        np->nPos = pos;

        ++cReenter;

        if (np->uClass == CLASS_LISTBOX)
        {
            SendMessage(np->hwndBuddy, LB_SETCURSEL, pos, 0L);
            FORWARD_WM_COMMAND(GetParent(np->hwndBuddy),
                                GetDlgCtrlID(np->hwndBuddy),
                np->hwndBuddy, LBN_SELCHANGE, SendMessage);
        }
        else
        {
            switch (np->nBase)
            {
                case BASE_HEX:
                    wsprintf(p, "0x%04X", pos);
                    break;

                case BASE_DECIMAL:
    	        default:
                    if (pos < 0)
                    {
                        *p++ = '-';
                        pos = -pos;
                    }

                    if (pos >= 1000 && !(np->style & UDS_NOTHOUSANDS))
                    {
                        getthousands(szThousand);
                        p += wsprintf(p, "%d", pos / 1000);
                        if (szThousand[0])
                            *p++ = szThousand[0];
                        wsprintf(p, "%03d", pos % 1000);
                    }
                    else
                    {
                        wsprintf(p, "%d", pos);
                    }
                    break;
            }

            SetWindowText(np->hwndBuddy, szInt);
        }

        --cReenter;
    }
}

// Use this to click the pos up or down by one.
//
void NEAR PASCAL bump(PUDSTATE np)
{
    BOOL bChanged = FALSE;
    UINT uElapsed, increment;
    int direction, i;

    /* So I'm not really getting seconds here; it's close enough, and
     * dividing by 1024 keeps __aFuldiv from being needed.
     */
    uElapsed = (UINT)((GetTickCount() - np->dwStart) / 1024);

    increment = np->udAccel[0].nInc;
    for (i = np->nAccel - 1; i >= 0; --i)
    {
        if (np->udAccel[i].nSec <= uElapsed)
        {
            increment = np->udAccel[i].nInc;
            break;
        }
    }

    if (increment == 0)
    {
        DebugMsg(DM_ERROR, "bad accelerator value");
        return;
    }

    direction = compare(np,np->nUpper,np->nLower, DONTCARE) < 0 ? -1 : 1;
    if (np->fUp)
    {
        bChanged = TRUE;
    }
    if (np->fDown)
    {
        direction = -direction;
        bChanged = TRUE;
    }

    if (bChanged)
    {
        /* Make sure we have a multiple of the increment
         * Note that we should loop only when the increment changes
         */
        NM_UPDOWN nm;

        nm.iPos = np->nPos;
        nm.iDelta = increment * direction;
        if (SendNotify(np->hwndParent, np->hwnd, UDN_DELTAPOS, &nm.hdr))
            return;

        np->nPos += nm.iDelta;
        for ( ; ; )
        {
            if (!(np->nPos % increment))
            {
                break;
            }
            np->nPos += direction;
        }

        nudge(np);
        setint(np);
        if (np->style & UDS_HORZ)
	    FORWARD_WM_HSCROLL(np->hwndParent, np->hwnd, SB_THUMBPOSITION, np->nPos, SendMessage);
	else
	    FORWARD_WM_VSCROLL(np->hwndParent, np->hwnd, SB_THUMBPOSITION, np->nPos, SendMessage);
    }
}

//#pragma data_seg(DATASEG_READONLY)
const char c_szEdit[] = "edit";
const char c_szListbox[] = "listbox";
//#pragma data_seg()

// Sets the new buddy
//
LRESULT NEAR PASCAL setbuddy(PUDSTATE np, HWND hwndBuddy)
{
    HWND hOldBuddy;
    char szClName[10];

    hOldBuddy = np->hwndBuddy;
    if (hOldBuddy && GetProp(hOldBuddy, s_szUpdownClass))
    {
        SetWindowLong(hOldBuddy, GWL_WNDPROC, (LONG)np->lpfnDefProc);
        RemoveProp(hOldBuddy, s_szUpdownClass);
    }

    np->hwndBuddy = hwndBuddy;
    if (!hwndBuddy)
    {
        pickbuddy(np);
        hwndBuddy = np->hwndBuddy;
    }

    np->uClass = CLASS_UNKNOWN;
    if (hwndBuddy)
    {
        if (np->style & UDS_ARROWKEYS)
        {
	    if (GetProp(hwndBuddy, s_szUpdownClass) == NULL)
	    {
                np->lpfnDefProc = (WNDPROC)SetWindowLong(hwndBuddy, GWL_WNDPROC, (LONG)ArrowKeyProc);
                SetProp(hwndBuddy, s_szUpdownClass, (HANDLE)np);
	    }
	    else
	    {
	        DebugMsg(DM_ERROR, "SetBuddy called on already subclassed buddy");
	    }
        }

        GetClassName(hwndBuddy, szClName, sizeof(szClName));
        if (!lstrcmpi(szClName, c_szEdit))
        {
            np->uClass = CLASS_EDIT;
        }
        else if (!lstrcmpi(szClName, c_szListbox))
        {
            np->uClass = CLASS_LISTBOX;
        }
    }

    anchor(np);
    return MAKELRESULT(hOldBuddy, 0);
}


// Paint the whole control
//
void NEAR PASCAL PaintUpDownControl(PUDSTATE np, HDC hdc)
{
    PAINTSTRUCT ps;
    RECT rcBtn;
    RECT rc;

    BOOL bEnabled = (np->nUpper != np->nLower) && IsWindowEnabled(np->hwnd);

    if (np->hwndBuddy)
        bEnabled = bEnabled && IsWindowEnabled(np->hwndBuddy);

    if (hdc)
        ps.hdc = hdc;
    else
        BeginPaint(np->hwnd, &ps);

    GetClientRect(np->hwnd, &rcBtn);

    // if we are autobuddy'd and anchored to a sunken-edge control, we draw the
    // "nonclient" area of ourselves to blend in with our buddy.
    if (np->fSharedBorder && np->fSunkenBorder)
    {
        UINT bf = BF_TOP | BF_BOTTOM | BF_ADJUST |
            (np->style & UDS_ALIGNLEFT ? BF_LEFT : 0) |
            (np->style & UDS_ALIGNRIGHT ? BF_RIGHT : 0);
        DrawEdge(ps.hdc, &rcBtn, EDGE_SUNKEN, bf);
    }

    // with remaining space, draw appropriate scrollbar arrow controls in
    // upper and lower halves

    rc = rcBtn;
        if (np->style & UDS_HORZ)
        {
                // Horizontal ones
        rc.right = (rcBtn.right + rcBtn.left) / 2;
        DrawFrameControl(ps.hdc, &rc, DFC_SCROLL,
                 (DFCS_SCROLLLEFT |
                 (np->fDown ? DFCS_PUSHED : 0) |
                 (bEnabled ? 0 : DFCS_INACTIVE)));
        rc.left = rcBtn.right - (rc.right - rc.left); // handles odd-x case, too
        rc.right = rcBtn.right;
        DrawFrameControl(ps.hdc, &rc, DFC_SCROLL,
                 (DFCS_SCROLLRIGHT |
                 (np->fUp ? DFCS_PUSHED : 0) |
                 (bEnabled ? 0 : DFCS_INACTIVE)));
        }
        else
        {
        rc.bottom = (rcBtn.bottom + rcBtn.top) / 2;
        DrawFrameControl(ps.hdc, &rc, DFC_SCROLL,
                 (DFCS_SCROLLUP |
                 (np->fUp ? DFCS_PUSHED : 0) |
                 (bEnabled ? 0 : DFCS_INACTIVE)));
        rc.top = rcBtn.bottom - (rc.bottom - rc.top); // handles odd-y case, too
        rc.bottom = rcBtn.bottom;
        DrawFrameControl(ps.hdc, &rc, DFC_SCROLL,
                 (DFCS_SCROLLDOWN |
                 (np->fDown ? DFCS_PUSHED : 0) |
                 (bEnabled ? 0 : DFCS_INACTIVE)));
        }

    if (hdc == NULL)
        EndPaint(np->hwnd, &ps);
}


LRESULT CALLBACK ArrowKeyProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PUDSTATE np = (PUDSTATE)GetProp(hWnd, s_szUpdownClass);
    WNDPROC  lpfnDefProc;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        /* Restore the window proc.
         */
        lpfnDefProc = np->lpfnDefProc;
        SetWindowLong(hWnd, GWL_WNDPROC, (LONG)lpfnDefProc);
        RemoveProp(hWnd, s_szUpdownClass);
        np->hwndBuddy = NULL;
        np->lpfnDefProc = NULL;
        if (np->fUpDownDestroyed)
        {
            // The buddy was destroyed after updown so free the memory now
            // And pass off to the message to who we subclassed...
            LocalFree((HLOCAL)np);
        }
        return CallWindowProc(lpfnDefProc, hWnd, uMsg, wParam, lParam);

    case WM_GETDLGCODE:
        return CallWindowProc(np->lpfnDefProc, hWnd, uMsg, wParam, lParam) | DLGC_WANTARROWS;

    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_UP:
        case VK_DOWN:
            if (GetCapture() != np->hwnd)
            {
                /* Get the value from the buddy if this is the first key down
                 */
                if (!(lParam&(1L<<30)))
                {
                    getint(np);
                }

                /* Update the visuals and bump the value
                 */
                np->bDown = (wParam == VK_DOWN);
                squish(np, !np->bDown, np->bDown);
                bump(np);
            }
            return(0L);

        default:
            break;
        }
        break;

    case WM_KEYUP:
        switch (wParam)
        {
        case VK_UP:
        case VK_DOWN:
            if (GetCapture() != np->hwnd)
            {
                squish(np, FALSE, FALSE);
            }
            return(0L);

        default:
            break;
        }
        break;

    case WM_CHAR:
        switch (wParam)
        {
        case VK_UP:
        case VK_DOWN:
            return(0L);

        default:
            break;
        }
        break;

    default:
        break;
    }

    return CallWindowProc(np->lpfnDefProc, hWnd, uMsg, wParam, lParam);
}

UINT NEAR PASCAL setbase(PUDSTATE np, UINT wNewBase)
{
    UINT wOldBase;

    switch (wNewBase)
    {
        case BASE_DECIMAL:
        case BASE_HEX:
            np->fUnsigned = (wNewBase != BASE_DECIMAL);
            wOldBase = np->nBase;
            np->nBase = wNewBase;
            setint(np);
            return wOldBase;
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////

HWND WINAPI CreateUpDownControl(DWORD dwStyle, int x, int y, int cx, int cy,
                                HWND hParent, int nID, HINSTANCE hInst,
                                HWND hwndBuddy, int nUpper, int nLower, int nPos)
{
    HWND hWnd = CreateWindow(s_szUpdownClass, NULL, dwStyle, x, y, cx, cy,
                             hParent, (HMENU)nID, hInst, 0L);
    if (hWnd)
    {
        SendMessage(hWnd, UDM_SETBUDDY, (WPARAM)hwndBuddy, 0L);
        SendMessage(hWnd, UDM_SETRANGE, 0, MAKELONG(nUpper, nLower));
        SendMessage(hWnd, UDM_SETPOS, 0, MAKELONG(nPos, 0));
    }
    return hWnd;
}

/////////////////////////////////////////////////////////////////////////////

// UpDownWndProc:
//
LRESULT CALLBACK UpDownWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    int i;
    PUDSTATE np = (PUDSTATE)GetWindowInt(hWnd, 0);

    switch (uMsg)
    {
    case WM_LBUTTONDOWN:
    {
        // Don't set a timer if on the middle border
        BOOL bTimeIt = TRUE;

        if (np->hwndBuddy && !IsWindowEnabled(np->hwndBuddy))
            break;

        SetCapture(hWnd);
        getint(np);

        switch (np->uClass)
        {
        case CLASS_EDIT:
        case CLASS_LISTBOX:
            SetFocus(np->hwndBuddy);
            break;
        }

        GetClientRect(hWnd, &rc);
                if (np->style & UDS_HORZ)
                {
                        // Horizontal placement
                if ((int)LOWORD(lParam) < (rc.right / 2))
                {
                    np->bDown = TRUE;
                    squish(np, FALSE, TRUE);
                }
                else if ((int)LOWORD(lParam) > (rc.right / 2))
                {
                    np->bDown = FALSE;
                    squish(np, TRUE, FALSE);
                }
                else
                bTimeIt = FALSE;
                }
                else
                {
                if ((int)HIWORD(lParam) > (rc.bottom / 2))
                {
                    np->bDown = TRUE;
                    squish(np, FALSE, TRUE);
                }
                else if ((int)HIWORD(lParam) < (rc.bottom / 2))
                {
                    np->bDown = FALSE;
                    squish(np, TRUE, FALSE);
                }
                else
                bTimeIt = FALSE;
                }

        if (bTimeIt)
        {
            SetTimer(hWnd, 1, GetProfileInt("windows", "CursorBlinkRate", 530), NULL);
            bump(np);
        }
        break;
    }

    case WM_TIMER:
    {
        POINT pt;

        if (GetCapture() != hWnd)
        {
            goto EndScroll;
        }

        SetTimer(hWnd, 1, 100, NULL);

        GetWindowRect(hWnd, &rc);
        i = (rc.top + rc.bottom) / 2;
        if (np->bDown)
        {
            rc.top = i;
        }
        else
        {
            rc.bottom = i;
        }

        InflateRect(&rc, (g_cxFrame+1)/2, (g_cyFrame+1)/2);
        GetCursorPos(&pt);
        if (PtInRect(&rc, pt))
        {
            squish(np, !np->bDown, np->bDown);
            bump(np);
        }
        else
        {
            squish(np, FALSE, FALSE);
        }
        break;
    }

    case WM_LBUTTONUP:
        if (np->hwndBuddy && !IsWindowEnabled(np->hwndBuddy))
            break;

        if (GetCapture() == hWnd)
        {
EndScroll:
            squish(np, FALSE, FALSE);
            ReleaseCapture();
            KillTimer(hWnd, 1);

            if (np->uClass == CLASS_EDIT)
                Edit_SetSel(np->hwndBuddy, 0, -1);

                        if (np->style & UDS_HORZ)
                            FORWARD_WM_HSCROLL(np->hwndParent, np->hwnd,
                                      SB_ENDSCROLL, np->nPos, SendMessage);
                        else
                            FORWARD_WM_VSCROLL(np->hwndParent, np->hwnd,
                                      SB_ENDSCROLL, np->nPos, SendMessage);
        }
        break;

    case WM_ENABLE:
        InvalidateRect(hWnd, NULL, TRUE);
        break;

    case WM_WININICHANGE:
        if (!wParam ||
            (wParam == SPI_SETNONCLIENTMETRICS) ||
            (wParam == SPI_SETICONTITLELOGFONT)) {
            InitGlobalMetrics(wParam);
            unachor(np);
            anchor(np);
        }
        break;

    case WM_PRINTCLIENT:
    case WM_PAINT:
        PaintUpDownControl(np, (HDC)wParam);
        break;

    case UDM_SETRANGE:
        np->nUpper = (int)(SHORT)LOWORD(lParam);
        np->nLower = (int)(SHORT)HIWORD(lParam);
        nudge(np);
        break;

    case UDM_GETRANGE:
        return MAKELONG(np->nUpper, np->nLower);

    case UDM_SETBASE:
        // wParam: new base
        // lParam: not used
        // return: 0 if invalid base is specified,
        //         previous base otherwise
        return (LRESULT)setbase(np, (UINT)wParam);

    case UDM_GETBASE:
        return np->nBase;

    case UDM_SETPOS:
    {
        int iNewPos = (int)(SHORT)LOWORD(lParam);
        if (compare(np, np->nLower, np->nUpper, DONTCARE) < 0) {

            if (compare(np, iNewPos, np->nUpper, DONTCARE) > 0) {
                iNewPos = np->nUpper;
            }

            if (compare(np, iNewPos, np->nLower, DONTCARE) < 0) {
                iNewPos = np->nLower;
            }
        } else {
            if (compare(np, iNewPos, np->nUpper, DONTCARE) < 0) {
                iNewPos = np->nUpper;
            }

            if (compare(np, iNewPos, np->nLower, DONTCARE) > 0) {
                iNewPos = np->nLower;
            }
        }

        i = np->nPos;
        np->nPos = iNewPos;
        setint(np);
        return (LRESULT)i;
    }

    case UDM_GETPOS:
        return getint(np);

    case UDM_SETBUDDY:
        return setbuddy(np, (HWND)wParam);

    case UDM_GETBUDDY:
        return (LRESULT)(int)np->hwndBuddy;

    case UDM_SETACCEL:
        if (wParam == 0)
            return(FALSE);
        if (wParam >= NUM_UDACCELS)
        {
	    HANDLE npPrev = (HANDLE)np;

            np = (PUDSTATE)LocalReAlloc((HLOCAL)np, sizeof(UDSTATE)+(wParam-NUM_UDACCELS)*sizeof(UDACCEL),
                LMEM_MOVEABLE);
            if (!np)
            {
                return(FALSE);
            } 
            else 
            {
                SetWindowInt(hWnd, 0, (int)np);

	    if ((np->style & UDS_ARROWKEYS) && np->hwndBuddy)
	    {
		// Update the property of our buddy.
		if ((HANDLE)GetProp(np->hwndBuddy, s_szUpdownClass) == npPrev)
		{
		    RemoveProp(np->hwndBuddy, s_szUpdownClass);
		    SetProp(np->hwndBuddy, s_szUpdownClass, (HANDLE)np);
		}
	    }
            }
        }

        np->nAccel = wParam;
        for (i = 0; i < (int)wParam; ++i)
        {
            np->udAccel[i] = ((LPUDACCEL)lParam)[i];
        }
        return(TRUE);

    case UDM_GETACCEL:
        if (wParam > np->nAccel)
        {
            wParam = np->nAccel;
        }
        for (i=0; i<(int)wParam; ++i)
        {
            ((LPUDACCEL)lParam)[i] = np->udAccel[i];
        }
        return(np->nAccel);


    case WM_CREATE:
        // Allocate the instance data space.
        np = (PUDSTATE)LocalAlloc(LPTR, sizeof(UDSTATE));
        if (!np)
            return -1;

        SetWindowInt(hWnd, 0, (int)np);

            #define lpCreate ((CREATESTRUCT FAR *)lParam)

        np->hwnd = hWnd;
            np->hwndParent = lpCreate->hwndParent;
            np->style = lpCreate->style;

        // np->fUp =
        // np->fDown =
            // np->fUnsigned =
            // np->fSharedBorder =
            // np->fSunkenBorder =
        //  FALSE;

        if (lpCreate->dwExStyle & WS_EX_CLIENTEDGE)
            np->fSunkenBorder = TRUE;

        np->nBase = BASE_DECIMAL;
        np->nUpper = 0;
        np->nLower = 100;
        np->nPos = 0;
        np->hwndBuddy = NULL;
        np->uClass = CLASS_UNKNOWN;

            np->nAccel = NUM_UDACCELS;
            np->udAccel[0].nSec = 0;
            np->udAccel[0].nInc = 1;
        np->udAccel[1].nSec = 2;
            np->udAccel[1].nInc = 5;
            np->udAccel[2].nSec = 5;
            np->udAccel[2].nInc = 20;

        /* This does the pickbuddy and anchor
         */
        setbuddy(np, NULL);
        setint(np);
        break;

    case WM_DESTROY:
        if (np) {
            if (np->hwndBuddy || np->lpfnDefProc)
            {
                // Make sure that our buddy is unsubclassed.
	        DebugMsg(DM_ERROR, "UpDown Destroyed while buddy subclassed");
                np->fUpDownDestroyed = TRUE;
            }
            else
                LocalFree((HLOCAL)np);
            SetWindowInt(hWnd, 0, 0);
        }
        break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0L;
}

/////////////////////////////////////////////////////////////////////////////

// InitUpDownClass:
// Adds our WNDCLASS to the system.
//
#pragma code_seg(CODESEG_INIT)

BOOL FAR PASCAL InitUpDownClass(HINSTANCE hInst)
{
    WNDCLASS wndclass;

        if (!GetClassInfo(hInst, s_szUpdownClass, &wndclass))
        {
#ifndef WIN32
	    extern LRESULT CALLBACK _UpDownWndProc(HWND, UINT, WPARAM, LPARAM);
            wndclass.lpfnWndProc    = _UpDownWndProc;
#else
            wndclass.lpfnWndProc    = (WNDPROC)UpDownWndProc;
#endif
            wndclass.lpszClassName  = s_szUpdownClass;
            wndclass.hInstance  = hInst;
            wndclass.hCursor    = LoadCursor(NULL, IDC_ARROW);
            wndclass.hIcon      = NULL;
            wndclass.lpszMenuName   = NULL;
            wndclass.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
            wndclass.style      = CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
            wndclass.cbClsExtra = 0;
            wndclass.cbWndExtra = sizeof(PUDSTATE);

            return RegisterClass(&wndclass);
        }
        return TRUE;
}
#pragma code_seg()

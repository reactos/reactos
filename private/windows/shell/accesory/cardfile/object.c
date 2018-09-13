#include "precomp.h"

/************************************************************************/
/*                                                                      */
/*  Windows Cardfile - Written by Mark Cliggett                         */
/*  (c) Copyright Microsoft Corp. 1985, 1994 - All Rights Reserved      */
/*                                                                      */
/************************************************************************/

POINT   dragPt;
RECT    dragRect;
int     fBMDown;
int     xmax;
int     ymax;


NOEXPORT void NEAR TurnOnEnclosure(
    void);

NOEXPORT void NEAR TurnOffEnclosure(
    void);

NOEXPORT void NEAR XorEnclosure(
    void);

NOEXPORT void NEAR DrawXorRect(
    HDC hDC,
    RECT *pRect);

/*
 * call this for every mouse action when in place picture mode
 * OLE modification:  Move the object around, or dblclk == primary verb
 */
#if defined(WIN32)
void BMMouse(
    HWND hWindow,
    UINT message,
    WPARAM wParam,
    MYPOINT pts)
#else
void BMMouse(
    HWND hWindow,
    UINT message,
    WPARAM wParam,
    MYPOINT pt)
#endif
{
    RECT rect;
    HDC hDC;
    int t;
#if defined(WIN32)
    POINT pt ;

    MYPOINTTOPOINT( pt, pts ) ;
#endif

    switch(message) 
    {
        case WM_LBUTTONDOWN:            /* start drag */
        /* No movement if there is no object */
            if (CurCard.lpObject && PtInRect(&(CurCard.rcObject), pt))
            {
                SetCapture(hWindow);
                GetClientRect(hWindow, &rect);
                xmax = rect.right - CharFixWidth;
                ymax = rect.bottom - CharFixHeight;
                fBMDown = TRUE;
                dragPt = pt;
                dragRect = CurCard.rcObject;
                TurnOnEnclosure();
            }
            break;

        case WM_LBUTTONDBLCLK:      /* Activate the object editor */
            /*
             * Perform the same operation as the "Activate" menu item.
             * (but only if dealing with activatable objects!)
             */
            if (fOLE && CurCard.lpObject && PtInRect(&(CurCard.rcObject), pt))
            {
                /* Alt+DblClk for links => Links dialog.
                 * Just DblClk for embedded objects => primary verb */
                if (GetKeyState(VK_MENU) < 0 && CurCard.otObject == LINK)
                    PostMessage(hIndexWnd, WM_COMMAND, LINKSDIALOG, 0L);
                else if (!(GetKeyState(VK_MENU) < 0))   /* Alt is not DOWN */
                {
                    if (CurCard.otObject != STATIC)
                    {
                        PicSaveUndo(&CurCard);
                        PostMessage(hIndexWnd, WM_COMMAND, PLAY, 0L);
                    }
                    else
                        ErrorMessage(W_STATIC_OBJECT);
                }
            }
            break;   

        case WM_LBUTTONUP:            /* end drag */
            if (fBMDown) 
            {
                ReleaseCapture();
                /* If the dragged rectangle actually moved, repaint image */
                if (dragRect.top != CurCard.rcObject.top
                     || dragRect.left != CurCard.rcObject.left) 
                {
                    InvalidateRect(hEditWnd, &(CurCard.rcObject), TRUE);
                    InvalidateRect(hEditWnd, &dragRect, TRUE);
                    CurCard.rcObject = dragRect;
                    CurCardHead.flags |= FDIRTY;
                }
                fBMDown = FALSE;
                TurnOffEnclosure();
            }
            break;

        case WM_MOUSEMOVE:            /* move rect if dragging */
            /*
             * This logic could be replaced by an IntersectRect() call,
             * but that would probably slow it down.
             */    
            if (fBMDown) 
            {
                /* Displace left coord by amount the point was dragged */
                t = dragRect.left + pt.x - dragPt.x;
                if (t > xmax)
                    pt.x = xmax - dragRect.left + dragPt.x;
                else if (t < CharFixWidth - (dragRect.right - dragRect.left))
                    pt.x = CharFixWidth - dragRect.right + dragPt.x;

                /* Displace top coord by amount the point was dragged */
                t = dragRect.top + pt.y - dragPt.y;
                if (t > ymax)
                    pt.y = ymax - dragRect.top + dragPt.y;
                else if (t < CharFixHeight - (dragRect.bottom - dragRect.top))
                    pt.y = CharFixHeight - dragRect.bottom + dragPt.y;

                /* Only draw if we moved (stop at one char from edges) */
                if (dragPt.x != pt.x || dragPt.y != pt.y) 
                {
                    hDC = GetDC(hEditWnd);
                    DrawXorRect(hDC, &dragRect);
                    OffsetRect(&dragRect, pt.x - dragPt.x, pt.y - dragPt.y);
                    dragPt = pt;
                    DrawXorRect(hDC, &dragRect);
                    ReleaseDC(hEditWnd, hDC);
                }
            }
            break;
    }
}

/* 
 * Toggle the drag rectangle 
 */
NOEXPORT void NEAR TurnOnEnclosure(
    void)
{
     XorEnclosure();
}

NOEXPORT void NEAR TurnOffEnclosure(
    void)
{
    XorEnclosure();
}

NOEXPORT void NEAR XorEnclosure(
    void) 
{
    HDC hDC;

    hDC = GetDC(hEditWnd);
    DrawXorRect(hDC, &dragRect);
    ReleaseDC(hEditWnd, hDC);
}

/*
 * is this as fast as it could be?
 */
NOEXPORT void NEAR DrawXorRect(
    HDC hDC,
    RECT *pRect)
{
    /* NOTE!!!  There is a high probability that you should use
     * WHITE_BRUSH and/or R2_XORPEN.  Why?  Try a different app
     * workspace background color.
     */     

    HBRUSH hOldBrush = SelectObject(hDC, GetStockObject(NULL_BRUSH));
    short OldRop = SetROP2(hDC, R2_NOT);

    Rectangle(hDC, pRect->left, pRect->top, pRect->right, pRect->bottom);

    SelectObject(hDC, hOldBrush);
    SetROP2(hDC, OldRop);
}

BOOL BMKey( WORD wParam )
{
    int x;
    int y;
    BOOL fShift;
    BOOL fControl;
    WORD wEditParam;

    dragRect = CurCard.rcObject;
    x = CurCard.rcObject.left;
    y = CurCard.rcObject.top;

    switch(wParam)
    {
        case VK_INSERT:
        case VK_DELETE:
            /* Hack in accelerators for standard edit functions when in
             * picture mode.  These used to not be needed, since these
             * keys were truly accelerators, but when the new functionality
             * was added to the edit control, these keys were required to
             * go directly to the edit control, and thus could not be accels.
             * This fix was added to still support these accels in picture mode.
             */
            fShift = GetKeyState(VK_SHIFT) < 0;
            fControl = GetKeyState(VK_CONTROL) < 0;
            if (wParam == VK_DELETE && fShift && !fControl)
                wEditParam = CUT;
            else if (wParam == VK_INSERT && !fShift && fControl)
                wEditParam = COPY;
            else if (wParam == VK_INSERT && fShift && !fControl)
                wEditParam = PASTE;
            else
                return(FALSE);
            /* To be faster, we could directly call IndexInput */
            PostMessage(hIndexWnd, WM_COMMAND, wEditParam, 0L);
            return(TRUE);

        case VK_UP:
            y -= CharFixHeight;
            break;
        case VK_DOWN:
            y += CharFixHeight;
            break;
        case VK_LEFT:
            x -= CharFixWidth;
            break;
        case VK_RIGHT:
            x += CharFixWidth;
            break;
        default:
            return(FALSE);
    }

    if (x > (LINELENGTH-1) * CharFixWidth)
        x = (LINELENGTH-1) * CharFixWidth;
    else if (x < CharFixWidth - (dragRect.right - dragRect.left))
        x = CharFixWidth - (dragRect.right - dragRect.left);

    if (y > 10 * CharFixHeight)
        y = 10 * CharFixHeight;
    else if (y < CharFixHeight - (dragRect.bottom - dragRect.top))
        y = CharFixHeight - (dragRect.bottom - dragRect.top);

    if (x != CurCard.rcObject.left || y != CurCard.rcObject.top)
    {
        InvalidateRect(hEditWnd, &(CurCard.rcObject), TRUE);
        OffsetRect(&(CurCard.rcObject), x-dragRect.left, y-dragRect.top);
        InvalidateRect(hEditWnd, &(CurCard.rcObject), TRUE);
    }
    CurCardHead.flags |= FDIRTY;
    return(TRUE);
}

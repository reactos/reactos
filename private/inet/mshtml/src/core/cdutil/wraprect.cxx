//
// Trident 16 bit source only.
//
// Provides wrappers for some common GID Apis that take long rectangles in 32bit
// world and take small rectangles in 16bit world.

#include "headers.hxx"

void    WINAPI ClientToScreen(HWND hwnd, POINTL FAR* lpp)
{
    ClientToScreen(hwnd, (CPointAutoconvert) lpp);
}

void    WINAPI ScreenToClient(HWND hwnd, POINTL FAR* lpp)
{
    ScreenToClient(hwnd, (CPointAutoconvert) lpp);
}

void    WINAPI MapWindowPoints(HWND hwndFrom, HWND hwndTo, POINTL FAR* lppt, UINT cpt)
{
    CPointAutoconvertArray aryPts(lppt, cpt);
    MapWindowPoints(hwndFrom, hwndTo, aryPts, cpt);
}

HWND    WINAPI WindowFromPoint(POINTL ptl)
{
    POINTword pt={ptl.x, ptl.y};
    return WindowFromPoint(pt);
}

HWND    WINAPI ChildWindowFromPoint(HWND hwnd, POINTL ptl)
{
    POINTword pt={ptl.x, ptl.y};
    return ChildWindowFromPoint(hwnd, pt);
}

void    WINAPI GetCursorPos(POINTL FAR* lpp)
{
    // could save stack space and actually use lpp, but I won't!
    POINTword pt;
    GetCursorPos(&pt);
    lpp->x = pt.x;
    lpp->y = pt.y;
}

void    WINAPI GetCaretPos(POINTL FAR* lpp)
{
    POINTword pt;
    GetCaretPos(&pt);
    lpp->x = pt.x;
    lpp->y = pt.y;
}


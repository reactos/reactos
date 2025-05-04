#include "desk.h"
#include <uxtheme.h>
#include <vsstyle.h>
#include <vssym32.h>

static INT
LengthOfStrResource(IN HINSTANCE hInst,
                    IN UINT uID)
{
    HRSRC hrSrc;
    HGLOBAL hRes;
    LPWSTR lpName, lpStr;

    if (hInst == NULL)
    {
        return -1;
    }

    /* There are always blocks of 16 strings */
    lpName = (LPWSTR)MAKEINTRESOURCE((uID >> 4) + 1);

    /* Find the string table block */
    if ((hrSrc = FindResourceW(hInst, lpName, (LPWSTR)RT_STRING)) &&
        (hRes = LoadResource(hInst, hrSrc)) &&
        (lpStr = LockResource(hRes)))
    {
        UINT x;

        /* Find the string we're looking for */
        uID &= 0xF; /* Position in the block, same as % 16 */
        for (x = 0; x < uID; x++)
        {
            lpStr += (*lpStr) + 1;
        }

        /* Found the string */
        return (int)(*lpStr);
    }
    return -1;
}

INT
AllocAndLoadString(OUT LPTSTR *lpTarget,
                   IN HINSTANCE hInst,
                   IN UINT uID)
{
    INT ln;

    ln = LengthOfStrResource(hInst,
                             uID);
    if (ln++ > 0)
    {
        (*lpTarget) = (LPTSTR)LocalAlloc(LMEM_FIXED,
                                         ln * sizeof(TCHAR));
        if ((*lpTarget) != NULL)
        {
            INT Ret;
            if (!(Ret = LoadString(hInst, uID, *lpTarget, ln)))
            {
                LocalFree((HLOCAL)(*lpTarget));
            }
            return Ret;
        }
    }
    return 0;
}

static void
DrawEtchedLine(HTHEME hTheme,
               HDC hdc,
               int iPart,
               int iState,
               LPRECT pRect,
               UINT uFlags)
{
    if (hTheme)
        DrawThemeEdge(hTheme, hdc, iPart, iState, pRect, EDGE_ETCHED, uFlags, pRect);
    else
        DrawEdge(hdc, pRect, EDGE_ETCHED, uFlags);
}

UINT
ClrBtn_CustomDraw(NMCUSTOMDRAW *pCD,
                  COLORREF Color)
{
    enum { MARGIN = 1, SEPLINE = 2 };
    HTHEME hTheme;
    HGDIOBJ hOrgPen, hOrgBrush;
    COLORREF clrOrgPen, clrOrgBrush, clrWidget;
    INT nPadW, nPadH, nHalf, nArrow;
    RECT r = pCD->rc;
    POINT pts[3];

    if (pCD->dwDrawStage == CDDS_PREPAINT)
        return CDRF_NOTIFYPOSTPAINT;
    if (pCD->dwDrawStage != CDDS_POSTPAINT)
        return CDRF_DODEFAULT;

    nPadW = GetSystemMetrics(SM_CXFOCUSBORDER) + MARGIN;
    nPadH = GetSystemMetrics(SM_CYFOCUSBORDER) + MARGIN;

    hTheme = GetWindowTheme(pCD->hdr.hwndFrom);
    if (hTheme)
    {
        MARGINS margins;
        GetThemeMargins(hTheme, pCD->hdc, BP_PUSHBUTTON, PBS_NORMAL, TMT_CONTENTMARGINS, NULL, &margins);
        r.left += margins.cxLeftWidth + nPadW;
        r.top += margins.cyTopHeight + nPadH;
        r.right -= margins.cxRightWidth + nPadW;
        r.bottom -= margins.cyBottomHeight + nPadH;
    }
    else
    {
        enum { FOCUSADJUST = 2 }; /* From comctl32 button.c */
        InflateRect(&r, -(nPadW + FOCUSADJUST), -(nPadH + FOCUSADJUST));
    }

    hOrgPen = SelectObject(pCD->hdc, GetStockObject(DC_PEN));
    hOrgBrush = SelectObject(pCD->hdc, GetStockObject(DC_BRUSH));
    clrWidget = GetSysColor(COLOR_BTNTEXT);
    clrOrgPen = SetDCPenColor(pCD->hdc, clrWidget);
    clrOrgBrush = SetDCBrushColor(pCD->hdc, Color);

    nHalf = (r.bottom - r.top) / 2;
    Rectangle(pCD->hdc, r.left, r.top, r.right - nHalf - SEPLINE * 3, r.bottom);

    SetDCBrushColor(pCD->hdc, clrWidget);
    nArrow = ((r.bottom - r.top) / 3) & ~1;
    pts[0].x = r.right - nHalf + nArrow / 2;
    pts[0].y = r.top + nHalf - nArrow / 3;
    pts[1].x = pts[0].x + nArrow;
    pts[1].y = pts[0].y;
    pts[2].x = pts[0].x + nArrow / 2;
    pts[2].y = pts[1].y + nArrow / 2;
    Polygon(pCD->hdc, pts, _countof(pts));

    r.left = r.right - nHalf - SEPLINE * 3 / 2;
    DrawEtchedLine(hTheme, pCD->hdc, BP_PUSHBUTTON, PBS_NORMAL, &r, BF_LEFT);

    SetDCBrushColor(pCD->hdc, clrOrgBrush);
    SetDCPenColor(pCD->hdc, clrOrgPen);
    SelectObject(pCD->hdc, hOrgBrush);
    SelectObject(pCD->hdc, hOrgPen);
    return CDRF_DODEFAULT;
}

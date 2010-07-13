/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            lib/cpl/desk/draw.c
 * PURPOSE:         Providing drawing functions
 *
 * PROGRAMMERS:     Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

/* #define NTOS_MODE_USER */
/* #define WIN32_NO_STATUS */
#include "desk.h"
#include "theme.h"
#include "draw.h"
/* #include <ndk/ntndk.h> */
/* #include <win32k/ntuser.h> */

/******************************************************************************/

static const signed char LTInnerNormal[] = {
    -1,        -1,                  -1,                 -1,
    -1,        COLOR_BTNHIGHLIGHT,  COLOR_BTNHIGHLIGHT, -1,
    -1,        COLOR_3DDKSHADOW,    COLOR_3DDKSHADOW,   -1,
    -1,        -1,                  -1,                 -1
};

static const signed char LTOuterNormal[] = {
    -1,                 COLOR_3DLIGHT,   COLOR_BTNSHADOW, -1,
    COLOR_BTNHIGHLIGHT, COLOR_3DLIGHT,   COLOR_BTNSHADOW, -1,
    COLOR_3DDKSHADOW,   COLOR_3DLIGHT,   COLOR_BTNSHADOW, -1,
    -1,                 COLOR_3DLIGHT,   COLOR_BTNSHADOW, -1
};

static const signed char RBInnerNormal[] = {
    -1,        -1,              -1,                 -1,
    -1,        COLOR_BTNSHADOW, COLOR_BTNSHADOW,    -1,
    -1,        COLOR_3DLIGHT,   COLOR_3DLIGHT,      -1,
    -1,        -1,              -1,                 -1
};

static const signed char RBOuterNormal[] = {
    -1,                 COLOR_3DDKSHADOW,  COLOR_BTNHIGHLIGHT, -1,
    COLOR_BTNSHADOW,    COLOR_3DDKSHADOW,  COLOR_BTNHIGHLIGHT, -1,
    COLOR_3DLIGHT,      COLOR_3DDKSHADOW,  COLOR_BTNHIGHLIGHT, -1,
    -1,                 COLOR_3DDKSHADOW,  COLOR_BTNHIGHLIGHT, -1
};

static BOOL
MyIntDrawRectEdge(HDC hdc, LPRECT rc, UINT uType, UINT uFlags, THEME *theme)
{
	signed char LTInnerI, LTOuterI;
	signed char RBInnerI, RBOuterI;
	HPEN LTInnerPen, LTOuterPen;
	HPEN RBInnerPen, RBOuterPen;
	RECT InnerRect = *rc;
	POINT SavePoint;
	HPEN SavePen;
	int LBpenplus = 0;
	int LTpenplus = 0;
	int RTpenplus = 0;
	int RBpenplus = 0;
	/* Init some vars */
	LTInnerPen = LTOuterPen = RBInnerPen = RBOuterPen = (HPEN)GetStockObject(NULL_PEN);
	SavePen = (HPEN)SelectObject(hdc, LTInnerPen);

	/* Determine the colors of the edges */
	LTInnerI = LTInnerNormal[uType & (BDR_INNER|BDR_OUTER)];
	LTOuterI = LTOuterNormal[uType & (BDR_INNER|BDR_OUTER)];
	RBInnerI = RBInnerNormal[uType & (BDR_INNER|BDR_OUTER)];
	RBOuterI = RBOuterNormal[uType & (BDR_INNER|BDR_OUTER)];

	if((uFlags & BF_BOTTOMLEFT) == BF_BOTTOMLEFT)
		LBpenplus = 1;
	if((uFlags & BF_TOPRIGHT) == BF_TOPRIGHT)
		RTpenplus = 1;
	if((uFlags & BF_BOTTOMRIGHT) == BF_BOTTOMRIGHT)
		RBpenplus = 1;
	if((uFlags & BF_TOPLEFT) == BF_TOPLEFT)
		LTpenplus = 1;

	if(LTInnerI != -1)
		LTInnerPen = GetStockObject(DC_PEN);
	if(LTOuterI != -1)
		LTOuterPen = GetStockObject(DC_PEN);
	if(RBInnerI != -1)
		RBInnerPen = GetStockObject(DC_PEN);
	if(RBOuterI != -1)
		RBOuterPen = GetStockObject(DC_PEN);
	{
		HBRUSH hbr;
		hbr = CreateSolidBrush(theme->crColor[COLOR_BTNFACE]);
		FillRect(hdc, &InnerRect, hbr);
		DeleteObject(hbr);
	}
	MoveToEx(hdc, 0, 0, &SavePoint);

	/* Draw the outer edge */
	SelectObject(hdc, LTOuterPen);
	SetDCPenColor(hdc, theme->crColor[LTOuterI]);
	if(uFlags & BF_TOP)
	{
		MoveToEx(hdc, InnerRect.left, InnerRect.top, NULL);
		LineTo(hdc, InnerRect.right, InnerRect.top);
	}
	if(uFlags & BF_LEFT)
	{
		MoveToEx(hdc, InnerRect.left, InnerRect.top, NULL);
		LineTo(hdc, InnerRect.left, InnerRect.bottom);
	}
	SelectObject(hdc, RBOuterPen);
	SetDCPenColor(hdc, theme->crColor[RBOuterI]);
	if(uFlags & BF_BOTTOM)
	{
		MoveToEx(hdc, InnerRect.left, InnerRect.bottom-1, NULL);
		LineTo(hdc, InnerRect.right, InnerRect.bottom-1);
	}
	if(uFlags & BF_RIGHT)
	{
		MoveToEx(hdc, InnerRect.right-1, InnerRect.top, NULL);
		LineTo(hdc, InnerRect.right-1, InnerRect.bottom);
	}

	/* Draw the inner edge */
	SelectObject(hdc, LTInnerPen);
	SetDCPenColor(hdc, theme->crColor[LTInnerI]);
	if(uFlags & BF_TOP)
	{
		MoveToEx(hdc, InnerRect.left+LTpenplus, InnerRect.top+1, NULL);
		LineTo(hdc, InnerRect.right-RTpenplus, InnerRect.top+1);
	}
	if(uFlags & BF_LEFT)
	{
		MoveToEx(hdc, InnerRect.left+1, InnerRect.top+LTpenplus, NULL);
		LineTo(hdc, InnerRect.left+1, InnerRect.bottom-LBpenplus);
	}
	SelectObject(hdc, RBInnerPen);
	SetDCPenColor(hdc, theme->crColor[RBInnerI]);
	if(uFlags & BF_BOTTOM)
	{
		MoveToEx(hdc, InnerRect.left+LBpenplus, InnerRect.bottom-2, NULL);
		LineTo(hdc, InnerRect.right-RBpenplus, InnerRect.bottom-2);
	}
	if(uFlags & BF_RIGHT)
	{
		MoveToEx(hdc, InnerRect.right-2, InnerRect.top+RTpenplus, NULL);
		LineTo(hdc, InnerRect.right-2, InnerRect.bottom-RBpenplus);
	}

	/* Cleanup */
	SelectObject(hdc, SavePen);
	MoveToEx(hdc, SavePoint.x, SavePoint.y, NULL);
	return TRUE;
}

static BOOL
MyDrawFrameButton(HDC hdc, LPRECT rc, UINT uState, THEME *theme)
{
	UINT edge;
	if(uState & (DFCS_PUSHED | DFCS_CHECKED | DFCS_FLAT))
		edge = EDGE_SUNKEN;
	else
		edge = EDGE_RAISED;
	return MyIntDrawRectEdge(hdc, rc, edge, (uState & DFCS_FLAT) | BF_RECT | BF_SOFT, theme);
}

static int
MyMakeSquareRect(LPRECT src, LPRECT dst)
{
	int Width  = src->right - src->left;
	int Height = src->bottom - src->top;
	int SmallDiam = Width > Height ? Height : Width;

	*dst = *src;

	/* Make it a square box */
	if (Width < Height)	  /* SmallDiam == Width */
	{
		dst->top += (Height-Width)/2;
		dst->bottom = dst->top + SmallDiam;
	}
	else if(Width > Height) /* SmallDiam == Height */
	{
		dst->left += (Width-Height)/2;
		dst->right = dst->left + SmallDiam;
	}

	return SmallDiam;
}

static BOOL
MyDrawFrameCaption(HDC dc, LPRECT r, UINT uFlags, THEME *theme)
{
	LOGFONT lf;
	HFONT hFont, hOldFont;
	COLORREF clrsave;
	RECT myr;
	INT bkmode;
	TCHAR Symbol;
	switch(uFlags & 0xff)
	{
	case DFCS_CAPTIONCLOSE:
		Symbol = 'r';
		break;
	case DFCS_CAPTIONHELP:
		Symbol = 's';
		break;
	case DFCS_CAPTIONMIN:
		Symbol = '0';
		break;
	case DFCS_CAPTIONMAX:
		Symbol = '1';
		break;
	case DFCS_CAPTIONRESTORE:
		Symbol = '2';
		break;
	}
	MyIntDrawRectEdge(dc, r, (uFlags & DFCS_PUSHED) ? EDGE_SUNKEN : EDGE_RAISED, BF_RECT | BF_MIDDLE | BF_SOFT, theme);
	ZeroMemory(&lf, sizeof(LOGFONT));
	MyMakeSquareRect(r, &myr);
	myr.left += 1;
	myr.top += 1;
	myr.right -= 1;
	myr.bottom -= 1;
	if(uFlags & DFCS_PUSHED)
	   OffsetRect(&myr,1,1);
	lf.lfHeight = myr.bottom - myr.top;
	lf.lfWidth = 0;
	lf.lfWeight = FW_NORMAL;
	lf.lfCharSet = DEFAULT_CHARSET;
	lstrcpy(lf.lfFaceName, TEXT("Marlett"));
	hFont = CreateFontIndirect(&lf);
	/* save font and text color */
	hOldFont = SelectObject(dc, hFont);
	clrsave = GetTextColor(dc);
	bkmode = GetBkMode(dc);
	/* set color and drawing mode */
	SetBkMode(dc, TRANSPARENT);
	if(uFlags & DFCS_INACTIVE)
	{
		/* draw shadow */
		SetTextColor(dc, theme->crColor[COLOR_BTNHIGHLIGHT]);
		TextOut(dc, myr.left + 1, myr.top + 1, &Symbol, 1);
	}
	SetTextColor(dc, theme->crColor[(uFlags & DFCS_INACTIVE) ? COLOR_BTNSHADOW : COLOR_BTNTEXT]);
	/* draw selected symbol */
	TextOut(dc, myr.left, myr.top, &Symbol, 1);
	/* restore previous settings */
	SetTextColor(dc, clrsave);
	SelectObject(dc, hOldFont);
	SetBkMode(dc, bkmode);
	DeleteObject(hFont);
	return TRUE;
}

/******************************************************************************/

static BOOL
MyDrawFrameScroll(HDC dc, LPRECT r, UINT uFlags, THEME *theme)
{
	LOGFONT lf;
	HFONT hFont, hOldFont;
	COLORREF clrsave;
	RECT myr;
	INT bkmode;
	TCHAR Symbol;
	switch(uFlags & 0xff)
	{
	case DFCS_SCROLLCOMBOBOX:
	case DFCS_SCROLLDOWN:
		Symbol = '6';
		break;

	case DFCS_SCROLLUP:
		Symbol = '5';
		break;

	case DFCS_SCROLLLEFT:
		Symbol = '3';
		break;

	case DFCS_SCROLLRIGHT:
		Symbol = '4';
		break;
	}
	MyIntDrawRectEdge(dc, r, (uFlags & DFCS_PUSHED) ? EDGE_SUNKEN : EDGE_RAISED, (uFlags&DFCS_FLAT) | BF_MIDDLE | BF_RECT, theme);
	ZeroMemory(&lf, sizeof(LOGFONT));
	MyMakeSquareRect(r, &myr);
	myr.left += 1;
	myr.top += 1;
	myr.right -= 1;
	myr.bottom -= 1;
	if(uFlags & DFCS_PUSHED)
	   OffsetRect(&myr,1,1);
	lf.lfHeight = myr.bottom - myr.top;
	lf.lfWidth = 0;
	lf.lfWeight = FW_NORMAL;
	lf.lfCharSet = DEFAULT_CHARSET;
	lstrcpy(lf.lfFaceName, TEXT("Marlett"));
	hFont = CreateFontIndirect(&lf);
	/* save font and text color */
	hOldFont = SelectObject(dc, hFont);
	clrsave = GetTextColor(dc);
	bkmode = GetBkMode(dc);
	/* set color and drawing mode */
	SetBkMode(dc, TRANSPARENT);
	if(uFlags & DFCS_INACTIVE)
	{
		/* draw shadow */
		SetTextColor(dc, theme->crColor[COLOR_BTNHIGHLIGHT]);
		TextOut(dc, myr.left + 1, myr.top + 1, &Symbol, 1);
	}
	SetTextColor(dc, theme->crColor[(uFlags & DFCS_INACTIVE) ? COLOR_BTNSHADOW : COLOR_BTNTEXT]);
	/* draw selected symbol */
	TextOut(dc, myr.left, myr.top, &Symbol, 1);
	/* restore previous settings */
	SetTextColor(dc, clrsave);
	SelectObject(dc, hOldFont);
	SetBkMode(dc, bkmode);
	DeleteObject(hFont);
	return TRUE;
}

BOOL
MyDrawFrameControl(HDC hDC, LPRECT rc, UINT uType, UINT uState, THEME *theme)
{
	switch(uType)
	{
	case DFC_BUTTON:
		return MyDrawFrameButton(hDC, rc, uState, theme);
	case DFC_CAPTION:
		return MyDrawFrameCaption(hDC, rc, uState, theme);
	case DFC_SCROLL:
		return MyDrawFrameScroll(hDC, rc, uState, theme);
	}
	return FALSE;
}

BOOL
MyDrawEdge(HDC hDC, LPRECT rc, UINT edge, UINT flags, THEME *theme)
{
	return MyIntDrawRectEdge(hDC, rc, edge, flags, theme);
}

VOID
MyDrawCaptionButtons(HDC hdc, LPRECT lpRect, BOOL bMinMax, int x, THEME *theme)
{
	RECT rc3;
	RECT rc4;
	RECT rc5;

	rc3.left = lpRect->right - 2 - x;
	rc3.top = lpRect->top + 2;
	rc3.right = lpRect->right - 2;
	rc3.bottom = lpRect->bottom - 2;

	MyDrawFrameControl(hdc, &rc3, DFC_CAPTION, DFCS_CAPTIONCLOSE, theme);

	if (bMinMax)
	{
		rc4.left = rc3.left - x - 2;
		rc4.top = rc3.top;
		rc4.right = rc3.right - x - 2;
		rc4.bottom = rc3.bottom;

		MyDrawFrameControl(hdc, &rc4, DFC_CAPTION, DFCS_CAPTIONMAX, theme);

		rc5.left = rc4.left - x;
		rc5.top = rc4.top;
		rc5.right = rc4.right - x;
		rc5.bottom = rc4.bottom;

		MyDrawFrameControl(hdc, &rc5, DFC_CAPTION, DFCS_CAPTIONMIN, theme);
	}
}

VOID
MyDrawScrollbar(HDC hdc, LPRECT rc, HBRUSH hbrScrollbar, THEME *theme)
{
	RECT rcTop;
	RECT rcBottom;
	RECT rcMiddle;
	int width;

	width = rc->right - rc->left;

	rcTop.left = rc->left;
	rcTop.right = rc->right;
	rcTop.top = rc->top;
	rcTop.bottom = rc->top + width;

	rcMiddle.left = rc->left;
	rcMiddle.right = rc->right;
	rcMiddle.top = rc->top + width;
	rcMiddle.bottom = rc->bottom - width;

	rcBottom.left = rc->left;
	rcBottom.right = rc->right;
	rcBottom.top = rc->bottom - width;
	rcBottom.bottom = rc->bottom;

	MyDrawFrameControl(hdc, &rcTop, DFC_SCROLL, DFCS_SCROLLUP, theme);
	MyDrawFrameControl(hdc, &rcBottom, DFC_SCROLL, DFCS_SCROLLDOWN, theme);

	FillRect(hdc, &rcMiddle, hbrScrollbar);
}

/******************************************************************************/

BOOL
MyDrawCaptionTemp(HWND hwnd, HDC hdc, const RECT *rect, HFONT hFont, HICON hIcon, LPCWSTR str, UINT uFlags, THEME *theme)
{
	/* FIXME */
	return DrawCaptionTemp(hwnd, hdc, rect, hFont, hIcon, str, uFlags);
}

/******************************************************************************/

DWORD
MyDrawMenuBarTemp(HWND Wnd, HDC DC, LPRECT Rect, HMENU Menu, HFONT Font, THEME *theme)
{
	/* FIXME */
	return DrawMenuBarTemp(Wnd, DC, Rect, Menu, Font);
}

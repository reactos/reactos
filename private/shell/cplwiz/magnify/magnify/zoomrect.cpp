// ZoomRect.cpp : implementation file
//

#include "stdafx.h"
#include "magnify.h"
#include "ZoomRect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CZoomRect

CZoomRect::CZoomRect()
{
	LPCTSTR lpszClass = AfxRegisterWndClass(CS_SAVEBITS);
	CreateEx(WS_EX_TOPMOST, lpszClass, __TEXT("ZoomRect"), WS_POPUP /* | WS_VISIBLE*/, 0, 0, 10, 10, NULL, NULL);
	m_nBorderWidth = 5;
}

CZoomRect::~CZoomRect()
{
}


BEGIN_MESSAGE_MAP(CZoomRect, CWnd)
	//{{AFX_MSG_MAP(CZoomRect)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CZoomRect message handlers

void CZoomRect::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	CRect rcClient;
	GetClientRect(&rcClient);
	CBrush brush(RGB(255, 255, 128));
	dc.FillRect(&rcClient, &brush);
}

HRGN CreateRectRgn(LPCRECT lpRect)
{
	return CreateRectRgn(lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
}

void CZoomRect::SetLocation(int nLeft, int nTop, int nRight, int nBottom)
{
	CRect rcInner(nLeft, nTop, nRight, nBottom);
	CRect rcOutter;
	rcOutter = rcInner;
	rcOutter.InflateRect(m_nBorderWidth, m_nBorderWidth);
	SetWindowPos(NULL, rcOutter.left, rcOutter.top, rcOutter.Width(), rcOutter.Height(), SWP_NOZORDER);

	ScreenToClient(&rcOutter);
	ScreenToClient(&rcInner);

	if(rcOutter != m_rcOutter)
	{
		m_rcOutter = rcOutter;
		HRGN hrgn1 = CreateRectRgn(&rcOutter);
		HRGN hrgn2 = CreateRectRgn(&rcInner);
		CombineRgn(hrgn1, hrgn1, hrgn2, RGN_XOR);
		DeleteObject(hrgn2);
		SetWindowRgn(hrgn1, TRUE);
	}

	UpdateWindow();
}

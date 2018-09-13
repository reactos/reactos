// srvritem.cpp : implementation of the CWordPadSrvrItem class
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"
#include "wordpad.h"
#include "wordpdoc.h"
#include "wordpvw.h"
#include "srvritem.h"
#include <limits.h>

IMPLEMENT_DYNAMIC(CEmbeddedItem, COleServerItem)

extern CLIPFORMAT cfRTF;

CEmbeddedItem::CEmbeddedItem(CWordPadDoc* pContainerDoc, int nBeg, int nEnd)
	: COleServerItem(pContainerDoc, TRUE)
{
	ASSERT(pContainerDoc != NULL);
	ASSERT_VALID(pContainerDoc);
	m_nBeg = nBeg;
	m_nEnd = nEnd;
}

CWordPadView* CEmbeddedItem::GetView() const
{
	CDocument* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	POSITION pos = pDoc->GetFirstViewPosition();
	if (pos == NULL)
		return NULL;

	CWordPadView* pView = (CWordPadView*)pDoc->GetNextView(pos);
	ASSERT_VALID(pView);
	ASSERT(pView->IsKindOf(RUNTIME_CLASS(CWordPadView)));
	return pView;
}

void CEmbeddedItem::Serialize(CArchive& ar)
{
	if (m_lpRichDataObj != NULL)
	{
		ASSERT(ar.IsStoring());
		FORMATETC etc = {NULL, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
		etc.cfFormat = (CLIPFORMAT)cfRTF;
		STGMEDIUM stg;
		if (SUCCEEDED(m_lpRichDataObj->GetData(&etc, &stg)))
		{
			LPBYTE p = (LPBYTE)GlobalLock(stg.hGlobal);
			if (p != NULL)
			{
				ar.Write(p, GlobalSize(stg.hGlobal));
				GlobalUnlock(stg.hGlobal);
			}
			ASSERT(stg.tymed == TYMED_HGLOBAL);
			ReleaseStgMedium(&stg);
		}
	}
	else
		GetDocument()->Serialize(ar);
}

BOOL CEmbeddedItem::OnGetExtent(DVASPECT dwDrawAspect, CSize& rSize)
{
	if (dwDrawAspect != DVASPECT_CONTENT)
		return COleServerItem::OnGetExtent(dwDrawAspect, rSize);

	CClientDC dc(NULL);
	return OnDrawEx(&dc, rSize, FALSE);
}

BOOL CEmbeddedItem::OnDraw(CDC* pDC, CSize& rSize)
{
	return OnDrawEx(pDC, rSize, TRUE);
}

BOOL CEmbeddedItem::OnDrawEx(CDC* pDC, CSize& rSize, BOOL bOutput)
{
	CDisplayIC dc;
	CWordPadView* pView = GetView();
	if (pView == NULL)
		return FALSE;
	ASSERT_VALID(pView);

	int nWrap = pView->m_nWordWrap;

	CRect rect;//rect in twips
	rect.left = rect.top = 0;
	rect.bottom = 32767; // bottomless

	rect.right = 32767;
	if (nWrap == 0) // no word wrap
		rect.right = 32767;
	else if (nWrap == 1) // wrap to window
	{
		CRect rectClient;
		pView->GetClientRect(&rectClient);
		rect.right = rectClient.right - HORZ_TEXTOFFSET;
		rect.right = MulDiv(rect.right, 1440, dc.GetDeviceCaps(LOGPIXELSX));
	}
	else if (nWrap == 2) // wrap to ruler
		rect.right = pView->GetPrintWidth();
                 
	// first just determine the correct extents of the text
	pDC->SetBkMode(TRANSPARENT);
	
	if (pView->PrintInsideRect(pDC, rect, m_nBeg, m_nEnd, FALSE) == 0)
	{
		// default to 12pts high and 4" wide if no text
		rect.bottom = rect.top+12*20+1; // 12 pts high
		rect.right = rect.left+ 4*1440;
	}
	rect.bottom+=3*(1440/dc.GetDeviceCaps(LOGPIXELSX)); // three pixels

	// then, really output the text
	CRect rectOut = rect; // don't pass rect because it will get clobbered
	if (bOutput)
		pView->PrintInsideRect(pDC, rectOut, m_nBeg, m_nEnd, TRUE);
	ASSERT(rectOut.right == rect.right);

	// adjust for border (rect.left is already adjusted)
	if (pView->GetStyle() & WS_HSCROLL)
		++rect.bottom;  // account for border on scroll bar!

	// return HIMETRIC size
	rSize = rect.Size();
	rSize.cx = MulDiv(rSize.cx, 2540, 1440); // convert twips to HIMETRIC
	rSize.cy = MulDiv(rSize.cy, 2540, 1440); // convert twips to HIMETRIC
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

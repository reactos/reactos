//*******************************************************************************
// COPYRIGHT NOTES
// ---------------
// You may use this source code, compile or redistribute it as part of your application 
// for free. You cannot redistribute it as a part of a software development 
// library without the agreement of the author. If the sources are 
// distributed along with the application, you should leave the original 
// copyright notes in the source code without any changes.
// This code can be used WITHOUT ANY WARRANTIES at your own risk.
// 
// For the latest updates to this code, check this site:
// http://www.masmex.com 
// after Sept 2000
// 
// Copyright(C) 2000 Philip Oldaker <email: philip@masmex.com>
//*******************************************************************************

#include "stdafx.h"
#include "UIDragImage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDragDropImage

IMPLEMENT_DYNCREATE(CDragDropImage, CCmdTarget)

CDragDropImage::CDragDropImage()
{
}

CDragDropImage::CDragDropImage(int nSelected,int nType) : 
	m_nSelected(nSelected),m_nType(nType)
{
}

CDragDropImage::~CDragDropImage()
{
	while (!m_itemList.IsEmpty())
	{
		CDragDropItem *pItem = m_itemList.RemoveHead();
		delete pItem;
	}
}

void CDragDropImage::AddItem(LPCRECT prcItem,LPCRECT prcIcon)
{
	CDragDropItem *pItem = new CDragDropItem(prcItem,prcIcon);
	m_itemList.AddHead(pItem);
}


BEGIN_MESSAGE_MAP(CDragDropImage, CCmdTarget)
    //{{AFX_MSG_MAP(CDragDropImage)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDragDropImage message handlers

void CDragDropImage::DrawDragImage(CDC *pDC, POINT point, POINT ActionPoint)
{
	point.x -= ActionPoint.x;
	point.y -= ActionPoint.y;
	for(POSITION pos=m_itemList.GetTailPosition();pos != NULL;)
	{
		CDragDropItem *pItem = m_itemList.GetPrev(pos);
		DrawItemImage(pDC,point,pItem->m_rcItem,pItem->m_rcIcon);		
	}
}

// rcItem contains the bounding text
// of an item in thge list control
void CDragDropImage::DrawItemImage(CDC *pDC,POINT point,const CRect &rcItem,const CRect &rcIcon)
{
// Make it the inverse of the screen color.
    int nOldMode = pDC->SetROP2 (R2_NOT); 
// just the outline
    CBrush* pOldBrush = (CBrush*) pDC->SelectStockObject (NULL_BRUSH);
// draw it icon size
	pDC->Rectangle (point.x+rcIcon.left, point.y+rcIcon.top, point.x+rcIcon.left + rcIcon.Width(),point.y+rcIcon.top + rcIcon.Height());
	pDC->SelectObject (pOldBrush);
// the outline of the text item
	CPen pen(PS_DASH,1,GetSysColor(COLOR_BTNTEXT));
	CPen *pOldPen = pDC->SelectObject(&pen);
// only in report or list mode
	if (m_nType != LVS_ICON && m_nType != LVS_SMALLICON)
	{
		// draw the line halfway down the icon rectangle
		pDC->MoveTo(point.x+rcIcon.left+rcIcon.Width(), point.y+rcIcon.top+(rcIcon.Height()/2));
		pDC->LineTo(point.x+rcIcon.left+rcItem.Width(), point.y+rcIcon.top+(rcIcon.Height()/2));
	}
    pDC->SelectObject (pOldPen);
    pDC->SetROP2 (nOldMode);
}


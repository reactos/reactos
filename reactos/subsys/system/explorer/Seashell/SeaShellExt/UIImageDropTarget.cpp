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

// IM_OleImageDropTarget.cpp : implementation file
//

#include "stdafx.h"
#include "UIDragImage.h"
#include "UIImageDropTarget.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUI_ImageDropTarget

CUI_ImageDropTarget::CUI_ImageDropTarget() : m_pImage(NULL),m_pWnd(NULL)
{	
	m_nCFFormat = ::RegisterClipboardFormat(_T("APP_CF_DD_LISTCTRL_IMAGE"));
	m_nImage = -1;
}

CUI_ImageDropTarget::~CUI_ImageDropTarget()
{
}


/////////////////////////////////////////////////
//
// For OLE drag and drop 
//
/////////////////////////////////////////////////
void CUI_ImageDropTarget::OnDragLeave( CWnd* pWnd)
{
    if (m_pImage == NULL) 
		return;

	// Erase the old drag image and delete the temporary widget.
	CClientDC dc(GetImageWin(pWnd));
    m_pImage->DrawDragImage (&dc,m_ptOldImage,m_ptItem);
    delete m_pImage;
    m_pImage = NULL;

	COleDropTarget::OnDragLeave(pWnd);
}

DROPEFFECT CUI_ImageDropTarget::OnDragEnter( CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point )
{
	DROPEFFECT dwEffect = COleDropTarget::OnDragEnter(pWnd,pDataObject,dwKeyState,point);
    HGLOBAL hData = pDataObject->GetGlobalData(GetClipboardFormat());
	if (hData == NULL)
		return dwEffect;
    //
    // Create a temporary image for drag imaging.
    //
    int *pData = (int*)::GlobalLock(hData);
    int nType = *pData++;
	int nSelected = *pData++;
    DD_ImageData *pImageData = (DD_ImageData*)pData;
	CPoint pt = pImageData->m_ptDrag;
	int i;
    switch (nType) 
	{
    case LVS_ICON: 
	case LVS_SMALLICON:
	case LVS_LIST:
	case LVS_REPORT:
        m_pImage = new CDragDropImage(nSelected,nType);
		for(i=0;i < nSelected;i++)
		{
			m_pImage->AddItem(&pImageData->m_rcItem,&pImageData->m_rcIcon);
			pImageData++;
		}
        break;
    default:
		ASSERT(0);
		// Just in case
	    ::GlobalUnlock(hData);
        return DROPEFFECT_NONE;
    }
	// total item
    CRect rcItem;
	pData = (int*)pImageData;
	rcItem.left = *pData++;
	rcItem.right = *pData++;
	rcItem.top = *pData++;
	rcItem.bottom = *pData;
    ::GlobalUnlock(hData);

    //
    // Begin dragging.
    //
	CClientDC dc(GetImageWin(pWnd));
	pWnd->ClientToScreen(&point);
	m_pWnd->ScreenToClient(&point);
    dc.DPtoLP(&point);
	m_ptItem = pt;

    CPoint ptDrag (point.x, point.y);
    m_ptOldImage.x = m_ptOldImage.y = -32000;
    m_ptPrevPos = point;

	return dwEffect;
}

DROPEFFECT CUI_ImageDropTarget::OnDragOver( CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point )
{
	DROPEFFECT dwEffect = COleDropTarget::OnDragOver(pWnd,pDataObject,dwKeyState,point);
	if (!pDataObject->IsDataAvailable(GetClipboardFormat()))
		return dwEffect;
    // Erase the old drag image and draw a new one if the cursor has moved.
	CClientDC dc(GetImageWin(pWnd));
	pWnd->ClientToScreen(&point);
	m_pWnd->ScreenToClient(&point);
    dc.DPtoLP(&point);
    if (m_pImage && point != m_ptPrevPos) 
	{
		// erase the old image
        m_pImage->DrawDragImage(&dc,m_ptOldImage,m_ptItem);
//        CPoint ptDrag (point.x - m_sizeDelta.cx, point.y - m_sizeDelta.cy);
        CPoint ptDrag (point.x, point.y);
		// draw the new image
        m_pImage->DrawDragImage (&dc, ptDrag, m_ptItem);
		// save coords
        m_ptOldImage = ptDrag;
        m_ptPrevPos = point;
    }
	return dwEffect;
}

BOOL CUI_ImageDropTarget::OnDrop( CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point )
{
	BOOL bRet = COleDropTarget::OnDrop(pWnd,pDataObject,dropEffect,point);
	if (!pDataObject->IsDataAvailable(GetClipboardFormat()))
		return bRet;
    //
    // Erase the old drag image and delete the temporary widget.
    //
    CClientDC dc(GetImageWin(pWnd));
    if (m_pImage != NULL) 
	{
        m_pImage->DrawDragImage (&dc,m_ptOldImage,m_ptItem);
        delete m_pImage;
        m_pImage = NULL;
    }
	return bRet;
}

void CUI_ImageDropTarget::EraseOldImage()
{
	if (m_pImage == NULL)
		return;
	CClientDC dc(GetImageWin(NULL));
    m_pImage->DrawDragImage(&dc,m_ptOldImage,m_ptItem);
    m_ptOldImage.x = m_ptOldImage.y = -32000;
}

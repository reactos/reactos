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
#include "UICtrl.h"
#include "UIDragImage.h"
#include "UIDropTarget.h"
#include "UIDragDropCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CDD_OleDropTargetInfo,CObject)

void CListDropTarget::OnDragLeave(CWnd* pWnd)
{
	CUI_ImageDropTarget::OnDragLeave(pWnd);
	CDD_OleDropTargetInfo Info(pWnd->GetSafeHwnd());
	if (!pWnd->SendMessage(WM_APP_OLE_DD_LEAVE,(WPARAM)&Info))
		pWnd->GetParent()->SendMessage(WM_APP_OLE_DD_LEAVE,(WPARAM)&Info);
}

DROPEFFECT CListDropTarget::OnDragEnter( CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point )
{
	// Should be starting in a CUIODListCtrl window
	CUI_ImageDropTarget::OnDragEnter(pWnd,pDataObject,dwKeyState,point);
	CDD_OleDropTargetInfo Info(pWnd->GetSafeHwnd(),point,pDataObject);
	Info.SetKeyboardState(dwKeyState);
	if (!pWnd->SendMessage(WM_APP_OLE_DD_ENTER,(WPARAM)&Info))
		pWnd->GetParent()->SendMessage(WM_APP_OLE_DD_ENTER,(WPARAM)&Info);
	return Info.GetDropEffect();
}

DROPEFFECT CListDropTarget::OnDragOver( CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point )
{
	// check if any image to draw
	if (GetDropImage() != NULL)
		CUI_ImageDropTarget::OnDragOver(pWnd,pDataObject,dwKeyState,point);
	m_dwKeyboardState = dwKeyState;
	CDD_OleDropTargetInfo Info(pWnd->GetSafeHwnd(),point,pDataObject);
	Info.SetKeyboardState(dwKeyState);
	// this will fill in info with item if mouse over one
	CUIODListCtrl *pList = static_cast<CUIODListCtrl*>(pWnd);
	return pList ? pList->SelectCurrentTarget(&Info) : DROPEFFECT_NONE;
}

BOOL CListDropTarget::OnDrop( CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point )
{
	CUI_ImageDropTarget::OnDrop(pWnd,pDataObject,dropEffect,point);

	TRACE0("CListDropTraget - OnDrop\n");
	// should be list control
	CUIODListCtrl *pList = static_cast<CUIODListCtrl*>(pWnd);
	if (pList != NULL)	
	{
		UINT flags;
		int iItem = pList->HitTest(point, &flags);
		CDD_OleDropTargetInfo Info(pWnd->GetSafeHwnd(),point,pDataObject);
		Info.SetDropEffect(dropEffect);
		Info.SetItem(iItem);
		Info.SetKeyboardState(m_dwKeyboardState);
		// alert control first otherwise parent
		if (!pWnd->SendMessage(WM_APP_OLE_DD_DROP,(WPARAM)&Info))
			return pWnd->GetParent()->SendMessage(WM_APP_OLE_DD_DROP,(WPARAM)&Info);
	}
	return CUI_ImageDropTarget::OnDrop(pWnd,pDataObject,dropEffect,point);
}




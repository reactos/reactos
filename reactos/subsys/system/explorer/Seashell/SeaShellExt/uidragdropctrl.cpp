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
#include "UIDragDropCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDragDropCtrl

CDragDropCtrl::CDragDropCtrl()
{
	m_dwKeyboardState = 0;
}

CDragDropCtrl::~CDragDropCtrl()
{
}

void CDragDropCtrl::OnDragLeave(CWnd* pWnd)
{
	CDD_OleDropTargetInfo Info(pWnd->GetSafeHwnd());
	// was message handled?
	if (pWnd->SendMessage(WM_APP_OLE_DD_LEAVE,(WPARAM)&Info) == FALSE)
		pWnd->GetParent()->SendMessage(WM_APP_OLE_DD_LEAVE,(WPARAM)&Info);
	COleDropTarget::OnDragLeave(pWnd);
}

DROPEFFECT CDragDropCtrl::OnDragEnter( CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point )
{
	CDD_OleDropTargetInfo Info(pWnd->GetSafeHwnd(),point,pDataObject);
	m_dwEnterKeyboardState = dwKeyState;
	Info.SetKeyboardState(dwKeyState);
	// was message handled?
	BOOL bRet = pWnd->SendMessage(WM_APP_OLE_DD_ENTER,(WPARAM)&Info);
	if (bRet == FALSE)
		bRet = pWnd->GetParent()->SendMessage(WM_APP_OLE_DD_ENTER,(WPARAM)&Info);
	return Info.GetDropEffect();
}

DROPEFFECT CDragDropCtrl::OnDragOver( CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point )
{
	CDD_OleDropTargetInfo Info(pWnd->GetSafeHwnd(),point,pDataObject);
	Info.SetKeyboardState(dwKeyState);
	// was message handled?
	m_dwKeyboardState = dwKeyState;
	BOOL bRet = pWnd->SendMessage(WM_APP_OLE_DD_OVER,(WPARAM)&Info);
	if (bRet == FALSE)
		bRet = pWnd->GetParent()->SendMessage(WM_APP_OLE_DD_OVER,(WPARAM)&Info);
	if (bRet == FALSE)
		return DROPEFFECT_NONE;
	return Info.GetDropEffect();
}

BOOL CDragDropCtrl::OnDrop( CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point )
{
	CDD_OleDropTargetInfo Info(pWnd->GetSafeHwnd(),point,pDataObject);
	Info.SetDropEffect(dropEffect);
	if (m_dwEnterKeyboardState & MK_RBUTTON)
		m_dwKeyboardState |= MK_RBUTTON;
	if (m_dwEnterKeyboardState & MK_LBUTTON)
		m_dwKeyboardState |= MK_LBUTTON;
	Info.SetKeyboardState(m_dwKeyboardState);
	BOOL bRet = pWnd->SendMessage(WM_APP_OLE_DD_DROP,(WPARAM)&Info);
	// try parent if no reply
	if (bRet == FALSE) 
		bRet = pWnd->GetParent()->SendMessage(WM_APP_OLE_DD_DROP,(WPARAM)&Info);
	return bRet;
}


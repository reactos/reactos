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

//////////////////////////////////////////////////////////////////////////////////////--*/
// UIDragDropTree.cpp : implementation file
//

#include "stdafx.h"
#include "UICtrl.h"
#include "UIDragDropTree.h"
#include "cbformats.h"
#include "UIRes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CUIDragDropTree,CTreeCtrl)
/////////////////////////////////////////////////
//
// For OLE drag and drop 
//
/////////////////////////////////////////////////
void CUI_TreeDropTarget::OnDragLeave(CWnd* pWnd)
{
	CUIDragDropTree *pTree = static_cast<CUIDragDropTree*>(pWnd);
	if (pTree == NULL)
		return;
	pTree->KillDragTimer();
	pTree->SelectDropTarget(NULL);
	CDD_OleDropTargetInfo Info(pWnd->GetSafeHwnd());
	Info.SetItem(pTree->GetDropHilightItem());
	BOOL bRet = pTree->SendMessage(WM_APP_OLE_DD_LEAVE,(WPARAM)&Info);
	if (bRet == FALSE)
		bRet = pTree->GetParent()->SendMessage(WM_APP_OLE_DD_LEAVE,(WPARAM)&Info);	
}

DROPEFFECT CUI_TreeDropTarget::OnDragEnter( CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point )
{
//	ASSERT_KINDOF(CUIDragDropTree,pWnd);
	CUIDragDropTree *pTree = static_cast<CUIDragDropTree*>(pWnd);
	if (pTree == NULL)
		return DROPEFFECT_NONE;
	((CUIDragDropTree*)pWnd)->SetDragTimer();
	CDD_OleDropTargetInfo Info(pWnd->GetSafeHwnd(),point,pDataObject);
	m_dwEnterKeyboardState = dwKeyState;
	Info.SetKeyboardState(dwKeyState);
	Info.SetItem(pTree->GetDropHilightItem());
	BOOL bRet = pTree->SendMessage(WM_APP_OLE_DD_ENTER,(WPARAM)&Info);
	if (bRet == FALSE)
		bRet = pTree->GetParent()->SendMessage(WM_APP_OLE_DD_ENTER,(WPARAM)&Info);
	return Info.GetDropEffect();
}

DROPEFFECT CUI_TreeDropTarget::OnDragOver( CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point )
{
	// disallow if the control key pressed
	EraseOldImage();
	CUIDragDropTree *pTree = static_cast<CUIDragDropTree*>(pWnd);
	if (pTree == NULL)
		return DROPEFFECT_NONE;
//	ASSERT_KINDOF(CUIDragDropTree,pWnd);
	CDD_OleDropTargetInfo Info(pWnd->GetSafeHwnd(),point,pDataObject);
	m_dwKeyboardState = dwKeyState;
	Info.SetKeyboardState(dwKeyState);
	// WM_APP_OLE_DD_OVER message sent in SelectCurrentTarget
	DROPEFFECT dropEffect = pTree->SelectCurrentTarget(&Info);
	CUI_ImageDropTarget::OnDragOver(pWnd,pDataObject,dwKeyState,point);
	return dropEffect;
}

BOOL CUI_TreeDropTarget::OnDrop( CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point )
{
	CUIDragDropTree *pTree = static_cast<CUIDragDropTree*>(pWnd);
	if (pTree == NULL)
		return FALSE;
//	ASSERT_KINDOF(CUIDragDropTree,pWnd);
	pTree->KillDragTimer();
	HTREEITEM hItem = pTree->GetDropHilightItem();
	BOOL bRet=FALSE;
	if (hItem) 
	{
		CDD_OleDropTargetInfo Info(pWnd->GetSafeHwnd(),point,pDataObject);
		Info.SetDropEffect(dropEffect);
		Info.SetItem(hItem);
        if (m_dwEnterKeyboardState & MK_RBUTTON)
			m_dwKeyboardState |= MK_RBUTTON;
        if (m_dwEnterKeyboardState & MK_LBUTTON)
			m_dwKeyboardState |= MK_LBUTTON;
		Info.SetKeyboardState(m_dwKeyboardState);
		bRet = pTree->SendMessage(WM_APP_OLE_DD_DROP,(WPARAM)&Info);
		if (bRet == FALSE)
			bRet = pTree->GetParent()->SendMessage(WM_APP_OLE_DD_DROP,(WPARAM)&Info);
	}
	pTree->SelectDropTarget(NULL);
	return bRet;
}

// During slow scroll, process every third message.
#define SLOWSCROLL_FREQUENCY 3
/////////////////////////////////////////////////////////////////////////////
// CUIDragDropTree

CUIDragDropTree::CUIDragDropTree(bool bDragDrop): m_nUpperYCoor(0), 
								m_nLowerYCoor(0), 
								m_nSlowScrollTimeout(0),
								m_nTimerID(0),
								m_bDragging(FALSE),
								m_pImageList(NULL),
								m_hitemDrag(NULL),
								m_hitemDrop(NULL),
								m_bDragDrop(bDragDrop),
								m_bDropFiles(true)

{
	m_nScrollBarSize = GetSystemMetrics( SM_CYHSCROLL );
	m_CopyMode = eDDCancel;
}

CUIDragDropTree::~CUIDragDropTree()
{
}

void CUIDragDropTree::OnDropFile(HTREEITEM hItem,LPCTSTR pszFile,UINT nFlags)
{
}

BEGIN_MESSAGE_MAP(CUIDragDropTree, CTreeCtrl)
	//{{AFX_MSG_MAP(CUIDragDropTree)
	ON_NOTIFY_REFLECT_EX(TVN_BEGINDRAG, OnBegindrag)
	ON_NOTIFY_REFLECT_EX(TVN_BEGINRDRAG, OnBeginRDrag)
	ON_WM_TIMER()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_DESTROY()
	ON_WM_CREATE()
	ON_WM_DROPFILES()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUIDragDropTree message handlers
void CUIDragDropTree::OnDropFiles(HDROP hDropInfo)
{
	UINT wNumFilesDropped = DragQueryFile(hDropInfo, 0XFFFFFFFF, NULL, 0);
	TCHAR szFile[MAX_PATH];
	UINT nLen;
	UINT nFlags;
	POINT pt;
	::DragQueryPoint(hDropInfo,&pt);
	HTREEITEM hitemDrag = HitTest(CPoint(pt), &nFlags);
	for(UINT i=0; i < wNumFilesDropped;i++)
	{
		nLen = DragQueryFile(hDropInfo,i,szFile,sizeof(szFile)/sizeof(TCHAR));
		if (nLen)
			OnDropFile(hitemDrag,szFile,nFlags);
	}
}

BOOL CUIDragDropTree::OnBeginRDrag(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	return StartDragDrop(pNMTreeView);
}

BOOL CUIDragDropTree::OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	return StartDragDrop(pNMTreeView);
}

BOOL CUIDragDropTree::StartDragDrop(NM_TREEVIEW* pNMTreeView)
{
	if (GetDragDrop() == false || pNMTreeView->itemNew.hItem == GetRootItem())
		return FALSE;
	// See if COM drag drop is implemented in this window or parent window
	DWORD dwDragEffect = SendMessage(WM_APP_OLE_DD_DODRAGDROP,(WPARAM)pNMTreeView,(LPARAM)&m_OleDataSource);
	if (dwDragEffect == 0)
		dwDragEffect = GetParent()->SendMessage(WM_APP_OLE_DD_DODRAGDROP,(WPARAM)pNMTreeView,(LPARAM)&m_OleDataSource);
	if (dwDragEffect)
	{
		CRect rcDrag; 
		m_hitemDrag = pNMTreeView->itemNew.hItem;
		// Define starting rect
		GetItemRect(pNMTreeView->itemNew.hItem,rcDrag,TRUE);
		// Start the DragDrop
		DWORD dwEffect = m_OleDataSource.DoDragDrop(dwDragEffect,rcDrag);
		// Clear the cache
		m_OleDataSource.Empty();        
	}
	else // otherwise start local drag drop
	{
		CPoint		ptAction;
		UINT		nFlags;

		GetCursorPos(&ptAction);
		ScreenToClient(&ptAction);
		
		m_bDragging = TRUE;
		m_hitemDrag = HitTest(pNMTreeView->ptDrag, &nFlags);
		m_hitemDrop = NULL;

		// Create drag image and begin dragging
		m_pImageList = CreateDragImage(m_hitemDrag);  
		m_pImageList->BeginDrag(0, CPoint(0,0));
		m_pImageList->DragEnter(GetDesktopWindow(), ptAction);
		SetCapture();

		SetDragTimer();
	}
	return TRUE;
}

void CUIDragDropTree::SetDragTimer()
{
	// Set the timer to slow down the scrolling when the dragging cursor
	// is close to the top/bottom border of the tree control
	if (!m_nTimerID) 
	{
		m_nTimerID = SetTimer(1, 75, NULL);
		CRect rect;
		GetClientRect(&rect);
		ClientToScreen(&rect);
		m_nUpperYCoor = rect.top;
		m_nLowerYCoor = rect.bottom;
	}
}

void CUIDragDropTree::KillDragTimer()
{
	if (m_nTimerID)
	{
		KillTimer(m_nTimerID);
	}
	m_nTimerID = 0;
	m_nUpperYCoor = 0;
	m_nLowerYCoor = 0;
	m_nSlowScrollTimeout = 0;
}

void CUIDragDropTree::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	if (nIDEvent != m_nTimerID)
	{
		CTreeCtrl::OnTimer(nIDEvent);
		return;
	}
	
	// Get the "current" mouse position
	const MSG* pMessage;
	CPoint ptMouse;
	pMessage = GetCurrentMessage();
	ASSERT(pMessage);
	ptMouse = pMessage->pt;

	// Move the ghost image
	if (m_pImageList)
		m_pImageList->DragMove(ptMouse);

	// Find out if scrolling is needed.
	// Scrolling is not needed for example, if the dragging cursor is 
	// within the tree control itself
	if(!NeedToScroll(ptMouse))
	{
	  	CTreeCtrl::OnTimer(nIDEvent);
		return;
	}
	if (!m_bDragging)
		m_OleDropTarget.EraseOldImage();

	// Refine the scrolling mode -
	// Scroll Up/Down, Slow/Normal
 	int nScrollMode = RefineScrollMode(ptMouse);

	switch(nScrollMode) 
	{
		case SCROLL_UP_SLOW:
		case SCROLL_DOWN_SLOW:
			if( m_nSlowScrollTimeout == 0)
				SendMessage( WM_VSCROLL, 
					nScrollMode == SCROLL_UP_SLOW ? SB_LINEUP : SB_LINEDOWN);
			m_nSlowScrollTimeout = ++m_nSlowScrollTimeout%SLOWSCROLL_FREQUENCY;
			break;
		case SCROLL_UP_NORMAL:
		case SCROLL_DOWN_NORMAL:
			SendMessage( WM_VSCROLL, 
				nScrollMode == SCROLL_UP_NORMAL ? SB_LINEUP : SB_LINEDOWN);
			break;
		default:
			ASSERT(FALSE);
			return;
			break;
	}	
	if (m_bDragging)
	{
		// Select the drop target
		if (m_pImageList)
			m_pImageList->DragLeave(this);
		HTREEITEM hitem = GetFirstVisibleItem();

		switch( nScrollMode )
		  {
			case SCROLL_UP_SLOW:
			case SCROLL_UP_NORMAL:
			  {
				SelectDropTarget(hitem);
				m_hitemDrop = hitem;
				break;
			  }
			case SCROLL_DOWN_SLOW:
			case SCROLL_DOWN_NORMAL:
			  {
		// Get the last visible item in the control
				int nCount = GetVisibleCount();
				for ( int i=0; i<nCount-1; ++i )
					hitem = GetNextVisibleItem(hitem);
				SelectDropTarget(hitem);
				m_hitemDrop = hitem;
				break;
			  }
			default:
				ASSERT(FALSE);
				return;
				break;
		  }

		if (m_pImageList)
			m_pImageList->DragEnter(this, ptMouse);	CTreeCtrl::OnTimer(nIDEvent);
	}
}

void CUIDragDropTree::OnRButtonUp(UINT nFlags, CPoint point) 
{
	if (!m_bDragging)
		return;
	// Kill the timer and reset all variables
	EndDragging();
	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_POPUPMENU_DRAGDROP));
	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);
	pPopup->SetDefaultItem(ID_POPUP_DD_MOVE);
	CPoint pts(point);
	ClientToScreen(&pts);
	if (GetRDragMenu(pPopup))
	{
		UINT nCmd = pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_NONOTIFY | TPM_RIGHTBUTTON | TPM_RETURNCMD, pts.x, pts.y,this);
		if (nCmd == ID_POPUP_DD_MOVE)
		{
			OnButtonUp(true);
		}
		else if (nCmd == ID_POPUP_DD_COPY)
		{
			OnButtonUp(false);
		}
	}
	else
	{
		// otherwise just move
		OnButtonUp(true);
	}	
	SelectDropTarget(NULL);
	CTreeCtrl::OnRButtonUp(nFlags, point);
}

void CUIDragDropTree::OnMouseMove(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	HTREEITEM			hItem;
	UINT				flags;

	if (!m_bDragging)
		return;
	CPoint ptScreen(point);
	ClientToScreen(&ptScreen);

	m_pImageList->DragMove(ptScreen);

	m_pImageList->DragShowNolock(FALSE);

	if ((hItem = HitTest(point, &flags)) != NULL)
	{
		CDD_OleDropTargetInfo Info(this->GetSafeHwnd(),point,NULL);
		Info.SetItem(hItem);
		LRESULT lResult = SendMessage(WM_APP_OLE_DD_OVER,(WPARAM)&Info);
		if (lResult == 0)
		{
			lResult = GetParent()->SendMessage(WM_APP_OLE_DD_OVER,(WPARAM)&Info);
		}
		if (lResult)
		{
			SelectDropTarget(hItem);
			m_hitemDrop = hItem;
		}
	}
	m_pImageList->DragShowNolock(TRUE);
	
	CTreeCtrl::OnMouseMove(nFlags, point);
}

void CUIDragDropTree::OnButtonUp(bool bMove)
{
	if (!m_bDragging)
		return;
	EndDragging();
	if (m_hitemDrag && m_hitemDrag != m_hitemDrop && !IsChildNodeOf(m_hitemDrop, m_hitemDrag) && 
								GetParentItem(m_hitemDrag) != m_hitemDrop)
	{
		CDD_OleDropTargetInfo Info(GetSafeHwnd(),CPoint(),NULL);
		Info.SetDropEffect(bMove ? DROPEFFECT_MOVE : DROPEFFECT_COPY);
		Info.SetItem(m_hitemDrop);
		LRESULT lResult = SendMessage(WM_APP_OLE_DD_DROP,(WPARAM)&Info);
		if (lResult == 0)
			lResult = GetParent()->SendMessage(WM_APP_OLE_DD_DROP,(WPARAM)&Info);
	}
}

void CUIDragDropTree::EndDragging()
{
	if (!m_bDragging)
		return;
	m_bDragging = FALSE;
	if (m_pImageList)
	{
		m_pImageList->DragLeave(this);
		m_pImageList->EndDrag();
	}
	delete m_pImageList;
	m_pImageList = NULL;
	ReleaseCapture();
	KillDragTimer();
}

void CUIDragDropTree::NewTransferItem(HTREEITEM hNewItem)
{
}

BOOL CUIDragDropTree::TransferItem(HTREEITEM hitemDrag, HTREEITEM hitemDrop)
{
	TV_INSERTSTRUCT		tvstruct;
	TCHAR				sztBuffer[256];
	HTREEITEM			hNewItem, hFirstChild;

		// avoid an infinite recursion situation
	tvstruct.item.hItem = hitemDrag;
	tvstruct.item.cchTextMax = sizeof(sztBuffer)-1;
	tvstruct.item.pszText = sztBuffer;
	tvstruct.item.mask = TVIF_CHILDREN | TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM;
	GetItem(&tvstruct.item);  
	tvstruct.hParent = hitemDrop;
	tvstruct.hInsertAfter = TVI_SORT;
	tvstruct.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM;
	hNewItem = InsertItem(&tvstruct);
	NewTransferItem(hNewItem);

	while ((hFirstChild = GetChildItem(hitemDrag)) != NULL)
	{
		// recursively transfer all the items
		TransferItem(hFirstChild, hNewItem);  
		// delete the first child and all its children
		DeleteItem(hFirstChild);		
	}
	return TRUE;
}



BOOL CUIDragDropTree::NeedToScroll( CPoint ptMouse )
{
	if (m_bDragging)
	{
		return ptMouse.y < m_nUpperYCoor || ptMouse.y > m_nLowerYCoor;
	}
	else
	{
		return ptMouse.y < m_nUpperYCoor+10 || ptMouse.y > m_nLowerYCoor-30;
	}
}


CUIDragDropTree::SCROLLMODE CUIDragDropTree::RefineScrollMode( CPoint ptMouse )
{
	int nYCoor = ptMouse.y;
	CUIDragDropTree::SCROLLMODE nScrollMode;

	if (m_bDragging)
	{
		nScrollMode = nYCoor > m_nLowerYCoor + m_nScrollBarSize ? SCROLL_DOWN_NORMAL :
					  nYCoor > m_nLowerYCoor ? SCROLL_DOWN_SLOW :
					  nYCoor < m_nUpperYCoor - m_nScrollBarSize ? SCROLL_UP_NORMAL :
					  SCROLL_UP_SLOW;
	}
	else
	{
		nScrollMode = nYCoor > m_nLowerYCoor-30 ? SCROLL_DOWN_NORMAL : 
					nYCoor < m_nUpperYCoor-10 ? SCROLL_UP_NORMAL : SCROLL_UP_NORMAL;
	}
	return nScrollMode;
}

BOOL CUIDragDropTree::IsChildNodeOf(HTREEITEM hitemChild, HTREEITEM hitemSuspectedParent)
{
	do
	{
		if (hitemChild == hitemSuspectedParent)
			break;
	}
	while ((hitemChild = GetParentItem(hitemChild)) != NULL);

	return (hitemChild != NULL);
}

void CUIDragDropTree::RegisterDropTarget()
{
	if (GetDragDrop())
	{
		VERIFY(m_OleDropTarget.Register(this));
		if (GetDropFiles())
			DragAcceptFiles();
	}
}

DROPEFFECT CUIDragDropTree::SelectCurrentTarget(CDD_OleDropTargetInfo *pInfo)
{
	HTREEITEM hItem;
	UINT flags;
	if ((hItem = HitTest(pInfo->GetPoint(), &flags)) != NULL) 
	{
		// select it
		SelectDropTarget(hItem);
		// save it
		m_hitemDrop = hItem;
		// returns 1 if allowed
		pInfo->SetItem(hItem);
		LRESULT lResult = SendMessage(WM_APP_OLE_DD_OVER,(WPARAM)pInfo);
		if (lResult == 0)
		{
			lResult = GetParent()->SendMessage(WM_APP_OLE_DD_OVER,(WPARAM)pInfo);
		}
		if (lResult)
		{
			if (::GetKeyState(VK_LCONTROL) < 0) 
			{
				if (ItemHasChildren(hItem))
				{
					Expand(hItem, TVE_EXPAND);
				}
			}
			return pInfo->GetDropEffect();
		}
	}
	return DROPEFFECT_NONE;
}

void CUIDragDropTree::OnDestroy() 
{

	CTreeCtrl::OnDestroy();
	
	// TODO: Add your message handler code here
	
}

int CUIDragDropTree::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CTreeCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	// TODO: Add your specialized creation code here
	RegisterDropTarget();
	
	return 0;
}

bool CUIDragDropTree::GetRDragMenu(CMenu *pMenu)
{
	return true;
}

void CUIDragDropTree::OnDDMove()
{
	m_CopyMode = eDDMove;
}

void CUIDragDropTree::OnDDCopy()
{
	m_CopyMode = eDDCopy;
}

void CUIDragDropTree::OnDDCancel()
{
	m_CopyMode = eDDCancel;
}

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

// URLTreeCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "UITreeCtrl.h"
#include "UIMessages.h"
#include "cbformats.h"
#include "UIres.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUITreeCtrl
IMPLEMENT_DYNAMIC(CUITreeCtrl,CUIDragDropTree)
IMPLEMENT_DYNAMIC(CRefresh,CObject)
IMPLEMENT_DYNAMIC(CRefreshCategory,CRefresh)


CUITreeCtrl::CUITreeCtrl(bool bDragDrop) 
: CUIDragDropTree(bDragDrop)
{
	m_idxClosed= 0;
	m_idxOpen = 0; 
	m_vkeydown = 0;
	m_IDTimer = 0;
	m_style = TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS;
	m_PopupID =	0;
	m_pPopupWnd = NULL;
	m_hOrigFont = NULL;
	m_bMe = false;
}

CUITreeCtrl::~CUITreeCtrl()
{
}

void CUITreeCtrl::OnDeleteItemData(DWORD dwData)
{

}

void CUITreeCtrl::ItemPrePaint(LPNMTVCUSTOMDRAW lptvcd,LRESULT *pResult)
{
	*pResult = CDRF_DODEFAULT;
	if (lptvcd->nmcd.dwDrawStage & CDDS_ITEM)
	{
		UINT state = GetItemState((HTREEITEM)lptvcd->nmcd.dwItemSpec,TVIS_SELECTED | TVIS_EXPANDED);
		if ((state & TVIS_SELECTED))
			return;
		if ((state & TVIS_SELECTED) && (state & TVIS_EXPANDED))
			return;
	}
	HTREEITEM hItem = (HTREEITEM)lptvcd->nmcd.dwItemSpec;
	CUIListCtrlData* pData = GetListCtrlData(hItem);
	if (pData == NULL)
		return;
	*pResult = CDRF_NOTIFYPOSTPAINT;
	if (pData->IsFontSet())
	{
		CDC *pDC = CDC::FromHandle(lptvcd->nmcd.hdc);
		CFont *pOldFont = pDC->SelectObject((CFont*)pData->GetFont());
		m_hOrigFont = (HFONT)pOldFont;
		*pResult |= CDRF_NEWFONT;
	}
	lptvcd->clrText = pData->GetTextColor();
	lptvcd->clrTextBk = pData->GetBkColor();
}

void CUITreeCtrl::ItemPostPaint(LPNMTVCUSTOMDRAW lptvcd,LRESULT *pResult)
{
	CDC *pDC = CDC::FromHandle(lptvcd->nmcd.hdc);
	if (m_hOrigFont)
	{
		pDC->SelectObject(CFont::FromHandle(m_hOrigFont));
		m_hOrigFont = NULL;
	}
	*pResult = CDRF_DODEFAULT;
}

bool CUITreeCtrl::EndLabelEdit(HTREEITEM hItem,LPCTSTR pszText)
{
	return false;
}

UINT CUITreeCtrl::GetMenuID()
{
	return m_PopupID;
}

CWnd *CUITreeCtrl::GetMenuWnd()
{
	return m_pPopupWnd;
}

void CUITreeCtrl::SetPopupID(UINT nID)
{
	m_PopupID = nID;
}

void CUITreeCtrl::SetPopupWnd(CWnd *pWnd)
{
	m_pPopupWnd = pWnd;
}

bool CUITreeCtrl::DragDrop(CDD_OleDropTargetInfo *pInfo)
{
	if (pInfo->GetDataObject() == NULL)
	{
		HTREEITEM hItem = pInfo->GetTreeItem();
		if (pInfo->GetDropEffect() == DROPEFFECT_MOVE)
		{
			if (TransferItem(GetDragItem(),hItem))
			{
				DeleteItem(GetDragItem());
			}
		}
		else
		{
			AfxMessageBox(_T("Copy not supported!"));
		}
		return true;
	}
	return false;
}

bool CUITreeCtrl::DragEnter(CDD_OleDropTargetInfo *pInfo)
{
	return false;
}

bool CUITreeCtrl::DragLeave(CDD_OleDropTargetInfo *pInfo)
{
	return false;
}

bool CUITreeCtrl::DragOver(CDD_OleDropTargetInfo *pInfo)
{
	// possible non ole drag & drop
	if (pInfo->GetDataObject() == NULL)
	{
		return true;
	}
	// ole drag and drop expects the dropeffect to be set
	// any valid formats?
	if (CWDClipboardData::Instance()->IsDataAvailable(pInfo->GetDataObject()))
	{
		FORMATETC FormatEtc;
		FormatEtc.cfFormat = CWDClipboardData::Instance()->GetClipboardFormat(CWDClipboardData::e_cfFileGroupDesc);
		FormatEtc.ptd = NULL;
		FormatEtc.dwAspect = DVASPECT_CONTENT;
		FormatEtc.lindex = -1;
		FormatEtc.tymed = TYMED_HGLOBAL;
		HRESULT hr = pInfo->GetDataObject()->m_lpDataObject->QueryGetData(&FormatEtc);
		if ((pInfo->GetKeyboardState() & MK_SHIFT))
			pInfo->SetDropEffect(DROPEFFECT_MOVE);
		else if (hr == S_OK)
		{
			pInfo->SetDropEffect(DROPEFFECT_COPY);
		}
		else
			pInfo->SetDropEffect(DROPEFFECT_COPY);
		return true;
	}
	return false;
}

DROPEFFECT CUITreeCtrl::DoDragDrop(NM_TREEVIEW* pNMTreeView,COleDataSource *pOleDataSource)
{
	// non ole drag and drop only within tree
	CCF_String ccfText(GetItemText(pNMTreeView->itemNew.hItem));
	CWDClipboardData::Instance()->SetData(pOleDataSource,&ccfText,CWDClipboardData::e_cfString);
	return DROPEFFECT_ALL;
}

CRefresh *CUITreeCtrl::CreateRefreshObject(HTREEITEM hItem,LPARAM lParam)
{
	return new CRefresh(hItem,lParam);
}

void CUITreeCtrl::UpdateEvent(LPARAM lHint,CObject *pHint)
{
	GetParent()->SendMessage(WM_APP_UPDATE_ALL_VIEWS,(WPARAM)lHint,(LPARAM)pHint);
}

void CUITreeCtrl::UpdateCurrentSelection()
{
	HTREEITEM hItem = GetSelectedItem();
	if (hItem)
	{
		SelectionChanged(hItem,GetItemData(hItem));
	}
}

void CUITreeCtrl::DoubleClick(HTREEITEM hItem)
{
}

void CUITreeCtrl::SelectionChanged(HTREEITEM hItem,LPARAM lParam)
{
	CRefresh *pRefresh = CreateRefreshObject(hItem,lParam);
	if (pRefresh)
		UpdateEvent(HINT_TREE_SEL_CHANGED,pRefresh);
	delete pRefresh;
}

HTREEITEM CUITreeCtrl::AddAnItem(HTREEITEM hParent, LPCTSTR szText, LPARAM lParam, HTREEITEM hInsAfter,int iImage,int iSelImage, int nChildren)
{
	ASSERT_VALID(this);

	TV_ITEM tvI;
	TV_INSERTSTRUCT tvIns;
	ZeroMemory(&tvI,sizeof(tvI));
	ZeroMemory(&tvIns,sizeof(tvIns));
	tvI.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
	if (nChildren) // get the plus sign
	{
		tvI.mask |= TVIF_CHILDREN;
		tvI.cChildren = nChildren;
	}
	tvI.pszText = (LPTSTR)szText;
	tvI.cchTextMax = lstrlen(szText);
	if (iImage == 0)
		iImage = m_idxClosed;
	tvI.iImage = iImage;
	if (iSelImage == 0)
		iSelImage = iImage;
	tvI.iSelectedImage = iSelImage;
	tvI.lParam = (LPARAM)GetNewListCtrlData(lParam,hParent);
	tvIns.item = tvI;
	tvIns.hInsertAfter = hInsAfter;
	tvIns.hParent = hParent;
	
	m_bMe = true;
	HTREEITEM hNewItem = InsertItem(&tvIns);
	m_bMe = false;
	return hNewItem;
}

CUIListCtrlData *CUITreeCtrl::GetNewListCtrlData(DWORD dwData,HTREEITEM hParent)
{
	CUIListCtrlData *pListCtrlData = new CUIListCtrlData;
	pListCtrlData->SetExtData(dwData); 
	return pListCtrlData;
}

DWORD CUITreeCtrl::GetItemData(HTREEITEM hItem) const
{
	if (hItem == NULL)
		return 0;
	const CUIListCtrlData* pData = GetListCtrlData(hItem);
	if (pData)
		return pData->GetExtData();
	return 0;
}

BOOL CUITreeCtrl::SetItemData(HTREEITEM hItem,DWORD dwData,bool bAutoDelete)
{
	ASSERT(hItem);
	if (hItem == NULL) 
		return FALSE;
	CUIListCtrlData* pData = GetListCtrlData(hItem);
	if (pData == NULL) 
		return FALSE;
	pData->SetExtData(dwData);
	pData->SetAutoDelete(bAutoDelete);
	return TRUE;
}

CUIListCtrlData *CUITreeCtrl::GetListCtrlData(HTREEITEM hItem) const
{
	((CUITreeCtrl*)this)->m_bMe = true;
	CUIListCtrlData *pData = (CUIListCtrlData*)CTreeCtrl::GetItemData(hItem);
	((CUITreeCtrl*)this)->m_bMe = false;
	if (pData == NULL)
		return NULL;
	ASSERT(pData->IsKindOf(RUNTIME_CLASS(CUIListCtrlData)));
	return pData;
}

void CUITreeCtrl::SetTextColor(HTREEITEM hItem,COLORREF TextColor)
{
	CUIListCtrlData *pData = GetListCtrlData(hItem);
	if (pData)
		pData->SetTextColor(TextColor);
};

void CUITreeCtrl::SetBkColor(HTREEITEM hItem,COLORREF BkColor)
{
	CUIListCtrlData *pData = GetListCtrlData(hItem);
	if (pData)
		pData->SetBkColor(BkColor);
}

void CUITreeCtrl::SetDefaultTextColor(HTREEITEM hItem)
{
	CUIListCtrlData *pData = GetListCtrlData(hItem);
	if (pData)
		pData->SetDefaultTextColor();
};

void CUITreeCtrl::SetDefaultBkColor(HTREEITEM hItem)
{
	CUIListCtrlData *pData = GetListCtrlData(hItem);
	if (pData)
		pData->SetDefaultBkColor();
}

void CUITreeCtrl::SetItemFont(HTREEITEM hItem,CFont *pFont)
{
	CUIListCtrlData* pData = GetListCtrlData(hItem);
	if (pData == NULL)
		return;
	ASSERT(pFont);
	if (pFont)
		pData->SetFont(pFont);
}

void CUITreeCtrl::SetItemBold(HTREEITEM hItem,bool bBold)
{
	CUIListCtrlData* pData = GetListCtrlData(hItem);
	if (pData == NULL)
		return;
	if (!pData->IsFontSet())
		pData->SetFont(GetFont());
	const CFont *pFont = pData->GetFont();
	LOGFONT lf;
	((CFont*)pFont)->GetLogFont(&lf);
	lf.lfWeight = bBold ? FW_BOLD : FW_NORMAL; 
	CFont font;
	font.CreateFontIndirect(&lf);
	pData->SetFont(&font);
	CRect rect;
	if (GetItemRect(hItem,&rect,FALSE))
	{
		InvalidateRect(rect);
		UpdateWindow();
	}
}

HTREEITEM CUITreeCtrl::GetHitItem(CPoint *pt)
{
	UINT Flag;
	HTREEITEM hItem;
	CPoint cpt;

	POINT pt1;
	GetCursorPos(&pt1);
	ScreenToClient(&pt1);	
	cpt.x = pt1.x;
	cpt.y = pt1.y;
	hItem = HitTest(cpt, &Flag);
	if (hItem && ((Flag & TVHT_ONITEMLABEL) || (Flag & TVHT_ONITEMICON)))
	{
		if (pt)
			*pt = pt1;
		return hItem;
	}
	return NULL;
}

void CUITreeCtrl::Refresh()
{
}

void CUITreeCtrl::DeleteKey(HTREEITEM hItem)
{
}

void CUITreeCtrl::GoBack(HTREEITEM hItem)
{
}

void CUITreeCtrl::ShowProperties(HTREEITEM hItem)
{
}

void CUITreeCtrl::ShowPopupMenu(HTREEITEM hItem,CPoint point)
{
	UINT MenuID = GetMenuID();
	if (MenuID == 0) 
		return;
	CWnd *pWndMess = GetMenuWnd();
	if (pWndMess == NULL)
		pWndMess = this;
	CMenu menu;
	VERIFY(menu.LoadMenu(MenuID));
	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWndMess);
}

BEGIN_MESSAGE_MAP(CUITreeCtrl, CUIDragDropTree)
	//{{AFX_MSG_MAP(CUITreeCtrl)
	ON_NOTIFY_REFLECT_EX(NM_DBLCLK, OnDblclk)
	ON_NOTIFY_REFLECT_EX(NM_RETURN, OnReturn)
	ON_NOTIFY_REFLECT_EX(NM_RCLICK, OnRclick)
	ON_NOTIFY_REFLECT_EX(NM_CUSTOMDRAW, OnCustomDraw)
	ON_NOTIFY_REFLECT_EX(TVN_ITEMEXPANDED, OnItemExpanded)
	ON_NOTIFY_REFLECT_EX(TVN_ITEMEXPANDING, OnItemExpanding)
	ON_NOTIFY_REFLECT_EX(TVN_SELCHANGED, OnSelchanged)
	ON_NOTIFY_REFLECT_EX(TVN_ENDLABELEDIT, OnEndLabelEdit)
	ON_NOTIFY_REFLECT_EX(TVN_DELETEITEM, OnDeleteItem)
	ON_NOTIFY_REFLECT(TVN_KEYDOWN, OnKeydownFolder)
	ON_WM_MOUSEWHEEL()
	ON_WM_TIMER()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_APP_ON_PROPERTIES_KEY,OnAppPropertiesKey)
	ON_MESSAGE(WM_APP_ON_DELETE_KEY,OnAppDeleteKey)
	ON_MESSAGE(WM_APP_ON_CONTEXT_MENU_KEY,OnAppContextMenuKey)
	ON_MESSAGE(WM_APP_ON_BACKSPACE_KEY,OnAppBackspaceKey)
	ON_MESSAGE(WM_APP_ON_EDIT_KEY,OnAppEditKey)
	ON_MESSAGE(WM_APP_ON_REFRESH_KEY,OnAppRefreshKey)

	ON_MESSAGE(TVM_GETITEM,OnTVMGetItem)
	ON_MESSAGE(TVM_SETITEM,OnTVMSetItem)
	ON_MESSAGE(TVM_INSERTITEM,OnTVMInsertItem)
	ON_MESSAGE(WM_APP_TIMER_SEL_CHANGE,OnTimerSelChange)
	ON_MESSAGE(WM_APP_OLE_DD_DODRAGDROP, OnDDDoDragDrop)
	ON_MESSAGE(WM_APP_OLE_DD_DROP, OnDragDrop)
	ON_MESSAGE(WM_APP_OLE_DD_OVER, OnDragOver)
	ON_MESSAGE(WM_APP_OLE_DD_ENTER, OnDragEnter)
	ON_MESSAGE(WM_APP_OLE_DD_LEAVE, OnDragLeave)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUITreeCtrl message handlers
BOOL CUITreeCtrl::OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMTVCUSTOMDRAW lptvcd = (LPNMTVCUSTOMDRAW)pNMHDR;
	if(lptvcd->nmcd.dwDrawStage == CDDS_PREPAINT)
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if(lptvcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
	{
		ItemPrePaint(lptvcd,pResult);
	}
	else if(lptvcd->nmcd.dwDrawStage == CDDS_ITEMPOSTPAINT)
	{
		ItemPostPaint(lptvcd,pResult);
	}
	return TRUE;
}

BOOL CUITreeCtrl::OnRclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	CPoint point;
	HTREEITEM hItem = GetHitItem(&point);
	if (hItem == NULL) 
		return FALSE;
	ClientToScreen(&point);
	ShowPopupMenu(hItem,point);
	*pResult = 0;
	return TRUE;
}

BOOL CUITreeCtrl::OnDeleteItem(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	CUIListCtrlData* pData = (CUIListCtrlData*)pNMTreeView->itemOld.lParam;
	if (!pData->GetAutoDelete())
		OnDeleteItemData(pData->GetExtData());
	delete pData;
	return TRUE;
}

LRESULT CUITreeCtrl::OnTVMGetItem(WPARAM wParam,LPARAM lParam)
{
	LPTVITEM pitem = (LPTVITEM)lParam;
	if (m_bMe == false && pitem->mask & TVIF_PARAM)
	{
		CUIListCtrlData* pData = GetListCtrlData(pitem->hItem);
		if (pData)
		{
			pitem->lParam = pData->GetExtData();
			pitem->mask &= ~TVIF_PARAM;
		}
	}
	return Default();
}

LRESULT CUITreeCtrl::OnTVMSetItem(WPARAM wParam,LPARAM lParam)
{
	LPTVITEM pitem = (LPTVITEM)lParam;
	if (m_bMe == false && pitem->mask & TVIF_PARAM)
	{
		CUIListCtrlData* pData = GetListCtrlData(pitem->hItem);
		if (pData)
		{
			pData->SetExtData(pitem->lParam);
			pitem->mask &= ~TVIF_PARAM;
		}
	}
	return Default();
}

LRESULT CUITreeCtrl::OnTVMInsertItem(WPARAM wParam,LPARAM lParam)
{
	LPTVINSERTSTRUCT lpis = (LPTVINSERTSTRUCT)lParam;
	if (m_bMe == false && lpis->item.mask & TVIF_PARAM)
	{
		lpis->item.lParam = (LPARAM)GetNewListCtrlData(lpis->item.lParam,lpis->hParent);
	}
	return Default();
}

int CUITreeCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CUIDragDropTree::OnCreate(lpCreateStruct) == -1)
		return -1;

	Init();

	// TODO: Add your specialized creation code here
	return 0;
}

UINT CUITreeCtrl::GetIconID()
{
	return IDI_FOLDER_CLOSED;
}

UINT CUITreeCtrl::GetOpenIconID()
{
	return 0;
}

bool CUITreeCtrl::Expanding(NM_TREEVIEW *nmtvw)
{
	return false;
}

bool CUITreeCtrl::Collapsing(NM_TREEVIEW *nmtvw)
{
	return true;
}

void CUITreeCtrl::Init()
{
	// Create the image list we will need
	int nCxSmall = GetSystemMetrics(SM_CXSMICON);
	int nCySmall = GetSystemMetrics(SM_CYSMICON);
	m_ImageList.Create(nCxSmall,
						nCySmall,
						TRUE,	// list does not include masks
						2,
						0 );	// list won't grow
	UINT nIconID = GetIconID();
	if (nIconID)
	{
		HINSTANCE hInstIcon = AfxFindResourceHandle(MAKEINTRESOURCE(nIconID),RT_GROUP_ICON);
		HICON hIcon = (HICON)LoadImage( hInstIcon, MAKEINTRESOURCE(nIconID), IMAGE_ICON, 
			nCxSmall, nCySmall, 0 );
		m_idxClosed = m_ImageList.Add(hIcon);
	}
	UINT nOpenIconID = GetOpenIconID();
	if (nOpenIconID)
	{
		HINSTANCE hInstIcon = AfxFindResourceHandle(MAKEINTRESOURCE(nOpenIconID),RT_GROUP_ICON);
		HICON hOpenIcon = (HICON)LoadImage( hInstIcon, MAKEINTRESOURCE(nOpenIconID), IMAGE_ICON, 
			nCxSmall, nCySmall, 0 );
		m_idxOpen = m_ImageList.Add(hOpenIcon);
	}
	// Associate the image list with the tree
	SetImageList(&m_ImageList,TVSIL_NORMAL);
	m_ImageList.SetBkColor(CLR_NONE);
	ModifyStyle(0,m_style);
}

BOOL CUITreeCtrl::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	cs.style |= m_style;
	return CUIDragDropTree::PreCreateWindow(cs);
}

BOOL CUITreeCtrl::OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMTVDISPINFO pDispInfo = (LPNMTVDISPINFO)pNMHDR;
	if (pDispInfo->item.pszText)
		*pResult = EndLabelEdit(pDispInfo->item.hItem,pDispInfo->item.pszText);
	return TRUE;
}

BOOL CUITreeCtrl::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	HTREEITEM hItem = GetSelectedItem();
	DoubleClick(hItem);
	return TRUE;
}

BOOL CUITreeCtrl::OnReturn(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	OnDblclk(pNMHDR,pResult);	
	return TRUE;
}

BOOL CUITreeCtrl::OnItemExpanding(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	CWaitCursor w;
	// TODO: Add your control notification handler code here
	if (pNMTreeView->action == TVE_EXPAND)
	{
		Expanding(pNMTreeView);
	}
	else if (pNMTreeView->action == TVE_COLLAPSE)
	{
		Collapsing(pNMTreeView);
	}
	return TRUE;
}

BOOL CUITreeCtrl::OnItemExpanded(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
/*	if (pNMTreeView->action == TVE_EXPAND)
	{
		SetItemImage(pNMTreeView->itemNew.hItem,m_idxClosed,0);
	}
	else if (pNMTreeView->action == TVE_COLLAPSE)
	{
		SetItemImage(pNMTreeView->itemNew.hItem,m_idxOpen,0);
	}*/
	return TRUE;
}

BOOL CUITreeCtrl::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	//VK_PRIOR 21 page up key  
	//VK_NEXT 22 page down key  
	//VK_END 23 end key  
	//VK_HOME 24 home key  
	//VK_LEFT 25 left arrow key  
	//VK_UP 26 up arrow key  
	//VK_RIGHT 27 right arrow key  
	//VK_DOWN 28 down arrow key  

	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	if ((pNMTreeView->itemNew.state & TVIS_SELECTED)) 
	{
		if (pNMTreeView->action == TVC_BYMOUSE || pNMTreeView->action == TVC_UNKNOWN)
		{
			CUIListCtrlData *pData = (CUIListCtrlData*)pNMTreeView->itemNew.lParam;
			if (pData)
			{
				ASSERT_KINDOF(CUIListCtrlData,pData);
				SelectionChanged(pNMTreeView->itemNew.hItem,pData->GetExtData());
			}
			else
				SelectionChanged(pNMTreeView->itemNew.hItem,pNMTreeView->itemNew.lParam);
		}
		else if (pNMTreeView->action == TVC_BYKEYBOARD)
		{
			CopyMemory(&m_NMTreeView,pNMTreeView,sizeof(NM_TREEVIEW));
			if (m_vkeydown && m_IDTimer == 0)
			{
				TRACE0("Timer set\n");
				m_IDTimer = SetTimer(2,100,NULL);
			}
		}
	}	
	*pResult = 0;
	return TRUE;
}

BOOL CUITreeCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	// TODO: Add your message handler code here and/or call default
	return CUIDragDropTree::OnMouseWheel(nFlags, zDelta, pt);
}

BOOL CUITreeCtrl::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN)
	{
		if (pMsg->wParam == VK_F10 && (GetKeyState(VK_SHIFT) < 0))
		{
			PostMessage(WM_APP_ON_CONTEXT_MENU_KEY);
			return TRUE;
		}
		else if (pMsg->wParam == VK_BACK)
		{
			PostMessage(WM_APP_ON_BACKSPACE_KEY);
			return TRUE;
		}
		else if (pMsg->wParam == VK_F2)
		{
			PostMessage(WM_APP_ON_EDIT_KEY);
			return TRUE;
		}
		else if (pMsg->wParam == VK_F5)
		{
			PostMessage(WM_APP_ON_REFRESH_KEY);
			return TRUE;
		}
		else if (pMsg->wParam == VK_DELETE)
		{
			PostMessage(WM_APP_ON_DELETE_KEY);
			return TRUE;
		}
		else if (pMsg->message == WM_SYSKEYDOWN && pMsg->wParam == VK_RETURN)
		{
			PostMessage(WM_APP_ON_PROPERTIES_KEY);
			return TRUE;
		}
	}
	
	return CUIDragDropTree::PreTranslateMessage(pMsg);
}

void CUITreeCtrl::OnKeydownFolder(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TV_KEYDOWN* pTVKeyDown = (TV_KEYDOWN*)pNMHDR;
	// TODO: Add your control notification handler code here
	m_vkeydown = pTVKeyDown->wVKey;
	*pResult = 0;
}

LRESULT CUITreeCtrl::OnTimerSelChange(WPARAM wParam,LPARAM lParam)
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)&m_NMTreeView;
	CUIListCtrlData *pData = (CUIListCtrlData*)pNMTreeView->itemNew.lParam;
	if (pData)
	{
		ASSERT_KINDOF(CUIListCtrlData,pData);
		SelectionChanged(pNMTreeView->itemNew.hItem,pData->GetExtData());
	}
	else
		SelectionChanged(pNMTreeView->itemNew.hItem,pNMTreeView->itemNew.lParam);
	return 1L;
}	

void CUITreeCtrl::OnTimer( UINT nIDEvent )
{
	CUIDragDropTree::OnTimer(nIDEvent);
	if (nIDEvent == m_IDTimer)
	{
		if (m_vkeydown)
		{
			SHORT sRet = GetAsyncKeyState(m_vkeydown);
			if (sRet >= 0)
			{
				KillTimer(m_IDTimer);
				m_IDTimer = 0;
				m_vkeydown = 0;
				PostMessage(WM_APP_TIMER_SEL_CHANGE);
			}
		}
	}
}

LRESULT CUITreeCtrl::OnDragDrop(WPARAM wParam,LPARAM lParam)
{
	if (GetDragDrop() == false)
		return 1L;
	// get the info we need
	CDD_OleDropTargetInfo *pInfo = (CDD_OleDropTargetInfo*)wParam;
	ASSERT(pInfo);
	if (pInfo->GetDataObject() != NULL)
		return DragDrop(pInfo) ? 1 : 0;
	else
		return 1;
}

// user is currently over the tree view window
// return 1 if we are interested in the CB format
LRESULT CUITreeCtrl::OnDragOver(WPARAM wParam,LPARAM lParam)
{
	if (GetDragDrop() == false)
		return 1L;
	CDD_OleDropTargetInfo *pInfo = (CDD_OleDropTargetInfo*)wParam;
	ASSERT(pInfo);
	if (pInfo->GetDataObject() != NULL)
		return DragOver(pInfo) ? 1 : 0;
	else
		return 1;
}

LRESULT CUITreeCtrl::OnDragEnter(WPARAM wParam,LPARAM lParam)
{
	if (GetDragDrop() == false)
		return 1L;
	CDD_OleDropTargetInfo *pInfo = (CDD_OleDropTargetInfo*)wParam;
	ASSERT(pInfo);
	if (pInfo->GetDataObject() != NULL)
		return DragEnter(pInfo) ? 1 : 0;
	else
		return 1;
}

LRESULT CUITreeCtrl::OnDragLeave(WPARAM wParam,LPARAM lParam)
{
	if (GetDragDrop() == false)
		return 1L;
	CDD_OleDropTargetInfo *pInfo = (CDD_OleDropTargetInfo*)wParam;
	ASSERT(pInfo);
	if (pInfo->GetDataObject() != NULL)
		return DragLeave(pInfo) ? 1 : 0;
	else
		return 1;
}

// Drag and drop initiated
// Return 1 if processed
LRESULT CUITreeCtrl::OnDDDoDragDrop(WPARAM wParam,LPARAM lParam)
{
	if (GetDragDrop() == false)
		return 1L;
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)wParam;
	COleDataSource *pOleDataSource = (COleDataSource*)lParam;
	ASSERT(pOleDataSource);
	return DoDragDrop(pNMTreeView,pOleDataSource);
}

LRESULT CUITreeCtrl::OnAppPropertiesKey(WPARAM wParam, LPARAM lParam)
{
	ShowProperties(GetSelectedItem());
	return 1L;
}

LRESULT CUITreeCtrl::OnAppDeleteKey(WPARAM wParam, LPARAM lParam)
{
	DeleteItem(GetSelectedItem());
	return 1L;
}

LRESULT CUITreeCtrl::OnAppRefreshKey(WPARAM wParam, LPARAM lParam)
{
	Refresh();
	return 1L;
}

LRESULT CUITreeCtrl::OnAppEditKey(WPARAM wParam, LPARAM lParam)
{
	EditLabel(GetSelectedItem());
	return 1L;
}

LRESULT CUITreeCtrl::OnAppContextMenuKey(WPARAM wParam, LPARAM lParam)
{
	CRect rc;
	GetItemRect(GetSelectedItem(),&rc,FALSE);
	ClientToScreen(&rc);
	ShowPopupMenu(GetSelectedItem(),rc.CenterPoint());
	return 1L;
}

LRESULT CUITreeCtrl::OnAppBackspaceKey(WPARAM wParam, LPARAM lParam)
{
	GoBack(GetSelectedItem());
	return 1L;
}

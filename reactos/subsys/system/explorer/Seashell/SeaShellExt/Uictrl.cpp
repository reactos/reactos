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
#include "InPlaceEdit.h"
#include "TextProgressCtrl.h"
#include "UIres.h"
#include "ctype.h"
#include "cbformats.h"
#include <yvals.h>

//////////////////////////////////////////////////////
// Temporarily changes the view type
//////////////////////////////////////////////////////
CChangeViewType::CChangeViewType(CUIODListCtrl *pLC,DWORD dwType)
{
	m_pLC = pLC;
	m_dwType = m_pLC->GetViewType();	
	m_pLC->SetViewType(dwType);
}

CChangeViewType::~CChangeViewType()
{
	m_pLC->SetViewType(m_dwType);
}
//////////////////////////////////////////////////////////

LPCTSTR CUIODListCtrl::szEntryHeadings		= _T("Headings");
LPCTSTR CUIODListCtrl::szEntryStyle			= _T("Style");
LPCTSTR CUIODListCtrl::szEntryRowSel		= _T("RowSelection");
LPCTSTR CUIODListCtrl::szEntryViewType		= _T("ViewType");
LPCTSTR CUIODListCtrl::szEntryColumnSizing	= _T("ColumnSizing");
LPCTSTR CUIODListCtrl::szEntrySortColumn	= _T("SortColumn");
LPCTSTR CUIODListCtrl::szEntrySubItems		= _T("Columns");
LPCTSTR CUIODListCtrl::szEntryColOrder		= _T("ColumnOrder");
LPCTSTR CUIODListCtrl::szEntryColWidths		= _T("ColumnWidths");

IMPLEMENT_SERIAL(CUIODListCtrl, CListCtrl, 0)

#define COLUMN_DELIMITER '|'
#define COLUMN_DELIMITER_STR "|"

const int nMaxDigLen = _MAX_INT_DIG*4;

//////////////////////////////
//
// CUIODListCtrl
//
//////////////////////////////
BEGIN_MESSAGE_MAP(CUIODListCtrl, CListCtrl)
	//{{AFX_MSG_MAP(CUIODListCtrl)
	ON_MESSAGE(WM_APP_ON_PROPERTIES_KEY,OnAppPropertiesKey)
	ON_MESSAGE(WM_APP_ON_DELETE_KEY,OnAppDeleteKey)
	ON_MESSAGE(WM_APP_ON_CONTEXT_MENU_KEY,OnAppContextMenuKey)
	ON_MESSAGE(WM_APP_ON_BACKSPACE_KEY,OnAppBackspaceKey)
	ON_MESSAGE(WM_APP_ON_EDIT_KEY,OnAppEditKey)
	ON_MESSAGE(WM_APP_ON_REFRESH_KEY,OnAppRefreshKey)

	ON_NOTIFY_REFLECT_EX(LVN_BEGINDRAG, OnBegindrag)
	ON_NOTIFY_REFLECT_EX(LVN_BEGINRDRAG, OnBeginRDrag)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_MESSAGE(LVM_SETCOLUMNWIDTH,OnSetColumnWidth)
	ON_MESSAGE(LVM_DELETEALLITEMS,OnDeleteAllItems)
	ON_MESSAGE(LVM_DELETEITEM,OnDeleteItem)
	ON_MESSAGE(LVM_DELETECOLUMN,OnDeleteColumn)
	ON_MESSAGE(WM_UPDATEHEADERWIDTH,OnUpdateHeaderWidth)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_CONTEXTMENU()
	ON_WM_INITMENUPOPUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_COMMAND(ID_UI_VIEW_LARGE_ICONS,OnViewLargeIcons)
	ON_COMMAND(ID_UI_VIEW_SMALL_ICONS,OnViewSmallIcons)
	ON_COMMAND(ID_UI_VIEW_DETAILS,OnViewDetails)
	ON_COMMAND(ID_UI_VIEW_LIST,OnViewList)
	ON_COMMAND(ID_UI_VIEW_FULL_ROW_SELECTION,OnViewFullRowSelection)
	ON_COMMAND(ID_UI_VIEW_COLUMN_ORDERING,OnViewColumnOrdering)
	ON_COMMAND(ID_UI_VIEW_COLUMN_SIZING,OnViewColumnSizing)
	ON_COMMAND(ID_UI_VIEW_GRIDLINES,OnViewGridlines)
	ON_COMMAND(ID_UI_VIEW_CHECKBOXES,OnViewCheckboxes)
	ON_COMMAND(ID_UI_VIEW_TRACK_SELECT,OnViewTrackSelect)
	ON_COMMAND(ID_UI_VIEW_EDIT_COLUMN,OnViewEditColumn)
	ON_COMMAND(ID_HEADER_REMOVE_COL,OnHeaderRemoveColumn)
	ON_COMMAND(ID_HEADER_EDIT_TEXT,OnHeaderEditText)
	ON_COMMAND(ID_HEADER_RESET,OnHeaderReset)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
	ON_WM_DROPFILES()
	ON_MESSAGE(LVM_SETIMAGELIST, OnSetImageList)
	ON_MESSAGE(LVM_SETTEXTCOLOR, OnSetTextColor)
	ON_MESSAGE(LVM_SETTEXTBKCOLOR, OnSetTextBkColor)
	ON_MESSAGE(LVM_SETBKCOLOR, OnSetBkColor)
	ON_MESSAGE(LVM_GETITEMTEXT, OnGetItemText)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_APP_OLE_DD_DROP, OnDragDrop)
	ON_MESSAGE(WM_APP_OLE_DD_DODRAGDROP, OnDDDoDragDrop)
	ON_MESSAGE(WM_APP_OLE_DD_OVER, OnDragOver)
	ON_MESSAGE(WM_APP_OLE_DD_ENTER, OnDragEnter)
	ON_MESSAGE(WM_APP_OLE_DD_LEAVE, OnDragLeave)
END_MESSAGE_MAP()


CUIODListCtrl::CUIODListCtrl(bool bDragDrop) 
{
	m_PopupID = 0;
	m_MultiPopupID = 0;
	m_bDragDrop = bDragDrop;
	m_bDropFiles = true;
	m_pColWidths = NULL;
	m_pColOrder = NULL;
	m_pColTypes = NULL;
	m_hOrigFont = NULL;
	m_pPopupWnd = NULL;
	m_nImageList = LVSIL_SMALL;
	m_nColClicked = -1;
	m_nSortColumn = 0;
	m_nSubItems = 0;
	m_nItems = 0;
	m_nImage = 0;
	m_dwExStyle = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_INFOTIP;
	m_dwViewType = LVS_REPORT;
	m_bDelayPaint = false;
	m_bColumnSizing=TRUE;
	m_bRClickChangeIconSize	= TRUE;
	// this is toggled on in InitListCtrl
	m_bFullRowSel=FALSE;
	m_bClientWidthSel=TRUE;
	m_bSortAscending=TRUE;
	m_bToolTips=FALSE;
	m_bEditSubItems = true;
	m_cxClient=0;
	m_cxStateImageOffset=0;

	m_clrText=::GetSysColor(COLOR_WINDOWTEXT);
	m_clrTextBk=::GetSysColor(COLOR_WINDOW);
	m_clrBkgnd=::GetSysColor(COLOR_WINDOW);
	m_strNoItemsMess.LoadString(IDS_NO_ITEMS_MESS);
}

CUIODListCtrl::~CUIODListCtrl()
{
	Empty();
}

void CUIODListCtrl::AllItemsDeleted()
{

}

void CUIODListCtrl::Empty()
{
	m_objList.DeleteAll();
	delete []m_pColWidths;
	delete []m_pColOrder;
	delete []m_pColTypes;
	m_pColWidths = NULL;
	m_pColOrder = NULL;
	m_pColTypes = NULL;
}

// should be overridden to do any initialization
void CUIODListCtrl::Init()
{
}

void CUIODListCtrl::UpdateEvent(LPARAM lHint,CObject *pHint)
{
	GetParent()->SendMessage(WM_APP_UPDATE_ALL_VIEWS,(WPARAM)lHint,(LPARAM)pHint);
}

BOOL CUIODListCtrl::SetViewType(DWORD dwViewType,UINT nFlags)
{
	BOOL bRet = ModifyStyle(LVS_TYPEMASK,dwViewType & LVS_TYPEMASK,nFlags);
	if (bRet)
		SaveProfile();
	return bRet;
}

DWORD CUIODListCtrl::GetViewType() const
{
	return(GetStyle() & LVS_TYPEMASK);
}

void CUIODListCtrl::ChangeStyle(UINT &dwStyle)
{
	LoadProfile();
	dwStyle |= LVS_SHOWSELALWAYS | m_dwViewType;
}

// offsets for first and other columns
#define OFFSET_FIRST	2
#define OFFSET_OTHER	6

void CUIODListCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC* pDC=CDC::FromHandle(lpDrawItemStruct->hDC);
	CRect rcItem(lpDrawItemStruct->rcItem);
	UINT uiFlags=ILD_TRANSPARENT;
	CImageList* pImageList;
	int nItem=lpDrawItemStruct->itemID;
	BOOL bFocus=(GetFocus()==this);
	COLORREF clrTextSave, clrBkSave;
	COLORREF clrImage=m_clrBkgnd;
	static _TCHAR szBuff[MAX_PATH];
	LPCTSTR pszText;

	if (m_cxClient == 0)
	{
		CRect rcClient;
		GetClientRect(rcClient);
		m_cxClient = rcClient.Width();
	}
// get item data

	LV_ITEM lvi;
	lvi.mask=LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
	lvi.iItem=nItem;
	lvi.iSubItem=0;
	lvi.pszText=szBuff;
	lvi.cchTextMax=sizeof(szBuff);
	lvi.stateMask=0xFFFF;		// get all state flags
	GetItem(&lvi);

	BOOL bSelected=(bFocus || (GetStyle() & LVS_SHOWSELALWAYS)) && lvi.state & LVIS_SELECTED;
	bSelected=bSelected || (lvi.state & LVIS_DROPHILITED);

// set colors if item is selected

	CRect rcAllLabels;
	GetItemRect(nItem,rcAllLabels,LVIR_BOUNDS);
	CRect rcLabel;
	GetItemRect(nItem,rcLabel,LVIR_LABEL);
	rcAllLabels.left=rcLabel.left;
//	if(m_bClientWidthSel && rcAllLabels.right<m_cxClient)
//		rcAllLabels.right=m_cxClient;
	int	BkMode = pDC->SetBkMode(TRANSPARENT);
	CUIListCtrlData *pData = (CUIListCtrlData*)lvi.lParam;
	ASSERT(pData->IsKindOf(RUNTIME_CLASS(CUIListCtrlData)));
	CFont *pOldFont = NULL;
	if (pData && pData->IsFontSet())
	{
		pOldFont = pDC->SelectObject((CFont*)pData->GetFont());
	}
	if(bSelected)
	{
		if (bFocus) 
		{
			clrTextSave=pDC->SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
			clrBkSave=pDC->SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));
			pDC->FillRect(rcAllLabels,&CBrush(::GetSysColor(COLOR_HIGHLIGHT)));
		}
		else 
		{
			clrTextSave=pDC->GetTextColor();
			clrBkSave=pDC->GetBkColor();
			DWORD clrTextBk;
			if (pData)
			{
				pDC->SetTextColor(pData->GetTextColor());
				clrTextBk = pData->GetBkColor();
			}
			else 
			{
				clrTextBk = m_clrTextBk;
				pDC->SetTextColor(m_clrText);
			}
			CRect rc(rcAllLabels);
			pDC->FillRect(rcAllLabels,&CBrush(clrTextBk));
			pDC->FrameRect(rc,&CBrush(::GetSysColor(COLOR_HIGHLIGHT)));
		}
	}
	else 
	{
		CUIListCtrlData *pData = (CUIListCtrlData *)lvi.lParam;
		if (pData) 
		{
			ASSERT(pData->IsKindOf(RUNTIME_CLASS(CUIListCtrlData)));
			pDC->SetTextColor(pData->GetTextColor());
			pDC->SetBkColor(pData->GetBkColor());
			pDC->FillRect(rcAllLabels,&CBrush(pData->GetBkColor()));
		}
		else
			pDC->FillRect(rcAllLabels,&CBrush(m_clrTextBk));
	}
// set color and mask for the icon

	if(lvi.state & LVIS_CUT)
	{
		clrImage=m_clrBkgnd;
		uiFlags|=ILD_BLEND50;
	}
	else if(bSelected)
	{
		clrImage=::GetSysColor(COLOR_HIGHLIGHT);
		uiFlags|=ILD_BLEND50;
	}

// draw state icon

	UINT nStateImageMask=lvi.state & LVIS_STATEIMAGEMASK;
	if(nStateImageMask)
	{
		int nImage=(nStateImageMask>>12)-1;
		pImageList=GetImageList(LVSIL_STATE);
		if(pImageList)
			pImageList->Draw(pDC,nImage,CPoint(rcItem.left,rcItem.top),ILD_TRANSPARENT);
	}

// draw normal and overlay icon

	CRect rcIcon;
	GetItemRect(nItem,rcIcon,LVIR_ICON);

	pImageList=GetImageList(LVSIL_SMALL);
	if(pImageList)
	{
		UINT nOvlImageMask=lvi.state & LVIS_OVERLAYMASK;
		if(rcItem.left<rcItem.right-1)
			ImageList_DrawEx(pImageList->m_hImageList,lvi.iImage,pDC->m_hDC,rcIcon.left,rcIcon.top,16,16,m_clrBkgnd,clrImage,uiFlags | nOvlImageMask);
	}

// draw item label

	GetItemRect(nItem,rcItem,LVIR_LABEL);
	rcItem.right-=m_cxStateImageOffset;

	pszText=MakeShortString(pDC,szBuff,rcItem.right-rcItem.left,2*OFFSET_FIRST);

	rcLabel=rcItem;
	rcLabel.left+=OFFSET_FIRST;
	rcLabel.right-=OFFSET_FIRST;

	pDC->DrawText(pszText,-1,rcLabel,DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER);
// draw labels for extra columns

	LV_COLUMN lvc;
	lvc.mask=LVCF_FMT | LVCF_WIDTH;

	for(int nColumn=1; GetColumn(nColumn,&lvc); nColumn++)
	{
		rcItem.left=rcItem.right;
		rcItem.right+=lvc.cx;

		int nRetLen=GetItemText(nItem,nColumn,szBuff,sizeof(szBuff));
		if(nRetLen==0) continue;

		pszText=MakeShortString(pDC,szBuff,rcItem.right-rcItem.left,2*OFFSET_OTHER);

		UINT nJustify=DT_LEFT;

		if(pszText==szBuff)
		{
			switch(lvc.fmt & LVCFMT_JUSTIFYMASK)
			{
			case LVCFMT_RIGHT:
				nJustify=DT_RIGHT;
				break;
			case LVCFMT_CENTER:
				nJustify=DT_CENTER;
				break;
			default:
				break;
			}
		}

		rcLabel=rcItem;
		rcLabel.left+=OFFSET_OTHER;
		rcLabel.right-=OFFSET_OTHER;

		pDC->DrawText(pszText,-1,rcLabel,nJustify | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER);
	}
	if (pData->IsKindOf(RUNTIME_CLASS(CUIStrListCtrlData)))
	{
		CUIStrListCtrlData *pStrData = (CUIStrListCtrlData*)pData;
		if (!pStrData->GetExtraString().IsEmpty())
		{
			rcLabel=rcAllLabels;
			rcLabel.top += (m_cySmallIcon+1);
			pDC->DrawText(pStrData->GetExtraString(),-1,rcLabel,DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER);
		}
	}
// draw focus rectangle if item has focus

	if(lvi.state & LVIS_FOCUSED && bFocus)
		pDC->DrawFocusRect(rcAllLabels);

// set original colors if item was selected

	if(bSelected)
	{
	    pDC->SetTextColor(clrTextSave);
		pDC->SetBkColor(clrBkSave);
	}
	if (pOldFont)
		pDC->SelectObject(pOldFont);
	pDC->SetBkMode(BkMode);
}

LPCTSTR CUIODListCtrl::MakeShortString(CDC* pDC, LPCTSTR lpszLong, int nColumnLen, int nOffset)
{
	static const _TCHAR szThreeDots[]=_T("...");

	int nStringLen=lstrlen(lpszLong);

	if(nStringLen==0 || pDC->GetTextExtent(lpszLong,nStringLen).cx+nOffset<=nColumnLen)
		return(lpszLong);

	static _TCHAR szShort[MAX_PATH];

	lstrcpy(szShort,lpszLong);
	int nAddLen=pDC->GetTextExtent(szThreeDots,sizeof(szThreeDots)).cx;

	for(int i=nStringLen-1; i>0; i--)
	{
		szShort[i]=0;
		if(pDC->GetTextExtent(szShort,i).cx+nOffset+nAddLen<=nColumnLen)
			break;
	}

	lstrcat(szShort,szThreeDots);

	return(szShort);
}

void CUIODListCtrl::RepaintSelectedItems()
{
	CRect rcItem, rcLabel;

// invalidate focused item so it can repaint properly

	int nItem=GetNextItem(-1,LVNI_FOCUSED);

	if(nItem!=-1)
	{
		GetItemRect(nItem,rcItem,LVIR_BOUNDS);
		GetItemRect(nItem,rcLabel,LVIR_LABEL);
		rcItem.left=rcLabel.left;

		InvalidateRect(rcItem,FALSE);
	}

// if selected items should not be preserved, invalidate them

//	if(!(GetStyle() & LVS_SHOWSELALWAYS))
//	{
		for(nItem=GetNextItem(-1,LVNI_SELECTED);
			nItem!=-1; nItem=GetNextItem(nItem,LVNI_SELECTED))
		{
			GetItemRect(nItem,rcItem,LVIR_BOUNDS);
			GetItemRect(nItem,rcLabel,LVIR_LABEL);
			rcItem.left=rcLabel.left;

			InvalidateRect(rcItem,FALSE);
		}
//	}

// update changes 

	UpdateWindow();
}

void CUIODListCtrl::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItem)
{
   	lpMeasureItem->itemHeight = (m_cySmallIcon+1)*3;
}


BOOL CUIODListCtrl::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	LPNMHDR pnmh;

	switch (message)
	{
			case WM_DRAWITEM:
				ASSERT(pResult == NULL);       
				DrawItem((LPDRAWITEMSTRUCT)lParam);
				break;
			case WM_MEASUREITEM:
				ASSERT(pResult == NULL);       
				MeasureItem((LPMEASUREITEMSTRUCT)lParam);
				break;
			case WM_NOTIFY:
				pnmh = (LPNMHDR)lParam;
				if (pnmh->code == LVN_ITEMCHANGED)
				{
					NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pnmh;
					if ((pNMListView->uNewState & LVIS_SELECTED) == LVIS_SELECTED && 
						(pNMListView->uNewState & LVIS_FOCUSED) == LVIS_FOCUSED) 
					{
						return OnSelChanged(pNMListView->iItem,pResult);
					}	
				}
				else if (pnmh->code == LVN_GETDISPINFO) 
				{
					return GetDispInfo((LV_DISPINFO*)pnmh);
				}
				else if (pnmh->code == LVN_COLUMNCLICK) 
				{
					return ColumnClick((NM_LISTVIEW*)pnmh);
				}
				else if (pnmh->code == LVN_ENDLABELEDIT) 
				{
					return OnEndLabelEdit(pnmh,pResult);
				}
				else if (pnmh->code == NM_DBLCLK) 
				{
					return DoubleClick((NM_LISTVIEW*)pnmh);
				}
				else if (pnmh->code == NM_RETURN) 
				{
					return OnEnter((NM_LISTVIEW*)pnmh);
				}
				else if (pnmh->code == NM_CUSTOMDRAW)
				{
				     LPNMLVCUSTOMDRAW  lplvcd = (LPNMLVCUSTOMDRAW)lParam;
					 if(lplvcd->nmcd.dwDrawStage == CDDS_PREPAINT)
					 {
						//tell the control we want pre-paint notifications for each item
						*pResult = CDRF_NOTIFYITEMDRAW;
						return TRUE;
					 }
					 if (lplvcd->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM))
					 {
						 return SubItemPrePaint(lplvcd,pResult);
					 }
					 if (lplvcd->nmcd.dwDrawStage == (CDDS_ITEMPOSTPAINT | CDDS_SUBITEM))
					 {
						 return SubItemPostPaint(lplvcd,pResult);
					 }
					 if (lplvcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
					 {
						 return ItemPrePaint(lplvcd,pResult);
					 }
					 if (lplvcd->nmcd.dwDrawStage == CDDS_ITEMPOSTPAINT)
					 {
						 return ItemPostPaint(lplvcd,pResult);
					 }
       			}
	}
	return CWnd::OnChildNotify(message, wParam, lParam, pResult);
}

LRESULT CUIODListCtrl::OnUpdateHeaderWidth(WPARAM wParam,LPARAM lParam)
{
	// lParam = column that has changed its width
	int nItem = lParam;
	m_pColWidths[nItem] = GetColumnWidth(nItem);
    GetParent()->SendMessage(WM_HEADERWIDTHCHANGED,(WPARAM)GetSafeHwnd(),nItem);
	return TRUE;
}

BOOL CUIODListCtrl::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	LPNMHDR pnmh = (LPNMHDR)lParam;
	if ((pnmh->code == HDN_BEGINTRACKW || pnmh->code == HDN_BEGINTRACKA)) 
	{
		if (m_bColumnSizing == FALSE) 
		{
			*pResult = TRUE;
			return TRUE;
		}
		LPNMHEADER phdr = (LPNMHEADER)lParam;
		if (phdr->pitem->mask & HDI_WIDTH)
		{
		}
	}
	if ((pnmh->code == HDN_ITEMCHANGINGW || pnmh->code == HDN_ITEMCHANGINGA)) 
	{
		LPNMHEADER phdr = (LPNMHEADER)lParam;
	}
	if ((pnmh->code == HDN_ENDTRACKW || pnmh->code == HDN_ENDTRACKA)) 
	{
	    HD_NOTIFY *phdn = (HD_NOTIFY*)pnmh;
		LPNMHEADER phdr = (LPNMHEADER)lParam;
		if (m_bColumnSizing == TRUE && (phdr->pitem->mask & HDI_WIDTH))
			PostMessage(WM_UPDATEHEADERWIDTH,(WPARAM)phdn->hdr.hwndFrom,(LPARAM)phdn->iItem);
	} 
	// wParam is zero for Header ctrl
	if( wParam == 0 && pnmh->code == NM_RCLICK )
	{
		// Right button was clicked on header
		CPoint ptScreen(GetMessagePos());
		CHeaderCtrl* pHeader = (CHeaderCtrl*)GetDlgItem(0);
		ASSERT(pHeader);
		CPoint ptClient(ptScreen);
		pHeader->ScreenToClient(&ptClient);
		
		// Determine the column index
		int index=-1;
		CRect rcCol;
		for( int i=0; Header_GetItemRect(pHeader->m_hWnd, i, &rcCol); i++ )
		{
			if( rcCol.PtInRect( ptClient ) )
			{
				index = i;
				break;
			}
		}
		OnRClickHeader(ptScreen,index);
	}
	return CWnd::OnNotify(wParam,lParam,pResult);
}

DWORD CUIODListCtrl::GetItemData(int nIndex) const
{
	if (nIndex != LB_ERR)
	{
		const CUIListCtrlData* pListObj = GetListCtrlData(nIndex);
		if (pListObj)
			return pListObj->GetExtData();
	}
	return 0;
}

BOOL CUIODListCtrl::SetItemDataAutoDelete(int nIndex,CObject *pObj)
{
	ASSERT_KINDOF(CObject,pObj);
	return SetItemData(nIndex,(DWORD)pObj,true);
}

BOOL CUIODListCtrl::SetItemData(int nIndex,DWORD dwData,bool bAutoDelete)
{
	if (nIndex != LB_ERR) 
	{
		CUIListCtrlData* pListObj = GetListCtrlData(nIndex);
		if (pListObj) 
		{
			pListObj->SetExtData(dwData);
			pListObj->SetAutoDelete(bAutoDelete);
			return TRUE;
		}
	}
	return FALSE;
}

int CUIODListCtrl::SetIcon(int nRow,int nImage)
{
	LV_ITEM lvItem;
	ZeroMemory(&lvItem,sizeof(LV_ITEM));
	lvItem.mask = LVIF_IMAGE;
	lvItem.iItem = nRow;
	lvItem.iImage = nImage;
	SetItem(&lvItem);
	return lvItem.iImage;
}

int CUIODListCtrl::SetIcon(int nRow,UINT nIconID)
{
	int nImage = GetImageIndex(nIconID);
	return SetIcon(nRow,nImage);
}

BOOL CUIODListCtrl::SetColumnFormat(int nCol,int fmt)
{
	LVCOLUMN lvCol;
	ZeroMemory(&lvCol,sizeof(lvCol));
	lvCol.mask = LVCF_FMT;
	lvCol.fmt = fmt;
	return SetColumn(nCol,&lvCol);
}

void CUIODListCtrl::SetTextColor(int nRow, int nCol, COLORREF TextColor)
{
	CUIListCtrlData *pData = GetListCtrlData(nRow);
	if (pData)
		pData->SetTextColor(TextColor,nCol);
}

void CUIODListCtrl::SetDefaultTextColor(int nRow,int nCol)
{
	CUIListCtrlData *pData = GetListCtrlData(nRow);
	if (pData)
		pData->SetDefaultTextColor(nCol);
}

void CUIODListCtrl::SetBkColor(int nRow,int nCol,COLORREF BkColor)
{
	CUIListCtrlData *pData = GetListCtrlData(nRow);
	if (pData)
		pData->SetBkColor(BkColor,nCol);
}

void CUIODListCtrl::SetDefaultBkColor(int nRow,int nCol)
{
	CUIListCtrlData *pData = GetListCtrlData(nRow);
	if (pData)
		pData->SetDefaultBkColor(nCol);
}

CUIListCtrlData *CUIODListCtrl::GetListCtrlData(int nItem) const
{
	CUIListCtrlData *pData = (CUIListCtrlData *)CListCtrl::GetItemData(nItem);
	if (pData == NULL)
		return NULL;
	ASSERT(pData->IsKindOf(RUNTIME_CLASS(CUIListCtrlData)));
	return pData;
}

int CUIODListCtrl::AddColumn(LPCTSTR szText)
{
	// don't add again
	int nCol = FindColumn(szText);
	if (nCol >= 0)
		return nCol;
	nCol = InsertColumn(m_nSubItems,szText);
	if (nCol != -1)
	{
		m_strHeadings += COLUMN_DELIMITER_STR;
		m_strHeadings += szText;
		m_nSubItems++;
		SaveProfile();
		LoadProfile();
	}
	return nCol;
}

int CUIODListCtrl::FindColumn(LPCTSTR pszText)
{
	LPCTSTR pszHeadings = m_strHeadings;
	int nCol=0;
	while (*pszHeadings != '\0')
	{
		if (*pszHeadings == COLUMN_DELIMITER)
			nCol++;
		if (_tcsnicmp(pszHeadings,pszText,_tcslen(pszText)) == 0)
		{
			break;			
		}
		pszHeadings = _tcsinc(pszHeadings);
	}
	return *pszHeadings == '\0' ? -1 : nCol;
}

BOOL CUIODListCtrl::InitListCtrl(UINT IconID,LPCTSTR szHeadings,LPCTSTR pszProfile)
{
	static UINT Icons[2];

	Icons[0] = IconID;
	Icons[1] = 0;
	return InitListCtrl(&Icons[0],szHeadings,pszProfile);
}

BOOL CUIODListCtrl::InitListCtrl(LPCTSTR szHeadings,LPCTSTR pszProfile)
{
	static UINT Icons[1];

	Icons[0] = 0;
	return InitListCtrl(&Icons[0],szHeadings,pszProfile);
}

int CUIODListCtrl::InitListCtrl(const UINT *pIconIDs,LPCTSTR szHeadings,LPCTSTR pszProfile)
{
	BOOL bFailed = FALSE;
	// TODO: Add extra initialization here
	if (pszProfile)
	{
		SetSection(pszProfile);
	}
	//clean up
	if (m_nSubItems) 
	{
		if (m_strHeadings == szHeadings)
			return m_nSubItems;
		DeleteAllItems();
		int nSubItems=m_nSubItems;
		for(int i=0;i < nSubItems;i++)
			DeleteColumn(0);
		m_nSubItems = 0;
		Empty();
	}
	if (pszProfile == NULL)
	{
		if (!m_strSection.IsEmpty())
			pszProfile = m_strSection;
	}
	if (pszProfile)
	{
		LoadProfile();
		// Use profile headings if found
		if (!m_strHeadings.IsEmpty())
		{
			if (m_strHeadings != szHeadings)
			{
				m_strHeadings = szHeadings;
				SaveProfile();
			}
		}
	}
	if (m_dwExStyle)
		SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, m_dwExStyle);

	// at least one image
	if (pIconIDs && *pIconIDs) 
	{
		// load the icons and add them to the image lists
		int i;
		for(i=0;*pIconIDs;i++,pIconIDs++) 
		{
			if (AddIcon(*pIconIDs) == -1)
				bFailed = TRUE;
		} 
	}

	// Now initialize the columns we will need
	// Initialize the LV_COLUMN structure
	// the mask specifies that the .fmt, .ex, width, and .subitem members 
	// of the structure are valid,
	LV_COLUMN lvC;
	lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvC.fmt = LVCFMT_LEFT;  // left align the column

	if (szHeadings)
	{
		// parse the string, format it and get column count
		const int MAX_HEADING_LEN = 512;
		TCHAR szHeadingsCopy[MAX_HEADING_LEN]; 
		if (_tcslen(szHeadings) >= MAX_HEADING_LEN-1)
		{
			_tcsncpy(szHeadingsCopy,szHeadings,MAX_HEADING_LEN-1);
			szHeadingsCopy[MAX_HEADING_LEN-1] = '\0';
		}
		else
			_tcscpy(szHeadingsCopy,szHeadings);
		LPTSTR p = szHeadingsCopy;
		int nCols = 0;
		while (*p != '\0')
		{
				if (*p == COLUMN_DELIMITER) 
				{
					 nCols++;
					 *p = '\0';
				}
				p = _tcsinc(p);
		}
		if (*szHeadingsCopy)		
			nCols++;
		LPTSTR pszText = szHeadingsCopy;
		m_nSubItems = nCols; 
		for (int index = 0;index < nCols; index++)
		{
			lvC.iSubItem = index;
			lvC.pszText = pszText;
//			lvC.cx = LVSCW_AUTOSIZE_USEHEADER;  
			lvC.cx = 100;  
			if (InsertColumn(index, &lvC) == -1) 
			{
				TRACE(_T("InsertColumn failed\n"));
				m_nSubItems--;
				bFailed = TRUE;
			}
			while (*pszText)
				   pszText = _tcsinc(pszText);
			pszText = _tcsinc(pszText);	// Runs past the end of the buffer
		}
		// Add the columns.
		m_strHeadings = szHeadings;
		if (pszProfile)
		{
			SaveProfile();
			LoadProfile();
		}
		if (m_pColOrder)
		{
			SendMessage(LVM_SETCOLUMNORDERARRAY,(WPARAM)nCols,(LPARAM)m_pColOrder);
		}
		if (m_pColWidths)
		{
			for(int i=0;i < nCols;i++)
			{
				if (m_pColWidths[i] != 0)
					SetColumnWidth(i,m_pColWidths[i]);
			}
		}
		else
		{
			m_pColWidths = new int[nCols];
			for(int i=0;i < nCols;i++)
			{
				m_pColWidths[i] = GetColumnWidth(i);
			}
		}
		if (m_pColTypes == NULL)
		{
			// set default column types
			m_pColTypes = new int[nCols];
			for(int i=0;i < nCols;i++)
			{
				m_pColTypes[i] = e_Text;
			}			
		}
	}
	SetSortHeader();
	return m_nSubItems;
}

void CUIODListCtrl::GetImageLists(CImageList **pILSmall,CImageList **pILLarge)
{
	*pILSmall = GetImageList(LVSIL_SMALL);
	if (*pILSmall == NULL)
	{
		CreateImageLists();
		*pILSmall = GetImageList(LVSIL_SMALL);
	}
	*pILLarge = GetImageList(LVSIL_NORMAL);
	ASSERT(*pILSmall);
	ASSERT(*pILLarge);
}

void CUIODListCtrl::CreateImageLists()
{
	m_cxSmallIcon = ::GetSystemMetrics(SM_CXSMICON);
	m_cySmallIcon = ::GetSystemMetrics(SM_CYSMICON);
	m_cxLargeIcon = ::GetSystemMetrics(SM_CXICON);
	m_cyLargeIcon = ::GetSystemMetrics(SM_CYICON);
	if (m_ImageSmall.GetSafeHandle() != NULL)
		m_ImageSmall.DeleteImageList();	
	if (m_ImageLarge.GetSafeHandle() != NULL)
		m_ImageLarge.DeleteImageList();	
	// create the small icon image list	
	m_ImageSmall.Create(m_cxSmallIcon,
						m_cySmallIcon,
						ILC_MASK | ILC_COLOR16,	
						1,
						1);	
	// create the large icon image list	
	m_ImageLarge.Create(m_cxLargeIcon,
						m_cyLargeIcon,
						ILC_MASK | ILC_COLOR16,	
						1,
						1);	
	// Associate the image lists with the list view
	SetImageList(&m_ImageSmall, LVSIL_SMALL);
	SetImageList(&m_ImageLarge, LVSIL_NORMAL);
}

void CUIODListCtrl::AddExtraString(int nRow,LPCTSTR pszExtraText)
{
	CUIStrListCtrlData *pData = (CUIStrListCtrlData *)GetListCtrlData(nRow);
	ASSERT_KINDOF(CUIStrListCtrlData,pData);
	pData->AddExtraString(pszExtraText);	
}

void CUIODListCtrl::ConvertToTime(CString &str)
{
	int nPoint = str.Find(GetDecimalSep());
	int nSecs=0;
	if (nPoint == -1)
		nSecs = _ttoi(str);
	else
	{
		LPTSTR pszStop;
		double dSecs = _tcstod(str,&pszStop);
		nSecs = dSecs;
	}
	if (nSecs == 0)
		return;
	int nMins = nSecs / 60;
	nSecs = nSecs % 60;
	str.Format(_T("%u:%u"),nMins,nSecs);
}

void CUIODListCtrl::AddThousandSeps(CString &str)
{
	CString strTemp;
	CString strExp;
	CString strNeg;
	int nDouble = str.Find(GetDecimalSep());
	if (nDouble >= 0)
	{
		strTemp = str.Left(nDouble);
		strExp = str.Right(str.GetLength()-nDouble);
	}
	else
	{
		if (str.Find(GetNegativeSign()) == 0)
		{
			strNeg = str.Left(1);
			strTemp = str.Right(str.GetLength()-1);
		}
		else
			strTemp = str;
	}
	int len = strTemp.GetLength();
	if (len <= 3 || len > nMaxDigLen)
		return;
	LPCTSTR pBuf = strTemp;
	pBuf += (len-1);
	TCHAR tempbuf[nMaxDigLen+1];
	LPTSTR pTempBuf = tempbuf;
	LPCTSTR pSep = GetThousandSep();
	for(int i=0;i < len;i++)
	{
		if (i && ((i % 3) == 0)) 
		{
			if (*pTempBuf != *pSep)
			{
				*pTempBuf =  *pSep;
				pTempBuf = _tcsinc(pTempBuf);
			}
		}
		*pTempBuf = *pBuf;
		pTempBuf = _tcsinc(pTempBuf);
		pBuf = _tcsdec(strTemp,pBuf);
	}
	*pTempBuf = '\0';
	_tcsrev(tempbuf);
	str = strNeg;
	str += tempbuf;
	str += strExp;
}

BOOL CUIODListCtrl::AddString(int nRow,int nCol,int nValue,CUIODListCtrl::eColTypes type)
{
	CString sValue;
	sValue.Format(_T("%d"),nValue);
	return AddString(nRow,nCol,sValue,type);
}

BOOL CUIODListCtrl::AddString(int nRow,int nCol,double dValue,CUIODListCtrl::eColTypes type)
{
	char szBuf[nMaxDigLen];
	_gcvt(dValue,_MAX_INT_DIG,szBuf);
#ifdef _UNICODE
USES_CONVERSION;	
	return AddString(nRow,nCol,A2W(szBuf),type);
#else	
	return AddString(nRow,nCol,szBuf,type);
#endif	
}

BOOL CUIODListCtrl::AddString(int nRow,int nCol,COleDateTime &dtValue,CUIODListCtrl::eColTypes type)
{
	return AddString(nRow,nCol,dtValue.Format(),type);
}

BOOL CUIODListCtrl::AddString(int nRow,int nCol,LPCTSTR szItem,CUIODListCtrl::eColTypes type)
{
	if (m_nItems == 0) 
	{
		TRACE(_T("No rows defined in ODListCtrl\n")); 
		return FALSE;
	}
	if (nCol >= m_nSubItems) 
	{
		TRACE(_T("Tried to add invalid column number(%d) in ODListCtrl\n"),nCol); 
		return FALSE;
	}
	SetColType(nCol,type);
//	return SetItemText(nRow,nCol/*+m_nImage*/,szItem);
	CUIStrListCtrlData *pData = (CUIStrListCtrlData *)GetListCtrlData(nRow);
	ASSERT_KINDOF(CUIStrListCtrlData,pData);
	BOOL ret=FALSE;
	if (type == e_NumericFormatComma || type == e_DoubleFormatComma)
	{
		CString sBuf(szItem);
		AddThousandSeps(sBuf);
		ret = pData->AddString(nCol,sBuf);
	}
	else if (type == e_NumericFormatTime || type == e_DoubleFormatTime)
	{
		CString sBuf(szItem);
		ConvertToTime(sBuf);
		ret = pData->AddString(nCol,sBuf);
	}
	else
		ret = pData->AddString(nCol,szItem);
	return ret;
}						  

void CUIODListCtrl::UpdateString(int nRow)
{ 
	CRect rect;
	if (GetItemRect(nRow,&rect,LVIR_BOUNDS))
	{
		RedrawItems(nRow,nRow);
		UpdateWindow();
	}
}

int CUIODListCtrl::AddCallBackItem(DWORD dwData,int nImage)
{
	if (m_nSubItems == 0) 
	{
		TRACE(_T("No columns defined in ODListCtrl\n")); 
		return FALSE;
	}
	CUIListCtrlData *pListCtrlData = GetNewListCtrlData(dwData,m_nItems);
	BOOL ret = InsertItem(LPSTR_TEXTCALLBACK,(LPARAM)pListCtrlData,nImage);
	if (ret)
		m_objList.Append(pListCtrlData);
	else
		delete pListCtrlData;
	return ret ? m_nItems-1 : -1;
}

int CUIODListCtrl::AddTextItem(int nImage)
{
	CUIStrListCtrlData *pListCtrlData = new CUIStrListCtrlData(m_nSubItems);
	if (InsertItem(LPSTR_TEXTCALLBACK,(LPARAM)pListCtrlData,nImage))
	{
		m_objList.Append(pListCtrlData);
		return m_nItems-1;
	}
	delete pListCtrlData;
	return -1;
}

int CUIODListCtrl::GetImageIndex(UINT nIconID)
{
	int nImage = 0;
	if (nIconID)
	{
		if (!m_mapImageIndex.Lookup(nIconID,nImage))
		{
			nImage = AddIcon(nIconID);
		}
	}
	return nImage;
}

// from handle
int CUIODListCtrl::AddIcon(HICON hIcon,UINT nIconID)
{
	CImageList *pILSmall = NULL;
	CImageList *pILLarge = NULL;
	GetImageLists(&pILSmall,&pILLarge);
	int nSmallIndex = pILSmall->Add(hIcon);
	// save the image ID
	if (nIconID)
		m_mapImageIndex[nIconID] = nSmallIndex;
	int nLargeIndex = pILLarge->Add(hIcon); 
	if (nSmallIndex == -1 || nLargeIndex == -1)
	{
		TRACE(_T("Failed to add icon to image list in CUIODListCtrl\n"));
		return -1;
	}
	return nSmallIndex;
}

// from resource id
int CUIODListCtrl::AddIcon(UINT nIconID)
{
	HICON hIcon = AfxGetApp()->LoadIcon(nIconID);
	if (hIcon == NULL)
	{
		TRACE(_T("LoadIcon failed in CUIODListCtrl\n"));
		return -1;
	}
	return AddIcon(hIcon,nIconID);
}

// from file
int CUIODListCtrl::AddIcon(LPCTSTR pszIcon)
{
	CString sImageFile;
	int nIndex;
	if (m_mapImageFile.Lookup(sImageFile,nIndex))
	{
		return nIndex;
	}
    HICON hIcon = (HICON)LoadImage(AfxGetInstanceHandle(), 
            pszIcon,
            IMAGE_ICON,
            ::GetSystemMetrics(SM_CXICON),
            ::GetSystemMetrics(SM_CYICON),
            LR_LOADFROMFILE);
	if (hIcon == NULL)
	{
		TRACE(_T("LoadImage(Large) failed in CUIODListCtrl\n"));
	}
    HICON hIconSm = (HICON)LoadImage(AfxGetInstanceHandle(), 
            pszIcon,
            IMAGE_ICON,
            ::GetSystemMetrics(SM_CXSMICON),
            ::GetSystemMetrics(SM_CYSMICON),
            LR_LOADFROMFILE);
	if (hIconSm == NULL)
	{
		TRACE(_T("LoadImage(Small) failed in CUIODListCtrl\n"));
	}
	if (hIcon == NULL && hIconSm == NULL)
		return -1;
	CImageList *pILSmall = NULL;
	CImageList *pILLarge = NULL;
	GetImageLists(&pILSmall,&pILLarge);
	int nSmallIndex = pILSmall->Add(hIconSm);
	int nLargeIndex = pILLarge->Add(hIcon); 
	if (nSmallIndex == -1 || nLargeIndex == -1)
	{
		TRACE(_T("Failed to add icon to image list in CUIODListCtrl\n"));
		return -1;
	}
	sImageFile = pszIcon;
	m_mapImageFile[sImageFile] = nSmallIndex;
	return nSmallIndex;
}

BOOL CUIODListCtrl::InsertItem(LPTSTR szItem,LPARAM lParam,int nImage)
{
	LV_ITEM lvi;
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
	lvi.state = 0;      
	lvi.stateMask = 0;  

	lvi.iItem = m_nItems;
	lvi.iSubItem = 0;
	lvi.pszText = szItem; 
	lvi.cchTextMax = MAX_TEXT;
	lvi.iImage = nImage;
	lvi.lParam = lParam;
	if (CListCtrl::InsertItem(&lvi) == -1) 
	{
		TRACE1("Unable to insert item %u into list control\n",m_nItems);
		return FALSE;
	}
	if (nImage) 
		m_nImage = 1;
	m_nItems++;
	return TRUE;
}

int CUIODListCtrl::SetCurSel(int nSelect)
{
	if (GetItemCount() && SetItemState(nSelect,LVIS_SELECTED | LVIS_FOCUSED,LVIS_SELECTED | LVIS_FOCUSED))
		return TRUE;
	return -1;
}

// messages
LRESULT CUIODListCtrl::OnSetColumnWidth(WPARAM wParam,LPARAM lParam)
{
	Default();
	return TRUE;
}

LRESULT CUIODListCtrl::OnDeleteAllItems(WPARAM wParam,LPARAM lParam)
{
	m_nItems=0;
	m_nImage=0;
	m_hOrigFont=NULL;
	AllItemsDeleted();
	m_objList.DeleteAll();
	TRACE0("All items deleted in CUIODListCtrl\n");
	Default();
	return TRUE;
}

CUIListCtrlData *CUIODListCtrl::GetNewListCtrlData(DWORD dwData,int nItem)
{
	CUIListCtrlData *pListCtrlData = new CUIListCtrlData(m_nSubItems);
	pListCtrlData->SetExtData(dwData); 
	return pListCtrlData;
}

int CUIODListCtrl::FindItem(DWORD dwExtData)
{
	int count = GetItemCount();
	CUIListCtrlData *pData;
	for(int i=0;i <	count;i++) 
	{
		pData = GetListCtrlData(i);
		if (pData->GetExtData() == dwExtData)
			break;
	}
	return i == count ? -1 : i;
}

BOOL CUIODListCtrl::GetFullRowSel() const
{
	return(m_bFullRowSel);
}

DWORD CUIODListCtrl::GetExStyle() 
{
	return SendMessage(LVM_GETEXTENDEDLISTVIEWSTYLE);
}

BOOL CUIODListCtrl::SetExStyle(UINT nStyle,BOOL bModify)
{
	DWORD dwStyle = GetExStyle();
	if (bModify)
		dwStyle |= nStyle;
	else
		dwStyle &= ~nStyle;
	BOOL bRet = SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle);
	if (bRet)
		SaveProfile();
	return bRet;
}

void CUIODListCtrl::SetFullRowSel(bool bSet)
{
	// no painting during change
	if ((bSet && m_bFullRowSel==TRUE) || (!bSet && m_bFullRowSel==FALSE))
		return;
	m_bFullRowSel=bSet;
	if(!(GetStyle() & LVS_OWNERDRAWFIXED)) 
	{
		SetExStyle(LVS_EX_FULLROWSELECT,m_bFullRowSel);
	}
	RedrawItems(GetCurSel(),GetCurSel());
	SaveProfile();
}

void CUIODListCtrl::SetIconSize(int nImageList)
{
	m_nImageList = nImageList;

	CRect rc;
	GetWindowRect(rc);
	GetParent()->ScreenToClient(rc);
	rc.bottom++; // Force a WM_MEASUREITEM message
	MoveWindow(rc,FALSE);
	rc.bottom--; // We really don't want to resize it
	MoveWindow(rc,TRUE);
}

void CUIODListCtrl::ChangeIconSize()
{
	SetCurSel(0);
	EnsureVisible(0,0);
	if (m_nImageList == LVSIL_NORMAL)
		SetIconSize(LVSIL_SMALL);
	else
		SetIconSize(LVSIL_NORMAL);
}

BOOL CUIODListCtrl::GetDispInfo(LV_DISPINFO *pDispInfo)
{
	CUIStrListCtrlData *pData = (CUIStrListCtrlData*)pDispInfo->item.lParam;
	ASSERT_KINDOF(CUIStrListCtrlData,pData);
	if (!pData->IsKindOf(RUNTIME_CLASS(CUIStrListCtrlData)))
		return FALSE;
	pDispInfo->item.pszText = pData->GetString(pDispInfo->item.iSubItem);
	return TRUE;
}

BOOL CUIODListCtrl::PopupMenuItem(int nItem,int nCol,CMenu *pPopup,CPoint point)
{
	int nCurSel = GetCurSel();
	int nSel = GetNextSel(nCurSel);
	UINT nPopupID = (nSel == -1 ? m_PopupID : m_MultiPopupID);
	if (nPopupID && (nCol == 0 && nItem != -1))
	{
		CMenu menu;
		VERIFY(menu.LoadMenu(nPopupID));
		CMenu* pMyPopup = menu.GetSubMenu(0);
		ASSERT(pMyPopup != NULL);
		if (m_pPopupWnd == NULL)
			m_pPopupWnd = this;
		pMyPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, m_pPopupWnd);
		return TRUE;
	}
	return nSel == -1 ? FALSE : TRUE;
}

int CUIODListCtrl::HitTestEx(CPoint &point, int *col) const
{
	int colnum = 0;
	int row = HitTest( point, NULL );
	
	if( col ) *col = 0;

	// Make sure that the ListView is in LVS_REPORT
	if( (GetWindowLong(m_hWnd, GWL_STYLE) & LVS_TYPEMASK) != LVS_REPORT)
		return row;

	// Get the top and bottom row visible
	row = GetTopIndex();
	int bottom = row + GetCountPerPage();
	if( bottom > GetItemCount() )
		bottom = GetItemCount();
	
	// Get the number of columns
	CHeaderCtrl* pHeader = (CHeaderCtrl*)GetDlgItem(0);
	int nColumnCount = pHeader->GetItemCount();
	CRect rcHeader;
	pHeader->GetClientRect(&rcHeader);
	if (point.y >= 0 && point.y <= rcHeader.Height())
		return -2;

	// Loop through the visible rows
	for( ;row <=bottom;row++)
	{
		// Get bounding rect of item and check whether point falls in it.
		CRect rect;
		GetItemRect( row, &rect, LVIR_BOUNDS );
		if( rect.PtInRect(point) )
		{
			// Now find the column
			for( colnum = 0; colnum < nColumnCount; colnum++ )
			{
				int colwidth = GetColumnWidth(colnum);
				if( point.x >= rect.left 
					&& point.x <= (rect.left + colwidth ) )
				{
					if( col ) *col = colnum;
					return row;
				}
				rect.left += colwidth;
			}
		}
	}
	return -1;
}

CEdit* CUIODListCtrl::EditSubLabel( int nItem, int nCol )
{
	// The returned pointer should not be saved
	if (m_bEditSubItems == false || (GetWindowLong(m_hWnd, GWL_STYLE) & LVS_EDITLABELS) != LVS_EDITLABELS)
	{
		return NULL;
	}

	// Make sure that the item is visible
	if( !EnsureVisible( nItem, TRUE ) ) 
		return NULL;

	// Make sure that nCol is valid
	CHeaderCtrl* pHeader = (CHeaderCtrl*)GetDlgItem(0);
	if (pHeader == NULL)
		return NULL;
	int nColumnCount = pHeader->GetItemCount();
	if( nCol >= nColumnCount || GetColumnWidth(nCol) < 5 )
		return NULL;

	// Get the column offset
	int offset = 0;
	for( int i = 0; i < nCol; i++ )
		offset += GetColumnWidth( i );

	CRect rect;
	GetItemRect( nItem, &rect, LVIR_BOUNDS );

	// Now scroll if we need to expose the column
	CRect rcClient;
	GetClientRect( &rcClient );
	if( offset + rect.left < 0 || offset + rect.left > rcClient.right )
	{
		CSize size;
		size.cx = offset + rect.left;
		size.cy = 0;
		Scroll( size );
		rect.left -= size.cx;
	}

	// Get Column alignment
	LV_COLUMN lvcol;
	lvcol.mask = LVCF_FMT;
	GetColumn( nCol, &lvcol );
	DWORD dwStyle ;
	if((lvcol.fmt&LVCFMT_JUSTIFYMASK) == LVCFMT_LEFT)
		dwStyle = ES_LEFT;
	else if((lvcol.fmt&LVCFMT_JUSTIFYMASK) == LVCFMT_RIGHT)
		dwStyle = ES_RIGHT;
	else dwStyle = ES_CENTER;

	rect.left += offset+4;
	rect.right = rect.left + GetColumnWidth( nCol ) - 3 ;
	if( rect.right > rcClient.right) rect.right = rcClient.right;

	dwStyle |= WS_BORDER|WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL;
	CEdit *pEdit = new CInPlaceEdit(nItem, nCol, GetItemText( nItem, nCol ));
	pEdit->Create( dwStyle, rect, this, -1/*IDC_IPEDIT*/ );

	return pEdit;
}

void CUIODListCtrl::DeleteProgressBar(int nRow,int nCol)
{
	CUIListCtrlData *pData = GetListCtrlData(nRow);
	if (pData)
	{
		pData->DestroyCtrl(nCol);
		RedrawItems(nRow,nRow);
	}
}

void CUIODListCtrl::UpdateProgressBar(int nRow,int nCol,int nPos)
{
	CUIListCtrlData *pData = GetListCtrlData(nRow);
	ASSERT(pData);
	if (pData == NULL)
		return;
	CTextProgressCtrl *pCtrl = (CTextProgressCtrl*)pData->GetCtrl(nCol);
	if (pCtrl)
	{
		pCtrl->SetPos(nPos);
		RedrawItems(nRow,nRow);
	}
}

CTextProgressCtrl *CUIODListCtrl::AddProgressBar(int nItem,int nCol,int nMin,int nMax)
{
	// The returned pointer should not be saved
	if (GetViewType() != LVS_REPORT)
		return NULL;
	CUIListCtrlData *pData = GetListCtrlData(nItem);
	if (pData == NULL)
		return NULL;
	CTextProgressCtrl *pCtrl = new CTextProgressCtrl;
	pData->SetCtrl(pCtrl,nCol);
	pCtrl->SetRange(nMin,nMax);
	return pCtrl;
}

//
// Message handlers
//
void CUIODListCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (GetFocus() != this) 
		SetFocus();
	CListCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CUIODListCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (GetFocus() != this) 
		SetFocus();
	CListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CUIODListCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	CListCtrl::OnLButtonDown(nFlags, point);
}

BOOL CUIODListCtrl::OnSelChanged(int nItem, LRESULT* pResult)
{
	*pResult = 0;
	return FALSE;
}

BOOL CUIODListCtrl::OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult)
{
	LV_DISPINFO *plvDispInfo = (LV_DISPINFO *)pNMHDR;
	LV_ITEM	*plvItem = &plvDispInfo->item;

	if (plvItem->pszText != NULL)
	{
		if (plvItem->iItem == -1)
		{
			CHeaderCtrl *pHeader = (CHeaderCtrl*)GetDlgItem(0);
			if (pHeader)
			{
				CString strText;
				GetHeaderText(pHeader,plvItem->iSubItem,strText);
				HD_ITEM hdItem;
				ZeroMemory(&hdItem,sizeof(hdItem));
				hdItem.mask = HDI_TEXT;
				hdItem.pszText = plvItem->pszText;
				hdItem.cchTextMax = lstrlen(plvItem->pszText);
				ReplaceString(m_strHeadings,strText,plvItem->pszText);
				if (EndLabelEdit(plvItem->iItem,plvItem->iSubItem,plvItem->pszText))
					pHeader->SetItem(plvItem->iSubItem,&hdItem);
			}
		}
		else
		{
			if (EndLabelEdit(plvItem->iItem,plvItem->iSubItem,plvItem->pszText))
				AddString(plvItem->iItem, plvItem->iSubItem, plvItem->pszText);
		}
	}
	*pResult = FALSE;
	return TRUE;
}

LRESULT CUIODListCtrl::OnDeleteItem(WPARAM wParam,LPARAM lParam)
{
	if (m_nItems)
		m_nItems--;
	TRACE(_T("Item deleted %d\n"),wParam);
	return Default();
}

LRESULT CUIODListCtrl::OnDeleteColumn(WPARAM wParam,LPARAM lParam)
{
	if (m_nSubItems)
		m_nSubItems--;
	TRACE(_T("Column deleted %d\n"),wParam);
	return Default();
}

LRESULT CUIODListCtrl::OnSetImageList(WPARAM wParam, LPARAM lParam)
{
	if((int)wParam==LVSIL_STATE)
	{
		int cx, cy;

		if(::ImageList_GetIconSize((HIMAGELIST)lParam,&cx,&cy))
			m_cxStateImageOffset=cx;
		else
			m_cxStateImageOffset=0;
	}

	return Default();
}

LRESULT CUIODListCtrl::OnSetTextColor(WPARAM wParam, LPARAM lParam)
{
	m_clrText=(COLORREF)lParam;
	return(Default());
}

LRESULT CUIODListCtrl::OnSetTextBkColor(WPARAM wParam, LPARAM lParam)
{
	m_clrTextBk=(COLORREF)lParam;
	return(Default());
}

LRESULT CUIODListCtrl::OnSetBkColor(WPARAM wParam, LPARAM lParam)
{
	m_clrBkgnd=(COLORREF)lParam;
	return(Default());
}

void CUIODListCtrl::OnSize(UINT nType, int cx, int cy) 
{
	m_cxClient=cx;
	CListCtrl::OnSize(nType, cx, cy);
}

void CUIODListCtrl::OnPaint() 
{
	if (m_bDelayPaint)
		return;
	Default();
    if (GetItemCount() <= 0)
    {
        COLORREF clrText = ::GetSysColor(COLOR_WINDOWTEXT);
        COLORREF clrTextBk = ::GetSysColor(COLOR_WINDOW);

        CDC* pDC = GetDC();
        // Save dc state
        int nSavedDC = pDC->SaveDC();

        CRect rc;
        GetWindowRect(&rc);
        ScreenToClient(&rc);

        CHeaderCtrl* pHC;
        pHC = GetHeaderCtrl();
        if (pHC != NULL)
        {
            CRect rcH;
            pHC->GetItemRect(0, &rcH);
            rc.top += rcH.bottom;
        }
        rc.top += 10;

        pDC->SetTextColor(clrText);
        pDC->SetBkColor(clrTextBk);
        pDC->FillRect(rc, &CBrush(clrTextBk));
        pDC->SelectStockObject(ANSI_VAR_FONT);
        pDC->DrawText(m_strNoItemsMess, -1, rc, 
                      DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_NOCLIP);

        // Restore dc
        pDC->RestoreDC(nSavedDC);
        ReleaseDC(pDC);
    }
    // Do not call CListCtrl::OnPaint() for painting messages
	// in full row select mode, we need to extend the clipping region
	// so we can paint a selection all the way to the right
	if(m_bClientWidthSel && (GetStyle() & LVS_TYPEMASK)==LVS_REPORT && GetFullRowSel() && (GetStyle() & LVS_OWNERDRAWFIXED))
	{
		CRect rcAllLabels;
		GetItemRect(0,rcAllLabels,LVIR_BOUNDS);

		if(rcAllLabels.right<m_cxClient)
		{
			// need to call BeginPaint (in CPaintDC c-tor)
			// to get correct clipping rect
			CPaintDC dc(this);

			CRect rcClip;
			dc.GetClipBox(rcClip);

			rcClip.left=min(rcAllLabels.right-1,rcClip.left);
//			rcClip.right=m_cxClient;

			InvalidateRect(rcClip,FALSE);
			// EndPaint will be called in CPaintDC d-tor
		}
	}

//	CListCtrl::OnPaint();
}

void CUIODListCtrl::OnSetFocus(CWnd* pOldWnd) 
{
	CListCtrl::OnSetFocus(pOldWnd);

	if(pOldWnd!=NULL && ::IsWindow(pOldWnd->m_hWnd))
	{
		// check if we are getting focus from label edit box
		if(pOldWnd!=NULL && pOldWnd->GetParent()==this)
			return;
	}
}

void CUIODListCtrl::OnKillFocus(CWnd* pNewWnd) 
{
	CListCtrl::OnKillFocus(pNewWnd);

	// check if we are losing focus to label edit box
	if (pNewWnd!=NULL && pNewWnd->GetParent()==this)
		return;
}

void CUIODListCtrl::OnRClickHeader(CPoint point,int nColIndex)
{
	m_nColClicked = nColIndex;
	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_POPUPMENU_HEADERCTRL));
	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);		
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,this);
}

void CUIODListCtrl::OnHeaderRemoveColumn()
{
	if (m_nColClicked != -1)
	{
		int nWidth = GetColumnWidth(m_nColClicked);
		if (nWidth)
		{
			m_pColWidths[m_nColClicked] = nWidth;
			SetColumnWidth(m_nColClicked,0);
		}
		m_nColClicked = -1;
	}
}

void CUIODListCtrl::OnHeaderEditText()
{
	CHeaderCtrl *pHeader = (CHeaderCtrl*)GetDlgItem(0);
	if (pHeader == NULL)
		return;

	CString strText;
	int cxy = GetHeaderText(pHeader,m_nColClicked,strText);
	int xPos = 0;
	for(int i=0;i < m_nSubItems;i++)
	{
		if (i == m_nColClicked)
			break;
		xPos += m_pColWidths[i];
	}
	CRect rect;
	rect.left = xPos;
	rect.right = rect.left+cxy;
	rect.top = 0;
	rect.bottom = GetSystemMetrics(SM_CXHSCROLL);
	TRACE2("Creating header edit control at left=%d,right=%d\n",rect.left,rect.right);
	DWORD dwStyle = WS_BORDER|WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL;
	CEdit *pEdit = new CInPlaceEdit(-1, m_nColClicked, strText);
	pEdit->Create( dwStyle, rect, this, -1/*IDC_IPEDIT*/ );
	m_nColClicked = -1;
}

int CUIODListCtrl::GetHeaderText(CHeaderCtrl *pHeader,int nPos,CString &strText)
{
	HD_ITEM hdItem;
	ZeroMemory(&hdItem,sizeof(hdItem));
	int nLen = 128;
	int nRes;
	do
	{
		nLen *= 2;
		hdItem.mask = HDI_TEXT | HDI_WIDTH;
		hdItem.cchTextMax = nLen;
		hdItem.pszText = strText.GetBufferSetLength(nLen);
		pHeader->GetItem(nPos,&hdItem);
		nRes = lstrlen(hdItem.pszText);
	} while (nRes == nLen-1);
	strText.ReleaseBuffer(-1);
	return hdItem.cxy;
}

void CUIODListCtrl::OnHeaderReset()
{
	for(int i=0;i < m_nSubItems;i++)
	{
		SetColumnWidth(i,m_pColWidths[i]);
	}
	m_nColClicked = -1;
}

void CUIODListCtrl::OnContextMenu(CWnd *pWnd,CPoint point)	
{
	CPoint ptClient(point);
	ScreenToClient(&ptClient);	
	int nCol;
	int nRow = HitTestEx(ptClient,&nCol);
	if (nRow == -2)
		return;
	ShowPopupMenu(nRow,nCol,point);
}

void CUIODListCtrl::ShowPopupMenu(int nRow,int nCol,CPoint point)
{
	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_POPUPMENU_LISTCTRL));
	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);
	m_PopupPoint.x = 0;
	m_PopupPoint.y = 0;
	if (PopupMenuItem(nRow,nCol,pPopup,point))
		return;
	m_PopupPoint = point;
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,this);
}

void CUIODListCtrl::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
	// is it for us?
	if (m_PopupPoint.x == 0 && m_PopupPoint.y == 0)
		return;
	if (pPopupMenu->GetMenuItemID(0) == ID_HEADER_REMOVE_COL)
	{
		if (m_nColClicked == -1)
		{
			pPopupMenu->EnableMenuItem(ID_HEADER_EDIT_TEXT,MF_BYCOMMAND | MF_GRAYED);
			pPopupMenu->EnableMenuItem(ID_HEADER_RESET,MF_BYCOMMAND | MF_GRAYED);
		}
		return;
	}
	// These commands are only available in report mode(detail mode)
	UINT nEnabled = (GetViewType() == LVS_REPORT) ? MF_ENABLED : MF_GRAYED; 
	pPopupMenu->EnableMenuItem(ID_UI_VIEW_FULL_ROW_SELECTION,MF_BYCOMMAND | nEnabled);
	pPopupMenu->EnableMenuItem(ID_UI_VIEW_GRIDLINES,MF_BYCOMMAND | nEnabled);
	pPopupMenu->EnableMenuItem(ID_UI_VIEW_COLUMN_SIZING,MF_BYCOMMAND | nEnabled);
	pPopupMenu->EnableMenuItem(ID_UI_VIEW_COLUMN_ORDERING,MF_BYCOMMAND | nEnabled);
	if (nEnabled == MF_ENABLED)
		pPopupMenu->EnableMenuItem(ID_UI_VIEW_EDIT_COLUMN,MF_BYCOMMAND | (m_bEditSubItems && (GetStyle() & LVS_EDITLABELS)) ? MF_ENABLED : MF_GRAYED);
	else
		pPopupMenu->EnableMenuItem(ID_UI_VIEW_EDIT_COLUMN,MF_BYCOMMAND | MF_GRAYED);
	// won't work with owner draw list control
	if (GetStyle() & LVS_OWNERDRAWFIXED)
	{
		pPopupMenu->EnableMenuItem(ID_UI_VIEW_CHECKBOXES,MF_BYCOMMAND | MF_GRAYED);
	}
	UINT nIDItem=ID_VIEW_DETAILS;
	switch(GetViewType())
	{
	case LVS_ICON:
			nIDItem = ID_UI_VIEW_LARGE_ICONS;
			break;
	case LVS_SMALLICON:
			nIDItem = ID_UI_VIEW_SMALL_ICONS;
			break;
	case LVS_LIST:
			nIDItem = ID_UI_VIEW_LIST;
			break;
	case LVS_REPORT:
			nIDItem = ID_UI_VIEW_DETAILS;
			break;
	}
	pPopupMenu->CheckMenuRadioItem(ID_UI_VIEW_LARGE_ICONS,ID_VIEW_LIST,nIDItem,MF_BYCOMMAND);
	DWORD dwExStyle = GetExStyle();
	if (m_bFullRowSel)
		pPopupMenu->CheckMenuItem(ID_UI_VIEW_FULL_ROW_SELECTION,MF_BYCOMMAND | MF_CHECKED);
	if (m_bColumnSizing)
		pPopupMenu->CheckMenuItem(ID_UI_VIEW_COLUMN_SIZING,MF_BYCOMMAND | MF_CHECKED);
	if ((dwExStyle & LVS_EX_HEADERDRAGDROP))
		pPopupMenu->CheckMenuItem(ID_UI_VIEW_COLUMN_ORDERING,MF_BYCOMMAND | MF_CHECKED);
	if ((dwExStyle & LVS_EX_GRIDLINES))
		pPopupMenu->CheckMenuItem(ID_UI_VIEW_GRIDLINES,MF_BYCOMMAND | MF_CHECKED);
	if ((dwExStyle & LVS_EX_CHECKBOXES))
		pPopupMenu->CheckMenuItem(ID_UI_VIEW_CHECKBOXES,MF_BYCOMMAND | MF_CHECKED);
	if ((dwExStyle & LVS_EX_TRACKSELECT))
		pPopupMenu->CheckMenuItem(ID_UI_VIEW_TRACK_SELECT,MF_BYCOMMAND | MF_CHECKED);
}

void CUIODListCtrl::OnViewLargeIcons()
{
	SetViewType(LVS_ICON);
}

void CUIODListCtrl::OnViewSmallIcons()
{
	SetViewType(LVS_SMALLICON);
}

void CUIODListCtrl::OnViewDetails()
{
	SetViewType(LVS_REPORT);
}

void CUIODListCtrl::OnViewList()
{
	SetViewType(LVS_LIST);
}

void CUIODListCtrl::OnViewFullRowSelection()
{
	ToggleFullRowSel();
}

void CUIODListCtrl::OnViewTrackSelect()
{
	ToggleTrackSelect();
}

void CUIODListCtrl::OnViewGridlines()
{
	ToggleGridLines();
}

void CUIODListCtrl::OnViewCheckboxes()
{
	ToggleCheckBoxes();
}

void CUIODListCtrl::OnViewColumnOrdering()
{
	ToggleHeaderDragDrop();
}

void CUIODListCtrl::OnViewColumnSizing()
{
	ToggleColumnSizing();
}

void CUIODListCtrl::OnViewEditColumn()
{
	int index;
	int colnum;
	CPoint point(m_PopupPoint);
	ScreenToClient(&point);
	if (( index = HitTestEx( point, &colnum )) != -1)
	{
		UINT flag = LVIS_FOCUSED;
		if( (GetItemState( index, flag ) & flag) == flag && colnum > 0)
		{
			// Add check for LVS_EDITLABELS
			EditSubLabel( index, colnum );
		}
	}
}

LRESULT CUIODListCtrl::OnGetItemText(WPARAM wParam, LPARAM lParam)
{
	int iItem = (int)wParam; 
	LV_ITEM *pitem = (LV_ITEM*)lParam;
	int nSubItem = pitem->iSubItem;
	int nLen = pitem->cchTextMax;
	const CUIStrListCtrlData *pData = (CUIStrListCtrlData *)GetListCtrlData(iItem);
//	ASSERT_KINDOF(CUIStrListCtrlData,pData);
	if (!pData->IsKindOf(RUNTIME_CLASS(CUIStrListCtrlData)))
	{
		NMLVDISPINFO di;
		di.hdr.hwndFrom = GetSafeHwnd();
		di.hdr.code = LVN_GETDISPINFO;
		memcpy(&di.item,pitem,sizeof(LVITEM));
		di.item.mask = LVIF_TEXT;
#if 1 // bugfix by Rex Lee
		di.item.iItem = iItem;
#endif
		SendMessage(WM_NOTIFY,0,(LPARAM)&di);
		_tcsncpy(pitem->pszText,di.item.pszText,nLen);
	}
	else
	{
		LPTSTR pStr = ((CUIStrListCtrlData *)pData)->GetString(nSubItem);
		if (pStr)
		{
			_tcsncpy(pitem->pszText,pStr,nLen);
			nLen = min(lstrlen(pStr)+1,nLen);
			pitem->pszText[nLen-1] = '\0';
			return nLen;
		}
	}
	return 0L;
}

int CUIODListCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CListCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	// TODO: Add your specialized creation code here
	RegisterDropTarget();
	
	return 0;
}

int CALLBACK CompareFunc(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);

PFNLVCOMPARE CUIODListCtrl::GetCompareFunc()
{
	return CompareFunc;
}

BOOL CUIODListCtrl::ColumnClick(NM_LISTVIEW* pNMListView)
{
	if (pNMListView->iSubItem != m_nSortColumn)
		m_bSortAscending = TRUE;
	else
		m_bSortAscending = !m_bSortAscending;
	m_nSortColumn = pNMListView->iSubItem;

	m_HeaderCtrl.SetSortImage(m_nSortColumn,m_bSortAscending);
	ASSERT(m_pColTypes);
	CUIODListCtrlSortInfo sortinfo(pNMListView->iSubItem,m_pColTypes[pNMListView->iSubItem],m_bSortAscending);
	SortItems(GetCompareFunc(),(DWORD)&sortinfo);
	int item = GetCurSel();
	EnsureVisible(item,0);
	return TRUE;
}

///////////////////////////////////////////////
// static callback for sorting when the header is clicked in report mode
int CALLBACK CompareFunc(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CUIStrListCtrlData *pData1 = (CUIStrListCtrlData*)lParam1;
	CUIStrListCtrlData *pData2 = (CUIStrListCtrlData*)lParam2;
	ASSERT(pData1);
	ASSERT(pData2);
	ASSERT_KINDOF(CUIStrListCtrlData,pData1);
	ASSERT_KINDOF(CUIStrListCtrlData,pData2);
	CUIODListCtrlSortInfo *pSortInfo = (CUIODListCtrlSortInfo *)lParamSort;
	int ret=0;
	switch(pSortInfo->GetColType())
	{
		case CUIODListCtrl::e_DateTime:
			{
				COleDateTime time1;
				CString str1(pData1->GetString(pSortInfo->GetColumn()));
				if (!str1.IsEmpty())
					time1.ParseDateTime(str1);
				COleDateTime time2;
				CString str2(pData2->GetString(pSortInfo->GetColumn()));
				if (!str2.IsEmpty())
					time2.ParseDateTime(str2);
				if (time1 > time2)
					ret = 1;
				else if (time1 < time2)
					ret = -1;
				break;
			}
		case CUIODListCtrl::e_NumericFormat:
			{
				int num1 = _ttoi(CUIODListCtrl::StripNonNumeric((LPCTSTR)pData1->GetString(pSortInfo->GetColumn()),CUIODListCtrl::e_Numeric));
				int num2 = _ttoi(CUIODListCtrl::StripNonNumeric((LPCTSTR)pData2->GetString(pSortInfo->GetColumn()),CUIODListCtrl::e_Numeric));
				ret = num1-num2;
				break;
			}
		case CUIODListCtrl::e_Numeric:
			{
				int num1 = _ttoi((LPCTSTR)pData1->GetString(pSortInfo->GetColumn()));
				int num2 = _ttoi((LPCTSTR)pData2->GetString(pSortInfo->GetColumn()));
				ret = num1-num2;
				break;
			}
		case CUIODListCtrl::e_DoubleFormat:
			{
				double num1 = _tcstod(CUIODListCtrl::StripNonNumeric((LPCTSTR)pData1->GetString(pSortInfo->GetColumn()),CUIODListCtrl::e_Double),NULL);
				double num2 = _tcstod(CUIODListCtrl::StripNonNumeric((LPCTSTR)pData2->GetString(pSortInfo->GetColumn()),CUIODListCtrl::e_Double),NULL);
				if (num1 > num2)
					ret = 1;
				else if (num1 < num2)
					ret = -1;
				break;
			}
		case CUIODListCtrl::e_Double:
			{
				double num1 = _tcstod((LPCTSTR)pData1->GetString(pSortInfo->GetColumn()),NULL);
				double num2 = _tcstod((LPCTSTR)pData2->GetString(pSortInfo->GetColumn()),NULL);
				if (num1 > num2)
					ret = 1;
				else if (num1 < num2)
					ret = -1;
				break;
			}
		case CUIODListCtrl::e_Text:
		default:
			ret = CompareString(LOCALE_USER_DEFAULT,NORM_IGNORECASE | NORM_IGNOREKANATYPE,
							(LPCTSTR)pData1->GetString(pSortInfo->GetColumn()),
							pData1->GetStringLen(pSortInfo->GetColumn()),
							(LPCTSTR)pData2->GetString(pSortInfo->GetColumn()),
							pData2->GetStringLen(pSortInfo->GetColumn())) - 2;
			break;
	}
	if (!pSortInfo->Ascending())
		ret = -ret;
	return ret;
}

void CUIODListCtrl::ReplaceString(CString &rStr,LPCTSTR pszOldText,LPCTSTR pszNewText)
{
	int nPos = rStr.Find(pszOldText);
	CString strLeft;
	if (nPos != -1)
	{
		strLeft = rStr.Left(nPos);
		CString strRight(rStr.Right(rStr.GetLength()-(nPos+lstrlen(pszOldText))));
		strLeft += pszNewText;
		strLeft += strRight;
	}
	rStr = strLeft;
}

void CUIODListCtrl::Sort()
{
	ASSERT(m_pColTypes);
	CUIODListCtrlSortInfo sortinfo(m_nSortColumn,m_pColTypes[m_nSortColumn],m_bSortAscending);
	SortItems(GetCompareFunc(),(DWORD)&sortinfo);
	m_HeaderCtrl.SetSortImage(m_nSortColumn,m_bSortAscending);
	TRACE1("Sorting column %d\n",m_nSortColumn);
}

void CUIODListCtrl::SetColFont(int nRow,int nCol,CFont *pFont)
{
	CUIListCtrlData* pListObj = GetListCtrlData(nRow);
	if (pListObj)
	{
		ASSERT(pFont);
		if (pFont)
			pListObj->SetFont(pFont,nCol);
	}
}

void CUIODListCtrl::SetRowFont(int nRow,CFont *pFont)
{
	CUIListCtrlData* pListObj = GetListCtrlData(nRow);
	if (pListObj)
	{
		ASSERT(pFont);
		if (pFont)
			pListObj->SetFont(pFont);
	}
}

void CUIODListCtrl::SetColBold(int nRow,int nCol,BOOL bBold)
{
	CUIListCtrlData* pListObj = GetListCtrlData(nRow);
	if (pListObj)
	{
		if (!pListObj->IsFontSet(nCol))
		{
			pListObj->SetFont(GetFont(),nCol);
		}
		const CFont *pFont = pListObj->GetFont(nCol);
		LOGFONT lf;
		((CFont*)pFont)->GetLogFont(&lf);
		lf.lfWeight = bBold ? FW_BOLD : FW_NORMAL; 
		CFont font;
		font.CreateFontIndirect(&lf);
		pListObj->SetFont(&font,nCol);
		CRect rect;
		RedrawItems(nRow,nRow);
		UpdateWindow();
	}	
}

void CUIODListCtrl::SetRowBold(int nRow,BOOL bBold)
{
	CUIListCtrlData* pListObj = GetListCtrlData(nRow);
	if (pListObj)
	{
		if (!pListObj->IsFontSet(-1))
		{
			pListObj->SetFont(GetFont());
		}
		const CFont *pFont = pListObj->GetFont();
		LOGFONT lf;
		((CFont*)pFont)->GetLogFont(&lf);
		lf.lfWeight = bBold ? FW_BOLD : FW_NORMAL; 
		CFont font;
		font.CreateFontIndirect(&lf);
		pListObj->SetFont(&font);
		CRect rect;
		RedrawItems(nRow,nRow);
		UpdateWindow();
	}	
}

BOOL CUIODListCtrl::SubItemPostPaint(LPNMLVCUSTOMDRAW lplvcd,LRESULT *pResult)
{
	if (m_hOrigFont)
	{
		CDC *pDC = CDC::FromHandle(lplvcd->nmcd.hdc);
		pDC->SelectObject(CFont::FromHandle(m_hOrigFont));
		m_hOrigFont = NULL;
	}
	return TRUE;
}

BOOL CUIODListCtrl::SubItemPrePaint(LPNMLVCUSTOMDRAW lplvcd,LRESULT *pResult)
{
	*pResult = CDRF_DODEFAULT;
	int nRow = lplvcd->nmcd.dwItemSpec;
	int nCol = lplvcd->iSubItem;
	CUIListCtrlData* pListObj = GetListCtrlData(nRow);
	if (pListObj == NULL)
		return FALSE;
	*pResult = CDRF_NOTIFYPOSTPAINT;
	lplvcd->clrText = pListObj->GetTextColor(nCol);
	lplvcd->clrTextBk = pListObj->GetBkColor(nCol);
	if (pListObj->IsFontSet(nCol))
	{
		CDC *pDC = CDC::FromHandle(lplvcd->nmcd.hdc);
		CFont *pOldFont = pDC->SelectObject((CFont*)pListObj->GetFont(nCol));
		m_hOrigFont = (HFONT)pOldFont;
		*pResult |= CDRF_NEWFONT;
	}
	CTextProgressCtrl *pWnd = (CTextProgressCtrl*)pListObj->GetCtrl(nCol);
	if (pWnd == NULL)
		return TRUE;
	CRect rcItem;
	GetItemRect(nRow,rcItem,LVIR_LABEL);
	rcItem.left = lplvcd->nmcd.rc.left;
	rcItem.right = lplvcd->nmcd.rc.right;
	CDC *pDC = CDC::FromHandle(lplvcd->nmcd.hdc);
	pWnd->DoPaint(pDC,rcItem,GetItemState(nRow,LVIS_SELECTED) == LVIS_SELECTED);
	*pResult = CDRF_SKIPDEFAULT;
	return TRUE;
}

BOOL CUIODListCtrl::ItemPrePaint(LPNMLVCUSTOMDRAW lplvcd,LRESULT *pResult)
{
    *pResult = CDRF_NOTIFYSUBITEMDRAW;
	return TRUE;
}

BOOL CUIODListCtrl::ItemPostPaint(LPNMLVCUSTOMDRAW lplvcd,LRESULT *pResult)
{
	*pResult = CDRF_DODEFAULT;
	return TRUE;
}

///////////////////////////////////////////////
// Persistence
///////////////////////////////////////////////
void CUIODListCtrl::SetSection(LPCTSTR pszSection) 
{
	if (pszSection == NULL)
		m_strSection.Empty();
	else
	{
		SaveProfile();
		m_strSection = _T("UIODListControl\\");
		m_strSection += pszSection;
	}
}

LPCTSTR CUIODListCtrl::GetSection() const
{
	return m_strSection;
}

void CUIODListCtrl::LoadProfile() 
{
	if (m_strSection.IsEmpty())
		return;
	CWinApp *pApp = AfxGetApp();
	if (pApp == NULL)
		return;	
	LPCTSTR pszSection = GetSection();

	m_strHeadings = pApp->GetProfileString(pszSection,szEntryHeadings);
	m_dwExStyle = pApp->GetProfileInt(pszSection,szEntryStyle,m_dwExStyle);
	m_bFullRowSel = pApp->GetProfileInt(pszSection,szEntryRowSel,m_bFullRowSel);
	m_dwViewType = pApp->GetProfileInt(pszSection,szEntryViewType,m_dwViewType);
	m_bColumnSizing = pApp->GetProfileInt(pszSection,szEntryColumnSizing,m_bColumnSizing);
	m_nSortColumn = pApp->GetProfileInt(pszSection,szEntrySortColumn,m_nSortColumn);
	int nSubItems = pApp->GetProfileInt(pszSection,szEntrySubItems,0);
	if (nSubItems)
	{
		delete []m_pColOrder;
		delete []m_pColWidths;
		m_pColOrder = new int[nSubItems];
		m_pColWidths = new int[nSubItems];
		CString strEntry;
		for(int i=0;i < nSubItems;i++)
		{
			strEntry.Format(_T("%s%d"),szEntryColOrder,i+1);
			m_pColOrder[i] = pApp->GetProfileInt(pszSection,strEntry,0);
			strEntry.Format(_T("%s%d"),szEntryColWidths,i+1);
			m_pColWidths[i] = pApp->GetProfileInt(pszSection,strEntry,0);
		}
	}
}

void CUIODListCtrl::SaveProfile()
{
	if (m_strSection.IsEmpty())
		return;
	CWinApp *pApp = AfxGetApp();
	if (pApp == NULL)
		return;		
	LPCTSTR pszSection = GetSection();

	pApp->WriteProfileString(pszSection,szEntryHeadings,m_strHeadings);
	pApp->WriteProfileInt(pszSection,szEntryStyle,SendMessage(LVM_GETEXTENDEDLISTVIEWSTYLE));
	pApp->WriteProfileInt(pszSection,szEntryRowSel,m_bFullRowSel);
	pApp->WriteProfileInt(pszSection,szEntryViewType,GetViewType());
	pApp->WriteProfileInt(pszSection,szEntryColumnSizing,m_bColumnSizing);
	pApp->WriteProfileInt(pszSection,szEntrySubItems,m_nSubItems);
	pApp->WriteProfileInt(pszSection,szEntrySortColumn,m_nSortColumn);
	if (m_nSubItems)
	{
		delete []m_pColOrder;
		m_pColOrder = new int[m_nSubItems],0;
		SendMessage(LVM_GETCOLUMNORDERARRAY,(WPARAM)m_nSubItems,(LPARAM)m_pColOrder);
		CString strEntry;
		for(int i=0;i < m_nSubItems;i++)
		{
			strEntry.Format(_T("%s%d"),szEntryColOrder,i+1);
			pApp->WriteProfileInt(pszSection,strEntry,m_pColOrder[i]);
			if (m_pColWidths)
			{
				strEntry.Format(_T("%s%d"),szEntryColWidths,i+1);
				pApp->WriteProfileInt(pszSection,strEntry,m_pColWidths[i]);
			}
		}
	}
}

void CUIODListCtrl::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strHeadings;
		ar << m_nSubItems;
		m_pColOrder = new int[m_nSubItems];
		SendMessage(LVM_GETCOLUMNORDERARRAY,(WPARAM)m_nSubItems,(LPARAM)m_pColOrder);
		for(int i=0;i < m_nSubItems;i++)
		{
			ar << (int)m_pColOrder[i];
			ar << (int)GetColumnWidth(i);
		}
		delete []m_pColOrder;
		m_pColOrder = NULL;
		ar << SendMessage(LVM_GETEXTENDEDLISTVIEWSTYLE);
		ar << m_dwViewType;
		ar << m_bFullRowSel;
		ar << m_bColumnSizing;
	}
	else
	{
		ar >> m_strHeadings;
		ar >> m_nSubItems;
		delete []m_pColOrder;
		delete []m_pColWidths;
		m_pColOrder = new int[m_nSubItems];
		m_pColWidths = new int[m_nSubItems];
		for(int i=0;i < m_nSubItems;i++)
		{
			ar >> (int)m_pColOrder[i];
			ar >> (int)m_pColWidths[i];
		}
		ar >> m_dwExStyle;
		ar >> m_bFullRowSel;
		ar >> m_dwViewType;
		ar >> m_bColumnSizing;
	}
}
/////////////////////////////////////////////////////////////////
// OLE drag and drop
/////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////

// CUIODListCtrl message handlers
#include "UIDragImage.h"

BOOL CUIODListCtrl::OnBeginRDrag(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	*pResult = 0;	
	DoOleDrag(pNMListView,true);
	return TRUE;
}

BOOL CUIODListCtrl::OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	*pResult = 0;	
	DoOleDrag(pNMListView,false);
	return TRUE;
}

void CUIODListCtrl::DoOleDrag(NM_LISTVIEW* pNMListView,bool bRightMenu)
{
	int nSelected = GetSelectedCount();
	if (nSelected == 0 || pNMListView->iItem == -1)
		return;
	BOOL bRet=TRUE;
	if (bRet == FALSE)
		return;
	// View type LVS_ICON etc
	int nType = GetViewType();
	// 2 ints(nSelected,nType) + ImageData + CRect 
    HANDLE hData = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE,sizeof(int)*2+sizeof(DD_ImageData)*nSelected+sizeof(CRect));
    int *pData = (int*)::GlobalLock(hData);
	*pData++ = nType;
	*pData++ = nSelected;
	// add the selections
    DD_ImageData *pImageData = (DD_ImageData*)pData;
	CRect rcItem;
	CRect rcIcon;
	CRect rcTotalItem;
	int iItem = pNMListView->iItem;
	GetItemRect(iItem,rcItem,LVIR_LABEL); 
	GetItemRect(iItem,rcIcon,LVIR_ICON);
	rcTotalItem.left = rcIcon.left;
	rcTotalItem.top = rcIcon.top;
	rcTotalItem.right = rcIcon.right+rcItem.right;
	rcTotalItem.bottom = 0;
	CPoint point(pNMListView->ptAction);
	CPoint ptHitTest;
	UINT flag=0;
	iItem = GetNextSel(-1);
	int *pRows = new int[nSelected+1];
	pRows[0] = iItem;
	for(int i=1;iItem != -1;i++)
	{
		GetItemRect(iItem,rcItem,LVIR_LABEL); 
		GetItemRect(iItem,rcIcon,LVIR_ICON); 
		ptHitTest.x = rcItem.left;
		ptHitTest.y = rcItem.top;
		HitTest(ptHitTest,&flag);
		if ((flag & LVHT_BELOW) != LVHT_BELOW && (flag & LVHT_ABOVE) != LVHT_ABOVE)
		{
			pImageData->m_rcItem = rcItem;
			pImageData->m_rcIcon = rcIcon;
			pImageData->m_ptDrag = point;
			rcTotalItem.bottom += rcIcon.bottom;
			pImageData++;
		}
		CCF_String ccfText(GetItemText(iItem,0));
		CWDClipboardData::Instance()->SetData(&m_OleDataSource,&ccfText,CWDClipboardData::e_cfString);
		iItem = GetNextSel(iItem);
		pRows[i] = iItem;
	}
	pData = (int*)pImageData;
	*pData++ = rcTotalItem.left;
	*pData++ = rcTotalItem.right;
	*pData++ = rcTotalItem.top;
	*pData = rcTotalItem.bottom;
    ::GlobalUnlock (hData);
    m_OleDataSource.CacheGlobalData(m_OleDropTarget.GetClipboardFormat(), hData);
	CCF_RightMenu rm;
	rm.SetRightDrag(bRightMenu);
	CWDClipboardData::Instance()->SetData(&m_OleDataSource,&rm,CWDClipboardData::e_cfRightMenu);
	DWORD dwDragEffect = SendMessage(WM_APP_OLE_DD_DODRAGDROP,(WPARAM)pRows,(LPARAM)&m_OleDataSource);
	if (dwDragEffect == 0)
		dwDragEffect = GetParent()->SendMessage(WM_APP_OLE_DD_DODRAGDROP,(WPARAM)pRows,(LPARAM)&m_OleDataSource);
	if (dwDragEffect)
	{
		// Start the DragDrop
		DROPEFFECT effect = m_OleDataSource.DoDragDrop(dwDragEffect,NULL);
		// Clear the cache
		m_OleDataSource.Empty();        
	}
	delete []pRows;
}

void CUIODListCtrl::RegisterDropTarget()
{
	if (!IsDragDrop())
		return;
	VERIFY(m_OleDropTarget.Register(this));
	if (IsDropFiles())
		DragAcceptFiles();
}

DROPEFFECT CUIODListCtrl::SelectCurrentTarget(CDD_OleDropTargetInfo *pInfo)
{
	// No method to select a list ctrl item during drag and drop
	UINT flags;
	int iItem = HitTest(pInfo->GetPoint(), &flags);
	pInfo->SetItem(iItem);
	// save it
	m_iItemDrop = iItem;
	LRESULT lResult = SendMessage(WM_APP_OLE_DD_OVER,(WPARAM)pInfo);
	if (lResult == 0)
	{
		lResult = GetParent()->SendMessage(WM_APP_OLE_DD_OVER,(WPARAM)pInfo);
	}
	// returns 0 if allowed (so the default is allowed)
	if (lResult)
	{
		// return what was ever set
		return pInfo->GetDropEffect();
	}
	return DROPEFFECT_NONE;
}

///////////////////////////////////////////////

void CUIODListCtrl::PostNcDestroy() 
{
	// TODO: Add your specialized code here and/or call the base class

	CListCtrl::PostNcDestroy();
}

void CUIODListCtrl::OnDestroy() 
{
	SaveProfile();	

	CListCtrl::OnDestroy();
	
	// TODO: Add your message handler code here
}

void CUIODListCtrl::PreSubclassWindow() 
{
	CListCtrl::PreSubclassWindow();

	// Add initialization code
	EnableToolTips(m_bToolTips);
	SetSortHeader();
}

int CUIODListCtrl::OnToolHitTest(CPoint point, TOOLINFO * pTI) const
{
	int row, col;
	CRect cellrect;
	row = CellRectFromPoint(point, &cellrect, &col );

	if ( row == -1 ) 
		return -1;
	if (GetStringWidth(GetItemText(row,col))-10 < cellrect.Width())
		return -1;

	CToolTipCtrl* pToolTip = AfxGetThreadState()->m_pToolTip;
	pToolTip->SendMessage(TTM_SETDELAYTIME,TTDT_AUTOPOP,10000);
	pToolTip->SendMessage(TTM_SETMAXTIPWIDTH,0,30);

	pTI->hwnd = m_hWnd;
	pTI->uId = (UINT)((row<<10)+(col&0x3ff)+1);
	pTI->lpszText = LPSTR_TEXTCALLBACK;

	pTI->rect = cellrect;
	return pTI->uId;
}

// CellRectFromPoint	- Determine the row, col and bounding rect of a cell
// Returns		- row index on success, -1 otherwise// point		- point to be tested.
// cellrect		- to hold the bounding rect// col			- to hold the column index
int CUIODListCtrl::CellRectFromPoint(CPoint & point, RECT * cellrect, int * col) const
{
	int colnum;	
	// Make sure that the ListView is in LVS_REPORT
	if( (GetWindowLong(m_hWnd, GWL_STYLE) & LVS_TYPEMASK) != LVS_REPORT)
		return -1;	
	// Get the top and bottom row visible	
	int row = GetTopIndex();
	int bottom = row + GetCountPerPage();	
	if( bottom > GetItemCount() )
		bottom = GetItemCount();		
	// Get the number of columns
	CHeaderCtrl* pHeader = (CHeaderCtrl*)GetDlgItem(0);
	int nColumnCount = pHeader->GetItemCount();	
	// Loop through the visible rows
	for( ;row <=bottom;row++)	
	{
		// Get bounding rect of item and check whether point falls in it.		
		CRect rect;
		GetItemRect( row, &rect, LVIR_BOUNDS );		
		if( rect.PtInRect(point) )
		{
			// Now find the column
			for( colnum = 0; colnum < nColumnCount; colnum++ )
			{
				int colwidth = GetColumnWidth(colnum);				
				if( point.x >= rect.left && point.x <= (rect.left + colwidth ) )
				{
						RECT rectClient;
						GetClientRect( &rectClient );					
						if( col ) 
						{
							CUIODListCtrl *pThis = const_cast<CUIODListCtrl*>(this);
							if (m_pColOrder && pThis->SendMessage(LVM_GETCOLUMNORDERARRAY,(WPARAM)m_nSubItems,(LPARAM)m_pColOrder))
								*col = m_pColOrder[colnum];
							else
								*col = colnum;
						}
						rect.right = rect.left + colwidth;
						// Make sure that the right extent does not exceed
						// the client area
						if( rect.right > rectClient.right ) 
							rect.right = rectClient.right;
						*cellrect = rect;					
						return row;		
				}
				rect.left += colwidth;
			}	
		}
	}
	return -1;
}

BOOL CUIODListCtrl::OnToolTipText( UINT id, NMHDR * pNMHDR, LRESULT * pResult )
{
	// need to handle both ANSI and UNICODE versions of the message
	static WCHAR szToolTipW[256];
	static char szToolTipA[256];
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;	
	UINT nID = pNMHDR->idFrom;
	if( nID == 0 )
	{
		// Notification in NT from automatically
		return FALSE;   	
	}
	// created tooltip	
	int row = ((nID-1) >> 10) & 0x3fffff;
	int col = (nID-1) & 0x3ff;	
	m_sToolTipText = GetItemText( row, col );
#ifndef _UNICODE	
//	if (pNMHDR->code == TTN_NEEDTEXTA)
//		lstrcpyn(pTTTA->szText, strTipText, 80);
//	else
//		_mbstowcsz(pTTTW->szText, strTipText, 80);
	if (pNMHDR->code == TTN_NEEDTEXTA)
	{
		pTTTA->lpszText = (LPSTR)(LPCSTR)m_sToolTipText;
	}
	else
	{
		_mbstowcsz(szToolTipW, m_sToolTipText, sizeof(szToolTipW)-1);
		pTTTW->lpszText = szToolTipW;	
	}
#else
	if (pNMHDR->code == TTN_NEEDTEXTW)
	{
		pTTTW->lpszText = (LPWSTR)(LPCWSTR)m_sToolTipText;
	}
	else
	{
		_wcstombsz(szToolTipA, m_sToolTipText, sizeof(szToolTipA)-1);
		pTTTA->lpszText = szToolTipA;
	}
//		pTTTA->lpszText = (LPTSTR)(LPCTSTR)m_sToolTipText;
//	if (pNMHDR->code == TTN_NEEDTEXTA)
//		_wcstombsz(pTTTA->szText, strTipText, 80);
//	else
//		lstrcpyn(pTTTW->szText, strTipText, 80);
#endif	
	*pResult = 0;
	return TRUE;    
	// message was handled
}

CODHeaderCtrl::CODHeaderCtrl()
{
	m_nSortCol = -1;
}

CODHeaderCtrl::~CODHeaderCtrl()
{
}

int CODHeaderCtrl::SetSortImage(int nCol, BOOL bAsc)
{
	if (m_hWnd == NULL)
		return -1;
	int nPrevCol = m_nSortCol;
	
	m_nSortCol = nCol;	
	m_bSortAsc = bAsc;
	// Change the item to ownder drawn	
	HD_ITEM hditem;	
	hditem.mask = HDI_FORMAT;
	GetItem( nCol, &hditem );	
	hditem.fmt |= HDF_OWNERDRAW;
	SetItem( nCol, &hditem );	// Invalidate header control so that it gets redrawn
	Invalidate();	
	return nPrevCol;
}

void CODHeaderCtrl::DrawItem( LPDRAWITEMSTRUCT lpDrawItemStruct )
{
	CDC dc;

	dc.Attach( lpDrawItemStruct->hDC );

	// Get the column rect
	CRect rcLabel( lpDrawItemStruct->rcItem );

	// Save DC
	int nSavedDC = dc.SaveDC();

	// Set clipping region to limit drawing within column
	CRgn rgn;
	rgn.CreateRectRgnIndirect( &rcLabel );
	dc.SelectObject( &rgn );
	rgn.DeleteObject();

        // Draw the background
        dc.FillRect(rcLabel, &CBrush(::GetSysColor(COLOR_3DFACE)));
	
	// Labels are offset by a certain amount  
	// This offset is related to the width of a space character
	int offset = dc.GetTextExtent(_T(" "), 1 ).cx*2;


	// Get the column text and format
	TCHAR buf[256];
	HD_ITEM hditem;
	
	hditem.mask = HDI_TEXT | HDI_FORMAT;
	hditem.pszText = buf;
	hditem.cchTextMax = 255;

	GetItem( lpDrawItemStruct->itemID, &hditem );

	// Determine format for drawing column label
	UINT uFormat = DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP 
						| DT_VCENTER | DT_END_ELLIPSIS ;

	if( hditem.fmt & HDF_CENTER)
		uFormat |= DT_CENTER;
	else if( hditem.fmt & HDF_RIGHT)
		uFormat |= DT_RIGHT;
	else
		uFormat |= DT_LEFT;

	// Adjust the rect if the mouse button is pressed on it
	if( lpDrawItemStruct->itemState == ODS_SELECTED )
	{
		rcLabel.left++;
		rcLabel.top += 2;
		rcLabel.right++;
	}

	// Adjust the rect further if Sort arrow is to be displayed
	if( lpDrawItemStruct->itemID == (UINT)m_nSortCol )
	{
		rcLabel.right -= 3 * offset;
	}

	rcLabel.left += offset;
	rcLabel.right -= offset;

	// Draw column label
	if( rcLabel.left < rcLabel.right )
		dc.DrawText(buf,-1,rcLabel, uFormat);

	// Draw the Sort arrow
	if( lpDrawItemStruct->itemID == (UINT)m_nSortCol )
	{
		CRect rcIcon( lpDrawItemStruct->rcItem );

		// Set up pens to use for drawing the triangle
		CPen penLight(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
		CPen penShadow(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
		CPen *pOldPen = dc.SelectObject( &penLight );

		if( m_bSortAsc )
		{
			// Draw triangle pointing upwards
			dc.MoveTo( rcIcon.right - 2*offset, offset-1);
			dc.LineTo( rcIcon.right - 3*offset/2, rcIcon.bottom - offset );
			dc.LineTo( rcIcon.right - 5*offset/2-2, rcIcon.bottom - offset );
			dc.MoveTo( rcIcon.right - 5*offset/2-1, rcIcon.bottom - offset-1 );

			dc.SelectObject( &penShadow );
			dc.LineTo( rcIcon.right - 2*offset, offset-2);
		}
		else
		{
			// Draw triangle pointing downwords
			dc.MoveTo( rcIcon.right - 3*offset/2, offset-1);
			dc.LineTo( rcIcon.right - 2*offset-1, rcIcon.bottom - offset + 1 );
			dc.MoveTo( rcIcon.right - 2*offset-1, rcIcon.bottom - offset );

			dc.SelectObject( &penShadow );
			dc.LineTo( rcIcon.right - 5*offset/2-1, offset -1 );
			dc.LineTo( rcIcon.right - 3*offset/2, offset -1);
		}

		// Restore the pen
		dc.SelectObject( pOldPen );
	}

	// Restore dc
	dc.RestoreDC( nSavedDC );

	// Detach the dc before returning
	dc.Detach();
}

void CUIODListCtrl::SetSortColumn(int nCol,BOOL bAsc)
{
	m_nSortColumn = nCol;
	m_bSortAscending = bAsc;
	m_HeaderCtrl.SetSortImage(nCol,bAsc);
}

void CUIODListCtrl::SetSortHeader()
{
	if (m_HeaderCtrl.GetSafeHwnd() == NULL && m_hWnd != NULL)
	{
		m_HeaderCtrl.SubclassWindow( ::GetDlgItem(m_hWnd,0) );
	}
}

CString CUIODListCtrl::StripNonNumeric(LPCTSTR pszOldNum,CUIODListCtrl::eColTypes type)
{
	TCHAR szNewNum[255];
	LPTSTR pszNewNum = szNewNum;
	while (*pszOldNum != '\0')
	{
		if ((type == CUIODListCtrl::e_Double && *pszOldNum == '.') || *pszOldNum == '-' || *pszOldNum == '+' || _istdigit(*pszOldNum))
		{
			*pszNewNum = *pszOldNum;
			pszNewNum = _tcsinc(pszNewNum);
		}
		pszOldNum = _tcsinc(pszOldNum);
	}
	*pszNewNum = '\0';
	return szNewNum;
}

BOOL CUIODListCtrl::PreTranslateMessage(MSG* pMsg)
{
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
	return CListCtrl::PreTranslateMessage(pMsg);
}

// User has finished editing a column
bool CUIODListCtrl::EndLabelEdit(int nRow,int nCol,LPCTSTR pszText)
{
	return false;
}

// Left mouse button double clicked
BOOL CUIODListCtrl::DoubleClick(NM_LISTVIEW* pNMListView)
{
	return FALSE;
}

// Enter Pressed
BOOL CUIODListCtrl::OnEnter(NM_LISTVIEW* pNMListView)
{
	return FALSE;
}

// Delete key pressed
bool CUIODListCtrl::DeleteKey(int nRow)
{
	return false;
}

// ALt-Enter pressed
void CUIODListCtrl::ShowProperties(int nRow)
{
}

// F5 key pressed
void CUIODListCtrl::Refresh()
{
}

// Backspace key pressed
void CUIODListCtrl::GoBack(int nSelRow)
{
}

LRESULT CUIODListCtrl::OnDragDrop(WPARAM wParam,LPARAM lParam)
{
	if (IsDragDrop() == false)
		return 1L;
	// get the info we need
	CDD_OleDropTargetInfo *pInfo = (CDD_OleDropTargetInfo*)wParam;
	ASSERT(pInfo);
	return DragDrop(pInfo) ? 1 : 0;
}

// user is currently over the list view window
// return 1 if we are interested in the Clipboard format
LRESULT CUIODListCtrl::OnDragOver(WPARAM wParam,LPARAM lParam)
{
	if (IsDragDrop() == false)
		return 1L;
	CDD_OleDropTargetInfo *pInfo = (CDD_OleDropTargetInfo*)wParam;
	ASSERT(pInfo);
	return DragOver(pInfo) ? 1 : 0;
}

LRESULT CUIODListCtrl::OnDragEnter(WPARAM wParam,LPARAM lParam)
{
	if (IsDragDrop() == false)
		return 1L;
	CDD_OleDropTargetInfo *pInfo = (CDD_OleDropTargetInfo*)wParam;
	ASSERT(pInfo);
	return DragEnter(pInfo) ? 1 : 0;
}

LRESULT CUIODListCtrl::OnDragLeave(WPARAM wParam,LPARAM lParam)
{
	if (IsDragDrop() == false)
		return 1L;
	CDD_OleDropTargetInfo *pInfo = (CDD_OleDropTargetInfo*)wParam;
	ASSERT(pInfo);
	return DragLeave(pInfo) ? 1 : 0;
}

// Drag and drop initiated
// Return 1 if processed
LRESULT CUIODListCtrl::OnDDDoDragDrop(WPARAM wParam,LPARAM lParam)
{
	if (IsDragDrop() == false)
		return 1L;	
	COleDataSource *pOleDataSource = (COleDataSource*)lParam;
	int *pRows = (int*)wParam;
	ASSERT(pOleDataSource);
	return DoDragDrop(pRows,pOleDataSource);
}

bool CUIODListCtrl::DragDrop(CDD_OleDropTargetInfo *pInfo)
{
	return false;
}

bool CUIODListCtrl::DragOver(CDD_OleDropTargetInfo *pInfo)
{
	return false;
}

bool CUIODListCtrl::DragEnter(CDD_OleDropTargetInfo *pInfo)
{
	return false;
}

bool CUIODListCtrl::DragLeave(CDD_OleDropTargetInfo *pInfo)
{
	return false;
}

DROPEFFECT CUIODListCtrl::DoDragDrop(int *pnRows,COleDataSource *pOleDataSource)
{
	return true;
}

void CUIODListCtrl::OnDropFiles(HDROP hDropInfo)
{
	UINT wNumFilesDropped = DragQueryFile(hDropInfo, 0XFFFFFFFF, NULL, 0);
	TCHAR szFile[MAX_PATH];
	UINT nLen;
	UINT nFlags;
	POINT pt;
	::DragQueryPoint(hDropInfo,&pt);
	int nRow = HitTest(CPoint(pt), &nFlags);
	for(UINT i=0; i < wNumFilesDropped;i++)
	{
		nLen = DragQueryFile(hDropInfo,i,szFile,sizeof(szFile)/sizeof(TCHAR));
		if (nLen)
			OnDropFile(nRow,szFile,nFlags);
	}
}

void CUIODListCtrl::OnDropFile(int nRow,LPCTSTR pszFile,UINT nFlags)
{
}

LRESULT CUIODListCtrl::OnAppPropertiesKey(WPARAM wParam, LPARAM lParam)
{
	int item=-1;
	while ((item = GetNextSel(item)) != -1)
	{
		ShowProperties(item);
	}
	return 1L;
}

LRESULT CUIODListCtrl::OnAppDeleteKey(WPARAM wParam, LPARAM lParam)
{
	int nNewItem = -1;
	int item=-1;
	CUIListCtrlData *pData = NULL;
	while ((item = GetNextSel(item)) != -1)
	{
		pData = GetListCtrlData(item);
		if (DeleteKey(item))
		{
			pData->SetDeleted(true);
		}
		nNewItem = item;
	}
	while ((item = GetCurSel()) != -1)
	{
		pData = GetListCtrlData(item);				
		if (pData->IsDeleted())
		{
			if (!DeleteItem(item))
				SetItemState(item,0,LVIS_SELECTED);
		}
		else
		{
			SetItemState(item,0,LVIS_SELECTED);
		}
		if (GetItemCount() == 0)
			break;
	}
	if (nNewItem != -1)	
	{
		EnsureVisible(nNewItem,0);
		SetCurSel(nNewItem);
	}
	return 1L;
}

LRESULT CUIODListCtrl::OnAppRefreshKey(WPARAM wParam, LPARAM lParam)
{
	Refresh();
	return 1L;
}

LRESULT CUIODListCtrl::OnAppEditKey(WPARAM wParam, LPARAM lParam)
{
	EditLabel(GetCurSel());
	return 1L;
}

LRESULT CUIODListCtrl::OnAppContextMenuKey(WPARAM wParam, LPARAM lParam)
{
	CPoint pt;
	if (GetCurSel() != -1)
		GetItemPosition(GetCurSel(),&pt);
	ClientToScreen(&pt);
	ShowPopupMenu(GetCurSel(),0,pt);
	return 1L;
}

LRESULT CUIODListCtrl::OnAppBackspaceKey(WPARAM wParam, LPARAM lParam)
{
	GoBack(GetCurSel());
	return 1L;
}

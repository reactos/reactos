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

#if !defined(AFX_URLTREECTRL_H__9743E546_1F3A_11D2_A40D_9CB186000000__INCLUDED_)
#define AFX_URLTREECTRL_H__9743E546_1F3A_11D2_A40D_9CB186000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// URLTreeCtrl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CUITreeCtrl window
#include "UIFolderRefresh.h"
#include "UIDragDropTree.h"
#include "UIData.h"

class CTRL_EXT_CLASS CUITreeCtrl : public CUIDragDropTree
{
	DECLARE_DYNAMIC(CUITreeCtrl)
// Construction
public:
	CUITreeCtrl(bool bDragDrop=true);

// Attributes
public:
	virtual void UpdateCurrentSelection();

	void SetStyle(UINT nStyle);
	void AddStyle(UINT nStyle);
	DWORD GetItemData(HTREEITEM hItem) const;
	BOOL SetItemData(HTREEITEM hItem,DWORD dwData,bool bAutoDelete=false);
	void SetDefaultTextColor(HTREEITEM hItem);
	void SetDefaultBkColor(HTREEITEM hItem);
	void SetTextColor(HTREEITEM hItem,COLORREF TextColor);
	void SetBkColor(HTREEITEM hItem,COLORREF BkColor);
	void SetItemFont(HTREEITEM hItem,CFont *pFont);
	void SetItemBold(HTREEITEM hItem,bool bBold);
// Operations
public:
	virtual void Init();
	virtual HTREEITEM AddAnItem(HTREEITEM hParent, LPCTSTR szText, LPARAM lParam, HTREEITEM hInsAfter,int iImage = 0,int iSelImage = 0,int nChildren = 0);
	void SetPopupID(UINT nID);
	void SetPopupWnd(CWnd *pWnd);
protected:
	virtual void OnDeleteItemData(DWORD dwData);
	virtual CUIListCtrlData *GetNewListCtrlData(DWORD dwData,HTREEITEM hParent);
	virtual CUIListCtrlData *GetListCtrlData(HTREEITEM hItem) const;
	virtual void ItemPostPaint(LPNMTVCUSTOMDRAW lptvcd,LRESULT *pResult);
	virtual void ItemPrePaint(LPNMTVCUSTOMDRAW lptvcd,LRESULT *pResult);
	virtual bool EndLabelEdit(HTREEITEM hItem,LPCTSTR pszText);
	virtual HTREEITEM GetHitItem(CPoint *pt);
	virtual UINT GetMenuID();
	virtual CWnd *GetMenuWnd();
	virtual bool Expanding(NM_TREEVIEW *nmtvw);
	virtual bool Collapsing(NM_TREEVIEW *nmtvw);
	virtual void UpdateEvent(LPARAM lHint,CObject *pHint);
	virtual UINT GetIconID();
	virtual UINT GetOpenIconID();
	virtual void DoubleClick(HTREEITEM hItem);
	virtual void SelectionChanged(HTREEITEM hItem,LPARAM lParam);
	virtual CRefresh *CreateRefreshObject(HTREEITEM hItem,LPARAM lParam);
	// OLE Drag and Drop
	// This called when a drop source is dropped on the list control
	virtual bool DragDrop(CDD_OleDropTargetInfo *pInfo);
	// This called when a drop source is currently over the list control
	// This called when a drop source is currently over the list control
	virtual bool DragOver(CDD_OleDropTargetInfo *pInfo);
	virtual bool DragEnter(CDD_OleDropTargetInfo *pInfo);
	virtual bool DragLeave(CDD_OleDropTargetInfo *pInfo);
	// This called when the user initiates a OLE drag and drop
	virtual DROPEFFECT DoDragDrop(NM_TREEVIEW* pNMTreeView,COleDataSource *pOleDataSource);
	virtual void DeleteKey(HTREEITEM hItem);
	virtual void ShowPopupMenu(HTREEITEM hItem,CPoint point);
	virtual void ShowProperties(HTREEITEM hItem);
	virtual void Refresh();
	virtual void GoBack(HTREEITEM hItem);
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUITreeCtrl)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CUITreeCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CUITreeCtrl)
	afx_msg BOOL OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnReturn(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnItemExpanded(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnItemExpanding(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnRclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnDeleteItem(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeydownFolder(NMHDR* pNMHDR, LRESULT* pResult);

	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg LRESULT OnTVMGetItem(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnTVMSetItem(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnTVMInsertItem(WPARAM wParam,LPARAM lParam);
	//}}AFX_MSG
	afx_msg LRESULT OnAppPropertiesKey(WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnAppDeleteKey(WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnAppRefreshKey(WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnAppBackspaceKey(WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnAppContextMenuKey(WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnAppEditKey(WPARAM wParam, LPARAM lParam );

	afx_msg LRESULT OnTimerSelChange(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnDragDrop(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnDragOver(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnDragEnter(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnDragLeave(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnDDDoDragDrop(WPARAM wParam,LPARAM lParam);

	DECLARE_MESSAGE_MAP()
protected:
	int m_idxClosed;
	int m_idxOpen;
	WORD m_vkeydown;
private:
	CImageList m_ImageList;
	NM_TREEVIEW m_NMTreeView;
	LPARAM m_lParam;
	UINT m_IDTimer;
	UINT m_style;
	HFONT m_hOrigFont;
	bool m_bMe;
protected:
	CWnd *m_pPopupWnd;
	UINT m_PopupID;
};

inline void CUITreeCtrl::SetStyle(UINT nStyle)
{
	m_style = nStyle;
}

inline void CUITreeCtrl::AddStyle(UINT nStyle)
{
	m_style |= nStyle;
}

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_URLTREECTRL_H__9743E546_1F3A_11D2_A40D_9CB186000000__INCLUDED_)

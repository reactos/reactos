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

#if !defined(AFX_DRAGDROPTREE_H__072E5B93_FD2A_11D0_ADBE_0000E81B9EF1__INCLUDED_)
#define AFX_DRAGDROPTREE_H__072E5B93_FD2A_11D0_ADBE_0000E81B9EF1__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DragDropTree.h : header file
//

#include "UIImageDropTarget.h"
// ///////////////////////////////////////////////////////////////////////////
class CUI_TreeDropTarget : public CUI_ImageDropTarget
{
public:
	virtual DROPEFFECT OnDragEnter( CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point );
	virtual DROPEFFECT OnDragOver( CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point );
	virtual BOOL OnDrop( CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point );
	virtual void OnDragLeave( CWnd* pWnd);
protected:
	DWORD m_dwKeyboardState;
	DWORD m_dwEnterKeyboardState;
};

/////////////////////////////////////////////////////////////////////////////
// CUIDragDropTree window

class CTRL_EXT_CLASS CUIDragDropTree : public CTreeCtrl
{
	friend class CUI_TreeDropTarget;
// Construction
public:
	DECLARE_DYNAMIC(CUIDragDropTree)
	CUIDragDropTree(bool bDragDrop=true);

// Attributes
public:
	enum CopyMode
	{
		eDDNull,
		eDDMove,
		eDDCopy,
		eDDCancel
	};
	enum SCROLLMODE
	  {
		SCROLL_UP_SLOW,
		SCROLL_DOWN_SLOW,
		SCROLL_UP_NORMAL,
		SCROLL_DOWN_NORMAL
	  };

	const COleDropTarget &GetDropTarget();

// Operations
public:
	void RegisterDropTarget();
	void SetDragTimer();
	void KillDragTimer();

	virtual DROPEFFECT SelectCurrentTarget(CDD_OleDropTargetInfo *pInfo);
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUIDragDropTree)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CUIDragDropTree();

protected:
	HTREEITEM GetDragItem();
	HTREEITEM GetDropItem();
	virtual BOOL	IsChildNodeOf(HTREEITEM hitemChild, HTREEITEM hitemSuspectedParent);
	virtual BOOL		NeedToScroll( CPoint ptMouse );
	virtual SCROLLMODE	RefineScrollMode( CPoint ptMouse );
	virtual BOOL	TransferItem(HTREEITEM hitem, HTREEITEM hNewParent);
	virtual void OnButtonUp(bool bMove);
	virtual void OnDropFile(HTREEITEM hItem,LPCTSTR pszFile,UINT nFlags);
	virtual void NewTransferItem(HTREEITEM hNewItem);
	virtual BOOL StartDragDrop(NM_TREEVIEW* pNMTreeView);
	virtual bool GetRDragMenu(CMenu *pMenu);
	virtual void EndDragging();
	// Generated message map functions
protected:
	//{{AFX_MSG(CUIDragDropTree)
	afx_msg BOOL OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnBeginRDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDropFiles( HDROP hDropInfo);
	afx_msg void OnDDMove();
	afx_msg void OnDDCopy();
	afx_msg void OnDDCancel();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
private:
	bool m_bDragDrop;
	bool m_bDropFiles;
	CImageList*	m_pImageList;
	BOOL		m_bDragging;
	HTREEITEM	m_hitemDrag;
	HTREEITEM	m_hitemDrop;
	UINT	m_nTimerID;
	int		m_nSlowScrollTimeout;
	int		m_nUpperYCoor;
	int		m_nLowerYCoor;	
	int		m_nScrollBarSize;
	CUI_TreeDropTarget m_OleDropTarget;
	COleDataSource m_OleDataSource;
protected:
	CopyMode		m_CopyMode;
public:
	void SetDragDrop(bool bDragDrop);
	bool GetDragDrop();
	void SetDropFiles(bool bDropFiles);
	bool GetDropFiles();
};

inline void CUIDragDropTree::SetDragDrop(bool bDragDrop)
{
	m_bDragDrop = bDragDrop;
}

inline bool CUIDragDropTree::GetDragDrop()
{
	return m_bDragDrop;
}

inline void CUIDragDropTree::SetDropFiles(bool bDropFiles)
{
	m_bDropFiles = bDropFiles;
}

inline bool CUIDragDropTree::GetDropFiles()
{
	return m_bDropFiles;
}

inline const COleDropTarget &CUIDragDropTree::GetDropTarget()
{
	return m_OleDropTarget;
}

inline HTREEITEM CUIDragDropTree::GetDragItem()
{
	return m_hitemDrag;
}

inline HTREEITEM CUIDragDropTree::GetDropItem()
{
	return m_hitemDrop;
}

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DRAGDROPTREE_H__072E5B93_FD2A_11D0_ADBE_0000E81B9EF1__INCLUDED_)

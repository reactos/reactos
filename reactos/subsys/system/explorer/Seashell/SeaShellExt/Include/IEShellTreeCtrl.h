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

#ifndef __IESHELLTREECTRL_H__
#define __IESHELLTREECTRL_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXMT_H__
#include "afxmt.h"
#endif
// IEShellTreeCtrl.h : header file
//
#include <afxpriv.h>
#include "IEFolderTreeCtrl.h"
#include "IEShellDragDrop.h"
/////////////////////////////////////////////////////////////////////////////
// CIEShellTreeCtrl window

class CTRL_EXT_CLASS CIEShellTreeCtrl : public CIEFolderTreeCtrl
{
// Construction
public:
	CIEShellTreeCtrl();

// Attributes
public:
	void SetNotifyParent(bool bNotifyParent) { m_bNotifyParent = bNotifyParent; }
	bool RefreshAllowed() { return m_bRefreshAllowed; }
	void SetRefreshAllowed(bool bRefresh) { m_bRefreshAllowed = bRefresh; } 
	void SetListCtrlWnd(HWND hWnd) { m_hListWnd = hWnd; }
	void SetComboBoxWnd(HWND hWnd) { m_hComboWnd = hWnd; }
	CString GetSelectedPath() { return GetPathName(); }
// Operations
public:
	virtual void UpOneLevel(HTREEITEM hItem=NULL);
	virtual void ShellExecute(HTREEITEM hItem,LPCTSTR pszVerb=NULL);
	virtual void SetNotificationObject(bool bNotify);
	virtual bool LoadFolderItems(LPCTSTR pszPath=NULL);
	virtual bool GetFolderInfo(HTREEITEM hItem,CString &sPath,CString &sName);
	virtual void Refresh();
	virtual HTREEITEM FindPidl(LPITEMIDLIST pidlAbs,BOOL bSelect=TRUE);
	virtual HTREEITEM ExpandPidl(LPITEMIDLIST pidlAbs);
	virtual HTREEITEM ExpandMyComputer(LPITEMIDLIST pidlAbs);
// Overrides
protected:
	virtual void RefreshComboBox(LPTVITEMDATA lptvid);
	virtual void DestroyThreads();
	virtual void CreateFileChangeThreads(HWND hwnd);
	virtual void CreateFileChangeThread (const CString& sPath,HWND hwnd);
	virtual HTREEITEM SearchSiblings(HTREEITEM hItem,LPITEMIDLIST pidlAbs);
	virtual HTREEITEM FindItem(HTREEITEM hItem, const CString& strTarget);
	virtual UINT DeleteChildren(HTREEITEM hItem);
	virtual bool SHMoveFile(HTREEITEM hSrcItem,HTREEITEM hDestItem);
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIEShellTreeCtrl)
	virtual BOOL OnWndMsg( UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult );
	virtual bool Expanding(NM_TREEVIEW *nmtvw);
	virtual CRefresh *CreateRefreshObject(HTREEITEM hItem,LPARAM lParam);
	virtual DROPEFFECT DoDragDrop(NM_TREEVIEW* pNMTreeView,COleDataSource *pOleDataSource);
	virtual bool EndLabelEdit(HTREEITEM hItem,LPCTSTR pszText);
	virtual BOOL TransferItem(HTREEITEM hitemDrag, HTREEITEM hitemDrop);
	protected:
	virtual void Init();
	virtual void ShowProperties(HTREEITEM hItem);
	virtual void ShowPopupMenu(HTREEITEM hItem,CPoint point);
	virtual void DoubleClick(HTREEITEM hItem);
	virtual void DeleteKey(HTREEITEM hItem);
	virtual void GoBack(HTREEITEM hItem);
	virtual void PreSubclassWindow();
	virtual void UpdateEvent(LPARAM lHint,CObject *pHint);
	virtual bool DragDrop(CDD_OleDropTargetInfo *pInfo);
	virtual bool DragOver(CDD_OleDropTargetInfo *pInfo);
	virtual bool DragEnter(CDD_OleDropTargetInfo *pInfo);
	virtual bool DragLeave(CDD_OleDropTargetInfo *pInfo);
	//}}AFX_VIRTUAL
// Implementation
public:
	virtual ~CIEShellTreeCtrl();

// Generated message map functions
protected:
	//{{AFX_MSG(CIEShellTreeCtrl)
			// NOTE - the ClassWizard will add and remove member functions here.
	afx_msg BOOL OnRclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	afx_msg LRESULT OnAppDirChangeEvent(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCBIESelChange(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnAppPopulateTree(WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnAppCbIeHitEnter(WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnSetmessagestring(WPARAM wParam, LPARAM lParam );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	enum { MAX_THREADS=32 };
	static UINT ThreadFunc (LPVOID pParam);
	LPTVITEMDATA m_lptvid;
	HANDLE m_hThreads[MAX_THREADS];
	CWinThread *m_pThreads[MAX_THREADS];
	CEvent m_event[MAX_THREADS];
	int m_nThreadCount;
	bool m_bRefreshAllowed;
	bool m_bNotifyParent;
	CIEShellDragDrop m_ShellDragDrop;
	HWND m_hListWnd;
	HWND m_hComboWnd;
};

/////////////////////////////////////////////////////////////////////////////
typedef struct DC_THREADINFO
{
    HANDLE hEvent;
    CIEShellTreeCtrl *pTreeCtrl;
	CString sPath;
} DC_THREADINFO, *PDC_THREADINFO;


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif //__IESHELLTREECTRL_H__

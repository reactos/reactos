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

#if !defined(AFX_IEFOLDERTREECTRL_H__9743E545_1F3A_11D2_A40D_9CB186000000__INCLUDED_)
#define AFX_IEFOLDERTREECTRL_H__9743E545_1F3A_11D2_A40D_9CB186000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// IEFolderTreeCtrl.h : header file
//
#include "ShellPidl.h"
#include "ShellSettings.h"
#include "Refresh.h"
#include "UITreeCtrl.h"
/////////////////////////////////////////////////////////////////////////////
// CIEFolderTreeCtrl window
#include <shlobj.h>

class CTRL_EXT_CLASS CIEFolderTreeCtrl : public CUITreeCtrl
{
// Construction
public:
	CIEFolderTreeCtrl();

// Attributes
public:
	CShellPidl &GetShellPidl();
	CString GetRootPath();
	void SetRootPath(const CString &sPath);
	const CShellSettings &GetShellSettings() const;
	CShellSettings &GetShellSettings();
	virtual LPCITEMIDLIST GetPathPidl(HTREEITEM hItem);
	virtual CString GetPathName(HTREEITEM hItem=NULL);

// Operations
public:
	virtual void Sort(HTREEITEM hParent,LPSHELLFOLDER pFolder);
	virtual void Refresh();
protected:
	virtual LPSHELLFOLDER GetItemFolder(HTREEITEM hItem);
	virtual HTREEITEM AddFolder(HTREEITEM hItem,LPITEMIDLIST pidl,LPSHELLFOLDER pFolder);
	virtual HTREEITEM AddFolder(HTREEITEM hItem,LPCTSTR pszPath);
	virtual bool LoadItems(LPCTSTR pszPath=NULL,DWORD dwFolderType=0);
	virtual bool AddItems(HTREEITEM hItem,IShellFolder* pFolder);
	virtual void OnDeleteItemData(DWORD dwData);
	virtual void SetButtonState(HTREEITEM hItem);
	virtual void RefreshNode(HTREEITEM hItem);
	virtual void SetAttributes(HTREEITEM hItem,LPSHELLFOLDER pFolder,LPITEMIDLIST pidl);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIEFolderTreeCtrl)
	public:
	virtual void Init();
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType = adjustBorder);
	//}}AFX_VIRTUAL
	virtual BOOL LoadURL(HTREEITEM hItem);

// Implementation
public:
	virtual ~CIEFolderTreeCtrl();

	// Generated message map functions
protected:
	static int CALLBACK CompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	//{{AFX_MSG(CIEFolderTreeCtrl)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDeleteItem(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg LRESULT OnSettingChange(WPARAM wParam,LPARAM lParam);
	afx_msg void OnDestroy();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
private:
	IMalloc* m_pMalloc;
	CShellPidl m_ShellPidl;
	CString m_sRootPath;
    HIMAGELIST  m_hImageList;
	CShellSettings m_ShellSettings;
};

inline const CShellSettings &CIEFolderTreeCtrl::GetShellSettings() const
{
	return m_ShellSettings;
}

inline CShellSettings &CIEFolderTreeCtrl::GetShellSettings()
{
	return m_ShellSettings;
}

inline CString CIEFolderTreeCtrl::GetRootPath()
{
	return m_sRootPath;
}

inline void CIEFolderTreeCtrl::SetRootPath(const CString &sPath)
{
	m_sRootPath = sPath;
}

inline CShellPidl &CIEFolderTreeCtrl::GetShellPidl()
{
	return m_ShellPidl;
}
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IEFOLDERTREECTRL_H__9743E545_1F3A_11D2_A40D_9CB186000000__INCLUDED_)

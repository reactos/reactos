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


#if !defined(AFX_IESHELLCOMBOBOX_H__1EABA279_32DD_4A2D_8957_F478E4D3E5EB__INCLUDED_)
#define AFX_IESHELLCOMBOBOX_H__1EABA279_32DD_4A2D_8957_F478E4D3E5EB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// IEShellComboBox.h : header file
//
#include "ShellContextMenu.h"
#include "ShellPidl.h"
#include "Refresh.h"

////////////////////////////////////////////////
// CIEShellComboBoxEdit
////////////////////////////////////////////////
class CIEShellComboBoxEdit : public CEdit
{
public:
	CIEShellComboBoxEdit();
	virtual ~CIEShellComboBoxEdit();

public:
	CString &GetText() { GetWindowText(m_sText); return m_sText; }
	void SetTreeWnd(HWND hWnd);  
	virtual BOOL PreTranslateMessage(MSG* pMsg);
protected:
private:
	CString m_sText;
	HWND m_hTreeWnd;	
};

inline CIEShellComboBoxEdit::CIEShellComboBoxEdit()
{
	m_hTreeWnd = NULL;
}

inline CIEShellComboBoxEdit::~CIEShellComboBoxEdit()
{

}

inline void CIEShellComboBoxEdit::SetTreeWnd(HWND hWnd)
{
	m_hTreeWnd = hWnd;
}

/////////////////////////////////////////////////////////////////////////////
// CIEShellComboBox window

class CTRL_EXT_CLASS CIEShellComboBox : public CComboBoxEx
{
// Construction
public:
	CIEShellComboBox();

// Attributes
public:
	CShellPidl &GetShellPidl() { return m_ShellPidl; }
	LPITEMIDLIST GetSelectedPidl();
	void SetTreeCtrlWnd(HWND hWnd) { m_hTreeWnd = hWnd; }
// Operations
public:
	void Populate(LPITEMIDLIST pidlAbsSel=NULL);
protected:
	void BuildFolderList(LPSHELLFOLDER pFolder,LPITEMIDLIST pidl,LPITEMIDLIST pidlAbsSel,int nIndent);
	void InitItems(LPITEMIDLIST pidlAbsSel);
	void LoadItems(LPITEMIDLIST pidlAbsSel);
	void LoadURLPrevList();
	void SetShellImageList();
	int AddItem(const CShCMSort *pItem);
	void DeleteItemData(LPTVITEMDATA pItemData);
	void DeleteAllItemData();
	void AddItems(vecCMSort &vItems,IShellFolder* pFolder,LPITEMIDLIST pidlAbs,int nIndent);
	void AddFolder(vecCMSort &vItems,const SHFILEINFO &FileInfo,LPITEMIDLIST pidlAbs,LPITEMIDLIST pidl,LPSHELLFOLDER pFolder,int nIndent);
	void SelectionChanged(bool bEnter=false);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIEShellComboBox)
	public:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CIEShellComboBox();

	// Generated message map functions
protected:
	//{{AFX_MSG(CIEShellComboBox)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnKillFocus(CWnd *pNewWnd);
	afx_msg void OnSetFocus(CWnd *pWnd);
	afx_msg void OnDropDown();
	afx_msg void OnSelChange();
	afx_msg LRESULT OnCBIEPopulate(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnAppCbIeHitEnter(WPARAM wParam, LPARAM lParam );
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
private:
	LPMALLOC     m_pMalloc;
	LPITEMIDLIST m_pidlMyComputer;
	LPITEMIDLIST m_pidlMyDocuments;
	LPITEMIDLIST m_pidlInternet;
	CShellPidl   m_ShellPidl;
	vecItemData  m_vecItemData;
	vecCMSort    m_vItems;
    HIMAGELIST   m_hImageList;
	CImageList	 m_ImageList;
	HWND		 m_hTreeWnd;	
	HICON		 m_hIcon;
	CIEShellComboBoxEdit m_cbEdit;
	CString		 m_sText;
	bool		 m_bInternet;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IESHELLCOMBOBOX_H__1EABA279_32DD_4A2D_8957_F478E4D3E5EB__INCLUDED_)

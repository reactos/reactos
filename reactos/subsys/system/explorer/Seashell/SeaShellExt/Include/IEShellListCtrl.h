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

#ifndef __IESHELLLISTCTRL_H__
#define __IESHELLLISTCTRL_H__

#ifndef __AFXMT_H__
#include "afxmt.h"
#endif

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

typedef struct SLC_COLUMN_DATA
{
	SLC_COLUMN_DATA() { pidl = NULL; iImage = -1;} 
	CStringArray sItems;
	LPITEMIDLIST pidl;
	int iImage;
} SLC_COLUMN_DATA, *PSLC_COLUMN_DATA;

class CCF_HDROP;

// IEShellListCtrl.h : header file
//
#include <afxpriv.h>
#include "UICtrl.h"
#include "ShellPidl.h"
#include "ShellDetails.h"
#include "ShellSettings.h"
#include "Refresh.h"
#include "IEShellDragDrop.h"
/////////////////////////////////////////////////////////////////////////////
// CIEShellListCtrl window
#include <vector>
using namespace std;
typedef vector<LPLVITEMDATA> vecListItemData;

class CTRL_EXT_CLASS CIEShellListCtrl : public CUIODListCtrl
{
// Construction
public:
	CIEShellListCtrl();

	enum Cols
	{
		COL_NAME,
		COL_SIZE,
		COL_TYPE,
		COL_MODIFIED,
		COL_MAX
	};
// Attributes
public:
	void SetFileFilter(LPCTSTR pszFilter) { m_sFileFilter = pszFilter; }
	CString GetFileFilter() { return m_sFileFilter.IsEmpty() ? _T("*.*") : m_sFileFilter; }
	void SetNotifyParent(bool bNotifyParent) { m_bNotifyParent = bNotifyParent; }
	void NoExtAllowed() { m_bNoExt = true; }
	void ExcludeFileType(LPCTSTR pszType) { m_ExcludedLookup.AddHead(pszType); }
	CStringList &GetExcludedFileTypes() { return m_ExcludedLookup; }
	void FillExcludedFileTypes(CComboBox &cb);
	void SetExcludedFileTypes(CComboBox &cb);
	bool RefreshAllowed() { return m_bRefreshAllowed; }
	void SetRefreshAllowed(bool bRefresh) { m_bRefreshAllowed = bRefresh; } 
	CShellPidl &GetShellPidl() { return m_ShellPidl; }
	LPMALLOC GetMalloc() { return m_pMalloc; }
	LPTVITEMDATA GetTVID() { return &m_tvid; }
	LPSHELLFOLDER GetShellFolder() { return m_tvid.lpsfParent; }
	bool GetCallBack() { return m_bCallBack; }
	CString GetPathName(int nRow);
	CString GetCurrPathName();
	LPCITEMIDLIST GetPathPidl(int nRow);
	const CShellSettings &GetShellSettings() const;
	CShellSettings &GetShellSettings();
// Operations
public:
	virtual void PopupTheMenu(int nRow,CPoint point);
	virtual void ShellExecute(int nRow,LPCTSTR pszVerb=NULL);
	virtual void SetNotificationObject(bool bNotify);
	virtual BOOL Populate(LPTVITEMDATA lptvid,LPSHELLFOLDER psfFolder,bool bCallBack);
	virtual BOOL Populate(LPCTSTR pszPath, bool bCallBack=false);
	virtual BOOL Populate(LPTVITEMDATA lptvid);
	virtual void Refresh();
	virtual void StartPopulate();
	virtual void EndPopulate();
// Overridables
protected:
	virtual void DestroyThreads();
	virtual void Load();
	virtual void SetColumnWidths();
	virtual void RemoveExt(LPTSTR pszFileName);
	virtual void InitColumns();
	virtual CString GetColumns();
	virtual BOOL InitImageLists();
	virtual BOOL InitItems(LPTVITEMDATA lptvid, bool bCallBack);
	virtual void InitShellSettings();
	virtual bool FilterItem(LPSHELLFOLDER pFolder,LPCITEMIDLIST pidl,UINT ulAttrs);
	virtual bool GetColText(int nCol,LPCITEMIDLIST pidlAbs,LPLVITEMDATA lplvid,CString &sText);
	virtual void GetText(PSLC_COLUMN_DATA pColData,LV_DISPINFO *pDispInfo,LPLVITEMDATA lplvid);
	virtual void GetFileDetails(LPCITEMIDLIST pidl, CString &sSize, CString &sDateTime);
	virtual void CreateFileChangeThread(HWND hwnd);
	virtual void FreeTVID();
	virtual void FreeInterface(IUnknown *pInterface);
	virtual bool UseShellColumns();
	virtual LPCTSTR GetFilterMask(LPCTSTR pszFilter,CString &sMask);
private:
	void LoadFilterFiles(const CString &sFileFilter);
// Overrides	
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIEShellListCtrl)
	public:
	virtual void ChangeStyle(UINT &dwStyle);
	virtual void Init();
	protected:
	virtual void GoBack(int nRow);
	virtual void ShowPopupMenu(int nRow,int nCol,CPoint point);
	virtual BOOL OnEnter(NM_LISTVIEW* pNMListView);
	virtual BOOL DoubleClick(NM_LISTVIEW* pNMListView);
	virtual void PreSubclassWindow();
	virtual bool DragOver(CDD_OleDropTargetInfo *pInfo);
	virtual bool DragLeave(CDD_OleDropTargetInfo *pInfo);
	virtual bool DragEnter(CDD_OleDropTargetInfo *pInfo);
	virtual bool DragDrop(CDD_OleDropTargetInfo *pInfo);
	virtual BOOL GetDispInfo(LV_DISPINFO *pDispInfo);
	virtual PFNLVCOMPARE GetCompareFunc();
	virtual void AllItemsDeleted();
	virtual DROPEFFECT DoDragDrop(int *pnRows,COleDataSource *pOleDataSource);
	virtual bool EndLabelEdit(int nRow,int nCol,LPCTSTR pszText);
	//}}AFX_VIRTUAL
// Implementation
public:
	virtual ~CIEShellListCtrl();

// Generated message map functions
protected:
	//{{AFX_MSG(CIEShellListCtrl)
			// NOTE - the ClassWizard will add and remove member functions here.
	afx_msg void OnDestroy();
	afx_msg void OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu);
	afx_msg LRESULT OnAppFileChangeNewPath(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAppFileChangeEvent(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAppUpdateAllViews(WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnSettingChange(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnSetmessagestring(WPARAM wParam, LPARAM lParam );
	//}}AFX_MSG
	afx_msg LRESULT OnAppDeleteKey(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAppPropertiesKey(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
private:
	static UINT ThreadFunc (LPVOID pParam);
	enum { MAX_THREADS=1 };

	vecListItemData m_vecItemData;
	CShellPidl m_ShellPidl;
	TVITEMDATA m_tvid;
	LPSHELLFOLDER m_psfSubFolder;
	LPITEMIDLIST m_pidlInternet;
	IMalloc *m_pMalloc;
	HANDLE m_hThreads[MAX_THREADS];
	CWinThread *m_pThreads[MAX_THREADS];
	CEvent m_event[MAX_THREADS];
	CEvent m_MonitorEvent[MAX_THREADS];
	CString m_sMonitorPath;
	CIEShellDragDrop m_ShellDragDrop;
	CShellDetails m_ShellDetails;
	CShellSettings m_ShellSettings;
	CString m_sColumns;
	CString m_sFileFilter;
	CStringList m_FilterLookup;
	CStringList m_ExcludedLookup;
	int m_nThreadCount;
	bool m_bCallBack;
	bool m_bNoExt;
	bool m_bRefreshAllowed;
	bool m_bPopulateInit;
	bool m_bInitiliazed;
	bool m_bNotifyParent;
};
/////////////////////////////////////////////////////////////////////////////
typedef struct FC_THREADINFO
{
    HANDLE hEvent;
	HANDLE hMonitorEvent;
    CIEShellListCtrl *pListCtrl;
} FC_THREADINFO, *PFC_THREADINFO;

inline const CShellSettings &CIEShellListCtrl::GetShellSettings() const
{
	return m_ShellSettings;
}

inline CShellSettings &CIEShellListCtrl::GetShellSettings()
{
	return m_ShellSettings;
}

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif //__IESHELLLISTCTRL_H__

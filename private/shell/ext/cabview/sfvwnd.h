//*******************************************************************************************
//
// Filename : SFVWnd.h
//	
//				Definitions of CListView, CSFViewDlg, CAccelerator, CSFView
//
// Copyright (c) 1994 - 1996 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************


#ifndef _SFVWnd_H_
#define _SFVWnd_H_

#include "ThisDll.H"

#include "SFView.H"

#include "XIcon.H"
#include "Dlg.H"
#include "Unknown.H"

#define IDC_ARRANGE_BY (FCIDM_SHVIEWFIRST + 0x100)
#define MAX_COL 0x20

#define SFV_CONTEXT_FIRST (FCIDM_SHVIEWFIRST + 0x1000)
#define SFV_CONTEXT_LAST (FCIDM_SHVIEWFIRST + 0x2000)

BOOL StrRetToStr(LPSTR szOut, UINT uszOut, LPSTRRET pStrRet, LPCITEMIDLIST pidl);

class CListView
{
public:
	CListView() {}
	~CListView() {}

	operator HWND() const {return(m_hwndList);}

	void Init(HWND hwndList, HWND hwndLB, UINT idiDef)
	{
		m_hwndList = hwndList;

		m_cxi.Init(hwndLB, idiDef);

		ListView_SetImageList(hwndList, m_cxi.GetIML(TRUE), LVSIL_NORMAL);
		ListView_SetImageList(hwndList, m_cxi.GetIML(FALSE), LVSIL_SMALL);
	}

	int InsertItem(LV_ITEM *pItem)
	{
		return(ListView_InsertItem(m_hwndList, pItem));
	}

	void DeleteAllItems() {ListView_DeleteAllItems(m_hwndList);}

	enum
	{
		AI_LARGE = CXIcon::AI_LARGE,
		AI_SMALL = CXIcon::AI_SMALL,
	} ;

	int GetIcon(IShellFolder *psf, LPCITEMIDLIST pidl)
	{
		return(m_cxi.GetIcon(psf, pidl));
	}

private:
	HWND m_hwndList;
	CXIcon m_cxi;
} ;


class CSFViewDlg : public CDlg
{
public:
	CSFViewDlg(class CSFView *psfv) : m_psfv(psfv), m_hrOLE(E_UNEXPECTED)
	{
		m_hDlg = NULL;
	}
	~CSFViewDlg() {}

	operator HWND() const {return(m_hDlg);}

	int AddObject(LPCITEMIDLIST pidl);
	void DeleteAllItems() {m_cList.DeleteAllItems();}

	BOOL DestroyWindow() {BOOL bRet=::DestroyWindow(m_hDlg); m_hDlg = NULL; return(bRet);}

	void SetStyle(DWORD dwAdd, DWORD dwRemove)
	{
		SetWindowLong(m_cList, GWL_STYLE, dwAdd |
			(GetWindowStyle(m_cList) & ~dwRemove));
	}

	void SelAll()
	{
		ListView_SetItemState(m_cList, -1, LVIS_SELECTED, LVIS_SELECTED);
	}

	void InvSel()
	{
		int iItem = -1;
		while ((iItem=ListView_GetNextItem(m_cList, iItem, 0)) != -1)
		{
			UINT flag;

			// flip the selection bit on each item
			flag = ListView_GetItemState(m_cList, iItem, LVIS_SELECTED);
			flag ^= LVNI_SELECTED;
			ListView_SetItemState(m_cList, iItem, flag, LVIS_SELECTED);
		}
	}

	UINT CharWidth();

	BOOL GetColumn(int i, LV_COLUMN *pcol) {return(ListView_GetColumn(m_cList, i, pcol));}
	BOOL SetColumn(int i, LV_COLUMN *pcol) {return(ListView_SetColumn(m_cList, i, pcol));}
	UINT InsertColumn(int i, LV_COLUMN *pcol) {return(ListView_InsertColumn(m_cList, i, pcol));}

	void SortItems(PFNDPACOMPARE pfnCmp) {ListView_SortItems(m_cList, pfnCmp, m_psfv);}

	static BOOL IsMenuSeparator(HMENU hm, int i);

	HRESULT GetUIObjectFromItem(REFIID riid, LPVOID * ppv, UINT uItem);
	HRESULT GetAttributesFromItem(ULONG *pdwAttr, UINT uItem);

	BOOL OleInited() {return(SUCCEEDED(m_hrOLE));}

private:
	virtual BOOL RealDlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

	UINT GetItemPIDLS(LPCITEMIDLIST apidl[], UINT cItemMax, UINT uItem);
	HRESULT GetItemObjects(LPCITEMIDLIST **ppidl, UINT uItem);

	LPCITEMIDLIST GetPIDL(int iItem);

	void InitDialog();
	LRESULT BeginDrag();
	BOOL Notify(LPNMHDR pNotify);
    void ContextMenu(DWORD dwPos, BOOL bDoDefault=FALSE);

			
	CListView m_cList;
	HRESULT m_hrOLE;

	class CSFView *m_psfv;
} ;


struct SFSTATE
{
	LPARAM lParamSort;
} ;


class CAccelerator
{
public:
	CAccelerator(UINT uID)
	{
		m_hAccel = LoadAccelerators(g_ThisDll.GetInstance(), MAKEINTRESOURCE(uID));
	}

	int TranslateAccelerator(HWND hwnd, LPMSG pmsg)
	{
		if (!m_hAccel)
		{
			return(FALSE);
		}

		return(::TranslateAccelerator(hwnd, m_hAccel, pmsg));
	}

private:
	HACCEL m_hAccel;
} ;

//
// CSFView - IShellView implementation
//

class CSFView : public CUnknown, public IShellView
{
public:
	CSFView(LPSHELLFOLDER psf, IShellFolderViewCallback *psfvcb);
	virtual ~CSFView();

	STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// *** IOleWindow methods ***
	STDMETHODIMP GetWindow(HWND * lphwnd);
	STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);

	// *** IShellView methods ***
	STDMETHODIMP TranslateAccelerator(LPMSG lpmsg);
	STDMETHODIMP EnableModeless(BOOL fEnable);
	STDMETHODIMP UIActivate(UINT uState);
	STDMETHODIMP Refresh();

	STDMETHODIMP CreateViewWindow(IShellView  *lpPrevView,
	                LPCFOLDERSETTINGS lpfs, IShellBrowser  * psb,
	                RECT * prcView, HWND  *phWnd);
	STDMETHODIMP DestroyViewWindow();
	STDMETHODIMP GetCurrentInfo(LPFOLDERSETTINGS lpfs);
	STDMETHODIMP AddPropertySheetPages(DWORD dwReserved,
	                LPFNADDPROPSHEETPAGE lpfn, LPARAM lparam);
	STDMETHODIMP SaveViewState();
	STDMETHODIMP SelectItem(LPCITEMIDLIST pidlItem, UINT uFlags);
	STDMETHODIMP GetItemObject(UINT uItem, REFIID riid,
	                LPVOID *ppv);

private:
	static int CALLBACK CSFView::CompareIDs(LPVOID p1, LPVOID p2, LPARAM lParam);

	void AddColumns();
	BOOL SaveColumns(LPSTREAM pstm);
	void RestoreColumns(LPSTREAM pstm, int nCols);
	void RestoreViewState();

	void ColumnClick(int iCol)
	{
		m_sfState.lParamSort = (LPARAM)DPA_GetPtr(m_aParamSort, iCol);
        m_cView.SortItems(CompareIDs);
	}

	HRESULT CallCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return(m_psfvcb ? m_psfvcb->Message(uMsg, wParam, lParam) : E_NOTIMPL);
	}

	int GetMenuIDFromViewMode();
	BOOL IsInCommDlg() {return(m_pCDB != NULL);}
	HRESULT IncludeObject(LPCITEMIDLIST pidl)
	{
		return(IsInCommDlg() ? m_pCDB->IncludeObject(this, pidl) : S_OK);
	}
	HRESULT OnDefaultCommand()
	{
		return(IsInCommDlg() ? m_pCDB->OnDefaultCommand(this) : S_FALSE);
	}
	HRESULT OnStateChange(UINT uFlags)
	{
		return(IsInCommDlg() ? m_pCDB->OnStateChange(this, uFlags) : S_FALSE);
	}

	void InitFileMenu(HMENU hmInit);
	void InitEditMenu(HMENU hmInit);
	void InitViewMenu(HMENU hmInit);
	int AddObject(LPCITEMIDLIST pidl);

	HRESULT FillList(BOOL bInteractive);
	BOOL ShowAllObjects() {return(TRUE);}

	void MergeArrangeMenu(HMENU hmView);
	void MergeViewMenu(HMENU hmenu, HMENU hmMerge);
	BOOL OnActivate(UINT uState);
	BOOL OnDeactivate();

	IContextMenu * GetSelContextMenu();
	void ReleaseSelContextMenu();

	BOOL OnInitMenuPopup(HMENU hmInit, int nIndex, BOOL fSystemMenu);
	void OnCommand(IContextMenu *pcm, WPARAM wParam, LPARAM lParam);

	void CheckToolbar();
	void MergeToolBar();

	BOOL GetArrangeText(int iCol, UINT idFmt, LPSTR pszText, UINT cText);
	void GetCommandHelpText(UINT id, LPSTR pszText, UINT cchText, BOOL bToolTip);
	LRESULT OnMenuSelect(UINT idCmd, UINT uFlags, HMENU hmenu);


	LPSHELLFOLDER m_psf;                       // ShellFolder pointer
	ICommDlgBrowser *m_pCDB;                   // ICommdlgBrowser
	IShellFolderViewCallback *m_psfvcb;        // pointer to ShellFolderView 
	                                           // callback
	CEnsureRelease m_erFolder; 
	CEnsureRelease m_erCB;

	CSFViewDlg m_cView;                        //  ViewDlg which contains the
	                                           //  listview in the right pane
	HWND m_hwndMain;

	FOLDERSETTINGS m_fs;
	IShellBrowser *m_psb;
	SFSTATE m_sfState;

	CMenuTemp m_cmCur;
	UINT m_uState;

	IContextMenu *m_pcmSel;

	HDPA m_aParamSort;                         // maintains a sorted list of 
	                                           // items in a DPA

	CAccelerator m_cAccel;

    CSafeMalloc m_cMalloc;

	friend class CSFViewDlg;
} ;

#endif // _SFVWnd_H_

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

// IEShellTreeCtrl.cpp : implementation file
#include "stdafx.h"
#include "IEShellTreeCtrl.h"
#include "cbformats.h"
#include "UIMessages.h"
#include "dirwalk.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CIEShellTreeCtrl

CIEShellTreeCtrl::CIEShellTreeCtrl()
{
	m_lptvid = NULL;
	m_hListWnd = NULL;
	m_hComboWnd = NULL;
	m_nThreadCount = 0;
	m_bRefreshAllowed = true;
	m_bNotifyParent = false;
	// Turn off WM_DROPFILES
	SetDropFiles(false);
}

CIEShellTreeCtrl::~CIEShellTreeCtrl()
{
}

void CIEShellTreeCtrl::ShellExecute(HTREEITEM hItem,LPCTSTR pszVerb)
{
	SHELLEXECUTEINFO si;
	ZeroMemory(&si,sizeof(si));
	si.cbSize = sizeof(si);
	si.hwnd = GetSafeHwnd();
	si.nShow = SW_SHOW;
	si.lpIDList = (LPVOID)GetPathPidl(hItem);
	si.fMask  = SEE_MASK_INVOKEIDLIST;
	if (pszVerb)
		si.lpVerb = pszVerb;
	ShellExecuteEx(&si);
}

void CIEShellTreeCtrl::RefreshComboBox(LPTVITEMDATA lptvid)
{
	if (m_hComboWnd)
	{
		::PostMessage(m_hComboWnd,WM_APP_CB_IE_POPULATE,(WPARAM)lptvid->lpifq,0);
	}
}

void CIEShellTreeCtrl::SetNotificationObject(bool bNotify)
{
	if (bNotify)
		CreateFileChangeThreads(GetSafeHwnd());
	else
		DestroyThreads();
}

void CIEShellTreeCtrl::UpOneLevel(HTREEITEM hItem)
{
	if (hItem == NULL)
	{
		hItem = GetSelectedItem();
	}
	if (hItem == NULL)
		return;
	HTREEITEM hParentItem = GetParentItem(hItem);
	if (hParentItem)
		Select(hParentItem,TVGN_CARET);
}

void CIEShellTreeCtrl::DestroyThreads()
{
    if (m_nThreadCount == 0) 
		return;
    for (UINT i=0;i < m_nThreadCount; i++)
	    m_event[i].SetEvent();
    ::WaitForMultipleObjects (m_nThreadCount, m_hThreads, TRUE, INFINITE);
    for (i=0; i < m_nThreadCount; i++)
        delete m_pThreads[i];
    m_nThreadCount = 0;
}

void CIEShellTreeCtrl::CreateFileChangeThreads(HWND hwnd)
{
	if (m_nThreadCount)
		return;
	TCHAR szDrives[MAX_PATH];
	DWORD dwSize = sizeof(szDrives)/sizeof(TCHAR);
	DWORD dwChars = GetLogicalDriveStrings(dwSize,szDrives);
	if (dwChars == 0 || dwChars > dwSize) 
	{
		TRACE(_T("Warning: CreateFileChangeThreads failed in GetLogicalDriveStrings\n"));
		return;
	}
	UINT nType;
	CString sDrive;
	LPCTSTR pszDrives=szDrives;
	while (*pszDrives != '\0')
	{
		sDrive = pszDrives;
		nType = ::GetDriveType(sDrive);
		if (nType == DRIVE_FIXED || nType == DRIVE_REMOTE || nType == DRIVE_RAMDISK)
		{
			CreateFileChangeThread(sDrive,hwnd);
		}
#if 1 // bugfix by mad79
		pszDrives=pszDrives+sDrive.GetLength()+1;
#else
		pszDrives = _tcsninc(pszDrives,sDrive.GetLength()+1);
#endif
	}
}

void CIEShellTreeCtrl::CreateFileChangeThread(const CString& sPath,HWND hwnd)
{
	if (m_nThreadCount >= MAX_THREADS)
		return;
    PDC_THREADINFO pThreadInfo = new DC_THREADINFO; // Thread will delete
    pThreadInfo->sPath = sPath;
    pThreadInfo->hEvent = m_event[m_nThreadCount].m_hObject;
    pThreadInfo->pTreeCtrl = this;

    CWinThread* pThread = AfxBeginThread (ThreadFunc, pThreadInfo,
        THREAD_PRIORITY_IDLE);

    pThread->m_bAutoDelete = FALSE;
    m_hThreads[m_nThreadCount] = pThread->m_hThread;
    m_pThreads[m_nThreadCount++] = pThread;
}

HTREEITEM CIEShellTreeCtrl::SearchSiblings(HTREEITEM hItem,LPITEMIDLIST pidlAbs)
{
	LPTVITEMDATA pItem = NULL;
	HTREEITEM hChildItem = GetChildItem(hItem);
	HTREEITEM hFoundItem;
	while (hChildItem) 
	{
			pItem = (LPTVITEMDATA)GetItemData(hChildItem);
			if (GetShellPidl().ComparePidls(NULL,pItem->lpifq,pidlAbs))
			   break;
			hFoundItem = SearchSiblings(hChildItem,pidlAbs);
			if (hFoundItem)
				return hFoundItem;
			hChildItem = GetNextSiblingItem(hChildItem);
	}
	return hChildItem;
}

HTREEITEM CIEShellTreeCtrl::ExpandMyComputer(LPITEMIDLIST pidlAbs)
{
	HTREEITEM hItem=NULL;
	if (pidlAbs == NULL)
		return hItem;
	LPITEMIDLIST pidlMyComputer=NULL;
	LPITEMIDLIST pidlFirst=GetShellPidl().CopyItemID(pidlAbs);
    SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidlMyComputer); 
	if (GetShellPidl().ComparePidls(NULL,pidlMyComputer,pidlFirst))
	{
		hItem = ExpandPidl(pidlMyComputer);
	}
	if (pidlMyComputer)
		GetShellPidl().FreePidl(pidlMyComputer);
	if (pidlFirst)
		GetShellPidl().FreePidl(pidlFirst);
	return hItem;
}

HTREEITEM CIEShellTreeCtrl::ExpandPidl(LPITEMIDLIST pidlAbs)
{
	HTREEITEM hItem = SearchSiblings(GetRootItem(),pidlAbs);
	if (hItem)
	{
		Expand(hItem,TVE_EXPAND);
	}
	return hItem;
}

HTREEITEM CIEShellTreeCtrl::FindPidl(LPITEMIDLIST pidlAbs,BOOL bSelect)
{
	HTREEITEM hItem = NULL;
	if (pidlAbs == NULL)
		hItem = GetRootItem();
	else
		hItem = SearchSiblings(GetRootItem(),pidlAbs);
	if (bSelect && hItem != GetSelectedItem())
	{
		SelectItem(hItem);
		SelectionChanged(hItem,GetItemData(hItem));
	}
	return hItem;
}

HTREEITEM CIEShellTreeCtrl::FindItem (HTREEITEM hItem, const CString& strTarget)
{
    while (hItem != NULL) 
	{
        if (GetItemText (hItem) == strTarget)
            break;
        hItem = GetNextSiblingItem (hItem);
    }
    return hItem;
}

UINT CIEShellTreeCtrl::DeleteChildren (HTREEITEM hItem)
{
    UINT nCount = 0;
    HTREEITEM hChild = GetChildItem (hItem);

    while (hChild != NULL)
	{
        HTREEITEM hNextItem = GetNextSiblingItem (hChild);
        DeleteItem (hChild);
        hChild = hNextItem;
        nCount++;
    }
    return nCount;
}

void CIEShellTreeCtrl::Init()
{
	ModifyStyle(0,TVS_EDITLABELS);
	CIEFolderTreeCtrl::Init();
}

void CIEShellTreeCtrl::Refresh()
{
	SetRefreshAllowed(false);
	CIEFolderTreeCtrl::Refresh();
	SetRefreshAllowed(true);
}

bool CIEShellTreeCtrl::DragEnter(CDD_OleDropTargetInfo *pInfo)
{
	HTREEITEM hItem = pInfo->GetTreeItem(); 
	if (hItem == NULL)
		return false;
	LPTVITEMDATA ptvid = (LPTVITEMDATA)GetItemData(hItem);
	ASSERT(ptvid);
	if (ptvid == NULL)
		return false;
	return m_ShellDragDrop.DragEnter(pInfo,ptvid->lpsfParent,ptvid->lpi);
}

bool CIEShellTreeCtrl::DragLeave(CDD_OleDropTargetInfo *pInfo)
{
	return m_ShellDragDrop.DragLeave(pInfo);
}

bool CIEShellTreeCtrl::DragOver(CDD_OleDropTargetInfo *pInfo)
{
	pInfo->SetDropEffect(DROPEFFECT_NONE);

	HTREEITEM hItem = pInfo->GetTreeItem(); 
	if (hItem == NULL)
		return false;
	LPTVITEMDATA ptvid = (LPTVITEMDATA)GetItemData(hItem);
	ASSERT(ptvid);
	if (ptvid == NULL)
		return false;
	return m_ShellDragDrop.DragOver(pInfo,ptvid->lpsfParent,ptvid->lpi);
}

bool CIEShellTreeCtrl::DragDrop(CDD_OleDropTargetInfo *pInfo)
{
	HTREEITEM hItem = pInfo->GetTreeItem(); 
	if (hItem == NULL)
		return false;
	LPTVITEMDATA ptvid = (LPTVITEMDATA)GetItemData(hItem);
	ASSERT(ptvid);
	if (ptvid == NULL)
		return false;
	return m_ShellDragDrop.DragDrop(pInfo,ptvid->lpsfParent,ptvid->lpi);
}

DROPEFFECT CIEShellTreeCtrl::DoDragDrop(NM_TREEVIEW* pNMTreeView,COleDataSource *pOleDataSource)
{
	if (pNMTreeView->itemNew.hItem == GetRootItem())
		return DROPEFFECT_NONE;
	CCF_ShellIDList sl;
	CShellPidl pidl;
	HTREEITEM hParentItem = GetParentItem(pNMTreeView->itemNew.hItem);
	LPTVITEMDATA ptvid = (LPTVITEMDATA)GetItemData(pNMTreeView->itemNew.hItem);
	LPTVITEMDATA ptvid_parent = (LPTVITEMDATA)GetItemData(hParentItem);
	ASSERT(ptvid);
	ASSERT(ptvid_parent);
	if (GetShellPidl().IsDesktopFolder(ptvid->lpsfParent))
		sl.AddPidl(GetShellPidl().GetEmptyPidl());
	else
		sl.AddPidl(ptvid_parent->lpifq);
	sl.AddPidl(ptvid->lpi);
	CCF_HDROP cf_hdrop;
	CCF_String cf_text;
	CString sPath;
	pidl.SHPidlToPathEx(ptvid->lpifq,sPath);
	cf_hdrop.AddDropPoint(CPoint(pNMTreeView->ptDrag),FALSE);
	cf_hdrop.AddFileName(sPath);
	sPath += _T("\r\n");
	cf_text.SetString(sPath);
	CWDClipboardData::Instance()->SetData(pOleDataSource,&cf_text,CWDClipboardData::e_cfString);
	CWDClipboardData::Instance()->SetData(pOleDataSource,&cf_hdrop,CWDClipboardData::e_cfHDROP);
	CWDClipboardData::Instance()->SetData(pOleDataSource,&sl,CWDClipboardData::e_cfShellIDList);
	return GetShellPidl().GetDragDropAttributes(ptvid);
}

bool CIEShellTreeCtrl::EndLabelEdit(HTREEITEM hItem,LPCTSTR pszText)
{
	LPTVITEMDATA plvit = (LPTVITEMDATA)GetItemData(hItem);
	CString sFromPath;
	CString sToPath;
	GetShellPidl().SHPidlToPathEx(plvit->lpifq,sFromPath);
	sToPath = sFromPath;
	sToPath.Replace(GetItemText(hItem),pszText);
	SHFILEOPSTRUCT shf;
	TCHAR szFrom[MAX_PATH+1];
	TCHAR szTo[MAX_PATH+1];
	ZeroMemory(szFrom,sizeof(szFrom));
	lstrcpy(szFrom,sFromPath);
	ZeroMemory(szTo,sizeof(szTo));
	lstrcpy(szTo,sToPath);
	ZeroMemory(&shf,sizeof(shf));
	shf.hwnd = GetSafeHwnd();
	shf.wFunc = FO_RENAME;
	shf.pFrom = szFrom;
	shf.pTo = szTo;
#ifdef _DEBUG
	CString sMess;
	sMess = szFrom;
	sMess += _T("\n");
	sMess += szTo;
	AfxMessageBox(sMess);
#endif
	if (SHFileOperation(&shf) == 0)
		return true;
	SetRefreshAllowed(false);
	return false;
}

bool CIEShellTreeCtrl::SHMoveFile(HTREEITEM hSrcItem,HTREEITEM hDestItem)
{
	LPTVITEMDATA plvit_src = (LPTVITEMDATA)GetItemData(hSrcItem);
	LPTVITEMDATA plvit_dest = (LPTVITEMDATA)GetItemData(hDestItem);
	CString sFromPath;
	CString sToPath;
	GetShellPidl().SHPidlToPathEx(plvit_src->lpifq,sFromPath);
	GetShellPidl().SHPidlToPathEx(plvit_dest->lpifq,sToPath);
	SHFILEOPSTRUCT shf;
	TCHAR szFrom[MAX_PATH+1];
	TCHAR szTo[MAX_PATH+1];
	ZeroMemory(szFrom,sizeof(szFrom));
	lstrcpy(szFrom,sFromPath);
	ZeroMemory(szTo,sizeof(szTo));
	lstrcpy(szTo,sToPath);
	ZeroMemory(&shf,sizeof(shf));
	shf.hwnd = GetSafeHwnd();
	shf.wFunc = FO_MOVE;
	shf.pFrom = szFrom;
	shf.pTo = szTo;
	return SHFileOperation(&shf) == 0 ? true : false;
}

BOOL CIEShellTreeCtrl::TransferItem(HTREEITEM hitemDrag, HTREEITEM hitemDrop)
{
	return SHMoveFile(hitemDrag,hitemDrop) ? TRUE : FALSE;
}

bool CIEShellTreeCtrl::GetFolderInfo(HTREEITEM hItem,CString &sPath,CString &sName)
{
	LPTVITEMDATA lptvid = (LPTVITEMDATA)GetItemData(hItem);
	if (lptvid == NULL)
		return false;
	GetShellPidl().SHPidlToPathEx(lptvid->lpifq,sPath,NULL,SHGDN_NORMAL);
	GetShellPidl().GetDisplayName(lptvid->lpi,sName);
	return true;
}

bool CIEShellTreeCtrl::LoadFolderItems(LPCTSTR pszPath)
{
	return LoadItems(pszPath);
}

CRefresh *CIEShellTreeCtrl::CreateRefreshObject(HTREEITEM hItem,LPARAM lParam)
{
	CRefreshShellFolder *pRefresh=NULL;
	if (lParam)
	{
		pRefresh = new CRefreshShellFolder(hItem,lParam);
	}
	return pRefresh;
}

bool CIEShellTreeCtrl::Expanding(NM_TREEVIEW *nmtvw)
{
	if ((nmtvw->itemNew.state & TVIS_EXPANDEDONCE) || nmtvw->itemNew.hItem == GetRootItem())
		return false;

	LPSHELLFOLDER  pFolder=NULL;
	CUIListCtrlData *pData = (CUIListCtrlData*)nmtvw->itemNew.lParam;
	ASSERT(pData);
	ASSERT_KINDOF(CUIListCtrlData,pData);
	LPTVITEMDATA lptvid=(LPTVITEMDATA)pData->GetExtData();
	if (lptvid)
	{
		HRESULT hr=lptvid->lpsfParent->BindToObject(lptvid->lpi,
			0, IID_IShellFolder,(LPVOID*)&pFolder);

		if (SUCCEEDED(hr))
		{
			if (!AddItems(nmtvw->itemNew.hItem,pFolder))
				SetButtonState(nmtvw->itemNew.hItem);
		}
		return false;
	}
	// prevent from expanding
	return true;
}

BEGIN_MESSAGE_MAP(CIEShellTreeCtrl, CIEFolderTreeCtrl)
	//{{AFX_MSG_MAP(CIEShellTreeCtrl)
	ON_MESSAGE(WM_SETMESSAGESTRING,OnSetmessagestring)
	ON_MESSAGE(WM_APP_CB_IE_HIT_ENTER,OnAppCbIeHitEnter)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_APP_POPULATE_TREE,OnAppPopulateTree)
	ON_MESSAGE(WM_APP_CB_IE_SEL_CHANGE,OnCBIESelChange)
	ON_MESSAGE(WM_APP_DIR_CHANGE_EVENT,OnAppDirChangeEvent)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIEShellTreeCtrl message handlers
BOOL CIEShellTreeCtrl::OnWndMsg( UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult )
{
	if ((message == WM_MEASUREITEM || message == WM_DRAWITEM && wParam == 0) || message == WM_INITMENUPOPUP)
	{
		if (m_lptvid)
		{
			return GetShellPidl().HandleMenuMsg(m_hWnd,m_lptvid->lpsfParent, m_lptvid->lpi,
									message,wParam,lParam);
		}
	}
	return CIEFolderTreeCtrl::OnWndMsg(message, wParam, lParam, pResult );
}

void CIEShellTreeCtrl::PreSubclassWindow()
{
	CIEFolderTreeCtrl::PreSubclassWindow();
	CreateFileChangeThreads(GetSafeHwnd());
}

/////////////////////////////////////////////////////////////////////////
// Thread function for detecting file system changes
UINT CIEShellTreeCtrl::ThreadFunc (LPVOID pParam)
{
	///////////////////////////////
    PDC_THREADINFO pThreadInfo = (PDC_THREADINFO) pParam;
    HANDLE hEvent = pThreadInfo->hEvent;
	CIEShellTreeCtrl *pTreeCtrl = pThreadInfo->pTreeCtrl;
    HWND hWnd = pTreeCtrl->GetSafeHwnd();
	TCHAR szPath[MAX_PATH];
	lstrcpy(szPath,pThreadInfo->sPath);
    delete pThreadInfo;
    ////////////////////////////////////

    // Get a handle to a file change notification object.
	TRACE(_T("Creating directory thread handler for %s\n"),szPath);
    HANDLE hDirChange = ::FindFirstChangeNotification (szPath,TRUE,FILE_NOTIFY_CHANGE_DIR_NAME);

    // Return now if ::FindFirstChangeNotification failed.
    if (hDirChange == INVALID_HANDLE_VALUE)
        return 1;
	const int nHandles=2;
    HANDLE aHandles[nHandles];
    aHandles[0] = hDirChange;
    aHandles[1] = hEvent;
    BOOL bContinue = TRUE;

    // Sleep until a file change notification wakes this thread or
    // m_event becomes set indicating it's time for the thread to end.
    while (bContinue)
	{
		TRACE(_T("TreeControl waiting for %u multiple objects\n"),nHandles);
        DWORD dw = ::WaitForMultipleObjects (nHandles, aHandles, FALSE, INFINITE);
        if (dw - WAIT_OBJECT_0 == 0) 
		{ // Respond to a change notification.
            ::FindNextChangeNotification (hDirChange);
			TRACE(_T("-- Directory notify event was fired in CIEShellTreeCtrl --\n"));
			if (pTreeCtrl->RefreshAllowed())
			{
				::PostMessage (hWnd, WM_APP_DIR_CHANGE_EVENT,0,0);
			}
			else
			{
				TRACE(_T("but not sending as refresh disallowed\n"));
				pTreeCtrl->SetRefreshAllowed(true);
				TRACE(_T("Refresh is now allowed\n"));
			}
        }
        else if(dw - WAIT_OBJECT_0 == 1) 
		{
            bContinue = FALSE;
			TRACE(_T("Directory Notify Thread was signalled to stop\n"));
		}
    }

    // Close the file change notification handle and return.
    ::FindCloseChangeNotification (hDirChange);
	TRACE(_T("Directory Notify Thread is ending\n"));
    return 0;
}

void CIEShellTreeCtrl::OnDestroy()
{
	DestroyThreads();
	CIEFolderTreeCtrl::OnDestroy();
}

LRESULT CIEShellTreeCtrl::OnAppDirChangeEvent(WPARAM wParam, LPARAM lParam)
{
	if (!RefreshAllowed())
		return 1L;
	Refresh();
	if (m_bNotifyParent)
		GetParent()->SendMessage(WM_APP_UPDATE_ALL_VIEWS,(WPARAM)HINT_SHELL_DIR_CHANGED,(LPARAM)(LPCTSTR)GetRootPath());
	return 1L;
}

LRESULT CIEShellTreeCtrl::OnCBIESelChange(WPARAM wParam,LPARAM lParam)
{
	LPITEMIDLIST pidl = (LPITEMIDLIST)wParam;
	ExpandMyComputer(pidl);
	FindPidl(pidl);				
	GetShellPidl().FreePidl(pidl);
	SetFocus();
	return 1L;
}

// Selection has changed
void CIEShellTreeCtrl::UpdateEvent(LPARAM lHint,CObject *pHint)
{
	// TODO: Add your specialized code here and/or call the base class
	// Notify the combo box
	const CRefreshShellFolder *pRefresh = static_cast<CRefreshShellFolder*>(pHint);
	LPTVITEMDATA lptvid = reinterpret_cast<LPTVITEMDATA>(pRefresh->GetItemData());
	ASSERT(lptvid);
	// Notify combo box
	RefreshComboBox(lptvid);
	// Notify list control
	if (m_hListWnd)
	{
		::SendMessage(m_hListWnd,WM_APP_UPDATE_ALL_VIEWS,(WPARAM)lHint,(LPARAM)pHint);
		return;
	}
	// or let base class handle it
	CUITreeCtrl::UpdateEvent(lHint,pHint);
}

LRESULT CIEShellTreeCtrl::OnAppPopulateTree(WPARAM wParam, LPARAM lParam)
{
	TRACE0("Selecting root item\n");
	HTREEITEM hRoot = GetRootItem();
	if (hRoot == NULL)
		return 1L;	
	SelectItem(hRoot);
#ifndef _DEBUG
	SelectionChanged(hRoot,GetItemData(hRoot));
#else
	LPTVITEMDATA lptvid = reinterpret_cast<LPTVITEMDATA>(GetItemData(hRoot));
	RefreshComboBox(lptvid);
#endif
	return 1;
}

void CIEShellTreeCtrl::DeleteKey(HTREEITEM hItem)
{
	// TODO: Add your specialized code here and/or call the base class
	SHFILEOPSTRUCT shf;
	ZeroMemory(&shf,sizeof(shf));
	TCHAR szFrom[MAX_PATH+1];
	ZeroMemory(szFrom,sizeof(szFrom)*sizeof(TCHAR));
	lstrcpy(szFrom,GetPathName(hItem));
	shf.hwnd = GetSafeHwnd();
	shf.wFunc = FO_DELETE;
	shf.pFrom = szFrom;
	shf.fFlags = GetKeyState(VK_SHIFT) < 0 ? 0 : FOF_ALLOWUNDO;
	SHFileOperation(&shf);
}

void CIEShellTreeCtrl::DoubleClick(HTREEITEM hItem)
{
	// TODO: Add your specialized code here and/or call the base class
	ShellExecute(hItem);
}

void CIEShellTreeCtrl::GoBack(HTREEITEM hItem)
{
	// TODO: Add your specialized code here and/or call the base class
	UpOneLevel(hItem);
}

void CIEShellTreeCtrl::ShowPopupMenu(HTREEITEM hItem,CPoint point)
{
	// TODO: Add your specialized code here and/or call the base class
	if (m_PopupID)
	{
		CUITreeCtrl::ShowPopupMenu(hItem,point);
	}
	// TODO: Add your control notification handler code here
	m_lptvid = (LPTVITEMDATA)GetItemData(hItem);
	if (m_lptvid)
		GetShellPidl().PopupTheMenu(m_hWnd,m_lptvid->lpsfParent, &m_lptvid->lpi, 1, &point);
}

void CIEShellTreeCtrl::ShowProperties(HTREEITEM hItem)
{
	// TODO: Add your specialized code here and/or call the base class
	ShellExecute(hItem,_T("properties"));
}

LRESULT CIEShellTreeCtrl::OnAppCbIeHitEnter(WPARAM wParam, LPARAM lParam)
{
	if (lParam == NULL)
		return 0L;
	LPCTSTR pszPath = (LPCTSTR)lParam;
	LPITEMIDLIST pidl=NULL;
	GetShellPidl().SHPathToPidlEx(pszPath,&pidl,NULL);
	if (pidl == NULL)
		return 0L;
	int nCount = GetShellPidl().GetCount(pidl);
	LPITEMIDLIST pidlPart=NULL;
	LPITEMIDLIST pidlFull=NULL;
	HTREEITEM hItem=NULL;
	SetRedraw(FALSE);
	for(int i=0;i < nCount;i++)
	{
		pidlPart=GetShellPidl().CopyItemID(pidl,i+1);
		if (pidlPart)
		{
			pidlFull = GetShellPidl().ConcatPidl(pidlFull,pidlPart);
			hItem = ExpandPidl(pidlFull);
			if (hItem == NULL)
				break;
			GetShellPidl().FreePidl(pidlPart);
		}
	}
	if (hItem && GetShellPidl().ComparePidls(NULL,pidl,pidlFull))
	{
		Select(hItem,TVGN_CARET);
	}
	else
	{
		CString sMess;
		sMess.Format(_T("%s was not found"),pszPath);
		AfxMessageBox(sMess,MB_ICONSTOP);
	}
	if (pidl)
		GetShellPidl().FreePidl(pidl);
	if (pidlFull)
		GetShellPidl().FreePidl(pidlFull);
	SetRedraw(TRUE);
	return 1;
}

LRESULT CIEShellTreeCtrl::OnSetmessagestring(WPARAM wParam, LPARAM lParam)
{
	if (GetParent())
		return GetParent()->SendMessage(WM_SETMESSAGESTRING,wParam,lParam);
	return 0;
}

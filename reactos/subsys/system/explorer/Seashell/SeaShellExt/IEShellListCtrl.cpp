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

// IEShellListCtrl.cpp : implementation file
#include "stdafx.h"
#include "IEShellListCtrl.h"
#include "UIMessages.h"
#include "dirwalk.h"
#include "cbformats.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define UMAKEINT64(low,high) ((unsigned __int64)(((DWORD)(low)) | ((unsigned __int64)((DWORD)(high))) << 32))
/////////////////////////////////////////////////////////////////////////////
// CIEShellListCtrl

CIEShellListCtrl::CIEShellListCtrl()
{
	SetEditSubItems(false);
	SetDropFiles(false);
	ZeroMemory(&m_tvid,sizeof(m_tvid));
	m_psfSubFolder = NULL;
	m_pMalloc = NULL;
	m_bCallBack = false;
	m_bNoExt = false;
	m_nThreadCount = 0;
	m_bRefreshAllowed = true;
	m_bPopulateInit = false;
	m_bInitiliazed = false;
	m_bNotifyParent = false;
	m_pidlInternet = NULL;
	SHGetMalloc(&m_pMalloc);
	SHGetSpecialFolderLocation(NULL,CSIDL_INTERNET,&m_pidlInternet);
	m_sColumns = _T("Name|Size|Type|Modified");
}

CIEShellListCtrl::~CIEShellListCtrl()
{
	FreeTVID();
	AllItemsDeleted();
	if (m_pidlInternet)
		m_pMalloc->Release();
	if (m_pMalloc)
		m_pMalloc->Release();
}

void CIEShellListCtrl::PopupTheMenu(int nRow,CPoint point)
{
	if (!GetSelectedCount())
		return;
	int nSel=-1;
	LPLVITEMDATA lplvid=NULL;
	LPITEMIDLIST *pidls=(LPITEMIDLIST*)GetShellPidl().GetMalloc()->Alloc(GetSelectedCount()*sizeof(LPITEMIDLIST));
	int i=0;
	while ((nSel = GetNextSel(nSel)) != -1)
	{
		lplvid=(LPLVITEMDATA)GetItemData(nSel);
		pidls[i] = lplvid->lpi;
		i++;
	}
	// at least one
	if (pidls && i)
	{
		GetShellPidl().PopupTheMenu(m_hWnd, lplvid->lpsfParent, pidls, i, &point);
	}
	GetShellPidl().Free(pidls);
}

void CIEShellListCtrl::ShellExecute(int nRow,LPCTSTR pszVerb)
{
	SHELLEXECUTEINFO si;
	ZeroMemory(&si,sizeof(si));
	si.cbSize = sizeof(si);
	si.hwnd = GetSafeHwnd();
	si.nShow = SW_SHOW;
	si.lpIDList = (LPVOID)GetPathPidl(nRow);
	si.fMask  = SEE_MASK_INVOKEIDLIST;
	if (pszVerb)
		si.lpVerb = pszVerb;
	ShellExecuteEx(&si);
}

void CIEShellListCtrl::SetNotificationObject(bool bNotify)
{
	if (bNotify)
		CreateFileChangeThread(GetSafeHwnd());
	else
		DestroyThreads();
}

void CIEShellListCtrl::FreeInterface(IUnknown *pInterface)
{
	if (pInterface)
		pInterface->Release();
}

void CIEShellListCtrl::DestroyThreads()
{
    if (m_nThreadCount == 0) 
		return;
    for (UINT i=0; i<m_nThreadCount; i++)
	    m_event[i].SetEvent();
    ::WaitForMultipleObjects (m_nThreadCount, m_hThreads, TRUE, INFINITE);
    for (i=0; i<m_nThreadCount; i++)
        delete m_pThreads[i];
    m_nThreadCount = 0;
}

void CIEShellListCtrl::FreeTVID()
{
	if (m_tvid.lpsfParent)
		m_tvid.lpsfParent->Release();
	if (m_tvid.lpi)
		m_pMalloc->Free(m_tvid.lpi);
	if (m_tvid.lpifq)
		m_pMalloc->Free(m_tvid.lpifq);
	ZeroMemory(&m_tvid,sizeof(m_tvid));
	if (m_psfSubFolder)
		m_psfSubFolder->Release();
	m_psfSubFolder = NULL;
}

CString CIEShellListCtrl::GetCurrPathName()
{
	return GetPathName(GetCurSel());
}

CString CIEShellListCtrl::GetPathName(int nRow)
{
	if (nRow == -1)
		nRow = GetCurSel();
	CString sPath;
	if (nRow == -1)
		return sPath;
	LPLVITEMDATA plvit = (LPLVITEMDATA)GetItemData(nRow);
	ASSERT(plvit);
	if (plvit == NULL)
		return sPath;
	SHGetPathFromIDList(plvit->lpifq,sPath.GetBuffer(MAX_PATH));
	sPath.ReleaseBuffer();
	return sPath;
}

LPCITEMIDLIST CIEShellListCtrl::GetPathPidl(int nRow)
{
	if (nRow == -1)
		return NULL;
	LPLVITEMDATA plvit = (LPLVITEMDATA)GetItemData(nRow);
	ASSERT(plvit);
	if (plvit == NULL)
		return NULL;
	return plvit->lpifq;
}

void CIEShellListCtrl::FillExcludedFileTypes(CComboBox &cb)
{
	cb.ResetContent();
	for(POSITION pos=m_ExcludedLookup.GetHeadPosition();pos != NULL;m_ExcludedLookup.GetNext(pos))
	{
		cb.AddString(m_ExcludedLookup.GetAt(pos));
	}
}

void CIEShellListCtrl::SetExcludedFileTypes(CComboBox &cb)
{
	m_ExcludedLookup.RemoveAll();
	CString sText;
	for(int i=0;i < cb.GetCount();i++)
	{
		cb.GetLBText(i,sText);
		m_ExcludedLookup.AddHead(sText);
	}
}

void CIEShellListCtrl::LoadFilterFiles(const CString &sFileFilter)
{
	m_FilterLookup.RemoveAll();
	CFileFind ff;
	CString sPath;
	if (FAILED(GetShellPidl().SHPidlToPathEx(m_tvid.lpifq,sPath)))
		return;
	if (sPath.Right(1) != _T('\\'))
		sPath += _T('\\');
	CString sFindPath;
	CString sMask;
	LPCTSTR pszFilter=sFileFilter;
	pszFilter = GetFilterMask(pszFilter,sMask);
	while (pszFilter != NULL)
	{
		sFindPath = sPath;
		sFindPath += sMask;
		BOOL bFind = ff.FindFile(sFindPath);  
		while (bFind)
		{
			bFind = ff.FindNextFile();
			m_FilterLookup.AddHead(ff.GetFilePath());
		}
		pszFilter = GetFilterMask(pszFilter,sMask);
	}
}

LPCTSTR CIEShellListCtrl::GetFilterMask(LPCTSTR pszFilter,CString &sMask)
{
	if (*pszFilter == '\0')
		return NULL;
	TCHAR szMask[MAX_PATH];
	szMask[0] = 0;
	for(int i=0;*pszFilter != '\0';i++)
	{
		if (*pszFilter == _T(';') || *pszFilter == _T(','))
		{
			pszFilter = _tcsinc(pszFilter);
			break;
		}
		szMask[i] = *pszFilter;
		pszFilter = _tcsinc(pszFilter);
	}
	szMask[i] = 0;
	sMask = szMask;
	return pszFilter;
}

void CIEShellListCtrl::AllItemsDeleted()
{
	LPLVITEMDATA pItemData=NULL;
	for(vecListItemData::iterator it=m_vecItemData.begin();it != m_vecItemData.end();it++)
	{
		pItemData = *it;
		if (pItemData)
		{
			if (pItemData->lpsfParent)
				pItemData->lpsfParent->Release();
			if (pItemData->lpi)
				m_pMalloc->Free(pItemData->lpi);  
			if (pItemData->lpifq)
				m_pMalloc->Free(pItemData->lpifq);  
			if (pItemData->lParam)
			{
				PSLC_COLUMN_DATA pColData = (PSLC_COLUMN_DATA)pItemData->lParam;
				if (pColData->pidl)
					GetShellPidl().FreePidl(pColData->pidl);
				delete pColData;
			}
			m_pMalloc->Free(pItemData);
		}
	}
	m_vecItemData.erase(m_vecItemData.begin(),m_vecItemData.end());
}

void CIEShellListCtrl::StartPopulate()
{
	m_bPopulateInit = true;
	DeleteAllItems();
	for(UINT i=0;i < m_nThreadCount;i++)
		m_MonitorEvent[i].SetEvent();
	if (m_sFileFilter.IsEmpty() || m_sFileFilter == _T("*.*") || m_sFileFilter == _T("*"))
		return;
	LoadFilterFiles(m_sFileFilter);
}

void CIEShellListCtrl::EndPopulate()
{
	m_bPopulateInit = false;
	Sort();
	SetColumnWidths();
}

BOOL CIEShellListCtrl::Populate(LPTVITEMDATA lptvid)
{
	IShellFolder *pFolder=NULL;
	BOOL bRet=FALSE;
	HRESULT hr=E_FAIL;
	if (lptvid->lpsfParent)
	{
		hr=lptvid->lpsfParent->BindToObject(lptvid->lpi,0,IID_IShellFolder,(LPVOID*)&pFolder);
	}
	else
	{
		LPSHELLFOLDER psfDesktop;
		LPITEMIDLIST pidlDesktop=NULL;
		hr=SHGetDesktopFolder(&psfDesktop);
		SHGetSpecialFolderLocation(NULL,CSIDL_DESKTOP,&pidlDesktop);
		if (GetShellPidl().ComparePidls(NULL,lptvid->lpifq,pidlDesktop))
		{
			pFolder = psfDesktop;
		}
		else
		{
			hr=psfDesktop->BindToObject(lptvid->lpifq,0,IID_IShellFolder,(LPVOID*)&pFolder);
			psfDesktop->Release();
		}
		if (pidlDesktop)
			GetShellPidl().FreePidl(pidlDesktop);
	}
	if (SUCCEEDED(hr))
	{
		bRet = Populate(lptvid,pFolder,true);
		pFolder->Release();
	}
	return bRet;
}

BOOL CIEShellListCtrl::Populate(LPCTSTR pszPath, bool bCallBack)
{
	LPITEMIDLIST pidlfq=NULL;
	LPITEMIDLIST pidl=NULL;
	LPSHELLFOLDER pDesktop = NULL;
    SHGetDesktopFolder(&pDesktop);
	if (FAILED(GetShellPidl().SHPathToPidlEx(pszPath,&pidlfq,pDesktop)))
		return FALSE;
	LPSHELLFOLDER pSubFolder = NULL;
#if 1 // bug fix by Dion Loy
	if (FAILED(pDesktop->BindToObject(pidlfq, 0, IID_IShellFolder,(LPVOID*)&pSubFolder)))
#else
	if (FAILED(pDesktop->BindToObject(pidl, 0, IID_IShellFolder,(LPVOID*)&pSubFolder)))
#endif
		return FALSE;
	pidl = GetShellPidl().CopyLastItemID(pidlfq);
	TVITEMDATA tvid;
	tvid.lpifq = pidlfq;
	tvid.lpi = pidl;
	tvid.lpsfParent = NULL;
	BOOL bRet = Populate(&tvid,pSubFolder,bCallBack);
	if (pDesktop)
		pDesktop->Release();
	if (pSubFolder)
		pSubFolder->Release();
	if (pidl)
		m_pMalloc->Free(pidl);
	if (pidlfq)
		m_pMalloc->Free(pidlfq);
		
	return bRet;
}

BOOL CIEShellListCtrl::Populate(LPTVITEMDATA lptvid,LPSHELLFOLDER psfFolder,bool bCallBack)
{
	m_bCallBack = bCallBack;
	FreeTVID();
	m_tvid.lpi = GetShellPidl().CopyItemIDList(lptvid->lpi);
	m_tvid.lpifq = GetShellPidl().CopyItemIDList(lptvid->lpifq);
#ifdef _DEBUG
	CString sPath;
	GetShellPidl().SHPidlToPathEx(m_tvid.lpifq,sPath,NULL);
	TRACE1("Populating path %s in CIEShellListCtrl\n",sPath);
#endif	
	if (lptvid->lpsfParent)
	{
		m_tvid.lpsfParent = lptvid->lpsfParent;
		m_tvid.lpsfParent->AddRef();
	}
	m_psfSubFolder = psfFolder;
	m_psfSubFolder->AddRef();
	Load();
    return TRUE;
}

CString CIEShellListCtrl::GetColumns()
{
	SHELLDETAILS sd;
	LPTSTR pszHeader=NULL;
	CString sColumns;
	for(int i=0;SUCCEEDED(m_ShellDetails.GetDetailsOf(NULL,i,&sd));i++)
	{
		GetShellPidl().StrRetToStr(sd.str,&pszHeader,NULL);
		if (pszHeader)
		{
			TRACE1("Column found %s\n",pszHeader);	
			if (!sColumns.IsEmpty())
				sColumns += _T("|");
			sColumns += pszHeader;
			GetShellPidl().Free(pszHeader);
			pszHeader= NULL;
		}
	}
	return sColumns;
}

void CIEShellListCtrl::InitColumns()
{
	CString sColumns = GetColumns();
	if (sColumns.IsEmpty())
		sColumns = m_sColumns;
	InitListCtrl(sColumns);
}

void CIEShellListCtrl::SetColumnWidths()
{
	if (!m_bCallBack)
	{
		for(int i=0;i < GetColumnCount();i++)
			SetColumnWidth(i,LVSCW_AUTOSIZE);
	}
}

void CIEShellListCtrl::Refresh()
{
	if (m_psfSubFolder==NULL)
		return;
	SetRefreshAllowed(false);
	SetRedraw(FALSE);
	int nCurSel = GetCurSel();
	TRACE(_T("Refreshing shell list control\n"));
	Load();
	SetCurSel(nCurSel);
	LONG Result;
	OnSelChanged(nCurSel,&Result);
	SetRedraw(TRUE);
	SetRefreshAllowed(true);
}

void CIEShellListCtrl::Load()
{
	CWaitCursor w;
    if (!InitItems(&m_tvid,m_bCallBack))
        return;
}

void CIEShellListCtrl::Init()
{
	if (m_bInitiliazed)
		return;
	CUIODListCtrl::Init();
	InitShellSettings();
	InitImageLists();
	m_bInitiliazed = true;
}

BOOL CIEShellListCtrl::InitItems(LPTVITEMDATA lptvid, bool bCallBack)
{
	CWaitCursor w;

	Init();

	ASSERT(m_psfSubFolder);
	if (m_psfSubFolder == NULL)
		return FALSE;

	// Try to initialize columns from shell
	m_ShellDetails.SetShellDetails(NULL);
	IShellFolder2 *pShellFolder2=NULL;
	HRESULT hr=m_psfSubFolder->QueryInterface(IID_IShellFolder2,(LPVOID*)&pShellFolder2);
	if (SUCCEEDED(hr))
	{
		LPUNKNOWN pUnk=NULL;
		if (SUCCEEDED(pShellFolder2->QueryInterface(IID_IUnknown,(LPVOID*)&pUnk)))
		{
			m_ShellDetails.SetShellDetails(pUnk);
			pUnk->Release();
		}
		m_ShellDetails.SetShellDetails((LPUNKNOWN)pShellFolder2);
		pShellFolder2->Release();
	}
	else
	{
		IShellDetails *pShellDetails=NULL;
		HRESULT hr = m_psfSubFolder->CreateViewObject(GetSafeHwnd(), IID_IShellDetails, (LPVOID*)&pShellDetails);
		if (SUCCEEDED(hr))
		{
			LPUNKNOWN pUnk=NULL;
			if (SUCCEEDED(pShellDetails->QueryInterface(IID_IUnknown,(LPVOID*)&pUnk)))
			{
				m_ShellDetails.SetShellDetails(pUnk);
				pUnk->Release();
			}
			pShellDetails->Release();
		}
	}
	InitColumns();
		
	DWORD dwExStyle = GetExStyle(); 
	if(!m_ShellSettings.DoubleClickInWebView()) 
	{
		dwExStyle |= LVS_EX_ONECLICKACTIVATE | LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT; 
	}
	else
	{
		dwExStyle &= ~LVS_EX_ONECLICKACTIVATE;
		dwExStyle &= ~LVS_EX_TRACKSELECT;
		dwExStyle &= ~LVS_EX_UNDERLINEHOT; 
	}
	SetExStyle(dwExStyle);

    LPITEMIDLIST lpi=NULL;
    LPENUMIDLIST lpe=NULL;
    LPLVITEMDATA lplvid=NULL;
	LPSHELLFOLDER lpsf=m_psfSubFolder;
    ULONG        ulFetched, ulAttrs;
    HWND         hwnd=::GetParent(m_hWnd);
	DWORD dwFlags = SHCONTF_NONFOLDERS;

	if (GetShellSettings().ShowAllObjects() && !GetShellSettings().ShowSysFiles())
		dwFlags |= SHCONTF_INCLUDEHIDDEN;
    hr=lpsf->EnumObjects(hwnd,dwFlags, &lpe);

    if (FAILED(hr))
		return FALSE;

	bool bEndPopulate=false;
	if (m_bPopulateInit == false)
	{
		StartPopulate();
		bEndPopulate = true;
	}
    while (NO_ERROR==lpe->Next(1, &lpi, &ulFetched))
    {
		ulAttrs = SFGAO_HASSUBFOLDER | SFGAO_FOLDER | SFGAO_DISPLAYATTRMASK | SFGAO_CONTENTSMASK | SFGAO_REMOVABLE; 
        lpsf->GetAttributesOf(1, (LPCITEMIDLIST*)&lpi, &ulAttrs);
		if (!FilterItem(lpsf,lpi,ulAttrs))
		{
	        m_pMalloc->Free(lpi);  // free PIDL the shell gave you
		    lpi=NULL;
			continue;
		}
        lplvid = (LPLVITEMDATA)m_pMalloc->Alloc(sizeof(LVITEMDATA));
        if (!lplvid)
           break;
		ZeroMemory(lplvid,sizeof(LVITEMDATA));
		m_vecItemData.push_back(lplvid);

        lplvid->ulAttribs=ulAttrs;

        lplvid->lpifq=m_ShellPidl.ConcatPidl(lptvid->lpifq, lpi);

        lplvid->lpsfParent=lpsf;
        lplvid->lpsfParent->AddRef();

        // Now make a copy of the ITEMIDLIST.
        lplvid->lpi=m_ShellPidl.CopyItemID(lpi);

		// Add the item to the list view control.   
		int nRow=-1;
		if (bCallBack)
		{
			nRow = AddCallBackItem((DWORD)lplvid,I_IMAGECALLBACK);			
			if ((ulAttrs & SFGAO_COMPRESSED) && m_ShellSettings.ShowCompColor())
				SetTextColor(nRow,0,RGB(0,0,255));
			else
				SetDefaultTextColor(nRow,-1);
		}
		else
		{
			nRow = AddTextItem();
			if (nRow != -1)
			{
				TCHAR szText[MAX_PATH+1];
				SetItemData(nRow,(DWORD)lplvid);
				LV_DISPINFO dsp;
				ZeroMemory(&dsp,sizeof(LV_DISPINFO));
				dsp.item.mask |= (LVIF_TEXT | LVIF_IMAGE);
				for(int i=0; i < GetColumnCount();i++)
				{
					szText[0] = 0;
					dsp.item.iItem = nRow;
					dsp.item.iSubItem = i;
					dsp.item.cchTextMax = MAX_PATH;
					dsp.item.pszText = szText;
					if (i > 0)
						dsp.item.mask &= ~LVIF_IMAGE;
					m_bCallBack = true;
					GetDispInfo(&dsp);
					m_bCallBack = false;
					SetIcon(nRow,dsp.item.iImage);
					AddString(nRow,i,szText);
				}
			}
		}
        m_pMalloc->Free(lpi);  // free PIDL the shell gave you
        lpi=NULL;
    }

    if (lpe)  
        lpe->Release();

    // The following two if statements will be TRUE only if you got here
    // on an error condition from the goto statement.  Otherwise, free 
    // this memory at the end of the while loop above.
    if (lpi)           
        m_pMalloc->Free(lpi);
 
	if (bEndPopulate)
		EndPopulate();
    return TRUE;
}

bool CIEShellListCtrl::FilterItem(LPSHELLFOLDER pFolder,LPCITEMIDLIST pidl,UINT ulAttrs)
{
	if ((m_sFileFilter.IsEmpty() || m_sFileFilter == _T("*.*") || m_sFileFilter == _T("*"))
		&& m_ExcludedLookup.IsEmpty())
		return true;
	bool bRet=false;
	if (m_sFileFilter == _T("*.*") || m_sFileFilter == _T("*"))
	{
		bRet=true;
	}
	else if (!m_sFileFilter.IsEmpty())
	{
		CString sPath;
		GetShellPidl().SHPidlToPathEx(pidl,sPath,pFolder);
		if (m_FilterLookup.Find(sPath))
			bRet = true;
	}
	if (bRet == false)
		return bRet;
	if (!m_ExcludedLookup.IsEmpty())
	{
		SHFILEINFO fileInfo;
		ZeroMemory(&fileInfo,sizeof(fileInfo));
		SHGetFileInfo((LPCTSTR)pidl, NULL, &fileInfo, sizeof(fileInfo), SHGFI_PIDL|SHGFI_TYPENAME);
		if (m_ExcludedLookup.Find(fileInfo.szTypeName))
		{
			bRet = false;
		}
	}
	return bRet;
}

void CIEShellListCtrl::InitShellSettings()
{
	m_ShellSettings.GetSettings();
}  

BOOL CIEShellListCtrl::InitImageLists()
{
    HIMAGELIST himlSmall;
    HIMAGELIST himlLarge;
    SHFILEINFO sfi;
    BOOL       bSuccess=TRUE;

    himlSmall = (HIMAGELIST)SHGetFileInfo((LPCTSTR)_T("C:\\"), 
                                           0,
                                           &sfi, 
                                           sizeof(SHFILEINFO), 
                                           SHGFI_SYSICONINDEX | SHGFI_SMALLICON);

    himlLarge = (HIMAGELIST)SHGetFileInfo((LPCTSTR)_T("C:\\"), 
                                           0,
                                           &sfi, 
                                           sizeof(SHFILEINFO), 
                                           SHGFI_SYSICONINDEX | SHGFI_LARGEICON);

    if (himlSmall && himlLarge)
    {
        ::SendMessage(GetSafeHwnd(), LVM_SETIMAGELIST, (WPARAM)LVSIL_SMALL,
            (LPARAM)himlSmall);
        ::SendMessage(GetSafeHwnd(), LVM_SETIMAGELIST, (WPARAM)LVSIL_NORMAL,
            (LPARAM)himlLarge);
    }
    else
       bSuccess = FALSE;

    return bSuccess;
}

BEGIN_MESSAGE_MAP(CIEShellListCtrl, CUIODListCtrl)
	//{{AFX_MSG_MAP(CIEShellListCtrl)
	ON_MESSAGE(WM_SETMESSAGESTRING,OnSetmessagestring)
	ON_MESSAGE(WM_SETTINGCHANGE,OnSettingChange)
	// NOTE - the ClassWizard will add and remove mapping macros here.
	ON_WM_DESTROY()
	ON_WM_MENUSELECT()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_APP_ON_DELETE_KEY,OnAppDeleteKey)
	ON_MESSAGE(WM_APP_ON_PROPERTIES_KEY,OnAppPropertiesKey)
	ON_MESSAGE(WM_APP_UPDATE_ALL_VIEWS,OnAppUpdateAllViews)
	ON_MESSAGE(WM_APP_FILE_CHANGE_NEW_PATH,OnAppFileChangeNewPath)
	ON_MESSAGE(WM_APP_FILE_CHANGE_EVENT,OnAppFileChangeEvent)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIEShellListCtrl message handlers
void CIEShellListCtrl::OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu)
{
	CUIODListCtrl::OnMenuSelect(nItemID,nFlags,hSysMenu);
}

void CIEShellListCtrl::ShowPopupMenu(int nRow,int nCol,CPoint point)	
{
	if ((nCol > 0 || nRow == -1) || (m_PopupID || m_MultiPopupID))
	{
		CUIODListCtrl::ShowPopupMenu(nRow,nCol,point);
		return;
	}
	PopupTheMenu(nRow,point);
}

void CIEShellListCtrl::OnDestroy()
{
	SetImageList(NULL,LVSIL_SMALL);
	SetImageList(NULL,LVSIL_NORMAL);
	DestroyThreads();
	CUIODListCtrl::OnDestroy();
}

bool CIEShellListCtrl::UseShellColumns()
{
	return m_ShellDetails.IsValidDetails();
}

BOOL CIEShellListCtrl::GetDispInfo(LV_DISPINFO *pDispInfo)
{
	// TODO: Add your specialized code here and/or call the base class
	if (m_psfSubFolder == NULL || m_bCallBack == false)
		return CUIODListCtrl::GetDispInfo(pDispInfo);
	LPLVITEMDATA lplvid = (LPLVITEMDATA)GetItemData(pDispInfo->item.iItem);
	ASSERT(lplvid);
	if (lplvid == NULL)
		return FALSE;
	PSLC_COLUMN_DATA pColData=(PSLC_COLUMN_DATA)lplvid->lParam;
	if (pColData == NULL)
	{
		pColData = new SLC_COLUMN_DATA;
		pColData->sItems.SetSize(GetColumnCount());
		lplvid->lParam = (LPARAM)pColData;
	}
	if (pColData->pidl == NULL)
		pColData->pidl = m_ShellPidl.ConcatPidl(m_tvid.lpifq, lplvid->lpi);
	if (pDispInfo->item.mask & LVIF_IMAGE)
	{
		if (pColData->iImage == -1)
		{
			pColData->iImage = m_ShellPidl.GetIcon(pColData->pidl,SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
		}	
		pDispInfo->item.iImage = pColData->iImage;
		if (lplvid->ulAttribs & SFGAO_GHOSTED)
		{
			pDispInfo->item.mask |= LVIF_STATE;
			pDispInfo->item.stateMask = LVIS_CUT;
			pDispInfo->item.state = LVIS_CUT;
		}
		if (lplvid->ulAttribs & SFGAO_LINK)
		{
			pDispInfo->item.mask |= LVIF_STATE;
			pDispInfo->item.stateMask = LVIS_OVERLAYMASK;
			pDispInfo->item.state = INDEXTOOVERLAYMASK(2);
		}
		if (lplvid->ulAttribs & SFGAO_SHARE)
		{
			pDispInfo->item.mask |= LVIF_STATE;
			pDispInfo->item.stateMask = LVIS_OVERLAYMASK;
			pDispInfo->item.state = INDEXTOOVERLAYMASK(1);
		}
	}
	if (pDispInfo->item.mask & LVIF_TEXT)
	{
		if (UseShellColumns())
		{
			if (pDispInfo->item.iSubItem == COL_NAME)
			{
				if (pColData->sItems[pDispInfo->item.iSubItem].IsEmpty())
					m_ShellPidl.GetName(lplvid->lpsfParent, lplvid->lpi, SHGDN_NORMAL, pColData->sItems[pDispInfo->item.iSubItem]);
				_tcscpy(pDispInfo->item.pszText,pColData->sItems[pDispInfo->item.iSubItem]);
				if (m_bNoExt)
					RemoveExt(pDispInfo->item.pszText);
			}
			else
			{
				if (pColData->sItems[pDispInfo->item.iSubItem].IsEmpty())
				{
					SHELLDETAILS sd;
					ZeroMemory(&sd,sizeof(sd));
					HRESULT hr = m_ShellDetails.GetDetailsOf(lplvid->lpi,pDispInfo->item.iSubItem,&sd);
					LPTSTR pszText=NULL;
					GetShellPidl().StrRetToStr(sd.str,&pszText,lplvid->lpi);
					if (pszText)
					{
						_tcscpy(pDispInfo->item.pszText,pszText);
						GetShellPidl().Free(pszText);
					}
				}
				else
				{
					_tcscpy(pDispInfo->item.pszText,pColData->sItems[pDispInfo->item.iSubItem]);
				}
			}
		}
		else
		{
			GetText(pColData,pDispInfo,lplvid);
		}
	}
	return TRUE;
}

bool CIEShellListCtrl::GetColText(int nCol,LPCITEMIDLIST pidlAbs,LPLVITEMDATA lplvid,CString &sText)
{
	return false;
}

void CIEShellListCtrl::GetText(PSLC_COLUMN_DATA pColData,LV_DISPINFO *pDispInfo,LPLVITEMDATA lplvid)
{
	if (!pColData->sItems[pDispInfo->item.iSubItem].IsEmpty())
	{
		_tcscpy(pDispInfo->item.pszText,pColData->sItems[pDispInfo->item.iSubItem]);
		return;
	}
	LPITEMIDLIST pidlAbs = pColData->pidl;
	CString sText;
	if (GetColText(pDispInfo->item.iSubItem,pidlAbs,lplvid,sText))
	{
		pColData->sItems[pDispInfo->item.iSubItem] = sText;
		_tcscpy(pDispInfo->item.pszText,pColData->sItems[pDispInfo->item.iSubItem]);
		return;
	}
	if (pDispInfo->item.iSubItem == COL_NAME)
	{
		if (pColData->sItems[pDispInfo->item.iSubItem].IsEmpty())
			m_ShellPidl.GetName(lplvid->lpsfParent, lplvid->lpi, SHGDN_NORMAL,pColData->sItems[pDispInfo->item.iSubItem]);
		_tcscpy(pDispInfo->item.pszText,pColData->sItems[pDispInfo->item.iSubItem]);
		if (m_bNoExt)
			RemoveExt(pDispInfo->item.pszText);
	}
	else if (pDispInfo->item.iSubItem == COL_TYPE)
	{
		if (pColData->sItems[pDispInfo->item.iSubItem].IsEmpty())
		{
			SHFILEINFO fileInfo;
			if (SHGetFileInfo((LPCTSTR)lplvid->lpi, NULL, &fileInfo, sizeof(fileInfo), SHGFI_PIDL|SHGFI_TYPENAME))
			{
				pColData->sItems[pDispInfo->item.iSubItem] = fileInfo.szTypeName;
			}
		}
		_tcscpy(pDispInfo->item.pszText,pColData->sItems[pDispInfo->item.iSubItem]);
	}
	else if (pDispInfo->item.iSubItem == COL_SIZE || pDispInfo->item.iSubItem == COL_MODIFIED)
	{
		if (pColData->sItems[COL_SIZE].IsEmpty() || (pColData->sItems[COL_MODIFIED].IsEmpty()))
		{
			CString sSize;
			CString sDateTime;
			GetFileDetails(pidlAbs,sSize,sDateTime);
			if (pDispInfo->item.iSubItem == COL_SIZE)
				pColData->sItems[pDispInfo->item.iSubItem] = sSize;
			else if (pDispInfo->item.iSubItem == COL_MODIFIED)
				pColData->sItems[pDispInfo->item.iSubItem] = sDateTime;
		}
		if (pDispInfo->item.iSubItem == COL_SIZE)
			_tcscpy(pDispInfo->item.pszText,pColData->sItems[pDispInfo->item.iSubItem]);
		else if (pDispInfo->item.iSubItem == COL_MODIFIED)
			_tcscpy(pDispInfo->item.pszText,pColData->sItems[pDispInfo->item.iSubItem]);
	}
}

void CIEShellListCtrl::GetFileDetails(LPCITEMIDLIST pidl, CString &sSize, CString &sDateTime)
{
	CString sPath;
	GetShellPidl().SHPidlToPathEx((LPITEMIDLIST)pidl,sPath,NULL);
	if (sPath.GetLength() <= 3)
		return;
	WIN32_FIND_DATA FindFileData;
    HANDLE hFind = FindFirstFile(sPath,&FindFileData);
    if (hFind != INVALID_HANDLE_VALUE)
	{
		if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			unsigned __int64 fs = UMAKEINT64(FindFileData.nFileSizeLow, FindFileData.nFileSizeHigh);
			unsigned __int64 kb=1;
			TCHAR szText[1024];
			if (fs > 1024)
			{
				kb = fs / 1024;
				if (fs % 1024)
					kb++;
			}
			_ui64tot(kb,szText,10);
			sSize = szText;
			AddThousandSeps(sSize);
			sSize += _T(" KB");
		}
		if ((FindFileData.ftLastWriteTime.dwLowDateTime != 0) || (FindFileData.ftLastWriteTime.dwHighDateTime != 0))
		{
			sDateTime = CLocaleInfo::Instance()->FormatDateTime(FindFileData.ftLastWriteTime);
		}
		else
		{
			sDateTime = CLocaleInfo::Instance()->FormatDateTime(FindFileData.ftCreationTime);
		}
	    FindClose(hFind);
	}
}

void CIEShellListCtrl::RemoveExt(LPTSTR pszFileName)
{
	if (pszFileName == NULL)
		return;
	LPTSTR pExtPos=NULL;
	LPTSTR pEndPos = pszFileName+lstrlen(pszFileName);
	LPTSTR pStartPos = pszFileName;
	while (pEndPos > pStartPos)
	{
		if (*pEndPos == '.')
		{
			pExtPos = pEndPos;
			break;
		}
		pEndPos = _tcsdec(pStartPos,pEndPos);
	}
	if (pExtPos)
		*pExtPos = '\0';
}

int CALLBACK ShellCompareFunc(LPARAM lparam1, 
                                 LPARAM lparam2,
                                 LPARAM lparamSort);

PFNLVCOMPARE CIEShellListCtrl::GetCompareFunc()
{
	if (m_bCallBack)
		return ShellCompareFunc;
	return CUIODListCtrl::GetCompareFunc();
}

int CALLBACK ShellCompareFunc(LPARAM lParam1, 
                         LPARAM lParam2,
                         LPARAM lParamSort)
{
	CUIListCtrlData *pData1 = (CUIListCtrlData*)lParam1;
	CUIListCtrlData *pData2 = (CUIListCtrlData*)lParam2;
    LPLVITEMDATA lplvid1=(LPLVITEMDATA)pData1->GetExtData();
    LPLVITEMDATA lplvid2=(LPLVITEMDATA)pData2->GetExtData();
	CUIODListCtrlSortInfo *pSortInfo = (CUIODListCtrlSortInfo*)lParamSort;
	int nRet=0;
	HRESULT hr = lplvid1->lpsfParent->CompareIDs(pSortInfo->GetColumn(),lplvid1->lpi,lplvid2->lpi);
    if (SUCCEEDED(hr))
		nRet = (short)hr;
	if (!pSortInfo->Ascending())
		nRet = -nRet;
	return nRet;
}

/////////////////////////////////////////////////////////////////////////
// Thread function for detecting file system changes
UINT CIEShellListCtrl::ThreadFunc (LPVOID pParam)
{
	///////////////////////
    PFC_THREADINFO pThreadInfo = (PFC_THREADINFO)pParam;
    HANDLE hEvent = pThreadInfo->hEvent;
    HANDLE hMonitorEvent = pThreadInfo->hMonitorEvent;
	CIEShellListCtrl *pListCtrl = pThreadInfo->pListCtrl;
    HWND hWnd = pListCtrl->GetSafeHwnd();
    delete pThreadInfo;
	////////////////////////
	TCHAR szPath[MAX_PATH];
    int nHandles=2;
	int nRet=0;
	HANDLE hFileChange=NULL;
    HANDLE aHandles[3];
	aHandles[0] = hMonitorEvent;
    aHandles[1] = hEvent;
	aHandles[2] = NULL;
    BOOL bContinue = TRUE;
    // Sleep until a file change notification wakes this thread or
    // m_event becomes set indicating it's time for the thread to end.
    while (bContinue)
	{
		TRACE(_T("ListControl waiting for %u multiple objects\n"),nHandles);
        DWORD dw = ::WaitForMultipleObjects (nHandles, aHandles, FALSE, INFINITE);
		if (dw >= WAIT_ABANDONED && dw <= (WAIT_ABANDONED+(nHandles-1)))
		{
			TRACE(_T("ListControl waiting abandoned\n"),nHandles);
			break;
		}
		// Reset Event
        if(dw - WAIT_OBJECT_0 == 0) 
		{
			if (hFileChange && hFileChange != INVALID_HANDLE_VALUE)
				::FindCloseChangeNotification(hFileChange);
			if (::SendMessage(hWnd, WM_APP_FILE_CHANGE_NEW_PATH, (WPARAM)szPath, 0))
			{
				if (szPath[0] == 0)
				{
					TRACE(_T("File notify path was returned empty\n"));
					hFileChange = NULL;
					aHandles[2] = hFileChange;				
					continue;
				}
				TRACE(_T("File notify path returned %s\n"),szPath);
			    hFileChange = ::FindFirstChangeNotification (szPath, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE);
			    if (hFileChange == INVALID_HANDLE_VALUE)
				{
					TRACE(_T("File notify thread was unable to create notification object\n"));
					hFileChange = NULL;
					aHandles[2] = hFileChange;				
					continue;
				}
				else
				{
					if (nHandles == 2)
					{
						TRACE(_T("File notify thread has created the notification object for the first time\n"));
						nHandles++;
					}
					TRACE(_T("File notify thread has created the notification object\n"));
					aHandles[2] = hFileChange;				
				}
			}
		}
		// End Event
        else if(dw - WAIT_OBJECT_0 == 1) 
		{
            bContinue = FALSE;
			TRACE(_T("File Notify Thread was signalled to stop\n"));
		}
		// File Change Event
        else if (dw - WAIT_OBJECT_0 == 2) 
		{ // Respond to a change notification.
            ::FindNextChangeNotification (hFileChange);
			TRACE(_T("-- File notify event was fired in CIEShellListCtrl --\n"));
			if (pListCtrl->RefreshAllowed())
			{
				::PostMessage (hWnd, WM_APP_FILE_CHANGE_EVENT,0,0);
			}
			else
			{
				TRACE(_T("but not sending as refreshing\n"));
				pListCtrl->SetRefreshAllowed(true);
				TRACE(_T("Refresh is now allowed\n"));
			}
        }
    }
	if (hFileChange && hFileChange != INVALID_HANDLE_VALUE)
		::FindCloseChangeNotification (hFileChange);
	TRACE(_T("File Notify Thread is ending\n"));
    return nRet;
}

LRESULT CIEShellListCtrl::OnAppFileChangeNewPath(WPARAM wParam, LPARAM lParam)
{
	ASSERT(wParam);
	SHGetPathFromIDList(m_tvid.lpifq,(LPTSTR)wParam);
	return 1;
}

LRESULT CIEShellListCtrl::OnAppFileChangeEvent(WPARAM wParam, LPARAM lParam)
{
	if (!RefreshAllowed())
		return 1L;
	Refresh();
	if (m_bNotifyParent)
		GetParent()->SendMessage(WM_APP_UPDATE_ALL_VIEWS,(WPARAM)HINT_SHELL_FILE_CHANGED,(LPARAM)(LPCTSTR)m_sMonitorPath);
	return 1L;
}

void CIEShellListCtrl::PreSubclassWindow()
{
	CUIODListCtrl::PreSubclassWindow();
	CreateFileChangeThread(GetSafeHwnd());
}

void CIEShellListCtrl::CreateFileChangeThread(HWND hwnd)
{
	if (m_nThreadCount >= MAX_THREADS)
		return;
    PFC_THREADINFO pThreadInfo = new FC_THREADINFO; // Thread will delete
    pThreadInfo->hMonitorEvent = m_MonitorEvent[m_nThreadCount].m_hObject;
    pThreadInfo->hEvent = m_event[m_nThreadCount].m_hObject;
    pThreadInfo->pListCtrl = this;

    CWinThread* pThread = AfxBeginThread (ThreadFunc, pThreadInfo,
        THREAD_PRIORITY_IDLE);

    pThread->m_bAutoDelete = FALSE;
    m_hThreads[m_nThreadCount] = pThread->m_hThread;
    m_pThreads[m_nThreadCount++] = pThread;
}

bool CIEShellListCtrl::DragDrop(CDD_OleDropTargetInfo *pInfo)
{
	// TODO: Add your specialized code here and/or call the base class
	return m_ShellDragDrop.DragDrop(pInfo,m_tvid.lpsfParent,m_tvid.lpi);
}

bool CIEShellListCtrl::DragEnter(CDD_OleDropTargetInfo *pInfo)
{
	// TODO: Add your specialized code here and/or call the base class
	return m_ShellDragDrop.DragEnter(pInfo,m_tvid.lpsfParent,m_tvid.lpi);
}

bool CIEShellListCtrl::DragLeave(CDD_OleDropTargetInfo *pInfo)
{
	// TODO: Add your specialized code here and/or call the base class
	return m_ShellDragDrop.DragLeave(pInfo);
}

bool CIEShellListCtrl::DragOver(CDD_OleDropTargetInfo *pInfo)
{
	// TODO: Add your specialized code here and/or call the base class
	return m_ShellDragDrop.DragOver(pInfo,m_tvid.lpsfParent,m_tvid.lpi);
}

// When the use starts a drag drop source
DROPEFFECT CIEShellListCtrl::DoDragDrop(int *pnRows,COleDataSource *pOleDataSource)
{
	CShellPidl pidl;
	CCF_ShellIDList sl;
	CCF_HDROP cf_hdrop;
	CCF_String cf_text;
	cf_hdrop.AddDropPoint(CPoint(),FALSE);
	CString sPath;
	CString sText;
	LPLVITEMDATA plvid=NULL;
	if (GetShellPidl().IsDesktopFolder(m_psfSubFolder))
		sl.AddPidl(GetShellPidl().GetEmptyPidl());
	else
		sl.AddPidl(m_tvid.lpifq);
	for(int i=0;*pnRows != -1;i++)
	{
		plvid = (LPLVITEMDATA)GetItemData(*pnRows);
		ASSERT(plvid);
		sl.AddPidl(plvid->lpi);
		pidl.SHPidlToPathEx(plvid->lpi,sPath,plvid->lpsfParent);
		cf_hdrop.AddFileName(sPath);
		sText += sPath;
		sText += _T("\n");
		pnRows++;
	}
	cf_text.SetString(sText);
	if (i > 0)
	{
		CWDClipboardData::Instance()->SetData(pOleDataSource,&cf_text,CWDClipboardData::e_cfString);
		CWDClipboardData::Instance()->SetData(pOleDataSource,&sl,CWDClipboardData::e_cfShellIDList);
		CWDClipboardData::Instance()->SetData(pOleDataSource,&cf_hdrop,CWDClipboardData::e_cfHDROP);
	}
	return GetShellPidl().GetDragDropAttributes(plvid);
}


LRESULT CIEShellListCtrl::OnAppUpdateAllViews(WPARAM wParam, LPARAM lParam)
{
	if (wParam == HINT_TREE_SEL_CHANGED)
	{
		ASSERT(lParam);
		const CRefreshShellFolder *pRefresh = reinterpret_cast<CRefreshShellFolder*>(lParam);
		if (pRefresh)
		{
			LPTVITEMDATA lptvid = reinterpret_cast<LPTVITEMDATA>(pRefresh->GetItemData());
			ASSERT(lptvid);
			if (lptvid == NULL)
				return 1L;
			bool bInternet=GetShellPidl().ComparePidls(NULL,lptvid->lpifq,m_pidlInternet);
			if (GetParent())
				GetParent()->SendMessage(WM_APP_UPDATE_ALL_VIEWS,HINT_TREE_INTERNET_FOLDER_SELECTED,(LPARAM)(bInternet ? 1 : 0));
			if (bInternet == false)
				Populate(lptvid);
		}
	}	
	return 1L;
}

LRESULT CIEShellListCtrl::OnSettingChange(WPARAM wParam,LPARAM lParam)
{ 
	InitShellSettings();
	Refresh();
	LPCTSTR lpszSection=(LPCTSTR)lParam;
	TRACE1("OnSettingsChange in CIEShellListCtrl with %s\n",lpszSection ? lpszSection : _T("null"));
	return 1L; 
}  

BOOL CIEShellListCtrl::DoubleClick(NM_LISTVIEW* pNMListView)
{
	// TODO: Add your specialized code here and/or call the base class
	if (m_ShellSettings.DoubleClickInWebView())
	{
		ShellExecute(pNMListView->iItem);
		return TRUE;
	}
	return CUIODListCtrl::DoubleClick(pNMListView);
}

BOOL CIEShellListCtrl::OnEnter(NM_LISTVIEW* pNMListView)
{
	// TODO: Add your specialized code here and/or call the base class
	ShellExecute(pNMListView->iItem);
	return TRUE;
}

LRESULT CIEShellListCtrl::OnAppDeleteKey(WPARAM wParam, LPARAM lParam)
{
	// TODO: Add your specialized code here and/or call the base class
	UINT nSize = GetSelectedCount()*MAX_PATH;
	if (nSize == 0)
		return 0L;
	SetRefreshAllowed(false);
	SHFILEOPSTRUCT shf;
	ZeroMemory(&shf,sizeof(shf));
	LPTSTR pszFrom=new TCHAR[nSize];
	LPTSTR pszStartFrom=pszFrom;
	ZeroMemory(pszFrom,nSize*sizeof(TCHAR));
	CString sPath;
	int item=-1;
	while ((item = GetNextSel(item)) != -1)
	{
		sPath = GetPathName(item);
		lstrcpy(pszFrom,(LPCTSTR)sPath);
		pszFrom += sPath.GetLength()+1;
	}
	shf.hwnd = GetSafeHwnd();
	shf.wFunc = FO_DELETE;
	shf.pFrom = (LPCTSTR)pszStartFrom;
	shf.fFlags = GetKeyState(VK_SHIFT) < 0 ? 0 : FOF_ALLOWUNDO;
	if (SHFileOperation(&shf) != 0)
		SetRefreshAllowed(true);
	delete []pszStartFrom;
	return 1L;
}

LRESULT CIEShellListCtrl::OnAppPropertiesKey(WPARAM wParam, LPARAM lParam)
{
	// TODO: Add your specialized code here and/or call the base class
	if (GetSelectedCount() == 0)
		return 0L;
	SHELLEXECUTEINFO si;
	ZeroMemory(&si,sizeof(si));
	si.cbSize = sizeof(si);
	si.hwnd = GetSafeHwnd();
	si.nShow = SW_SHOW;
	si.fMask  = SEE_MASK_INVOKEIDLIST;
	si.lpVerb = _T("properties");
	int item=GetCurSel();
	while ((item = GetNextSel(item)) != -1)
	{
		CString sPath(GetPathName(item));
		si.lpFile = (LPCTSTR)sPath;
		ShellExecuteEx(&si);
	}
	return 1L;
}

bool CIEShellListCtrl::EndLabelEdit(int nRow,int nCol,LPCTSTR pszText)
{
	// TODO: Add your specialized code here and/or call the base class
	if (nCol != 0)
		return CUIODListCtrl::EndLabelEdit(nRow,nCol,pszText);
	CString sFromPath(GetPathName(nRow));
	CString sToPath;
	CSplitPath path(sFromPath);
	if (GetShellSettings().ShowExtensions())
	{
		CSplitPath file(pszText);
		path.SetFileName(file.GetFileName());
		path.SetExt(file.GetExt());
	}
	else
	{
		path.SetFileName(pszText);
	}
	path.Make();
	sToPath = path.GetPath();
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
	int nRet = SHFileOperation(&shf);
	TRACE1("SHFileOperation returned %u\n",nRet);
	return false;
}

void CIEShellListCtrl::GoBack(int nRow)
{
	// TODO: Add your specialized code here and/or call the base class
	AfxMessageBox(_T("Needs to be implemented"));
}

void CIEShellListCtrl::ChangeStyle(UINT &dwStyle)
{
	// TODO: Add your specialized code here and/or call the base class
	CUIODListCtrl::ChangeStyle(dwStyle);

	dwStyle |= (LVS_SHAREIMAGELISTS | LVS_EDITLABELS);  
}

LRESULT CIEShellListCtrl::OnSetmessagestring(WPARAM wParam, LPARAM lParam)
{
	if (GetParent())
		return GetParent()->SendMessage(WM_SETMESSAGESTRING,wParam,lParam);
	return 0;
}

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

// IEShellComboBox.cpp : implementation file
//

#include "stdafx.h"
#include "UIMessages.h"
#include "IEShellComboBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const CString strURLKey(_T("Software\\Microsoft\\Internet Explorer\\TypedURLs"));
const CString strIEKey(_T("\\CLSID\\{0002DF01-0000-0000-C000-000000000046}\\LocalServer32"));
/////////////////////////////////////////////////////////////////////////////
// CIEShellComboBox

CIEShellComboBox::CIEShellComboBox()
{
	m_pidlMyComputer = NULL;
	m_pidlMyDocuments = NULL;
	m_pidlInternet = NULL;
	m_hImageList = NULL;
	m_hTreeWnd = NULL;
	m_hIcon = NULL;
	SHGetMalloc(&m_pMalloc);
	m_bInternet = false;
}

CIEShellComboBox::~CIEShellComboBox()
{
	DeleteAllItemData();
	CShCMSort *pItem=NULL;
	STL_FOR_ITERATOR(vecCMSort,m_vItems)
	{
		pItem = STL_GET_CURRENT(m_vItems);
		DeleteItemData((LPTVITEMDATA)pItem->GetItemData());
		delete pItem;
	}
	if (m_pMalloc)
	{
		if (m_pidlInternet)
			m_pMalloc->Free(m_pidlInternet);
		if (m_pidlMyComputer)
			m_pMalloc->Free(m_pidlMyComputer);
		if (m_pidlMyDocuments)
			m_pMalloc->Free(m_pidlMyDocuments);
		m_pMalloc->Release();
	}
	if (m_hIcon)
		::DestroyIcon(m_hIcon);
}

void CIEShellComboBox::DeleteAllItemData()
{
	for(vecItemData::iterator it=m_vecItemData.begin();it != m_vecItemData.end();it++)
	{
		DeleteItemData(*it);
	}
	m_vecItemData.erase(m_vecItemData.begin(),m_vecItemData.end());
}

void CIEShellComboBox::DeleteItemData(LPTVITEMDATA pItemData)
{
	if (pItemData == NULL)
		return;
	if (pItemData->lpsfParent)
		pItemData->lpsfParent->Release();
	if (pItemData->lpi)
		m_pMalloc->Free(pItemData->lpi);  
	if (pItemData->lpifq)
		m_pMalloc->Free(pItemData->lpifq);  
	m_pMalloc->Free(pItemData);
}

LPITEMIDLIST CIEShellComboBox::GetSelectedPidl()
{
	int nCurSel = GetCurSel();
	if (nCurSel == -1)
		return NULL;
	LPTVITEMDATA lptvid = (LPTVITEMDATA)GetItemData(nCurSel);	
	if (lptvid == NULL)
		return NULL;
	return GetShellPidl().CopyItemIDList(lptvid->lpifq);
}

BEGIN_MESSAGE_MAP(CIEShellComboBox, CComboBoxEx)
	//{{AFX_MSG_MAP(CIEShellComboBox)
	ON_MESSAGE(WM_APP_CB_IE_HIT_ENTER,OnAppCbIeHitEnter)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_CONTROL_REFLECT(CBN_DROPDOWN, OnDropDown)
	ON_CONTROL_REFLECT(CBN_SELCHANGE, OnSelChange)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_APP_CB_IE_POPULATE,OnCBIEPopulate)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIEShellComboBox message handlers

void CIEShellComboBox::Populate(LPITEMIDLIST pidlAbsSel)
{
	if (m_hImageList == NULL)
		SetShellImageList();
	if (STL_EMPTY(m_vItems))
		InitItems(pidlAbsSel);
	if (GetShellPidl().ComparePidls(NULL,pidlAbsSel,m_pidlInternet))
		LoadURLPrevList();
	else
		LoadItems(pidlAbsSel);
}

void CIEShellComboBox::LoadURLPrevList()
{
	if (m_ImageList.m_hImageList)
		::SendMessage(GetSafeHwnd(), CBEM_SETIMAGELIST, 0, (LPARAM)m_ImageList.m_hImageList);
	ResetContent();
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER,strURLKey,0,KEY_READ,&hKey) != ERROR_SUCCESS)
		return;
	TCHAR szValueName[_MAX_PATH];
	BYTE pData[_MAX_PATH];
	DWORD dwSizeValueName = sizeof(szValueName)-1;
	DWORD dwSizeData = sizeof(pData)-1;
	DWORD dwType=0;
	COMBOBOXEXITEM item;
	ZeroMemory(&item,sizeof(item));
	item.mask |= (CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE);	
	item.iItem = -1;
	item.iImage = 0;
	for(DWORD dwIndex=0;RegEnumValue(hKey,
					  dwIndex, 
					  szValueName, 
					  &dwSizeValueName,
					  NULL,
					  &dwType,
					  pData,
					  &dwSizeData) == ERROR_SUCCESS;dwIndex++)
	{
		item.pszText = (LPTSTR)(LPCTSTR)pData;
		item.cchTextMax = lstrlen(item.pszText);
		InsertItem(&item);
		dwSizeValueName = sizeof(szValueName)-1;
		dwSizeData = sizeof(pData)-1;
	}
	RegCloseKey(hKey);
	m_bInternet = true;
}

void CIEShellComboBox::InitItems(LPITEMIDLIST pidlAbsSel)
{
	LPSHELLFOLDER psfDesktop=NULL;
	LPITEMIDLIST pidlDesktop=NULL;
	HRESULT hr = SHGetDesktopFolder(&psfDesktop);
	SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidlDesktop);
    SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &m_pidlMyComputer); 
    SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &m_pidlMyDocuments); 
	SHGetSpecialFolderLocation(NULL, CSIDL_INTERNET, &m_pidlInternet);
	int nImage=0, nSelImage=0;
	SHFILEINFO fileInfo;
	SHGetFileInfo((LPCTSTR)pidlDesktop, NULL, &fileInfo, sizeof(fileInfo), SHGFI_PIDL|SHGFI_ATTRIBUTES|SHGFI_DISPLAYNAME);
	m_ShellPidl.GetNormalAndSelectedIcons(pidlDesktop, nImage, nSelImage);
	CShCMSort *pItem = new CShCMSort;
	pItem->SetText(fileInfo.szDisplayName);
	pItem->SetImage(nImage);
	pItem->SetSelImage(nSelImage);
	LPTVITEMDATA lptvid = (LPTVITEMDATA)m_pMalloc->Alloc(sizeof(TVITEMDATA));
	lptvid->lpi = GetShellPidl().CopyItemID(pidlDesktop);
	lptvid->lpifq = GetShellPidl().CopyItemIDList(pidlDesktop);
	lptvid->lpsfParent = NULL;
	STL_ADD_ITEM(m_vItems,pItem);
	BuildFolderList(psfDesktop,pidlDesktop,pidlAbsSel,1);
	if (m_pMalloc)
	{
		if (pidlDesktop)
			m_pMalloc->Free(pidlDesktop);
	}
	psfDesktop->Release();
}

void CIEShellComboBox::BuildFolderList(LPSHELLFOLDER pFolder,LPITEMIDLIST pidl,LPITEMIDLIST pidlAbsSel, int nIndent)
{
	vecCMSort vItems;
	AddItems(vItems,pFolder,pidl,nIndent);
	// Sort the this node based on pidls
	STL_SORT(vItems,pFolder,STL_SORT_FUNC);
	CShCMSort *pItem=NULL;
	STL_FOR_ITERATOR(vecCMSort,vItems)
	{
		pItem = STL_GET_CURRENT(vItems);
		STL_ADD_ITEM(m_vItems,pItem);
		if (GetShellPidl().ComparePidls(pFolder,pItem->GetPidl(),m_pidlMyComputer))
		{
			LPSHELLFOLDER pSubFolder=NULL;
			HRESULT hr = pFolder->BindToObject(pItem->GetPidl(), 0, IID_IShellFolder,(LPVOID*)&pSubFolder);
			if (SUCCEEDED(hr))
			{
				BuildFolderList(pSubFolder,pItem->GetPidl(),pidlAbsSel,nIndent+1);
				pSubFolder->Release();
			}
		}
	}
}

void CIEShellComboBox::LoadItems(LPITEMIDLIST pidlAbsSel)
{
	if (m_hImageList)
		::SendMessage(GetSafeHwnd(), CBEM_SETIMAGELIST, 0, (LPARAM)m_hImageList);
	GetComboBoxCtrl()->ResetContent();
	DeleteAllItemData();

	CShCMSort *pItem=NULL;
	int nCount = GetShellPidl().GetCount(pidlAbsSel);
	LPITEMIDLIST pidlCompare=NULL;
	int nSelItem=-1;
	int nItem=0;
	int n=0;
	int nIndent=0;
	CString sPath;
	STL_FOR_ITERATOR(vecCMSort,m_vItems)
	{
		pItem = STL_GET_CURRENT(m_vItems);		
		nItem = AddItem(pItem);
		nIndent = pItem->GetIndent();
		if (nIndent == 0)
			nIndent = 1;
//		TRACE2("Getting pidl %d of %u\n",nIndent,nCount);
		pidlCompare=GetShellPidl().CopyItemID(pidlAbsSel,nIndent);
		// Desktop item
		if (nCount == 0 && n == 0)
		{
			nSelItem = 0;
			sPath = pItem->GetText();
		}
		else if (nCount == 1 && GetShellPidl().ComparePidls(NULL,pItem->GetPidl(),pidlAbsSel) == true) 
		{
			nSelItem = nItem;
			sPath = pItem->GetText();
		}
		else if (GetShellPidl().CompareMemPidls(pItem->GetPidl(),pidlCompare) == true 
			&& GetShellPidl().CompareMemPidls(pItem->GetPidl(),m_pidlMyComputer) == false)
		{
			LPITEMIDLIST pidlAbs=NULL;
			CString sDisplayName;
			CShCMSort Item;
			int nImage=0;
			int nSelImage=0;
			for(int i=nIndent+1;i < (nCount+1);i++)
			{
				pidlAbs = GetShellPidl().CopyAbsItemID(pidlAbsSel,i);
				if (pidlAbs)
				{
					GetShellPidl().SHPidlToPathEx(pidlAbs,sPath,NULL);
					GetShellPidl().GetDisplayName(pidlAbs,sDisplayName);
					GetShellPidl().GetNormalAndSelectedIcons(pidlAbs,nImage,nSelImage);
					Item.SetText(sDisplayName);
					Item.SetImage(nImage);
					Item.SetSelImage(nSelImage);
					LPTVITEMDATA lptvid = (LPTVITEMDATA)m_pMalloc->Alloc(sizeof(TVITEMDATA));
					lptvid->lpi = GetShellPidl().CopyItemID(pidlAbs);
					lptvid->lpifq = pidlAbs;
					LPTVITEMDATA dt_lptvid = (LPTVITEMDATA)pItem->GetItemData();
					lptvid->lpsfParent = dt_lptvid->lpsfParent;
					lptvid->lpsfParent->AddRef();
					Item.SetItemData((DWORD)lptvid);
					Item.SetIndent(i);
					m_vecItemData.push_back(lptvid);
					nItem = AddItem(&Item);
				}
			}
			if (nSelItem == -1)
				nSelItem = nItem;
			if (sPath.Find(_T("::")) == 0)
				GetShellPidl().GetDisplayName(pidlAbsSel,sPath);
			else if(sPath.IsEmpty())
				sPath = pItem->GetText();
		}			
		if (pidlCompare)
		{
			m_pMalloc->Free(pidlCompare);
			pidlCompare = NULL;
		}
		n++;
	}
	if (nSelItem >= 0)
		GetComboBoxCtrl()->SetCurSel(nSelItem);
	GetEditCtrl()->SetWindowText(sPath);
	m_bInternet = false;
}

int CIEShellComboBox::AddItem(const CShCMSort *pItem)
{
	// add the node to the combo box
	COMBOBOXEXITEM item;
	ZeroMemory(&item,sizeof(item));
	item.mask |= (CBEIF_IMAGE | CBEIF_INDENT | CBEIF_LPARAM | CBEIF_TEXT | CBEIF_SELECTEDIMAGE);	
	item.iItem = -1;
	item.pszText = (LPTSTR)(LPCTSTR)pItem->GetText();
	item.cchTextMax = lstrlen(item.pszText);
	item.iImage = pItem->GetImage();
	item.iSelectedImage = pItem->GetSelImage();
	item.iOverlay = pItem->GetOverlayImage();
	item.iIndent = pItem->GetIndent();
	item.lParam = (LPARAM)pItem->GetItemData();
	return InsertItem(&item);
}

void CIEShellComboBox::AddItems(vecCMSort &vItems,IShellFolder* pFolder,LPITEMIDLIST pidlAbs,int nIndent)
{
	IEnumIDList* pItems = NULL;
	LPITEMIDLIST pidlNext = NULL;
	LPITEMIDLIST pidlCopy = NULL;
	LPTSTR pszFilePath = NULL;
	STRRET StrRetFilePath;
	// Enumerate all object in the given folder
	HRESULT hr = pFolder->EnumObjects(NULL, SHCONTF_FOLDERS|SHCONTF_NONFOLDERS, &pItems);
	SHFILEINFO fileinfo;
	while (NOERROR == hr)
	{
		hr = pItems->Next(1, &pidlNext, NULL);
		if (hr == S_FALSE || pidlNext == NULL || pidlNext == pidlCopy)
			break;
		pidlCopy = pidlNext;
		pFolder->GetDisplayNameOf(pidlNext,SHGDN_INFOLDER,&StrRetFilePath);
		GetShellPidl().StrRetToStr(StrRetFilePath, &pszFilePath, pidlNext);
		ZeroMemory(&fileinfo,sizeof(fileinfo));
		fileinfo.dwAttributes=SFGAO_HASSUBFOLDER | SFGAO_FOLDER;
		hr = pFolder->GetAttributesOf(1,(const struct _ITEMIDLIST **)&pidlCopy,&fileinfo.dwAttributes);
		if (SUCCEEDED(hr))
			lstrcpy(fileinfo.szDisplayName,pszFilePath);
		// Create a submenu if this item is a folder
		if (fileinfo.dwAttributes & (SFGAO_HASSUBFOLDER | SFGAO_FOLDER))
		{
			if (fileinfo.dwAttributes & SFGAO_FOLDER)
			{			
				AddFolder(vItems,fileinfo,pidlAbs,pidlNext,pFolder,nIndent);
			}
		}
		if (pszFilePath)
			m_pMalloc->Free(pszFilePath);
	}
	if (pidlNext)
		m_pMalloc->Free(pidlNext);
	if (pItems)
		pItems->Release();
}

void CIEShellComboBox::AddFolder(vecCMSort &vItems,const SHFILEINFO &FileInfo,LPITEMIDLIST pidlAbs,LPITEMIDLIST pidl,LPSHELLFOLDER pFolder,int nIndent)
{
	ASSERT(m_pMalloc);
	// allocate new itemdata
	LPTVITEMDATA lptvid = (LPTVITEMDATA)m_pMalloc->Alloc(sizeof(TVITEMDATA));
	if (lptvid == NULL)
		return;
	ZeroMemory(lptvid,sizeof(TVITEMDATA));
	// create new fully qualified pidl
	lptvid->lpifq = m_ShellPidl.ConcatPidl(pidlAbs,pidl);
	// Now make a copy of the last item in the pidl.
	lptvid->lpi = pidl;
	int nImage=0;
	int nSelImage=0;
	// get icons for new fq pidl
	m_ShellPidl.GetNormalAndSelectedIcons(lptvid->lpifq, nImage, nSelImage);
	// save folder for later use(when node is expanded)
	lptvid->lpsfParent = pFolder; // pointer to parent folder
	// keep hold of it(will be released in clean up)
	lptvid->lpsfParent->AddRef();
	CShCMSort *pSMI = new CShCMSort;
	pSMI->SetPidl(pidl);
	pSMI->SetText(FileInfo.szDisplayName);
	pSMI->SetImage(nImage);
	pSMI->SetSelImage(nSelImage);
	pSMI->SetItemData((DWORD)lptvid);
	pSMI->SetIndent(nIndent);
	STL_ADD_ITEM(vItems,pSMI);
}

void CIEShellComboBox::SetShellImageList()
{
	// TODO: Add your specialized creation code here
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_CLASSES_ROOT,strIEKey,0,KEY_READ,&hKey) == ERROR_SUCCESS)
	{
		BYTE szValueName[_MAX_PATH];
		DWORD dwType=0;
		DWORD dwSizeData = sizeof(szValueName)-1;
		RegQueryValueEx(hKey,NULL,0,&dwType,szValueName,&dwSizeData);
		CString sPath((LPCTSTR)szValueName);
		if (sPath.Left(1) == _T('"'))
			sPath = sPath.Right(sPath.GetLength()-1);
		if (sPath.Right(1) == _T('"'))
			sPath = sPath.Left(sPath.GetLength()-1);
		if (sPath.Find(_T("%")) != -1)
		{
			TCHAR szPath[MAX_PATH];
			if (ExpandEnvironmentStrings(sPath,szPath,sizeof(szPath)) > 0)
				sPath = szPath;
		}
		if (sPath.Find(_T("~")) != -1)
		{
			TCHAR szPath[MAX_PATH];
			if (GetLongPathName(sPath,szPath,sizeof(szPath)) > 0)
				sPath = szPath;
		}
		ExtractIconEx(sPath, 1, NULL, &m_hIcon, 1);		
		// create the small icon image list	
		UINT cxSmallIcon = ::GetSystemMetrics(SM_CXSMICON);
		UINT cySmallIcon = ::GetSystemMetrics(SM_CYSMICON);
		if (m_ImageList.GetSafeHandle() != NULL)
			m_ImageList.DeleteImageList();	
		m_ImageList.Create(cxSmallIcon,
							cySmallIcon,
							ILC_MASK | ILC_COLOR16,	
							1,
							1);	
		if (m_hIcon)
			m_ImageList.Add(m_hIcon);
	}
    SHFILEINFO    sfi;
    m_hImageList = (HIMAGELIST)SHGetFileInfo((LPCTSTR)_T("C:\\"), 
                                           0,
                                           &sfi, 
                                           sizeof(SHFILEINFO), 
                                           SHGFI_SYSICONINDEX | SHGFI_SMALLICON);

    // Attach ImageList to the window
    if (m_hImageList)
	    ::SendMessage(m_hWnd, CBEM_SETIMAGELIST, 0, (LPARAM)m_hImageList);
}

void CIEShellComboBox::SelectionChanged(bool bEnter)
{
	LPITEMIDLIST pidlSel = GetSelectedPidl();
	GetEditCtrl()->GetWindowText(m_sText);
	UINT mess=WM_APP_CB_IE_SEL_CHANGE;
	if (bEnter)
		mess=WM_APP_CB_IE_HIT_ENTER;
	if (m_hTreeWnd)
	{
		if (mess == WM_APP_CB_IE_SEL_CHANGE)
			::SendMessage(m_hTreeWnd,mess,(WPARAM)pidlSel,(LPARAM)(LPCTSTR)m_sText);
		else
			::SendMessage(m_hTreeWnd,mess,(WPARAM)m_bInternet,(LPARAM)(LPCTSTR)m_sText);
	}
}

int CIEShellComboBox::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CComboBoxEx::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	SetShellImageList();
	
	if (m_cbEdit.GetSafeHwnd() == NULL)
	{
		m_cbEdit.SubclassWindow(GetEditCtrl()->GetSafeHwnd());
		m_cbEdit.SetTreeWnd(m_hTreeWnd);	
	}
	return 0;
}

void CIEShellComboBox::OnDestroy() 
{
	SetImageList(NULL);

	CComboBoxEx::OnDestroy();
	
	// TODO: Add your message handler code here
	
}

LRESULT CIEShellComboBox::OnCBIEPopulate(WPARAM wParam,LPARAM lParam)
{
	Populate((LPITEMIDLIST)wParam);
	return 1L;
}

void CIEShellComboBox::OnDropDown()
{
}

void CIEShellComboBox::OnSelChange()
{
	SelectionChanged(false);
}

void CIEShellComboBox::OnKillFocus(CWnd *pNewWnd)
{
	CComboBoxEx::OnKillFocus(pNewWnd);
	SetEditSel(-1,0);
}

void CIEShellComboBox::OnSetFocus(CWnd *pWnd)
{
	CComboBoxEx::OnSetFocus(pWnd);
	SetEditSel(-1,-1);
}

BOOL CIEShellComboBoxEdit::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_RETURN)
		{
			GetParent()->GetParent()->PostMessage(WM_APP_CB_IE_HIT_ENTER);
			return TRUE;
		}
	}
	return CEdit::PreTranslateMessage(pMsg);
}

void CIEShellComboBox::PreSubclassWindow()
{
	// TODO: Add your specialized code here and/or call the base class
	CComboBoxEx::PreSubclassWindow();

	if (m_cbEdit.GetSafeHwnd() == NULL)
	{
		m_cbEdit.SubclassWindow(GetEditCtrl()->GetSafeHwnd());
		m_cbEdit.SetTreeWnd(m_hTreeWnd);	
	}
}

LRESULT CIEShellComboBox::OnAppCbIeHitEnter(WPARAM wParam, LPARAM lParam)
{
	SelectionChanged(true);
	return 1;
}

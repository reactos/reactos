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


#include "stdafx.h"
#include "IEFolderTreeCtrl.h"
#include "UIMessages.h"
#include <vector>
#include <map>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef map<CShellPidlCompare,HTREEITEM> mapPidlToHTREEITEM;
typedef vector<LPITEMIDLIST> vecPidl;

int CALLBACK CIEFolderTreeCtrl::CompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	LPTVITEMDATA lptvid1 = (LPTVITEMDATA)((CUIListCtrlData*)lParam1)->GetExtData();
	LPTVITEMDATA lptvid2 = (LPTVITEMDATA)((CUIListCtrlData*)lParam2)->GetExtData();
	LPSHELLFOLDER psfParent = (LPSHELLFOLDER)lParamSort;

	HRESULT hr = psfParent->CompareIDs (0, lptvid1->lpi, lptvid2->lpi);
	if (FAILED (hr))
		return 0;
	return (short)hr;
} 

/////////////////////////////////////////////////////////////////////////////
// CIEFolderTreeCtrl

CIEFolderTreeCtrl::CIEFolderTreeCtrl()
{
	SHGetMalloc(&m_pMalloc);
	m_hImageList = NULL;
}

CIEFolderTreeCtrl::~CIEFolderTreeCtrl()
{
	// Free our memory allocator
	if (m_pMalloc)
		m_pMalloc->Release();
}

void CIEFolderTreeCtrl::Refresh()
{
	HTREEITEM hItem = GetRootItem();
    if (hItem == NULL)
        return;
	CWaitCursor w;
	SetRedraw(FALSE);
    RefreshNode(hItem);
	SetRedraw(TRUE);
}

void CIEFolderTreeCtrl::OnDeleteItemData(DWORD dwData)
{
	LPTVITEMDATA pItemData=(LPTVITEMDATA)dwData;
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

BOOL CIEFolderTreeCtrl::LoadURL(HTREEITEM hItem)
{
	if (GetRootItem() == hItem)
		return FALSE;
	if (ItemHasChildren(hItem))
		return FALSE;
	CString strText(GetItemText(hItem));
	AfxMessageBox(strText);
	return TRUE;
}

bool CIEFolderTreeCtrl::LoadItems(LPCTSTR pszPath,DWORD dwFolderType)
{
	ASSERT(m_pMalloc);
	if (m_hImageList == NULL)
		Init();
	bool bRet = false;

	CWaitCursor w;
	DeleteAllItems();
	//DeleteItemData();

	LPITEMIDLIST pidl=NULL;
	LPSHELLFOLDER psfDesktop=NULL;
	LPSHELLFOLDER pSubFolder=NULL;
	HRESULT hr = SHGetDesktopFolder(&psfDesktop);

	if (dwFolderType)
	{
		hr = SHGetSpecialFolderLocation(NULL, dwFolderType, &pidl);
#ifdef _DEBUG
		CString sPath;
		GetShellPidl().SHPidlToPathEx(pidl,sPath);
		TRACE1("Populating special folder %s\n",sPath);
#endif
		hr = psfDesktop->BindToObject(pidl, 0, IID_IShellFolder,(LPVOID*)&pSubFolder);
	}
	else
	{
		if (pszPath && *pszPath != '\0')
		{
			hr = m_ShellPidl.SHPathToPidlEx(pszPath,&pidl,psfDesktop);
			if (SUCCEEDED(hr))
			{
				hr = psfDesktop->BindToObject(pidl, 0, IID_IShellFolder,(LPVOID*)&pSubFolder);
				if (SUCCEEDED(hr))
					m_sRootPath = pszPath;
			}
		}
		else
		{
			hr = SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidl);
		}
	}
	LPCTSTR pszTitle=NULL;
	SHFILEINFO fileInfo;
	ZeroMemory(&fileInfo,sizeof(fileInfo));
	int nImage=0, nSelImage=0;
	if (pidl)
	{
		SHGetFileInfo((LPCTSTR)pidl, NULL, &fileInfo, sizeof(fileInfo), SHGFI_PIDL|SHGFI_ATTRIBUTES|SHGFI_DISPLAYNAME);
		pszTitle = fileInfo.szDisplayName;
		m_ShellPidl.GetNormalAndSelectedIcons(pidl, nImage, nSelImage);
		if (nImage < 0)
			nImage = 0;
		if (nSelImage < 0)
			nSelImage = 0;
	}
	if (SUCCEEDED(hr))
	{
		LPTVITEMDATA lptvid = (LPTVITEMDATA)m_pMalloc->Alloc(sizeof(TVITEMDATA));
		if (lptvid == NULL)
			return bRet;
		ZeroMemory(lptvid,sizeof(TVITEMDATA));
		// Now make a copy of the ITEMIDLIST.
		lptvid->lpi = m_ShellPidl.CopyLastItemID(pidl);
		lptvid->lpifq = m_ShellPidl.CopyItemIDList(pidl);
		lptvid->lpsfParent = NULL;
		if (lptvid->lpsfParent)
			lptvid->lpsfParent->AddRef();

		HTREEITEM hRootItem = AddAnItem((HTREEITEM)NULL,pszTitle,(LPARAM)lptvid,(HTREEITEM)TVI_ROOT,nImage,nSelImage);
		AddItems(hRootItem,pSubFolder ? pSubFolder : psfDesktop);
		Expand(hRootItem,TVE_EXPAND);
		PostMessage(WM_APP_POPULATE_TREE);
		bRet = true;
	}
	if (pidl)
		m_pMalloc->Free(pidl);
	if (pSubFolder)
		pSubFolder->Release();
	if (psfDesktop)
		psfDesktop->Release();
	return bRet;
}

bool CIEFolderTreeCtrl::AddItems(HTREEITEM hItem,IShellFolder* pFolder)
{
	IEnumIDList* pItems = NULL;
	LPITEMIDLIST pidlNext = NULL;
	DWORD dwFlags = SHCONTF_FOLDERS;
	if (GetShellSettings().ShowAllObjects() && !GetShellSettings().ShowSysFiles())
		dwFlags |= SHCONTF_INCLUDEHIDDEN;
	// Enumerate all object in the given folder
	HRESULT hr = pFolder->EnumObjects(NULL, dwFlags, &pItems);
	if (hr != NOERROR)
		return false;
	while (NOERROR == hr)
	{
		hr = pItems->Next(1, &pidlNext, NULL);
		if (hr == S_FALSE || pidlNext == NULL)
			break;
		if (AddFolder(hItem,pidlNext,pFolder) == NULL)
			m_pMalloc->Free(pidlNext);
		pidlNext = NULL;
	}
	if (pidlNext)
		m_pMalloc->Free(pidlNext);
	if (pItems)
		pItems->Release();
	Sort(hItem,pFolder);
	return true;
}

void CIEFolderTreeCtrl::Sort(HTREEITEM hParent,LPSHELLFOLDER pFolder)
{
	// Sort the the node based on pidls
	TVSORTCB cbSort;
	cbSort.hParent = hParent;
	cbSort.lpfnCompare = CompareProc;
	cbSort.lParam = (LPARAM)pFolder;
	SortChildrenCB(&cbSort);
}

HTREEITEM CIEFolderTreeCtrl::AddFolder(HTREEITEM hItem,LPCTSTR pszPath)
{
	LPITEMIDLIST pidlfq=NULL;
	HRESULT hr = GetShellPidl().SHPathToPidlEx(pszPath,&pidlfq,NULL);
	if (FAILED(hr))
		return NULL;
	LPTVITEMDATA lptvid = (LPTVITEMDATA)GetItemData(hItem);
	ASSERT(lptvid);
	LPITEMIDLIST pidl = GetShellPidl().CopyLastItemID(pidlfq);
	HTREEITEM hFolderItem = AddFolder(hItem,pidl,lptvid->lpsfParent);
	if (pidlfq)
		m_pMalloc->Free(pidlfq);
	return hFolderItem;
}

HTREEITEM CIEFolderTreeCtrl::AddFolder(HTREEITEM hItem,LPITEMIDLIST pidl,LPSHELLFOLDER pFolder)
{
	ASSERT(m_pMalloc);
	LPTSTR pszFilePath = NULL;
	STRRET StrRetFilePath;
	SHFILEINFO FileInfo;

	ZeroMemory(&FileInfo,sizeof(FileInfo));
	FileInfo.dwAttributes=SFGAO_HASSUBFOLDER | SFGAO_FOLDER;
	HRESULT hr = pFolder->GetAttributesOf(1,(LPCITEMIDLIST*)&pidl,&FileInfo.dwAttributes);
	if (FAILED(hr))
		return NULL;
// Create a submenu if this item is a folder
	if (!(FileInfo.dwAttributes & (SFGAO_HASSUBFOLDER | SFGAO_FOLDER)))
		return NULL;
	pFolder->GetDisplayNameOf(pidl,SHGDN_INFOLDER,&StrRetFilePath);
	GetShellPidl().StrRetToStr(StrRetFilePath, &pszFilePath, pidl);
	if (pszFilePath)
	{
		lstrcpy(FileInfo.szDisplayName,pszFilePath);
		m_pMalloc->Free(pszFilePath);
		pszFilePath = NULL;
	}
	// allocate new itemdata
	LPTVITEMDATA lptvid = (LPTVITEMDATA)m_pMalloc->Alloc(sizeof(TVITEMDATA));
	if (lptvid == NULL)
		return NULL;
	ZeroMemory(lptvid,sizeof(TVITEMDATA));
	// get itemdata for current node
	LPTVITEMDATA lpptvid = (LPTVITEMDATA)GetItemData(hItem);
	ASSERT(lpptvid);
	// create new fully qualified pidl
	lptvid->lpifq = m_ShellPidl.ConcatPidl(lpptvid->lpifq,pidl);
	// save relative pidl (will be freed in the clean up)
	lptvid->lpi = pidl;
	int nImage=0;
	int nSelImage=0;
	// get icons for new fq pidl
	m_ShellPidl.GetNormalAndSelectedIcons(lptvid->lpifq, nImage, nSelImage);
	// save folder for later use(when node is expanded)
	lptvid->lpsfParent = pFolder; // pointer to parent folder
	// keep hold of it(will be released in clean up)
	lptvid->lpsfParent->AddRef();
	// add the node to the tree unsorted (will be sorted later)
	int nChildren = 0;
	if (FileInfo.dwAttributes & SFGAO_HASSUBFOLDER)
	{
		nChildren=1;
	}
	HTREEITEM hNewItem = AddAnItem(hItem,FileInfo.szDisplayName,(DWORD)lptvid,(HTREEITEM)TVI_FIRST,nImage,nSelImage,nChildren);
	// set overlay images
	if (hNewItem)
	{
		SetAttributes(hNewItem,pFolder,pidl);
	}
	return hNewItem;
}

void CIEFolderTreeCtrl::SetAttributes(HTREEITEM hItem,LPSHELLFOLDER pFolder,LPITEMIDLIST pidl)
{
	DWORD dwAttributes = SFGAO_DISPLAYATTRMASK | SFGAO_REMOVABLE;
	HRESULT hr = pFolder->GetAttributesOf(1,(LPCITEMIDLIST*)&pidl,&dwAttributes);
	if (FAILED(hr))
		return;	 
	if ((dwAttributes & SFGAO_COMPRESSED) && GetShellSettings().ShowCompColor())
		SetTextColor(hItem,RGB(0,0,255));
	else
		SetDefaultTextColor(hItem);
	if (dwAttributes & SFGAO_GHOSTED)
		SetItemState(hItem,TVIS_CUT,TVIS_CUT);
	else
		SetItemState(hItem,TVIS_CUT,0);
	if (dwAttributes & SFGAO_LINK)
		SetItemState(hItem,INDEXTOOVERLAYMASK(2),TVIS_OVERLAYMASK);
	else
		SetItemState(hItem,0,TVIS_OVERLAYMASK);
	if (dwAttributes & SFGAO_SHARE)
		SetItemState(hItem,INDEXTOOVERLAYMASK(1),TVIS_OVERLAYMASK);
	else
		SetItemState(hItem,0,TVIS_OVERLAYMASK);
}

BEGIN_MESSAGE_MAP(CIEFolderTreeCtrl, CUITreeCtrl)
	//{{AFX_MSG_MAP(CIEFolderTreeCtrl)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_SETTINGCHANGE,OnSettingChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIEFolderTreeCtrl message handlers

int CIEFolderTreeCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CUITreeCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	// TODO: Add your specialized creation code here
	
	return 0;
}

BOOL CIEFolderTreeCtrl::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	// No label editing for Explorer items
	cs.style |= (TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS);	
	return CTreeCtrl::PreCreateWindow(cs);
}

void CIEFolderTreeCtrl::CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	CUITreeCtrl::CalcWindowRect(lpClientRect, nAdjustType);
}

LPCITEMIDLIST CIEFolderTreeCtrl::GetPathPidl(HTREEITEM hItem)
{
	if (hItem == NULL)
		return NULL;
	LPLVITEMDATA plvit = (LPLVITEMDATA)GetItemData(hItem);
	ASSERT(plvit);
	if (plvit == NULL)
		return NULL;
	return plvit->lpifq;
}

LPSHELLFOLDER CIEFolderTreeCtrl::GetItemFolder(HTREEITEM hItem)
{
	LPTVITEMDATA lpidCurr = (LPTVITEMDATA)GetItemData(hItem);
	ASSERT(lpidCurr);
	if (lpidCurr == NULL)
		return NULL;
	LPSHELLFOLDER psfCurr=NULL;

#if 1 // bug fix by Gregoire
	LPSHELLFOLDER psfDesktop=NULL;
	SHGetDesktopFolder(&psfDesktop);
	psfDesktop->BindToObject(lpidCurr->lpifq,0,IID_IShellFolder,(LPVOID*)&psfCurr);
#else
	if (lpidCurr->lpsfParent)
		lpidCurr->lpsfParent->BindToObject(lpidCurr->lpi,0,IID_IShellFolder,(LPVOID*)&psfCurr);
#endif
	
	if (psfCurr == NULL)
	{
		SHGetDesktopFolder(&psfCurr);
	}
	return psfCurr;
}

CString CIEFolderTreeCtrl::GetPathName(HTREEITEM hItem)
{
	if (hItem == NULL)
		hItem = GetSelectedItem();
    CString sPath;
	if (hItem == NULL)
		return sPath;
	LPTVITEMDATA lptvid = (LPTVITEMDATA)GetItemData(hItem);
	if (lptvid != NULL)
	{
		SHGetPathFromIDList(lptvid->lpifq,sPath.GetBuffer(MAX_PATH));
		sPath.ReleaseBuffer();
	}
	return sPath;
}


void CIEFolderTreeCtrl::SetButtonState(HTREEITEM hItem)
{
	LPSHELLFOLDER psfCurr=GetItemFolder(hItem);
	if (psfCurr == NULL)
		return;
	IEnumIDList* pItems=NULL;
	HRESULT hr = psfCurr->EnumObjects(NULL, SHCONTF_FOLDERS, &pItems);
	int nChildren=0;
	if (SUCCEEDED(hr))
	{
		pItems->Release();
		nChildren=1;
	}
	if (nChildren == 1 && !ItemHasChildren(hItem))
	{
		TVITEM tv;
		tv.mask = TVIF_CHILDREN;
		ZeroMemory(&tv,sizeof(tv));
		SetItem(&tv);
	}
	psfCurr->Release();
}

void CIEFolderTreeCtrl::RefreshNode(HTREEITEM hItem)
{
    // If the item is not expanded, update its button state and return.
    if (!(GetItemState(hItem, TVIS_EXPANDED) & TVIS_EXPANDED)) 
	{
        SetButtonState(hItem);
        return;
	}
	LPSHELLFOLDER psfCurr=GetItemFolder(hItem);
	if (psfCurr == NULL)
		return;
	mapPidlToHTREEITEM mPidlCurr;
	vecPidl vPidlNew;
	HTREEITEM hSelItem = GetSelectedItem();
    HTREEITEM hChild = GetChildItem(hItem);
    while (hChild != NULL)
	{
        HTREEITEM hNextItem = GetNextSiblingItem(hChild);
        LPTVITEMDATA lpid = (LPTVITEMDATA)GetItemData(hChild);
		mPidlCurr[CShellPidlCompare(psfCurr,lpid->lpi)] = hChild;
        hChild = hNextItem;
    }
	LPITEMIDLIST pidlNext=NULL;
	LPITEMIDLIST pidlCopy=NULL;
	IEnumIDList* pItems=NULL;
	DWORD dwFlags = SHCONTF_FOLDERS;
	if (GetShellSettings().ShowAllObjects() && !GetShellSettings().ShowSysFiles())
		dwFlags |= SHCONTF_INCLUDEHIDDEN;
	HRESULT hr = psfCurr->EnumObjects(NULL, dwFlags, &pItems);
	while (NOERROR == hr)
	{
		hr = pItems->Next(1, &pidlNext, NULL);
		if (hr == S_FALSE || pidlNext == NULL)// || pidlNext == pidlCopy)
			break;
		pidlCopy = pidlNext;
		mapPidlToHTREEITEM::iterator it = mPidlCurr.find(CShellPidlCompare(psfCurr,pidlNext));
		if (it != mPidlCurr.end())
		{
			mPidlCurr.erase(it);
		}
		else
		{
			SetAttributes((*it).second,psfCurr,pidlNext);
			vPidlNew.push_back(GetShellPidl().CopyItemIDList(pidlNext));
		}
		GetShellPidl().FreePidl(pidlNext);
		pidlNext=NULL;
	}
	if (pItems)
		pItems->Release();
	for(mapPidlToHTREEITEM::iterator it1=mPidlCurr.begin();it1 != mPidlCurr.end();it1++)
	{
		HTREEITEM hDelItem = (*it1).second;
#ifdef _DEBUG
		CString sPath;
		GetShellPidl().SHPidlToPathEx((*it1).first.GetPidl(),sPath,psfCurr);
		TRACE1("Deleting item %s in tree refresh\n",sPath);
#endif
		DeleteItem(hDelItem);
	}
	HTREEITEM hSortItem=NULL;
	for(vecPidl::iterator it2=vPidlNew.begin();it2 != vPidlNew.end();it2++)
	{
		AddFolder(hItem,*it2,psfCurr);
		hSortItem = hItem;
	}
	if (hSortItem)
		Sort(hSortItem,psfCurr);
    // Remove all items from the map
    mPidlCurr.erase(mPidlCurr.begin(),mPidlCurr.end());
    vPidlNew.erase(vPidlNew.begin(),vPidlNew.end());
	psfCurr->Release();
    // Now repeat this procedure for hItem's children.
    hChild = GetChildItem(hItem);

    while (hChild != NULL) 
	{
        RefreshNode(hChild); 
        hChild = GetNextSiblingItem (hChild);
    }
}

void CIEFolderTreeCtrl::Init()
{
	CUITreeCtrl::Init();

	GetShellSettings().GetSettings();
	// TODO: Add your specialized code here and/or call the base class
    // Get the handle to the system image list, for our icons
    SHFILEINFO    sfi;

    m_hImageList = (HIMAGELIST)SHGetFileInfo((LPCTSTR)_T("C:\\"), 
                                           0,
                                           &sfi, 
                                           sizeof(SHFILEINFO), 
                                           SHGFI_SYSICONINDEX | SHGFI_SMALLICON);

    // Attach ImageList to TreeCtrl
    if (m_hImageList)
        ::SendMessage(m_hWnd, TVM_SETIMAGELIST, (WPARAM) TVSIL_NORMAL,(LPARAM)m_hImageList);	
}

void CIEFolderTreeCtrl::OnDestroy()
{
	SetImageList(NULL,TVSIL_NORMAL);
	CUITreeCtrl::OnDestroy();
}

LRESULT CIEFolderTreeCtrl::OnSettingChange(WPARAM wParam,LPARAM lParam)
{ 
	LPCTSTR lpszSection=(LPCTSTR)lParam;
	if (lpszSection == NULL)
		return 0L;
	if (lstrcmpi(lpszSection, _T("ShellState")) == 0) 
	{  
		GetShellSettings().GetSettings();
		Refresh();
    }
	return 1L; 
}  

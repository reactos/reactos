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

// ShellPidl.cpp: implementation of the CShellPidl class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ShellPidl.h"
#include "UIMessages.h"
#include <intshcut.h>
#include <subsmgr.h>
#include <ExDisp.h>
#include "ShellContextMenu.h"
#include "UICoolMenu.h"
#include "cbformats.h"

#define SF_DRAGDROP_FLAGS SFGAO_CANCOPY | SFGAO_CANMOVE | SFGAO_CANLINK;
UINT CF_IDLIST = RegisterClipboardFormat(CFSTR_SHELLIDLIST);
UINT CF_SHELLURL = RegisterClipboardFormat(CFSTR_SHELLURL);

#define TPM_FLAGS               (TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShellPidl::CShellPidl()
{
	SHGetMalloc(&m_pMalloc);
    SHGetDesktopFolder(&m_psfDesktop);
	ZeroMemory(&m_EmptyPidl,sizeof(ITEMIDLIST));
}

CShellPidl::~CShellPidl()
{
	if (m_pMalloc)
		m_pMalloc->Release();
	if (m_psfDesktop)
		m_psfDesktop->Release();
}

IMalloc *CShellPidl::GetMalloc()
{
	return m_pMalloc;
}

DWORD CShellPidl::GetDragDropAttributes(LPLVITEMDATA plvid)
{
	return GetDragDropAttributes(plvid->lpsfParent,plvid->lpi);
}

DWORD CShellPidl::GetDragDropAttributes(LPTVITEMDATA ptvid)
{
	return GetDragDropAttributes(ptvid->lpsfParent,ptvid->lpi);
}

DWORD CShellPidl::GetDragDropAttributes(LPSHELLFOLDER pFolder,LPCITEMIDLIST pidl)
{
	if (pFolder == NULL)
		pFolder = m_psfDesktop;
	DWORD dwAttrs=SF_DRAGDROP_FLAGS;
	HRESULT hr = pFolder->GetAttributesOf(1,(LPCITEMIDLIST*)&pidl,&dwAttrs);
	if (FAILED(hr))
		dwAttrs = SF_DRAGDROP_FLAGS;
	return dwAttrs;
}

DWORD CShellPidl::GetDragDropAttributes(COleDataObject *pDataObject)
{
	IDataObject *pDataObj = pDataObject->m_lpDataObject;

	STGMEDIUM stgm;
	ZeroMemory(&stgm, sizeof(stgm));

    FORMATETC fetc;
    fetc.cfFormat = CF_IDLIST;
    fetc.ptd = NULL;
    fetc.dwAspect = DVASPECT_CONTENT;
    fetc.lindex = -1;
    fetc.tymed = TYMED_HGLOBAL;

	DWORD dwAttrs=DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK;
    HRESULT hr = pDataObj->QueryGetData(&fetc);
	if (FAILED(hr))
		return dwAttrs;
    hr = pDataObj->GetData(&fetc, &stgm);
	if (FAILED(hr))
		return dwAttrs;
    DWORD pData = (DWORD)GlobalLock(stgm.hGlobal);
	LPIDA pIDList = (LPIDA)pData;
	UINT nFolderOffset = pIDList->aoffset[0];
	TRACE2("PIDL(%u) offfset=%u\n",0,nFolderOffset);
	DWORD pFolder = pData+nFolderOffset;
    LPITEMIDLIST pidl = (LPITEMIDLIST)pFolder;
    LPSHELLFOLDER psfParent = GetFolder(pidl);
	if (psfParent)
	{
		// get attributes for the children
		TRACE1("PIDL count=%u\n",pIDList->cidl);
		for(UINT i=1;i < (pIDList->cidl+1);i++)
		{
			UINT nListOffset = pIDList->aoffset[i];
			TRACE2("PIDL(%u) offfset=%u\n",i,nListOffset);
			ULONG ulAttrs = SF_DRAGDROP_FLAGS;
			DWORD dwPidl = pData+nListOffset;
			LPITEMIDLIST pidlist = (LPITEMIDLIST)dwPidl;
			psfParent->GetAttributesOf(1,(LPCITEMIDLIST*)&pidlist,&ulAttrs);
#ifdef _DEBUG
			CString sPath;
			SHPidlToPathEx(pidlist,sPath,psfParent);
			TRACE2("Drag drop source path=%s Attributes=%u\n",sPath,ulAttrs);
#endif
			if (ulAttrs)
				dwAttrs = dwAttrs & ulAttrs;
		}
		psfParent->Release();
	}
	GlobalUnlock(stgm.hGlobal);
    ReleaseStgMedium(&stgm);
	return dwAttrs;
}

LPCITEMIDLIST CShellPidl::GetEmptyPidl()
{
	return &m_EmptyPidl;
}

bool CShellPidl::IsDesktopFolder(LPSHELLFOLDER psFolder)
{
	return psFolder == NULL || psFolder == m_psfDesktop;
}

LPSHELLFOLDER CShellPidl::GetDesktopFolder()
{
	return m_psfDesktop;
}

LPSHELLFOLDER CShellPidl::GetFolder(LPITEMIDLIST pidl)
{
	if (pidl == NULL || pidl->mkid.cb == 0)
		return m_psfDesktop;
	LPSHELLFOLDER pFolder=NULL;
	if (FAILED(m_psfDesktop->BindToObject(pidl, 0, IID_IShellFolder,(LPVOID*)&pFolder)))
		return NULL;
	return pFolder;
}

// CopyItemID - creates an item identifier list containing the first 
//     item identifier in the specified list. 
// Returns a PIDL if successful, or NULL if out of memory. 
LPITEMIDLIST CShellPidl::CopyItemID(LPITEMIDLIST pidl,int n) 
{ 
	// Get the size of the specified item identifier. 
	ASSERT(pidl);
	if (n == 0)
	{
		int cb = pidl->mkid.cb;
		int nSize = cb + sizeof(pidl->mkid.cb);
		// Allocate a new item identifier list. 
		LPITEMIDLIST pidlNew = (LPITEMIDLIST)m_pMalloc->Alloc(nSize); 
		ZeroMemory(pidlNew,nSize);
		if (pidlNew == NULL) 
			return NULL; 
		// Copy the specified item identifier. 
		CopyMemory(pidlNew, pidl, nSize-sizeof(pidl->mkid.cb)); 
		return pidlNew; 
	}
	else
	{
		LPITEMIDLIST pidl_index=NULL;
  		for(int i=0;i < n && pidl->mkid.cb;i++)
		{
			pidl_index = pidl;
			pidl = Next(pidl);
		}
		return pidl_index ? CopyItemID(pidl_index,0) : NULL;
	}
	return NULL;
}

// Returns a PIDL if successful, or NULL if out of memory. 
LPITEMIDLIST CShellPidl::CopyLastItemID(LPITEMIDLIST pidl) 
{ 
	// Get the size of the specified item identifier. 
	ASSERT(pidl);
    if (pidl == NULL)
		return NULL;
	LPITEMIDLIST last_pidl=pidl;
    while (pidl->mkid.cb)
    {
		last_pidl = pidl;
        pidl = Next(pidl);
    }
	if (last_pidl == NULL)
		return NULL;
	return CopyItemID(last_pidl);
}

// copies the absolute pidl up till n
LPITEMIDLIST CShellPidl::CopyAbsItemID(LPITEMIDLIST pidl,int n) 
{ 
	// Get the size of the specified item identifier. 
	ASSERT(pidl);
    if (pidl == NULL)
		return NULL;
	LPITEMIDLIST first_pidl=NULL;
	LPITEMIDLIST abs_pidl=NULL;
	LPITEMIDLIST new_abs_pidl=NULL;
    for(int i=0;i < n && pidl && pidl->mkid.cb;i++)
    {
		first_pidl = CopyItemID(pidl);
		new_abs_pidl = ConcatPidl(abs_pidl,first_pidl);
		if (abs_pidl)
		{
			m_pMalloc->Free(abs_pidl);
		}
		abs_pidl = new_abs_pidl;
		if (first_pidl)
		{
			m_pMalloc->Free(first_pidl);
		}
        pidl = Next(pidl);
    }
	return new_abs_pidl;
}

// Makes a copy of an ITEMIDLIST 
LPITEMIDLIST CShellPidl::CopyItemIDList(LPITEMIDLIST pidl) 
{ 
	// Allocate a new item identifier list. 
	int nSize = GetSize(pidl);
	LPITEMIDLIST pidlNew = (LPITEMIDLIST)m_pMalloc->Alloc(nSize); 
	ZeroMemory(pidlNew,nSize);
	if (pidlNew == NULL) 
		return NULL; 
	// Copy the specified item identifier. 
	CopyMemory(pidlNew, pidl, nSize); 

	return pidlNew; 
}

bool CShellPidl::CompareMemPidls(LPCITEMIDLIST pidl1,LPCITEMIDLIST pidl2) 
{ 
	// Allocate a new item identifier list. 
	if (pidl1 == NULL || pidl2 == NULL)
		return false;
	return memcmp(pidl1,pidl2,(size_t)GetSize(pidl1)) == 0;
}

// Returns true if lists are the same
bool CShellPidl::ComparePidls(LPSHELLFOLDER pFolder,LPCITEMIDLIST pidl1,LPCITEMIDLIST pidl2) 
{ 
	// Allocate a new item identifier list. 
	if (pFolder == NULL)
		pFolder = GetDesktopFolder();
	if (pidl1 == NULL || pidl2 == NULL)
		return false;
	return (short)pFolder->CompareIDs(0,pidl1,pidl2) == 0;
}

void CShellPidl::Free(void *pv)
{
	if (m_pMalloc)
		m_pMalloc->Free(pv);
}

void CShellPidl::FreePidl(LPITEMIDLIST pidl)
{
	if (m_pMalloc)
		m_pMalloc->Free(pidl);
}

UINT CShellPidl::GetCount(LPCITEMIDLIST pidl)
{
    UINT nCount = 0;
    if (pidl)
    {
        while (pidl->mkid.cb)
        {
            pidl = Next(pidl);
			nCount++;
        }
    }
    return nCount;
}

UINT CShellPidl::GetSize(LPCITEMIDLIST pidl)
{
    UINT cbTotal = 0;
    if (pidl)
    {
        cbTotal += sizeof(pidl->mkid.cb);       // Null terminator
        while (pidl->mkid.cb)
        {
            cbTotal += pidl->mkid.cb;
            pidl = Next(pidl);
        }
    }
    return cbTotal;
}

LPITEMIDLIST CShellPidl::Next(LPCITEMIDLIST pidl)
{
   LPSTR lpMem=(LPSTR)pidl;

   lpMem+=pidl->mkid.cb;

   return (LPITEMIDLIST)lpMem;
}

LPITEMIDLIST CShellPidl::ConcatPidl(LPITEMIDLIST pidlDest,LPITEMIDLIST pidlSrc) 
{ 
	// Get the size of the specified item identifier. 
    UINT cbDest=0;
    UINT cbSrc=0;
    if (pidlDest)  //May be NULL
       cbDest = GetSize(pidlDest) - sizeof(pidlDest->mkid.cb);
    cbSrc = GetSize(pidlSrc);

	// Allocate a new item identifier list. 
	LPITEMIDLIST pidlNew = (LPITEMIDLIST)m_pMalloc->Alloc(cbSrc+cbDest); 
	if (pidlNew == NULL) 
		return NULL; 
	ZeroMemory(pidlNew,cbSrc+cbDest);
	// Copy the specified item identifier. 
	if (pidlDest)
		CopyMemory(pidlNew, pidlDest, cbDest); 
	CopyMemory(((USHORT*)(((LPBYTE)pidlNew)+cbDest)), pidlSrc, cbSrc); 

	return pidlNew; 
}

int CShellPidl::GetIcon(LPITEMIDLIST lpi, UINT uFlags)
{
   SHFILEINFO    sfi;
   ZeroMemory(&sfi,sizeof(sfi));
   if (uFlags == 0)
		uFlags |= (SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
   uFlags |= SHGFI_PIDL;
   SHGetFileInfo((LPCTSTR)lpi, 0, &sfi, sizeof(SHFILEINFO), uFlags);

   return sfi.iIcon;
}

STDMETHODIMP CShellPidl::SHPidlToPathEx(LPCITEMIDLIST pidl, CString &sPath, LPSHELLFOLDER pFolder, DWORD dwFlags)
{
	STRRET StrRetFilePath;
	LPTSTR pszFilePath = NULL;
	HRESULT hr=E_FAIL;
	if (pFolder == NULL)
		pFolder = GetDesktopFolder();
	if (pFolder == NULL)
		return E_FAIL;
	hr = pFolder->GetDisplayNameOf(pidl, dwFlags, &StrRetFilePath);
	if (SUCCEEDED(hr))
	{
		StrRetToStr(StrRetFilePath, &pszFilePath, (LPITEMIDLIST)pidl);
		sPath = pszFilePath;
	}
	if (pszFilePath)
		m_pMalloc->Free(pszFilePath);
	return hr;
}

STDMETHODIMP CShellPidl::SHPathToPidlEx(LPCTSTR szPath, LPITEMIDLIST* ppidl, LPSHELLFOLDER pFolder)
{
   OLECHAR wszPath[MAX_PATH] = {0};
   ULONG nCharsParsed = 0;
   LPSHELLFOLDER pShellFolder = NULL;
   BOOL bFreeOnExit = FALSE;
#ifdef UNICODE
   lstrcpy(wszPath,szPath);
#else
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szPath, -1, wszPath, MAX_PATH);
#endif
   // Use the desktop's IShellFolder by default
   if(pFolder == NULL)
   {
      SHGetDesktopFolder(&pShellFolder);
      bFreeOnExit = TRUE;
   }
   else
      pShellFolder = pFolder;

   HRESULT hr = pShellFolder->ParseDisplayName(NULL, NULL, wszPath, &nCharsParsed, ppidl, NULL);

   if(bFreeOnExit)
      pShellFolder->Release();

   return hr;
}

void CShellPidl::GetTypeName(LPITEMIDLIST lpi,CString &sTypeName)
{
   SHFILEINFO    sfi;
   ZeroMemory(&sfi,sizeof(sfi));
   UINT uFlags = SHGFI_PIDL | SHGFI_TYPENAME;
   SHGetFileInfo((LPCTSTR)lpi, 0, &sfi, sizeof(SHFILEINFO), uFlags);
   sTypeName = sfi.szTypeName;
}

void CShellPidl::GetDisplayName(LPITEMIDLIST lpifq,CString &sDisplayName)
{
   SHFILEINFO    sfi;
   ZeroMemory(&sfi,sizeof(sfi));
   UINT uFlags = SHGFI_PIDL | SHGFI_DISPLAYNAME;
   SHGetFileInfo((LPCTSTR)lpifq, 0, &sfi, sizeof(SHFILEINFO), uFlags);
   sDisplayName = sfi.szDisplayName;
}

void CShellPidl::GetNormalAndSelectedIcons (
   LPITEMIDLIST lpifq, int &iImage, int &iSelectedImage)
{
   // Don't check the return value here. 
   // If IGetIcon() fails, you're in big trouble.
   iImage = GetIcon (lpifq, 
      SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
   
   iSelectedImage = GetIcon (lpifq, 
      SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_OPENICON);
   
   return;
}

BOOL CShellPidl::HandleMenuMsg(HWND hwnd, LPSHELLFOLDER lpsfParent,
     LPITEMIDLIST  lpi, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPCONTEXTMENU lpcm=NULL;
    HRESULT hr=lpsfParent->GetUIObjectOf(hwnd,
        1,  // get attributes for this many objects
        (const struct _ITEMIDLIST **)&lpi,
        IID_IContextMenu,
        0,
        (LPVOID *)&lpcm);
    if (SUCCEEDED(hr))  
    {
	    LPCONTEXTMENU2 lpcm2=NULL;
		hr = lpcm->QueryInterface(IID_IContextMenu2,(LPVOID*)&lpcm2);
		lpcm->Release();
		if (SUCCEEDED(hr))  
	    {
			lpcm2->HandleMenuMsg(uMsg,wParam,lParam);
			lpcm2->Release();
		}
		return TRUE;
	}
	return FALSE;
}

BOOL CShellPidl::PopupTheMenu(HWND hwnd, LPSHELLFOLDER lpsfParent,
     LPITEMIDLIST  *plpi, UINT cidl, LPPOINT lppt)
{
 	CMenu menu;
    menu.CreatePopupMenu();
	g_CoolMenuManager.Install(CWnd::FromHandle(hwnd));
	CString sPath;
	if (lpsfParent == NULL)
		lpsfParent = GetDesktopFolder();
	SHPidlToPathEx(*plpi,sPath,lpsfParent);
	CShellContextMenu shell_menu(hwnd,sPath,plpi,cidl,lpsfParent);
	shell_menu.SetMenu(&menu);
    int idCmd = menu.TrackPopupMenu(TPM_FLAGS, lppt->x, lppt->y, CWnd::FromHandle(hwnd));
	shell_menu.InvokeCommand(idCmd);
	g_CoolMenuManager.Uninstall();
	return TRUE;
}

/****************************************************************************
*
*    FUNCTION: GetName(LPSHELLFOLDER lpsf,LPITEMIDLIST  lpi,DWORD dwFlags,
*             LPSTR         lpFriendlyName)
*
*    PURPOSE:  Gets the friendly name for the folder 
*
****************************************************************************/
BOOL CShellPidl::GetName (LPSHELLFOLDER lpsf, LPITEMIDLIST lpi, 
   DWORD dwFlags, CString &sFriendlyName)
{
   BOOL   bSuccess=TRUE;
   STRRET str;

   if (NOERROR==lpsf->GetDisplayNameOf(lpi,dwFlags, &str))
   {
      switch (str.uType)
      {
         case STRRET_WSTR:
#ifdef UNICODE
			 _tcscpy(sFriendlyName.GetBuffer(MAX_PATH),str.pOleStr);
			 sFriendlyName.ReleaseBuffer();
#else
            WideCharToMultiByte(
               CP_ACP,                 // code page
               0,		               // dwFlags
               str.pOleStr,            // lpWideCharStr
               -1,                     // cchWideCharStr
               sFriendlyName.GetBuffer(_MAX_PATH),         // lpMultiByteStr
               _MAX_PATH, // cchMultiByte
               NULL,                   // lpDefaultChar
               NULL);                  // lpUsedDefaultChar
#endif
			sFriendlyName.ReleaseBuffer();	
             break;

         case STRRET_OFFSET:
             sFriendlyName = (LPTSTR)lpi+str.uOffset;
             break;

         case STRRET_CSTR:             
             sFriendlyName = (LPTSTR)str.cStr;
             break;

         default:
             bSuccess = FALSE;
             break;
      }
   }
   else
      bSuccess = FALSE;

   return bSuccess;
}

//
// ResolveChannel: Resolves a Channel Shortcut to its URL
//
STDMETHODIMP CShellPidl::ResolveChannel(IShellFolder* pFolder, LPCITEMIDLIST pidl, LPTSTR* lpszURL)
{
   IShellLink* pShellLink;

   *lpszURL = NULL;  // Assume failure

   // Get a pointer to the IShellLink interface from the given folder
   HRESULT hr = pFolder->GetUIObjectOf(NULL, 1, &pidl, IID_IShellLink, NULL, (LPVOID*)&pShellLink);
   if (SUCCEEDED(hr))
   {
      LPITEMIDLIST pidlChannel;

      // Convert the IShellLink pointer to a PIDL.
      hr = pShellLink->GetIDList(&pidlChannel);
      if (SUCCEEDED(hr))
      {
         IShellFolder* psfDesktop;

         hr = SHGetDesktopFolder(&psfDesktop);
         if (SUCCEEDED(hr))
         {
            STRRET strret;

            hr = psfDesktop->GetDisplayNameOf(pidlChannel, 0, &strret);
            if (SUCCEEDED(hr))
				StrRetToStr(strret, lpszURL, pidlChannel);

            psfDesktop->Release();
         }
      }

      pShellLink->Release();
   }

   return hr;
}

STDMETHODIMP CShellPidl::ResolveHistoryShortcut(LPSHELLFOLDER pFolder,LPCITEMIDLIST *ppidl,CString &sURL)
{
	HRESULT hr=E_FAIL;
	IDataObject *pObj=NULL;
	hr = pFolder->GetUIObjectOf(NULL, 1, ppidl, IID_IDataObject, NULL, (LPVOID*)&pObj);
	if (SUCCEEDED(hr))
	{
		hr = GetURL(pObj,sURL);
		pObj->Release();
	}
	return hr;
}

STDMETHODIMP CShellPidl::GetURL(IDataObject *pDataObj,CString &sURL)
{
	sURL.Empty();
	STGMEDIUM stgm;
	ZeroMemory(&stgm, sizeof(stgm));

    FORMATETC fetc;
    fetc.cfFormat = CF_SHELLURL;
    fetc.ptd = NULL;
    fetc.dwAspect = DVASPECT_CONTENT;
    fetc.lindex = -1;
    fetc.tymed = TYMED_HGLOBAL;

    HRESULT hr = pDataObj->QueryGetData(&fetc);
	if (FAILED(hr))
		return hr;
    hr = pDataObj->GetData(&fetc, &stgm);
	if (FAILED(hr))
		return hr;
    LPCTSTR pData = (LPCTSTR)GlobalLock(stgm.hGlobal);
	sURL = pData;
	GlobalUnlock(stgm.hGlobal);
    ReleaseStgMedium(&stgm);
	return S_OK;
}

//
// ResolveInternetShortcut: Resolves an Internet Shortcut to its URL
//
STDMETHODIMP CShellPidl::ResolveInternetShortcut(LPCTSTR lpszLinkFile, LPTSTR* lpszURL)
{
	IUniformResourceLocator* pUrlLink = NULL;

	*lpszURL=NULL;

	HRESULT hr = CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
							 IID_IUniformResourceLocator, (LPVOID*)&pUrlLink);
	if (FAILED(hr))
		return hr;

	IPersistFile* pPersistFile = NULL;
	hr = pUrlLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile);
	if (SUCCEEDED(hr))
	{
		 // Ensure that the string is Unicode. 
		 WORD wsz[MAX_PATH];  
		#ifdef UNICODE
		 _tcscpy(wsz,lpszLinkFile);
		#else
		 MultiByteToWideChar(CP_ACP, 0, lpszLinkFile, -1, wsz, MAX_PATH);
		#endif
		 // Load the Internet Shortcut from persistent storage.
		 hr = pPersistFile->Load(wsz, STGM_READ);
		 if (SUCCEEDED(hr))
		 {
			hr = pUrlLink->GetURL(lpszURL);
		 }
		 pPersistFile->Release();
	}
	pUrlLink->Release();

	return hr;
}  

//
// ResolveLink: Resolves a Shell Link to its actual folder location
//
STDMETHODIMP CShellPidl::ResolveLink(HWND hWnd,LPCTSTR lpszLinkFile, LPTSTR* lpszURL)
{
   IShellLink* pShellLink = NULL;

   *lpszURL = NULL;   // Assume failure

   HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                 IID_IShellLink, (LPVOID*)&pShellLink); 
   if (SUCCEEDED(hr))
   {
      IPersistFile* pPersistFile = NULL;

      hr = pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile);
      if (SUCCEEDED(hr))
      {
         // Ensure that the string is Unicode. 
         WORD wsz[MAX_PATH];  
#ifdef UNICODE
		 _tcscpy(wsz,lpszLinkFile);
#else
         MultiByteToWideChar(CP_ACP, 0, lpszLinkFile, -1, wsz, MAX_PATH);
#endif

         // Load the shortcut.from persistent storage
         hr = pPersistFile->Load(wsz, STGM_READ);
         if (SUCCEEDED(hr))
         {
            WIN32_FIND_DATA wfd;      

            hr = pShellLink->Resolve(hWnd, SLR_NO_UI); 
            if (NOERROR == hr)
            {
               // Get the path to the link target. 
	   		   *lpszURL = (LPTSTR)m_pMalloc->Alloc(MAX_PATH);  // Must remember to Free later

               hr = pShellLink->GetPath(*lpszURL, MAX_PATH - 1, (WIN32_FIND_DATA*)&wfd, SLGP_UNCPRIORITY);
            }
         }

         pPersistFile->Release();
      }

      pShellLink->Release();
   }

   return hr;
}

//
// This method converts a STRRET structure to a LPTSTR
//
#ifdef UNICODE
STDMETHODIMP CShellPidl::StrRetToStr(STRRET StrRet, LPTSTR* str, LPITEMIDLIST pidl)
{
	HRESULT hr = S_OK;
	int cch;
	LPSTR strOffset;

	*str = NULL;  // Assume failure

	switch (StrRet.uType)
   {
		case STRRET_WSTR: 
			cch = wcslen(StrRet.pOleStr) + 1; // NULL terminator
			*str = (LPTSTR)m_pMalloc->Alloc(cch * sizeof(TCHAR));

			if (*str != NULL)
				lstrcpyn(*str, StrRet.pOleStr, cch);
			else
				hr = E_FAIL;
			break;

		case STRRET_OFFSET: 
			strOffset = (((char *) pidl) + StrRet.uOffset);

			cch = MultiByteToWideChar(CP_OEMCP, 0, strOffset, -1, NULL, 0); 
			*str = (LPTSTR)m_pMalloc->Alloc(cch * sizeof(TCHAR));

			if (*str != NULL)
				MultiByteToWideChar(CP_OEMCP, 0, strOffset, -1, *str, cch); 
			else
				hr = E_FAIL;
			break;

		case STRRET_CSTR: 
			cch = MultiByteToWideChar(CP_OEMCP, 0, StrRet.cStr, -1, NULL, 0); 
			*str = (LPTSTR)m_pMalloc->Alloc(cch * sizeof(TCHAR)); 

			if (*str != NULL)
				MultiByteToWideChar(CP_OEMCP, 0, StrRet.cStr, -1, *str, cch); 
			else
				hr = E_FAIL;

			break;
	} 
 
	return hr;
}
#else // UNICODE not defined
STDMETHODIMP CShellPidl::StrRetToStr(STRRET StrRet, LPTSTR* str, LPITEMIDLIST pidl)
{

	HRESULT hr = S_OK;
	int cch;
	LPSTR strOffset;

	*str = NULL;  // Assume failure

	switch (StrRet.uType)
   {
		case STRRET_WSTR: 
			cch = WideCharToMultiByte(CP_ACP, 0, StrRet.pOleStr, -1, NULL, 0, NULL, NULL); 
			*str = (LPTSTR)m_pMalloc->Alloc(cch * sizeof(TCHAR)); 

			if (*str != NULL)
				WideCharToMultiByte(CP_ACP, 0, StrRet.pOleStr, -1, *str, cch, NULL, NULL); 
			else
				hr = E_FAIL;
			break;

		case STRRET_OFFSET: 
			strOffset = (((char *) pidl) + StrRet.uOffset);

			cch = strlen(strOffset) + 1; // NULL terminator
			*str = (LPTSTR)m_pMalloc->Alloc(cch * sizeof(TCHAR));

			if (*str != NULL)
				strcpy(*str, strOffset);
			else
				hr = E_FAIL;
			break;

		case STRRET_CSTR: 
			cch = strlen(StrRet.cStr) + 1; // NULL terminator
			*str = (LPTSTR)m_pMalloc->Alloc(cch * sizeof(TCHAR));

			if (*str != NULL)
				strcpy(*str, StrRet.cStr);
			else
				hr = E_FAIL;

			break;
	} 

	return hr;
}
#endif

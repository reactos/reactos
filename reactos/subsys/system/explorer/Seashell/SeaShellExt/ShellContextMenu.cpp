/***************************************************************************/
/* NOTE:                                                                   */
/* This document is copyright (c) by Oz Solomonovich.  All non-commercial  */
/* use is allowed, as long as this document is not altered in any way, and */
/* due credit is given.                                                    */
/***************************************************************************/

// ShellContextMenu.cpp : implementation file
//
// Handles the creation of the shell context menu, including the "Send To..."
// sub-menu.  
// Personal note: I think that MS should have made the code to populate and
// handle the "Send To..." sub-menu a part of the shell context menu code.  
// But they didn't, so now we're forced to write a whole lot of spaghetti COM 
// code to do what should have been a trivial part of the OS.  See the code 
// below and judge for yourself.
//
// ==========================================================================  
// HISTORY:   
// ==========================================================================
//    1.01  7 Jul 2000 - Philip Oldaker [philip@masmex.com] - Fixed a problem
//                with the SendTo menu now checking for menu id. 
//                Added support for the new OpenWith menu. 
//                The SendTo Menu is now sorted.


#include "stdafx.h"
#include "ShellContextMenu.h"
#include "UICoolMenu.h"
#include <atlbase.h>
#include "PIDL.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define IDM_SHELLCTXFIRST      20000
#define IDM_SHELLCTXLAST       29999

#define IDM_SENDTOFIRST        30000
#define IDM_SENDTOLAST         32000
#define IDM_OPENWITHFIRST      32001
#define IDM_OPENWITHLAST       32767
#define IDM_SENDTOID           20028
#define IDM_OPENWITHID1        20127
#define IDM_OPENWITHID2        20128

LPSHELLFOLDER is_less_than_pidl::psf;
LPSHELLFOLDER is_greater_than_pidl::psf;

// OpenWith menu registry keys
static LPCTSTR szAppKey = _T("Applications");
static LPCTSTR szOpenWithListKey = _T("OpenWithList");
static LPCTSTR szShellKey = _T("shell");
static LPCTSTR szCommandKey = _T("command");
static LPCTSTR szFileExtKey = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts");
// OpenWith menu entries
static LPCTSTR szMRUListEntry = _T("MRUList");
static LPCTSTR szDisplayNameEntry = _T("FriendlyCache");

static class CShCMInitializer
{
public:
    CShCMInitializer();
    ~CShCMInitializer();

    static LPSHELLFOLDER   m_sfDesktop;
    static LPSHELLFOLDER   m_sfSendTo;
    static CImageList *    m_pShellImageList;
    static CPIDL           m_pidlSendTo;
} stat_data;

LPSHELLFOLDER CShCMInitializer::m_sfDesktop       = NULL;
LPSHELLFOLDER CShCMInitializer::m_sfSendTo        = NULL;
CImageList *  CShCMInitializer::m_pShellImageList = NULL;
CPIDL         CShCMInitializer::m_pidlSendTo;

CShCMInitializer::CShCMInitializer()
{
    HRESULT     hr;
    SHFILEINFO  sfi;

    SHGetDesktopFolder(&m_sfDesktop);

    hr = SHGetSpecialFolderLocation(NULL, CSIDL_SENDTO, m_pidlSendTo);
    if (SUCCEEDED(hr)) 
    {
        hr = m_sfDesktop->BindToObject(m_pidlSendTo, NULL, IID_IShellFolder, 
            (LPVOID *)&m_sfSendTo);
        if (!SUCCEEDED(hr)) 
        {
            m_sfSendTo = NULL;
        }
    } 
    else 
    {
        m_sfSendTo = NULL;
    }

    m_pShellImageList = CImageList::FromHandle((HIMAGELIST)
        SHGetFileInfo((LPCTSTR)_T("C:\\"), 0, &sfi, sizeof(SHFILEINFO), 
        SHGFI_SYSICONINDEX | SHGFI_SMALLICON));
}

CShCMInitializer::~CShCMInitializer()
{
    m_sfSendTo->Release();
    m_sfDesktop->Release();
}



CShellContextMenu::CShellContextMenu(HWND hWnd,const CString &sAbsPath, LPITEMIDLIST *ppidl, UINT cidl, LPSHELLFOLDER psfParent) 
:   m_hWnd(hWnd), 
	m_sAbsPath(sAbsPath),
	m_psfParent(psfParent),
	m_ppidl(ppidl),
	m_cidl(cidl),
	m_pSendToMenu(NULL)
{
    m_lpcm = NULL;
	m_pidl = *ppidl;
	// Initialize button sizes
	m_szOldButtonSize = g_CoolMenuManager.SetButtonSize();
}

CShellContextMenu::~CShellContextMenu()
{
    if (m_lpcm) m_lpcm->Release();

    if (m_pSendToMenu)
    {
        int     i = IDM_SENDTOFIRST;
        void *  pData;

        pData = CCoolMenuManager::GetItemData(*m_pSendToMenu, i);
        while (pData)
        {
            CPIDL toFree((LPITEMIDLIST)pData);
            pData = CCoolMenuManager::GetItemData(*m_pSendToMenu, ++i);
        }
        g_CoolMenuManager.UnconvertMenu(m_pSendToMenu);
        m_pSendToMenu = NULL;
    }
	// Clean up owner draw menus
	STL_FOR_ITERATOR(vecODMenu,m_OwnerDrawMenus)
	{
		g_CoolMenuManager.UnconvertMenu(STL_GET_CURRENT(m_OwnerDrawMenus));
	}
	STL_ERASE_ALL(m_OwnerDrawMenus);
	g_CoolMenuManager.SetButtonSize(m_szOldButtonSize);
	//////////////////////////////
}

/////////////////////////////
// Addition: Philip Oldaker
/////////////////////////////
CString CShellContextMenu::GetExt(const CString &sPath) const
{
	TCHAR szDir[_MAX_PATH];
	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szFileName[_MAX_FNAME];
	TCHAR szExt[_MAX_EXT];
	szDir[0] = szDrive[0] = szFileName[0] = szExt[0] = 0;
	_tsplitpath(sPath,szDrive,szDir,szFileName,szExt);
	return szExt;
}

// A bunch of registry nonsense to return all relevant details
// but especially the application icon
void CShellContextMenu::GetAppDetails(const CString &sAppName,CString &sDisplayName,CString &sCommand,HICON &hIconApp) const
{
	hIconApp = NULL;
	sDisplayName.Empty();
	sCommand.Empty();

	CString sAppKey(szAppKey);
	AddKey(sAppKey,sAppName);
	AddKey(sAppKey,szShellKey);
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_CLASSES_ROOT,sAppKey,0,KEY_READ,&hKey) != ERROR_SUCCESS)
		return;
	BYTE szDisplayName[_MAX_PATH];
	DWORD dwSize=sizeof(szDisplayName);
	DWORD dwType=REG_SZ;
	LONG nRet = RegQueryValueEx(hKey,szDisplayNameEntry,NULL,&dwType,szDisplayName,&dwSize);
	if (nRet != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return;
	}
	sDisplayName = szDisplayName;
	TCHAR szSubKey[_MAX_PATH];
	TCHAR szClass[_MAX_PATH];
	DWORD dwSizeSubKey = sizeof(szSubKey)-1;
	DWORD dwSizeClass = sizeof(szClass)-1;
	ZeroMemory(szSubKey,sizeof(szSubKey));
	ZeroMemory(szClass,sizeof(szClass));
	FILETIME ftLastWriteTime;
	HKEY hCmdKey=NULL;
	CString sCmdKey;
	// Search for a key that contains the "command" key as it may not be under "open"
	for(DWORD dwIndex=0;RegEnumKeyEx(hKey,dwIndex, 
					  szSubKey, 
					  &dwSizeSubKey,
					  NULL,
					  szClass,
					  &dwSizeClass,
					  &ftLastWriteTime) == ERROR_SUCCESS;dwIndex++)
	{
		sCmdKey = szSubKey;
		AddKey(sCmdKey,szCommandKey);
		if (RegOpenKeyEx(hKey,sCmdKey,0,KEY_READ,&hCmdKey) == ERROR_SUCCESS)
		{
			break;
		}
		dwSizeSubKey = sizeof(szSubKey)-1;
		dwSizeClass = sizeof(szClass)-1;
	}
	RegCloseKey(hKey);
	hKey = NULL;
	if (hCmdKey == NULL)
		return;
	dwType=REG_SZ | REG_EXPAND_SZ;
	BYTE szCommand[_MAX_PATH];
	dwSize=sizeof(szCommand);
	nRet = RegQueryValueEx(hCmdKey,NULL,NULL,&dwType,szCommand,&dwSize);
	if (nRet != ERROR_SUCCESS)
		return;
	TCHAR szPath[MAX_PATH];
	sCommand = szCommand;
	if (ExpandEnvironmentStrings(sCommand,szPath,sizeof(szPath)))
		sCommand = szPath;
	CString sIconPath(sCommand);
	sIconPath.MakeLower();
	// Only extract icons from exe's at the moment
	int nPos = sIconPath.Find(_T(".exe"));
	if (nPos == -1)
		return;
	sIconPath = sIconPath.Left(nPos+4);
	// Remove auy quotes
	if (sIconPath.Left(1) == '\"')
		sIconPath = sIconPath.Right(sIconPath.GetLength()-1);
	if (sIconPath.Right(1) == '\"')
		sIconPath = sIconPath.Left(sIconPath.GetLength()-1);
	ExtractIconEx(sIconPath, 0, NULL, &hIconApp, 1);		

	RegCloseKey(hCmdKey);
}

// reg helper
void CShellContextMenu::AddKey(CString &sDestKey,const CString &sSrcKey) const
{
	if (sDestKey.Right(1) != '\\')
		sDestKey += '\\';
	sDestKey += sSrcKey;
}
////////////////////////////////////////

bool CShellContextMenu::IsMenuCommand(int iCmd) const
{
    return (
            (IDM_SENDTOFIRST   <= iCmd  &&  iCmd <= IDM_SENDTOLAST) ||
            (IDM_SHELLCTXFIRST <= iCmd  &&  iCmd <= IDM_SHELLCTXLAST)
           );
}

void CShellContextMenu::InvokeCommand(int iCmd) const
{
	if (!iCmd)
		return;
USES_CONVERSION;
		/////////////////////////////
        if (IDM_SENDTOFIRST <= iCmd  &&  iCmd <= IDM_SENDTOLAST)
        {
            // "Send To..." item

            CPIDL        pidlFile(m_sAbsPath), pidlDrop;
            LPDROPTARGET pDT;
            LPDATAOBJECT pDO;
            HRESULT hr;

            hr = pidlFile.GetUIObjectOf(IID_IDataObject, (LPVOID *)&pDO, 
                m_hWnd);
            if (SUCCEEDED(hr))
            {
                pidlDrop.Set((LPITEMIDLIST)
                    CCoolMenuManager::GetItemData(*m_pSendToMenu, iCmd));
                hr = pidlDrop.GetUIObjectOf(IID_IDropTarget, 
                    (LPVOID *)&pDT, m_hWnd);

                if (SUCCEEDED(hr))
                {
                    // do the drop
                    POINTL pt = { 0, 0 };
                    DWORD dwEffect = 
                        DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK;
                    hr = pDT->DragEnter(pDO, MK_LBUTTON, pt, &dwEffect);

                    if (SUCCEEDED(hr) && dwEffect) 
                    {
                        hr = pDT->Drop(pDO, MK_LBUTTON, pt, &dwEffect);
                    } 
                    else 
                    {
                        hr = pDT->DragLeave();
                    }

                    pDT->Release();
                }

                pidlDrop.m_pidl = NULL;
            }
        }
        else
        {
			DWORD dwItemData=0;
			CMenu *pMenu=NULL;
			STL_FOR_ITERATOR(vecODMenu,m_OwnerDrawMenus)
			{
				pMenu = STL_GET_CURRENT(m_OwnerDrawMenus);
				if (pMenu && pMenu->GetMenuItemCount() > 0)
				{
					// switch to original item data
					if ((UINT)iCmd >= pMenu->GetMenuItemID(0) && (UINT)iCmd <= pMenu->GetMenuItemID(pMenu->GetMenuItemCount()-1))
					{
						dwItemData = g_CoolMenuManager.SwitchContextItemData(pMenu,iCmd,0,FALSE);
						break;
					}
				}
			}
			CWaitCursor w;
            // Shell command
            CMINVOKECOMMANDINFO cmi;
            cmi.cbSize       = sizeof(cmi);
            cmi.fMask        = 0;
            cmi.hwnd         = m_hWnd;
            cmi.lpVerb       = T2CA(MAKEINTRESOURCE(iCmd - IDM_SHELLCTXFIRST));
            cmi.lpParameters = NULL;
            cmi.lpDirectory  = NULL;
            cmi.nShow        = SW_SHOWNORMAL;
            cmi.dwHotKey     = 0;
            cmi.hIcon        = NULL;
            m_lpcm->InvokeCommand(&cmi);
			// switch back to our item data
			if (dwItemData)
			{
				g_CoolMenuManager.SwitchContextItemData(pMenu,iCmd,dwItemData,FALSE);
			}
        }
}

// retrieves the shell context menu for a file
void CShellContextMenu::SetMenu(CMenu *pMenu)
{
    if (m_sAbsPath.IsEmpty())
		return;
	ASSERT(m_psfParent);
    HRESULT hr = m_psfParent->GetUIObjectOf(m_hWnd, m_cidl, (LPCITEMIDLIST*)m_ppidl, IID_IContextMenu, NULL, (LPVOID *)&m_lpcm);
// NOTE: Slight change to return here but change it back if you want: PO
    if (FAILED(hr))
		return;
	
    g_CoolMenuManager.UnconvertMenu(pMenu);
	g_CoolMenuManager.SetShellContextMenu(m_lpcm,IDM_SHELLCTXFIRST,IDM_SHELLCTXLAST);

    pMenu->DeleteMenu(0, MF_BYPOSITION);
    hr = m_lpcm->QueryContextMenu(*pMenu, 0, IDM_SHELLCTXFIRST, 
        IDM_SHELLCTXLAST, CMF_EXPLORE);
	/////////////////////////////
	// Additions Philip Oldaker 
	// Check for IContextMenu2 used by owner draw menus
    CMenu *pSubMenu;
    int count = pMenu->GetMenuItemCount();
    for (int i = 0; i < count; i++)
    {
		CMenuItemInfo mii;
		mii.fMask = MIIM_TYPE | MIIM_ID;
		pMenu->GetMenuItemInfo(i,&mii,TRUE);
		if (mii.fType & MFT_OWNERDRAW)
		{
			TRACE(_T("OwnerDraw menu item found in CShellContextMenu\n"));
            g_CoolMenuManager.AddShellContextMenu(pMenu,m_lpcm,i);
			STL_ADD_ITEM(m_OwnerDrawMenus,pMenu);
		}
        pSubMenu = pMenu->GetSubMenu(i);
		/////////////////////////////
		// Additions Philip Oldaker 
		// Search for owner draw menus and add the ICM2 pointer
		if (pSubMenu)
		{
			// NOTE: Remove the TRACE statements if not needed
			UINT nSubCount = pSubMenu->GetMenuItemCount();
			TRACE(_T("Count=%u\n"),nSubCount);
			for(UINT m=0;m < nSubCount;m++)
			{
				CMenuItemInfo mii;
				mii.fMask = MIIM_TYPE;
				pSubMenu->GetMenuItemInfo(m,&mii,TRUE);
				if (mii.fType & MFT_OWNERDRAW)
				{
					TRACE(_T("OwnerDraw submenu found in CShellContextMenu\n"));
                    g_CoolMenuManager.AddShellContextMenu(pSubMenu,m_lpcm,m);
					STL_ADD_ITEM(m_OwnerDrawMenus,pSubMenu);
				}
			}
		}
		/////////////////////////////
	    // find the "Send To" and the new "Open With" submenu: look for a defined menu item id
        if (pSubMenu && pSubMenu->GetMenuItemCount() == 1)
        {
		    CString str;
			pSubMenu->GetMenuString(0,str,MF_BYPOSITION);
            UINT idmFirst = pSubMenu->GetMenuItemID(0);
			TRACE(_T("Menu Item %s=%d\n"),str,idmFirst);
			/////////////////////////////
			// Additions Philip Oldaker 
			// Populate the OpenWith menu
			if (idmFirst == IDM_OPENWITHID1 || idmFirst == IDM_OPENWITHID2)
			{
				// ok - found it.  now populate it
				IContextMenu2 *lpcm2=NULL;
				HRESULT hr = m_lpcm->QueryInterface(IID_IContextMenu2,(LPVOID*)&lpcm2);
				if (SUCCEEDED(hr))
				{
					hr = lpcm2->HandleMenuMsg(WM_INITMENUPOPUP,(WPARAM)pSubMenu->GetSafeHmenu(),0);
					lpcm2->Release();
					// We need the extension to search the registry for valid applications
					CString sExt(GetExt(m_sAbsPath));
					// No extension then there's nothing to do
					if (sExt.IsEmpty())
						continue;
					FillOpenWithMenu(pSubMenu,sExt);
					STL_ADD_ITEM(m_OwnerDrawMenus,pSubMenu);
				}
			}
            else if (idmFirst == IDM_SENDTOID)
            {
	            UINT idm = IDM_SENDTOFIRST;
                // ok - found it.  now populate it
                m_pSendToMenu = CMenu::FromHandle(::CreatePopupMenu());
                ASSERT_VALID(m_pSendToMenu);
                pSubMenu->DestroyMenu();
                pMenu->ModifyMenu(i, MF_BYPOSITION | MF_POPUP, 
                    (UINT)m_pSendToMenu->m_hMenu, str);
                FillSendToMenu(m_pSendToMenu, stat_data.m_sfSendTo, idm);
            }
			/////////////////////////////
        }
    }
// Not needed I think :PO
    return;
}

////////////////////////////
// Additions: Philip Oldaker
// Scan the registry looking for a OpenWithList key in this format
// Entry MRUList contains the alphabetical order eg. afgrde for each entry
// each of these entries points to the application name which in turn
// has the command and display name
////////////////////////////
void CShellContextMenu::FillOpenWithMenu(CMenu *pMenu,const CString &sExt)
{
	HKEY hKey;
	CString sFileExtKey(szFileExtKey);
	AddKey(sFileExtKey,sExt);
	AddKey(sFileExtKey,szOpenWithListKey);
	if (RegOpenKeyEx(HKEY_CURRENT_USER,sFileExtKey,0,KEY_READ,&hKey) != ERROR_SUCCESS)
		return;
	BYTE szMRUList[_MAX_PATH];
	szMRUList[0] = 0;
	DWORD dwSize=sizeof(szMRUList);
	DWORD dwType=REG_SZ;
	RegQueryValueEx(hKey,szMRUListEntry,NULL,&dwType,szMRUList,&dwSize);
	int nLen = _tcslen((LPCTSTR)szMRUList);
	TCHAR szOpenWithItemEntry[sizeof(TCHAR)*2];
	ZeroMemory(szOpenWithItemEntry,sizeof(szOpenWithItemEntry));
	BYTE szOpenWithItem[_MAX_PATH];
	CString sDisplayName;
	HICON hIconApp;
	CString sCommand;
	CString sMenuText;
	for(int i=0;i < nLen;i++)
	{
		szOpenWithItemEntry[0] = szMRUList[i];
		dwSize = sizeof(szOpenWithItem);
		RegQueryValueEx(hKey,szOpenWithItemEntry,NULL,&dwType,szOpenWithItem,&dwSize);
		GetAppDetails(szOpenWithItem, sDisplayName, sCommand, hIconApp);
		if (!sCommand.IsEmpty())
		{
			for(UINT idm=0;idm < pMenu->GetMenuItemCount();idm++)
			{
				pMenu->GetMenuString(idm,sMenuText,MF_BYPOSITION);
				if (sMenuText == sDisplayName)
				{
					g_CoolMenuManager.ConvertMenuItem(pMenu, idm);
					CCoolMenuManager::SetItemIcon(*pMenu, hIconApp, idm, TRUE);
					break;
				}
			}
		}
	}
	RegCloseKey(hKey);
}

void CShellContextMenu::FillSendToMenu(CMenu *pMenu, LPSHELLFOLDER pSF, 
                                       UINT &idm)
{
    if (pSF == NULL) 
		return;
    USES_CONVERSION;
    CPIDL           pidl, abspidl;
    LPENUMIDLIST    peidl;
    HRESULT         hr;
    STRRET          str;
    UINT            idmStart = idm;
    LPSHELLFOLDER   pSubSF;
    SHFILEINFO      sfi;
	vecCMSort vMenuItems;

    int idx_folder = 0; // folder insertion index

    hr = pSF->EnumObjects(m_hWnd, 
        SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &peidl);
    if (FAILED(hr))
		return;
    g_CoolMenuManager.ConvertMenu(pMenu);
    while (peidl->Next(1, pidl, NULL) == S_OK  &&
           idm < IDM_SENDTOLAST) 
    {
        hr = pSF->GetDisplayNameOf(pidl, SHGDN_NORMAL, &str);
        if (SUCCEEDED(hr)) 
        {
            ULONG ulAttrs = (unsigned)-1;
            pSF->GetAttributesOf(1, pidl, &ulAttrs);
            abspidl.MakeAbsPIDLOf(pSF, pidl);
            SHGetFileInfo((LPCTSTR)abspidl.m_pidl, 0, &sfi, sizeof(sfi),
                SHGFI_PIDL | SHGFI_ICON | SHGFI_SMALLICON);
            pidl.ExtractCStr(str);
            if (ulAttrs & SFGAO_FOLDER) // folder?
            {
                // create new submenu & recurse
                HMENU hSubMenu = ::CreateMenu();
                pMenu->InsertMenu(idx_folder, 
                    MF_POPUP | MF_BYPOSITION | MF_STRING, 
                    (UINT)hSubMenu, A2T(str.cStr));
                g_CoolMenuManager.ConvertMenuItem(pMenu, 
                    idx_folder);
                CCoolMenuManager::SetItemIcon(*pMenu, 
                    sfi.hIcon, idx_folder, TRUE);
                idx_folder++;
                hr = pSF->BindToObject(pidl, NULL, 
                    IID_IShellFolder, (LPVOID *)&pSubSF);
                if (!SUCCEEDED(hr)) pSubSF = NULL;
                FillSendToMenu(CMenu::FromHandle(hSubMenu), pSubSF, 
                    idm);
                if (pSubSF) pSubSF->Release();
                abspidl.Free();
            }
            else
            {
				CShCMSort *pSMI = new CShCMSort(idm,pidl.m_pidl,sfi.hIcon,A2T(str.cStr),(DWORD)abspidl.m_pidl);
				STL_ADD_ITEM(vMenuItems,pSMI);
                idm++;
            }
            abspidl.m_pidl = NULL;
        }
    }
    peidl->Release();
// Addition: Philip Oldaker
// To keep it familiar the SendTo menu is sorted
	STL_SORT(vMenuItems,pSF,STL_SORT_FUNC);
	CShCMSort *pItem=NULL;
	STL_FOR_ITERATOR(vecCMSort,vMenuItems)
	{
		pItem = STL_GET_CURRENT(vMenuItems);
		pMenu->AppendMenu(MF_STRING, pItem->GetItemID(), pItem->GetText());
		g_CoolMenuManager.ConvertMenuItem(pMenu, 
			pMenu->GetMenuItemCount() - 1);
		CCoolMenuManager::SetItemIcon(*pMenu, 
			pItem->GetIcon(), pItem->GetItemID());
		CCoolMenuManager::SetItemData(*pMenu, 
			(void*)pItem->GetItemData(), pItem->GetItemID());
        CPIDL toFree(pItem->GetPidl());
		delete pItem;
	}
	STL_ERASE_ALL(vMenuItems);
//////////////////////////////////
    // If the menu is still empty (the user has an empty SendTo folder),
    // then add a disabled "(empty)" item so we have at least something
    // to display.
    if (idm == idmStart) 
    {
        pMenu->AppendMenu(MF_GRAYED | MF_DISABLED | MF_STRING, idm, 
            _T("(empty)"));
        idm++;
    }
}

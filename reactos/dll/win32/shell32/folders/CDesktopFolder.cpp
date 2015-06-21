/*
 *    Virtual Desktop Folder
 *
 *    Copyright 1997                Marcus Meissner
 *    Copyright 1998, 1999, 2002    Juergen Schmied
 *    Copyright 2009                Andrew Hill
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/*
CDesktopFolder should create two file system folders internally, one representing the
user's desktop folder, and the other representing the common desktop folder. It should
also create a CRegFolder to represent the virtual items that exist only in the registry.
The CRegFolder is aggregated by the CDesktopFolder, and queries for the CLSID_IShellFolder,
CLSID_IShellFolder2, or CLSID_IShellIconOverlay interfaces prefer the CRegFolder
implementation.
The CDesktopFolderEnum class should create two enumerators, one for each of the file
system folders, and enumerate the contents of each folder. Since the CRegFolder
implementation of IShellFolder::EnumObjects enumerates the virtual items, the
CDesktopFolderEnum is only responsible for returning the physical items.
CDesktopFolderEnum is incorrect where it filters My Computer from the enumeration
if the new start menu is used. The CDesktopViewCallback is responsible for filtering
it from the view by handling the IncludeObject query to return S_FALSE. The enumerator
always shows My Computer.
*/

/* Undocumented functions from shdocvw */
extern "C" HRESULT WINAPI IEParseDisplayNameWithBCW(DWORD codepage, LPCWSTR lpszDisplayName, LPBC pbc, LPITEMIDLIST *ppidl);

/***********************************************************************
*     Desktopfolder implementation
*/

class CDesktopFolderDropTarget :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDropTarget
{
    private:
        CComPtr<IShellFolder> m_psf;
        BOOL m_fAcceptFmt;       /* flag for pending Drop */
        UINT m_cfShellIDList;    /* clipboardformat for IDropTarget */
        
        void SF_RegisterClipFmt();
        BOOL QueryDrop (DWORD dwKeyState, LPDWORD pdwEffect);
    public:
        CDesktopFolderDropTarget();
        
        HRESULT WINAPI Initialize(IShellFolder *psf);

        // IDropTarget
        virtual HRESULT WINAPI DragEnter(IDataObject *pDataObject, DWORD dwKeyState, POINTL pt, DWORD *pdwEffect);
        virtual HRESULT WINAPI DragOver(DWORD dwKeyState, POINTL pt, DWORD *pdwEffect);
        virtual HRESULT WINAPI DragLeave();
        virtual HRESULT WINAPI Drop(IDataObject *pDataObject, DWORD dwKeyState, POINTL pt, DWORD *pdwEffect);

        BEGIN_COM_MAP(CDesktopFolderDropTarget)
        COM_INTERFACE_ENTRY_IID(IID_IDropTarget, IDropTarget)
        END_COM_MAP()
};

class CDesktopFolderEnum :
    public CEnumIDListBase
{
    private:
//    CComPtr                                fDesktopEnumerator;
//    CComPtr                                fCommonDesktopEnumerator;
    public:
        CDesktopFolderEnum();
        ~CDesktopFolderEnum();
        HRESULT WINAPI Initialize(CDesktopFolder *desktopFolder, HWND hwndOwner, DWORD dwFlags);

        BEGIN_COM_MAP(CDesktopFolderEnum)
        COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
        END_COM_MAP()
};

int SHELL_ConfirmMsgBox(HWND hWnd, LPWSTR lpszText, LPWSTR lpszCaption, HICON hIcon, BOOL bYesToAll);

static const shvheader DesktopSFHeader[] = {
    {IDS_SHV_COLUMN1, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},
    {IDS_SHV_COLUMN2, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN3, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN4, SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 12},
    {IDS_SHV_COLUMN5, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 5}
};

#define DESKTOPSHELLVIEWCOLUMNS 5

CDesktopFolderEnum::CDesktopFolderEnum()
{
}

CDesktopFolderEnum::~CDesktopFolderEnum()
{
}

static const WCHAR ClassicStartMenuW[] = L"SOFTWARE\\Microsoft\\Windows\\"
    L"CurrentVersion\\Explorer\\HideDesktopIcons\\ClassicStartMenu";

static INT
IsNamespaceExtensionHidden(const WCHAR *iid)
{
    DWORD Result, dwResult;
    dwResult = sizeof(DWORD);

    if (RegGetValueW(HKEY_CURRENT_USER, /* FIXME use NewStartPanel when activated */
                     ClassicStartMenuW,
                     iid,
                     RRF_RT_DWORD,
                     NULL,
                     &Result,
                     &dwResult) != ERROR_SUCCESS)
    {
        return -1;
    }

    return Result;
}

static VOID
SetNamespaceExtensionVisibleStatus(const WCHAR * iid, DWORD dwStatus)
{
    HKEY hKey;

    if (RegOpenKeyExW(HKEY_CURRENT_USER, ClassicStartMenuW, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
    {
        RegSetValueExW(hKey, iid, 0, REG_DWORD, (LPBYTE)&dwStatus, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

/**************************************************************************
 *  CreateDesktopEnumList()
 */

HRESULT WINAPI CDesktopFolderEnum::Initialize(CDesktopFolder *desktopFolder, HWND hwndOwner, DWORD dwFlags)
{
    BOOL ret = TRUE;
    WCHAR szPath[MAX_PATH];

    static const WCHAR MyDocumentsClassString[] = L"{450D8FBA-AD25-11D0-98A8-0800361B1103}";
    static const WCHAR Desktop_NameSpaceW[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Desktop\\Namespace";

    TRACE("(%p)->(flags=0x%08x)\n", this, dwFlags);

    /* enumerate the root folders */
    if (dwFlags & SHCONTF_FOLDERS)
    {
        HKEY hkey;
        UINT i;
        DWORD dwResult;

        /* create the pidl for This item */
        if (IsNamespaceExtensionHidden(MyDocumentsClassString) < 1)
        {
            ret = AddToEnumList(_ILCreateMyDocuments());
        }
        ret = AddToEnumList(_ILCreateMyComputer());

        for (i = 0; i < 2; i++)
        {
            if (i == 0)
                dwResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, Desktop_NameSpaceW, 0, KEY_READ, &hkey);
            else
                dwResult = RegOpenKeyExW(HKEY_CURRENT_USER, Desktop_NameSpaceW, 0, KEY_READ, &hkey);

            if (dwResult == ERROR_SUCCESS)
            {
                WCHAR iid[50];
                LPITEMIDLIST pidl;
                int i = 0;

                while (ret)
                {
                    DWORD size;
                    LONG r;

                    size = sizeof (iid) / sizeof (iid[0]);
                    r = RegEnumKeyExW(hkey, i, iid, &size, 0, NULL, NULL, NULL);
                    if (ERROR_SUCCESS == r)
                    {
                        if (IsNamespaceExtensionHidden(iid) < 1)
                        {
                            pidl = _ILCreateGuidFromStrW(iid);
                            if (pidl != NULL)
                            {
                                if (!HasItemWithCLSID(pidl))
                                {
                                    ret = AddToEnumList(pidl);
                                }
                                else
                                {
                                    SHFree(pidl);
                                }
                            }
                        }
                    }
                    else if (ERROR_NO_MORE_ITEMS == r)
                        break;
                    else
                        ret = FALSE;
                    i++;
                }
                RegCloseKey(hkey);
            }
        }
        for (i = 0; i < 2; i++)
        {
            if (i == 0)
                dwResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, ClassicStartMenuW, 0, KEY_READ, &hkey);
            else
                dwResult = RegOpenKeyExW(HKEY_CURRENT_USER, ClassicStartMenuW, 0, KEY_READ, &hkey);

            if (dwResult == ERROR_SUCCESS)
            {
                DWORD j = 0, dwVal, Val, dwType, dwIID;
                LONG r;
                WCHAR iid[50];

                while(ret)
                {
                    dwVal = sizeof(Val);
                    dwIID = sizeof(iid) / sizeof(WCHAR);

                    r = RegEnumValueW(hkey, j++, iid, &dwIID, NULL, &dwType, (LPBYTE)&Val, &dwVal);
                    if (r == ERROR_SUCCESS)
                    {
                        if (Val == 0 && dwType == REG_DWORD)
                        {
                            LPITEMIDLIST pidl = _ILCreateGuidFromStrW(iid);
                            if (pidl != NULL)
                            {
                                if (!HasItemWithCLSID(pidl))
                                {
                                    AddToEnumList(pidl);
                                }
                                else
                                {
                                    SHFree(pidl);
                                }
                            }
                        }
                    }
                    else if (ERROR_NO_MORE_ITEMS == r)
                        break;
                    else
                        ret = FALSE;
                }
                RegCloseKey(hkey);
            }

        }
    }

    /* enumerate the elements in %windir%\desktop */
    ret = ret && SHGetSpecialFolderPathW(0, szPath, CSIDL_DESKTOPDIRECTORY, FALSE);
    ret = ret && CreateFolderEnumList(szPath, dwFlags);

    ret = ret && SHGetSpecialFolderPathW(0, szPath, CSIDL_COMMON_DESKTOPDIRECTORY, FALSE);
    ret = ret && CreateFolderEnumList(szPath, dwFlags);

    return ret ? S_OK : E_FAIL;
}

CDesktopFolder::CDesktopFolder() :
    sPathTarget(NULL),
    pidlRoot(NULL)
{
}

CDesktopFolder::~CDesktopFolder()
{
}

HRESULT WINAPI CDesktopFolder::FinalConstruct()
{
    WCHAR                                szMyPath[MAX_PATH];

    if (!SHGetSpecialFolderPathW( 0, szMyPath, CSIDL_DESKTOPDIRECTORY, TRUE ))
        return E_UNEXPECTED;

    pidlRoot = _ILCreateDesktop();    /* my qualified pidl */
    sPathTarget = (LPWSTR)SHAlloc((wcslen(szMyPath) + 1) * sizeof(WCHAR));
    wcscpy(sPathTarget, szMyPath);
    return S_OK;
}

/**************************************************************************
 *    CDesktopFolder::ParseDisplayName
 *
 * NOTES
 *    "::{20D04FE0-3AEA-1069-A2D8-08002B30309D}" and "" binds
 *    to MyComputer
 */
HRESULT WINAPI CDesktopFolder::ParseDisplayName(
    HWND hwndOwner,
    LPBC pbc,
    LPOLESTR lpszDisplayName,
    DWORD *pchEaten,
    PIDLIST_RELATIVE *ppidl,
    DWORD *pdwAttributes)
{
    WCHAR szElement[MAX_PATH];
    LPCWSTR szNext = NULL;
    LPITEMIDLIST pidlTemp = NULL;
    PARSEDURLW urldata;
    HRESULT hr = S_OK;
    CLSID clsid;

    TRACE ("(%p)->(HWND=%p,%p,%p=%s,%p,pidl=%p,%p)\n",
           this, hwndOwner, pbc, lpszDisplayName, debugstr_w(lpszDisplayName),
           pchEaten, ppidl, pdwAttributes);

    if (!ppidl)
        return E_INVALIDARG;

    if (!lpszDisplayName)
    {
        *ppidl = NULL;
        return E_INVALIDARG;
    }

    *ppidl = NULL;

    if (pchEaten)
        *pchEaten = 0;        /* strange but like the original */

    urldata.cbSize = sizeof(urldata);

    if (lpszDisplayName[0] == ':' && lpszDisplayName[1] == ':')
    {
        szNext = GetNextElementW (lpszDisplayName, szElement, MAX_PATH);
        TRACE ("-- element: %s\n", debugstr_w (szElement));
        CLSIDFromString (szElement + 2, &clsid);
        pidlTemp = _ILCreateGuid (PT_GUID, clsid);
    }
    else if (PathGetDriveNumberW (lpszDisplayName) >= 0)
    {
        /* it's a filesystem path with a drive. Let MyComputer/UnixDosFolder parse it */
        pidlTemp = _ILCreateMyComputer ();
        szNext = lpszDisplayName;
    }
    else if (PathIsUNCW(lpszDisplayName))
    {
        pidlTemp = _ILCreateNetwork();
        szNext = lpszDisplayName;
    }
    else if( (pidlTemp = SHELL32_CreatePidlFromBindCtx(pbc, lpszDisplayName)) )
    {
        *ppidl = pidlTemp;
        return S_OK;
    }
    else if (SUCCEEDED(ParseURLW(lpszDisplayName, &urldata)))
    {
        if (urldata.nScheme == URL_SCHEME_SHELL) /* handle shell: urls */
        {
            TRACE ("-- shell url: %s\n", debugstr_w(urldata.pszSuffix));
            SHCLSIDFromStringW (urldata.pszSuffix + 2, &clsid);
            pidlTemp = _ILCreateGuid (PT_GUID, clsid);
        }
        else
            return IEParseDisplayNameWithBCW(CP_ACP, lpszDisplayName, pbc, ppidl);
    }
    else
    {
        /* it's a filesystem path on the desktop. Let a FSFolder parse it */

        if (*lpszDisplayName)
        {
            WCHAR szPath[MAX_PATH];
            LPWSTR pathPtr;

            /* build a complete path to create a simple pidl */
            lstrcpynW(szPath, sPathTarget, MAX_PATH);
            pathPtr = PathAddBackslashW(szPath);
            if (pathPtr)
            {
                lstrcpynW(pathPtr, lpszDisplayName, MAX_PATH - (pathPtr - szPath));
                hr = _ILCreateFromPathW(szPath, &pidlTemp);
            }
            else
            {
                /* should never reach here, but for completeness */
                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            }
        }
        else
            pidlTemp = _ILCreateMyComputer();

        szNext = NULL;
    }

    if (SUCCEEDED(hr) && pidlTemp)
    {
        if (szNext && *szNext)
        {
            hr = SHELL32_ParseNextElement(this, hwndOwner, pbc,
                                          &pidlTemp, (LPOLESTR) szNext, pchEaten, pdwAttributes);
        }
        else
        {
            if (pdwAttributes && *pdwAttributes)
                hr = SHELL32_GetItemAttributes((IShellFolder *)this,
                                               pidlTemp, pdwAttributes);
        }
    }

    if (SUCCEEDED(hr))
        *ppidl = pidlTemp;
    else
        *ppidl = NULL;

    TRACE ("(%p)->(-- ret=0x%08x)\n", this, hr);

    return hr;
}

/**************************************************************************
 *        CDesktopFolder::EnumObjects
 */
HRESULT WINAPI CDesktopFolder::EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
{
    return ShellObjectCreatorInit<CDesktopFolderEnum>(this, hwndOwner, dwFlags, IID_IEnumIDList, ppEnumIDList);
}

/**************************************************************************
 *        CDesktopFolder::BindToObject
 */
HRESULT WINAPI CDesktopFolder::BindToObject(
    PCUIDLIST_RELATIVE pidl,
    LPBC pbcReserved,
    REFIID riid,
    LPVOID *ppvOut)
{
    TRACE ("(%p)->(pidl=%p,%p,%s,%p)\n",
           this, pidl, pbcReserved, shdebugstr_guid (&riid), ppvOut);

    return SHELL32_BindToChild( pidlRoot, sPathTarget, pidl, riid, ppvOut );
}

/**************************************************************************
 *    CDesktopFolder::BindToStorage
 */
HRESULT WINAPI CDesktopFolder::BindToStorage(
    PCUIDLIST_RELATIVE pidl,
    LPBC pbcReserved,
    REFIID riid,
    LPVOID *ppvOut)
{
    FIXME ("(%p)->(pidl=%p,%p,%s,%p) stub\n",
           this, pidl, pbcReserved, shdebugstr_guid (&riid), ppvOut);

    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
 *     CDesktopFolder::CompareIDs
 */
HRESULT WINAPI CDesktopFolder::CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
    int nReturn;

    TRACE ("(%p)->(0x%08lx,pidl1=%p,pidl2=%p)\n", this, lParam, pidl1, pidl2);
    nReturn = SHELL32_CompareIDs ((IShellFolder *)this, lParam, pidl1, pidl2);
    TRACE ("-- %i\n", nReturn);
    return nReturn;
}

/**************************************************************************
 *    CDesktopFolder::CreateViewObject
 */
HRESULT WINAPI CDesktopFolder::CreateViewObject(
    HWND hwndOwner,
    REFIID riid,
    LPVOID *ppvOut)
{
    CComPtr<IShellView> pShellView;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(hwnd=%p,%s,%p)\n",
           this, hwndOwner, shdebugstr_guid (&riid), ppvOut);

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if (IsEqualIID (riid, IID_IDropTarget))
    {
        hr = ShellObjectCreatorInit<CDesktopFolderDropTarget>(this, IID_IDropTarget, ppvOut);
    }
    else if (IsEqualIID (riid, IID_IContextMenu))
    {
        WARN ("IContextMenu not implemented\n");
        hr = E_NOTIMPL;
    }
    else if (IsEqualIID (riid, IID_IShellView))
    {
        hr = IShellView_Constructor((IShellFolder *)this, &pShellView);
        if (pShellView)
            hr = pShellView->QueryInterface(riid, ppvOut);
    }
    TRACE ("-- (%p)->(interface=%p)\n", this, ppvOut);
    return hr;
}

/**************************************************************************
 *  CDesktopFolder::GetAttributesOf
 */
HRESULT WINAPI CDesktopFolder::GetAttributesOf(
    UINT cidl,
    PCUITEMID_CHILD_ARRAY apidl,
    DWORD *rgfInOut)
{
    HRESULT hr = S_OK;
    static const DWORD dwDesktopAttributes =
        SFGAO_HASSUBFOLDER | SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_FILESYSANCESTOR |
        SFGAO_STORAGEANCESTOR | SFGAO_HASPROPSHEET | SFGAO_STORAGE | SFGAO_CANLINK;
    static const DWORD dwMyComputerAttributes =
        SFGAO_CANRENAME | SFGAO_CANDELETE | SFGAO_HASPROPSHEET | SFGAO_DROPTARGET |
        SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_CANLINK;
    static DWORD dwMyNetPlacesAttributes =
        SFGAO_CANRENAME | SFGAO_CANDELETE | SFGAO_HASPROPSHEET | SFGAO_DROPTARGET |
        SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_CANLINK;

    TRACE("(%p)->(cidl=%d apidl=%p mask=%p (0x%08x))\n",
          this, cidl, apidl, rgfInOut, rgfInOut ? *rgfInOut : 0);

    if (cidl && !apidl)
        return E_INVALIDARG;

    if (*rgfInOut == 0)
        *rgfInOut = ~0;

    if(cidl == 0)
        *rgfInOut &= dwDesktopAttributes;
    else
    {
        /* TODO: always add SFGAO_CANLINK */
        for (UINT i = 0; i < cidl; ++i)
        {
            pdump(*apidl);
            if (_ILIsDesktop(*apidl))
                *rgfInOut &= dwDesktopAttributes;
            else if (_ILIsMyComputer(apidl[i]))
                *rgfInOut &= dwMyComputerAttributes;
            else if (_ILIsNetHood(apidl[i]))
                *rgfInOut &= dwMyNetPlacesAttributes;
            else if (_ILIsSpecialFolder(apidl[i]))
                SHELL32_GetItemAttributes(this, apidl[i], rgfInOut);
            else if(_ILIsFolder(apidl[i]) || _ILIsValue(apidl[i]))
                SHELL32_GetItemAttributes(this, apidl[i], rgfInOut);
            else
                ERR("Got an unknown pidl type!!!\n");
        }
    }
    /* make sure SFGAO_VALIDATE is cleared, some apps depend on that */
    *rgfInOut &= ~SFGAO_VALIDATE;

    TRACE("-- result=0x%08x\n", *rgfInOut);

    return hr;
}

/**************************************************************************
 *    CDesktopFolder::GetUIObjectOf
 *
 * PARAMETERS
 *  HWND           hwndOwner, //[in ] Parent window for any output
 *  UINT           cidl,      //[in ] array size
 *  LPCITEMIDLIST* apidl,     //[in ] simple pidl array
 *  REFIID         riid,      //[in ] Requested Interface
 *  UINT*          prgfInOut, //[   ] reserved
 *  LPVOID*        ppvObject) //[out] Resulting Interface
 *
 */
HRESULT WINAPI CDesktopFolder::GetUIObjectOf(
    HWND hwndOwner,
    UINT cidl,
    PCUITEMID_CHILD_ARRAY apidl,
    REFIID riid,
    UINT *prgfInOut,
    LPVOID *ppvOut)
{
    LPITEMIDLIST pidl;
    IUnknown *pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(%p,%u,apidl=%p,%s,%p,%p)\n",
           this, hwndOwner, cidl, apidl, shdebugstr_guid (&riid), prgfInOut, ppvOut);

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if (IsEqualIID (riid, IID_IContextMenu))
    {
        hr = CDefFolderMenu_Create2(pidlRoot, hwndOwner, cidl, apidl, (IShellFolder *)this, NULL, 0, NULL, (IContextMenu **)&pObj);
    }
    else if (IsEqualIID (riid, IID_IDataObject) && (cidl >= 1))
    {
        hr = IDataObject_Constructor( hwndOwner, pidlRoot, apidl, cidl, (IDataObject **)&pObj);
    }
    else if (IsEqualIID (riid, IID_IExtractIconA) && (cidl == 1))
    {
        pidl = ILCombine (pidlRoot, apidl[0]);
        pObj = IExtractIconA_Constructor (pidl);
        SHFree (pidl);
        hr = S_OK;
    }
    else if (IsEqualIID (riid, IID_IExtractIconW) && (cidl == 1))
    {
        pidl = ILCombine (pidlRoot, apidl[0]);
        pObj = IExtractIconW_Constructor (pidl);
        SHFree (pidl);
        hr = S_OK;
    }
    else if (IsEqualIID (riid, IID_IDropTarget))
    {
        /* only interested in attempting to bind to shell folders, not files, semicolon intentionate */
        if (cidl != 1 || FAILED(hr = this->_GetDropTarget(apidl[0], (LPVOID*) &pObj)))
        {
            IDropTarget * pDt = NULL;
            hr = ShellObjectCreatorInit<CDesktopFolderDropTarget>(this, IID_IDropTarget, &pDt);
            pObj = pDt;
        }
    }
    else if ((IsEqualIID(riid, IID_IShellLinkW) ||
              IsEqualIID(riid, IID_IShellLinkA)) && (cidl == 1))
    {
        pidl = ILCombine (pidlRoot, apidl[0]);
        hr = IShellLink_ConstructFromFile(NULL, riid, pidl, (LPVOID*)&pObj);
        SHFree (pidl);
    }
    else
        hr = E_NOINTERFACE;

    if (SUCCEEDED(hr) && !pObj)
        hr = E_OUTOFMEMORY;

    *ppvOut = pObj;
    TRACE ("(%p)->hr=0x%08x\n", this, hr);
    return hr;
}

/**************************************************************************
 *    CDesktopFolder::GetDisplayNameOf
 *
 * NOTES
 *    special case: pidl = null gives desktop-name back
 */
HRESULT WINAPI CDesktopFolder::GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet)
{
    HRESULT hr = S_OK;
    LPWSTR pszPath;

    TRACE ("(%p)->(pidl=%p,0x%08x,%p)\n", this, pidl, dwFlags, strRet);
    pdump (pidl);

    if (!strRet)
        return E_INVALIDARG;

    pszPath = (LPWSTR)CoTaskMemAlloc((MAX_PATH + 1) * sizeof(WCHAR));
    if (!pszPath)
        return E_OUTOFMEMORY;

    if (_ILIsDesktop (pidl))
    {
        if ((GET_SHGDN_RELATION (dwFlags) == SHGDN_NORMAL) &&
                (GET_SHGDN_FOR (dwFlags) & SHGDN_FORPARSING))
            wcscpy(pszPath, sPathTarget);
        else
            HCR_GetClassNameW(CLSID_ShellDesktop, pszPath, MAX_PATH);
    }
    else if (_ILIsPidlSimple (pidl))
    {
        GUID const *clsid;

        if ((clsid = _ILGetGUIDPointer (pidl)))
        {
            if (GET_SHGDN_FOR (dwFlags) == SHGDN_FORPARSING)
            {
                int bWantsForParsing;

                /*
                 * We can only get a filesystem path from a shellfolder if the
                 *  value WantsFORPARSING in CLSID\\{...}\\shellfolder exists.
                 *
                 * Exception: The MyComputer folder doesn't have this key,
                 *   but any other filesystem backed folder it needs it.
                 */
                if (IsEqualIID (*clsid, CLSID_MyComputer))
                {
                    bWantsForParsing = TRUE;
                }
                else
                {
                    /* get the "WantsFORPARSING" flag from the registry */
                    static const WCHAR clsidW[] =
                    { 'C', 'L', 'S', 'I', 'D', '\\', 0 };
                    static const WCHAR shellfolderW[] =
                    { '\\', 's', 'h', 'e', 'l', 'l', 'f', 'o', 'l', 'd', 'e', 'r', 0 };
                    static const WCHAR wantsForParsingW[] =
                    {   'W', 'a', 'n', 't', 's', 'F', 'o', 'r', 'P', 'a', 'r', 's', 'i', 'n',
                        'g', 0
                    };
                    WCHAR szRegPath[100];
                    LONG r;

                    wcscpy (szRegPath, clsidW);
                    SHELL32_GUIDToStringW (*clsid, &szRegPath[6]);
                    wcscat (szRegPath, shellfolderW);
                    r = SHGetValueW(HKEY_CLASSES_ROOT, szRegPath,
                                    wantsForParsingW, NULL, NULL, NULL);
                    if (r == ERROR_SUCCESS)
                        bWantsForParsing = TRUE;
                    else
                        bWantsForParsing = FALSE;
                }

                if ((GET_SHGDN_RELATION (dwFlags) == SHGDN_NORMAL) &&
                        bWantsForParsing)
                {
                    /*
                     * we need the filesystem path to the destination folder.
                     * Only the folder itself can know it
                     */
                    hr = SHELL32_GetDisplayNameOfChild (this, pidl, dwFlags,
                                                        pszPath,
                                                        MAX_PATH);
                }
                else
                {
                    /* parsing name like ::{...} */
                    pszPath[0] = ':';
                    pszPath[1] = ':';
                    SHELL32_GUIDToStringW (*clsid, &pszPath[2]);
                }
            }
            else
            {
                /* user friendly name */
                HCR_GetClassNameW (*clsid, pszPath, MAX_PATH);
            }
        }
        else
        {
            int cLen = 0;

            /* file system folder or file rooted at the desktop */
            if ((GET_SHGDN_FOR(dwFlags) == SHGDN_FORPARSING) &&
                    (GET_SHGDN_RELATION(dwFlags) != SHGDN_INFOLDER))
            {
                lstrcpynW(pszPath, sPathTarget, MAX_PATH - 1);
                PathAddBackslashW(pszPath);
                cLen = wcslen(pszPath);
            }

            _ILSimpleGetTextW(pidl, pszPath + cLen, MAX_PATH - cLen);
            if (!_ILIsFolder(pidl))
                SHELL_FS_ProcessDisplayFilename(pszPath, dwFlags);

            if (GetFileAttributes(pszPath) == INVALID_FILE_ATTRIBUTES)
            {
                /* file system folder or file rooted at the AllUsers desktop */
                if ((GET_SHGDN_FOR(dwFlags) == SHGDN_FORPARSING) &&
                        (GET_SHGDN_RELATION(dwFlags) != SHGDN_INFOLDER))
                {
                    SHGetSpecialFolderPathW(0, pszPath, CSIDL_COMMON_DESKTOPDIRECTORY, FALSE);
                    PathAddBackslashW(pszPath);
                    cLen = wcslen(pszPath);
                }

                _ILSimpleGetTextW(pidl, pszPath + cLen, MAX_PATH - cLen);
                if (!_ILIsFolder(pidl))
                    SHELL_FS_ProcessDisplayFilename(pszPath, dwFlags);
            }
        }
    }
    else
    {
        /* a complex pidl, let the subfolder do the work */
        hr = SHELL32_GetDisplayNameOfChild (this, pidl, dwFlags,
                                            pszPath, MAX_PATH);
    }

    if (SUCCEEDED(hr))
    {
        /* Win9x always returns ANSI strings, NT always returns Unicode strings */
        if (GetVersion() & 0x80000000)
        {
            strRet->uType = STRRET_CSTR;
            if (!WideCharToMultiByte(CP_ACP, 0, pszPath, -1, strRet->cStr, MAX_PATH,
                                     NULL, NULL))
                strRet->cStr[0] = '\0';
            CoTaskMemFree(pszPath);
        }
        else
        {
            strRet->uType = STRRET_WSTR;
            strRet->pOleStr = pszPath;
        }
    }
    else
        CoTaskMemFree(pszPath);

    TRACE ("-- (%p)->(%s,0x%08x)\n", this,
           strRet->uType == STRRET_CSTR ? strRet->cStr :
           debugstr_w(strRet->pOleStr), hr);
    return hr;
}

/**************************************************************************
 *  CDesktopFolder::SetNameOf
 *  Changes the name of a file object or subfolder, possibly changing its item
 *  identifier in the process.
 *
 * PARAMETERS
 *  HWND          hwndOwner,  //[in ] Owner window for output
 *  LPCITEMIDLIST pidl,       //[in ] simple pidl of item to change
 *  LPCOLESTR     lpszName,   //[in ] the items new display name
 *  DWORD         dwFlags,    //[in ] SHGNO formatting flags
 *  LPITEMIDLIST* ppidlOut)   //[out] simple pidl returned
 */
HRESULT WINAPI CDesktopFolder::SetNameOf(
    HWND hwndOwner,
    PCUITEMID_CHILD pidl,    /* simple pidl */
    LPCOLESTR lpName,
    DWORD dwFlags,
    PITEMID_CHILD *pPidlOut)
{
    CComPtr<IShellFolder2>                psf;
    HRESULT hr;
    WCHAR szSrc[MAX_PATH + 1], szDest[MAX_PATH + 1];
    LPWSTR ptr;
    BOOL bIsFolder = _ILIsFolder (ILFindLastID (pidl));

    TRACE ("(%p)->(%p,pidl=%p,%s,%u,%p)\n", this, hwndOwner, pidl,
           debugstr_w (lpName), dwFlags, pPidlOut);

    if (_ILGetGUIDPointer(pidl))
    {
        if (SUCCEEDED(BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder2, &psf))))
        {
            hr = psf->SetNameOf(hwndOwner, pidl, lpName, dwFlags, pPidlOut);
            return hr;
        }
    }

    /* build source path */
    lstrcpynW(szSrc, sPathTarget, MAX_PATH);
    ptr = PathAddBackslashW (szSrc);
    if (ptr)
        _ILSimpleGetTextW (pidl, ptr, MAX_PATH + 1 - (ptr - szSrc));

    /* build destination path */
    if (dwFlags == SHGDN_NORMAL || dwFlags & SHGDN_INFOLDER) {
        lstrcpynW(szDest, sPathTarget, MAX_PATH);
        ptr = PathAddBackslashW (szDest);
        if (ptr)
            lstrcpynW(ptr, lpName, MAX_PATH + 1 - (ptr - szDest));
    } else
        lstrcpynW(szDest, lpName, MAX_PATH);

    if(!(dwFlags & SHGDN_FORPARSING) && SHELL_FS_HideExtension(szSrc)) {
        WCHAR *ext = PathFindExtensionW(szSrc);
        if(*ext != '\0') {
            INT len = wcslen(szDest);
            lstrcpynW(szDest + len, ext, MAX_PATH - len);
        }
    }

    if (!memcmp(szSrc, szDest, (wcslen(szDest) + 1) * sizeof(WCHAR)))
    {
        /* src and destination is the same */
        hr = S_OK;
        if (pPidlOut)
            hr = _ILCreateFromPathW(szDest, pPidlOut);

        return hr;
    }

    TRACE ("src=%s dest=%s\n", debugstr_w(szSrc), debugstr_w(szDest));
    if (MoveFileW (szSrc, szDest))
    {
        hr = S_OK;

        if (pPidlOut)
            hr = _ILCreateFromPathW(szDest, pPidlOut);

        SHChangeNotify (bIsFolder ? SHCNE_RENAMEFOLDER : SHCNE_RENAMEITEM,
                        SHCNF_PATHW, szSrc, szDest);

        return hr;
    }
    return E_FAIL;
}

HRESULT WINAPI CDesktopFolder::GetDefaultSearchGUID(GUID *pguid)
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CDesktopFolder::EnumSearches(IEnumExtraSearch **ppenum)
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CDesktopFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    TRACE ("(%p)\n", this);

    if (pSort)
        *pSort = 0;
    if (pDisplay)
        *pDisplay = 0;

    return S_OK;
}

HRESULT WINAPI CDesktopFolder::GetDefaultColumnState(UINT iColumn, DWORD *pcsFlags)
{
    TRACE ("(%p)\n", this);

    if (!pcsFlags || iColumn >= DESKTOPSHELLVIEWCOLUMNS)
        return E_INVALIDARG;

    *pcsFlags = DesktopSFHeader[iColumn].pcsFlags;

    return S_OK;
}

HRESULT WINAPI CDesktopFolder::GetDetailsEx(
    PCUITEMID_CHILD pidl,
    const SHCOLUMNID *pscid,
    VARIANT *pv)
{
    FIXME ("(%p)\n", this);

    return E_NOTIMPL;
}

HRESULT WINAPI CDesktopFolder::GetDetailsOf(
    PCUITEMID_CHILD pidl,
    UINT iColumn,
    SHELLDETAILS *psd)
{
    HRESULT hr = S_OK;

    TRACE ("(%p)->(%p %i %p)\n", this, pidl, iColumn, psd);

    if (!psd || iColumn >= DESKTOPSHELLVIEWCOLUMNS)
        return E_INVALIDARG;

    if (!pidl)
    {
        psd->fmt = DesktopSFHeader[iColumn].fmt;
        psd->cxChar = DesktopSFHeader[iColumn].cxChar;
        psd->str.uType = STRRET_CSTR;
        LoadStringA (shell32_hInstance, DesktopSFHeader[iColumn].colnameid,
                     psd->str.cStr, MAX_PATH);
        return S_OK;
    }

    /* the data from the pidl */
    psd->str.uType = STRRET_CSTR;
    switch (iColumn)
    {
        case 0:        /* name */
            hr = GetDisplayNameOf(pidl,
                                  SHGDN_NORMAL | SHGDN_INFOLDER, &psd->str);
            break;
        case 1:        /* size */
            _ILGetFileSize (pidl, psd->str.cStr, MAX_PATH);
            break;
        case 2:        /* type */
            _ILGetFileType (pidl, psd->str.cStr, MAX_PATH);
            break;
        case 3:        /* date */
            _ILGetFileDate (pidl, psd->str.cStr, MAX_PATH);
            break;
        case 4:        /* attributes */
            _ILGetFileAttributes (pidl, psd->str.cStr, MAX_PATH);
            break;
    }

    return hr;
}

HRESULT WINAPI CDesktopFolder::MapColumnToSCID(UINT column, SHCOLUMNID *pscid)
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CDesktopFolder::GetClassID(CLSID *lpClassId)
{
    TRACE ("(%p)\n", this);

    if (!lpClassId)
        return E_POINTER;

    *lpClassId = CLSID_ShellDesktop;

    return S_OK;
}

HRESULT WINAPI CDesktopFolder::Initialize(LPCITEMIDLIST pidl)
{
    TRACE ("(%p)->(%p)\n", this, pidl);

    return E_NOTIMPL;
}

HRESULT WINAPI CDesktopFolder::GetCurFolder(LPITEMIDLIST * pidl)
{
    TRACE ("(%p)->(%p)\n", this, pidl);

    if (!pidl) return E_POINTER;
    *pidl = ILClone (pidlRoot);
    return S_OK;
}

HRESULT WINAPI CDesktopFolder::GetUniqueName(LPWSTR pwszName, UINT uLen)
{
    CComPtr<IEnumIDList>                penum;
    HRESULT hr;
    WCHAR wszText[MAX_PATH];
    WCHAR wszNewFolder[25];
    const WCHAR wszFormat[] = {'%', 's', ' ', '%', 'd', 0 };

    LoadStringW(shell32_hInstance, IDS_NEWFOLDER, wszNewFolder,  sizeof(wszNewFolder) / sizeof(WCHAR));

    TRACE ("(%p)(%p %u)\n", this, pwszName, uLen);

    if (uLen < sizeof(wszNewFolder) / sizeof(WCHAR) + 3)
        return E_POINTER;

    lstrcpynW (pwszName, wszNewFolder, uLen);

    hr = EnumObjects(0,
                     SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN, &penum);
    if (penum) {
        LPITEMIDLIST pidl;
        DWORD dwFetched;
        int i = 1;

next:
        penum->Reset ();
        while (S_OK == penum->Next(1, &pidl, &dwFetched) &&
                dwFetched) {
            _ILSimpleGetTextW (pidl, wszText, MAX_PATH);
            if (0 == lstrcmpiW (wszText, pwszName)) {
                _snwprintf (pwszName, uLen, wszFormat, wszNewFolder, i++);
                if (i > 99) {
                    hr = E_FAIL;
                    break;
                }
                goto next;
            }
        }

    }
    return hr;
}

HRESULT WINAPI CDesktopFolder::AddFolder(HWND hwnd, LPCWSTR pwszName, LPITEMIDLIST *ppidlOut)
{
    WCHAR wszNewDir[MAX_PATH];
    DWORD bRes;
    HRESULT hres = E_FAIL;

    TRACE ("(%p)(%s %p)\n", this, debugstr_w(pwszName), ppidlOut);

    wszNewDir[0] = 0;
    if (sPathTarget)
        lstrcpynW(wszNewDir, sPathTarget, MAX_PATH);
    PathAppendW(wszNewDir, pwszName);
    bRes = CreateDirectoryW (wszNewDir, NULL);
    if (bRes)
    {
        SHChangeNotify (SHCNE_MKDIR, SHCNF_PATHW, wszNewDir, NULL);
        hres = S_OK;
        if (ppidlOut)
            hres = _ILCreateFromPathW(wszNewDir, ppidlOut);
    }

    return hres;
}

HRESULT WINAPI CDesktopFolder::DeleteItems(UINT cidl, LPCITEMIDLIST *apidl)
{
    UINT i;
    SHFILEOPSTRUCTW op;
    WCHAR wszPath[MAX_PATH];
    WCHAR wszCaption[50];
    WCHAR *wszPathsList;
    HRESULT ret;
    WCHAR *wszCurrentPath;
    UINT bRestoreWithDeskCpl = FALSE;
    int res;

    TRACE ("(%p)(%u %p)\n", this, cidl, apidl);
    if (cidl == 0) return S_OK;

    for(i = 0; i < cidl; i++)
    {
        if (_ILIsMyComputer(apidl[i]))
            bRestoreWithDeskCpl++;
        else if (_ILIsNetHood(apidl[i]))
            bRestoreWithDeskCpl++;
        else if (_ILIsMyDocuments(apidl[i]))
            bRestoreWithDeskCpl++;
    }

    if (bRestoreWithDeskCpl)
    {
        /* FIXME use FormatMessage
         * use a similar message resource as in windows
         */
        LoadStringW(shell32_hInstance, IDS_DELETEMULTIPLE_TEXT, wszPath, sizeof(wszPath) / sizeof(WCHAR));
        wszPath[(sizeof(wszPath)/sizeof(WCHAR))-1] = 0;

        LoadStringW(shell32_hInstance, IDS_DELETEITEM_CAPTION, wszCaption, sizeof(wszCaption) / sizeof(WCHAR));
        wszCaption[(sizeof(wszCaption)/sizeof(WCHAR))-1] = 0;

        res = SHELL_ConfirmMsgBox(GetActiveWindow(), wszPath, wszCaption, NULL, cidl > 1);
        if (res == IDC_YESTOALL || res == IDYES)
        {
            for(i = 0; i < cidl; i++)
            {
                if (_ILIsMyComputer(apidl[i]))
                    SetNamespaceExtensionVisibleStatus(L"{20D04FE0-3AEA-1069-A2D8-08002B30309D}", 0x1);
                else if (_ILIsNetHood(apidl[i]))
                    SetNamespaceExtensionVisibleStatus(L"{208D2C60-3AEA-1069-A2D7-08002B30309D}", 0x1);
                else if (_ILIsMyDocuments(apidl[i]))
                    SetNamespaceExtensionVisibleStatus(L"{450D8FBA-AD25-11D0-98A8-0800361B1103}", 0x1);
            }
        }
    }
    if (sPathTarget)
        lstrcpynW(wszPath, sPathTarget, MAX_PATH);
    else
        wszPath[0] = '\0';

    PathAddBackslashW(wszPath);
    wszPathsList = BuildPathsList(wszPath, cidl, apidl);

    ZeroMemory(&op, sizeof(op));
    op.hwnd = GetActiveWindow();
    op.wFunc = FO_DELETE;
    op.pFrom = wszPathsList;
    op.fFlags = FOF_ALLOWUNDO;
    if (SHFileOperationW(&op))
    {
        WARN("SHFileOperation failed\n");
        ret = E_FAIL;
    }
    else
        ret = S_OK;

    /* we currently need to manually send the notifies */
    wszCurrentPath = wszPathsList;
    for (i = 0; i < cidl; i++)
    {
        LONG wEventId;

        if (_ILIsFolder(apidl[i]))
            wEventId = SHCNE_RMDIR;
        else if (_ILIsValue(apidl[i]))
            wEventId = SHCNE_DELETE;
        else
            continue;

        /* check if file exists */
        if (GetFileAttributesW(wszCurrentPath) == INVALID_FILE_ATTRIBUTES)
        {
            LPITEMIDLIST pidl = ILCombine(pidlRoot, apidl[i]);
            SHChangeNotify(wEventId, SHCNF_IDLIST, pidl, NULL);
            SHFree(pidl);
        }

        wszCurrentPath += wcslen(wszCurrentPath) + 1;
    }
    HeapFree(GetProcessHeap(), 0, wszPathsList);
    return ret;
}

HRESULT WINAPI CDesktopFolder::CopyItems(IShellFolder *pSFFrom, UINT cidl, LPCITEMIDLIST *apidl, BOOL bCopy)
{
    CComPtr<IPersistFolder2> ppf2;
    WCHAR szSrcPath[MAX_PATH];
    WCHAR szTargetPath[MAX_PATH];
    SHFILEOPSTRUCTW op;
    LPITEMIDLIST pidl;
    LPWSTR pszSrc, pszTarget, pszSrcList, pszTargetList, pszFileName;
    int res, length;
    STRRET strRet;

    TRACE ("(%p)->(%p,%u,%p)\n", this, pSFFrom, cidl, apidl);

    pSFFrom->QueryInterface(IID_PPV_ARG(IPersistFolder2, &ppf2));
    if (ppf2)
    {
        if (FAILED(ppf2->GetCurFolder(&pidl)))
            return E_FAIL;

        if (FAILED(pSFFrom->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &strRet)))
        {
            SHFree (pidl);
            return E_FAIL;
        }

        if (FAILED(StrRetToBufW(&strRet, pidl, szSrcPath, MAX_PATH)))
        {
            SHFree (pidl);
            return E_FAIL;
        }
        SHFree (pidl);

        pszSrc = PathAddBackslashW (szSrcPath);

        wcscpy(szTargetPath, sPathTarget);
        pszTarget = PathAddBackslashW (szTargetPath);

        pszSrcList = BuildPathsList(szSrcPath, cidl, apidl);
        pszTargetList = BuildPathsList(szTargetPath, cidl, apidl);

        if (!pszSrcList || !pszTargetList)
        {
            if (pszSrcList)
                HeapFree(GetProcessHeap(), 0, pszSrcList);

            if (pszTargetList)
                HeapFree(GetProcessHeap(), 0, pszTargetList);

            SHFree (pidl);
            return E_OUTOFMEMORY;
        }
        ZeroMemory(&op, sizeof(op));
        if (!pszSrcList[0])
        {
            /* remove trailing backslash */
            pszSrc--;
            pszSrc[0] = L'\0';
            op.pFrom = szSrcPath;
        }
        else
        {
            op.pFrom = pszSrcList;
        }

        if (!pszTargetList[0])
        {
            /* remove trailing backslash */
            if (pszTarget - szTargetPath > 3)
            {
                pszTarget--;
                pszTarget[0] = L'\0';
            }
            else
            {
                pszTarget[1] = L'\0';
            }

            op.pTo = szTargetPath;
            op.fFlags = 0;
        }
        else
        {
            op.pTo = pszTargetList;
            op.fFlags = FOF_MULTIDESTFILES;
        }
        op.hwnd = GetActiveWindow();
        op.wFunc = bCopy ? FO_COPY : FO_MOVE;
        op.fFlags |= FOF_ALLOWUNDO | FOF_NOCONFIRMMKDIR;

        res = SHFileOperationW(&op);

        if (res == DE_SAMEFILE)
        {
            length = wcslen(szTargetPath);


            pszFileName = wcsrchr(pszSrcList, '\\');
            pszFileName++;

            if (LoadStringW(shell32_hInstance, IDS_COPY_OF, pszTarget, MAX_PATH - length))
            {
                wcscat(szTargetPath, L" ");
            }

            wcscat(szTargetPath, pszFileName);
            op.pTo = szTargetPath;

            res = SHFileOperationW(&op);
        }


        HeapFree(GetProcessHeap(), 0, pszSrcList);
        HeapFree(GetProcessHeap(), 0, pszTargetList);

        if (res)
            return E_FAIL;
        else
            return S_OK;
    }
    return E_FAIL;
}

/****************************************************************************
 * IDropTarget implementation
 *
 * This should allow two somewhat separate things, copying files to the users directory,
 * as well as allowing icons to be moved anywhere and updating the registry to save.
 *
 * The first thing I think is best done using fs.cpp to prevent WET code. So we'll simulate
 * a drop to the user's home directory. The second will look at the pointer location and
 * set sensible places for the icons to live.
 *
 */
void CDesktopFolderDropTarget::SF_RegisterClipFmt()
{
    TRACE ("(%p)\n", this);

    if (!m_cfShellIDList)
        m_cfShellIDList = RegisterClipboardFormatW(CFSTR_SHELLIDLIST);
}

CDesktopFolderDropTarget::CDesktopFolderDropTarget() :
    m_psf(NULL),
    m_fAcceptFmt(FALSE),
    m_cfShellIDList(0)
{
}

HRESULT WINAPI CDesktopFolderDropTarget::Initialize(IShellFolder *psf)
{
    m_psf = psf;
    SF_RegisterClipFmt();
    return S_OK;
}

BOOL CDesktopFolderDropTarget::QueryDrop(DWORD dwKeyState, LPDWORD pdwEffect)
{
    /* TODO Windows does different drop effects if dragging across drives.
    i.e., it will copy instead of move if the directories are on different disks. */

    DWORD dwEffect = DROPEFFECT_MOVE;

    *pdwEffect = DROPEFFECT_NONE;

    if (m_fAcceptFmt) { /* Does our interpretation of the keystate ... */
        *pdwEffect = KeyStateToDropEffect (dwKeyState);

        if (*pdwEffect == DROPEFFECT_NONE)
            *pdwEffect = dwEffect;

        /* ... matches the desired effect ? */
        if (dwEffect & *pdwEffect) {
            return TRUE;
        }
    }
    return FALSE;
}

HRESULT WINAPI CDesktopFolderDropTarget::DragEnter(IDataObject *pDataObject,
                                    DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    TRACE("(%p)->(DataObject=%p)\n", this, pDataObject);
    FORMATETC fmt;
    FORMATETC fmt2;
    m_fAcceptFmt = FALSE;

    InitFormatEtc (fmt, m_cfShellIDList, TYMED_HGLOBAL);
    InitFormatEtc (fmt2, CF_HDROP, TYMED_HGLOBAL);

    if (SUCCEEDED(pDataObject->QueryGetData(&fmt)))
        m_fAcceptFmt = TRUE;
    else if (SUCCEEDED(pDataObject->QueryGetData(&fmt2)))
        m_fAcceptFmt = TRUE;

    QueryDrop(dwKeyState, pdwEffect);
    return S_OK;
}

HRESULT WINAPI CDesktopFolderDropTarget::DragOver(DWORD dwKeyState, POINTL pt,
                                   DWORD *pdwEffect)
{
    TRACE("(%p)\n", this);

    if (!pdwEffect)
        return E_INVALIDARG;

    QueryDrop(dwKeyState, pdwEffect);

    return S_OK;
}

HRESULT WINAPI CDesktopFolderDropTarget::DragLeave()
{
    TRACE("(%p)\n", this);
    m_fAcceptFmt = FALSE;
    return S_OK;
}

HRESULT WINAPI CDesktopFolderDropTarget::Drop(IDataObject *pDataObject,
                               DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    TRACE("(%p) object dropped desktop\n", this);

    STGMEDIUM medium;
    bool passthroughtofs = FALSE;
    FORMATETC formatetc;
    InitFormatEtc(formatetc, RegisterClipboardFormatW(CFSTR_SHELLIDLIST), TYMED_HGLOBAL);
    
    HRESULT hr = pDataObject->GetData(&formatetc, &medium);
    if (SUCCEEDED(hr))
    {
        /* lock the handle */
        LPIDA lpcida = (LPIDA)GlobalLock(medium.hGlobal);
        if (!lpcida)
        {
            ReleaseStgMedium(&medium);
            return E_FAIL;
        }

        /* convert the clipboard data into pidl (pointer to id list) */
        LPITEMIDLIST pidl;
        LPITEMIDLIST *apidl = _ILCopyCidaToaPidl(&pidl, lpcida);
        if (!apidl)
        {
            ReleaseStgMedium(&medium);
            return E_FAIL;
        }
        passthroughtofs = !_ILIsDesktop(pidl) || (dwKeyState & MK_CONTROL);
        SHFree(pidl);
        _ILFreeaPidl(apidl, lpcida->cidl);
        ReleaseStgMedium(&medium);
    }
    else
    {
        InitFormatEtc (formatetc, CF_HDROP, TYMED_HGLOBAL);
        if (SUCCEEDED(pDataObject->QueryGetData(&formatetc)))
        {
            passthroughtofs = TRUE;
        }
    }
    /* We only want to really move files around if they don't already
       come from the desktop, or we're linking or copying */
    if (passthroughtofs)
    {
        LPITEMIDLIST pidl = NULL;

        WCHAR szPath[MAX_PATH];
        STRRET strRet;
        //LPWSTR pathPtr;

        /* build a complete path to create a simple pidl */
        hr = m_psf->GetDisplayNameOf(NULL, SHGDN_NORMAL | SHGDN_FORPARSING, &strRet);
        if (SUCCEEDED(hr))
        {
            hr = StrRetToBufW(&strRet, NULL, szPath, MAX_PATH);
            ASSERT(SUCCEEDED(hr));
            /*pathPtr = */PathAddBackslashW(szPath);
            //hr = _ILCreateFromPathW(szPath, &pidl);
            hr = m_psf->ParseDisplayName(NULL, NULL, szPath, NULL, &pidl, NULL);
        }

        if (SUCCEEDED(hr))
        {
            CComPtr<IDropTarget> pDT;
            hr = m_psf->BindToObject(pidl, NULL, IID_PPV_ARG(IDropTarget, &pDT));
            CoTaskMemFree(pidl);
            if (SUCCEEDED(hr))
                SHSimulateDrop(pDT, pDataObject, dwKeyState, NULL, pdwEffect);
            else
                ERR("Error Binding");
        }
        else
            ERR("Error creating from %s\n", debugstr_w(szPath));
    }

    /* Todo, rewrite the registry such that the icons are well placed.
    Blocked by no bags implementation. */
    return hr;
}

HRESULT WINAPI CDesktopFolder::_GetDropTarget(LPCITEMIDLIST pidl, LPVOID *ppvOut) {
    HRESULT hr;

    TRACE("CFSFolder::_GetDropTarget entered\n");

    if (_ILGetGUIDPointer (pidl) || _ILIsFolder (pidl))
        return this->BindToObject(pidl, NULL, IID_IDropTarget, ppvOut);

    LPITEMIDLIST pidlNext = NULL;

    STRRET strFile;
    hr = this->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &strFile);
    if (SUCCEEDED(hr))
    {
        WCHAR wszPath[MAX_PATH];
        hr = StrRetToBufW(&strFile, pidl, wszPath, _countof(wszPath));

        if (SUCCEEDED(hr))
        {
            PathRemoveFileSpecW (wszPath);
            hr = this->ParseDisplayName(NULL, NULL, wszPath, NULL, &pidlNext, NULL);

            if (SUCCEEDED(hr))
            {
                CComPtr<IShellFolder> psf;
                hr = this->BindToObject(pidlNext, NULL, IID_PPV_ARG(IShellFolder, &psf));
                CoTaskMemFree(pidlNext);
                if (SUCCEEDED(hr))
                {
                    hr = psf->GetUIObjectOf(NULL, 1, &pidl, IID_IDropTarget, NULL, ppvOut);
                    if (FAILED(hr))
                        ERR("FS GetUIObjectOf failed: %x\n", hr);
                }
                else 
                    ERR("BindToObject failed: %x\n", hr);
            }
            else
                ERR("ParseDisplayName failed: %x\n", hr);
        }
        else
            ERR("StrRetToBufW failed: %x\n", hr);
    }    
    else
        ERR("GetDisplayNameOf failed: %x\n", hr);

    return hr;
}

/*************************************************************************
 * SHGetDesktopFolder            [SHELL32.@]
 */
HRESULT WINAPI SHGetDesktopFolder(IShellFolder **psf)
{
    HRESULT    hres = S_OK;
    TRACE("\n");

    if(!psf) return E_INVALIDARG;
    *psf = NULL;
    hres = CDesktopFolder::_CreatorClass::CreateInstance(NULL, IID_PPV_ARG(IShellFolder, psf));

    TRACE("-- %p->(%p)\n",psf, *psf);
    return hres;
}
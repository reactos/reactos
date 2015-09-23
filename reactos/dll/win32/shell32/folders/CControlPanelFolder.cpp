/*
 * Control panel folder
 *
 * Copyright 2003 Martin Fuchs
 * Copyright 2009 Andrew Hill
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

/***********************************************************************
*   control panel implementation in shell namespace
*/

class CControlPanelEnum :
    public CEnumIDListBase
{
    public:
        CControlPanelEnum();
        ~CControlPanelEnum();
        HRESULT WINAPI Initialize(DWORD dwFlags);
        BOOL RegisterCPanelApp(LPCWSTR path);
        int RegisterRegistryCPanelApps(HKEY hkey_root, LPCWSTR szRepPath);
        int RegisterCPanelFolders(HKEY hkey_root, LPCWSTR szRepPath);
        BOOL CreateCPanelEnumList(DWORD dwFlags);

        BEGIN_COM_MAP(CControlPanelEnum)
        COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
        END_COM_MAP()
};

/***********************************************************************
*   IShellFolder [ControlPanel] implementation
*/

static const shvheader ControlPanelSFHeader[] = {
    {IDS_SHV_COLUMN8, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 15},/*FIXME*/
    {IDS_SHV_COLUMN9, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 80},/*FIXME*/
};

#define CONROLPANELSHELLVIEWCOLUMNS 2

CControlPanelEnum::CControlPanelEnum()
{
}

CControlPanelEnum::~CControlPanelEnum()
{
}

HRESULT WINAPI CControlPanelEnum::Initialize(DWORD dwFlags)
{
    if (CreateCPanelEnumList(dwFlags) == FALSE)
        return E_FAIL;
    return S_OK;
}

static LPITEMIDLIST _ILCreateCPanelApplet(LPCWSTR pszName, LPCWSTR pszDisplayName, LPCWSTR pszComment, int iIconIdx)
{
    PIDLCPanelStruct *pCP;
    LPITEMIDLIST pidl;
    LPPIDLDATA pData;
    int cchName, cchDisplayName, cchComment, cbData;

    /* Calculate lengths of given strings */
    cchName = wcslen(pszName);
    cchDisplayName = wcslen(pszDisplayName);
    cchComment = wcslen(pszComment);

    /* Allocate PIDL */
    cbData = sizeof(pidl->mkid.cb) + sizeof(pData->type) + sizeof(pData->u.cpanel) - sizeof(pData->u.cpanel.szName)
             + (cchName + cchDisplayName + cchComment + 3) * sizeof(WCHAR);
    pidl = (LPITEMIDLIST)SHAlloc(cbData + sizeof(WORD));
    if (!pidl)
        return NULL;

    /* Copy data to allocated memory */
    pidl->mkid.cb = cbData;
    pData = (PIDLDATA *)pidl->mkid.abID;
    pData->type = PT_CPLAPPLET;

    pCP = &pData->u.cpanel;
    pCP->dummy = 0;
    pCP->iconIdx = iIconIdx;
    wcscpy(pCP->szName, pszName);
    pCP->offsDispName = cchName + 1;
    wcscpy(pCP->szName + pCP->offsDispName, pszDisplayName);
    pCP->offsComment = pCP->offsDispName + cchDisplayName + 1;
    wcscpy(pCP->szName + pCP->offsComment, pszComment);

    /* Add PIDL NULL terminator */
    *(WORD*)(pCP->szName + pCP->offsComment + cchComment + 1) = 0;

    pcheck(pidl);

    return pidl;
}

/**************************************************************************
 *  _ILGetCPanelPointer()
 * gets a pointer to the control panel struct stored in the pidl
 */
static PIDLCPanelStruct *_ILGetCPanelPointer(LPCITEMIDLIST pidl)
{
    LPPIDLDATA pdata = _ILGetDataPointer(pidl);

    if (pdata && pdata->type == PT_CPLAPPLET)
        return (PIDLCPanelStruct *) & (pdata->u.cpanel);

    return NULL;
}

BOOL CControlPanelEnum::RegisterCPanelApp(LPCWSTR wpath)
{
    CPlApplet* applet = Control_LoadApplet(0, wpath, NULL);
    int iconIdx;

    if (applet)
    {
        for (UINT i = 0; i < applet->count; ++i)
        {
            if (applet->info[i].idIcon > 0)
                iconIdx = -applet->info[i].idIcon; /* negative icon index instead of icon number */
            else
                iconIdx = 0;

            LPITEMIDLIST pidl = _ILCreateCPanelApplet(wpath, 
                                                      applet->info[i].name, 
                                                      applet->info[i].info, 
                                                      iconIdx);

            if (pidl)
                AddToEnumList(pidl);
        }
        Control_UnloadApplet(applet);
    }
    return TRUE;
}

int CControlPanelEnum::RegisterRegistryCPanelApps(HKEY hkey_root, LPCWSTR szRepPath)
{
    WCHAR name[MAX_PATH];
    WCHAR value[MAX_PATH];
    HKEY hkey;

    int cnt = 0;

    if (RegOpenKeyW(hkey_root, szRepPath, &hkey) == ERROR_SUCCESS)
    {
        int idx = 0;

        for(; ; idx++)
        {
            DWORD nameLen = MAX_PATH;
            DWORD valueLen = MAX_PATH;
            WCHAR buffer[MAX_PATH];

            if (RegEnumValueW(hkey, idx, name, &nameLen, NULL, NULL, (LPBYTE)&value, &valueLen) != ERROR_SUCCESS)
                break;

            if (ExpandEnvironmentStringsW(value, buffer, MAX_PATH))
            {
                wcscpy(value, buffer);
            }

            if (RegisterCPanelApp(value))
                ++cnt;
        }
        RegCloseKey(hkey);
    }

    return cnt;
}

int CControlPanelEnum::RegisterCPanelFolders(HKEY hkey_root, LPCWSTR szRepPath)
{
    WCHAR name[MAX_PATH];
    HKEY hkey;

    int cnt = 0;

    if (RegOpenKeyW(hkey_root, szRepPath, &hkey) == ERROR_SUCCESS)
    {
        for (int idx = 0; ; idx++)
        {
            if (RegEnumKeyW(hkey, idx, name, MAX_PATH) != ERROR_SUCCESS)
                break;

            if (*name == '{')
            {
                LPITEMIDLIST pidl = _ILCreateGuidFromStrW(name);

                if (pidl && AddToEnumList(pidl))
                    ++cnt;
            }
        }

        RegCloseKey(hkey);
    }

    return cnt;
}

/**************************************************************************
 *  CControlPanelEnum::CreateCPanelEnumList()
 */
BOOL CControlPanelEnum::CreateCPanelEnumList(DWORD dwFlags)
{
    WCHAR szPath[MAX_PATH];
    WIN32_FIND_DATAW wfd;
    HANDLE hFile;

    TRACE("(%p)->(flags=0x%08x)\n", this, dwFlags);

    /* enumerate control panel folders */
    if (dwFlags & SHCONTF_FOLDERS)
        RegisterCPanelFolders(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ControlPanel\\NameSpace");

    /* enumerate the control panel applets */
    if (dwFlags & SHCONTF_NONFOLDERS)
    {
        LPWSTR p;

        GetSystemDirectoryW(szPath, MAX_PATH);
        p = PathAddBackslashW(szPath);
        wcscpy(p, L"*.cpl");

        hFile = FindFirstFileW(szPath, &wfd);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (!(dwFlags & SHCONTF_INCLUDEHIDDEN) && (wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
                    continue;

                if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    wcscpy(p, wfd.cFileName);
                    if (wcscmp(wfd.cFileName, L"ncpa.cpl"))
                        RegisterCPanelApp(szPath);
                }
            } while(FindNextFileW(hFile, &wfd));
            FindClose(hFile);
        }

        RegisterRegistryCPanelApps(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\Cpls");
        RegisterRegistryCPanelApps(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\Cpls");
    }
    return TRUE;
}

CControlPanelFolder::CControlPanelFolder()
{
    pidlRoot = NULL;    /* absolute pidl */
}

CControlPanelFolder::~CControlPanelFolder()
{
    TRACE("-- destroying IShellFolder(%p)\n", this);
    SHFree(pidlRoot);
}

HRESULT WINAPI CControlPanelFolder::FinalConstruct()
{
    pidlRoot = _ILCreateControlPanel();    /* my qualified pidl */
    if (pidlRoot == NULL)
        return E_OUTOFMEMORY;
    return S_OK;
}

/**************************************************************************
*    CControlPanelFolder::ParseDisplayName
*/
HRESULT WINAPI CControlPanelFolder::ParseDisplayName(
    HWND hwndOwner,
    LPBC pbc,
    LPOLESTR lpszDisplayName,
    DWORD *pchEaten,
    PIDLIST_RELATIVE *ppidl,
    DWORD *pdwAttributes)
{
    /* We only support parsing guid names */
    return SH_ParseGuidDisplayName(this, hwndOwner, pbc, lpszDisplayName, pchEaten, ppidl, pdwAttributes);
}

/**************************************************************************
*        CControlPanelFolder::EnumObjects
*/
HRESULT WINAPI CControlPanelFolder::EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
{
    return ShellObjectCreatorInit<CControlPanelEnum>(dwFlags, IID_IEnumIDList, ppEnumIDList);
}

/**************************************************************************
*        CControlPanelFolder::BindToObject
*/
HRESULT WINAPI CControlPanelFolder::BindToObject(
    PCUIDLIST_RELATIVE pidl,
    LPBC pbcReserved,
    REFIID riid,
    LPVOID *ppvOut)
{
    return SHELL32_BindToGuidItem(pidlRoot, pidl, pbcReserved, riid, ppvOut);
}

/**************************************************************************
*    CControlPanelFolder::BindToStorage
*/
HRESULT WINAPI CControlPanelFolder::BindToStorage(
    PCUIDLIST_RELATIVE pidl,
    LPBC pbcReserved,
    REFIID riid,
    LPVOID *ppvOut)
{
    FIXME("(%p)->(pidl=%p,%p,%s,%p) stub\n", this, pidl, pbcReserved, shdebugstr_guid(&riid), ppvOut);

    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
*     CControlPanelFolder::CompareIDs
*/

HRESULT WINAPI CControlPanelFolder::CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
    int nReturn;

    TRACE("(%p)->(0x%08lx,pidl1=%p,pidl2=%p)\n", this, lParam, pidl1, pidl2);
    nReturn = SHELL32_CompareIDs(this, lParam, pidl1, pidl2);
    TRACE("-- %i\n", nReturn);
    return nReturn;
}

/**************************************************************************
*    CControlPanelFolder::CreateViewObject
*/
HRESULT WINAPI CControlPanelFolder::CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID * ppvOut)
{
    CComPtr<IShellView>                    pShellView;
    HRESULT hr = E_INVALIDARG;

    TRACE("(%p)->(hwnd=%p,%s,%p)\n", this, hwndOwner, shdebugstr_guid(&riid), ppvOut);

    if (ppvOut) {
        *ppvOut = NULL;

        if (IsEqualIID(riid, IID_IDropTarget)) {
            WARN("IDropTarget not implemented\n");
            hr = E_NOTIMPL;
        } else if (IsEqualIID(riid, IID_IContextMenu)) {
            WARN("IContextMenu not implemented\n");
            hr = E_NOTIMPL;
        } else if (IsEqualIID(riid, IID_IShellView)) {
            hr = CDefView_Constructor(this, riid, ppvOut);
        }
    }
    TRACE("--(%p)->(interface=%p)\n", this, ppvOut);
    return hr;
}

/**************************************************************************
*  CControlPanelFolder::GetAttributesOf
*/
HRESULT WINAPI CControlPanelFolder::GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD * rgfInOut)
{
    HRESULT hr = S_OK;
    static const DWORD dwControlPanelAttributes =
        SFGAO_HASSUBFOLDER | SFGAO_FOLDER | SFGAO_CANLINK;

    TRACE("(%p)->(cidl=%d apidl=%p mask=%p (0x%08x))\n",
          this, cidl, apidl, rgfInOut, rgfInOut ? *rgfInOut : 0);

    if (!rgfInOut)
        return E_INVALIDARG;
    if (cidl && !apidl)
        return E_INVALIDARG;

    if (*rgfInOut == 0)
        *rgfInOut = ~0;

    if (!cidl)
    {
        *rgfInOut &= dwControlPanelAttributes;
    }
    else
    {
        while(cidl > 0 && *apidl)
        {
            pdump(*apidl);
            if (_ILIsCPanelStruct(*apidl))
                *rgfInOut &= SFGAO_CANLINK;
            else if (_ILIsSpecialFolder(*apidl))
                SHELL32_GetGuidItemAttributes(this, *apidl, rgfInOut);
            else
                ERR("Got an unkown pidl here!\n");
            apidl++;
            cidl--;
        }
    }
    /* make sure SFGAO_VALIDATE is cleared, some apps depend on that */
    *rgfInOut &= ~SFGAO_VALIDATE;

    TRACE("-- result=0x%08x\n", *rgfInOut);
    return hr;
}

/**************************************************************************
*    CControlPanelFolder::GetUIObjectOf
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
HRESULT WINAPI CControlPanelFolder::GetUIObjectOf(HWND hwndOwner,
        UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid, UINT * prgfInOut, LPVOID * ppvOut)
{
    LPITEMIDLIST pidl;
    IUnknown *pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE("(%p)->(%p,%u,apidl=%p,%s,%p,%p)\n",
          this, hwndOwner, cidl, apidl, shdebugstr_guid(&riid), prgfInOut, ppvOut);

    if (ppvOut) {
        *ppvOut = NULL;

        if (IsEqualIID(riid, IID_IContextMenu) && (cidl >= 1)) {
            
            /* HACK: We should use callbacks from CDefaultContextMenu instead of creating one on our own */
            BOOL bHasCpl = FALSE;
            for (UINT i = 0; i < cidl; i++)
            {
                if(_ILIsCPanelStruct(apidl[i]))
                {
                    bHasCpl = TRUE;
                }
            }

            if (bHasCpl)
                hr = ShellObjectCreatorInit<CCPLItemMenu>(cidl, apidl, riid, &pObj);
            else
                hr = CDefFolderMenu_Create2(pidlRoot, hwndOwner, cidl, apidl, (IShellFolder*)this, NULL, 0, NULL, (IContextMenu**)&pObj);
        } else if (IsEqualIID(riid, IID_IDataObject) && (cidl >= 1)) {
            hr = IDataObject_Constructor(hwndOwner, pidlRoot, apidl, cidl, (IDataObject **)&pObj);
        } else if (IsEqualIID(riid, IID_IExtractIconA) && (cidl == 1)) {
            pidl = ILCombine(pidlRoot, apidl[0]);
            pObj = IExtractIconA_Constructor(pidl);
            SHFree(pidl);
            hr = S_OK;
        } else if (IsEqualIID(riid, IID_IExtractIconW) && (cidl == 1)) {
            pidl = ILCombine(pidlRoot, apidl[0]);
            pObj = IExtractIconW_Constructor(pidl);
            SHFree(pidl);
            hr = S_OK;
        } else if ((IsEqualIID(riid, IID_IShellLinkW) || IsEqualIID(riid, IID_IShellLinkA))
                   && (cidl == 1)) {
            pidl = ILCombine(pidlRoot, apidl[0]);
            hr = IShellLink_ConstructFromFile(NULL, riid, pidl, (LPVOID*)&pObj);
            SHFree(pidl);
        } else {
            hr = E_NOINTERFACE;
        }

        if (SUCCEEDED(hr) && !pObj)
            hr = E_OUTOFMEMORY;

        *ppvOut = pObj;
    }
    TRACE("(%p)->hr=0x%08x\n", this, hr);
    return hr;
}

/**************************************************************************
*    CControlPanelFolder::GetDisplayNameOf
*/
HRESULT WINAPI CControlPanelFolder::GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet)
{
    if (!pidl)
        return S_FALSE;

    PIDLCPanelStruct *pCPanel = _ILGetCPanelPointer(pidl);

    if (pCPanel)
    {
        return SHSetStrRet(strRet, pCPanel->szName + pCPanel->offsDispName);
    }
    else if (_ILIsSpecialFolder(pidl))
    {
        static const WCHAR* pszCPanelPath = L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}";
        return SHELL32_GetDisplayNameOfGUIDItem(this, pszCPanelPath, pidl, dwFlags, strRet);
    }

    return S_OK;
}

/**************************************************************************
*  CControlPanelFolder::SetNameOf
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
HRESULT WINAPI CControlPanelFolder::SetNameOf(HWND hwndOwner, PCUITEMID_CHILD pidl,    /*simple pidl */
        LPCOLESTR lpName, DWORD dwFlags, PITEMID_CHILD *pPidlOut)
{
    FIXME("(%p)->(%p,pidl=%p,%s,%u,%p)\n", this, hwndOwner, pidl, debugstr_w(lpName), dwFlags, pPidlOut);
    return E_FAIL;
}

HRESULT WINAPI CControlPanelFolder::GetDefaultSearchGUID(GUID *pguid)
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CControlPanelFolder::EnumSearches(IEnumExtraSearch **ppenum)
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CControlPanelFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    TRACE("(%p)\n", this);

    if (pSort) *pSort = 0;
    if (pDisplay) *pDisplay = 0;
    return S_OK;
}

HRESULT WINAPI CControlPanelFolder::GetDefaultColumnState(UINT iColumn, DWORD *pcsFlags)
{
    TRACE("(%p)\n", this);

    if (!pcsFlags || iColumn >= CONROLPANELSHELLVIEWCOLUMNS) return E_INVALIDARG;
    *pcsFlags = ControlPanelSFHeader[iColumn].pcsFlags;
    return S_OK;
}

HRESULT WINAPI CControlPanelFolder::GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CControlPanelFolder::GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd)
{
    if (!psd || iColumn >= CONROLPANELSHELLVIEWCOLUMNS)
        return E_INVALIDARG;

    if (!pidl) 
    {
        psd->fmt = ControlPanelSFHeader[iColumn].fmt;
        psd->cxChar = ControlPanelSFHeader[iColumn].cxChar;
        return SHSetStrRet(&psd->str, shell32_hInstance, ControlPanelSFHeader[iColumn].colnameid);
    }
    else if (_ILIsSpecialFolder(pidl))
    {
        return SHELL32_GetDetailsOfGuidItem(this, pidl, iColumn, psd);
    }
    else 
    {
        PIDLCPanelStruct *pCPanel = _ILGetCPanelPointer(pidl);

        if (!pCPanel)
            return E_FAIL;

        switch(iColumn) 
        {
            case 0:        /* name */
                return SHSetStrRet(&psd->str, pCPanel->szName + pCPanel->offsDispName);
            case 1:        /* comment */
                return SHSetStrRet(&psd->str, pCPanel->szName + pCPanel->offsComment);
        }
    }

    return S_OK;
}

HRESULT WINAPI CControlPanelFolder::MapColumnToSCID(UINT column, SHCOLUMNID *pscid)
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

/************************************************************************
 *    CControlPanelFolder::GetClassID
 */
HRESULT WINAPI CControlPanelFolder::GetClassID(CLSID *lpClassId)
{
    TRACE("(%p)\n", this);

    if (!lpClassId)
        return E_POINTER;
    *lpClassId = CLSID_ControlPanel;

    return S_OK;
}

/************************************************************************
 *    CControlPanelFolder::Initialize
 *
 * NOTES: it makes no sense to change the pidl
 */
HRESULT WINAPI CControlPanelFolder::Initialize(LPCITEMIDLIST pidl)
{
    if (pidlRoot)
        SHFree((LPVOID)pidlRoot);

    pidlRoot = ILClone(pidl);
    return S_OK;
}

/**************************************************************************
 *    CControlPanelFolder::GetCurFolder
 */
HRESULT WINAPI CControlPanelFolder::GetCurFolder(LPITEMIDLIST * pidl)
{
    TRACE("(%p)->(%p)\n", this, pidl);

    if (!pidl)
        return E_POINTER;
    *pidl = ILClone(pidlRoot);
    return S_OK;
}

HRESULT CPanel_GetIconLocationW(LPCITEMIDLIST pidl, LPWSTR szIconFile, UINT cchMax, int* piIndex)
{
    PIDLCPanelStruct* pcpanel = _ILGetCPanelPointer(pidl);

    if (!pcpanel)
        return E_INVALIDARG;

    wcsncpy(szIconFile, pcpanel->szName, cchMax);
    *piIndex = (int)pcpanel->iconIdx != -1 ? pcpanel->iconIdx : 0;

    return S_OK;
}


CCPLItemMenu::CCPLItemMenu()
{
    m_apidl = NULL;
    m_cidl = 0;
}

HRESULT WINAPI CCPLItemMenu::Initialize(UINT cidl, PCUITEMID_CHILD_ARRAY apidl)
{
    m_cidl = cidl;
    m_apidl = const_cast<PCUITEMID_CHILD_ARRAY>(_ILCopyaPidl(apidl, m_cidl));
    if (m_cidl && !m_apidl)
        return E_OUTOFMEMORY;

    return S_OK;
}

CCPLItemMenu::~CCPLItemMenu()
{
    _ILFreeaPidl(const_cast<PITEMID_CHILD *>(m_apidl), m_cidl);
}

HRESULT WINAPI CCPLItemMenu::QueryContextMenu(
    HMENU hMenu,
    UINT indexMenu,
    UINT idCmdFirst,
    UINT idCmdLast,
    UINT uFlags)
{
    _InsertMenuItemW(hMenu, indexMenu++, TRUE, IDS_OPEN, MFT_STRING, MAKEINTRESOURCEW(IDS_OPEN), MFS_DEFAULT);
    _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + 1, MFT_SEPARATOR, NULL, MFS_ENABLED);
    _InsertMenuItemW(hMenu, indexMenu++, TRUE, IDS_CREATELINK, MFT_STRING, MAKEINTRESOURCEW(IDS_CREATELINK), MFS_ENABLED);

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 2);
}

EXTERN_C
void WINAPI Control_RunDLLW(HWND hWnd, HINSTANCE hInst, LPCWSTR cmd, DWORD nCmdShow);

/**************************************************************************
* ICPanel_IContextMenu_InvokeCommand()
*/
HRESULT WINAPI CCPLItemMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
    HRESULT hResult;

    PIDLCPanelStruct *pCPanel = _ILGetCPanelPointer(m_apidl[0]);
    if(!pCPanel)
        return E_FAIL;

    TRACE("(%p)->(invcom=%p verb=%p wnd=%p)\n", this, lpcmi, lpcmi->lpVerb, lpcmi->hwnd);

    if (lpcmi->lpVerb == MAKEINTRESOURCEA(IDS_OPEN)) //FIXME
    {
        WCHAR wszParams[MAX_PATH];

        wcscpy(wszParams, pCPanel->szName);
        wcscat(wszParams, L",");
        wcscat(wszParams, pCPanel->szName + pCPanel->offsDispName);

        Control_RunDLLW (NULL, NULL, wszParams, SW_NORMAL);
    }
    else if (lpcmi->lpVerb == MAKEINTRESOURCEA(IDS_CREATELINK)) //FIXME
    {
        CComPtr<IDataObject> pDataObj;
        LPITEMIDLIST pidl = _ILCreateControlPanel();

        hResult = SHCreateDataObject(pidl, m_cidl, m_apidl, NULL, IID_PPV_ARG(IDataObject, &pDataObj));
        if (FAILED(hResult))
            return hResult;

        SHFree(pidl);

        //FIXME: Use SHCreateLinks
        CComPtr<IShellFolder> psf;
        CComPtr<IDropTarget> pDT;

        hResult = SHGetDesktopFolder(&psf);
        if (FAILED(hResult))
            return hResult;

        hResult = psf->CreateViewObject(NULL, IID_PPV_ARG(IDropTarget, &pDT));
        if (FAILED(hResult))
            return hResult;

        SHSimulateDrop(pDT, pDataObj, MK_CONTROL|MK_SHIFT, NULL, NULL);
    }
    return S_OK;
}

/**************************************************************************
 *  ICPanel_IContextMenu_GetCommandString()
 *
 */
HRESULT WINAPI CCPLItemMenu::GetCommandString(
    UINT_PTR idCommand,
    UINT uFlags,
    UINT* lpReserved,
    LPSTR lpszName,
    UINT uMaxNameLen)
{
    TRACE("(%p)->(idcom=%lx flags=%x %p name=%p len=%x)\n", this, idCommand, uFlags, lpReserved, lpszName, uMaxNameLen);

    FIXME("unknown command string\n");
    return E_FAIL;
}

/**************************************************************************
* ICPanel_IContextMenu_HandleMenuMsg()
*/
HRESULT WINAPI CCPLItemMenu::HandleMenuMsg(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE("ICPanel_IContextMenu_HandleMenuMsg (%p)->(msg=%x wp=%lx lp=%lx)\n", this, uMsg, wParam, lParam);

    return E_NOTIMPL;
}

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

/*
TODO:
1. The selected items list should not be stored in CControlPanelFolder, it should
    be a result returned by an internal method.
*/

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/***********************************************************************
*   control panel implementation in shell namespace
*/

class CControlPanelEnum :
    public IEnumIDListImpl
{
private:
public:
    CControlPanelEnum();
    ~CControlPanelEnum();
    HRESULT WINAPI Initialize(DWORD dwFlags);
    BOOL SHELL_RegisterCPanelApp(LPCSTR path);
    int SHELL_RegisterRegistryCPanelApps(HKEY hkey_root, LPCSTR szRepPath);
    int SHELL_RegisterCPanelFolders(HKEY hkey_root, LPCSTR szRepPath);
    BOOL CreateCPanelEnumList(DWORD dwFlags);

BEGIN_COM_MAP(CControlPanelEnum)
    COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
END_COM_MAP()
};

/***********************************************************************
*   IShellFolder [ControlPanel] implementation
*/

static const shvheader ControlPanelSFHeader[] = {
    {IDS_SHV_COLUMN8, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},/*FIXME*/
    {IDS_SHV_COLUMN9, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 200},/*FIXME*/
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

static LPITEMIDLIST _ILCreateCPanelApplet(LPCSTR name, LPCSTR displayName, LPCSTR comment, int iconIdx)
{
    PIDLCPanelStruct *p;
    LPITEMIDLIST pidl;
    PIDLDATA tmp;
    int size0 = (char*)&tmp.u.cpanel.szName - (char*)&tmp.u.cpanel;
    int size = size0;
    int l;

    tmp.type = PT_CPLAPPLET;
    tmp.u.cpanel.dummy = 0;
    tmp.u.cpanel.iconIdx = iconIdx;

    l = strlen(name);
    size += l + 1;

    tmp.u.cpanel.offsDispName = l+1;
    l = strlen(displayName);
    size += l + 1;

    tmp.u.cpanel.offsComment = tmp.u.cpanel.offsDispName + 1 + l;
    l = strlen(comment);
    size += l + 1;

    pidl = (LPITEMIDLIST)SHAlloc(size + 4);
    if (!pidl)
        return NULL;

    pidl->mkid.cb = size + 2;
    memcpy(pidl->mkid.abID, &tmp, 2 + size0);

    p = &((PIDLDATA *)pidl->mkid.abID)->u.cpanel;
    strcpy(p->szName, name);
    strcpy(p->szName+tmp.u.cpanel.offsDispName, displayName);
    strcpy(p->szName+tmp.u.cpanel.offsComment, comment);

    *(WORD*)((char*)pidl + (size + 2)) = 0;

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
        return (PIDLCPanelStruct *)&(pdata->u.cpanel);

    return NULL;
}

BOOL CControlPanelEnum::SHELL_RegisterCPanelApp(LPCSTR path)
{
    LPITEMIDLIST pidl;
    CPlApplet* applet;
    CPanel panel;
    CPLINFO info;
    unsigned i;
    int iconIdx;

    char displayName[MAX_PATH];
    char comment[MAX_PATH];

    WCHAR wpath[MAX_PATH];

    MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, MAX_PATH);

    panel.first = NULL;
    applet = Control_LoadApplet(0, wpath, &panel);

    if (applet)
    {
        for (i = 0; i < applet->count; ++i)
        {
            WideCharToMultiByte(CP_ACP, 0, applet->info[i].szName, -1, displayName, MAX_PATH, 0, 0);
            WideCharToMultiByte(CP_ACP, 0, applet->info[i].szInfo, -1, comment, MAX_PATH, 0, 0);

            applet->proc(0, CPL_INQUIRE, i, (LPARAM)&info);

            if (info.idIcon > 0)
                iconIdx = -info.idIcon; /* negative icon index instead of icon number */
            else
                iconIdx = 0;

            pidl = _ILCreateCPanelApplet(path, displayName, comment, iconIdx);

            if (pidl)
                AddToEnumList(pidl);
        }
        Control_UnloadApplet(applet);
    }
    return TRUE;
}

int CControlPanelEnum::SHELL_RegisterRegistryCPanelApps(HKEY hkey_root, LPCSTR szRepPath)
{
    char name[MAX_PATH];
    char value[MAX_PATH];
    HKEY hkey;

    int cnt = 0;

    if (RegOpenKeyA(hkey_root, szRepPath, &hkey) == ERROR_SUCCESS)
    {
        int idx = 0;

        for(; ; idx++)
        {
            DWORD nameLen = MAX_PATH;
            DWORD valueLen = MAX_PATH;

            if (RegEnumValueA(hkey, idx, name, &nameLen, NULL, NULL, (LPBYTE)&value, &valueLen) != ERROR_SUCCESS)
                break;

            if (SHELL_RegisterCPanelApp(value))
                ++cnt;
        }
        RegCloseKey(hkey);
    }

    return cnt;
}

int CControlPanelEnum::SHELL_RegisterCPanelFolders(HKEY hkey_root, LPCSTR szRepPath)
{
    char name[MAX_PATH];
    HKEY hkey;

    int cnt = 0;

    if (RegOpenKeyA(hkey_root, szRepPath, &hkey) == ERROR_SUCCESS)
    {
        int idx = 0;
        for (; ; idx++)
        {
            if (RegEnumKeyA(hkey, idx, name, MAX_PATH) != ERROR_SUCCESS)
                break;

            if (*name == '{')
            {
                LPITEMIDLIST pidl = _ILCreateGuidFromStrA(name);

                if (pidl && AddToEnumList(pidl))
                    ++cnt;
            }
        }

        RegCloseKey(hkey);
    }

    return cnt;
}

/**************************************************************************
 *  CreateCPanelEnumList()
 */
BOOL CControlPanelEnum::CreateCPanelEnumList(DWORD dwFlags)
{
    CHAR szPath[MAX_PATH];
    WIN32_FIND_DATAA wfd;
    HANDLE hFile;

    TRACE("(%p)->(flags=0x%08x)\n", this, dwFlags);

    /* enumerate control panel folders */
    if (dwFlags & SHCONTF_FOLDERS)
        SHELL_RegisterCPanelFolders(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ControlPanel\\NameSpace");

    /* enumerate the control panel applets */
    if (dwFlags & SHCONTF_NONFOLDERS)
    {
        LPSTR p;

        GetSystemDirectoryA(szPath, MAX_PATH);
        p = PathAddBackslashA(szPath);
        strcpy(p, "*.cpl");

        TRACE("-- (%p)-> enumerate SHCONTF_NONFOLDERS of %s\n", this, debugstr_a(szPath));
        hFile = FindFirstFileA(szPath, &wfd);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (!(dwFlags & SHCONTF_INCLUDEHIDDEN) && (wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
                    continue;

                if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    strcpy(p, wfd.cFileName);
                    if (strcmp(wfd.cFileName, "ncpa.cpl"))
                        SHELL_RegisterCPanelApp(szPath);
                }
            } while(FindNextFileA(hFile, &wfd));
            FindClose(hFile);
        }

        SHELL_RegisterRegistryCPanelApps(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\Cpls");
        SHELL_RegisterRegistryCPanelApps(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\Cpls");
    }
    return TRUE;
}

CControlPanelFolder::CControlPanelFolder()
{
    pidlRoot = NULL;    /* absolute pidl */
    dwAttributes = 0;        /* attributes returned by GetAttributesOf FIXME: use it */
    apidl = NULL;
    cidl = 0;
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
*    ISF_ControlPanel_fnParseDisplayName
*/
HRESULT WINAPI CControlPanelFolder::ParseDisplayName(HWND hwndOwner,
                   LPBC pbc,
                   LPOLESTR lpszDisplayName,
                   DWORD * pchEaten, LPITEMIDLIST * ppidl, DWORD * pdwAttributes)
{
    WCHAR szElement[MAX_PATH];
    LPCWSTR szNext = NULL;
    LPITEMIDLIST pidlTemp = NULL;
    HRESULT hr = S_OK;
    CLSID clsid;

    TRACE ("(%p)->(HWND=%p,%p,%p=%s,%p,pidl=%p,%p)\n",
           this, hwndOwner, pbc, lpszDisplayName, debugstr_w(lpszDisplayName),
           pchEaten, ppidl, pdwAttributes);

    if (!lpszDisplayName || !ppidl)
        return E_INVALIDARG;

    *ppidl = 0;

    if (pchEaten)
        *pchEaten = 0;        /* strange but like the original */

    if (lpszDisplayName[0] == ':' && lpszDisplayName[1] == ':')
    {
        szNext = GetNextElementW (lpszDisplayName, szElement, MAX_PATH);
        TRACE ("-- element: %s\n", debugstr_w (szElement));
        CLSIDFromString (szElement + 2, &clsid);
        pidlTemp = _ILCreateGuid (PT_GUID, clsid);
    }
    else if( (pidlTemp = SHELL32_CreatePidlFromBindCtx(pbc, lpszDisplayName)) )
    {
        *ppidl = pidlTemp;
        return S_OK;
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
                hr = SHELL32_GetItemAttributes(this,
                                               pidlTemp, pdwAttributes);
        }
    }

    *ppidl = pidlTemp;

    TRACE ("(%p)->(-- ret=0x%08x)\n", this, hr);

    return hr;
}

/**************************************************************************
*        ISF_ControlPanel_fnEnumObjects
*/
HRESULT WINAPI CControlPanelFolder::EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST * ppEnumIDList)
{
    CComObject<CControlPanelEnum>            *theEnumerator;
    CComPtr<IEnumIDList>                    result;
    HRESULT                                    hResult;

    TRACE ("(%p)->(HWND=%p flags=0x%08x pplist=%p)\n", this, hwndOwner, dwFlags, ppEnumIDList);

    if (ppEnumIDList == NULL)
        return E_POINTER;
    *ppEnumIDList = NULL;
    ATLTRY (theEnumerator = new CComObject<CControlPanelEnum>);
    if (theEnumerator == NULL)
        return E_OUTOFMEMORY;
    hResult = theEnumerator->QueryInterface (IID_IEnumIDList, (void **)&result);
    if (FAILED (hResult))
    {
        delete theEnumerator;
        return hResult;
    }
    hResult = theEnumerator->Initialize (dwFlags);
    if (FAILED (hResult))
        return hResult;
    *ppEnumIDList = result.Detach ();

    TRACE ("-- (%p)->(new ID List: %p)\n", this, *ppEnumIDList);

    return S_OK;
}

/**************************************************************************
*        ISF_ControlPanel_fnBindToObject
*/
HRESULT WINAPI CControlPanelFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    TRACE("(%p)->(pidl=%p,%p,%s,%p)\n", this, pidl, pbcReserved, shdebugstr_guid(&riid), ppvOut);

    return SHELL32_BindToChild(pidlRoot, NULL, pidl, riid, ppvOut);
}

/**************************************************************************
*    ISF_ControlPanel_fnBindToStorage
*/
HRESULT WINAPI CControlPanelFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    FIXME("(%p)->(pidl=%p,%p,%s,%p) stub\n", this, pidl, pbcReserved, shdebugstr_guid(&riid), ppvOut);

    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
*     ISF_ControlPanel_fnCompareIDs
*/

HRESULT WINAPI CControlPanelFolder::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    int nReturn;

    TRACE("(%p)->(0x%08lx,pidl1=%p,pidl2=%p)\n", this, lParam, pidl1, pidl2);
    nReturn = SHELL32_CompareIDs(this, lParam, pidl1, pidl2);
    TRACE("-- %i\n", nReturn);
    return nReturn;
}

/**************************************************************************
*    ISF_ControlPanel_fnCreateViewObject
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
        hr = IShellView_Constructor((IShellFolder *)this, &pShellView);
        if (pShellView) {
        hr = pShellView->QueryInterface(riid, ppvOut);
        }
    }
    }
    TRACE("--(%p)->(interface=%p)\n", this, ppvOut);
    return hr;
}

/**************************************************************************
*  ISF_ControlPanel_fnGetAttributesOf
*/
HRESULT WINAPI CControlPanelFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl, DWORD * rgfInOut)
{
    HRESULT hr = S_OK;

    TRACE("(%p)->(cidl=%d apidl=%p mask=%p (0x%08x))\n",
          this, cidl, apidl, rgfInOut, rgfInOut ? *rgfInOut : 0);

    if (!rgfInOut)
        return E_INVALIDARG;
    if (cidl && !apidl)
        return E_INVALIDARG;

    if (*rgfInOut == 0)
    *rgfInOut = ~0;

    while(cidl > 0 && *apidl) {
    pdump(*apidl);
    SHELL32_GetItemAttributes(this, *apidl, rgfInOut);
    apidl++;
    cidl--;
    }
    /* make sure SFGAO_VALIDATE is cleared, some apps depend on that */
    *rgfInOut &= ~SFGAO_VALIDATE;

    TRACE("-- result=0x%08x\n", *rgfInOut);
    return hr;
}

/**************************************************************************
*    ISF_ControlPanel_fnGetUIObjectOf
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
                UINT cidl, LPCITEMIDLIST * apidl, REFIID riid, UINT * prgfInOut, LPVOID * ppvOut)
{
    LPITEMIDLIST pidl;
    IUnknown *pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE("(%p)->(%p,%u,apidl=%p,%s,%p,%p)\n",
       this, hwndOwner, cidl, apidl, shdebugstr_guid(&riid), prgfInOut, ppvOut);

    if (ppvOut) {
    *ppvOut = NULL;

    if (IsEqualIID(riid, IID_IContextMenu) &&(cidl >= 1)) {
        // TODO
        // create a seperate item struct
        //
        pObj = (IContextMenu *)this;
        this->apidl = apidl;
        cidl = cidl;
        pObj->AddRef();
        hr = S_OK;
    } else if (IsEqualIID(riid, IID_IDataObject) &&(cidl >= 1)) {
        hr = IDataObject_Constructor(hwndOwner, pidlRoot, apidl, cidl, (IDataObject **)&pObj);
    } else if (IsEqualIID(riid, IID_IExtractIconA) &&(cidl == 1)) {
        pidl = ILCombine(pidlRoot, apidl[0]);
        pObj = (LPUNKNOWN) IExtractIconA_Constructor(pidl);
        SHFree(pidl);
        hr = S_OK;
    } else if (IsEqualIID(riid, IID_IExtractIconW) &&(cidl == 1)) {
        pidl = ILCombine(pidlRoot, apidl[0]);
        pObj = (LPUNKNOWN) IExtractIconW_Constructor(pidl);
        SHFree(pidl);
        hr = S_OK;
    } else if ((IsEqualIID(riid, IID_IShellLinkW) || IsEqualIID(riid, IID_IShellLinkA))
                && (cidl == 1)) {
        pidl = ILCombine(pidlRoot, apidl[0]);
        hr = IShellLink_ConstructFromFile(NULL, riid, pidl,(LPVOID*)&pObj);
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
*    ISF_ControlPanel_fnGetDisplayNameOf
*/
HRESULT WINAPI CControlPanelFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET strRet)
{
    CHAR szPath[MAX_PATH];
    WCHAR wszPath[MAX_PATH+1]; /* +1 for potential backslash */
    PIDLCPanelStruct* pcpanel;

    *szPath = '\0';

    TRACE("(%p)->(pidl=%p,0x%08x,%p)\n", this, pidl, dwFlags, strRet);
    pdump(pidl);

    if (!pidl || !strRet)
    return E_INVALIDARG;

    pcpanel = _ILGetCPanelPointer(pidl);

    if (pcpanel)
    {
        lstrcpyA(szPath, pcpanel->szName+pcpanel->offsDispName);

        if (!(dwFlags & SHGDN_FORPARSING))
            FIXME("retrieve display name from control panel app\n");
    }
    /* take names of special folders only if it's only this folder */
    else if (_ILIsSpecialFolder(pidl))
    {
        BOOL bSimplePidl = _ILIsPidlSimple(pidl);

        if (bSimplePidl)
        {
            _ILSimpleGetTextW(pidl, wszPath, MAX_PATH);    /* append my own path */
        }
        else
        {
            FIXME("special pidl\n");
        }

        if ((dwFlags & SHGDN_FORPARSING) && !bSimplePidl)
        {
            /* go deeper if needed */
            int len = 0;

            PathAddBackslashW(wszPath);
            len = wcslen(wszPath);

            if (!SUCCEEDED(SHELL32_GetDisplayNameOfChild(this, pidl, dwFlags, wszPath + len, MAX_PATH + 1 - len)))
                return E_OUTOFMEMORY;
            
            if (!WideCharToMultiByte(CP_ACP, 0, wszPath, -1, szPath, MAX_PATH, NULL, NULL))
                wszPath[0] = '\0';
        }
        else
        {
            if (bSimplePidl)
            {
                if (!WideCharToMultiByte(CP_ACP, 0, wszPath, -1, szPath, MAX_PATH, NULL, NULL))
                    wszPath[0] = '\0';
            }
        }
    }

    strRet->uType = STRRET_CSTR;
    lstrcpynA(strRet->cStr, szPath, MAX_PATH);

    TRACE("--(%p)->(%s)\n", this, szPath);
    return S_OK;
}

/**************************************************************************
*  ISF_ControlPanel_fnSetNameOf
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
HRESULT WINAPI CControlPanelFolder::SetNameOf(HWND hwndOwner, LPCITEMIDLIST pidl,    /*simple pidl */
                          LPCOLESTR lpName, DWORD dwFlags, LPITEMIDLIST * pPidlOut)
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

HRESULT WINAPI CControlPanelFolder::GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CControlPanelFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *psd)
{
    HRESULT hr;

    TRACE("(%p)->(%p %i %p)\n", this, pidl, iColumn, psd);

    if (!psd || iColumn >= CONROLPANELSHELLVIEWCOLUMNS)
    return E_INVALIDARG;

    if (!pidl) {
    psd->fmt = ControlPanelSFHeader[iColumn].fmt;
    psd->cxChar = ControlPanelSFHeader[iColumn].cxChar;
    psd->str.uType = STRRET_CSTR;
    LoadStringA(shell32_hInstance, ControlPanelSFHeader[iColumn].colnameid, psd->str.cStr, MAX_PATH);
    return S_OK;
    } else {
    psd->str.cStr[0] = 0x00;
    psd->str.uType = STRRET_CSTR;
    switch(iColumn) {
    case 0:        /* name */
        hr = GetDisplayNameOf(pidl, SHGDN_NORMAL | SHGDN_INFOLDER, &psd->str);
        break;
    case 1:        /* comment */
        _ILGetFileType(pidl, psd->str.cStr, MAX_PATH);
        break;
    }
    hr = S_OK;
    }

    return hr;
}
HRESULT WINAPI CControlPanelFolder::MapColumnToSCID(UINT column, SHCOLUMNID *pscid)
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

/************************************************************************
 *    ICPanel_PersistFolder2_GetClassID
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
 *    ICPanel_PersistFolder2_Initialize
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
 *    IPersistFolder2_fnGetCurFolder
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

    MultiByteToWideChar(CP_ACP, 0, pcpanel->szName, -1, szIconFile, cchMax);
    *piIndex = (int)pcpanel->iconIdx != -1 ? pcpanel->iconIdx : 0;

    return S_OK;
}


/**************************************************************************
* IShellExecuteHookW Implementation
*/

HRESULT
ExecuteAppletFromCLSID(LPOLESTR pOleStr)
{
    WCHAR szCmd[MAX_PATH];
    WCHAR szExpCmd[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    WCHAR szBuffer[90] = { 'C', 'L', 'S', 'I', 'D', '\\', 0 };
    DWORD dwType, dwSize;

    wcscpy(&szBuffer[6], pOleStr);
    wcscat(szBuffer, L"\\shell\\open\\command");

    dwSize = sizeof(szCmd);
    if (RegGetValueW(HKEY_CLASSES_ROOT, szBuffer, NULL, RRF_RT_REG_SZ, &dwType, (PVOID)szCmd, &dwSize) != ERROR_SUCCESS)
    {
        ERR("RegGetValueW failed with %u\n", GetLastError());
        return E_FAIL;
    }

#if 0
    if (dwType != RRF_RT_REG_SZ && dwType != RRF_RT_REG_EXPAND_SZ)
        return E_FAIL;
#endif

    if (!ExpandEnvironmentStringsW(szCmd, szExpCmd, sizeof(szExpCmd)/sizeof(WCHAR)))
        return E_FAIL;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    if (!CreateProcessW(NULL, szExpCmd, NULL, NULL, FALSE, 0, NULL,    NULL, &si, &pi))
        return E_FAIL;

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return S_OK;
}


HRESULT WINAPI CControlPanelFolder::Execute(LPSHELLEXECUTEINFOW psei)
{
    static const WCHAR wCplopen[] = {'c','p','l','o','p','e','n','\0'};
    SHELLEXECUTEINFOW sei_tmp;
    PIDLCPanelStruct* pcpanel;
    WCHAR path[MAX_PATH];
    WCHAR params[MAX_PATH];
    BOOL ret;
    HRESULT hr;
    int l;

    TRACE("(%p)->execute(%p)\n", this, psei);

    if (!psei)
        return E_INVALIDARG;

    pcpanel = _ILGetCPanelPointer(ILFindLastID((LPCITEMIDLIST)psei->lpIDList));

    if (!pcpanel)
    {
        LPOLESTR pOleStr;

        IID * iid = _ILGetGUIDPointer(ILFindLastID((LPCITEMIDLIST)psei->lpIDList));
        if (!iid)
            return E_INVALIDARG;
        if (StringFromCLSID(*iid, &pOleStr) == S_OK)
        {

            hr = ExecuteAppletFromCLSID(pOleStr);
            CoTaskMemFree(pOleStr);
            return hr;
        }

        return E_INVALIDARG;
    }
    path[0] = '\"';
    /* Return value from MultiByteToWideChar includes terminating NUL, which
     * compensates for the starting double quote we just put in */
    l = MultiByteToWideChar(CP_ACP, 0, pcpanel->szName, -1, path+1, MAX_PATH);

    /* pass applet name to Control_RunDLL to distinguish between applets in one .cpl file */
    path[l++] = '"';
    path[l] = '\0';

    MultiByteToWideChar(CP_ACP, 0, pcpanel->szName+pcpanel->offsDispName, -1, params, MAX_PATH);

    memcpy(&sei_tmp, psei, sizeof(sei_tmp));
    sei_tmp.lpFile = path;
    sei_tmp.lpParameters = params;
    sei_tmp.fMask &= ~SEE_MASK_INVOKEIDLIST;
    sei_tmp.lpVerb = wCplopen;

    ret = ShellExecuteExW(&sei_tmp);
    if (ret)
        return S_OK;
    else
        return S_FALSE;
}

/**************************************************************************
* IShellExecuteHookA Implementation
*/

HRESULT WINAPI CControlPanelFolder::Execute(LPSHELLEXECUTEINFOA psei)
{
    SHELLEXECUTEINFOA sei_tmp;
    PIDLCPanelStruct* pcpanel;
    char path[MAX_PATH];
    BOOL ret;

    TRACE("(%p)->execute(%p)\n", this, psei);

    if (!psei)
    return E_INVALIDARG;

    pcpanel = _ILGetCPanelPointer(ILFindLastID((LPCITEMIDLIST)psei->lpIDList));

    if (!pcpanel)
    return E_INVALIDARG;

    path[0] = '\"';
    lstrcpyA(path+1, pcpanel->szName);

    /* pass applet name to Control_RunDLL to distinguish between applets in one .cpl file */
    lstrcatA(path, "\" ");
    lstrcatA(path, pcpanel->szName+pcpanel->offsDispName);

    memcpy(&sei_tmp, psei, sizeof(sei_tmp));
    sei_tmp.lpFile = path;
    sei_tmp.fMask &= ~SEE_MASK_INVOKEIDLIST;

    ret = ShellExecuteExA(&sei_tmp);
    if (ret)
    return S_OK;
    else
    return S_FALSE;
}

/**************************************************************************
* IContextMenu2 Implementation
*/

/**************************************************************************
* ICPanel_IContextMenu_QueryContextMenu()
*/
HRESULT WINAPI CControlPanelFolder::QueryContextMenu(
    HMENU hMenu,
    UINT indexMenu,
    UINT idCmdFirst,
    UINT idCmdLast,
    UINT uFlags)
{
    WCHAR szBuffer[30] = {0};
    ULONG Count = 1;

    TRACE("(%p)->(hmenu=%p indexmenu=%x cmdfirst=%x cmdlast=%x flags=%x )\n",
          this, hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    if (LoadStringW(shell32_hInstance, IDS_OPEN, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
    {
        szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, IDS_OPEN, MFT_STRING, szBuffer, MFS_DEFAULT); //FIXME identifier
        Count++;
    }

    if (LoadStringW(shell32_hInstance, IDS_CREATELINK, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
    {
        if (Count)
        {
            _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count, MFT_SEPARATOR, NULL, MFS_ENABLED);
        }
        szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';

        _InsertMenuItemW(hMenu, indexMenu++, TRUE, IDS_CREATELINK, MFT_STRING, szBuffer, MFS_ENABLED); //FIXME identifier
        Count++;
    }
    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, Count);
}

/**************************************************************************
* ICPanel_IContextMenu_InvokeCommand()
*/
HRESULT WINAPI CControlPanelFolder::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
    SHELLEXECUTEINFOW sei;
    WCHAR szPath[MAX_PATH];
    char szTarget[MAX_PATH];
    STRRET strret;
    WCHAR* pszPath;
    INT Length, cLength;
    PIDLCPanelStruct *pcpanel;
    CComPtr<IPersistFile>                ppf;
    CComPtr<IShellLinkA>                isl;
    HRESULT hResult;

    TRACE("(%p)->(invcom=%p verb=%p wnd=%p)\n",this,lpcmi,lpcmi->lpVerb, lpcmi->hwnd);

    if (lpcmi->lpVerb == MAKEINTRESOURCEA(IDS_OPEN)) //FIXME
    {
       ZeroMemory(&sei, sizeof(sei));
       sei.cbSize = sizeof(sei);
       sei.fMask = SEE_MASK_INVOKEIDLIST;
       sei.lpIDList = ILCombine(pidlRoot, apidl[0]);
       sei.hwnd = lpcmi->hwnd;
       sei.nShow = SW_SHOWNORMAL;
       sei.lpVerb = L"open";

       if (ShellExecuteExW(&sei) == FALSE)
            return E_FAIL;
    }
    else if (lpcmi->lpVerb == MAKEINTRESOURCEA(IDS_CREATELINK)) //FIXME
    {
        if (!SHGetSpecialFolderPathW(NULL, szPath, CSIDL_DESKTOPDIRECTORY, FALSE))
             return E_FAIL;

        pszPath = PathAddBackslashW(szPath);
        if (!pszPath)
             return E_FAIL;

        if (GetDisplayNameOf(apidl[0], SHGDN_FORPARSING, &strret) != S_OK)
            return E_FAIL;

        Length =  MAX_PATH - (pszPath - szPath);
        cLength = strlen(strret.cStr);
        if (Length < cLength + 5)
        {
            FIXME("\n");
            return E_FAIL;
        }

        if (MultiByteToWideChar(CP_ACP, 0, strret.cStr, cLength + 1, pszPath, Length))
        {
            pszPath += cLength;
            Length -= cLength;
        }

        if (Length > 10)
        {
            wcscpy(pszPath, L" - ");
            cLength = LoadStringW(shell32_hInstance, IDS_LNK_FILE, &pszPath[3], Length - 4) + 3;
            if (cLength + 5 > Length)
                cLength = Length - 5;
            Length -= cLength;
            pszPath += cLength;
        }
        wcscpy(pszPath, L".lnk");

        pcpanel = _ILGetCPanelPointer(ILFindLastID(apidl[0]));
        if (pcpanel)
        {
           strncpy(szTarget, pcpanel->szName, MAX_PATH);
        }
        else
        {
           FIXME("Couldn't retrieve pointer to cpl structure\n");
           return E_FAIL;
        }
        hResult = ShellLink::_CreatorClass::CreateInstance(NULL, IID_IShellLinkA, (void **)&isl);
        if (SUCCEEDED(hResult))
        {
            isl->SetPath(szTarget);
            if (SUCCEEDED(isl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf)))
                ppf->Save(szPath, TRUE);
        }
        return NOERROR;
    }
    return S_OK;
}

/**************************************************************************
 *  ICPanel_IContextMenu_GetCommandString()
 *
 */
HRESULT WINAPI CControlPanelFolder::GetCommandString(
    UINT_PTR idCommand,
    UINT uFlags,
    UINT* lpReserved,
    LPSTR lpszName,
    UINT uMaxNameLen)
{
    TRACE("(%p)->(idcom=%lx flags=%x %p name=%p len=%x)\n",this, idCommand, uFlags, lpReserved, lpszName, uMaxNameLen);

    FIXME("unknown command string\n");
    return E_FAIL;
}

/**************************************************************************
* ICPanel_IContextMenu_HandleMenuMsg()
*/
HRESULT WINAPI CControlPanelFolder::HandleMenuMsg(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE("ICPanel_IContextMenu_HandleMenuMsg (%p)->(msg=%x wp=%lx lp=%lx)\n",this, uMsg, wParam, lParam);

    return E_NOTIMPL;
}

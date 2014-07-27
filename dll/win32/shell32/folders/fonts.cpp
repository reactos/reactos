/*
 * Fonts folder
 *
 * Copyright 2008       Johannes Anderwald <johannes.anderwald@reactos.org>
 * Copyright 2009       Andrew Hill
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

WINE_DEFAULT_DEBUG_CHANNEL (shell);

/*
This folder should not exist. It is just a file system folder... The \windows\fonts
directory contains a hidden desktop.ini with a UIHandler entry that specifies a class
that lives in fontext.dll. The UI handler creates a custom view for the folder, which
is what we normally see. However, the folder is a perfectly normal CFSFolder.
*/

/***********************************************************************
*   IShellFolder implementation
*/

class CDesktopFolderEnumZ: public IEnumIDListImpl
{
    public:
        CDesktopFolderEnumZ();
        ~CDesktopFolderEnumZ();
        HRESULT WINAPI Initialize(DWORD dwFlags);
        BOOL CreateFontsEnumList(DWORD dwFlags);

        BEGIN_COM_MAP(CDesktopFolderEnumZ)
        COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
        END_COM_MAP()
};

static shvheader FontsSFHeader[] = {
    {IDS_SHV_COLUMN8, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},
    {IDS_SHV_COLUMN_FONTTYPE , SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},
    {IDS_SHV_COLUMN2, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},
    {IDS_SHV_COLUMN12, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15}
};

#define COLUMN_NAME     0
#define COLUMN_TYPE     1
#define COLUMN_SIZE     2
#define COLUMN_FILENAME 3

#define FontsSHELLVIEWCOLUMNS (4)

CDesktopFolderEnumZ::CDesktopFolderEnumZ()
{
}

CDesktopFolderEnumZ::~CDesktopFolderEnumZ()
{
}

HRESULT WINAPI CDesktopFolderEnumZ::Initialize(DWORD dwFlags)
{
    if (CreateFontsEnumList(dwFlags) == FALSE)
        return E_FAIL;
    return S_OK;
}

static LPITEMIDLIST _ILCreateFontItem(LPWSTR pszFont, LPWSTR pszFile)
{
    PIDLDATA tmp;
    LPITEMIDLIST pidl;
    PIDLFontStruct * p;
    int size0 = (char*)&tmp.u.cfont.szName - (char*)&tmp.u.cfont;
    int size = size0;

    tmp.type = 0x00;
    tmp.u.cfont.dummy = 0xFF;
    tmp.u.cfont.offsFile = wcslen(pszFont) + 1;

    size += (tmp.u.cfont.offsFile + wcslen(pszFile) + 1) * sizeof(WCHAR);

    pidl = (LPITEMIDLIST)SHAlloc(size + 4);
    if (!pidl)
        return pidl;

    pidl->mkid.cb = size + 2;
    memcpy(pidl->mkid.abID, &tmp, 2 + size0);

    p = &((PIDLDATA*)pidl->mkid.abID)->u.cfont;
    wcscpy(p->szName, pszFont);
    wcscpy(p->szName + tmp.u.cfont.offsFile, pszFile);

    *(WORD*)((char*)pidl + (size + 2)) = 0;
    return pidl;
}

static PIDLFontStruct * _ILGetFontStruct(LPCITEMIDLIST pidl)
{
    LPPIDLDATA pdata = _ILGetDataPointer(pidl);

    if (pdata && pdata->type == 0x00)
        return (PIDLFontStruct*) & (pdata->u.cfont);

    return NULL;
}

/**************************************************************************
 *  CDesktopFolderEnumZ::CreateFontsEnumList()
 */
BOOL CDesktopFolderEnumZ::CreateFontsEnumList(DWORD dwFlags)
{
    WCHAR szPath[MAX_PATH];
    WCHAR szName[LF_FACESIZE+20];
    WCHAR szFile[MAX_PATH];
    LPWSTR pszPath;
    UINT Length;
    LONG ret;
    DWORD dwType, dwName, dwFile, dwIndex;
    LPITEMIDLIST pidl;
    HKEY hKey;

    if (dwFlags & SHCONTF_NONFOLDERS)
    {
        if (!SHGetSpecialFolderPathW(NULL, szPath, CSIDL_FONTS, FALSE))
            return FALSE;

        pszPath = PathAddBackslashW(szPath);
        if (!pszPath)
            return FALSE;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
            return FALSE;

        Length = pszPath - szPath;
        dwIndex = 0;
        do
        {
            dwName = sizeof(szName) / sizeof(WCHAR);
            dwFile = sizeof(szFile) / sizeof(WCHAR);
            ret = RegEnumValueW(hKey, dwIndex++, szName, &dwName, NULL, &dwType, (LPBYTE)szFile, &dwFile);
            if (ret == ERROR_SUCCESS)
            {
                szFile[(sizeof(szFile)/sizeof(WCHAR))-1] = L'\0';
                if (dwType == REG_SZ && wcslen(szFile) + Length + 1 < (sizeof(szPath) / sizeof(WCHAR)))
                {
                    wcscpy(&szPath[Length], szFile);
                    pidl = _ILCreateFontItem(szName, szPath);
                    TRACE("pidl %p name %s path %s\n", pidl, debugstr_w(szName), debugstr_w(szPath));
                    if (pidl)
                    {
                        if (!AddToEnumList(pidl))
                            SHFree(pidl);
                    }
                }
            }
        } while(ret != ERROR_NO_MORE_ITEMS);
        RegCloseKey(hKey);

    }
    return TRUE;
}

CFontsFolder::CFontsFolder()
{
    pidlRoot = NULL;
    apidl = NULL;
}

CFontsFolder::~CFontsFolder()
{
    TRACE("-- destroying IShellFolder(%p)\n", this);
    SHFree(pidlRoot);
}

HRESULT WINAPI CFontsFolder::FinalConstruct()
{
    pidlRoot = _ILCreateFont();    /* my qualified pidl */
    if (pidlRoot == NULL)
        return E_OUTOFMEMORY;
    return S_OK;
}

/**************************************************************************
*    CFontsFolder::ParseDisplayName
*/
HRESULT WINAPI CFontsFolder::ParseDisplayName(
    HWND hwndOwner,
    LPBC pbcReserved,
    LPOLESTR lpszDisplayName,
    DWORD *pchEaten,
    LPITEMIDLIST *ppidl,
    DWORD * pdwAttributes)
{
    HRESULT hr = E_UNEXPECTED;

    TRACE ("(%p)->(HWND=%p,%p,%p=%s,%p,pidl=%p,%p)\n", this,
           hwndOwner, pbcReserved, lpszDisplayName, debugstr_w (lpszDisplayName),
           pchEaten, ppidl, pdwAttributes);

    *ppidl = 0;
    if (pchEaten)
        *pchEaten = 0;        /* strange but like the original */

    TRACE ("(%p)->(-- ret=0x%08x)\n", this, hr);

    return hr;
}

/**************************************************************************
*        CFontsFolder::EnumObjects
*/
HRESULT WINAPI CFontsFolder::EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
{
    CComObject<CDesktopFolderEnumZ>            *theEnumerator;
    CComPtr<IEnumIDList>                    result;
    HRESULT                                    hResult;

    TRACE ("(%p)->(HWND=%p flags=0x%08x pplist=%p)\n", this, hwndOwner, dwFlags, ppEnumIDList);

    if (ppEnumIDList == NULL)
        return E_POINTER;
    *ppEnumIDList = NULL;
    ATLTRY (theEnumerator = new CComObject<CDesktopFolderEnumZ>);
    if (theEnumerator == NULL)
        return E_OUTOFMEMORY;
    hResult = theEnumerator->QueryInterface(IID_PPV_ARG(IEnumIDList, &result));
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
*        CFontsFolder::BindToObject
*/
HRESULT WINAPI CFontsFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    TRACE ("(%p)->(pidl=%p,%p,%s,%p)\n", this,
           pidl, pbcReserved, shdebugstr_guid (&riid), ppvOut);

    return SHELL32_BindToChild (pidlRoot, NULL, pidl, riid, ppvOut);
}

/**************************************************************************
*    CFontsFolder::BindToStorage
*/
HRESULT WINAPI CFontsFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    FIXME ("(%p)->(pidl=%p,%p,%s,%p) stub\n", this,
           pidl, pbcReserved, shdebugstr_guid (&riid), ppvOut);

    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
*     CFontsFolder::CompareIDs
*/

HRESULT WINAPI CFontsFolder::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    int nReturn;

    TRACE ("(%p)->(0x%08lx,pidl1=%p,pidl2=%p)\n", this, lParam, pidl1, pidl2);
    nReturn = SHELL32_CompareIDs (this, lParam, pidl1, pidl2);
    TRACE ("-- %i\n", nReturn);
    return nReturn;
}

/**************************************************************************
*    CFontsFolder::CreateViewObject
*/
HRESULT WINAPI CFontsFolder::CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut)
{
    CComPtr<IShellView>                    pShellView;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(hwnd=%p,%s,%p)\n", this,
           hwndOwner, shdebugstr_guid (&riid), ppvOut);

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if (IsEqualIID (riid, IID_IDropTarget))
    {
        WARN ("IDropTarget not implemented\n");
        hr = E_NOTIMPL;
    }
    else if (IsEqualIID (riid, IID_IContextMenu))
    {
        WARN ("IContextMenu not implemented\n");
        hr = E_NOTIMPL;
    }
    else if (IsEqualIID (riid, IID_IShellView))
    {
        hr = IShellView_Constructor (this, &pShellView);
        if (pShellView)
            hr = pShellView->QueryInterface(riid, ppvOut);
    }
    TRACE ("-- (%p)->(interface=%p)\n", this, ppvOut);
    return hr;
}

/**************************************************************************
*  CFontsFolder::GetAttributesOf
*/
HRESULT WINAPI CFontsFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, DWORD *rgfInOut)
{
    HRESULT hr = S_OK;

    TRACE ("(%p)->(cidl=%d apidl=%p mask=%p (0x%08x))\n", this,
           cidl, apidl, rgfInOut, rgfInOut ? *rgfInOut : 0);

    if (!rgfInOut)
        return E_INVALIDARG;
    if (cidl && !apidl)
        return E_INVALIDARG;

    if (*rgfInOut == 0)
        *rgfInOut = ~0;

    if (cidl == 0)
    {
        CComPtr<IShellFolder>            psfParent;
        LPCITEMIDLIST rpidl = NULL;

        hr = SHBindToParent(pidlRoot, IID_PPV_ARG(IShellFolder, &psfParent), (LPCITEMIDLIST *)&rpidl);
        if (SUCCEEDED(hr))
            SHELL32_GetItemAttributes (psfParent, rpidl, rgfInOut);
    }
    else
    {
        while (cidl > 0 && *apidl)
        {
            pdump (*apidl);
            SHELL32_GetItemAttributes (this, *apidl, rgfInOut);
            apidl++;
            cidl--;
        }
    }

    /* make sure SFGAO_VALIDATE is cleared, some apps depend on that */
    *rgfInOut &= ~SFGAO_VALIDATE;

    TRACE ("-- result=0x%08x\n", *rgfInOut);
    return hr;
}

/**************************************************************************
*    CFontsFolder::GetUIObjectOf
*
* PARAMETERS
*  hwndOwner [in]  Parent window for any output
*  cidl      [in]  array size
*  apidl     [in]  simple pidl array
*  riid      [in]  Requested Interface
*  prgfInOut [   ] reserved
*  ppvObject [out] Resulting Interface
*
*/
HRESULT WINAPI CFontsFolder::GetUIObjectOf(
    HWND hwndOwner,
    UINT cidl, LPCITEMIDLIST *apidl,
    REFIID riid, UINT *prgfInOut, LPVOID *ppvOut)
{
    LPITEMIDLIST pidl;
    CComPtr<IUnknown>                    pObj;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(%p,%u,apidl=%p,%s,%p,%p)\n", this,
           hwndOwner, cidl, apidl, shdebugstr_guid (&riid), prgfInOut, ppvOut);

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if (IsEqualIID (riid, IID_IContextMenu) && (cidl >= 1))
    {
        pObj = (IContextMenu *)this;
        this->apidl = apidl[0];
        hr = S_OK;
    }
    else if (IsEqualIID (riid, IID_IDataObject) && (cidl >= 1))
    {
        hr = IDataObject_Constructor (hwndOwner, pidlRoot, apidl, cidl, (IDataObject **)&pObj);
    }
    else if (IsEqualIID (riid, IID_IExtractIconA) && (cidl == 1))
    {
        pidl = ILCombine (pidlRoot, apidl[0]);
        pObj = (LPUNKNOWN) IExtractIconA_Constructor (pidl);
        SHFree (pidl);
        hr = S_OK;
    }
    else if (IsEqualIID (riid, IID_IExtractIconW) && (cidl == 1))
    {
        pidl = ILCombine (pidlRoot, apidl[0]);
        pObj = (LPUNKNOWN) IExtractIconW_Constructor (pidl);
        SHFree (pidl);
        hr = S_OK;
    }
    else if (IsEqualIID (riid, IID_IDropTarget) && (cidl >= 1))
    {
        hr = this->QueryInterface(IID_IDropTarget, (LPVOID *) & pObj);
    }
    else
        hr = E_NOINTERFACE;

    if (SUCCEEDED(hr) && !pObj)
        hr = E_OUTOFMEMORY;

    *ppvOut = pObj.Detach();
    TRACE ("(%p)->hr=0x%08x\n", this, hr);
    return hr;
}

/**************************************************************************
*    CFontsFolder::GetDisplayNameOf
*
*/
HRESULT WINAPI CFontsFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET strRet)
{
    PIDLFontStruct *pFont;

    TRACE("(%p)->(pidl=%p,0x%08x,%p)\n", this, pidl, dwFlags, strRet);
    pdump (pidl);

    if (!strRet)
        return E_INVALIDARG;

    pFont = _ILGetFontStruct(pidl);
    if (pFont)
    {
        strRet->pOleStr = (LPWSTR)CoTaskMemAlloc((wcslen(pFont->szName) + 1) * sizeof(WCHAR));
        if (!strRet->pOleStr)
            return E_OUTOFMEMORY;

        wcscpy(strRet->pOleStr, pFont->szName);
        strRet->uType = STRRET_WSTR;
    }
    else if (!pidl->mkid.cb)
    {
        WCHAR wszPath[MAX_PATH];

        if ((GET_SHGDN_RELATION (dwFlags) == SHGDN_NORMAL) &&
                (GET_SHGDN_FOR (dwFlags) & SHGDN_FORPARSING))
        {
            if (!SHGetSpecialFolderPathW(NULL, wszPath, CSIDL_FONTS, FALSE))
                return E_FAIL;
        }
        else if (!HCR_GetClassNameW(CLSID_FontsFolderShortcut, wszPath, MAX_PATH))
            return E_FAIL;

        strRet->pOleStr = (LPWSTR)CoTaskMemAlloc((wcslen(wszPath) + 1) * sizeof(WCHAR));
        if (!strRet->pOleStr)
            return E_OUTOFMEMORY;

        wcscpy(strRet->pOleStr, wszPath);
        strRet->uType = STRRET_WSTR;
    }
    else
        return E_INVALIDARG;

    return S_OK;
}

/**************************************************************************
*  CFontsFolder::SetNameOf
*  Changes the name of a file object or subfolder, possibly changing its item
*  identifier in the process.
*
* PARAMETERS
*  hwndOwner [in]  Owner window for output
*  pidl      [in]  simple pidl of item to change
*  lpszName  [in]  the items new display name
*  dwFlags   [in]  SHGNO formatting flags
*  ppidlOut  [out] simple pidl returned
*/
HRESULT WINAPI CFontsFolder::SetNameOf(HWND hwndOwner, LPCITEMIDLIST pidl,    /*simple pidl */
                                       LPCOLESTR lpName, DWORD dwFlags, LPITEMIDLIST * pPidlOut)
{
    FIXME ("(%p)->(%p,pidl=%p,%s,%u,%p)\n", this,
           hwndOwner, pidl, debugstr_w (lpName), dwFlags, pPidlOut);
    return E_FAIL;
}

HRESULT WINAPI CFontsFolder::GetDefaultSearchGUID(GUID *pguid)
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CFontsFolder::EnumSearches(IEnumExtraSearch **ppenum)
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CFontsFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    TRACE ("(%p)\n", this);

    if (pSort)
        *pSort = 0;
    if (pDisplay)
        *pDisplay = 0;

    return S_OK;
}

HRESULT WINAPI CFontsFolder::GetDefaultColumnState(UINT iColumn, DWORD *pcsFlags)
{
    TRACE ("(%p)\n", this);

    if (!pcsFlags || iColumn >= FontsSHELLVIEWCOLUMNS)
        return E_INVALIDARG;
    *pcsFlags = FontsSFHeader[iColumn].pcsFlags;
    return S_OK;
}

HRESULT WINAPI CFontsFolder::GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CFontsFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *psd)
{
    WCHAR buffer[MAX_PATH] = {0};
    HRESULT hr = E_FAIL;
    PIDLFontStruct * pfont;
    HANDLE hFile;
    LARGE_INTEGER FileSize;
    SHFILEINFOW fi;

    TRACE("(%p, %p, %d, %p)\n", this, pidl, iColumn, psd);

    if (iColumn >= FontsSHELLVIEWCOLUMNS)
        return E_FAIL;

    psd->fmt = FontsSFHeader[iColumn].fmt;
    psd->cxChar = FontsSFHeader[iColumn].cxChar;
    if (pidl == NULL)
    {
        psd->str.uType = STRRET_WSTR;
        if (LoadStringW(shell32_hInstance, FontsSFHeader[iColumn].colnameid, buffer, MAX_PATH))
            hr = SHStrDupW(buffer, &psd->str.pOleStr);

        return hr;
    }

    if (iColumn == COLUMN_NAME)
    {
        psd->str.uType = STRRET_WSTR;
        return GetDisplayNameOf(pidl, SHGDN_NORMAL, &psd->str);
    }

    psd->str.uType = STRRET_CSTR;
    psd->str.cStr[0] = '\0';

    switch(iColumn)
    {
        case COLUMN_TYPE:
            pfont = _ILGetFontStruct(pidl);
            if (pfont)
            {
                if (SHGetFileInfoW(pfont->szName + pfont->offsFile, 0, &fi, sizeof(fi), SHGFI_TYPENAME))
                {
                    psd->str.pOleStr = (LPWSTR)CoTaskMemAlloc((wcslen(fi.szTypeName) + 1) * sizeof(WCHAR));
                    if (!psd->str.pOleStr)
                        return E_OUTOFMEMORY;
                    wcscpy(psd->str.pOleStr, fi.szTypeName);
                    psd->str.uType = STRRET_WSTR;
                    return S_OK;
                }
            }
            break;
        case COLUMN_SIZE:
            pfont = _ILGetFontStruct(pidl);
            if (pfont)
            {
                hFile = CreateFileW(pfont->szName + pfont->offsFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    if (GetFileSizeEx(hFile, &FileSize))
                    {
                        if (StrFormatByteSizeW(FileSize.QuadPart, buffer, sizeof(buffer) / sizeof(WCHAR)))
                        {
                            psd->str.pOleStr = (LPWSTR)CoTaskMemAlloc(wcslen(buffer) + 1);
                            if (!psd->str.pOleStr)
                            {
                                CloseHandle(hFile);
                                return E_OUTOFMEMORY;
                            }
                            wcscpy(psd->str.pOleStr, buffer);
                            psd->str.uType = STRRET_WSTR;
                            CloseHandle(hFile);
                            return S_OK;
                        }
                    }
                    CloseHandle(hFile);
                }
            }
            break;
        case COLUMN_FILENAME:
            pfont = _ILGetFontStruct(pidl);
            if (pfont)
            {
                psd->str.pOleStr = (LPWSTR)CoTaskMemAlloc((wcslen(pfont->szName + pfont->offsFile) + 1) * sizeof(WCHAR));
                if (psd->str.pOleStr)
                {
                    psd->str.uType = STRRET_WSTR;
                    wcscpy(psd->str.pOleStr, pfont->szName + pfont->offsFile);
                    return S_OK;
                }
                else
                    return E_OUTOFMEMORY;
            }
            break;
    }

    return E_FAIL;
}

HRESULT WINAPI CFontsFolder::MapColumnToSCID(UINT column, SHCOLUMNID *pscid)
{
    FIXME ("(%p)\n", this);

    return E_NOTIMPL;
}

/************************************************************************
 *    CFontsFolder::GetClassID
 */
HRESULT WINAPI CFontsFolder::GetClassID(CLSID *lpClassId)
{
    TRACE ("(%p)\n", this);

    if (!lpClassId)
        return E_POINTER;

    *lpClassId = CLSID_FontsFolderShortcut;

    return S_OK;
}

/************************************************************************
 *    CFontsFolder::Initialize
 *
 * NOTES: it makes no sense to change the pidl
 */
HRESULT WINAPI CFontsFolder::Initialize(LPCITEMIDLIST pidl)
{
    TRACE ("(%p)->(%p)\n", this, pidl);

    return E_NOTIMPL;
}

/**************************************************************************
 *    CFontsFolder::GetCurFolder
 */
HRESULT WINAPI CFontsFolder::GetCurFolder(LPITEMIDLIST *pidl)
{
    TRACE ("(%p)->(%p)\n", this, pidl);

    if (!pidl)
        return E_POINTER;

    *pidl = ILClone(pidlRoot);

    return S_OK;
}

/**************************************************************************
* IContextMenu2 Implementation
*/

/**************************************************************************
* CFontsFolder::QueryContextMenu()
*/
HRESULT WINAPI CFontsFolder::QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    WCHAR szBuffer[30] = {0};
    ULONG Count = 1;

    TRACE("(%p)->(hmenu=%p indexmenu=%x cmdfirst=%x cmdlast=%x flags=%x )\n",
          this, hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    if (LoadStringW(shell32_hInstance, IDS_OPEN, szBuffer, sizeof(szBuffer) / sizeof(WCHAR)))
    {
        szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count, MFT_STRING, szBuffer, MFS_DEFAULT);
        Count++;
    }

    if (LoadStringW(shell32_hInstance, IDS_PRINT_VERB, szBuffer, sizeof(szBuffer) / sizeof(WCHAR)))
    {
        szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count++, MFT_STRING, szBuffer, MFS_ENABLED);
    }

    if (LoadStringW(shell32_hInstance, IDS_COPY, szBuffer, sizeof(szBuffer) / sizeof(WCHAR)))
    {
        szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count++, MFT_SEPARATOR, NULL, MFS_ENABLED);
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count++, MFT_STRING, szBuffer, MFS_ENABLED);
    }

    if (LoadStringW(shell32_hInstance, IDS_DELETE, szBuffer, sizeof(szBuffer) / sizeof(WCHAR)))
    {
        szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count++, MFT_SEPARATOR, NULL, MFS_ENABLED);
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count, MFT_STRING, szBuffer, MFS_ENABLED);
    }

    if (LoadStringW(shell32_hInstance, IDS_PROPERTIES, szBuffer, sizeof(szBuffer) / sizeof(WCHAR)))
    {
        szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count++, MFT_SEPARATOR, NULL, MFS_ENABLED);
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count, MFT_STRING, szBuffer, MFS_ENABLED);
    }

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, Count);
}

/**************************************************************************
* CFontsFolder::InvokeCommand()
*/
HRESULT WINAPI CFontsFolder::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
    SHELLEXECUTEINFOW sei;
    PIDLFontStruct * pfont;
    SHFILEOPSTRUCTW op;

    TRACE("(%p)->(invcom=%p verb=%p wnd=%p)\n", this, lpcmi, lpcmi->lpVerb, lpcmi->hwnd);

    if (lpcmi->lpVerb == MAKEINTRESOURCEA(1) || lpcmi->lpVerb == MAKEINTRESOURCEA(2) || lpcmi->lpVerb == MAKEINTRESOURCEA(7))
    {
        ZeroMemory(&sei, sizeof(sei));
        sei.cbSize = sizeof(sei);
        sei.hwnd = lpcmi->hwnd;
        sei.nShow = SW_SHOWNORMAL;
        if (lpcmi->lpVerb == MAKEINTRESOURCEA(1))
            sei.lpVerb = L"open";
        else if (lpcmi->lpVerb == MAKEINTRESOURCEA(2))
            sei.lpVerb = L"print";
        else if (lpcmi->lpVerb == MAKEINTRESOURCEA(7))
            sei.lpVerb = L"properties";

        pfont = _ILGetFontStruct(apidl);
        sei.lpFile = pfont->szName + pfont->offsFile;

        if (ShellExecuteExW(&sei) == FALSE)
            return E_FAIL;
    }
    else if (lpcmi->lpVerb == MAKEINTRESOURCEA(4))
    {
        FIXME("implement font copying\n");
        return E_NOTIMPL;
    }
    else if (lpcmi->lpVerb == MAKEINTRESOURCEA(6))
    {
        ZeroMemory(&op, sizeof(op));
        op.hwnd = lpcmi->hwnd;
        op.wFunc = FO_DELETE;
        op.fFlags = FOF_ALLOWUNDO;
        pfont = _ILGetFontStruct(apidl);
        op.pFrom = pfont->szName + pfont->offsFile;
        SHFileOperationW(&op);
    }

    return S_OK;
}

/**************************************************************************
 *  CFontsFolder::GetCommandString()
 *
 */
HRESULT WINAPI CFontsFolder::GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *lpReserved, LPSTR lpszName, UINT uMaxNameLen)
{
    TRACE("(%p)->(idcom=%lx flags=%x %p name=%p len=%x)\n", this, idCommand, uFlags, lpReserved, lpszName, uMaxNameLen);

    return E_FAIL;
}

/**************************************************************************
* CFontsFolder::HandleMenuMsg()
*/
HRESULT WINAPI CFontsFolder::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TRACE("(%p)->(msg=%x wp=%lx lp=%lx)\n", this, uMsg, wParam, lParam);

    return E_NOTIMPL;
}

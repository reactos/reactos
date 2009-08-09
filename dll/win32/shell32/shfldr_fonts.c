/*
 * Fonts folder
 *
 * Copyright 2008       Johannes Anderwald <janderwald@reactos.org>
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

/***********************************************************************
*   IShellFolder implementation
*/

typedef struct {
    const IShellFolder2Vtbl  *lpVtbl;
    LONG                       ref;
    const IContextMenu2Vtbl *lpVtblContextMenuFontItem;
    const IPersistFolder2Vtbl *lpVtblPersistFolder2;

    /* both paths are parsible from the desktop */
    LPITEMIDLIST pidlRoot;	/* absolute pidl */
    LPCITEMIDLIST apidl;    /* currently focused font item */
} IGenericSFImpl, *LPIGenericSFImpl;

static const IShellFolder2Vtbl vt_ShellFolder2;
static const IPersistFolder2Vtbl vt_NP_PersistFolder2;
static const IContextMenu2Vtbl vt_ContextMenu2FontItem;

#define _IPersistFolder2_Offset ((int)(&(((IGenericSFImpl*)0)->lpVtblPersistFolder2)))
#define _ICOM_THIS_From_IPersistFolder2(class, name) class* This = (class*)(((char*)name)-_IPersistFolder2_Offset);
#define _IContextMenuFontItem_Offset ((int)(&(((IGenericSFImpl*)0)->lpVtblContextMenuFontItem)))
#define _ICOM_THIS_From_IContextMenu2FontItem(class, name) class* This = (class*)(((char*)name)-_IContextMenuFontItem_Offset);

#define _IUnknown_(This)	(IUnknown*)&(This->lpVtbl)
#define _IShellFolder_(This)	(IShellFolder*)&(This->lpVtbl)
#define _IPersistFolder2_(This)	(IPersistFolder2*)&(This->lpVtblPersistFolder2)

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


static LPIGenericSFImpl __inline impl_from_IPersistFolder2( IPersistFolder2 *iface )
{
    return (LPIGenericSFImpl)((char*)iface - FIELD_OFFSET(IGenericSFImpl, lpVtblPersistFolder2));
}

static LPIGenericSFImpl __inline impl_from_IContextMenu2(IContextMenu2 *iface)
{
    return (LPIGenericSFImpl)((char *)iface - FIELD_OFFSET(IGenericSFImpl, lpVtblContextMenuFontItem));
}

/**************************************************************************
*	ISF_Fonts_Constructor
*/
HRESULT WINAPI ISF_Fonts_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv)
{
    IGenericSFImpl *sf;

    TRACE ("unkOut=%p %s\n", pUnkOuter, shdebugstr_guid (riid));

    if (!ppv)
        return E_POINTER;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    sf = (IGenericSFImpl *) HeapAlloc ( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof (IGenericSFImpl));
    if (!sf)
        return E_OUTOFMEMORY;

    sf->ref = 0;
    sf->lpVtbl = &vt_ShellFolder2;
    sf->lpVtblPersistFolder2 = &vt_NP_PersistFolder2;
    sf->lpVtblContextMenuFontItem = &vt_ContextMenu2FontItem;
    sf->pidlRoot = _ILCreateFont();	/* my qualified pidl */

    if (!SUCCEEDED (IUnknown_QueryInterface (_IUnknown_ (sf), riid, ppv)))
    {
        IUnknown_Release (_IUnknown_ (sf));
        return E_NOINTERFACE;
    }

    TRACE ("--(%p)\n", sf);
    return S_OK;
}

/**************************************************************************
 *	ISF_Fonts_fnQueryInterface
 *
 * NOTE
 *     supports not IPersist/IPersistFolder
 */
static HRESULT WINAPI ISF_Fonts_fnQueryInterface (IShellFolder2 *iface, REFIID riid, LPVOID *ppvObj)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    TRACE ("(%p)->(%s,%p)\n", This, shdebugstr_guid (riid), ppvObj);

    *ppvObj = NULL;

    if (IsEqualIID (riid, &IID_IUnknown) ||
        IsEqualIID (riid, &IID_IShellFolder) ||
        IsEqualIID (riid, &IID_IShellFolder2))
    {
        *ppvObj = This;
    }
    else if (IsEqualIID (riid, &IID_IPersist) ||
             IsEqualIID (riid, &IID_IPersistFolder) ||
             IsEqualIID (riid, &IID_IPersistFolder2))
    {
        *ppvObj = _IPersistFolder2_ (This);
    }

    if (*ppvObj)
    {
        IUnknown_AddRef ((IUnknown *) (*ppvObj));
        TRACE ("-- Interface: (%p)->(%p)\n", ppvObj, *ppvObj);
        return S_OK;
    }
    TRACE ("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ISF_Fonts_fnAddRef (IShellFolder2 * iface)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE ("(%p)->(count=%u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI ISF_Fonts_fnRelease (IShellFolder2 * iface)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE ("(%p)->(count=%u)\n", This, refCount + 1);

    if (!refCount) {
        TRACE ("-- destroying IShellFolder(%p)\n", This);
        SHFree (This->pidlRoot);
        HeapFree (GetProcessHeap(), 0, This);
    }
    return refCount;
}

/**************************************************************************
*	ISF_Fonts_fnParseDisplayName
*/
static HRESULT WINAPI ISF_Fonts_fnParseDisplayName (IShellFolder2 * iface,
               HWND hwndOwner, LPBC pbcReserved, LPOLESTR lpszDisplayName,
               DWORD * pchEaten, LPITEMIDLIST * ppidl, DWORD * pdwAttributes)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    HRESULT hr = E_UNEXPECTED;

    TRACE ("(%p)->(HWND=%p,%p,%p=%s,%p,pidl=%p,%p)\n", This,
            hwndOwner, pbcReserved, lpszDisplayName, debugstr_w (lpszDisplayName),
            pchEaten, ppidl, pdwAttributes);

    *ppidl = 0;
    if (pchEaten)
        *pchEaten = 0;		/* strange but like the original */

    TRACE ("(%p)->(-- ret=0x%08x)\n", This, hr);

    return hr;
}

static LPITEMIDLIST _ILCreateFontItem(LPWSTR pszFont, LPWSTR pszFile)
{
    PIDLDATA tmp;
    LPITEMIDLIST pidl;
    PIDLFontStruct * p;
    int size0 = (char*)&tmp.u.cfont.szName-(char*)&tmp.u.cfont;
    int size = size0;

    tmp.type = 0x00;
    tmp.u.cfont.dummy = 0xFF;
    tmp.u.cfont.offsFile = wcslen(pszFont) + 1;

    size += (tmp.u.cfont.offsFile + wcslen(pszFile) + 1) * sizeof(WCHAR);

    pidl = (LPITEMIDLIST)SHAlloc(size + 4);
    if (!pidl)
        return pidl;

    pidl->mkid.cb = size+2;
    memcpy(pidl->mkid.abID, &tmp, 2+size0);

    p = &((PIDLDATA*)pidl->mkid.abID)->u.cfont;
    wcscpy(p->szName, pszFont);
    wcscpy(p->szName + tmp.u.cfont.offsFile, pszFile);

    *(WORD*)((char*)pidl+(size+2)) = 0;
    return pidl;
}

static PIDLFontStruct * _ILGetFontStruct(LPCITEMIDLIST pidl)
{
    LPPIDLDATA pdata = _ILGetDataPointer(pidl);

    if (pdata && pdata->type==0x00)
        return (PIDLFontStruct*)&(pdata->u.cfont);

    return NULL;
}



/**************************************************************************
 *  CreateFontsEnumListss()
 */
static BOOL CreateFontsEnumList(IEnumIDList *list, DWORD dwFlags)
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
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts", 0, KEY_READ, &hKey)!= ERROR_SUCCESS)
            return FALSE;

        Length = pszPath - szPath;
        dwIndex = 0;
        do
        {
            dwName = sizeof(szName)/sizeof(WCHAR);
            dwFile = sizeof(szFile)/sizeof(WCHAR);
            ret = RegEnumValueW(hKey, dwIndex++, szName, &dwName, NULL, &dwType, (LPVOID)szFile, &dwFile);
            if (ret == ERROR_SUCCESS)
            {
                szFile[(sizeof(szFile)/sizeof(WCHAR))-1] = L'\0';
                if (dwType == REG_SZ && wcslen(szFile) + Length + 1< (sizeof(szPath)/sizeof(WCHAR)))
                {
                    wcscpy(&szPath[Length], szFile);
                    pidl = _ILCreateFontItem(szName, szPath);
                    TRACE("pidl %p name %s path %s\n", pidl, debugstr_w(szName), debugstr_w(szPath));
                    if (pidl)
                    {
                        if (!AddToEnumList(list, pidl))
                            SHFree(pidl);
                    }
                }
            }
        }while(ret != ERROR_NO_MORE_ITEMS);
        RegCloseKey(hKey);

    }
    return TRUE;
}

/**************************************************************************
*		ISF_Fonts_fnEnumObjects
*/
static HRESULT WINAPI ISF_Fonts_fnEnumObjects (IShellFolder2 * iface,
               HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST * ppEnumIDList)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    TRACE ("(%p)->(HWND=%p flags=0x%08x pplist=%p)\n", This,
            hwndOwner, dwFlags, ppEnumIDList);

    *ppEnumIDList = IEnumIDList_Constructor();
    if(*ppEnumIDList)
        CreateFontsEnumList(*ppEnumIDList, dwFlags);

    TRACE ("-- (%p)->(new ID List: %p)\n", This, *ppEnumIDList);

    return (*ppEnumIDList) ? S_OK : E_OUTOFMEMORY;
}

/**************************************************************************
*		ISF_Fonts_fnBindToObject
*/
static HRESULT WINAPI ISF_Fonts_fnBindToObject (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    TRACE ("(%p)->(pidl=%p,%p,%s,%p)\n", This,
            pidl, pbcReserved, shdebugstr_guid (riid), ppvOut);

    return SHELL32_BindToChild (This->pidlRoot, NULL, pidl, riid, ppvOut);
}

/**************************************************************************
*	ISF_Fonts_fnBindToStorage
*/
static HRESULT WINAPI ISF_Fonts_fnBindToStorage (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    FIXME ("(%p)->(pidl=%p,%p,%s,%p) stub\n", This,
            pidl, pbcReserved, shdebugstr_guid (riid), ppvOut);

    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
* 	ISF_Fonts_fnCompareIDs
*/

static HRESULT WINAPI ISF_Fonts_fnCompareIDs (IShellFolder2 * iface,
               LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    int nReturn;

    TRACE ("(%p)->(0x%08lx,pidl1=%p,pidl2=%p)\n", This, lParam, pidl1, pidl2);
    nReturn = SHELL32_CompareIDs (_IShellFolder_ (This), lParam, pidl1, pidl2);
    TRACE ("-- %i\n", nReturn);
    return nReturn;
}

/**************************************************************************
*	ISF_Fonts_fnCreateViewObject
*/
static HRESULT WINAPI ISF_Fonts_fnCreateViewObject (IShellFolder2 * iface,
               HWND hwndOwner, REFIID riid, LPVOID * ppvOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    LPSHELLVIEW pShellView;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(hwnd=%p,%s,%p)\n", This,
            hwndOwner, shdebugstr_guid (riid), ppvOut);

    if (!ppvOut)
        return hr;

	*ppvOut = NULL;

	if (IsEqualIID (riid, &IID_IDropTarget))
    {
	    WARN ("IDropTarget not implemented\n");
	    hr = E_NOTIMPL;
	}
    else if (IsEqualIID (riid, &IID_IContextMenu))
    {
	    WARN ("IContextMenu not implemented\n");
	    hr = E_NOTIMPL;
	}
    else if (IsEqualIID (riid, &IID_IShellView))
    {
	    pShellView = IShellView_Constructor ((IShellFolder *) iface);
	    if (pShellView)
        {
            hr = IShellView_QueryInterface (pShellView, riid, ppvOut);
            IShellView_Release (pShellView);
	    }
    }
    TRACE ("-- (%p)->(interface=%p)\n", This, ppvOut);
    return hr;
}

/**************************************************************************
*  ISF_Fonts_fnGetAttributesOf
*/
static HRESULT WINAPI ISF_Fonts_fnGetAttributesOf (IShellFolder2 * iface,
               UINT cidl, LPCITEMIDLIST * apidl, DWORD * rgfInOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    HRESULT hr = S_OK;

    TRACE ("(%p)->(cidl=%d apidl=%p mask=%p (0x%08x))\n", This,
            cidl, apidl, rgfInOut, rgfInOut ? *rgfInOut : 0);

    if (!rgfInOut)
        return E_INVALIDARG;
    if (cidl && !apidl)
        return E_INVALIDARG;

    if (*rgfInOut == 0)
        *rgfInOut = ~0;

    if (cidl == 0)
    {
        IShellFolder *psfParent = NULL;
        LPCITEMIDLIST rpidl = NULL;

        hr = SHBindToParent(This->pidlRoot, &IID_IShellFolder, (LPVOID*)&psfParent, (LPCITEMIDLIST*)&rpidl);
        if(SUCCEEDED(hr))
        {
            SHELL32_GetItemAttributes (psfParent, rpidl, rgfInOut);
            IShellFolder_Release(psfParent);
        }
    }
    else
    {
        while (cidl > 0 && *apidl)
        {
            pdump (*apidl);
            SHELL32_GetItemAttributes (_IShellFolder_ (This), *apidl, rgfInOut);
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
*	ISF_Fonts_fnGetUIObjectOf
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
static HRESULT WINAPI ISF_Fonts_fnGetUIObjectOf (IShellFolder2 * iface,
               HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl, REFIID riid,
               UINT * prgfInOut, LPVOID * ppvOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    LPITEMIDLIST pidl;
    IUnknown *pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(%p,%u,apidl=%p,%s,%p,%p)\n", This,
            hwndOwner, cidl, apidl, shdebugstr_guid (riid), prgfInOut, ppvOut);

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if (IsEqualIID (riid, &IID_IContextMenu) && (cidl >= 1))
    {
        pObj = (IUnknown*)(&This->lpVtblContextMenuFontItem);
        This->apidl = apidl[0];
        IUnknown_AddRef(pObj);
        hr = S_OK;
    }
    else if (IsEqualIID (riid, &IID_IDataObject) && (cidl >= 1))
    {
        pObj = (LPUNKNOWN) IDataObject_Constructor (hwndOwner, This->pidlRoot, apidl, cidl);
        hr = S_OK;
    }
    else if (IsEqualIID (riid, &IID_IExtractIconA) && (cidl == 1))
    {
        pidl = ILCombine (This->pidlRoot, apidl[0]);
        pObj = (LPUNKNOWN) IExtractIconA_Constructor (pidl);
        SHFree (pidl);
        hr = S_OK;
    }
    else if (IsEqualIID (riid, &IID_IExtractIconW) && (cidl == 1))
    {
        pidl = ILCombine (This->pidlRoot, apidl[0]);
        pObj = (LPUNKNOWN) IExtractIconW_Constructor (pidl);
        SHFree (pidl);
        hr = S_OK;
    }
    else if (IsEqualIID (riid, &IID_IDropTarget) && (cidl >= 1))
    {
        hr = IShellFolder_QueryInterface (iface, &IID_IDropTarget, (LPVOID *) & pObj);
    }
    else
        hr = E_NOINTERFACE;

    if (SUCCEEDED(hr) && !pObj)
        hr = E_OUTOFMEMORY;

    *ppvOut = pObj;
    TRACE ("(%p)->hr=0x%08x\n", This, hr);
    return hr;
}

/**************************************************************************
*	ISF_Fonts_fnGetDisplayNameOf
*
*/
static HRESULT WINAPI ISF_Fonts_fnGetDisplayNameOf (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET strRet)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    PIDLFontStruct * pfont;

    TRACE("ISF_Fonts_fnGetDisplayNameOf (%p)->(pidl=%p,0x%08x,%p)\n", This, pidl, dwFlags, strRet);
    pdump (pidl);

    if (!strRet)
        return E_INVALIDARG;

    pfont = _ILGetFontStruct(pidl);
    if (!pfont)
        return E_INVALIDARG;

    strRet->u.pOleStr = CoTaskMemAlloc((wcslen(pfont->szName)+1) * sizeof(WCHAR));
    if (!strRet->u.pOleStr)
        return E_OUTOFMEMORY;

    wcscpy(strRet->u.pOleStr, pfont->szName);
    strRet->uType = STRRET_WSTR;

    return S_OK;
}

/**************************************************************************
*  ISF_Fonts_fnSetNameOf
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
static HRESULT WINAPI ISF_Fonts_fnSetNameOf (IShellFolder2 * iface,
               HWND hwndOwner, LPCITEMIDLIST pidl,	/*simple pidl */
               LPCOLESTR lpName, DWORD dwFlags, LPITEMIDLIST * pPidlOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    FIXME ("(%p)->(%p,pidl=%p,%s,%u,%p)\n", This,
            hwndOwner, pidl, debugstr_w (lpName), dwFlags, pPidlOut);
    return E_FAIL;
}

static HRESULT WINAPI ISF_Fonts_fnGetDefaultSearchGUID (
               IShellFolder2 * iface, GUID * pguid)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_Fonts_fnEnumSearches (IShellFolder2 * iface,
               IEnumExtraSearch ** ppenum)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_Fonts_fnGetDefaultColumn (IShellFolder2 * iface,
               DWORD dwRes, ULONG * pSort, ULONG * pDisplay)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    TRACE ("(%p)\n", This);

    if (pSort)
        *pSort = 0;
    if (pDisplay)
        *pDisplay = 0;

    return S_OK;
}

static HRESULT WINAPI ISF_Fonts_fnGetDefaultColumnState (
               IShellFolder2 * iface, UINT iColumn, DWORD * pcsFlags)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    TRACE ("(%p)\n", This);

    if (!pcsFlags || iColumn >= FontsSHELLVIEWCOLUMNS)
        return E_INVALIDARG;
    *pcsFlags = FontsSFHeader[iColumn].pcsFlags;
    return S_OK;
}

static HRESULT WINAPI ISF_Fonts_fnGetDetailsEx (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, const SHCOLUMNID * pscid, VARIANT * pv)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_Fonts_fnGetDetailsOf (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS * psd)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    WCHAR buffer[MAX_PATH] = {0};
    HRESULT hr = E_FAIL;
    PIDLFontStruct * pfont;
    HANDLE hFile;
    LARGE_INTEGER FileSize;
    SHFILEINFOW fi;

    TRACE("(%p, %p, %d, %p)\n", This, pidl, iColumn, psd);

    if (iColumn >= FontsSHELLVIEWCOLUMNS)
        return E_FAIL;

    psd->fmt = FontsSFHeader[iColumn].fmt;
    psd->cxChar = FontsSFHeader[iColumn].cxChar;
    if (pidl == NULL)
    {
        psd->str.uType = STRRET_WSTR;
        if (LoadStringW(shell32_hInstance, FontsSFHeader[iColumn].colnameid, buffer, MAX_PATH))
            hr = SHStrDupW(buffer, &psd->str.u.pOleStr);

        return hr;
    }

    if (iColumn == COLUMN_NAME)
    {
        psd->str.uType = STRRET_WSTR;
        return IShellFolder2_GetDisplayNameOf(iface, pidl, SHGDN_NORMAL, &psd->str);
    }

    psd->str.uType = STRRET_CSTR;
    psd->str.u.cStr[0] = '\0';

    switch(iColumn)
    {
        case COLUMN_TYPE:
            pfont = _ILGetFontStruct(pidl);
            if (pfont)
            {
                if (SHGetFileInfoW(pfont->szName + pfont->offsFile, 0, &fi, sizeof(fi), SHGFI_TYPENAME))
                {
                    psd->str.u.pOleStr = CoTaskMemAlloc((wcslen(fi.szTypeName)+1) * sizeof(WCHAR));
                    if (!psd->str.u.pOleStr)
                        return E_OUTOFMEMORY;
                    wcscpy(psd->str.u.pOleStr, fi.szTypeName);
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
                        if (StrFormatByteSizeW(FileSize.QuadPart, buffer, sizeof(buffer)/sizeof(WCHAR)))
                        {
                            psd->str.u.pOleStr = CoTaskMemAlloc(wcslen(buffer) + 1);
                            if (!psd->str.u.pOleStr)
                                return E_OUTOFMEMORY;
                            wcscpy(psd->str.u.pOleStr, buffer);
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
                psd->str.u.pOleStr = CoTaskMemAlloc((wcslen(pfont->szName + pfont->offsFile) + 1) * sizeof(WCHAR));
                if (psd->str.u.pOleStr)
                {
                    psd->str.uType = STRRET_WSTR;
                    wcscpy(psd->str.u.pOleStr, pfont->szName + pfont->offsFile);
                    return S_OK;
                }
                else
                    return E_OUTOFMEMORY;
            }
            break;
     }

    return E_FAIL;
}

static HRESULT WINAPI ISF_Fonts_fnMapColumnToSCID (IShellFolder2 * iface,
               UINT column, SHCOLUMNID * pscid)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    FIXME ("(%p)\n", This);

    return E_NOTIMPL;
}

static const IShellFolder2Vtbl vt_ShellFolder2 = {
    ISF_Fonts_fnQueryInterface,
    ISF_Fonts_fnAddRef,
    ISF_Fonts_fnRelease,
    ISF_Fonts_fnParseDisplayName,
    ISF_Fonts_fnEnumObjects,
    ISF_Fonts_fnBindToObject,
    ISF_Fonts_fnBindToStorage,
    ISF_Fonts_fnCompareIDs,
    ISF_Fonts_fnCreateViewObject,
    ISF_Fonts_fnGetAttributesOf,
    ISF_Fonts_fnGetUIObjectOf,
    ISF_Fonts_fnGetDisplayNameOf,
    ISF_Fonts_fnSetNameOf,
    /* ShellFolder2 */
    ISF_Fonts_fnGetDefaultSearchGUID,
    ISF_Fonts_fnEnumSearches,
    ISF_Fonts_fnGetDefaultColumn,
    ISF_Fonts_fnGetDefaultColumnState,
    ISF_Fonts_fnGetDetailsEx,
    ISF_Fonts_fnGetDetailsOf,
    ISF_Fonts_fnMapColumnToSCID
};

/************************************************************************
 *	INPFldr_PersistFolder2_QueryInterface
 */
static HRESULT WINAPI INPFldr_PersistFolder2_QueryInterface (IPersistFolder2 * iface,
               REFIID iid, LPVOID * ppvObj)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)\n", This);

    return IUnknown_QueryInterface (_IUnknown_ (This), iid, ppvObj);
}

/************************************************************************
 *	INPFldr_PersistFolder2_AddRef
 */
static ULONG WINAPI INPFldr_PersistFolder2_AddRef (IPersistFolder2 * iface)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)->(count=%u)\n", This, This->ref);

    return IUnknown_AddRef (_IUnknown_ (This));
}

/************************************************************************
 *	ISFPersistFolder_Release
 */
static ULONG WINAPI INPFldr_PersistFolder2_Release (IPersistFolder2 * iface)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)->(count=%u)\n", This, This->ref);

    return IUnknown_Release (_IUnknown_ (This));
}

/************************************************************************
 *	INPFldr_PersistFolder2_GetClassID
 */
static HRESULT WINAPI INPFldr_PersistFolder2_GetClassID (
               IPersistFolder2 * iface, CLSID * lpClassId)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)\n", This);

    if (!lpClassId)
        return E_POINTER;

    *lpClassId = CLSID_FontsFolderShortcut;

    return S_OK;
}

/************************************************************************
 *	INPFldr_PersistFolder2_Initialize
 *
 * NOTES: it makes no sense to change the pidl
 */
static HRESULT WINAPI INPFldr_PersistFolder2_Initialize (
               IPersistFolder2 * iface, LPCITEMIDLIST pidl)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)->(%p)\n", This, pidl);

    return E_NOTIMPL;
}

/**************************************************************************
 *	IPersistFolder2_fnGetCurFolder
 */
static HRESULT WINAPI INPFldr_PersistFolder2_GetCurFolder (
               IPersistFolder2 * iface, LPITEMIDLIST * pidl)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)->(%p)\n", This, pidl);

    if (!pidl)
        return E_POINTER;

    *pidl = ILClone (This->pidlRoot);

    return S_OK;
}

static const IPersistFolder2Vtbl vt_NP_PersistFolder2 =
{
    INPFldr_PersistFolder2_QueryInterface,
    INPFldr_PersistFolder2_AddRef,
    INPFldr_PersistFolder2_Release,
    INPFldr_PersistFolder2_GetClassID,
    INPFldr_PersistFolder2_Initialize,
    INPFldr_PersistFolder2_GetCurFolder
};

/**************************************************************************
* IContextMenu2 Implementation
*/

/************************************************************************
 * ISF_Fonts_IContextMenu_QueryInterface
 */
static HRESULT WINAPI ISF_Fonts_IContextMenu2_QueryInterface(IContextMenu2 * iface, REFIID iid, LPVOID * ppvObject)
{
    _ICOM_THIS_From_IContextMenu2FontItem(IGenericSFImpl, iface);

    TRACE("(%p)\n", This);

    return IUnknown_QueryInterface(_IUnknown_(This), iid, ppvObject);
}

/************************************************************************
 * ISF_Fonts_IContextMenu_AddRef
 */
static ULONG WINAPI ISF_Fonts_IContextMenu2_AddRef(IContextMenu2 * iface)
{
    _ICOM_THIS_From_IContextMenu2FontItem(IGenericSFImpl, iface);

    TRACE("(%p)->(count=%u)\n", This, This->ref);

    return IUnknown_AddRef(_IUnknown_(This));
}

/************************************************************************
 * ISF_Fonts_IContextMenu_Release
 */
static ULONG WINAPI ISF_Fonts_IContextMenu2_Release(IContextMenu2  * iface)
{
    _ICOM_THIS_From_IContextMenu2FontItem(IGenericSFImpl, iface);

    TRACE("(%p)->(count=%u)\n", This, This->ref);

    return IUnknown_Release(_IUnknown_(This));
}

/**************************************************************************
* ISF_Fonts_IContextMenu_QueryContextMenu()
*/
static HRESULT WINAPI ISF_Fonts_IContextMenu2_QueryContextMenu(
	IContextMenu2 *iface,
	HMENU hMenu,
	UINT indexMenu,
	UINT idCmdFirst,
	UINT idCmdLast,
	UINT uFlags)
{
    WCHAR szBuffer[30] = {0};
    ULONG Count = 1;

    _ICOM_THIS_From_IContextMenu2FontItem(IGenericSFImpl, iface);

    TRACE("(%p)->(hmenu=%p indexmenu=%x cmdfirst=%x cmdlast=%x flags=%x )\n",
          This, hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    if (LoadStringW(shell32_hInstance, IDS_OPEN, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
    {
        szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count, MFT_STRING, szBuffer, MFS_DEFAULT);
        Count++;
    }

    if (LoadStringW(shell32_hInstance, IDS_PRINT_VERB, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
    {
        szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count++, MFT_STRING, szBuffer, MFS_ENABLED);
    }

    if (LoadStringW(shell32_hInstance, IDS_COPY, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
    {
        szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count++, MFT_SEPARATOR, NULL, MFS_ENABLED);
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count++, MFT_STRING, szBuffer, MFS_ENABLED);
    }

    if (LoadStringW(shell32_hInstance, IDS_DELETE, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
    {
        szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count++, MFT_SEPARATOR, NULL, MFS_ENABLED);
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count, MFT_STRING, szBuffer, MFS_ENABLED);
    }

    if (LoadStringW(shell32_hInstance, IDS_PROPERTIES, szBuffer, sizeof(szBuffer)/sizeof(WCHAR)))
    {
        szBuffer[(sizeof(szBuffer)/sizeof(WCHAR))-1] = L'\0';
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count++, MFT_SEPARATOR, NULL, MFS_ENABLED);
        _InsertMenuItemW(hMenu, indexMenu++, TRUE, idCmdFirst + Count, MFT_STRING, szBuffer, MFS_ENABLED);
    }

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, Count);
}


/**************************************************************************
* ISF_Fonts_IContextMenu_InvokeCommand()
*/
static HRESULT WINAPI ISF_Fonts_IContextMenu2_InvokeCommand(
	IContextMenu2 *iface,
	LPCMINVOKECOMMANDINFO lpcmi)
{
    SHELLEXECUTEINFOW sei;
    PIDLFontStruct * pfont;
    SHFILEOPSTRUCTW op;
    IGenericSFImpl * This = impl_from_IContextMenu2(iface);


    TRACE("(%p)->(invcom=%p verb=%p wnd=%p)\n",This,lpcmi,lpcmi->lpVerb, lpcmi->hwnd);

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

        pfont = _ILGetFontStruct(This->apidl);
        sei.lpFile = pfont->szName + pfont->offsFile;

        ShellExecuteExW(&sei);
        if (sei.hInstApp <= (HINSTANCE)32)
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
       pfont = _ILGetFontStruct(This->apidl);
       op.pFrom = pfont->szName + pfont->offsFile;
       SHFileOperationW(&op);
    }

    return S_OK;
}

/**************************************************************************
 *  ISF_Fonts_IContextMenu_GetCommandString()
 *
 */
static HRESULT WINAPI ISF_Fonts_IContextMenu2_GetCommandString(
	IContextMenu2 *iface,
	UINT_PTR idCommand,
	UINT uFlags,
	UINT* lpReserved,
	LPSTR lpszName,
	UINT uMaxNameLen)
{
    _ICOM_THIS_From_IContextMenu2FontItem(IGenericSFImpl, iface);

	TRACE("(%p)->(idcom=%lx flags=%x %p name=%p len=%x)\n",This, idCommand, uFlags, lpReserved, lpszName, uMaxNameLen);


	return E_FAIL;
}



/**************************************************************************
* ISF_Fonts_IContextMenu_HandleMenuMsg()
*/
static HRESULT WINAPI ISF_Fonts_IContextMenu2_HandleMenuMsg(
	IContextMenu2 *iface,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
    _ICOM_THIS_From_IContextMenu2FontItem(IGenericSFImpl, iface);

    TRACE("ISF_Fonts_IContextMenu_HandleMenuMsg (%p)->(msg=%x wp=%lx lp=%lx)\n",This, uMsg, wParam, lParam);

    return E_NOTIMPL;
}

static const IContextMenu2Vtbl vt_ContextMenu2FontItem =
{
	ISF_Fonts_IContextMenu2_QueryInterface,
	ISF_Fonts_IContextMenu2_AddRef,
	ISF_Fonts_IContextMenu2_Release,
	ISF_Fonts_IContextMenu2_QueryContextMenu,
	ISF_Fonts_IContextMenu2_InvokeCommand,
	ISF_Fonts_IContextMenu2_GetCommandString,
	ISF_Fonts_IContextMenu2_HandleMenuMsg
};







/*
 *    Virtual Workplace folder
 *
 *    Copyright 1997            Marcus Meissner
 *    Copyright 1998, 1999, 2002    Juergen Schmied
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

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"

#include "wingdi.h"
#include "pidl.h"
#include "shlguid.h"
#include "enumidlist.h"
#include "undocshell.h"
#include "shell32_main.h"
#include "shresdef.h"
#include "shlwapi.h"
#include "wine/debug.h"
#include "debughlp.h"
#include "shfldr.h"

WINE_DEFAULT_DEBUG_CHANNEL (shell);

/***********************************************************************
*   IShellFolder implementation
*/

typedef struct {
    const IShellFolder2Vtbl   *lpVtbl;
    LONG                ref;
    const IPersistFolder2Vtbl *lpVtblPersistFolder2;

    /* both paths are parsible from the desktop */
    LPITEMIDLIST pidlRoot;    /* absolute pidl */
} IGenericSFImpl;

static const IShellFolder2Vtbl vt_ShellFolder2;
static const IPersistFolder2Vtbl vt_PersistFolder2;

static inline IGenericSFImpl *impl_from_IPersistFolder2( IPersistFolder2 *iface )
{
    return (IGenericSFImpl *)((char*)iface - FIELD_OFFSET(IGenericSFImpl, lpVtblPersistFolder2));
}


/*
  converts This to an interface pointer
*/
#define _IUnknown_(This)    (IUnknown*)&(This->lpVtbl)
#define _IShellFolder_(This)    (IShellFolder*)&(This->lpVtbl)
#define _IShellFolder2_(This)    (IShellFolder2*)&(This->lpVtbl)

#define _IPersist_(This)    (IPersist*)&(This->lpVtblPersistFolder2)
#define _IPersistFolder_(This)    (IPersistFolder*)&(This->lpVtblPersistFolder2)
#define _IPersistFolder2_(This)    (IPersistFolder2*)&(This->lpVtblPersistFolder2)

/***********************************************************************
*   IShellFolder [MyComputer] implementation
*/

static const shvheader MyComputerSFHeader[] = {
    {IDS_SHV_COLUMN1, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},
    {IDS_SHV_COLUMN3, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN6, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN7, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
};

#define MYCOMPUTERSHELLVIEWCOLUMNS 4

/**************************************************************************
*    ISF_MyComputer_Constructor
*/
HRESULT WINAPI ISF_MyComputer_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv)
{
    IGenericSFImpl *sf;

    TRACE ("unkOut=%p %s\n", pUnkOuter, shdebugstr_guid (riid));

    if (!ppv)
        return E_POINTER;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    sf = LocalAlloc (LMEM_ZEROINIT, sizeof (IGenericSFImpl));
    if (!sf)
        return E_OUTOFMEMORY;

    sf->ref = 0;
    sf->lpVtbl = &vt_ShellFolder2;
    sf->lpVtblPersistFolder2 = &vt_PersistFolder2;
    sf->pidlRoot = _ILCreateMyComputer ();    /* my qualified pidl */

    if (!SUCCEEDED (IUnknown_QueryInterface (_IUnknown_ (sf), riid, ppv)))
    {
        IUnknown_Release (_IUnknown_ (sf));
        return E_NOINTERFACE;
    }

    TRACE ("--(%p)\n", sf);
    return S_OK;
}

/**************************************************************************
 *    ISF_MyComputer_fnQueryInterface
 *
 * NOTES supports not IPersist/IPersistFolder
 */
static HRESULT WINAPI ISF_MyComputer_fnQueryInterface (IShellFolder2 *iface,
               REFIID riid, LPVOID *ppvObj)
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

static ULONG WINAPI ISF_MyComputer_fnAddRef (IShellFolder2 * iface)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE ("(%p)->(count=%u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI ISF_MyComputer_fnRelease (IShellFolder2 * iface)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE ("(%p)->(count=%u)\n", This, refCount + 1);

    if (!refCount)
    {
        TRACE ("-- destroying IShellFolder(%p)\n", This);
        SHFree (This->pidlRoot);
        LocalFree ((HLOCAL) This);
    }
    return refCount;
}

/**************************************************************************
*    ISF_MyComputer_fnParseDisplayName
*/
static HRESULT WINAPI ISF_MyComputer_fnParseDisplayName (IShellFolder2 *iface,
               HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName,
               DWORD * pchEaten, LPITEMIDLIST * ppidl, DWORD * pdwAttributes)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    HRESULT hr = E_INVALIDARG;
    LPCWSTR szNext = NULL;
    WCHAR szElement[MAX_PATH];
    LPITEMIDLIST pidlTemp = NULL;
    CLSID clsid;

    TRACE("(%p)->(HWND=%p,%p,%p=%s,%p,pidl=%p,%p)\n", This,
          hwndOwner, pbc, lpszDisplayName, debugstr_w (lpszDisplayName),
          pchEaten, ppidl, pdwAttributes);

    *ppidl = 0;
    if (pchEaten)
        *pchEaten = 0;        /* strange but like the original */

    /* handle CLSID paths */
    if (lpszDisplayName[0] == ':' && lpszDisplayName[1] == ':')
    {
        szNext = GetNextElementW (lpszDisplayName, szElement, MAX_PATH);
        TRACE ("-- element: %s\n", debugstr_w (szElement));
        SHCLSIDFromStringW (szElement + 2, &clsid);
        pidlTemp = _ILCreateGuid (PT_GUID, &clsid);
    }
    /* do we have an absolute path name ? */
    else if (PathGetDriveNumberW (lpszDisplayName) >= 0 &&
              lpszDisplayName[2] == (WCHAR) '\\')
    {
        szNext = GetNextElementW (lpszDisplayName, szElement, MAX_PATH);
        /* make drive letter uppercase to enable PIDL comparison */
        szElement[0] = toupper(szElement[0]);
        pidlTemp = _ILCreateDrive (szElement);
    }

    if (szNext && *szNext)
    {
        hr = SHELL32_ParseNextElement (iface, hwndOwner, pbc, &pidlTemp,
                              (LPOLESTR) szNext, pchEaten, pdwAttributes);
    }
    else
    {
        if (pdwAttributes && *pdwAttributes)
            SHELL32_GetItemAttributes (_IShellFolder_ (This),
                                       pidlTemp, pdwAttributes);
        hr = S_OK;
    }

    *ppidl = pidlTemp;

    TRACE ("(%p)->(-- ret=0x%08x)\n", This, hr);

    return hr;
}

/**************************************************************************
 *  CreateMyCompEnumList()
 */
static const WCHAR MyComputer_NameSpaceW[] = { 'S','O','F','T','W','A','R','E',
 '\\','M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\',
 'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\','E','x','p','l',
 'o','r','e','r','\\','M','y','C','o','m','p','u','t','e','r','\\','N','a','m',
 'e','s','p','a','c','e','\0' };

static BOOL CreateMyCompEnumList(IEnumIDList *list, DWORD dwFlags)
{
    BOOL ret = TRUE;

    TRACE("(%p)->(flags=0x%08x)\n", list, dwFlags);

    /* enumerate the folders */
    if (dwFlags & SHCONTF_FOLDERS)
    {
        WCHAR wszDriveName[] = {'A', ':', '\\', '\0'};
        DWORD dwDrivemap = GetLogicalDrives();
        HKEY hkey;
        UINT i;

        while (ret && wszDriveName[0]<='Z')
        {
            if(dwDrivemap & 0x00000001L)
                ret = AddToEnumList(list, _ILCreateDrive(wszDriveName));
            wszDriveName[0]++;
            dwDrivemap = dwDrivemap >> 1;
        }

        TRACE("-- (%p)-> enumerate (mycomputer shell extensions)\n",list);
        for (i=0; i<2; i++) {
            if (ret && !RegOpenKeyExW(i == 0 ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER,
                                      MyComputer_NameSpaceW, 0, KEY_READ, &hkey))
            {
                WCHAR iid[50];
                int i=0;

                while (ret)
                {
                    DWORD size;
                    LONG r;

                    size = sizeof(iid) / sizeof(iid[0]);
                    r = RegEnumKeyExW(hkey, i, iid, &size, 0, NULL, NULL, NULL);
                    if (ERROR_SUCCESS == r)
                    {
                        /* FIXME: shell extensions, shouldn't the type be
                         * PT_SHELLEXT? */
                        ret = AddToEnumList(list, _ILCreateGuidFromStrW(iid));
                        i++;
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
    return ret;
}

/**************************************************************************
*        ISF_MyComputer_fnEnumObjects
*/
static HRESULT WINAPI ISF_MyComputer_fnEnumObjects (IShellFolder2 *iface,
               HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    TRACE("(%p)->(HWND=%p flags=0x%08x pplist=%p)\n", This,
          hwndOwner, dwFlags, ppEnumIDList);

    *ppEnumIDList = IEnumIDList_Constructor();
    if (*ppEnumIDList)
        CreateMyCompEnumList(*ppEnumIDList, dwFlags);

    TRACE ("-- (%p)->(new ID List: %p)\n", This, *ppEnumIDList);

    return (*ppEnumIDList) ? S_OK : E_OUTOFMEMORY;
}

/**************************************************************************
*        ISF_MyComputer_fnBindToObject
*/
static HRESULT WINAPI ISF_MyComputer_fnBindToObject (IShellFolder2 *iface,
               LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    TRACE("(%p)->(pidl=%p,%p,%s,%p)\n", This,
          pidl, pbcReserved, shdebugstr_guid (riid), ppvOut);

    return SHELL32_BindToChild (This->pidlRoot, NULL, pidl, riid, ppvOut);
}

/**************************************************************************
*    ISF_MyComputer_fnBindToStorage
*/
static HRESULT WINAPI ISF_MyComputer_fnBindToStorage (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    FIXME("(%p)->(pidl=%p,%p,%s,%p) stub\n", This,
          pidl, pbcReserved, shdebugstr_guid (riid), ppvOut);

    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
*     ISF_MyComputer_fnCompareIDs
*/

static HRESULT WINAPI ISF_MyComputer_fnCompareIDs (IShellFolder2 *iface,
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
*    ISF_MyComputer_fnCreateViewObject
*/
static HRESULT WINAPI ISF_MyComputer_fnCreateViewObject (IShellFolder2 *iface,
               HWND hwndOwner, REFIID riid, LPVOID * ppvOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    LPSHELLVIEW pShellView;
    HRESULT hr = E_INVALIDARG;

    TRACE("(%p)->(hwnd=%p,%s,%p)\n", This,
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
*  ISF_MyComputer_fnGetAttributesOf
*/
static HRESULT WINAPI ISF_MyComputer_fnGetAttributesOf (IShellFolder2 * iface,
                UINT cidl, LPCITEMIDLIST * apidl, DWORD * rgfInOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    HRESULT hr = S_OK;

    TRACE ("(%p)->(cidl=%d apidl=%p mask=%p (0x%08x))\n",
           This, cidl, apidl, rgfInOut, rgfInOut ? *rgfInOut : 0);

    if (!rgfInOut)
        return E_INVALIDARG;
    if (cidl && !apidl)
        return E_INVALIDARG;

    if (*rgfInOut == 0)
        *rgfInOut = ~0;

    if(cidl == 0){
        IShellFolder *psfParent = NULL;
        LPCITEMIDLIST rpidl = NULL;

        hr = SHBindToParent(This->pidlRoot, &IID_IShellFolder, (LPVOID*)&psfParent, (LPCITEMIDLIST*)&rpidl);
        if(SUCCEEDED(hr)) {
            SHELL32_GetItemAttributes (psfParent, rpidl, rgfInOut);
            IShellFolder_Release(psfParent);
        }
    } else {
        while (cidl > 0 && *apidl) {
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
*    ISF_MyComputer_fnGetUIObjectOf
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
static HRESULT WINAPI ISF_MyComputer_fnGetUIObjectOf (IShellFolder2 * iface,
                HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl, REFIID riid,
                UINT * prgfInOut, LPVOID * ppvOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    LPITEMIDLIST pidl;
    IUnknown *pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE("(%p)->(%p,%u,apidl=%p,%s,%p,%p)\n", This,
          hwndOwner, cidl, apidl, shdebugstr_guid (riid), prgfInOut, ppvOut);

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if (IsEqualIID (riid, &IID_IContextMenu) && (cidl >= 1))
    {
        pObj = (LPUNKNOWN) ISvItemCm_Constructor ((IShellFolder *) iface,
                                              This->pidlRoot, apidl, cidl);
        hr = S_OK;
    }
    else if (IsEqualIID (riid, &IID_IDataObject) && (cidl >= 1))
    {
        pObj = (LPUNKNOWN) IDataObject_Constructor (hwndOwner,
                                              This->pidlRoot, apidl, cidl);
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
        hr = IShellFolder_QueryInterface (iface, &IID_IDropTarget,
                                          (LPVOID *) &pObj);
    }
    else if ((IsEqualIID(riid,&IID_IShellLinkW) ||
              IsEqualIID(riid,&IID_IShellLinkA)) && (cidl == 1))
    {
        pidl = ILCombine (This->pidlRoot, apidl[0]);
        hr = IShellLink_ConstructFromFile(NULL, riid, pidl, (LPVOID*) &pObj);
        SHFree (pidl);
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
*    ISF_MyComputer_fnGetDisplayNameOf
*/
static HRESULT WINAPI ISF_MyComputer_fnGetDisplayNameOf (IShellFolder2 *iface,
               LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET strRet)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    LPWSTR pszPath;
    HRESULT hr = S_OK;

    TRACE ("(%p)->(pidl=%p,0x%08x,%p)\n", This, pidl, dwFlags, strRet);
    pdump (pidl);

    if (!strRet)
        return E_INVALIDARG;

    pszPath = CoTaskMemAlloc((MAX_PATH +1) * sizeof(WCHAR));
    if (!pszPath)
        return E_OUTOFMEMORY;

    pszPath[0] = 0;

    if (!pidl->mkid.cb)
    {
        /* parsing name like ::{...} */
        pszPath[0] = ':';
        pszPath[1] = ':';
        SHELL32_GUIDToStringW(&CLSID_MyComputer, &pszPath[2]);
    }
    else if (_ILIsPidlSimple(pidl))
    {
        /* take names of special folders only if its only this folder */
        if (_ILIsSpecialFolder(pidl))
        {
            GUID const *clsid;

            clsid = _ILGetGUIDPointer (pidl);
            if (clsid)
            {
                if (GET_SHGDN_FOR (dwFlags) & SHGDN_FORPARSING)
                {
                    static const WCHAR clsidW[] =
                     { 'C','L','S','I','D','\\',0 };
                    static const WCHAR shellfolderW[] =
                     { '\\','s','h','e','l','l','f','o','l','d','e','r',0 };
                    static const WCHAR wantsForParsingW[] =
                     { 'W','a','n','t','s','F','o','r','P','a','r','s','i','n',
                     'g',0 };
                    int bWantsForParsing = FALSE;
                    WCHAR szRegPath[100];
                    LONG r;

                    /*
                     * We can only get a filesystem path from a shellfolder
                     * if the value WantsFORPARSING exists in
                     *      CLSID\\{...}\\shellfolder
                     * exception: the MyComputer folder has this keys not
                     *            but like any filesystem backed
                     *            folder it needs these behaviour
                     *
                     * Get the "WantsFORPARSING" flag from the registry
                     */

                    lstrcpyW (szRegPath, clsidW);
                    SHELL32_GUIDToStringW (clsid, &szRegPath[6]);
                    lstrcatW (szRegPath, shellfolderW);
                    r = SHGetValueW (HKEY_CLASSES_ROOT, szRegPath,
                                     wantsForParsingW, NULL, NULL, NULL);
                    if (r == ERROR_SUCCESS)
                        bWantsForParsing = TRUE;

                    if ((GET_SHGDN_RELATION (dwFlags) == SHGDN_NORMAL) &&
                        bWantsForParsing)
                    {
                        /*
                         * We need the filesystem path to the destination folder
                         * Only the folder itself can know it
                         */
                        hr = SHELL32_GetDisplayNameOfChild (iface, pidl,
                                                dwFlags, pszPath, MAX_PATH);
                    }
                    else
                    {
                        LPWSTR p = pszPath;

                        /* parsing name like ::{...} */
                        p[0] = ':';
                        p[1] = ':';
                        p += 2;
                        p += SHELL32_GUIDToStringW(&CLSID_MyComputer, p);

                        /* \:: */
                        p[0] = '\\';
                        p[1] = ':';
                        p[2] = ':';
                        p += 3;
                        SHELL32_GUIDToStringW(clsid, p);
                    }
                }
                else
                {
                    /* user friendly name */
                    HCR_GetClassNameW (clsid, pszPath, MAX_PATH);
                }
            }
            else
            {
                /* append my own path */
                _ILSimpleGetTextW (pidl, pszPath, MAX_PATH);
            }
        }
        else if (_ILIsDrive(pidl))
        {
            _ILSimpleGetTextW (pidl, pszPath, MAX_PATH);    /* append my own path */

            /* long view "lw_name (C:)" */
            if (!(dwFlags & SHGDN_FORPARSING))
            {
                DWORD dwVolumeSerialNumber, dwMaximumComponetLength, dwFileSystemFlags;
                WCHAR wszDrive[18] = {0};
                static const WCHAR wszOpenBracket[] = {' ','(',0};
                static const WCHAR wszCloseBracket[] = {')',0};

                GetVolumeInformationW (pszPath, wszDrive,
                           sizeof(wszDrive)/sizeof(wszDrive[0]) - 6,
                           &dwVolumeSerialNumber,
                           &dwMaximumComponetLength, &dwFileSystemFlags, NULL, 0);
                strcatW (wszDrive, wszOpenBracket);
                lstrcpynW (wszDrive + strlenW(wszDrive), pszPath, 3);
                strcatW (wszDrive, wszCloseBracket);
                strcpyW (pszPath, wszDrive);
            }
        }
        else
        {
            /* Neither a shell namespace extension nor a drive letter. */
            ERR("Wrong pidl type\n");
            return E_INVALIDARG;
        }
    }
    else
    {
        /* Complex pidl. Let the child folder do the work */
        hr = SHELL32_GetDisplayNameOfChild(iface, pidl, dwFlags, pszPath, MAX_PATH);
    }

    if (SUCCEEDED (hr))
    {
        strRet->uType = STRRET_WSTR;
        strRet->u.pOleStr = pszPath;
    }
    else
        CoTaskMemFree(pszPath);

    TRACE ("-- (%p)->(%s)\n", This, strRet->uType == STRRET_CSTR ? strRet->u.cStr : debugstr_w(strRet->u.pOleStr));
    return hr;
}

/**************************************************************************
*  ISF_MyComputer_fnSetNameOf
*  Changes the name of a file object or subfolder, possibly changing its item
*  identifier in the process.
*
* PARAMETERS
*  hwndOwner  [in]   Owner window for output
*  pidl       [in]   simple pidl of item to change
*  lpszName   [in]   the items new display name
*  dwFlags    [in]   SHGNO formatting flags
*  ppidlOut   [out]  simple pidl returned
*/
static HRESULT WINAPI ISF_MyComputer_fnSetNameOf (
               IShellFolder2 * iface, HWND hwndOwner, LPCITEMIDLIST pidl,
               LPCOLESTR lpName, DWORD dwFlags, LPITEMIDLIST * pPidlOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    FIXME ("(%p)->(%p,pidl=%p,%s,%u,%p)\n", This,
           hwndOwner, pidl, debugstr_w (lpName), dwFlags, pPidlOut);
    return E_FAIL;
}

static HRESULT WINAPI ISF_MyComputer_fnGetDefaultSearchGUID (
               IShellFolder2 * iface, GUID * pguid)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}
static HRESULT WINAPI ISF_MyComputer_fnEnumSearches (
               IShellFolder2 * iface, IEnumExtraSearch ** ppenum)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}
static HRESULT WINAPI ISF_MyComputer_fnGetDefaultColumn (
               IShellFolder2 *iface, DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    TRACE ("(%p)\n", This);

    if (pSort)
         *pSort = 0;
    if (pDisplay)
        *pDisplay = 0;
    return S_OK;
}
static HRESULT WINAPI ISF_MyComputer_fnGetDefaultColumnState (
               IShellFolder2 * iface, UINT iColumn, DWORD * pcsFlags)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    TRACE ("(%p)\n", This);

    if (!pcsFlags || iColumn >= MYCOMPUTERSHELLVIEWCOLUMNS)
        return E_INVALIDARG;
    *pcsFlags = MyComputerSFHeader[iColumn].pcsFlags;
    return S_OK;
}

static HRESULT WINAPI ISF_MyComputer_fnGetDetailsEx (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, const SHCOLUMNID * pscid, VARIANT * pv)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}

/* FIXME: drive size >4GB is rolling over */
static HRESULT WINAPI ISF_MyComputer_fnGetDetailsOf (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS * psd)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    HRESULT hr;

    TRACE ("(%p)->(%p %i %p)\n", This, pidl, iColumn, psd);

    if (!psd || iColumn >= MYCOMPUTERSHELLVIEWCOLUMNS)
        return E_INVALIDARG;

    if (!pidl)
    {
        psd->fmt = MyComputerSFHeader[iColumn].fmt;
        psd->cxChar = MyComputerSFHeader[iColumn].cxChar;
        psd->str.uType = STRRET_CSTR;
        LoadStringA (shell32_hInstance, MyComputerSFHeader[iColumn].colnameid,
                     psd->str.u.cStr, MAX_PATH);
        return S_OK;
    }
    else
    {
        char szPath[MAX_PATH];
        ULARGE_INTEGER ulBytes;

        psd->str.u.cStr[0] = 0x00;
        psd->str.uType = STRRET_CSTR;
        switch (iColumn)
        {
        case 0:        /* name */
            hr = IShellFolder_GetDisplayNameOf (iface, pidl,
                       SHGDN_NORMAL | SHGDN_INFOLDER, &psd->str);
            break;
        case 1:        /* type */
            _ILGetFileType (pidl, psd->str.u.cStr, MAX_PATH);
            break;
        case 2:        /* total size */
            if (_ILIsDrive (pidl))
            {
                _ILSimpleGetText (pidl, szPath, MAX_PATH);
                GetDiskFreeSpaceExA (szPath, NULL, &ulBytes, NULL);
                StrFormatByteSizeA (ulBytes.u.LowPart, psd->str.u.cStr, MAX_PATH);
            }
            break;
        case 3:        /* free size */
            if (_ILIsDrive (pidl))
            {
                _ILSimpleGetText (pidl, szPath, MAX_PATH);
                GetDiskFreeSpaceExA (szPath, &ulBytes, NULL, NULL);
                StrFormatByteSizeA (ulBytes.u.LowPart, psd->str.u.cStr, MAX_PATH);
            }
            break;
        }
        hr = S_OK;
    }

    return hr;
}

static HRESULT WINAPI ISF_MyComputer_fnMapColumnToSCID (
               IShellFolder2 * iface, UINT column, SHCOLUMNID * pscid)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}

static const IShellFolder2Vtbl vt_ShellFolder2 =
{
    ISF_MyComputer_fnQueryInterface,
    ISF_MyComputer_fnAddRef,
    ISF_MyComputer_fnRelease,
    ISF_MyComputer_fnParseDisplayName,
    ISF_MyComputer_fnEnumObjects,
    ISF_MyComputer_fnBindToObject,
    ISF_MyComputer_fnBindToStorage,
    ISF_MyComputer_fnCompareIDs,
    ISF_MyComputer_fnCreateViewObject,
    ISF_MyComputer_fnGetAttributesOf,
    ISF_MyComputer_fnGetUIObjectOf,
    ISF_MyComputer_fnGetDisplayNameOf,
    ISF_MyComputer_fnSetNameOf,
    /* ShellFolder2 */
    ISF_MyComputer_fnGetDefaultSearchGUID,
    ISF_MyComputer_fnEnumSearches,
    ISF_MyComputer_fnGetDefaultColumn,
    ISF_MyComputer_fnGetDefaultColumnState,
    ISF_MyComputer_fnGetDetailsEx,
    ISF_MyComputer_fnGetDetailsOf,
    ISF_MyComputer_fnMapColumnToSCID
};

/************************************************************************
 *    IMCFldr_PersistFolder2_QueryInterface
 */
static HRESULT WINAPI IMCFldr_PersistFolder2_QueryInterface (
               IPersistFolder2 * iface, REFIID iid, LPVOID * ppvObj)
{
    IGenericSFImpl *This = impl_from_IPersistFolder2(iface);

    TRACE ("(%p)\n", This);

    return IUnknown_QueryInterface (_IUnknown_ (This), iid, ppvObj);
}

/************************************************************************
 *    IMCFldr_PersistFolder2_AddRef
 */
static ULONG WINAPI IMCFldr_PersistFolder2_AddRef (IPersistFolder2 * iface)
{
    IGenericSFImpl *This = impl_from_IPersistFolder2(iface);

    TRACE ("(%p)->(count=%u)\n", This, This->ref);

    return IUnknown_AddRef (_IUnknown_ (This));
}

/************************************************************************
 *    ISFPersistFolder_Release
 */
static ULONG WINAPI IMCFldr_PersistFolder2_Release (IPersistFolder2 * iface)
{
    IGenericSFImpl *This = impl_from_IPersistFolder2(iface);

    TRACE ("(%p)->(count=%u)\n", This, This->ref);

    return IUnknown_Release (_IUnknown_ (This));
}

/************************************************************************
 *    IMCFldr_PersistFolder2_GetClassID
 */
static HRESULT WINAPI IMCFldr_PersistFolder2_GetClassID (
               IPersistFolder2 * iface, CLSID * lpClassId)
{
    IGenericSFImpl *This = impl_from_IPersistFolder2(iface);

    TRACE ("(%p)\n", This);

    if (!lpClassId)
    return E_POINTER;
    *lpClassId = CLSID_MyComputer;

    return S_OK;
}

/************************************************************************
 *    IMCFldr_PersistFolder2_Initialize
 *
 * NOTES: it makes no sense to change the pidl
 */
static HRESULT WINAPI IMCFldr_PersistFolder2_Initialize (
               IPersistFolder2 * iface, LPCITEMIDLIST pidl)
{
    IGenericSFImpl *This = impl_from_IPersistFolder2(iface);
    TRACE ("(%p)->(%p)\n", This, pidl);
    return E_NOTIMPL;
}

/**************************************************************************
 *    IPersistFolder2_fnGetCurFolder
 */
static HRESULT WINAPI IMCFldr_PersistFolder2_GetCurFolder (
               IPersistFolder2 * iface, LPITEMIDLIST * pidl)
{
    IGenericSFImpl *This = impl_from_IPersistFolder2(iface);

    TRACE ("(%p)->(%p)\n", This, pidl);

    if (!pidl)
        return E_POINTER;
    *pidl = ILClone (This->pidlRoot);
    return S_OK;
}

static const IPersistFolder2Vtbl vt_PersistFolder2 =
{
    IMCFldr_PersistFolder2_QueryInterface,
    IMCFldr_PersistFolder2_AddRef,
    IMCFldr_PersistFolder2_Release,
    IMCFldr_PersistFolder2_GetClassID,
    IMCFldr_PersistFolder2_Initialize,
    IMCFldr_PersistFolder2_GetCurFolder
};

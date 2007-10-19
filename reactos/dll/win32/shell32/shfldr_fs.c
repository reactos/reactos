
/*
 * file system folder
 *
 * Copyright 1997             Marcus Meissner
 * Copyright 1998, 1999, 2002 Juergen Schmied
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
#include "winuser.h"

#include "ole2.h"
#include "shlguid.h"

#include "enumidlist.h"
#include "pidl.h"
#include "undocshell.h"
#include "shell32_main.h"
#include "shresdef.h"
#include "shlwapi.h"
#include "shellfolder.h"
#include "wine/debug.h"
#include "debughlp.h"
#include "shfldr.h"

WINE_DEFAULT_DEBUG_CHANNEL (shell);

/***********************************************************************
*   IShellFolder implementation
*/

typedef struct {
    const IUnknownVtbl        *lpVtbl;
    LONG                ref;
    const IShellFolder2Vtbl   *lpvtblShellFolder;
    const IPersistFolder3Vtbl *lpvtblPersistFolder3;
    const IDropTargetVtbl     *lpvtblDropTarget;
    const ISFHelperVtbl       *lpvtblSFHelper;

    IUnknown *pUnkOuter; /* used for aggregation */

    CLSID *pclsid;

    /* both paths are parsible from the desktop */
    LPWSTR sPathTarget;     /* complete path to target used for enumeration and ChangeNotify */

    LPITEMIDLIST pidlRoot; /* absolute pidl */

    UINT cfShellIDList;    /* clipboardformat for IDropTarget */
    BOOL fAcceptFmt;       /* flag for pending Drop */
} IGenericSFImpl;

static const IUnknownVtbl unkvt;
static const IShellFolder2Vtbl sfvt;
static const IPersistFolder3Vtbl vt_FSFldr_PersistFolder3; /* IPersistFolder3 for a FS_Folder */
static const IDropTargetVtbl dtvt;
static const ISFHelperVtbl shvt;

static inline IGenericSFImpl *impl_from_IShellFolder2( IShellFolder2 *iface )
{
    return (IGenericSFImpl *)((char*)iface - FIELD_OFFSET(IGenericSFImpl, lpvtblShellFolder));
}

static inline IGenericSFImpl *impl_from_IPersistFolder3( IPersistFolder3 *iface )
{
    return (IGenericSFImpl *)((char*)iface - FIELD_OFFSET(IGenericSFImpl, lpvtblPersistFolder3));
}

static inline IGenericSFImpl *impl_from_IDropTarget( IDropTarget *iface )
{
    return (IGenericSFImpl *)((char*)iface - FIELD_OFFSET(IGenericSFImpl, lpvtblDropTarget));
}

static inline IGenericSFImpl *impl_from_ISFHelper( ISFHelper *iface )
{
    return (IGenericSFImpl *)((char*)iface - FIELD_OFFSET(IGenericSFImpl, lpvtblSFHelper));
}


/*
  converts This to an interface pointer
*/
#define _IUnknown_(This)        (IUnknown*)&(This->lpVtbl)
#define _IShellFolder_(This)    (IShellFolder*)&(This->lpvtblShellFolder)
#define _IShellFolder2_(This)   (IShellFolder2*)&(This->lpvtblShellFolder)
#define _IPersist_(This)        (IPersist*)&(This->lpvtblPersistFolder3)
#define _IPersistFolder_(This)  (IPersistFolder*)&(This->lpvtblPersistFolder3)
#define _IPersistFolder2_(This) (IPersistFolder2*)&(This->lpvtblPersistFolder3)
#define _IPersistFolder3_(This) (IPersistFolder3*)&(This->lpvtblPersistFolder3)
#define _IDropTarget_(This)     (IDropTarget*)&(This->lpvtblDropTarget)
#define _ISFHelper_(This)       (ISFHelper*)&(This->lpvtblSFHelper)

/**************************************************************************
* registers clipboardformat once
*/
static void SF_RegisterClipFmt (IGenericSFImpl * This)
{
    TRACE ("(%p)\n", This);

    if (!This->cfShellIDList) {
        This->cfShellIDList = RegisterClipboardFormatA (CFSTR_SHELLIDLIST);
    }
}

/**************************************************************************
* we need a separate IUnknown to handle aggregation
* (inner IUnknown)
*/
static HRESULT WINAPI IUnknown_fnQueryInterface (IUnknown * iface, REFIID riid, LPVOID * ppvObj)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    TRACE ("(%p)->(%s,%p)\n", This, shdebugstr_guid (riid), ppvObj);

    *ppvObj = NULL;

    if (IsEqualIID (riid, &IID_IUnknown))
        *ppvObj = _IUnknown_ (This);
    else if (IsEqualIID (riid, &IID_IShellFolder))
        *ppvObj = _IShellFolder_ (This);
    else if (IsEqualIID (riid, &IID_IShellFolder2))
        *ppvObj = _IShellFolder_ (This);
    else if (IsEqualIID (riid, &IID_IPersist))
        *ppvObj = _IPersist_ (This);
    else if (IsEqualIID (riid, &IID_IPersistFolder))
        *ppvObj = _IPersistFolder_ (This);
    else if (IsEqualIID (riid, &IID_IPersistFolder2))
        *ppvObj = _IPersistFolder2_ (This);
    else if (IsEqualIID (riid, &IID_IPersistFolder3))
        *ppvObj = _IPersistFolder3_ (This);
    else if (IsEqualIID (riid, &IID_ISFHelper))
        *ppvObj = _ISFHelper_ (This);
    else if (IsEqualIID (riid, &IID_IDropTarget)) {
        *ppvObj = _IDropTarget_ (This);
        SF_RegisterClipFmt (This);
    }

    if (*ppvObj) {
        IUnknown_AddRef ((IUnknown *) (*ppvObj));
        TRACE ("-- Interface = %p\n", *ppvObj);
        return S_OK;
    }
    TRACE ("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI IUnknown_fnAddRef (IUnknown * iface)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE ("(%p)->(count=%u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IUnknown_fnRelease (IUnknown * iface)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE ("(%p)->(count=%u)\n", This, refCount + 1);

    if (!refCount) {
        TRACE ("-- destroying IShellFolder(%p)\n", This);

        SHFree (This->pidlRoot);
        SHFree (This->sPathTarget);
        LocalFree ((HLOCAL) This);
    }
    return refCount;
}

static const IUnknownVtbl unkvt =
{
      IUnknown_fnQueryInterface,
      IUnknown_fnAddRef,
      IUnknown_fnRelease,
};

static const shvheader GenericSFHeader[] = {
    {IDS_SHV_COLUMN1, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},
    {IDS_SHV_COLUMN2, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN3, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN4, SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 12},
    {IDS_SHV_COLUMN5, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 5}
};

#define GENERICSHELLVIEWCOLUMNS 5

/**************************************************************************
* IFSFolder_Constructor
*
* NOTES
*  creating undocumented ShellFS_Folder as part of an aggregation
*  {F3364BA0-65B9-11CE-A9BA-00AA004AE837}
*
*/
HRESULT WINAPI
IFSFolder_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv)
{
    IGenericSFImpl *sf;

    TRACE ("unkOut=%p %s\n", pUnkOuter, shdebugstr_guid (riid));

    if (pUnkOuter && !IsEqualIID (riid, &IID_IUnknown))
        return CLASS_E_NOAGGREGATION;
    sf = (IGenericSFImpl *) LocalAlloc (LMEM_ZEROINIT, sizeof (IGenericSFImpl));
    if (!sf)
        return E_OUTOFMEMORY;

    sf->ref = 0;
    sf->lpVtbl = &unkvt;
    sf->lpvtblShellFolder = &sfvt;
    sf->lpvtblPersistFolder3 = &vt_FSFldr_PersistFolder3;
    sf->lpvtblDropTarget = &dtvt;
    sf->lpvtblSFHelper = &shvt;
    sf->pclsid = (CLSID *) & CLSID_ShellFSFolder;
    sf->pUnkOuter = pUnkOuter ? pUnkOuter : _IUnknown_ (sf);

    if (!SUCCEEDED (IUnknown_QueryInterface (_IUnknown_ (sf), riid, ppv))) {
        IUnknown_Release (_IUnknown_ (sf));
        return E_NOINTERFACE;
    }

    TRACE ("--%p\n", *ppv);
    return S_OK;
}

/**************************************************************************
 *  IShellFolder_fnQueryInterface
 *
 * PARAMETERS
 *  REFIID riid       [in ] Requested InterfaceID
 *  LPVOID* ppvObject [out] Interface* to hold the result
 */
static HRESULT WINAPI
IShellFolder_fnQueryInterface (IShellFolder2 * iface, REFIID riid,
                               LPVOID * ppvObj)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);

    TRACE ("(%p)->(%s,%p)\n", This, shdebugstr_guid (riid), ppvObj);

    return IUnknown_QueryInterface (This->pUnkOuter, riid, ppvObj);
}

/**************************************************************************
*  IShellFolder_AddRef
*/

static ULONG WINAPI IShellFolder_fnAddRef (IShellFolder2 * iface)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);

    TRACE ("(%p)->(count=%u)\n", This, This->ref);

    return IUnknown_AddRef (This->pUnkOuter);
}

/**************************************************************************
 *  IShellFolder_fnRelease
 */
static ULONG WINAPI IShellFolder_fnRelease (IShellFolder2 * iface)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);

    TRACE ("(%p)->(count=%u)\n", This, This->ref);

    return IUnknown_Release (This->pUnkOuter);
}

/**************************************************************************
 *  SHELL32_CreatePidlFromBindCtx  [internal]
 *
 *  If the caller bound File System Bind Data, assume it is the
 *   find data for the path.
 *  This allows binding of paths that don't exist.
 */
LPITEMIDLIST SHELL32_CreatePidlFromBindCtx(IBindCtx *pbc, LPCWSTR path)
{
    static WCHAR szfsbc[] = {
        'F','i','l','e',' ','S','y','s','t','e','m',' ',
        'B','i','n','d',' ','D','a','t','a',0 };
    IFileSystemBindData *fsbd = NULL;
    LPITEMIDLIST pidl = NULL;
    IUnknown *param = NULL;
    WIN32_FIND_DATAW wfd;
    HRESULT r;

    TRACE("%p %s\n", pbc, debugstr_w(path));

    if (!pbc)
        return NULL;

    /* see if the caller bound File System Bind Data */
    r = IBindCtx_GetObjectParam( pbc, (LPOLESTR) szfsbc, &param );
    if (FAILED(r))
        return NULL;

    r = IUnknown_QueryInterface( param, &IID_IFileSystemBindData,
                                 (LPVOID*) &fsbd );
    if (SUCCEEDED(r))
    {
        r = IFileSystemBindData_GetFindData( fsbd, &wfd );
        if (SUCCEEDED(r))
        {
            lstrcpynW( &wfd.cFileName[0], path, MAX_PATH );
            pidl = _ILCreateFromFindDataW( &wfd );
        }
        IFileSystemBindData_Release( fsbd );
    }

    return pidl;
}

/**************************************************************************
* IShellFolder_ParseDisplayName {SHELL32}
*
* Parse a display name.
*
* PARAMS
*  hwndOwner       [in]  Parent window for any message's
*  pbc             [in]  optional FileSystemBindData context
*  lpszDisplayName [in]  Unicode displayname.
*  pchEaten        [out] (unicode) characters processed
*  ppidl           [out] complex pidl to item
*  pdwAttributes   [out] items attributes
*
* NOTES
*  Every folder tries to parse only its own (the leftmost) pidl and creates a
*  subfolder to evaluate the remaining parts.
*  Now we can parse into namespaces implemented by shell extensions
*
*  Behaviour on win98: lpszDisplayName=NULL -> crash
*                      lpszDisplayName="" -> returns mycoputer-pidl
*
* FIXME
*    pdwAttributes is not set
*    pchEaten is not set like in windows
*/
static HRESULT WINAPI
IShellFolder_fnParseDisplayName (IShellFolder2 * iface,
                                 HWND hwndOwner,
                                 LPBC pbc,
                                 LPOLESTR lpszDisplayName,
                                 DWORD * pchEaten, LPITEMIDLIST * ppidl,
                                 DWORD * pdwAttributes)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);

    HRESULT hr = E_INVALIDARG;
    LPCWSTR szNext = NULL;
    WCHAR szElement[MAX_PATH];
    WCHAR szPath[MAX_PATH];
    LPITEMIDLIST pidlTemp = NULL;
    DWORD len;

    TRACE ("(%p)->(HWND=%p,%p,%p=%s,%p,pidl=%p,%p)\n",
     This, hwndOwner, pbc, lpszDisplayName, debugstr_w (lpszDisplayName),
     pchEaten, ppidl, pdwAttributes);

    if (!lpszDisplayName || !ppidl)
        return E_INVALIDARG;

    if (pchEaten)
        *pchEaten = 0; /* strange but like the original */

    pidlTemp = SHELL32_CreatePidlFromBindCtx(pbc, lpszDisplayName);
    if (!pidlTemp && *lpszDisplayName)
    {
        /* get the next element */
        szNext = GetNextElementW (lpszDisplayName, szElement, MAX_PATH);

        /* build the full pathname to the element */
        lstrcpynW(szPath, This->sPathTarget, MAX_PATH - 1);
        PathAddBackslashW(szPath);
        len = lstrlenW(szPath);
        lstrcpynW(szPath + len, szElement, MAX_PATH - len);

        /* get the pidl */
        hr = _ILCreateFromPathW(szPath, &pidlTemp);

        if (SUCCEEDED(hr)) {
            if (szNext && *szNext) {
                /* try to analyse the next element */
                hr = SHELL32_ParseNextElement (iface, hwndOwner, pbc,
                 &pidlTemp, (LPOLESTR) szNext, pchEaten, pdwAttributes);
            } else {
                /* it's the last element */
                if (pdwAttributes && *pdwAttributes) {
                    hr = SHELL32_GetItemAttributes (_IShellFolder_ (This),
                     pidlTemp, pdwAttributes);
                }
            }
        }
    }

    if (SUCCEEDED(hr))
        *ppidl = pidlTemp;
    else
        *ppidl = NULL;

    TRACE ("(%p)->(-- pidl=%p ret=0x%08x)\n", This, ppidl ? *ppidl : 0, hr);

    return hr;
}

/**************************************************************************
* IShellFolder_fnEnumObjects
* PARAMETERS
*  HWND          hwndOwner,    //[in ] Parent Window
*  DWORD         grfFlags,     //[in ] SHCONTF enumeration mask
*  LPENUMIDLIST* ppenumIDList  //[out] IEnumIDList interface
*/
static HRESULT WINAPI
IShellFolder_fnEnumObjects (IShellFolder2 * iface, HWND hwndOwner,
                            DWORD dwFlags, LPENUMIDLIST * ppEnumIDList)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);

    TRACE ("(%p)->(HWND=%p flags=0x%08x pplist=%p)\n", This, hwndOwner,
     dwFlags, ppEnumIDList);

    *ppEnumIDList = IEnumIDList_Constructor();
    if (*ppEnumIDList)
        CreateFolderEnumList(*ppEnumIDList, This->sPathTarget, dwFlags);

    TRACE ("-- (%p)->(new ID List: %p)\n", This, *ppEnumIDList);

    return *ppEnumIDList ? S_OK : E_OUTOFMEMORY;
}

/**************************************************************************
* IShellFolder_fnBindToObject
* PARAMETERS
*  LPCITEMIDLIST pidl,       //[in ] relative pidl to open
*  LPBC          pbc,        //[in ] optional FileSystemBindData context
*  REFIID        riid,       //[in ] Initial Interface
*  LPVOID*       ppvObject   //[out] Interface*
*/
static HRESULT WINAPI
IShellFolder_fnBindToObject (IShellFolder2 * iface, LPCITEMIDLIST pidl,
                             LPBC pbc, REFIID riid, LPVOID * ppvOut)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);

    TRACE ("(%p)->(pidl=%p,%p,%s,%p)\n", This, pidl, pbc,
     shdebugstr_guid (riid), ppvOut);

    return SHELL32_BindToChild (This->pidlRoot, This->sPathTarget, pidl, riid,
     ppvOut);
}

/**************************************************************************
*  IShellFolder_fnBindToStorage
* PARAMETERS
*  LPCITEMIDLIST pidl,       //[in ] complex pidl to store
*  LPBC          pbc,        //[in ] reserved
*  REFIID        riid,       //[in ] Initial storage interface
*  LPVOID*       ppvObject   //[out] Interface* returned
*/
static HRESULT WINAPI
IShellFolder_fnBindToStorage (IShellFolder2 * iface, LPCITEMIDLIST pidl,
                              LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);

    FIXME ("(%p)->(pidl=%p,%p,%s,%p) stub\n", This, pidl, pbcReserved,
     shdebugstr_guid (riid), ppvOut);

    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
*  IShellFolder_fnCompareIDs
*/

static HRESULT WINAPI
IShellFolder_fnCompareIDs (IShellFolder2 * iface, LPARAM lParam,
                           LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);

    int nReturn;

    TRACE ("(%p)->(0x%08lx,pidl1=%p,pidl2=%p)\n", This, lParam, pidl1, pidl2);
    nReturn = SHELL32_CompareIDs (_IShellFolder_ (This), lParam, pidl1, pidl2);
    TRACE ("-- %i\n", nReturn);
    return nReturn;
}

/**************************************************************************
* IShellFolder_fnCreateViewObject
*/
static HRESULT WINAPI
IShellFolder_fnCreateViewObject (IShellFolder2 * iface, HWND hwndOwner,
                                 REFIID riid, LPVOID * ppvOut)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);

    LPSHELLVIEW pShellView;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(hwnd=%p,%s,%p)\n", This, hwndOwner, shdebugstr_guid (riid),
     ppvOut);

    if (ppvOut) {
        *ppvOut = NULL;

        if (IsEqualIID (riid, &IID_IDropTarget)) {
            hr = IShellFolder_QueryInterface (iface, &IID_IDropTarget, ppvOut);
        } else if (IsEqualIID (riid, &IID_IContextMenu)) {
            FIXME ("IContextMenu not implemented\n");
            hr = E_NOTIMPL;
        } else if (IsEqualIID (riid, &IID_IShellView)) {
            pShellView = IShellView_Constructor ((IShellFolder *) iface);
            if (pShellView) {
                hr = IShellView_QueryInterface (pShellView, riid, ppvOut);
                IShellView_Release (pShellView);
            }
        }
    }
    TRACE ("-- (%p)->(interface=%p)\n", This, ppvOut);
    return hr;
}

/**************************************************************************
*  IShellFolder_fnGetAttributesOf
*
* PARAMETERS
*  UINT            cidl,     //[in ] num elements in pidl array
*  LPCITEMIDLIST*  apidl,    //[in ] simple pidl array
*  ULONG*          rgfInOut) //[out] result array
*
*/
static HRESULT WINAPI
IShellFolder_fnGetAttributesOf (IShellFolder2 * iface, UINT cidl,
                                LPCITEMIDLIST * apidl, DWORD * rgfInOut)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);

    HRESULT hr = S_OK;

    TRACE ("(%p)->(cidl=%d apidl=%p mask=%p (0x%08x))\n", This, cidl, apidl,
     rgfInOut, rgfInOut ? *rgfInOut : 0);

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
    }
    else {
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
*  IShellFolder_fnGetUIObjectOf
*
* PARAMETERS
*  HWND           hwndOwner, //[in ] Parent window for any output
*  UINT           cidl,      //[in ] array size
*  LPCITEMIDLIST* apidl,     //[in ] simple pidl array
*  REFIID         riid,      //[in ] Requested Interface
*  UINT*          prgfInOut, //[   ] reserved
*  LPVOID*        ppvObject) //[out] Resulting Interface
*
* NOTES
*  This function gets asked to return "view objects" for one or more (multiple
*  select) items:
*  The viewobject typically is an COM object with one of the following
*  interfaces:
*  IExtractIcon,IDataObject,IContextMenu
*  In order to support icon positions in the default Listview your DataObject
*  must implement the SetData method (in addition to GetData :) - the shell
*  passes a barely documented "Icon positions" structure to SetData when the
*  drag starts, and GetData's it if the drop is in another explorer window that
*  needs the positions.
*/
static HRESULT WINAPI
IShellFolder_fnGetUIObjectOf (IShellFolder2 * iface,
                              HWND hwndOwner,
                              UINT cidl, LPCITEMIDLIST * apidl, REFIID riid,
                              UINT * prgfInOut, LPVOID * ppvOut)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);

    LPITEMIDLIST pidl;
    IUnknown *pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(%p,%u,apidl=%p,%s,%p,%p)\n",
     This, hwndOwner, cidl, apidl, shdebugstr_guid (riid), prgfInOut, ppvOut);

    if (ppvOut) {
        *ppvOut = NULL;

        if (IsEqualIID (riid, &IID_IContextMenu) && (cidl >= 1)) {
            pObj = (LPUNKNOWN) ISvItemCm_Constructor ((IShellFolder *) iface,
             This->pidlRoot, apidl, cidl);
            hr = S_OK;
        } else if (IsEqualIID (riid, &IID_IDataObject) && (cidl >= 1)) {
            pObj = (LPUNKNOWN) IDataObject_Constructor (hwndOwner,
             This->pidlRoot, apidl, cidl);
            hr = S_OK;
        } else if (IsEqualIID (riid, &IID_IExtractIconA) && (cidl == 1)) {
            pidl = ILCombine (This->pidlRoot, apidl[0]);
            pObj = (LPUNKNOWN) IExtractIconA_Constructor (pidl);
            SHFree (pidl);
            hr = S_OK;
        } else if (IsEqualIID (riid, &IID_IExtractIconW) && (cidl == 1)) {
            pidl = ILCombine (This->pidlRoot, apidl[0]);
            pObj = (LPUNKNOWN) IExtractIconW_Constructor (pidl);
            SHFree (pidl);
            hr = S_OK;
        } else if (IsEqualIID (riid, &IID_IDropTarget) && (cidl >= 1)) {
            hr = IShellFolder_QueryInterface (iface, &IID_IDropTarget,
             (LPVOID *) & pObj);
        } else if ((IsEqualIID(riid,&IID_IShellLinkW) ||
         IsEqualIID(riid,&IID_IShellLinkA)) && (cidl == 1)) {
            pidl = ILCombine (This->pidlRoot, apidl[0]);
            hr = IShellLink_ConstructFromFile(NULL, riid, pidl, (LPVOID*)&pObj);
            SHFree (pidl);
        } else {
            hr = E_NOINTERFACE;
        }

        if (SUCCEEDED(hr) && !pObj)
            hr = E_OUTOFMEMORY;

        *ppvOut = pObj;
    }
    TRACE ("(%p)->hr=0x%08x\n", This, hr);
    return hr;
}

static const WCHAR AdvancedW[] = { 'S','O','F','T','W','A','R','E',
 '\\','M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\',
 'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\','E','x','p','l',
 'o','r','e','r','\\','A','d','v','a','n','c','e','d',0 };
static const WCHAR HideFileExtW[] = { 'H','i','d','e','F','i','l','e','E','x',
 't',0 };
static const WCHAR NeverShowExtW[] = { 'N','e','v','e','r','S','h','o','w','E',
 'x','t',0 };

/******************************************************************************
 * SHELL_FS_HideExtension [Internal]
 *
 * Query the registry if the filename extension of a given path should be
 * hidden.
 *
 * PARAMS
 *  szPath [I] Relative or absolute path of a file
 *
 * RETURNS
 *  TRUE, if the filename's extension should be hidden
 *  FALSE, otherwise.
 */
BOOL SHELL_FS_HideExtension(LPWSTR szPath)
{
    HKEY hKey;
    DWORD dwData;
    DWORD dwDataSize = sizeof (DWORD);
    BOOL doHide = FALSE; /* The default value is FALSE (win98 at least) */

    if (!RegCreateKeyExW(HKEY_CURRENT_USER, AdvancedW, 0, 0, 0, KEY_ALL_ACCESS, 0, &hKey, 0)) {
        if (!RegQueryValueExW(hKey, HideFileExtW, 0, 0, (LPBYTE) &dwData, &dwDataSize))
            doHide = dwData;
        RegCloseKey (hKey);
    }

    if (!doHide) {
        LPWSTR ext = PathFindExtensionW(szPath);

        if (*ext != '\0') {
            WCHAR classname[MAX_PATH];
            LONG classlen = sizeof(classname);

            if (!RegQueryValueW(HKEY_CLASSES_ROOT, ext, classname, &classlen))
                if (!RegOpenKeyW(HKEY_CLASSES_ROOT, classname, &hKey)) {
                    if (!RegQueryValueExW(hKey, NeverShowExtW, 0, NULL, NULL, NULL))
                        doHide = TRUE;
                    RegCloseKey(hKey);
                }
        }
    }
    return doHide;
}

void SHELL_FS_ProcessDisplayFilename(LPWSTR szPath, DWORD dwFlags)
{
    /*FIXME: MSDN also mentions SHGDN_FOREDITING which is not yet handled. */
    if (!(dwFlags & SHGDN_FORPARSING) &&
        ((dwFlags & SHGDN_INFOLDER) || (dwFlags == SHGDN_NORMAL))) {
        if (SHELL_FS_HideExtension(szPath) && szPath[0] != '.')
            PathRemoveExtensionW(szPath);
    }
}

/**************************************************************************
*  IShellFolder_fnGetDisplayNameOf
*  Retrieves the display name for the specified file object or subfolder
*
* PARAMETERS
*  LPCITEMIDLIST pidl,    //[in ] complex pidl to item
*  DWORD         dwFlags, //[in ] SHGNO formatting flags
*  LPSTRRET      lpName)  //[out] Returned display name
*
* FIXME
*  if the name is in the pidl the ret value should be a STRRET_OFFSET
*/

static HRESULT WINAPI
IShellFolder_fnGetDisplayNameOf (IShellFolder2 * iface, LPCITEMIDLIST pidl,
                                 DWORD dwFlags, LPSTRRET strRet)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);
    LPWSTR pszPath;

    HRESULT hr = S_OK;
    int len = 0;

    TRACE ("(%p)->(pidl=%p,0x%08x,%p)\n", This, pidl, dwFlags, strRet);
    pdump (pidl);

    if (!pidl || !strRet)
        return E_INVALIDARG;

    pszPath = CoTaskMemAlloc((MAX_PATH +1) * sizeof(WCHAR));
    if (!pszPath)
        return E_OUTOFMEMORY;

    if (_ILIsDesktop(pidl)) { /* empty pidl */
        if ((GET_SHGDN_FOR(dwFlags) & SHGDN_FORPARSING) &&
            (GET_SHGDN_RELATION(dwFlags) != SHGDN_INFOLDER))
        {
            if (This->sPathTarget)
                lstrcpynW(pszPath, This->sPathTarget, MAX_PATH);
        } else {
            /* pidl has to contain exactly one non null SHITEMID */
            hr = E_INVALIDARG;
        }
    } else if (_ILIsPidlSimple(pidl)) {
        if ((GET_SHGDN_FOR(dwFlags) & SHGDN_FORPARSING) &&
            (GET_SHGDN_RELATION(dwFlags) != SHGDN_INFOLDER) &&
            This->sPathTarget)
        {
            lstrcpynW(pszPath, This->sPathTarget, MAX_PATH);
            PathAddBackslashW(pszPath);
            len = lstrlenW(pszPath);
        }
        _ILSimpleGetTextW(pidl, pszPath + len, MAX_PATH + 1 - len);
        if (!_ILIsFolder(pidl)) SHELL_FS_ProcessDisplayFilename(pszPath, dwFlags);
    } else {
        hr = SHELL32_GetDisplayNameOfChild(iface, pidl, dwFlags, pszPath, MAX_PATH);
    }

    if (SUCCEEDED(hr)) {
        /* Win9x always returns ANSI strings, NT always returns Unicode strings */
        if (GetVersion() & 0x80000000) {
            strRet->uType = STRRET_CSTR;
            if (!WideCharToMultiByte(CP_ACP, 0, pszPath, -1, strRet->u.cStr, MAX_PATH,
                 NULL, NULL))
                strRet->u.cStr[0] = '\0';
            CoTaskMemFree(pszPath);
        } else {
            strRet->uType = STRRET_WSTR;
            strRet->u.pOleStr = pszPath;
        }
    } else
        CoTaskMemFree(pszPath);

    TRACE ("-- (%p)->(%s)\n", This, strRet->uType == STRRET_CSTR ? strRet->u.cStr : debugstr_w(strRet->u.pOleStr));
    return hr;
}

/**************************************************************************
*  IShellFolder_fnSetNameOf
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
static HRESULT WINAPI IShellFolder_fnSetNameOf (IShellFolder2 * iface,
                                                HWND hwndOwner,
                                                LPCITEMIDLIST pidl,
                                                LPCOLESTR lpName,
                                                DWORD dwFlags,
                                                LPITEMIDLIST * pPidlOut)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);
    WCHAR szSrc[MAX_PATH + 1], szDest[MAX_PATH + 1];
    LPWSTR ptr;
    BOOL bIsFolder = _ILIsFolder (ILFindLastID (pidl));

    TRACE ("(%p)->(%p,pidl=%p,%s,%u,%p)\n", This, hwndOwner, pidl,
     debugstr_w (lpName), dwFlags, pPidlOut);

    /* build source path */
    lstrcpynW(szSrc, This->sPathTarget, MAX_PATH);
    ptr = PathAddBackslashW (szSrc);
    if (ptr)
        _ILSimpleGetTextW (pidl, ptr, MAX_PATH + 1 - (ptr - szSrc));

    /* build destination path */
    if (dwFlags == SHGDN_NORMAL || dwFlags & SHGDN_INFOLDER) {
        lstrcpynW(szDest, This->sPathTarget, MAX_PATH);
        ptr = PathAddBackslashW (szDest);
        if (ptr)
            lstrcpynW(ptr, lpName, MAX_PATH + 1 - (ptr - szDest));
    } else
        lstrcpynW(szDest, lpName, MAX_PATH);

    if(!(dwFlags & SHGDN_FORPARSING) && SHELL_FS_HideExtension(szSrc)) {
        WCHAR *ext = PathFindExtensionW(szSrc);
        if(*ext != '\0') {
            INT len = strlenW(szDest);
            lstrcpynW(szDest + len, ext, MAX_PATH - len);
        }
    }

    TRACE ("src=%s dest=%s\n", debugstr_w(szSrc), debugstr_w(szDest));

    if (MoveFileW (szSrc, szDest)) {
        HRESULT hr = S_OK;

        if (pPidlOut)
            hr = _ILCreateFromPathW(szDest, pPidlOut);

        SHChangeNotify (bIsFolder ? SHCNE_RENAMEFOLDER : SHCNE_RENAMEITEM,
         SHCNF_PATHW, szSrc, szDest);

        return hr;
    }

    return E_FAIL;
}

static HRESULT WINAPI IShellFolder_fnGetDefaultSearchGUID (IShellFolder2 *iface,
                                                           GUID * pguid)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);
    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}
static HRESULT WINAPI IShellFolder_fnEnumSearches (IShellFolder2 * iface,
                                                   IEnumExtraSearch ** ppenum)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);
    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IShellFolder_fnGetDefaultColumn (IShellFolder2 * iface, DWORD dwRes,
                                 ULONG * pSort, ULONG * pDisplay)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);

    TRACE ("(%p)\n", This);

    if (pSort)
        *pSort = 0;
    if (pDisplay)
        *pDisplay = 0;

    return S_OK;
}

static HRESULT WINAPI
IShellFolder_fnGetDefaultColumnState (IShellFolder2 * iface, UINT iColumn,
                                      DWORD * pcsFlags)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);

    TRACE ("(%p)\n", This);

    if (!pcsFlags || iColumn >= GENERICSHELLVIEWCOLUMNS)
        return E_INVALIDARG;

    *pcsFlags = GenericSFHeader[iColumn].pcsFlags;

    return S_OK;
}

static HRESULT WINAPI
IShellFolder_fnGetDetailsEx (IShellFolder2 * iface, LPCITEMIDLIST pidl,
                             const SHCOLUMNID * pscid, VARIANT * pv)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);
    FIXME ("(%p)\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI
IShellFolder_fnGetDetailsOf (IShellFolder2 * iface, LPCITEMIDLIST pidl,
                             UINT iColumn, SHELLDETAILS * psd)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);
    HRESULT hr = E_FAIL;

    TRACE ("(%p)->(%p %i %p)\n", This, pidl, iColumn, psd);

    if (!psd || iColumn >= GENERICSHELLVIEWCOLUMNS)
        return E_INVALIDARG;

    if (!pidl) {
        /* the header titles */
        psd->fmt = GenericSFHeader[iColumn].fmt;
        psd->cxChar = GenericSFHeader[iColumn].cxChar;
        psd->str.uType = STRRET_CSTR;
        LoadStringA (shell32_hInstance, GenericSFHeader[iColumn].colnameid,
         psd->str.u.cStr, MAX_PATH);
        return S_OK;
    } else {
        hr = S_OK;
        psd->str.uType = STRRET_CSTR;
        /* the data from the pidl */
        switch (iColumn) {
        case 0:                /* name */
            hr = IShellFolder_GetDisplayNameOf (iface, pidl,
             SHGDN_NORMAL | SHGDN_INFOLDER, &psd->str);
            break;
        case 1:                /* size */
            _ILGetFileSize (pidl, psd->str.u.cStr, MAX_PATH);
            break;
        case 2:                /* type */
            _ILGetFileType (pidl, psd->str.u.cStr, MAX_PATH);
            break;
        case 3:                /* date */
            _ILGetFileDate (pidl, psd->str.u.cStr, MAX_PATH);
            break;
        case 4:                /* attributes */
            _ILGetFileAttributes (pidl, psd->str.u.cStr, MAX_PATH);
            break;
        }
    }

    return hr;
}

static HRESULT WINAPI
IShellFolder_fnMapColumnToSCID (IShellFolder2 * iface, UINT column,
                                SHCOLUMNID * pscid)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);
    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}

static const IShellFolder2Vtbl sfvt =
{
    IShellFolder_fnQueryInterface,
    IShellFolder_fnAddRef,
    IShellFolder_fnRelease,
    IShellFolder_fnParseDisplayName,
    IShellFolder_fnEnumObjects,
    IShellFolder_fnBindToObject,
    IShellFolder_fnBindToStorage,
    IShellFolder_fnCompareIDs,
    IShellFolder_fnCreateViewObject,
    IShellFolder_fnGetAttributesOf,
    IShellFolder_fnGetUIObjectOf,
    IShellFolder_fnGetDisplayNameOf,
    IShellFolder_fnSetNameOf,
    /* ShellFolder2 */
    IShellFolder_fnGetDefaultSearchGUID,
    IShellFolder_fnEnumSearches,
    IShellFolder_fnGetDefaultColumn,
    IShellFolder_fnGetDefaultColumnState,
    IShellFolder_fnGetDetailsEx,
    IShellFolder_fnGetDetailsOf,
    IShellFolder_fnMapColumnToSCID
};

/****************************************************************************
 * ISFHelper for IShellFolder implementation
 */

static HRESULT WINAPI
ISFHelper_fnQueryInterface (ISFHelper * iface, REFIID riid, LPVOID * ppvObj)
{
    IGenericSFImpl *This = impl_from_ISFHelper(iface);

    TRACE ("(%p)->(count=%u)\n", This, This->ref);

    return IUnknown_QueryInterface (This->pUnkOuter, riid, ppvObj);
}

static ULONG WINAPI ISFHelper_fnAddRef (ISFHelper * iface)
{
    IGenericSFImpl *This = impl_from_ISFHelper(iface);

    TRACE ("(%p)->(count=%u)\n", This, This->ref);

    return IUnknown_AddRef (This->pUnkOuter);
}

static ULONG WINAPI ISFHelper_fnRelease (ISFHelper * iface)
{
    IGenericSFImpl *This = impl_from_ISFHelper(iface);

    TRACE ("(%p)\n", This);

    return IUnknown_Release (This->pUnkOuter);
}

/****************************************************************************
 * ISFHelper_fnAddFolder
 *
 * creates a unique folder name
 */

static HRESULT WINAPI
ISFHelper_fnGetUniqueName (ISFHelper * iface, LPWSTR pwszName, UINT uLen)
{
    IGenericSFImpl *This = impl_from_ISFHelper(iface);
    IEnumIDList *penum;
    HRESULT hr;
    WCHAR wszText[MAX_PATH];
    const WCHAR wszNewFolder[] = {'N','e','w',' ','F','o','l','d','e','r',0 };
    const WCHAR wszFormat[] = {'%','s',' ','%','d',0 };

    TRACE ("(%p)(%p %u)\n", This, pwszName, uLen);

    if (uLen < sizeof(wszNewFolder)/sizeof(WCHAR) + 3)
        return E_POINTER;

    lstrcpynW (pwszName, wszNewFolder, uLen);

    hr = IShellFolder_fnEnumObjects (_IShellFolder2_ (This), 0,
     SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN, &penum);
    if (penum) {
        LPITEMIDLIST pidl;
        DWORD dwFetched;
        int i = 1;

next:
        IEnumIDList_Reset (penum);
        while (S_OK == IEnumIDList_Next (penum, 1, &pidl, &dwFetched) &&
         dwFetched) {
            _ILSimpleGetTextW (pidl, wszText, MAX_PATH);
            if (0 == lstrcmpiW (wszText, pwszName)) {
                snprintfW (pwszName, uLen, wszFormat, wszNewFolder, i++);
                if (i > 99) {
                    hr = E_FAIL;
                    break;
                }
                goto next;
            }
        }

        IEnumIDList_Release (penum);
    }
    return hr;
}

/****************************************************************************
 * ISFHelper_fnAddFolder
 *
 * adds a new folder.
 */

static HRESULT WINAPI
ISFHelper_fnAddFolder (ISFHelper * iface, HWND hwnd, LPCWSTR pwszName,
                       LPITEMIDLIST * ppidlOut)
{
    IGenericSFImpl *This = impl_from_ISFHelper(iface);
    WCHAR wszNewDir[MAX_PATH];
    DWORD bRes;
    HRESULT hres = E_FAIL;

    TRACE ("(%p)(%s %p)\n", This, debugstr_w(pwszName), ppidlOut);

    wszNewDir[0] = 0;
    if (This->sPathTarget)
        lstrcpynW(wszNewDir, This->sPathTarget, MAX_PATH);
    PathAppendW(wszNewDir, pwszName);

    bRes = CreateDirectoryW (wszNewDir, NULL);
    if (bRes) {
        SHChangeNotify (SHCNE_MKDIR, SHCNF_PATHW, wszNewDir, NULL);

        hres = S_OK;

        if (ppidlOut)
                hres = _ILCreateFromPathW(wszNewDir, ppidlOut);
    } else {
        WCHAR wszText[128 + MAX_PATH];
        WCHAR wszTempText[128];
        WCHAR wszCaption[256];

        /* Cannot Create folder because of permissions */
        LoadStringW (shell32_hInstance, IDS_CREATEFOLDER_DENIED, wszTempText,
         sizeof (wszTempText));
        LoadStringW (shell32_hInstance, IDS_CREATEFOLDER_CAPTION, wszCaption,
         sizeof (wszCaption));
        sprintfW (wszText, wszTempText, wszNewDir);
        MessageBoxW (hwnd, wszText, wszCaption, MB_OK | MB_ICONEXCLAMATION);
    }

    return hres;
}

/****************************************************************************
 * build_paths_list
 *
 * Builds a list of paths like the one used in SHFileOperation from a table of
 * PIDLs relative to the given base folder
 */
static WCHAR *build_paths_list(LPCWSTR wszBasePath, int cidl, LPCITEMIDLIST *pidls)
{
    WCHAR *wszPathsList;
    WCHAR *wszListPos;
    int iPathLen;
    int i;

    iPathLen = lstrlenW(wszBasePath);
    wszPathsList = HeapAlloc(GetProcessHeap(), 0, MAX_PATH*sizeof(WCHAR)*cidl+1);
    wszListPos = wszPathsList;

    for (i = 0; i < cidl; i++) {
        if (!_ILIsFolder(pidls[i]) && !_ILIsValue(pidls[i]))
            continue;

        lstrcpynW(wszListPos, wszBasePath, MAX_PATH);
        /* FIXME: abort if path too long */
        _ILSimpleGetTextW(pidls[i], wszListPos+iPathLen, MAX_PATH-iPathLen);
        wszListPos += lstrlenW(wszListPos)+1;
    }
    *wszListPos=0;
    return wszPathsList;
}

/****************************************************************************
 * ISFHelper_fnDeleteItems
 *
 * deletes items in folder
 */
static HRESULT WINAPI
ISFHelper_fnDeleteItems (ISFHelper * iface, UINT cidl, LPCITEMIDLIST * apidl)
{
    IGenericSFImpl *This = impl_from_ISFHelper(iface);
    UINT i;
    SHFILEOPSTRUCTW op;
    WCHAR wszPath[MAX_PATH];
    WCHAR *wszPathsList;
    HRESULT ret;
    WCHAR *wszCurrentPath;

    TRACE ("(%p)(%u %p)\n", This, cidl, apidl);
    if (cidl==0) return S_OK;

    if (This->sPathTarget)
        lstrcpynW(wszPath, This->sPathTarget, MAX_PATH);
    else
        wszPath[0] = '\0';
    PathAddBackslashW(wszPath);
    wszPathsList = build_paths_list(wszPath, cidl, apidl);

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
            LPITEMIDLIST pidl = ILCombine(This->pidlRoot, apidl[i]);
            SHChangeNotify(wEventId, SHCNF_IDLIST, pidl, NULL);
            SHFree(pidl);
        }

        wszCurrentPath += lstrlenW(wszCurrentPath)+1;
    }
    HeapFree(GetProcessHeap(), 0, wszPathsList);
    return ret;
}

/****************************************************************************
 * ISFHelper_fnCopyItems
 *
 * copies items to this folder
 */
static HRESULT WINAPI
ISFHelper_fnCopyItems (ISFHelper * iface, IShellFolder * pSFFrom, UINT cidl,
                       LPCITEMIDLIST * apidl)
{
    UINT i;
    IPersistFolder2 *ppf2 = NULL;
    char szSrcPath[MAX_PATH],
      szDstPath[MAX_PATH];

    IGenericSFImpl *This = impl_from_ISFHelper(iface);

    TRACE ("(%p)->(%p,%u,%p)\n", This, pSFFrom, cidl, apidl);

    IShellFolder_QueryInterface (pSFFrom, &IID_IPersistFolder2,
     (LPVOID *) & ppf2);
    if (ppf2) {
        LPITEMIDLIST pidl;

        if (SUCCEEDED (IPersistFolder2_GetCurFolder (ppf2, &pidl))) {
            for (i = 0; i < cidl; i++) {
                SHGetPathFromIDListA (pidl, szSrcPath);
                PathAddBackslashA (szSrcPath);
                _ILSimpleGetText (apidl[i], szSrcPath + strlen (szSrcPath),
                 MAX_PATH);

                if (!WideCharToMultiByte(CP_ACP, 0, This->sPathTarget, -1, szDstPath, MAX_PATH, NULL, NULL))
                    szDstPath[0] = '\0';
                PathAddBackslashA (szDstPath);
                _ILSimpleGetText (apidl[i], szDstPath + strlen (szDstPath),
                 MAX_PATH);
                MESSAGE ("would copy %s to %s\n", szSrcPath, szDstPath);
            }
            SHFree (pidl);
        }
        IPersistFolder2_Release (ppf2);
    }
    return S_OK;
}

static const ISFHelperVtbl shvt =
{
    ISFHelper_fnQueryInterface,
    ISFHelper_fnAddRef,
    ISFHelper_fnRelease,
    ISFHelper_fnGetUniqueName,
    ISFHelper_fnAddFolder,
    ISFHelper_fnDeleteItems,
    ISFHelper_fnCopyItems
};

/************************************************************************
 * IFSFldr_PersistFolder3_QueryInterface
 *
 */
static HRESULT WINAPI
IFSFldr_PersistFolder3_QueryInterface (IPersistFolder3 * iface, REFIID iid,
                                       LPVOID * ppvObj)
{
    IGenericSFImpl *This = impl_from_IPersistFolder3(iface);

    TRACE ("(%p)\n", This);

    return IUnknown_QueryInterface (This->pUnkOuter, iid, ppvObj);
}

/************************************************************************
 * IFSFldr_PersistFolder3_AddRef
 *
 */
static ULONG WINAPI
IFSFldr_PersistFolder3_AddRef (IPersistFolder3 * iface)
{
    IGenericSFImpl *This = impl_from_IPersistFolder3(iface);

    TRACE ("(%p)->(count=%u)\n", This, This->ref);

    return IUnknown_AddRef (This->pUnkOuter);
}

/************************************************************************
 * IFSFldr_PersistFolder3_Release
 *
 */
static ULONG WINAPI
IFSFldr_PersistFolder3_Release (IPersistFolder3 * iface)
{
    IGenericSFImpl *This = impl_from_IPersistFolder3(iface);

    TRACE ("(%p)->(count=%u)\n", This, This->ref);

    return IUnknown_Release (This->pUnkOuter);
}

/************************************************************************
 * IFSFldr_PersistFolder3_GetClassID
 */
static HRESULT WINAPI
IFSFldr_PersistFolder3_GetClassID (IPersistFolder3 * iface, CLSID * lpClassId)
{
    IGenericSFImpl *This = impl_from_IPersistFolder3(iface);

    TRACE ("(%p)\n", This);

    if (!lpClassId)
        return E_POINTER;
    *lpClassId = *This->pclsid;

    return S_OK;
}

/************************************************************************
 * IFSFldr_PersistFolder3_Initialize
 *
 * NOTES
 *  sPathTarget is not set. Don't know how to handle in a non rooted environment.
 */
static HRESULT WINAPI
IFSFldr_PersistFolder3_Initialize (IPersistFolder3 * iface, LPCITEMIDLIST pidl)
{
    WCHAR wszTemp[MAX_PATH];

    IGenericSFImpl *This = impl_from_IPersistFolder3(iface);

    TRACE ("(%p)->(%p)\n", This, pidl);

    SHFree (This->pidlRoot);     /* free the old pidl */
    This->pidlRoot = ILClone (pidl); /* set my pidl */

    SHFree (This->sPathTarget);
    This->sPathTarget = NULL;

    /* set my path */
    if (SHGetPathFromIDListW (pidl, wszTemp)) {
        int len = strlenW(wszTemp);
        This->sPathTarget = SHAlloc((len + 1) * sizeof(WCHAR));
        if (!This->sPathTarget)
            return E_OUTOFMEMORY;
        memcpy(This->sPathTarget, wszTemp, (len + 1) * sizeof(WCHAR));
    }

    TRACE ("--(%p)->(%s)\n", This, debugstr_w(This->sPathTarget));
    return S_OK;
}

/**************************************************************************
 * IFSFldr_PersistFolder3_GetCurFolder
 */
static HRESULT WINAPI
IFSFldr_PersistFolder3_fnGetCurFolder (IPersistFolder3 * iface,
                                       LPITEMIDLIST * pidl)
{
    IGenericSFImpl *This = impl_from_IPersistFolder3(iface);

    TRACE ("(%p)->(%p)\n", This, pidl);

    if (!pidl) return E_POINTER;
    *pidl = ILClone (This->pidlRoot);
    return S_OK;
}

/**************************************************************************
 * IFSFldr_PersistFolder3_InitializeEx
 *
 * FIXME: error handling
 */
static HRESULT WINAPI
IFSFldr_PersistFolder3_InitializeEx (IPersistFolder3 * iface,
                                     IBindCtx * pbc, LPCITEMIDLIST pidlRoot,
                                     const PERSIST_FOLDER_TARGET_INFO * ppfti)
{
    WCHAR wszTemp[MAX_PATH];

    IGenericSFImpl *This = impl_from_IPersistFolder3(iface);

    TRACE ("(%p)->(%p,%p,%p)\n", This, pbc, pidlRoot, ppfti);
    if (ppfti)
        TRACE ("--%p %s %s 0x%08x 0x%08x\n",
         ppfti->pidlTargetFolder, debugstr_w (ppfti->szTargetParsingName),
         debugstr_w (ppfti->szNetworkProvider), ppfti->dwAttributes,
         ppfti->csidl);

    pdump (pidlRoot);
    if (ppfti && ppfti->pidlTargetFolder)
        pdump (ppfti->pidlTargetFolder);

    if (This->pidlRoot)
        __SHFreeAndNil (&This->pidlRoot);    /* free the old */
    if (This->sPathTarget)
        __SHFreeAndNil (&This->sPathTarget);

    /*
     * Root path and pidl
     */
    This->pidlRoot = ILClone (pidlRoot);

    /*
     *  the target folder is spezified in csidl OR pidlTargetFolder OR
     *  szTargetParsingName
     */
    if (ppfti) {
        if (ppfti->csidl != -1) {
            if (SHGetSpecialFolderPathW (0, wszTemp, ppfti->csidl,
             ppfti->csidl & CSIDL_FLAG_CREATE)) {
                int len = strlenW(wszTemp);
                This->sPathTarget = SHAlloc((len + 1) * sizeof(WCHAR));
                if (!This->sPathTarget)
                    return E_OUTOFMEMORY;
                memcpy(This->sPathTarget, wszTemp, (len + 1) * sizeof(WCHAR));
            }
        } else if (ppfti->szTargetParsingName[0]) {
            int len = strlenW(ppfti->szTargetParsingName);
            This->sPathTarget = SHAlloc((len + 1) * sizeof(WCHAR));
            if (!This->sPathTarget)
                return E_OUTOFMEMORY;
            memcpy(This->sPathTarget, ppfti->szTargetParsingName,
                   (len + 1) * sizeof(WCHAR));
        } else if (ppfti->pidlTargetFolder) {
            if (SHGetPathFromIDListW(ppfti->pidlTargetFolder, wszTemp)) {
                int len = strlenW(wszTemp);
                This->sPathTarget = SHAlloc((len + 1) * sizeof(WCHAR));
                if (!This->sPathTarget)
                    return E_OUTOFMEMORY;
                memcpy(This->sPathTarget, wszTemp, (len + 1) * sizeof(WCHAR));
            }
        }
    }

    TRACE ("--(%p)->(target=%s)\n", This, debugstr_w(This->sPathTarget));
    pdump (This->pidlRoot);
    return (This->sPathTarget) ? S_OK : E_FAIL;
}

static HRESULT WINAPI
IFSFldr_PersistFolder3_GetFolderTargetInfo (IPersistFolder3 * iface,
                                            PERSIST_FOLDER_TARGET_INFO * ppfti)
{
    IGenericSFImpl *This = impl_from_IPersistFolder3(iface);
    FIXME ("(%p)->(%p)\n", This, ppfti);
    ZeroMemory (ppfti, sizeof (ppfti));
    return E_NOTIMPL;
}

static const IPersistFolder3Vtbl vt_FSFldr_PersistFolder3 =
{
    IFSFldr_PersistFolder3_QueryInterface,
    IFSFldr_PersistFolder3_AddRef,
    IFSFldr_PersistFolder3_Release,
    IFSFldr_PersistFolder3_GetClassID,
    IFSFldr_PersistFolder3_Initialize,
    IFSFldr_PersistFolder3_fnGetCurFolder,
    IFSFldr_PersistFolder3_InitializeEx,
    IFSFldr_PersistFolder3_GetFolderTargetInfo
};

/****************************************************************************
 * ISFDropTarget implementation
 */
static BOOL
ISFDropTarget_QueryDrop (IDropTarget * iface, DWORD dwKeyState,
                         LPDWORD pdwEffect)
{
    DWORD dwEffect = *pdwEffect;

    IGenericSFImpl *This = impl_from_IDropTarget(iface);

    *pdwEffect = DROPEFFECT_NONE;

    if (This->fAcceptFmt) { /* Does our interpretation of the keystate ... */
        *pdwEffect = KeyStateToDropEffect (dwKeyState);

        /* ... matches the desired effect ? */
        if (dwEffect & *pdwEffect) {
            return TRUE;
        }
    }
    return FALSE;
}

static HRESULT WINAPI
ISFDropTarget_QueryInterface (IDropTarget * iface, REFIID riid, LPVOID * ppvObj)
{
    IGenericSFImpl *This = impl_from_IDropTarget(iface);

    TRACE ("(%p)\n", This);

    return IUnknown_QueryInterface (This->pUnkOuter, riid, ppvObj);
}

static ULONG WINAPI ISFDropTarget_AddRef (IDropTarget * iface)
{
    IGenericSFImpl *This = impl_from_IDropTarget(iface);

    TRACE ("(%p)\n", This);

    return IUnknown_AddRef (This->pUnkOuter);
}

static ULONG WINAPI ISFDropTarget_Release (IDropTarget * iface)
{
    IGenericSFImpl *This = impl_from_IDropTarget(iface);

    TRACE ("(%p)\n", This);

    return IUnknown_Release (This->pUnkOuter);
}

static HRESULT WINAPI
ISFDropTarget_DragEnter (IDropTarget * iface, IDataObject * pDataObject,
                         DWORD dwKeyState, POINTL pt, DWORD * pdwEffect)
{
    FORMATETC fmt;

    IGenericSFImpl *This = impl_from_IDropTarget(iface);

    TRACE ("(%p)->(DataObject=%p)\n", This, pDataObject);

    InitFormatEtc (fmt, This->cfShellIDList, TYMED_HGLOBAL);

    This->fAcceptFmt = (S_OK == IDataObject_QueryGetData (pDataObject, &fmt)) ?
     TRUE : FALSE;

    ISFDropTarget_QueryDrop (iface, dwKeyState, pdwEffect);

    return S_OK;
}

static HRESULT WINAPI
ISFDropTarget_DragOver (IDropTarget * iface, DWORD dwKeyState, POINTL pt,
                        DWORD * pdwEffect)
{
    IGenericSFImpl *This = impl_from_IDropTarget(iface);

    TRACE ("(%p)\n", This);

    if (!pdwEffect)
        return E_INVALIDARG;

    ISFDropTarget_QueryDrop (iface, dwKeyState, pdwEffect);

    return S_OK;
}

static HRESULT WINAPI ISFDropTarget_DragLeave (IDropTarget * iface)
{
    IGenericSFImpl *This = impl_from_IDropTarget(iface);

    TRACE ("(%p)\n", This);

    This->fAcceptFmt = FALSE;

    return S_OK;
}

static HRESULT WINAPI
ISFDropTarget_Drop (IDropTarget * iface, IDataObject * pDataObject,
                    DWORD dwKeyState, POINTL pt, DWORD * pdwEffect)
{
    IGenericSFImpl *This = impl_from_IDropTarget(iface);

    FIXME ("(%p) object dropped\n", This);

    return E_NOTIMPL;
}

static const IDropTargetVtbl dtvt = {
    ISFDropTarget_QueryInterface,
    ISFDropTarget_AddRef,
    ISFDropTarget_Release,
    ISFDropTarget_DragEnter,
    ISFDropTarget_DragOver,
    ISFDropTarget_DragLeave,
    ISFDropTarget_Drop
};


/*
 *	file system folder
 *
 *	Copyright 1997			Marcus Meissner
 *	Copyright 1998, 1999, 2002	Juergen Schmied
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

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
    ICOM_VFIELD (IUnknown);
    DWORD ref;
    ICOM_VTABLE (IShellFolder2) * lpvtblShellFolder;
    ICOM_VTABLE (IPersistFolder3) * lpvtblPersistFolder3;
    ICOM_VTABLE (IDropTarget) * lpvtblDropTarget;
    ICOM_VTABLE (ISFHelper) * lpvtblSFHelper;

    IUnknown *pUnkOuter;	/* used for aggregation */

    CLSID *pclsid;

    /* both paths are parsible from the desktop */
    LPSTR sPathTarget;		/* complete path to target used for enumeration and ChangeNotify */

    LPITEMIDLIST pidlRoot;	/* absolute pidl */

    int dwAttributes;		/* attributes returned by GetAttributesOf FIXME: use it */

    UINT cfShellIDList;		/* clipboardformat for IDropTarget */
    BOOL fAcceptFmt;		/* flag for pending Drop */
} IGenericSFImpl;

static struct ICOM_VTABLE (IUnknown) unkvt;
static struct ICOM_VTABLE (IShellFolder2) sfvt;
static struct ICOM_VTABLE (IPersistFolder3) vt_FSFldr_PersistFolder3;	/* IPersistFolder3 for a FS_Folder */
static struct ICOM_VTABLE (IDropTarget) dtvt;
static struct ICOM_VTABLE (ISFHelper) shvt;

#define _IShellFolder2_Offset ((int)(&(((IGenericSFImpl*)0)->lpvtblShellFolder)))
#define _ICOM_THIS_From_IShellFolder2(class, name) class* This = (class*)(((char*)name)-_IShellFolder2_Offset);

#define _IPersistFolder2_Offset ((int)(&(((IGenericSFImpl*)0)->lpvtblPersistFolder3)))
#define _ICOM_THIS_From_IPersistFolder2(class, name) class* This = (class*)(((char*)name)-_IPersistFolder2_Offset);

#define _IPersistFolder3_Offset ((int)(&(((IGenericSFImpl*)0)->lpvtblPersistFolder3)))
#define _ICOM_THIS_From_IPersistFolder3(class, name) class* This = (class*)(((char*)name)-_IPersistFolder3_Offset);

#define _IDropTarget_Offset ((int)(&(((IGenericSFImpl*)0)->lpvtblDropTarget)))
#define _ICOM_THIS_From_IDropTarget(class, name) class* This = (class*)(((char*)name)-_IDropTarget_Offset);

#define _ISFHelper_Offset ((int)(&(((IGenericSFImpl*)0)->lpvtblSFHelper)))
#define _ICOM_THIS_From_ISFHelper(class, name) class* This = (class*)(((char*)name)-_ISFHelper_Offset);

/*
  converts This to a interface pointer
*/
#define _IUnknown_(This)	(IUnknown*)&(This->lpVtbl)
#define _IShellFolder_(This)	(IShellFolder*)&(This->lpvtblShellFolder)
#define _IShellFolder2_(This)	(IShellFolder2*)&(This->lpvtblShellFolder)
#define _IPersist_(This)	(IPersist*)&(This->lpvtblPersistFolder3)
#define _IPersistFolder_(This)	(IPersistFolder*)&(This->lpvtblPersistFolder3)
#define _IPersistFolder2_(This)	(IPersistFolder2*)&(This->lpvtblPersistFolder3)
#define _IPersistFolder3_(This)	(IPersistFolder3*)&(This->lpvtblPersistFolder3)
#define _IDropTarget_(This)	(IDropTarget*)&(This->lpvtblDropTarget)
#define _ISFHelper_(This)	(ISFHelper*)&(This->lpvtblSFHelper)

/**************************************************************************
*	registers clipboardformat once
*/
static void SF_RegisterClipFmt (IGenericSFImpl * This)
{
    TRACE ("(%p)\n", This);

    if (!This->cfShellIDList) {
	This->cfShellIDList = RegisterClipboardFormatA (CFSTR_SHELLIDLIST);
    }
}

/**************************************************************************
*	we need a separate IUnknown to handle aggregation
*	(inner IUnknown)
*/
static HRESULT WINAPI IUnknown_fnQueryInterface (IUnknown * iface, REFIID riid, LPVOID * ppvObj)
{
    ICOM_THIS (IGenericSFImpl, iface);

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
    ICOM_THIS (IGenericSFImpl, iface);

    TRACE ("(%p)->(count=%lu)\n", This, This->ref);

    return ++(This->ref);
}

static ULONG WINAPI IUnknown_fnRelease (IUnknown * iface)
{
    ICOM_THIS (IGenericSFImpl, iface);

    TRACE ("(%p)->(count=%lu)\n", This, This->ref);

    if (!--(This->ref)) {
	TRACE ("-- destroying IShellFolder(%p)\n", This);

	if (This->pidlRoot)
	    SHFree (This->pidlRoot);
	if (This->sPathTarget)
	    SHFree (This->sPathTarget);
	LocalFree ((HLOCAL) This);
	return 0;
    }
    return This->ref;
}

static ICOM_VTABLE (IUnknown) unkvt =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE IUnknown_fnQueryInterface,
      IUnknown_fnAddRef,
      IUnknown_fnRelease,
};

static shvheader GenericSFHeader[] = {
    {IDS_SHV_COLUMN1, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},
    {IDS_SHV_COLUMN2, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN3, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN4, SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 12},
    {IDS_SHV_COLUMN5, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 5}
};

#define GENERICSHELLVIEWCOLUMNS 5

/**************************************************************************
*	IFSFolder_Constructor
*
* NOTES
*  creating undocumented ShellFS_Folder as part of an aggregation
*  {F3364BA0-65B9-11CE-A9BA-00AA004AE837}
*
*/
HRESULT WINAPI IFSFolder_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv)
{
    IGenericSFImpl *sf;

    TRACE ("unkOut=%p %s\n", pUnkOuter, shdebugstr_guid (riid));

    if (pUnkOuter && !IsEqualIID (riid, &IID_IUnknown))
	return CLASS_E_NOAGGREGATION;
    sf = (IGenericSFImpl *) LocalAlloc (GMEM_ZEROINIT, sizeof (IGenericSFImpl));
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
 *  REFIID riid		[in ] Requested InterfaceID
 *  LPVOID* ppvObject	[out] Interface* to hold the result
 */
static HRESULT WINAPI IShellFolder_fnQueryInterface (IShellFolder2 * iface, REFIID riid, LPVOID * ppvObj)
{
    _ICOM_THIS_From_IShellFolder2 (IGenericSFImpl, iface)

	TRACE ("(%p)->(%s,%p)\n", This, shdebugstr_guid (riid), ppvObj);

    return IUnknown_QueryInterface (This->pUnkOuter, riid, ppvObj);
}

/**************************************************************************
*  IShellFolder_AddRef
*/

static ULONG WINAPI IShellFolder_fnAddRef (IShellFolder2 * iface)
{
    _ICOM_THIS_From_IShellFolder2 (IGenericSFImpl, iface)

    TRACE ("(%p)->(count=%lu)\n", This, This->ref);

    return IUnknown_AddRef (This->pUnkOuter);
}

/**************************************************************************
 *  IShellFolder_fnRelease
 */
static ULONG WINAPI IShellFolder_fnRelease (IShellFolder2 * iface)
{
    _ICOM_THIS_From_IShellFolder2 (IGenericSFImpl, iface)

    TRACE ("(%p)->(count=%lu)\n", This, This->ref);

    return IUnknown_Release (This->pUnkOuter);
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
*  Behaviour on win98:	lpszDisplayName=NULL -> crash
*			lpszDisplayName="" -> returns mycoputer-pidl
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
				 DWORD * pchEaten, LPITEMIDLIST * ppidl, DWORD * pdwAttributes)
{
    _ICOM_THIS_From_IShellFolder2 (IGenericSFImpl, iface)

    HRESULT hr = E_INVALIDARG;
    LPCWSTR szNext = NULL;
    WCHAR szElement[MAX_PATH];
    CHAR szPath[MAX_PATH];
    LPITEMIDLIST pidlTemp = NULL;
    DWORD len;

    TRACE ("(%p)->(HWND=%p,%p,%p=%s,%p,pidl=%p,%p)\n",
	   This, hwndOwner, pbc, lpszDisplayName, debugstr_w (lpszDisplayName), pchEaten, ppidl, pdwAttributes);

    if (!lpszDisplayName || !ppidl)
	return E_INVALIDARG;

    if (pchEaten)
	*pchEaten = 0;		/* strange but like the original */

    if (*lpszDisplayName) {
	/* get the next element */
	szNext = GetNextElementW (lpszDisplayName, szElement, MAX_PATH);

	/* build the full pathname to the element */
	lstrcpyA(szPath, This->sPathTarget);
	PathAddBackslashA(szPath);
	len = lstrlenA(szPath);
	WideCharToMultiByte(CP_ACP, 0, szElement, -1, szPath + len, MAX_PATH - len, NULL, NULL);

	/* get the pidl */
	pidlTemp = _ILCreateFromPathA(szPath);
	if (pidlTemp) {
	    if (szNext && *szNext) {
		/* try to analyse the next element */
		hr = SHELL32_ParseNextElement (iface, hwndOwner, pbc, &pidlTemp, (LPOLESTR) szNext, pchEaten, pdwAttributes);
	    } else {
		/* it's the last element */
		if (pdwAttributes && *pdwAttributes) {
		    SHELL32_GetItemAttributes (_IShellFolder_ (This), pidlTemp, pdwAttributes);
		}
		hr = S_OK;
	    }
	}
    }

    if (!hr)
	*ppidl = pidlTemp;
    else
	*ppidl = NULL;

    TRACE ("(%p)->(-- pidl=%p ret=0x%08lx)\n", This, ppidl ? *ppidl : 0, hr);

    return hr;
}

/**************************************************************************
*		IShellFolder_fnEnumObjects
* PARAMETERS
*  HWND          hwndOwner,    //[in ] Parent Window
*  DWORD         grfFlags,     //[in ] SHCONTF enumeration mask
*  LPENUMIDLIST* ppenumIDList  //[out] IEnumIDList interface
*/
static HRESULT WINAPI
IShellFolder_fnEnumObjects (IShellFolder2 * iface, HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST * ppEnumIDList)
{
    _ICOM_THIS_From_IShellFolder2 (IGenericSFImpl, iface)

    TRACE ("(%p)->(HWND=%p flags=0x%08lx pplist=%p)\n", This, hwndOwner, dwFlags, ppEnumIDList);

    *ppEnumIDList = IEnumIDList_Constructor();
    if (*ppEnumIDList)
        CreateFolderEnumList(*ppEnumIDList, This->sPathTarget, dwFlags);

    TRACE ("-- (%p)->(new ID List: %p)\n", This, *ppEnumIDList);

    return *ppEnumIDList ? S_OK : E_OUTOFMEMORY;
}

/**************************************************************************
*		IShellFolder_fnBindToObject
* PARAMETERS
*  LPCITEMIDLIST pidl,       //[in ] relative pidl to open
*  LPBC          pbc,        //[in ] optional FileSystemBindData context
*  REFIID        riid,       //[in ] Initial Interface
*  LPVOID*       ppvObject   //[out] Interface*
*/
static HRESULT WINAPI
IShellFolder_fnBindToObject (IShellFolder2 * iface, LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, LPVOID * ppvOut)
{
    _ICOM_THIS_From_IShellFolder2 (IGenericSFImpl, iface)

    TRACE ("(%p)->(pidl=%p,%p,%s,%p)\n", This, pidl, pbc, shdebugstr_guid (riid), ppvOut);

    return SHELL32_BindToChild (This->pidlRoot, This->sPathTarget, pidl, riid, ppvOut);
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
IShellFolder_fnBindToStorage (IShellFolder2 * iface, LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    _ICOM_THIS_From_IShellFolder2 (IGenericSFImpl, iface)

    FIXME ("(%p)->(pidl=%p,%p,%s,%p) stub\n", This, pidl, pbcReserved, shdebugstr_guid (riid), ppvOut);

    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
*  IShellFolder_fnCompareIDs
*/

static HRESULT WINAPI
IShellFolder_fnCompareIDs (IShellFolder2 * iface, LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    _ICOM_THIS_From_IShellFolder2 (IGenericSFImpl, iface)

    int nReturn;

    TRACE ("(%p)->(0x%08lx,pidl1=%p,pidl2=%p)\n", This, lParam, pidl1, pidl2);
    nReturn = SHELL32_CompareIDs (_IShellFolder_ (This), lParam, pidl1, pidl2);
    TRACE ("-- %i\n", nReturn);
    return nReturn;
}

/**************************************************************************
*	IShellFolder_fnCreateViewObject
*/
static HRESULT WINAPI
IShellFolder_fnCreateViewObject (IShellFolder2 * iface, HWND hwndOwner, REFIID riid, LPVOID * ppvOut)
{
    _ICOM_THIS_From_IShellFolder2 (IGenericSFImpl, iface)

    LPSHELLVIEW pShellView;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(hwnd=%p,%s,%p)\n", This, hwndOwner, shdebugstr_guid (riid), ppvOut);

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
IShellFolder_fnGetAttributesOf (IShellFolder2 * iface, UINT cidl, LPCITEMIDLIST * apidl, DWORD * rgfInOut)
{
    _ICOM_THIS_From_IShellFolder2 (IGenericSFImpl, iface)

    HRESULT hr = S_OK;

    TRACE ("(%p)->(cidl=%d apidl=%p mask=0x%08lx)\n", This, cidl, apidl, *rgfInOut);

    if ((!cidl) || (!apidl) || (!rgfInOut))
	return E_INVALIDARG;

    while (cidl > 0 && *apidl) {
	pdump (*apidl);
	SHELL32_GetItemAttributes (_IShellFolder_ (This), *apidl, rgfInOut);
	apidl++;
	cidl--;
    }

    TRACE ("-- result=0x%08lx\n", *rgfInOut);

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
*  This function gets asked to return "view objects" for one or more (multiple select)
*  items:
*  The viewobject typically is an COM object with one of the following interfaces:
*  IExtractIcon,IDataObject,IContextMenu
*  In order to support icon positions in the default Listview your DataObject
*  must implement the SetData method (in addition to GetData :) - the shell passes
*  a barely documented "Icon positions" structure to SetData when the drag starts,
*  and GetData's it if the drop is in another explorer window that needs the positions.
*/
static HRESULT WINAPI
IShellFolder_fnGetUIObjectOf (IShellFolder2 * iface,
			      HWND hwndOwner,
			      UINT cidl, LPCITEMIDLIST * apidl, REFIID riid, UINT * prgfInOut, LPVOID * ppvOut)
{
    _ICOM_THIS_From_IShellFolder2 (IGenericSFImpl, iface)

    LPITEMIDLIST pidl;
    IUnknown *pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(%p,%u,apidl=%p,%s,%p,%p)\n",
	   This, hwndOwner, cidl, apidl, shdebugstr_guid (riid), prgfInOut, ppvOut);

    if (ppvOut) {
	*ppvOut = NULL;

	if (IsEqualIID (riid, &IID_IContextMenu) && (cidl >= 1)) {
	    pObj = (LPUNKNOWN) ISvItemCm_Constructor ((IShellFolder *) iface, This->pidlRoot, apidl, cidl);
	    hr = S_OK;
	} else if (IsEqualIID (riid, &IID_IDataObject) && (cidl >= 1)) {
	    pObj = (LPUNKNOWN) IDataObject_Constructor (hwndOwner, This->pidlRoot, apidl, cidl);
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
	    hr = IShellFolder_QueryInterface (iface, &IID_IDropTarget, (LPVOID *) & pObj);
	} else if ((IsEqualIID(riid,&IID_IShellLinkW) || IsEqualIID(riid,&IID_IShellLinkA))
				&& (cidl == 1)) {
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
    TRACE ("(%p)->hr=0x%08lx\n", This, hr);
    return hr;
}

void SHELL_FS_ProcessDisplayFilename(LPSTR szPath, DWORD dwFlags)
{
    /*FIXME: MSDN also mentions SHGDN_FOREDITING which is not yet handled. */
    if (!(dwFlags & SHGDN_FORPARSING) &&
	((dwFlags & SHGDN_INFOLDER) || (dwFlags == SHGDN_NORMAL))) {
	HKEY hKey;
	DWORD dwData;
	DWORD dwDataSize = sizeof (DWORD);
	BOOL doHide = FALSE;	/* The default value is FALSE (win98 at least) */

	if (!RegCreateKeyExA (HKEY_CURRENT_USER,
			      "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
			      0, 0, 0, KEY_ALL_ACCESS, 0, &hKey, 0)) {
	    if (!RegQueryValueExA (hKey, "HideFileExt", 0, 0, (LPBYTE) & dwData, &dwDataSize))
		doHide = dwData;

	    RegCloseKey (hKey);
	}

	if (!doHide) {
	    LPSTR ext = PathFindExtensionA(szPath);

	    if (ext) {
		HKEY hkey;
		char classname[MAX_PATH];
		LONG classlen = MAX_PATH;

		if (!RegQueryValueA(HKEY_CLASSES_ROOT, ext, classname, &classlen))
		    if (!RegOpenKeyA(HKEY_CLASSES_ROOT, classname, &hkey)) {
			if (!RegQueryValueExA(hkey, "NeverShowExt", 0, NULL, NULL, NULL))
			    doHide = TRUE;

			RegCloseKey(hkey);
		    }
	    }
	}

	if (doHide && szPath[0] != '.')
	    PathRemoveExtensionA (szPath);
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
IShellFolder_fnGetDisplayNameOf (IShellFolder2 * iface, LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET strRet)
{
    _ICOM_THIS_From_IShellFolder2 (IGenericSFImpl, iface)

    CHAR szPath[MAX_PATH];
    int len = 0;
    BOOL bSimplePidl;

    *szPath = '\0';

    TRACE ("(%p)->(pidl=%p,0x%08lx,%p)\n", This, pidl, dwFlags, strRet);
    pdump (pidl);

    if (!pidl || !strRet)
	return E_INVALIDARG;

    bSimplePidl = _ILIsPidlSimple (pidl);

    /* take names of special folders only if its only this folder */
    if (_ILIsSpecialFolder (pidl)) {
	if (bSimplePidl) {
	    _ILSimpleGetText (pidl, szPath, MAX_PATH);	/* append my own path */
	} else {
	    FIXME ("special pidl\n");
	}
    } else {
	if (!(dwFlags & SHGDN_INFOLDER) && (dwFlags & SHGDN_FORPARSING) && This->sPathTarget) {
	    lstrcpyA (szPath, This->sPathTarget);	/* get path to root */
	    PathAddBackslashA (szPath);
	    len = lstrlenA (szPath);
	}
	_ILSimpleGetText (pidl, szPath + len, MAX_PATH - len);	/* append my own path */

	if (!_ILIsFolder(pidl))
	    SHELL_FS_ProcessDisplayFilename(szPath, dwFlags);
    }

    if ((dwFlags & SHGDN_FORPARSING) && !bSimplePidl) {	/* go deeper if needed */
	PathAddBackslashA (szPath);
	len = lstrlenA (szPath);

	if (!SUCCEEDED
	    (SHELL32_GetDisplayNameOfChild (iface, pidl, dwFlags | SHGDN_INFOLDER, szPath + len, MAX_PATH - len)))
	    return E_OUTOFMEMORY;
    }
    strRet->uType = STRRET_CSTR;
    lstrcpynA (strRet->u.cStr, szPath, MAX_PATH);

    TRACE ("-- (%p)->(%s)\n", This, szPath);
    return S_OK;
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
static HRESULT WINAPI IShellFolder_fnSetNameOf (IShellFolder2 * iface, HWND hwndOwner, LPCITEMIDLIST pidl,	/*simple pidl */
						LPCOLESTR lpName, DWORD dwFlags, LPITEMIDLIST * pPidlOut)
{
    _ICOM_THIS_From_IShellFolder2 (IGenericSFImpl, iface)
    char szSrc[MAX_PATH],
      szDest[MAX_PATH];
    int len;
    BOOL bIsFolder = _ILIsFolder (ILFindLastID (pidl));

    TRACE ("(%p)->(%p,pidl=%p,%s,%lu,%p)\n", This, hwndOwner, pidl, debugstr_w (lpName), dwFlags, pPidlOut);

    /* build source path */
    if (dwFlags & SHGDN_INFOLDER) {
	strcpy (szSrc, This->sPathTarget);
	PathAddBackslashA (szSrc);
	len = strlen (szSrc);
	_ILSimpleGetText (pidl, szSrc + len, MAX_PATH - len);
    } else {
	/* FIXME: Can this work with a simple PIDL? */
	SHGetPathFromIDListA (pidl, szSrc);
    }

    /* build destination path */
    strcpy (szDest, This->sPathTarget);
    PathAddBackslashA (szDest);
    len = strlen (szDest);
    WideCharToMultiByte (CP_ACP, 0, lpName, -1, szDest + len, MAX_PATH - len, NULL, NULL);
    szDest[MAX_PATH - 1] = 0;
    TRACE ("src=%s dest=%s\n", szSrc, szDest);
    if (MoveFileA (szSrc, szDest)) {
	if (pPidlOut)
	    *pPidlOut = _ILCreateFromPathA(szDest);
	SHChangeNotify (bIsFolder ? SHCNE_RENAMEFOLDER : SHCNE_RENAMEITEM, SHCNF_PATHA, szSrc, szDest);
	return S_OK;
    }
    return E_FAIL;
}

static HRESULT WINAPI IShellFolder_fnGetDefaultSearchGUID (IShellFolder2 * iface, GUID * pguid)
{
    _ICOM_THIS_From_IShellFolder2 (IGenericSFImpl, iface)
	FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}
static HRESULT WINAPI IShellFolder_fnEnumSearches (IShellFolder2 * iface, IEnumExtraSearch ** ppenum)
{
    _ICOM_THIS_From_IShellFolder2 (IGenericSFImpl, iface)
	FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}
static HRESULT WINAPI
IShellFolder_fnGetDefaultColumn (IShellFolder2 * iface, DWORD dwRes, ULONG * pSort, ULONG * pDisplay)
{
    _ICOM_THIS_From_IShellFolder2 (IGenericSFImpl, iface)

	TRACE ("(%p)\n", This);

    if (pSort)
	*pSort = 0;
    if (pDisplay)
	*pDisplay = 0;

    return S_OK;
}
static HRESULT WINAPI IShellFolder_fnGetDefaultColumnState (IShellFolder2 * iface, UINT iColumn, DWORD * pcsFlags)
{
    _ICOM_THIS_From_IShellFolder2 (IGenericSFImpl, iface)

	TRACE ("(%p)\n", This);

    if (!pcsFlags || iColumn >= GENERICSHELLVIEWCOLUMNS)
	return E_INVALIDARG;

    *pcsFlags = GenericSFHeader[iColumn].pcsFlags;

    return S_OK;
}
static HRESULT WINAPI
IShellFolder_fnGetDetailsEx (IShellFolder2 * iface, LPCITEMIDLIST pidl, const SHCOLUMNID * pscid, VARIANT * pv)
{
    _ICOM_THIS_From_IShellFolder2 (IGenericSFImpl, iface)
	FIXME ("(%p)\n", This);

    return E_NOTIMPL;
}
static HRESULT WINAPI
IShellFolder_fnGetDetailsOf (IShellFolder2 * iface, LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS * psd)
{
    _ICOM_THIS_From_IShellFolder2 (IGenericSFImpl, iface)
    HRESULT hr = E_FAIL;

    TRACE ("(%p)->(%p %i %p)\n", This, pidl, iColumn, psd);

    if (!psd || iColumn >= GENERICSHELLVIEWCOLUMNS)
	return E_INVALIDARG;

    if (!pidl) {
	/* the header titles */
	psd->fmt = GenericSFHeader[iColumn].fmt;
	psd->cxChar = GenericSFHeader[iColumn].cxChar;
	psd->str.uType = STRRET_CSTR;
	LoadStringA (shell32_hInstance, GenericSFHeader[iColumn].colnameid, psd->str.u.cStr, MAX_PATH);
	return S_OK;
    } else {
	/* the data from the pidl */
	switch (iColumn) {
	case 0:		/* name */
	    hr = IShellFolder_GetDisplayNameOf (iface, pidl, SHGDN_NORMAL | SHGDN_INFOLDER, &psd->str);
	    break;
	case 1:		/* size */
	    _ILGetFileSize (pidl, psd->str.u.cStr, MAX_PATH);
	    break;
	case 2:		/* type */
	    _ILGetFileType (pidl, psd->str.u.cStr, MAX_PATH);
	    break;
	case 3:		/* date */
	    _ILGetFileDate (pidl, psd->str.u.cStr, MAX_PATH);
	    break;
	case 4:		/* attributes */
	    _ILGetFileAttributes (pidl, psd->str.u.cStr, MAX_PATH);
	    break;
	}
	hr = S_OK;
	psd->str.uType = STRRET_CSTR;
    }

    return hr;
}
static HRESULT WINAPI IShellFolder_fnMapColumnToSCID (IShellFolder2 * iface, UINT column, SHCOLUMNID * pscid)
{
    _ICOM_THIS_From_IShellFolder2 (IGenericSFImpl, iface)
    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}

static ICOM_VTABLE (IShellFolder2) sfvt =
{
        ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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

static HRESULT WINAPI ISFHelper_fnQueryInterface (ISFHelper * iface, REFIID riid, LPVOID * ppvObj)
{
    _ICOM_THIS_From_ISFHelper (IGenericSFImpl, iface);

    TRACE ("(%p)->(count=%lu)\n", This, This->ref);

    return IUnknown_QueryInterface (This->pUnkOuter, riid, ppvObj);
}

static ULONG WINAPI ISFHelper_fnAddRef (ISFHelper * iface)
{
    _ICOM_THIS_From_ISFHelper (IGenericSFImpl, iface);

    TRACE ("(%p)->(count=%lu)\n", This, This->ref);

    return IUnknown_AddRef (This->pUnkOuter);
}

static ULONG WINAPI ISFHelper_fnRelease (ISFHelper * iface)
{
    _ICOM_THIS_From_ISFHelper (IGenericSFImpl, iface);

    TRACE ("(%p)\n", This);

    return IUnknown_Release (This->pUnkOuter);
}

/****************************************************************************
 * ISFHelper_fnAddFolder
 *
 * creates a unique folder name
 */

static HRESULT WINAPI ISFHelper_fnGetUniqueName (ISFHelper * iface, LPSTR lpName, UINT uLen)
{
    _ICOM_THIS_From_ISFHelper (IGenericSFImpl, iface)
    IEnumIDList *penum;
    HRESULT hr;
    char szText[MAX_PATH];
    char *szNewFolder = "New Folder";

    TRACE ("(%p)(%s %u)\n", This, lpName, uLen);

    if (uLen < strlen (szNewFolder) + 4)
	return E_POINTER;

    strcpy (lpName, szNewFolder);

    hr = IShellFolder_fnEnumObjects (_IShellFolder2_ (This), 0,
				     SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN, &penum);
    if (penum) {
	LPITEMIDLIST pidl;
	DWORD dwFetched;
	int i = 1;

      next:IEnumIDList_Reset (penum);
	while (S_OK == IEnumIDList_Next (penum, 1, &pidl, &dwFetched) && dwFetched) {
	    _ILSimpleGetText (pidl, szText, MAX_PATH);
	    if (0 == strcasecmp (szText, lpName)) {
		sprintf (lpName, "%s %d", szNewFolder, i++);
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

static HRESULT WINAPI ISFHelper_fnAddFolder (ISFHelper * iface, HWND hwnd, LPCSTR lpName, LPITEMIDLIST * ppidlOut)
{
    _ICOM_THIS_From_ISFHelper (IGenericSFImpl, iface)
    char lpstrNewDir[MAX_PATH];
    DWORD bRes;
    HRESULT hres = E_FAIL;

    TRACE ("(%p)(%s %p)\n", This, lpName, ppidlOut);

    strcpy (lpstrNewDir, This->sPathTarget);
    PathAppendA(lpstrNewDir, lpName);

    bRes = CreateDirectoryA (lpstrNewDir, NULL);
    if (bRes) {
	SHChangeNotify (SHCNE_MKDIR, SHCNF_PATHA, lpstrNewDir, NULL);
	if (ppidlOut)
	    *ppidlOut = _ILCreateFromPathA(lpstrNewDir);
	hres = S_OK;
    } else {
	char lpstrText[128 + MAX_PATH];
	char lpstrTempText[128];
	char lpstrCaption[256];

	/* Cannot Create folder because of permissions */
	LoadStringA (shell32_hInstance, IDS_CREATEFOLDER_DENIED, lpstrTempText, sizeof (lpstrTempText));
	LoadStringA (shell32_hInstance, IDS_CREATEFOLDER_CAPTION, lpstrCaption, sizeof (lpstrCaption));
	sprintf (lpstrText, lpstrTempText, lpstrNewDir);
	MessageBoxA (hwnd, lpstrText, lpstrCaption, MB_OK | MB_ICONEXCLAMATION);
    }

    return hres;
}

/****************************************************************************
 * ISFHelper_fnDeleteItems
 *
 * deletes items in folder
 */
static HRESULT WINAPI ISFHelper_fnDeleteItems (ISFHelper * iface, UINT cidl, LPCITEMIDLIST * apidl)
{
    _ICOM_THIS_From_ISFHelper (IGenericSFImpl, iface)
    UINT i;
    char szPath[MAX_PATH];
    BOOL bConfirm = TRUE;

    TRACE ("(%p)(%u %p)\n", This, cidl, apidl);

    /* deleting multiple items so give a slightly different warning */
    if (cidl != 1) {
	char tmp[8];

	snprintf (tmp, sizeof (tmp), "%d", cidl);
	if (!SHELL_ConfirmDialog(ASK_DELETE_MULTIPLE_ITEM, tmp))
	    return E_FAIL;
	bConfirm = FALSE;
    }

    for (i = 0; i < cidl; i++) {
	strcpy (szPath, This->sPathTarget);
	PathAddBackslashA (szPath);
	_ILSimpleGetText (apidl[i], szPath + strlen (szPath), MAX_PATH);

	if (_ILIsFolder (apidl[i])) {
	    LPITEMIDLIST pidl;

	    TRACE ("delete %s\n", szPath);
	    if (!SHELL_DeleteDirectoryA (szPath, bConfirm)) {
		TRACE ("delete %s failed, bConfirm=%d\n", szPath, bConfirm);
		return E_FAIL;
	    }
	    pidl = ILCombine (This->pidlRoot, apidl[i]);
	    SHChangeNotify (SHCNE_RMDIR, SHCNF_IDLIST, pidl, NULL);
	    SHFree (pidl);
	} else if (_ILIsValue (apidl[i])) {
	    LPITEMIDLIST pidl;

	    TRACE ("delete %s\n", szPath);
	    if (!SHELL_DeleteFileA (szPath, bConfirm)) {
		TRACE ("delete %s failed, bConfirm=%d\n", szPath, bConfirm);
		return E_FAIL;
	    }
	    pidl = ILCombine (This->pidlRoot, apidl[i]);
	    SHChangeNotify (SHCNE_DELETE, SHCNF_IDLIST, pidl, NULL);
	    SHFree (pidl);
	}

    }
    return S_OK;
}

/****************************************************************************
 * ISFHelper_fnCopyItems
 *
 * copies items to this folder
 */
static HRESULT WINAPI
ISFHelper_fnCopyItems (ISFHelper * iface, IShellFolder * pSFFrom, UINT cidl, LPCITEMIDLIST * apidl)
{
    UINT i;
    IPersistFolder2 *ppf2 = NULL;
    char szSrcPath[MAX_PATH],
      szDstPath[MAX_PATH];

    _ICOM_THIS_From_ISFHelper (IGenericSFImpl, iface);

    TRACE ("(%p)->(%p,%u,%p)\n", This, pSFFrom, cidl, apidl);

    IShellFolder_QueryInterface (pSFFrom, &IID_IPersistFolder2, (LPVOID *) & ppf2);
    if (ppf2) {
	LPITEMIDLIST pidl;

	if (SUCCEEDED (IPersistFolder2_GetCurFolder (ppf2, &pidl))) {
	    for (i = 0; i < cidl; i++) {
		SHGetPathFromIDListA (pidl, szSrcPath);
		PathAddBackslashA (szSrcPath);
		_ILSimpleGetText (apidl[i], szSrcPath + strlen (szSrcPath), MAX_PATH);

		strcpy (szDstPath, This->sPathTarget);
		PathAddBackslashA (szDstPath);
		_ILSimpleGetText (apidl[i], szDstPath + strlen (szDstPath), MAX_PATH);
		MESSAGE ("would copy %s to %s\n", szSrcPath, szDstPath);
	    }
	    SHFree (pidl);
	}
	IPersistFolder2_Release (ppf2);
    }
    return S_OK;
}

static ICOM_VTABLE (ISFHelper) shvt =
{
        ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ISFHelper_fnQueryInterface,
	ISFHelper_fnAddRef,
	ISFHelper_fnRelease,
	ISFHelper_fnGetUniqueName,
	ISFHelper_fnAddFolder,
	ISFHelper_fnDeleteItems,
        ISFHelper_fnCopyItems
};

/************************************************************************
 *	IFSFldr_PersistFolder3_QueryInterface
 *
 */
static HRESULT WINAPI IFSFldr_PersistFolder3_QueryInterface (IPersistFolder3 * iface, REFIID iid, LPVOID * ppvObj)
{
    _ICOM_THIS_From_IPersistFolder3 (IGenericSFImpl, iface);

    TRACE ("(%p)\n", This);

    return IUnknown_QueryInterface (This->pUnkOuter, iid, ppvObj);
}

/************************************************************************
 *	IFSFldr_PersistFolder3_AddRef
 *
 */
static ULONG WINAPI IFSFldr_PersistFolder3_AddRef (IPersistFolder3 * iface)
{
    _ICOM_THIS_From_IPersistFolder3 (IGenericSFImpl, iface);

    TRACE ("(%p)->(count=%lu)\n", This, This->ref);

    return IUnknown_AddRef (This->pUnkOuter);
}

/************************************************************************
 *	IFSFldr_PersistFolder3_Release
 *
 */
static ULONG WINAPI IFSFldr_PersistFolder3_Release (IPersistFolder3 * iface)
{
    _ICOM_THIS_From_IPersistFolder3 (IGenericSFImpl, iface);

    TRACE ("(%p)->(count=%lu)\n", This, This->ref);

    return IUnknown_Release (This->pUnkOuter);
}

/************************************************************************
 *	IFSFldr_PersistFolder3_GetClassID
 */
static HRESULT WINAPI IFSFldr_PersistFolder3_GetClassID (IPersistFolder3 * iface, CLSID * lpClassId)
{
    _ICOM_THIS_From_IPersistFolder3 (IGenericSFImpl, iface);

    TRACE ("(%p)\n", This);

    if (!lpClassId)
	return E_POINTER;
    *lpClassId = *This->pclsid;

    return S_OK;
}

/************************************************************************
 *	IFSFldr_PersistFolder3_Initialize
 *
 * NOTES
 *  sPathTarget is not set. Don't know how to handle in a non rooted environment.
 */
static HRESULT WINAPI IFSFldr_PersistFolder3_Initialize (IPersistFolder3 * iface, LPCITEMIDLIST pidl)
{
    char sTemp[MAX_PATH];

    _ICOM_THIS_From_IPersistFolder3 (IGenericSFImpl, iface);

    TRACE ("(%p)->(%p)\n", This, pidl);

    if (This->pidlRoot)
	SHFree (This->pidlRoot);	/* free the old pidl */
    This->pidlRoot = ILClone (pidl);	/* set my pidl */

    if (This->sPathTarget)
	SHFree (This->sPathTarget);

    /* set my path */
    if (SHGetPathFromIDListA (pidl, sTemp)) {
	This->sPathTarget = SHAlloc (strlen (sTemp) + 1);
	strcpy (This->sPathTarget, sTemp);
    }

    TRACE ("--(%p)->(%s)\n", This, This->sPathTarget);
    return S_OK;
}

/**************************************************************************
 *	IFSFldr_PersistFolder3_GetCurFolder
 */
static HRESULT WINAPI IFSFldr_PersistFolder3_fnGetCurFolder (IPersistFolder3 * iface, LPITEMIDLIST * pidl)
{
    _ICOM_THIS_From_IPersistFolder3 (IGenericSFImpl, iface);

    TRACE ("(%p)->(%p)\n", This, pidl);

    if (!pidl) return E_POINTER;
    *pidl = ILClone (This->pidlRoot);
    return S_OK;
}

/**************************************************************************
 *	IFSFldr_PersistFolder3_InitializeEx
 *
 * FIXME: errorhandling
 */
static HRESULT WINAPI
IFSFldr_PersistFolder3_InitializeEx (IPersistFolder3 * iface,
				     IBindCtx * pbc, LPCITEMIDLIST pidlRoot, const PERSIST_FOLDER_TARGET_INFO * ppfti)
{
    char sTemp[MAX_PATH];

    _ICOM_THIS_From_IPersistFolder3 (IGenericSFImpl, iface);

    TRACE ("(%p)->(%p,%p,%p)\n", This, pbc, pidlRoot, ppfti);
    if (ppfti)
	TRACE ("--%p %s %s 0x%08lx 0x%08x\n",
	       ppfti->pidlTargetFolder, debugstr_w (ppfti->szTargetParsingName),
	       debugstr_w (ppfti->szNetworkProvider), ppfti->dwAttributes, ppfti->csidl);

    pdump (pidlRoot);
    if (ppfti && ppfti->pidlTargetFolder)
	pdump (ppfti->pidlTargetFolder);

    if (This->pidlRoot)
	__SHFreeAndNil (&This->pidlRoot);	/* free the old */
    if (This->sPathTarget)
	__SHFreeAndNil (&This->sPathTarget);

    /*
     * Root path and pidl
     */
    This->pidlRoot = ILClone (pidlRoot);

    /*
     *  the target folder is spezified in csidl OR pidlTargetFolder OR szTargetParsingName
     */
    if (ppfti) {
	if (ppfti->csidl != -1) {
	    if (SHGetSpecialFolderPathA (0, sTemp, ppfti->csidl, ppfti->csidl & CSIDL_FLAG_CREATE)) {
		__SHCloneStrA (&This->sPathTarget, sTemp);
	    }
	} else if (ppfti->szTargetParsingName[0]) {
	    __SHCloneStrWtoA (&This->sPathTarget, ppfti->szTargetParsingName);
	} else if (ppfti->pidlTargetFolder) {
	    if (SHGetPathFromIDListA (ppfti->pidlTargetFolder, sTemp)) {
		__SHCloneStrA (&This->sPathTarget, sTemp);
	    }
	}
    }

    TRACE ("--(%p)->(target=%s)\n", This, debugstr_a (This->sPathTarget));
    pdump (This->pidlRoot);
    return (This->sPathTarget) ? S_OK : E_FAIL;
}

static HRESULT WINAPI
IFSFldr_PersistFolder3_GetFolderTargetInfo (IPersistFolder3 * iface, PERSIST_FOLDER_TARGET_INFO * ppfti)
{
    _ICOM_THIS_From_IPersistFolder3 (IGenericSFImpl, iface);
    FIXME ("(%p)->(%p)\n", This, ppfti);
    ZeroMemory (ppfti, sizeof (ppfti));
    return E_NOTIMPL;
}

static ICOM_VTABLE (IPersistFolder3) vt_FSFldr_PersistFolder3 =
{
        ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
static BOOL ISFDropTarget_QueryDrop (IDropTarget * iface, DWORD dwKeyState, LPDWORD pdwEffect)
{
    DWORD dwEffect = *pdwEffect;

    _ICOM_THIS_From_IDropTarget (IGenericSFImpl, iface);

    *pdwEffect = DROPEFFECT_NONE;

    if (This->fAcceptFmt) {	/* Does our interpretation of the keystate ... */
	*pdwEffect = KeyStateToDropEffect (dwKeyState);

	/* ... matches the desired effect ? */
	if (dwEffect & *pdwEffect) {
	    return TRUE;
	}
    }
    return FALSE;
}

static HRESULT WINAPI ISFDropTarget_QueryInterface (IDropTarget * iface, REFIID riid, LPVOID * ppvObj)
{
    _ICOM_THIS_From_IDropTarget (IGenericSFImpl, iface);

    TRACE ("(%p)\n", This);

    return IUnknown_QueryInterface (This->pUnkOuter, riid, ppvObj);
}

static ULONG WINAPI ISFDropTarget_AddRef (IDropTarget * iface)
{
    _ICOM_THIS_From_IDropTarget (IGenericSFImpl, iface);

    TRACE ("(%p)\n", This);

    return IUnknown_AddRef (This->pUnkOuter);
}

static ULONG WINAPI ISFDropTarget_Release (IDropTarget * iface)
{
    _ICOM_THIS_From_IDropTarget (IGenericSFImpl, iface);

    TRACE ("(%p)\n", This);

    return IUnknown_Release (This->pUnkOuter);
}

static HRESULT WINAPI
ISFDropTarget_DragEnter (IDropTarget * iface, IDataObject * pDataObject, DWORD dwKeyState, POINTL pt, DWORD * pdwEffect)
{
    FORMATETC fmt;

    _ICOM_THIS_From_IDropTarget (IGenericSFImpl, iface);

    TRACE ("(%p)->(DataObject=%p)\n", This, pDataObject);

    InitFormatEtc (fmt, This->cfShellIDList, TYMED_HGLOBAL);

    This->fAcceptFmt = (S_OK == IDataObject_QueryGetData (pDataObject, &fmt)) ? TRUE : FALSE;

    ISFDropTarget_QueryDrop (iface, dwKeyState, pdwEffect);

    return S_OK;
}

static HRESULT WINAPI ISFDropTarget_DragOver (IDropTarget * iface, DWORD dwKeyState, POINTL pt, DWORD * pdwEffect)
{
    _ICOM_THIS_From_IDropTarget (IGenericSFImpl, iface);

    TRACE ("(%p)\n", This);

    if (!pdwEffect)
	return E_INVALIDARG;

    ISFDropTarget_QueryDrop (iface, dwKeyState, pdwEffect);

    return S_OK;
}

static HRESULT WINAPI ISFDropTarget_DragLeave (IDropTarget * iface)
{
    _ICOM_THIS_From_IDropTarget (IGenericSFImpl, iface);

    TRACE ("(%p)\n", This);

    This->fAcceptFmt = FALSE;

    return S_OK;
}

static HRESULT WINAPI
ISFDropTarget_Drop (IDropTarget * iface, IDataObject * pDataObject, DWORD dwKeyState, POINTL pt, DWORD * pdwEffect)
{
    _ICOM_THIS_From_IDropTarget (IGenericSFImpl, iface);

    FIXME ("(%p) object dropped\n", This);

    return E_NOTIMPL;
}

static struct ICOM_VTABLE (IDropTarget) dtvt = {
        ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ISFDropTarget_QueryInterface,
	ISFDropTarget_AddRef,
	ISFDropTarget_Release,
	ISFDropTarget_DragEnter,
	ISFDropTarget_DragOver,
	ISFDropTarget_DragLeave,
	ISFDropTarget_Drop
};

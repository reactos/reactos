
/*
 *	Virtual Workplace folder
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
#include "pidl.h"
#include "shlguid.h"

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
    ICOM_VFIELD (IShellFolder2);
    DWORD ref;
      ICOM_VTABLE (IPersistFolder2) * lpVtblPersistFolder2;

    /* both paths are parsible from the desktop */
    LPITEMIDLIST pidlRoot;	/* absolute pidl */
    int dwAttributes;		/* attributes returned by GetAttributesOf FIXME: use it */
} IGenericSFImpl;

static struct ICOM_VTABLE (IShellFolder2) vt_ShellFolder2;
static struct ICOM_VTABLE (IPersistFolder2) vt_PersistFolder2;

#define _IPersistFolder2_Offset ((int)(&(((IGenericSFImpl*)0)->lpVtblPersistFolder2)))
#define _ICOM_THIS_From_IPersistFolder2(class, name) class* This = (class*)(((char*)name)-_IPersistFolder2_Offset);

/*
  converts This to a interface pointer
*/
#define _IUnknown_(This)	(IUnknown*)&(This->lpVtbl)
#define _IShellFolder_(This)	(IShellFolder*)&(This->lpVtbl)
#define _IShellFolder2_(This)	(IShellFolder2*)&(This->lpVtbl)

#define _IPersist_(This)	(IPersist*)&(This->lpVtblPersistFolder2)
#define _IPersistFolder_(This)	(IPersistFolder*)&(This->lpVtblPersistFolder2)
#define _IPersistFolder2_(This)	(IPersistFolder2*)&(This->lpVtblPersistFolder2)

/***********************************************************************
*   IShellFolder [MyComputer] implementation
*/

static shvheader MyComputerSFHeader[] = {
    {IDS_SHV_COLUMN1, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},
    {IDS_SHV_COLUMN3, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN6, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN7, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
};

#define MYCOMPUTERSHELLVIEWCOLUMNS 4

/**************************************************************************
*	ISF_MyComputer_Constructor
*/
HRESULT WINAPI ISF_MyComputer_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv)
{
    IGenericSFImpl *sf;

    TRACE ("unkOut=%p %s\n", pUnkOuter, shdebugstr_guid (riid));

    if (!ppv)
	return E_POINTER;
    if (pUnkOuter)
	return CLASS_E_NOAGGREGATION;

    sf = (IGenericSFImpl *) LocalAlloc (GMEM_ZEROINIT, sizeof (IGenericSFImpl));
    if (!sf)
	return E_OUTOFMEMORY;

    sf->ref = 0;
    sf->lpVtbl = &vt_ShellFolder2;
    sf->lpVtblPersistFolder2 = &vt_PersistFolder2;
    sf->pidlRoot = _ILCreateMyComputer ();	/* my qualified pidl */

    if (!SUCCEEDED (IUnknown_QueryInterface (_IUnknown_ (sf), riid, ppv))) {
	IUnknown_Release (_IUnknown_ (sf));
	return E_NOINTERFACE;
    }

    TRACE ("--(%p)\n", sf);
    return S_OK;
}

/**************************************************************************
 *	ISF_MyComputer_fnQueryInterface
 *
 * NOTES supports not IPersist/IPersistFolder
 */
static HRESULT WINAPI ISF_MyComputer_fnQueryInterface (IShellFolder2 * iface, REFIID riid, LPVOID * ppvObj)
{
    ICOM_THIS (IGenericSFImpl, iface);

    TRACE ("(%p)->(%s,%p)\n", This, shdebugstr_guid (riid), ppvObj);

    *ppvObj = NULL;

    if (IsEqualIID (riid, &IID_IUnknown) ||
	IsEqualIID (riid, &IID_IShellFolder) || IsEqualIID (riid, &IID_IShellFolder2)) {
	*ppvObj = This;
    } else if (IsEqualIID (riid, &IID_IPersist) ||
	       IsEqualIID (riid, &IID_IPersistFolder) || IsEqualIID (riid, &IID_IPersistFolder2)) {
	*ppvObj = _IPersistFolder2_ (This);
    }

    if (*ppvObj) {
	IUnknown_AddRef ((IUnknown *) (*ppvObj));
	TRACE ("-- Interface: (%p)->(%p)\n", ppvObj, *ppvObj);
	return S_OK;
    }
    TRACE ("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ISF_MyComputer_fnAddRef (IShellFolder2 * iface)
{
    ICOM_THIS (IGenericSFImpl, iface);

    TRACE ("(%p)->(count=%lu)\n", This, This->ref);

    return ++(This->ref);
}

static ULONG WINAPI ISF_MyComputer_fnRelease (IShellFolder2 * iface)
{
    ICOM_THIS (IGenericSFImpl, iface);

    TRACE ("(%p)->(count=%lu)\n", This, This->ref);

    if (!--(This->ref)) {
        TRACE ("-- destroying IShellFolder(%p)\n", This);
        if (This->pidlRoot)
            SHFree (This->pidlRoot);
        LocalFree ((HLOCAL) This);
        return 0;
    }
    return This->ref;
}

/**************************************************************************
*	ISF_MyComputer_fnParseDisplayName
*/
static HRESULT WINAPI
ISF_MyComputer_fnParseDisplayName (IShellFolder2 * iface,
				   HWND hwndOwner,
				   LPBC pbc,
				   LPOLESTR lpszDisplayName,
				   DWORD * pchEaten, LPITEMIDLIST * ppidl, DWORD * pdwAttributes)
{
    ICOM_THIS (IGenericSFImpl, iface);

    HRESULT hr = E_INVALIDARG;
    LPCWSTR szNext = NULL;
    WCHAR szElement[MAX_PATH];
    CHAR szTempA[MAX_PATH];
    LPITEMIDLIST pidlTemp = NULL;
    CLSID clsid;

    TRACE ("(%p)->(HWND=%p,%p,%p=%s,%p,pidl=%p,%p)\n",
	   This, hwndOwner, pbc, lpszDisplayName, debugstr_w (lpszDisplayName), pchEaten, ppidl, pdwAttributes);

    *ppidl = 0;
    if (pchEaten)
	*pchEaten = 0;		/* strange but like the original */

    /* handle CLSID paths */
    if (lpszDisplayName[0] == ':' && lpszDisplayName[1] == ':') {
	szNext = GetNextElementW (lpszDisplayName, szElement, MAX_PATH);
	TRACE ("-- element: %s\n", debugstr_w (szElement));
	SHCLSIDFromStringW (szElement + 2, &clsid);
	pidlTemp = _ILCreate (PT_MYCOMP, &clsid, sizeof (clsid));
    }
    /* do we have an absolute path name ? */
    else if (PathGetDriveNumberW (lpszDisplayName) >= 0 && lpszDisplayName[2] == (WCHAR) '\\') {
	szNext = GetNextElementW (lpszDisplayName, szElement, MAX_PATH);
	WideCharToMultiByte (CP_ACP, 0, szElement, -1, szTempA, MAX_PATH, NULL, NULL);
	pidlTemp = _ILCreateDrive (szTempA);
    }

    if (szNext && *szNext) {
	hr = SHELL32_ParseNextElement (iface, hwndOwner, pbc, &pidlTemp, (LPOLESTR) szNext, pchEaten, pdwAttributes);
    } else {
	if (pdwAttributes && *pdwAttributes) {
	    SHELL32_GetItemAttributes (_IShellFolder_ (This), pidlTemp, pdwAttributes);
	}
	hr = S_OK;
    }

    *ppidl = pidlTemp;

    TRACE ("(%p)->(-- ret=0x%08lx)\n", This, hr);

    return hr;
}

/**************************************************************************
*		ISF_MyComputer_fnEnumObjects
*/
static HRESULT WINAPI
ISF_MyComputer_fnEnumObjects (IShellFolder2 * iface, HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST * ppEnumIDList)
{
    ICOM_THIS (IGenericSFImpl, iface);

    TRACE ("(%p)->(HWND=%p flags=0x%08lx pplist=%p)\n", This, hwndOwner, dwFlags, ppEnumIDList);

    *ppEnumIDList = IEnumIDList_Constructor (NULL, dwFlags, EIDL_MYCOMP);

    TRACE ("-- (%p)->(new ID List: %p)\n", This, *ppEnumIDList);

    return (*ppEnumIDList) ? S_OK : E_OUTOFMEMORY;
}

/**************************************************************************
*		ISF_MyComputer_fnBindToObject
*/
static HRESULT WINAPI
ISF_MyComputer_fnBindToObject (IShellFolder2 * iface, LPCITEMIDLIST pidl,
			       LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    ICOM_THIS (IGenericSFImpl, iface);

    TRACE ("(%p)->(pidl=%p,%p,%s,%p)\n", This, pidl, pbcReserved, shdebugstr_guid (riid), ppvOut);

    return SHELL32_BindToChild (This->pidlRoot, NULL, pidl, riid, ppvOut);
}

/**************************************************************************
*	ISF_MyComputer_fnBindToStorage
*/
static HRESULT WINAPI
ISF_MyComputer_fnBindToStorage (IShellFolder2 * iface,
				LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    ICOM_THIS (IGenericSFImpl, iface);

    FIXME ("(%p)->(pidl=%p,%p,%s,%p) stub\n", This, pidl, pbcReserved, shdebugstr_guid (riid), ppvOut);

    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
* 	ISF_MyComputer_fnCompareIDs
*/

static HRESULT WINAPI
ISF_MyComputer_fnCompareIDs (IShellFolder2 * iface, LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    ICOM_THIS (IGenericSFImpl, iface);

    int nReturn;

    TRACE ("(%p)->(0x%08lx,pidl1=%p,pidl2=%p)\n", This, lParam, pidl1, pidl2);
    nReturn = SHELL32_CompareIDs (_IShellFolder_ (This), lParam, pidl1, pidl2);
    TRACE ("-- %i\n", nReturn);
    return nReturn;
}

/**************************************************************************
*	ISF_MyComputer_fnCreateViewObject
*/
static HRESULT WINAPI
ISF_MyComputer_fnCreateViewObject (IShellFolder2 * iface, HWND hwndOwner, REFIID riid, LPVOID * ppvOut)
{
    ICOM_THIS (IGenericSFImpl, iface);

    LPSHELLVIEW pShellView;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(hwnd=%p,%s,%p)\n", This, hwndOwner, shdebugstr_guid (riid), ppvOut);

    if (ppvOut) {
	*ppvOut = NULL;

	if (IsEqualIID (riid, &IID_IDropTarget)) {
	    WARN ("IDropTarget not implemented\n");
	    hr = E_NOTIMPL;
	} else if (IsEqualIID (riid, &IID_IContextMenu)) {
	    WARN ("IContextMenu not implemented\n");
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
*  ISF_MyComputer_fnGetAttributesOf
*/
static HRESULT WINAPI
ISF_MyComputer_fnGetAttributesOf (IShellFolder2 * iface, UINT cidl, LPCITEMIDLIST * apidl, DWORD * rgfInOut)
{
    ICOM_THIS (IGenericSFImpl, iface);

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
*	ISF_MyComputer_fnGetUIObjectOf
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
static HRESULT WINAPI
ISF_MyComputer_fnGetUIObjectOf (IShellFolder2 * iface,
				HWND hwndOwner,
				UINT cidl, LPCITEMIDLIST * apidl, REFIID riid, UINT * prgfInOut, LPVOID * ppvOut)
{
    ICOM_THIS (IGenericSFImpl, iface);

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

/**************************************************************************
*	ISF_MyComputer_fnGetDisplayNameOf
*/
static HRESULT WINAPI ISF_MyComputer_fnGetDisplayNameOf (IShellFolder2 * iface, LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET strRet)
{
    ICOM_THIS (IGenericSFImpl, iface);

    char szPath[MAX_PATH],
    szDrive[18];
    int len = 0;
    BOOL bSimplePidl;
    HRESULT hr = S_OK;

    TRACE ("(%p)->(pidl=%p,0x%08lx,%p)\n", This, pidl, dwFlags, strRet);
    pdump (pidl);

    if (!strRet)
	return E_INVALIDARG;

    szPath[0] = 0x00;
    szDrive[0] = 0x00;

    bSimplePidl = _ILIsPidlSimple (pidl);

    if (!pidl->mkid.cb) {
	/* parsing name like ::{...} */
	lstrcpyA (szPath, "::");
	SHELL32_GUIDToStringA(&CLSID_MyComputer, &szPath[2]);
    } else if (_ILIsSpecialFolder (pidl)) {
	/* take names of special folders only if its only this folder */
	if (bSimplePidl) {
	    GUID const *clsid;

	    if ((clsid = _ILGetGUIDPointer (pidl))) {
		if (GET_SHGDN_FOR (dwFlags) == SHGDN_FORPARSING) {
		    int bWantsForParsing;

		    /*
		     * we can only get a filesystem path from a shellfolder if the value WantsFORPARSING in
		     * CLSID\\{...}\\shellfolder exists
		     * exception: the MyComputer folder has this keys not but like any filesystem backed
		     *            folder it needs these behaviour
		     */
		    /* get the "WantsFORPARSING" flag from the registry */
		    char szRegPath[100];

		    lstrcpyA (szRegPath, "CLSID\\");
		    SHELL32_GUIDToStringA (clsid, &szRegPath[6]);
		    lstrcatA (szRegPath, "\\shellfolder");
		    bWantsForParsing =
			(ERROR_SUCCESS ==
			 SHGetValueA (HKEY_CLASSES_ROOT, szRegPath, "WantsFORPARSING", NULL, NULL, NULL));

		    if ((GET_SHGDN_RELATION (dwFlags) == SHGDN_NORMAL) && bWantsForParsing) {
			/* we need the filesystem path to the destination folder. Only the folder itself can know it */
			hr = SHELL32_GetDisplayNameOfChild (iface, pidl, dwFlags, szPath, MAX_PATH);
		    } else {
			LPSTR p;

			/* parsing name like ::{...} */
			p = lstrcpyA(szPath, "::") + 2;
			p += SHELL32_GUIDToStringA(&CLSID_MyComputer, p);

			lstrcatA(p, "\\::");
			p += 3;
			SHELL32_GUIDToStringA(clsid, p);
		    }
		} else {
		    /* user friendly name */
		    HCR_GetClassNameA (clsid, szPath, MAX_PATH);
		}
	    } else
		_ILSimpleGetText (pidl, szPath, MAX_PATH);	/* append my own path */
	} else {
	    FIXME ("special folder\n");
	}
    } else {
	if (!_ILIsDrive (pidl)) {
	    ERR ("Wrong pidl type\n");
	    return E_INVALIDARG;
	}

	_ILSimpleGetText (pidl, szPath, MAX_PATH);	/* append my own path */

	/* long view "lw_name (C:)" */
	if (bSimplePidl && !(dwFlags & SHGDN_FORPARSING)) {
	    DWORD dwVolumeSerialNumber,
	      dwMaximumComponetLength,
	      dwFileSystemFlags;

	    GetVolumeInformationA (szPath, szDrive, sizeof (szDrive) - 6, &dwVolumeSerialNumber,
				   &dwMaximumComponetLength, &dwFileSystemFlags, NULL, 0);
	    strcat (szDrive, " (");
	    strncat (szDrive, szPath, 2);
	    strcat (szDrive, ")");
	    strcpy (szPath, szDrive);
	}
    }

    if (!bSimplePidl) {		/* go deeper if needed */
	PathAddBackslashA (szPath);
	len = strlen (szPath);

	hr = SHELL32_GetDisplayNameOfChild (iface, pidl, dwFlags | SHGDN_INFOLDER, szPath + len, MAX_PATH - len);
    }

    if (SUCCEEDED (hr)) {
	strRet->uType = STRRET_CSTR;
	lstrcpynA (strRet->u.cStr, szPath, MAX_PATH);
    }

    TRACE ("-- (%p)->(%s)\n", This, szPath);
    return hr;
}

/**************************************************************************
*  ISF_MyComputer_fnSetNameOf
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
static HRESULT WINAPI ISF_MyComputer_fnSetNameOf (IShellFolder2 * iface, HWND hwndOwner, LPCITEMIDLIST pidl,	/*simple pidl */
						  LPCOLESTR lpName, DWORD dwFlags, LPITEMIDLIST * pPidlOut)
{
    ICOM_THIS (IGenericSFImpl, iface);
    FIXME ("(%p)->(%p,pidl=%p,%s,%lu,%p)\n", This, hwndOwner, pidl, debugstr_w (lpName), dwFlags, pPidlOut);
    return E_FAIL;
}

static HRESULT WINAPI ISF_MyComputer_fnGetDefaultSearchGUID (IShellFolder2 * iface, GUID * pguid)
{
    ICOM_THIS (IGenericSFImpl, iface);
    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}
static HRESULT WINAPI ISF_MyComputer_fnEnumSearches (IShellFolder2 * iface, IEnumExtraSearch ** ppenum)
{
    ICOM_THIS (IGenericSFImpl, iface);
    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}
static HRESULT WINAPI ISF_MyComputer_fnGetDefaultColumn (IShellFolder2 * iface, DWORD dwRes, ULONG * pSort, ULONG * pDisplay)
{
    ICOM_THIS (IGenericSFImpl, iface);

    TRACE ("(%p)\n", This);

    if (pSort) *pSort = 0;
    if (pDisplay) *pDisplay = 0;
    return S_OK;
}
static HRESULT WINAPI ISF_MyComputer_fnGetDefaultColumnState (IShellFolder2 * iface, UINT iColumn, DWORD * pcsFlags)
{
    ICOM_THIS (IGenericSFImpl, iface);

    TRACE ("(%p)\n", This);

    if (!pcsFlags || iColumn >= MYCOMPUTERSHELLVIEWCOLUMNS) return E_INVALIDARG;
    *pcsFlags = MyComputerSFHeader[iColumn].pcsFlags;
    return S_OK;
}
static HRESULT WINAPI ISF_MyComputer_fnGetDetailsEx (IShellFolder2 * iface, LPCITEMIDLIST pidl, const SHCOLUMNID * pscid, VARIANT * pv)
{
    ICOM_THIS (IGenericSFImpl, iface);
    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}

/* FIXME: drive size >4GB is rolling over */
static HRESULT WINAPI ISF_MyComputer_fnGetDetailsOf (IShellFolder2 * iface, LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS * psd)
{
    ICOM_THIS (IGenericSFImpl, iface);
    HRESULT hr;

    TRACE ("(%p)->(%p %i %p)\n", This, pidl, iColumn, psd);

    if (!psd || iColumn >= MYCOMPUTERSHELLVIEWCOLUMNS)
	return E_INVALIDARG;

    if (!pidl) {
	psd->fmt = MyComputerSFHeader[iColumn].fmt;
	psd->cxChar = MyComputerSFHeader[iColumn].cxChar;
	psd->str.uType = STRRET_CSTR;
	LoadStringA (shell32_hInstance, MyComputerSFHeader[iColumn].colnameid, psd->str.u.cStr, MAX_PATH);
	return S_OK;
    } else {
	char szPath[MAX_PATH];
	ULARGE_INTEGER ulBytes;

	psd->str.u.cStr[0] = 0x00;
	psd->str.uType = STRRET_CSTR;
	switch (iColumn) {
	case 0:		/* name */
	    hr = IShellFolder_GetDisplayNameOf (iface, pidl, SHGDN_NORMAL | SHGDN_INFOLDER, &psd->str);
	    break;
	case 1:		/* type */
	    _ILGetFileType (pidl, psd->str.u.cStr, MAX_PATH);
	    break;
	case 2:		/* total size */
	    if (_ILIsDrive (pidl)) {
		_ILSimpleGetText (pidl, szPath, MAX_PATH);
		GetDiskFreeSpaceExA (szPath, NULL, &ulBytes, NULL);
		StrFormatByteSizeA (ulBytes.u.LowPart, psd->str.u.cStr, MAX_PATH);
	    }
	    break;
	case 3:		/* free size */
	    if (_ILIsDrive (pidl)) {
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
static HRESULT WINAPI ISF_MyComputer_fnMapColumnToSCID (IShellFolder2 * iface, UINT column, SHCOLUMNID * pscid)
{
    ICOM_THIS (IGenericSFImpl, iface);
    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}

static ICOM_VTABLE (IShellFolder2) vt_ShellFolder2 =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
 *	IMCFldr_PersistFolder2_QueryInterface
 */
static HRESULT WINAPI IMCFldr_PersistFolder2_QueryInterface (IPersistFolder2 * iface, REFIID iid, LPVOID * ppvObj)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)\n", This);

    return IUnknown_QueryInterface (_IUnknown_ (This), iid, ppvObj);
}

/************************************************************************
 *	IMCFldr_PersistFolder2_AddRef
 */
static ULONG WINAPI IMCFldr_PersistFolder2_AddRef (IPersistFolder2 * iface)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)->(count=%lu)\n", This, This->ref);

    return IUnknown_AddRef (_IUnknown_ (This));
}

/************************************************************************
 *	ISFPersistFolder_Release
 */
static ULONG WINAPI IMCFldr_PersistFolder2_Release (IPersistFolder2 * iface)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)->(count=%lu)\n", This, This->ref);

    return IUnknown_Release (_IUnknown_ (This));
}

/************************************************************************
 *	IMCFldr_PersistFolder2_GetClassID
 */
static HRESULT WINAPI IMCFldr_PersistFolder2_GetClassID (IPersistFolder2 * iface, CLSID * lpClassId)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)\n", This);

    if (!lpClassId)
	return E_POINTER;
    *lpClassId = CLSID_MyComputer;

    return S_OK;
}

/************************************************************************
 *	IMCFldr_PersistFolder2_Initialize
 *
 * NOTES: it makes no sense to change the pidl
 */
static HRESULT WINAPI IMCFldr_PersistFolder2_Initialize (IPersistFolder2 * iface, LPCITEMIDLIST pidl)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);
    TRACE ("(%p)->(%p)\n", This, pidl);
    return E_NOTIMPL;
}

/**************************************************************************
 *	IPersistFolder2_fnGetCurFolder
 */
static HRESULT WINAPI IMCFldr_PersistFolder2_GetCurFolder (IPersistFolder2 * iface, LPITEMIDLIST * pidl)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)->(%p)\n", This, pidl);

    if (!pidl)
	return E_POINTER;
    *pidl = ILClone (This->pidlRoot);
    return S_OK;
}

static ICOM_VTABLE (IPersistFolder2) vt_PersistFolder2 =
{
ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IMCFldr_PersistFolder2_QueryInterface,
	IMCFldr_PersistFolder2_AddRef,
	IMCFldr_PersistFolder2_Release,
	IMCFldr_PersistFolder2_GetClassID,
	IMCFldr_PersistFolder2_Initialize,
	IMCFldr_PersistFolder2_GetCurFolder
};

/*
 *	Copyright 1997	Marcus Meissner
 *	Copyright 1998	Juergen Schmied
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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "undocshell.h"
#include "shlguid.h"

#include "wine/debug.h"
#include "winerror.h"

#include "pidl.h"
#include "shell32_main.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/***********************************************************************
*   IExtractIconW implementation
*/
typedef struct
{
	ICOM_VFIELD(IExtractIconW);
	DWORD	ref;
	ICOM_VTABLE(IPersistFile)*	lpvtblPersistFile;
	ICOM_VTABLE(IExtractIconA)*	lpvtblExtractIconA;
	LPITEMIDLIST	pidl;
} IExtractIconWImpl;

static struct ICOM_VTABLE(IExtractIconA) eiavt;
static struct ICOM_VTABLE(IExtractIconW) eivt;
static struct ICOM_VTABLE(IPersistFile) pfvt;

#define _IPersistFile_Offset ((int)(&(((IExtractIconWImpl*)0)->lpvtblPersistFile)))
#define _ICOM_THIS_From_IPersistFile(class, name) class* This = (class*)(((char*)name)-_IPersistFile_Offset);

#define _IExtractIconA_Offset ((int)(&(((IExtractIconWImpl*)0)->lpvtblExtractIconA)))
#define _ICOM_THIS_From_IExtractIconA(class, name) class* This = (class*)(((char*)name)-_IExtractIconA_Offset);

/**************************************************************************
*  IExtractIconW_Constructor
*/
IExtractIconW* IExtractIconW_Constructor(LPCITEMIDLIST pidl)
{
	IExtractIconWImpl* ei;
	
	TRACE("%p\n", pidl);

	ei = (IExtractIconWImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IExtractIconWImpl));
	ei->ref=1;
	ei->lpVtbl = &eivt;
	ei->lpvtblPersistFile = &pfvt;
	ei->lpvtblExtractIconA = &eiavt;
	ei->pidl=ILClone(pidl);

	pdump(pidl);

	TRACE("(%p)\n", ei);
	return (IExtractIconW *)ei;
}
/**************************************************************************
 *  IExtractIconW_QueryInterface
 */
static HRESULT WINAPI IExtractIconW_fnQueryInterface(IExtractIconW *iface, REFIID riid, LPVOID *ppvObj)
{
	ICOM_THIS(IExtractIconWImpl, iface);

	TRACE("(%p)->(\n\tIID:\t%s,%p)\n", This, debugstr_guid(riid), ppvObj);

	*ppvObj = NULL;

	if (IsEqualIID(riid, &IID_IUnknown))				/*IUnknown*/
	{
	  *ppvObj = This;
	}
	else if (IsEqualIID(riid, &IID_IPersistFile))	/*IExtractIcon*/
	{
	  *ppvObj = (IPersistFile*)&(This->lpvtblPersistFile);
	}
	else if (IsEqualIID(riid, &IID_IExtractIconA))	/*IExtractIcon*/
	{
	  *ppvObj = (IExtractIconA*)&(This->lpvtblExtractIconA);
	}
	else if (IsEqualIID(riid, &IID_IExtractIconW))	/*IExtractIcon*/
	{
	  *ppvObj = (IExtractIconW*)This;
	}

	if(*ppvObj)
	{
	  IExtractIconW_AddRef((IExtractIconW*) *ppvObj);
	  TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE("-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

/**************************************************************************
*  IExtractIconW_AddRef
*/
static ULONG WINAPI IExtractIconW_fnAddRef(IExtractIconW * iface)
{
	ICOM_THIS(IExtractIconWImpl, iface);

	TRACE("(%p)->(count=%lu)\n",This, This->ref );

	return ++(This->ref);
}
/**************************************************************************
*  IExtractIconW_Release
*/
static ULONG WINAPI IExtractIconW_fnRelease(IExtractIconW * iface)
{
	ICOM_THIS(IExtractIconWImpl, iface);

	TRACE("(%p)->()\n",This);

	if (!--(This->ref))
	{
	  TRACE(" destroying IExtractIcon(%p)\n",This);
	  SHFree(This->pidl);
	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}

WCHAR swShell32Name[MAX_PATH];

/**************************************************************************
*  IExtractIconW_GetIconLocation
*
* mapping filetype to icon
*/
static HRESULT WINAPI IExtractIconW_fnGetIconLocation(
	IExtractIconW * iface,
	UINT uFlags,		/* GIL_ flags */
	LPWSTR szIconFile,
	UINT cchMax,
	int * piIndex,
	UINT * pwFlags)		/* returned GIL_ flags */
{
	ICOM_THIS(IExtractIconWImpl, iface);

	char	sTemp[MAX_PATH];
	DWORD	dwNr;
	GUID const * riid;
	LPITEMIDLIST	pSimplePidl = ILFindLastID(This->pidl);

	TRACE("(%p) (flags=%u %p %u %p %p)\n", This, uFlags, szIconFile, cchMax, piIndex, pwFlags);

	if (pwFlags)
	  *pwFlags = 0;

	if (_ILIsDesktop(pSimplePidl))
	{
	  lstrcpynW(szIconFile, swShell32Name, cchMax);
	  *piIndex = 34;
	}

	/* my computer and other shell extensions */
	else if ((riid = _ILGetGUIDPointer(pSimplePidl)))
	{
	  static WCHAR fmt[] = { 'C','L','S','I','D','\\','{','%','0','8','l','x',
       '-','%','0','4','x','-','%','0','4','x','-','%','0','2','x',
       '%','0','2','x','-','%','0','2','x', '%','0','2','x', '%','0','2','x',
       '%','0','2','x','%','0','2','x','%','0','2','x','}',0 };
	  WCHAR xriid[50];

	  sprintfW(xriid, fmt,
	          riid->Data1, riid->Data2, riid->Data3,
	          riid->Data4[0], riid->Data4[1], riid->Data4[2], riid->Data4[3],
	          riid->Data4[4], riid->Data4[5], riid->Data4[6], riid->Data4[7]);

	  if (HCR_GetDefaultIconW(xriid, szIconFile, cchMax, &dwNr))
	  {
	    *piIndex = dwNr;
	  }
	  else
	  {
	    lstrcpynW(szIconFile, swShell32Name, cchMax);
	    *piIndex = 15;
	  }
	}

	else if (_ILIsDrive (pSimplePidl))
	{
	  static WCHAR drive[] = { 'D','r','i','v','e',0 };

	  if (HCR_GetDefaultIconW(drive, szIconFile, cchMax, &dwNr))
	  {
	    *piIndex = dwNr;
	  }
	  else
	  {
	    lstrcpynW(szIconFile, swShell32Name, cchMax);
	    *piIndex = 8;
	  }
	}

	else if (_ILIsFolder (pSimplePidl))
	{
	  static WCHAR folder[] = { 'F','o','l','d','e','r',0 };

	  if (!HCR_GetDefaultIconW(folder, szIconFile, cchMax, &dwNr))
	  {
	    lstrcpynW(szIconFile, swShell32Name, cchMax);
	    dwNr = 3;
	  }
	  *piIndex = (uFlags & GIL_OPENICON) ? dwNr + 1 : dwNr;
	}

	else
	{
	  BOOL found = FALSE;

	  if (_ILIsCPanelStruct(pSimplePidl))
	  {
	    if (SUCCEEDED(CPanel_GetIconLocationW(pSimplePidl, szIconFile, cchMax, piIndex)))
		found = TRUE;
	  }
	  else if (_ILGetExtension(pSimplePidl, sTemp, MAX_PATH))
	  {
	    if (HCR_MapTypeToValueA(sTemp, sTemp, MAX_PATH, TRUE)
		&& HCR_GetDefaultIconA(sTemp, sTemp, MAX_PATH, &dwNr))
	    {
	      if (!lstrcmpA("%1", sTemp))		/* icon is in the file */
	      {
		SHGetPathFromIDListW(This->pidl, szIconFile);
		*piIndex = 0;
	      }
	      else
	      {
		MultiByteToWideChar(CP_ACP, 0, sTemp, -1, szIconFile, cchMax);
		*piIndex = dwNr;
	      }

	      found = TRUE;
	    }
	    else if (!strcasecmp(sTemp, "lnkfile"))
	    {
	      /* extract icon from shell shortcut */
	      IShellFolder* dsf;
	      IShellLinkW* psl;

	      if (SUCCEEDED(SHGetDesktopFolder(&dsf)))
	      {
		HRESULT hr = IShellFolder_GetUIObjectOf(dsf, NULL, 1, (LPCITEMIDLIST*)&This->pidl, &IID_IShellLinkW, NULL, (LPVOID*)&psl);

		if (SUCCEEDED(hr))
		{
		  /*no need to resolve shortcuts here
		  hr = IShellLinkW_Resolve(psl, pThis->_hwnd, SLR_NO_UI);
		  */

		  hr = IShellLinkW_GetIconLocation(psl, szIconFile, MAX_PATH, piIndex);

		  if (SUCCEEDED(hr) && *szIconFile)
		    found = TRUE;

		  IShellLinkW_Release(psl);
		}

		IShellFolder_Release(dsf);
	      }
	    }
	  }

	  if (!found)					/* default icon */
	  {
	    lstrcpynW(szIconFile, swShell32Name, cchMax);
	    *piIndex = 0;
	  }
	}

	TRACE("-- %s %x\n", debugstr_w(szIconFile), *piIndex);
	return NOERROR;
}

/**************************************************************************
*  IExtractIconW_Extract
*/
static HRESULT WINAPI IExtractIconW_fnExtract(IExtractIconW * iface, LPCWSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
	ICOM_THIS(IExtractIconWImpl, iface);

	FIXME("(%p) (file=%p index=%u %p %p size=%u) semi-stub\n", This, debugstr_w(pszFile), nIconIndex, phiconLarge, phiconSmall, nIconSize);

	if (phiconLarge)
	  *phiconLarge = ImageList_GetIcon(ShellBigIconList, nIconIndex, ILD_TRANSPARENT);

	if (phiconSmall)
	  *phiconSmall = ImageList_GetIcon(ShellSmallIconList, nIconIndex, ILD_TRANSPARENT);

	return S_OK;
}

static struct ICOM_VTABLE(IExtractIconW) eivt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IExtractIconW_fnQueryInterface,
	IExtractIconW_fnAddRef,
	IExtractIconW_fnRelease,
	IExtractIconW_fnGetIconLocation,
	IExtractIconW_fnExtract
};

/**************************************************************************
*  IExtractIconA_Constructor
*/
IExtractIconA* IExtractIconA_Constructor(LPCITEMIDLIST pidl)
{
	ICOM_THIS(IExtractIconWImpl, IExtractIconW_Constructor(pidl));
	IExtractIconA *eia = (IExtractIconA *)&This->lpvtblExtractIconA;
	
	TRACE("(%p)->(%p)\n", This, eia);
	return eia;
}
/**************************************************************************
 *  IExtractIconA_QueryInterface
 */
static HRESULT WINAPI IExtractIconA_fnQueryInterface(IExtractIconA * iface, REFIID riid, LPVOID *ppvObj)
{
	_ICOM_THIS_From_IExtractIconA(IExtractIconW, iface);

	return IExtractIconW_QueryInterface(This, riid, ppvObj);
}

/**************************************************************************
*  IExtractIconA_AddRef
*/
static ULONG WINAPI IExtractIconA_fnAddRef(IExtractIconA * iface)
{
	_ICOM_THIS_From_IExtractIconA(IExtractIconW, iface);

	return IExtractIconW_AddRef(This);
}
/**************************************************************************
*  IExtractIconA_Release
*/
static ULONG WINAPI IExtractIconA_fnRelease(IExtractIconA * iface)
{
	_ICOM_THIS_From_IExtractIconA(IExtractIconW, iface);

	return IExtractIconW_AddRef(This);
}
/**************************************************************************
*  IExtractIconA_GetIconLocation
*
* mapping filetype to icon
*/
static HRESULT WINAPI IExtractIconA_fnGetIconLocation(
	IExtractIconA * iface,
	UINT uFlags,
	LPSTR szIconFile,
	UINT cchMax,
	int * piIndex,
	UINT * pwFlags)
{
	HRESULT ret;
	LPWSTR lpwstrFile = HeapAlloc(GetProcessHeap(), 0, cchMax * sizeof(WCHAR));
	_ICOM_THIS_From_IExtractIconA(IExtractIconW, iface);
	
	TRACE("(%p) (flags=%u %p %u %p %p)\n", This, uFlags, szIconFile, cchMax, piIndex, pwFlags);

	ret = IExtractIconW_GetIconLocation(This, uFlags, lpwstrFile, cchMax, piIndex, pwFlags);
	WideCharToMultiByte(CP_ACP, 0, lpwstrFile, -1, szIconFile, cchMax, NULL, NULL);
	HeapFree(GetProcessHeap(), 0, lpwstrFile);

	TRACE("-- %s %x\n", szIconFile, *piIndex);
	return ret;
}
/**************************************************************************
*  IExtractIconA_Extract
*/
static HRESULT WINAPI IExtractIconA_fnExtract(IExtractIconA * iface, LPCSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
	HRESULT ret;
	INT len = MultiByteToWideChar(CP_ACP, 0, pszFile, -1, NULL, 0);
	LPWSTR lpwstrFile = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
	_ICOM_THIS_From_IExtractIconA(IExtractIconW, iface);

	TRACE("(%p) (file=%p index=%u %p %p size=%u)\n", This, pszFile, nIconIndex, phiconLarge, phiconSmall, nIconSize);

	MultiByteToWideChar(CP_ACP, 0, pszFile, -1, lpwstrFile, len);
	ret = IExtractIconW_Extract(This, lpwstrFile, nIconIndex, phiconLarge, phiconSmall, nIconSize);
	HeapFree(GetProcessHeap(), 0, lpwstrFile);
	return ret;
}

static struct ICOM_VTABLE(IExtractIconA) eiavt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IExtractIconA_fnQueryInterface,
	IExtractIconA_fnAddRef,
	IExtractIconA_fnRelease,
	IExtractIconA_fnGetIconLocation,
	IExtractIconA_fnExtract
};

/************************************************************************
 * IEIPersistFile_QueryInterface (IUnknown)
 */
static HRESULT WINAPI IEIPersistFile_fnQueryInterface(
	IPersistFile	*iface,
	REFIID		iid,
	LPVOID		*ppvObj)
{
	_ICOM_THIS_From_IPersistFile(IExtractIconW, iface);

	return IExtractIconW_QueryInterface(This, iid, ppvObj);
}

/************************************************************************
 * IEIPersistFile_AddRef (IUnknown)
 */
static ULONG WINAPI IEIPersistFile_fnAddRef(
	IPersistFile	*iface)
{
	_ICOM_THIS_From_IPersistFile(IExtractIconW, iface);

	return IExtractIconW_AddRef(This);
}

/************************************************************************
 * IEIPersistFile_Release (IUnknown)
 */
static ULONG WINAPI IEIPersistFile_fnRelease(
	IPersistFile	*iface)
{
	_ICOM_THIS_From_IPersistFile(IExtractIconW, iface);

	return IExtractIconW_Release(This);
}

/************************************************************************
 * IEIPersistFile_GetClassID (IPersist)
 */
static HRESULT WINAPI IEIPersistFile_fnGetClassID(
	IPersistFile	*iface,
	LPCLSID		lpClassId)
{
	CLSID StdFolderID = { 0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} };

	if (lpClassId==NULL)
	  return E_POINTER;

	memcpy(lpClassId, &StdFolderID, sizeof(StdFolderID));

	return S_OK;
}

/************************************************************************
 * IEIPersistFile_Load (IPersistFile)
 */
static HRESULT WINAPI IEIPersistFile_fnLoad(IPersistFile* iface, LPCOLESTR pszFileName, DWORD dwMode)
{
	_ICOM_THIS_From_IPersistFile(IExtractIconW, iface);
	FIXME("%p\n", This);
	return E_NOTIMPL;

}

static struct ICOM_VTABLE(IPersistFile) pfvt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IEIPersistFile_fnQueryInterface,
	IEIPersistFile_fnAddRef,
	IEIPersistFile_fnRelease,
	IEIPersistFile_fnGetClassID,
	(void *) 0xdeadbeef /* IEIPersistFile_fnIsDirty */,
	IEIPersistFile_fnLoad,
	(void *) 0xdeadbeef /* IEIPersistFile_fnSave */,
	(void *) 0xdeadbeef /* IEIPersistFile_fnSaveCompleted */,
	(void *) 0xdeadbeef /* IEIPersistFile_fnGetCurFile */
};

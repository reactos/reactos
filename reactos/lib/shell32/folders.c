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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "objbase.h"
#include "undocshell.h"
#include "shlguid.h"
#include "winreg.h"

#include "wine/debug.h"

#include "pidl.h"
#include "shell32_main.h"
#include "shfldr.h"
#include "shresdef.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/***********************************************************************
*   IExtractIconW implementation
*/
typedef struct
{
	const IExtractIconWVtbl *lpVtbl;
	LONG               ref;
	const IPersistFileVtbl  *lpvtblPersistFile;
	const IExtractIconAVtbl *lpvtblExtractIconA;
	LPITEMIDLIST       pidl;
} IExtractIconWImpl;

static const IExtractIconAVtbl eiavt;
static const IExtractIconWVtbl eivt;
static const IPersistFileVtbl pfvt;

static inline IExtractIconW *impl_from_IPersistFile( IPersistFile *iface )
{
    return (IExtractIconW *)((char*)iface - FIELD_OFFSET(IExtractIconWImpl, lpvtblPersistFile));
}

static inline IExtractIconW *impl_from_IExtractIconA( IExtractIconA *iface )
{
    return (IExtractIconW *)((char*)iface - FIELD_OFFSET(IExtractIconWImpl, lpvtblExtractIconA));
}


/**************************************************************************
*  IExtractIconW_Constructor
*/
IExtractIconW* IExtractIconW_Constructor(LPCITEMIDLIST pidl)
{
	IExtractIconWImpl* ei;
	
	TRACE("%p\n", pidl);

	ei = HeapAlloc(GetProcessHeap(),0,sizeof(IExtractIconWImpl));
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
	IExtractIconWImpl *This = (IExtractIconWImpl *)iface;

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
	IExtractIconWImpl *This = (IExtractIconWImpl *)iface;
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(count=%lu)\n", This, refCount - 1);

	return refCount;
}
/**************************************************************************
*  IExtractIconW_Release
*/
static ULONG WINAPI IExtractIconW_fnRelease(IExtractIconW * iface)
{
	IExtractIconWImpl *This = (IExtractIconWImpl *)iface;
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(count=%lu)\n", This, refCount + 1);

	if (!refCount)
	{
	  TRACE(" destroying IExtractIcon(%p)\n",This);
	  SHFree(This->pidl);
	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return refCount;
}

static HRESULT getIconLocationForFolder(IExtractIconW *iface, UINT uFlags,
 LPWSTR szIconFile, UINT cchMax, int *piIndex, UINT *pwFlags)
{
    IExtractIconWImpl *This = (IExtractIconWImpl *)iface;
    DWORD dwNr;
    WCHAR wszPath[MAX_PATH];
    WCHAR wszCLSIDValue[CHARS_IN_GUID];
    static const WCHAR shellClassInfo[] = { '.','S','h','e','l','l','C','l','a','s','s','I','n','f','o',0 };
    static const WCHAR iconFile[] = { 'I','c','o','n','F','i','l','e',0 };
    static const WCHAR clsid[] = { 'C','L','S','I','D',0 };
    static const WCHAR clsid2[] = { 'C','L','S','I','D','2',0 };
    static const WCHAR iconIndex[] = { 'I','c','o','n','I','n','d','e','x',0 };

    if (SHELL32_GetCustomFolderAttribute(This->pidl, shellClassInfo, iconFile,
        wszPath, MAX_PATH))
    {
        WCHAR wszIconIndex[10];
        SHELL32_GetCustomFolderAttribute(This->pidl, shellClassInfo, iconIndex,
            wszIconIndex, 10);
        *piIndex = atoiW(wszIconIndex);
    }
    else if (SHELL32_GetCustomFolderAttribute(This->pidl, shellClassInfo, clsid,
        wszCLSIDValue, CHARS_IN_GUID) &&
        HCR_GetDefaultIconW(wszCLSIDValue, szIconFile, cchMax, &dwNr))
    {
       *piIndex = dwNr;
    }
    else if (SHELL32_GetCustomFolderAttribute(This->pidl, shellClassInfo, clsid2,
        wszCLSIDValue, CHARS_IN_GUID) &&
        HCR_GetDefaultIconW(wszCLSIDValue, szIconFile, cchMax, &dwNr))
    {
       *piIndex = dwNr;
    }
    else
    {
        static const WCHAR folder[] = { 'F','o','l','d','e','r',0 };

        if (!HCR_GetDefaultIconW(folder, szIconFile, cchMax, &dwNr))
        {
            lstrcpynW(szIconFile, swShell32Name, cchMax);
            dwNr = IDI_SHELL_FOLDER;
        }
        *piIndex = -((uFlags & GIL_OPENICON) ? dwNr + 1 : dwNr);
    }
    return S_OK;
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
	IExtractIconWImpl *This = (IExtractIconWImpl *)iface;

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
	  *piIndex = -IDI_SHELL_DESKTOP;
	}

	/* my computer and other shell extensions */
	else if ((riid = _ILGetGUIDPointer(pSimplePidl)))
	{
	  static const WCHAR fmt[] = { 'C','L','S','I','D','\\',
       '{','%','0','8','l','x','-','%','0','4','x','-','%','0','4','x','-',
       '%','0','2','x','%','0','2','x','-','%','0','2','x', '%','0','2','x',
       '%','0','2','x','%','0','2','x','%','0','2','x','%','0','2','x','}',0 };
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
            if(IsEqualGUID(riid, &CLSID_MyComputer))
                *piIndex = -IDI_SHELL_MY_COMPUTER;
            else if(IsEqualGUID(riid, &CLSID_MyDocuments))
                *piIndex = -IDI_SHELL_MY_DOCUMENTS;
            else if(IsEqualGUID(riid, &CLSID_NetworkPlaces))
                *piIndex = -IDI_SHELL_MY_NETWORK_PLACES;
            else
                *piIndex = -IDI_SHELL_FOLDER;
	  }
	}

	else if (_ILIsDrive (pSimplePidl))
	{
	  static const WCHAR drive[] = { 'D','r','i','v','e',0 };

	  int icon_idx = -1;

	  if (_ILGetDrive(pSimplePidl, sTemp, MAX_PATH))
	  {
		switch(GetDriveTypeA(sTemp))
		{
                  case DRIVE_REMOVABLE:   icon_idx = IDI_SHELL_FLOPPY;        break;
                  case DRIVE_CDROM:       icon_idx = IDI_SHELL_CDROM;         break;
                  case DRIVE_REMOTE:      icon_idx = IDI_SHELL_NETDRIVE;      break;
                  case DRIVE_RAMDISK:     icon_idx = IDI_SHELL_RAMDISK;       break;
		}
	  }

	  if (icon_idx != -1)
	  {
		lstrcpynW(szIconFile, swShell32Name, cchMax);
		*piIndex = -icon_idx;
	  }
	  else
	  {
		if (HCR_GetDefaultIconW(drive, szIconFile, cchMax, &dwNr))
		{
		  *piIndex = dwNr;
		}
		else
		{
		  lstrcpynW(szIconFile, swShell32Name, cchMax);
		  *piIndex = -IDI_SHELL_DRIVE;
		}
	  }
	}
	else if (_ILIsFolder (pSimplePidl))
	{
            getIconLocationForFolder(iface, uFlags, szIconFile, cchMax, piIndex,
                                     pwFlags);
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
	    else if (!lstrcmpiA(sTemp, "lnkfile"))
	    {
	      /* extract icon from shell shortcut */
	      IShellFolder* dsf;
	      IShellLinkW* psl;

	      if (SUCCEEDED(SHGetDesktopFolder(&dsf)))
	      {
		HRESULT hr = IShellFolder_GetUIObjectOf(dsf, NULL, 1, (LPCITEMIDLIST*)&This->pidl, &IID_IShellLinkW, NULL, (LPVOID*)&psl);

		if (SUCCEEDED(hr))
		{
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
	IExtractIconWImpl *This = (IExtractIconWImpl *)iface;
        int index;

	FIXME("(%p) (file=%p index=%d %p %p size=%08x) semi-stub\n", This, debugstr_w(pszFile), (signed)nIconIndex,
              phiconLarge, phiconSmall, nIconSize);

        index = SIC_GetIconIndex(pszFile, nIconIndex, 0);

	if (phiconLarge)
	  *phiconLarge = ImageList_GetIcon(ShellBigIconList, index, ILD_TRANSPARENT);

	if (phiconSmall)
	  *phiconSmall = ImageList_GetIcon(ShellSmallIconList, index, ILD_TRANSPARENT);

	return S_OK;
}

static const IExtractIconWVtbl eivt =
{
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
	IExtractIconWImpl *This = (IExtractIconWImpl *)IExtractIconW_Constructor(pidl);
	IExtractIconA *eia = (IExtractIconA *)&This->lpvtblExtractIconA;
	
	TRACE("(%p)->(%p)\n", This, eia);
	return eia;
}
/**************************************************************************
 *  IExtractIconA_QueryInterface
 */
static HRESULT WINAPI IExtractIconA_fnQueryInterface(IExtractIconA * iface, REFIID riid, LPVOID *ppvObj)
{
	IExtractIconW *This = impl_from_IExtractIconA(iface);

	return IExtractIconW_QueryInterface(This, riid, ppvObj);
}

/**************************************************************************
*  IExtractIconA_AddRef
*/
static ULONG WINAPI IExtractIconA_fnAddRef(IExtractIconA * iface)
{
	IExtractIconW *This = impl_from_IExtractIconA(iface);

	return IExtractIconW_AddRef(This);
}
/**************************************************************************
*  IExtractIconA_Release
*/
static ULONG WINAPI IExtractIconA_fnRelease(IExtractIconA * iface)
{
	IExtractIconW *This = impl_from_IExtractIconA(iface);

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
	IExtractIconW *This = impl_from_IExtractIconA(iface);
	
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
	IExtractIconW *This = impl_from_IExtractIconA(iface);

	TRACE("(%p) (file=%p index=%u %p %p size=%u)\n", This, pszFile, nIconIndex, phiconLarge, phiconSmall, nIconSize);

	MultiByteToWideChar(CP_ACP, 0, pszFile, -1, lpwstrFile, len);
	ret = IExtractIconW_Extract(This, lpwstrFile, nIconIndex, phiconLarge, phiconSmall, nIconSize);
	HeapFree(GetProcessHeap(), 0, lpwstrFile);
	return ret;
}

static const IExtractIconAVtbl eiavt =
{
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
	IExtractIconW *This = impl_from_IPersistFile(iface);

	return IExtractIconW_QueryInterface(This, iid, ppvObj);
}

/************************************************************************
 * IEIPersistFile_AddRef (IUnknown)
 */
static ULONG WINAPI IEIPersistFile_fnAddRef(
	IPersistFile	*iface)
{
	IExtractIconW *This = impl_from_IPersistFile(iface);

	return IExtractIconW_AddRef(This);
}

/************************************************************************
 * IEIPersistFile_Release (IUnknown)
 */
static ULONG WINAPI IEIPersistFile_fnRelease(
	IPersistFile	*iface)
{
	IExtractIconW *This = impl_from_IPersistFile(iface);

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
	IExtractIconW *This = impl_from_IPersistFile(iface);
	FIXME("%p\n", This);
	return E_NOTIMPL;

}

static const IPersistFileVtbl pfvt =
{
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

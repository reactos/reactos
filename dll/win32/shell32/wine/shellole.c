/*
 *	handling of SHELL32.DLL OLE-Objects
 *
 *	Copyright 1997	Marcus Meissner
 *	Copyright 1998	Juergen Schmied  <juergen.schmied@metronet.de>
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

#include <wine/config.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COBJMACROS
#define NONAMELESSUNION

#include <windef.h>
#include <winbase.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <debughlp.h>

#include <wine/debug.h>
#include <wine/unicode.h>

#include "shell32_main.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

extern INT WINAPI SHStringFromGUIDW(REFGUID guid, LPWSTR lpszDest, INT cchMax);  /* shlwapi.24 */

/**************************************************************************
 * Default ClassFactory types
 */
typedef HRESULT (CALLBACK *LPFNCREATEINSTANCE)(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObject);

#ifndef __REACTOS__

static IClassFactory * IDefClF_fnConstructor(LPFNCREATEINSTANCE lpfnCI, PLONG pcRefDll, REFIID riidInst);

/* this table contains all CLSIDs of shell32 objects */
static const struct {
	REFIID			clsid;
	LPFNCREATEINSTANCE	lpfnCI;
} InterfaceTable[] = {

	{&CLSID_ApplicationAssociationRegistration, ApplicationAssociationRegistration_Constructor},
	{&CLSID_AutoComplete,   IAutoComplete_Constructor},
	{&CLSID_ControlPanel,	IControlPanel_Constructor},
	{&CLSID_DragDropHelper, IDropTargetHelper_Constructor},
	{&CLSID_FolderShortcut, FolderShortcut_Constructor},
	{&CLSID_MyComputer,	ISF_MyComputer_Constructor},
	{&CLSID_MyDocuments,    MyDocuments_Constructor},
	{&CLSID_NetworkPlaces,  ISF_NetworkPlaces_Constructor},
	{&CLSID_Printers,       Printers_Constructor},
	{&CLSID_QueryAssociations, QueryAssociations_Constructor},
	{&CLSID_RecycleBin,     RecycleBin_Constructor},
	{&CLSID_ShellDesktop,	ISF_Desktop_Constructor},
	{&CLSID_ShellFSFolder,	IFSFolder_Constructor},
	{&CLSID_ShellItem,	IShellItem_Constructor},
	{&CLSID_ShellLink,	IShellLink_Constructor},
	{&CLSID_UnixDosFolder,  UnixDosFolder_Constructor},
	{&CLSID_UnixFolder,     UnixFolder_Constructor},
	{&CLSID_ExplorerBrowser,ExplorerBrowser_Constructor},
	{&CLSID_KnownFolderManager, KnownFolderManager_Constructor},
	{&CLSID_Shell,          IShellDispatch_Constructor},
	{NULL, NULL}
};

#endif /* !__REACTOS__ */

/*************************************************************************
 * SHCoCreateInstance [SHELL32.102]
 *
 * Equivalent to CoCreateInstance. Under Windows 9x this function could sometimes
 * use the shell32 built-in "mini-COM" without the need to load ole32.dll - see
 * SHLoadOLE for details.
 *
 * Under wine if a "LoadWithoutCOM" value is present or the object resides in
 * shell32.dll the function will load the object manually without the help of ole32
 *
 * NOTES
 *     exported by ordinal
 *
 * SEE ALSO
 *     CoCreateInstance, SHLoadOLE
 */
HRESULT WINAPI SHCoCreateInstance(
	LPCWSTR aclsid,
	const CLSID *clsid,
	LPUNKNOWN pUnkOuter,
	REFIID refiid,
	LPVOID *ppv)
{
	DWORD	hres;
	CLSID	iid;
	const	CLSID * myclsid = clsid;
	WCHAR	sKeyName[MAX_PATH];
	WCHAR	sClassID[60];
	WCHAR	sDllPath[MAX_PATH];
	HKEY	hKey = 0;
	DWORD	dwSize;
	IClassFactory * pcf = NULL;

	if(!ppv) return E_POINTER;
	*ppv=NULL;

	/* if the clsid is a string, convert it */
	if (!clsid)
	{
	  if (!aclsid) return REGDB_E_CLASSNOTREG;
	  SHCLSIDFromStringW(aclsid, &iid);
	  myclsid = &iid;
	}

	TRACE("(%p,%s,unk:%p,%s,%p)\n",
		aclsid,shdebugstr_guid(myclsid),pUnkOuter,shdebugstr_guid(refiid),ppv);

        if (SUCCEEDED(DllGetClassObject(myclsid, &IID_IClassFactory,(LPVOID*)&pcf)))
        {
            hres = IClassFactory_CreateInstance(pcf, pUnkOuter, refiid, ppv);
            IClassFactory_Release(pcf);
            goto end;
        }

	/* we look up the dll path in the registry */
	SHStringFromGUIDW(myclsid, sClassID, ARRAY_SIZE(sClassID));
	swprintf(sKeyName, L"CLSID\\%s\\InprocServer32", sClassID);

	if (RegOpenKeyExW(HKEY_CLASSES_ROOT, sKeyName, 0, KEY_READ, &hKey))
            return E_ACCESSDENIED;

        /* if a special registry key is set, we load a shell extension without help of OLE32 */
        if (!SHQueryValueExW(hKey, L"LoadWithoutCOM", 0, 0, 0, 0))
        {
	    /* load an external dll without ole32 */
	    HANDLE hLibrary;
	    typedef HRESULT (CALLBACK *DllGetClassObjectFunc)(REFCLSID clsid, REFIID iid, LPVOID *ppv);
	    DllGetClassObjectFunc DllGetClassObject;

            dwSize = sizeof(sDllPath);
            SHQueryValueExW(hKey, NULL, 0,0, sDllPath, &dwSize );

	    if ((hLibrary = LoadLibraryExW(sDllPath, 0, LOAD_WITH_ALTERED_SEARCH_PATH)) == 0) {
	        ERR("couldn't load InprocServer32 dll %s\n", debugstr_w(sDllPath));
		hres = E_ACCESSDENIED;
	        goto end;
	    } else if (!(DllGetClassObject = (DllGetClassObjectFunc)GetProcAddress(hLibrary, "DllGetClassObject"))) {
	        ERR("couldn't find function DllGetClassObject in %s\n", debugstr_w(sDllPath));
	        FreeLibrary( hLibrary );
		hres = E_ACCESSDENIED;
	        goto end;
            } else if (FAILED(hres = DllGetClassObject(myclsid, &IID_IClassFactory, (LPVOID*)&pcf))) {
		    TRACE("GetClassObject failed 0x%08x\n", hres);
		    goto end;
	    }

            hres = IClassFactory_CreateInstance(pcf, pUnkOuter, refiid, ppv);
            IClassFactory_Release(pcf);
	} else {

	    /* load an external dll in the usual way */
	    hres = CoCreateInstance(myclsid, pUnkOuter, CLSCTX_INPROC_SERVER, refiid, ppv);
	}

end:
        if (hKey) RegCloseKey(hKey);
	if(hres!=S_OK)
	{
	  ERR("failed (0x%08x) to create CLSID:%s IID:%s\n",
              hres, shdebugstr_guid(myclsid), shdebugstr_guid(refiid));
	  ERR("class not found in registry\n");
	}

	TRACE("-- instance: %p\n",*ppv);
	return hres;
}

#ifndef __REACTOS__
/*************************************************************************
 * DllGetClassObject     [SHELL32.@]
 * SHDllGetClassObject   [SHELL32.128]
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
	IClassFactory * pcf = NULL;
	HRESULT	hres;
	int i;

	TRACE("CLSID:%s,IID:%s\n",shdebugstr_guid(rclsid),shdebugstr_guid(iid));

	if (!ppv) return E_INVALIDARG;
	*ppv = NULL;

	/* search our internal interface table */
	for(i=0;InterfaceTable[i].clsid;i++) {
	    if(IsEqualIID(InterfaceTable[i].clsid, rclsid)) {
	        TRACE("index[%u]\n", i);
	        pcf = IDefClF_fnConstructor(InterfaceTable[i].lpfnCI, NULL, NULL);
	        break;
	    }
	}

        if (!pcf) {
	    FIXME("failed for CLSID=%s\n", shdebugstr_guid(rclsid));
	    return CLASS_E_CLASSNOTAVAILABLE;
	}

	hres = IClassFactory_QueryInterface(pcf, iid, ppv);
	IClassFactory_Release(pcf);

	TRACE("-- pointer to class factory: %p\n",*ppv);
	return hres;
}
#endif

/*************************************************************************
 * SHCLSIDFromString				[SHELL32.147]
 *
 * Under Windows 9x this was an ANSI version of CLSIDFromString. It also allowed
 * to avoid dependency on ole32.dll (see SHLoadOLE for details).
 *
 * Under Windows NT/2000/XP this is equivalent to CLSIDFromString
 *
 * NOTES
 *     exported by ordinal
 *
 * SEE ALSO
 *     CLSIDFromString, SHLoadOLE
 */
DWORD WINAPI SHCLSIDFromStringA (LPCSTR clsid, CLSID *id)
{
    WCHAR buffer[40];
    TRACE("(%p(%s) %p)\n", clsid, clsid, id);
    if (!MultiByteToWideChar( CP_ACP, 0, clsid, -1, buffer, sizeof(buffer)/sizeof(WCHAR) ))
        return CO_E_CLASSSTRING;
    return CLSIDFromString( buffer, id );
}
DWORD WINAPI SHCLSIDFromStringW (LPCWSTR clsid, CLSID *id)
{
	TRACE("(%p(%s) %p)\n", clsid, debugstr_w(clsid), id);
	return CLSIDFromString(clsid, id);
}
DWORD WINAPI SHCLSIDFromStringAW (LPCVOID clsid, CLSID *id)
{
	if (SHELL_OsIsUnicode())
	  return SHCLSIDFromStringW (clsid, id);
	return SHCLSIDFromStringA (clsid, id);
}

/*************************************************************************
 *			 SHGetMalloc			[SHELL32.@]
 *
 * Equivalent to CoGetMalloc(MEMCTX_TASK, ...). Under Windows 9x this function
 * could use the shell32 built-in "mini-COM" without the need to load ole32.dll -
 * see SHLoadOLE for details. 
 *
 * PARAMS
 *  lpmal [O] Destination for IMalloc interface.
 *
 * RETURNS
 *  Success: S_OK. lpmal contains the shells IMalloc interface.
 *  Failure. An HRESULT error code.
 *
 * SEE ALSO
 *  CoGetMalloc, SHLoadOLE
 */
HRESULT WINAPI SHGetMalloc(LPMALLOC *lpmal)
{
	TRACE("(%p)\n", lpmal);
	return CoGetMalloc(MEMCTX_TASK, lpmal);
}

/*************************************************************************
 * SHAlloc					[SHELL32.196]
 *
 * Equivalent to CoTaskMemAlloc. Under Windows 9x this function could use
 * the shell32 built-in "mini-COM" without the need to load ole32.dll -
 * see SHLoadOLE for details. 
 *
 * NOTES
 *     exported by ordinal
 *
 * SEE ALSO
 *     CoTaskMemAlloc, SHLoadOLE
 */
LPVOID WINAPI SHAlloc(SIZE_T len)
{
	LPVOID ret;

	ret = CoTaskMemAlloc(len);
	TRACE("%u bytes at %p\n",len, ret);
	return ret;
}

/*************************************************************************
 * SHFree					[SHELL32.195]
 *
 * Equivalent to CoTaskMemFree. Under Windows 9x this function could use
 * the shell32 built-in "mini-COM" without the need to load ole32.dll -
 * see SHLoadOLE for details. 
 *
 * NOTES
 *     exported by ordinal
 *
 * SEE ALSO
 *     CoTaskMemFree, SHLoadOLE
 */
void WINAPI SHFree(LPVOID pv)
{
	TRACE("%p\n",pv);
	CoTaskMemFree(pv);
}

#ifndef __REACTOS__
/*************************************************************************
 * SHGetDesktopFolder			[SHELL32.@]
 */
HRESULT WINAPI SHGetDesktopFolder(IShellFolder **psf)
{
	HRESULT	hres;

	TRACE("(%p)\n", psf);

	if(!psf) return E_INVALIDARG;

	*psf = NULL;
	hres = ISF_Desktop_Constructor(NULL, &IID_IShellFolder, (LPVOID*)psf);

	TRACE("-- %p->(%p) 0x%08x\n", psf, *psf, hres);
	return hres;
}
#endif

/**************************************************************************
 * Default ClassFactory Implementation
 *
 * SHCreateDefClassObject
 *
 * NOTES
 *  Helper function for dlls without their own classfactory.
 *  A generic classfactory is returned.
 *  When the CreateInstance of the cf is called the callback is executed.
 */

#ifndef __REACTOS__

typedef struct
{
    IClassFactory               IClassFactory_iface;
    LONG                        ref;
    CLSID			*rclsid;
    LPFNCREATEINSTANCE		lpfnCI;
    const IID *			riidInst;
    LONG *			pcRefDll; /* pointer to refcounter in external dll (ugrrr...) */
} IDefClFImpl;

static inline IDefClFImpl *impl_from_IClassFactory(IClassFactory *iface)
{
	return CONTAINING_RECORD(iface, IDefClFImpl, IClassFactory_iface);
}

static const IClassFactoryVtbl dclfvt;

/**************************************************************************
 *  IDefClF_fnConstructor
 */

static IClassFactory * IDefClF_fnConstructor(LPFNCREATEINSTANCE lpfnCI, PLONG pcRefDll, REFIID riidInst)
{
	IDefClFImpl* lpclf;

	lpclf = HeapAlloc(GetProcessHeap(),0,sizeof(IDefClFImpl));
	lpclf->ref = 1;
	lpclf->IClassFactory_iface.lpVtbl = &dclfvt;
	lpclf->lpfnCI = lpfnCI;
	lpclf->pcRefDll = pcRefDll;

	if (pcRefDll) InterlockedIncrement(pcRefDll);
	lpclf->riidInst = riidInst;

	TRACE("(%p)%s\n",lpclf, shdebugstr_guid(riidInst));
	return (LPCLASSFACTORY)lpclf;
}
/**************************************************************************
 *  IDefClF_fnQueryInterface
 */
static HRESULT WINAPI IDefClF_fnQueryInterface(
  LPCLASSFACTORY iface, REFIID riid, LPVOID *ppvObj)
{
	IDefClFImpl *This = impl_from_IClassFactory(iface);

	TRACE("(%p)->(%s)\n",This,shdebugstr_guid(riid));

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IClassFactory)) {
	  *ppvObj = This;
	  InterlockedIncrement(&This->ref);
	  return S_OK;
	}

	TRACE("-- E_NOINTERFACE\n");
	return E_NOINTERFACE;
}
/******************************************************************************
 * IDefClF_fnAddRef
 */
static ULONG WINAPI IDefClF_fnAddRef(LPCLASSFACTORY iface)
{
	IDefClFImpl *This = impl_from_IClassFactory(iface);
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(count=%u)\n", This, refCount - 1);

	return refCount;
}
/******************************************************************************
 * IDefClF_fnRelease
 */
static ULONG WINAPI IDefClF_fnRelease(LPCLASSFACTORY iface)
{
	IDefClFImpl *This = impl_from_IClassFactory(iface);
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(count=%u)\n", This, refCount + 1);

	if (!refCount)
	{
	  if (This->pcRefDll) InterlockedDecrement(This->pcRefDll);

	  TRACE("-- destroying IClassFactory(%p)\n",This);
	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return refCount;
}
/******************************************************************************
 * IDefClF_fnCreateInstance
 */
static HRESULT WINAPI IDefClF_fnCreateInstance(
  LPCLASSFACTORY iface, LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObject)
{
	IDefClFImpl *This = impl_from_IClassFactory(iface);

	TRACE("%p->(%p,%s,%p)\n",This,pUnkOuter,shdebugstr_guid(riid),ppvObject);

	*ppvObject = NULL;

	if ( This->riidInst==NULL ||
	     IsEqualCLSID(riid, This->riidInst) ||
	     IsEqualCLSID(riid, &IID_IUnknown) )
	{
	  return This->lpfnCI(pUnkOuter, riid, ppvObject);
	}

	ERR("unknown IID requested %s\n",shdebugstr_guid(riid));
	return E_NOINTERFACE;
}
/******************************************************************************
 * IDefClF_fnLockServer
 */
static HRESULT WINAPI IDefClF_fnLockServer(LPCLASSFACTORY iface, BOOL fLock)
{
	IDefClFImpl *This = impl_from_IClassFactory(iface);
	TRACE("%p->(0x%x), not implemented\n",This, fLock);
	return E_NOTIMPL;
}

static const IClassFactoryVtbl dclfvt =
{
    IDefClF_fnQueryInterface,
    IDefClF_fnAddRef,
  IDefClF_fnRelease,
  IDefClF_fnCreateInstance,
  IDefClF_fnLockServer
};

/******************************************************************************
 * SHCreateDefClassObject			[SHELL32.70]
 */
HRESULT WINAPI SHCreateDefClassObject(
	REFIID	riid,
	LPVOID*	ppv,
	LPFNCREATEINSTANCE lpfnCI,	/* [in] create instance callback entry */
	LPDWORD	pcRefDll,		/* [in/out] ref count of the dll */
	REFIID	riidInst)		/* [in] optional interface to the instance */
{
	IClassFactory * pcf;

	TRACE("%s %p %p %p %s\n",
              shdebugstr_guid(riid), ppv, lpfnCI, pcRefDll, shdebugstr_guid(riidInst));

	if (! IsEqualCLSID(riid, &IID_IClassFactory) ) return E_NOINTERFACE;
	if (! (pcf = IDefClF_fnConstructor(lpfnCI, (PLONG)pcRefDll, riidInst))) return E_OUTOFMEMORY;
	*ppv = pcf;
	return S_OK;
}

#endif /* !__REACTOS__ */

/*************************************************************************
 *  DragAcceptFiles		[SHELL32.@]
 */
void WINAPI DragAcceptFiles(HWND hWnd, BOOL b)
{
	LONG exstyle;

	if( !IsWindow(hWnd) ) return;
	exstyle = GetWindowLongPtrA(hWnd,GWL_EXSTYLE);
	if (b)
	  exstyle |= WS_EX_ACCEPTFILES;
	else
	  exstyle &= ~WS_EX_ACCEPTFILES;
	SetWindowLongPtrA(hWnd,GWL_EXSTYLE,exstyle);
}

/*************************************************************************
 * DragFinish		[SHELL32.@]
 */
void WINAPI DragFinish(HDROP h)
{
	TRACE("\n");
	GlobalFree(h);
}

/*************************************************************************
 * DragQueryPoint		[SHELL32.@]
 */
BOOL WINAPI DragQueryPoint(HDROP hDrop, POINT *p)
{
        DROPFILES *lpDropFileStruct;
	BOOL bRet;

	TRACE("\n");

	lpDropFileStruct = GlobalLock(hDrop);

        *p = lpDropFileStruct->pt;
	bRet = lpDropFileStruct->fNC;

	GlobalUnlock(hDrop);
	return bRet;
}

/*************************************************************************
 *  DragQueryFileA		[SHELL32.@]
 *  DragQueryFile 		[SHELL32.@]
 */
UINT WINAPI DragQueryFileA(
	HDROP hDrop,
	UINT lFile,
	LPSTR lpszFile,
	UINT lLength)
{
	LPSTR lpDrop;
	UINT i = 0;
	DROPFILES *lpDropFileStruct = GlobalLock(hDrop);

	TRACE("(%p, %x, %p, %u)\n",	hDrop,lFile,lpszFile,lLength);

	if(!lpDropFileStruct) goto end;

	lpDrop = (LPSTR) lpDropFileStruct + lpDropFileStruct->pFiles;

        if(lpDropFileStruct->fWide) {
            LPWSTR lpszFileW = NULL;

            if(lpszFile && lFile != 0xFFFFFFFF) {
                lpszFileW = HeapAlloc(GetProcessHeap(), 0, lLength*sizeof(WCHAR));
                if(lpszFileW == NULL) {
                    goto end;
                }
            }
            i = DragQueryFileW(hDrop, lFile, lpszFileW, lLength);

            if(lpszFileW) {
                WideCharToMultiByte(CP_ACP, 0, lpszFileW, -1, lpszFile, lLength, 0, NULL);
                HeapFree(GetProcessHeap(), 0, lpszFileW);
            }
            goto end;
        }

	while (i++ < lFile)
	{
	  while (*lpDrop++); /* skip filename */
	  if (!*lpDrop)
	  {
	    i = (lFile == 0xFFFFFFFF) ? i : 0;
	    goto end;
	  }
	}

	i = strlen(lpDrop);
	if (!lpszFile ) goto end;   /* needed buffer size */
	lstrcpynA (lpszFile, lpDrop, lLength);
end:
	GlobalUnlock(hDrop);
	return i;
}

/*************************************************************************
 *  DragQueryFileW		[SHELL32.@]
 */
UINT WINAPI DragQueryFileW(
	HDROP hDrop,
	UINT lFile,
	LPWSTR lpszwFile,
	UINT lLength)
{
	LPWSTR lpwDrop;
	UINT i = 0;
	DROPFILES *lpDropFileStruct = GlobalLock(hDrop);

	TRACE("(%p, %x, %p, %u)\n", hDrop,lFile,lpszwFile,lLength);

	if(!lpDropFileStruct) goto end;

	lpwDrop = (LPWSTR) ((LPSTR)lpDropFileStruct + lpDropFileStruct->pFiles);

        if(lpDropFileStruct->fWide == FALSE) {
            LPSTR lpszFileA = NULL;

            if(lpszwFile && lFile != 0xFFFFFFFF) {
                lpszFileA = HeapAlloc(GetProcessHeap(), 0, lLength);
                if(lpszFileA == NULL) {
                    goto end;
                }
            }
            i = DragQueryFileA(hDrop, lFile, lpszFileA, lLength);

            if(lpszFileA) {
                MultiByteToWideChar(CP_ACP, 0, lpszFileA, -1, lpszwFile, lLength);
                HeapFree(GetProcessHeap(), 0, lpszFileA);
            }
            goto end;
        }

	i = 0;
	while (i++ < lFile)
	{
	  while (*lpwDrop++); /* skip filename */
	  if (!*lpwDrop)
	  {
	    i = (lFile == 0xFFFFFFFF) ? i : 0;
	    goto end;
	  }
	}

	i = strlenW(lpwDrop);
	if ( !lpszwFile) goto end;   /* needed buffer size */
	lstrcpynW (lpszwFile, lpwDrop, lLength);
end:
	GlobalUnlock(hDrop);
	return i;
}

/*************************************************************************
 *  SHPropStgCreate             [SHELL32.685]
 */
HRESULT WINAPI SHPropStgCreate(IPropertySetStorage *psstg, REFFMTID fmtid,
        const CLSID *pclsid, DWORD grfFlags, DWORD grfMode,
        DWORD dwDisposition, IPropertyStorage **ppstg, UINT *puCodePage)
{
    PROPSPEC prop;
    PROPVARIANT ret;
    HRESULT hres;

    TRACE("%p %s %s %x %x %x %p %p\n", psstg, debugstr_guid(fmtid), debugstr_guid(pclsid),
            grfFlags, grfMode, dwDisposition, ppstg, puCodePage);

    hres = IPropertySetStorage_Open(psstg, fmtid, grfMode, ppstg);

    switch(dwDisposition) {
    case CREATE_ALWAYS:
        if(SUCCEEDED(hres)) {
            IPropertyStorage_Release(*ppstg);
            hres = IPropertySetStorage_Delete(psstg, fmtid);
            if(FAILED(hres))
                return hres;
            hres = E_FAIL;
        }

    case OPEN_ALWAYS:
    case CREATE_NEW:
        if(FAILED(hres))
            hres = IPropertySetStorage_Create(psstg, fmtid, pclsid,
                    grfFlags, grfMode, ppstg);

    case OPEN_EXISTING:
        if(FAILED(hres))
            return hres;

        if(puCodePage) {
            prop.ulKind = PRSPEC_PROPID;
            prop.u.propid = PID_CODEPAGE;
            hres = IPropertyStorage_ReadMultiple(*ppstg, 1, &prop, &ret);
            if(FAILED(hres) || ret.vt!=VT_I2)
                *puCodePage = 0;
            else
                *puCodePage = ret.u.iVal;
        }
    }

    return S_OK;
}

/*************************************************************************
 *  SHPropStgReadMultiple       [SHELL32.688]
 */
HRESULT WINAPI SHPropStgReadMultiple(IPropertyStorage *pps, UINT uCodePage,
        ULONG cpspec, const PROPSPEC *rgpspec, PROPVARIANT *rgvar)
{
    STATPROPSETSTG stat;
    HRESULT hres;

    FIXME("%p %u %u %p %p\n", pps, uCodePage, cpspec, rgpspec, rgvar);

    memset(rgvar, 0, cpspec*sizeof(PROPVARIANT));
    hres = IPropertyStorage_ReadMultiple(pps, cpspec, rgpspec, rgvar);
    if(FAILED(hres))
        return hres;

    if(!uCodePage) {
        PROPSPEC prop;
        PROPVARIANT ret;

        prop.ulKind = PRSPEC_PROPID;
        prop.u.propid = PID_CODEPAGE;
        hres = IPropertyStorage_ReadMultiple(pps, 1, &prop, &ret);
        if(FAILED(hres) || ret.vt!=VT_I2)
            return S_OK;

        uCodePage = ret.u.iVal;
    }

    hres = IPropertyStorage_Stat(pps, &stat);
    if(FAILED(hres))
        return S_OK;

    /* TODO: do something with codepage and stat */
    return S_OK;
}

/*************************************************************************
 *  SHPropStgWriteMultiple      [SHELL32.689]
 */
HRESULT WINAPI SHPropStgWriteMultiple(IPropertyStorage *pps, UINT *uCodePage,
        ULONG cpspec, const PROPSPEC *rgpspec, PROPVARIANT *rgvar, PROPID propidNameFirst)
{
    STATPROPSETSTG stat;
    UINT codepage;
    HRESULT hres;

    FIXME("%p %p %u %p %p %d\n", pps, uCodePage, cpspec, rgpspec, rgvar, propidNameFirst);

    hres = IPropertyStorage_Stat(pps, &stat);
    if(FAILED(hres))
        return hres;

    if(uCodePage && *uCodePage)
        codepage = *uCodePage;
    else {
        PROPSPEC prop;
        PROPVARIANT ret;

        prop.ulKind = PRSPEC_PROPID;
        prop.u.propid = PID_CODEPAGE;
        hres = IPropertyStorage_ReadMultiple(pps, 1, &prop, &ret);
        if(FAILED(hres))
            return hres;
        if(ret.vt!=VT_I2 || !ret.u.iVal)
            return E_FAIL;

        codepage = ret.u.iVal;
        if(uCodePage)
            *uCodePage = codepage;
    }

    /* TODO: do something with codepage and stat */

    hres = IPropertyStorage_WriteMultiple(pps, cpspec, rgpspec, rgvar, propidNameFirst);
    return hres;
}

/*************************************************************************
 *  SHCreateQueryCancelAutoPlayMoniker [SHELL32.@]
 */
HRESULT WINAPI SHCreateQueryCancelAutoPlayMoniker(IMoniker **moniker)
{
    TRACE("%p\n", moniker);

    if (!moniker) return E_INVALIDARG;
    return CreateClassMoniker(&CLSID_QueryCancelAutoPlay, moniker);
}

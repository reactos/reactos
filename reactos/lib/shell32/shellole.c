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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "shellapi.h"
#include "wingdi.h"
#include "winuser.h"
#include "shlobj.h"
#include "shlguid.h"
#include "winreg.h"
#include "winerror.h"

#include "undocshell.h"
#include "wine/unicode.h"
#include "shell32_main.h"

#include "wine/debug.h"
#include "shlwapi.h"
#include "debughlp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

extern HRESULT WINAPI IFSFolder_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);

const WCHAR sShell32[12] = {'S','H','E','L','L','3','2','.','D','L','L','\0'};
const WCHAR sOLE32[10] = {'O','L','E','3','2','.','D','L','L','\0'};

HINSTANCE hShellOle32 = 0;
/**************************************************************************
 * Default ClassFactory types
 */
typedef HRESULT (CALLBACK *LPFNCREATEINSTANCE)(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObject);
IClassFactory * IDefClF_fnConstructor(LPFNCREATEINSTANCE lpfnCI, PLONG pcRefDll, REFIID riidInst);

/* this table contains all CLSID's of shell32 objects */
struct {
	REFIID			riid;
	LPFNCREATEINSTANCE	lpfnCI;
} InterfaceTable[] = {
	{&CLSID_ShellFSFolder,	&IFSFolder_Constructor},
	{&CLSID_MyComputer,	&ISF_MyComputer_Constructor},
	{&CLSID_ShellDesktop,	&ISF_Desktop_Constructor},
	{&CLSID_ShellLink,	&IShellLink_Constructor},
	{&CLSID_DragDropHelper, &IDropTargetHelper_Constructor},
	{&CLSID_ControlPanel,	&IControlPanel_Constructor},
	{NULL,NULL}
};

/*************************************************************************
 * __CoCreateInstance [internal]
 *
 * NOTES
 *   wraper for late bound call to OLE32.DLL
 *
 */
HRESULT (WINAPI *pCoCreateInstance)(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID iid, LPVOID *ppv) = NULL;

void * __GetExternalFunc(HMODULE * phModule, LPCWSTR szModuleName, LPCSTR szProcName)
{
	if (!*phModule) *phModule = GetModuleHandleW(szModuleName);
	if (!*phModule) *phModule = LoadLibraryW(szModuleName);
	if (*phModule) return GetProcAddress(*phModule, szProcName);
	return NULL;
}

HRESULT  __CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID iid, LPVOID *ppv)
{
	if(!pCoCreateInstance) pCoCreateInstance = __GetExternalFunc(&hShellOle32, sOLE32, "CoCreateInstance");
	if(!pCoCreateInstance) return E_FAIL;
	return pCoCreateInstance(rclsid, pUnkOuter, dwClsContext, iid, ppv);
}

/*************************************************************************
 * SHCoCreateInstance [SHELL32.102]
 *
 * NOTES
 *     exported by ordinal
 */

/* FIXME: this should be SHLWAPI.24 since we can't yet import by ordinal */

DWORD WINAPI __SHGUIDToStringW (REFGUID guid, LPWSTR str)
{
    WCHAR sFormat[52] = {'{','%','0','8','l','x','-','%','0','4',
		         'x','-','%','0','4','x','-','%','0','2',
                         'x','%','0','2','x','-','%','0','2','x',
			 '%','0','2','x','%','0','2','x','%','0',
			 '2','x','%','0','2','x','%','0','2','x',
			 '}','\0'};

    return wsprintfW ( str, sFormat,
             guid->Data1, guid->Data2, guid->Data3,
             guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
             guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7] );

}

/************************************************************************/

LRESULT WINAPI SHCoCreateInstance(
	LPCWSTR aclsid,
	const CLSID *clsid,
	LPUNKNOWN pUnkOuter,
	REFIID refiid,
	LPVOID *ppv)
{
	DWORD	hres;
	IID	iid;
	CLSID * myclsid = (CLSID*)clsid;
	WCHAR	sKeyName[MAX_PATH];
	const	WCHAR sCLSID[7] = {'C','L','S','I','D','\\','\0'};
	WCHAR	sClassID[60];
	const WCHAR sInProcServer32[16] ={'\\','I','n','p','r','o','c','S','e','r','v','e','r','3','2','\0'};
	const WCHAR sLoadWithoutCOM[15] ={'L','o','a','d','W','i','t','h','o','u','t','C','O','M','\0'};
	WCHAR	sDllPath[MAX_PATH];
	HKEY	hKey;
	DWORD	dwSize;
	BOOLEAN bLoadFromShell32 = FALSE;
	BOOLEAN bLoadWithoutCOM = FALSE;
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

	/* we look up the dll path in the registry */
        __SHGUIDToStringW(myclsid, sClassID);
	lstrcpyW(sKeyName, sCLSID);
	lstrcatW(sKeyName, sClassID);
	lstrcatW(sKeyName, sInProcServer32);

	if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_CLASSES_ROOT, sKeyName, 0, KEY_READ, &hKey)) {
	    dwSize = sizeof(sDllPath);
	    SHQueryValueExW(hKey, NULL, 0,0, sDllPath, &dwSize );

	    /* if a special registry key is set, we load a shell extension without help of OLE32 */
	    bLoadWithoutCOM = (ERROR_SUCCESS == SHQueryValueExW(hKey, sLoadWithoutCOM, 0, 0, 0, 0));

	    /* if the com object is inside shell32, omit use of ole32 */
	    bLoadFromShell32 = (0==lstrcmpiW( PathFindFileNameW(sDllPath), sShell32));

	    RegCloseKey (hKey);
	} else {
	    /* since we can't find it in the registry we try internally */
	    bLoadFromShell32 = TRUE;
	}

	TRACE("WithoutCom=%u FromShell=%u\n", bLoadWithoutCOM, bLoadFromShell32);

	/* now we create a instance */
	if (bLoadFromShell32) {
	    if (! SUCCEEDED(SHELL32_DllGetClassObject(myclsid, &IID_IClassFactory,(LPVOID*)&pcf))) {
	        ERR("LoadFromShell failed for CLSID=%s\n", shdebugstr_guid(myclsid));
	    }
	} else if (bLoadWithoutCOM) {

	    /* load a external dll without ole32 */
	    HANDLE hLibrary;
	    typedef HRESULT (CALLBACK *DllGetClassObjectFunc)(REFCLSID clsid, REFIID iid, LPVOID *ppv);
	    DllGetClassObjectFunc DllGetClassObject;

	    if ((hLibrary = LoadLibraryExW(sDllPath, 0, LOAD_WITH_ALTERED_SEARCH_PATH)) == 0) {
		ERR("couldn't load InprocServer32 dll %s\n", debugstr_w(sDllPath));
		hres = E_ACCESSDENIED;
		goto end;
	    } else if (!(DllGetClassObject = (DllGetClassObjectFunc)GetProcAddress(hLibrary, "DllGetClassObject"))) {
		ERR("couldn't find function DllGetClassObject in %s\n", debugstr_w(sDllPath));
		FreeLibrary( hLibrary );
		hres = E_ACCESSDENIED;
		goto end;
	    } else if (! SUCCEEDED(hres = DllGetClassObject(myclsid, &IID_IClassFactory, (LPVOID*)&pcf))) {
		TRACE("GetClassObject failed 0x%08lx\n", hres);
		goto end;
	    }

	} else {

	    /* load a external dll in the usual way */
	    hres = __CoCreateInstance(myclsid, pUnkOuter, CLSCTX_INPROC_SERVER, refiid, ppv);
	    goto end;
	}

	/* here we should have a ClassFactory */
	if (!pcf) return E_ACCESSDENIED;

	hres = IClassFactory_CreateInstance(pcf, pUnkOuter, refiid, ppv);
	IClassFactory_Release(pcf);
end:
	if(hres!=S_OK)
	{
	  ERR("failed (0x%08lx) to create CLSID:%s IID:%s\n",
              hres, shdebugstr_guid(myclsid), shdebugstr_guid(refiid));
	  ERR("class not found in registry\n");
	}

	TRACE("-- instance: %p\n",*ppv);
	return hres;
}

/*************************************************************************
 * DllGetClassObject   [SHELL32.128]
 */
HRESULT WINAPI SHELL32_DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
	HRESULT	hres = E_OUTOFMEMORY;
	IClassFactory * pcf = NULL;
	int i;

	TRACE("CLSID:%s,IID:%s\n",shdebugstr_guid(rclsid),shdebugstr_guid(iid));

	if (!ppv) return E_INVALIDARG;
	*ppv = NULL;

	/* search our internal interface table */
	for(i=0;InterfaceTable[i].riid;i++) {
	    if(IsEqualIID(InterfaceTable[i].riid, rclsid)) {
	        TRACE("index[%u]\n", i);
	        pcf = IDefClF_fnConstructor(InterfaceTable[i].lpfnCI, NULL, NULL);
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

/*************************************************************************
 * SHCLSIDFromString				[SHELL32.147]
 *
 * NOTES
 *     exported by ordinal
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
	return CLSIDFromString((LPWSTR)clsid, id);
}
DWORD WINAPI SHCLSIDFromStringAW (LPVOID clsid, CLSID *id)
{
	if (SHELL_OsIsUnicode())
	  return SHCLSIDFromStringW (clsid, id);
	return SHCLSIDFromStringA (clsid, id);
}

/*************************************************************************
 *	Shell Memory Allocator
 */

/* set the vtable later */
extern ICOM_VTABLE(IMalloc) VT_Shell_IMalloc32;

/* this is the static object instance */
typedef struct {
	ICOM_VFIELD(IMalloc);
	DWORD dummy;
} _ShellMalloc;

_ShellMalloc Shell_Malloc = { &VT_Shell_IMalloc32,1};

/* this is the global allocator of shell32 */
IMalloc * ShellTaskAllocator = NULL;

/******************************************************************************
 *              IShellMalloc_QueryInterface        [VTABLE]
 */
static HRESULT WINAPI IShellMalloc_fnQueryInterface(LPMALLOC iface, REFIID refiid, LPVOID *obj)
{
	TRACE("(%s,%p)\n",shdebugstr_guid(refiid),obj);
	if (IsEqualIID(refiid, &IID_IUnknown) || IsEqualIID(refiid, &IID_IMalloc)) {
		*obj = (LPMALLOC) &Shell_Malloc;
		return S_OK;
	}
	return E_NOINTERFACE;
}

/******************************************************************************
 *              IShellMalloc_AddRefRelease        [VTABLE]
 */
static ULONG WINAPI IShellMalloc_fnAddRefRelease(LPMALLOC iface)
{
        return 1;
}

/******************************************************************************
 *		IShellMalloc_Alloc [VTABLE]
 */
static LPVOID WINAPI IShellMalloc_fnAlloc(LPMALLOC iface, DWORD cb)
{
        LPVOID addr;

	addr = (LPVOID) LocalAlloc(GMEM_ZEROINIT, cb);
        TRACE("(%p,%ld);\n",addr,cb);
        return addr;
}

/******************************************************************************
 *		IShellMalloc_Realloc [VTABLE]
 */
static LPVOID WINAPI IShellMalloc_fnRealloc(LPMALLOC iface, LPVOID pv, DWORD cb)
{
        LPVOID addr;

	if (pv) {
		if (cb) {
			addr = (LPVOID) LocalReAlloc((HANDLE) pv, cb, GMEM_ZEROINIT | GMEM_MOVEABLE);
		} else {
			LocalFree((HANDLE) pv);
			addr = NULL;
		}
	} else {
		if (cb) {
			addr = (LPVOID) LocalAlloc(GMEM_ZEROINIT, cb);
		} else {
			addr = NULL;
		}
	}

        TRACE("(%p->%p,%ld)\n",pv,addr,cb);
        return addr;
}

/******************************************************************************
 *		IShellMalloc_Free [VTABLE]
 */
static VOID WINAPI IShellMalloc_fnFree(LPMALLOC iface, LPVOID pv)
{
        TRACE("(%p)\n",pv);
	LocalFree((HANDLE) pv);
}

/******************************************************************************
 *		IShellMalloc_GetSize [VTABLE]
 */
static DWORD WINAPI IShellMalloc_fnGetSize(LPMALLOC iface, LPVOID pv)
{
        DWORD cb = (DWORD) LocalSize((HANDLE)pv);
        TRACE("(%p,%ld)\n", pv, cb);
	return cb;
}

/******************************************************************************
 *		IShellMalloc_DidAlloc [VTABLE]
 */
static INT WINAPI IShellMalloc_fnDidAlloc(LPMALLOC iface, LPVOID pv)
{
        TRACE("(%p)\n",pv);
        return -1;
}

/******************************************************************************
 * 		IShellMalloc_HeapMinimize [VTABLE]
 */
static VOID WINAPI IShellMalloc_fnHeapMinimize(LPMALLOC iface)
{
	TRACE("()\n");
}

static ICOM_VTABLE(IMalloc) VT_Shell_IMalloc32 =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IShellMalloc_fnQueryInterface,
	IShellMalloc_fnAddRefRelease,
	IShellMalloc_fnAddRefRelease,
	IShellMalloc_fnAlloc,
	IShellMalloc_fnRealloc,
	IShellMalloc_fnFree,
	IShellMalloc_fnGetSize,
	IShellMalloc_fnDidAlloc,
	IShellMalloc_fnHeapMinimize
};

/*************************************************************************
 *			 SHGetMalloc			[SHELL32.@]
 *
 * Return the shell IMalloc interface.
 *
 * PARAMS
 *  lpmal [O] Destination for IMalloc interface.
 *
 * RETURNS
 *  Success: S_OK. lpmal contains the shells IMalloc interface.
 *  Failure. An HRESULT error code.
 *
 * NOTES
 *  This function will use CoGetMalloc() if OLE32.DLL is already loaded.
 *  If not it uses an internal implementation as a fallback.
 */
HRESULT WINAPI SHGetMalloc(LPMALLOC *lpmal)
{
	HRESULT (WINAPI *pCoGetMalloc)(DWORD,LPMALLOC *);
	HMODULE hOle32;

	TRACE("(%p)\n", lpmal);

	if (!ShellTaskAllocator)
	{
		hOle32 = GetModuleHandleA("OLE32.DLL");
		if(hOle32) {
			pCoGetMalloc = (void*) GetProcAddress(hOle32, "CoGetMalloc");
			if (pCoGetMalloc) pCoGetMalloc(MEMCTX_TASK, &ShellTaskAllocator);
			TRACE("got ole32 IMalloc\n");
		}
		if(!ShellTaskAllocator) {
			ShellTaskAllocator = (IMalloc* ) &Shell_Malloc;
			TRACE("use fallback allocator\n");
		}
	}
	*lpmal = ShellTaskAllocator;
	return  S_OK;
}

/*************************************************************************
 * SHAlloc					[SHELL32.196]
 *
 * NOTES
 *     exported by ordinal
 */
LPVOID WINAPI SHAlloc(DWORD len)
{
	IMalloc * ppv;
	LPBYTE ret;

	if (!ShellTaskAllocator) SHGetMalloc(&ppv);

	ret = (LPVOID) IMalloc_Alloc(ShellTaskAllocator, len);
	TRACE("%lu bytes at %p\n",len, ret);
	return (LPVOID)ret;
}

/*************************************************************************
 * SHFree					[SHELL32.195]
 *
 * NOTES
 *     exported by ordinal
 */
void WINAPI SHFree(LPVOID pv)
{
	IMalloc * ppv;

	TRACE("%p\n",pv);
	if (!ShellTaskAllocator) SHGetMalloc(&ppv);
	IMalloc_Free(ShellTaskAllocator, pv);
}

/*************************************************************************
 * SHGetDesktopFolder			[SHELL32.@]
 */
HRESULT WINAPI SHGetDesktopFolder(IShellFolder **psf)
{
	HRESULT	hres = S_OK;
	TRACE("\n");

	if(!psf) return E_INVALIDARG;
	*psf = NULL;
	hres = ISF_Desktop_Constructor(NULL, &IID_IShellFolder,(LPVOID*)psf);

	TRACE("-- %p->(%p)\n",psf, *psf);
	return hres;
}
/**************************************************************************
 * Default ClassFactory Implementation
 *
 * SHCreateDefClassObject
 *
 * NOTES
 *  helper function for dll's without a own classfactory
 *  a generic classfactory is returned
 *  when the CreateInstance of the cf is called the callback is executed
 */

typedef struct
{
    ICOM_VFIELD(IClassFactory);
    DWORD                       ref;
    CLSID			*rclsid;
    LPFNCREATEINSTANCE		lpfnCI;
    const IID *			riidInst;
    ULONG *			pcRefDll; /* pointer to refcounter in external dll (ugrrr...) */
} IDefClFImpl;

static ICOM_VTABLE(IClassFactory) dclfvt;

/**************************************************************************
 *  IDefClF_fnConstructor
 */

IClassFactory * IDefClF_fnConstructor(LPFNCREATEINSTANCE lpfnCI, PLONG pcRefDll, REFIID riidInst)
{
	IDefClFImpl* lpclf;

	lpclf = (IDefClFImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IDefClFImpl));
	lpclf->ref = 1;
	lpclf->lpVtbl = &dclfvt;
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
	ICOM_THIS(IDefClFImpl,iface);

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
	ICOM_THIS(IDefClFImpl,iface);
	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	return InterlockedIncrement(&This->ref);
}
/******************************************************************************
 * IDefClF_fnRelease
 */
static ULONG WINAPI IDefClF_fnRelease(LPCLASSFACTORY iface)
{
	ICOM_THIS(IDefClFImpl,iface);
	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	if (!InterlockedDecrement(&This->ref))
	{
	  if (This->pcRefDll) InterlockedDecrement(This->pcRefDll);

	  TRACE("-- destroying IClassFactory(%p)\n",This);
	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}
/******************************************************************************
 * IDefClF_fnCreateInstance
 */
static HRESULT WINAPI IDefClF_fnCreateInstance(
  LPCLASSFACTORY iface, LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObject)
{
	ICOM_THIS(IDefClFImpl,iface);

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
	ICOM_THIS(IDefClFImpl,iface);
	TRACE("%p->(0x%x), not implemented\n",This, fLock);
	return E_NOTIMPL;
}

static ICOM_VTABLE(IClassFactory) dclfvt =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
	if (! (pcf = IDefClF_fnConstructor(lpfnCI, pcRefDll, riidInst))) return E_OUTOFMEMORY;
	*ppv = pcf;
	return NOERROR;
}

/*************************************************************************
 *  DragAcceptFiles		[SHELL32.54]
 */
void WINAPI DragAcceptFiles(HWND hWnd, BOOL b)
{
	LONG exstyle;

	if( !IsWindow(hWnd) ) return;
	exstyle = GetWindowLongA(hWnd,GWL_EXSTYLE);
	if (b)
	  exstyle |= WS_EX_ACCEPTFILES;
	else
	  exstyle &= ~WS_EX_ACCEPTFILES;
	SetWindowLongA(hWnd,GWL_EXSTYLE,exstyle);
}

/*************************************************************************
 * DragFinish		[SHELL32.80]
 */
void WINAPI DragFinish(HDROP h)
{
	TRACE("\n");
	GlobalFree((HGLOBAL)h);
}

/*************************************************************************
 * DragQueryPoint		[SHELL32.135]
 */
BOOL WINAPI DragQueryPoint(HDROP hDrop, POINT *p)
{
        DROPFILES *lpDropFileStruct;
	BOOL bRet;

	TRACE("\n");

	lpDropFileStruct = (DROPFILES *) GlobalLock(hDrop);

        *p = lpDropFileStruct->pt;
	bRet = lpDropFileStruct->fNC;

	GlobalUnlock(hDrop);
	return bRet;
}

/*************************************************************************
 *  DragQueryFile 		[SHELL32.81]
 *  DragQueryFileA		[SHELL32.82]
 */
UINT WINAPI DragQueryFileA(
	HDROP hDrop,
	UINT lFile,
	LPSTR lpszFile,
	UINT lLength)
{
	LPSTR lpDrop;
	UINT i = 0;
	DROPFILES *lpDropFileStruct = (DROPFILES *) GlobalLock(hDrop);

	TRACE("(%p, %x, %p, %u)\n",	hDrop,lFile,lpszFile,lLength);

	if(!lpDropFileStruct) goto end;

	lpDrop = (LPSTR) lpDropFileStruct + lpDropFileStruct->pFiles;

        if(lpDropFileStruct->fWide == TRUE) {
            LPWSTR lpszFileW = NULL;

            if(lpszFile) {
                lpszFileW = (LPWSTR) HeapAlloc(GetProcessHeap(), 0, lLength*sizeof(WCHAR));
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
	i++;
	if (!lpszFile ) goto end;   /* needed buffer size */
	i = (lLength > i) ? i : lLength;
	lstrcpynA (lpszFile,  lpDrop,  i);
end:
	GlobalUnlock(hDrop);
	return i;
}

/*************************************************************************
 *  DragQueryFileW		[SHELL32.133]
 */
UINT WINAPI DragQueryFileW(
	HDROP hDrop,
	UINT lFile,
	LPWSTR lpszwFile,
	UINT lLength)
{
	LPWSTR lpwDrop;
	UINT i = 0;
	DROPFILES *lpDropFileStruct = (DROPFILES *) GlobalLock(hDrop);

	TRACE("(%p, %x, %p, %u)\n", hDrop,lFile,lpszwFile,lLength);

	if(!lpDropFileStruct) goto end;

	lpwDrop = (LPWSTR) ((LPSTR)lpDropFileStruct + lpDropFileStruct->pFiles);

        if(lpDropFileStruct->fWide == FALSE) {
            LPSTR lpszFileA = NULL;

            if(lpszwFile) {
                lpszFileA = (LPSTR) HeapAlloc(GetProcessHeap(), 0, lLength);
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
	i++;
	if ( !lpszwFile) goto end;   /* needed buffer size */

	i = (lLength > i) ? i : lLength;
	lstrcpynW (lpszwFile, lpwDrop, i);
end:
	GlobalUnlock(hDrop);
	return i;
}

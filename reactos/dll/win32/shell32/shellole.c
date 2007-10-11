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

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define COBJMACROS

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

static const WCHAR sShell32[12] = {'S','H','E','L','L','3','2','.','D','L','L','\0'};

/**************************************************************************
 * Default ClassFactory types
 */
typedef HRESULT (CALLBACK *LPFNCREATEINSTANCE)(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObject);
static IClassFactory * IDefClF_fnConstructor(LPFNCREATEINSTANCE lpfnCI, PLONG pcRefDll, REFIID riidInst);

/* this table contains all CLSID's of shell32 objects */
static const struct {
	REFIID			riid;
	LPFNCREATEINSTANCE	lpfnCI;
} InterfaceTable[] = {
	{&CLSID_ShellFSFolder,	&IFSFolder_Constructor},
	{&CLSID_MyComputer,	&ISF_MyComputer_Constructor},
	{&CLSID_ShellDesktop,	&ISF_Desktop_Constructor},
	{&CLSID_ShellLink,	&IShellLink_Constructor},
	{&CLSID_DragDropHelper, &IDropTargetHelper_Constructor},
	{&CLSID_ControlPanel,	&IControlPanel_Constructor},
	{&CLSID_AutoComplete,   &IAutoComplete_Constructor},
#if 0
	{&CLSID_UnixFolder,     &UnixFolder_Constructor},
	{&CLSID_UnixDosFolder,  &UnixDosFolder_Constructor},
	{&CLSID_FolderShortcut, &FolderShortcut_Constructor},
	{&CLSID_MyDocuments,    &MyDocuments_Constructor},
	{&CLSID_RecycleBin,     &RecycleBin_Constructor},
#endif
	{NULL,NULL}
};


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
 *     CoCreateInstace, SHLoadOLE
 */
HRESULT WINAPI SHCoCreateInstance(
	LPCWSTR aclsid,
	const CLSID *clsid,
	LPUNKNOWN pUnkOuter,
	REFIID refiid,
	LPVOID *ppv)
{
	DWORD	hres;
	IID	iid;
	const	CLSID * myclsid = clsid;
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

	/* now we create an instance */
	if (bLoadFromShell32) {
	    if (! SUCCEEDED(DllGetClassObject(myclsid, &IID_IClassFactory,(LPVOID*)&pcf))) {
	        ERR("LoadFromShell failed for CLSID=%s\n", shdebugstr_guid(myclsid));
	    }
	} else if (bLoadWithoutCOM) {

	    /* load an external dll without ole32 */
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
		    TRACE("GetClassObject failed 0x%08x\n", hres);
		    goto end;
	    }

	} else {

	    /* load an external dll in the usual way */
	    hres = CoCreateInstance(myclsid, pUnkOuter, CLSCTX_INPROC_SERVER, refiid, ppv);
	    goto end;
	}

	/* here we should have a ClassFactory */
	if (!pcf) return E_ACCESSDENIED;

	hres = IClassFactory_CreateInstance(pcf, pUnkOuter, refiid, ppv);
	IClassFactory_Release(pcf);
end:
	if(hres!=S_OK)
	{
	  ERR("failed (0x%08x) to create CLSID:%s IID:%s\n",
              hres, shdebugstr_guid(myclsid), shdebugstr_guid(refiid));
	  ERR("class not found in registry\n");
	}

	TRACE("-- instance: %p\n",*ppv);
	return hres;
}

/*************************************************************************
 * DllGetClassObject     [SHELL32.@]
 * SHDllGetClassObject   [SHELL32.128]
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
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
	return CLSIDFromString((LPWSTR)clsid, id);
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
LPVOID WINAPI SHAlloc(DWORD len)
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
 *  Helper function for dlls without their own classfactory.
 *  A generic classfactory is returned.
 *  When the CreateInstance of the cf is called the callback is executed.
 */

typedef struct
{
    const IClassFactoryVtbl    *lpVtbl;
    LONG                        ref;
    CLSID			*rclsid;
    LPFNCREATEINSTANCE		lpfnCI;
    const IID *			riidInst;
    LONG *			pcRefDll; /* pointer to refcounter in external dll (ugrrr...) */
} IDefClFImpl;

static const IClassFactoryVtbl dclfvt;

/**************************************************************************
 *  IDefClF_fnConstructor
 */

static IClassFactory * IDefClF_fnConstructor(LPFNCREATEINSTANCE lpfnCI, PLONG pcRefDll, REFIID riidInst)
{
	IDefClFImpl* lpclf;

	lpclf = HeapAlloc(GetProcessHeap(),0,sizeof(IDefClFImpl));
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
	IDefClFImpl *This = (IDefClFImpl *)iface;

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
	IDefClFImpl *This = (IDefClFImpl *)iface;
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(count=%u)\n", This, refCount - 1);

	return refCount;
}
/******************************************************************************
 * IDefClF_fnRelease
 */
static ULONG WINAPI IDefClF_fnRelease(LPCLASSFACTORY iface)
{
	IDefClFImpl *This = (IDefClFImpl *)iface;
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
	IDefClFImpl *This = (IDefClFImpl *)iface;

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
	IDefClFImpl *This = (IDefClFImpl *)iface;
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
	return NOERROR;
}

/*************************************************************************
 *  DragAcceptFiles		[SHELL32.@]
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
 * DragFinish		[SHELL32.@]
 */
void WINAPI DragFinish(HDROP h)
{
	TRACE("\n");
	GlobalFree((HGLOBAL)h);
}

/*************************************************************************
 * DragQueryPoint		[SHELL32.@]
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
	DROPFILES *lpDropFileStruct = (DROPFILES *) GlobalLock(hDrop);

	TRACE("(%p, %x, %p, %u)\n",	hDrop,lFile,lpszFile,lLength);

	if(!lpDropFileStruct) goto end;

	lpDrop = (LPSTR) lpDropFileStruct + lpDropFileStruct->pFiles;

        if(lpDropFileStruct->fWide) {
            LPWSTR lpszFileW = NULL;

            if(lpszFile) {
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
	DROPFILES *lpDropFileStruct = (DROPFILES *) GlobalLock(hDrop);

	TRACE("(%p, %x, %p, %u)\n", hDrop,lFile,lpszwFile,lLength);

	if(!lpDropFileStruct) goto end;

	lpwDrop = (LPWSTR) ((LPSTR)lpDropFileStruct + lpDropFileStruct->pFiles);

        if(lpDropFileStruct->fWide == FALSE) {
            LPSTR lpszFileA = NULL;

            if(lpszwFile) {
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

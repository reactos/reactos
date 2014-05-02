/*
 *    handling of SHELL32.DLL OLE-Objects
 *
 *    Copyright 1997    Marcus Meissner
 *    Copyright 1998    Juergen Schmied  <juergen.schmied@metronet.de>
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

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

static const WCHAR sShell32[12] = {'S','H','E','L','L','3','2','.','D','L','L','\0'};

/**************************************************************************
 * Default ClassFactory types
 */
typedef HRESULT (CALLBACK *LPFNCREATEINSTANCE)(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObject);
HRESULT IDefClF_fnConstructor(LPFNCREATEINSTANCE lpfnCI, PLONG pcRefDll, const IID *riidInst, IClassFactory **theFactory);

/* FIXME: this should be SHLWAPI.24 since we can't yet import by ordinal */

DWORD WINAPI __SHGUIDToStringW (REFGUID guid, LPWSTR str)
{
    WCHAR sFormat[52] = {'{','%','0','8','l','x','-','%','0','4',
                 'x','-','%','0','4','x','-','%','0','2',
                         'x','%','0','2','x','-','%','0','2','x',
             '%','0','2','x','%','0','2','x','%','0',
             '2','x','%','0','2','x','%','0','2','x',
             '}','\0'};

    return swprintf ( str, sFormat,
             guid.Data1, guid.Data2, guid.Data3,
             guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
             guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7] );

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
    DWORD    hres;
    CLSID    iid;
    const    CLSID * myclsid = clsid;
    WCHAR    sKeyName[MAX_PATH];
    const    WCHAR sCLSID[7] = {'C','L','S','I','D','\\','\0'};
    WCHAR    sClassID[60];
    const WCHAR sInProcServer32[16] ={'\\','I','n','p','r','o','c','S','e','r','v','e','r','3','2','\0'};
    const WCHAR sLoadWithoutCOM[15] ={'L','o','a','d','W','i','t','h','o','u','t','C','O','M','\0'};
    WCHAR    sDllPath[MAX_PATH];
    HKEY    hKey;
    DWORD    dwSize;
    BOOLEAN bLoadFromShell32 = FALSE;
    BOOLEAN bLoadWithoutCOM = FALSE;
    CComPtr<IClassFactory>        pcf;

    if(!ppv) return E_POINTER;
    *ppv=NULL;

    /* if the clsid is a string, convert it */
    if (!clsid)
    {
      if (!aclsid) return REGDB_E_CLASSNOTREG;
      CLSIDFromString((LPOLESTR)aclsid, &iid);
      myclsid = &iid;
    }

    TRACE("(%p,%s,unk:%p,%s,%p)\n",
        aclsid, shdebugstr_guid(myclsid), pUnkOuter, shdebugstr_guid(&refiid), ppv);

    /* we look up the dll path in the registry */
        __SHGUIDToStringW(*myclsid, sClassID);
    wcscpy(sKeyName, sCLSID);
    wcscat(sKeyName, sClassID);
    wcscat(sKeyName, sInProcServer32);

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
        if (! SUCCEEDED(DllGetClassObject(*myclsid, IID_PPV_ARG(IClassFactory, &pcf)))) {
            ERR("LoadFromShell failed for CLSID=%s\n", shdebugstr_guid(myclsid));
        }
    } else if (bLoadWithoutCOM) {

        /* load an external dll without ole32 */
        HINSTANCE hLibrary;
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
        } else if (! SUCCEEDED(hres = DllGetClassObject(*myclsid, IID_IClassFactory, (LPVOID*)&pcf))) {
            TRACE("GetClassObject failed 0x%08x\n", hres);
            goto end;
        }

    } else {

        /* load an external dll in the usual way */
        hres = CoCreateInstance(*myclsid, pUnkOuter, CLSCTX_INPROC_SERVER, refiid, ppv);
        goto end;
    }

    /* here we should have a ClassFactory */
    if (!pcf) return E_ACCESSDENIED;

    hres = pcf->CreateInstance(pUnkOuter, refiid, ppv);
end:
    if(hres!=S_OK)
    {
      ERR("failed (0x%08x) to create CLSID:%s IID:%s\n",
              hres, shdebugstr_guid(myclsid), shdebugstr_guid(&refiid));
      ERR("class not found in registry\n");
    }

    TRACE("-- instance: %p\n",*ppv);
    return hres;
}

/*************************************************************************
 * SHCLSIDFromString                [SHELL32.147]
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

EXTERN_C DWORD WINAPI SHCLSIDFromStringAW (LPCVOID clsid, CLSID *id)
{
    if (SHELL_OsIsUnicode())
      return SHCLSIDFromStringW ((LPCWSTR)clsid, id);
    return SHCLSIDFromStringA ((LPCSTR)clsid, id);
}

/*************************************************************************
 *             SHGetMalloc            [SHELL32.@]
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
 * SHAlloc                    [SHELL32.196]
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
 * SHFree                    [SHELL32.195]
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
 * SHGetDesktopFolder            [SHELL32.@]
 */
HRESULT WINAPI SHGetDesktopFolder(IShellFolder **psf)
{
    HRESULT    hres = S_OK;
    TRACE("\n");

    if(!psf) return E_INVALIDARG;
    *psf = NULL;
    hres = CDesktopFolder::_CreatorClass::CreateInstance(NULL, IID_PPV_ARG(IShellFolder, psf));

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

class IDefClFImpl :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IClassFactory
{
private:
    CLSID                    *rclsid;
    LPFNCREATEINSTANCE        lpfnCI;
    const IID                *riidInst;
    LONG                    *pcRefDll;        /* pointer to refcounter in external dll (ugrrr...) */
public:
    IDefClFImpl();
    HRESULT Initialize(LPFNCREATEINSTANCE lpfnCI, PLONG pcRefDll, const IID *riidInstx);

    // IClassFactory
    virtual HRESULT WINAPI CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObject);
    virtual HRESULT WINAPI LockServer(BOOL fLock);

BEGIN_COM_MAP(IDefClFImpl)
    COM_INTERFACE_ENTRY_IID(IID_IClassFactory, IClassFactory)
END_COM_MAP()
};

IDefClFImpl::IDefClFImpl()
{
    lpfnCI = NULL;
    riidInst = NULL;
    pcRefDll = NULL;
    rclsid = NULL;
}

HRESULT IDefClFImpl::Initialize(LPFNCREATEINSTANCE lpfnCIx, PLONG pcRefDllx, const IID *riidInstx)
{
    lpfnCI = lpfnCIx;
    riidInst = riidInstx;
    pcRefDll = pcRefDllx;

    if (pcRefDll)
        InterlockedIncrement(pcRefDll);

    TRACE("(%p)%s\n", this, shdebugstr_guid(riidInst));
    return S_OK;
}

/******************************************************************************
 * IDefClF_fnCreateInstance
 */
HRESULT WINAPI IDefClFImpl::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObject)
{
    TRACE("%p->(%p,%s,%p)\n", this, pUnkOuter, shdebugstr_guid(&riid), ppvObject);

    *ppvObject = NULL;

    if (riidInst == NULL || IsEqualCLSID(riid, *riidInst) || IsEqualCLSID(riid, IID_IUnknown))
    {
        return lpfnCI(pUnkOuter, riid, ppvObject);
    }

    ERR("unknown IID requested %s\n", shdebugstr_guid(&riid));
    return E_NOINTERFACE;
}

/******************************************************************************
 * IDefClF_fnLockServer
 */
HRESULT WINAPI IDefClFImpl::LockServer(BOOL fLock)
{
    TRACE("%p->(0x%x), not implemented\n", this, fLock);
    return E_NOTIMPL;
}

/**************************************************************************
 *  IDefClF_fnConstructor
 */

HRESULT IDefClF_fnConstructor(LPFNCREATEINSTANCE lpfnCI, PLONG pcRefDll, const IID *riidInst, IClassFactory **theFactory)
{
    CComObject<IDefClFImpl>                    *theClassObject;
    CComPtr<IClassFactory>                    result;
    HRESULT                                    hResult;

    if (theFactory == NULL)
        return E_POINTER;
    *theFactory = NULL;
    ATLTRY (theClassObject = new CComObject<IDefClFImpl>);
    if (theClassObject == NULL)
        return E_OUTOFMEMORY;
    hResult = theClassObject->QueryInterface (IID_PPV_ARG(IClassFactory, &result));
    if (FAILED (hResult))
    {
        delete theClassObject;
        return hResult;
    }
    hResult = theClassObject->Initialize (lpfnCI, pcRefDll, riidInst);
    if (FAILED (hResult))
        return hResult;
    *theFactory = result.Detach ();
    return S_OK;
}

/******************************************************************************
 * SHCreateDefClassObject            [SHELL32.70]
 */
HRESULT WINAPI SHCreateDefClassObject(
    REFIID    riid,
    LPVOID*    ppv,
    LPFNCREATEINSTANCE lpfnCI,    /* [in] create instance callback entry */
    LPDWORD    pcRefDll,        /* [in/out] ref count of the dll */
    REFIID    riidInst)        /* [in] optional interface to the instance */
{
    IClassFactory                *pcf;
    HRESULT                        hResult;

    TRACE("%s %p %p %p %s\n", shdebugstr_guid(&riid), ppv, lpfnCI, pcRefDll, shdebugstr_guid(&riidInst));

    if (!IsEqualCLSID(riid, IID_IClassFactory))
        return E_NOINTERFACE;
    hResult = IDefClF_fnConstructor(lpfnCI, (PLONG)pcRefDll, &riidInst, &pcf);
    if (FAILED(hResult))
        return hResult;
    *ppv = pcf;
    return S_OK;
}

/*************************************************************************
 *  DragAcceptFiles        [SHELL32.@]
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
 * DragFinish        [SHELL32.@]
 */
void WINAPI DragFinish(HDROP h)
{
    TRACE("\n");
    GlobalFree((HGLOBAL)h);
}

/*************************************************************************
 * DragQueryPoint        [SHELL32.@]
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
 *  DragQueryFileA        [SHELL32.@]
 *  DragQueryFile         [SHELL32.@]
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

    TRACE("(%p, %x, %p, %u)\n",    hDrop,lFile,lpszFile,lLength);

    if(!lpDropFileStruct) goto end;

    lpDrop = (LPSTR) lpDropFileStruct + lpDropFileStruct->pFiles;

        if(lpDropFileStruct->fWide) {
            LPWSTR lpszFileW = NULL;

            if(lpszFile) {
                lpszFileW = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, lLength*sizeof(WCHAR));
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
 *  DragQueryFileW        [SHELL32.@]
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
                lpszFileA = (LPSTR)HeapAlloc(GetProcessHeap(), 0, lLength);
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

    i = wcslen(lpwDrop);
    if ( !lpszwFile) goto end;   /* needed buffer size */
    lstrcpynW (lpszwFile, lpwDrop, lLength);
end:
    GlobalUnlock(hDrop);
    return i;
}

/*************************************************************************
 *  SHPropStgCreate             [SHELL32.685]
 */
EXTERN_C HRESULT WINAPI SHPropStgCreate(IPropertySetStorage *psstg, REFFMTID fmtid,
         const CLSID *pclsid, DWORD grfFlags, DWORD grfMode,
         DWORD dwDisposition, IPropertyStorage **ppstg, UINT *puCodePage)
{
    PROPSPEC prop;
    PROPVARIANT ret;
    HRESULT hres;

    TRACE("%p %s %s %x %x %x %p %p\n", psstg, debugstr_guid(&fmtid), debugstr_guid(pclsid),
        grfFlags, grfMode, dwDisposition, ppstg, puCodePage);

    hres = psstg->Open(fmtid, grfMode, ppstg);

     switch (dwDisposition)
     {
         case CREATE_ALWAYS:
             if (SUCCEEDED(hres))
             {
                 (*ppstg)->Release();
                 hres = psstg->Delete(fmtid);
                 if(FAILED(hres))
                     return hres;
                 hres = E_FAIL;
             }

         case OPEN_ALWAYS:
         case CREATE_NEW:
             if (FAILED(hres))
                 hres = psstg->Create(fmtid, pclsid, grfFlags, grfMode, ppstg);

         case OPEN_EXISTING:
             if (FAILED(hres))
                 return hres;

             if (puCodePage)
             {
                 prop.ulKind = PRSPEC_PROPID;
                 prop.propid = PID_CODEPAGE;
                 hres = (*ppstg)->ReadMultiple(1, &prop, &ret);
                 if (FAILED(hres) || ret.vt!=VT_I2)
                     *puCodePage = 0;
                 else
                     *puCodePage = ret.iVal;
             }
     }

     return S_OK;
}

/*************************************************************************
 *  SHPropStgReadMultiple       [SHELL32.688]
 */
EXTERN_C HRESULT WINAPI SHPropStgReadMultiple(IPropertyStorage *pps, UINT uCodePage,
         ULONG cpspec, const PROPSPEC *rgpspec, PROPVARIANT *rgvar)
{
    STATPROPSETSTG stat;
    HRESULT hres;

    FIXME("%p %u %u %p %p\n", pps, uCodePage, cpspec, rgpspec, rgvar);

    memset(rgvar, 0, cpspec*sizeof(PROPVARIANT));
    hres = pps->ReadMultiple(cpspec, rgpspec, rgvar);
    if (FAILED(hres))
        return hres;

    if (!uCodePage)
    {
        PROPSPEC prop;
        PROPVARIANT ret;

        prop.ulKind = PRSPEC_PROPID;
        prop.propid = PID_CODEPAGE;
        hres = pps->ReadMultiple(1, &prop, &ret);
        if(FAILED(hres) || ret.vt!=VT_I2)
            return S_OK;

        uCodePage = ret.iVal;
    }

    hres = pps->Stat(&stat);
    if (FAILED(hres))
        return S_OK;

    /* TODO: do something with codepage and stat */
    return S_OK;
}

/*************************************************************************
 *  SHPropStgWriteMultiple      [SHELL32.689]
 */
EXTERN_C HRESULT WINAPI SHPropStgWriteMultiple(IPropertyStorage *pps, UINT *uCodePage,
         ULONG cpspec, const PROPSPEC *rgpspec, PROPVARIANT *rgvar, PROPID propidNameFirst)
{
    STATPROPSETSTG stat;
    UINT codepage;
    HRESULT hres;

    FIXME("%p %p %u %p %p %d\n", pps, uCodePage, cpspec, rgpspec, rgvar, propidNameFirst);

    hres = pps->Stat(&stat);
    if (FAILED(hres))
        return hres;

    if (uCodePage && *uCodePage)
        codepage = *uCodePage;
    else
    {
        PROPSPEC prop;
        PROPVARIANT ret;

        prop.ulKind = PRSPEC_PROPID;
        prop.propid = PID_CODEPAGE;
        hres = pps->ReadMultiple(1, &prop, &ret);
        if (FAILED(hres))
            return hres;
        if (ret.vt!=VT_I2 || !ret.iVal)
            return E_FAIL;

        codepage = ret.iVal;
        if (uCodePage)
            *uCodePage = codepage;
    }

    /* TODO: do something with codepage and stat */

    hres = pps->WriteMultiple(cpspec, rgpspec, rgvar, propidNameFirst);
    return hres;
}

/*************************************************************************
 *  SHCreateQueryCancelAutoPlayMoniker [SHELL32.@]
 */
HRESULT WINAPI SHCreateQueryCancelAutoPlayMoniker(IMoniker **moniker)
{
    TRACE("%p\n", moniker);

    if (!moniker) return E_INVALIDARG;
    return CreateClassMoniker(CLSID_QueryCancelAutoPlay, moniker);
}

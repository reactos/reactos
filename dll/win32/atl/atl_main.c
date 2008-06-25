/*
 * Implementation of Active Template Library (atl.dll)
 *
 * Copyright 2004 Aric Stewart for CodeWeavers
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

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"
#include "objbase.h"
#include "objidl.h"
#include "ole2.h"
#include "atlbase.h"
#include "atliface.h"
#include "atlwin.h"

#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(atl);

HINSTANCE hInst;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n",hinstDLL,fdwReason,lpvReserved);

    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
        hInst = hinstDLL;
    }
    return TRUE;
}

#define ATLVer1Size 100

HRESULT WINAPI AtlModuleInit(_ATL_MODULEW* pM, _ATL_OBJMAP_ENTRYW* p, HINSTANCE h)
{
    INT i;
    UINT size;

    FIXME("SEMI-STUB (%p %p %p)\n",pM,p,h);

    size = pM->cbSize;
    if  (size != sizeof(_ATL_MODULEW) && size != ATLVer1Size)
    {
        FIXME("Unknown structure version (size %i)\n",size);
        return E_INVALIDARG;
    }

    memset(pM,0,pM->cbSize);
    pM->cbSize = size;
    pM->m_hInst = h;
    pM->m_hInstResource = h;
    pM->m_hInstTypeLib = h;
    pM->m_pObjMap = p;
    pM->m_hHeap = GetProcessHeap();

    InitializeCriticalSection(&pM->u.m_csTypeInfoHolder);
    InitializeCriticalSection(&pM->m_csWindowCreate);
    InitializeCriticalSection(&pM->m_csObjMap);

    /* call mains */
    i = 0;
    if (pM->m_pObjMap != NULL  && size > ATLVer1Size)
    {
        while (pM->m_pObjMap[i].pclsid != NULL)
        {
            TRACE("Initializing object %i %p\n",i,p[i].pfnObjectMain);
            if (p[i].pfnObjectMain)
                p[i].pfnObjectMain(TRUE);
            i++;
        }
    }

    return S_OK;
}

HRESULT WINAPI AtlModuleLoadTypeLib(_ATL_MODULEW *pM, LPCOLESTR lpszIndex,
                                    BSTR *pbstrPath, ITypeLib **ppTypeLib)
{
    HRESULT hRes;
    OLECHAR path[MAX_PATH+8]; /* leave some space for index */

    TRACE("(%p, %s, %p, %p)\n", pM, debugstr_w(lpszIndex), pbstrPath, ppTypeLib);

    if (!pM)
        return E_INVALIDARG;

    GetModuleFileNameW(pM->m_hInstTypeLib, path, MAX_PATH);
    if (lpszIndex)
        lstrcatW(path, lpszIndex);

    hRes = LoadTypeLib(path, ppTypeLib);
    if (FAILED(hRes))
        return hRes;

    *pbstrPath = SysAllocString(path);

    return S_OK;
}

HRESULT WINAPI AtlModuleTerm(_ATL_MODULEW* pM)
{
    _ATL_TERMFUNC_ELEM *iter = pM->m_pTermFuncs, *tmp;

    TRACE("(%p)\n", pM);

    while(iter) {
        iter->pFunc(iter->dw);
        tmp = iter;
        iter = iter->pNext;
        HeapFree(GetProcessHeap(), 0, tmp);
    }

    HeapFree(GetProcessHeap(), 0, pM);

    return S_OK;
}

HRESULT WINAPI AtlModuleAddTermFunc(_ATL_MODULEW *pM, _ATL_TERMFUNC *pFunc, DWORD_PTR dw)
{
    _ATL_TERMFUNC_ELEM *termfunc_elem;

    TRACE("(%p %p %ld)\n", pM, pFunc, dw);

    termfunc_elem = HeapAlloc(GetProcessHeap(), 0, sizeof(_ATL_TERMFUNC_ELEM));
    termfunc_elem->pFunc = pFunc;
    termfunc_elem->dw = dw;
    termfunc_elem->pNext = pM->m_pTermFuncs;

    pM->m_pTermFuncs = termfunc_elem;

    return S_OK;
}

HRESULT WINAPI AtlModuleRegisterClassObjects(_ATL_MODULEW *pM, DWORD dwClsContext,
                                             DWORD dwFlags)
{
    HRESULT hRes = S_OK;
    int i=0;

    TRACE("(%p %i %i)\n",pM, dwClsContext, dwFlags);

    if (pM == NULL)
        return E_INVALIDARG;

    while(pM->m_pObjMap[i].pclsid != NULL)
    {
        IUnknown* pUnknown;
        _ATL_OBJMAP_ENTRYW *obj = &(pM->m_pObjMap[i]);
        HRESULT rc;

        TRACE("Registering object %i\n",i);
        if (obj->pfnGetClassObject)
        {
            rc = obj->pfnGetClassObject(obj->pfnCreateInstance, &IID_IUnknown,
                                   (LPVOID*)&pUnknown);
            if (SUCCEEDED (rc) )
            {
                CoRegisterClassObject(obj->pclsid, pUnknown, dwClsContext,
                                      dwFlags, &obj->dwRegister);
                if (pUnknown)
                    IUnknown_Release(pUnknown);
            }
        }
        i++;
    }

   return hRes;
}

HRESULT WINAPI AtlModuleUnregisterServerEx(_ATL_MODULEW* pM, BOOL bUnRegTypeLib, const CLSID* pCLSID)
{
    FIXME("(%p, %i, %p) stub\n", pM, bUnRegTypeLib, pCLSID);
    return S_OK;
}


IUnknown* WINAPI AtlComPtrAssign(IUnknown** pp, IUnknown *p)
{
    TRACE("(%p %p)\n", pp, p);

    if (p) IUnknown_AddRef(p);
    if (*pp) IUnknown_Release(*pp);
    *pp = p;
    return p;
}

IUnknown* WINAPI AtlComQIPtrAssign(IUnknown** pp, IUnknown *p, REFIID riid)
{
    IUnknown *new_p = NULL;

    TRACE("(%p %p %s)\n", pp, p, debugstr_guid(riid));

    if (p) IUnknown_QueryInterface(p, riid, (void **)&new_p);
    if (*pp) IUnknown_Release(*pp);
    *pp = new_p;
    return new_p;
}


HRESULT WINAPI AtlInternalQueryInterface(void* this, const _ATL_INTMAP_ENTRY* pEntries,  REFIID iid, void** ppvObject)
{
    int i = 0;
    HRESULT rc = E_NOINTERFACE;
    TRACE("(%p, %p, %s, %p)\n",this, pEntries, debugstr_guid(iid), ppvObject);

    if (IsEqualGUID(iid,&IID_IUnknown))
    {
        TRACE("Returning IUnknown\n");
        *ppvObject = ((LPSTR)this+pEntries[0].dw);
        IUnknown_AddRef((IUnknown*)*ppvObject);
        return S_OK;
    }

    while (pEntries[i].pFunc != 0)
    {
        TRACE("Trying entry %i (%s %i %p)\n",i,debugstr_guid(pEntries[i].piid),
              pEntries[i].dw, pEntries[i].pFunc);

        if (pEntries[i].piid && IsEqualGUID(iid,pEntries[i].piid))
        {
            TRACE("MATCH\n");
            if (pEntries[i].pFunc == (_ATL_CREATORARGFUNC*)1)
            {
                TRACE("Offset\n");
                *ppvObject = ((LPSTR)this+pEntries[i].dw);
                IUnknown_AddRef((IUnknown*)*ppvObject);
                rc = S_OK;
            }
            else
            {
                TRACE("Function\n");
                rc = pEntries[i].pFunc(this, iid, ppvObject,0);
            }
            break;
        }
        i++;
    }
    TRACE("Done returning (0x%x)\n",rc);
    return rc;
}

/***********************************************************************
 *           AtlModuleRegisterServer         [ATL.@]
 *
 */
HRESULT WINAPI AtlModuleRegisterServer(_ATL_MODULEW* pM, BOOL bRegTypeLib, const CLSID* clsid)
{
    int i;
    HRESULT hRes;

    TRACE("%p %d %s\n", pM, bRegTypeLib, debugstr_guid(clsid));

    if (pM == NULL)
        return E_INVALIDARG;

    for (i = 0; pM->m_pObjMap[i].pclsid != NULL; i++) /* register CLSIDs */
    {
        if (!clsid || IsEqualCLSID(pM->m_pObjMap[i].pclsid, clsid))
        {
            const _ATL_OBJMAP_ENTRYW *obj = &pM->m_pObjMap[i];

            TRACE("Registering clsid %s\n", debugstr_guid(obj->pclsid));
            hRes = obj->pfnUpdateRegistry(TRUE); /* register */
            if (FAILED(hRes))
                return hRes;
        }
    }

    if (bRegTypeLib)
    {
        hRes = AtlModuleRegisterTypeLib(pM, NULL);
        if (FAILED(hRes))
            return hRes;
    }

    return S_OK;
}

/***********************************************************************
 *           AtlAdvise         [ATL.@]
 */
HRESULT WINAPI AtlAdvise(IUnknown *pUnkCP, IUnknown *pUnk, const IID *iid, LPDWORD pdw)
{
    FIXME("%p %p %p %p\n", pUnkCP, pUnk, iid, pdw);
    return E_FAIL;
}

/***********************************************************************
 *           AtlUnadvise         [ATL.@]
 */
HRESULT WINAPI AtlUnadvise(IUnknown *pUnkCP, const IID *iid, DWORD dw)
{
    FIXME("%p %p %d\n", pUnkCP, iid, dw);
    return S_OK;
}

/***********************************************************************
 *           AtlFreeMarshalStream         [ATL.@]
 */
HRESULT WINAPI AtlFreeMarshalStream(IStream *stm)
{
    FIXME("%p\n", stm);
    return S_OK;
}

/***********************************************************************
 *           AtlMarshalPtrInProc         [ATL.@]
 */
HRESULT WINAPI AtlMarshalPtrInProc(IUnknown *pUnk, const IID *iid, IStream **pstm)
{
    FIXME("%p %p %p\n", pUnk, iid, pstm);
    return E_FAIL;
}

/***********************************************************************
 *           AtlUnmarshalPtr              [ATL.@]
 */
HRESULT WINAPI AtlUnmarshalPtr(IStream *stm, const IID *iid, IUnknown **ppUnk)
{
    FIXME("%p %p %p\n", stm, iid, ppUnk);
    return E_FAIL;
}

/***********************************************************************
 *           AtlModuleGetClassObject              [ATL.@]
 */
HRESULT WINAPI AtlModuleGetClassObject(_ATL_MODULEW *pm, REFCLSID rclsid,
                                       REFIID riid, LPVOID *ppv)
{
    int i;
    HRESULT hres = CLASS_E_CLASSNOTAVAILABLE;

    TRACE("%p %s %s %p\n", pm, debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if (pm == NULL)
        return E_INVALIDARG;

    for (i = 0; pm->m_pObjMap[i].pclsid != NULL; i++)
    {
        if (IsEqualCLSID(pm->m_pObjMap[i].pclsid, rclsid))
        {
            _ATL_OBJMAP_ENTRYW *obj = &pm->m_pObjMap[i];

            TRACE("found object %i\n", i);
            if (obj->pfnGetClassObject)
            {
                if (!obj->pCF)
                    hres = obj->pfnGetClassObject(obj->pfnCreateInstance,
                                                  &IID_IUnknown,
                                                  (void **)&obj->pCF);
                if (obj->pCF)
                    hres = IUnknown_QueryInterface(obj->pCF, riid, ppv);
                break;
            }
        }
    }

    WARN("no class object found for %s\n", debugstr_guid(rclsid));

    return hres;
}

/***********************************************************************
 *           AtlModuleGetClassObject              [ATL.@]
 */
HRESULT WINAPI AtlModuleRegisterTypeLib(_ATL_MODULEW *pm, LPCOLESTR lpszIndex)
{
    HRESULT hRes;
    BSTR path;
    ITypeLib *typelib;

    TRACE("%p %s\n", pm, debugstr_w(lpszIndex));

    if (!pm)
        return E_INVALIDARG;

    hRes = AtlModuleLoadTypeLib(pm, lpszIndex, &path, &typelib);

    if (SUCCEEDED(hRes))
    {
        hRes = RegisterTypeLib(typelib, path, NULL); /* FIXME: pass help directory */
        ITypeLib_Release(typelib);
        SysFreeString(path);
    }

    return hRes;
}

/***********************************************************************
 *           AtlModuleRevokeClassObjects          [ATL.@]
 */
HRESULT WINAPI AtlModuleRevokeClassObjects(_ATL_MODULEW *pm)
{
    FIXME("%p\n", pm);
    return E_FAIL;
}

/***********************************************************************
 *           AtlModuleUnregisterServer           [ATL.@]
 */
HRESULT WINAPI AtlModuleUnregisterServer(_ATL_MODULEW *pm, const CLSID *clsid)
{
    FIXME("%p %s\n", pm, debugstr_guid(clsid));
    return E_FAIL;
}

/***********************************************************************
 *           AtlModuleRegisterWndClassInfoW           [ATL.@]
 *
 * PARAMS
 *  pm   [IO] Information about the module registering the window.
 *  wci  [IO] Information about the window being registered.
 *  pProc [O] Window procedure of the registered class.
 *
 * RETURNS
 *  Atom representing the registered class.
 *
 * NOTES
 *  Can be called multiple times without error, unlike RegisterClassEx().
 *
 *  If the class name is NULL, then a class with a name of "ATLxxxxxxxx" is
 *  registered, where the 'x's represent a unique value.
 *
 */
ATOM WINAPI AtlModuleRegisterWndClassInfoW(_ATL_MODULEW *pm, _ATL_WNDCLASSINFOW *wci, WNDPROC *pProc)
{
    ATOM atom;

    FIXME("%p %p %p semi-stub\n", pm, wci, pProc);

    atom = wci->m_atom;
    if (!atom)
    {
        WNDCLASSEXW wc;

        TRACE("wci->m_wc.lpszClassName = %s\n", debugstr_w(wci->m_wc.lpszClassName));

        if (!wci->m_wc.lpszClassName)
        {
            static const WCHAR szFormat[] = {'A','T','L','%','0','8','x',0};
            sprintfW(wci->m_szAutoName, szFormat, (UINT)(UINT_PTR)wci);
            TRACE("auto-generated class name %s\n", debugstr_w(wci->m_szAutoName));
            wci->m_wc.lpszClassName = wci->m_szAutoName;
        }

        atom = GetClassInfoExW(pm->m_hInst, wci->m_wc.lpszClassName, &wc);
        if (!atom)
            atom = RegisterClassExW(&wci->m_wc);

        wci->pWndProc = wci->m_wc.lpfnWndProc;
        wci->m_atom = atom;
    }
    *pProc = wci->pWndProc;

    TRACE("returning 0x%04x\n", atom);
    return atom;
}

void WINAPI AtlHiMetricToPixel(const SIZEL* lpHiMetric, SIZEL* lpPix)
{
    HDC dc = GetDC(NULL);
    lpPix->cx = lpHiMetric->cx * GetDeviceCaps( dc, LOGPIXELSX ) / 100;
    lpPix->cy = lpHiMetric->cy * GetDeviceCaps( dc, LOGPIXELSY ) / 100;
    ReleaseDC( NULL, dc );
}

void WINAPI AtlPixelToHiMetric(const SIZEL* lpPix, SIZEL* lpHiMetric)
{
    HDC dc = GetDC(NULL);
    lpHiMetric->cx = 100 * lpPix->cx / GetDeviceCaps( dc, LOGPIXELSX );
    lpHiMetric->cy = 100 * lpPix->cy / GetDeviceCaps( dc, LOGPIXELSY );
    ReleaseDC( NULL, dc );
}

/***********************************************************************
 *           AtlModuleAddCreateWndData          [ATL.@]
 */
void WINAPI AtlModuleAddCreateWndData(_ATL_MODULEW *pM, _AtlCreateWndData *pData, void* pvObject)
{
    TRACE("(%p, %p, %p)\n", pM, pData, pvObject);

    pData->m_pThis = pvObject;
    pData->m_dwThreadID = GetCurrentThreadId();
    pData->m_pNext = pM->m_pCreateWndList;
    pM->m_pCreateWndList = pData;
}

/***********************************************************************
 *           AtlModuleExtractCreateWndData      [ATL.@]
 *
 *  NOTE: I failed to find any good description of this function.
 *        Tests show that this function extracts one of _AtlCreateWndData
 *        records from the current thread from a list
 *
 */
void* WINAPI AtlModuleExtractCreateWndData(_ATL_MODULEW *pM)
{
    _AtlCreateWndData **ppData;

    TRACE("(%p)\n", pM);

    for(ppData = &pM->m_pCreateWndList; *ppData!=NULL; ppData = &(*ppData)->m_pNext)
    {
        if ((*ppData)->m_dwThreadID == GetCurrentThreadId())
        {
            _AtlCreateWndData *pData = *ppData;
            *ppData = pData->m_pNext;
            return pData->m_pThis;
        }
    }
    return NULL;
}

/* FIXME: should be in a header file */
typedef struct ATL_PROPMAP_ENTRY
{
    LPCOLESTR szDesc;
    DISPID dispid;
    const CLSID* pclsidPropPage;
    const IID* piidDispatch;
    DWORD dwOffsetData;
    DWORD dwSizeData;
    VARTYPE vt;
} ATL_PROPMAP_ENTRY;

/***********************************************************************
 *           AtlIPersistStreamInit_Load      [ATL.@]
 */
HRESULT WINAPI AtlIPersistStreamInit_Load( LPSTREAM pStm, ATL_PROPMAP_ENTRY *pMap,
                                           void *pThis, IUnknown *pUnk)
{
    FIXME("(%p, %p, %p, %p)\n", pStm, pMap, pThis, pUnk);

    return S_OK;
}

HRESULT WINAPI AtlIPersistStreamInit_Save(LPSTREAM pStm, BOOL fClearDirty,
                                          ATL_PROPMAP_ENTRY *pMap, void *pThis,
                                          IUnknown *pUnk)
{
    FIXME("(%p, %d, %p, %p, %p)\n", pStm, fClearDirty, pMap, pThis, pUnk);

    return S_OK;
}

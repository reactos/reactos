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

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <stdio.h>

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <objbase.h>
#include <oleauto.h>

//#include "objidl.h"
#include <atlbase.h>
#include <atlwin.h>

#include <wine/debug.h>
#include <wine/unicode.h>

WINE_DEFAULT_DEBUG_CHANNEL(atl);

DECLSPEC_HIDDEN HINSTANCE hInst;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n",hinstDLL,fdwReason,lpvReserved);

    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
        hInst = hinstDLL;
    }
    return TRUE;
}

#define ATLVer1Size FIELD_OFFSET(_ATL_MODULEW, dwAtlBuildVer)

HRESULT WINAPI AtlModuleInit(_ATL_MODULEW* pM, _ATL_OBJMAP_ENTRYW* p, HINSTANCE h)
{
    INT i;
    UINT size;

    //FIXME("SEMI-STUB (%p %p %p)\n",pM,p,h);

    size = pM->cbSize;
    switch (size)
    {
    case ATLVer1Size:
    case sizeof(_ATL_MODULEW):
#ifdef _WIN64
    case sizeof(_ATL_MODULEW) + sizeof(void *):
#endif
        break;
    default:
        WARN("Unknown structure version (size %i)\n",size);
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

static _ATL_OBJMAP_ENTRYW_V1 *get_objmap_entry( _ATL_MODULEW *mod, unsigned int index )
{
    _ATL_OBJMAP_ENTRYW_V1 *ret;

    if (mod->cbSize == ATLVer1Size)
        ret = (_ATL_OBJMAP_ENTRYW_V1 *)mod->m_pObjMap + index;
    else
        ret = (_ATL_OBJMAP_ENTRYW_V1 *)(mod->m_pObjMap + index);

    if (!ret->pclsid) ret = NULL;
    return ret;
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
    _ATL_OBJMAP_ENTRYW_V1 *obj;
    int i=0;

    TRACE("(%p %i %i)\n",pM, dwClsContext, dwFlags);

    if (pM == NULL)
        return E_INVALIDARG;

    while ((obj = get_objmap_entry( pM, i++ )))
    {
        IUnknown* pUnknown;
        HRESULT rc;

        TRACE("Registering object %i\n",i);
        if (obj->pfnGetClassObject)
        {
            rc = obj->pfnGetClassObject(obj->pfnCreateInstance, &IID_IUnknown,
                                   (LPVOID*)&pUnknown);
            if (SUCCEEDED (rc) )
            {
                rc = CoRegisterClassObject(obj->pclsid, pUnknown, dwClsContext,
                                           dwFlags, &obj->dwRegister);

                if (FAILED (rc) )
                    WARN("Failed to register object %i: 0x%08x\n", i, rc);

                if (pUnknown)
                    IUnknown_Release(pUnknown);
            }
        }
    }

   return S_OK;
}

HRESULT WINAPI AtlModuleUnregisterServerEx(_ATL_MODULEW* pM, BOOL bUnRegTypeLib, const CLSID* pCLSID)
{
    FIXME("(%p, %i, %p) stub\n", pM, bUnRegTypeLib, pCLSID);
    return S_OK;
}

/***********************************************************************
 *           AtlModuleRegisterServer         [ATL.@]
 *
 */
HRESULT WINAPI AtlModuleRegisterServer(_ATL_MODULEW* pM, BOOL bRegTypeLib, const CLSID* clsid)
{
    const _ATL_OBJMAP_ENTRYW_V1 *obj;
    int i;
    HRESULT hRes;

    TRACE("%p %d %s\n", pM, bRegTypeLib, debugstr_guid(clsid));

    if (pM == NULL)
        return E_INVALIDARG;

    for (i = 0; (obj = get_objmap_entry( pM, i )) != NULL; i++) /* register CLSIDs */
    {
        if (!clsid || IsEqualCLSID(obj->pclsid, clsid))
        {
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
 *           AtlModuleGetClassObject              [ATL.@]
 */
HRESULT WINAPI AtlModuleGetClassObject(_ATL_MODULEW *pm, REFCLSID rclsid,
                                       REFIID riid, LPVOID *ppv)
{
    _ATL_OBJMAP_ENTRYW_V1 *obj;
    int i;
    HRESULT hres = CLASS_E_CLASSNOTAVAILABLE;

    TRACE("%p %s %s %p\n", pm, debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if (pm == NULL)
        return E_INVALIDARG;

    for (i = 0; (obj = get_objmap_entry( pm, i )) != NULL; i++)
    {
        if (IsEqualCLSID(obj->pclsid, rclsid))
        {
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
 *           AtlModuleRegisterWndClassInfoA           [ATL.@]
 *
 * See AtlModuleRegisterWndClassInfoW.
 */
ATOM WINAPI AtlModuleRegisterWndClassInfoA(_ATL_MODULEA *pm, _ATL_WNDCLASSINFOA *wci, WNDPROC *pProc)
{
    ATOM atom;

    FIXME("%p %p %p semi-stub\n", pm, wci, pProc);

    atom = wci->m_atom;
    if (!atom)
    {
        WNDCLASSEXA wc;

        TRACE("wci->m_wc.lpszClassName = %s\n", wci->m_wc.lpszClassName);

        if (wci->m_lpszOrigName)
            FIXME( "subclassing %s not implemented\n", debugstr_a(wci->m_lpszOrigName));

        if (!wci->m_wc.lpszClassName)
        {
            snprintf(wci->m_szAutoName, sizeof(wci->m_szAutoName), "ATL%08lx", (UINT_PTR)wci);
            TRACE("auto-generated class name %s\n", wci->m_szAutoName);
            wci->m_wc.lpszClassName = wci->m_szAutoName;
        }

        atom = GetClassInfoExA(pm->m_hInst, wci->m_wc.lpszClassName, &wc);
        if (!atom)
        {
            wci->m_wc.hInstance = pm->m_hInst;
            wci->m_wc.hCursor   = LoadCursorA( wci->m_bSystemCursor ? NULL : pm->m_hInst,
                                               wci->m_lpszCursorID );
            atom = RegisterClassExA(&wci->m_wc);
        }
        wci->pWndProc = wci->m_wc.lpfnWndProc;
        wci->m_atom = atom;
    }

    if (wci->m_lpszOrigName) *pProc = wci->pWndProc;

    TRACE("returning 0x%04x\n", atom);
    return atom;
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

        if (wci->m_lpszOrigName)
            FIXME( "subclassing %s not implemented\n", debugstr_w(wci->m_lpszOrigName));

        if (!wci->m_wc.lpszClassName)
        {
            static const WCHAR szFormat[] = {'A','T','L','%','0','8','l','x',0};
            snprintfW(wci->m_szAutoName, sizeof(wci->m_szAutoName)/sizeof(WCHAR), szFormat, (UINT_PTR)wci);
            TRACE("auto-generated class name %s\n", debugstr_w(wci->m_szAutoName));
            wci->m_wc.lpszClassName = wci->m_szAutoName;
        }

        atom = GetClassInfoExW(pm->m_hInst, wci->m_wc.lpszClassName, &wc);
        if (!atom)
        {
            wci->m_wc.hInstance = pm->m_hInst;
            wci->m_wc.hCursor   = LoadCursorW( wci->m_bSystemCursor ? NULL : pm->m_hInst,
                                               wci->m_lpszCursorID );
            atom = RegisterClassExW(&wci->m_wc);
        }
        wci->pWndProc = wci->m_wc.lpfnWndProc;
        wci->m_atom = atom;
    }

    if (wci->m_lpszOrigName) *pProc = wci->pWndProc;

    TRACE("returning 0x%04x\n", atom);
    return atom;
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

/***********************************************************************
 *           AtlGetVersion              [ATL.@]
 */
DWORD WINAPI AtlGetVersion(void *pReserved)
{
   return 0x0300;
}

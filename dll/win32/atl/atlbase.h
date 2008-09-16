/*
 * Implementation of the Active Template Library (atl.dll)
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

#ifndef __WINE_ATLBASE_H__
#define __WINE_ATLBASE_H__

#define COBJMACROS

#include "atliface.h"

typedef HRESULT (WINAPI _ATL_CREATORFUNC)(void* pv, REFIID riid, LPVOID* ppv);
typedef HRESULT (WINAPI _ATL_CREATORARGFUNC)(void* pv, REFIID riid, LPVOID* ppv, DWORD dw);
typedef HRESULT (WINAPI _ATL_MODULEFUNC)(DWORD dw);
typedef LPCSTR (WINAPI _ATL_DESCRIPTIONFUNCA)();
typedef LPCWSTR (WINAPI _ATL_DESCRIPTIONFUNCW)();
typedef const struct _ATL_CATMAP_ENTRY* (_ATL_CATMAPFUNC)();
typedef void (WINAPI _ATL_TERMFUNC)(DWORD dw);

typedef struct _ATL_OBJMAP_ENTRYA_TAG
{
    const CLSID* pclsid;
    HRESULT (WINAPI *pfnUpdateRegistry)(BOOL bRegister);
    _ATL_CREATORFUNC* pfnGetClassObject;
    _ATL_CREATORFUNC* pfnCreateInstance;
    IUnknown* pCF;
    DWORD dwRegister;
    _ATL_DESCRIPTIONFUNCA* pfnGetObjectDescription;
    _ATL_CATMAPFUNC* pfnGetCategoryMap;
    void (WINAPI *pfnObjectMain)(BOOL bStarting);
}_ATL_OBJMAP_ENTRYA;

typedef struct _ATL_OBJMAP_ENTRYW_TAG
{
    const CLSID* pclsid;
    HRESULT (WINAPI *pfnUpdateRegistry)(BOOL bRegister);
    _ATL_CREATORFUNC* pfnGetClassObject;
    _ATL_CREATORFUNC* pfnCreateInstance;
    IUnknown* pCF;
    DWORD dwRegister;
    _ATL_DESCRIPTIONFUNCW* pfnGetObjectDescription;
    _ATL_CATMAPFUNC* pfnGetCategoryMap;
    void (WINAPI *pfnObjectMain)(BOOL bStarting);
} _ATL_OBJMAP_ENTRYW;


typedef struct _ATL_TERMFUNC_ELEM_TAG
{
    _ATL_TERMFUNC* pFunc;
    DWORD_PTR dw;
    struct _ATL_TERMFUNC_ELEM_TAG* pNext;
} _ATL_TERMFUNC_ELEM;

typedef struct _AtlCreateWndData_TAG
{
    void* m_pThis;
    DWORD m_dwThreadID;
    struct _AtlCreateWndData_TAG* m_pNext;
} _AtlCreateWndData;

typedef struct _ATL_MODULEA_TAG
{
    UINT cbSize;
    HINSTANCE m_hInst;
    HINSTANCE m_hInstResource;
    HINSTANCE m_hInstTypeLib;
    _ATL_OBJMAP_ENTRYA* m_pObjMap;
    LONG m_nLockCnt;
    HANDLE m_hHeap;
    union
    {
        CRITICAL_SECTION m_csTypeInfoHolder;
        CRITICAL_SECTION m_csStaticDataInit;
    } u;
    CRITICAL_SECTION m_csWindowCreate;
    CRITICAL_SECTION m_csObjMap;

    DWORD dwAtlBuildVer;
    _AtlCreateWndData* m_pCreateWndList;
    BOOL m_bDestroyHeap;
    GUID* pguidVer;
    DWORD m_dwHeaps;
    HANDLE* m_phHeaps;
    int m_nHeap;
    _ATL_TERMFUNC_ELEM* m_pTermFuncs;
} _ATL_MODULEA;

typedef struct _ATL_MODULEW_TAG
{
    UINT cbSize;
    HINSTANCE m_hInst;
    HINSTANCE m_hInstResource;
    HINSTANCE m_hInstTypeLib;
    _ATL_OBJMAP_ENTRYW* m_pObjMap;
    LONG m_nLockCnt;
    HANDLE m_hHeap;
    union
    {
        CRITICAL_SECTION m_csTypeInfoHolder;
        CRITICAL_SECTION m_csStaticDataInit;
    } u;
    CRITICAL_SECTION m_csWindowCreate;
    CRITICAL_SECTION m_csObjMap;

    DWORD dwAtlBuildVer;
    _AtlCreateWndData* m_pCreateWndList;
    BOOL m_bDestroyHeap;
    GUID* pguidVer;
    DWORD m_dwHeaps;
    HANDLE* m_phHeaps;
    int m_nHeap;
    _ATL_TERMFUNC_ELEM* m_pTermFuncs;
} _ATL_MODULEW;

typedef struct _ATL_INTMAP_ENTRY_TAG
{
    const IID* piid;
    DWORD dw;
    _ATL_CREATORARGFUNC* pFunc;
} _ATL_INTMAP_ENTRY;

struct _ATL_REGMAP_ENTRY
{
    LPCOLESTR szKey;
    LPCOLESTR szData;
};

HRESULT WINAPI AtlAdvise(IUnknown *pUnkCP, IUnknown *pUnk, const IID * iid, LPDWORD dpw);
HRESULT WINAPI AtlAxAttachControl(IUnknown*,HWND,IUnknown**);
HRESULT WINAPI AtlAxCreateControl(LPCOLESTR,HWND,IStream*,IUnknown**);
HRESULT WINAPI AtlAxCreateControlEx(LPCOLESTR,HWND,IStream*,IUnknown**,IUnknown**,REFIID,IUnknown*);
HRESULT WINAPI AtlFreeMarshalStream(IStream *pStream);
HRESULT WINAPI AtlInternalQueryInterface(void* pThis, const _ATL_INTMAP_ENTRY* pEntries, REFIID iid, void** ppvObject);
HRESULT WINAPI AtlMarshalPtrInProc(IUnknown *pUnk, const IID *iid, IStream **ppStream);
void    WINAPI AtlModuleAddCreateWndData(_ATL_MODULEW *pM, _AtlCreateWndData *pData, void* pvObject);
HRESULT WINAPI AtlModuleAddTermFunc(_ATL_MODULEW *pM, _ATL_TERMFUNC *pFunc, DWORD_PTR dw);
void*  WINAPI AtlModuleExtractCreateWndData(_ATL_MODULEW *pM);
HRESULT WINAPI AtlModuleInit(_ATL_MODULEW* pM, _ATL_OBJMAP_ENTRYW* p, HINSTANCE h);
HRESULT WINAPI AtlModuleLoadTypeLib(_ATL_MODULEW *pM, LPCOLESTR lpszIndex, BSTR *pbstrPath, ITypeLib **ppTypeLib);
HRESULT WINAPI AtlModuleRegisterClassObjects(_ATL_MODULEW* pM, DWORD dwClsContext, DWORD dwFlags);
HRESULT WINAPI AtlModuleRegisterServer(_ATL_MODULEW* pM, BOOL bRegTypeLib, const CLSID* pCLSID);
HRESULT WINAPI AtlModuleRegisterTypeLib(_ATL_MODULEW *pM, LPCOLESTR lpszIndex);
HRESULT WINAPI AtlModuleUnregisterServer(_ATL_MODULEW* pM, const CLSID* pCLSID);
HRESULT WINAPI AtlModuleUpdateRegistryFromResourceD(_ATL_MODULEW *pM, LPCOLESTR lpszRes, BOOL bRegister, struct _ATL_REGMAP_ENTRY *pMapEntries, IRegistrar *pReg );
HRESULT WINAPI AtlModuleUnregisterServerEx(_ATL_MODULEW* pM, BOOL bUnRegTypeLib, const CLSID* pCLSID);
HRESULT WINAPI AtlModuleTerm(_ATL_MODULEW* pM);
HRESULT WINAPI AtlUnadvise(IUnknown *pUnkCP, const IID * iid, DWORD dw);
HRESULT WINAPI AtlUnmarshalPtr(IStream *pStream, const IID *iid, IUnknown **ppUnk);

#endif /* __WINE_ATLBASE_H__ */

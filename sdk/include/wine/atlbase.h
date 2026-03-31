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

#include <atliface.h>
#include <comcat.h>

/* Wine extension: we (ab)use _ATL_VER to handle struct layout differences between ATL versions. */
#define _ATL_VER_30  0x0300
#define _ATL_VER_70  0x0700
#define _ATL_VER_80  0x0800
#define _ATL_VER_90  0x0900
#define _ATL_VER_100 0x0a00
#define _ATL_VER_110 0x0b00

#ifndef _ATL_VER
#define _ATL_VER _ATL_VER_100
#endif

typedef HRESULT (WINAPI _ATL_CREATORFUNC)(void* pv, REFIID riid, LPVOID* ppv);
typedef HRESULT (WINAPI _ATL_CREATORARGFUNC)(void* pv, REFIID riid, LPVOID* ppv, DWORD_PTR dw);
typedef HRESULT (WINAPI _ATL_MODULEFUNC)(DWORD_PTR dw);
typedef LPCSTR (WINAPI _ATL_DESCRIPTIONFUNCA)(void);
typedef LPCWSTR (WINAPI _ATL_DESCRIPTIONFUNCW)(void);
typedef const struct _ATL_CATMAP_ENTRY* (_ATL_CATMAPFUNC)(void);
typedef void (WINAPI _ATL_TERMFUNC)(DWORD_PTR dw);

typedef CRITICAL_SECTION CComCriticalSection;

typedef struct _ATL_OBJMAP_ENTRYA_V1_TAG
{
    const CLSID* pclsid;
    HRESULT (WINAPI *pfnUpdateRegistry)(BOOL bRegister);
    _ATL_CREATORFUNC* pfnGetClassObject;
    _ATL_CREATORFUNC* pfnCreateInstance;
    IUnknown* pCF;
    DWORD dwRegister;
    _ATL_DESCRIPTIONFUNCA* pfnGetObjectDescription;
}_ATL_OBJMAP_ENTRYA_V1;

typedef struct _ATL_OBJMAP_ENTRYW_V1_TAG
{
    const CLSID* pclsid;
    HRESULT (WINAPI *pfnUpdateRegistry)(BOOL bRegister);
    _ATL_CREATORFUNC* pfnGetClassObject;
    _ATL_CREATORFUNC* pfnCreateInstance;
    IUnknown* pCF;
    DWORD dwRegister;
    _ATL_DESCRIPTIONFUNCW* pfnGetObjectDescription;
} _ATL_OBJMAP_ENTRYW_V1;

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
} _ATL_OBJMAP_ENTRYW, _ATL_OBJMAP_ENTRY30, _ATL_OBJMAP_ENTRY;

typedef struct _ATL_OBJMAP_CACHE
{
    IUnknown *pCF;
    DWORD dwRegister;
} _ATL_OBJMAP_CACHE;

typedef struct _ATL_OBJMAP_ENTRY110
{
        const CLSID* pclsid;
        HRESULT (WINAPI *pfnUpdateRegistry)(BOOL bRegister);
        _ATL_CREATORFUNC* pfnGetClassObject;
        _ATL_CREATORFUNC* pfnCreateInstance;
        _ATL_OBJMAP_CACHE* pCache;
        _ATL_DESCRIPTIONFUNCW* pfnGetObjectDescription;
        _ATL_CATMAPFUNC* pfnGetCategoryMap;
        void (WINAPI *pfnObjectMain)(BOOL bStarting);
} _ATL_OBJMAP_ENTRY110, _ATL_OBJMAP_ENTRY_EX;

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

typedef struct
{
    void *m_aT;
    int m_nSize;
    int m_nAllocSize;
} CSimpleArray;

typedef struct _ATL_MODULE70
{
    UINT cbSize;
    LONG m_nLockCnt;
    _ATL_TERMFUNC_ELEM *m_pTermFuncs;
    CComCriticalSection m_csStaticDataInitAndTypeInfo;
} _ATL_MODULE70;

typedef struct _ATL_WIN_MODULE70
{
    UINT cbSize;
    CComCriticalSection m_csWindowCreate;
    _AtlCreateWndData *m_pCreateWndList;
    CSimpleArray /* <ATOM> */ m_rgWindowClassAtoms;
} _ATL_WIN_MODULE70;

#if _ATL_VER >= _ATL_VER_110
typedef struct _ATL_COM_MODULE70
{
    UINT cbSize;
    HINSTANCE m_hInstTypeLib;
    _ATL_OBJMAP_ENTRY_EX **m_ppAutoObjMapFirst;
    _ATL_OBJMAP_ENTRY_EX **m_ppAutoObjMapLast;
    CComCriticalSection m_csObjMap;
} _ATL_COM_MODULE70, _ATL_COM_MODULE;
#else
typedef struct _ATL_COM_MODULE70
{
    UINT cbSize;
    HINSTANCE m_hInstTypeLib;
    _ATL_OBJMAP_ENTRY **m_ppAutoObjMapFirst;
    _ATL_OBJMAP_ENTRY **m_ppAutoObjMapLast;
    CComCriticalSection m_csObjMap;
} _ATL_COM_MODULE70, _ATL_COM_MODULE;
#endif

#if _ATL_VER >= _ATL_VER_70
typedef _ATL_MODULE70 _ATL_MODULE;
typedef _ATL_WIN_MODULE70 _ATL_WIN_MODULE;
#else
typedef _ATL_MODULEW _ATL_MODULE;
typedef _ATL_MODULEW _ATL_WIN_MODULE;
#endif

typedef struct _ATL_INTMAP_ENTRY_TAG
{
    const IID* piid;
    DWORD_PTR dw;
    _ATL_CREATORARGFUNC* pFunc;
} _ATL_INTMAP_ENTRY;

struct _ATL_REGMAP_ENTRY
{
    LPCOLESTR szKey;
    LPCOLESTR szData;
};

struct _ATL_CATMAP_ENTRY
{
    int iType;
    const CATID *pcatid;
};

#define _ATL_CATMAP_ENTRY_END 0
#define _ATL_CATMAP_ENTRY_IMPLEMENTED 1
#define _ATL_CATMAP_ENTRY_REQUIRED 2

HRESULT WINAPI AtlAdvise(IUnknown *pUnkCP, IUnknown *pUnk, const IID * iid, LPDWORD dpw);
HRESULT WINAPI AtlAxAttachControl(IUnknown*,HWND,IUnknown**);
HRESULT WINAPI AtlAxCreateControl(LPCOLESTR,HWND,IStream*,IUnknown**);
HRESULT WINAPI AtlAxCreateControlEx(LPCOLESTR,HWND,IStream*,IUnknown**,IUnknown**,REFIID,IUnknown*);
HRESULT WINAPI AtlFreeMarshalStream(IStream *pStream);
HRESULT WINAPI AtlInternalQueryInterface(void* pThis, const _ATL_INTMAP_ENTRY* pEntries, REFIID iid, void** ppvObject);
HRESULT WINAPI AtlMarshalPtrInProc(IUnknown *pUnk, const IID *iid, IStream **ppStream);
void    WINAPI AtlModuleAddCreateWndData(_ATL_MODULEW *pM, _AtlCreateWndData *pData, void* pvObject);
HRESULT WINAPI AtlWinModuleInit(_ATL_WIN_MODULE*);
void    WINAPI AtlWinModuleAddCreateWndData(_ATL_WIN_MODULE*,_AtlCreateWndData*,void*);
void*   WINAPI AtlWinModuleExtractCreateWndData(_ATL_WIN_MODULE*);
HRESULT WINAPI AtlModuleAddTermFunc(_ATL_MODULE *pM, _ATL_TERMFUNC *pFunc, DWORD_PTR dw);
void WINAPI AtlCallTermFunc(_ATL_MODULE*);
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
HRESULT WINAPI AtlCreateRegistrar(IRegistrar**);
HRESULT WINAPI AtlUpdateRegistryFromResourceD(HINSTANCE,LPCOLESTR,BOOL,struct _ATL_REGMAP_ENTRY*,IRegistrar*);
HRESULT WINAPI AtlLoadTypeLib(HINSTANCE,LPCOLESTR,BSTR*,ITypeLib**);
HRESULT WINAPI AtlRegisterTypeLib(HINSTANCE,LPCOLESTR);
HRESULT WINAPI AtlRegisterClassCategoriesHelper(REFCLSID,const struct _ATL_CATMAP_ENTRY*,BOOL);
HRESULT WINAPI AtlComModuleGetClassObject(_ATL_COM_MODULE*,REFCLSID,REFIID,void**);
HRESULT WINAPI AtlComModuleRegisterClassObjects(_ATL_COM_MODULE*,DWORD,DWORD);
HRESULT WINAPI AtlComModuleRevokeClassObjects(_ATL_COM_MODULE*);
HRESULT WINAPI AtlComModuleUnregisterServer(_ATL_COM_MODULE*,BOOL,const CLSID*);
BOOL WINAPI AtlWaitWithMessageLoop(HANDLE);
HRESULT WINAPI AtlGetObjectSourceInterface(IUnknown*,GUID*,IID*,unsigned short*,unsigned short*);
HRESULT WINAPI AtlSetPerUserRegistration(unsigned char /*bool*/);
HRESULT WINAPI AtlGetPerUserRegistration(unsigned char /*bool*/ *);

#endif /* __WINE_ATLBASE_H__ */

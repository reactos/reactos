/*
 * ReactOS ATL
 *
 * Copyright 2009 Andrew Hill <ash77@reactos.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include "atlcore.h"
#include "statreg.h"

#ifdef _MSC_VER
// It is common to use this in ATL constructors. They only store this for later use, so the usage is safe.
#pragma warning(disable:4355)
#endif

#ifndef _ATL_PACKING
#define _ATL_PACKING 8
#endif

#ifndef _ATL_FREE_THREADED
#ifndef _ATL_APARTMENT_THREADED
#ifndef _ATL_SINGLE_THREADED
#define _ATL_FREE_THREADED
#endif
#endif
#endif

#ifndef ATLTRY
#define ATLTRY(x) x;
#endif

#ifdef _ATL_DISABLE_NO_VTABLE
#define ATL_NO_VTABLE
#else
#define ATL_NO_VTABLE __declspec(novtable)
#endif

#define offsetofclass(base, derived) (reinterpret_cast<DWORD_PTR>(static_cast<base *>(reinterpret_cast<derived *>(_ATL_PACKING))) - _ATL_PACKING)

namespace ATL
{

class CAtlModule;
class CComModule;
class CAtlComModule;
__declspec(selectany) CAtlModule *_pAtlModule = NULL;
__declspec(selectany) CComModule *_pModule = NULL;
extern CAtlComModule _AtlComModule;

typedef HRESULT (WINAPI _ATL_CREATORFUNC)(void *pv, REFIID riid, LPVOID *ppv);
typedef LPCTSTR (WINAPI _ATL_DESCRIPTIONFUNC)();
typedef const struct _ATL_CATMAP_ENTRY * (_ATL_CATMAPFUNC)();

struct _ATL_OBJMAP_ENTRY30
{
    const CLSID *pclsid;
    HRESULT (WINAPI *pfnUpdateRegistry)(BOOL bRegister);
    _ATL_CREATORFUNC *pfnGetClassObject;
    _ATL_CREATORFUNC *pfnCreateInstance;
    IUnknown *pCF;
    DWORD dwRegister;
    _ATL_DESCRIPTIONFUNC *pfnGetObjectDescription;
    _ATL_CATMAPFUNC *pfnGetCategoryMap;
    void (WINAPI *pfnObjectMain)(bool bStarting);

    HRESULT WINAPI RevokeClassObject()
    {
        if (dwRegister == 0)
            return S_OK;
        return CoRevokeClassObject(dwRegister);
    }

    HRESULT WINAPI RegisterClassObject(DWORD dwClsContext, DWORD dwFlags)
    {
        IUnknown *p;
        HRESULT hResult;

        p = NULL;
        if (pfnGetClassObject == NULL)
            return S_OK;

        hResult = pfnGetClassObject(reinterpret_cast<LPVOID *>(pfnCreateInstance), IID_IUnknown, reinterpret_cast<LPVOID *>(&p));
        if (SUCCEEDED(hResult))
            hResult = CoRegisterClassObject(*pclsid, p, dwClsContext, dwFlags, &dwRegister);

        if (p != NULL)
            p->Release();

        return hResult;
    }
};

typedef _ATL_OBJMAP_ENTRY30 _ATL_OBJMAP_ENTRY;

typedef void (__stdcall _ATL_TERMFUNC)(DWORD_PTR dw);

struct _ATL_TERMFUNC_ELEM
{
    _ATL_TERMFUNC *pFunc;
    DWORD_PTR dw;
    _ATL_TERMFUNC_ELEM *pNext;
};

struct _ATL_MODULE70
{
    UINT cbSize;
    LONG m_nLockCnt;
    _ATL_TERMFUNC_ELEM *m_pTermFuncs;
    CComCriticalSection m_csStaticDataInitAndTypeInfo;
};
typedef _ATL_MODULE70 _ATL_MODULE;

typedef HRESULT (WINAPI _ATL_CREATORARGFUNC)(void *pv, REFIID riid, LPVOID *ppv, DWORD_PTR dw);

#define _ATL_SIMPLEMAPENTRY ((ATL::_ATL_CREATORARGFUNC *)1)

struct _ATL_INTMAP_ENTRY
{
    const IID *piid;
    DWORD_PTR dw;
    _ATL_CREATORARGFUNC *pFunc;
};

struct _AtlCreateWndData
{
    void *m_pThis;
    DWORD m_dwThreadID;
    _AtlCreateWndData *m_pNext;
};

struct _ATL_COM_MODULE70
{
    UINT cbSize;
    HINSTANCE m_hInstTypeLib;
    _ATL_OBJMAP_ENTRY **m_ppAutoObjMapFirst;
    _ATL_OBJMAP_ENTRY **m_ppAutoObjMapLast;
    CComCriticalSection m_csObjMap;
};
typedef _ATL_COM_MODULE70 _ATL_COM_MODULE;

struct _ATL_WIN_MODULE70
{
    UINT cbSize;
    CComCriticalSection m_csWindowCreate;
    _AtlCreateWndData *m_pCreateWndList;
#ifdef NOTYET
    CSimpleArray<ATOM>                        m_rgWindowClassAtoms;
#endif
};
typedef _ATL_WIN_MODULE70 _ATL_WIN_MODULE;

struct _ATL_REGMAP_ENTRY
{
    LPCOLESTR szKey;
    LPCOLESTR szData;
};

HRESULT __stdcall AtlWinModuleInit(_ATL_WIN_MODULE *pWinModule);
HRESULT __stdcall AtlWinModuleTerm(_ATL_WIN_MODULE *pWinModule, HINSTANCE hInst);
HRESULT __stdcall AtlInternalQueryInterface(void *pThis, const _ATL_INTMAP_ENTRY *pEntries, REFIID iid, void **ppvObject);
void __stdcall AtlWinModuleAddCreateWndData(_ATL_WIN_MODULE *pWinModule, _AtlCreateWndData *pData, void *pObject);
void *__stdcall AtlWinModuleExtractCreateWndData(_ATL_WIN_MODULE *pWinModule);
HRESULT __stdcall AtlComModuleGetClassObject(_ATL_COM_MODULE *pComModule, REFCLSID rclsid, REFIID riid, LPVOID *ppv);

template<class TLock>
class CComCritSecLock
{
private:
    bool m_bLocked;
    TLock &m_cs;
public:
    CComCritSecLock(TLock &cs, bool bInitialLock = true) : m_cs(cs)
    {
        HRESULT hResult;

        m_bLocked = false;
        if (bInitialLock)
        {
            hResult = Lock();
            if (FAILED(hResult))
            {
                ATLASSERT(false);
            }
        }
    }

    ~CComCritSecLock()
    {
        if (m_bLocked)
            Unlock();
    }

    HRESULT Lock()
    {
        HRESULT                                    hResult;

        ATLASSERT(!m_bLocked);
        hResult = m_cs.Lock();
        if (FAILED(hResult))
            return hResult;
        m_bLocked = true;

        return S_OK;
    }

    void Unlock()
    {
        HRESULT                                    hResult;

        ATLASSERT(m_bLocked);
        hResult = m_cs.Unlock();
        if (FAILED(hResult))
        {
            ATLASSERT(false);
        }
        m_bLocked = false;
    }
};


class CHandle
{
public:
    HANDLE m_handle;

public:
    CHandle() :
        m_handle(NULL)
    {
    }

    CHandle(_Inout_ CHandle& handle) :
        m_handle(NULL)
    {
        Attach(handle.Detach());
    }

    explicit CHandle(_In_ HANDLE handle) :
        m_handle(handle)
    {
    }

    ~CHandle()
    {
        if (m_handle)
        {
            Close();
        }
    }

    CHandle& operator=(_Inout_ CHandle& handle)
    {
        if (this != &handle)
        {
            if (m_handle)
            {
                Close();
            }
            Attach(handle.Detach());
        }

        return *this;
    }

    operator HANDLE() const
    {
        return m_handle;
    }

    void Attach(_In_ HANDLE handle)
    {
        ATLASSERT(m_handle == NULL);
        m_handle = handle;
    }

    HANDLE Detach()
    {
        HANDLE handle = m_handle;
        m_handle = NULL;
        return handle;
    }

    void Close()
    {
        if (m_handle)
        {
            ::CloseHandle(m_handle);
            m_handle = NULL;
        }
    }
};


inline BOOL WINAPI InlineIsEqualUnknown(REFGUID rguid1)
{
   return (
      ((unsigned long *)&rguid1)[0] == 0 &&
      ((unsigned long *)&rguid1)[1] == 0 &&
      ((unsigned long *)&rguid1)[2] == 0x000000C0 &&
      ((unsigned long *)&rguid1)[3] == 0x46000000);
}

class CComMultiThreadModelNoCS
{
public:
    typedef CComFakeCriticalSection AutoCriticalSection;
    typedef CComFakeCriticalSection CriticalSection;
    typedef CComMultiThreadModelNoCS ThreadModelNoCS;
    typedef CComFakeCriticalSection AutoDeleteCriticalSection;

    static ULONG WINAPI Increment(LPLONG p)
    {
        return InterlockedIncrement(p);
    }

    static ULONG WINAPI Decrement(LPLONG p)
    {
        return InterlockedDecrement(p);
    }
};

class CComMultiThreadModel
{
public:
    typedef CComAutoCriticalSection AutoCriticalSection;
    typedef CComCriticalSection CriticalSection;
    typedef CComMultiThreadModelNoCS ThreadModelNoCS;
    typedef CComAutoDeleteCriticalSection AutoDeleteCriticalSection;

    static ULONG WINAPI Increment(LPLONG p)
    {
        return InterlockedIncrement(p);
    }

    static ULONG WINAPI Decrement(LPLONG p)
    {
        return InterlockedDecrement(p);
    }
};

class CComSingleThreadModel
{
public:
    typedef CComFakeCriticalSection AutoCriticalSection;
    typedef CComFakeCriticalSection CriticalSection;
    typedef CComSingleThreadModel ThreadModelNoCS;
    typedef CComFakeCriticalSection AutoDeleteCriticalSection;

    static ULONG WINAPI Increment(LPLONG p)
    {
        return ++*p;
    }

    static ULONG WINAPI Decrement(LPLONG p)
    {
        return --*p;
    }
};

#if defined(_ATL_FREE_THREADED)

    typedef CComMultiThreadModel CComObjectThreadModel;
    typedef CComMultiThreadModel CComGlobalsThreadModel;

#elif defined(_ATL_APARTMENT_THREADED)

    typedef CComSingleThreadModel CComObjectThreadModel;
    typedef CComMultiThreadModel CComGlobalsThreadModel;

#elif defined(_ATL_SINGLE_THREADED)

    typedef CComSingleThreadModel CComObjectThreadModel;
    typedef CComSingleThreadModel CComGlobalsThreadModel;

#else
#error No threading model
#endif

class CAtlModule : public _ATL_MODULE
{
protected:
    static GUID m_libid;
public:
    CAtlModule()
    {
        ATLASSERT(_pAtlModule == NULL);
        _pAtlModule = this;
        cbSize = sizeof(_ATL_MODULE);
        m_nLockCnt = 0;
    }

    virtual LONG GetLockCount()
    {
        return m_nLockCnt;
    }

    virtual LONG Lock()
    {
        return CComGlobalsThreadModel::Increment(&m_nLockCnt);
    }

    virtual LONG Unlock()
    {
        return CComGlobalsThreadModel::Decrement(&m_nLockCnt);
    }

    virtual HRESULT AddCommonRGSReplacements(IRegistrarBase* /*pRegistrar*/) = 0;

    HRESULT WINAPI UpdateRegistryFromResource(LPCTSTR lpszRes, BOOL bRegister, struct _ATL_REGMAP_ENTRY *pMapEntries = NULL)
    {
        CRegObject registrar;
        WCHAR modulePath[MAX_PATH];
        HRESULT hResult;
        PCWSTR lpwszRes;

        hResult = CommonInitRegistrar(registrar, modulePath, sizeof(modulePath) / sizeof(modulePath[0]), pMapEntries);
        if (FAILED(hResult))
            return hResult;
#ifdef UNICODE
        lpwszRes = lpszRes;
#else
        /* FIXME: this is a bit of a hack, need to re-evaluate */
        WCHAR resid[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, lpszRes, -1, resid, MAX_PATH);
        lpwszRes = resid;
#endif
        if (bRegister != FALSE)
            hResult = registrar.ResourceRegisterSz(modulePath, lpwszRes, L"REGISTRY");
        else
            hResult = registrar.ResourceUnregisterSz(modulePath, lpwszRes, L"REGISTRY");

        return hResult;
    }

    HRESULT WINAPI UpdateRegistryFromResource(UINT nResID, BOOL bRegister, struct _ATL_REGMAP_ENTRY *pMapEntries = NULL)
    {
        CRegObject registrar;
        WCHAR modulePath[MAX_PATH];
        HRESULT hResult;

        hResult = CommonInitRegistrar(registrar, modulePath, sizeof(modulePath) / sizeof(modulePath[0]), pMapEntries);
        if (FAILED(hResult))
            return hResult;

        if (bRegister != FALSE)
            hResult = registrar.ResourceRegister(modulePath, nResID, L"REGISTRY");
        else
            hResult = registrar.ResourceUnregister(modulePath, nResID, L"REGISTRY");

        return hResult;
    }

private:
    HRESULT CommonInitRegistrar(CRegObject &registrar, WCHAR *modulePath, DWORD modulePathCount, struct _ATL_REGMAP_ENTRY *pMapEntries)
    {
        HINSTANCE hInstance;
        DWORD dwFLen;
        HRESULT hResult;

        hInstance = _AtlBaseModule.GetModuleInstance();
        dwFLen = GetModuleFileNameW(hInstance, modulePath, modulePathCount);
        if (dwFLen == modulePathCount)
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        else if (dwFLen == 0)
            return HRESULT_FROM_WIN32(GetLastError());

        if (pMapEntries != NULL)
        {
            while (pMapEntries->szKey != NULL)
            {
                ATLASSERT(pMapEntries->szData != NULL);
                hResult = registrar.AddReplacement(pMapEntries->szKey, pMapEntries->szData);
                if (FAILED(hResult))
                    return hResult;
                pMapEntries++;
            }
        }

        hResult = AddCommonRGSReplacements(&registrar);
        if (FAILED(hResult))
            return hResult;

        hResult = registrar.AddReplacement(L"Module", modulePath);
        if (FAILED(hResult))
            return hResult;

        hResult = registrar.AddReplacement(L"Module_Raw", modulePath);
        if (FAILED(hResult))
            return hResult;

        return S_OK;
    }
};

__declspec(selectany) GUID CAtlModule::m_libid = {0x0, 0x0, 0x0, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0} };

template <class T>
class CAtlModuleT : public CAtlModule
{
public:

    virtual HRESULT AddCommonRGSReplacements(IRegistrarBase *pRegistrar)
    {
        return pRegistrar->AddReplacement(L"APPID", T::GetAppId());
    }

    static LPCOLESTR GetAppId()
    {
        return L"";
    }
};

class CAtlComModule : public _ATL_COM_MODULE
{
public:
    CAtlComModule()
    {
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)this, &m_hInstTypeLib);
        m_ppAutoObjMapFirst = NULL;
        m_ppAutoObjMapLast = NULL;
        if (FAILED(m_csObjMap.Init()))
        {
            ATLASSERT(0);
            CAtlBaseModule::m_bInitFailed = true;
            return;
        }
        cbSize = sizeof(_ATL_COM_MODULE);
    }

    ~CAtlComModule()
    {
        Term();
    }

    void Term()
    {
        if (cbSize != 0)
        {
            ATLASSERT(m_ppAutoObjMapFirst == NULL);
            ATLASSERT(m_ppAutoObjMapLast == NULL);
            m_csObjMap.Term();
            cbSize = 0;
        }
    }
};

template <class T>
class CAtlDllModuleT : public CAtlModuleT<T>
{
public:
    CAtlDllModuleT()
    {
    }

    HRESULT DllCanUnloadNow()
    {
        T *pThis;

        pThis = static_cast<T *>(this);
        if (pThis->GetLockCount() == 0)
            return S_OK;
        return S_FALSE;
    }

    HRESULT DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
    {
        T *pThis;

        pThis = static_cast<T *>(this);
        return pThis->GetClassObject(rclsid, riid, ppv);
    }

    HRESULT DllRegisterServer(BOOL bRegTypeLib = TRUE)
    {
        T *pThis;
        HRESULT hResult;

        pThis = static_cast<T *>(this);
        hResult = pThis->RegisterServer(bRegTypeLib);
        return hResult;
    }

    HRESULT DllUnregisterServer(BOOL bUnRegTypeLib = TRUE)
    {
        T *pThis;
        HRESULT hResult;

        pThis = static_cast<T *>(this);
        hResult = pThis->UnregisterServer(bUnRegTypeLib);
        return hResult;
    }

    HRESULT GetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
    {
        return AtlComModuleGetClassObject(&_AtlComModule, rclsid, riid, ppv);
    }
};

class CComModule : public CAtlModuleT<CComModule>
{
public:
    _ATL_OBJMAP_ENTRY *m_pObjMap;
public:
    CComModule()
    {
        ATLASSERT(_pModule == NULL);
        _pModule = this;
        _pModule->m_pObjMap = NULL;
    }

    ~CComModule()
    {
        _pModule = NULL;
    }

    HRESULT Init(_ATL_OBJMAP_ENTRY *p, HINSTANCE /* h */, const GUID *plibid)
    {
        _ATL_OBJMAP_ENTRY *objectMapEntry;

        if (plibid != NULL)
            m_libid = *plibid;

        if (p != reinterpret_cast<_ATL_OBJMAP_ENTRY *>(-1))
        {
            m_pObjMap = p;
            if (p != NULL)
            {
                objectMapEntry = p;
                while (objectMapEntry->pclsid != NULL)
                {
                    objectMapEntry->pfnObjectMain(true);
                    objectMapEntry++;
                }
            }
        }
        return S_OK;
    }

    void Term()
    {
        _ATL_OBJMAP_ENTRY                    *objectMapEntry;

        if (m_pObjMap != NULL)
        {
            objectMapEntry = m_pObjMap;
            while (objectMapEntry->pclsid != NULL)
            {
                if (objectMapEntry->pCF != NULL)
                    objectMapEntry->pCF->Release();
                objectMapEntry->pCF = NULL;
                objectMapEntry->pfnObjectMain(false);
                objectMapEntry++;
            }
        }
    }

    HRESULT GetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
    {
        _ATL_OBJMAP_ENTRY                    *objectMapEntry;
        HRESULT                                hResult;

        ATLASSERT(ppv != NULL);
        if (ppv == NULL)
            return E_POINTER;
        *ppv = NULL;
        hResult = S_OK;
        if (m_pObjMap != NULL)
        {
            objectMapEntry = m_pObjMap;
            while (objectMapEntry->pclsid != NULL)
            {
                if (objectMapEntry->pfnGetClassObject != NULL && InlineIsEqualGUID(rclsid, *objectMapEntry->pclsid) != FALSE)
                {
                    if (objectMapEntry->pCF == NULL)
                    {
                        CComCritSecLock<CComCriticalSection> lock(_AtlComModule.m_csObjMap, true);

                        if (objectMapEntry->pCF == NULL)
                            hResult = objectMapEntry->pfnGetClassObject(reinterpret_cast<void *>(objectMapEntry->pfnCreateInstance), IID_IUnknown, reinterpret_cast<LPVOID *>(&objectMapEntry->pCF));
                    }
                    if (objectMapEntry->pCF != NULL)
                        hResult = objectMapEntry->pCF->QueryInterface(riid, ppv);
                    break;
                }
                objectMapEntry++;
            }
        }
        if (hResult == S_OK && *ppv == NULL)
        {
            // FIXME: call AtlComModuleGetClassObject
            hResult = CLASS_E_CLASSNOTAVAILABLE;
        }
        return hResult;
    }

    HRESULT RegisterServer(BOOL bRegTypeLib = FALSE, const CLSID *pCLSID = NULL)
    {
        _ATL_OBJMAP_ENTRY *objectMapEntry;
        HRESULT hResult;

        hResult = S_OK;
        objectMapEntry = m_pObjMap;
        if (objectMapEntry != NULL)
        {
            while (objectMapEntry->pclsid != NULL)
            {
                if (pCLSID == NULL || IsEqualGUID(*pCLSID, *objectMapEntry->pclsid) != FALSE)
                {
                    hResult = objectMapEntry->pfnUpdateRegistry(TRUE);
                    if (FAILED(hResult))
                        break;
                }
                objectMapEntry++;
            }
        }
        return hResult;
    }

    HRESULT UnregisterServer(BOOL bUnRegTypeLib, const CLSID *pCLSID = NULL)
    {
        _ATL_OBJMAP_ENTRY *objectMapEntry;
        HRESULT hResult;

        hResult = S_OK;
        objectMapEntry = m_pObjMap;
        if (objectMapEntry != NULL)
        {
            while (objectMapEntry->pclsid != NULL)
            {
                if (pCLSID == NULL || IsEqualGUID(*pCLSID, *objectMapEntry->pclsid) != FALSE)
                {
                    hResult = objectMapEntry->pfnUpdateRegistry(FALSE); //unregister
                    if (FAILED(hResult))
                        break;
                }
                objectMapEntry++;
            }
        }
        return hResult;
    }

    HRESULT DllCanUnloadNow()
    {
        if (GetLockCount() == 0)
            return S_OK;
        return S_FALSE;
    }

    HRESULT DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
    {
        return GetClassObject(rclsid, riid, ppv);
    }

    HRESULT DllRegisterServer(BOOL bRegTypeLib = TRUE)
    {
        return RegisterServer(bRegTypeLib);
    }

    HRESULT DllUnregisterServer(BOOL bUnRegTypeLib = TRUE)
    {
        return UnregisterServer(bUnRegTypeLib);
    }

};

class CAtlWinModule : public _ATL_WIN_MODULE
{
public:
    CAtlWinModule()
    {
        HRESULT hResult;

        hResult = AtlWinModuleInit(this);
        if (FAILED(hResult))
        {
            CAtlBaseModule::m_bInitFailed = true;
            ATLASSERT(0);
        }
    }

    ~CAtlWinModule()
    {
        Term();
    }

    void Term()
    {
        AtlWinModuleTerm(this, _AtlBaseModule.GetModuleInstance());
    }

    void AddCreateWndData(_AtlCreateWndData *pData, void *pObject)
    {
        AtlWinModuleAddCreateWndData(this, pData, pObject);
    }

    void *ExtractCreateWndData()
    {
        return AtlWinModuleExtractCreateWndData(this);
    }
};

extern CAtlWinModule _AtlWinModule;

template<class T>
class CComPtr
{
public:
    T *p;
public:
    CComPtr()
    {
        p = NULL;
    }

    CComPtr(T *lp)
    {
        p = lp;
        if (p != NULL)
            p->AddRef();
    }

    CComPtr(const CComPtr<T> &lp)
    {
        p = lp.p;
        if (p != NULL)
            p->AddRef();
    }

    ~CComPtr()
    {
        if (p != NULL)
            p->Release();
    }

    T *operator = (T *lp)
    {
        if (p != NULL)
            p->Release();
        p = lp;
        if (p != NULL)
            p->AddRef();
        return *this;
    }

    T *operator = (const CComPtr<T> &lp)
    {
        if (p != NULL)
            p->Release();
        p = lp.p;
        if (p != NULL)
            p->AddRef();
        return *this;
    }

    void Release()
    {
        if (p != NULL)
        {
            p->Release();
            p = NULL;
        }
    }

    void Attach(T *lp)
    {
        if (p != NULL)
            p->Release();
        p = lp;
    }

    T *Detach()
    {
        T *saveP;

        saveP = p;
        p = NULL;
        return saveP;
    }

    T **operator & ()
    {
        ATLASSERT(p == NULL);
        return &p;
    }

    operator T * ()
    {
        return p;
    }

    T *operator -> ()
    {
        ATLASSERT(p != NULL);
        return p;
    }
};


// TODO: When someone needs it, make the allocator a template, so you can use it for both
//       CoTask* allocations, and CRT-like allocations (malloc, realloc, free)
template<class T>
class CComHeapPtr
{
public:
    CComHeapPtr() :
        m_Data(NULL)
    {
    }

    explicit CComHeapPtr(T *lp) :
        m_Data(lp)
    {
    }

    explicit CComHeapPtr(CComHeapPtr<T> &lp)
    {
        m_Data = lp.Detach();
    }

    ~CComHeapPtr()
    {
        Release();
    }

    T *operator = (CComHeapPtr<T> &lp)
    {
        if (lp.m_Data != m_Data)
            Attach(lp.Detach());
        return *this;
    }

    bool Allocate(size_t nElements = 1)
    {
        ATLASSERT(m_Data == NULL);
        m_Data = static_cast<T*>(::CoTaskMemAlloc(nElements * sizeof(T)));
        return m_Data != NULL;
    }

    bool Reallocate(_In_ size_t nElements)
    {
        T* newData = static_cast<T*>(::CoTaskMemRealloc(m_Data, nElements * sizeof(T)));
        if (newData == NULL)
            return false;
        m_Data = newData;
        return true;
    }

    void Release()
    {
        if (m_Data)
        {
            ::CoTaskMemFree(m_Data);
            m_Data = NULL;
        }
    }

    void Attach(T *lp)
    {
        Release();
        m_Data = lp;
    }

    T *Detach()
    {
        T *saveP = m_Data;
        m_Data = NULL;
        return saveP;
    }

    T **operator &()
    {
        ATLASSERT(m_Data == NULL);
        return &m_Data;
    }

    operator T* () const
    {
        return m_Data;
    }

protected:
    T *m_Data;
};


class CComBSTR
{
public:
    BSTR m_str;
public:
    CComBSTR() :
        m_str(NULL)
    {
    }

    CComBSTR(LPCOLESTR pSrc)
    {
        if (pSrc == NULL)
            m_str = NULL;
        else
            m_str = ::SysAllocString(pSrc);
    }

    CComBSTR(int length)
    {
        if (length == 0)
            m_str = NULL;
        else
            m_str = ::SysAllocStringLen(NULL, length);
    }

    CComBSTR(int length, LPCOLESTR pSrc)
    {
        if (length == 0)
            m_str = NULL;
        else
            m_str = ::SysAllocStringLen(pSrc, length);
    }

    CComBSTR(PCSTR pSrc)
    {
        if (pSrc)
        {
            int len = MultiByteToWideChar(CP_THREAD_ACP, 0, pSrc, -1, NULL, 0);
            m_str = ::SysAllocStringLen(NULL, len - 1);
            if (m_str)
            {
                int res = MultiByteToWideChar(CP_THREAD_ACP, 0, pSrc, -1, m_str, len);
                ATLASSERT(res == len);
                if (res != len)
                {
                    ::SysFreeString(m_str);
                    m_str = NULL;
                }
            }
        }
        else
        {
            m_str = NULL;
        }
    }

    CComBSTR(const CComBSTR &other)
    {
        m_str = other.Copy();
    }

    CComBSTR(REFGUID guid)
    {
        OLECHAR szGuid[40];
        ::StringFromGUID2(guid, szGuid, 40);
        m_str = ::SysAllocString(szGuid);
    }

    ~CComBSTR()
    {
        ::SysFreeString(m_str);
        m_str = NULL;
    }

    operator BSTR () const
    {
        return m_str;
    }

    BSTR *operator & ()
    {
        return &m_str;
    }

    CComBSTR &operator = (const CComBSTR &other)
    {
        ::SysFreeString(m_str);
        m_str = other.Copy();
        return *this;
    }

    BSTR Copy() const
    {
        if (!m_str)
            return NULL;
        return ::SysAllocStringLen(m_str, ::SysStringLen(m_str));
    }

    HRESULT CopyTo(BSTR *other) const
    {
        if (!other)
            return E_POINTER;
        *other = Copy();
        return S_OK;
    }

    bool LoadString(HMODULE module, DWORD uID)
    {
        ::SysFreeString(m_str);
        m_str = NULL;
        const wchar_t *ptr = NULL;
        int len = ::LoadStringW(module, uID, (PWSTR)&ptr, 0);
        if (len)
            m_str = ::SysAllocStringLen(ptr, len);
        return m_str != NULL;
    }

    unsigned int Length() const
    {
        return ::SysStringLen(m_str);
    }

    unsigned int ByteLength() const
    {
        return ::SysStringByteLen(m_str);
    }
};

class CComVariant : public tagVARIANT
{
public:
    CComVariant()
    {
        ::VariantInit(this);
    }

    ~CComVariant()
    {
        Clear();
    }

    HRESULT Clear()
    {
        return ::VariantClear(this);
    }
};

inline HRESULT __stdcall AtlAdvise(IUnknown *pUnkCP, IUnknown *pUnk, const IID &iid, LPDWORD pdw)
{
    CComPtr<IConnectionPointContainer>        container;
    CComPtr<IConnectionPoint>                connectionPoint;
    HRESULT                                    hResult;

    if (pUnkCP == NULL)
        return E_INVALIDARG;
    hResult = pUnkCP->QueryInterface(IID_IConnectionPointContainer, (void **)&container);
    if (FAILED(hResult))
        return hResult;
    hResult = container->FindConnectionPoint(iid, &connectionPoint);
    if (FAILED(hResult))
        return hResult;
    return connectionPoint->Advise(pUnk, pdw);
}

inline HRESULT __stdcall AtlUnadvise(IUnknown *pUnkCP, const IID &iid, DWORD dw)
{
    CComPtr<IConnectionPointContainer>        container;
    CComPtr<IConnectionPoint>                connectionPoint;
    HRESULT                                    hResult;

    if (pUnkCP == NULL)
        return E_INVALIDARG;
    hResult = pUnkCP->QueryInterface(IID_IConnectionPointContainer, (void **)&container);
    if (FAILED(hResult))
        return hResult;
    hResult = container->FindConnectionPoint(iid, &connectionPoint);
    if (FAILED(hResult))
        return hResult;
    return connectionPoint->Unadvise(dw);
}

inline HRESULT __stdcall AtlInternalQueryInterface(void *pThis, const _ATL_INTMAP_ENTRY *pEntries, REFIID iid, void **ppvObject)
{
    int i;
    IUnknown *resultInterface;
    HRESULT hResult;

    ATLASSERT(pThis != NULL && pEntries != NULL);
    if (pThis == NULL || pEntries == NULL)
        return E_INVALIDARG;
    ATLASSERT(ppvObject != NULL);
    if (ppvObject == NULL)
        return E_POINTER;

    if (InlineIsEqualUnknown(iid))
    {
        resultInterface = reinterpret_cast<IUnknown *>(reinterpret_cast<char *>(pThis) + pEntries[0].dw);
        *ppvObject = resultInterface;
        resultInterface->AddRef();
        return S_OK;
    }

    i = 0;
    while (pEntries[i].pFunc != 0)
    {
        if (pEntries[i].piid == NULL || InlineIsEqualGUID(iid, *pEntries[i].piid))
        {
            if (pEntries[i].pFunc == reinterpret_cast<_ATL_CREATORARGFUNC *>(1))
            {
                ATLASSERT(pEntries[i].piid != NULL);
                resultInterface = reinterpret_cast<IUnknown *>(reinterpret_cast<char *>(pThis) + pEntries[i].dw);
                *ppvObject = resultInterface;
                resultInterface->AddRef();
                return S_OK;
            }
            else
            {
                hResult = pEntries[i].pFunc(pThis, iid, ppvObject, 0);
                if (hResult == S_OK || (FAILED(hResult) && pEntries[i].piid != NULL))
                    return hResult;
            }
            break;
        }
        i++;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

inline HRESULT __stdcall AtlWinModuleInit(_ATL_WIN_MODULE *pWinModule)
{
    if (pWinModule == NULL)
        return E_INVALIDARG;
    pWinModule->m_pCreateWndList = NULL;
    return pWinModule->m_csWindowCreate.Init();
}

inline HRESULT __stdcall AtlWinModuleTerm(_ATL_WIN_MODULE *pWinModule, HINSTANCE hInst)
{
    if (pWinModule == NULL)
        return E_INVALIDARG;
    pWinModule->m_csWindowCreate.Term();
    return S_OK;
}

inline void __stdcall AtlWinModuleAddCreateWndData(_ATL_WIN_MODULE *pWinModule, _AtlCreateWndData *pData, void *pObject)
{
    CComCritSecLock<CComCriticalSection> lock(pWinModule->m_csWindowCreate, true);

    ATLASSERT(pWinModule != NULL);
    ATLASSERT(pObject != NULL);

    pData->m_pThis = pObject;
    pData->m_dwThreadID = ::GetCurrentThreadId();
    pData->m_pNext = pWinModule->m_pCreateWndList;
    pWinModule->m_pCreateWndList = pData;
}

inline void *__stdcall AtlWinModuleExtractCreateWndData(_ATL_WIN_MODULE *pWinModule)
{
    CComCritSecLock<CComCriticalSection> lock(pWinModule->m_csWindowCreate, true);
    void *result;
    _AtlCreateWndData *currentEntry;
    _AtlCreateWndData **previousLink;
    DWORD threadID;

    ATLASSERT(pWinModule != NULL);

    result = NULL;
    threadID = GetCurrentThreadId();
    currentEntry = pWinModule->m_pCreateWndList;
    previousLink = &pWinModule->m_pCreateWndList;
    while (currentEntry != NULL)
    {
        if (currentEntry->m_dwThreadID == threadID)
        {
            *previousLink = currentEntry->m_pNext;
            result = currentEntry->m_pThis;
            break;
        }
        previousLink = &currentEntry->m_pNext;
        currentEntry = currentEntry->m_pNext;
    }
    return result;
}

}; // namespace ATL

#ifndef _ATL_NO_AUTOMATIC_NAMESPACE
using namespace ATL;
#endif //!_ATL_NO_AUTOMATIC_NAMESPACE

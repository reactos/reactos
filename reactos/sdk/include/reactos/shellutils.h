/*
 * Copyright 1999, 2000 Juergen Schmied
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

#ifndef __ROS_SHELL_UTILS_H
#define __ROS_SHELL_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#ifdef __cplusplus
#   define IID_PPV_ARG(Itype, ppType) IID_##Itype, reinterpret_cast<void**>((static_cast<Itype**>(ppType)))
#   define IID_NULL_PPV_ARG(Itype, ppType) IID_##Itype, NULL, reinterpret_cast<void**>((static_cast<Itype**>(ppType)))
#else
#   define IID_PPV_ARG(Itype, ppType) IID_##Itype, (void**)(ppType)
#   define IID_NULL_PPV_ARG(Itype, ppType) IID_##Itype, NULL, (void**)(ppType)
#endif

#if 1
#define FAILED_UNEXPECTEDLY(hr) (FAILED(hr) && (Win32DbgPrint(__FILE__, __LINE__, "Unexpected failure %08x.\n", hr), TRUE))
#else
#define FAILED_UNEXPECTEDLY(hr) FAILED(hr)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#ifdef __cplusplus
template <typename T>
class CComCreatorCentralInstance
{
private:
    static IUnknown *s_pInstance;

public:
    static HRESULT WINAPI CreateInstance(void *pv, REFIID riid, LPVOID *ppv)
    {
        *ppv = NULL;
        if (pv != NULL)
            return CLASS_E_NOAGGREGATION;
        if (!s_pInstance)
        {
            PVOID pObj;
            HRESULT hr;
            hr = ATL::CComCreator< T >::CreateInstance(NULL, IID_IUnknown, &pObj);
            if (FAILED(hr))
                return hr;
            if (InterlockedCompareExchangePointer((PVOID *)&s_pInstance, pObj, NULL))
                static_cast<IUnknown *>(pObj)->Release();
        }
        return s_pInstance->QueryInterface(riid, ppv);
    }
    static void Term()
    {
        if (s_pInstance)
        {
            s_pInstance->Release();
            s_pInstance = NULL;
        }
    }
};

template <typename T>
IUnknown *CComCreatorCentralInstance<T>::s_pInstance = NULL;

#define DECLARE_CENTRAL_INSTANCE_NOT_AGGREGATABLE(x)                            \
public:                                                                         \
    typedef CComCreatorCentralInstance< ATL::CComObject<x> > _CreatorClass;
#endif

#ifdef __cplusplus
template <class Base>
class CComDebugObject : public Base
{
public:
    CComDebugObject(void * = NULL)
    {
#if DEBUG_CCOMOBJECT_CREATION
        DbgPrint("%S, this=%08p\n", __FUNCTION__, static_cast<Base*>(this));
#endif
        _pAtlModule->Lock();
    }

    virtual ~CComDebugObject()
    {
        this->FinalRelease();
        _pAtlModule->Unlock();
    }

    STDMETHOD_(ULONG, AddRef)()
    {
        int rc = this->InternalAddRef();
#if DEBUG_CCOMOBJECT_REFCOUNTING
        DbgPrint("%s, RefCount is now %d(--)! \n", __FUNCTION__, rc);
#endif
        return rc;
    }

    STDMETHOD_(ULONG, Release)()
    {
        int rc = this->InternalRelease();

#if DEBUG_CCOMOBJECT_REFCOUNTING
        DbgPrint("%s, RefCount is now %d(--)! \n", __FUNCTION__, rc);
#endif

        if (rc == 0)
        {
#if DEBUG_CCOMOBJECT_DESTRUCTION
            DbgPrint("%s, RefCount reached 0 Deleting!\n", __FUNCTION__);
#endif
            delete this;
        }
        return rc;
    }

    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObject)
    {
        return this->_InternalQueryInterface(iid, ppvObject);
    }

    static HRESULT WINAPI CreateInstance(CComDebugObject<Base> **pp)
    {
        CComDebugObject<Base>				*newInstance;
        HRESULT								hResult;

        ATLASSERT(pp != NULL);
        if (pp == NULL)
            return E_POINTER;

        hResult = E_OUTOFMEMORY;
        newInstance = NULL;
        ATLTRY(newInstance = new CComDebugObject<Base>());
        if (newInstance != NULL)
        {
            newInstance->SetVoid(NULL);
            newInstance->InternalFinalConstructAddRef();
            hResult = newInstance->_AtlInitialConstruct();
            if (SUCCEEDED(hResult))
                hResult = newInstance->FinalConstruct();
            if (SUCCEEDED(hResult))
                hResult = newInstance->_AtlFinalConstruct();
            newInstance->InternalFinalConstructRelease();
            if (hResult != S_OK)
            {
                delete newInstance;
                newInstance = NULL;
            }
        }
        *pp = newInstance;
        return hResult;
    }
};

#ifdef DEBUG_CCOMOBJECT
#   define _CComObject CComDebugObject
#else
#   define _CComObject CComObject
#endif

template<class T>
void ReleaseCComPtrExpectZero(CComPtr<T>& cptr, BOOL forceRelease = FALSE)
{
    if (cptr.p != NULL)
    {
        int nrc = cptr->Release();
        if (nrc > 0)
        {
            DbgPrint("WARNING: Unexpected RefCount > 0 (%d)!\n", nrc);
            if (forceRelease)
            {
                while (nrc > 0)
                {
                    nrc = cptr->Release();
                }
            }
        }
        cptr.Detach();
    }
}

template<class T, class R>
HRESULT inline ShellDebugObjectCreator(REFIID riid, R ** ppv)
{
    CComPtr<T>       obj;
    HRESULT          hResult;

    if (ppv == NULL)
        return E_POINTER;
    *ppv = NULL;
    ATLTRY(obj = new CComDebugObject<T>);
    if (obj.p == NULL)
        return E_OUTOFMEMORY;
    hResult = obj->QueryInterface(riid, reinterpret_cast<void **>(ppv));
    if (FAILED(hResult))
        return hResult;
    return S_OK;
}

template<class T>
HRESULT inline ShellObjectCreator(REFIID riid, void ** ppv)
{
    _CComObject<T> *pobj;
    HRESULT hResult;

    hResult = _CComObject<T>::CreateInstance(&pobj);
    if (FAILED(hResult))
        return hResult;

    pobj->AddRef(); /* CreateInstance returns object with 0 ref count */

    hResult = pobj->QueryInterface(riid, reinterpret_cast<void **>(ppv));

    pobj->Release(); /* In case of failure the object will be released */

    return hResult;
}

template<class T>
HRESULT inline ShellObjectCreatorInit(REFIID riid, void ** ppv)
{
    _CComObject<T> *pobj;
    HRESULT hResult;

    hResult = _CComObject<T>::CreateInstance(&pobj);
    if (FAILED(hResult))
        return hResult;

    pobj->AddRef(); /* CreateInstance returns object with 0 ref count */

    hResult = pobj->Initialize();

    if (SUCCEEDED(hResult))
        hResult = pobj->QueryInterface(riid, reinterpret_cast<void **>(ppv));

    pobj->Release(); /* In case of failure the object will be released */

    return hResult;
}

template<class T, class T1>
HRESULT inline ShellObjectCreatorInit(T1 initArg1, REFIID riid, void ** ppv)
{
    _CComObject<T> *pobj;
    HRESULT hResult;

    hResult = _CComObject<T>::CreateInstance(&pobj);
    if (FAILED(hResult))
        return hResult;

    pobj->AddRef(); /* CreateInstance returns object with 0 ref count */

    hResult = pobj->Initialize(initArg1);

    if (SUCCEEDED(hResult))
        hResult = pobj->QueryInterface(riid, reinterpret_cast<void **>(ppv));

    pobj->Release(); /* In case of failure the object will be released */

    return hResult;
}

template<class T, class T1, class T2>
HRESULT inline ShellObjectCreatorInit(T1 initArg1, T2 initArg2, REFIID riid, void ** ppv)
{
    _CComObject<T> *pobj;
    HRESULT hResult;

    hResult = _CComObject<T>::CreateInstance(&pobj);
    if (FAILED(hResult))
        return hResult;

    pobj->AddRef(); /* CreateInstance returns object with 0 ref count */

    hResult = pobj->Initialize(initArg1, initArg2);

    if (SUCCEEDED(hResult))
        hResult = pobj->QueryInterface(riid, reinterpret_cast<void **>(ppv));

    pobj->Release(); /* In case of failure the object will be released */

    return hResult;
}

template<class T, class T1, class T2, class T3>
HRESULT inline ShellObjectCreatorInit(T1 initArg1, T2 initArg2, T3 initArg3, REFIID riid, void ** ppv)
{
    _CComObject<T> *pobj;
    HRESULT hResult;

    hResult = _CComObject<T>::CreateInstance(&pobj);
    if (FAILED(hResult))
        return hResult;

    pobj->AddRef(); /* CreateInstance returns object with 0 ref count */

    hResult = pobj->Initialize(initArg1, initArg2, initArg3);

    if (SUCCEEDED(hResult))
        hResult = pobj->QueryInterface(riid, reinterpret_cast<void **>(ppv));

    pobj->Release(); /* In case of failure the object will be released */

    return hResult;
}

template<class T, class T1, class T2, class T3, class T4>
HRESULT inline ShellObjectCreatorInit(T1 initArg1, T2 initArg2, T3 initArg3, T4 initArg4, REFIID riid, void ** ppv)
{
    _CComObject<T> *pobj;
    HRESULT hResult;

    hResult = _CComObject<T>::CreateInstance(&pobj);
    if (FAILED(hResult))
        return hResult;

    pobj->AddRef(); /* CreateInstance returns object with 0 ref count */

    hResult = pobj->Initialize(initArg1, initArg2, initArg3, initArg4);

    if (SUCCEEDED(hResult))
        hResult = pobj->QueryInterface(riid, reinterpret_cast<void **>(ppv));

    pobj->Release(); /* In case of failure the object will be released */

    return hResult;
}

HRESULT inline SHSetStrRet(LPSTRRET pStrRet, LPCSTR pstrValue)
{
    pStrRet->uType = STRRET_CSTR;
    strcpy(pStrRet->cStr, pstrValue);
    return S_OK;
}

HRESULT inline SHSetStrRet(LPSTRRET pStrRet, LPCWSTR pwstrValue)
{
    ULONG cchr = wcslen(pwstrValue);
    LPWSTR buffer = static_cast<LPWSTR>(CoTaskMemAlloc((cchr + 1) * sizeof(WCHAR)));
    if (buffer == NULL)
        return E_OUTOFMEMORY;

    pStrRet->uType = STRRET_WSTR;
    pStrRet->pOleStr = buffer;
    wcscpy(buffer, pwstrValue);
    return S_OK;
}

HRESULT inline SHSetStrRet(LPSTRRET pStrRet, HINSTANCE hInstance, DWORD resId)
{
    WCHAR Buffer[MAX_PATH];

    if (!LoadStringW(hInstance, resId, Buffer, MAX_PATH))
        return E_FAIL;

    return SHSetStrRet(pStrRet, Buffer);
}

#endif /* __cplusplus */

#define S_LESSTHAN 0xffff
#define S_EQUAL S_OK
#define S_GREATERTHAN S_FALSE
#define MAKE_COMPARE_HRESULT(x) ((x)>0 ? S_GREATERTHAN : ((x)<0 ? S_LESSTHAN : S_EQUAL))

#endif /* __ROS_SHELL_UTILS_H */

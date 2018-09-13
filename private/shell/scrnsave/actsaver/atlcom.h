// This is a part of the ActiveX Template Library.
// Copyright (C) 1996 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// ActiveX Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// ActiveX Template Library product.

#ifndef __ATLCOM_H__
#define __ATLCOM_H__

#ifndef __cplusplus
    #error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLBASE_H__
    #error atlcom.h requires atlbase.h to be included first
#endif

#pragma pack(push, _ATL_PACKING)

#ifdef _DEBUG
#define RELEASE_AND_DESTROY() ULONG l = InternalRelease();if (l == 0) delete this; return l
#else
#define RELEASE_AND_DESTROY() if (InternalRelease() == 0) delete this; return 0
#endif

#define offsetofclass(base, derived) (PtrToUlong((base*)((derived*)8))-8)

#ifndef _SYS_GUID_OPERATORS_
inline BOOL InlineIsEqualGUID(REFGUID rguid1, REFGUID rguid2)
{
   return (
      ((PLONG) &rguid1)[0] == ((PLONG) &rguid2)[0] &&
      ((PLONG) &rguid1)[1] == ((PLONG) &rguid2)[1] &&
      ((PLONG) &rguid1)[2] == ((PLONG) &rguid2)[2] &&
      ((PLONG) &rguid1)[3] == ((PLONG) &rguid2)[3]);
}
#endif

inline BOOL InlineIsEqualUnknown(REFGUID rguid1)
{
   return (
      ((PLONG) &rguid1)[0] == 0 &&
      ((PLONG) &rguid1)[1] == 0 &&
#ifdef _MAC
      ((PLONG) &rguid1)[2] == 0xC0000000 &&
      ((PLONG) &rguid1)[3] == 0x00000046);
#else
      ((PLONG) &rguid1)[2] == 0x000000C0 &&
      ((PLONG) &rguid1)[3] == 0x46000000);
#endif
}

HRESULT WINAPI AtlReportError(const CLSID& clsid, LPCOLESTR lpszDesc,
    const IID& iid = GUID_NULL, HRESULT hRes = 0);
#ifndef OLE2ANSI
HRESULT WINAPI AtlReportError(const CLSID& clsid, LPCSTR lpszDesc,
    const IID& iid = GUID_NULL, HRESULT hRes = 0);
#endif
HRESULT WINAPI AtlReportError(const CLSID& clsid, UINT nID,
    const IID& iid = GUID_NULL, HRESULT hRes = 0);


/////////////////////////////////////////////////////////////////////////////
// CComBSTR
class CComBSTR
{
public:
    BSTR m_str;
    CComBSTR()
    {
        m_str = NULL;
    }
    CComBSTR(int nSize, LPCOLESTR sz = NULL)
    {
        m_str = ::SysAllocStringLen(sz, nSize);
    }
    CComBSTR(LPCOLESTR pSrc)
    {
        m_str = ::SysAllocString(pSrc);
    }
    CComBSTR(const CComBSTR& src)
    {
        m_str = src.Copy();
    }
    CComBSTR& operator=(const CComBSTR& src);
    CComBSTR& operator=(LPCOLESTR pSrc);
#ifndef OLE2ANSI
    CComBSTR(LPCSTR pSrc);
    CComBSTR& operator=(LPCSTR pSrc);
    CComBSTR(int nSize, LPCSTR sz = NULL);
#endif
    ~CComBSTR()
    {
        ::SysFreeString(m_str);
    }
    operator BSTR() const
    {
        return m_str;
    }
    BSTR* operator&()
    {
        return &m_str;
    }
    BSTR Copy() const
    {
        return ::SysAllocStringLen(m_str, ::SysStringLen(m_str));
    }
    void Attach(BSTR src)
    {
        if (m_str != src)
        {
            ::SysFreeString(m_str);
            m_str = src;
        }
    }
    BSTR Detach()
    {
        BSTR s = m_str;
        m_str = NULL;
        return s;
    }
    void Empty()
    {
        ::SysFreeString(m_str);
        m_str = NULL;
    }
    BOOL operator!()
    {
        return (m_str == NULL) ? TRUE : FALSE;
    }
};

/////////////////////////////////////////////////////////////////////////////
// CComVariant

class CComVariant : public tagVARIANT
{
public:
    CComVariant() {VariantInit(this);}
    ~CComVariant() {VariantClear(this);}
    CComVariant(VARIANT& var)
    {
        VariantInit(this);
        VariantCopy(this, &var);
    }
    CComVariant(LPCOLESTR lpsz)
    {
        VariantInit(this);
        vt = VT_BSTR;
        bstrVal = ::SysAllocString(lpsz);
    }
#ifndef OLE2ANSI
    CComVariant(LPCSTR lpsz);
#endif
    CComVariant(const CComVariant& var)
    {
        VariantInit(this);
        VariantCopy(this, (VARIANT*)&var);
    }
    CComVariant& operator=(const CComVariant& var)
    {
        VariantCopy(this, (VARIANT*)&var);
        return *this;
    }
    CComVariant& operator=(VARIANT& var)
    {
        VariantCopy(this, &var);
        return *this;
    }
    CComVariant& operator=(LPCOLESTR lpsz)
    {
        VariantClear(this);
        vt = VT_BSTR;
        bstrVal = ::SysAllocString(lpsz);
        return *this;
    }
#ifndef OLE2ANSI
    CComVariant& operator=(LPCSTR lpsz);
#endif
};

/////////////////////////////////////////////////////////////////////////////
// Connection point helpers
//

HRESULT AtlAdvise(IUnknown* pUnkCP, IUnknown* pUnk, const IID& iid, LPDWORD pdw);
HRESULT AtlUnadvise(IUnknown* pUnkCP, const IID& iid, DWORD dw);

/////////////////////////////////////////////////////////////////////////////
// IRegister
//

EXTERN_C const IID IID_IRegister;
EXTERN_C const CLSID CLSID_Register;

interface IRegister : public IDispatch
{
    public:
    virtual HRESULT STDMETHODCALLTYPE AddReplacement(
        /* [in] */ BSTR key,
        /* [in] */ BSTR item) = 0;

    virtual HRESULT STDMETHODCALLTYPE ClearReplacements( void) = 0;

    virtual HRESULT STDMETHODCALLTYPE ResourceRegister(
        /* [in] */ BSTR fileName,
        /* [in] */ VARIANT ID,
        /* [in] */ VARIANT type) = 0;

    virtual HRESULT STDMETHODCALLTYPE ResourceUnregister(
        /* [in] */ BSTR fileName,
        /* [in] */ VARIANT ID,
        /* [in] */ VARIANT type) = 0;

    virtual HRESULT STDMETHODCALLTYPE FileRegister(
        /* [in] */ BSTR fileName) = 0;

    virtual HRESULT STDMETHODCALLTYPE FileUnregister(
        /* [in] */ BSTR fileName) = 0;

    virtual HRESULT STDMETHODCALLTYPE StringRegister(
        /* [in] */ BSTR data) = 0;

    virtual HRESULT STDMETHODCALLTYPE StringUnregister(
        /* [in] */ BSTR data) = 0;

    virtual HRESULT STDMETHODCALLTYPE AddKey(
        /* [in] */ BSTR keyName) = 0;

    virtual HRESULT STDMETHODCALLTYPE DeleteKey(
        /* [in] */ BSTR keyName) = 0;

    virtual HRESULT STDMETHODCALLTYPE AddKeyValue(
        /* [in] */ BSTR keyName,
        /* [in] */ BSTR valueName,
        /* [in] */ VARIANT value) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetKeyValue(
        /* [in] */ BSTR keyName,
        /* [in] */ BSTR valueName,
        /* [retval][out] */ VARIANT __RPC_FAR *value) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetKeyValue(
        /* [in] */ BSTR keyName,
        /* [in] */ BSTR valueName,
        /* [in] */ VARIANT value) = 0;
};

/////////////////////////////////////////////////////////////////////////////
// Smart OLE pointers provide automatic AddRef/Release
// CComPtr<IFoo> p;

IUnknown* WINAPI _AtlComPtrAssign(IUnknown** pp, IUnknown* lp);
IUnknown* WINAPI _AtlComQIPtrAssign(IUnknown** pp, IUnknown* lp, REFIID riid);

template <class T>
class CComPtr
{
public:
    typedef T _PtrClass;
    CComPtr() {p=NULL;}
    CComPtr(T* lp)
    {
        if ((p = lp) != NULL)
            p->AddRef();
    }
    CComPtr(const CComPtr<T>& lp)
    {
        if ((p = lp.p) != NULL)
            p->AddRef();
    }
    ~CComPtr() {if (p) p->Release();}
    void Release() {if (p) p->Release(); p=NULL;}
    operator T*() {return (T*)p;}
    T& operator*() { ASSERT(p != NULL); return *p; }
    T** operator&() { ASSERT(p == NULL); return &p; }
    T* operator->() { ASSERT(p != NULL); return p; }
    T* operator=(T* lp){return (T*)_AtlComPtrAssign((IUnknown**)&p, lp);}
    T* operator=(const CComPtr<T>& lp)
    {
        return (T*)_AtlComPtrAssign((IUnknown**)&p, lp.p);
    }
    BOOL operator!(){return (p == NULL) ? TRUE : FALSE;}
    T* p;
};

//Note: CComQIPtr<IUnknown, &IID_IUnknown> is not meaningful
//      Use CComPtr<IUnknown>
template <class T, const IID* piid>
class CComQIPtr
{
public:
    typedef T _PtrClass;
    CComQIPtr() {p=NULL;}
    CComQIPtr(T* lp)
    {
        if ((p = lp) != NULL)
            p->AddRef();
    }
    CComQIPtr(const CComQIPtr<T,piid>& lp)
    {
        if ((p = lp.p) != NULL)
            p->AddRef();
    }
    // If you get an error that this member is already defined, you are probably
    // using a CComQIPtr<IUnknown, &IID_IUnknown>.  This is not necessary.
    // Use CComPtr<IUnknown>
    CComQIPtr(IUnknown* lp)
    {
        p=NULL;
        if (lp != NULL)
            lp->QueryInterface(*piid, (void **)&p);
    }
    ~CComQIPtr() {if (p) p->Release();}
    void Release() {if (p) p->Release(); p=NULL;}
    operator T*() {return p;}
    T& operator*() {ASSERT(p!=NULL); return *p; }
    T** operator&() {ASSERT(p==NULL); return &p; }
    T* operator->() {ASSERT(p!=NULL); return p; }
    T* operator=(T* lp){return (T*)_AtlComPtrAssign((IUnknown**)&p, lp);}
    T* operator=(const CComQIPtr<T,piid>& lp)
    {
        return (T*)_AtlComPtrAssign((IUnknown**)&p, lp.p);
    }
    T* operator=(IUnknown* lp)
    {
        return (T*)_AtlComQIPtrAssign((IUnknown**)&p, lp, *piid);
    }
    BOOL operator!(){return (p == NULL) ? TRUE : FALSE;}
    T* p;
};

void WINAPI AtlFreeMarshalStream(IStream* pStream);
HRESULT WINAPI AtlMarshalPtrInProc(IUnknown* pUnk, const IID& iid, IStream** ppStream);
HRESULT WINAPI AtlUnmarshalPtr(IStream* pStream, const IID& iid, IUnknown** ppUnk);

/////////////////////////////////////////////////////////////////////////////
// COM Objects

#define DECLARE_PROTECT_FINAL_CONSTRUCT()\
    void InternalFinalConstructAddRef() {InternalAddRef();}\
    void InternalFinalConstructRelease() {InternalRelease();}

typedef HRESULT (WINAPI _ATL_CREATORFUNC)(void* pv, REFIID riid, LPVOID* ppv);
typedef HRESULT (WINAPI _ATL_CREATORARGFUNC)(void* pv, REFIID riid, LPVOID* ppv, DWORD dw);

template <class T1>
class CComCreator
{
public:
    static HRESULT WINAPI CreateInstance(void* pv, REFIID riid, LPVOID* ppv)
    {
        ASSERT(*ppv == NULL);
        HRESULT hRes = E_OUTOFMEMORY;
        T1* p = NULL;
        ATLTRY(p = new T1(pv))
        if (p != NULL)
        {
            p->SetVoid(pv);
            p->InternalFinalConstructAddRef();
            hRes = p->FinalConstruct();
            p->InternalFinalConstructRelease();
            if (hRes == S_OK)
                hRes = p->QueryInterface(riid, ppv);
            if (hRes != S_OK)
                delete p;
        }
        return hRes;
    }
};

template <class T1>
class CComSingleCreator
{
public:
    static T1 * m_pThis;

public:
    static HRESULT WINAPI CreateInstance(void* pv, REFIID riid, LPVOID* ppv)
    {
        HRESULT hrResult = S_OK;

        ASSERT(*ppv == NULL);

        if (m_pThis == NULL)
        {
            ATLTRY(m_pThis = new T1);

            if (m_pThis != NULL)
            {
                m_pThis->SetVoid(pv);
                m_pThis->InternalFinalConstructAddRef();
                hrResult = m_pThis->FinalConstruct();
                m_pThis->InternalFinalConstructRelease();

                if  (
                    FAILED(hrResult)
                    ||
                    (FAILED(hrResult = m_pThis->QueryInterface(riid, ppv)))
                    )
                {
                    delete m_pThis;
                    m_pThis = NULL;
                }
            }
            else
                hrResult = E_OUTOFMEMORY;
        }
        else
            hrResult = m_pThis->QueryInterface(riid, ppv);

        return hrResult;
    }
};

template <HRESULT hr>
class CComFailCreator
{
public:
    static HRESULT WINAPI CreateInstance(void*, REFIID, LPVOID*)
    {
        return hr;
    }
};

template <class T1, class T2>
class CComCreator2
{
public:
    static HRESULT WINAPI CreateInstance(void* pv, REFIID riid, LPVOID* ppv)
    {
        ASSERT(*ppv == NULL);
        HRESULT hRes = E_OUTOFMEMORY;
        if (pv == NULL)
            hRes = T1::CreateInstance(NULL, riid, ppv);
        else
            hRes = T2::CreateInstance(pv, riid, ppv);
        return hRes;
    }
};

#define DECLARE_NOT_AGGREGATABLE(x) public:\
    typedef CComCreator2< CComCreator< CComObject<x> >, CComFailCreator<CLASS_E_NOAGGREGATION> > _CreatorClass;
#define DECLARE_AGGREGATABLE(x) public:\
    typedef CComCreator2< CComCreator< CComObject<x> >, CComCreator< CComAggObject<x> > > _CreatorClass;
#define DECLARE_ONLY_AGGREGATABLE(x) public:\
    typedef CComCreator2< CComFailCreator<E_FAIL>, CComCreator< CComAggObject<x> > > _CreatorClass;

#define DECLARE_SINGLE_NOT_AGGREGATABLE(x) public:\
    typedef CComCreator2< CComSingleCreator< CComObject<x> >, CComFailCreator<CLASS_E_NOAGGREGATION> > _CreatorClass;

#define IMPLEMENT_SINGLE_NOT_AGGREGATABLE(x) CComObject<x> * CComSingleCreator< CComObject<x> >::m_pThis = NULL;
#define CLEANUP_SINGLE_NOT_AGGRAGATABLE(x) CComSingleCreator< CComObject<x> >::m_pThis = NULL;

struct _ATL_INTMAP_ENTRY
{
    const IID* piid;       // the interface id (IID)
    DWORD dw;
    _ATL_CREATORARGFUNC* pFunc; //NULL:end, 1:offset, n:ptr
};

struct _ATL_CREATORDATA
{
    _ATL_CREATORFUNC* pFunc;
};

template <class Creator>
class _CComCreatorData
{
public:
    static _ATL_CREATORDATA data;
};

template <class Creator>
_ATL_CREATORDATA _CComCreatorData<Creator>::data = {Creator::CreateInstance};

struct _ATL_CACHEDATA
{
    DWORD dwOffsetVar;
    DWORD dwOffsetCS;
    _ATL_CREATORFUNC* pFunc;
};

template <class Creator, DWORD dwVar, DWORD dwCS>
class _CComCacheData
{
public:
    static _ATL_CACHEDATA data;
};

template <class Creator, DWORD dwVar, DWORD dwCS>
_ATL_CACHEDATA _CComCacheData<Creator, dwVar, dwCS>::data = {dwVar, dwCS, Creator::CreateInstance};

struct _ATL_CHAINDATA
{
    DWORD dwOffset;
    const _ATL_INTMAP_ENTRY* (WINAPI *pFunc)();
};

template <class base, class derived>
class _CComChainData
{
public:
    static _ATL_CHAINDATA data;
};

template <class base, class derived>
_ATL_CHAINDATA _CComChainData<base, derived>::data =
    {offsetofclass(base, derived), base::_GetEntries};

template <class T, const CLSID* pclsid>
class CComAggregateCreator
{
public:
    static HRESULT WINAPI CreateInstance(void* pv, REFIID/*riid*/, LPVOID* ppv)
    {
        ASSERT(*ppv == NULL);
        ASSERT(pv != NULL);
        T* p = (T*) pv;
        // Add the following line to your object if you get a message about
        // GetControllingUnknown() being undefined
        // DECLARE_GET_CONTROLLING_UNKNOWN()
        return CoCreateInstance(*pclsid, p->GetControllingUnknown(), CLSCTX_INPROC, IID_IUnknown, ppv);
    }
};

template <class T>
class CComCachedTearOffCreator
{
public:
    static HRESULT WINAPI CreateInstance(void* pv, REFIID riid, LPVOID* ppv)
    {
        ASSERT(pv != NULL);
        T::_OwnerClass* pOwner = (T::_OwnerClass*)pv;
        ASSERT(*ppv == NULL);
        HRESULT hRes = E_OUTOFMEMORY;
        CComCachedTearOffObject<T>* p = NULL;
        // Add the following line to your object if you get a message about
        // GetControllingUnknown() being undefined
        // DECLARE_GET_CONTROLLING_UNKNOWN()
        ATLTRY(p = new CComCachedTearOffObject<T>(pOwner->GetControllingUnknown(), pOwner))
        if (p != NULL)
        {
            p->SetVoid(pv);
            p->InternalFinalConstructAddRef();
            hRes = p->FinalConstruct();
            p->InternalFinalConstructRelease();
            if (hRes == S_OK)
                hRes = p->QueryInterface(riid, ppv);
            if (hRes != S_OK)
                delete p;
        }
        return hRes;
    }
};

#ifdef _ATL_DEBUG_QI
#define DEBUG_QI_ENTRY(x) \
        {NULL, \
        (DWORD)_T(#x), \
        (_ATL_CREATORARGFUNC*)0},
#else
#define DEBUG_QI_ENTRY(x)
#endif //_ATL_DEBUG_QI

//If you get a message that FinalConstruct is ambiguous then you need to
// override it in your class and call each base class' version of this
#define BEGIN_COM_MAP(x) public:\
    typedef x _ComMapClass;\
    IUnknown* GetUnknown() { ASSERT(_GetEntries()->pFunc == (_ATL_CREATORARGFUNC*)1); \
            return (IUnknown*)((INT_PTR)this+_GetEntries()->dw); } \
    HRESULT _InternalQueryInterface(REFIID iid, void** ppvObject)\
    {return InternalQueryInterface(this, _GetEntries(), iid, ppvObject);}\
    const static _ATL_INTMAP_ENTRY* WINAPI _GetEntries() {\
    static const _ATL_INTMAP_ENTRY _entries[] = { DEBUG_QI_ENTRY(x)

#define DECLARE_GET_CONTROLLING_UNKNOWN() public:\
    virtual IUnknown* GetControllingUnknown() {return GetUnknown();}

#define COM_INTERFACE_ENTRY(x)\
    {&IID_##x, \
    offsetofclass(x, _ComMapClass), \
    (_ATL_CREATORARGFUNC*)1},

#define COM_INTERFACE_ENTRY_IID(iid, x)\
    {&iid,\
    offsetofclass(x, _ComMapClass),\
    (_ATL_CREATORARGFUNC*)1},

#define COM_INTERFACE_ENTRY2(x, x2)\
    {&IID_##x,\
    PtrToUlong((x*)(x2*)((_ComMapClass*)8))-8,\
    (_ATL_CREATORARGFUNC*)1},

#define COM_INTERFACE_ENTRY2_IID(iid, x, x2)\
    {&iid,\
    PtrToUlong((x*)(x2*)((_ComMapClass*)8))-8,\
    (_ATL_CREATORARGFUNC*)1},

#define COM_INTERFACE_ENTRY_FUNC(iid, dw, func)\
    {&iid, \
    dw, \
    func},

#define COM_INTERFACE_ENTRY_FUNC_BLIND(dw, func)\
    {NULL, \
    dw, \
    func},

#define COM_INTERFACE_ENTRY_TEAR_OFF(iid, x)\
    {&iid,\
    (DWORD)&_CComCreatorData<\
        CComCreator<CComTearOffObject<x, _ComMapClass> >\
        >::data,\
    _Creator},

#define COM_INTERFACE_ENTRY_CACHED_TEAR_OFF(iid, x, punk, cs)\
    {&iid,\
    (DWORD)&_CComCacheData<\
        CComCachedTearOffCreator< x >,\
        (DWORD)offsetof(_ComMapClass, punk),\
        (DWORD)offsetof(_ComMapClass, cs)\
        >::data,\
    _Cache},

#define COM_INTERFACE_ENTRY_AGGREGATE(iid, punk)\
    {&iid,\
    (DWORD)offsetof(_ComMapClass, punk),\
    _Delegate},

#define COM_INTERFACE_ENTRY_AGGREGATE_BLIND(punk)\
    {NULL,\
    (DWORD)offsetof(_ComMapClass, punk),\
    _Delegate},

#define COM_INTERFACE_ENTRY_AUTOAGGREGATE(iid, punk, clsid, cs)\
    {&iid,\
    (DWORD)&_CComCacheData<\
        CComAggregateCreator<_ComMapClass, &clsid>,\
        (DWORD)offsetof(_ComMapClass, punk),\
        (DWORD)offsetof(_ComMapClass, cs)\
        >::data,\
    _Cache},

#define COM_INTERFACE_ENTRY_AUTOAGGREGATE_BLIND(punk, clsid, cs)\
    {NULL,\
    (DWORD)&_CComCacheData<\
        CComAggregateCreator<_ComMapClass, &clsid>,\
        (DWORD)offsetof(_ComMapClass, punk),\
        (DWORD)offsetof(_ComMapClass, cs)\
        >::data,\
    _Cache},

#define COM_INTERFACE_ENTRY_CHAIN(classname)\
    {NULL,\
    (DWORD)&_CComChainData<classname, _ComMapClass>::data,\
    _Chain},

#ifdef _ATL_DEBUG_QI
#define END_COM_MAP()   {NULL, 0, 0}};\
    return &_entries[1];}
#else
#define END_COM_MAP()   {NULL, 0, 0}};\
    return _entries;}
#endif // _ATL_DEBUG_QI

struct _ATL_OBJMAP_ENTRY
{
    const CLSID* pclsid;
    HRESULT (WINAPI *pfnUpdateRegistry)(BOOL bRegister);
    _ATL_CREATORFUNC* pfnGetClassObject;
    _ATL_CREATORFUNC* pfnCreateInstance;
    HRESULT WINAPI RevokeClassObject()
    {
        return CoRevokeClassObject(dwRegister);
    }
    HRESULT WINAPI RegisterClassObject(DWORD dwClsContext, DWORD dwFlags)
    {
        CComPtr<IUnknown> p;
        HRESULT hRes = pfnGetClassObject(pfnCreateInstance, IID_IUnknown, (LPVOID*) &p);
        if (SUCCEEDED(hRes))
            hRes = CoRegisterClassObject(*pclsid, p, dwClsContext, dwFlags, &dwRegister);
        return hRes;
    }
    IUnknown* pCF;
    DWORD dwRegister;
};


#define BEGIN_OBJECT_MAP(x) static _ATL_OBJMAP_ENTRY x[] = {
#define END_OBJECT_MAP()   {NULL, NULL, NULL, NULL}};
#define OBJECT_ENTRY(clsid, class) {&clsid, &class::UpdateRegistry, &class::_ClassFactoryCreatorClass::CreateInstance, &class::_CreatorClass::CreateInstance, NULL, 0},

#define THREADFLAGS_APARTMENT 0x1
#define THREADFLAGS_BOTH 0x2
#define AUTPRXFLAG 0x4

// the functions in this class don't need to be virtual because
// they are called from CComObject
class CComObjectRoot
{
public:
    typedef CComObjectThreadModel _ThreadModel;
    CComObjectRoot()
    {
        m_dwRef = 0L;
    }
    HRESULT FinalConstruct()
    {
        return S_OK;
    }
    void FinalRelease() {}

    ULONG InternalAddRef()
    {
        ASSERT(m_dwRef != -1L);
        return _ThreadModel::Increment(&m_dwRef);
    }
    ULONG InternalRelease()
    {
        return _ThreadModel::Decrement(&m_dwRef);
    }
    static HRESULT WINAPI InternalQueryInterface(void* pThis,
        const _ATL_INTMAP_ENTRY* entries, REFIID iid, void** ppvObject);
//Outer funcs
    ULONG OuterAddRef() {return m_pOuterUnknown->AddRef();}
    ULONG OuterRelease() {return m_pOuterUnknown->Release();}
    HRESULT OuterQueryInterface(REFIID iid, void ** ppvObject)
    {return m_pOuterUnknown->QueryInterface(iid, ppvObject);}

    void SetVoid(void*) {}
    void InternalFinalConstructAddRef() {}
    void InternalFinalConstructRelease() {ASSERT(m_dwRef == 0);}
    // If this assert occurs, your object has probably been deleted
    // Try using DECLARE_PROTECT_FINAL_CONSTRUCT()

#ifdef _ATL_DEBUG_QI
    static HRESULT DumpIID(REFIID iid, LPCTSTR pszClassName, HRESULT hr);
#endif // _ATL_DEBUG_QI

    static HRESULT WINAPI _Cache(void* pv, REFIID iid, void** ppvObject, DWORD dw);
    static HRESULT WINAPI _Creator(void* pv, REFIID iid, void** ppvObject, DWORD dw);
    static HRESULT WINAPI _Delegate(void* pv, REFIID iid, void** ppvObject, DWORD dw);
    static HRESULT WINAPI _Chain(void* pv, REFIID iid, void** ppvObject, DWORD dw);

    union
    {
        long m_dwRef;
        IUnknown* m_pOuterUnknown;
    };
};

#if defined(_WINDLL) | defined(_USRDLL)
#define DECLARE_CLASSFACTORY_EX(cf) typedef CComCreator< CComObjectCached< cf > > _ClassFactoryCreatorClass;
#else
// don't let class factory refcount influence lock count
#define DECLARE_CLASSFACTORY_EX(cf) typedef CComCreator< CComObjectNoLock< cf > > _ClassFactoryCreatorClass;
#endif
#define DECLARE_CLASSFACTORY() DECLARE_CLASSFACTORY_EX(CComClassFactory)
#define DECLARE_CLASSFACTORY2(lic) DECLARE_CLASSFACTORY_EX(CComClassFactory2<lic>)


#define DECLARE_NO_REGISTRY()\
    static HRESULT WINAPI UpdateRegistry(BOOL /*bRegister*/)\
    {return S_OK;}

#define DECLARE_REGISTRY(class, pid, vpid, nid, flags)\
    static HRESULT WINAPI UpdateRegistry(BOOL bRegister)\
    {\
        return _pModule->UpdateRegistryClass(GetObjectCLSID(), pid, vpid, nid,\
            flags, bRegister);\
    }

#define DECLARE_REGISTRY_RESOURCE(x)\
    static HRESULT WINAPI UpdateRegistry(BOOL bRegister)\
    {\
    return _pModule->UpdateRegistryFromResource(_T(#x), bRegister);\
    }

#define DECLARE_REGISTRY_RESOURCEID(x)\
    static HRESULT WINAPI UpdateRegistry(BOOL bRegister)\
    {\
    return _pModule->UpdateRegistryFromResource(x, bRegister);\
    }

// Statically linking to Registry Ponent
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Must have included atl\ponent\register\static\statreg.cpp and statreg.h
#ifdef _ATL_STATIC_REGISTRY
#define DECLARE_STATIC_REGISTRY_RESOURCE(x)\
    static HRESULT WINAPI UpdateRegistry(BOOL bRegister)\
    {\
    return _pModule->UpdateRegistryFromResourceS(_T(#x), bRegister);\
    }

#define DECLARE_STATIC_REGISTRY_RESOURCEID(x)\
    static HRESULT WINAPI UpdateRegistry(BOOL bRegister)\
    {\
    return _pModule->UpdateRegistryFromResourceS(x, bRegister);\
    }
#endif //_ATL_STATIC_REGISTRY

template<class Base> class CComObject; // fwd decl

template <class Owner>
class CComTearOffObjectBase : public CComObjectRoot
{
public:
    typedef Owner _OwnerClass;
    CComObject<Owner>* m_pOwner;
    CComTearOffObjectBase() {m_pOwner = NULL;}
};

//Base is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object
template <class Base>
class CComObject : public Base
{
public:
    typedef Base _BaseClass;
    CComObject(void* = NULL)
    {
        _pModule->Lock();
    }
    // Set refcount to 1 to protect destruction
    ~CComObject(){m_dwRef = 1L; FinalRelease(); _pModule->Unlock();}
    //If InternalAddRef or InteralRelease is undefined then your class
    //doesn't derive from CComObjectRoot
    STDMETHOD_(ULONG, AddRef)() {return InternalAddRef();}
    STDMETHOD_(ULONG, Release)()
    {
        RELEASE_AND_DESTROY();
    }
    //if _InternalQueryInterface is undefined then you forgot BEGIN_COM_MAP
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
    {return _InternalQueryInterface(iid, ppvObject);}
    static HRESULT WINAPI CreateInstance(CComObject<Base>** pp);
};

template <class Base>
HRESULT WINAPI CComObject<Base>::CreateInstance(CComObject<Base>** pp)
{
    ASSERT(pp != NULL);
    HRESULT hRes = E_OUTOFMEMORY;
    CComObject<Base>* p = NULL;
    ATLTRY(p = new CComObject<Base>())
    if (p != NULL)
    {
        p->SetVoid(NULL);
        p->InternalFinalConstructAddRef();
        hRes = p->FinalConstruct();
        p->InternalFinalConstructRelease();
        if (hRes != S_OK)
        {
            delete p;
            p = NULL;
        }
    }
    *pp = p;
    return hRes;
}

//Base is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object
// CComObjectCached is used primarily for class factories in DLL's
// but it is useful anytime you want to cache an object
template <class Base>
class CComObjectCached : public Base
{
public:
    typedef Base _BaseClass;
    CComObjectCached(void* = NULL){}
    // Set refcount to 1 to protect destruction
    ~CComObjectCached(){m_dwRef = 1L; FinalRelease();}
    //If InternalAddRef or InteralRelease is undefined then your class
    //doesn't derive from CComObjectRoot
    STDMETHOD_(ULONG, AddRef)()
    {
        m_csCached.Lock();
        ULONG l = InternalAddRef();
        if (m_dwRef == 2)
            _pModule->Lock();
        m_csCached.Unlock();
        return l;
    }
    STDMETHOD_(ULONG, Release)()
    {
        m_csCached.Lock();
        InternalRelease();
        ULONG l = m_dwRef;
        m_csCached.Unlock();
        if (l == 0)
            delete this;
        else if (l == 1)
            _pModule->Unlock();
        return l;
    }
    //if _InternalQueryInterface is undefined then you forgot BEGIN_COM_MAP
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
    {return _InternalQueryInterface(iid, ppvObject);}
    static HRESULT WINAPI CreateInstance(CComObject<Base>** pp);
    CComGlobalsThreadModel::AutoCriticalSection m_csCached;
};

//Base is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object
template <class Base>
class CComObjectNoLock : public Base
{
public:
    typedef Base _BaseClass;
    CComObjectNoLock(void* = NULL){}
    // Set refcount to 1 to protect destruction
    ~CComObjectNoLock() {m_dwRef = 1L; FinalRelease();}

    //If InternalAddRef or InteralRelease is undefined then your class
    //doesn't derive from CComObjectRoot
    STDMETHOD_(ULONG, AddRef)() {return InternalAddRef();}
    STDMETHOD_(ULONG, Release)()
    {
        RELEASE_AND_DESTROY();
    }
    //if _InternalQueryInterface is undefined then you forgot BEGIN_COM_MAP
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
    {return _InternalQueryInterface(iid, ppvObject);}
};

// It is possible for Base not to derive from CComObjectRoot
// However, you will need to provide FinalConstruct and InternalQueryInterface
template <class Base>
class CComObjectGlobal : public Base
{
public:
    typedef Base _BaseClass;
    CComObjectGlobal(void* = NULL){m_hResFinalConstruct = FinalConstruct();}
    ~CComObjectGlobal() {FinalRelease();}

    STDMETHOD_(ULONG, AddRef)() {return _pModule->Lock();}
    STDMETHOD_(ULONG, Release)(){return _pModule->Unlock();}
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
    {return _InternalQueryInterface(iid, ppvObject);}
    HRESULT m_hResFinalConstruct;
};

// It is possible for Base not to derive from CComObjectRoot
// However, you will need to provide FinalConstruct and InternalQueryInterface
template <class Base>
class CComObjectStack : public Base
{
public:
    typedef Base _BaseClass;
    CComObjectStack(void* = NULL){m_hResFinalConstruct = FinalConstruct();}
    ~CComObjectStack() {FinalRelease();}

    STDMETHOD_(ULONG, AddRef)() {ASSERT(FALSE);return 0;}
    STDMETHOD_(ULONG, Release)(){ASSERT(FALSE);return 0;}
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
    {ASSERT(FALSE);return E_NOINTERFACE;}
    HRESULT m_hResFinalConstruct;
};

//Base is the user's class that derives from CComTearOffObjectBase and whatever
//interfaces the user wants to support on the object
//Owner is the class of object that Base is a tear-off for
template <class Base, class Owner>
class CComTearOffObject : public Base
{
public:
    typedef Base _BaseClass;
    CComTearOffObject(void* p)
    {
        m_pOwner = reinterpret_cast<CComObject<Owner>*>(p);
        m_pOwner->AddRef();
    }
    // Set refcount to 1 to protect destruction
    ~CComTearOffObject(){m_dwRef = 1L; FinalRelease(); m_pOwner->Release();}

    //If InternalAddRef or InteralRelease is undefined then your class
    //doesn't derive from CComObjectRoot
    STDMETHOD_(ULONG, AddRef)() {return InternalAddRef();}
    STDMETHOD_(ULONG, Release)()
    {
        RELEASE_AND_DESTROY();
    }
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
    {
        if (InlineIsEqualUnknown(iid) ||
            FAILED(_InternalQueryInterface(iid, ppvObject)))
        {
            return m_pOwner->QueryInterface(iid, ppvObject);
        }
        return S_OK;
    }
};

template <class Base> //Base must be derived from CComObjectRoot
class CComContainedObject : public Base
{
public:
    typedef Base _BaseClass;
    CComContainedObject(void* pv) {m_pOuterUnknown = (IUnknown*)pv;}

    STDMETHOD_(ULONG, AddRef)() {return OuterAddRef();}
    STDMETHOD_(ULONG, Release)() {return OuterRelease();}
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
    {return OuterQueryInterface(iid, ppvObject);}
    //GetControllingUnknown may be virtual if the Base class has declared
    //DECLARE_GET_CONTROLLING_UNKNOWN()
    IUnknown* GetControllingUnknown() {return m_pOuterUnknown;}
};

//contained is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object
template <class contained>
class CComAggObject : public IUnknown, public CComObjectRoot
{
public:
    typedef contained _BaseClass;
    CComAggObject(void* pv) : m_contained(pv)
    {_pModule->Lock();}
    //If you get a message that this call is ambiguous then you need to
    // override it in your class and call each base class' version of this
    HRESULT FinalConstruct() {CComObjectRoot::FinalConstruct(); return m_contained.FinalConstruct();}
    void FinalRelease() {CComObjectRoot::FinalRelease(); m_contained.FinalRelease();}
    // Set refcount to 1 to protect destruction
    ~CComAggObject(){m_dwRef = 1L; FinalRelease(); _pModule->Unlock();}

    STDMETHOD_(ULONG, AddRef)() {return InternalAddRef();}
    STDMETHOD_(ULONG, Release)()
    {
        RELEASE_AND_DESTROY();
    }
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
    {
        HRESULT hRes = S_OK;
        if (InlineIsEqualUnknown(iid))
        {
            *ppvObject = (void*)(IUnknown*)this;
            AddRef();
        }
        else
            hRes = m_contained._InternalQueryInterface(iid, ppvObject);
        return hRes;
    }
    CComContainedObject<contained> m_contained;
};

template <class contained>
class CComCachedTearOffObject : public IUnknown, public CComObjectRoot
{
public:
    typedef contained _BaseClass;
    CComCachedTearOffObject(void* pv, contained::_OwnerClass* pOwner) : m_contained(pv)
    {
        ASSERT(m_contained.m_pOwner == NULL);
        m_contained.m_pOwner = reinterpret_cast<CComObject<contained::_OwnerClass>*>(pOwner);
    }
    //If you get a message that this call is ambiguous then you need to
    // override it in your class and call each base class' version of this
    HRESULT FinalConstruct() {CComObjectRoot::FinalConstruct(); return m_contained.FinalConstruct();}
    void FinalRelease() {CComObjectRoot::FinalRelease(); m_contained.FinalRelease();}
    // Set refcount to 1 to protect destruction
    ~CComCachedTearOffObject(){m_dwRef = 1L; FinalRelease();}

    STDMETHOD_(ULONG, AddRef)() {return InternalAddRef();}
    STDMETHOD_(ULONG, Release)()
    {
        RELEASE_AND_DESTROY();
    }
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
    {
        HRESULT hRes = S_OK;
        if (InlineIsEqualUnknown(iid))
        {
            *ppvObject = (void*)(IUnknown*)this;
            AddRef();
        }
        else
            hRes = m_contained._InternalQueryInterface(iid, ppvObject);
        return hRes;
    }
    CComContainedObject<contained> m_contained;
};

class CComClassFactory : public IClassFactory, public CComObjectRoot
{
public:
    // This typedef is because class factories are globally held
    typedef CComGlobalsThreadModel _ThreadModel;
BEGIN_COM_MAP(CComClassFactory)
    COM_INTERFACE_ENTRY(IClassFactory)
END_COM_MAP()
    // IClassFactory
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj);
    STDMETHOD(LockServer)(BOOL fLock);
    // helper
    void SetVoid(void* pv)
    {
        m_pfnCreateInstance = (_ATL_CREATORFUNC*)pv;
    }
    _ATL_CREATORFUNC* m_pfnCreateInstance;
};

class CComClassFactory2Base : public IClassFactory2, public CComObjectRoot
{
public:
    // This typedef is because class factories are globally held
    typedef CComGlobalsThreadModel _ThreadModel;
BEGIN_COM_MAP(CComClassFactory2Base)
    COM_INTERFACE_ENTRY(IClassFactory)
    COM_INTERFACE_ENTRY(IClassFactory2)
END_COM_MAP()
    // IClassFactory
    STDMETHOD(LockServer)(BOOL fLock);
    // helper
    void SetVoid(void* pv)
    {
        m_pfnCreateInstance = (_ATL_CREATORFUNC*)pv;
    }
    _ATL_CREATORFUNC* m_pfnCreateInstance;
};

template <class license>
class CComClassFactory2 : public CComClassFactory2Base, license
{
public:
    typedef license _LicenseClass;
    typedef CComClassFactory2<license> _ComMapClass;
    // IClassFactory
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
        REFIID riid, void** ppvObj)
    {
        ASSERT(m_pfnCreateInstance != NULL);
        if (ppvObj == NULL)
            return E_POINTER;
        *ppvObj = NULL;
        if (!IsLicenseValid())
            return CLASS_E_NOTLICENSED;

        if ((pUnkOuter != NULL) && !InlineIsEqualUnknown(riid))
            return CLASS_E_NOAGGREGATION;
        else
            return m_pfnCreateInstance(pUnkOuter, riid, ppvObj);
    }
    // IClassFactory2
    STDMETHOD(CreateInstanceLic)(IUnknown* pUnkOuter, IUnknown* pUnkReserved,
                REFIID riid, BSTR bstrKey, void** ppvObject)
    {
        ASSERT(m_pfnCreateInstance != NULL);
        if (ppvObject == NULL)
            return E_POINTER;
        *ppvObject = NULL;
        if ( ((bstrKey != NULL) && !VerifyLicenseKey(bstrKey)) ||
             ((bstrKey == NULL) && !IsLicenseValid()) )
            return CLASS_E_NOTLICENSED;
        return m_pfnCreateInstance(pUnkOuter, riid, ppvObject);
    }
    STDMETHOD(RequestLicKey)(DWORD dwReserved, BSTR* pbstrKey)
    {
        if (pbstrKey == NULL)
            return E_POINTER;
        *pbstrKey = NULL;

        if (!IsLicenseValid())
            return CLASS_E_NOTLICENSED;
        return GetLicenseKey(dwReserved,pbstrKey) ? S_OK : E_FAIL;
    }
    STDMETHOD(GetLicInfo)(LICINFO* pLicInfo)
    {
        if (pLicInfo == NULL)
            return E_POINTER;
        pLicInfo->cbLicInfo = sizeof(LICINFO);
        pLicInfo->fLicVerified = IsLicenseValid();
        BSTR bstr = NULL;
        pLicInfo->fRuntimeKeyAvail = GetLicenseKey(0,&bstr);
        ::SysFreeString(bstr);
        return S_OK;
    }
};

template <class T, const CLSID* pclsid>
class CComCoClass
{
public:
    DECLARE_CLASSFACTORY()
    DECLARE_AGGREGATABLE(T)
    typedef T _CoClass;
    static const CLSID& WINAPI GetObjectCLSID() {return *pclsid;}
    static HRESULT WINAPI Error(LPCOLESTR lpszDesc,
        const IID& iid = GUID_NULL, HRESULT hRes = 0)
    {return AtlReportError(GetObjectCLSID(), lpszDesc, iid, hRes);}
    static HRESULT WINAPI Error(UINT nID, const IID& iid = GUID_NULL,
        HRESULT hRes = 0)
    {return AtlReportError(GetObjectCLSID(), nID, iid, hRes);}
#ifndef OLE2ANSI
    static HRESULT WINAPI Error(LPCSTR lpszDesc,
        const IID& iid = GUID_NULL, HRESULT hRes = 0)
    {return AtlReportError(GetObjectCLSID(), lpszDesc, iid, hRes);}
#endif
};

// ATL doesn't support multiple LCID's at the same time
// Whatever LCID is queried for first is the one that is used.
class CComTypeInfoHolder
{
// Should be 'protected' but can cause compiler to generate fat code.
public:
    const GUID* m_pguid;
    const GUID* m_plibid;
    WORD m_wMajor;
    WORD m_wMinor;

    ITypeInfo* m_pInfo;
    long m_dwRef;

public:
    HRESULT GetTI(LCID lcid, ITypeInfo** ppInfo);

    void AddRef();
    void Release();
    HRESULT GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
    HRESULT GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
        LCID lcid, DISPID* rgdispid);
    HRESULT Invoke(IDispatch* p, DISPID dispidMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
        EXCEPINFO* pexcepinfo, UINT* puArgErr);
};

template <class T, const IID* piid, const GUID* plibid, WORD wMajor = 1,
WORD wMinor = 0, class tihclass = CComTypeInfoHolder>
class CComDualImpl : public T
{
public:
    typedef tihclass _tihclass;
    CComDualImpl() {_tih.AddRef();}
    ~CComDualImpl() {_tih.Release();}

    STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
    {*pctinfo = 1; return S_OK;}

    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
    {return _tih.GetTypeInfo(itinfo, lcid, pptinfo);}

    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
        LCID lcid, DISPID* rgdispid)
    {return _tih.GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);}

    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
        EXCEPINFO* pexcepinfo, UINT* puArgErr)
    {return _tih.Invoke((IDispatch*)this, dispidMember, riid, lcid,
        wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);}
protected:
    static _tihclass _tih;
    static HRESULT GetTI(LCID lcid, ITypeInfo** ppInfo)
    {return _tih.GetTI(lcid, ppInfo);}
};

template <class T, const IID* piid, const GUID* plibid, WORD wMajor,
WORD wMinor, class tihclass>
CComDualImpl<T, piid, plibid, wMajor, wMinor, tihclass>::_tihclass
CComDualImpl<T, piid, plibid, wMajor, wMinor, tihclass>::_tih =
{piid, plibid, wMajor, wMinor, NULL, 0};


template <const CLSID* pcoclsid, const IID* psrcid, const GUID* plibid,
WORD wMajor = 1, WORD wMinor = 0, class tihclass = CComTypeInfoHolder>
class CComProvideClassInfo2Impl : public IProvideClassInfo2
{
public:
    typedef tihclass _tihclass;
    CComProvideClassInfo2Impl() {_tih.AddRef();}
    ~CComProvideClassInfo2Impl() {_tih.Release();}

    STDMETHOD(GetClassInfo)(ITypeInfo** pptinfo)
    {
        return _tih.GetTypeInfo(0, LANG_NEUTRAL, pptinfo);
    }
    STDMETHOD(GetGUID)(DWORD dwGuidKind, GUID* pGUID)
    {
        if (pGUID == NULL)
            return E_POINTER;

        if (dwGuidKind == GUIDKIND_DEFAULT_SOURCE_DISP_IID && psrcid)
        {
            *pGUID = *psrcid;
            return S_OK;
        }
        *pGUID = GUID_NULL;
        return E_FAIL;
    }

protected:
    static _tihclass _tih;
};


template <const CLSID* pcoclsid, const IID* psrcid, const GUID* plibid,
WORD wMajor, WORD wMinor, class tihclass>
CComProvideClassInfo2Impl<pcoclsid, psrcid, plibid, wMajor, wMinor, tihclass>::_tihclass
CComProvideClassInfo2Impl<pcoclsid, psrcid, plibid, wMajor, wMinor, tihclass>::_tih =
{pcoclsid,plibid, wMajor, wMinor, NULL, 0};


/////////////////////////////////////////////////////////////////////////////
// CISupportErrorInfo

template <const IID* piid>
class CComISupportErrorInfoImpl : public ISupportErrorInfo
{
public:
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid)\
    {return (InlineIsEqualGUID(riid,*piid)) ? S_OK : S_FALSE;}
};


/////////////////////////////////////////////////////////////////////////////
// CComEnumImpl

// These _CopyXXX classes are used with enumerators in order to control
// how enumerated items are initialized, copied, and deleted

// Default is shallow copy with no special init or cleanup
template <class T>
class _Copy
{
public:
    static void copy(T* p1, T* p2) {memcpy(p1, p2, sizeof(T));}
    static void init(T*) {}
    static void destroy(T*) {}
};

class _Copy<VARIANT>
{
public:
    static void copy(VARIANT* p1, VARIANT* p2) {VariantCopy(p1, p2);}
    static void init(VARIANT* p) {VariantInit(p);}
    static void destroy(VARIANT* p) {VariantClear(p);}
};

class _Copy<LPOLESTR>
{
public:
    static void copy(LPOLESTR* p1, LPOLESTR* p2)
    {
        (*p1) = (LPOLESTR)CoTaskMemAlloc(sizeof(OLECHAR)*(ocslen(*p2)+1));
        ocscpy(*p1,*p2);
    }
    static void init(LPOLESTR* p) {*p = NULL;}
    static void destroy(LPOLESTR* p) { CoTaskMemFree(*p);}
};

class _Copy<OLEVERB>
{
public:
    static void copy(OLEVERB* p1, OLEVERB* p2)
    {
        *p1 = *p2;
        if (p1->lpszVerbName == NULL)
            return;
        p1->lpszVerbName = (LPOLESTR)CoTaskMemAlloc(sizeof(OLECHAR)*(ocslen(p2->lpszVerbName)+1));
        ocscpy(p1->lpszVerbName,p2->lpszVerbName);
    }
    static void init(OLEVERB* p) { p->lpszVerbName = NULL;}
    static void destroy(OLEVERB* p) { if (p->lpszVerbName) CoTaskMemFree(p->lpszVerbName);}
};

class _Copy<CONNECTDATA>
{
public:
    static void copy(CONNECTDATA* p1, CONNECTDATA* p2)
    {
        *p1 = *p2;
        if (p1->pUnk)
            p1->pUnk->AddRef();
    }
    static void init(CONNECTDATA* ) {}
    static void destroy(CONNECTDATA* p) {if (p->pUnk) p->pUnk->Release();}
};

template <class T>
class _CopyInterface
{
public:
    static void copy(T** p1, T** p2)
    {*p1 = *p2;if (*p1) (*p1)->AddRef();}
    static void init(T** ) {}
    static void destroy(T** p) {if (*p) (*p)->Release();}
};

template<class T>
class CComIEnum : public IUnknown
{
public:
    STDMETHOD(Next)(ULONG celt, T* rgelt, ULONG* pceltFetched) = 0;
    STDMETHOD(Skip)(ULONG celt) = 0;
    STDMETHOD(Reset)(void) = 0;
    STDMETHOD(Clone)(CComIEnum<T>** ppEnum) = 0;
};


enum CComEnumFlags
{
    //see FlagBits in CComEnumImpl
    AtlFlagNoCopy = 0,
    AtlFlagTakeOwnership = 2,
    AtlFlagCopy = 3 // copy implies ownership
};

template <class Base, const IID* piid, class T, class Copy>
class CComEnumImpl : public Base
{
public:
    CComEnumImpl() {m_begin = m_end = m_iter = NULL; m_dwFlags = 0; m_pUnk = NULL;}
    ~CComEnumImpl();
    STDMETHOD(Next)(ULONG celt, T* rgelt, ULONG* pceltFetched);
    STDMETHOD(Skip)(ULONG celt);
    STDMETHOD(Reset)(void){m_iter = m_begin;return S_OK;}
    STDMETHOD(Clone)(Base** ppEnum);
    HRESULT Init(T* begin, T* end, IUnknown* pUnk,
        CComEnumFlags flags = AtlFlagNoCopy);
    IUnknown* m_pUnk;
    T* m_begin;
    T* m_end;
    T* m_iter;
    DWORD m_dwFlags;
protected:
    enum FlagBits
    {
        BitCopy=1,
        BitOwn=2
    };
};

template <class Base, const IID* piid, class T, class Copy>
CComEnumImpl<Base, piid, T, Copy>::~CComEnumImpl()
{
    if (m_dwFlags & BitOwn)
    {
        for (T* p = m_begin; p != m_end; p++)
            Copy::destroy(p);
        delete [] m_begin;
    }
    if (m_pUnk)
        m_pUnk->Release();
}

template <class Base, const IID* piid, class T, class Copy>
STDMETHODIMP CComEnumImpl<Base, piid, T, Copy>::Next(ULONG celt, T* rgelt,
    ULONG* pceltFetched)
{
    if (rgelt == NULL || (celt != 1 && pceltFetched == NULL))
        return E_POINTER;
    if (m_begin == NULL || m_end == NULL || m_iter == NULL)
        return E_FAIL;
    ULONG nRem = (ULONG)(m_end - m_iter);
    HRESULT hRes = S_OK;
    if (nRem < celt)
        hRes = S_FALSE;
    ULONG nMin = min(celt, nRem);
    if (pceltFetched != NULL)
        *pceltFetched = nMin;
    while(nMin--)
        Copy::copy(rgelt++, m_iter++);
    return hRes;
}

template <class Base, const IID* piid, class T, class Copy>
STDMETHODIMP CComEnumImpl<Base, piid, T, Copy>::Skip(ULONG celt)
{
    m_iter += celt;
    if (m_iter < m_end)
        return S_OK;
    m_iter = m_end;
    return S_FALSE;
}

template <class Base, const IID* piid, class T, class Copy>
STDMETHODIMP CComEnumImpl<Base, piid, T, Copy>::Clone(Base** ppEnum)
{
    typedef CComObject<CComEnum<Base, piid, T, Copy> > _class;
    HRESULT hRes = E_POINTER;
    if (ppEnum != NULL)
    {
        _class* p = NULL;
        ATLTRY(p = new _class)
        if (p == NULL)
        {
            *ppEnum = NULL;
            hRes = E_OUTOFMEMORY;
        }
        else
        {
            // If the data is a copy then we need to keep "this" object around
            hRes = p->Init(m_begin, m_end, (m_dwFlags & BitCopy) ? this : m_pUnk);
            if (FAILED(hRes))
                delete p;
            else
            {
                p->m_iter = m_iter;
                hRes = p->_InternalQueryInterface(*piid, (void**)ppEnum);
                if (FAILED(hRes))
                    delete p;
            }
        }
    }
    return hRes;
}

template <class Base, const IID* piid, class T, class Copy>
HRESULT CComEnumImpl<Base, piid, T, Copy>::Init(T* begin, T* end, IUnknown* pUnk,
    CComEnumFlags flags)
{
    if (flags == AtlFlagCopy)
    {
        ASSERT(m_begin == NULL); //Init called twice?
        ATLTRY(m_begin = new T[(int)(end-begin)])
        m_iter = m_begin;
        if (m_begin == NULL)
            return E_OUTOFMEMORY;
        for (T* i=begin; i != end; i++)
        {
            Copy::init(m_iter);
            Copy::copy(m_iter++, i);
        }
        m_end = m_begin + (end-begin);
    }
    else
    {
        m_begin = begin;
        m_end = end;
    }
    m_pUnk = pUnk;
    if (m_pUnk)
        m_pUnk->AddRef();
    m_iter = m_begin;
    m_dwFlags = flags;
    return S_OK;
}

template <class Base, const IID* piid, class T, class Copy>
class CComEnum : public CComEnumImpl<Base, piid, T, Copy>, public CComObjectRoot
{
public:
    typedef CComEnum<Base, piid, T, Copy > _CComEnum;
    typedef CComEnumImpl<Base, piid, T, Copy > _CComEnumBase;
    BEGIN_COM_MAP(_CComEnum)
        COM_INTERFACE_ENTRY_IID(*piid, _CComEnumBase)
    END_COM_MAP()
};

#ifndef _ATL_NO_CONNECTION_POINTS
/////////////////////////////////////////////////////////////////////////////
// Connection Points

struct _ATL_CONNMAP_ENTRY
{
    DWORD dwOffset;
};


// We want the offset of the connection point relative to the connection
// point container base class
#define BEGIN_CONNECTION_POINT_MAP(x)\
    typedef x _atl_conn_classtype;\
    virtual const _ATL_CONNMAP_ENTRY* GetConnMap()\
        { return _StaticGetConnMap(); }\
    static const _ATL_CONNMAP_ENTRY* _StaticGetConnMap() {\
    static const _ATL_CONNMAP_ENTRY _entries[] = {
// CONNECTION_POINT_ENTRY computes the offset of the connection point to the
// IConnectionPointContainer interface
#define CONNECTION_POINT_ENTRY(member){offsetof(_atl_conn_classtype, member)-\
    offsetofclass(IConnectionPointContainer, _atl_conn_classtype)},
#define END_CONNECTION_POINT_MAP() {(DWORD)-1} }; return _entries;}


#ifndef _DEFAULT_VECTORLENGTH
#define _DEFAULT_VECTORLENGTH 4
#endif

template <unsigned int nMaxSize>
class CComStaticArrayCONNECTDATA
{
public:
    CComStaticArrayCONNECTDATA()
    {
        memset(m_arr, 0, sizeof(CONNECTDATA)*nMaxSize);
        m_pCurr = &m_arr[0];
    }
    BOOL Add(IUnknown* pUnk);
    BOOL Remove(DWORD dwCookie);
    CONNECTDATA* begin() {return &m_arr[0];}
    CONNECTDATA* end() {return &m_arr[nMaxSize];}
protected:
    CONNECTDATA m_arr[nMaxSize];
    CONNECTDATA* m_pCurr;
};

template <unsigned int nMaxSize>
inline BOOL CComStaticArrayCONNECTDATA<nMaxSize>::Add(IUnknown* pUnk)
{
    for (CONNECTDATA* p = begin();p<end();p++)
    {
        if (p->pUnk == NULL)
        {
            p->pUnk = pUnk;
            p->dwCookie = (DWORD)pUnk;
            return TRUE;
        }
    }
    return FALSE;
}

template <unsigned int nMaxSize>
inline BOOL CComStaticArrayCONNECTDATA<nMaxSize>::Remove(DWORD dwCookie)
{
    CONNECTDATA* p;
    for (p=begin();p<end();p++)
    {
        if (p->dwCookie == dwCookie)
        {
            p->pUnk = NULL;
            p->dwCookie = NULL;
            return TRUE;
        }
    }
    return FALSE;
}

class CComStaticArrayCONNECTDATA<1>
{
public:
    CComStaticArrayCONNECTDATA() {m_cd.pUnk = NULL; m_cd.dwCookie = 0;}
    BOOL Add(IUnknown* pUnk)
    {
        if (m_cd.pUnk != NULL)
            return FALSE;
        m_cd.pUnk = pUnk;
        m_cd.dwCookie = 1;
        return TRUE;
    }
    BOOL Remove(DWORD dwCookie)
    {
        if (dwCookie != m_cd.dwCookie)
            return FALSE;
        m_cd.pUnk = NULL;
        m_cd.dwCookie = 0;
        return TRUE;
    }
    CONNECTDATA* begin() {return &m_cd;}
    CONNECTDATA* end() {return (&m_cd)+1;}
protected:
    CONNECTDATA m_cd;
};

class CComDynamicArrayCONNECTDATA
{
public:
    CComDynamicArrayCONNECTDATA()
    {
        m_nSize = 0;
        m_pCD = NULL;
    }

    ~CComDynamicArrayCONNECTDATA() {free(m_pCD);}
    BOOL Add(IUnknown* pUnk);
    BOOL Remove(DWORD dwCookie);
    CONNECTDATA* begin() {return (m_nSize < 2) ? &m_cd : m_pCD;}
    CONNECTDATA* end() {return (m_nSize < 2) ? (&m_cd)+m_nSize : &m_pCD[m_nSize];}
protected:
    CONNECTDATA* m_pCD;
    CONNECTDATA m_cd;
    int m_nSize;
};

class CComConnectionPointBase : public IConnectionPoint, public CComObjectRoot
{
    typedef CComEnum<IEnumConnections, &IID_IEnumConnections, CONNECTDATA,
        _Copy<CONNECTDATA> > CComEnumConnections;
public:
    CComConnectionPointBase(IConnectionPointContainer* pCont, const IID* piid)
    {
        m_pContainer = pCont;
        m_piid = piid;
    }

    BEGIN_COM_MAP(CComConnectionPointBase)
        COM_INTERFACE_ENTRY(IConnectionPoint)
    END_COM_MAP()

    //Connection point lifetimes are determined by the container
    STDMETHOD_(ULONG, AddRef)() {ASSERT(m_pContainer != NULL); return m_pContainer->AddRef();}
    STDMETHOD_(ULONG, Release)(){ASSERT(m_pContainer != NULL); return m_pContainer->Release();}
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
    {return _InternalQueryInterface(iid, ppvObject);}

    STDMETHOD(GetConnectionInterface)(IID* piid);
    STDMETHOD(GetConnectionPointContainer)(IConnectionPointContainer** ppCPC);

    const IID* GetIID() {return m_piid;}
    void Lock() {m_sec.Lock();}
    void Unlock() {m_sec.Unlock();}
protected:
    const IID* m_piid;
    _ThreadModel::AutoCriticalSection m_sec;
    IConnectionPointContainer* m_pContainer;
    friend class CComConnectionPointContainerImpl;
};

class CComConnectionPointContainerImpl; // fwd decl

template <class CDV = CComDynamicArrayCONNECTDATA>
class CComConnectionPoint : public CComConnectionPointBase
{
    typedef CDV _CDV;
    typedef CComEnum<IEnumConnections, &IID_IEnumConnections, CONNECTDATA,
        _Copy<CONNECTDATA> > CComEnumConnections;
public:
    CComConnectionPoint(IConnectionPointContainer* pCont, const IID* piid) :
        CComConnectionPointBase(pCont, piid) {}

    // interface methods
    STDMETHOD(Advise)(IUnknown* pUnkSink, DWORD* pdwCookie);
    STDMETHOD(Unadvise)(DWORD dwCookie);
    STDMETHOD(EnumConnections)(IEnumConnections** ppEnum);

    _CDV m_vec;
    friend class CComConnectionPointContainerImpl;
};

template <class CDV>
STDMETHODIMP CComConnectionPoint<CDV>::Advise(IUnknown* pUnkSink,
    DWORD* pdwCookie)
{
    IUnknown* p;
    HRESULT hRes = S_OK;
    if (pUnkSink == NULL || pdwCookie == NULL)
        return E_POINTER;
    m_sec.Lock();
    if (SUCCEEDED(pUnkSink->QueryInterface(*m_piid, (void**)&p)))
    {
        *pdwCookie = (DWORD)p;
        hRes = m_vec.Add(p) ? S_OK : CONNECT_E_ADVISELIMIT;
        if (hRes != S_OK)
        {
            *pdwCookie = 0;
            p->Release();
        }
    }
    else
        hRes = CONNECT_E_CANNOTCONNECT;
    m_sec.Unlock();
    return hRes;
}

template <class CDV>
STDMETHODIMP CComConnectionPoint<CDV>::Unadvise(DWORD dwCookie)
{
    m_sec.Lock();
    HRESULT hRes = m_vec.Remove(dwCookie) ? S_OK : CONNECT_E_NOCONNECTION;
    IUnknown* p = (IUnknown*) dwCookie;
    m_sec.Unlock();
    if (hRes == S_OK && p != NULL)
        p->Release();
    return hRes;
}

template <class CDV>
STDMETHODIMP CComConnectionPoint<CDV>::EnumConnections(
    IEnumConnections** ppEnum)
{
    if (ppEnum == NULL)
        return E_POINTER;
    *ppEnum = NULL;
    CComObject<CComEnumConnections>* pEnum = NULL;
    ATLTRY(pEnum = new CComObject<CComEnumConnections>)
    if (pEnum == NULL)
        return E_OUTOFMEMORY;
    m_sec.Lock();
    CONNECTDATA* pcd = NULL;
    ATLTRY(pcd = new CONNECTDATA[m_vec.end()-m_vec.begin()])
    if (pcd == NULL)
    {
        delete pEnum;
        m_sec.Unlock();
        return E_OUTOFMEMORY;
    }
    CONNECTDATA* pend = pcd;
    // Copy the valid CONNECTDATA's
    for (CONNECTDATA* p = m_vec.begin();p<m_vec.end();p++)
    {
        if (p->pUnk != NULL)
        {
            p->pUnk->AddRef();
            *pend++ = *p;
        }
    }
    // don't copy the data, but transfer ownership to it
    pEnum->Init(pcd, pend, NULL, AtlFlagTakeOwnership);
    m_sec.Unlock();
    HRESULT hRes = pEnum->_InternalQueryInterface(IID_IEnumConnections, (void**)ppEnum);
    if (FAILED(hRes))
        delete pEnum;
    return hRes;
}


class CComConnectionPointContainerImpl : public IConnectionPointContainer
{
    typedef CComEnum<IEnumConnectionPoints,
        &IID_IEnumConnectionPoints, IConnectionPoint*,
        _CopyInterface<IConnectionPoint> >
        CComEnumConnectionPoints;
public:

    STDMETHOD(EnumConnectionPoints)(IEnumConnectionPoints** ppEnum);
    STDMETHOD(FindConnectionPoint)(REFIID riid, IConnectionPoint** ppCP);

protected:
    virtual const _ATL_CONNMAP_ENTRY* GetConnMap() = 0;
    CComConnectionPointBase* FindConnPoint(REFIID riid);
    void InitCloneVector(CComConnectionPointBase** ppCP);
};


#endif //!_ATL_NO_CONNECTION_POINTS

#pragma pack(pop)

/////////////////////////////////////////////////////////////////////////////
// CComModule

//Although these functions are big, they are only used once in a module
//so we should make them inline.

inline void CComModule::Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h)
{
    m_pObjMap   = p;
    m_hInst     = h;
    m_nLockCnt  = 0L;

    m_csTypeInfoHolder.Init();
    m_csObjMap.Init();
}

inline HRESULT CComModule::RegisterClassObjects(DWORD dwClsContext, DWORD dwFlags)
{
    ASSERT(m_pObjMap != NULL);
    _ATL_OBJMAP_ENTRY* pEntry = m_pObjMap;
    HRESULT hRes = S_OK;
    while (pEntry->pclsid != NULL && hRes == S_OK)
    {
        hRes = pEntry->RegisterClassObject(dwClsContext, dwFlags);
        pEntry++;
    }
    return hRes;
}

inline HRESULT CComModule::RevokeClassObjects()
{
    ASSERT(m_pObjMap != NULL);
    _ATL_OBJMAP_ENTRY* pEntry = m_pObjMap;
    HRESULT hRes = S_OK;
    while (pEntry->pclsid != NULL && hRes == S_OK)
    {
        hRes = pEntry->RevokeClassObject();
        pEntry++;
    }
    return hRes;
}

inline HRESULT CComModule::GetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    ASSERT(m_pObjMap != NULL);
    _ATL_OBJMAP_ENTRY* pEntry = m_pObjMap;
    HRESULT hRes = S_OK;
    if (ppv == NULL)
        return E_POINTER;
    while (pEntry->pclsid != NULL)
    {
        if (InlineIsEqualGUID(rclsid, *pEntry->pclsid))
        {
            if (pEntry->pCF == NULL)
            {
                m_csObjMap.Lock();
                if (pEntry->pCF == NULL)
                    hRes = pEntry->pfnGetClassObject(pEntry->pfnCreateInstance, IID_IUnknown, (LPVOID*)&pEntry->pCF);
                m_csObjMap.Unlock();
            }
            if (pEntry->pCF != NULL)
                hRes = pEntry->pCF->QueryInterface(riid, ppv);
            break;
        }
        pEntry++;
    }
    if (*ppv == NULL && hRes == S_OK)
        hRes = CLASS_E_CLASSNOTAVAILABLE;
    return hRes;
}

inline void CComModule::Term()
{
    ASSERT(m_hInst != NULL);

    if (m_pObjMap != NULL)
    {
        _ATL_OBJMAP_ENTRY* pEntry = m_pObjMap;

        while (pEntry->pclsid != NULL)
        {
            if (pEntry->pCF != NULL)
                pEntry->pCF->Release();

            pEntry->pCF = NULL;
            pEntry++;
        }
    }

    m_csTypeInfoHolder.Term();
    m_csObjMap.Term();
}

inline HRESULT CComModule::RegisterServer(BOOL bRegTypeLib)
{
    ASSERT(m_hInst != NULL);
    ASSERT(m_pObjMap != NULL);
    _ATL_OBJMAP_ENTRY* pEntry = m_pObjMap;
    HRESULT hRes = S_OK;
    while (pEntry->pclsid != NULL)
    {
        hRes = pEntry->pfnUpdateRegistry(TRUE);
        if (FAILED(hRes))
            break;
        pEntry++;
    }
    if (SUCCEEDED(hRes) && bRegTypeLib)
        hRes = RegisterTypeLib();
    return hRes;
}

inline HRESULT CComModule::UnregisterServer()
{
    ASSERT(m_hInst != NULL);
    ASSERT(m_pObjMap != NULL);
    _ATL_OBJMAP_ENTRY* pEntry = m_pObjMap;
    while (pEntry->pclsid != NULL)
    {
        pEntry->pfnUpdateRegistry(FALSE); //unregister
        pEntry++;
    }
    return S_OK;
}

#endif // __ATLCOM_H__

/////////////////////////////////////////////////////////////////////////////

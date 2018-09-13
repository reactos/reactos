/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _DISPATCH_HXX
#define _DISPATCH_HXX

#ifndef __dispex_h__
#include <dispex.h>
#endif

extern TAG tagDispatch;

HRESULT GetTypeInfo(REFGUID libid, int ord, LCID lcid, REFGUID uuid, ITypeInfo **ppITypeInfo);

struct INVOKE_ARG
{
    VARIANT     vArg;
    bool        fClear;
    bool        fMissing;
};

typedef HRESULT STDMETHODCALLTYPE INVOKEFUNC(void *pTarget, DISPID dispid, INVOKE_ARG *prgMethods, WORD wInvokeType, VARIANT *pVarResult, UINT cArgs);

struct INVOKE_METHOD
{
    WCHAR       *pwszName;
    DISPID      dispid;
    BYTE        cArgs;
    VARTYPE     *rgTypes;
    const IID   **rgTypeIds;
    VARTYPE     vtResult;
    WORD        wInvokeType;
};

struct DISPIDTOINDEX
{
    DISPID dispid;
    int    index;
};

struct DISPATCHINFO
{
    ITypeInfo * _pTypeInfoCache; 
    const GUID * _puuid;
    const IID * _plibid; 
    int _ord;
    INVOKE_METHOD * pMethods;
    BYTE            cMethods;
    DISPIDTOINDEX * pIndex;
    BYTE            cIndex;
    INVOKEFUNC    * pInvokeFunc;
};


class NOVTABLE _dispatchImpl
{
public:
    static HRESULT GetTypeInfoCount( DISPATCHINFO * pdispatchinfo,
        /* [out] */ UINT __RPC_FAR *pctinfo);
    
    static HRESULT GetTypeInfo( DISPATCHINFO * pdispatchinfo,
        /* [in] */ UINT iTInfo,
        /* [in] */ LCID lcid,
        /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
    
    static HRESULT GetIDsOfNames( DISPATCHINFO * pdispatchinfo,
        /* [in] */ REFIID riid,
        /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
        /* [in] */ UINT cNames,
        /* [in] */ LCID lcid,
        /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
    
    static HRESULT Invoke( DISPATCHINFO * pdispatchinfo, VOID FAR* pObj,
        /* [in] */ DISPID dispIdMember,
        /* [in] */ REFIID riid,
        /* [in] */ LCID lcid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
        /* [out] */ VARIANT __RPC_FAR *pVarResult,
        /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
        /* [out] */ UINT __RPC_FAR *puArgErr);

    // IDispatchEx
    static HRESULT GetDispID( 
        DISPATCHINFO * pdispatchinfo,
        bool fCollection,
        /* [in] */ BSTR bstrName,
        /* [in] */ DWORD grfdex,
        /* [out] */ DISPID __RPC_FAR *pid);
    
    static HRESULT InvokeEx( 
        IDispatch* pObj,
        DISPATCHINFO * pdispatchinfo, 
        bool fCollection,
        /* [in] */ DISPID id,
        /* [in] */ LCID lcid,
        /* [in] */ WORD wFlags,
        /* [in] */ DISPPARAMS __RPC_FAR *pdp,
        /* [out] */ VARIANT __RPC_FAR *pvarRes,
        /* [out] */ EXCEPINFO __RPC_FAR *pei,
        /* [unique][in] */ IServiceProvider __RPC_FAR *pspCaller);

    static HRESULT ensureTypeInfo( DISPATCHINFO * pdispatchinfo,
        /* [in] */ LCID lcid);

    static Exception * setErrorInfo(Exception * e);

    static void setErrorInfo(const WCHAR * szDescription);

    static void setErrorInfo(ResourceID resId);

    static HRESULT InvokeHelper(
        /* [in] */ void * pTarget,
        /* [in] */ DISPATCHINFO * pdispatchinfo,
        /* [in] */ DISPID dispid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
        /* [out] */ VARIANT __RPC_FAR *pVarResult,
        /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
        /* [out] */ UINT __RPC_FAR *puArgErr);

    static HRESULT FindIdsOfNames(
        WCHAR **rgNames, 
        UINT cNames,
        INVOKE_METHOD *rgKnownNames,
        int cKnownNames, LCID lcid,
        DISPID *rgdispid,
        bool fCaseSensitive = false );

#if DBG == 1

    static void TestMethodTable(INVOKE_METHOD* table, int size);
    static void TestIndexMap(INVOKE_METHOD* table, DISPIDTOINDEX* indexTable, int size);

#endif

protected:

    static HRESULT FindIndex(DISPID dispid, DISPIDTOINDEX * pIndexTable, int cMax, int &index);
    static HRESULT PrepareInvokeArgs( DISPPARAMS *pdispparams, INVOKE_ARG *rgArgs, VARTYPE *rgTypes, const GUID **rgTypeIds, UINT cArgs );
    static HRESULT PrepareInvokeArgsAndResult( DISPPARAMS *pdispParams, WORD wFlags, INVOKE_METHOD *pInvokeInfo, VARIANT *&pvarResult, INVOKE_ARG *rgArgs, UINT &cArgs, WORD &wInvokeType);
    static HRESULT FailedInvoke(const HRESULT &hr, EXCEPINFO * pexcepinfo);
};


class NOVTABLE __dispatch : public ISupportErrorInfo
{
protected:  DISPATCHINFO * _pdispatchinfo;

public: __dispatch(DISPATCHINFO * pdispatchinfo) : _pdispatchinfo(pdispatchinfo) {}

public: HRESULT QueryInterface(IUnknown * punk, REFIID riid, void ** ppvObject);

public:

    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(
        /* [out] */ UINT __RPC_FAR *pctinfo);
    
    HRESULT STDMETHODCALLTYPE GetTypeInfo(
        /* [in] */ UINT iTInfo,
        /* [in] */ LCID lcid,
        /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
    
    HRESULT STDMETHODCALLTYPE GetIDsOfNames(
        /* [in] */ REFIID riid,
        /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
        /* [in] */ UINT cNames,
        /* [in] */ LCID lcid,
        /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
    
    HRESULT STDMETHODCALLTYPE Invoke(void * pobj, 
        /* [in] */ DISPID dispIdMember,
        /* [in] */ REFIID riid,
        /* [in] */ LCID lcid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
        /* [out] */ VARIANT __RPC_FAR *pVarResult,
        /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
        /* [out] */ UINT __RPC_FAR *puArgErr);

    // ISupportErrorInfo

    virtual HRESULT STDMETHODCALLTYPE InterfaceSupportsErrorInfo(REFIID riid);
};


template <class T, class I, const IID * libid, int ord, const IID* I_IID> class NOVTABLE _dispatchexport : public I, public __dispatch, public __comexport
{
protected:    static DISPATCHINFO s_dispatchinfo;

public:        
    _dispatchexport<T, I, libid, ord, I_IID>(T * pWrapped) : __comexport(s_dispatchinfo._puuid, reinterpret_cast<Object *>(pWrapped)), __dispatch(&s_dispatchinfo) {}

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject)
            {
                HRESULT hr = __dispatch::QueryInterface((I *)this, riid, ppvObject);
                if (hr)
                {
                    hr = __comexport::QueryInterface((I *)this, riid, ppvObject);
                }
                return hr;
            }

 
    virtual ULONG STDMETHODCALLTYPE AddRef()
            {
                return __comexport::AddRef();
            }

    virtual ULONG STDMETHODCALLTYPE Release()
            {
                return __comexport::Release();
            }

    T * getWrapped()
            {
                return reinterpret_cast<T *>((Object *)_pWrapped);
            }

    static T * getExported(IUnknown * p, REFIID refIID)
            {
                return reinterpret_cast<T *>(__comexport::getExported(p, refIID));
            }

    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount( 
        /* [out] */ UINT __RPC_FAR *pctinfo)
    {
        return __dispatch::GetTypeInfoCount(pctinfo);
    }
    
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo( 
        /* [in] */ UINT iTInfo,
        /* [in] */ LCID lcid,
        /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo)
    {
        return __dispatch::GetTypeInfo(iTInfo, lcid, ppTInfo);
    }
    
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames( 
        /* [in] */ REFIID riid,
        /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
        /* [in] */ UINT cNames,
        /* [in] */ LCID lcid,
        /* [size_is][out] */ DISPID __RPC_FAR *rgDispId)
    {
        return __dispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispId);
    }
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Invoke( 
        /* [in] */ DISPID dispIdMember,
        /* [in] */ REFIID riid,
        /* [in] */ LCID lcid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
        /* [out] */ VARIANT __RPC_FAR *pVarResult,
        /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
        /* [out] */ UINT __RPC_FAR *puArgErr)
    {
        return __dispatch::Invoke((I * )this,
                dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    }
};

/*
    NOTE: the order in which the ancestor classes is declared must be preserved
*/
extern TAG tagRefCount;
extern LONG _dispatchCount;

template <class I, const IID * libid, const IID* I_IID> class NOVTABLE _dispatch : public I, public __dispatch, public __unknown
{
protected:    static DISPATCHINFO s_dispatchinfo;

public:
        _dispatch<I, libid, I_IID>() : __unknown(s_dispatchinfo._puuid), __dispatch(&s_dispatchinfo)
        { 
            TraceTag((tagRefCount, "IncrementComponents - _dispatch<I, libid, I_IID>"));
#if DBG == 1
            InterlockedIncrement(&_dispatchCount);
#endif
            ::IncrementComponents(); 
        }

#ifdef RENTAL_MODEL
        _dispatch<I, libid, I_IID>(RentalEnum re) : __unknown(s_dispatchinfo._puuid, re), __dispatch(&s_dispatchinfo)
        { 
            TraceTag((tagRefCount, "IncrementComponents - _dispatch<I, libid, I_IID>"));
#if DBG == 1
            InterlockedIncrement(&_dispatchCount);
#endif
            ::IncrementComponents(); 
        }
#endif

        virtual ~_dispatch<I, libid, I_IID>()
        {
            TraceTag((tagRefCount, "DecrementComponents - ~_dispatch<I, libid, I_IID>"));
#if DBG == 1
            InterlockedDecrement(&_dispatchCount);
#endif
            ::DecrementComponents();
        }

        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject)
        {
            HRESULT hr = __dispatch::QueryInterface((I *)this, riid, ppvObject);
            if (hr)
            {
                hr = __unknown::QueryInterface((I *)this, riid, ppvObject);
            }
            return hr;
        }
    
        virtual ULONG STDMETHODCALLTYPE AddRef( void)
        {
            return __unknown::AddRef();
        }
    
        virtual ULONG STDMETHODCALLTYPE Release( void)
        {
            return __unknown::Release();
        }

    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount( 
        /* [out] */ UINT __RPC_FAR *pctinfo)
    {
        return __dispatch::GetTypeInfoCount(pctinfo);
    }
    
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo( 
        /* [in] */ UINT iTInfo,
        /* [in] */ LCID lcid,
        /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo)
    {
        return __dispatch::GetTypeInfo(iTInfo, lcid, ppTInfo); 
    }
    
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames( 
        /* [in] */ REFIID riid,
        /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
        /* [in] */ UINT cNames,
        /* [in] */ LCID lcid,
        /* [size_is][out] */ DISPID __RPC_FAR *rgDispId)
    {
        return __dispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispId);
    }
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Invoke( 
        /* [in] */ DISPID dispIdMember,
        /* [in] */ REFIID riid,
        /* [in] */ LCID lcid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
        /* [out] */ VARIANT __RPC_FAR *pVarResult,
        /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
        /* [out] */ UINT __RPC_FAR *puArgErr)
    {
        return __dispatch::Invoke((I * )this,
                dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    }
};    

template <class I, const IID * libid, const IID* I_IID, bool fCollection> class NOVTABLE _dispatchEx : public _dispatch<I, libid, I_IID>, public IDispatchEx
{
public:        
        _dispatchEx<I, libid, I_IID, fCollection>() : _dispatch<I, libid, I_IID>()
        { 
        }

#ifdef RENTAL_MODEL
        _dispatchEx<I, libid, I_IID, fCollection>(RentalEnum re) : _dispatch<I, libid, I_IID>(re)
        { 
        }
#endif

        virtual ~_dispatchEx<I, libid, I_IID, fCollection>()
        {
        }

        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject)
        {
            HRESULT hr;
            if (riid == IID_IDispatchEx)
            {
                AddRef();
                *ppvObject = static_cast<IDispatchEx *>(this);
                hr = S_OK;
            }
            else
            {
                hr = _dispatch<I, libid, I_IID>::QueryInterface(riid, ppvObject);
            }
            return hr;
        }
    
        virtual ULONG STDMETHODCALLTYPE AddRef( void)
        {
            return _dispatch<I, libid, I_IID>::AddRef();
        }
    
        virtual ULONG STDMETHODCALLTYPE Release( void)
        {
            return _dispatch<I, libid, I_IID>::Release();
        }

    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount( 
        /* [out] */ UINT __RPC_FAR *pctinfo)
    {
        return _dispatch<I, libid, I_IID>::GetTypeInfoCount(pctinfo);
    }
    
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo( 
        /* [in] */ UINT iTInfo,
        /* [in] */ LCID lcid,
        /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo)
    {
        return _dispatch<I, libid, I_IID>::GetTypeInfo(iTInfo, lcid, ppTInfo); 
    }
    
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames( 
        /* [in] */ REFIID riid,
        /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
        /* [in] */ UINT cNames,
        /* [in] */ LCID lcid,
        /* [size_is][out] */ DISPID __RPC_FAR *rgDispId)
    {
        TraceTag((tagDispatch, "GetIDsOfNames"));
        return _dispatch<I, libid, I_IID>::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispId);
    }
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Invoke( 
        /* [in] */ DISPID dispIdMember,
        /* [in] */ REFIID riid,
        /* [in] */ LCID lcid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
        /* [out] */ VARIANT __RPC_FAR *pVarResult,
        /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
        /* [out] */ UINT __RPC_FAR *puArgErr)
    {
        TraceTag((tagDispatch, "Invoke"));
        return _dispatch<I, libid, I_IID>::Invoke(dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    }

    HRESULT STDMETHODCALLTYPE GetDispID( 
        /* [in] */ BSTR bstrName,
        /* [in] */ DWORD grfdex,
        /* [out] */ DISPID __RPC_FAR *pid)
    {
        TraceTag((tagDispatch, "GetDispID"));
        return _dispatchImpl::GetDispID(&_dispatch<I, libid, I_IID>::s_dispatchinfo, fCollection,
                bstrName, grfdex, pid);
    }
    
    HRESULT STDMETHODCALLTYPE InvokeEx( 
        /* [in] */ DISPID id,
        /* [in] */ LCID lcid,
        /* [in] */ WORD wFlags,
        /* [in] */ DISPPARAMS __RPC_FAR *pdp,
        /* [out] */ VARIANT __RPC_FAR *pvarRes,
        /* [out] */ EXCEPINFO __RPC_FAR *pei,
        /* [unique][in] */ IServiceProvider __RPC_FAR *pspCaller)
    {
        TraceTag((tagDispatch, "InvokeEx"));
        return _dispatchImpl::InvokeEx((I * )this, &_dispatch<I, libid, I_IID>::s_dispatchinfo, fCollection,
                id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);
    }
    
    HRESULT STDMETHODCALLTYPE DeleteMemberByName( 
        /* [in] */ BSTR bstrName,
        /* [in] */ DWORD grfdex)
    {
        TraceTag((tagDispatch, "DeleteMemberByName"));
        return E_NOTIMPL;
    }
    
    HRESULT STDMETHODCALLTYPE DeleteMemberByDispID( 
        /* [in] */ DISPID id)
    {
        TraceTag((tagDispatch, "DeleteMemberByDispID"));
        return E_NOTIMPL;
    }
    
    HRESULT STDMETHODCALLTYPE GetMemberProperties( 
        /* [in] */ DISPID id,
        /* [in] */ DWORD grfdexFetch,
        /* [out] */ DWORD __RPC_FAR *pgrfdex)
    {
        TraceTag((tagDispatch, "GetMemberProperties"));
        return E_NOTIMPL;
    }
    
    HRESULT STDMETHODCALLTYPE GetMemberName( 
        /* [in] */ DISPID id,
        /* [out] */ BSTR __RPC_FAR *pbstrName)
    {
        TraceTag((tagDispatch, "GetMemberName"));
        return E_NOTIMPL;
    }
    
    HRESULT STDMETHODCALLTYPE GetNextDispID( 
        /* [in] */ DWORD grfdex,
        /* [in] */ DISPID id,
        /* [out] */ DISPID __RPC_FAR *pid)
    {
        TraceTag((tagDispatch, "GetNextDispID"));
        return E_NOTIMPL;
    }
    
    HRESULT STDMETHODCALLTYPE GetNameSpaceParent( 
        /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunk)
    {
        TraceTag((tagDispatch, "GetNameSpaceParent"));
        return E_NOTIMPL;
    }
};    


/////////////////////////////////////////////////////////////////////////////////////
// Macros to help implementing IDispatch::Invoke without using oleaut.dll
//

#define VT_OPTIONAL 0x0800

#define VARMEMBER(pv,m) (((pv)->vt&VT_BYREF)?*((pv)->p##m):(pv)->##m)
#define VARTYPEMAP(vt)  (vt&~VT_BYREF)

#ifndef NUMELEM
# define NUMELEM( rg ) (sizeof(rg)/sizeof(*rg))
#endif



// Requires some fixed named variables: eg. wInVokeType as the invoke type 
#define PROPERTY_INVOKE_READWRITE( pObj, _property, _member, _type ) \
    if ( DISPATCH_PROPERTYGET == wInvokeType )\
        hr = pObj->get_##_property( (_type *) &pVarResult->_member );\
    else\
        hr = pObj->put_##_property( (_type) VARMEMBER(&rgArgs[0].vArg,_member) );\


#define PROPERTY_INVOKE_READWRITE_VARIANT( pObj, _property ) \
    if ( DISPATCH_PROPERTYGET == wInvokeType)\
        hr = pObj->get_##_property( pVarResult );\
    else\
        hr = pObj->put_##_property( rgArgs[0].vArg );

// method with 1 input parameter and a return VT_DISPATCH result
#define METHOD_INVOKE_1( pObj, _method, _member, _nodetype, _rtnode ) \
    hr = pObj->_method( (_nodetype) VARMEMBER( &rgArgs[0].vArg, _member),\
                  (_rtnode*) &pVarResult->pdispVal);


#if DBG == 1

#define TEST_METHOD_TABLE(table, size, indexTable, indexSize)\
    static fTest = true;\
    if (fTest) \
    {\
        fTest = false; \
        _dispatchImpl::TestMethodTable(table, size); \
        _dispatchImpl::TestIndexMap(table, indexTable, indexSize);\
    }

#else
#define TEST_METHOD_TABLE(table, size, indexTable, indexSize)
#endif

#endif _DISPATCH_HXX

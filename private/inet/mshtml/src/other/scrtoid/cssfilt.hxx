#ifndef __CSSFILT_HXX__
#define __CSSFILT_HXX__

#pragma warning(disable:4100)   // unreferenced formal param

#define DISPID_CSSFILTERHANDLER_ELEMENT         2500
#define DISPID_CSSFILTERHANDLER_PARAMS          DISPID_CSSFILTERHANDLER_ELEMENT+1


interface ICSSFilterSite;
interface ICSSFilter;

class CCSSFilterCP;
class CCSSFilterIntDispatch;
class CCSSFilterDispatchSink;

MtExtern(CCSSFilterHandler)
MtExtern(CCSSFilterCP_arySinks_pv)

// BUGBUG: Appears ICPC is unnecessary.. remove it and related classes
class CCSSFilterHandler : public CBase
{
    DECLARE_CLASS_TYPES(CCSSFilterHandler, CBase)

    friend CCSSFilterIntDispatch;

public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CCSSFilterHandler))

    CCSSFilterHandler(IUnknown *pUnkContext, IUnknown *pUnkOuter);
    virtual ~CCSSFilterHandler ();

    DECLARE_AGGREGATED_IUNKNOWN(CCSSFilterHandler)
    DECLARE_PRIVATE_QI_FUNCS(CCSSFilterHandler)
    DECLARE_DERIVED_DISPATCH(CCSSFilterHandler)

    DECLARE_TEAROFF_TABLE(ICSSFilter)
    DECLARE_TEAROFF_TABLE(IConnectionPointContainer)
    DECLARE_TEAROFF_TABLE(IPersistPropertyBag2)
    DECLARE_TEAROFF_TABLE(IObjectSafety)
    DECLARE_TEAROFF_TABLE(IScriptletHandler)
    DECLARE_TEAROFF_TABLE(IScriptletHandlerConstructor)

    // IScriptletHandlerConstructor
    NV_DECLARE_TEAROFF_METHOD(Load, load, (WORD wFlags, IScriptletXML *pxmlElement))
        { return S_OK;}
	NV_DECLARE_TEAROFF_METHOD(Create, create, (IUnknown *punkContext, IUnknown *punkOuter, IUnknown **ppunkHandler));
	NV_DECLARE_TEAROFF_METHOD(Register, register, (LPCOLESTR pstrPath))
	    { RRETURN(E_NOTIMPL); }
	NV_DECLARE_TEAROFF_METHOD(Unregister, unregister, ())
	    { RRETURN(E_NOTIMPL); }
	NV_DECLARE_TEAROFF_METHOD(AddInterfaceTypeInfo, addinterfacetypeinfo, (ICreateTypeLib *ptclib, ICreateTypeInfo *pctiCoclass))
	    { RRETURN(E_NOTIMPL); }

    // IScriptletHandler
	NV_DECLARE_TEAROFF_METHOD(GetNameSpaceObject, getnamespaceobject, (IUnknown **ppunk));
	NV_DECLARE_TEAROFF_METHOD(SetScriptNameSpace, setscriptnamespace, (IUnknown *punkNameSpace));

    // ICSSFilter methods
    NV_DECLARE_TEAROFF_METHOD(SetSite, setsite, (ICSSFilterSite *pSite));
    NV_DECLARE_TEAROFF_METHOD(OnAmbientPropertyChange, onambientpropertychange, (DISPID dispid))
        { return S_OK; }

    // IPersist methods
    NV_DECLARE_TEAROFF_METHOD(GetClassID, getclassid, (CLSID *pclsID));

    // IPersistPropertyBag2 methods
    NV_DECLARE_TEAROFF_METHOD(InitNew, initnew, (void))
      { return S_OK; }     
    NV_DECLARE_TEAROFF_METHOD(Load, load, (IPropertyBag2  *pPropBag, IErrorLog *pErrLog));
    NV_DECLARE_TEAROFF_METHOD(Save, save, (IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties))
      { return E_FAIL; }
    NV_DECLARE_TEAROFF_METHOD(IsDirty, isdirty, (void))
      { return S_FALSE; }

    // IObjectSafety methods
    NV_DECLARE_TEAROFF_METHOD(GetInterfaceSafetyOptions, getinterfacesafetyoptions, (REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions));
    NV_DECLARE_TEAROFF_METHOD(SetInterfaceSafetyOptions, setinterfacesafetyoptions, (REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions));

    // IConnectionPointContainer
    NV_DECLARE_TEAROFF_METHOD(EnumConnectionPoints, enumconnectionpoints, (IEnumConnectionPoints **ppEnum));
    NV_DECLARE_TEAROFF_METHOD(FindConnectionPoint, findconnectionpoint, (REFIID riid, IConnectionPoint **ppCP));

    STDMETHODIMP RunFilterCode();
    void UnloadFilter();

    static const CBase::CLASSDESC s_classdesc;
    virtual const CBase::CLASSDESC *GetClassDesc() const
        { return  (CBase::CLASSDESC *)&s_classdesc; }

    virtual HRESULT Init();
    virtual void Passivate();
    IUnknown * PunkOuter() { return _pUnkOuter; }

private:
    IUnknown               *_pUnkOuter;     // Outer unknown
    ICSSFilterSite         *_pFilterSite;   // setup/torn down in SetSite()
    CAttrBag               *_pAttrBag;      // setup/torn down in Load, also torn down in dtor
    CCSSFilterCP           *_pCP;           // a CP that we hand out (maybe? who wants one??)

    // The following are setup in RunFilterCode(), torn down in UnloadFilter()
    IHTMLElement           *_pElement;
    IConnectionPoint       *_pWinDispCP;    // a CP that we obtained from Trident's window obj
    CCSSFilterDispatchSink *_pDispSink;     // the sink we handed to Trident
    DWORD                   _dwDispCookie;  // advise cookie for sink
    IDispatch              *_pScript;       // dispatch ptr to script engine
    IDispatch              *_pCSSDisp;
};

class CCSSFilterCP : public IConnectionPoint
{
    friend CCSSFilterHandler;
public:
    CCSSFilterCP(CCSSFilterHandler *pContainer);
    virtual ~CCSSFilterCP();

    // IUnknown
    STDMETHODIMP         QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
        
    // IConnectionPoint
    STDMETHODIMP GetConnectionInterface( IID *pIID );
    STDMETHODIMP GetConnectionPointContainer( IConnectionPointContainer **ppCPC);
    STDMETHODIMP Advise( IUnknown *pUnkSink,
                         DWORD *pdwCookie );
    STDMETHODIMP Unadvise( DWORD dwCookie );
    STDMETHODIMP EnumConnections( IEnumConnections **ppEnum );

private:
    void Detach();

    DWORD _cRef;
    CCSSFilterHandler *_pContainer;

    DECLARE_CPtrAry(CArySinks, IDispatch *, Mt(Mem), Mt(CCSSFilterCP_arySinks_pv))
    CArySinks _arySinks;
};

class CCSSFilterDispatchSink : public IDispatch
{
public:
    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IDispatch methods
    STDMETHODIMP         GetTypeInfoCount(UINT *pctinfo)
                          { *pctinfo = 0; return S_OK; }
    STDMETHODIMP         GetTypeInfo(UINT iTInfo,
                                     LCID lcid,
                                     ITypeInfo **ppTInfo)
                          { *ppTInfo = NULL; return S_OK; }
    
    STDMETHODIMP         GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames,
                                       UINT cNames, LCID lcid,
                                       DISPID *rgDispId);
    
    STDMETHODIMP         Invoke(DISPID dispIdMember,
                                REFIID riid,
                                LCID lcid,
                                WORD wFlags,
                                DISPPARAMS *pDispParams,
                                VARIANT *pVarResult,
                                EXCEPINFO *pExcepInfo,
                                UINT *puArgErr);
    CCSSFilterDispatchSink(CCSSFilterHandler *pCSSFilterHandler);
    virtual ~CCSSFilterDispatchSink();

    void Detach();

private:
    DWORD               _cRef;
    CCSSFilterHandler   *_pCSSFilterHandler;
};

class CCSSFilterIntDispatch : public IDispatchEx
{
public:
    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IDispatch methods
    STDMETHODIMP         GetTypeInfoCount(UINT *pctinfo)
                          { *pctinfo = 0; return S_OK; }
    STDMETHODIMP         GetTypeInfo(UINT iTInfo,
                                     LCID lcid,
                                     ITypeInfo **ppTInfo)
                          { *ppTInfo = NULL; return S_OK; }
    
    STDMETHODIMP         GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames,
                                       UINT cNames, LCID lcid,
                                       DISPID *rgDispId);
    
    STDMETHODIMP         Invoke(DISPID dispIdMember,
                                REFIID riid,
                                LCID lcid,
                                WORD wFlags,
                                DISPPARAMS *pDispParams,
                                VARIANT *pVarResult,
                                EXCEPINFO *pExcepInfo,
                                UINT *puArgErr);

    // IDispatchEx methods
    STDMETHODIMP GetDispID( /* [in] */ BSTR bstrName,
                            /* [in] */ DWORD grfdex,
                            /* [out] */ DISPID *pid);
        
    STDMETHODIMP InvokeEx(  /* [in] */ DISPID id,
                            /* [in] */ LCID lcid,
                            /* [in] */ WORD wFlags,
                            /* [in] */ DISPPARAMS *pdp,
                            /* [out] */ VARIANT *pvarRes,
                            /* [out] */ EXCEPINFO *pei,
                            /* [unique][in] */ IServiceProvider *pspCaller);
        
    STDMETHODIMP DeleteMemberByName( /* [in] */ BSTR bstr,
                                     /* [in] */ DWORD grfdex);
        
    STDMETHODIMP DeleteMemberByDispID( /* [in] */ DISPID id);
        
    STDMETHODIMP GetMemberProperties( /* [in] */ DISPID id,
                                      /* [in] */ DWORD grfdexFetch,
                                      /* [out] */ DWORD *pgrfdex);
        
    STDMETHODIMP GetMemberName( /* [in] */ DISPID id,
                                /* [out] */ BSTR *pbstrName);
        
    STDMETHODIMP GetNextDispID( /* [in] */ DWORD grfdex,
                                /* [in] */ DISPID id,
                                /* [out] */ DISPID *pid);
        
    STDMETHODIMP GetNameSpaceParent( /* [out] */ IUnknown **ppunk);    


    CCSSFilterIntDispatch(CCSSFilterHandler *pCSSFilterHandler);
    virtual ~CCSSFilterIntDispatch();

private:
    DWORD               _cRef;
    CCSSFilterHandler   *_pCSSFilterHandler;
};


#endif // __CSSFILT_HXX__

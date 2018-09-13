#ifndef __ATTRBAG_HXX_INCLUDED__
#define __ATTRBAG_HXX_INCLUDED__

class CAttrBag : public IDispatchEx
{
public:
    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IDispatch methods
    STDMETHODIMP         GetTypeInfoCount(UINT *pctinfo);
    STDMETHODIMP         GetTypeInfo(UINT iTInfo, LCID lcid,
                                     ITypeInfo **ppTInfo);
    
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

    CAttrBag();
    ~CAttrBag();
    STDMETHODIMP Load( IPropertyBag *pBag, IPropertyBag2 *pBag2 );

private:
    DWORD    _cRef;
    VARIANT *_pAllArgs;
    LPTSTR  *_pAllNames;
    ULONG   _uNumArgs;
};

#endif

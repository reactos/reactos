#ifndef __PEERS_HXX__
#define __PEERS_HXX__

#define _hxx_
#include "htmlapp.hdl"

/////////////////////////////////////////////////////////////////////////////
// Memory meter support
MtExtern(CAppBehavior)

class CBitsCtx;

/////////////////////////////////////////////////////////////////////////////
// CBaseBehavior

class CBaseBehavior : public CBase, public IElementBehavior
{
    NO_COPY(CBaseBehavior);
    DECLARE_CLASS_TYPES(CBaseBehavior, CBase)

private:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(Mem))

public:
    CBaseBehavior() {}
	~CBaseBehavior()
	{
        if (_pSite)
            _pSite->Release();
	}

public:

    DECLARE_PLAIN_IUNKNOWN(CBaseBehavior)
    STDMETHOD(PrivateQueryInterface)(REFIID riid, void ** ppv);

    //IElementBehavior methods
    STDMETHOD(Init)(/*[in]*/ IElementBehaviorSite *pSite);
    STDMETHOD(Notify)(/*[in]*/ LONG lNotify, VARIANT * pVarNotify);
    STDMETHOD(Detach)() { return S_OK; };

protected:
    IElementBehaviorSite *  _pSite;
};

/////////////////////////////////////////////////////////////////////////////
// CAppBehavior
class CAppBehavior : public CBaseBehavior
{
#define _CAppBehavior_
    NO_COPY(CAppBehavior);
    DECLARE_CLASS_TYPES(CAppBehavior, CBaseBehavior)
    DECLARE_CLASSDESC_MEMBERS;

#include "htmlapp.hdl"

public:
    CAppBehavior();
    ~CAppBehavior();
    
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CAppBehavior))

    STDMETHOD(PrivateQueryInterface)(REFIID riid, void ** ppv);

    // per IElementBehavior
    STDMETHOD(Init)(/*[in]*/ IElementBehaviorSite *pSite);

    // per IDispatchEx
    STDMETHOD(GetDispID)(BSTR bstrName, DWORD grfdex, DISPID *pid);
    STDMETHOD(InvokeEx)(DISPID dispid,
                         LCID lcid,
                         WORD wFlags,
                         DISPPARAMS *pdispparams,
                         VARIANT *pvarResult,
                         EXCEPINFO *pexcepinfo,
                         IServiceProvider *pSrvProvider);
    
public:
    DWORD GetStyles();
    DWORD GetExtendedStyles();
    
private:
    void OnDwnChan();
    static void CALLBACK OnDwnChanCallback(void * pvObj, void * pvArg)
        { ((CAppBehavior *) pvArg)->OnDwnChan(); }

    CBitsCtx * _pBitsCtx;
};

HRESULT GetAttrValue(IHTMLElement *pElement, const TCHAR * pchAttrName, VARIANT *pVarOut);

#endif // __PEERS_HXX__


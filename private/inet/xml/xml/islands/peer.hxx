/*
 * @(#)Peer.hxx 1.0 2/27/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * xtag code for XML data islands
 * 
 */

#ifndef _XML_ISLANDS_PEER
#define _XML_ISLANDS_PEER


#include <mshtmhst.h>
#include "xmldso.hxx"
#include "events.hxx"

DEFINE_STRUCT(IHTMLElement);
DEFINE_STRUCT(IElementBehaviorSite);

typedef _reference<DOMDocumentWrapper> RDOMDocumentWrapper;

///////////////////////////////////////////////////////////////////////////////
//  class CXMLIslandPeer
//  a CXMLIslandPeer is an XML Document wrapped in an IElementBehavior interface
//
//  NOTE IUnknown is implemented by wrapper class, so we don't implement it here
//

#define MAX_EVENT_NAME_LENGTH   20

#define PEEREVENT_ONDATAAVAILABLE       0
#define PEEREVENT_NUMEVENTS             1  // Keep this in sync

class CXMLIslandPeer : 
    public _unknown<IElementBehavior, &IID_IElementBehavior>,
    public IDispatchEx,
    public CDocEventSink,
    public CElementEventSink
{
    typedef _unknown<IElementBehavior, &IID_IElementBehavior> super;
private:
    RDOMDocumentWrapper     _pDocWrapper;
    RIElementBehaviorSite   _PeerSite;      // IElementBehaviorSite
    RIHTMLElement           _HTMLElement;   // IHTMLElement
    CDocEventReceiver       *_pDocEvents;
    CPropChangeReceiver     *_pPropChange; 
    DWORD                   _dwAdviseCookie;
    LONG                    _rglEventCookies[PEEREVENT_NUMEVENTS]; // Currently we only have two events
    IElementBehaviorSiteOM  *_pEBOM;
    PCPNODE                 _pCPNodeList;
    RXMLDSO                 _pDSO;

    typedef struct tagEvent
    {
        WCHAR rgwchEventName[MAX_EVENT_NAME_LENGTH];
        LONG  lCookieNumber;
    } EVENTINFO;

    static EVENTINFO s_eventinfoTable[];

public:

    // Constructor
    CXMLIslandPeer(DSODocument* doc);

    virtual ~CXMLIslandPeer();

public:

    // IUnknown
    ULONG STDMETHODCALLTYPE AddRef( void)
    {
        return super::AddRef();
    }

    ULONG STDMETHODCALLTYPE Release( void)
    {
        return super::Release();
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID *ppvObject);

    // IDispatch overrides.
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount( 
        /* [out] */ UINT __RPC_FAR *pctinfo)
    {
        return getDispatch()->GetTypeInfoCount(pctinfo);
    }
    
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo( 
        /* [in] */ UINT iTInfo,
        /* [in] */ LCID lcid,
        /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo)
    {
        return getDispatch()->GetTypeInfo(iTInfo,lcid,ppTInfo);
    }
    
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames( 
        /* [in] */ REFIID riid,
        /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
        /* [in] */ UINT cNames,
        /* [in] */ LCID lcid,
        /* [size_is][out] */ DISPID __RPC_FAR *rgDispId)
    {
        TraceTag((tagDispatch, "GetIDsOfNames"));
        return getDispatch()->GetIDsOfNames(riid,rgszNames,cNames,lcid,rgDispId);
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
        return getDispatch()->Invoke(dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr);
    }

    // IDispatchEx overrides.
    HRESULT STDMETHODCALLTYPE GetDispID( 
        /* [in] */ BSTR bstrName,
        /* [in] */ DWORD grfdex,
        /* [out] */ DISPID __RPC_FAR *pid);
    
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
        return getDispatch()->InvokeEx(id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);
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

    // IElementBehavior interface
    //

    STDMETHOD(Init)(IElementBehaviorSite * pPeerSite);
    STDMETHOD(Notify)(LONG event, VARIANT * pVar);
    STDMETHOD(Detach)() { return S_OK; };

    //
    // CDocEventSink methods
    //
    HRESULT STDMETHODCALLTYPE onReadyStateChange();
    HRESULT STDMETHODCALLTYPE onDataAvailable();

    // 
    // CElementEventSink
    // 

    HRESULT STDMETHODCALLTYPE onSrcChange();

    //
    // Need to over-ride QI, so we need to implement the whole of IUnknown
    //

private:
    HRESULT InitXMLIslandPeer();
    HRESULT GetParsedText(BSTR *pbstrParsedText);
    HRESULT HookUpToConnection(BOOL fAdvise);
    HRESULT RegisterBehaviourEvents();

    DSODocument* getWrapped() { return (DSODocument*)(_pDocWrapper->getWrapped()); }
    IDispatchEx* getDispatch() { return ((IDispatchEx*)_pDocWrapper); }
};

#endif _XML_ISLANDS_PEER

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

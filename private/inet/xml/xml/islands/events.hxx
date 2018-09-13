/*
 * @(#)events.hxx 1.0 7/13/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * IDispatch object for receiving events from the tree
 * 
 */

#ifndef __EVENTS_HXX__
#define __EVENTS_HXX__

// Pure virtual base class.  To receive events, inherit from 
// this and pass in a pointer to yourself

class NOVTABLE CDocEventSink
{
public:
    virtual HRESULT STDMETHODCALLTYPE onReadyStateChange() = 0;
    virtual HRESULT STDMETHODCALLTYPE onDataAvailable() = 0;
};


class NOVTABLE CElementEventSink
{
public:
    virtual HRESULT STDMETHODCALLTYPE onSrcChange() = 0;
};


class NOVTABLE CEventReceiverBase : public IDispatch
{
protected:
    ULONG _cRef;
    IID _iid;

public:
    CEventReceiverBase(REFIID iid);
    virtual ~CEventReceiverBase();

    STDMETHODIMP QueryInterface(
        REFIID riid, 
        void **ppv);

    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    //IDispatch
    STDMETHODIMP GetTypeInfoCount(UINT* pctinfo)
    {
        return E_NOTIMPL;
    }
    
    
    STDMETHODIMP GetTypeInfo( UINT iTInfo,
        LCID lcid,
        ITypeInfo** ppTInfo)
    {
        return E_NOTIMPL;
    }
    
    
    STDMETHODIMP GetIDsOfNames(
        REFIID riid,
        LPOLESTR *rgszNames,
        UINT cNames,
        LCID lcid,
        DISPID *rgDispId)
    {
        return E_NOTIMPL;
    }

};

template <class EventSink>
class NOVTABLE CEventReceiver :public CEventReceiverBase
{
public:
    CEventReceiver(REFIID iid, EventSink *pSink) : 
        CEventReceiverBase(iid)
    {
        Assert (_pSink && "NULL Eventsink passed into CEventReceiver");
        _pSink = pSink;
    }

protected:
    EventSink *_pSink;

public:
};


class CDocEventReceiver : public CEventReceiver<CDocEventSink>
{
public:
    CDocEventReceiver(CDocEventSink *pSink) :
        CEventReceiver<CDocEventSink>(DIID_XMLDOMDocumentEvents, pSink)
    {
    }

    STDMETHODIMP Invoke(
        DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS  *pDispParams,
        VARIANT  *pVarResult,
        EXCEPINFO *pExcepInfo,
        UINT *puArgErr);
};


class CPropChangeReceiver : public CEventReceiver<CElementEventSink>
{
private:
    IHTMLWindow2 *_pHTMLWindow;

public:
    CPropChangeReceiver (CElementEventSink *pSink, IHTMLWindow2 *pHTMLWindow); 
    ~CPropChangeReceiver();

    STDMETHODIMP Invoke(
        DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS  *pDispParams,
        VARIANT  *pVarResult,
        EXCEPINFO *pExcepInfo,
        UINT *puArgErr);
};

HRESULT 
AttachEventHelper(
    IHTMLElement *pElement,
    IDispatch *pdispHandler,
    LPWSTR pwszEventName,
    BOOL fAttach);


HRESULT 
RegisterForElementEvents(
    IHTMLElement *pElem, 
    CPropChangeReceiver **ppEventDispObj, 
    CElementEventSink *pEventSink);

#endif // __EVENTS_HXX__
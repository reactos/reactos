//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       olecpc.cxx
//
//  Contents:   Connection point container implementation for COleSite.
//
//  History:
//              5-22-95     KFL     stubbed MAC functions/macros ForwardToOutSink and
//                                  SINK_METHOD
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_CONNECT_HXX_
#define X_CONNECT_HXX_
#include "connect.hxx"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

extern "C" const IID IID_IDATASRCListener;
extern "C" const IID IID_IDatasrcChangeEvents;

//+---------------------------------------------------------------------------
//
//  Member:     COleSiteCPC::COleSiteCPC
//
//  Synopsis:   ctor.  Makes a local copy of the CONNECTION_POINT_INFO,
//              inserting the control's source event interfaces.
//
//  Arguments:  [pOleSite]    -- The site.
//              [pUnkPrivate] -- The private unknown of the control.
//
//----------------------------------------------------------------------------

COleSiteCPC::COleSiteCPC(COleSite *pOleSite, IUnknown *  pUnkPrivate)
        :
        super(pOleSite, NULL)
{
    const CBase::CLASSDESC *        pclassdesc;
    const CONNECTION_POINT_INFO *   pcpi;
    long                            i;

    pclassdesc = pOleSite->BaseDesc();
    
    for (i = 0, pcpi = pclassdesc->_pcpi; pcpi && pcpi->piid; i++, pcpi++)
    {
        _acpi[i].piid = pcpi->piid;
        _acpi[i].dispid = pcpi->dispid;
    }

    //
    // If this assert fires, then the classdescs for things derived from
    // COleSite are messed up. (anandra)
    //
    
    Assert(i == 6 || _acpi[CPI_OFFSET_OLESITECONTROL].piid == NULL);                     
    
    //
    // Assert that the last entry in the acpi array is IID_NULL
    //

    if (_acpi[CPI_OFFSET_OLESITECONTROL].piid)
    {
        Assert(IsEqualIID(IID_NULL, *(_acpi[CPI_OFFSET_OLESITECONTROL].piid)));
        _acpi[CPI_OFFSET_OLESITECONTROL].piid = pOleSite->GetpIIDDispEvent();
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::EnsurePrivateSink
//
//  Synopsis:   Ensures that we are sinking events on the control's
//              primary event interface.
//
//----------------------------------------------------------------------------

// after COleSite::EnsureControlSink is executed, COleSiteEventSink OSES, which
// is allocated on stack, goes away. However, the sink is supposed to be alive as
// event will be firing in it, namely using it's vtable. The code below still works
// because the only thing used of OSES is it's vtable pointer, and the object does not
// have any state (data memebers) at all. Should COleSiteEventSink get any state, the code
// below should be changed such that OSES does not go away.

StartupAssert(sizeof(void *) == sizeof(COleSiteEventSink));

void
COleSite::EnsurePrivateSink()
{
    HRESULT             hr = S_OK;
    COleSiteEventSink   OSES;
    DWORD_PTR           dwSink;

    // If the control isn't yet created, or if it isn't verifiably safe, don't
    // connect the event sink.
	if (!_pUnkCtrl)
		return;

    SetEventsShouldFire();

    dwSink = *(DWORD_PTR *)&OSES;

    //
    // Assert that either we've not initialized the event sink yet
    // or if we have then the event sink vtable is the same as the old
    // event sink.
    //
    
    Assert(!_dwEventSinkIn || _dwEventSinkIn == dwSink);

    if (_dwEventSinkIn)
        return;
        
    _dwEventSinkIn |= dwSink;

    Assert(_pUnkCtrl);

    hr = THR(ConnectSink(
            _pUnkCtrl,
            *GetpIIDDispEvent(),
            (IUnknown *) &_dwEventSinkIn,
            NULL));
    if (!OK(hr))
    {
        //
        // Failed with primary iid, try IDispatch.
        //

        hr = THR(ConnectSink(
                _pUnkCtrl,
                IID_IDispatch,
                (IUnknown *) &_dwEventSinkIn,
                NULL));
    }
}


//+------------------------------------------------------------------------
//
//  Connection point sinks implementations.
//
//-------------------------------------------------------------------------

IMPLEMENT_FORMS_SUBOBJECT_IUNKNOWN(COleSiteEventSink, COleSite, _dwEventSinkIn)

//+---------------------------------------------------------------------------
//
//  Member:     COleSiteEventSink::QueryInterface
//
//  Synopsis:   Per IUnknown.
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSiteEventSink::QueryInterface(REFIID iid, void ** ppv)
{
    IID* pIIDDispEvent;

    if (MyCOleSite()->IllegalSiteCall(0))
        RRETURN(E_UNEXPECTED);

    if (!ppv)
        RRETURN(E_INVALIDARG);

    if (iid == IID_IUnknown  ||
        iid == IID_IDispatch)
    {
        *ppv = (IDispatch *) this;
    }
    else
    {
        pIIDDispEvent = MyCOleSite()->GetpIIDDispEvent();
        if (pIIDDispEvent && iid == *pIIDDispEvent)
        {
            *ppv = (IDispatch *) this;
        }
        else
        {
            *ppv = NULL;
            RRETURN_NOTRACE(E_NOINTERFACE);
        }
    }

    ((IUnknown *) *ppv)->AddRef();
    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Member:     COleSiteEventSink::Invoke
//
//  Synopsis:   Forwards to the connection point.
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSiteEventSink::Invoke(
        DISPID              dispidMember,
        REFIID              riid,
        LCID                lcid,
        WORD                wFlags,
        DISPPARAMS FAR*     pdispparams,
        VARIANT FAR*        pvarResult,
        EXCEPINFO FAR*      pexcepinfo,
        UINT FAR*           puArgErr)
{
    if (MyCOleSite()->IllegalSiteCall(0))
        RRETURN(E_UNEXPECTED);

    if (!MyCOleSite()->IsSafeToScript())
        return S_OK;

    CMessage        Message(NULL, 0, 0, 0);
    POINT           pt;
    CTreeNode *     pNodeParent;
    ITypeInfo *     pTIEvent;

    //
    // Refire event to any listening sinks out there.  Includes any hooked up
    // function pointers.
    //

    // Find the event name that corresponds to this event (used for fast event hookup).
    pTIEvent = MyCOleSite()->GetClassInfo()->_pTypeInfoEvents;

    IGNORE_HR(MyCOleSite()->CBase::FireEvent(
        dispidMember,
        dispidMember,
        pvarResult,
        pdispparams,
        pexcepinfo,
        puArgErr,
        pTIEvent));

    //
    // For the standard events, we will recalculate parameters to send
    // because arbitrary objects may use different coordinates than the
    // ones that we want.
    //

    switch (dispidMember)
    {
    case DISPID_CLICK:
    case DISPID_DBLCLICK:
        goto FireStdMessage;

    case DISPID_MOUSEMOVE:
        Message.message = WM_MOUSEMOVE;
        goto FireStdMessage;

    case DISPID_MOUSEDOWN:
        Message.message = 
            (Message.dwKeyState & MK_LBUTTON) ? WM_LBUTTONDOWN :
                (Message.dwKeyState & MK_RBUTTON) ? WM_RBUTTONDOWN : 
                    WM_MBUTTONDOWN;
        goto FireStdMessage;

    case DISPID_MOUSEUP:
        Message.message = 
            (Message.dwKeyState & MK_LBUTTON) ? WM_LBUTTONUP :
                (Message.dwKeyState & MK_RBUTTON) ? WM_RBUTTONUP : 
                    WM_MBUTTONUP;
        goto FireStdMessage;

FireStdMessage:
        Message.SetNodeHit( MyCOleSite()->GetFirstBranch() );
        GetCursorPos(&pt);
        ScreenToClient(MyCOleSite()->Doc()->_pInPlace->_hwnd, &pt);
        Message.pt = pt;
        Message.wParam = Message.dwKeyState;
        Message.lParam = MAKELPARAM(Message.pt.x, Message.pt.y);

        //
        // Find the first element parent which can fire events
        //
        
        pNodeParent = MyCOleSite()->GetFirstBranch()->Parent();
        
        if (pNodeParent)
        {
            if (DISPID_CLICK == dispidMember)
            {
                IGNORE_HR(pNodeParent->Element()->Fire_onclick(pNodeParent));
            }
            else if (DISPID_DBLCLICK == dispidMember)
            {
                IGNORE_HR(pNodeParent->Element()->Fire_ondblclick(pNodeParent));
            }
            else
            {
                IGNORE_HR(pNodeParent->Element()->FireStdEventOnMessage(pNodeParent, &Message, pNodeParent));
            }
        }
        break;
    }

    return S_OK;
}

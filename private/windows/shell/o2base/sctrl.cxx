//+---------------------------------------------------------------------
//
//   File:       sctrl.cxx
//
//------------------------------------------------------------------------

//[ srvr_overview
/*
                        Srvr Library Overview

The Srvr library is a set of C++ base classes intended to greatly simplify
the process of implementing an OLE Compound Document object DLL in C++.
This library requires the O2UTIL library and and understanding of elements
in that library is prerequisite to a complete understanding of these base
classes.  Consult documentation for the O2UTIL library.

The library consists of three C++ base classes: SrvrCtrl, SrvrDV, and
SrvrInPlace.  An OLE Compound Document object implemented using the Srvr
library is an aggregate of three subobjects -- the control, data/view
and in-place subobjects.  The implementations of these subobjects are C++
classes that inherit from SrvrCtrl, SrvrDV, and SrvrInPlace respectively.
Behaviour specific to the C.D. object type is implemented
by overriding virtual methods on the base classes.  The base classes are
designed so that a simple but functional server can be implemented by overriding
only a small number of virtual methods.  Progressively more advanced servers
can be implemented by deriving more virtual methods and adding support for
new interfaces in the derived classes.

In the following discussion, the unqualified term "object" refers to an OLE
Compound Document (C.D.) object.  The term "subobject" refers to a C++ object
whose class inherits from one of the Srvr base classes.

The data/view subobject encapsulates the persistent data of an object and the
rendering of that data.  This subobject supports the IDataObject, IViewObject,
and the IPersist family of interfaces.  These subobjects can also function
independently as a data transfer object for clipboard and drag-drop operations.

The control subobject manages the dynamic control of the object.  This subobject
supports the IOleObject interface and directs all state transitions of the object.

The in-place subobject is responsible for the child window and user interface
of an object while it is in-place active.  The subobject supports the
IOleInPlaceObject and IOleInPlaceActiveObject interfaces.  This subobject is
not required for objects that don't support in-place editing.

The control subobject controls the aggregate and holds pointers to the data/view
and inplace subobjects.  It maintains the reference count for the object as a
whole and delegates QueryInterfaces to the other subobjects for interfaces
that it does not handle.  The data/view and in-place subobjects each hold a
pointer to the control subobject.  They each forward their IUnknown methods
to the control.  When a data/view subobject is being used independently as
a data-transfer object then its control pointer is NULL.

For more information consult the overview sections for each of the base
classes and the documentation for the base class methods.

*/
//]

//[ srvrctrl_overview
/*
                        SrvrCtrl Overview

The SrvrCtrl base class implements the control aspects common to most
OLE Compound Document objects.  It records the state of the object and
directs the state transitions.  It implements the IOleObject interface.

An object is in one of five possible states: passive, loaded, in-place,
U.I. active, or opened.  An object is passive when it is holding no
resources.  This is true for objects that are newly created or have
been released.  An object becomes loaded when it gets
an IPersistXXX::Load or IPersistStorage::InitNew call and has loaded
or initialized the necessary part of its persistent state.  An object in
the in-place state has a child window in its containers window and
can receive window messages.  The object (nor any of its embeddings)
does not have its U.I. visible (e.g. shared menu or toolbars).
A U.I. active object does have its (or one of its embeddings) U.I. visible.
An open object is one that is being open-edited in a separate, top-level
window.

Part of implementing the control subobject of an OLE Compound Document object
is implementing verbs.  There are a number of standard, OLE-defined verbs
and an object can add its own.  Since the set of verbs is very object
dependent SrvrCtrl requires a derived class to supply tables indicating
the verbs that are supported.  One is table of OLEVERB structures giving
standard information about the verb including the verb number and name.
A parallel table contains a pointer for each verb pointing to a function
that implements that verb.  SrvrCtrl has a set of static methods that
implement the standard OLE verbs.  The derived class can include these methods
in its verb table.  SrvrCtrl implements all the IOleObject verb-related
methods using these two verb tables.  The verb tables must be in order
of verb number and must be contiguous (i.e. no missing verb numbers).

*/
//]

#include "headers.hxx"
#pragma hdrstop


//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::SrvrCtrl, protected
//
//  Synopsis:   Constructor for SrvrCtrl object
//
//  Notes:      To create a properly initialized object you must
//              call the Init method immediately after construction.
//
//---------------------------------------------------------------

SrvrCtrl::SrvrCtrl(void)
{
    DOUT(L"SrvrCtrl: Constructing\r\n");

    _pDV        = NULL;
    _pInPlace   = NULL;
    _pPrivUnkDV = NULL;
    _pPrivUnkIP = NULL;

     // site-related information
    _pClientSite = NULL;
    _pOleAdviseHolder = NULL;
    _pClass = NULL;

    _dwRegROT = 0;

    _lpstrCntrApp = NULL;
    _lpstrCntrObj = NULL;

    _state = OS_PASSIVE;

    EnableIPB(TRUE);
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::Init, protected
//
//  Synopsis:   Fully initializes a SrvrCtrl object
//
//  Arguments:  [pClass] -- The initialized class descriptor for the server
//              [pUnkOuter] -- The controlling unknown if this server is being
//                          created as part of an aggregate; NULL otherwise
//
//  Returns:    NOERROR if successful
//
//  Notes:      The class descriptor pointer is saved in the protected _pClass
//              member variable where it is accessible during the lifetime
//              of the object.
//
//---------------------------------------------------------------

HRESULT
SrvrCtrl::Init(LPCLASSDESCRIPTOR pClass)
{
    _pClass = pClass;
    return NOERROR;
}


//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::~SrvrCtrl, protected
//
//  Synopsis:   Destructor for the SrvrCtrl object
//
//  Notes:      The destructor is called as a result of the servers
//              reference count going to 0.  It ensure the object
//              is in a passive state and releases the data/view and inplace
//              subobjects objects.
//
//---------------------------------------------------------------

SrvrCtrl::~SrvrCtrl(void)
{
    DOUT(L"~~~~~SrvrCtrl::~OPCtrl\r\n");
    // note: we don't have to release _pDV and _pInPlace because
    // we should have released them right away (standard aggregation policy)
    // We must release the private unknowns of those two subobjects, though.

    if (_pPrivUnkIP)
        _pPrivUnkIP->Release();
    if (_pPrivUnkDV)
        _pPrivUnkDV->Release();

    // free our advise holder
    if(_pOleAdviseHolder != NULL)
        _pOleAdviseHolder->Release();

    // release our client site
    TaskFreeString(_lpstrCntrApp);
    TaskFreeString(_lpstrCntrObj);

    if (_pClientSite != NULL)
        _pClientSite->Release();

    DOUT(L"SrvrCtrl: Destructed\r\n");
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::TransitionTo, public
//
//  Synopsis:   Drives the transition of the object from one state to another
//
//  Arguments:  [state] -- the desired resulting state of the object
//
//  Returns:    Success iff the transition completed successfully.  On failure
//              the object will be in the original or some intermediate,
//              but consistent, state.
//
//  Notes:      There are eight direct state transitions.  These are:
//              between the passive and loaded states, between the
//              loaded and inplace states, between the inplace and U.I. active
//              states, and between the loaded and opened states.
//              Each of these direct transitions has an overridable method
//              that effects it.  The TransitionTo function implements
//              transitions between any two arbitrary states by calling
//              these direct transition methods in the proper order.
//
//---------------------------------------------------------------

HRESULT
SrvrCtrl::TransitionTo(OLE_SERVER_STATE state)
{
#if DBG
    OLECHAR achTemp[256];
    wsprintf(achTemp,L"[%d] --> [%d] SrvrCtrl::TransitionTo\n\r",(int)_state,(int)state);
    DOUT(achTemp);
#endif //DBG

    Assert(state >= OS_PASSIVE && state <= OS_OPEN);
    Assert(_state >= OS_PASSIVE && _state <= OS_OPEN);

    //
    // at each iteration we transition one state closer to
    // our destination state...
    //
    HRESULT hr = NOERROR;
    while (state != _state && OK(hr))
    {
        switch(_state)
        {
        case OS_PASSIVE:
            // from passive we can only go to loaded!
            if (OK(hr = PassiveToLoaded()))
                _state = OS_LOADED;

            break;

        case OS_LOADED:
            switch(state)
            {
            default:
                if (OK(hr = LoadedToRunning()))
                    _state = OS_RUNNING;
                break;

            case OS_PASSIVE:
                if (OK(hr = LoadedToPassive()))
                    _state = OS_PASSIVE;
                break;
            }
            break;

        case OS_RUNNING:
            switch(state)
            {
            default:
            case OS_LOADED:
                if (OK(hr = RunningToLoaded()))
                    _state = OS_LOADED;
                break;
            case OS_INPLACE:
            case OS_UIACTIVE:
                if (OK(hr = RunningToInPlace()))
                    _state = OS_INPLACE;
                break;
            case OS_OPEN:
                if (OK(hr = RunningToOpened()))
                    _state = OS_OPEN;
                break;
            }
            break;

        case OS_INPLACE:
            switch(state)
            {
            default:
                if (OK(hr = InPlaceToRunning()))
                {
                    //
                    // The following handles re-entrancy cases in which
                    // processing of this state transition caused us to
                    // reach a state below our current target state...
                    if(_state < OS_RUNNING)
                        goto LExit;

                    _state = OS_RUNNING;
                }
                break;
            case OS_UIACTIVE:
                if (OK(hr = InPlaceToUIActive()))
                    _state = OS_UIACTIVE;
                break;
            }
            break;

        case OS_UIACTIVE:
            // from UIActive we can only go to inplace
            if (OK(hr = UIActiveToInPlace()))
            {
                //
                // In the course of notifying the container that we
                // are no longer UIActive, it is possible that we
                // got InPlace deactivated (or worse, Closed).
                // If this happened we abort our currently targeted
                // transition...
                if(_state < OS_INPLACE)
                    goto LExit;

                _state = OS_INPLACE;
            }
            break;

        case OS_OPEN:
            // from Open we can only go to running
            if (OK(hr = OpenedToRunning()))
                _state = OS_RUNNING;
            break;
        }
    }

LExit:
#if DBG
    wsprintf(achTemp,L"SrvrCtrl::TransitionTo [%d] hr = %lx _state = %d\n\r",
            (int)state, hr, (int)_state);
    DOUT(achTemp);
#endif //DBG
    return hr;
}


//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::PassiveToLoaded, protected
//
//  Synopsis:   Effects the direct passive to loaded state transition
//
//  Returns:    Success iff the object is in the loaded state.  On failure
//              the object will be in a consistent passive state.
//
//  Notes:      The base class does not do any processing on this transition.
//
//---------------------------------------------------------------

HRESULT
SrvrCtrl::PassiveToLoaded(void)
{
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::LoadedToRunning, protected
//
//  Synopsis:   Effects the direct loaded to running state transition
//
//  Returns:    Success if the object is running.
//
//  Notes:      This transition occurs as a result of an
//              IRunnableObject::Run call (TBD) and is implicit in any
//              DoVerb call.
//
//---------------------------------------------------------------

HRESULT
SrvrCtrl::LoadedToRunning(void)
{
    DOUT(L"SrvrCtrl::LoadedToRunning\r\n");

    //
    // enter ourself in the Running Object Table
    //
    LPMONIKER pmk;
    if (OK(_pDV->GetMoniker(OLEGETMONIKER_ONLYIFTHERE, &pmk)))
    {
        RegisterAsRunning((LPUNKNOWN)this, pmk, &_dwRegROT);
        pmk->Release();
    }
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::LoadedToPassive, protected
//
//  Synopsis:   Effects the direct loaded to passive state transition
//
//  Returns:    Success if the object is loaded.
//
//  Notes:      This transition occurs as a result of an IOleObject::Close()
//              call.
//              This method sends an OnClose notification to all of our
//              advises.
//
//---------------------------------------------------------------

HRESULT
SrvrCtrl::LoadedToPassive(void)
{
    DOUT(L"SrvrCtrl::LoadedToPassive\r\n");

    // notify our data advise holders of 'stop'
    _pDV->OnDataChange(ADVF_DATAONSTOP);

    // notify our advise holders that we have closed
    if (_pOleAdviseHolder != NULL)
    {
        DOUT(L"SrvrCtrl::LoadedToPassive calling _pOleAdviseHolder->SendOnClose()\r\n");
        _pOleAdviseHolder->SendOnClose();
    }

    // forcibly cut off remoting clients???
    //CoDisconnectObject((LPUNKNOWN)this, 0);

    //
    // revoke our entry in the running object table
    //
    if (_dwRegROT != 0)
    {
        DOUT(L".-.-.SrvrCtrl::RunningToLoaded calling RevokeAsRunning\r\n");
        RevokeAsRunning(&_dwRegROT);
        _dwRegROT = 0;
    }

    DOUT(L"SrvrCtrl::LoadedToPassive (returning)\r\n");
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::RunningToLoaded, protected
//
//  Synopsis:   Effects the direct running to loaded state transition
//
//  Returns:    Success if the object is loaded.
//
//---------------------------------------------------------------

HRESULT
SrvrCtrl::RunningToLoaded(void)
{
    DOUT(L"SrvrCtrl::RunningToLoaded\r\n");

    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::RunningToOpened, protected
//
//  Synopsis:   Effects the direct running to opened state transition
//
//  Returns:    Success if the object is open-edited.
//
//  Notes:      Open-editing is not yet supported.  This returns E_FAIL.
//
//              The derived class MUST completely override this
//              transition to implement an open-ediing server!
//
//---------------------------------------------------------------

HRESULT
SrvrCtrl::RunningToOpened(void)
{
    DOUT(L"SrvrCtrl::RunningToOpened E_FAIL\r\n");
    return E_FAIL;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::OpenedToRunning, protected
//
//  Synopsis:   Effects the direct opened to running state transition
//
//  Returns:    Success if the open-editing session was shut down
//
//  Notes:      This occurs as the result of a DoVerb(HIDE...)
//
//---------------------------------------------------------------

HRESULT
SrvrCtrl::OpenedToRunning(void)
{
    // notify our container so it can un-hatch
    if (_pClientSite != NULL)
        _pClientSite->OnShowWindow(FALSE);

    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::RunningToInPlace, protected
//
//  Synopsis:   Effects the direct Running to inplace state transition
//
//  Returns:    Success iff the object is in the inplace state.  On failure
//              the object will be in a consistent Running state.
//
//  Notes:      This transition invokes the ActivateInPlace method on the
//              inplace subobject of the server, if there is one.  Containers
//              will typically override this method in order to additionally
//              inplace activate any inside-out embeddings that are visible.
//              If the server does not support in-place
//              activation then this method will return E_UNEXPECTED.
//
//---------------------------------------------------------------

HRESULT
SrvrCtrl::RunningToInPlace(void)
{
    if (_pInPlace == NULL)
    {
        DOUT(L"SrvrCtrl::RunningToInPlace E_FAIL\r\n");
        return E_FAIL;
    }

    return _pInPlace->ActivateInPlace(_pClientSite);
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::InPlaceToRunning, protected
//
//  Synopsis:   Effects the direct inplace to Running state transition
//
//  Returns:    Success under all but catastrophic circumstances.
//
//  Notes:      This transition invokes the DeactivateInPlace method on the
//              inplace subobject of the server, if there is one.  Containers
//              will typically override this method in order to additionally
//              inplace deactivate any inplace-active embeddings.
//              If the server does not support in-place activation then
//              this method will never be called.
//              This method is called as the result of a DoVerb(HIDE...)
//
//---------------------------------------------------------------

HRESULT
SrvrCtrl::InPlaceToRunning(void)
{
    Assert(_pInPlace != NULL);
    return _pInPlace->DeactivateInPlace();
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::InPlaceToUIActive, protected
//
//  Synopsis:   Effects the direct inplace to U.I. active state transition
//
//  Returns:    Success iff the object is in the U.I. active state.  On failure
//              the object will be in a consistent inplace state.
//
//  Notes:      This transition invokes the ActivateUI methods on the inplace
//              subobject of the server.
//              If the server does not support in-place activation then
//              this method will never be called.
//
//---------------------------------------------------------------

HRESULT
SrvrCtrl::InPlaceToUIActive(void)
{
    Assert(_pInPlace != NULL);
    return _pInPlace->ActivateUI();
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::UIActiveToInPlace, protected
//
//  Synopsis:   Effects the direct U.I. Active to inplace state transition
//
//  Returns:    Success under all but catastrophic circumstances.
//
//  Notes:      This transition invokes the DeactivateUI methods
//              on the inplace subobject of the server.  Containers
//              will typically override this method in order to possibly
//              U.I. deactivate a U.I. active embedding.
//              If the server does not support in-place activation then
//              this method will never be called.
//
//---------------------------------------------------------------

HRESULT
SrvrCtrl::UIActiveToInPlace(void)
{
    Assert(_pInPlace != NULL);
    return _pInPlace->DeactivateUI();
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::OnSave, public
//
//  Synopsis:   Raises the OnSave advise to any registered advises
//
//  Notes:      This method is called by the data/view subobject upon
//              successful completion of a save operation.
//
//---------------------------------------------------------------

void
SrvrCtrl::OnSave(void)
{
    if (_pOleAdviseHolder != NULL)
        _pOleAdviseHolder->SendOnSave();
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::DoShow, public
//
//  Synopsis:   Implementation of the standard verb OLEIVERB_SHOW
//
//  Arguments:  [pv] -- pointer to a SrvrCntrl object.
//              All other parameters are the same as the IOleObject::DoVerb
//              method.
//
//  Returns:    Success if the verb was successfully executed
//
//  Notes:      This and the other static Do functions are provided for
//              use in the server's verb table.  This verb results in
//              a ShowObject call on our container and a transition
//              to the U.I. active state
//
//---------------------------------------------------------------

HRESULT
SrvrCtrl::DoShow(LPVOID pv,
                LONG iVerb,
                LPMSG lpmsg,
                LPOLECLIENTSITE pActiveSite,
                LONG lindex,
                HWND hwndParent,
                LPCRECT lprcPosRect)
{
    LPSRVRCTRL pCtrl = (LPSRVRCTRL)pv;
    HRESULT hr = NOERROR;

    OLECHAR achTemp[256];
    wsprintf(achTemp,L"SrvrCtrl::DoShow [%d]\n\r",(int)pCtrl->State());
    DOUT(achTemp);

    if (pCtrl->_pClientSite != NULL)
    {
        pCtrl->_pClientSite->ShowObject();
        if(pCtrl->State() == OS_OPEN)
        {
            HWND hwnd = NULL;
            if(pCtrl->_pInPlace)
                pCtrl->_pInPlace->GetWindow(&hwnd);
            if(hwnd != NULL)
                SetForegroundWindow(hwnd);
        }
        else
        {
            hr = pCtrl->TransitionTo(OS_UIACTIVE);
        }
    }

    if (!OK(hr))
    {
        // the default action is OPEN...
        hr = pCtrl->TransitionTo(OS_OPEN);
    }

    // if the verb was unknown then return Unknown Verb error.
    if (OK(hr) && iVerb != OLEIVERB_PRIMARY && iVerb != OLEIVERB_SHOW)
    {
        DOUT(L"SrvrCtrl::DoShow returning OLEOBJ_S_INVALIDVERB\r\n");
        hr = OLEOBJ_S_INVALIDVERB;
    }

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::DoOpen, public
//
//  Synopsis:   Implementation of the standard verb OLEIVERB_OPEN.
//              This verb results in a transition to the open state.
//
//---------------------------------------------------------------

HRESULT
SrvrCtrl::DoOpen(LPVOID pv,
                LONG iVerb,
                LPMSG lpmsg,
                LPOLECLIENTSITE pActiveSite,
                LONG lindex,
                HWND hwndParent,
                LPCRECT lprcPosRect)
{
    DOUT(L"SrvrCtrl::DoOpen\r\n");

    LPSRVRCTRL pCtrl = (LPSRVRCTRL)pv;
    if(pCtrl->State() == OS_OPEN)
    {
        HWND hwnd = NULL;
        if(pCtrl->_pInPlace)
            pCtrl->_pInPlace->GetWindow(&hwnd);
        if(hwnd != NULL)
            SetForegroundWindow(hwnd);
    }
    return pCtrl->TransitionTo(OS_OPEN);
}

//+---------------------------------------------------------------
//
//  Member: SrvrCtrl::DoHide, public
//
//  Synopsis:   Implementation of the standard verb OLEIVERB_HIDE
//              This verb results in a transition to the Running state.
//
//---------------------------------------------------------------

HRESULT
SrvrCtrl::DoHide(LPVOID pv,
                LONG iVerb,
                LPMSG lpmsg,
                LPOLECLIENTSITE pActiveSite,
                LONG lindex,
                HWND hwndParent,
                LPCRECT lprcPosRect)
{
    DOUT(L"SrvrCtrl::DoHide\r\n");

    LPSRVRCTRL pCtrl = (LPSRVRCTRL)pv;
    if(pCtrl != NULL)
    {
//        if(pCtrl->_state == OS_LOADED || pCtrl->_state == OS_PASSIVE)
//            return pCtrl->TransitionTo(OS_PASSIVE);
//        else
            return pCtrl->TransitionTo(OS_RUNNING);
    }
    else
    {
        DOUT(L"SrvrCtrl::DoHide E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::DoUIActivate, public
//
//  Synopsis:   Implementation of the standard verb OLEIVERB_UIACTIVATE
//              This verb results in a transition to the U.I. active state.
//
//---------------------------------------------------------------

HRESULT
SrvrCtrl::DoUIActivate(LPVOID pv,
                        LONG iVerb,
                        LPMSG lpmsg,
                        LPOLECLIENTSITE pActiveSite,
                        LONG lindex,
                        HWND hwndParent,
                        LPCRECT lprcPosRect)
{
    DOUT(L"SrvrCtrl::DoUIActivate\r\n");

    LPSRVRCTRL pCtrl = (LPSRVRCTRL)pv;
    HRESULT hr = pCtrl->TransitionTo(OS_UIACTIVE);
    if(OK(hr) && (lpmsg != NULL))
    {
        PostMessage(pCtrl->_pInPlace->WindowHandle(),
                lpmsg->message,
                lpmsg->wParam,
                lpmsg->lParam);
    }
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::DoInPlaceActivate, public
//
//  Synopsis:   Implementation of the standard verb OLEIVERB_INPLACEACTIVATE
//              This verb results in a transition to the inplace state.
//
//---------------------------------------------------------------

HRESULT
SrvrCtrl::DoInPlaceActivate(LPVOID pv,
                            LONG iVerb,
                            LPMSG lpmsg,
                            LPOLECLIENTSITE pActiveSite,
                            LONG lindex,
                            HWND hwndParent,
                            LPCRECT lprcPosRect)
{
    DOUT(L"SrvrCtrl::DoInPlaceActivate\r\n");

    LPSRVRCTRL pCtrl = (LPSRVRCTRL)pv;
    HRESULT hr = pCtrl->TransitionTo(OS_INPLACE);
    if(OK(hr) && (lpmsg != NULL))
    {
        PostMessage(pCtrl->_pInPlace->WindowHandle(),
                lpmsg->message,
                lpmsg->wParam,
                lpmsg->lParam);
    }
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::SetClientSite, public
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method saves the client site pointer in the
//              _pClientSite member variable.
//
//              We never fail!
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrCtrl::SetClientSite(LPOLECLIENTSITE pClientSite)
{
    DOUT(L"SrvrCtrl::SetClientSite\r\n");

    //
    // if we already have a client site then release it
    //
    if (_pClientSite != NULL)
        _pClientSite->Release();

    //
    // if there is a new client site then hold on to it
    //
    _pClientSite = pClientSite;
    if (_pClientSite != NULL)
        _pClientSite->AddRef();

    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::GetClientSite
//
//  Synopsis:   Method of IOleObject interface
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrCtrl::GetClientSite(LPOLECLIENTSITE FAR* ppClientSite)
{
    if (ppClientSite == NULL)
    {
        DOUT(L"SrvrCtrl::GetClientSite E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }
    *ppClientSite = NULL;   // set out params to NULL

    //
    // if we have a client site then return it, but addref it first.
    //
    if (_pClientSite != NULL)
        (*ppClientSite = _pClientSite)->AddRef();

    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::SetHostNames
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method saves copies of the container-related strings
//              in the _lpstrCntrApp and _lpstrCntrObj member variables.
//
//              We never fail!
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrCtrl::SetHostNames(LPCWSTR lpstrCntrApp, LPCWSTR lpstrCntrObj)
{
    // free any strings we are holding on to
    if (_lpstrCntrApp != NULL)
    {
        TaskFreeString(_lpstrCntrApp);
        _lpstrCntrApp = NULL;
    }
    if (_lpstrCntrObj != NULL)
    {
        TaskFreeString(_lpstrCntrObj);
        _lpstrCntrObj = NULL;
    }

    // make copies of the new strings and hold on
    TaskAllocString(lpstrCntrApp, &_lpstrCntrApp);
    TaskAllocString(lpstrCntrObj, &_lpstrCntrObj);

    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::Close
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method ensures the object is in the passive state
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrCtrl::Close(DWORD dwSaveOption)
{
    DOUT(L"SrvrCtrl::Close\r\n");

    HRESULT hr = NOERROR;

    // if our object is dirty then we should save it, depending on the
    // save options
    if (_pClientSite != NULL && NOERROR == _pDV->IsDirty())
    {
        BOOL fSave;
        switch(dwSaveOption)
        {
        case OLECLOSE_SAVEIFDIRTY:
            fSave = TRUE;
            break;

        case OLECLOSE_NOSAVE:
            fSave = FALSE;
            break;

        case OLECLOSE_PROMPTSAVE:
            {
                // put up a message box asking the user if they want to update
                // the container
                LPWSTR lpstrObj =
                    _pDV->GetMonikerDisplayName(OLEGETMONIKER_ONLYIFTHERE);
                if (lpstrObj == NULL)
                {
                    lpstrObj = _pClass->_szUserClassType[USERCLASSTYPE_FULL];
                }

                int i = MessageBox(NULL,
                            L"Object has changed.  Update?",
                            lpstrObj,
                            MB_YESNOCANCEL | MB_ICONQUESTION);

                if (IDCANCEL == i)
                {
                    return OLE_E_PROMPTSAVECANCELLED;
                }

                fSave = (IDYES == i);
            }
            break;

        default:
            DOUT(L"SrvrCtrl::Close E_INVALIDARG\r\n");
            return E_INVALIDARG;
        }

        if (fSave)
            hr = _pClientSite->SaveObject();
    }

    //BUGBUG we are losing the SaveObject return value!
    hr = TransitionTo(OS_PASSIVE);

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::SetMoniker
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method notifies our data/view subobject of our new
//              moniker.  It also registers us in the running object
//              table and sends an OnRename notification to all registered
//              advises.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrCtrl::SetMoniker(DWORD dwWhichMoniker, LPMONIKER pmk)
{
    DOUT(L"SrvrCtrl::SetMoniker\r\n");

    // our moniker has changed so revoke our entry in the running object table
    if (_dwRegROT != 0)
    {
        RevokeAsRunning(&_dwRegROT);
    }

    //
    // insure that we have a full moniker to register in the ROT
    // if we have a full moniker, then go with it
    // otherwise ask our client site for a full moniker
    //
    HRESULT hr = NOERROR;
    LPMONIKER pmkFull = NULL;
    if (dwWhichMoniker == OLEWHICHMK_OBJFULL)
    {
        if((pmkFull = pmk) != NULL)
            pmkFull->AddRef();
    }
    else
    {
        if (_pClientSite == NULL)
        {
            DOUT(L"SrvrCtrl::SetMoniker E_FAIL\r\n");
            hr = E_FAIL;
        }
        else
        {
            hr = _pClientSite->GetMoniker(OLEGETMONIKER_ONLYIFTHERE,
                    OLEWHICHMK_OBJFULL,
                    &pmkFull);
        }
    }

    if (OK(hr))
    {
        // stow the moniker away in our data object
        _pDV->SetMoniker(pmkFull);

        if(pmkFull != NULL)
        {
            // register ourself in the running object table
            // NOTE: if we had native data items that weren't embeddings
            // but could be pseudo-objects then we would also register a
            // wildcard moniker
            //
            RegisterAsRunning((LPUNKNOWN)this, pmkFull, &_dwRegROT);

            // notify our advise holders that we have been renamed!
            if (_pOleAdviseHolder != NULL)
            {
                _pOleAdviseHolder->SendOnRename(pmkFull);
            }

            pmkFull->Release();
        }
        else
        {
            RevokeAsRunning(&_dwRegROT);
        }
    }
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::GetMoniker
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method forwards the request to our client site
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrCtrl::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER FAR* ppmk)
{
    DOUT(L"SrvrCtrl::GetMoniker\r\n");

    if (ppmk == NULL)
    {
        DOUT(L"SrvrCtrl::GetMoniker E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }

    *ppmk = NULL;   // set out parameters to NULL

    // get the requested moniker from our client site
    HRESULT hr;
    if (_pClientSite == NULL)
    {
        DOUT(L"SrvrCtrl::GetMoniker MK_E_UNAVAILABLE\r\n");
        hr = MK_E_UNAVAILABLE;
    }
    else
    {
        DOUT(L"SrvrCtrl::GetMoniker (calling _pClientSite->GetMoniker)\r\n");
        hr = _pClientSite->GetMoniker(dwAssign, dwWhichMoniker, ppmk);
    }

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::InitFromData
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method returns S_FALSE indicating InitFromData
//              is not supported.  Servers that wish to support initialization
//              from a selection should override this function.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrCtrl::InitFromData(LPDATAOBJECT pDataObject,
                        BOOL fCreation,
                        DWORD dwReserved)
{
    DOUT(L"SrvrCtrl::InitFromData\r\n");

    return S_FALSE;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::GetClipboardData
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method uses the GetClipboardCopy method on our
//              data/view subobject to obtain a data transfer object
//              representing a snapshot of our compound document object.
//
//              This is considered an exotic, and OPTIONAL method to
//              implement. It was intended for programatic access
//              (bypassing the clipboard), and will probably never
//              be used...
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrCtrl::GetClipboardData(DWORD dwReserved, LPDATAOBJECT FAR* ppDataObject)
{
    DOUT(L"SrvrCtrl::GetClipboardData\r\n");

    if (ppDataObject == NULL)
    {
        DOUT(L"SrvrCtrl::GetClipboardData E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }
    *ppDataObject = NULL;               // set out params to NULL

    // create a new data object initialized from our own data object
    LPSRVRDV pDV = NULL;
    HRESULT hr;
    if (OK(hr = _pDV->GetClipboardCopy(&pDV)))
    {
        *ppDataObject = (LPDATAOBJECT)pDV;
    }

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::DoVerb
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method locates the requested verb in the servers
//              verb table and calls the associated verb function.
//              If the verb is not found then the first (at index 0) verb in
//              the verb table is called.  This should be the primary verb
//              with verb number 0.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrCtrl::DoVerb(LONG iVerb,
                LPMSG lpmsg,
                LPOLECLIENTSITE pActiveSite,
                LONG lindex,
                HWND hwndParent,
                LPCRECT lprcPosRect)
{
    OLECHAR achTemp[256];
    wsprintf(achTemp,L"SrvrCtrl::DoVerb [%ld]\n\r",lindex);
    DOUT(achTemp);

    //
    // All DoVerbs make an implicit transition to the running state if
    // the object is not already running...
    //
    HRESULT hr = NOERROR;
    if (_state < OS_RUNNING)
        hr = TransitionTo(OS_RUNNING);

    if (OK(hr))
    {
        //
        // find the verb in the verb table.  if it is not there then default
        // to the 0th entry in the table (should be primary verb)
        //
        for (int i = 0; i < _pClass->_cVerbTable; i++)
        {
            if (iVerb == _pClass->_pVerbTable[i].lVerb)
            {
                break;
            }
        }
        if (i >= _pClass->_cVerbTable)
        {
            i = 0;
        }

        // dispatch the verb
        hr = (*_pVerbFuncs[i])(this,
                iVerb,
                lpmsg,
                pActiveSite,
                lindex,
                hwndParent,
                lprcPosRect);
    }

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::EnumVerbs
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method produces an enumerator over the server's
//              verb table.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrCtrl::EnumVerbs(LPENUMOLEVERB FAR* ppenum)
{
    DOUT(L"SrvrCtrl::EnumVerbs\r\n");

    if (ppenum == NULL)
    {
        DOUT(L"SrvrCtrl::EnumVerbs E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }

    *ppenum = NULL;

    //return OLE_S_USEREG;
    return CreateOLEVERBEnum(_pClass->_pVerbTable, _pClass->_cVerbTable, ppenum);
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::Update
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method returns NOERROR indicating that the update was
//              successful.  Containers will wish to override this function
//              in order to recursively pass the function on to all embeddings.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrCtrl::Update(void)
{
    DOUT(L"SrvrCtrl::Update\r\n");

    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::IsUpToDate
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method returns NOERROR indicating that the object is
//              up to date.  Containers will wish to override this function
//              in order to recursively pass the function on to all embeddings.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrCtrl::IsUpToDate(void)
{
    DOUT(L"SrvrCtrl::IsUpToDate\r\n");

    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::GetUserClassID
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method supplies the class id from the server's
//              CLASSDESCRIPTOR structure
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrCtrl::GetUserClassID(CLSID FAR* pClsid)
{
    DOUT(L"SrvrCtrl::GetUserClassID\r\n");

    if (pClsid == NULL)
    {
        DOUT(L"SrvrCtrl::GetUserClassID E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }
    *pClsid = _pClass->_clsid;
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::GetUserType
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method supplies the user type string from the server's
//              CLASSDESCRIPTOR structure
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrCtrl::GetUserType(DWORD dwFormOfType, LPWSTR FAR* plpstr)
{
    DOUT(L"SrvrCtrl::GetUserType\r\n");

    if (plpstr == NULL || dwFormOfType < 1 || dwFormOfType > 3)
    {
        DOUT(L"SrvrCtrl::GetUserType E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }

    return TaskAllocString(_pClass->_szUserClassType[dwFormOfType], plpstr);
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::SetExtent
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method is forwarded to the SetExtent method on
//              the data/view subobject.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrCtrl::SetExtent(DWORD dwDrawAspect, LPSIZEL lpsizel)
{
    DOUT(L"SrvrCtrl::SetExtent\r\n");

    if (lpsizel == NULL)
    {
        DOUT(L"SrvrCtrl::SetExtent E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }
    return _pDV->SetExtent(dwDrawAspect, *lpsizel);
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::GetExtent
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method is forwarded to the SetExtent method on
//              the data/view subobject.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrCtrl::GetExtent(DWORD dwDrawAspect, LPSIZEL lpsizel)
{
    DOUT(L"SrvrCtrl::GetExtent\r\n");

    if (lpsizel == NULL)
    {
        DOUT(L"SrvrCtrl::GetExtent E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }
    return _pDV->GetExtent(dwDrawAspect, lpsizel);
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::Advise
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method is implemented using the standard
//              OLE advise holder.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrCtrl::Advise(IAdviseSink FAR* pAdvSink, DWORD FAR* pdwConnection)
{
    DOUT(L"SrvrCtrl::Advise\r\n");

    if (pdwConnection == NULL)
    {
        DOUT(L"SrvrCtrl::Advise E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }
    *pdwConnection = NULL;              // set out params to NULL

    HRESULT hr = NOERROR;
    if (_pOleAdviseHolder == NULL)
        hr = CreateOleAdviseHolder(&_pOleAdviseHolder);

    if (OK(hr))
        hr = _pOleAdviseHolder->Advise(pAdvSink, pdwConnection);

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::Unadvise
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method is implemented using the standard
//              OLE advise holder.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrCtrl::Unadvise(DWORD dwConnection)
{
    DOUT(L"SrvrCtrl::Unadvise\r\n");

    if (_pOleAdviseHolder == NULL)
        return NOERROR;

    return _pOleAdviseHolder->Unadvise(dwConnection);
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::EnumAdvise
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method is implemented using the standard
//              OLE advise holder.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrCtrl::EnumAdvise(LPENUMSTATDATA FAR* ppenumAdvise)
{
    DOUT(L"SrvrCtrl::EnumAdvise\r\n");

    if (ppenumAdvise == NULL)
    {
        DOUT(L"SrvrCtrl::EnumAdvise E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }

    HRESULT hr;
    if (_pOleAdviseHolder == NULL)
    {
        *ppenumAdvise = NULL;
        hr = NOERROR;
    }
    else
    {
        hr = _pOleAdviseHolder->EnumAdvise(ppenumAdvise);
    }

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::GetMiscStatus
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method supplies the misc status flags from the server's
//              CLASSDESCRIPTOR structure
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrCtrl::GetMiscStatus(DWORD dwAspect, DWORD FAR* pdwStatus)
{
    DOUT(L"SrvrCtrl::GetMiscStatus\r\n");

    if (pdwStatus == NULL)
    {
        DOUT(L"SrvrCtrl::GetMiscStatus E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }
    *pdwStatus = _pClass->_dwMiscStatus;
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrCtrl::SetColorScheme
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      Servers should override this method if they are
//              interested in the palette.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrCtrl::SetColorScheme(LPLOGPALETTE lpLogpal)
{
    DOUT(L"SrvrCtrl::SetColorScheme E_NOTIMPL\r\n");
    return E_NOTIMPL;   //will we ever?
}

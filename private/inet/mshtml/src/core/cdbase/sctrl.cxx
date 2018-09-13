//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  Contents:   CServer implementation (partial).
//
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_CONNECT_HXX_
#define X_CONNECT_HXX_
#include "connect.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_OLECTL_H_
#define X_OLECTL_H_
#include <olectl.h>
#endif

#ifdef _MAC
#ifndef X_MACCONTROLS_H_
#define X_MACCONTROLS_H_
#include "maccontrols.h"
#endif
#endif

DeclareTag(tagOleState, "Server", "OLE state transitions")
PerfDbgExtern(tagPerfWatch)
ExternTag(tagDisableLockAR);

BEGIN_TEAROFF_TABLE(CServer, IOleInPlaceObjectWindowless)
    TEAROFF_METHOD(CServer, GetWindow, getwindow, (HWND*))
    TEAROFF_METHOD(CServer, ContextSensitiveHelp, contextsensitivehelp, (BOOL fEnterMode))
    TEAROFF_METHOD(CServer, InPlaceDeactivate, inplacedeactivate, ())
    TEAROFF_METHOD(CServer, UIDeactivate, uideactivate, ())
    TEAROFF_METHOD(CServer, SetObjectRects, setobjectrects, (LPCOLERECT lprcPosRect, LPCOLERECT lprcClipRect))
    TEAROFF_METHOD(CServer, ReactivateAndUndo, reactivateandundo, ())
    TEAROFF_METHOD(CServer, OnWindowMessage, onwindowmessage, (UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* plResult))
    TEAROFF_METHOD(CServer, GetDropTarget, getdroptarget, (IDropTarget** ppDropTarget))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CServer, IOleDocumentView)
    TEAROFF_METHOD(CServer, SetInPlaceSite, setinplacesite, (LPOLEINPLACESITE pIPSite))
    TEAROFF_METHOD(CServer, GetInPlaceSite, getinplacesite, (LPOLEINPLACESITE* ppIPSite))
    TEAROFF_METHOD(CServer, GetDocument, getdocument, (IUnknown** ppUnk))
    TEAROFF_METHOD(CServer, SetRect, setrect, (LPRECT lprcView))
    TEAROFF_METHOD(CServer, GetRect, getrect, (LPRECT lprcView))
    TEAROFF_METHOD(CServer, SetRectComplex, setrectcomplex, (LPRECT lprcView, LPRECT lprcHScroll, LPRECT lprcVScroll, LPRECT lprcSizeBox))
    TEAROFF_METHOD(CServer, Show, show, (BOOL fShow))
    TEAROFF_METHOD(CServer, UIActivate, uiactivate, (BOOL fActivate))
    TEAROFF_METHOD(CServer, Open, open, ())
    TEAROFF_METHOD(CServer, CloseView, closeview, (DWORD dwReserved))
    TEAROFF_METHOD(CServer, SaveViewState, saveviewstate, (IStream* pStm))
    TEAROFF_METHOD(CServer, ApplyViewState, applyviewstate, (IStream* pStm))
    TEAROFF_METHOD(CServer, Clone, clone, (IOleInPlaceSite* pNewIPSite, IOleDocumentView** ppNewView))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CServer, IOleInPlaceActiveObject)
    TEAROFF_METHOD(CServer, GetWindow, getwindow, (HWND*))
    TEAROFF_METHOD(CServer, ContextSensitiveHelp, contextsensitivehelp, (BOOL fEnterMode))
    TEAROFF_METHOD(CServer, TranslateAccelerator, translateaccelerator, (LPMSG lpmsg))
    TEAROFF_METHOD(CServer, OnFrameWindowActivate, onframewindowactivate, (BOOL fActivate))
    TEAROFF_METHOD(CServer, OnDocWindowActivate, ondocwindowactivate, (BOOL fActivate))
    TEAROFF_METHOD(CServer, ResizeBorder, resizeborder, ( LPCOLERECT lprectBorder, LPOLEINPLACEUIWINDOW lpUIWindow, BOOL fFrameWindow))
    TEAROFF_METHOD(CServer, EnableModeless, enablemodeless, (BOOL fEnable))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CServer, IOleObject)
    TEAROFF_METHOD(CServer, SetClientSite, setclientsite, (LPOLECLIENTSITE pClientSite))
    TEAROFF_METHOD(CServer, GetClientSite, getclientsite, (LPOLECLIENTSITE FAR* ppClientSite))
    TEAROFF_METHOD(CServer, SetHostNames, sethostnames, (LPCTSTR szContainerApp, LPCTSTR szContainerObj))
    TEAROFF_METHOD(CServer, Close, close, (DWORD dwSaveOption))
    TEAROFF_METHOD(CServer, SetMoniker, setmoniker, (DWORD dwWhichMoniker, LPMONIKER pmk))
    TEAROFF_METHOD(CServer, GetMoniker, getmoniker, (DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER FAR* ppmk))
    TEAROFF_METHOD(CServer, InitFromData, initfromdata, (LPDATAOBJECT pDataObject, BOOL fCreation, DWORD dwReserved))
    TEAROFF_METHOD(CServer, GetClipboardData, getclipboarddata, (DWORD dwReserved, LPDATAOBJECT * ppDataObject))
    TEAROFF_METHOD(CServer, DoVerb, doverb, (LONG iVerb, LPMSG lpmsg, LPOLECLIENTSITE pActiveSite, LONG lindex, HWND hwndParent, LPCOLERECT lprcPosRect))
    TEAROFF_METHOD(CServer, EnumVerbs, enumverbs, (LPENUMOLEVERB FAR* ppenumOleVerb))
    TEAROFF_METHOD(CServer, Update, update, ())
    TEAROFF_METHOD(CServer, IsUpToDate, isuptodate, ())
    TEAROFF_METHOD(CServer, GetUserClassID, getuserclassid, (CLSID FAR* pClsid))
    TEAROFF_METHOD(CServer, GetUserType, getusertype, (DWORD dwFormOfType, LPTSTR FAR* pszUserType))
    TEAROFF_METHOD(CServer, SetExtent, setextent, (DWORD dwDrawAspect, LPSIZEL lpsizel))
    TEAROFF_METHOD(CServer, GetExtent, getextent, (DWORD dwDrawAspect, LPSIZEL lpsizel))
    TEAROFF_METHOD(CServer, Advise, advise, (IAdviseSink FAR* pAdvSink, DWORD FAR* pdwConnection))
    TEAROFF_METHOD(CServer, Unadvise, unadvise, (DWORD dwConnection))
    TEAROFF_METHOD(CServer, EnumAdvise, enumadvise, (LPENUMSTATDATA FAR* ppenumAdvise))
    TEAROFF_METHOD(CServer, GetMiscStatus, getmiscstatus, (DWORD dwAspect, DWORD FAR* pdwStatus))
    TEAROFF_METHOD(CServer, SetColorScheme, setcolorscheme, (LPLOGPALETTE lpLogpal))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CServer, IOleControl)
    TEAROFF_METHOD(CServer, GetControlInfo, getcontrolinfo, (CONTROLINFO * pCI))
    TEAROFF_METHOD(CServer, OnMnemonic, onmnemonic, (LPMSG pMsg))
    TEAROFF_METHOD(CServer, OnAmbientPropertyChange, onambientpropertychange, (DISPID dispid))
    TEAROFF_METHOD(CServer, FreezeEvents, freezeevents, (BOOL fFreeze))
END_TEAROFF_TABLE()

#ifdef FANCY_CONNECTION_STUFF
BEGIN_TEAROFF_TABLE(CServer, IRunnableObject)
    TEAROFF_METHOD(CServer, GetRunningClass, getrunningclass, (LPCLSID lpclsid))
    TEAROFF_METHOD(CServer, Run, run, (LPBINDCTX pbc))
    TEAROFF_METHOD_(CServer, IsRunning, isrunning, BOOL, ())
    TEAROFF_METHOD(CServer, LockRunning, lockrunning, (BOOL fLock, BOOL fLastUnlockCloses))
    TEAROFF_METHOD(CServer, SetContainedObject, setcontainedobject, (BOOL fContained))
END_TEAROFF_TABLE()
#endif

#ifdef FANCY_CONNECTION_STUFF
BEGIN_TEAROFF_TABLE(CServer, IExternalConnection)
    TEAROFF_METHOD_(CServer, AddConnection, addconnection, DWORD, (DWORD extconn, DWORD reserved))
    TEAROFF_METHOD_(CServer, ReleaseConnection, releaseconnection, DWORD, (DWORD extconn, DWORD reserved, BOOL fLastReleaseCloses))
END_TEAROFF_TABLE()
#endif

BEGIN_TEAROFF_TABLE(CServer, IPersistStreamInit)
    TEAROFF_METHOD(CServer, GetClassID, getclassid, (LPCLSID lpClassID))
    TEAROFF_METHOD(CServer, IsDirty, isdirty, ())
    TEAROFF_METHOD(CServer, Load, LOAD, (LPSTREAM pStm))
    TEAROFF_METHOD(CServer, Save, SAVE, (LPSTREAM pStm, BOOL fClearDirty))
    TEAROFF_METHOD(CServer, GetSizeMax, GETSIZEMAX, (ULARGE_INTEGER FAR * pcbSize))
    TEAROFF_METHOD(CServer, InitNew, initnew, ())
END_TEAROFF_TABLE()

//BEGIN_TEAROFF_TABLE(CServer, IPersistStorage)
//    TEAROFF_METHOD(GetClassID, (LPCLSID lpClassID))
//    TEAROFF_METHOD(IsDirty, ())
//    TEAROFF_METHOD(InitNew, (LPSTORAGE  pStg))
//    TEAROFF_METHOD(Load, (LPSTORAGE  pStg))
//    TEAROFF_METHOD(Save, (LPSTORAGE  pStgSave, BOOL fSameAsLoad))
//    TEAROFF_METHOD(SaveCompleted, (LPSTORAGE  pStgNew))
//    TEAROFF_METHOD(HandsOffStorage, ())
//END_TEAROFF_TABLE()

//BEGIN_TEAROFF_TABLE(CServer, IPersistPropertyBag)
//    TEAROFF_METHOD(GetClassID, (LPCLSID lpClassID))
//    TEAROFF_METHOD(InitNew, ())
//    TEAROFF_METHOD(Load, (LPPROPERTYBAG pBag, LPERRORLOG pErrLog))
//    TEAROFF_METHOD(Save, (LPPROPERTYBAG pBag, BOOL fClearDirty, BOOL fSaveAllProperties))
//END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CServer, IDataObject)
    TEAROFF_METHOD(CServer, GetData, getdata, (LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium))
    TEAROFF_METHOD(CServer, GetDataHere, getdatahere, (LPFORMATETC pformatetc, LPSTGMEDIUM pmedium))
    TEAROFF_METHOD(CServer, QueryGetData, querygetdata, (LPFORMATETC pformatetc))
    TEAROFF_METHOD(CServer, GetCanonicalFormatEtc, getcanonicalformatetc, (LPFORMATETC pformatetc, LPFORMATETC pformatetcOut))
    TEAROFF_METHOD(CServer, SetData, setdata, (LPFORMATETC pformatetc, LPSTGMEDIUM pmedium, BOOL fRelease))
    TEAROFF_METHOD(CServer, EnumFormatEtc, enumformatetc, (DWORD dwDirection, LPENUMFORMATETC FAR* ppenum))
    TEAROFF_METHOD(CServer, DAdvise, dadvise, (FORMATETC FAR* pFormatetc, DWORD advf, LPADVISESINK pAdvSink, DWORD FAR* pdwConnection))
    TEAROFF_METHOD(CServer, DUnadvise, dunadvise, (DWORD dwConnection))
    TEAROFF_METHOD(CServer, EnumDAdvise, enumdadvise, (LPENUMSTATDATA FAR* ppenumAdvise))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CServer, IOleDocument)
    TEAROFF_METHOD(CServer, CreateView, createview, (IOleInPlaceSite * pIPSite, IStream * pStm, DWORD dwReserved, IOleDocumentView ** ppView))
    TEAROFF_METHOD(CServer, GetDocMiscStatus, getdocmiscstatus, (DWORD * pdwStatus))
    TEAROFF_METHOD(CServer, EnumViews, enumviews, (IEnumOleDocumentViews ** ppEnumViews, IOleDocumentView ** ppView))
END_TEAROFF_TABLE()

//BEGIN_TEAROFF_TABLE(CServer, IQuickActivate)
//    TEAROFF_METHOD(QuickActivate, (QACONTAINER *pqacontainer, QACONTROL *pqacontrol))
//    TEAROFF_METHOD(SetContentExtent, (LPSIZEL))
//    TEAROFF_METHOD(GetContentExtent, (LPSIZEL))
//END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CServer, IOleCache2)
 TEAROFF_METHOD(CServer, Cache, cache, (FORMATETC *pformatetc, DWORD advf, DWORD *pdwConnection))
 TEAROFF_METHOD(CServer, Uncache, uncache, (DWORD dwConnection))
 TEAROFF_METHOD(CServer, EnumCache, enumcache, (IEnumSTATDATA  **ppenumSTATDATA))
 TEAROFF_METHOD(CServer, InitCache, initcache, (IDataObject *pDataObject))
 // SetData renamed to SetDataCache to avoid conflict with IDataObject::SetData
 TEAROFF_METHOD(CServer, SetDataCache, setdatacache, (FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease))
 TEAROFF_METHOD(CServer, UpdateCache, updatecache, (LPDATAOBJECT pDataObject, DWORD grfUpdf, LPVOID pReserved))
 TEAROFF_METHOD(CServer, DiscardCache, discardcache, (DWORD dwDiscardOptions))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CServer, IPointerInactive)
    TEAROFF_METHOD(CServer, GetActivationPolicy, getactivationpolicy, (DWORD * pdwPolicy))
    TEAROFF_METHOD(CServer, OnInactiveMouseMove, oninactivemousemove, (LPCRECT pRectBounds, long x, long y, DWORD grfKeyState))
    TEAROFF_METHOD(CServer, OnInactiveSetCursor, oninactivesetcursor, (LPCRECT pRectBounds, long x, long y, DWORD dwMouseMsg, BOOL fSetAlways))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CServer, IPerPropertyBrowsing)
    TEAROFF_METHOD(CServer, GetDisplayString, getdisplaystring, (DISPID dispid, BSTR * lpbstr))
    TEAROFF_METHOD(CServer, MapPropertyToPage, mappropertytopage, (DISPID dispid, LPCLSID lpclsid))
    TEAROFF_METHOD(CServer, GetPredefinedStrings, getpredefinedstrings, (DISPID dispid, CALPOLESTR * lpcaStringsOut, CADWORD FAR* lpcaCookiesOut))
    TEAROFF_METHOD(CServer, GetPredefinedValue, getpredefinedvalue, (DISPID dispid, DWORD dwCookie, VARIANT * lpvarOut))
END_TEAROFF_TABLE()

ATOM   CServer::s_atomWndClass;

//+---------------------------------------------------------------
//
//  Member:     CServer::CServer
//
//  Synopsis:   Constructor for CServer object
//
//  Notes:      To create a properly initialized object you must
//              call the Init method immediately after construction.
//
//---------------------------------------------------------------

// CHROME
// Additional flag to indicate that the server is being created via
// the special CHROME classid and not by the standard MSHTML classids.
CServer::CServer(IUnknown *pUnkOuter, BOOL fChrome) : CBase(), _fChrome(fChrome)
{
    TraceTag((tagCServer, "constructing CServer"));

    IncrementObjectCount(&_dwObjCnt);

    _pUnkOuter  = pUnkOuter ? pUnkOuter : PunkInner();

    _state = OS_PASSIVE;
    _fUserMode = TRUE;

#ifndef NO_EDIT
    Assert(!_pUndoMgr);

    _pUndoMgr = &g_DummyUndoMgr;
#endif // NO_EDIT

#ifdef WIN16
    _dwThreadId = GetCurrentThreadId();
#endif

    _dwAspects = DVASPECT_CONTENT;

    _sizel.cx = _sizel.cy = 1;

}

//+---------------------------------------------------------------
//
//  Member:     CServer::~CServer
//
//  Synopsis:   Destructor for the CServer object
//
//  Notes:      The destructor is called as a result of the servers
//              reference count going to 0.  It ensure the object
//              is in a passive state and releases the data/view and inplace
//              subobjects objects.
//
//---------------------------------------------------------------

CServer::~CServer(void)
{
    //  Release interface pointers and strings

    // Delete here just in case we created it without going inplace.
    delete _pInPlace;
    _pInPlace = NULL;

    ReleaseInterface(_pClientSite);
    ReleaseInterface(_pAdvSink);
    ReleaseInterface(_pStg);
#ifndef NO_EDIT
    ReleaseInterface(_pUndoMgr);
#endif // NO_EDIT

    ReleaseInterface(_pOleAdviseHolder);
    ReleaseInterface(_pDataAdviseHolder);
    ReleaseInterface(_pCache);
    ReleaseInterface(_pPStgCache);
    ReleaseInterface(_pViewObjectCache);
    ReleaseInterface(_pPictureMouse);

#ifdef _MAC
	DestroyMacScrollbar(_hVertScroll);
	DestroyMacScrollbar(_hHorzScroll);
#endif

    DecrementObjectCount(&_dwObjCnt);

    TraceTag((tagCServer, "destructed CServer"));
}



//+---------------------------------------------------------------------------
//
//  Member:     CServer::Passivate
//
//  Synopsis:   Per CBase.
//
//----------------------------------------------------------------------------

void
CServer::Passivate()
{
    //  Containers are not required to call IOleObject::Close on
    //    objects; containers are allowed to just release all pointers
    //    to an embedded object.  This means that the last reference
    //    to an object can disappear while the object is still in
    //    the OS_RUNNING state.  So, we demote it if necessary.

    Assert(_state <= OS_RUNNING);
    if (_state > OS_LOADED)
    {
        TransitionTo(OS_LOADED);
    }

#ifndef NO_EDIT
    ClearInterface(&_pUndoMgr);

    _pUndoMgr = &g_DummyUndoMgr;
#endif // NO_EDIT


    CBase::Passivate();
}



//+---------------------------------------------------------------------------
//
//  Member:     CServer::PrivateQueryInterface, public
//
//  Synopsis:   QueryInterface on the private unknown for CServer
//
//  Arguments:  [riid] -- Interface to return
//              [ppv]  -- New interface returned here
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

STDMETHODIMP
CServer::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_INHERITS(this, IViewObjectEx)
        QI_INHERITS2(this, IViewObject, IViewObjectEx)
        QI_INHERITS2(this, IViewObject2, IViewObjectEx)
        QI_TEAROFF(this, IOleObject, _pUnkOuter)
        QI_TEAROFF(this, IOleControl, _pUnkOuter)
        QI_TEAROFF((CBase *)this, IProvideMultipleClassInfo, _pUnkOuter)
        QI_TEAROFF2((CBase *)this, IProvideClassInfo, IProvideMultipleClassInfo, _pUnkOuter)
        QI_TEAROFF2((CBase *)this, IProvideClassInfo2, IProvideMultipleClassInfo, _pUnkOuter)
        QI_TEAROFF((CBase *)this, ISpecifyPropertyPages, _pUnkOuter)
#ifdef FANCY_CONNECTION_STUFF
        QI_TEAROFF(this, IRunnableObject, _pUnkOuter)
        QI_TEAROFF(this, IExternalConnection, _pUnkOuter)
#endif
        QI_TEAROFF(this, IDataObject, _pUnkOuter)
        QI_TEAROFF(this, IOleDocument, _pUnkOuter)
        QI_TEAROFF(this, IOleCache2, _pUnkOuter)
        QI_TEAROFF2(this, IOleCache, IOleCache2, _pUnkOuter)
        QI_TEAROFF((CBase *)this, IOleCommandTarget, _pUnkOuter)
        QI_TEAROFF(this, IPointerInactive, _pUnkOuter)
        QI_TEAROFF(this, ISupportErrorInfo, _pUnkOuter)
        QI_TEAROFF(this, IPerPropertyBrowsing, _pUnkOuter)
        QI_TEAROFF2(this, IOleWindow, IOleInPlaceObjectWindowless, _pUnkOuter)
        QI_TEAROFF2(this, IOleInPlaceObject, IOleInPlaceObjectWindowless, _pUnkOuter)
        QI_TEAROFF(this, IOleInPlaceObjectWindowless, _pUnkOuter)
        QI_TEAROFF(this, IOleInPlaceActiveObject, _pUnkOuter)
        QI_TEAROFF(this, IOleDocumentView, _pUnkOuter)
#ifdef WIN16
        QI_TEAROFF(this, IPersistStreamInit, _pUnkOuter)
#endif 

        default:
            if (iid == IID_IConnectionPointContainer)
            {
                *((IConnectionPointContainer **)ppv) =
                        new CConnectionPointContainer(this, NULL);

                if (!*ppv)
                    RRETURN(E_OUTOFMEMORY);
            }
            else if (((iid == *(BaseDesc()->_piidDispinterface)) ||
                (iid == IID_IDispatch)) &&
                (ServerDesc()->_ibItfPrimary != 0))
            {
                *ppv = (IUnknown *) ((BYTE *) this + ServerDesc()->_ibItfPrimary);
            }
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    (*(IUnknown **)ppv)->AddRef();
    DbgTrackItf(iid, "CServer", FALSE, ppv);

    return S_OK;
}



//+---------------------------------------------------------------
//
//  Member:     CServer::TransitionTo, public
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
CServer::TransitionTo(OLE_SERVER_STATE state, LPMSG lpmsg)
{
    PerfDbgLog2(tagPerfWatch, this, "+CServer::TransitionTo %s --> %s", DebugOleStateName(_state), DebugOleStateName(state));

    HRESULT hr = S_OK;

    //  Our objects are not arbitrarily reentrant; specifically,
    //    during TransitionTo, reentrancy to other state transition
    //    functions is only allowed during the various
    //    On[InPlace/UI][Activate/Deactivate] notifications.

    if (TestLock(SERVERLOCK_TRANSITION))
        RRETURN(E_UNEXPECTED);
#ifdef WIN16
    // in case we were are on some other thread's stack (e.g. Java)... 
    // impersonate our own
    CThreadImpersonate cimp(_dwThreadId);
#endif
    //
    //  CLock takes care of blocking transitions and
    //    stabilizing the reference count.
    //

    CLock Lock(this, SERVERLOCK_TRANSITION);

    //  Note that support for the OPEN state is being removed
    //    from the base classes

    Assert(state >= OS_LOADED && state <= OS_UIACTIVE);
    Assert(_state >= OS_PASSIVE && _state <= OS_UIACTIVE);

    // trace tag output
    TraceTag((tagOleState,
                    "%08lX Transition from %s to %s",
                    this,
                    DebugOleStateName(_state),
                    DebugOleStateName(state)));
    
    //  Loop until we've reached the target state.  Some of the
    //    transitions include callbacks to the container, where we
    //    allow the container to make a reentrant call to TransitionTo.
    //    If the state doesn't match our immediate (one-hop) target,
    //    then we stop trying, under the assumption that someone else
    //    has transitioned us to a more urgent target.
    //
    //  + Transitions which do not have reentrant callbacks have
    //    asserts that either an error occurred, or else we're in
    //    the right state
    //
    //  + Transitions with callbacks exit (with success) if after
    //    the transition the object is in an unexpected state

    while (State() != state && !hr)
    {
        switch (_state)
        {
        case OS_PASSIVE:
            PerfDbgLog(tagPerfWatch, this, "+CServer::TransitionTo (PassiveToLoaded)");
            hr = THR(PassiveToLoaded());
            PerfDbgLog(tagPerfWatch, this, "-CServer::TransitionTo (PassiveToLoaded)");
            Assert(hr || _state == OS_LOADED);
            break;

        case OS_LOADED:
            PerfDbgLog(tagPerfWatch, this, "+CServer::TransitionTo (LoadedToRunning)");
            hr = THR(LoadedToRunning());
            PerfDbgLog(tagPerfWatch, this, "-CServer::TransitionTo (LoadedToRunning)");
            Assert(hr || _state == OS_RUNNING);
            break;

        case OS_RUNNING:
            switch (state)
            {
            case OS_LOADED:
                PerfDbgLog(tagPerfWatch, this, "+CServer::TransitionTo (RunningToLoaded)");
                hr = THR(RunningToLoaded());
                PerfDbgLog(tagPerfWatch, this, "-CServer::TransitionTo (RunningToLoaded)");
                Assert(hr || _state == OS_LOADED);
                break;

            case OS_INPLACE:
            case OS_UIACTIVE:
                PerfDbgLog(tagPerfWatch, this, "+CServer::TransitionTo (RunningToInPlace)");
                hr = THR(RunningToInPlace(lpmsg));
                PerfDbgLog(tagPerfWatch, this, "-CServer::TransitionTo (RunningToInPlace)");
                if (_state != OS_INPLACE)
                    goto StopTrying;
                break;

            }
            break;

        case OS_INPLACE:
            switch (state)
            {
            case OS_UIACTIVE:
                if (!RequestUIActivate())
                {
                    hr = E_FAIL;
                    goto StopTrying;
                }
                PerfDbgLog(tagPerfWatch, this, "+CServer::TransitionTo (InPlaceToUIActive)");
                hr = THR(InPlaceToUIActive(lpmsg));
                PerfDbgLog(tagPerfWatch, this, "-CServer::TransitionTo (InPlaceToUIActive)");
                if (_state != OS_UIACTIVE)
                    goto StopTrying;
                break;

            default:
                PerfDbgLog(tagPerfWatch, this, "+CServer::TransitionTo (InPlaceToRunning)");
                hr = THR(InPlaceToRunning());
                PerfDbgLog(tagPerfWatch, this, "-CServer::TransitionTo (InPlaceToRunning)");
                if (_state != OS_RUNNING)
                    goto StopTrying;
                break;
            }
            break;

        case OS_UIACTIVE:
            PerfDbgLog(tagPerfWatch, this, "+CServer::TransitionTo (UIActiveToInPlace)");
            hr = THR(UIActiveToInPlace());
            PerfDbgLog(tagPerfWatch, this, "-CServer::TransitionTo (UIActiveToInPlace)");
            if (_state != OS_INPLACE)
                goto StopTrying;
            break;

        }
    }

StopTrying:

    PerfDbgLog(tagPerfWatch, this, "-CServer::TransitionTo");
    RRETURN1(hr, OLEOBJ_S_CANNOT_DOVERB_NOW);
}


//+---------------------------------------------------------------
//
//  Member:     CServer::PassiveToLoaded, protected
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
CServer::PassiveToLoaded(void)
{
    _state = OS_LOADED;
    return S_OK;
}



//+---------------------------------------------------------------
//
//  Member:     CServer::LoadedToRunning, protected
//
//  Synopsis:   Effects the direct loaded to running state transition
//
//  Returns:    Success if the object is running.
//
//  Notes:      This transition occurs as a result of an
//              IRunnableObject::Run call and is implicit in any
//              DoVerb call.
//
//---------------------------------------------------------------

HRESULT
CServer::LoadedToRunning(void)
{
    LPOLECONTAINER  pContainer;

    //  Put a lock on our container.  This is necessary if our container
    //    allows other clients to link to contained objects (us).

    if (_pClientSite && !_pClientSite->GetContainer(&pContainer))
    {
        // BUGBUG - should we remember LockContainer failed so that we
        //      can avoid calling LockContainer(FALSE) later?

        THR_NOTRACE(pContainer->LockContainer(TRUE));
        pContainer->Release();
    }

    _state = OS_RUNNING;

    return S_OK;
}



//+---------------------------------------------------------------
//
//  Member:     CServer::RunningToLoaded, protected
//
//  Synopsis:   Effects the direct running to loaded state transition
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
CServer::RunningToLoaded(void)
{
    LPOLECONTAINER  pContainer  = NULL;

    //  Release the lock we put on our container.  Not all containers
    //    support locking, so we allow failures

    if (_pClientSite)
    {
        if (!_pClientSite->GetContainer(&pContainer))
        {
            THR_NOTRACE(pContainer->LockContainer(FALSE));
            pContainer->Release();
        }
    }

    if (_lDirtyVersion)
    {
        SendOnDataChange(ADVF_DATAONSTOP);
    }

    //  Notify our advise holders that we have closed

    if (_pOleAdviseHolder)
        _pOleAdviseHolder->SendOnClose();

#if 0
    //  BUGBUG not sure where (or even if) we should be calling
    //    this.  Chris and Gary need to investigate.

    //  Forcibly cut off remoting clients

    CoDisconnectObject((LPOLEOBJECT) this, 0);
#endif

    _state = OS_LOADED;

    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CServer::RunningToInPlace, protected
//
//  Synopsis:   Effects the direct Running to inplace state transition
//
//  Returns:    Success iff the object is in the inplace state.  On failure
//              the object will be in a consistent Running state.
//
//  Notes:      This transition invokes the ActivateInPlace method.  Containers
//              will typically override this method in order to additionally
//              inplace activate any inside-out embeddings that are visible.
//
//---------------------------------------------------------------

HRESULT
CServer::RunningToInPlace(LPMSG lpmsg)
{
    HRESULT                     hr;

    //  BUGBUG flatten call tree

    hr = THR(ActivateInPlace(lpmsg));
    if (hr)
        goto Error;

#ifdef FANCY_CONNECTION_STUFF
    //  Put an external lock on ourselves so we stay in the running
    //    state even if we have other external connections.

    IGNORE_HR(LockRunning(TRUE, FALSE));
#endif

Error:
    RRETURN(hr);
}



//+---------------------------------------------------------------
//
//  Member:     CServer::InPlaceToRunning, protected
//
//  Synopsis:   Effects the direct inplace to Running state transition
//
//  Returns:    Success under all but catastrophic circumstances.
//
//  Notes:      This transition invokes the DeactivateInPlace method.
//
//              Containers will typically override this method in
//              order to additionally inplace deactivate any inplace-active
//              embeddings.
//
//              This method is called as the result of a DoVerb(HIDE...)
//
//---------------------------------------------------------------

HRESULT
CServer::InPlaceToRunning(void)
{
    HRESULT     hr;

    //  BUGBUG flatten call tree

    //
    // Free the external lock we hold to keep ourselves in the running
    // state in the case of other external connections.
    //

#ifdef FANCY_CONNECTION_STUFF
    IGNORE_HR(LockRunning(FALSE, FALSE));
#endif

    hr = THR(DeactivateInPlace());

    RRETURN(hr);
}



//+---------------------------------------------------------------
//
//  Member:     CServer::InPlaceToUIActive, protected
//
//  Synopsis:   Effects the direct inplace to U.I. active state transition
//
//  Returns:    Success iff the object is in the U.I. active state.  On failure
//              the object will be in a consistent inplace state.
//
//  Notes:      This transition invokes the ActivateUI method.
//
//---------------------------------------------------------------

HRESULT
CServer::InPlaceToUIActive(LPMSG lpmsg)
{
    HRESULT     hr;

    //  BUGBUG flatten call tree

#ifndef NO_OLEUI
    hr = THR(ActivateUI(lpmsg));
    if (hr)
        goto Error;
#endif // NO_OLEUI

    hr = THR(SetActiveObject());

Error:
    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member:     CServer::UIActiveToInPlace, protected
//
//  Synopsis:   Effects the direct U.I. Active to inplace state transition
//
//  Returns:    Success under all but catastrophic circumstances.
//
//  Notes:      This transition invokes the DeactivateUI method.
//              Containers will typically override this method in
//              order to possibly U.I. deactivate a U.I. active embedding.
//
//---------------------------------------------------------------

HRESULT
CServer::UIActiveToInPlace(void)
{
    IGNORE_HR(ClearActiveObject());

#ifndef NO_OLEUI
    RRETURN(THR(DeactivateUI()));
#else
	RRETURN(S_OK);
#endif // NO_OLEUI
}


//+---------------------------------------------------------------
//
//  Member:     CServer::SetActiveObject, protected
//
//  Synopsis:   Informs the frame that we are now the UIActive object.
//
//---------------------------------------------------------------

HRESULT
CServer::SetActiveObject()
{
    HRESULT hr;
    IOleInPlaceActiveObject * pInPlaceActiveObject = NULL;

    TCHAR   ach[MAX_USERTYPE_LEN + 1];

    Verify(LoadString(
            GetResourceHInst(),
            IDS_USERTYPESHORT(BaseDesc()->_idrBase),
            ach,
            ARRAY_SIZE(ach)));

    hr = THR(PrivateQueryInterface(IID_IOleInPlaceActiveObject, (void **)&pInPlaceActiveObject));
    if (hr)
        goto Cleanup;

    hr = THR(_pInPlace->_pFrame->SetActiveObject(pInPlaceActiveObject, ach));

Cleanup:
    ReleaseInterface(pInPlaceActiveObject);
    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member:     CServer:ClearActiveObject, protected
//
//  Synopsis:   Informs the frame that we are no longer the UIActive object.
//
//---------------------------------------------------------------

HRESULT
CServer::ClearActiveObject()
{
    RRETURN(THR(_pInPlace->_pFrame->SetActiveObject(NULL,NULL)));
}


//+---------------------------------------------------------------
//
//  Member:     CServer::SetClientSite, public
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method saves the client site pointer in the
//              _pClientSite member variable.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::SetClientSite(LPOLECLIENTSITE pClientSite)
{
    IOleDocumentSite *   pMsoDocSite;
    IOleUndoManager *    pUM = NULL;
    HRESULT              hr;
    HRESULT              hr2;


    //  If we already have a client site, refuse requests to
    //    change it
    hr = S_OK;
    if (_pClientSite)
    {
        if (pClientSite)
            RRETURN(E_INVALIDARG);

        ClearInterface(&_pClientSite);

        Assert(!_pInPlace || !_pInPlace->_pInPlaceSite);

        // BUGBUG - Why is this needed?
        _fMsoViewExists = FALSE;
    }
    else
    {
        _pClientSite = pClientSite;
        _pClientSite->AddRef();

        DbgTrackItf(IID_IOleClientSite, "HostSite", TRUE, (void **)&_pClientSite);

        _fUserMode = GetAmbientBool(DISPID_AMBIENT_USERMODE, TRUE);

        // BUGBUG Can we recognize doc mode without doing this? (garybu)
        if (OK(THR_NOTRACE(pClientSite->QueryInterface(
                IID_IOleDocumentSite, (void **) &pMsoDocSite))))
        {
            _fMsoDocMode = TRUE;
            ReleaseInterface(pMsoDocSite);
        }

#ifndef NO_EDIT
        if (!hr && (_pUndoMgr == &g_DummyUndoMgr))
        {
            //
            // We don't want to call any overridden implementations because
            // all we want to do is check our parent. We don't want any
            // derived classes creating the undo manager in this call because
            // we'll create it ourselves below.
            //
            hr2 = THR_NOTRACE(CServer::QueryService(
                                SID_SOleUndoManager,
                                IID_IOleUndoManager,
                                (LPVOID*)&pUM));

            if (OK(hr2) && pUM)
            {
                _pUndoMgr = pUM;
            }
            else
            {
                if (ServerDesc()->TestFlag(SERVERDESC_CREATE_UNDOMGR))
                {
                    hr = THR(CreateUndoManager());
                }
                else
                {
                    _pUndoMgr = &g_DummyUndoMgr;
                }
            }
        }
#endif // NO_EDIT
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member:     CServer::GetClientSite, IOleObject
//
//  Synopsis:   Method of IOleObject interface
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::GetClientSite(LPOLECLIENTSITE * ppClientSite)
{
    *ppClientSite = _pClientSite;
    if (_pClientSite)
    {
        _pClientSite->AddRef();
    }
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CServer::SetHostNames
//
//  Synopsis:   Method of IOleObject interface
//
//              These strings should be remembered for open
//              edit or to pass along to embeddings that open
//              edit.  Most classes derived from CServer don't
//              care about this, so we ignore them here.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::SetHostNames(LPCTSTR lpstrCntrApp, LPCTSTR lpstrCntrObj)
{
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CServer::Close
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method ensures the object is in the loaded
//              (not passive!) state.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::Close(DWORD dwSaveOption)
{
    HRESULT hr          = S_OK;
    BOOL    fSave;
    int     id;

    // if our object is dirty then we should save it, depending on the
    // save options
    if (_pClientSite != NULL && _lDirtyVersion)
    {
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
                if (_state <= OS_INPLACE)
                {
                    //we do not prompt if the object
                    //is in-place or invisible
                    fSave = TRUE;
                    break;
                }

                //we only prompt in the following state
                Assert(_state == OS_UIACTIVE);

                //  Put up a message box asking the user if they want
                //    to update the container.

                //  CONSIDER make this work better with DisplayName
                //    ambient property

                hr = THR(ShowMessage(
                    &id,
                    MB_YESNOCANCEL | MB_ICONQUESTION,
                    0,
                    IDS_MSG_SAVE_MODIFIED_OBJECT));

                if (hr)
                    RRETURN(hr);

                if (id == IDCANCEL)
                    RRETURN(OLE_E_PROMPTSAVECANCELLED);

                fSave = (id == IDYES);
            }
            break;

        default:
            RRETURN(E_INVALIDARG);
        }

        if (fSave)
            hr = THR(_pClientSite->SaveObject());
    }

    if (!hr)
        hr = THR(TransitionTo(OS_LOADED));

    RRETURN(hr);
}



//+---------------------------------------------------------------
//
//  Member:     CServer::SetMoniker
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
CServer::SetMoniker(DWORD dwWhichMoniker, LPMONIKER pmk)
{
    HRESULT     hr      = S_OK;
    LPMONIKER   pmkFull = NULL;

    //  Ensure that we have a full moniker to register in the ROT
    //  If we have a full moniker, then go with it.
    //  Otherwise ask our client site for a full moniker.

    if (dwWhichMoniker == OLEWHICHMK_OBJFULL)
    {
        pmkFull = pmk;
        if (pmkFull)
            pmkFull->AddRef();
    }
    else
    {
        if (_pClientSite == NULL)
        {
            hr = E_FAIL;
        }
        else
        {
            hr = THR(_pClientSite->GetMoniker(
                    OLEGETMONIKER_ONLYIFTHERE,
                    OLEWHICHMK_OBJFULL,
                    &pmkFull));
        }

        if (hr)
            goto Cleanup;
    }

    if (pmkFull)
    {
        //  Notify our advise holders that we have been renamed

        if (_pOleAdviseHolder)
            _pOleAdviseHolder->SendOnRename(pmkFull);
    }

Cleanup:
    RRETURN(hr);
}



//+---------------------------------------------------------------
//
//  Member:     CServer::GetMoniker
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method forwards the request to our client site
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER FAR* ppmk)
{
    HRESULT     hr;

    if (ppmk == NULL)
        RRETURN(E_INVALIDARG);

    *ppmk = NULL;   // set out parameters to NULL

    //  Get the requested moniker from our client site

    if (_pClientSite == NULL)
        hr = MK_E_UNAVAILABLE;
    else
        hr = THR_NOTRACE(_pClientSite->GetMoniker(dwAssign, dwWhichMoniker, ppmk));

    RRETURN_NOTRACE(hr);
}



//+---------------------------------------------------------------
//
//  Member:     CServer::InitFromData
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method returns S_FALSE indicating InitFromData
//              is not supported.  Servers that wish to support initialization
//              from a selection should override this function.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::InitFromData(
        LPDATAOBJECT pDataObject,
        BOOL fCreation,
        DWORD dwReserved)
{
    return S_FALSE;
}



//+---------------------------------------------------------------
//
//  Member:     CServer::GetClipboardData
//
//  Synopsis:   Method of IOleObject interface
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::GetClipboardData(DWORD dwReserved, LPDATAOBJECT FAR* ppDataObject)
{
    if (ppDataObject == NULL)
        RRETURN(E_INVALIDARG);

    *ppDataObject = NULL;               // set out params to NULL

    RRETURN(E_NOTIMPL);
}



//+---------------------------------------------------------------
//
//  Member:     CServer::DoVerb
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
CServer::DoVerb(
        LONG iVerb,
        LPMSG lpmsg,
        LPOLECLIENTSITE pActiveSite,
        LONG lindex,
        HWND hwndParent,
        LPCOLERECT lprcPosRect)
{
    HRESULT hr  = S_OK;
    int     i;

    Assert(ServerDesc());
    Assert(ServerDesc()->_pOleVerbTable);
    Assert(ServerDesc()->_pOleVerbTable[0].lVerb == OLEIVERB_PRIMARY);

    //  Find the verb in the verb table.

    for (i = 0; i < ServerDesc()->_cOleVerbTable; i++)
    {
        if (iVerb == ServerDesc()->_pOleVerbTable[i].lVerb)
            break;
    }

    //  If the verb is not in the table, then return an error for negative
    //      verbs or default to the 0th entry in the table for positive verbs.
    //      The 0th entry in the table must be the primary verb.

    if (i >= ServerDesc()->_cOleVerbTable)
    {
        if (iVerb < 0)
            RRETURN(E_NOTIMPL);

        i = 0;
    }

    //  All DoVerbs make an implicit transition to the running state if
    //  the object is not already running.
    //
    //  Putting the server into the running state and then returning
    //  E_NOTIMPL for a verb will confuse the container about our state.
    //  Because some classes can return E_NOTIMPL from the state transition
    //  verbs, we skip the step of putting the object into the running
    //  state here.  The implementation of the state transition verbs
    //  will take care of getting the server into the running state.

    if (_state < OS_RUNNING &&
            iVerb != OLEIVERB_INPLACEACTIVATE &&
            iVerb != OLEIVERB_UIACTIVATE)
    {
        // if we are less than OS_LOADED this will transition us up to
        // running.  This happens with outlook
        hr = THR(TransitionTo(OS_RUNNING));
        if (hr)
            goto Cleanup;
    }

    //  Dispatch the verb

#ifdef WIN16
    // we should probably make all the verbs take short rects.
    // In fact, they should all be PFNDOVERB, not individually typed.
    // But for now we'll leave them be.
    {
        RECTL rcPosRectl;
        RECTL *pRectl = NULL;
        if (lprcPosRect)
        {
            CopyRect(&rcPosRectl, lprcPosRect);
            pRectl = &rcPosRectl;
        }
        hr = THR((*ServerDesc()->_pfnDoVerb[i])(
                this,
                iVerb,
                lpmsg,
                pActiveSite,
                lindex,
                hwndParent,
                pRectl));
    }
#else
    hr = THR((*ServerDesc()->_pfnDoVerb[i])(
            this,
            iVerb,
            lpmsg,
            pActiveSite,
            lindex,
            hwndParent,
            lprcPosRect));
#endif
    
    if (hr)
        goto Cleanup;

    //  If we defaulted to the 0th entry in the table (primary verb),
    //      then return the appropriate status.

    if (OK(hr) && iVerb != ServerDesc()->_pOleVerbTable[i].lVerb)
    {
        hr = OLEOBJ_S_INVALIDVERB;
    }

Cleanup:

    //  CONSIDER the error handling here is a little unclear.  Normally,
    //    if we get an error, we try to leave ourselves in our original
    //    state.  However, with DoVerb, we don't have enough information
    //    to know why we're in the state we're in.  We might have
    //    failed, or someone might have moved us there during a callback,
    //    or whatever.
    //
    //  For Forms95, we just leave ourselves in whatever state we're
    //    in if an error occurs.  Note that we do guarantee that we've
    //    legally reached the current state.  Revisit for Forms96.

    RRETURN2(hr, OLEOBJ_S_INVALIDVERB, OLEOBJ_S_CANNOT_DOVERB_NOW);
}



//+---------------------------------------------------------------
//
//  Member:     CServer::EnumVerbs
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method produces an enumerator over the server's
//              verb table.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::EnumVerbs(LPENUMOLEVERB FAR* ppenum)
{
    HRESULT     hr;

    if (ppenum == NULL)
        RRETURN(E_INVALIDARG);

    hr = THR(CreateOLEVERBEnum(
            ServerDesc()->_pOleVerbTable,
            ServerDesc()->_cOleVerbTable,
            ppenum));

    RRETURN(hr);
}



//+---------------------------------------------------------------
//
//  Member:     CServer::Update
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method returns S_OK indicating that the update was
//              successful.  Containers will wish to override this function
//              in order to recursively pass the function on to all embeddings.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::Update(void)
{
    return S_OK;
}



//+---------------------------------------------------------------
//
//  Member:     CServer::IsUpToDate
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method returns S_OK indicating that the object is
//              up to date.  Containers will wish to override this function
//              in order to recursively pass the function on to all embeddings.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::IsUpToDate(void)
{
    return S_OK;
}



//+---------------------------------------------------------------
//
//  Member:     CServer::GetUserClassID
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method supplies the class id from the server's
//              CLASSDESC structure
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::GetUserClassID(CLSID FAR* pClsid)
{
    if (pClsid == NULL)
    {
        RRETURN(E_INVALIDARG);
    }

    *pClsid = *BaseDesc()->_pclsid;
    return S_OK;
}



//+---------------------------------------------------------------
//
//  Member:     CServer::GetUserType
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method supplies the user type string from the server's
//              CLASSDESC structure
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::GetUserType(DWORD dwFormOfType, LPTSTR FAR* ppstr)
{
    int         ids;
    TCHAR       ach[MAX_USERTYPE_LEN + 1];
    const CBase::CLASSDESC * pcd = BaseDesc();

    switch (dwFormOfType)
    {
    case USERCLASSTYPE_APPNAME:
        ids = IDS_USERTYPEAPP;
        break;

    case USERCLASSTYPE_SHORT:
        ids = IDS_USERTYPESHORT(pcd->_idrBase);
        break;

    case USERCLASSTYPE_FULL:
        ids = IDS_USERTYPEFULL(pcd->_idrBase);
        break;

    default:
        *ppstr = NULL;
        RRETURN(E_INVALIDARG);
    }



    Verify(LoadString(
            GetResourceHInst(),
            ids,
            ach,
            ARRAY_SIZE(ach)));

    RRETURN(TaskAllocString(ach, ppstr));
}



//+---------------------------------------------------------------
//
//  Member:     CServer::SetExtent
//
//  Synopsis:   Method of IOleObject interface.  Sets the extent
//              for some aspect of the object.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::SetExtent(DWORD dwAspect, SIZEL *psizel)
{
    HRESULT     hr;
    BOOL fChangedExtent = FALSE;

    Assert(psizel->cx >= 0);
    Assert(psizel->cy >= 0);

    if (psizel->cx < 0 || psizel->cy < 0)
    {
        RRETURN(E_FAIL);
    }

    switch (dwAspect)
    {
    case DVASPECT_CONTENT:

/*
        if (State() >= OS_INPLACE && _sizel.cx && _sizel.cy)
        {
            SIZE size;

            // In order to do himetric <-> pixel conversions, we need to keep
            // _sizel and _pInPlace->_rcPos in sync.  We do this by updating
            // _pInPlace->_rcPos to what we think the container will tell us
            // in a later call to IOleInPlaceObject::SetObjectRects. If the
            // container does not do this, we will be in a confused state.
            //
            // CONSIDER: Consider the CDK behavior.  It keeps things in sync
            // by calling IOleInPlace::OnPosRectChanged with the new rectangle
            // instead of setting _pInPlace->_rcPos directly. This seems
            // wasteful because it causes the container to call
            // SetObjectRects twice -- once from OnPosRectChanged and a
            // second time because the container is trying to resize the
            // control. The CDK uses a flag to prevent infinite recursion
            // should the container call IOleObject::SetExtent from
            // IOleInPlaceSite::OnPosRectChanged (as many seem to do).

            WindowFromDocument(&size, *psizel);
            _pInPlace->_rcPos.right  = _pInPlace->_rcPos.left + size.cx;
            _pInPlace->_rcPos.bottom = _pInPlace->_rcPos.top  + size.cy;
        }
*/
        if ( memcmp ( &_sizel, psizel, sizeof ( SIZEL ) ) )
        {
            fChangedExtent = TRUE;
        }
        _sizel = *psizel;
        hr = S_OK;
        break;

    case DVASPECT_DOCPRINT:
        hr = E_NOTIMPL;
        goto Cleanup;

    case DVASPECT_THUMBNAIL:
    case DVASPECT_ICON:
        hr = E_FAIL;
        goto Cleanup;

    default:
        hr = E_INVALIDARG;
        goto Cleanup;
    }


    if (!_fMsoDocMode && fChangedExtent )
    {
        // Our width & height are persisted through property changes
        // So don't dirty the form
        OnDataChange(FALSE);
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member:     CServer::GetExtent
//
//  Synopsis:   Method of IOleObject interface.  Returns the current
//              size of some aspect of the object.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::GetExtent(DWORD dwAspect, LPSIZEL lpsizel)
{
    HRESULT     hr  = S_OK;

    if (lpsizel == NULL)
        RRETURN(E_INVALIDARG);

    switch (dwAspect)
    {
    case DVASPECT_CONTENT:
    case DVASPECT_DOCPRINT:
        *lpsizel = _sizel;
        break;

    case DVASPECT_THUMBNAIL:
        lpsizel->cx = HimetricFromHPix(32);
        lpsizel->cy = HimetricFromVPix(32);
        break;

    case DVASPECT_ICON:
        if (_pViewObjectCache)
        {
            hr = THR(_pViewObjectCache->GetExtent(
                    dwAspect,
                    -1,
                    NULL,
                    lpsizel));
        }
        else
        {
#ifdef WINCE
	        hr = E_INVALIDARG;
#else
            HGLOBAL         hMF;
            LPMETAFILEPICT  pMF;

            hMF = OleGetIconOfClass(*BaseDesc()->_pclsid, NULL, TRUE);
            if (hMF)
            {
                pMF = (LPMETAFILEPICT) GlobalLock(hMF);

                lpsizel->cx = pMF->xExt;    //  Values are HIMETRIC
                lpsizel->cy = pMF->yExt;

                DeleteMetaFile(pMF->hMF);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
#endif // WINCE
        }
        break;

    default:
        hr = E_INVALIDARG;
        break;
    }

    RRETURN(hr);
}



//+---------------------------------------------------------------
//
//  Member:     CServer::Advise
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method is implemented using the standard
//              OLE advise holder.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::Advise(IAdviseSink FAR* pAdvSink, DWORD FAR* pdwConnection)
{
    HRESULT hr;

    if (!pdwConnection)
        RRETURN(E_INVALIDARG);

    *pdwConnection = NULL;              // set out params to NULL

    if (!_pOleAdviseHolder)
    {
        hr = THR(CreateOleAdviseHolder(&_pOleAdviseHolder));
        if (hr)
            goto Cleanup;
    }

    hr = THR(_pOleAdviseHolder->Advise(pAdvSink, pdwConnection));

Cleanup:
    RRETURN(hr);
}



//+---------------------------------------------------------------
//
//  Member:     CServer::Unadvise
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method is implemented using the standard
//              OLE advise holder.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::Unadvise(DWORD dwConnection)
{
    HRESULT     hr;

    if (!_pOleAdviseHolder)
        RRETURN(OLE_E_NOCONNECTION);

    hr = THR(_pOleAdviseHolder->Unadvise(dwConnection));

    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member:     CServer::EnumAdvise
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method is implemented using the standard
//              OLE advise holder.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::EnumAdvise(LPENUMSTATDATA FAR* ppenumAdvise)
{
    HRESULT     hr;

    if (!ppenumAdvise)
        RRETURN(E_INVALIDARG);

    if (_pOleAdviseHolder == NULL)
    {
        *ppenumAdvise = NULL;
        hr = S_OK;
    }
    else
    {
        hr = THR(_pOleAdviseHolder->EnumAdvise(ppenumAdvise));
    }

    RRETURN(hr);
}



//+---------------------------------------------------------------
//
//  Member:     CServer::GetMiscStatus
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      This method supplies the misc status flags from the server's
//              CLASSDESC structure
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::GetMiscStatus (DWORD dwAspect, DWORD FAR* pdwStatus)
{
    *pdwStatus = ServerDesc()->_dwMiscStatus;
    return S_OK;
}



//+---------------------------------------------------------------
//
//  Member:     CServer::SetColorScheme
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      Servers should override this method if they are
//              interested in the palette.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::SetColorScheme (LPLOGPALETTE lpLogpal)
{
    RRETURN(E_NOTIMPL);   //will we ever?
}



//+---------------------------------------------------------------------------
//
//  Member:     CServer::GetRunningClass, public
//
//  Synopsis:   Returns our current class ID.  Method on IRunnableObject.
//
//  Arguments:  [lpClsid] -- Place to return clsid
//
//----------------------------------------------------------------------------
#ifdef FANCY_CONNECTION_STUFF

STDMETHODIMP
CServer::GetRunningClass(LPCLSID lpClsid)
{
    if (!lpClsid)
        RRETURN(E_INVALIDARG);

    *lpClsid = *BaseDesc()->_pclsid;

    return S_OK;
}
#endif


//+---------------------------------------------------------------------------
//
//  Member:     CServer::Run, public
//
//  Synopsis:   Method in IRunnableObject.  Puts us into the running state if
//              we're not already there.
//
//  Arguments:  [pbc] -- Bind context. (Ignored)
//
//  Notes:      If we're in the running, inplace, or UI Active states, then
//              this method has no effect.
//
//----------------------------------------------------------------------------

#ifdef FANCY_CONNECTION_STUFF

STDMETHODIMP
CServer::Run(LPBINDCTX pbc)
{
    HRESULT hr = S_OK;

    if (_state < OS_RUNNING)
        hr = THR(TransitionTo(OS_RUNNING));

    RRETURN(hr);
}
#endif


//+---------------------------------------------------------------------------
//
//  Member:     CServer::IsRunning, public
//
//  Synopsis:   Method on IRunnableObject.  Returns whether or not we're
//              running.
//
//  Arguments:  (none)
//
//----------------------------------------------------------------------------
#ifdef FANCY_CONNECTION_STUFF
STDMETHODIMP_(BOOL)
CServer::IsRunning(void)
{
    return (_state >= OS_RUNNING);
}
#endif


//+---------------------------------------------------------------------------
//
//  Member:     CServer::LockRunning, public
//
//  Synopsis:   Method on IRunnableObject.  Locks us in the running state.
//
//  Arguments:  [fLock]             -- TRUE to add a lock, FALSE to release.
//              [fLastUnlockCloses] -- If TRUE, releasing the last lock
//                                     closes us by calling IOleObject::Close
//
//----------------------------------------------------------------------------

#ifdef FANCY_CONNECTION_STUFF
STDMETHODIMP
CServer::LockRunning(BOOL fLock, BOOL fLastUnlockCloses)
{
    if (fLock)
    {
        _pUnkOuter->AddRef();

        Assert(_cStrongRefs < 255);
        _cStrongRefs += 1;
    }
    else
    {
        Assert(_cStrongRefs > 0);
        _cStrongRefs -= 1;
        if ((_cStrongRefs == 0) && fLastUnlockCloses)
        {
            Close(OLECLOSE_NOSAVE);
        }
        _pUnkOuter->Release();
    }
    return S_OK;
}
#endif


//+---------------------------------------------------------------------------
//
//  Member:     CServer::SetContainedObject, public
//
//  Synopsis:   Indicates whether or not we're a contained object.
//
//  Arguments:  [fContained] -- Tells us if we're a contained object.
//
//  Notes:      Since objects based on CServer are always contained, this
//              method is ignored.
//
//----------------------------------------------------------------------------

#ifdef FANCY_CONNECTION_STUFF

STDMETHODIMP
CServer::SetContainedObject(BOOL fContained)
{
    return S_OK;
}
#endif


//+---------------------------------------------------------------------------
//
//  Member:     CServer::AddConnection, public
//
//  Synopsis:   Method on IExternalConnection.  Indicates we have an
//              external connection, which will allow us to clean up
//              properly when all the external connections go away.
//
//  Arguments:  [extconn]  -- Type of connection
//              [reserved] --
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

#ifdef FANCY_CONNECTION_STUFF
STDMETHODIMP_(DWORD)
CServer::AddConnection(DWORD extconn, DWORD reserved)
{
    if (extconn & EXTCONN_STRONG)
    {
        LockRunning(TRUE, FALSE);
        return _cStrongRefs;
    }
    return 0;
}
#endif


//+---------------------------------------------------------------------------
//
//  Member:     CServer::ReleaseConnection, public
//
//  Synopsis:   Method on IExternalConnection. Tells us one of our external
//              connections is going away.  If we have no more external
//              connections, then we close.
//
//  Arguments:  [extconn]            -- Type of connection being released
//              [reserved]           --
//              [fLastReleaseCloses] -- Indicates if we should close or not.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

#ifdef FANCY_CONNECTION_STUFF
STDMETHODIMP_(DWORD)
CServer::ReleaseConnection(DWORD extconn,
                            DWORD reserved,
                            BOOL fLastReleaseCloses)
{
    if (extconn & EXTCONN_STRONG)
    {
        DWORD cStrongRefs = _cStrongRefs;
        LockRunning(FALSE, fLastReleaseCloses);
        return cStrongRefs - 1;
    }
    return 0;
}
#endif

//+------------------------------------------------------------------------
//
//  Member:     CServer::QuickActivate, IQuickActivate
//
//  Synopsis:   Connect this object to the container.
//
//              For high performance, functionality in CServer::SetClientSite
//              and other functions is inlined here.  If a derived class
//              overrides CServer::SetClientSite, it will need to override
//              this function as well.
//
//              This function assumes that the class has the following
//              connection points:
//
//                  [0] - Automation events
//                  [1] - Property notifications
//
//  Arguments:  pqacontainer    In, information about the container.
//              pqacontrol      Out, information about this object.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

STDMETHODIMP
CServer::QuickActivate(QACONTAINER *pqacontainer, QACONTROL *pqacontrol)
{
    HRESULT                 hr = S_OK;
    const CLASSDESC *       pcd = ServerDesc();

    if (pqacontainer->cbSize < sizeof(QACONTAINER) ||
        pqacontrol->cbSize < sizeof(QACONTROL))
        RRETURN(E_INVALIDARG);

    // CONSIDER: inline ReplaceInterface here for high speed.
    ReplaceInterface(&_pAdvSink, (IAdviseSink *)pqacontainer->pAdviseSink);
    ReplaceInterface(&_pClientSite, pqacontainer->pClientSite);

#ifndef NO_EDIT
    if (pqacontainer->pUndoMgr)
    {
        ReplaceInterface(&_pUndoMgr, pqacontainer->pUndoMgr);
    }
    else if (ServerDesc()->TestFlag(SERVERDESC_CREATE_UNDOMGR))
    {
        hr = THR(CreateUndoManager());
        if (hr)
            goto Cleanup;
    }
    else
    {
        _pUndoMgr = &g_DummyUndoMgr;
    }
#endif // NO_EDIT

    _fUseAdviseSinkEx = TRUE;
    _fUserMode = (pqacontainer->dwAmbientFlags & QACONTAINER_USERMODE) != 0;


    if (pqacontainer->pPropertyNotifySink)
    {
        hr = THR(ClampITFResult(DoAdvise(
                IID_IPropertyNotifySink,
                DISPID_A_PROPNOTIFYSINK,
                pqacontainer->pPropertyNotifySink,
                NULL,
                &pqacontrol->dwPropNotifyCookie)));
        if (hr)
            goto Cleanup;
    }

    pqacontrol->dwMiscStatus = pcd->_dwMiscStatus;
    pqacontrol->dwViewStatus = pcd->_dwViewStatus;
    GetActivationPolicy(&pqacontrol->dwPointerActivationPolicy);

Cleanup:
    RRETURN(hr);
}



//+------------------------------------------------------------------------
//
//  Member:     CServer::SetContentExtent, IQuickActivate
//
//  Synopsis:   Set the extent of the object.
//
//  Arguments:  lpsizel     The extent in himetric
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CServer::SetContentExtent(LPSIZEL lpsizel)
{
    RRETURN(THR(SetExtent(DVASPECT_CONTENT, lpsizel)));
}

//+------------------------------------------------------------------------
//
//  Member:     CServer::GetContentExtent, IQuickActivate
//
//  Synopsis:   Get the extent of the object.
//
//  Arguments:  lpsizel     The extent in himetric.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CServer::GetContentExtent(LPSIZEL lpsizel)
{
    RRETURN(THR(GetExtent(DVASPECT_CONTENT, lpsizel)));
}


//+------------------------------------------------------------------------
//
//  Member:     CServer::GetAmbientBool
//
//  Synopsis:   Helper method to get an ambient property of type BOOL.
//              The caller passes in a default value that is returned if
//              the property isn't present on the site, or if no
//              site is present.
//
//  Arguments:  [dispid]        --  property to get
//              [fDefaultValue] --  default value to use if property
//                                  not present
//
//  Returns:    BOOL
//
//-------------------------------------------------------------------------

BOOL
CServer::GetAmbientBool(DISPID dispid, BOOL fDefaultValue)
{
    HRESULT     hr;
    CVariant    var;
    BOOL        f = fDefaultValue;

    hr = GetAmbientVariant(dispid, &var);
    if (!hr && (var.vt == VT_BOOL))
    {
        f = (var.boolVal != 0);
    }

    return f;
}


//+------------------------------------------------------------------------
//
//  Member:     CServer::GetAmbientBstr
//
//  Synopsis:   Helper method to get an ambient property of type BSTR.
//
//  Arguments:  [dispid]        --  property to get
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CServer::GetAmbientBstr(DISPID dispid, BSTR *pbstr)
{
    HRESULT     hr;
    CVariant    var;

    *pbstr = NULL;

    hr = GetAmbientVariant(dispid, &var);
    if (!hr)
    {
        if (var.vt == VT_BSTR)
        {
            *pbstr = var.bstrVal;
            VariantInit(&var);
        }
        else
            hr = DISP_E_MEMBERNOTFOUND;
    }

    return hr;
}

HPALETTE
CServer::GetAmbientPalette()
{
#ifndef WIN16
    CVariant var;
    HRESULT hr = GetAmbientVariant(DISPID_AMBIENT_PALETTE, &var);
    if (SUCCEEDED(hr))
    {
        if (var.vt == VT_HANDLE)
        {
            Assert(!var.byref || GetObjectType((HGDIOBJ)var.byref) == OBJ_PAL);
            
            if (GetObjectType((HGDIOBJ)var.byref) != OBJ_PAL)
                return NULL;
                
            return (HPALETTE)var.byref;
        }
    }
#endif // ndef WIN16

    return NULL;
}

//+------------------------------------------------------------------------
//
//  Member:     CServer::GetAmbientPropOfType
//
//  Synopsis:   Helper method to get an ambient property of arbitrary
//              type.  Used as GetDispPropOfType.
//
//  Arguments:  [dispid]    --  Property to get
//              [vt]        --  Type of property
//              [pv]        --  Pointer to c data type that receives
//                              the value
//
//  Returns:    HRESULT.  Returns DISP_E_MEMBERNOTFOUND if no client
//              site has been set.
//
//-------------------------------------------------------------------------

HRESULT
CServer::GetAmbientVariant(
        DISPID dispid,
        VARIANT * pvar)
{
    HRESULT     hr;
    IDispatch * pDisp = NULL;

    if (!_pClientSite)
        RRETURN_NOTRACE(DISP_E_MEMBERNOTFOUND);

    hr = THR_NOTRACE(_pClientSite->QueryInterface(
            IID_IDispatch,
            (LPVOID*) &pDisp));
    if (hr)
        RRETURN_NOTRACE(DISP_E_MEMBERNOTFOUND);

    Assert(pDisp);

    hr = THR_NOTRACE(GetDispProp(
                        pDisp,
                        dispid,
                        g_lcidUserDefault,
                        pvar));

    pDisp->Release();

    RRETURN1(hr, DISP_E_MEMBERNOTFOUND);
}

//+-------------------------------------------------------------------------
//
//  Method:     CServer::GetCanUndo
//
//  Synopsis:   Helper to determine whether an undo operation is available.
//
//--------------------------------------------------------------------------

#ifndef NO_EDIT
HRESULT
CServer::GetCanUndo(VARIANT_BOOL * pfCanUndo)
{
    HRESULT     hr;
    BSTR        bstr = NULL;

    if (!pfCanUndo)
        RRETURN(SetErrorInfoInvalidArg());

    hr = THR_NOTRACE(_pUndoMgr->GetLastUndoDescription(&bstr));
    FormsFreeString(bstr);

    *pfCanUndo = (hr == S_OK) ? VB_TRUE : VB_FALSE;

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CServer::GetCanRedo
//
//  Synopsis:   Helper to determine whether a redo operation is available.
//
//--------------------------------------------------------------------------

HRESULT
CServer::GetCanRedo(VARIANT_BOOL * pfCanRedo)
{
    HRESULT     hr;
    BSTR        bstr = NULL;

    if (!pfCanRedo)
        RRETURN(SetErrorInfoInvalidArg());

    hr = THR_NOTRACE(_pUndoMgr->GetLastRedoDescription(&bstr));
    FormsFreeString(bstr);

    *pfCanRedo = (hr == S_OK) ? VB_TRUE : VB_FALSE;

    return S_OK;
}
#endif // NO_EDIT

//+-------------------------------------------------------------------------
//
//  Method:     CServer::RequestUIActivate
//
//  Synopsis:   Ask site if it's ok to UI activate.
//
//--------------------------------------------------------------------------

BOOL
CServer::RequestUIActivate()
{
    Assert(State() >= OS_INPLACE);

    if (State() < OS_UIACTIVE &&
        _pInPlace->_fUseExtendedSite)
    {
        if (((IOleInPlaceSiteEx *)_pInPlace->_pInPlaceSite)->RequestUIActivate() == S_OK)
            return TRUE;

        // Per spec call OnUIDeactivate.
        // Bug 7243: during the request the host could fire eventcode that will change
        // our current state (like making us invisible). That would invalidate the _pInplace
        // pointer
        if (_pInPlace)
        {
            IGNORE_HR(_pInPlace->_pInPlaceSite->OnUIDeactivate(FALSE));
        }
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CServer::SetFocus, public
//
//  Synopsis:   Set the focus to this server.
//
//----------------------------------------------------------------------------

HRESULT
CServer::SetFocus(BOOL fFocus)
{
    HRESULT           hr = S_OK;

    Assert(_pInPlace);
    Assert(State() >= OS_INPLACE);

    if (State() < OS_INPLACE)
    {
        Assert(!fFocus);
        goto Cleanup;
    }

    if (_pInPlace->_fWindowlessInplace)
    {
        hr = THR(((IOleInPlaceSiteWindowless*)
                (_pInPlace->_pInPlaceSite))->SetFocus(fFocus));
    }
    else
    {
        Assert(_pInPlace->_hwnd);
        if (fFocus)
        {
            // Capture the focus.
            ::SetFocus(_pInPlace->_hwnd);
        }
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CServer::GetFocus, public
//
//  Synopsis:   Determines whether the client site has focus
//
//----------------------------------------------------------------------------

BOOL
CServer::GetFocus()
{
    if (_pInPlace->_fWindowlessInplace)
        return (((IOleInPlaceSiteWindowless *) (_pInPlace->_pInPlaceSite))->GetFocus() == S_OK);
    else
        return (_pInPlace->_hwnd == ::GetFocus());
}

//+---------------------------------------------------------------------------
//
//  Member:     CServer::GetCapture, public
//
//  Synopsis:   Find out if we have captured the mouse.
//
//----------------------------------------------------------------------------

BOOL
CServer::GetCapture()
{
    Assert(_pInPlace);
    Assert(State() >= OS_INPLACE);

    if (_pInPlace->_fWindowlessInplace)
    {
        return ((IOleInPlaceSiteWindowless*)
                (_pInPlace->_pInPlaceSite))->GetCapture() == S_OK;
    }
    else
    {
        Assert(_pInPlace->_hwnd);
        return ::GetCapture() == _pInPlace->_hwnd;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CServer::SetCapture, public
//
//  Synopsis:   fCaptured --> Capture the mouse to this server
//              !fCaptured --> This server will not have the capture.
//
//----------------------------------------------------------------------------

HRESULT
CServer::SetCapture(BOOL fCaptured)
{
    HRESULT hr = S_OK;

    if (State() < OS_INPLACE)
    {
        Assert(!fCaptured);
    }
    else if (_pInPlace->_fWindowlessInplace)
    {
        hr = THR(((IOleInPlaceSiteWindowless*)
                (_pInPlace->_pInPlaceSite))->SetCapture(fCaptured));
    }
    else
    {
        Assert(_pInPlace->_hwnd);
        if (fCaptured)
        {
             // Capture the focus.
            ::SetCapture(_pInPlace->_hwnd);
        }
        else if (::GetCapture() == _pInPlace->_hwnd)
        {
            // Release capture if we have it.
#if DBG==1
            Assert(!TLS(fHandleCaptureChanged));
#endif //DBG==1
            ::ReleaseCapture();
        }
    }

    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CServer::GetActivationPolicy
//
//  Synopsis:   Member of IPointerInactive.  Returns bits indicating whether
//              this server should be automatically activated when the
//              mouse passes over it, and deactivated when no longer over.
//
//--------------------------------------------------------------------------

STDMETHODIMP
CServer::GetActivationPolicy(DWORD * pdwPolicy)
{
    *pdwPolicy = 0;

    if (ServerDesc()->TestFlag(SERVERDESC_ACTIVATEONENTRY))
    {
        *pdwPolicy |= POINTERINACTIVE_ACTIVATEONENTRY;
    }

    if (ServerDesc()->TestFlag(SERVERDESC_DEACTIVATEONLEAVE))
    {
        *pdwPolicy |= POINTERINACTIVE_DEACTIVATEONLEAVE;
    }

    if (ServerDesc()->TestFlag(SERVERDESC_ACTIVATEONDRAG))
    {
        *pdwPolicy |= POINTERINACTIVE_ACTIVATEONDRAG;
    }

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CServer::OnInactiveSetCursor
//
//  Synopsis:   Member of IPointerInactive.  Allows the server to set the
//              cursor without requiring activation.
//              Coordinates are in window device units.
//
//  Returns:    S_OK if cursor set.
//              S_FALSE if _mousepointer == fmMousePointerDefault (and does not
//                  set the cursor.)
//
//--------------------------------------------------------------------------

STDMETHODIMP
CServer::OnInactiveSetCursor(LPCRECT pRectBounds,
                             long x, long y, DWORD dwMouseMsg, BOOL fSetAlways)
{
    HRESULT hr;

    hr = SetCursor();

    // if cursor not set yet and required to set something,
    // set standard arrow cursor
    if(fSetAlways && hr == S_FALSE)
    {
        SetCursorIDC(IDC_ARROW);
        hr = S_OK;
    }

    RRETURN1(hr, S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:     CServer::SetCursor
//
//  Synopsis:   Helper function that sets the cursor according to the
//              MousePointer and MouseIcon properties of the CServer.
//
//  Returns:    S_OK if cursor set.
//              S_FALSE if _mousepointer == fmMousePointerDefault (and does not
//                  set the cursor.)
//
//--------------------------------------------------------------------------

HRESULT
CServer::SetCursor()
{
       return S_FALSE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CServer::OnInactiveMouseMove
//
//  Synopsis:   Member of IPointerInactive.  Allows the server to fire
//              mouse move events without going inplace active.
//
//--------------------------------------------------------------------------

STDMETHODIMP
CServer::OnInactiveMouseMove(LPCRECT prc, long x, long y, DWORD grfKeyState)
{
    /*
    POINTL          ptl;
    POINTF          ptf;

    if (ServerDesc()->_sef & SEF_MOUSEMOVE)
    {
        // Convert to document coordinates
        ptl.x = MulDivQuick(x - prc->left, _sizel.cx, prc->right - prc->left);
        ptl.y = MulDivQuick(y -  prc->top, _sizel.cy, prc->bottom -  prc->top);

        UserFromDocument(&ptf, ptl.x, ptl.y);
        IGNORE_HR(FireStdControlEvent_MouseMove(
                VBButtonState((short)grfKeyState),
                VBShiftState(),
                ptf.x,
                ptf.y));
    }
    */
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CServer::GetEnabled
//
//  Synopsis:   Helper method.  Many objects will simply override this.
//
//--------------------------------------------------------------------------

STDMETHODIMP
CServer::GetEnabled(VARIANT_BOOL * pfEnabled)
{
    if (!pfEnabled)
        RRETURN(E_INVALIDARG);

#ifdef WIN16
    //BUGWIN16: I needed to do this to get Java applets a chance to
    //process accelarator keys. - SreeramN.
    Assert(_pInPlace->_hwnd);
    *pfEnabled = (::GetFocus() == _pInPlace->_hwnd) ? VB_TRUE : VB_FALSE;
#else
    *pfEnabled = VB_TRUE;
#endif
    return S_OK;
}



//+-------------------------------------------------------------------------
//
//  Method:     CServer::QueryService, CServer
//
//  Synopsis:   Get service from the inplace or client site.
//
//  Arguments:  guidService     id of service
//              iid             id of interface on service
//              ppv             the service
//
//--------------------------------------------------------------------------

STDMETHODIMP
CServer::QueryService(REFGUID guidService, REFIID iid, void **ppv)
{
    IUnknown *  apUnk[2];
    IUnknown ** ppUnk;
    int         cUnk = 0;
    HRESULT     hr = E_FAIL;            // Assume failure.

    *ppv = NULL;

    //
    // If were are in a doc object and we're inplace (or above), then
    // first see if the service is supported by the InPlaceSite.
    //
    
    if (_fMsoDocMode && _pInPlace && _pInPlace->_pInPlaceSite)
    {
        apUnk[cUnk++] = _pInPlace->_pInPlaceSite;
    }

    // If we have a client site, check it next.
    if (_pClientSite)
    {
        apUnk[cUnk++] = _pClientSite;
    }

    // Now perform the checks.
    ppUnk = apUnk;
    for (cUnk--; cUnk >= 0; cUnk--, ppUnk++)
    {
        IServiceProvider *  pSP;

        hr = THR_NOTRACE((*ppUnk)->QueryInterface(
                IID_IServiceProvider,
                (void **)&pSP));
        if (!hr)
        {
            hr = THR_NOTRACE(pSP->QueryService(guidService, iid, ppv));
            pSP->Release();
            if (!hr)
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN_NOTRACE(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CServer::CLock::CLock
//
//  Synopsis:   Lock resources in CServer object.
//
//-------------------------------------------------------------------------

CServer::CLock::CLock(CServer *pServer, WORD wLockFlags)
{
#if DBG==1
    extern BOOL g_fDisableBaseTrace;
    g_fDisableBaseTrace = TRUE;
#endif

    _pServer = pServer;
    _wLockFlags = pServer->_wLockFlags;
    pServer->_wLockFlags |= wLockFlags | SERVERLOCK_STABILIZED;

#if DBG==1
    if (!IsTagEnabled(tagDisableLockAR))
#endif
    {
        pServer->_pUnkOuter->AddRef();
        pServer->PrivateAddRef();
    }
    
#if DBG==1
    g_fDisableBaseTrace = FALSE;
#endif
}

CServer::CLock::CLock(CServer *pServer)
{
#if DBG==1
    extern BOOL g_fDisableBaseTrace;
    g_fDisableBaseTrace = TRUE;
#endif

    _pServer = pServer;
    _wLockFlags = pServer->_wLockFlags;
    pServer->_wLockFlags |= SERVERLOCK_STABILIZED;

#if DBG==1
    if (!IsTagEnabled(tagDisableLockAR))
#endif
    {
        pServer->_pUnkOuter->AddRef();

        pServer->PrivateAddRef();
    }
    
#if DBG==1
    g_fDisableBaseTrace = FALSE;
#endif
}

//+------------------------------------------------------------------------
//
//  Member:     CServer::CLock::~CLock
//
//  Synopsis:   Unlock resources in CServer object.
//
//-------------------------------------------------------------------------

CServer::CLock::~CLock()
{
#if DBG==1
    extern BOOL g_fDisableBaseTrace;
    g_fDisableBaseTrace = TRUE;
#endif

    // _pServer refcount can go down to zero and then the
    // statement : delete this will be executed. After that
    // _pUnkOuter has no valid value any longer and
    // _pServer->_pUnkOuter->Release() can cause a GPF.
    // Therefore we save the value of _pUnkOuter in a local
    // variable maintaining the original release order.
    // The GPF can be reproduce in VBAPP by putting the
    // Unload Me statement into the UserForm1_Layout event
    // procedure

    IUnknown *  pUnkOuter = _pServer->_pUnkOuter;

    _pServer->_wLockFlags = _wLockFlags;

#if DBG==1
    if (!IsTagEnabled(tagDisableLockAR))
#endif
    {
        _pServer->PrivateRelease();
        pUnkOuter->Release();
    }
    
#if DBG==1
    g_fDisableBaseTrace = FALSE;
#endif
}

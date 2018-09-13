//=--------------------------------------------------------------------------=
// CtlEmbed.Cpp
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// implementation of the interfaces required for inplace activation for
// COleControl
//

#include "IPServer.H"
#include "CtrlObj.H"

#include "CtlHelp.H"
#include "Globals.H"
#include "StdEnum.H"
#include "Util.H"

// for ASSERT and FAIL
//
SZTHISFILE


//=--------------------------------------------------------------------------=
// all controls support the following in-place verbs at an absolute minimum.
//
#define CINPLACEVERBS 4

const VERBINFO rgInPlaceVerbs [] = {
    { OLEIVERB_SHOW,            0, 0, 0},
    { OLEIVERB_HIDE,            0, 0, 0},
    { OLEIVERB_INPLACEACTIVATE, 0, 0, 0},
    { OLEIVERB_PRIMARY,         0, 0, 0}
};

// NOTE: Resource ID for Properties string must be 1000
//
const VERBINFO ovProperties =
    { CTLIVERB_PROPERTIES, 1000, 0, OLEVERBATTRIB_ONCONTAINERMENU };

const VERBINFO ovUIActivate =
    { OLEIVERB_UIACTIVATE, 0, 0, 0};


//=--------------------------------------------------------------------------=
// COleControl::GetControlInfo    (IOleControl)
//=--------------------------------------------------------------------------=
// returns some information on a control, such as an accelerator table, and
// flags.  really used for keyboard handling and mnemonics
//
// Parameters:
//    CONTROLINFO *        - [in]  where to put said information
//
// Output:
//    HRESULT              - S_OK
//
// Notes:
//
STDMETHODIMP COleControl::GetControlInfo
(
    CONTROLINFO *pControlInfo
)
{
    CHECK_POINTER(pControlInfo);

    // certain hosts have a bug in which it doesn't initialize the cb in the
    // CONTROLINFO structure, so we can only assert on that here.
    //
    ASSERT(pControlInfo->cb == sizeof(CONTROLINFO), "Host doesn't initialize CONTROLINFO structure");

    // NOTE: control writers should override this routine if they want to
    // return accelerator information in their control.
    //
    pControlInfo->hAccel = NULL;
    pControlInfo->cAccel = NULL;

    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::OnMnemonic    [IOleControl]
//=--------------------------------------------------------------------------=
// the container has decided to pass on a key that the end-user has pressed to
// us.  default implementation will be to just activate the control.  people
// looking for more functionality should override this method.
//
// Parameters:
//    LPMSG                - [in] message for this mnemonic
//
// Output:
//    HRESULT              - S_OK, E_POINTER
//
// Notes:
//
STDMETHODIMP COleControl::OnMnemonic
(
    LPMSG pMsg
)
{
    // OVERRIDE: default implementation is to just activate our control.  
    // user can override if they want more interesting behaviour.
    //
    return InPlaceActivate(OLEIVERB_UIACTIVATE);
}

//=--------------------------------------------------------------------------=
// COleControl:OnAmbientPropertyChange    [IOleControl]
//=--------------------------------------------------------------------------=
// a container calls this whenever it changes an ambient property.
//
// Parameters:
//    DISPID            - [in] dispid of the property that changed.
//
// Output:
//    HRESULT           - S_OK
//
// Notes:
//
STDMETHODIMP COleControl::OnAmbientPropertyChange
(
    DISPID dispid
)
{
    // if we're being told about a change in mode [design/run] then
    // remember that so our stashing of mode will update itself
    // correctly
    //
    if (dispid == DISPID_AMBIENT_USERMODE || dispid == DISPID_UNKNOWN)
        m_fModeFlagValid = FALSE;

    // just pass this on to the derived control and see if they want
    // to do anything with it.
    //
    AmbientPropertyChanged(dispid);
    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControL::FreezeEvents    [IOleControl]
//=--------------------------------------------------------------------------=
// allows a container to freeze all of a controls events.  when events are
// frozen, a control will not fire any of them.
//
// Parameters:
//    BOOL            - [in] TRUE means FREEZE, FALSE means THAW
//
// Output:
//    HRESULT         - S_OK
//
// Notes:
//    - we maintain an internal count of freezes versus thaws.
//
STDMETHODIMP COleControl::FreezeEvents
(
    BOOL fFreeze
)
{
    // OVERRIDE: by default, we don't care.  user can override if they want to.
    //
    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::SetClientSite    [IOleObject]
//=--------------------------------------------------------------------------=
// informs the embedded object [control] of it's client site [display
// location] within it's container
//
// Parameters:
//    IOleClientSite *        - [in] pointer to client site.
//
// Output:
//    HRESULT                 - S_OK, E_UNEXPECTED
//
// Notes:
//
STDMETHODIMP COleControl::SetClientSite
(
    IOleClientSite *pClientSite
)
{
    RELEASE_OBJECT(m_pClientSite);
    RELEASE_OBJECT(m_pControlSite);
    RELEASE_OBJECT(m_pSimpleFrameSite);

    // store away the new client site
    //
    m_pClientSite = pClientSite;

    // if we've actually got one, then get some other interfaces we want to keep
    // around, and keep a handle on it
    //
    if (m_pClientSite) {
        m_pClientSite->AddRef();
        m_pClientSite->QueryInterface(IID_IOleControlSite, (void **)&m_pControlSite);

        if (OLEMISCFLAGSOFCONTROL(m_ObjectType) & OLEMISC_SIMPLEFRAME)
            m_pClientSite->QueryInterface(IID_ISimpleFrameSite, (void **)&m_pSimpleFrameSite);
    }

    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::GetClientSite    [IOleObject]
//=--------------------------------------------------------------------------=
// obtains a pointer to the controls client site.
//
// Parameters:
//    IOleClientSite **        - [out]
//
// Output:
//    HRESULT                  - S_OK
//
// Notes:
//
STDMETHODIMP COleControl::GetClientSite
(
    IOleClientSite **ppClientSite
)
{
    CHECK_POINTER(ppClientSite);

    *ppClientSite = m_pClientSite;
    ADDREF_OBJECT(*ppClientSite);
    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::SetHostNames    [IOleObject]
//=--------------------------------------------------------------------------=
// Provides the control with the name of its container application and the
// compound document in which it is embedded
//
// Parameters:
//    LPCOLESTR        - [in] name of container application
//    LPCOLESTR        - [in] name of container document
//
// Output:
//    HRESULT          - S_OK
//
// Notes:
//    - we don't care about this
//
STDMETHODIMP COleControl::SetHostNames
(
    LPCOLESTR szContainerApp,
    LPCOLESTR szContainerObject
)
{
    // we don't care about these
    //
    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::Close    [IOleObject]
//=--------------------------------------------------------------------------=
// Changes the control from the running to the loaded state
//
// Parameters:
//    DWORD             - [in] indicates whether to save the object before closing
//
// Output:
//    HRESULT           - S_OK, OLE_E_PROMPTSAVECANCELLED
//
// Notes:
//
STDMETHODIMP COleControl::Close
(
    DWORD dwSaveOption
)
{
    HRESULT hr;

    if (m_fInPlaceActive) {
        hr = InPlaceDeactivate();
        RETURN_ON_FAILURE(hr);
    }

    // handle the save flag.
    //
    if ((dwSaveOption == OLECLOSE_SAVEIFDIRTY || dwSaveOption == OLECLOSE_PROMPTSAVE) && m_fDirty) {
        if (m_pClientSite) m_pClientSite->SaveObject();
        if (m_pOleAdviseHolder) m_pOleAdviseHolder->SendOnSave();
    }
    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::SetMoniker    [IOleObject]
//=--------------------------------------------------------------------------=
// Notifies an object of its container's moniker, the object's own moniker
// relative to the container, or the object's full moniker
//
// Parameters:
//    DWORD                - [in] which moniker is being set
//    IMoniker *           - [in] the moniker
//
// Output:
//    HRESULT              - S_OK, E_FAIL
//
// Notes:
//    - we don't support monikers.
//
STDMETHODIMP COleControl::SetMoniker
(
    DWORD     dwWhichMoniker,
    IMoniker *pMoniker
)
{
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// COleControl::GetMoniker    [IOleObject]
//=--------------------------------------------------------------------------=
// Returns a embedded object's moniker, which the caller can use to link to
// the object
//
// Parameters:
//    DWORD            - [in]  how it's assigned
//    DWORD            - [in]  which moniker
//    IMoniker **      - [out] duh.
//
// Output:
//    HRESULT          - E_NOTIMPL
//
// Notes:
//    - we don't support monikers
//
STDMETHODIMP COleControl::GetMoniker
(
    DWORD      dwAssign,
    DWORD      dwWhichMoniker,
    IMoniker **ppMonikerOut
)
{
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// COleControl::InitFromData    [IOleObject]
//=--------------------------------------------------------------------------=
// Initializes a newly created object with data from a specified data object,
// which can reside either in the same container or on the Clipboard
//
// Parameters:
//    IDataObject*    - [in] data object with the data
//    BOOL            - [in] how object is created
//    DWORD           - reserved
//
// Output:
//    HRESULT         - S_OK, S_FALSE, E_NOTIMPL, OLE_E_NOTRUNNING
//
// Notes:
//    - we don't have data object support
//
STDMETHODIMP COleControl::InitFromData
(
    IDataObject *pDataObject,
    BOOL         fCreation,
    DWORD        dwReserved
)
{
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// COleControl::GetClipboardData    [IOleObject]
//=--------------------------------------------------------------------------=
// Retrieves a data object containing the current contents of the control.
// Using the pointer to this data object, it is possible to create a new control
// with the same data as the original
//
// Parameters:
//    DWORD          - reserved
//    IDataObject ** - [out] data object for this control
//
// Output:
//    HREUSLT        - S_OK, E_NOTIMPL, OLE_E_NOTRUNNING
//
// Notes:
//
STDMETHODIMP COleControl::GetClipboardData
(
    DWORD         dwReserved,
    IDataObject **ppDataObject
)
{
    *ppDataObject = NULL;        // be a good neighbour
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// COleControl::DoVerb    [IOleObject]
//=--------------------------------------------------------------------------=
// Requests an object to perform an action in response to an end-user's
// action.
//
// Parameters:
//    LONG             - [in]  verb to be performed
//    LPMSG            - [in]  event that invoked the verb
//    IOleClientSite * - [in]  the controls active client site
//    LONG             - [in]  reserved
//    HWND             - [in]  handle of window containing the object.
//    LPCRECT          - [in]  pointer to objects's display rectangle
//
// Output:
//    HRESULT          - S_OK, OLE_E_NOTINPLACEACTIVE, OLE_E_CANT_BINDTOSOURCE,
//                       DV_E_LINK, OLEOBJ_S_CANNOT_DOVERB_NOW, OLEOBJ_S_INVALIDHWND,
//                       OLEOBJ_E_NOVERBS, OLEOBJ_S_INVALIDVERB, MK_E_CONNECT,
//                       OLE_CLASSDIFF, E_NOTIMPL
//
// Notes:
//
STDMETHODIMP COleControl::DoVerb
(
    LONG            lVerb,
    LPMSG           pMsg,
    IOleClientSite *pActiveSite,
    LONG            lIndex,
    HWND            hwndParent,
    LPCRECT         prcPosRect
)
{
    HRESULT hr;

    switch (lVerb) {
      case OLEIVERB_SHOW:
      case OLEIVERB_INPLACEACTIVATE:
      case OLEIVERB_UIACTIVATE:
        hr = InPlaceActivate(lVerb);
        OnVerb(lVerb);
        return (hr);

      case OLEIVERB_HIDE:
        UIDeactivate();
        if (m_fInPlaceVisible) SetInPlaceVisible(FALSE);
        OnVerb(lVerb);
        return S_OK;

      // we used to have OLEIVERB_PRIMARY InPlaceActivate Ourselves, but it
      // turns out that the CDK and certain hosts expect this to show the
      // properties instead.  Users can change what this verb does at will.
      //
      case OLEIVERB_PRIMARY:
      case CTLIVERB_PROPERTIES:
      case OLEIVERB_PROPERTIES:
        {
        // show the frame ourselves if the hose can't.
        //
        if (m_pControlSite) {
            hr = m_pControlSite->ShowPropertyFrame();
            if (hr != E_NOTIMPL)
                return hr;
        }
        IUnknown *pUnk = (IUnknown *)(IOleObject *)this;
        MAKE_WIDEPTR_FROMANSI(pwsz, NAMEOFOBJECT(m_ObjectType));

        ModalDialog(TRUE);
        hr = OleCreatePropertyFrame(GetActiveWindow(),
                            GetSystemMetrics(SM_CXSCREEN) / 2,
                            GetSystemMetrics(SM_CYSCREEN) / 2,
                            pwsz,
                            1,
                            &pUnk,
                            CPROPPAGESOFCONTROL(m_ObjectType),
                            (LPCLSID)*(PPROPPAGESOFCONTROL(m_ObjectType)),
                            g_lcidLocale,
                            NULL, NULL);
        ModalDialog(FALSE);
        return hr;
        }

      default:
        // if it's a derived-control defined verb, pass it on to them
        //
        if (lVerb > 0) {
            hr = DoCustomVerb(lVerb);

            if (hr == OLEOBJ_S_INVALIDVERB) {
                // unrecognised verb -- just do the primary verb and
                // activate the sucker.
                //
                hr = InPlaceActivate(OLEIVERB_PRIMARY);
                return (FAILED(hr)) ? hr : OLEOBJ_S_INVALIDVERB;
            } else
                return hr;
        } else {
            FAIL("Unrecognized Negative verb in DoVerb().  bad.");
            return E_NOTIMPL;
        }
        break;
    }

    // dead code
    FAIL("this should be dead code!");
}

//=--------------------------------------------------------------------------=
// COleControl::EnumVerbs    [IOleObject]
//=--------------------------------------------------------------------------=
// create an enumerator object for the verbs this object supports.
//
// Parameters:
//    IEnumOleVERB **    - [out] new enumerator.
//
// Output:
//    HRESULT            - S_OK, E_OUTOFMEMORY
//
// Notes:
//
STDMETHODIMP COleControl::EnumVerbs
(
    IEnumOLEVERB **ppEnumVerbs
)
{
    int cVerbs;
    OLEVERB *rgVerbs, *pVerb;

    DWORD dw = OLEMISCFLAGSOFCONTROL(m_ObjectType);
    BOOL fCanInPlace = !(dw & OLEMISC_INVISIBLEATRUNTIME) || (dw & OLEMISC_ACTIVATEWHENVISIBLE);
    BOOL fCanUIActivate = !(dw & OLEMISC_NOUIACTIVATE);
    BOOL fHasProperties = (CPROPPAGESOFCONTROL(m_ObjectType) != 0);

    int cVerbExtra = CCUSTOMVERBSOFCONTROL(m_ObjectType);

    // count up all the verbs
    //
    cVerbs = (fCanInPlace ? CINPLACEVERBS : 0) + (fCanUIActivate ? 1 : 0)
             + (fHasProperties ? 1 : 0) + cVerbExtra;

    // if there aren't any, this suddenly gets really easy !
    //
    if (cVerbs == 0)
        return OLEOBJ_E_NOVERBS;

    // HeapAlloc some storage for these dudes so that we can pass them on to
    // the standard enumerator!
    //
    if (! (rgVerbs = (OLEVERB *)HeapAlloc(g_hHeap, 0, cVerbs * sizeof(OLEVERB))))
        return E_OUTOFMEMORY;
  
    // start copying over verbs.  first, the in-place guys
    //
    pVerb = rgVerbs;
    if (fCanInPlace) {
        memcpy(pVerb, rgInPlaceVerbs, CINPLACEVERBS * sizeof(OLEVERB));
        pVerb += CINPLACEVERBS;
      }

    if (fCanUIActivate)
        memcpy(pVerb++, &ovUIActivate, sizeof(OLEVERB));

    // if their control has properties, copy that over now.
    //
    if (fHasProperties) {
        memcpy(pVerb, &ovProperties, sizeof(OLEVERB));
        pVerb++;
    }

    // finally, any custom verbs!
    //
    if (cVerbExtra) {
        memcpy(pVerb, CUSTOMVERBSOFCONTROL(m_ObjectType), sizeof(OLEVERB) * cVerbExtra);
    }

    *ppEnumVerbs = (IEnumOLEVERB *) (IEnumGeneric *) new CStandardEnum(IID_IEnumOLEVERB,
                                     cVerbs, sizeof(OLEVERB), rgVerbs, CopyOleVerb);
    if (!*ppEnumVerbs)
        return E_OUTOFMEMORY;

    // this forces us to go and look for the Localized DLLs.  This is necessary here
    // because the CopyOleVerb will get information from localized resources, but
    // will only use the global GetResourceHandle, which only uses the global value
    // for the LCID.  This turns out to not be a big performance hit, since this
    // function is typically only called in design mode, and we stash this value.
    //
    GetResourceHandle();
    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::Update    [IOleObject]
//=--------------------------------------------------------------------------=
// Updates an object handler's or link object's data or view caches.
//
// Output:
//    HRESULT            - S_OK
//
// Notes:
//
STDMETHODIMP COleControl::Update
(
    void
)
{
    // nothing to do!!!
    //
    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::IsUpToDate    [IOleObject]
//=--------------------------------------------------------------------------=
// Checks recursively whether or not an object is up to date.
//
// Output:
//    HRESULT        - S_OK, S_FALSE, OLE_E_UNVAILABLE
//
// Notes:
//
STDMETHODIMP COleControl::IsUpToDate
(
    void
)
{
    // we're always up to date
    //
    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::GetUserClassID    [IOleObject]
//=--------------------------------------------------------------------------=
// Returns the controls class identifier, the CLSID corresponding to the
// string identifying the object to an end user.
//
// Parameters:
//    CLSID *      - [in] where to put the CLSID
//
// Output:
//    HRESULT      - S_OK, E_FAIL
//
// Notes:
//
STDMETHODIMP COleControl::GetUserClassID
(
    CLSID *pclsid
)
{
    // this is the same as IPersist::GetClassID
    //
    return GetClassID(pclsid);
}

//=--------------------------------------------------------------------------=
// COleControl::GetUserType    [IOleObject]
//=--------------------------------------------------------------------------=
// Retrieves the user-type name of the control for display in user-interface
// elements such as menus, list boxes, and dialog boxes.
//
// Parameters:
//    DWORD        - [in]  specifies the form of the type name.
//    LPOLESTR *   - [out] where to put user type
//
// Output:
//    HRESULT      - S_OK, OLE_S_USEREG, E_OUTOFMEMORY
//
// Notes:
//
STDMETHODIMP COleControl::GetUserType
(
    DWORD     dwFormOfType,
    LPOLESTR *ppszUserType
)
{
    *ppszUserType = OLESTRFROMANSI(NAMEOFOBJECT(m_ObjectType));
    return (*ppszUserType) ? S_OK : E_OUTOFMEMORY;
}

//=--------------------------------------------------------------------------=
// COleControl::SetExtent    [IOleObject]
//=--------------------------------------------------------------------------=
// Informs the control of how much display space its container has assigned it.
//
// Parameters:
//    DWORD            - [in] which form or 'aspect'  is to be displayed.
//    SIZEL *          - [in] size limit for the control.
//
// Output:
//    HRESULT          - S_OK, E_FAIL, OLE_E_NOTRUNNING
//
// Notes:
//
STDMETHODIMP COleControl::SetExtent
(
    DWORD  dwDrawAspect,
    SIZEL *psizel
)
{
    SIZEL sl;
    RECT  rect;
    BOOL  f;

    if (dwDrawAspect & DVASPECT_CONTENT) {

        // change the units to pixels, and resize the control.
        //
        HiMetricToPixel(psizel, &sl);

        // first call the user version.  if they return FALSE, they want
        // to keep their current size
        //
        f = OnSetExtent(&sl);
        if (f)
            HiMetricToPixel(psizel, &m_Size);

        // set things up with our HWND if we've got one.
        //
        if (!m_pInPlaceSiteWndless) {
            if (m_fInPlaceActive) {
    
                // theoretically, one should not need to call OnPosRectChange
                // here, but there appear to be a few host related issues that
                // will make us keep it here.  we won't, however, both with
                // windowless ole controls, since they are all new hosts who
                // should know better
                //
                GetWindowRect(m_hwnd, &rect);
                MapWindowPoints(NULL, m_hwndParent, (LPPOINT)&rect, 2);
                rect.right = rect.left + m_Size.cx;
                rect.bottom = rect.top + m_Size.cy;
                m_pInPlaceSite->OnPosRectChange(&rect);
    
                if (m_hwnd) {
                    // just go and resize
                    //
                    if (m_hwndReflect)
                        SetWindowPos(m_hwndReflect, 0, 0, 0, m_Size.cx, m_Size.cy,
                                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
                    SetWindowPos(m_hwnd, 0, 0, 0, m_Size.cx, m_Size.cy,
                                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
                }
            } else if (m_hwnd) {
                SetWindowPos(m_hwnd, NULL, 0, 0, m_Size.cx, m_Size.cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            } else {
                ViewChanged();
            }
        } else
            if (m_pInPlaceSite) m_pInPlaceSite->OnPosRectChange(&rect);

        // return code depending on whether or not user accepted given
        // size
        //
        return (f) ? S_OK : E_FAIL;

    } else {
        // we don't support any other aspects.
        //
        return DV_E_DVASPECT;
    }

    // dead code
    FAIL("This should be dead code");
}

//=--------------------------------------------------------------------------=
// COleControl::GetExtent    [IOleObject]
//=--------------------------------------------------------------------------=
// Retrieves the control's current display size.
//
// Parameters:
//    DWORD            - [in] aspect
//    SIZEL *          - [in] where to put results
//
// Output:
//    S_OK, E_INVALIDARG
//
// Notes:
//
STDMETHODIMP COleControl::GetExtent
(
    DWORD  dwDrawAspect,
    SIZEL *pSizeLOut
)
{

    if (dwDrawAspect & DVASPECT_CONTENT) {
        PixelToHiMetric((const SIZEL *)&m_Size, pSizeLOut);
        return S_OK;
    } else {
        return DV_E_DVASPECT;
    }

    // dead code
}

//=--------------------------------------------------------------------------=
// COleControl::Advise    [IOleObject]
//=--------------------------------------------------------------------------=
// establishes and advisory connection between the control and the container,
// in which the control will notify the container of certain events.
//
// Parameters:
//    IAdviseSink *     - [in]  advise sink of calling object
//    DWORD             - [out] cookie
//
// Output:
//    HRESULT           - S_OK, E_OUTOFMEMORY
//
// Notes:
//
STDMETHODIMP COleControl::Advise
(
    IAdviseSink *pAdviseSink,
    DWORD       *pdwConnection
)
{
    HRESULT hr;

    // if we haven't yet created a standard advise holder object, do so
    // now
    //
    if (!m_pOleAdviseHolder) {
        hr = CreateOleAdviseHolder(&m_pOleAdviseHolder);
        RETURN_ON_FAILURE(hr);
    }

    // just get it to do the work for us!
    //
    return m_pOleAdviseHolder->Advise(pAdviseSink, pdwConnection);
}

//=--------------------------------------------------------------------------=
// COleControl::Unadvise    [IOleObject]
//=--------------------------------------------------------------------------=
// Deletes a previously established advisory connection.
//
// Parameters:
//    DWORD         - [in] connection cookie
//
// Output:
//    HRESULT       - S_OK, E_FAIL, OLE_E_NOCONNECTION
//
// Notes:
//
STDMETHODIMP COleControl::Unadvise
(
    DWORD dwConnection
)
{
    if (!m_pOleAdviseHolder) {
        FAIL("Somebody called Unadvise on IOleObject without calling Advise!");
        CONNECT_E_NOCONNECTION;
    }

    return m_pOleAdviseHolder->Unadvise(dwConnection);
}

//=--------------------------------------------------------------------------=
// COleControl::EnumAdvise    [IOleObject]
//=--------------------------------------------------------------------------=
// Enumerates the advisory connections registered for an object, so a container
// can know what to release prior to closing down.
//
// Parameters:
//    IEnumSTATDATA **    - [out] where to put enumerator
//
// Output:
//    HRESULT             - S_OK, E_FAIL, E_NOTIMPL
//
// Notes:
//
STDMETHODIMP COleControl::EnumAdvise
(
    IEnumSTATDATA **ppEnumOut
)
{
    if (!m_pOleAdviseHolder) {
        FAIL("Somebody Called EnumAdvise without setting up any connections");
        *ppEnumOut = NULL;
        return E_FAIL;
    }

    return m_pOleAdviseHolder->EnumAdvise(ppEnumOut);
}

//=--------------------------------------------------------------------------=
// COleControl::GetMiscStatus    [IOleObject]
//=--------------------------------------------------------------------------=
// Returns a value indicating the status of an object at creation and loading.
//
// Parameters:
//    DWORD         - [in]  aspect desired
//    DWORD *       - [out] where to put the bits.
//
// Output:
//    HRESULT       - S_OK, OLE_S_USEREG, CO_E_CLASSNOTREG, CO_E_READREGDB
//
// Notes:
//
STDMETHODIMP COleControl::GetMiscStatus
(
    DWORD  dwAspect,
    DWORD *pdwStatus
)
{
    CHECK_POINTER(pdwStatus);

    if (dwAspect == DVASPECT_CONTENT) {
        *pdwStatus = OLEMISCFLAGSOFCONTROL(m_ObjectType);
        return S_OK;
    } else {
        return DV_E_DVASPECT;
    }

    // dead code
}

//=--------------------------------------------------------------------------=
// COleControl::SetColorScheme    [IOleObject]
//=--------------------------------------------------------------------------=
// Specifies the color palette that the object application should use when it
// edits the specified object.
//
// Parameters:
//    LOGPALETTE *     - [in] new palette
//
// Output:
//    HRESULT          - S_OK, E_NOTIMPL, OLE_E_PALETTE, OLE_E_NOTRUNNING
//
// Notes:
//    - we don't care.
//
STDMETHODIMP COleControl::SetColorScheme
(
    LOGPALETTE *pLogpal
)
{
    // OVERRIDE: control writers can use this if they want to
    //
    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::GetWindow    [IOleWindow/IOleInPlaceObject]
//=--------------------------------------------------------------------------=
// Returns the window handle to one of the windows participating in in-place
// activation (frame, document, parent, or in-place object window).
//
// Parameters:
//    HWND *        - [out] where to return window handle.
//
// Output:
//    HRESULT       - S_OK, E_INVALIDARG, E_OUTOFMEMORY, E_UNEXPECTED, E_FAIL
//
// Notes:
//    - this routine has slightly different semantics for windowless controls
//
STDMETHODIMP COleControl::GetWindow
(
    HWND *phwnd
)
{
    // if we're windowles, then we want to return E_FAIL for this so hosts
    // know we're windowless
    //
    if (m_pInPlaceSiteWndless)
        return E_FAIL;

    // otherwise, just return our outer window.
    //
    *phwnd = GetOuterWindow();

    return (*phwnd) ? S_OK : E_UNEXPECTED;
}

//=--------------------------------------------------------------------------=
// COleControl::ContextSensitiveHelp    [IOleWindow/IOleInPlaceObject]
//=--------------------------------------------------------------------------=
// Determines whether context-sensitive help mode should be entered during an
// in-place activation session.
//
// Parameters:
//    BOOL            - [in] whether or not to enter help mode.
//
// Output:
//    HRESULT         - S_OK, E_INVALIDARG, E_OUTOFMEMORY, E_UNEXPECTED
//
// Notes:
//
STDMETHODIMP COleControl::ContextSensitiveHelp
(
    BOOL fEnterMode
)
{
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// COleControl::InPlaceActivate
//=--------------------------------------------------------------------------=
// activates the control, and depending on the verb, optionally ui activates
// it as well.
//
// Parameters:
//    LONG         - [in] the verb that caused us to activate
//
// Output:
//    HRESULT
//
// Notes:
//    - this is spaghetti code at it's worst.  effectively, we have to
//      be able to handle three types of site pointers -- IOleInPlaceSIte,
//      IOleInPlaceSiteEx, and IOleInPlaceSiteWindowless.  not terribly
//      pretty.
//
HRESULT COleControl::InPlaceActivate
(
    LONG lVerb
)
{
    BOOL f;
    SIZEL sizel;
    IOleInPlaceSiteEx *pIPSEx = NULL;
    HRESULT hr;
    BOOL    fNoRedraw = FALSE;

    // if we don't have a client site, then there's not much to do.
    //
    if (!m_pClientSite)
        return S_OK;

    // get an InPlace site pointer.
    //
    if (!GetInPlaceSite()) {

        // if they want windowless support, then we want IOleInPlaceSiteWindowless
        //
        if (FCONTROLISWINDOWLESS(m_ObjectType))
            m_pClientSite->QueryInterface(IID_IOleInPlaceSiteWindowless, (void **)&m_pInPlaceSiteWndless);

        // if we're not able to do windowless siting, then we'll just get an
        // IOleInPlaceSite pointer.
        //
        if (!m_pInPlaceSiteWndless) {
            hr = m_pClientSite->QueryInterface(IID_IOleInPlaceSite, (void **)&m_pInPlaceSite);
            RETURN_ON_FAILURE(hr);
        }
    }

    // now, we want an IOleInPlaceSiteEx pointer for windowless and flicker free
    // activation.  if we're windowless, we've already got it, else we need to
    // try and get it
    //
    if (m_pInPlaceSiteWndless) {
        pIPSEx = (IOleInPlaceSiteEx *)m_pInPlaceSiteWndless;
        pIPSEx->AddRef();
    } else
        m_pClientSite->QueryInterface(IID_IOleInPlaceSiteEx, (void **)&pIPSEx);

    // if we're not already active, go and do it.
    //
    if (!m_fInPlaceActive) {
        OLEINPLACEFRAMEINFO InPlaceFrameInfo;
        RECT rcPos, rcClip;

        // if we have a windowless site, see if we can go in-place windowless
        // active
        //
        hr = S_FALSE;
        if (m_pInPlaceSiteWndless) {
            hr = m_pInPlaceSiteWndless->CanWindowlessActivate();
            CLEANUP_ON_FAILURE(hr);

            // if they refused windowless, we'll try windowed
            //
            if (S_OK != hr) {
                RELEASE_OBJECT(m_pInPlaceSiteWndless);
                hr = m_pClientSite->QueryInterface(IID_IOleInPlaceSite, (void **)&m_pInPlaceSite);
                CLEANUP_ON_FAILURE(hr);
            }
        }

        // just try regular windowed in-place activation
        //
        if (hr != S_OK) {
            hr = m_pInPlaceSite->CanInPlaceActivate();
            if (hr != S_OK) {
                hr = (FAILED(hr)) ? E_FAIL : hr;
                goto CleanUp;
            }
        }

        // if we are here, then we have permission to go in-place active.
        // now, announce our intentions to actually go ahead and do this.
        //
        hr = (pIPSEx) ? pIPSEx->OnInPlaceActivateEx(&fNoRedraw, (m_pInPlaceSiteWndless) ? ACTIVATE_WINDOWLESS : 0)
                       : m_pInPlaceSite->OnInPlaceActivate();
        CLEANUP_ON_FAILURE(hr);

        // if we're here, we're ready to go in-place active.  we just need
        // to set up some flags, and then create the window [if we have
        // one]
        //
        m_fInPlaceActive = TRUE;

        // we need to get some information about our location in the parent
        // window, as well as some information about the parent
        //
        InPlaceFrameInfo.cb = sizeof(OLEINPLACEFRAMEINFO);
        hr = GetInPlaceSite()->GetWindow(&m_hwndParent);
        if (SUCCEEDED(hr))
            hr = GetInPlaceSite()->GetWindowContext(&m_pInPlaceFrame, &m_pInPlaceUIWindow, &rcPos, &rcClip, &InPlaceFrameInfo);
        CLEANUP_ON_FAILURE(hr);

        // make sure we'll display ourselves in the correct location with the correct size
        //
        sizel.cx = rcPos.right - rcPos.left;
        sizel.cy = rcPos.bottom - rcPos.top;
        f = OnSetExtent(&sizel);
        if (f) m_Size = sizel;
        SetObjectRects(&rcPos, &rcClip);

        // finally, create our window if we have to!
        //
        if (!m_pInPlaceSiteWndless) {
    
            SetInPlaceParent(m_hwndParent);

            // create the window, and display it.  die horribly if we couldnt'
            //
            if (!CreateInPlaceWindow(rcPos.left, rcPos.top, fNoRedraw)) {
                hr = E_FAIL;
                goto CleanUp;
            }
        }
    }

    // don't need this any more
    //
    RELEASE_OBJECT(pIPSEx);

    // if we're not inplace visible yet, do so now.
    //
    if (!m_fInPlaceVisible)
        SetInPlaceVisible(TRUE);

    // if we weren't asked to UIActivate, then we're done.
    //
    if (lVerb != OLEIVERB_PRIMARY && lVerb != OLEIVERB_UIACTIVATE)
        return S_OK;

    // if we're not already UI active, do sow now.
    //
    if (!m_fUIActive) {
        m_fUIActive = TRUE;

        // inform the container of our intent
        //
        GetInPlaceSite()->OnUIActivate();

        // take the focus  [which is what UI Activation is all about !]
        //
        SetFocus(TRUE);

        // set ourselves up in the host.
        //
        m_pInPlaceFrame->SetActiveObject((IOleInPlaceActiveObject *)this, NULL);
        if (m_pInPlaceUIWindow)
            m_pInPlaceUIWindow->SetActiveObject((IOleInPlaceActiveObject *)this, NULL);

        // we have to explicitly say we don't wany any border space.
        //
        m_pInPlaceFrame->SetBorderSpace(NULL);
        if (m_pInPlaceUIWindow)
            m_pInPlaceUIWindow->SetBorderSpace(NULL);
    }

    // be-de-be-de-be-de that's all folks!
    //
    return S_OK;

  CleanUp:
    // something catastrophic happened [or, at least something bad].
    // die a horrible fiery mangled painful death.
    //
    QUICK_RELEASE(pIPSEx);
    m_fInPlaceActive = FALSE;
    return hr;

}

//=--------------------------------------------------------------------------=
// COleControl::InPlaceDeactivate    [IOleInPlaceObject]
//=--------------------------------------------------------------------------=
// Deactivates an active in-place object and discards the object's undo state.
//
// Output:
//    HRESULT        - S_OK, E_UNEXPECTED
//
// Notes:
//
STDMETHODIMP COleControl::InPlaceDeactivate
(
    void
)
{
    // if we're not in-place active yet, then this is easy.
    //
    if (!m_fInPlaceActive)
        return S_OK;

    // transition from UIActive back to active
    //
    if (m_fUIActive)
        UIDeactivate();

    m_fInPlaceActive = FALSE;
    m_fInPlaceVisible = FALSE;

    // if we have a window, tell it to go away.
    //
    if (m_hwnd) {
        ASSERT(!m_pInPlaceSiteWndless, "internal state really messed up");

        // so our window proc doesn't crash.
        //
        BeforeDestroyWindow();
        SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)0xFFFFFFFF);
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;

        if (m_hwndReflect) {
            SetWindowLongPtr(m_hwndReflect, GWLP_USERDATA, 0);
            DestroyWindow(m_hwndReflect);
            m_hwndReflect = NULL;
        }
    }

    RELEASE_OBJECT(m_pInPlaceFrame);
    RELEASE_OBJECT(m_pInPlaceUIWindow);
    GetInPlaceSite()->OnInPlaceDeactivate();
    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::UIDeactivate    [IOleInPlaceObject]
//=--------------------------------------------------------------------------=
// transitions us from UI Active to merely being active [visible]  for
// a control, this doesn't mean all that much.
//
// Output:
//    HRESULT         - S_OK, E_UNEXPECTED
//
// Notes:
//
STDMETHODIMP COleControl::UIDeactivate
(
    void
)
{
    // if we're not UIActive, not much to do.
    //
    if (!m_fUIActive)
        return S_OK;

    m_fUIActive = FALSE;

    // notify frame windows, if appropriate, that we're no longer ui-active.
    //
    if (m_pInPlaceUIWindow) m_pInPlaceUIWindow->SetActiveObject(NULL, NULL);
    m_pInPlaceFrame->SetActiveObject(NULL, NULL);

    // we don't need to explicitly release the focus here since somebody
    // else grabbing the focus is what is likely to cause us to get lose it
    //
    GetInPlaceSite()->OnUIDeactivate(FALSE);

    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::SetObjectRects    [IOleInPlaceObject]
//=--------------------------------------------------------------------------=
// Indicates how much of the control is visible.
//
// Parameters:
//    LPCRECT        - [in] position of the control.
//    LPCRECT        - [in] clipping rectangle for the control.
//
// Output:
//    HRESULT        - S_OK, E_INVALIDARG, E_OUTOFMEMORY, E_UNEXPECTED
//
// Notes:
//
STDMETHODIMP COleControl::SetObjectRects
(
    LPCRECT prcPos,
    LPCRECT prcClip
)
{
    BOOL fRemoveWindowRgn;

    // move our window to the new location and handle clipping. not applicable
    // for windowless controls, since the container will be responsible for all
    // clipping.
    //
    if (m_hwnd) {
        fRemoveWindowRgn = m_fUsingWindowRgn;
        if (prcClip) {
            // the container wants us to clip, so figure out if we really
            // need to
            //
            RECT rcIXect;
            if ( IntersectRect(&rcIXect, prcPos, prcClip) ) {
                if (!EqualRect(&rcIXect, prcPos)) {
                    OffsetRect(&rcIXect, -(prcPos->left), -(prcPos->top));

                    HRGN tempRgn = CreateRectRgnIndirect(&rcIXect);
                    SetWindowRgn(GetOuterWindow(), tempRgn, TRUE);

                    if (m_hRgn != NULL)
                       DeleteObject(m_hRgn);
                    m_hRgn = tempRgn;

                    m_fUsingWindowRgn = TRUE;
                    fRemoveWindowRgn  = FALSE;
                }
            }
        }

        if (fRemoveWindowRgn) {
            SetWindowRgn(GetOuterWindow(), NULL, TRUE);
            if (m_hRgn != NULL)
            {
               DeleteObject(m_hRgn);
               m_hRgn = NULL;
            }
            m_fUsingWindowRgn = FALSE;
        }

        // set our control's location, but don't change it's size at all
        // [people for whom zooming is important should set that up here]
        //
        DWORD dwFlag;
        OnSetObjectRectsChangingWindowPos(&dwFlag);

        int cx, cy;
        cx = prcPos->right - prcPos->left;
        cy = prcPos->bottom - prcPos->top;
        SetWindowPos(GetOuterWindow(), NULL, prcPos->left, prcPos->top, cx, cy, dwFlag | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // save out our current location.  windowless controls want this more
    // that windowed ones do, but everybody can have it just in case
    //
    m_rcLocation = *prcPos;

    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::ReactivateAndUndo    [IOleInPlaceObject]
//=--------------------------------------------------------------------------=
// Reactivates a previously deactivated object, undoing the last state of the object.
//
// Output:
//    HRESULT        - S_OK, E_NOTUNDOABLE
//
// Notes:
//
STDMETHODIMP COleControl::ReactivateAndUndo
(
    void
)
{
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// COleControl::OnWindowMessage    [IOleInPlaceObjectWindowless]
//=--------------------------------------------------------------------------=
// this method lets the container dispatch a message to a windowless OLE
// object.
//
// Parameters:
//    UINT                - [in]  the message
//    WPARAM              - [in]  the messages wparam
//    LPARAM              - [in]  duh.
//    LRESULT *           - [out] the output value
//
// Output:
//    HRESULT             - S_OK
//
// Notes:
//    - people should call m_pInPlaceSiteWndless->OnDefWindowMessage [control
//      writers should just call OcxDefWindowProc(msg, wparam, lparam)];
//
STDMETHODIMP COleControl::OnWindowMessage
(
    UINT     msg,
    WPARAM   wParam,
    LPARAM   lParam,
    LRESULT *plResult
)
{
    // little bit of pre-processing -- we need to handle some cases here
    // before passing the messages on
    //
    switch (msg) {
        // make sure our UI Activation correctly matches the focus
        //
        case WM_KILLFOCUS:
        case WM_SETFOCUS:
            // give the control site focus notification
            //
            if (m_fInPlaceActive && m_pControlSite)
                m_pControlSite->OnFocus(msg == WM_SETFOCUS);
            break;
    }

    // just pass it to the control's window proc.
    //
    *plResult = WindowProc(msg, wParam, lParam);
    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::GetDropTarget    [IOleInPlaceObjectWindowless]
//=--------------------------------------------------------------------------=
// this method returns a pointer to the objects IDropTarget interface.  since
// they do not have a window, windowless objects cannot register an IDropTarget
// interface.
//
// Parameters:
//    IDropTarget **        - [out]
//
// Output:
//    HRESULT               - S_OK, E_NOTIMPL
//
// Notes:
//
STDMETHODIMP COleControl::GetDropTarget
(
    IDropTarget **ppDropTarget
)
{
    // OVERRIDE: if you want to do drag and drop and you're windowless,
    // override me.
    //
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// COleControl::TranslateAccelerator    [IOleInPlaceActiveObject]
//=--------------------------------------------------------------------------=
// Processes menu accelerator-key messages from the container's message queue.
//
// Parameters:
//    LPMSG            - [in] the message that has the special key in it.
//
// Output:
//    HRESULT          - S_OK, S_FALSE, E_UNEXPECTED
//
// Notes:
//
STDMETHODIMP COleControl::TranslateAccelerator
(
    LPMSG    pmsg
)
{
    // see if we want it or not.
    //
    if (OnSpecialKey(pmsg))
        return S_OK;

    // if not, then we want to forward it back to the site for further processing
    //
    if (m_pControlSite)
        return m_pControlSite->TranslateAccelerator(pmsg, _SpecialKeyState());

    // we didn't want it.
    //
    return S_FALSE;
}

//=--------------------------------------------------------------------------=
// COleControl::OnFrameWindowActivate    [IOleInPlaceActiveObject]
//=--------------------------------------------------------------------------=
// Notifies the control when the container's top-level frame window is
// activated or deactivated.
//
// Parameters:
//    BOOL        - [in] state of containers top level window.
//
// Output:
//    HRESULT     - S_OK
//
// Notes:
//
STDMETHODIMP COleControl::OnFrameWindowActivate
(
    BOOL fActivate
)
{
    // we're supposed to go UI active in this case
    //
    return InPlaceActivate(OLEIVERB_UIACTIVATE);
}

//=--------------------------------------------------------------------------=
// COleControl::OnDocWindowActivate    [IOleInPlaceActiveObject]
//=--------------------------------------------------------------------------=
// Notifies the active control when the container's document window is
// activated or deactivated.
//
// Parameters:
//    BOOL            - state of mdi child window.
//
// Output:
//    HRESULT         - S_OK
//
// Notes:
//
STDMETHODIMP COleControl::OnDocWindowActivate
(
    BOOL fActivate
)
{
    // we're supposed to go UI active in this case
    //
    return InPlaceActivate(OLEIVERB_UIACTIVATE);
}

//=--------------------------------------------------------------------------=
// COleControl::ResizeBorder    [IOleInPlaceActiveObject]
//=--------------------------------------------------------------------------=
// Alerts the control that it needs to resize its border space.
//
// Parameters:
//    LPCRECT               - [in] new outer rectangle for border space
//    IOleInPlaceUIWindow * - [in] the document or frame who's border has changed
//    BOOL                  - [in] true if it was the fram window taht called.
//
// Output:
//    HRESULT               - S_OK
//
// Notes:
//
STDMETHODIMP COleControl::ResizeBorder
(
    LPCRECT              prcBorder,
    IOleInPlaceUIWindow *pInPlaceUIWindow,
    BOOL                 fFrame
)
{
    // this is largely uninteresting to us, since we have no border.
    //
    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::EnableModeless    [IOleInPlaceActiveObject]
//=--------------------------------------------------------------------------=
// Enables or disables modeless dialog boxes when the container creates or
// destroys a modal dialog box.
//
// Parameters:
//    BOOL            - [in] enable or disable modeless dialogs.
//
// Output:
//    HRESULT         - S_OK
//
// Notes:
//
STDMETHODIMP COleControl::EnableModeless
(
    BOOL fEnable
)
{
    // phenomenally uninteresting
    //
    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::GetClassInfo    [IProvideClassInfo]
//=--------------------------------------------------------------------------=
// returns the TypeInfo for the control's coclass.
//
// Parameters:
//    ITypeInfo **        - [out]
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP COleControl::GetClassInfo
(
    ITypeInfo **ppTypeInfo
)
{
    ITypeLib *pTypeLib;
    HRESULT hr;

    CHECK_POINTER(ppTypeInfo);
    *ppTypeInfo = NULL;

    // go and get our type library.
    // CONSIDER: - go to the same sorta scheme that we use for TypeInfo caching.
    // CONSIDER: - consider trying to register our typelib if this fails.
    //
    hr = LoadRegTypeLib(*g_pLibid, (USHORT)VERSIONOFOBJECT(m_ObjectType), 0,
                        LANGIDFROMLCID(g_lcidLocale), &pTypeLib);
    RETURN_ON_FAILURE(hr);

    // got the typelib.  get typeinfo for our coclass.
    //
    hr = pTypeLib->GetTypeInfoOfGuid((REFIID)CLSIDOFOBJECT(m_ObjectType), ppTypeInfo);
    pTypeLib->Release();
    RETURN_ON_FAILURE(hr);

    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::ViewChange    [callable]
//=--------------------------------------------------------------------------=
// called whenever the view of the object has changed.
//
// Notes:
//
void COleControl::ViewChanged
(
    void
)
{
    // send the view change notification to anybody listening.
    //
    if (m_pViewAdviseSink) {
        m_pViewAdviseSink->OnViewChange(DVASPECT_CONTENT, -1);

        // if they only asked to be advised once, kill the connection
        //
        if (m_fViewAdviseOnlyOnce)
            SetAdvise(DVASPECT_CONTENT, 0, NULL);
    }
}

//=--------------------------------------------------------------------------=
// COleControl::SetInPlaceVisible    [helper]
//=--------------------------------------------------------------------------=
// controls the visibility of the control window.
//
// Parameters:
//    BOOL        - TRUE shows FALSE hides.
//
// Notes:
//
void COleControl::SetInPlaceVisible
(
    BOOL fShow
)
{
    BOOL fVisible;

    m_fInPlaceVisible = fShow;

    // don't do anything if we don't have a window.  otherwise, set it
    //
    if (m_hwnd) {
        fVisible = ((GetWindowLong(GetOuterWindow(), GWL_STYLE) & WS_VISIBLE) != 0);

        if (fVisible && !fShow)
            ShowWindow(GetOuterWindow(), SW_HIDE);
        else if (!fVisible && fShow)
            ShowWindow(GetOuterWindow(), SW_SHOWNA);
    }
}

//=--------------------------------------------------------------------------=
// COleControl::AmbientPropertyChanged    [overridable]
//=--------------------------------------------------------------------------=
// a method that derived controls can override to do whatever they want.
// we don't particularily care about this event.
//
// Parameters:
//    DISPID            - [in] dispid of prop that changed.
//
// Notes:
//
void COleControl::AmbientPropertyChanged
(
    DISPID dispid
)
{
    // do nothing
}

//=--------------------------------------------------------------------------=
// COleControl::DoCustomVerb    [overridable]
//=--------------------------------------------------------------------------=
// we were asked to execute a verb we don't know about right away.  see if
// it's a verb that the dervied-control defined.
//
// Parameters:
//    LONG            - [in] the verb.
//
// Output:
//    HRESULT         - S_OK, OLEOBJ_S_INVALIDVERB
//
// Notes:
//
HRESULT COleControl::DoCustomVerb
(
    LONG    lVerb
)
{
    return OLEOBJ_S_INVALIDVERB;
}

//=--------------------------------------------------------------------------=
// COleControl::OnSetExtent    [overridable]
//=--------------------------------------------------------------------------=
// let the user do something in the resize, if they care.
//
// Parameters:
//    SIZEL *        - [in] new values.
//
// Output:
//    BOOL           - FALSE means keep current size
//
// Notes:
//
BOOL COleControl::OnSetExtent
(
    const SIZEL *pSizeL
)
{
    return TRUE;
}

//=--------------------------------------------------------------------------=
// COleControl::OnSpecialKey    [overridable]
//=--------------------------------------------------------------------------=
// allows controls to handle special keys such as arrows, CTRL+, etc ...
//
// Parameters:
//    LPMSG        - [in] the special key msg.
//
// Output:
//    BOOL         - TRUE we processed it, FALSE we didn't.
//
// Notes:
//
BOOL COleControl::OnSpecialKey
(
    LPMSG pmsg
)
{
    // do nothing.
    //
    return FALSE;
}

//=--------------------------------------------------------------------------=
// COleControl::ModalDialog    [callable, utility]
//=--------------------------------------------------------------------------=
// should be called when the control is about to show and hide a modal dialog.
//
// Parameters:
//    BOOL        - [in] true means showing a modal dialog, false means done
//
// Notes:
//
void COleControl::ModalDialog
(
    BOOL fShow
)
{
    // notify the container of our intention to show a modal dialog...
    //
    if (m_pInPlaceFrame)
        m_pInPlaceFrame->EnableModeless(!fShow);
}

//=--------------------------------------------------------------------------=
// COleControl::BeforeDestroyWindow    [overridable]
//=--------------------------------------------------------------------------=
// called just before we destroy a window.  gives the user the opportunity to
// save information out, especially if they're a subclassed control, and this
// is an interesting thing to do.
//
// Notes:
//
void COleControl::BeforeDestroyWindow
(
    void
)
{
    // fweeee
}

//=--------------------------------------------------------------------------=
// COleControl::OnSetObjectRectsChangingWIndowPos    [overridable]
//=--------------------------------------------------------------------------=
// called just before we perform a SetWindowPos in the SetObjectRects
// function. gives a control the opportunity to change the flags.
//
// Notes:
//
void COleControl::OnSetObjectRectsChangingWindowPos(DWORD *dwFlag)
{
    *dwFlag = 0;
}

void COleControl::OnVerb(LONG lVerb)
{
}

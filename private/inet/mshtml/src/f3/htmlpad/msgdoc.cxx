//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       msgdoc.cxx
//
//  Contents:   CPadMessage implementation
//
//-------------------------------------------------------------------------

#include "padhead.hxx"

#ifndef X_MSG_HXX_
#define X_MSG_HXX_
#include "msg.hxx"
#endif

#ifndef X_MSGCIDX_H_
#define X_MSGCIDX_H_
#include "msgcidx.h"
#endif

#ifndef X_NTVERP_H_
#define X_NTVERP_H_
#include "ntverp.h"
#endif

__declspec(thread)  static BOOL      s_fModalUp = FALSE;
__declspec(thread)  static BOOL      s_fMBoxUp = FALSE;
__declspec(thread)  static HWND      s_hwndUp = NULL;

// BUGBUG: chrisf - should this really be thread local ?
__declspec(thread)  static HINSTANCE s_hInstRichEd32 = NULL;

// BUGBUG: the following should be thread local when we use different threads
// for each mail message
static BOOL         s_fMapiInitialized = FALSE;
CLastError *        g_pLastError = NULL;

TCHAR g_achFormName[] = SZ_APPLICATION_NAME TEXT(" Exchange Form");
TCHAR g_achWindowCaption[] = TEXT("Microsoft Trident Form");
char  g_achFormClassName[] = "IPM.Note.Trident";
char  g_achPlainTextHeader[] = "[This is an HTML message written with "
                         "Microsoft Trident " VER_PRODUCTVERSION_STR ". You are "
                         "reading a plain text version of the original HTML.]";

static HRESULT
CreateMessage(IUnknown **ppUnk)
{
    HRESULT hr = S_OK;

    // BUGBUG -
    // Currently, messages are created on the main thread only.  If messages are
    // created on multiple threads then s_fMapiInitialized must b moved to thread
    // local storage and mapi must be initialized for each thread.

    if (!s_fMapiInitialized)
    {
        hr = THR(MAPIInitialize(NULL));
        if (hr)
            RRETURN(hr);

        s_fMapiInitialized = TRUE;

        Assert(!g_pLastError);

        g_pLastError = new CLastError();
        if (!g_pLastError)
        {
            hr= E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = g_pLastError->Init(g_achFormName);
        if (hr)
            goto Cleanup;
    }

    Assert(g_pLastError);
    
    if(!s_hInstRichEd32)
    {
        s_hInstRichEd32 = LoadLibraryEx(TEXT("RICHED32.DLL"), NULL, 0);
    }

    *ppUnk = (IMAPIForm *)new CPadMessage();

    if (!*ppUnk)
        hr = E_OUTOFMEMORY;

Cleanup:
    RRETURN(hr);
}

void RevokeMsgFact()
{
    if (s_fMapiInitialized)
        MAPIUninitialize();

    if (g_pLastError)
        delete g_pLastError;
}


CPadFactory Factory(CLSID_CPadMessage, CreateMessage, RevokeMsgFact);

BOOL CALLBACK FormDlgProcSend(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK FormDlgProcRead(HWND, UINT, WPARAM, LPARAM);

SizedSPropTagArray(cPropReadMsg, tagaRead) = {cPropReadMsg, {MESSAGE_TAGS}};

CPadMessage::CPadMessage()
{
    // FormBase
    _state = stateUninit;
}

CPadMessage::~CPadMessage()
{

    //formBase Release ()

    MAPIFreeBuffer(_pval);

    FreePadrlist(_padrlist);

    MAPIFreeBuffer(_lpbConvIdx);

    if (_hChsFldDll)
        FreeLibrary(_hChsFldDll);

    MAPIFreeBuffer(_pbCFDState);
}   

HRESULT
CPadMessage::QueryInterface(REFIID iid, void **ppvObj)
{
    if (CPadDoc::QueryInterface(iid, ppvObj) == E_NOINTERFACE)
    {
        if (iid == IID_IMAPIForm)
        {
            *ppvObj = (LPVOID) (IMAPIForm *) this;
        }
        else if (iid == IID_IPersistMessage)
        {
            *ppvObj = (LPVOID) (IPersistMessage *) this;
        }
        else if (iid == IID_IMAPIFormAdviseSink)
        {
            *ppvObj = (LPVOID) (IMAPIFormAdviseSink *) this;
        }
        else
        {
            *ppvObj = NULL;
            return E_NOINTERFACE;
        }

        ((IUnknown *)*ppvObj)->AddRef();
    }

    return S_OK;
}

void
CPadMessage::Passivate()
{

    //
    //  Release the view context
    //

    if (_pviewctx != NULL)
    {
        _pviewctx->SetAdviseSink(NULL);
        _pviewctx->Release();
        _pviewctx = NULL;
    }

    //
    //  Release message objects if we have them
    //

    if (s_fModalUp)
        _pviewctxOverride = NULL;

    ReleaseInterface(_pmsg);
    _pmsg = NULL;

    ReleaseInterface(_pmsgsite);
    _pmsgsite = NULL;

    ReleaseInterface(_pab);
    _pab = NULL;

    ReleaseInterface(_pses);
    _pses = NULL;

    //
    //  Tell all objects to be closed and de-initialized, only IUnknown
    //          calls are legal after this.
    //

    Assert(_pmsg == NULL);
    Assert(_pmsgsite == NULL);
    Assert(_pviewctx == NULL);
    Assert(_pviewctxOverride == NULL);
    Assert(_pab == NULL);
    Assert(_pses == NULL);

    _state = stateDead;

    //
    //  Now deinit paddoc
    //

    CPadDoc::Passivate();

}

///////////////////////////////////////////////////////////////////////////////
//
//  IMAPIForm interface
//
///////////////////////////////////////////////////////////////////////////////


////    IMAPIForm::SetViewContext
//
//

STDMETHODIMP CPadMessage::SetViewContext(IN IMAPIViewContext * pvc)
{

    //
    //  If we currently have a view context, then release it
    //

    if (_pviewctx != NULL)
    {
        _pviewctx->SetAdviseSink(NULL);
        _pviewctx->Release();
    }

    //
    // Accept the new view context.
    //

    _pviewctx = pvc;

    //
    //  If the new view context is non-null, then save it away, setup
    //  the advise sink back to check for things and get the current set
    //  of status flags
    //

    _ulViewStatus = 0;
    if (pvc != NULL)
    {
        _pviewctx->AddRef ();
        _pviewctx->SetAdviseSink (this);
        _pviewctx->GetViewStatus(&_ulViewStatus);
    }

    ConfigWinMenu();

    return S_OK;
}


////    IMAPIForm::GetViewContext
//

STDMETHODIMP CPadMessage::GetViewContext(OUT IMAPIViewContext * FAR * ppvc)
{
    Assert(ppvc);

    *ppvc = _pviewctx;

    if (_pviewctx != NULL)
    {
        _pviewctx->AddRef();
        return S_OK;
    }
    else
        return ResultFromScode(S_FALSE);
}


////    IMAPIForm::ShutdownForm
//
//  Description:
//      This routine is called to shut down the form and if necessary
//      to cause save changes to the form.
//

STDMETHODIMP CPadMessage::ShutdownForm(DWORD dwSaveOptions)
{
    HRESULT hr;

    //
    //  Check for valid state to make the call
    //

    switch( _state )
    {
        default:
        case stateDead:
            _viewnotify.OnShutdown ();
            return g_LastError.SetLastError(ResultFromScode(E_UNEXPECTED));

        case stateUninit:
        case stateNormal:
        case stateNoScribble:
        case stateHandsOffFromSave:
        case stateHandsOffFromNormal:
            break;
    }

    hr = THR(QuerySave(dwSaveOptions));

    if (hr == S_FALSE)
        return MAPI_E_USER_CANCEL;

    // Notify viewer that we are going down
    _viewnotify.OnShutdown ();

    // Hide the document to remove the user's reference count.
    this->ShowWindow(SW_HIDE);

    RRETURN(hr);
}


////    IMAPIForm::DoVerb
//

STDMETHODIMP CPadMessage::DoVerb(LONG iVerb, LPMAPIVIEWCONTEXT pviewctx,
                               ULONG hwndParent, LPCRECT lprcPosRect)
{
    HRESULT             hr;

    //
    //  If a view context was passed in, then we need to get the
    //  status bits from this view context.  Also we are going to save
    //  the current view context and use this view context for the
    //  duration of the verb execution.
    //

    if (pviewctx != NULL)
    {
        _pviewctxOverride = pviewctx;
        pviewctx->GetViewStatus(&_ulViewStatus);
    }

    //
    //   Execute the requested verb.  If we do not understand the verb
    //  or we do not support the verb then we return NO SUPPORT and let
    //  the viewer deal with this.
    //

    switch (iVerb)
    {

    case EXCHIVERB_OPEN:
        hr = THR(OpenForm((HWND) hwndParent, lprcPosRect, _ulViewStatus));
        break;

    case EXCHIVERB_REPLYTOSENDER:
        hr = THR(Reply(eREPLY, (HWND) hwndParent, lprcPosRect));
        if (HR_SUCCEEDED(hr))
        {
            _pviewctxOverride = NULL;
            ShutdownForm(SAVEOPTS_NOSAVE);
        }
        break;

    case EXCHIVERB_REPLYTOALL:
        hr = THR(Reply(eREPLY_ALL, (HWND) hwndParent, lprcPosRect));
        if (HR_SUCCEEDED(hr))
        {
            _pviewctxOverride = NULL;
            ShutdownForm(SAVEOPTS_NOSAVE);
        }
        break;

    case EXCHIVERB_FORWARD:
        hr = THR(Reply(eFORWARD, (HWND) hwndParent, lprcPosRect));
        if (HR_SUCCEEDED(hr))
        {
            _pviewctxOverride = NULL;
            ShutdownForm(SAVEOPTS_NOSAVE);
        }
        break;

    case EXCHIVERB_PRINT:
    case EXCHIVERB_SAVEAS:
    case EXCHIVERB_REPLYTOFOLDER:
        //the viewer should not call us here
        //(see Value in extensions section of smpfrm.cfg)
        Assert(FALSE);

    default:
        hr = THR(g_LastError.SetLastError(ResultFromScode(MAPI_E_NO_SUPPORT)));
        break;
    }

    //
    //  If we moved to a different view context, then switch back to
    //  the one we started with.
    //

    _pviewctxOverride = NULL;

    if (_pviewctx != NULL)
    {
        _ulViewStatus =0;
        _pviewctx->GetViewStatus(&_ulViewStatus);
        ConfigWinMenu();
    }

    RRETURN(hr);
}


////    IMAPIForm::Advise
//

STDMETHODIMP CPadMessage::Advise (IN IMAPIViewAdviseSink * pViewAdvise,
                                OUT ULONG FAR * pulConnection)
{
    HRESULT     hr;

    hr = THR(_viewnotify.Advise (pViewAdvise, pulConnection));
    if (FAILED(hr))
    {
        hr = THR(g_LastError.SetLastError(hr));
    }
    RRETURN(hr);
}


////    IMAPIForm::Unadvise
//

STDMETHODIMP CPadMessage::Unadvise(ULONG ulConnection)
{
    HRESULT     hr;

    hr = THR(_viewnotify.Unadvise(ulConnection));
    if (FAILED(hr))
    {
        hr = THR(g_LastError.SetLastError(hr));
    }

    RRETURN(hr);
}


///////////////////////////////////////////////////////////////////////////////
//
//  IPersistMessage interface
//
///////////////////////////////////////////////////////////////////////////////

////    IPersistMessage::GetClassID

STDMETHODIMP CPadMessage::GetClassID(LPCLSID lpClassID)
{
    *lpClassID = CLSID_CPadMessage;
    return S_OK;
}


////  IPersistMessage::GetLastError
//
//  Description:  This routine is used to get back a string giving more
//              information about the last error in the form.
//

STDMETHODIMP CPadMessage::GetLastError(HRESULT hr, ULONG ulFlags,
                                     LPMAPIERROR FAR * lppMAPIError)
{
    return g_LastError.GetLastError(hr, ulFlags, lppMAPIError);
}


////    IPersistMessage::IsDirty
//

STDMETHODIMP CPadMessage::IsDirty ()
{
    IPersistFile *pPF = NULL;

    if (_fDirty)
        return ResultFromScode(S_OK);

    _fDirty = GetDirtyState();

    return ResultFromScode ((_fDirty ? S_OK : S_FALSE));
}

BOOL
CPadMessage::GetDirtyState()
{
    BOOL fDirty = FALSE;

    fDirty = CPadDoc::GetDirtyState();

    if (_hwndDialog && _eFormType == eformSend)
    {
        fDirty = fDirty ||
                 _fRecipientsDirty ||
                 Edit_GetModify(GetDlgItem(_hwndDialog, ID_SUBJECT)) ||
                 AreWellsDirty();
    }

    return fDirty;
}

BOOL 
CPadMessage::AreWellsDirty()
{
    BOOL fDirty = FALSE;

    if (_hwndDialog && _eFormType == eformSend)
    {
        fDirty = Edit_GetModify(GetDlgItem(_hwndDialog, ID_TO)) ||
                 Edit_GetModify(GetDlgItem(_hwndDialog, ID_CC));
    }

    return fDirty;
}


////  IPersistMessage::InitNew
//
//  Description: This function is called in the case of composing a new
//      message.  There is a small set of properties which are set by
//      the constructor of the message, however in general it can be
//      assumed the message is clean.
//

STDMETHODIMP CPadMessage::InitNew(LPMAPIMESSAGESITE pmsgsite, LPMESSAGE pmsg)
{
    HRESULT hr = S_OK;

    //
    //  Ensure we are in a state where we can accept this call
    //

    switch(_state)
    {
    case stateUninit:
    case stateHandsOffFromSave:
    case stateHandsOffFromNormal:
        break;

    default:
        return g_LastError.SetLastError(ResultFromScode(E_UNEXPECTED));
    }

    //
    //  If we currently have a message site, then release it as we
    //  will no longer be using it.
    //

    ReleaseInterface(_pmsgsite);
    _pmsgsite = NULL;

    //
    //  Save away the pointers to the message and message site
    //

    _pmsgsite = pmsgsite;
    pmsgsite->AddRef();

    _ulSiteStatus = 0;
    _pmsgsite->GetSiteStatus(&_ulSiteStatus);

    _pmsg = pmsg;
    pmsg->AddRef();

    //
    //  Make an assumption on the message flags and status
    //

    _ulMsgStatus = 0;
    _ulMsgFlags = MSGFLAG_UNSENT;

    if (_hwnd)
    {
        hr = THR(DisplayMessage());
        if (hr)
            goto Cleanup;
    }

    //
    //  We succeeded in doing the InitNew so move to the normal state
    //

    _state = stateNormal;

    //
    //  Tell everybody who cares that we just loaded a new message
    //

    _viewnotify.OnNewMessage();

    _fNewMessage = TRUE;

Cleanup:
    RRETURN(hr);
}

//// IPersistMessage::Load
//
//  Description:  This routine is called as part of loading an existing
//      message into the form.
//

STDMETHODIMP CPadMessage::Load(LPMAPIMESSAGESITE pmsgsite, LPMESSAGE pmsg,
                             ULONG ulMsgStatus, ULONG ulMsgFlags)
{
    HRESULT hr = S_OK;

    //
    //  Ensure we are in a state where we can accept this call
    //

    switch(_state)
    {
    case stateUninit:
    case stateHandsOffFromSave:
    case stateHandsOffFromNormal:
        break;

    default:
        return g_LastError.SetLastError(ResultFromScode(E_UNEXPECTED));
    }

    //
    //  If we currently have a message site, then release it as we
    //  will no longer be using it.
    //

    ReleaseInterface(_pmsgsite);
    _pmsgsite = NULL;

    ReleaseInterface(_pmsg);
    _pmsg = NULL;


    hr = THR(GetMsgDataFromMsg(pmsg, ulMsgFlags));
    if (FAILED(hr))
        goto err;

    //
    //  Save away the message and message site which are passed in.
    //

    _pmsg = pmsg;
    pmsg->AddRef();

    _pmsgsite = pmsgsite;
    pmsgsite->AddRef();

    //
    //  Get the site status flags for disabling buttons & menus
    //
    _ulSiteStatus = 0;
    _pmsgsite->GetSiteStatus(&_ulSiteStatus);

    //
    //  Save away these properties
    //

    _ulMsgStatus = ulMsgStatus;
    _ulMsgFlags = ulMsgFlags;

    //
    //  Put us into the normal state
    //

    _state = stateNormal;


    //
    //  if our form is up, display the message
    //
    if (_hwnd)
    {
        hr = THR(DisplayMessage());
        if (hr)
            goto err;
    }

    //
    //  Tell everybody who cares that we just loaded a new message
    //

    _viewnotify.OnNewMessage();

    _fNewMessage = FALSE;

err:
    RRETURN(hr);
}

////    IPersistMessage::Save
//
//  Description:
//      This function will be called whenever a save operation of the
//      information into the form should be done.  We should only make
//      modifications to the message in this function.
//

STDMETHODIMP CPadMessage::Save(IN LPMESSAGE pmsg, IN ULONG fSameAsLoad)
{
    HRESULT             hr;

    //
    //  Check that we are in a state where we are willing to accept
    //  this call.  Must have a message.
    //

    switch( _state )
    {
    default:
        Assert(FALSE);
    case stateDead:
    case stateUninit:
    case stateNoScribble:
    case stateHandsOffFromSave:
    case stateHandsOffFromNormal:
        return g_LastError.SetLastError(ResultFromScode(E_UNEXPECTED));

    case stateNormal:
        break;
    }

    if (fSameAsLoad)
    {
        //
        //  Its the same message interface as was loaded into us.  We can
        //      assume that the pmsg passed in is either NULL or an interface
        //      on the same object as the message we already have loaded
        //

        hr = THR(SaveInto(_pmsg));
    }
    else
    {
        //
        //  We need to copy everything into the new message as we are going
        //      to clone ourselves into it.
        //

        hr = THR(_pmsg->CopyTo(0, NULL, NULL, 0, NULL, &IID_IMessage, pmsg, 0, NULL));
        if (FAILED(hr))
        {
            g_LastError.SetLastError(hr, _pmsg);
            RRETURN(hr);
        }

        //
        //  Now make all of the incremental changes
        //

        hr = THR(SaveInto(pmsg));
    }

    if (hr)
        goto Cleanup;

    _state = stateNoScribble;
    _fSameAsLoaded = fSameAsLoad;
    _fNewMessage = FALSE;

Cleanup:
    RRETURN(hr);
}


////    IPersistMessage::SaveCompleted
//
//


STDMETHODIMP CPadMessage::SaveCompleted(IN LPMESSAGE pmsg)
{

    switch( _state )
    {
    case stateHandsOffFromNormal:
    case stateHandsOffFromSave:
    case stateNoScribble:
        break;

    default:
        return g_LastError.SetLastError(ResultFromScode(E_UNEXPECTED));
    }

    if ((stateHandsOffFromNormal == _state ||
        stateHandsOffFromSave == _state)  && NULL == pmsg)
    {
        //DebugTrace("smpfrm: SaveCompleted called in handsOff state with pmsg==NULL\r\n");
        return  g_LastError.SetLastError(ResultFromScode(E_INVALIDARG));
    }

    ULONG ulOldState = _state;
    _state = stateNormal;

    //state == NoScribble , pmsg == NULL
    if (NULL == pmsg)
    {
        if (_fSameAsLoaded)
        {
            ClearDirty();
            _viewnotify.OnSaved();
        }

        return S_OK;
    }


    //state == handsOffFromNormal, pmsg != NULL
    if (stateHandsOffFromNormal == ulOldState)
    {
        ReleaseInterface(_pmsg);
        _pmsg = pmsg;
        pmsg->AddRef();

        return S_OK;
    }

    //state == handsOffFromSave || NoScribble, pmsg != NULL
    if (stateNoScribble == ulOldState ||
        stateHandsOffFromSave == ulOldState)
    {
        ReleaseInterface(_pmsg);
        _pmsg = pmsg;
        pmsg->AddRef();
    }

    _viewnotify.OnSaved();
    ClearDirty();

   return S_OK;
}


////  IPersistMessage::HandsOffMessage
//
//  Description: store, folder and message objects has to be released
//              in this method.
//
//

STDMETHODIMP CPadMessage::HandsOffMessage ()
{

    switch( _state )
    {
    case stateNormal:
    case stateNoScribble:
        break;

    default:
        return g_LastError.SetLastError(ResultFromScode(E_UNEXPECTED));
    }

    if (stateNormal == _state)
        _state = stateHandsOffFromNormal;
    else
        _state = stateHandsOffFromSave;

    //
    //  We must have a message
    //

    Assert(_pmsg != NULL);
    _pmsg->Release();
    _pmsg = NULL;


    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//
//  IMAPIFormAdviseSink interfaces
//
///////////////////////////////////////////////////////////////////////////////



////    IMAPIFormAdviseSink::OnChange
//
//  Description: called to notify about changes in viewctx status

STDMETHODIMP CPadMessage::OnChange(ULONG ulflag)
{
    if (_pviewctxOverride == NULL)
    {
        _ulViewStatus = ulflag;
        ConfigWinMenu();
    }

    return S_OK;
}

////    CPadMessage::OnActivateNext
//
//  Description:  We only say that we will handle the next message if
//              it is the exact same message class as the current message.
//              If the next message has the same "unsentness" will reuse the
//              current object, otherwise ask our ClassFactory for a new one.
//

STDMETHODIMP CPadMessage::OnActivateNext(LPCSTR lpszMessageClass, ULONG ulMessageStatus,
                                        ULONG ulMessageFlags,
                                       LPPERSISTMESSAGE FAR * ppPersistMessage)
{
    HRESULT hr;
    //TCHAR pwchMessageClass[128];

    *ppPersistMessage = NULL;

    Assert(_pval);

    //MultiByteToWideChar(CP_ACP, 0, lpszMessageClass, -1, pwchMessageClass, sizeof(pwchMessageClass));

    if (PR_MESSAGE_CLASS_A == _pval[irtClass].ulPropTag)
    {
        //the message class comparison has to be case insensitive
        if ((lstrcmpiA(_pval[irtClass].Value.lpszA, lpszMessageClass) != 0) &&
            lstrcmpiA(g_achFormClassName, lpszMessageClass) != 0)
        {
            return ResultFromScode(S_FALSE);
        }
    }
    else
    {
        if (lstrcmpiA(g_achFormClassName, lpszMessageClass) != 0)
        {
            return ResultFromScode(S_FALSE);
        }
    }


    if ((_ulMsgFlags & MSGFLAG_UNSENT) == (ulMessageFlags & MSGFLAG_UNSENT))
        //tell the viewer to reuse our object
        return ResultFromScode(S_OK);


    //Get a new object from our class factory
    hr = THR(Factory.CreateInstance(NULL, IID_IPersistMessage, (LPVOID FAR *)ppPersistMessage));
    if (hr)
        return ResultFromScode (S_FALSE);
    else
        return ResultFromScode(S_OK);
}


///////////////////////////////////////////////////////////////////////////////
//
//  Non-IMAPIinterface functions
//
///////////////////////////////////////////////////////////////////////////////


///     CPadMessage::GetMsgDataFromMsg
//
//      fills in _pval (for unsent msgs only)
//      with the info from pmsg
HRESULT CPadMessage::GetMsgDataFromMsg(LPMESSAGE pmsg, ULONG ulMsgFlags)
{
    Assert(pmsg);

    ULONG   cValues = 0;
    MAPIFreeBuffer(_pval);
    _pval = NULL;

    MAPIFreeBuffer(_lpbConvIdx);
    _lpbConvIdx = NULL;

    // Get properties from message

    HRESULT hr = THR(pmsg->GetProps((LPSPropTagArray) &tagaRead, 0,
                                    &cValues, &_pval));
    if (FAILED(hr))
    {
        g_LastError.SetLastError(hr, pmsg);
        goto err;
    }

    Assert(cValues ==  cPropReadMsg);

    // Ignore errors on individual properties

    if (hr == MAPI_W_ERRORS_RETURNED)
    {
        hr = S_OK;
    }

    // Cache conversation index

    if (PR_CONVERSATION_INDEX == _pval[irtConvIdx].ulPropTag)
    {
        LPSPropValue pval = &_pval[irtConvIdx];

        _cbConvIdx = pval->Value.bin.cb;
        if (MAPIAllocateBuffer(_cbConvIdx, (LPVOID *)&_lpbConvIdx))
        {
            _lpbConvIdx = NULL;
            _cbConvIdx = 0;
        }
        else
        {
            CopyMemory(_lpbConvIdx, pval->Value.bin.lpb, _cbConvIdx);
        }
    }
    else
    {
        _lpbConvIdx = NULL;
        _cbConvIdx = 0;
    }

    _fConvTopicSet = (PR_CONVERSATION_TOPIC_A == _pval[irtConvTopic].ulPropTag);

    // If message yet unsent, cache recipient list so that user
    // can add or remove recipients

    if (ulMsgFlags & MSGFLAG_UNSENT)
    {
        hr = THR(GetMsgAdrlist(pmsg, (LPSRowSet *)&_padrlist, &g_LastError));
        if (FAILED(hr))
        {
            goto err;
        }
    }

    RRETURN(hr);

err:
    MAPIFreeBuffer(_pval);
    _pval = NULL;

    FreePadrlist(_padrlist);
    _padrlist = NULL;

    RRETURN(hr);
}

///         CPadMessage::ClearDirty
//
//      Clears dirty state
void CPadMessage::ClearDirty(void)
{
    _fDirty = FALSE;
    _fRecipientsDirty = FALSE;

    if (_eFormType == eformSend)
    {
        Edit_SetModify(GetDlgItem(_hwndDialog, ID_SUBJECT), FALSE);
        Edit_SetModify(GetDlgItem(_hwndDialog, ID_TO), FALSE);
        Edit_SetModify(GetDlgItem(_hwndDialog, ID_CC), FALSE);
    }
}


HRESULT CPadMessage::OpenAddrBook()
{
    HRESULT hr = S_OK;

    if (_pses == NULL)
    {
        hr = THR(_pmsgsite->GetSession(&_pses));
        if (hr)
        {
            g_LastError.SetLastError(hr, _pmsgsite);
            goto Cleanup;
        }
    }
    
    Assert(_pses != NULL);

    if (_pab == NULL)
    {
        hr = THR(_pses->OpenAddressBook((ULONG) _hwnd, NULL, 0, &_pab));
        if (hr)
        {
            g_LastError.SetLastError(hr, _pses);
            if (FAILED(hr)) //if it's a real error (not a warning)
                goto Cleanup; 
        }
    }
    
    Assert(_pab != NULL);

Cleanup:
    RRETURN1(hr, MAPI_W_ERRORS_RETURNED);
}

////    CPadMessage::Address
//
//  Description:
//      This function is used to address the form.
//      The parameter determines which button in the address
//      dialog has the focus.
//

void CPadMessage::Address(int id)
{
    Assert( ID_TO_BUTTON == id || ID_CC_BUTTON == id);

    HRESULT hr;

    hr = THR(OpenAddrBook());
    if (hr)
    {
        ShowError();
        if (FAILED(hr))
            return;
    }

    // BUGBUG: chrisf - total hack below casting char[] to LPSTR
    // Mapi seems to work only with Ansi though the header files
    // contain UNICODE declarations on UNICODE platform !!!
    ADRPARM adrparm = { 0, NULL, AB_RESOLVE | DIALOG_MODAL, NULL, 0L,
                        NULL, NULL, NULL, NULL, (LPTSTR)"Address Book", NULL,
                        (LPTSTR)"Send Note To", 2, (id == ID_TO_BUTTON ? 0:1),
                        NULL, NULL, NULL, NULL };

    ULONG   ulHwndAddr = (ULONG) _hwnd;

    hr = THR(_pab->Address(&ulHwndAddr, &adrparm, &_padrlist));
    if (!hr)
    {
        DisplayRecipients(TRUE);
    }
    else if (hr != MAPI_E_USER_CANCEL)
    {
        g_LastError.SetLastError(hr, _pab);
        ShowError();
    }

}

////  CPadMessage::OpenForm
//
//  Description:  This is the internal routine which is called from the
//      open/display verb.  It will cause UI to appear if there is none
//      and force the window to the foreground if there is already UI.
//

HRESULT CPadMessage::OpenForm(HWND hwndParent, LPCRECT lprcPosRect,
                              ULONG ulViewFlags)
{
    HRESULT hr = S_OK;

    if (lprcPosRect == NULL)
        return g_LastError.SetLastError(ResultFromScode(E_INVALIDARG));

    //
    //  If any modal forms are visible then do not do anything
    //

    Assert( s_fModalUp && s_hwndUp || !s_fModalUp && !s_hwndUp);
    if (s_fMBoxUp || (s_fModalUp && hwndParent != s_hwndUp))
        return g_LastError.SetLastError(
                               ResultFromScode(OLEOBJ_S_CANNOT_DOVERB_NOW));

   if (!(ulViewFlags & VCSTATUS_MODAL))
    {
        //  If we are not modal then don't do anything relative to the parent
        hwndParent = NULL;
    }

    //
    //  Check to see if we have a window up
    //

    if (_hwnd != 0)
    {
        ::MoveWindow(_hwnd, lprcPosRect->left, lprcPosRect->top,
                   lprcPosRect->right - lprcPosRect->left,
                   lprcPosRect->bottom - lprcPosRect->top,
                   TRUE);
    }
    else
    {

        hr = THR(RegisterPadWndClass());
        if (hr)
            return g_LastError.SetLastError(ResultFromScode(hr));

        _hwnd = CreateWindowEx(0,
                              SZ_PAD_WNDCLASS,
                              g_achWindowCaption,
                              WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                              lprcPosRect->left,
                              lprcPosRect->top,
                              lprcPosRect->right - lprcPosRect->left,
                              lprcPosRect->bottom - lprcPosRect->top,
                              hwndParent,
                              NULL,
                              g_hInstCore,
                              this);

        if (_hwnd == NULL)
        {
            return g_LastError.SetLastError(ResultFromScode(E_OUTOFMEMORY));
        }

        //
        //  Create the dialog as a child of this window
        //

        if (_ulMsgFlags & MSGFLAG_UNSENT)
        {
            _eFormType = eformSend;
            _hwndDialog = CreateDialog(g_hInstCore, MAKEINTRESOURCE(IDR_SEND_FORM),
                                        _hwnd, &FormDlgProcSend);
            _HAccelTable = LoadAccelerators(g_hInstCore, MAKEINTRESOURCE(IDR_SEND_FORM));

            _hmenuMain = LoadMenu(g_hInstResource, MAKEINTRESOURCE(IDR_SEND_FORM));

            _fUserMode = FALSE;
        }
        else
        {
            _eFormType = eformRead;

            InitReadToolbar();

            _hwndDialog = CreateDialog(g_hInstCore, MAKEINTRESOURCE(IDR_READ_FORM),
                                        _hwnd, &FormDlgProcRead);
            _HAccelTable = LoadAccelerators(g_hInstCore, MAKEINTRESOURCE(IDR_READ_FORM));

            _hmenuMain = LoadMenu(g_hInstResource, MAKEINTRESOURCE(IDR_READ_FORM));

            _fUserMode = TRUE;
        }

        SetMenu(_hwnd, _hmenuMain);
    }

    //
    //  Stuff message into form
    //

    hr = THR(DisplayMessage());
    if (hr)
        goto Cleanup;

    //
    //  Position the window where it is supposed to be
    //

    this->ShowWindow(SW_SHOW);

    SetForegroundWindow(_hwnd);

    //
    //  If we are modal, then we loop until the form is closed
    //

    if (ulViewFlags & VCSTATUS_MODAL)
    {
        MSG         msg;

        BOOL fOldModalUp = s_fModalUp;
        HWND hwndOldUp = s_hwndUp;

        s_fModalUp = TRUE;
        s_hwndUp = _hwnd;

        while ((_hwnd != NULL) && (GetMessage(&msg, _hwnd, 0, 0)))
        {
            //first call our method and see if this message makes sense to us.
            //if not, let WIN API care about it.
            if (!_DocHost.TranslateAccelerator(&msg, &CGID_MSHTML, IDM_UNKNOWN))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
            }
        }

        s_fModalUp = fOldModalUp;
        s_hwndUp = hwndOldUp;
    }

Cleanup:
    RRETURN(hr);
}

////    CPadMessage::SaveInto
//
//  Description:
//    This routine gives one central location which save all modified
//      properties into a message.
//

HRESULT CPadMessage::SaveInto(LPMESSAGE pmsg)
{
    HRESULT             hr;
    ULONG               cval = 0;
    LONG                cb;
    LPSPropProblemArray pProblems = NULL;

    Assert(_eFormType == eformSend);

    if (_eFormType == eformRead)
        return S_OK;
    
    // Call IsDirty() to make sure  _fDirty is current

    if (!_fDirty)
        IsDirty();
    
    // If not dirty and we already have a cache, return

    if (!_fDirty && _pval != NULL) 
        return S_OK;

    // If wells have been touched, regenerate recipient list
    // but do not resolve names (user may save a message without 
    // resolving the names)

    if (AreWellsDirty())
    {
        hr = THR(ParseRecipients(TRUE));
        if (hr)
            goto Cleanup;                
    }

    //  Write out the recipient table to the message

    if (_padrlist && _fRecipientsDirty)
    {
        hr = THR(pmsg->ModifyRecipients(0, _padrlist));
        if (hr)
        {
            g_LastError.SetLastError(hr, pmsg);
            goto Cleanup;
        }
    }

    // Create new cache

    if (NULL != _pval)
    {
        MAPIFreeBuffer(_pval);
        _pval = NULL;
    }
    
    if (MAPIAllocateBuffer(sizeof(SPropValue) * cPropSendMsg, (LPVOID FAR *) &_pval))
    {
        hr = g_LastError.SetLastError(ResultFromScode(E_OUTOFMEMORY));
        goto Cleanup;
    }
        
    ZeroMemory(_pval, sizeof(SPropValue) * cPropSendMsg);
        
    _pval[irtTime].ulPropTag = PR_NULL;
    _pval[irtSenderName].ulPropTag = PR_NULL;                     
    _pval[irtNormSubject].ulPropTag = PR_NULL;
    _pval[irtTo].ulPropTag = PR_NULL;
    _pval[irtCc].ulPropTag = PR_NULL;

    // Get subject into the cache

    cb = GetWindowTextLength(GetDlgItem(_hwndDialog, ID_SUBJECT));
    if (cb > 0)
    {
        if (MAPIAllocateBuffer(cb+1, (LPVOID FAR *)&_pval[irtSubject].Value.lpszA))
        {
            hr = g_LastError.SetLastError(ResultFromScode(E_OUTOFMEMORY));
            goto Cleanup;
        }

        GetWindowTextA(GetDlgItem(_hwndDialog, ID_SUBJECT), _pval[irtSubject].Value.lpszA, cb+1);
        _pval[irtSubject].ulPropTag = PR_SUBJECT_A;
    }
    else
    { //no subject

        _pval[irtSubject].ulPropTag = PR_NULL;
    }

    // Set form class

    _pval[irtClass].ulPropTag = PR_MESSAGE_CLASS_A;
    _pval[irtClass].Value.lpszA = g_achFormClassName;

    // If the message didn't have PR_CONVERSATION_TOPIC when we loaded it, we'll
    // set it every time we save the message. Otherwise we don't touch it

    if (!_fConvTopicSet)
    {
        _pval[irtConvTopic].ulPropTag = PR_CONVERSATION_TOPIC_A;
        if (PR_SUBJECT_A == _pval[irtSubject].ulPropTag)
        {
            _pval[irtConvTopic].Value.lpszA = _pval[irtSubject].Value.lpszA;
        }
        else
        {
            _pval[irtConvTopic].Value.lpszA = "";
        }
    }
    else
    {
        _pval[irtConvTopic].ulPropTag = PR_NULL;
    }

    // If the message doesn't have a PR_CONVERSATION_INDEX, create and set it

    if (_cbConvIdx == 0)
    {
        if (!ScAddConversationIndex(0, NULL, &_cbConvIdx,   &_lpbConvIdx))
        {
            _pval[irtConvIdx].ulPropTag = PR_CONVERSATION_INDEX;
            _pval[irtConvIdx].Value.bin.lpb = _lpbConvIdx;
            _pval[irtConvIdx].Value.bin.cb = _cbConvIdx;
        }
        else
        {
            _pval[irtConvIdx].ulPropTag = PR_NULL;
        }
    }
    else
    {
        _pval[irtConvIdx].ulPropTag = PR_NULL;
    }

    // For an unsent form, set the delivery time so that it shows up
    // at the right place in the folder

        if (_ulMsgFlags & MSGFLAG_UNSENT)
        {
                SYSTEMTIME                      st;

                GetSystemTime(&st);
                SystemTimeToFileTime(&st, &_pval[irtTime].Value.ft);
                _pval[irtTime].ulPropTag = PR_MESSAGE_DELIVERY_TIME;
    }

    // Now set the message properties

    hr = THR(pmsg->SetProps(cPropSendMsg, _pval, &pProblems));
    if (hr)
    {
        g_LastError.SetLastError(hr, pmsg);
        goto Cleanup;
    }

    Assert(!pProblems);

    // Stream the down level body (plain text or RTF) 
    // out of trident into the message

    hr = THR(StreamOutTextBody(pmsg));
    if (hr)
    {
        g_LastError.SetLastError(hr, pmsg);
        goto Cleanup;
    }

    // Stream the HTML body out of Trident into the message

    hr = THR(StreamOutHtmlBody(pmsg));
    if (hr)
    {
        g_LastError.SetLastError(hr, pmsg);
        goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

BOOL CPadMessage::OnTranslateAccelerator(MSG * pMsg)
{
    //
    // Check for TAB between subject and body
    //

    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_TAB)
    {
        if (GetKeyState(VK_SHIFT) & 0x80000000)
        {
            if (_pInPlaceActiveObject)
            {
               UIDeactivateDoc();
               SetFocus(GetDlgItem(_hwndDialog, ID_SUBJECT));
               return TRUE;
            }
        }
        else
        {
            if (GetFocus() == GetDlgItem(_hwndDialog, ID_SUBJECT))
            {
                UIActivateDoc(pMsg);
                return TRUE;
            }
        }
    }

    //
    //  We translate accelerators before the dialog message so that we
    //  can get our accelerators to override the dialog's.
    //

    if (::TranslateAccelerator(_hwnd, _HAccelTable, pMsg))
        return TRUE;

    //
    // Now let Trident process accelerators
    //

    if(CPadDoc::OnTranslateAccelerator(pMsg))
        return TRUE;

    //
    // Finally process dialog accelerators
    //

    if ((_hwndDialog != NULL) && ::IsDialogMessage(_hwndDialog, pMsg))
        return TRUE;

    return FALSE;
}


///  CPadMessage::DisplayMessage
//
//  display the info from _pval in the dialog
HRESULT CPadMessage::DisplayMessage(void)
{
    HRESULT  hr = S_OK;
    TCHAR    awch[256];
    char     ach[256];

    Assert(_hwnd);
    Assert(_hwndDialog);

    if (NULL != _pval)
    {
        if (_pval[irtSubject].ulPropTag == PR_SUBJECT_A)
        {
            SetDlgItemTextA(_hwndDialog, ID_SUBJECT, _pval[irtSubject].Value.lpszA);
            MultiByteToWideChar(CP_ACP, 0, _pval[irtSubject].Value.lpszA, -1,
                                awch, sizeof(awch));
            lstrcat(awch, TEXT(" - "));
            lstrcat(awch, g_achWindowCaption);
            SetWindowText(_hwnd, awch);
        }
        else
        {
            SetWindowText(_hwnd, g_achWindowCaption);
        }

        if (_eFormType == eformRead)
        {
            if (_pval[irtSenderName].ulPropTag == PR_SENDER_NAME_A)
            SetDlgItemTextA(_hwndDialog, ID_FROM, _pval[irtSenderName].Value.lpszA);

            if (_pval[irtTime].ulPropTag == PR_CLIENT_SUBMIT_TIME) {
                FormatTime(&_pval[irtTime].Value.ft, ach, sizeof(ach));
                SetDlgItemTextA(_hwndDialog, ID_SENT, ach);
            }

            if (_pval[irtTo].ulPropTag == PR_DISPLAY_TO_A)
                SetDlgItemTextA(_hwndDialog, ID_TO, _pval[irtTo].Value.lpszA);

            if (_pval[irtCc].ulPropTag == PR_DISPLAY_CC_A)
                SetDlgItemTextA(_hwndDialog, ID_CC, _pval[irtCc].Value.lpszA);
        }
        else if (_eFormType == eformSend)
        {
            DisplayRecipients(TRUE);
        }
        else
        {
            Assert(FALSE);
        }

        ClearDirty();
    }

    if (_pval && _pval[irtHtmlBody].ulPropTag == PR_HTML_BODY)
    {
        hr = THR(StreamInHtmlBody(_pval[irtHtmlBody].Value.lpszA));
        if (hr)
            goto err;   
    }    
    else
    {
        hr = THR(StreamInHtmlBody(_pmsg));
        if (hr)
            goto err;
    }

err:
    RRETURN(hr);
}


/// CPadMessage::IsAddressed
//
// Does _padrlist contain a recipient?
BOOL CPadMessage::IsAddressed(void)
{
    Assert(_eFormType == eformSend) ;

    if (NULL == _padrlist || _padrlist->cEntries == 0)
        return FALSE;

    for(LPADRENTRY pae = _padrlist->aEntries;
        pae < _padrlist->aEntries + _padrlist->cEntries; ++pae)
    {
        if (pae->rgPropVals)
            return TRUE;
    }

    return FALSE;
}

///     CPadMessage::ConfigMenu
//Enable/disable menu commands based on the values of _ulSiteStatus
// and _ulViewStatus
void CPadMessage::ConfigMenu(HMENU hMenu)
{
    if (_eFormType == eformRead)
    {
        EnableMenuItem(hMenu, IDM_MESSAGE_SAVE,
            MF_BYCOMMAND|((_ulSiteStatus & VCSTATUS_SAVE)? MF_ENABLED:MF_GRAYED));
        EnableMenuItem(hMenu, IDM_MESSAGE_DELETE,
            MF_BYCOMMAND| (!(_ulViewStatus & VCSTATUS_READONLY) &&
                            (_ulSiteStatus & VCSTATUS_DELETE) ? MF_ENABLED:MF_GRAYED));
        EnableMenuItem(hMenu, IDM_MESSAGE_COPY,
            MF_BYCOMMAND| (!(_ulViewStatus & VCSTATUS_READONLY) &&
                            (_ulSiteStatus & VCSTATUS_COPY) ? MF_ENABLED:MF_GRAYED));
        EnableMenuItem(hMenu, IDM_MESSAGE_MOVE,
            MF_BYCOMMAND| (!(_ulViewStatus & VCSTATUS_READONLY) &&
                            (_ulSiteStatus & VCSTATUS_MOVE) ? MF_ENABLED:MF_GRAYED));
        EnableMenuItem(hMenu, IDM_VIEW_ITEMABOVE,
            MF_BYCOMMAND|(_ulViewStatus & VCSTATUS_PREV ? MF_ENABLED:MF_GRAYED));
        EnableMenuItem(hMenu, IDM_VIEW_ITEMBELOW,
            MF_BYCOMMAND|(_ulViewStatus & VCSTATUS_NEXT ? MF_ENABLED:MF_GRAYED));
    }

    else
    {
        EnableMenuItem(hMenu, IDM_MESSAGE_SUBMIT,
            MF_BYCOMMAND|((_ulSiteStatus &VCSTATUS_SUBMIT) ? MF_ENABLED:MF_GRAYED));
        EnableMenuItem(hMenu, IDM_MESSAGE_SAVE,
            MF_BYCOMMAND|((_ulSiteStatus & VCSTATUS_SAVE)? MF_ENABLED:MF_GRAYED));
        EnableMenuItem(hMenu, IDM_MESSAGE_DELETE,
            MF_BYCOMMAND| (!(_ulViewStatus & VCSTATUS_READONLY) &&
                            (_ulSiteStatus & VCSTATUS_DELETE) ? MF_ENABLED:MF_GRAYED));
        EnableMenuItem(hMenu, IDM_MESSAGE_COPY,
            MF_BYCOMMAND| (!(_ulViewStatus & VCSTATUS_READONLY) &&
                            (_ulSiteStatus & VCSTATUS_COPY) ? MF_ENABLED:MF_GRAYED));
        EnableMenuItem(hMenu, IDM_MESSAGE_MOVE,
            MF_BYCOMMAND| (!(_ulViewStatus & VCSTATUS_READONLY) &&
                            (_ulSiteStatus & VCSTATUS_MOVE) ? MF_ENABLED:MF_GRAYED));

    }
}

HRESULT
CPadMessage::DoSave(BOOL fPrompt)
{
    HRESULT hr;

    if (fPrompt)
        return CPadDoc::DoSave(TRUE);

    hr = THR(_pmsgsite->SaveMessage());
    if (hr)
    {
        g_LastError.SetLastError(hr, _pmsgsite);
        ShowError();
    }

    RRETURN(hr);
}

///  CPadMessage::DoDelete
//
//  called only from our UI
void CPadMessage::DoDelete(void)
{
    HRESULT hr;
    RECT rect;

    GetWindowRect(_hwnd, &rect);

    hr = THR(_pmsgsite->DeleteMessage(_pviewctx, &rect));
    if (FAILED(hr))
    {
        g_LastError.SetLastError(hr, _pmsgsite);
        ShowError();
    }
    if (NULL == _pmsg)
    {
        ShutdownForm(SAVEOPTS_NOSAVE);
    }
}

///  CPadMessage::DoSubmit
//
//  called only from our UI
void CPadMessage::DoSubmit(void)
{
    HRESULT hr;

    // Get and check recipients from the UI

    hr = THR_NOTRACE(GetAndCheckRecipients(FALSE));
    
    if (hr == MAPI_E_USER_CANCEL)
        return;
    
    if (hr)
        goto Cleanup;

    // GeAndCheckRecipients is supposed to ensure that the addrlist
    // is in sync with the wells
    Assert (!AreWellsDirty());

    // Verify we have at least a recipient

    if (!IsAddressed())
    {
        ShowMessageBox(_hwndDialog, TEXT("No recipients"), g_achFormName, MB_OK);
        return;
    }

    hr = THR(_pmsgsite->SubmitMessage(0));

Cleanup:
    if (FAILED(hr))
    {
        Assert(hr != MAPI_E_USER_CANCEL);
        g_LastError.SetLastError(hr, _pmsgsite);
        ShowError();
    }
    else
    {
        _viewnotify.OnSubmitted();
    }
    if (_pmsg == NULL)
        ShutdownForm(SAVEOPTS_NOSAVE);
}

///  CPadMessage::DoNext
//
//  called only from our UI
void CPadMessage::DoNext(ULONG ulDir)
{
    Assert(VCDIR_NEXT == ulDir || VCDIR_PREV == ulDir);

    HRESULT hr;

    hr = THR(QuerySave(SAVEOPTS_PROMPTSAVE));
    if (hr)
    {
        if (hr != S_FALSE)
            ShowError();
        return;
    }

    RECT rect;
    GetWindowRect(_hwnd, &rect);

    hr = THR(ViewCtx()->ActivateNext(ulDir, &rect));
    if (NULL == _pmsg)
    {
        ShutdownForm(SAVEOPTS_NOSAVE);
    }
}

///  CPadMessage::DoReply
//
//  called only from our UI
void CPadMessage::DoReply(eREPLYTYPE eType)
{
    HRESULT hr;

    hr = THR(QuerySave(SAVEOPTS_PROMPTSAVE));
    if (hr)
    {
        if (hr != S_FALSE)
            ShowError();
        return;
    }

    RECT rect;
    GetWindowRect(_hwnd, &rect);

    int iOffset = GetSystemMetrics(SM_CYCAPTION);
    OffsetRect(&rect, iOffset, iOffset);

    hr = THR(Reply(eType, _hwnd, &rect));
    if (!hr)
    {
        ShutdownForm(SAVEOPTS_NOSAVE);
    }
    else
    {
        ShowError();
    }
}

///  CPadMessage::DoCopy
//
//  called only from our UI
void CPadMessage::DoCopy(void)
{
    HRESULT         hr;
    LPMAPIFOLDER    pfld = NULL;
    LPMDB           pmdb = NULL;

    if (_pses == NULL)
    {
        hr = THR(_pmsgsite->GetSession(&_pses));
        if (hr)
        {
            g_LastError.SetLastError(hr, _pmsgsite);
            ShowError();
            return;
        }
    }

    BOOL fOldModalUp = s_fModalUp;

    s_fModalUp = TRUE;

    hr = THR(HrPickFolder(g_hInstCore, _hwnd, _pses, &pfld, &pmdb,
                                &_cbCFDState, &_pbCFDState));


    s_fModalUp = fOldModalUp;

    if (hr)
    {
        if (hr != MAPI_E_USER_CANCEL)
            ShowMessageBox(_hwnd, TEXT("Can't copy"), g_achFormName, MB_OK | MB_ICONSTOP);

        return;
    }

    Assert(_pmsgsite);
    Assert(pfld);
    Assert(pmdb);

    hr = THR(_pmsgsite->CopyMessage(pfld));
    pfld->Release();
    pmdb->Release();
    if (hr)
    {
        g_LastError.SetLastError(hr, _pmsgsite);
        ShowError();
        return;
    }

}


///  CPadMessage::DoMove
//
//  called only from our UI
void CPadMessage::DoMove(void)
{
    HRESULT         hr;
    LPMAPIFOLDER    pfld = NULL;
    LPMDB           pmdb = NULL;

    if (_pses == NULL)
    {
        hr = THR(_pmsgsite->GetSession(&_pses));
        if (hr)
        {
            g_LastError.SetLastError(hr, _pmsgsite);
            ShowError();
            return;
        }
    }


    BOOL fOldModalUp = s_fModalUp;

    s_fModalUp = TRUE;

    hr = THR(HrPickFolder(g_hInstCore, _hwnd, _pses, &pfld, &pmdb,
                                //&_cbCFDState, &_pbCFDState);
                                NULL, NULL));

    s_fModalUp = fOldModalUp;

    if (hr)
    {
        if (hr != MAPI_E_USER_CANCEL)
            ShowMessageBox(_hwnd, TEXT("Can't move"), g_achFormName, MB_OK | MB_ICONSTOP);

        return;
    }

    Assert(_pmsgsite);
    Assert(pfld);
    Assert(pmdb);

    RECT rect;
    GetWindowRect(_hwnd, &rect);

    hr = THR(_pmsgsite->MoveMessage(pfld, ViewCtx(), &rect));
    pfld->Release();
    pmdb->Release();
    if (FAILED(hr))
    {
        g_LastError.SetLastError(hr, _pmsgsite);
        ShowError();
        return;
    }

    if (NULL == _pmsg)
    {
        ShutdownForm(SAVEOPTS_NOSAVE);
    }


}

//wraper for MessageBox()
int CPadMessage::ShowMessageBox(HWND hwnd, LPCTSTR lpszText, LPCTSTR lpszTitle, UINT uiStyle)
{
    int iret;
    BOOL fOldModalUp = s_fMBoxUp;

    s_fMBoxUp = TRUE;

    iret = MessageBox(hwnd, lpszText, lpszTitle, uiStyle);

    s_fMBoxUp = fOldModalUp;

    return iret;
}

//wraper for g_LastError.ShowError()
void CPadMessage::ShowError(void)
{
    int iret;
    BOOL fOldModalUp = s_fMBoxUp;

    s_fMBoxUp = TRUE;

    iret = g_LastError.ShowError(_hwnd);

    s_fMBoxUp = fOldModalUp;

}

HRESULT
CPadMessage::StreamInHtmlBody(LPMESSAGE pmsg)
{
    HRESULT hr;
    LPSTREAM pStm = NULL;

    if (!_fNewMessage)
    {
        hr = THR(pmsg->OpenProperty(PR_HTML_BODY, &IID_IStream,
                                STGM_READ, 0, (LPUNKNOWN FAR *) &pStm));
        if (hr)
            goto err;
    }

    hr = THR(Open(pStm));

err:
    ReleaseInterface(pStm);
    RRETURN(hr);
}

HRESULT
CPadMessage::StreamInHtmlBody(char * pch)
{
    HRESULT hr;
    LPSTREAM pStm = NULL;
    DWORD cb;
    DWORD cbWritten;
    LARGE_INTEGER i64Start = {0, 0};

    hr = THR(CreateStreamOnHGlobal(NULL, TRUE, &pStm));
    if (hr)
        goto err;

    cb = lstrlenA(pch);

    hr = THR(pStm->Write(pch, cb, &cbWritten));
    if (hr)
        goto err;

    hr = THR(pStm->Seek(i64Start, STREAM_SEEK_SET, NULL));
    if (hr)
        goto err;

    Assert (cbWritten == cb);

    hr = THR(Open(pStm));

err:
    ReleaseInterface(pStm);
    RRETURN(hr);
}

HRESULT
CPadMessage::StreamOutHtmlBody(LPMESSAGE pmsg)
{
    Assert(pmsg);

    HRESULT hr;
    LPSTREAM pStm = NULL;

    hr = THR(pmsg->OpenProperty(PR_HTML_BODY, &IID_IStream,
                            STGM_READWRITE, MAPI_CREATE | MAPI_MODIFY,
                            (LPUNKNOWN FAR *) &pStm));
    if (hr)
        goto err;

    hr = THR(CPadDoc::Save(pStm));

err:
    ReleaseInterface(pStm);
    RRETURN(hr);
}

HRESULT
CPadMessage::StreamOutTextBody(LPMESSAGE pmsg)
{
    Assert(pmsg);

    HRESULT                 hr;
    LPSTREAM                pStm;
    LPSTREAM                pStmIn;
    FORMATETC               formatetc;
    STGMEDIUM               medium;
    IDataObject *           pDO;

    pStmIn = NULL;
    pStm = NULL;
    pDO = NULL;

    formatetc.cfFormat = RegisterClipboardFormat(_T("CF_RTF"));
    formatetc.ptd = NULL;
    formatetc.dwAspect = DVASPECT_CONTENT;
    formatetc.lindex = -1;
    formatetc.tymed = TYMED_ISTREAM;

    hr = THR(_pObject->QueryInterface(IID_IDataObject, (void**)&pDO));
    if (hr)
        goto err;

    hr = THR(pmsg->OpenProperty(PR_RTF_COMPRESSED, &IID_IStream,
            0, MAPI_CREATE | MAPI_MODIFY, (LPUNKNOWN *) &pStmIn));
    if (hr)
        goto err;

    hr = THR(WrapCompressedRTFStream(pStmIn, MAPI_MODIFY, &pStm));
    if (hr)
        goto err;

    medium.tymed = TYMED_ISTREAM;
    medium.pstm = pStm;
    medium.pUnkForRelease = NULL;

    hr = THR(pDO->GetDataHere(&formatetc, &medium));
    if (hr)
    {
        //
        // RTF failed, try plain text
        //

        ReleaseInterface(pStm);

        formatetc.cfFormat = CF_TEXT;
        hr = THR(pmsg->OpenProperty(PR_BODY_A, &IID_IStream,
                    STGM_READWRITE, MAPI_CREATE | MAPI_MODIFY,
                    (LPUNKNOWN *) &pStm));
        if (hr)
            goto err;

        medium.pstm = pStm;

        hr = THR(pDO->GetDataHere(&formatetc, &medium));
        if (hr)
            goto err;

        hr = THR(pStm->Write(g_achPlainTextHeader, strlen(g_achPlainTextHeader), NULL));
        if (hr)
            goto err;

    }
    else
    {
        hr = THR(pStm->Commit(STGC_OVERWRITE));
        if (hr)
            goto err;
    }

err:
    ReleaseInterface(pStm);
    ReleaseInterface(pStmIn);
    ReleaseInterface(pDO);
    RRETURN(hr);
}

/*
 *  Formats a Win32 file time as a MAPI date/time string.
 *  NOTE: converts from GMT to local time.
 */
void FormatTime(FILETIME *pft, LPSTR szTime, DWORD cchTime)
{
    FILETIME        ft;
    SYSTEMTIME      systime;
    DWORD           dwLen;

    FileTimeToLocalFileTime(pft, &ft);
    FileTimeToSystemTime(&ft, &systime);

    dwLen = GetDateFormatA(
        LOCALE_USER_DEFAULT,
        DATE_LONGDATE,
        &systime,
        NULL,
        szTime,
        cchTime);

    Assert(cchTime - dwLen > 0);

    *(szTime + dwLen - 1) = ' '; // Replace terminating NULL with space

    GetTimeFormatA(
        LOCALE_USER_DEFAULT,
        TIME_NOSECONDS,
        &systime,
        NULL,
        szTime + dwLen,
        cchTime - dwLen);
}


void
CPadMessage::GetViewRect(RECT *prc, BOOL fIncludeObjectAdornments)
{
    RECT    rcDialog;
    RECT    rcFormat;
    RECT    rcToolbar;
    RECT    rcStatus;

    GetClientRect(_hwnd, prc);

    if (_hwndToolbar)
    {
        GetWindowRect(_hwndToolbar, &rcToolbar);
        SetWindowPos(_hwndToolbar, 0, 0, prc->top, prc->right - prc->left, rcFormat.bottom - rcFormat.top, SWP_NOZORDER | SWP_NOACTIVATE);
        prc->top += rcToolbar.bottom - rcToolbar.top;
    }

    if (_hwndTBFormat)
    {
        GetWindowRect(_hwndTBFormat, &rcFormat);
        SetWindowPos(_hwndTBFormat, 0, 0, prc->top, prc->right - prc->left, rcFormat.bottom - rcFormat.top, SWP_NOZORDER | SWP_NOACTIVATE);
        prc->top += rcFormat.bottom - rcFormat.top;
    }

    if (_hwndStatus)
    {
        GetWindowRect(_hwndStatus, &rcStatus);
        prc->bottom += rcStatus.top - rcStatus.bottom;
    }

    // Reduce rectangle to leave space for address controls

    if (fIncludeObjectAdornments)
    {
        GetWindowRect (_hwndDialog,&rcDialog);
        SetWindowPos(_hwndDialog, 0, 0, prc->top, prc->right - prc->left, rcDialog.bottom - rcDialog.top, SWP_NOZORDER | SWP_NOACTIVATE);

        prc->top += rcDialog.bottom - rcDialog.top;
        if (prc->top > prc->bottom)
        {
            prc->top = prc->bottom;
        }
    }

    if (prc->bottom < prc->top)
        prc->bottom = prc->top;

    if (prc->right < prc->left)
        prc->right = prc->left;
}


LRESULT
CPadMessage::OnSize(WORD fwSizeType, WORD nWidth, WORD nHeight)
{
    RECT rc;
    GetViewRect(&rc, FALSE);
    if (_pInPlaceActiveObject)
    {
        THR_NOTRACE(_pInPlaceActiveObject->ResizeBorder(
                &rc,
                &_Frame,
                TRUE));
    }

    Resize();
    return 0;
}


HRESULT
CPadMessage::InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS pmgw)
{
    if (_eFormType == eformRead)
    {
        AppendMenu(
                hmenuShared,
                MF_BYPOSITION | MF_POPUP,
                (UINT)GetSubMenu(_hmenuMain, 0),
                TEXT("&File"));
        AppendMenu(
                hmenuShared,
                MF_BYPOSITION | MF_POPUP,
                (UINT)GetSubMenu(_hmenuMain, 1),
                TEXT("&View"));
        AppendMenu(
                hmenuShared,
                MF_BYPOSITION | MF_POPUP,
                (UINT)GetSubMenu(_hmenuMain, 2),
                TEXT("Co&mpose"));

        pmgw->width[0] = 1;
        pmgw->width[2] = 1;
        pmgw->width[4] = 1;
    }
    else if (_eFormType == eformSend)
    {
        AppendMenu(
                hmenuShared,
                MF_BYPOSITION | MF_POPUP,
                (UINT)GetSubMenu(_hmenuMain, 0),
                TEXT("&File"));

        pmgw->width[0] = 1;
        pmgw->width[2] = 0;
        pmgw->width[4] = 0;
    }
    else
    {
        Assert(FALSE);
    }

    _hmenuHelp = LoadMenu(
            g_hInstResource,
            MAKEINTRESOURCE(IDR_PADHELPMENU));

    AppendMenu(
            hmenuShared,
            MF_BYPOSITION | MF_POPUP,
            (UINT) (_hmenuHelp),
            TEXT("&Help"));

    _cMenuHelpItems = GetMenuItemCount(_hmenuHelp);

    pmgw->width[5] = 2;

    return S_OK;
}

void
CPadMessage::SetDocTitle(TCHAR * pchTitle)
{
    // don't do anything here since we set our own title
}

//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       ebgsound.cxx
//
//  Contents:   CBGsound & related
//
//
//----------------------------------------------------------------------------

#include "headers.hxx"      // for the world

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"     // for cbitsctx
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"     // for celement
#endif

#ifndef X_EBGSOUND_HXX_
#define X_EBGSOUND_HXX_
#include "ebgsound.hxx"     // for cbgsound
#endif

#ifndef X_MMPLAY_HXX_
#define X_MMPLAY_HXX_
#include "mmplay.hxx"       // for ciemmplayer
#endif

#define _cxx_
#include "bgsound.hdl"      

MtDefine(CBGsound, Elements, "CBGsound")

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const CBGsound::CLASSDESC CBGsound::s_classdesc =
{
    {
        &CLSID_HTMLBGsound,                 // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                               // _pcpi
        ELEMENTDESC_NOLAYOUT,               // _dwFlags
        &IID_IHTMLBGsound,                  // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLBGsound
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
HRESULT CBGsound::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(ppElement);

    *ppElement = new CBGsound(pDoc);

    return *ppElement ? S_OK : E_OUTOFMEMORY;
}


HRESULT
CBGsound::EnterTree()
{
    // BUGBUG: This code is moved directly from Init2.  While init
    // is always called just once, this notification will be sent
    // each time this element enters a tree.  There may be bugs
    // dealing with this notification coming at the wrong time or
    // more than once.  BTW, there is also a matching SN_EXITTREE
    // notification (jbeda)

    CDoc *pDoc = Doc();

    if (pDoc && (pDoc->State() >= OS_INPLACE))
        _fIsInPlace = TRUE;

    THR(OnPropertyChange(DISPID_CBGsound_src, 0));

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CBGSound::Init2
//
//  Synopsis:   Init override
//
//----------------------------------------------------------------------------

HRESULT 
CBGsound::Init2(CInit2Context * pContext)
{
    CDoc * pDoc = Doc();

    pDoc->_fBroadcastInteraction = TRUE;
    pDoc->_fBroadcastStop = TRUE;

    RRETURN(super::Init2(pContext));
}

//+---------------------------------------------------------------------------
//
//  Member:     CBGSound::Notify
//
//  Synopsis:   Handle notifications
//
//----------------------------------------------------------------------------

void
CBGsound::Notify(CNotification *pNF)
{
    super::Notify(pNF);
    switch (pNF->Type())
    {
    case NTYPE_DOC_STATE_CHANGE_1:
        if (!!_fIsInPlace != (Doc()->State() >= OS_INPLACE))
        {
            pNF->SetSecondChanceRequested();
        }
        break;

    case NTYPE_DOC_STATE_CHANGE_2:
        {
            CDoc * pDoc = Doc();

            Assert( !!_fIsInPlace != (pDoc->State() >= OS_INPLACE) );

            _fIsInPlace = !_fIsInPlace;

            SetAudio();

            if (_pBitsCtx && _fIsInPlace)
            {
                DWNLOADINFO dli;
                pDoc->InitDownloadInfo(&dli);
                _pBitsCtx->SetLoad(TRUE, &dli, FALSE);
            }
        }
        break;

    case NTYPE_STOP_1:
        if (_pBitsCtx)
            _pBitsCtx->SetLoad(FALSE, NULL, FALSE);

        if (_pSoundObj)
            pNF->SetSecondChanceRequested();
        break;

    case NTYPE_STOP_2:
        if (_pSoundObj)
        {
            _pSoundObj->Stop();    // stop whatever we were playing
            _fStopped = TRUE;
        }
        break;

    case NTYPE_ENABLE_INTERACTION_1:
        pNF->SetSecondChanceRequested();
        break;

    case NTYPE_ENABLE_INTERACTION_2:
        SetAudio();
        break;

    case NTYPE_ACTIVE_MOVIE:
        {
            void * pv;

            pNF->Data(&pv);

            if (_pSoundObj && (pv == this))
                _pSoundObj->NotifyEvent();              
                // Let the sound object know something happened
        }
        break;

    case NTYPE_ELEMENT_EXITTREE_1:
        if (_pSoundObj)
        {
            pNF->SetSecondChanceRequested();
        }
        break;

    case NTYPE_ELEMENT_EXITTREE_2:
        if (_pSoundObj)
        {
            _pSoundObj->Release();
            _pSoundObj = NULL;
        }
        break;

    case NTYPE_ELEMENT_ENTERTREE:
        EnterTree();
        break;
        
    case NTYPE_BASE_URL_CHANGE:
        OnPropertyChange( DISPID_CBGsound_src, ((PROPERTYDESC *)&s_propdescCBGsoundsrc)->GetdwFlags());
        break;
    }
}


void
CBGsound::SetBitsCtx(CBitsCtx * pBitsCtx)
{
    if (_pBitsCtx)
    {
        if (_pSoundObj)
        {
            _pSoundObj->Stop();    // stop whatever we were playing
            _pSoundObj->Release();
            _pSoundObj = NULL;
        }
        
        _pBitsCtx->SetProgSink(NULL); // detach download from document's load progress
        _pBitsCtx->Disconnect();
        _pBitsCtx->Release();
    }

    _pBitsCtx = pBitsCtx;

    if (pBitsCtx)
    {
        pBitsCtx->AddRef();

        _fStopped = FALSE;

        if (pBitsCtx->GetState() & (DWNLOAD_COMPLETE | DWNLOAD_ERROR | DWNLOAD_STOPPED))
            OnDwnChan(pBitsCtx);
        else
        {
            pBitsCtx->SetProgSink(Doc()->GetProgSink());
            pBitsCtx->SetCallback(OnDwnChanCallback, this);
            pBitsCtx->SelectChanges(DWNCHG_COMPLETE, 0, TRUE);
        }
    }
}

//+------------------------------------------------------------------------
//
//  Method:     CBgsound::SetAudio
//
//-------------------------------------------------------------------------

void CBGsound::SetAudio()
{
    CDoc *  pDoc = Doc();

    if (!_pSoundObj)
        return;

    if (_fIsInPlace && pDoc->_fEnableInteraction && !_fStopped)
    {
        _pSoundObj->SetNotifyWindow(pDoc->GetHWND(), WM_ACTIVEMOVIE, (LONG_PTR)this);
        _pSoundObj->Play();
    }
    else
    {
        _pSoundObj->SetNotifyWindow(NULL, WM_ACTIVEMOVIE, (LONG_PTR)this);
        _pSoundObj->Stop();
    }
}

//+------------------------------------------------------------------------
//
//  Method:     CBGsound::OnDwnChan
//
//-------------------------------------------------------------------------

void CBGsound::OnDwnChan(CDwnChan * pDwnChan)
{
    ULONG ulState = _pBitsCtx->GetState();
    BOOL fDone = FALSE;
    TCHAR * pchFile = NULL;
   
    if (ulState & DWNLOAD_COMPLETE)
    {
        fDone = TRUE;

        // If security redirect occurred, we may need to blow away doc's lock icon
        Doc()->OnSubDownloadSecFlags(_pBitsCtx->GetUrl(), _pBitsCtx->GetSecFlags());

        // Ensure a sound object
        // BUGBUG: Maybe we need to release or at least stop the current sound
        if(!_pSoundObj)
        {
            _pSoundObj = (CIEMediaPlayer *)new (CIEMediaPlayer);

            if(!_pSoundObj)
                goto Nosound;
        }

        if((S_OK == _pBitsCtx->GetFile(&pchFile)) &&
           (S_OK == _pSoundObj->SetURL(pchFile)))       // Initialize & RenderFile
        {
            // Set the volume & balance values
            //

            // if the colume property is not initialized to something within range 
            // get the value from the soundobject and set it to that
            //
            if(GetAAvolume() > 0)   // range is -10000 to 0
            {
                VARIANT vtLong;

                vtLong.vt = VT_I4;
                vtLong.lVal = _pSoundObj->GetVolume();

                put_VariantHelper(vtLong, (PROPERTYDESC *)&s_propdescCBGsoundvolume);
            }
            else
                _pSoundObj->SetVolume(GetAAvolume());

            if(GetAAbalance() > 10000)   // range is -10000 to 10000
            {
                VARIANT vtLong;

                vtLong.vt = VT_I4;
                vtLong.lVal = _pSoundObj->GetBalance();

                put_VariantHelper(vtLong, (PROPERTYDESC *)&s_propdescCBGsoundbalance);
            }
            else
                _pSoundObj->SetBalance(GetAAbalance());

            _pSoundObj->SetLoopCount(GetAAloop());

            SetAudio();

        }

    }
    else if (ulState & (DWNLOAD_STOPPED | DWNLOAD_ERROR))
    {
        //
        // BUGBUG:
        // else (if we decide to implement event firing) we should fire an 
        // event to signal the error
        //
        fDone = TRUE;
    }

Nosound:
    if (fDone)
    {
        _pBitsCtx->SetProgSink(NULL); // detach download from document's load progress
    }

    MemFreeString(pchFile);
    
}

//+------------------------------------------------------------------------
//
//  Member:     OnPropertyChange
//
//  Note:       Called after a property has changed to notify derived classes
//              of the change.  All properties (except those managed by the
//              derived class) are consistent before this call.
//
//              Also, fires a property change notification to the site.
//
//-------------------------------------------------------------------------

HRESULT CBGsound::OnPropertyChange(DISPID dispid, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    CDoc *pDoc = Doc();

    switch (dispid)
    {
        case DISPID_CBGsound_src:
        {
            // Should we even play the sound?
            if (pDoc->_dwLoadf & DLCTL_BGSOUNDS)
            {
                CBitsCtx *pBitsCtx = NULL;
                LPCTSTR szUrl = GetAAsrc();

                hr = THR(pDoc->NewDwnCtx(DWNCTX_FILE, szUrl, this,
                            (CDwnCtx **)&pBitsCtx));

                if (hr == S_OK)
                {
                    SetBitsCtx(pBitsCtx);

                    if (pBitsCtx)
                        pBitsCtx->Release();
                }
            }
            break;
        }

        case DISPID_CBGsound_loop:
            if(_pSoundObj)
                _pSoundObj->SetLoopCount(GetAAloop());
            break;

        case DISPID_CBGsound_balance:
            if(_pSoundObj)
                _pSoundObj->SetBalance(GetAAbalance());
            break;
        
        case DISPID_CBGsound_volume:
            if(_pSoundObj)
                _pSoundObj->SetVolume(GetAAvolume());
            break;
    }

    if (OK(hr))
        hr = THR(super::OnPropertyChange(dispid, dwFlags));

    RRETURN(hr);
}

//--------------------------------------------------------------------------
//
//  Method:     CBGsound::Passivate
//
//  Synopsis:   Shutdown main object by releasing references to
//              other objects and generally cleaning up.  This
//              function is called when the main reference count
//              goes to zero.  The destructor is called when
//              the reference count for the main object and all
//              embedded sub-objects goes to zero.
//
//              Release any event connections held by the form.
//
//--------------------------------------------------------------------------

void CBGsound::Passivate(void)
{
    if (_pSoundObj)
    {
        _pSoundObj->Release();
        _pSoundObj = NULL;
    }
    SetBitsCtx(NULL);
    super::Passivate();
}

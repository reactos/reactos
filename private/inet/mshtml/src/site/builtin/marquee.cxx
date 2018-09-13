//+---------------------------------------------------------------------
//
//   File:      inputtxt.cxx
//
//  Contents:   InputTxt element class, etc..
//
//  Classes:    CInputTxtBase, etc..
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_CGUID_H_
#define X_CGUID_H_
#include <cguid.h>
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_TASKMAN_HXX_
#define X_TASKMAN_HXX_
#include "taskman.hxx"
#endif

#ifndef X_MARQUEE_HXX_
#define X_MARQUEE_HXX_
#include "marquee.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_MARQLYT_HXX_
#define X_MARQLYT_HXX_
#include "marqlyt.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_EFORM_HXX_
#define X_EFORM_HXX_
#include "eform.hxx"
#endif

#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"
#endif

#ifndef X_PROPS_HXX_
#define X_PROPS_HXX_
#include "props.hxx"
#endif

#ifndef X_DISPDEFS_HXX_
#define X_DISPDEFS_HXX_
#include "dispdefs.hxx"
#endif

#define _cxx_
#include "marquee.hdl"

DeclareTag(tagMarquee,          "Marquee", "Trace Marquee Ticks")
MtDefine(CMarquee, Elements, "CMarquee")
MtDefine(CMarqueeTask, CMarquee, "CMarqueeTask")

IMPLEMENT_LAYOUT_FNS(CMarquee, CMarqueeLayout)


//+-------------------------------------------------------------------
// CMarqueeTask methods
//---------------------------------------------------------------------

CMarqueeTask::CMarqueeTask(CMarquee *pMarquee)
{
    _pMarquee = pMarquee;
}

void
CMarqueeTask::OnRun(DWORD dwTimeout)
{
    _pMarquee->OnRun(dwTimeout);
}



#ifndef NO_PROPERTY_PAGE
const CLSID * CMarquee::s_apclsidPages[] =
{
    // Browse-time pages
    NULL,
    // Edit-time pages
#if DBG==1    
    &CLSID_CCDGenericPropertyPage,
    &CLSID_CInlineStylePropertyPage,
#endif // DBG==1    
    NULL
};
#endif // NO_PROPERTY_PAGE

const CElement::CLASSDESC CMarquee::s_classdesc =
{
    {
        &CLSID_HTMLMarqueeElement,      // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_NOTIFYENDPARSE |
        ELEMENTDESC_TEXTSITE       |
        ELEMENTDESC_CANSCROLL      |
        ELEMENTDESC_ANCHOROUT      |
        ELEMENTDESC_NOBKGRDRECALC,      // _dwFlags
        &IID_IHTMLMarqueeElement,       // _piidDispinterface
        &s_apHdlDescs,                  // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLMarqueeElement,  // _pfnTearOff
    &CTxtSite::s_AccelsTxtSiteDesign,   // _pAccelsDesign
    &CTxtSite::s_AccelsTxtSiteRun       // _pAccelsRun
};

HRESULT
CMarquee::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(ppElement);

    *ppElement = new CMarquee(pht->GetTag(), pDoc);

    RRETURN ( (*ppElement) ? S_OK : E_OUTOFMEMORY);
}


HRESULT
CMarquee::Init2(CInit2Context * pContext)
{
    HRESULT hr = S_OK;

    Doc()->_fBroadcastInteraction = TRUE;

    hr = THR(super::Init2(pContext));

    _pMarqueeTask  = NULL;

    if (!OK(hr))
        goto Cleanup;

    Layout()->_fContentsAffectSize = TRUE;

    // These we do not set inside InitScrollParam
    _fStop = FALSE;
    _fDone = FALSE;
    _fFirstRun = TRUE;
    _fIsVisible = TRUE;

    InitScrollParams();

Cleanup:
    RRETURN1(hr, S_INCOMPLETE);
}

void
CMarquee::InitScrollParams()
{
    _direction = GetAAdirection();
    _lLoop = GetAAloop();

    _fSwitch = GetAAbehavior() == htmlMarqueeBehavioralternate;
    _fSlide = GetAAbehavior() == htmlMarqueeBehaviorslide;
    _lScrollBy = GetAAscrollAmount();
    _lScrollDelay = GetAAscrollDelay();
    _fTrueSpeed = GetAAtrueSpeed();

    if (_lScrollDelay < 60 && !_fTrueSpeed)
    {
        _lScrollDelay = 60;
    }

    Assert(_lScrollDelay > 0);

    if (_lLoop > 0 &&
        !_fSwitch &&
        !_fSlide &&
        (_direction == htmlMarqueeDirectionright ||
         _direction == htmlMarqueeDirectiondown))
    {
        // THe way we do scrolling we're going to lose
        // one loop in the right and down case
        _lLoop++;
    }

    // In the new layout world wordwrap is set when the marquee is
    // inserted in the tree.

    // We were asked to allow looping on slide behavior. The default loop value though is -1
    // which usually means loop for ever. But this is not good for Slide so we have to
    // make a special case here
    if (_fSlide && _lLoop < 0)
    {
        _lLoop = 1;
    }
}



void
CMarquee::OnRun(DWORD dwTimeout)
{
    long    lScrollDelta;
    long    lSwitchPoint;
    long    *plPos;
    long    *plPos2;       
    long    lScroll;
    long    lText;
    long    lMargin;
    long    lMargin2;

    long    lPosStart;
    long    lPosEnd;

    BOOL    fTooBig;

    BOOL    fShouldSwitch;
    BOOL    fPassedEdge;

    BOOL    fLeftOrUp       = TRUE;
    BOOL    fLeftOrRight    = TRUE;
    htmlMarqueeDirection    dirOpposite = _direction;
    htmlMarqueeBehavior     behavior;

    #if DBG==1
    __int64 t1, t2, tfrq;
    QueryPerformanceCounter((LARGE_INTEGER *)&t1);
    QueryPerformanceCounter((LARGE_INTEGER *)&t2);
    QueryPerformanceFrequency((LARGE_INTEGER *)&tfrq);
    #endif


    // BUGBUG (gideons) we should stop the Task under these conditions
    if (_fDone || _fStop || _lLoop == 0 || IsEditable(TRUE))
        return;
    if (0 == _dwOldTimeout)
    {
        _dwOldTimeout = dwTimeout;
    }


    DWORD dwTimeDelta = dwTimeout - _dwOldTimeout;


    if (dwTimeout > _dwOldTimeout)
    {
        lScrollDelta = MulDivQuick(_lScrollBy, dwTimeDelta, _lScrollDelay); //GetAAscrollDelay());
    }
    else
    {
        // overflow case
        lScrollDelta = _lScrollBy;
    }

    TraceTag((tagMarquee, "OnRun: trueSpeed=%ld Delay %ld last %ld tks ago Amount %ld Delta %ld", _fTrueSpeed ? 1 : 0,_lScrollDelay, dwTimeout - _dwOldTimeout, _lScrollBy, lScrollDelta));

    _dwOldTimeout = dwTimeout;

    if (_fFirstRun)
    {
        Fire_onstart();
    }

    switch (_direction)
    {
    case    htmlMarqueeDirectionleft:
            fLeftOrUp       = !_fRightToLeft;
            dirOpposite     = _fRightToLeft ? _direction : htmlMarqueeDirectionright;
            break;
    case    htmlMarqueeDirectionright:
            fLeftOrUp       = _fRightToLeft;
            dirOpposite     = _fRightToLeft ? _direction : htmlMarqueeDirectionleft;
            break;
    case    htmlMarqueeDirectionup:
            fLeftOrRight    = FALSE;
            dirOpposite     = htmlMarqueeDirectiondown;
            break;
    case    htmlMarqueeDirectiondown:
            fLeftOrUp       = FALSE;
            fLeftOrRight    = FALSE;
            dirOpposite     = htmlMarqueeDirectionup;
            break;
    default:
            AssertSz(0, "Wrong htmlMarqueeDirection enum value");
    }


    plPos       = fLeftOrRight ? &_lXPos : &_lYPos;
    plPos2      = fLeftOrRight ? &_lYPos : &_lXPos;
    lScroll     = fLeftOrRight ? _sizeScroll.cx : _sizeScroll.cy;

    if (!lScroll)
        goto Cleanup;

    lMargin     = fLeftOrRight ? _lXMargin : _lYMargin;
    lMargin2    = fLeftOrRight ? _lYMargin : _lXMargin;
    lText       = lScroll - 2*lMargin;

    lPosStart   = fLeftOrUp ? 0 : lScroll - lMargin;
    lPosEnd     = fLeftOrUp ? lScroll - lMargin : 0;

    if (fLeftOrRight)
    {
        fTooBig = fLeftOrUp ? _fToBigForSwitch : !_fToBigForSwitch;
    }
    else
    {
        fTooBig = !fLeftOrUp;
    }

    behavior = GetAAbehavior();
    switch (behavior)
    {
    case    htmlMarqueeBehaviorslide:
            if (_fFirstRun)
            {
                *plPos = lPosStart;
            }
    case    htmlMarqueeBehavioralternate:
            lSwitchPoint = fTooBig ?  lText : lMargin;
            if (behavior == htmlMarqueeBehavioralternate && _fFirstRun)
            {
                *plPos = fTooBig ? lMargin : lText;
            }
            else
            {
                fShouldSwitch = fLeftOrUp ? (*plPos >= lSwitchPoint) : (*plPos <= lSwitchPoint);
                if (fShouldSwitch)
                {
                    // Count this scroll, if we have an active loop count
                    if (_lLoop > 0)
                    {
                        _lLoop--;
                        if (_lLoop == 0)
                        {
                            _fDone = TRUE;
                            if (!Fire_onfinish())
                            {
                                _lLoop=-1; // On finish was cancelled so we continue for ever.
                            }
                            goto Cleanup;
                        }
                    }

                    if (behavior == htmlMarqueeBehaviorslide)
                    {
                        *plPos = 0;
                        Fire_onstart();
                    }
                    else    // alternate
                    {
                        *plPos = lSwitchPoint ;
                        Fire_onbounce();
                        _direction = dirOpposite;
                        goto Cleanup;
                    }
                }
                else
                {
                    *plPos += fLeftOrUp ? lScrollDelta : -lScrollDelta;
                    fPassedEdge = fLeftOrUp ? *plPos > lSwitchPoint : *plPos < lSwitchPoint;
                    if (fPassedEdge)
                    {   
                        // Dont baunce behind the edge
                        *plPos = lSwitchPoint;
                    }
                }
            }
            break;
    default:
            {
                BOOL    fGoToofar = fLeftOrUp ? (*plPos >= lPosEnd) : (*plPos <= lPosEnd);
                if (fGoToofar)
                {
                    *plPos = lPosStart;
                    Fire_onstart();
                    // Count this scroll, if we have an active loop count
                    if (_lLoop > 0)
                    {
                        _lLoop--;
                        if (_lLoop == 0)
                        {
                            if (!Fire_onfinish())
                            {
                                _lLoop=-1; // On finish was cancelled so we continue for ever.
                            }
                            goto Cleanup;
                        }
                    }
                }
                *plPos += fLeftOrUp ? lScrollDelta : -lScrollDelta;
            }
    }

    *plPos2 = lMargin2;

    if (Doc()->State() >= OS_INPLACE)
    {
        Layout()->ScrollTo(!_fRightToLeft ? _lXPos : -_lXPos, _lYPos);
    }

#if DBG==1
    QueryPerformanceCounter((LARGE_INTEGER *)&t2);
    TraceTag((tagMarquee, "OnRun: ScrollView took %ld ticks", GetTickCount() - dwTimeout));
    TraceTag((tagMarquee, "ScrollView done in %ld", (LONG)(((t2 - t1) * 1000000) / tfrq)));
#endif

Cleanup:
    _fFirstRun = FALSE;
    return;
}


#ifdef NEVER
TXTBACKSTYLE
CMarquee::GetBackStyle() const
{
    return ((CMarquee *) this)->GetCascadedbackgroundColor().IsDefined() ? TXTBACK_OPAQUE : TXTBACK_TRANSPARENT;
}
#endif


HRESULT BUGCALL
CMarquee::stop()
{
    _fStop = TRUE;
    // this will cause the OnRun to start from where it stopped
    _dwOldTimeout = 0;
    return S_OK;
}

HRESULT BUGCALL
CMarquee::start()
{
    _fStop = FALSE;
    return S_OK;
}

void
CMarquee::SetMarqueeTask(void)
{
    CDoc * pDoc = Doc();

    if (    pDoc->State() >= OS_INPLACE
        &&  !IsDisplayNone()
        &&  pDoc->_fEnableInteraction
        &&  IsInPrimaryMarkup())
    {
        if (_pMarqueeTask == NULL)
        {
            _pMarqueeTask = new CMarqueeTask(this);
            if (_pMarqueeTask)
            {
                _pMarqueeTask->SetInterval(_lScrollDelay);
            }
        }
    }
    else
    {
        if (_pMarqueeTask)
        {
            _pMarqueeTask->Terminate();
            _pMarqueeTask->Release();
            _pMarqueeTask = NULL;
        }
    }

}


void
CMarquee::Passivate()
{
    SetMarqueeTask();
    super::Passivate();
}

//+------------------------------------------------------------------------
//
//  Member:     CMarquee::Notify
//
//  Synopsis:   Listen for inplace (de)activation so we can turn on/off
//              Scrolling
//
//-------------------------------------------------------------------------

void
CMarquee::Notify(CNotification *pNF)
{
    super::Notify(pNF);
    switch (pNF->Type())
    {
    case NTYPE_ELEMENT_EXITTREE_1:
        if (_pMarqueeTask)
        {
            _pMarqueeTask->Terminate();
            _pMarqueeTask->Release();
            _pMarqueeTask = NULL;
        }
        break;
    case NTYPE_ELEMENT_ENTERTREE:
    case NTYPE_ENABLE_INTERACTION_1:
    case NTYPE_DOC_STATE_CHANGE_1:
        SetMarqueeTask();
        break;
    }
}


#ifndef NO_DATABINDING
class CDBindMethodsMarquee : public CDBindMethodsText
{
    typedef CDBindMethodsText super;
public:
    CDBindMethodsMarquee() : super(DBIND_ONEWAY|DBIND_HTMLOK) {}
    ~CDBindMethodsMarquee()     {}

    virtual HRESULT BoundValueToElement(CElement *pElem, LONG id,
                                        BOOL fHTML, LPVOID pvData) const;
};

static const CDBindMethodsMarquee DBindMethodsMarquee;

const CDBindMethods *
CMarquee::GetDBindMethods()
{
    return &DBindMethodsMarquee;
}

//+---------------------------------------------------------------------------
//
//  Function: BoundValueToElement, CDBindMethods
//
//  Synopsis: Transfer data into bound marquee.
//
//  Arguments:
//            [id]      - ID of binding point.  For the select, is always
//                        DISPID_VALUE.
//            [pvData]  - pointer to data to transfer. must be BSTR.
//
//----------------------------------------------------------------------------

HRESULT
CDBindMethodsMarquee::BoundValueToElement(CElement *pElem,
                                          LONG id,
                                          BOOL fHTML,
                                          LPVOID pvData) const
{
    HRESULT hr;

    hr = THR(super::BoundValueToElement(pElem, id, fHTML, pvData));
    if (hr)
        goto Cleanup;

    pElem->ResizeElement();

Cleanup:
    RRETURN(hr);
}
#endif // ndef NO_DATABINDING


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

HRESULT
CMarquee::OnPropertyChange(DISPID dispid, DWORD dwFlags)
{
    HRESULT hr = S_OK;

    switch (dispid)
    {

    case DISPID_CMarquee_direction:
        {
            // We need a full recalc since margins are direction dependent
            // and we need to update the display to have the correct scroll
            // params
            ResizeElement();
            break;
        } 
    case DISPID_CMarquee_behavior:
    case DISPID_CMarquee_loop:
        {
            _fStop = FALSE;
            _fDone = FALSE;
            _fFirstRun = TRUE;
            // Fall through
        }
    case DISPID_CMarquee_scrollDelay:
    case DISPID_CMarquee_scrollAmount:
    case DISPID_CMarquee_width:
    case DISPID_CMarquee_height:
            // InitScrollParams will be called later
            break;

    case STDPROPID_XOBJ_VISIBLE:
        {
            // if set position put us out of visible range
            // _fVisible should have no effect
            if(_fInVisibleRange)
            {
                _fIsVisible = Layout()->_fVisible;
            }
        }
    }

    if (OK(hr))
        hr = THR(super::OnPropertyChange(dispid, dwFlags));

    if (OK(hr))
    {
        InitScrollParams();
        SetMarqueeTask();
    }

    RRETURN(hr);
}


HRESULT
CMarquee::ApplyDefaultFormat(CFormatInfo *pCFI)
{
    HRESULT hr = S_OK;

    // BUGBUG We need to decide what we would like to inherit from parent
    // For now we inherit everything
    // In order to avoid inheriting the code below should be compiled

    // override parent's format to default on all formats, so we get only styles
//    pCFI->_cf.InitDefault(Doc()->_pOptionSettings, FALSE);
//    pCFI->_pf.InitDefault();
//    pCFI->_ff.InitDefault();

    pCFI->_bCtrlBlockAlign = pCFI->_bBlockAlign;
    pCFI->_bBlockAlign     = htmlBlockAlignNotSet;

    // our intrinsics shouldn't inherit the cursor property. they have a 'default'
    pCFI->PrepareCharFormat();
    pCFI->_cf()._bCursorIdx = styleCursorAuto;
    pCFI->UnprepareForDebug();

    // Default to percent width.
    pCFI->PrepareFancyFormat();
    pCFI->_ff()._fWidthPercent = pCFI->_ff()._cuvWidth.IsNullOrEnum();
    pCFI->UnprepareForDebug();

    hr = THR(super::ApplyDefaultFormat(pCFI));

    // (paulnel) Store the text flow direction for scrolling. This is set after
    // super:: to make sure any styles have been updated.
    _fRightToLeft = pCFI->_pcf->_fRTL;

    // BUGBUG: until DISPLAY_CHANGE/VISIBILITY_CHANGE are received, this is the HACK
    pCFI->PrepareCharFormat();
    if (!pCFI->_cf()._fVisibilityHidden && !pCFI->_cf()._fDisplayNone)
    {
        if (!_fIsVisible)
        {
            _fIsVisible = TRUE;
            if (_pMarqueeTask == NULL)
            {
                _pMarqueeTask = new CMarqueeTask(this);
                if (_pMarqueeTask)
                {
                    _pMarqueeTask->SetInterval(_lScrollDelay);
                }
            }
        }
    }
    else
    {
        _fIsVisible = FALSE;
    }
    pCFI->UnprepareForDebug();

    RRETURN(hr);
}

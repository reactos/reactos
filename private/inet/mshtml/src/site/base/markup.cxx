#include "headers.hxx"

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_ROOTCTX_HXX
#define X_ROOTCTX_HXX
#include "rootctx.hxx"
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_IRANGE_HXX_
#define X_IRANGE_HXX_
#include "irange.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_PROGSINK_HXX_
#define X_PROGSINK_HXX_
#include "progsink.hxx"
#endif

#ifndef X_CONNECT_HXX_
#define X_CONNECT_HXX_
#include "connect.hxx"
#endif

#ifndef X_HTC_HXX_
#define X_HTC_HXX_
#include "htc.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_DLL_HXX_
#define X_DLL_HXX_
#include "dll.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_ESTYLE_HXX_
#define X_ESTYLE_HXX_
#include "estyle.hxx"
#endif

#ifndef X_ELINK_HXX_
#define X_ELINK_HXX_
#include "elink.hxx"
#endif

#ifndef X_DEBUGGER_HXX_
#define X_DEBUGGER_HXX_
#include "debugger.hxx"
#endif

#ifndef X_XMLNS_HXX_
#define X_XMLNS_HXX_
#include "xmlns.hxx"
#endif

#ifndef X_FPRINT_HXX_
#define X_FPRINT_HXX_
#include "fprint.hxx"
#endif

MtDefine(CMarkup, Tree, "CMarkup")

MtDefine(CMarkupDirtyRangeContext, CMarkup, "CMarkupDirtyRangeContext")
MtDefine(CMarkupDirtyRangeContext_aryMarkupDirtyRange_pv, CMarkupDirtyRangeContext, "CMarkupDirtyRangeContext::aryMarkupDirtyRange::pv")

MtDefine(CMarkupTextFragContext, CMarkup, "CMarkupTextFragContext")
MtDefine(CMarkupTextFragContext_aryMarkupTextFrag_pv, CMarkupTextFragContext,  "CMarkupTextFragContext::aryMarkupTextFrag::pv")
MtDefine(MarkupTextFrag_pchTextFrag, CMarkupTextFragContext,  "MarkupTextFrag::_pchTextFrag")

MtDefine(CDocFrag, CMarkup, "CDocFrag")

MtDefine(CMarkupScriptContext, Tree, "CMarkupScriptContext")
MtDefine(CMarkup_aryScriptEnqueued, CDoc, "CDoc::_aryScriptEnqueued")

#define _cxx_
#include "markup.hdl"

BEGIN_TEAROFF_TABLE(CMarkup, IMarkupContainer)
    TEAROFF_METHOD(CMarkup, OwningDoc, owningdoc, ( IHTMLDocument2 ** ppDoc ))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CMarkup, IMarkupTextFrags)
    TEAROFF_METHOD(CMarkup, GetTextFragCount, gettextfragcount, (long *pcFrags) )
    TEAROFF_METHOD(CMarkup, GetTextFrag, gettextfrag, (long iFrag, BSTR* pbstrFrag, IMarkupPointer* pPointerFrag ) )
    TEAROFF_METHOD(CMarkup, RemoveTextFrag, removetextfrag, (long iFrag) )
    TEAROFF_METHOD(CMarkup, InsertTextFrag, inserttextfrag, (long iFrag, BSTR bstrInsert, IMarkupPointer* pPointerInsert ) )
    TEAROFF_METHOD(CMarkup, FindTextFragFromMarkupPointer, findtextfragfrommarkuppointer, (IMarkupPointer* pPointerFind, long* piFrag, BOOL* pfFragFound ) )
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CMarkup, ISelectionRenderingServices)
    TEAROFF_METHOD(CMarkup, MovePointersToSegment, movepointerstosegment, (    int iSegmentIndex, IMarkupPointer* pStart, IMarkupPointer* pEnd ))
    TEAROFF_METHOD(CMarkup, GetSegmentCount, getsegmentcount, ( int* piSegmentCount, SELECTION_TYPE * peType  ))
    TEAROFF_METHOD(CMarkup, AddSegment, addsegment, ( IMarkupPointer* pStart, IMarkupPointer* pEnd, HIGHLIGHT_TYPE HighlightType, int * iSegmentIndex ))
    TEAROFF_METHOD(CMarkup, AddElementSegment, addelementsegment, ( IHTMLElement* pElement , int * iSegmentIndex ))
    TEAROFF_METHOD(CMarkup, MoveSegmentToPointers, movesegmenttopointers, ( int iSegmentIndex, IMarkupPointer* pStart, IMarkupPointer* pEnd, HIGHLIGHT_TYPE HighlightType ))
    TEAROFF_METHOD(CMarkup, GetElementSegment, getelementsegment, ( int iSegmentIndex, IHTMLElement** ppElement ))
    TEAROFF_METHOD(CMarkup, SetElementSegment, setelementsegment, ( int iSegmentIndex,    IHTMLElement* pElement ))
    TEAROFF_METHOD(CMarkup, ClearSegment, clearsegment, ( int iSegmentIndex, BOOL fInvalidate ))
    TEAROFF_METHOD(CMarkup, ClearSegments, clearsegments, ( BOOL fInvalidate ))
    TEAROFF_METHOD(CMarkup, ClearElementSegments, clearelementsegments, ())
END_TEAROFF_TABLE()

const CBase::CLASSDESC CMarkup::s_classdesc =
{
    &CLSID_HTMLDocument,            // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    CDoc::s_acpi,                   // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLDocument2,            // _piidDispinterface
    &CDoc::s_apHdlDescs,            // _apHdlDesc
};

#if TREE_SYNC
// this is a big HACK, to temporarily get markup-sync working for netdocs.  it
// will totally change in the future.  don't want to expose this in iedev, so
// leaving it hacked in like this for now.
extern const IID IID_ITreeSyncServices = {0x8860B601,0x178E,0x11d2,0x96,0xAE,0x00,0x80,0x5F,0x85,0x2B,0x4D};
#endif // TREE_SYNC

extern "C" const GUID SID_SXmlNamespaceMapping;

#if DBG==1
    CMarkup *    g_pDebugMarkup = NULL;        // Used in tree dump overwrite cases to see real-time tree changes
#endif

DWORD CMarkup::s_dwDirtyRangeServiceCookiePool = 0x0;

//+---------------------------------------------------------------------------
//
//  Member:     CMarkupScriptContext constructor
//
//----------------------------------------------------------------------------

CMarkupScriptContext::CMarkupScriptContext()
{
    _dwScriptCookie             = NO_SOURCE_CONTEXT;
    _dwScriptDownloadingCookie  = 1;
    _idxDefaultScriptHolder     = -1;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkupScriptContext destructor
//
//----------------------------------------------------------------------------

CMarkupScriptContext::~CMarkupScriptContext()
{
    if (_pScriptDebugDocument)
    {
        _pScriptDebugDocument->Release();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup constructor
//
//----------------------------------------------------------------------------

CMarkup::CMarkup ( CDoc *pDoc, CElement * pElementMaster )
#if DBG == 1 || defined(DUMPTREE)
    : _nSerialNumber( CTreeNode::s_NextSerialNumber++ )
#endif
{
    __lMarkupTreeVersion = 1;
    __lMarkupContentsVersion = 1;

    pDoc->SubAddRef();

    _pDoc = pDoc;

    Assert( _pDoc && _pDoc->AreLookasidesClear( this, LOOKASIDE_MARKUP_NUMBER ) );

    _pElementMaster = pElementMaster;

    _LoadStatus = LOADSTATUS_UNINITIALIZED;

    if (_pElementMaster)
        _fIncrementalAlloc = TRUE;

    if (pDoc->IsPrintDoc())
        _fXML = DYNCAST(CPrintDoc, pDoc->GetRootDoc())->_fXML;
    //_fEnableDownload = TRUE;

    WHEN_DBG( ZeroMemory(_apLookAside, sizeof(_apLookAside)); )
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup destructor
//
//----------------------------------------------------------------------------

CMarkup::~CMarkup()
{
    ClearLookasidePtrs();

    //
    // Destroy stylesheets collection subobject (if anyone held refs on it, we
    // should never have gotten here since the doc should then have subrefs
    // keeping it alive).  This is the only place where we directly access the
    // CBase impl. of IUnk. for the CStyleSheetArray -- we do this instead of
    // just calling delete in order to assure that the CBase part of the CSSA
    // is properly destroyed (CBase::Passivate gets called etc.)
    //

    if (HasStyleSheetArray())
    {
        CStyleSheetArray * pStyleSheets = GetStyleSheetArray();

        pStyleSheets->CBase::PrivateRelease();
        DelLookasidePtr(LOOKASIDE_STYLESHEETS);
    }

    _pDoc->SubRelease();

    // BUGBUG: we should be able to assert this but we keep seeing it
    // in a really strange stress case.  Revisit later.
    // Assert(!_aryANotification.Size());
}

//+---------------------------------------------------------------------------
//
//  Members:    Debug only version number incrementors
//
//----------------------------------------------------------------------------

#if DBG == 1

void
CMarkup::UpdateMarkupTreeVersion ( )
{
    AssertSz(!__fDbgLockTree, "Tree was modified when it should not have been (e.g., inside ENTERTREE notification)");
    __lMarkupTreeVersion++;
    Doc()->UpdateDocTreeVersion();
    UpdateMarkupContentsVersion();
}

void
CMarkup::UpdateMarkupContentsVersion ( )
{
    AssertSz(!__fDbgLockTree, "Tree was modified when it should not have been (e.g., inside ENTERTREE notification)");
    __lMarkupContentsVersion++;
    Doc()->UpdateDocContentsVersion();
}

#endif

//+---------------------------------------------------------------------------
//
//  Member:     ClearLookasidePtrs
//
//----------------------------------------------------------------------------

void
CMarkup::ClearLookasidePtrs ( )
{
    delete DelCollectionCache();
    Assert(_pDoc->GetLookasidePtr((DWORD *) this + LOOKASIDE_COLLECTIONCACHE) == NULL);

#if MARKUP_DIRTYRANGE
    Assert( !HasDirtyRangeContext() );
    Assert( !_fOnDirtyRangeChangePosted );
#endif

    CMarkup *pParentMarkup = (CMarkup *)DelParentMarkup();
    
    if (pParentMarkup)
        pParentMarkup->Release();

    Assert(_pDoc->GetLookasidePtr((DWORD *) this + LOOKASIDE_PARENTMARKUP) == NULL);

    Assert (!HasBehaviorContext());
    Assert (!HasScriptContext());
    Assert (!HasTextRangeListPtr());
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::Init
//
//----------------------------------------------------------------------------

HRESULT 
CMarkup::Init( CRootElement * pElementRoot )
{
    HRESULT     hr;

    Assert( pElementRoot );

    _pElementRoot = pElementRoot;

    _tpRoot.MarkLast();
    _tpRoot.MarkRight();

    WHEN_DBG( _cchTotalDbg = 0 );
    WHEN_DBG( _cElementsTotalDbg = 0 );

    hr = CreateInitialMarkup( _pElementRoot );
    if (hr)
        goto Cleanup;

    hr = THR(super::Init());
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::UnloadContents
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::UnloadContents( BOOL fForPassivate /*= FALSE*/)
{
    HRESULT hr = S_OK;
    CElement::CLock lock( _pElementRoot );

    //
    // text and tree
    // 

    delete _pSelRenSvcProvider;
    _pSelRenSvcProvider = NULL;

    _TxtArray.RemoveAll();

    hr = DestroySplayTree( !fForPassivate );
    if (hr)
        goto Cleanup;

    //
    // Restore initial state
    //

    _fIncrementalAlloc = ! ! _pElementMaster;

    _fNoUndoInfo = FALSE;
    _fLoaded = FALSE;
    _fStreaming = FALSE;
    //_fEnableDownload = TRUE;

    //
    // behaviors
    //

    delete DelBehaviorContext();

    //
    // script
    //

    Assert( !ScriptContext() ||
           ( ScriptContext() && 0 == ScriptContext()->_aryScriptEnqueued.Size()));

    if (HasScriptContext())
    {
        CMarkupScriptContext *  pScriptContext = ScriptContext();

        if (pScriptContext->_fWaitScript)
        {
            _pDoc->UnregisterMarkupForInPlace(this);
        }

        if (_pDoc->_pScriptCookieTable && NO_SOURCE_CONTEXT != pScriptContext->_dwScriptCookie)
        {
            IGNORE_HR(_pDoc->_pScriptCookieTable->RevokeSourceObject(pScriptContext->_dwScriptCookie, this));
        }
    }

    delete DelScriptContext();

    //
    // Dirty range service
    //
    
#if MARKUP_DIRTYRANGE

    delete DelDirtyRangeContext();

    if (_fOnDirtyRangeChangePosted)
    {
        GWKillMethodCall(this, ONCALL_METHOD(CMarkup, OnDirtyRangeChange, ondirtyrangechange), 0);
        _fOnDirtyRangeChangePosted = FALSE;
    }
#endif

    //
    // loading
    //

    _LoadStatus = LOADSTATUS_UNINITIALIZED;

    if (_pHtmCtx)
        _pHtmCtx->Release();

    if (_pProgSink)
    {
        _pProgSink->Detach();
        _pProgSink->Release();
        _pProgSink = NULL;
    }

    if (HasStyleSheetArray())
    {
        CStyleSheetArray * pStyleSheets = GetStyleSheetArray();
        pStyleSheets->Free( );  // Force our stylesheets collection to release its
                                    // refs on stylesheets/rules.  We will rel in passivate,
                                    // delete in destructor.
    }

Cleanup:

    // BUGBUG: we should be able to assert this but we keep seeing it
    // in a really strange stress case.  Revisit later.
    // Assert(!_aryANotification.Size());
    
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::Passivate
//
//----------------------------------------------------------------------------

void
CMarkup::Passivate()
{
    //
    // Release everything
    //
    
    IGNORE_HR( UnloadContents( TRUE ) );

    // Release stylesheets subobj, which will subrel us
    if ( HasStyleSheetArray() )
    {
        CStyleSheetArray * pStyleSheets = GetStyleSheetArray();
        pStyleSheets->Release();
        // we will delete in destructor
    }

    //
    // super
    //

    super::Passivate();
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::DoEmbedPointers
//
//  Synopsis:   Puts unembedded pointers into the splay tree.
//
//----------------------------------------------------------------------------

#if DBG!=1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif

HRESULT
CMarkup::DoEmbedPointers ( )
{
    HRESULT hr = S_OK;
    
    //
    // This should only be called from EmbedPointers when there are any
    // outstanding
    //
    
#if DBG == 1
    
    //
    // Make sure all unembedded pointers are correct
    //
    
    {
        for ( CMarkupPointer * pmp = _pmpFirst ; pmp ; pmp = pmp->_pmpNext )
        {
            Assert( pmp->Markup() == this );
            Assert( ! pmp->_fEmbedded );
            Assert( pmp->_ptpRef );
            Assert( pmp->_ptpRef->GetMarkup() == this );
            Assert( ! pmp->_ptpRef->IsPointer() );

            if (pmp->_ptpRef->IsText())
                Assert( pmp->_ichRef <= pmp->_ptpRef->Cch() );
            else
                Assert( pmp->_ichRef == 0 );
        }
    }
#endif

    while ( _pmpFirst )
    {
        CMarkupPointer * pmp;
        CTreePos *       ptpNew;

        //
        // Remove the first pointer from the list
        //
        
        pmp = _pmpFirst;
        
        _pmpFirst = pmp->_pmpNext;
        
        if (_pmpFirst)
            _pmpFirst->_pmpPrev = NULL;
        
        pmp->_pmpNext = pmp->_pmpPrev = NULL;

        Assert( pmp->_ichRef == 0 || pmp->_ptpRef->IsText() );

        //
        // Consider the case where two markup pointers point in to the the
        // middle of the same text pos, where the one with the larger ich
        // occurs later in the list of unembedded pointers.  When the first
        // is encountered, it will split the text pos, leaving the second
        // with an invalid ich.
        //
        // Here we check to see if the ich is within the text pos.  If it is
        // not, then the embedding of a previous pointer must have split the
        // text pos.  In this case, we scan forward to locate the right hand
        // side of that split, and re-adjust this pointer!
        //

        if (pmp->_ptpRef->IsText() && pmp->_ichRef > pmp->_ptpRef->Cch())
        {
            //
            // If we are out of range, then there better very well be a pointer
            // next which did it.
            //
            
            Assert( pmp->_ptpRef->NextTreePos()->IsPointer() );

            while ( pmp->_ichRef > pmp->_ptpRef->Cch() )
            {
                Assert( pmp->_ptpRef->IsText() && pmp->_ptpRef->Cch() > 0 );
            
                pmp->_ichRef -= pmp->_ptpRef->Cch();

                do
                    { pmp->_ptpRef = pmp->_ptpRef->NextTreePos(); }
                while ( ! pmp->_ptpRef->IsText() );
            }
        }
        
        //
        // See if we have to split a text pos
        //

        if (pmp->_ptpRef->IsText() && pmp->_ichRef < pmp->_ptpRef->Cch())
        {
            Assert( pmp->_ichRef != 0 );
            
            hr = THR( Split( pmp->_ptpRef, pmp->_ichRef ) );

            if (hr)
                goto Cleanup;
        }
        
        ptpNew = NewPointerPos( pmp, pmp->Gravity(), pmp->Cling() );

        if (!ptpNew)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        //
        // We should always be at the end of a text pos.
        //

        Assert( ! pmp->_ptpRef->IsText() || pmp->_ichRef == pmp->_ptpRef->Cch() );

        hr = THR( Insert( ptpNew, pmp->_ptpRef, FALSE ) );

        if (hr)
            goto Cleanup;

        pmp->_fEmbedded = TRUE;
        pmp->_ptpEmbeddedPointer = ptpNew;
        pmp->_ichRef = 0;
    }

Cleanup:

    RRETURN( hr );
}

#if DBG!=1
#pragma optimize("", on)
#endif


//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::GetDD
//
//  Synopsis:   returns Default Dispatch object for this markup
//
//----------------------------------------------------------------------------

CBase *
CMarkup::GetDD()
{
    CMarkupBehaviorContext *    pBehaviorContext = BehaviorContext();
    return (pBehaviorContext && pBehaviorContext->_pHtmlComponent) ?
                  (CBase*)&pBehaviorContext->_pHtmlComponent->_DD :
                  (CBase*)&_OmDoc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::EnsureScriptContext
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::EnsureScriptContext(CMarkupScriptContext ** ppScriptContext, LPTSTR pchUrl)
{
    HRESULT                 hr;
    CMarkupScriptContext *  pScriptContext;

    if (!HasScriptContext())
    {
        //
        // create the context
        //

        pScriptContext = new CMarkupScriptContext();
        if (!pScriptContext)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(SetScriptContext(pScriptContext));
        if (hr)
            goto Cleanup;

        //
        // init namespace
        //

        if (IsPrimaryMarkup())
        {
            hr = THR(pScriptContext->_cstrNamespace.Set(DEFAULT_OM_SCOPE));

            // don't do AddNamedItem for PrimaryMarkup: this is done once when we create script engines.
            // we don't want to be re-registering DEFAULT_OM_SCOPE namespace each time in refresh
        }
        else
        {
            hr = THR(_pDoc->GetUniqueIdentifier(&pScriptContext->_cstrNamespace));
            if (hr)
                goto Cleanup;

            // BUGBUG (alexz) !_pDoc->_fMarkupServicesParsing: this should be an assert:
            // we should not even create script context for auxillary markups
            if (_pDoc->_pScriptCollection && !_pDoc->_fMarkupServicesParsing)
                IGNORE_HR(_pDoc->_pScriptCollection->AddNamedItem(
                      pScriptContext->_cstrNamespace, (IUnknown*)(IPrivateUnknown*)GetDD()));
        }

        //
        // misc
        //

        if (pchUrl)
        {
            hr = THR(pScriptContext->_cstrUrl.Set(pchUrl));
            if (hr)
                goto Cleanup;

            {
                CScriptDebugDocument::CCreateInfo   createInfo(_pDoc, pchUrl, HtmCtx());

                hr = THR(CScriptDebugDocument::Create(&createInfo, &pScriptContext->_pScriptDebugDocument));
                if (hr)
                    goto Cleanup;
            }
        }

        if (ppScriptContext)
        {
            *ppScriptContext = pScriptContext;
        }
    }
    else
    {
        hr = S_OK;

        if (ppScriptContext)
        {
            *ppScriptContext = ScriptContext();
        }
    }

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::PrivateQueryInterface
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    HRESULT      hr = S_OK;
    const void * apfn = NULL;
    void *       pv = NULL;
    void *       appropdescsInVtblOrder = NULL;
    const IID * const * apIID = NULL;
    extern const IID * const g_apIID_IDispatchEx[];
    BOOL fPrimaryMarkup = IsPrimaryMarkup();

    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF(this, IMarkupContainer, NULL)
        QI_TEAROFF(this, ISelectionRenderingServices, NULL)
        QI_TEAROFF2(this, ISegmentList, ISelectionRenderingServices, NULL)
        QI_TEAROFF(this, IMarkupTextFrags, NULL)
        
    default:
        if (IsEqualIID(iid, CLSID_CMarkup))
        {
            *ppv = this;
            return S_OK;
        }
        else if (iid == IID_IProvideMultipleClassInfo ||
                 iid == IID_IProvideClassInfo ||
                 iid == IID_IProvideClassInfo2)
        {
            pv = Doc();
            apfn = CDoc::s_apfnIProvideMultipleClassInfo;
        }
        else if (iid == IID_IDispatchEx || iid == IID_IDispatch)
        {
            apIID = g_apIID_IDispatchEx;
            if (fPrimaryMarkup)
            {
                pv = (IDispatchEx *)Doc();
                apfn = *(void **)pv;
            }
            else
            {
                pv = (void *)this;
                apfn = (const void *)s_apfnIDispatchEx;
            }
        }
        else if (iid == IID_IHTMLDocument || iid == IID_IHTMLDocument2)
        {
            if (fPrimaryMarkup)
            {
                pv = (IHTMLDocument2 *)Doc();
                apfn = *(void **)pv;
            }
            else
            {
                pv = (void *)this;
                apfn = (const void *)s_apfnpdIHTMLDocument2;
                appropdescsInVtblOrder = (void *)s_ppropdescsInVtblOrderIHTMLDocument2; 
            }
        }
        else if (iid == IID_IHTMLDocument3)
        {
            if (fPrimaryMarkup)
            {
                pv = (IHTMLDocument3 *)Doc();
                apfn = *(void **)pv;
            }
            else
            {
                pv = (void *)this;
                apfn = (const void *)s_apfnpdIHTMLDocument3;
                appropdescsInVtblOrder = (void *)s_ppropdescsInVtblOrderIHTMLDocument3; 
            }
        }
        else if (iid == IID_IConnectionPointContainer)
        {
            *((IConnectionPointContainer **)ppv) = new CConnectionPointContainer(
                fPrimaryMarkup ? Doc() : (CBase *)this, (IUnknown *)(IPrivateUnknown *)this);
            if (!*ppv)
                RRETURN(E_OUTOFMEMORY);
        }
        else if (iid == IID_IMarkupServices)
        {
            pv = Doc();
            apfn = CDoc::s_apfnIMarkupServices;
        }
        else if (iid == IID_IHTMLViewServices)
        {
            pv = Doc();
            apfn = CDoc::s_apfnIHTMLViewServices;
        }
#if DBG ==1
        else if (iid == IID_IEditDebugServices)
        {
            pv = Doc();
            apfn = CDoc::s_apfnIEditDebugServices;
        }    
#endif
        else if (iid == IID_IServiceProvider )
        {
            pv = Doc();
            apfn = CDoc::s_apfnIServiceProvider;
        }
        else if (iid == IID_IOleWindow)
        {
            pv = Doc();
            apfn = CDoc::s_apfnIOleInPlaceObjectWindowless;
        }
        else if (iid == IID_IOleCommandTarget)
        {
            pv = Doc();
            apfn = CDoc::s_apfnIOleCommandTarget;
        }
#if TREE_SYNC
        // this is a big HACK, to temporarily get markup-sync working for netdocs.  it
        // will totally change in the future.
        // trident team: do not bother trying to maintain this code, delete if it causes problems
        else if (iid == IID_IMarkupSyncServices)
        {
            pv = Doc();
            apfn = CDoc::s_apfnIMarkupSyncServices;
        }
#endif // TREE_SYNC
        else
        {
            RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
        }

        if (pv)
        {
            Assert(apfn);
            hr = THR(CreateTearOffThunk(
                    pv, 
                    apfn, 
                    NULL, 
                    ppv, 
                    (IUnknown *)(IPrivateUnknown *)this, 
                    *(void **)(IUnknown *)(IPrivateUnknown *)this,
                    QI_MASK | ADDREF_MASK | RELEASE_MASK,
                    apIID,
                    appropdescsInVtblOrder));
            if (hr)
                RRETURN(hr);
        }
    }

    // any unknown interface will be handled by the default above
    Assert(*ppv);

    (*(IUnknown **)ppv)->AddRef();

    DbgTrackItf(iid, "CMarkup", FALSE, ppv);

    return S_OK;

}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::PrivateQueryInterface
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::QueryService(REFGUID rguidService, REFIID riid, void ** ppvObject)
{
    HRESULT     hr;

    if (IsEqualGUID(rguidService, SID_SXmlNamespaceMapping))
    {
        CXmlNamespaceTable *    pXmlNamespaceTable;

        hr = THR(EnsureXmlNamespaceTable(&pXmlNamespaceTable));
        if (hr)
            goto Cleanup;

        hr = THR(pXmlNamespaceTable->QueryInterface(riid, ppvObject));

        goto Cleanup; // done
    }

    hr = THR_NOTRACE(_pDoc->QueryService(rguidService, riid, ppvObject));

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::Load
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::Load (IStream * pStream, HTMPASTEINFO * phtmpasteinfo)
{
    HRESULT         hr;
    HTMLOADINFO     htmloadinfo;

    htmloadinfo.pDoc            = _pDoc;
    htmloadinfo.pMarkup         = this;
    htmloadinfo.pDwnDoc         = _pDoc->_pDwnDoc;
    htmloadinfo.pVersions       = _pDoc->_pVersions;
    htmloadinfo.pchUrl          = _pDoc->_cstrUrl;
    htmloadinfo.fParseSync      = TRUE;

    htmloadinfo.phpi            = phtmpasteinfo;
    htmloadinfo.pstm            = pStream;

    hr = THR(Load(&htmloadinfo));

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::Load
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::Load (IMoniker * pMoniker, IBindCtx * pBindCtx)
{
    HRESULT         hr;
    HTMLOADINFO     htmloadinfo;
    LPTSTR          pchUrl = NULL;

    hr = THR(pMoniker->GetDisplayName(pBindCtx, NULL, &pchUrl));
    if (hr)
        goto Cleanup;

    htmloadinfo.pDoc            = _pDoc;
    htmloadinfo.pMarkup         = this;
    htmloadinfo.pDwnDoc         = _pDoc->_pDwnDoc;
    htmloadinfo.pInetSess       = TlsGetInternetSession();
    htmloadinfo.pmk             = pMoniker;
    htmloadinfo.pbc             = pBindCtx;
    htmloadinfo.pchUrl          = pchUrl;

    hr = THR(Load(&htmloadinfo));

Cleanup:
    CoTaskMemFree(pchUrl);

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::Load
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::Load (HTMLOADINFO * phtmloadinfo)
{
    HRESULT         hr;

    //
    // create _pHtmCtx
    //

    hr = THR(NewDwnCtx(DWNCTX_HTM, FALSE, phtmloadinfo, (CDwnCtx **)&_pHtmCtx));
    if (hr)
        goto Cleanup;

    //
    // create _pProgSink if necessary:
    //
    // 1. Primary markup always needs it
    // 2. It is always necessary during async download
    //

    if (IsPrimaryMarkup() || !phtmloadinfo->fParseSync)
    {
        _pProgSink = new CProgSink(_pDoc, this);
        if (!_pProgSink)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(_pHtmCtx->SetProgSink(_pProgSink));
        if (hr)
            goto Cleanup;
    }

    //
    // set up Url
    //

    if (phtmloadinfo->pchUrl)
    {
        hr = THR(EnsureScriptContext(NULL, (LPTSTR)phtmloadinfo->pchUrl));
        if (hr)
            goto Cleanup;
    }

    //
    // launch download
    //

    _pHtmCtx->SetLoad(TRUE, phtmloadinfo, FALSE);
    hr = (_pHtmCtx->GetState() & DWNLOAD_ERROR) ? E_FAIL : S_OK;


Cleanup:

    //
    // if this is sync download, and it is not primary markup, then we don't need _pHtmCtx
    //
    if (!IsPrimaryMarkup() && phtmloadinfo->fParseSync && _pHtmCtx)
    {
        _pHtmCtx->Release();
        _pHtmCtx = NULL;
    }
    

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::StopDownload
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::StopDownload()
{
    if (_pHtmCtx)
    {
        _pHtmCtx->SetLoad(FALSE, NULL, FALSE);
        _pHtmCtx->Release();
        _pHtmCtx = NULL;
    }
    RRETURN (S_OK);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::LoadStatus
//
//----------------------------------------------------------------------------

LOADSTATUS
CMarkup::LoadStatus()
{
    return _LoadStatus;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::OnLoadStatus
//
//----------------------------------------------------------------------------

void
CMarkup::OnLoadStatus(LOADSTATUS LoadStatus)
{
    // NOTE (lmollico): we need to addref CMarkup because CDoc::OnLoadStatus can cause 
    // this (CMarkup) to be destroyed if the doc is unloaded
    AddRef();

    while (_LoadStatus < LoadStatus)
    {
        _LoadStatus = (LOADSTATUS)(_LoadStatus + 1);

        //
        // process status change ourselves BEFORE the doc
        //

        switch (_LoadStatus)
        {
        case LOADSTATUS_QUICK_DONE:

            if (ScriptContext())
            {
                ScriptContext()->_fWaitScript = FALSE;
            }

            break;
        }

        //
        // send to doc if we are the primary markup
        //

        if (IsPrimaryMarkup())
        {
            _pDoc->OnLoadStatus (_LoadStatus);
            if (!IsPrimaryMarkup())
                goto Cleanup;       // abort if the doc has been unloaded
        }

        //
        // process status change ourselves AFTER the doc
        //

        switch (_LoadStatus)
        {
        case LOADSTATUS_QUICK_DONE:

            // CONSIDER (alexz) _fPeerPossible can be on per tree basis
            if (_pDoc->_fPeersPossible)
            {
                CNotification   nf;

                nf.DocEndParse(Root());
                _pDoc->BroadcastNotify(&nf);
            }

            break;

        case LOADSTATUS_DONE:

            // release _pHtmCtx if it is no longer needed.
            // cases when _pHtmCtx is necessary are listed below

            if (_pHtmCtx &&
                !IsPrimaryMarkup() &&                                               // primary markup
                !(HasScriptContext() && ScriptContext()->_pScriptDebugDocument))    // markups with script debug document
            {
                _pHtmCtx->Release();
                _pHtmCtx = NULL;
            }

            break;
        }

        //
        // send to html component, if any
        //

        if (BehaviorContext() && BehaviorContext()->_pHtmlComponent)
        {
            BehaviorContext()->_pHtmlComponent->OnLoadStatus(_LoadStatus);
        }

    } // eo while (_LoadStatus < LoadStatus)

Cleanup:
    Release();
}

//+-------------------------------------------------------------------
//
//  Member:     EnsureTitle
//
//--------------------------------------------------------------------

HRESULT
CMarkup::EnsureTitle ( )
{
    HRESULT hr = S_OK;
    CElement *  pElementTitle = NULL;

    if (!GetTitleElement())
    {
        if (!GetHeadElement())
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = THR( CreateElement( ETAG_TITLE_ELEMENT, & pElementTitle ) );

        if (hr)
            goto Cleanup;
        
        // BUGBUG: watch how this might interfere with the parser!
        hr = THR( AddHeadElement( pElementTitle ) );

        if (hr)
            goto Cleanup;
    }
    
Cleanup:

    CElement::ClearPtr( & pElementTitle );
    
    RRETURN( hr );
}

//+-------------------------------------------------------------------
//
//  Member:     CRootSite::AddHeadElement
//
//  Synopsis:   Stores and addrefs element in HEAD.
//              Also picks out first TITLE element...
//
//--------------------------------------------------------------------

HRESULT
CMarkup::AddHeadElement ( CElement * pElement, long lIndex )
{
    HRESULT hr = S_OK;

    Assert( GetHeadElement() );

    if (lIndex >= 0)
    {
        CTreeNode * pNodeIter;
        CChildIterator ci ( GetHeadElement(), NULL, CHILDITERATOR_DEEP );

        while ( (pNodeIter = ci.NextChild()) != NULL )
        {
            if (lIndex-- == 0)
            {
                hr = THR(
                    pNodeIter->Element()->InsertAdjacent(
                        CElement::BeforeBegin, pElement ) );

                if (hr)
                    goto Cleanup;

                goto Cleanup;
            }
        }
    }

    // If the parser is still in the head while we are trying
    // to insert an element in the head, make sure we don't insert it in
    // front of the parser's frontier.
    
    if (_pRootParseCtx)
    {
        CTreePosGap tpgFrontier;

        if (_pRootParseCtx->SetGapToFrontier( & tpgFrontier ) 
            && tpgFrontier.Branch()->SearchBranchToRootForScope( GetHeadElement() ))
        {
            CMarkupPointer mp ( Doc() );

            hr = THR( mp.MoveToGap( & tpgFrontier, this ) );

            if (hr)
                goto Cleanup;

            hr = THR( Doc()->InsertElement( pElement, & mp, NULL ) );

            if (hr)
                goto Cleanup;

            goto Cleanup;
        }
    }

    hr = THR(
        GetHeadElement()->InsertAdjacent(
            CElement::BeforeEnd, pElement ) );

    if (hr)
        goto Cleanup;
    
Cleanup:

    RRETURN( hr );
}

#ifdef XMV_PARSE
//+---------------------------------------------------------------
//
//  Member:     CMarkup::SetXML
//
//  Synopsis:   Treats unqualified tags as xtags.
//
//---------------------------------------------------------------
void
CMarkup::SetXML(BOOL flag)
{
    // pass down to the parser if it is live    
    _fXML = flag ? 1 : 0;
    if (_pHtmCtx)
        _pHtmCtx->SetGenericParse(_fXML);
}
#endif


//+---------------------------------------------------------------
//
//  Member:     CMarkup::PasteClipboard
//
//  Synopsis:   Manage paste from clipboard
//
//---------------------------------------------------------------

HRESULT
CMarkup::PasteClipboard()
{
    HRESULT          hr;
    IDataObject    * pDataObj   = NULL;
    CCurs            curs(IDC_WAIT);
    CLock            Lock(this);
    
    hr = THR(OleGetClipboard(&pDataObj));
    if (hr)
        goto Cleanup;

    hr = THR(Doc()->AllowPaste(pDataObj));
    if (hr)
        goto Cleanup;


Cleanup:
    ReleaseInterface( pDataObj );

    if (hr)
    {
        ClearErrorInfo();
        PutErrorInfoText(EPART_ACTION, IDS_EA_PASTE_CONTROL);
        CloseErrorInfo(hr);
    }

    RRETURN(hr);
}

#ifdef UNIX_NOTYET
/*
 *  CMarkup::PasteUnixQuickTextToRange
 *
 *  @mfunc  Freeze display and paste object
 *
 *  @rdesc  HRESULT from IDataTransferEngine::PasteMotifTextToRange
 *
 */

HRESULT 
CMarkup::PasteUnixQuickTextToRange(
    CTxtRange *prg,
    VARIANTARG *pvarTextHandle
    )
{
    HRESULT      hr;

    {
        CLightDTEngine ldte( this );

        hr = THR(ldte.PasteUnixQuickTextToRange(prg, pvarTextHandle));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}
#endif // UNIX

void 
CMarkup::SetModified ( void )
{
    _fModified = TRUE;
    // Tell the form
    Doc()->OnDataChange();
}

//+-------------------------------------------------------------------------
//
//  Method:     createTextRange
//
//  Synopsis:   Get an automation range for the entire document
//
//--------------------------------------------------------------------------

HRESULT
CMarkup::createTextRange( IHTMLTxtRange * * ppDisp, CElement * pElemContainer )
{
    HRESULT hr = S_OK;

    hr = THR(createTextRange(ppDisp, pElemContainer, NULL, NULL, TRUE));

    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     createTextRange
//
//  Synopsis:   Get an automation range for the entire document
//
//--------------------------------------------------------------------------

HRESULT
CMarkup::createTextRange( 
    IHTMLTxtRange * * ppDisp, 
    CElement * pElemContainer, 
    IMarkupPointer *pLeft, 
    IMarkupPointer *pRight,
    BOOL fAdjustPointers)
{
    HRESULT hr = S_OK;
    CAutoRange * pAutoRange = NULL;

    if (!ppDisp)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pAutoRange = new CAutoRange( this, pElemContainer );

    if (!pAutoRange)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Set the lookaside entry for this range
    pAutoRange->SetNext( DelTextRangeListPtr() );
    hr = THR( SetTextRangeListPtr( pAutoRange ) );
    if (hr)
        goto Cleanup;

    hr = THR( pAutoRange->Init() );
    if (hr)
        goto Cleanup;

    if (pLeft && pRight)
    {
        hr = THR_NOTRACE( pAutoRange->SetLeftAndRight(pLeft, pRight, fAdjustPointers) );
    }
    else
    {
        hr = THR_NOTRACE( pAutoRange->SetTextRangeToElement( pElemContainer ) );
    }

    Assert( hr != S_FALSE );

    if (hr)
        goto Cleanup;

    *ppDisp = pAutoRange;
    pAutoRange->AddRef();
  
Cleanup:
    if (pAutoRange)
    {
        pAutoRange->Release();
    }
    RRETURN( SetErrorInfo( hr ) );
}




//+-------------------------------------------------------------------------
//
//  Method:     CMarkup::AcceptingUndo
//
//  Synopsis:   Tell the PedUndo if we're taking any changes.
//
//--------------------------------------------------------------------------

BOOL
CMarkup::AcceptingUndo()
{
    return      !_fNoUndoInfo;
//            &&  IsEditable()
}

//+-------------------------------------------------------------------------
//
// IMarkupContainer Method Implementations
//
//--------------------------------------------------------------------------

HRESULT
CMarkup::OwningDoc( IHTMLDocument2 * * ppDoc )
{
    return _pDoc->_pPrimaryMarkup->QueryInterface( IID_IHTMLDocument2, (void **) ppDoc );
}

HRESULT 
CMarkup::AddSegment( 
    IMarkupPointer* pIStart, 
    IMarkupPointer* pIEnd,
    HIGHLIGHT_TYPE HighlightType,
    int * piSegmentIndex )  
{
    HRESULT hr  = EnsureSelRenSvc();

    if (OK(hr))
    {
        hr = THR( _pSelRenSvcProvider->AddSegment( pIStart, pIEnd, HighlightType, piSegmentIndex ) );
    }

    RRETURN ( hr );            
}


HRESULT 
CMarkup::AddElementSegment( 
    IHTMLElement* pIElement ,
    int * piSegmentIndex )  
{
    HRESULT hr = EnsureSelRenSvc();
        
    if ( !hr)
        hr =_pSelRenSvcProvider->AddElementSegment( pIElement, piSegmentIndex);        

    RRETURN ( hr );        
}

HRESULT 
CMarkup::MovePointersToSegment( 
    int iSegmentIndex, 
    IMarkupPointer* pIStart, 
    IMarkupPointer* pIEnd ) 
{
    HRESULT hr = EnsureSelRenSvc();

    if ( !hr)
        hr = _pSelRenSvcProvider->MovePointersToSegment( iSegmentIndex, pIStart, pIEnd );

    RRETURN ( hr );            
}    

HRESULT 
CMarkup::GetElementSegment( 
    int iSegmentIndex, 
    IHTMLElement** ppIElement) 
{
    HRESULT hr = EnsureSelRenSvc();

    if ( !hr )
        hr =  _pSelRenSvcProvider->GetElementSegment( iSegmentIndex, ppIElement );

    RRETURN ( hr );            
}    
    
HRESULT 
CMarkup::MoveSegmentToPointers( 
    int iSegmentIndex,
    IMarkupPointer* pIStart, 
    IMarkupPointer* pIEnd,
    HIGHLIGHT_TYPE HighlightType )  
{
    HRESULT hr = EnsureSelRenSvc();

    if ( !hr )
        hr = _pSelRenSvcProvider->MoveSegmentToPointers(iSegmentIndex, pIStart, pIEnd, HighlightType ) ;

    RRETURN ( hr );        
}    
    
HRESULT 
CMarkup::SetElementSegment( 
    int iSegmentIndex,
    IHTMLElement* pIElement)  
{
    HRESULT hr = EnsureSelRenSvc();

    if ( ! hr )
        hr =  _pSelRenSvcProvider->SetElementSegment( iSegmentIndex, pIElement );

    RRETURN ( hr );        
}    

HRESULT 
CMarkup::ClearSegment( int iSegmentIndex, BOOL fInvalidate )
{
    if ( _pSelRenSvcProvider)
        RRETURN ( _pSelRenSvcProvider->ClearSegment(iSegmentIndex, fInvalidate));
    else
        return S_OK;
}

HRESULT 
CMarkup::ClearSegments(BOOL fInvalidate)
{
    if ( _pSelRenSvcProvider)
        RRETURN ( _pSelRenSvcProvider->ClearSegments(fInvalidate));
    else
        return S_OK;
}

HRESULT
CMarkup::ClearElementSegments()
{
    if ( _pSelRenSvcProvider )
        RRETURN ( _pSelRenSvcProvider->ClearElementSegments());
#ifdef NOT_YET    
        RRETURN ( _pSelRenSvcProvider->ClearElementSegments(TRUE));
#endif        
    else
        return S_OK;
}


HRESULT
CMarkup::GetSegmentCount(
    int* piSegmentCount,
    SELECTION_TYPE * peSegmentType )
{
    HRESULT hr = EnsureSelRenSvc();
    if ( ! hr )
        hr = _pSelRenSvcProvider->GetSegmentCount( piSegmentCount, peSegmentType ) ;

    RRETURN( hr );
}

HRESULT
CMarkup::EnsureSelRenSvc()
{
    if ( ! _pSelRenSvcProvider )
#if DBG == 1
        _pSelRenSvcProvider = new CSelectionRenderingServiceProvider( Doc() , this   );
#else
        _pSelRenSvcProvider = new CSelectionRenderingServiceProvider( Doc() );
#endif
    if ( ! _pSelRenSvcProvider )
        return E_OUTOFMEMORY;
    return S_OK;        
}

//+==============================================================================
// 
// Method: GetSelectionChunksForLayout
//
// Synopsis: Get the 'chunks" for a given Flow Layout, as well as the Max and Min Cp's of the chunks
//              
//            A 'chunk' is a part of a SelectionSegment, broken by FlowLayout
//
//-------------------------------------------------------------------------------

VOID
CMarkup::GetSelectionChunksForLayout( 
    CFlowLayout* pFlowLayout, CPtrAry<HighlightSegment*> *paryHighlight, 
    int* piCpMin, 
    int * piCpMax )
{
    if ( _pSelRenSvcProvider )
    {
           _pSelRenSvcProvider->GetSelectionChunksForLayout( pFlowLayout, paryHighlight, piCpMin, piCpMax );
    }
    else
    {
        Assert( piCpMax );
        *piCpMax = -1;
    }    
}

//+====================================================================================
//
// Method: GetFlattenedSelection
//
// Synopsis: Get a "flattened" selection ( for undo)
//
//------------------------------------------------------------------------------------


HRESULT
CMarkup::GetFlattenedSelection( 
                                int iSegmentIndex, 
                                int & cpStart, 
                                int & cpEnd, 
                                SELECTION_TYPE&  eType )
{
    Assert ( _pSelRenSvcProvider );
    if ( _pSelRenSvcProvider )
        RRETURN( _pSelRenSvcProvider->GetFlattenedSelection( iSegmentIndex, cpStart, cpEnd, eType));
    else
    {
        cpStart = -1;
        cpEnd = -1;
        eType = SELECTION_TYPE_None;
        return S_OK;
    }
}

VOID
CMarkup::HideSelection()
{
    if ( _pSelRenSvcProvider )
    {
        _pSelRenSvcProvider->HideSelection();
    }
}

VOID
CMarkup::ShowSelection()
{
    if ( _pSelRenSvcProvider )
    {
        _pSelRenSvcProvider->ShowSelection();
    }
}


VOID
CMarkup::InvalidateSelection(BOOL fFireOM )
{
    if ( _pSelRenSvcProvider )
    {
        _pSelRenSvcProvider->InvalidateSelection( TRUE, fFireOM );
    }
}

BOOL
CMarkup::IsElementSelected( 
    CElement* pElement )
{
    if ( _pSelRenSvcProvider )
    {
          return  _pSelRenSvcProvider->IsElementSelected( pElement );
    }
    else
    {
        return false;
    }    
}

CElement* 
CMarkup::GetSelectedElement( 
    int iElement  )
{
    if ( _pSelRenSvcProvider )
    {
          return  _pSelRenSvcProvider->GetSelectedElement( iElement );
    }
    else
    {
        return NULL;
    }    
}

CElement *
CMarkup::GetElementTop()
{
    CElement * pElement = GetElementClient();

    return pElement ? pElement : Root();
}


void
CMarkup::EnsureTopElems ( )
{
    CHtmlElement  * pHtmlElementCached;
    CHeadElement  * pHeadElementCached;
    CTitleElement * pTitleElementCached;
    CElement *      pElementClientCached;
    
    if (Doc()->GetDocTreeVersion() == _lTopElemsVersion)
        return;

    _lTopElemsVersion = Doc()->GetDocTreeVersion();

    static ELEMENT_TAG  atagStop[2] = { ETAG_BODY, ETAG_FRAMESET };
    
    CChildIterator ci(
        Root(), 
        NULL,
        CHILDITERATOR_USETAGS,
        atagStop, ARRAY_SIZE(atagStop));
    int nFound = 0;
    
    CTreeNode * pNode;

    pHtmlElementCached = NULL;
    pHeadElementCached = NULL;
    pTitleElementCached = NULL;
    pElementClientCached = NULL;
    
    while ( nFound < 4 && (pNode = ci.NextChild()) != NULL )
    {
        CElement * pElement = pNode->Element();
        
        switch ( pElement->Tag() )
        {
            case ETAG_HEAD  :
            {
                if (!pHeadElementCached)
                {
                    pHeadElementCached = DYNCAST( CHeadElement, pElement );
                    nFound++;
                }
                
                break;
            }
            case ETAG_HTML  :
            {
                if (!pHtmlElementCached)
                {
                    pHtmlElementCached = DYNCAST( CHtmlElement, pElement );
                    nFound++;
                }
                
                break;
            }
            case ETAG_TITLE_ELEMENT :
            {
                if (!pTitleElementCached)
                {
                    pTitleElementCached = DYNCAST( CTitleElement, pElement );
                    nFound++;
                }
                
                break;
            }
            case ETAG_BODY :
            case ETAG_FRAMESET :
            case ETAG_TXTSLAVE :
            {
                if (!pElementClientCached)
                {
                    pElementClientCached = pElement;
                    nFound++;
                }
                
                break;
            }
        }
    }

    if (nFound)
    {
        CMarkupTopElemCache * ptec = EnsureTopElemCache();

        if (ptec)
        {
            ptec->__pHtmlElementCached = pHtmlElementCached;
            ptec->__pHeadElementCached = pHeadElementCached;
            ptec->__pTitleElementCached = pTitleElementCached;
            ptec->__pElementClientCached = pElementClientCached;
        }
    }
    else
    {
        delete DelTopElemCache();
    }
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkup::DoesPersistMetaAllow
//
//  Synopsis:   the perist Metatags can enable/disable sets of controls
//      and objects.  This helper function will look for the given
//      element ID and try to determin if the meta tags allow or restrict
//      its default operation (wrt persistence)
//
//--------------------------------------------------------------------

BOOL
CMarkup::MetaPersistEnabled(long eState)
{
    CTreeNode * pNode;

    // search through the list of meta tags for one that would activate
    // the persistData objects

    if (!eState || !GetHeadElement())
        return FALSE;

    CChildIterator ci ( GetHeadElement() );

    while ( (pNode = ci.NextChild()) != NULL )
    {
        if (pNode->Tag() == ETAG_META )
        {
            if (DYNCAST( CMetaElement, pNode->Element() )->IsPersistMeta( eState ))
                return TRUE;
        }
    }

    return FALSE;
}

//+-------------------------------------------------------------------
//
//  Member:     CRootSite::LocateHeadMeta
//
//  Synopsis:   Look for a particular type of meta tag in the head
//
//--------------------------------------------------------------------

HRESULT
CMarkup::LocateHeadMeta (
    BOOL ( * pfnCompare ) ( CMetaElement * ), CMetaElement * * ppMeta )
{
    HRESULT hr = S_OK;
    CTreeNode * pNode;

    Assert( ppMeta );

    *ppMeta = NULL;

    if (GetHeadElement())
    {
        //
        // Attempt to locate an existing meta tag associated with the
        // head (we will not look for meta tags in the rest of the doc).
        //

        CChildIterator ci ( GetHeadElement() );

        while ( (pNode = ci.NextChild()) != NULL )
        {
            if (pNode->Tag() == ETAG_META)
            {
                *ppMeta = DYNCAST( CMetaElement, pNode->Element() );

                if (pfnCompare( *ppMeta ))
                    break;

                *ppMeta = NULL;
            }
        }
    }
    
    RRETURN( hr );
}

//+-------------------------------------------------------------------
//
//  Member:     CRootSite::LocateOrCreateHeadMeta
//
//  Synopsis:   Locate a
//
//--------------------------------------------------------------------

HRESULT
CMarkup::LocateOrCreateHeadMeta (
    BOOL ( * pfnCompare ) ( CMetaElement * ),
    CMetaElement * * ppMeta,
    BOOL fInsertAtEnd )
{
    HRESULT hr;
    CElement *pTempElement = NULL;

    Assert( ppMeta );

    //
    // First try to find one
    //

    *ppMeta = NULL;

    hr = THR(LocateHeadMeta(pfnCompare, ppMeta));
    if (hr)
        goto Cleanup;

    //
    // If we could not locate an existing element, make a new one
    //
    // _pElementHEAD can be null if we are doing a doc.open on a
    // partially created document.(pending navigation scenario)
    //

    if (!*ppMeta && GetHeadElement())
    {
        long lIndex = -1;

        if (_LoadStatus < LOADSTATUS_PARSE_DONE)
        {
            // Do not add create a meta element if we have not finished
            // parsing.
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = THR( CreateElement( ETAG_META, & pTempElement ) );

        if (hr)
            goto Cleanup;

        * ppMeta = DYNCAST( CMetaElement, pTempElement );
        
        Assert( * ppMeta );

        if (!fInsertAtEnd)
        {
            lIndex = 0;

            //
            // Should not insert stuff before the title
            //

            if (GetTitleElement() &&
                GetTitleElement()->GetFirstBranch()->Parent() &&
                GetTitleElement()->GetFirstBranch()->Parent()->Element() == GetHeadElement())
            {
                lIndex = 1;
            }
        }
            
        hr = THR( AddHeadElement( pTempElement, lIndex ) );
        
        if (hr)
            goto Cleanup;
    }

Cleanup:
    
    CElement::ClearPtr( & pTempElement );

    RRETURN( hr );
}

CElement *
CMarkup::FirstElement()
{
    // BUGBUG: when we get rid of the root elem, we want
    // to take these number down by one
    if(NumElems() >= 2 )
    {
        CTreePos * ptp;

        ptp = TreePosAtSourceIndex( 1 );
        Assert( ptp );
        return ptp->Branch()->Element();
    }

    return NULL;
}


//+-------------------------------------------------------------------
//
//  Member:     CMarkup::GetContentTreeExtent
//
//--------------------------------------------------------------------
void
CMarkup::GetContentTreeExtent(CTreePos ** pptpStart, CTreePos ** pptpEnd)
{
    // BUGBUG: this implementation is a hack and should change when we
    // get rid of the root and slave element

    CTreePos * ptpStart;

    Assert(Master());
    ptpStart = FirstTreePos()->NextTreePos();

    Assert( ptpStart );

    // Find first element (besides the root) in the markup.
    while( !ptpStart->IsBeginNode() )
    {
        Assert( ptpStart );
        ptpStart = ptpStart->NextTreePos();
    }

    Assert( ptpStart );

    if( pptpStart )
        *pptpStart = ptpStart;

    if (pptpEnd)
        ptpStart->Branch()->Element()->GetTreeExtent(NULL, pptpEnd);
}


//+-------------------------------------------------------------------
//
//  Member:     CMarkup::GetProgSinkC
//
//--------------------------------------------------------------------

CProgSink *
CMarkup::GetProgSinkC()
{
    return _pProgSink;
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkup::GetProgSink
//
//--------------------------------------------------------------------

IProgSink *
CMarkup::GetProgSink()
{
    return (IProgSink*) _pProgSink;
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkup::IsEditable
//
//--------------------------------------------------------------------

BOOL        
CMarkup::IsEditable(BOOL fCheckContainerOnly) const
{
    if (_pElementMaster)
        return _pElementMaster->IsEditable(fCheckContainerOnly);

    return _pDoc->_fDesignMode;
}

HRESULT
CMarkup::InvokeEx(DISPID dispid,
                  LCID lcid,
                  WORD wFlags,
                  DISPPARAMS *pdispparams,
                  VARIANT *pvarResult,
                  EXCEPINFO *pexcepinfo,
                  IServiceProvider *pSrvProvider)
{
    HRESULT hr;

    hr = THR(ValidateInvoke(pdispparams, pvarResult, pexcepinfo, NULL));
    if (hr)
        goto Cleanup;

    //
    // Handle the OLE automation reserved dispid DISPID_WINDOWOBJECT 
    // and dispid_READYSTATE specially.
    //

    hr = THR_NOTRACE(ReadyStateInvoke(dispid, wFlags, _pDoc->_readyState, pvarResult));
    if (hr != S_FALSE)
        goto Cleanup;

    if (DISPID_WINDOWOBJECT == dispid || 
        DISPID_SECURITYCTX == dispid ||
        DISPID_SECURITYDOMAIN == dispid)
    {
        hr = THR(_pDoc->EnsureOmWindow());
        if (hr)
            goto Cleanup;

        hr = THR(_pDoc->_pOmWindow->InvokeEx(dispid,
                                      lcid,
                                      wFlags,
                                      pdispparams,
                                      pvarResult,
                                      pexcepinfo,
                                      pSrvProvider));
    }
    else 
    {
        hr = THR(EnsureCollectionCache(NAVDOCUMENT_COLLECTION));
        if (hr)
            goto Cleanup;

        // for IE3 backward compat, doc.Script has a diff dispid than in IE4, so 
        // map it to new one.
        if (0x60020000 == dispid)
            dispid = DISPID_CDoc_Script;

        hr = THR_NOTRACE(DispatchInvokeCollection(IsPrimaryMarkup() ? _pDoc : (CBase *)this,
                                                  super::InvokeEx,
                                                  CollectionCache(),
                                                  NAVDOCUMENT_COLLECTION,
                                                  dispid,
                                                  IID_NULL,
                                                  lcid,
                                                  wFlags,
                                                  pdispparams,
                                                  pvarResult,
                                                  pexcepinfo,
                                                  NULL,
                                                  pSrvProvider));
    }

Cleanup:
    RRETURN_NOTRACE(hr);
}

HRESULT
CMarkup::GetDispID(BSTR bstrName,
                   DWORD grfdex,
                   DISPID *pid)
{
    HRESULT hr;

    hr = THR(EnsureCollectionCache(NAVDOCUMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(DispatchGetDispIDCollection(IsPrimaryMarkup() ? _pDoc : (CBase *)this,
                                                 super::GetDispID,
                                                 CollectionCache(),
                                                 NAVDOCUMENT_COLLECTION,
                                                 bstrName,
                                                 grfdex,
                                                 pid));

Cleanup:
    RRETURN(THR_NOTRACE(hr));
}

HRESULT
CMarkup::GetNextDispID(DWORD grfdex,
                       DISPID id,
                       DISPID *prgid)
{
    HRESULT     hr;

    hr = THR(EnsureCollectionCache(NAVDOCUMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    // Notice here that we set the fIgnoreOrdinals to TRUE for Nav compatability
    // We don't want ordinals in the document name space
    hr = DispatchGetNextDispIDCollection(IsPrimaryMarkup() ? _pDoc : (CBase *)this,
                                         super::GetNextDispID,
                                         CollectionCache(),
                                         NAVDOCUMENT_COLLECTION,
                                         grfdex,
                                         id,
                                         prgid);

Cleanup:
    RRETURN1(hr, S_FALSE);
}

HRESULT
CMarkup::GetMemberName(DISPID id,
                       BSTR *pbstrName)
{
    HRESULT     hr;

    hr = THR(EnsureCollectionCache(NAVDOCUMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    hr = DispatchGetMemberNameCollection(IsPrimaryMarkup() ? _pDoc : (CBase *)this,
                                         super::GetMemberName,
                                         CollectionCache(),
                                         NAVDOCUMENT_COLLECTION,
                                         id,
                                         pbstrName);

Cleanup:
    RRETURN(hr);
}

HRESULT
CMarkup::GetNameSpaceParent(IUnknown **ppunk)
{
    HRESULT hr;

    if ( ppunk )
        *ppunk = NULL;
    hr = THR(_pDoc->GetNameSpaceParent(ppunk));
    RRETURN(hr);
}

HRESULT
CMarkup::get_designMode(BSTR * pbstrMode)
{
    if ( pbstrMode )
        *pbstrMode = NULL;
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::put_designMode(BSTR bstrMode)
{
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::open(BSTR mimeType, VARIANT varName, VARIANT varFeatures, VARIANT varReplace,
                    /* retval */ IDispatch **ppDisp)
{
    if (ppDisp)
        *ppDisp = NULL;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::write(SAFEARRAY * psarray)
{
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::writeln(SAFEARRAY * psarray)
{
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::close(void)
{
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::clear(void)
{
    // This routine is a no-op under Navigator and IE.  Use document.close
    // followed by document.open to clear all elements in the document.

    return S_OK;
}

HRESULT
CMarkup::get_bgColor(VARIANT * p)
{
    if ( p )
    {
        p->vt = VT_NULL;
    }
        
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::put_bgColor(VARIANT p)
{
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_fgColor(VARIANT * p)
{
    if ( p )
    {
        p->vt = VT_NULL;
    }
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::put_fgColor(VARIANT p)
{
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_linkColor(VARIANT * p)
{
    if ( p )
    {
        p->vt = VT_NULL;
    }

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::put_linkColor(VARIANT p)
{
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_alinkColor(VARIANT * p)
{
    if ( p )
    {
        p->vt = VT_NULL;
    }

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::put_alinkColor(VARIANT p)
{
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_vlinkColor(VARIANT * p)
{
    if ( p )
    {
        p->vt = VT_NULL;
    }

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::put_vlinkColor(VARIANT p)
{
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_parentWindow(IHTMLWindow2 **ppWindow)
{
    if ( ppWindow )
        *ppWindow = NULL;
    return _pDoc->get_parentWindow(ppWindow);
}

HRESULT
CMarkup::get_activeElement(IHTMLElement ** ppElement)
{
    // BUGBUG(sramani) need to implement for view-slave elements
    if (ppElement)
        *ppElement = NULL;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_URL(BSTR * p)
{
    if ( p )
        *p = NULL;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::put_URL(BSTR b)
{
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_location(IHTMLLocation** ppLocation)
{
    if ( ppLocation )
        *ppLocation = NULL;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_lastModified(BSTR * p)
{
    if ( p )
        *p = NULL;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_referrer(BSTR * p)
{
    if ( p )
        *p = NULL;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_domain(BSTR * p)
{
    if ( p )
        *p = NULL;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::put_domain (BSTR p)
{
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_readyState(BSTR * p)
{
    if (p)
        *p = NULL;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_Script(IDispatch **ppWindow)
{
    if (ppWindow)
        *ppWindow = NULL;
    
    return _pDoc->get_Script(ppWindow);
}

HRESULT
CMarkup::releaseCapture()
{
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_frames( IHTMLFramesCollection2 ** ppDisp )
{
    if (ppDisp)
        *ppDisp = NULL;
    
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_styleSheets(IHTMLStyleSheetsCollection** ppDisp)
{
    HRESULT hr = E_POINTER;

    if (!ppDisp)
        goto Cleanup;

    hr = THR(EnsureStyleSheets());
    if (hr)
        goto Cleanup;

    hr = THR(GetStyleSheetArray()->QueryInterface(IID_IHTMLStyleSheetsCollection, (void**)ppDisp));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CMarkup::get_selection( IHTMLSelectionObject ** ppDisp )
{
    if (ppDisp)
        *ppDisp = NULL;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_cookie(BSTR* retval)
{
    if ( retval )
        *retval = NULL;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT CMarkup::put_cookie(BSTR cookie)
{
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT CMarkup::get_expando(VARIANT_BOOL *pfExpando)
{
    return _pDoc->get_expando(pfExpando);
}

HRESULT CMarkup::put_expando(VARIANT_BOOL fExpando)
{
    return _pDoc->put_expando(fExpando);
}

HRESULT CMarkup::get_charset(BSTR* retval)
{
    if ( retval )
        *retval = NULL;
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::put_charset(BSTR mlangIdStr)
{
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_defaultCharset(BSTR* retval)
{
    if ( retval )
        *retval = NULL;
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::put_defaultCharset(BSTR mlangIdStr)
{
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_dir(BSTR * p)
{
    if ( p )
        *p = NULL;
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::put_dir(BSTR v)
{
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_mimeType(BSTR * pMimeType)
{
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_fileSize(BSTR * pFileSize)
{
    if ( pFileSize )
        *pFileSize = NULL;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_fileCreatedDate(BSTR * pFileCreatedDate)
{
    if ( pFileCreatedDate )
        *pFileCreatedDate = NULL;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_fileModifiedDate(BSTR * pFileModifiedDate)
{
    if (pFileModifiedDate )
        *pFileModifiedDate = NULL;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_fileUpdatedDate(BSTR * pFileUpdatedDate)
{
    if (pFileUpdatedDate)
        *pFileUpdatedDate = NULL;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_security(BSTR * pSecurity)
{
    if ( pSecurity )
        pSecurity = NULL;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_protocol(BSTR * pProtocol)
{
    if (pProtocol)
        *pProtocol = NULL;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_nameProp(BSTR * pName)
{
    if ( pName )
        *pName = NULL;
    return get_title(pName);
}

HRESULT
CMarkup::toString(BSTR *pbstrString)
{
    RRETURN(super::toString(pbstrString));
}

HRESULT
CMarkup::attachEvent(BSTR event, IDispatch* pDisp, VARIANT_BOOL *pResult)
{
    RRETURN(super::attachEvent(event, pDisp, pResult));
}

HRESULT
CMarkup::detachEvent(BSTR event, IDispatch* pDisp)
{
    RRETURN(super::detachEvent(event, pDisp));
}

HRESULT
CMarkup::recalc(VARIANT_BOOL fForce)
{
    // recalc across all markups for now
    return _pDoc->recalc(fForce);
}

HRESULT CMarkup::createTextNode(BSTR text, IHTMLDOMNode **ppTextNode)
{
    if ( ppTextNode )
        *ppTextNode = NULL;
    return _pDoc->createTextNode(text, ppTextNode);
}

HRESULT
CMarkup::get_uniqueID(BSTR *pID)
{
    if ( pID )
        *pID = NULL;
    return _pDoc->get_uniqueID(pID);
}

HRESULT
CMarkup::createElement ( BSTR bstrTag, IHTMLElement ** pIElementNew )
{
    HRESULT hr = S_OK;
    CElement * pElement = NULL;

    if (!bstrTag || !pIElementNew)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( CreateElement( ETAG_NULL, & pElement, bstrTag, SysStringLen( bstrTag ) ) );
    if (hr)
        goto Cleanup;

    hr = THR( pElement->QueryInterface( IID_IHTMLElement, (void **) pIElementNew ) );
    if (hr)
        goto Cleanup;

Cleanup:

    if (pElement)
        pElement->Release();

    RRETURN( hr );
}

HRESULT
CMarkup::createStyleSheet ( BSTR bstrHref /*=""*/, long lIndex/*=-1*/, IHTMLStyleSheet ** ppnewStyleSheet )
{
    HRESULT         hr = S_OK;
    CElement      * pElementNew = NULL;
    CStyleSheet   * pStyleSheet;
    BOOL            fIsLinkElement;
    CStyleSheetArray * pStyleSheets;

    if ( !ppnewStyleSheet )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if ( !GetHeadElement() )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    *ppnewStyleSheet = NULL;

    // If there's an HREF, we create a LINK element, otherwise it's a STYLE element.

    fIsLinkElement = (bstrHref && *bstrHref);

    hr = THR(CreateElement((fIsLinkElement) ? ETAG_LINK : ETAG_STYLE, & pElementNew ) );
    
    if (hr)
        goto Cleanup;

    hr = EnsureStyleSheets();
    if (hr)
        goto Cleanup;

    Assert(HasStyleSheetArray());
    pStyleSheets = GetStyleSheetArray();

    if ( lIndex < -1 || lIndex >= pStyleSheets->Size() )
        lIndex = -1;    // Just append to the end if input is outside the bounds

    Assert( "Must have a root site!" && Root() );

    // Fix up the index - incoming index is index in stylesheets list, but param to
    // AddHeadElement is index in ALL head elements.  There may be META or TITLE, etc.
    // tags mixed in.
    if ( lIndex > 0 )
    {
        long nHeadNodes;
        long i;
        long nSSInHead;
        CTreeNode *pNode;
        CLinkElement *pLink;
        CStyleElement *pStyle;

        CChildIterator ci ( GetHeadElement(), NULL, CHILDITERATOR_DEEP );

        for ( nHeadNodes = 0 ; ci.NextChild() ; )
            nHeadNodes++;

        ci.SetBeforeBegin();
        
        nSSInHead = 0;
        
        for ( i = 0 ; (pNode = ci.NextChild()) != NULL ; i++ )
        {
            if ( pNode->Tag() == ETAG_LINK )
            {
                pLink = DYNCAST( CLinkElement, pNode->Element() );
                if ( pLink->_pStyleSheet ) // faster than IsLinkedStyleSheet() and adequate here
                    ++nSSInHead;
            }
            else if ( pNode->Tag() == ETAG_STYLE )
            {
                pStyle = DYNCAST( CStyleElement, pNode->Element() );
                if ( pStyle->_pStyleSheet ) // Not all STYLE elements create a SS.
                    ++nSSInHead;
            }
            if ( nSSInHead == lIndex )
            {           // We've found the stylesheet that should immediately precede us - we'll
                i++;    // add our new ss at the next head index.
                break;
            }
        }
        if ( i == nHeadNodes )   // We'll be at the end anyway.
            lIndex = -1;
        else
            lIndex = i;         // Here's the new index, adjusted for other HEAD elements.
    }

    // Go ahead and add it to the head.
    //--------------------------------
    //   For style elements we need to set _fParseFinished to FALSE so that the style sheet 
    // is not automatically created when we insert the style element into the tree from here. 
    // We will create and insert the styleSheet into the proper position of the collection later.
    //   When the element is inserted through DOM we want to have a stylesheet so we set 
    // _fParseFinished to TRUE (we are not going to parse anything in that case)
    if(!fIsLinkElement)
    {
        DYNCAST( CStyleElement, pElementNew)->_fParseFinished = FALSE;
    }

    hr = THR(AddHeadElement( pElementNew, lIndex ));
    if (hr)
        goto Cleanup;

    // We MUST put the element in the HEAD (the AddHeadElement() above) before we do the element-specific
    // stuff below, because the element will try to find itself in the head in CLinkElement::OnPropertyChange()
    // or CStyleElement::EnsureStyleSheet().
    if ( fIsLinkElement )
    {   // It's a LINK element - DYNCAST it and grab the CStyleSheet.
        CLinkElement *pLink = DYNCAST( CLinkElement, pElementNew );
        pLink->put_StringHelper( _T("stylesheet"), (PROPERTYDESC *)&s_propdescCLinkElementrel );
        pLink->put_UrlHelper( bstrHref, (PROPERTYDESC *)&s_propdescCLinkElementhref );
        pStyleSheet = pLink->_pStyleSheet;
    }
    else
    {   // It's a STYLE element - this will make sure we create a stylesheet attached to the CStyleElement.
        CStyleElement *pStyle = DYNCAST( CStyleElement, pElementNew );
        hr = THR( pStyle->EnsureStyleSheet() );
        if (hr)
            goto Cleanup;
        pStyleSheet = pStyle->_pStyleSheet;
    }

    if ( !pStyleSheet )
        hr = E_OUTOFMEMORY;
    else
        hr = THR( pStyleSheet->QueryInterface ( IID_IHTMLStyleSheet, (void **)ppnewStyleSheet ) );

Cleanup:
    CElement::ClearPtr(&pElementNew);

    RRETURN(SetErrorInfo(hr));
}

HRESULT
CMarkup::elementFromPoint(long x, long y, IHTMLElement **ppElement)
{
    if ( ppElement )
        *ppElement = NULL;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::execCommand(BSTR bstrCmdId, VARIANT_BOOL showUI, VARIANT value, VARIANT_BOOL *pfRet)
{
    if (pfRet)
        *pfRet = VARIANT_FALSE;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::execCommandShowHelp(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    if (pfRet)
        *pfRet = VARIANT_FALSE;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::queryCommandSupported(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    if (pfRet)
        *pfRet = VARIANT_FALSE;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::queryCommandEnabled(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    if (pfRet)
        *pfRet = VARIANT_FALSE;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::queryCommandState(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    if (pfRet)
        *pfRet = VARIANT_FALSE;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::queryCommandIndeterm(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    if (pfRet)
        *pfRet = VARIANT_FALSE;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::queryCommandText(BSTR bstrCmdId, BSTR *pcmdText)
{
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::queryCommandValue(BSTR bstrCmdId, VARIANT *pvarRet)
{
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::createDocumentFragment(IHTMLDocument2 **ppNewDoc)
{
/*
    // BUGBUG: Revisit in IE6
    HRESULT hr;
    CMarkup *pMarkup;

    if ( !ppNewDoc )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
        
    *ppNewDoc = NULL;

    hr = THR(_pDoc->CreateMarkup(&pMarkup));
    if (hr)
        goto Cleanup;

    hr = THR(pMarkup->QueryInterface(IID_IHTMLDocument2, (void **)ppNewDoc));
    if (hr)
        goto Cleanup;

    pMarkup->Release();
    hr = THR(pMarkup->SetParentMarkup(this));
    if (hr)
        goto Cleanup;

    AddRef(); // parent has to stick around.
    pMarkup->_fEnableDownload = _fEnableDownload;

Cleanup:
    RRETURN(SetErrorInfo(hr));
*/
    if (ppNewDoc)
        *ppNewDoc = NULL;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_parentDocument(IHTMLDocument2 **ppParentDoc)
{
    if (ppParentDoc)
        *ppParentDoc = NULL;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_enableDownload(VARIANT_BOOL *pfDownload)
{
/*
    // BUGBUG: Revisit in IE6
    if (pfDownload)
        *pfDownload = _fEnableDownload ? VARIANT_TRUE : VARIANT_FALSE;
*/
    if (pfDownload)
        *pfDownload = VARIANT_FALSE;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::put_enableDownload(VARIANT_BOOL fDownload)
{
/*
    // BUGBUG: Revisit in IE6
    _fEnableDownload = !!fDownload;
*/
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_baseUrl(BSTR *p)
{
    if ( p )
        *p = NULL;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::put_baseUrl(BSTR b)
{
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::get_childNodes(IDispatch **ppChildCollection)
{
    return Root()->get_childNodes(ppChildCollection);
}

HRESULT
CMarkup::get_inheritStyleSheets(VARIANT_BOOL *pfInherit)
{
    if ( pfInherit )
        *pfInherit = NULL;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::put_inheritStyleSheets(VARIANT_BOOL fInherit)
{
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CMarkup::getElementsByName(BSTR v, IHTMLElementCollection** ppDisp)
{
    HRESULT hr = E_INVALIDARG;

    if (!ppDisp || !v)
        goto Cleanup;

    *ppDisp = NULL;

    hr = THR(GetDispByNameOrID(v, (IDispatch **)ppDisp, TRUE));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CMarkup::getElementsByTagName(BSTR v, IHTMLElementCollection** ppDisp)
{
    HRESULT hr = E_INVALIDARG;
    if (!ppDisp || !v)
        goto Cleanup;

    *ppDisp = NULL;

    // Make sure our collection is up-to-date.
    hr = THR(EnsureCollectionCache(ELEMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    // Get a collection of the specified tags.
    hr = THR(CollectionCache()->GetDisp(ELEMENT_COLLECTION,
                                        v,
                                        CacheType_Tag,
                                        (IDispatch**)ppDisp,
                                        FALSE)); // Case sensitivity ignored for TagName
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CMarkup::getElementById(BSTR v, IHTMLElement** ppel)
{
    HRESULT hr = E_INVALIDARG;
    CElement *pel = NULL;

    if (!ppel || !v)
        goto Cleanup;

    *ppel = NULL;

    hr = THR(GetElementByNameOrID(v, &pel));
    // Didn't find elem with id v, return null, if hr == S_FALSE, more than one elem, return first
    if (FAILED(hr))
    {
        hr = S_OK;
        goto Cleanup;
    }

    Assert(pel);
    hr = THR(pel->QueryInterface(IID_IHTMLElement, (void **)ppel));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

#ifdef IE5_ZOOM

HRESULT
CMarkup::zoom(long Numer, long Denom)
{
    // TODO
    return S_OK;
}

HRESULT
CMarkup::get_zoomNumerator(long * pNumer)
{
    // TODO
    return S_OK;
}

HRESULT
CMarkup::get_zoomDenominator(long * pDenom)
{
    // TODO
    return S_OK;
}

#endif  // IE5_ZOOM

//+---------------------------------------------------------------------------
//
//  Lookaside storage
//
//----------------------------------------------------------------------------

void *
CMarkup::GetLookasidePtr(int iPtr)
{
#if DBG == 1
    if(HasLookasidePtr(iPtr))
    {
        void * pLookasidePtr =  Doc()->GetLookasidePtr((DWORD *)this + iPtr);

        Assert(pLookasidePtr == _apLookAside[iPtr]);

        return pLookasidePtr;
    }
    else
        return NULL;
#else
    return(HasLookasidePtr(iPtr) ? Doc()->GetLookasidePtr((DWORD *)this + iPtr) : NULL);
#endif
}

HRESULT
CMarkup::SetLookasidePtr(int iPtr, void * pvVal)
{
    Assert (!HasLookasidePtr(iPtr) && "Can't set lookaside ptr when the previous ptr is not cleared");

    HRESULT hr = THR(Doc()->SetLookasidePtr((DWORD *)this + iPtr, pvVal));

    if (hr == S_OK)
    {
        _fHasLookasidePtr |= 1 << iPtr;

#if DBG == 1
        _apLookAside[iPtr] = pvVal;
#endif
    }

    RRETURN(hr);
}

void *
CMarkup::DelLookasidePtr(int iPtr)
{
    if (HasLookasidePtr(iPtr))
    {
        void * pvVal = Doc()->DelLookasidePtr((DWORD *)this + iPtr);
        _fHasLookasidePtr &= ~(1 << iPtr);
#if DBG == 1
        _apLookAside[iPtr] = NULL;
#endif
        return(pvVal);
    }

    return(NULL);
}

//
// CMarkup ref counting helpers
//

void
CMarkup::ReplacePtr ( CMarkup * * pplhs, CMarkup * prhs )
{
    if (pplhs)
    {
        if (prhs)
        {
            prhs->AddRef();
        }
        if (*pplhs)
        {
            (*pplhs)->Release();
        }
        *pplhs = prhs;
    }
}

void
CMarkup::SetPtr ( CMarkup ** pplhs, CMarkup * prhs )
{
    if (pplhs)
    {
        if (prhs)
        {
            prhs->AddRef();
        }
        *pplhs = prhs;
    }
}

void
CMarkup::StealPtrSet ( CMarkup ** pplhs, CMarkup * prhs )
{
    SetPtr( pplhs, prhs );

    if (pplhs && *pplhs)
        (*pplhs)->Release();
}

void
CMarkup::StealPtrReplace ( CMarkup ** pplhs, CMarkup * prhs )
{
    ReplacePtr( pplhs, prhs );

    if (pplhs && *pplhs)
        (*pplhs)->Release();
}

void
CMarkup::ClearPtr ( CMarkup * * pplhs )
{
    if (pplhs && * pplhs)
    {
        CMarkup * pElement = *pplhs;
        *pplhs = NULL;
        pElement->Release();
    }
}

void
CMarkup::ReleasePtr ( CMarkup * pMarkup )
{
    if (pMarkup)
    {
        pMarkup->Release();
    }
}

const CBase::CLASSDESC CDocFrag::s_classdesc =
{
    &CLSID_HTMLDocumentFragment,    // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLDocumentFragment,     // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

ULONG
CDocFrag::PrivateAddRef()
{
    return Markup()->SubAddRef();
}

ULONG
CDocFrag::PrivateRelease()
{
    return Markup()->SubRelease();
}

HRESULT
CDocFrag::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_TEAROFF_DISPEX(this, NULL)
    QI_TEAROFF(this, IHTMLDocumentFragment, NULL)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
    {
        RRETURN (super::PrivateQueryInterface(iid, ppv));
    }
}

HRESULT
CDocFrag::get_document(IDispatch ** ppIHtmlDoc)
{
    HRESULT hr;

    if ( !ppIHtmlDoc )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(Markup()->PrivateQueryInterface(IID_IHTMLDocument2, (void**)ppIHtmlDoc));
Cleanup:
    RRETURN(hr);
}


HRESULT
CDocFrag::GetNameSpaceParent(IUnknown **ppunk)
{
    HRESULT hr;

    hr = THR(Markup()->PrivateQueryInterface(IID_IDispatchEx, (void**) ppunk));

    RRETURN(hr);
}

#if MARKUP_DIRTYRANGE
//+---------------------------------------------------------------------------
//
//  Dirty Range Service
//
//----------------------------------------------------------------------------
HRESULT 
CMarkup::RegisterForDirtyRange( DWORD * pdwCookie )
{
    HRESULT                     hr = S_OK;
    MarkupDirtyRange *          pdr;
    CMarkupDirtyRangeContext *  pdrc;

    pdrc = EnsureDirtyRangeContext();
    if (!pdrc)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR( pdrc->_aryMarkupDirtyRange.AppendIndirect( NULL, &pdr ) );
    if (hr)
        goto Cleanup;

    Assert( pdr );

    pdr->_dwCookie  = 
    *pdwCookie      = InterlockedIncrement( (long *) &s_dwDirtyRangeServiceCookiePool );

Cleanup:
    RRETURN( hr );
}

HRESULT 
CMarkup::UnRegisterForDirtyRange( DWORD dwCookie )
{
    HRESULT                     hr = S_OK;
    CMarkupDirtyRangeContext *  pdrc;
    MarkupDirtyRange *          pdr;
    int                         cdr, idr;

    if (!HasDirtyRangeContext())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pdrc = GetDirtyRangeContext();
    Assert( pdrc );

    for( idr = 0, cdr = pdrc->_aryMarkupDirtyRange.Size(), pdr = pdrc->_aryMarkupDirtyRange;
         idr < cdr;
         idr++, pdr++ )
    {
        if( pdr->_dwCookie == dwCookie )
        {
            pdrc->_aryMarkupDirtyRange.Delete( idr );
            goto Cleanup;
        }
    }

    hr = E_FAIL;

Cleanup:
    RRETURN( hr );
}

HRESULT 
CMarkup::GetAndClearDirtyRange( DWORD dwCookie, CMarkupPointer * pmpBegin, CMarkupPointer * pmpEnd )
{
    HRESULT                     hr = S_OK;
    CMarkupDirtyRangeContext *  pdrc;
    MarkupDirtyRange *          pdr;
    int                         cdr, idr;

    if (!HasDirtyRangeContext())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pdrc = GetDirtyRangeContext();
    Assert( pdrc );

    for( idr = 0, cdr = pdrc->_aryMarkupDirtyRange.Size(), pdr = pdrc->_aryMarkupDirtyRange;
         idr < cdr;
         idr++, pdr++ )
    {
        if (pdr->_dwCookie == dwCookie)
        {
            if (pmpBegin)
            {
                hr = THR( pmpBegin->MoveToCp( pdr->_dtr._cp, this ) );
                if (hr)
                    goto Cleanup;
            }
            
            if (pmpEnd)
            {
                hr = THR( pmpEnd->MoveToCp( pdr->_dtr._cp + pdr->_dtr._cchNew, this ) );
                if (hr)
                    goto Cleanup;
            }

            pdr->_dtr.Reset();

            goto Cleanup;
        }
    }

    hr = E_FAIL;

Cleanup:
    RRETURN( hr );
}

HRESULT 
CMarkup::GetAndClearDirtyRange( DWORD dwCookie, IMarkupPointer * pIPointerBegin, IMarkupPointer * pIPointerEnd )
{
    HRESULT hr;
    CDoc *  pDoc = Doc();
    CMarkupPointer * pmpBegin, * pmpEnd;

    if (    (pIPointerBegin && !pDoc->IsOwnerOf( pIPointerBegin ))
        ||  (pIPointerEnd && !pDoc->IsOwnerOf( pIPointerEnd )))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pIPointerBegin->QueryInterface( CLSID_CMarkupPointer, (void **) & pmpBegin ) );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    hr = THR( pIPointerEnd->QueryInterface( CLSID_CMarkupPointer, (void **) & pmpEnd ) );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = GetAndClearDirtyRange( dwCookie, pmpBegin, pmpEnd );

Cleanup:
    RRETURN( hr );
}

void    
CMarkup::OnDirtyRangeChange(DWORD_PTR dwContext)
{
    _fOnDirtyRangeChangePosted = FALSE;
    // TODO: this is where listeners will hear that the dirty range has changed
}

CMarkupDirtyRangeContext *    
CMarkup::EnsureDirtyRangeContext()
{
    CMarkupDirtyRangeContext *    pdrc;

    if (HasDirtyRangeContext())
        return GetDirtyRangeContext();

    pdrc = new CMarkupDirtyRangeContext;
    if (!pdrc)
        return NULL;

    if (SetDirtyRangeContext( pdrc ))
    {
        delete pdrc;
        return NULL;
    }

    return pdrc;
}
#endif // MARKUP_DIRTYRANGE

//+---------------------------------------------------------------------------
//
//  Top Elem Cache
//
//----------------------------------------------------------------------------

CMarkupTopElemCache * 
CMarkup::EnsureTopElemCache()
{
    CMarkupTopElemCache * ptec;

    if (HasTopElemCache())
        return GetTopElemCache();

    ptec = new CMarkupTopElemCache;
    if (!ptec)
        return NULL;

    if (SetTopElemCache( ptec ))
    {
        delete ptec;
        return NULL;
    }

    return ptec;
}

//+---------------------------------------------------------------------------
//
//  Text Frag Service
//
//----------------------------------------------------------------------------

CMarkupTextFragContext * 
CMarkup::EnsureTextFragContext()
{
    CMarkupTextFragContext *    ptfc;

    if (HasTextFragContext())
        return GetTextFragContext();

    ptfc = new CMarkupTextFragContext;
    if (!ptfc)
        return NULL;

    if (SetTextFragContext( ptfc ))
    {
        delete ptfc;
        return NULL;
    }

    return ptfc;
}

CMarkupTextFragContext::~CMarkupTextFragContext()
{
    int cFrag;
    MarkupTextFrag * ptf;

    for( cFrag = _aryMarkupTextFrag.Size(), ptf = _aryMarkupTextFrag; cFrag > 0; cFrag--, ptf++ )
    {
        MemFreeString( ptf->_pchTextFrag );
    }
}

HRESULT     
CMarkupTextFragContext::AddTextFrag( CTreePos * ptpTextFrag, TCHAR* pchTextFrag, ULONG cchTextFrag, long iTextFrag )
{
    HRESULT             hr = S_OK;
    TCHAR *             pchCopy = NULL;
    MarkupTextFrag *    ptf = NULL;

    Assert( iTextFrag >= 0 && iTextFrag <= _aryMarkupTextFrag.Size() );

    // Allocate and copy the string
    hr = THR( MemAllocStringBuffer( Mt(MarkupTextFrag_pchTextFrag), cchTextFrag, pchTextFrag, &pchCopy ) );
    if (hr)
        goto Cleanup;

    // Allocate the TextFrag object in the array
    hr = THR( _aryMarkupTextFrag.InsertIndirect( iTextFrag, NULL ) );
    if (hr)
    {
        MemFreeString( pchCopy );
        goto Cleanup;
    }

    // Fill in the text frag

    ptf = &_aryMarkupTextFrag[iTextFrag];
    Assert( ptf );

    ptf->_ptpTextFrag = ptpTextFrag;
    ptf->_pchTextFrag = pchCopy;

Cleanup:
    RRETURN( hr );
}

HRESULT     
CMarkupTextFragContext::RemoveTextFrag( long iTextFrag, CMarkup * pMarkup )
{
    HRESULT             hr = S_OK;
    MarkupTextFrag *    ptf;

    Assert( iTextFrag >= 0 && iTextFrag <= _aryMarkupTextFrag.Size() );

    // Get the text frag
    ptf = &_aryMarkupTextFrag[iTextFrag];

    // Free the string
    MemFreeString( ptf->_pchTextFrag );

    hr = THR( pMarkup->RemovePointerPos( ptf->_ptpTextFrag, NULL, NULL ) );
    if (hr)
        goto Cleanup;

Cleanup:

    // Remove the entry from the array
    _aryMarkupTextFrag.Delete( iTextFrag );

    RRETURN( hr );
}

long
CMarkupTextFragContext::FindTextFragAtCp( long cpFind, BOOL * pfFragFound )
{
    // Do a binary search through the array to find the spot in the array
    // where cpFind lies

    int             iFragLow, iFragHigh, iFragMid;
    MarkupTextFrag* pmtfAry = _aryMarkupTextFrag;
    MarkupTextFrag* pmtfMid;
    BOOL            fResult;
    long            cpMid;

    iFragLow  = 0;
    fResult   = FALSE;
    iFragHigh = _aryMarkupTextFrag.Size() - 1;

    while (iFragLow <= iFragHigh)
    {
        iFragMid = (iFragLow + iFragHigh) >> 1;

        pmtfMid  = pmtfAry + iFragMid;
        cpMid    = pmtfMid->_ptpTextFrag->GetCp();

        if (cpMid == cpFind)
        {
            iFragLow = iFragMid;
            fResult = TRUE;
            break;
        }
        else if (cpMid < cpFind)
            iFragLow = iFragMid + 1;
        else
            iFragHigh = iFragMid - 1;
    }

    if( fResult )
    {
        // Search backward through all of the entries
        // at the same cp so we return the first one
        iFragLow--;
        while (iFragLow)
        {
            if( pmtfAry[iFragLow]._ptpTextFrag->GetCp() != cpFind )
            {
                iFragLow++;
                break;
            }
            iFragLow--;
        }
    }

    if (pfFragFound)
        *pfFragFound = fResult;

    Assert( iFragLow == 0 || pmtfAry[iFragLow-1]._ptpTextFrag->GetCp() < cpFind );
    Assert(     iFragLow == _aryMarkupTextFrag.Size() - 1 
            ||  _aryMarkupTextFrag.Size() == 0
            ||  cpFind <= pmtfAry[iFragLow]._ptpTextFrag->GetCp() );
    Assert( !fResult || pmtfAry[iFragLow]._ptpTextFrag->GetCp() == cpFind );

    return iFragLow;
}

#if DBG==1
void
CMarkupTextFragContext::TextFragAssertOrder()
{
    // Walk the array and assert order for each pair
    // to make sure that the array is ordered correctly
    int cFrag;
    MarkupTextFrag * ptf, *ptfLast = NULL;

    for( cFrag = _aryMarkupTextFrag.Size(), ptf = _aryMarkupTextFrag; cFrag > 0; cFrag--, ptf++ )
    {
        if (ptfLast)
        {
            Assert( ptfLast->_ptpTextFrag->InternalCompare( ptf->_ptpTextFrag ) == -1 );
        }

        ptfLast = ptf;
    }
}
#endif


//+-------------------------------------------------------------------------
//
// IMarkupTextFrags Method Implementations
//
//--------------------------------------------------------------------------
HRESULT 
CMarkup::GetTextFragCount(long* pcFrags)
{
    HRESULT hr = S_OK;

    if (!pcFrags)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if( !HasTextFragContext() )
        *pcFrags = 0;
    else
        *pcFrags = GetTextFragContext()->_aryMarkupTextFrag.Size();

Cleanup:
    RRETURN( hr );
}

HRESULT 
CMarkup::GetTextFrag(long iFrag, BSTR* pbstrFrag, IMarkupPointer* pIPointerFrag)
{
    HRESULT hr = S_OK;
    CDoc *  pDoc = Doc();
    CMarkupTextFragContext *    ptfc = GetTextFragContext();
    MarkupTextFrag *            ptf;
    CMarkupPointer *            pPointerFrag;

    // Check args
    if (    !pbstrFrag
        ||  !pIPointerFrag
        ||  !pDoc->IsOwnerOf( pIPointerFrag )
        ||  !ptfc
        ||  iFrag < 0
        ||  iFrag >= ptfc->_aryMarkupTextFrag.Size())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pbstrFrag = NULL;

    // Crack the pointer
    hr = pIPointerFrag->QueryInterface(CLSID_CMarkupPointer, (void**)&pPointerFrag);
    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // copy the string
    ptf = &(ptfc->_aryMarkupTextFrag[iFrag]);
    Assert( ptf );

    hr = THR( FormsAllocString( ptf->_pchTextFrag, pbstrFrag ) );
    if (hr)
        goto Cleanup;

    // position the pointer
    {
        CTreePosGap tpgPointer( ptf->_ptpTextFrag, TPG_RIGHT );
        hr = THR( pPointerFrag->MoveToGap( &tpgPointer, this ) );
        if (hr)
        {
            FormsFreeString( *pbstrFrag );
            *pbstrFrag = NULL;
            goto Cleanup;
        }
    }

Cleanup:
    RRETURN( hr );
}

HRESULT 
CMarkup::RemoveTextFrag(long iFrag)
{
    HRESULT hr = S_OK;
    CMarkupTextFragContext *    ptfc = GetTextFragContext();
    
    // Check args
    if (    !ptfc
        ||  iFrag < 0
        ||  iFrag >= ptfc->_aryMarkupTextFrag.Size())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Remove the text frag
    hr = THR( ptfc->RemoveTextFrag( iFrag, this ) );
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN( hr );
}

HRESULT 
CMarkup::InsertTextFrag(long iFragInsert, BSTR bstrInsert, IMarkupPointer* pIPointerInsert)
{
    HRESULT                     hr = S_OK;
    CDoc *                      pDoc = Doc();
    CMarkupTextFragContext *    ptfc = EnsureTextFragContext();
    CMarkupPointer *            pPointerInsert;
    CTreePosGap                 tpgInsert;

    // Check for really bad args
    if (    !pIPointerInsert
        ||  !pDoc->IsOwnerOf( pIPointerInsert )
        ||  !bstrInsert)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Make sure we had or created a text frag context
    if (!ptfc)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Crack the pointer
    hr = pIPointerInsert->QueryInterface(CLSID_CMarkupPointer, (void**)&pPointerInsert);
    if (hr || !pPointerInsert->IsPositioned())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }


    // Find the position for insert
    {
        BOOL fFragFound = TRUE;
        BOOL fPosSpecified = TRUE;
        long cFrags = ptfc->_aryMarkupTextFrag.Size();
        long cpInsert = pPointerInsert->GetCp();
        long cpBefore = -1;
        long cpAfter = MAXLONG;

        if (iFragInsert < 0)
        {
            iFragInsert = ptfc->FindTextFragAtCp( cpInsert, &fFragFound );
            fPosSpecified = FALSE;
        }

        // Make sure that the pointer and iFragInsert are in sync
        if (fPosSpecified)
        {
            if (iFragInsert > 0)
            {
                cpBefore = ptfc->_aryMarkupTextFrag[iFragInsert-1]._ptpTextFrag->GetCp();
            }

            if (iFragInsert <= cFrags - 1)
            {
                cpAfter = ptfc->_aryMarkupTextFrag[iFragInsert]._ptpTextFrag->GetCp();
            }

            if (cpBefore > cpInsert || cpAfter < cpInsert || iFragInsert > cFrags)
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }
        }

        // Position it carefully if a neighbor is at the same cp
        if (fPosSpecified && cpBefore == cpInsert)
        {
            hr = THR( tpgInsert.MoveTo( ptfc->_aryMarkupTextFrag[iFragInsert-1]._ptpTextFrag, TPG_RIGHT ) );
            if (hr)
                goto Cleanup;
        }
        else if (fPosSpecified && cpAfter == cpInsert || !fPosSpecified && fFragFound)
        {
            hr = THR( tpgInsert.MoveTo( ptfc->_aryMarkupTextFrag[iFragInsert]._ptpTextFrag, TPG_LEFT ) );
            if (hr)
                goto Cleanup;
        }
        else
        {
            // Note: Eric, you will have to split any text poses here if pPointerInsert
            // is ghosted.
            // Note2 (Eric): just force all pointers to embed.  Could make this faster
            //

            hr = THR( EmbedPointers() );

            if (hr)
                goto Cleanup;
            
            hr = THR( tpgInsert.MoveTo( pPointerInsert->GetEmbeddedTreePos(), TPG_LEFT ) );
            if (hr)
                goto Cleanup;
        }
    }

    // Now that we have where we want to insert this text frag, we have to actually insert
    // it an add it to the list
    {
        CTreePos * ptpInsert;

        ptpInsert = NewPointerPos( NULL, FALSE, FALSE );
        if( ! ptpInsert )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR( Insert( ptpInsert, &tpgInsert ) );
        if (hr)
        {
            FreeTreePos( ptpInsert );
            goto Cleanup;
        }

        hr = THR( ptfc->AddTextFrag( ptpInsert, bstrInsert, FormsStringLen( bstrInsert), iFragInsert ) );
        if (hr)
        {
            IGNORE_HR( Remove( ptpInsert ) );
            goto Cleanup;
        }
    }

    WHEN_DBG( ptfc->TextFragAssertOrder() );

Cleanup:
    RRETURN( hr );
}

HRESULT 
CMarkup::FindTextFragFromMarkupPointer(IMarkupPointer* pIPointerFind,long* piFrag,BOOL* pfFragFound)
{
    HRESULT hr = S_OK;
    CDoc *  pDoc = Doc();
    CMarkupTextFragContext *    ptfc = GetTextFragContext();
    CMarkupPointer *            pPointerFind;

    // Check args
    if (    !pIPointerFind
        ||  !pDoc->IsOwnerOf( pIPointerFind ))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Crack the pointer
    hr = pIPointerFind->QueryInterface(CLSID_CMarkupPointer, (void**)&pPointerFind);
    if (hr || !pPointerFind->IsPositioned())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }


    {
        long iFrag;
        BOOL fFragFound = FALSE;

        if (ptfc)
        {
            iFrag = ptfc->FindTextFragAtCp( pPointerFind->GetCp(), &fFragFound );
        }
        else
        {
            // If we don't have a context, return 0/False
            iFrag = 0;
        }

        if (piFrag)
            *piFrag = iFrag;

        if (pfFragFound)
            *pfFragFound = fFragFound;
    }


Cleanup:
    RRETURN( hr );
}


//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::EnsureStyleSheets
//
//  Synopsis:   Ensure the stylesheets collection exists, creates it if not..
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::EnsureStyleSheets()
{
    CStyleSheetArray * pStyleSheets;
    if (HasStyleSheetArray())
        return S_OK;

    pStyleSheets = new CStyleSheetArray( this, NULL, 0 );
    if (!pStyleSheets || pStyleSheets->_fInvalid )
        return E_OUTOFMEMORY;

    SetStyleSheetArray(pStyleSheets);
    return S_OK;
}


HRESULT 
CMarkup::ApplyStyleSheets(
        CStyleInfo *    pStyleInfo,
        ApplyPassType   passType,
        EMediaType      eMediaType,
        BOOL *          pfContainsImportant)
{
    HRESULT hr = S_OK;
    CStyleSheetArray * pStyleSheets;
    
    if (Doc()->_pHostStyleSheets)
    {
        hr = THR(Doc()->_pHostStyleSheets->Apply(pStyleInfo, passType, eMediaType, NULL));
        if (hr)
            goto Cleanup;
    }

    if (!HasStyleSheetArray())
        return S_OK;

    pStyleSheets = GetStyleSheetArray();
        
    if (!pStyleSheets->Size())
        return S_OK;

    hr = THR(pStyleSheets->Apply(pStyleInfo, passType, eMediaType, pfContainsImportant));

Cleanup:
    RRETURN(hr);
}
    

//+---------------------------------------------------------------
//
//  Member:         CMarkup::OnCssChange
//
//---------------------------------------------------------------

HRESULT
CMarkup::OnCssChange(BOOL fStable, BOOL fRecomputePeers)
{
    HRESULT     hr = S_OK;

    if (fRecomputePeers)
    {
        if (fStable)
            hr = THR(RecomputePeers());
        else
            hr = THR(ProcessPeerTask(PEERTASK_MARKUP_RECOMPUTEPEERS_UNSTABLE));

        if (hr)
            goto Cleanup;
    }

    if (IsPrimaryMarkup())
    {
        hr = Doc()->OnCssChange();
    }

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------
//
//  Member:         CMarkup::EnsureFormats
//
//---------------------------------------------------------------

void
CMarkup::EnsureFormats()
{
    // Walk the tree and make sure all formats are computed

    CTreePos * ptpCurr = FirstTreePos();

    while (ptpCurr)
    {
        if (ptpCurr->IsBeginNode())
        {
            ptpCurr->Branch()->EnsureFormats();
        }
        ptpCurr = ptpCurr->NextTreePos();
    }
}

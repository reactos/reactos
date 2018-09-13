//+---------------------------------------------------------------------
//
//   File:      element.cxx
//
//  Contents:   Element class
//
//  Classes:    CElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ASSOC_HXX_
#define X_ASSOC_HXX_
#include "assoc.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_CGUID_H_
#define X_CGUID_H_
#include "cguid.h"
#endif

#ifndef X_CONNECT_HXX_
#define X_CONNECT_HXX_
#include "connect.hxx"
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X_SCRIPT_HXX_
#define X_SCRIPT_HXX_
#include "script.hxx"
#endif

#ifndef X_CFPF_HXX_
#define X_CFPF_HXX_
#include "cfpf.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_HTMTAGS_HXX_
#define X_HTMTAGS_HXX_
#include "htmtags.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X__DOC_H_
#define X__DOC_H_
#include "_doc.h"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_HYPLNK_HXX_
#define X_HYPLNK_HXX_
#include "hyplnk.hxx"
#endif

#ifndef X_EANCHOR_HXX_
#define X_EANCHOR_HXX_
#include "eanchor.hxx"
#endif

#ifndef X_ELABEL_HXX_
#define X_ELABEL_HXX_
#include "elabel.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include "dispex.h"
#endif

#ifndef X_EVENTOBJ_HXX_
#define X_EVENTOBJ_HXX_
#include "eventobj.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif


#ifndef X_EBODY_HXX_
#define X_EBODY_HXX_
#include "ebody.hxx"    // for body's dispids
#endif

#ifndef X_EAREA_HXX_
#define X_EAREA_HXX_
#include "earea.hxx"
#endif

#ifndef X_MISCPROT_H_
#define X_MISCPROT_H_
#include "miscprot.h"
#endif

#ifndef X_FILTCOL_HXX_
#define X_FILTCOL_HXX_
#include "filtcol.hxx"
#endif

#ifndef X_OCIDL_H_
#define X_OCIDL_H_
#include <ocidl.h>
#endif

#ifndef X_SAFETY_HXX_
#define X_SAFETY_HXX_
#include "safety.hxx"
#endif

#ifndef X_SHOLDER_HXX_
#define X_SHOLDER_HXX_
#include "sholder.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_PEERFACT_HXX_
#define X_PEERFACT_HXX_
#include "peerfact.hxx"
#endif

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_FILTER_HXX_
#define X_FILTER_HXX_
#include "filter.hxx"
#endif

#ifndef X_GENERIC_HXX_
#define X_GENERIC_HXX_
#include "generic.hxx"
#endif

#ifndef X_EXTDL_HXX_
#define X_EXTDL_HXX_
#include "extdl.hxx"
#endif

// Note - The enums in types are defined in this file

#ifndef X_STRING_H_
#define X_STRING_H_
#include "string.h"
#endif

#ifndef X_HTMTAGS_HXX_
#define X_HTMTAGS_HXX_
#include "htmtags.hxx"
#endif

#if defined(_M_ALPHA)
#ifndef X_TEAROFF_HXX_
#define X_TEAROFF_HXX_
#include "tearoff.hxx"
#endif
#endif

#ifndef X_MSHTMDID_H_
#define X_MSHTMDID_H_
#include "mshtmdid.h"
#endif

#ifndef X_IEXTAG_HXX_
#define X_IEXTAG_HXX_
#include "iextag.h"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#ifndef X_DMEMBMGR_HXX_
#define X_DMEMBMGR_HXX_
#include <dmembmgr.hxx>       // for CDataMemberMgr
#endif

#ifndef X_ACCHDRS_HXX_
#define X_ACCHDRS_HXX_
#include "acchdrs.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_EFORM_HXX_
#define X_EFORM_HXX_
#include "eform.hxx"
#endif

#ifndef X_EOBJECT_HXX_
#define X_EOBJECT_HXX_
#include "eobject.hxx"
#endif

#ifndef X_XMLNS_HXX_
#define X_XMLNS_HXX_
#include "xmlns.hxx"
#endif

#ifndef X_AVUNDO_HXX_
#define X_AVUNDO_HXX_
#include "avundo.hxx"
#endif

#ifdef UNIX
#include "mainwin.h"
extern "C" HANDLE MwGetPrimarySelectionData();
#include "quxcopy.hxx"
#endif

BEGIN_TEAROFF_TABLE(CElement, IProvideMultipleClassInfo)
    TEAROFF_METHOD(super, GetClassInfo, getclassinfo, (ITypeInfo ** ppTI))
    TEAROFF_METHOD(super, GetGUID, getguid, (DWORD dwGuidKind, GUID * pGUID))
    TEAROFF_METHOD(CElement, GetMultiTypeInfoCount, getmultitypeinfocount, (ULONG *pcti))
    TEAROFF_METHOD(CElement, GetInfoOfIndex, getinfoofindex, (ULONG iti, DWORD dwFlags, ITypeInfo** pptiCoClass, DWORD* pdwTIFlags, ULONG* pcdispidReserved, IID* piidPrimary, IID* piidSource))
END_TEAROFF_TABLE()

MtDefine(Elements, Mem, "Elements")
MtDefine(CElementCLock, Locals, "CElement::CLock")
MtDefine(CElementHitTestPoint_aryRects_pv, Locals, "CElement::HitTestPoint aryRects::_pv")
MtDefine(CElementHitTestPoint_aryElements_pv, Locals, "CElement::HitTestPoint aryElements::_pv")
MtDefine(CElementGetElementRc_aryRects_pv, Locals, "CElement::GetElementRc aryRects::_pv")

MtDefine(CMessage, Locals, "CMessage")

#define _cxx_
#include "types.hdl"

#define _cxx_
#include "element.hdl"

DeclareTagOther(tagFormatTooltips, "Format", "Show format indices with tooltips");
ExternTag(tagDisableLockAR);

ExternTag(tagHtmSrcTest);

class CAnchorElement;

// Each property which has a url image has an associated internal property
// which holds the cookie for the image stored in the cache.

static const struct {
    DISPID propID;          // url image property
    DISPID cacheID;         // internal cookie property
}

// make sure that DeleteImageCtx is modified, if any dispid's
// are added to this array
s_aryImgDispID[] = {
    { DISPID_A_BACKGROUNDIMAGE, DISPID_A_BGURLIMGCTXCACHEINDEX },
    { DISPID_A_LISTSTYLEIMAGE,  DISPID_A_LIURLIMGCTXCACHEINDEX }
};

extern "C" const IID IID_DataSource;

BEGIN_TEAROFF_TABLE_(CElement, IServiceProvider)
        TEAROFF_METHOD(CElement, QueryService, queryservice, (REFGUID guidService, REFIID riid, void **ppvObject))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_(CElement, IRecalcProperty)
    TEAROFF_METHOD(CElement, GetCanonicalProperty, getcanonicalproperty, (DISPID dispid, IUnknown **ppUnk, DISPID *pdispid))
END_TEAROFF_TABLE()

//  Default property page list for elements that don't have their own.
//  This gives them the allpage by default.

#ifndef NO_PROPERTY_PAGE
const CLSID * CElement::s_apclsidPages[] =
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

typedef void (CALLBACK* NOTIFYWINEVENTPROC)(UINT, HWND, LONG, LONG);
extern NOTIFYWINEVENTPROC g_pfnNotifyWinEvent;

//+------------------------------------------------------------------------
//
//  Class:     CMessage
//
//-------------------------------------------------------------------------

DWORD
FormsGetKeyState()
{
    static int vk[] =
    {
        VK_LBUTTON,     // MK_LBUTTON = 0x0001
        VK_RBUTTON,     // MK_RBUTTON = 0x0002
        VK_SHIFT,       // MK_SHIFT   = 0x0004
        VK_CONTROL,     // MK_CONTROL = 0x0008
        VK_MBUTTON,     // MK_MBUTTON = 0x0010
        VK_MENU,        // MK_ALT     = 0x0020
    };

    DWORD dwKeyState = 0;

    for (int i = 0; i < ARRAY_SIZE(vk); i++)
    {
        if (GetKeyState(vk[i]) & 0x8000)
        {
            dwKeyState |= (1 << i);
        }
    }

    return dwKeyState;
}

void
CMessage::CommonCtor()
{
    memset(this, 0, sizeof(*this));
    dwKeyState = FormsGetKeyState();
    resultsHitTest._cpHit = -1;
}

CMessage::CMessage(const CMessage *pMessage)
{
    if (pMessage)
    {
        memcpy(this, pMessage, sizeof(CMessage));
        // NEWTREE: we don't have subrefs on nodes...? regular add ref?
        if (pNodeHit)
            pNodeHit->NodeAddRef();
    }
}

CMessage::CMessage(const MSG * pmsg)
{
    CommonCtor();
    if (pmsg)
    {
        memcpy(this, pmsg, sizeof(MSG));
        htc = HTC_YES;
    }
}


CMessage::CMessage(
    HWND hwndIn,
    UINT msg,
    WPARAM wParamIn,
    LPARAM lParamIn)
{
    CommonCtor();
    hwnd      = hwndIn;
    message   = msg;
    wParam    = wParamIn;
    lParam    = lParamIn;
    htc       = HTC_YES;
    time      = GetMessageTime();
    DWORD  dw = GetMessagePos();
    MSG::pt.x = (short)LOWORD(dw);
    MSG::pt.y = (short)HIWORD(dw);

    Assert(!fEventsFired);
    Assert(!fSelectionHMCalled);
}

CMessage::~CMessage()
{
    // NEWTREE: same subref note here
    CTreeNode::ReplacePtr(&pNodeHit, NULL);
}

//+---------------------------------------------------------------------------
//
// Member : CMessage : SetNodeHit
//
//  Synopsis : things like the tracker cache the message and then access it
//  on a timer callback. inorder to ensure that the elements that are in the
//  message are still there, we need to sub(?) addref the element.
//
//----------------------------------------------------------------------------

void
CMessage::SetNodeHit( CTreeNode * pNodeHitIn )
{
    // NEWTREE: same subref note here
    CTreeNode::ReplacePtr( &pNodeHit, pNodeHitIn );
}

//+-----------------------------------------------------------------------------
//
//  Member : CMessage::SetElementClk
//
//  Synopsis : consolidating the click firing code requires a helper function
//      to set the click element member of the message structure.
//              this function should only be called from handling a mouse buttonup
//      event message which could fire off a click (i.e. LButton only)
//
//------------------------------------------------------------------------------
void
CMessage::SetNodeClk( CTreeNode * pNodeClkIn )
{
    CTreeNode * pNodeDown = NULL;

    Assert(pNodeClkIn && !pNodeClk || pNodeClkIn == pNodeClk);

    // get the element that this message is related to or if there
    // isn't one use the pointer passed in
    pNodeClk = (pNodeHit) ? pNodeHit : pNodeClkIn;

    Assert( pNodeClk );

    // if this is not a LButtonUP just return, using the value
    // set (i.e. we do not need to look for a common ancester)
    if ( message != WM_LBUTTONUP)
    {
        return;
    }

    // now go get the _pEltGotButtonDown from the doc and look for
    // the first common ancester between the two. this is the lowest
    // element that the mouse went down and up over.
    pNodeDown = pNodeClk->Element()->Doc()->_pNodeGotButtonDown;

    if (!pNodeDown)
    {
        // Button down was not on this doc, or cleared due to capture.
        // so this is not a click
        pNodeClk = NULL;
    }
    else
    {
        // Convert both nodes from from slave to master before comparison
        if (pNodeDown->Tag() == ETAG_TXTSLAVE)
        {
            pNodeDown = pNodeDown->GetMarkup()->Master()->GetFirstBranch();
        }
        if (pNodeClk->Tag() == ETAG_TXTSLAVE)
        {
            pNodeClk = pNodeClk->GetMarkup()->Master()->GetFirstBranch();
        }

        if (!pNodeDown)
            pNodeClk = NULL;
        if (!pNodeClk)
            return;

        if (pNodeDown != pNodeClk)
        {
            if (!pNodeDown->Element()->TestClassFlag(CElement::ELEMENTDESC_NOANCESTORCLICK))
            {
                // The mouse up is on a different element than us.  This
                // can only happen if someone got capture and
                // forwarded the message to us.  Now we find the first
                // common anscestor and fire the click event from
                // there.
                pNodeClk = pNodeDown->GetFirstCommonAncestor(pNodeClk, NULL);
            }
            else
                pNodeClk = NULL;
        }
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::CElement
//
//-------------------------------------------------------------------------

CElement::CElement (ELEMENT_TAG etag, CDoc *pDoc)
#if DBG == 1 || defined(DUMPTREE)
    : _nSerialNumber( CTreeNode::s_NextSerialNumber++ )
#endif
{
    __pvChain = pDoc;
    WHEN_DBG( _pDocDbg = pDoc );
    pDoc->SubAddRef();

    Assert( pDoc && pDoc->AreLookasidesClear( this, LOOKASIDE_ELEMENT_NUMBER ) );

    IncrementObjectCount(&_dwObjCnt);

    _etag = etag;
    WHEN_DBG( _etagDbg = etag );
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::~CElement
//
//-------------------------------------------------------------------------

CElement::~CElement()
{
    // NOTE:  Please cleanup in Passivate() if at all possible.
    //        Thread local storage can be deleted by the time this runs.
    Assert(!IsInMarkup());
}


//+------------------------------------------------------------------------
//
//  Member:     CElement::CLock::CLock
//
//  Synopsis:   Lock resources in CElement object.
//
//-------------------------------------------------------------------------

CElement::CLock::CLock(CElement *pElement, ELEMENTLOCK_FLAG enumLockFlags)
{
    Assert(enumLockFlags < (1 <<(sizeof(_wLockFlags)*8) ));//USHRT_MAX);

#if DBG==1
    extern BOOL g_fDisableBaseTrace;
    g_fDisableBaseTrace = TRUE;
#endif

    _pElement = pElement;
    if (_pElement)
    {
        _wLockFlags = pElement->_wLockFlags;
        pElement->_wLockFlags |= (WORD) enumLockFlags;
#if DBG==1
        if (!IsTagEnabled(tagDisableLockAR))
#endif
        {
            pElement->PrivateAddRef();
        }
    }

#if DBG==1
    g_fDisableBaseTrace = FALSE;
#endif

}


//+------------------------------------------------------------------------
//
//  Member:     CElement::CLock::~CLock
//
//  Synopsis:   Unlock resources in CElement object.
//
//-------------------------------------------------------------------------

CElement::CLock::~CLock()
{
#if DBG==1
    extern BOOL g_fDisableBaseTrace;
    g_fDisableBaseTrace = TRUE;
#endif

    if (_pElement)
    {
        _pElement->_wLockFlags = _wLockFlags;
#if DBG==1
        if (!IsTagEnabled(tagDisableLockAR))
#endif
        {
            _pElement->PrivateRelease();
        }
    }

#if DBG==1
    g_fDisableBaseTrace = FALSE;
#endif

}

//+------------------------------------------------------------------------
//
//  Member:     Passivate
//
//-------------------------------------------------------------------------

void
CElement::Passivate()
{
    CDoc * pDoc = Doc();

    // If we are in a tree, the tree will keep us alive
    Assert(!IsInMarkup());

    // Make sure we aren't on some delay release list somewhere
    Assert(!_fDelayRelease);

    // Make sure we don't have any pending event tasks on the view
    if (_fHasPendingEvent)
    {
        pDoc->GetView()->RemoveEventTasks(this);
    }

    // Destroy slave markup, if any
    if (HasSlaveMarkupPtr())
    {
        CMarkup * pMarkupSlave = DelSlaveMarkupPtr();

        pMarkupSlave->ClearMaster();
        pMarkupSlave->Release();
    }

    if (HasPeerHolder())
    {
        Assert (1 == GetPeerHolder()->_ulRefs);  // assert that we hold the last refcount on peer holder;
                                                    // no other object should hold refs on peer holder
        DelPeerHolder()->Release();              // delete the ptr and release peer holder
    }

    if (HasPeerMgr())
    {
        CPeerMgr::EnsureDeletePeerMgr(this, /* fForce = */ TRUE);
    }

#ifndef NO_DATABINDING
    if (HasDataBindPtr())
    {
        DetachDataBindings();
    }
#endif // ndef NO_DATABINDING

    if (pDoc->_pElementOMCapture == this)
    {
        releaseCapture();
    }

    GWKillMethodCall (this, NULL, 0);

    if (_fHasImage)
    {
        ReleaseImageCtxts();
    }

    // release layout engine if any.
    if(HasLayoutPtr())
    {
        Assert(_fSite);

        CLayout * pLayout = DelLayoutPtr();

        if (!pLayout->_fDetached)
            pLayout->Detach();

        pLayout->Release();
    }

    //delete the related accessible object if there is one
    if ( HasAccObjPtr() )
    {
        delete DelAccObjPtr();
    }

    if (_fHasPendingFilterTask)
        Doc()->RemoveFilterTask(this);

    if (Doc()->GetView()->HasRecalcTask(this))
        Doc()->GetView()->RemoveRecalcTask(this);

    if (_pAA)
    {
        // clear up the FiltersCollection

        if( _fHasFilterCollectionPtr )
        {
            delete GetFilterCollectionPtr();
        }

        // Kill the cached style pointer if present.  super::passivate
        // will delete the attribute array holding it.
        if (_pAA->IsStylePtrCachePossible())
        {
            delete GetInLineStylePtr();
            delete GetRuntimeStylePtr();
        }
#if DBG==1
        else
            Assert( !GetInLineStylePtr() && !GetRuntimeStylePtr() );
#endif
    }

    super::Passivate();

    // Ensure Lookaside cleanup.  Go directly to document to avoid bogus flags
    Assert(Doc()->GetLookasidePtr((DWORD *) this + LOOKASIDE_FILTER) == NULL);
    Assert(Doc()->GetLookasidePtr((DWORD *) this + LOOKASIDE_DATABIND) == NULL);
    Assert(Doc()->GetLookasidePtr((DWORD *) this + LOOKASIDE_PEER) == NULL);
    Assert(Doc()->GetLookasidePtr((DWORD *) this + LOOKASIDE_PEERMGR) == NULL);
    Assert(Doc()->GetLookasidePtr((DWORD *) this + LOOKASIDE_ACCESSIBLE) == NULL);
    Assert(Doc()->GetLookasidePtr((DWORD *) this + LOOKASIDE_SLAVEMARKUP) == NULL);

    pDoc->SubRelease();

    DecrementObjectCount(&_dwObjCnt);
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------

HRESULT
CElement::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    HRESULT hr;

    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IProvideMultipleClassInfo, NULL)
        QI_TEAROFF2(this, IProvideClassInfo, IProvideMultipleClassInfo, NULL)
        QI_TEAROFF2(this, IProvideClassInfo2, IProvideMultipleClassInfo, NULL)
        QI_TEAROFF((CBase *)this, ISpecifyPropertyPages, NULL)
        QI_TEAROFF((CBase *)this, IPerPropertyBrowsing, NULL)
        QI_TEAROFF(this, ISupportErrorInfo, NULL)
        QI_HTML_TEAROFF(this, IHTMLElement, NULL)
        QI_HTML_TEAROFF(this, IHTMLElement2, NULL)
        QI_TEAROFF(this, IHTMLUniqueName, NULL);
        QI_TEAROFF(this, IHTMLDOMNode, NULL);
        QI_TEAROFF(this, IObjectIdentity, NULL);
        QI_TEAROFF(this, IServiceProvider, NULL);
        QI_TEAROFF(this, IRecalcProperty, NULL);
        QI_CASE(IConnectionPointContainer)
        {
            if (iid == IID_IConnectionPointContainer)
            {
                *((IConnectionPointContainer **)ppv) =
                        new CConnectionPointContainer(this, NULL);

                if (!*ppv)
                    RRETURN(E_OUTOFMEMORY);

                SetEventsShouldFire();
            }
            break;
        }
#ifndef NO_DATABINDING
        QI_CASE(IHTMLDatabinding)
        {
            if (iid == IID_IHTMLDatabinding)
            {
                if (GetDBindMethods() == NULL)
                    RRETURN(E_NOINTERFACE);

                hr = THR(CreateTearOffThunk(this, s_apfnIHTMLDatabinding, NULL, ppv));
                if (hr)
                    RRETURN(hr);
            }
            break;
        }
#endif // ndef NO_DATABINDING

        default:
        {
            const CLASSDESC *pclassdesc = ElementDesc();

            if (iid == CLSID_CElement)
            {
                *ppv = this;    // Weak ref
                return S_OK;
            }

            // Primary default interface, or the non dual
            // dispinterface return the same object -- the primary interface
            // tearoff.
            if (pclassdesc &&
                pclassdesc->_apfnTearOff &&
                pclassdesc->_classdescBase._piidDispinterface &&
                (iid == *pclassdesc->_classdescBase._piidDispinterface ||
                 DispNonDualDIID(iid)))
            {
#ifndef WIN16
                hr = THR(CreateTearOffThunk(this, pclassdesc->_apfnTearOff, NULL, ppv, (void *)pclassdesc->_classdescBase._apHdlDesc->ppVtblPropDescs));
#else
                BYTE *pThis = ((BYTE *) (void *) ((CBase *) this)) - m_baseOffset;
                hr = THR(CreateTearOffThunk(pThis, (void *)(pclassdesc->_apfnTearOff), NULL, ppv));
#endif
                if (hr)
                    RRETURN(hr);

                break;
            }
            if (HasPeerHolder() &&
                IsEqualGUID(iid, IID_DataSource)    // please clear with alexz or anandra before adding
                                                    // any new interfaces to this list
                )
            {
                CPeerHolder *   pPeerHolder = GetPeerHolder();
                IUnknown *      pUnk;

                //
                // allow small set of interfaces to be delegated to identity behaviors;
                // thunk the interface pointers to element identity via peer holder to get correct refcounting
                //

                hr = THR_NOTRACE(pPeerHolder->QueryPeerInterfaceMulti(
                    iid, (void**)&pUnk, /* fIdentityOnly = */ TRUE));
                if (hr)
                    RRETURN (hr);

                hr = THR(::CreateTearOffThunk(
                    pUnk,
                    *(void **)pUnk,
                    NULL,
                    ppv,
                    pPeerHolder,
                    *(void **)(IUnknown *)pPeerHolder,
                    QI_MASK,
                    NULL));

                ReleaseInterface(pUnk);

                break;
            }
            else
            {
                hr = E_NOINTERFACE;
            }

            RRETURN (hr);

            break;
        }
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    (*(IUnknown **)ppv)->AddRef();

    DbgTrackItf(iid, "CElement", FALSE, ppv);

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::PrivateAddRef, IUnknown
//
//  Synopsis:   Private unknown AddRef.
//
//-------------------------------------------------------------------------
ULONG
CElement::PrivateAddRef()
{
    if( _ulRefs == 1 && IsInMarkup() )
    {
        Assert( GetMarkupPtr() );
        GetMarkupPtr()->AddRef();
    }

    return super::PrivateAddRef();
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::PrivateRelease, IUnknown
//
//  Synopsis:   Private unknown Release.
//
//-------------------------------------------------------------------------
ULONG
CElement::PrivateRelease()
{
    CMarkup * pMarkup = NULL;

    if( _ulRefs == 2 && IsInMarkup() )
    {
        Assert( GetMarkupPtr() );
        pMarkup = GetMarkupPtr();
    }

    ULONG ret =  super::PrivateRelease();

    if( pMarkup )
    {
        pMarkup->Release();
    }

    return ret;
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::PrivateEnterTree
//
//  Synopsis:   Ref counting fixup as tree entered.
//
//-------------------------------------------------------------------------
void
CElement::PrivateEnterTree()
{
    Assert( IsInMarkup() );
    super::PrivateAddRef();
    GetMarkupPtr()->AddRef();
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::PrivateExitTree
//
//  Synopsis:   Ref counting fixup as tree exited.
//
//-------------------------------------------------------------------------
void
CElement::PrivateExitTree( CMarkup * pMarkupOld)
{
    BOOL fReleaseMarkup = _ulRefs > 1;

    Assert( ! IsInMarkup() );
    Assert( pMarkupOld );

    // If we sent the EXITTREE_PASSIVATEPENDING bit then we
    // must also passivate right here.
    AssertSz( !_fPassivatePending || _ulRefs == 1, 
        "EXITTREE_PASSIVATEPENDING set and element did not passivate.  Talk to JBeda." );

    super::PrivateRelease();

    if ( fReleaseMarkup )
    {
        pMarkupOld->Release();
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     contains
//
//  Synopsis:   IHTMLElement method. returns a boolean  if PIelement is within
//              the scope of this
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CElement::contains(IHTMLElement * pIElement, VARIANT_BOOL *pfResult)
{
    CTreeNode     *pNode = NULL;
    HRESULT hr = S_OK;

    Assert ( pfResult );

    if ( !pfResult )
        goto Cleanup;

    *pfResult = VB_FALSE;
    if ( !pIElement )
        goto Cleanup;

    // get a CTreeNode pointer
    hr = THR(pIElement->QueryInterface(CLSID_CTreeNode, (void **)&pNode) );
    if ( hr == E_NOINTERFACE )
    {
        CElement *pElement;
        hr = THR(pIElement->QueryInterface(CLSID_CElement, (void **)&pElement) );
        if( hr )
            goto Cleanup;

        pNode = pElement->GetFirstBranch();
    }
    else if( hr )
        goto Cleanup;

    while (pNode &&
           DifferentScope(pNode, this))
    {
        // stop after the HTML tag
        if (pNode->Tag() == ETAG_ROOT)
            pNode = NULL;
        else
            pNode = pNode->Parent();
    }

    if ( pNode )
        *pfResult = VB_TRUE;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::ClearRunCaches(DWORD dwFlags)
{
    CMarkup * pMarkup = GetMarkup();

    if (pMarkup)
    {
        pMarkup->ClearRunCaches(dwFlags, this);
    }

    RRETURN(S_OK);
}

BOOL
CElement::IsFormatCacheValid()
{
    CTreeNode * pNode;
    
    pNode = GetFirstBranch();
    while (pNode)
    {
        if (!pNode->IsFancyFormatValid() ||
            !pNode->IsCharFormatValid() ||
            !pNode->IsParaFormatValid())
            return FALSE;
        pNode = pNode->NextBranch();
    }

    return TRUE;
}


HRESULT
CElement::EnsureFormatCacheChange ( DWORD dwFlags)
{
    HRESULT hr = S_OK;

    //
    // If we're not in the tree, it really isn't
    // very safe to do what we do.  Since putting
    // the element in the tree will call us again,
    // simply returning should be safe
    //
    if (GetFirstBranch() == 0)
        goto Cleanup;

    if ( dwFlags & (ELEMCHNG_CLEARCACHES | ELEMCHNG_CLEARFF) )
    {
        hr = THR(ClearRunCaches(dwFlags));
        if(hr)
            goto Cleanup;
    }

Cleanup:

    RRETURN( hr );
}

DeclareTag(tagPropChange, "Property changes", "OnPropertyChange");

//+------------------------------------------------------------------------
//
//  Member:     CElement::OnPropertyChange
//
//
//-------------------------------------------------------------------------

HRESULT
CElement::OnPropertyChange(DISPID dispid, DWORD dwFlags)
{
    HRESULT             hr = S_OK;
    BOOL                fHasLayout;
    BOOL                fYieldCurrency = FALSE;
    CTreeNode *         pNode = GetFirstBranch();
    CLayout   *         pLayoutOld;
    CLayout   *         pLayoutNew;
    CDoc *              pDoc = Doc();
    CCollectionCache *  pCollectionCache;
    CElement *          pElemCurrent = pDoc->_pElemCurrent;

    // if this is an event property that has just been hooked up then we need to 
    // start firing events for this element. we want to do this event if this 
    // element is not yet in the tree (i.e. no pNode) so that when it IS put into
    // the tree we can fire events.
    //
    // BUGBUG a good alternative implementation is to have this flag on the AttrArray.
    //  then with dword stored, we could have event level granularity and control.
    //  to support this all we need to do is change the implementation fo SetEventsShouldFire()
    // adn ShouldEventsFire() to use the AA.
    if (dispid >= DISPID_EVENTS && dispid < DISPID_EVENTS + DISPID_EVPROPS_COUNT)
    {
        SetEventsShouldFire();

        // don't expose this to the outside.
        if ( dispid == DISPID_EVPROP_ONATTACHEVENT)
            goto Cleanup;
    }

    if(!IsInMarkup() || !pNode)
        goto Cleanup;

    Verify(OpenView());

    TraceTag((tagPropChange, "Property changed, flags:%ld", dwFlags));

    if (DISPID_A_BEHAVIOR == dispid || DISPID_CElement_className == dispid || DISPID_UNKNOWN == dispid)
    {
        if (DISPID_A_BEHAVIOR == dispid)
        {
            pDoc->SetPeersPossible();
        }

        hr = THR(ProcessPeerTask(PEERTASK_RECOMPUTEBEHAVIORS));
        if (hr)
            goto Cleanup;
    }

    pLayoutOld   = GetUpdatedLayout();

    // some changes invalidate collections
    if ( dwFlags & ELEMCHNG_UPDATECOLLECTION )
    {
        // BUGBUG rgardner, for now Inval all the collections, whether they are filtered on property values
        // or not. We should tweak the PDL code to indicate which collections should be invaled, or do
        // this intelligently through some other mechanism, we should tweak this
        // when we remove the all collection.
        Assert(IsInMarkup());

        pCollectionCache = GetMarkup()->CollectionCache();
        if (pCollectionCache)
            pCollectionCache->InvalidateAllSmartCollections();

        // Clear this flag: exclusive or
        dwFlags ^= ELEMCHNG_UPDATECOLLECTION;
    }

    switch(dispid)
    {
    case DISPID_A_BACKGROUNDIMAGE:
    case DISPID_A_LISTSTYLEIMAGE:
        // Release any dispid's which hold image contexts
        DeleteImageCtx(dispid);
        break;
    }

    // Make sure our caches are up to date - if we haven't already
    hr = THR(EnsureFormatCacheChange ( dwFlags ));
    if ( hr )
        goto Cleanup;

    fHasLayout = NeedsLayout();
    pLayoutNew = GetUpdatedNearestLayout();

    if (!pLayoutNew || !pLayoutNew->ElementOwner()->IsInMarkup())
        goto Cleanup;

    switch (dispid)
    {
    case DISPID_A_POSITION:
        //BUGBUG tomfakes  EnsureInMarkup?
        Assert(IsInMarkup());

        pCollectionCache = GetMarkup()->CollectionCache();
        if (pCollectionCache)
            pCollectionCache->InvalidateItem(CMarkup::REGION_COLLECTION);

        SendNotification(NTYPE_ZPARENT_CHANGE);
        break;

    case STDPROPID_XOBJ_LEFT:
    case STDPROPID_XOBJ_RIGHT:
        {
            const CFancyFormat * pFF = GetFirstBranch()->GetFancyFormat();

            //
            // if width is auto, left & right control the width of the
            // element if it is absolute, so fire a resize instead of
            // reposition
            //
            if (    pFF->IsAbsolute()
                &&  pFF->IsWidthAuto())
            {
                dwFlags |= ELEMCHNG_SIZECHANGED;
            }
            else if (!pFF->IsPositionStatic())
            {
                RepositionElement();
            }

            if (dispid == STDPROPID_XOBJ_LEFT)
            {
                IGNORE_HR(FireOnChanged(DISPID_IHTMLSTYLE_PIXELLEFT));
                IGNORE_HR(FireOnChanged(DISPID_IHTMLSTYLE_POSLEFT));
            }
            else
            {
                IGNORE_HR(FireOnChanged(DISPID_IHTMLSTYLE2_PIXELRIGHT));
                IGNORE_HR(FireOnChanged(DISPID_IHTMLSTYLE2_POSRIGHT));
            }
        }
        break;

    case STDPROPID_XOBJ_TOP:
    case STDPROPID_XOBJ_BOTTOM:
        {
            const CFancyFormat * pFF = GetFirstBranch()->GetFancyFormat();

            //
            // if height is auto, and the bottom is not auto then the size
            // of the element if absolute can change based on it's top & bottom.
            // So, fire a resize instead of reposition. If bottom is auto,
            // then the element is sized to content.
            //
            if (    pFF->IsAbsolute()
                &&  pFF->IsHeightAuto()
                &&  (dispid != STDPROPID_XOBJ_TOP || !pFF->IsBottomAuto()))
            {
                dwFlags |= ELEMCHNG_SIZECHANGED;
            }
            else if (!pFF->IsPositionStatic())
            {
                RepositionElement();
            }

            if (dispid == STDPROPID_XOBJ_TOP)
            {
                IGNORE_HR(FireOnChanged(DISPID_IHTMLSTYLE_PIXELTOP));
                IGNORE_HR(FireOnChanged(DISPID_IHTMLSTYLE_POSTOP));
            }
            else
            {
                IGNORE_HR(FireOnChanged(DISPID_IHTMLSTYLE2_PIXELBOTTOM));
                IGNORE_HR(FireOnChanged(DISPID_IHTMLSTYLE2_POSBOTTOM));
            }
        }
        break;

    case STDPROPID_XOBJ_WIDTH:
        IGNORE_HR(FireOnChanged(DISPID_IHTMLSTYLE_PIXELWIDTH));
        IGNORE_HR(FireOnChanged(DISPID_IHTMLSTYLE_POSWIDTH));
        break;

    case STDPROPID_XOBJ_HEIGHT:
        IGNORE_HR(FireOnChanged(DISPID_IHTMLSTYLE_PIXELHEIGHT));
        IGNORE_HR(FireOnChanged(DISPID_IHTMLSTYLE_POSHEIGHT));
        break;

    case DISPID_A_ZINDEX:
        if (!IsPositionStatic())
        {
            ZChangeElement();
        }

        pDoc->FixZOrder();
        break;


    case DISPID_CElement_accessKey:
        // if accessKey property is changed, always insert it into
        // CDoc::_aryAccessKeyItems, we are not considering removing CElements
        // from CDoc::_aryAccessKeyItems as of now
        //
        if (pDoc->_aryAccessKeyItems.Find(this) < 0)
        {
            hr = pDoc->InsertAccessKeyItem(this);
            if (hr)
                goto Cleanup;
        }
        break;

    case DISPID_CElement_tabIndex:
        hr = OnTabIndexChange();
        break;
    
    case DISPID_CElement_disabled:
        // if (fHasLayout)
        {
            BOOL        fEnabled    = !GetAAdisabled();
            CElement *  pNewDefault = 0;
            CElement *  pOldDefault = 0;
            CElement *  pSavDefault = 0;

            // we should not be the default button before becoming enabled
            Assert(!(fEnabled && _fDefault));

            if (!_fDefault && fEnabled)
            {
                // this is not the previous default button
                // look for it
                pOldDefault = FindDefaultElem(TRUE);
            }

            if (pDoc->_pElemCurrent == this && !fEnabled)
            {
                // this is the case where the button disables itself
                pOldDefault = this;
            }

            // if this element act like a button
            // becomes disabled or enabled
            // we need to make sure this is recorded in the cached
            // default element of the doc or form
            if (TestClassFlag(CElement::ELEMENTDESC_DEFAULT))
            {
                // try to find a new default
                // set _fDefault to FALSE in order to avoid
                // FindDefaultLayout returning this site again.
                // because FindDefaultLayout will use _fDefault
                _fDefault = FALSE;
                pSavDefault = FindDefaultElem(TRUE, TRUE);
                if (pSavDefault == this || !fEnabled)
                {
                    CFormElement    *pForm = GetParentForm(); 
                    if (pSavDefault)
                    {
                        pSavDefault->_fDefault = TRUE;
                    }
                    if (pForm)
                        pForm->_pElemDefault = pSavDefault;
                    else
                        pDoc->_pElemDefault = pSavDefault;
                }
            }
            if (!pDoc->_pElemCurrent->_fActsLikeButton || _fDefault)
            {
                _fDefault = FALSE;
                pNewDefault = pSavDefault ? pSavDefault : FindDefaultElem(TRUE, TRUE);
            }

            // if this was the default, and now becoming disabled
            _fDefault = FALSE;

            if (pNewDefault != pOldDefault)
            {
                if (pOldDefault)
                {
                    CNotification   nf;

                    nf.AmbientPropChange(pOldDefault, (void *)DISPID_AMBIENT_DISPLAYASDEFAULT);

                    // refresh the old button
                    pOldDefault->_fDefault = FALSE;
                    pOldDefault->Notify(&nf);
                    pOldDefault->Invalidate();
                }

                if (pNewDefault)
                {
                    CNotification   nf;

                    Assert(pNewDefault->_fActsLikeButton);
                    nf.AmbientPropChange(pNewDefault, (void *)DISPID_AMBIENT_DISPLAYASDEFAULT);
                    pNewDefault->_fDefault = TRUE;
                    pNewDefault->Notify(&nf);
                    pNewDefault->Invalidate();
                }
            }
        }
        break;

    case DISPID_A_VISIBILITY:
        //
        //  Notify element and all descendents of the change
        //

        SendNotification(NTYPE_VISIBILITY_CHANGE);

        //
        //  If the element is being hidden, ensure it and any descendent which inherits visibility are the current element
        //  (Do this by, within this routine, pretending that this element is the current element)
        //

        fYieldCurrency = !!pNode->GetCharFormat()->IsVisibilityHidden();

        if (    fYieldCurrency
            &&  pElemCurrent->GetFirstBranch()->SearchBranchToRootForScope(this)
            &&  pElemCurrent->GetFirstBranch()->GetCharFormat()->IsVisibilityHidden())
        {
            pElemCurrent = this;
        }
        break;

    case DISPID_A_DISPLAY:
        //
        //  If the element is being hidden, ensure it and none of its descendents are the current element
        //  (Do this by, within this routine, pretending that this element is the current element)
        //

        if (pNode->GetCharFormat()->IsDisplayNone())
        {
            fYieldCurrency = TRUE;

            if (pElemCurrent->GetFirstBranch()->SearchBranchToRootForScope(this))
            {
                pElemCurrent = this;
            }
        }
#ifdef NEVER
        else
        {
            dwFlags |= ELEMCHNG_REMEASUREALLCONTENTS;
        }
#endif
        break;

    case DISPID_A_MARGINTOP:
    case DISPID_A_MARGINLEFT:
    case DISPID_A_MARGINRIGHT:
    case DISPID_A_MARGINBOTTOM:
        if (Tag() == ETAG_BODY || Tag() == ETAG_FRAMESET)
        {
            dwFlags |= ELEMCHNG_REMEASURECONTENTS;
            dwFlags &= ~(ELEMCHNG_SIZECHANGED | ELEMCHNG_REMEASUREINPARENT);
        }
        break;
    case DISPID_A_CLIP:
    case DISPID_A_CLIPRECTTOP:
    case DISPID_A_CLIPRECTRIGHT:
    case DISPID_A_CLIPRECTBOTTOM:
    case DISPID_A_CLIPRECTLEFT:
        if (fHasLayout)
        {
            CDispNode* pDispNode = pLayoutNew->GetElementDispNode(this);
            if (pDispNode)
            {
                if (pDispNode->HasUserClip())
                {
                    CSize size;

                    pLayoutNew->GetSize(&size);
                    pLayoutNew->SizeDispNodeUserClip(&(pDoc->_dci), size);
                }

                // we need to create a display node that can have user
                // clip information, and ResizeElement will force this
                // node to be created.  Someday, we might be able to morph
                // the existing display node for greater efficiency.
                else
                {
                    ResizeElement();
                }
            }
        }
        break;
    }

    //
    // Notify the layout of the property change, layout fixes up
    // its display node to handle visibility/background changes.
    //
    if(fHasLayout)
        pLayoutNew->OnPropertyChange(dispid, dwFlags);

    if(     (pLayoutOld && !fHasLayout)
        ||  (!pLayoutOld && fHasLayout))
    {
        if(this == pDoc->_pElemCurrent)
        {
            pLayoutNew->ElementOwner()->BecomeCurrent(pDoc->_lSubCurrent);
        }

        dwFlags |= ELEMCHNG_REMEASUREINPARENT;
        dwFlags &= ~ELEMCHNG_SIZECHANGED;
    }

    if (dwFlags & (ELEMCHNG_REMEASUREINPARENT | ELEMCHNG_SIZECHANGED))
    {
        MinMaxElement();
    }

    if (dwFlags & ELEMCHNG_REMEASUREINPARENT)
    {
        RemeasureInParentContext();
    }
    else if (   dwFlags & ELEMCHNG_SIZECHANGED
            ||  (!fHasLayout && dwFlags & ELEMCHNG_RESIZENONSITESONLY))
    {
        ResizeElement();
    }

    if (dwFlags & (ELEMCHNG_REMEASURECONTENTS | ELEMCHNG_REMEASUREALLCONTENTS))
    {
        RemeasureElement( dwFlags & ELEMCHNG_REMEASUREALLCONTENTS
                            ? NFLAGS_FORCE
                            : 0);
    }

    // we need to send the display change notification after sending
    // the RemeasureInParent notification. RemeasureInParent marks the
    // ancestors dirty, therefore any ZParentChange notifications fired
    // from DisplayChange are queued up until the ZParent is calced.
    if (    dispid == DISPID_A_DISPLAY
        ||  dispid == DISPID_CElement_className
        ||  dispid == DISPID_UNKNOWN)
    {
        CNotification   nf;

        nf.DisplayChange(this);

        SendNotification(&nf);
    }

    if (this == pElemCurrent)
    {
        if (    !IsEnabled()
            ||  fYieldCurrency)
        {
            IGNORE_HR(pNode->Parent()->Element()->BubbleBecomeCurrentAndActive());
        }

        else if (   dispid == DISPID_A_VISIBILITY
                ||  dispid == DISPID_A_DISPLAY)
        {
            pDoc->GetView()->SetFocus(pDoc->_pElemCurrent, pDoc->_lSubCurrent);
        }
    }

    // Invalidation on z-index changes will be handled by FixZOrder in CSite
    // unless the change is on a non-site
    if (    pDoc->_state >= OS_INPLACE
        &&  !(dispid == DISPID_A_ZINDEX && fHasLayout))
    {
        if (    (dwFlags & (ELEMCHNG_SITEREDRAW | ELEMCHNG_CLEARCACHES | ELEMCHNG_CLEARFF))
            &&  !(dwFlags & ELEMCHNG_SITEPOSITION && fHasLayout))
        {
            Invalidate();

            // Invalidate() sends a notification ot the parent, however in OPC
            // descendant elemetns may inherit what has just changed, so we need
            // a notification that goes down to the positioned children
            // so that they know to invalidate.  This is necessary since
            // when a property is changed they may have a change in an 
            // inherited value that they need to display
            //
            //  if we wind up needing this notification fired from other 
            // places, consider moving it into InvalidateElement()
            SendNotification(NTYPE_ELEMENT_INVAL_Z_DESCENDANTS);
        }
    }

    // once we start firing events, the pLayout Old is not reliable since script can
    // change the page.


    IGNORE_HR(FireOnChanged(dispid));

    pDoc->OnDataChange();

    // fire the onpropertychange script event, but only if it is possible
    //   that someone is actually listeing. otherwise, don't waste the time.
    //
    if ( ShouldFireEvents() && DISPID_UNKNOWN != dispid)
    {
        hr = THR(Fire_PropertyChangeHelper(dispid, dwFlags));
        if ( hr )
            goto Cleanup;
    }

    // Accessibility state change check and event firing
    if ( ( dwFlags & ELEMCHNG_ACCESSIBILITY ) &&
            HasAccObjPtr() )
    {
        // fire accesibility state change event.
        hr = pDoc->FireAccesibilityEvents( NULL, GetSourceIndex() );
    }


    dwFlags &= ~(ELEMCHNG_CLEARCACHES | ELEMCHNG_CLEARFF);

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:     OpenView
//
//  Synopsis:   Open the view associated with the element - That view is the one
//              associated with the nearest layout
//
//  Returns:    TRUE if the view was successfully opened, FALSE if we're in the
//              middle of rendering
//
//-----------------------------------------------------------------------------
BOOL
CElement::OpenView()
{
    CLayout *   pLayout = GetCurNearestLayout();
    return pLayout ? pLayout->OpenView() : Doc()->OpenView();
}


//+----------------------------------------------------------------------------
//
//  Member:     InvalidateElement
//              MinMaxElement
//              ResizeElement
//              RemeasureElement
//              RemeasureInParentContext
//              RepositionElement
//              ZChangeElement
//
//  Synopsis:   Notfication send helpers
//
//  Arguments:  grfFlags - NFLAGS_xxxx flags
//
//-----------------------------------------------------------------------------
void
CElement::InvalidateElement(DWORD grfFlags)
{
    // this notification goes up to the parent to request that 
    // we be invalidated
    SendNotification(NTYPE_ELEMENT_INVALIDATE, grfFlags);
}

void
CElement::MinMaxElement(DWORD grfFlags)
{
    CLayout *   pLayout = GetCurLayout();

    if (    pLayout
        &&  pLayout->_fMinMaxValid)
    {
        SendNotification(NTYPE_ELEMENT_MINMAX, grfFlags);
    }
}

void
CElement::ResizeElement(DWORD grfFlags)
{
    //
    //  Resize notifications are only fired when:
    //    a) The element does not have a layout (and must always notify its container) or
    //    b) It has a layout, but it's currently "clean" and
    //    c) The element is not presently being sized by its container
    //
    CLayout * pLayout = GetCurLayout();

    if (    !pLayout
        ||  (   !pLayout->_fSizeThis
            &&  !pLayout->IsCalcingSize()))
    {
            SendNotification(NTYPE_ELEMENT_RESIZE, grfFlags);
    }
}

void
CElement::RemeasureInParentContext(DWORD grfFlags)
{
    SendNotification(NTYPE_ELEMENT_RESIZEANDREMEASURE, grfFlags);
}

void
CElement::RemeasureElement(DWORD grfFlags)
{
    SendNotification(NTYPE_ELEMENT_REMEASURE, grfFlags);
}

void
CElement::RepositionElement(DWORD grfFlags)
{
    Assert(!IsPositionStatic());
    SendNotification(NTYPE_ELEMENT_REPOSITION, grfFlags);
}

void
CElement::ZChangeElement(DWORD grfFlags, CPoint * ppt)
{
    CMarkup *       pMarkup = GetMarkup();
    CNotification   nf;

    Assert(!IsPositionStatic() || GetFirstBranch()->GetCharFormat()->_fRelative);
    Assert(pMarkup);

    nf.Initialize(NTYPE_ELEMENT_ZCHANGE, this, GetFirstBranch(), NULL, grfFlags);
    if (ppt)
        nf.SetData(*ppt);

    pMarkup->Notify(nf);
}


//+----------------------------------------------------------------------------
//
//  Member:     SendNotification
//
//  Synopsis:   Send a notification associated with this element
//
//-----------------------------------------------------------------------------

void
CElement::SendNotification(CNotification *pNF)
{
    CMarkup * pMarkup = GetMarkup();

    if (pMarkup)
    {
        pMarkup->Notify(pNF);
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     SendNotification
//
//  Synopsis:   Send a notification associated with this element
//
//  Arguments:  ntype    - NTYPE_xxxxx flag
//              grfFlags - NFLAGS_xxxx flags
//
//-----------------------------------------------------------------------------
void
CElement::SendNotification(
    NOTIFYTYPE  ntype,
    DWORD       grfFlags,
    void *      pvData)
{
    CMarkup *   pMarkup = GetMarkup();

    if ( pMarkup )
    {
        CNotification   nf;

        Assert( GetFirstBranch() );

        nf.Initialize(ntype, this, GetFirstBranch(), pvData, grfFlags);

        pMarkup->Notify(nf);
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     DirtyLayout
//
//  Synopsis:   Dirty the layout engine associated with an element
//
//-----------------------------------------------------------------------------
void
CElement::DirtyLayout(
    DWORD   grfLayout)
{
    if (HasLayout())
    {
        CLayout *   pLayout = GetCurLayout();

        Assert(pLayout);

        pLayout->_fSizeThis = TRUE;

        if (grfLayout & LAYOUT_FORCE)
        {
            pLayout->_fForceLayout = TRUE;
        }
    }
}

#ifdef WIN16
#pragma code_seg ("ELEMENT_2_TEXT")
#endif

//+---------------------------------------------------------------------------
//
//  Member:     CElement::HitTestPoint, public
//
//  Synopsis:   Determines if this element is hit by the given point
//
//  Arguments:  [pt]        -- Point to check against.
//              [ppSite]    -- If there's a hit, the site that was hit.
//              [ppElement] -- If there's a hit, the element that was hit.
//              [dwFlags]   -- HitTest flags.
//
//  Returns:    HTC
//
//  Notes:      Only ever returns a hit if this element is a relatively
//              positioned element.
//
//              BUGBUG -- This will be changed to call CTxtSite soon. (lylec)
//
//----------------------------------------------------------------------------

HTC
CElement::HitTestPoint(CMessage*    pMessage,
                       CTreeNode ** ppNodeElement,
                       DWORD        dwFlags)
{
    CLayout *   pLayout = GetUpdatedNearestLayout();
    HTC         htc     = HTC_NO;

    if (pLayout)
    {
        CDispNode * pDispNodeOut = NULL;
        CView *     pView        = pLayout->GetView();
        POINT       ptContent;

        if (pView)
        {
            *ppNodeElement = GetFirstBranch();

            htc = pLayout->GetView()->HitTestPoint(
                                                pMessage->pt,
                                                COORDSYS_GLOBAL,
                                                this,
                                                dwFlags,
                                                &pMessage->resultsHitTest,
                                                ppNodeElement,
                                                ptContent,
                                                &pDispNodeOut);
        }
    }

    return htc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetRange
//
//  Synopsis:   Returns the range of char's under this element including the
//              end nodes
//
//----------------------------------------------------------------------------

HRESULT
CElement::GetRange(long * pcpStart, long * pcch)
{
    CTreePos *ptpStart, *ptpEnd;

    GetTreeExtent(&ptpStart, &ptpEnd);

    //
    // The range returned include the WCH_NODE characters for the element
    //
    *pcpStart = ptpStart->GetCp();
    *pcch = ptpEnd->GetCp() - *pcpStart + 1;
    return(S_OK);
}


//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetFocusShape
//
//  Synopsis:   Returns the shape of the focus outline that needs to be drawn
//              when this element has focus. This function creates a new
//              CShape-derived object. It is the caller's responsibility to
//              release it.
//
//  Notes:      This default implementation assumes that the element encloses
//              a text range (e.g. anchor, label). Other elements (buttons,
//              body, image, checkbox, radio button, input file, image map
//              area) must override this function to supply the correct shape.
//
//
//----------------------------------------------------------------------------

HRESULT
CElement::GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape)
{
    HRESULT hr = S_FALSE;

    Assert(ppShape);

    *ppShape = NULL;

    // By default, provide focus shape only for 
    // 1) elements that had focus shape in IE4 (compat)
    // 2) elements that have a tab index specified (#40434)
    //
    // In IE6, we will introduce a CSS style that lets elements override this
    // this behavior to turn on/off focus shapes.

    if (GetAAtabIndex() < 0)
    {
        switch (Tag())
        {
        case ETAG_A:
        case ETAG_LABEL:
        case ETAG_IMG:
            break;
        default:
            goto Cleanup;
        }
    }

    {
    }

    if (HasLayout())
    {
        CRect       rc;
        CLayout *   pLayout = GetCurLayout();

        if (!pLayout)
            goto Cleanup;

        pLayout->GetClientRect(&rc);
        if (rc.IsEmpty())
            goto Cleanup;

        CRectShape *pShape = new CRectShape;
        if (!pShape)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        pShape->_rect = rc;
        *ppShape = pShape;

        hr = S_OK;
    }
    else
    {
        long            cpStart, cch;
        CFlowLayout *   pFlowLayout     = GetFlowLayout();

        if (!pFlowLayout)
            goto Cleanup;

        cch = GetFirstAndLastCp( &cpStart, NULL );
        // GetFirstAndLast gets a cpStart + 1. We need the element's real cpStart
        // Also, the cch is less than the real cch by 2. We need to add this back
        // in so RegionFromElement can get the correct width of this element.
        hr = THR(pFlowLayout->GetDisplay()->GetWigglyFromRange(pdci, cpStart - 1, cch + 2, ppShape));
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+------------------------------------------------------------------------
//
//  CElement::GetImageUrlCookie
//
//  Returns a Adds the specified URL to the url image cache on the doc
//
//-------------------------------------------------------------------------

HRESULT
CElement::GetImageUrlCookie(LPCTSTR lpszURL, LONG *plCtxCookie)
{
    HRESULT         hr = S_OK;
    CDoc *          pDoc = Doc();
    LONG            lNewCookie = 0;

    // Element better be in the tree when this function is called
    Assert (pDoc);

    if (lpszURL && *lpszURL)
    {
        hr = pDoc->AddRefUrlImgCtx(lpszURL, this, &lNewCookie);
        if(hr)
            goto Cleanup;
    }

Cleanup:
    *plCtxCookie = lNewCookie;

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  CElement::AddImgCtx
//
//  Adds the info specified in ImgCtxInfo on the attr array, releasing
//  the current value if there is one.
//
//-------------------------------------------------------------------------

HRESULT
CElement::AddImgCtx(DISPID dispID, LONG lCookie)
{
    HRESULT hr = S_OK;
    CDoc    * pDoc = Doc();
    AAINDEX iCookieIndex;

    iCookieIndex = FindAAIndex(dispID, CAttrValue::AA_Internal);

    if (iCookieIndex != AA_IDX_UNKNOWN)
    {
        // Remove the current entry
        DWORD dwCookieOld = 0;

        if (GetSimpleAt(iCookieIndex, &dwCookieOld) == S_OK)
        {
            pDoc->ReleaseUrlImgCtx(LONG(dwCookieOld), this);
        }
    }

    hr = THR(AddSimple(dispID, DWORD(lCookie), CAttrValue::AA_Internal));

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:   ReleaseImageCtxts
//
//  Synopsis:   Finds any image contexts associated with this element,
//              and frees them up.  Cookies can be held for LI bullets
//              and for background images.
//
//-----------------------------------------------------------------------------

void
CElement::ReleaseImageCtxts()
{
    CDoc * pDoc = Doc();
    AAINDEX iCookieIndex;
    DWORD   dwCookieOld = 0;
    int     n;

    if (!_fHasImage)
        return;         // nothing to do, bail

    for (n = 0; n < ARRAY_SIZE(s_aryImgDispID); ++n)
    {
        // Check for a bg url image cookie in the standard attr array
        iCookieIndex = FindAAIndex(s_aryImgDispID[n].cacheID,
                                   CAttrValue::AA_Internal);

        if (iCookieIndex != AA_IDX_UNKNOWN &&
            GetSimpleAt(iCookieIndex, &dwCookieOld) == S_OK)
        {
            pDoc->ReleaseUrlImgCtx((LONG)dwCookieOld, this);
        }
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   DeleteImageCtx
//
//  Synopsis:   Finds any image contexts associated with this element,
//              corresponding to the dispid and free it up.  Cookies
//              can be held for LI bullets and for background images.
//
//-----------------------------------------------------------------------------

void
CElement::DeleteImageCtx(DISPID dispid)
{
    CDoc *      pDoc = Doc();
    CAttrArray* pAA;

    if (_fHasImage && (pAA = *GetAttrArray()) != NULL)
    {
        int n;
        for (n = 0; n < ARRAY_SIZE(s_aryImgDispID); ++n)
        {
            if (dispid == s_aryImgDispID[n].propID)
            {
                long lCookie;

                if ( pAA->FindSimpleInt4AndDelete(s_aryImgDispID[n].cacheID,
                                               (DWORD *)&lCookie, CAttrValue::AA_Internal) )
                {
                    // Release UrlImgCtxCacheEntry
                    pDoc->ReleaseUrlImgCtx(lCookie, this);
                }
                break;
            }
        }
    }
}

//+------------------------------------------------------------------------
//
//  Member:     GetSourceIndex
//
//-------------------------------------------------------------------------


long
CElement::GetSourceIndex ()
{
    CTreeNode * pNodeCurr;

    if ( Tag() == ETAG_ROOT )
        return -1;

    pNodeCurr = GetFirstBranch();
    if ( !pNodeCurr )
        return -1;
    else
    {
        Assert( !pNodeCurr->GetBeginPos()->IsUninit() );
        return pNodeCurr->GetBeginPos()->SourceIndex()-1; // subtract one because of ETAG_ROOT
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CompareZOrder
//
//  Synopsis:   Compare the z-order of two elements
//
//  Arguments:  pElement - The CElement to compare against
//
//  Returns:    Greater than zero if this element is greater
//              Less than zero if this element is less
//              Zero if they are equal
//
//----------------------------------------------------------------------------

long
CElement::CompareZOrder(
    CElement *  pElement)
{
    long    lCompare;

    lCompare = GetFirstBranch()->GetCascadedzIndex() - pElement->GetFirstBranch()->GetCascadedzIndex();

    if (!lCompare)
    {
        lCompare = GetSourceIndex() - pElement->GetSourceIndex();
    }

    return lCompare;
}


//+------------------------------------------------------------------------
//
//  Member:     GetTreeExtent
//
//  Synopsis:   Return the edge node pos' for this element.  Pretty
//              much just walks the context chain and gets the first
//              and last node pos'.
//
//-------------------------------------------------------------------------
void
CElement::GetTreeExtent(
    CTreePos ** pptpStart,
    CTreePos ** pptpEnd )
{
    CTreeNode * pNodeCurr = GetFirstBranch();

    if (pptpStart)
        *pptpStart = NULL;

    if (pptpEnd)
        *pptpEnd = NULL;

    if (!pNodeCurr)
        goto Cleanup;

    Assert(     ! pNodeCurr->GetBeginPos()->IsUninit()
            &&  ! pNodeCurr->GetEndPos()->IsUninit() );

    if (pptpStart)
    {
        *pptpStart = pNodeCurr->GetBeginPos();

        Assert( *pptpStart );
        Assert( (*pptpStart)->IsBeginNode() && (*pptpStart)->IsEdgeScope() );
        Assert( (*pptpStart)->Branch() == pNodeCurr );
    }

    if (pptpEnd)
    {
        while( pNodeCurr->NextBranch() )
            pNodeCurr = pNodeCurr->NextBranch();

        Assert( pNodeCurr );

        *pptpEnd = pNodeCurr->GetEndPos();

        Assert( *pptpEnd );
        Assert( (*pptpEnd)->IsEndNode() && (*pptpEnd)->IsEdgeScope() );
        Assert( (*pptpEnd)->Branch() == pNodeCurr );
    }

Cleanup:
    return;
}

//+------------------------------------------------------------------------
//
//  Member:     GetLastBranch
//
//  Synopsis:   Like GetFirstBranch, but gives the last one.
//
//-------------------------------------------------------------------------
CTreeNode *
CElement::GetLastBranch()
{
    CTreeNode *pNode = GetFirstBranch();
    CTreeNode *pNodeLast = pNode;

    while (pNode)
    {
        pNodeLast = pNode;
        pNode = pNode->NextBranch();
    }

    return pNodeLast;
}

//+------------------------------------------------------------------------
//
//  Static Member:  CElement::ReplacePtr, CElement::ClearPtr
//
//  Synopsis:   Do a CElement* assignment, but worry about refcounts
//
//-------------------------------------------------------------------------

void
CElement::ReplacePtr ( CElement * * pplhs, CElement * prhs )
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

//+------------------------------------------------------------------------
//
//  Static Member:  CElement::ReplacePtrSub, CElement::ClearPtr
//
//  Synopsis:   Do a CElement* assignment, but worry about weak refcounts
//
//-------------------------------------------------------------------------

void
CElement::ReplacePtrSub ( CElement * * pplhs, CElement * prhs )
{
    if (pplhs)
    {
        if (prhs)
        {
            prhs->SubAddRef();
        }
        if (*pplhs)
        {
            (*pplhs)->SubRelease();
        }
        *pplhs = prhs;
    }
}

void
CElement::SetPtr ( CElement ** pplhs, CElement * prhs )
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
CElement::StealPtrSet ( CElement ** pplhs, CElement * prhs )
{
    SetPtr( pplhs, prhs );

    if (pplhs && *pplhs)
        (*pplhs)->Release();
}

void
CElement::StealPtrReplace ( CElement ** pplhs, CElement * prhs )
{
    ReplacePtr( pplhs, prhs );

    if (pplhs && *pplhs)
        (*pplhs)->Release();
}

void
CElement::ClearPtr ( CElement * * pplhs )
{
    if (pplhs && * pplhs)
    {
        CElement * pElement = *pplhs;
        *pplhs = NULL;
        pElement->Release();
    }
}

void
CElement::ReleasePtr ( CElement * pElement )
{
    if (pElement)
    {
        pElement->Release();
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     CElement::IsBlockElement
//
//  Synopsis:   Describes whether or not this node is a block element
//
//  Returns:    BOOL indicating a block element
//
//-----------------------------------------------------------------------------

BOOL
CElement::IsBlockElement ( )
{
    CTreeNode * pTreeNode = GetFirstBranch();

    if (pTreeNode->_iFF == -1)
        pTreeNode->GetFancyFormat();

    return BOOL( pTreeNode->_fBlockNess );
}

//+----------------------------------------------------------------------------
//
//  Member:     CElement::IsOwnLineElement
//
//  Synopsis:   Tells us if the element is a ownline element
//
//  Returns:    BOOL indicating an ownline element
//
//-----------------------------------------------------------------------------
BOOL
CElement::IsOwnLineElement(CFlowLayout *pFlowLayoutContext)
{
    BOOL fRet;
    
    if (   IsInlinedElement()
        && (   HasFlag(TAGDESC_OWNLINE)
            || pFlowLayoutContext->IsElementBlockInContext(this)
           )
       )
    {
        fRet = TRUE;
    }
    else
        fRet = FALSE;
    
    return fRet;
}

//+----------------------------------------------------------------------------
//
//  Member:     CElement::IsBlockTag
//
//  Synopsis:   Describes whether or not this element is a block tag
//              This should rarely be used - it returns the same value no
//              matter what the display: style setting on the element is.
//              To determine if you should break lines before and after the
//              element use IsBlockElement().
//
//  Returns:    BOOL indicating a block tag
//
//-----------------------------------------------------------------------------

BOOL
CElement::IsBlockTag ( void )
{
    return HasFlag(TAGDESC_BLOCKELEMENT) || Tag() == ETAG_OBJECT;
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::BreaksLine
//
//  Synopsis:   Describes whether or not this node starts a new line
//
//  Returns:    BOOL indicating start of new line
//
//-------------------------------------------------------------------------

BOOL
CElement::BreaksLine ( void )
{
    return (IsBlockElement() &&
                !HasFlag(TAGDESC_WONTBREAKLINE));
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::HasFlag
//
//  Synopsis:   Checks if the element has an given tag
//
//  Returns:    TRUE if it has an end tag else FALSE
//
//-------------------------------------------------------------------------

BOOL
CElement::HasFlag(TAGDESC_FLAGS flag) const
{
    const CTagDesc *ptd = TagDescFromEtag(Tag());
    return ptd ? (ptd->_dwTagDescFlags & flag) ? TRUE : ptd->HasFlag(flag)
               : FALSE;
}


#ifdef VSTUDIO7
//+------------------------------------------------------------------------
//
//  Member:     CElement::SetTagNameAndScope
//
//  Synopsis:   Sets the tag and scope as attributes of an element.
//
//-------------------------------------------------------------------------

HRESULT
CElement::SetTagNameAndScope(CHtmTag *pht)
{
    HRESULT  hr = S_OK;
    LPTSTR   pchStart;
    LPTSTR   pchColon;

    if (ETAG_UNKNOWN == Tag())
    {
        hr = THR(DYNCAST(CUnknownElement, this)->InternalSetTagName(pht));
        goto Cleanup;   // done
    }

    if (GetAAtagName())
        goto Cleanup;

    Assert(!GetAAscopeName());

    if (!pht->GetPch())
        goto Cleanup;

    pchStart = pht->GetPch();
    pchColon = StrChr(pchStart, _T(':'));
    if (pchColon)
    {
        CStringNullTerminator   nullTerminator(pchColon);
        
        hr = SetAAscopeName(pchStart);
        if (hr)
            goto Cleanup;
        
        hr = SetAAtagName(pchColon + 1);
    }
    else
    {
        SetAAtagName(pchStart);
    }

Cleanup:
    RRETURN(hr);
}
#endif //VSTUDIO7

//+------------------------------------------------------------------------
//
//  Member:     CElement::TagName
//
//  Synopsis:   Chases the proxy and the returns the tag name of
//
//  Returns:    const TCHAR *
//
//-------------------------------------------------------------------------

const TCHAR *
CElement::TagName ()
{
#ifndef VSTUDIO7
    return NameFromEtag(Tag());
#else
    HRESULT   hr = S_OK;
    LPCTSTR   pchTagName;

    pchTagName = GetAAtagName();
    return !pchTagName ? NameFromEtag(Tag()) : pchTagName;
#endif //VSTUDIO7
}

const TCHAR *
CElement::Namespace( )
{
#ifndef VSTUDIO7
    return NULL;
#else
    return GetAAscopeName();
#endif //VSTUDIO7
}

const TCHAR *
CElement::NamespaceHtml()
{
    LPCTSTR  pchNamespace = Namespace();

    return pchNamespace ? pchNamespace : _T("HTML");
}

BOOL
SameScope ( CTreeNode * pNode1, const CElement * pElement2 )
{
    // Both NULL
    if(!pNode1 && !pElement2)
        return TRUE;

    return pNode1 && pElement2
        ? pNode1->Element() == pElement2
        : FALSE;
}

BOOL
SameScope ( const CElement * pElement1, CTreeNode * pNode2 )
{
    // Both NULL
    if(!pElement1 && !pNode2)
        return TRUE;

    return pElement1 && pNode2
        ? pElement1 == pNode2->Element()
        : FALSE;
}

BOOL
SameScope ( CTreeNode * pNode1, CTreeNode * pNode2 )
{
    if (pNode1 == pNode2)
        return TRUE;

    return pNode1 && pNode2
        ? pNode1->Element() == pNode2->Element()
        : FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::NameOrIDOfParentForm
//
//  Synopsis:   Return the name or id of a parent form if one exists.
//              NULL if not.
//
//-------------------------------------------------------------------------

LPCTSTR
CElement::NameOrIDOfParentForm()
{
    CElement *  pElementForm;
    LPCTSTR     pchName = NULL;

    pElementForm = GetFirstBranch()->SearchBranchToRootForTag( ETAG_FORM )->SafeElement();

    if (pElementForm)
    {
        pchName = pElementForm->GetIdentifier();
    }
    return pchName;
}


//+------------------------------------------------------------------------
//
//  Member:     CElement::SaveAttribute
//
//  Synopsis:   Save a single attribute to the stream
//
//-------------------------------------------------------------------------

HRESULT
CElement::SaveAttribute (
    CStreamWriteBuff *      pStreamWrBuff,
    LPTSTR                  pchName,
    LPTSTR                  pchValue,
    const PROPERTYDESC *    pPropDesc /* = NULL */,
    CBase *                 pBaseObj /* = NULL */,
    BOOL                    fEqualSpaces /* = TRUE */,    // BUGBUG (dbau) fix all the test cases so that it never has spaces
    BOOL                    fAlwaysQuote /* = FALSE */)
    
{
    HRESULT     hr;
    DWORD       dwOldFlags;

    hr = THR(pStreamWrBuff->Write(_T(" "), 1));
    if (hr)
        goto Cleanup;

    hr = THR(pStreamWrBuff->Write(pchName, _tcslen(pchName)));
    if (hr)
        goto Cleanup;

    if (pchValue || pPropDesc)
    {
        // Quotes are necessary for pages like ASP, that might have
        // <% =x %>. This will screw up the parser if we don't output
        // the quotes around such ASP expressions.
        BOOL fForceQuotes = fAlwaysQuote || !pchValue || !pchValue[0] || 
            (pchValue && ( StrChr(pchValue, _T('<')) || StrChr(pchValue, _T('>')) ));

        if (fEqualSpaces)
            hr = THR(pStreamWrBuff->Write(_T(" = ")));
        else
            hr = THR(pStreamWrBuff->Write(_T("=")));
        if (hr)
            goto Cleanup;

        // We dont want to break the line in the middle of an attribute value
        dwOldFlags = pStreamWrBuff->SetFlags(WBF_NO_WRAP);

        if (pchValue)
        {
            hr = THR(pStreamWrBuff->WriteQuotedText(pchValue, fForceQuotes));
        }
        else
        {
            Assert (pPropDesc && pBaseObj);
            hr = THR(pPropDesc->HandleSaveToHTMLStream (pBaseObj, (void *)pStreamWrBuff));
        }
        if (hr)
            goto Cleanup;

        pStreamWrBuff->RestoreFlags(dwOldFlags);
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
// Member:     CElement::SaveUnknown
//
// Synopsis:   Write these guys out
//
// Returns:    HRESULT
//
//+------------------------------------------------------------------------

HRESULT
CElement::SaveUnknown(CStreamWriteBuff * pStreamWrBuff, BOOL *pfAny)
{
    HRESULT hr = S_OK;
    AAINDEX aaix = AA_IDX_UNKNOWN;
    LPCTSTR lpPropName;
    LPCTSTR lpszValue = NULL;
    BSTR bstrTemp = NULL;
    DISPID expandoDISPID;
    BOOL fAny = FALSE;

    // Look for all expandos & dump them out
    while ( (aaix = FindAAType (CAttrValue::AA_Expando, aaix ) )
        != AA_IDX_UNKNOWN )
    {
        CAttrValue *pAV = _pAA->FindAt(aaix);

        Assert (pAV);

        // Get value into a string, but skip VT_DISPATCH & VT_UNKNOWN
        if (pAV->GetAVType() == VT_DISPATCH || pAV->GetAVType() == VT_UNKNOWN )
            continue;

        // BUGBUG rgardner - we should smarten this up so we don't need to allocate a string

        // Found a literal attrValue
        hr = pAV->GetIntoString( &bstrTemp, &lpszValue );

        if ( hr == S_FALSE )
        {
            // Can't convert to string
            continue;
        }
        else if ( hr )
        {
            goto Cleanup;
        }

        fAny = TRUE;

        expandoDISPID = GetDispIDAt ( aaix );
        if (TestClassFlag(ELEMENTDESC_OLESITE))
        {
            expandoDISPID = expandoDISPID + DISPID_EXPANDO_BASE - DISPID_ACTIVEX_EXPANDO_BASE;
        }

        hr = GetExpandoName ( expandoDISPID, &lpPropName );
        if (hr)
            goto Cleanup;

        hr = THR(SaveAttribute(pStreamWrBuff, (LPTSTR)lpPropName, (LPTSTR)lpszValue, NULL, NULL, FALSE, TRUE)); // Always quote value: IE5 57717
        if (hr)
            goto Cleanup;

        if ( bstrTemp )
        {
            SysFreeString ( bstrTemp );
            bstrTemp = NULL;
        }
    }

    if (pfAny)
        *pfAny = fAny;

Cleanup:
    if ( bstrTemp )
        FormsFreeString ( bstrTemp );
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
// Member:     CElement::SaveUnknown
//
// Synopsis:   Write these guys out
//
// Returns:    HRESULT
//
//+------------------------------------------------------------------------

HRESULT
CElement::SaveUnknown(IPropertyBag * pPropBag, BOOL fSaveBlankAttributes )
{
    HRESULT     hr = S_OK;
    AAINDEX     aaix = AA_IDX_UNKNOWN;
    LPCTSTR     lpPropName;
    CVariant    var;
    DISPID      dispidExpando;

    while ( (aaix = FindAAType ( CAttrValue::AA_Expando, aaix )) != AA_IDX_UNKNOWN )
    {
        var.vt = VT_EMPTY;

        hr = THR(GetIntoBSTRAt(aaix, &(var.bstrVal)));
        if (hr == S_FALSE)
        {
            // Can't convert to string
            continue;
        }
        else if (hr)
        {
            goto Cleanup;
        }

        // We do not save attributes with null string values for netscape compatibility of
        // <EMBED src=thisthat loop> attributes which have no value - the loop attribute
        // in the example.   Pluginst.cxx passes in FALSE for fSaveBlankAttributes, everybody
        // else passes in TRUE via a default param value.
        if( !fSaveBlankAttributes && (var.bstrVal == NULL || *var.bstrVal == _T('\0') ) )
        {
            VariantClear (&var);
            continue;
        }

        var.vt = VT_BSTR;

        dispidExpando = GetDispIDAt(aaix);
        if (TestClassFlag(ELEMENTDESC_OLESITE))
        {
            dispidExpando = dispidExpando +
                DISPID_EXPANDO_BASE - DISPID_ACTIVEX_EXPANDO_BASE;
        }
        hr = THR(GetExpandoName(dispidExpando, &lpPropName));
        if (hr)
            goto Cleanup;

        hr = THR(pPropBag->Write(lpPropName, &var));
        if (hr)
            goto Cleanup;

        VariantClear (&var);
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Helper:     StoreLineAndOffsetInfo
//
//  Synopsis:   stores line and offset information for a property in attr array of the object
//
//-------------------------------------------------------------------------

HRESULT
StoreLineAndOffsetInfo(CBase * pBaseObj, DISPID dispid, ULONG uLine, ULONG uOffset)
{
    HRESULT         hr;
    // pchData will be of the form "ulLine ulOffset", for example: "13 1313"
    TCHAR           pchData [30];   // in only needs to be 21, but let's be safe

    hr = Format(0, &pchData, 30, _T("<0du> <1du>"), uLine, uOffset);
    if (hr)
        goto Cleanup;

    hr = THR(pBaseObj->AddString(dispid, pchData, CAttrValue::AA_Internal));

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::GetLineAndOffsetInfo
//
//  Synopsis:   retrieves line and offset information for a property from attr array
//
//  Returns:    S_OK        successfully retrieved the data
//              S_FALSE     no error, but the information in attr array is not
//                          a line/offset string
//              FAILED(hr)  generic error condition
//
//-------------------------------------------------------------------------

HRESULT
GetLineAndOffsetInfo(CAttrArray * pAA, DISPID dispid, ULONG * puLine, ULONG * puOffset)
{
    HRESULT         hr;
    CAttrValue *    pAV;
    AAINDEX         aaIdx;
    LPTSTR          pchData;
    TCHAR *         pchTempStart;
    TCHAR *         pchTempEnd;

    Assert (puOffset && puLine);

    (*puOffset) = (*puLine) = 0;      // set defaults

    //
    // get the information string
    //

    aaIdx = AA_IDX_UNKNOWN;
    pAV = pAA->Find(dispid, CAttrValue::AA_Internal, &aaIdx);
    if (!pAV || VT_LPWSTR != pAV->GetAVType())
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    pchData = pAV->GetString();

    //
    // pchData is of the format: "lLine lOffset", for example: "13 1313"
    // Here we crack the string apart.
    //

    pchTempStart = pchData;
    pchTempEnd = _tcschr(pchData, _T(' '));

    Assert (pchTempEnd);

    *pchTempEnd = _T('\0');
    hr = THR(ttol_with_error(pchTempStart, (LONG*)puLine));
    *pchTempEnd = _T(' ');
    if (hr)
        goto Cleanup;

    pchTempStart = ++pchTempEnd;

    hr = THR(ttol_with_error(pchTempStart, (LONG*)puOffset));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN1 (hr, S_FALSE);
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::ConnectEventHandler
//
//  Synopsis:   retrieves string with the specified dispid from attr array, constructs
//              code from the string and puts it in attr array with dispidCode.
//              When fStandard is set, meaning that the event handler is standard,
//              such as onclick, dispid identifies script text in AA_Attribute
//              section of attr array. If fStandard is false, then dispid identifies script
//              text in AA_Expando section of attr array.
//              Also caches language attribute in ppchLanguageCached if provided.
//              uOffset and uLine specify line and offset of script text in source html -
//              this information is necessary for script debugger.
//
//-------------------------------------------------------------------------

HRESULT
CElement::ConnectInlineEventHandler(
    DISPID      dispid,
    DISPID      dispidCode,
    ULONG       uOffset,
    ULONG       uLine,
    BOOL        fStandard,
    LPCTSTR *   ppchLanguageCached)
{
    HRESULT         hr = S_OK;
    LPCTSTR         pchCode = NULL;
    IDispatch *     pDispCode = NULL;
    CAttrArray *    pAA = *GetAttrArray();
    CDoc *          pDoc = Doc();
    CBase *         pBaseObj;
    LPCTSTR         pchLanguageLocal;

    //
    // get language
    //

    if (!ppchLanguageCached)
    {
        pchLanguageLocal = NULL;
        ppchLanguageCached = &pchLanguageLocal;
    }

    if (!(*ppchLanguageCached))
    {
        if (pAA == NULL || !pAA->FindString (DISPID_A_LANGUAGE, ppchLanguageCached, CAttrValue::AA_Attribute))
        {
            (*ppchLanguageCached) = _T("");
        }
    }

    //
    // get base object and code
    //

    pBaseObj = GetBaseObjectFor (dispid);

    if (!pBaseObj || !(*(pBaseObj->GetAttrArray())))
        goto Cleanup;

    // this is supposed to be found because of the logic dispid-s passed in this function (alexz)
    //   however, for peers on the object tag, the expandos are saved as activeX_EXPANDOS and we
    //   need to search for a differnt dispid
    if (! (*(pBaseObj->GetAttrArray()))->FindString(
                                dispid,
                                (LPCTSTR *) &pchCode,
                                fStandard ? CAttrValue::AA_Attribute :
                                            CAttrValue::AA_Expando))
    {
        (*(pBaseObj->GetAttrArray()))->FindString(
                                ((dispid - DISPID_EXPANDO_BASE) + DISPID_ACTIVEX_EXPANDO_BASE),
                                (LPCTSTR *) &pchCode,
                                CAttrValue::AA_Expando);
    }

    if (!pchCode)
        goto Cleanup;

    //
    // debug stuff
    //

#if DBG==1
    if (IsTagEnabled(tagHtmSrcTest))
    {
        TCHAR achSrc[512];
        ULONG cch;

        HRESULT hrT;

        hrT = pDoc->GetHtmSourceText(uOffset, ARRAY_SIZE(achSrc) - 1,
                achSrc, &cch);

        if (hrT == S_OK)
        {
            cch = min(cch, (ULONG)_tcslen(pchCode));
            achSrc[cch] = 0;

            TraceTag((
                tagHtmSrcTest,
                "Expect \"%.64ls\", (ln=%ld pos=%ld) \"%.64ls\"",
                pchCode, uLine, uOffset, achSrc));
        }
    }
#endif

    //
    // get previously stored line/offset information
    //

    if (!fStandard)
    {
        Assert (0 == uLine && 0 == uOffset);

        hr = THR(GetLineAndOffsetInfo(pAA, dispid, &uLine, &uOffset));
        if (!OK(hr))        // if not S_OK or S_FALSE
            goto Cleanup;
    }

    //
    // construct code and handle result
    //

    hr = THR_NOTRACE(pDoc->_pScriptCollection->ConstructCode(
        (LPTSTR)NameOrIDOfParentForm(),     // pchScope
        (LPTSTR)pchCode,                    // pchCode
        NULL,                               // pchFormalParams
        (LPTSTR)(*ppchLanguageCached),      // pchLanguage
        GetMarkup(),                        // pMarkup
        NULL,                               // pchType (valid on script tags only)
        uOffset,                            // ulOffset
        uLine,                              // ulStartingLine
        GetMarkup(),                        // pSourceObject
        SCRIPTPROC_HOSTMANAGESSOURCE,       // dwFlags
        &pDispCode,                         // ppDispCode result
        TRUE));                             // fSingleLine

    if (S_OK == hr && pDispCode)
    {
        // pDispCode can be NULL if the script was parsed
        // but it contained no executeable statements
        hr = THR(pBaseObj->AddDispatchObject(
            dispidCode,
            pDispCode,
            CAttrValue::AA_Internal));
        ClearInterface (&pDispCode);
        if (hr)
            goto Cleanup;

        // if we add a data event to an element, make sure the event can fire
        CDataMemberMgr::EnsureDataEventsFor(pBaseObj, dispid);
    }
    else if (E_NOTIMPL == hr && !pDispCode)
    {
        //
        // ConstructCode must have failed because we are parsing VBScript and it does not support
        // function pointers. In this case, we will do AddScriptlet in CElement::AddAllScriptlets.
        // The only thing we need to do now is to store line/offset numbers in attr array in
        // AA_Internal section, if it is not there yet.
        // There are 2 codepaths which can lead us here:
        // 1.   We call this method from CElement::InitAttrBag to connect standard inline
        // event handlers, such as onclick. We call it passing actual line/offset information
        // available there from CHtmlTag * pht. In this case line/offset information is passed
        // and we store it
        // 2.   We call this method from CPeerHolder::RegisterEvent to connect custom peer event
        // handler. In this case the event handler is an expando and line/offset information was
        // stored for it in CElement::InitAttrBag; we don't need to do anything.

        if (fStandard) // equivalent to condition (0 != uLine || 0 != uOffset)
        {
            hr = StoreLineAndOffsetInfo (pBaseObj, dispid, uLine, uOffset);
            if (hr)
                goto Cleanup;
        }
    }
    
    // Don't propagate the last error - code construction could have failed due to
    // syntax errors so try to construct other event handlers
    hr = S_OK;

Cleanup:
    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CElement::InitAttrBag
//
//  Synopsis:   Fetch values from CHtmTag and put into the bag
//
//  Arguments:  pht : parsed attributes in text format
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CElement::InitAttrBag(CHtmTag *pht)
{
    int i;
    CHtmTag::CAttr *pattr = NULL;
    HRESULT hr = S_OK;
    const PROPERTYDESC * ppropdesc;
    CBase *pBaseObj;
    InlineEvts  *pInlineEvts = NULL;
    WORD    wMaxstrlen = 0;
    TCHAR   chOld = _T('\0');
    CDoc *  pDoc = Doc();
    THREADSTATE * pts = GetThreadState();
// HACK HACK 47681 start: ignore height/width for inputs not input/image
    BOOL    fHackForInput;
    BOOL    fHackForInputProp;
// HACK HACK 47681 end

    if (*GetAttrArray())
    {
        // InitAttrBag must have already been called.
        return S_OK;
    }

    pts->fInInitAttrBag = TRUE;

// HACK HACK 47681 start: ignore height/width for inputs not input/image
    fHackForInput = (Tag() == ETAG_INPUT && DYNCAST(CInput, this)->GetType() != htmlInputImage);
// HACK HACK 47681 end

    // Loop over all attr pairs in the tag, and see if their BYTE in the known-attr array
    // is set; if not, add the attr and any val to the CaryUnknownAttrs in the attr bag
    // (create one if needed).

    for (i = pht ? pht->GetAttrCount() : 0; --i >= 0; )
    {
        pattr = pht->GetAttr(i);

        if (!pattr->_pchName)
            continue;

// HACK HACK 47681 start: ignore height/width for inputs not input/image
        fHackForInputProp = fHackForInput && 
                (StrCmpIC(s_propdescCInputheight.a.pstrName, pattr->_pchName) == 0
                || StrCmpIC(s_propdescCInputwidth.a.pstrName, pattr->_pchName) == 0);

        if (!fHackForInputProp && (ppropdesc = FindPropDescForName(pattr->_pchName)) != NULL)
//
// HACK HACK 47681 end. uncomment the following line when removing the HACK section
        // if ((ppropdesc = FindPropDescForName(pattr->_pchName)) != NULL)
        {
            // Allow some elements to redirect to another attr array
            pBaseObj = GetBaseObjectFor (ppropdesc->GetDispid());

            if (!pBaseObj)
            {
                continue;
            }

#ifndef WIN16
            // BUGWIN16: HandleLoadFromHTMLString is a class method, I am amazed that we can check
            // for it being non - null !? - vamshi - 4/29/97
#ifndef UNIX
// Unix gets, Error: Taking address of the bound function PROPERTYDESC::HandleLoadFromHTMLString(CBase*, wchar_t*) const.
            AssertSz(ppropdesc->HandleLoadFromHTMLString != NULL, "attempt to load abstract property from html");
#endif
#endif
            wMaxstrlen = (ppropdesc->GetBasicPropParams()->wMaxstrlen == pdlNoLimit) ? 0 :
                         (ppropdesc->GetBasicPropParams()->wMaxstrlen ? ppropdesc->GetBasicPropParams()->wMaxstrlen : DEFAULT_ATTR_SIZE);

            if (wMaxstrlen && pattr->_pchVal && _tcslen(pattr->_pchVal) > wMaxstrlen)
            {
                chOld = pattr->_pchVal[wMaxstrlen];
                pattr->_pchVal[wMaxstrlen] = _T('\0');
            }
            hr = THR_NOTRACE( ppropdesc->HandleLoadFromHTMLString ( pBaseObj, pattr->_pchVal ));

            if (ppropdesc->GetPPFlags() & PROPPARAM_SCRIPTLET)
            {
                if (!pInlineEvts)
                    pInlineEvts = new InlineEvts;

                if (pInlineEvts)
                {
                    pInlineEvts->adispidScriptlets[pInlineEvts->cScriptlets] = ppropdesc->GetDispid();
                    pInlineEvts->aOffsetScriptlets[pInlineEvts->cScriptlets] = pattr->_ulOffset;
                    pInlineEvts->aLineScriptlets[pInlineEvts->cScriptlets++] = pattr->_ulLine;
                }
                else
                    goto Cleanup;
            }

            if ( hr )
            {
                // Create an "unknown" attribute containing the original string from the HTML
                // SetString with fIsUnkown set to TRUE
                if (chOld)
                {
                    pattr->_pchVal[wMaxstrlen] = chOld;
                    chOld = 0;
                }
                hr = CAttrArray::SetString ( pBaseObj->GetAttrArray(), ppropdesc,
                    pattr->_pchVal, TRUE, CAttrValue::AA_Extra_DefaultValue );
            }

            // If the parameter was invalid, value will get set to default &
            // parameter will go into the unknown bag
            if ( !hr )
            {
                if ( ppropdesc->GetDispid() == DISPID_A_BACKGROUNDIMAGE )
                {
                    // Fork off an early download for background images
                    LPCTSTR lpszURL;
                    if ( !(*(pBaseObj->GetAttrArray()))->FindString(DISPID_A_BACKGROUNDIMAGE, &lpszURL) )
                    {
                        LONG lCookie;

                        if (GetImageUrlCookie(lpszURL, &lCookie) == S_OK)
                        {
                            AddImgCtx(DISPID_A_BGURLIMGCTXCACHEINDEX,
                                      lCookie);
                        }
                    }
                }
            }
            else if (hr == E_OUTOFMEMORY)
            {
                goto Cleanup;
            }
        }
        else
        {
            DISPID  expandoDISPID;

            // Create an expando

            hr = THR_NOTRACE(AddExpando(pattr->_pchName, &expandoDISPID));

            if (hr == DISP_E_MEMBERNOTFOUND)
            {
                hr = S_OK;
                continue; // Expando not turned on
            }
            if (hr)
                goto Cleanup;

            if (TestClassFlag(ELEMENTDESC_OLESITE))
            {
                expandoDISPID = expandoDISPID - DISPID_EXPANDO_BASE +
                    DISPID_ACTIVEX_EXPANDO_BASE;
            }

            // Note that we always store expandos in the current object - we never redirect them
            hr = THR(AddString(
                    expandoDISPID,
                    pattr->_pchVal,
                    CAttrValue::AA_Expando));
            if (hr)
                goto Cleanup;

            // if begins with "on", this can be a peer registered event - need to store line/offset numbers
            if (0 == StrCmpNIC(_T("on"), pattr->_pchName, 2))
            {
                hr = THR(StoreLineAndOffsetInfo(this, expandoDISPID, pattr->_ulLine, pattr->_ulOffset));
                if (hr)
                    goto Cleanup;
            }
        }

        if (chOld)
        {
            pattr->_pchVal[wMaxstrlen] = chOld;
            chOld = _T('\0');
        }
    }

    if (pInlineEvts && pInlineEvts->cScriptlets && pDoc->_pScriptCollection)
    {
        SetEventsShouldFire();

        StoreEventsToHook(pInlineEvts);
        pInlineEvts = NULL; // Make sure we don't free it.
    }

Cleanup:
    if (pInlineEvts)
        delete pInlineEvts;
    if (chOld)
        pattr->_pchVal[wMaxstrlen] = chOld;

    pts->fInInitAttrBag = FALSE;

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::MergeAttrBag
//
//  Synopsis:   Add any value from CHtmTag that are not already present
//              in the attrbag
//
//              Note: currently, expandos are not merged.
//
//  Arguments:  pht : parsed attributes in text format
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CElement::MergeAttrBag(CHtmTag *pht)
{
    int i;
    CHtmTag::CAttr *pattr = NULL;
    HRESULT hr = S_OK;
    const PROPERTYDESC * ppropdesc;
    InlineEvts inlineEvts;
    CBase *pBaseObj;
    WORD    wMaxstrlen = 0;
    TCHAR   chOld = _T('\0');
    CDoc *  pDoc = Doc();

    // Loop over all attr pairs in the tag, and see if their BYTE in the known-attr array
    // is set; if not, add the attr and any val to the CaryUnknownAttrs in the attr bag
    // (create one if needed).

    for (i = pht ? pht->GetAttrCount() : 0; --i >= 0; )
    {
        pattr = pht->GetAttr(i);

        if (!pattr->_pchName)
            continue;

        if ((ppropdesc = FindPropDescForName(pattr->_pchName)) != NULL)
        {
            // Allow some elements to redirect to another attr array
            pBaseObj = GetBaseObjectFor (ppropdesc->GetDispid());

            // Only add the attribute if it has not been previously defined
            // style attribute requires special handling
            if (!pBaseObj
                || (AA_IDX_UNKNOWN != pBaseObj->FindAAIndex(ppropdesc->GetDispid(), CAttrValue::AA_Attribute))
                || (AA_IDX_UNKNOWN != pBaseObj->FindAAIndex(ppropdesc->GetDispid(), CAttrValue::AA_UnknownAttr))
                || (AA_IDX_UNKNOWN != pBaseObj->FindAAIndex(ppropdesc->GetDispid(), CAttrValue::AA_Internal))
                || (AA_IDX_UNKNOWN != pBaseObj->FindAAIndex(ppropdesc->GetDispid(), CAttrValue::AA_AttrArray))
                || (ppropdesc == (PROPERTYDESC *)&s_propdescCElementstyle_Str && AA_IDX_UNKNOWN != pBaseObj->FindAAIndex(DISPID_INTERNAL_INLINESTYLEAA, CAttrValue::AA_AttrArray)))
            {
                continue;
            }

            wMaxstrlen = (ppropdesc->GetBasicPropParams()->wMaxstrlen == pdlNoLimit) ? 0 :
                         (ppropdesc->GetBasicPropParams()->wMaxstrlen ? ppropdesc->GetBasicPropParams()->wMaxstrlen : DEFAULT_ATTR_SIZE);

            if (wMaxstrlen && pattr->_pchVal && _tcslen(pattr->_pchVal) > wMaxstrlen)
            {
                chOld = pattr->_pchVal[wMaxstrlen];
                pattr->_pchVal[wMaxstrlen] = _T('\0');
            }
            hr = THR ( ppropdesc->HandleMergeFromHTMLString ( pBaseObj, pattr->_pchVal ) );

            if (ppropdesc->GetPPFlags() & PROPPARAM_SCRIPTLET)
            {
                inlineEvts.adispidScriptlets[inlineEvts.cScriptlets] = ppropdesc->GetDispid();
                inlineEvts.aOffsetScriptlets[inlineEvts.cScriptlets] = pattr->_ulOffset;
                inlineEvts.aLineScriptlets[inlineEvts.cScriptlets++] = pattr->_ulLine;
            }

            if (hr)
            {
                // Create an "unknown" attribute containing the original string from the HTML
                // SetString with fIsUnkown set to TRUE
                hr = CAttrArray::SetString ( pBaseObj->GetAttrArray(), ppropdesc,
                    pattr->_pchVal, TRUE, CAttrValue::AA_Extra_DefaultValue );
            }

            // If the parameter was invalid, value will get set to default &
            // parameter will go into the unknown bag
            if ( !hr )
            {
                if ( ppropdesc->GetDispid() == DISPID_A_BACKGROUNDIMAGE )
                {
                    // Fork off an early download for background images
                    LPCTSTR lpszURL;
                    if ( !(*(pBaseObj->GetAttrArray()))->FindString(DISPID_A_BACKGROUNDIMAGE, &lpszURL) )
                    {
                        LONG lCookie;

                        if (GetImageUrlCookie(lpszURL, &lCookie) == S_OK)
                        {
                            AddImgCtx(DISPID_A_BGURLIMGCTXCACHEINDEX,
                                      lCookie);
                        }
                    }
                }
            }
            else if (hr == E_OUTOFMEMORY)
            {
                goto Cleanup;
            }
        }
        
        if (chOld)
        {
            pattr->_pchVal[wMaxstrlen] = chOld;
            chOld = _T('\0');
        }
    }
    
     hr = THR(inlineEvts.Connect(pDoc, this));
     if (hr)
         goto Cleanup;

Cleanup:
    if (chOld)
        pattr->_pchVal[wMaxstrlen] = chOld;

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::Init2
//
//  Synopsis:   Perform any element level initialization
//
//-------------------------------------------------------------------------

HRESULT
CElement::Init2(CInit2Context * pContext)
{
    HRESULT         hr = S_OK;
    LPCTSTR         pch;
//  ELEMENT_TAG     etag = Tag();
    CDoc *          pDoc = Doc();
    CAttrArray *    pAAInline;

    pch = GetIdentifier();
    if (pch)
    {
        hr = THR(pDoc->_AtomTable.AddNameToAtomTable(pch, NULL));
        if (hr)
            goto Cleanup;
    }

    //
    // behaviors support
    //

    // BUGBUG (alexz) see if it is possible to make it without parsing inline styles here
    pAAInline = GetInLineStyleAttrArray();
    if (pAAInline && pAAInline->Find(DISPID_A_BEHAVIOR))
    {
        pDoc->SetPeersPossible();
    }

#ifdef NEVER
    //
    // Set the _fTabStop bit for all the default tags.
    //

    switch (etag)
    {
    case ETAG_HR:
    case ETAG_DIV:
    case ETAG_TABLE:
    case ETAG_IMG:
        if (pDoc->_fDesignMode)
        {
            _fTabStop = TRUE;
        }
        break;

    case ETAG_LABEL:
    case ETAG_A:
    case ETAG_FRAME:
    case ETAG_EMBED:
    case ETAG_LEGEND:
        if (etag == ETAG_FRAME && _fSynthesized)
            break;

        if (!pDoc->_fDesignMode)
        {
            _fTabStop = TRUE;
        }
        break;

    case ETAG_INPUT:
    case ETAG_SELECT:
    case ETAG_TEXTAREA:
    case ETAG_BUTTON:
#ifdef  NEVER
    case ETAG_HTMLAREA:
#endif
    case ETAG_IFRAME:
    case ETAG_OBJECT:
    case ETAG_APPLET:
        if (etag == ETAG_IFRAME && _fSynthesized)
            break;

        _fTabStop = TRUE;
       break;

    case ETAG_MAP:
    case ETAG_AREA:
        _fTabStop = FALSE;
        break;

    default:
        if (GetAAtabIndex() > 0)
        {
            _fTabStop = TRUE;
        }
        break;
    }
#endif

#ifdef VSTUDIO7
    // BUGBUG (alexz) this can be enabled for main build when pContext carries
    // a bit indicating that this is a DOM case. Then the bit will be used instead of
    // condition "(!pHtmCtx || !pHtmCtx->IsLoading())"
    {
        CHtmCtx * pHtmCtx = pContext && pContext->_pTargetMarkup ?
                                        pContext->_pTargetMarkup->HtmCtx() :
                                        NULL;

        // CONSIDER (alexz, anandra) get rid of this special-casing

        // if not parsing
        if (!pHtmCtx || !pHtmCtx->IsLoading())
        {
            // Assert (tree, parser and the world are in robust state);

            hr = THR(EnsureIdentityPeer());
            if (hr)
                goto Cleanup;
        }
        // else
        // {
        //      Assert (identity peer will be ensured when the element enters tree);
        // }
    }
#endif

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CElement::Save
//
//  Synopsis:   Save the element to the stream
//
//-------------------------------------------------------------------------

HRESULT
CElement::Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd)
{
    HRESULT hr;

    if (fEnd && HasFlag(TAGDESC_SAVEINDENT))
    {
        pStreamWrBuff->EndIndent();
    }

    hr = WriteTag(pStreamWrBuff, fEnd);
    if(hr)
        goto Cleanup;

    if (!fEnd && HasFlag(TAGDESC_SAVEINDENT))
    {
        pStreamWrBuff->BeginIndent();
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::SaveAttributes
//
//  Synopsis:   Save the attributes to the stream
//
//-------------------------------------------------------------------------

HRESULT
CElement::SaveAttributes ( CStreamWriteBuff * pStreamWrBuff, BOOL *pfAny )
{
    const PROPERTYDESC * const * pppropdescs;
    HRESULT hr = S_OK;
    BOOL fSave;
    LPCTSTR lpstrUnknownValue;
    CBase *pBaseObj;
    BOOL fAny = FALSE;

    if (GetPropDescArray())
    {
        for (pppropdescs = GetPropDescArray(); *pppropdescs; pppropdescs++)
        {
            const PROPERTYDESC * ppropdesc = (*pppropdescs);
            // BUGBUG for now check for the method pointer because of old property implementation...
            if (!ppropdesc->pfnHandleProperty)
            {
                continue;
            }

            pBaseObj = GetBaseObjectFor (ppropdesc->GetDispid());

            if (!pBaseObj)
            {
                continue;
            }

            lpstrUnknownValue = NULL;
            if ( ppropdesc->GetPPFlags() & PROPPARAM_ATTRARRAY)
            {
                AAINDEX aaIx = AA_IDX_UNKNOWN;
                CAttrValue *pAV = NULL;
                CAttrArray *pAA = *(pBaseObj->GetAttrArray());

                if (pAA)
                    pAV = pAA->Find(ppropdesc->GetDispid(), CAttrValue::AA_Attribute, &aaIx);

                if (pAA && (!pAV || pAV->IsDefault()))
                {
                    if (pAV)
                        aaIx++;

                    pAV = pAA->FindAt(aaIx);
                    if (pAV)
                    {
                        if ((pAV->GetDISPID() == ppropdesc->GetDispid()) &&
                            (pAV->GetAAType() == CAttrValue::AA_UnknownAttr))
                        {
                            // Unknown attrs are always strings
                            lpstrUnknownValue = pAV->GetLPWSTR();
                        }
                        else
                            pAV = NULL;
                    }
                }
                fSave = !!pAV;

                // don't save databinding attributes during printing, so that we
                // print the current content instead of re-binding
                if (pStreamWrBuff->TestFlag(WBF_NO_DATABIND_ATTRS))
                {
                    DISPID dispid = ppropdesc->GetDispid();
                    if (    dispid == DISPID_CElement_dataSrc ||
                            dispid == DISPID_CElement_dataFld ||
                            dispid == DISPID_CElement_dataFormatAs)
                        fSave = FALSE;
                }
            }
            else
            {
                // Save the property if it was not the same as the default.
                // Do not save if we got an error retrieving it.
#ifdef VSTUDIO7
                DISPID dispid = ppropdesc->GetDispid();
                if (dispid == DISPID_CElement_style_Str)
                {
                    FlushPeerProperties();
                }
#endif
                fSave = ppropdesc->HandleCompare ( pBaseObj,
                    (void *)&ppropdesc->ulTagNotPresentDefault ) == S_FALSE;
            }

            if (fSave)
            {
                fAny = TRUE;

                if (lpstrUnknownValue)
                {
                    hr = THR(SaveAttribute(
                        pStreamWrBuff,
                        (LPTSTR)ppropdesc->pstrName,
                        (LPTSTR)lpstrUnknownValue,  // pchValue
                        NULL,                       // ppropdesc
                        NULL,                       // pBaseObj
                        FALSE));                    // fEqualSpaces
                }
                else
                {
                    if (ppropdesc->IsBOOLProperty())
                    {
                        hr = THR(SaveAttribute(
                            pStreamWrBuff,
                            (LPTSTR)ppropdesc->pstrName,
                            NULL,                       // pchValue
                            NULL,                       // ppropdesc
                            NULL,                       // pBaseObj
                            FALSE));                    // fEqualSpaces
                    }
                    else
                        hr = THR(SaveAttribute(
                            pStreamWrBuff,
                            (LPTSTR)ppropdesc->pstrName,
                            NULL,                       // pchValue
                            ppropdesc,                  // ppropdesc
                            pBaseObj,                   // pBaseObj
                            FALSE));                    // fEqualSpaces
                }
            }
        }
    }

    hr = SaveUnknown(pStreamWrBuff, fAny ? NULL : &fAny);
    if (hr)
        goto Cleanup;

    if (HasPeerHolder())
    {
        IGNORE_HR(GetPeerHolder()->SaveMulti(pStreamWrBuff, fAny ? NULL : &fAny));
    }

    if (pfAny)
        *pfAny = fAny;

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CElement::SaveAttributes
//
//  Synopsis:   Save the attributes into property bag
//
//-------------------------------------------------------------------------

HRESULT
CElement::SaveAttributes ( IPropertyBag * pPropBag, BOOL fSaveBlankAttributes )
{
    const PROPERTYDESC * const * pppropdescs;
    HRESULT             hr = S_OK;
    CVariant            Var;
    BOOL                fSave;
    CBase               *pBaseObj;

    if (GetPropDescArray())
    {
        for (pppropdescs = GetPropDescArray(); *pppropdescs; pppropdescs++)
        {
            const PROPERTYDESC * ppropdesc = (*pppropdescs);
            // BUGBUG for now check for the method pointer because of old property implementation...
            if (!ppropdesc->pfnHandleProperty)
            {
                continue;
            }

            pBaseObj = GetBaseObjectFor (ppropdesc->GetDispid());

            if (!pBaseObj)
            {
                continue;
            }

            if (ppropdesc->GetPPFlags() & PROPPARAM_ATTRARRAY)
            {
                AAINDEX aaIx;
                aaIx = pBaseObj->FindAAIndex ( ppropdesc->GetDispid(), CAttrValue::AA_Attribute );
                fSave = ( aaIx == AA_IDX_UNKNOWN ) ? FALSE : TRUE;
            }
            else
            {
                // Save the property if it was not the same as the default.
                // Do not save if we got an error retrieving it.
                fSave = ppropdesc->HandleCompare ( pBaseObj,
                    (void *)&ppropdesc->ulTagNotPresentDefault ) == S_FALSE;
            }

            if (fSave)
            {
                // If we're dealing with a BOOL type, don't put a value
                if ( ppropdesc->IsBOOLProperty() )
                {
                    // Boolean (flag), skip the =<val>
                    Var.vt = VT_EMPTY;
                }
                else
                {
                    hr = THR(ppropdesc->HandleGetIntoBSTR ( pBaseObj, &V_BSTR(&Var) ));
                    if (hr)
                        continue;
                    V_VT(&Var) = VT_BSTR;
                }
                hr = pPropBag->Write(ppropdesc->pstrName, &Var);
                if (hr)
                    goto Cleanup;
                // if Var has an Allocated value, we need to free it before
                // going around the loop again.
                VariantClear(&Var);
            }
        }
    }

    hr = SaveUnknown(pPropBag, fSaveBlankAttributes);

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CElement::HandleMessage
//
//  Synopsis:   Perform any element specific mesage handling
//
//  Arguments:  pmsg    Ptr to incoming message
//
//  Notes:      pBranch should always be non-scoped. This is the only way
//              that the context information can be maintained. each element
//              HandleMessage which uses pBranch needs to be very careful
//              to scope first.
//
//-------------------------------------------------------------------------

HRESULT
CElement::HandleMessage(CMessage *pmsg)
{
    HRESULT     hr                  = S_FALSE;

    // Only the marquee is allowed to cheat and pass the wrong
    // context in
    Assert(IsInMarkup());

    CLayout * pLayout = GetUpdatedLayout();

    if (pLayout)
    {
        Assert(!pmsg->fStopForward);
        hr = THR(pLayout->HandleMessage(pmsg));
        if (hr != S_FALSE || pmsg->fStopForward)
            goto Cleanup;
    }

    if (pmsg->message == WM_SETCURSOR)
    {
        hr = THR_NOTRACE(SetCursorStyle((LPTSTR)NULL, GetFirstBranch()));
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+------------------------------------------------------------------------
//
//  Member:     DisallowSelection
//
//  Synopsis:   Returns TRUE iff selection must be diallowed.
//
//-------------------------------------------------------------------------

BOOL
CElement::DisallowSelection()
{
    // In dialogs, only editable controls can be selected
    return ((Doc()->_dwFlagsHostInfo & DOCHOSTUIFLAG_DIALOG)
         && !(HasLayout() && GetCurLayout()->_fAllowSelectionInDialog && IsEnabled()));
}


//+------------------------------------------------------------------------
//
//  Member:     CloseErrorInfo
//
//  Synopsis:   Pass the call to the form so it can return its clsid
//              instead of the object's clsid as in CBase.
//
//-------------------------------------------------------------------------

HRESULT
CElement::CloseErrorInfo(HRESULT hr)
{
    Doc()->CloseErrorInfo(hr);

    return hr;
}

//+------------------------------------------------------------------------
//
//  Member  :   SetCursorStyle
//
//  Synopsis : if the element.style.cursor property is set then on the handling
//      of the WM_CURSOR we should set the cursor to the one specified in
//      the style.
//
//-----------------------------------------------------------------------------

HRESULT
CElement::SetCursorStyle(LPCTSTR idcArg, CTreeNode* pContext /* = NULL */)
{
    HRESULT hr = E_FAIL;
    LPCTSTR idc;
    CDoc *  pDoc = Doc();
    static const LPCTSTR aStyleToCursor[] = {
        IDC_ARROW,                       // auto map to arrow
        IDC_CROSS,                       // map to crosshair
        IDC_ARROW,                       // default map to arrow
        MAKEINTRESOURCE(IDC_HYPERLINK),  // hand map to IDC_HYPERLINK
        IDC_SIZEALL,                     // move map to SIZEALL
        MAKEINTRESOURCE(IDC_SCROLLEAST), // e-resize
        MAKEINTRESOURCE(IDC_SCROLLNE),   // ne-resize
        MAKEINTRESOURCE(IDC_SCROLLNW),   // nw-resize
        MAKEINTRESOURCE(IDC_SCROLLNORTH),// n-resize
        MAKEINTRESOURCE(IDC_SCROLLSE),   // se-resize
        MAKEINTRESOURCE(IDC_SCROLLSW),   // sw-resize
        MAKEINTRESOURCE(IDC_SCROLLSOUTH),// s-resize
        MAKEINTRESOURCE(IDC_SCROLLWEST), // w-resize
        IDC_IBEAM,                       // text
        IDC_WAIT,                        // wait
    #if(WINVER >= 0x0400)
        IDC_HELP,                       // help as IDC_help
    #else
        IDC_ARROW,                      // help as IDC_ARROW
    #endif
    };


    const CCharFormat * pCF;

    pContext = pContext ? pContext : GetFirstBranch();
    if (!pContext)
        goto Cleanup;

    pCF = pContext->GetCharFormat();
    Assert(pCF);

    if (!pDoc->_fEnableInteraction)
    {
        // Waiting for page to navigate.  Show wait cursor.
        idc = IDC_APPSTARTING;
    }
    else if ((!pDoc->_fDesignMode) && (pCF->_bCursorIdx))
    {
        // The style is set to something other than auto
        // so use aStyleToCursor to map the enum to a cursor id.

        Assert(pCF->_bCursorIdx >= 0 && pCF->_bCursorIdx < ARRAY_SIZE(aStyleToCursor));
        idc = aStyleToCursor[pCF->_bCursorIdx];
    }
    else if (idcArg)
    {
        idc = idcArg;
    }
    else
    {
        // We didn't handle it.

        idc = NULL;
    }

    if (idc)
    {
        SetCursorIDC(idc);
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

Cleanup:
    RRETURN1(hr, S_FALSE );
}


//+------------------------------------------------------------------------
//
//  Function:   IsIDMSuperscript
//
//  Synopsis:   Tests the element passed is a superscript
//
//  Arguments:  CTreeNode *  element to be tested.
//
//  Returns:    BOOL    TRUE if the element passed is a superscript element
//
//-----------------------------------------------------------------------------
BOOL
IsIDMSuperscript(CTreeNode * pNode)
{
    ELEMENT_TAG etag = pNode->Tag();

    return etag == ETAG_SUP;
}

//+------------------------------------------------------------------------
//
//  Function:   IsIDMSubscript
//
//  Synopsis:   Tests the element passed is a subscript
//
//  Arguments:  CTreeNode *   element to be tested.
//
//  Returns:    BOOL    TRUE if the element passed is a subscript element
//
//-----------------------------------------------------------------------------
BOOL
IsIDMSubscript(CTreeNode * pNode)
{
    ELEMENT_TAG etag = pNode->Tag();

    return etag == ETAG_SUB;
}


//+------------------------------------------------------------------------
//
//  Function:   IsIDMBold
//
//  Synopsis:   Tests the element passed is a Bold or Strong element
//
//  Arguments:  CTreeNode *   element to be tested.
//
//  Returns:    BOOL    TRUE if the element passed is a bold/strong element
//
//-----------------------------------------------------------------------------
BOOL
IsIDMBold(CTreeNode * pNode)
{
    ELEMENT_TAG etag = pNode->Tag();

    return (etag == ETAG_B || etag == ETAG_STRONG);
}


//+------------------------------------------------------------------------
//
//  Function:   IsIDMCharAttr
//
//  Synopsis:   Tests the element passed is a character attribute.  This
//              function is used to determine whether an element is a
//              candidate for deletion when the user selects "remove
//              character formatting" (aka "set to normal.")
//
//  Arguments:  CTreeNode *   element to be tested.
//
//  Returns:    BOOL    TRUE if the element passed is a not "normal"
//
//-----------------------------------------------------------------------------

BOOL
IsIDMCharAttr(CTreeNode * pNode)
{
    return pNode->Element()->HasFlag(TAGDESC_EDITREMOVABLE);
}

//+----------------------------------------------------------------------------
//
//  Function:   IsIDMItalic
//
//  Synopsis:   Tests the element passed is a Italic or Cite element
//
//  Arguments:  CTreeNode *   element to be tested.
//
//  Returns:    BOOL    TRUE if the element passed is a Italic/Cite element
//
//-----------------------------------------------------------------------------

BOOL
IsIDMItalic(CTreeNode * pNode)
{
    ELEMENT_TAG etag = pNode->Tag();

    return (etag == ETAG_I || etag == ETAG_EM || etag == ETAG_CITE);
}

//+----------------------------------------------------------------------------
//
//  Function:   IsIDMUnderlined
//
//  Synopsis:   Tests the element passed is a Underline element
//
//  Arguments:  CTreeNode *   element to be tested.
//
//  Returns:    BOOL    TRUE if the element passed is a Underline element
//
//-----------------------------------------------------------------------------

BOOL
IsIDMUnderlined(CTreeNode * pNode)
{
    return (pNode->Tag() == ETAG_U);
}


//+----------------------------------------------------------------------------
//
//  Function:   WriteTag
//
//  Synopsis:   writes an open/end tag to the stream buffer
//
//  Arguments:  pStreamWrBuff   -   stream buffer
//              fEnd            -   TRUE if End tag is to be written out
//
//  Returns:    S_OK    if  successful
//
//-----------------------------------------------------------------------------

HRESULT
CElement::WriteTag(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd, BOOL fForce)
{
    HRESULT         hr = S_OK;
    DWORD           dwOldFlags = pStreamWrBuff->ClearFlags(WBF_ENTITYREF);
    const TCHAR *   pszTagName = TagName();
    const TCHAR *   pszScopeName;
    ELEMENT_TAG     etag = Tag();

    //
    // Do not write tags out in plaintext mode or when we are
    // explicitly asked not to.
    //
    if (pStreamWrBuff->TestFlag(WBF_SAVE_PLAINTEXT)
        || !pszTagName[0]
        || (this == pStreamWrBuff->GetElementContext()
            && pStreamWrBuff->TestFlag(WBF_NO_TAG_FOR_CONTEXT)
           )
        || (fEnd
            && (TagHasNoEndTag(Tag())
                || (!_fExplicitEndTag
                    && !HasFlag(TAGDESC_SAVEALWAYSEND)
                    && !fForce
                   )
               )
           )
       )
    {
        goto Cleanup;
    }

    if(fEnd)
    {
        hr = pStreamWrBuff->Write(_T("</"), 2);
    }
    else
    {
        if (Namespace())
        {
            LPTSTR  pchUrn;

            hr = THR(GetUrn(&pchUrn));
            if (hr)
                goto Cleanup;

            hr = THR(pStreamWrBuff->EnsureNamespaceSaved(
                Doc(), (LPTSTR)Namespace(), pchUrn, XMLNAMESPACETYPE_TAG));
            if (hr)
                goto Cleanup;
        }

        if (pStreamWrBuff->TestFlag(WBF_FOR_RTF_CONV) &&
            Tag() == ETAG_DIV)
        {
            // For the RTF converter, transform DIV tags into P tags.
            pszTagName = SZTAG_P;
        }

        // NOTE: In IE4, we would save a NewLine before every
        // element that was a ped.  This is roughly equivalent
        // to saving it before every element that is a container now
        // However, this does not round trip properly so I'm taking
        // that check out.

        if (    ! pStreamWrBuff->TestFlag(WBF_NO_PRETTY_CRLF)
            &&  (   HasFlag(TAGDESC_SAVETAGOWNLINE)
                ||  IsBlockTag() ) )
        {
            hr = pStreamWrBuff->NewLine();
            if (hr)
                goto Cleanup;
        }
        hr = pStreamWrBuff->Write(_T("<"), 1);
    }

    if(hr)
        goto Cleanup;


    pszScopeName = pStreamWrBuff->TestFlag(WBF_SAVE_FOR_PRINTDOC) && 
                   pStreamWrBuff->TestFlag(WBF_SAVE_FOR_XML) &&
                   Tag() != ETAG_GENERIC
                        ? NamespaceHtml() : Namespace();
    if (pszScopeName)
    {
        hr = pStreamWrBuff->Write(pszScopeName);
        if(hr)
            goto Cleanup;

        hr = pStreamWrBuff->Write(_T(":"));
        if(hr)
            goto Cleanup;
    }

    hr = pStreamWrBuff->Write(pszTagName);
    if(hr)
        goto Cleanup;

    if(!fEnd)
    {
        BOOL fAny;

        hr = SaveAttributes(pStreamWrBuff, &fAny);
        if(hr)
            goto Cleanup;

        if (ETAG_HTML == etag)
        {
            Assert (IsInMarkup());

            hr = THR(GetMarkup()->SaveXmlNamespaceAttrs(pStreamWrBuff));
            if (hr)
                goto Cleanup;
        }
    }

    hr = pStreamWrBuff->Write(_T(">"), 1);
    if (hr)
        goto Cleanup;

#if 0
    if(fEnd && HasFlag(TAGDESC_SAVENEWLINEATEND))
    {
            hr = pStreamWrBuff->NewLine();
            if (hr)
                    goto Cleanup;
    }
#endif

Cleanup:
    pStreamWrBuff->RestoreFlags(dwOldFlags);

    RRETURN(hr);
}


HRESULT STDMETHODCALLTYPE
CElement::scrollIntoView(VARIANTARG varargStart)
{
    HRESULT                hr;
    SCROLLPIN              spVert;
    BOOL                   fStart;
    CVariant               varBOOLStart;

    hr = THR(varBOOLStart.CoerceVariantArg(&varargStart, VT_BOOL));
    if ( hr == S_OK )
    {
        fStart = V_BOOL(&varBOOLStart);
    }
    else if ( hr == S_FALSE )
    {
        // when no argument
        fStart = TRUE;
    }
    else
        goto Cleanup;

    spVert = fStart ? SP_TOPLEFT : SP_BOTTOMRIGHT;

    hr = THR(ScrollIntoView(spVert, SP_TOPLEFT));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::DeferScrollIntoView(SCROLLPIN spVert, SCROLLPIN spHorz )
{
    HRESULT hr;

    GWKillMethodCall (this, ONCALL_METHOD(CElement, DeferredScrollIntoView, deferredscrollintoview), 0);
    hr = THR(GWPostMethodCall (this, ONCALL_METHOD(CElement, DeferredScrollIntoView, deferredscrollintoview),
                                (DWORD_PTR)(spVert | (spHorz << 16)), FALSE, "CElement::DeferredScrollIntoView"));
    return hr;
}

void
CElement::DeferredScrollIntoView(DWORD_PTR dwParam)
{
    SCROLLPIN spVert = (SCROLLPIN)((DWORD)dwParam & 0xffff);
    SCROLLPIN spHorz = (SCROLLPIN)((DWORD)dwParam >> 16);

    IGNORE_HR(ScrollIntoView(spVert, spHorz));
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::GetAtomTable, virtual override from CBase
//
//-------------------------------------------------------------------------

CAtomTable *
CElement::GetAtomTable (BOOL *pfExpando)
{
    CAtomTable  *pat = NULL;
    CDoc        *pDoc;

    pDoc = Doc();
    if (pDoc)
    {
        pat = &(pDoc->_AtomTable);
        if (pfExpando)
        {
            *pfExpando = pDoc->_fExpando;
        }
    }

    // We should have an atom table otherwise there's a problem.
    Assert(pat && "Element is not associated with a CDoc.");

    return pat;
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::GetPlainTextInScope
//
//  Synopsis:   Returns a text string containing all the plain text in the
//              scope of this element. The caller must free the memory.
//              This function can be used to merely retrieve the text
//              length by setting ppchText to NULL.
//
//  Arguments:  pstrText    If NULL,
//                              no text is returned.
//                          If not NULL but there is no text,
//                              *pstrText is set to NULL.
//                          Otherwise
//                              *pstrText points to a new CStr
//
//-------------------------------------------------------------------------

HRESULT
CElement::GetPlainTextInScope(CStr * pstrText)
{
    HRESULT     hr = S_OK;
    long        cp, cch;

    Assert(pstrText);

    if(!IsInMarkup())
    {
        pstrText->Set(NULL);
        goto Cleanup;
    }

    cp = GetFirstCp();
    cch = GetElementCch();

    {
        CTxtPtr     tp( GetMarkup(), cp );

        cch = tp.GetPlainTextLength(cch);

        // copy text into buffer

        pstrText->SetLengthNoAlloc(0);
        pstrText->ReAlloc(cch);

        cch = tp.GetPlainText(cch, (LPTSTR)*pstrText);

        Assert(cch >= 0);

        if (cch)
        {
            // Terminate with 0. GetPlainText() does not seem to do this.
            pstrText->SetLengthNoAlloc(cch);
            *(LPTSTR(*pstrText) + cch) = 0;
        }
        else
        {
            // just making sure...
            pstrText->Free();
        }
    }

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
// Member: GetAccessKey & MatchAccessKey
//
//-----------------------------------------------------------------------------

// In lieu of the C Run-time function _totlower() that we are not linking with
#define TOTLOWER(ch)  (TCHAR((DWORD)(DWORD_PTR)CharLower((LPTSTR)(DWORD_PTR)ch)))
// BUGBUG - yinxie, need to make sure this will work for non IBM compatible keyboards
// Caps Lock scan code = 0x3a, all keys higher than this will not be allowed in access key
#ifndef UNIX
#define ISVALIDKEY(x)   (((x >> 16) & 0x00FF) < 0x3a)
#else
inline BOOL ISVALIDKEY(LPARAM x) { return (MwCharacterFromWM_KEY(x) != 0); }
#endif

#define VIRTKEY_TO_SCAN 0

BOOL
CElement::MatchAccessKey(CMessage * pmsg)
{
    BOOL    fMatched = FALSE;
    LPCTSTR lpAccKey = GetAAaccessKey();
    WCHAR chKey = (TCHAR) pmsg->wParam;

    // Raid 57053
    // If we are in HTML dialog, accessKey can be matched with/without
    // SHIFT/CTRL/ALT keys.
    //

    // 60711 - Translate the virtkey to unicode for foreign languages.
    // We only test for 0x20 (space key) or above. This way we avoid
    // coming in here for other system type keys
    if(pmsg->message == WM_SYSKEYDOWN && pmsg->wParam > 31)
    {
        BYTE bKeyState[256];
        if(GetKeyboardState(bKeyState))
        {
            WORD cBuf[2];
            int cchBuf;

            UINT wScanCode = MapVirtualKeyEx(pmsg->wParam, VIRTKEY_TO_SCAN, GetKeyboardLayout(0));
            cchBuf = ToAscii(pmsg->wParam, wScanCode, bKeyState, cBuf, 0);

            if(cchBuf == 1)
            {
                WCHAR wBuf[2];
                UINT  uKbdCodePage = GetKeyboardCodePage();

                MultiByteToWideChar(uKbdCodePage, 0, (char *)cBuf, 2, wBuf, 2);
                chKey = wBuf[0];
            }
        }
    }   
 
    if ((pmsg->message == WM_SYSKEYDOWN ||
                    (Doc()->_fInHTMLDlg && pmsg->message == WM_CHAR))
            && lpAccKey
            && lpAccKey[0]
            && TOTLOWER((TCHAR) chKey) == TOTLOWER(lpAccKey[0])
            && ISVALIDKEY(pmsg->lParam))
    {
        fMatched = TRUE;
    }
    return fMatched;
}

//+-------------------------------------------------------------------------
//
// Member::    CElement::ShowMessage
//
//--------------------------------------------------------------------------

HRESULT __cdecl
CElement::ShowMessage(
        int * pnResult,
        DWORD dwFlags,
        DWORD dwHelpContext,
        UINT  idsMessage, ...)
{
    CDoc *              pDoc = Doc();
    HRESULT             hr = S_OK;
    va_list             arg;

    va_start(arg, idsMessage);

    if (pDoc)
    {
        hr = THR(pDoc->ShowMessageV(
            pnResult,
            dwFlags,
            dwHelpContext,
            idsMessage,
            &arg));
    }

    va_end(arg);
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
// Member::    CElement::ShowMessageV
//
//--------------------------------------------------------------------------

HRESULT
CElement::ShowMessageV(
                int   * pnResult,
                DWORD   dwFlags,
                DWORD   dwHelpContext,
                UINT    idsMessage,
                void  * pvArgs)
{
    RRETURN (Doc()->ShowMessageV(pnResult, dwFlags, dwHelpContext, idsMessage, pvArgs));
}

//+-------------------------------------------------------------------------
//
// Member::    CElement::ShowLastErrorInfo
//
//--------------------------------------------------------------------------

HRESULT
CElement::ShowLastErrorInfo(HRESULT hr, int iIDSDefault)
{
    RRETURN (Doc()->ShowLastErrorInfo(hr, iIDSDefault));
}

//+-------------------------------------------------------------------------
//
// Member::    CElement::ShowHelp
//
//--------------------------------------------------------------------------

HRESULT
CElement::ShowHelp(TCHAR * szHelpFile, DWORD dwData, UINT uCmd, POINT pt)
{
    RRETURN (Doc()->ShowHelp(szHelpFile, dwData, uCmd, pt));
}

CBase *CElement::GetOmWindow ( void )
{
    CDoc *  pDoc = Doc();

    pDoc->EnsureOmWindow();
    return pDoc->_pOmWindow;
}

//+-------------------------------------------------------------------------
//
//  Method:     CElement::UndoManager
//
//  Synopsis:   Get the undo manager
//
//--------------------------------------------------------------------------

#ifndef NO_EDIT
IOleUndoManager *
CElement::UndoManager()
{
    return Doc()->UndoManager();
}
#endif // NO_EDIT

//+-------------------------------------------------------------------------
//
//  Method:     CElement::QueryCreateUndo
//
//  Synopsis:   Query whether to create undo or not.  Also dirties the doc.
//
//--------------------------------------------------------------------------

#ifndef NO_EDIT
BOOL
CElement::QueryCreateUndo(BOOL fRequiresParent, BOOL fDirtyChange)
{
    return Doc()->QueryCreateUndo( fRequiresParent, fDirtyChange );
}
#endif // NO_EDIT


//+-------------------------------------------------------------------------
//
//  Method:     CElement::ShowTooltip
//
//  Synopsis:   Displays the tooltip for the site.
//
//  Arguments:  [pt]    Mouse position in container window coordinates
//              msg     Message passed to tooltip for Processing
//
//--------------------------------------------------------------------------

HRESULT
CElement::ShowTooltip(CMessage *pmsg, POINT pt)
{
    HRESULT hr = S_FALSE;
    RECT    rc;
    CDoc*   pDoc = Doc();
    TCHAR * pchString;
    BOOL fRTL = FALSE;

    if (pDoc->State() < OS_INPLACE)
        goto Cleanup;

#if DBG == 1
    TCHAR  achBuf[100];

    if (IsTagEnabled(tagFormatTooltips))
    {
        CTreeNode *pNode = pmsg->pNodeHit;

        Format(0,
               achBuf,
               100,
               L"<0s>  SN=<1d>\n_iCF=<2d>  _iPF=<3d>  _iFF=<4d>",
               pNode->Element()->TagName(),
               pNode->SN(),
               pNode->_iCF,
               pNode->_iPF,
               pNode->_iFF);

        pchString = achBuf;
    }
    else
#endif
    //
    // if there is a title property, use it as tooltip
    //

    pchString = (LPTSTR) GetAAtitle();
    if (pchString != NULL)
    {
        GetElementRc(&rc, GERC_CLIPPED | GERC_ONALINE, &pt);

        //It is possible to have an empty rect when an
        //element doesn't have a TxtSite above it.
        //Should this be an ASSERT or BUGBUG?
        if(IsRectEmpty(&rc))
        {
            rc.left = pt.x - 10;
            rc.right = pt.x + 10;
            rc.top = pt.y - 10;
            rc.bottom = pt.y + 10;
        }

        // Ignore spurious WM_ERASEBACKGROUNDs generated by tooltips
        CServer::CLock Lock(pDoc, SERVERLOCK_IGNOREERASEBKGND);

        // COMPLEXSCRIPT - determine if element is right to left for tooltip style setting
        if(GetFirstBranch())
        {
            fRTL = GetFirstBranch()->GetCharFormat()->_fRTL;
        }

        FormsShowTooltip(pchString, pDoc->_pInPlace->_hwnd, *pmsg, &rc, (DWORD_PTR) this, fRTL);
        hr = S_OK;
    }

Cleanup:
    return hr;
}

#ifndef WIN16
BEGIN_TEAROFF_TABLE(CElementFactory, IDispatchEx)
    //  IDispatch methods
    TEAROFF_METHOD(super, GetTypeInfoCount, gettypeinfocount, (UINT *pcTinfo))
    TEAROFF_METHOD(super, GetTypeInfo, gettypeinfo, (UINT itinfo, ULONG lcid, ITypeInfo ** ppTI))
    TEAROFF_METHOD(super, GetIDsOfNames, getidsofnames, (REFIID riid,
                                          LPOLESTR *prgpsz,
                                          UINT cpsz,
                                          LCID lcid,
                                          DISPID *prgid))
    TEAROFF_METHOD(super, Invoke, invoke, (DISPID dispidMember,
                                   REFIID riid,
                                   LCID lcid,
                                   WORD wFlags,
                                   DISPPARAMS * pdispparams,
                                   VARIANT * pvarResult,
                                   EXCEPINFO * pexcepinfo,
                                   UINT * puArgErr))
    TEAROFF_METHOD(super, GetDispID, getdispid, (BSTR bstrName,
                                      DWORD grfdex,
                                      DISPID *pid))
    TEAROFF_METHOD(CElementFactory, InvokeEx, invokeex, (DISPID id,
                             LCID lcid,
                             WORD wFlags,
                             DISPPARAMS *pdp,
                             VARIANT *pvarRes,
                             EXCEPINFO *pei,
                             IServiceProvider *pSrvProvider))
    TEAROFF_METHOD(super, DeleteMemberByName, deletememberbyname, (BSTR bstr,DWORD grfdex))
    TEAROFF_METHOD(super, DeleteMemberByDispID, deletememberbydispid, (DISPID id))
    TEAROFF_METHOD(super, GetMemberProperties, getmemberproperties, (DISPID id,
                                                DWORD grfdexFetch,
                                                DWORD *pgrfdex))
    TEAROFF_METHOD(super, GetMemberName, getmembername, (DISPID id,
                                          BSTR *pbstrName))
    TEAROFF_METHOD(super, GetNextDispID, getnextdispid, (DWORD grfdex,
                                          DISPID id,
                                          DISPID *pid))
    TEAROFF_METHOD(super, GetNameSpaceParent, getnamespaceparent, (IUnknown **ppunk))
END_TEAROFF_TABLE()
#endif

HRESULT
CElementFactory::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    HRESULT hr=S_OK;

    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
#ifndef WIN16
        QI_TEAROFF_DISPEX(this, NULL)
#else
    case Data1_IDispatchEx:
        CBase::PrivateQueryInterface(iid, ppv);
        break;
#endif
    QI_TEAROFF(this, IObjectIdentity, NULL)

    default:
        {
            const CLASSDESC *pclassdesc = (const CLASSDESC *) BaseDesc();
            if (pclassdesc &&
                pclassdesc->_apfnTearOff &&
                pclassdesc->_classdescBase._piidDispinterface &&
                (iid == *pclassdesc->_classdescBase._piidDispinterface ))
            {
                hr = THR(CreateTearOffThunk(this, (void *)(pclassdesc->_apfnTearOff), NULL, ppv));
            }
        }
    }

    if (!hr)
    {
        if (*ppv)
            (*(IUnknown **)ppv)->AddRef();
        else
            hr = E_NOINTERFACE;
    }
    RRETURN(hr);
}

HRESULT
CElementFactory::InvokeEx(DISPID dispidMember,
                          LCID lcid,
                          WORD wFlags,
                          DISPPARAMS * pdispparams,
                          VARIANT * pvarResult,
                          EXCEPINFO * pexcepinfo,
                          IServiceProvider *pSrvProvider)
{
    if ( (wFlags & DISPATCH_CONSTRUCT) && (dispidMember == DISPID_VALUE) )
    {
        // turn it into a method call ( expect dispid=0 )
        wFlags &= ~DISPATCH_CONSTRUCT;
        wFlags |= DISPATCH_METHOD;
    }

    RRETURN(super::InvokeEx(dispidMember,
                            lcid,
                            wFlags,
                            pdispparams,
                            pvarResult,
                            pexcepinfo,
                            pSrvProvider));
}

//+----------------------------------------------------------------------------
//
// Member:      CopyCommonAttributes
//
// Synopsis:    Copy the common attributes for a given element to the current
//              element (generally used when replacing an element with another
//              while editing).
//
//-----------------------------------------------------------------------------

HRESULT
CElement::MergeAttributes(CElement *pElementFrom, BOOL fOMCall, BOOL fCopyID)
{
    HRESULT         hr = S_OK;
    CAttrArray *    pInLineStyleAAFrom;
    CAttrArray **   ppInLineStyleAATo;
    CAttrArray *    pAAFrom = *(pElementFrom->GetAttrArray());
    CBase *         pelTarget = this;
    CMergeAttributesUndo Undo( this );

    if (pElementFrom->Tag() == Tag())
        pelTarget = NULL;

    // Allow only elements with the same tagName to be merged thru' the OM.
    if (fOMCall && pelTarget)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    Undo.SetWasNamed( _fIsNamed );
    Undo.SetCopyId( fCopyID );
    Undo.SetPassElTarget( !!pelTarget );

    if (pAAFrom)
    {
        CAttrArray     **ppAATo = GetAttrArray();

        hr = THR(pAAFrom->Merge(ppAATo, pelTarget, Undo.GetAA(), FALSE, fCopyID));
        if (hr)
            goto Cleanup;

        SetEventsShouldFire();

        // If the From has is a named element then the element is probably changed.
        if (pElementFrom->_fIsNamed)
        {
            _fIsNamed = TRUE;
            // Inval all collections affected by a name change
            DoElementNameChangeCollections();
        }

        pInLineStyleAAFrom = pElementFrom->GetInLineStyleAttrArray();
        if (pInLineStyleAAFrom && pInLineStyleAAFrom->Size())
        {
            ppInLineStyleAATo = CreateStyleAttrArray(DISPID_INTERNAL_INLINESTYLEAA);
            if (!ppInLineStyleAATo)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            hr = THR(pInLineStyleAAFrom->Merge(ppInLineStyleAATo, NULL, Undo.GetAAStyle()));
            if (hr)
                goto Cleanup;
        }
    }

    IGNORE_HR(Undo.CreateAndSubmit());

Cleanup:

    RRETURN(hr);
}

HRESULT
CElement::GetNameSpaceParent(IUnknown **ppunk)
{
    HRESULT     hr;
    CTreeNode * pNodeForm;
    CTreeNode * pNodeContext = GetFirstBranch();
    CMarkup *   pMarkup;

    if (!ppunk)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppunk = NULL;

    if (pNodeContext && pNodeContext->Parent())
    {
        pNodeForm = pNodeContext->Parent()->SearchBranchToRootForTag( ETAG_FORM );

        if (pNodeForm)
        {
            hr = THR( pNodeForm->GetElementInterface( IID_IDispatchEx, (void **) ppunk ) );

            goto Cleanup;
        }
    }

    pMarkup = IsInMarkup() ? GetMarkup() : Doc()->_pPrimaryMarkup;

    if (!pMarkup)
    {
        hr = S_OK;
        goto Cleanup;
    }
    else if (pMarkup->IsPrimaryMarkup())
    {
        // not in any markup or in primary markup, return omdoc as namespace  (GetMarkup()->QI returns it)
        hr = THR(pMarkup->QueryInterface(IID_IDispatchEx, (void**) ppunk));
    }
    else
    {
        // in non-primary markup, return DefaultDispatch
        hr = THR(pMarkup->GetDD()->PrivateQueryInterface(IID_IDispatchEx, (void**) ppunk));
    }

Cleanup:

    RRETURN( hr );
}

//+-------------------------------------------------------------------------
//
//  Method:     CElement::CanShow
//
//  Synopsis:   Determines whether an element can be shown at this moment.
//
//--------------------------------------------------------------------------
BOOL
CElement::CanShow()
{
    BOOL fRet = TRUE;
    CTreeNode *pNodeSite = GetFirstBranch()->GetCurNearestLayoutNode();

    while (pNodeSite)
    {
        if (!pNodeSite->Element()->GetInfo(GETINFO_ISCOMPLETED))
        {
            fRet = FALSE;
            break;
        }
        pNodeSite = pNodeSite->GetCurParentLayoutNode();
    }
    return fRet;
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::OnCssChangeStable
//
//-------------------------------------------------------------------------

HRESULT
CElement::OnCssChange(BOOL fStable, BOOL fRecomputePeers)
{
    HRESULT     hr = S_OK;

    if (IsInMarkup())
    {
        hr = THR(GetMarkup()->OnCssChange(fStable, fRecomputePeers));
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::Invalidate
//
//  Synopsis:   Invalidates the area occupied by the element.
//
//-------------------------------------------------------------------------
void
CElement::Invalidate()
{
    if(!GetFirstBranch())
        return;

    InvalidateElement();
}

//+-------------------------------------------------------------------------
//
//  Method:     GetElementRc
//
//  Synopsis:   Get the bounding rect for the element
//
//  Arguments:  prc:      the rc to be returned
//              dwFlags:  flags indicating desired behaviour
//                           GERC_ONALINE: rc on a line, line indicated by ppt.
//                           GERC_CLIPPED: rc clipped by visible client rect.
//              ppt:      the point around which we want the rect
//
//  Returns:    hr
//
//--------------------------------------------------------------------------

HRESULT
CElement::GetElementRc(RECT *prc, DWORD dwFlags, POINT *ppt)
{
    CDataAry<RECT> aryRects(Mt(CElementGetElementRc_aryRects_pv));
    HRESULT        hr = S_FALSE;
    LONG           i;

    Assert(prc);

    // Get the region for the element
    GetElementRegion(&aryRects,
                    !(dwFlags & GERC_ONALINE)
                        ? prc
                        : NULL,
                    RFE_SCREENCOORD);

    if (dwFlags & GERC_ONALINE)
    {
        Assert(ppt);
        for (i = 0; i < aryRects.Size(); i++)
        {
            if (PtInRect(&aryRects[i], *ppt))
            {
                *prc = aryRects[i];
                hr = S_OK;
                break;
            }
        }
    }

    if (   (S_OK == hr)
        && (dwFlags & GERC_CLIPPED)
       )
    {
        RECT       rcVisible;
        CDispNode *pDispNode;
        CLayout   *pLayout = GetUpdatedNearestLayout();

        Assert(Doc()->GetPrimaryElementClient());
        pLayout = pLayout ? pLayout : Doc()->GetPrimaryElementClient()->GetUpdatedNearestLayout();

        Assert(pLayout);
        pDispNode = pLayout->GetElementDispNode(this);

        if (!pDispNode)
        {
            pDispNode = pLayout->GetElementDispNode(pLayout->ElementOwner());
        }


        if (pDispNode)
        {
            pDispNode->GetClippedBounds(&rcVisible, COORDSYS_GLOBAL);
            IntersectRect(prc, prc, &rcVisible);
        }
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
// Member:      CElement::ComputeHorzBorderAndPadding
//
// Synopsis:    Compute horizontal border and padding for a given element
//              The results represent cumulative border and padding up the
//              element's ancestor chain, up to but NOT INCLUDING element's
//              containing layout.  The layout's border should not be counted
//              when determining a contained element's indent, because it lies
//              outside the box boundary from which we are measuring.  The 
//              layout's padding usually does need to be accounted for; the caller
//              must do this! (via GetPadding on the layout's CDisplay).
//
//-----------------------------------------------------------------------------

void
CElement::ComputeHorzBorderAndPadding(CCalcInfo * pci, CTreeNode * pNodeContext, CElement * pElementStop,
                                  LONG * pxBorderLeft, LONG * pxPaddingLeft,
                                  LONG * pxBorderRight, LONG * pxPaddingRight)
{
    Assert(pNodeContext && SameScope(this, pNodeContext));

    Assert(pxBorderLeft || pxPaddingLeft || pxBorderRight || pxPaddingRight);

    CTreeNode * pNode = pNodeContext;
    CBorderInfo borderinfo;
    CElement *  pElement;
    const CFancyFormat * pFF;
    const CParaFormat *  pPF;

    Assert(pxBorderLeft && pxPaddingLeft && pxBorderRight && pxPaddingRight);

    *pxBorderLeft = 0;
    *pxBorderRight = 0;
    *pxPaddingLeft = 0;;
    *pxPaddingRight = 0;

    while(pNode && pNode->Element() != pElementStop)
    {
        pElement = pNode->Element();
        pFF = pNode->GetFancyFormat();
        pPF = pNode->GetParaFormat();
        
        if ( !pElement->_fDefinitelyNoBorders )
        {
            pElement->_fDefinitelyNoBorders = !GetBorderInfoHelper( pNode, pci, &borderinfo, FALSE );

            *pxBorderRight  += borderinfo.aiWidths[BORDER_RIGHT];
            *pxBorderLeft   += borderinfo.aiWidths[BORDER_LEFT];
        }

        *pxPaddingLeft  += pFF->_cuvPaddingLeft.XGetPixelValue(pci,
                                    pci->_sizeParent.cx, pPF->_lFontHeightTwips);
        *pxPaddingRight += pFF->_cuvPaddingRight.XGetPixelValue(pci,
                                    pci->_sizeParent.cx, pPF->_lFontHeightTwips);

        pNode = pNode->Parent();
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CElement::Clone
//
//  Synopsis:   Make a new one just like this one
//
//-------------------------------------------------------------------------

HRESULT
CElement::Clone ( CElement * * ppElementClone, CDoc * pDoc )
{
    HRESULT          hr;
    CAttrArray *     pAA;
    CStr             cstrPch;
    CElement       * pElementNew = NULL;
    const CTagDesc * ptd;
    CHtmTag          ht;
    BOOL             fDie = FALSE;
    ELEMENT_TAG      etag = Tag();
    
    Assert( ppElementClone );

    if (IsGenericTag( etag ))
    {
        hr = THR( cstrPch.Set( Namespace() ) );

        if (hr)
            goto Cleanup;

        hr = THR( cstrPch.Append( _T( ":" ) ) );

        if (hr)
            goto Cleanup;

        hr = THR( cstrPch.Append( TagName() ) );

        if (hr)
            goto Cleanup;
    }
    else if (etag == ETAG_UNKNOWN)
    {
        hr = THR( cstrPch.Append( TagName() ) );

        if (hr)
            goto Cleanup;
    }

    ptd = TagDescFromEtag( etag );

    if (!ptd)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    ht.Reset();
    ht.SetTag( etag );
    ht.SetPch( cstrPch );
    ht.SetCch( cstrPch.Length() );

    if (etag == ETAG_INPUT)
    {
        hr = THR(CInput::CreateElement(&ht, pDoc, &pElementNew, (htmlInput)(DYNCAST(CInput, this)->GetType())));
        if (hr)
            goto Cleanup;

        DYNCAST(CInput, pElementNew)->_fScriptCreated = TRUE;
    }
    else
        hr = ptd->_pfnElementCreator( & ht, pDoc, & pElementNew );

    if (hr)
        goto Cleanup;

    if (fDie)
        goto Die;

    hr = THR( pElementNew->Init() );

    if (hr)
        goto Cleanup;

    if (fDie)
        goto Die;

    pElementNew->_fBreakOnEmpty = _fBreakOnEmpty;
    pElementNew->_fExplicitEndTag = _fExplicitEndTag;

    pAA = * GetAttrArray();

    if (pAA)
    {
        CAttrArray * * ppAAClone = pElementNew->GetAttrArray();

        hr = THR( pAA->Clone( ppAAClone ) );

        if (hr)
            goto Cleanup;

        if (fDie)
            goto Die;
    }

    pElementNew->_fIsNamed = _fIsNamed;    

    {
        CInit2Context   context(&ht, GetMarkup(), INIT2FLAG_EXECUTE);

        hr = THR( pElementNew->Init2(&context) );
    }
    if (hr)
        goto Cleanup;

    if (etag == ETAG_INPUT)
    {
        CStr cstrValue;

        hr = THR(DYNCAST(CInput, this)->GetValueHelper(&cstrValue));
        if (hr)
            goto Cleanup;

        hr = THR(DYNCAST(CInput, pElementNew)->SetValueHelper(&cstrValue));
        if (hr)
            goto Cleanup;
    }


    if (fDie)
        goto Die;

     pElementNew->SetEventsShouldFire();

Cleanup:

    if (hr && pElementNew)
        CElement::ClearPtr( & pElementNew );

    *ppElementClone = pElementNew;

    RRETURN( hr );

Die:

    hr = E_ABORT;

    goto Cleanup;
}


//+---------------------------------------------------------------------------
//
//  Member:     CElement::StealAttributes, public
//
//  Synopsis:   Steals the attributes from another element.
//
//  Arguments:  [pElementVictim] -- Element to steal attributes from.
//
//  Returns:    HRESULT
//
//  Notes:      Used when we want to replace one element with another, after
//              we've computed all the formats and created the attribute bag.
//
//----------------------------------------------------------------------------

HRESULT
CElement::StealAttributes(CElement * pElementVictim)
{
    _pAA = pElementVictim->_pAA;
    pElementVictim->_pAA = NULL;


    // BUGBUG rgardner - Why do we do this ? Why not just steal the attr array & the format indices,
    // and set the indices in the victim to -1 ????


    // After we have stolen the attr array, we must adjust
    // any addrefs for images that the victim may be holding on to.

    if (pElementVictim->_fHasImage)
    {
        int n;
        long lCookie;

        // Add ref any image context cookies this element holds

        for (n = 0; n < ARRAY_SIZE(s_aryImgDispID); ++n)
        {
            if ( _pAA->FindSimple(s_aryImgDispID[n].cacheID, (DWORD *)&lCookie,
                                  CAttrValue::AA_Internal) )
            {
                CDoc *  pDoc = Doc();

                // Replace ref on victim with one on thief
                pDoc->AddRefUrlImgCtx(lCookie, this);
                pDoc->ReleaseUrlImgCtx(lCookie, pElementVictim);
            }
        }

        _fHasImage = TRUE;
        pElementVictim->_fHasImage = FALSE;
    }

    // some of the properties depend on the element being
    // a site so do not copy format indicies.

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CElement::AddExtension, public
//
//  Synopsis:   creates a CSS extension object (a.k.a. CSS Filter, not to be
//              mistaken for visual filters)
//
//  Returns:    HRESULT
//
//  Notes:      A side effect is that the _pFilter member of the
//              CSSFilterSite passed in is set
//
//----------------------------------------------------------------------------
HRESULT
CElement::AddExtension( TCHAR *name, CPropertyBag *pBag, CCSSFilterSite *pSite )
{
    HRESULT             hr=S_OK;
    HKEY                key;
    DWORD               dwType, dwLength;
    TCHAR               szClsid[78];
    CLSID               clsid;
    LONG                ret=0;
    ICSSFilter          *pFilter = NULL;
    IPersistPropertyBag *pPPB = NULL;
    IPersistPropertyBag2 *pPPB2 = NULL;

    Assert(name && _tcsclen(name) > 0);
    Assert(pSite);

    ret = RegOpenKey( HKEY_LOCAL_MACHINE,
                      _T("Software\\MICROSOFT\\Windows\\CurrentVersion\\Explorer\\CSSFilters"),
                      &key);
    if ( ERROR_SUCCESS == ret )
    {
        ret = RegQueryValueEx( key, name,
            0, &dwType, NULL,
            &dwLength );
        if ( ERROR_SUCCESS == ret )
        {
            if ( dwLength <= 78 )
            {
                ret = RegQueryValueEx( key, name,
                    0, &dwType,
                    (LPBYTE)szClsid,
                    &dwLength );
                if ( ERROR_SUCCESS == ret )
                    hr = CLSIDFromString( szClsid, &clsid );
            }
            else
                hr = E_FAIL;
        }
        RegCloseKey(key);
    }
    if ( !hr )
        hr = HRESULT_FROM_WIN32(ret);

    if ( FAILED(hr) )
        goto error;

    // got the CLSID for the extension object, create it and connect it
    hr = THR(CoCreateInstance( clsid, NULL, CLSCTX_INPROC_SERVER,
                               IID_ICSSFilter, (void **)&pFilter ));
    if ( FAILED(hr) )
        goto error;

    // make sure this object is safe to init. Try IPPBag2 first, then IPPBag
    Assert( pBag );
    hr = pFilter->QueryInterface( IID_IPersistPropertyBag2, (void **)&pPPB2 );
    if ( SUCCEEDED(hr) )
    {
        if ( IsSafeTo( SAFETY_INIT, IID_IPersistPropertyBag2,
                       clsid, (IUnknown *)pFilter, NULL) )
        {
            hr = pPPB2->Load( (IPropertyBag2 *)pBag, NULL );
            if ( FAILED(hr) )
                goto error;
        }
    }
    else
    {
        // go thru here if filter doesn't support IPPBag2
        hr = pFilter->QueryInterface( IID_IPersistPropertyBag, (void **)&pPPB );
        if ( SUCCEEDED(hr) )
        {
            if ( IsSafeTo( SAFETY_INIT, IID_IPersistPropertyBag,
                clsid, (IUnknown *)pFilter, NULL) )
            {
                hr = pPPB->Load( (IPropertyBag *)pBag, NULL );
                if ( FAILED(hr) )
                    goto error;
            }
        }
    }

    hr = pFilter->SetSite((ICSSFilterSite*)pSite );
    if ( FAILED(hr) )
        goto error;

    Assert( pSite->_pFilter == NULL );
    pSite->_pFilter = pFilter;

    // check to see if this filter is safe to script
    pSite->_fSafeToScript = IsSafeTo( SAFETY_SCRIPT, IID_IDispatch, clsid,
                                      (IUnknown *)pFilter, NULL );

cleanup:
    ReleaseInterface( pPPB );
    ReleaseInterface( pPPB2 );

    RRETURN(hr);

error:
    if ( pFilter )
        pFilter->Release();
    goto cleanup;
}

//+-----------------------------------------------------------------
//
//  members : get_filters
//
//  synopsis : IHTMLELement implementaion to return the filter collection
//
//-------------------------------------------------------------------

HRESULT
CElement::get_filters(IHTMLFiltersCollection **ppFilters)
{
    HRESULT     hr;
    CFilterArray *pFilterCollection;
    CTreeNode *pNode = 0;

    if (!ppFilters)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppFilters = NULL;

    pNode = GetUpdatedNearestLayoutNode();
    if (pNode)
    {
        pNode->GetFancyFormatIndex();
    }

    pFilterCollection = GetFilterCollectionPtr();

    //Get existing Filter Collection or create a new one
    if (pFilterCollection)
    {
        hr = THR_NOTRACE(pFilterCollection->QueryInterface(IID_IHTMLFiltersCollection,
            (VOID **)ppFilters));
    }
    else
    {
        pFilterCollection = new CFilterArray(this);
        if (!pFilterCollection)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR_NOTRACE(pFilterCollection->QueryInterface(IID_IHTMLFiltersCollection,
            (VOID **)ppFilters));
        if ( hr )
        {
            delete pFilterCollection;
            goto Cleanup;
        }

        AddPointer ( DISPID_INTERNAL_FILTERPTRCACHE,
                     (void *)pFilterCollection,
                     CAttrValue::AA_Internal );
        _fHasFilterCollectionPtr = TRUE;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CElement::toString(BSTR* String)
{
    RRETURN(super::toString(String));
};

HRESULT
CElement::setCapture(VARIANT_BOOL containerCapture)
{
    HRESULT hr = S_OK;
    CDoc * pDoc = Doc();

    if (!pDoc || (pDoc->State() < OS_INPLACE) || pDoc->_fOnLoseCapture)
        goto Cleanup;

    if (pDoc->_pElementOMCapture == this)
        goto Cleanup;

    hr = pDoc->releaseCapture();
    if (hr)
        goto Cleanup;

    pDoc->_fContainerCapture = (containerCapture == VB_TRUE);
    pDoc->_pElementOMCapture = this;
    SubAddRef();

    // CHROME
    // If Chrome hosted use the container's windowless interface
    // rather than Win32's GetCapture and an HWND as we are
    // windowless and have no HWND.
    if (!pDoc->IsChromeHosted() ? (::GetCapture() != pDoc->_pInPlace->_hwnd) : !pDoc->GetCapture())
    {
        // CHROME
        // If Chrome hosted use the container's windowless interface
        // rather than Win32's SetCapture and an HWND as we are
        // windowless and have no HWND.
        if (!pDoc->IsChromeHosted())
            ::SetCapture(pDoc->_pInPlace->_hwnd);
        else
            pDoc->SetCapture(TRUE);
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::releaseCapture()
{
    HRESULT hr = S_OK;
    CDoc * pDoc = Doc();

    if (!pDoc)
        goto Cleanup;

    if (pDoc->_pElementOMCapture == this)
        hr = pDoc->releaseCapture();

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

BOOL
CElement::WantTextChangeNotifications()
{
    if( ! HasLayout() )
        return FALSE;

    CFlowLayout * pFlowLayout = GetCurLayout()->IsFlowLayout();
    if( pFlowLayout && pFlowLayout->IsListening() )
        return TRUE;

    return FALSE;
}

void
CElement::Notify(CNotification * pnf)
{
    CFilterArray *  pFilterCollection;

    Assert(pnf);

    if (HasPeerHolder())
    {
        GetPeerHolder()->OnElementNotification(pnf);
    }

    switch (pnf->Type())
    {
    case NTYPE_AMBIENT_PROP_CHANGE:
        pFilterCollection = GetFilterCollectionPtr();
        if (pFilterCollection)
        {
            DISPID  dispid;

            pnf->Data(&dispid);
            pFilterCollection->OnAmbientPropertyChange(dispid);
        }
        Invalidate();
        break;

    case NTYPE_COMMAND:
        pFilterCollection = GetFilterCollectionPtr();
        if (pFilterCollection)
        {
            COnCommandExecParams *  pParams;

            pnf->Data((void **)&pParams);
            pFilterCollection->OnCommand(pParams);
        }
        break;

    case NTYPE_FAVORITES_LOAD:
    case NTYPE_XTAG_HISTORY_LOAD:
    case NTYPE_SNAP_SHOT_SAVE:
        {
            IHTMLPersistData *  pPersist = GetPeerPersist();

            if (pPersist)
            {
                CPtrAry<CElement *> *   pary;

                pnf->Data((void **)&pary);
                pary->Append(this);
                pPersist->Release();
            }
        }
        break;

    case NTYPE_FAVORITES_SAVE:
        {
            void * pv;

            pnf->Data(&pv);
            IGNORE_HR(TryPeerPersist(FAVORITES_SAVE, pv));
        }
        break;

    case NTYPE_XTAG_HISTORY_SAVE:
        {
            void * pv;

            pnf->Data(&pv);
            IGNORE_HR(TryPeerPersist(XTAG_HISTORY_SAVE, pv));
        }
        break;

    case NTYPE_DELAY_LOAD_HISTORY:
        {
            CDoc *  pDoc = Doc();

            if (pDoc->_historyCurElem.lIndex >= 0 &&
                HistoryCode() == pDoc->_historyCurElem.dwCode)
            {
                long   lSrcIndex  = GetSourceIndex();

                Assert(lSrcIndex >= 0);

                if ( !pDoc->_fUserInteracted &&
                     lSrcIndex == pDoc->_historyCurElem.lIndex)
                {
                    if (IsEnabled())
                    {
                        pDoc->GetRootDoc()->_fFirstTimeTab = FALSE;
                        BecomeCurrent(pDoc->_historyCurElem.lSubDivision);
                    }
                    
                    // found the current element
                    pDoc->_historyCurElem.lIndex = -1;
                    pDoc->_historyCurElem.lSubDivision = 0;
                }
            }
        }
        break;

    case NTYPE_ELEMENT_EXITTREE_1:
        if( HasPeerHolder() || HasAccObjPtr() )
        {
            pnf->SetData( pnf->DataAsDWORD() | EXITTREE_DELAYRELEASENEEDED );
        }

#ifndef NO_DATABINDING
        if (GetDataMemberManager())
        {
            GetDataMemberManager()->Notify(pnf);
        }
#endif
        ExitTree( pnf->DataAsDWORD() );

        break;

    case NTYPE_RELEASE_EXTERNAL_OBJECTS:
        // NOTE in most cases we don't receive the notification
        if (HasPeerHolder())
        {
            DelPeerHolder()->Release(); // delete the ptr and release peer holder
        }
        break;

    case NTYPE_RECOMPUTE_BEHAVIOR:
        IGNORE_HR(ProcessPeerTask(PEERTASK_RECOMPUTEBEHAVIORS));
        break;

    case NTYPE_ELEMENT_ENTERTREE:
        EnterTree();
        break;

#ifndef NO_DATABINDING
    case NTYPE_STOP_1:
    case NTYPE_STOP_2:
    case NTYPE_BEFORE_UNLOAD:
        if (GetDataMemberManager())
        {
            GetDataMemberManager()->Notify(pnf);
        }
        break;
#endif

    case NTYPE_VISIBILITY_CHANGE:
// BUGBUG: Sanitize this by making official throughout the code that layout-like notifications
//         targeted at positioned elements without layouts are re-directed to the nearest layout.
//         This redirection should be done in CMarkup::NotifyElement rather than ad hoc. (brendand)
        Assert(GetFirstBranch()->GetCascadedposition() == stylePositionrelative);
        Assert(!HasLayout());
        {
            CLayout *   pLayout = GetCurNearestLayout();

            if (pLayout)
            {
                WHEN_DBG(pnf->ResetSN());
                pLayout->Notify(pnf);
            }
        }
        break;

    case NTYPE_ELEMENT_INVAL_Z_DESCENDANTS:
        {
            // bugbug (carled) this notification should really be unnecessary. what it
            // (and the one above) are crying for is a Notifcation for OnPropertyChange
            // so that descendants can take specific action.  More generally, there are
            // other OPP things like VoidCachedInfo that are duplicating the Notification
            // logic (walking the subtree) which could get rolled into this.  unifiying 
            // these things could go a long way to streamlining the code, and preventing
            // inconsistencies and lots of workarounds. (like this notification)
            //
            // This notification is targeted at positioned elements which are children of 
            // the element that sent it. We do not want to delegate to the nearest Layout
            // since if they are positioned, they have thier own. Since the source element2
            // may not have a layout we have to use the element tree for the routing.

            CLayout *   pLayout = GetUpdatedLayout();

            if (pLayout && IsPositioned())
            {
                WHEN_DBG(pnf->ResetSN());
                pLayout->Notify(pnf);
            }
        }
        break;
    }

    return;
}

HRESULT
CElement::EnterTree()
{
    HRESULT         hr;
    CDoc           *pDoc = Doc();
    InlineEvts     *pInlineEvts;

    Assert( IsInMarkup() );

    // Connect up event handlers...
    pInlineEvts = GetEventsToHook();
    if (pInlineEvts)
    {
        pInlineEvts->Connect(pDoc, this);

        delete pInlineEvts;
        StoreEventsToHook(NULL);
    }

    hr = THR(ProcessPeerTask(PEERTASK_ENTERTREE_UNSTABLE));
    if (hr)
        goto Cleanup;

    if (IsInPrimaryMarkup())
    {
        //
        // These things only make sense on elements which are in the main
        // tree of the document, not on any old random tree that's been
        // created.
        //

        pDoc->OnElementEnter(this);
        AttachDataBindings();
    }

    if (HasLayout())
    {
        GetCurLayout()->Init();
    }

    if (GetMarkup()->CollectionCache())
    {
        OnEnterExitInvalidateCollections(FALSE);
    }

Cleanup:
    
    RRETURN (hr);
}



void
CElement::ExitTree( DWORD dwExitFlags )
{
    CMarkup *   pMarkup = GetMarkup();
    CDoc *      pDoc  = pMarkup->Doc();
    CLayout *   pLayout;

    Assert( IsInMarkup() );

    pDoc->OnElementExit(this, dwExitFlags);
    if( !(dwExitFlags & EXITTREE_DESTROY) )
    {
        pLayout = GetUpdatedLayout();

        if (( pLayout &&  pLayout->_fAdorned ) || 
            ( pDoc->_fDesignMode && HasLayout() ) )
        {
            //
            // We now send the notification for adorned elements - or 
            // any LayoutElement leaving the tree.
            //
            //
            // BUGBUG marka - make this work via the adorner telling the selection manager
            // to change state. Also make sure _fAdorned is cleared on removal of the Adorner
            //
            IUnknown* pUnknown = NULL;
            IGNORE_HR( this->QueryInterface( IID_IUnknown, ( void**) & pUnknown ));
            IGNORE_HR( pDoc->NotifySelection( SELECT_NOTIFY_EXIT_TREE , pUnknown ));
            ReleaseInterface( pUnknown );
        }
        pLayout = NULL;
    }

    if (pDoc->GetView()->HasAdorners())
    {
        Verify(pDoc->GetView()->OpenView());
        pDoc->GetView()->RemoveAdorners(this);
    }

    if (pMarkup->HasCollectionCache())
    {
        OnEnterExitInvalidateCollections(FALSE);
    }

    //
    // Filters do not survive tree changes
    //
    if (_fHasPendingFilterTask)
    {
        pDoc->RemoveFilterTask(this);
        Assert(!_fHasPendingFilterTask);
    }

    //
    // In theory, recalc tasks could be left
    // on but given that ComputeFormats will
    // be called again anyway, this is safer
    //
    pDoc->GetView()->RemoveRecalcTask(this);

    // If we don't have any lookasides we don't have to do *any* of the following tests!
    if (_fHasLookasidePtr)
    {
        if (HasPeerHolder() && !(dwExitFlags & EXITTREE_PASSIVATEPENDING))
        {
            IGNORE_HR(ProcessPeerTask(PEERTASK_EXITTREE_UNSTABLE));
        }

#ifndef NO_DATABINDING
        if (HasDataBindPtr())
        {
            DetachDataBindings();
        }
#endif

        // BUGBUG: all filter logic should be moved to Layout::Detach
        // Detach the view filters
        // n.b., according to an old comment here, the filter must
        // be detached before the surfaces are cleaned up (now inside
        // CLayout::Detach).
        //
        // Addendum to the above.  (michaelw).
        //
        // The above isn't quite right.  Given that the OM of the filters must be
        // exposed even if we don't have a layout, the filter objects must be
        // instantiated and Detached by the element.
        //

        if (HasFilterPtr())
        {
            GetFilterPtr()->Detach();
        }

        if (HasPeerMgr())
        {
            CPeerMgr * pPeerMgr = GetPeerMgr();
            pPeerMgr->OnExitTree();
        }
    }

    if (HasLayoutPtr())
    {
        CLayout *   pLayout;

        GetCurLayout()->OnExitTree();

        if (!_fSite)
        {
            pLayout = DelLayoutPtr();

            pLayout->Detach();

            Assert(pLayout->_fDetached);

            pLayout->Release();
        }
    }
}

//+---------------------------------------------------------------------------------
//
//  Member :    CElement::IsEqualObject()
//
//  Synopsis :  IObjectIdentity method implementation. it direct comparison of two
//              pUnks fails, then the script engines will QI for IObjectIdentity and
//              call IsEqualObject one one, passing in the other pointer.
//
//   Returns : S_OK if the Objects are the same
//             E_FAIL if the objects are different
//
//----------------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CElement::IsEqualObject(IUnknown * pUnk)
{
    HRESULT      hr = E_POINTER;
    IServiceProvider * pISP = NULL;
    IUnknown   * pUnkTarget = NULL;
    IUnknown   * pUnkThis = NULL;

    if (!pUnk)
        goto Cleanup;

    hr = THR_NOTRACE(QueryInterface(IID_IUnknown, (void **)&pUnkThis));
    if (hr)
        goto Cleanup;

    if (pUnk == pUnkThis)
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr = THR_NOTRACE(pUnk->QueryInterface(IID_IServiceProvider, (void**)&pISP));
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(pISP->QueryService(SID_ELEMENT_SCOPE_OBJECT,
                                        IID_IUnknown,
                                        (void**)&pUnkTarget));
    if (hr)
        goto Cleanup;

    hr = (pUnkThis == pUnkTarget) ? S_OK : S_FALSE;

Cleanup:
    ReleaseInterface(pUnkThis);
    ReleaseInterface(pUnkTarget);
    ReleaseInterface(pISP);
    RRETURN1(hr, S_FALSE);
}

//+----------------------------------------------------------------------------
//
//  Method : CElement :: QueryService
//
//  Synopsis : IServiceProvider methoid Implementaion.
//
//-----------------------------------------------------------------------------
HRESULT
CElement::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    HRESULT hr = E_POINTER;

    if (!ppvObject)
        goto Cleanup;

    if (IsEqualGUID(guidService, SID_ELEMENT_SCOPE_OBJECT))
    {
        hr = THR_NOTRACE(QueryInterface(riid, ppvObject));
    }
#ifdef VSTUDIO7
    else if (IsEqualGUID(guidService, IID_IIdentityBehavior))
    {
        CPeerHolder *   pPeerHolder = NULL;

        hr = EnsureIdentityPeer(TRUE);
        if (hr)
            goto Cleanup;

        pPeerHolder = GetIdentityPeerHolder();
        Assert(pPeerHolder);
        Assert(pPeerHolder->_pPeer);

        hr = pPeerHolder->_pPeer->QueryInterface(riid, ppvObject);
    }
#endif //VSTUDIO7
    else
    {
        hr = THR_NOTRACE(super::QueryService(guidService, riid, ppvObject));
        if (hr == E_NOINTERFACE)
        {
            hr = THR_NOTRACE(Doc()->QueryService(guidService, riid, ppvObject));
        }
    }

Cleanup:
    RRETURN1( hr, E_NOINTERFACE);
}

//+-------------------------------------------------------------------------
//
//  Method:     CElement::ContextThunk_Invoke
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

#ifdef USE_STACK_SPEW
#pragma check_stack(off)
#endif

STDMETHODIMP
CElement::ContextThunk_Invoke(
        DISPID dispidMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS * pdispparams,
        VARIANT * pvarResult,
        EXCEPINFO * pexcepinfo,
        UINT *)
{
    IUnknown * pUnkContext;

    // Magic macro which pulls context out of nowhere (actually eax)
    CONTEXTTHUNK_SETCONTEXT

    HRESULT         hr;
    IDispatchEx    *pDispEx;

    if (!IsEqualIID(riid, IID_NULL))
        RRETURN(E_INVALIDARG);

    if (!pUnkContext)
        pUnkContext = (IUnknown*)this;

    if (HasPeerHolder())
    {
        hr = THR_NOTRACE(GetPeerHolder()->InvokeExMulti(
            dispidMember,
            lcid,
            wFlags,
            pdispparams,
            pvarResult,
            pexcepinfo,
            NULL));
        if (DISP_E_MEMBERNOTFOUND != hr) // if succeced or failed with error other then DISP_E_MEMBERNOTFOUND
            goto Cleanup;
    }

    hr = THR(pUnkContext->QueryInterface(IID_IDispatchEx, (void **)&pDispEx));
    if (hr)
    {
        // Object doesn't support IDispatchEx use CBase::InvokeEx
        hr = ContextInvokeEx(
            dispidMember,
            lcid,
            wFlags,
            pdispparams,
            pvarResult,
            pexcepinfo,
            NULL,
            pUnkContext ? pUnkContext : (IUnknown*)this);
    }
    else
    {
        // Object supports IDispatchEx call InvokeEx thru it's v-table.
        hr = pDispEx->InvokeEx(dispidMember, lcid, wFlags, pdispparams,pvarResult, pexcepinfo, NULL);
        ReleaseInterface(pDispEx);
    }

Cleanup:
    RRETURN(hr);
}

#ifdef USE_STACK_SPEW
#pragma check_stack(on)
#endif


//+-------------------------------------------------------------------------
//
//  Method:     CElement::ContextThunk_InvokeEx, IDispatchEx
//
//  Synopsis:   Gets node context from eax and passes it to ContextInvokeEx
//
//--------------------------------------------------------------------------

#ifdef USE_STACK_SPEW
#pragma check_stack(off)
#endif

STDMETHODIMP CElement::ContextThunk_InvokeEx(DISPID dispidMember,
                                             LCID lcid,
                                             WORD wFlags,
                                             DISPPARAMS * pdispparams,
                                             VARIANT * pvarResult,
                                             EXCEPINFO * pexcepinfo,
                                             IServiceProvider *pSrvProvider)
{
    HRESULT     hr;
    IUnknown *  pUnkContext;

    // Magic macro which pulls context out of nowhere (actually eax)
    CONTEXTTHUNK_SETCONTEXT

    hr = THR_NOTRACE(CElement::ContextInvokeEx(dispidMember,
                                  lcid,
                                  wFlags,
                                  pdispparams,
                                  pvarResult,
                                  pexcepinfo,
                                  pSrvProvider,
                                  pUnkContext ? pUnkContext : (IUnknown*)this));

    RRETURN (hr);
}

#ifdef USE_STACK_SPEW
#pragma check_stack(on)
#endif

//+-------------------------------------------------------------------------
//
//  Method:     CElement::ContextInvokeEx, IDispatchEx
//
//  Synopsis:   Real implementation of InvokeEx.  Uses context passed
//              in for actual calls
//
//--------------------------------------------------------------------------

STDMETHODIMP CElement::ContextInvokeEx(DISPID dispidMember,
                                    LCID lcid,
                                    WORD wFlags,
                                    DISPPARAMS * pdispparams,
                                    VARIANT * pvarResult,
                                    EXCEPINFO * pexcepinfo,
                                    IServiceProvider *pSrvProvider,
                                    IUnknown *pUnkContext)
{
    HRESULT     hr;

    if (HasPeerHolder())
    {
        hr = THR_NOTRACE(GetPeerHolder()->InvokeExMulti(
            dispidMember,
            lcid,
            wFlags,
            pdispparams,
            pvarResult,
            pexcepinfo,
            pSrvProvider));
        if (DISP_E_MEMBERNOTFOUND != hr) // if succeced or failed with error other then DISP_E_MEMBERNOTFOUND
            goto Cleanup;
    }

    hr = THR_NOTRACE(CBase::ContextInvokeEx(dispidMember,
                                  lcid,
                                  wFlags,
                                  pdispparams,
                                  pvarResult,
                                  pexcepinfo,
                                  pSrvProvider,
                                  pUnkContext ? pUnkContext : (IUnknown*)this));

Cleanup:
    RRETURN (hr);
}



//+----------------------------------------------------------------------------------
//
//  Member:     CElement::GetDispID, per IDispatchEx
//
//-----------------------------------------------------------------------------------

HRESULT
CElement::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HRESULT         hr;

    Doc()->PeerDequeueTasks(0);

    if (HasPeerHolder())
    {
        hr = THR_NOTRACE(GetPeerHolder()->GetDispIDMulti(bstrName, grfdex, pid));
        if (DISP_E_UNKNOWNNAME != hr) // if succeeded or failed with error other then DISP_E_UNKNOWNNAME
            goto Cleanup;
    }

    hr = THR_NOTRACE(super::GetDispID(bstrName, grfdex, pid));

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------------
//
//  Member:     CElement::GetExpandoDispID, helper
//
//-----------------------------------------------------------------------------------

HRESULT
CElement::GetExpandoDispID(BSTR bstrName, DISPID * pid, DWORD grfdex)
{
    HRESULT     hr;

    hr = THR_NOTRACE(super::GetExpandoDispID(bstrName, pid, grfdex));
    if (hr)
        goto Cleanup;

    if (TestClassFlag(ELEMENTDESC_OLESITE))
    {
        DYNCAST(COleSite, this)->RemapActivexExpandoDispid(pid);
    }

Cleanup:

    RRETURN (hr);
}


//+----------------------------------------------------------------------------------
//
//  Member:     CElement::GetNextDispID, per IDispatchEx
//
//-----------------------------------------------------------------------------------

HRESULT
CElement::GetNextDispID(DWORD grfdex, DISPID dispid, DISPID * pdispid)
{
    HRESULT         hr = S_FALSE;
    CPeerHolder *   pPeerHolder;

    if (!IsPeerDispid(dispid))
    {
        hr = THR_NOTRACE(super::GetNextDispID(grfdex, dispid, pdispid));
        if (S_FALSE != hr)  // if (S_OK == hr || FAILED(hr))
            goto Cleanup;
    }

    pPeerHolder = GetPeerHolder();

    if (pPeerHolder)
    {
        hr = THR_NOTRACE(pPeerHolder->GetNextDispIDMulti(grfdex, dispid, pdispid));
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+----------------------------------------------------------------------------------
//
//  Member:     CElement::GetMemberName, per IDispatchEx
//
//-----------------------------------------------------------------------------------

HRESULT
CElement::GetMemberName(DISPID dispid, BSTR * pbstrName)
{
    HRESULT         hr;

    if (IsPeerDispid(dispid))
    {
        CPeerHolder *   pPeerHolder = GetPeerHolder();

        if (pPeerHolder)
        {
            hr = THR_NOTRACE(pPeerHolder->GetMemberNameMulti(dispid, pbstrName));
        }
        else
        {
            hr = DISP_E_UNKNOWNNAME;
        }

        goto Cleanup;   // done
    }

    hr = THR_NOTRACE(super::GetMemberName(dispid, pbstrName));

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetMultiTypeInfoCount, per IProvideMultipleClassInfo
//
//----------------------------------------------------------------------------

HRESULT
CElement::GetMultiTypeInfoCount(ULONG * pCount)
{
    HRESULT     hr;

    if (GetReadyState() < READYSTATE_COMPLETE)
        RRETURN (E_UNEXPECTED);

    Assert (pCount);
    if (!pCount)
        RRETURN (E_POINTER);

    hr = THR(super::GetMultiTypeInfoCount(pCount));
    if (hr)
        goto Cleanup;

    if (HasPeerHolder())
    {
        hr = THR(GetPeerHolder()->GetCustomEventsTypeInfoCount(pCount));
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetInfoOfIndex, per IProvideMultipleClassInfo
//
//----------------------------------------------------------------------------

HRESULT
CElement::GetInfoOfIndex(
    ULONG       iTI,
    DWORD       dwFlags,
    ITypeInfo** ppTICoClass,
    DWORD*      pdwTIFlags,
    ULONG*      pcdispidReserved,
    IID *       piidPrimary,
    IID *       piidSource)
{
    HRESULT         hr;

    if (GetReadyState() < READYSTATE_COMPLETE)
        RRETURN (E_UNEXPECTED);

    if (HasPeerHolder())
    {
        hr = THR(GetPeerHolder()->CreateCustomEventsTypeInfo(iTI, ppTICoClass));
        if (S_FALSE != hr)  // if (S_OK == hr || FAILED(hr))
            goto Cleanup;   // nothing more to do
        // S_FALSE indicated that this belongs to super
    }

    hr = THR(super::GetInfoOfIndex(
        iTI, dwFlags, ppTICoClass, pdwTIFlags, pcdispidReserved, piidPrimary, piidSource));

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::CreateTearOffThunk
//
//----------------------------------------------------------------------------

HRESULT
CElement::CreateTearOffThunk(
    void *      pvObject1,
    const void * apfn1,
    IUnknown *  pUnkOuter,
    void **     ppvThunk,
    void *      appropdescsInVtblOrder)
{
    CPeerHolder *   pPeerHolder = GetPeerHolder();
    if (pPeerHolder && pPeerHolder->TestFlag(CPeerHolder::LOCKINQI))
    {
        return ::CreateTearOffThunk(
            pvObject1,
            apfn1,
            pUnkOuter,
            ppvThunk,
            pPeerHolder,
            *(void **)(IUnknown *)pPeerHolder,
            QI_MASK | ADDREF_MASK | RELEASE_MASK,
            NULL,
            appropdescsInVtblOrder);
    }
    else
    {
        return ::CreateTearOffThunk(
            pvObject1,
            apfn1,
            pUnkOuter,
            ppvThunk,
            appropdescsInVtblOrder);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::CreateTearOffThunk
//
//----------------------------------------------------------------------------

HRESULT
CElement::CreateTearOffThunk(
    void*       pvObject1,
    const void * apfn1,
    IUnknown *  pUnkOuter,
    void **     ppvThunk,
    void *      pvObject2,
    void *      apfn2,
    DWORD       dwMask,
    const IID * const * apIID,
    void *      appropdescsInVtblOrder)
{
    CPeerHolder *   pPeerHolder = GetPeerHolder();
    if (pPeerHolder && pPeerHolder->TestFlag(CPeerHolder::LOCKINQI) && !pvObject2)
    {
        pvObject2 = pPeerHolder;
        apfn2 = *(void **)(IUnknown *)pPeerHolder;
        dwMask |= QI_MASK | ADDREF_MASK | RELEASE_MASK;
    }

    return ::CreateTearOffThunk(
        pvObject1,
        apfn1,
        pUnkOuter,
        ppvThunk,
        pvObject2,
        apfn2,
        dwMask,
        apIID,
        appropdescsInVtblOrder);
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::get_dir
//
//  Synopsis: Object model entry point to get the dir property.
//
//  Arguments:
//            [p]  - where to return BSTR containing the string
//
//  Returns:  S_OK                  - this element supports this property
//            DISP_E_MEMBERNOTFOUND - this element doesn't support this
//                                    property
//            E_OUTOFMEMORY         - memory allocation failure
//            E_POINTER             - NULL pointer to receive BSTR
//
//----------------------------------------------------------------------------
STDMETHODIMP
CElement::get_dir(BSTR * p)
{
    HRESULT hr;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if ( p )
        *p = NULL;

    hr = s_propdescCElementdir.b.GetEnumStringProperty(
                p,
                this,
                (CVoid *)(void *)(GetAttrArray()) );

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::put_dir
//
//  Synopsis: Object model entry point to put the dir property.
//
//  Arguments:
//            [v]  - BSTR containing the new property value (ltr or rtl)
//
//  Returns:  S_OK                  - successful
//            DISP_E_MEMBERNOTFOUND - this element doesn't support this
//                                    property
//            E_INVALIDARG          - the argument is not one of the enum string
//                                    values (ltr or rtl)
//
//----------------------------------------------------------------------------
STDMETHODIMP
CElement::put_dir(BSTR v)
{
    HRESULT hr;

    hr = s_propdescCElementdir.b.SetEnumStringProperty(
                v,
                this,
                (CVoid *)(void *)(GetAttrArray()) );

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Lookaside storage
//
//----------------------------------------------------------------------------

void *
CElement::GetLookasidePtr(int iPtr)
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
CElement::SetLookasidePtr(int iPtr, void * pvVal)
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
CElement::DelLookasidePtr(int iPtr)
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


STDMETHODIMP
CElement::put_onresize(VARIANT v)
{
    HRESULT hr;

    // only delegate to the window if this is body or frameset
    if (Tag()==ETAG_BODY || Tag()==ETAG_FRAMESET)
    {
        CDoc * pDoc = Doc();

        hr = THR(pDoc->EnsureOmWindow());
        if ( hr )
            goto Cleanup;

        hr = THR(pDoc->_pOmWindow->put_onresize(v));
    }
    else
    {
        hr = THR(s_propdescCElementonresize.a.HandleCodeProperty(
                    HANDLEPROP_SET | HANDLEPROP_AUTOMATION | (PROPTYPE_VARIANT << 16),
                    &v,
                    this,
                    CVOID_CAST(GetAttrArray())));
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

STDMETHODIMP
CElement::get_onresize(VARIANT * p)
{
    HRESULT hr;

    // only delegate to the window if this is body or frameset
    if (Tag()==ETAG_BODY || Tag()==ETAG_FRAMESET)
    {
        CDoc *pDoc = Doc();

        hr = THR(pDoc->EnsureOmWindow());
        if ( hr )
            goto Cleanup;

        hr = THR(pDoc->_pOmWindow->get_onresize(p));
    }
    else
    {
        hr = THR(s_propdescCElementonresize.a.HandleCodeProperty(
                    HANDLEPROP_AUTOMATION | (PROPTYPE_VARIANT << 16),
                    p,
                    this,
                    CVOID_CAST(GetAttrArray())));
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

void CElement::Fire_onfocus(DWORD_PTR dwContext)
{
    CDoc *pDoc = Doc();
    if (!IsInMarkup() || !pDoc->_pPrimaryMarkup)
        return;

    CDoc::CLock LockForm(pDoc, FORMLOCK_CURRENT);
    CLock LockFocus(this, ELEMENTLOCK_FOCUS);
    FireEvent(DISPID_EVMETH_ONFOCUS, DISPID_EVPROP_ONFOCUS, _T("focus"), (BYTE *) VTS_NONE);
}
void CElement::Fire_onblur(DWORD_PTR dwContext)
{
    CDoc *pDoc = Doc();
    if (!IsInMarkup() || !pDoc->_pPrimaryMarkup)
        return;

    CDoc::CLock LockForm(pDoc, FORMLOCK_CURRENT);
    CLock LockBlur(this, ELEMENTLOCK_BLUR);
    pDoc->_fModalDialogInOnblur = (BOOL)dwContext;
    FireEvent(DISPID_EVMETH_ONBLUR, DISPID_EVPROP_ONBLUR, _T("blur"), (BYTE *) VTS_NONE);
    pDoc->_fModalDialogInOnblur = FALSE;
}

//+---------------------------------------------------------------------------
//
//  Native accessibility support
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//  Member:   CElement::CreateAccObj
//
//  Synopsis: Creates and returns the proper accessibility object for this element
//
//  Arguments:
//
//
//----------------------------------------------------------------------------
CAccElement *
CElement::CreateAccObj()
{

    switch ( Tag() )
    {
        case ETAG_BODY:
        case ETAG_FRAMESET:
            return new CAccBody( this );
            break;

        case ETAG_A:
          return new CAccAnchor( this );
          break;

        case ETAG_AREA:
            return new CAccArea( this );
            break;

        case ETAG_BUTTON:
            return new CAccButton( this );
            break;

        case ETAG_IMG:
            return new CAccImage( this );
            break;

        case ETAG_MARQUEE:
            return new CAccMarquee( this );
            break;

        case ETAG_TEXTAREA:
            return new CAccEdit( this, FALSE);
            break;

        case ETAG_INPUT:
            {
                htmlInput type = DYNCAST(CInput, this)->GetAAtype();
                switch(type)
                {
                case htmlInputCheckbox:
                    return new CAccCheckbox( this );

                case htmlInputRadio:
                   return new CAccRadio( this );

                case htmlInputImage:
                    return new CAccInputImg( this );

                case htmlInputButton:
                case htmlInputReset:
                case htmlInputSubmit:
                    return new CAccButton( this );

                case htmlInputPassword:
                    return new CAccEdit( this, TRUE );

                case htmlInputText:
                case htmlInputHidden:
                    return new CAccEdit( this, FALSE );
                }
            }
            break;

        case ETAG_OBJECT:
        case ETAG_EMBED:
        case ETAG_APPLET:
            return new CAccObject( this );
            break;

        case ETAG_TABLE:
            return new CAccTable( this );
            break;

        case ETAG_TH:
        case ETAG_TD:
            return new CAccTableCell( this );
            break;

        case ETAG_SELECT:
            return new CAccSelect( this );
            break;

        default:
            // but wait there is one more check, if this element is normally
            // unsupported (not in the about cases) then we still want to
            // create an accObject if this element has an explicit tabstop
            if (GetAAtabIndex() != htmlTabIndexNotSet)
            {
                return new CAccTabStopped( this );
            }
    }

    return NULL;
}

CElement *
CElement::GetParentAncestorSafe(ELEMENT_TAG etag) const
{
    CTreeNode *pNode = GetFirstBranch();
    CElement  *p = NULL;
    if (pNode)
    {
        pNode = pNode->Parent();
        if (pNode)
        {
            pNode = pNode->Ancestor(etag);
            if (pNode)
                p = pNode->Element();
        }
    }
    return p;
}

CElement *
CElement::GetParentAncestorSafe(ELEMENT_TAG *arytag) const
{
    CTreeNode *pNode = GetFirstBranch();
    CElement  *p = NULL;
    if (pNode)
    {
        pNode = pNode->Parent();
        if (pNode)
        {
            pNode = pNode->Ancestor(arytag);
            if (pNode)
                p = pNode->Element();
        }
    }
    return p;
}

//
// CElement Collection Helpers - manage the WINDOW_COLLECTION
//
HRESULT
CElement::GetIDHelper ( CStr *pf )
{
    LPCTSTR lpszID = NULL;
    HRESULT hr;

    if (_pAA && _pAA->HasAnyAttribute())
        _pAA->FindString(DISPID_CElement_id, &lpszID);

    hr = THR(pf->Set ( lpszID ));

    return hr;
}

HRESULT 
CElement::SetIdentifierHelper ( LPCTSTR lpszValue, DISPID dspIDThis, DISPID dspOther1, DISPID dspOther2 )
{
    HRESULT     hr;
    BOOL        fNamed;
    CDoc *      pDoc = Doc();

    // unhook any old script attached to this element.
    if ((Tag() != ETAG_SCRIPT) && IsNamed())
        pDoc->CommitScripts(this, FALSE);

    hr = AddString (dspIDThis, lpszValue, CAttrValue::AA_Attribute );
    if ( !hr )
    {
        // Remember if we're named, so that if this element moves into a different tree
        // we can inval the appropriate collections
        fNamed = lpszValue && lpszValue [ 0 ];
 
        // We're named if NAME= or ID= something or we have a unique name
        if ( !fNamed )
        {
            LPCTSTR lpsz = NULL;
            if (_pAA && _pAA->HasAnyAttribute())
            {
                _pAA->FindString(dspOther1, &lpsz);
                if ( !(lpsz && *lpsz ) )
                    _pAA->FindString(dspOther2, &lpsz);
            }
            fNamed = lpsz && *lpsz;
        }
        
        _fIsNamed = fNamed;
        // Inval all collections affected by a name change
        DoElementNameChangeCollections();

        // hookup any script attached to this element thru new id.
        if (Tag() != ETAG_SCRIPT)
            pDoc->CommitScripts(this);
    }
    RRETURN(hr);
}

HRESULT
CElement::SetIDHelper ( CStr *pf )
{
    RRETURN(SetIdentifierHelper((LPCTSTR)(*pf),
        DISPID_CElement_id,
        STDPROPID_XOBJ_NAME,
        DISPID_CElement_uniqueName));   
}

HRESULT
CElement::GetnameHelper ( CStr *pf )
{
    LPCTSTR lpszID = NULL;
    HRESULT hr;

    if (_pAA && _pAA->HasAnyAttribute())
        _pAA->FindString(STDPROPID_XOBJ_NAME, &lpszID);

    hr = THR(pf->Set ( lpszID ));

    return hr;
}

HRESULT
CElement::SetnameHelper ( CStr *pf )
{
    RRETURN(SetIdentifierHelper((LPCTSTR)(*pf),
        STDPROPID_XOBJ_NAME,
        DISPID_CElement_id,
        DISPID_CElement_uniqueName));   
}

HRESULT
CElement::SetUniqueNameHelper ( LPCTSTR szUniqueName )
{
    RRETURN(SetIdentifierHelper( szUniqueName,
        DISPID_CElement_uniqueName,
        DISPID_CElement_id,
        STDPROPID_XOBJ_NAME));   
}

void
CElement::InvalidateCollection ( long lIndex )
{
    CMarkup *pMarkup;
    CCollectionCache *pCollCache;

    pMarkup = GetMarkup();
    if ( pMarkup )
    {
        pCollCache = pMarkup->CollectionCache();
        if ( pCollCache )
        {
            pCollCache->InvalidateItem ( lIndex );
        }
    }
}

//
// Replace CBase::removeAttribute, need to special case some DISPID's
//

HRESULT 
CElement::removeAttribute(BSTR strPropertyName, LONG lFlags, VARIANT_BOOL *pfSuccess)
{
    DISPID dispID;
    IDispatchEx *pDEX = NULL;

    if (pfSuccess)
        *pfSuccess = VB_FALSE;

    // BUGBUG rgardner should move the STDPROPID_XOBJ_STYLE
    // code from CBase::removeAttribute to here

    if (THR_NOTRACE(PrivateQueryInterface ( IID_IDispatchEx, (void**)&pDEX )))
        goto Cleanup;

    if (THR_NOTRACE(pDEX->GetDispID(strPropertyName, lFlags & GETMEMBER_CASE_SENSITIVE ?
                                                    fdexNameCaseSensitive : 0, &dispID)))
        goto Cleanup;

    if (!removeAttributeDispid(dispID))
        goto Cleanup;


    // If we remove name or ID, update the WINDOW_COLLECTION if needed
    // Don't need to deal with uniqueName here - it's not exposed externaly

    if ( dispID == DISPID_CElement_id || dispID == STDPROPID_XOBJ_NAME )
    {
        LPCTSTR lpszNameID = NULL;
        BOOL fNamed;

        // Named if ID or name is present
        if (_pAA && _pAA->HasAnyAttribute())
        {
            _pAA->FindString( dispID == DISPID_CElement_id ?
                    STDPROPID_XOBJ_NAME : 
                    DISPID_CElement_id, 
                &lpszNameID);
            if ( !(lpszNameID && *lpszNameID ) )
                _pAA->FindString(DISPID_CElement_uniqueName, &lpszNameID);
        }

        fNamed = lpszNameID && *lpszNameID;

        if ( fNamed != !!_fIsNamed )
        {
            // Inval all collections affected by a name change
            DoElementNameChangeCollections();
            _fIsNamed = fNamed;
        }
    }

    if (pfSuccess)
        *pfSuccess = VB_TRUE;

Cleanup:
    ReleaseInterface ( pDEX );

    RRETURN ( SetErrorInfo ( S_OK ) );
}


HRESULT
CElement::OnTabIndexChange()
{
    long    i;
    HRESULT hr = S_OK;
    CDoc *  pDoc = Doc();
    
    for (i = 0; i < pDoc->_aryFocusItems.Size(); i++)
    {
        if (pDoc->_aryFocusItems[i].pElement == this)
        {
            pDoc->_aryFocusItems.Delete(i);
        }
    }
    
    hr = THR(pDoc->InsertFocusArrayItem(this));
    if (hr) 
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}


void
CElement::OnEnterExitInvalidateCollections(BOOL fForceNamedBuild)
{
    //
    // Optimized collections
    // If a named (name= or ID=) element enters the tree, inval the collections
    //
    
    //
    // DEVNOTE rgardner
    // This code is tighly couples with CMarkup::AddToCollections and needs
    // to be kept in sync with any changes in that function
    //

    if (IsNamed() || fForceNamedBuild)
    {
        InvalidateCollection ( CMarkup::WINDOW_COLLECTION );
    }

    // Inval collections based on specific element types
    switch ( _etag )
    {
    case ETAG_LABEL:
        InvalidateCollection ( CMarkup::LABEL_COLLECTION );
        break;

    case ETAG_FRAME:
    case ETAG_IFRAME:
        InvalidateCollection ( CMarkup::FRAMES_COLLECTION );
        if ( IsNamed() || fForceNamedBuild )
            InvalidateCollection ( CMarkup::NAVDOCUMENT_COLLECTION );
        break;

    case ETAG_IMG:
        InvalidateCollection ( CMarkup::IMAGES_COLLECTION );
        if ( IsNamed() || fForceNamedBuild )
            InvalidateCollection ( CMarkup::NAVDOCUMENT_COLLECTION );
        break;

    case ETAG_OBJECT:
    case ETAG_APPLET:
        InvalidateCollection ( CMarkup::APPLETS_COLLECTION );
        if ( IsNamed() || fForceNamedBuild )
            InvalidateCollection ( CMarkup::NAVDOCUMENT_COLLECTION );
        break;

    case ETAG_SCRIPT:
        InvalidateCollection ( CMarkup::SCRIPTS_COLLECTION );
        break;

    case ETAG_MAP:
        InvalidateCollection ( CMarkup::MAPS_COLLECTION );
        break;

    case ETAG_AREA:
        InvalidateCollection ( CMarkup::LINKS_COLLECTION );
        break;

    case ETAG_EMBED:
        InvalidateCollection ( CMarkup::EMBEDS_COLLECTION );
        if ( IsNamed() || fForceNamedBuild )
            InvalidateCollection ( CMarkup::NAVDOCUMENT_COLLECTION );
        break;

    case ETAG_FORM:
        InvalidateCollection ( CMarkup::FORMS_COLLECTION );
        if ( IsNamed() || fForceNamedBuild )
            InvalidateCollection ( CMarkup::NAVDOCUMENT_COLLECTION );
        break;

    case ETAG_A:
        InvalidateCollection ( CMarkup::LINKS_COLLECTION );
        if (IsNamed() || fForceNamedBuild)
            InvalidateCollection ( CMarkup::ANCHORS_COLLECTION );
        break;
    }
}

void
CElement::DoElementNameChangeCollections(void)
{
    // Inval all the base collections, based on TAGNAme.
    // Force the WINDOW_COLLECTION to be built
    OnEnterExitInvalidateCollections ( TRUE );

    // Artificialy update the _lTreeVersion so any named collection derived from 
    // the ELEMENT_COLLECTION are inval'ed
    
// BUGBUG PERF Must not modify this unless document is changing!!!!
// What should really be done here is have another version number on the
// doc which this bumps, and have the collections record two version
// numbers, one for the tree, and another they can bump here.  THen to
// see if you are out of date, simply comapre the two pairs of version
// numbers.
    
    Doc()->UpdateDocTreeVersion();
}
HRESULT
InlineEvts::Connect(CDoc *pDoc, CElement *pElement)
{
    HRESULT     hr = S_OK;

    Assert(pDoc && pElement);

    //
    // Look over all scriptlets and hook them up
    //
    if (cScriptlets && pDoc->_pScriptCollection)
    {
        BOOL        fRunScript;
        LPCTSTR     pchLanguage = NULL;

        hr = THR(pDoc->ProcessURLAction(URLACTION_SCRIPT_RUN, &fRunScript));
        if (hr || !fRunScript)
            goto Cleanup;

        for (int i = 0; i < cScriptlets; i++)
        {
            IGNORE_HR(pElement->ConnectInlineEventHandler(
                adispidScriptlets[i],   // dispid
                adispidScriptlets[i],   // dispidCode - in this case it is the same
                aOffsetScriptlets[i],
                aLineScriptlets[i],
                TRUE,                   // fStandard
                &pchLanguage));
        }
    }

Cleanup:
    RRETURN(hr);
}

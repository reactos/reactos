//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       olesite.cxx
//
//  Contents:   Implementation of COleSite class
//
//  Classes:    COleSite
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_CGUID_H_
#define X_CGUID_H_
#include <cguid.h>
#endif

#ifndef X_SIMPDATA_H_
#define X_SIMPDATA_H_
#include <simpdata.h>
#endif

#ifndef X_BINDER_HXX_
#define X_BINDER_HXX_
#include <binder.hxx>       // for cdatasourceprovider
#endif

#ifndef X_VBCURSOR_VBDSC_H_
#define X_VBCURSOR_VBDSC_H_
#include <vbcursor/vbdsc.h> // for iid_ivbdsc
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"
#endif

#ifndef X_DBTASK_HXX_
#define X_DBTASK_HXX_
#include "dbtask.hxx"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#ifndef X_CODELOAD_HXX_
#define X_CODELOAD_HXX_
#include "codeload.hxx"
#endif

#ifndef X_OLEACC_H_
#define X_OLEACC_H_
#include "oleacc.h"
#endif

#ifndef X_OCIDL_H_
#define X_OCIDL_H_
#include <ocidl.h>
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx" // for drawplaceholder
#endif

#ifndef X_PROPBAG_HXX_
#define X_PROPBAG_HXX_
#include "propbag.hxx"
#endif

#ifndef X_VBCURSOR_OCDB_H_
#define X_VBCURSOR_OCDB_H_
#include "vbcursor/ocdb.h"
#endif

#ifndef X_CUTIL_HXX_
#define X_CUTIL_HXX_
#include "cutil.hxx"
#endif

#ifndef X_TYPENAV_HXX_
#define X_TYPENAV_HXX_
#include "typenav.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_PERHIST_HXX_
#define X_PERHIST_HXX_
#include "perhist.hxx"
#endif

#ifndef X_EOBJECT_HXX_
#define X_EOBJECT_HXX_
#include "eobject.hxx"
#endif

#ifndef X_PLUGINST_HXX_
#define X_PLUGINST_HXX_
#include "pluginst.hxx"
#endif

#ifndef X_SHELL_H_
#define X_SHELL_H_
#include <shell.h>
#endif

#ifndef X_COMCAT_H_
#define X_COMCAT_H_
#include "comcat.h"
#endif

#ifndef X_OBJSAFE_H_
#define X_OBJSAFE_H_
#include "objsafe.h"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_IDISPIDS_H_
#define X_IDISPIDS_H_
#include "idispids.h"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"                     // for quickmimegetcsetinfo
#endif

#ifndef X_OLELYT_HXX_
#define X_OLELYT_HXX_
#include "olelyt.hxx"
#endif

#ifndef X_ACTIVSCP_H_
#define X_ACTIVSCP_H_
#include <activscp.h>
#endif

#ifndef X_PERHIST_H_
#define X_PERHIST_H_
#include <perhist.h>
#endif

#ifndef X_OBJEXT_H_
#define X_OBJEXT_H_
#include <objext.h>
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_SCRIPT_HXX_
#define X_SCRIPT_HXX_
#include "script.hxx"
#endif

#ifndef X_SAFETY_HXX_
#define X_SAFETY_HXX_
#include "safety.hxx"
#endif

#ifndef X_DBTASK_HXX_
#define X_DBTASK_HXX_
#include <dbtask.hxx>       // for CDatabindTask
#endif

#ifndef X_DMEMBMGR_HXX_
#define X_DMEMBMGR_HXX_
#include <dmembmgr.hxx>       // for CDataMemberMgr
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_DISPTREE_H_
#define X_DISPTREE_H_
#pragma INCMSG("--- Beg <disptree.h>")
#include <disptree.h>
#pragma INCMSG("--- End <disptree.h>")
#endif

#ifndef X_DISPDEFS_HXX_
#define X_DISPDEFS_HXX_
#include "dispdefs.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

// external reference.
HRESULT WrapSpecialUrl(TCHAR *pchURL, CStr *pcstr, CStr &cstrBaseURL, BOOL fNotPrivate, BOOL fIgnoreUrlScheme);

IMPLEMENT_LAYOUT_FNS(COleSite, COleLayout)

#define _cxx_
#include "olesite.hdl"

MtDefine(COleSiteCLock, Locals, "COleSite::CLock")

#define IDM_BORDERSTYLEFIXED 1000       // BUGBUG -- Remove when BillMc supports.
#define IDM_BORDERSTYLENO    1001



BEGIN_TEAROFF_TABLE(COleSite, IProvideMultipleClassInfo)
    TEAROFF_METHOD(COleSite, GetClassInfo, getclassinfo, (ITypeInfo ** ppTI))
    TEAROFF_METHOD(COleSite, GetGUID, getguid, (DWORD dwGuidKind, GUID * pGUID))
    TEAROFF_METHOD(COleSite, GetMultiTypeInfoCount, getmultitypeinfocount, (ULONG *pcti))
    TEAROFF_METHOD(COleSite, GetInfoOfIndex, getinfoofindex, (
            ULONG iti,
            DWORD dwFlags,
            ITypeInfo** pptiCoClass,
            DWORD* pdwTIFlags,
            ULONG* pcdispidReserved,
            IID* piidPrimary,
            IID* piidSource))
END_TEAROFF_TABLE()

DeclareTag(tagIgnoreAWV, "OleSite", "Ignore ActiveWhenVisible bit on controls");
DeclareTag(tagOleSiteRect, "OleSite", "Olesite position trace");

BEGIN_TEAROFF_TABLE_(COleSite, IServiceProvider)
        TEAROFF_METHOD(COleSite, QueryService, queryservice, (REFGUID guidService, REFIID riid, void **ppvObject))
END_TEAROFF_TABLE()

extern TCHAR g_szPropertiesVerb[];
extern HRESULT CreateStreamOnFile(
        LPCTSTR lpstrFile,
        DWORD dwSTGM,
        LPSTREAM * ppstrm);

extern BOOL OLECMDIDFromIDM(int idm, ULONG *pulCmdID);
extern HRESULT GetCategoryManager(ICatInformation **ppCat);


#ifndef NO_PROPERTY_PAGE
const CLSID * COleSite::s_apclsidPages[] =
{
    // Browse-time pages
    NULL,
    // Edit-time pages
#if DBG==1    
    &CLSID_CCDGenericPropertyPage,
#endif // DBG==1    
    NULL
};
#endif // NO_PROPERTY_PAGE

EXTERN_C CLSID CLSID_AppletOCX;
EXTERN_C const IID IID_IActiveDesigner;
EXTERN_C const CLSID CLSID_Scriptlet;

//
// misc helper functions
//

HRESULT
CLSIDFromHtmlString(TCHAR *pchClsid, CLSID *pclsid)
{
    const TCHAR achClsidPrefix [] = _T("clsid:");
    HRESULT     hr = S_OK;
    TCHAR       achClsid[CLSID_STRLEN + 1];

    // if begins with "clsid:" prefix
    if (pchClsid &&
        _tcsnipre(achClsidPrefix, ARRAY_SIZE(achClsidPrefix)-1, pchClsid, -1))
    {
        achClsid[0] = _T('{');
        _tcsncpy(
            achClsid + 1,
            pchClsid + ARRAY_SIZE(achClsidPrefix) - 1,
            CLSID_STRLEN - 2);
        achClsid[CLSID_STRLEN - 1] = _T('}');
        achClsid[CLSID_STRLEN] = 0;

        hr = THR(CLSIDFromString(achClsid, pclsid));
    }
    else
    {
        hr = E_FAIL;
    }

    RRETURN (hr);
}


//+---------------------------------------------------------------
//
//  Member:     OLECREATEINFO::~OLECREATEINFO
//
//  Synopsis:   dtor
//
//---------------------------------------------------------------

COleSite::OLECREATEINFO::~OLECREATEINFO()
{
    MemFreeString(pchSourceUrl);
    MemFreeString(pchDataUrl);
    MemFreeString(pchMimeType);
    MemFreeString(pchClassid);
    MemFreeString(pchFileName);
    ReleaseInterface(pStream);
    ReleaseInterface(pStorage);
    ReleaseInterface(pPropBag);
    ReleaseInterface(pDataObject);
    ReleaseInterface(pStreamHistory);
    ReleaseInterface(pBindCtxHistory);
    ReleaseInterface(pShortCutInfo);
}


//+---------------------------------------------------------------
//
//  Member:     COleSite::COleSite, protected
//
//  Synopsis:   used internally by the static Create methods to
//              initialize site object to a default state
//
//  Arguments:  [pUnkOuter] --  Controlling unknown for this site
//              [pParent]   --  Site that's our parent
//
//  Notes:      can not fail
//
//---------------------------------------------------------------

COleSite::COleSite(ELEMENT_TAG etag, CDoc *pDoc)
    : super(etag, pDoc)
{
    TraceTag((tagCDoc, "constructing COleSite"));
#ifdef WIN16
    m_baseOffset = ((BYTE *) (void *) (CBase *)this) - ((BYTE *) this);
    m_ElementOffset = ((BYTE *) (void *) (CElement *)this) - ((BYTE *) this);
#endif
    _pAtomTable = NULL;
    _wclsid = 0;
    _fDataSameDomain = TRUE;
}


//+------------------------------------------------------------------------
//
//  Member:     COleSite::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------

HRESULT
COleSite::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    HRESULT hr = S_OK;

    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_TEAROFF(this,  IProvideMultipleClassInfo, NULL)
        QI_TEAROFF2(this, IProvideClassInfo, IProvideMultipleClassInfo, NULL)
        QI_TEAROFF2(this, IProvideClassInfo2, IProvideMultipleClassInfo, NULL)
        QI_TEAROFF((CBase *)this, IOleCommandTarget, NULL)
        QI_TEAROFF_DISPEX(this,  NULL)
        QI_TEAROFF(this, IServiceProvider, NULL)
        QI_CASE(IConnectionPointContainer)
            if (_pUnkCtrl && iid == IID_IConnectionPointContainer)
            {
                COleSiteCPC *   pOleCPC;

                pOleCPC = new COleSiteCPC(this, _pUnkCtrl);
                if (!pOleCPC)
                    RRETURN(E_OUTOFMEMORY);

                (*(IConnectionPointContainer **)ppv) = pOleCPC;

                //
                // Since we just created an ole cpc, it's time to start
                // sinking events from the ocx.
                //

                EnsurePrivateSink();
            }
            break;

        default:
            hr = THR_NOTRACE(super::PrivateQueryInterface(iid, ppv));
            if (OK(hr))
                return S_OK;

            if (_pUnkCtrl)
            {
                void * pvObject;

                if (_fXAggregate)
                {
                    RRETURN(_pUnkCtrl->QueryInterface(iid, ppv));
                }

                hr = THR_NOTRACE(QueryControlInterface(iid, &pvObject));
                if (OK(hr))
                {
                    hr = THR(CreateTearOffThunk(
                            pvObject,
                            *(void **)pvObject,
                            NULL,
                            ppv,
                            (IUnknown *)this,
                            *(void **)(IUnknown *)this,
                            1,      // Call QI on object 2.
                            NULL));
                    ((IUnknown *)pvObject)->Release();
                }
            }
            break;
    }

    if (*ppv)
    {
        ((IUnknown *) *ppv)->AddRef();
        return S_OK;
    }

    RRETURN(E_NOINTERFACE);
}

//+----------------------------------------------------------------------------
//
//  Method : COleSite :: QueryService
//
//  Synopsis : IServiceProvider methoid Implementaion.
//          first QI for IServiceProvider on the control, and pass the QS to that.
//          if it fails, then pass the QS to our super's implementation
//
//-----------------------------------------------------------------------------

HRESULT
COleSite::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    HRESULT hr = E_POINTER;

    if (!ppvObject)
        goto Cleanup;

    if (_pUnkCtrl)
    {
        IServiceProvider * pISP = NULL;

        hr = THR_NOTRACE(_pUnkCtrl->QueryInterface(IID_IServiceProvider,
                                                   (void**)&pISP));
        if (hr)
            goto TryElement;

        hr = THR_NOTRACE(pISP->QueryService(guidService, riid, ppvObject));
        ReleaseInterface(pISP);
        if (hr)
            goto TryElement;

        goto Cleanup;
    }


TryElement:
    hr = THR_NOTRACE(super::QueryService(guidService,riid, ppvObject));

Cleanup:
    RRETURN1( hr, E_NOINTERFACE);
}


//+------------------------------------------------------------------------
//
//  Member:     COleSite::QueryControlInterface
//
//  Synopsis:   Query the control for an interface.
//              The purpose of this function is to reduce code size.
//
//  Arguments:  iid     Interface to query for
//              ppv     Returned interface
//
//-------------------------------------------------------------------------

HRESULT
COleSite::QueryControlInterface(REFIID iid, void **ppv)
{
    if (_pUnkCtrl)
        return _pUnkCtrl->QueryInterface(iid, ppv);
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

#ifdef WIN16
#pragma code_seg( "OLESITE_2_TEXT" )
#endif // WIN16

//+------------------------------------------------------------------------
//
//  Member:     COleSite::InvalidCall
//
//  Synopsis:   Return TRUE if call to client site is illegal.
//
//-------------------------------------------------------------------------

BOOL
COleSite::IllegalSiteCall(DWORD dwFlags)
{
    CDoc *  pDoc = Doc();

    //
    // Step 1: Check to see if object is alive and call is on correct thread.
    //

    if (GetRefs() == 0)
        return TRUE;

    Assert(pDoc);  // Should be set if _pUnkOuter != NULL.

    if (pDoc->_dwTID != GetCurrentThreadId())
    {
        Assert(0 && "ActiveX control called MSHTML across apartment thread boundary (not an MSHTML bug)");
        return TRUE;
    }

    //
    // Step 2:  Check state of COleSite.
    //

    switch (dwFlags & 0xFF)
    {
    case VALIDATE_WINDOWLESSINPLACE:
        if (!_fWindowlessInplace)
        {
            Assert(0 && "Illegal call to windowless interface by ActiveX control (not an MSHTML bug)");
            return TRUE;
        }
        break;

    case VALIDATE_INPLACE:
        if (State() < OS_INPLACE)
            return TRUE;
        break;

    case VALIDATE_LOADED:
        if (State() < OS_LOADED || !IsInMarkup())
            return TRUE;
        break;

    case VALIDATE_ATTACHED:
        if (!IsInMarkup())
            return TRUE;
        break;

    case VALIDATE_NOTRENDERING:
        {
            CLayout * pLayout = GetCurLayout();

            if (pLayout && ( pLayout->GetView()->IsInState(CView::VS_INRENDER)))
                return TRUE;
        }
    }

    //
    // Step 3: Check state of document.
    //

    switch (dwFlags & (0xFF << 8))
    {
    case VALIDATE_DOC_INPLACE:
        if (pDoc->State() < OS_INPLACE)
            return TRUE;
        break;

    case VALIDATE_DOC_SITE:
        if (!pDoc->_pUnkOuter || !pDoc->_pClientSite)
            return TRUE;
        break;

    case VALIDATE_DOC_ALIVE:
        if (!pDoc->_pUnkOuter)
            return TRUE;
        break;
    }

    return FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:     CObjectElement::Init2
//
//  Synopsis:   2nd phase of initialization
//
//-------------------------------------------------------------------------

HRESULT
COleSite::Init2(CInit2Context * pContext)
{
    CDoc *  pDoc = Doc();
    ELEMENT_TAG etag = Tag();

    pDoc->_fHasOleSite = TRUE;
    pDoc->_fBroadcastInteraction = TRUE;
    pDoc->_fBroadcastStop = TRUE;

    // should we post a createobject call?

    //
    //BUGBUG:(ferhane)  Frame and Iframes are going to be added here, when they can actually live 
    //                  outside of a tree. The if statement below should not need to checl for 
    //                  APPLET and OBJECT's explicitly when that happens. 
    //
    if ((etag == ETAG_EMBED) || 
        (((etag == ETAG_OBJECT) || (etag == ETAG_APPLET)) && (pContext->_dwFlags & INIT2FLAG_EXECUTE)))
    {
        IGNORE_HR(GWPostMethodCall(this, 
                                    ONCALL_METHOD(COleSite, DeferredCreateObject, deferredcreateobject), 0, FALSE, 
                                    "COleSite::DeferredCreateObject"));
    }

    // TODO: load any <OBJECT>-specific state (must match saved data in :Notify)

    RRETURN(super::Init2(pContext));
}


//+------------------------------------------------------------------------
//
//  Member:     Notify
//
//  Synopsis:   Handle notification
//
//-------------------------------------------------------------------------

void
COleSite::Notify(CNotification *pnf)
{
    HRESULT hr = S_OK;
    INSTANTCLASSINFO * pici;

    Assert(pnf);

    super::Notify(pnf);

    switch (pnf->Type())
    {
    case NTYPE_ELEMENT_QUERYFOCUSSABLE:
        pici = GetInstantClassInfo();

        ((CQueryFocus *)pnf->DataAsPtr())->_fRetVal =
                            _fObjAvailable
                        &&  !_fActsLikeLabel

                            // Hack for #68793
                        &&  pici && !(pici->dwCompatFlags & COMPAT_NEVERFOCUSSABLE);
        break;

    case NTYPE_ELEMENT_QUERYMNEMONICTARGET:
        if (_fActsLikeLabel)
        {
            FOCUS_ITEM fi;

            fi.pElement = NULL;
            fi.lSubDivision = 0;

            if (S_OK == Doc()->FindNextTabOrder(DIRECTION_FORWARD, this, 0, &fi.pElement, &fi.lSubDivision))
            {
                *(FOCUS_ITEM*)pnf->DataAsPtr() = fi;
            }
        }
        break;

    case NTYPE_ELEMENT_SETTINGFOCUS:
        {
            OLE_SERVER_STATE    baseState;
            CDoc *              pDoc = Doc();
            CSetFocus *         pSetFocus = (CSetFocus *)pnf->DataAsPtr();

            _fInBecomeCurrent = TRUE;

            if (pDoc->_fDontUIActivateOleSite && (pDoc->State() < OS_UIACTIVE))
            {
                // if the call is coming from DeferSetCurrency, and the document
                // is not UI activated, do not UI activate the olesite, since that
                // would cause the document to be UI activated prematurely.
                baseState = OS_INPLACE;
            }
            else
            {
                // If the call is not coming from the DeferSetCurrency, or the document
                // is already UI activated, then UI activate the olesite.
                baseState = OS_UIACTIVE;
            }

            pSetFocus->_hr = THR(TransitionTo(baseState, pSetFocus->_pMessage));

            if (S_OK == pSetFocus->_hr && _state != (unsigned)baseState)
            {
                pSetFocus->_hr = S_FALSE;                
            }
            else if (E_NOTIMPL == pSetFocus->_hr)
            {
                Assert(_state != (unsigned)baseState);
                pSetFocus->_hr = S_FALSE;                
            }
            _fInBecomeCurrent = FALSE;
        }
        break;

    case NTYPE_ELEMENT_SETFOCUSFAILED:
        IGNORE_HR(TransitionToBaselineState(Doc()->State()));
        break;

    case NTYPE_ELEMENT_SETFOCUS:
        {
            CDoc * pDoc = Doc();

            //
            // If this flag is set, then we did not UI activate the olesite
            // and left it inplace activated. So, we can not tell the document
            // that it has an active element.
            if (!pDoc->_fDontUIActivateOleSite)
            {
                Assert(OS_UIACTIVE == _state);

                // Update our state
                pDoc->_pInPlace->_fChildActive = TRUE;

                //  The user might open-edit something in the embedding,
                //  so pass along the host names.
                IGNORE_HR(SetHostNames());
            }
        }
        break;

    case NTYPE_UPDATE_DOC_DIRTY:
        if (S_FALSE == IsClean())
        {
            Doc()->_lDirtyVersion = MAXLONG;
            pnf->SetFlag(NFLAGS_SENDENDED);
        }
        break;

    case NTYPE_RELEASE_EXTERNAL_OBJECTS:
        ReleaseObject();
        break;

    case NTYPE_FREEZE_EVENTS:
        //
        // BUGBUG: (anandra)
        // Damn alpha version of the VM is still busted.  Stupid code
        // is baaaack.
        //

        if (OlesiteTag() != OSTAG_APPLET)
        {
            IOleControl * pControl = NULL;

            if (OK(THR_OLE(QueryControlInterface(
                    IID_IOleControl,
                    (void **) &pControl))))
            {
                BOOL    fFreeze;

                pnf->Data(&fFreeze);
                THR_OLE(pControl->FreezeEvents(fFreeze));
                pControl->Release();
            }
        }
        break;

    case NTYPE_CLEAR_FORMAT_CACHES:
    case NTYPE_AMBIENT_PROP_CHANGE:
        {
            DISPID          dispid;
            IOleControl *   pControl;

            pnf->Data(&dispid);
            if (OK(QueryControlInterface(IID_IOleControl, (void **)&pControl)))
            {
                IGNORE_HR(pControl->OnAmbientPropertyChange(dispid));
                pControl->Release();
            }
        }
        break;

    case NTYPE_UPDATE_VIEW_CACHE:
        {
            IOleObject *pObj;

            if (OK(QueryControlInterface(IID_IOleObject, (void **)&pObj)))
            {
                IGNORE_HR(pObj->Update());
                pObj->Release();
            }
        }
        break;

    case NTYPE_UPDATE_DOC_UPTODATE:
        {
            IOleObject *pObj;

            if (OK(QueryControlInterface(IID_IOleObject, (void **)&pObj)))
            {
                if (S_FALSE == THR_OLE(pObj->IsUpToDate()))
                {
                    pnf->SetFlag(NFLAGS_SENDENDED);
                    Doc()->_fIsUpToDate = FALSE;
                }
                pObj->Release();
            }
        }
        break;

    case NTYPE_DOC_STATE_CHANGE_1:
    case NTYPE_DOC_STATE_CHANGE_2:
        {
            DWORD   dw;
            CDoc *  pDoc = Doc();
            BOOL    fDown = FALSE;
            
            pnf->Data(&dw);

            // Our parent form is broadcasting a state change, transition
            // ourself correspondingly if the Doc's state is falling.

            if (pDoc->State() < (OLE_SERVER_STATE)dw)
            {
                fDown = TRUE;
            }

            pici = GetInstantClassInfo();
            
            if (pici && fDown ||  
                (pDoc->State() >= OS_INPLACE && 
                 (pici->dwCompatFlags & COMPAT_ALWAYS_INPLACEACTIVATE)))
            {
                if (pnf->IsSecondChance())
                {
                    //
                    // If it's a down transition and the state is already at the max,
                    // nothing more to do.
                    //
                    
                    if (fDown && State() <= pDoc->State())
                        break;
                        
                    IGNORE_HR(TransitionToBaselineState(pDoc->State()));
                }
                else
                    pnf->SetSecondChanceRequested();
            }
        }
        break;

    case NTYPE_STOP_1:
        pnf->SetSecondChanceRequested();
        break;

    case NTYPE_STOP_2:
        ClearRefresh();

        ReleaseCodeLoad();
        OnCommand(NULL, OLECMDID_STOP);

        break;

    case NTYPE_ENABLE_INTERACTION_1:
        pnf->SetSecondChanceRequested();
        break;

    case NTYPE_ENABLE_INTERACTION_2:
        {
            VARIANT varargIn;

            varargIn.vt = VT_I4;
            varargIn.lVal = Doc()->_fEnableInteraction;
            OnCommand(NULL, OLECMDID_ENABLE_INTERACTION, 0, &varargIn);
        }
        break;

    case NTYPE_BEFORE_UNLOAD:
        {
            BOOL *pfContinue;

            pnf->Data((void **)&pfContinue);
            if (*pfContinue)
            {
                VARIANT varOut = {0};
                OnCommand (NULL, OLECMDID_ONUNLOAD, 0,
                        NULL, &varOut);
                if (varOut.vt == VT_BOOL && varOut.boolVal == FALSE)
                    *pfContinue = FALSE;
            }
        }
        break;

    case NTYPE_BEFORE_REFRESH:
        {
            VARIANTARG varargIn;
            varargIn.vt = VT_I4;
            pnf->Data(&varargIn.lVal);
            OnCommand (NULL, OLECMDID_PREREFRESH, MSOCMDEXECOPT_DONTPROMPTUSER,
                    &varargIn, NULL);
        }
        break;

    case NTYPE_COMMAND:
        {
            COnCommandExecParams* pParm;

            pnf->Data((void **)&pParm);
            Assert(pParm);
            hr = THR(OnCommand(pParm->pguidCmdGroup, pParm->nCmdID,
                               pParm->nCmdexecopt, pParm->pvarargIn,
                               pParm->pvarargOut));
        }
        break;

    case NTYPE_SAVE_HISTORY_1:
        pnf->SetSecondChanceRequested();
        break;

    case NTYPE_SAVE_HISTORY_2:
        {
            IStream *           pStream = NULL;
            CHistorySaveCtx *   phsc = NULL;

            pnf->Data((void **)&phsc);
            hr = THR(phsc->BeginSaveStream(GetSourceIndex(), HistoryCode(), &pStream));
            if (hr)
                goto CleanupHistory;

            hr = THR(SaveHistoryStream(pStream));
            if (hr)
                goto CleanupHistory;

            hr = THR(phsc->EndSaveStream());
            if (hr)
                goto CleanupHistory;

        CleanupHistory:
            ReleaseInterface(pStream);
        }
        break;

    case NTYPE_END_PARSE:
#ifndef NO_DATABINDING
        if (OlesiteTag() == OSTAG_ACTIVEX || OlesiteTag() == OSTAG_APPLET)
        {
            DYNCAST(CObjectElement, this)->AttachDataBindings();
            Doc()->GetDataBindTask()->SetWaiting();
        }
#endif // ndef NO_DATABINDING
        break;

    case NTYPE_ELEMENT_EXITTREE_1:
        pnf->SetData( pnf->DataAsDWORD() | EXITTREE_DELAYRELEASENEEDED );
        if (State() > OS_RUNNING)
        {
            pnf->SetSecondChanceRequested();
        }
        break;

    case NTYPE_ELEMENT_EXITTREE_2:
        // If it is inplace-active or greater, transition to the running state.
        // The object shouldn't display after it leaves the tree.
        if (State() > OS_RUNNING)
           TransitionTo(OS_RUNNING);
        break;

    case NTYPE_ELEMENT_ENTERTREE:
        EnterTree();
        break;
    }
}


//+------------------------------------------------------------------------
//
//  Member:     COleSite::OnCommand
//
//  Note:       propagates a given notification down to doc living in the
//              WB control
//
//-------------------------------------------------------------------------

HRESULT
COleSite::OnCommand(const GUID * pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt,
                    VARIANTARG * pvarargIn, VARIANTARG * pvarargOut)
{
    HRESULT                 hr;
    IOleCommandTarget *     pCmdTarget = NULL;

    hr = THR_NOTRACE(QueryControlInterface(
            IID_IOleCommandTarget,
            (void**)&pCmdTarget));
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(pCmdTarget->Exec(
            pguidCmdGroup,
            nCmdID,
            nCmdexecopt,
            pvarargIn,
            pvarargOut));

Cleanup:
    ReleaseInterface(pCmdTarget);

    RRETURN (hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::SetMiscStatusFlags
//
//  Synopsis:   Set members from ole misc status flags
//
//  Arguments:  dwMiscStats     the flags
//
//----------------------------------------------------------------------------

void
COleSite::SetMiscStatusFlags(DWORD dwMiscStatus)
{
    INSTANTCLASSINFO * pici = GetInstantClassInfo();
    
    CLayout * pLayout = GetCurLayout();
    Assert(pLayout);

    //  We may have to add in some flags from our compatability class table
    // since some controls' record their flags wrong and IE3 did not fully
    // enforce the meaning of missing flags such as INSIDEOUT.
    if (pici)
        dwMiscStatus |= pici->dwMiscStatusFlags;

    //  If a control acts like a label, we default it's TabStop
    //    property to FALSE, rather than the normal TRUE default

    if (dwMiscStatus & OLEMISC_ACTSLIKELABEL)
    {
        _fActsLikeLabel = TRUE;
    }

    if (dwMiscStatus & OLEMISC_ACTSLIKEBUTTON)
        _fActsLikeButton = TRUE;

    if (dwMiscStatus & OLEMISC_NOUIACTIVATE)
        pLayout->_fNoUIActivate = TRUE;

    if (dwMiscStatus & OLEMISC_INSIDEOUT)
        _fInsideOut = TRUE;

    if (dwMiscStatus & OLEMISC_INVISIBLEATRUNTIME)
        pLayout->_fInvisibleAtRuntime = TRUE;

    if (dwMiscStatus & OLEMISC_ALWAYSRUN)
        _fAlwaysRun = TRUE;

    // A control is activate when visible if
    //      the flag is set AND
    //      the ignore flag is not set AND
    //      the control is not iconic and invisible at runtime.
    //
    // The last part of this check is required because some controls
    // marked as activiate when visible and invisible at runtime do
    // not correctly handle being active. In the case of the VB4 common
    // dialog control, it will not paint its window correctly.

    if ((dwMiscStatus &
            (OLEMISC_ACTIVATEWHENVISIBLE|OLEMISC_IGNOREACTIVATEWHENVISIBLE)) ==
                OLEMISC_ACTIVATEWHENVISIBLE &&
        (dwMiscStatus &
            (OLEMISC_ONLYICONIC|OLEMISC_INVISIBLEATRUNTIME)) !=
                (OLEMISC_ONLYICONIC|OLEMISC_INVISIBLEATRUNTIME))
    {
#if DBG == 1
        if (!IsTagEnabled(tagIgnoreAWV))
#endif
        _fActivateWhenVisible = TRUE;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::SetViewStatusFlags
//
//  Synopsis:   Set members from view object status flags
//
//  Arguments:  dwViewStatus     the flags
//
//----------------------------------------------------------------------------

void
COleSite::SetViewStatusFlags(DWORD dwViewStatus)
{
    CLayout * pLayout = GetCurLayout();
    Assert(pLayout);

    pLayout->SetSurfaceFlags(dwViewStatus & VIEWSTATUS_SURFACE, dwViewStatus & VIEWSTATUS_3DSURFACE);

    _fDVAspectOpaque = !!(dwViewStatus & VIEWSTATUS_DVASPECTOPAQUE);
    _fDVAspectTransparent = !!(dwViewStatus & VIEWSTATUS_DVASPECTTRANSPARENT);

    // send a REmeaseure notification, which on the layout will PostLayoutRequest
    // this will cause a ensureDispNode to be called, and that in turn will
    // set the opacity flag
    RemeasureElement();
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::ConnectControl1
//
//  Synopsis:   Establish connection between control and client site
//              and set status bits.  Call this function before calling
//              IPersist*::InitNew or IPersist*::Load.
//
//  Arguments:  pdwInitFlags  In/out argument. Taken from the
//                            FSI_ enumeration.  Pass result to ConnectControl.
//
//  Returns:    HRESULT
//----------------------------------------------------------------------------

HRESULT
COleSite::ConnectControl1(DWORD *pdwInitFlags)
{
    HRESULT         hr = S_OK;
    DWORD           dwMiscStatus;
    QACONTROL       qacontrol;
    QACONTAINER     qacontainer;
    IQuickActivate *pQA = NULL;
    IOleObject *    pOleObject = NULL;
    CDoc *          pDoc = Doc();
    INSTANTCLASSINFO * pici;

    // Make sure we have an undo manager to hand to our embeddings.  If we
    // haven't gotten our undo manager by now, then our container better not
    // be providing one.  If they are providing one they have to support
    // SETCLIENTSITEFIRST, which means we would have gotten it before loading.

#ifndef NO_EDIT
    if (pDoc->UndoManager() == &g_DummyUndoMgr)
    {
        hr = THR(pDoc->CreateUndoManager());
        if (hr)
            goto Cleanup;
    }
#endif // NO_EDIT

    //
    // Only use QuickActivate if the following conditions are met:
    //
    //  1)  The COMPAT_NO_QUICKACTIVATE flag is not set.
    //  2)  _pUnkCtrl is not an ole proxy (i.e. it is apartment modelled)
    //      There's an OLE bug in the marshalling for IQuickActivate which
    //      causes E_UNEXPECTED to be returned on win95 and nt versions
    //      < SP3.  (anandra)
    //  3)  _pUnkCtrl supports the QI for IQuickActivate.
    //
    pici = GetInstantClassInfo();

    // If pici is NULL, don't try to quick activate the control.
    // We don't try to quick activate it because if pici is NULL,
    // SetOC96 won't be called and the control will never be drawn.
    //
    if (pici && !(pici->dwCompatFlags & COMPAT_NO_QUICKACTIVATE) &&
        !IsOleProxy() &&
        OK(THR_NOTRACE(QueryControlInterface(
            IID_IQuickActivate, (void **)&pQA))))
    {
        *pdwInitFlags |= FSI_QUICKACTIVATE|FSI_CLIENTSITESET;

        memset(&qacontrol, 0, sizeof(qacontrol));
        qacontrol.cbSize = sizeof(qacontrol);

        memset(&qacontainer, 0, sizeof(qacontainer));
        qacontainer.cbSize = sizeof(qacontainer);
        qacontainer.pAdviseSink = &_Client;
        qacontainer.pClientSite = &_Client;
        qacontainer.pPropertyNotifySink = &_Client;

#if DBG==1
        qacontainer.pAdviseSink->AddRef();
        DbgTrackItf(IID_IAdviseSink, "CClient", FALSE, (void **)&qacontainer.pAdviseSink);

        qacontainer.pClientSite->AddRef();
        DbgTrackItf(IID_IOleClientSite, "CClient", FALSE, (void **)&qacontainer.pClientSite);

        qacontainer.pPropertyNotifySink->AddRef();
        DbgTrackItf(IID_IPropertyNotifySink, "PNS", FALSE, (void **)&qacontainer.pPropertyNotifySink);
#endif

        VARIANT v;
        VariantInit(&v);

        // Ambient foregound color
        IGNORE_HR(GetAmbientProp(DISPID_AMBIENT_FORECOLOR, &v));
        Assert(VT_I4 == V_VT(&v));
        qacontainer.colorFore = V_I4(&v);

        // Ambient backgound color
        IGNORE_HR(GetAmbientProp(DISPID_AMBIENT_BACKCOLOR, &v));
        Assert(VT_I4 == V_VT(&v));
        qacontainer.colorBack = V_I4(&v);

        // Ambient locale
        IGNORE_HR(GetAmbientProp(DISPID_AMBIENT_LOCALEID, &v));
        Assert(VT_I4 == V_VT(&v));
        qacontainer.lcid = V_I4(&v);

        // Ambient Font...
        hr = THR(GetAmbientProp(DISPID_AMBIENT_FONT, &v));
        if (OK(hr) && V_DISPATCH(&v))
        {
            // Since the ambient font property is an LPFONTDISP, we need
            // to ask for IFont.
            LPDISPATCH pFontDisp = V_DISPATCH(&v);
            pFontDisp->QueryInterface(IID_IFont, (void **) &qacontainer.pFont);
            VariantClear(&v);   // will release the LPFONTDISP interface
        }

#ifndef NO_EDIT
        qacontainer.pUndoMgr = (pDoc->_pUndoMgr != &g_DummyUndoMgr) ?
                                       pDoc->_pUndoMgr : NULL;
#endif // NO_EDIT

        //
        // BUGBUG: Disabling quick-activate of event sink, since
        // the olesite event sink does not know how to correctly
        // return from events when there is not an event sink
        // hooked up to the control.
        //
        // qacontainer.pUnkEventSink     = pUnkEventSink;

        //
        // Initialize all current mode behaviors.
        //
        qacontainer.dwAmbientFlags =
                QACONTAINER_MESSAGEREFLECT |
                QACONTAINER_SUPPORTSMNEMONICS;

        if (pDoc->_fDesignMode)
        {
            qacontainer.dwAmbientFlags |=
                    QACONTAINER_SHOWHATCHING |
                    QACONTAINER_SHOWGRABHANDLES;
        }
        else
        {
            qacontainer.dwAmbientFlags |= QACONTAINER_USERMODE;
        }

        if (_fDefault)
        {
            //  Note that the _fActsLikeButton flag is not set until
            //    we call SetMiscStatusFlags below

            if (!CheckDisplayAsDefault(pDoc->_pElemCurrent))
            {
                qacontainer.dwAmbientFlags |= QACONTAINER_DISPLAYASDEFAULT;
            }
        }

        hr = THR_OLE(pQA->QuickActivate(&qacontainer, &qacontrol));
#if DBG==1
        qacontainer.pAdviseSink->Release();
        qacontainer.pClientSite->Release();
        qacontainer.pPropertyNotifySink->Release();
#endif
        ReleaseInterface(qacontainer.pFont);
        if (hr)
            goto Cleanup;

        SetMiscStatusFlags(qacontrol.dwMiscStatus);
        SetViewStatusFlags(qacontrol.dwViewStatus);
        *pdwInitFlags |= FSI_PNSCONNECTED;

        pici->SetOC96(TRUE);
    }
    else if (OK(THR_OLE(QueryControlInterface(
                IID_IOleObject, (void **)&pOleObject))))
    {
        hr = THR_OLE(pOleObject->GetMiscStatus(
                DVASPECT_CONTENT,
                &dwMiscStatus));
        if (hr)
        {
            dwMiscStatus = 0;  // this is how IE3 behaves.
            hr = S_OK;         // this happens with the Interactive Music Control, 11/18/96
        }

        SetMiscStatusFlags(dwMiscStatus);
        if (dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST)
        {
            *pdwInitFlags |= FSI_CLIENTSITESET;
            hr = THR_OLE(SetClientSite(&_Client));
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    ReleaseInterface(pOleObject);
    ReleaseInterface(pQA);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::ConnectControl2
//
//  Synopsis:   Establish connection between control and client site
//              and get extent.  Call this function after calling
//              IPersist*::InitNew or IPersist*::Load.
//
//  Arguments:  dwInitFlags     Some combination of:
//
//                  FSI_NONE            No special behavior.
//                  FSI_QUICKACTIVATE   ConnectControl1 used IQuickActivate.
//                  FSI_CLIENTSITESET   ConnectControl1 set the client site.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
COleSite::ConnectControl2(DWORD *pdwInitFlags)
{
    HRESULT         hr = S_OK;
    DWORD           dwViewStatus;
    IActiveScript * pScript = NULL;
    CNotification   nf;
    CDoc *          pDoc = Doc();
    ICatInformation *pCatInfo = NULL;

    CLayout * pLayout = GetCurLayout();
    Assert(pLayout);

    _state = OS_LOADED;

    // Cache a pointer to the object's IViewObject implementation

    if (OK(THR_NOTRACE(QueryControlInterface(IID_IViewObjectEx,
            (void **)&_pVO))))
    {
        _fUseViewObjectEx = TRUE;
    }
    else
    {
        THR_OLE(QueryControlInterface(IID_IViewObject, (void **) &_pVO));
    }

    // Set the client site if not already set.

    if (!(*pdwInitFlags & FSI_CLIENTSITESET))
    {
        *pdwInitFlags |= FSI_CLIENTSITESET;
        hr = THR_OLE(SetClientSite(&_Client));
        if (hr)
            goto Cleanup;
    }

    if (!(*pdwInitFlags & FSI_QUICKACTIVATE))
    {
        // Setup stuff handled by quick activate.

        if (_pVO)
        {
#if DBG==1
            IAdviseSink *pAdviseSink = &_Client;
            pAdviseSink->AddRef();
            DbgTrackItf(IID_IAdviseSink, "CClient", FALSE, (void **)&pAdviseSink);
#endif
            IGNORE_HR(_pVO->SetAdvise(DVASPECT_CONTENT, 0, &_Client));
#if DBG==1
            pAdviseSink->Release();
#endif
        }

        // Parse view status flags.

        if (_fUseViewObjectEx)
        {
            hr = THR_OLE(((IViewObjectEx*)_pVO)->GetViewStatus(&dwViewStatus));
            if (hr)
                goto Cleanup;

            SetViewStatusFlags(dwViewStatus);
        }

        //
        // Some down-level controls don't like to be UI-activated
        // in design mode. Therefore, if the control does not support
        // IQuickActivate we assume it's down-level and prevent transitioning
        // it to UI-active state except through its verb menu (if it provides it).
        // If it does not support IOleControl then it's not a control and
        // we don't set the _fNoUIActivateInDesign flag.
        // (LyleC)
        //
        // Note that it _is_ possible to get into this codepath (with
        // FSI_QUICKACTIVATE not set) but still have the control support
        // IQuickActivate.
        //

        // BUGBUG: Remove the OSTAG_APPLET check below when the Java VM supports IQuickActivate.
        if (pDoc->_fDesignMode && OlesiteTag() != OSTAG_APPLET)
        {
            HRESULT             hr2;
            IQuickActivate *    pQA    = NULL;
            IOleControl *       pCtrl  = NULL;

            hr2 = THR_NOTRACE(QueryControlInterface(IID_IQuickActivate,
                                                  (LPVOID*)&pQA));
            if (hr2)
                pQA = NULL;

            hr2 = THR_NOTRACE(QueryControlInterface(IID_IOleControl,
                                                  (LPVOID*)&pCtrl));
            if (hr2)
                pCtrl = NULL;

            if (!pQA && pCtrl && pLayout)
            {
                pLayout->_fNoUIActivateInDesign  = TRUE;
            }

            ReleaseInterface (pQA);
            ReleaseInterface (pCtrl);

        }

        // Don't require the component to support IConnectionPointContainer or
        // a connection point for IPropertyNotifySink.  Since all errors are clamped
        // to E_FAIL anyway, ignore the return value  (philco).

        IPropertyNotifySink *pPNS = &_Client;

#if DBG==1
        pPNS->AddRef();
        DbgTrackItf(IID_IPropertyNotifySink, "cclient", FALSE, (void **)&pPNS);
#endif

        if (OK(THR(ConnectSink(
                _pUnkCtrl,
                IID_IPropertyNotifySink,
                pPNS,
                &_dwPropertyNotifyCookie))))
        {
            *pdwInitFlags |= FSI_PNSCONNECTED;
        }

#if DBG==1
        pPNS->Release();
#endif
    }

    //
    // Set _fNoUIActivateInDesign flag for Design Time Controls,
    // Java Applets or other OLE Controls based on host preferences
    //
    if ( pDoc->_fDesignMode &&
        (pDoc->_fNoActivateNormalOleControls ||
         pDoc->_fNoActivateDesignTimeControls ||
         pDoc->_fNoActivateJavaApplets))
    {
        HRESULT           hr2;
        IUnknown *        pAD = NULL;
        BOOL              fDesignTimeControl = FALSE;
        BOOL              fJavaApplet = (OlesiteTag() == OSTAG_APPLET);

        hr2 = THR_NOTRACE(QueryControlInterface(IID_IActiveDesigner,
                                          (LPVOID*)&pAD));
        if (S_OK == hr2)
            fDesignTimeControl = TRUE;

        ReleaseInterface (pAD);

        if (pLayout && pDoc->_fNoActivateDesignTimeControls && fDesignTimeControl)
            pLayout->_fNoUIActivateInDesign = TRUE;

        if (pLayout && pDoc->_fNoActivateJavaApplets && fJavaApplet)
            pLayout->_fNoUIActivateInDesign = TRUE;

        if (pLayout && pDoc->_fNoActivateNormalOleControls && !fDesignTimeControl && !fJavaApplet)
            pLayout->_fNoUIActivateInDesign = TRUE;
    }

    //
    // If pLayout->_fNoUIActivateInDesign is set, then override this if they support 
    // CATID_DesignTimeUIActivatableControl. Note that anything with an appropriate
    // CATID will be UI active even if the HOST tells us not to do so.
    //

    if (pLayout && pLayout->_fNoUIActivateInDesign)
    {
        //
        // Get the category manager
        //
        
        hr = THR(GetCategoryManager(&pCatInfo));
        if (hr)
            goto Cleanup;

        //
        // Check if control supports CATID_DesignTimeUIActivatableControl
        //
        CATID rgcatid[1];
        rgcatid[0] = CATID_DesignTimeUIActivatableControl;

        CLSID * pCLSID = GetpCLSID();
        
        if (pCLSID)
        {
            pLayout->_fNoUIActivateInDesign = 
                (pCatInfo->IsClassOfCategories(*pCLSID, 1, rgcatid, 0, NULL) != S_OK);        
        }
        else
        {
            pLayout->_fNoUIActivateInDesign = TRUE;
        }
    }    

    //
    // Query for the scripting interfaces.  If available then add
    // a holder for them.
    //

    QueryControlInterface(IID_IActiveScript, (void **)&pScript);
    if (pScript && pDoc->_pScriptCollection)
    {
        hr = THR(pDoc->_pScriptCollection->AddHolderForObject(
                this,
                pScript,
                GetpCLSID()));
        if (hr)
            goto Cleanup;
    }

    //
    // If we just created the license manager and no license manager
    // currently exists, cache pointer to license manager.
    //

    if (!pDoc->_pLicenseMgr)
    {
        QueryControlInterface(IID_ILocalRegistry,(void**)&pDoc->_pLicenseMgr);
    }

    //
    // Check for support for the new palette notification
    //

    if ((Tag() == ETAG_FRAME || Tag() == ETAG_IFRAME) &&
        SUCCEEDED(OnCommand(&CGID_ShellDocView, SHDVID_CANDOCOLORSCHANGE)))
    {
        _fCanDoShColorsChange = TRUE;
    }

    //
    // Freeze events if the doc's freeze count is > 0
    //

    if (pDoc->_cFreeze)
    {
        //
        // Will be unfrozen when doc's freezecount drops to zero.
        //

        nf.FreezeEvents(this, (void *)TRUE);
        Notify(&nf);
    }

    //
    // Freeze events again if the doc is not fully loaded
    //
    if (pDoc->LoadStatus() < LOADSTATUS_DONE)
    {
        //
        // Will be unfrozen when the doc is fully loaded.
        //

        nf.FreezeEvents(this, (void *)TRUE);
        Notify(&nf);
    }

    nf.EnableInteraction1(this);
    Notify(&nf);

    if (nf.IsSecondChanceRequested())
    {
        nf.EnableInteraction2(this);
        Notify(&nf);
    }

Cleanup:
    ReleaseInterface(pScript);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::AssignWclsidFromControl
//
//  Synopsis:   Assign _wclsid using IPersist* interface on control.
//
//  Arguments:  None
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
COleSite::AssignWclsidFromControl()
{
    HRESULT     hr;
    IPersist *  pPers = NULL;
    CLSID       clsid;
    CDoc *      pDoc = Doc();

    hr = THR_OLE(QueryControlInterface(IID_IPersist, (void **) &pPers));
    if (hr)
        goto Cleanup;

    hr = THR_OLE(pPers->GetClassID(&clsid));
    if (hr)
        goto Cleanup;

    hr = THR(pDoc->_clsTab.AssignWclsid(pDoc, clsid, &_wclsid));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pPers);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::InitNewObject
//
//  Synopsis:   Initializes embedded object from scratch.
//              Assumes _wclsid is initialized.
//
//  Arguments:  None
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
COleSite::InitNewObject()
{
    HRESULT                 hr;
    IPersistStreamInit *    pPStm = NULL;
    IPersistPropertyBag *   pPPBag = NULL;
    IPersistPropertyBag2 *  pPPBag2 = NULL;
    IPersistStorage         *pStorage = NULL;

    if (!QueryControlInterface (IID_IPersistPropertyBag2, (void**)&pPPBag2))
    {
        hr = THR_OLE(pPPBag2->InitNew());
        if (hr)
            goto Cleanup;
    }
    else if (!QueryControlInterface(IID_IPersistPropertyBag,(void**)&pPPBag))
    {
        hr = THR_OLE(pPPBag->InitNew());
        if (hr)
            goto Cleanup;
    }
    else
    {   // control does not support IPersistPropertyBag

        hr = THR_OLE(QueryControlInterface(
                IID_IPersistStreamInit,
                (void **) &pPStm));

        if (!hr)
        {
            hr = THR_OLE(pPStm->InitNew());
            if (hr)
                goto Cleanup;
        }
        else
        {

            hr = THR_OLE(QueryControlInterface(
                IID_IPersistStorage,
                (void **) &pStorage ));

            if (!hr)
            {
                // we do not support controls who require storage....
                hr = E_FAIL;
                goto Cleanup;
            }
            // The control does not support IPersist
            hr = S_OK; // we allow such controls
        }
    }

Cleanup:
    ReleaseInterface(pPPBag);
    ReleaseInterface(pPPBag2);
    ReleaseInterface(pPStm);
    ReleaseInterface(pStorage);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::LoadObject, public
//
//  Synopsis:   Loads our embedded object from a stream.
//
//  Arguments:  [pStm] -- Stream to load from
//
//  Returns:    HRESULT
//
//  Notes:      S_FALSE is returned to indicate the object cannot be
//              safely initialized on this interface.
//
//----------------------------------------------------------------------------

HRESULT
COleSite::LoadObject(IStream * pStm)
{
    HRESULT                 hr      = S_FALSE;
    IPersistStreamInit *    pPStm   = NULL;

    hr = THR_OLE(QuerySafeLoadInterface(IID_IPersistStreamInit, (void **) &pPStm));
    if (OK(hr) && pPStm)
    {
        hr = THR_OLE(pPStm->Load(pStm));
        goto Cleanup;
    }
    else
    {
        hr = THR_OLE(QuerySafeLoadInterface(IID_IPersistStream, (void **) &pPStm));
        if (OK(hr) && pPStm)
        {
            hr = THR_OLE(pPStm->Load(pStm));
            goto Cleanup;
        }
    }

Cleanup:
    ReleaseInterface(pPStm);
    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::LoadObject, public
//
//  Synopsis:   Loads our embedded object from a storage
//
//  Arguments:  [pStg] -- Storage to load from.
//
//  Returns:    HRESULT
//
//  Notes:      S_FALSE is returned to indicate the object cannot be
//              safely initialized on this interface.
//----------------------------------------------------------------------------

HRESULT
COleSite::LoadObject(IStorage * pStg)
{
    HRESULT             hr          = S_FALSE;
    IPersistStorage *   pPStg       = NULL;

    hr = THR_OLE(QuerySafeLoadInterface(IID_IPersistStorage, (void **) &pPStg));
    if (OK(hr) && pPStg)
    {
        hr = THR_OLE(pPStg->Load(pStg));
        goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pPStg);
    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::LoadObject, public
//
//  Synopsis:   Initializes embedded object from a PropertyBag
//
//  Notes:      S_FALSE is returned to indicate the object cannot be
//              safely initialized on this interface.
//----------------------------------------------------------------------------

HRESULT
COleSite::LoadObject(IPropertyBag * pPropBag)
{
    HRESULT                 hr = S_FALSE;
    IPersistPropertyBag *   pPPBag = NULL;
    IPersistPropertyBag2 *  pPPBag2 = NULL;
    IPropertyBag2 *         pPBag2 = NULL;

    //
    // Ensure the VB standard _ExtentX and _ExtentY properties are
    // available.  Some lame controls like SurroundVideoCtrl Object
    // choke if those properties are not available.
    //
    if (IsInMarkup())
    {
        {   const static TCHAR tchExtentX[] = _T("_ExtentX");
            CVariant Var( VT_EMPTY );
            if( pPropBag->Read( tchExtentX, &Var, NULL ) == E_INVALIDARG)
            {
                // Property does not exist:
                CUnitValue  uvWidth      = GetFirstBranch()->GetCascadedwidth();
                if( !uvWidth.IsNull() )
                {
                    Var.vt = VT_I4;
                    Var.lVal = HimetricFromHPix(uvWidth.GetPixelValue());
                    // We do not concern ourselves with failures here
                    // since this whole operation is just a convenience
                    // for some controls and not strictly neccessary.
                    pPropBag->Write( tchExtentX, &Var );
                }
            }
        }
        {   const static TCHAR tchExtentY[] = _T("_ExtentY");
            CVariant Var( VT_EMPTY );
            if( pPropBag->Read( tchExtentY, &Var, NULL ) == E_INVALIDARG)
            {
                // Property does not exist:
                CUnitValue  uvHeight     = GetFirstBranch()->GetCascadedheight();
                if( !uvHeight.IsNull() )
                {
                    Var.vt = VT_I4;
                    Var.lVal = HimetricFromVPix(uvHeight.GetPixelValue());
                    // We do not concern ourselves with failures here
                    // since this whole operation is just a convenience
                    // for some controls and not strictly neccessary.
                    pPropBag->Write( tchExtentY, &Var );
                }
            }
        }
    }


    //
    // First try IPersistPropertyBag2.
    //

    hr = THR_NOTRACE(QuerySafeLoadInterface(IID_IPersistPropertyBag2, (void **) &pPPBag2));
    if (OK(hr) && pPPBag2)
    {
        Verify(OK(pPropBag->QueryInterface(IID_IPropertyBag2, (void **)&pPBag2)));
        hr = THR_OLE(pPPBag2->Load(pPBag2, NULL));
        if (hr)
            goto Cleanup;
    }
    else
    {
        //
        // Then IPersistPropertyBag
        //

        hr = THR_OLE(QuerySafeLoadInterface(IID_IPersistPropertyBag, (void **) &pPPBag));
        if (OK(hr) && pPPBag)
        {
            hr = THR_OLE(pPPBag->Load(pPropBag, NULL));
            goto Cleanup;
        }
    }

Cleanup:
    ReleaseInterface(pPPBag);
    ReleaseInterface(pPBag2);
    ReleaseInterface(pPPBag2);
    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::LoadObject, public
//
//  Synopsis:   Loads our embedded object from a file name
//
//  Arguments:  [pchFileName] -- Filename to load from.
//
//  Returns:    HRESULT
//
//  Notes:      S_FALSE is returned to indicate the object cannot be
//              safely initialized on this interface.
//
//----------------------------------------------------------------------------

HRESULT
COleSite::LoadObject(TCHAR *pchDataUrl, TCHAR *pchFileName)
{
    HRESULT             hr = S_FALSE;
    IPersistFile *      pPF = NULL;
    IPersistMoniker *   pPMk = NULL;
    IStream *           pStm = NULL;
    IStorage *          pStg = NULL;
    IBindCtx *          pBCtx = NULL;
    IMoniker *          pMk = NULL;

    //
    // First try IPersistMoniker if we have a data url
    //

    if (pchDataUrl)
    {
        hr = THR_OLE(QuerySafeLoadInterface(IID_IPersistMoniker, (void **)&pPMk));
        if (OK(hr) && pPMk)
        {
            hr = THR(CreateAsyncBindCtxEx(NULL, 0, NULL, NULL, &pBCtx, 0));
            if (hr)
                goto Cleanup;

            hr = THR(CreateURLMoniker(NULL, pchDataUrl, &pMk));
            if (hr)
                goto Cleanup;

            hr = THR(pPMk->Load(FALSE, pMk, pBCtx, 0));
            goto Cleanup;
        }
    }

    if (!pchFileName)
    {
        hr = E_NOINTERFACE;
        goto Cleanup;
    }

    hr = THR_OLE(QuerySafeLoadInterface(IID_IPersistFile, (void **) &pPF));
    if (OK(hr) && pPF)
    {
        hr = THR(pPF->Load(pchFileName, 0));
        if (hr)
            goto Cleanup;
    }
#ifndef WIN16
    else
    {
        //
        // Try the stream approach on this file
        //

        hr = THR(CreateStreamOnFile(
                pchFileName,
                STGM_READ | STGM_SHARE_DENY_WRITE,
                &pStm));
        if (hr)
            goto Cleanup;

        hr = THR_OLE(LoadObject(pStm));
        if (!hr)
            goto Cleanup;

        //
        // Finally try to create a storage on the file
        //

        ClearInterface(&pStm);
        hr = THR(StgOpenStorage(
          pchFileName,
          NULL,
          STGM_READ | STGM_DIRECT | STGM_SHARE_DENY_WRITE,
          NULL,
          0,
          &pStg));
        if (hr)
            goto Cleanup;

        hr = THR_OLE(LoadObject(pStg));
        if (hr)
            goto Cleanup;
    }
#endif //ndef WIN16

Cleanup:
    ReleaseInterface(pPMk);
    ReleaseInterface(pBCtx);
    ReleaseInterface(pPF);
    ReleaseInterface(pMk);
    ReleaseInterface(pStm);
    ReleaseInterface(pStg);
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::LoadHistoryStream, public
//
//  Synopsis:   Loads our embedded object from a history stream
//
//  Arguments:  [pStream] - the history stream to load from
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
COleSite::LoadHistoryStream(IStream *pStream, IBindCtx *pbc)
{
    CDataStream ds(pStream);

    HRESULT             hr = S_FALSE;
    HRESULT             hr2;
    IPersistHistory *   pPH = NULL;
    IStream *           pStm = NULL;

    hr = THR_NOTRACE(QuerySafeLoadInterface(IID_IPersistHistory, (void **) &pPH));
    if (OK(hr) && pPH)
    {
        hr = THR(ds.LoadDword((DWORD*)&hr2));
        if (hr)
            goto Cleanup;

        hr = THR(ds.LoadSubstream(&pStm));
        if (hr)
            goto Cleanup;

        if (!hr2)
            hr2 = THR_OLE(pPH->LoadHistory(pStm, pbc));

        if (hr2)
            hr = hr2;
    }

Cleanup:
    ReleaseInterface(pPH);
    ReleaseInterface(pStm);
    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::SaveHistoryStream, public
//
//  Synopsis:   Saves our embedded object to a history stream
//
//  Arguments:  [pStream] - the history stream to save to
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
COleSite::SaveHistoryStream(IStream *pStream)
{
    CDataStream ds;
    IStream *pSubstream = NULL;
    IPersistHistory *pPH = NULL;
    DWORD dwPosition;
    HRESULT hr = S_OK;
    HRESULT hr2 = S_OK;

    if (OK(QueryControlInterface(IID_IPersistHistory, (void **)&pPH)))
    {
        ds.Init(pStream);

        hr = THR(ds.SaveDataLater(&dwPosition, sizeof(hr2)));
        if (hr)
            goto Cleanup;

        hr = THR(ds.BeginSaveSubstream(&pSubstream));
        if (hr)
            goto Cleanup;

        hr2 = THR_OLE(pPH->SaveHistory(pSubstream));

        hr = THR(ds.EndSaveSubstream());
        if (hr)
            goto Cleanup;

        hr = THR(ds.SaveDataNow(dwPosition, &hr2, sizeof(hr2)));
        if (hr)
            goto Cleanup;
    }

Cleanup:

    ReleaseInterface(pSubstream);
    ReleaseInterface(pPH);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CreateObject
//
//  Synopsis:   selects a way to instantiate object or launch download;
//              actual instantiation always happens in CreateObjectNow
//              called from this function or in the end of download
//
//  Arguments:  pinfo       Creation info
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
COleSite::CreateObject(OLECREATEINFO *pinfo)
{
    HRESULT         hr = S_OK;
    CDoc *          pDoc = Doc();
    INSTANTCLASSINFO * pici;

    Assert(!_pUnkCtrl);

    // 
    //  During the creation of a control, we may make sycnhronous calls
    //  to that component. There are some cases, where the component pushes 
    //  a message loop, and the browser window is closed in that time frame.
    //  If that ever happens, we risk not having a document at the end of this
    //  function.
    //  We addref here and release at the end of this function to guarantee.
    //  Bug#IE573270 (ferhane)
    //
    Assert(pDoc);
    pDoc->AddRef();

    // init2 is called at the begin tag parse time, since this element is not
    // in the tree yet, we can't call GetLoadHistoryStream().  This is safe to
    // do here since _pStreamHistory isn't used until CreateObjectNow() (below) and 
    // in CFrameSite::CreatObject (which doesn't call super); plugins can now leverage
    // this (bug in previous logic)
    if (!_pStreamHistory)
    {
        THR(pDoc->GetLoadHistoryStream(GetSourceIndex(), HistoryCode(), &_pStreamHistory));
    }

    if (_pStreamHistory)
    {
        Assert(!pinfo->pStreamHistory);

        pinfo->pStreamHistory = _pStreamHistory;
        _pStreamHistory->AddRef();
    }

    hr = THR(pDoc->_clsTab.AssignWclsid(pDoc, pinfo->clsid, &_wclsid));
    if (hr)
        goto Error;

    pici = GetInstantClassInfo();
    
    if (pici && (pici->dwCompatFlags & COMPAT_EVIL_DONT_LOAD))
    {
        //
        // The registry says that this control is not allowed to run at all
        // in IE4.
        //

        hr = E_ACCESSDENIED;
        goto Error;
    }

    //
    // If we have a data object, instantiate immediately without
    // code loader.
    //

    if (pinfo->pDataObject)
    {
        hr = THR(CreateObjectNow(IID_IDataObject, pinfo->pDataObject, pinfo));
        if (hr)
            goto Cleanup;
    }
    else
    {
        //
        // If we have any attributes whatsoever with which code-download
        // can do its thang, start the codeloader.  Otherwise, just fail
        // immediately.
        //

#ifndef WIN16
        if (!IsEqualGUID(pinfo->clsid, g_Zero.guid) ||
            !FormsIsEmptyString(pinfo->pchMimeType) ||
            !FormsIsEmptyString(pinfo->pchClassid) ||
            !FormsIsEmptyString(pinfo->pchDataUrl))
        {
            //
            // instantiate immediately through CCodeLoadCtx or launch download
            //

            _pCodeLoad = new CCodeLoad;
            if (!_pCodeLoad)
            {
                hr = E_OUTOFMEMORY;
                goto Error;
            }

            hr = THR(_pCodeLoad->Init(this, pinfo));
            if (S_OK != hr && MK_S_ASYNCHRONOUS != hr)
                goto Error;

            hr = S_OK;
        }
        else
#endif // ndef WIN16
        {
            // this will be a very empty olesite
            Assert (!_pUnkCtrl);
            _state = OS_LOADED;
            OnFailToCreate();
            hr = S_OK;
        }
    }

Cleanup:
    ClearInterface(&_pStreamHistory);

    if (hr && _dwProgCookie)
    {
        //
        // If there was some failure and a progress cookie still exists
        // delete progress sink
        //

        IGNORE_HR(pDoc->GetProgSink()->DelProgress(_dwProgCookie));
        _dwProgCookie = 0;
    }

    pDoc->Release();

    RRETURN(hr);

Error:
    ReleaseCodeLoad();
    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::AllowCreate
//
//  Synopsis:   Returns whether it's ok to create this object after checking
//              permissions on doc, etc.
//
//----------------------------------------------------------------------------

BOOL
COleSite::AllowCreate(REFCLSID clsid)
{
    DWORD   dwAction = 0;
    HRESULT hr;
    BOOL    fAllowed = TRUE;

    if (Tag() == ETAG_APPLET)
    {
        dwAction = URLACTION_JAVA_PERMISSIONS;
    }
    else if (Tag() == ETAG_OBJECT || Tag() == ETAG_EMBED)
    {
        dwAction = URLACTION_ACTIVEX_RUN;
    }

    if (dwAction)
    {
        hr = THR(Doc()->ProcessURLAction(
                dwAction,
                &fAllowed,
                0,
                NULL,
                NULL,
                (BYTE *)&clsid,
                sizeof(CLSID)));

        if (hr || !fAllowed)
        {
            //
            // The control was not allowed to create itself.  Treat page
            // as if control doesn't exist.  This creates no layout
            // space for this site.
            //
            GetCurLayout()->_fInvisibleAtRuntime = TRUE;
        }

    }


       return fAllowed;

}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CreateObjectNow
//
//  Synopsis:   Creates, connects and initializes embedded object from
//              a class factory.
//
//  Arguments:  pCF     The class factory (can be null if intializing from an
//                      IDataObject)
//              pinfo   OLE initialization info
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
COleSite::CreateObjectNow(
    REFIID          riidObject,
    IUnknown *      punkObject,
    OLECREATEINFO * pinfo)
{
    HRESULT hr = S_OK;
    DWORD   dwFlags = 0;
    BOOL    fNoLoad = FALSE;
    CDoc *  pDoc = Doc();
    INSTANTCLASSINFO * pici;

    Assert(!_dwProgCookie);

    //
    // Bail out if not allowed to create control/applet/plugin
    //

    if (!AllowCreate(pinfo->clsid))
    {
        NotifyHaveProtectedUserFromUnsafeContent(pDoc, IDS_PROTECTEDFROMUNSAFEOCX);
        OnFailToCreate();
        goto Cleanup;
    }

    //
    // instantiate object
    //

    if (IsEqualGUID(IID_IDataObject, riidObject))
    {
        hr = THR(InstantiateObjectFromDataObject((IDataObject *)punkObject));
        if (hr)
        {
            goto Error;
        }
    }
    else if (IsEqualGUID(IID_IClassFactory, riidObject))
    {
        hr = THR(InstantiateObjectFromCF((IClassFactory*)punkObject, pinfo));
        if (hr)
            goto Error;
    }
    else
    {
        //
        // No need to perform IPersist*::Load because that has already
        // happened as a result of the BindToObject.
        //

        fNoLoad = TRUE;
        _pUnkCtrl = punkObject;
        _pUnkCtrl->AddRef();
    }

    if (!_pUnkCtrl)
        goto Error;

    // CHROME
    // If we are Chrome hosted, deny any Windowed OCX from loading
    if (pDoc->IsChromeHosted())
    {
        IOleInPlaceObjectWindowless *pObj = NULL;

        Assert(_pUnkCtrl != NULL);

        HRESULT hrTemp = _pUnkCtrl->QueryInterface(IID_IOleInPlaceObjectWindowless, (void**)&pObj);
        if (FAILED(hrTemp))
        {
            OnFailToCreate();
            hr = E_ACCESSDENIED;
            goto Error;
        }

        Assert(pObj != NULL);

        pObj->Release();
    }

    // Register for release object notifications from the Doc.
    RegisterForRelease();

    //
    // If we think the clsid is GUID_NULL, try and get a real wclsid from
    // the control.
    //

    if (pinfo->clsid == g_Zero.clsid)
    {
        IGNORE_HR(AssignWclsidFromControl());
    }

    //
    // Turn off events for the duration of creation
    //

    {
        CNotification   nf;

        nf.FreezeEvents(this, (void *)TRUE);
        Notify(&nf);
    }

    //
    // First stage control creation.
    //

    hr = THR(ConnectControl1(&dwFlags));
    if (hr)
        goto Error;

    AfterObjectInstantiation();

    //
    // Load/InitNew the control
    //

    if (!fNoLoad)
    {
        if (!pinfo->pStreamHistory ||
            !OK(THR(LoadHistoryStream(
                pinfo->pStreamHistory,
                pinfo->pBindCtxHistory))))
        {
            if (pinfo->pStream)
            {
                hr = THR(LoadObject(pinfo->pStream));

                pici = GetInstantClassInfo();
                
                if (pici && (pici->dwCompatFlags & COMPAT_USE_PROPBAG_AND_STREAM) &&
                    pinfo->pPropBag)
                {
                    //
                    // If this is one of those nasty controls that want both
                    // IPersistPropertyBag::Load and IPersistStreamInit::Load,
                    // honor them.
                    //

                    hr = THR(LoadObject(pinfo->pPropBag));
                }
            }
            else if (pinfo->pPropBag)
            {
                hr = THR(LoadObject(pinfo->pPropBag));
            }
            else if (pinfo->pStorage)
            {
                hr = THR(LoadObject(pinfo->pStorage));
            }
            else if (pinfo->pchDataUrl || pinfo->pchFileName)
            {
                CStr cstrSpecialURL;

                if (WrapSpecialUrl(pinfo->pchDataUrl, &cstrSpecialURL, pDoc->_cstrUrl, FALSE, FALSE) != S_OK)
                {
                    hr = E_ACCESSDENIED;
                    goto Error;
                }

                hr = THR(LoadObject(pinfo->pchDataUrl, pinfo->pchFileName));
            }
            else
            {
                hr = THR(InitNewObject());
            }
        }

        //
        // S_FALSE is used in this context to indicate that initialization
        // failed because of a safety problem.  In this case, we use InitNew
        // to initialize to a default state - ignoring any potentially unsafe
        // persistent data.
        if (hr == S_FALSE || hr == E_NOINTERFACE)
        {
            hr = THR(InitNewObject());
            if (hr)
                goto Error;
        }
    }

    // Give derived classes a chance to perform special processing after the object has
    // been loaded in the normal way...
    PostLoad();
    
    //
    // Second stage control creation
    //

    hr = THR(ConnectControl2(&dwFlags));
    if (hr)
        goto Error;

    //
    // Finally transition control to the loaded state.
    //

    hr = THR(TransitionTo(OS_LOADED));
    if (hr)
        goto Error;

    //
    // Set bit informing world that the control is downloaded and
    // ready.
    //

    _fObjAvailable = TRUE;

    OnControlReadyStateChanged(!(dwFlags & FSI_PNSCONNECTED));

    //
    // For certain controls we must add a Progsink until they inplace
    // activate so that the OnLoad() script event won't fire until the
    // control is ready for scripting
    //
    pici = GetInstantClassInfo();
    
    if (pici && (pici->dwCompatFlags & COMPAT_PROGSINK_UNTIL_ACTIVATED))
    {
        Assert( _dwProgCookie == 0 );  // these controls had better not support readystate.
        IGNORE_HR(pDoc->GetProgSink()->AddProgress(
            PROGSINK_CLASS_OTHER,
            &_dwProgCookie));
    }

Cleanup:

    //
    // Now that we've actually created the object, its size could have changed.
    // Also, the way a object initially gets put in its baseline state
    // (inplace active or whatever) is via a SetPos().  Thus we must ensure
    // the size is updated and SetPos() is called sending a ResizeElement notification.
    // If this olesite is being parented to the rootsite directly, there's
    // no need to call RequestResize because this ocx will never be drawn.
    //

    if (    GetFirstBranch()
        &&  GetUpdatedParentLayoutNode()
        &&  GetFirstBranch()->GetCascadeddisplay() != styleDisplayNone)
    {
        ResizeElement();
        pici = GetInstantClassInfo();
        
        if (pici && (pDoc->State() >= OS_INPLACE)
            && (pici->dwCompatFlags & COMPAT_ALWAYS_INPLACEACTIVATE))
        {
            IGNORE_HR(TransitionToBaselineState(pDoc->State()));
        }
    }
    else
    {
        TransitionToBaselineState(pDoc->State());
    }


    //
    // Turn events back on
    //

    {
        CNotification   nf;

        nf.FreezeEvents(this, (void *)FALSE);
        Notify(&nf);
    }

    //
    // If an error occurred make for darn sure that any progsink
    // we created is deleted.
    //
    if (hr && _dwProgCookie)
    {
        IGNORE_HR(pDoc->GetProgSink()->DelProgress(_dwProgCookie));
        _dwProgCookie = 0;
    }

    ReleaseCodeLoad();
    RRETURN(hr);

Error:
    //
    // this codepath completely aborts creation process
    //

    // Revoke client site (if set)
    if (dwFlags & FSI_CLIENTSITESET)
    {
        SetClientSite(NULL);
    }
    ClearInterface(&_pUnkCtrl);

    _state = OS_LOADED;
    goto Cleanup;
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::InstantitateObjectFromCF, public
//
//  Synopsis:   Creates a control using a license manager or class factory.
//
//  Arguments:  [pCF] -- Class Factory to use if license manager not available.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
COleSite::InstantiateObjectFromCF(
    IClassFactory * pCF,
    OLECREATEINFO * pinfo)
{
    HRESULT             hr;
    DWORD               dwCompatFlags;
    CDoc *              pDoc = Doc();
    IClassFactoryEx *   pCFEx = NULL;
    INSTANTCLASSINFO *  pici;
    
    hr = THR(pDoc->_clsTab.AssignWclsid(pDoc, pinfo->clsid, &_wclsid));
    if (hr)
        goto Cleanup;

    pici = GetInstantClassInfo();
    
    dwCompatFlags = pici ? pici->dwCompatFlags : 0;

    // Create the control.  If a license manager exists, use it.
    // Otherwise use provided class factory.
    //

    if (pDoc->_pLicenseMgr)
    {
        hr = THR_OLE(pDoc->_pLicenseMgr->CreateInstance(
                pinfo->clsid,
                (dwCompatFlags & COMPAT_AGGREGATE) ?
                        PunkInner() : NULL,
                IID_IUnknown,
                CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                (void **)&_pUnkCtrl));
    }
    else
    {
        //
        // Try IClassFactory3 first
        //

        if (OK(pCF->QueryInterface(IID_IClassFactoryEx, (void **)&pCFEx)))
        {
            hr = THR(pCFEx->CreateInstanceWithContext(
                (IOleClientSite *)&_Client, 
                (dwCompatFlags & COMPAT_AGGREGATE) ?
                    PunkInner() : NULL,
                IID_IUnknown, 
                (void **)&_pUnkCtrl));
        }
        else
        {
            hr = THR_OLE(pCF->CreateInstance(
                    (dwCompatFlags & COMPAT_AGGREGATE) ?
                            PunkInner() : NULL,
                    IID_IUnknown,
                    (void **)&_pUnkCtrl));
        }
    }
    if (hr)
        goto Cleanup;

    if (dwCompatFlags & COMPAT_AGGREGATE)
    {
#if DBG == 1
    //
    // Debug mode check to verify that when we aggregate controls
    // we really get the private unknown of the control.  Even though
    // we're really selective about what we aggregate there have been
    // cases of certain unnamed controls which don't aggregate correctly.
    //

        IUnknown *  pUnk = NULL;

        hr = THR_OLE(QueryControlInterface(IID_IUnknown, (void **)&pUnk));
        if (OK(hr))
        {
            Assert(pUnk == _pUnkCtrl && "Control did not aggregate correctly");
            ReleaseInterface(pUnk);
        }
#endif
        _fXAggregate = TRUE;
    }

    MemSetName((this, "%s _pUnkCtrl=%08x", MemGetName(this), _pUnkCtrl));

    DbgTrackItf(GUID_NULL,
            "pUnkCtrl",
            TRUE,
            (void **)&_pUnkCtrl);

Cleanup:
    ReleaseInterface(pCFEx);
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::SaveFormatSupported
//
//  Synopsis:   Returns TRUE if the embedded control supports a save format
//              we can use.
//
//  Returns:    BOOL
//
//----------------------------------------------------------------------------

BOOL
COleSite::SaveFormatSupported()
{
    HRESULT                hr;
    IPersistPropertyBag *  pPropBag = NULL;
    IPersistPropertyBag2 * pPropBag2 = NULL;
    IPersistStreamInit *   pStm = NULL;

    hr = THR_OLE(QueryControlInterface(IID_IPersistPropertyBag,
                                       (void **)&pPropBag));
    if (!hr)
        goto Cleanup;

    hr = THR_OLE(QueryControlInterface(IID_IPersistPropertyBag2,
                                       (void **)&pPropBag2));
    if (!hr)
        goto Cleanup;

    hr = THR_OLE(QueryControlInterface(IID_IPersistStreamInit,
                                       (void **)&pStm));

    if (!hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pPropBag);
    ReleaseInterface(pPropBag2);
    ReleaseInterface(pStm);

    return hr == S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::InstantitateObjectFromDataObject, public
//
//  Synopsis:   Creates a control from a data object.
//
//  Arguments:  [pDataObject] -- Data object to load from.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
COleSite::InstantiateObjectFromDataObject(IDataObject * pDataObject)
{
    HRESULT     hr = E_FAIL;
#ifndef WIN16
    static FORMATETC s_formatetcContent =
    {
        0,                  //CLIPFORMAT
        NULL,               //DVTARGETDEVICE FAR*
        DVASPECT_CONTENT,   //dwAspect
        -1,                 //lindex
        TYMED_NULL          //tymed
    };


    IStorage *  pStg = NULL;

    hr = THR(StgCreateDocfile(
                NULL,
                STGM_READWRITE | STGM_DIRECT | STGM_SHARE_EXCLUSIVE | STGM_DELETEONRELEASE,
                0, &pStg));

    if (hr)
        goto Cleanup;


    hr = THR_OLE(OleCreateFromData(pDataObject, IID_IOleObject, OLERENDER_DRAW,
                                   &s_formatetcContent,
                                   NULL,
                                   pStg,
                                   (void **)&_pUnkCtrl));

    if (hr)
        goto Cleanup;

    if (!SaveFormatSupported())
    {
        // If we can not persist the object, it is misleading to instantiate
        // it from the data.
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(AssignWclsidFromControl());
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pStg);
#endif // ndef WIN16

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     COleSite::RegisterForRelease
//
//  Synopsis:   Adds this element to the Doc array to recieve notifications when
//              it should release the contained OLE object
//
//  Returns:    void
//
//-------------------------------------------------------------------------
void
COleSite::RegisterForRelease()
{
    if (!_fRegisteredForRelease)
    {
        Doc()->RequestReleaseNotify(this);
        _fRegisteredForRelease = TRUE;
    }
}

//+------------------------------------------------------------------------
//
//  Member:     COleSite::UnregisterForRelease
//
//  Synopsis:   Removes this element from the Doc array to recieve notifications when
//              it should release the contained OLE object
//
//  Returns:    void
//
//-------------------------------------------------------------------------
void
COleSite::UnregisterForRelease()
{
    if (_fRegisteredForRelease)
    {
        Doc()->RevokeRequestReleaseNotify(this);
        _fRegisteredForRelease = FALSE;
    }
}


//+------------------------------------------------------------------------
//
//  Member:     COleSite::SetHostNames
//
//  Synopsis:   Passes along to this control the host name object
//              provided to the form.  Since the form is always
//              embedded, it is never the original source of the
//              names.  It can only pass along the strings passed
//              to its IOleObject::SetHostNames method.
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------

HRESULT
COleSite::SetHostNames( )
{
    HRESULT hr = S_OK;
    CDoc *  pDoc = Doc();

    if (!_fSetHostNames && ((LPTSTR)pDoc->_cstrCntrApp != NULL))
    {
        IOleObject * pObj;

        // Note: it's legal for the container object name to be NULL.

        hr = THR_OLE(QueryControlInterface(IID_IOleObject, (LPVOID*)&pObj));
        if (!hr)
        {
            hr = THR_OLE(pObj->SetHostNames(
                    pDoc->_cstrCntrApp,
                    pDoc->_cstrCntrObj));

            ReleaseInterface(pObj);
        }

        _fSetHostNames = TRUE;
    }

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     COleSite::SetClientSite
//
//  Synopsis:   Sets the client site to whatever pointer is passed.  Supports
//                both IOleObject::SetClientSite and IObjectWithSite::SetSite
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------

HRESULT
COleSite::SetClientSite(IOleClientSite *pClientSite)
{
    IOleObject *pOleObject = NULL;
    IObjectWithSite *pObjSite = NULL;

#if DBG==1
    if (pClientSite)
    {
        pClientSite->AddRef();
        DbgTrackItf(IID_IOleClientSite, "CClient", FALSE, (void **)&pClientSite);
    }
#endif

    HRESULT hr = QueryControlInterface(IID_IOleObject, (void **)&pOleObject);
    if (OK(hr) && pOleObject)
    {
        hr = pOleObject->SetClientSite(pClientSite);
        ReleaseInterface(pOleObject);
        goto Cleanup;
    }

    hr = QueryControlInterface(IID_IObjectWithSite, (void **)&pObjSite);
    if (OK(hr) && pObjSite)
    {
        hr = pObjSite->SetSite(pClientSite);
        ReleaseInterface(pObjSite);
        goto Cleanup;
    }
    hr = S_OK;
Cleanup:

#if DBG==1
    if (pClientSite)
    {
        pClientSite->Release();
    }
#endif

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     COleSite::TransitionTo, public
//
//  Synopsis:   Manage internal state transitions.
//
//  Arguments:  [state] -- the target state
//
//  Returns:    NOERROR iff sucessful
//
//  Notes:      This method manages "push" requests to
//              get us into a desired state. There are
//              a number of important "pull" requests that
//              change our state without going through here:
//              for example, OnUIDeactivate
//
//              IPD = IOleInPlaceObject::InPlaceDeactivate
//              UID = IOleInPlaceObject::UIDeactivate
//              + means that the state transition is incomplete
//
//   (from)
//           | PASSIVE | LOADED | RUNNING | INPLACE  | UIACTIVE | OPEN
//------------------------------------------------------------------------
//           |         |        |         |          |          |
// PASSIVE   | Illegal |  SHN   |   Err   |    Err   |   Err    |  Err
//           |         |        |         |          |          |
//------------------------------------------------------------------------
//           |         |        |         |          |          |
// LOADED    | Illegal |  NOP   | OleRun  |  DoVerb  |  DoVerb  |  DoVerb
//           |         |        |         |          |          |
//------------------------------------------------------------------------
//           |         |        |         |          |          |
// RUNNING   | Illegal | Close  |   NOP   |  DoVerb  |  DoVerb  |  DoVerb
//           |         |        |         |          |          |
//------------------------------------------------------------------------
//           |         |        |         |          |          |
// INPLACE   | Illegal |  IPD+  |   IPD   |   NOP    |  DoVerb  |   IPD+
//           |         |        |         |          |          |
//------------------------------------------------------------------------
//           |         |        |         |          |          |
// UIACTIVE  | Illegal |  IPD+  |   IPD   |   UID    |   NOP    |   IPD+
//           |         |        |         |          |          |
//------------------------------------------------------------------------
//           |         |        |         |          |          |
// OPEN      | Illegal | DoVerb+| DoVerb  |  DoVerb+ |  DoVerb+ |   NOP
//           |         |        |         |          |          |
//------------------------------------------------------------------------

HRESULT
COleSite::TransitionTo(OLE_SERVER_STATE state, LPMSG pMsg)
{
    HRESULT         hr;
    HWND            hWndCntr;
    IOleObject *    pObj;
    CDoc *          pDoc = Doc();
    INSTANTCLASSINFO * pici;

    COleLayout * pLayout = DYNCAST(COleLayout, GetCurLayout());
    Assert(pLayout);

    Assert(state >= OS_LOADED && state <= OS_OPEN);
    Assert(_state >= OS_PASSIVE && _state <= OS_OPEN);

    //
    // It's ok to try to transition a site to >= OS_INPLACE even
    // if it's _fSizeThis or _fPositionThis is true because it just
    // means that it will be sized and SetObjectRects at sometime later.
    //      Assert(state < OS_INPLACE || _fPositionThis);
    //

    Assert(_cLockInPlaceActive == 0 ||
           (state == OS_INPLACE || state == OS_UIACTIVE));
    Assert(!(state == OS_UIACTIVE && pLayout->_fNoUIActivate));

    //  Early exit if we're already in the requested state.
    if (state == State())
        return S_OK;

    //
    // If the object is not yet available, bail out now.
    // We could get into this situation if the safety confirmation
    // dialog is up for initializing a control
    //

    if (!_fObjAvailable)
        return S_OK;

    //
    // If the transition lock is on, bail out right now.  This means
    // we're in the middle of an ocx requested state change and some
    // document change is causing a transitionTo.
    //

    if (TestLock(OLESITELOCK_TRANSITION))
        return S_OK;

    //  Get state required by multiple state transitions

    hr = THR_NOTRACE(QueryControlInterface(IID_IOleObject, (LPVOID*)&pObj));
    if (!OK(hr))
    {
        if (E_NOINTERFACE == hr)
            hr = S_OK;
        RRETURN(hr);
    }

    hWndCntr = (pDoc->_pInPlace) ? pDoc->_pInPlace->_hwnd : NULL;

    //  Loop until we're in the correct state or an error occurs

    while (state != State() && OK(hr))
    {
        switch(_state)
        {

#if DBG == 1
        //
        //  We don't use the TransitionTo method to get out of
        //  the OS_PASSIVE state.  Use the various IPersist*::Load
        //  methods instead.
        //

        case OS_PASSIVE:
            Assert(0 && "Transitioning a passive olesite");
            hr = E_FAIL;
            break;
#endif
        case OS_LOADED:
            switch (state)
            {
            case OS_RUNNING:
                hr = THR_OLE(OleRun(pObj));
                if (!hr)
                    _state = OS_RUNNING;

                break;

            // in the rest of the cases we can go direct (recall that DoVerb
            // will automatically run!)

            case OS_INPLACE:
                if (_fInsideOut)
                {
                    hr = THR(InPlaceActivate(pObj, hWndCntr, pMsg));
                }
                if (_state != OS_INPLACE && OK(hr))
                {
                    hr = E_NOTIMPL;
                }
                break;

            case OS_UIACTIVE:
                if (!NoUIActivate())
                {
                    CRect   rc;

                    pLayout->GetClientRect(&rc, COORDSYS_GLOBAL);

                    hr = THR_OLE(pObj->DoVerb(
                            OLEIVERB_UIACTIVATE,
                            pMsg,
                            &_Client,
                            0,
                            hWndCntr,
                            ENSUREOLERECT(&rc)));
                }
                if (_state != OS_UIACTIVE && OK(hr))
                {
                    hr = E_NOTIMPL;
                }
                break;

            case OS_OPEN:
                hr = THR(SetHostNames());
                if (!hr && !NoUIActivate())
                {
                    CRect   rc;

                    pLayout->GetClientRect(&rc, COORDSYS_GLOBAL);

                    hr = THR_OLE(pObj->DoVerb(
                            OLEIVERB_OPEN,
                            pMsg,
                            &_Client,
                            0,
                            hWndCntr,
                            ENSUREOLERECT(&rc)));
                    if (_state != OS_OPEN && OK(hr))
                    {
                        hr = E_NOTIMPL;
                    }
                }
                break;
            }
            break;

        case OS_RUNNING:
            switch(state)
            {
            case OS_LOADED:

                //  NOTE it is legal to close an object merely
                //    by letting go of all pointers to it; however,
                //    in practice we've encountered objects which
                //    do not cope with this usage.  So, we explicitly
                //    close everything.  (garybu, chrisz)

                hr = THR_OLE(pObj->Close(OLECLOSE_NOSAVE));
                if (!hr || hr==E_NOTIMPL)   // MS License Manager returns E_NOTIMPL!
                {
                    _state = OS_LOADED;
                    hr = S_OK;
                }
                break;

            case OS_INPLACE:
            case OS_UIACTIVE:
                if (_fInsideOut)
                {
                    hr = THR(InPlaceActivate(pObj, hWndCntr, pMsg));
                }
                if (_state != OS_INPLACE && OK(hr))
                {
                    hr = E_NOTIMPL;
                }
                break;

            case OS_OPEN:
                hr = THR(SetHostNames());
                if (!hr && !NoUIActivate())
                {
                    CRect   rc;

                    pLayout->GetClientRect(&rc, COORDSYS_GLOBAL);

                    hr = THR_OLE(pObj->DoVerb(
                            OLEIVERB_OPEN,
                            pMsg,
                            &_Client,
                            0,
                            hWndCntr,
                            ENSUREOLERECT(&rc)));
                    if (_state != OS_OPEN && OK(hr))
                    {
                        hr = E_NOTIMPL;
                    }
                }
                break;
            }
            break;

        case OS_INPLACE:
            switch(state)
            {
            case OS_UIACTIVE:
                if (!NoUIActivate())
                {
                    CRect   rc;

                    pLayout->GetClientRect(&rc, COORDSYS_GLOBAL);

                    hr = THR_OLE(pObj->DoVerb(
                            OLEIVERB_UIACTIVATE,
                            pMsg,
                            &_Client,
                            0,
                            hWndCntr,
                            ENSUREOLERECT(&rc)));
                }
                if (_state != OS_UIACTIVE && OK(hr))
                {
                    hr = E_NOTIMPL;
                }
                break;

            default:
                if (_cLockInPlaceActive)
                {
                    hr = E_FAIL;
                }
                else
                {   // Some controls count on being hidden before deactivating:
                    pici = GetInstantClassInfo();
                    
                    if (pici && (pici->dwCompatFlags & COMPAT_SEND_HIDE))
                    {
                        CRect   rc;

                        pLayout->GetClientRect(&rc, COORDSYS_GLOBAL);

                        hr = THR_OLE(pObj->DoVerb(
                                OLEIVERB_HIDE,
                                pMsg,
                                &_Client,
                                0,
                                hWndCntr,
                                ENSUREOLERECT(&rc)));
                    }
                    // That OLEIVERB_HIDE above may have put us in a different
                    // state....
                    if( _state >= OS_INPLACE && NULL != _pInPlaceObject )
                    {
                        // The following pulls us to OS_RUNNING
                        hr = THR_OLE(_pInPlaceObject->InPlaceDeactivate());
                    }
                }

                // if everything went well then our state should already be
                // in the running state.  Otherwise, something went wrong
                if (_state != OS_RUNNING && !hr)
                {
                    hr = E_FAIL;
                }
                break;
            }
            break;

        case OS_UIACTIVE:
            switch(state)
            {
            case OS_INPLACE:
                Assert(_fInsideOut);

                hr = THR_OLE(_pInPlaceObject->UIDeactivate());
                if (_state != OS_INPLACE && !hr)
                {
                    hr = E_FAIL;
                }
                break;

            default:
                // Some controls count on being hidden before deactivating:
                pici = GetInstantClassInfo();
                
                if (pici && (pici->dwCompatFlags & COMPAT_SEND_HIDE))
                {
                    CRect   rc;

                    pLayout->GetClientRect(&rc, COORDSYS_GLOBAL);

                    hr = THR_OLE(pObj->DoVerb(
                            OLEIVERB_HIDE,
                            pMsg,
                            &_Client,
                            0,
                            hWndCntr,
                            ENSUREOLERECT(&rc)));
                }

                // That OLEIVERB_HIDE above may have put us in a different
                // state....
                if( _state >= OS_INPLACE && NULL != _pInPlaceObject )
                {
                    hr = THR_OLE(_pInPlaceObject->InPlaceDeactivate());
                }
                if (_state != OS_RUNNING && !hr)
                {
                    hr = E_FAIL;
                }
                break;
            }
            break;

        case OS_OPEN:
            {
                CRect   rc;

                pLayout->GetClientRect(&rc, COORDSYS_GLOBAL);

                hr = THR_OLE(pObj->DoVerb(
                        OLEIVERB_HIDE,
                        pMsg,
                        &_Client,
                        0,
                        hWndCntr,
                        ENSUREOLERECT(&rc)));
            }
            if (!hr)
            {
                pLayout->Invalidate();
                _state = OS_RUNNING;
                // CHROME
                // If Chrome there is no valid HWND so ::SetFocus() cannot
                // be used. Instead use CServer::SetFocus() which handles
                // the windowless case
                if (!pDoc->IsChromeHosted())
                    ::SetFocus(hWndCntr);
                else
                    pDoc->SetFocus(pDoc->_pInPlace ? TRUE : FALSE);
            }
            break;
        }
    }

    ReleaseInterface(pObj);

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     COleSite::TransitionToBaselineState
//
//  Synopsis:   Transitions this object to its proper baseline state,
//              given a maximum state allowed to the object
//
//  Arguments:  [osMax]     --  Maximum state allowed
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
COleSite::TransitionToBaselineState(OLE_SERVER_STATE osMax)
{
    HRESULT hr;

    osMax = BaselineState(osMax);

    if (State() != osMax)
    {
        hr = THR(TransitionTo(osMax));
    }
    else
    {
        hr = S_OK;
    }

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     COleSite::TransitionToCorrectState
//
//  Synopsis:   In some circumstances, the object's baseline state may
//              change.  This method transitions the object to its new
//              baseline state, unless the object is currently the UI
//              Active object, in which case it is unaffected.
//
//  Arguments:  [osMax]     --  Maximum state allowed
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
COleSite::TransitionToCorrectState()
{
    HRESULT hr;

    if (_state == OS_UIACTIVE)
    {
        hr = S_OK;
    }
    else
    {
        hr = THR(TransitionToBaselineState(Doc()->State()));
    }

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     COleSite::InPlaceActivate
//
//  Synopsis:   Helper to encapsulate the inplace activation process
//
//  Arguments:  [pOleObject]    --  Ptr to the IOleObject
//              [hwnd]          --  Hwnd of the container
//              [pMsg]          --  Message to send while inplace activating
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
COleSite::InPlaceActivate(IOleObject *pOleObject, HWND hwnd, LPMSG pMsg)
{
    CRect   rc;
    HRESULT hr;
    CLock   Lock(this, OLESITELOCK_INPLACEACTIVATE);

    COleLayout * pLayout = DYNCAST(COleLayout, GetCurLayout());
    Assert(pLayout);

    INSTANTCLASSINFO * pici = GetInstantClassInfo();

    if (pici && (pici->dwCompatFlags & COMPAT_SEND_SHOW))
    {
        pLayout->GetClientRect(&rc, COORDSYS_GLOBAL);

        hr = THR_OLE(pOleObject->DoVerb(
                OLEIVERB_SHOW,
                pMsg,
                &_Client,
                0,
                hwnd,
                ENSUREOLERECT(&rc)));

        // Some buggy controls return S_FALSE instead of S_OK when they succeed.  This is
        // normally caused by returning the value of a boolean function which succeeds.  Treat
        // this scode as S_OK.  -- philco
        
        hr = (hr == S_FALSE) ? S_OK : hr;
            
        if (hr)
            goto Cleanup;
    }

        pLayout->GetClientRect(&rc, COORDSYS_GLOBAL);

        hr = THR_OLE(pOleObject->DoVerb(
                OLEIVERB_INPLACEACTIVATE,
                pMsg,
                &_Client,
                0,
                hwnd,
                ENSUREOLERECT(&rc)));

    // Some buggy controls return S_FALSE instead of S_OK when they succeed.  This is
    // normally caused by returning the value of a boolean function which succeeds.  Treat
    // this scode as S_OK.  -- philco
    
    hr = (hr == S_FALSE) ? S_OK : hr;
            
    if (hr)
        goto Cleanup;

    pici = GetInstantClassInfo();
    
    if (pici && _dwProgCookie &&
        (pici->dwCompatFlags & COMPAT_PROGSINK_UNTIL_ACTIVATED))
    {
        Doc()->GetProgSink()->DelProgress(_dwProgCookie);
        _dwProgCookie = 0;
    }

    //
    // After inplace-activation also send down a SetObjectRects.  It
    // should not be necessary to do this, but unfortunately the marcwan
    // basectl framework has a bug where it does not clip correctly
    // if it's only inplace-activated without a subsequent SetObjectRects.
    // The bug is that the basectl framework does not go through its
    // SetWindowRgn code if it's only inplace-activated.  A SetObjectRects
    // after inplace-activation forces it down that code path. (anandra)
    //

    if (State() >= OS_INPLACE && _pInPlaceObject)
    {
        CView *     pView     = Doc()->GetView();
        CDispNode * pDispNode = pView->IsActive()
                                    ? pLayout->GetElementDispNode()
                                    : NULL;

        if (pDispNode)
        {
            // make sure our deferred calls get processed later
            pView->OpenView();

            DWORD       grfFlags = pDispNode->IsInView()
                                        ? VCF_INVIEW | VCF_INVIEWCHANGED | VCF_POSITIONCHANGED
                                        : VCF_INVIEWCHANGED | VCF_POSITIONCHANGED;
            CRect       rc;
            CRect       rcClip;

            pLayout->GetClientRect(&rc, COORDSYS_GLOBAL);
            pLayout->GetClippedClientRect(&rcClip, COORDSYS_GLOBAL);

            pLayout->HandleViewChange(grfFlags, &rc, &rcClip, pDispNode);
        }
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::GetHwnd, CSite
//
//  Synopsis:   Return control's window, if any
//
//----------------------------------------------------------------------------

HWND
COleSite::GetHwnd()
{
    HWND hwnd = NULL;
    HWND hwndParent;
    CDoc *  pDoc = Doc();

    if (!_fObjAvailable || !_pInPlaceObject || !pDoc->_pInPlace)
        return NULL;

    _pInPlaceObject->GetWindow(&hwnd);
    if (!hwnd)
        return NULL;

    hwndParent = ::GetParent(hwnd);
    if (hwndParent == pDoc->_pInPlace->_hwnd)
        return hwnd;

    // The object is being tricky and has inserted some windows
    // between the form and the window it returns from
    // IOleInPlaceActiveObject::GetWindow.  Find out what the
    // window handle that is a direct descendant of us is.

    if (!IsChild(pDoc->_pInPlace->_hwnd, hwnd))
    {
        TraceTag((tagError, "Window handle returned from object not a child of the Form!"));
        return hwnd;
    }

    while (hwndParent != pDoc->_pInPlace->_hwnd)
    {
        hwnd = hwndParent;
        hwndParent = ::GetParent(hwnd);
    }

    return hwnd;
}

//+---------------------------------------------------------------
//
//  Member:     COleSite::Passivate
//
//  Synopsis:   Called when main CSite reference count (_ulRefs) drops
//              to zero. Do not call this method directly.  Use
//              IUnknown::Release instead.
//
//---------------------------------------------------------------

void
COleSite::Passivate()
{
#if defined(UNIX) && defined(ux10)
//  IEUNIX:  BUGBUG : Ensure that the this object is not deleted before the
//  end of the function. (ReleaseObject results in deletion of this.

    CLock  lock(this);
#endif
    ReleaseObject();

    Assert(_cLockInPlaceActive == 0);
    Assert(!_pInPlaceObject);

    if (_pAtomTable)
    {
        _pAtomTable->Free();
        delete _pAtomTable;
        _pAtomTable = NULL;
    }

    // Release control's private unknown.
    ClearInterface(&_pUnkCtrl);

    ClearInterface(&_pStreamHistory);

    ClearRefresh();
    super::Passivate();
}

void
COleSite::ClearRefresh()
{
    FormsKillTimer(this, REFRESH_TIMER_ID);
    FormsFreeString(_pstrRefreshURL);
    _pstrRefreshURL = NULL;
}

//+---------------------------------------------------------------
//
//  Member:     COleSite::EnterTree
//
//  Synopsis:   Notification that this object is entering the tree.
//
//---------------------------------------------------------------
// BUGBUG:: No one calls this?!? (jbeda)
void
COleSite::EnterTree()
{
    // Since we are entering the tree, this object should transition
    // to the correct state.
    if (_fObjAvailable)
        IGNORE_HR(TransitionToBaselineState(Doc()->State()));

#ifndef NO_DATABINDING
    Doc()->GetDataBindTask()->SetWaiting();
#endif // ndef NO_DATABINDING

}

//+---------------------------------------------------------------
//
//  Member:     COleSite::ReleaseObject
//
//  Synopsis:   Disconnect from the form.
//
//---------------------------------------------------------------

void
COleSite::ReleaseObject()
{

    // Ensure this object stays alive until the function returns.
    CLock lock(this);

    // if the underlying OLE object is already released, simply return.
    if (_fObjectReleased)
        return;

    _fObjectReleased = TRUE;

#ifndef NO_DATABINDING
    DetachDataBindings();
#endif // ndef NO_DATABINDING

    //         Temporary hack for controls built with MFC 4.0 (VC 3.0).
    //         COleControl::~COleControl does not release our advise
    //         sink, so we force COleControl to release it explicitly
    //         here.  We check for IViewObjectEx to avoid hurting the
    //         performance of our own controls.
    if (_pVO && !_fUseViewObjectEx)
    {
        IGNORE_HR(_pVO->SetAdvise(DVASPECT_CONTENT, 0, NULL));
    }

    // If we are still holding onto the param bag, release it now.
    ReleaseParamBag();

    // Break reference count loop when control is aggregated
    // by releasing cached object pointers.

    ClearInterface(&_pDisp);
    ClearInterface(&_pVO);
    _fUseViewObjectEx = FALSE;

    // Since site's are generic non-ole things, we have to explicitly
    // ensure that we're at most in the loaded state.
    if (_state > OS_LOADED)
    {
        IGNORE_HR(TransitionTo(OS_LOADED));
    }

    Assert(_state <= OS_LOADED && "Object did not shutdown correctly.");

    // The above transitions the site from states above OS_LOADED
    // to OS_LOADED.  If all went well, the site should now be in either
    // the OS_LOADED or OS_PASSIVE state.  No matter what happened,
    // we now assume the site is in the OS_PASSIVE state.  This assumption
    // simplifies checks for unexpected calls in the client interfaces.

    _state = OS_PASSIVE;
    _fWindowlessInplace = FALSE;
    _fUseInPlaceObjectWindowless = FALSE;

    ReleaseCodeLoad();

    if (_dwProgCookie)
    {
        CDoc * pDoc = Doc();

        // Please DO NOT remove this if or the assert in the else clause.. 
        // If a control is listed in the compat list with COMPAT_PROGSINK_UNTIL_ACTIVATED
        // and then the control is modified to support the readystate w/o updating the
        // clsid of the control, we will crash, since the CMarkup has released the CProgSink
        // already.(ferhane)
        if (pDoc->GetProgSink())
            IGNORE_HR(pDoc->GetProgSink()->DelProgress(_dwProgCookie));
#if DBG==1      
        else
        {
            AssertSz(pDoc->GetProgSink(), "Possible update of a control we have in the compat list.");
        }
#endif    

        _dwProgCookie = 0;
    }

    // Revoke the client site.
    SetClientSite(NULL);

    // Disconnect property notify sink.  Shouldn't need to do this,
    // but MSN controls will leak if we do not.

    if (_dwPropertyNotifyCookie)
    {
        DisconnectSink(_pUnkCtrl, IID_IPropertyNotifySink, &_dwPropertyNotifyCookie);
    }

    UnregisterForRelease();
}

HRESULT
COleSite::GetColors(CColorInfo *pCI)
{
    HRESULT hr;

    if (_pVO)
        hr = pCI->AddColors(_pVO);
    else
        hr = S_FALSE;

    RRETURN1(hr, S_FALSE);
}

//+-------------------------------------------------------------------------
//
//  Method:     COleSite::YieldCurrency
//
//  Synopsis:   Relinquish currency
//
//  Arguments:  pSiteNew    New site that wants currency
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

HRESULT
COleSite::YieldCurrency(CElement * pElemNew)
{
    HRESULT hr;
    CDoc *  pDoc = Doc();

    hr = THR(super::YieldCurrency(pElemNew));
    if (hr)
        goto Cleanup;

    // IE5 #62775. The control needs to yield UI. Remaining ui-active while not having
    // focus/currency is bad.
    if (_state >= OS_UIACTIVE)
    {
        // First, reset _pElemUIActive, in order to prevent the call to BecomeCurrentAndActive
        // on the parent in COleSite::CClient::OnUIDeactivate()
        pDoc->_pElemUIActive = NULL;

        YieldUI(pDoc->PrimaryRoot());
    }

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     COleSite::YieldUI
//
//  Synopsis:   Relinquish UI
//
//  Arguments:  pSiteNew    New site that wants UI
//
//--------------------------------------------------------------------------

void
COleSite::YieldUI(CElement *pElemNew)
{
    Assert(OS_UIACTIVE == _state);
    IGNORE_HR(TransitionToBaselineState(OS_INPLACE));
}


//+-------------------------------------------------------------------------
//
//  Method:     COleSite::BecomeUIActive
//
//  Synopsis:   Force ui activity on the site.
//
//  Notes:      This is the method that external objects should call
//              to force sites to become ui active.
//
//--------------------------------------------------------------------------

HRESULT
COleSite::BecomeUIActive()
{
    // Nothing to do here.  Controls are responsible for getting
    // into the UI active state if that's what they want.
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::ActivationChangeRequiresRedraw
//
//  Synopsis:   Determine if change in activation requires redraw.
//
//----------------------------------------------------------------------------

BOOL
COleSite::ActivationChangeRequiresRedraw()
{
#if 0
    BOOL fAbove = FALSE;

    Assert(!_fWindowlessInplace);

    // If the site's window will be positioned above some other site
    // with a window, or if the site will pop above a windowless site,
    // then redraw is required.

    return Doc()->_pSiteRoot->GetCurLayout()->CheckLayoutIntersect(
                            GetCurLayout(),
                            &fAbove,
                            SI_ABOVE | SI_BELOW | SI_BELOWWINDOWED);
#else
// BUGBUG: Fix this! (brendand)
    return FALSE;
#endif
}


//+------------------------------------------------------------------------
//
//  Member:     COleSite::BaselineState
//
//  Synopsis:   Returns the OLE state this object should be in, barring
//              other special circumstances, and subject to a "maximum"
//              state the object is allowed
//
//  Parameters: [osMax]     --  Maximum state allowed to the object;
//                              usually determined by the state of the
//                              enclosing form
//
//  Returns:    OLE_SERVER_STATE
//
//-------------------------------------------------------------------------

OLE_SERVER_STATE
COleSite::BaselineState(OLE_SERVER_STATE osMax)
{
    OLE_SERVER_STATE    os;
    INSTANTCLASSINFO *  pici = GetInstantClassInfo();
    
#if DBG == 1
    Assert(!g_Zero.ab[0]);
#endif

    // If we're locked in the inplace-active state, that's our baseline.
    //

    if (_cLockInPlaceActive > 0)
    {
        os = OS_INPLACE;
    }
    else if (pici && !Doc()->_fDesignMode
             && (   pici->dwCompatFlags & COMPAT_ALWAYS_INPLACEACTIVATE
                 || pici->dwCompatFlags & COMPAT_INPLACEACTIVATEEVENWHENINVISIBLE
                 || (_fActivateWhenVisible && (IsVisible(TRUE)))))
    {
        //
        // The baseline is inplace if the following conditions are met:
        //  1)  the doc is in browse mode           AND
        //  2)  the ocx is visible                  AND
        //  3)  we're an activate-when-visible ocx  OR
        //  4)  it's one which requires inplace
        //      activation *all* the time.

        //  The visibility check is defined as the ocx's visible property
        //  should be set to true (hidden is not set) and the site is
        //  in the doc's current view rect.  However, if all these conditions
        //  are met except for the view rect one, we look at the current state.
        //  If this is > INPLACE, then we stay right here.
        //

        os = OS_INPLACE;
    }
    else if (_fAlwaysRun || _state >= OS_RUNNING)
    {
        //
        // Otherwise if this control is marked to be always running
        // or if we're already greater than running, the base-line
        // is running.
        //

        os = OS_RUNNING;
    }
    else
    {
        //
        // Otherwise, the baseline is just plain old OS_LOADED.
        //

        os = OS_LOADED;
    }

    if (os > osMax)
        os = osMax;

    return os;
}

//+------------------------------------------------------------------------
//
//  Member:     COleSite::CacheDispatch
//
//  Synopsis:   Fill in our pointer to the control's IDispatch, if it
//              has one.
//
//-------------------------------------------------------------------------

void
COleSite::CacheDispatch()
{
    if (_fDispatchCached || !_pUnkCtrl)
        return;

    INSTANTCLASSINFO * picti = GetInstantClassInfo();

    // if we havne' determined if the control is IDispatchEx yet, then
    // call this.
    if (picti && !picti->HasIDex2BeenCalled())
    {
        IDispatchEx * pDispEx2 = NULL;

        THR_NOTRACE(QueryControlInterface(IID_IDispatchEx,
                                               (void **)&pDispEx2));
        picti->SetIsDispatchEx2( !!pDispEx2 );
        ReleaseInterface(pDispEx2);
    }

    IGNORE_HR(QueryControlInterface(IID_IDispatch, (void **)&_pDisp ));

    _fDispatchCached = TRUE;
}

HRESULT
COleSite::Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd)
{
    HRESULT hr = super::Save ( pStreamWrBuff, fEnd );
    if ( !hr )
    {
        _fDirty = FALSE;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::IsClean, COleSite
//
//  Synopsis:   Return S_OK if contained object is clean.
//
//----------------------------------------------------------------------------

HRESULT
COleSite::IsClean(void)
{
    HRESULT hr = S_FALSE;

    // If we are an HTML control site don't use the control's dirty state
    // because we are setting properties in it from HTML

    if (_fDirty)
        return S_FALSE;

    IPersistStreamInit * pPSI;

    if (OK(THR_OLE(QueryControlInterface(IID_IPersistStreamInit, (void **)&pPSI))))
    {
        hr = THR_OLE(pPSI->IsDirty());
        pPSI->Release();
    }

    hr = (hr == S_FALSE) ? S_OK : S_FALSE;

    RRETURN1(hr, S_FALSE);
}


//+------------------------------------------------------------------------
//
//  Member:     COleSite::GetReadyState
//
//  Synopsis:   Returns the readystate of the object.  If not supported
//              an error is returned
//
//  Arguments:  plReadyState    Resultant readystate
//
//  Returns:    S_OK if control supports ReadyState natively.
//              S_FALSE if control doesn't support ReadyState.
//
//-------------------------------------------------------------------------

HRESULT
COleSite::GetReadyState(long *plReadyState)
{
    HRESULT     hr = S_OK;
    CVariant    Var;

    CacheDispatch();
    if (!_pDisp)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR_NOTRACE(GetDispProp(
            _pDisp,
            DISPID_READYSTATE,
            g_lcidUserDefault,
            &Var,
            NULL));
    if (hr)
        goto Cleanup;

    //
    // Look for either VT_I4 or VT_I2
    //

    if (V_VT(&Var) == VT_I4)
    {
        *plReadyState = V_I4(&Var);
    }
    else if (V_VT(&Var) == VT_I2)
    {
        *plReadyState = V_I2(&Var);
    }
    else
    {
        hr = E_FAIL;
    }

#if DBG==1
    if (hr == S_OK)
    {
        VerifyReadyState(*plReadyState);
    }
#endif

Cleanup:

    // If the control doesn't support the readystate property, that's ok.
    // Just return S_FALSE to indicate that.
    if (DISP_E_MEMBERNOTFOUND == hr)
        hr = S_FALSE;

    RRETURN1_NOTRACE(hr, S_FALSE);
}


//+------------------------------------------------------------------------
//
//  Member:     COleSite::GetBoolPropertyOnObject
//
//  Synopsis:   Returns TRUE if the property is not a member of the control or
//              if the property exists it's value is TRUE.  Returns FALSE only
//              if the property exists and it's value is FALSE.
//
//  Arguments:  dispid -- dispatch ID of property to read.
//
//  Returns:    BOOL
//
//-------------------------------------------------------------------------

BOOL
COleSite::GetBoolPropertyOnObject(DISPID dispid)
{
    HRESULT hr;
    BOOL    fBoolean = TRUE;            // Any failure with the one exception
                                        // will return TRUE (see below).

    CacheDispatch();

    // Does the object support IDispatch?
    if (_pDisp)
    {
        // Yes, try to get the property?
        CVariant var;

        hr = THR_NOTRACE(GetDispProp(_pDisp,
                                     dispid,
                                     LOCALE_SYSTEM_DEFAULT,
                                     &var));
        if (!hr)
        {
            if (var.vt == VT_BOOL)
                fBoolean = var.boolVal;
            else
                hr = DISP_E_MEMBERNOTFOUND;
        }

        // Did we get a problem accessing the property?
        if (hr && !DISPID_NOT_FOUND(hr))
        {
            // some other error: the DISPID_ENABLED property will disable the
            // control BUGBUG: is this correct?

            // Yes, and it wasn't because the property wasn't there so set it to
            // false.
            TraceTag((tagWarning, "COleSite::GetBoolPropertyOnObject -- %l "
                                     "via Dispatch returns %hr", dispid, hr));

            fBoolean = FALSE;
        }
    }

    return fBoolean;
}


//+---------------------------------------------------------------------------
//
//  Member:     HandleMessage
//
//  Synopsis:   Handle messages bubbling when the passed site is non null
//
//  Arguments:  [pMessage]  -- message
//              [pChild]    -- pointer to child when bubbling allowed
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT BUGCALL
COleSite::HandleMessage(CMessage * pMessage)
{
    HRESULT         hr      = S_FALSE;
    IOleControl *   pCtrl   = NULL;
    LRESULT         lr;
    CDoc *          pDoc = Doc();
    BOOL fBranchTaken = FALSE;
    BOOL fEditable = IsEditable();
    
    // Note: WM_SETCURSOR and WM_CONTEXTMENU messages can arrive
    // here after bubbling up from a windowed control in another
    // thread.  We are careful about calling the control methods
    // in this scenario because OLE does not allow it.

#if !defined(WINCE) && !defined(_MAC)
    if (pMessage->message != WM_ERASEBKGND &&
        pMessage->message != WM_SETCURSOR &&
        InSendMessage())
    {
        //
        // If the message is not erasebkgnd or setcursor and
        // we're in the middle of some ole thread synchronization
        // then re-post this message so that the control thread
        // is no longer waiting on us.  Add other messages here on
        // a case-by-case basis.
        //

        PostMessage(
            pDoc->_pInPlace->_hwnd,
            pMessage->message,
            pMessage->wParam,
            pMessage->lParam);
        hr = S_OK;  // To signify that we handled the message.
        goto Cleanup;
    }
#endif // GAL_VERSION

    // Check for special internal messages first.

    if (pMessage->message == WM_MOUSELEAVE)
    {
        fBranchTaken = TRUE;
        if (_fWindowlessInplace)
        {
            hr = THR_OLE(((IOleInPlaceObjectWindowless *)_pInPlaceObject)->
                OnWindowMessage(
                    pMessage->message,
                    pMessage->wParam,
                    pMessage->lParam,
                    &lr));
        }

        if (_fDeactivateOnMouseExit)
        {
            IGNORE_HR(TransitionToCorrectState());
        }
    }

    if (!fBranchTaken )        
    {
        if (_fWindowlessInplace)
        {
            //
            // Otherwise, if the control we're holding is windowless
            // inplace, let it handle the message.
            //
#ifdef _MAC
            pMessage->lParam = MAKELPARAM(pMessage->ptContent.x, pMessage->ptContent.y);
#endif
            hr = THR_OLE(((IOleInPlaceObjectWindowless *)_pInPlaceObject)->
                    OnWindowMessage(
                        pMessage->message,
                        pMessage->wParam,
                        pMessage->lParam,
                        &lr));

        }
        else if (pMessage->message == WM_CONTEXTMENU)
        {
            //
            // Show context menu on behalf of the control.
            // A number of control don't handle themselves context
            // menus upon reception of WM_CONTEXTMENU, but do it
            // instead upon reception of WM_RBUTTONUP. To compensate,
            // in browse mode, eat up the context menu message and never
            // show a context menu on behalf of the ocx. (anandra)
            //

            //
            // BUGBUG - certain OLE Sites can "eat" WM_RBUTTONUP messages.
            // we hence pass the WM_CONTEXTMENU to mshtmled where we can set state saying we got an RBUTTONUP
            //
            if (fEditable)
            {
                hr = THR(OnContextMenu(
                        (short) LOWORD(pMessage->lParam),
                        (short) HIWORD(pMessage->lParam),
                        CONTEXT_MENU_CONTROL));
            }
            else
            {
                hr = S_OK;
            }
        }
        else if (_state < OS_INPLACE)
        {
            // Otherwise, the form is in run-mode and the control is inactive
            // Pass messages to inactive control as appropriate.
            switch (pMessage->message)
            {
                //
                // marka - we used to always go UI Active on Mouse Down.
                // we now want to go UI Active on Mouse UP in design mode
                // this allows single click dragging of site selected controls 
                // ( because control wont go UI Active on Mouse Down ).
                //
            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
                if ( ! fEditable )
                {
                    hr = THR(OnInactiveMouseButtonMessage(pMessage, &lr));
                }
                break;

                //
                // Always go UI Active on double click
                //
            case WM_LBUTTONDBLCLK:  
                if ( ! fEditable )
                {
                    hr = THR(OnInactiveMouseButtonMessage(pMessage, &lr));                                    
                }                
                else
                {
                    //
                    // At design time - the decision to go Active is up tot hte tracker
                    //
                    hr = S_FALSE;
                }
                break;

            case WM_SETCURSOR:
                if ( ! fEditable )
                {
                    hr = THR(OnInactiveMouseButtonMessage(pMessage, &lr));
                }
                else
                {
                    SetCursorStyle(IDC_SIZEALL);
                    hr = S_OK;
                }
                break;
                
            case WM_MOUSEMOVE:
                if ( ! fEditable )
                {
                    hr = THR(OnInactiveMousePtrMessage(pMessage, GetFirstBranch(), &lr));
                }
                else
                {
                    hr = S_FALSE;
                }                    
                break;

#ifndef NO_MENU
            case WM_INITMENUPOPUP:
                hr = THR(OnInitMenuPopup(
                        (HMENU) pMessage->wParam,
                        (int) LOWORD(pMessage->lParam),
                        (BOOL) HIWORD(pMessage->lParam)));
                break;
#endif // NO_MENU

            default:
                hr = THR(super::HandleMessage(pMessage));
                break;
            }
        }
        else if (!((pMessage->dwKeyState & FALT) &&
                (pMessage->message == WM_KEYDOWN ||
                pMessage->message == WM_SYSKEYDOWN ||
                pMessage->message == WM_CHAR ||
                pMessage->message == WM_SYSCHAR) &&
                pMessage->wParam != VK_MENU))
        {
            //
            // Handle WM_SETCURSOR & WM_ERASEBKGND right here due to
            // the ole synchronization problem with multiple threads, where
            // the ctrl thread is waiting on processing of these messages.
            //

            if (pMessage->message == WM_ERASEBKGND ||
                pMessage->message == WM_SETCURSOR)
            {
                hr = S_FALSE;
            }
            else
            {
                // Otherwise, if the message is not an accelerator, as signified
                // by the alt key not being held down, pass it to default processing.
                hr = super::HandleMessage(pMessage);
                // No bubbling up to parent here.
                goto Cleanup;
            }
        }
        else
        {
            hr = THR(super::HandleMessage(pMessage));
            goto Cleanup;
        }
    }

Cleanup:
    ReleaseInterface(pCtrl);
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::OnMnemonic, public
//
//  Synopsis:   Indicates that our mnemonic has been pressed by the user.
//
//  Arguments:  [pMsg] -- Message corresponding to our mnemonic
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
COleSite::OnMnemonic(LPMSG lpmsg)
{
    HRESULT         hr      = S_FALSE;
    IOleControl *   pCtrl   = NULL;

    if (!THR_OLE(QueryControlInterface(IID_IOleControl, (void **) &pCtrl)))
    {
        hr = THR_OLE(pCtrl->OnMnemonic(lpmsg));
        pCtrl->Release();
    }

    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::OnMenuEvent, public
//
//  Synopsis:   Handles context menu items for this site.
//
//  Arguments:  [id]   -- Menu item ID
//              [code] -- Menu code
//
//  Returns:    TRUE if the message is handled.  FALSE if we want the form
//              to provide default behavior for that menu item.
//
//----------------------------------------------------------------------------

BOOL
COleSite::OnMenuEvent(int id, UINT code)
{

    if (Doc()->_fDesignMode)
    {
        //
        //  Embedded object verbs on the context menu.  All other context menu
        //  items are handled by the form.
        //

        if (id >= IDM_OBJECTVERBLIST0 && id <= IDM_OBJECTVERBLISTLAST)
        {
            DoEmbedVerbs(id - IDM_OBJECTVERBLIST0);
            return TRUE;
        }

    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Method:     COleSite::OnControlChanged
//
//  Synopsis:   Give Site (or its derived class) a chance to act on a
//              control's OnChanged notification,  and cancel any forwarding
//              which might normally take place through our XObject.
//
//  Arguments:  dispid of property who's value has changed
//
//  Returns:    HRESULT.  Any non-zero HRESULT, including S_FALSE, will
//              cancel OnChanged forwarding.
//
//--------------------------------------------------------------------------

HRESULT
COleSite::OnControlChanged(DISPID dispid)
{
    HRESULT             hr          = S_OK;

    if (dispid != DISPID_READYSTATE)
        _fDirty = TRUE;

    // handle changes in readystate
    if (dispid == DISPID_READYSTATE || dispid == DISPID_UNKNOWN)
    {
        OnControlReadyStateChanged();
    }

    RRETURN1(hr, S_FALSE);
}

//+-------------------------------------------------------------------------
//
//  Method:     COleSite::OnControlReadyStateChanged
//
//  Synopsis:   Handle readystate change notifications from control
//
//--------------------------------------------------------------------------

void
COleSite::OnControlReadyStateChanged(BOOL fForceComplete)
{
    long    lReadyState;
    long    lReadyStateOrig = _lReadyState;
    HRESULT hr;
    BOOL    fNativeSupport = FALSE;

    fNativeSupport = (THR_NOTRACE(GetReadyState(&lReadyState)) == S_OK);

    if (fForceComplete || (!fNativeSupport))
    {
        // If we have trouble getting the readystate, assume it's complete.
        lReadyState = READYSTATE_COMPLETE;
    }

    if (lReadyState == READYSTATE_COMPLETE)
    {
        if (_iRefreshTime > 0)
        {
            FormsSetTimer(this,
                          ONTICK_METHOD(COleSite, RefreshCallback, refreshcallback),
                          REFRESH_TIMER_ID,
                          _iRefreshTime);
            _iRefreshTime = 0; // in case we become READYSTATE_COMPLETE again
        }
    }

    TraceTagEx((tagOLEWatch,
        TAG_NONAME,
        "COleSite::OnControlReadyStateChange SSN=0x%x, _lreadystate=0x%x lreadystate=0x%x",
        _ulSSN,
        _lReadyState,
        lReadyState));

    // change by cfranks 8 May 1997:
    // We now allow controls that are in the complete state to transition
    // as far back as loaded, but not further back. (Example: data source
    // controls can have a new query run against them, in which case they
    // might go COMPLETE -> LOADED -> INTERACTIVE -> COMPLETE again.)
    if (_lReadyState == READYSTATE_COMPLETE && lReadyState < READYSTATE_LOADED)
    {
        return;                         // not further back than LOADED
    }

    if (_lReadyState != lReadyState ||
        _lReadyState == READYSTATE_UNINITIALIZED)
    {
        CDoc *  pDoc = Doc();
        IProgSink * pProgSink = IsInMarkup() ? pDoc->GetProgSink() : NULL;

        if (pProgSink)
        {
            if (lReadyState < READYSTATE_LOADED)
            {
                if (_dwProgCookie == 0)
                {
                    // Until we're in the loaded state, we need to add progress on
                    // the PROSINK_CLASS_CONTROL class.

                    IGNORE_HR(pProgSink->AddProgress(GetProgSinkClass(),
                        &_dwProgCookie));
                }
            }
            else if (lReadyState < READYSTATE_COMPLETE)
            {
                if (_lReadyState < READYSTATE_LOADED)
                {
                    DWORD dwProgCookie = _dwProgCookie;
                    _dwProgCookie = 0;

                    // Until we're in the complete state, we need to add progress
                    // on the PROGSINK_CLASS_OTHER class.

                    hr = THR(pProgSink->AddProgress(GetProgSinkClassOther(),
                            &_dwProgCookie));

                    if (hr)
                    {
                        _dwProgCookie = dwProgCookie;
                        goto Cleanup;
                    }

                    if (dwProgCookie)
                    {
                        pProgSink->DelProgress(dwProgCookie);
                    }
                }
            }
            else if (_dwProgCookie)
            {
                // We're all done.  Remove the progress from the document.

                pProgSink->DelProgress(_dwProgCookie);
                _dwProgCookie = 0;
            }
        }

        _lReadyState = lReadyState;

#ifndef NO_DATABINDING
        if (    lReadyStateOrig < READYSTATE_LOADED
            &&  lReadyState >= READYSTATE_LOADED
            &&  Tag() != ETAG_IFRAME   &&  Tag() != ETAG_FRAME)
        {
            // the control/applet can now respond to more questions;
            // try doing more databinding
            AttachDataBindings();
        }
#endif // ndef NO_DATABINDING

        if (    Tag() == ETAG_FRAME
            &&  _lReadyState >= LOADSTATUS_INTERACTIVE
            &&  GetMarkup()->LoadStatus() < LOADSTATUS_INTERACTIVE)
        {
            GetMarkup()->OnLoadStatus(LOADSTATUS_INTERACTIVE);
        }
    }

Cleanup:
    // If ready state has changed, fire the READYSTATECHANGE event on behalf
    // of the control (but only if it doesn't fire the event itself).
    if (_lReadyState != lReadyStateOrig)
    {
        if (_lReadyState == READYSTATE_COMPLETE)
            IGNORE_HR(Doc()->CommitScripts(this));

        if (!fNativeSupport)
        {
            OLESITE_TAG ostag = OlesiteTag();

            if (ostag == OSTAG_ACTIVEX || ostag == OSTAG_APPLET)
            {
                DYNCAST(CObjectElement, this)->Fire_onreadystatechange();
            }
        }
    }
}

//+------------------------------------------------------------------------
//
//  Function:   LooksLikePropertiesVerb
//
//  Synopsis:   Return true if the given string is likely to be the
//              properties verb.
//
//              We are forced to use this disgusting hack because we don't
//              know of a more reliable mechanism to determine if an object
//              has a properites verb.
//
//  Arguments:  pstrVerb    String to examine
//
//
//-------------------------------------------------------------------------
BOOL
LooksLikePropertiesVerb(TCHAR *pstrVerb)
{
    TCHAR achPropertiesVerb[FORMS_MAXVERBNAMLEN + 1];
    TCHAR *pch1 = pstrVerb;
    TCHAR *pch2 = achPropertiesVerb;
    TCHAR s_achNoise[] = _T("&.");

    if (!pch1)
    {
        return FALSE;
    }

    LoadString(GetResourceHInst(),
               IDS_PROPVERB,
               achPropertiesVerb,
               ARRAY_SIZE(achPropertiesVerb));

    for (;;)
    {
        // Skip over noise characters

        while (*pch1 && _tcschr(s_achNoise, *pch1))
            pch1 += 1;

        while (*pch2 && _tcschr(s_achNoise, *pch2))
            pch2 += 1;

        // CONSIDER: do comparison in lowercase.

        if (*pch1 != *pch2)
            return FALSE;

        if (*pch1 == 0)
            return TRUE;

        Assert(*pch1 != 0 && *pch2 != 0);

        pch1 += 1;
        pch2 += 1;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::DoEmbedVerbs, public
//
//  Synopsis:   Invokes a verb on an embedding when the user selects it from
//              the context menu.
//
//  Arguments:  [usVerbIndex] -- 0-based index of the verb as returned from
//                               the enumerator.
//
//----------------------------------------------------------------------------

void
COleSite::DoEmbedVerbs(USHORT usVerbIndex)
{
    USHORT        i          = 0;
    long          lVerb      = 0;
    IOleObject *  pObj       = NULL;
    IEnumOLEVERB *pEnumVerb  = NULL;
    HRESULT       hr;
    OLEVERB       verb;
    CRect         rc;

    // We should never have been able to get here if the number of selected
    // sites wasn't one.
    //
    COleLayout * pLayout = DYNCAST(COleLayout, GetCurLayout());
    Assert(pLayout);

    Assert(Doc()->GetSelectionType() == SELECTION_TYPE_Control );

    hr = THR_OLE(QueryControlInterface(IID_IOleObject, (LPVOID*)&pObj));
    if (hr)
        goto Cleanup;

    hr = THR_OLE(pObj->EnumVerbs(&pEnumVerb));
    if (hr)
        goto Cleanup;

    // Get the verb number (lVerb)

    while(pEnumVerb->Next(1, &verb, NULL) == S_OK)
    {
        CoTaskMemFree(verb.lpszVerbName);
        if ((verb.grfAttribs & OLEVERBATTRIB_ONCONTAINERMENU) &&
                verb.lVerb != OLEIVERB_PROPERTIES &&
                !LooksLikePropertiesVerb(verb.lpszVerbName))
        {
            if (i == usVerbIndex)
            {
                lVerb = verb.lVerb;
                break;
            }
            i++;
        }
    }

    Assert(i == usVerbIndex);

     // Set the host names just in case it's opening.
    IGNORE_HR(SetHostNames());

    Assert(Doc()->_pInPlace);

    // BUGBUG - Should we really be ignoring the error code from this call?
    //
    pLayout->GetClientRect(&rc, COORDSYS_GLOBAL);

    IGNORE_HR(pObj->DoVerb(lVerb,
                           NULL,
                           &_Client,
                           0,
                           Doc()->_pInPlace->_hwnd,
                           ENSUREOLERECT(&rc)));

Cleanup:
    ReleaseInterface(pObj);
    ReleaseInterface(pEnumVerb);
    return;
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::GetControlInfo, public
//
//  Synopsis:   Returns a filled in CONTROLINFO struct for this object.
//
//  Arguments:  [pci] -- Struct to fill in.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
COleSite::GetControlInfo(CONTROLINFO *pci)
{
    IOleControl *   pCtrl;
    HRESULT         hr;

    // Can't just do a memset because of pci->cb

    pci->hAccel = NULL;
    pci->cAccel = 0;
    pci->dwFlags = 0;

    hr = THR_OLE(QueryControlInterface(IID_IOleControl, (void **) &pCtrl));
    if (!hr)
    {
        hr = THR_OLE(pCtrl->GetControlInfo(pci));

        pCtrl->Release();

        if (pci->dwFlags & CTRLINFO_EATS_RETURN)
            _fEatsReturn = TRUE;
        else
            _fEatsReturn = FALSE;
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::OnInactiveMousePtrMessage
//
//  Synopsis:   Handle WM_SETCURSOR, WM_MOUSEMOVE on inactive control.
//
//----------------------------------------------------------------------------
HRESULT
COleSite::OnInactiveMousePtrMessage(CMessage *pMessage, CTreeNode * pNodeContext, LRESULT *plResult)
{
    HRESULT             hr;
    IPointerInactive *  pPI = NULL;
    DWORD               dwPolicy;
    RECT                rc;
    POINT               pt;

    COleLayout * pLayout = DYNCAST(COleLayout, GetCurLayout());
    Assert(pLayout);
    Assert( ! IsEditable() );
    
    if (THR_NOTRACE(QueryControlInterface(
                IID_IPointerInactive, (void **) &pPI)))
    {
        hr = THR(super::HandleMessage(pMessage));
        goto Cleanup;
    }

    hr = THR(pPI->GetActivationPolicy(&dwPolicy));
    if (hr)
        goto Cleanup;

    if (dwPolicy & POINTERINACTIVE_DEACTIVATEONLEAVE)
    {
        _fDeactivateOnMouseExit = TRUE;
    }

    if (dwPolicy & POINTERINACTIVE_ACTIVATEONENTRY)
    {
        hr = THR(TransitionTo(OS_INPLACE, NULL));
        if (hr)
            goto Cleanup;
    }

    // If the control became windowless inplace in TransitionTo, then
    // pass the message along to the control. Otherwise, use
    // IPointerInactive.
    if ( _fWindowlessInplace )
    {

        hr = THR(((IOleInPlaceObjectWindowless *)_pInPlaceObject)->
                OnWindowMessage(
                        pMessage->message,
                        pMessage->wParam,
                        pMessage->lParam,
                        plResult));
    }
    else
    {
        if (pMessage->message == WM_MOUSEMOVE)
        {
            CRect   rc;

            pLayout->GetClientRect(&rc, COORDSYS_GLOBAL);

            hr = THR_NOTRACE(pPI->OnInactiveMouseMove(
                                        &rc,
                                        MAKEPOINTS(pMessage->lParam).x,
                                        MAKEPOINTS(pMessage->lParam).y,
                                        pMessage->wParam));
        }
        else
        {
            GetCursorPos(&pt);
            ScreenToClient(Doc()->_pInPlace->_hwnd, &pt);
            // BUGBUG should last param be keystate?
            // See if the control wants to set the cursor
            hr = THR(pPI->OnInactiveSetCursor(
                &rc,
                pt.x,
                pt.y,
                HIWORD(pMessage->lParam),
                FALSE));

//
//BUGBUG: (ferhane) 
//          The spec says that we can only receive an S_FALSE, but ATL returns E_NOTIMPL for the
//          default case. We are accomodating the vast number of NT ATL controls here.
//
            if ( (hr == S_FALSE) || (hr == E_NOTIMPL) )
            {
                CTreeNode * pNodeParent = pNodeContext->GetUpdatedParentLayoutNode();
                Assert(pNodeParent);

                // Control did not set the cursor, give the container a chance
                // Call HandleMessage directly because we do not want bubbling here.
                hr = THR(pNodeParent->Element()->HandleMessage(pMessage));

                // The container did not set the cursor
                // Force the control to do it
                if (*plResult == FALSE)
                {
                    hr = THR(pPI->OnInactiveSetCursor(
                        &rc,
                        pt.x,
                        pt.y,
                        HIWORD(pMessage->lParam),
                        TRUE));
                    if (hr == S_FALSE)
                    {
                        hr = S_OK;
                    }
                }
            }
        }
    }

Cleanup:
    ReleaseInterface(pPI);
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::OnInactiveButtonMessage
//
//  Synopsis:   Handle WM_xBUTTONDOWN, WM_LBUTTONDBLCLK on inactive control.
//
//----------------------------------------------------------------------------

HRESULT
COleSite::OnInactiveMouseButtonMessage(CMessage *pMessage, LRESULT *plResult)
{
    HRESULT      hr = S_OK;
    int          iVerb;
    MSG          msg;
    UINT         message;
    IOleObject * pObj = NULL;
    CRect        rc;
    INSTANTCLASSINFO * pici;

    COleLayout * pLayout = DYNCAST(COleLayout, GetCurLayout());
    Assert(pLayout);
    Assert( ! IsEditable() );

    if (OK(THR_OLE(QueryControlInterface(IID_IOleObject, (void  **)&pObj))))
    {
        CDoc *  pDoc = Doc();

        message = pMessage->message;
        if (pMessage->message == WM_LBUTTONDBLCLK)
        {
            // Use 0 == default verb for double click.
            iVerb = 0;

            // Make double click look like single click.
            message = WM_LBUTTONDOWN;

            // Set the host names in case control opens in default verb.
            IGNORE_HR(SetHostNames());
        }
        else
        {
            //
            // Temp fix for IE5 bug 45562.  (anandra) Too risky to make this change
            // in full right now, should do this post beta2. BUGBUG 48041 has been
            // raided about this.  
            //

            if (pDoc->_fDesignMode &&
                (pDoc->_fNoActivateNormalOleControls ||
                 pDoc->_fNoActivateDesignTimeControls ||
                 pDoc->_fNoActivateJavaApplets))
            {
                // Only vid sets these bits so we should only come here for them.
                hr = S_OK;
                if (!NoUIActivate())
                {
                    hr = THR(TransitionTo(OS_UIACTIVE, pMessage));
                }
                goto Cleanup;
            }

            pici = GetInstantClassInfo();

            // inplace activate in all cases, except:
            // in design mode, controls which do not do menu merge
            iVerb = (pDoc->_fDesignMode && pici &&
                     !(OLEMISC_WANTSTOMENUMERGE & pici->dwMiscStatusFlags)) ?
                        OLEIVERB_UIACTIVATE :
                        OLEIVERB_INPLACEACTIVATE;
        }

        msg = *(MSG *)pMessage;
        msg.message = message;

        pLayout->GetClientRect(&rc, COORDSYS_GLOBAL);

        hr = THR_OLE(pObj->DoVerb(
                iVerb,
                &msg,
                &_Client,
                0,
                pDoc->_pInPlace->_hwnd,
                ENSUREOLERECT(&rc)));

        if (!hr && _state < OS_RUNNING)
        {
            _state = OS_RUNNING;
        }
    }

Cleanup:
    ReleaseInterface(pObj);
    RRETURN(hr);
}


#ifndef NO_MENU
//+------------------------------------------------------------------------
//
//  Member:     COleSite::InitMenuPopup
//
//  Synopsis:   Handles WM_CONTEXTMENU message after default handling occurs.
//
//-------------------------------------------------------------------------

HRESULT
COleSite::OnInitMenuPopup(HMENU hmenu, int item, BOOL fSystemMenu)
{
    IOleObject *    pObj = NULL;
    IEnumOLEVERB *  pEnumVerb = NULL;
    OLEVERB         verb;
    HRESULT         hr;
    UINT            i;
    ULONG           cFetched;
    CDoc *          pDoc = Doc();

    hr = super::OnInitMenuPopup(hmenu, item, fSystemMenu);
    if (hr)
        goto Cleanup;

    // Clean up the verbs on the context menu from last time

    if (pDoc->_usNumVerbs)
    {
        for (i=0; i < pDoc->_usNumVerbs; i++)
        {
            DeleteMenu(hmenu, IDM_OBJECTVERBLIST0+i, MF_BYCOMMAND);
        }

        pDoc->_usNumVerbs = 0;
    }

    //
    // If we're the only site selected, then add the verbs of the embedding
    // to the context menu.
    //

    if (pDoc->GetSelectionType() == SELECTION_TYPE_Control )
    {
        hr = THR_OLE(QueryControlInterface(IID_IOleObject, (LPVOID*)&pObj));
        if (hr)
            goto Cleanup;

        hr = THR_OLE(pObj->EnumVerbs(&pEnumVerb));
        if (hr)
            goto Cleanup;

        while (pEnumVerb->Next(1, &verb, &cFetched) == S_OK)
        {
            if (verb.grfAttribs & OLEVERBATTRIB_ONCONTAINERMENU)
            {
                // If it is the properties verb or it looks like the
                // properties verb, then add a menu item for our own
                // properites command.  This insures that our own
                // properties command will always be available, but
                // has the potential for hiding an object verb that just
                // happens to look like the property verb.

                if (verb.lVerb == OLEIVERB_PROPERTIES ||
                        LooksLikePropertiesVerb(verb.lpszVerbName))
                {
#ifdef NEVER // BUGBUG
                    fHasPropertiesVerb = TRUE;
                    AppendMenu(hmenu,
                           MF_STRING | MF_ENABLED,
                           IDM_PROPERTIES,
                           verb.lpszVerbName);
#endif
                }
                else
                {
                    AppendMenu(hmenu,
                               MF_STRING | verb.fuFlags,
                               IDM_OBJECTVERBLIST0+pDoc->_usNumVerbs,
                               verb.lpszVerbName);

                    pDoc->_usNumVerbs += 1;

                    if (pDoc->_usNumVerbs > (IDM_OBJECTVERBLISTLAST - IDM_OBJECTVERBLIST0 + 1))
                        break;
                }
            }

            CoTaskMemFree(verb.lpszVerbName);
        }
    }

#ifdef NEVER
    // Add a properties verb if object didn't have one or it's multi-select.

    if (!fHasPropertiesVerb)
    {
       AppendMenu(hMenu,
               MF_STRING | MF_ENABLED,
               IDM_PROPERTIES,
               g_szPropertiesVerb);
    }
#endif // NEVER

Cleanup:
    ReleaseInterface(pObj);
    ReleaseInterface(pEnumVerb);

    RRETURN(hr);
}
#endif // NO_MENU


//+-------------------------------------------------------------------------
//
//  Method:     COleSite::ReleaseCodeLoad
//
//  Synopsis:   Calculate the size of the object
//
//--------------------------------------------------------------------------

void
COleSite::ReleaseCodeLoad()
{
#ifndef WIN16
    if (_pCodeLoad)
    {
        CCodeLoad * pCodeLoad = _pCodeLoad;
        _pCodeLoad = NULL;
        pCodeLoad->Terminate();
        pCodeLoad->Release();
    }
#endif
}


//+-------------------------------------------------------------------------
//
//  Method:     COleSite::GetNaturalExtent
//
//  Synopsis:   Get the extent of the site.
//
//--------------------------------------------------------------------------

HRESULT
COleSite::GetNaturalExtent(DWORD dwMode, SIZEL *psizel)
{
    HRESULT hr;

    if (!_fUseViewObjectEx)
        return E_FAIL;

    DVEXTENTINFO extinfo;

    memset(&extinfo, 0, sizeof(extinfo));
    extinfo.cb = sizeof(DVEXTENTINFO);
    extinfo.sizelProposed = *psizel;
    extinfo.dwExtentMode = dwMode;

    hr = THR_OLE(((IViewObjectEx *)_pVO)->GetNaturalExtent(
            DVASPECT_CONTENT,
            -1,
            NULL,
            TLS(hdcDesktop),
            &extinfo,
            psizel));

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   COleSite::SizeToFit
//
//  Synopsis:   Do sizetoFit on.
//
//----------------------------------------------------------------------------

HRESULT
COleSite::SizeToFit()
{
    IOleObject *pObject = NULL;
    RECT        rc;
    HRESULT     hr = S_OK;
    SIZEL       sizel = { 0, 0 };

    COleLayout * pLayout = DYNCAST(COleLayout, GetCurLayout());
    Assert(pLayout);

    if (!_fUseViewObjectEx)
        goto Cleanup;

    DVEXTENTINFO extinfo;
    memset(&extinfo, 0, sizeof(extinfo));
    extinfo.cb = sizeof(extinfo);
    extinfo.dwExtentMode = DVEXTENT_CONTENT;
    extinfo.sizelProposed = sizel;

    hr = THR_OLE(((IViewObjectEx *)_pVO)->GetNaturalExtent(
            DVASPECT_CONTENT,
            -1,
            NULL,
            NULL,
            &extinfo,
            &sizel));
    if (hr)
        goto Cleanup;

    hr = THR_OLE(QueryControlInterface(IID_IOleObject, (void **)&pObject));
    if (hr)
        goto Cleanup;

    // Control must be at least running to set the extent.

    if (_state < OS_RUNNING)
    {
        IGNORE_HR(TransitionTo(OS_RUNNING));
    }

    // The CDK implementation of IOleObject::SetExtent calls
    // IOleInplaceSite::OnPosRectChange with the old position
    // of the control.  We note that we are setting the extent
    // in order to give OnPosRectChange a reasonable answer.

    pLayout->_sizelLast = sizel;

    {
        CLock   Lock(this, OLESITELOCK_SETEXTENT);

        hr = THR_OLE(pObject->SetExtent(DVASPECT_CONTENT, &sizel));
    }

    pLayout->GetPosition((CPoint *)&rc);
    rc.right    = -1;
    rc.bottom   = -1;

    hr = THR(pLayout->Move(&rc, 0));

Cleanup:
    ReleaseInterface(pObject);

    // Ignore errors if the object does not want to resize.
    return S_OK;
}

//+-------------------------------------------------------------------
//
// Member :   COleSite::QueryStatus
//
// Synopsis :
//
//--------------------------------------------------------------------

HRESULT COleSite::QueryStatus(
        GUID * pguidCmdGroup,
        ULONG cCmds,
        MSOCMD rgCmds[],
        MSOCMDTEXT * pcmdtext)
{
    Assert(IsCmdGroupSupported(pguidCmdGroup));
    Assert(cCmds == 1);

    MSOCMD * pCmd = NULL;
    HRESULT hr = S_OK;
    UINT        uPropName;
    VARTYPE     vt = VT_EMPTY;
    int         c;
    int         idm;
    GUID *      pguidControl;
    ULONG       ulCmdID;

    #if 0
            if (InSendMessage())
            {
                ReplyMessage(0);
            }
    #endif

    for (pCmd = rgCmds, c = cCmds; --c >= 0; pCmd++)
    {
        Assert(!pCmd->cmdf);

        // Disable Office documents in frameset from showing/hiding toolbars.
        if (pguidCmdGroup == NULL && pCmd->cmdID == OLECMDID_HIDETOOLBARS)
        {
            pCmd->cmdID = MSOCMDSTATE_DISABLED;
            continue;
        }

        idm = IDMFromCmdID(pguidCmdGroup, pCmd->cmdID);

        uPropName = 0;

        switch (idm)
        {
        case IDM_OBJECTVERBLIST0:
            pCmd->cmdf = MSOCMDSTATE_UP;
            break;

        case IDM_FONTNAME:
            uPropName = IDS_DISPID_FONTNAME;
            vt = VT_BSTR;
            break;

        case IDM_FONTSIZE:
            uPropName = IDS_DISPID_FONTSIZE;
            vt = VT_CY;
            break;

        case IDM_SUPERSCRIPT:
            uPropName = IDS_DISPID_FONTSUPERSCRIPT;
            vt = VT_BOOL;
            break;

        case IDM_SUBSCRIPT:
            uPropName = IDS_DISPID_FONTSUBSCRIPT;
            vt = VT_BOOL;
            break;

        case IDM_BOLD:
            uPropName = IDS_DISPID_FONTBOLD;
            vt = VT_BOOL;
            break;

        case IDM_ITALIC:
            uPropName = IDS_DISPID_FONTITAL;
            vt = VT_BOOL;
            break;

        case IDM_UNDERLINE:
            uPropName = IDS_DISPID_FONTUNDER;
            vt = VT_BOOL;
            break;

        case IDM_BACKCOLOR:
            uPropName = IDS_DISPID_BACKCOLOR;
            vt = VT_I4;
            break;

        case IDM_FORECOLOR:
            {
            // BUGBUG (jenlc)
            // this is just a transition code, will be changed later for
            // QueryStatus/Exec architecture rework.
            //
            CVariant varargOut;
            DISPID     dispidProp;
            HRESULT    hr;

            dispidProp = DISPID_A_COLOR;
            vt         = VT_I4;
            V_VT(&varargOut) = VT_I4;
            hr = THR_NOTRACE(ExecSetGetKnownProp(NULL, &varargOut, dispidProp, vt));
            pCmd->cmdf = (hr) ? (MSOCMDSTATE_DISABLED) : (MSOCMDSTATE_UP);
            break;
            }

        case IDM_BORDERCOLOR:
            uPropName = IDS_DISPID_BORDERCOLOR;
            vt = VT_I4;
            break;

        case IDM_BORDERSTYLEFIXED:
        case IDM_BORDERSTYLENO:
            uPropName = IDS_DISPID_BORDERSTYLE;
            vt = VT_I4;
            break;

        case IDM_JUSTIFYLEFT:
        case IDM_JUSTIFYCENTER:
        case IDM_JUSTIFYRIGHT:
        case IDM_JUSTIFYGENERAL:
        case IDM_JUSTIFYFULL:
            uPropName = IDS_DISPID_TEXTALIGN;
            vt = VT_I4;
            break;

        case IDM_FLAT:
        case IDM_RAISED:
        case IDM_SUNKEN:
            uPropName = IDS_DISPID_SPECIALEFFECT;
            vt = VT_I4;
            break;

        default:
            //
            // Do a reverse lookup to try and match into the standard cmd set.
            //

            if (OLECMDIDFromIDM(idm, &ulCmdID))
            {
                pguidControl = NULL;
                pCmd->cmdID = ulCmdID;
            }
            else
            {
                pguidControl = pguidCmdGroup;
            }

            if (!pguidControl ||
                (pguidControl == (GUID *)&CGID_MSHTML &&
                    (Tag() == ETAG_FRAME || Tag() == ETAG_IFRAME)))
            {
                // Only call control if not locked and the command guid
                // is the standard one.  (Forms3 specific commands are
                // never passed down.)
                //

// WINCEREVIEW - cut call to InSendMessage
#ifndef WINCE
                if (_pUnkCtrl && !CElement::TestLock(ELEMENTLOCK_QUERYSTATUS) && !InSendMessage())
                {
                    CElement::CLock Lock(this, ELEMENTLOCK_QUERYSTATUS);

                    hr = THR_OLE(CTQueryStatus(
                            _pUnkCtrl,
                            pguidControl,
                            1,
                            pCmd,
                            pcmdtext));
                }
                else
#endif // WINCE
                {
                    pCmd->cmdf = MSOCMDSTATE_DISABLED;
                }
                continue;
           }

        }

        if(uPropName)
        {
            Assert(vt != VT_EMPTY);
            hr = THR_NOTRACE(QueryStatusProperty(pCmd, uPropName, vt));
        }

        if ((hr == S_OK || hr == OLECMDERR_E_NOTSUPPORTED) && !pCmd->cmdf)
        {
            hr = THR_NOTRACE(super::QueryStatus(pguidCmdGroup, 1, pCmd, pcmdtext));
        }

    }

    RRETURN_NOTRACE(hr);
}

//+-------------------------------------------------------------------
//
// Member :   COleSite::Exec
//
// Synopsis :
//
//--------------------------------------------------------------------

HRESULT
COleSite::Exec(
        GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut)
{
    Assert(CBase::IsCmdGroupSupported(pguidCmdGroup));

    HRESULT         hr = OLECMDERR_E_NOTSUPPORTED;
    UINT            uPropName;
    VARTYPE         vt = VT_EMPTY;
    DWORD           dwValue;
    int             idm;
    DISPID          dispidProp;
    CParentUndoUnit *pCPUU = NULL;
    GUID *          pguidControl;
    ULONG           ulCmdID;

    // Disable Office documents in frameset from showing/hiding toolbars.
    if (pguidCmdGroup == NULL && nCmdID == OLECMDID_HIDETOOLBARS)
    {
        RRETURN(OLECMDERR_E_DISABLED);
    }

    idm = IDMFromCmdID(pguidCmdGroup, nCmdID);

    // If the idm represents an undoable property change, open
    // a parent undo unit.

#ifndef NO_EDIT
    if (idm != IDM_UNDO && idm != IDM_REDO)
    {
        pCPUU = OpenParentUnit(this, IDS_UNDOPROPCHANGE);
    }
#endif // NO_EDIT
    switch (idm)
    {

    case IDM_SIZETOFIT:
        hr = THR(SizeToFit());
        break;

    case IDM_OBJECTVERBLIST0:
        break;

    case IDM_FONTNAME:
        uPropName = IDS_DISPID_FONTNAME;
        vt = VT_BSTR;
        goto ExecSetGetProperty;

    case IDM_FONTSIZE:
        {
            CVariant varTemp;
            CY      cy;

             V_VT(&varTemp) = VT_CY;
            // Need to do convert from long (twips) to CURRENCY for font size
            if (pvarargIn)
            {
                cy.Lo = V_I4(pvarargIn)/20 * 10000;
                cy.Hi = 0;
                V_CY(&varTemp) = cy;
            }

            hr = THR_NOTRACE(ExecSetGetProperty(
                    pvarargIn ? &varTemp: NULL,
                    pvarargOut ? &varTemp : NULL ,
                    IDS_DISPID_FONTSIZE,
                    VT_CY));

            if (!hr && pvarargOut)
            {
                V_VT(pvarargOut) = VT_I4;
                cy = V_CY(&varTemp);
                V_I4(pvarargOut) = cy.Lo /10000*20;
            }
            goto Cleanup;
        }

    case IDM_SUPERSCRIPT:
        uPropName = IDS_DISPID_FONTSUPERSCRIPT;
        goto ExecToggleCmd;

    case IDM_SUBSCRIPT:
        uPropName = IDS_DISPID_FONTSUBSCRIPT;
        goto ExecToggleCmd;

    case IDM_BOLD:
        uPropName = IDS_DISPID_FONTBOLD;
        goto ExecToggleCmd;

    case IDM_ITALIC:
        uPropName = IDS_DISPID_FONTITAL;
        goto ExecToggleCmd;

    case IDM_UNDERLINE:
        uPropName = IDS_DISPID_FONTUNDER;
        goto ExecToggleCmd;

    case IDM_BACKCOLOR:
        uPropName = IDS_DISPID_BACKCOLOR;
        vt = VT_I4;
        goto ExecSetGetProperty;

    case IDM_FORECOLOR:
        dispidProp = DISPID_A_COLOR;
        // for this color we need to swap
        if (pvarargIn)
        {
            CColorValue cvValue;
            CVariant varColor;

            hr = THR(varColor.CoerceVariantArg(pvarargIn, VT_I4));
            if (hr)
                goto Cleanup;

            cvValue.SetFromRGB(V_I4(&varColor));

            V_I4(pvarargIn) = (DWORD)cvValue.GetRawValue();
            V_VT(pvarargIn) = VT_I4;
        }
        vt = VT_I4;
        goto ExecKnownDispidProperty;

    case IDM_BORDERCOLOR:
        uPropName = IDS_DISPID_BORDERCOLOR;
        vt = VT_I4;
        goto ExecSetGetProperty;

    case IDM_BORDERSTYLEFIXED:
        uPropName = IDS_DISPID_BORDERSTYLE;
        dwValue = fmBorderStyleSingle;
        goto ExecSetPropertyCmd;

    case IDM_BORDERSTYLENO:
        uPropName = IDS_DISPID_BORDERSTYLE;
        dwValue = fmBorderStyleNone;
        goto ExecSetPropertyCmd;

    case IDM_RAISED:
        uPropName = IDS_DISPID_SPECIALEFFECT;
        dwValue = fmBorderStyleRaised;
        goto ExecSetPropertyCmd;

    case IDM_SUNKEN:
        uPropName = IDS_DISPID_SPECIALEFFECT;
        dwValue = fmBorderStyleSunken;
        goto ExecSetPropertyCmd;

    case IDM_GETPUNKCONTROL:
        if (pvarargOut)
        {
            V_VT(pvarargOut) = VT_UNKNOWN;
            pvarargOut->punkVal = _pUnkCtrl;
            hr = S_OK;
        }
        goto Cleanup;

    default:
        //
        // Do a reverse lookup to try and match into the standard cmd set.
        //

        if (OLECMDIDFromIDM(idm, &ulCmdID))
        {
            pguidControl = NULL;
        }
        else
        {
            pguidControl = pguidCmdGroup;
            ulCmdID = nCmdID;
        }


        if (_pUnkCtrl && 
            (!pguidCmdGroup ||
                (pguidCmdGroup == (GUID *)&CGID_MSHTML &&
                    (Tag() == ETAG_FRAME || Tag() == ETAG_IFRAME))))
        {
            OPTIONSETTINGS *pos = Doc()->_pOptionSettings;
            
            if ( !Doc()->IsInScript() || _pDisp && AccessAllowed(_pDisp))
            {
                hr = THR_OLE(CTExec(
                        _pUnkCtrl,
                        pguidControl,
                        ulCmdID,
                        nCmdexecopt,
                        pvarargIn,
                        pvarargOut));
            }
            else if (_pDisp && !AccessAllowed(_pDisp) )
            {
                // It's coming from script, but not allowed access.
                // Allow access based on specific options setting.
                if (pos && pos->fAllowCutCopyPaste &&
                        (idm == IDM_PASTE || idm == IDM_CUT || idm == IDM_COPY))
                {
                    hr = THR_OLE(CTExec(
                            _pUnkCtrl,
                            pguidControl,
                            ulCmdID,
                            nCmdexecopt,
                            pvarargIn,
                            pvarargOut));
                }
                else
                {
                    hr = E_ACCESSDENIED;
                    goto Cleanup;
                }
            }
            // else...pass through when _pDisp is NULL
        }
        break;
    }

    // When user cancel saveas, Excel returns OLECMDERR_E_NOTSUPPORTED
    if (idm == IDM_SAVEAS && hr == OLECMDERR_E_NOTSUPPORTED)
    {
        hr = S_OK;
    }

    // Some controls return DISP_E_MEMBERNOTFOUND even though they should return OLECMDERR_E_NOTSUPPORTED
    // instead.  This safety valve catches those cases:    -Tom
    if( DISPID_NOT_FOUND(hr) || hr == E_NOINTERFACE ||
            hr == OLE_E_NOCONNECTION || hr == OLECMDERR_E_UNKNOWNGROUP)
    {
        hr = OLECMDERR_E_NOTSUPPORTED;
    }

    if (OLECMDERR_E_NOTSUPPORTED == hr)
        hr = THR_NOTRACE(super::Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn,
                pvarargOut));
    goto Cleanup;

ExecKnownDispidProperty:
    Assert(vt != VT_EMPTY);
    hr = THR_NOTRACE(ExecSetGetKnownProp(pvarargIn,pvarargOut,dispidProp, vt));
    goto Cleanup;

ExecSetGetProperty:
    Assert(vt != VT_EMPTY);
    hr = THR_NOTRACE(ExecSetGetProperty(pvarargIn,pvarargOut,uPropName,vt));
    goto Cleanup;

ExecToggleCmd:
    hr = THR_NOTRACE(ExecToggleCmd(uPropName));
    goto Cleanup;

ExecSetPropertyCmd:
    hr = THR_NOTRACE(ExecSetPropertyCmd(uPropName, dwValue));
    goto Cleanup;

Cleanup:
#ifndef NO_EDIT
    CloseParentUnit(pCPUU, hr);
#endif // NO_EDIT
    RRETURN_NOTRACE(hr);
}





BOOL
COleSite::CheckDisplayAsDefault(CElement * pElem)
{
     return (pElem && pElem->TestClassFlag(ELEMENTDESC_OLESITE) &&
      (DYNCAST(COleSite, pElem)->_fActsLikeButton ||
       DYNCAST(COleSite, pElem)->_fEatsReturn));
}
//+---------------------------------------------------------------------------
//
//  Member:     COleSite::EnsureParamBag()
//
//  Synopsis:   creates param bag if not yet
//
//----------------------------------------------------------------------------

HRESULT
COleSite::EnsureParamBag()
{
    if (!_pParamBag)
    {
        _pParamBag = new CPropertyBag();
        if (!_pParamBag)
            RRETURN (E_OUTOFMEMORY);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::ReleaseParamBag()
//
//  Synopsis:   releases param bag if not yet; sets _pParamBag to NULL
//
//----------------------------------------------------------------------------

HRESULT
COleSite::ReleaseParamBag()
{
    if (_pParamBag)
    {
        _pParamBag->Release();
        _pParamBag = NULL;
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::ExchangeParamBag()
//
//  Synopsis:   exchanges properties between control and param bag using
//              IPersistProperyBag interface
//
//  Parameters  dir:    if (dir == UPLOADPARAMBAG)
//                          uploads parameters from param bag of CObjectElement
//                          to control maintained in this site
//                      else if (dir == DOWNLOADPARAMBAG)
//                          downloads parameters to param bag of CObjectElement
//                          from control maintained in this site
//
//
//----------------------------------------------------------------------------

HRESULT
COleSite::ExchangeParamBag(ExchangeParamBagDir dir)
{
    HRESULT                 hr = S_OK;
    IPersistPropertyBag *   pPPBag = NULL;
    IPersistPropertyBag2 *  pPPBag2 = NULL;

    if (QueryControlInterface(IID_IPersistPropertyBag2, (void **)&pPPBag2))
    {
        hr = QueryControlInterface(IID_IPersistPropertyBag, (void**)&pPPBag);
        if (hr)
            goto Cleanup;
    }

    Assert(pPPBag || pPPBag2);

    switch (dir)
    {
    case TOCONTROL:
        Assert(_pParamBag);
        hr = (pPPBag2) ?
                THR_OLE(pPPBag2->Load(_pParamBag, NULL)) :
                THR_OLE(pPPBag->Load(_pParamBag, NULL));
        break;

    case FROMCONTROL:
        hr = EnsureParamBag();
        if (hr)
            goto Cleanup;

        hr = (pPPBag2) ?
                THR_OLE(pPPBag2->Save(_pParamBag, TRUE, TRUE)) :
                THR_OLE(pPPBag->Save(_pParamBag, TRUE, TRUE));
        break;

    default:
        Assert (0 && "invalid argument");
    }

Cleanup:
    ReleaseInterface(pPPBag);
    ReleaseInterface(pPPBag2);
    RRETURN (hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::IsOleProxy
//
//  Returns:    BOOL      True if _pUnkCtrl is actually an OLE proxy.
//
//  Notes:      It performs this check by QI'ing for IClientSecurity.
//              Typically only ole proxies implement this interface.  An
//              arbitrary ocx has no use for it.
//
//----------------------------------------------------------------------------

BOOL
COleSite::IsOleProxy()
{
    IClientSecurity *   pCL;
    BOOL                fRet = FALSE;

#ifndef WIN16       //BUGWIN16 deal with this when do code download
    if (OK(QueryControlInterface(IID_IClientSecurity, (void **)&pCL)))
    {
        ReleaseInterface(pCL);
        // Only proxy objects should support this interface.
        fRet = TRUE;
    }
#endif

    return fRet;
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::QuerySafeLoadInterface
//
//  Returns:    HRESULT     E_NOINTERFACE if not verifiably safe to initialize via
//                          the requested interface.  Otherwise, the result
//                          returned from QueryInterface.
//
//  Notes:      Queries for the interface, then determines safety of initialization
//              via that interface.  If not verifiable safe, clears the interface
//              and returns E_NOINTERFACE even if the control implements it.
//
//----------------------------------------------------------------------------

HRESULT
COleSite::QuerySafeLoadInterface(REFIID riid, void ** ppOut)
{
    HRESULT hr = THR_NOTRACE(QueryControlInterface(riid, ppOut));

    if (OK(hr) && *ppOut)
    {
        if (!IsSafeToInitialize(riid))
        {
            ClearInterface((IUnknown **)ppOut);
            hr = S_FALSE;
            goto Cleanup;
        }
        else
        {

            //  Loading data into a control on a print doc is always safe since the
            //  data was saved by this control and wouldn't have been loaded originally
            //  unless it was safe to do so.
            if (Doc()->IsPrintDoc())
            {
                goto Cleanup;
            }    
            else if ( riid == IID_IPersistMoniker ||
                      riid == IID_IPersistStream ||
                      riid == IID_IPersistStreamInit)
            {
                //  For certain interfaces, deny access to x-domain data if the user
                //  has not explicitly allowed it.
            
                CLSID * pCLSID = GetpCLSID();
                
                if (!pCLSID || (!_fDataSameDomain && !IsEqualCLSID(CLSID_Scriptlet, *pCLSID)))
                {
                    BOOL fAllowXDomain = FALSE;
                    THR(Doc()->ProcessURLAction(
                        URLACTION_CROSS_DOMAIN_DATA,
                        &fAllowXDomain));

                    // if user has chosen not to allow x-domain data access, honor that.
                    if (!fAllowXDomain)
                    {
                        ClearInterface((IUnknown **)ppOut);
                        hr = S_FALSE;
                        goto Cleanup;
                    }
                }
            }
        }
    }

Cleanup:
    return hr;
}


HRESULT
COleSite::GetAmbientProp(DISPID dispid, VARIANT *pvarResult)
{
    Assert(pvarResult);

    HRESULT hr = S_OK;
    BOOL    fBoolValue;

    switch (dispid)
    {
        case DISPID_AMBIENT_BACKCOLOR:
            {
                OLE_COLOR backColor = (OLE_COLOR)(GetInheritedBackgroundColor() & 0x00ffffff);
                V_VT(pvarResult) = VT_I4;
                V_I4(pvarResult) = backColor;
            }
            break;

        case DISPID_AMBIENT_FORECOLOR:
            {
                CColorValue ccvTextColor;
                OLE_COLOR foreColor;

                if (GetFirstBranch())
                    ccvTextColor = GetFirstBranch()->GetCascadedcolor();

                if (ccvTextColor.IsDefined())
                {
                    // If ForeColor is available from character formatting info...
                    foreColor = ccvTextColor.GetOleColor();
                }
                else
                {
                    // otherwise get ForeColor from document settings.
                    foreColor = Doc()->_pOptionSettings->colorText;
                }

                V_VT(pvarResult) = VT_I4;
                V_I4(pvarResult) = foreColor;
            }
            break;

        case DISPID_AMBIENT_FONT:
            {
                const CCharFormat *pCF = GetFirstBranch() ? GetFirstBranch()->GetCharFormat() : NULL;
                LPDISPATCH pFontDisp = NULL;

                if (pCF)
                {
                    FONTDESC fontDesc;

                    CStr sFaceName;
                    sFaceName.Set(pCF->GetFaceName());

                    fontDesc.cbSizeofstruct = sizeof (FONTDESC);
                    fontDesc.lpstrName = sFaceName;
                    fontDesc.cySize.Hi = 0;
                    fontDesc.cySize.Lo = MulDivQuick(pCF->GetHeightInTwips(Doc()), 10000, TWIPS_PER_POINT);
                    fontDesc.sWeight = pCF->_wWeight;
                    fontDesc.sCharset = pCF->_bCharSet;
                    fontDesc.fItalic = pCF->_fItalic;
                    fontDesc.fUnderline = pCF->_fUnderline;
                    fontDesc.fStrikethrough = pCF->_fStrikeOut;
                    hr = THR(OleCreateFontIndirect(&fontDesc, IID_IFontDisp, (void **)&pFontDisp));
                }

                V_VT(pvarResult) = VT_DISPATCH;
                if (OK(hr) && pFontDisp)
                    V_DISPATCH(pvarResult) = pFontDisp;  // Consumer must Release...
                else
                    V_DISPATCH(pvarResult) = NULL;
            }
            break;

        case DISPID_AMBIENT_DISPLAYNAME:
            V_VT(pvarResult) = VT_BSTR;
            hr = THR(FormsAllocString(GetIdentifier(), &V_BSTR(pvarResult)));
            break;

        case DISPID_AMBIENT_UIDEAD:
            fBoolValue = FALSE;
            goto StoreBool;

        case DISPID_AMBIENT_USERMODE:
            fBoolValue = !Doc()->_fDesignMode;
            goto StoreBool;

        case DISPID_AMBIENT_SHOWGRABHANDLES:
        case DISPID_AMBIENT_SHOWHATCHING:
            fBoolValue = FALSE; // Never allow clients to show hatching
            goto StoreBool;

        case DISPID_AMBIENT_LOCALEID:
            {
                V_VT(pvarResult) = VT_I4;
                const CCharFormat *pCF = GetFirstBranch() ? GetFirstBranch()->GetCharFormat() : NULL;
                if (pCF)
                {
                    V_I4(pvarResult) = pCF->_lcid;
                }
                else
                {
                    V_I4(pvarResult) = g_lcidUserDefault;
                }
            }
            break;

#ifndef WIN16
            // BUGBUG michaelw - temporary fix until ambient palette is implemented by CDoc
        case DISPID_AMBIENT_PALETTE:
            V_VT(pvarResult) = VT_HANDLE;
            V_I4(pvarResult) = HandleToLong(Doc()->GetPalette());
            break;
#endif //!WIN16

        case DISPID_AMBIENT_MESSAGEREFLECT:
        case DISPID_AMBIENT_SUPPORTSMNEMONICS:
            fBoolValue = TRUE;
            goto StoreBool;

        case DISPID_AMBIENT_DISPLAYASDEFAULT:
            {
                CElement * pElemCurrent = Doc()->_pElemCurrent;

                if (pElemCurrent == this)
                {
                    fBoolValue = TRUE;
                }
                else if (_fDefault &&
                        !CheckDisplayAsDefault(pElemCurrent))
                {
                    fBoolValue = TRUE;
                }
                else
                {
                    fBoolValue = FALSE;
                }
            }
            goto StoreBool;

        case DISPID_AMBIENT_OFFLINEIFNOTCONNECTED :
            fBoolValue = ((Doc()->_dwLoadf & DLCTL_OFFLINEIFNOTCONNECTED) ? TRUE : FALSE);
            goto StoreBool;

        case DISPID_AMBIENT_SILENT :
            fBoolValue = ((Doc()->_dwLoadf & DLCTL_SILENT) ? TRUE : FALSE);
            goto StoreBool;

        case DISPID_AMBIENT_USERAGENT:
            V_VT(pvarResult) = VT_BSTR;
            hr = THR(FormsAllocString(Doc()->_bstrUserAgent, &V_BSTR(pvarResult)));
            break;

        case DISPID_AMBIENT_DLCONTROL:
            V_VT(pvarResult) = VT_I4;
            V_I4(pvarResult) = Doc()->_dwLoadf;
            break;

        case DISPID_AMBIENT_RIGHTTOLEFT:
            hr = THR(Doc()->GetDocDirection(&fBoolValue));
            fBoolValue = ENSURE_BOOL(fBoolValue);
            goto StoreBool;

StoreBool:
            V_VT(pvarResult) = VT_BOOL;
            V_BOOL(pvarResult) = (VARIANT_BOOL)-fBoolValue;
            break;

            // BUGWIN16: enable the following.
#ifndef WIN16
        case DISPID_AMBIENT_CODEPAGE:
            V_VT(pvarResult) = VT_UI4;
            V_UI4(pvarResult) = Doc()->GetCodePage();
            break;

        case DISPID_AMBIENT_CHARSET:
            {
                TCHAR tszCharset[MAX_MIMECSET_NAME] = {0};
                V_VT(pvarResult) = VT_BSTR;
                IGNORE_HR(GetMlangStringFromCodePage(Doc()->GetCodePage(), tszCharset,
                                                     ARRAY_SIZE(tszCharset)));
                hr = THR(FormsAllocString(tszCharset, &V_BSTR(pvarResult)));
            }
            break;
#endif

        default:
            hr = THR_NOTRACE(Doc()->GetAmbientVariant(dispid, pvarResult));
            break;

    }   // switch

    RRETURN_NOTRACE(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::OlesiteTag
//
//  Synopsis:   Return type of thingy that's really running in this site
//
//----------------------------------------------------------------------------

COleSite::OLESITE_TAG
COleSite::OlesiteTag()
{
    OLESITE_TAG ostag;
    INSTANTCLASSINFO * pici;

    // BUGBUG For now just look at my tag.  Replace this with a more sophisticated
    // test that drills down to the thing that's really running.
    switch (Tag())
    {
    case ETAG_OBJECT:
        pici = GetInstantClassInfo();

        if (!pici)
        {
            ostag = OSTAG_UNKNOWN;
        }
        else if (IsEqualGUID((REFGUID)pici->clsid, (REFGUID)CLSID_AppletOCX))
        {
           ostag = OSTAG_APPLET;
        }
        else
        {
           ostag = OSTAG_ACTIVEX;
        }
        break;

    case ETAG_APPLET:
        ostag = OSTAG_APPLET;
        break;

    case ETAG_EMBED:
        ostag = OSTAG_PLUGIN;
        break;

    case ETAG_FRAME:
        ostag = OSTAG_FRAME;
        break;

    case ETAG_IFRAME:
        ostag = OSTAG_IFRAME;
        break;

    default:
        AssertSz(0, "Unknown tag");
        ostag = OSTAG_UNKNOWN;
        break;
    }

    return ostag;
}

//+---------------------------------------------------------------------------
//
//  Member:     COleSite::AfterObjectInstantiation
//
//  Synopsis:   an opportunity to do something after the object is
//              instantiated but before the document is created
//
//----------------------------------------------------------------------------

void COleSite::AfterObjectInstantiation()
{
    // nothing here but subclasses might want to do something...
}


//+------------------------------------------------------------------------
//
//  Member:     COleSite::RefreshCallback(UINT uTimerID)
//
//  Synopsis:   refresh our control which can only be a CDoc by
//              potentially bashing it's _cstrUrl
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT BUGCALL
COleSite::RefreshCallback(UINT uTimerID)
{
    HRESULT hr = S_OK;
    FormsKillTimer(this, REFRESH_TIMER_ID);
    DWORD dwRefreshFlags = OLECMDIDF_REFRESH_NO_CACHE;

    CDoc *pDoc = (CDoc*)(void*)_pUnkCtrl;
    Assert( (*(void**)pDoc) == *(void**)Doc()); // This *should* always be a CDoc.
    if (_pstrRefreshURL)
    {
        dwRefreshFlags |= OLECMDIDF_REFRESH_CLEARUSERINPUT;
        hr = THR(pDoc->_cstrUrl.Set(_pstrRefreshURL));
        // _pstrRefreshURL must be NULL before we ExecRefresh
        // since the ExecRefresh may free the string
        FormsFreeString(_pstrRefreshURL);
        _pstrRefreshURL = NULL;
        if (hr)
            goto Cleanup;
        
        hr = THR(pDoc->_cstrCOMPAT_OMUrl.Set(_T("")));
        if (hr)
            goto Cleanup;
    }
    pDoc->ExecRefresh(dwRefreshFlags);

Cleanup:
    return hr;
}

HRESULT COleSite::OnFailToCreate()
{
    _fFailedToCreate = TRUE;
    if (GetCurLayout())
        GetCurLayout()->_fNoUIActivateInDesign = TRUE;
    return S_OK;
}

void
COleSite::DeferredCreateObject( DWORD_PTR dwContext )
{
    ELEMENT_TAG etag = Tag();

    if ((etag == ETAG_APPLET) || (etag == ETAG_OBJECT))
    {
        IGNORE_HR((DYNCAST(CObjectElement, this))->CreateObject());
    }
    else if (etag == ETAG_EMBED)
    {
        IGNORE_HR((DYNCAST(CPluginSite, this))->CreateObject());
    }
}

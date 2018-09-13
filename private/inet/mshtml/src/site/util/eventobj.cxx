//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       eventobj.cxx
//
//  Contents:   Implementation of CEventObject class
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_EVENTOBJ_HXX_
#define X_EVENTOBJ_HXX_
#include "eventobj.hxx"
#endif

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X_FATSTG_HXX_
#define X_FATSTG_HXX_
#include "fatstg.hxx"
#endif

#ifndef X_WINBASE_H_
#define X_WINBASE_H_
#include "winbase.h"
#endif

#ifndef X_UNICWRAP_HXX_
#define X_UNICWRAP_HXX_
#include "unicwrap.hxx"
#endif

#ifndef X_BINDER_HXX_
#define X_BINDER_HXX_
#include <binder.hxx>       // for CDataSourceProvider
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_SHELL_H_
#define X_SHELL_H_
#include <shell.h>
#endif

#ifndef X_SHLOBJP_H_
#define X_SHLOBJP_H_
#include <shlobjp.h>
#endif

#ifndef X_IMGELEM_HXX_
#define X_IMGELEM_HXX_
#include "imgelem.hxx"
#endif

#ifndef X_EMAP_HXX_
#define X_EMAP_HXX_
#include "emap.hxx"
#endif

#ifndef X_EAREA_HXX_
#define X_EAREA_HXX_
#include "earea.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif


#define _cxx_
#include "eventobj.hdl"

MtDefine(CEventObj, ObjectModel, "CEventObj")
MtDefine(BldEventObjElemsCol, PerfPigs, "Build CEventObj::EVENT_BOUND_ELEMENTS_COLLECTION")

//=======================================================================
//
//  #defines  -- this section contains a nubmer of pound defines of functions that
//    are repreated throughout this file.  there are not quite enough to warrent
//    a functional re-write, but enough to make the redundant code difficult to
//    read.
//
//=======================================================================

//=======================================================================

#define MAKESUREPUTSAREALLOWED          \
    if(!_fReadWrite)                    \
    {                                   \
        hr = DISP_E_MEMBERNOTFOUND;     \
        goto Cleanup;                   \
    }


#define GETKEYVALUE(fnName,  MASK)   \
    HRESULT CEventObj::get_##fnName (VARIANT_BOOL *pfAlt)   \
{                                                       \
    HRESULT         hr;                                 \
    EVENTPARAM *    pparam;                             \
                                                        \
    hr = THR(GetParam(&pparam));                        \
    if (hr)                                             \
        goto Cleanup;                                   \
                                                        \
    *pfAlt = VARIANT_BOOL_FROM_BOOL(pparam->_sKeyState & ##MASK); \
                                                        \
Cleanup:                                                \
    RRETURN(_pDoc->SetErrorInfo(hr));                   \
}                                                       \

#define PUTKEYVALUE(fnName,  MASK)                      \
    HRESULT CEventObj::put_##fnName (VARIANT_BOOL fPressed)   \
{                                                       \
    HRESULT         hr;                                 \
    EVENTPARAM *    pparam;                             \
                                                        \
    MAKESUREPUTSAREALLOWED                              \
                                                        \
    hr = THR(GetParam(&pparam));                        \
    if (hr)                                             \
        goto Cleanup;                                   \
                                                        \
    if(fPressed)                                        \
        pparam->_sKeyState |= ##MASK;                    \
    else                                                \
        pparam->_sKeyState &= ~##MASK;                   \
                                                        \
                                                        \
Cleanup:                                                \
    RRETURN(_pDoc->SetErrorInfo(hr));                   \
}




//=======================================================================

HRESULT
CEventObj::GenericGetElement (IHTMLElement** ppElement, DISPID dispid, ULONG uOffset)
{
    EVENTPARAM *    pparam;
    HRESULT         hr = S_OK;
    CTreeNode  *    pTarget = NULL;
    IUnknown   *    pUnk;

    if (!ppElement)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppElement = NULL;

    if (S_OK == GetUnknownPtr(dispid, &pUnk))
    {
        goto Cleanup;
    }

    *ppElement = (IHTMLElement *)pUnk;

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    pTarget = *(CTreeNode **)(((BYTE *)pparam) + uOffset);

    if (!pTarget || pTarget->Tag() == ETAG_ROOT)
        goto Cleanup;

    if (pparam->lSubDivision >= 0)
    {
        if (pTarget->Tag() == ETAG_IMG )
        {
            CAreaElement * pArea = NULL;
            CImgElement *pImg = DYNCAST(CImgElement, pTarget->Element());

            if (pImg->GetMap())
            {
                pImg->GetMap()->GetAreaContaining(pparam->lSubDivision, &pArea);
                if (pArea)
                    pTarget = pArea->GetFirstBranch();
            }
        }
    }

    if (!pTarget)
        goto Cleanup;

    if (pTarget == pTarget->Element()->GetFirstBranch())
    {
        hr = THR(pTarget->Element()->QueryInterface(IID_IHTMLElement, (void **)ppElement));
    }
    else
    {
        hr = THR(pTarget->GetInterface(IID_IHTMLElement, (void **)ppElement));
    }

Cleanup:
    RRETURN(_pDoc->SetErrorInfo(hr));
}


HRESULT
CEventObj::GenericPutElement (IHTMLElement* pElement, DISPID dispid)
{
    RRETURN(PutUnknownPtr(dispid, pElement));
}

//=======================================================================

#define GET_STRING_VALUE(fnName, strName)       \
    HRESULT CEventObj::get_##fnName  (BSTR *p)  \
{                                               \
    HRESULT         hr;                         \
    EVENTPARAM *    pparam;                     \
                                                \
    if (!p)                                     \
    {                                           \
        hr = E_POINTER;                         \
        goto Cleanup;                           \
    }                                           \
                                                \
    hr = THR(GetParam(&pparam));                \
    if (hr)                                     \
        goto Cleanup;                           \
                                                \
    hr = pparam->##strName.AllocBSTR(p);        \
                                                \
Cleanup:                                        \
    RRETURN(_pDoc->SetErrorInfo(hr));           \
}


#define PUT_STRING_VALUE(fnName, strName)       \
    HRESULT CEventObj::put_##fnName  (BSTR p)   \
{                                               \
    HRESULT         hr;                         \
    EVENTPARAM *    pparam;                     \
                                                \
    MAKESUREPUTSAREALLOWED                      \
                                                \
    if (!p)                                     \
    {                                           \
        hr = E_POINTER;                         \
        goto Cleanup;                           \
    }                                           \
                                                \
    hr = THR(GetParam(&pparam));                \
    if (hr)                                     \
        goto Cleanup;                           \
                                                \
    pparam->##strName(p);                       \
                                                \
Cleanup:                                        \
    RRETURN(_pDoc->SetErrorInfo(hr));           \
}


//=======================================================================

HRESULT
CEventObj::GenericGetLong(long * plongRet, ULONG uOffset)
{
    HRESULT         hr;
    EVENTPARAM *    pparam;

    if (!plongRet)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *plongRet = -1;

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    *plongRet = *(long *)(((BYTE *)pparam) + uOffset);

Cleanup:
    RRETURN(_pDoc->SetErrorInfo(hr));
}



#define PUT_LONG_VALUE(fnName, propName )       \
    HRESULT CEventObj::put_##fnName (long lLongVal) \
{                                               \
    HRESULT         hr;                         \
    EVENTPARAM *    pparam;                     \
                                                \
    MAKESUREPUTSAREALLOWED                      \
                                                \
    hr = THR(GetParam(&pparam));                \
    if (hr)                                     \
        goto Cleanup;                           \
                                                \
    pparam->##propName = lLongVal;              \
                                                \
Cleanup:                                        \
    RRETURN(_pDoc->SetErrorInfo(hr));           \
}


//=======================================================================

//---------------------------------------------------------------------------
//
//  CEventObj ClassDesc
//
//---------------------------------------------------------------------------

const CBase::CLASSDESC CEventObj::s_classdesc =
{
    &CLSID_CEventObj,                // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLEventObj,             // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};



//+-------------------------------------------------------------------------
//
//  Method:     CEventObj::CreateEventObject
//
//--------------------------------------------------------------------------

HRESULT
CEventObj::Create(
    IHTMLEventObj** ppEventObj,
    CDoc *          pDoc,
    BOOL            fCreateAttached /* = TRUE*/,
    LPTSTR          pchSrcUrn /* = NULL */)
{
    HRESULT         hr;
    CEventObj *     pEventObj = NULL;

    if (!ppEventObj)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppEventObj = NULL;

    if (fCreateAttached && pDoc->_pparam->pEventObj)
    {
        pEventObj = pDoc->_pparam->pEventObj;
        pEventObj->AddRef();
    }
    else
    {
        pEventObj = new CEventObj(pDoc);
        if (!pEventObj)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    hr = THR(pEventObj->QueryInterface(IID_IHTMLEventObj, (void **)ppEventObj));
    if (hr)
        goto Cleanup;

    if (!fCreateAttached)
    {
        pEventObj->_pparam = new EVENTPARAM(pDoc, /* fInitState = */ TRUE, /* fPush = */ FALSE);
        if (!pEventObj->_pparam)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        pEventObj->_pparam->pEventObj = pEventObj;

        // This is an XTag event object, mark it as read/write
        pEventObj->_fReadWrite = TRUE;
    }

    hr = pEventObj->SetAttributes(pDoc);

    if (pchSrcUrn)
        pEventObj->_pparam->SetSrcUrn(pchSrcUrn);

Cleanup:
    if (pEventObj)
        pEventObj->Release();

    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CEventObj::SetAttributes
//
//--------------------------------------------------------------------------

HRESULT
CEventObj::SetAttributes(CDoc * pDoc)
{
    HRESULT hr = S_OK;
    EVENTPARAM * pparam = pDoc->_pparam;
    BOOL fExpando = pDoc->_fExpando;

    if (!pparam || !pparam->GetType())
        goto Cleanup;

    pDoc->_fExpando = TRUE;

    // script error expandos
    if (!StrCmpIC(_T("error"), pparam->GetType()))
    {
        CVariant varErrorMessage(VT_BSTR);
        CVariant varErrorUrl(VT_BSTR);
        CVariant varErrorLine(VT_I4);
        CVariant varErrorCharacter(VT_I4);
        CVariant varErrorCode(VT_I4);

        hr = FormsAllocString(pparam->errorParams.pchErrorMessage, &varErrorMessage.bstrVal);
        if (hr)
            goto Cleanup;
        hr = FormsAllocString(pparam->errorParams.pchErrorUrl, &varErrorUrl.bstrVal);
        if (hr)
            goto Cleanup;
        V_I4(&varErrorLine) =           pparam->errorParams.lErrorLine;
        V_I4(&varErrorCharacter) =      pparam->errorParams.lErrorCharacter;
        V_I4(&varErrorCode) =           pparam->errorParams.lErrorCode;

        hr = SetExpando(_T("errorMessage"), &varErrorMessage);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("errorUrl"), &varErrorUrl);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("errorLine"), &varErrorLine);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("errorCharacter"), &varErrorCharacter);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("errorCode"), &varErrorCode);
    }

    // ShowMessage expandos
    else if (!StrCmpIC(_T("message"), pparam->GetType()))
    {
        CVariant varMessageText(VT_BSTR);
        CVariant varMessageCaption(VT_BSTR);
        CVariant varMessageStyle(VT_UI4);
        CVariant varMessageHelpFile(VT_BSTR);
        CVariant varMessageHelpContext(VT_UI4);

        hr = FormsAllocString(pparam->messageParams.pchMessageText, &varMessageText.bstrVal);
        if (hr)
            goto Cleanup;
        hr = FormsAllocString(pparam->messageParams.pchMessageCaption, &varMessageCaption.bstrVal);
        if (hr)
            goto Cleanup;
        hr = FormsAllocString(pparam->messageParams.pchMessageHelpFile, &varMessageHelpFile.bstrVal);
        if (hr)
            goto Cleanup;
        V_UI4(&varMessageStyle)         = pparam->messageParams.dwMessageStyle;
        V_UI4(&varMessageHelpContext)   = pparam->messageParams.dwMessageHelpContext;

        hr = SetExpando(_T("messageText"), &varMessageText);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("messageCaption"), &varMessageCaption);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("messageStyle"), &varMessageStyle);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("messageHelpFile"), &varMessageHelpFile);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("messageHelpContext"), &varMessageHelpContext);
        if (hr)
            goto Cleanup;
    }

    // page setup dialog expandos
    else if (!StrCmpIC(_T("pagesetup"), pparam->GetType()))
    {
        CVariant varPageSetupHeader(VT_BSTR);
        CVariant varPageSetupFooter(VT_BSTR);
        CVariant varPageSetupDlg(VT_I4);

        hr = FormsAllocString(pparam->pagesetupParams.pchPagesetupHeader, &varPageSetupHeader.bstrVal);
        if (hr)
            goto Cleanup;
        hr = FormsAllocString(pparam->pagesetupParams.pchPagesetupFooter, &varPageSetupFooter.bstrVal);
        if (hr)
            goto Cleanup;

//$ WIN64: varPageSetupDlg is casting down LONG_PTR lPagesetupDlg to a VT_I4 variant

        V_I4(&varPageSetupDlg) = (LONG)pparam->pagesetupParams.lPagesetupDlg;

        hr = SetExpando(_T("pagesetupHeader"), &varPageSetupHeader);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("pagesetupFooter"), &varPageSetupFooter);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("pagesetupStruct"), &varPageSetupDlg);
        if (hr)
            goto Cleanup;
    }

    // print dialog expandos
    else if (!StrCmpIC(_T("print"), pparam->GetType()))
    {
        CVariant varPrintfRootDocumentHasFrameset(VT_BOOL);
        CVariant varPrintfAreRatingsEnabled(VT_BOOL);
        CVariant varPrintfActiveFrame(VT_BOOL);
        CVariant varPrintfLinked(VT_BOOL);
        CVariant varPrintfSelection(VT_BOOL);
        CVariant varPrintfAsShown(VT_BOOL);
        CVariant varPrintfShortcutTable(VT_BOOL);
        CVariant varPrintiFontScaling(VT_INT);
        CVariant varPrintpBodyActiveTarget(VT_UNKNOWN);
        CVariant varPrintDlg(VT_I4);
        CVariant varPrintToFileOk(VT_BOOL);
        CVariant varPrintToFileName(VT_BSTR);
#ifdef UNIX
        CVariant varPrintdmOrientation(VT_INT);
#endif

        V_BOOL(&varPrintfRootDocumentHasFrameset)   = pparam->printParams.fPrintRootDocumentHasFrameset
                                                        ? VB_TRUE : VB_FALSE;
        V_BOOL(&varPrintfAreRatingsEnabled)         = pparam->printParams.fPrintAreRatingsEnabled
                                                        ? VB_TRUE : VB_FALSE;
        V_BOOL(&varPrintfActiveFrame)               = pparam->printParams.fPrintActiveFrame
                                                        ? VB_TRUE : VB_FALSE;
        V_BOOL(&varPrintfLinked)                    = pparam->printParams.fPrintLinked
                                                        ? VB_TRUE : VB_FALSE;
        V_BOOL(&varPrintfSelection)                 = pparam->printParams.fPrintSelection
                                                        ? VB_TRUE : VB_FALSE;
        V_BOOL(&varPrintfAsShown)                   = pparam->printParams.fPrintAsShown
                                                        ? VB_TRUE : VB_FALSE;
        V_BOOL(&varPrintfShortcutTable)             = pparam->printParams.fPrintShortcutTable
                                                        ? VB_TRUE : VB_FALSE;
        V_INT(&varPrintiFontScaling)                = pparam->printParams.iPrintFontScaling;
        if (pparam->printParams.pPrintBodyActiveTarget)
            pparam->printParams.pPrintBodyActiveTarget->AddRef();
        V_UNKNOWN(&varPrintpBodyActiveTarget)       = pparam->printParams.pPrintBodyActiveTarget;

//$ WIN64: varPrintDlg is casting down LONG_PTR lPrintDlg to a VT_I4 variant

        V_I4(&varPrintDlg)                          = (LONG)pparam->printParams.lPrintDlg;
        V_BOOL(&varPrintToFileOk)                   = pparam->printParams.fPrintToFileOk
                                                        ? VB_TRUE : VB_FALSE;
        hr = FormsAllocString(pparam->printParams.pchPrintToFileName, &varPrintToFileName.bstrVal);
        if (hr)
            goto Cleanup;
#ifdef UNIX
        V_I4(&varPrintdmOrientation)                = pparam->printParams.iPrintdmOrientation;
#endif

        hr = SetExpando(_T("printfRootDocumentHasFrameset"), &varPrintfRootDocumentHasFrameset);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("printfAreRatingsEnabled"), &varPrintfAreRatingsEnabled);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("printfActiveFrame"), &varPrintfActiveFrame);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("printfLinked"), &varPrintfLinked);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("printfSelection"), &varPrintfSelection);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("printfAsShown"), &varPrintfAsShown);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("printfShortcutTable"), &varPrintfShortcutTable);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("printiFontScaling"), &varPrintiFontScaling);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("printpBodyActiveTarget"), &varPrintpBodyActiveTarget);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("printStruct"), &varPrintDlg);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("printToFileOk"), &varPrintToFileOk);
        if (hr)
            goto Cleanup;

        hr = SetExpando(_T("printToFileName"), &varPrintToFileName);
        if (hr)
            goto Cleanup;

#ifdef UNIX
        hr = SetExpando(_T("printdmOrientation"), &varPrintdmOrientation);
        if (hr)
            goto Cleanup;
#endif // UNIX
    }
    
    // property sheet dialog expandos
    else if (!StrCmpIC(_T("propertysheet"), pparam->GetType()))
    {
        // we are using a VARIANT and not a CVariant because we do not want to free the
        // array here
        VARIANT varpaPropertysheetPunks;
        V_VT(&varpaPropertysheetPunks) = VT_SAFEARRAY;
        V_ARRAY(&varpaPropertysheetPunks) = pparam->propertysheetParams.paPropertysheetPunks;

        hr = SetExpando(_T("propertysheetPunks"), &varpaPropertysheetPunks);
        if (hr)
            goto Cleanup;
    }

Cleanup:
    pDoc->_fExpando = fExpando;
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CEventObj::COnStackLock::COnStackLock
//
//--------------------------------------------------------------------------

CEventObj::COnStackLock::COnStackLock(IHTMLEventObj * pEventObj)
{
    HRESULT     hr;

    Assert (pEventObj);

    _pEventObj = pEventObj;
    _pEventObj->AddRef();

    hr = THR(pEventObj->QueryInterface (CLSID_CEventObj, (void**)&_pCEventObj));
    if (hr)
        goto Cleanup;

    _pCEventObj->_pparam->Push();

Cleanup:
    return;
}

//+-------------------------------------------------------------------------
//
//  Method:     CEventObj::COnStackLock::~COnStackLock
//
//--------------------------------------------------------------------------

CEventObj::COnStackLock::~COnStackLock()
{
    _pCEventObj->_pparam->Pop();
    _pEventObj->Release();
}

//---------------------------------------------------------------------------
//
//  Member:     CEventObj::CEventObj
//
//  Synopsis:   constructor
//
//---------------------------------------------------------------------------

CEventObj::CEventObj(CDoc * pDoc)
{
    _pDoc = pDoc;
    _pDoc->SubAddRef();
}

//---------------------------------------------------------------------------
//
//  Member:     CEventObj::~CEventObj
//
//  Synopsis:   destructor
//
//---------------------------------------------------------------------------

CEventObj::~CEventObj()
{
    _pDoc->SubRelease();
    delete _pCollectionCache;
    delete _pparam;
}

//---------------------------------------------------------------------------
//
//  Member:     CEventObj::GetParam
//
//---------------------------------------------------------------------------

HRESULT
CEventObj::GetParam(EVENTPARAM ** ppParam)
{
    Assert (ppParam);

    if (_pparam)
    {
        (*ppParam) = _pparam;
    }
    else
    {
        if (_pDoc->_pparam)
        {
            (*ppParam) = _pDoc->_pparam;
        }
        else
        {
            (*ppParam) = NULL;
            RRETURN (DISP_E_MEMBERNOTFOUND);

        }
    }
    RRETURN (S_OK);
}

//+-------------------------------------------------------------------------
//
//  Method:     CEventObj::PrivateQueryInterface
//
//  Synopsis:   Per IPrivateUnknown
//
//--------------------------------------------------------------------------

HRESULT
CEventObj::PrivateQueryInterface(REFIID iid, LPVOID * ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_TEAROFF_DISPEX(this, NULL)
    QI_TEAROFF(this, IObjectIdentity, NULL)
    QI_TEAROFF(this, IHTMLEventObj, NULL)
    QI_TEAROFF(this, IHTMLEventObj2, NULL)
    QI_TEAROFF2(this, IUnknown, IHTMLEventObj, NULL)
    default:
        if (IsEqualGUID(iid, CLSID_CEventObj))
        {
            *ppv = this;
            return S_OK;
        }

        // Primary default interface, or the non dual
        // dispinterface return the same object -- the primary interface
        // tearoff.
        if (DispNonDualDIID(iid))
        {
#ifndef WIN16
            HRESULT hr = CreateTearOffThunk( this,
                                        (void *)s_apfnIHTMLEventObj,
                                        NULL,
                                        ppv);
#else
            BYTE *pThis = ((BYTE *) (void *) ((CBase *) this)) - m_baseOffset;
            hr = THR(CreateTearOffThunk(pThis, (void *)s_apfnIHTMLEventObj, NULL, ppv));
#endif
            if (hr)
                RRETURN(hr);
        }
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     EnsureCollectionCache
//
//  Synopsis:   Create the event's collection cache if needed.
//
//-------------------------------------------------------------------------

HRESULT
CEventObj::EnsureCollectionCache()
{
    HRESULT hr = S_OK;

    if (!_pCollectionCache)
    {
        _pCollectionCache = new CCollectionCache(
                this,          // double cast needed for Win16.
                _pDoc,
                ENSURE_METHOD(CEventObj, EnsureCollections, ensurecollections));
        if (!_pCollectionCache)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(_pCollectionCache->InitReservedCacheItems(NUMBER_OF_EVENT_COLLECTIONS));
        if (hr)
            goto Error;
    }

    hr = THR( _pCollectionCache->EnsureAry( 0 ) );

Cleanup:
    RRETURN(hr);

Error:
    delete _pCollectionCache;
    _pCollectionCache = NULL;
    goto Cleanup;
}


//+------------------------------------------------------------------------
//
//  Member:     EnsureCollections
//
//  Synopsis:   Refresh the event's collections, if needed.
//
//-------------------------------------------------------------------------

HRESULT
CEventObj::EnsureCollections(long lIndex, long * plCollectionVersion)
{
    HRESULT hr = S_OK;
    EVENTPARAM *    pparam;
    int i;

    // Nothing to do so get out.
    if (*plCollectionVersion)
        goto Cleanup;

    MtAdd(Mt(BldEventObjElemsCol), +1, 0);

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    // Reset the collections.
    for (i = 0; i < NUMBER_OF_EVENT_COLLECTIONS; i++)
    {
        _pCollectionCache->ResetAry(i);
    }

    // Reload the bound elements collection
    if (pparam->pProvider)
    {
        hr = pparam->pProvider->
                LoadBoundElementCollection(_pCollectionCache, EVENT_BOUND_ELEMENTS_COLLECTION);
        if (hr)
        {
            _pCollectionCache->ResetAry(EVENT_BOUND_ELEMENTS_COLLECTION);
        }
    }

    *plCollectionVersion = 1;   // to mark it done

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CEventObj::get_srcElem
//  Method:     CEventObj::get_fromElement
//  Method:     CEventObj::get_toElement
//
//  Synopsis:   Per IEventObj.  see macro defined at the top of this file.
//
//--------------------------------------------------------------------------

HRESULT
CEventObj::get_srcElement (IHTMLElement** ppElement)
{
    return GenericGetElement(ppElement, DISPID_CEventObj_srcElement,  offsetof(EVENTPARAM, _pNode));
}


HRESULT
CEventObj::get_fromElement (IHTMLElement** ppElement)
{
    return GenericGetElement(ppElement, DISPID_CEventObj_fromElement,  offsetof(EVENTPARAM, _pNodeFrom));
}


HRESULT
CEventObj::get_toElement (IHTMLElement** ppElement)
{
    return GenericGetElement(ppElement, DISPID_CEventObj_toElement,  offsetof(EVENTPARAM, _pNodeTo));
}


HRESULT
CEventObj::put_srcElement (IHTMLElement* pElement)
{
    return GenericPutElement(pElement, DISPID_CEventObj_srcElement);
}


HRESULT
CEventObj::put_fromElement (IHTMLElement* pElement)
{
    return GenericPutElement(pElement, DISPID_CEventObj_fromElement);
}


HRESULT
CEventObj::put_toElement (IHTMLElement* pElement)
{
    return GenericPutElement(pElement, DISPID_CEventObj_toElement);
}


//+-------------------------------------------------------------------------
//
//  Method:     CEventObj::get_button
//  Method:     CEventObj::get_keyCode      KeyCode of a key event
//  Method:     CEventObj::get_reason       reason enum for ondatasetcomplete
//  Method:     CEventObj::get_clientX      Per IEventObj, in client coordinates
//  Method:     CEventObj::get_clientY      Per IEventObj, in client coordinates
//  Method:     CEventObj::get_screenX      Per IEventObj, in screen coordinates
//  Method:     CEventObj::get_screenY      Per IEventObj, in screen coordinates
//
//  Synopsis:   Per IEventObj
//
//--------------------------------------------------------------------------

HRESULT
CEventObj::get_button (long * plReturn)
{
    return GenericGetLong( plReturn, offsetof(EVENTPARAM, _lButton ));
}

HRESULT
CEventObj::get_keyCode (long * plReturn)
{
    return GenericGetLong( plReturn, offsetof(EVENTPARAM, _lKeyCode ));
}

HRESULT
CEventObj::get_reason (long * plReturn)
{
    return GenericGetLong( plReturn, offsetof(EVENTPARAM, _lReason ));
}

HRESULT
CEventObj::get_clientX (long * plReturn)
{
    return GenericGetLong( plReturn, offsetof(EVENTPARAM, _clientX ));
}

HRESULT
CEventObj::get_clientY (long * plReturn)
{
    return GenericGetLong( plReturn, offsetof(EVENTPARAM, _clientY ));
}

HRESULT
CEventObj::get_screenX (long * plReturn)
{
    return GenericGetLong( plReturn, offsetof(EVENTPARAM, _screenX ));
}

HRESULT
CEventObj::get_screenY (long * plReturn)
{
    return GenericGetLong( plReturn, offsetof(EVENTPARAM, _screenY ));
}

HRESULT
CEventObj::get_offsetX (long * plReturn)
{
    HRESULT      hr;
    EVENTPARAM * pparam;

    if (!plReturn)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *plReturn = -1;

    hr = THR(GetParam(&pparam));
    if (hr == S_OK && pparam && pparam->_pNode)
    {
        // Do the work to subtract away the border width
        CDocInfo            di;
        CBorderInfo         bi;

        memset(&bi, 0, sizeof(bi));
        pparam->_pNode->Element()->GetBorderInfo(&di, &bi, TRUE);

        long  iBdrLeft = bi.aiWidths[BORDER_LEFT];

        *plReturn = pparam->_offsetX - iBdrLeft;
    }

Cleanup:
    return hr;
}

HRESULT
CEventObj::get_offsetY (long * plReturn)
{
    HRESULT      hr;
    EVENTPARAM * pparam;

    if (!plReturn)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *plReturn = -1;

    hr = THR(GetParam(&pparam));
    if (hr == S_OK && pparam && pparam->_pNode)
    {
        // Do the work to subtract away the border width
        CDocInfo            di;
        CBorderInfo         bi;

        memset(&bi, 0, sizeof(bi));
        pparam->_pNode->Element()->GetBorderInfo(&di, &bi, TRUE);

        long  iBdrTop  = bi.aiWidths[BORDER_TOP];

        *plReturn = pparam->_offsetY - iBdrTop;
    }

Cleanup:
    return hr;
}

HRESULT
CEventObj::get_x (long * plReturn)
{
    return GenericGetLong( plReturn, offsetof(EVENTPARAM, _x ));
}

HRESULT
CEventObj::get_y (long * plReturn)
{
    return GenericGetLong( plReturn, offsetof(EVENTPARAM, _y ));
}


PUT_LONG_VALUE(button, _lButton);
PUT_LONG_VALUE(reason, _lReason);
PUT_LONG_VALUE(clientX, _clientX);
PUT_LONG_VALUE(clientY, _clientY);
PUT_LONG_VALUE(screenX, _screenX);
PUT_LONG_VALUE(screenY, _screenY);
PUT_LONG_VALUE(offsetX, _offsetX);
PUT_LONG_VALUE(offsetY, _offsetY);
PUT_LONG_VALUE(x, _x);
PUT_LONG_VALUE(y, _y);

//+---------------------------------------------------------------------------
//
//  Member:     CEventObj::put_keyCode
//
//  Synopsis:   Puts the keyCode
//
//----------------------------------------------------------------------------

HRESULT
CEventObj::put_keyCode(long lKeyCode)
{
    HRESULT         hr;
    EVENTPARAM *    pparam;

    // SECURITY ALERT:- we cannot allow a user to set the keycode if the 
    // srcElement is an <input type=file>, otherwise it is possible for 
    // a page to access any file off the user's HardDrive without them
    // knowing (see  bug 49620 for a more complete description)
    // HTA's (trusted Doc's) should allow this.
    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    // is the srcElement an input , and if so is the type: file ?
    if (pparam->_pNode &&
        (pparam->_pNode->Tag() == ETAG_INPUT) &&
        (DYNCAST(CInput, pparam->_pNode->Element())->GetAAtype() == htmlInputFile) &&
        !(_pDoc->IsTrustedDoc()))
    {
        hr = E_ACCESSDENIED;
    }
    else
    {
        pparam->_lKeyCode = lKeyCode;
    }

Cleanup:
    RRETURN(_pDoc->SetErrorInfo(hr));
}

//+---------------------------------------------------------------
//
//  Method : get_repeat
//
//  Synopsis : returns a vbool indicating whether this was a repeated event
//          as in the case of holing a key down
//
//--------------------------------------------------------------

HRESULT
CEventObj::get_repeat (VARIANT_BOOL *pfRepeat)
{
    HRESULT         hr;
    EVENTPARAM *    pparam;

    if (!pfRepeat)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    *pfRepeat = VARIANT_BOOL_FROM_BOOL(pparam->fRepeatCode);

Cleanup:
    RRETURN(_pDoc->SetErrorInfo(hr));
}



//+-------------------------------------------------------------------------
//
//  Method:     CEventObj::get_altKey, get_ctrlKey, get_shiftKey
//
//  Synopsis:   Per IEventObj.  see macro defined at the top of this file.
//
//--------------------------------------------------------------------------

GETKEYVALUE(altKey, VB_ALT);
GETKEYVALUE(ctrlKey, VB_CONTROL);
GETKEYVALUE(shiftKey, VB_SHIFT);


PUTKEYVALUE(altKey, VB_ALT);
PUTKEYVALUE(ctrlKey, VB_CONTROL);
PUTKEYVALUE(shiftKey, VB_SHIFT);


//+---------------------------------------------------------------------------
//
//  Member:     CEventObj::get_cancelBubble
//
//  Synopsis:   Cancels the event bubbling. Used by CElement::FireEvents
//
//----------------------------------------------------------------------------

HRESULT
CEventObj::get_cancelBubble(VARIANT_BOOL *pfCancel)
{
    HRESULT         hr;
    EVENTPARAM *    pparam;

    if (!pfCancel)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    *pfCancel = VARIANT_BOOL_FROM_BOOL(pparam->fCancelBubble);

Cleanup:
    RRETURN(_pDoc->SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Member:     CEventObj::put_cancelBubble
//
//  Synopsis:   Cancels the event bubbling. Used by CElement::FireEvents
//
//----------------------------------------------------------------------------

HRESULT
CEventObj::put_cancelBubble(VARIANT_BOOL fCancel)
{
    HRESULT         hr;
    EVENTPARAM *    pparam;

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    pparam->fCancelBubble = fCancel ? TRUE : FALSE;

Cleanup:
    RRETURN(_pDoc->SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Member:     CEventObj::get_returnValue
//
//  Synopsis:   Retrieve the current cancel status.
//
//----------------------------------------------------------------------------

HRESULT
CEventObj::get_returnValue(VARIANT * pvarReturnValue)
{
    HRESULT         hr;
    EVENTPARAM *    pparam;

    if (!pvarReturnValue)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    hr = THR(VariantCopy(pvarReturnValue, &pparam->varReturnValue));

Cleanup:
    RRETURN(_pDoc->SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Member:     CEventObj::put_returnValue
//
//  Synopsis:   Cancels the default action of an event.
//
//----------------------------------------------------------------------------

HRESULT
CEventObj::put_returnValue(VARIANT varReturnValue)
{
    HRESULT         hr;
    EVENTPARAM *    pparam;

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    hr = THR(VariantCopy (&pparam->varReturnValue, &varReturnValue));

Cleanup:
    RRETURN(_pDoc->SetErrorInfo(hr));
}




//+---------------------------------------------------------------------------
//
//  member :    CEventObj::get_type
//
//  Synopsis :  For Nav 4.2 compatability. this returns a string which is the
//              type of this event.
//
//----------------------------------------------------------------------------
GET_STRING_VALUE(type, _cstrType )

PUT_STRING_VALUE(type, SetType )

//+---------------------------------------------------------------------------
//
//  member :    CEventObj::get_propertyName
//
//  Synopsis :  For the onproperty change event, this returnes the name of the
//     property that changed
//
//----------------------------------------------------------------------------

GET_STRING_VALUE(propertyName, _cstrPropName)

PUT_STRING_VALUE(propertyName, SetPropName)

//+---------------------------------------------------------------------------
//
//  member :    CEventObj::get_qualifier
//
//  Synopsis :  Qualifier argument to ondatasetchanged, ondataavailable, and
//              ondatasetcomplete events
//
//----------------------------------------------------------------------------

GET_STRING_VALUE(qualifier, _cstrQualifier)

PUT_STRING_VALUE(qualifier, SetQualifier)

//+---------------------------------------------------------------------------
//
//  member :    CEventObj::get_srcUrn
//
//----------------------------------------------------------------------------

GET_STRING_VALUE(srcUrn, _cstrSrcUrn)

PUT_STRING_VALUE(srcUrn, SetSrcUrn)

//+-------------------------------------------------------------------------
//
//  Method:     EVENTPARAM::GetParentCoordinates
//
//  Synopsis:   Helper function for getting the parent coordinates
//              notset/statis -- doc coordinates
//              relative      --  [styleleft, styletop] +
//                                [x-site.rc.x, y-site.rc.y]
//              absolute      --  [A_parent.rc.x, A_parent.rc.y,] +
//                                [x-site.rc.x, y-site.rc.y]
//
//  Parameters : px, py - the return point 
//               fContainer :  TRUE - COORDSYS_CONTAINER
//                             FALSE - COORDSYS_CONTENT
//
//  these parameters are here for NS compat. and as such the parent defined
//  for the positioning are only ones that are absolutely or relatively positioned
//  or the body.
//
//--------------------------------------------------------------------------


HRESULT
EVENTPARAM::GetParentCoordinates(long * px, long * py)
{
    CPoint         pt(0,0);
    HRESULT        hr = S_OK;
    CLayout *      pLayout;
    CElement *     pElement;
    CDispNode *    pDispNode;
    CRect          rc;
    CTreeNode    * pZNode = _pNode;

    if (!_pNode || !_pNode->_pNodeParent)
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    // First, determine if we need to climg out of a slavetree
    //---------------------------------------------------------
    if (_pNode->Tag() == ETAG_TXTSLAVE)
    {
        Assert(_pNode->GetMarkup());

        CElement *pElemMaster;

        pElemMaster = _pNode->GetMarkup()->Master();
        if (!pElemMaster)
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }
        
        pZNode = pElemMaster->GetFirstBranch();
        if (!pZNode)
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }
    }

    // now the tricky part. We have a Node for the element that 
    // the event is on, but the "parent" that we are reporting
    // position information for is not constant. The parent is the 
    // first ZParent of this node that is relatively positioned
    // and the body is always reported as relative.  Absolutely 
    // positioned elements DON'T COUNT.
    //---------------------------------------------------------

    // walk up looking for a positioned thing.
    while (pZNode && 
           !pZNode->IsRelative() && 
           !(pZNode->Tag() == ETAG_ROOT) &&
           !(pZNode->Tag() == ETAG_BODY) )
    {
        pZNode = pZNode->ZParentBranch();
    }

    pElement = pZNode ? pZNode->Element() : NULL;

    // now we know the element that we are reporting a position wrt
    // and so we just need to get the dispnode and the position info
    //---------------------------------------------------------
    if(pElement)
    {
        pLayout = pElement->GetUpdatedNearestLayout();

        if(pLayout)
        {
            pDispNode = pLayout->GetElementDispNode(pElement);

            if(pDispNode)
            {
                pDispNode->TransformPoint(&pt, COORDSYS_CONTAINER, COORDSYS_GLOBAL);
            }
        }
    }

    // adjust for the offset of the mouse wrt the postition of the parent
    pt.x = _clientX - pt.x;
    pt.y = _clientY - pt.y;


    // and return the values.
    if (px)
        *px = pt.x;
    if (py)
        *py = pt.y;

Cleanup:
    RRETURN1(hr, DISP_E_MEMBERNOTFOUND);
}
//+---------------------------------------------------------------------------
//
//  member :    CEventObj::get_srcFilter
//
//  Synopsis :  Return boolean of the filter that fired the event.
//
//----------------------------------------------------------------------------
HRESULT
CEventObj::get_srcFilter(IDispatch **pFilter)
{
    HRESULT         hr = S_OK;
    EVENTPARAM *    pparam;

    if (!pFilter)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Try to get from attr array in first
    if(S_OK == GetDispatchPtr(DISPID_CEventObj_srcFilter, pFilter))
        goto Cleanup;

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    *pFilter = NULL;

    if ( pparam->psrcFilter )
    {
        hr = pparam->psrcFilter->QueryInterface (
            IID_IDispatch, (void**)&pFilter );
        if ( hr == E_NOINTERFACE )
            hr = S_OK; // Just return NULL - some filters aren't automatable
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  member :    CEventObj::get_bookmarks
//
//  Synopsis :
//
//----------------------------------------------------------------------------
HRESULT
CEventObj::get_bookmarks(IHTMLBookmarkCollection **ppBookmarkCollection)
{
    HRESULT         hr = E_NOTIMPL;
    EVENTPARAM *    pparam;
    IUnknown      * pUnk;

    if (!ppBookmarkCollection)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppBookmarkCollection = NULL;

    if(S_OK == GetUnknownPtr(DISPID_CEventObj_bookmarks, &pUnk))
    {
        *ppBookmarkCollection = (IHTMLBookmarkCollection *)pUnk;
        goto Cleanup;
    }

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    if (pparam->pProvider)
    {
        hr = pparam->pProvider->get_bookmarks(ppBookmarkCollection);
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    CEventObj::get_recordset
//
//  Synopsis :
//
//----------------------------------------------------------------------------
HRESULT
CEventObj::get_recordset(IDispatch **ppDispRecordset)
{
    HRESULT         hr = S_OK;
    EVENTPARAM *    pparam;

    if (!ppDispRecordset)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppDispRecordset = NULL;

    if(!GetDispatchPtr(DISPID_CEventObj_recordset, ppDispRecordset))
        goto Cleanup;

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    if (pparam->pProvider)
    {
        hr = pparam->pProvider->get_recordset(ppDispRecordset);
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    CEventObj::get_dataFld
//
//  Synopsis :
//
//----------------------------------------------------------------------------
HRESULT
CEventObj::get_dataFld(BSTR *pbstrDataFld)
{
    HRESULT         hr;
    EVENTPARAM *    pparam;
    AAINDEX         aaIndex;

    if (!pbstrDataFld)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrDataFld = NULL;

    aaIndex = FindAAIndex(DISPID_CEventObj_dataFld, CAttrValue::AA_Internal);
    if(aaIndex != AA_IDX_UNKNOWN)
    {
        BSTR bstrStr;
        hr = THR(GetIntoBSTRAt(aaIndex, &bstrStr));
        if(hr)
            goto Cleanup;
        hr = THR(FormsAllocString(bstrStr, pbstrDataFld));
        goto Cleanup;
    }

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    if (pparam->pProvider)
    {
        hr = pparam->pProvider->get_dataFld(pbstrDataFld);
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    CEventObj::get_boundElements
//
//  Synopsis :
//
//----------------------------------------------------------------------------
HRESULT
CEventObj::get_boundElements(IHTMLElementCollection **ppElementCollection)
{
    HRESULT         hr = S_OK;
    IUnknown      * pUnk;

    if (!ppElementCollection)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppElementCollection = NULL;

    if(S_OK == GetUnknownPtr(DISPID_CEventObj_boundElements, &pUnk))
    {
        *ppElementCollection = (IHTMLElementCollection *)pUnk;
        goto Cleanup;
    }

    // Create a collection cache if we don't already have one.
    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;

    hr = THR(_pCollectionCache->GetDisp(EVENT_BOUND_ELEMENTS_COLLECTION,
                                        (IDispatch **)ppElementCollection));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CEventObj::put_repeat(VARIANT_BOOL fRepeat)
{
    HRESULT         hr;
    EVENTPARAM    * pparam;

    MAKESUREPUTSAREALLOWED

    hr = THR(GetParam(&pparam));
    if (hr)
        goto Cleanup;

    pparam->fRepeatCode = BOOL_FROM_VARIANT_BOOL(fRepeat);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CEventObj::put_boundElements(struct IHTMLElementCollection *pColl)
{
    RRETURN(PutUnknownPtr(DISPID_CEventObj_boundElements, pColl));
}


HRESULT
CEventObj::put_dataFld(BSTR strFld)
{
    HRESULT     hr;

    MAKESUREPUTSAREALLOWED

    hr = THR(AddBSTR(DISPID_CEventObj_dataFld, strFld, CAttrValue::AA_Internal));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CEventObj::put_recordset(struct IDispatch *pDispatch)
{
    RRETURN(PutDispatchPtr(DISPID_CEventObj_recordset, pDispatch));
}


HRESULT
CEventObj::put_bookmarks(struct IHTMLBookmarkCollection *pColl)
{
    RRETURN(PutUnknownPtr(DISPID_CEventObj_bookmarks, pColl));
}


HRESULT
CEventObj::put_srcFilter(struct IDispatch *pDispatch)
{
    RRETURN(PutDispatchPtr(DISPID_CEventObj_srcFilter, pDispatch));
}


HRESULT
EVENTPARAM::CalcRestOfCoordinates()
{
    HRESULT     hr = S_OK;
    CLayout   * pLayout = NULL;

    if(_pNode)
    {
        hr = THR(GetParentCoordinates(&_x, &_y));
        if(hr)
            goto Cleanup;

        pLayout = _pNode->GetUpdatedNearestLayout();
        if (!pLayout)
        {
            _offsetX = -1;
            _offsetY = -1;
            goto Cleanup;
        }

// BUGBUG: Do this a better way? (brendand)
        // the _offsetX and _offsetY are reported always INSIDE the borders of the offsetParent
        // but for perf reasons we don't subtract out the borders until they are asked for
        //------------------------------------------------------------------------------------
        _offsetX = _clientX - pLayout->GetPositionLeft(COORDSYS_GLOBAL) + pLayout->GetXScroll();
        _offsetY = _clientY - pLayout->GetPositionTop(COORDSYS_GLOBAL) + pLayout->GetYScroll();
    }

Cleanup:
    RRETURN(hr);
}


void
EVENTPARAM::SetNodeAndCalcCoordinates(CTreeNode *pNewNode)
{
    if(_pNode != pNewNode)
    {
        _pNode = pNewNode;
        if(_pNode) CalcRestOfCoordinates();
    }
}

HRESULT
CEventObj::PutUnknownPtr(DISPID dispid, IUnknown *pElement)
{
    HRESULT hr;

    MAKESUREPUTSAREALLOWED

    if(pElement)                                              \
        pElement->AddRef();                                   \

    hr = THR(AddUnknownObject(dispid, (IUnknown *)pElement, CAttrValue::AA_Internal));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CEventObj::PutDispatchPtr(DISPID dispid, IDispatch *pDispatch)
{
    HRESULT hr;

    MAKESUREPUTSAREALLOWED

    if(pDispatch)
        pDispatch->AddRef();

    hr = THR(AddDispatchObject(dispid, pDispatch, CAttrValue::AA_Internal));

Cleanup:
    RRETURN(SetErrorInfo(hr));

}


HRESULT
CEventObj::GetUnknownPtr(DISPID dispid, IUnknown **ppElement)
{
    HRESULT hr;
    AAINDEX aaIndex = FindAAIndex(dispid, CAttrValue::AA_Internal);

    Assert(ppElement);

    if(aaIndex != AA_IDX_UNKNOWN)
        hr = THR(GetUnknownObjectAt(aaIndex, ppElement));
    else
    {
        *ppElement = NULL;
        hr = S_FALSE;
    }

    RRETURN1(SetErrorInfo(hr), S_FALSE);
}


HRESULT
CEventObj::GetDispatchPtr(DISPID dispid, IDispatch **ppElement)
{
    HRESULT hr;
    AAINDEX aaIndex = FindAAIndex(dispid, CAttrValue::AA_Internal);

    if(aaIndex != AA_IDX_UNKNOWN)
        hr = THR(GetDispatchObjectAt(aaIndex, ppElement));
    else
        hr = S_FALSE;

    RRETURN1(SetErrorInfo(hr), S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  member :    CEventObj::get_dataTransfer
//
//  Synopsis :  Return the data transfer object.
//
//----------------------------------------------------------------------------
HRESULT
CEventObj::get_dataTransfer(IHTMLDataTransfer **ppDataTransfer)
{
    HRESULT hr = S_OK;
    CDataTransfer * pDataTransfer;
    IDataObject * pDataObj = _pDoc->_pInPlace->_pDataObj;

    if (!ppDataTransfer)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppDataTransfer = NULL;

    // are we in a drag-drop operation?
    if (!pDataObj)
    {
        CDragStartInfo * pDragStartInfo = _pDoc->_pDragStartInfo;
        if (!pDragStartInfo)
            // leave hr = S_OK and just return NULL
            goto Cleanup;
        if (!pDragStartInfo->_pDataObj)
        {
            hr = pDragStartInfo->CreateDataObj();
            if (hr || pDragStartInfo->_pDataObj == NULL)
                goto Cleanup;
        }

        pDataObj = pDragStartInfo->_pDataObj;
    }

    pDataTransfer = new CDataTransfer(_pDoc, pDataObj, TRUE);   // fDragDrop = TRUE
    if (!pDataTransfer)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = THR(pDataTransfer->QueryInterface(
                IID_IHTMLDataTransfer,
                (void **) ppDataTransfer));
        pDataTransfer->Release();
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

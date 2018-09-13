//+--------------------------------------------------------------------------
//
//  File:       stdform.cxx
//
//  Contents:   Script Debugger related code
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_ESCRIPT_HXX_
#define X_ESCRIPT_HXX_
#include "escript.hxx"
#endif

#ifndef X_DEBUGGER_HXX_
#define X_DEBUGGER_HXX_
#include "debugger.hxx"
#endif

///////////////////////////////////////////////////////////////////////////
//
//  misc
//
///////////////////////////////////////////////////////////////////////////

// MtDefines

MtDefine(CScriptCookieTable,                   Mem,                "CScriptCookieTable");
MtDefine(CScriptCookieTable_CItemsArray,       Mem,                "CScriptCookieTable::CItemsArray");

MtDefine(CScriptDebugDocument,                 Mem,                "CScriptDebugDocument");

// trace tags

DeclareTag(tagDebuggerDocumentSize,            "Debugger",          "trace RequestDocumentSize and UpdateDocumentSize")

// CScriptDebugDocument related
#ifndef NO_SCRIPT_DEBUGGER
extern interface IProcessDebugManager * g_pPDM;
extern interface IDebugApplication *    g_pDebugApp;
#endif // NO_SCRIPT_DEBUGGER
//+------------------------------------------------------------------------
//
//  Function:     GetMarkupFromBase
//
//-------------------------------------------------------------------------

CMarkup *
GetMarkupFromBase (CBase * pBase)
{
    CMarkup *   pMarkup = NULL;

    THR_NOTRACE(pBase->PrivateQueryInterface(CLSID_CMarkup, (void**)&pMarkup));

    return pMarkup;
}

//+------------------------------------------------------------------------
//
//  Function:     GetScriptElementFromBase
//
//-------------------------------------------------------------------------

CScriptElement *
GetScriptElementFromBase (CBase * pBase)
{
    CScriptElement *   pScriptElement = NULL;

    THR_NOTRACE(pBase->PrivateQueryInterface(CLSID_HTMLScriptElement, (void**)&pScriptElement));

    return pScriptElement;
}

//+------------------------------------------------------------------------
//
//  Function:     CrackSourceObject
//
//-------------------------------------------------------------------------

void
CrackSourceObject(CBase * pSourceObject, CMarkup ** ppMarkup, CScriptElement ** ppScriptElement)
{
    Assert (ppMarkup && ppScriptElement);

    *ppScriptElement = NULL;

    *ppMarkup = GetMarkupFromBase(pSourceObject);
    if (*ppMarkup)
        goto Cleanup;   // done

    *ppScriptElement = GetScriptElementFromBase(pSourceObject);
    if ((*ppScriptElement) && (*ppScriptElement)->_fSrc)
        goto Cleanup;   // done

    *ppMarkup = (*ppScriptElement)->GetMarkup();
    *ppScriptElement = NULL;

Cleanup:
    Assert (( (*ppScriptElement) && !(*ppMarkup)) ||
            (!(*ppScriptElement) &&  (*ppMarkup)));
    return;
}

//+------------------------------------------------------------------------
//
//  Function:   GetNamesFromUrl
//
//-------------------------------------------------------------------------

HRESULT
GetNamesFromUrl(LPTSTR pchUrl, LPTSTR * ppchShortName, LPTSTR * ppchLongName)
{
    HRESULT         hr = S_OK;
    URL_COMPONENTS  uc;
    TCHAR           achPath[pdlUrlLen];
    TCHAR           achHost[pdlUrlLen];
    LPTSTR          pchFile;

    Assert (ppchShortName && ppchLongName);

    ZeroMemory(&uc, sizeof(uc));
    uc.dwStructSize = sizeof(uc);

    uc.lpszUrlPath      = achPath;
    uc.dwUrlPathLength  = ARRAY_SIZE(achPath);
    uc.lpszHostName     = achHost;
    uc.dwHostNameLength = ARRAY_SIZE(achHost);

    if (InternetCrackUrl(pchUrl, _tcslen(pchUrl), 0, &uc))
    {
        pchFile = _tcsrchr(achPath, _T('/'));
        if (!pchFile)
            pchFile = _tcsrchr (achPath, _T('\\'));

        *ppchShortName = pchFile ? (pchFile + 1) : achPath;

        // if we have no path use the host name
        if (!(*ppchShortName)[0])
            *ppchShortName = achHost;
    }
    else
    {
        *ppchShortName = _T("Unknown");
    }

    // long name is the full URL
    *ppchLongName = pchUrl;

    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
//  Stateless helpers
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Helper:     GetScriptDebugDocument
//
//-------------------------------------------------------------------------

HRESULT
GetScriptDebugDocument(CBase * pSourceObject, CScriptDebugDocument ** ppScriptDebugDocument)
{
    HRESULT             hr = S_OK;
    CScriptElement *    pScriptElement;
    CMarkup *           pMarkup;

    Assert (ppScriptDebugDocument);

    *ppScriptDebugDocument = NULL;

    CrackSourceObject(pSourceObject, &pMarkup, &pScriptElement);

    if (pMarkup && pMarkup->HasScriptContext())
    {
        *ppScriptDebugDocument = pMarkup->ScriptContext()->_pScriptDebugDocument;
    }
    else if (pScriptElement)
    {
        *ppScriptDebugDocument = pScriptElement->_pScriptDebugDocument;
    }

    RRETURN (hr);
}

///////////////////////////////////////////////////////////////////////////
//
//  Class:      CScriptCookieTable
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CScriptCookieTable::CreateCookieForSourceObject
//
//-------------------------------------------------------------------------

HRESULT
CScriptCookieTable::CreateCookieForSourceObject (DWORD * pdwCookie, CBase * pSourceObject)
{
    HRESULT     hr;

    Assert (pdwCookie && pSourceObject);

    // we may generate 0 cookie so make sure it is not treated as NO_SOURCE_CONTEXT
    Assert (0 != NO_SOURCE_CONTEXT);

    *pdwCookie = _aryItems.Size();

    hr = THR(MapCookieToSourceObject(*pdwCookie, pSourceObject));

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CScriptCookieTable::MapCookieToSourceObject
//
//-------------------------------------------------------------------------

HRESULT
CScriptCookieTable::MapCookieToSourceObject(DWORD dwCookie, CBase * pSourceObject)
{
    HRESULT                 hr = S_OK;
    CItem *                 pItem;
    CScriptElement *        pScriptElement;
    CMarkup *               pMarkup;
    CMarkupScriptContext *  pMarkupScriptContext;

    Assert (NO_SOURCE_CONTEXT != dwCookie && pSourceObject);

    // assert that the cookies are unique, except when for markups
    Assert (NULL == FindItem(dwCookie) || GetMarkupFromBase(pSourceObject));

    pItem = _aryItems.Append();
    if (!pItem)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pItem->_dwCookie = dwCookie;

    CrackSourceObject(pSourceObject, &pMarkup, &pScriptElement);

    if (pScriptElement)
    {
        pItem->_type = ITEMTYPE_SCRIPTELEMENT;
        pItem->_pScriptElement = (CScriptElement*)pSourceObject;
        pItem->_pScriptElement->_dwScriptCookie = pItem->_dwCookie;
    }
    else
    {
        Assert (pMarkup);

        hr = THR(pMarkup->EnsureScriptContext(&pMarkupScriptContext));
        if (hr)
            goto Cleanup;

        if (NO_SOURCE_CONTEXT == pMarkupScriptContext->_dwScriptCookie)
        {
            pItem->_type        = ITEMTYPE_MARKUP;
            pItem->_pMarkup     = pMarkup;

            pMarkupScriptContext->_dwScriptCookie = pItem->_dwCookie;
        }
        else
        {
            pItem->_type        = ITEMTYPE_REF;
            pItem->_dwCookieRef = pMarkupScriptContext->_dwScriptCookie;
        }
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CScriptCookieTable::FindItem
//
//-------------------------------------------------------------------------

CScriptCookieTable::CItem *
CScriptCookieTable::FindItem(DWORD dwCookie, CBase * pSourceObject)
{
    CItem *     pItem;
    int         c;

    //
    // optimization: first, try at index specified by the cookie.
    // (when we get to generate cookies, we generate them so that they are indexes in the array)
    //

    if (0 <= dwCookie && dwCookie < (DWORD)_aryItems.Size())
    {
        pItem = &_aryItems[dwCookie];

        if (pItem->IsMatch(dwCookie, pSourceObject))
            return pItem;
    }

    //
    // if failed to find, make the normal search
    //

    for (pItem = _aryItems, c = _aryItems.Size(); c; c--, pItem++)
    {
        if (pItem->IsMatch(dwCookie, pSourceObject))
            return pItem;
    }

    return NULL;
}

//+------------------------------------------------------------------------
//
//  Member:     CScriptCookieTable::FindItemDerefed
//
//-------------------------------------------------------------------------

CScriptCookieTable::CItem *
CScriptCookieTable::FindItemDerefed(DWORD dwCookie, CBase * pSourceObject)
{
    CItem * pItem = FindItem(dwCookie, pSourceObject);

    if (pItem && ITEMTYPE_REF == pItem->_type)
    {
        pItem = FindItem(pItem->_dwCookieRef, pSourceObject);

        Assert (ITEMTYPE_MARKUP == pItem->_type); // we use refs to markups only, and no refs to refs
    }

    return pItem;
}
//+------------------------------------------------------------------------
//
//  Member:     CScriptCookieTable::GetSourceObjects
//
//-------------------------------------------------------------------------

HRESULT
CScriptCookieTable::GetSourceObjects(
    DWORD                   dwCookie,
    CMarkup **              ppMarkup,
    CScriptElement **       ppScriptElement,
    CScriptDebugDocument ** ppScriptDebugDocument)
{
    HRESULT             hr = S_OK;
    CScriptElement *    pScriptElement;
    CItem *             pItem= FindItemDerefed(dwCookie);

    Assert (ppMarkup);

    if (!ppScriptElement)
        ppScriptElement = &pScriptElement;

    (*ppMarkup)        = NULL;
    (*ppScriptElement) = NULL;

    if (!pItem)
        goto Cleanup;

    switch (pItem->_type)
    {
    case ITEMTYPE_NULL:
            break;

    case ITEMTYPE_SCRIPTELEMENT:
            (*ppScriptElement) = pItem->_pScriptElement;
            Assert (*ppScriptElement);
            break;

    case ITEMTYPE_MARKUP:
            (*ppMarkup) = pItem->_pMarkup;
            Assert (*ppMarkup);
            break;

    default:
            Assert (FALSE);
            break;
    }

    if ((*ppScriptElement) && (*ppScriptElement)->IsInMarkup())
    {
        *ppMarkup = (*ppScriptElement)->GetMarkup();
    }

    if (ppScriptDebugDocument)
    {
        hr = THR(GetScriptDebugDocument(pItem, ppScriptDebugDocument));
    }

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CScriptCookieTable::GetScriptDebugDocument
//
//-------------------------------------------------------------------------

HRESULT
CScriptCookieTable::GetScriptDebugDocument(
    CItem *                 pItem,
    CScriptDebugDocument ** ppScriptDebugDocument)
{
    HRESULT     hr = S_OK;

    Assert (pItem && ppScriptDebugDocument);

    *ppScriptDebugDocument = NULL;

    switch (pItem->_type)
    {
    case ITEMTYPE_SCRIPTELEMENT:
            *ppScriptDebugDocument = pItem->_pScriptElement->_pScriptDebugDocument;
            break;

    case ITEMTYPE_MARKUP:
            *ppScriptDebugDocument = pItem->_pMarkup->HasScriptContext() ?
                pItem->_pMarkup->ScriptContext()->_pScriptDebugDocument : NULL;
            break;
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CScriptCookieTable::GetScriptDebugDocument
//
//-------------------------------------------------------------------------

HRESULT
CScriptCookieTable::GetScriptDebugDocument (
    DWORD                   dwCookie,
    CScriptDebugDocument ** ppScriptDebugDocument)
{
    HRESULT             hr = S_OK;
    CItem *             pItem = FindItemDerefed(dwCookie);

    if (!pItem)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(GetScriptDebugDocument(pItem, ppScriptDebugDocument));

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CScriptCookieTable::RevokeSourceObject
//
//-------------------------------------------------------------------------

HRESULT
CScriptCookieTable::RevokeSourceObject(DWORD dwCookie, CBase * pSourceObject)
{
    HRESULT     hr = S_OK;
    CItem *     pItem = FindItem(dwCookie, pSourceObject);

    if (pItem)
    {
        // assert that we do not revoke same thing twice 
        Assert (ITEMTYPE_NULL != pItem->_type);

        pItem->_type = ITEMTYPE_NULL;
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CScriptCookieTable::GetScriptDebugDocumentContext
//
//-------------------------------------------------------------------------

HRESULT
CScriptCookieTable::GetScriptDebugDocumentContext(
    DWORD                       dwCookie,
    ULONG                       uCharacterOffset,
    ULONG                       uNumChars,
    IDebugDocumentContext **    ppDebugDocumentContext)
{
    HRESULT                 hr;
    CScriptDebugDocument *  pScriptDebugDocument;

    if (!ppDebugDocumentContext)
        RRETURN(E_POINTER);

    hr = THR(GetScriptDebugDocument(dwCookie, &pScriptDebugDocument));
    if (hr)
        goto Cleanup;

    Assert (pScriptDebugDocument);

    hr = THR(pScriptDebugDocument->GetDocumentContext(dwCookie, uCharacterOffset, uNumChars, ppDebugDocumentContext));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

///////////////////////////////////////////////////////////////////////////
//
//  Class:      CScriptDebugDocument
//
///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CScriptDebugDocument::CreateScriptDebugDocument, static helper
//
//-------------------------------------------------------------------------

HRESULT
CScriptDebugDocument::Create(CCreateInfo * pInfo, CScriptDebugDocument ** ppScriptDebugDocument)
{
    HRESULT                 hr = S_OK;

    Assert (ppScriptDebugDocument);

    *ppScriptDebugDocument = NULL;

    if (
#ifndef NO_SCRIPT_DEBUGGER
		!g_pPDM ||
        !g_pDebugApp ||
#endif
        !pInfo->_pDoc->_pScriptCollection)
        goto Cleanup;

    *ppScriptDebugDocument = new CScriptDebugDocument();
    if (!(*ppScriptDebugDocument))
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR((*ppScriptDebugDocument)->Init(pInfo));

Cleanup:

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CScriptDebugDocument constructor
//
//-------------------------------------------------------------------------

CScriptDebugDocument::CScriptDebugDocument()
{
#if DBG == 1
    _Host._pScriptDebugDocumentDbg = this;
#endif
}

//+------------------------------------------------------------------------
//
//  Member:     CScriptDebugDocument destructor
//
//-------------------------------------------------------------------------

CScriptDebugDocument::~CScriptDebugDocument()
{
}

//+------------------------------------------------------------------------
//
//  Member:     CScriptDebugDocument::Init
//
//-------------------------------------------------------------------------

HRESULT
CScriptDebugDocument::Init(CCreateInfo * pInfo)
{
    HRESULT                 hr = S_OK;
    LPTSTR                  pchShortName;
    LPTSTR                  pchLongName;
    IDebugDocumentHelper *  pParentDebugHelper;
    CMarkup *               pParentMarkup;

    //
    // setup data
    //

    _dwThreadId = pInfo->_pDoc->_dwTID;

    hr = THR(_cstrUrl.Set(pInfo->_pchUrl));
    if (hr)
        goto Cleanup;

    //
    // pInfo->_pMarkupHtmCtx or pInfo->_pchSource may be necessary in methods called from threads
    // other then main doc's thread. To avoid free-threading issues with access to the data,
    // we take it local (pInfo->_pchSource) or keep it SubAddRefed (pInfo->_pMarkupHtmCtx) for lifetime.
    // SubAddRef on HtmCtx is enough because it is already free-threaded.
    //

    if (pInfo->_pMarkupHtmCtx)
    {
        Assert (!pInfo->_pchScriptElementCode);

        _pMarkupHtmCtx = pInfo->_pMarkupHtmCtx;
        _pMarkupHtmCtx->SubAddRef();
    }
    else
    {
        Assert (pInfo->_pchScriptElementCode);

        hr = THR(_cstrScriptElementCode.Set(pInfo->_pchScriptElementCode));
        if (hr)
            goto Cleanup;
    }

    //
    // create and initialize
    //
#ifndef NO_SCRIPT_DEBUGGER
    hr = THR(g_pPDM->CreateDebugDocumentHelper (0, &_pDebugHelper));
    if (hr)
#endif
        goto Cleanup;

    GetDebugHelper(NULL);   // balanced by ReleaseDebugHelper in Passivate

    hr = THR(GetNamesFromUrl(_cstrUrl, &pchShortName, &pchLongName));
    if (hr)
        goto Cleanup;

#ifndef NO_SCRIPT_DEBUGGER
    hr = THR(_pDebugHelper->Init( g_pDebugApp, pchShortName, pchLongName, 0));
    if (hr)
#endif
        goto Cleanup;

    hr = THR(_pDebugHelper->SetDebugDocumentHost (&_Host));
    if (hr)
        goto Cleanup;

    //
    // set parent
    //

    pParentDebugHelper = NULL;

    pParentMarkup = pInfo->_pDoc->_pDocParent ? pInfo->_pDoc->_pDocParent->PrimaryMarkup() : NULL;

    if (pParentMarkup)
    {
        CMarkupScriptContext * pScriptContext = pParentMarkup->ScriptContext();

        if (pScriptContext && pScriptContext->_pScriptDebugDocument)
        {
            pParentDebugHelper = pScriptContext->_pScriptDebugDocument->_pDebugHelper;
        }
    }

    hr = THR(_pDebugHelper->Attach(pParentDebugHelper));
    if (hr)
        goto Cleanup;

Cleanup:

    if (hr && _pDebugHelper)
    {
        ReleaseDebugHelper(NULL);
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CScriptDebugDocument::Passivate
//
//-------------------------------------------------------------------------

void
CScriptDebugDocument::Passivate()
{
    ReleaseDebugHelper(NULL);   // to balance GetDebugHelper(NULL) in Init

    if (_pMarkupHtmCtx)
    {
        _pMarkupHtmCtx->SubRelease();
        _pMarkupHtmCtx = NULL;
    }

    super::Passivate();
}

//+------------------------------------------------------------------------
//
//  Member:     CScriptDebugDocument::GetDebugHelper
//
//-------------------------------------------------------------------------

void
CScriptDebugDocument::GetDebugHelper (IDebugDocumentHelper ** ppDebugHelper)
{
    _DebugHelperCriticalSection.Enter();

    if (_pDebugHelper)
    {
        _cDebugHelperAccesses++;

        if (ppDebugHelper)
        {
            *ppDebugHelper = _pDebugHelper;
            _pDebugHelper->AddRef();
        }
    }

    _DebugHelperCriticalSection.Leave();
}

//+------------------------------------------------------------------------
//
//  Member:     CScriptDebugDocument::ReleaseDebugHelper
//
//-------------------------------------------------------------------------

void
CScriptDebugDocument::ReleaseDebugHelper(IDebugDocumentHelper * pDebugHelper)
{
    IDebugDocumentHelper *  pDebugHelperToDetach = NULL;

    _DebugHelperCriticalSection.Enter();

    if (_pDebugHelper)
    {
        Assert(_cDebugHelperAccesses);
    
        _cDebugHelperAccesses--;

        if (0 == _cDebugHelperAccesses)
        {
            pDebugHelperToDetach = _pDebugHelper;
            if (pDebugHelperToDetach)
            {
                pDebugHelperToDetach->AddRef();
                ClearInterface(&_pDebugHelper);
            }
        }
    }

    _DebugHelperCriticalSection.Leave();

    if (pDebugHelperToDetach)
    {
        IGNORE_HR(pDebugHelperToDetach->Detach());
        pDebugHelperToDetach->Release();
    }

    ReleaseInterface (pDebugHelper);
}

//+------------------------------------------------------------------------
//
//  Member:     CScriptDebugDocument::CreateDebugContext
//
//-------------------------------------------------------------------------

HRESULT
CScriptDebugDocument::DefineScriptBlock(
    IActiveScript *     pActiveScript, 
    ULONG               ulOffset, 
    ULONG               ulCodeLen, 
    BOOL                fScriptlet, 
    DWORD *             pdwScriptCookie)
{
    HRESULT     hr;

    Assert (pActiveScript && pdwScriptCookie);
    Assert (_pDebugHelper);

    *pdwScriptCookie = NO_SOURCE_CONTEXT;

    hr = THR(_pDebugHelper->DefineScriptBlock(ulOffset, ulCodeLen, pActiveScript, fScriptlet, pdwScriptCookie));

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member:     CScriptDebugDocument::IllegalCall
//
//---------------------------------------------------------------------------

BOOL
CScriptDebugDocument::IllegalCall()
{
    if (_dwThreadId != GetCurrentThreadId())
    {
        AssertSz(FALSE, "Script debugger called across thread boundry (not an MSHTML bug)");
        return TRUE;
    }

    return FALSE;
}

//---------------------------------------------------------------------------
//
//  Member:     CScriptDebugDocument::GetDocumentContext
//
//---------------------------------------------------------------------------

HRESULT
CScriptDebugDocument::GetDocumentContext(
    DWORD                       dwCookie,
    ULONG                       uCharacterOffset,
    ULONG                       uNumChars,
    IDebugDocumentContext **    ppDebugDocumentContext)
{
    HRESULT                 hr;
    ULONG                   ulStartOffset;
    IDebugDocumentHelper *  pDebugHelper = NULL;

    GetDebugHelper(&pDebugHelper);
    if (!pDebugHelper)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }
    
    hr = THR(pDebugHelper->GetScriptBlockInfo(dwCookie, NULL, &ulStartOffset, NULL));
    if (FAILED(hr))
        goto Cleanup;

    hr = THR(pDebugHelper->CreateDebugDocumentContext(
        ulStartOffset + uCharacterOffset, uNumChars, ppDebugDocumentContext)); 

Cleanup:

    ReleaseDebugHelper(pDebugHelper);

    RRETURN (hr);
}

//---------------------------------------------------------------------------
//
//  Member:     CScriptDebugDocument::SetDocumentSize
//
//---------------------------------------------------------------------------

HRESULT
CScriptDebugDocument::SetDocumentSize(ULONG ulSize)
{
    HRESULT     hr = S_OK;

    hr = THR(RequestDocumentSize(ulSize));
    if (hr)
        goto Cleanup;

    hr = THR(UpdateDocumentSize());

Cleanup:
    RRETURN (hr);
}

//---------------------------------------------------------------------------
//
//  Member:     CScriptDebugDocument::RequestDocumentSize
//
//---------------------------------------------------------------------------

HRESULT
CScriptDebugDocument::RequestDocumentSize(ULONG ulSize)
{
    HRESULT     hr = S_OK;

    TraceTag((
        tagDebuggerDocumentSize,
        "CScriptDebugDocument::RequestDocumentSize: url: %ls, current size = %ld, requested size = %ld",
        (LPTSTR)_cstrUrl, _ulCurrentSize, ulSize));

    _ulNewSize = max (_ulCurrentSize, ulSize);

    RRETURN (hr);
}

//---------------------------------------------------------------------------
//
//  Member:     CScriptDebugDocument::UpdateDocumentSize
//
//---------------------------------------------------------------------------

HRESULT
CScriptDebugDocument::UpdateDocumentSize()
{
    HRESULT     hr = S_OK;

    TraceTag((
        tagDebuggerDocumentSize,
        "CScriptDebugDocument::UpdateDocumentSize: url: %ls, current size = %ld, new size = %ld",
        (LPTSTR)_cstrUrl, _ulCurrentSize, _ulNewSize));

    if (_ulCurrentSize < _ulNewSize)
    {
#ifndef NO_SCRIPT_DEBUGGER
        hr = THR(_pDebugHelper->AddDeferredText (_ulNewSize - _ulCurrentSize, _ulCurrentSize));
#endif

        _ulCurrentSize = _ulNewSize;
    }

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CScriptDebugDocument::ViewSourceInDebugger
//
//----------------------------------------------------------------------------

HRESULT
CScriptDebugDocument::ViewSourceInDebugger (ULONG ulLine, ULONG ulOffsetInLine)
{
    HRESULT                 hr = S_OK;
    ULONG                   ulLineOffset = 0;
    IDebugDocumentText *    pDebugDocumentText = NULL;
    IDebugDocumentContext * pDebugDocumentContext = NULL;

    hr = THR(UpdateDocumentSize());
    if (hr)
        goto Cleanup;

    if (ulLine || ulOffsetInLine)
    {
        hr = THR(_pDebugHelper->QueryInterface(IID_IDebugDocumentText, (void**)&pDebugDocumentText));
        if(hr)
            goto Cleanup;

        hr = THR(pDebugDocumentText->GetPositionOfLine(ulLine, &ulLineOffset));
        if(hr)
            goto Cleanup;
    }

    hr = THR(_pDebugHelper->CreateDebugDocumentContext(ulLineOffset + ulOffsetInLine, 0, &pDebugDocumentContext));
    if (hr)
        goto Cleanup;

    hr = THR(_pDebugHelper->BringDocumentContextToTop (pDebugDocumentContext));

Cleanup:

    ReleaseInterface(pDebugDocumentText);
    ReleaseInterface(pDebugDocumentContext);

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member:     CScriptDebugDocument::CHost::QueryInterface, per IUnknown
//
//---------------------------------------------------------------------------

//..todo move it to core\include\qi_impl.h
#define Data1_IDebugDocumentHost 0x51973C27

HRESULT
CScriptDebugDocument::CHost::QueryInterface(REFIID iid, void **ppv)
{
    if (SDD()->IllegalCall())
        RRETURN(E_NOINTERFACE);

    if (!ppv)
        RRETURN(E_POINTER);

    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS(this, IUnknown)
    QI_INHERITS(this, IDebugDocumentHost)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    RRETURN(E_NOINTERFACE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CScriptDebugDocument::CHost::GetDeferredText, per IDebugDocumentHost
//
//----------------------------------------------------------------------------

HRESULT
CScriptDebugDocument::CHost::GetDeferredText(
    DWORD               dwStart,
    WCHAR *             pchText,
    SOURCE_TEXT_ATTR *  ,
    ULONG *             pcTextLen,
    ULONG               cMaxTextLen)
{
    HRESULT     hr = S_OK;

    if (!pchText || !pcTextLen)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (SDD()->_pMarkupHtmCtx)
    {
        hr = THR(SDD()->_pMarkupHtmCtx->ReadUnicodeSource(pchText, dwStart, cMaxTextLen, pcTextLen));
    }
    else
    {
        Assert (!SDD()->_cstrScriptElementCode.IsNull());

        if (dwStart < 0 || SDD()->_cstrScriptElementCode.Length() <= dwStart ||
            cMaxTextLen < 0)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        *pcTextLen = min ((ULONG)(SDD()->_cstrScriptElementCode.Length() - dwStart), cMaxTextLen - 1);

        _tcsncpy(pchText, SDD()->_cstrScriptElementCode, *pcTextLen);

        pchText[*pcTextLen] = 0;
    }

Cleanup:

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CScriptDebugDocument::CHost::GetPathName, per IDebugDocumentHost
//
//----------------------------------------------------------------------------

HRESULT
CScriptDebugDocument::CHost::GetPathName(BSTR * pbstrLongName, BOOL * pfIsOriginalFile)
{
    HRESULT         hr = E_NOTIMPL;

    Assert (FALSE && "not implemeneted; not supposed to be called");

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CScriptDebugDocument::CHost::GetFileName, per IDebugDocumentHost
//
//----------------------------------------------------------------------------

HRESULT
CScriptDebugDocument::CHost::GetFileName(BSTR * pbstrShortName)
{
    HRESULT         hr = E_NOTIMPL;

    Assert (FALSE && "not implemeneted; not supposed to be called");

    RRETURN (hr);
}

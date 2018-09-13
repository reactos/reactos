#include "headers.hxx"

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_XMLNS_HXX_
#define X_XMLNS_HXX_
#include "xmlns.hxx"
#endif

//////////////////////////////////////////////////////////////////////////////
//
//  misc
//
//////////////////////////////////////////////////////////////////////////////

MtDefine(CMarkupBehaviorContext, CMarkup, "CMarkupBehaviorContext")

DeclareTag(tagPeerCMarkupIsGenericElement, "Peer", "trace CMarkup::IsGenericElement")

//////////////////////////////////////////////////////////////////////////////
//
//  Class:      CMarkupBehaviorContext
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
//  Member:     CMarkupBehaviorContext constructor
//
//----------------------------------------------------------------------------

CMarkupBehaviorContext::CMarkupBehaviorContext()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkupBehaviorContext destructor
//
//----------------------------------------------------------------------------

CMarkupBehaviorContext::~CMarkupBehaviorContext()
{
    if (_pXmlNamespaceTable)
        _pXmlNamespaceTable->Release();
}

//////////////////////////////////////////////////////////////////////////////
//
//  Class:      CMarkup
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::ProcessPeerTask
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::ProcessPeerTask(PEERTASK task)
{
    HRESULT     hr = S_OK;

    switch (task)
    {
    case PEERTASK_MARKUP_RECOMPUTEPEERS_UNSTABLE:

        PrivateAddRef();

        IGNORE_HR(_pDoc->PeerEnqueueTask(this, PEERTASK_MARKUP_RECOMPUTEPEERS_STABLE));

        break;

    case PEERTASK_MARKUP_RECOMPUTEPEERS_STABLE:

        if (!_pDoc->TestLock(FORMLOCK_UNLOADING))
        {
            IGNORE_HR(RecomputePeers());
        }

        PrivateRelease(); // the markup may passivate after this call

        break;
    }

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::RecomputePeers
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::RecomputePeers()
{
    HRESULT     hr = S_OK;

    if (_pDoc->_fPeersPossible)
    {
        // BUGBUG (alexz: enable this for IE6 after CStyleSheet::OnDwnChan is fixed not to assume that it a safe moment)
        // AssertSz(!__fDbgLockTree, "CMarkup::RecomputePeers appears to be called at an unsafe moment of time");

        CNotification   nf;
        nf.RecomputeBehavior(Root());
        Notify(&nf);
    }

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::EnsureBehaviorContext
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::EnsureBehaviorContext(CMarkupBehaviorContext ** ppBehaviorContext)
{
    HRESULT                     hr = S_OK;
    CMarkupBehaviorContext *    pBehaviorContext;

    if (!HasBehaviorContext())
    {
        pBehaviorContext = new CMarkupBehaviorContext();
        if (!pBehaviorContext)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(SetBehaviorContext(pBehaviorContext));
        if (hr)
            goto Cleanup;

        if (ppBehaviorContext)
        {
            *ppBehaviorContext = pBehaviorContext;
        }
    }
    else
    {
        if (ppBehaviorContext)
        {
            *ppBehaviorContext = BehaviorContext();
        }
    }

Cleanup:

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::IsGenericElement
//
//----------------------------------------------------------------------------

ELEMENT_TAG
#ifdef VSTUDIO7
CMarkup::IsGenericElement(LPTSTR pchFullName, BOOL fAllSprinklesGeneric, BOOL *pfDerived /* = NULL */)
#else
CMarkup::IsGenericElement(LPTSTR pchFullName, BOOL fAllSprinklesGeneric)
#endif //VSTUDIO7
{
    ELEMENT_TAG     etag = ETAG_UNKNOWN;
    LPTSTR          pchColon;

    pchColon = StrChr (pchFullName, _T(':'));

    if (pchColon)
    {
        CStringNullTerminator   nullTerminator(pchColon);

        //
        // is it a valid sprinkle ?
        //

        etag = IsXmlSprinkle(/* pchNamespace = */pchFullName);
        if (ETAG_UNKNOWN != etag)
            goto Cleanup;
    }

    //
    // delegate to doc (host provided behaviors?, builtin behaviors?, etc)
    //

#ifdef VSTUDIO7
    etag = _pDoc->IsGenericElement(pchFullName, pchColon, pfDerived);
#else
    etag = _pDoc->IsGenericElement(pchFullName, pchColon);
#endif //VSTUDIO7
    if (ETAG_UNKNOWN != etag)
        goto Cleanup;

    if (pchColon)
    {
        // (NOTE: these checks should be in the very end - we might need to return ETAG_GENERIC_BUILTIN)

        //
        // all sprinkles are generic elements within context of the current operation?
        //

        if (fAllSprinklesGeneric)
        {
            etag = ETAG_GENERIC;
            goto Cleanup;           // done
        }

        //
        // markup services parsing? any sprinkle goes as generic element
        //

        if (_pDoc->_fMarkupServicesParsing)
        {
            etag = ETAG_GENERIC;
            goto Cleanup;           // done
        }
    }

Cleanup:

    TraceTag((tagPeerCMarkupIsGenericElement,
              "CMarkup::IsGenericElement, name <%ls> recognized as %ls",
              pchFullName,
              ETAG_UNKNOWN == etag ? _T("UNKNOWN") :
                ETAG_GENERIC_BUILTIN == etag ? _T("GENERIC_BUILTIN") : _T("GENERIC")));

    return etag;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::EnsureXmlNamespaceTable
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::EnsureXmlNamespaceTable(CXmlNamespaceTable ** ppXmlNamespaceTable)
{
    HRESULT                     hr;
    CMarkupBehaviorContext *    pContext;

    hr = THR(EnsureBehaviorContext(&pContext));
    if (hr)
        goto Cleanup;

    if (!pContext->_pXmlNamespaceTable)
    {
        pContext->_pXmlNamespaceTable = new CXmlNamespaceTable(_pDoc);
        if (!pContext->_pXmlNamespaceTable)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(pContext->_pXmlNamespaceTable->Init());
        if (hr)
            goto Cleanup;
    }

    if (ppXmlNamespaceTable)
    {
        *ppXmlNamespaceTable = pContext->_pXmlNamespaceTable;
    }

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::RegisterXmlNamespace
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::RegisterXmlNamespace(LPTSTR pchNamespace, LPTSTR pchUrn, XMLNAMESPACETYPE namespaceType)
{
    HRESULT                 hr;
    CXmlNamespaceTable *    pNamespaceTable;

    hr = THR(EnsureXmlNamespaceTable(&pNamespaceTable));
    if (hr)
        goto Cleanup;

    hr = THR(pNamespaceTable->RegisterNamespace(pchNamespace, pchUrn, namespaceType));

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::IsXmlSprinkle
//
//----------------------------------------------------------------------------

ELEMENT_TAG
CMarkup::IsXmlSprinkle (LPTSTR pchNamespace)
{
    HRESULT                 hr;
    CXmlNamespaceTable *    pNamespaceTable;

    hr = THR(EnsureXmlNamespaceTable(&pNamespaceTable));
    if (hr)
        return ETAG_UNKNOWN;

    return pNamespaceTable->IsXmlSprinkle(pchNamespace);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::SaveXmlNamespaceAttrs
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::SaveXmlNamespaceAttrs (CStreamWriteBuff * pStreamWriteBuff)
{
    HRESULT                 hr = S_OK;
    CXmlNamespaceTable *    pNamespaceTable = GetXmlNamespaceTable();

    if (!pNamespaceTable)
        goto Cleanup;

    hr = THR(pNamespaceTable->SaveNamespaceAttrs(pStreamWriteBuff));
    
Cleanup:
    RRETURN (hr);
}


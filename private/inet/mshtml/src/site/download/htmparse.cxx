//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       htmparse.cxx
//
//  Contents:   Support for HTML parsing:  including
//
//              CElement
//              CParser
//              CHTMLParser
//              CLex
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_HTM_HXX_
#define X_HTM_HXX_
#include "htm.hxx"
#endif

#ifndef X_EBODY_HXX_
#define X_EBODY_HXX_
#include "ebody.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_ROOTCTX_HXX_
#define X_ROOTCTX_HXX_
#include "rootctx.hxx"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifdef VSTUDIO7
#ifndef X_PEERFACT_HXX_
#define X_PEERFACT_HXX_
#include "peerfact.hxx"
#endif
#endif //VSTUDIO7

// Trace tags
DeclareTag(tagParse,            "Dwn", "HtmParse: Detailed parsing trace");
MtDefine(CHtmParse, Dwn, "CHtmParse")
MtDefine(CHtmParse_aryContextStack_pv, CHtmParse, "CHtmParse::_aryContxtStack::_pv")
MtDefine(CHtmParse_aryPelEndParse_pv, CHtmParse, "CHtmParse::_aryPelEndParse::_pv")
MtDefine(CHtmParse_aryPelFrontier_pv, CHtmParse, "CHtmParse::_aryPelFrontier::_pv")
MtDefine(CHtmParse_aryFccl_pv, CHtmParse, "CHtmParse::_aryPelFrontier::_pv")
MtDefine(CHtmParsePrepareContainer_aetagStack_pv, Dwn, "CHtmParse::PrepareContainer aetagStack::_pv")
MtDefine(CHtmParseOpenContainer_aetagStack_pv, Dwn, "CHtmParse::OpenContainer aetagStack::_pv")
MtDefine(CHtmParseCloseContainer_apNodeStack_pv, Dwn, "CHtmParse::CloseContainer apNodeStack::_pv")
MtDefine(CHtmParseCloseContainer_apNodeClose_pv, Dwn, "CHtmParse::CloseContainer apNodeClose::_pv")

// Asserts specific to CHtmParse
#if DBG == 1
#define AssertNoneProhibited(etag) AssertNoneProhibitedImpl(etag)
#define AssertAllRequired(etag) AssertAllRequiredImpl(etag)
#define AssertNoEndContainees(element) AssertNoEndContaineesImpl(element)
#define AssertOnStack(element) AssertOnStackImpl(element)
#define AssertInsideContext(element) AssertInsideContextImpl(element)
#else
#define AssertNoneProhibited(etag)
#define AssertAllRequired(etag)
#define AssertNoEndContainees(element)
#define AssertOnStack(element)
#define AssertInsideContext(element)
#endif

#if DBG != 1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif

extern ELEMENT_TAG s_atagNull[];
extern ELEMENT_TAG s_atagEOFProhibited[];

HRESULT CreateHtmRootParseCtx(CHtmParseCtx **pphpxRoot, CMarkup *pMarkup);
HRESULT CreateHtmTopParseCtx(CHtmParseCtx **pphpx, CHtmParseCtx *phpxParent);

HRESULT CreateElement (
              ELEMENT_TAG   etag,
              CElement * *  ppElementResult,
              CDoc *        pDoc,
              CMarkup *     pMarkup,
              BOOL          fCreateAttrBag,
              BOOL *        pfDie );

//+------------------------------------------------------------------------
//
//  Member:     CreateElement
//
//  Synopsis:   Creates an element of type etag parented to pElementParent
//
//              Inits an empty AttrBag if asked to.
//
//-------------------------------------------------------------------------

HRESULT
CreateElement(ELEMENT_TAG   etag,
              CElement **   ppElementResult,
              CDoc *        pDoc,
              CMarkup *     pMarkup,
              BOOL          fCreateAttrBag,
              BOOL *        pfDie)
{
    CElement       *            pElement = NULL;
    const CTagDesc *            ptd;
    CHtmTag                     ht;
    HRESULT                     hr;

    if (!pfDie)
        pfDie = (BOOL*)&g_Zero;

    ptd = TagDescFromEtag(etag);
    if (!ptd)
        return E_FAIL;

    ht.Reset();
    ht.SetTag(etag);
    
#ifdef VSTUDIO7
    if (pDoc->_fHasIdentityPeerFactory)
        Assert(etag != ETAG_UNKNOWN);
#endif //VSTUDIO7

    hr = ptd->_pfnElementCreator(&ht, pDoc, &pElement);
    if (hr)
        goto Cleanup;

    if (*pfDie)
        goto Die;

    hr = THR(pElement->Init());
    if (hr)
        goto Cleanup;

    if (*pfDie)
        goto Die;

    if (fCreateAttrBag)
    {
        hr = THR(pElement->InitAttrBag(&ht));
        if (hr)
            goto Cleanup;

        if (*pfDie)
            goto Die;

    }

#ifdef VSTUDIO7
    if (pDoc->_fHasIdentityPeerFactory &&
        ht.IsDerivedTag() &&
        ht.GetTag() != ETAG_GENERIC)
    {
        hr = pElement->SetTagNameAndScope(&ht);
        if (hr)
            goto Cleanup;
        if (*pfDie)
            goto Die;
    }
#endif //VSTUDIO7

    {
        CElement::CInit2Context   context (&ht, pMarkup);

        hr = THR(pElement->Init2(&context));
        if (hr)
            goto Cleanup;
    }

    if (*pfDie)
        goto Die;

Cleanup:

    if (hr && pElement)
    {
        CElement::ClearPtr(&pElement);
    }
    
    *ppElementResult = pElement;

    RRETURN(hr);

Die:

    hr = E_ABORT;
    goto Cleanup;
}


//+------------------------------------------------------------------------
//
//  Member:     CreateElement
//
//  Synopsis:   Creates an element of type etag parented to pElementParent
//
//              Inits an empty AttrBag if asked to.
//
//-------------------------------------------------------------------------
HRESULT
CreateElement(CHtmTag *     pht,
              CElement **   ppElementResult,
              CDoc *        pDoc,
              CMarkup *     pMarkup,
              BOOL *        pfDie,
              DWORD         dwFlags)
{
    CElement       *pElement = NULL;
    const CTagDesc *ptd;
    HRESULT         hr;
#ifdef VSTUDIO7
    ELEMENT_TAG     etag = pht->GetTag();
#endif //VSTUDIO7

    if (!pfDie)
        pfDie = (BOOL*)&g_Zero;

    ptd = TagDescFromEtag(pht->GetTag());
    if (!ptd)
        return E_FAIL;

#ifdef VSTUDIO7
    if (pDoc->_fHasIdentityPeerFactory)
        Assert(etag != ETAG_UNKNOWN);
#endif //VSTUDIO7

    hr = ptd->_pfnElementCreator(pht, pDoc, &pElement);
    if (hr)
        goto Cleanup;
    
    if (*pfDie)
        goto Die;

    hr = THR(pElement->Init());
    if (hr)
        goto Cleanup;

    if (*pfDie)
        goto Die;

    hr = THR(pElement->InitAttrBag(pht));
    if (hr)
        goto Cleanup;

    if (*pfDie)
        goto Die;

#ifdef VSTUDIO7
    if (pDoc->_fHasIdentityPeerFactory &&
        pht->IsDerivedTag() &&
        pht->GetTag() != ETAG_GENERIC)
    {
        hr = pElement->SetTagNameAndScope(pht);
        if (hr)
            goto Cleanup;
        if (*pfDie)
            goto Die;
    }
#endif //VSTUDIO7

    {
        CElement::CInit2Context   context(pht, pMarkup, dwFlags);

        hr = THR(pElement->Init2(&context));
        if (hr)
            goto Cleanup;
    }

    if (*pfDie)
        goto Die;

Cleanup:

    if (hr && pElement)
    {
        CElement::ClearPtr(&pElement);
    }
    
    *ppElementResult = pElement;
    RRETURN(hr);

Die:

    hr = E_ABORT;
    goto Cleanup;
}


//+------------------------------------------------------------------------
//
//  Member:     CreateUnknownElement
//
//  Synopsis:   Creates an unknown element of type etag parented to
//              pElementParent
//
//              Inits an empty AttrBag if asked to.
//
//-------------------------------------------------------------------------

#ifdef _M_IA64
//$ WIN64: Why is there unreachable code in the retail build of this next function for IA64?
#pragma warning(disable:4702) /* unreachable code */
#endif

HRESULT
CreateUnknownElement(CHtmTag *pht,
              CElement **ppElementResult,
              CDoc *pDoc,
              BOOL *pfDie)
{
    CElement       *pElement = NULL;
    const CTagDesc *ptd;
    HRESULT         hr;

    ptd = TagDescFromEtag(ETAG_UNKNOWN);
    if (!ptd)
        return E_FAIL;

    hr = ptd->_pfnElementCreator(pht, pDoc, &pElement);
    if (hr)
        goto Cleanup;

    if (*pfDie)
        goto Die;

    hr = THR(pElement->Init());
    if (hr)
        goto Cleanup;

    if (*pfDie)
        goto Die;

    hr = THR(pElement->InitAttrBag(pht));
    if (hr)
        goto Cleanup;

    if (*pfDie)
        goto Die;

    {
        CElement::CInit2Context   context (pht, NULL);

        hr = THR(pElement->Init2(&context));
        if (hr)
            goto Cleanup;
    }

    if (*pfDie)
        goto Die;

Cleanup:

    if (hr && pElement)
        CElement::ClearPtr(&pElement);

    *ppElementResult = pElement;
    
    RRETURN(hr);

Die:

    hr = E_ABORT;
    goto Cleanup;
}

#ifdef _M_IA64
#pragma warning(default:4702) /* unreachable code */
#endif

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::dtor
//
//  Synopsis:   destructor
//
//-------------------------------------------------------------------------
CHtmParse::~CHtmParse()
{
    int i;

    delete _phpxExecute;

    delete _ctx._phpx;

    for (i = _aryContextStack.Size(); i;)
    {
        delete _aryContextStack[--i]._phpx;
    }

    _aryContextStack.DeleteAll();
    
    _aryPelFrontier.ReleaseAll();

    if (_pTagQueue)
        _pTagQueue->Release();
    
    if (_pelMerge)
        _pelMerge->Release();

    if (_pMergeTagQueue)
        _pMergeTagQueue->Release();

    _aryPelEndParse.ReleaseAll();
    
    if (_pMarkup)
        _pMarkup->Release();

#ifdef NOPARSEADDREF
    CTreeNode::ReleasePtr(_pNode);
#endif
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::Init
//
//  Synopsis:   Sets up parsing (and does the first Prepare).
//
//-------------------------------------------------------------------------

HRESULT
CHtmParse::Init(CDoc *pDoc, CMarkup *pMarkup, CTreeNode *pNode)
{
    HRESULT   hr;
    CHtmParseCtx *phpxRoot;
    CHtmParseCtx *phpxTop;
    
    Assert(pMarkup);

    Assert(!_pMarkup);
    
    if (pMarkup)
    {
        _pMarkup = pMarkup;
        _pMarkup->AddRef();
    }

    Assert(pDoc);
    _pDoc = pDoc;

    // Create and push the root context

    hr = THR(CreateHtmRootParseCtx(&phpxRoot, pMarkup));
    if (hr)
        goto Cleanup;

    _ctx._phpx = phpxRoot;
    _ctx._pelTop = NULL;

    hr = THR(_ctx._phpx->Init());
    if (hr)
        goto Cleanup;

    hr = THR(_ctx._phpx->Prepare());
    if (hr)
        goto Cleanup;
        
    // Set up the frontier pointer, AddRef the root element
    
    hr = THR(_aryPelFrontier.Append(pNode->Element()));
    if (hr)
        goto Cleanup;
        
    pNode->Element()->AddRef();

    _pNode = NULL;
    _cDepth = 1;

    hr = THR(_aryContextStack.AppendIndirect(&_ctx));
    if (hr)
        goto Cleanup;

    _ctx._phpx = NULL;
    _ctx._pelTop = NULL;

    // Create and push the top context

    hr = THR(CreateHtmTopParseCtx(&phpxTop, phpxRoot));
    if (hr)
        goto Cleanup;
        
    _ctx._phpx = phpxTop;
    _ctx._pelTop = pMarkup->Root();

    hr = THR(_ctx._phpx->Init());
    if (hr)
        goto Cleanup;

    hr = THR(_ctx._phpx->Prepare());
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::Prepare
//
//  Synopsis:   Prepares each context for possible parsing
//
//-------------------------------------------------------------------------
HRESULT CHtmParse::Prepare()
{
    int       c;
    CContext *pctx;
    HRESULT   hr = S_OK;

    Assert(_aryPelFrontier.Size());
    Assert(!_pNode);

    _pNode = _aryPelFrontier[_aryPelFrontier.Size() - 1]->GetLastBranch();
    if (!_pNode)
    {
        hr = E_ABORT;
        goto Cleanup;
    }

    if (_lVersionSafe != _pDoc->GetDocTreeVersion())
    {
        // If the tree has been changed, we need to drop the entire FindContainer cache
        // (For coherency it depends on nodes never being recycled because they're never
        // released during, which may not be true if tree versions don't match.)
        _aryFccl.DeleteAll();
        
        CElement **ppel;
        CTreePos *ptpRight;

        for (ptpRight = _pNode->GetEndPos(), c = _aryPelFrontier.Size(), ppel = _aryPelFrontier + c - 1;
             c;
             ptpRight = ptpRight->NextTreePos())
        {
            // To ensure that everything beyond the frontier is totally locked down, verify:

            // 1. The right number of treeposes follow the frontier
            // 2. All the treeposes after the frontier are end edges
            // 3. Nothing on the frontier is dead
            // 4. The nodes belong to the elements that were recorded and refed
            // 5. The nodes form a parent chain above the current node to the root

            // Any failures -> the frontier has changed, so abort the parse

            if (!ptpRight)
                break;

            //
            // It's okay to have pointers in the frontier
            //
            
            if (ptpRight->IsPointer())
                continue;

            //
            // It's okay to have empty, text id-less chunks-o-text
            //

            if (ptpRight->IsText() && ptpRight->Cch() == 0 && ptpRight->Sid() == 0)
                continue;
            
            if (!ptpRight->IsEndElementScope()                   ||
                ptpRight->Branch()->IsDead()                     ||
                ptpRight->Branch()->Element() != (*ppel)         ||
                (c == 1 ? ptpRight->Branch()->Parent() != NULL :
                          ptpRight->Branch()->Parent()->Element() != *(ppel - 1)))
            {
                break;
            }

            ppel -= 1, c -= 1;
        }

        if (c)
            hr = E_ABORT;

        // ... and verify 6. that there are no extra treeposes at the very end
        if (ptpRight)
            hr = E_ABORT;

        if (hr)
            goto Cleanup;
    }

    for (pctx = _aryContextStack, c = _aryContextStack.Size(); c; pctx++, c--)
    {
        Assert(pctx->_phpx);
        hr = THR(pctx->_phpx->Prepare());
        if (hr)
            goto Cleanup;
    }

    if (_ctx._phpx)
    {
        hr = THR(_ctx._phpx->Prepare());
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::Commit
//
//  Synopsis:   Commits each context for possible return to message loop
//
//-------------------------------------------------------------------------
HRESULT CHtmParse::Commit()
{
    int       c, size;
    CContext *pctx;
    HRESULT   hr = S_OK;
    CElement **ppel;

    if (_ctx._phpx)
    {
        hr = THR(_ctx._phpx->Commit());
        if (hr)
            goto Cleanup;

        for (c = _aryContextStack.Size(), pctx = (CContext *)_aryContextStack + c; c; c--)
        {
            pctx--;
            Assert(pctx->_phpx);
            hr = THR(pctx->_phpx->Commit());
            if (hr)
                goto Cleanup;
        }
    }

    CTreeNode *pNode;

    // Update the frontier

    size = _aryPelFrontier.Size();
    pNode = _pNode;

    if (size < _cDepth)
    {
        // Grow the frontier
        
        _aryPelFrontier.Grow(_cDepth);
        for (ppel = _aryPelFrontier + _cDepth - 1, c = _cDepth - size; c; ppel -= 1, c -= 1)
        {
            *ppel = pNode->Element();
            (*ppel)->AddRef();
            pNode = pNode->Parent();
        }
    }
    else if (size > _cDepth)
    {
        // Shrink the frontier
        
        for (ppel = _aryPelFrontier + size - 1, c = size - _cDepth; c; ppel -= 1, c -= 1)
        {
            (*ppel)->Release();
        }
        _aryPelFrontier.SetSize(_cDepth);
    }
    else
    {
        ppel = _aryPelFrontier + size - 1;
    }

    // Update the frontier from the end forward, stopping where it's already OK
    // (to avoid quadratric behavior, never iterate more than # of elementbegin/end)
    
    while (pNode)
    {
        Assert(ppel >= (CElement **)_aryPelFrontier);
        
        if (pNode->Element() == *ppel)
            break;
            
        (*ppel)->Release();
        *ppel = pNode->Element();
        (*ppel)->AddRef();

        ppel -= 1;
        pNode = pNode->Parent();
    }

    _lVersionSafe = _pDoc->GetDocTreeVersion();
    _pNode = NULL;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::Finish
//
//  Synopsis:   Finish each context for final return from parser
//
//-------------------------------------------------------------------------
HRESULT CHtmParse::Finish()
{
    HRESULT    hr = S_OK;

    if (!_fDone)
    {
        // Implicit children
        hr = THR(AddImplicitChildren(ETAG_NULL));
        if (hr)
            goto Cleanup;

        // Litctx
        if (_etagLitCtx)
        {
            hr = THR(PopHpx());
            if (hr)
                goto Cleanup;
                
            _etagLitCtx = ETAG_NULL;
        }
        
        while (_pNode)
        {
            hr = THR(EndElement(_pNode));
            if (hr)
                goto Cleanup;

            if (_phpxExecute && !_phpxExecute->_fExecuteOnEof)
            {
                delete _phpxExecute;
                _phpxExecute = NULL;
            }
        }

        _fDone = TRUE;
        _fIgnoreInput = TRUE;
    }

    Assert(!_aryContextStack.Size());
    
    // Frontier be gone

    _aryPelFrontier.ReleaseAll();
    
    // Finish and delete the root context

    hr = THR(_ctx._phpx->Finish());
    if (hr)
        goto Cleanup;
        
    delete _ctx._phpx;
    _ctx._phpx = NULL;
    Assert(!_pNode);

    if (_pMarkup)
    {
        _pMarkup->Release();
        _pMarkup = NULL;
    }

Cleanup:
    
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::Execute
//
//  Synopsis:   Executes the context which needs to be executed.
//
//-------------------------------------------------------------------------
HRESULT CHtmParse::Execute()
{
    HRESULT hr = S_OK;
    CElement **ppel;
    long c;

    if (_pMergeTagQueue)
    {
        Assert(!_pMergeTagQueue->IsEmpty());

        hr = THR(MergeTags());
        if (hr)
            goto Cleanup;
    }
    
    if (_phpxExecute)
    {
        CHtmParseCtx *phpx = _phpxExecute;
        _phpxExecute = NULL;
        Verify(!THR(phpx->Execute()));

        delete phpx;
    }

    for (ppel = _aryPelEndParse, c = _aryPelEndParse.Size(); c; ppel++, c--)
    {
        CNotification   nf;

        nf.EndParse(*ppel);
        (*ppel)->Notify(&nf);

        (*ppel)->Release();
    }
    
    _aryPelEndParse.DeleteAll();

Cleanup:

    RRETURN(hr);
}



//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::GetCurrentElement
//
//  Synopsis:   Obtains the current element
//
//-------------------------------------------------------------------------

CElement*
CHtmParse::GetCurrentElement()
{
    CElement *pElement;
        
    if (_pNode)
    {
        pElement = _pNode->Element();
    }
    else if (_aryPelFrontier.Size())
    {
        pElement = _aryPelFrontier[_aryPelFrontier.Size() - 1];
    }
    else
    {
        pElement = NULL;
    }

    return pElement;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::ParseToken
//
//  Synopsis:   Dispatches a token to the appropriate method based on etag
//
//-------------------------------------------------------------------------
HRESULT
CHtmParse::ParseToken(CHtmTag *pht)
{
    HRESULT hr;

    if (_fIgnoreInput)
        return S_OK;

    // Dispatch depending on token type:

    // TEXT

    if (pht->Is(ETAG_RAW_TEXT))
    {
        Assert(pht->GetCch());

        hr = THR(ParseText(pht->GetPch(), pht->GetCch(), pht->IsAscii()));
    }

    // TAGS

    else if (pht->GetTag() < ETAG_RAW_COMMENT)
    {
        if ((_ctx._phpx->_atagReject && !IsEtagInSet(pht->GetTag(), _ctx._phpx->_atagReject))
            || (_ctx._phpx->_atagAccept && IsEtagInSet(pht->GetTag(), _ctx._phpx->_atagAccept)) )
        {
            if (!pht->IsEnd())
            {
                hr = THR(ParseBeginTag(pht));
            }
            else
            {
                if (_ctx._phpx->_atagIgnoreEnd && IsEtagInSet(pht->GetTag(), _ctx._phpx->_atagIgnoreEnd))
                {
                    hr = THR(ParseIgnoredTag(pht));
                }
                else
                {
                    hr = THR(ParseEndTag(pht));
                }
            }
        }
        else
        {
            if (_ctx._phpx->_atagTag && IsEtagInSet(pht->GetTag(), _ctx._phpx->_atagTag))
            {
                hr = THR(_ctx._phpx->AddTag(pht));
            }
            else if (_ctx._phpx->_atagAlwaysEnd && IsEtagInSet(pht->GetTag(), _ctx._phpx->_atagAlwaysEnd))
            {
                hr = THR(ParseEndTag(pht));
            }
            else if (_ctx._phpx->_atagIgnoreEnd && pht->IsEnd() && IsEtagInSet(pht->GetTag(), _ctx._phpx->_atagIgnoreEnd))
            {
                hr = THR(ParseIgnoredTag(pht));
            }
            else
            {
                hr = THR(ParseUnknownTag(pht));
            }
        }
    }

    // COMMENTS

    else if (pht->Is(ETAG_RAW_COMMENT))
    {
        hr = THR(ParseComment(pht));
    }

    // RAW SOURCE
    
    else if (pht->Is(ETAG_RAW_SOURCE))
    {
        hr = THR(ParseSource(pht));
    }

    // PASTE MARKER
    
    else if (pht->Is(ETAG_RAW_BEGINSEL) || pht->Is(ETAG_RAW_ENDSEL))
    {
        hr = THR(ParseMarker(pht));
    }

    // TEXT FRAG

    else if (pht->Is(ETAG_RAW_TEXTFRAG))
    {
        hr = THR(ParseTextFrag(pht));
    }

    // EOF
    
    else if (pht->GetTag() == ETAG_RAW_EOF)
    {
        hr = THR(ParseEof());
    }

    else
    {
        AssertSz(0, "Parser was given an unrecognized token");
        hr = S_OK;
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::ParseMarker
//
//  Synopsis:   Process a marker. Markers get queued if there is a queue.
//
//-------------------------------------------------------------------------
HRESULT
CHtmParse::ParseMarker(CHtmTag *pht)
{
    HRESULT hr;
    
    if (_pTagQueue)
    {
        Assert(_etagReplay);
        hr = THR(QueueTag(pht));
        goto Cleanup;
    }
    
    switch (pht->GetTag())
    {
    case ETAG_RAW_BEGINSEL:
        hr = THR(InsertLPointer());
        break;
        
    case ETAG_RAW_ENDSEL:
        hr = THR(InsertRPointer());
        break;

    default:
        hr = S_OK;
        Assert(0);
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::ParseTextFrag
//
//  Synopsis:   Process a text frag.  Right now only conditional comments
//              generate text frags.
//
//-------------------------------------------------------------------------
HRESULT
CHtmParse::ParseTextFrag(CHtmTag *pht)
{
    HRESULT hr;
    
    hr =  ((CHtmRootParseCtx *) _aryContextStack[0]._phpx)->InsertTextFrag(pht->GetPch(), pht->GetCch(), _pNode);

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::ParseText
//
//  Synopsis:   Process the text in pht.
//
//-------------------------------------------------------------------------
HRESULT
CHtmParse::ParseText(TCHAR * pch, ULONG cch, BOOL fAscii)
{
    ELEMENT_TAG etag = ETAG_NULL;
    HRESULT hr = S_OK;

    // Step 1: Insert required container if any

    if (!_etagLitCtx)
    {
        etag = _fValidRTC ? _etagRTC : RequiredTextContainer();

        if (etag)
        {
            if (cch && ISSPACE(*pch))
            {
                TCHAR * pchScan = pch + 1;
                TCHAR * pchLast = pch + cch;
                while (pchScan < pchLast && ISSPACE(*pchScan))
                    ++pchScan;
                cch -= (LONG)(pchScan - pch);

                if (cch == 0)
                {
                    // Entire text was spaces, just eat it.
                    return(S_OK);
                }

                pch  = pchScan;
            }

            CHtmlParseClass *phpc;

            phpc = HpcFromEtag(etag);
            if (phpc->_atagProhibitedContainers)
            {
                hr = THR(CloseAllContainers(phpc->_atagProhibitedContainers, phpc->_atagBeginContainers));
                if (hr)
                    goto Cleanup;
            }

            hr = THR(OpenContainer(etag));
            if (hr == S_FALSE)
            {
                AssertSz(0,"Required text container could not be inserted (DTD error)");
                hr = S_OK;
                goto Cleanup;
            }
            else if (hr)
                goto Cleanup;

            etag = ETAG_NULL;
        }

        // Before actually inserting any text, add any tags that have been queued
        
        if (_fImplicitChild || _etagReplay)
        {
            hr = THR(AddImplicitChildren(ETAG_NULL));
            if (hr)
                goto Cleanup;
        }
        
    }

    // Step 2: Insert deferred paste pointer
    
    Assert(!etag);

    if (_fDelayPointer)
    {
        hr = InsertLPointerNow();
        if (hr)
            goto Cleanup;
    }
        
    // Step 3: Send text to context (cch == 0 only for EOF)

    if (cch)
    {
        hr = THR(_ctx._phpx->AddText(_pNode, pch, cch, fAscii));
    }

Cleanup:

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::ParseEof
//
//  Synopsis:   Process EOF
//
//-------------------------------------------------------------------------
HRESULT
CHtmParse::ParseEof()
{
    HRESULT hr;
    
    // Close literal context (TITLE)

    if (_etagLitCtx)
    {
        hr = THR(PopHpx());
        if (hr)
            goto Cleanup;
            
        _etagLitCtx = ETAG_NULL;
    }

    // Close any element that needs to be closed before processing EOF
    
    hr = THR(CloseAllContainers(s_atagEOFProhibited, s_atagNull));
    if (hr)
        goto Cleanup;

    // Feed a zero-length string through (may imply a BODY etc).
    
    hr = THR(ParseText(NULL, 0, TRUE));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::ParseBeginTag
//
//  Synopsis:   Process the begin tag in pht.
//
//-------------------------------------------------------------------------
HRESULT
CHtmParse::ParseBeginTag(CHtmTag *pht)
{
    CElement *pel = NULL;
    const CTagDesc *ptd;
    HRESULT hr;
    PREPARE_CODE code;

    // Step 1: do all the implicit begin/end parsing

    hr = THR(PrepareContainer(pht->GetTag(), pht, &code));
    if (hr)
        goto Cleanup;
    if (code != PREPARE_NORMAL)
        goto Special;

    // Step 2: add implicit children
    hr = THR(AddImplicitChildren(pht->GetTag()));
    if (hr)
        goto Cleanup;

    // Step 3: Detect litctx tags (just TITLE) that must not be in the tree
    
    ptd = TagDescFromEtag(pht->GetTag());
    
    if (ptd->_dwTagDescFlags & TAGDESC_LITCTX)
    {
        Assert(ptd->_pParseClass->_pfnHpxCreator);
        _etagLitCtx = pht->GetTag();
        hr = THR(PushHpx(ptd->_pParseClass->_pfnHpxCreator, _pMarkup->Root(), _pNode));
        if (hr)
            goto Cleanup;

        // merge the tag into the element, if any
        if (HpcFromEtag(pht->GetTag())->_fMerge)
        {
            code = PREPARE_MERGE;
            goto Special;
        }
        
        goto Cleanup;
    }

    // Step 4: Make element

    hr = THR(CreateElement(pht, &pel, _pDoc, _pMarkup, &_fDie));
    if (hr)
        goto Cleanup;

    // Step 5: Put it into the tree
    hr = THR(BeginElement(pel, TRUE, pht->IsEmpty()));
    if (hr)
        goto Cleanup;

Cleanup:

    CElement::ReleasePtr(pel);
        
    RRETURN(hr);

Special:

    switch (code)
    {
    case PREPARE_UNKNOWN:
        hr = THR(ParseUnknownTag(pht));
        goto Cleanup;
    case PREPARE_MERGE:
        hr = THR(RequestMergeTag(pht));
        goto Cleanup;
    case PREPARE_QUEUE:
        hr = THR(QueueTag(pht));
        goto Cleanup;
    default:
        Assert(0);
        goto Cleanup;
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::RequestMergeTag
//
//  Synopsis:   Merge the begin tag in pht.
//
//-------------------------------------------------------------------------
HRESULT
CHtmParse::RequestMergeTag(CHtmTag *pht)
{
    HRESULT hr = S_OK;
    
    if (_pelMerge)
    {
        if (!MergableTags(_pelMerge->Tag(), pht->GetTag()))
            goto Cleanup;
    }
    else
    {
        CTreeNode *pNode = FindContainer(pht->GetTag(), HpcFromEtag(pht->GetTag())->_atagEndContainers);
        if (pNode)
        {
            _pelMerge = pNode->Element();
            _pelMerge->AddRef();
        }
    }
    
    if (!_pMergeTagQueue)
    {
        _pMergeTagQueue = new CHtmTagQueue();
        if (!_pMergeTagQueue)
            goto OutOfMemory;
    }

    hr = THR(_pMergeTagQueue->EnqueueTag(pht));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::MergeTags
//
//  Synopsis:   Merge the begin tag in pht.
//
//-------------------------------------------------------------------------
HRESULT
CHtmParse::MergeTags()
{
    HRESULT hr = S_OK;
    CElement *pelMerge;
    CHtmTagQueue *pTagQueue = NULL;

    if (_pelMerge)
    {
        pelMerge = _pelMerge;
    }
    else
    {
        pelMerge = _ctx._phpx->GetMergeElement();
    }

    if (pelMerge)
    {
        pTagQueue = _pMergeTagQueue;
        _pMergeTagQueue = NULL;
        
        while (!pTagQueue->IsEmpty())
        {
            CHtmTag *pht;
            pht = pTagQueue->DequeueTag();
            
            Assert(pht);
            Assert(HpcFromEtag(pht->GetTag())->_fMerge || (TagDescFromEtag(pht->GetTag())->_dwTagDescFlags & TAGDESC_LITCTX));
            Assert(MergableTags(pelMerge->Tag(), pht->GetTag()));

        
            pelMerge->_fSynthesized = FALSE;
            
            hr = THR(pelMerge->MergeAttrBag(pht));
            if (hr)
                goto Cleanup;
        }

        if (_pelMerge)
        {
            _pelMerge->Release();
            _pelMerge = NULL;
        }
    }

Cleanup:
    if (pTagQueue)
        pTagQueue->Release();

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::QueueTag
//
//  Synopsis:   Queue the tag in pht.
//
//-------------------------------------------------------------------------
HRESULT
CHtmParse::QueueTag(CHtmTag *pht)
{
    CHtmlParseClass *phpc;
    HRESULT hr;

    if (pht->Is(ETAG_RAW_BEGINSEL) || pht->Is(ETAG_RAW_ENDSEL))
    {
        Assert(_etagReplay && _pTagQueue);
    }
    else
    {
        phpc = HpcFromEtag(pht->GetTag());

        Assert(phpc->_fQueueForRequired);

        // If queuing in hope of a different tag than we're keeping track of
        // for replay, Assert, give up, and drop tag
        if (_etagReplay && phpc->_etagDefaultContainer != _etagReplay)
        {
            Assert(0);
            return S_OK;
        }

        _etagReplay = phpc->_etagDefaultContainer;

        if (!_pTagQueue)
        {
            _pTagQueue = new CHtmTagQueue;
            if (!_pTagQueue)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }
    }

    hr = THR(_pTagQueue->EnqueueTag(pht));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Function:   GenericTagMatch
//
//  Synopsis:   Determines if a closing generic tag matches the given one
//
//-------------------------------------------------------------------------

BOOL
GenericTagMatch(CHtmTag * pht, CTreeNode * pNode)
{
    LPTSTR          pchColon;
    const TCHAR *   pchTagName   = pNode->Element()->TagName();
    const TCHAR *   pchScopeName = pNode->Element()->Namespace();

    Assert(IsGenericTag(pht->GetTag()));
    Assert(pht->IsEnd());

    if (!pht->GetCch())
        return FALSE;
    
    pchColon = StrChr(pht->GetPch(), _T(':'));

    if (pchColon)
    {
        if (0 != StrCmpNIC(pchTagName, pchColon + 1, pht->GetCch() - PTR_DIFF(pchColon, pht->GetPch()) - 1))
            return FALSE;

        if (0 != StrCmpNIC(pchScopeName, pht->GetPch(), PTR_DIFF(pchColon, pht->GetPch())))
            return FALSE;
    }
    else
    {
        if (0 != StrCmpNIC(pchTagName, pht->GetPch(), pht->GetCch()))
            return FALSE;
        
        if (pchScopeName)
            return FALSE;
    }

    return TRUE;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::ParseEndTag
//
//  Synopsis:   Processes the end tag in pht.
//
//-------------------------------------------------------------------------
HRESULT
CHtmParse::ParseEndTag(CHtmTag *pht)
{
    CTreeNode * pNode;
    CHtmlParseClass *phpc;
    PREPARE_CODE code;
    ELEMENT_TAG etag;
    HRESULT hr = S_OK;

    phpc = HpcFromEtag(pht->GetTag());

    // Step 1: End litctx if any (for TITLE)

    if (_etagLitCtx)
    {
        BOOL fMatch;
        
        hr = THR(PopHpx());
        if (hr)
            goto Cleanup;
            
        fMatch = (pht->GetTag() == _etagLitCtx);
        
        _etagLitCtx = ETAG_NULL;
        
        if (fMatch)
            RRETURN(hr);
    }

    // Step 2: Find matching container

    if (phpc->_atagMatch) // NS compat: bug 24945 - some end tags match several begin tags
    {
        pNode = FindContainer(phpc->_atagMatch, phpc->_atagEndContainers);
    }
    else if (pht->Is(ETAG_GENERIC))
    {
        pNode = FindGenericContainer(pht, phpc->_atagEndContainers);
    }
    else
    {
        pNode = FindContainer(pht->GetTag(), phpc->_atagEndContainers);
    }

    // Step 3: if tag is XML or generic, verify that the name matches; throw an error if it does not
    if (IsGenericTag(pht->GetTag()))
    {
        if (pNode)
        {
            if (!GenericTagMatch(pht, pNode))
            {
                goto Unknown;
            }
        }
        else
            goto Unknown;
    }

    // Step 4: imply begin tags for compat
    // NS compat bug 15597 (new fix) Some unmatched end tags imply a begin tag
    // other unmatched end tags are replaced by a different end tag
    if (!pNode)
    {
        if (phpc->_fQueueForRequired)
        {
            // If required container is not present and _fQueueForRequired is set, queue up end tag
            
            if (!FindContainer(phpc->_atagRequiredContainers, phpc->_atagBeginContainers))
            {
                hr = THR(QueueTag(pht));
                goto Cleanup;
            }
        }
        
        etag = phpc->_etagUnmatchedSubstitute;
        
        if (!etag)
            goto Unknown;
            
        // open a container to be closed immediately
        if (etag == ETAG_IMPLICIT_BEGIN)
        {
            etag = pht->GetTag();
                
            hr = THR(PrepareContainer(etag, NULL, &code));
            if (hr)
                goto Cleanup;

            if (code != PREPARE_NORMAL)
            {
                if (code == PREPARE_UNKNOWN)
                    goto Unknown;
                else
                    goto Cleanup; // drop end w/ implicit begin in merge or queue situations
            }
                
            hr = THR(OpenContainer(etag));
            if (hr == S_FALSE)
                goto Unknown;
            if (hr)
                goto Cleanup;

            if (phpc->_scope == SCOPE_EMPTY)
                goto Cleanup;
            
            Assert(_pNode->Tag() == etag);
            pNode = _pNode;
        }
        else
        {
            // search for the alternate match
            phpc = HpcFromEtag(etag);
            
            pNode = FindContainer(etag, phpc->_atagEndContainers);
            if (!pNode)
                goto Unknown;
        }
    }

    // Step 5: Close the container

    pNode->Element()->_fExplicitEndTag = TRUE;

    hr = THR(CloseContainer(pNode, TRUE));
    
Cleanup:
    RRETURN(hr);

Unknown:
    hr = THR(ParseUnknownTag(pht));
    goto Cleanup;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::ParseIgnoredTag
//
//  Synopsis:   Processes the ignored tag in pht.
//
//-------------------------------------------------------------------------
HRESULT
CHtmParse::ParseIgnoredTag(CHtmTag *pht)
{
    CHtmlParseClass *phpc;
    CTreeNode *pNode;
    
    if (!pht->IsEnd())
        return S_OK;

    phpc = HpcFromEtag(pht->GetTag());

    if (phpc && phpc->_fMerge)
    {
        pNode = FindContainer(pht->GetTag(), phpc->_atagEndContainers);
        if (pNode)
        {
            pNode->Element()->_fExplicitEndTag = TRUE;
        }
    }

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::ParseUnknownTag
//
//  Synopsis:   Processes the unknown tag in pht.
//
//-------------------------------------------------------------------------
HRESULT
CHtmParse::ParseUnknownTag(CHtmTag *pht)
{
    CElement *pel = NULL;
    const TCHAR *pchName;
    HRESULT hr;

    // Would the context prefer pht's or unknown elements?
    if (_ctx._phpx->_fDropUnknownTags)
    {
        hr = S_OK;
        goto Cleanup;
    }

    // If the tag has no name (e.g. ETAG_FRAG), drop it
    if( pht->GetTag() != ETAG_UNKNOWN )
    {
        pchName = NameFromEtag(pht->GetTag());
        if (!pchName || !*pchName)
            return S_OK;
    }

    TraceTag((tagParse, "Unknown tag <%s%ls>", pht->IsEnd() ? "/" : "", NameFromEtag(pht->GetTag())));

    // Implicit children
    hr = THR(AddImplicitChildren(pht->GetTag()));
    if (hr)
        goto Cleanup;

    // Make element and put in the tree

    hr = THR(CreateUnknownElement(pht, &pel, _pDoc, &_fDie));
    if (hr)
        goto Cleanup;
        
    hr = THR(BeginElement(pel, TRUE));
    if (hr)
        goto Cleanup;

Cleanup:
    CElement::ReleasePtr(pel);

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::ParseComment
//
//  Synopsis:   Processes the comment in pht.
//
//-------------------------------------------------------------------------
HRESULT
CHtmParse::ParseComment(CHtmTag *pht)
{
    CElement *pel = NULL;

    TraceTag((tagParse, "Comment <%ls> %ls", NameFromEtag(pht->GetTag()), pht->GetPch()));

    HRESULT hr;

    // Comments should be dropped by the context if _fDropUnknownTags
    if (_ctx._phpx->_fDropUnknownTags)
    {
        hr = S_OK;
        goto Cleanup;
    }

    // Implicit children
    hr = THR(AddImplicitChildren(ETAG_NULL));
    if (hr)
        goto Cleanup;

    // Make element and put in the tree

    hr = THR(CreateElement(pht, &pel, _pDoc, _pMarkup, &_fDie));
    if (hr)
        goto Cleanup;

    hr = THR(BeginElement(pel, TRUE));
    if (hr)
        goto Cleanup;

Cleanup:
    CElement::ReleasePtr(pel);

    RRETURN(hr); // discard for now
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::ParseSource
//
//  Synopsis:   Processes the original source annotation in pht.
//
//-------------------------------------------------------------------------
HRESULT
CHtmParse::ParseSource(CHtmTag *pht)
{
    TraceTag((tagParse, "Original source %d", NameFromEtag(pht->GetTag()), pht->GetSourceCch()));

    HRESULT hr;

    hr = THR(_ctx._phpx->AddSource(pht));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::FindContainer (singleton version)
//
//  Synopsis:   Finds an element matching exactly stopping at set Stop
//
//-------------------------------------------------------------------------
CTreeNode *
CHtmParse::FindContainer(ELEMENT_TAG etagMatch, ELEMENT_TAG *pSetStop)
{
    CTreeNode * pNode;

    pNode = _pNode;

    while (pNode && pNode->Tag() != etagMatch)
    {
        if (IsEtagInSet(pNode->Tag(), pSetStop))
            return NULL;

        pNode = pNode->Parent();
    }

    return pNode;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::FindContainer (set version)
//
//  Synopsis:   Finds an element in set Match stopping at set Stop
//
//-------------------------------------------------------------------------

#define FCC_MIN_DEPTH 16

CTreeNode *
CHtmParse::FindContainer(ELEMENT_TAG *pSetMatch, ELEMENT_TAG *pSetStop)
{
    CTreeNode * pNode;
    ULONG cDepth;

    pNode = _pNode;
    
    if (_cDepth >= FCC_MIN_DEPTH)
    {
        cDepth = _cDepth;

        while (cDepth < FCC_MIN_DEPTH || !FindContainerCache(cDepth - FCC_MIN_DEPTH, pNode, pSetMatch, pSetStop, &pNode))
        {
            if (!pNode || IsEtagInSet(pNode->Tag(), pSetMatch))
                break;
                
            if (IsEtagInSet(pNode->Tag(), pSetStop))
            {
                pNode = NULL;
                break;
            }
            
            pNode = pNode->Parent();
            cDepth -= 1;
        }
        
        SetContainerCache(_cDepth - FCC_MIN_DEPTH, _pNode, pSetMatch, pSetStop, pNode);

        return pNode;
    }
    else
    {
        while (pNode && !IsEtagInSet(pNode->Tag(), pSetMatch))
        {
            if (IsEtagInSet(pNode->Tag(), pSetStop))
                return NULL;

            pNode = pNode->Parent();
        }

        return pNode;
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::FindContainerCache
//
//  Synopsis:   Speeds FindContainer by using cached results
//
//-------------------------------------------------------------------------
BOOL
CHtmParse::FindContainerCache(ULONG iCache, CTreeNode *pNodeCheck, ELEMENT_TAG *pset1, ELEMENT_TAG *pset2, CTreeNode **ppNodeOut)
{
    CFccLine *pfccl;
    CFccItem *pfcci;
    ULONG c;
    ULONG cDbg = 0;

    if ((ULONG)_aryFccl.Size() < iCache + 1)
        return FALSE;

    pfccl = _aryFccl + iCache;

    if (pfccl->_pNodeCheck != pNodeCheck)
        return FALSE;

    c = pfccl->_iFirst;
    pfcci = pfccl->_afcci + c;
    c += 1;

    if (c > pfccl->_cCached)
        c = pfccl->_cCached;
        
Reloop:
    while (c)
    {
        cDbg += 1;
        
        if (pfcci->_pset1 == pset1 && pfcci->_pset2 == pset2)
        {
            *ppNodeOut = pfcci->_pNodeOut;
            return TRUE;
        }
            
        pfcci -= 1;
        c -= 1;
    }

    if (pfcci < pfccl->_afcci)
    {
        c = pfccl->_cCached - pfccl->_iFirst - 1;
        pfcci = pfccl->_afcci + FCC_WIDTH - 1;
        goto Reloop;
    }

    return FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::SetContainerCache
//
//  Synopsis:   Speeds FindContainer by caching results
//
//-------------------------------------------------------------------------
void
CHtmParse::SetContainerCache(ULONG iCache, CTreeNode *pNodeCheck, ELEMENT_TAG *pset1, ELEMENT_TAG *pset2, CTreeNode *pNodeOut)
{
    CFccLine *pfccl;
    CFccItem *pfcci;
    ULONG c;

    if ((ULONG)_aryFccl.Size() < iCache + 1)
    {
        c = _aryFccl.Size();

        if (!!_aryFccl.Grow(iCache + 1 + iCache / 8)) // 1.125 times the size we need for exponential growth
            return; // OOM

        pfccl = _aryFccl + c;
        c = _aryFccl.Size() - c;
        
        while (c)
        {
            pfccl->_pNodeCheck = NULL;
            pfccl += 1;
            c -= 1;
        }
    }

    pfccl = _aryFccl + iCache;

    if (pfccl->_pNodeCheck != pNodeCheck)
    {
        pfccl->_pNodeCheck = pNodeCheck;
        pfccl->_iFirst = 0;
        pfccl->_cCached = 1;
        pfcci = pfccl->_afcci;
    }
    else
    {
        pfccl->_iFirst += 1;
        if (pfccl->_iFirst >= FCC_WIDTH)
            pfccl->_iFirst = 0;
            
        if (pfccl->_cCached < FCC_WIDTH)
            pfccl->_cCached += 1;
            
        pfcci = pfccl->_afcci + pfccl->_iFirst;
    }
    
    pfcci->_pset1 = pset1;
    pfcci->_pset2 = pset2;
    pfcci->_pNodeOut = pNodeOut;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::FindGenericContainer
//
//  Synopsis:   same as FindContainer, but also matches scope/name of generic
//              tags
//
//-------------------------------------------------------------------------
CTreeNode *
CHtmParse::FindGenericContainer(CHtmTag *pht, ELEMENT_TAG *pSetStop)
{
    CTreeNode * pNode;
    ELEMENT_TAG etagMatch = pht->GetTag();

    pNode = _pNode;

    while (pNode &&
           !(pNode->Tag() == etagMatch && GenericTagMatch(pht, pNode)))
    {
        if (IsEtagInSet(pNode->Tag(), pSetStop))
            return NULL;

        pNode = pNode->Parent();
    }

    return pNode;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::RequiredTextContainer
//
//  Synopsis:   Computes the etag of the required text container, if any.
//
//-------------------------------------------------------------------------
ELEMENT_TAG
CHtmParse::RequiredTextContainer()
{
    CTreeNode * pNode;
    CHtmlParseClass *phpc;

    // optimization: cached required text container

    Assert(!_fValidRTC);

    _fValidRTC = TRUE;

    // find first element with _textscope != TEXTSCOPE_NEUTRAL

    for (pNode = _pNode; pNode; pNode = pNode->Parent())
    {
        phpc = HpcFromEtag(pNode->Tag());

        if (phpc->_textscope == TEXTSCOPE_INCLUDE)
            return (_etagRTC = ETAG_NULL);

        if (phpc->_textscope == TEXTSCOPE_EXCLUDE)
        {
            Assert(HpcFromEtag(phpc->_etagTextSubcontainer)->_textscope == TEXTSCOPE_INCLUDE);
            return (_etagRTC = phpc->_etagTextSubcontainer);
        }
    }

    Assert(0);

    return (_etagRTC = ETAG_NULL);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::PrepareContainer
//
//  Synopsis:   Do all the implicit opening and closing before
//              beginning the specified container
//
//-------------------------------------------------------------------------
HRESULT
CHtmParse::PrepareContainer(ELEMENT_TAG etag, CHtmTag *pht, PREPARE_CODE *pCode)
{
    CTreeNode *pNode;
    CHtmlParseClass *phpc;
    ELEMENT_TAG etag2;
    HRESULT hr = S_OK;

    Assert(!pht || pht->Is(etag));

    phpc = HpcFromEtag(etag);

    // Step 0: Check for masking container
    
    if (phpc->_atagMaskingContainers)
    {
        pNode = FindContainer(phpc->_atagMaskingContainers, phpc->_atagBeginContainers);
        if (pNode)
        {
            if (phpc->_fMerge)
                goto Merge;
            else
                goto Unknown;
        }
    }

    // Step 1: Close all prohibited containers (implied end tags)

    if (phpc->_atagProhibitedContainers)
    {
        hr = THR(CloseAllContainers(phpc->_atagProhibitedContainers, phpc->_atagBeginContainers));
        if (hr)
            goto Cleanup;
    }

    // Step 2: If textlike, ensure text container (implied begin tags depend on nearest text-excluder)
    if ((phpc->_texttype == TEXTTYPE_ALWAYS ||
         phpc->_texttype == TEXTTYPE_QUERY && _ctx._phpx->QueryTextlike(etag, pht)) &&
        (ETAG_NULL != (etag2 = (_fValidRTC ? _etagRTC : RequiredTextContainer()))) )
    {
        phpc = HpcFromEtag(etag2);
        if (phpc->_atagProhibitedContainers)
        {
            hr = THR(CloseAllContainers(phpc->_atagProhibitedContainers, phpc->_atagBeginContainers));
            if (hr)
                goto Cleanup;
        }

        hr = THR(OpenContainer(etag2));
        if (hr == S_FALSE)
        {
            AssertSz(0,"Required text container could not be inserted (DTD error)");
            goto Unknown;
        }
        else if (hr)
            goto Cleanup;
    }

    // Step 3: Otherwise, ensure any required containers (implied begin tags depend on tag itself)

    else
    if (phpc->_atagRequiredContainers)
    {
        pNode = FindContainer(phpc->_atagRequiredContainers, phpc->_atagBeginContainers);
        if (!pNode)
        {
            if (phpc->_fQueueForRequired)
                goto Queue;
                
            hr = THR(OpenContainer(phpc->_etagDefaultContainer));
            if (hr == S_FALSE)
                goto Unknown;
            else if (hr)
                goto Cleanup;
        }
    }

    *pCode = PREPARE_NORMAL;

Cleanup:
    RRETURN(hr);

Unknown:
    *pCode = PREPARE_UNKNOWN;
    hr = S_OK;
    goto Cleanup;

Queue:
    *pCode = PREPARE_QUEUE;
    hr = S_OK;
    goto Cleanup;

Merge:
    *pCode = PREPARE_MERGE;
    hr = S_OK;
    goto Cleanup;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::OpenContainer
//
//  Synopsis:   Opens a default container (by etag) and any outer
//              containers which are required.
//
//  Returns:    S_OK    if the etag was opened sucessfully
//              S_FALSE if the etag is not allowed in the current context
//              E_*     on error
//
//-------------------------------------------------------------------------
HRESULT
CHtmParse::OpenContainer(ELEMENT_TAG etag)
{
    CTreeNode *pNode;
    CElement *pel = NULL;
    CHtmlParseClass *phpc;
    CStackPtrAry<INT_PTR, 8> aetagStack(Mt(CHtmParseOpenContainer_aetagStack_pv));
    INT_PTR * petagPtr;
    int i;
    HRESULT hr = S_OK;

    // Step 1: Compute all required containers (implied begin tags)

    while (etag)
    {
        aetagStack.Append(etag);

        phpc = HpcFromEtag(etag);
        if (!phpc)
            break;

        if (!phpc->_atagRequiredContainers)
            break;

        pNode = FindContainer(phpc->_atagRequiredContainers, phpc->_atagBeginContainers);
        if (pNode)
            break;

        Assert(!phpc->_fQueueForRequired);
        etag = phpc->_etagDefaultContainer;
    }

    // If unable to insert required containers, return false

    if (!etag)
        return S_FALSE;

    // Step 2: Insert all needed containers in order

    for (i=aetagStack.Size(), petagPtr = (INT_PTR *)aetagStack+i; i; i--)
    {
        ELEMENT_TAG etagItem = (ELEMENT_TAG)*--petagPtr;

        // Implicit children
        hr = THR(AddImplicitChildren(etagItem));
        if (hr)
            goto Cleanup;

        AssertNoneProhibited(etagItem);
        AssertAllRequired(etagItem);
        
        hr = THR(CreateElement(etagItem, &pel, _pDoc, _pMarkup, TRUE, &_fDie));
        if (hr)
            goto Cleanup;

        pel->_fSynthesized = TRUE;
        
        hr = THR(BeginElement(pel, FALSE));
        if (hr)
            goto Cleanup;

        CElement::ClearPtr(&pel);

        aetagStack.Delete(aetagStack.Size()-1);
    }

Cleanup:
    CElement::ReleasePtr(pel);

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::AddImplicitChildren
//
//  Synopsis:   Adds any implicit children required under the current
//              node, given that "etagNext" is the next element that is
//              going to be created.
//
//-------------------------------------------------------------------------
HRESULT
CHtmParse::AddImplicitChildren(ELEMENT_TAG etagNext)
{
    ELEMENT_TAG etagChild;
    CTreeNode *pNodeStart = NULL;
    CTreeNode *pNodeClose = NULL;
    CHtmTagQueue *pTagQueue = NULL;
    CElement *pel = NULL;
    CHtmlParseClass *phpc;
    HRESULT hr = S_OK;

    if (!_fImplicitChild && !_etagReplay)
        return S_OK;

#ifdef NOPARSEADDREF
    CTreeNode::ReplacePtr(&pNodeStart, _pNode);
#else
    pNodeStart = _pNode;
#endif
    
    // Step 1: add implicit children

    while (_fImplicitChild)
    {
        phpc = HpcFromEtag(_pNode->Tag());
        if (!phpc)
            break; // Assert below
            
        etagChild = phpc->_etagImplicitChild;
        if (!etagChild)
            break; // Assert below
            
        _fImplicitChild = FALSE;

        if (!pNodeClose)
        {
            // Close this element if _fCloseImplicitChild is set
            if (phpc->_fCloseImplicitChild)
#ifdef NOPARSEADDREF
                CTreeNode::ReplacePtr(&pNodeClose, _pNode);
#else
                pNodeClose = _pNode;
#endif
                
            // break out if the required child == etagNext
            if (etagChild == etagNext)
                goto Replay;
        }
          
        AssertNoneProhibited(etagChild);
        AssertAllRequired(etagChild);
        
        hr = THR(CreateElement(etagChild, &pel, _pDoc, _pMarkup, TRUE, &_fDie));
        if (hr)
            goto Cleanup;

        pel->_fSynthesized = TRUE;

        hr = THR(BeginElement(pel, FALSE));
        if (hr)
            goto Cleanup;
        
        CElement::ClearPtr(&pel);
    }

    AssertSz(!_fImplicitChild,"_fImplicitChild set when current element doesn't require implicit children");
    _fImplicitChild = FALSE;
    
    // Step 2: close all the implicit children that need to be closed

    if (etagNext || !pNodeClose)
    {
#ifdef NOPARSEADDREF
        CTreeNode::ReplacePtr(&pNodeClose, pNodeStart);
#else
        pNodeClose = pNodeStart;
#endif
    }

    AssertOnStack(pNodeClose);

    while (_pNode != pNodeClose)
    {
        hr = THR(EndElement(_pNode));
        if (hr)
            goto Cleanup;
    }

Replay:

    // If the current node matches the _etagReplay tag, implicitly
    // replay all the queued begin tags.
    
    // The problem with this approach is that there's no guarantee that
    // after parsing all the queued tags, the frontier is in the state
    // that's required for the caller of OpenContainer on the stack.
    // Nevertheless, since all queued tags are fairly inert for the
    // parser (A, P, CENTER, etc), it _should_ work (dbau 5/3/98)
    
    if (_etagReplay)
    {
        if (_pNode->Tag() == _etagReplay)
        {
            if (_pTagQueue)
            {
                pTagQueue = _pTagQueue;
                _pTagQueue = NULL;
                while (!pTagQueue->IsEmpty())
                {
                    CHtmTag *pht;
                    pht = pTagQueue->DequeueTag();
                    Assert(pht);
                    Assert(FindContainer(_etagReplay, s_atagNull));

                    hr = THR(ParseToken(pht));
                    if (hr)
                        goto Cleanup;

                    Assert(!_pTagQueue);
                }
                _etagReplay = ETAG_NULL;
            }
        }
        else
        {
            Assert(!_pTagQueue || !FindContainer(_etagReplay, s_atagNull));
        }
    }
    
Cleanup:
#ifdef NOPARSEADDREF
    CTreeNode::ReleasePtr(pNodeStart);
    CTreeNode::ReleasePtr(pNodeClose);
#endif
    CElement::ReleasePtr(pel);

    if (pTagQueue)
        pTagQueue->Release();

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::CloseContainer
//
//  Synopsis:   Closes pelContainer and any elements between peInner and
//              peContainer which are closed when peContainer closes.
//
//              Creates proxies as needed.
//
//-------------------------------------------------------------------------
HRESULT
CHtmParse::CloseContainer(CTreeNode *pNodeContainer, BOOL fExplicit)
{
    CTreeNode *pNode = NULL;
    CTreeNode **ppNode;
    CTreeNode **ppNode2;
    CHtmlParseClass *phpc;
    CStackPtrAry<CTreeNode*,8> apNodeStack(Mt(CHtmParseCloseContainer_apNodeStack_pv));
    CStackPtrAry<CTreeNode*,8> apNodeClose(Mt(CHtmParseCloseContainer_apNodeClose_pv));
    int c, c2;
    enum { CLOSE_NESTED, CLOSE_NORMAL } closeWhich;
    HRESULT hr;

    // Implicit children
    hr = THR(AddImplicitChildren(ETAG_NULL));
    if (hr)
        goto Cleanup;

    // optimization: do nested close computation only if closing node is not current
    
    if (pNodeContainer != _pNode)
    {
        AssertOnStack(pNodeContainer);

        // step 1: create stack of nested elements to reverse order
        
        for (pNode = _pNode; pNode != pNodeContainer; pNode = pNode->Parent())
        {
            hr = THR(apNodeStack.Append(pNode));
            if (hr)
                goto Cleanup;
        }

        // step 2: create stack of elements to close
        
        hr = THR(apNodeClose.Append(pNodeContainer));
        if (hr)
            goto Cleanup;

        if (HpcFromEtag(pNodeContainer->Tag())->_scope == SCOPE_NESTED)
            closeWhich = CLOSE_NESTED;
        else
            closeWhich = CLOSE_NORMAL;
            
        for (c = apNodeStack.Size(), ppNode = (CTreeNode**)apNodeStack + c - 1; c; c--, ppNode--)
        {
            phpc = HpcFromEtag((*ppNode)->Tag());
            
            if (closeWhich == CLOSE_NESTED && phpc->_scope == SCOPE_NESTED)
            {
                hr = THR(apNodeClose.Append(*ppNode));
                if (hr)
                    goto Cleanup;

                goto Closed;
            }

            for (c2 = apNodeClose.Size(), ppNode2 = apNodeClose; c2; c2--, ppNode2++)
            {
                if (IsEtagInSet((*ppNode2)->Tag(), phpc->_atagEndContainers))
                {
                    hr = THR(apNodeClose.Append(*ppNode));
                    if (hr)
                        goto Cleanup;
                        
                    if (closeWhich == CLOSE_NORMAL && HpcFromEtag((*ppNode)->Tag())->_scope == SCOPE_NESTED)
                        closeWhich = CLOSE_NESTED;
                        
                    goto Closed;
                }
            }

        Closed:
            ;
        }
        
        // step 3: Close nested elements
        
        Assert(apNodeClose.Size());

        for (c = apNodeClose.Size() - 1, ppNode = (CTreeNode**)apNodeClose + c; c; c--, ppNode--)
        {
            AssertOnStack(*ppNode);
            AssertNoEndContainees(*ppNode);
            hr = THR(EndElement(*ppNode));
            if (hr)
                goto Cleanup;
        }

        Assert(*ppNode == pNodeContainer);
    }

    // Step 4: Close requested element itself
    
    AssertOnStack(pNodeContainer);
    AssertNoEndContainees(pNodeContainer);
    
    if (fExplicit && _fDelayPointer)
    {
        hr = InsertLPointerNow();
        if (hr)
            goto Cleanup;
    }
    
    hr = THR(EndElement(pNodeContainer));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::CloseAllContainers
//
//  Synopsis:   Closes every container which matches atagClose up to
//              and (possibly) including the first match in atagBegin
//
//-------------------------------------------------------------------------
HRESULT
CHtmParse::CloseAllContainers(ELEMENT_TAG *atagClose, ELEMENT_TAG *atagBegin)
{
    CTreeNode *pNode;
    HRESULT hr = S_OK;

    for (;;)
    {
        pNode = FindContainer(atagClose, atagBegin);
        if (!pNode)
            break;

        hr = THR(CloseContainer(pNode, FALSE));
        if (hr)
            goto Cleanup;

    }

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::BeginElement
//
//  Synopsis:   Notifies context of the beginning of an element, and
//              advances _pel as needed.
//
//              Also handles creating a new context if needed.
//
//-------------------------------------------------------------------------
HRESULT
CHtmParse::BeginElement(CElement *pel, BOOL fExplicit, BOOL fEndTag)
{
    CHtmlParseClass *phpc;
    CTreeNode       *pNode = NULL;
    HRESULT          hr = S_OK;
    PARSESCOPE       parsescope;
    
    TraceTag((tagParse, "Begin element <%ls>", NameFromEtag(pel->Tag())));

    phpc = HpcFromEtag(pel->Tag());
    parsescope = phpc->_scope;
    if (fEndTag && (pel->Tag() == ETAG_GENERIC || pel->Tag() == ETAG_GENERIC_BUILTIN))
    {
        parsescope = SCOPE_EMPTY;
        pel->_fExplicitEndTag = TRUE;
    }
    
    // optimization: invalidate cached RequiredTextContainer

    if (phpc->_textscope != TEXTSCOPE_NEUTRAL)
        _fValidRTC = FALSE;

    // handle delay-pointer insert
    if (fExplicit && _fDelayPointer)
    {
        hr = InsertLPointerNow();
        if (hr)
            goto Cleanup;
    }
        
    // step 1: notify context

    hr = THR(_ctx._phpx->BeginElement(&pNode, pel, _pNode, (parsescope == SCOPE_EMPTY)));
    if (hr)
        goto Cleanup;
        
    if (_fDie)
        goto Die;

    // step 2: advance _pNode

    if (parsescope != SCOPE_EMPTY)
    {
        Assert(pNode);
#ifdef NOPARSEADDREF
        CTreeNode::ReplacePtr(&_pNode, pNode);
#else
        _pNode = pNode;
#endif
        _cDepth += 1;
    }
        

    // step 3: notify empty element
    else
    {
        if (pel->WantEndParseNotification())
        {
            hr = THR(_aryPelEndParse.Append(pel));
            if (hr)
                goto Cleanup;

            pel->AddRef();
        }
    }
    

    // step 3: deal with pushing context stack

    if (phpc->_pfnHpxCreator)
    {
        hr = THR(PushHpx(phpc->_pfnHpxCreator, pel, pNode));
        if (hr)
            goto Cleanup;
            
        if (parsescope == SCOPE_EMPTY)
        {
            hr = THR(PopHpx());
            if (hr)
                goto Cleanup;
        }
    }

    // step 4: call EndElement for SCOPE_EMPTY elements
    // NOTE: this is needed so that the parse context can
    // advance any cached pointers

    if (parsescope == SCOPE_EMPTY)
    {
        CTreeNode * pNodeNew;

        hr = THR(_ctx._phpx->EndElement(&pNodeNew, pNode, pNode));
        if (hr)
            goto Cleanup;

        Assert( pNodeNew == _pNode );

#ifdef NOPARSEADDREF
        pNodeNew->NodeRelease();
#endif
    }

    // make a note if implicit children are needed
    if (phpc->_etagImplicitChild)
        _fImplicitChild = TRUE;


Cleanup:
#ifdef NOPARSEADDREF
    CTreeNode::ReleasePtr(pNode);
#endif
    
    RRETURN(hr);

Die:
    hr = E_ABORT;
    goto Cleanup;
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::EndElement
//
//  Synopsis:   Closes pelEnd, notifies context if needed,
//              creates proxy chain below, and advances _pel
//
//-------------------------------------------------------------------------
HRESULT
CHtmParse::EndElement(CTreeNode *pNodeEnd)
{
    CTreeNode *pNodeNew = NULL;
    HRESULT hr = S_OK;

    TraceTag((tagParse, "End element   <%ls>", NameFromEtag(pNodeEnd->Tag())));

    // optimization: invalidate cached RequiredTextContainer

    if (HpcFromEtag(pNodeEnd->Tag())->_textscope != TEXTSCOPE_NEUTRAL)
        _fValidRTC = FALSE;

    // step 1: deal with popping the context stack

    if (pNodeEnd->Element() == _ctx._pelTop)
    {
        hr = THR(PopHpx());
        if (hr)
            goto Cleanup;
    }

    // Step 1: notify the owning context that the element is ending

    // Ending within a context: our node should now be under the current context
#if DBG == 1 && 0
    if (_ctx._phpx->IsLeaf())
        AssertInsideContext(pNodeEnd);
#endif
    Assert(_pNode);

    // the context is now responsible for proxying
    hr = THR(_ctx._phpx->EndElement(&pNodeNew, _pNode, pNodeEnd));
    if (hr)
        goto Cleanup;
        
    if (_fDie)
        goto Die;

    _cDepth -= 1;

    // step 3: notify the element itself that it is ending (defered until we can execute).

    if (pNodeEnd->Element()->WantEndParseNotification())
    {
        hr = THR(_aryPelEndParse.Append(pNodeEnd->Element()));
        if (hr)
            goto Cleanup;

        pNodeEnd->Element()->AddRef();
    }

    // step 4: update _pNode

#ifdef NOPARSEADDREF
    _pNode->NodeRelease();
#endif
    _pNode = pNodeNew; // take ref
    pNodeNew = NULL;

Cleanup:

#ifdef NOPARSEADDREF
    CTreeNode::ReleasePtr(pNodeNew);
#endif
    RRETURN(hr);

Die:

    hr = E_ABORT;
    goto Cleanup;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::InsertLPointer
//
//  Synopsis:   Sets a flag so that the L pointer will be inserted
//              right before the next explicit end/begin/text
//
//-------------------------------------------------------------------------

HRESULT
CHtmParse::InsertLPointer()
{
    _fDelayPointer = TRUE;

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::InsertRPointer
//
//  Synopsis:   Immediately inserts a pointer
//
//-------------------------------------------------------------------------

HRESULT
CHtmParse::InsertLPointerNow()
{
    Assert(_fDelayPointer);
    Assert(!_ptpL);
    Assert( _aryContextStack[0]._phpx );

    _fDelayPointer = FALSE;

    RRETURN(_ctx._phpx->InsertLPointer(&_ptpL, _pNode));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::InsertRPointer
//
//  Synopsis:   Immediately inserts a pointer
//
//-------------------------------------------------------------------------

HRESULT
CHtmParse::InsertRPointer()
{
    Assert(!_ptpR);
    Assert( _aryContextStack[0]._phpx );

    RRETURN(_ctx._phpx->InsertRPointer(&_ptpR, _pNode));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::GetPointers
//
//  Synopsis:   Gets CTreePos'es for the L and R pointers
//
//-------------------------------------------------------------------------
void
CHtmParse::GetPointers(CTreePos **pptpL, CTreePos **pptpR)
{
    (*pptpL) = _ptpL;
    (*pptpR) = _ptpR;
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::PushHpx
//
//  Synopsis:   Constructs and pushes a context
//
//-------------------------------------------------------------------------

HRESULT
CHtmParse::PushHpx(HRESULT (*pfnHpxCreator)(CHtmParseCtx **pphpx, CElement *pelTop, CHtmParseCtx *phpxParent), CElement *pel, CTreeNode *pNode)
{
    HRESULT hr;
    CHtmParseCtx *phpxNew = NULL;
    
    TraceTag((tagParse, "Push Hpx for <%ls>", pel ? NameFromEtag(pel->Tag()) : _T("NULL") ));
    
    hr = THR((pfnHpxCreator)(&phpxNew, pel, _ctx._phpx));
    if (hr)
        goto Cleanup;

    hr = THR(phpxNew->Init());
    if (hr)
        goto Cleanup;

    if (_fDie)
        goto Die;

    hr = THR(phpxNew->Prepare());
    if (hr)
        goto Cleanup;

    if (_fDie)
        goto Die;

    hr = THR(_aryContextStack.AppendIndirect(&_ctx));
    if (hr)
        goto Cleanup;

    _ctx._phpx   = phpxNew;
    _ctx._pelTop = pel;

    phpxNew = NULL;
    
Cleanup:
    delete phpxNew; // Expected to be NULL
    RRETURN(hr);
    
Die:
    hr = E_ABORT;
    goto Cleanup;
}


//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::PopHpx
//
//  Synopsis:   pops and destroys a context
//
//-------------------------------------------------------------------------

HRESULT
CHtmParse::PopHpx()
{
    HRESULT hr = S_OK;
    
    TraceTag((tagParse, "Pop Hpx for <%ls>", NameFromEtag(_etagLitCtx ? _etagLitCtx : _ctx._pelTop->Tag())));
    
    hr = THR(_ctx._phpx->Finish());
    if (hr)
        goto Cleanup;

    if (_fDie)
        goto Die;

    if (_ctx._phpx->_fIgnoreSubsequent)
    {
        _fIgnoreInput = TRUE;
    }

    if (!_ctx._phpx->_fNeedExecute)
        delete _ctx._phpx;
    else
    {
        // processing one token should not cause multiple scripts to commit (even EOF)
        // Assert(!_phpxExecute); - no longer true with nested OBJECT tags
        
        delete _phpxExecute;
        _phpxExecute = _ctx._phpx;
    }

    _ctx = _aryContextStack[_aryContextStack.Size()-1];
    _aryContextStack.Delete(_aryContextStack.Size()-1);
    
Cleanup:
    RRETURN(hr);
    
Die:
    hr = E_ABORT;
    goto Cleanup;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmParse::Die
//
//  Synopsis:   Sets _fDie; next time we look, we'll return E_ABORT.
//
//-------------------------------------------------------------------------
HRESULT
CHtmParse::Die()
{
    _fDie = TRUE;
    return(S_OK);
}


#if DBG == 1

//+------------------------------------------------------------------------
//
//  DEBUG ONLY: CHtmParse::AssertNoneProhibitedImpl
//
//  Synopsis:   Asserts that no elements on the stack are prohibited
//              containers for the proposed etag.
//
//-------------------------------------------------------------------------
void
CHtmParse::AssertNoneProhibitedImpl(ELEMENT_TAG etag)
{
    CTreeNode *pNode;
    CHtmlParseClass *phpc;

    phpc = HpcFromEtag(etag);

    if (phpc->_atagProhibitedContainers)
    {
        pNode = FindContainer(phpc->_atagProhibitedContainers, phpc->_atagBeginContainers);
        AssertSz(!pNode, "Encountered prohibited container (DTD error)");
    }
}

//+------------------------------------------------------------------------
//
//  DEBUG ONLY: CHtmParse::AssertAllRequiredImpl
//
//  Synopsis:   Asserts that all required elements are on the stack
//              for inserting the proposed etag.
//
//-------------------------------------------------------------------------
void
CHtmParse::AssertAllRequiredImpl(ELEMENT_TAG etag)
{
    CTreeNode *pNode;
    CHtmlParseClass *phpc;

    phpc = HpcFromEtag(etag);

    if (phpc->_atagRequiredContainers)
    {
        pNode = FindContainer(phpc->_atagRequiredContainers, phpc->_atagBeginContainers);
        AssertSz(pNode, "Missing required container (DTD error)");
    }
}

//+------------------------------------------------------------------------
//
//  DEBUG ONLY: CHtmParse::AssertNoEndContainteesImpl
//
//  Synopsis:   Asserts that all the specified element is not the end
//              container for any elements on the stack.
//
//-------------------------------------------------------------------------
void
CHtmParse::AssertNoEndContaineesImpl(CTreeNode *pNode)
{
    ELEMENT_TAG etag;
    BOOL fNested;
    CTreeNode *pNodeScan;
    CHtmlParseClass *phpc;

    etag = pNode->Tag();
    fNested = (HpcFromEtag(etag)->_scope == SCOPE_NESTED);

    for (pNodeScan = _pNode; pNodeScan; pNodeScan = pNodeScan->Parent())
    {
        if (pNodeScan == pNode)
            return;

        phpc = HpcFromEtag(pNodeScan->Tag());

        AssertSz(
            (ETAG_GENERIC != etag && etag != pNodeScan->Tag()) ||
            (ETAG_GENERIC == etag &&
                ((0 != StrCmpI(pNode->_pElement->TagName(),   pNodeScan->_pElement->TagName())) ||
                 (0 != StrCmpI(pNode->_pElement->NamespaceHtml(), pNodeScan->_pElement->NamespaceHtml())))),
            "Element cannot be closed: same tag below on stack");

        AssertSz(!IsEtagInSet(etag, phpc->_atagEndContainers),
            "Element cannot be closed: end containee below on stack");
        AssertSz(!fNested || phpc->_scope != SCOPE_NESTED,
            "Element cannot be closed: nested element below on stack");
    }

    AssertSz(0, "Element cannot be closed: element not on stack");
}


//+------------------------------------------------------------------------
//
//  DEBUG ONLY: CHtmParse::AssertOnStackImpl
//
//  Synopsis:   Asserts that
//              1. the specified element is above _pel.
//              2. the specified element is not a proxy of elts above _pel
//
//-------------------------------------------------------------------------
void
CHtmParse::AssertOnStackImpl(CTreeNode *pNode)
{
    CTreeNode *pNodeScan;

    for (pNodeScan = _pNode; pNodeScan; pNodeScan = pNodeScan->Parent())
    {
        if (pNodeScan == pNode)
            return;
        AssertSz(DifferentScope(pNodeScan, pNode), "Non first context node on parser stack");
    }

    AssertSz(0, "Node not on parser stack");
}

//+------------------------------------------------------------------------
//
//  DEBUG ONLY: CHtmParse::AssertInsideContextImpl
//
//  Synopsis:   Asserts that
//              1. the specified element is under _pelTop.
//              2. the specified element is not under a proxy of _pelTop
//
//-------------------------------------------------------------------------
void
CHtmParse::AssertInsideContextImpl(CTreeNode *pNode)
{
    CTreeNode *pNodeScan;

    if (!_ctx._pelTop)
        return;

    for (pNodeScan = pNode; pNodeScan; pNodeScan = pNodeScan->Parent())
    {
        if (pNodeScan->Element() == _ctx._pelTop)
            return;
    }

    AssertSz(0, "Node not inside context");
}

#endif // DBG

//+------------------------------------------------------------------------
//
//  Function:   ScanNodeList
//
//  Synopsis:   Like FindContainer; used by the ValidateNodeList function
//
//-------------------------------------------------------------------------

CTreeNode **
ScanNodeList(CTreeNode **apNodeStack, long cNodeStack,
                ELEMENT_TAG *pSetMatch, ELEMENT_TAG *pSetStop)
{
    CTreeNode **ppNode;
    long c;

    for (ppNode = apNodeStack + cNodeStack - 1, c = cNodeStack; c; ppNode -= 1, c -= 1)
    {
        if (IsEtagInSet((*ppNode)->Tag(), pSetMatch))
            return ppNode;

        if (IsEtagInSet((*ppNode)->Tag(), pSetStop))
            return NULL;
    }

    return NULL;
}

//+------------------------------------------------------------------------
//
//  Function:   ValidateNodeList
//
//  Synopsis:   Used by the paster to recognize if two element chains
//              can be spliced together (and if not, which two elements
//              conflict with each other.)
//
//              If there is a conflict, returns the two indices to
//              conflicting members in the array.
//
//-------------------------------------------------------------------------

static inline BOOL
RequiresFirstContextContainers ( ELEMENT_TAG etag )
{
    switch ( etag )
    {
    case ETAG_BASE :
    case ETAG_MAP :
    case ETAG_FORM :
    case ETAG_NOEMBED_OFF :
    case ETAG_NOFRAMES_OFF :
    case ETAG_NOSCRIPT_OFF :
        return TRUE;
        
    default:
        return FALSE;
    }
}

HRESULT
ValidateNodeList(
    CTreeNode ** apNode,    // array of tree Nodes
    long         cNode,     // length of apNode array
    long         cNodeOk,   // number of elts of apElement array already okayed
    BOOL         fContain,  // should we validate required containers or not
    long *       piT,       // index of top-conflicting elt
    long *       piB)       // index of bottom-conflicting elt (> top index)
{
    CTreeNode **ppNodeT = NULL, **ppNodeB = NULL;
    CHtmlParseClass *phpc;
    long c;

    // First check for a container that has literal tokenizing rules
    // (<XMP>, <SCRIPT>, <TITLE>, <TEXTAREA>)

    ppNodeB = apNode + cNode - 2;
    ppNodeT = apNode + max( (LONG)(cNodeOk - 1), 0L );

    for ( ; ppNodeB >= ppNodeT ; --ppNodeB )
    {
        if (TagDescFromEtag((*ppNodeB)->Tag())->HasFlag(TAGDESC_LITERALTAG))
        {
            ppNodeT = ppNodeB;
            ppNodeB++;
            goto Conflict;
        }
    }

    // Then check for parsing rules
    
    for (ppNodeB = apNode + cNodeOk, c = cNode - cNodeOk; c; c -= 1, ppNodeB += 1)
    {
        phpc = HpcFromEtag((*ppNodeB)->Tag());

        if (phpc->_atagMaskingContainers)
        {
            ppNodeT = ScanNodeList(apNode, ppNodeB - apNode, phpc->_atagMaskingContainers, phpc->_atagBeginContainers);
            if (ppNodeT)
                goto Conflict;
        }
        
        if (phpc->_atagProhibitedContainers)
        {
            ppNodeT = ScanNodeList(apNode, ppNodeB - apNode, phpc->_atagProhibitedContainers, phpc->_atagBeginContainers);
            if (ppNodeT)
                goto Conflict;
        }

#if 0 // Commented out to fix 58326.
      // Don't check TEXTSCOPE because it's only used to generate TCs around
      // textlike tags in tables in the parser; these are actually optional
      // and shouldn't be needed during paste.
      
        if (phpc->_fTextlike)
        {
            for (ppNodeT = ppNodeB - 1, c2 = ppNodeB - apNode; c2; c2 -= 1, ppNodeT -= 1)
            {
                phpc2 = HpcFromEtag((*pNodeT)->Tag());

                if (phpc2->_textscope == TEXTSCOPE_INCLUDE)
                    break;

                if (phpc2->_textscope == TEXTSCOPE_EXCLUDE)
                    goto Conflict;
            }
        }
#endif

        //
        // If desired, validate required containers.
        //
        
        if (fContain && phpc->_atagRequiredContainers &&
            ((*ppNodeB)->IsFirstBranch() || !RequiresFirstContextContainers( (*ppNodeB)->Tag() )))
        {
            ppNodeT =
                ScanNodeList(
                    apNode, ppNodeB - apNode, phpc->_atagRequiredContainers,
                    phpc->_atagBeginContainers );
            
            if (!ppNodeT)
                goto Conflict;
        }
    }

    *piT = *piB = 0;

    return S_OK;

Conflict:

    *piT = ppNodeT ? ppNodeT - apNode : 0;
    *piB = ppNodeB ? ppNodeB - apNode : 0;

    return S_FALSE;
}


#if DBG != 1
#pragma optimize("", on)
#endif

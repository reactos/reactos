#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_IRANGE_HXX_
#define X_IRANGE_HXX_
#include "irange.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_CODEPAGE_H_
#define X_CODEPAGE_H_
#include "codepage.h"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X_UNDO_HXX_
#define X_UNDO_HXX_
#include "undo.hxx"
#endif

#ifndef X_MARKUPUNDO_HXX_
#define X_MARKUPUNDO_HXX_
#include "markupundo.hxx"
#endif

////////////////////////////////////////////////////////////////
//    IMarkupServices methods

HRESULT
CDoc::CreateMarkupPointer ( CMarkupPointer * * ppPointer )
{
    Assert( ppPointer );

    *ppPointer = new CMarkupPointer( this );

    if (!*ppPointer)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT
CDoc::CreateMarkupPointer ( IMarkupPointer ** ppIPointer )
{
    HRESULT hr;
    CMarkupPointer * pPointer = NULL;

    hr = THR( CreateMarkupPointer( & pPointer ) );

    if (hr)
        goto Cleanup;

    hr = THR(
        pPointer->QueryInterface(
            IID_IMarkupPointer, (void **) ppIPointer ) );

    if (hr)
        goto Cleanup;

Cleanup:

    ReleaseInterface( pPointer );
    
    RRETURN( hr );
}


HRESULT
CDoc::MovePointersToRange (
    IHTMLTxtRange * pIRange,
    IMarkupPointer *  pIPointerStart,
    IMarkupPointer *  pIPointerFinish )
{
    HRESULT hr = S_OK;
    CAutoRange *pRange;
    CMarkupPointer *pPointerStart=NULL, *pPointerFinish=NULL;
    
    // check argument sanity
    if (pIRange==NULL          || !IsOwnerOf(pIRange) ||
        (pIPointerStart !=NULL && !IsOwnerOf(pIPointerStart))  ||
        (pIPointerFinish!=NULL && !IsOwnerOf(pIPointerFinish)) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // get the internal objects corresponding to the arguments
    hr = pIRange->QueryInterface(CLSID_CRange, (void**)&pRange);
    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (pIPointerStart)
    {
        hr = pIPointerStart->QueryInterface(CLSID_CMarkupPointer, (void**)&pPointerStart);
        if (hr)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }

    if (pIPointerFinish)
    {
        hr = pIPointerFinish->QueryInterface(CLSID_CMarkupPointer, (void**)&pPointerFinish);
        if (hr)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }

    // move the pointers
    
    if (pPointerStart)
        hr = pRange->GetLeft( pPointerStart );
    
    if (!hr && pPointerFinish)
        hr = pRange->GetRight( pPointerFinish );
    
Cleanup:
    
    RRETURN( hr );
}


HRESULT
CDoc::MoveRangeToPointers (
    IMarkupPointer *  pIPointerStart,
    IMarkupPointer *  pIPointerFinish,
    IHTMLTxtRange * pIRange )
{
    HRESULT        hr = S_OK;
    CAutoRange *   pRange;
    BOOL           fPositioned;

    if (!pIPointerStart || !pIPointerFinish || !pIRange)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    if (!IsOwnerOf( pIRange ) ||
        !IsOwnerOf( pIPointerStart )  || !IsOwnerOf( pIPointerFinish ))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pIPointerStart->IsPositioned( &fPositioned ) );
    if (hr)
        goto Cleanup;
    if (! fPositioned)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pIPointerFinish->IsPositioned( &fPositioned ) );
    if (hr)
        goto Cleanup;
    if (! fPositioned)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // get the internal objects corresponding to the arguments
    hr = THR( pIRange->QueryInterface( CLSID_CRange, (void**) & pRange ) );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pRange->SetLeftAndRight( pIPointerStart, pIPointerFinish, FALSE ));
    
Cleanup:
    
    RRETURN( hr );
}


HRESULT
CDoc::InsertElement (
    CElement *       pElementInsert,
    CMarkupPointer * pPointerStart,
    CMarkupPointer * pPointerFinish,
    DWORD            dwFlags )
{
    HRESULT     hr = S_OK;
    CTreePosGap tpgStart, tpgFinish;

    Assert( pElementInsert );
    Assert( pPointerStart );

    //
    // If the the finish is not specified, set it to the start to make the element span
    // nothing at the start.
    //
    
    if (!pPointerFinish)
        pPointerFinish = pPointerStart;

    Assert( ! pElementInsert->GetFirstBranch() );

    //
    // If the element is no scope, then we must ignore the finish
    //

    if (pElementInsert->IsNoScope())
        pPointerFinish = pPointerStart;

    //
    // Make sure the start if before the finish
    //

    Assert( pPointerStart->IsLeftOfOrEqualTo( pPointerFinish ) );

    //
    // Make sure both pointers are positioned, and in the same tree
    //

    Assert( pPointerStart->IsPositioned() );
    Assert( pPointerFinish->IsPositioned() );
    Assert( pPointerStart->Markup() == pPointerFinish->Markup() );

    //
    // Make sure unembedded markup pointers go in for the modification
    //

    hr = THR( pPointerStart->Markup()->EmbedPointers() );

    if (hr)
        goto Cleanup;

    //
    // Position the gaps and do the insert
    //

    // Note: We embed to make sure the pointers get updated, but we
    // also take advantage of it to get pointer pos's for the input
    // args.  It would be nice to treat the inputs specially in the
    // operation and not have to embed them......
    
    tpgStart.MoveTo( pPointerStart->GetEmbeddedTreePos(), TPG_LEFT );
    tpgFinish.MoveTo( pPointerFinish->GetEmbeddedTreePos(), TPG_LEFT );

    hr = THR(
        pPointerStart->Markup()->InsertElementInternal(
            pElementInsert, & tpgStart, & tpgFinish, dwFlags ) );

    if (hr)
        goto Cleanup;

Cleanup:
    
    RRETURN( hr );
}

HRESULT
CDoc::InsertElement(
    IHTMLElement *   pIElementInsert,
    IMarkupPointer * pIPointerStart,
    IMarkupPointer * pIPointerFinish)
{
    HRESULT          hr = S_OK;
    CElement *       pElementInsert;
    CMarkupPointer * pPointerStart, * pPointerFinish;

    //
    // If the the finish is not specified, set it to the start to make the element span
    // nothing at the start.
    //
    
    if (!pIPointerFinish)
        pIPointerFinish = pIPointerStart;

    //
    // Make sure all the arguments are specified and belong to this document
    //
    
    if (!pIElementInsert || !IsOwnerOf( pIElementInsert ) ||
        !pIPointerStart  || !IsOwnerOf( pIPointerStart  )  ||
        !pIPointerFinish || !IsOwnerOf( pIPointerFinish ) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // get the internal objects corresponding to the arguments
    //
    
    hr = THR( pIElementInsert->QueryInterface( CLSID_CElement, (void **) & pElementInsert ) );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    Assert( pElementInsert );

    //
    // The element must not already be in a tree
    //

    if (pElementInsert->GetFirstBranch())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Get the "real" objects associated with these pointer interfaces
    //
    
    hr = THR( pIPointerStart->QueryInterface( CLSID_CMarkupPointer, (void **) & pPointerStart ) );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    hr = THR( pIPointerFinish->QueryInterface( CLSID_CMarkupPointer, (void **) & pPointerFinish ) );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Make sure both pointers are positioned, and in the same tree
    //

    if (!pPointerStart->IsPositioned() || !pPointerFinish->IsPositioned())
    {
        hr = CTL_E_UNPOSITIONEDPOINTER;
        goto Cleanup;
    }
    
    if (pPointerStart->Markup() != pPointerFinish->Markup())
    {
        hr = CTL_E_INCOMPATIBLEPOINTERS;
        goto Cleanup;
    }

    //
    // Make sure the start if before the finish
    //

    EnsureLogicalOrder( pPointerStart, pPointerFinish );
    
#if TREE_SYNC
/**/    // this is a big HACK, to temporarily get markup-sync working for netdocs.  it
/**/    // will totally change in the future.
/**/    // trident team: do not bother trying to maintain this code, delete if it causes problems
/**/    if (_SyncLogger.IsLogging())
/**/    {
/**/        hr = THR(_SyncLogger.InsertElementHandler(pIPointerStart,pIPointerFinish,pIElementInsert));
/**/        if (S_OK != hr)
/**/        {
/**/            goto Cleanup;
/**/        }
/**/    }
#endif // TREE_SYNC

    hr = THR( InsertElement( pElementInsert, pPointerStart, pPointerFinish ) );

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( hr );
}


HRESULT
CDoc::RemoveElement ( 
    CElement *  pElementRemove,
    DWORD       dwFlags )
{
    HRESULT    hr = S_OK;

    //
    // Element to be removed must be specified and it must be associated
    // with this document.
    //

    Assert( pElementRemove );

    //
    //  Assert that the element is in the markup
    //
    
    Assert( pElementRemove->IsInMarkup() );

#if TREE_SYNC
/**/    // this is a big HACK, to temporarily get markup-sync working for netdocs.  it
/**/    // will totally change in the future.
/**/    // trident team: do not bother trying to maintain this code, delete if it causes problems
/**/    if (_SyncLogger.IsLogging())
/**/    {
/**/        hr = THR(_SyncLogger.RemoveElementHandler(pElementRemove));
/**/        if (hr != S_OK)
/**/        {
/**/            goto Cleanup;
/**/        }
/**/    }
#endif // TREE_SYNC

    //
    // Now, remove the element
    //

    hr = THR( pElementRemove->GetMarkup()->RemoveElementInternal( pElementRemove, dwFlags ) );

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( hr );
}

HRESULT
CDoc::RemoveElement ( IHTMLElement * pIElementRemove )
{
    HRESULT    hr = S_OK;
    CElement * pElementRemove = NULL;

    //
    // Element to be removed must be specified and it must be associated
    // with this document.
    //
    
    if (!pIElementRemove || !IsOwnerOf( pIElementRemove ))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // get the interneal objects corresponding to the arguments
    //
    
    hr = THR(
        pIElementRemove->QueryInterface(
            CLSID_CElement, (void **) & pElementRemove ) );
    
    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Make sure element is in the tree
    //

    if (!pElementRemove->IsInMarkup())
    {
        hr = CTL_E_UNPOSITIONEDELEMENT;
        goto Cleanup;
    }

    //
    // Do the remove
    //

    hr = THR( RemoveElement( pElementRemove ) );

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( hr );
}

HRESULT
CDoc::Remove (
    CMarkupPointer * pPointerStart,
    CMarkupPointer * pPointerFinish,
    DWORD            dwFlags )
{
    return CutCopyMove( pPointerStart, pPointerFinish, NULL, TRUE, dwFlags );
}

HRESULT
CDoc::Copy (
    CMarkupPointer * pPointerSourceStart,
    CMarkupPointer * pPointerSourceFinish,
    CMarkupPointer * pPointerTarget,
    DWORD            dwFlags )
{
    return
        CutCopyMove(
            pPointerSourceStart, pPointerSourceFinish,
            pPointerTarget, FALSE, dwFlags );
}

HRESULT
CDoc::Move (
    CMarkupPointer * pPointerSourceStart,
    CMarkupPointer * pPointerSourceFinish,
    CMarkupPointer * pPointerTarget,
    DWORD            dwFlags )
{
    return
        CutCopyMove(
            pPointerSourceStart, pPointerSourceFinish,
            pPointerTarget, TRUE, dwFlags );
}

HRESULT
CDoc::Remove (
    IMarkupPointer * pIPointerStart,
    IMarkupPointer * pIPointerFinish )
{
    return CutCopyMove( pIPointerStart, pIPointerFinish, NULL, TRUE );
}

HRESULT
CDoc::Copy(
    IMarkupPointer * pIPointerStart,
    IMarkupPointer * pIPointerFinish,
    IMarkupPointer * pIPointerTarget )
{
    return CutCopyMove( pIPointerStart, pIPointerFinish, pIPointerTarget, FALSE );
}


HRESULT
CDoc::Move(
    IMarkupPointer * pIPointerStart,
    IMarkupPointer * pIPointerFinish,
    IMarkupPointer * pIPointerTarget )
{
    return CutCopyMove( pIPointerStart, pIPointerFinish, pIPointerTarget, TRUE );
}


HRESULT
CDoc::InsertText (
    CMarkupPointer * pPointerTarget,
    const OLECHAR *  pchText,
    long             cch,
    DWORD            dwFlags )
{
    HRESULT hr = S_OK;

    Assert( pPointerTarget );
    Assert( pPointerTarget->IsPositioned() );

    hr = THR( pPointerTarget->Markup()->EmbedPointers() );

    if (hr)
        goto Cleanup;

    if (cch < 0)
        cch = pchText ? _tcslen( pchText ) : 0;

#if TREE_SYNC
/**/    // this is a big HACK, to temporarily get markup-sync working for netdocs.  it
/**/    // will totally change in the future.
/**/    // trident team: do not bother trying to maintain this code, delete if it causes problems
/**/    if (_SyncLogger.IsLogging())
/**/    {
/**/        hr = THR(_SyncLogger.InsertTextHandler(pchText,cch,pPointerTarget));
/**/        if (hr != S_OK)
/**/        {
/**/            goto Cleanup;
/**/        }
/**/    }
#endif // TREE_SYNC

    hr = THR(
        pPointerTarget->Markup()->InsertTextInternal(
            pPointerTarget->GetEmbeddedTreePos(), pchText, cch, dwFlags ) );

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN(hr);
}

HRESULT
CDoc::InsertText ( OLECHAR * pchText, long cch, IMarkupPointer * pIPointerTarget )
{
    HRESULT          hr = S_OK;
    CMarkupPointer * pPointerTarget;

    if (!pIPointerTarget || !IsOwnerOf( pIPointerTarget ))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Get the internal objects corresponding to the arguments
    //
    
    hr = THR(
        pIPointerTarget->QueryInterface(
            CLSID_CMarkupPointer, (void **) & pPointerTarget ) );
    
    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // more sanity checks
    //
    
    if (!pPointerTarget->IsPositioned())
    {
        hr = CTL_E_UNPOSITIONEDPOINTER;
        goto Cleanup;
    }

    //
    // Do it
    //

    hr = THR( InsertText( pPointerTarget, pchText, cch ) );

    if (hr)
        goto Cleanup;

Cleanup:
    
    RRETURN( hr );
}


HRESULT
CDoc::ParseString (
    OLECHAR *            pchHTML,
    DWORD                dwFlags,
    IMarkupContainer * * ppIContainerResult,
    IMarkupPointer *     pIPointerStart,
    IMarkupPointer *     pIPointerFinish )
{
    HRESULT hr = S_OK;
    HGLOBAL hHtmlText = NULL;

    if (!pchHTML || !ppIContainerResult)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    extern HRESULT HtmlStringToSignaturedHGlobal (
        HGLOBAL * phglobal, const TCHAR * pStr, long cch );

    hr = THR(
        HtmlStringToSignaturedHGlobal(
            & hHtmlText, pchHTML, _tcslen( pchHTML ) ) );

    if (hr)
        goto Cleanup;

    Assert( hHtmlText );

    hr = THR(
        ParseGlobal(
            hHtmlText, dwFlags, ppIContainerResult,
            pIPointerStart, pIPointerFinish ) );

    if (hr)
        goto Cleanup;

Cleanup:

    if (hHtmlText)
        GlobalFree( hHtmlText );
    
    RRETURN( hr );
}

static HRESULT
PrepareStream (
    HGLOBAL hGlobal,
    IStream * * ppIStream,
    BOOL fInsertFrags,
    HTMPASTEINFO * phtmpasteinfo )
{
    CStreamReadBuff * pstreamReader = NULL;
    LPSTR pszGlobal = NULL;
    long lSize;
    BOOL fIsEmpty, fHasSignature;
    TCHAR szVersion[ 24 ];
    TCHAR szSourceUrl [ pdlUrlLen ];
    long iStartHTML, iEndHTML;
    long iStartFragment, iEndFragment;
    long iStartSelection, iEndSelection;
    ULARGE_INTEGER ul = { 0, 0 };
    HRESULT hr = S_OK;

    Assert( hGlobal );
    Assert( ppIStream );

    //
    // Get access to the bytes of the global
    //

    pszGlobal = LPSTR( GlobalLock( hGlobal ) );

    if (!pszGlobal)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // First, compute the size of the global
    //

    lSize = 0;

    // BUGBUG: Support nonnative signature and sizeof(wchar) == 4/2 (davidd)

    fHasSignature = * (WCHAR *) pszGlobal == NATIVE_UNICODE_SIGNATURE;

    if (fHasSignature)
    {
        lSize = _tcslen( (TCHAR *) pszGlobal ) * sizeof( TCHAR );
        fIsEmpty = (lSize - sizeof( TCHAR )) == 0;
    }
    else
    {
        lSize = lstrlenA( pszGlobal );
        fIsEmpty = lSize == 0;
    }

    //
    // If the HGLOBAL is effectively empty, do nothing, and return this
    // fact.
    //

    if (fIsEmpty)
    {
        *ppIStream = NULL;
        goto Cleanup;
    }

    //
    // Create if the stream got the load context
    //

    hr = THR( CreateStreamOnHGlobal( hGlobal, FALSE, ppIStream ) );

    if (hr)
        goto Cleanup;

    // N.B. (johnv) This is necessary for Win95 support.  Apparently
    // GlobalSize() may return different values at different times for the same
    // hGlobal.  This makes IStream's behavior unpredictable.  To get around
    // this, we set the size of the stream explicitly.

    // Make sure we don't have unicode in the hGlobal

#ifdef UNIX
    U_QUAD_PART(ul)= lSize;
    Assert( U_QUAD_PART(ul) <= GlobalSize( hGlobal ) );
#else
    ul.QuadPart = lSize;
    Assert( ul.QuadPart <= GlobalSize( hGlobal ) );
#endif

    hr = THR( (*ppIStream)->SetSize( ul ) );

    if (hr)
        goto Cleanup;

    iStartHTML      = -1;
    iEndHTML        = -1;
    iStartFragment  = -1;
    iEndFragment    = -1;
    iStartSelection = -1;
    iEndSelection   = -1;

    //
    // Locate the required contextual information in the stream
    //

    pstreamReader = new CStreamReadBuff ( * ppIStream, CP_UTF_8 );
    if (pstreamReader == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(
        pstreamReader->GetStringValue(
            _T("Version"), szVersion, ARRAY_SIZE( szVersion ) ) );

    if (hr == S_FALSE)
        goto PlainStream;

    if (hr)
        goto Cleanup;

    hr = THR(
        pstreamReader->GetLongValue(
            _T( "StartHTML" ), & iStartHTML ) );

    if (hr == S_FALSE)
        goto PlainStream;

    if (hr)
        goto Cleanup;

    hr = THR(
        pstreamReader->GetLongValue(
            _T( "EndHTML" ), & iEndHTML ) );

    if (hr == S_FALSE)
        goto PlainStream;

    if (hr)
        goto Cleanup;

    hr = THR(
        pstreamReader->GetLongValue(
            _T( "StartFragment" ), & iStartFragment ) );

    if (hr == S_FALSE)
        goto PlainStream;

    if (hr)
        goto Cleanup;

    hr = THR(
        pstreamReader->GetLongValue(
            _T( "EndFragment" ), & iEndFragment ) );

    //
    // Locate optional contextual information
    //

    hr = THR(
        pstreamReader->GetLongValue(
            _T( "StartSelection" ), & iStartSelection ) );

    if (hr && hr != S_FALSE)
        goto Cleanup;

    if (hr != S_FALSE)
    {
        hr = THR(
            pstreamReader->GetLongValue(
                _T( "EndSelection" ), & iEndSelection ) );

        if (hr && hr != S_FALSE)
            goto Cleanup;

        if (hr == S_FALSE)
        {
            hr = E_FAIL;
            goto Cleanup;
        }
    }
    else
    {
        iStartSelection = -1;
    }

    //
    // Get the source URL info
    //

    hr = THR(
        pstreamReader->GetStringValue(
            _T( "SourceURL" ), szSourceUrl, ARRAY_SIZE( szSourceUrl ) ) );

    if (hr && hr != S_FALSE)
        goto Cleanup;

    if (phtmpasteinfo && hr != S_FALSE)
    {
        hr = THR( phtmpasteinfo->cstrSourceUrl.Set( szSourceUrl ) );

        if (hr)
            goto Cleanup;
    }

    //
    // Make sure contextual info is sane
    //

    if (iStartHTML < 0 && iEndHTML < 0)
    {
        //
        // per cfhtml spec, start and end html can be -1 if there is no
        // context.  there must always be a fragment, however
        //
        iStartHTML = iStartFragment;
        iEndHTML   = iEndFragment;
    }

    if (iStartHTML     < 0 || iEndHTML     < 0 ||
        iStartFragment < 0 || iEndFragment < 0 ||
        iStartHTML     > iEndHTML     ||
        iStartFragment > iEndFragment ||
        iStartHTML > iStartFragment ||
        iEndHTML   < iEndFragment)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (iStartSelection != -1)
    {
        if (iEndSelection < 0 ||
            iStartSelection > iEndSelection ||
            iStartHTML > iStartSelection ||
            iEndHTML < iEndSelection ||
            iStartFragment > iStartSelection ||
            iEndFragment < iEndSelection)
        {
            hr = E_FAIL;
            goto Cleanup;
        }
    }

    //
    // Rebase the fragment and selection off of the start html
    //

    iStartFragment -= iStartHTML;
    iEndFragment -= iStartHTML;

    if (iStartSelection != -1)
    {
        iStartSelection -= iStartHTML;
        iEndSelection -= iStartHTML;
    }
    else
    {
        iStartSelection = iStartFragment;
        iEndSelection = iEndFragment;
    }

    phtmpasteinfo->cbSelBegin  = iStartSelection;
    phtmpasteinfo->cbSelEnd    = iEndSelection;

    pstreamReader->SetPosition( iStartHTML );

    hr = S_OK;

Cleanup:

    if (pstreamReader)
        phtmpasteinfo->cp = pstreamReader->GetCodePage();

    delete pstreamReader;

    if (pszGlobal)
        GlobalUnlock( hGlobal );

    RRETURN( hr );

PlainStream:

    pstreamReader->SetPosition( 0 );
    pstreamReader->SwitchCodePage(g_cpDefault);

    if (fInsertFrags)
    {
        phtmpasteinfo->cbSelBegin  = fHasSignature ? sizeof( TCHAR ) : 0;
        phtmpasteinfo->cbSelEnd    = lSize;
    }
    else
    {
        phtmpasteinfo->cbSelBegin  = -1;
        phtmpasteinfo->cbSelEnd    = -1;
    }

    hr = S_OK;

    goto Cleanup;
}

static HRESULT
MoveToPointer (
    CDoc *           pDoc,
    CMarkupPointer * pMarkupPointer,
    CTreePos *       ptp )
{
    HRESULT hr = S_OK;

    if (!pMarkupPointer)
    {
        if (ptp)
        {
            hr = THR( ptp->GetMarkup()->RemovePointerPos( ptp, NULL, NULL ) );

            if (hr)
                goto Cleanup;
        }
        
        goto Cleanup;
    }

    if (!ptp)
    {
        hr = THR( pMarkupPointer->Unposition() );

        if (hr)
            goto Cleanup;

        goto Cleanup;
    }

    hr = THR( pMarkupPointer->MoveToOrphan( ptp ) );

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( hr );
}

HRESULT
CDoc::ParseGlobal (
    HGLOBAL          hGlobal,
    DWORD            dwFlags,
    CMarkup * *      ppMarkupResult,
    CMarkupPointer * pPointerSelStart,
    CMarkupPointer * pPointerSelFinish )
{
    HRESULT hr = S_OK;
    IStream * pIStream = NULL;
    HTMPASTEINFO htmpasteinfo;

    Assert( pPointerSelStart );
    Assert( pPointerSelFinish );
    Assert( ppMarkupResult );

    //
    // Returning NULL suggests that there was absolutely nothing to parse.
    //
    
    *ppMarkupResult = NULL;
    
    if (!hGlobal)
        goto Cleanup;

    //
    // Prepare the stream ...
    //

    hr = THR( PrepareStream( hGlobal, & pIStream, TRUE, & htmpasteinfo ) );

    if (hr)
        goto Cleanup;

    //
    // If no stream was created, then there is nothing to parse
    //

    if (!pIStream)
        goto Cleanup;

    //
    // Create the new markup container which will receive the the bounty
    // of the parse
    //

    hr = THR( CreateMarkup( ppMarkupResult ) );

    if (hr)
        goto Cleanup;

    Assert( *ppMarkupResult );
    
    //
    // Prepare the frag/sel begin/end for the return 
    //

    htmpasteinfo.ptpSelBegin = NULL;
    htmpasteinfo.ptpSelEnd = NULL;

    //
    // Parse the sucker
    //

    _fPasteIE40Absolutify = dwFlags & PARSE_ABSOLUTIFYIE40URLS;

    Assert( !_fMarkupServicesParsing );
    
    _fMarkupServicesParsing = TRUE;

    hr = THR( (*ppMarkupResult)->Load( pIStream, & htmpasteinfo ) );
    
    Assert( _fMarkupServicesParsing );
    
    _fMarkupServicesParsing = FALSE;

    if (hr)
        goto Cleanup;

    //
    // Move the pointers to the pointer pos' the parser left in
    //

    hr = THR(
        MoveToPointer(
            this, pPointerSelStart,  htmpasteinfo.ptpSelBegin ) );

    if (hr)
        goto Cleanup;
    
    hr = THR(
        MoveToPointer(
            this, pPointerSelFinish, htmpasteinfo.ptpSelEnd   ) );

    if (hr)
        goto Cleanup;

#if DBG == 1
    pPointerSelStart->SetDebugName( _T( "Selection Start" ) );
    pPointerSelFinish->SetDebugName( _T( "Selection Finish" ) );
#endif

    //
    // Make sure the finish is (totally) ordered properly with
    // respect to the begin
    //

    if (pPointerSelStart->IsPositioned() && pPointerSelFinish->IsPositioned())
        EnsureLogicalOrder( pPointerSelStart, pPointerSelFinish );

Cleanup:

    ReleaseInterface( pIStream );

    RRETURN( hr );
}

HRESULT
CDoc::ParseGlobal (
    HGLOBAL              hGlobal,
    DWORD                dwFlags,
    IMarkupContainer * * ppIContainerResult,
    IMarkupPointer *     pIPointerSelStart,
    IMarkupPointer *     pIPointerSelFinish )
{
    HRESULT          hr = S_OK;
    CMarkup *        pMarkup = NULL;
    CMarkupPointer * pPointerSelStart = NULL;
    CMarkupPointer * pPointerSelFinish = NULL;

    if (!ppIContainerResult)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppIContainerResult = NULL;

    if (!hGlobal)
        goto Cleanup;

    if (pIPointerSelStart)
    {
        hr = THR(
            pIPointerSelStart->QueryInterface(
                CLSID_CMarkupPointer, (void **) & pPointerSelStart ) );

        if (hr)
            goto Cleanup;
    }

    if (pIPointerSelFinish)
    {
        hr = THR(
            pIPointerSelFinish->QueryInterface(
                CLSID_CMarkupPointer, (void **) & pPointerSelFinish ) );

        if (hr)
            goto Cleanup;
    }

    hr = THR(
        ParseGlobal(
            hGlobal, dwFlags, & pMarkup, pPointerSelStart, pPointerSelFinish ) );

    if (hr)
        goto Cleanup;

    if (pMarkup)
    {
        hr = THR(
            pMarkup->QueryInterface(
                IID_IMarkupContainer, (void **) ppIContainerResult ) );

        if (hr)
            goto Cleanup;
    }

Cleanup:

    if (pMarkup)
        pMarkup->Release();

    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Functions:  Equal & Compare
//
//  Synopsis:   Helpers for comparing IMarkupPointers
//
//-----------------------------------------------------------------------------

static inline BOOL
IsEqualTo ( IMarkupPointer * p1, IMarkupPointer * p2 )
{
    BOOL fEqual;
    IGNORE_HR( p1->IsEqualTo( p2, & fEqual ) );
    return fEqual;
}

static inline int
OldCompare ( IMarkupPointer * p1, IMarkupPointer * p2 )
{
    int result;
    IGNORE_HR( OldCompare( p1, p2, & result ) );
    return result;
}

//+----------------------------------------------------------------------------
//
//  Function:   ComputeTotalOverlappers
//
//  Synopsis:   This function retunrs a node which can be found above the
//              finish pointer.  All elements starting at this node and above
//              it are in the scope of both the start and finish pointers.
//              
//              If an element were to be inserted between the pointers, that
//              element would finish just above this node.  All elements above
//              the return node, including the return element, begin before
//              the start pointer and finish after the finish pointer.
//
//-----------------------------------------------------------------------------

typedef CStackPtrAry < CTreeNode *, 32 > NodeArray;

MtDefine( ComputeTotalOverlappers_aryNodes_pv, Locals, "ComputeTotalOverlappers aryNodes::_pv" );

static HRESULT
ComputeTotalOverlappers(
    CMarkupPointer * pPointerStart,
    CMarkupPointer * pPointerFinish,
    CTreeNode * *    ppNodeTarget )
{
    HRESULT     hr = S_OK;
    CTreeNode * pNode;
    NodeArray   aryNodesStart( Mt( ComputeTotalOverlappers_aryNodes_pv ) );
    NodeArray   aryNodesFinish( Mt( ComputeTotalOverlappers_aryNodes_pv ) );
    int         iStart, iFinish;

    Assert( ppNodeTarget );
    Assert( pPointerStart && pPointerFinish );
    Assert( pPointerStart->IsPositioned() );
    Assert( pPointerFinish->IsPositioned() );
    Assert( pPointerStart->Markup() == pPointerFinish->Markup() );
    Assert( OldCompare( pPointerStart, pPointerFinish ) <= 0 );

    *ppNodeTarget = NULL;

    for ( pNode = pPointerStart->Branch() ; pNode ; pNode = pNode->Parent() )
        IGNORE_HR( aryNodesStart.Append( pNode ) );

    for ( pNode = pPointerFinish->Branch() ; pNode ; pNode = pNode->Parent() )
        IGNORE_HR( aryNodesFinish.Append( pNode ) );

    iStart = aryNodesStart.Size() - 1;
    iFinish = aryNodesFinish.Size() - 1;

    for ( ; ; )
    {
        if (iStart < 0 || iFinish < 0)
        {
            if (iFinish + 1 < aryNodesFinish.Size())
                *ppNodeTarget = aryNodesFinish[ iFinish + 1 ];

            goto Cleanup;
        }

        if (aryNodesStart [ iStart ] == aryNodesFinish [ iFinish ])
            iFinish--;

        iStart--;
    }

Cleanup:

    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Function:   ValidateElements
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------

MtDefine( ValidateElements_aryNodes_pv, Locals, "ValidateElements aryNodes::_pv" );

//
// BUGBUG
//
// This function is broken in the general case.  First, we need to add a flag
// to validate copies v.s. moves.  Also, it can only validate either inplace
// (pPointerTarget is NULL) or a tree-to-tree move.  Luckily, these are the
// only ways this function is used in IE5.
//

HRESULT
CDoc::ValidateElements (
    CMarkupPointer *   pPointerStart,
    CMarkupPointer *   pPointerFinish,
    CMarkupPointer *   pPointerTarget,
    DWORD              dwFlags,
    CMarkupPointer * * ppPointerStatus,
    CTreeNode * *      ppNodeFailBottom,
    CTreeNode * *      ppNodeFailTop )
{  
    HRESULT     hr = S_OK;
    NodeArray   aryNodes ( Mt( ValidateElements_aryNodes_pv ) );
    long        nNodesOk;
    CTreeNode * pNodeCommon;
    CTreeNode * pNode;
    CTreePos *  ptpWalk;
    CTreePos *  ptpFinish;

    Assert( pPointerStart && pPointerFinish && ppPointerStatus );
    Assert( pPointerStart->IsPositioned() );
    Assert( pPointerFinish->IsPositioned() );
    Assert( pPointerStart->Markup() ==pPointerFinish->Markup() );
    Assert( !pPointerTarget  || pPointerTarget->IsPositioned() );
    Assert( ppPointerStatus );
    Assert( !*ppPointerStatus || (*ppPointerStatus)->IsPositioned() );
    Assert( !*ppPointerStatus || (*ppPointerStatus)->Markup() == pPointerStart->Markup() );

    //
    // BUGBUG!!! Should not have to embed pointers here!
    //

    hr = THR( pPointerStart->Markup()->EmbedPointers() );

    if (hr)
        goto Cleanup;

    if (pPointerTarget)
    {
        hr = THR( pPointerTarget->Markup()->EmbedPointers() );

        if (hr)
            goto Cleanup;
    }

    //
    // If the status pointer is NULL, then we are validating from the
    // beginning.  Otherwise, we are to continue validating from that
    // position.
    //

    if (!*ppPointerStatus)
    {
        hr = THR( CreateMarkupPointer( ppPointerStatus ) );

        if (hr)
            goto Cleanup;

        hr = THR( (*ppPointerStatus)->SetGravity( POINTER_GRAVITY_Left ) );

        if (hr)
            goto Cleanup;
        
        hr = THR( (*ppPointerStatus)->MoveToPointer( pPointerStart ) );

        if (hr)
            goto Cleanup;
    }

    WHEN_DBG( (*ppPointerStatus)->SetDebugName( _T( "Validate Status" ) ) );

    //
    // If the status pointer is outside the range to validate, then
    // the validation is ended.  In this case, release the status pointer
    // and return.
    //

    if (OldCompare( pPointerStart, *ppPointerStatus ) > 0 ||
        OldCompare( pPointerFinish, *ppPointerStatus ) <= 0)
    {
        (*ppPointerStatus)->Release();
        *ppPointerStatus = NULL;
        
        goto Cleanup;
    }

    //
    // Get the common node for the range to validate.  Anything above
    // and including the common node cannot be a candidate for source
    // validation because they must cover the entire range and will not
    // participate in a move.
    //

    {
        CTreeNode * pNodeStart  = pPointerStart->Branch();
        CTreeNode * pNodeFinish = pPointerFinish->Branch();

        pNodeCommon =
            (pNodeStart && pNodeFinish)
                ? pNodeStart->GetFirstCommonAncestorNode( pNodeFinish, NULL )
                : NULL;
    }
    
    //
    // Here we prime the tag array with target elements (if any).
    //

    {
        CTreeNode * pNodeTarget;
        int         nNodesTarget = 0, i;
        
        if (pPointerTarget && pPointerTarget->IsPositioned())
        {
            //
            // If the target is in the range of the source, then compute
            // the branch which consists of elements which totally overlap
            // the range to validate.  These are the elements which would
            // remain of the range were be removed.
            //
            // If the target is not in the validation range, then simply use
            // the base of the target.
            //

            if (pPointerTarget->Markup() == pPointerStart->Markup() &&
                OldCompare( pPointerStart,  pPointerTarget ) < 0 &&
                    OldCompare( pPointerFinish, pPointerTarget ) > 0)
            {
AssertSz( 0, "This path should never be used in IE5" );
                hr = THR(
                    ComputeTotalOverlappers(
                        pPointerStart, pPointerFinish, & pNodeTarget ) );

                if (hr)
                    goto Cleanup;
            }
            else
            {
                pNodeTarget = pPointerTarget->Branch();
            }
        }
        else
        {
            pNodeTarget = pNodeCommon;
        }

        //
        // Now, put tags in the tag array starting from the target
        //

        for ( pNode = pNodeTarget ;
              pNode && pNode->Tag() != ETAG_ROOT ;
              pNode = pNode->Parent() )
        {
            nNodesTarget++;
        }

        hr = THR( aryNodes.Grow( nNodesTarget ) );

        if (hr)
            goto Cleanup;
        
        for ( i = 1, pNode = pNodeTarget ;
              pNode && pNode->Tag() != ETAG_ROOT ;
              pNode = pNode->Parent(), i++ )
        {
            aryNodes [ nNodesTarget - i ] = pNode;
        }
    }
    
    //
    // During the validation walk, the mark bit on the element will
    // indicate whether or not the element will participate in a move
    // if the range to validate were moved to another location.  A mark
    // or 1 indicates that the element participates, 0 means it does not.
    //
    // By setting all the bits (up to the common node) on the left branch
    // to 1 and then clearing all the bits on the right branch, all elements
    // on the first branch which partially overlap the left side of the
    // range to validate.
    //
    // Note, this only applies if a target has been specified.  If we
    // are validating inplacem, then we simply validate all elements
    // in scope.
    //

    for ( pNode = pPointerStart->Branch() ;
          pNode != pNodeCommon ;
          pNode = pNode->Parent() )
    {
        pNode->Element()->_fMark1 = 1;
    }

    //
    // Only clear the bits if a tree-to-tree move validation is being
    // performed.  If not, then we leave all the marks up to the common
    // element on.  Thus, all elements in scope will be validated against.
    //

    if (pPointerTarget)
    {
        for ( pNode = pPointerFinish->Branch() ;
              pNode != pNodeCommon ;
              pNode = pNode->Parent() )
        {
            pNode->Element()->_fMark1 = 0;
        }
    }

    //
    // Now, walk from the start pointer to the status pointer, setting the
    // mark bit to true on any elements comming into scope.  This needs to
    // be done to make sure we include these elements in the validation
    // because they have come into scope.  We also do this as we walk the
    // main loop later, doing the actual validation.
    //
    // Note: We do a Compare here because the pointers may be at the same
    // place in the markup, but the start is after the status pointer.
    //

    ptpWalk = pPointerStart->GetEmbeddedTreePos();
    
    if (! IsEqualTo( *ppPointerStatus, pPointerStart ))
    {
        CTreePos * ptpStatus = (*ppPointerStatus)->GetEmbeddedTreePos();

        for ( ; ptpWalk != ptpStatus ; ptpWalk = ptpWalk->NextTreePos() )
        {
            if (ptpWalk->IsBeginElementScope())
                ptpWalk->Branch()->Element()->_fMark1 = TRUE;
        }
    }

    //
    // Now, prime the tag array with the current set of source elements.
    // As we walk the validation range, we will add or remove elements
    // as they enter and exit scope.
    //

    {
        int i;
        
        for ( pNode = ptpWalk->GetBranch(), i = 0 ;
              pNode != pNodeCommon ;
              pNode = pNode->Parent() )
        {
            if (pNode->Element()->_fMark1)
                i++;
        }

        hr = THR( aryNodes.Grow( aryNodes.Size() + i ) );

        if (hr)
            goto Cleanup;

        i = aryNodes.Size() - 1;
        
        for ( pNode = ptpWalk->GetBranch() ;
              pNode != pNodeCommon ;
              pNode = pNode->Parent() )
        {
            if (pNode->Element()->_fMark1)
                aryNodes[ i-- ] = pNode;
        }
    }

    //
    // This is the 'main' loop where validation actually takes place.
    //

    ptpFinish = pPointerFinish->GetEmbeddedTreePos();
    nNodesOk = 0;
    
    for ( ; ; )
    {
        BOOL fDone;
        long iConflictTop, iConflictBottom;
        
        //
        // Validate the current tag array
        //

        extern HRESULT
            ValidateNodeList (
                CTreeNode **, long, long, BOOL, long *, long *);

        hr = THR(
            ValidateNodeList(
                aryNodes, aryNodes.Size(), nNodesOk,
                dwFlags & VALIDATE_ELEMENTS_REQUIREDCONTAINERS,
                & iConflictTop, & iConflictBottom ) );

        //
        // Returning S_FALSE indicates a conflict
        //

        if (hr == S_FALSE)
        {
            CTreePosGap gap ( ptpWalk, TPG_LEFT );
            
            hr = THR( (*ppPointerStatus)->MoveToGap( & gap, pPointerStart->Markup() ) );

            if (hr)
                goto Cleanup;

            if (ppNodeFailBottom)
                *ppNodeFailBottom = aryNodes[ iConflictBottom ];
            
            if (ppNodeFailTop)
                *ppNodeFailTop = aryNodes[ iConflictTop ];

            hr = S_FALSE;

            goto Cleanup;
        }

        if (hr)
            goto Cleanup;

        //
        // Since we've just validated the entire array, don't do it again
        // next time 'round.
        //

        nNodesOk = aryNodes.Size();

        //
        // Now, scan forward looking for an interesting event.  If we get
        // to the finish before that, we are done.
        //

        for ( fDone = FALSE ; ; )
        {
            ptpWalk = ptpWalk->NextTreePos();

            if (ptpWalk == ptpFinish)
            {
                fDone = TRUE;
                break;
            }

            //
            // If we run accross an edge, either push that tag onto the
            // stack, or remove it.
            //

            if (ptpWalk->IsNode())
            {
                Assert( ptpWalk->IsBeginNode() || ptpWalk->IsEndNode() );
                
                if (ptpWalk->IsBeginNode())
                {
                    Assert( ptpWalk->IsEdgeScope() );
                    
                    pNode = ptpWalk->Branch();
                    pNode->Element()->_fMark1 = 1;

                    IGNORE_HR( aryNodes.Append( pNode ) );
                }
                else
                {
                    int cIncl, cPop;
                    //
                    // Walk the first half of the inclusion
                    //
                    
                    for ( cIncl = cPop = 0 ;
                          ! ptpWalk->IsEdgeScope() ;
                          cIncl++, ptpWalk = ptpWalk->NextTreePos() )
                    {
                        Assert( ptpWalk != ptpFinish && ptpWalk->IsEndNode() );
                        
                        Assert(
                            !ptpWalk->Branch()->Element()->_fMark1 ||
                            aryNodes [ aryNodes.Size() - cPop - 1 ] == ptpWalk->Branch() );

                        //
                        // We're counting the number of items to pop off the stack, not
                        // the number of non-edge nodes to the left of the inlcusion
                        //
                        
                        if (ptpWalk->Branch()->Element()->_fMark1)
                            cPop++;
                    }

                    //
                    // Mae sure we got to the kernel of the inclusion
                    //

                    Assert( ptpWalk->IsEndNode() );
                    Assert( aryNodes [ aryNodes.Size() - cPop - 1 ] == ptpWalk->Branch() );
                    
                    //
                    // Pop the number of elements before the one going out of scope plus
                    // the real one going out of scope;
                    //

                    aryNodes.SetSize( aryNodes.Size() - cPop - 1 );

                    //
                    // Reset the number of nodes which have already been verified to the current
                    // size of the stack.
                    //
                    
                    nNodesOk = aryNodes.Size();

                    //
                    // Walk the right hand side of the inclusion, putting the non
                    // kernel nodes back on.
                    //

                    while ( cIncl-- )
                    {
                        ptpWalk = ptpWalk->NextTreePos();

                        Assert( ptpWalk->IsBeginNode() && ! ptpWalk->IsEdgeScope() );

                        //
                        // Make sure we don't put an element on which does not participate
                        // in the "move".
                        //
                        
                        if (ptpWalk->Branch()->Element()->_fMark1)
                            aryNodes.Append( ptpWalk->Branch() );
                    }
                }

                break;
            }
        }

        if (fDone)
            break;
    }

    //
    // If we are here, then the validation got through with out
    // any conflicts.  In this case, clear the status pointer.
    //

    Assert( *ppPointerStatus );

    (*ppPointerStatus)->Release();
    
    *ppPointerStatus = NULL;

    Assert( hr == S_OK );

Cleanup:

    RRETURN1( hr, S_FALSE );
}

HRESULT 
CDoc::BeginUndoUnit( OLECHAR * pchDescription )
{
    HRESULT hr = S_OK;
    
    if (!pchDescription)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!_uOpenUnitsCounter)
    {
        _pMarkupServicesParentUndo = new CParentUndo( this );
        
        if (!_pMarkupServicesParentUndo)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR( _pMarkupServicesParentUndo->Start( pchDescription ) );
    }        

    {
        CSelectionUndo Undo( _pElemCurrent, GetCurrentMarkup() );    
    }
    
    _uOpenUnitsCounter++;

Cleanup:
    
    RRETURN( hr );
}

HRESULT 
CDoc::EndUndoUnit ( )
{
    HRESULT hr = S_OK;

    if (!_uOpenUnitsCounter)
        goto Cleanup;

    _uOpenUnitsCounter--;

    {
        CDeferredSelectionUndo DeferredUndo( _pPrimaryMarkup );
    }
    
    if (_uOpenUnitsCounter == 0)
    {
        Assert( _pMarkupServicesParentUndo );
        
        hr = _pMarkupServicesParentUndo->Finish( S_OK );
        
        delete _pMarkupServicesParentUndo;
    }

Cleanup:
    
    RRETURN( hr );
}

HRESULT
CDoc::IsScopedElement ( IHTMLElement * pIHTMLElement, BOOL * pfScoped )
{
    HRESULT hr = S_OK;
    CElement * pElement = NULL;

    if (!pIHTMLElement || !pfScoped || !IsOwnerOf( pIHTMLElement ))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pIHTMLElement->QueryInterface( CLSID_CElement, (void **) & pElement ) );
    
    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pfScoped = ! pElement->IsNoScope();
    
Cleanup:
    
    RRETURN( hr );
}


//
// BUGBUG: These switchs are not the bast way to tdo this.  .asc perhaps ?
//

#ifdef VSTUDIO7
ELEMENT_TAG_ID
#else
static ELEMENT_TAG_ID
#endif //VSTUDIO7
TagIdFromETag ( ELEMENT_TAG etag )
{
    ELEMENT_TAG_ID tagID = TAGID_NULL;
    
    switch( etag )
    {
#define X(Y) case ETAG_##Y:tagID=TAGID_##Y;break;
    X(UNKNOWN) X(A) X(ACRONYM) X(ADDRESS) X(APPLET) X(AREA) X(B) X(BASE) X(BASEFONT)
    X(BDO) X(BGSOUND) X(BIG) X(BLINK) X(BLOCKQUOTE) X(BODY) X(BR) X(BUTTON) X(CAPTION)
    X(CENTER) X(CITE) X(CODE) X(COL) X(COLGROUP) X(COMMENT) X(DD) X(DEL) X(DFN) X(DIR)
    X(DIV) X(DL) X(DT) X(EM) X(EMBED) X(FIELDSET) X(FONT) X(FORM) X(FRAME)
    X(FRAMESET) X(H1) X(H2) X(H3) X(H4) X(H5) X(H6) X(HEAD) X(HR) X(HTML) X(I) X(IFRAME)
    X(IMG) X(INPUT) X(INS) X(KBD) X(LABEL) X(LEGEND) X(LI) X(LINK) X(LISTING)
    X(MAP) X(MARQUEE) X(MENU) X(META) X(NEXTID) X(NOBR) X(NOEMBED) X(NOFRAMES)
    X(NOSCRIPT) X(OBJECT) X(OL) X(OPTION) X(P) X(PARAM) X(PLAINTEXT) X(PRE) X(Q)
#ifdef  NEVER
    X(HTMLAREA)
#endif
    X(RP) X(RT) X(RUBY) X(S) X(SAMP) X(SCRIPT) X(SELECT) X(SMALL) X(SPAN) 
    X(STRIKE) X(STRONG) X(STYLE) X(SUB) X(SUP) X(TABLE) X(TBODY) X(TC) X(TD) X(TEXTAREA)
    X(TFOOT) X(TH) X(THEAD) X(TR) X(TT) X(U) X(UL) X(VAR) X(WBR) X(XMP)
#undef X
        
    case ETAG_TITLE_ELEMENT :
    case ETAG_TITLE_TAG :
        tagID = TAGID_TITLE; break;
        
    case ETAG_GENERIC :
    case ETAG_GENERIC_BUILTIN :
    case ETAG_GENERIC_LITERAL :
        tagID = TAGID_GENERIC; break;
        
    case ETAG_RAW_COMMENT :
        tagID = TAGID_COMMENT_RAW; break;

    // BUGBUG (MohanB) For now, use INPUT's tag id for TXTSLAVE. Needs to be generified.
    case ETAG_TXTSLAVE :
        tagID = TAGID_INPUT; break;
    }

    AssertSz( tagID != TAGID_NULL, "Invalid ELEMENT_TAG" );

    return tagID;
}
    
ELEMENT_TAG
ETagFromTagId ( ELEMENT_TAG_ID tagID )
{
    ELEMENT_TAG etag = ETAG_NULL;
    
    switch( tagID )
    {
#define X(Y) case TAGID_##Y:etag=ETAG_##Y;break;
    X(UNKNOWN) X(A) X(ACRONYM) X(ADDRESS) X(APPLET) X(AREA) X(B) X(BASE) X(BASEFONT)
    X(BDO) X(BGSOUND) X(BIG) X(BLINK) X(BLOCKQUOTE) X(BODY) X(BR) X(BUTTON) X(CAPTION)
    X(CENTER) X(CITE) X(CODE) X(COL) X(COLGROUP) X(COMMENT) X(DD) X(DEL) X(DFN) X(DIR)
    X(DIV) X(DL) X(DT) X(EM) X(EMBED) X(FIELDSET) X(FONT) X(FORM) X(FRAME)
    X(FRAMESET) X(GENERIC) X(H1) X(H2) X(H3) X(H4) X(H5) X(H6) X(HEAD) X(HR) X(HTML) X(I) X(IFRAME)
    X(IMG) X(INPUT) X(INS) X(KBD) X(LABEL) X(LEGEND) X(LI) X(LINK) X(LISTING)
    X(MAP) X(MARQUEE) X(MENU) X(META) X(NEXTID) X(NOBR) X(NOEMBED) X(NOFRAMES)
    X(NOSCRIPT) X(OBJECT) X(OL) X(OPTION) X(P) X(PARAM) X(PLAINTEXT) X(PRE) X(Q)
#ifdef  NEVER
    X(HTMLAREA)
#endif
    X(RP) X(RT) X(RUBY) X(S) X(SAMP) X(SCRIPT) X(SELECT) X(SMALL) X(SPAN) 
    X(STRIKE) X(STRONG) X(STYLE) X(SUB) X(SUP) X(TABLE) X(TBODY) X(TC) X(TD) X(TEXTAREA) 
    X(TFOOT) X(TH) X(THEAD) X(TR) X(TT) X(U) X(UL) X(VAR) X(WBR) X(XMP)
#undef X
            
    case TAGID_TITLE : etag = ETAG_TITLE_ELEMENT; break;
    case TAGID_COMMENT_RAW : etag = ETAG_RAW_COMMENT; break;
    }

    AssertSz( etag != ETAG_NULL, "Invalid ELEMENT_TAG_ID" );

    return etag;
}

HRESULT
CDoc::GetElementTagId ( IHTMLElement * pIHTMLElement, ELEMENT_TAG_ID * ptagId )
{
    HRESULT hr;
    CElement * pElement = NULL;

    if (!pIHTMLElement || !ptagId || !IsOwnerOf( pIHTMLElement ))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pIHTMLElement->QueryInterface( CLSID_CElement, (void **) & pElement ) );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ptagId = TagIdFromETag( pElement->Tag() );
    
Cleanup:

    RRETURN( hr );
}

HRESULT
CDoc::GetTagIDForName ( BSTR bstrName, ELEMENT_TAG_ID * ptagId )
{
    HRESULT hr = S_OK;
    
    if (!bstrName || !ptagId)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ptagId = TagIdFromETag( EtagFromName( bstrName, SysStringLen( bstrName ) ) );

Cleanup:

    RRETURN( hr );
}

HRESULT
CDoc::GetNameForTagID ( ELEMENT_TAG_ID tagId, BSTR * pbstrName )
{
    HRESULT hr = S_OK;
    
    if (!pbstrName)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( FormsAllocString( NameFromEtag( ETagFromTagId( tagId ) ), pbstrName ) );

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( hr );
}

HRESULT
CDoc::CreateElement (
    ELEMENT_TAG_ID   tagID,
    OLECHAR *        pchAttributes,
    IHTMLElement * * ppIHTMLElement )
{
    HRESULT     hr = S_OK;
    ELEMENT_TAG etag;
    CElement *  pElement = NULL;

    if (!ppIHTMLElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    etag = ETagFromTagId( tagID );

    if (etag == ETAG_NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(
        CreateElement(
            etag, & pElement,
            pchAttributes, pchAttributes ? _tcslen( pchAttributes ) : 0 ) );

    if (hr)
        goto Cleanup;

    hr = THR( pElement->QueryInterface( IID_IHTMLElement, (void **) ppIHTMLElement ) );

    if (hr)
        goto Cleanup;

Cleanup:

    CElement::ReleasePtr( pElement );

    RRETURN( hr );
}

HRESULT
CDoc::CloneElement (
    IHTMLElement *  pElementCloneThis,
    IHTMLElement ** ppElementClone )
{
    HRESULT hr;
    IHTMLDOMNode *pThisDOMNode = NULL;
    IHTMLDOMNode *pDOMNode = NULL;

    if (!pElementCloneThis || !ppElementClone)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pElementCloneThis->QueryInterface( IID_IHTMLDOMNode, (void **) & pThisDOMNode ) );

    if (hr)
        goto Cleanup;

    // BUBUG rgardner - should this be Deep ?
    hr = THR( pThisDOMNode->cloneNode( FALSE /* Not Deep */, &pDOMNode ) );
    if (hr)
        goto Cleanup;

    hr = THR( pDOMNode->QueryInterface ( IID_IHTMLElement, (void**)ppElementClone ));
    if (hr)
        goto Cleanup;

Cleanup:

    ClearInterface( & pThisDOMNode );
    ClearInterface( & pDOMNode );
    
    RRETURN( hr );
}

HRESULT
CDoc::CreateMarkupContainer ( IMarkupContainer * * ppIMarkupContainer )
{
    HRESULT   hr;
    CMarkup * pMarkup = NULL;

    if (!ppIMarkupContainer)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( CreateMarkup( & pMarkup ) );
    
    if (hr)
        goto Cleanup;

    hr = THR(
        pMarkup->QueryInterface(
            IID_IMarkupContainer, (void **) ppIMarkupContainer ) );
    
    if (hr)
        goto Cleanup;

Cleanup:

    if (pMarkup)
        pMarkup->Release();

    RRETURN( hr );
}


///////////////////////////////////////////////////////
//  tree service helper functions


BOOL
CDoc::IsOwnerOf ( IHTMLElement * pIElement )
{
    HRESULT    hr;
    BOOL       result = FALSE;
    CElement * pElement;

    hr = THR( pIElement->QueryInterface( CLSID_CElement, (void **) & pElement ) );
    
    if (hr)
        goto Cleanup;

    result = this == pElement->Doc();

Cleanup:
    return result;
}


BOOL
CDoc::IsOwnerOf ( IMarkupPointer * pIPointer )
{
    HRESULT         hr;
    BOOL            result = FALSE;
    CMarkupPointer  *pPointer;

    hr =  THR(pIPointer->QueryInterface( CLSID_CMarkupPointer, (void **) & pPointer ) );
    if (hr)
        goto Cleanup;

    result = (this == pPointer->Doc());

Cleanup:        
    return result;
}

BOOL
CDoc::IsOwnerOf ( IHTMLTxtRange * pIRange )
{
    HRESULT         hr;
    BOOL            result = FALSE;
    CAutoRange *    pRange;

    hr = THR( pIRange->QueryInterface( CLSID_CRange, (void **) & pRange ) );
    
    if (hr)
        goto Cleanup;

    result = this == pRange->GetMarkup()->Doc();

Cleanup:
    
    return result;
}

BOOL
CDoc::IsOwnerOf ( IMarkupContainer * pContainer )
{
    HRESULT          hr;
    BOOL             result = FALSE;
    CMarkup         *pMarkup;

    hr = THR(pContainer->QueryInterface(CLSID_CMarkup, (void **)&pMarkup));
    if (hr)
        goto Cleanup;

    result = (this == pMarkup->Doc());

Cleanup:
    return result;
}

HRESULT
CDoc::CutCopyMove (
    IMarkupPointer * pIPointerStart,
    IMarkupPointer * pIPointerFinish,
    IMarkupPointer * pIPointerTarget,
    BOOL             fRemove )
{
    HRESULT          hr = S_OK;
    CMarkupPointer * pPointerStart;
    CMarkupPointer * pPointerFinish;
    CMarkupPointer * pPointerTarget = NULL;

    //
    // Check argument sanity
    //
    
    if (pIPointerStart  == NULL  || !IsOwnerOf( pIPointerStart  ) ||
        pIPointerFinish == NULL  || !IsOwnerOf( pIPointerFinish ) ||
        (pIPointerTarget != NULL && !IsOwnerOf( pIPointerTarget )) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Get the internal objects
    //
    
    hr = THR(
        pIPointerStart->QueryInterface(
            CLSID_CMarkupPointer, (void **) & pPointerStart ) );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    hr = THR(
        pIPointerFinish->QueryInterface(
            CLSID_CMarkupPointer, (void **) & pPointerFinish ) );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (pIPointerTarget)
    {
        hr = THR(
            pIPointerTarget->QueryInterface(
                CLSID_CMarkupPointer, (void **) & pPointerTarget ) );

        if (hr)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }

    //
    // More sanity checks
    //

    if (!pPointerStart->IsPositioned() || !pPointerFinish->IsPositioned())
    {
        hr = CTL_E_UNPOSITIONEDPOINTER;
        goto Cleanup;
    }
    
    if (pPointerStart->Markup() != pPointerFinish->Markup())
    {
        hr = CTL_E_INCOMPATIBLEPOINTERS;
        goto Cleanup;
    }

    //
    // Make sure the start if before the finish
    //

    EnsureLogicalOrder( pPointerStart, pPointerFinish );

    //
    // More checks
    //

    if (pPointerTarget && !pPointerTarget->IsPositioned())
    {
        hr = CTL_E_UNPOSITIONEDPOINTER;
        goto Cleanup;
    }

    //
    // Do it
    //

    hr = THR(
        CutCopyMove(
            pPointerStart, pPointerFinish, pPointerTarget, fRemove, NULL ) );

Cleanup:

    RRETURN( hr );
}

HRESULT
CDoc::CutCopyMove (
    CMarkupPointer * pPointerStart,
    CMarkupPointer * pPointerFinish,
    CMarkupPointer * pPointerTarget,
    BOOL             fRemove,
    DWORD            dwFlags )
{
    HRESULT     hr = S_OK;
    CTreePosGap tpgStart;
    CTreePosGap tpgFinish;
    CTreePosGap tpgTarget;
    CMarkup *   pMarkupSource = NULL;
    CMarkup *   pMarkupTarget = NULL;

    //
    // Sanity check the args
    //

    Assert( pPointerStart );
    Assert( pPointerFinish );
    Assert( OldCompare( pPointerStart, pPointerFinish ) <= 0 );
    Assert( pPointerStart->IsPositioned() );
    Assert( pPointerFinish->IsPositioned() );
    Assert( pPointerStart->Markup() == pPointerFinish->Markup() );
    Assert( ! pPointerTarget || pPointerTarget->IsPositioned() );

    //
    // Make sure unembedded pointers get in before the modification
    //

    hr = THR( pPointerStart->Markup()->EmbedPointers() );

    if (hr)
        goto Cleanup;

    if (pPointerTarget)
    {
        hr = THR( pPointerTarget->Markup()->EmbedPointers() );

        if (hr)
            goto Cleanup;
    }

    //
    // Set up the gaps
    //
    
    tpgStart.MoveTo( pPointerStart->GetEmbeddedTreePos(), TPG_LEFT );
    tpgFinish.MoveTo( pPointerFinish->GetEmbeddedTreePos(), TPG_RIGHT );
    
    if (pPointerTarget)
        tpgTarget.MoveTo( pPointerTarget->GetEmbeddedTreePos(), TPG_LEFT );

    pMarkupSource = pPointerStart->Markup();

    if (pPointerTarget)
        pMarkupTarget = pPointerTarget->Markup();

#if TREE_SYNC
/**/    // this is a big HACK, to temporarily get markup-sync working for netdocs.  it
/**/    // will totally change in the future.
/**/    // trident team: do not bother trying to maintain this code, delete if it causes problems
/**/    if (_SyncLogger.IsLogging())
/**/    {
/**/        hr = THR(_SyncLogger.CutCopyMoveHandler(pPointerStart,pPointerFinish,pPointerTarget,fRemove));
/**/        if (hr != S_OK)
/**/        {
/**/            goto Cleanup;
/**/        }
/**/    }
#endif // TREE_SYNC

    //
    // Do it.
    //
    
    if (pPointerTarget)
    {
        hr = THR(
            pMarkupSource->SpliceTreeInternal(
                & tpgStart, & tpgFinish, pPointerTarget->Markup(),
                & tpgTarget, fRemove, dwFlags ) );

        if (hr)
            goto Cleanup;
    }
    else
    {
        Assert( fRemove );
        
        hr = THR(
            pMarkupSource->SpliceTreeInternal(
                & tpgStart, & tpgFinish, NULL, NULL, fRemove, dwFlags ) );

        if (hr)
            goto Cleanup;
    }

#if TREE_SYNC
/**/    // this is a big HACK, to temporarily get markup-sync working for netdocs.  it
/**/    // will totally change in the future.
/**/    // trident team: do not bother trying to maintain this code, delete if it causes problems
/**/    if (_SyncLogger.IsLogging())
/**/    {
/**/        THR(_SyncLogger.CutCopyMoveFinalizeHandler());
/**/    }
#endif // TREE_SYNC

Cleanup:

    RRETURN( hr );
}



HRESULT         
CDoc::CreateMarkup( CMarkup ** ppMarkup, CElement * pElementMaster /*= NULL*/ )
{
    HRESULT         hr;
    CRootElement *  pRootElement;
    CMarkup *       pMarkup = NULL;

    Assert( ppMarkup );

    pRootElement = new CRootElement( this );
    
    if (!pRootElement)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pMarkup = new CMarkup( this, pElementMaster );
    
    if (!pMarkup)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR( pRootElement->Init() );
    
    if (hr)
        goto Cleanup;

    {
        CElement::CInit2Context   context(NULL, pMarkup);

        hr = THR( pRootElement->Init2(&context) );
    }
    
    if (hr)
        goto Cleanup;

    hr = THR( pMarkup->Init( pRootElement ) );
    
    if (hr)
        goto Cleanup;

    *ppMarkup = pMarkup;
    pMarkup = NULL;

Cleanup:
    
    if (pMarkup)
        pMarkup->Release();

    CElement::ReleasePtr( pRootElement );

    RRETURN( hr );
}

HRESULT         
CDoc::CreateMarkupWithElement( 
    CMarkup ** ppMarkup, 
    CElement * pElement, 
    CElement * pElementMaster /*= NULL*/ )
{
    HRESULT   hr = S_OK;
    CMarkup * pMarkup = NULL;

    Assert( pElement && !pElement->IsInMarkup() );

    hr = THR( CreateMarkup( &pMarkup, pElementMaster ) );
    
    if (hr)
        goto Cleanup;

    // Insert the element into the empty tree
    
    {
        CTreePos * ptpRootBegin = pMarkup->FirstTreePos();
        CTreePos * ptpNew;
        CTreeNode *pNodeNew;

        Assert( ptpRootBegin );
    
        // Assert that the only thing in this tree is two WCH_NODE characters
        Assert( pMarkup->Cch() == 2 );

        pNodeNew = new CTreeNode( pMarkup->Root()->GetFirstBranch(), pElement );
        if( !pNodeNew )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        Assert( pNodeNew->GetEndPos()->IsUninit() );
        ptpNew = pNodeNew->InitEndPos( TRUE );
        hr = THR( pMarkup->Insert( ptpNew, ptpRootBegin, FALSE ) );
        if(hr)
        {
            // The node never made it into the tree
            // so delete it
            delete pNodeNew;

            goto Cleanup;
        }

        Assert( pNodeNew->GetBeginPos()->IsUninit() );
        ptpNew = pNodeNew->InitBeginPos( TRUE );
        hr = THR( pMarkup->Insert( ptpNew, ptpRootBegin, FALSE ) );
        if(hr)
            goto Cleanup;

        pNodeNew->PrivateEnterTree();

        pElement->SetMarkupPtr( pMarkup );
        pElement->__pNodeFirstBranch = pNodeNew;
        pElement->PrivateEnterTree();

        {
            CNotification   nf;
            nf.ElementEntertree(pElement);
            pElement->Notify(&nf);
        }

        // Insert the WCH_NODE characters for the element
        // The 2 is hardcoded since we know that there are
        // only 2 WCH_NODE characters for the root
        Verify(
            ULONG(
                CTxtPtr( pMarkup, 2 ).
                    InsertRepeatingChar( 2, WCH_NODE ) ) == 2 );

        // Don't send a notification but do update the 
        // debug character count
        WHEN_DBG( pMarkup->_cchTotalDbg += 2 );
        WHEN_DBG( pMarkup->_cElementsTotalDbg += 1 );

        Assert( pMarkup->IsNodeValid() );

        pMarkup->SetLoaded(TRUE);
    }

    if (ppMarkup)
    {
        *ppMarkup = pMarkup;
        pMarkup->AddRef();
    }
    
Cleanup:
    if(pMarkup)
        pMarkup->Release();

    RRETURN(hr);
}


#if TREE_SYNC
/**/
/**///
/**/// this is a big HACK, to temporarily get markup-sync working for netdocs.  it
/**/// will totally change in the future.
/**///
/**/
/**/MtDefine(CMkpSyncLogger_aryLogSinks_pv, CMkpSyncLogger, "CMkpSyncLogger::_aryLogSinks::_pv")
/**/
/**/CMkpSyncLogger::CMkpSyncLogger()
/**/{
/**/    // todo: if this is not needed, then get rid of it
/**/}
/**/
/**/CMkpSyncLogger::~CMkpSyncLogger()
/**/{
/**/    int                 i;
/**/    IMarkupSyncLogSink *  pISink;
/**/
/**/    // release interface pointers for any registered sinks
/**/    for (i = 0; i < _aryLogSinks.Size(); i ++)
/**/    {
/**/        pISink = _aryLogSinks.Item(i);
/**/        ReleaseInterface(pISink);
/**/    }
/**/}
/**/
/**/HRESULT
/**/CMkpSyncLogger::SetAttributeHandler(CElement * pElement, BSTR strPropertyName, VARIANT * pvarPropertyValue, LONG lFlags)
/**/{
/**/    HRESULT             hr;
/**/    IHTMLElement *      pIElement = NULL;
/**/    int                 i;
/**/    IMarkupSyncLogSink *  pISink;
/**/    IHTMLDocument2 *    pIDoc = NULL;
/**/
/**/    //
/**/    // validate and set up arguments
/**/    //
/**/
/**/    hr = THR(pElement->QueryInterface(IID_IHTMLElement,(void**)&pIElement));
/**/    if (S_OK != hr)
/**/    {
/**/        goto Cleanup;
/**/    }
/**/
/**/    hr = THR(CONTAINING_RECORD(this, CDoc, _SyncLogger)->QueryInterface(IID_IHTMLDocument2,(void**)&pIDoc));
/**/    if (S_OK != hr)
/**/    {
/**/        goto Cleanup;
/**/    }
/**/
/**/    //
/**/    // dispatch to all listeners
/**/    //
/**/
/**/    for (i = 0; i < _aryLogSinks.Size(); i ++)
/**/    {
/**/        pISink = _aryLogSinks.Item(i);
/**/        IGNORE_HR(pISink->SetAttribute(pIDoc,pIElement,strPropertyName,pvarPropertyValue,lFlags));
/**/    }
/**/
/**/Cleanup:
/**/    ReleaseInterface(pIElement);
/**/    ReleaseInterface(pIDoc);
/**/    RRETURN(hr);
/**/}
/**/
/**/HRESULT
/**/CMkpSyncLogger::InsertElementHandler(IMarkupPointer * pIPointerStart, IMarkupPointer * pIPointerFinish, IHTMLElement * pIElementInsert)
/**/{
/**/    HRESULT             hr = S_OK;
/**/    int                 i;
/**/    IMarkupSyncLogSink *  pISink;
/**/    IHTMLDocument2 *    pIDoc = NULL;
/**/
/**/    //
/**/    // validate and set up arguments
/**/    //
/**/
/**/    hr = THR(CONTAINING_RECORD(this, CDoc, _SyncLogger)->QueryInterface(IID_IHTMLDocument2,(void**)&pIDoc));
/**/    if (S_OK != hr)
/**/    {
/**/        goto Cleanup;
/**/    }
/**/
/**/    //
/**/    // dispatch to all listeners
/**/    //
/**/
/**/    for (i = 0; i < _aryLogSinks.Size(); i ++)
/**/    {
/**/        pISink = _aryLogSinks.Item(i);
/**/        IGNORE_HR(pISink->InsertElement(pIDoc,pIPointerStart,pIPointerFinish,pIElementInsert));
/**/    }
/**/
/**/Cleanup:
/**/    ReleaseInterface(pIDoc);
/**/    RRETURN(hr);
/**/}
/**/
/**/HRESULT
/**/CMkpSyncLogger::RemoveElementHandler(CElement * pElementRemove)
/**/{
/**/    HRESULT             hr = S_OK;
/**/    IHTMLElement *      pIElement = NULL;
/**/    int                 i;
/**/    IMarkupSyncLogSink *  pISink;
/**/    IHTMLDocument2 *    pIDoc = NULL;
/**/
/**/    //
/**/    // validate and set up arguments
/**/    //
/**/
/**/    hr = THR(pElementRemove->QueryInterface(IID_IHTMLElement,(void**)&pIElement));
/**/    if (S_OK != hr)
/**/    {
/**/        goto Cleanup;
/**/    }
/**/
/**/    hr = THR(CONTAINING_RECORD(this, CDoc, _SyncLogger)->QueryInterface(IID_IHTMLDocument2,(void**)&pIDoc));
/**/    if (S_OK != hr)
/**/    {
/**/        goto Cleanup;
/**/    }
/**/
/**/    //
/**/    // dispatch to all listeners
/**/    //
/**/
/**/    for (i = 0; i < _aryLogSinks.Size(); i ++)
/**/    {
/**/        pISink = _aryLogSinks.Item(i);
/**/        IGNORE_HR(pISink->RemoveElement(pIDoc,pIElement));
/**/    }
/**/
/**/Cleanup:
/**/    ReleaseInterface(pIElement);
/**/    ReleaseInterface(pIDoc);
/**/    RRETURN(hr);
/**/}
/**/
/**/HRESULT
/**/CMkpSyncLogger::InsertTextHandler(OLECHAR * pchText, long cch, CMarkupPointer * pPointerTarget)
/**/{
/**/    HRESULT             hr = S_OK;
/**/    IMarkupPointer *    pIPointer = NULL;
/**/    int                 i;
/**/    IMarkupSyncLogSink *  pISink;
/**/    IHTMLDocument2 *    pIDoc = NULL;
/**/
/**/    //
/**/    // validate and set up arguments
/**/    //
/**/
/**/    hr = THR(pPointerTarget->QueryInterface(IID_IMarkupPointer,(void**)&pIPointer));
/**/    if (S_OK != hr)
/**/    {
/**/        goto Cleanup;
/**/    }
/**/
/**/    hr = THR(CONTAINING_RECORD(this, CDoc, _SyncLogger)->QueryInterface(IID_IHTMLDocument2,(void**)&pIDoc));
/**/    if (S_OK != hr)
/**/    {
/**/        goto Cleanup;
/**/    }
/**/
/**/    //
/**/    // dispatch to all listeners
/**/    //
/**/
/**/    for (i = 0; i < _aryLogSinks.Size(); i ++)
/**/    {
/**/        pISink = _aryLogSinks.Item(i);
/**/        IGNORE_HR(pISink->InsertText(pIDoc,pchText,cch,pIPointer));
/**/    }
/**/
/**/Cleanup:
/**/    ReleaseInterface(pIPointer);
/**/    ReleaseInterface(pIDoc);
/**/    RRETURN(hr);
/**/}
/**/
/**/HRESULT
/**/CMkpSyncLogger::CutCopyMoveHandler(CMarkupPointer * pPointerStart, CMarkupPointer * pPointerFinish, CMarkupPointer * pPointerTarget, BOOL fRemove)
/**/{
/**/    HRESULT             hr = S_OK;
/**/    IMarkupPointer *    pIPointerStart = NULL;
/**/    IMarkupPointer *    pIPointerFinish = NULL;
/**/    IMarkupPointer *    pIPointerTarget = NULL;
/**/    int                 i;
/**/    IMarkupSyncLogSink *  pISink;
/**/    IHTMLDocument2 *    pIDoc = NULL;
/**/
/**/    //
/**/    // validate and set up arguments
/**/    //
/**/
/**/    hr = THR(pPointerStart->QueryInterface(IID_IMarkupPointer,(void**)&pIPointerStart));
/**/    if (S_OK != hr)
/**/    {
/**/        goto Cleanup;
/**/    }
/**/
/**/    hr = THR(pPointerFinish->QueryInterface(IID_IMarkupPointer,(void**)&pIPointerFinish));
/**/    if (S_OK != hr)
/**/    {
/**/        goto Cleanup;
/**/    }
/**/
/**/    if (pPointerTarget != NULL)
/**/    {
/**/        hr = THR(pPointerTarget->QueryInterface(IID_IMarkupPointer,(void**)&pIPointerTarget));
/**/        if (S_OK != hr)
/**/        {
/**/            goto Cleanup;
/**/        }
/**/    }
/**/
/**/    hr = THR(CONTAINING_RECORD(this, CDoc, _SyncLogger)->QueryInterface(IID_IHTMLDocument2,(void**)&pIDoc));
/**/    if (S_OK != hr)
/**/    {
/**/        goto Cleanup;
/**/    }
/**/
/**/    //
/**/    // dispatch to all listeners
/**/    //
/**/
/**/    for (i = 0; i < _aryLogSinks.Size(); i ++)
/**/    {
/**/        pISink = _aryLogSinks.Item(i);
/**/        IGNORE_HR(pISink->CutCopyMove(pIDoc,pIPointerStart,pIPointerFinish,pIPointerTarget,fRemove));
/**/    }
/**/
/**/Cleanup:
/**/    ReleaseInterface(pIPointerStart);
/**/    ReleaseInterface(pIPointerFinish);
/**/    ReleaseInterface(pIPointerTarget);
/**/    ReleaseInterface(pIDoc);
/**/    RRETURN(hr);
/**/}
/**/
/**/HRESULT
/**/CMkpSyncLogger::CutCopyMoveFinalizeHandler()
/**/{
/**/    HRESULT             hr = S_OK;
/**/    int                 i;
/**/    IMarkupSyncLogSink *  pISink;
/**/    IHTMLDocument2 *    pIDoc = NULL;
/**/
/**/    //
/**/    // validate and set up arguments
/**/    //
/**/
/**/    hr = THR(CONTAINING_RECORD(this, CDoc, _SyncLogger)->QueryInterface(IID_IHTMLDocument2,(void**)&pIDoc));
/**/    if (S_OK != hr)
/**/    {
/**/        goto Cleanup;
/**/    }
/**/
/**/    //
/**/    // dispatch to all listeners
/**/    //
/**/
/**/    for (i = 0; i < _aryLogSinks.Size(); i ++)
/**/    {
/**/        pISink = _aryLogSinks.Item(i);
/**/        IGNORE_HR(pISink->CutCopyMoveFinalize(pIDoc));
/**/    }
/**/
/**/Cleanup:
/**/    ReleaseInterface(pIDoc);
/**/    RRETURN(hr);
/**/}
/**/
/**/HRESULT
/**/CMkpSyncLogger::StaticSetAttributeHandler(CBase *pbase, BSTR strPropertyName, VARIANT * pvarPropertyValue, LONG lFlags)
/**/{
/**/    HRESULT             hr = S_OK;
/**/    CElement *          pElement = NULL;
/**/
/**/    // this is a very bad way, and will be redone properly soon.
/**/
/**/    IGNORE_HR(pbase->PrivateQueryInterface(CLSID_CElement,(void**)&pElement));
/**/
/**/    // if it's an element, then sync it
/**/    if (pElement != NULL)
/**/    {
/**/        if (pElement->Doc()->_SyncLogger.IsLogging())
/**/        {
/**/            hr = THR(pElement->Doc()->_SyncLogger.SetAttributeHandler(pElement,strPropertyName,pvarPropertyValue,lFlags));
/**/        }
/**/    }
/**/
/**/    RRETURN(hr);
/**/}
/**/
/**/HRESULT
/**/CDoc::GetCpFromPointer ( IMarkupPointer * pIPointer, long * pcp )
/**/{
/**/    HRESULT          hr = S_OK;
/**/    CMarkupPointer * pPointer;
/**/    CTreePos *       ptp;
/**/
/**/    if (pIPointer == NULL)
/**/    {
/**/        hr = E_INVALIDARG;
/**/        goto Cleanup;
/**/    }
/**/
/**/    hr = THR( pIPointer->QueryInterface( CLSID_CMarkupPointer, (void **) & pPointer ) );
/**/    
/**/    if (hr)
/**/    {
/**/        hr = E_INVALIDARG;
/**/        goto Cleanup;
/**/    }
/**/
/**/    ptp = pPointer->TreePos();
/**/    
/**/    if (ptp == NULL)
/**/    {
/**/        hr = E_INVALIDARG;
/**/        goto Cleanup;
/**/    }
/**/
/**/    if (pcp != NULL)
/**/        *pcp = ptp->GetCp();
/**/
/**/Cleanup:
/**/    
/**/    RRETURN( hr );
/**/}
/**/
/**/HRESULT
/**/CDoc::MovePointerToCp ( IMarkupPointer * pIPointer, long cp, IMarkupContainer * pIContainer )
/**/{
/**/    HRESULT          hr;
/**/    CMarkupPointer * pPointer = NULL;
/**/    CMarkup *        pMarkup;
/**/
/**/    if ((pIPointer == NULL) || !IsOwnerOf(pIPointer))
/**/    {
/**/        hr = E_INVALIDARG;
/**/        goto Cleanup;
/**/    }
/**/
/**/    hr = THR( pIPointer->QueryInterface(CLSID_CMarkupPointer, (void **) & pPointer ) );
/**/    
/**/    if (hr)
/**/    {
/**/        hr = E_INVALIDARG;
/**/        goto Cleanup;
/**/    }
/**/
/**/    if (pIContainer == NULL)
/**/    {
/**/        pMarkup = PrimaryMarkup();
/**/    }
/**/    else
/**/    {
/**/        hr = THR( pIContainer->QueryInterface( CLSID_CMarkup, (void **) & pMarkup ) );
/**/        
/**/        if (hr)
/**/            goto Cleanup;
/**/    }
/**/
/**/    hr = THR( pPointer->MoveToCp( cp, pMarkup ) );
/**/
/**/Cleanup:
/**/    
/**/    RRETURN( hr );
/**/}
/**/
/**/// HACK: this should be rewritten to be standard OLE like IAdviseSink
/**/
/**/HRESULT
/**/CDoc::RegisterLogSink(IMarkupSyncLogSink * pILogSink)
/**/{
/**/    HRESULT             hr;
/**/
/**/    hr = THR(_SyncLogger._aryLogSinks.Append(pILogSink));
/**/    if (S_OK != hr)
/**/    {
/**/        goto Cleanup;
/**/    }
/**/
/**/    pILogSink->AddRef();
/**/
/**/Cleanup:
/**/    RRETURN(hr);
/**/}
/**/
/**/// HACK: this should be rewritten to be standard OLE like IAdviseSink
/**/HRESULT
/**/CDoc::UnregisterLogSink(IMarkupSyncLogSink * pILogSink)
/**/{
/**/    HRESULT             hr;
/**/    int                 i;
/**/    IUnknown *          punkRequested = NULL;
/**/    IUnknown *          punkTest = NULL;
/**/
/**/    hr = THR(pILogSink->QueryInterface(IID_IUnknown,(void**)&punkRequested));
/**/    if (S_OK != hr)
/**/    {
/**/        goto Cleanup;
/**/    }
/**/
/**/    hr = E_FAIL;
/**/    for (i = 0; i < _SyncLogger._aryLogSinks.Size(); i ++)
/**/    {
/**/        IMarkupSyncLogSink *  pISinkTest;
/**/
/**/        pISinkTest = _SyncLogger._aryLogSinks.Item(i);
/**/        ClearInterface(&punkTest);
/**/        if (SUCCEEDED(pISinkTest->QueryInterface(IID_IUnknown,(void**)&punkTest)))
/**/        {
/**/            if (punkTest == punkRequested)
/**/            {
/**/                _SyncLogger._aryLogSinks.Delete(i);
/**/                ReleaseInterface(pISinkTest);
/**/                hr = S_OK;
/**/                break;
/**/            }
/**/        }
/**/    }
/**/
/**/Cleanup:
/**/    
/**/    ReleaseInterface(punkRequested);
/**/    ReleaseInterface(punkTest);
/**/    RRETURN(hr);
/**/}
/**/
/**/#endif // TREE_SYNC


#if DBG==1

struct TagPos
{
    IHTMLElement * pel;
    BOOL           fEnd;
    TCHAR *        pText;
    long           cch;
};

void
Stuff ( CDoc * pDoc, IMarkupServices * pms, IHTMLElement * pIElement )
{
    HRESULT            hr = S_OK;
    IHTMLDocument2 *   pDoc2;
    IMarkupServices *  pMS;
    IMarkupContainer * pMarkup;
    IMarkupPointer *   pPtr1, * pPtr2;
    TCHAR          *   pstrFrom = _T( "XY" );
    TCHAR          *   pstrTo = _T( "AB" );
    
    pDoc->QueryInterface( IID_IHTMLDocument2, (void **) & pDoc2 );

    pDoc2->QueryInterface( IID_IMarkupContainer, (void **) & pMarkup );
    pDoc2->QueryInterface( IID_IMarkupServices, (void **) & pMS );

    pMS->CreateMarkupPointer( & pPtr1 );
    pMS->CreateMarkupPointer( & pPtr2 );

    //
    // Set gravity of this pointer so that when the replacement text is inserted
    // it will float to be after it.
    //

    pPtr1->SetGravity( POINTER_GRAVITY_Right );

    //
    // Start the seach at the beginning of the primary container
    //

    pPtr1->MoveToContainer( pMarkup, TRUE );

    for ( ; ; )
    {
        hr = pPtr1->FindText( pstrFrom, 0, pPtr2, NULL );

        if (hr == S_FALSE)
            break;

        pMS->Remove( pPtr1, pPtr2 );
        
        pMS->InsertText( pstrTo, -1, pPtr1 );
    }
}

void
TestMarkupServices(CElement *pElement)
{
    IMarkupServices * pIMarkupServices = NULL;
    IHTMLElement *pIElement = NULL;
    CDoc *pDoc = pElement->Doc();

    pElement->QueryInterface( IID_IHTMLElement, (void * *) & pIElement );
    pDoc->QueryInterface( IID_IMarkupServices, (void * *) & pIMarkupServices );

    Stuff( pDoc, pIMarkupServices, pIElement );

    ReleaseInterface( pIMarkupServices );
    ReleaseInterface( pIElement );
}

#endif // DBG==1

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

#if TREE_SYNC
#ifndef X_SYNCBUF_HXX_
#define X_SYNCBUF_HXX_
#include "syncbuf.hxx"
#endif
#endif // TREE_SYNC

#if TREE_SYNC
#ifndef X_ITREESYNC_H_
#define X_ITREESYNC_H_
#include "itreesync.h"
#endif
#endif // TREE_SYNC

#if TREE_SYNC
#ifndef X_EBODY_HXX_
#define X_EBODY_HXX_
#include "ebody.hxx"
#endif
#endif // TREE_SYNC

#if TREE_SYNC
#ifndef X__TXTSAVE_H_
#define X__TXTSAVE_H_
#include "_txtsave.h"
#endif
#endif // TREE_SYNC

#if TREE_SYNC
#ifndef X_QDOCGLUE_HXX_
#define X_QDOCGLUE_HXX_
#include "qdocglue.hxx"
#endif
#endif // TREE_SYNC

#if TREE_SYNC
static ELEMENT_TAG_ID
TagIdFromETag ( ELEMENT_TAG etag );

static ELEMENT_TAG
ETagFromTagId ( ELEMENT_TAG_ID tagID );
#endif // TREE_SYNC

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

#ifdef MERGEFUN // iRun
    long cpStart, cpFinish;
    // more sanity checks
    if (!hr)
        pRange->GetRange(cpStart, cpFinish);
    if (!hr)
        ptpl = &(pRange->GetPed()->GetList());
    if (hr)
        goto Cleanup;

    // move the pointers
    if (pPointerStart)
    {
        hr = pPointerStart->MoveToCp(cpStart, ptpl);
    }
    if (!hr && pPointerFinish)
    {
        hr = pPointerFinish->MoveToCp(cpFinish, ptpl, /*fAfter*/ TRUE);
    }

Cleanup:

#else //MERGEGFUN
    // move the pointers
    if (pPointerStart)
    {
        hr = pRange->GetLeft( pPointerStart );
    }
    if (!hr && pPointerFinish)
    {
        hr = pRange->GetRight( pPointerFinish );
    }
#endif //MERGEFUN
    
Cleanup:
    RRETURN(hr);
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

    hr = THR( pIPointerStart->IsPositioned( &fPositioned, NULL ) );
    if (hr)
        goto Cleanup;
    if (! fPositioned)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pIPointerFinish->IsPositioned( &fPositioned, NULL ) );
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
  
    hr = THR( pRange->SetLeft( pIPointerStart ) );
    if (! hr)   hr = THR( pRange->SetRight( pIPointerFinish ) );
    if (hr)
        goto Cleanup;
    
Cleanup:
    
    RRETURN( hr );
}


HRESULT
CDoc::InsertElement (
    CElement *       pElementInsert,
    CMarkupPointer * pPointerStart,
    CMarkupPointer * pPointerFinish )
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

    Assert( pPointerStart->Compare( pPointerFinish ) <= 0 );

    //
    // Make sure both pointers are positioned, and in the same tree
    //

    Assert( pPointerStart->IsPositioned() );
    Assert( pPointerFinish->IsPositioned() );
    Assert( pPointerStart->Markup() == pPointerFinish->Markup() );

    //
    // Position the gaps and do the insert
    //
    
    tpgStart.MoveTo( pPointerStart->TreePos(), TPG_LEFT );
    tpgFinish.MoveTo( pPointerFinish->TreePos(), TPG_LEFT );

    hr = THR(
        pPointerStart->Markup()->InsertElement(
            pElementInsert, & tpgStart, & tpgFinish ) );

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
#if TREE_SYNC
    long cpPointerStart, cpPointerFinish;
    CInsertElementLogRecord LogRecord;
    CElement * pElementSyncRoot = NULL;
    IStream * pStream = NULL;
    HANDLE hdata = NULL;
    ULARGE_INTEGER zero;
    void * pvStream;
    BSTR bstrUnicodeStream = NULL;
    bool fSaveLog = false;
    DWORD dw;
#endif // TREE_SYNC

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
    // Make sure the start if before the finish
    //

    if (pPointerStart->Compare( pPointerFinish ) > 0)
    {
        CMarkupPointer * pPointerTemp = pPointerStart;
        pPointerStart = pPointerFinish;
        pPointerFinish = pPointerTemp;
    }

    //
    // Make sure both pointers are positioned, and in the same tree
    //

    if (! pPointerStart->IsPositioned() ||
        ! pPointerFinish->IsPositioned() ||
          pPointerStart->Markup() != pPointerFinish->Markup() )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

#if TREE_SYNC
    if (_pqdocGlue->_SyncLogger.IsLogging(this))
    {
        hr = THR(_pqdocGlue->GetSyncInfoFromPointer(pIPointerStart, &pElementSyncRoot, &cpPointerStart));
        if (S_OK != hr)
        {
            goto SkipLogging;
        }

        hr = THR(_pqdocGlue->GetSyncInfoFromPointer(pIPointerFinish, &pElementSyncRoot, &cpPointerFinish));
        if (S_OK != hr)
        {
            goto SkipLogging;
        }

        fSaveLog = true;

        LogRecord.SetCpStart(cpPointerStart);
        LogRecord.SetCpFinish(cpPointerFinish);
        LogRecord.SetTagId(TagIdFromETag(pElementInsert->Tag()));

        zero.LowPart = 0;
        zero.HighPart = 0;
        hdata = GlobalAlloc(GHND,0);
        if (hdata == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(CreateStreamOnHGlobal(hdata,false/*fDeleteOnRelease*/,&pStream));
        if (S_OK != hr)
        {
            goto Cleanup;
        }

        hr = THR(pStream->SetSize(zero));
        if (S_OK != hr)
        {
            goto Cleanup;
        }

        // must create CStreamWriteBuff inline in scope due to constructor requiring stream arg
        {
            CStreamWriteBuff WriteBuf(pStream);
            hr = THR(pElementInsert->SaveAttributes(&WriteBuf));
            if (S_OK != hr)
            {
                goto Cleanup;
            }
        }

        ClearInterface(&pStream);

        pvStream = GlobalLock(hdata);

        // bug:  convert to unicode -- done wrong
        bstrUnicodeStream = SysAllocStringLen(NULL,GlobalSize(hdata));
        if (bstrUnicodeStream == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        for (dw = 0; dw < GlobalSize(hdata); dw ++)
        {
            bstrUnicodeStream[dw] = ((char*)pvStream)[dw];
        }

        hr = THR(LogRecord.SetAttrs(bstrUnicodeStream,GlobalSize(hdata)));
        if (S_OK != hr)
        {
            goto Cleanup;
        }

        hr = THR(LogRecord.Write(&(_pqdocGlue->_SyncLogger._bufOut)));
        if (S_OK != hr)
        {
            goto Cleanup;
        }

SkipLogging:
        ;
    }
#endif // TREE_SYNC
#if TREE_SYNC // hack: before doc modification
    if (fSaveLog)
    {
        hr = THR(_pqdocGlue->_SyncLogger.SyncLoggerSend(pElementSyncRoot));
        if (S_OK != hr)
        {
            goto Cleanup;
        }
    }
#endif // TREE_SYNC

    hr = THR( InsertElement( pElementInsert, pPointerStart, pPointerFinish ) );

    if (hr)
        goto Cleanup;

#if TREE_SYNC && 0//hack:disabled
    if (fSaveLog)
    {
        hr = THR(_pqdocGlue->_SyncLogger.CloseLog());
        if (S_OK != hr)
        {
            goto Cleanup;
        }

        hr = THR(_pqdocGlue->_SyncLogger.SyncLoggerSend(pElementSyncRoot));
        if (S_OK != hr)
        {
            goto Cleanup;
        }
    }
#endif // TREE_SYNC

Cleanup:

#if TREE_SYNC
    ReleaseInterface(pStream);
    if (hdata != NULL)
    {
        GlobalFree(hdata);
    }
    SysFreeString(bstrUnicodeStream);
#endif // TREE_SYNC

    RRETURN( hr );
}


HRESULT
CDoc::RemoveElement ( CElement * pElementRemove )
{
    HRESULT    hr = S_OK;
#if TREE_SYNC
    CDeleteElementLogRecord LogRecord;
    long           cpPointerStart;
    long           cpPointerFinish;
    CElement *     pElementSyncRoot = NULL;
    IStream *      pStream = NULL;
    HANDLE         hdata = NULL;
    ULARGE_INTEGER zero;
    BSTR           bstrUnicodeStream = NULL;
    void *         pvStream;
    IHTMLElement * pIElementRemove = NULL;
    bool           fSaveLog = false;
    DWORD          dw;
#endif // TREE_SYNC

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
    if (_pqdocGlue->_SyncLogger.IsLogging(this))
    {
        hr = THR(pElementRemove->QueryInterface(IID_IHTMLElement,(void**)&pIElementRemove));
        if (S_OK != hr)
        {
            goto Cleanup;
        }

        // get element cp's
        hr = THR(_pqdocGlue->GetSyncInfoFromElement(pIElementRemove, 
                                        &pElementSyncRoot, 
                                        &cpPointerStart,
                                        &cpPointerFinish));
        if (S_OK != hr)
        {
            goto SkipLogging;
        }

        fSaveLog = true;

        LogRecord.SetCpStart(cpPointerStart);
        LogRecord.SetCpFinish(cpPointerFinish);
        LogRecord.SetTagId(TagIdFromETag(pElementRemove->Tag()));

        zero.LowPart = 0;
        zero.HighPart = 0;
        hdata = GlobalAlloc(GHND,0);
        if (hdata == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(CreateStreamOnHGlobal(hdata,false/*fDeleteOnRelease*/,&pStream));
        if (S_OK != hr)
        {
            goto Cleanup;
        }

        hr = THR(pStream->SetSize(zero));
        if (S_OK != hr)
        {
            goto Cleanup;
        }

        // must create CStreamWriteBuff inline in scope due to constructor requiring stream arg
        {
            CStreamWriteBuff WriteBuf(pStream);
            hr = THR(pElementRemove->SaveAttributes(&WriteBuf));
            if (S_OK != hr)
            {
                goto Cleanup;
            }
        }

        ClearInterface(&pStream);

        pvStream = GlobalLock(hdata);

        // bug:  convert to unicode -- done wrong
        bstrUnicodeStream = SysAllocStringLen(NULL,GlobalSize(hdata));
        if (bstrUnicodeStream == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        for (dw = 0; dw < GlobalSize(hdata); dw ++)
        {
            bstrUnicodeStream[dw] = ((char*)pvStream)[dw];
        }

        hr = THR(LogRecord.SetAttrs(bstrUnicodeStream,GlobalSize(hdata)));
        if (S_OK != hr)
        {
            goto Cleanup;
        }

        hr = THR(LogRecord.Write(&(_pqdocGlue->_SyncLogger._bufOut)));
        if (S_OK != hr)
        {
            goto Cleanup;
        }

SkipLogging:
        ;
    }
#endif // TREE_SYNC
#if TREE_SYNC // hack: before doc modification
    if (fSaveLog)
    {
        hr = THR(_pqdocGlue->_SyncLogger.SyncLoggerSend(pElementSyncRoot));
        if (S_OK != hr)
        {
            goto Cleanup;
        }
    }
#endif // TREE_SYNC

    //
    // Now, remove the element
    //

    hr = THR( pElementRemove->GetMarkup()->RemoveElement( pElementRemove ) );

#if TREE_SYNC && 0 // hack disabled
    if (fSaveLog)
    {
        hr = THR(_pqdocGlue->_SyncLogger.CloseLog());
        if (S_OK != hr)
        {
            goto Cleanup;
        }

        hr = THR(_pqdocGlue->_SyncLogger.SyncLoggerSend(pElementSyncRoot));
        if (S_OK != hr)
        {
            goto Cleanup;
        }
    }
#endif // TREE_SYNC

    if (hr)
        goto Cleanup;

Cleanup:

#if TREE_SYNC
    ReleaseInterface(pStream);
    if (hdata != NULL)
    {
        GlobalFree(hdata);
    }
    SysFreeString(bstrUnicodeStream);
    ReleaseInterface(pIElementRemove);
#endif // TREE_SYNC

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
    
    if (!pIElementRemove || ! IsOwnerOf( pIElementRemove ))
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
        hr = E_INVALIDARG;
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
    CMarkupPointer * pPointerFinish )
{
    return CutCopyMove( pPointerStart, pPointerFinish, NULL, TRUE );
}

HRESULT
CDoc::Copy (
    CMarkupPointer * pPointerSourceStart,
    CMarkupPointer * pPointerSourceFinish,
    CMarkupPointer * pPointerTarget )
{
    return
        CutCopyMove(
            pPointerSourceStart, pPointerSourceFinish,
            pPointerTarget, FALSE );
}

HRESULT
CDoc::Move (
    CMarkupPointer * pPointerSourceStart,
    CMarkupPointer * pPointerSourceFinish,
    CMarkupPointer * pPointerTarget )
{
    return
        CutCopyMove(
            pPointerSourceStart, pPointerSourceFinish,
            pPointerTarget, TRUE );
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
    OLECHAR *        pchText,
    long             cch,
    CMarkupPointer * pPointerTarget )
{
    HRESULT    hr = S_OK;
    CTreePos * ptpPointer;
#if TREE_SYNC
    long cpPointer;
    CInsertTextLogRecord InsertTextThingie;
    CElement * pElementSyncRoot = NULL;
    IMarkupPointer * pIPointerTarget = NULL;
    bool fSaveLog = false;
#endif // TREE_SYNC

    Assert( pPointerTarget );
    Assert( pPointerTarget->IsPositioned() );

    ptpPointer = pPointerTarget->TreePos();

    if (cch < 0)
        cch = pchText ? _tcslen( pchText ) : 0;

#if TREE_SYNC
    if (_pqdocGlue->_SyncLogger.IsLogging(this))
    {
        hr = pPointerTarget->QueryInterface(IID_IMarkupPointer,(void**)&pIPointerTarget);
        if (S_OK != hr)
        {
            goto Cleanup;
        }

        hr = THR(_pqdocGlue->GetSyncInfoFromPointer(pIPointerTarget, &pElementSyncRoot, &cpPointer));
        if (S_OK != hr)
        {
            goto SkipLogging;
        }

        fSaveLog = true;

        InsertTextThingie.SetCp(cpPointer);
        hr = THR(InsertTextThingie.SetText(pchText,_tcslen(pchText)/*!!!slow!!!*/));
        if (S_OK != hr)
        {
            goto Cleanup;
        }

        hr = THR(InsertTextThingie.Write(&(_pqdocGlue->_SyncLogger._bufOut)));
        if (S_OK != hr)
        {
            goto Cleanup;
        }

SkipLogging:
        ;
    }
#endif // TREE_SYNC
#if TREE_SYNC // hack: before doc modification
    if (fSaveLog)
    {
        hr = THR(_pqdocGlue->_SyncLogger.SyncLoggerSend(pElementSyncRoot));
        if (S_OK != hr)
        {
            goto Cleanup;
        }
    }
#endif // TREE_SYNC

    hr = THR( ptpPointer->GetMarkup()->InsertText( ptpPointer, pchText, cch ) );

    if (hr)
        goto Cleanup;

#if TREE_SYNC && 0 // hack: disabled
    if (fSaveLog)
    {
        hr = THR(_pqdocGlue->_SyncLogger.CloseLog());
        if (S_OK != hr)
        {
            goto Cleanup;
        }

        hr = THR(_pqdocGlue->_SyncLogger.SyncLoggerSend(pElementSyncRoot));
        if (S_OK != hr)
        {
            goto Cleanup;
        }
    }
#endif // TREE_SYNC

Cleanup:

#if TREE_SYNC
    ReleaseInterface(pIPointerTarget);
#endif // TREE_SYNC

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
    
    if (!pPointerTarget || !pPointerTarget->IsPositioned())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Do it
    //

    hr = THR( InsertText( pchText, cch, pPointerTarget ) );

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
            hr = THR( ptp->GetMarkup()->RemovePointerPos( ptp ) );

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

    //
    // Make sure the finish is (totally) ordered properly with
    // respect to the begin
    //

    if (pPointerSelStart->IsPositioned() && pPointerSelFinish->IsPositioned() &&
        TotalOrderCompare( pPointerSelStart, pPointerSelFinish ) > 0)
    {
        CMarkupPointer * pMarkupTemp = pPointerSelStart;
        pPointerSelStart = pPointerSelFinish;
        pPointerSelFinish = pMarkupTemp;
    }

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
Equal ( IMarkupPointer * p1, IMarkupPointer * p2 )
{
    BOOL fEqual;
    IGNORE_HR( p1->Equal( p2, & fEqual ) );
    return fEqual;
}

static inline int
Compare ( IMarkupPointer * p1, IMarkupPointer * p2 )
{
    int result;
    IGNORE_HR( p1->Compare( p2, & result ) );
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
    Assert( Compare( pPointerStart, pPointerFinish ) <= 0 );

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

HRESULT
CDoc::ValidateElements (
    CMarkupPointer *   pPointerStart,
    CMarkupPointer *   pPointerFinish,
    CMarkupPointer *   pPointerTarget,
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

    //
    // If the status pointer is outside the range to validate, then
    // the validation is ended.  In this case, release the status pointer
    // and return.
    //

    if (Compare( pPointerStart, *ppPointerStatus ) > 0 ||
        Compare( pPointerFinish, *ppPointerStatus ) <= 0)
    {
        (*ppPointerStatus)->Release();
        *ppPointerStatus = NULL;
        
        goto Cleanup;
    }

    //
    // Set up stuff if there is a target specified
    //

    if (pPointerTarget)
    {
        //
        // If the target is unpositioned, then it is as good as not being
        // specified at all
        //

        if (!pPointerTarget->IsPositioned())
            pPointerTarget = NULL;
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
        CTreeNode * pNodeTarget = pNodeCommon;
        int         nNodesTarget = 0, i;

        if (pPointerTarget)
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

            if (Compare( pPointerStart,  pPointerTarget ) < 0 &&
                Compare( pPointerFinish, pPointerTarget ) > 0)
            {
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

        //
        // Now, put tags in the tag array starting from the target
        //

        for ( i = 1, pNode = pNodeTarget ;
              pNode && pNode->Tag() != ETAG_ROOT ;
              pNode = pNode->Parent(), i++ )
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

    for ( pNode = pPointerStart->Branch() ;
          pNode != pNodeCommon ;
          pNode = pNode->Parent() )
    {
        pNode->Element()->_fMark1 = 1;
    }

    for ( pNode = pPointerFinish->Branch() ;
          pNode != pNodeCommon ;
          pNode = pNode->Parent() )
    {
        pNode->Element()->_fMark1 = 0;
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

    ptpWalk = pPointerStart->TreePos();
    
    if (! Equal( *ppPointerStatus, pPointerStart ))
    {
        CTreePos * ptpStatus = (*ppPointerStatus)->TreePos();

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
              pNode = pNode->Parent(), i-- )
        {
            if (pNode->Element()->_fMark1)
                aryNodes[ i ] = pNode;
        }
    }

    //
    // This is the 'main' loop where validation actually takes place.
    //

    ptpFinish = pPointerFinish->TreePos();
    nNodesOk = 0;
    
    for ( ; ; )
    {
        BOOL fDone;
        long iConflictTop, iConflictBottom;
        
        //
        // Validate the current tag array
        //

        extern HRESULT ValidateNodeList(
            CTreeNode ** apNode, long cNode, long cNodeOk,
            long * piT, long * piB);

        hr = THR(
            ValidateNodeList(
                aryNodes, aryNodes.Size(), nNodesOk,
                & iConflictTop, & iConflictBottom ) );

        //
        // Returning S_FALSE indicates a conflict
        //

        if (hr == S_FALSE)
        {
            CTreePosGap gap ( ptpWalk, TPG_LEFT );
            
            hr = THR( (*ppPointerStatus)->MoveToGap( & gap ) );

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

                    IGNORE_HR( aryNodes.Append( ptpWalk->Branch() ) );
                }
                else
                {
                    int cIncl = 0;

                    // walk the first half of the inclusion
                    while( !ptpWalk->IsEdgeScope() )
                    {
                        Assert( ptpWalk != ptpFinish );

                        Assert( ptpWalk->IsEndNode() );

                        cIncl++;

                        Assert( aryNodes[ aryNodes.Size() - 1] == ptpWalk->Branch() );

                        // pop the bottom node off of the stack
                        aryNodes.SetSize( aryNodes.Size() - 1 );

                        ptpWalk = ptpWalk->NextTreePos();
                    }

                    // pop the node that is really ending
                    Assert( ptpWalk->IsEndNode() );

                    Assert( aryNodes[ aryNodes.Size() - 1] == ptpWalk->Branch() );

                    aryNodes.SetSize( aryNodes.Size() - 1 );

                    nNodesOk = aryNodes.Size();

                    // walk the second half of the inclusion
                    while( cIncl-- )
                    {
                        ptpWalk = ptpWalk->NextTreePos();

                        Assert( ptpWalk->IsBeginNode() && ! ptpWalk->IsEdgeScope() );
                        
                        // push the new nodes onto the stack
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
CDoc::ValidateElements (
    IMarkupPointer *   pIPointerStart,
    IMarkupPointer *   pIPointerFinish,
    IMarkupPointer *   pIPointerTarget,
    IMarkupPointer * * ppIPointerStatus,
    IHTMLElement * *   ppIElementFailBottom,
    IHTMLElement * *   ppIElementFailTop )
{
    HRESULT          hr = S_OK;
    CMarkupPointer * pPointerStart;
    CMarkupPointer * pPointerFinish;
    CMarkupPointer * pPointerTarget = NULL;
    CMarkupPointer * pPointerStatus = NULL;
    CTreeNode *      pNodeFailBottom = NULL;
    CTreeNode *      pNodeFailTop = NULL;

    //
    // make sure incomming args are OK
    //

    if (!pIPointerStart || !pIPointerFinish)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    //
    // Get the "real" start and finish
    //

    hr = THR(
        pIPointerStart->QueryInterface(
            CLSID_CMarkupPointer, (void **) & pPointerStart ) );

    if (hr)
        goto Cleanup;

    Assert( pPointerStart );

    hr = THR(
        pIPointerFinish->QueryInterface(
            CLSID_CMarkupPointer, (void **) & pPointerFinish ) );

    if (hr)
        goto Cleanup;

    Assert( pPointerFinish );

    //
    // Make sure the start and finish are Kosher
    //
    
    if (!pPointerStart->Markup() ||
        pPointerStart->Markup() != pPointerFinish->Markup())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (Compare( pPointerStart, pPointerFinish ) > 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Make sure a status pointer has been passed in
    //
    
    if (!ppIPointerStatus)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    //
    // Check the target
    //

    if (pIPointerTarget)
    {
        hr = THR(
            pIPointerTarget->QueryInterface(
                CLSID_CMarkupPointer, (void **) & pPointerTarget ) );

        if (hr)
            goto Cleanup;

        if (!pPointerTarget->IsPositioned())
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }

    //
    // Do it.
    //
    

    hr = THR(
        ValidateElements(
            pPointerStart, pPointerFinish, pPointerTarget, & pPointerStatus,
            ppIElementFailBottom ? & pNodeFailBottom : NULL,
            ppIElementFailTop    ? & pNodeFailTop    : NULL ) );

    if (hr && hr != S_FALSE)
        goto Cleanup;

    if (hr == S_FALSE)
    {
        if (pPointerStatus)
        {
            hr = THR(
                pPointerStatus->QueryInterface(
                    IID_IMarkupPointer, (void **) ppIPointerStatus ) );

            if (hr)
                goto Cleanup;
        }

        if (pNodeFailBottom)
        {
            hr = THR(
                pNodeFailBottom->GetElementInterface(
                    IID_IHTMLElement, (void **) ppIElementFailBottom ) );

            if (hr)
                goto Cleanup;
        }

        if (pNodeFailTop)
        {
            hr = THR(
                pNodeFailTop->GetElementInterface(
                    IID_IHTMLElement, (void **) ppIElementFailTop ) );

            if (hr)
                goto Cleanup;
        }
    }
    
Cleanup:

    RRETURN1( hr, S_FALSE );
}

HRESULT 
CDoc::BeginUndoUnit( OLECHAR * pchDescription )
{
    HRESULT hr = S_OK;
    
    if (! pchDescription)
        goto Cleanup;

    if (! _uOpenUnitsCounter)
    {
        _pMarkupServicesParentUndo = new CParentUndo( this );  
        if (!_pMarkupServicesParentUndo)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }


        hr = _pMarkupServicesParentUndo->Start( pchDescription ); 

    }        

    {
        CSelectionUndo Undo( _pElemCurrent);    
    }
    _uOpenUnitsCounter++;

Cleanup:
    RRETURN( hr );
}


HRESULT 
CDoc::EndUndoUnit(void)
{
    HRESULT  hr = S_OK;

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


//
// BUGBUG: These switchs are not the bast way to tdo this.  .asc perhaps ?
//

static ELEMENT_TAG_ID
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
    X(IMG) X(INPUT) X(INS) X(ISINDEX) X(KBD) X(LABEL) X(LEGEND) X(LI) X(LINK) X(LISTING)
    X(MAP) X(MARQUEE) X(MENU) X(META) X(NEXTID) X(NOBR) X(NOEMBED) X(NOFRAMES)
    X(NOSCRIPT) X(OBJECT) X(OL) X(OPTION) X(P) X(PARAM) X(PLAINTEXT) X(PRE) X(Q)
    X(HTMLAREA) X(RP) X(RT) X(RUBY) X(S) X(SAMP) X(SCRIPT) X(SELECT) X(SMALL) X(SPAN) 
    X(STRIKE) X(STRONG) X(STYLE) X(SUB) X(SUP) X(TABLE) X(TBODY) X(TD) X(TEXTAREA) 
    X(TFOOT) X(TH) X(THEAD) X(TR) X(TT) X(U) X(UL) X(VAR) X(WBR) X(XMP)
#undef X
    case ETAG_TITLE_ELEMENT : tagID = TAGID_TITLE; break;

    // BUGBUG (MohanB) For now, use INPUT's tag id for TXTSLAVE. Needs to be generified.
    case ETAG_TXTSLAVE : tagID = TAGID_INPUT; break;
    }

    AssertSz( tagID != TAGID_NULL, "Invalid ELEMENT_TAG" );

    return tagID;
}
    
static ELEMENT_TAG
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
    X(FRAMESET) X(H1) X(H2) X(H3) X(H4) X(H5) X(H6) X(HEAD) X(HR) X(HTML) X(I) X(IFRAME)
    X(IMG) X(INPUT) X(INS) X(ISINDEX) X(KBD) X(LABEL) X(LEGEND) X(LI) X(LINK) X(LISTING)
    X(MAP) X(MARQUEE) X(MENU) X(META) X(NEXTID) X(NOBR) X(NOEMBED) X(NOFRAMES)
    X(NOSCRIPT) X(OBJECT) X(OL) X(OPTION) X(P) X(PARAM) X(PLAINTEXT) X(PRE) X(Q)
    X(HTMLAREA) X(RP) X(RT) X(RUBY) X(S) X(SAMP) X(SCRIPT) X(SELECT) X(SMALL) X(SPAN) 
    X(STRIKE) X(STRONG) X(STYLE) X(SUB) X(SUP) X(TABLE) X(TBODY) X(TD) X(TEXTAREA) 
    X(TFOOT) X(TH) X(THEAD) X(TR) X(TT) X(U) X(UL) X(VAR) X(WBR) X(XMP)
#undef X
            
    case TAGID_TITLE : etag = ETAG_TITLE_ELEMENT; break;
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
    HRESULT hr = S_OK;
    ELEMENT_TAG etag;
    CElement * pElement = NULL;

    if (!ppIHTMLElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // BUGBUG: Find out out to use parser to parse attributes to create element with attrs
    //
    
    if (pchAttributes)
    {
        hr = E_NOTIMPL;
        goto Cleanup;
    }

    etag = ETagFromTagId( tagID );

    if (etag == ETAG_NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(::CreateElement(etag, & pElement, this, TRUE, NULL ) );

    // By default we want all text type elements created by tree services
    // to have an explicit end tag.
    if (!pElement->IsNoScope())
        pElement->_fExplicitEndTag = TRUE;

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
    HRESULT          hr;
    BOOL             result = FALSE;
    CElement        *pElement;

    hr = THR(pIElement->QueryInterface(CLSID_CElement, (void **)&pElement));
    if (hr)
        goto Cleanup;

    result = (this == pElement->Doc());

Cleanup:
    return result;
}


BOOL
CDoc::IsOwnerOf(IMarkupPointer *pIPointer)
{
    HRESULT hr;
    BOOL result;
    IHTMLDocument2 *pDispPointerDoc = NULL;
    IHTMLDocument2 *pIDoc = NULL;

    hr = pIPointer->OwningDoc(&pDispPointerDoc);
    
    if (!hr)
        hr = _pPrimaryMarkup->QueryInterface(IID_IHTMLDocument2, (void**)&pIDoc);

    result = hr ? FALSE : IsSameObject(pIDoc, pDispPointerDoc);

    ReleaseInterface(pDispPointerDoc);
    ReleaseInterface(pIDoc);
        
    return result;
}

BOOL
CDoc::IsOwnerOf(IHTMLTxtRange *pIRange)
{
    HRESULT hr;
    IHTMLElement *pIElement=NULL;
    BOOL result;

    hr = pIRange->parentElement(&pIElement);

    result = hr ? FALSE : IsOwnerOf(pIElement);

    ReleaseInterface(pIElement);
        
    return result;
}

BOOL
CDoc::IsOwnerOf(IMarkupContainer *pContainer)
{
    HRESULT hr;
    BOOL result;
    IHTMLDocument2 *pDispContainerDoc = NULL;
    IHTMLDocument2 *pIDoc = NULL;

    hr = pContainer->OwningDoc(&pDispContainerDoc);
    
    if (!hr)
        hr = _pPrimaryMarkup->QueryInterface(IID_IHTMLDocument2, (void**)&pIDoc);

    result = hr ? FALSE : IsSameObject(pIDoc, pDispContainerDoc);

    ReleaseInterface(pDispContainerDoc);
    ReleaseInterface(pIDoc);
        
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

    if (!pPointerStart->IsPositioned() ||
        !pPointerFinish->IsPositioned() ||
        pPointerStart->Markup() != pPointerFinish->Markup())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Make sure the start if before the finish
    //

    if (pPointerStart->Compare( pPointerFinish ) > 0)
    {
        CMarkupPointer * pPointerTemp = pPointerStart;
        pPointerStart = pPointerFinish;
        pPointerFinish = pPointerTemp;
    }

    //
    // More checks
    //

    if (pPointerTarget && !pPointerTarget->IsPositioned())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Do it
    //

    hr = THR(
        CutCopyMove(
            pPointerStart, pPointerFinish, pPointerTarget, fRemove ) );

Cleanup:

    RRETURN( hr );
}

HRESULT
CDoc::CutCopyMove (
    CMarkupPointer * pPointerStart,
    CMarkupPointer * pPointerFinish,
    CMarkupPointer * pPointerTarget,
    BOOL             fRemove )
{
    HRESULT     hr = S_OK;
    CTreePos *  ptpStart  = NULL;
    CTreePos *  ptpFinish = NULL;
    CTreePos *  ptpTarget = NULL;
    CTreePosGap tpgStart;
    CTreePosGap tpgFinish;
    CTreePosGap tpgTarget;
    CMarkup *   pMarkupSource = NULL;
    CMarkup *   pMarkupTarget = NULL;
#if TREE_SYNC
    CElement * pElementSyncRoot = NULL;
    CInsertTreeLogRecord LogRecordInsert;
    CCutCopyMoveTreeLogRecord LogRecordCCM;
    bool fDoInsertTree = false;
    bool fDoCutCopyMove = false;
    BOOL fSourceIsInMainTree = false;
    BOOL fTargetIsInMainTree = false;
    IMarkupContainer * pIContainerSource = NULL;
    BOOL fPositioned;
    long cpPointerStart, cpPointerFinish, cpPointerTarget;
    CMarkup * pContainer;
    IStream * pStream = NULL;
    HANDLE hdata = NULL;
    ULARGE_INTEGER zero;
    void * pvStream;
    BSTR bstrUnicodeStream = NULL;
    CMarkup * pPrimaryMarkup;
    IMarkupPointer * pIPointerStart = NULL;
    IMarkupPointer * pIPointerFinish = NULL;
    IMarkupPointer * pIPointerTarget = NULL;
    bool fSaveLog = true;
    IMarkupPointer * pIPtrDelGapStart = NULL;
    IMarkupPointer * pIPtrDelGapFinish = NULL;
    IMarkupPointer * pIPtrInsGapStart = NULL;
    IMarkupPointer * pIPtrInsGapFinish = NULL;
#endif // TREE_SYNC

    //
    // Sanity check the args
    //

    Assert( pPointerStart );
    Assert( pPointerFinish );
    Assert( pPointerStart->Compare( pPointerFinish ) <= 0 );
    Assert( pPointerStart->IsPositioned() );
    Assert( pPointerFinish->IsPositioned() );
    Assert( pPointerStart->Markup() == pPointerFinish->Markup() );

    //
    // Set up the gaps
    //
    
    tpgStart.MoveTo( pPointerStart->TreePos(), TPG_LEFT );
    tpgFinish.MoveTo( pPointerFinish->TreePos(), TPG_RIGHT );
    
    if (pPointerTarget)
        tpgTarget.MoveTo( pPointerTarget->TreePos(), TPG_LEFT );

    pMarkupSource = pPointerStart->Markup();

    if (pPointerTarget)
        pMarkupTarget = pPointerTarget->Markup();

#if TREE_SYNC
    // log it
    if (_pqdocGlue->_SyncLogger.IsLogging(this))
    {
        hr = pPointerStart->QueryInterface(IID_IMarkupPointer,(void**)&pIPointerStart);
        if (S_OK != hr)
        {
            goto Cleanup;
        }
        hr = pPointerFinish->QueryInterface(IID_IMarkupPointer,(void**)&pIPointerFinish);
        if (S_OK != hr)
        {
            goto Cleanup;
        }
        if (pPointerTarget != NULL)
        {
            hr = pPointerTarget->QueryInterface(IID_IMarkupPointer,(void**)&pIPointerTarget);
            if (S_OK != hr)
            {
                goto Cleanup;
            }
        }

        ptpStart = pPointerStart->TreePos();
        if (NULL == ptpStart)
        {
            hr = E_UNEXPECTED;
            goto Cleanup;
        }

        ptpFinish = pPointerFinish->TreePos();
        if (NULL == ptpFinish)
        {
            hr = E_UNEXPECTED;
            goto Cleanup;
        }

        if (pPointerTarget != NULL)
        {
            ptpTarget = pPointerTarget->TreePos();
            if (NULL == ptpTarget)
            {
                hr = E_UNEXPECTED;
                goto Cleanup;
            }
        }

        pPrimaryMarkup = PrimaryMarkup();
        if (pPrimaryMarkup == NULL)
        {
            hr = E_UNEXPECTED;
            goto Cleanup;
        }

        fSourceIsInMainTree = ptpStart->IsInMarkup(pPrimaryMarkup);
        fTargetIsInMainTree = (pIPointerTarget != NULL) && ptpTarget->IsInMarkup(pPrimaryMarkup);

        // if source is in aux, target in main, then do insert blob
        if (!fSourceIsInMainTree && fTargetIsInMainTree)
        {
            hr = THR(_pqdocGlue->GetSyncInfoFromPointer(pIPointerTarget, &pElementSyncRoot, &cpPointerTarget));
            if (S_OK != hr)
            {
                goto SkipLogging;
            }

            // first log the tree we're getting it from

            fDoInsertTree = true;

            // may be alien tree, can't use usual methods, so use GetCp() directly
            cpPointerStart = ptpStart->GetCp();
            cpPointerFinish = ptpFinish->GetCp();

            LogRecordInsert.SetCpStart(cpPointerStart);
            LogRecordInsert.SetCpFinish(cpPointerFinish);

            fSaveLog = true;

            LogRecordInsert.SetCpTarget(cpPointerTarget);

            hr = pIPointerStart->IsPositioned(&fPositioned,&pIContainerSource);
            if (S_OK != hr)
            {
                goto Cleanup;
            }

            hr = pIContainerSource->QueryInterface(CLSID_CMarkup,(void**)&pContainer);
            if (S_OK != hr)
            {
                goto Cleanup;
            }

            zero.LowPart = 0;
            zero.HighPart = 0;
            hdata = GlobalAlloc(GHND,0);
            if (hdata == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            hr = THR(CreateStreamOnHGlobal(hdata,false/*fDeleteOnRelease*/,&pStream));
            if (S_OK != hr)
            {
                goto Cleanup;
            }

            hr = THR(pStream->SetSize(zero));
            if (S_OK != hr)
            {
                goto Cleanup;
            }

            // must create CStreamWriteBuff inline in scope due to constructor requiring stream arg
            {
                CStreamWriteBuff WriteBuf(pStream);
                CTreeSaver saver(pContainer->Root(),&WriteBuf);
                hr = THR(saver.Save());
                if (S_OK != hr)
                {
                    goto Cleanup;
                }
            }

            ClearInterface(&pStream);

            pvStream = GlobalLock(hdata);

            // bug:  convert to unicode -- done wrong
            bstrUnicodeStream = SysAllocStringLen(NULL,GlobalSize(hdata));
            if (bstrUnicodeStream == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            for (DWORD i = 0; i < GlobalSize(hdata); i ++)
            {
                bstrUnicodeStream[i] = ((char*)pvStream)[i];
            }

            hr = THR(LogRecordInsert.SetHTML(bstrUnicodeStream,GlobalSize(hdata)));
            if (S_OK != hr)
            {
                goto Cleanup;
            }

            hr = THR(LogRecordInsert.Write(&(_pqdocGlue->_SyncLogger._bufOut)));
            if (S_OK != hr)
            {
                goto Cleanup;
            }
        }
        // else just pickle the arguments, as long as something is in the main tree
        else if (fSourceIsInMainTree)
        {
            hr = THR(_pqdocGlue->GetSyncInfoFromPointer(pIPointerStart, &pElementSyncRoot, &cpPointerStart));
            if (S_OK != hr)
            {
                goto SkipLogging;
            }

            hr = THR(_pqdocGlue->GetSyncInfoFromPointer(pIPointerFinish, &pElementSyncRoot, &cpPointerFinish));
            if (S_OK != hr)
            {
                goto SkipLogging;
            }

            fDoCutCopyMove = true;

            fSaveLog = true;

            // target might not be in tree
            if (fTargetIsInMainTree)
            {
                hr = THR(_pqdocGlue->GetSyncInfoFromPointer(pIPointerTarget, &pElementSyncRoot, &cpPointerTarget));
                if (S_OK != hr)
                {
                    goto Cleanup;
                }
            }
            else
            {
                cpPointerTarget = -1;
            }

            LogRecordCCM.SetCpStart(cpPointerStart);
            LogRecordCCM.SetCpFinish(cpPointerFinish);
            LogRecordCCM.SetCpTarget(cpPointerTarget);
            LogRecordCCM.SetFRemove(fRemove);


            CTreeNode * pnLeft = ptpStart->GetBranch();
            CTreeNode * pnRight = ptpFinish->GetBranch();
            CTreeNode * pnTop = pnLeft->GetFirstCommonAncestor(pnRight,NULL);
            DWORD cpTopStart = pnTop->GetBeginPos()->GetCp();

            zero.LowPart = 0;
            zero.HighPart = 0;
            hdata = GlobalAlloc(GHND,0);
            if (hdata == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            hr = THR(CreateStreamOnHGlobal(hdata,false/*fDeleteOnRelease*/,&pStream));
            if (S_OK != hr)
            {
                goto Cleanup;
            }

            hr = THR(pStream->SetSize(zero));
            if (S_OK != hr)
            {
                goto Cleanup;
            }

            // must create CStreamWriteBuff inline in scope due to constructor requiring stream arg
            {
                CStreamWriteBuff WriteBuf(pStream);
                CTreeSaver saver(pnTop->Element(),&WriteBuf);
                hr = THR(saver.Save());
                if (S_OK != hr)
                {
                    goto Cleanup;
                }
            }

            ClearInterface(&pStream);

            pvStream = GlobalLock(hdata);

            // bug:  convert to unicode -- done wrong
            bstrUnicodeStream = SysAllocStringLen(NULL,GlobalSize(hdata));
            if (bstrUnicodeStream == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            for (DWORD i = 0; i < GlobalSize(hdata); i ++)
            {
                bstrUnicodeStream[i] = ((char*)pvStream)[i];
            }

            LogRecordCCM.SetOldHTML(bstrUnicodeStream,GlobalSize(hdata));
            LogRecordCCM.SetCpOldHTMLStart(ptpStart->GetCp() - cpTopStart);
            LogRecordCCM.SetCpOldHTMLFinish(ptpFinish->GetCp() - cpTopStart);

            hr = THR(CreateMarkupPointer(&pIPtrDelGapStart));
            if (S_OK != hr)
            {
                goto Cleanup;
            }
            hr = THR(pIPtrDelGapStart->MoveToPointer(pIPointerStart));
            if (S_OK != hr)
            {
                goto Cleanup;
            }
            hr = THR(pIPtrDelGapStart->SetGravity(POINTER_GRAVITY_Left));
            if (S_OK != hr)
            {
                goto Cleanup;
            }

            hr = THR(CreateMarkupPointer(&pIPtrDelGapFinish));
            if (S_OK != hr)
            {
                goto Cleanup;
            }
            hr = THR(pIPtrDelGapFinish->MoveToPointer(pIPointerFinish));
            if (S_OK != hr)
            {
                goto Cleanup;
            }
            hr = THR(pIPtrDelGapFinish->SetGravity(POINTER_GRAVITY_Right));
            if (S_OK != hr)
            {
                goto Cleanup;
            }

            if (fTargetIsInMainTree)
            {
                hr = THR(CreateMarkupPointer(&pIPtrInsGapStart));
                if (S_OK != hr)
                {
                    goto Cleanup;
                }
                hr = THR(pIPtrInsGapStart->MoveToPointer(pIPointerTarget));
                if (S_OK != hr)
                {
                    goto Cleanup;
                }
                hr = THR(pIPtrInsGapStart->SetGravity(POINTER_GRAVITY_Left));
                if (S_OK != hr)
                {
                    goto Cleanup;
                }

                hr = THR(CreateMarkupPointer(&pIPtrInsGapFinish));
                if (S_OK != hr)
                {
                    goto Cleanup;
                }
                hr = THR(pIPtrInsGapFinish->MoveToPointer(pIPointerTarget));
                if (S_OK != hr)
                {
                    goto Cleanup;
                }
                hr = THR(pIPtrInsGapFinish->SetGravity(POINTER_GRAVITY_Right));
                if (S_OK != hr)
                {
                    goto Cleanup;
                }
            }
        }
#if 1 // all this stuff is a hack
        {
            CElement * pESR = pElementSyncRoot;
            while (pESR)
            {
                IHTMLElement * pI = NULL;
                ITreeSyncBehavior *pTreeSyncBehavior = NULL;

                hr = pESR->QueryInterface(IID_IHTMLElement,(void**)&pI);
                Assert(S_OK == hr);
                // send to all registered clients

                for (DWORD i = 0; i < (DWORD)_pqdocGlue->_SyncLogger._aryLogSinks.Size(); i ++)
                {
                    CTreeSyncLogger::SINKREC    rec = _pqdocGlue->_SyncLogger._aryLogSinks.Item(i);

                    (void)rec._pSink->ReceiveStreamData(pI,NULL,0,0);
                }

                //  Now, walk up to the next tree sync root (usually just the one!)
                for (CTreeNode *pNode = pESR->GetFirstBranch()->Parent(); pNode; pNode = pNode->Parent())
                {
                    pESR = pNode->Element();
                    ClearInterface(&pI);
                    hr = THR(pESR->QueryInterface(IID_IHTMLElement,(void**)&pI));
                    Assert(S_OK == hr);
                    ClearInterface(&pTreeSyncBehavior);
                    if (pESR->Tag() == ETAG_ROOT ||
                        S_OK == pESR->Doc()->_pqdocGlue->GetBindBehavior(pI,&pTreeSyncBehavior))
                    {
                        break;
                    }
                }
                ReleaseInterface(pI);
                if (NULL == pTreeSyncBehavior)
                {
                    break;
                }
                ReleaseInterface(pTreeSyncBehavior);
            }
        }
#endif

SkipLogging:
        ;
    }
#endif // TREE_SYNC

    //
    // Do it.
    //
    
    if (pPointerTarget)
    {
        hr = THR(
            pMarkupSource->SpliceTree(
                & tpgStart, & tpgFinish, pPointerTarget->Markup(),
                & tpgTarget, fRemove ) );

        if (hr)
            goto Cleanup;
    }
    else
    {
        Assert( fRemove );
        
        hr = THR(
            pMarkupSource->SpliceTree(
                & tpgStart, & tpgFinish, NULL, NULL, fRemove ) );

        if (hr)
            goto Cleanup;
    }

#if TREE_SYNC
    if (fSaveLog)
    {
        // Bug:  this is just yucky
        if (fDoCutCopyMove)
        {
            long cpSrcAfterDelStart;
            long cpSrcAfterDelFinish;
            long cpTgtAfterOpStart = -1;
            long cpTgtAfterOpFinish = -1;

            hr = THR(_pqdocGlue->GetSyncInfoFromPointer(pIPtrDelGapStart, &pElementSyncRoot, &cpSrcAfterDelStart));
            if (S_OK != hr)
            {
                goto Cleanup;
            }
            hr = THR(_pqdocGlue->GetSyncInfoFromPointer(pIPtrDelGapFinish, &pElementSyncRoot, &cpSrcAfterDelFinish));
            if (S_OK != hr)
            {
                goto Cleanup;
            }

            if (fTargetIsInMainTree)
            {
                hr = THR(_pqdocGlue->GetSyncInfoFromPointer(pIPtrInsGapStart, &pElementSyncRoot, &cpTgtAfterOpStart));
                if (S_OK != hr)
                {
                    goto Cleanup;
                }
                hr = THR(_pqdocGlue->GetSyncInfoFromPointer(pIPtrInsGapFinish, &pElementSyncRoot, &cpTgtAfterOpFinish));
                if (S_OK != hr)
                {
                    goto Cleanup;
                }
            }

            LogRecordCCM.SetCpPostOpSrcStart(cpSrcAfterDelStart);
            LogRecordCCM.SetCpPostOpSrcFinish(cpSrcAfterDelFinish);
            LogRecordCCM.SetCpPostOpTgtStart(cpTgtAfterOpStart);
            LogRecordCCM.SetCpPostOpTgtFinish(cpTgtAfterOpFinish);

            hr = THR(LogRecordCCM.Write(&(_pqdocGlue->_SyncLogger._bufOut)));
            if (S_OK != hr)
            {
                goto Cleanup;
            }

            hr = THR(_pqdocGlue->_SyncLogger.SyncLoggerSend(pElementSyncRoot));
            if (S_OK != hr)
            {
                goto Cleanup;
            }
        }
        else if (fDoInsertTree)
        {
            hr = THR(_pqdocGlue->_SyncLogger.SyncLoggerSend(pElementSyncRoot));
            if (S_OK != hr)
            {
                goto Cleanup;
            }
        }
    }
#endif // TREE_SYNC

Cleanup:

#if TREE_SYNC
    ReleaseInterface(pIContainerSource);
    ReleaseInterface(pStream);
    if (hdata != NULL)
    {
        GlobalFree(hdata);
    }
    SysFreeString(bstrUnicodeStream);
    ReleaseInterface(pIPointerStart);
    ReleaseInterface(pIPointerFinish);
    ReleaseInterface(pIPointerTarget);
    ReleaseInterface(pIPtrDelGapStart);
    ReleaseInterface(pIPtrDelGapFinish);
    ReleaseInterface(pIPtrInsGapStart);
    ReleaseInterface(pIPtrInsGapFinish);
#endif // TREE_SYNC

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
    if( !pRootElement )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pMarkup = new CMarkup( this, pElementMaster );
    if( !pMarkup )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR( pRootElement->Init() );
    if (hr)
        goto Cleanup;

    hr = THR( pRootElement->Init2() );
    if (hr)
        goto Cleanup;

    hr = THR( pMarkup->Init( pRootElement ) );
    if (hr)
        goto Cleanup;

    *ppMarkup = pMarkup;
    pMarkup = NULL;

Cleanup:
    if( pMarkup )
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
    HRESULT     hr = S_OK;
    CMarkup     * pMarkup = NULL;

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

        Assert( pNodeNew->_tpEnd.IsUninit() );
        ptpNew = pNodeNew->InitEndPos( TRUE );
        hr = THR( pMarkup->Insert( ptpNew, ptpRootBegin, FALSE ) );
        if(hr)
        {
            // The node never made it into the tree
            // so delete it
            delete pNodeNew;

            goto Cleanup;
        }

        Assert( pNodeNew->_tpBegin.IsUninit() );
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


#if DBG==1
#define TEST(x) hr = x; if (hr) goto Cleanup

void
NewStuff ( IMarkupServices * pIMarkupServices, IHTMLElement * pIElement )
{
    IMarkupPointer * pPtr1 = NULL;
    MARKUP_CONTEXT_TYPE ct;
    IHTMLElement * pElem;
    CElement * pElement;
    TCHAR chBuff [ 100 ];
    int i;
            
    pIMarkupServices->CreateMarkupPointer( & pPtr1 );
    
    pPtr1->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterBegin );
    
    for ( ; ; )
    {
        long cch = 1;
        
        pElem = NULL;

        for ( i = 0 ; i < 100 ; i++ ) chBuff[i] = 0;
        
        pPtr1->Right( TRUE, & ct, & pElem, & cch, chBuff );

        if (pElem)
            pElem->QueryInterface( CLSID_CElement, (void * *) & pElement );

        ClearInterface( & pElem );

        if (ct == CONTEXT_TYPE_None)
            break;
    }

    for ( ; ; )
    {
        long cch = 1;
        
        pElem = NULL;
        
        for ( i = 0 ; i < 100 ; i++ ) chBuff[i] = 0;
        
        pPtr1->Left( TRUE, & ct, & pElem, & cch, chBuff );

        if (pElem)
            pElem->QueryInterface( CLSID_CElement, (void * *) & pElement );

        ClearInterface( & pElem );

        if (ct == CONTEXT_TYPE_None)
            break;
    }

//Cleanup:
    
    ReleaseInterface( pPtr1 );
}

void
NewerStuff ( CDoc * pDoc, IMarkupServices * pIMarkupServices, IHTMLElement * pIElement )
{
    long cch = 1;
    IMarkupPointer * pPtr1 = NULL;
    IMarkupPointer * pPtr2 = NULL;
    IMarkupPointer * pPtr3 = NULL;
    IHTMLElement * pElem = NULL;
    IHTMLElement * pElemB = NULL;
    IHTMLElement * pElemI = NULL;
    IHTMLElement * pElemU = NULL;
    IHTMLElement * pElemEM = NULL;
    CStr cstrTemp;
            
    pIMarkupServices->CreateMarkupPointer( & pPtr1 );
    pIMarkupServices->CreateMarkupPointer( & pPtr2 );
    pIMarkupServices->CreateMarkupPointer( & pPtr3 );
    

    // Test SpliceTree
    
    pIMarkupServices->CreateElement( TAGID_B, NULL, & pElemB );
    pIMarkupServices->CreateElement( TAGID_I, NULL, & pElemI );
    pIMarkupServices->CreateElement( TAGID_U, NULL, & pElemU );
    pIMarkupServices->CreateElement( TAGID_EM, NULL, & pElemEM );
    
    pPtr1->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterBegin );

    cstrTemp.Set(_T("123456789"));
    pDoc->InsertText(cstrTemp, -1, pPtr1);

    // 1<I>2<B>3<U>4<EM>5</EM>6</I>7</U>8</B>9
    
    pPtr1->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterBegin );
    pPtr1->Right(TRUE, NULL, NULL, (cch = 2, & cch), NULL);
    pPtr2->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeEnd  );
    pPtr2->Left(TRUE, NULL, NULL, (cch = 1, & cch), NULL);
    pIMarkupServices->InsertElement(pElemB, pPtr1, pPtr2);

    pPtr1->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterBegin );
    pPtr1->Right(TRUE, NULL, NULL, (cch = 1, & cch), NULL);
    pPtr2->MoveAdjacentToElement( pElemB, ELEM_ADJ_BeforeEnd  );
    pPtr2->Left(TRUE, NULL, NULL, (cch = 2, & cch), NULL);
    pIMarkupServices->InsertElement(pElemI, pPtr1, pPtr2);
    
    pPtr1->MoveAdjacentToElement( pElemB, ELEM_ADJ_AfterBegin );
    pPtr1->Right(TRUE, NULL, NULL, (cch = 1, & cch), NULL);
    pPtr2->MoveAdjacentToElement( pElemB, ELEM_ADJ_BeforeEnd  );
    pPtr2->Left(TRUE, NULL, NULL, (cch = 1, & cch), NULL);
    pIMarkupServices->InsertElement(pElemU, pPtr1, pPtr2);
    
    pPtr1->MoveAdjacentToElement( pElemU, ELEM_ADJ_AfterBegin );
    pPtr1->Right(TRUE, NULL, NULL, (cch = 1, & cch), NULL);
    pPtr2->MoveAdjacentToElement( pElemI, ELEM_ADJ_BeforeEnd  );
    pPtr2->Left(TRUE, NULL, NULL, (cch = 1, & cch), NULL);
    pIMarkupServices->InsertElement(pElemEM, pPtr1, pPtr2);
    
    pPtr1->MoveAdjacentToElement( pElemB, ELEM_ADJ_AfterBegin );
    pPtr2->MoveAdjacentToElement( pElemU, ELEM_ADJ_BeforeEnd  );
    pPtr3->MoveAdjacentToElement( pElemEM, ELEM_ADJ_BeforeEnd  );
    pDoc->Copy(pPtr1, pPtr2, pPtr3);
    
    pPtr1->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeBegin );
    pPtr2->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeEnd  );
    pPtr1->Left( TRUE, NULL, NULL, NULL, NULL  );
    pPtr1->Left( TRUE, NULL, NULL, NULL, NULL  );
    pPtr1->Left( TRUE, NULL, NULL, NULL, NULL  );
    pPtr1->Left( TRUE, NULL, NULL, NULL, NULL  );
    pPtr1->Left( TRUE, NULL, NULL, NULL, NULL  );
    pPtr2->Left( TRUE, NULL, NULL, (cch = 1, & cch), NULL  );
    pIMarkupServices->InsertElement( pElem, pPtr1, pPtr2 );
    pPtr1->Unposition();
    pPtr2->Unposition();
    pIMarkupServices->RemoveElement( pElem );
    
    pPtr1->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeBegin );
    pPtr2->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterEnd  );
    pIMarkupServices->InsertElement( pElem, pPtr1, pPtr2 );
    pPtr1->Unposition();
    pPtr2->Unposition();
    pIMarkupServices->RemoveElement( pElem );
    
    pPtr1->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterBegin );
    pPtr2->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterEnd  );
    pIMarkupServices->InsertElement( pElem, pPtr1, pPtr2 );
    pPtr1->Unposition();
    pPtr2->Unposition();
    pIMarkupServices->RemoveElement( pElem );
    
    pPtr1->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeBegin );
    pPtr2->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeEnd  );
    pIMarkupServices->InsertElement( pElem, pPtr1, pPtr2 );
    pPtr1->Unposition();
    pPtr2->Unposition();
    pIMarkupServices->RemoveElement( pElem );
    
    pPtr1->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterBegin );
    pPtr2->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeEnd  );
    pIMarkupServices->InsertElement( pElem, pPtr1, pPtr2 );
    pPtr1->Unposition();
    pPtr2->Unposition();
    pIMarkupServices->RemoveElement( pElem );
    
    pPtr1->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeBegin );
    pPtr2->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeEnd  );
    pPtr2->Left( TRUE, NULL, NULL, (cch = 1, & cch), NULL  );
    pIMarkupServices->InsertElement( pElem, pPtr1, pPtr2 );
    pPtr1->Unposition();
    pPtr2->Unposition();
    pIMarkupServices->RemoveElement( pElem );
    
    pPtr1->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterBegin );
    pPtr2->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeEnd  );
    pPtr2->Left( TRUE, NULL, NULL, (cch = 1, & cch), NULL  );
    pIMarkupServices->InsertElement( pElem, pPtr1, pPtr2 );
    pPtr1->Unposition();
    pPtr2->Unposition();
    pIMarkupServices->RemoveElement( pElem );
    
    pPtr1->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterBegin );
    pPtr1->Right( TRUE, NULL, NULL, (cch = 1, & cch), NULL  );
    pPtr2->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeEnd  );
    pIMarkupServices->InsertElement( pElem, pPtr1, pPtr2 );

    pPtr1->Unposition();
    pPtr2->Unposition();
    pIMarkupServices->RemoveElement( pElem );
    
    pPtr1->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterBegin );
    pPtr1->Right( TRUE, NULL, NULL, (cch = 1, & cch), NULL  );
    pPtr2->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterEnd  );
    pIMarkupServices->InsertElement( pElem, pPtr1, pPtr2 );
    pPtr1->Unposition();
    pPtr2->Unposition();
    pIMarkupServices->RemoveElement( pElem );

    ReleaseInterface( pPtr1 );
    ReleaseInterface( pPtr2 );
    ReleaseInterface( pPtr3 );
    ReleaseInterface( pElem );
    ReleaseInterface( pElemB );
    ReleaseInterface( pElemU );
    ReleaseInterface( pElemI );
    ReleaseInterface( pElemEM );
}

#define ERRTEST(hr) if ((hr)) goto Error;

void
TextIDTest( CDoc * pDoc, IMarkupServices * pIMarkupServices, IHTMLElement * pIElement )
{
    IMarkupPointer *    pPtr1 = NULL;
    IMarkupPointer *    pPtr2 = NULL;
    IHTMLElement *      pElem = NULL;

    MARKUP_CONTEXT_TYPE mc;
            
    ERRTEST( pIMarkupServices->CreateMarkupPointer( & pPtr1 ) );
    ERRTEST( pIMarkupServices->CreateMarkupPointer( & pPtr2 ) );

    ERRTEST( pPtr1->MoveAdjacentToElement( pIElement, ELEM_ADJ_AfterBegin ) );
    ERRTEST( pPtr2->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeEnd  ) );

    ERRTEST( pPtr1->SetTextIdentity( pPtr2) );

    do 
    {
        pPtr1->Right( TRUE, &mc, NULL, NULL, NULL );

        if( mc == CONTEXT_TYPE_Text )
        {
            long lTextID, lTextID2;

            pPtr1->Left( TRUE, &mc, NULL, NULL, NULL );

            ERRTEST( pPtr1->GetTextIdentity( &lTextID ) );

            Assert( lTextID > 0 );

            if( lTextID % 3 )
            {
                pPtr1->Left( TRUE, NULL, NULL, NULL, NULL );
            }
            else if( (lTextID + 1) % 3 )
            {
                pPtr1->Right( TRUE, NULL, NULL, NULL, NULL );
            }

            ERRTEST( pPtr1->FindTextIdentity( lTextID, pPtr2 ) );

            ERRTEST( pPtr1->GetTextIdentity( &lTextID2 ) );

            Assert( lTextID == lTextID2 );

            ERRTEST( pPtr1->MoveToPointer( pPtr2 ) );
        }
    }
    while( mc != CONTEXT_TYPE_None );

    pDoc->_pElemCurrent->GetMarkup()->DumpTree();

Cleanup:
    ReleaseInterface( pPtr1 );
    ReleaseInterface( pPtr2 );

    return;

Error:

    AssertSz( 0, "Tree Services test failed" );
    goto Cleanup;
}

void
TestMarkupServices(CElement *pElement)
{
    IMarkupServices * pIMarkupServices = NULL;
    IHTMLElement *pIElement = NULL;
    CDoc *pDoc = pElement->Doc();
    HRESULT hr;

    TEST(pElement->QueryInterface( IID_IHTMLElement, (void * *) & pIElement ) );
    TEST(pDoc->QueryInterface( IID_IMarkupServices, (void * *) & pIMarkupServices ) );

//    NewStuff( pIMarkupServices, pIElement );
//    NavigationTest( pDoc, pIMarkupServices, pIElement, 0xFF );
//    NewerStuff( pDoc, pIMarkupServices, pIElement );
//    MultiTreeTest( pIMarkupServices );
//    TextIDTest( pDoc, pIMarkupServices, pIElement );
//    ParseStuff( pIMarkupServices );
//    ValidateStuff ( pDoc, pIMarkupServices, pIElement );

Cleanup:
    
    ReleaseInterface( pIMarkupServices );
    ReleaseInterface( pIElement );
}
#endif // DBG==1

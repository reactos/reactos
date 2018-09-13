//+---------------------------------------------------------------------
//
//   File:      markupundo.cxx
//
//  Contents:   Undo of markup changes
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_MARKUPUNDO_HXX_
#define X_MARKUPUNDO_HXX_
#include "markupundo.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

MtDefine(CInsertElementUndoUnit, Undo, "CInsertElementUndoUnit")
MtDefine(CRemoveElementUndoUnit, Undo, "CRemoveElementUndoUnit")
MtDefine(CInsertTextUndoUnit, Undo, "CInsertTextUndoUnit")
MtDefine(CRemoveTextUndoUnit, Undo, "CRemoveTextUndoUnit")
MtDefine(CRemoveTextUndoUnit_pchText, CRemoveTextUndoUnit, "CRemoveTextUndoUnit::pchText")
MtDefine(CInsertSpliceUndoUnit, Undo, "CInsertSpliceUndoUnit")
MtDefine(CRemoveSpliceUndoUnit, Undo, "CRemoveSpliceUndoUnit")
MtDefine(CRemoveSpliceUndoUnit_pchRemoved, CRemoveSpliceUndoUnit, "CRemoveSpliceUndoUnit::_pchRemoved")
MtDefine(CSelectionUndoUnit, Undo, "CSelectionUndoUnit")
MtDefine(CDeferredSelectionUndoUnit, Undo, "CDeferredSelectionUndoUnit")

DeclareTag(tagUndoSel, "Undo", "Selection Undo ");

//---------------------------------------------------------------------------
//
// CUndoHelper
//
//---------------------------------------------------------------------------

BOOL 
CUndoHelper::UndoDisabled()
{
    return FALSE;
}

HRESULT 
CUndoHelper::CreateAndSubmit( BOOL fDirtyChange /*=TRUE*/)
{
    HRESULT         hr = S_OK;
    IOleUndoUnit *  pUU = NULL;

    if( !UndoDisabled() && _pDoc->QueryCreateUndo( TRUE, fDirtyChange ) )
    {
        pUU = CreateUnit();
        if( !pUU )
        {
            if( fDirtyChange )
                _pDoc->FlushUndoData();

            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR( _pDoc->UndoManager()->Add( pUU ) ) ;
        if (hr)
            goto Cleanup;
    }

Cleanup:
    if( pUU )
        pUU->Release();

    RRETURN( hr );
}

BOOL
CUndoHelper::AcceptingUndo()
{
    // Query if we create undo but don't flush the stack
    return !UndoDisabled() && _pDoc->QueryCreateUndo( TRUE, FALSE );
}

//---------------------------------------------------------------------------
//
// CInsertElementUndo
//
//---------------------------------------------------------------------------

void    
CInsertElementUndo::SetData( CElement* pElement )
{ 
    Assert( !_pElement );
    CElement::SetPtr( &_pElement, pElement ); 
}

IOleUndoUnit * 
CInsertElementUndo::CreateUnit()
{
    CInsertElementUndoUnit * pUU;

    Assert( _pElement );

    TraceTag((tagUndo, "CInsertElementUndo::CreateUnit"));

    pUU = new CInsertElementUndoUnit( _pMarkup->Doc() );

    if (pUU)
    {
        pUU->SetData( _pElement, _dwFlags );
    }

    return pUU;
}

//---------------------------------------------------------------------------
//
// CInsertElementUndoUnit
//
//---------------------------------------------------------------------------

CInsertElementUndoUnit::CInsertElementUndoUnit(CDoc * pDoc)
    : CUndoUnitBase( pDoc, IDS_UNDOGENERICTEXT )
{
}

CInsertElementUndoUnit::~CInsertElementUndoUnit()
{ 
    CElement::ReleasePtr( _pElement ); 
}

void    
CInsertElementUndoUnit::SetData( CElement * pElement, DWORD dwFlags )
{ 
    Assert( !_pElement );
    CElement::SetPtr( &_pElement, pElement );
    _dwFlags = dwFlags;
}

HRESULT
CInsertElementUndoUnit::PrivateDo(IOleUndoManager *pUndoManager)
{
    HRESULT hr = S_OK;

    Assert( _pElement );
    Assert( _pElement->IsInMarkup() );

    hr = THR( _pElement->Doc()->RemoveElement( _pElement, _dwFlags ) );

    RRETURN( hr );
}

//---------------------------------------------------------------------------
//
// CRemoveElementUndo
//
//---------------------------------------------------------------------------

CRemoveElementUndo::CRemoveElementUndo( CMarkup * pMarkup, CElement * pElementRemove, DWORD dwFlags ) 
    : CMarkupUndoBase( pMarkup->Doc(), pMarkup, dwFlags ), _pElement( NULL )
{
    _fAcceptingUndo = CMarkupUndoBase::AcceptingUndo();

    if( _fAcceptingUndo )
    {
        CElement::SetPtr( &_pElement, pElementRemove );
    }
}

void
CRemoveElementUndo::SetData(
    long cpBegin, 
    long cpEnd )
{
    _cpBegin = cpBegin;
    _cpEnd = cpEnd;
}

IOleUndoUnit * 
CRemoveElementUndo::CreateUnit()
{
    CRemoveElementUndoUnit * pUU;

    Assert( _fAcceptingUndo );
    Assert( _pElement );

    TraceTag((tagUndo, "CRemoveElementUndo::CreateUnit"));

    pUU = new CRemoveElementUndoUnit( _pMarkup->Doc() );

    if (pUU)
    {
        pUU->SetData( _pMarkup, _pElement, _cpBegin, _cpEnd, _dwFlags );
    }

    return pUU;
}

//---------------------------------------------------------------------------
//
// CRemoveElementUndoUnit
//
//---------------------------------------------------------------------------
CRemoveElementUndoUnit::CRemoveElementUndoUnit(CDoc * pDoc)
    : CUndoUnitBase( pDoc, IDS_UNDOGENERICTEXT ) 
{
}


CRemoveElementUndoUnit::~CRemoveElementUndoUnit()               
{ 
    CMarkup::ReleasePtr( _pMarkup ); 
    CElement::ReleasePtr( _pElement ); 
}

void    
CRemoveElementUndoUnit::SetData( 
    CMarkup * pMarkup, 
    CElement* pElement, 
    long cpBegin, 
    long cpEnd,
    DWORD dwFlags )
{
    Assert( !_pMarkup && !_pElement );

    CMarkup::SetPtr( &_pMarkup, pMarkup );
    CElement::SetPtr( &_pElement, pElement );
    _cpBegin = cpBegin;
    _cpEnd = cpEnd;
    _dwFlags = dwFlags;
}

HRESULT
CRemoveElementUndoUnit::PrivateDo(IOleUndoManager *pUndoManager)
{
    HRESULT     hr = S_OK;
    CDoc *      pDoc = _pMarkup->Doc();
    CMarkupPointer mpBegin(pDoc), mpEnd(pDoc);

    Assert( _pElement );
    Assert( ! _pElement->IsInMarkup() );

    hr = THR( mpBegin.MoveToCp( _cpBegin, _pMarkup ) );
    if (hr)
        goto Cleanup;

    hr = THR( mpEnd.MoveToCp( _cpEnd, _pMarkup ) );
    if (hr)
        goto Cleanup;

    hr = THR( pDoc->InsertElement( _pElement, & mpBegin, & mpEnd, _dwFlags ) );

Cleanup:
    RRETURN( hr );
}

//---------------------------------------------------------------------------
//
// CInsertSpliceUndo
//
//---------------------------------------------------------------------------
IOleUndoUnit * 
CInsertSpliceUndo::CreateUnit()
{
    CInsertSpliceUndoUnit * pUU = NULL;

    Assert( _cpBegin >= 0 );

    TraceTag((tagUndo, "CInsertSpliceUndo::CreateUnit"));

    pUU = new CInsertSpliceUndoUnit( _pMarkup->Doc() );

    if (pUU)
    {
        pUU->SetData( _pMarkup, _cpBegin, _cpEnd, _dwFlags );
    }

    return pUU;
}

//---------------------------------------------------------------------------
//
// CInsertSpliceUndoUnit
//
//---------------------------------------------------------------------------
CInsertSpliceUndoUnit::CInsertSpliceUndoUnit(CDoc * pDoc)
    : CUndoUnitBase( pDoc, IDS_UNDOGENERICTEXT ) 
{
    _cpBegin = -1;
}

CInsertSpliceUndoUnit::~CInsertSpliceUndoUnit()
{
    CMarkup::ReleasePtr( _pMarkup );
}

void
CInsertSpliceUndoUnit::SetData(
    CMarkup * pMarkup, 
    long cpBegin, 
    long cpEnd,
    DWORD dwFlags )
{
    Assert( !_pMarkup );

    CMarkup::SetPtr( &_pMarkup, pMarkup );
    _cpBegin = cpBegin;
    _cpEnd = cpEnd;
    _dwFlags = dwFlags;
}

HRESULT
CInsertSpliceUndoUnit::PrivateDo(IOleUndoManager *pUndoManager)
{
    HRESULT         hr = S_OK;
    CDoc *          pDoc = _pMarkup->Doc();
    CMarkupPointer  mpBegin(pDoc), mpEnd(pDoc);

    Assert( _cpBegin >= 0 );

    hr = THR( mpBegin.MoveToCp( _cpBegin, _pMarkup ) );
    
    if (hr)
        goto Cleanup;

    hr = THR( mpEnd.MoveToCp( _cpEnd, _pMarkup ) );
    
    if (hr)
        goto Cleanup;

    hr = THR( pDoc->Remove( & mpBegin, & mpEnd, _dwFlags ) );

Cleanup:
    
    RRETURN( hr );
}

//---------------------------------------------------------------------------
//
// CRemoveSpliceUndo
//
//---------------------------------------------------------------------------
CRemoveSpliceUndo::CRemoveSpliceUndo( CDoc * pDoc ) 
    : CMarkupUndoBase( pDoc, NULL, NULL )
{
    _paryRegion = NULL;
    _pchRemoved = NULL;
    _cchNodeReinsert = 0;
    _cpBegin = -1;
}

void 
CRemoveSpliceUndo::Init( CMarkup * pMarkup, DWORD dwFlags )
{
    CMarkupUndoBase::Init( pMarkup, dwFlags );

    _fAcceptingUndo = CMarkupUndoBase::AcceptingUndo();

    if( _fAcceptingUndo )
    {
        _paryRegion = new CSpliceRecordList();
    }
}

void
CRemoveSpliceUndo::SetText(long cpBegin, long cchRemoved, long cchNodeReinsert)
{
    if( _paryRegion )
    {
        CTxtPtr tp( _pMarkup, cpBegin );

        _cpBegin = cpBegin;
        _cchRemoved = cchRemoved;
        _cchNodeReinsert = cchNodeReinsert;

        _pchRemoved = (TCHAR *)MemAlloc( Mt(CRemoveSpliceUndoUnit_pchRemoved), _cchRemoved * sizeof(TCHAR) );
        if( !_pchRemoved )
        {
            delete _paryRegion;
            _paryRegion = NULL;
            return;
        }

        Verify( tp.GetRawText( cchRemoved, _pchRemoved ) == cchRemoved );
    }
}

void
CRemoveSpliceUndo::AppendSpliceRecord( CSpliceRecord ** pprec )
{
    Assert( pprec );

    if( _paryRegion )
    {
        _paryRegion->AppendIndirect(NULL, pprec );

        if( *pprec == NULL )
        {
            delete _paryRegion;
            _paryRegion = NULL;
            MemFree( _pchRemoved );
            _pchRemoved = NULL;
        }

        return;
    }

    *pprec = NULL;
}

CSpliceRecord * 
CRemoveSpliceUndo::LastSpliceRecord()
{
    if (!_paryRegion || !_paryRegion->Size())
        return NULL;

    return (CSpliceRecord*)(*_paryRegion) + _paryRegion->Size() - 1;
}


IOleUndoUnit * 
CRemoveSpliceUndo::CreateUnit()
{
    CRemoveSpliceUndoUnit * pUU = NULL;

    if( _paryRegion && _pchRemoved )
    {
        Assert( _cpBegin >= 0 );

        TraceTag((tagUndo, "CRemoveSpliceUndo::CreateUnit"));

        pUU = new CRemoveSpliceUndoUnit( _pMarkup->Doc() );

        if (pUU)
        {
            // The undo unit owns the string 
            // and list after this

            pUU->SetData( _pMarkup, 
                          _paryRegion,
                          _cchRemoved, 
                          _pchRemoved, 
                          _cpBegin,
                          _cchNodeReinsert,
                          _dwFlags );
            
            _paryRegion = NULL;
            _pchRemoved = NULL;
        }
    }

    return pUU;
}

BOOL    
CRemoveSpliceUndo::AcceptingUndo()
{
    return _paryRegion && _fAcceptingUndo;
}


//---------------------------------------------------------------------------
//
// CRemoveSpliceUndoUnit
//
//---------------------------------------------------------------------------

CRemoveSpliceUndoUnit::CRemoveSpliceUndoUnit(CDoc * pDoc)
            : CUndoUnitBase( pDoc, IDS_UNDOGENERICTEXT ) 
{
}

CRemoveSpliceUndoUnit::~CRemoveSpliceUndoUnit()
{
    CMarkup::ReleasePtr( _pMarkup );
    delete _paryRegion;
    MemFree(_pchRemoved);
}

void    
CRemoveSpliceUndoUnit::SetData( CMarkup * pMarkup, CSpliceRecordList * paryRegion, 
                                long cchRemoved, TCHAR * pchRemoved, 
                                long cpBegin, long cchNodeReinsert, DWORD dwFlags )
{
    CMarkup::SetPtr( &_pMarkup, pMarkup );
    _paryRegion = paryRegion;
    _cchRemoved = cchRemoved;
    _pchRemoved = pchRemoved;
    _cpBegin = cpBegin;
    _cchNodeReinsert = cchNodeReinsert;
    _dwFlags = dwFlags;
}

HRESULT
CRemoveSpliceUndoUnit::PrivateDo(IOleUndoManager *pUndoManager)
{
    HRESULT hr = S_OK;
    CDoc *  pDoc = _pMarkup->Doc();

    Assert( _cpBegin >= 0 );

    CMarkupPointer mpBegin(pDoc);

    hr = THR( mpBegin.MoveToCp( _cpBegin, _pMarkup ) );
    if (hr)
        goto Cleanup;

    hr = THR( _pMarkup->UndoRemoveSplice( 
                &mpBegin, _paryRegion, _cchRemoved, _pchRemoved, _cchNodeReinsert, _dwFlags ) );
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN( hr );
}


//---------------------------------------------------------------------------
//
// CSelectionUndo
//
//---------------------------------------------------------------------------

CSelectionUndo::CSelectionUndo( CElement* pElement, CMarkup * pMarkup )
    : CMarkupUndoBase( pMarkup->Doc(), pMarkup, NULL )
{
    CElement::SetPtr( &_pElement, pElement );
    CreateAndSubmit( FALSE );
}

IOleUndoUnit * 
CSelectionUndo::CreateUnit()
{
    CSelectionUndoUnit * pUU;
    int cpStart, cpEnd = 0;
    SELECTION_TYPE eType = SELECTION_TYPE_None;
    
    Assert( _pElement );

    TraceTag((tagUndo, "CSelectionUndo::CreateUnit"));

    pUU = new CSelectionUndoUnit( _pMarkup->Doc() );

    if (pUU)    
    {
        //
        // BUGBUG - won't work for multiple selection
        //
        int ctSegment = 0;
        _pMarkup->GetSegmentCount( & ctSegment, & eType );
        if ((ctSegment > 0 ) && (eType != SELECTION_TYPE_None))
        {
            _pMarkup->GetFlattenedSelection( 0, cpStart, cpEnd, eType );   
            pUU->SetData( _pElement, cpStart, cpEnd, eType, _pMarkup);
        }
        else
        {
            //
            // We don't have a selection. Set the Element and type anyway.
            //
            pUU->SetData( _pElement, 0, 0, eType, _pMarkup );
        }
    }
    return pUU;
}

//---------------------------------------------------------------------------
//
// CSelectionUndoUnit
//
//---------------------------------------------------------------------------


CSelectionUndoUnit::CSelectionUndoUnit(CDoc * pDoc)
    : CUndoUnitBase( pDoc, IDS_UNDOGENERICTEXT )
{
    _eSelType = SELECTION_TYPE_None;
}

CSelectionUndoUnit::~CSelectionUndoUnit()
{ 
    CElement::ReleasePtr( _pElement ); 
    CMarkup::ReleasePtr( _pMarkup ); 
}

void    
CSelectionUndoUnit::SetData( 
                            CElement * pElement,  
                            int cpStart, 
                            int cpEnd, 
                            SELECTION_TYPE eSelType,
                            CMarkup *pMarkup )
{ 
    Assert( !_pElement );
    Assert( pMarkup );

    CElement::SetPtr( &_pElement, pElement );
    _cpStart = cpStart;
    _cpEnd = cpEnd;
    _eSelType = eSelType;
    CMarkup::SetPtr( &_pMarkup, pMarkup);

    TraceTag((tagUndoSel, "CSelectionUndoUnit. Set Data this:%ld type:%d start:%ld end:%ld",
                this, _eSelType, _cpStart, _cpEnd ));

}

HRESULT
CSelectionUndoUnit::PrivateDo(IOleUndoManager *pUndoManager)
{
    HRESULT hr = S_OK;

    Assert( _pElement );
    CDoc* pDoc = _pElement->Doc();
    IMarkupPointer* pStart = NULL;
    IMarkupPointer* pEnd = NULL;
    CMarkupPointer* pInternalStart = NULL;
    CMarkupPointer* pInternalEnd = NULL;
    Assert( _pElement );
    Assert( _pMarkup);

    
    if ( _eSelType != SELECTION_TYPE_None )
    {                                
        if ( pDoc->_pElemCurrent != _pElement )
        {
            _pElement->BecomeCurrent( 0 );
        }
        
        //
        // Do some work to position some MarkupPointers where we were
        //
        hr = THR( pDoc->CreateMarkupPointer( & pStart ));
        if ( hr )
            goto Cleanup;
            
        hr = THR( pDoc->CreateMarkupPointer( & pEnd ));
        if ( hr )
            goto Cleanup;   

        hr = THR( pStart->QueryInterface( CLSID_CMarkupPointer, (void**) & pInternalStart ));
        if ( hr )
            goto Cleanup;

        hr = THR( pEnd->QueryInterface( CLSID_CMarkupPointer, (void**) & pInternalEnd ));
        if ( hr )
            goto Cleanup;

        hr = THR( pInternalStart->MoveToCp( _cpStart, _pMarkup ));
        if ( hr )
            goto Cleanup;

        hr = THR( pInternalEnd->MoveToCp( _cpEnd , _pMarkup ));
        if ( hr )
            goto Cleanup;     

        //
        // Select the MarkupPointers
        //
        hr = THR( pDoc->Select( pStart, pEnd, _eSelType ));

        TraceTag(( tagUndoSel, "Selection restored to this:%ld type:%d start:%ld end:%ld", 
                this, _eSelType, _cpStart, _cpEnd ));
    }
    else
    {
        //
        // Whack any Selection that we may have 
        // (as we had nothing when the undo unit was created )
        //
        pDoc->NotifySelection( SELECT_NOTIFY_DESTROY_ALL_SELECTION, NULL );        
    }

    //
    //Create a Deferred Selection Undo
    //
    {   
        CDeferredSelectionUndo DeferUndo( _pMarkup );
    }
    
Cleanup:
    ReleaseInterface( pStart );
    ReleaseInterface( pEnd );
    RRETURN( hr );
}

//---------------------------------------------------------------------------
//
// CDeferredSelectionUndo
//
//---------------------------------------------------------------------------

CDeferredSelectionUndo::CDeferredSelectionUndo(CMarkup* pMarkup )
    : CMarkupUndoBase( pMarkup->Doc(), pMarkup, NULL ) 
{
    CreateAndSubmit( FALSE );
}

IOleUndoUnit * 
CDeferredSelectionUndo::CreateUnit()
{
    CDeferredSelectionUndoUnit * pUU;

    TraceTag((tagUndo, "CDeferredSelectionUndo::CreateUnit"));

    pUU = new CDeferredSelectionUndoUnit( _pMarkup->Doc() );

    return pUU;
}

//---------------------------------------------------------------------------
//
// CDeferredSelectionUndoUnit
//
//---------------------------------------------------------------------------

CDeferredSelectionUndoUnit::CDeferredSelectionUndoUnit(CDoc * pDoc)
    : CUndoUnitBase( pDoc, IDS_UNDOGENERICTEXT )
{
}

HRESULT
CDeferredSelectionUndoUnit::PrivateDo(IOleUndoManager *pUndoManager)
{
    HRESULT hr = S_OK;
    CDoc* pDoc = DYNCAST( CDoc, _pBase);

    TraceTag((tagUndoSel, "CDeferredSelectionUndoUnit::PrivateDo. About to create a CSelectionUndo"));
    
    CSelectionUndo Undo( pDoc->_pElemCurrent, pDoc->GetCurrentMarkup() );
    
    RRETURN( hr );
}



//+---------------------------------------------------------------------------
//
//  splice.cxx
//
//  CMarkup::SpliceTree implementation
//
//  SpliceTree can remove, copy, or move, or undo a remove 
//  of a range in the tree.
//  
//  Remove: SpliceTree's behavior is to remove all the text in the
//          specified range, as well as all elements that fall completely
//          in the range.
//
//          The sematics are such that if an element is not in a range
//          completely, its end tags will not be moved with respect
//          to other elements.  However, it may be necessary to
//          reduce the number of nodes for that element.  When this
//          happens, the nodes will be removed from the right edge.
//
//          Pointers without cling in the range end up in the space
//          between the end tags and the begin tags (arguably they should
//          stick between the end tags). Pointers with cling are removed.
//
//  Copy:   All the text in the specified range, as well as elements that
//          fall completely in the range, are copied.
//
//          Elements that overlap the range on the left are copied; begin
//          edges are implied at the very beginning of the range, in the
//          same order in which the begin edges appear in the source.
//
//          Elements that overlap the range on the right are copied; end
//          edges are implied at the very end of the range, in the same
//          order in which they appear in the source.
//
//  Move:   All the text in the specified range, as well as elements that
//          fall completely in the range, are moved (removed and inserted
//          into the new location, not copied).
//
//          Elements that overlap on the right or the left are modified
//          using the same rules as Remove, then copied to the new location
//          using the same rules as Copy.
//
//  Undo Remove: This use of SpliceTree is only called from the undo code.
//          Essentially it is a move driven by data that collected in
//          a previous remove.  To complicate matters, we must weave the
//          saved data through the already present tree.
//
//----------------------------------------------------------------------------
#include "headers.hxx"

#ifndef X__DOC_H_
#define X__DOC_H_
#include "_doc.h"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#ifndef X_MARKUPUNDO_HXX_
#define X_MARKUPUNDO_HXX_
#include "markupundo.hxx"
#endif

//+---------------------------------------------------------------------------
//
//  SpliceTree and supporting classes
//
//----------------------------------------------------------------------------

MtDefine(CSpliceTreeEngine_pchRecord, Locals, "CSpliceTreeEngine_pchRecord");
MtDefine(CSpliceTreeEngine_aryElement, Locals, "CSpliceTreeEngine_aryElement");
MtDefine(CSpliceTreeEngine_InsertSplice_aryNodeStack_pv, Locals, "CSpliceTreeEngine::InsertSplice aryNodeStack::_pv");
MtDefine(CSpliceTreeEngine_RemoveSplice_aryDelayRelease_pv, Locals, "CSpliceTreeEngine::RemoveSplice aryDelayRelease::_pv");
MtDefine(CSpliceRecordList, Undo, "CSpliceRecordList (used in SpliceTree and undo)");
MtDefine(CSpliceRecordList_pv, CSpliceRecordList, "CSpliceRecordList::_pv");

//+---------------------------------------------------------------------------
//
//  Class:      CSpliceTreeEngine
//
//  Synposis:   The class that actually does the work for SpliceTree
//
//----------------------------------------------------------------------------
class CSpliceTreeEngine
{
public:
    CSpliceTreeEngine( CDoc * pDoc );
    ~CSpliceTreeEngine();
    
    HRESULT Init(CMarkup *pMarkupSource, CTreePosGap *ptpgSourceL, CTreePosGap *ptpgSourceR,
                         CMarkup *pMarkupTarget, CTreePosGap *ptpgTarget, BOOL fRemove, DWORD dwFlags);

    HRESULT InitUndoRemove( CMarkup * pMarkupTarget, CTreePosGap * ptpgTarget,
                            CSpliceRecordList * paryRegion, long cchRemove, 
                            TCHAR * pchRemove, long cchNodeReinsert, DWORD dwFlags );
                         
    HRESULT RecordSplice();
    HRESULT RemoveSplice();
    HRESULT InsertSplice();

    // Used by RecordSplice
    HRESULT NoteRightElement(CElement *pel);
    HRESULT RecordLeftBeginElement(CElement *pel);
    HRESULT RecordBeginElement(CElement *pel);
    HRESULT RecordEndElement(long cInclTotal, long cInclSkip, BOOL fElementStays, BOOL fSynth);
    HRESULT RecordTextPos(unsigned long cch, unsigned long sid, long lTextID);
    HRESULT RecordPointer(CMarkupPointer *pmp);
    void    RecordReverse();
    HRESULT RecordText(const TCHAR *pch, long cch);
    HRESULT RecordStory(CTxtPtr *ptx, long cch);
    HRESULT RecordNodeChars(long cch);
    LONG    LeftOverlap();

    // Used by InsertSplice
    CSpliceRecord *FirstRecord();
    CSpliceRecord *NextRecord();
    void GetNodeCharsRemove( long *pcchNode );
    void GetTextRecorded(long *pcchNodeRemove, const TCHAR **ppch, long *pcch);

    // Control flags
    BOOL _fInsert;
    BOOL _fRemove;
    BOOL _fDOMOperation;

    // Source pointers
    CMarkup *_pMarkupSource;
    CTreeNode *_pnodeSourceTop;
    CTreePos *_ptpSourceL;
    CTreePos *_ptpSourceR;
    CTreeNode *_pnodeSourceL;
    CTreeNode *_pnodeSourceR;
    
    // Target pointers
    CMarkup *_pMarkupTarget;
    CTreePos *_ptpTarget;

    // Text
    TCHAR *_pchRecord;
    LONG _cchRecord;
    LONG _cchRecordAlloc;
    
    // Recorded information

    CSpliceRecord *_prec;
    LONG _crec;
    enum WhichAry { AryNone = 0, AryLeft = 1, AryInside = 2, AryDone = 3 } _cAry;
    BOOL _fReversed;

    // Undo source
    CSpliceRecordList * _paryRemoveUndo;
    BOOL                _fNoFreeRecord;
    LONG                _cchNodeRemove;
    
    CElement **_ppelRight;
    
    WHEN_DBG( BOOL  _fRemoveUndo );

    //------------------------------------------
    // Everything below this line will *not*
    // get memset to 0

    CSpliceRecordList _aryLeft;
    CSpliceRecordList _aryInside;
    
    CPtrAry<CElement *> _aryElementRight;

    // Undo classes
    CRemoveSpliceUndo   _RemoveUndo;
    CInsertSpliceUndo   _InsertUndo;
};

//+---------------------------------------------------------------------------
//
//  Method:     CSpliceRecordList dtor
//
//  Synposis:   Releases everything from the CSpliceRecordList
//
//----------------------------------------------------------------------------
CSpliceRecordList::~CSpliceRecordList()
{
    CSpliceRecord * prec = (CSpliceRecord*)PData();
    long            c = Size();

    for( ; c ; c--, prec++ )
    {
        Assert( prec->_type != CTreePos::NodeEnd
            ||  prec->_pel == NULL );

        if (prec->_type == CTreePos::NodeBeg)
        {
            Assert( prec->_pel );
            prec->_pel->Release();
        }
        else if (prec->_type == CTreePos::Pointer)
        {
            prec->_pPointer->Release();
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     SpliceTreeEngine constructor
//
//  Synposis:   Zero initialize members
//
//----------------------------------------------------------------------------

CSpliceTreeEngine::CSpliceTreeEngine(CDoc * pDoc) :
    _aryElementRight(Mt(CSpliceTreeEngine_aryElement)),
    _RemoveUndo( pDoc ), _InsertUndo( pDoc )
{
    // Zero up to but not including the arrays
    
    memset(this, 0, offsetof(CSpliceTreeEngine, _aryLeft));
}


//+---------------------------------------------------------------------------
//
//  Method:     SpliceTreeEngine destructor
//
//  Synposis:   Release pointers
//
//----------------------------------------------------------------------------
    
CSpliceTreeEngine::~CSpliceTreeEngine()
{
    CElement **ppel;
    long c;

    for (ppel = _aryElementRight, c = _aryElementRight.Size(); c; ppel++, c--)
    {
        (*ppel)->Release();
    }
    
    if( !_fNoFreeRecord )
        MemFree(_pchRecord);

    if (_pMarkupTarget)
        _pMarkupTarget->PrivateRelease();

    if (_pMarkupSource)
        _pMarkupSource->PrivateRelease();
}


//+---------------------------------------------------------------------------
//
//  Method:     CSpliceTreeEngine::Init
//
//  Synposis:   Initialize members of CSpliceTreeEngine based on arguments.
//
//              Unpositions gaps that come in as input.
//
//----------------------------------------------------------------------------
HRESULT
CSpliceTreeEngine::Init(CMarkup *pMarkupSource, CTreePosGap *ptpgSourceL, CTreePosGap *ptpgSourceR,
                        CMarkup *pMarkupTarget, CTreePosGap *ptpgTarget, BOOL fRemove, DWORD dwFlags)
{
    HRESULT hr = S_OK;

    Assert(_fRemoveUndo == !pMarkupSource);
    Assert(_fRemoveUndo || ptpgSourceL && ptpgSourceL->IsPositioned() && ptpgSourceL->IsValid());
    Assert(_fRemoveUndo || ptpgSourceR && ptpgSourceR->IsPositioned() && ptpgSourceR->IsValid());
    Assert(_fRemoveUndo || ptpgSourceL->AttachedTreePos()->GetMarkup() == pMarkupSource);
    Assert(_fRemoveUndo || ptpgSourceR->AttachedTreePos()->GetMarkup() == pMarkupSource);
    Assert(_fRemoveUndo || ptpgSourceL->Branch() && ptpgSourceR->Branch());
    Assert(_fRemoveUndo || 0 >= ptpgSourceL->AdjacentTreePos(TPG_LEFT)->InternalCompare( ptpgSourceR->AdjacentTreePos(TPG_LEFT)));

    Assert(!_fRemoveUndo || pMarkupTarget);
    Assert(!!pMarkupTarget == !!ptpgTarget);
    Assert(!ptpgTarget || ptpgTarget->AttachedTreePos()->GetMarkup() == pMarkupTarget);
    Assert(!ptpgTarget || (ptpgTarget->IsPositioned() && ptpgTarget->IsValid()));
    Assert(pMarkupTarget || fRemove);

    Assert( !pMarkupTarget || !pMarkupSource || pMarkupTarget->Doc() == pMarkupSource->Doc() );
    
    _fInsert = !!pMarkupTarget;
    _fRemove = fRemove;

    _fDOMOperation = dwFlags & MUS_DOMOPERATION;

    if (_fInsert)
    {
        _pMarkupTarget = pMarkupTarget;
        _pMarkupTarget->PrivateAddRef();

        _InsertUndo.Init( _pMarkupTarget, dwFlags );
        
        if (_pMarkupTarget->Doc()->_lLastTextID)
        {
            _pMarkupTarget->SplitTextID(
                ptpgTarget->AdjacentTreePos( TPG_LEFT ),
                ptpgTarget->AdjacentTreePos( TPG_RIGHT) );
        }

        ptpgTarget->PartitionPointers(_pMarkupTarget, _fDOMOperation);

        _ptpTarget = _pMarkupTarget->NewPointerPos( NULL, FALSE, FALSE );

        if (!_ptpTarget)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(
            _pMarkupTarget->Insert(
                _ptpTarget, ptpgTarget->AdjacentTreePos( TPG_LEFT ), FALSE ) );

        if (hr)
            goto Cleanup;

        ptpgTarget->UnPosition();
    }
    
    if (pMarkupSource)
    {
        _pMarkupSource = pMarkupSource;
        _pMarkupSource->PrivateAddRef();

        if( _fRemove )
        {
            ptpgSourceL->PartitionPointers(_pMarkupSource, _fDOMOperation);
            ptpgSourceR->PartitionPointers(_pMarkupSource, _fDOMOperation);

            if (_pMarkupSource->Doc()->_lLastTextID)
            {
                _pMarkupSource->SplitTextID(
                    ptpgSourceL->AdjacentTreePos( TPG_LEFT ),
                    ptpgSourceL->AdjacentTreePos( TPG_RIGHT) );

                _pMarkupSource->SplitTextID(
                    ptpgSourceR->AdjacentTreePos( TPG_LEFT ),
                    ptpgSourceR->AdjacentTreePos( TPG_RIGHT) );
            }

            _RemoveUndo.Init( _pMarkupSource, dwFlags );
        }

        _ptpSourceL = ptpgSourceL->AdjacentTreePos(TPG_RIGHT);
        _ptpSourceR = ptpgSourceR->AdjacentTreePos(TPG_LEFT);
        _pnodeSourceL = ptpgSourceL->Branch();
        _pnodeSourceR = ptpgSourceR->Branch();
        _pnodeSourceTop = _pnodeSourceL->GetFirstCommonAncestorNode(_pnodeSourceR, NULL);
        ptpgSourceL->UnPosition();
        ptpgSourceR->UnPosition();
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Method:     CSpliceTreeEngine::InitUndoRemove
//
//  Synposis:   Initialize for the undo of a remove operation
//
//----------------------------------------------------------------------------
HRESULT 
CSpliceTreeEngine::InitUndoRemove(CMarkup * pMarkupTarget, CTreePosGap * ptpgTarget,
                                  CSpliceRecordList * paryRegion, long cchRemove, 
                                  TCHAR * pchRemove, long cchNodeReinsert, DWORD dwFlags )
{
    HRESULT hr = S_OK;

    WHEN_DBG( _fRemoveUndo = TRUE );

    hr = THR( Init( NULL, NULL, NULL, pMarkupTarget, ptpgTarget, FALSE, dwFlags ) );
    if (hr)
        goto Cleanup;

    Assert( paryRegion );
    _paryRemoveUndo = paryRegion;

    Assert( pchRemove );
    _pchRecord = pchRemove;
    _cchRecord = cchRemove;
    _fNoFreeRecord = TRUE;
    _cchNodeRemove = cchNodeReinsert;

Cleanup:
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Method:     CSpliceTreeEngine::NoteRightElement
//
//  Synposis:   Note element on the right, in bottom-to-top order
//
//----------------------------------------------------------------------------
HRESULT
CSpliceTreeEngine::NoteRightElement(CElement *pel)
{
    HRESULT hr;

    Assert(!_ppelRight || _ppelRight == _aryElementRight + _aryElementRight.Size() - 1);
    
    hr = THR(_aryElementRight.Append(pel));
    if (hr) 
        goto Cleanup;

    _ppelRight = _aryElementRight + _aryElementRight.Size() - 1;

    pel->AddRef();

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Method:     CSpliceTreeEngine::RecordLeftBeginElement
//
//  Synposis:   Records a begin-edge before the left edge of the source;
//              these are fed in reverse (bottom up, right-to-left) order.
//
//----------------------------------------------------------------------------
HRESULT
CSpliceTreeEngine::RecordLeftBeginElement(CElement *pel)
{
    HRESULT hr;
    CElement *pelClone = NULL;
    CSpliceRecord *prec;

    Assert(_pMarkupTarget && _fInsert);
    
    hr = THR(pel->Clone(&pelClone, _pMarkupTarget->Doc()));
    if (hr)
        goto Cleanup;
        
    hr = THR(_aryLeft.AppendIndirect(NULL, &prec));
    if (hr) 
        goto Cleanup;

    prec->_type = CTreePos::NodeBeg;
    prec->_pel = pelClone;
    prec->_fSkip = FALSE;
    pelClone = NULL;

Cleanup:
    CElement::ReleasePtr(pelClone);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Method:     CSpliceTreeEngine::RecordBeginElement
//
//  Synposis:   Records a begin-edge in the source, and clones the
//              element if needed.
//
//----------------------------------------------------------------------------
HRESULT
CSpliceTreeEngine::RecordBeginElement(CElement *pel)
{
    HRESULT         hr = S_OK;
    CSpliceRecord * prec;
    BOOL            fElementStays = FALSE;
    CElement *      pelNew = NULL;

    if( _fRemove && _ppelRight && pel == *_ppelRight )
    {
        fElementStays = TRUE;

        if (_ppelRight != _aryElementRight)
            _ppelRight -= 1;
        else
            _ppelRight = NULL;
    }

    // Handle the insert case
    if( _fInsert )
    {
        if (!_fRemove || fElementStays)
        {
            hr = THR(pel->Clone(&pelNew, _pMarkupTarget->Doc()));
            if (hr)
                goto Cleanup;
        }
        else
        {
            pelNew = pel;
            pel->AddRef();
        }
        
        hr = THR(_aryInside.AppendIndirect(NULL, &prec));
        if (hr) 
            goto Cleanup;

        prec->_type = CTreePos::NodeBeg;
        prec->_pel = pelNew;
        prec->_fSkip = FALSE;
        pelNew = NULL;
    }

    // Handle the undo case
    if (_fRemove)
    {
        _RemoveUndo.AppendSpliceRecord( &prec );

        if (prec)
        {
            prec->_type = CTreePos::NodeBeg;
            prec->_pel = pel;
            pel->AddRef();
            prec->_fSkip = fElementStays;
        }
    }

Cleanup:
    CElement::ReleasePtr(pelNew);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Method:     CSpliceTreeEngine::RecordEndElement
//
//  Synposis:   Records a end-edge in the source, and the amount of
//              overlapping (the depth of the inclusion) at the point
//              at which it ends.
//
//----------------------------------------------------------------------------
HRESULT
CSpliceTreeEngine::RecordEndElement(long cInclTotal, long cInclSkip, BOOL fElementStays, BOOL fSynth)
{
    HRESULT hr = S_OK;
    CSpliceRecord *prec;
    
    if (_fInsert)
    {
        hr = THR(_aryInside.AppendIndirect(NULL, &prec));
        if (hr)
            goto Cleanup;

        prec->_type = CTreePos::NodeEnd;
        prec->_pel = NULL;
        prec->_cIncl = cInclTotal - cInclSkip;
        prec->_fSkip = FALSE;
    }

    if (_fRemove && !fSynth)
    {
        _RemoveUndo.AppendSpliceRecord( &prec );

        if (prec)
        {
            prec->_type = CTreePos::NodeEnd;
            prec->_pel = NULL;
            prec->_cIncl = cInclTotal;
            prec->_fSkip = fElementStays;
        }
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Method:     CSpliceTreeEngine::LeftOverlap
//
//  Synposis:   Returns the number of elements partially overlapping range
//              to the left
//
//----------------------------------------------------------------------------
LONG
CSpliceTreeEngine::LeftOverlap()
{
    return _aryLeft.Size();
}


//+---------------------------------------------------------------------------
//
//  Method:     CSpliceTreeEngine::RecordTextPos
//
//  Synposis:   Records a text pos in the source, and merges adjacent
//              text poses if possible (this happens when tree pointers
//              are not copied out of the splice).
//
//----------------------------------------------------------------------------
HRESULT
CSpliceTreeEngine::RecordTextPos(unsigned long cch, unsigned long sid, long lTextID)
{
    HRESULT hr = S_OK;
    CSpliceRecord *prec;

    if (_fInsert)
    {
        if (    _aryInside.Size() 
            &&  (prec = _aryInside + _aryInside.Size() - 1, prec->_type == CTreePos::Text) 
            &&  prec->_sid == sid
            &&  prec->_lTextID == lTextID)
        {
            prec->_cch += cch;
        }
        else
        {
            hr = THR(_aryInside.AppendIndirect(NULL, &prec));
            if (hr)
                goto Cleanup;

            prec->_type = CTreePos::Text;
            prec->_cch = cch;
            prec->_sid = sid;
            prec->_lTextID = lTextID;
        }
    }

    if (_fRemove)
    {
        prec = _RemoveUndo.LastSpliceRecord();

        if (    prec 
            &&  prec->_type == CTreePos::Text 
            &&  prec->_sid == sid
            &&  prec->_lTextID == lTextID)
        {
            prec->_cch += cch;
        }
        else
        {
            _RemoveUndo.AppendSpliceRecord( &prec );

            if (prec)
            {
                prec->_type = CTreePos::Text;
                prec->_cch = cch;
                prec->_sid = sid;
                prec->_lTextID = lTextID;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Method:     CSpliceTreeEngine::RecordPointer
//
//  Synposis:   Records a markup pointer in the source, and addrefs it.
//
//----------------------------------------------------------------------------
HRESULT
CSpliceTreeEngine::RecordPointer(CMarkupPointer *pmp)
{
    HRESULT hr = S_OK;
    CSpliceRecord *prec;
    
    if (_fInsert)
    {
        hr = THR(_aryInside.AppendIndirect(NULL, &prec));
        if (hr)
            goto Cleanup;

        prec->_type = CTreePos::Pointer;
        prec->_pPointer = pmp;
        pmp->AddRef();
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Method:     CSpliceTreeEngine::FirstRecord
//
//  Synposis:   Returns the leftmost item recorded in RecordSplice
//
//----------------------------------------------------------------------------
CSpliceRecord *
CSpliceTreeEngine::FirstRecord()
{
    Assert(_paryRemoveUndo || _fReversed || (!_prec && !_crec && !_cAry));

    // 1. Reverse _aryLeft

    if (!_paryRemoveUndo && !_fReversed)
    {
        CSpliceRecord *precL;
        CSpliceRecord *precR;
        CSpliceRecord rec;

        if (_aryLeft.Size() > 1)
        {
            precL = _aryLeft;
            precR = _aryLeft + _aryLeft.Size() - 1;

            while (precL < precR)
            {
                memcpy(&rec, precL, sizeof(CSpliceRecord));
                memcpy(precL, precR, sizeof(CSpliceRecord));
                memcpy(precR, &rec, sizeof(CSpliceRecord));
                precL += 1;
                precR -= 1;
            }
        }

        _fReversed = TRUE;
    }

    // 2. Return first thing
    
    _prec = NULL;
    _crec = 0;
    _cAry = AryNone;

    return NextRecord();
}


//+---------------------------------------------------------------------------
//
//  Method:     CSpliceTreeEngine::NextRecord
//
//  Synposis:   Advances to the next (to the right) item recorded
//              in RecordSplice
//
//----------------------------------------------------------------------------
CSpliceRecord *
CSpliceTreeEngine::NextRecord()
{
    Assert(_paryRemoveUndo || _fReversed);
    
    _prec += 1;
    
    while (!_crec)
    {
        _cAry = (WhichAry)(_cAry + 1);

        switch (_cAry)
        {
        case AryLeft:
            if (!_paryRemoveUndo)
            {
                _crec = _aryLeft.Size();
                _prec = _aryLeft;
            }
            break;
            
        case AryInside:
            if (!_paryRemoveUndo)
            {
                _crec = _aryInside.Size();
                _prec = _aryInside;
            }
            else
            {
                _crec = _paryRemoveUndo->Size();
                _prec = *_paryRemoveUndo;
            }
            break;
            
        case AryDone:
            return NULL;

        default:
            AssertSz(0, "Iterated past end");
        }
    }
    
    _crec -= 1;

    return _prec;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSpliceTreeEngine::RecordStory
//
//  Synposis:   Accumulates text to be copied from the splice.
//
//              ptx should also be advanced by cch
//
//----------------------------------------------------------------------------
HRESULT
CSpliceTreeEngine::RecordStory(CTxtPtr *ptx, long cch)
{
    HRESULT hr = S_OK;
    const TCHAR *pch;
    LONG cchValid;

    Assert( _fInsert );
    
    while (cch)
    {
        pch = ptx->GetPch(cchValid);
        if (cchValid > cch)
            cchValid = cch;

        hr = THR(RecordText(pch, cchValid));
        if (hr)
            goto Cleanup;
            
        ptx->AdvanceCp(cchValid);
        cch -= cchValid;
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Method:     CSpliceTreeEngine::RecordText
//
//  Synposis:   Records a string
//
//----------------------------------------------------------------------------
HRESULT
CSpliceTreeEngine::RecordText(const TCHAR *pch, long cch)
{
    HRESULT hr = S_OK;

    Assert( _fInsert );
    
    if (_cchRecord + cch > _cchRecordAlloc)
    {
        if (!_cchRecordAlloc)
             _cchRecordAlloc = 16;

        do _cchRecordAlloc *= 2;
            while (_cchRecord + cch > _cchRecordAlloc);
            
        hr = THR(MemRealloc(Mt(CSpliceTreeEngine_pchRecord), (void **)&_pchRecord, _cchRecordAlloc * sizeof(TCHAR)));
        if (hr)
            goto Cleanup;
    }

    memcpy(_pchRecord + _cchRecord, pch, cch * sizeof(TCHAR));

    _cchRecord += cch;

Cleanup:
    RRETURN(hr);
}

static const TCHAR achNodeChars[32] = {
    WCH_NODE, WCH_NODE, WCH_NODE, WCH_NODE, WCH_NODE, WCH_NODE, WCH_NODE, WCH_NODE,
    WCH_NODE, WCH_NODE, WCH_NODE, WCH_NODE, WCH_NODE, WCH_NODE, WCH_NODE, WCH_NODE,
    WCH_NODE, WCH_NODE, WCH_NODE, WCH_NODE, WCH_NODE, WCH_NODE, WCH_NODE, WCH_NODE,
    WCH_NODE, WCH_NODE, WCH_NODE, WCH_NODE, WCH_NODE, WCH_NODE, WCH_NODE, WCH_NODE };

//+---------------------------------------------------------------------------
//
//  Method:     CSpliceTreeEngine::RecordNodeChars
//
//  Synposis:   Records a certain number of nodechars
//
//----------------------------------------------------------------------------
HRESULT
CSpliceTreeEngine::RecordNodeChars(long cch)
{
    HRESULT hr = S_OK;
    LONG cch2;

    Assert( _fInsert );

    cch2 = ARRAY_SIZE(achNodeChars);

    while (cch)
    {
        if (cch < cch2)
            cch2 = cch;
            
        hr = THR(RecordText(achNodeChars, cch2));
        if (hr)
            goto Cleanup;

        cch -= cch2;
    }
    
Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Method:     CSpliceTreeEngine::TextRecorded
//
//  Synposis:   Returns the text recorded by RecordText
//
//----------------------------------------------------------------------------
void
CSpliceTreeEngine::GetTextRecorded(long *pcchNodeRemove, const TCHAR **ppch, long *pcch)
{
    *pcchNodeRemove = _cchNodeRemove;
    *ppch = _pchRecord;
    *pcch = _cchRecord;
}


//+---------------------------------------------------------------------------
//
//  Method:     CSpliceTreeEngine::RecordSplice
//
//  Synposis:   Does not change the tree; is paired with InsertSplice
//
//              Traverses the region to be spliced and records all
//              begin-edges, end-edges, text poses, text, and pointers
//              to be spliced.
//
//              Elements that partially overlap the left or the right
//              of the region are treated specially: implicit begin-edges
//              (on the left) and end-edges (on the right) are noted.
//
//              Elements are cloned into the record if !_fRemove or
//              if they partially overlap the region.
//
//              Differentiates pointers with cling from those
//              without, and only records those with cling.
//
//              Most text is copied verbatim. But elements that entirely
//              cover the region are special: node chars for those
//              elements must be skipped when they appear in inclusions.
//
//----------------------------------------------------------------------------
HRESULT
CSpliceTreeEngine::RecordSplice()
{
    // Record all begin edges, end edges, and text
    HRESULT hr = S_OK;
    CTxtPtr tx(_pMarkupSource, _ptpSourceL->GetCp());
    CTreeNode *pnode;
    CTreeNode *pnodeLimit;
    CTreePos *ptp;
    LONG c;
    LONG cIncl;
    LONG cLimit;
    LONG cSkip;
    LONG cch;

    // Step 1: mark and record all the L-overlapping elements and
    // note all the R-overlapping elements

    for (pnode = _pnodeSourceR; pnode != _pnodeSourceTop; pnode = pnode->Parent())
        pnode->Element()->_fMark1 = FALSE;

    for (pnode = _pnodeSourceL; pnode != _pnodeSourceTop; pnode = pnode->Parent())
        pnode->Element()->_fMark1 = TRUE;
        
    for (pnode = _pnodeSourceR; pnode != _pnodeSourceTop; pnode = pnode->Parent())
    {
        if (pnode->Element()->_fMark1 == FALSE)
        {
            hr = THR(NoteRightElement(pnode->Element()));
            if (hr)
                goto Cleanup;
        }
        else
        {
            pnode->Element()->_fMark1 = FALSE;
        }
    }

    if (_fInsert)
    {
        for (pnode = _pnodeSourceL; pnode != _pnodeSourceTop; pnode = pnode->Parent())
        {
            if (pnode->Element()->_fMark1)
            {
                hr = THR(RecordLeftBeginElement(pnode->Element()));
                if (hr)
                    goto Cleanup;
            }
        }

        hr = THR(RecordNodeChars(LeftOverlap()));
        if (hr)
            goto Cleanup;
    }

    // Step 2: scan range, recording all begin-edges, end-edges, and text

    cIncl = 0;
    cch = 0;
    pnodeLimit = _pnodeSourceL;
    
    if (_ptpSourceR->NextTreePos() != _ptpSourceL) for (ptp = _ptpSourceL; ; ptp = ptp->NextTreePos())
    {
        Assert( ptp->InternalCompare( _ptpSourceR ) <= 0 );

        switch (ptp->Type())
        {
        case CTreePos::NodeBeg:
        
            if (ptp->IsEdgeScope())
            {
                Assert(!cIncl);
                
                hr = THR(RecordBeginElement(ptp->Branch()->Element()));
                if (hr)
                    goto Cleanup;
            }
            else
            {
                cIncl -= 1;
            }
            
            cch += 1;

            break;

        case CTreePos::NodeEnd:
        
            if (ptp->Branch() == pnodeLimit) // it's an inclusion out of which we need to skip some poses
            {
                cSkip = 0;

                for (c = 0; !ptp->IsEdgeScope(); ptp = ptp->NextTreePos(), c += 1)
                    if (!ptp->Branch()->Element()->_fMark1)
                        cSkip += 1;
                
                Assert(ptp->IsEndElementScope());

                hr = THR(RecordEndElement(cIncl + c, cSkip, TRUE, FALSE));
                if (hr)
                    goto Cleanup;

                cch += 2 * (c - cSkip) + 1;
                
                for (; c; ptp = ptp->NextTreePos(), c -= 1);

                pnodeLimit = ptp->IsEdgeScope() ? pnodeLimit->Parent() : ptp->Branch();

                if (cSkip && _fInsert)
                {
                    hr = THR(RecordStory(&tx, cch));
                    if (hr)
                        goto Cleanup;
                        
                    tx.AdvanceCp(cSkip * 2);
                    cch = 0;
                }
            }
            else
            {
                if (ptp->IsEdgeScope())
                {
                    hr = THR(RecordEndElement(cIncl, 0, FALSE, FALSE));
                    if (hr)
                        goto Cleanup;
                }
                else
                {
                    cIncl += 1;
                }

                cch += 1;
            }

            break;
            
        case CTreePos::Text:

            Assert(cIncl == 0);
        
            hr = THR(RecordTextPos(ptp->Cch(), ptp->Sid(), _fRemove ? ptp->TextID() : 0));
            if (hr)
                goto Cleanup;

            cch += ptp->Cch();
            
            break;

        case CTreePos::Pointer:

            Assert(cIncl == 0);
        
            if (ptp->Cling() && _fRemove)
            {
                hr = THR(RecordPointer(ptp->MarkupPointer()));
                if (hr)
                    goto Cleanup;
            }
        }
        
        if (ptp == _ptpSourceR)
            break;
    }

    Assert(cIncl == 0);
    
    // 3. Flush text
    
    if (cch && _fInsert)
    {
        hr = THR(RecordStory(&tx, cch));
        if (hr)
            goto Cleanup;
    }

    // 4. Finish scan to get right-overlap element end edges in correct order
    if (_fInsert)
    {
        for (pnode = _pnodeSourceR, cLimit = 0; pnode != pnodeLimit; pnode = pnode->Parent(), cLimit += 1);
        pnode = _pnodeSourceR;
        cch = 0;

        while (cLimit)
        {
            for (ptp = pnode->GetEndPos(), cIncl = 0; !ptp->IsEdgeScope(); ptp = ptp->NextTreePos(), cIncl += 1);
        
            if (cIncl < cLimit)
            {
                hr = THR(RecordEndElement(cIncl, 0, FALSE, TRUE));
                if (hr)
                    goto Cleanup;

                cch += cIncl * 2 + 1;
                cLimit -= 1;
            }
        
            for (; cIncl; ptp = ptp->NextTreePos(), cIncl -= 1);

            pnode = ptp->IsEdgeScope() ? pnode->Parent() : ptp->Branch();
        }

        hr = THR(RecordNodeChars(cch));
        if (hr)
            goto Cleanup;
    }

Cleanup:

    RRETURN(hr);
    
}

//+---------------------------------------------------------------------------
//
//  Helper functions for sending notifications in InsertSplice
//
//----------------------------------------------------------------------------

static void 
SendTextAddedNotification( CMarkup * pMarkup, CTreeNode * pNode, long cp, long cch )
{
    CNotification nfText;
    
    nfText.CharsAdded( cp, cch, pNode );
    WHEN_DBG( nfText._fNoTextValidate = TRUE );
    
    pMarkup->Notify( nfText );
}

static void 
SendElementAddedNotification( CMarkup * pMarkup, long si, long cel )
{
    CNotification nfElements;
    
    nfElements.ElementsAdded( si, cel );
    WHEN_DBG( nfElements._fNoElementsValidate = TRUE );
    
    pMarkup->Notify( nfElements );
}

//+---------------------------------------------------------------------------
//
//  Method:     CSpliceTreeEngine::InsertSplice
//
//  Synposis:   Inserts the elements and text noted by RecordSplice
//
//----------------------------------------------------------------------------
HRESULT
CSpliceTreeEngine::InsertSplice()
{
    HRESULT hr;

    CSpliceRecord *prec;
    long c, cpTarget;
    CTreeNode *pnodeNew;
    CTreeNode *pnodeCur;
    CTreePos *ptpIns;
    CTreePos *ptp;
    CTreePos *ptpNew;

    // As we make the pass inserting stuff,
    // we will fix up the parent chain as we hit
    // the end of the node instead of at the begining
    // We do this so that notifications won't go to elements
    // that are currently being inserted.
    CStackPtrAry<CTreeNode *, 32> aryNodeStack(Mt(CSpliceTreeEngine_InsertSplice_aryNodeStack_pv));

    long        siNotify, cElementsNotify = 0;
    long        cpNotify, cchNotify = 0;
    CTreeNode * pNodeNotify;

    ptpIns = _ptpTarget->NextTreePos();
    
    pnodeCur = _ptpTarget->GetBranch();
    pNodeNotify = pnodeCur;

    // Record where we will want to insert the text
    cpTarget = _ptpTarget->GetCp();
    cpNotify = cpTarget;

    siNotify = _ptpTarget->SourceIndex();

    // 1. Prime the node stack
    aryNodeStack.Append( pnodeCur );

    // 2. Replay the recorded tree
    
    for (prec = FirstRecord(); prec; prec = NextRecord())
    {
        switch (prec->_type)
        {
        case CTreePos::NodeBeg:
            
            // If _fSkip is turned on, the make sure that the target
            // and our list are in sync.  Just flush notifications and
            // skip over the pos in the target
            if( prec->_fSkip )
            {
                // Move past any pointer pos
                while (ptpIns->IsPointer())
                    ptpIns = ptpIns->NextTreePos();

                Assert( ptpIns->IsBeginElementScope( prec->_pel ) );

                if( cchNotify )
                    SendTextAddedNotification( _pMarkupTarget, pNodeNotify , cpNotify, cchNotify);

                cpNotify += cchNotify + 1;
                cchNotify = 0;
                pNodeNotify = ptpIns->Branch();

                if( cElementsNotify )
                    SendElementAddedNotification( _pMarkupTarget, siNotify, cElementsNotify );

                siNotify += cElementsNotify + 1;
                cElementsNotify = 0;

                pnodeCur = ptpIns->Branch();
                aryNodeStack.Append( pnodeCur );

                // Advance ptpIns (while adjusting pointers)
                {
                    CTreePosGap tpgAdjust( ptpIns, TPG_RIGHT );
                    tpgAdjust.PartitionPointers(_pMarkupTarget, _fDOMOperation);
                    ptpIns = tpgAdjust.AdjacentTreePos( TPG_RIGHT );
                }
            }
            else
            {
                // Create a new node and insert it in the markup
                pnodeNew = new CTreeNode(pNodeNotify, prec->_pel);
                if (!pnodeNew)
                    goto OutOfMemory;

                ptpNew = pnodeNew->InitBeginPos(TRUE);

                hr = THR(_pMarkupTarget->Insert(ptpNew, ptpIns, TRUE));
                if (hr)
                    goto Cleanup;

                prec->_pel->__pNodeFirstBranch = pnodeNew;

                prec->_pel->SetMarkupPtr(_pMarkupTarget);
                prec->_pel->PrivateEnterTree();

                pnodeNew->PrivateEnterTree();

                pnodeCur = pnodeNew;
                aryNodeStack.Append( pnodeCur );
                cElementsNotify++;
                cchNotify++; 
            }
            break;

        case CTreePos::NodeEnd:
            // There are three cases we have to handle here:
            // 1) The inclusion is all-new
            // 2) The inclusion is new but includes some elements
            //    which were already in the tree
            // 3) The inclusion already exists and we need to thread
            //    elements through it
            //
            // 1 and 2 happen when _fSkip is off.  2 happens as soon
            // as we hit pNodeNotify as we are couting off the inclusions.
            // 3 happens only when _fSkip is on.
            {
                CTreePos *  ptpKernel;
                CTreeNode * pNodeStay = pNodeNotify;
                CTreeNode * pNodeCurL;
                long        iNodeStack = aryNodeStack.Size() - 1;
                long        cInclStay = 0;
                BOOL        fInclusionCreated = FALSE;

                // Move past any pointer pos
                if( prec->_fSkip )
                {
                    while (ptpIns->IsPointer())
                        ptpIns = ptpIns->NextTreePos();
                }

                // Create/skip the left half of the inclusion.  We use the parent 
                // chain starting with pNodeNotify as the list of elements already 
                // in the tree and we use aryNodeStack as the total list of elements at
                // this point

                for( c = prec->_cIncl; c; c--, iNodeStack-- )
                {
                    Assert( pnodeCur == aryNodeStack[ aryNodeStack.Size() - 1 ] );

                    if (pnodeCur == pNodeStay)
                    {
                        if (prec->_fSkip)
                        {
                            if( cchNotify )
                                SendTextAddedNotification( _pMarkupTarget, pNodeNotify , cpNotify, cchNotify);

                            cpNotify += cchNotify + 1;
                            cchNotify = 0;
                        }
                        // We hit case 2 so create an inclusion so we
                        // can sorta treat it like case 3
                        else if (!fInclusionCreated)
                        {
                            CTreePosGap tpgLocation( ptpIns, TPG_LEFT, TPG_LEFT );
                            CTreeNode * pNodeStop = aryNodeStack[ iNodeStack - c ]->Parent();
                            long        cchInserted;

                            hr = THR( _pMarkupTarget->CreateInclusion( 
                                        pNodeStop, &tpgLocation, NULL, &cchInserted, pnodeCur ) );
                            if (hr)
                                goto Cleanup;
                                                    
                            fInclusionCreated = TRUE;

                            ptpIns = tpgLocation.AdjacentTreePos( TPG_RIGHT );

                            cchNotify += cchInserted;
                        }

                        Assert( ptpIns->IsEndNode() 
                            &&  !ptpIns->IsEdgeScope() 
                            &&  ptpIns->Branch() == pNodeStay );

                        ptpIns = ptpIns->NextTreePos();

                        pNodeStay = pNodeStay->Parent();
                        cInclStay++;
                    }
                    else
                    {
                        // This is an element that is being inserted
                        // so thread it through the inclusion

                        Assert( pnodeCur->GetEndPos()->IsUninit() );
                        ptpNew = pnodeCur->InitEndPos(FALSE);
            
                        hr = THR(_pMarkupTarget->Insert(ptpNew, ptpIns, TRUE));
                        if (hr)
                            goto Cleanup;

                        cchNotify++;
                    }

                    Assert( iNodeStack >= 0 && iNodeStack == aryNodeStack.Size() - 1 );
                    aryNodeStack.Delete( iNodeStack );
                    if( iNodeStack == 0 )
                    {
                        iNodeStack = 1;
                        aryNodeStack.Append( pnodeCur->Parent() );
                    }
                    pnodeCur = aryNodeStack[ iNodeStack - 1 ];
                }

                //
                // Now handle the kernel of the inclusion
                //

                if( prec->_fSkip )
                {                            
                    if( cchNotify )
                        SendTextAddedNotification( _pMarkupTarget, pNodeNotify , cpNotify, cchNotify);

                    cpNotify += cchNotify + 1;
                    cchNotify = 0;

                    Assert( pnodeCur == pNodeStay );
                    Assert( ptpIns->IsEndElementScope() && ptpIns->Branch() == pnodeCur );

                    ptpKernel = ptpIns;
                    ptpIns = ptpIns->NextTreePos();
                }
                else
                {
                    // Insert the kernel of the inclusion
                    Assert( pnodeCur->GetEndPos()->IsUninit() );
                    ptpKernel = pnodeCur->InitEndPos(TRUE);
        
                    hr = THR(_pMarkupTarget->Insert(ptpKernel, ptpIns, TRUE));
                    if (hr)
                        goto Cleanup;

                    cchNotify++;
                }

                pNodeStay = pNodeNotify = pnodeCur->Parent();

                // pop the kernel off of the stack
                Assert( iNodeStack >= 0 && iNodeStack == aryNodeStack.Size() - 1 );
                aryNodeStack.Delete( iNodeStack );
                if( iNodeStack == 0 )
                {
                    iNodeStack = 1;
                    aryNodeStack.Append( pnodeCur->Parent() );
                }
                iNodeStack--;
                pnodeCur = aryNodeStack[ iNodeStack ];

                // reparent the kernel node
                ptpKernel->Branch()->SetParent( pnodeCur );

                //
                // Find the node to send notifications on the right
                //
                if (cInclStay)
                {
                    for( c = cInclStay - 1, ptp = ptpIns; c; c--, ptp = ptp->NextTreePos() )
                    {
                        Assert( ptp->IsBeginNode() && ! ptp->IsEdgeScope() );
                    }

                    Assert( ptp->IsBeginNode() && ! ptp->IsEdgeScope() );

                    pNodeNotify = ptp->Branch();
                }

                //
                // Finish off the right side of the inclusion and fix up the parent
                // chain on the left
                //

                pNodeCurL = ptpKernel->Branch();

                for (ptp = ptpKernel->PreviousTreePos(), c = prec->_cIncl; c; ptp = ptp->PreviousTreePos(), c--)
                {
                    Assert( ptp->IsEndNode() && !ptp->IsEdgeScope() );

                    if( cInclStay && ptp->Branch()->Element() == ptpIns->Branch()->Element() )
                    {
                        // Advance past the node that is already there
                        Assert( pNodeStay == ptpIns->Branch()->Parent() );
                        pNodeStay = ptpIns->Branch();

                        if( prec->_fSkip )
                        {
                            if( cchNotify )
                                SendTextAddedNotification( _pMarkupTarget, pNodeNotify , cpNotify, cchNotify);

                            cpNotify += cchNotify + 1;
                            cchNotify = 0;
                        }

                        pnodeNew = pNodeStay;

                        ptpIns = ptpIns->NextTreePos();

                        cInclStay--;
                    }
                    else
                    {
                    
                        // Insert a new node into the tree, parented to an element that
                        // is not being inserted
                        pnodeNew = new CTreeNode(pNodeStay, ptp->Branch()->Element());
                        if (!pnodeNew)
                            goto OutOfMemory;

                        ptpNew = pnodeNew->InitBeginPos(FALSE);
                
                        hr = THR(_pMarkupTarget->Insert(ptpNew, ptpIns, TRUE));
                        if (hr)
                            goto Cleanup;

                        pnodeNew->PrivateEnterTree();

                        cchNotify++;
                    }

                    pnodeCur = pnodeNew;
                    aryNodeStack.Append( pnodeCur );

                    // Reparent the node on the left
                    ptp->Branch()->SetParent( pNodeCurL );
                    pNodeCurL = ptp->Branch();
                }

                Assert( pNodeStay == pNodeNotify );

                //
                // Finally, if we skipped over something, partition pointers
                //
                if( prec->_fSkip )
                {
                    CTreePosGap tpgAdjust( ptpIns, TPG_LEFT );
                    tpgAdjust.PartitionPointers(_pMarkupTarget, _fDOMOperation);
                    ptpIns = tpgAdjust.AdjacentTreePos( TPG_RIGHT );
                }
            }
            break;

        case CTreePos::Text:

            // BUGBUG: do we want to merge with pervious/next text pos here?
            ptpNew = _pMarkupTarget->NewTextPos(prec->_cch, prec->_sid, prec->_lTextID);
            if (!ptpNew)
                goto OutOfMemory;

            hr = THR(_pMarkupTarget->Insert(ptpNew, ptpIns, TRUE));
            if (hr) 
                goto Cleanup;

            cchNotify += prec->_cch;
            
            break;

        case CTreePos::Pointer:
            {
                CTreePosGap tpg(ptpIns, TPG_LEFT);

                //
                // BUGBUG - Would be nice to not have to force an embedding here
                //

                hr = THR( prec->_pPointer->MoveToGap( & tpg, _pMarkupTarget, TRUE ) );
                if (hr)
                    goto Cleanup;

                Assert(ptpIns->PreviousTreePos()->MarkupPointer() == prec->_pPointer);
            }
            break;
        }
    }

    // Flush any left over notifications
    if( cchNotify )
        SendTextAddedNotification( _pMarkupTarget, pNodeNotify , cpNotify, cchNotify);
    WHEN_DBG( cpNotify += cchNotify );

    if( cElementsNotify )
        SendElementAddedNotification( _pMarkupTarget, siNotify, cElementsNotify );

    // 3. Merge text on the left and right (keeping L and R clear of the action),
    //    and unposition tree pointer at right

    // ptpIns is at the right.  So try to merge with whatever is before it
    _pMarkupTarget->MergeText(ptpIns->PreviousTreePos());

    // _ptpTarget is at the left.  Removing it will merge as
    Assert( ! _pMarkupTarget->HasUnembeddedPointers() );
    Assert( _ptpTarget );
    hr = THR(_pMarkupTarget->RemovePointerPos( _ptpTarget, NULL, NULL ) );
    if (hr)
        goto Cleanup;

    // 4. Insert recorded text
    {
        CTxtPtr tx(_pMarkupTarget, cpTarget);
        const TCHAR *pch;
        LONG cch, cchNodeRemove;

        GetTextRecorded(&cchNodeRemove, &pch, &cch);

#if DBG==1
        CTxtPtr tpDbg( _pMarkupTarget, cpTarget );
        for( int cchDbg = cchNodeRemove; cchDbg; cchDbg-- )
        {
            Assert( tpDbg.GetChar() == WCH_NODE );
            tpDbg.NextChar();
        }
#endif
        tx.DeleteRange( cchNodeRemove );

        if (tx.InsertRange(cch, pch) != cch)
            goto OutOfMemory;

        Assert( cpTarget + cch == cpNotify );
        _InsertUndo.SetData( cpTarget, cpTarget + cch );
    }

    // 5. Send ENTERTREE notifications, top town
    
    for (prec = FirstRecord(); prec; prec = NextRecord())
    {
        if (    prec->_type == CTreePos::NodeBeg
            &&  ! prec->_fSkip )
        {
            CNotification nf;
            nf.ElementEntertree(prec->_pel);
            nf.SetData( (_pnodeSourceL == _pnodeSourceR && _fRemove) ? ENTERTREE_MOVE : 0 );
            prec->_pel->Notify(&nf);
        }

    }
    
    _pMarkupTarget->UpdateMarkupTreeVersion();

    _InsertUndo.CreateAndSubmit();

Cleanup:

    Assert( _pMarkupTarget->IsNodeValid() );

    RRETURN(hr);

OutOfMemory:

    hr = E_OUTOFMEMORY;
    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Helper functions for sending notifications in RemoveSplice
//
//----------------------------------------------------------------------------

static void 
SendTextDeletedNotification( CMarkup * pMarkup, CTreeNode * pNode, long cp, long cch )
{
    CNotification nfText;
    
    nfText.CharsDeleted( cp, cch, pNode );
    WHEN_DBG( nfText._fNoTextValidate = TRUE );
    
    pMarkup->Notify( nfText );
}

static void 
SendElementDeletedNotification( CMarkup * pMarkup, long si, long cel )
{
    CNotification nfElements;
    
    nfElements.ElementsDeleted( si, cel );
    WHEN_DBG( nfElements._fNoElementsValidate = TRUE );
    
    pMarkup->Notify( nfElements );
}

//+---------------------------------------------------------------------------
//
//  Method:     CSpliceTreeEngine::RemoveSplice
//
//  Synposis:   Remove all
//
// BUGBUG: There might be an issue node caches not being cleared properly.
//
//----------------------------------------------------------------------------
HRESULT
CSpliceTreeEngine::RemoveSplice()
{
    HRESULT             hr = S_OK;
    long                cchNodeReinsert = 0, cchRemove = 0;
    long                cp = _ptpSourceR->GetCp() + _ptpSourceR->GetCch();
    long                cElementsExitTotal = 0;
    CStackPtrAry<CElement *, 32> aryDelayRelease(Mt(CSpliceTreeEngine_RemoveSplice_aryDelayRelease_pv));
    CTreePos *          ptpOutsideL;
    CMarkup::CLock      MarkupLock( _pMarkupSource );

    if (!_fRemove || _ptpSourceL == _ptpSourceR->NextTreePos())
        goto Cleanup;

    // 0. Get rid of pointers with cling on the edges.  We do this so
    //    that _ptpSourceL/R can be repositioned on non pointer
    //    poses.  We do this so elements can muck with selection
    //    in their exit tree notification.
    while (_ptpSourceL->IsPointer())
    {
        if( _ptpSourceL == _ptpSourceR )
            break;

        if (_ptpSourceL->Cling())
        {
            CTreePos * ptp = _ptpSourceL->NextTreePos();

            Verify( ! _pMarkupSource->Remove( _ptpSourceL ) );

            _ptpSourceL = ptp;
        }
        else
        {
            _ptpSourceL = _ptpSourceL->NextTreePos();
        }
    }

    while (_ptpSourceR->IsPointer())
    {
        if( _ptpSourceL == _ptpSourceR )
            break;

        if (_ptpSourceR->Cling())
        {
            CTreePos * ptp = _ptpSourceR->PreviousTreePos();

            Verify( ! _pMarkupSource->Remove( _ptpSourceR ) );

            _ptpSourceR = ptp;
        }
        else
        {
            _ptpSourceR = _ptpSourceR->PreviousTreePos();
        }
    }

    // 1. Nondestructive: Mark all elements leaving the tree as about 
    //    to leave.  Also send ElementsDeleted to the markup

    if (_ptpSourceR->NextTreePos() != _ptpSourceL)
    {
        CTreeNode * pNodeLimit = _pnodeSourceR;
        CElement *  pelLimit = _pnodeSourceR->Element();
        CElement *  pel;
        CTreePos *  ptp;

        // First go through and set the _fExitTreePending bit on
        // all of the elements that are leaving the tree.  Also
        // send ElementsDeleted notifications to the markup.
        long cElements = 0;

        for (ptp = _ptpSourceR; ; ptp = ptp->PreviousTreePos())
        {
            if (!ptp->IsBeginElementScope())
                goto Previous1;
            
            pel = ptp->Branch()->Element();

            if (pel == pelLimit)
            {
                pNodeLimit = pNodeLimit->Parent();
                pelLimit = pNodeLimit->Element();

                if( cElements )
                {
                    SendElementDeletedNotification( _pMarkupSource, ptp->SourceIndex() + 1, cElements );
                    cElements = 0;
                }

                goto Previous1;
            }
        
            Assert( ! pel->_fExittreePending );
            pel->_fExittreePending = TRUE;
            cElements++;
            cElementsExitTotal++;
            
        Previous1:
            if (ptp == _ptpSourceL)
                break;
        }

        // finish up any elements left
        if( cElements )
        {
            SendElementDeletedNotification( 
                _pMarkupSource, 
                ptp->SourceIndex() + ptp->GetCElements(), 
                cElements );
        }
    }

    // 2. Nondestructive: Actually send the exit tree notification to
    //    the elements that are leaving the markup
    if( cElementsExitTotal )
    {
        CTreeNode * pNodeLimit = _pnodeSourceR;
        CElement *  pelLimit = _pnodeSourceR->Element();
        CElement *  pel;
        CTreePos *  ptp;

        // Now actually send the notification
        for (ptp = _ptpSourceR; ; ptp = ptp->PreviousTreePos())
        {
            if (!ptp->IsBeginElementScope())
                goto Previous2;
            
            pel = ptp->Branch()->Element();

            if (pel == pelLimit)
            {
                pNodeLimit = pNodeLimit->Parent();
                pelLimit = pNodeLimit->Element();
                goto Previous2;
            }
        
            Assert( pel->_fExittreePending );

            {
                CNotification   nf;
                DWORD           dwData = 0;

                if( _pnodeSourceL == _pnodeSourceR && _fInsert )
                    dwData |= EXITTREE_MOVE;

                if( pel->GetObjectRefs() == 1 )
                {
                    dwData |= EXITTREE_PASSIVATEPENDING;
                    Assert( !pel->_fPassivatePending );
                    WHEN_DBG( pel->_fPassivatePending = TRUE );
                }

                nf.ElementExittree1(pel);

                Assert( nf.IsSecondChanceAvailable() );

                nf.SetData( dwData );
                pel->Notify(&nf);

                //
                // During exit tree, someone may have repositioned pointers within the
                // markup.  THey may not be embedded.  Do so now.
                //

                if (_pMarkupSource->HasUnembeddedPointers())
                {
                    hr = THR( _pMarkupSource->EmbedPointers() );

                    if (hr)
                        goto Cleanup;
                }

                dwData = nf.DataAsDWORD();

                if (nf.IsSecondChanceRequested() || (dwData & EXITTREE_DELAYRELEASENEEDED))
                {
                    CElement * pElementStore = pel;

                    WHEN_DBG( pel->_fDelayRelease = TRUE );

                    pel->AddRef();

                    if( nf.IsSecondChanceRequested() )
                        pElementStore = (CElement*)((DWORD_PTR)pElementStore | 0x1);

                    hr = THR( aryDelayRelease.Append( pElementStore ) );

                    if (hr)
                        goto Cleanup;
                }
                WHEN_DBG( else pel->_fDelayRelease = FALSE );
            }

        Previous2:
            if (ptp == _ptpSourceL)
                break;
        }
    }

    // 3. Destructive: remove all text and elements that are completely in the area
    //    being removed.
    {
        CTreePos *  ptp;
        long        cchRemoveNotify = 0;
        CTreePos *  ptpRemoveR = NULL;
        CTreePos *  ptpLast = NULL, * ptpPrev;
        CTreeNode * pNodeNotify = _pnodeSourceR;
        CTreeNode * pNodeCurrStay = _pnodeSourceR;

        long        cInclTotal = 0;
        long        cInclKeep = 0;
        BOOL        fInclEdgeKeep = TRUE;

        ptpOutsideL = _ptpSourceL->PreviousTreePos();

        for ( ptp = _ptpSourceR, ptpPrev = ptp->PreviousTreePos(); 
              ptp != ptpOutsideL; 
              ptpLast = ptp, ptp = ptpPrev )
        {
            ptpPrev = ptp->PreviousTreePos();

            switch( ptp->Type() )
            {
            case CTreePos::NodeBeg:
                {
                    // If we are not an edge, we are in an inclusion
                    if (!ptp->IsEdgeScope())
                        cInclTotal++;

                    // If we are an edge, we can't be in an inclusion
                    Assert( ptp->IsEdgeScope() == !cInclTotal );

                    CTreeNode * pNode = ptp->Branch();
                    CElement *  pElement = pNode->Element();

                    cchRemove++;

                    // This node is really going away...remove
                    // this tree pos and tell the element/node
                    if (pElement->_fExittreePending)
                    {
                        Assert( pNode != pNodeCurrStay );
                        Assert( pNode != pNodeNotify );

                        if (!ptpRemoveR)
                            ptpRemoveR = ptp;

                        _pMarkupSource->Remove( ptp, ptpRemoveR );
                        ptp = NULL;

                        pNode->PrivateExitTree();

                        Assert( ! _pMarkupSource->HasUnembeddedPointers() );

                        if (!cInclTotal)
                        {
#if DBG == 1
                            // If we are delay release, then we want
                            // to clear the passivate pending here to protect
                            // against the assert check in PrivateExitTree
                            BOOL fDelayRelease = pElement->_fDelayRelease;
                            BOOL fPassivatePending = pElement->_fPassivatePending;

                            if (fDelayRelease)
                                pElement->_fPassivatePending = FALSE;
#endif

                            pElement->_fExittreePending = FALSE;

                            pElement->__pNodeFirstBranch = NULL;
                            pElement->DelMarkupPtr();
                            pElement->PrivateExitTree(_pMarkupSource);
                            
                            Assert( ! _pMarkupSource->HasUnembeddedPointers() );

#if DBG == 1
                            // Restore the passivate pending debug flag
                            if (fDelayRelease)
                                pElement->_fPassivatePending = fPassivatePending;
#endif
                        }

                        cchRemoveNotify++;
                    }
                    else
                    // The node is staying, so just remove everything accumulated
                    // and send the text notification.
                    {
                        Assert( pNode == pNodeCurrStay );
                        pNodeCurrStay = pNode->Parent();
                        Assert( !pNodeCurrStay->Element()->_fExittreePending );

                        if (ptpRemoveR)
                            _pMarkupSource->Remove( ptpLast, ptpRemoveR );

                        // Send any needed text notifications
                        if (cchRemoveNotify)
                        {
                            Assert( cp == ptp->GetCp() + 1 );

                            // BUGBUG: we can probably get rid of this notification if we
                            // know that we are in an inclusion and the inclusion is going
                            // to be closed
                            SendTextDeletedNotification( 
                                _pMarkupSource, pNodeNotify, cp, cchRemoveNotify);

                            cchRemoveNotify = 0;
                        }

                        cchNodeReinsert++;

                        if (!ptp->IsEdgeScope())
                            cInclKeep++;
                        else
                            pNodeNotify = pNodeCurrStay;
                    }

                    ptpRemoveR = NULL;
                }

                cp--;

                break;

            case CTreePos::NodeEnd:
                {
                    CTreeNode * pNode = ptp->Branch();
                    CElement *  pElement = pNode->Element();
                    BOOL        fAtEdge = ptp->IsEdgeScope();

                    // If we are a non edge, we must be in an inclusion
                    Assert( fAtEdge || cInclTotal );

                    cchRemove++;

                    if( pElement->_fExittreePending )
                    {
                        // mark this node pos to be removed
                        Assert( pNode != pNodeCurrStay );
                        Assert( pNode != pNodeNotify );

                        if (!ptpRemoveR)
                            ptpRemoveR = ptp;

                        cchRemoveNotify++;
                    }
                    else
                    {
                        // Remove anything marked to be removed
                        if (ptpRemoveR)
                            _pMarkupSource->Remove( ptpLast, ptpRemoveR );

                        ptpRemoveR = NULL;

                        pNode->SetParent( pNodeCurrStay );
                        pNodeCurrStay = pNode;

                        cchNodeReinsert++;

                        // If we are the last kept non edge
                        // of an inclusion in which the edge was removed
                        // we have to go back and cleanup the inclusion
                        if (!fAtEdge && cInclKeep == 1 && !fInclEdgeKeep)
                        {
                            CTreePos *  ptpIncl = ptp;
                            CTreePosGap tpgIncl;
                            long        cchInclRemoved;

                            do
                            {
                                Assert( ptpIncl->IsEndNode() && !ptpIncl->IsEdgeScope() );
                                ptpIncl = ptpIncl->NextTreePos();
                            } while (ptpIncl->IsEndNode());

                            tpgIncl.MoveTo( ptpIncl, TPG_LEFT );

                            Verify( ! _pMarkupSource->CloseInclusion( &tpgIncl, &cchInclRemoved ) );

                            // These pointers should no longer be used
                            ptp = NULL;
                            _ptpSourceR = NULL;

                            cchNodeReinsert -= cchInclRemoved;
                            Assert( cchNodeReinsert >= 0 );
                            cchRemoveNotify += cchInclRemoved;

                            // We are now exiting the inclusion and 
                            // pNodeNotify should be back in sync with pNodeCurrStay
                            Assert( pNodeNotify == pNodeCurrStay );
                        }
                        // Send any needed text notifications if we didn't remove the inclusion
                        else if (cchRemoveNotify)
                        {
                            Assert( cp == ptp->GetCp() + 1 );
                            SendTextDeletedNotification(
                                _pMarkupSource, pNodeNotify, cp, cchRemoveNotify);

                            cchRemoveNotify = 0;
                        }
                    }

                    // Fix up the parent chains on the left side of 
                    // the inclusion if there is any so that 
                    // notifications go to the right place.
                    if (fAtEdge)
                    {
                        if (cInclKeep)
                        {
                            CTreePos *  ptpFixup;
                            CTreeNode * pNodeReparent = pNodeCurrStay, *pNodeFixup;
                            long        cInclFixup;
                        
                            for( cInclFixup = cInclKeep, ptpFixup = ptp->PreviousTreePos();
                                 cInclFixup;
                                 ptpFixup = ptpFixup->PreviousTreePos() )
                            {
                                Assert( ptpFixup->IsEndNode() && !ptpFixup->IsEdgeScope() );

                                pNodeFixup = ptpFixup->Branch();

                                if( !pNodeFixup->Element()->_fExittreePending )
                                {
                                    pNodeFixup->SetParent( pNodeReparent );
                                    pNodeReparent = pNodeFixup;

                                    cInclFixup--;
                                }
                            }

                            pNodeNotify = pNodeReparent;
                        }
                        else if( !pElement->_fExittreePending )
                            pNodeNotify = pNodeCurrStay;


                        // this flag will tell us if we have to come back
                        // and close this inclusion later
                        fInclEdgeKeep = !pElement->_fExittreePending;
                    }
                    // keep all of our inclusion data up to date
                    else
                    {
                        cInclTotal--;
                        if (!pElement->_fExittreePending)
                            cInclKeep--;
                    }
                }

                cp--;

                break;
            case CTreePos::Text:
                {
                    Assert( !cInclTotal );

                    long cch = ptp->Cch();
                
                    // Mark the text for removal

                    cp              -= cch;
                    Assert( cp == ptp->GetCp() );
                    cchRemove       += cch;
                    cchRemoveNotify += cch;

                    if ( !ptpRemoveR )
                        ptpRemoveR = ptp;
                }

                break;
            case CTreePos::Pointer:

                Assert( !cInclTotal );

                // If the pointer has cling, mark it for removal, otherwise
                // save the pointer by flushing the remove range
                if (!ptp->Cling())
                {
                    if (ptpRemoveR)
                    {
                        _pMarkupSource->Remove( ptpLast, ptpRemoveR );
                        ptpRemoveR = NULL;
                    }
                }
                else if (!ptpRemoveR)
                {
                    ptpRemoveR = ptp;
                }
                break;
            default:
                AssertSz( FALSE, "Unknown CTreePos type" );
            } // end switch
        } // end loop

        Assert( cp == ptp->GetCp() + ptp->GetCch() );

        // finish up by removing anything marked to be removed and
        // by sending any pending notifications
        if (ptpRemoveR)
        {
            _pMarkupSource->Remove( ptpLast, ptpRemoveR );
        }

        if (cchRemoveNotify)
        {
            SendTextDeletedNotification(
                _pMarkupSource, pNodeNotify, cp, cchRemoveNotify );
        }

        // May need to merge two adjacent remaining textposes
        Verify( !_pMarkupSource->MergeText(ptpOutsideL) );
    }

    // 4. Destructive: Actually muck with the text story
    _RemoveUndo.SetText( cp, cchRemove, cchNodeReinsert );

    Assert( cchRemove >= cchNodeReinsert );
    if( cchRemove > cchNodeReinsert )
    {
        CTxtPtr tx( _pMarkupSource, cp );

        tx.DeleteRange( cchRemove );

        if (cchNodeReinsert)
        {
            Verify( tx.InsertRepeatingChar( cchNodeReinsert, WCH_NODE ) == cchNodeReinsert );
        }
    }

    Assert( _pMarkupSource->IsNodeValid() );
    _pMarkupSource->UpdateMarkupTreeVersion();

    _RemoveUndo.CreateAndSubmit();

    // Do delay release/after exit tree notifications
    {
        int         cDelay = aryDelayRelease.Size();
        CElement ** ppElement = aryDelayRelease;

        for( ; cDelay ; cDelay--, ppElement++ )
        {
            CElement *  pElement = (CElement*)((DWORD_PTR)*ppElement & ~0x1);
            BOOL        fExitTreeSc = (DWORD_PTR)*ppElement & 0x1;

            // Send any needed AfterExitTree notification
            if (fExitTreeSc)
            {
                CNotification nf;

                nf.ElementExittree2( pElement );
                pElement->Notify( &nf );
            }

            // Release the element
            Assert( pElement->_fDelayRelease );
            WHEN_DBG( pElement->_fDelayRelease = FALSE );
            Assert( !pElement->_fPassivatePending || pElement->GetObjectRefs() == 1 );
            pElement->Release();
        }
    }

Cleanup:
    
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Method:     CMarkup::SpliceTree
//
//  Synposis:   Copies, moves, or removes the region of the tree from
//              ptpgStartSource to ptpgEndSource.
//
//----------------------------------------------------------------------------
HRESULT
CMarkup::SpliceTreeInternal(
    CTreePosGap *   ptpgStartSource,
    CTreePosGap *   ptpgEndSource,
    CMarkup *       pMarkupTarget  /* = NULL */,
    CTreePosGap *   ptpgTarget /* = NULL */,
    BOOL            fRemove    /* = TRUE */,
    DWORD           dwFlags    /* = NULL */)
{
    HRESULT hr;
    CSpliceTreeEngine engine( Doc() );

    Assert( ptpgStartSource && ptpgEndSource );
    Assert( ptpgStartSource->GetAttachedMarkup() == this );
    Assert( !fRemove || ptpgEndSource->GetAttachedMarkup() == this );

    Assert( ! ptpgStartSource->GetAttachedMarkup()->HasUnembeddedPointers() );
    Assert( !ptpgTarget || ! ptpgTarget->GetAttachedMarkup()->HasUnembeddedPointers() );

    Assert( ! HasUnembeddedPointers() );
    Assert( ! pMarkupTarget || ! pMarkupTarget->HasUnembeddedPointers() );

    //
    // The incoming source gaps must be logically ordered, but they may not be totally
    // ordered.  Make sure of that.
    //

    EnsureTotalOrder( ptpgStartSource, ptpgEndSource );

    hr = THR(engine.Init(this, ptpgStartSource, ptpgEndSource, pMarkupTarget, ptpgTarget, fRemove, dwFlags));
    
    if (hr)
        goto Cleanup;

    if (    engine._fInsert 
        ||  engine._fRemove && engine._RemoveUndo.AcceptingUndo() )
    {
        hr = THR(engine.RecordSplice());
        if (hr)
            goto Cleanup;
    }

    if (engine._fRemove)
    {
        hr = THR(engine.RemoveSplice());
        if (hr)
            goto Cleanup;
    }
    
    if (engine._fInsert)
    {
        hr = THR(engine.InsertSplice());
        if (hr)
            goto Cleanup;
    }

Cleanup:

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Method:     CMarkup::UndoRemoveSplice
//
//  Synposis:   Undoes the remove part of a splice operation
//
//----------------------------------------------------------------------------
HRESULT
CMarkup::UndoRemoveSplice(
    CMarkupPointer *    pmpBegin,
    CSpliceRecordList * paryRegion,
    long                cchRemove,
    TCHAR *             pchRemove,
    long                cchNodeReinsert,
    DWORD               dwFlags)
{
    HRESULT hr;
    CTreePosGap       tpgBegin;
    CSpliceTreeEngine engine( Doc() );

    Assert( pmpBegin );

    hr = THR( EmbedPointers() );
    if (hr)
        goto Cleanup;

    Verify( ! tpgBegin.MoveTo( pmpBegin->GetEmbeddedTreePos(), TPG_LEFT ) );

    hr = THR(engine.InitUndoRemove(this, & tpgBegin, paryRegion, cchRemove, pchRemove, cchNodeReinsert, dwFlags));
    if (hr)
        goto Cleanup;

    hr = THR(engine.InsertSplice());
    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN(hr);
}

//+---------------------------------------------------------------------
//
//   File:      avundo.cxx
//
//  Contents:   AttrValue Undo support
//
//  Classes:    CUndoAttrValueSimpleChange
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef NO_EDIT // {

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_AVUNDO_HXX_
#define X_AVUNDO_HXX_
#include "avundo.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

MtDefine(CUndoAttrValueSimpleChange, Undo, "CUndoAttrValueSimpleChange")
MtDefine(CUndoPropChangeNotification, Undo, "CUndoPropChangeNotification")
MtDefine(CUndoPropChangeNotificationPlaceHolder, Undo, "CUndoPropChangeNotificationPlaceHolder")
MtDefine(CMergeAttributesUndoUnit, Undo, "CMergeAttributesUndoUnit")

//+---------------------------------------------------------------------------
//
//  CUndoAttrValueSimpleChange Implementation
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CUndoAttrValueSimpleChange::Init, public
//
//  Synopsis:   Initializes the undo unit for simple AttrValue change.
//
//  Arguments:  [dispidProp]   -- Dispid of property
//              [varProp]      -- unit value of property
//              [fInlineStyle]
//              [aaType]
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CUndoAttrValueSimpleChange::Init(
    DISPID             dispid,
    VARIANT &          varProp,
    BOOL               fInlineStyle,
    CAttrValue::AATYPE aaType)
{
    HRESULT hr;

    TraceTag((tagUndo, "CUndoAttrValueSimpleChange::Init"));

    hr = THR( super::Init( dispid, &varProp ) );

    _fInlineStyle = fInlineStyle;
    _aaType = aaType;

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CUndoAttrValueSimpleChange::Do, public
//
//  Synopsis:   Performs the undo of the property change.
//
//  Arguments:  [pUndoManager] -- Pointer to Undo Manager
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CUndoAttrValueSimpleChange::PrivateDo(IOleUndoManager *pUndoManager)
{
    HRESULT       hr;
    DWORD         dwCookie = 0;
    CUnitValue    uvOldValue;
    CUnitValue    uvCurValue;
    CAttrArray *  pAttrArray;
    CElement *    pElement = DYNCAST( CElement, _pBase );

    TraceTag((tagUndo, "CUndoAttrValueSimpleChange::Do"));

    //
    // The redo unit should be put on the stack in this call to Invoke()
    // unless we need to disable it.
    //
    if (!pUndoManager)
    {
        _pBase->BlockNewUndoUnits(&dwCookie);
    }

    Assert( _varData.vt == VT_I4 );

    uvOldValue.SetRawValue( _varData.lVal );

    pAttrArray = _fInlineStyle
                 ? pElement->GetInLineStyleAttrArray()
                 : *pElement->GetAttrArray();

    Assert( pAttrArray );

    uvCurValue.SetNull();

    if ( pAttrArray )
    {
        pAttrArray->GetSimpleAt( pAttrArray->FindAAIndex(
            _dispid, CAttrValue::AA_Attribute ),
            (DWORD *)&uvCurValue );
    }

    {
        VARIANT vtProp;

        vtProp.vt = VT_I4;
        vtProp.lVal = uvCurValue.GetRawValue();

        IGNORE_HR( pElement->CreateUndoAttrValueSimpleChange(
            _dispid, vtProp, _fInlineStyle, _aaType ) );
    }

    if (uvOldValue.IsNull())
    {
        DWORD dwOldValue;

        Verify( pAttrArray->FindSimpleInt4AndDelete( _dispid, &dwOldValue ) );

        Assert( dwOldValue == (DWORD)uvCurValue.GetRawValue() );

        hr = S_OK;
    }
    else
    {
        hr = THR(CAttrArray::AddSimple( &pAttrArray, _dispid,
                                        *(DWORD*)&uvOldValue, _aaType ));
    }

    if (!pUndoManager)
    {
        _pBase->UnblockNewUndoUnits(dwCookie);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  CUndoPropChangeNotification Implementation
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CUndoPropChangeNotification::Do, public
//
//  Synopsis:   Performs the undo of the property change.
//
//  Arguments:  [pUndoManager] -- Pointer to Undo Manager
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CUndoPropChangeNotification::PrivateDo(IOleUndoManager *pUndoManager)
{
    HRESULT    hr;
    DWORD      dwCookie = 0;
    CElement * pElement = DYNCAST( CElement, _pBase );

    TraceTag((tagUndo, "CUndoPropChangeNotification::Do"));

    //
    // The redo unit should be put on the stack in this call to Invoke()
    // unless we need to disable it.
    //
    if (!pUndoManager)
    {
        _pBase->BlockNewUndoUnits(&dwCookie);
    }

    // For Redo, create the opposite object.

    hr = THR( pElement->CreateUndoPropChangeNotification( _dispid,
                                                          _dwFlags,
                                                          !_fPlaceHolder ) );

    // Fire the event, shall we?

    if (!_fPlaceHolder)
    {
        pElement->OnPropertyChange( _dispid, _dwFlags );
    }

    if (!pUndoManager)
    {
        _pBase->UnblockNewUndoUnits(dwCookie);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  CUndoPropChangeNotificationPlaceHolder Implementation
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CUndoPropChangeNotificationPlaceHolder ctor
//
//  Synopsis:   if fPost is true, it handles the ParentUndoUnit creation,
//              and (along with the dtor) CUndoPropChangeNotification posting.
//
//  Arguments:  fPost -- whether we should bother with this or not
//              pElement -- element against which we should notify upon undo
//              dispid -- dispid for notification
//              dwFlags -- flags for notification
//
//----------------------------------------------------------------------------

CUndoPropChangeNotificationPlaceHolder::CUndoPropChangeNotificationPlaceHolder(
    BOOL          fPost,
    CElement *    pElement,
    DISPID        dispid,
    DWORD         dwFlags ) : CUndoPropChangeNotification( pElement )
{
    _hr = S_FALSE;

    if (fPost && pElement->QueryCreateUndo(TRUE,FALSE))
    {
        CDoc *  pDoc = pElement->Doc();

        _pPUU = pDoc->OpenParentUnit( pDoc, IDS_UNDOGENERICTEXT );

        CUndoPropChangeNotification::Init( dispid, dwFlags, FALSE );

        IGNORE_HR( pElement->CreateUndoPropChangeNotification(
            dispid, dwFlags, FALSE ) );

        _fPost = TRUE;
    }
    else
    {
        _pPUU = NULL;
        _fPost = FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CUndoPropChangeNotificationPlaceHolder dtor
//
//  Synopsis:   if _fPost was true at construction time, we post the
//              CUndoPropChangeNotification object and close the parent unit
//
//----------------------------------------------------------------------------

CUndoPropChangeNotificationPlaceHolder::~CUndoPropChangeNotificationPlaceHolder()
{
    if (_fPost)
    {
        CElement * pElement = DYNCAST( CElement, _pBase );
        IGNORE_HR( pElement->CreateUndoPropChangeNotification( _dispid,
            _dwFlags, TRUE ) );

        pElement->Doc()->CloseParentUnit( _pPUU, _hr );
    }
}

//+---------------------------------------------------------------------------
//
//  CMergeAttributesUndo Implementation
//
//----------------------------------------------------------------------------

CMergeAttributesUndo::CMergeAttributesUndo( CElement * pElement )
    : CUndoHelper( pElement->Doc() )
{
    Assert(pElement);
    CElement::SetPtr( &_pElement, pElement );

    _pAA = NULL;
    _pAAStyle = NULL;
    _fWasNamed = FALSE;
    _fCopyId = FALSE;
    _fRedo = FALSE;
    _fClearAttr = FALSE;
    _fPassElTarget = FALSE;

    _fAcceptingUndo = CUndoHelper::AcceptingUndo();
}

CMergeAttributesUndo::~CMergeAttributesUndo()
{
    CElement::ReleasePtr( _pElement ); 
    delete _pAA;
    delete _pAAStyle;
}

IOleUndoUnit * 
CMergeAttributesUndo::CreateUnit()
{
    CMergeAttributesUndoUnit * pUU;

    Assert( _fAcceptingUndo );

    TraceTag((tagUndo, "CMergeAttributesUndo::CreateUnit"));

    pUU = new CMergeAttributesUndoUnit( _pElement );

    if (pUU)
    {
        pUU->SetData( _pAA, _pAAStyle, _fWasNamed, _fCopyId, _fRedo, _fClearAttr, _fPassElTarget );
        _pAA = NULL;
        _pAAStyle = NULL;
    }

    return pUU;
}

CAttrArray *        
CMergeAttributesUndo::GetAA()
{
    // If we are OOM, then we just don't create and AttrArray and stuff doesn't get undone
    if (!_pAA && AcceptingUndo())
        _pAA = new CAttrArray();

    return _pAA;
}

CAttrArray *        
CMergeAttributesUndo::GetAAStyle()
{
    // If we are OOM, then we just don't create and AttrArray and stuff doesn't get undone
    if (!_pAAStyle && AcceptingUndo())
        _pAAStyle = new CAttrArray();

    return _pAAStyle;
}

//+---------------------------------------------------------------------------
//
//  CMergeAttributesUndoUnit Implementation
//
//----------------------------------------------------------------------------

CMergeAttributesUndoUnit::CMergeAttributesUndoUnit(CElement * pElement)
    : CUndoUnitBase( pElement, IDS_UNDOPROPCHANGE )
{
    CElement::SetPtr( &_pElement, pElement );
    _pAA = NULL;
    _pAAStyle = NULL;
    _fWasNamed = FALSE;
    _fCopyId = FALSE;
    _fRedo = FALSE;
    _fClearAttr = FALSE;
    _fPassElTarget = FALSE;
}

CMergeAttributesUndoUnit::~CMergeAttributesUndoUnit()
{
    CElement::ReleasePtr( _pElement ); 
    delete _pAA;
    delete _pAAStyle;
}

void 
CMergeAttributesUndoUnit::SetData( 
    CAttrArray * pAA, 
    CAttrArray * pAAStyle, 
    BOOL fWasNamed, 
    BOOL fCopyId, 
    BOOL fRedo,
    BOOL fClearAttr,
    BOOL fPassElTarget )
{
    Assert( !_pAA && !_pAAStyle );

    _pAA = pAA;
    _pAAStyle = pAAStyle;
    _fWasNamed = fWasNamed;
    _fCopyId = fCopyId;
    _fRedo = fRedo;
    _fClearAttr = fClearAttr;
    _fPassElTarget = fPassElTarget;
}

HRESULT 
CMergeAttributesUndoUnit::PrivateDo(IOleUndoManager *pUndoManager)
{
    HRESULT              hr = S_OK;
    CMergeAttributesUndo Redo( _pElement );

    // If we have _pAAStyle then we must have _pAA
    Assert( !_pAAStyle || _pAA );

    // We should never have a style AA when clearing
    Assert( !_fClearAttr || !_pAAStyle );

    Redo.SetWasNamed( _pElement->_fIsNamed );
    Redo.SetCopyId( _fCopyId );
    Redo.SetClearAttr( _fClearAttr );
    Redo.SetPassElTarget( _fPassElTarget );

    if(!_fRedo)
        Redo.SetRedo();

    if (_pAA)
    {
        CAttrArray     **ppAATo = _pElement->GetAttrArray();

        if (_fClearAttr && _fRedo)
        {
            (*ppAATo)->Clear( Redo.GetAA() );
        }
        else
        {
            CElement * pelTarget = (_fRedo && _fPassElTarget) ? _pElement : NULL;

            hr = THR( _pAA->Merge( ppAATo, pelTarget, Redo.GetAA(), !_fRedo, _fCopyId ) );
            if (hr)
                goto Cleanup;
        }

        if (!_fClearAttr)
        {
            _pElement->SetEventsShouldFire();

            // If the From has is a named element then the element is probably changed.
            if (_fWasNamed != _pElement->_fIsNamed)
            {
                _pElement->_fIsNamed = _fWasNamed;
                // Inval all collections affected by a name change
                _pElement->DoElementNameChangeCollections();
            }

            if (_pAAStyle && _pAAStyle->Size())
            {
                CAttrArray **   ppInLineStyleAATo;

                ppInLineStyleAATo = _pElement->CreateStyleAttrArray(DISPID_INTERNAL_INLINESTYLEAA);
                if (!ppInLineStyleAATo)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }

                hr = THR(_pAAStyle->Merge(ppInLineStyleAATo, NULL, Redo.GetAAStyle(), !_fRedo));
                if (hr)
                    goto Cleanup;
            }
        }
    }

    hr = THR(_pElement->OnPropertyChange(DISPID_UNKNOWN, ELEMCHNG_REMEASUREINPARENT|ELEMCHNG_CLEARCACHES|ELEMCHNG_REMEASUREALLCONTENTS));
    if (hr)
        goto Cleanup;

    IGNORE_HR( Redo.CreateAndSubmit() );

Cleanup:
    RRETURN(hr);
}

#endif

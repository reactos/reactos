#include "headers.hxx"

#ifndef X_BREAKER_HXX_
#define X_BREAKER_HXX_
#include "breaker.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

//+---------------------------------------------------------------------------
//
//  Member:     LineBreakCompat
//
//  Notes:      Ported from IE4.  Simulates the presence of block break
//              chars.
//
//----------------------------------------------------------------------------

MtDefine(CLineBreakCompat_Misc, CLineBreakCompat, "Misc CLineBreakCompat arrays")

CLineBreakCompat::CLineBreakCompat ( )
  : _aryStackBS           ( Mt( CLineBreakCompat_Misc ) ),
    _aryRunEvents         ( Mt( CLineBreakCompat_Misc ) ),
    _aryBreaks            ( Mt( CLineBreakCompat_Misc ) ),
    _aryTableBreakCharIds ( Mt( CLineBreakCompat_Misc ) )
{
    _pMarkup = NULL;
    _fWantEndingBreak = FALSE;
}

HRESULT
CLineBreakCompat::Init ( CTreePos * ptp )
{
    HRESULT hr = S_OK;
    CTreeNode * pNode;
    CStackDataAry < CBlockScope, 32 > aryStackBS ( Mt( Mem ) );
    int i;

    _pMarkup = ptp->GetMarkup();

    _lMarkupContentsVersion = _pMarkup->GetMarkupContentsVersion();
            
    _aryStackBS.SetSize( 0 );
    _aryTableBreakCharIds.SetSize( 0 );

    //
    // Move back to a chunk of text
    //
    
    for ( _ptpPrevBreak = NULL, _ptpWalk = ptp ; ; _ptpWalk = _ptpWalk->PreviousTreePos() )
    {
        if (!_ptpWalk)
        {
            _ptpPrevBreak = _pMarkup->FirstTreePos();
            _ptpWalk = _ptpPrevBreak->NextTreePos();
            _ptpNextBreak = _ptpWalk;
            break;
        }
        else if (_ptpWalk->IsText() && _ptpWalk->Cch())
        {
            _ptpPrevBreak = _ptpWalk;
            _ptpNextBreak = _ptpWalk;
            break;
        }
    }

    _textInScope = TIS_NONE;
    _nNextTableBreakId = -1;
    _pElementLastBlockBreak = NULL;
    _fInTable = FALSE;
    _BreakPending.Clear();

    //
    // Prime block stack
    //

    for ( pNode = _ptpWalk->GetBranch() ; pNode ; pNode = pNode->Parent() )
    {
        CElement * pElement = pNode->Element();
        CFlowLayout * pFlowLayoutAbove = pElement->GetFlowLayout();

        if (pFlowLayoutAbove && pFlowLayoutAbove->IsElementBlockInContext( pElement ))
        {
            CBlockScope * pbsNew;
            
            hr = THR( aryStackBS.AppendIndirect( NULL, & pbsNew ) );
            
            if (hr)
                goto Cleanup;

            pbsNew->pNodeBlock = pNode;
            pbsNew->fVirgin = FALSE;
        }
    }

    for ( i = aryStackBS.Size() - 1 ; i >= 0 ; i-- )
    {
        hr = THR( _aryStackBS.AppendIndirect( & aryStackBS [ i ], NULL ) );
        
        if (hr)
            goto Cleanup;
    }

    if (aryStackBS.Size() > 0)
        _aryStackBS[ aryStackBS.Size() - 1 ].fVirgin = TRUE;

Cleanup:

    RRETURN( hr );
}

void
CLineBreakCompat::SetWantPendingBreak ( BOOL fWant )
{
    _fWantEndingBreak = fWant;
    _pMarkup = NULL;
}

HRESULT
CLineBreakCompat::QueryBreaks ( CMarkupPointer * pPointer, DWORD * pdwBreaks )
{
    HRESULT     hr;
    CTreePos *  ptp;
    long        ich;
    CTreePosGap tpg;

    ptp = pPointer->GetNormalizedReference( ich );

    //
    // Here can be no breaks between chars.
    //

    if (ptp->IsText())
    {
        if (ich > 0 && ich < ptp->Cch())
        {
            *pdwBreaks = 0;
            return S_OK;
        }

        hr = THR( tpg.MoveTo( ptp, ich == 0 ? TPG_LEFT : TPG_RIGHT ) );
    }
    else
        hr = THR( tpg.MoveTo( ptp, TPG_RIGHT ) );

    if (!hr)
        hr = THR( QueryBreaks( & tpg, pdwBreaks ) );

    RRETURN( hr );
}

HRESULT
CLineBreakCompat::QueryBreaks ( CTreePosGap * ptpg, DWORD * pdwBreaks )
{
    HRESULT    hr = S_OK;
    CTreePos * ptp;
    int        i;
    int        nAttempts;
    BOOL       fReset;

    Assert( pdwBreaks );

    *pdwBreaks = 0;

    if (!ptpg->IsValid())
        goto Cleanup;

    ptp = ptpg->AdjacentTreePos( TPG_RIGHT );

    while ( ptp->IsPointer() )
        ptp = ptp->NextTreePos();

    //
    // Can only have breaks to the left of node pos's
    //
    
    if (!ptp->IsNode())
        goto Cleanup;

    //
    // Restart the breaker as neccesary.
    //

    if (!_pMarkup || _lMarkupContentsVersion != _pMarkup->GetMarkupContentsVersion() ||
        !_ptpPrevBreak || _ptpPrevBreak->InternalCompare( ptp ) >= 0)
    {
        hr = THR( Init( ptp ) );

        if (hr)
            goto Cleanup;
    }

    //
    // Now, while ptp is greater than the end of the computed range,
    // compute the next range.  
    //
    // If we can't get in range after 5 times, then reset the
    // state machine to the ptp, and go on from there.
    //

    nAttempts = 5;
    fReset = FALSE;
    
    while ( _ptpNextBreak->InternalCompare( ptp ) < 0 )
    {
        if (!fReset && !nAttempts--)
        {
            //
            // This should get us into range
            //
            
            hr = THR( Init( ptp ) );

            if (hr)
                goto Cleanup;

            //
            // Never reset more than once.
            //
            
            fReset = TRUE;

            continue;
        }
        
        hr = THR( ComputeNextBreaks() );

        if (hr)
            goto Cleanup;
    }
    
    Assert( _ptpPrevBreak->InternalCompare( ptp ) < 0 );
    Assert( _ptpNextBreak->InternalCompare( ptp ) >= 0 );

    for ( i = _aryBreaks.Size() - 1 ; i >= 0 ; i-- )
    {
        BreakEntry * pbe = & _aryBreaks [ i ];

        if (pbe->ptp == ptp)
        {
            Assert(
                pbe->bt == BREAK_BLOCK_BREAK || 
                pbe->bt == BREAK_SITE_BREAK || 
                pbe->bt == BREAK_SITE_END );
            
            (*pdwBreaks) |= pbe->bt;
        }
    }

Cleanup:

    RRETURN( hr );
}

HRESULT
CLineBreakCompat::ComputeNextBreaks ( )
{
    HRESULT    hr = S_OK;

    Assert( _lMarkupContentsVersion == _pMarkup->GetMarkupContentsVersion() );

    Assert( _ptpNextBreak );
    Assert( _ptpPrevBreak );

    //
    // Clear out the existing breaks in the current ran1ge
    //

    _aryBreaks.SetSize( 0 );
    
    //
    // Fill _aryRunEvents with events until substantive text is found
    //

    _aryRunEvents.SetSize( 0 );
    
    while ( _ptpWalk )
    {
        long cch = -1;
        
        Assert( _lMarkupContentsVersion == _pMarkup->GetMarkupContentsVersion() );
        
        if (_ptpWalk->IsNode())
        {
            if (!_ptpWalk->IsEdgeScope() || _ptpWalk->IsEndNode())
            {
                if (_ptpWalk->Branch()->Tag() == ETAG_ROOT)
                {
                    cch = 0;            // Force HandleText
                    _ptpWalk = NULL;    // Force exit of loop
                }
                else
                {
                    CRunEvent * preNew = _aryRunEvents.Append();

                    if (!preNew)
                    {
                        hr = E_OUTOFMEMORY;
                        goto Cleanup;
                    }

                    preNew->ptp = _ptpWalk;
                    preNew->fEnd = TRUE;

                    int cIncl;
                    for ( cIncl = 0 ; ! _ptpWalk->IsEdgeScope() ; cIncl++ )
                        _ptpWalk = _ptpWalk->NextTreePos();

                    preNew->pNode = _ptpWalk->Branch();

                    if (hr)
                        goto Cleanup;

                    while ( cIncl-- )
                        _ptpWalk = _ptpWalk->NextTreePos();
                    
                    _ptpWalk = _ptpWalk->NextTreePos();
                }
            }
            else
            {
                Assert( _ptpWalk->IsBeginNode() );
                
                CRunEvent * preNew = _aryRunEvents.Append();
                
                if (!preNew)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }

                preNew->ptp = _ptpWalk;
                preNew->fEnd = FALSE;
                preNew->pNode = _ptpWalk->Branch();
                
                _ptpWalk = _ptpWalk->NextTreePos();
            }
        }
        else if (_ptpWalk->IsText())
        {
            long cch2 = 0;

            while ( ! _ptpWalk->IsNode() )
            {
                if (_ptpWalk->IsText())
                    cch2 += _ptpWalk->Cch();
                
                _ptpWalk = _ptpWalk->NextTreePos();
            }

            if (cch2 > 0)
                cch = cch2;
        }
        else
        {
            _ptpWalk = _ptpWalk->NextTreePos();
        }

        if (cch >= 0)
        {
            hr = THR( HandleText( _ptpWalk, cch ) );

            if (hr)
                goto Cleanup;

            _aryRunEvents.SetSize( 0 );

            //
            // If we generated any breaks, then we are done with this chunk!
            //

            if (_aryBreaks.Size())
                break;
        }
    }

    //
    // Establish the new range
    //

    _ptpPrevBreak = _ptpNextBreak;
    
    _ptpNextBreak =
        _aryBreaks.Size()
            ? _aryBreaks [ _aryBreaks.Size() - 1 ].ptp
            : _pMarkup->LastTreePos();

    Assert( _ptpPrevBreak && _ptpNextBreak );

Cleanup:

    RRETURN( hr );
}

HRESULT
CLineBreakCompat::SetPendingBreak ( DWORD btBreakNew, CTreePos * ptpBreakNew )
{
    HRESULT hr = S_OK;
    DWORD btBreak = BREAK_NONE;
    CTreePos * ptpBreak = NULL;

    //
    // We must put the new pending break into _BreakPending so that SetBreak
    // can fixup the iRun in _BreakPending if the tree if modified.
    //

    if (_BreakPending.IsSet())
    {
        btBreak = _BreakPending.btBreak;
        ptpBreak = _BreakPending.ptpBreak;
    }

    _BreakPending.btBreak = btBreakNew;
    _BreakPending.ptpBreak = ptpBreakNew;
    _BreakPending.nBreakId = _nNextTableBreakId++;

    if (ptpBreak)
    {
        hr = THR( SetBreak( btBreak, ptpBreak ) );

        if (hr)
            goto Cleanup;
    }

Cleanup:

    RRETURN( hr );
}

HRESULT
CLineBreakCompat::SetBreak ( DWORD btBreak, CTreePos * ptpBreak )
{
    BreakEntry * pbe = _aryBreaks.Append();

    if (!pbe)
        return E_OUTOFMEMORY;

    pbe->ptp = ptpBreak;
    pbe->bt = btBreak;

    return S_OK;
}

void
CLineBreakCompat::ClearPendingBlockBreak ( )
{
    if (_BreakPending.IsSet() && _BreakPending.btBreak == BREAK_BLOCK_BREAK)
        _BreakPending.Clear();
}

HRESULT
CLineBreakCompat::FlushPendingBreak ( )
{
    DWORD btBreak;
    CTreePos * ptpBreak;

    if (!_BreakPending.IsSet())
        return S_OK;

    btBreak = _BreakPending.btBreak;
    ptpBreak = _BreakPending.ptpBreak;

    _BreakPending.Clear();

    RRETURN( SetBreak( btBreak, ptpBreak ) );
}

void
CLineBreakCompat::RemoveScope ( CElement * pElement, CBlockScope & bsRemoved )
{
    int cbs = _aryStackBS.Size();

    Assert( cbs > 0 );

    //
    // Block tags CAN overlap, but that is rather rare.  Test for the most
    // common case where the block scope is the top one.
    //

    if (_aryStackBS [ cbs - 1 ].pNodeBlock->Element() == pElement)
    {
        bsRemoved = _aryStackBS [ cbs - 1 ];
        _aryStackBS.Delete( cbs - 1 );
        
        return;
    }

    int i;
    for ( i = _aryStackBS.Size() - 1 ; i >= 0 ; i-- )
    {
        if (_aryStackBS[ i ].pNodeBlock->Element() == pElement)
            break;
    }

    Assert( i >= 0 );

    bsRemoved = _aryStackBS[ i ];

    _aryStackBS.Delete( i );
}

static inline BOOL
BreaksLineInEmptyLi ( CElement * pElement )
{
    return pElement->HasFlag( TAGDESC_LIST );
}

static BOOL
RequiresTextSiteEndChar ( CTreeNode * pNode )
{
    return pNode->HasFlowLayout() && ! pNode->IsContainer();
}

static BOOL
InducesTextSiteBreakChar ( CTreeNode * pNode )
{
    BOOL fBreak;
    
    if (pNode->Tag() == ETAG_TABLE)
        return TRUE;

    if (!pNode->HasFlowLayout())
        return FALSE;

    if (pNode->IsContainer())
        return FALSE;

    //
    // If the first site above this one is not a text site, then no break.
    // This makes sure, for example, that table cells do not get a site break
    // char before them.
    //

    fBreak = FALSE;
    
    for ( pNode = pNode->Parent() ; pNode ; pNode = pNode->Parent() )
    {
        if (pNode->HasLayout())
        {
            fBreak = pNode->HasFlowLayout() != NULL;
            break;
        }
    }

    return fBreak;
}

HRESULT
CLineBreakCompat::HandleText ( CTreePos * ptpRunNow, long cchInRun )
{
    HRESULT hr = S_OK;

    //
    // Process all the element events.
    //

    for ( int i = 0 ; i < _aryRunEvents.Size() ; i++ )
    {
        CTreePos * ptp = _aryRunEvents[ i ].ptp;
        CTreeNode * pNode = _aryRunEvents[ i ].pNode;
        CElement * pElement = pNode->Element();
        CFlowLayout * pFlowLayout = pElement->HasFlowLayout();
        CFlowLayout * pFlowLayoutAbove = pElement->GetFlowLayout();
        ELEMENT_TAG etag = pElement->Tag();
        BOOL fEnd = _aryRunEvents[ i ].fEnd;
        BOOL fIsBlock = pFlowLayoutAbove ? pFlowLayoutAbove->IsElementBlockInContext( pElement ) : FALSE;
        BOOL fAlwaysBreak = etag == ETAG_LI;
        BOOL fBreakOnEmpty = !fAlwaysBreak && pElement->_fBreakOnEmpty;
        BOOL fBreaksLine = pElement->BreaksLine();
        BOOL fIs1DDiv = (etag == ETAG_DIV || etag == ETAG_FIELDSET) && pElement->HasLayout();

        //
        // The following controls the insertion of text site break characters
        // into the correct places.
        //
        // When certain sites come into scope, this causes site break characters
        // to be inserted into the parent text site.
        //
        // When certain text sites go out of scope, they get text site break
        // characters inserted at their end.
        //
        // If we are linebreaking for a text site which is not the ped, make
        // sure that we don't attempt to place a site break char before the
        // text site we are breaking (doing so would mean that we modified
        // stuff outside of the text site we are supposed to break).
        //
        
        if (!fEnd && InducesTextSiteBreakChar( pNode ))
        {
            CTreeNode * pNodeParent;
            CElement * pFlowLayoutParent;

            hr = THR( FlushPendingBreak() );

            if (hr)
                goto Cleanup;

            //
            // Associate the break with the element directly above
            // the table/marquee
            //

            pNodeParent = pNode->Parent();

            pFlowLayoutParent = pNodeParent->GetFlowLayoutElement();

            if (pFlowLayoutParent)
            {
                hr = THR( SetPendingBreak( BREAK_SITE_BREAK, ptp ) );

                if (hr)
                    goto Cleanup;
            }

            //
            // If the element inducing the site break is a table, we may
            // need to clear this site break char if there are no cells in
            // the table.  Here we record the id of the break char to be used
            // when the table goes out of scope.
            //

            if (etag == ETAG_TABLE)
            {
                hr = THR( _aryTableBreakCharIds.Append( _BreakPending.nBreakId ) );

                if (hr)
                    goto Cleanup;
            }
        }
        
        if (fEnd && RequiresTextSiteEndChar( pNode ))
        {
            Assert( pFlowLayout );

            //
            // Only clear a pending block break if we have not been
            // requested to save them.  If we are asked to save them,
            // then we will, for example, produce block breaks for blocks
            // at the end of text sites.
            //

            if (!_fWantEndingBreak)
                ClearPendingBlockBreak();

            hr = THR( SetPendingBreak( BREAK_SITE_END, ptp ) );

            if (hr)
                goto Cleanup;

            //
            // Text site end chars always go in.  If we do not flush here,
            // we might not get another break to push it in.
            //

            hr = THR( FlushPendingBreak() );

            if (hr)
                goto Cleanup;
            
            //
            // No text in scope just after a text site char.  However, a table
            // going out of scope, for example, will, later, cause fake text to
            // come into scope.
            //

            _textInScope = TIS_NONE;
        }

        //
        // When a table goes out of scope, we need to check to see if we should
        // clear the pending site break for the table.
        //

        if (fEnd && etag == ETAG_TABLE)
        {
            long nIds, nIdLast;
            
            nIds = _aryTableBreakCharIds.Size();
            
            if (nIds > 0)
            {
                nIdLast = (LONG)_aryTableBreakCharIds[ nIds - 1 ];
                          
                _aryTableBreakCharIds.Delete( nIds - 1 );

                if (_BreakPending.IsSet() && nIdLast == _BreakPending.nBreakId)
                    _BreakPending.Clear();
            }
        }
        
        //
        // Keep the _fInTable state up to date
        //

        if (fEnd)
        {
            if (etag == ETAG_TABLE)
            {
                _fInTable = FALSE;
            }
            else if (pFlowLayout)
            {
                CTreeNode * pNodeTemp = pNode->Parent();

                _fInTable = FALSE;
                
                for ( ; pNodeTemp ; pNodeTemp = pNodeTemp->Parent() )
                {
                    if (pNodeTemp->Tag() == ETAG_TABLE)
                    {
                        _fInTable = TRUE;
                        break;
                    }
                    else if (pNodeTemp->HasFlowLayout())
                    {
                        break;
                    }
                }
            }
        }
        else
        {
            if (etag == ETAG_TABLE)
            {
                _fInTable = TRUE;
            }
            else if (pFlowLayout)
            {
                _fInTable = FALSE;
            }
        }

        //
        // Because of flat run, some tags should not break a line.
        //

        if (pNode->HasLayout() || _fInTable)
            fBreaksLine = FALSE;

        ///////////////////////////////////////////////////////////////////////
        //
        // !!HACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKAHCK!!
        //
        // When elements are created during editing, the break on empty bit
        // is always set on them (I'm not thrilled about this).  For some
        // elements, we must always ignore the break on empty.
        //

        if (pElement->HasFlag( TAGDESC_LIST ) || pFlowLayout)
        {
            fBreakOnEmpty = FALSE;
        }

        //
        // !!HACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKAHCK!!
        //
        ///////////////////////////////////////////////////////////////////////

        if (fIsBlock && !fEnd)
        {
            CBlockScope * pbsNew;

            //
            // Before we push the new block element onto the stack, if there is
            // a parent block element which is currently non-empty and not
            // already broken, and the new element is capable of breaking a
            // line, break it.
            //
            // This takes care of HTML like <h1>A<h2>, where the nested <h2>
            // immediately breaks the current contents of the <h1>.
            //
            // Also, for HTML like <li><h1>A, the <li> will not be broken
            // immedaitely because it will be currently empty (there is no
            // text between <li> and <h1>.
            //

            if (_aryStackBS.Size() > 0)
            {
                CBlockScope & bsTop = TopScope();
                enum BREAK { NO_BREAK, PENDING_BREAK, MANDATORY_BREAK };
                BREAK fBreak = NO_BREAK;

                //
                // If we have just seen text and this element comming into
                // scope is the kind of element which breaks lines, then break
                // a line because of it.
                //

                if (fBreaksLine && _textInScope)
                {
                    fBreak = PENDING_BREAK;

                    ///////////////////////////////////////////////////////////
                    //
                    // !!HACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACK
                    //
                    // Again, because of the nasty break on empty bits in
                    // ui-generated elements, we must ignore simulated text
                    // when certain elements are nested. For example,
                    // <blockquote> and <div>.
                    //

                    if (_textInScope == TIS_FAKE &&
                        (bsTop.pNodeBlock->Tag() == ETAG_BLOCKQUOTE ||
                         bsTop.pNodeBlock->Tag() == ETAG_DIV))
                    {
                        fBreak = NO_BREAK;
                    }

                    //
                    // !!HACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACK
                    //
                    ///////////////////////////////////////////////////////////
                }

                ///////////////////////////////////////////////////////////////
                //
                // !!HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK!!
                //
                // Hack to break certain element in the context of <LI>
                //

                if (bsTop.pNodeBlock->Tag() == ETAG_LI && bsTop.fVirgin &&
                    BreaksLineInEmptyLi( pElement ))
                {
                    fBreak = MANDATORY_BREAK;
                }

                //
                // !!HACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACKHACK
                //
                ///////////////////////////////////////////////////////////////

                if (fBreak != NO_BREAK)
                {
                    hr = THR( SetPendingBreak( BREAK_BLOCK_BREAK, ptp ) );
                    
                    if (hr)
                        goto Cleanup;

                    if (fBreak == MANDATORY_BREAK)
                    {
                        hr = THR( FlushPendingBreak() );

                        if (hr)
                            goto Cleanup;
                    }

                    //
                    // Note that we do not set _pElementLastBreak to
                    // bsTop.pElementBlockhere because pElementScope is comming
                    // intpo scope, and would simply NULL it out anyway.
                    //
                }

                //
                // The parent block element is going out of scope, if it is
                // entered again we have to know if it not the first time.
                //

                bsTop.fVirgin = FALSE;
            }

            //
            // Push the new block element onto the stack.
            //

            hr = THR( _aryStackBS.AppendIndirect( NULL, & pbsNew ) );
            
            if (hr)
                goto Cleanup;

            pbsNew->pNodeBlock = pNode;
            pbsNew->fVirgin = TRUE;

            //
            // On the opening of a new block element, we must forget the last
            // element which received a block break.
            //

            _pElementLastBlockBreak = NULL;

            //
            // If the current block is marked as break on empty, then simulate
            // the presense of text, other wise we have not yet seen text after
            // this element.
            //

            _textInScope = fBreakOnEmpty ? TIS_FAKE : TIS_NONE;
        }
        else if (fIsBlock && fEnd)
        {
            //
            // Remove the scope corresponding to the element, copying the scope
            // state into bs.
            //

            CBlockScope bsEnd;

            RemoveScope( pElement, bsEnd );

            //
            // If the element going out of scope was not already broken and is
            // either non-empty or always breaks a line, the let it break a
            // line. (remember that end-tags are received in the first run they
            // don't influence after the last run they do).
            //
            // This takes care of HTML like <p>A<p> where the first <p> goes
            // out of scope, and should break.
            //
            // This also deals with empty <li>'s (the fAlwaysBreak check).
            //
            // Note that here we make sure that we break a line only the first
            // time this block is entered.
            //
            // Note here that we are associating the break char with the
            // element most recent in scope.  We have to do this because the
            // element going out of scope might not be on the top of the stack,
            // and we might split a run, and must point to the nearest element.
            //

            if (fBreaksLine && (_textInScope || (fAlwaysBreak && bsEnd.fVirgin)))
            {
                //
                // Must flush here (potentially modify the tree) before
                // inspecting the tree.
                //

                hr = THR( FlushPendingBreak() );

                if (hr)
                    goto Cleanup;

                //
                // We must not insert two breaks in a row for the same element
                //

                if (pElement != _pElementLastBlockBreak)
                {
                    hr = THR( SetPendingBreak( BREAK_BLOCK_BREAK, ptp ) );

                    if (hr)
                        goto Cleanup;
                    
                    _pElementLastBlockBreak = pElement;

                    //
                    // An 5.0 change to the measurer/renderer causes the last
                    // empty LI in a UL to have a block break when the list is
                    // in a table cell.  This was not the case for 4.0.  Thus,
                    // when an LI sets a pending block break, we force it in, so
                    // the ClearPendingBlockBreak call made when the table cell
                    // goes out of scope does not nuke the block break created
                    // by the LI.
                    //

                    if (etag == ETAG_LI)
                    {
                        hr = THR( FlushPendingBreak() );

                        if (hr)
                            goto Cleanup;
                    }
                }
            }

            //
            // If wanted, don't loose a pending break when major containers
            // go out of scope.
            //

            if (_fWantEndingBreak && (etag == ETAG_BODY || etag == ETAG_TXTSLAVE))
            {
                hr = THR( FlushPendingBreak() );

                if (hr)
                    goto Cleanup;
            }
            
            //
            // If a line breaking element just went out of scope, no text is in
            // scope
            //

            if (fBreaksLine)
                _textInScope = TIS_NONE;
        }

        //
        // Before flat run, some elements used to be embedded in the runs and
        // had an embedding character in the text, and had their own runs.
        // Now, in the non-flat world, these elements no longer have embedding
        // characters, but they still need to behave as though they did.  So,
        // here we check for these elements, and synthesize the presence of
        // text.
        //

        if (fEnd &&
            (etag == ETAG_TABLE ||
             etag == ETAG_MARQUEE ||
             etag == ETAG_SELECT ||
             etag == ETAG_BR ||
             etag == ETAG_HR ||
             etag == ETAG_OBJECT ||
             etag == ETAG_IMG ||
             etag == ETAG_INPUT ||
             etag == ETAG_BUTTON ||
             etag == ETAG_TEXTAREA ||
             fIs1DDiv) &&
            pNode->IsInlinedElement())
        {
            _textInScope = TIS_SIMU;
        }
    }

    _aryRunEvents.SetSize( 0 );

    if (cchInRun > 0)
    {
        //
        // If we find characters in a run, then we can flush any pending
        // breaks set prior to finding this text
        //

        hr = THR( FlushPendingBreak() );

        if (hr)
            goto Cleanup;

        //
        // There are real chars here, mark the block element owning these
        // chars to indicate that chars have been found in it scope.
        //

        _textInScope = TIS_REAL;

        //
        // When se see text, we forget the last element which received a block
        // break;
        //

        _pElementLastBlockBreak = NULL;
    }

Cleanup:

    RRETURN( hr );
}

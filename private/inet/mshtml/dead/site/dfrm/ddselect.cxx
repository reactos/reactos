//+------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994, 1995
//  All rights Reserved.
//  Information contained herein is Proprietary and Confidential.
//
//  File:       forms3\src\ddoc\datadoc\SELECT.CXX
//
//  Contents:   The selection code
//
//  Classes:    CAryPly
//              CArySelector
//              CRootSite
//
//  Maintained by:  LaszloG
//
//  Comments:
//      The selection code depends on the OLE-DB provider for persistent bookmarks that can
//      be used to remember selected locations even if tha associated layouts
//      and controls have been temporarily released (scrolled off the screen).
//
//-------------------------------------------------------------------------

#include "headers.hxx"
#include "dfrm.hxx"


BOOL g_fShiftSelect;


DeclareTag(tagArySelector,"src\\ddoc\\datadoc\\select.cxx","ArySelector");
DeclareTag(tagAryPly,"src\\ddoc\\datadoc\\select.cxx","AryPly");
DeclareTag(tagParentSite,"src\\ddoc\\datadoc\\select.cxx","ParentSite");

//+---------------------------------------------------------------------------
//
//  Member:     CDDSelBitAry::s_cReservedBits
//
//  Synopsis:   Holds the number of reserved bits in the selection bitarray
//
//  Comments:   Its value comes from the enum in select.hxx defining
//              the various reserved bits in the bit array
//
//----------------------------------------------------------------------------
unsigned int CDDSelBitAry::s_cReservedBits = SRB_NUM_RESERVED_BITS;










CAryPly::~CAryPly()
{
    FreePlies(NULL);
}







CArySelector::~CArySelector()
{
    FreeQualifierList(NULL);
}





#if DBG==1
SelectUnit::~SelectUnit()
{
}

QUALIFIER::~QUALIFIER()
{
}
#endif


//+---------------------------------------------------------------------------
//
//  Member:     CAryPly::FreePlies
//
//  Synopsis:   Walks down the ply list, recurses into the selector lists,
//              then deletes the ply list
//
//  Arguments:  pParent     the parent site whose _arySites list is to be indexed
//                          with the ply indices. May be NULL if the instances have
//                          been destroyed in the meantime (scrolled off the screen)
//
//  Returns:    Nothing
//
//
//  Comments:
//
//----------------------------------------------------------------------------

void CAryPly::FreePlies(CSite * pParent)
{
    CSite * pSite = NULL;
    PLY * pPly;
    PLY * pEndPlies;

    TraceTag((tagAryPly,"CAryPly::FreePlies"));

    for ( pPly = *this, pEndPlies = pPly + Size();
          pPly < pEndPlies;
          pPly++ )
    {
        delete pPly->parySelectors;
    }
    DeleteAll();
}




//+---------------------------------------------------------------------------
//
//  Member:     CArySelector::FreeQualifierList
//
//  Synopsis:   Walks down the selector list, recurses into the ply lists,
//              then deletes the ply list
//
//  Arguments:  pParent         the parent site whose _arySites list is to be indexed
//                              with the ply indices. May be NULL if the instances have
//                              been destroyed in the meantime (scrolled off the screen)
//
//  Returns:    Nothing
//
//
//  Comments:
//----------------------------------------------------------------------------

void CArySelector::FreeQualifierList(CSite * pParent)
{
    SelectUnit *  pSelector;
    SelectUnit ** ppSelector;
    SelectUnit ** ppEndSelectors;

    TraceTag((tagArySelector,"CArySelector::FreeQualifierList"));

    for ( ppSelector = *this, ppEndSelectors = ppSelector + Size();
          ppSelector < ppEndSelectors;
          ppSelector++ )
    {
        pSelector = *ppSelector;
        Assert(pSelector);
        pSelector->arySubLevels.FreePlies(NULL);
        //  Review : break this out into a disposal function --> bookmark/qualifier will become a class
        pSelector->qStart.bookmark.Passivate();
        delete pSelector->pqEnd;
        delete pSelector;
    }

    DeleteAll();
}



//+------------------------------------------------------------------------
//
//  CArySelector implementation details
//
//-------------------------------------------------------------------------



//+---------------------------------------------------------------------------
//
//  Member:     CArySelector::Append
//
//  Synopsis:   Augment a qualifier into a selector and add it to the selector list.
//
//  Arguments:  qualifier   the qualifier to be added
//
//  Returns:    S_OK if OK
//              E_OUTOFMEMORY if can't allocate
//
//
//----------------------------------------------------------------------------

//  Review: do we need this?

HRESULT CArySelector::Append(QUALIFIER qualifier)
{
    SelectUnit * psu;

    psu = new SelectUnit;
    if ( ! psu )
        RRETURN(E_OUTOFMEMORY);

    psu->qStart = qualifier;

//  return Append(psu);
    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Member:     CArySelector::Find(qualifier)
//
//  Synopsis:   Augment a qualifier into a selector and find it in the selector list.
//
//  Arguments:  qualifier   the qualifier to be found
//
//  Returns:    the selector's pointer if found
//              NULL if not
//
//
//----------------------------------------------------------------------------

SelectUnit * CArySelector::Find(QUALIFIER qualifier)
{
    SelectUnit su;

    TraceTag((tagArySelector,"CArySelector::Find(qualifier)"));

    su.pqEnd = NULL;
    su.qStart = qualifier;

    return Find(&su);
}




//+---------------------------------------------------------------------------
//
//  Member:     CArySelector::Find(pSelector)
//
//  Synopsis:   Find a selector in the selector list.
//
//  Arguments:  pSelectUnit the selector to be found
//
//  Returns:    the selector's pointer if found
//              NULL if not
//
//
//  Comments:   Right now it works only for bookmarks
//
//  Todo:       Enhance it to handle all the cases.
//----------------------------------------------------------------------------
SelectUnit * CArySelector::Find(SelectUnit * pSelectUnit)
{
    SelectUnit ** ppSelector = *this;
    int c = Size();
    SelectUnit * psu;

    TraceTag((tagArySelector,"CArySelector::Find(pSelector)"));

    Assert(pSelectUnit->qStart.type != QUALI_UNKNOWN);

    while (c--)
    {
        psu = *ppSelector++;

        Assert(psu->qStart.type != QUALI_UNKNOWN);

        // allow to match QUALI_DETAIL on the left to a bookmark...
        if (psu->qStart.type == QUALI_DETAIL && pSelectUnit->qStart.type == QUALI_BOOKMARK ||
            psu->qStart == pSelectUnit->qStart ||
            (psu->pqEnd && (*psu->pqEnd == pSelectUnit->qStart ||
            (psu->qStart < pSelectUnit->qStart && pSelectUnit->qStart < *psu->pqEnd))))
        {
            return psu;

        }
    }

    return NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CArySelector::Merge(SelectUnit *psuAnchor, SelectUnit *psuNew)
//
//  Synopsis:   Merge the new selectunit with the anchor (if at the same level)
//
//  Arguments:  psuAnchor   pointer to anchor point
//              psuNew      pointer to new selection point
//
//  Returns:    success
//
//
//  Comments:   Right now it works only for bookmarks
//
//  Todo:       Enhance it to handle all the cases.
//----------------------------------------------------------------------------
HRESULT
CArySelector::Merge(SelectUnit *psuAnchor, SelectUnit *psuNew)
{
    HRESULT hr;

    // BUGBUG we rely on that all selection is cleared now, we'll try real merge later
    FreeQualifierList(NULL);

    Assert(NULL == psuNew->pqEnd);
    Assert(NULL == psuAnchor->pqEnd);

    //  We can only set up bookmark ranges now.
    //  Ordinal and primary key ranges might follow
    //  if template was selected type is QUALI_DETAIL
    if ( psuAnchor->qStart.type != QUALI_DETAIL &&
        ((psuAnchor->qStart.type != psuNew->qStart.type) ||
         (psuAnchor->qStart.type != QUALI_BOOKMARK)))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    psuNew->pqEnd = new QUALIFIER;
    if (!psuNew->pqEnd)
        goto MemoryError;

    if (psuAnchor->qStart.type == QUALI_DETAIL)
    {
        *psuNew->pqEnd = psuNew->qStart;
    }
    else if (psuAnchor->qStart < psuNew->qStart)
    {
        *psuNew->pqEnd = psuNew->qStart;
        psuNew->qStart = psuAnchor->qStart;
    }
    else
    {
        *psuNew->pqEnd = psuAnchor->qStart;
    }

    // Add the new selection
    hr = Append(psuNew);

Cleanup:

    RRETURN(hr);

MemoryError:

    hr = E_OUTOFMEMORY;
    goto Cleanup;
}



//+---------------------------------------------------------------------------
//
//  Member:     CArySelector::Split
//
//  Synopsis:   Split the selector around the incoming bookmark/qualifier
//
//  Arguments:  psuSplitter pointer to the splitter qualifier/selector
//              psuOld      pointer to the selector to be split
//
//  Returns:    success
//
//
//  Comments:   Right now it works only for bookmarks
//
//  Todo:       Enhance it to handle all the cases.
//----------------------------------------------------------------------------
HRESULT
CArySelector::Split(SelectUnit *psuSplitter, SelectUnit *psuOld)
{
    HRESULT hr;

    hr = S_OK;

    Assert(psuSplitter->qStart.type == psuOld->qStart.type);



    RRETURN(hr);
}
//+---------------------------------------------------------------------------
//
//  Member:     CAryPly::Find
//
//  Synopsis:   Find a ply in the ply list based on its index
//
//  Arguments:  idx     the ply index to be found
//
//  Returns:    the ply's pointer if found
//              NULL if not
//
//
//----------------------------------------------------------------------------
PLY * CAryPly::Find(UINT idx)
{
    PLY * pPly, * pPlyEnd;

    TraceTag((tagAryPly,"CAryPly::Find(idx)"));

    if ( 0 == Size() )
        return NULL;

    for ( pPly = *this, pPlyEnd = pPly + Size();
          pPly <= pPlyEnd;
          pPly++ )
    {
        if ( pPly->idx == idx )
        {
            return pPly;
        }
    }
    return NULL;
}




//+---------------------------------------------------------------------------
//
//  Member:   Copy
//
//  Synopsis:   Copies a selector
//
//---------------------------------------------------------------------------
HRESULT
SelectUnit::Copy(SelectUnit &su)
{
    HRESULT hr = S_OK;

    qStart = su.qStart;
    if ( su.pqEnd )
    {
        pqEnd = new QUALIFIER;
        if ( ! pqEnd )
            goto MemoryError;

        *pqEnd = *su.pqEnd;
    }

    hr = THR(arySubLevels.Copy(su.arySubLevels,FALSE));
    if ( hr )
        goto Error;

    hr = THR(aryfControls.Copy(su.aryfControls));
    if ( hr )
        goto Error;

Cleanup:
    RRETURN(hr);

MemoryError:
    hr = E_OUTOFMEMORY;

Error:
    goto Cleanup;
}


//+---------------------------------------------------------------------------
//
//  Member:   operator==
//
//  Synopsis:   compare the qualifiers
//
//---------------------------------------------------------------------------
BOOL
QUALIFIER::operator==(const QUALIFIER &q) const
{
#if NEVER
    //  BUGBUG
    //  Asserts temporarily removed until I can initialize the
    //  Current selectuni correctly.

    Assert(QUALI_UNKNOWN != type);
    Assert(QUALI_UNKNOWN != q.type);

    Assert( (QUALI_UNBOUND_LAYOUT == type) ||
            (QUALI_BOOKMARK == type) ||
            (QUALI_HEADER == type) ||
            (QUALI_FOOTER == type));
    Assert( (QUALI_UNBOUND_LAYOUT == q.type) ||
            (QUALI_BOOKMARK == q.type) ||
            (QUALI_HEADER == q.type) ||
            (QUALI_FOOTER == q.type));
#endif

    return (q.type == type) && ( (type == QUALI_HEADER) ||
                                 (type == QUALI_FOOTER) ||
                                 (type == QUALI_DETAIL) ||
                                 (type == QUALI_UNBOUND_LAYOUT) ||
                                 (bookmark == q.bookmark) );
}


//+---------------------------------------------------------------------------
//
//  Member:   operator<
//
//  Synopsis:   compare the qualifiers
//
//---------------------------------------------------------------------------
BOOL
QUALIFIER::operator<(const QUALIFIER &q) const
{
#if NEVER
    //  BUGBUG
    //  Asserts temporarily removed until I can initialize the
    //  Current selectuni correctly.
    Assert(QUALI_UNKNOWN != type);
    Assert(QUALI_UNKNOWN != q.type);

    Assert((QUALI_UNBOUND_LAYOUT == type) || (QUALI_BOOKMARK == type));
    Assert((QUALI_UNBOUND_LAYOUT == q.type) || (QUALI_BOOKMARK == q.type));
#endif


    return (type == q.type) && ( (type == QUALI_BOOKMARK) && (bookmark < q.bookmark) );
}


//+---------------------------------------------------------------------------
//
//  Member:   Clear
//
//  Synopsis:   Clears the qualifier
//
//---------------------------------------------------------------------------
void
QUALIFIER::Clear()
{
    switch(type)
    {
    case QUALI_BOOKMARK:
        bookmark.Passivate();
        break;
    }
    type = QUALI_UNKNOWN;
}




//+---------------------------------------------------------------------------
//
//  Member:     SelectUnit::Merge
//
//  Synopsis:   Merges the selected bits into a range
//
//  Arguments:  psuAnchor   the anchor selector
//
//  Returns: success
//
//----------------------------------------------------------------------------

HRESULT
SelectUnit::Merge(SelectUnit *psuAnchor)
{
    if (aryfControls.GetReservedBit(SRB_LAYOUTFRAME_SELECTED) ||
        psuAnchor->aryfControls.GetReservedBit(SRB_LAYOUTFRAME_SELECTED))
    {
        aryfControls.Clear();
        aryfControls.SetReservedBit(SRB_LAYOUTFRAME_SELECTED);
    }
    else
    {
        // this could potentially ruin the reserved bits !
        aryfControls.Merge(psuAnchor->aryfControls);
    }

    return S_OK;
}





//+---------------------------------------------------------------------------
//
//  Member:     SelectUnit::Clear
//
//  Synopsis:   Clears/initializes the selector
//
//  Arguments:  psuAnchor   the anchor selector
//
//  Returns: success
//
//----------------------------------------------------------------------------

void
SelectUnit::Clear(void)
{
    if ( qStart.type == QUALI_BOOKMARK )
    {
        qStart.bookmark.Passivate();
    }
    qStart.type = QUALI_UNKNOWN;
    delete pqEnd;
    pqEnd = NULL;
    aryfControls.Clear();
    arySubLevels.DeleteAll();
}



//+---------------------------------------------------------------------------
//
//  Member:     SelectUnit::Normalize
//
//  Synopsis:   Normalize the resulting selector: if it is a range-selector
//              and both qStart and pqEnd point to the same record,
//              delete pqEnd.
//
//----------------------------------------------------------------------------

void
SelectUnit::Normalize(void)
{
    if ( qStart == *pqEnd )
    {
        delete pqEnd;
        pqEnd = NULL;
    }
}

//
//  ****    End of file
//
///////////////////////////////////////////////////////////////////////////////


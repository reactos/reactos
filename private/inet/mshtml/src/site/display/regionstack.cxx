//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       regionstack.cxx
//
//  Contents:   Store regions associated with particular display nodes.
//
//  Classes:    CRegionStack
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_REGIONSTACK_HXX_
#define X_REGIONSTACK_HXX_
#include "regionstack.hxx"
#endif


//+---------------------------------------------------------------------------
//
//  Member:     CRegionStack::CRegionStack
//              
//  Synopsis:   Constructor that creates a new region stack clipped to the
//              given band rectangle.
//              
//  Arguments:  rgnStack        region stack to copy
//              rcBand          band rectangle to clip regions
//              
//  Notes:      
//              
//----------------------------------------------------------------------------


CRegionStack::CRegionStack(const CRegionStack& rgnStack, const CRect& rcBand)
{
    _stackIndex = 0;
    
    // copy regions that intersect the band
    for (int i = 0; i < rgnStack._stackMax; i++)
    {
        // unless the bounds of an opaque item intersects this band, we're not
        // interested
        if (rcBand.Intersects(rgnStack._stack[i]._rcBounds))
        {
            CRegion* prgn = new CRegion(*rgnStack._stack[i]._prgn);

            if (prgn == NULL)
                break;

            prgn->Intersect(rcBand);
            
            // believe it or not, both the opaque item's bounds AND the new
            // redraw region have to intersect this band in order for us
            // to put this entry in the stack
            if (prgn->IsEmpty())
            {
                delete prgn;
            }
            else
            {
                stackElement* p = &_stack[_stackIndex++];
                p->_prgn = prgn;
                p->_key = rgnStack._stack[i]._key;
            }
        }
    }
    
    _stackMax = _stackIndex;
}


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     CRegionStack::~CRegionStack
//              
//  Synopsis:   destructor
//              
//----------------------------------------------------------------------------


CRegionStack::~CRegionStack()
{
    Assert(_stackMax == 0);
}
#endif




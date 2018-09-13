//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       region.cxx
//
//  Contents:   Class to accelerate operations on rects/regions, and make
//              regions easier to deal with.
//
//  Classes:    CRegion
//
//  Notes:      The theory behind this class is that it is cheaper to store
//              and operate on rectangles when they will do the job.  So,
//              CRegion stores a rectangle and a type in addition to the HRGN
//              handle.
//
//              _type indicates the type of region, corresponding with the
//              codes returned by ::CombineRgn (RGN_ERROR, NULLREGION,
//              SIMPLEREGION, COMPLEXREGION).
//              
//  valid states:
// 
//  empty region:
//   _rc = 0 0 0 0
//   _type = NULLREGION
//   _hrgn = NULL
//   
//  rectangle:
//   _rc = any rect values
//   _type = SIMPLEREGION
//   _hrgn = NULL
//   
//  complex region:
//   _rc = bounds of region
//   _type = COMPLEXREGION
//   _hrgn = HRGN
//   
//  error:
//   _rc = 0 0 0 0
//   _type = RGN_ERROR
//   _hrgn = NULL
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_REGION_HXX_
#define X_REGION_HXX_
#include "region.hxx"
#endif

MtDefine(CRegion, DisplayTree, "CRegion")

#define NORMALIZECOMPLEXREGION()        \
    switch (_type)                      \
    {                                   \
    case RGN_ERROR:                     \
        goto Error;                     \
    case NULLREGION:                    \
        DiscardHRGN();                  \
        SetToNullRegion();              \
        break;                          \
    case SIMPLEREGION:                  \
        ::GetRgnBox(_hrgn,(RECT*)&_rc); \
        DiscardHRGN();                  \
        break;                          \
    case COMPLEXREGION:                 \
        ::GetRgnBox(_hrgn,(RECT*)&_rc); \
        break;                          \
    }
    

#define ERRORRETURN()                   \
    return;                             \
Error:                                  \
    if (_hrgn != NULL)                  \
        DiscardHRGN();                  \
    _SetInvalid();                      \
    return;

    
//+---------------------------------------------------------------------------
//
//  Member:     CopyHRGN
//              
//  Synopsis:   Copy the given HRGN into this CRegion, and normalize.
//              
//  Arguments:  hrgn    region to copy
//              
//----------------------------------------------------------------------------

void
CRegion::CopyHRGN(HRGN hrgn)
{
    Assert(hrgn != NULL);
    Assert(_hrgn == NULL);
    
    _type = ::GetRgnBox(hrgn,(RECT*)&_rc);
    
    switch (_type)
    {
    case RGN_ERROR:
        goto Error;
    case NULLREGION:
        _rc.SetRectEmpty();
        break;
    case SIMPLEREGION:
        break;
    case COMPLEXREGION:
        _hrgn = ::CreateRectRgn(0,0,0,0);
        if (_hrgn == NULL)
            goto Error;
        _type = ::CombineRgn(_hrgn, hrgn, NULL, RGN_COPY);
        if (_type != COMPLEXREGION)
            goto Error;
        break;
    }
    
    ERRORRETURN();
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::CRegion
//              
//  Synopsis:   copy constructor
//              
//  Arguments:  rgn     region to copy
//              
//----------------------------------------------------------------------------


CRegion::CRegion(const CRegion& rgn)
{
    if (rgn.IsComplex())
    {
        _hrgn = NULL;
        CopyHRGN(rgn._hrgn);
    }
    else
    {
        Assert(rgn._hrgn == NULL);
        ::memcpy(this, &rgn, sizeof(CRegion));
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::operator=
//              
//  Synopsis:   Assign a new region.
//              
//  Arguments:  rgn     region to copy
//              
//----------------------------------------------------------------------------

CRegion&
CRegion::operator=(const CRegion& rgn)
{
    if (IsComplex())
    {
        DiscardHRGN();
    }
    
    Assert(_hrgn == NULL);
    
    if (rgn.IsComplex())
    {
        CopyHRGN(rgn._hrgn);
    }
    else
    {
        Assert(rgn._hrgn == NULL);
        ::memcpy(this, &rgn, sizeof(CRegion) - sizeof(HRGN));
    }

    return *this;
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::GetRegion
//              
//  Synopsis:   Return a new HRGN containing this region.
//              
//  Arguments:  prgn    pointer to HRGN to modify
//              
//  Returns:    The type of region: RGN_ERROR, NULLREGION, SIMPLEREGION, COMPLEXREGION
//              
//----------------------------------------------------------------------------

int
CRegion::GetRegion(HRGN* prgn) const
{
    switch (_type)
    {
    case RGN_ERROR:
        *prgn = NULL;
        break;
    case NULLREGION:
    case SIMPLEREGION:
        *prgn = ::CreateRectRgnIndirect(&_rc);
        break;
    case COMPLEXREGION:
        *prgn = ::CreateRectRgn(0,0,0,0);
        if (*prgn != NULL)
        {
            if (::CombineRgn(*prgn, _hrgn, NULL, RGN_COPY) != COMPLEXREGION)
            {
                ::DeleteObject(*prgn);
                *prgn = NULL;
            }
        }
        break;
    }
    
    return (*prgn != NULL) ? _type : RGN_ERROR;
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::operator==
//              
//  Synopsis:   Equality operator.
//              
//  Arguments:  rgn     region to compare
//              
//  Returns:    TRUE if both regions are valid and are equal.
//              
//----------------------------------------------------------------------------

BOOL
CRegion::operator==(const CRegion& rgn) const
{
    if (_type == rgn._type)
    {
        switch (_type)
        {
        case RGN_ERROR:     return FALSE;
        case NULLREGION:    return TRUE;
        case SIMPLEREGION:  return _rc == rgn._rc;
        case COMPLEXREGION: return _rc == rgn._rc && ::EqualRgn(_hrgn, rgn._hrgn);
        }
    }
    
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion::IntersectComplex
//              
//  Synopsis:   Intersect this complex region with the given rect.
//              
//  Arguments:  rc      rect to intersect
//              
//  Notes:      The first part of the intersection has already been performed
//              in the inline Intersect method.
//              
//----------------------------------------------------------------------------

void
CRegion::IntersectComplex(const RECT& rc)
{
    if (_rc.IsEmpty())
    {
        DiscardHRGN();
        SetToNullRegion();    // empty rect may not be 0,0,0,0
        return;
    }
    
    HRGN rgnTemp = ::CreateRectRgnIndirect(&rc);
    if (rgnTemp == NULL)
        goto Error;
    _type = ::CombineRgn(_hrgn, _hrgn, rgnTemp, RGN_AND);
    ::DeleteObject(rgnTemp);
    NORMALIZECOMPLEXREGION();
    
    ERRORRETURN();
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::Intersect
//              
//  Synopsis:   Intersect this region with the given HRGN.
//              
//  Arguments:  hrgn        HRGN to intersect
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CRegion::Intersect(HRGN hrgn)
{
    Assert(hrgn != NULL);
    
    switch (_type)
    {
    case RGN_ERROR:
    case NULLREGION:
        break;
        
    case SIMPLEREGION:
        _hrgn = ::CreateRectRgnIndirect(&_rc);
        if (_hrgn == NULL)
            goto Error;
        // fall thru...
        
    case COMPLEXREGION:
        _type = ::CombineRgn(_hrgn, _hrgn, hrgn, RGN_AND);
        NORMALIZECOMPLEXREGION();
        break;
    }
    
    ERRORRETURN();
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::Intersect
//              
//  Synopsis:   Intersect this region with the given region.
//              
//  Arguments:  rgn     region to intersect
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CRegion::Intersect(const CRegion& rgn)
{
    switch (rgn._type)
    {
    case RGN_ERROR:
        SetInvalid();
        break;
        
    case NULLREGION:
        SetEmpty();
        break;
        
    case SIMPLEREGION:
        Intersect(rgn._rc);
        break;
        
    case COMPLEXREGION:
        if (_type >= SIMPLEREGION)
        {
            if (_rc.Intersects(rgn._rc))
            {
                Intersect(rgn._hrgn);
            }
            else
            {
                if (_type > SIMPLEREGION)
                {
                    DiscardHRGN();
                }
                SetToNullRegion();
            }
        }
        break;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::Union
//              
//  Synopsis:   Union this region with the given rect.
//              
//  Arguments:  rc      rect to union
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CRegion::Union(const RECT& rc)
{
    HRGN rgnTemp;
    CRect rcOld;
    
    if (((const CRect&)rc).IsEmpty())
        return;
    
    switch (_type)
    {
    case RGN_ERROR:
        break;
        
    case NULLREGION:
        _rc = rc;
        _type = SIMPLEREGION;
        break;
        
    case SIMPLEREGION:
        rcOld = _rc;
        _rc.Union(rc);
        if (_rc == rc || _rc == rcOld)
            return;
        _hrgn = ::CreateRectRgnIndirect(&rcOld);
        if (_hrgn == NULL)
            goto Error;
        // fall thru...
        
    case COMPLEXREGION:
        rgnTemp = ::CreateRectRgnIndirect(&rc);
        if (rgnTemp == NULL)
            goto Error;
        _type = ::CombineRgn(_hrgn, _hrgn, rgnTemp, RGN_OR);
        ::DeleteObject(rgnTemp);
        NORMALIZECOMPLEXREGION();
        break;
    }
    
    ERRORRETURN();
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::Union
//              
//  Synopsis:   Union this region with the given HRGN.
//              
//  Arguments:  hrgn        HRGN to union
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CRegion::Union(HRGN hrgn)
{
    switch (_type)
    {
    case RGN_ERROR:
        break;
        
    case NULLREGION:
        *this = hrgn;
        break;
        
    case SIMPLEREGION:
        _hrgn = ::CreateRectRgnIndirect(&_rc);
        if (_hrgn == NULL)
            goto Error;
        // fall thru...
        
    case COMPLEXREGION:
        _type = ::CombineRgn(_hrgn, _hrgn, hrgn, RGN_OR);
        NORMALIZECOMPLEXREGION();
        break;
    }
    
    ERRORRETURN();
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::Union
//              
//  Synopsis:   Union this region with the given region.
//              
//  Arguments:  rgn     region to union
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CRegion::Union(const CRegion& rgn)
{
    switch (rgn._type)
    {
    case RGN_ERROR:
        SetInvalid();
        break;
        
    case NULLREGION:
        break;
        
    case SIMPLEREGION:
        Union(rgn._rc);
        break;
        
    case COMPLEXREGION:
        Union(rgn._hrgn);
        break;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::Subtract
//              
//  Synopsis:   Subtract the given rect from this region.
//              
//  Arguments:  rc      rect to subtract
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CRegion::Subtract(const RECT& rc)
{
    HRGN rgnTemp;
    
    switch (_type)
    {
    case RGN_ERROR:
    case NULLREGION:
        break;
        
    case SIMPLEREGION:
        if (!_rc.Intersects(rc))
            break;
        if (((const CRect&)rc).Contains(_rc))
        {
            SetToNullRegion();
            break;
        }
        _hrgn = ::CreateRectRgnIndirect(&_rc);
        if (_hrgn == NULL)
            goto Error;
        // fall thru...
        
    case COMPLEXREGION:
        rgnTemp = ::CreateRectRgnIndirect(&rc);
        if (rgnTemp == NULL)
            goto Error;
        _type = ::CombineRgn(_hrgn, _hrgn, rgnTemp, RGN_DIFF);
        ::DeleteObject(rgnTemp);
        NORMALIZECOMPLEXREGION();
        break;
    }

    ERRORRETURN();
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::Subtract
//              
//  Synopsis:   Subtract the given HRGN from this region.
//              
//  Arguments:  hrgn        HRGN to subtract
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CRegion::Subtract(HRGN hrgn)
{
    Assert(hrgn != NULL);
    
    switch (_type)
    {
    case RGN_ERROR:
    case NULLREGION:
        break;
        
    case SIMPLEREGION:
        _hrgn = ::CreateRectRgnIndirect(&_rc);
        if (_hrgn == NULL)
            goto Error;
        // fall thru...
        
    case COMPLEXREGION:
        _type = ::CombineRgn(_hrgn, _hrgn, hrgn, RGN_DIFF);
        NORMALIZECOMPLEXREGION();
        break;
    }

    ERRORRETURN();
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::Subtract
//              
//  Synopsis:   Subtract the given region from this region.
//              
//  Arguments:  rgn     region to subtract
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CRegion::Subtract(const CRegion& rgn)
{
    switch (rgn._type)
    {
    case RGN_ERROR:
        SetInvalid();
        break;
        
    case NULLREGION:
        break;
        
    case SIMPLEREGION:
        Subtract(rgn._rc);
        break;
        
    case COMPLEXREGION:
        if (_type >= SIMPLEREGION && _rc.Intersects(rgn._rc))
        {
            Subtract(rgn._hrgn);
        }
        break;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::Intersects
//              
//  Synopsis:   Determine whether this region an HRGN.
//              
//  Arguments:  rgn     the query HRGN
//              
//  Returns:    TRUE if the region intersects the HRGN.
//              
//----------------------------------------------------------------------------

BOOL
CRegion::Intersects(HRGN rgn) const
{
    HRGN rgnIntersect;
    int result;
    
    Assert(rgn != NULL);
        
    switch (_type)
    {
    case SIMPLEREGION:
        return ::RectInRegion(rgn,&_rc);
        
    case COMPLEXREGION:
        rgnIntersect = ::CreateRectRgn(0,0,0,0);
        if (!rgnIntersect)
            return FALSE;
        result = ::CombineRgn(rgnIntersect, _hrgn, rgn, RGN_AND);
        ::DeleteObject(rgnIntersect);
        return (result != RGN_ERROR && result != NULLREGION);
    }
    
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::Intersects
//              
//  Synopsis:   Determine whether this region intersects another.
//              
//  Arguments:  rgn     the other region
//              
//  Returns:    TRUE if the region intersects this one.
//              
//----------------------------------------------------------------------------

BOOL
CRegion::Intersects(const CRegion& rgn) const
{
    switch (rgn._type)
    {
    case SIMPLEREGION:
        return Intersects(rgn._rc);
        
    case COMPLEXREGION:
        return Intersects(rgn._hrgn);
    }
    
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::ResultOfSubtract
//              
//  Synopsis:   This method returns a quick estimate of the type of region that
//              will be returned if the given region is subtracted from this
//              one.
//              
//  Arguments:  rgnSubtract     region to be subtracted
//              
//  Returns:    SR_RECTANGLE    if the result would be a rectangle
//              SR_REGION       if the result would be a complex region
//              SR_UNKNOWN      if the result cannot be determined without
//                              invoking the more expensive Subtract method
//                              
//  Notes:      
//              
//----------------------------------------------------------------------------

CRegion::SUBTRACTRESULT
CRegion::ResultOfSubtract(const CRegion& rgnSubtract) const
{
    if (IsComplex())
    {
        if (rgnSubtract.IsComplex())
            return SUB_UNKNOWN;
        return (rgnSubtract._rc.Contains(_rc))
            ? SUB_EMPTY
            : SUB_UNKNOWN;
    }
    
    if (rgnSubtract.IsComplex())
    {
        return (_rc.Intersects(rgnSubtract._rc))
            ? SUB_UNKNOWN
            : SUB_RECTANGLE;
    }
    
    // subtracted rectangle must contain at least two corners of this
    // rectangle to yield a rectangular result.  If it contains 4, the
    // result will be empty.
    int cContainedCorners =
        rgnSubtract._rc.CountContainedCorners(_rc);
    switch (cContainedCorners)
    {
    case -1:
        return SUB_RECTANGLE;   // rectangles do not intersect
    case 0:
        return SUB_REGION;      // produces rectangle with a hole in it
    case 1:
        return SUB_REGION;      // produces a rectangle with notch out of a corner
    case 2:
        return SUB_RECTANGLE;   // produces a smaller rectangle
    case 4:
        return SUB_EMPTY;       // rectangles completely overlap
    default:
        Assert(FALSE);          // 3 is an impossible result
    }
    return SUB_REGION;
}



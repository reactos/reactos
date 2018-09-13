//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       region.hxx
//
//  Contents:   Class to accelerate operations on rects/regions, and make
//              regions easier to deal with.
//
//----------------------------------------------------------------------------

#ifndef I_REGION_HXX_
#define I_REGION_HXX_
#pragma INCMSG("--- Beg 'region.hxx'")

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

MtExtern(CRegion)


//+---------------------------------------------------------------------------
//
//  Class:      CRegion
//
//  Synopsis:   Region/rect class.
//
//  Notes:      A region becomes invalid if a region operation fails.  Nothing
//              will affect an invalid region until SetValid() is called.
//              This is handy, because you can do a whole bunch of operations
//              on a CRegion, and then error check once at the end.
//
//----------------------------------------------------------------------------

class CRegion
{
private:
    CRect   _rc;
    LONG    _type;
    HRGN    _hrgn;
    
    // valid states:
    // 
    // empty region:
    //   _rc = 0 0 0 0
    //   _type = NULLREGION
    //   _hrgn = NULL
    //   
    // rectangle:
    //   _rc = any rect values
    //   _type = SIMPLEREGION
    //   _hrgn = NULL
    //   
    // complex region:
    //   _rc = bounds of region
    //   _type = COMPLEXREGION
    //   _hrgn = HRGN
    //   
    // error:
    //   _rc = 0 0 0 0
    //   _type = RGN_ERROR
    //   _hrgn = NULL
    
    void                    DiscardHRGN()
                                    {Assert(_hrgn != NULL);
                                    ::DeleteObject(_hrgn);
                                    _hrgn = NULL;}
    void                    SetToNullRegion()
                                    {Assert(_hrgn == NULL);
                                    _rc.SetRectEmpty();
                                    _type = NULLREGION;}
    void                    CopyRect(int left, int top, int right, int bottom)
                                    {Assert(_hrgn == NULL);
                                    _rc.SetRect(left, top, right, bottom);
                                    if (_rc.IsEmpty())
                                        SetToNullRegion();
                                    else
                                        _type = SIMPLEREGION;}
    void                    CopyRect(const RECT& rc)
                                    {Assert(_hrgn == NULL);
                                    _rc = rc;
                                    if (_rc.IsEmpty())
                                        SetToNullRegion();
                                    else
                                        _type = SIMPLEREGION;}
    void                    CopyHRGN(HRGN hrgn);
    void                    _SetInvalid()
                                    {Assert(_hrgn == NULL);
                                    ZeroMemory(this, sizeof(CRegion) - sizeof(HRGN));}
                                            
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CRegion))
    
                            // constructors
                            
                            // empty region
                            CRegion()
                                    {_hrgn = NULL;
                                    SetToNullRegion();}
                            CRegion(const RECT& rc)
                                    {_hrgn = NULL;
                                    CopyRect(rc);}
                            CRegion(const POINT& topLeft, const SIZE& size)
                                    {_hrgn = NULL;
                                    CopyRect(topLeft.x,topLeft.y,
                                        topLeft.x+size.cx,topLeft.y+size.cy);}
                            CRegion(const SIZE& size)
                                    {_hrgn = NULL;
                                    CopyRect(0,0,size.cx,size.cy);}
                            CRegion(int left, int top, int right, int bottom)
                                    {_hrgn = NULL;
                                    CopyRect(left,top,right,bottom);}
                            CRegion(HRGN rgn)
                                    {Assert(rgn != NULL);
                                    _hrgn = NULL;
                                    CopyHRGN(rgn);}
                            CRegion(const CRegion& rgn);
                            CRegion(const RECT* prc, HRGN hrgn)
                                    {Assert(prc != NULL || hrgn != NULL);
                                    _hrgn = NULL;
                                    if (hrgn) CopyHRGN(hrgn);
                                    else CopyRect(*prc);}
                            
                            ~CRegion()
                                    {if (_hrgn != NULL) ::DeleteObject(_hrgn);}

    BOOL                    IsComplex() const
                                    {return _hrgn != NULL;}
    // BUGBUG (donmarsh) -- IsRegion is a deprecated synonym for IsComplex
    BOOL                    IsRegion() const {return _hrgn != NULL;}
                                                
    BOOL                    IsValid() const
                                    {return _type != RGN_ERROR;}
    void                    SetValid() {if (_type == RGN_ERROR) _type = NULLREGION;}
    void                    SetInvalid()
                                    {if (IsComplex()) DiscardHRGN();
                                    _SetInvalid();}
    
    void                    SetEmpty()
                                    {if (IsComplex()) DiscardHRGN();
                                    SetToNullRegion();}
    BOOL                    IsEmpty() const
                                    {return _type == NULLREGION || _type == RGN_ERROR;}

    CRegion&                operator= (const RECT& rc)
                                    {if (IsComplex()) DiscardHRGN();
                                    CopyRect(rc);
                                    return *this;}
    CRegion&                operator= (HRGN rgn)
                                    {if (IsComplex()) DiscardHRGN();
                                    CopyHRGN(rgn);
                                    return *this;}
    CRegion&                operator= (const CRegion& rgn);
    
    void                    SetRegion(const SIZE& sz)
                                    {if (IsComplex()) DiscardHRGN();
                                    CopyRect(0,0,sz.cx,sz.cy);}
    void                    SetRegion(const RECT* prc, HRGN rgn)
                                    {Assert(prc != NULL || rgn != NULL);
                                    if (IsComplex()) DiscardHRGN();
                                    if (rgn) CopyHRGN(rgn);
                                    else CopyRect(*prc);}
    void                    SetRegion(int left, int top, int right, int bottom)
                                    {if (IsComplex()) DiscardHRGN();
                                    CopyRect(left,top,right,bottom);}
    int                     GetRegion(RECT* prc, HRGN* prgn) const
                                    {if (IsComplex()) return GetRegion(prgn);
                                    *prc = _rc; return _type;}
    int                     GetRegion(HRGN* prgn) const;

    CRect&                  AsRect()
                                    {Assert(!IsComplex());
                                    return _rc;}
    const CRect&            AsRect() const
                                    {Assert(!IsComplex());
                                    return _rc;}
    
    BOOL                    operator== (const CRegion& rgn) const;
        
    const CRect&            GetBounds() const
                                    {return _rc;}
    void                    GetBounds(RECT* prc) const
                                    {*prc = _rc;}

    void                    Offset(const SIZE& offset)
                                    {
                                        if (_type >= SIMPLEREGION)
                                        {
                                            _rc.OffsetRect(offset);
                                            if (_type == COMPLEXREGION)
                                                ::OffsetRgn(_hrgn, offset.cx, offset.cy);
                                        }
                                    }
    
    void                    Intersect(const RECT& rc)
                                    {_rc.IntersectRect(rc);
                                    if (_hrgn != NULL)
                                        IntersectComplex(rc);
                                    else if (_type == SIMPLEREGION && _rc.IsEmpty())
                                        SetToNullRegion();}
    void                    Intersect(HRGN hrgn);
    void                    Intersect(const CRegion& rgn);
    
    void                    Union(const RECT& rc);
    void                    Union(HRGN hrgn);
    void                    Union(const CRegion& rgn);
    
    void                    Subtract(const RECT& rc);
    void                    Subtract(HRGN hrgn);
    void                    Subtract(const CRegion& rgn);
    
    BOOL                    Contains(const POINT& pt) const
                                    {return _rc.Contains(pt) &&
                                        (_hrgn == NULL || ::PtInRegion(_hrgn, pt.x, pt.y));}
    
    BOOL                    BoundsIntersects(const RECT& rc) const
                                    {return _rc.Intersects(rc);}
    BOOL                    BoundsIntersects(HRGN hrgn) const
                                    {Assert(hrgn != NULL);
                                    RECT rc;
                                    ::GetRgnBox(hrgn, &rc);
                                    return _rc.Intersects(rc);}
    BOOL                    BoundsIntersects(const CRegion& rgn) const
                                    {return _rc.Intersects(rgn._rc);}
    
    BOOL                    BoundsInside(const RECT& rc) const
                                    {return ((CRect&)rc).Contains(_rc);}
    
    BOOL                    Intersects(const RECT& rc) const
                                    {return _rc.Intersects(rc);}
    BOOL                    Intersects(HRGN rgn) const;
    BOOL                    Intersects(const CRegion& rgn) const;

                            // ResultOfSubtract provides a quick estimate of
                            // the kind of region that will result from
                            // subtracting the given region from this one.
                            // SUB_UNKNOWN is returned if we have two complex
                            // regions, and the result can only be discovered
                            // through the more expensive Subtract method.
    enum SUBTRACTRESULT {
        SUB_EMPTY,
        SUB_RECTANGLE,
        SUB_REGION,
        SUB_UNKNOWN
    };
    SUBTRACTRESULT          ResultOfSubtract(const CRegion& rgn) const;
    
                            // WARNING! This method returns an alias to the
                            // HRGN in this CRegion.  The caller
                            // should not modify this HRGN, or hold onto it
                            // after this region is modified or deleted.
    HRGN                    GetRegionAlias() const
                                    {return _hrgn;}
    
                            // WARNING: assumes that we have a valid region
    HRGN                    OrphanRegion()
                                    {HRGN rgnTmp = _hrgn;
                                    _hrgn = NULL;
                                    SetToNullRegion();
                                    return rgnTmp;}
        
private:
    void                    IntersectComplex(const RECT& rc);
    
                            // prevent unchecked casts
                            operator RECT();
                            operator RECT() const;
                            operator RECT&();
                            operator RECT&() const;
                            operator CRect();
                            operator CRect() const;
                            operator CRect&();
                            operator CRect&() const;
    
};


#pragma INCMSG("--- End 'region.hxx'")
#else
#pragma INCMSG("*** Dup 'region.hxx'")
#endif

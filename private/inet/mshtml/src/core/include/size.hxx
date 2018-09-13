//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       size.hxx
//
//  Contents:   An extent, derived from Windows SIZE structure.
//
//----------------------------------------------------------------------------

#ifndef I_SIZE_HXX_
#define I_SIZE_HXX_
#pragma INCMSG("--- Beg 'size.hxx'")

class CPoint;

//+---------------------------------------------------------------------------
//
//  Class:      CSize
//
//  Synopsis:   Size class (same as struct SIZE, but with handy methods). 
//
//----------------------------------------------------------------------------

// type of coordinates used in SIZE structure (must match SIZE definition)
typedef LONG SIZECOORD;

class CSize :
    public SIZE
{
public:
                            CSize() {}
                            CSize(SIZECOORD x, SIZECOORD y) 
                                    {cx = x; cy = y;}
                            CSize(const SIZE& s) {cx = s.cx; cy = s.cy;}
                            // this could be dangerous... use explicit cast
                            //CSize(const POINT& p) {cx = p.x; cy = p.y;}
                            ~CSize() {}
    
    // AsPoint() returns this size as a point, because occasionally
    // you want to use a size to construct a CRect or something like that.
    CPoint&                 AsPoint() {return (CPoint&) *this;}
    const CPoint&           AsPoint() const {return (const CPoint&) *this;}
    
    void                    SetSize(SIZECOORD x, SIZECOORD y)
                                    {cx = x; cy = y;}
    BOOL                    IsZero() const
                                    {return cx == 0 && cy == 0;}
    
    CSize                   operator - () const
                                    {return CSize(-cx,-cy);}

    CSize&                  operator += (const SIZE& s)
                                    {cx += s.cx; cy += s.cy; return *this;}
    CSize&                  operator -= (const SIZE& s)
                                    {cx -= s.cx; cy -= s.cy; return *this;}
    CSize&                  operator *= (const SIZE& s)
                                    {cx *= s.cx; cy *= s.cy; return *this;}
    CSize&                  operator /= (const SIZE& s)
                                    {cx /= s.cx; cy /= s.cy; return *this;}

    BOOL                    operator == (const SIZE& s) const
                                    {return (cx == s.cx && cy == s.cy);}
    BOOL                    operator != (const SIZE& s) const
                                    {return (cx != s.cx || cy != s.cy);}
                                    
    SIZECOORD&              operator [] (int index)
                                    {return (index == 0) ? cx : cy;}
    const SIZECOORD&        operator [] (int index) const
                                    {return (index == 0) ? cx : cy;}
    
    void                    Min(const SIZE& s)
                                    {if (cx > s.cx) cx = s.cx;
                                     if (cy > s.cy) cy = s.cy;}
    void                    Max(const SIZE& s)
                                    {if (cx < s.cx) cx = s.cx;
                                     if (cy < s.cy) cy = s.cy;}
};

inline CSize operator+(const SIZE& a, const SIZE& b)
        {return CSize(a.cx+b.cx, a.cy+b.cy);}
inline CSize operator-(const SIZE& a, const SIZE& b)
        {return CSize(a.cx-b.cx, a.cy-b.cy);}
inline CSize operator*(const SIZE& a, const SIZE& b)
        {return CSize(a.cx*b.cx, a.cy*b.cy);}
inline CSize operator/(const SIZE& a, const SIZE& b)
        {return CSize(a.cx/b.cx, a.cy/b.cy);}

#pragma INCMSG("--- End 'size.hxx'")
#else
#pragma INCMSG("*** Dup 'size.hxx'")
#endif


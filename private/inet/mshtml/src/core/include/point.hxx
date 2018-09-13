//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       point.hxx
//
//  Contents:   Class to make points easier to deal with.
//
//----------------------------------------------------------------------------

#ifndef I_POINT_HXX_
#define I_POINT_HXX_
#pragma INCMSG("--- Beg 'point.hxx'")

#ifndef X_SIZE_HXX_
#define X_SIZE_HXX_
#include "size.hxx"
#endif


//+---------------------------------------------------------------------------
//
//  Class:      CPoint
//
//  Synopsis:   Point class (same as struct POINT, but with handy methods). 
//
//  Note:       To be perfectly correct, some of the arithmetic operators
//              should return CSize instead of CPoint.  However, being strict
//              about this doesn't lead to huge benefits, so we're a little
//              sloppy in our distinction between CSize and CPoint.
//
//----------------------------------------------------------------------------

// type of coordinates used in POINT structure (must match POINT definition)
typedef LONG POINTCOORD;

class CPoint
        : public POINT
{
public:
                            CPoint() {}
                            CPoint(POINTCOORD x_, POINTCOORD y_) 
                                    {x = x_; y = y_;}
                            CPoint(const POINT& p) {x = p.x; y = p.y;}
                            // this could be dangerous... do an explicit cast
                            //CPoint(const SIZE& s) {x = s.cx; y = s.cy;}
                            ~CPoint() {}
    
                            
    // AsSize() returns this point as a vector (CSize), because occasionally
    // you want to use a point to do OffsetRect() or something like that.
    CSize&                  AsSize() {return (CSize&) *this;}
    const CSize&            AsSize() const {return (const CSize&) *this;}
    
    
    CPoint                  operator - () const
                                    {return CPoint(-x,-y);}

    // NOTE: these operators take SIZE instead of POINT to keep the client
    // clear about what is happening
    CPoint&                 operator += (const SIZE& s)
                                    {x += s.cx; y += s.cy; return *this;}
    CPoint&                 operator -= (const SIZE& s)
                                    {x -= s.cx; y -= s.cy; return *this;}
    CPoint&                 operator *= (const SIZE& s)
                                    {x *= s.cx; y *= s.cy; return *this;}
    CPoint&                 operator /= (const SIZE& s)
                                    {x /= s.cx; y /= s.cy; return *this;}

    BOOL                    operator == (const POINT& p) const
                                    {return (x == p.x && y == p.y);}
    BOOL                    operator != (const POINT& p) const
                                    {return (x != p.x || y != p.y);}
                                    
    POINTCOORD&             operator [] (int index)
                                    {return (index == 0) ? x : y;}
    const POINTCOORD&       operator [] (int index) const
                                    {return (index == 0) ? x : y;}
};

inline CSize operator-(const POINT& a, const POINT& b)
        {return CSize(a.x-b.x, a.y-b.y);}

inline CPoint operator+(const SIZE& a, const POINT& b)
        {return CPoint(a.cx+b.x, a.cy+b.y);}
inline CPoint operator-(const SIZE& a, const POINT& b)
        {return CPoint(a.cx-b.x, a.cy-b.y);}
inline CPoint operator*(const SIZE& a, const POINT& b)
        {return CPoint(a.cx*b.x, a.cy*b.y);}
inline CPoint operator/(const SIZE& a, const POINT& b)
        {return CPoint(a.cx/b.x, a.cy/b.y);}

inline CPoint operator+(const POINT& a, const SIZE& b)
        {return CPoint(a.x+b.cx, a.y+b.cy);}
inline CPoint operator-(const POINT& a, const SIZE& b)
        {return CPoint(a.x-b.cx, a.y-b.cy);}
inline CPoint operator*(const POINT& a, const SIZE& b)
        {return CPoint(a.x*b.cx, a.y*b.cy);}
inline CPoint operator/(const POINT& a, const SIZE& b)
        {return CPoint(a.x/b.cx, a.y/b.cy);}

inline CPoint operator*(POINTCOORD a, const POINT& b)
        {return CPoint(a*b.x, a*b.y);}
inline CPoint operator/(POINTCOORD a, const POINT& b)
        {return CPoint(a/b.x, a/b.y);}

inline CPoint operator*(const POINT& a, POINTCOORD b)
        {return CPoint(a.x*b, a.y*b);}
inline CPoint operator/(const POINT& a, POINTCOORD b)
        {return CPoint(a.x/b, a.y/b);}

#pragma INCMSG("--- End 'point.hxx'")
#else
#pragma INCMSG("*** Dup 'point.hxx'")
#endif

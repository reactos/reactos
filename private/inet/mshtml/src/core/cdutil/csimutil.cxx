#include "headers.hxx"

#ifndef X_CSIMUTIL_HXX_
#define X_CSIMUTIL_HXX_
#include "csimutil.hxx"
#endif

MtDefine(CPointAry, Utilities, "CPointAry")
MtDefine(CPointAry_pv, CPointAry, "CPointAry::_pv")

//=-----------------------------------------------------------------------=
//
// Function:    NextNum
// 
// Synopsis:    Helper function for parsing out the COORDS attribute - 
//              After an intial call to _tcstok to set the pointer to
//              the first token, each call will set *plNum to the number
//              pointed to by *ppch, and then update *ppch to point to
//              the next token.
//
// Arguments:   LONG *plNum - Pointer to LONG to store next num in
//              TCHAR **ppch - Pointer to location in string
//
//=-----------------------------------------------------------------------=
HRESULT
NextNum(LONG *plNum, TCHAR **ppch)
{
    HRESULT hr = S_FALSE;

    Assert(plNum);
    *plNum = 0;


    if(*ppch)
    {
        IGNORE_HR(ttol_with_error(*ppch, plNum));
        *ppch = _tcstok(NULL, DELIMS);
        hr = S_OK;
    }

    RRETURN1(hr, S_FALSE);
}


//=-----------------------------------------------------------------------=
//
// Function:    PointInCircle
//
// Synopsis:    Returns TRUE if the point is contained within the circle.
//
// Arguments:   POINT pt - The point to be checked for containment
//
//=-----------------------------------------------------------------------=
BOOL
PointInCircle(POINT pt, LONG lx, LONG ly, LONG lradius)
{
    LONG lDistance;

    // Calculate the square of the distance to the center
    lDistance = (pt.x - lx) * (pt.x - lx) + (pt.y - ly) * (pt.y - ly);

    // Compare against the square of the radius
    if(lDistance <= lradius * lradius)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


//=-----------------------------------------------------------------------=
//
// Function: Contains
//
// Synopsis: Returns TRUE if the area contains the given point,
//              false if it does not.
//
//=-----------------------------------------------------------------------=

BOOL
Contains(POINT pt, union CoordinateUnion coords, UINT nShapeType)
{
    switch(nShapeType)
    {
    case SHAPE_TYPE_RECT:
        return PtInRect(&(coords.Rect), pt);
        break;

    case SHAPE_TYPE_CIRCLE:
        return PointInCircle(pt, 
                             coords.Circle.lx, 
                             coords.Circle.ly, 
                             coords.Circle.lradius);
        break;

    case SHAPE_TYPE_POLY:
        return PtInRegion(coords.Polygon.hPoly, pt.x, pt.y);
        break;

    default:
        Assert(FALSE && "Undefined shape");

        return FALSE;
        break;
    }
}


//=-----------------------------------------------------------------------=

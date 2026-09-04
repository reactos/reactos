// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:     "Energized" versions of MilRect structure, which adds
//                   members and operators.
//
//                   Designed to have the same memory layout as MilRect, so
//                   that you can cast between them.
//

#pragma once

//+----------------------------------------------------------------------------
//
//  Function:  SetIfGreater, SetIfLess
//
//  Synopsis:  Set InOutVariable to TestValue if TestValue is greater than or
//             less than InOutVariable, respectively.  These routines adds a
//             measure of stability when InOut value is a valid value and the >
//             and < operators for type T returns false when an invalid value
//             is involved.
//
//             For example for floating point types > and < will always return
//             false if a NaN value is involved.  Therefore both
//             SetIfGreater(normal_float, NaN) and SetIfLess(normal_float, NaN)
//             will always keep normal_float.
//

template <typename T> MIL_FORCEINLINE
void
SetIfGreater(
    __inout_ecount(1) T &InOutVariable,
    __in_ecount(1) T const &TestValue
    )
{
    if (TestValue > InOutVariable)
    {
        InOutVariable = TestValue;
    }
}

template <typename T> MIL_FORCEINLINE
void
SetIfLess(
    __inout_ecount(1) T &InOutVariable,
    __in_ecount(1) T const &TestValue
    )
{
    if (TestValue < InOutVariable)
    {
        InOutVariable = TestValue;
    }
}


//+----------------------------------------------------------------------------
//
//  Class:     TMilRect
//
//  Synopsis:  "Energized" version of template type TBaseRect that must
//             be defined as struct with these members:
//
//                  TBase left;
//                  TBase top;
//                  TBase right;
//                  TBase bottom;
//
//             This is an "energized" version as it provides extra member
//             methods.
//
//-----------------------------------------------------------------------------

// Enum tokens used to clarify which version of the contructor is being used
// in the cases where four TBase arguments are passed and the LTRB and XYWH
// constructors are indistinguishable.
typedef enum LTRB { LTRB_Parameters } LTRB;
typedef enum XYWH { XYWH_Parameters } XYWH;

//
// Values used to differentiate TMilRect's that are otherwise identical. 
// 
namespace RectUniqueness
{
    typedef struct {} NotNeeded;
    typedef struct {} _CMilRectL_;
    typedef struct {} _CMILSurfaceRect_;
}

template <class TBase, class TBaseRect, typename unique = RectUniqueness::NotNeeded>
class TMilRect : public TBaseRect
{
public:

    typedef TBase BaseUnitType;
    typedef TBaseRect BaseRectType;

public:

    //=========================================================================
    // Public typedefs:
    // 
    typedef TMilRect Rect_t;

    //=========================================================================
    // Constructors
    //

    //+------------------------------------------------------------------------
    //
    //  Member:    TMilRect
    //
    //  Synopsis:  Default ctor leaving members uninitialized
    //
    //-------------------------------------------------------------------------

    TMilRect()
    {
        // We require that you can typecast between TBaseRect and TMilRect.
        // To achieve this, TMilRect must have no data members or virtual functions.

        // This is a compile time assert so we only need it once here, but no where else.
        C_ASSERT( sizeof(TBaseRect) == sizeof(TMilRect) );
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    TMilRect ( left,top , right,bottom )
    //
    //  Synopsis:  Construct an LTRB rect from left,top - right,bottom params
    //
    //-------------------------------------------------------------------------

    TMilRect(
        TBase _left,
        TBase _top,
        TBase _right,
        TBase _bottom,
        LTRB ltrb
        )
    {
        UNREFERENCED_PARAMETER(ltrb);

        left = _left;
        top = _top;
        right = _right;
        bottom = _bottom;
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    TMilRect ( x, y, width, height )
    //
    //  Synopsis:  Construct an LTRB rect from X,Y WxH parameters
    //
    //  Notes:     !! Warning !!  No attempt is made to validate that the
    //              incoming values will fall within any range after converted
    //              to left,top - right,bottom.
    //
    //-------------------------------------------------------------------------

    TMilRect(
        TBase x,
        TBase y,
        TBase width,
        TBase height,
        XYWH xywh
        )
    {
        UNREFERENCED_PARAMETER(xywh);

        left = x;
        top = y;
        right = x + width;
        bottom = y + height;
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    TMilRect ( pt1, pt2 )
    //
    //  Synopsis:  Construct the smallest LTRB rect that contains pt1 and pt2
    //
    //-------------------------------------------------------------------------

    template<typename TPoint>
    TMilRect(
        TPoint pt1,
        TPoint pt2
        )
    {
        if (pt1.X < pt2.X)
        {
            left = pt1.X;
            right = pt2.X;
        }
        else
        {
            left = pt2.X;
            right = pt1.X;
        }

        if (pt1.Y < pt2.Y)
        {
            top = pt1.Y;
            bottom = pt2.Y;
        }
        else
        {
            top = pt2.Y;
            bottom = pt1.Y;
        }
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    TMilRect ( LTRB rect )
    //
    //  Synopsis:  Copy constructor
    //
    //-------------------------------------------------------------------------

    TMilRect(
        __in_ecount(1) const TBaseRect &rc
        )
    {
        left = rc.left;
        top = rc.top;
        right = rc.right;
        bottom = rc.bottom;
    }


    //=========================================================================
    // Properties
    //

    //+------------------------------------------------------------------------
    //
    //  Member:    HasValidValues
    //
    //  Synopsis:  Check if rectangle contains only valid values.  This is not
    //             a check for a well ordered rectangle.
    //
    //  Notes:     This assumes the == operator always returns false for
    //             invalid values, like NaNs for floating point types.
    //
    //-------------------------------------------------------------------------

    bool HasValidValues() const
    {
        return (   (left == left)
                && (top == top)
                && (right == right)
                && (bottom == bottom)
               );
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    IsWellOrdered
    //
    //  Synopsis:  Check if rectangle is well order such that all values are
    //             valid and right is not less than left and bottom is not less
    //             than top.
    //
    //  Notes:     This assumes the <= operator always returns false for
    //             invalid values, like NaNs for floating point types.
    //
    //-------------------------------------------------------------------------

    bool IsWellOrdered() const
    {
        return (left <= right) && (top <= bottom);
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    IsEmpty
    //
    //  Synopsis:  Check if rectangle has an area.
    //
    //  Notes:     This check assumes a well-ordered/normalized rectangle
    //
    //-------------------------------------------------------------------------

    bool IsEmpty() const
    {
        return (right <= left) || (bottom <= top);
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    IsInfinite
    //
    //  Synopsis:  Check if rectangle range is so great that it is considered
    //             to have infinite area.  See definition of sc_rcInfinite.
    //
    //-------------------------------------------------------------------------
    bool IsInfinite() const
    {
        //
        //  - These infinite values are actually the invalid
        // values of the rect, not the maximum extent. The reason for this is that
        // INT_MAX is already at the extreme boundary of the range. 
        // 
        return (   (left <= sc_rcInfinite.left && right  >= sc_rcInfinite.right )
                || (top  <= sc_rcInfinite.top  && bottom >= sc_rcInfinite.bottom));
    }

    //=========================================================================
    // Comparison members
    //

protected:
    //+------------------------------------------------------------------------
    //
    //  Member:    ==
    //
    //  Synopsis:  Returns true if this rectangle and given retangle contain
    //             exactly the same values.  To check for reprensentational
    //             equivalence use IsEquivalentTo.  Note that rectangles with
    //             invalid values are not expected to compare as equals -- see
    //             HasValidValues.
    //
    //             Note: There seems to be some danger in exposing this
    //                   publicly as the caller may want IsEquivalentTo.
    //
    //-------------------------------------------------------------------------

    bool operator == (
        __in_ecount(1) const TBaseRect &rc
        ) const
    {
        return (   (left == rc.left)
                && (top  == rc.top )
                && (right  == rc.right )
                && (bottom == rc.bottom)
               );
    }


public:
    //+------------------------------------------------------------------------
    //
    //  Member:    IsEquivalentTo
    //
    //  Synopsis:  Returns true if this rectangle and given rectangle express
    //             the same rectangle.  Some rectangles, such as empty and
    //             infinite rectangles, may be represented by a range of
    //             values.  See IsEmpty and IsInfinite for details.  Note also
    //             that rectangles with invalid values are not considered to
    //             represent the same rectangle and are not expected to compare
    //             as equals via == nor return true from IsEmpty or IsInfinite.
    //
    //-------------------------------------------------------------------------

    bool IsEquivalentTo(
        __in_ecount(1) const TMilRect &rc
        ) const
    {
        Assert(IsWellOrdered());
        Assert(rc.IsWellOrdered());

        return (
                   // Do they have exaclty the same values?
                   (*this == rc)
                   // Or are both empty representations?
                || (IsEmpty() && rc.IsEmpty())
                   // Or are both infinite representations?
                || (IsInfinite() && rc.IsInfinite())
               );
    }


    //+------------------------------------------------------------------------
    //
    //  Member:    DoesContain
    //
    //  Synopsis:  Returns true if this rectangle fully contains the given
    //             rectangle.  That is to say that the bounds of this rectangle
    //             are equal to or greater than the bounds of the given
    //             rectangle.
    //
    //  Returns:   true:  rc is within this.
    //             false: rc is not within or has invalid values
    //
    //-------------------------------------------------------------------------

    bool DoesContain(
        __in_ecount(1) const TMilRect &rc) const
    {
        Assert(IsWellOrdered());

        if (rc.IsEmpty())
        {
            return true;
        }

        return (   rc.left >= left
                && rc.top  >= top
                && rc.right  <= right
                && rc.bottom <= bottom
               );
    }

    //+-------------------------------------------------------------------------
    //
    //  Member:    DoesIntersectInclusive
    //
    //  Synopsis:  Check whether intersection of this rectangle and another one
    //             is empty.  Unlike DoesIntersect, this method treats rects as
    //             bottom/right inclusive, so it's possible for zero-area rects
    //             to intersect.
    //
    //  Returns:   true:  intersection is non-empty.
    //             false: intersection is empty.
    //
    //--------------------------------------------------------------------------

    bool DoesIntersectInclusive(
        __in_ecount(1) const TMilRect &rc
        ) const
    {
        // we want normalized rects here
        Assert(IsWellOrdered());
        Assert(rc.IsWellOrdered());

        return !(   right  >= rc.left) ? false
             : !(rc.right  >=    left) ? false
             : !(   bottom >= rc.top ) ? false
             : !(rc.bottom >=    top ) ? false
             : true;
    }

    //+-------------------------------------------------------------------------
    //
    //  Member:    DoesIntersect
    //
    //  Synopsis:  Check whether intersection of this rectangle and another one
    //             is empty.
    //
    //  Returns:   TRUE:  intersection is non-empty.
    //             FALSE: intersection is empty.
    //
    //--------------------------------------------------------------------------

    bool DoesIntersect(
        __in_ecount(1) const TMilRect &rc
        ) const
    {
        // we want normalized rects here
        Assert(IsWellOrdered());
        Assert(rc.IsWellOrdered());

        return IsEmpty() ? false
            : rc.IsEmpty() ? false
            : !(   right  > rc.left) ? false
            : !(rc.right  >    left) ? false
            : !(   bottom > rc.top ) ? false
            : !(rc.bottom >    top ) ? false
            : true;
    }




    //=========================================================================
    // Modification members
    //


    //+------------------------------------------------------------------------
    //
    //  Member:    SetEmpty
    //
    //  Synopsis:  Set this rectangle to empty, such that it has no area
    //
    //-------------------------------------------------------------------------

    VOID SetEmpty()
    {
        left = top = right = bottom = 0;
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    SetInfinite
    //
    //  Synopsis:  Set this rectangle to canonical infinite rectangle
    //
    //-------------------------------------------------------------------------

    VOID SetInfinite()
    {
        *this = sc_rcInfinite;
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    Inflate
    //
    //  Synopsis:  Inflates the rectangle by CX and CY such that
    //                  left'   = left - CX
    //                  top'    = top - CY
    //                  right'  = right + CX
    //                  bottom' = bottom + CY
    //                  width'  = width + 2 CX
    //                  height' = height + 2 CY
    //
    //              Note: CX and CY are expected to be valid non-negatives.
    //                    Accepting negative would mean we have a deflate
    //                    operation and that needs more logic.
    //
    //-------------------------------------------------------------------------
    void Inflate(
        TBase CX,
        TBase CY
        )
    {
        Assert(!IsEmpty()); // Inflating empty is ill-defined

        // Make sure we have valid non-negative inflation values
        Assert(CX >= 0);
        Assert(CY >= 0);

        left -= CX;
        top -= CY;
        right += CX;
        bottom += CY;
    }
    //+------------------------------------------------------------------------
    //
    //  Member:    Deflate
    //
    //  Synopsis:  Deflates the rectangle by CX and CY such that
    //                  left'   = left + CX
    //                  top'    = top + CY
    //                  right'  = right-CX
    //                  bottom' = bottom-CY
    //                  width'  = width-2*CX
    //                  height' = height-2*CY
    //             when 2*CX and 2*CY are less than width and height,
    //             respectively.  Otherwise the result is an empty rect.
    //
    //              Note: CX and CY are expected to be valid non-negatives.
    //                    Accepting negative would mean we have an inflate
    //                    operation and that different logic.
    //
    //-------------------------------------------------------------------------

    void Deflate(
        TBase CX,
        TBase CY
        )
    {
        Assert(IsWellOrdered());
        Assert(!IsInfinite()); // Deflating infinite is ill-defined

        // Make sure we have valid non-negative inflation values
        Assert(CX >= 0);
        Assert(CY >= 0);

        left += CX;
        top += CY;
        right -= CX;
        bottom -= CY;

        // check for empty rect
        if (IsEmpty())
        {
            // set beautified empty rect
            SetEmpty();
        }
        else
        {
            // Postcondition

            // For floats don't allow NaNs in result
            Assert(right >= left);
            Assert(bottom >= top);
        }

    }

    //+------------------------------------------------------------------------
    //
    //  Member:    Offset
    //
    //  Synopsis:  Offsets the reactangle by DX and DY such that
    //                  left'   = left + DX
    //                  top'    = top + DY
    //                  right'  = right + DX
    //                  bottom' = bottom + DY
    //                  width'  = width
    //                  height' = height
    //
    //              Note: DX and DY are expected to be valid values.
    //
    //-------------------------------------------------------------------------

    void Offset(
        TBase DX,
        TBase DY
        )
    {
        // Offsetting empty is "okay", if a little silly
        // We can at least expect a well ordered rectangle
        Assert(IsWellOrdered());

        // Make sure we have valid offset values - expect that invalid values
        // are not even equal to themselves, like NaNs for float types.
        Assert(DX == DX);
        Assert(DY == DY);

        left += DX;
        top += DY;
        right += DX;
        bottom += DY;
    }

//+------------------------------------------------------------------------
    //
    //  Member:    OffsetNoCheck
    //
    //  Synopsis:  Offsets the reactangle by DX and DY such that
    //                  left'   = left + DX
    //                  top'    = top + DY
    //                  right'  = right + DX
    //                  bottom' = bottom + DY
    //                  width'  = width
    //                  height' = height
    //
    //              Note: DX and DY can be invalid values like NaN.
    //                       The Rect can also have invalid values
    //
    //-------------------------------------------------------------------------

    void OffsetNoCheck(
        TBase DX,
        TBase DY
        )
    {
        left += DX;
        top += DY;
        right += DX;
        bottom += DY;
    }


    //+-------------------------------------------------------------------------
    //
    //  Member:    Intersect
    //
    //  Synopsis:  Intersects this rectangle with another one. Operates
    //             in-place.
    //
    //  Returns:   TRUE:  The result is non-empty.
    //             FALSE: The result is empty.
    //
    //  Notes:     If the incoming rectangle contains invalid values, like NaNs
    //             for floating point types, those invalid values will be
    //             ignored.  See SetIfGreater/SetIfLess for more details.
    //
    //             For example if rc.left was NaN then the result for this's
    //             left will be untouched, assuming neither rect is empty.
    //
    //--------------------------------------------------------------------------

    bool Intersect(
        __in_ecount(1) const TMilRect &rc
        )
    {
        // We want normalized rects here - assert assumption
        // For floats don't allow NaNs in this rectangle
        Assert(IsWellOrdered());
        // For floats allow for NaNs in incoming rectangle
        Assert(!(rc.right < rc.left));
        Assert(!(rc.bottom < rc.top));

        SetIfGreater(left, rc.left);
        SetIfGreater(top,  rc.top);
        SetIfLess(right, rc.right);
        SetIfLess(bottom, rc.bottom);

        // check for empty rect
        if (IsEmpty())
        {
            // set beautified empty rect
            SetEmpty();
        }
        else
        {
            // Postcondition

            // For floats don't allow NaNs in result
            Assert(right >= left);
            Assert(bottom >= top);
        }

        return !IsEmpty();
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    InclusiveUnion
    //
    //  Synopsis:  Unions this rectangle with another one. Unlike Union,
    //             this method treats rects as bottom/right inclusive, so if
    //             either or both of the rectangles is zero-sized, we still
    //             respect their positions when calculating the bounding
    //             rectangle.
    //
    //  Notes:     If the incoming rectangle contains invalid values, like NaNs
    //             for floating point types, those invalid values will be
    //             ignored.  See SetIfGreater/SetIfLess for more details.
    //
    //             For example if rc.left was NaN then the result for this's
    //             left will be untouched, assuming neither rect is empty.
    //
    //-------------------------------------------------------------------------

    void InclusiveUnion(
        __in_ecount(1) const TMilRect &rc
        )
    {
        // We want normalized rects here - assert assumption
        // For floats don't allow NaNs in this rectangle
        Assert(IsWellOrdered());
        // For floats allow for NaNs in incoming rectangle
        Assert(!(rc.right < rc.left));
        Assert(!(rc.bottom < rc.top));

        SetIfLess(left, rc.left);
        SetIfLess(top,  rc.top);
        SetIfGreater(right, rc.right);
        SetIfGreater(bottom, rc.bottom);

        // Postcondition

        // For floats don't allow NaNs in result
        Assert(right >= left);
        Assert(bottom >= top);
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    Union
    //
    //  Synopsis:  Unions this rectangle with another one. Operates in-place.
    //
    //  Returns:   TRUE:  The result is non-empty.
    //             FALSE: The result is empty.
    //
    //  Notes:     If the incoming rectangle contains invalid values, like NaNs
    //             for floating point types, those invalid values will be
    //             ignored.  See SetIfGreater/SetIfLess for more details.
    //
    //             For example if rc.left was NaN then the result for this's
    //             left will be untouched, assuming neither rect is empty.
    //
    //-------------------------------------------------------------------------

    bool Union(
        __in_ecount(1) const TMilRect &rc
        )
    {
        // We want normalized rects here - assert assumption
        // For floats don't allow NaNs in this rectangle
        Assert(IsWellOrdered());
        // For floats allow for NaNs in incoming rectangle
        Assert(!(rc.right < rc.left));
        Assert(!(rc.bottom < rc.top));

        BOOL fEmpty = IsEmpty();
        BOOL fEmpty2 = rc.IsEmpty();

        if (fEmpty && (fEmpty2 || !rc.HasValidValues())) {
            // Set it to the canonical empty rectangle
            SetEmpty();
            return FALSE;
        }

        if (fEmpty) {
            Assert(rc.HasValidValues());
            *this = rc;
            return TRUE;
        }

        if (fEmpty2) {
            // The result is unchanged
            return TRUE;
        }

        SetIfLess(left, rc.left);
        SetIfLess(top,  rc.top);
        SetIfGreater(right, rc.right);
        SetIfGreater(bottom, rc.bottom);

        // Postcondition

        // For floats don't allow NaNs in result
        Assert(right >= left);
        Assert(bottom >= top);
        return TRUE;
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    CalculateSubtractionRectangles
    //
    //  Synopsis:  Subtracts another supplied rectangle from this one and produces
    //             0 to 4 resultant rectangles
    //
    //  Returns:   Result count is always from 0 to 4, even if
    //             cMaxResultantRects is less than 4, and indicates how many
    //             results rects are needed for a complete calculation.  If
    //             fewer than that many rectangles are given, then the output
    //             array will be completely filled, but not a complete list.
    //
    //  Outputs:   Array of 0 to 4 rects
    //
    // The inversion produces at most four rectangles.
    //
    //      This
    // -----------------------------------
    // |         Top                     |
    // |                                 |
    // |---------------------------------|
    // |Left   | Mask    |       Right   |
    // |---------------------------------|
    // |                                 |
    // |         Bottom                  |
    // -----------------------------------
    //
    //
    //
    //  Notes:     Incoming rects should be well ordered, and the subtraction
    //             rect should be contained within the this rect.
    //
    //-------------------------------------------------------------------------

    __range(0,4) UINT CalculateSubtractionRectangles(
         __in_ecount(1) const TMilRect &subtractionRectangle,
         __out_ecount_part(cMaxResultantRects, ((return < cMaxResultantRects) ? return : cMaxResultantRects))
            TMilRect *rgResultantRects,
         __in_range(0,4) UINT cMaxResultantRects
         ) const
    {
        Assert(IsWellOrdered());
        Assert(DoesContain(subtractionRectangle));
        Assert(subtractionRectangle.IsWellOrdered());
        Assert(!subtractionRectangle.IsEmpty());

        UINT cResultantRects = 0;

        // top
        if (subtractionRectangle.top > top)
        {
            if (cResultantRects < cMaxResultantRects)
            {
                rgResultantRects[cResultantRects].left   = left;
                rgResultantRects[cResultantRects].top    = top;
                rgResultantRects[cResultantRects].right  = right;
                rgResultantRects[cResultantRects].bottom = subtractionRectangle.top;
                Assert(!rgResultantRects[cResultantRects].IsEmpty());
            }
            cResultantRects++;
        }

        // left
        if (subtractionRectangle.left > left)
        {
            if (cResultantRects < cMaxResultantRects)
            {
                rgResultantRects[cResultantRects].left   = left;
                rgResultantRects[cResultantRects].top    = subtractionRectangle.top;
                rgResultantRects[cResultantRects].right  = subtractionRectangle.left;
                rgResultantRects[cResultantRects].bottom = subtractionRectangle.bottom;
                Assert(!rgResultantRects[cResultantRects].IsEmpty());
            }
            cResultantRects++;
        }

        // right
        if (right > subtractionRectangle.right)
        {
            if (cResultantRects < cMaxResultantRects)
            {
                rgResultantRects[cResultantRects].left   = subtractionRectangle.right;
                rgResultantRects[cResultantRects].top    = subtractionRectangle.top;
                rgResultantRects[cResultantRects].right  = right;
                rgResultantRects[cResultantRects].bottom = subtractionRectangle.bottom;
                Assert(!rgResultantRects[cResultantRects].IsEmpty());
            }
            cResultantRects++;
        }

        // bottom
        if (bottom > subtractionRectangle.bottom)
        {
            if (cResultantRects < cMaxResultantRects)
            {
                rgResultantRects[cResultantRects].left   = left;
                rgResultantRects[cResultantRects].top    = subtractionRectangle.bottom;
                rgResultantRects[cResultantRects].right  = right;
                rgResultantRects[cResultantRects].bottom = bottom;
                Assert(!rgResultantRects[cResultantRects].IsEmpty());
            }
            cResultantRects++;
        }

        return cResultantRects;
    }


    //+------------------------------------------------------------------------
    //
    //  Member:    UnorderedWidth
    //
    //  Synopsis:  Returns the Width of the rectangle independent of whether
    //             the rectangle is well ordered; so, Width may be negative.
    //
    //-------------------------------------------------------------------------

    template <typename TDiff>
    TDiff UnorderedWidth() const
    {
        return static_cast<TDiff>(right) - left;
    }

    //+------------------------------------------------------------------------
    //
    //  Members:   Width
    //
    //  Synopsis:  Returns the Width of the rectangle.  Unless <type> is
    //             specified return type will be TBase.  Well ordered
    //             rectangles are expected; so the result should be
    //             non-negative.
    //
    //-------------------------------------------------------------------------

    template <typename TDiff>
    TDiff Width() const
    {
        Assert(IsWellOrdered());

        AssertOrderedDiffValid<TDiff>(left, right);

        return static_cast<TDiff>(right) - left;
    }

    TBase Width() const
    {
        return Width<TBase>();
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    UnorderedHeight
    //
    //  Synopsis:  Returns the Height of the rectangle independent of whether
    //             the rectangle is well ordered; so, Height may be negative.
    //
    //-------------------------------------------------------------------------

    template <typename TDiff>
    TDiff UnorderedHeight() const
    {
        return static_cast<TDiff>(bottom) - top;
    }

    //+------------------------------------------------------------------------
    //
    //  Members:   Height
    //
    //  Synopsis:  Returns the Height of the rectangle.  Unless <type> is
    //             specified return type will be TBase.  Well ordered
    //             rectangles are expected; so the result should be
    //             non-negative.
    //
    //-------------------------------------------------------------------------

    template <typename TDiff>
    TDiff Height() const
    {
        Assert(IsWellOrdered());

        AssertOrderedDiffValid<TDiff>(top, bottom);

        return static_cast<TDiff>(bottom) - top;
    }

    TBase Height() const
    {
        return Height<TBase>();
    }


    static TMilRect* ReinterpretBaseType(__inout_ecount_opt(1) TBaseRect *base)
    {
        return reinterpret_cast<TMilRect*>(base);
    }
    static const TMilRect* ReinterpretBaseType(__inout_ecount_opt(1) const TBaseRect *base)
    {
        return reinterpret_cast<const TMilRect*>(base);
    }

    //=========================================================================
    // Predefined constant versions of this class
    //

    //   Extreme coordinates not supported
    //   Because I'm not using infinity, these "empty" and "infinite"
    //   rectangles don't really include/exclude all possible points. (See
    //   their definitions in milrectf.cpp).

    static const Rect_t sc_rcEmpty;     // Warning: See ISSUE above

    static const Rect_t sc_rcInfinite;  // Warning: See ISSUE above
};


//+----------------------------------------------------------------------------
//
//  Class:     TMilRect_
//
//  Synopsis:  Derivative of  type TMilRect that defines a ctor for
//             converting TBaseRect_WH types.
//
//             TBaseRect_WH identifies the XYWH version of this rect that
//             must be defined as a struct with these members:
//
//                  TBase X, Y, Width, Height;
//
//-----------------------------------------------------------------------------

template <typename TBase, typename TBaseRect, typename TBaseRect_WH, typename unique = RectUniqueness::NotNeeded>
class TMilRect_ : public TMilRect<TBase, TBaseRect, unique>
{
public:

    //=========================================================================
    // Constructors
    //

    //+------------------------------------------------------------------------
    //
    //  Member:    TMilRect_
    //
    //  Synopsis:  Default ctor leaving members uninitialized
    //
    //-------------------------------------------------------------------------

    TMilRect_()
    {
        // We require that you can typecast between TBaseRect and TMilRect.
        // To achieve this, TMilRect must have no data members or virtual functions.

        // This is a compile time assert so we only need it once here, but no where else.
        C_ASSERT( sizeof(TBaseRect) == sizeof(TMilRect_) );
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    TMilRect_ ( XYWH rect )
    //
    //  Synopsis:  Contruct a LTRB rect from XYWH rect
    //
    //  Notes:     !! Warning !!  No attempt is made to validate that the
    //              incoming values will fall within any range after converted
    //              to left,top - right,bottom.
    //
    //-------------------------------------------------------------------------

    TMilRect_(
        __in_ecount(1) const TBaseRect_WH &rc
        )
    {
        left = rc.X;
        top = rc.Y;
        right = rc.X + rc.Width;
        bottom = rc.Y + rc.Height;
    }


    //+------------------------------------------------------------------------
    //
    //  Member:    Other TMilRect_ ctors
    //
    //  Synopsis:  Delegate to base class ctors
    //
    //-------------------------------------------------------------------------

    TMilRect_(
        TBase _left,
        TBase _top,
        TBase _right,
        TBase _bottom,
        LTRB ltrb
        )
//
// [pfx_parse] - workaround for PREfix parse problems with initializing
//
#if (!defined(_PREFIX_)) && (!defined(_PREFAST_))
    : TMilRect<TBase, TBaseRect, unique>(_left, _top, _right, _bottom, ltrb)
#endif // !_PREFIX_
    {}


    TMilRect_(
        TBase x,
        TBase y,
        TBase width,
        TBase height,
        XYWH xywh
        )
//
// [pfx_parse] - workaround for PREfix parse problems with initializing
//
#if (!defined(_PREFIX_)) && (!defined(_PREFAST_))
    : TMilRect<TBase, TBaseRect, unique>(x, y, width, height, xywh)
#endif // !_PREFIX_
    {}


    TMilRect_(__in_ecount(1) const TBaseRect &rc)
//
// [pfx_parse] - workaround for PREfix parse problems with initializing
//
#if (!defined(_PREFIX_)) && (!defined(_PREFAST_))
    : TMilRect<TBase, TBaseRect, unique>(rc)
#endif // !_PREFIX_
    {}

    template<typename TPoint>
    TMilRect_(
        TPoint pt1,
        TPoint pt2
        )
//
// [pfx_parse] - workaround for PREfix parse problems with initializing
//
#if (!defined(_PREFIX_)) && (!defined(_PREFAST_))
    : TMilRect<TBase, TBaseRect, unique>(pt1, pt2)
#endif // !_PREFIX_
    {}


    static TMilRect_* ReinterpretBaseType(__inout_ecount_opt(1) TBaseRect *base)
    {
        return reinterpret_cast<TMilRect_*>(base);
    }

    static const TMilRect_* ReinterpretBaseType(__inout_ecount_opt(1) const TBaseRect *base)
    {
        return reinterpret_cast<const TMilRect_*>(base);
    }

    static void HasBaseType();
};


// template <class TBase, class TBaseRect, class TBaseRect_WH>

typedef TMilRect_<FLOAT, MilRectF, MilPointAndSizeF> CMilRectF;

typedef
    TMilRect_<
        INT,
        MilRectL,
        MilPointAndSizeL,
        RectUniqueness::_CMilRectL_
        > CMilRectL;

typedef TMilRect<UINT , MilRectU > CMilRectU;


//+----------------------------------------------------------------------------
//
//  Function:
//      ExtendBaseByAdjacentSectionOfRect
//
//  Synopsis:
//      Given a base rectangle A (rcBase) and a second rectangle B
//      (rcPossibleExtension), find the largest extension of A that has no area
//      which does not intersect at least A or B.
//
//      This is a helper method for computing new valid area from the required
//      area and the current valid area.
//

template <typename TRect_RB>
void ExtendBaseByAdjacentSectionsOfRect(
    __in_ecount(1) TRect_RB const &rcBase,
    __in_ecount(1) TRect_RB const &rcPossibleExtension,
    __out_ecount(1) TRect_RB &rcExtended
    )
{
    // If "possible extension" area doesn't have a vertical gap separating it
    // and completely spans base area horizontally, then include vertical
    // extension of possible area with base area to form extended area.
    // Example:

    //       Extended (+)    Possible Extension (-)
    //      +-------+-+-+-+-+-+-+-------+
    //      | - - - :+ + + + + +: - - - |
    //      |- - - -: + + + + + :- - - -|
    //      +-------*+*+*+*+*+*+*-------+
    //              * + + + + + *
    //              *+ + + + + +*
    //              * + + + + + * Base (*) 
    //              *+*+*+*+*+*+*
    //

    bool const fExtendVertically =
            // Check for intersection or abutting edge
        (   (rcPossibleExtension.bottom >= rcBase.top)
         && (rcPossibleExtension.top <= rcBase.bottom)
            // Check horizontal extents
         && (rcPossibleExtension.left <= rcBase.left)
         && (rcBase.right <= rcPossibleExtension.right));

    // If "possible extension" area doesn't have a horizontal gap separating it
    // and completely spans base area vertically, then include horizontal
    // extension of possible area with base area to form extended area.
    bool const fExtendHorizontally =
            // Check for intersection or abutting edge
        (   (rcPossibleExtension.right >= rcBase.left)
         && (rcPossibleExtension.left <= rcBase.right)
            // Check vertical extents
         && (rcPossibleExtension.top <= rcBase.top)
         && (rcBase.bottom <= rcPossibleExtension.bottom));

    rcExtended.left =
        (fExtendHorizontally && rcPossibleExtension.left < rcBase.left) ?
        rcPossibleExtension.left : rcBase.left;

    rcExtended.top =
        (fExtendVertically && rcPossibleExtension.top < rcBase.top) ?
        rcPossibleExtension.top : rcBase.top;

    rcExtended.right =
        (fExtendHorizontally && rcPossibleExtension.right > rcBase.right) ?
        rcPossibleExtension.right : rcBase.right;

    rcExtended.bottom =
        (fExtendVertically && rcPossibleExtension.bottom > rcBase.bottom) ?
        rcPossibleExtension.bottom : rcBase.bottom;
}

inline CMilRectF MilRectLToMilRectF(__in const CMilRectL &rc)
{
    CMilRectF output(
        static_cast<float>(rc.left),
        static_cast<float>(rc.top),
        static_cast<float>(rc.right),
        static_cast<float>(rc.bottom),
        LTRB_Parameters
        );

    return output;        
}




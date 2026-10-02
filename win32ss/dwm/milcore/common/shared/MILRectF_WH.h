// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:     "Energized" version of MilPointAndSizeF
//                    structure, which adds members and operators.
//
//                   Designed to have the same memory layout as
//                   MilPointAndSizeF, so that you can cast between them.
//
//  Notes:           We should make a CMilPointAndSizeL class
//                   (for CMilPointAndSizeL) analogous to this one, instead of
//                   having standalone "IntersectRect" and "UnionRect" functions
//                   hidden in common/engine.cpp.
//

#pragma once

//+----------------------------------------------------------------------------
//
//  Class:       CMilPointAndSizeF, MilPointAndSizeF
//
//  Synopsis:    An "energized" version of MilPointAndSizeF, which adds members
//               and operators.
//
//               Designed to have the same memory layout as MilPointAndSizeF, so
//               that you can cast between them.
//
//-----------------------------------------------------------------------------

class CMilPointAndSizeF : public MilPointAndSizeF
{
public:

    // Constructors
    CMilPointAndSizeF()
    {
        // We require that you can typecast between MilPointAndSizeF and CMilPointAndSizeF.
        // To achieve this, CMilPointAndSizeF must have no data members or virtual functions.

        // This is a compile time assert so we only need it once here, but no where else.
        C_ASSERT( sizeof(MilPointAndSizeF) == sizeof(CMilPointAndSizeF) );
    }

    CMilPointAndSizeF(FLOAT x, FLOAT y, FLOAT width, FLOAT height)
    {
        X = x;
        Y = y;
        Width = width;
        Height = height;
    }

    CMilPointAndSizeF(__in_ecount(1) const MilPointAndSizeF &rc)
    {
        X = rc.X;
        Y = rc.Y;
        Width = rc.Width;
        Height = rc.Height;
    }

    const CMilPointAndSizeF & operator=(__in_ecount(1) const MilPointAndSizeF &rc)
    {
        X = rc.X;
        Y = rc.Y;
        Width = rc.Width;
        Height = rc.Height;
        return *this;
    }

    BOOL IsEmpty() const
    {
        return (Width <= 0.0f) || (Height <= 0.0f);
    }

    BOOL IsInfinite() const
    {
        return (Width >= FLT_MAX || Height >= FLT_MAX);
    }

    VOID SetEmpty()
    {
        X = Y = Width = Height = 0.0f;
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
    VOID Inflate(
        FLOAT CX,
        FLOAT CY
        )
    {
        Assert(!IsEmpty()); // Inflating empty is ill-defined

        // Make sure we have valid non-negative inflation values
        Assert(CX >= 0);
        Assert(CY >= 0);

        X -= CX;
        Y -= CY;
        Width += 2*CX;
        Height += 2*CY;
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
    //--------------------------------------------------------------------------

    BOOL Intersect(__in_ecount(1) const CMilPointAndSizeF &rc)
    {
        MilPointAndSizeF rcDst;

        // we want normalized rects here
        Assert(Width >= 0);
        Assert(Height >= 0);
        Assert(rc.Width >= 0);
        Assert(rc.Height >= 0);

        rcDst.X  = max(X, rc.X);
        rcDst.Width = -rcDst.X + min(
            X + Width,
            rc.X + rc.Width
            );

        // check for empty rect

        if (rcDst.Width > 0)
        {
            rcDst.Y = max(Y, rc.Y);
            rcDst.Height = -rcDst.Y + min(
                Y + Height,
                rc.Y + rc.Height
                );

            // check for empty rect

            if (rcDst.Height > 0)
            {
                Assert(rcDst.Width >= 0);
                Assert(rcDst.Height >= 0);

                *this = rcDst;

                return TRUE;        // not empty
            }
        }

        // empty rect

        SetEmpty();

        return FALSE;
    }


    //+-------------------------------------------------------------------------
    //
    //  Member:    Union
    //
    //  Synopsis:  Unions this rectangle with another one. Operates
    //             in-place.
    //
    //  Notes:     Ported from "UnionRect" in common/engine.cpp.
    //
    //  Returns:   TRUE:  The result is non-empty.
    //             FALSE: The result is empty.
    //
    //--------------------------------------------------------------------------

    BOOL Union(
        __in_ecount(1) const CMilPointAndSizeF &rc
        )
    {
        MilPointAndSizeF rcDst;

        // we want normalized rects here
        Assert(Width >= 0);
        Assert(Height >= 0);
        Assert(rc.Width >= 0);
        Assert(rc.Height >= 0);

        BOOL fEmpty = IsEmpty();
        BOOL fEmpty2 = rc.IsEmpty();

        if (fEmpty && fEmpty2) {
            // Set it to the canonical empty rectangle
            SetEmpty();
            return FALSE;
        }

        if (fEmpty) {
            *this = rc;
            return TRUE;
        }

        if (fEmpty2) {
            // The result is unchanged
            return TRUE;
        }

        rcDst.X  = min(X, rc.X);
        rcDst.Width = -rcDst.X + max(
            X + Width,
            rc.X + rc.Width
            );

        rcDst.Y = min(Y, rc.Y);
        rcDst.Height = -rcDst.Y + max(
            Y + Height,
            rc.Y + rc.Height
            );

        // Postcondition

        Assert(rcDst.Width >= 0);
        Assert(rcDst.Height >= 0);

        *this = rcDst;

        return TRUE;
    }

    //   Extreme coordinates not supported
    //   Because I'm not using infinity, these "empty" and "infinite" rectangles don't
    //   really include/exclude all possible points. (See their definitions in milrectf.cpp).

    //   Consider changing our rectangle format
    //
    //   The issue here demonstrates a general issue with using the
    //   (x,y,width,height) form of a rectangle instead of (x1,y1,x2,y2). The
    //   two different forms represent different sets of rectangles.
    //
    //   The latter form represents all points that can be expressed using
    //   floats, while the former can't include all of them, and has some
    //   rectangles which can include points not representable with floats.
    //   (Consider (FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX), which has points like
    //   (2*FLT_MAX - epsilon, 2*FLT_MAX - epsilon).
    //
    //   Similar arguments hold for MilPointAndSizeL (the integer version). And
    //   note that currently, MilPointAndSizeL uses signed types for Width and
    //   Height, yet negative values are considered "invalid".

    static const CMilPointAndSizeF sc_rcEmpty;     // Warning: See ISSUE above

    static const CMilPointAndSizeF sc_rcInfinite;  // Warning: See ISSUE above
};

// Convenience typedef for existing uses of CMilPointAndSizeF which assum XYWH
typedef CMilPointAndSizeF CMilPointAndSizeF;




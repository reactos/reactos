// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_geometry
//      $Keywords:
//
//  $Description:
//      Interfaces for constructing a Shape.
//
//  $ENDTAG
//
//  Classes:
//      IShapeBuilder, IFigureBuilder.
//
//------------------------------------------------------------------------------

class IFigureBuilder
{
public:
    virtual HRESULT StartAt(
        REAL x, REAL y)=0; // In: Figure's start point

    virtual HRESULT LineTo(
        IN   REAL x,              // Line's end point X
        IN   REAL y,              // Line's end point Y
        bool fSmoothJoin=false    // = true if forcing a smooth join
        )=0;

    virtual HRESULT BezierTo(
        IN REAL x2, REAL y2, 
            // Second Bezier point
        IN REAL x3, REAL y3,
            // Third Bezier point
        IN REAL x4, REAL y4,
            // Fourth Bezier point
        bool fSmoothJoin=false)=0; 
            // = true if forcing a smooth join

    virtual void SetStrokeState(BOOL fValue)=0;
    
    virtual HRESULT Close()=0;

    virtual void SetFillable(BOOL fValue)=0;
};

// Rename IShapeBuilder to CShape & remove typedef
typedef CShape IShapeBuilder;



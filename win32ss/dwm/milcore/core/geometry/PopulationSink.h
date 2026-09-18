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
//      Definition of IPopulationSink
//
//  $ENDTAG
//
//  Classes:
//      IPopulationSink.
//
//------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  Class:
//      IPopulationSink
//
//  Synopsis:
//      Interface for receiving points of a geometry.
//
//------------------------------------------------------------------------------
class IPopulationSink
{
public:
    IPopulationSink()
    {
    }

    virtual ~IPopulationSink()
    {
    }

    virtual HRESULT StartFigure(
        __in_ecount(1) const GpPointR &pt) = NULL;
            // Figure's first point

    virtual HRESULT AddLine(
        __in_ecount(1) const GpPointR &ptNew) = NULL;
            // The line segment's endpoint

    virtual HRESULT AddCurve(
        __in_ecount(3) const GpPointR *ptNew) = NULL;
            // The last 3 Bezier points of the curve (we already have the first one)

    virtual void SetCurrentVertexSmooth(bool val) = NULL;

    virtual void SetStrokeState(bool val) = NULL;

    virtual HRESULT EndFigure(
        bool fClosed) = NULL;
            // =true if the figure is closed

    virtual void SetFillMode(
        MilFillMode::Enum eFillMode) = NULL;
            // The mode that defines the fill set
};



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
//      Definition of CFlattener
//
//  $Notes:
//      Generates a flattened shape. This class is guaranteed never to call
//      AddCurve on its sink.
//
//  $ENDTAG
//
//  Classes:
//      CFlattener
//
//------------------------------------------------------------------------------

class CShapeFlattener  :   public IPopulationSink, CFlatteningSink
{
public:
    /*************************************************************************/
    // Constructor / destructor
    CShapeFlattener(
        __in_ecount(1) IPopulationSink *pSink,
            // The flattened destination shape
        double rTolerance)
            // Absolute Tolerance to which to flatten the Bezier
        
    : m_pSink(pSink), m_rTolerance(rTolerance)
    {
    }
    
    virtual ~CShapeFlattener() {}

    virtual HRESULT StartFigure(
        __in_ecount(1) const GpPointR &pt)
    {
        m_ptCurrent = pt;
        RRETURN(m_pSink->StartFigure(pt));
    }

    virtual HRESULT AddLine(
        __in_ecount(1) const GpPointR &ptNew)
            // The line segment's endpoint
    {
        m_ptCurrent = ptNew;
        RRETURN(m_pSink->AddLine(ptNew));
    }

    virtual HRESULT AddCurve(
        __in_ecount(3) const GpPointR *ptNew)
            // The last 3 Bezier points of the curve (we already have the first one)
    {
        HRESULT hr;

        CBezierFlattener flattener(this, m_rTolerance);

        flattener.SetPoint(0, m_ptCurrent);
        flattener.SetPoint(1, ptNew[0]);
        flattener.SetPoint(2, ptNew[1]);
        flattener.SetPoint(3, ptNew[2]);

        m_ptCurrent = ptNew[2];

        IFC(flattener.Flatten(false /* => no tangents */));

    Cleanup:
        RRETURN(hr);
    }

    virtual void SetCurrentVertexSmooth(bool val)
    {
        m_pSink->SetCurrentVertexSmooth(val);
    }

    virtual void SetStrokeState(bool val)
    {
        m_pSink->SetStrokeState(val);
    }

    virtual HRESULT EndFigure(
        bool fClosed)
            // =true if the figure is closed
    {
        RRETURN(m_pSink->EndFigure(fClosed));
    }

    virtual void SetFillMode(
        MilFillMode::Enum eFillMode)
            // The mode that defines the fill set
    {
        m_pSink->SetFillMode(eFillMode);
    }

    // CBezierFlattener callbacks
    virtual HRESULT Begin(
        __in_ecount(1) const GpPointR &)
            // First point (transformed)
    {
        // Ignore -- we've already dealt with this point.
        return S_OK;
    }

    virtual HRESULT AcceptPoint(
        IN const GpPointR &ptNew,
            // The point
        IN GpReal t,
            // Parameter we're at
        OUT bool          &fAbort)
            // Set to true to signal aborting
    {
        fAbort = false;
        RRETURN(m_pSink->AddLine(ptNew));
    }

    // Data
protected:
    IPopulationSink *m_pSink; // Our destination sink
    GpPointR m_ptCurrent;     // The last point we've seen
    double   m_rTolerance;    // Tolerance to which to flatten the Bezier
};




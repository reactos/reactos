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
//      Definition of CPopulationSinkAdapter
//
//  $Notes:
//      This class is intended to be placed at the end of a IPopulationSink
//      pipeline. It converts IPopulationSink calls into corresponding
//      IShapeBuilder calls.
//
//      Note that we do not AddRef() the IShapeBuilder we are given during
//      construction. Thus, users must ensure that they continue to hold a
//      reference to it.
//
//      Since callers are allowed to call SetCurrentVertexSmooth() at any point
//      in time after calling AddVertex(), we can't add the vertex inside the
//      AddVertex(). Rather, we have to store it and wait until the next
//      AddVertex() or EndFigure() call to update our figure.
//
//  $ENDTAG
//
//  Classes:
//      CPopulationSinkAdapter.
//
//------------------------------------------------------------------------------


//+-----------------------------------------------------------------------------
//
//  Class:
//      IPopulationSinkAdapter
//
//  Synopsis:
//      Converts IPopulationSink method calls to IShapeBuilder calls.
//
//------------------------------------------------------------------------------
class CPopulationSinkAdapter : public IPopulationSink
{
public:
    CPopulationSinkAdapter(
        __in_ecount(1) IShapeBuilder *pResult)
            // The recepient of the result of the operation
        : m_pShapeNoRef(pResult), m_pFigure(NULL),
          m_eLastSegmentType(MilSegmentType::None), m_fLastPointSmooth(false),
          m_fStrokedState(true), m_fStrokeStateUpdated(false)
    {
    }

    virtual ~CPopulationSinkAdapter()
    {
        Assert(m_pFigure == NULL);
    }

    virtual HRESULT StartFigure(
         __in_ecount(1) const GpPointR &pt)
        // Figure's first point
    {
        HRESULT hr = S_OK;

        Assert(m_pFigure == NULL);
        IFC(m_pShapeNoRef->AddNewFigure(m_pFigure));
        IFC(m_pFigure->StartAt(static_cast<REAL>(pt.X), static_cast<REAL>(pt.Y)));

        m_eLastSegmentType = MilSegmentType::None;

    Cleanup:
        RRETURN(hr);
    }

    virtual HRESULT AddLine(
        __in_ecount(1) const GpPointR &pt)
            // The line segment's endpoint
    {
        HRESULT hr = S_OK;
        Assert(m_pFigure != NULL);

        IFC(AddLastSegment());

        m_lastPoints[0] = pt;

        m_eLastSegmentType = MilSegmentType::Line;
        m_fLastPointSmooth = false;

    Cleanup:
        RRETURN(hr);
    }

    virtual HRESULT AddCurve(
        __in_ecount(3) const GpPointR *pts)
        // The last 3 Bezier points of the curve (we already have the first one)
    {
        HRESULT hr = S_OK;
        Assert(m_pFigure != NULL);

        IFC(AddLastSegment());

        for (int i = 0; i < 3; ++i)
        {
            m_lastPoints[i] = pts[i];
        }

        m_eLastSegmentType = MilSegmentType::Bezier;
        m_fLastPointSmooth = false;

    Cleanup:
        RRETURN(hr);
    }
    
    virtual void SetCurrentVertexSmooth(bool val)
    {
        Assert(m_eLastSegmentType == MilSegmentType::Line ||
               m_eLastSegmentType == MilSegmentType::Bezier);

        m_fLastPointSmooth = val;
    }

    virtual void SetStrokeState(bool val)
    {
        if (val != m_fStrokedState)
        {
            m_fStrokeStateUpdated = true;
            m_fStrokedState = val;
        }
    }

    virtual HRESULT EndFigure(
        bool fClosed)
            // =true if the figure is closed
    {
        HRESULT hr = S_OK;

        IFC(AddLastSegment());

        if (fClosed)
        {
            IFC(m_pFigure->Close());
        }

        m_pFigure = NULL;

    Cleanup:
        RRETURN(hr);
    }

    virtual void SetFillMode(
        MilFillMode::Enum eFillMode)
            // The mode that defines the fill set
    {
        m_pShapeNoRef->SetFillMode(eFillMode);
    }

private:

    HRESULT AddLastSegment()
    {
        HRESULT hr = S_OK;

        if (m_eLastSegmentType == MilSegmentType::Line)
        {
            IFC(m_pFigure->LineTo(static_cast<REAL>(m_lastPoints[0].X),
                                  static_cast<REAL>(m_lastPoints[0].Y),
                                  m_fLastPointSmooth));
        }
        else if (m_eLastSegmentType == MilSegmentType::Bezier)
        {
            IFC(m_pFigure->BezierTo(static_cast<REAL>(m_lastPoints[0].X), static_cast<REAL>(m_lastPoints[0].Y), 
                                    static_cast<REAL>(m_lastPoints[1].X), static_cast<REAL>(m_lastPoints[1].Y), 
                                    static_cast<REAL>(m_lastPoints[2].X), static_cast<REAL>(m_lastPoints[2].Y), 
                                    m_fLastPointSmooth));
        }
        else
        {
            Assert(m_eLastSegmentType == MilSegmentType::None);
        }

        if (m_fStrokeStateUpdated)
        {
            m_pFigure->SetStrokeState(m_fStrokedState);
            m_fStrokeStateUpdated = false;
        }

    Cleanup:
        RRETURN(hr);
    }

private:

    IShapeBuilder * m_pShapeNoRef;
        // The shape to populate
    IFigureBuilder * m_pFigure;
        // The current figure being added to.

    MilSegmentType::Enum m_eLastSegmentType;
        // The type of the last segment
    GpPointR m_lastPoints[3];
        // Points received from the last AddLine() or AddCurve() call 
        // (in the case of AddLine(), only the first value is valid).
    bool m_fLastPointSmooth;
        // Has m_lastPoint been designated smooth?
    bool m_fStrokedState;
        // Are we stroking or no?
    bool m_fStrokeStateUpdated;
        // Has our stroke state been updated since our last add?
};



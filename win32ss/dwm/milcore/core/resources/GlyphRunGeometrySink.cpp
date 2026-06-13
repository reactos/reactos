// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_text
//      $Keywords:
//
//  $Description:
//      Glyph run geometry creator.
//
//  $ENDTAG
//
//  Classes:
//      CGlyphRunGeometrySink
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CGlyphRunGeometrySink, MILRender, "CGlyphRunGeometrySink");

MtDefine(MilPathGeometry, CGlyphRunGeometrySink, "CGlyphRunGeometrySink geometry struct");
MtDefine(MilPathFigure, CGlyphRunGeometrySink, "CGlyphRunGeometrySink figure struct");
MtDefine(MilSegmentLine, CGlyphRunGeometrySink, "CGlyphRunGeometrySink line segment struct");
MtDefine(MilSegmentBezier, CGlyphRunGeometrySink, "CGlyphRunGeometrySink bezier segment struct");
MtDefine(GlyphGeometryDataStructs, CGlyphRunGeometrySink, "CGlyphRunGeometrySink contiguous geometry structs");


//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunResource::CGlyphRunResource
//
//  Synopsis:
//      Constructor
//
//------------------------------------------------------------------------------
CGlyphRunGeometrySink::CGlyphRunGeometrySink()
{
    m_hr = S_OK;

    m_isSinkClosed = false;
    m_hasProducedGeometry = false;
    m_pathGeometryData.m_cbFiguresSize = 0;
    m_pathGeometryData.m_FillRule = MilFillMode::Alternate;
    m_pathGeometryData.m_pTransform = NULL;
    m_pathGeometryData.m_pFiguresData = NULL;

    m_currentOffset = sizeof(MilPathGeometry);

    m_cFigures = -1;
    
    m_pCurrentFigure = NULL;
    m_currentFigureOffset = -1;
    m_offsetToLastSegment = -1;
    m_lastFigureSize = 0;

    m_pCurrentSegment = NULL;
    m_currentSegmentOffset = -1;
    m_lastSegmentSize = 0;
    m_isSegGap = false;
    m_isSegSmoothJoin = false;

    m_pGeometry = NULL;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunResource::Init
//
//  Synopsis:
//      Initializer
//
//------------------------------------------------------------------------------
HRESULT
CGlyphRunGeometrySink::Initialize()
{
    HRESULT hr = S_OK;
    
    // Initialize our geometry struct.
    m_pGeometry = static_cast<MilPathGeometry*>WPFAlloc(ProcessHeap, Mt(MilPathGeometry), sizeof(MilPathGeometry));
    IFCOOM(m_pGeometry);

    m_pGeometry->Flags = 0;
    m_pGeometry->FigureCount = 0;
    m_pGeometry->Bounds.top = m_pGeometry->Bounds.left = m_pGeometry->Bounds.bottom = m_pGeometry->Bounds.right = 0.0;
    m_pGeometry->Size = m_currentOffset;

    m_arrGeometryDataStructsCount = 1;
    IFC(m_arrGeometryDataStructs.Add(static_cast<void*>(m_pGeometry)));

Cleanup:
    if (FAILED(hr) && m_pGeometry)
    {
        WPFFree(ProcessHeap, m_pGeometry);
        m_pGeometry = NULL;
    }
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunResource::Create
//
//  Synopsis:
//      Static Factory
//
//------------------------------------------------------------------------------
HRESULT
CGlyphRunGeometrySink::Create(
    __deref_out CGlyphRunGeometrySink **ppGeometrySink
    )
{
    HRESULT hr = S_OK;

    CGlyphRunGeometrySink *pGeometrySink = NULL;

    IFCOOM(pGeometrySink = new CGlyphRunGeometrySink());    
    pGeometrySink->AddRef();
   
    IFC(pGeometrySink->Initialize());

    *ppGeometrySink = pGeometrySink; // Transitioning ref count to out argument
    pGeometrySink = NULL;
   
Cleanup:
    ReleaseInterface(pGeometrySink);
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunGeometrySink::~CGlyphRunGeometrySink()
//
//  Synopsis:
//      Destructor
//
//------------------------------------------------------------------------------
CGlyphRunGeometrySink::~CGlyphRunGeometrySink()
{   
    // If we haven't closed the sink, we need to free any structs we may have
    // accumulated.
    if (!m_isSinkClosed)
    {
        for (UINT i = 0; i < m_arrGeometryDataStructsCount; i += 1)
        {
            WPFFree(ProcessHeap, m_arrGeometryDataStructs[i]);
        }
    }
    
    // If we've created the geometry structs but haven't copied them into a 
    // resource, free them.
    if (m_isSinkClosed && !m_hasProducedGeometry)
    {
        if (m_pathGeometryData.m_pFiguresData)
        {
            WPFFree(ProcessHeap, m_pathGeometryData.m_pFiguresData);
            m_pathGeometryData.m_pFiguresData = NULL;
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunGeometrySink::ProduceGeometry()
//
//  Synopsis:
//      Returns the geometry object created by this sink.  The caller owns the reference
//      to the CMilGeometryDuce returned.
//
//------------------------------------------------------------------------------
HRESULT 
CGlyphRunGeometrySink::ProduceGeometry(
    __in MilPoint2F* pBaselineOrigin,
    __deref_out CMilGeometryDuce **ppGeometry
    )
{
    HRESULT hr = S_OK;

    Assert(!m_hasProducedGeometry);

    CMilTranslateTransformDuce *pTransform = NULL;
    CMilPathGeometryDuce *pPathGeometry = NULL;
    
    // We can only produce geometry once per sink since we write the data directly
    // into m_pathGeometryData, the resource data struct - this prevents needing to copy it again.
    if (m_hasProducedGeometry)
    {
        IFC(E_FAIL);
    }
    
    // We only have a valid geometry if we have processed all the commands
    // and closed our sink.
    if (!m_isSinkClosed)
    {
        IFC(Close());
    }

    IFC(CMilTranslateTransformDuce::Create(pBaselineOrigin, &pTransform));
    m_pathGeometryData.m_pTransform = pTransform;

    IFC(CMilPathGeometryDuce::Create(
        m_pathGeometryData.m_pTransform,
        m_pathGeometryData.m_FillRule,
        m_pathGeometryData.m_cbFiguresSize,
        m_pathGeometryData.m_pFiguresData, // Transitioning allocation to new object to avoid copy
        &pPathGeometry
        ));

    m_hasProducedGeometry = true;
    
    // The figures will continue to live after the sink has been destroyed, we passed
    // the reference to them out inside the CMilGeometryDuce.  We could follow the
    // standard pattern and have the Create method copy the data, but this data could
    // be large so we're optimizing it here since the sink is no longer useful
    // after it produces a geometry object.
    m_pathGeometryData.m_pFiguresData = NULL;
    
    *ppGeometry = pPathGeometry; // Transitioning ref to out arg
    pPathGeometry = NULL;

Cleanup:
    ReleaseInterface(pTransform);
    ReleaseInterface(pPathGeometry);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunGeometrySink::ProduceGeometryData()
//
//  Synopsis:
//      Returns the path geometry data struct created by this sink.  The caller now owns the
//      *ppGeometryData memory allocation, the serialized geometry data.
//
//------------------------------------------------------------------------------
HRESULT 
CGlyphRunGeometrySink::ProduceGeometryData(
    __deref_out MilPathGeometry **ppGeometryData,
    OUT UINT *pSize,
    OUT MilFillMode::Enum *pFillRule
    )
{
    HRESULT hr = S_OK;

    Assert(!m_hasProducedGeometry);

    CMilTranslateTransformDuce *pTransform = NULL;
    CMilPathGeometryDuce *pPathGeometry = NULL;
    
    // We can only produce geometry once per sink since we write the data directly
    // into m_pathGeometryData, the resource data struct - this prevents needing to copy it again.
    if (m_hasProducedGeometry)
    {
        IFC(E_FAIL);
    }
    
    // We only have a valid geometry if we have processed all the commands
    // and closed our sink.
    if (!m_isSinkClosed)
    {
        IFC(Close());
    }

    *ppGeometryData = m_pathGeometryData.m_pFiguresData;
    *pSize = m_pathGeometryData.m_cbFiguresSize;
    *pFillRule = m_pathGeometryData.m_FillRule;

    m_hasProducedGeometry = true;

Cleanup:
    if (FAILED(hr))
    {
        ReleaseInterface(pTransform);
        ReleaseInterface(pPathGeometry);
    }
    RRETURN(hr);
}
    
//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunGeometrySink::HrFindInterface()
//
//  Synopsis:
//      
//
//------------------------------------------------------------------------------
HRESULT
CGlyphRunGeometrySink::HrFindInterface(
    __in_ecount(1) REFIID riid, __deref_out void **ppvObject
    )
{
    return E_NOTIMPL;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunGeometrySink::AddLine()
//
//  Synopsis:
//      
//
//------------------------------------------------------------------------------

void
CGlyphRunGeometrySink::AddLine(
    D2D1_POINT_2F point 
    )
{
    if (SUCCEEDED(m_hr))
    {
        MilPoint2D segPoint;
        segPoint.X = point.x;
        segPoint.Y = point.y;
        
        AddGenericPoly(
            &segPoint, 
            NULL, 
            NULL,  
            false, 
            MilSegmentType::Line
            );
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunGeometrySink::AddBezier()
//
//  Synopsis:
//      
//
//------------------------------------------------------------------------------
void
CGlyphRunGeometrySink::AddBezier(
    __in CONST D2D1_BEZIER_SEGMENT *bezier 
    )
{    
    if (SUCCEEDED(m_hr))
    {
        MilPoint2D segPoint1;
        segPoint1.X = bezier->point1.x;
        segPoint1.Y = bezier->point1.y;

        MilPoint2D segPoint2;
        segPoint2.X = bezier->point2.x;
        segPoint2.Y = bezier->point2.y;

        MilPoint2D segPoint3;
        segPoint3.X = bezier->point3.x;
        segPoint3.Y = bezier->point3.y;
        
        AddGenericPoly(
            &segPoint1, 
            &segPoint2, 
            &segPoint3,  
            true, 
            MilSegmentType::Bezier
            );
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunGeometrySink::SetFillMode()
//
//  Synopsis:
//      
//
//------------------------------------------------------------------------------
void
CGlyphRunGeometrySink::SetFillMode(
    D2D1_FILL_MODE fillMode 
    )
{
    if (m_isSinkClosed)
    {
        MIL_THRX(m_hr, E_FAIL);
    }
    
    if (SUCCEEDED(m_hr))
    {
        if (fillMode == D2D1_FILL_MODE_ALTERNATE)
        {
            m_pathGeometryData.m_FillRule = MilFillMode::Alternate;
        }
        else if (fillMode == D2D1_FILL_MODE_WINDING)
        {
            m_pathGeometryData.m_FillRule = MilFillMode::Winding;
        }
        else
        {
            MIL_THRX(m_hr, E_FAIL);
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunGeometrySink::SetSegmentFlags()
//
//  Synopsis:
//      
//
//------------------------------------------------------------------------------    
void
CGlyphRunGeometrySink::SetSegmentFlags(
    D2D1_PATH_SEGMENT vertexFlags 
    )
{
    if (m_isSinkClosed)
    {
        MIL_THRX(m_hr, E_FAIL);
    }
    
    // Must have begun a segment.
    if (m_pCurrentSegment == NULL || m_currentSegmentOffset == -1)
    {
        MIL_THRX(m_hr, E_FAIL);
    }

    if (SUCCEEDED(m_hr))
    {
        bool isAGap = ((vertexFlags & D2D1_PATH_SEGMENT_FORCE_UNSTROKED) == 0);
        bool isSmooth = ((vertexFlags & D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN) == 0);

        // If the flags have changed, we must end the current segment and start a new one.
        // We set the old flags on the old segment in EndSegment().  Then we cache the new
        // flag values for use later.  A new segment will be created by the next Add___ call.
        if (isAGap != m_isSegGap || isSmooth != m_isSegSmoothJoin)
        {
            EndSegment();        
            m_isSegGap = isAGap;
            m_isSegSmoothJoin = isSmooth;
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunGeometrySink::BeginFigure()
//
//  Synopsis:
//      
//
//------------------------------------------------------------------------------
    
void
CGlyphRunGeometrySink::BeginFigure(
    D2D1_POINT_2F startPoint,
    D2D1_FIGURE_BEGIN figureBegin 
    )
{
    if (m_isSinkClosed)
    {
        MIL_THRX(m_hr, E_FAIL);
    }
    
    // If we haven't EndFigure()ed we should fail.
    if (m_pCurrentFigure != NULL || m_currentFigureOffset != -1)
    {
        MIL_THRX(m_hr, E_FAIL);
    }
    
    if (SUCCEEDED(m_hr))
    {
        // Save the offset to the start of this figure.
        m_currentFigureOffset = m_currentOffset;

        // Create a new figure and fill it in.
        m_pCurrentFigure = static_cast<MilPathFigure*>WPFAlloc(ProcessHeap, Mt(MilPathFigure), sizeof(MilPathFigure));
        m_pCurrentFigure->BackSize = m_lastFigureSize;
        m_pCurrentFigure->Flags = (figureBegin == D2D1_FIGURE_BEGIN_FILLED) ? MilPathFigureFlags::IsFillable : 0;
        m_pCurrentFigure->StartPoint.X = startPoint.x;
        m_pCurrentFigure->StartPoint.Y = startPoint.y;

        m_currentOffset += sizeof(MilPathFigure);
        // Initializing these fields to be updating by subsequent Add__ and EndFigure() calls.
        m_pCurrentFigure->Count = 0;
        m_pCurrentFigure->Size = m_currentOffset - m_currentFigureOffset;
        m_pCurrentFigure->OffsetToLastSegment = 0;


        // Add the struct to our list.
        MIL_THRX(m_hr, m_arrGeometryDataStructs.Add(static_cast<void*>(m_pCurrentFigure)));
        m_arrGeometryDataStructsCount += 1;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunGeometrySink::AddLines()
//
//  Synopsis:
//      
//
//------------------------------------------------------------------------------    
void
CGlyphRunGeometrySink::AddLines(
    __in_ecount(pointsCount) CONST D2D1_POINT_2F *points,
    UINT pointsCount 
    )
{
    if (m_isSinkClosed)
    {
        MIL_THRX(m_hr, E_FAIL);
    }
    
    if (SUCCEEDED(m_hr))
    {
        for (UINT i = 0; i < pointsCount; i++)
        {
            AddLine(*(points + i));
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunGeometrySink::AddBeziers()
//
//  Synopsis:
//      
//
//------------------------------------------------------------------------------    
void
CGlyphRunGeometrySink::AddBeziers(
    __in_ecount(beziersCount) CONST D2D1_BEZIER_SEGMENT *beziers,
    UINT beziersCount 
    )
{
    if (m_isSinkClosed)
    {
        MIL_THRX(m_hr, E_FAIL);
    }
    
    if (SUCCEEDED(m_hr))
    {
        for (UINT i = 0; i < beziersCount; i++)
        {
            AddBezier(beziers + i);
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunGeometrySink::EndFigure()
//
//  Synopsis:
//      
//
//------------------------------------------------------------------------------    
void
CGlyphRunGeometrySink::EndFigure(
    D2D1_FIGURE_END figureEnd 
    )
{
    if (m_isSinkClosed)
    {
        MIL_THRX(m_hr, E_FAIL);
    }
    
    // Must have begun a figure.
    if (m_pCurrentFigure == NULL || m_currentFigureOffset == -1)
    {
        MIL_THRX(m_hr, E_FAIL);
    }

    if (SUCCEEDED(m_hr))
    {    
        EndSegment();

        m_pCurrentFigure->Flags |= (figureEnd == D2D1_FIGURE_END_CLOSED) ? MilPathFigureFlags::IsClosed : 0;

        m_pGeometry->Flags |= ((m_pCurrentFigure->Flags & MilPathFigureFlags::HasCurves) != 0) ? MilPathGeometryFlags::HasCurves : 0;
        m_pGeometry->Flags |= ((m_pCurrentFigure->Flags & MilPathFigureFlags::HasGaps) != 0) ? MilPathGeometryFlags::HasGaps : 0;
        m_pGeometry->Flags |= ((m_pCurrentFigure->Flags & MilPathFigureFlags::IsFillable) == 0) ? MilPathGeometryFlags::HasHollows : 0;
        m_pGeometry->FigureCount += 1;
        m_pGeometry->Size = m_currentOffset;

        m_lastFigureSize = m_pCurrentFigure->Size;

        // Note, we've saved the pointer in our DynArray in BeginFigure.
        m_pCurrentFigure = NULL;
        m_currentFigureOffset = -1;

        m_lastSegmentSize = 0;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunGeometrySink::AddGenericPoly()
//
//  Synopsis:
//      
//
//------------------------------------------------------------------------------   
void 
CGlyphRunGeometrySink::AddGenericPoly(
    MilPoint2D *point1,
    MilPoint2D *point2,
    MilPoint2D *point3,
    bool hasCurves,
    MilSegmentType::Enum segmentType)
{
    // Must have begun a figure.
    if (m_pCurrentFigure == NULL || m_currentFigureOffset == -1)
    {
        MIL_THRX(m_hr, E_FAIL);
    }
    
    if (SUCCEEDED(m_hr))
    {
        Assert(point1
            && (   (!point2 && !point3 && segmentType == MilSegmentType::Line)
                || (point2 && point3 && segmentType == MilSegmentType::Bezier)));

        // If we have a current segment, finish it first.
        if (m_pCurrentSegment != NULL)
        {
            EndSegment();
        }

        Assert(m_pCurrentSegment == NULL && m_currentSegmentOffset == -1);

        // Save the offset to this segment.
        m_currentSegmentOffset = m_currentOffset;

        // Initialize a new segment based on segment type;
        switch (segmentType)
        {
            case MilSegmentType::Line:
            {
                MilSegmentLine *tempLineSeg = static_cast<MilSegmentLine*>WPFAlloc(ProcessHeap, Mt(MilSegmentLine), sizeof(MilSegmentLine));
                tempLineSeg->Point = *point1;
                m_pCurrentSegment = tempLineSeg;
                m_currentOffset += sizeof(MilSegmentLine);
                break;
            }
            case MilSegmentType::Bezier:
            {
                MilSegmentBezier *tempBezierSeg = static_cast<MilSegmentBezier*>WPFAlloc(ProcessHeap, Mt(MilSegmentBezier), sizeof(MilSegmentBezier));
                tempBezierSeg->Point1 = *point1;
                tempBezierSeg->Point2 = *point2;
                tempBezierSeg->Point3 = *point3;
                m_pCurrentSegment = tempBezierSeg; 
                m_currentOffset += sizeof(MilSegmentBezier);  
                break;
            }
            default:
                // Arcs and quadratic beziers are not currently supported.                
                Assert(false);
        }
    
        m_pCurrentSegment->Type = segmentType;
        m_pCurrentSegment->Flags = 0;
        m_pCurrentSegment->Flags |= hasCurves ? MilCoreSeg::IsCurved : 0;
        m_pCurrentSegment->BackSize = m_lastSegmentSize;

        // Add the struct pointer to our list.
        MIL_THRX(m_hr, m_arrGeometryDataStructs.Add(static_cast<void*>(m_pCurrentSegment)));
        m_arrGeometryDataStructsCount += 1;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunGeometrySink::EndSegment()
//
//  Synopsis:
//      
//
//------------------------------------------------------------------------------   
void 
CGlyphRunGeometrySink::EndSegment()
{
    // Must have begun a segment.
    if (m_pCurrentSegment == NULL || m_currentSegmentOffset == -1)
    {
        MIL_THRX(m_hr, E_FAIL);
    }
    
    if (SUCCEEDED(m_hr))
    {
        // Update segment flags, since SetSegmentFlags is a separate call.
        m_pCurrentSegment->Flags |= m_isSegGap? MilCoreSeg::IsAGap : 0;
        m_pCurrentSegment->Flags |= m_isSegSmoothJoin? MilCoreSeg::SmoothJoin : 0;

        // Update figure information.
        m_pCurrentFigure->Flags |= ((m_pCurrentSegment->Flags & MilCoreSeg::IsAGap) == 0) ? MilPathFigureFlags::HasGaps : 0;
        m_pCurrentFigure->Flags |= ((m_pCurrentSegment->Flags & MilCoreSeg::IsCurved) == 0) ? MilPathFigureFlags::HasCurves : 0;
        m_pCurrentFigure->Count += 1;
        m_pCurrentFigure->Size = m_currentOffset - m_currentFigureOffset;
        m_pCurrentFigure->OffsetToLastSegment = m_currentSegmentOffset - m_currentFigureOffset;

        // Reset segment information.
        switch (m_pCurrentSegment->Type)
        {
            case MilSegmentType::Line:
                m_lastSegmentSize = sizeof(MilSegmentLine);
                break;
            case MilSegmentType::Bezier:
                m_lastSegmentSize = sizeof(MilSegmentBezier);
                break;
            default:
                // Arcs and quadratic beziers are not currently supported.                
                Assert(false);
        }
    
        // Note, we've saved the pointer in our DynArray in AddGenericPoly.
        m_pCurrentSegment = NULL;
        m_currentSegmentOffset = -1;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunGeometrySink::Close()
//
//  Synopsis:
//      
//
//------------------------------------------------------------------------------    
HRESULT
CGlyphRunGeometrySink::Close()
{
    if (m_isSinkClosed)
    {
        MIL_THRX(m_hr, E_FAIL);
    }
    
    if (SUCCEEDED(m_hr))
    {
        // Write our structs out into a contiguous block of memory.
        void *pGeometryStructMem = WPFAlloc(ProcessHeap, Mt(GlyphGeometryDataStructs), m_currentOffset);
        if (pGeometryStructMem == NULL)
        {
            MIL_THRX(m_hr, E_OUTOFMEMORY);
            goto Cleanup;
        }
        
        byte *pGeometryStructs = static_cast<byte*>(pGeometryStructMem);

        UINT structIndex = 0;
        Assert(static_cast<MilPathGeometry*>(m_arrGeometryDataStructs[0]) == m_pGeometry);
        // Write the geometry struct
        memcpy(pGeometryStructs, static_cast<void*>(m_pGeometry), sizeof(MilPathGeometry));
        pGeometryStructs += sizeof(MilPathGeometry);
        structIndex += 1;
        
        // Write all figures
        while (structIndex < m_arrGeometryDataStructsCount)
        {
            MilPathFigure *pFigure = static_cast<MilPathFigure*>(m_arrGeometryDataStructs[structIndex]);
            // Write figure
            memcpy(pGeometryStructs, static_cast<void*>(pFigure), sizeof(MilPathFigure));
            pGeometryStructs += sizeof(MilPathFigure);
            structIndex += 1;

            // Write all segments for this figure
            for (UINT i = 0; i < pFigure->Count; i++)
            {
                MilSegment *pSegment = static_cast<MilSegment*>(m_arrGeometryDataStructs[structIndex]);

                switch (pSegment->Type)
                {
                    case MilSegmentType::Line:
                    {
                        MilSegmentLine *pLineSeg = static_cast<MilSegmentLine*>(pSegment);
                        // Write segment
                        memcpy(pGeometryStructs, static_cast<void*>(pLineSeg), sizeof(MilSegmentLine));
                        pGeometryStructs += sizeof(MilSegmentLine);
                        WPFFree(ProcessHeap, pLineSeg);
                        break;
                    }
                    case MilSegmentType::Bezier:
                    {
                        MilSegmentBezier *pBezSeg = static_cast<MilSegmentBezier*>(pSegment);
                        // Write segment
                        memcpy(pGeometryStructs, static_cast<void*>(pBezSeg), sizeof(MilSegmentBezier));
                        pGeometryStructs += sizeof(MilSegmentBezier);
                        WPFFree(ProcessHeap, pBezSeg);
                        break;
                    }
                    default:
                        // Arcs and quadratic beziers are not currently supported.                
                        Assert(false);
                }

                structIndex += 1;
            }

            WPFFree(ProcessHeap, pFigure);
        }
        WPFFree(ProcessHeap, m_pGeometry);
        m_pGeometry = NULL;

        // Save our flattened structs to the path geometry resource.
        m_pathGeometryData.m_cbFiguresSize = m_currentOffset;
        m_pathGeometryData.m_pFiguresData = static_cast<MilPathGeometry*>(pGeometryStructMem);

        // Set the state of this geometry sink to closed.
        m_isSinkClosed = true;
    }

Cleanup:
    // We return the first bad HR we encountered while processing the geometry.
    RRETURN(m_hr);
}


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
//      Implementastion of CShape.
//
//  $Notes:
//      IF YOU WRITE ANY CODE THAT MAY CHANGE THE GEOMETRY OF THE SHAPE (E.G.
//      ADD FIGURES, CHANGE POINTS COORDINATES), BE SURE TO HAVE THAT CODE CALL
//      INVALIDATECACHE!
//
//  $ENDTAG
//
//  Classes:
//      CShape.
//
//------------------------------------------------------------------------------


#include "precomp.hpp"

MtDefine(CShape, MILRender, "CShape");

///////////////////////////////////////////////////////////////////////////
//
// Implementation of CShape

const CShape CShape::s_emptyShape;

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShape::~CShape
//
//  Synopsis:
//      Destructor
//
//------------------------------------------------------------------------------
CShape::~CShape() 
{
    // Delete all the heap-allocated figures.
    
    for (UINT i = 0;  i < m_rgFigures.GetCount();  i++)
    {
        if (m_rgFigures[i] != &m_oCachedFigure)
        {
            delete m_rgFigures[i];
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShape::HasGaps
//
//  Synopsis:
//      Return true if this shape has gaps
//
//      Cache it upon construction, and here just check the cache
//
//------------------------------------------------------------------------------
bool
CShape::HasGaps() const
{
    for (UINT i = 0;  i < GetFigureCount();  i++)
    {
        if (m_rgFigures[i]->HasGaps())
        {
            return true;
        }
    }
    
    return false;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CShape::HasHollows
//
//  Synopsis:
//      Return true if this shape has non-fillable figures
//
//      Cache it upon construction, and here just check the cache
//
//------------------------------------------------------------------------------
bool
CShape::HasHollows() const
{
    for (UINT i = 0;  i < GetFigureCount();  i++)
    {
        if (!m_rgFigures[i]->IsFillable())
        {
            return true;
        }
    }
    
    return false;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShape::IsEmpty
//
//  Synopsis:
//      Return true if this shape is empty
//
//      Cache it upon construction, and here just check the cache
//
//------------------------------------------------------------------------------
bool
CShape::IsEmpty() const
{
    bool fIsEmpty = true;

    for (UINT i = 0;  fIsEmpty  &&  i < GetFigureCount();  i++)
    {
        fIsEmpty = m_rgFigures[i]->IsEmpty();
    }
    return fIsEmpty;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShape::AddFigure
//
//  Synopsis:
//      Add a new empty figure
//
//  Return:
//      A pointer to the new figure
//
//------------------------------------------------------------------------------
HRESULT
CShape::AddFigure(
    __deref_out_ecount(1) CFigureData *&pOutFigure)   // The newly added figure
{
    HRESULT hr = S_OK;

    CFigureData *pFigure = NULL;
    
    if (m_rgFigures.GetCount() == 0)
    {
        pFigure = &m_oCachedFigure;
        pFigure->Reset();
    }
    else
    {
        IFCOOM(pFigure = new CFigureData);
    }

    IFC(m_rgFigures.Add(pFigure));
    InvalidateCache();

    // Success
    pFigure->SetFillable(m_fFillState);
    pOutFigure = pFigure;  // Output the figure pointer.
    pFigure = NULL;        // Don't delete the figure.

Cleanup:
    if (pFigure != &m_oCachedFigure)
    {
        delete pFigure;
    }

    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CShape::AddNewFigure
//
//  Synopsis:
//      Add a figure
//
//  Return:
//      A pointer to the new figure as an IFigureBuilder
//
//  Notes:
//      This is a IShapeBuilder override
//
//------------------------------------------------------------------------------
HRESULT
CShape::AddNewFigure(
    __deref_out_ecount(1) IFigureBuilder *&pInterface)     // The newly added figure
{
    CFigureData *pFigure = NULL;
    HRESULT hr = S_OK;

    IFC(AddFigure(pFigure));
    pInterface = static_cast<IFigureBuilder*>(pFigure);

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShape::Reset
//
//  Synopsis:
//      Reset to an empty shape
//
//  Notes:
//      Release allocated space if shring=true
//
//------------------------------------------------------------------------------
void
CShape::Reset(bool shrink)
{
    m_eFillMode = MilFillMode::Winding;

    m_oCachedFigure.Reset(shrink);
    for (UINT i = 0;  i < m_rgFigures.GetCount();  i++)
    {
        if (m_rgFigures[i] != &m_oCachedFigure)
        {
            delete m_rgFigures[i];
            m_rgFigures[i] = NULL;
        }
    }
    m_rgFigures.Reset(shrink);

    InvalidateCache();
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CShape::Copy
//
//  Synopsis:
//      Copy another shape.
//
//------------------------------------------------------------------------------
HRESULT
CShape::Copy(
    __in_ecount(1) const CShape &other)     // The shape to copy
{
    HRESULT hr = S_OK;
    
    Reset();
    CFigureData *pFigure = NULL;
    UINT i;

    SetFillMode(other.GetFillMode());

    for (i = 0;  i < other.GetFigureCount();  i++)
    {
        IFC(AddFigure(pFigure));
        IFC(pFigure->Copy(other.GetFigureData(i)));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShape::AddAndTakeOwnership
//
//  Synopsis:
//      Add a given figure to the shape, taking ownership of the figure's
//      memory. (i.e. no copy is made).
//
//------------------------------------------------------------------------------
HRESULT
CShape::AddAndTakeOwnership(
    __inout_ecount(1) CFigureData *pFigure)   // In: The figure to add
{
    // Note: We don't use m_oCachedFigure here, even if this is the first
    // figure. That would cause an extra copy.

    HRESULT hr = m_rgFigures.Add(pFigure);

    Assert(pFigure);

    pFigure->SetFillable(m_fFillState);
    InvalidateCache();
    return hr;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CShape::InvalidateCache
//
//  Synopsis:
//      The shape has changed, cached data is invalid
//
//------------------------------------------------------------------------------
void 
CShape::InvalidateCache()
{
    m_wCacheState = 0;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CCompoundShapeNoRef::GetFigure
//
//  Synopsis:
//      Grab a figure from the appropriate shape.
//
//      WARNING! Currently GetFigure has a linear running time for finding the
//      figure.
//
//------------------------------------------------------------------------------
__outro_ecount(1) const IFigureData & 
CCompoundShapeNoRef::GetFigure(UINT index) const
{
    Assert(index < GetFigureCount());
    #if DBG
    Assert(m_fDbgHaveShapesBeenWalkedToPrecomputeProperties);
    #endif


    for (UINT uShapeNum = 0; uShapeNum < m_rgpShapeDatas.GetCount(); uShapeNum++)
    {
        UINT uNewShapeCount = m_rgpShapeDatas[uShapeNum]->GetFigureCount();

        if (uNewShapeCount > index)
        {
            return m_rgpShapeDatas[uShapeNum]->GetFigure(index);
        }
        else
        {
            index -= uNewShapeCount;
        }
    }

    NO_DEFAULT("Should not go through GetFigure without finding the figure");
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CCompoundShapeNoRef::SetShapeDatasNoRef
//
//  Synopsis:
//      Sets the IShapeData list.
//
//      WARNING! The shape data isn't copied or referenced.
//               It doesn't handle lifetime control of the IShapeData referenced.
//
//------------------------------------------------------------------------------
HRESULT 
CCompoundShapeNoRef::SetShapeDatasNoRef(
    __in_ecount(uNumShapes) IShapeData **rgpNewShapes,
    UINT uNumShapes
    )
{
    HRESULT hr = S_OK;

    //
    // This function should only be called once.
    //
    #if DBG
    Assert(m_fDbgHaveShapesBeenWalkedToPrecomputeProperties == false);
    #endif
    Assert(m_fCachedBoundsSet == false);
    Assert(m_rgpShapeDatas.GetCount() == 0);

    for (UINT i = 0; i < uNumShapes; i++)
    {
        if (rgpNewShapes[i])
        {
            //
            // Future Consideration:  Could accumulate cached info here.
            //
            // We can track all of our precomputed properties here while
            // walking through the shapes for the first time.  This could be a
            // performance win, but we just don't care at this point since
            // we're only working with <= 4 shapes
            //
            IFC(m_rgpShapeDatas.Add(rgpNewShapes[i]));
        }
    }

    IFC(WalkShapesAndPrecomputeProperties());

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CCompoundShapeNoRef::WalkShapesAndPrecomputeProperties
//
//  Synopsis:
//      Precompute data that could be asked of the shape.
//
//------------------------------------------------------------------------------
HRESULT
CCompoundShapeNoRef::WalkShapesAndPrecomputeProperties()
{
    HRESULT hr = S_OK;

    m_uFigureCount = GetFigureCountInternal();
    m_fHasGaps = HasGapsInternal();
    m_fHasHollows = HasHollowsInternal();
    m_fIsEmpty = IsEmptyInternal();
    m_fIsAxisAlignedRectangle = IsAxisAlignedRectangleInternal();
    m_fIsARegion = IsARegionInternal();

    //
    // Future Consideration:  Could wait to calculate bounds until needed.
    //
    IFC(GetFillBoundsInternal(&m_rcCachedBounds));
    m_fCachedBoundsSet = true;

#if DBG
    m_fDbgHaveShapesBeenWalkedToPrecomputeProperties = true;
#endif

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CCompoundShapeNoRef::WalkShapesAndPrecomputeProperties
//
//  Synopsis:
//      Precompute data that could be asked of the shape.
//
//------------------------------------------------------------------------------
UINT
CCompoundShapeNoRef::GetFigureCountInternal() const
{
    UINT uCount = 0;

    for (UINT uShapeNum = 0; uShapeNum < m_rgpShapeDatas.GetCount(); uShapeNum++)
    {
        uCount += m_rgpShapeDatas[uShapeNum]->GetFigureCount();
    }

    return uCount;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CCompoundShapeNoRef::GetBoundsShapeSpaceInternal
//
//  Synopsis:
//      Gets the combined bounds of the shapes.
//

HRESULT
CCompoundShapeNoRef::GetFillBoundsInternal(
    __out_ecount(1) MilRectF *prcBoundsShapeSpace
    ) const
{
    HRESULT hr = S_OK;

    CMilRectF rcOverallBoundsShapeSpace = CMilRectF::sc_rcEmpty;

    *prcBoundsShapeSpace = CMilRectF::sc_rcEmpty;

    for (UINT uShapeNum = 0; uShapeNum < m_rgpShapeDatas.GetCount(); uShapeNum++)
    {
        CMilRectF rcShapeBounds;

        IFC(m_rgpShapeDatas[uShapeNum]->GetTightBounds(rcShapeBounds));

        rcOverallBoundsShapeSpace.Union(rcShapeBounds);
    }

    *prcBoundsShapeSpace = rcOverallBoundsShapeSpace;

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CCompoundShapeNoRef::HasGapsInternal
//
//  Synopsis:
//      Calculate if any of the shapes referenced contain gaps.
//
//------------------------------------------------------------------------------
bool
CCompoundShapeNoRef::HasGapsInternal() const
{
    bool fHasGaps = false;

    for (UINT uShapeNum = 0; uShapeNum < m_rgpShapeDatas.GetCount(); uShapeNum++)
    {
        if(m_rgpShapeDatas[uShapeNum]->HasGaps())
        {
            fHasGaps = true;
            break;
        }

    }

    return fHasGaps;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CCompoundShapeNoRef::HasHollowsInternal
//
//  Synopsis:
//      Calculate if any of the shapes referenced contain hollows.
//
//------------------------------------------------------------------------------
bool
CCompoundShapeNoRef::HasHollowsInternal() const
{
    bool fHasHollows = false;

    for (UINT uShapeNum = 0; uShapeNum < m_rgpShapeDatas.GetCount(); uShapeNum++)
    {
        if(m_rgpShapeDatas[uShapeNum]->HasHollows())
        {
            fHasHollows = true;
            break;
        }

    }

    return fHasHollows;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CCompoundShapeNoRef::IsEmptyInternal
//
//  Synopsis:
//      Calculate if any of the shapes are not empty.
//
//------------------------------------------------------------------------------
bool
CCompoundShapeNoRef::IsEmptyInternal() const
{
    bool fIsEmpty = true;

    if (m_rgpShapeDatas.GetCount() > 0)
    {
        for (UINT uShapeNum = 0; uShapeNum < m_rgpShapeDatas.GetCount(); uShapeNum++)
        {
            if(!m_rgpShapeDatas[uShapeNum]->IsEmpty())
            {
                fIsEmpty = false;
                break;
            }
    
        }
    }

    return fIsEmpty;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CCompoundShapeNoRef::IsARegionInternal
//
//  Synopsis:
//      Calculate if all of the shapes referenced are regions.
//
//------------------------------------------------------------------------------
bool
CCompoundShapeNoRef::IsARegionInternal() const
{
    bool fIsARegion = true;

    for (UINT uShapeNum = 0; uShapeNum < m_rgpShapeDatas.GetCount(); uShapeNum++)
    {
        if(!m_rgpShapeDatas[uShapeNum]->IsARegion())
        {
            fIsARegion = false;
            break;
        }

    }

    return fIsARegion;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CCompoundShapeNoRef::IsAxisAlignedRectangleInternal
//
//  Synopsis:
//      Calculates if all the shapes referenced are AxisAlignedRectangles
//
//------------------------------------------------------------------------------
bool 
CCompoundShapeNoRef::IsAxisAlignedRectangleInternal() const
{
    bool fIsAxisAlignedRectangle = false;

    if (m_rgpShapeDatas.GetCount() == 1 && m_rgpShapeDatas[0]->IsAxisAlignedRectangle())
    {
        fIsAxisAlignedRectangle = true;
    }

    return fIsAxisAlignedRectangle;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShape::AddCopyOf
//
//  Synopsis:
//      Add a new figure that is a copy of a given figure
//
//------------------------------------------------------------------------------

HRESULT
CShape::AddCopyOf(
    __in_ecount(1) const CFigure &figure)  // figure to copy
{
    HRESULT hr = S_OK;
    CFigureData *pNew = NULL;

    // Add an empty figure and copy
    IFC(AddFigure(pNew));
    IFC(pNew->SetFrom(figure.GetData()));

    // Set the fillable flag
    pNew->SetFillable(GetFillState());

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CShape::ConstructFromGpPath
//
//  Synopsis:
//      Construct from old-fashioned GpPath points & types arrays
//
//------------------------------------------------------------------------------
HRESULT
CShape::ConstructFromGpPath(
    IN MilFillMode::Enum      eMode,
        // FillMode
    IN INT              count,
        // Number of points & types
    __in_ecount(count) const MilPoint2F  *pPoints,
        // Path points
    __in_ecount(count) const BYTE       *pTypes)
        // Path types
{
    HRESULT hr = S_OK;

    if (count < 1)
    {
        IFC(E_INVALIDARG);
    }

    Assert(pPoints);
    Assert(pTypes);

    Reset();
    Assert(MilFillMode::Winding == m_eFillMode); // The default after Reset()
    if (MilFillMode::Alternate == eMode)
    {
        m_eFillMode = MilFillMode::Alternate;
    }

    CFigureData *pFigure = NULL;
    BOOL fClosed = FALSE;

    if ((pTypes[0] & PathPointTypePathTypeMask) != PathPointTypeStart)
    {
        // Type array is malformed
        IFC(E_INVALIDARG);
    }

    for (int i = 0;  i < count;  /* i incremented inside the loop*/)
    {
        BYTE bType = pTypes[i];
        switch (bType & PathPointTypePathTypeMask)
        {
        case PathPointTypeStart:
            // Close the current figure if necessary
            if (fClosed)
            {
                IFC(pFigure->Close());
            }

            // Start a new figure with the current point
            IFC(AddFigure(pFigure));
            Assert(pFigure); // Otherwise AddFigure should have failed
            IFC(pFigure->StartAtPtF(pPoints[i++]));
            break;

        case PathPointTypeLine:
            Assert(pFigure); // should have been allocated in the PathPointTypeStart case
            IFC(pFigure->LineToPtF(pPoints[i++]));
            break;
    
        case PathPointTypeBezier:
            Assert(pFigure); // should have been allocated in the PathPointTypeStart case
            if (i + 2 <  count)
            {
                IFC(pFigure->BezierToPtF(pPoints[i],
                                         pPoints[i+1],
                                         pPoints[i+2]));
                i += 3;
            }
            else
            {
                IFC(E_INVALIDARG);
            }
            break;

        default:
            IFC(E_INVALIDARG);
            break;
        }
        fClosed = IsClosedType(bType);
    }
    // Close the last figure if so prescribed
    if (fClosed)
    {
        Assert(pFigure); // should have been allocated in the PathPointTypeStart case
        IFC(pFigure->Close());
    }

Cleanup:
    if (FAILED(hr))
    {
        Reset();
    }

    RRETURN(hr);
}                                                                                                   

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShape::GetModifyFigure
//
//  Synopsis:
//      Get the figure with a given index for modification
//
//------------------------------------------------------------------------------
__out_ecount(1) CFigureData &
CShape::GetModifyFigure(
    IN UINT index)          // Index
{
    Assert(index < GetFigureCount());
    
    InvalidateCache(); // Because we are going to modify this figure
    return *m_rgFigures[index];
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CShape::Transform
//
//  Synopsis:
//      Transform this shape
//
//------------------------------------------------------------------------------
void
CShape::Transform(
    __in_ecount_opt(1) const CBaseMatrix *pMatrix)  // In: Transformation matrix (NULL OK)
{
    if (NULL != pMatrix  &&  !pMatrix->IsIdentity())
    {
        for (UINT i = 0;  i < GetFigureCount();  i++)
        {
            GetModifyFigure(i).Transform(*pMatrix);
        }
    }
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CShape::Abort
//
//  Synopsis:
//      Abort the construction of the latest figure.
//
//  Notes:
//      To be used judiciously, upon construction failure only.
//
//------------------------------------------------------------------------------
void
CShape::Abort()
{
    Assert(m_rgFigures.GetCount() > 0);
        
    if (m_rgFigures.Last() != &m_oCachedFigure)
    {
        delete m_rgFigures.Last();
    }
    else
    {
        Assert(m_rgFigures.GetCount() == 1);
    }

    m_rgFigures.DecrementCount();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShape::AddRect
//
//  Synopsis:
//      Add a possibly transformed rectangle
//
//------------------------------------------------------------------------------
HRESULT
CShape::AddRect(
    __in_ecount(1) const MilRectF &rect,         // The rectangle
    __in_ecount_opt(1) const CMILMatrix *pMatrix)   // Optional: Transformation matrix (NULL OK)
{
    HRESULT hr = S_OK;
    CFigureData *pFigure = NULL;

    IFC(AddFigure(pFigure));
    Assert(pFigure); // Otherwise AddFigure should have failed

    IFC(pFigure->InitAsRectangle(rect));

    if (pMatrix)
    {
        pFigure->Transform(*pMatrix);
    }

    pFigure = NULL;  // Success

Cleanup:
    if (pFigure)
    {
        Abort();
    }
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShape::AddFigureFromRawData
//
//  Synopsis:
//      Add a figure constructed from raw points and segment-types +
//      transformation
//
//------------------------------------------------------------------------------
HRESULT
CShape::AddFigureFromRawData(
    __in UINT cPoints,
        // Point count
    __in UINT cSegments,
        // Segment count
    __in_ecount(cPoints) MilPoint2D *pPoints,
        // Points
    __in_ecount(cSegments) byte *pTypes,
        // Types
    __in_ecount_opt(1) CMILMatrix *pMatrix)
        // A transformation to apply to the points
{
    HRESULT hr = S_OK;
    CFigureData *pFigure = NULL;

    IFC(AddFigure(pFigure));
    Assert(pFigure);            // Otherwise AddFigure should have failed

    IFC(pFigure->InitFromRawData(cPoints, cSegments, pPoints, pTypes, pMatrix));

    pFigure = NULL;  // Success

Cleanup:
    if (pFigure)
    {
        Abort();
    }
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CShape::GetCachedBoundsCore
//
//  Synopsis:
//      Get the cached bounds if they exist
//
//  Returns:
//      true if bounds have previously been cached.
//
//------------------------------------------------------------------------------
bool
CShape::GetCachedBoundsCore(
    __out_ecount(1) MilRectF &rect)  const // The cached bounds, set only if valid
{
    bool fCached = ((m_wCacheState & SHAPE_BOX_VALID) != 0);
    if (fCached)
    {
        // Update the cache
        rect = m_cachedBounds;
    }

    return fCached;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShape::SetCachedBounds
//
//  Synopsis:
//      Set the cached bounds
//
//------------------------------------------------------------------------------
void
CShape::SetCachedBounds(
    __in_ecount(1) MilRectF const &rect) const // The bounding box to cache
{
    m_cachedBounds = rect;
    m_wCacheState |= SHAPE_BOX_VALID;
}
///////////////////////////////////////////////////////////////////////////
//
// Implementation of CShape

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Add a line as a new figure to this shape
//
//------------------------------------------------------------------------------
HRESULT
CShape::AddLine(
    REAL x1, REAL y1,   // In: The line's start point 
    REAL x2, REAL y2)   // In: Line's end point
{
    HRESULT hr = S_OK;
    CFigureData *pFigure = NULL;

    IFC(AddFigure(pFigure));

    Assert(pFigure); // Otherwise AddFigure should have failed
    
    IFC(pFigure->StartAt(x1, y1));
    IFC(pFigure->LineTo(x2, y2));

    pFigure = NULL;  // Success

Cleanup:
    if (pFigure)
    {
        Abort();
    }
    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Add a polygonal line as a new figure to this shape
//
//------------------------------------------------------------------------------
HRESULT
CShape::AddLines(
    __in_ecount(count) const MilPoint2F *rgPoints,
        // In: Array of points
    INT count)
        // In: Number of points
{
    HRESULT hr = S_OK;
    CFigureData *pFigure = NULL;

    if (!rgPoints  ||  count < 2)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    IFC(AddFigure(pFigure));

    Assert(pFigure); // Otherwise AddFigure should have failed
    
    IFC(pFigure->StartAtPtF(rgPoints[0]));
    IFC(pFigure->LinesTo(rgPoints + 1, count - 1));

    pFigure = NULL;  // Success

Cleanup:
    if (pFigure)
    {
        Abort();
    }
    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Add a Bezier curve as a new figure to this shape
//
//------------------------------------------------------------------------------
HRESULT
CShape::AddBezier(
    REAL x1, REAL y1,   // In: First Bezier point
    REAL x2, REAL y2,   // In: Second Bezier point
    REAL x3, REAL y3,   // In: Third Bezier point
    REAL x4, REAL y4)   // In: Fourth Bezier point
{
    HRESULT hr = S_OK;
    CFigureData *pFigure = NULL;

    IFC(AddFigure(pFigure));

    Assert(pFigure); // Otherwise AddFigure should have failed
    
    IFC(pFigure->StartAt(x1, y1));
    IFC(pFigure->BezierTo(x2, y2, x3, y3, x4, y4));

    pFigure = NULL;  // Success

Cleanup:
    if (pFigure)
    {
        Abort();
    }
    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Add a composite Bezier curve as a new figure to this shape
//
//------------------------------------------------------------------------------
HRESULT
CShape::AddBeziers(
    __in_ecount(count) const MilPoint2F *rgPoints,
        // In: Array of points
    INT count)
        // In: Number of points
{
    HRESULT hr = S_OK;
    CFigureData *pFigure = NULL;

    if (!rgPoints ||  count < 3)
    {
        IFC(E_INVALIDARG);
    }

    IFC(AddFigure(pFigure));

    Assert(pFigure); // Otherwise AddFigure should have failed
    
    IFC(pFigure->StartAtPtF(rgPoints[0]));
    IFC(pFigure->BeziersTo(rgPoints+1, count - 1));

    pFigure = NULL;  // Success

Cleanup:
    if (pFigure)
    {
        Abort();
    }
    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Add a rectangle as a new figure to this shape
//
//------------------------------------------------------------------------------
HRESULT
CShape::AddRectangle(
    REAL x, REAL y,  // In: The rectangle's corner
    REAL width,      // In: Defining rectangle's width
    REAL height)     // In: Defining rectangle's height
{
    CMilRectF rect(x, y, width, height, XYWH_Parameters);
    return AddRect(rect);
}
//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Add a bunch of rectangles as new figures to this shape
//
//------------------------------------------------------------------------------
HRESULT
CShape::AddRectangles(
    __in_ecount(count) const MilPointAndSizeF *rgRects,  // In: Array of rectangles
    INT count)               // In: Number of points   
{
    HRESULT hr = S_OK;

    if (!rgRects)
    {
        IFC(E_INVALIDARG);
    }

    for (INT i = 0;  SUCCEEDED(hr) &&  i < count;  i++)
    {
        MIL_THR(AddRect(CMilRectF(rgRects[i])));
    }
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Add an ellipse with center/radii parameters as a new figure to this
//      shape
//
//------------------------------------------------------------------------------
HRESULT 
CShape::AddEllipse(     
    REAL centerX, REAL centerY, // The center of the ellipse
    REAL radiusX,               // The X-radius of the ellipse
    REAL radiusY,               // The Y-radius of the ellipse
    CR crParameters
    )
{
    HRESULT hr = S_OK;
    CFigureData *pFigure = NULL;

    UNREFERENCED_PARAMETER(crParameters);
  
    IFC(AddFigure(pFigure));

    Assert(pFigure); // Otherwise AddFigure should have failed

    // If the radii are negative, we use their absolute value
    radiusX = fabs(radiusX);
    radiusY = fabs(radiusY);

    IFC(pFigure->InitAsEllipse(centerX, centerY, radiusX, radiusY));

    pFigure = NULL;  // Success

Cleanup:
    if (pFigure)
    {
        Abort();
    }
    
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Add an ellipse with origin/width/height parameters as a new figure to
//      this shape
//
//------------------------------------------------------------------------------
HRESULT 
CShape::AddEllipse(     // The following parameters refer to the rectangular bounds of the ellipse:
    REAL x, REAL y,     // The upper left hand corner.
    REAL width,         // Rectangle width
    REAL height,         // Rectangle height
    OWH owhParameters
    )
{
    HRESULT hr = S_OK;

    UNREFERENCED_PARAMETER(owhParameters);

    // If the radii are negative, we use their absolute value
    REAL radiusX = fabs(width*.5f);
    REAL radiusY = fabs(height*.5f);
    REAL centerX = x + radiusX;
    REAL centerY = y + radiusY;

    IFC(AddEllipse(centerX, centerY, radiusX, radiusX, CR_Parameters));

Cleanup:
    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShape::AddRoundedRectangle
//
//  Synopsis:
//      Add a rounded rectangle as a new figure to this shape
//
//------------------------------------------------------------------------------

HRESULT
CShape::AddRoundedRectangle(
    __in_ecount(1) const MilPointAndSizeF &rect,
        // The bounding rectangle
    IN REAL radiusX,
        // The X-radius of the corner (elliptical arc)
    IN REAL radiusY
        // The Y-radius of the corner (elliptical arc)
    )
{
    HRESULT hr = S_OK;
    CFigureData *pFigure = NULL;

    if (!IsRectEmptyOrInvalid(&rect)
            && !_isnan(radiusX) && !_isnan(radiusY)) // Don't add a figure for invalid dimensions.
    {
        IFC(AddFigure(pFigure));
        Assert(pFigure); // Otherwise AddFigure should have failed

        if ((radiusX == 0.0) ||
            (radiusY == 0.0))
        {
            // If either radii is zero, then this rounded rectangle 
            // is equivalent to a rectangle.
            IFC(pFigure->InitAsRectangle(CMilRectF(rect)));
        }
        else
        {
            // If the radii are negative, we use their absolute value
            radiusX = fabs(radiusX);
            radiusY = fabs(radiusY);        

            // Initialize this rounded rectangle with positive radii.
            IFC(pFigure->InitAsRoundedRectangle(CMilRectF(rect), radiusX, radiusY));
        }

        pFigure = NULL;  // Success
    }

Cleanup:
    if (pFigure)
    {
        Abort();
    }
    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShape::AddBeveledRectangle
//
//  Synopsis:
//      Add a beveled rectangle as a new figure to this shape
//
//------------------------------------------------------------------------------

HRESULT
CShape::AddBeveledRectangle(
    __in_ecount(1) const MilPointAndSizeF &rect,
        // The bounding rectangle
    IN REAL rBevelOffset
        // The bevel offset
    )
{
    HRESULT hr = S_OK;
    CFigureData *pFigure = NULL;

    if (!IsRectEmptyOrInvalid(&rect)
            && !_isnan(rBevelOffset)) // Don't add a figure for invalid dimensions.
    {
        IFC(AddFigure(pFigure));
        Assert(pFigure); // Otherwise AddFigure should have failed

        if (rBevelOffset == 0.0f)
        {
            // If the bevel offset is 0, then this beveled rectangle 
            // is equivalent to a rectangle.
            IFC(pFigure->InitAsRectangle(CMilRectF(rect)));
        }
        else
        {
            // Initialize this rounded rectangle with positive radii.
            IFC(pFigure->InitAsBeveledRectangle(rect, rBevelOffset));
        }

        pFigure = NULL;  // Success
    }

Cleanup:
    if (pFigure)
    {
        Abort();
    }
    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Add a polygon as a new figure to this shape
//
//------------------------------------------------------------------------------
HRESULT
CShape::AddPolygon(
    __in_ecount(count) const MilPoint2F *rgPoints,
        // In: Array of points
    INT count)
        // In: Number of points
{
    HRESULT hr = S_OK;
    CFigureData *pFigure = NULL;

    if (!rgPoints || count < 3)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    IFC(AddFigure(pFigure));

    Assert(pFigure); // Otherwise AddFigure should have failed

    IFC(pFigure->StartAtPtF(rgPoints[0]));
    IFC(pFigure->LinesTo(rgPoints+1, count-1));
    IFC(pFigure->Close());

    pFigure = NULL;  // Success

Cleanup:
    if (pFigure)
    {
        Abort();
    }
    
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Cmbine this shape with another shape
//
//------------------------------------------------------------------------------
HRESULT
CShape::AddShape(
    __in_ecount(1) const CShape &shape)   // The shape to add
{
    HRESULT hr = S_OK;

    CFigureData *pFigure = NULL;

    for (UINT i = 0;  i < shape.GetFigureCount();  i++)
    {
        IFC(AddFigure(pFigure));

        Assert(pFigure);  // Otherwise AddFigure should have failed
        
        IFC(pFigure->Copy(shape.GetFigureData(i)));
    }

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Cmbine this shape with another shape
//
//------------------------------------------------------------------------------
HRESULT
CShape::AddShapeData(
    __in_ecount(1) const IShapeData    &shape,      // The shape to add
    __in_ecount_opt(1) const CMILMatrix *pMatrix)   // Transformation to apply to the input
{
    HRESULT hr = S_OK;

    CFigureData *pFigure = NULL;

    for (UINT i = 0;  i < shape.GetFigureCount();  i++)
    {
        IFC(AddFigure(pFigure));

        Assert(pFigure);  // Otherwise AddFigure should have failed
        
        IFC(pFigure->SetFrom(shape.GetFigure(i), pMatrix));
    }

Cleanup:
    RRETURN(hr);
}








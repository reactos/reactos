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
//  $ENDTAG
//
//  Description
//      Implementation of CPlainPen class
//

#include "precomp.hpp"

const BYTE PENPLAIN_HAS_START_SHAPE = 0x1;
const BYTE PENPLAIN_HAS_END_SHAPE =   0x2;

MtDefine(CPlainPen, MILRender, "CPlainPen");



//+-----------------------------------------------------------------------------
//
//  Member:
//      CPPenGeometry::GetInflateFactor
//
//  Synopsis:
//      Get the factor by which the pen width may be inflated due to corners &
//      caps
//
//------------------------------------------------------------------------------
REAL 
CPenGeometry::GetInflateFactor() const
{
    // The pen may inflates the stroked shape by its width/height
    REAL rExtents = 1;

    // Include the potential expansion caused by mitered corners
    if ((MilLineJoin::Miter == m_eJoin) || (MilLineJoin::MiterClipped == m_eJoin))
    {
        Assert(m_rMiterLimit >= 1);
        rExtents = static_cast<REAL>(m_rMiterLimit * SQRT_2);
    }
    else if(MilPenCap::Square == m_eStartCap || MilPenCap::Square == m_eEndCap
            || MilPenCap::Square == m_eDashCap)
    {
        // Include the potential diagonal of a cap.
        rExtents = static_cast<REAL>(SQRT_2);
    }

    return rExtents;
}
//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Default constructor
//
//------------------------------------------------------------------------------
CPlainPen::CPlainPen()
   :m_eDashStyle(MilDashStyle::Solid),
    m_rDashOffset(0),
    m_eStartShapeType(MilLineShape::None),
    m_pStartShape(NULL),
    m_eEndShapeType(MilLineShape::None),
    m_pEndShape(NULL)
{
}
//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Copy constructor (private)
//
//------------------------------------------------------------------------------
CPlainPen::CPlainPen(
    __in_ecount(1) const CPlainPen &other)  // In: The other pen to copy
   :m_oGeom(other.m_oGeom),
    m_eDashStyle(MilDashStyle::Solid),
    m_rDashOffset(other.m_rDashOffset),
    m_eStartShapeType(MilLineShape::None),
    m_pStartShape(NULL),
    m_eEndShapeType(MilLineShape::None),
    m_pEndShape(NULL)
{
    /* This constructor does NOT copy the members whose allocation on the
    heap may fail - dashes, custom shapes and compound array.  Until the
    dashes are successfuly copied the dash style is set to solid. */
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CPlainPen::~CPlainPen
//
//  Synopsis:
//      Destructor
//
//------------------------------------------------------------------------------
CPlainPen::~CPlainPen()
{
#ifdef LINE_SHAPES_ENABLED
    delete m_pStartShape;
    delete m_pEndShape;
#endif
}
//+-----------------------------------------------------------------------------
//
//  Function:
//      Clone
//
//  Synopsis:
//
//------------------------------------------------------------------------------
HRESULT
CPlainPen::Clone(
    __deref_out_ecount(1) CPlainPen *&pClone) const
        // Out: The clone
{
    pClone = NULL;
    HRESULT hr = S_OK;
    CPlainPen * pCopy =  new CPlainPen(*this);
    if (!pCopy)
    {
        hr = E_OUTOFMEMORY;
        GOTO(exit);
    }

    // Copy dash style
    if (MilDashStyle::Solid != m_eDashStyle)
    {
        Assert(m_rgDashes.GetCount() > 1);
        pCopy->m_eDashStyle = m_eDashStyle;
        if (FAILED(hr = pCopy->m_rgDashes.Copy(m_rgDashes)))
        {
            GOTO(exit);
        }
    }

#ifdef COMPOUND_PEN_IMPLEMENTED
    // Copy compound array
    if (m_rgCompound.GetCount() > 0)
    {

        if (FAILED(hr = pCopy->m_rgCompound.Copy(m_rgCompound)))
        {
            hr = E_OUTOFMEMORY;
            GOTO(exit);
        }
    }
        
    // Copy the start and end shapes
    if (m_pStartShape)
    {
        if (FAILED(hr = m_pStartShape->Clone(pCopy->m_pStartShape)))
        {
            hr = E_OUTOFMEMORY;
            GOTO(exit);
        }
        pCopy->m_eStartShapeType = m_eStartShapeType;
    }
#endif // COMPOUND_PEN_IMPLEMENTED
        
#ifdef LINE_SHAPES_ENABLED
     if(m_pEndShape)
    {
        if (FAILED(hr = m_pEndShape->Clone(pCopy->m_pEndShape)))
        {
            hr = E_OUTOFMEMORY;
            GOTO(exit);
        }
        pCopy->m_eEndShapeType = m_eEndShapeType;
    }
#endif // LINE_SHAPES_ENABLED
 
    // Cloning suceeded;
    pClone = pCopy;
    pCopy = NULL;
exit:
    delete pCopy;   // Should be NULL if SUCCEEDED(hr)
    return hr;
}
//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Set dash style and array
//
//------------------------------------------------------------------------------
HRESULT
CPlainPen::SetDashStyle(
    IN MilDashStyle::Enum style) // Dash style, cannot be custom
{
    HRESULT hr = S_OK;
    REAL    dashes[6];
    INT     count=0;

    switch(style)
    {
    case MilDashStyle::Solid:
        break;

    case MilDashStyle::Dash:
        count = 2;
        dashes[0] = 2;   // a dash
        dashes[1] = 2;   // a space
        break;

    case MilDashStyle::Dot:
        count = 2;
        dashes[0] = 0;   // a dot
        dashes[1] = 2;   // a space
        break;

    case MilDashStyle::DashDot:
        count = 4;
        dashes[0] = 2;   // a dash
        dashes[1] = 2;   // a space
        dashes[2] = 0;   // a dot
        dashes[3] = 2;   // a space
        break;

    case MilDashStyle::DashDotDot:
        count = 6;
        dashes[0] = 2;   // a dash
        dashes[1] = 2;   // a space
        dashes[2] = 0;   // a dot
        dashes[3] = 2;   // a space
        dashes[4] = 0;   // a dot
        dashes[5] = 2;   // a space
        break;
    
    default:
        // The dash style must be one of the predefined ones.  Custom dash
        // style can only be set internally when the user sets the dash 
        // array
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (count > 0)
    {
        // Set Dash Offset to be at half the first dash, so that there will
        // always be a solid dash at the path's start, end and corners.
        m_rDashOffset = dashes[0] / 2;

        m_rgDashes.Reset(FALSE);
        IFC(m_rgDashes.AddMultipleAndSet(dashes, count));
    }

    // Set the style
    m_eDashStyle = style;

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CPlainPen::SetDashArray
//
//  Synopsis:
//      Set the dash array to the input array if valid
//
//------------------------------------------------------------------------------
HRESULT
CPlainPen::SetDashArray(
    __in_ecount_opt(count) const REAL *dashes,
        // The arrray of dash starts/ends
    IN INT   count)
        // Size of the array
{
    INT  i = 0;
    HRESULT hr = S_OK;

    // The dash array must be non-null, with a positive even number of entries
    if (!dashes  ||  count <= 1  ||  ((count & 1) != 0))
    {
        goto Cleanup;
    }

    // Rreserve space
    IFC(m_rgDashes.ReserveSpace(count, true /* exact */));

    // Add the dashes and gaps, making sure they are nonnegative
    for (i = 0;  i < count;  i++)
    {
        IFC(m_rgDashes.Add(fabs(dashes[i])));
    }

    m_eDashStyle = MilDashStyle::Custom;

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Get the dash array
//
//------------------------------------------------------------------------------
HRESULT
CPlainPen::GetDashArray(
    IN UINT count,
        // Output buffer size
    __out_ecount(count) /* _part(count,m_rgDashes.Count) */ REAL *dashes)
        // The arrray of dash starts/ends
{
    HRESULT hr = S_OK;
    UINT dashCount = m_rgDashes.GetCount();

    if (!dashes  ||  dashCount > count)
    {
        IFC(E_INVALIDARG);
    }

    for (UINT i = 0;  i < dashCount;  i++)
    {
        dashes[i] = m_rgDashes[i];
    }
Cleanup:
    RRETURN(hr);
}
#ifdef COMPOUND_PEN_IMPLEMENTED
//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Set compound array
//
//------------------------------------------------------------------------------
HRESULT
CPlainPen::SetCompoundArray(
    __in_ecount(count) const REAL *array,
        // In: Compound array values to set
    INT count)
        // In: Number of entries in the array
{
    HRESULT hr = S_OK;

    // count must be a positive even number.
    if (array == NULL || count <= 0 || (count & 0x01))
    {
        IFC(E_INVALIDARG);
    }

    // Make sure the all elements are monitonically increasing
    // and its values are between 0 and 1.

    REAL last, next;

    last = array[0];
    if (last < 0.0f || last > 1.0f)
    {
        IFC(E_INVALIDARG);
    }

    INT i = 1;
    while (i < count)
    {
        next = array[i++];
        if (next < last || next > 1.0f)
        {
            IFC(E_INVALIDARG);
        }
        last = next;
    }

    // Set the array
    m_rgCompound.Reset(FALSE);
    IFC(m_rgCompound.AddMultipleAndSet(array, count));

Cleanup:
    return hr;
}
//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Get the dash array
//
//------------------------------------------------------------------------------
HRESULT
CPlainPen::GetCompoundArray(
    UINT count,
        // In: Output buffer size
    __out_ecount(count) /* _part(count, m_rgCompound.Count) */ REAL *array)
        // Out: The Output arrray
{
    HRESULT hr = S_OK;

    Assert(array);

    if (!array  ||  count > m_rgCompound.GetCount())
    {
        IFC(E_INVALIDARG);
    }

    for (UINT i = 0;  i < m_rgCompound.GetCount();  i++)
    {
        array[i] = m_rgCompound[i];
    }
Cleanup:
    RRETURN(hr);
}
#endif // COMPOUND_PEN_IMPLEMENTED

 //+----------------------------------------------------------------------------
 //
 // Synopsis:
 //     Set a custom line shape
 //
 //-----------------------------------------------------------------------------
HRESULT
CPlainPen::SetCustomLineShape(
    __in_ecount(1) const CShape &shape,
        // The line shape's geometry        
    IN FLOAT        rInset,
        // The amount the path will be trimmed
    IN FLOAT        rAnchorDistance,
        // The -Y coordinate of the anchor point 
    IN bool         fFill,
        // Fill the shape if true 
    IN bool         fStroke,
        // Stroke the shape if true
    __in_ecount_opt(1) CPlainPen    *pPen,
        // An overriding pen for strokes  (NULL OK)
    __deref_out_ecount(1) CLineShape *&pDest,
        // The destination (m_pStartShape of m_pEndShape)
    __out_ecount(1) MilLineShape::Enum &eTypeToSet)     // m_eStartShapeType or m_pEndShapeType to set to Custom
{
#ifdef LINE_SHAPES_ENABLED
    HRESULT hr;
    CLineShape *pLIneShape = NULL;

    if (rInset < 0)
    {
        rInset = 0;
    }

    if (rAnchorDistance < 0)
    {
        rAnchorDistance = 0;
    }

    IFCOOM(pLIneShape = new CLineShape(rInset, rAnchorDistance, fFill, fStroke, pPen));
    IFC(pLIneShape->SetPath(shape));

    pDest = pLIneShape;
    pLIneShape = NULL;
    eTypeToSet = MilLineShape::Custom;

Cleanup:
    delete pLIneShape;
    RRETURN(hr);
#else
    RRETURN(E_NOTIMPL);
#endif // LINE_SHAPES_ENABLED
}
////////////////////////////////////////////////////////////////////////////
//      Canned line shape construction
#ifdef LINE_SHAPES_ENABLED

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a reference CLineShape representing a MilLineShape::Arrow. This
//      is an equilateral triangle with edge equal to 2. This means that the
//      scaling will create a 2xStrokeWidth cap edge length.
//
//------------------------------------------------------------------------------
HRESULT
SetArrowLineShape(
    __deref_out_ecount(1) CLineShape *&pDest)
        // The destination (m_pStartShape or m_pEndShape)
{
    const REAL root3 = 1.732050808f; // the square root of 3
    HRESULT hr;
    CLineShape *pNew = NULL;
    
    // Anti-clockwise definition of an equilateral triangle of side length 2.0f
    // with a vertex on the origin and axis extending along the negative
    // y axis.
     
    const MilPoint2F points[3] = {
        {  0.0f, 0.0f },
        { -1.0f, -root3 },
        {  1.0f, -root3 }
    };
    
    // Create the line shape as a fill only triangle
    IFCOOM(pNew = new CLineShape(1, 1, true, false, NULL));
    IFC(pNew->AddPolygon(points, 3));

    // Transfer ownership
    pDest = pNew;
    pNew = NULL;

Cleanup:
    delete pNew; // In case we failed
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a reference CLineShape representing a MilLineShape::Diamond.
//      This is a square centered on the end point of the path with it's
//      diagonal along the axis of the spine.
//
//------------------------------------------------------------------------------
HRESULT
SetDiamondLineShape(
    __deref_out_ecount(1) CLineShape *&pDest)
        // The destination (m_pStartShape or m_pEndShape)
{
    HRESULT hr;
    CLineShape *pNew = NULL;

    // Anti-clockwise definition of a square of diagonal size 2.0f
    // with the center on the origin and axis extending along the negative
    // y axis.
     
    const MilPoint2F points[4] = {
        {  0.0f,  1.0f },
        { -1.0f,  0.0f },
        {  0.0f, -1.0f },
        {  1.0f,  0.0f }
    };
    
    // Create the line shape as a fill only triangle
    IFCOOM(pNew = new CLineShape(1, 1, true, false, NULL));
    IFC(pNew->AddPolygon(points, 4));

    // Transfer ownership
    pDest = pNew;
    pNew = NULL;

Cleanup:
    delete pNew; // In case we failed
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a reference CLineShape representing a MilLineShape::Square. This
//      is a square that has a 2 unit long diagonal and is centered on the end
//      point of the path.
//
//------------------------------------------------------------------------------
HRESULT
SetSquareLineShape(
    __deref_out_ecount(1) CLineShape *&pDest)
        // The destination (m_pStartShape or m_pEndShape)
{
    HRESULT hr;
    CLineShape *pNew = NULL;

    const REAL halfRoot2 = 0.7071068f;
    
    const MilPoint2F points[4] = {
        { -halfRoot2, -halfRoot2 },
        {  halfRoot2, -halfRoot2 },
        {  halfRoot2,  halfRoot2 },
        { -halfRoot2,  halfRoot2 }
    };
    
    // Create the line shape as a fill only triangle
    IFCOOM(pNew = new CLineShape(0, 0, true, false, NULL));
    IFC(pNew->AddPolygon(points, 4));

    // Transfer ownership
    pDest = pNew;
    pNew = NULL;

Cleanup:
    delete pNew; // In case we failed
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a reference CLineShape representing a MilLineShape::Round. This
//      is a circle centered on the end point of the path.
//
//------------------------------------------------------------------------------
HRESULT
SetRoundLineShape(
    __deref_out_ecount(1) CLineShape *&pDest)
        // The destination (m_pStartShape or m_pEndShape)
{
    HRESULT hr;

    // Create the custom line shape. If it fails it will return NULL.
    
    CLineShape *pNew = NULL;
    
    // Create the line shape as a fill only triangle
    IFCOOM(pNew = new CLineShape(0, 0, true, false, NULL));
    IFC(pNew->AddEllipse(0.0f, 0.0f, 1.0f, 1.0f, CR_Parameters));

    // Transfer ownership
    pDest = pNew;
    pNew = NULL;

Cleanup:
    delete pNew; // In case we failed
    RRETURN(hr);
}
#endif // LINE_SHAPES_ENABLED
//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Cunstruct the custom line shape that corresponds to a given canned type.
//      As a side effect, the line cap there will be changed to round, to avoid
//      a visible seam between the line and the shape.
//
//------------------------------------------------------------------------------
HRESULT
CPlainPen::SetCannedLineShape(
    IN MilLineShape::Enum   eType,
        // The line end type
    __deref_out_ecount(1) CLineShape    *&pDest,
        // The destination for the line shape
    __out_ecount(1) MilLineShape::Enum  &eTypeToSet,
        // The destination type to set
    __out_ecount(1) MilPenCap::Enum    &eCap)
        // Line cap there, set    
{
#ifdef LINE_SHAPES_ENABLED
    // Should only be called with non-custom line shape--
    Assert (MilLineShape::Custom != eType);

    HRESULT hr = S_OK;
    
    // Construct the line end
    switch(eType)
    {      
        case MilLineShape::Arrow:
            IFC(SetArrowLineShape(pDest));
            eTypeToSet = MilLineShape::Arrow;
            eCap = MilPenCap::Round;
            break;
        
        case MilLineShape::Diamond:
            IFC(SetDiamondLineShape(pDest));
            eTypeToSet = MilLineShape::Diamond;
            eCap = MilPenCap::Flat;
           break;
        
        case MilLineShape::Round:
            IFC(SetRoundLineShape(pDest));
            eTypeToSet = MilLineShape::Round;
            eCap = MilPenCap::Flat;
            break;
        
        case MilLineShape::Square:
            IFC(SetSquareLineShape(pDest));
            eTypeToSet = MilLineShape::Square;
            eCap = MilPenCap::Flat;
            break;

        case MilLineShape::Custom:
            hr = THR(E_INVALIDARG);
    }
    // default is do nothing but no error

Cleanup:
    RRETURN(hr);
#else
    Assert(false);
    RRETURN(E_NOTIMPL);
#endif // LINE_SHAPES_ENABLED
}
 
//+-----------------------------------------------------------------------------
//
//  Member:
//      CPlainPen::GetExtents
//
//  Synopsis:
//      Get the extents of the stroke
//
//------------------------------------------------------------------------------
HRESULT 
CPlainPen::GetExtents(__out_ecount(1) REAL &rExtents) const
{
    HRESULT hr = S_OK;

    // The pen may inflates the stroked shape by its width/height, but that may be extended further
    // by mitered corners, square caps and line shapes.  These multiply the pen's width or height,
    // so let us first compute the maximal factor

    REAL rThickness = max(m_oGeom.GetWidth(), m_oGeom.GetHeight()) / 2;
    rExtents = rThickness * m_oGeom.GetInflateFactor();

    if (_isnan(rExtents))
    {
        IFC(WGXERR_BADNUMBER);
    }

#ifdef LINE_SHAPES_ENABLED
    REAL r;

     // Include the potential expansion caused by line shapes
    if (m_pStartShape)
    {
        IFC(m_pStartShape->GetExtents(rThickness, rExtents, OUT r));
        if (r > rExtents)
        {
            rExtents = r;
        }
    }
    if (m_pEndShape)
    {
        IFC(m_pEndShape->GetExtents(rThickness, rExtents, OUT r));
        if (r > rExtents)
        {
            rExtents = r;
        }
    }
#endif // LINE_SHAPES_ENABLED
 
Cleanup:
    RRETURN(hr);
}
////////////////////////////////////////////////////////////////////////////
//      Implementation of CPenGeometry
//+-------------------------------------------------------------------------------------------------
//
//  Member:     CPenGeometry::Set
//
//  Synopsis:   Set the width, height and angle of the pen's ellipse
//
//--------------------------------------------------------------------------------------------------
void
CPenGeometry::Set(
    REAL width,   // In: Pen ellipse width
    REAL height,  // In: Pen ellipse height
    REAL angle)   // In: Angle in radians the ellipse is rotated
{
    m_rWidth = fabs(width);
    m_rHeight = fabs(height);
    m_rAngle = angle;
}





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
//      Implementation of CShapeBase.
//
//  $ENDTAG
//
//  Classes:
//      CShapeBase.
//
//------------------------------------------------------------------------------


#include "precomp.hpp"

MtDefine(CShapeBase, MILRender, "CShapeBase");

///////////////////////////////////////////////////////////////////////////
//
// Implementation of CShapeBase

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShapeBase::ConvertToGpPath
//
//  Synopsis:
//      Export to GpPath points & types arrays GDI+ style
//
//  Notes:
//      This method is used as an interface with legacy code.  The GDI+ supports
//      neither no-fill figures nor no-stroke segments.  So if called while
//      filling, it will skip non-fillable figures, and if called for stroking,
//      it will skip no-stroke segments.
//
//------------------------------------------------------------------------------
HRESULT 
CShapeBase::ConvertToGpPath(
    __out_ecount(1) DynArray<MilPoint2F> &rgPoints,
        // Path points
    __out_ecount(1) DynArray<BYTE>      &rgTypes,
        // Path types
    IN  bool                fStroking
        // Stroking if true, filling otherwise (optional)
    ) const
{
    HRESULT hr = S_OK;

    for (UINT i = 0;   i < GetFigureCount();   i++)
    {
        if (fStroking  ||  GetFigure(i).IsFillable())
        {
            CFigureBase figure(GetFigure(i));
            IFC(figure.AddToGpPath(rgPoints, rgTypes, fStroking));
        }
    }
Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Produce the flattened version of this shape
//
//  Note:
//      This method does NOT reset the output shape before populating it with
//      the flattenening
//
//------------------------------------------------------------------------------
HRESULT
CShapeBase::FlattenToShape(
    IN double rTolerance,
        // Flattening tolerance
    IN bool fRelative,
        // True if the tolerance is relative       
    __inout_ecount(1) IShapeBuilder &flattened,
        // The flattened shape
    __in_ecount_opt(1) const CMILMatrix *pMatrix
        // Transformation matrix (NULL OK)
    ) const
{
    HRESULT hr;
    double rAbsoluteTolerance;

    IFC(GetAbsoluteTolerance(rTolerance, fRelative, NULL, pMatrix, OUT rAbsoluteTolerance));

    {
        CPopulationSinkAdapter adapter(&flattened);
        CShapeFlattener sink(&adapter, rAbsoluteTolerance);

        // Organize the shape into chains
        IFC(Populate(&sink, pMatrix));
    }

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Produce the widened version of this shape
//
//  Note:
//      This method does NOT reset the shape, it just adds the results to it
//
//------------------------------------------------------------------------------
HRESULT
CShapeBase::WidenToShape(
    __in_ecount(1) const CPlainPen &pen,
        // The pen
    double rTolerance,
        // Flattening tolerance - absolute
    bool fRelative,
        // True if the tolerance is relative       
    __inout_ecount(1) CShape &widened,
        // The widened shape, populated here
    __in_ecount_opt(1) const CMILMatrix *pMatrix,
        // Render transform (NULL OK)
    __in_ecount_opt(1) const CMILSurfaceRect *prcViewable
        // Viewable region (NULL OK)
    ) const
{
    HRESULT hr;
    double rAbsoluteTolerance;

    IFC(GetAbsoluteTolerance(rTolerance, fRelative, &pen, pMatrix, OUT rAbsoluteTolerance));

    {
        CShapeWideningSink sink(widened);
        IFC(WidenToSink(pen, pMatrix, rAbsoluteTolerance, sink, prcViewable));
    }
Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Widen this path into a widening sink
//
//------------------------------------------------------------------------------
HRESULT
CShapeBase::WidenToSink(
    __in_ecount(1) const CPlainPen &pen,
        // The pen
    __in_ecount_opt(1) const  CMILMatrix *pMatrix,
        // Render transform (NULL OK)
    double rTolerance,
        // Approximation tolerance - absolute
    __inout_ecount(1) CWideningSink &sink,
        // The widening sink
    __in_ecount_opt(1) const CMILSurfaceRect *prcViewable,
        // Viewable region (NULL OK)
    __out_ecount_opt(1) bool *pfPenEmpty
        // If true, we earlied out because the pen is either empty or very
        // close to it (NULL OK)
    ) const
{
    HRESULT hr = S_OK;

    // Set up the scafolding
    CWidener oWidener(rTolerance);
    
    CStartMarker *pStartMarker = NULL;
    CEndMarker *pEndMarker = NULL;
    bool fEmpty = false;

    if (prcViewable && prcViewable->IsEmpty())
    {
        // Our widened geometry won't be visible, so why bother?
        // Indeed, later code depends on prcViewable being non-empty.
        goto Cleanup;
    }
        
    fEmpty = pen.IsEmpty();
    if (fEmpty)
    {
        // The pen was set up with such values (e.g. 0 width) that widening with
        // it woulld produce an empty set, so let us not waste our time.
        goto Cleanup;
    }

    IFC(oWidener.Initialize(pen, &sink, pMatrix, prcViewable, OUT fEmpty));
    if (fEmpty)
    {
        goto Cleanup;
    }

#ifdef LINE_SHAPES_ENABLED
    // Get the start or end shape if any
    if (pen.GetStartShape())
    {
        // Get the start shape marker
        IFCOOM(pStartMarker = new CStartMarker(oWidener.GetPen(), 
                                               *pen.GetStartShape(), 
                                               pMatrix, 
                                               sink, 
                                               rTolerance)); 
        if (pen.GetStartShape()->IsStroked())
        {
            IFC(pStartMarker->SetForStroke(oWidener, &sink));
        }
    }
        
    if (pen.GetEndShape())
    {
        // Get the end shape marker
        IFCOOM(pEndMarker = new CEndMarker(oWidener.GetPen(), *pen.GetEndShape(),
                                           pMatrix, 
                                           sink, 
                                           rTolerance));
        if (pen.GetEndShape()->IsStroked())
        {
            IFC(pEndMarker->SetForStroke(oWidener, &sink));
        }
    }
#endif // LINE_SHAPES_ENABLED
         
    // Process all figures
    for (UINT i = 0;    i < GetFigureCount();    i++)
    {
        IFC(oWidener.Widen(GetFigure(i), pStartMarker, pEndMarker));
    }
Cleanup:

    if (pfPenEmpty)
    {
        *pfPenEmpty = fEmpty;
    }

#ifdef LINE_SHAPES_ENABLED
    delete pStartMarker;
    delete pEndMarker;
#endif

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShapeBase::SetupFillTessellator
//
//  Synopsis:
//      Prepares a Tessellator that should be able to tessellate this shape,
//      optionally with "antialiased edge" triangles. For performance reasons
//      this should filter out what cases it can so that tessellation maybe
//      deferred until the caller is sure to need the tessellation.
//
//       ???:
//      Right now, only succeeds if the shape is a single convex
//      polygon. (Otherwise, returns E_NOTIMPL.)
//
//  Return:
//      WGXHR_EMPTYFILL if the tessellation would yield nothing
//
//------------------------------------------------------------------------------
    
HRESULT
CShapeBase::SetupFillTessellator(
    __in_ecount_opt(1) const CBaseMatrix *pMatrix,
        // Transformation matrix (NULL OK)
    __in_ecount_opt(1) CBufferDispenser *pBufferDispenser,
        // Fast allocator (NULL OK)
    __deref_out_ecount(1) CFillTessellator **ppTessellator
        // Shape/context optimized Tessellator
    ) const
{
    HRESULT hr = S_OK;

    *ppTessellator = NULL;

    //
    // Check for empty fill or special case fill
    //

    if (IsARegion())
    {
        // This shape was constructed as a collection of nonoverlapping rectangles
        IFCOOM(*ppTessellator = new(pBufferDispenser) CRegionFillTessellator(
            *this,
            pMatrix));
    }
    else
    {
        UINT cFillable = 0;
        UINT uLastFillable = 0;

        for (UINT i = 0;  i < GetFigureCount() && cFillable < 2;  i++)
        {
            if (GetFigure(i).IsFillable())
            {
                cFillable++;
                uLastFillable = i;
            }
        }

        if (cFillable == 0)
        {
            // The shape is empty, do nothing and return empty status
            hr = WGXHR_EMPTYFILL;
            goto Cleanup;
        }

        if ((cFillable == 1) && GetFigure(uLastFillable).IsAParallelogram())
        {
            //
            // There is one and only one fillable figure whose index has been
            // stored in uLastFillable, and it is a parallelogram.
            //

            IFCOOM(*ppTessellator = new(pBufferDispenser) CRectFillTessellator(
                GetFigure(uLastFillable),
                pMatrix));
        }
        else
        {
            // Not a special case, create a general tessellator

            IFCOOM(*ppTessellator = new(pBufferDispenser) CGeneralFillTessellator(
                *this,
                pMatrix));
        }
    }

Cleanup:

    RRETURN1(hr, WGXHR_EMPTYFILL);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CCShapeBase::Populate
//
//  Synopsis:
//      Populate a scanner
//
//  Notes:
//      We are NOT checking that the scanner is pristine.
//
//------------------------------------------------------------------------------
HRESULT
CShapeBase::Populate(
    __in_ecount(1) IPopulationSink *scanner,
        // The receiving scanner
    __in_ecount_opt(1) const CBaseMatrix *pMatrix) const
        // Transformation matrix (NULL OK)
{
    HRESULT hr = S_OK;
    UINT i;

    scanner->SetFillMode(GetFillMode());
    
    // Populate
    for (i = 0;  i < GetFigureCount();  i++) 
    {
        if (GetFigure(i).IsFillable())
        {
            CFigureBase figure(GetFigure(i));
            IFC(figure.Populate(scanner, CMILMatrix::ReinterpretBase(pMatrix)));
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Macros:
//      Debug dumps for Outline and Combine
//
//  Synopsis:
//      Dump the input and output of an Outline or Combine operation
//
//  Notes:
//      - Setting g_fTraceOutline=true below will trigger the Outline dump
//      - Setting g_fTraceCombine=true below will trigger the Combine dump
//
//------------------------------------------------------------------------------
#ifdef DBG
    bool g_fTraceCombine = false;
    bool g_fTraceOutline = false;

    #define SAVE_FIGURE_TRACE_STATE bool fSavedFigureTrace = g_fTraceFigureConstruction;
    #define RESTORE_FIGURE_TRACE_STATE  g_fTraceFigureConstruction = fSavedFigureTrace;

    #define DUMP_COMBINE_INPUT(first, second)   \
    if (g_fTraceCombine)                        \
    {                                           \
        (first).Dump();                         \
        (second).Dump();                        \
        OutputDebugString(L"\nResult:\n");      \
        g_fTraceFigureConstruction = true;      \
    }

    #define DUMP_OUTLINE_INPUT                  \
    if (g_fTraceOutline)                        \
    {                                           \
        Dump();                                 \
        OutputDebugString(L"\nResult:\n");      \
        g_fTraceFigureConstruction = true;      \
    }
#else
    #define SAVE_FIGURE_TRACE_STATE
    #define RESTORE_FIGURE_TRACE_STATE
    #define DUMP_COMBINE_INPUT(first, second)
    #define DUMP_OUTLINE_INPUT
#endif

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShapeBase::Outline
//
//  Synopsis:
//      Return the outline of this shape
//
//------------------------------------------------------------------------------
HRESULT
CShapeBase::Outline(
    __inout_ecount(1) IShapeBuilder &result,
        // The outline shape, populated here
    __in double rTolerance,
        // Flattening tolerance
    __in bool fRelative,
        // True if the tolerance is relative       
    __in_ecount_opt(1) const CMILMatrix *pMatrix,
        // Transformation matrix (NULL OK)
    __in bool fRetrieveCurves) const
        // Retrieve curves in the result if true
{
    HRESULT hr = S_OK;
    CDoubleFPU fpu; // Setting floating point state to double precision

    // Debug tracing
    SAVE_FIGURE_TRACE_STATE;
    DUMP_OUTLINE_INPUT;

    double rAbsoluteTolerance;
    IFC(GetAbsoluteTolerance(rTolerance, fRelative, NULL, pMatrix, OUT rAbsoluteTolerance));

    {
        COutline outline(&result, fRetrieveCurves, rAbsoluteTolerance);
        CMilRectF rect;
        bool fDegenerate;


        // Set scanner workspace
        IFC(GetTightBounds(OUT rect, NULL /*pen*/, pMatrix));
        IFC(outline.SetWorkspaceTransform(rect, fDegenerate));
        if (fDegenerate)
            goto Cleanup;
        
        // Organize the shape into chains
        IFC(Populate(&outline, pMatrix));

        // Scan the chains to obtain the outline
        hr = THR(outline.Scan());
    }

Cleanup:
    RESTORE_FIGURE_TRACE_STATE;
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShapeBase::ClipWithParallelogram
//
//  Synopsis:
//      Constructs a shape that, within the supplied parallelogram is identical
//      to shape, but edges appearing outside the parallelogram are stripped.
//
//------------------------------------------------------------------------------

HRESULT
CShapeBase::ClipWithParallelogram(
    __in_ecount(1) const IShapeData *pShape,
        // Shape to be clipped
    __in_ecount(1) const CParallelogram *pClipParallelogram,
        // Parallelogram to clip to
    __inout_ecount(1) IShapeBuilder *pResult,
        // The recipient of the result
    __in_ecount_opt(1) const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *pShapeTransform,
        // Transform for the first shape (Optional, NULL OK)
    __in double rTolerance,
        // Flattening tolerance
    __in bool fRelative)
        // True if the tolerance is relative       
{
    HRESULT hr = S_OK;

    CDoubleFPU fpu; // Setting floating point state to double precision

    MilPoint2F vertices[4];

    double rAbsoluteTolerance;

    IFC(pShape->GetAbsoluteTolerance(
            rTolerance,
            fRelative,
            NULL, /* no pen */
            pShapeTransform,
            OUT rAbsoluteTolerance
            ));

    //  Construct a clip pipeline. Our pipeline has two nodes: one for each
    //  pair of opposite sides of the parallelogram. For each node, clip bounds
    //  are defined by the lines:
    //      a*x + b*y = c
    //      a*x + b*y = d
    //
    //  Label our paralleogram's vertices V0 = (x0,y0), V1 = (x1,y1), etc...:
    //  
    //          V1-----------V2
    //         /             /
    //        /             /
    //       /             /
    //      V0------------V3
    //
    //  We wish the line a*x + b*y = c to be the line passing through V0 and V1.
    //  Hence,
    //
    //     a*x1      + b*y1      = c
    //   - a*x0      + b*y0      = c
    //   --------------------------
    //     a*(x1-x0) + b*(y1-y0) = 0
    //
    //  Hence, we can let a = (y1-y0) and b = -(x1-x0).
    //
    
    pClipParallelogram->GetParallelogramVertices(vertices);

    double a1 = vertices[1].Y - vertices[0].Y;
    double b1 = vertices[0].X - vertices[1].X;

    double a2 = vertices[2].Y - vertices[1].Y;
    double b2 = vertices[1].X - vertices[2].X;

    double c1 = a1 * vertices[0].X + b1 * vertices[0].Y;
    double d1 = a1 * vertices[2].X + b1 * vertices[2].Y;

    double c2 = a2 * vertices[1].X + b2 * vertices[1].Y;
    double d2 = a2 * vertices[3].X + b2 * vertices[3].Y;

    //
    // if a == b == 0, our equations cease to be line equations:
    //      0*x + 0*y = c
    // but in this case the parallelogram is empty, so we needn't bother
    // populating pResult.
    //
    if ((abs(a1) > FUZZ_DOUBLE || abs(b1) > FUZZ_DOUBLE) &&
        (abs(a2) > FUZZ_DOUBLE || abs(b2) > FUZZ_DOUBLE))
    {
        CPopulationSinkAdapter adapter(pResult);
        CStripClipper clip(a1, b1, c1, d1, &adapter, rAbsoluteTolerance);
        CStripClipper clip2(a2, b2, c2, d2, &clip, rAbsoluteTolerance);

        IFC(pShape->Populate(&clip2, pShapeTransform));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShapeBase::ClipWithRect
//
//  Synopsis:
//      Constructs a shape that, within the supplied rectangle is identical to
//      shape, but edges appearing outside the rectangle are stripped.
//
//------------------------------------------------------------------------------

HRESULT
CShapeBase::ClipWithRect(
    __in_ecount(1) const IShapeData *pShape,
        // Shape to be clipped
    __in_ecount(1) const CRectF<CoordinateSpace::Device> *prcClip,
        // Rect to clip to
    __inout_ecount(1) IShapeBuilder *pResult,
        // The recipient of the result
    __in_ecount_opt(1) const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *pShapeTransform,
        // Transform for the first shape (Optional, NULL OK)
    __in double rTolerance,
        // Flattening tolerance
    __in bool fRelative)
        // True if the tolerance is relative       
{
    HRESULT hr = S_OK;

    CDoubleFPU fpu; // Setting floating point state to double precision

    double rAbsoluteTolerance;

    IFC(pShape->GetAbsoluteTolerance(
            rTolerance,
            fRelative,
            NULL, /* no pen */
            pShapeTransform,
            OUT rAbsoluteTolerance
            ));

    //  Construct a clip pipeline. Our pipeline has two nodes: one for the
    //  horizontal bounds, and one for the vertical. 
    
    {
        CPopulationSinkAdapter adapter(pResult);
        CAxisAlignedStripClipper clip(true /* vertical strip */, prcClip->left /* c */, prcClip->right /* d */, &adapter, rAbsoluteTolerance);
        CAxisAlignedStripClipper clip2(false /* horizontal strip */, prcClip->top /* c */, prcClip->bottom /* d */, &clip, rAbsoluteTolerance);

        IFC(pShape->Populate(&clip2, pShapeTransform));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShapeBase::Combine
//
//  Synopsis:
//      Add the result of a Boolean operation to this shape
//
//------------------------------------------------------------------------------
HRESULT
CShapeBase::Combine(
    __in_ecount(1) const IShapeData *pFirst,
        // First operand
    __in_ecount(1) const IShapeData *pSecond,
        // Second operand
    __in MilCombineMode::Enum eOperation,
        // The operation
    __in bool fRetrieveCurves,
        // Retrieve curves in the result if true
    __inout_ecount(1) IShapeBuilder *pResult,
        // The recipient of the result
    __in_ecount_opt(1) const CMILMatrix *pFirstTransform,
        // Transform for the first shape (Optional, NULL OK)
    __in_ecount_opt(1) const CMILMatrix *pSecondTransform,
        // Transform for the second shape (Optional, NULL OK)
    __in double rTolerance,
        // Flattening tolerance
    __in bool fRelative)
        // True if the tolerance is relative       
{
    HRESULT hr = S_OK;
    BOOL fPerformedIntersect = FALSE;
    SAVE_FIGURE_TRACE_STATE;

    if (!pFirst  ||  !pSecond  ||  !pResult)
    {
        IFC(E_INVALIDARG);
    }

    {
        DUMP_COMBINE_INPUT(*pFirst, *pSecond);

        if (eOperation == MilCombineMode::Intersect &&
            pFirst->IsAxisAlignedRectangle() &&
            pSecond->IsAxisAlignedRectangle()
            )
        {
            IFC(IntersectAxisAlignedRectangles(
                *pFirst,
                *pSecond,
                pResult,
                pFirstTransform,
                pSecondTransform,
                &fPerformedIntersect
                ));
        }
        if (!fPerformedIntersect)
        {
            CDoubleFPU fpu; // Setting floating point state to double precision
        
            CMilRectF rect1, rect2;
            bool fDegenerate;
            double rGeometryWidth, rGeometryHeight, rExtent;

            // Set scanner workspace
            IFC(pFirst->GetTightBounds(OUT rect1, NULL /*pen*/, pFirstTransform));
            IFC(pSecond->GetTightBounds(OUT rect2, NULL /*pen*/, pSecondTransform));

            rect1.InclusiveUnion(rect2);

            // convert to double before finding width and height to avoid overflow
            rGeometryWidth = static_cast<double>(rect1.right) - rect1.left;
            rGeometryHeight = static_cast<double>(rect1.bottom) - rect1.top;
            rExtent = max(rGeometryWidth, rGeometryHeight);

            // Clamp and compute absolute toleraence if necessary
            if (fRelative)
            {
                rTolerance = max(rTolerance, FUZZ_DOUBLE) * rExtent;
            }
            else
            {
                rTolerance = max(rTolerance, rExtent * FUZZ_DOUBLE);
            }
                    
            // Set up the boolean operation machinary
            CBoolean boolean(pResult, eOperation, fRetrieveCurves, rTolerance);
            IFC(boolean.SetWorkspaceTransform(rect1, fDegenerate));
            if (fDegenerate)
                goto Cleanup;

            // Organize the first shape into chains
            IFC(pFirst->Populate(&boolean, pFirstTransform));
        
            // Organize the second shape into chains
            IFC(boolean.SetNext());
            IFC(pSecond->Populate(&boolean, pSecondTransform));
        
            // Scan the chains to obtain the result of the operation.
            hr = THR(boolean.Scan());
        }
    }

Cleanup:
    RESTORE_FIGURE_TRACE_STATE;
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CShapeBase::IntersectAxisAlignedRectangles
//
//  Synopsis:
//      Add the result of an intersect of two axis aligned rectangles to this
//      shape. This is a special case of Combine. This function is not always
//      able to do the job (if for example the incoming shapes are not axis
//      aligned). Hence the output flag.
//
//------------------------------------------------------------------------------
HRESULT
CShapeBase::IntersectAxisAlignedRectangles(
    __in_ecount(1) const CShapeBase &first,
        // First operand
    __in_ecount(1) const CShapeBase &second,
        // Second operand
    __inout_ecount(1) IShapeBuilder    *pResult,
        // The recipient of the result
    __in_ecount_opt(1) const CMILMatrix *pFirstTransform,
        // Transform for the first shape (NULL OK)
    __in_ecount_opt(1) const CMILMatrix *pSecondTransform,
        // Transform for the second shape (NULL OK)
    __out_ecount(1) BOOL *pfPerformedIntersect
        // =true if the intersect happened
    )
{
    HRESULT hr = S_OK;
    CMilRectF rc1;
    CMilRectF rc2;

    Assert(pResult);
    Assert(first.IsAxisAlignedRectangle() &&
           second.IsAxisAlignedRectangle());

    *pfPerformedIntersect = FALSE;

    // Special case transforms being the same
    if ((pFirstTransform == pSecondTransform) ||
        ((pFirstTransform != NULL) &&
         (pSecondTransform != NULL) &&
         RtlEqualMemory(pFirstTransform, pSecondTransform, sizeof(*pFirstTransform))))
    {
        //
        // Perform the intersect in source space, transform it
        // and add the resulting rectangle/figure
        //

        first.GetFigure(0).GetAsWellOrderedRectangle(rc1);
        second.GetFigure(0).GetAsWellOrderedRectangle(rc2);

        if (rc1.Intersect(rc2))
        {
            if (pFirstTransform == NULL)
            {
                IFC(pResult->AddRect(rc1));
            }
            else if (pFirstTransform->Is2DAxisAlignedPreserving())
            {
                // Future Consideration:   Transform2DBounds is not optimal for 2DAxisAligned
                //  as it tranforms 4 points and then loops though finding min
                //  and max values.
                pFirstTransform->Transform2DBounds(rc1, OUT rc1);
                IFC(pResult->AddRect(rc1));
            }
            else
            {
                // Add the transformed rectangle
                IFC(pResult->AddRect(rc1, pFirstTransform));
            }
        }
        else
        {
            // intersect is empty- nothing to add
        }
        
        *pfPerformedIntersect = TRUE;
    }

    if (!*pfPerformedIntersect &&
        ((pFirstTransform == NULL) || pFirstTransform->Is2DAxisAlignedPreserving()) &&
        ((pSecondTransform == NULL) || pSecondTransform->Is2DAxisAlignedPreserving())
        )
    {
        //
        // Transform the two rectangles into destination space,
        // perform the intersect there, and add the resulting rect
        //

        first.GetFigure(0).GetAsWellOrderedRectangle(rc1);
        second.GetFigure(0).GetAsWellOrderedRectangle(rc2);
        
        if (pFirstTransform != NULL)
        {
            // Future Consideration:   Transform2DBounds is not optimal for 2DAxisAligned
            //  as it tranforms 4 points and then loops though finding min
            //  and max values.
            pFirstTransform->Transform2DBounds(rc1, OUT rc1);
        }

        if (pSecondTransform != NULL)
        {
            // Future Consideration:   Transform2DBounds is not optimal for 2DAxisAligned
            //  as it tranforms 4 points and then loops though finding min
            //  and max values.
            pSecondTransform->Transform2DBounds(rc2, OUT rc2);
        }

        if (rc1.Intersect(rc2))
        {
            IFC(pResult->AddRect(rc1));
        }
        else
        {
            // intersect is empty- nothing to add
        }

        *pfPerformedIntersect = TRUE;
    }

Cleanup:

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShapeBase::GetRelation
//
//  Synopsis:
//      Get the relation with another shape
//
//------------------------------------------------------------------------------
HRESULT
CShapeBase::GetRelation(
    __in_ecount(1) const IShapeData  &data,      // The other shape
    __in double                      rTolerance, // Flattening tolerance
    __in bool                        fRelative,  // True if the tolerance is relative       
    __out_ecount(1) MilPathsRelation::Enum &eResult    // The operation
    ) const
{
    HRESULT hr = S_OK;
    CMilRectF rcThis, rcOther;

    IFC(GetTightBounds(OUT rcThis));
    IFC(data.GetTightBounds(OUT rcOther));

    if (rcThis.DoesIntersectInclusive(rcOther))
    {
        // Bounding boxes overlap

        CDoubleFPU fpu; // Setting floating point state to double precision
        bool fDegenerate;
        double rAbsoluteTolerance;
        
        IFC(GetAbsoluteTolerance(rTolerance, fRelative, NULL, NULL, OUT rAbsoluteTolerance));
        CRelation relation(rAbsoluteTolerance);

        // Set scanner workspace
        rcThis.InclusiveUnion(rcOther);
        IFC(relation.SetWorkspaceTransform(rcThis, fDegenerate));
        if (fDegenerate)
        {
            // The bounding boxes intersect and are miniscule, so we assume the
            // geometries intersect.
            eResult = MilPathsRelation::Overlap;
        }
        else
        {
            // Organize this shape into chains
            IFC(Populate(&relation));

            // Organize the other shape into chains
            IFC(relation.SetNext());
            IFC(data.Populate(&relation));

            // Scan the chains to obtain the result of the operation.
            IFC(relation.Scan());
            eResult = relation.GetResult();
        }
    }
    else
    {
        // Bounding boxes do not overlap, the shapes are disjoint
        eResult = MilPathsRelation::Disjoint;
    }

#ifdef DBG
    // DumpRelation(eResult);
#endif

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShapeBase::HitTestFill
//
//  Synopsis:
//      Find if a given point is in or near the fill of this shape
//
//------------------------------------------------------------------------------

HRESULT
CShapeBase::HitTestFill(
    __in_ecount(1) const MilPoint2F &ptHit,
        // The point
    IN double rThreshold,
        // Distance considered near
    IN bool fRelative,
        // True if the threshold is relative       
    __in_ecount_opt(1) const CMILMatrix *pMatrix,
        // Transformation to apply to the shape (NULL OK)
    __out_ecount(1) BOOL &fHit,
        // = TRUE if the point is in or near the shape
    __out_ecount(1) BOOL &fIsNear
        // = TRUE if the point is close to the boundary
    ) const
{
    HRESULT hr;
    double rAbsoluteTolerance;

    IFC(GetAbsoluteTolerance(rThreshold, fRelative, NULL, pMatrix, OUT rAbsoluteTolerance));

    {
        CHitTest tester(ptHit, pMatrix, rAbsoluteTolerance);
    
        fHit = fIsNear = FALSE;

        IFC(HitTestFiguresFill(IN OUT tester));

        fHit = fIsNear = tester.WasAborted();

        if (!fHit)
        {
            if (GetFillMode() == MilFillMode::Winding)
            {
                fHit = (tester.GetWindingNumber() != 0);
            }
            else
            {
                Assert(GetFillMode() ==MilFillMode::Alternate);
                fHit = ((tester.GetWindingNumber() & 1) != 0);
            }
        }
    }

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CShapeBase::HitTestFiguresFill
//
//  Synopsis:
//      Hit test all figures fill with a hit-tester
//
//------------------------------------------------------------------------------

HRESULT
CShapeBase::HitTestFiguresFill(
    __inout_ecount(1) CHitTest &tester  // A hit tester
    ) const
{
    UINT i;
    HRESULT hr = S_OK;

    // Traverse the figures to get the winding number at the hit point
    for (i = 0;  i < GetFigureCount();  i++)
    {
        const IFigureData &figure = GetFigure(i); 
        if (!figure.IsEmpty()  &&  figure.IsFillable())
        {
            if (tester.StartAt(figure.GetStartPoint()))
            {
                // We have a hit near the figure's start point
                break;
            }
            IFC(tester.TraverseForward(figure));
            if (tester.WasAborted())
            {
                // A hit was detected near this figure
                break;
            }
            if (!figure.IsClosed() &&
                (tester.EndAt(figure.GetStartPoint())))
            {
                // We have a hit near the figure's closing segment
                break;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CShapeBase::HitTestStroke
//
//  Synopsis:
//      Find if a given point is in or near the stroked shape
//
//  Notes:
//      There is minor difference between the behavior of HitTestStroke and
//      HitTestFill.   HitTestFill is guaranteed to return fIsNear=true whenever
//      the hit is near the defining geometry, even when it lies inside the fill
//      (which can happen in in Winding mode). HitTestStroke is less dilligent
//      about flagging a hit as being near the countour, because it bails out as
//      soon as it identifies any kind of hit, neglecting to check the entire
//      widened contour.  So HitTestStroke may return fIsNear=false when ptHit
//      is actually on the contour of the widened path. Since fIsNear is
//      currently nowhere checked, this negligence is accepted in the interest
//      of speed.
//
//------------------------------------------------------------------------------

HRESULT
CShapeBase::HitTestStroke(
    __in_ecount(1) const CPlainPen &pen,
        // The pen with which we stroke
    __in_ecount(1) const MilPoint2F &ptHit,
        // The point
    IN double rThreshold,
        // Distance considered near
    IN bool fRelative,
        // True if the threshold is relative       
    __in_ecount_opt(1) const CMILMatrix  *pMatrix,
        // Transformation to apply to the shape (NULL OK)
    __out_ecount(1) BOOL &fHit,
        // = TRUE if the point is in or near the shape
    __out_ecount(1) BOOL &fIsNear
        // = TRUE if the point is close to the boundary
    ) const
{
    HRESULT hr;
    double rAbsoluteTolerance;

    IFC(GetAbsoluteTolerance(rThreshold, fRelative, NULL, pMatrix, OUT rAbsoluteTolerance));

    {
        // Instantiate a hit-test widening-sink
        CHitTest tester(ptHit, pMatrix, rAbsoluteTolerance);
        CHitTestSink sink(tester);

        fHit = fIsNear = false;

        // Widening to that sink to hit test the stroke.
        IFC(WidenToSink(pen, pMatrix, DEFAULT_FLATTENING_TOLERANCE, IN OUT sink));

        // Get the results
        fHit = sink.WasHit();
        fIsNear = sink.WasHitNear();
    }

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CShapeBase::GetArea
//
//  Synopsis:
//      Get the fill area
//
//------------------------------------------------------------------------------
HRESULT
CShapeBase::GetArea(
    IN double rTolerance,
        // Flattening tolerance
    IN bool fRelative,
        // True if the tolerance is relative       
    __in_ecount_opt(1) const CMILMatrix *pMatrix,
        // Transformation to apply to the shape (NULL OK)
    __out_ecount(1) double &result 
        // The area, not set if error encountered
    ) const
{
    HRESULT hr = S_OK;

    if (IsAxisAlignedRectangle())
    {
        MilRectF rc;
        GetFigure(0).GetAsRectangle(rc);
        // convert to double before finding width and height to avoid overflow
        result = fabs((static_cast<double>(rc.right)-rc.left) * (static_cast<double>(rc.bottom)-rc.top));

        if (pMatrix)
        {
            result *= fabs(pMatrix->GetDeterminant2D());
        }
    }
    else
    {
        double rAbsoluteTolerance;
        IFC(GetAbsoluteTolerance(rTolerance, fRelative, NULL, pMatrix, OUT rAbsoluteTolerance));

        CArea area(rAbsoluteTolerance);
        CMilRectF rect;
        bool fDegenerate;
        CDoubleFPU fpu; // Setting floating point state to double precision

        // Set scanner workspace
        IFC(GetTightBounds(OUT rect, NULL /*pen*/, pMatrix));
        IFC(area.SetWorkspaceTransform(rect, fDegenerate));
        if (fDegenerate)
            goto Cleanup;
       
        // Organize the shape into chains
        IFC(Populate(&area, pMatrix));

        // Scan the chains to obtain the area
        IFC(area.Scan());
        result = area.GetResult();
    }

Cleanup:
    RRETURN(hr);
}

///////////////////////////////////////////////////////////////////////////////
// BOUNDS COMPUTATION

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShapeBase::GetLooseBounds
//
//  Synopsis:
//      Get the loose bounds
//
//  Notes:
//      This method is cheaper than GetTightBounds, but the bounds may be
//      considerably looser.
//
//      This method is guaranteed to return a well-ordered rectangle.
//
//------------------------------------------------------------------------------
HRESULT
CShapeBase::GetLooseBounds(
    __out_ecount(1) CMilRectF &rect,            // The bounds of this shape
    __in_ecount_opt(1) const CPlainPen *pPen,      // The pen (optional, NULL OK)
    __in_ecount_opt(1) const CBaseMatrix *pMatrix   // Transformation (optional, NULL OK)
    ) const 
{
    HRESULT hr;

    // Get the cached bounds of the geometry
    IFC(GetCachedBounds(rect));

    // Not good to extend or tranform empty bounds -- so check
    if (!rect.IsEmpty())
    {
        if (NULL != pPen)
        {
            // Pad with what the pen might add
            REAL pad;
            IFC(pPen->GetExtents(OUT pad));
            rect.Inflate(pad, pad);
        }
          
        if (NULL != pMatrix)
        {
            // Get the bounds of the transformed bounding box
            CMILMatrix::ReinterpretBase(pMatrix)->Transform2DBounds(rect, OUT rect);
        }
    }

    if (!rect.HasValidValues())
    {
        IFC(WGXERR_BADNUMBER);
    }

    Assert(rect.IsWellOrdered());

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShapeBase::GetTightBoundsNoBadNumber
//
//  Synopsis:
//      Get the tight bounds of the shape's fill and stroke
//
//  Note:
//      Non-fillable figures will be ignored in the absence of a pen.
//
//  Note:
//      This method is guaranteed not to return WGXERR_BADNUMBER. In such a
//      case, though, we will return a non well-ordered rect.
//
//  Future Consideration:  Ideally, GetTightBounds itself should act this
//  way (it fits in better with the float-carries-nan-taint philosophy).
//  Unfortunately, there's too much code at the present time that relies on us
//  returning a well-ordered rectangle. Eventually, all callers should be made
//  NaN aware and the GetTightBounds behavior can be removed.
//
//------------------------------------------------------------------------------
HRESULT
CShapeBase::GetTightBoundsNoBadNumber(
    __out_ecount(1) CMilRectF &rect,
        // The bounds of this shape
    __in_ecount_opt(1) const CPlainPen *pPen,
        // The pen (NULL OK)
    __in_ecount_opt(1) const CMILMatrix *pMatrix,
        // Transformation (NULL OK)
    __in double rTolerance, 
        // Error tolerance (optional)
    __in bool fRelative,
        // True if the tolerance is relative (optional)       
    __in bool fSkipHollows) const
        // If true, skip non-fillable figures when computing fill bound       
{
    HRESULT hr = S_OK;
    
    hr = GetTightBounds(
        rect,
        pPen,
        pMatrix,
        rTolerance, 
        fRelative,
        fSkipHollows
        );

    if (WGXERR_BADNUMBER == hr)
    {
        hr = S_OK;
        rect.left = rect.right = rect.top = rect.bottom = FLOAT_QNAN;
    }
    else
    {
        IFC(hr);
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShapeBase::GetTightBounds
//
//  Synopsis:
//      Get the tight bounds of the shape's fill and stroke
//
//  Note:
//      Non-fillable figures will be ignored in the absence of a pen.
//
//  Note:
//      This method is guaranteed to return a well-ordered rect.
//
//------------------------------------------------------------------------------
HRESULT
CShapeBase::GetTightBounds(
    __out_ecount(1) CMilRectF &rect,
        // The bounds of this shape
    __in_ecount_opt(1) const CPlainPen *pPen,
        // The pen (NULL OK)
    __in_ecount_opt(1) const CMILMatrix *pMatrix,
        // Transformation (NULL OK)
    __in double rTolerance, 
        // Error tolerance (optional)
    __in bool fRelative,
        // True if the tolerance is relative (optional)       
    __in bool fSkipHollows) const
        // If true, skip non-fillable figures when computing fill bound       
{
    HRESULT hr = S_OK;
    
    // Fill bounds are needed if there is no stroke, a very
    // thin stroke, or if the fill bounds may exceed the stroke bounds
    bool fFillBoundsNeeded = true;

    if (pPen)
    {
        double rAbsoluteTolerance;
        IFC(GetAbsoluteTolerance(rTolerance, fRelative, pPen, pMatrix, OUT rAbsoluteTolerance));

        IFC(GetStrokeBounds(OUT rect, &fFillBoundsNeeded, *pPen, pMatrix, rAbsoluteTolerance));

        if (!rect.HasValidValues())
        {
            IFC(WGXERR_BADNUMBER);
        }
    }

    fFillBoundsNeeded = fFillBoundsNeeded || !pPen || pPen->CanFillBoundsExceedStrokeBounds(*this);

    if (fFillBoundsNeeded)
    {
        CMilRectF rcPenRect;
        
        if (pPen)
        {
            // store rect so we can union it at the end.
            rcPenRect = rect;
        }

        if ((NULL == pMatrix || pMatrix->IsTranslateOrScale()) && !HasHollows())
        {
            // No complex transformation, get the shape's cached bounds
            IFC(GetCachedBounds(OUT rect));
            if (pMatrix)
            {
                // The transformation is simple so the transformed bounding box is tight
                pMatrix->Transform2DBounds(rect, OUT rect);
            }
        }
        else
        {
            IFC(GetFillBounds(OUT rect, fSkipHollows, pMatrix));
        }

        if (!rect.HasValidValues())
        {
            IFC(WGXERR_BADNUMBER);
        }

        if (pPen)
        {
            Assert(rect.IsWellOrdered());
            Assert(rcPenRect.IsWellOrdered());
            rect.InclusiveUnion(rcPenRect);
        }
    }
    
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShapeBase::GetFillBounds
//
//  Synopsis:
//      Get the bounds of the bounds of the filled region
//
//------------------------------------------------------------------------------
HRESULT
CShapeBase::GetFillBounds(
    __out_ecount(1) CMilRectF &rect,
        // The bounds of this shape
    __in bool fFillOnly,
        // Skip non-fillable figures if true
    __in_ecount_opt(1) const CMILMatrix *pMatrix 
        // Transformation (NULL OK)
    ) const
{
    HRESULT hr = S_OK;
    CBounds bounds;

    IFC(UpdateBounds(IN OUT bounds, fFillOnly, pMatrix));

    IFC(bounds.SetRect(OUT rect));

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CShapeBase::UpdateBounds
//
//  Synopsis:
//      Update the bounds with this shape's geometry
//
//------------------------------------------------------------------------------
HRESULT
CShapeBase::UpdateBounds(
    __inout_ecount(1) CBounds &bounds,
        // Bounds, updated here
    __in bool fFillOnly,
        // Skip non-fillable figures if true
    __in_ecount_opt(1) const CMILMatrix *pMatrix
        // Transformation (NULL OK)
    ) const
{
    HRESULT hr = S_OK;

    for (UINT i = 0;  i < GetFigureCount();  i++)
    {
        const IFigureData & figure = GetFigure(i);
        if (!fFillOnly || figure.IsFillable())
        {
            IFC(CFigureBase(figure).UpdateBounds(OUT bounds, pMatrix));
        }
    }

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Get the bounding box of a shape
//
//  Note:
//      This method takes an optional tolerance argument that will be used in
//      the widening.  The resulting box will be inflated by that tolerance, so
//      it will always cover the shape.  Passing a large tolerance will speed up
//      the computation but will produce a more conservative (larger) box.  If
//      no tolerance or 0 tolerance is passed then the tolerance used will be
//      1/4 the transformed pen width.
//
//------------------------------------------------------------------------------
HRESULT
CShapeBase::GetStrokeBounds(
    __out_ecount(1) CMilRectF &rect,
        // The bounds of this shape
    __out_ecount_opt(1) bool *pfNotCalculated,
        // The calculation short-circuited due, for instance, to a small pen.
        // (NULL ok)
    __in_ecount(1) const CPlainPen &pen, 
        // The pen
    __in_ecount_opt(1) const CMILMatrix *pMatrix,
        // Transformation (optional, NULL OK)
    double rTolerance
        // Error tolerance (optional)
    ) const 
{
    HRESULT hr = S_OK;
    CStrokeBoundsSink sink;

    if (0 == rTolerance)
    {
        rTolerance = DEFAULT_FLATTENING_TOLERANCE;
    }
    else
    {
        rTolerance = ClampMinDouble(rTolerance, MIN_TOLERANCE);
    }
  
    IFC(WidenToSink(pen, pMatrix, rTolerance, sink, NULL /* no clip rectangle */, pfNotCalculated));

    if (sink.NotUpdated())
    {
        // If the shape has zero size, the sink may never have been updated.
        // In that case, though, we still want to record the location of
        // the shape. 

        MilPoint2F milPoint;
        if (GetPointOnShape(&milPoint))
        {
            GpPointR points[2];

            points[0] = points[1] = GpPointR(milPoint);
            IFC(sink.QuadTo(points));
        }
    }

    IFC(sink.SetRect(OUT rect));

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CShapeBase::GetCachedBounds
//
//  Synopsis:
//      Get the cached bounds
//
//  Notes:
//      Although this is a "Get" method, it may actually update the data's
//      cached bounds
//
//------------------------------------------------------------------------------
HRESULT
CShapeBase::GetCachedBounds(
    __out_ecount(1) CMilRectF &rect) const     // The data's cached bounds
{
    HRESULT hr = S_OK;

    if (!GetCachedBoundsCore(rect))
    {
        // Compute the bounds and update the cache
        CMilRectF box;
        IFC(GetFillBounds(OUT box, false /* including non-fillable figures */));
        SetCachedBounds(box);
        rect = box;
    }

    if (!rect.HasValidValues())
    {
        IFC(WGXERR_BADNUMBER);
    }

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CShapeBase::GetAbsoluteTolerance
//
//  Synopsis:
//      Get the absolute tolerance that corresponds to a given relative
//      tolerance
//
//  Return:
//      An error if loose bounds were computed to NaN
//
//------------------------------------------------------------------------------
HRESULT
CShapeBase::GetAbsoluteTolerance(
    __in double                          rTolerance, // The tolerance
    __in bool                            fRelative,  // =true if the tolerance is relative
    __in_ecount_opt(1) const CPlainPen   *pPen,      // A pen that may be stroking this shape
    __in_ecount_opt(1) const CBaseMatrix *pMatrix,   // Transformation matrix
    __out_ecount(1) double               &rAbsolute  // absolute tolerance
    ) const
{
    HRESULT hr;
    CMilRectF rcLooseBounds;
    double rBoundsWidth, rBoundsHeight, rExtent;

    // Get an estimate of the size of this shape
    IFC(GetLooseBounds(rcLooseBounds, pPen, pMatrix));

    // convert to double before finding width and height to avoid overflow
    rBoundsWidth = static_cast<double>(rcLooseBounds.right) - rcLooseBounds.left;
    rBoundsHeight = static_cast<double>(rcLooseBounds.bottom) - rcLooseBounds.top;

    if (_isnan(rBoundsWidth) || _isnan(rBoundsHeight))
    {
        IFC(WGXERR_BADNUMBER);
    }
    rExtent = max(rBoundsWidth, rBoundsHeight);
    
    if (fRelative)
    {
        rAbsolute = max(rTolerance, FUZZ_DOUBLE) * rExtent;
    }
    else
    {
        rAbsolute = max(rTolerance, rExtent * FUZZ_DOUBLE);
    }

    // Now rAbsolute cannot be NaN
    Assert(rAbsolute >= 0);

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CShapeBase::GetPointOnShape
//
//  Synopsis:
//      Returns an arbitrary point on the shape (or FALSE if all the shape's
//      figures are empty).
//
//------------------------------------------------------------------------------
BOOL CShapeBase::GetPointOnShape(
    __out_ecount(1) MilPoint2F *point) const
{
    BOOL fFoundOne = FALSE;

    for (UINT i=0; i< GetFigureCount(); ++i)
    {
        if (!GetFigure(i).IsEmpty())
        {
            *point = GetFigure(i).GetStartPoint();

            fFoundOne = TRUE;

            break;
        }
    }

    return fFoundOne;
}

#ifdef DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CShapeBase::Dump
//
//  Synopsis:
//      Debug dump
//
//------------------------------------------------------------------------------
void CShapeBase::Dump() const
{
    UINT i;

    if (MilFillMode::Winding == GetFillMode())
    {
        OutputDebugString(L"Shape: fill mode = Winding\n"); 
    }
    else
    {
        OutputDebugString(L"Shape: fill mode = Alternate\n"); 
    }

    for (i = 0;  i < GetFigureCount();  i++)
    {
        CFigureBase figure(GetFigure(i));
        figure.Dump();
    }
}
#endif // def DBG




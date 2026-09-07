// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*++



Module Name:

    geometry_api.cpp

Abstract:

   Geometry flat apis


Environment:

    User mode only.

--*/

#include "precomp.hpp"

MtDefine(DashArray, MILRender, "DashArray");

typedef void (CALLBACK *AddFigureToList)(
    BOOL isFilled,
    BOOL isClosed,
    __in_ecount(pointCount) MilPoint2F *pPoints,
    UINT pointCount,
    __in_ecount(typeCount) BYTE *pTypes,
    UINT typeCount);

/*++

Routine Description:

    SetPenDoubleDashArray

    This function takes a dash array of doubles, converts it to floating
    point and sets it on the pen. If needed it expands out the array during
    the conversion.

--*/

HRESULT SetPenDoubleDashArray(
    __inout_ecount(1) CPlainPen *pPen,
    __in_ecount_opt(cDash) double *rgDashDouble,
    UINT cDash
    )
{
    Assert(rgDashDouble || cDash == 0);

    HRESULT hr = S_OK;

    float *rgDashFloat = NULL;

    if (cDash > 0 && rgDashDouble != NULL)
    {
        UINT cNewDash = cDash;

        //
        // If there are an odd number of dashes, we duplicate the dashes
        // to achieve an even number.
        //

        if (cDash & 1)
        {
            IFC(UIntMult(cNewDash, 2, &cNewDash));
        }

        IFC(HrMalloc(
            Mt(DashArray),
            sizeof(float),
            cNewDash,
            (void**)&rgDashFloat
            ));

        //
        // Convert the dash array from double to float.
        //

        for (UINT i = 0; i < cDash; i++)
        {
            rgDashFloat[i] = static_cast<float>(rgDashDouble[i]);
        }

        //
        // Duplicate dash array if there was an odd number of dashes
        //

        if (cDash != cNewDash)
        {
            Assert(cDash*2 == cNewDash);

            RtlCopyMemory(
                rgDashFloat + cDash,
                rgDashFloat,
                sizeof(rgDashFloat[0]) * cDash
                );
        }

        IFC(pPen->SetDashArray(rgDashFloat, cNewDash));
    }

Cleanup:

    FreeHeap(rgDashFloat);
    RRETURN(hr);
}

/*++

Routine Description:

    InitializePen

    This function initializes a CPlainPen object from the input data protocol
    command in the form of a MilPenData and a provided dash array consisting
    of doubles.

--*/

HRESULT InitializePen(
    __inout_ecount(1) CPlainPen *pPen,
    __in_ecount(1) MilPenData *pData,
    __in_bcount_opt(pData->DashArraySize) double *pDashArray
    )
{
    pPen->Set(static_cast<float>(pData->Thickness), static_cast<float>(pData->Thickness), 0);
    pPen->SetStartCap(pData->StartLineCap);
    pPen->SetEndCap(pData->EndLineCap);
    pPen->SetDashCap(pData->DashCap);
    pPen->SetJoin(static_cast<MilLineJoin::Enum>(pData->LineJoin));
    pPen->SetMiterLimit(static_cast<float>(pData->MiterLimit));
    pPen->SetDashOffset(static_cast<float>(pData->DashOffset));

    UINT cDash = pData->DashArraySize / sizeof(double);

    Assert(
        (pDashArray != NULL && cDash > 0) ||
        (pDashArray == NULL && cDash == 0)
        );

    RRETURN(SetPenDoubleDashArray(pPen, pDashArray, cDash));
}

HRESULT WINAPI MilUtility_PathGeometryWiden(
    __in_ecount(1) MilPenData *pPenData,
    __in_bcount(pPenData->DashArraySize) double *pDashArray,
    __in_ecount_opt(1) MilMatrix3x2D *pMatrix, //applied to the geometry but not to the pen
    IN MilFillMode::Enum fillRule,
    __in_bcount(nSize) MilPathGeometry *pPathData,
    IN UINT32 nSize,
    IN double rTolerance,
    IN bool fRelative,
    IN AddFigureToList fnAddFigureToList,
    __out_ecount(1) MilFillMode::Enum *pOutFillRule)
{
    HRESULT hr = S_OK;

    IFCNULL(pPenData);
    Assert(nSize >= sizeof(MilPathGeometry));

    IFCNULL(pPathData);
    IFCNULL(fnAddFigureToList);
    IFCNULL(pOutFillRule);

    {
        CMILMatrix matrix(pMatrix);

        CPlainPen pen;
        IFC(InitializePen(&pen, pPenData, pDashArray));

        {
            PathGeometryData pathGeometry(
                pPathData,
                nSize,
                fillRule,
                matrix.IsIdentity() ? NULL : &matrix);

            CShape widenedShape;

            IFC(pathGeometry.WidenToShape(
                pen,
                rTolerance,
                fRelative,
                widenedShape,
                NULL    // matrix
                ));

            // For each resulting figure, use the call back to pass the points up to managed code
            // and construct a PathFigure. An alternative is to implement IFigureBuilder
            // as an internal interface on PathGeometry and allow direct construction of the
            // managed object as its widened.
            for (UINT index=0; index<widenedShape.GetFigureCount(); index++)
            {
                const CFigureData &figureData = widenedShape.GetFigureData(index);

                fnAddFigureToList(
                    figureData.IsFillable(),
                    figureData.IsClosed(),
                    figureData.GetRawPoints(),
                    figureData.GetPointCount(),
                    figureData.GetRawTypes(),
                    figureData.GetSegCount()
                    );
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI MilUtility_PathGeometryOutline(
    __in_ecount_opt(1) MilMatrix3x2D *pMatrix,
    IN MilFillMode::Enum fillRule,
    __in_bcount(nSize) MilPathGeometry *pPathData,
    IN UINT32 nSize,
    IN double rTolerance,
    IN bool fRelative,
    IN AddFigureToList fnAddFigureToList,
    __out_ecount(1) MilFillMode::Enum *pOutFillRule)
{
    HRESULT hr = S_OK;

    Assert(nSize >= sizeof(MilPathGeometry));

    IFCNULL(pPathData);
    IFCNULL(fnAddFigureToList);
    IFCNULL(pOutFillRule);

    {
        CMILMatrix matrix(pMatrix);

        PathGeometryData pathGeometry(
            pPathData,
            nSize,
            fillRule,
            matrix.IsIdentity() ? NULL : &matrix);

        CShape outlinedShape;

        IFC(pathGeometry.Outline(
            outlinedShape,
            rTolerance,
            fRelative,
            NULL));                      // There is opportunity to send a second matrix here.

        *pOutFillRule = pathGeometry.GetFillMode();

        // For each resulting figure, use the call back to pass the points up to managed code
        // and construct a PathFigure. An alternative is to implement IFigureBuilder
        // as an internal interface on PathGeometry and allow direct construction of the
        // managed object as its widened.
        for (UINT index=0; index<outlinedShape.GetFigureCount(); index++)
        {
            const CFigureData &figureData = outlinedShape.GetFigureData(index);

            fnAddFigureToList(
                figureData.IsFillable(),
                figureData.IsClosed(),
                figureData.GetRawPoints(),
                figureData.GetPointCount(),
                figureData.GetRawTypes(),
                figureData.GetSegCount());
        }
    }

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI MilUtility_GetPointAtLengthFraction(
    __in_ecount_opt(1) MilMatrix3x2D *pMatrix,
    IN MilFillMode::Enum fillRule,
    __in_bcount(nSize) MilPathGeometry *pPathData,
    IN UINT32 nSize,
    IN double rFraction,
    __out_ecount(1) MilPoint2D *pPoint,
    __out_ecount(1) MilPoint2D *pVecTangent)
{
    HRESULT hr = S_OK;

    Assert(nSize >= sizeof(MilPathGeometry));

    IFCNULL(pPathData);
    IFCNULL(pPoint);
    IFCNULL(pVecTangent);

    {
        CMILMatrix matrix(pMatrix);

        PathGeometryData pathGeometry(
            pPathData,
            nSize,
            fillRule,
            matrix.IsIdentity() ? NULL : &matrix);

        MilPoint2F ptF;
        MilPoint2F vecTangentF;

        {

            CAnimationPath animationPath;
            IFC(animationPath.SetUp(pathGeometry));

            animationPath.GetPointAtLengthFraction(
                static_cast<FLOAT>(rFraction),
                ptF, 
                &vecTangentF
                );
        }

        pPoint->X = static_cast<DOUBLE>(ptF.X);
        pPoint->Y = static_cast<DOUBLE>(ptF.Y);

        pVecTangent->X = static_cast<DOUBLE>(vecTangentF.X);
        pVecTangent->Y = static_cast<DOUBLE>(vecTangentF.Y);

    }

Cleanup:
    RRETURN(hr);
}


HRESULT WINAPI MilUtility_PathGeometryCombine(
    __in_ecount_opt(1) MilMatrix3x2D *pGeometryMatrix,
        // Matrix applied to the final result
    __in_ecount_opt(1) MilMatrix3x2D *pMatrix1,
        // Matrix applied to the pPathData1
    __in MilFillMode::Enum fillRule1,
    __in_bcount(nSize1) MilPathGeometry *pPathData1,
    __in UINT32 nSize1,
    __in_ecount_opt(1) MilMatrix3x2D *pMatrix2,
        // Matrix applied to the pPathData2
    __in MilFillMode::Enum fillRule2,
    __in_bcount(nSize2) MilPathGeometry *pPathData2,
    __in UINT32 nSize2,
    __in double rTolerance, 
    __in bool fRelative,
    __in_ecount(1) AddFigureToList fnAddFigureToList,
    __in MilCombineMode::Enum combineMode,
    __out_ecount(1) MilFillMode::Enum *pOutFillRule)
{
    HRESULT hr = S_OK;

    Assert(nSize1 >= sizeof(MilPathGeometry));
    Assert(nSize2 >= sizeof(MilPathGeometry));

    IFCNULL(pGeometryMatrix);
    IFCNULL(pMatrix1);
    IFCNULL(pPathData1);
    IFCNULL(pMatrix2);
    IFCNULL(pPathData2);
    IFCNULL(fnAddFigureToList);
    IFCNULL(pOutFillRule);

    {
        CMILMatrix matrix1(pMatrix1);

        PathGeometryData pathGeometry1(
            pPathData1,
            nSize1,
            fillRule1,
            matrix1.IsIdentity() ? NULL : &matrix1);

        CMILMatrix matrix2(pMatrix2);

        PathGeometryData pathGeometry2(
            pPathData2,
            nSize2,
            fillRule2,
            matrix2.IsIdentity() ? NULL : &matrix2);

        CShape combinedShape;

        CMILMatrix matrix(pGeometryMatrix);
        const CMILMatrix *pMatrix = matrix.IsIdentity() ? NULL : &matrix;

        IFC(CShapeBase::Combine(
            &pathGeometry1,
            &pathGeometry2,
            combineMode,
            true,  // ==> Do retrieve curves from the flattened result
            &combinedShape,
            pMatrix,
            pMatrix,
            rTolerance,
            fRelative));

        *pOutFillRule = combinedShape.GetFillMode();

        // For each resulting figure, use the call back to pass the points up to managed code
        // and construct a PathFigure. An alternative is to implement IFigureBuilder
        // as an internal interface on PathGeometry and allow direct construction of the
        // managed object as its widened.
        for (UINT index=0; index<combinedShape.GetFigureCount(); index++)
        {
            const CFigureData &figureData = combinedShape.GetFigureData(index);

            fnAddFigureToList(
                figureData.IsFillable(),
                figureData.IsClosed(),
                figureData.GetRawPoints(),
                figureData.GetPointCount(),
                figureData.GetRawTypes(),
                figureData.GetSegCount());
        }
    }

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI MilUtility_PathGeometryFlatten(
    __in_ecount_opt(1) MilMatrix3x2D *pMatrix,
    IN MilFillMode::Enum fillRule,
    __in_bcount(nSize) MilPathGeometry *pPathData,
    IN UINT32 nSize,
    IN double rTolerance,
    IN bool fRelative,
    IN AddFigureToList fnAddFigureToList,
    __out_ecount(1) MilFillMode::Enum *pOutFillRule)
{
    HRESULT hr = S_OK;

    Assert(nSize >= sizeof(MilPathGeometry));

    IFCNULL(pPathData);
    IFCNULL(fnAddFigureToList);
    IFCNULL(pOutFillRule);

    {
        CMILMatrix matrix(pMatrix);

        PathGeometryData pathGeometry(
            pPathData,
            nSize,
            fillRule,
            matrix.IsIdentity() ? NULL : &matrix);

        CShape flattenedShape;

        IFC(pathGeometry.FlattenToShape(rTolerance, fRelative, flattenedShape, NULL));

        *pOutFillRule = pathGeometry.GetFillMode();

        // For each resulting figure, use the call back to pass the points up to managed code
        // and construct a PathFigure. An alternative is to implement IFigureBuilder
        // as an internal interface on PathGeometry and allow direct construction of the
        // managed object as its widened.
        for (UINT index=0; index<flattenedShape.GetFigureCount(); index++)
        {
            const CFigureData &figureData = flattenedShape.GetFigureData(index);

            fnAddFigureToList(
                figureData.IsFillable(),
                figureData.IsClosed(),
                figureData.GetRawPoints(),
                figureData.GetPointCount(),
                figureData.GetRawTypes(),
                figureData.GetSegCount());
        }
    }

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI MilUtility_PolygonBounds(
    __in_ecount_opt(1) MilMatrix3x2D *pWorldMatrix,
        // Transformation matrix to be applied to both pen and geometry
    __in_ecount(1) MilPenData *pPenData,
        // Pen, hit test the stroke if not null
    __in_bcount_opt(pPenData->DashArraySize) double *pDashArray,
        // Dash array
    __in_ecount(cPoints) MilPoint2D *pPoints,
        // Points defining the path
    __in_ecount(cSegments) byte *pTypes,
        // Types defining the path
    __in UINT cPoints,
        // Number of points
    __in UINT cSegments,
        // Number of segments
    __in_ecount_opt(1) MilMatrix3x2D *pGeometryMatrix,
        // Transformation matrix to be applied to the geometry but not to the pen
    __in double rTolerance,
        // Approximation error tolerance
    __in bool fRelative,
        // =true if the tolerance is relative
    __in bool fSkipHollows,
        // If true, skip non-fillable figures when computing fill bounds       
    __out_ecount(1) MilPointAndSizeD *pBounds)
        // The bounds
{
    HRESULT hr = S_OK;
    CMILMatrix matWorld(pWorldMatrix);
    CMILMatrix matGeometry(pGeometryMatrix);
    CMilRectF rect;
    CShape shape;
    CPlainPen pen;

    IFCNULL(pPoints);
    IFCNULL(pTypes);
    IFCNULL(pBounds);

    if (pPenData)
    {
        IFC(InitializePen(&pen, pPenData, pDashArray));
    }

    // Construct a CShape
    IFC(shape.AddFigureFromRawData(cPoints, cSegments, pPoints, pTypes, &matGeometry));

    IFC(shape.GetTightBounds(
        rect,
        (pPenData==NULL)?NULL:&pen,
        matWorld.IsIdentity() ? NULL : &matWorld,
        rTolerance,
        fRelative,
        fSkipHollows));

    MilPointAndSizeDFromMilRectF(OUT *pBounds, rect);

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI MilUtility_PathGeometryBounds(
    __in_ecount(1) MilPenData *pPenData,
        // Pen data
    __in_bcount_opt(pPenData->DashArraySize) double *pDashArray,
        // Pen dash array
    __in_ecount_opt(1) MilMatrix3x2D *pWorldMatrix,
        // Transformation matrix to be applied to both pen and geometry
    __in MilFillMode::Enum fillRule,
        // Fill rule       
    __in_bcount(nSize) MilPathGeometry *pPathData,
        // Geometry data
    __in UINT32 nSize,
        // Size of the above
    __in_ecount_opt(1) MilMatrix3x2D *pGeometryMatrix,
        // Transformation matrix to be applied to the geometry but not to the pen
    __in double rTolerance,
        // Approximation tolerance
    __in bool fRelative,
        // =true if the tolerance is relative
    __in bool fSkipHollows,
        // If true, skip non-fillable figures when computing fill bounds       
    __out_ecount(1) MilRectD *pBounds)
        // The computed bounds
{
    HRESULT hr = S_OK;

    Assert(nSize >= sizeof(MilPathGeometry));

    IFCNULL(pPathData);
    IFCNULL(pBounds);

    {
        CMILMatrix matWorld(pWorldMatrix);
        CMILMatrix matGeometry(pGeometryMatrix);

        CPlainPen pen;
        if (pPenData)
        {
            IFC(InitializePen(&pen, pPenData, pDashArray));
        }

        {
            CMilRectF rcBounds;

            PathGeometryData pathGeometry(
                pPathData,
                nSize,
                fillRule,
                matGeometry.IsIdentity() ? NULL : &matGeometry);

            IFC(pathGeometry.GetTightBounds(
                OUT rcBounds,
                (pPenData==NULL)?NULL:&pen,
                matWorld.IsIdentity() ? NULL : &matWorld,
                rTolerance,
                fRelative,
                fSkipHollows));

            MilRectDFromMilRectF(OUT *pBounds, rcBounds);
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Function: MilUtility_PolygonHitTest
//
//  Synopsis: Hit test the fill or a stroke of a given path
//
//  Notes:    The path is defined with points and segment-types + transformation.
//            The transformation matrix applies to the geometry only, not to the pen
//
//------------------------------------------------------------------------------

HRESULT WINAPI MilUtility_PolygonHitTest(
    __in_ecount_opt(1) MilMatrix3x2D       *pMatrix,    // Geometry (and not pen) transformation
    __in_ecount(1) MilPenData             *pPenData,   // Pen, hit test the stroke if not null
    __in_bcount_opt(pPenData->DashArraySize) double* pDashArray, // Dash array
    __in_ecount(cPoints) MilPoint2D       *pPoints,    // Points defining the path
    __in_ecount(cSegments) byte             *pTypes,     // Types defining the path
    __in UINT                               cPoints,     // Number of points
    __in UINT                               cSegments,   // Number of segments
    __in double                             rThreshold,  // Distance considered a hit
    __in bool                               fRelative,   // True if the threashold is relative       
    __in_ecount(1) MilPoint2D             *pHitPoint,  // The point to hit with
    __out_ecount(1) BOOL                    *pfIsHit)    // True if hit
{
    HRESULT hr = S_OK;
    CMILMatrix matrix(pMatrix);
    CShape shape;
    MilPoint2F hitPt;
    BOOL fIsNear;

    IFCNULL(pPoints);
    IFCNULL(pTypes);
    IFCNULL(pHitPoint);
    IFCNULL(pfIsHit);

    // Construct a CShape
    IFC(shape.AddFigureFromRawData(cPoints, cSegments, pPoints, pTypes, &matrix));

    // Convert the hit point to floats
    hitPt.X = (FLOAT) pHitPoint->X;
    hitPt.Y = (FLOAT) pHitPoint->Y;

    // Hit test
    if (pPenData)
    {
        // Hit testing a stroke
        CPlainPen pen;
        IFC(InitializePen(&pen, pPenData, pDashArray));

        IFC(shape.HitTestStroke(
            pen,
            hitPt,
            rThreshold,
            fRelative,
            NULL,  // matrix
            *pfIsHit,
            fIsNear));
    }
    else
    {
        // Hit testing a fill
        IFC(shape.HitTestFill(
            hitPt,
            rThreshold,
            fRelative,
            NULL,  // matrix
            *pfIsHit,
            fIsNear));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Function: MilUtility_PathGeometryHitTest
//
//  Synopsis: Hit test the fill or a stroke of a given path
//
//  Notes:    The path is as MIL_PATHGEOMETRY
//
//------------------------------------------------------------------------------
HRESULT WINAPI MilUtility_PathGeometryHitTest(
    __in_ecount_opt(1) MilMatrix3x2D       *pMatrix,    // Transformation matrix  
    __in_ecount(1) MilPenData             *pPenData,   // Pen, hit test the stroke if not null
    __in_bcount_opt(pPenData->DashArraySize) double* pDashArray, // Dash array
    __in MilFillMode::Enum                      fillRule,    // Fill mode
    __in_bcount(nSize) MilPathGeometry     *pPathData,  // The path data 
    __in UINT32                             nSize,       // The size of the above in bytes
    __in double                             rThreshold,  // Distance considered a hit
    __in bool                               fRelative,   // =true if the threshold is relative
    __in_ecount(1) MilPoint2D             *pHitPoint,  // The point to hit with
    __out_ecount(1) BOOL                    *pfIsHit)    // True if hit
{
    HRESULT hr = S_OK;

    Assert(nSize >= sizeof(MilPathGeometry));

    IFCNULL(pPathData);
    IFCNULL(pHitPoint);
    IFCNULL(pfIsHit);

    {
        BOOL fIsNear;
        CMILMatrix matrix(pMatrix);

        // Construct a CShapeBase
        PathGeometryData pathGeometry(
            pPathData,
            nSize,
            fillRule,
            matrix.IsIdentity() ? NULL : &matrix);

        // Convert the hit point to floats
        MilPoint2F hitPt;
        hitPt.X = (FLOAT) pHitPoint->X;
        hitPt.Y = (FLOAT) pHitPoint->Y;

        // Hit test
        if (pPenData)
        {
            // Hit testing a stroke
            CPlainPen pen;
            IFC(InitializePen(&pen, pPenData, pDashArray));

            IFC(pathGeometry.HitTestStroke(
                pen,
                hitPt,
                rThreshold,
                fRelative,
                NULL, // matrix
                *pfIsHit,
                fIsNear));
        }
        else
        {
            // Hit testing a fill
            IFC(pathGeometry.HitTestFill(
                hitPt,
                rThreshold,
                fRelative,
                NULL, // matrix
                *pfIsHit,
                fIsNear));
        }
    }

Cleanup:
    RRETURN(hr);
}
#undef DASH_COUNT

HRESULT WINAPI MilUtility_PathGeometryHitTestPathGeometry(
    __in_ecount_opt(1) MilMatrix3x2D *pMatrix1,
    IN MilFillMode::Enum fillRule1,
    __in_bcount(nSize1) MilPathGeometry *pPathData1,
    IN UINT32 nSize1,
    __in_ecount_opt(1) MilMatrix3x2D *pMatrix2,
    IN MilFillMode::Enum fillRule2,
    __in_bcount(nSize2) MilPathGeometry *pPathData2,
    IN UINT32 nSize2,
    IN double rTolerance,
    IN bool fRelative,      // =true if the tolerance is relative
    __out_ecount(1) MilPathsRelation::Enum *pRelation)
{
    HRESULT hr = S_OK;

    Assert(nSize1 >= sizeof(MilPathGeometry));
    Assert(nSize2 >= sizeof(MilPathGeometry));

    IFCNULL(pPathData1);
    IFCNULL(pPathData2);
    IFCNULL(pRelation);

    {
        CMILMatrix matrix1(pMatrix1);

        PathGeometryData pathGeometry1(
            pPathData1,
            nSize1,
            fillRule1,
            matrix1.IsIdentity() ? NULL : &matrix1);

        CMILMatrix matrix2(pMatrix2);

        PathGeometryData pathGeometry2(
            pPathData2,
            nSize2,
            fillRule2,
            matrix2.IsIdentity() ? NULL : &matrix2);

        IFC(pathGeometry1.GetRelation(pathGeometry2, rTolerance, fRelative, *pRelation));
    }

Cleanup:
    RRETURN(hr);
}


/*++

Routine Description:

    MilResource_Geometry_GetArea

--*/

HRESULT WINAPI MilUtility_GeometryGetArea(
    IN MilFillMode::Enum fillRule,
        // Path fill rule
    __in_ecount(nSize) MilPathGeometry *pPathData,
        // Path data
    IN UINT32 nSize,
        // Path data header size
    __in_ecount_opt(1) MilMatrix3x2D *pMatrix,
        // Transformation, NULL OK
    IN double rTolerance,
        // Flattening tolerance
    IN bool fRelative,
        // true if the tolerance is relative
    __out_ecount(1) double *pArea
        // The area
    )
{
    HRESULT hr = S_OK;

    Assert(nSize >= sizeof(MilPathGeometry));

    IFCNULL(pPathData);
    IFCNULL(pArea);

    {
        PathGeometryData data(pPathData, nSize, fillRule, NULL);

        if (pMatrix)
        {
            CMILMatrix matrix(pMatrix);
            IFC(data.GetArea(rTolerance, fRelative, &matrix, *pArea));
        }
        else
        {
            IFC(data.GetArea(rTolerance, fRelative, NULL, *pArea));
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Function:   MilUtility_GetArcAsBezier
//
//  Synopsis: Compute the approximation of a given arc with Bezier segments
//
//  Notes:    Returning cPieces = 0 indicates a line instead of an arc
//                      cPieces = -1 indicates that the arc degenerates to a point 
//
//------------------------------------------------------------------------------
void
MilUtility_ArcToBezier(
    __in MilPoint2D ptStart,
        // The arc's start point
    __in MilPoint2D rRadii,
        // The ellipse's X and Y radii
    __in double rotation,
        // Rotation angle of the ellipse's x axis
    __in bool fLargeArc,
        // Choose the larger of the 2 arcs if TRUE
    __in bool fSweepUp,
        // Sweep the arc increasing the angle if TRUE
    __in MilPoint2D ptEnd,
        // The arc's end point
    __in_ecount_opt(1) MilMatrix3x2D *pMatrix,
        // Transformation matrix
    __out_ecount(12) /* _part(12, cPieces < 0 ? 0 : (cPieces == 0 ? 1 : 3*cPieces) */ GpPointR *pPt,
        // An array receiving the Bezier points
    __out_ecount(1) int &cPieces)
        // The number of output Bezier segments
{
    MilPoint2F points[12];

    ArcToBezier(
        static_cast<FLOAT>(ptStart.X), 
        static_cast<FLOAT>(ptStart.Y),
        static_cast<FLOAT>(rRadii.X), 
        static_cast<FLOAT>(rRadii.Y), 
        static_cast<FLOAT>(rotation), 
        fLargeArc, 
        fSweepUp, 
        static_cast<FLOAT>(ptEnd.X),
        static_cast<FLOAT>(ptEnd.Y),
        OUT points, 
        OUT cPieces);

    // Prefast isn't catching that cPieces has __range(-1,4)
    __analysis_assume(-1 <= cPieces && cPieces <= 4);
    Assert(-1 <= cPieces && cPieces <= 4);

    if (cPieces >= 0)
    {
        int cPoints = (cPieces > 0) ? 3 * cPieces : 1;
        
        if (pMatrix)
        {
            CMILMatrix matrix(pMatrix);
            TransformPoints(matrix, cPoints, points, pPt);
        }
        else
        {
            for (int i = 0;  i < cPoints;  i++)
            {
                pPt[i].X = points[i].X;
                pPt[i].Y = points[i].Y;
            }
        }
    }
}




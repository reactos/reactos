// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Trapezoidal anti-aliasing implementation
//

struct CEdge;
struct CInactiveEdge;

MtExtern(CHwRasterizer);

//------------------------------------------------------------------------------
//
//  Class: CHwRasterizer
//
//  Description:
//      Trapezoidal AA implementation of IGeometryGenerator
//
//------------------------------------------------------------------------------

class CHwRasterizer : public IGeometryGenerator
{
public:
    DECLARE_BUFFERDISPENSER_NEW(CHwRasterizer, Mt(CHwRasterizer));
    DECLARE_BUFFERDISPENSER_DELETE;

    CHwRasterizer();

    //
    // Setup methods
    //

    HRESULT Setup(
        __in_ecount(1) CD3DDeviceLevel1       *pD3DDevice,
        __in_ecount(1) IShapeData       const *pShape,
        __inout_ecount(1) DynArray<MilPoint2F> *prgPointsScratch,
        __inout_ecount(1) DynArray<BYTE>      *prgTypesScratch,
        __in_ecount_opt(1) CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> const *pmatWorldToDevice
        );

    //
    // IGeometryGenerator methods
    //

    //+------------------------------------------------------------------------
    //
    //  Member:    GetPerVertexDataType
    //
    //  Synopsis:  Return vertex fields that are generated when this generator
    //             is used
    //
    //-------------------------------------------------------------------------

    override void GetPerVertexDataType(
        __out_ecount(1) MilVertexFormat &mvfFullyGenerated
        ) const
    {
        //
        // (X,Y) destination coordinate and alpha falloff (in diffuse) are
        // generated for each vertex.  The diffuse value is a 32 bit float and
        // not a fully generated vertex data member.  It must be multiplied by
        // a color to be ready for HW consumption.  Therefore it is not fully
        // generated.
        //

        mvfFullyGenerated = MILVFAttrXY;
    }

    //+-----------------------------------------------------------------------
    //
    //  Member:    SendGeometry
    //
    //  Synopsis:  Geometry is generated and passed to given sink.
    //
    //------------------------------------------------------------------------
    override HRESULT SendGeometry(
        __inout_ecount(1) IGeometrySink *pGeomSink
        );

    //+-----------------------------------------------------------------------
    //
    //  Member:    SendGeometryModifiers
    //
    //  Synopsis:  Generator has a chance to modify the pipeline colors in
    //             order to apply anti-aliasing, blend colors, etc.
    //
    //------------------------------------------------------------------------
    override HRESULT SendGeometryModifiers(
        __inout_ecount(1) CHwPipelineBuilder *pPipelineBuilder
        );

    //+-----------------------------------------------------------------------
    //
    //  Member:    SendLighting
    //
    //  Synopsis:  Geometry generator supplies lighting information to the
    //             pipeline.
    //
    //------------------------------------------------------------------------
    override HRESULT SendLighting(
        __inout_ecount(1) CHwPipelineBuilder *pPipelineBuilder
        )
    {
        UNREFERENCED_PARAMETER(pPipelineBuilder);

        //
        // HwRasterizer is 2D only, so it doesn't have lighting information.
        //

        return S_OK;
    }

private:
    HRESULT RasterizePath(
        __in_ecount(cPoints)   const MilPoint2F *rgpt,
        __in_ecount(cPoints)   const BYTE *rgTypes,
        const UINT cPoints,
        __in_ecount(1) const CMILMatrix *pmatWorldTransform,
        MilFillMode::Enum fillMode
        );

    HRESULT RasterizeEdges(
        __inout_ecount(1) CEdge *pEdgeActiveList,
        __inout_ecount(1) CInactiveEdge *pInactiveEdgeArray,
        INT nSubpixelYCurrent,
        INT nSubpixelYBottom
        );

    float ConvertSubpixelXToPixel(
        INT x, 
        INT error, 
        FLOAT rErrorDown
        );

    float ConvertSubpixelYToPixel(
        INT nSubpixel
        );

    INT ComputeTrapezoidsEndScan(
        __in_ecount(1) const CEdge *pEdgeCurrent,
        INT nSubpixelYCurrent,
        INT nSubpixelYNextInactive
        );

    HRESULT OutputTrapezoids(
        __inout_ecount(1) CEdge *pEdgeCurrent,
        INT nSubpixelYCurrent, // inclusive
        INT nSubpixelYNext     // exclusive
        );

    HRESULT GenerateOutputAndClearCoverage(
        INT nSubpixelY
        );

private:
    //
    // Local fill state
    //

    DynArray<MilPoint2F> *m_prgPoints;
    DynArray<BYTE>       *m_prgTypes;
    MilPointAndSizeL      m_rcClipBounds;
    CMILMatrix            m_matWorldToDevice;
    IGeometrySink        *m_pIGeometrySink;
    MilFillMode::Enum     m_fillMode;

    //
    // Complex scan coverage buffer
    //

    CCoverageBuffer m_coverageBuffer;

    CD3DDeviceLevel1 * m_pDeviceNoRef;
};




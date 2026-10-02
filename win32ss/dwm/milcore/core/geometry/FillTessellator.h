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
//      Contains declarations of CFillTesselator classes
//
//      CFillTesselators generate tessellations for Figures
//
//  $ENDTAG
//
//------------------------------------------------------------------------------


MtExtern(CRectFillTessellator);
MtExtern(CRegionFillTessellator);
MtExtern(CGeneralFillTessellator);

//+-----------------------------------------------------------------------------
//
//  Class:
//      CFillTessellator
//
//  Synopsis:
//      Base class for all fill tessellators
//
//------------------------------------------------------------------------------

class CFillTessellator : public IGeometryGenerator
{
public:

    DECLARE_BUFFERDISPENSER_DELETE

    CFillTessellator(
        __in_ecount_opt(1) const CBaseMatrix *pMatrix)
            // Transformation matrix (NULL OK)

        : m_pMatrix(CMILMatrix::ReinterpretBase(pMatrix))
    {
#if DBG
        m_fDbgDestroyed = false;
#endif DBG
    }

    virtual ~CFillTessellator()
    {
#if DBG
        Assert(!m_fDbgDestroyed);
        m_fDbgDestroyed = true;
#endif DBG
    }
    
    //
    // IGeometryGenerator methods
    //

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      GetPerVertexDataType
    //
    //  Synopsis:
    //      Return vertex fields that are generated when this generator is used
    //
    //--------------------------------------------------------------------------

    void GetPerVertexDataType(
        __out_ecount(1) MilVertexFormat &mvfFullyGenerated
        ) const override
    {
        // (X,Y) destination coordinate are generated for each vertex.
        mvfFullyGenerated = MILVFAttrXY;
    }

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      SendGeometryModifiers
    //
    //  Synopsis:
    //      Generator has a chance to modify the pipeline colors in order to
    //      apply anti-aliasing, blend colors, etc.
    //
    //--------------------------------------------------------------------------
    HRESULT SendGeometryModifiers(
        __inout_ecount(1) CHwPipelineBuilder *pPipelineBuilder
        ) override
    {
        UNREFERENCED_PARAMETER(pPipelineBuilder);

        //
        // Fill Tessellator doesn't have any Anti-Aliasing color sources to
        // send to the pipeline.
        //

        return S_OK;
    }

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      SendLighting
    //
    //  Synopsis:
    //      Geometry generator supplies lighting information to the pipeline.
    //
    //--------------------------------------------------------------------------
    HRESULT SendLighting(
        __inout_ecount(1) CHwPipelineBuilder *pPipelineBuilder
        ) override
    {
        UNREFERENCED_PARAMETER(pPipelineBuilder);

        //
        // Fill Tessellator is 2D only, so it doesn't have lighting information.
        //

        return S_OK;
    }

private:

    // Disallow default constructor
    CFillTessellator()
    {
        Assert(false);
    }

#if DBG
    bool m_fDbgDestroyed;     // Used to check single Release pattern
#endif DBG

protected:
    // Data
    const CMILMatrix     *m_pMatrix;    // Transformation matrix
};


//+-----------------------------------------------------------------------------
//
//  Class:
//      CSpecialCaseFillTessellator
//
//  Synopsis:
//      Helper class for tessellating special case figures
//
//  Notes:
//      This is an the base class that provides common services for the
//      tessellators of special shapes - rectangle, ellipse, and rounded
//      rectangle.
//
//------------------------------------------------------------------------------
class CSpecialCaseFillTessellator : public CFillTessellator
{
protected:

    // Constructor destructor
    MIL_FORCEINLINE CSpecialCaseFillTessellator(
        __in_ecount_opt(1) const CBaseMatrix *pMatrix);
            // Transformation matrix (NULL OK)

    virtual ~CSpecialCaseFillTessellator()
    {
        // nothing
    }

    HRESULT TessellateFigure(
        __in_ecount(1) const IFigureData &figure,
            // The figure
        __inout_ecount(1) IGeometrySink *pgs);
            // Geometry sink for the resulting tessellation

};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CRectFillTessellator
//
//  Synopsis:
//      Optimized class for tessellating a rectangle
//
//------------------------------------------------------------------------------
class CRectFillTessellator  :   public CSpecialCaseFillTessellator
{
public:

    DECLARE_BUFFERDISPENSER_NEW(CRectFillTessellator, Mt(CRectFillTessellator))

    // Constructor destructor
    CRectFillTessellator(
        __in_ecount(1) const IFigureData &figure,
            // The figure to tessellate
        __in_ecount_opt(1) const CBaseMatrix *pMatrix);
            // Transformation (NULL OK)

    ~CRectFillTessellator()
    {
    }

    // IGeometryGenerator methods
    HRESULT SendGeometry(
        __inout_ecount(1) IGeometrySink *pgs);
            // Geometry sink to tessellate into

private:

    // Data
    const IFigureData &m_figure; // The figure
};


//+-----------------------------------------------------------------------------
//
//  Class:
//      CRegionFillTessellator
//
//  Synopsis:
//      Optimized class for tessellating a multi-rectangle shape
//
//------------------------------------------------------------------------------
class CRegionFillTessellator  :   public CSpecialCaseFillTessellator
{
public:

    DECLARE_BUFFERDISPENSER_NEW(CRectFillTessellator, Mt(CRectFillTessellator))

    // Constructor destructor
    CRegionFillTessellator(
        __in_ecount(1) const IShapeData &shape,
            // The shape to tessellate
        __in_ecount_opt(1) const CBaseMatrix *pMatrix);
            // Transformation (NULL OK)

    ~CRegionFillTessellator()
    {
    }

    // IGeometryGenerator methods
    virtual HRESULT SendGeometry(__inout_ecount(1) IGeometrySink *pGeomBuffer);
private:

    // Data
    const IShapeData &m_shape; // The shape
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CGeneralFillTessellator
//
//  Synopsis:
//      Helper class for tessellating general figures
//
//  Notes:
//      This class essentially provides the CFillTessellator interface for using
//      CTessellator.  The purpose of this wrapping is to keep CTessellator as
//      simple as possible, so that it can be used outside Avalon.
//
//------------------------------------------------------------------------------
MtExtern(CGeneralFillTessellator);

class CGeneralFillTessellator : public CFillTessellator
{
public:
    DECLARE_BUFFERDISPENSER_NEW(CGeneralFillTessellator, Mt(CGeneralFillTessellator))

    CGeneralFillTessellator(
        __in_ecount(1) const IShapeData &shape,
            // Shape to tessellate
        __in_ecount_opt(1) const CBaseMatrix *pMatrix)
            // Transformation matrix
        
        : CFillTessellator(pMatrix),
          m_shape(shape)
    {
    }

    virtual ~CGeneralFillTessellator()
    {
    }
    
    // IGeometryGenerator methods
    virtual HRESULT SendGeometry(__inout_ecount(1) IGeometrySink *pGeomBuffer);

    // Data
private:
    const IShapeData          &m_shape;     // Shape to tessellate
};




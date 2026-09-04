// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_software
//      $Keywords:
//
//  $Description:
//      Software Rasterizer
//
//      The software rasterizer (SR) scan converts a primitive, feeding the
//      scanlines to a CScanPipeline.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

class CAntialiasedFiller;

//+-----------------------------------------------------------------------------
//
//  Class:
//      CColorSource
//
//  Synopsis:
//      Base for classes which generate colors (mostly for different types of
//      brush).
//
//------------------------------------------------------------------------------

class CColorSource : public OpSpecificData
{
public:
    virtual ~CColorSource() {}
    virtual VOID ReleaseExpensiveResources() = 0;
    virtual ScanOpFunc GetScanOp() const = 0;
    virtual MilPixelFormat::Enum GetPixelFormat() const = 0;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      COutputSpan
//
//  Synopsis:
//      A base interface which receives a "span" - the location and size of a
//      horizontal group of pixels.
//
//      This interface is intended to be used only by:
//      1) CSpanClipper
//      2) CSpanSink
//
//      For proposed additional uses, first ask if it would make more sense to
//      add a new type of scan operation, to be inserted into a CScanPipeline.
//
//------------------------------------------------------------------------------

class COutputSpan
{
public:
    virtual ~COutputSpan() {}

    virtual void OutputSpan(INT y, INT xMin, INT xMax) = 0;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CSpanClipper, COutputSpan
//
//  Synopsis:
//      Clips the input spans according to some kind of clipping data. For each
//      call to OutputSpan, the CSpanClipper implementor will send the unclipped
//      portion to the sink, via zero, one, or more calls to the sink's
//      OutputSpan method.
//
//------------------------------------------------------------------------------

class CSpanClipper : public COutputSpan
{
public:
    virtual void GetClipBounds(__in_ecount(1) CMILSurfaceRect *prc) const = 0;
    virtual void SetOutputSpan(COutputSpan *pOutputSpan) = 0;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CSpanSink
//
//  Synopsis:
//      Consumer of spans produced by a software rasterizer.
//
//      AddDirtyRect is called once to let the render target know that the
//      rasterizer will be changing this rect per call - in essense this is for
//      efficiency purposes only, since the render target could extract this
//      information from the OutputSpan calls.
//
//  Notes:
//      pSpanClipper is only needed for masking (via CMaskClipper). Real
//      clipping is not done by the span sink.
//
//------------------------------------------------------------------------------

class CSpanSink : public COutputSpan
{
public:
    virtual HRESULT SetupPipeline(
        MilPixelFormat::Enum fmtColorData,              // Either 32bppPARGB or
                                                  // 128bppPABGR.
        CColorSource *pColorSource,
        BOOL fPPAA,
        bool fComplementAlpha,
        MilCompositingMode::Enum eCompositingMode,
        CSpanClipper *pSpanClipper,               // See Notes.
        IMILEffectList *pIEffectList,             // Can be NULL.
        const CMatrix<CoordinateSpace::Effect,CoordinateSpace::Device>
            *pmatEffectToDevice,                  // Needed only when
                                                  // pIEffectList != NULL
        const CContextState *pContextState        // Needed only when
                                                  // pIEffectList != NULL
        ) = 0;

    virtual HRESULT SetupPipelineForText(
        CColorSource *pColorSource,
        MilCompositingMode::Enum eCompositingMode,
        CSWGlyphRunPainter &painter,
        bool fNeedsAA
        ) = 0;

    // Release expensive resources. The rasterizer *must* call this when it is
    // done with the span sink, but before returning control. (Assertions in the
    // next ScanPipeline call will verify this.)
    //
    // Failing to do this will cause a kind of resource leak - although someone
    // owns the resources and will release them eventually (on the next render,
    // or during later destruction), use cases exist in which an arbitrarily
    // large amount of resources can be held unused for an arbitrarily long
    // time.
    //
    // The implementation is free to retain some of the resources as a cache.
    // But to do this without risk of leaking, it needs to have an upper limit
    // on how much it caches, and probably also communicate with other similar
    // objects to limit the global resource load.

    virtual VOID ReleaseExpensiveResources() = 0;

    // When per-primitive antialiasing is used, this function passes the
    // coverage data down to the scan pipeline (to be used by
    // ScalePPAACoverage).
    virtual VOID SetAntialiasedFiller(CAntialiasedFiller *pFiller) = 0;
};

class CColorSourceCreator
{
public:
    virtual ~CColorSourceCreator() {}

    virtual MilPixelFormat::Enum GetPixelFormat() const = 0;
    virtual MilPixelFormat::Enum GetSupportedSourcePixelFormat(
        MilPixelFormat::Enum fmtSourceGiven,
        bool fForceAlpha
        ) const = 0;

    // The color sources are returned through "GetCS_*" and "ReleaseCS" calls.
    // From the caller's POV they are not "creation" calls because the caller
    // doesn't gain ownership of the memory (and they aren't refcounted either).

    virtual VOID ReleaseCS(CColorSource *pColorSource) = 0;

    virtual HRESULT GetCS_Constant(
        const MilColorF *pColor,
        OUT CColorSource **ppColorSource
        ) = 0;

    virtual HRESULT GetCS_EffectShader(
        __in const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::DeviceHPC> *pRealizationSamplingToDevice,
        __inout CMILBrushShaderEffect* pShaderEffectBrush,
        __deref_out CColorSource **ppColorSource
        ) = 0;   

    virtual HRESULT GetCS_LinearGradient(
        const MilPoint2F *pGradientPoints,
        UINT nColorCount,
        const MilColorF *pColors,
        const FLOAT *pPositions,
        MilGradientWrapMode::Enum wrapMode,
        MilColorInterpolationMode::Enum colorInterpolationMode,
        const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatWorldHPCToDeviceHPC,
        OUT CColorSource **ppColorSource
        ) = 0;

    virtual HRESULT GetCS_RadialGradient(
        const MilPoint2F *pGradientPoints,
        const UINT nColorCount,
        const MilColorF *pColors,
        const FLOAT *pPositions,
        MilGradientWrapMode::Enum wrapMode,
        MilColorInterpolationMode::Enum colorInterpolationMode,
        const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatWorldHPCToDeviceHPC,
        OUT CColorSource **ppColorSource
        ) = 0;

    virtual HRESULT GetCS_FocalGradient(
        const MilPoint2F *pGradientPoints,
        UINT nColorCount,
        const MilColorF *pColors,
        const FLOAT *pPositions,
        MilGradientWrapMode::Enum wrapMode,
        MilColorInterpolationMode::Enum colorInterpolationMode,
        const MilPoint2F *pptOrigin,
        const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatWorldHPCToDeviceHPC,
        OUT CColorSource **ppColorSource
        ) = 0;


    HRESULT GetCS_PrefilterAndResample(
        __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
        MilBitmapWrapMode::Enum wrapMode,
        __in_ecount_opt(1) const MilColorF *pBorderColor,
        __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatTextureHPCToDeviceHPC,
        MilBitmapInterpolationMode::Enum interpolationMode,
        bool prefilterEnable,
        FLOAT prefilterThreshold,
        __inout_ecount_opt(1) IMILResourceCache *pICacheAlternate,
        __deref_out_ecount(1) CColorSource **ppColorSource
        );


protected:

    virtual HRESULT GetCS_Resample(
        __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
        MilBitmapWrapMode::Enum wrapMode,
        __in_ecount_opt(1) const MilColorF *pBorderColor,
        __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatTextureHPCToDeviceHPC,
        MilBitmapInterpolationMode::Enum interpolationMode,
        __deref_out_ecount(1) CColorSource **ppColorSource
        ) = 0;

};

class CRenderState;
class CShape;
class CPlainPen;
class CMILBrush;

class CConstantColorBrushSpan;
class CLinearGradientBrushSpan;
class CRadialGradientBrushSpan;
class CFocalGradientBrushSpan;
class CShaderEffectBrushSpan;

class CIdentitySpan;
class CNearestNeighborSpan;
class CBilinearSpan;
class CUnoptimizedBilinearSpan;
class CBilinearSpan_MMX;

class CConstantColorBrushSpan_scRGB;
class CLinearGradientBrushSpan_scRGB;
class CRadialGradientBrushSpan_scRGB;
class CFocalGradientBrushSpan_scRGB;

class CNearestNeighborSpan_scRGB;
class CBilinearSpan_scRGB;

class CConstantAlphaSpan;
class CMaskAlphaSpan;

class CConstantAlphaSpan_scRGB;
class CMaskAlphaSpan_scRGB;


// Future Consideration: 
// Remove this non-optimized codepath once Intel integration is complete
#define ENABLE_INTEL_OPTIMIZED_BILINEAR

class CResampleSpanCreator_sRGB
{
public:
    CResampleSpanCreator_sRGB();
    ~CResampleSpanCreator_sRGB();

    HRESULT GetCS_Resample(
        __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
        MilBitmapWrapMode::Enum wrapMode,
        __in_ecount_opt(1) const MilColorF *pBorderColor,
        __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatTextureHPCToDeviceHPC,
        MilBitmapInterpolationMode::Enum interpolationMode,
        __deref_out_ecount(1) CColorSource **ppColorSource
        );

private:
    CIdentitySpan *m_pIdentitySpan;
    CNearestNeighborSpan *m_pNearestNeighborSpan;

    CBilinearSpan_MMX *m_pBilinearSpanMMX;

// Future Consideration: 
// Remove the non-optimized codepath once Intel integration is complete
#ifndef ENABLE_INTEL_OPTIMIZED_BILINEAR
    CUnoptimizedBilinearSpan *m_pUnoptimizedBilinearSpan;
#else
    CBilinearSpan *m_pBilinearSpan;
#endif
};

class CColorSourceCreator_sRGB : public CColorSourceCreator
{
public:
    CColorSourceCreator_sRGB();
    virtual ~CColorSourceCreator_sRGB();
    MilPixelFormat::Enum GetPixelFormat() const override { return MilPixelFormat::PBGRA32bpp; }
    MilPixelFormat::Enum GetSupportedSourcePixelFormat(
        MilPixelFormat::Enum fmtSourceGiven,
        bool fForceAlpha
    ) const override;

    VOID ReleaseCS(CColorSource *pColorSource) override;

    HRESULT GetCS_Constant(
        const MilColorF *pColor,
        OUT CColorSource **ppColorSource
        ) override;

    HRESULT GetCS_EffectShader(
        __in const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::DeviceHPC> *pRealizationSamplingToDevice,
        __inout CMILBrushShaderEffect* pShaderEffectBrush,
        __deref_out CColorSource **ppColorSource
        ) override;

    HRESULT GetCS_LinearGradient(
        const MilPoint2F *pGradientPoints,
        UINT nColorCount,
        const MilColorF *pColors,
        const FLOAT *pPositions,
        MilGradientWrapMode::Enum wrapMode,
        MilColorInterpolationMode::Enum colorInterpolationMode,
        const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatWorldHPCToDeviceHPC,
        OUT CColorSource **ppColorSource
        ) override;

    HRESULT GetCS_RadialGradient(
        const MilPoint2F *pGradientPoints,
        const UINT nColorCount,
        const MilColorF *pColors,
        const FLOAT *pPositions,
        MilGradientWrapMode::Enum wrapMode,
        MilColorInterpolationMode::Enum colorInterpolationMode,
        const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatWorldHPCToDeviceHPC,
        OUT CColorSource **ppColorSource
        ) override;

    HRESULT GetCS_FocalGradient(
        const MilPoint2F *pGradientPoints,
        UINT nColorCount,
        const MilColorF *pColors,
        const FLOAT *pPositions,
        MilGradientWrapMode::Enum wrapMode,
        MilColorInterpolationMode::Enum colorInterpolationMode,
        const MilPoint2F *pptOrigin,
        const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatWorldHPCToDeviceHPC,
        OUT CColorSource **ppColorSource
        ) override;

    HRESULT GetCS_Resample(
        __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
        MilBitmapWrapMode::Enum wrapMode,
        __in_ecount_opt(1) const MilColorF *pBorderColor,
        __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatTextureHPCToDeviceHPC,
        MilBitmapInterpolationMode::Enum interpolationMode,
        __deref_out_ecount(1) CColorSource **ppColorSource
        ) override;

private:
    CConstantColorBrushSpan *m_pConstantColorSpan;
    CLinearGradientBrushSpan *m_pLinearGradientSpan;
    CRadialGradientBrushSpan *m_pRadialGradientSpan;
    CFocalGradientBrushSpan *m_pFocalGradientSpan;
    CShaderEffectBrushSpan *m_pShaderEffectSpan;
    CResampleSpanCreator_sRGB m_ResampleSpans;
};

class CResampleSpanCreator_scRGB
{
public:
    CResampleSpanCreator_scRGB();
    ~CResampleSpanCreator_scRGB();

    HRESULT GetCS_Resample(
        __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
        MilBitmapWrapMode::Enum wrapMode,
        __in_ecount_opt(1) const MilColorF *pBorderColor,
        __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatTextureHPCToDeviceHPC,
        MilBitmapInterpolationMode::Enum interpolationMode,
        __deref_out_ecount(1) CColorSource **ppColorSource
        );

};

class CColorSourceCreator_scRGB : public CColorSourceCreator
{
public:
    CColorSourceCreator_scRGB();
    virtual ~CColorSourceCreator_scRGB();
    MilPixelFormat::Enum GetPixelFormat() const override { return MilPixelFormat::PRGBA128bppFloat; }
    MilPixelFormat::Enum GetSupportedSourcePixelFormat(
        MilPixelFormat::Enum,
        bool fForceAlpha
        ) const override
    {
        return MilPixelFormat::PRGBA128bppFloat;
    }

    VOID ReleaseCS(CColorSource *pColorSource) override;

    HRESULT GetCS_Constant(
        const MilColorF *pColor,
        OUT CColorSource **ppColorSource
        ) override;

    HRESULT GetCS_EffectShader(
        __in const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::DeviceHPC> *pRealizationSamplingToDevice,
        __inout CMILBrushShaderEffect* pShaderEffectBrush,
        __deref_out CColorSource **ppColorSource
        ) override;

    HRESULT GetCS_LinearGradient(
        const MilPoint2F *pGradientPoints,
        UINT nColorCount,
        const MilColorF *pColors,
        const FLOAT *pPositions,
        MilGradientWrapMode::Enum wrapMode,
        MilColorInterpolationMode::Enum colorInterpolationMode,
        const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatWorldHPCToDeviceHPC,
        OUT CColorSource **ppColorSource
        ) override;

    HRESULT GetCS_RadialGradient(
        const MilPoint2F *pGradientPoints,
        const UINT nColorCount,
        const MilColorF *pColors,
        const FLOAT *pPositions,
        MilGradientWrapMode::Enum wrapMode,
        MilColorInterpolationMode::Enum colorInterpolationMode,
        const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatWorldHPCToDeviceHPC,
        OUT CColorSource **ppColorSource
        ) override;

    HRESULT GetCS_FocalGradient(
        const MilPoint2F *pGradientPoints,
        UINT nColorCount,
        const MilColorF *pColors,
        const FLOAT *pPositions,
        MilGradientWrapMode::Enum wrapMode,
        MilColorInterpolationMode::Enum colorInterpolationMode,
        const MilPoint2F *pptOrigin,
        const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatWorldHPCToDeviceHPC,
        OUT CColorSource **ppColorSource
        ) override;

    HRESULT GetCS_Resample(
        __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
        MilBitmapWrapMode::Enum wrapMode,
        __in_ecount_opt(1) const MilColorF *pBorderColor,
        __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatTextureHPCToDeviceHPC,
        MilBitmapInterpolationMode::Enum interpolationMode,
        __deref_out_ecount(1) CColorSource **ppColorSource
        ) override;

};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CSoftwareRasterizer
//
//  Synopsis:
//      Rasterizes primitives, by performing the following steps.
//
//      * Rasterizer scan-converts the primitive (produces 'spans').
//      * An CSpanClipper clips the spans.
//      * An CSpanSink consumes the spans. (Most types of CSpanSink will
//        use a CScanPipeline here.)
//

class CSoftwareRasterizer
{
public:
    CSoftwareRasterizer();
    ~CSoftwareRasterizer();

    // Methods for satisfying individual calls for various primitives

    HRESULT DrawBitmap(
        __inout_ecount(1) CSpanSink *pSpanSink,
        __in_ecount(1) CSpanClipper *pClipper,
        __in_ecount(1) const CContextState *pContextState,
        __in_ecount(1) IWGXBitmapSource *pISurface,
        __in_ecount_opt(1) IMILEffectList *pIEffect
        );

    HRESULT FillPathUsingBrushRealizer(
        __inout_ecount(1) CSpanSink *pSpanSink,
        MilPixelFormat::Enum fmtTarget,
        DisplayId associatedDisplay,
        __inout_ecount(1) CSpanClipper *pSpanClipper,
        __in_ecount(1) const CContextState *pContextState,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __in_ecount_opt(1) const IShapeData *pShape,
        __in_ecount_opt(1) const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *pShapeToDevice, // (NULL OK)
        __in_ecount(1) CBrushRealizer *pBrush,
        __in_ecount(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> &matWorldToDevice
        DBG_STEP_RENDERING_COMMA_PARAM(__inout_ecount(1) ISteppedRenderingDisplayRT *pDisplayRTParent)
        );
    
    HRESULT FillPath(
        __inout_ecount(1) CSpanSink *pSpanSink,
        __inout_ecount(1) CSpanClipper *pSpanClipper,
        __in_ecount(1) const CContextState *pContextState,
        __in_ecount_opt(1) const IShapeData *pShape,         // NULL treated as infinite shape
        __in_ecount_opt(1) const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *pShapeToDevice, // NULL OK
        __in_ecount(1) CMILBrush *pBrush,
        __in_ecount(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> &matWorldToDevice,
        __in_ecount_opt(1) IMILEffectList *pIEffect,
        float rComplementFactor = -1,
        __in_ecount_opt(1) const CMILSurfaceRect *prcComplementBounds = NULL
        );


    HRESULT DrawGlyphRun(
        __inout_ecount(1) CSpanSink *pSpanSink,
        __in_ecount(1) CSpanClipper *pClipper,
        __inout_ecount(1) DrawGlyphsParameters &pars,
        __inout_ecount(1) CMILBrush *pBrush,
        FLOAT flEffectAlpha,
        __in_ecount(1) CGlyphPainterMemory *pGlyphPainterMemory,
        bool fTargetSupportsClearType,
        __out_opt bool* pfClearTypeUsedToRender = NULL
        );

    // Clear the device.

    HRESULT Clear(
        __inout_ecount(1) CSpanSink *pSpanSink,
        __in_ecount(1) CSpanClipper *pSpanClipper,
        __in_ecount(1) const MilColorF *pColor
        );

    // Modify the pixelformat to be used for color data
    void SetColorDataPixelFormat(MilPixelFormat::Enum fmtPixels);

    // In a CScanPipelineRendering we support either 32bppPARGB and 32bppRGB
    // or 128bbpPARGB.
    // 
    // 32bppRGB support is needed so that we can use RGB destination render
    // targets as color sources without doing an upfront conversion.
    static bool IsValidPixelFormat32(MilPixelFormat::Enum pixelFormat)
    {
        return
            pixelFormat == MilPixelFormat::PBGRA32bpp ||
            pixelFormat == MilPixelFormat::BGR32bpp;
    }

    static bool IsValidPixelFormat(MilPixelFormat::Enum pixelFormat)
    {
        return
            pixelFormat == MilPixelFormat::PRGBA128bppFloat ||
            IsValidPixelFormat32(pixelFormat);
    }


private:
    HRESULT GetCS_Brush(
        CMILBrush *pBrush,
        __in_ecount(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::DeviceHPC> &matWorldHPCToDeviceHPC,
        const CContextState *pContextState,
        OUT CColorSource **ppColorSource
        );

private:

    //
    // Points and types arrays, the rasterizer needs this input
    // which is provided by the geometry library
    //

    DynArray<MilPoint2F> m_rgPoints;
    DynArray<BYTE> m_rgTypes;

    //
    // This class creates the color sources needed by the rasterizer.
    //

    CColorSourceCreator_sRGB m_Creator_sRGB;

    CColorSourceCreator *m_pCSCreator;
    MilPixelFormat::Enum m_fmtColorSource;     // Either 32bppPARGB, or 128bppPABGR.
};





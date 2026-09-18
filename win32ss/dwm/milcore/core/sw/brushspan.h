// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_brush
//      $Keywords:
//
//  $Description:
//      Color sources which generate colors for various brush types. "Span" is
//      obsolete - these classes don't actually handle spans.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CConstantColorBrushSpan);
MtExtern(CLinearGradientBrushSpan);
MtExtern(CLinearGradientBrushSpan_MMX);
MtExtern(CRadialGradientBrushSpan);
MtExtern(CFocalGradientBrushSpan);
MtExtern(CShaderEffectBrushSpan);

//+-----------------------------------------------------------------------------
//
//  Class:
//      CConstantColorBrushSpan
//
//  Synopsis:
//      CColorSource implementation that emits a constant color
//
//------------------------------------------------------------------------------
class CConstantColorBrushSpan : public CColorSource
{
public:
    
    CConstantColorBrushSpan();
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CConstantColorBrushSpan));

    HRESULT Initialize(
        __in_ecount(1) const MilColorF *pColor
        );
    
    // CColorSource interface
    friend VOID FASTCALL ColorSource_Constant_32bppPARGB(
        __in_ecount(1) const PipelineParams *, 
        __in_ecount(1) const ScanOpParams *
        );

    ScanOpFunc GetScanOp() const override { return ColorSource_Constant_32bppPARGB; }
    MilPixelFormat::Enum GetPixelFormat() const override { return MilPixelFormat::PBGRA32bpp; }

    virtual VOID ReleaseExpensiveResources() {}  // No expensive resources are
                                                 // needed for a constant color
                                                 // brush.
protected:

    ARGB m_Color;
};


//+-----------------------------------------------------------------------------
//
//  Class:
//      CGradientBrushSpan
//
//  Synopsis:
//      Base sRGB gradient span class that initalizes the gradient texture used
//      by all the gradient span classes.
//
//------------------------------------------------------------------------------
class CGradientBrushSpan : public CColorSource
{
public:
    CGradientBrushSpan()
    {
    }
    
    MilPixelFormat::Enum GetPixelFormat() const  override { return MilPixelFormat::PBGRA32bpp; }

protected:
    
    HRESULT InitializeTexture(
        __in_ecount(1) const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatWorldHPCToDeviceHPC,
        __in_ecount(3) const MilPoint2F *pGradientPoints,
        __in BOOL fRadialGradient,
        __in_ecount(uCount) const MilColorF *pColors,
        __in_ecount(uCount) const FLOAT *pPositions,
        __in UINT uCount,
        __in MilGradientWrapMode::Enum wrapMode,
        __in MilColorInterpolationMode::Enum colorInterpolationMode,
        __out_ecount(1) CMILMatrix *pmatDeviceIPCtoGradientTextureHPC 
        );

protected:

    UINT m_uTexelCount;             // Number of texels in the gradient texture

    UINT m_uTexelCountMinusOne;     // One less than m_uTexelCount.  

    FLOAT m_flGradientSpanEnd;

    // Heap-allocating textures of the required size is slower than always allocating
    // the largest-sized textures on the stack.  
    union
    {
        ULONGLONG m_rgStartTexelArgb[MAX_GRADIENTTEXEL_COUNT];
                                    // Array of colors (at 16-bits per channel,
                                    //   with zeroes in the significant bytes)
                                    //   representing the start color of the
                                    //   linear approximation at interval 'x'
                                    //   (in A-R-G-B format)
        AGRB64TEXEL m_rgStartTexelAgrb[MAX_GRADIENTTEXEL_COUNT];
                                    // Similarly, but for the non-MMX renderer
                                    //   (in A-G-R-B format)
    };
    union
    {
        ULONGLONG m_rgEndTexelArgb[MAX_GRADIENTTEXEL_COUNT];
                                    // End color for the interval (in A-R-G-B
                                    //   format)
        AGRB64TEXEL m_rgEndTexelAgrb[MAX_GRADIENTTEXEL_COUNT];
                                    // Similarly, but for the non-MMX renderer
                                    //   (in A-G-R-B format)
    };

    MilGradientWrapMode::Enum m_wrapMode;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CLinearGradientBrushSpan
//
//  Synopsis:
//      This class performs texture resampling that is optimized for
//      one-dimensional gradient textures.  An analysis of the regressions
//      caused by removing this class and replacing it with the general
//      resampling mechanism, CResampleSpan
//
//------------------------------------------------------------------------------
class CLinearGradientBrushSpan : public CGradientBrushSpan
{
public:
    
    CLinearGradientBrushSpan();
    virtual ~CLinearGradientBrushSpan();
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CLinearGradientBrushSpan));

    HRESULT Initialize(
        __in_ecount(1) const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatWorldHPCToDeviceHPC,
        __in_ecount(3) const MilPoint2F *pGradientPoints,
        __in_ecount(uCount) const MilColorF *pColors,
        __in_ecount(uCount) const FLOAT *pPositions,
        __in UINT uCount,
        __in MilGradientWrapMode::Enum wrapMode,
        __in MilColorInterpolationMode::Enum colorInterpolationMode
        );


    virtual ScanOpFunc GetScanOp() const { return ColorSource_LinearGradient_32bppPARGB; }

    virtual VOID ReleaseExpensiveResources();

protected:

    INT MatrixValueToFix16(FLOAT value);

    MIL_FORCEINLINE void GenerateColorsInit(
        __in INT nX,
        __in INT nY,
        __in INT nCount,
        __out_ecount(1) INT *pnTexturePositionIPC,
        __out_ecount(1) INT *pnXIncrement
        );

private:

    VOID GenerateColors(
        __in INT nX, 
        __in INT nY, 
        __in INT nCount, 
        __out_ecount_full(nCount) ARGB *pArgbDest
        );
    
    friend VOID FASTCALL ColorSource_LinearGradient_32bppPARGB(
        __in_ecount(1) const PipelineParams *, 
        __in_ecount(1) const ScanOpParams *
        );

protected:

    INT m_nM11;                     // Fixed point representation of
                                    //   M11 element of DeviceToNormalized
    INT m_nM21;                     // Fixed point representation of
                                    //   M21 element of DeviceToNormalized
    INT m_nDx;                      // Fixed point representation of
                                    //   Dx element of DeviceToNormalized
    INT m_nXIncrement;              // Fixed point increment (in format
                                    //   defined by ONEDNUMFRACTIONALBITS)
                                    //   representing texture x-distance
                                    //   traveled for every x pixel increment
                                    //   in device space
};


//+-----------------------------------------------------------------------------
//
//  Member:
//      CLinearGradientBrushSpan::GenerateColorsInit
//
//  Synopsis:
//      Initializes the texture position and increment, accounting for overflow
//

void
CLinearGradientBrushSpan::GenerateColorsInit(
    __in INT nX,
    __in INT nY,
    __in INT nCount,
    __out_ecount(1) INT *pnTexturePositionIPC,
    __out_ecount(1) INT *pnXIncrement
    )
{
    *pnXIncrement = m_nXIncrement;

    if (m_wrapMode == MilGradientWrapMode::Extend)
    {
        LONGLONG llTexturePositionIPC =
              (static_cast<LONGLONG>(nX) * static_cast<LONGLONG>(m_nM11))
            + (static_cast<LONGLONG>(nY) * static_cast<LONGLONG>(m_nM21))
            + static_cast<LONGLONG>(m_nDx);

        if (llTexturePositionIPC < INT_MIN)
        {
            //
            // We are going to underflow Fix16 range. Choose the start color
            // for the whole span
            //
            Assert(0 == GpIntToFix16(0));
            *pnTexturePositionIPC = 0; 
            *pnXIncrement = 0;
        }
        else if ((llTexturePositionIPC
                  + (static_cast<LONGLONG>(m_nXIncrement) * static_cast<LONGLONG>(nCount))
                  + (FIX16_ONE - 1) // cut off at end color, without bleeding into start color
                 ) > INT_MAX
                )
        {
            //
            // We are going to overflow Fix16 range. Choose the end color for
            // the whole span
            //
            *pnTexturePositionIPC = GpIntToFix16(m_uTexelCountMinusOne);
            *pnXIncrement = 0;
        }
        else
        {
            *pnTexturePositionIPC = static_cast<INT>(llTexturePositionIPC);
        }
    }
    else
    {
        // Don't worry about overflowing or underflowing Fix16 range. We should
        // have this covered already by putting modulo values in the matrix.
        *pnTexturePositionIPC = (nX * m_nM11) + (nY * m_nM21) + m_nDx;
    }
}

//+-----------------------------------------------------------------------------
//
//  Class:
//      CLinearGradientBrushSpan_MMX
//
//  Synopsis:
//      This class performs texture resampling that is optimized for
//      one-dimensional gradient textures on MMX-enabled CPU's.   
//
//------------------------------------------------------------------------------
class CLinearGradientBrushSpan_MMX : public CLinearGradientBrushSpan
{
public:
    
    CLinearGradientBrushSpan_MMX();
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CLinearGradientBrushSpan_MMX));

    HRESULT Initialize(
        __in_ecount(1) const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatWorldHPCToDeviceHPC,
        __in_ecount(3) const MilPoint2F *pGradientPoints,
        __in_ecount(uCount) const MilColorF *pColors,
        __in_ecount(uCount) const FLOAT *pPositions,
        __in UINT uCount,
        __in MilGradientWrapMode::Enum wrapMode,
        __in MilColorInterpolationMode::Enum colorInterpolationMode
        );

    virtual ScanOpFunc GetScanOp() const { return ColorSource_LinearGradient_32bppPARGB_MMX; }

private:
    
    VOID GenerateColors(
        __in INT nX, 
        __in INT nY, 
        __in INT nCount, 
        __out_ecount_full(nCount) ARGB *pArgbDest
        );
    
    friend VOID FASTCALL ColorSource_LinearGradient_32bppPARGB_MMX(
        __in_ecount(1) const PipelineParams *, 
        __in_ecount(1) const ScanOpParams *
        );    
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CRadialGradientBrushSpan
//
//  Synopsis:
//      sRGB RadialGradientBrush implementation that is optimized for
//      RadialGradientBrushes with a Focus equal to it's center.
//
//------------------------------------------------------------------------------
class CRadialGradientBrushSpan : public CGradientBrushSpan
{
public:
    
    CRadialGradientBrushSpan();
    virtual ~CRadialGradientBrushSpan();
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CRadialGradientBrushSpan));

    HRESULT Initialize(
        __in_ecount(1) const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatWorldHPCToDeviceHPC,
        __in_ecount(3) const MilPoint2F *pGradientPoints,
        __in_ecount(uCount) const MilColorF *pColors,
        __in_ecount(uCount) const FLOAT *pPositions,
        __in UINT uCount,
        __in MilGradientWrapMode::Enum wrapMode,
        __in MilColorInterpolationMode::Enum colorInterpolationMode
        );

    virtual ScanOpFunc GetScanOp() const { return ColorSource_RadialGradient_32bppPARGB; }
    
    virtual VOID ReleaseExpensiveResources();

protected:

    // Matrix elements that convert from device space to
    // texture space.

    FLOAT m_rM11;
    FLOAT m_rM21;
    FLOAT m_rDx; 
    FLOAT m_rM12;
    FLOAT m_rM22;
    FLOAT m_rDy; 

private:
    
    template<typename TPlatform>
    VOID GenerateColors(
        __in INT nX, 
        __in INT nY, 
        __in INT nCount, 
        __out_ecount_full(nCount) ARGB *pArgbDest
        );
    
    friend VOID FASTCALL ColorSource_RadialGradient_32bppPARGB(
        __in_ecount(1) const PipelineParams *, 
        __in_ecount(1) const ScanOpParams *
        );    
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CFocalGradientBrushSpan
//
//  Synopsis:
//      sRGB RadialGradientBrush implementation that contains logic for
//      RadialGradientBrush's with a Focus that isn't equal to it's center
//
//------------------------------------------------------------------------------
class CFocalGradientBrushSpan : public CRadialGradientBrushSpan
{
public:
    CFocalGradientBrushSpan();
    virtual ~CFocalGradientBrushSpan();
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CFocalGradientBrushSpan));

    HRESULT Initialize(
        __in_ecount(1) const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatWorldHPCToDeviceHPC,
        __in_ecount(3) const MilPoint2F *pGradientPoints,
        __in_ecount(uCount) const MilColorF *pColors,
        __in_ecount(uCount) const FLOAT *pPositions,
        __in UINT uCount,
        __in MilGradientWrapMode::Enum wrapMode,
        __in MilColorInterpolationMode::Enum colorInterpolationMode,
        __in_ecount(1) const MilPoint2F *pFocalPoint
        );

    virtual ScanOpFunc GetScanOp() const { return ColorSource_FocalGradient_32bppPARGB; }

protected:

    // Gradient origin in non-normalized gradient circle space
    FLOAT m_rXFocalHPC;
    FLOAT m_rYFocalHPC;

    // Center of first stop region in non-normalized gradient circle space
    FLOAT m_rXFirstTexelRegionCenter;
    FLOAT m_rYFirstTexelRegionCenter;

private:

    void TransformPointFromWorldHPCToGradientCircle(
        __in_ecount(1) const CMatrix<CoordinateSpace::BaseSamplingHPC,CoordinateSpace::DeviceHPC> *pmatWorldHPCtoDeviceHPC,
        __in_ecount(1) const MilPoint2F *pptWorldHPC,
        __out_ecount(1) FLOAT *pflXUnitCircle,
        __out_ecount(1) FLOAT *pflYUnitCircle
        ) const;

    VOID GenerateColors(
        __in INT nX, 
        __in INT nY, 
        __in INT nCount, 
        __out_ecount_full(nCount) ARGB *pArgbDest
        );

    friend VOID FASTCALL ColorSource_FocalGradient_32bppPARGB(
        __in_ecount(1) const PipelineParams *, 
        __in_ecount(1) const ScanOpParams *
        );
};



class CShaderEffectBrushSpan : public CColorSource
{
public:
    CShaderEffectBrushSpan();
    ~CShaderEffectBrushSpan() override {}

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CShaderEffectBrushSpan));

    HRESULT Initialize(
        __in const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::DeviceHPC> *pRealizationSamplingToDevice,
        __inout CMILBrushShaderEffect* pShaderEffectBrush);


    ScanOpFunc GetScanOp() const override { return ColorSource_ShaderEffect_32bppPARGB; }
    MilPixelFormat::Enum GetPixelFormat() const override { return MilPixelFormat::PBGRA32bpp; }
    void ReleaseExpensiveResources() override;

private:

    VOID GenerateColors(
        __in INT nX, 
        __in INT nY, 
        __in INT nCount, 
        __out_ecount_full(nCount) ARGB *pArgbDest
        );

    friend VOID FASTCALL ColorSource_ShaderEffect_32bppPARGB(
        __in_ecount(1) const PipelineParams *, 
        __in_ecount(1) const ScanOpParams *);


    CPixelShaderState m_pixelShaderState;
    CPixelShaderCompiler *m_pPixelShaderCompiler;
    GenerateColorsEffect *m_pfnGenerateColorsEffectWeakRef;
    CMILBrushShaderEffect *m_pShaderEffectBrushNoRef;
};





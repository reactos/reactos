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
//      Texture interpolation (bilinear and others)
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CNearestNeighborSpan);
MtExtern(CBilinearSpan);
MtExtern(CBilinearSpan_MMX);
MtExtern(CIdentitySpan);

MtExtern(CNearestNeighborSpan_scRGB);
MtExtern(CBilinearSpan_scRGB);

MtExtern(CConstantAlphaSpan);
MtExtern(CMaskAlphaSpan);

MtExtern(CConstantAlphaSpan_scRGB);
MtExtern(CMaskAlphaSpan_scRGB);

/**************************************************************************
*
*   class CResampleSpan
*
*   Common class for the resampling spans.
*
**************************************************************************/

template <class TColor>
class CResampleSpan : public CColorSource
{
public:
    CResampleSpan();
    virtual ~CResampleSpan();

    virtual HRESULT Initialize(
        __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
        MilBitmapWrapMode::Enum wrapMode,
        __in_ecount_opt(1) const MilColorF *pBorderColor,
        __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatTextureHPCToDeviceHPC
        );

    virtual VOID ReleaseExpensiveResources() override;

    virtual MilPixelFormat::Enum GetPixelFormat() const override
    {
        return m_PixelFormat;
    }

protected:
    HRESULT InitializeBitmapPointer(
        __in_ecount(1) IWGXBitmapSource *pIBitmapSource
        );

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      InitializeColors
    //
    //  Synopsis:
    //      Initialize color type specific members.  This method virtual to
    //      allow the caller, Initialize, to have a common implementation for
    //      each unique color type (TColor).
    //
    //--------------------------------------------------------------------------

    virtual void InitializeColors(
        __in_ecount_opt(1) const MilColorF *pBorderColor
        );

    template <class TResampleClass, class TColor>
    friend VOID FASTCALL ColorSource_Image_ScanOp(
        __in_ecount(1) const PipelineParams *, __in_ecount(1) const ScanOpParams *);

    IWGXBitmapSource *m_pIBitmapSource;
    IWGXBitmap *m_pIBitmap;
    IWGXBitmapLock *m_pILock;

    void *m_pvBits;
    UINT m_cbStride;
    UINT m_nWidth;
    UINT m_nHeight;

    MilPixelFormat::Enum m_PixelFormat;
    MilBitmapWrapMode::Enum m_WrapMode;

    CMILMatrix m_matDeviceToTexture;

    // Note: keep template parameters at the end so TColor independent methods
    //       have a single compiled form.

    TColor m_BorderColor;
};

/**************************************************************************
*
*   class CResampleSpan_sRGB
*
*   Common class for the resampling spans in sRGB space.
*
**************************************************************************/

typedef CResampleSpan<GpCC> CResampleSpan_sRGB;

/**************************************************************************
*
*   class CNearestNeighborSpan
*
*   Resampling span using nearest pixel filtering.
*
**************************************************************************/

class CNearestNeighborSpan : public CResampleSpan_sRGB
{
public:
    CNearestNeighborSpan();
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CNearestNeighborSpan));

    virtual ScanOpFunc GetScanOp() const override;

    void GenerateColors(INT x, INT y, __range(>=,1) UINT uiCount, __out_ecount_full(uiCount) GpCC *pargbDest) const;
};

/**************************************************************************
*
*   class CBilinearSpan
*
*   Resampling span using bilinear filtering.
*
**************************************************************************/


class CBilinearSpan : public CResampleSpan_sRGB
{
public:
    CBilinearSpan();
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CBilinearSpan));

    virtual HRESULT Initialize(
        __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
        MilBitmapWrapMode::Enum wrapMode,
        __in_ecount_opt(1) const MilColorF *pBorderColor,
        __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatTextureHPCToDeviceHPC
        );

    virtual ScanOpFunc GetScanOp() const override;

    void GenerateColors(
        INT x,
        INT y,
        __range(>=,1) UINT uiCount,
        __out_ecount_full(uiCount) GpCC *pargbDest
        ) const;

private:

    void Handle_Extend_OutsideTexture_SSE2(
        INT u,
        INT v,
        UINT uiCount,
        __out_ecount_full(uiCount) ARGB *pargbDest
        ) const;

    bool IsOnBorder(__int64 u, __int64 v) const;

    __range(0, uiCount)
    UINT Handle_OutsideTexture_C(
        __inout_ecount(1) __int64 &u,
        __inout_ecount(1) __int64 &v,
        UINT uiCount,
        __out_ecount_part(uiCount,uiCount-return) ARGB *pargbDest
        ) const;

    __range(0, uiCount)
    inline UINT Handle_OutsideTexture_C(
        __inout_ecount(1) INT &u,
        __inout_ecount(1) INT &v,
        UINT uiCount,
        __out_ecount_part(uiCount,uiCount-return) ARGB *pargbDest
        ) const;

    void FlippedTile_Interpolation_C(
        INT u,
        INT v,
        UINT uiCount,
        __out_ecount_full(uiCount) ARGB *pargbDest
        ) const;

#if defined(_X86_)
    // Future Consideration:  SSE2 on 64-bit platforms.  SSE2 intrinsics are not
    // linking for 64bit.
    void FlippedTile_Interpolation_SSE2(
        INT u,
        INT v,
        UINT uiCount,
        __out_ecount_full(uiCount) ARGB *pargbDest
        ) const;

    void InTile_Interpolation_SSE2(
        INT x,
        INT y,
        UINT uiCount,
        __out_ecount_full(uiCount) ARGB *pargbDest
        ) const;

    void InTile_Interpolation_SSE2_Nonrotated(
        INT u,
        INT v,
        UINT uiCount,
        __out_ecount_full(uiCount) ARGB *pargbDest
        ) const;
#endif

    void DeterminePixels_OutBoundary(INT u,INT v,INT UIncrement,INT VIncrement,UINT uiCount,INT *N1,INT *N2);

    VOID InitializeFixedPointState();
    VOID SetDeviceOffset();

    INT M11;                   // 16.16 fixed point representation of the
    INT M12;                   //   device-to-world transform
    INT M21;
    INT M22;
    INT Dx;
    INT Dy;

    INT UIncrement;         // Increment in texture space for every one-
    INT VIncrement;         //   pixel-to-the-right in device space

    INT ModulusWidth;       // Modulus value for doing tiling
    INT ModulusHeight;

    INT XEdgeIncrement;     // Edge condition increments.
    INT YEdgeIncrement;

    INT XDeviceOffset;      // "origin" of path, in device space
    INT YDeviceOffset;

private:
    void WrapPositionAndFlipState(
        __inout_ecount(1) INT &u,
        __inout_ecount(1) INT &v) const;

    // This state is for optimization purposes only:  to avoid recalculating these parameters continuously.
    __int64 inTileUMax;
    __int64 inTileVMax;
    __int64 flipTileUMin;
    __int64 flipTileVMin;
    __int64 inflipTileUMax;
    __int64 inflipTileVMax;

    INT canonicalWidth;
    INT canonicalHeight;

    bool IsLargeTexture() const;
    bool IsLargeSpan(__int64 u, __int64 v, UINT uiCount) const;
};


/**************************************************************************
*
*   class CUnoptimizedBilinearSpan
*
*   Historical non-optimized BilinearSpan implementation.
*
**************************************************************************/

// Future Consideration: 
// Remove this class once Intel-optimized bilinear span is online
//
// This class is being renamed & checked in side-by-side with the
// Intel-optimized implementation until the
// Intel-optimized version (CBilinearSpan) has been fully tested
// We hope to reduce integration costs by checking in the disabled
// Intel-optimized implementation in the interim.
//
// We can remove this implementation once the Intel-optimized implementation
// has been updated to handle the full integer range and has been fully tested.
class CUnoptimizedBilinearSpan : public CResampleSpan_sRGB
{
public:
    CUnoptimizedBilinearSpan();
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CBilinearSpan));

    virtual ScanOpFunc GetScanOp() const override;

    void GenerateColors(INT x, INT y, __range(>=,1) UINT uiCount, __out_ecount_full(uiCount) GpCC *pargbDest) const;
};

/**************************************************************************
*
*   class CBilinearSpan_MMX
*
*   Resampling span using bilinear filtering. Code optimized using
*   MMX instruction set.
*
**************************************************************************/

class CBilinearSpan_MMX : public CUnoptimizedBilinearSpan
{
public:
    CBilinearSpan_MMX();
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CBilinearSpan_MMX));

    virtual HRESULT Initialize(
        __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
        MilBitmapWrapMode::Enum wrapMode,
        __in_ecount_opt(1) const MilColorF *pBorderColor,
        __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatTextureHPCToDeviceHPC
        ) override;

    virtual ScanOpFunc GetScanOp() const override;

    static BOOL CanHandleInputRange(
        UINT uBitmapWidth,
        UINT uBitmapHeight,
        MilBitmapWrapMode::Enum wrapMode);

    void GenerateColors(INT x, INT y, __range(>=,1) UINT uiCount, __out_ecount_full(uiCount) GpCC *pargbDest) const;

private:
    VOID InitializeFixedPointState();
    VOID SetDeviceOffset();

    INT M11;                   // 16.16 fixed point representation of the
    INT M12;                   //   device-to-world transform
    INT M21;
    INT M22;
    INT Dx;
    INT Dy;

    INT UIncrement;         // Increment in texture space for every one-
    INT VIncrement;         //   pixel-to-the-right in device space

    INT ModulusWidth;       // Modulus value for doing tiling
    INT ModulusHeight;

    INT XEdgeIncrement;     // Edge condition increments.
    INT YEdgeIncrement;

    INT XDeviceOffset;      // "origin" of path, in device space
    INT YDeviceOffset;

};

/**************************************************************************
*
*   class CIdentitySpan
*
*   Identity resampling span. Used when there is no complicated affine
*   operation on the input bitmap just integer translation from one
*   location to another.
*
**************************************************************************/

class CIdentitySpan : public CResampleSpan_sRGB
{
public:
    CIdentitySpan();
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CIdentitySpan));

    virtual HRESULT Initialize(
        __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
        MilBitmapWrapMode::Enum wrapMode,
        __in_ecount_opt(1) const MilColorF *pBorderColor,
        __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatTextureHPCToDeviceHPC
        ) override;

    virtual ScanOpFunc GetScanOp() const override;

    VOID GenerateColors(INT x, INT y, __range(>=,1) UINT uiCount, __out_ecount_full(uiCount) GpCC *pargbDest) const;

protected:
    INT Dx;
    INT Dy;

    // True if both texture dimensions power of two
    BOOL PowerOfTwo;
};

/**************************************************************************
*
*   class CResampleSpan_scRGB
*
*   Common class for the resampling spans in scRGB space.
*
**************************************************************************/

typedef CResampleSpan<MilColorF> CResampleSpan_scRGB;

/**************************************************************************
*
*   class CNearestNeighborSpan_scRGB
*
*   Resampling span using nearest pixel filtering in scRGB space.
*
**************************************************************************/

class CNearestNeighborSpan_scRGB : public CResampleSpan_scRGB
{
public:
    CNearestNeighborSpan_scRGB();
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CNearestNeighborSpan_scRGB));

    virtual ScanOpFunc GetScanOp() const override;

    void GenerateColors(INT x, INT y, __range(>=,1) UINT uiCount, __out_ecount_full(uiCount) MilColorF *pcolDest) const;
};

/**************************************************************************
*
*   class CBilinearSpan_scRGB
*
*   Resampling span using bilinear filtering in scRGB space.
*
**************************************************************************/

class CBilinearSpan_scRGB : public CResampleSpan_scRGB
{
public:
    CBilinearSpan_scRGB();
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CBilinearSpan_scRGB));

    virtual ScanOpFunc GetScanOp() const override;

    void GenerateColors(INT x, INT y, __range(>=,1) UINT uiCount, __out_ecount_full(uiCount) MilColorF *pcolDest) const;
};

/**************************************************************************
*
*   class CConstantAlphaSpan
*
*   Span class applying constant alpha on its input.
*
**************************************************************************/

class CConstantAlphaSpan : public COwnedOSD
{
public:
    CConstantAlphaSpan();
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CConstantAlphaSpan));

    HRESULT Initialize(FLOAT flAlpha);

    friend VOID FASTCALL ConstantAlpha_32bppPARGB(
        const PipelineParams *, const ScanOpParams *);

    friend VOID FASTCALL ConstantAlpha_32bppRGB(
        const PipelineParams *, const ScanOpParams *);

private:
    // Don't call, this is the implementation of above functions
    friend static VOID MIL_FORCEINLINE ConstantAlpha_32bppPARGB_or_32bppRGB_Slow(
        const PipelineParams *, const ScanOpParams *, bool);

    INT m_nAlpha;
};

/**************************************************************************
*
*   class CMaskAlphaSpan
*
*   Span class applying alpha mask on its input.
*
**************************************************************************/

class CMaskAlphaSpan : public COwnedOSD
{
public:
    CMaskAlphaSpan();
    virtual ~CMaskAlphaSpan();

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMaskAlphaSpan));

    HRESULT Initialize(
        __in_ecount(1) IWGXBitmapSource *pIMask,
        __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatMaskToDevice,
        MilBitmapInterpolationMode::Enum interpolationMode,
        bool prefilterEnable,
        FLOAT prefilterThreshold,
        INT spanWidth
        );

    friend VOID FASTCALL MaskAlpha_32bppPARGB_32bppPARGB(
        const PipelineParams *, const ScanOpParams *);

    friend VOID FASTCALL MaskAlpha_32bppRGB_32bppPARGB(
        const PipelineParams *, const ScanOpParams *);

private:
    // Implementation for more specific functions
    friend static VOID MIL_FORCEINLINE MaskAlpha_32bpp_Slow_32bppPARGB(
        const PipelineParams *,
        const ScanOpParams *,
        bool fHasAlpha
        );

    BYTE *m_pBuffer;
    UINT m_nBufferLen;
    CColorSource *m_pMaskResampleCS;

    CColorSourceCreator_sRGB m_Creator_sRGB;
};

/**************************************************************************
*
*   class CConstantAlphaSpan_scRGB
*
*   Span class applying constant alpha on its input.
*
**************************************************************************/

class CConstantAlphaSpan_scRGB : public COwnedOSD
{
public:
    CConstantAlphaSpan_scRGB();
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CConstantAlphaSpan_scRGB));

    HRESULT Initialize(FLOAT flAlpha);

    friend VOID FASTCALL ConstantAlpha_128bppPABGR(
        const PipelineParams *, const ScanOpParams *);

private:
    FLOAT m_flAlpha;
};

/**************************************************************************
*
*   class CMaskAlphaSpan_scRGB
*
*   Span class applying alpha mask on its input.
*
**************************************************************************/

class CMaskAlphaSpan_scRGB : public COwnedOSD
{
public:
    CMaskAlphaSpan_scRGB();
    virtual ~CMaskAlphaSpan_scRGB();

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMaskAlphaSpan_scRGB));

    HRESULT Initialize(
        __in_ecount(1) IWGXBitmapSource *pIMask,
        __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatMaskToDevice,
        MilBitmapInterpolationMode::Enum interpolationMode,
        bool prefilterEnable,
        FLOAT prefilterThreshold,
        INT spanWidth
        );

    friend VOID FASTCALL MaskAlpha_128bppPABGR_128bppPABGR(
        const PipelineParams *, const ScanOpParams *);

private:
    FLOAT *m_pBuffer;
    UINT m_nBufferLen;
    CColorSource *m_pMaskResampleCS;

    CColorSourceCreator_scRGB m_Creator_scRGB;
};





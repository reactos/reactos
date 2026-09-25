// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//
//      Declaration of MIL API brush and related objects including:
//
//          CMILBrush
//          CMILBrushSolid
//          CMILBrushLinearGradient
//          CMILBrushRadialGradient
//          CMILBrushBitmap
//          CMILBrushVideo
//

#pragma once

class CMilEffectDuce;

/*=========================================================================*\
    CMILBrushSolid - MIL Solid Brush
\*=========================================================================*/

MtExtern(CMILBrushSolid);

class CMILBrushSolid :
    public CMILObject,
    public CMILBrush
{
public:

    // Creation methods

    static HRESULT Create(
        __deref_out_ecount(1) CMILBrushSolid **ppSolidBrush)

    {
        RRETURN(Create(NULL, NULL, ppSolidBrush));
    }

    static HRESULT Create(
        __in_ecount_opt(1) CMILFactory *pFactory,
        __in_ecount_opt(1) const MilColorF *pColor,
        __deref_out_ecount(1) CMILBrushSolid **ppSolidBrush);


protected:

    CMILBrushSolid(__in_ecount_opt(1) CMILFactory *pFactory = NULL);
    virtual ~CMILBrushSolid();

private:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMILBrushSolid));

public:

    DECLARE_MIL_OBJECT

    // CMILBrush methods

    __override BrushTypes GetType() const
    {
        return BrushSolid;
    }

    BOOL ObviouslyHasZeroAlpha() const override
    {
        return HasZeroAlpha();
    }

    BOOL HasZeroAlpha() const;

    void GetColor(__out_ecount(1) MilColorF *pColor) const;
    void SetColor(__in_ecount(1) const MilColorF *pColor);

public:

    MilColorF m_SolidColor;

};


/*=========================================================================*\
    CMILBrushLinearGradient - MIL Linear Gradient Brush
\*=========================================================================*/

MtExtern(CMILBrushLinearGradient);

class CMILBrushLinearGradient :
    public CMILBrushGradient
{
public:

    // Creation methods

    static HRESULT Create(
        CMILBrushLinearGradient **ppLinearGradientBrush)
    {
        RRETURN(Create(NULL, ppLinearGradientBrush));
    }

    static HRESULT Create(
        CMILFactory *pFactory,
        CMILBrushLinearGradient **ppLinearGradientBrush);

protected:
    CMILBrushLinearGradient(__in_ecount_opt(1) CMILFactory *pFactory = NULL);
    virtual ~CMILBrushLinearGradient();

private:
    // Create should be used to instantiate this object, not operator new.
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMILBrushLinearGradient));

public:

    DECLARE_MIL_OBJECT

    // CMILBrush Methods

    BrushTypes GetType() const
    {
        return BrushGradientLinear;
    }


    void SetLinePoints(
        __in_ecount(1) const MilPoint2F *pptBeginPoint,
        __in_ecount(1) const MilPoint2F *pptEndPoint
        );

    // CMILGradientBrush members

    BOOL IsRadial() { return FALSE; }
};


/*=========================================================================*\
    CMILBrushRadialGradient - MIL Radial Gradient Brush
\*=========================================================================*/

MtExtern(CMILBrushRadialGradient);

class CMILBrushRadialGradient :
    public CMILBrushGradient
{
public:

    // Creation methods

    static HRESULT Create(
        CMILBrushRadialGradient **ppRadialGradientBrush)
    {
        RRETURN(Create(NULL, ppRadialGradientBrush));
    }

    static HRESULT Create(
        CMILFactory *pFactory,
        CMILBrushRadialGradient **ppRadialGradientBrush);

protected:

    CMILBrushRadialGradient(__in_ecount_opt(1) CMILFactory *pFactory = NULL);
    virtual ~CMILBrushRadialGradient();

private:
    // Create should be used to instantiate this object, not operator new.
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMILBrushRadialGradient));

public:

    DECLARE_MIL_OBJECT

    // CMILBrush Methods

    BrushTypes GetType() const
    {
        return BrushGradientRadial;
    }

    void SetGradientOrigin(
        BOOL fHasSeperateOriginFromCenter,
        __in_ecount(1) const MilPoint2F *pptGradientOrigin
        );
  
    // CMILGradientBrush members

    BOOL IsRadial() { return TRUE; }


    // internal methods

    BOOL HasSeparateOriginFromCenter() const { return m_fHasSeparateOriginFromCenter; }

    const MilPoint2F &GetGradientOrigin() const;

    const MilPoint2F &GetGradientCenter() const { return m_ptStartPointOrCenter; }

private:
    BOOL m_fHasSeparateOriginFromCenter;

    MilPoint2F m_ptGradientOrigin;
};


/*=========================================================================*\
    CMILBrushBitmap - MIL Bitmap Brush
\*=========================================================================*/

class CMILBrushBitmapLocalSetterWrapper;  // Defined later, needed for friend declaration

MtExtern(CMILBrushBitmap);

enum XSpaceDefinition
{
    XSpaceIsSampleSpace,
    XSpaceIsWorldSpace,
    XSpaceIsIrrelevant
};

struct BitmapToXSpaceTransform
{
    CMultiOutSpaceMatrix<CoordinateSpace::RealizationSampling> matBitmapSpaceToXSpace;

#if DBG
    void DbgSetXSpace(CoordinateSpaceId::Enum eCoordSpace)
    {
        Assert(   (eCoordSpace == CoordinateSpaceId::Device)
               || (eCoordSpace == CoordinateSpaceId::BaseSampling));

        dbgXSpaceDefinition = (eCoordSpace == CoordinateSpaceId::BaseSampling)
            ? XSpaceIsWorldSpace
            : XSpaceIsSampleSpace;
    }

    XSpaceDefinition dbgXSpaceDefinition;
#endif
};

class CMILBrushBitmap :
    public CMILObject,
    public CMILBrushWithCache,
    public CObjectUniqueness
{
public:

    // Creation methods

    static HRESULT Create(
        CMILBrushBitmap **ppBrushBitmap)
    {
        RRETURN(Create(NULL, NULL, ppBrushBitmap));
    }


    static HRESULT Create(
        CMILFactory *pFactory,
        IWGXBitmapSource *piBitmap,
        CMILBrushBitmap **ppBrushBitmap
        );

protected:

    CMILBrushBitmap(OPTIONAL CMILFactory *pFactory = NULL);
    virtual ~CMILBrushBitmap();

private:

    // Create should be used to instantiate this object, not operator new.
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMILBrushBitmap));

public:

    DECLARE_MIL_OBJECT

    // IMILResourceCache
    STDMETHOD_(void, GetUniquenessToken)(
        __out_ecount(1) UINT *puToken
        ) const override;

    // CMILBrush Methods

    BrushTypes GetType() const override
    {
        return BrushBitmap;
    }

    bool MayNeedNonPow2Tiling() const override;

    BOOL ObviouslyHasZeroAlpha() const override
    {
        return GetOpacity() == 0.0f;
    }

    // IMILBrushBitmap Helper Methods

    HRESULT SetBitmap(
        IN IWGXBitmapSource *pBitmapSource
        );

    HRESULT GetBitmap(
        IN IWGXBitmapSource **ppBitmapSource
        ) const;

    HRESULT SetWrapMode(
        IN MilBitmapWrapMode::Enum wrapMode,
        IN const MilColorF *pBorderColor
        );

    HRESULT GetWrapMode(
        OUT MilBitmapWrapMode::Enum *pWrapMode,
        OUT MilColorF *pBorderColor
        ) const; 

    void SetBitmapToXSpaceTransform(
        __in_ecount(1) const CBaseMatrix *pmatBitmapToXSpace,
        XSpaceDefinition xSpaceDefinition
        DBG_COMMA_PARAM(__in_ecount_opt(1) const CMILMatrix *pmatDBGWorldToSampleSpace)
        );

    void GetBitmapToSampleSpaceTransform(
        __in_ecount(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> &matWorldToSampleSpace,
        __out_ecount(1) CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> &matBitmapToSampleSpace
        );

    void GetBitmapToWorldSpaceTransform(
        __out_ecount(1) CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::BaseSampling> &matBitmapToWorldSpace
        ) const;



    HRESULT SetSourceClipXSpace(
        BOOL fUseSourceClip,
        BOOL fSourceClipIsEntireSource,
        __in_ecount(1) const CParallelogram *pSourceClipSampleSpace
        DBG_COMMA_PARAM(XSpaceDefinition xSpaceDefinition)
        DBG_COMMA_PARAM(__in_ecount_opt(1) const CMILMatrix *pmatDBGWorldToSampleSpace)
        );

    BOOL HasSourceClip() const
    {
        // Returns whether or not this brush should be clipped to 
        // a source parallelogram
        return m_fUseSourceClip;
    }

    BOOL SourceClipIsEntireSource() const
    {
        Assert(m_fUseSourceClip);
        return m_fSourceClipIsEntireSource;
    }

    const CParallelogram *GetSourceClipWorldSpace() const
    {
        Assert(m_xSamplingSpaceDefinition == XSpaceIsWorldSpace);
        return m_fUseSourceClip
            ? &m_sourceClipInSamplingSpace
            : NULL;
    }

    void GetSourceClipSampleSpace(
        __in_ecount_opt(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> *pmatWorldToSampleSpace,
        __out_ecount(1) CParallelogram *pSourceClipSampleSpace
        ) const;

    // Provides access to m_pTexture -- Any changes to the texture will not be accounted for in uniqueness.
    IWGXBitmapSource* GetTextureNoAddRef() const
    {
        return m_pTexture;
    }

    // Assigns a new value to m_pTexture.
    void SetTextureNoAddRef(__in_ecount(1) IWGXBitmapSource *pTexture)
    {
        UpdateUniqueCount();
        m_pTexture = pTexture;
    }

    // Returns the value of m_WrapMode.
    MilBitmapWrapMode::Enum GetWrapMode() const
    {
        return m_WrapMode;
    }

    // Returns a const reference to m_BorderColor.
    __outro_ecount(1) const MilColorF &GetBorderColorRef() const
    {
        return m_BorderColor;
    }

    void SetOpacity(FLOAT opacity)
    {
        m_opacity = opacity;
    }

    FLOAT GetOpacity() const
    {
        return m_opacity;
    }

private:
#if DBG
    void DBGAssertWorldToSampleSpaceHasNotChanged(
        __in_ecount_opt(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> *pmatWorldToSampleSpace
        ) const;
#endif
    
private:
    //
    // This enum contains the definition of "SamplingSpace".  This definition
    // affects the meaning of m_matBitmapToSamplingSpace and
    // m_sourceClipInSamplingSpace
    //
    XSpaceDefinition m_xSamplingSpaceDefinition;

    // transform from bitmap space to either WorldSpace or SampleSpace
    CMultiOutSpaceMatrix<CoordinateSpace::RealizationSampling> m_matBitmapToSamplingSpace;

    //
    // Source clipping is used to implement TileMode.None brushes. These
    // brushes should not render outside the viewport, which means that their
    // source space is a finite area. The viewport imposes a rectangular clip
    // in viewport space, but in world or sample space it can be a
    // parallelogram.
    //

    // source clip parallelogram in either WorldSpace or SampleSpace
    CParallelogram m_sourceClipInSamplingSpace;

    // If m_fUseSourceClip is true, then the source clip is valid and should be
    // used as a clip.
    //
    BOOL m_fUseSourceClip;
    BOOL m_fSourceClipIsEntireSource;

    IWGXBitmapSource *m_pTexture;
    MilBitmapWrapMode::Enum m_WrapMode;
    MilColorF m_BorderColor;

    FLOAT m_opacity;

    friend class CMILBrushBitmapLocalSetterWrapper;   // Needs access to the private member variables

#if DBG
private:
    CMILMatrix m_matDBGWorldToSampleSpaceWhenSetBitmapToSampleSpace;
#endif
};

//
// CMILBrushBitmapLocalSetterWrapper
// Wrapper for doing temporary local changes to a CMILBrushBitmap.
//
class CMILBrushBitmapLocalSetterWrapper
{
public:

    CMILBrushBitmapLocalSetterWrapper(
        __in_ecount(1) CMILBrushBitmap *pBrushBitmap,
        __in_ecount(1) IWGXBitmapSource *pTexture,
        MilBitmapWrapMode::Enum wrapMode,
        __in_ecount(1) const CBaseMatrix *pmatBitmapToXSpace,
        XSpaceDefinition xSpaceDefinition
        DBG_COMMA_PARAM(__in_ecount_opt(1) const CMultiOutSpaceMatrix<CoordinateSpace::BaseSampling> *pmatDBGWorldToSampleSpace)
        )
    {
        Initialize(pBrushBitmap, 
                   pTexture, 
                   wrapMode, 
                   pmatBitmapToXSpace,
                   xSpaceDefinition
                   DBG_COMMA_PARAM(pmatDBGWorldToSampleSpace));
    }

    ~CMILBrushBitmapLocalSetterWrapper();

private:

    void Initialize(
        __in_ecount(1) CMILBrushBitmap *pBrushBitmap,
        __in_ecount(1) IWGXBitmapSource *pTexture,
        MilBitmapWrapMode::Enum wrapMode,
        __in_ecount(1) const CBaseMatrix *pmatBitmapToXSpace,
        XSpaceDefinition xSpaceDefinition
        DBG_COMMA_PARAM(__in_ecount_opt(1) const CMultiOutSpaceMatrix<CoordinateSpace::BaseSampling> *pmatDBGWorldToSampleSpace)
        );

    CMILBrushBitmap *m_pBrushBitmap;
};

/*=========================================================================*\
    CMILShaderBrush - Shader Brush
\*=========================================================================*/

MtExtern(CMILBrushShaderEffect);

class CMILBrushShaderEffect :
    public CMILObject,
    public CMILBrush
{
public:

    // Creation methods

    static HRESULT Create(
        __in CMilEffectDuce *pShaderEffect,
        __deref_out CMILBrushShaderEffect **ppShaderEffectBrush);


protected:

    CMILBrushShaderEffect(__in CMilEffectDuce *pShaderEffect);
    ~CMILBrushShaderEffect() override;

private:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMILBrushShaderEffect));

public:

    DECLARE_MIL_OBJECT   

    HRESULT ConfigurePass(
        __in const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::BaseSampling> &matBitmapToBaseSamplingSpace);
    
    void GetBitmapToSampleSpaceTransform(
        __in const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> &matBaseSamplingToSampleSpace, // Composition: local to device.
        __out CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pMatBitmapToBaseSamplingSpace); // Composition: texture to device
    

    HRESULT PreparePass(
        __in const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::DeviceHPC> *pRealizationSamplingToDevice,
        __inout CPixelShaderState *pPixelShaderState, 
        __deref_out CPixelShaderCompiler **ppPixelShaderCompiler);

    // CMILBrush methods

    BrushTypes GetType() const override
    {
        return BrushShaderEffect;
    }  

    BOOL ObviouslyHasZeroAlpha() const override
    {
        return false;
    }

private:

    CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::BaseSampling> m_matBitmapToBaseSamplingSpace;   
    CMilEffectDuce *m_pShaderEffectWeakRef;
};




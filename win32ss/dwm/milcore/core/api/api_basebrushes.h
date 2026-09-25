// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains declaration for CMILBrushGradient class
//

#pragma once


enum BrushTypes
{
    BrushSolid = 1,
    BrushGradientLinear,
    BrushGradientRadial,
    BrushBitmap,
    BrushShaderEffect,
    BRUSHTYPES_FORCE_DWORD = MIL_FORCE_DWORD
};

class CMILBrush : public IUnknown
{
public:
    
    virtual BrushTypes GetType() const = 0;

    virtual bool MayNeedNonPow2Tiling() const
    {
        return false;
    }

    virtual BOOL ObviouslyHasZeroAlpha() const
    {
        return false;
    }
};


//+----------------------------------------------------------------------------
//
//  Class:
//      CMILBrushWithCache
//
//  Synopsis:
//      Base class for brushes which have caching support
//
//-----------------------------------------------------------------------------

class CMILBrushWithCache :
    public CMILBrush,
    public CMILResourceCache
{
};

/*=========================================================================*\
    CGradientColorData - merges some color data management for the
                         gradient brushes

\*=========================================================================*/

class CGradientColorData
{
public:
    HRESULT SetColors(
        __in_ecount(nCount) MilColorF const *pColors,
        UINT nCount
        );

    HRESULT AddColorWithPosition(
        __in_ecount(1) MilColorF const *pColor,
        FLOAT rPosition
        );

    HRESULT CopyFrom(
        __in_ecount(1) CGradientColorData const *pColorData
        );    

    HRESULT ApplyOpacity(FLOAT rOpacity);
    
    UINT GetCount() const
    {
        return m_rgColors.GetCount();
    }

    __out_ecount(1) MilColorF *GetColorsPtr() const
    {
        return m_rgColors.GetDataBuffer();
    }

    __out_ecount(1) FLOAT *GetPositionsPtr() const
    {
        return m_rgPositions.GetDataBuffer();
    }
    
    VOID Clear()
    {
        m_rgColors.Reset(FALSE);
        m_rgPositions.Reset(FALSE);
    }

private:
    DynArray<MilColorF> m_rgColors;
    DynArray<FLOAT> m_rgPositions;
};

/*=========================================================================*\
    CMILBrushGradient - MIL Base Gradient Brush
\*=========================================================================*/

class CMILBrushGradient :
    public CMILObject,
    public CMILBrushWithCache,
    public CObjectUniqueness
{
protected:

    CMILBrushGradient(
        __in_ecount_opt(1) CMILFactory *pFactory
        );

    virtual ~CMILBrushGradient();

public:

#if DBG
    //+------------------------------------------------------------------------
    //
    //  Member:    
    //      CMILBrushGradient::MayNeedNonPow2Tiling
    //
    //  Synopsis:  
    //      Returns whether the brush needs non-pow2 tiling. Now-pow2 tiling is
    //      not implemented in hardware text rendering, so text uses this query
    //      to determine if software should be used instead.
    //
    //  Notes:
    //      This DBG method is entirely unecessary. It exists so that I could
    //      comment why it doesn't need to exist.
    //
    //-------------------------------------------------------------------------
    override bool MayNeedNonPow2Tiling() const
    {
        //
        // Gradients are always realized to Pow2 textures in hardware, so they
        // never need non-pow2 tiling.
        //

        bool fReturn = CMILBrush::MayNeedNonPow2Tiling();
        Assert(fReturn == false);

        return fReturn;
    }
#endif

    HRESULT SetColorInterpolationMode(
        MilColorInterpolationMode::Enum colorInterpolationMode
        );

    MilColorInterpolationMode::Enum GetColorInterpolationMode() const 
        { return m_ColorInterpolationMode; }

    HRESULT SetWrapMode(
        MilGradientWrapMode::Enum wrapMode
        );

    STDMETHOD_(void, GetUniquenessToken)(
        __out_ecount(1) UINT *puToken
        ) const override;

    void SetEndPoints(
        __in_ecount(1) MilPoint2F const *pptStartPointOrCenter,
        __in_ecount(1) MilPoint2F const *pptEndPoint,
        __in_ecount(1) MilPoint2F const *pptDirPointOrEndPoint2
        );
    
    HRESULT AddColorWithPosition(
        __in_ecount(1) MilColorF const *pColor,
        FLOAT rPosition
        );

    HRESULT SetColors(
        __in_ecount(nCount) MilColorF const *pColors,
        UINT nCount
        );

    // internal methods

    void GetEndPoints(
        __out_ecount(1) MilPoint2F *pptCenter,
        __out_ecount(1) MilPoint2F *pptEndPoint,
        __out_ecount(1) MilPoint2F *pptDirPointOrEndPoint2
        ) const;

    MilGradientWrapMode::Enum GetWrapMode() const
        { return m_WrapMode; }

    __out_ecount(1) CGradientColorData *GetColorData()
        { return &m_ColorData; }
    
    virtual BOOL IsRadial() PURE;

protected:

    MilPoint2F m_ptStartPointOrCenter;
    MilPoint2F m_ptEndPoint;
    MilPoint2F m_ptDirPointOrEndPoint2;

    CGradientColorData m_ColorData;

    MilGradientWrapMode::Enum m_WrapMode;
    MilColorInterpolationMode::Enum m_ColorInterpolationMode;
};






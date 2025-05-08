// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_d3d
//      $Keywords:
//
//  $Description:
//      Contains declaration for Constant type HW Color Sources
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CHwConstantMilColorFColorSource);
MtExtern(CHwConstantAlphaScalableColorSource);

class CHwSolidColorTextureSource;

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwConstantColorSource
//
//  Synopsis:
//      Base class for CHwColorSources that produce a constant color over the
//      entire area of a primitive
//
//------------------------------------------------------------------------------

class CHwConstantColorSource : public CHwColorSource
{
protected:

    CHwConstantColorSource(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice
        );

    ~CHwConstantColorSource();

public:

    // CHwColorSource methods

    override TypeFlags GetSourceType(
        ) const;

    override bool IsOpaque(
        ) const;

    override HRESULT SendVertexMapping(
        __inout_ecount_opt(1) CHwVertexBuffer::Builder *pVertexBuilder,
        MilVertexFormatAttribute mvfaLocation
        );

    HRESULT Realize(
        ); 

    override HRESULT SendDeviceStates(
        DWORD dwStage,
        DWORD dwSampler
        );

    override void ResetForPipelineReuse()
    {
        m_hShaderColorHandle = MILSP_INVALID_HANDLE;
    }

    override HRESULT SendShaderData(
        __inout_ecount(1) CHwPipelineShader *pHwShader
        );

    void SetColorShaderHandle(
        MILSPHandle hShaderColorHandle
        )
    {
        Assert(m_hShaderColorHandle == MILSP_INVALID_HANDLE);

        m_hShaderColorHandle = hShaderColorHandle;
    }

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      GetColor
    //
    //  Synopsis:
    //      This method returns the constant scRGB color of this source
    //
    //--------------------------------------------------------------------------

    virtual void GetColor(
        __out_ecount(1) MilColorF &color
        ) const PURE;

    MILSPHandle GetShaderParameterHandle() const
    {
        return m_hShaderColorHandle;
    }

private:

    CD3DDeviceLevel1 * const m_pDevice;

    CHwSolidColorTextureSource *m_pHwTexturedColorSource;   // Textured version of
                                                            // color source

    MILSPHandle m_hShaderColorHandle;
};


//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwConstantMilColorFColorSource
//
//  Synopsis:
//      This class represents a constant scRGB color source to HW
//
//------------------------------------------------------------------------------

class CHwConstantMilColorFColorSource : public CHwConstantColorSource
{
public:

    static __checkReturn HRESULT Create(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        __in_ecount(1) const MilColorF &color,
        __deref_out_ecount(1) CHwConstantMilColorFColorSource ** const ppHwColorSource
        );

    void GetColor(
        __out_ecount(1) MilColorF &color
        ) const;

protected:

    CHwConstantMilColorFColorSource(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice
        ) : CHwConstantColorSource(pDevice)
    {}

private:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwConstantMilColorFColorSource));

    CHwConstantMilColorFColorSource(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        __in_ecount(1) const MilColorF &color
        );

protected:

    MilColorF m_color;

};


//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwConstantAlphaColorSource
//
//  Synopsis:
//      This class represents a constant white scRGB color source with arbitrary
//      transparency to HW
//
//------------------------------------------------------------------------------

class CHwConstantAlphaColorSource : public CHwConstantColorSource
{
public:

    virtual void GetColor(
        __out_ecount(1) MilColorF &color
        ) const; 

    float GetAlpha() const
    {
        return m_alpha;
    }

    void SetShaderAlphaHandle(
        MILSPHandle hFloatHandle
        )
    {
        Assert(m_hShaderFloat == MILSP_INVALID_HANDLE);

        m_hShaderFloat = hFloatHandle;
    }

    override void ResetForPipelineReuse()
    {
        m_hShaderFloat = MILSP_INVALID_HANDLE;
    }

    override HRESULT SendShaderData(
        __inout_ecount(1) CHwPipelineShader *pHwShader
        );

protected:

    CHwConstantAlphaColorSource(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        FLOAT alpha
        );

    virtual ~CHwConstantAlphaColorSource();

protected:

    FLOAT m_alpha;
    MILSPHandle m_hShaderFloat;

};


//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwConstantAlphaScalableColorSource
//
//  Synopsis:
//      This constant HW Color Source is just like a
//      CHwConstantAlphaColorSource, but with the added ability to take another
//      constant color source as input and apply it alpha scale to that color
//      when a color is requested
//
//      If no original source color is provided it is assumed to be opaque
//      white.
//
//------------------------------------------------------------------------------

class CHwConstantAlphaScalableColorSource : public CHwConstantAlphaColorSource
{
public:

    DECLARE_BUFFERDISPENSER_NEW(CHwConstantAlphaScalableColorSource,
                                Mt(CHwConstantAlphaScalableColorSource))
    DECLARE_BUFFERDISPENSER_DELETE

    static __checkReturn HRESULT Create(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        FLOAT alpha,
        __in_ecount_opt(1) CHwConstantColorSource *pHwColorSource,
        __in_ecount(1) CBufferDispenser *pBufferDispenser,
        __deref_out_ecount(1) CHwConstantAlphaScalableColorSource ** const ppHwColorSource
        );

    void GetColor(
        __out_ecount(1) MilColorF &color
        ) const; 

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      IsAlphaScalable
    //
    //  Synopsis:
    //      Returns true as that is the point of this class.
    //
    //--------------------------------------------------------------------------

    override bool IsAlphaScalable() const { return true; }

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      AlphaScale
    //
    //  Synopsis:
    //      Scale (multiply) the current alpha value by the given scale
    //
    //--------------------------------------------------------------------------

    override void AlphaScale(
        FLOAT alphaScale
        );

    override HRESULT SendShaderData(
        __inout_ecount(1) CHwPipelineShader *pHwShader
        );

private:

    CHwConstantAlphaScalableColorSource(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        FLOAT alpha,
        __in_ecount_opt(1) CHwConstantColorSource *pHwColorSource
        );

    virtual ~CHwConstantAlphaScalableColorSource();

private:

    CHwConstantColorSource *m_pHwColorSource;   // Original color source or
                                                // NULL (opaque white)

};



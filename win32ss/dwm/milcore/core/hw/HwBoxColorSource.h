// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CHwBoxColorSource declaration
//

MtExtern(CHwBoxColorSource);

//+----------------------------------------------------------------------------
//
//  Class:     CHwBoxColorSource
//
//  Synopsis:  Provides a texture color source with an opaque box in the center
//             surrounded by transparent.
//
//             0000
//             0110
//             0110
//             0000
//
//
//-----------------------------------------------------------------------------

class CHwBoxColorSource : public CHwTexturedColorSource
{
public:
    static __checkReturn HRESULT Create(
        __in_ecount(1) CD3DDeviceLevel1 *pD3DDevice,
        __deref_out_ecount(1) CHwBoxColorSource ** const ppTextureSource
        );

private:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwBoxColorSource));

    CHwBoxColorSource(
        __in_ecount(1) CD3DDeviceLevel1 *pD3DDevice
        );

    ~CHwBoxColorSource();

public:
    //
    // CHwColorSource methods
    //

    override bool IsOpaque(
        ) const
    {
        return false;
    }

    //+------------------------------------------------------------------------
    //
    //  Member:
    //      IsAlphaScalable
    //
    //  Synopsis:
    //      Returns true since AlphaScale functionality is available.
    //
    //-------------------------------------------------------------------------

    override bool IsAlphaScalable() const { return true; }

    //+-----------------------------------------------------------------------
    //
    //  Member:    AlphaScale
    //
    //  Synopsis:  Scale (multiply) the current alpha mask by the given scale
    //
    //------------------------------------------------------------------------

    override void AlphaScale(
        FLOAT alphaScale
        );


    override HRESULT Realize(
        );

    override HRESULT SendDeviceStates(
        DWORD dwStage,
        DWORD dwSampler
        );

    void ResetAlphaScaleFactor() { m_alphaScale = 1.0f; }

    void SetContext(
        __in_ecount(1) const MILMatrix3x2 *pMatXSpaceToSourceClip
        );

private:
    HRESULT FillTexture();

private:
    CHwVidMemTextureManager m_vidMemManager;
    
    FLOAT m_alphaScale;
    FLOAT m_alphaScaleRealized;
};




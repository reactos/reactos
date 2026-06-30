// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CHwSolidColorTextureSource declaration
//

MtExtern(CHwSolidColorTextureSource);

//+----------------------------------------------------------------------------
//
//  Class:     CHwSolidColorTextureSource
//
//  Synopsis:  Provides a solid color texture source for a HW device
//
//-----------------------------------------------------------------------------

class CHwSolidColorTextureSource : public CHwTexturedColorSource
{
public:
    static __checkReturn HRESULT Create(
        __in_ecount(1) CD3DDeviceLevel1 *pD3DDevice,
        __deref_out_ecount(1) CHwSolidColorTextureSource **ppTextureSource
        );

private:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwSolidColorTextureSource));

    CHwSolidColorTextureSource(
        __in_ecount(1) CD3DDeviceLevel1 *pD3DDevice
        );

    ~CHwSolidColorTextureSource();

public:

    void SetColor(
        __in_ecount(1) const MilColorF &color
        )
    { m_fValidRealization = false; m_color = color; }


    //
    // CHwColorSource methods
    //

    override TypeFlags GetSourceType() const;

    override bool IsOpaque(
        ) const
    {
        // Note this comparison is too restrictive for sRGB which has less
        // granularity and is considered opaque at values less than 1.
        return (m_color.a >= 1.0f);
    }

    override HRESULT Realize(
        );

    override HRESULT SendDeviceStates(
        DWORD dwStage,
        DWORD dwSampler
        );

private:
    __checkReturn HRESULT CreateLockableTexture();
    HRESULT FillTexture();

private:
    CD3DLockableTexture *m_pLockableTexture;
    bool m_fValidRealization;
    MilColorF m_color;
};




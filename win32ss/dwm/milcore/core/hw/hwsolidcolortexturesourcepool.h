// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CHwSolidColorTextureSourcePool declaration
//

MtExtern(CHwSolidColorTextureSourcePool);

class CHwSolidColorTextureSourcePool
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwSolidColorTextureSourcePool));

    CHwSolidColorTextureSourcePool();
    ~CHwSolidColorTextureSourcePool();

public:
    HRESULT Init(__in_ecount(1) CD3DDeviceLevel1 *pD3DDevice);

    //+----------------------------------------------------------------------------
    //
    //  Function:  CHwSolidColorTextureSource::Clear
    //
    //  Synopsis:  Tells the pool it can start reusing texture sources.
    //
    //-----------------------------------------------------------------------------
    void Clear()
        { m_uNumTexturesOpen = 0; }

    HRESULT RetrieveTexture(
        __in_ecount(1) const MilColorF &color,
        __deref_out_ecount(1) CHwSolidColorTextureSource **ppTexture
        );

private:
    HRESULT AddTexture();

private:
    DynArray<CHwSolidColorTextureSource *> m_rgTextures;
    CD3DDeviceLevel1 *m_pD3DDeviceNoRef;
    UINT m_uNumTexturesOpen;
};




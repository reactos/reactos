// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains vid mem only texture class declaration
//

MtExtern(CD3DVidMemOnlyTexture);
MtExtern(D3DResource_VidMemOnlyTexture);

class CHwVidMemTextureManager;

class CD3DVidMemOnlyTexture : public CD3DTexture
{
protected:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CD3DVidMemOnlyTexture));

public:
    DEFINE_RESOURCE_REF_COUNT_BASE

    static HRESULT Create(
        __in_ecount(1) const D3DSURFACE_DESC *pSurfDesc,
        UINT uLevels,
        bool fIsEvictable,
        __inout_ecount(1) CD3DDeviceLevel1 *pDevice,
        __deref_out_ecount(1) CD3DVidMemOnlyTexture **ppVidMemOnlyTexture,
        __deref_opt_inout_ecount(1) HANDLE * const pSharedHandle
        );

    static HRESULT Create(
        __inout_ecount(1) IDirect3DTexture9 *pD3DExistingTexture,
        bool fIsEvictable,
        __inout_ecount(1) CD3DDeviceLevel1 *pDevice,
        __deref_out_ecount(1) CD3DVidMemOnlyTexture **ppVidMemOnlyTexture
        );

protected:
    CD3DVidMemOnlyTexture();
    ~CD3DVidMemOnlyTexture();

    HRESULT Init(
        __inout_ecount(1) CD3DResourceManager *pResourceManager,
        __inout_ecount(1) IDirect3DTexture9 *pD3DTexture
        );

#if PERFMETER
    virtual PERFMETERTAG GetPerfMeterTag() const
    {
        return Mt(D3DResource_VidMemOnlyTexture);
    }
#endif
};




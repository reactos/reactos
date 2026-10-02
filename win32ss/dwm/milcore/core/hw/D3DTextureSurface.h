// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains D3D texture surface level wrapper class declaration
//

MtExtern(CD3DTextureSurface);
MtExtern(D3DResource_TextureSurface);

class CD3DTextureSurface : public CD3DSurface
{
private:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CD3DTextureSurface));

public:
    static HRESULT Create(
        __inout_ecount(1) CD3DResourceManager *pResourceManager,
        __inout_ecount(1) IDirect3DSurface9 *pID3DSurface,
        __deref_out_ecount(1) CD3DSurface **ppSurface
        );

protected:

    CD3DTextureSurface(__inout_ecount(1) IDirect3DSurface9 * pD3DSurface);

    HRESULT Init(
        __inout_ecount(1) CD3DResourceManager *pResourceManager
        );

#if PERFMETER
    virtual override PERFMETERTAG GetPerfMeterTag() const
    {
        return Mt(D3DResource_TextureSurface);
    }
#endif

private:
    // 
    // CD3DResource methods
    //

    override void ReleaseD3DResources();

};



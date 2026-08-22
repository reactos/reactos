// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//------------------------------------------------------------------------------


MtExtern(CHwPixelShaderEffect);
MtExtern(CHwPixelShaderEffect_NotImplementedAndShouldAlwaysBeZero);


//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwPixelShaderEffect
//
//  Synopsis:
//      This class implements the life-time managment for a device specific 
//      D3D pixel shader resource by inheriting from CD3DResource. It is also
//      cacheable using the CMILCacheableResource infrastructure. 
//
//------------------------------------------------------------------------------

class CHwPixelShaderEffect : 
    public CD3DResource,            // Enables device lost tracking so the resource
                                    // can be destroyed when the device is lost. 
    public CMILCacheableResource    // Enables caching of this resource in a 
                                    // CMILSimpleResourceCache. 
{
protected:
    CHwPixelShaderEffect() {}

    ~CHwPixelShaderEffect()
    {
        // Release all D3DResources.
        ReleaseD3DResources();
    }
    
public:

    // 
    // New Operator
    //
    //     Override new operator to work with debug MeterHeap and
    //     zero initialize the object.
    //

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CHwPixelShaderEffect));


    // 
    // AddRef, Release defined via CD3DResource
    //

    DEFINE_RESOURCE_REF_COUNT_BASE

    //
    // Create 
    //
    //    Creates a device dependent D3D pixel shader object. 
    //

    static HRESULT Create(
        __in CD3DDeviceLevel1 *pDevice,
        __in_bcount(sizeInBytes) BYTE *pPixelShaderByteCode,
        __in UINT sizeInBytes,
        __out CHwPixelShaderEffect **ppHwPixelShaderEffect);
   
    //
    // SendToDevice
    //
    //    Configures the device for rendering with this pixel 
    //    shader. 
    //
    // Remark: 
    //    pDevice must point to the same device which has been used when
    //    creating this CHwPixelShaderEffect, otherwise this may fail.
    //    This class only contains verification to ensure that the proper 
    //    device is being used in debug builds.

    HRESULT SendToDevice(
        __in CD3DDeviceLevel1 *pDevice);


#if PERFMETER
    // 
    // CD3DResource::GetPerfMeterTag (override)
    //
    //    Used to publish video memory consumption for this resource
    //    type. However, since it is not known how much video memory a
    //    shader consumes, for this resource this is effectively not
    //    implemented. 
    //
    //
    override PERFMETERTAG GetPerfMeterTag() const
    {
        return Mt(CHwPixelShaderEffect_NotImplementedAndShouldAlwaysBeZero);
    }
#endif

    // 
    // CD3DResource::ReleaseD3DResources (override)
    //
    //    Release D3D resources associated with this PixelShaderEffect.
    //    ReleaseD3DResources is called by the device on device lost, shutdown, etc.
    //
    override void ReleaseD3DResources();

    //
    // CMILCacheableResource::IsValid (override)
    //
    //     This resource is valid if the D3D resource is valid. Therefore
    //     mappping IsValid to CD3DResource::IsValid.
    //
    override bool IsValid() const
    {
        return CD3DResource::IsValid();
    }

private:

    HRESULT Init(
        __in CD3DDeviceLevel1 *pDevice,
        __in_bcount(sizeInBytes) BYTE *pPixelShaderByteCode,
        __in UINT sizeInBytes);
        
private:

    IDirect3DPixelShader9 *m_pD3DPixelShader; // D3D pixel shader. 

#if DBG
    CD3DDeviceLevel1 *m_pDbgDeviceNoRef;
#endif
};



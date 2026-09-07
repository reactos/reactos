// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_resourcemgmt
//      $Keywords:
//
//  $Description:
//      Contains the CHwDestinationTexturePool class
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CHwDestinationTexturePool);


//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwDestinationTexturePool
//
//  Synopsis:
//      Controls realized instances of CHwDestinationTexture objects.
//
//      This class will take creation parameters for a CHwDestinationTexture and
//      either return an unused/cached texture or create a new one.
//
//      This pool is intended to live in a CD3DDeviceLevel1 as a member.
//
//------------------------------------------------------------------------------
class CHwDestinationTexturePool : public IMILPoolManager
{
public:
    static __checkReturn HRESULT Create(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        __deref_out_ecount(1) CHwDestinationTexturePool **ppDestinationTexturePool
        );

    HRESULT GetHwDestinationTexture(
        __deref_out_ecount(1) CHwDestinationTexture **ppHwDestinationTexture
        );

    // Used to notify the manager that there are no outstanding uses and
    //  the manager has full control.
    void UnusedNotification(
        __inout_ecount(1) CMILPoolResource *pUnused
        );

    // Used to notify the manager that the resource is no longer usable
    //  and should be removed from the pool.
    void UnusableNotification(
        __inout_ecount(1) CMILPoolResource *pUnusable
        );

    void Release();
    
private:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwDestinationTexturePool));

    CHwDestinationTexturePool(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice
        );

    ~CHwDestinationTexturePool();
    
private:
    void AddToUnused(
        __in_ecount(1) CHwDestinationTexture *pHwDestTexture
        );

    void RemoveFromUnused(
        __deref_out_ecount(1) CHwDestinationTexture **ppHwDestTexture
        );

    // This method reduces the count of textures that will call this
    // manager at some time.  When there are no outstanding textures
    // and the pool, which created this pool manager, Release's it,
    // the count will reach -1 and the object will be deleted.
    MIL_FORCEINLINE void DecOutstanding()
    {
        m_cOutstandingTextures--;

        if (m_cOutstandingTextures == -1)
        {
            delete this;
        }
    }
    
private:
    CD3DDeviceLevel1 * const m_pDevice;

    // List of brushes that have recently become unused
    LIST_ENTRY m_oUnusedListHead;
    UINT m_cUnusedTextures;

    // Count of all textures currently in use.  When the manager
    // has been released by the referencing pool object this
    // value is decremented by 1 thus enabling it to reach -1.
    // When the count is -1 this manager should be deleted.
    LONG m_cOutstandingTextures;    
};





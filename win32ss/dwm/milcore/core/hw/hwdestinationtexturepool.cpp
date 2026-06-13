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
//      Contains the CHwDestinationTexturePool implementation
//
//  $ENDTAG
//
//------------------------------------------------------------------------------
#include "precomp.hpp"

#define DESTINATION_TEXTURES_POOL_LIMIT 4

MtDefine(CHwDestinationTexturePool, MILRender, "CHwDestinationTexturePool");


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDestinationTexture::ctor
//
//  Synopsis:
//      Set members to NULL.
//
//------------------------------------------------------------------------------
CHwDestinationTexturePool::CHwDestinationTexturePool(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice
    ) : m_pDevice(pDevice)
{
    m_cOutstandingTextures = 0;
    m_cUnusedTextures = 0;

    InitializeListHead(&m_oUnusedListHead);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDestinationTexture::dtor
//
//  Synopsis:
//      Release members
//
//------------------------------------------------------------------------------
CHwDestinationTexturePool::~CHwDestinationTexturePool()
{
    CHwDestinationTexture *pCachedTexture = NULL;
    Assert(m_cOutstandingTextures == -1);

    PLIST_ENTRY poRemovedEntry = NULL;

    for (;;)
    {
        poRemovedEntry = RemoveTailList(&m_oUnusedListHead);

        if (poRemovedEntry == &m_oUnusedListHead)
        {
            break;
        }
        else
        {
            pCachedTexture = CONTAINING_RECORD(
                poRemovedEntry,
                CHwDestinationTexture,
                m_oUnusedPoolListEntry
                );

            Assert(pCachedTexture->GetRefCount() == 0);
            delete pCachedTexture;

            --m_cUnusedTextures;
        }
    }

    Assert(m_cUnusedTextures == 0);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDestinationTexturePool::Release
//
//  Synopsis:
//      Release this pool manager from the pool.  The only valid caller of this
//      method is its owner, which should be a pool.
//
//------------------------------------------------------------------------------
void CHwDestinationTexturePool::Release()
{
    // Decrement the outstanding brush count so that it may now reach
    // -1 signaling the need for object deletion.
    DecOutstanding();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDestinationTexture::Create
//
//  Synopsis:
//      Create a pool
//
//------------------------------------------------------------------------------
__checkReturn HRESULT 
CHwDestinationTexturePool::Create(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice,
    __deref_out_ecount(1) CHwDestinationTexturePool **ppDestinationTexturePool
    )
{
    HRESULT hr = S_OK;
    CHwDestinationTexturePool *pNewTexturePool = NULL;

    pNewTexturePool = new CHwDestinationTexturePool(pDevice);
    IFCOOM(pNewTexturePool);

    *ppDestinationTexturePool = pNewTexturePool; // steal the reference
    pNewTexturePool = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pNewTexturePool);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDestinationTexture::UnusedNotification
//
//  Synopsis:
//      Used to notify the manager that there are no outstanding uses and the
//      manager has full control.
//
//------------------------------------------------------------------------------
void
CHwDestinationTexturePool::UnusedNotification(
    __inout_ecount(1) CMILPoolResource *pUnused
    )
{
    Assert(m_pDevice);

    CHwDestinationTexture *pDestTexture = DYNCAST(CHwDestinationTexture, pUnused);
    Assert(pDestTexture);

    AddToUnused(pDestTexture);

    //
    // Trim pool if it exceeds quota
    //

    if (m_cUnusedTextures > DESTINATION_TEXTURES_POOL_LIMIT)
    {
        //
        // Remove least recently used texture with expectation that its size
        // is too small for the next use.  RT Layers have a stack pattern.
        //

        PLIST_ENTRY poRemovedEntry = RemoveTailList(&m_oUnusedListHead);
        m_cUnusedTextures--;

        CHwDestinationTexture *pCachedTexture = CONTAINING_RECORD(
            poRemovedEntry,
            CHwDestinationTexture,
            m_oUnusedPoolListEntry
            );

        delete pCachedTexture;
    }

    DecOutstanding();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDestinationTexture::AddToUnused
//
//  Synopsis:
//      Add to list of available, unused destination textures.
//
//------------------------------------------------------------------------------
void
CHwDestinationTexturePool::AddToUnused(
    __in_ecount(1) CHwDestinationTexture *pHwDestTexture
    )
{
    Assert(pHwDestTexture->GetRefCount() == 0);

    InsertHeadList(&m_oUnusedListHead, &pHwDestTexture->m_oUnusedPoolListEntry);
    m_cUnusedTextures++;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDestinationTexture::RemoveFromUnused
//
//  Synopsis:
//      Remove entry from list of unused destination textures.
//
//------------------------------------------------------------------------------
void
CHwDestinationTexturePool::RemoveFromUnused(
    __deref_out_ecount(1) CHwDestinationTexture **ppHwDestTexture
    )
{
    CHwDestinationTexture *pCachedTexture = NULL;

    PLIST_ENTRY poRemovedEntry;

    Assert(m_cUnusedTextures > 0);
    Assert(!IsListEmpty(&m_oUnusedListHead));

    //
    // Reuse most recently pushed texture with expecation that size will be
    // about the same or that it will be reused in the same context.
    //

    poRemovedEntry = RemoveHeadList(&m_oUnusedListHead);

    pCachedTexture = CONTAINING_RECORD(
        poRemovedEntry,
        CHwDestinationTexture,
        m_oUnusedPoolListEntry
        );

    pCachedTexture->AddRef();

    m_cOutstandingTextures++;

    *ppHwDestTexture = pCachedTexture;

    m_cUnusedTextures--;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDestinationTexture::UnusableNotification
//
//  Synopsis:
//      Used to notify the manager that the resource is no longer usable and
//      should be removed from the pool.
//
//      Currently it is never called.
//
//------------------------------------------------------------------------------
void
CHwDestinationTexturePool::UnusableNotification(
    __inout_ecount(1) CMILPoolResource *pUnusable
    )
{
    Assert(m_pDevice);

    RIP("DestinationTexturePOOL::UnusableNotification should never be called");
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDestinationTexture::GetHwDestinationTexture
//
//  Synopsis:
//      Grabs a HwDestination Texture by either returning an existing unused
//      destination texture if there are any available, or creates a new one.
//
//------------------------------------------------------------------------------
HRESULT 
CHwDestinationTexturePool::GetHwDestinationTexture(
    __deref_out_ecount(1) CHwDestinationTexture **ppHwDestinationTexture
    )
{
    HRESULT hr = S_OK;

    if (m_cUnusedTextures)
    {
        RemoveFromUnused(ppHwDestinationTexture);
    }
    else
    {
        IFC(CHwDestinationTexture::Create(
            m_pDevice,
            this,
            ppHwDestinationTexture
            ));

        m_cOutstandingTextures++;
    }

Cleanup:
    RRETURN(hr);
}





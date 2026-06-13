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
//      Contains the implementation for the CHwShaderCacheNode and
//      CHwShaderCache classes.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------


#include "precomp.hpp"

MtDefine(CHwShaderCache, MILRender, "CHwShaderCache");
MtDefine(CHwShaderCacheNode, CHwShaderCache, "CHwShaderCacheNode");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderCacheNode::CHwShaderCacheNode
//
//  Synopsis:
//      Initialize members.
//

CHwShaderCacheNode::CHwShaderCacheNode(
    __in_ecount(1) HwPipelineItem const &oItem
    )
{
    m_pCompiledShader = NULL;
    m_hrPreviouslyCompiledFailure = S_OK;

    m_oItem.dwSampler = oItem.dwSampler;
    m_oItem.mvfaTextureCoordinates = oItem.mvfaTextureCoordinates;
    m_oItem.pFragment = oItem.pFragment;
};

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderCacheNode::GetChildNode
//
//  Synopsis:
//      Retrieves the next cached node which can render the specified pipeline
//      item.
//
//------------------------------------------------------------------------------
HRESULT
CHwShaderCacheNode::GetChildNode(
    __in_ecount(1) HwPipelineItem const &oPipelineItem,
    __deref_out_ecount(1) CHwShaderCacheNode **ppChildNode
    )
{
    HRESULT hr = S_OK;
    CHwShaderCacheNode *pNewCacheNode = NULL;
    bool fMatchFound = false;

    for (UINT i = 0; i < m_rgpChildNodes.GetCount(); i++)
    {
        HwPipelineItem const &pCurrentItem = m_rgpChildNodes[i]->m_oItem;

        if (   pCurrentItem.dwSampler == oPipelineItem.dwSampler
            && pCurrentItem.mvfaTextureCoordinates == oPipelineItem.mvfaTextureCoordinates
            && pCurrentItem.pFragment == oPipelineItem.pFragment
               )
        {
            *ppChildNode = m_rgpChildNodes[i];
            fMatchFound = true;
            break;
        }
    }

    if (!fMatchFound)
    {
        pNewCacheNode = new CHwShaderCacheNode(oPipelineItem);
        IFCOOM(pNewCacheNode);

        IFC(m_rgpChildNodes.Add(pNewCacheNode));

        *ppChildNode = pNewCacheNode;
        pNewCacheNode = NULL;
    }

Cleanup:
    delete pNewCacheNode;

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderCacheNode::~CHwShaderCacheNode
//
//  Synopsis:
//      Destroys all child nodes
//

CHwShaderCacheNode::~CHwShaderCacheNode()
{
    ReleaseInterfaceNoNULL(m_pCompiledShader);

    for (UINT i = 0; i < m_rgpChildNodes.GetCount(); i++)
    {
        delete m_rgpChildNodes[i];
    }
};

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderCache::Create
//
//  Synopsis:
//      Creates the Cache.
//

__checkReturn HRESULT
CHwShaderCache::Create(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice,
    __deref_out_ecount(1) CHwShaderCache **ppCache
    )
{
    HRESULT hr = S_OK;

    CHwShaderCache *pNewCache = NULL;

    pNewCache = new CHwShaderCache(pDevice);
    IFCOOM(pNewCache);

    pNewCache->AddRef();

    IFC(pNewCache->Init());

    *ppCache = pNewCache;       // Steal ref
    pNewCache = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pNewCache);

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderCache::Reset
//
//  Synopsis:
//      Initializes the cache to the correct 2D root
//

void
CHwShaderCache::Reset()
{
    m_pCurrentNode = m_pRootNode;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderCache::AddOperation
//
//  Synopsis:
//      Updates the cache to support the specified item.
//
//------------------------------------------------------------------------------
HRESULT
CHwShaderCache::AddOperation(
    HwPipelineItem &oPipelineItem
    )
{
    HRESULT hr = S_OK;
    
    IFC(m_pCurrentNode->GetChildNode(
        oPipelineItem,
        &m_pCurrentNode
        ));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderCache::GetHwShader
//
//  Synopsis:
//      Retrieves a shader based on all the operations that were added to the
//      cache.
//
//      NOTE: This function can return S_OK with *pfFailedCompile ==
//            true
//
//            This means that no shader could be built with those operations.
//

HRESULT
CHwShaderCache::GetHwShader(
    __in_ecount(uNumPipelineItems) const HwPipelineItem *rgShaderItem,
    UINT uNumPipelineItems,
    __deref_out_ecount(1) CHwPipelineShader **ppHwShader
    )
{
    HRESULT hr = S_OK;

    CHwPipelineShader *pReturnShader = NULL;
    HRESULT hrPreviouslyFailedResult = S_OK;

    *ppHwShader = NULL;

    m_pCurrentNode->GetHwShader(
        &pReturnShader,
        &hrPreviouslyFailedResult
        );

    if (SUCCEEDED(hrPreviouslyFailedResult))
    {
        if (pReturnShader == NULL)
        {
            //
            // We don't have the shader cached, so try to build it
            //
            hr = m_pDeviceNoRef->DerivePipelineShader(
                rgShaderItem,
                uNumPipelineItems,
                &pReturnShader
                );

            if (FAILED(hr))
            {
                m_pCurrentNode->SetFailedCompile(hr);

                IFC(hr);
            }

            //
            // Set the new shader in the node.
            //
            m_pCurrentNode->SetHwShader(
                pReturnShader
                );
        }
    }
    else
    {
        //
        // Last time we tried this shader it didn't compile. Bubble up
        // an error so we fall back to FF
        //
        IFC(hrPreviouslyFailedResult);
    }

    *ppHwShader = pReturnShader; // Steal ref
    pReturnShader = NULL;

Cleanup:
    
    ReleaseInterfaceNoNULL(pReturnShader);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderCache::CHwShaderCache
//
//  Synopsis:
//      Intialize members.
//

CHwShaderCache::CHwShaderCache(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice
    )
{
    m_pRootNode = NULL;

    m_pDeviceNoRef = pDevice;
    m_pCurrentNode = NULL;
}



//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderCache::~CHwShaderCache
//
//  Synopsis:
//      Destroys both trees and releases the fragment to hlsl converter.
//

CHwShaderCache::~CHwShaderCache()
{
    delete m_pRootNode;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderCache::Init
//
//  Synopsis:
//      Creates both root nodes and gets a fragment to hlsl converter.
//

HRESULT
CHwShaderCache::Init()
{
    HRESULT hr = S_OK;

    Assert(m_pRootNode == NULL);

    HwPipelineItem oDummyItem;

    oDummyItem.dwStage                  = static_cast<DWORD>(INVALID_PIPELINE_STAGE);
    oDummyItem.dwSampler                = static_cast<DWORD>(INVALID_PIPELINE_SAMPLER);
    oDummyItem.pHwColorSource           = NULL;
    oDummyItem.pFragment                = NULL;
    oDummyItem.mvfaTextureCoordinates   = MILVFAttrNone;

    m_pRootNode = new CHwShaderCacheNode(oDummyItem);
    IFCOOM(m_pRootNode);

Cleanup:
    RRETURN(hr);
}






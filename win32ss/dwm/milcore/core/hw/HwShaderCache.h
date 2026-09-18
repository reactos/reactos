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
//      Contains the definition for the CHwShaderCacheNode and
//      CHwShaderCache classes.       
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CHwShaderCache);
MtExtern(CHwShaderCacheNode);

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwShaderCacheNode
//
//  Synopsis:
//      Node of the CHwShaderCache.  It holds onto a compiled shader and child
//      nodes for each possible pipeline operation.
//
//------------------------------------------------------------------------------

class CHwShaderCacheNode
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwShaderCacheNode));

    CHwShaderCacheNode(
        __in_ecount(1) HwPipelineItem const &oItem
        );

    ~CHwShaderCacheNode();

    HRESULT GetChildNode(
        __in_ecount(1) HwPipelineItem const &oPipelineItem,
        __deref_out_ecount(1) CHwShaderCacheNode **ppChildNode
        );

    void SetFailedCompile(HRESULT hrFailure)
    {
        Assert(FAILED(hrFailure));

        m_hrPreviouslyCompiledFailure = hrFailure;
    }

    void GetHwShader(
        __deref_out_ecount(1) CHwPipelineShader **ppHwShader,
        __out_ecount(1) HRESULT *pfPreviouslyFailedResult
        )
    {
        *ppHwShader = m_pCompiledShader;
        *pfPreviouslyFailedResult = m_hrPreviouslyCompiledFailure;

        if (m_pCompiledShader)
        {
            m_pCompiledShader->AddRef();
        }
    }

    void SetHwShader(
        __in_ecount(1) CHwPipelineShader *pHwShader
        )
    {
        Assert(m_pCompiledShader == NULL);

        m_pCompiledShader = pHwShader;
        m_pCompiledShader->AddRef();
    }

private:
    DynArray<CHwShaderCacheNode *> m_rgpChildNodes;

    HwPipelineItem m_oItem;
    CHwPipelineShader *m_pCompiledShader;
    HRESULT m_hrPreviouslyCompiledFailure;
};


//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwShaderCache
//
//  Synopsis:
//      Cache of all compiled pipeline shaders.
//
//------------------------------------------------------------------------------

class CHwShaderCache :
    public CMILRefCountBase
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwShaderCache));

    static __checkReturn HRESULT Create(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        __deref_out_ecount(1) CHwShaderCache **ppCache
        );

    void Reset();

    HRESULT AddOperation(
        HwPipelineItem &oPipelineItem
        );

    HRESULT GetHwShader(
        __in_ecount(uNumPipelineItems) HwPipelineItem const *rgShaderItem,
        UINT uNumPipelineItems,
        __deref_out_ecount(1) CHwPipelineShader **ppHwShader
        );

private:
    CHwShaderCache(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice
        );

    ~CHwShaderCache();

    HRESULT Init();

private:
    CD3DDeviceLevel1 *m_pDeviceNoRef;

    CHwShaderCacheNode *m_pRootNode;
    CHwShaderCacheNode *m_pCurrentNode;
};




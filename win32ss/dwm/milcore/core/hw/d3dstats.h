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
//      Contains CD3DStats implementation
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#if DBG

MtExtern(CD3DStats);

//+-----------------------------------------------------------------------------
//
//  Class:
//      D3DDEVINFO_DDISTATS
//
//  Synopsis:
//      Query data for DDI status
//
//------------------------------------------------------------------------------

#define D3DQUERYTYPE_DDISTATS ( D3DQUERYTYPE( 7 ) )
struct D3DDEVINFO_DDISTATS
{
    DWORD   m_dwNumTotalFlushes;         // Number of flushes to the driver
    DWORD   m_dwAvgNumCommandBytes;      // Average number of bytes of command data sent down to the driver 
    DWORD   m_dwNumFrontEndStateUpdates; // Number of calls to update front-end state 
    DWORD   m_dwNumShaderUpdates;        // Number of calls to SetupFVF to update front-end state 
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CD3DStats
//
//  Synopsis:
//      Querys and output DX stats
//
//------------------------------------------------------------------------------

class CD3DStats 
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CD3DStats));

    CD3DStats();

    //
    // CD3DDevice::Present notification
    //

    void OnPresent(__inout_ecount(1) IDirect3DDevice9 *pD3DDevice);

private:
    //
    // Query helpers
    //

    void QueryStats(__inout_ecount(1) IDirect3DDevice9 *pD3DDevice);
    
    HRESULT QueryGetData(
        __inout_ecount(1) IDirect3DDevice9 *pD3DDevice, 
        D3DQUERYTYPE d3dQueryType, 
        __out_bcount(dwDataSize) void* pData, 
        DWORD dwDataSize
        ); 

    //
    // Output DDI stats
    //

    void OutputDDIStats(__in_ecount(1) const D3DDEVINFO_DDISTATS &ddiStats);
    void OutputResourceManagerStats(__in_ecount(1) const D3DDEVINFO_RESOURCEMANAGER &resourceManagerStats);
    void OutputVertexStats(__in_ecount(1) const D3DDEVINFO_D3DVERTEXSTATS &vertexStats);

private:
    D3DDEVINFO_DDISTATS m_ddiStatsPrevious;
    D3DDEVINFO_RESOURCEMANAGER m_resourceManagerStatsPrevious;
    D3DDEVINFO_D3DVERTEXSTATS m_vertexStatsPrevious;

    int m_nFrames;
};

#endif // DBG



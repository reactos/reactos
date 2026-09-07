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

#include "precomp.hpp"

#if DBG

MtDefine(CD3DStats, MILRender, "CD3DStats");

const int c_numD3DStatsFrames = 100;

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DStats::CD3DStats
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------
CD3DStats::CD3DStats()
{
    m_nFrames = 0;

    ZeroMemory(&m_ddiStatsPrevious, sizeof(m_ddiStatsPrevious));
    ZeroMemory(&m_resourceManagerStatsPrevious, sizeof(m_resourceManagerStatsPrevious));
    ZeroMemory(&m_vertexStatsPrevious, sizeof(m_vertexStatsPrevious));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DStats::QueryStats
//
//  Synopsis:
//      Query and output each of the stats
//
//------------------------------------------------------------------------------
void 
CD3DStats::QueryStats(
    __inout_ecount(1) IDirect3DDevice9 *pD3DDevice
    )
{
    HRESULT hr = S_OK;
    D3DDEVINFO_DDISTATS ddiStats;
    D3DDEVINFO_RESOURCEMANAGER resourceManagerStats;
    D3DDEVINFO_D3DVERTEXSTATS vertexStats;

    //
    // Get and output the DDI stats
    //

    MIL_THR(QueryGetData(
        pD3DDevice, 
        D3DQUERYTYPE_DDISTATS, 
        &ddiStats, 
        sizeof(ddiStats)
        ));

    if (SUCCEEDED(hr))
    {
        OutputDDIStats(ddiStats);
    }
    else
    {
        TraceTag((tagError, "Failed to query DDI stats (hr = 0x%x)", hr));
    }


    //
    // Get and output the resource manager stats
    //

    MIL_THR(QueryGetData(
        pD3DDevice, 
        D3DQUERYTYPE_RESOURCEMANAGER, 
        &resourceManagerStats, 
        sizeof(resourceManagerStats)
        ));

    if (SUCCEEDED(hr))
    {
        OutputResourceManagerStats(resourceManagerStats);
    }
    else
    {
        TraceTag((tagError, "Failed to query resource manager stats (hr = 0x%x)", hr));
    }

    //
    // Get and output the vertex stats
    //

    MIL_THR(QueryGetData(
        pD3DDevice, 
        D3DQUERYTYPE_VERTEXSTATS, 
        &vertexStats, 
        sizeof(vertexStats)
        ));

    if (SUCCEEDED(hr))
    {
        OutputVertexStats(vertexStats);
    }
    else
    {
        TraceTag((tagError, "Failed to query vertex stats (hr = 0x%x)", hr));
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DStats::OnPresent
//
//  Synopsis:
//      The present notification.  Every c_numD3DStatsFrames frames, we'll query
//      and output the stats.
//
//------------------------------------------------------------------------------
void
CD3DStats::OnPresent(
    __inout_ecount(1) IDirect3DDevice9 *pD3DDevice
    )
{
    m_nFrames++;

    if (m_nFrames % c_numD3DStatsFrames == 0)
    {
        QueryStats(pD3DDevice);
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DStats::QueryGetData
//
//  Synopsis:
//      Create the query, issue it, and call GetData.
//
//------------------------------------------------------------------------------
HRESULT 
CD3DStats::QueryGetData(
    __inout_ecount(1) IDirect3DDevice9 *pD3DDevice, 
    D3DQUERYTYPE d3dQueryType, 
    __out_bcount(dwDataSize) void* pData, 
    DWORD dwDataSize
    )
{
    HRESULT hr = S_OK;
    IDirect3DQuery9 *pD3DQuery = NULL;

    IFC(pD3DDevice->CreateQuery(d3dQueryType, &pD3DQuery));
    IFC(pD3DQuery->Issue(D3DISSUE_END));
    IFC(pD3DQuery->GetData(pData, dwDataSize, 0));

Cleanup:
    ReleaseInterface(pD3DQuery);
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DStats::OutputDDIStats
//
//  Synopsis:
//      Output the DDI stats
//
//------------------------------------------------------------------------------
void 
CD3DStats::OutputDDIStats(
    __in_ecount(1) const D3DDEVINFO_DDISTATS &ddiStats
    )
{
    //
    // Output delta since last query
    //

    TraceTag((tagError, "D3DDEVINFO_DDISTATS (num frames = %d)", c_numD3DStatsFrames));

    TraceTag((
        tagError, 
        "AvgNumCommandBytes = %d",
        ddiStats.m_dwAvgNumCommandBytes - m_ddiStatsPrevious.m_dwAvgNumCommandBytes
        ));

    TraceTag((
        tagError, 
        "NumFrontEndStateUpdates = %d",
        ddiStats.m_dwNumFrontEndStateUpdates - m_ddiStatsPrevious.m_dwNumFrontEndStateUpdates
        ));

    TraceTag((
        tagError, 
        "NumShaderUpdates = %d",
        ddiStats.m_dwNumShaderUpdates - m_ddiStatsPrevious.m_dwNumShaderUpdates
        ));

    TraceTag((
        tagError, 
        "NumTotalFlushes = %d\n",
        ddiStats.m_dwNumTotalFlushes - m_ddiStatsPrevious.m_dwNumTotalFlushes
        ));

    //
    // Update stats
    //

    m_ddiStatsPrevious = ddiStats;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DStats::OutputResourceManagerStats
//
//  Synopsis:
//      Output the resource manager stats
//
//------------------------------------------------------------------------------
void 
CD3DStats::OutputResourceManagerStats(
    __in_ecount(1) const D3DDEVINFO_RESOURCEMANAGER &resourceManagerStats
    )
{
    const D3DRESOURCESTATS resourceStats = resourceManagerStats.stats[D3DRTYPE_TEXTURE];
    const D3DRESOURCESTATS resourceStatsPrevious = m_resourceManagerStatsPrevious.stats[D3DRTYPE_TEXTURE];

    TraceTag((tagError, "D3DDEVINFO_RESOURCEMANAGER (num frames = %d)", c_numD3DStatsFrames));

    //
    // Output per frame stats
    //

    TraceTag((tagError, "bThrashing = %d", resourceStats.bThrashing));
    TraceTag((tagError, "ApproxBytesDownloaded = %d", resourceStats.ApproxBytesDownloaded));
    TraceTag((tagError, "NumEvicts = %d", resourceStats.NumEvicts));
    TraceTag((tagError, "NumVidCreates = %d", resourceStats.NumVidCreates));
    TraceTag((tagError, "LastPri = %d", resourceStats.LastPri));

    //
    // Output accumulated stats
    //

    TraceTag((
        tagError, 
        "WorkingSet = %d",
        resourceStats.WorkingSet - resourceStatsPrevious.WorkingSet
        ));

    TraceTag((
        tagError, 
        "WorkingSetBytes = %d",
        resourceStats.WorkingSetBytes - resourceStatsPrevious.WorkingSetBytes
        ));

    TraceTag((
        tagError, 
        "TotalManaged = %d",
        resourceStats.TotalManaged - resourceStatsPrevious.TotalManaged
        ));

    TraceTag((
        tagError, 
        "TotalBytes = %d\n",
        resourceStats.TotalBytes - resourceStatsPrevious.TotalBytes
        ));

    //
    // Update previous stats
    //

    m_resourceManagerStatsPrevious = resourceManagerStats;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DStats::OutputVertexStats
//
//  Synopsis:
//      Output the vertex stats
//
//------------------------------------------------------------------------------
void 
CD3DStats::OutputVertexStats(
    __in_ecount(1) const D3DDEVINFO_D3DVERTEXSTATS &vertexStats
    )
{
    //
    // Output vertex stats delta since last query
    //

    TraceTag((tagError, "D3DDEVINFO_D3DVERTEXSTATS (num frames = %d)", c_numD3DStatsFrames));

    TraceTag((
        tagError, 
        "NumRenderedTriangles = %d",
        vertexStats.NumRenderedTriangles - m_vertexStatsPrevious.NumRenderedTriangles 
        ));

    TraceTag((
        tagError, 
        "NumExtraClippingTriangles = %d\n",
        vertexStats.NumExtraClippingTriangles - m_vertexStatsPrevious.NumExtraClippingTriangles 
        ));

    //
    // Update previous stats
    //

    m_vertexStatsPrevious = vertexStats;
}

#endif // DBG



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
//      GPUMarker class used to monitor GPU process of rendering
//
//  $ENDTAG
//
//------------------------------------------------------------------------------
MtExtern(CGPUMarker);

class CGPUMarker
{
public:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CGPUMarker));

    CGPUMarker(
        __in_ecount(1) IDirect3DQuery9 *pQuery,
        ULONGLONG ullMarkerId
        );

    ~CGPUMarker();

    void Reset(ULONGLONG ullMarkerId);

    HRESULT InsertIntoCommandStream();

    HRESULT CheckStatus(
        BOOL fFlush,
        __out_ecount(1) bool *pfConsumed
        );
    
    ULONGLONG GetId() const { return m_ullMarkerId; }
    
private:
    IDirect3DQuery9 *m_pQuery;
    ULONGLONG        m_ullMarkerId;
    BOOL             m_fIssued;
    BOOL             m_fConsumed;
};



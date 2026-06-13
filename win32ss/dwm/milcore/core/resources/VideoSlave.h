// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_media
//      $Keywords:
//
//  $Description:
//      Video resource definitions.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------
MtExtern(CMilSlaveVideo);

interface IMILSurfaceRendererProvider;

class CMilSlaveVideo : public CMilSlaveResource
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMilSlaveVideo));

    CMilSlaveVideo(__in_ecount(1) CComposition* pComposition);

    ~CMilSlaveVideo();

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_MEDIAPLAYER;
    }

    bool
    NewFrame(
        void
        );

    void
    InvalidateLastCompositionSampleTime(
        void
        );

    HRESULT
    BeginComposition(
        __in        bool        displaySetChanged,
        __out       bool        *pbFrameReady
        );

    HRESULT
    EndComposition(
        void
        );

    HRESULT
    GetSurfaceRenderer(
        __deref_out_opt IAVSurfaceRenderer **ppSurfaceRenderer
        );

    //------------------------------------------------------------------------
    //
    //   Command handlers
    //
    //------------------------------------------------------------------------
    HRESULT
    ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable *pHandleTable,
        __in_ecount(1) const MILCMD_MEDIAPLAYER* pvideo
        );

private:

    HRESULT
    PrivateGetSurfaceRenderer(
        __deref_out_opt IAVSurfaceRenderer **ppSurfaceRenderer
        );

    CComposition                *m_pDevice;
    IAVSurfaceRenderer          *m_pICurrentRenderer;
    IMILSurfaceRendererProvider *m_pISurfaceRendererProvider;
    bool                        m_notifyUceDirect;
    LONGLONG                    m_lastCompositionSampleTime;
};


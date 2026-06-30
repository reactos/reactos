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
//      Video resource. This file contains the implementation for all the Video
//      resource functionality. This includes creating the resource, update,
//      query, lock and unlock.
//
//  $ENDTAG
//
//  Environment:
//      User mode only.
//
//------------------------------------------------------------------------------
#include "precomp.hpp"

MtDefine(CMilSlaveVideo, MILRender, "CMilSlaveVideo");

// +---------------------------------------------------------------------------
//
// CMilSlaveVideo::CMilSlaveVideo
//
// +---------------------------------------------------------------------------
CMilSlaveVideo::CMilSlaveVideo(__in_ecount(1) CComposition* pComposition)
    : m_pDevice(pComposition),
      m_pICurrentRenderer(NULL),
      m_pISurfaceRendererProvider(NULL),
      m_notifyUceDirect(false),
      m_lastCompositionSampleTime(-1LL)
{
}

// +---------------------------------------------------------------------------
//
// CMilSlaveVideo::~CMilSlaveVideo
//
// +---------------------------------------------------------------------------
CMilSlaveVideo::~CMilSlaveVideo()
{
    //
    // Call EndComposition to release m_pICurrentRenderer.
    // Releasing m_pICurrentRenderer directly without calling EndComposition is
    // unsafe.
    //
    IGNORE_HR(EndComposition());

    if (m_pISurfaceRendererProvider)
    {
        IGNORE_HR(m_pISurfaceRendererProvider->UnregisterResource(this));
        m_pDevice->UnregisterVideo(this);

        ReleaseInterface(m_pISurfaceRendererProvider);
    }
}

// +---------------------------------------------------------------------------
//
// CMilSlaveVideo::NewFrame
//
// Returns true if the frame was actually shown, if false, the caller should
// send a frame notification through UI.
//
// +---------------------------------------------------------------------------
bool
CMilSlaveVideo::NewFrame()
{
    //
    // Notify compositor only if we are running in the same process as composition
    // and it isn't a synchronous render target. (A synchronous render target
    // is handled as an animation in managed code).
    //
    if (m_notifyUceDirect)
    {
        m_pDevice->ScheduleCompositionPass();
    }

    return m_notifyUceDirect;
}

void
CMilSlaveVideo::
InvalidateLastCompositionSampleTime(
    void
    )
{
    m_lastCompositionSampleTime = -1LL;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilSlaveVideo::BeginComposition
//
//  Synopsis:
//      Called at the begining of a composition pass. Media needs to snap the
//      current samples at this point (so that we show a consistent frame for a
//      long composition pass). This is also used by media to work out how many
//      devices we are spanning so that we we move over to another device
//      consistently we can start decoding on it.
//
//------------------------------------------------------------------------------
HRESULT
CMilSlaveVideo::
BeginComposition(
    __in    bool            displaySetChanged,
    __out   bool            *pbFrameReady
    )
{
    HRESULT hr = S_OK;

    BOOL    bFrameReady = FALSE;

    //
    // EndComposition may have been missed on a previous composition pass if
    // there were errors so, too ensure resilience against other errors in the
    // composition pass, we call EndComposition here.
    //
    IFC(EndComposition());

    //
    // In the remote case, we won't have a surface renderer, that will be
    // on the server.
    //
    IFC(PrivateGetSurfaceRenderer(&m_pICurrentRenderer));

    if (NULL != m_pICurrentRenderer)
    {
        IFC(m_pICurrentRenderer->BeginComposition(
                this,
                displaySetChanged,
                !m_notifyUceDirect, // If we have a surface renderer and
                                    // we aren't directly notifying the composition
                                    // engine, then this is a sync channel.
                &m_lastCompositionSampleTime,
                &bFrameReady));

    }

    *pbFrameReady = !!bFrameReady;

Cleanup:
    //
    // We don't want to let any errors through or we'll make the composition engine non-responsive
    //
    Assert(SUCCEEDED(hr));

    RRETURN(S_OK);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilSlaveVideo::EndComposition
//
//  Synopsis:
//      Called for all the video at the end of a composition pass. This allows
//      media to return its samples to the mixer
//
//------------------------------------------------------------------------------
HRESULT
CMilSlaveVideo::
EndComposition(
    void
    )
{
    HRESULT hr = S_OK;

    if (NULL != m_pICurrentRenderer)
    {
        IFC(m_pICurrentRenderer->EndComposition(this));
    }

Cleanup:
    ReleaseInterface(m_pICurrentRenderer);

    //
    // We don't want to let any errors through or we'll make the composition engine
    // non-responsive
    //
    Assert(SUCCEEDED(hr));

    RRETURN(S_OK);
}

HRESULT
CMilSlaveVideo::GetSurfaceRenderer(
    __deref_out_opt IAVSurfaceRenderer **ppSurfaceRenderer
    )
{
    HRESULT hr = S_OK;
    CHECKPTRARG(ppSurfaceRenderer);

    SetInterface(*ppSurfaceRenderer, m_pICurrentRenderer);

Cleanup:
    RRETURN(hr);
}

// +---------------------------------------------------------------------------
//
// CMilSlaveVideo::PrivateGetSurfaceRenderer
//
// +---------------------------------------------------------------------------
HRESULT
CMilSlaveVideo::PrivateGetSurfaceRenderer(
    __deref_out_opt IAVSurfaceRenderer **ppSurfaceRenderer
    )
{
    HRESULT hr = S_OK;

    IAVSurfaceRenderer  *pISurfaceRenderer = NULL;

    if (NULL != m_pISurfaceRendererProvider)
    {
        IFC(m_pISurfaceRendererProvider->GetSurfaceRenderer(&pISurfaceRenderer));
    }

    *ppSurfaceRenderer = pISurfaceRenderer;
    pISurfaceRenderer = NULL;

Cleanup:

    ReleaseInterface(pISurfaceRenderer);

    RRETURN(hr);
}

// +---------------------------------------------------------------------------
//
// CMilSlaveVideo::ProcessUpdate
//
// +---------------------------------------------------------------------------
HRESULT
CMilSlaveVideo::ProcessUpdate(
    __in_ecount(1) CMilSlaveHandleTable *pHandleTable,
    __in_ecount(1) const MILCMD_MEDIAPLAYER* pVideo
    )
{
    HRESULT hr = S_OK;

    IMILSurfaceRendererProvider *pISurfaceRendererProvider = NULL;

    m_notifyUceDirect = !!pVideo->notifyUceDirect;

    IMILMedia *pMedia = reinterpret_cast<IMILMedia*>(pVideo->pMedia);
    IFCNULL(pMedia);

    //
    // No need to actually AddRef pMedia, since the master did it. only transfer reference
    //
    IFC(pMedia->QueryInterface(IID_IMILSurfaceRendererProvider, (LPVOID*)&pISurfaceRendererProvider));

    //
    // A video slave an only ever have one native media source.
    //
    if (m_pISurfaceRendererProvider != NULL)
    {
        if (m_pISurfaceRendererProvider != pISurfaceRendererProvider)
        {
            Assert(!L"We should only have one instance of native media per slave resource.");

            IFC(E_INVALIDARG);
        }
    }
    else
    {
        //
        // Transfer the reference across.
        //
        m_pISurfaceRendererProvider = pISurfaceRendererProvider;
        pISurfaceRendererProvider = NULL;

        IFC(m_pDevice->RegisterVideo(this));

        IFC(m_pISurfaceRendererProvider->RegisterResource(this));
    }

Cleanup:
    ReleaseInterface(pMedia);
    ReleaseInterface(pISurfaceRendererProvider);

    // No need to UnregisterSlaveVideo on failure as the destructor will
    // No need to SetMixListener(NULL) on failure as the destructor will

    RRETURN(hr);
}




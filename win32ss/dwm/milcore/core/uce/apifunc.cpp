// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Abstract:
//     MIL core flat API set.
//
//------------------------------------------------------------------------------

#include "precomp.hpp"
#include "osversionhelper.h"

extern void DumpInstrumentationData();

ExternTag(tagMILConnection);

//+-----------------------------------------------------------------------------
//
//    Function:
//        MilVersionCheck
//
//    Synopsis:
//        Provides means for the caller to verify that this binary has been
//        built with exactly the same SDK version the caller is using.
//
//    Returns:
//        S_OK if milcore.dll was built with the specified MIL SDK version.
//        WGXERR_UNSUPPORTEDVERSION otherwise.
//
//------------------------------------------------------------------------------

HRESULT WINAPI
MilVersionCheck(
    UINT uiCallerMilSdkVersion
    )
{
    HRESULT hr = S_OK;

    if (uiCallerMilSdkVersion != MIL_SDK_VERSION)
    {
        TraceTag((tagMILWarning,
                  "MilVersionCheck: binary version mismatch (caller: 0x%08x, callee: 0x%08x), abort operation.",
                  uiCallerMilSdkVersion,
                  MIL_SDK_VERSION
                  ));

        IFC(WGXERR_UNSUPPORTEDVERSION);
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Function:
//        MilCompositionEngine_InitializePartitionManager
//
//    Synopsis:
//        Creates and initializes a partition manager. This will result in
//        creation of infrastructure necessary to perform composition in the
//        current process. Among other, a scheduler and a set of worker threads
//        will be created.
//
//------------------------------------------------------------------------------

HRESULT WINAPI
MilCompositionEngine_InitializePartitionManager(
    int nPriority
    )
{
    RRETURN(EnsurePartitionManager(nPriority));
}


//+-----------------------------------------------------------------------------
//
//    Function:
//        MilCompositionEngine_UpdateSchedulerSettings
//
//    Synopsis:
//        Asks the partition manager to change scheduler settings.
//
//------------------------------------------------------------------------------

HRESULT WINAPI
MilCompositionEngine_UpdateSchedulerSettings(
    int nPriority
    )
{
    RRETURN(UpdateSchedulerSettings(nPriority));
}


//+-----------------------------------------------------------------------------
//
//    Function:
//        MilCompositionEngine_DeinitializePartitionManager
//
//    Synopsis:
//        Releases the partition manager and all the relevant infrastructure.
//
//------------------------------------------------------------------------------

HRESULT WINAPI
MilCompositionEngine_DeinitializePartitionManager()
{
    ReleasePartitionManager();

    return S_OK;
}


//+-----------------------------------------------------------------------
//
//    Function:
//        WgxConnection_SameThreadPresent
//
//    Synopsis:
//        Presents on the same thread sync compositor.
//------------------------------------------------------------------------


HRESULT WINAPI WgxConnection_SameThreadPresent(
    __in HMIL_CONNECTION hConnection
    )
{
    HRESULT hr = S_OK;
    CMilConnection* pConnection = HandleToPointer(hConnection);
    IFC(pConnection->PresentAllPartitions());

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------
//
//    Function:
//        WgxConnection_ShouldForceSoftwareForGraphicsStreamClient
//
//------------------------------------------------------------------------

BOOL WINAPI 
WgxConnection_ShouldForceSoftwareForGraphicsStreamClient()
{
    BOOL fForceSoftwareForGraphicsStreamClient = FALSE;
    
    //
    // Discover graphics stream clients, but only on Vista. On OS < Vista, graphics
    // stream clients were not available. On OS > Vista, we don't want to force sw if a graphics 
    // stream client is present since the magnifier on OS > Vista will support magnifying DX content.
    //

    if (WPFUtils::OSVersionHelper::IsWindowsVistaOrGreater() && 
        !WPFUtils::OSVersionHelper::IsWindows7OrGreater())
    {
        UUID uuid;
        HRESULT hrEnumerate = GetGraphicsStreamClient(0 /* Only looking if there are any at all */, &uuid);

        if (hrEnumerate == S_OK)
        {
            fForceSoftwareForGraphicsStreamClient = TRUE;
        }
    }

    return fForceSoftwareForGraphicsStreamClient;
}


//+-----------------------------------------------------------------------------
//
//    Function:
//        WgxConnection_Create
//
//    Synopsis:
//        Creates a client transport object.
//
//------------------------------------------------------------------------------

HRESULT WINAPI WgxConnection_Create(
    bool requestSynchronousTransport,
    HMIL_CONNECTION *phConnection
    )
{
    HRESULT hr = S_OK;
    CMilConnection *pConnection = NULL;
    CHECKPTRARG(phConnection);

    
    IFC(CMilConnection::Create(
        requestSynchronousTransport ? MilMarshalType::SameThread : MilMarshalType::CrossThread,
        OUT &pConnection));

    *phConnection = PointerToHandle(pConnection);
    pConnection = NULL;

Cleanup:
    ReleaseInterface(pConnection);
    
    RRETURN(hr);
}




HRESULT WINAPI WgxConnection_Disconnect(
    HMIL_CONNECTION hConnection
    )
{
    HRESULT hr = S_OK;

    CHECKPTRARG(hConnection);

    CMilConnection *pConnection = HandleToPointer(hConnection);

    pConnection->Release();

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI MilConnection_CreateChannel(
    HMIL_CONNECTION hConnection,
    MIL_CHANNEL hSourceChannel,
    MIL_CHANNEL *phChannel
    )
{
    HRESULT hr = S_OK;
    HMIL_CHANNEL hPartSource = NULL;

    CHECKPTRARG(phChannel);

    CMilConnection *pConnection = NULL;
    CHECKPTRARG(hConnection);

    const CMilChannel *pSourceChannel = HandleToPointer(hSourceChannel);
    CMilChannel *pChannel = NULL;

    pConnection = HandleToPointer(hConnection);

    if (pSourceChannel)
    {
        hPartSource = pSourceChannel->GetChannel();
    }

    IFC(pConnection->CreateChannel(hPartSource, &pChannel));
    *phChannel = PointerToHandle(pChannel);

    EventWriteCreateChannel(pChannel, pChannel->GetChannel());

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI MilConnection_DestroyChannel(
    MIL_CHANNEL hChannel
    )
{
    HRESULT hr = S_OK;
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);

    IFC(pChannel->Destroy());

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI MilChannel_CloseBatch(
    MIL_CHANNEL hChannel
    )
{
    HRESULT hr = S_OK;
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);

    IFC(pChannel->CloseBatch());

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI MilChannel_CommitChannel(
    MIL_CHANNEL hChannel
    )
{
    HRESULT hr = S_OK;
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);

    IFC(pChannel->Commit());

Cleanup:
    RRETURN(hr);
}


 
HRESULT WINAPI MilComposition_SyncFlush(
    MIL_CHANNEL hChannel
    )
{
    HRESULT hr = S_OK;
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);

    IFC(pChannel->SyncFlush());

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI MilComposition_PeekNextMessage(
    __in MIL_CHANNEL hChannel,
    __out_bcount_part(cbSize, sizeof(MIL_MESSAGE)) MIL_MESSAGE *pmsg,
    __in size_t cbSize,
    __out_ecount(1) BOOL *pfMessageRetrieved
    )
{
    HRESULT hr = S_OK;
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);
    CHECKPTRARG(pmsg);
    CHECKPTRARG(pfMessageRetrieved);

    IFC(pChannel->PeekNextMessage(pmsg, cbSize, pfMessageRetrieved));

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI MilComposition_WaitForNextMessage(
    MIL_CHANNEL hChannel,
    DWORD nCount,
    const HANDLE *pHandles,
    BOOL bWaitAll,
    DWORD waitTimeout,
    DWORD *pWaitReturn
    )
{
    HRESULT hr = S_OK;
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);

    if (pWaitReturn == NULL)
    {
        IFC(E_INVALIDARG);
    }

    if (nCount > 0 && pHandles == NULL)
    {
        IFC(E_INVALIDARG);
    }

    if (nCount > (MAXIMUM_WAIT_OBJECTS - 1))
    {
        IFC(E_INVALIDARG);
    }

    IFC(pChannel->WaitForNextMessage(
        nCount,
        pHandles,
        bWaitAll,
        waitTimeout,
        pWaitReturn
        ));

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI
MilResource_CreateOrAddRefOnChannel(
    MIL_CHANNEL hChannel,
    MIL_RESOURCE_TYPE type,
    __inout_ecount(1) HMIL_RESOURCE *ph
    )
{
    HRESULT hr = S_OK;
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);
    CHECKPTRARG(ph);

    IFC(pChannel->CreateOrAddRefOnChannel(type, ph));

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI MilResource_DuplicateHandle(
    __in_ecount(1) MIL_CHANNEL hSourceChannel,
    HMIL_RESOURCE hOriginal,
    __in_ecount(1) MIL_CHANNEL hTargetChannel,
    __out_ecount(1) HMIL_RESOURCE* phDuplicate
    )
{
    HRESULT hr = S_OK;
    CMilChannel *pSourceChannel = HandleToPointer(hSourceChannel);
    CMilChannel *pTargetChannel = HandleToPointer(hTargetChannel);

    CHECKPTRARG(pSourceChannel);
    CHECKPTRARG(pTargetChannel);
    CHECKPTRARG(phDuplicate);

    IFC(pSourceChannel->DuplicateHandle(hOriginal, pTargetChannel, phDuplicate));

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI
MilResource_ReleaseOnChannel(
    MIL_CHANNEL hChannel,
    HMIL_RESOURCE h,
    __out_ecount_opt(1) BOOL *pfDeleted
    )
{
    HRESULT hr = S_OK;
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);
    CHECKPTRARG(h);

    IFC(pChannel->ReleaseOnChannel(h, pfDeleted));

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI
MilResource_GetRefCountOnChannel(
    MIL_CHANNEL hChannel,
    HMIL_RESOURCE h,
    __out_ecount_opt(1) UINT *pcRefs
    )
{
    HRESULT hr = S_OK;
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);
    CHECKPTRARG(h);

    IFC(pChannel->GetRefCount(h, pcRefs));

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI
MilChannel_SetReceiveBroadcastMessages(
    MIL_CHANNEL hChannel,
    bool fReceivesBroadcast
    )
{
    HRESULT hr = S_OK;
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);

    pChannel->SetReceiveBroadcastMessages(fReceivesBroadcast);

Cleanup:
    RRETURN(hr);
}


HRESULT WINAPI
MilChannel_GetMarshalType(
    MIL_CHANNEL hChannel,
    __out_ecount(1) MilMarshalType::Enum *pMarshalType
    )
{
    HRESULT hr = S_OK;
    const CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);
    CHECKPTRARG(pMarshalType);

    *pMarshalType = pChannel->GetMarshalType();

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI
MilResource_SendCommand(
    __in_bcount(cbSize) VOID *pvCommandData,
    UINT32 cbSize,
    bool sendInSeparateBatch,
    MIL_CHANNEL hChannel
    )
{
    HRESULT hr = S_OK;
    CMilChannel *pChannel = HandleToPointer(hChannel);

    if (!pvCommandData && cbSize > 0)
    {
        IFC(E_INVALIDARG);
    }

    CHECKPTRARG(pChannel);

    IFC(pChannel->SendCommand(pvCommandData, cbSize, sendInSeparateBatch));

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI
MilChannel_BeginCommand(
    MIL_CHANNEL hChannel,
    __in_bcount(cbCmd) VOID *pCmd,
    UINT32 cbCmd,
    UINT32 cbExtra
    )
{
    HRESULT hr = S_OK;
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);

    if (!pCmd || cbCmd < sizeof(MILCMD))
    {
        IFC(E_INVALIDARG);
    }

    IFC(pChannel->BeginCommand(pCmd, cbCmd, cbExtra));

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI
MilChannel_AppendCommandData(
    MIL_CHANNEL hChannel,
    __in_bcount(cbSize) VOID *pvData,
    UINT32 cbSize
    )
{
    HRESULT hr = S_OK;
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);

    if (!pvData && cbSize > 0)
    {
        IFC(E_INVALIDARG);
    }

    IFC(pChannel->AppendCommandData(pvData, cbSize));

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI
MilChannel_EndCommand(
    MIL_CHANNEL hChannel
    )
{
    HRESULT hr = S_OK;
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);

    IFC(pChannel->EndCommand());

Cleanup:
    RRETURN(hr);
}

/*++

Routine Description:

    MilResource_SendCommandMedia -

        This method sends a command from the managed MediaPlayer object to the
        associated slave media resource refreshing its media content.

        handle - Handle to the slave media resource
        pIMedia - Interface for accessing media content
        pBatch - Records commands to the slave media resource

--*/
HRESULT WINAPI
MilResource_SendCommandMedia(
    HMIL_RESOURCE       handle,
    IMILMedia           *pIMedia,
    MIL_CHANNEL         hChannel,
    bool                notifyUceDirect
    )
{
    HRESULT hr = S_OK;
    CMilChannel *pChannel = HandleToPointer(hChannel);
    MILCMD_MEDIAPLAYER player = { MilCmdMediaPlayer };

    CHECKPTRARG(pChannel);

    player.Handle = handle;
    player.pMedia = (UINT64)(ULONG_PTR)pIMedia;
    player.notifyUceDirect = notifyUceDirect;

    IFC(pChannel->SendCommand(&player, sizeof(player)));

Cleanup:
    RRETURN(hr);
}
/*++

Routine Description:

    MilResource_SendCommandBitmapSource -

        This method sends a command from the managed ImageData object to the
        associated slave bitmap resource refreshing its bitmap content based on
        the IWGXBitmapSource.

        handle - Handle to the slave bitmap resource
        pIBitmapSource - Interface for accessing bitmap content
        shareBitmap - Share the bitmap bits if possible.
        shareMemoryBitmap - Share the raw IWGXBitmapSource pointer.  We should
          only do this if we know its (1) trusted and (2) uncompressed.
        pBatch - Records commands to the slave bitmap resource

    The logic is as follows:

        If it's not 32-bpp, instantiate a format converter to 32-bpp and use
        this as the input "IWGXBitmapSource".

        If we're sharing and not cross machine then
            - If system memory bitmap
                    send the IWGXBitmapSource
            - else
                    create a section object
                    send section handle
        else
            send copy of pixels

        In the sharing case, the slave bitmap resource acquires its own
        reference through the bitmap source or section object and must
        release it when it's done.

--*/
MtDefine(BitmapMemory, MILRender, "BitmapMemory");
MtDefine(PaletteMemory, MILRender, "PaletteMemory");

HRESULT WINAPI
MilResource_CreateCWICWrapperBitmap(
    __in_ecount(1) IWICBitmapSource *pIBitmapSource,
    __out_ecount(1) IWICBitmapSource **ppCWICWrapperBitmap
    )
{
    HRESULT hr = S_OK;
    IWICImagingFactory *pIWICFactory = NULL;
    WICPixelFormatGUID fmtWIC;
    UINT width, height, stride;
    IWICBitmap *pIWICBitmap = NULL;
    IWGXBitmap *pCWICWrapperBitmap = NULL;

    CHECKPTRARG(pIBitmapSource);
    CHECKPTRARG(ppCWICWrapperBitmap);

    // We don't need to format convert pIBitmapSource, we're already in an acceptable format.

    // Sanity check the bitmap size
    IFC(pIBitmapSource->GetPixelFormat(&fmtWIC));
    IFC(pIBitmapSource->GetSize(&width, &height));
    IFC(HrCalcDWordAlignedScanlineStride(width, fmtWIC, OUT stride));
    if (height >= (INT_MAX / stride))
    {
        IFC(WGXERR_VALUEOVERFLOW);
    }

    if (FAILED(pIBitmapSource->QueryInterface(
        IID_IWICBitmap,
        reinterpret_cast<void **>(&pIWICBitmap)
        )))
    {
        IFC(WICCreateImagingFactory_Proxy(WINCODEC_SDK_VERSION_WPF, &pIWICFactory));
        IFC(pIWICFactory->CreateBitmapFromSource(
            pIBitmapSource,
            WICBitmapNoCache,
            &pIWICBitmap
            ));
    }

    IFC(CWICWrapperBitmap::Create(pIWICBitmap, &pCWICWrapperBitmap));

    *ppCWICWrapperBitmap = static_cast<IWICBitmapSource *>(static_cast<CWICWrapperBitmap *>(pCWICWrapperBitmap));
    pCWICWrapperBitmap = NULL;

Cleanup:
    // If succeeded, the wrapper now owns the reference to pIWICBitmap
    // If failed, clean up pIWICBitmap
    ReleaseInterface(pIWICBitmap);

    ReleaseInterface(pIWICFactory);
    ReleaseInterface(pCWICWrapperBitmap);
    
    RRETURN(hr);
}

HRESULT WINAPI
MilResource_SendCommandBitmapSource(
    __in_ecount(1) HMIL_RESOURCE handle,
    __in_ecount(1) IWICBitmapSource *pIBitmapSource,
    __in_ecount(1) MIL_CHANNEL hChannel
    )
{
    HRESULT hr = S_OK;
    IWICBitmapSource *pIBitmapSourceExtraAddRef = NULL;
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pIBitmapSource);
    CHECKPTRARG(pChannel);

    pIBitmapSourceExtraAddRef = pIBitmapSource;
    pIBitmapSourceExtraAddRef->AddRef();
    //
    // At this point, pIBitmapSourceExtraAddRef has a ref count of +1.  This reference
    // keeps it alive during transport and will be passed onto the slave bitmap
    // resource, which will release it when it's finished.
    //

    MILCMD_BITMAP_SOURCE bmp;
    bmp.Type     = MilCmdBitmapSource;
    bmp.Handle   = handle;
    bmp.pIBitmap = pIBitmapSourceExtraAddRef;

    IFC(pChannel->SendCommand(&bmp, sizeof(bmp)));

Cleanup:
    if (FAILED(hr))
    {
        ReleaseInterface(pIBitmapSourceExtraAddRef);
    }

    RRETURN(hr);
}


HRESULT WINAPI
MilChannel_SetNotificationWindow(
    __in MIL_CHANNEL hChannel,
    HWND hwnd,
    UINT message
    )
{
    HRESULT hr = S_OK;
    CMilChannel *pChannel = HandleToPointer(hChannel);

    CHECKPTRARG(pChannel);

    IFC(pChannel->SetNotificationWindow(hwnd, message));

Cleanup:
    RRETURN(hr);
}

/*++

MilCompositionEngine_EnterCompositionEngineLock

Enters the composition engine lock.

--*/

VOID WINAPI
MilCompositionEngine_EnterCompositionEngineLock()
{
    g_csCompositionEngine.Enter();
}

/*++

MilCompositionEngine_ExitCompositionEngineLock

Enters the composition engine lock.

--*/

VOID WINAPI
MilCompositionEngine_ExitCompositionEngineLock()
{
    g_csCompositionEngine.Leave();
}

HRESULT WINAPI MilPlayer_Create(
    __deref_out_ecount(1) HMIL_PLAYER* phPlayer
    )
{
    HRESULT hr = S_OK;

    CHECKPTRARG(phPlayer);

#if PRERELEASE
    CMilRecordPacketPlayer *pPlayer = NULL;

    IFC(CMilRecordPacketPlayer::CreateRecordPacketPlayer(&pPlayer));

    *phPlayer = reinterpret_cast<HMIL_PLAYER>(pPlayer);
#else
    IFC(E_NOTIMPL);
#endif

Cleanup:
    RRETURN(hr);
}

HRESULT WINAPI MilPlayer_Process(
    __in_ecount(1) HMIL_PLAYER hPlayer,
    __in_xcount(sizeof(MIL_REC_PACKET_HEADER)) const BYTE* pbHeader,
    __in_xcount_opt(sizeof(UCE_RDP_HEADER)) const BYTE* pbRdpHeader,
    __in_bcount_opt(cbData) const BYTE* pbData,
    UINT cbData
    )
{
    HRESULT hr = S_OK;

    CHECKPTRARG(hPlayer);
    CHECKPTRARG(pbHeader);

#if PRERELEASE
    CMilRecordPacketPlayer *pPlayer =
        reinterpret_cast<CMilRecordPacketPlayer *>(hPlayer);

    IFC(pPlayer->ProcessFilePacketContents(
        reinterpret_cast<const MIL_REC_PACKET_HEADER*>(pbHeader),
        reinterpret_cast<const UCE_RDP_HEADER*>(pbRdpHeader),
        pbData,
        cbData
        ));
#else
    IFC(E_NOTIMPL);
#endif

Cleanup:
    RRETURN(hr);
}




//+-----------------------------------------------------------------------
//
//  Member: MilCompositionEngine_GetComposedEventId
//
//  Synopsis:  Gets the counter used to create the name of the compsed event
//
//------------------------------------------------------------------------
HRESULT WINAPI MilCompositionEngine_GetComposedEventId(
    __out_ecount(1) UINT *pcEventId
    )
{
    return GetCompositionEngineComposedEventId(pcEventId);
}

// Ignore deprecation of D3DMATRIX on method prototypes defined
// in windows/published, where CMILMatrix isn't defined.
#pragma warning (push)
#pragma warning (disable : 4995)

//+------------------------------------------------------------------------
//
//  Function:
//      MilUtility_GetTileBrushMapping
//
//  Synopsis:
//      MILCore export that exposes CTileBrushUtils::CalculateTileBrushMapping
//      to external callers (e.g., managed code).
//
//-------------------------------------------------------------------------
VOID WINAPI
MilUtility_GetTileBrushMapping(
    __in_ecount_opt(1) const D3DMATRIX *pTransform,
        // Transform that is applied to the Viewport
    __in_ecount_opt(1) const D3DMATRIX *pRelativeTransform,
        // RelativeTransform that is applied to the Viewport
    MilStretch::Enum stretch,
        // Stretch mode to use in Viewbox->Viewport mapping
    MilHorizontalAlignment::Enum alignmentX,
        // X Alignment to use in Viewbox->Viewport mapping
    MilVerticalAlignment::Enum alignmentY,
        // Y Alignment to use in Viewbox->Viewport mapping
    MilBrushMappingMode::Enum viewportUnits,
        // Viewport mapping mode (relative or absolute)
    MilBrushMappingMode::Enum viewboxUnits,
        // Viewbox mapping mode (relative or absolute)
    __in_ecount(1) const MilPointAndSizeD *pShapeFillBounds,
        // Shape fill-bounds pViewport is relative to.
        // Only needed when viewportUnits == RelativeToBoundingBox.
    __in_ecount(1) const MilPointAndSizeD *pContentBounds,
        // Content bounds pViewbox is relative to.
        // Only needed when viewboxUnits == RelativeToBoundingBox.
    __inout_ecount(1) MilPointAndSizeD *pViewport,
        // IN: User-specified Viewport to map Viewbox to
        // OUT: Viewport in absolute units
    __inout_ecount(1) MilPointAndSizeD *pViewbox,
        // IN: User-specified Viewbox to map to Viewport
        // OUT: Viewbox in absolute units
    __out_ecount(1) D3DMATRIX *pContentToWorld,
        // Combined Content->World transform
    __out_ecount(1) BOOL *pfBrushIsEmpty
        // Whether or not this brush renders nothing because of an empty viewport/viewbox
    )
{
    CTileBrushUtils::CalculateTileBrushMapping(
        reinterpret_cast<const CMILMatrix*>(pTransform),
        reinterpret_cast<const CMILMatrix*>(pRelativeTransform),
        stretch,
        alignmentX,
        alignmentY,
        viewportUnits,
        viewboxUnits,
        reinterpret_cast<const MilPointAndSizeD*>(pShapeFillBounds),
        reinterpret_cast<const MilPointAndSizeD*>(pContentBounds),
        1.0f,   // Content scale is only used for ImageBrush's, which do not call this method
        1.0f,   // Content scale is only used for ImageBrush's, which do not call this method
        reinterpret_cast<MilPointAndSizeD*>(pViewport),
        reinterpret_cast<MilPointAndSizeD*>(pViewbox),
        NULL,   // Caller doesn't need the Content->Viewport transform seperated from the final transform
        NULL,   // Caller doesn't need the Viewport->World transform seperated from the final transform
        reinterpret_cast<CMILMatrix*>(pContentToWorld),
        pfBrushIsEmpty
        );
}

#pragma warning (pop)

/*++

Routine Description:

    Enable instrumentation for rendering performance measurement.
    See comments for "MilPerfInstrumentationFlags" in partition.h.
--*/

UINT g_uMilPerfInstrumentationFlags = 0;
VOID WINAPI SetMilPerfInstrumentationFlags(UINT flags)
{
    g_uMilPerfInstrumentationFlags = flags;
}

//+-----------------------------------------------------------------------------
//
//    Function:
//        MilGlyphRun_GetGlyphOutline
//
//    Synopsis:
//        Used to communicate with DWrite directly to get a glyph's serialized geometric
//        representation and return it to managed code.
//
//------------------------------------------------------------------------------

HRESULT WINAPI
MilGlyphRun_GetGlyphOutline(
    __in IDWriteFontFace* pFontFace,
    USHORT glyphIndex, 
    bool sideways, 
    double renderingEmSize,
    __deref_out_ecount(*pSize) byte **ppFigureDataBytes,
    __out UINT *pSize,
    __out MilFillMode::Enum *pFillRule
    )
{
    HRESULT hr = S_OK;
    Assert(pFontFace);

    CGlyphRunGeometrySink *pGeometrySink = NULL;
    MilPathGeometry *pFigureData = NULL;
   
    IFC(CGlyphRunGeometrySink::Create(&pGeometrySink));
    
    IFC(pFontFace->GetGlyphRunOutline(
        static_cast<float>(renderingEmSize),
        &glyphIndex,
        NULL,
        NULL,
        1,
        sideways,
        false, // This is handled by GlyphRun::BuildGeometry in managed code.
        pGeometrySink
        ));

    // We now own the reference to pFigureData, the allocated memory block containing serialized 
    // geometry data structs.
    IFC(pGeometrySink->ProduceGeometryData(&pFigureData, pSize, pFillRule));

    *ppFigureDataBytes = reinterpret_cast<byte*>(pFigureData);
    pFigureData = NULL;

Cleanup:
    delete pFigureData;
    ReleaseInterface(pGeometrySink);
    ReleaseInterface(pFontFace); // This was AddRef-ed when passed in from managed code.
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Function:
//        MilGlyphRun_ReleasePathGeometryData
//
//    Synopsis:
//        Frees the glyph data returned to managed code by MilGlyphRun_GetGlyphOutline.
//
//------------------------------------------------------------------------------

HRESULT WINAPI
MilGlyphRun_ReleasePathGeometryData(
    __in byte* pPathGeometryData
    )
{
    MilPathGeometry *pFigureData = reinterpret_cast<MilPathGeometry*>(pPathGeometryData);
    delete(pFigureData);
    RRETURN(S_OK);
}

//+-----------------------------------------------------------------------------
//
//    Function:
//        GetNextPerfElementId
//
//    Synopsis:
//        Gets an ID that will be unique across appdomains for taging elements so they
//        can be identified by tools like VisualProfiler.
//
//------------------------------------------------------------------------------

// Definition for _InterlockedCompareExchange64 from intrin.h
extern "C" {
#if defined(_M_AMD64)
__int64 _InterlockedCompareExchange64_np(__int64 volatile *, __int64, __int64);
#define _InterlockedCompareExchange64 _InterlockedCompareExchange64_np
#else
__int64 _InterlockedCompareExchange64(__int64 volatile *, __int64, __int64);
#endif
}

LONGLONG WINAPI
GetNextPerfElementId()
{
    static volatile __int64 id = 0;
    // _InterlockedIncrement64 is not available on Windows XP so use InterlockedCompareExchange to emulate it.
    __int64 old;
    do
    {
        old = id;
    } while(_InterlockedCompareExchange64(&id, old + 1, old) != old);
    return old + 1;
}



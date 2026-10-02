// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------------
//

//
// Module Name:
//
//    HWNDtarget.h
//
//---------------------------------------------------------------------------------

MtExtern(CSlaveHWndRenderTarget);

class CDeviceDependentResource;
class CComposeOnceContent;

//------------------------------------------------------------------
// CSlaveHwndRenderTarget
//------------------------------------------------------------------

class CSlaveHWndRenderTarget : 
    public CRenderTarget, 
    public DpiProvider
{
    friend class CResourceFactory;

private:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CSlaveHWndRenderTarget));

    CSlaveHWndRenderTarget(CComposition *pComposition);

    virtual ~CSlaveHWndRenderTarget();

    HRESULT HandleWindowErrors(HRESULT hr);

    void SetScreenAccessDenied();

public:
    DECLARE_COM_BASE

    inline STDMETHOD(HrFindInterface)(__in_ecount(1) REFIID riid, __deref_out void **ppvObject) override
    {
        if (riid == __uuidof(IDpiProvider))
        {
            *ppvObject = static_cast<DpiProvider*>(this);
            return S_OK;
        }

        return E_NOINTERFACE;
    }

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_HWNDRENDERTARGET ||
            CRenderTarget::IsOfType(type);
    }
    
    override HRESULT GetBaseRenderTargetInternal(
        __deref_out_opt IRenderTargetInternal **ppIRT
        );
    
    virtual HRESULT Render(
        __out_ecount(1) bool *pfNeedsPresent
        );
    
    virtual HRESULT Present();

    override HRESULT NotifyDisplaySetChange(bool invalid, int oldDisplayCount, int displayCount);
    override BOOL PostDisplayAvailabilityMessage(int displayCount);
    override HRESULT UpdateRenderTargetFlags();
    
    HRESULT WaitForVBlank();

    HRESULT GetNumQueuedPresents(
        __out_ecount(1) UINT *puNumQueuedPresents
        );

    void AdvanceFrame(UINT uFrameNumber);
    
    void GetIntersectionWithDisplay(
        __in UINT iDisplay,
        __out_ecount(1) CMILSurfaceRect &rcIntersection
        );

    void InvalidateWindow()
    {
        if (m_hWnd != NULL && IsWindow(m_hWnd))
        {
            InvalidateRect(m_hWnd, NULL, FALSE);
        }
    }

    // ------------------------------------------------------------------------
    //
    //   Command handlers
    //
    // ------------------------------------------------------------------------
    HRESULT ProcessCreate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_HWNDTARGET_CREATE* pCmd
        );

    HRESULT ProcessSuppressLayered(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_HWNDTARGET_SUPPRESSLAYERED* pCmd
        );

    HRESULT ProcessDpiChanged(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_HWNDTARGET_DPICHANGED* pCmd
    );

    override virtual HRESULT ProcessSetClearColor(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_TARGET_SETCLEARCOLOR* pCmd
        );

    override virtual HRESULT ProcessSetFlags(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_TARGET_SETFLAGS* pCmd
        );

    override virtual HRESULT ProcessInvalidate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_TARGET_INVALIDATE* pCmd,
        __in_bcount_opt(cbPayload) LPCVOID pPayload,
        UINT cbPayload
        );

     override virtual HRESULT ProcessUpdateWindowSettings(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_TARGET_UPDATEWINDOWSETTINGS* pCmd
        );

private:
    HRESULT SetNewUCETargetFlags(
        MilRTInitialization::Flags NewUCETargetFlags
        );

    HRESULT UpdateRenderTargetFlags(
        MilRTInitialization::Flags UCETargetFlags
        );

    HRESULT EnsureRenderTargetInternal();

    HRESULT UpdateWindowSettingsInternal();

    HRESULT InvalidateInternal(
        __in_ecount_opt(1) const MilRectF *pRect = NULL
        );

    HRESULT SendInvalidRegions();

    void ReleaseResources();
    
    HRESULT CalculateWindowRect();

private:
    IMILRenderTargetHWND *m_pRenderTarget;
    HWND m_hWnd;
    CMilRectF m_rcWindow;
    MilWindowProperties::Flags m_WindowProperties;
    MilWindowLayerType::Enum m_WindowLayerType;
    MilTransparency::Flags m_WindowTransparency;

    // UCE Target behavior/property flags requested by client including UCE
    // specific flags.  Whenever this value is changed UpdateRenderTargetFlags
    // should be called.
    MilRTInitialization::Flags m_UCETargetFlags;

    // Core Render Target behavior/property flags adjusted by core rendering to
    // account for OS compatibility and capabilities.  If this value changes
    // then m_pRenderTarget needs to be recreated.  ReleaseResources should be
    // called to ensure any m_pRenderTarget dependent resources are also
    // released.
    MilRTInitialization::Flags m_RenderTargetFlags;

    MilColorF m_clearColor;
    MilColorF m_colorKey;
    FLOAT m_constantAlpha;

    // It is important that this start at zero to allow an initial
    // UpdateWindowSettings(enable) command to enable the render target
    // without a preceeding UpdateWindowSettings(disable) command.
    UINT m_disableCookie;

    // Window Message registered with name L"DisplayDevicesAvailabilityChanged"
    // This is also registered in the managed HwndTarget.cs, which is the intended
    // recipient of this message.
    const UINT m_DisplayDevicesAvailabilityChangedWindowMessage;

    short m_LastKnownDisplayDevicesAvailabilityChangedWParam;

    // Flags
    bool m_fNeedsFullRender                 : 1; // The whole back buffer needs to be re-rendered
    bool m_fIsZombie                        : 1; // The render target has been disabled.
    bool m_fNeedsPresent                    : 1; // There is unpresented rendered content
    bool m_fRenderingEnabled                : 1; // Rendering is enabled. This flag is controlled with the UpdateWindowSetting command packet.
    bool m_fNoScreenAccess                  : 1; // This bool indicates that screen access has been denied and for now we have no screen access.
    bool m_fChild                           : 1; // Window is a child window.
    bool m_fTransparencyDirty               : 1; // Transparency settings of the window have been updated
    bool m_fSoftwareFallback                : 1; // If set, the hwnd target is in software fallback mode.

	bool m_fHasInvalidRegions               : 1; // There are invalid regions
	bool m_fFullRegionInvalid               : 1; // The entire region is invalid

	DynArray<MilRectF> m_invalidRegions;
};



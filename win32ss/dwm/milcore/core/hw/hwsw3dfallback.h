// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CHw3DSoftwareSurface
//
//      This object will take 3D only drawing functions and implements them
//      using a D3D RGB Rast software device.  This is then drawn back onto a
//      Software Rendertarget
//

MtExtern(CHw3DSoftwareSurface);

class CHw3DSoftwareSurface :
    public CHwSurfaceRenderTarget,
    public CMILCOMBase
{
public:

    static HRESULT Create(
        MilPixelFormat::Enum fmtTarget,
        DisplayId associatedDisplay,
        UINT uWidth,
        UINT uHeight,
        __deref_out_ecount(1) CHw3DSoftwareSurface ** const pp3DFallbackRT
        );

private:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHw3DSoftwareSurface));
    CHw3DSoftwareSurface(
        __inout_ecount(1) CD3DDeviceLevel1 *pDevice,
        MilPixelFormat::Enum fmtTarget,
        D3DFORMAT d3dfmtTarget,
        DisplayId associatedDisplay,
        bool fComposeWithCopy
        );

    ~CHw3DSoftwareSurface();

public:

    //
    // CMILCOMBase overrides
    //

    DECLARE_COM_BASE;
    STDMETHOD(HrFindInterface)(
        __in_ecount(1) REFIID riid,
        __deref_out void **ppv
        );

    //
    // 3D RenderTarget Drawing Functions
    //

    HRESULT BeginSw3D(
        __in_bcount(cbDbgAnalysisTargetBufferSize) void *pvTargetPixels,
        DBG_ANALYSIS_PARAM_COMMA(UINT cbDbgAnalysisTargetBufferSize)
        UINT cbTargetStride,
        __in_ecount(1) CMILSurfaceRect const &rcBounds,
        bool fUseZBuffer,
        __in_ecount_opt(1) FLOAT *prZ   // Optional to support stepped rendering
        );

    HRESULT EndSw3D(
        __inout_ecount(1) CSpanSink *pSwSink,
        __inout_bcount(cbDbgAnalysisTargetBufferSize) void *pvTargetPixels,
        DBG_ANALYSIS_PARAM_COMMA(UINT cbDbgAnalysisTargetBufferSize)
        UINT cbTargetStride,
        __inout_ecount(1) CSoftwareRasterizer *pSwRast
        );

    STDMETHODIMP DrawMesh3D(
        __inout_ecount(1) CContextState* pContextState,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __inout_ecount(1) CMILMesh3D *pMesh3D,
        __inout_ecount_opt(1) CMILShader *pShader,
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        );

    //
    // Set Size on Surface
    //

    HRESULT Resize(
        UINT uWidth,
        UINT uHeight
        );

    void CleanupFreedResources();

protected:

    //
    // CHwSurfaceRenderTarget methods
    //

    override bool IsValid() const;

private:

    //
    // Ensure the SW Surface Exists
    //

    HRESULT EnsureSurface();

    //
    // Composite the current contents of the buffer into the Sw Render Target
    //

    HRESULT CompositeWithSwRenderTarget(
        __in_ecount(1) const CMILSurfaceRect &rc3DBounds,
        __inout_ecount(1) CSpanSink *pSwSink,
        __inout_bcount(cbDbgAnalysisTargetBufferSize) void *pvTargetPixels,
        DBG_ANALYSIS_PARAM_COMMA(UINT cbDbgAnalysisTargetBufferSize)
        UINT cbTargetStride,
        __inout_ecount(1) CSoftwareRasterizer *pSwRast
        );

private:

    bool m_fEnableRendering;
    bool m_fSurfaceDirty;
    bool const m_fComposeWithCopy;

    UINT m_uiNewTargetWidth;
    UINT m_uiNewTargetHeight;

    CClientMemoryBitmap *m_pWrapperBitmap;
};



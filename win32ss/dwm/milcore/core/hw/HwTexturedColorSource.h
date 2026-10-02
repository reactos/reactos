// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CHwTexturedColorSource declaration
//

MtExtern(CHwTexturedColorSource);

class CHwBoxColorSource;

//+----------------------------------------------------------------------------
//
//  Class:     CHwTexturedColorSource
//
//  Synopsis:  Base class for texture based color sources
//
//-----------------------------------------------------------------------------

class CHwTexturedColorSource : public CHwColorSource
{
public:

    //
    // CHwColorSource methods
    //

    override TypeFlags GetSourceType(
        ) const;    

    virtual override HRESULT SendVertexMapping(
        __inout_ecount_opt(1) CHwVertexBuffer::Builder *pVertexBuilder,
        MilVertexFormatAttribute mvfaLocation
        );

    __out_ecount(1) const MILMatrix3x2 &GetXSpaceToTextureUV() const
    {
    #if DBG
        Assert(m_fDbgValidXSpaceToTextureUV);
    #endif
    
        return m_matXSpaceToTextureUV;
    }

    __out_ecount(1) const MILMatrix3x2 & GetDevicePointToTextureUV() const
    {
    #if DBG
        Assert(   m_dbgXSpaceDefinition == XSpaceIsSampleSpace
               || m_dbgXSpaceDefinition == XSpaceIsIrrelevant
              );
    #endif
        return GetXSpaceToTextureUV();
    }

    __out_ecount(1) const MILMatrix3x2 & GetBrushCoordToTextureUV() const
    {
    #if DBG
        Assert(   m_dbgXSpaceDefinition == XSpaceIsWorldSpace
               || m_dbgXSpaceDefinition == XSpaceIsIrrelevant
              );
    #endif
        return GetXSpaceToTextureUV();
    }

    virtual override HRESULT SendDeviceStates(
        DWORD dwStage,
        DWORD dwSampler
        );

    HRESULT SendDeviceStates(
        DWORD dwStage,
        DWORD dwSampler,
        DWORD dwTexCoordIndex
        );

    override void ResetForPipelineReuse()
    {
        m_hTextureTransform = MILSP_INVALID_HANDLE;
        m_fUseHwTransform = false;
    }

    override HRESULT SendShaderData(
        __inout_ecount(1) CHwPipelineShader *pHwShader
        );

    void SetTextureTransformHandle(
        MILSPHandle hTransform
        )
    {
        Assert(m_hTextureTransform == MILSP_INVALID_HANDLE);
        m_hTextureTransform = hTransform;
    }
    
    // This is workaround to force SendDeviceStates() using
    // border color instead of default clamping.
    void ForceBorder()
    {
        m_taU = m_taV = D3DTADDRESS_BORDER;
    }

    // Set a clip parallelogram that will be implemented
    // using multitexturing.
    HRESULT SetMaskClipWorldSpace(
        __in_ecount_opt(1) const CParallelogram *
        );

    HRESULT GetMaskColorSource(
        __deref_out_ecount_opt(1) CHwBoxColorSource ** const ppColorSource
        ) const;
    
    static HRESULT MVFAttrToCoordIndex(
        MilVertexFormatAttribute mvfaLocation,
        __out_ecount(1) DWORD * const pwdCoordIndex
        );

    void SetWrapModes(
        D3DTEXTUREADDRESS taU,
        D3DTEXTUREADDRESS taV
        );

    static void ConvertWrapModeToTextureAddressModes(
        MilBitmapWrapMode::Enum wrapMode,
        __out_ecount(1) D3DTEXTUREADDRESS *ptaU,
        __out_ecount(1) D3DTEXTUREADDRESS *ptaV
        );

private:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwTexturedColorSource));

protected:

    CHwTexturedColorSource(__in_ecount(1) CD3DDeviceLevel1 *pDevice);

    virtual ~CHwTexturedColorSource();

    void SetFilterAndWrapModes(
        MilBitmapInterpolationMode::Enum interpolationMode,
        D3DTEXTUREADDRESS taU,
        D3DTEXTUREADDRESS taV
        );

    void SetFilterMode(
        MilBitmapInterpolationMode::Enum interpolationMode
        );

    D3DTEXTUREADDRESS GetTAU() const { return m_taU; }
    D3DTEXTUREADDRESS GetTAV() const { return m_taV; }

    HRESULT CalcTextureTransform(
        __in_ecount(1) const BitmapToXSpaceTransform *pBitmapToXSpaceTransform,
        __range(>=, 1) UINT uTextureWidth,
        __range(>=, 1) UINT uTextureHeight
        );

    //   Potentially obsolete method
    //  With change 175689 shader handles are always reset when added to the
    //  pipeline.  Resetting when setting context should no longer be required.
    //  Leaving code paths as they are today to avoid churn.
    void ResetShaderTextureTransformHandle()
    {
        m_hTextureTransform = MILSP_INVALID_HANDLE;
    }

    bool UsingTrilinearFiltering() const
    {
        return m_pFilterMode == &CD3DRenderState::sc_fmTriLinear;
    }

public:

#if DBG
    __out_ecount(1) CD3DDeviceLevel1 *DbgGetDevice() const
    {
        return m_pDevice;
    }
#endif

protected:

#if DBG
    void DbgMarkXSpaceToTextureUVAsSet(
        XSpaceDefinition xSpaceDefinition
        )
    {
        m_fDbgValidXSpaceToTextureUV = true;
        m_dbgXSpaceDefinition = xSpaceDefinition;
    }
#endif


protected:

    CD3DDeviceLevel1 * const m_pDevice;



private:

    const FilterMode *m_pFilterMode;    // Filter settings for render state

    D3DTEXTUREADDRESS m_taU, m_taV;     // Current texture addressing/wrapping modes

    bool m_fUseHwTransform;             // Request tex coordinate transform
                                        // from hardware device

    MILSPHandle m_hTextureTransform;

protected:

    // Vertex mapping from x space space to texture coordinates
    MILMatrix3x2 m_matXSpaceToTextureUV;

    // Mapping from device space to a space where the clip parallelogram is the unit square.    
    MILMatrix3x2 m_matXSpaceToSourceClip;

    // Should we add a mask texture corresponding to the source clip parallelogram.
    bool m_fMaskWithSourceClip;

#if DBG
private:

    XSpaceDefinition m_dbgXSpaceDefinition;
    bool m_fDbgValidXSpaceToTextureUV;
#endif
};




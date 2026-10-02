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
//      Contains declaration for CHwPipeline class
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
// Hardware Accelerated Rendering Procedure
// ----------------------------------------
//
//  1. Validate support for basic operation via hw accel or sw fallback
//  2. Check for hw accel support (some of operation handled by hw)
//  3. Split code path to hw accel or sw fallback code
//     (Same steps apply to both assuming sw fallback does not directly
//      read/write to target.)
//  4. Determine steps involving intense sw processing (partial sw fallback
//      only)
//  5. In parallel (or serialized for single thread)
//     a. Handle sw processing
//     b. Determine hw settings
//  6. Realize hw resources (not necessarily ordered)
//     a. Allocate/populate textures
//     b. Allocate/populate VBs
//     c. Realize complex masking (clipping) resources
//  7. Realize hw settings/instructions (not necessarily ordered)
//     a. Set target (if not already set with clip setup)
//     b. Set clipping
//     c. Set rendering states (including texture/sampler states)
//     d. Set vertex format
//     e. Set pixel shaders
//     f. Set textures
//  8. Execute hw operations
//  9. Repeat some or all of steps 4 to 8 as needed for multi-pass rendering
//     (Multi-step rendering with intermediate targets should probably recurse
//      these steps during the resource realization step.)
//
//
// Logical Components Involved in Hardware Rendering Procedure
// -----------------------------------------------------------
//
//   Pipeline              CHwPipeline
//   Pipeline Builder      CHwPipeline::Builder
//   Primary Color Source  IHwPrimaryColorSource (akin to a Brush)
//   Color Source          CHwColorSource
//   Vertex Buffer         CHwVertexBuffer
//   Vertex Builder        CHwVertexBuffer::Builder (IGeometrySink)
//   Fill Tessellator      IGeometryGenerator
//   Effects Processor     CHwPipeline::Builder::ProcessEffectList
//
//
// Open Work Items
// ----------------
//  - Design more interface(s) for communicating vertex mapping
//  - Implement more complete/general vertex builders
//  - Implement general FinalizeBlendOperations
//
//-----------------------------------------------------------------------------


interface IHwPrimaryColorSource;


//+-----------------------------------------------------------------------------
//
//  Enumeration:
//      HwBlendOp
//
//  Synopsis:
//      Basic blending operation HW can perform
//
//  WARNING:
//      When changing this array, update the sc_PipeOpProperties and
//      sc_tsoFromPipeOp tables.
//
//------------------------------------------------------------------------------

enum HwBlendOp
{
    HBO_Nop = -1,
    HBO_SelectSource = 0,
    HBO_Multiply,
    HBO_SelectSourceColorIgnoreAlpha,
    HBO_MultiplyColorIgnoreAlpha,
    HBO_BumpMap,

    HBO_MultiplyByAlpha,
    
    // NOTE MultiplyAlphaOnly multiplies the alpha channels of both
    // sources but keeps the color channel of one.  This generally
    // produces a non-premultiplied color value and should be used
    // carefully.
    HBO_MultiplyAlphaOnly,

    HBO_Total
};

//+-----------------------------------------------------------------------------
//
//  Enumeration:
//      HwBlendArg
//
//  Synopsis:
//      Sources to HW blending operations
//
//------------------------------------------------------------------------------

enum HwBlendArg
{
    HBA_None = 0,
    HBA_Current,
    HBA_Diffuse,
    HBA_Specular,
    HBA_Texture,

    HBA_Total
};

//+-----------------------------------------------------------------------------
//
//  Structure:
//      HwBlendParams
//
//  Synopsis:
//      Record of all parameters to a HW blending operation
//
//------------------------------------------------------------------------------

struct HwBlendParams
{
    HwBlendArg  hbaSrc1;
    HwBlendArg  hbaSrc2;
};


//+-----------------------------------------------------------------------------
//
//  Structure:
//      HwPipelineItem
//
//  Synopsis:
//      Contains information about a particular pipeline stage including
//      operation and arguments
//
//------------------------------------------------------------------------------

struct HwPipelineItem
{
    DWORD dwStage;                                      // Blending stage for      
                                                        //  easy reference         
    DWORD dwSampler;                                    // Sampler number when a   
                                                        //  texture is an argument 
    CHwColorSource *pHwColorSource;                     // Color source for this      
                                                        //  stage                     
    union
    {
        //
        // Fixed Function specific data
        //
        struct 
        {
            HwBlendOp eBlendOp;                         // Blending operation
            HwBlendParams oBlendParams;                 // Blending arguments
            MilVertexFormatAttribute mvfaSourceLocation;// Vertex field used by
        };

        //
        // Shader specific data
        //
        struct
        {
            const ShaderFunction *pFragment;                  // Shader Fragment
                                                              // we'll use.
            MilVertexFormatAttribute mvfaTextureCoordinates;  // A Texture coordinate
                                                              // transform we need
                                                              // calculated by the
                                                              // VertexBuffer::Builder                                                              
        };
    };
};


namespace HwPipeline
{
    enum Type
    {
        FixedFunction,
        Shader,
        Total
    };
};

#define INVALID_PIPELINE_SAMPLER -1
#define INVALID_PIPELINE_STAGE -1
#define INVALID_PIPELINE_ITEM -1

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwPipeline
//
//  Synopsis:
//      Abstraction for hardware device pipeline covering most states and
//      optionally geometry involved in rendering.
//
//      The pipeline texture blending stages/pixel shaders are stored in an
//      array of pipeline items.  Each item can have at most one color source
//      and one operation.
//
//      The functionality for single-pass rendering is to initialize the
//      pipeline giving it a geometry source and rasterization info (the
//      rasterization info includes the color source for the geometry, the blend
//      mode and any effects) and then execute.
//
//      Execute will push the state and geometry to the card.
//
//      In some cases, the caller may want to fidget some state after calling
//      Execute and then call Execute multiple times in a row. ClearType is a
//      likely candidate for this.  The ReInitialize call changes the
//      rasterization info for the pipeline and allows another call to Execute.
//
//      If the entire geometry of the pipeline fit into a single HwVertexBuffer
//      ReInitialize and Execute will re-use that HwVertexBuffer without calling
//      back into the geometry source. If the geometry required multiple draw
//      calls in the 1st place subsequent re-executes will do everything over.
//
//      Summary:
//
//           InitializeForRendering
//           Execute
//           [
//              ReInitialize
//              Execute
//           ]*                    (this part repeated 0 or more times)
//           ReleaseExpensiveResources
//
//      Another possible use is to "cache" a commonly used pipeline and with its
//      resources such that only SendDeviceStates and ExecuteRendering would be
//      needed.  However this is not currently implemented for the case where
//      the geometry required multiple draws (i.e. flushes.)
//
//  Notes:
//      It is required that ReleaseExpensiveResources be called after the caller
//      is done with the pipeline or before another call to
//      InitializeForRendering is called.
//
//      If the geometry source returns WGXHR_EMPTYFILL the state won't get
//      pushed.  This is an optimization but note (further) that some users of
//      the pipeline use no-op geometry sources that return S_OK to send just
//      the state.
//
//  Responsibilities:
//      - Coordinating rendering operations/stages
//        - Including step consolidations and use of SW
//      - Selecting some device states / shaders
//        - Texture blending states or pixel shader
//        - Final blend
//      - Determining vertex input requirements
//         - Select appropriate VB builder class
//
//  Requests from others:
//      - Color sources
//        - Solid colors
//        - Per-Vertex colors
//        - Textures and respective transforms and wrapping
//
//  Not responsible for:
//      - Clipping
//      - Setting target(s)
//      - Tessellation
//      - Vertex building (does select VB builder though)
//      - Device state management
//      - Texture realization
//
//  Inputs required:
//      - Device to set up
//      - Compositing mode (from context)
//      - Primary color source
//      - Effects list
//      - Per-Primitive AA (from context)
//
//------------------------------------------------------------------------------

class CHwPipeline
{

public:
    friend class CHwPipelineBuilder;

    CHwPipeline(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice
        );
    virtual ~CHwPipeline();

    virtual HRESULT Execute(
        );
    
    void ReleaseExpensiveResources(
        );

    // This is public for the use of the vertex buffer builder to send
    // the device state when it flushes.
    HRESULT RealizeColorSourcesAndSendState(
        __in_ecount_opt(1) const CHwVertexBuffer *pVB
        );

    virtual HRESULT InitializeForRendering(
        MilCompositingMode::Enum CompositingMode,
        __in_ecount(1) IGeometryGenerator *pGeometryGenerator,
        __inout_ecount(1) IHwPrimaryColorSource *pIPCS,
        __in_ecount_opt(1) const IMILEffectList *pIEffects,
        __in_ecount(1) const CHwBrushContext    *pEffectContext,
        __in_ecount_opt(1) const CMILSurfaceRect *prcOutsideBounds = NULL,
        bool fNeedInside = true
        ) PURE;

protected:


    void SetupCompositionMode(
        MilCompositingMode::Enum eCompositingMode
        );

    HRESULT RealizeColorSources();

    virtual HRESULT SendDeviceStates(
        __in_ecount_opt(1) const CHwVertexBuffer *pVB
        ) PURE;

    HRESULT AddPipelineItem(
        __deref_out_ecount(1) HwPipelineItem ** const ppItem
        )
    {
        return m_rgItem.AddMultiple(1, ppItem);
    }

protected:
    CD3DDeviceLevel1 * const m_pDevice;

    AlphaBlendMode const *m_pABM;

    DWORD m_dwFirstUnusedStage;

    CHwVertexBuffer::Builder *m_pVBB;

    IGeometryGenerator *m_pGG;

    CHwVertexBuffer *m_pVB;

    enum {
        kGeneralScratchSpace =
            sizeof(CHwConstantAlphaScalableColorSource) +
            sizeof(CHwTexturedColorSource),

        kScratchAllocationSpace = kMaxVertexBuilderSize + kGeneralScratchSpace
    };

    CDispensableBuffer<kScratchAllocationSpace, 3> m_dbScratch;

    DynArrayIA<HwPipelineItem,6> m_rgItem;

    static const int s_Invalid_Pipeline_Sampler = -1;
    static const int s_Invalid_Pipeline_Stage   = -1;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwFFPipeline
//
//  Synopsis:
//      Pipeline that uses only fixed function calls.
//
//------------------------------------------------------------------------------

class CHwFFPipeline :
    public CHwPipeline
{
public:
    friend class CHwFFPipelineBuilder;
    
    CHwFFPipeline(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice
        )
        : CHwPipeline(pDevice)
    {};

    override HRESULT InitializeForRendering(
        MilCompositingMode::Enum CompositingMode,
        __inout_ecount(1) IGeometryGenerator *pGeometryGenerator,
        __inout_ecount(1) IHwPrimaryColorSource *pIPCS,
        __in_ecount_opt(1) const IMILEffectList *pIEffects,
        __in_ecount(1) const CHwBrushContext    *pEffectContext,
        __in_ecount_opt(1) const CMILSurfaceRect *prcOutsideBounds = NULL,
        bool fNeedInside = true
        );

private:
    HRESULT SendRenderStates();
    HRESULT SendFFStageState(
        __in_ecount(1) HwPipelineItem &oFFItem
        );
    
    override HRESULT SendDeviceStates(
        __in_ecount_opt(1) const CHwVertexBuffer *pVB
        );
};


//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwShaderPipeline
//
//  Synopsis:
//      Pipeline that uses vertex and pixel shaders.
//
//------------------------------------------------------------------------------

class CHwShaderPipeline :
    public CHwPipeline
{
public:
    friend class CHwShaderPipelineBuilder;
    
    CHwShaderPipeline(
        bool f2D,
        __in_ecount(1) CD3DDeviceLevel1 *pDevice
        )
    : CHwPipeline(pDevice)
    {
        m_f2D = f2D;
        m_pPipelineShader = NULL;
    };

    ~CHwShaderPipeline();

    override HRESULT InitializeForRendering(
        MilCompositingMode::Enum CompositingMode,
        __inout_ecount(1) IGeometryGenerator *pGeometryGenerator,
        __inout_ecount(1) IHwPrimaryColorSource *pIPCS,
        __in_ecount_opt(1) const IMILEffectList *pIEffects,
        __in_ecount(1) const CHwBrushContext    *pEffectContext,
        __in_ecount_opt(1) const CMILSurfaceRect *prcOutsideBounds,
        bool fNeedInside
        );

    HRESULT ReInitialize(
        MilCompositingMode::Enum CompositingMode,
        __inout_ecount_opt(1) IHwPrimaryColorSource *pIPrimaryColorSource,
        __in_ecount_opt(1) const IMILEffectList *pIEffects,
        __in_ecount(1) const CHwBrushContext    *pEffectContext,
        __in_ecount_opt(1) const CMILSurfaceRect *prcOutsideBounds,
        bool fNeedInside
        );

    static bool CanRunWithDevice(
        __in_ecount(1) const CD3DDeviceLevel1 *pDevice
        );

    override HRESULT Execute();

private:
    CHwPipelineShader *m_pPipelineShader;
    bool m_f2D;

    override HRESULT SendDeviceStates(
        __in_ecount_opt(1) const CHwVertexBuffer *pVB
        );

};



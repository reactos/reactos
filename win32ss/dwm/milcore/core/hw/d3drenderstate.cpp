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
//      Contains CD3DRenderState implementation
//
//      There are 2 purposes to this class:
//
//      1.  Forward state setting calls to the CHwRenderStateManager
//      2.  Group states commonly set together into tables to make it easier to
//          specify rendering options.
//
//      For the second part there are several objects that contain a collection
//      of renderstates:
//
//          AlphaBlendMode FilterMode TextureStageOperation
//
//      We used to check to see if we had the same table set to minimize work,
//      but since the change to using the CHwRenderStateManager that
//      optimization was removed.  We will likely have to revisit it for
//      performance.
//
//      NOTE-2004/05/21-chrisra State blocks are not a win.
//
//      Removing the stateblocks to go to setting the states and restoring them
//      for 3D saved about 20% on our scenarios.  If we have to manage more
//      states that may change, but for the time it looks like a big win to keep
//      from using stateblocks.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

extern HINSTANCE g_DllInstance;

DeclareTag(tagDisableHWText, "MIL-HW", "Disable HW text rendering");
ExternTag(tagWireframe);

// We use these values to define "unknown" states. They need to be values that
// we don't use for any render states, sampler states or texture stage states
// we set. We double-check this using assertions.

#define    MILRS_UNKNOWN 0x7fffffff
#define   MILTOP_UNKNOWN D3DTOP_FORCE_DWORD
#define  MILTEXF_UNKNOWN D3DTEXF_FORCE_DWORD
#define MILBLEND_UNKNOWN D3DBLEND_FORCE_DWORD

//+-----------------------------------------------------------------------------
//
//  Structure:
//      AlphaBlendMode
//
//  Synopsis:
//      Blend mode for the back-end alpha blender.
//
//------------------------------------------------------------------------------
struct AlphaBlendMode
{
    // The subset of dx9 render states this controls
    enum
    {
        ABM_ALPHABLENDENABLE,
        ABM_SRCBLEND,
        ABM_DESTBLEND,
        ABM_NUM
    };

    union
    {
        // This struct adds useful debugging info
        struct
        {
            DWORD AlphaBlendEnable;    // D3DRS_ALPHABLENDENABLE
            D3DBLEND SrcBlend;         // D3DRS_SRCBLEND
            D3DBLEND DestBlend;        // D3DRS_DESTBLEND
        };
        DWORD m_dwValues[ABM_NUM];
    };

    static D3DRENDERSTATETYPE const sc_types[ABM_NUM];

    // This assert protected the range annoation on Type and Value
    C_ASSERT(ABM_NUM == 3);

    D3DRENDERSTATETYPE Type(__range(0, 2 /* this should be ABM_NUM - 1, but PREF* can't parse */) UINT uIndex) const { return sc_types[uIndex]; }
    DWORD Value(__range(0, 2 /* this should be ABM_NUM - 1, but PREF* can't parse */) UINT uIndex) const { return m_dwValues[uIndex]; }
};

D3DRENDERSTATETYPE const AlphaBlendMode::sc_types[] =
{
    D3DRS_ALPHABLENDENABLE   ,
    D3DRS_SRCBLEND           ,
    D3DRS_DESTBLEND          ,
};

//+-----------------------------------------------------------------------------
//
//  Structure:
//      TextureStageOperation
//
//  Synopsis:
//      Description of the operation performed in a given texture stage. These
//      are the building blocks of the fixed-function (legacy) pixel shader.
//
//------------------------------------------------------------------------------

struct TextureStageOperation
{
    BOOL m_fUsesTexture;

    // We treat D3DTOP_DISABLE specially (so that we can preserve the other states)
    D3DTEXTUREOP m_opColor;       // D3DTSS_COLOROP

    // The texture stage states used in m_dwValues
    enum
    {
        TSO_COLORARG1             = 0,
        TSO_COLORARG2             = 1,
        TSO_ALPHAOP               = 2,
        TSO_ALPHAARG1             = 3,
        TSO_ALPHAARG2             = 4,
        TSO_NUM
    };

    union
    {
        // This struct adds useful debugging info
        struct
        {
            DWORD        ColorArg1;     // D3DTSS_COLORARG1
            DWORD        ColorArg2;     // D3DTSS_COLORARG2
            D3DTEXTUREOP AlphaOp;       // D3DTSS_ALPHAOP
            DWORD        AlphaArg1;     // D3DTSS_ALPHAARG1
            DWORD        AlphaArg2;     // D3DTSS_ALPHAARG2
        };
        DWORD m_dwValues[TSO_NUM];
    };

    static D3DTEXTURESTAGESTATETYPE const sc_types[TSO_NUM];

    // This assert protected the range annoation on Type and Value
    C_ASSERT(TSO_NUM == 5);

    D3DTEXTURESTAGESTATETYPE Type(__range(0, 4 /* this should be TSO_NUM - 1, but PREF* can't parse */) UINT uIndex) const { return sc_types[uIndex]; }
    DWORD Value(__range(0, 4 /* this should be TSO_NUM - 1, but PREF* can't parse */) UINT uIndex) const { return m_dwValues[uIndex]; }
};

D3DTEXTURESTAGESTATETYPE const TextureStageOperation::sc_types[] =
{
    D3DTSS_COLORARG1            ,
    D3DTSS_COLORARG2            ,
    D3DTSS_ALPHAOP              ,
    D3DTSS_ALPHAARG1            ,
    D3DTSS_ALPHAARG2            ,
};

//+-----------------------------------------------------------------------------
//
//  Structure:
//      FilterMode
//
//  Synopsis:
//      A set of filter modes for a given sampler.
//
//------------------------------------------------------------------------------

struct FilterMode
{
    enum
    {
        FM_MAGFILTER = 0,
        FM_MINFILTER = 1,
        FM_MIPFILTER = 2,
        FM_NUM
    };

    union
    {
        // This struct adds useful debugging info
        struct
        {
            D3DTEXTUREFILTERTYPE MagFilter; // D3DSAMP_MAGFILTER
            D3DTEXTUREFILTERTYPE MinFilter; // D3DSAMP_MINFILTER
            D3DTEXTUREFILTERTYPE MipFilter; // D3DSAMP_MIPFILTER
        };
        DWORD m_dwValues[FM_NUM];
    };

    static D3DSAMPLERSTATETYPE const sc_types[FM_NUM];

    // This assert protected the range annoation on Type and Value
    C_ASSERT(FM_NUM == 3);

    D3DSAMPLERSTATETYPE Type(__range(0, 2 /* this should be FM_NUM - 1, but PREF* can't parse */) UINT uIndex) const { return sc_types[uIndex]; }
    DWORD Value(__range(0, 2 /* this should be FM_NUM - 1, but PREF* can't parse */) UINT uIndex) const { return m_dwValues[uIndex]; }
};

D3DSAMPLERSTATETYPE const FilterMode::sc_types[] =
{
    D3DSAMP_MAGFILTER,
    D3DSAMP_MINFILTER,
    D3DSAMP_MIPFILTER,
};

//-------------------------------------------------------------------------
// Back-end alpha-blend modes
//-------------------------------------------------------------------------

const AlphaBlendMode

// "Unknown" AlphaBlendMode. Used before initialization or on error, to indicate
// that we don't know the state.

CD3DRenderState::sc_abmUnknown =
{
    /* D3DRENDERSTATE_ALPHABLENDENABLE   */ MILRS_UNKNOWN      ,
    /* D3DRENDERSTATE_SRCBLEND           */ MILBLEND_UNKNOWN   ,
    /* D3DRENDERSTATE_DESTBLEND          */ MILBLEND_UNKNOWN   ,
},

// "SrcCopy". This mode can also be used to implement SrcOver when
// all input colors are opaque.

CD3DRenderState::sc_abmSrcCopy =
{
    /* D3DRENDERSTATE_ALPHABLENDENABLE   */ FALSE           ,
    /* D3DRENDERSTATE_SRCBLEND           */ D3DBLEND_ONE    ,  // Unused
    /* D3DRENDERSTATE_DESTBLEND          */ D3DBLEND_ZERO   ,  // Unused
},

// "SrcOver" - the most common alpha blend. The source and destination use
// premultiplied alpha.

CD3DRenderState::sc_abmSrcOverPremultiplied =
{
    /* D3DRENDERSTATE_ALPHABLENDENABLE   */ TRUE ,
    /* D3DRENDERSTATE_SRCBLEND           */ D3DBLEND_ONE         ,
    /* D3DRENDERSTATE_DESTBLEND          */ D3DBLEND_INVSRCALPHA ,
},

// "SrcUnder" - not the most common, but merely the opposite of SrcOver.
// Both source and destination use premultiplied alpha.

CD3DRenderState::sc_abmSrcUnderPremultiplied =
{
    /* D3DRENDERSTATE_ALPHABLENDENABLE   */ TRUE ,
    /* D3DRENDERSTATE_SRCBLEND           */ D3DBLEND_INVDESTALPHA,
    /* D3DRENDERSTATE_DESTBLEND          */ D3DBLEND_ONE         ,
},

// "SrcAlphaMultiply."  Multiplies the destination by the source
// alpha.  Ignores source color.

CD3DRenderState::sc_abmSrcAlphaMultiply =
{
    /* D3DRENDERSTATE_ALPHABLENDENABLE   */ TRUE ,
    /* D3DRENDERSTATE_SRCBLEND           */ D3DBLEND_ZERO        ,
    /* D3DRENDERSTATE_DESTBLEND          */ D3DBLEND_SRCALPHA    ,
},

// "SrcInverseAlphaMultiply."  Like SrcOver but without adding the source.

CD3DRenderState::sc_abmSrcInverseAlphaMultiply =
{
    /* D3DRENDERSTATE_ALPHABLENDENABLE   */ TRUE ,
    /* D3DRENDERSTATE_SRCBLEND           */ D3DBLEND_ZERO        ,
    /* D3DRENDERSTATE_DESTBLEND          */ D3DBLEND_INVSRCALPHA ,
},

// "SrcOver" but with the source color using non-premultiplied alpha. (The
// destination still uses premultiplied alpha).

CD3DRenderState::sc_abmSrcOver_SrcNonPremultiplied =
{
    /* D3DRENDERSTATE_ALPHABLENDENABLE   */ TRUE ,
    /* D3DRENDERSTATE_SRCBLEND           */ D3DBLEND_SRCALPHA    ,
    /* D3DRENDERSTATE_DESTBLEND          */ D3DBLEND_INVSRCALPHA ,
},

// "SrcOver" but with the source color using non-premultiplied alpha and
// inverting the alpha before using.  The destination still uses
// premultiplied alpha.

CD3DRenderState::sc_abmSrcOver_InverseAlpha_SrcNonPremultiplied =
{
    /* D3DRENDERSTATE_ALPHABLENDENABLE   */ TRUE ,
    /* D3DRENDERSTATE_SRCBLEND           */ D3DBLEND_INVSRCALPHA ,
    /* D3DRENDERSTATE_DESTBLEND          */ D3DBLEND_SRCALPHA    ,
},

// Source color is accepted as vector alpha;
// real color source comes from blend factor.
// This is used for text rendering in both clear type and grey scale mode.

// Note: ClearType blends do not preserve the destination alpha channel.

CD3DRenderState::sc_abmSrcVectorAlphaWithBlendFactor =
{
    /* D3DRENDERSTATE_ALPHABLENDENABLE   */ TRUE ,
    /* D3DRENDERSTATE_SRCBLEND           */ D3DBLEND_BLENDFACTOR ,
    /* D3DRENDERSTATE_DESTBLEND          */ D3DBLEND_INVSRCCOLOR ,
},

// Source color is accepted as vector alpha;
// no real color source yet involved (we'll add the color
// in a second pass, using sc_abmAddSourceColor blend mode).
CD3DRenderState::sc_abmSrcVectorAlpha =
{
    /* D3DRENDERSTATE_ALPHABLENDENABLE   */ TRUE ,
    /* D3DRENDERSTATE_SRCBLEND           */ D3DBLEND_ZERO        ,
    /* D3DRENDERSTATE_DESTBLEND          */ D3DBLEND_INVSRCCOLOR ,
},

// The source and destination are added together. Used in 2-pass ClearType, to
// add in the brush color.
//
// ClearType blends do not preserve the destination alpha channel.

CD3DRenderState::sc_abmAddSourceColor =
{
    /* D3DRENDERSTATE_ALPHABLENDENABLE   */ TRUE ,
    /* D3DRENDERSTATE_SRCBLEND           */ D3DBLEND_ONE         ,
    /* D3DRENDERSTATE_DESTBLEND          */ D3DBLEND_ONE         ,
},

CD3DRenderState::sc_abmSrcAlphaWithInvDestColor =
{
    /* D3DRENDERSTATE_ALPHABLENDENABLE   */ TRUE ,
    /* D3DRENDERSTATE_SRCBLEND           */ D3DBLEND_INVDESTCOLOR,
    /* D3DRENDERSTATE_DESTBLEND          */ D3DBLEND_SRCALPHA    ,
};

//------------------------------------------------------------------------------
// Texture stage operations for the "legacy pixel shader"
//------------------------------------------------------------------------------

const TextureStageOperation

// "Unknown" TextureStageOperation. Used before initialization or on error, to
// indicate that we don't know the state.

CD3DRenderState::sc_tsoUnknown =
{
    /* m_fUsesTexture               */ TRUE              ,
    /* D3DTSS_COLOROP               */ MILTOP_UNKNOWN    ,
    /* D3DTSS_COLORARG1             */ MILRS_UNKNOWN     ,
    /* D3DTSS_COLORARG2             */ MILRS_UNKNOWN     ,
    /* D3DTSS_ALPHAOP               */ MILTOP_UNKNOWN    ,
    /* D3DTSS_ALPHAARG1             */ MILRS_UNKNOWN     ,
    /* D3DTSS_ALPHAARG2             */ MILRS_UNKNOWN     ,
},

CD3DRenderState::sc_tsoDiffuse =
{
    /* m_fUsesTexture               */ FALSE             ,
    /* D3DTSS_COLOROP               */ D3DTOP_SELECTARG1 ,
    /* D3DTSS_COLORARG1             */ D3DTA_DIFFUSE     ,
    /* D3DTSS_COLORARG2             */ D3DTA_CURRENT     ,/*UNUSED*/
    /* D3DTSS_ALPHAOP               */ D3DTOP_SELECTARG1 ,
    /* D3DTSS_ALPHAARG1             */ D3DTA_DIFFUSE     ,
    /* D3DTSS_ALPHAARG2             */ D3DTA_CURRENT     ,/*UNUSED*/
},

// Completely ignore argument 2 and take only the texture's
// values
CD3DRenderState::sc_tsoSelectTexture =
{
    /* m_fUsesTexture               */ TRUE              ,
    /* D3DTSS_COLOROP               */ D3DTOP_SELECTARG1 ,
    /* D3DTSS_COLORARG1             */ D3DTA_TEXTURE     ,
    /* D3DTSS_COLORARG2             */ D3DTA_CURRENT     ,/*UNUSED*/
    /* D3DTSS_ALPHAOP               */ D3DTOP_SELECTARG1 ,
    /* D3DTSS_ALPHAARG1             */ D3DTA_TEXTURE     ,
    /* D3DTSS_ALPHAARG2             */ D3DTA_CURRENT     ,/*UNUSED*/
},

// This is the default D3D state for stage 0 and it is
// used by diffuse material to minimize the number of
// state changes
CD3DRenderState::sc_tsoTextureXCurrentRGB =
{
    /* m_fUsesTexture               */ TRUE              ,
    /* D3DTSS_COLOROP               */ D3DTOP_MODULATE   ,
    /* D3DTSS_COLORARG1             */ D3DTA_TEXTURE     ,
    /* D3DTSS_COLORARG2             */ D3DTA_CURRENT     ,/*DIFFUSE in stage 0*/
    /* D3DTSS_ALPHAOP               */ D3DTOP_SELECTARG1 ,
    /* D3DTSS_ALPHAARG1             */ D3DTA_TEXTURE     ,
    /* D3DTSS_ALPHAARG2             */ D3DTA_CURRENT     ,/*UNUSED*/
},

// This is no longer used but we'll leave it here
// in case we ever put specular back in the vertex format
CD3DRenderState::sc_tsoTextureXSpecularRGB =
{
    /* m_fUsesTexture               */ TRUE              ,
    /* D3DTSS_COLOROP               */ D3DTOP_MODULATE   ,
    /* D3DTSS_COLORARG1             */ D3DTA_TEXTURE     ,
    /* D3DTSS_COLORARG2             */ D3DTA_SPECULAR    ,
    /* D3DTSS_ALPHAOP               */ D3DTOP_SELECTARG1 ,
    /* D3DTSS_ALPHAARG1             */ D3DTA_TEXTURE     ,
    /* D3DTSS_ALPHAARG2             */ D3DTA_CURRENT     ,/*UNUSED*/
},

CD3DRenderState::sc_tsoPremulTextureXCurrent =
{
    /* m_fUsesTexture               */ TRUE              ,
    /* D3DTSS_COLOROP               */ D3DTOP_MODULATE   ,
    /* D3DTSS_COLORARG1             */ D3DTA_TEXTURE     ,
    /* D3DTSS_COLORARG2             */ D3DTA_CURRENT     ,/*DIFFUSE in stage 0*/
    /* D3DTSS_ALPHAOP               */ D3DTOP_MODULATE   ,
    /* D3DTSS_ALPHAARG1             */ D3DTA_TEXTURE     ,
    /* D3DTSS_ALPHAARG2             */ D3DTA_CURRENT     ,/*DIFFUSE in stage 0*/
},

CD3DRenderState::sc_tsoPremulTextureXDiffuse =
{
    /* m_fUsesTexture               */ TRUE              ,
    /* D3DTSS_COLOROP               */ D3DTOP_MODULATE   ,
    /* D3DTSS_COLORARG1             */ D3DTA_TEXTURE     ,
    /* D3DTSS_COLORARG2             */ D3DTA_DIFFUSE     ,
    /* D3DTSS_ALPHAOP               */ D3DTOP_MODULATE   ,
    /* D3DTSS_ALPHAARG1             */ D3DTA_TEXTURE     ,
    /* D3DTSS_ALPHAARG2             */ D3DTA_CURRENT     ,/*DIFFUSE in stage 0*/
},

// Ignore texture's alpha by treating it as opaque, then
// modulate (multiply) by current (which for alpha is
// effectively select current alpha).
CD3DRenderState::sc_tsoOpaqueTextureXCurrent =
{
    /* m_fUsesTexture               */ TRUE              ,
    /* D3DTSS_COLOROP               */ D3DTOP_MODULATE   ,
    /* D3DTSS_COLORARG1             */ D3DTA_TEXTURE     ,
    /* D3DTSS_COLORARG2             */ D3DTA_CURRENT     ,/*DIFFUSE in stage 0*/
    /* D3DTSS_ALPHAOP               */ D3DTOP_SELECTARG1 ,
    /* D3DTSS_ALPHAARG1             */ D3DTA_CURRENT     ,/*DIFFUSE in stage 0*/
    /* D3DTSS_ALPHAARG2             */ D3DTA_CURRENT     ,/*UNUSED*/
},

// Ignore texture's alpha by treating it as opaque, then
// modulate (multiply) by diffuse (which for alpha is
// effectively select diffuse alpha).
CD3DRenderState::sc_tsoOpaqueTextureXDiffuse =
{
    /* m_fUsesTexture               */ TRUE              ,
    /* D3DTSS_COLOROP               */ D3DTOP_MODULATE   ,
    /* D3DTSS_COLORARG1             */ D3DTA_TEXTURE     ,
    /* D3DTSS_COLORARG2             */ D3DTA_DIFFUSE     ,
    /* D3DTSS_ALPHAOP               */ D3DTOP_SELECTARG1 ,
    /* D3DTSS_ALPHAARG1             */ D3DTA_CURRENT     ,/*DIFFUSE in stage 0*/
    /* D3DTSS_ALPHAARG2             */ D3DTA_CURRENT     ,/*UNUSED*/
},

CD3DRenderState::sc_tsoMaskTextureXCurrent =
{
    /* m_fUsesTexture               */ TRUE              ,
    /* D3DTSS_COLOROP               */ D3DTOP_MODULATE   ,
    /* D3DTSS_COLORARG1             */ D3DTA_TEXTURE | D3DTA_ALPHAREPLICATE,
    /* D3DTSS_COLORARG2             */ D3DTA_CURRENT     ,
    /* D3DTSS_ALPHAOP               */ D3DTOP_MODULATE   ,
    /* D3DTSS_ALPHAARG1             */ D3DTA_TEXTURE     ,
    /* D3DTSS_ALPHAARG2             */ D3DTA_CURRENT     ,
},

CD3DRenderState::sc_tsoBumpMapTexture =
{
    /* m_fUsesTexture               */ TRUE              ,
    /* D3DTSS_COLOROP               */ D3DTOP_BUMPENVMAP ,
    /* D3DTSS_COLORARG1             */ D3DTA_TEXTURE     ,/*UNUSED*/
    /* D3DTSS_COLORARG2             */ D3DTA_DIFFUSE     ,/*UNUSED*/
    /* D3DTSS_ALPHAOP               */ D3DTOP_MODULATE   ,/*UNUSED*/
    /* D3DTSS_ALPHAARG1             */ D3DTA_TEXTURE     ,/*UNUSED*/
    /* D3DTSS_ALPHAARG2             */ D3DTA_CURRENT     ,/*UNUSED*/
},

    // For color selects the texture, for alpha multiplies texture and diffuse.
CD3DRenderState::sc_tsoColorSelectTextureAlphaMultiplyDiffuse =
{
    /* m_fUsesTexture               */ TRUE              ,
    /* D3DTSS_COLOROP               */ D3DTOP_SELECTARG1 ,
    /* D3DTSS_COLORARG1             */ D3DTA_TEXTURE     ,
    /* D3DTSS_COLORARG2             */ D3DTA_DIFFUSE     ,/*UNUSED*/
    /* D3DTSS_ALPHAOP               */ D3DTOP_MODULATE   ,
    /* D3DTSS_ALPHAARG1             */ D3DTA_TEXTURE     ,
    /* D3DTSS_ALPHAARG2             */ D3DTA_DIFFUSE     ,
},

    // For color selects the texture, for alpha multiplies texture and current.
CD3DRenderState::sc_tsoColorSelectTextureAlphaMultiplyCurrent =
{
    /* m_fUsesTexture               */ TRUE              ,
    /* D3DTSS_COLOROP               */ D3DTOP_SELECTARG1 ,
    /* D3DTSS_COLORARG1             */ D3DTA_TEXTURE     ,
    /* D3DTSS_COLORARG2             */ D3DTA_CURRENT     ,/*UNUSED*/
    /* D3DTSS_ALPHAOP               */ D3DTOP_MODULATE   ,
    /* D3DTSS_ALPHAARG1             */ D3DTA_TEXTURE     ,
    /* D3DTSS_ALPHAARG2             */ D3DTA_CURRENT     ,
},
    
    // For color selects the diffuse, for alpha multiplies texture and diffuse.
CD3DRenderState::sc_tsoColorSelectDiffuseAlphaMultiplyTexture =
{
    /* m_fUsesTexture               */ TRUE              ,
    /* D3DTSS_COLOROP               */ D3DTOP_SELECTARG2 ,
    /* D3DTSS_COLORARG1             */ D3DTA_TEXTURE     ,/*UNUSED*/
    /* D3DTSS_COLORARG2             */ D3DTA_DIFFUSE     ,
    /* D3DTSS_ALPHAOP               */ D3DTOP_MODULATE   ,
    /* D3DTSS_ALPHAARG1             */ D3DTA_TEXTURE     ,
    /* D3DTSS_ALPHAARG2             */ D3DTA_DIFFUSE     ,
},

    // For color selects the current, for alpha multiplies texture and current.
CD3DRenderState::sc_tsoColorSelectCurrentAlphaMultiplyTexture =
{
    /* m_fUsesTexture               */ TRUE              ,
    /* D3DTSS_COLOROP               */ D3DTOP_SELECTARG2 ,
    /* D3DTSS_COLORARG1             */ D3DTA_TEXTURE     ,/*UNUSED*/
    /* D3DTSS_COLORARG2             */ D3DTA_CURRENT     ,
    /* D3DTSS_ALPHAOP               */ D3DTOP_MODULATE   ,
    /* D3DTSS_ALPHAARG1             */ D3DTA_TEXTURE     ,
    /* D3DTSS_ALPHAARG2             */ D3DTA_CURRENT     ,
};


//-------------------------------------------------------------------------
// Texture Filter Modes
//-------------------------------------------------------------------------

const FilterMode

// "Unknown" FilterMode. Used before initialization or on error, to indicate
// that we don't know the state.

CD3DRenderState::sc_fmUnknown =
{
        /* D3DSAMP_MAGFILTER            */ MILTEXF_UNKNOWN  ,
        /* D3DSAMP_MINFILTER            */ MILTEXF_UNKNOWN  ,
        /* D3DSAMP_MIPFILTER            */ MILTEXF_UNKNOWN  ,
},

CD3DRenderState::sc_fmNearest =
{
        /* D3DSAMP_MAGFILTER            */ D3DTEXF_POINT    ,
        /* D3DSAMP_MINFILTER            */ D3DTEXF_POINT    ,
        /* D3DSAMP_MIPFILTER            */ D3DTEXF_NONE     ,
},

CD3DRenderState::sc_fmLinear =
{
        /* D3DSAMP_MAGFILTER            */ D3DTEXF_LINEAR   ,
        /* D3DSAMP_MINFILTER            */ D3DTEXF_LINEAR   ,
        /* D3DSAMP_MIPFILTER            */ D3DTEXF_NONE     ,
},

CD3DRenderState::sc_fmTriLinear =
{
        /* D3DSAMP_MAGFILTER            */ D3DTEXF_LINEAR   ,
        /* D3DSAMP_MINFILTER            */ D3DTEXF_LINEAR   ,
        /* D3DSAMP_MIPFILTER            */ D3DTEXF_LINEAR   ,
},

CD3DRenderState::sc_fmAnisotropic =
{
        /* D3DSAMP_MAGFILTER            */ D3DTEXF_ANISOTROPIC  ,
        /* D3DSAMP_MINFILTER            */ D3DTEXF_ANISOTROPIC  ,
        /* D3DSAMP_MIPFILTER            */ D3DTEXF_LINEAR       ,
},

CD3DRenderState::sc_fmMinOnlyAnisotropic =
{
        /* D3DSAMP_MAGFILTER            */ D3DTEXF_LINEAR,
        /* D3DSAMP_MINFILTER            */ D3DTEXF_ANISOTROPIC  ,
        /* D3DSAMP_MIPFILTER            */ D3DTEXF_LINEAR       ,
},

CD3DRenderState::sc_fmConvolution =
{
        /* D3DSAMP_MAGFILTER            */ D3DTEXF_CONVOLUTIONMONO   ,
        /* D3DSAMP_MINFILTER            */ D3DTEXF_CONVOLUTIONMONO   ,
        /* D3DSAMP_MIPFILTER            */ D3DTEXF_NONE     ,
};

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::CD3DRenderState
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------

CD3DRenderState::CD3DRenderState()
{
    for (int i = 0; i < ARRAYSIZE(m_pPixelShaders); i++)
    {
        m_pPixelShaders[i] = NULL;
    }

    m_pStateManager = NULL;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::~CD3DRenderState
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------

CD3DRenderState::~CD3DRenderState()
{
    for (int i = 0; i < ARRAYSIZE(m_pPixelShaders); i++)
    {
        ReleaseInterface(m_pPixelShaders[i]);
    }

    ReleaseInterface(m_pStateManager);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::GetFillMode
//
//  Synopsis:
//      Retrieves the current fillmode
//
//------------------------------------------------------------------------------
HRESULT
CD3DRenderState::GetFillMode(
    __out_ecount(1) D3DFILLMODE *pd3dFillMode
    ) const
{
    HRESULT hr = S_OK;
    DWORD dwFillMode;

    IFC(m_pStateManager->GetRenderState(D3DRS_FILLMODE, &dwFillMode));

    *pd3dFillMode = static_cast<D3DFILLMODE>(dwFillMode);

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::GetDepthTestFunction
//
//  Synopsis:
//      Retrieves the current depth test function
//
//------------------------------------------------------------------------------
HRESULT
CD3DRenderState::GetDepthTestFunction(
    __out_ecount(1) D3DCMPFUNC *pd3dDepthTestFunction
    ) const
{
    HRESULT hr = S_OK;
    DWORD dwDepthTestFunction;
    
    IFC(m_pStateManager->GetRenderState(D3DRS_ZFUNC, &dwDepthTestFunction));

    *pd3dDepthTestFunction = static_cast<D3DCMPFUNC>(dwDepthTestFunction);

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::ResetState
//
//  Synopsis:
//      This function resets all render state to the default values.
//
//------------------------------------------------------------------------------
HRESULT
CD3DRenderState::ResetState()
{
    return m_pStateManager->SetDefaultState(
        m_pDeviceNoRef->CanHandleBlendFactor(),
        m_pDeviceNoRef->SupportsScissorRect(),
        m_pDeviceNoRef->GetMaxStreams(),
        m_pDeviceNoRef->GetMaxDesiredAnisotropicFilterLevel()
        );
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::Init
//
//  Synopsis:
//      Associated a d3d device for this manager
//
//------------------------------------------------------------------------------

HRESULT
CD3DRenderState::Init(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice,
    __inout_ecount(1) IDirect3DDevice9 *pD3DDevice
    )
{
    HRESULT hr = S_OK;
    DWORD dwMaxTextureBlendStages = 0;

    Assert(pDevice && !m_pDeviceNoRef);

    //
    // Cache device
    //

    m_pDeviceNoRef = pDevice; // No AddRef because it would be a circular reference.

    dwMaxTextureBlendStages = min(
        DWORD(MIL_TEXTURE_STAGE_COUNT),
        m_pDeviceNoRef->GetMaxTextureBlendStages()
        );

    IFC(CHwRenderStateManager::Create(
        pD3DDevice,
        dwMaxTextureBlendStages,
        m_pDeviceNoRef->CanHandleBlendFactor(),
        m_pDeviceNoRef->SupportsScissorRect(),
        m_pDeviceNoRef->GetMaxStreams(),
        m_pDeviceNoRef->GetMaxDesiredAnisotropicFilterLevel(),
        &m_pStateManager
        ));

    m_pStateManager->InvalidateScissorRect();

    m_fDrawTextUsingPS20 = FALSE;

    // If pixel shaders are not available then prohibit HW accelerated text rendering.
    // (it will go through SW fallback)
    m_fCanDrawText = pDevice->GetPixelShaderVersion() >= D3DPS_VERSION(1,1)
                     && pDevice->GetMaxTextureBlendStages() >= 4
                     && pDevice->CanHandleBlendFactor()
                     && !IsTagEnabled(tagDisableHWText);

    if (m_fCanDrawText)
    {
        // InitAlphaTextures() should be called prior to InitPixelShaders that
        // depends on alpha texture format
        if (FAILED(InitAlphaTextures()))
        {
            // the device does not support required texture formats:
            // don't fail, just reject HW accelerated text
            m_fCanDrawText = FALSE;
        }
    }

    if (m_fCanDrawText)
    {
        m_fDrawTextUsingPS20 = pDevice->GetPixelShaderVersion() >= D3DPS_VERSION(2,0);
        IFC(InitPixelShaders());
    }

    // Choose text filtering mode depending on dbg settings.


    m_pTextFilterMode = &sc_fmLinear;

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::SetFilterMode
//
//  Synopsis:
//      Set the filter mode for a given sampler.
//
//------------------------------------------------------------------------------

HRESULT CD3DRenderState::SetFilterMode(
    DWORD dwSampler,
    __in_ecount(1) const FilterMode *pfmNew
    )
{
    Assert(dwSampler < MIL_SAMPLER_COUNT);
    AssertMsg(
        (pfmNew && (pfmNew != &sc_fmUnknown)),
        "Trying to set an undefined filter mode");

    HRESULT hr = S_OK;

    for (int i = 0; i < FilterMode::FM_NUM; i++)
    {
        DWORD newValue = pfmNew->Value(i);

        IFC(m_pStateManager->SetSamplerStateInline(
            dwSampler,
            pfmNew->Type(i),
            newValue
            ));
    }

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::SetDepthStencilBufferInternal
//
//  Synopsis:
//      Grabs the d3d surface and then forwards the call to the state manager.
//
//------------------------------------------------------------------------------
HRESULT
CD3DRenderState::SetDepthStencilSurfaceInternal(
    __in_ecount_opt(1) CD3DSurface *pDepthStencilBuffer
    )
{
    HRESULT hr = S_OK;
    IDirect3DSurface9 *pD3DSurfaceNoRef = NULL;
    UINT uWidth = 0;
    UINT uHeight = 0;
    
    if (pDepthStencilBuffer != NULL)
    {
        Assert(pDepthStencilBuffer->IsValid());

        pD3DSurfaceNoRef = pDepthStencilBuffer->GetD3DSurfaceNoAddRef();

        Assert(pD3DSurfaceNoRef);

        pDepthStencilBuffer->GetSurfaceSize(
            &uWidth,
            &uHeight
            );
    }
    
    IFC(m_pStateManager->SetDepthStencilSurfaceInline(
        pD3DSurfaceNoRef,
        uWidth,
        uHeight
        ));

Cleanup:            
    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::ReleaseUseOfDepthStencilBufferInternal
//
//  Synopsis:
//      Grabs the d3d surface and then forwards the call to the state manager.
//
//------------------------------------------------------------------------------
HRESULT 
CD3DRenderState::ReleaseUseOfDepthStencilSurfaceInternal(
    __in_ecount(1) CD3DSurface *pDepthStencilBuffer
    )
{
    HRESULT hr = S_OK;

    IDirect3DSurface9 *pD3DSurfaceNoRef =
        pDepthStencilBuffer->GetD3DSurfaceNoAddRef();

    if (pD3DSurfaceNoRef)
    {
        IFC(m_pStateManager->ReleaseUseOfDepthStencilBuffer(
            pD3DSurfaceNoRef
            ));
    }

Cleanup:
    RRETURN(hr);
}

#if DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::AssertFilterMode
//
//  Synopsis:
//      Assert that the current sampler state state matches the current
//      FilterMode.
//
//  Notes:
//      We assume we won't use a "pure" D3D device, at least in debug builds.
//
//------------------------------------------------------------------------------

VOID CD3DRenderState::AssertFilterMode(
    DWORD dwSampler
    )
{
    // State assertions need a D3D9 bug fixed
    //   This is disabled until D3D fixes a bug in GetXXXState
    //   (XXX=TextureStage, Sampler, Render). This bug causes GetXXXState to
    //   always fail on debug builds of D3D9, for many states.
    //
    //   No bug # yet; GilesB is the contact dev.

#if 0
    if (!m_pDeviceNoRef->IsPureDevice())
    {
        Assert(dwSampler < MIL_MAX_SAMPLER);

        HRESULT hr = S_OK;

        FilterMode const* pfmCurrent = m_pFilterModeCurrent[dwSampler];

        Assert(pfmCurrent);

        if (pfmCurrent != &sc_fmUnknown)
        {
            for (int i = 0; i < FilterMode::FM_NUM; i++)
            {
                // Check that the device state is what we expect.

                DWORD dwValueExpected = pfmCurrent->Value(i);
                DWORD dwValueUnknown = sc_fmUnknown.Value(i);
                DWORD dwValueCurrent;

                hr = m_pD3DDevice->GetSamplerState(dwSampler, pfmCurrent->Type(i), &dwValueCurrent);

                Assert(SUCCEEDED(hr));
                Assert(dwValueExpected != dwValueUnknown);
                Assert(dwValueCurrent == dwValueExpected);
            }
        }
    }
#endif
}
#endif

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::InternalSetAlphaBlendMode
//
//  Synopsis:
//      Set the alpha-blend mode.
//
//------------------------------------------------------------------------------

HRESULT CD3DRenderState::SetAlphaBlendMode(
    __in_ecount(1) const AlphaBlendMode *pabmNew
    )
{
    AssertMsg(
        (pabmNew && (pabmNew != &sc_abmUnknown)),
        "Trying to set an undefined blend mode");

    HRESULT hr = S_OK;

    for (int i = 0; i < AlphaBlendMode::ABM_NUM; i++)
    {
        DWORD newValue = pabmNew->Value(i);

        IFC(SetRenderState(pabmNew->Type(i), newValue));
    }

Cleanup:
    RRETURN(hr);
}

#if DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::AssertAlphaBlendMode
//
//  Synopsis:
//      Assert that the current render state matches the current AlphaBlendMode.
//
//  Notes:
//      We assume we won't use a "pure" D3D device, at least in debug builds.
//
//------------------------------------------------------------------------------

VOID CD3DRenderState::AssertAlphaBlendMode()
{
    //  State assertions need a D3D9 bug fixed
    //   This is disabled until D3D fixes a bug in GetXXXState
    //   (XXX=TextureStage, Sampler, Render). This bug causes GetXXXState to
    //   always fail on debug builds of D3D9, for many states.
    //
    //   No bug # yet; GilesB is the contact dev.

#if 0
    if (!m_pDeviceNoRef->IsPureDevice())
    {
        HRESULT hr = S_OK;

        AlphaBlendMode const* pabmCurrent = m_pAlphaBlendModeCurrent;

        Assert(pabmCurrent);

        if (pabmCurrent != &sc_abmUnknown)
        {
            for (int i = 0; i < AlphaBlendMode::ABM_NUM; i++)
            {
                // Check that the device state is what we expect.

                DWORD dwValueExpected = pabmCurrent->Value(i);
                DWORD dwValueUnknown = sc_abmUnknown.Value(i);
                DWORD dwValueCurrent;

                hr = m_pD3DDevice->GetRenderState(pabmCurrent->Type(i), &dwValueCurrent);

                Assert(SUCCEEDED(hr));
                Assert(dwValueExpected != dwValueUnknown);
                Assert(dwValueCurrent == dwValueExpected);
            }
        }
    }
#endif
}
#endif

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::SetTextureStageOperation
//
//  Synopsis:
//      Set the texture operation for a given texture stage.
//
//------------------------------------------------------------------------------

HRESULT
CD3DRenderState::SetTextureStageOperation(
    DWORD dwStage,
    __in_ecount(1) const TextureStageOperation *ptsoNew
    )
{
    AssertMsg(
        (ptsoNew && (ptsoNew != &sc_tsoUnknown)),
        "Trying to set an undefined texture stage operation");
    Assert(dwStage < MIL_TEXTURE_STAGE_COUNT);
    AssertTextureStageOperation(dwStage);

    HRESULT hr = S_OK;

    // When a stage is disabled, D3DTSS_COLOROP is D3DTOP_DISABLE, but
    // m_ptsoCurrent[dwStage] still describes the current values for the other
    // states.
    //
    // m_fTextureStageDisabled[dwStage] records whether we're in this situation
    // or not.

    // Texture stages should not be disabled in this way - use
    // DisableTextureStage instead.
    Assert(ptsoNew->m_opColor != D3DTOP_DISABLE);

    // Set D3DTSS_COLOROP

    IFC(m_pStateManager->SetTextureStageStateInline(dwStage, D3DTSS_COLOROP, ptsoNew->m_opColor) );

    for (int i = 0; i < TextureStageOperation::TSO_NUM; i++)
    {
        DWORD newValue = ptsoNew->Value(i);

        IFC(m_pStateManager->SetTextureStageStateInline(dwStage, ptsoNew->Type(i), newValue) );
    }

    // [agodfrey] Is this required? Workitem #1743 covers the need to not
    // hold onto textures for longer than necessary. And this doesn't work
    // if the previous primitive used multiple stages or a pixel shader.

    // If the new operation doesn't use a texture, we'll make sure that the
    // texture for this stage is set to NULL. Otherwise, the caller must set
    // the texture later using CD3DRenderState::SetTexture.

    if (!ptsoNew->m_fUsesTexture)
    {
        // We only need to do anything if the previous operation is undefined or uses a texture

        //   NULL unused textures immediately?
        //   Should we instead require NULL to be the default texture for
        //   all stages (i.e. the caller must NULL out the texture before
        //   returning)?
        //
        // Right now, if the previous primitive used more texture stages, we
        // won't remember to NULL out the texture for the additional texture
        // stages. We also don't NULL the texture in DisableTextureStage.
        //
        // Likewise - when a pixel shader is used, we don't NULL out its texture,

        //   Pixel shaders use textures
        //   This test doesn't work when some primitives use pixel shaders.


        IFC(m_pStateManager->SetTextureInline(dwStage, NULL) );
    }

Cleanup:
    RRETURN(hr);
}

#if DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::AssertTextureStageOperation
//
//  Synopsis:
//      Assert that the current texture stage state matches the current
//      TextureStageOperation.
//
//  Notes:
//      We assume we won't use a "pure" D3D device, at least in debug builds.
//
//------------------------------------------------------------------------------

VOID CD3DRenderState::AssertTextureStageOperation(DWORD dwStage)
{
    //  State assertions need a D3D9 bug fixed
    //   This is disabled until D3D fixes a bug in GetXXXState
    //   (XXX=TextureStage, Sampler, Render). This bug causes GetXXXState to
    //   always fail on debug builds of D3D9, for many states.
    //
    //   No bug # yet; GilesB is the contact dev.
#if 0
    Assert(dwStage < MIL_MAX_TEXTURE_STAGE);

    HRESULT hr = S_OK;

    TextureStageOperation const* ptsoCurrent = m_ptsoCurrent[dwStage];

    Assert(ptsoCurrent);

    if (ptsoCurrent != &sc_tsoUnknown
        && !m_pDeviceNoRef->IsPureDevice())
    {
        DWORD dwValueCurrent;
        DWORD dwValueExpected;
        DWORD dwValueUnknown = sc_tsoUnknown.m_opColor;

        dwValueExpected = ptsoCurrent->m_opColor;
        if (m_fTextureStageDisabled[dwStage])
        {
            dwValueExpected = D3DTOP_DISABLE;
        }

        hr = m_pD3DDevice->GetTextureStageState(dwStage, D3DTSS_COLOROP, &dwValueCurrent);

        Assert(SUCCEEDED(hr));
        Assert(dwValueExpected != dwValueUnknown);
        Assert(dwValueCurrent == dwValueExpected);

        if (m_fTextureStageDisabled[dwStage])
        {
            Assert(dwValueCurrent == D3DTOP_DISABLE);
        }
        else
        {
            Assert(dwValueCurrent == ptsoCurrent->m_opColor);
        }

        for (int i = 0; i < TextureStageOperation::TSO_NUM; i++)
        {
            // Check that the device state is what we expect.

            dwValueExpected = ptsoCurrent->Value(i);
            dwValueUnknown = sc_tsoUnknown.Value(i);

            hr = m_pD3DDevice->GetTextureStageState(dwStage, ptsoCurrent->Type(i), &dwValueCurrent);

            // We assume we won't use a "pure" D3D device, at least in debug
            // builds.

            Assert(SUCCEEDED(hr));
            Assert(dwValueExpected != dwValueUnknown);
            Assert(dwValueCurrent == dwValueExpected);
        }
    }
#endif
}
#endif

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::SetConstantRegisters_SolidBrush_PS11()
//
//  Synopsis:
//      Adjust pixel shader 1.1 for gamma correction in solid brush mode.
//
//------------------------------------------------------------------------------
HRESULT
CD3DRenderState::SetConstantRegisters_SolidBrush_PS11(
    DWORD dwColor,
    UINT gammaIndex
    )
{
    float cregs[5][4];

    int ir = MIL_COLOR_GET_RED  (dwColor);
    int ig = MIL_COLOR_GET_GREEN(dwColor);
    int ib = MIL_COLOR_GET_BLUE (dwColor);
    int ia = MIL_COLOR_GET_ALPHA(dwColor);

    // average intensity
    int ii = (ir+ig+ig+ib)>>2;

    float fr = float(ir*(1./255));
    float fg = float(ig*(1./255));
    float fb = float(ib*(1./255));
    float fa = float(ia*(1./255));
    float fi = float(ii*(1./255));

    const GammaRatios& coefs = CGammaHandler::sc_gammaRatios[gammaIndex];

    cregs[0][0] = fa;
    cregs[0][1] = 0.f;
    cregs[0][2] = 0.f;
    cregs[0][3] = 0.f;

    cregs[1][0] = 0.f;
    cregs[1][1] = fa;
    cregs[1][2] = 0.f;
    cregs[1][3] = 0.f;

    cregs[2][0] = 0.f;
    cregs[2][1] = 0.f;
    cregs[2][2] = fa;
    cregs[2][3] = 0.f;

    cregs[3][0] = coefs.g1*fr + coefs.g2; // for clear type rendering
    cregs[3][1] = coefs.g1*fg + coefs.g2; // for clear type rendering
    cregs[3][2] = coefs.g1*fb + coefs.g2; // for clear type rendering
    cregs[3][3] = coefs.g1*fi + coefs.g2; // for grey scale rendering

    cregs[4][0] = coefs.g3*fr + coefs.g4; // for clear type rendering
    cregs[4][1] = coefs.g3*fg + coefs.g4; // for clear type rendering
    cregs[4][2] = coefs.g3*fb + coefs.g4; // for clear type rendering
    cregs[4][3] = coefs.g3*fi + coefs.g4; // for grey scale rendering

    return m_pStateManager->SetPixelShaderConstantF(1, cregs[0], 5);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::SetConstantRegisters_SolidBrush_PS20()
//
//  Synopsis:
//      Adjust pixel shader 2.0 for gamma correction in solid brush mode.
//
//------------------------------------------------------------------------------
HRESULT
CD3DRenderState::SetConstantRegisters_SolidBrush_PS20(
    DWORD dwColor,
    UINT gammaIndex
    )
{
    float cregs[4][4];

    int ir = MIL_COLOR_GET_RED  (dwColor);
    int ig = MIL_COLOR_GET_GREEN(dwColor);
    int ib = MIL_COLOR_GET_BLUE (dwColor);
    int ia = MIL_COLOR_GET_ALPHA(dwColor);

    // average intensity
    int ii = (ir+ig+ig+ib)>>2;

    float fr = float(ir*(1./255));
    float fg = float(ig*(1./255));
    float fb = float(ib*(1./255));
    float fa = float(ia*(1./255));
    float fi = float(ii*(1./255));

    const GammaRatios& coefs = CGammaHandler::sc_gammaRatios[gammaIndex];

    cregs[0][0] = fa;
    cregs[0][1] = fa;
    cregs[0][2] = fa;
    cregs[0][3] = fa;

    cregs[1][0] = 1.f;
    cregs[1][1] = 1.f;
    cregs[1][2] = 1.f;
    cregs[1][3] = 1.f;

    cregs[2][0] = coefs.g1*fr + coefs.g2; // for clear type rendering
    cregs[2][1] = coefs.g1*fg + coefs.g2; // for clear type rendering
    cregs[2][2] = coefs.g1*fb + coefs.g2; // for clear type rendering
    cregs[2][3] = coefs.g1*fi + coefs.g2; // for grey scale rendering

    cregs[3][0] = coefs.g3*fr + coefs.g4; // for clear type rendering
    cregs[3][1] = coefs.g3*fg + coefs.g4; // for clear type rendering
    cregs[3][2] = coefs.g3*fb + coefs.g4; // for clear type rendering
    cregs[3][3] = coefs.g3*fi + coefs.g4; // for grey scale rendering

    return m_pStateManager->SetPixelShaderConstantF(2, cregs[0], 4);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::SetClearTypeOffsets()
//
//  Synopsis:
//      Adjust pixel shader 2.0 for clear type rendering. Given values are
//      texture coordinate offsets for sampling alpha for blue color component.
//      Green one is sampled without offset, red uses offsets opposite to blue.
//
//------------------------------------------------------------------------------
HRESULT
CD3DRenderState::SetClearTypeOffsets(float ds, float dt)
{
    float creg[4];

    // Following code proves the cost of SetPixelShaderConstantF().
    // TextRender demo-shift animation shows 3% speed improvement.
    // Likely it should be in render state manager
    //static bool fSkip = false;
    //if (fSkip) return S_OK;
    //fSkip = true;

    creg[0] = ds;
    creg[1] = dt;
    creg[2] = 0.f;
    creg[3] = 0.f;

    return m_pStateManager->SetPixelShaderConstantF(1, creg, 1);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::SetConstantRegisters_TexturedBrush()
//
//  Synopsis:
//      Adjust pixel shader for gamma correction in textured brush mode.
//
//------------------------------------------------------------------------------
HRESULT
CD3DRenderState::SetConstantRegisters_TexturedBrush(
    UINT gammaIndex,
    float flEffectAlpha
    )
{
    float cregs[5][4];

    const GammaRatios& coefs = CGammaHandler::sc_gammaRatios[gammaIndex];

    cregs[0][0] =
    cregs[0][1] =
    cregs[0][2] = coefs.d4;
    cregs[0][3] = coefs.d1;

    cregs[1][0] =
    cregs[1][1] =
    cregs[1][2] = coefs.d5;
    cregs[1][3] = coefs.d2;

    cregs[2][0] =
    cregs[2][1] =
    cregs[2][2] = coefs.d6;
    cregs[2][3] = coefs.d2;

    cregs[3][0] = 1.f;
    cregs[3][1] = 1.f;
    cregs[3][2] = 1.f;
    cregs[3][3] = 1.f;

    cregs[4][0] = 0; // not in use
    cregs[4][1] = 0; // not in use
    cregs[4][2] = 0; // not in use
    cregs[4][3] = flEffectAlpha;

    return m_pStateManager->SetPixelShaderConstantF(1, cregs[0], 5);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::SetRenderState_AlphaSolidBrush
//
//  Synopsis:
//      Set up the pipeline to do regular SourceOver with a solid brush (brush
//      color supplied in the vertex diffuse color)
//
//------------------------------------------------------------------------------

HRESULT
CD3DRenderState::SetRenderState_AlphaSolidBrush()
{
    HRESULT hr = S_OK;

    IFC(SetAlphaBlendMode(&sc_abmSrcOverPremultiplied));
    IFC(SetPixelShader(NULL));
    IFC(SetVertexShader(NULL));

    IFC(SetTextureStageOperation(0, &sc_tsoDiffuse));
    IFC(m_pStateManager->DisableTextureStage(1));

    // FilterMode: Unused

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::SetRenderState_Texture
//
//  Synopsis:
//      Set up the pipeline to draw from a texture
//
//  blendMode notes:
//      TBM_COPY: for using when texels are fully opaque (or texture has no alpha)
//      TBM_APPLY_PREMULTIPLIED: most common case, assuming the texture has alpha
//                               and it's color values are already multiplied by alpha.
//      TBM_APPLY_VECTOR_ALPHA and TBM_ADD_COLORS:
//        these two guys are for doing the same as TBM_APPLY_PREMULTIPLIED do,
//        but when alpha values are vectors (i.e. alphaR, alphaG and alphaB).
//        Since normal texture can't keep six numbers per pixel, we are forced
//        to use two textures and do blending in two passes. First is controlled
//        by TBM_APPLY_VECTOR_ALPHA that accepts color values as alphas,
//        and the second by TBM_ADD_COLORS.
//
//------------------------------------------------------------------------------

HRESULT
CD3DRenderState::SetRenderState_Texture(
    TextureBlendMode blendMode,
    TextureBlendArgument eBlendArgument,
    MilBitmapInterpolationMode::Enum interpolationMode,
    UINT cMasks
    )
{
    HRESULT hr = S_OK;

    const FilterMode *pfm = NULL;
    const TextureStageOperation *ptso = NULL;
    const AlphaBlendMode *pabm = NULL;

    switch (interpolationMode)
    {
    case MilBitmapInterpolationMode::NearestNeighbor:
        pfm = &sc_fmNearest;
        break;

    case MilBitmapInterpolationMode::Linear:
        pfm = &sc_fmLinear;
        break;

    case MilBitmapInterpolationMode::TriLinear:
        pfm = &sc_fmTriLinear;
        break;

    default:
        AssertMsg(false, "MIL-HW: Unsupported interpolation mode.\n");
        IFC(E_FAIL);
    }

    switch (eBlendArgument)
    {
    case TBA_None:
        ptso = &sc_tsoSelectTexture;
        break;

    case TBA_Diffuse:
        ptso = &sc_tsoTextureXCurrentRGB;
        break;

    case TBA_Specular:
        ptso = &sc_tsoTextureXSpecularRGB;
        break;

    default:
        AssertMsg(false, "MIL_HW: Unsupported diffuse blend mode.\n");
    }

    switch(blendMode)
    {
    case TBM_COPY:
        pabm = &sc_abmSrcCopy;
        break;

    case TBM_APPLY_VECTOR_ALPHA:
        pabm = &sc_abmSrcVectorAlpha;
        break;

    case TBM_ADD_COLORS:
        pabm = &sc_abmAddSourceColor;
        break;

    default:
        Assert(blendMode == TBM_DEFAULT);
        pabm = &sc_abmSrcOverPremultiplied;
        break;

    }

    IFC(SetPixelShader(NULL));
    IFC(SetVertexShader(NULL));

    Assert(pfm);
    IFC(SetFilterMode(0, pfm));

    Assert(pabm);
    IFC(SetAlphaBlendMode(pabm));

    Assert(ptso);
    IFC(SetTextureStageOperation(0, ptso));

    IFC(m_pStateManager->SetTextureStageState(
        0,
        D3DTSS_TEXTURETRANSFORMFLAGS,
        D3DTTFF_DISABLE
        ));


    Assert(cMasks + 1 <= m_pDeviceNoRef->GetMaxTextureBlendStages());
    for (UINT i = 1; i <= cMasks; i++)
    {
        // Same filter mode == correct behavior?
        IFC(SetFilterMode(i, pfm));

        IFC(SetTextureStageOperation(
            i,
            &sc_tsoMaskTextureXCurrent
            ));

        IFC(m_pStateManager->SetTextureStageState(
            i,
            D3DTSS_TEXTURETRANSFORMFLAGS,
            D3DTTFF_DISABLE
            ));
    }

    // disabling the color stage of texture stage cMasks+1 will disable all subsequent texture stages
    IFC(m_pStateManager->DisableTextureStage(cMasks+1));
    IFC(m_pStateManager->SetTextureStageStateInline(cMasks+1, D3DTSS_COLOROP, D3DTOP_DISABLE));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::InitAlphaTextures
//
//  Synopsis:
//      choose the format of alpha textures to use, depending on device caps.
//      Initialize pallette table if necessary.
//
//------------------------------------------------------------------------------

HRESULT
CD3DRenderState::InitAlphaTextures()
{
    HRESULT hr = S_OK;

    // If the device supports alpha-only textures, adjust to
    // use them always.
    if (m_pDeviceNoRef->SupportsD3DFMT_A8())
    {
        m_alphaTextureFormat = D3DFMT_A8;
    }

    // If the device is capable to work with L8 textures, go this way.
    // Pixel shaders are assumed available.
    else if (m_pDeviceNoRef->SupportsD3DFMT_L8())
    {
        m_alphaTextureFormat = D3DFMT_L8;
    }

    // When the device supports P8 textures, try to do so.
    // P8 can work with or without pixel shaders.
    else if (m_pDeviceNoRef->SupportsD3DFMT_P8())
    {
        m_alphaTextureFormat = D3DFMT_P8;
        IFC( m_pDeviceNoRef->SetLinearPalette() );
    }

    else
    {
        hr = E_FAIL;
    }

Cleanup:
    //no RRETURN, E_FAIL is legal
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::InitPixelShaders
//
//  Synopsis:
//      create an array of pixel shaders
//
//------------------------------------------------------------------------------

HRESULT
CD3DRenderState::InitPixelShaders()
{
    HRESULT hr = S_OK;

    UINT rgResourceIds[PXS_NUM];

    if (m_fDrawTextUsingPS20)
    {
        if (m_alphaTextureFormat == D3DFMT_L8)
        {
            rgResourceIds[PXS_CTSB] = g_PixelShader_Text20L_CTSB_P0;
            rgResourceIds[PXS_GSSB] = g_PixelShader_Text20L_GSSB_P0;
            rgResourceIds[PXS_CTTB] = g_PixelShader_Text20L_CTTB_P0;
            rgResourceIds[PXS_GSTB] = g_PixelShader_Text20L_GSTB_P0;
        }
        else
        {
            rgResourceIds[PXS_CTSB] = g_PixelShader_Text20A_CTSB_P0;
            rgResourceIds[PXS_GSSB] = g_PixelShader_Text20A_GSSB_P0;
            rgResourceIds[PXS_CTTB] = g_PixelShader_Text20A_CTTB_P0;
            rgResourceIds[PXS_GSTB] = g_PixelShader_Text20A_GSTB_P0;
        }
    }
    else
    {
        if (m_alphaTextureFormat == D3DFMT_L8)
        {
            rgResourceIds[PXS_CTSB] = g_PixelShader_Text11L_CTSB_P0;
            rgResourceIds[PXS_GSSB] = g_PixelShader_Text11L_GSSB_P0;
            rgResourceIds[PXS_CTTB] = g_PixelShader_Text11L_CTTB_P0;
            rgResourceIds[PXS_GSTB] = g_PixelShader_Text11L_GSTB_P0;
        }
        else
        {
            rgResourceIds[PXS_CTSB] = g_PixelShader_Text11A_CTSB_P0;
            rgResourceIds[PXS_GSSB] = g_PixelShader_Text11A_GSSB_P0;
            rgResourceIds[PXS_CTTB] = g_PixelShader_Text11A_CTTB_P0;
            rgResourceIds[PXS_GSTB] = g_PixelShader_Text11A_GSTB_P0;
        }
    }

    C_ASSERT(ARRAYSIZE(m_pPixelShaders) == PXS_NUM);
    
    for (int i = 0; i < PXS_NUM; i++)
    {
        IFC(m_pDeviceNoRef->CreatePixelShaderFromResource(
            rgResourceIds[i],
            &m_pPixelShaders[i]
            ));
    }

Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::SetRenderState_Text_ClearType_SolidBrush
//
//  Synopsis:
//      Set up the pipeline to do blend a glyph alpha-mask in clear type mode,
//      using solid brush
//
//------------------------------------------------------------------------------
HRESULT
CD3DRenderState::SetRenderState_Text_ClearType_SolidBrush(
    DWORD dwForegroundColor,
    UINT gammaIndex
)
{
    HRESULT hr = S_OK;

    IFC(SetRenderState(
        D3DRS_DIFFUSEMATERIALSOURCE,
        D3DMCS_COLOR1
        ));

    IFC(SetRenderState(
        D3DRS_SPECULARMATERIALSOURCE,
        D3DMCS_COLOR1
        ));

    IFC(SetPixelShader(m_pPixelShaders[PXS_CTSB]));
    IFC(SetVertexShader(NULL));

    IFC(SetAlphaBlendMode(&sc_abmSrcVectorAlphaWithBlendFactor));
    IFC(SetFilterMode(0, m_pTextFilterMode));

    if (m_fDrawTextUsingPS20)
    {
        //
        // The shader outputs alpha values rather than colors. We pass the brush
        // alpha to the shader in a constant register, and the shader then combines
        // that with the ClearType alphas to produce the 4 final alpha values for
        // the 4 channels (alpha included, so there's an "alpha alpha").
        //
        // The actual brush color is passed to the blend stage as the blend factor.
        // The blend mode used here uses D3DRS_BLENDFACTOR as the source coefficient
        // and (1 - source) as the destination coefficient, giving
        //     output = (BlendFactor)(Source) + (1 - Source)(Destination)
        //            = (Brush)(ShaderAlphas) + (1 - ShaderAlphas)(Destination)
        //
        // So r = (r_brush)(alpha_r) + (1 - alpha_r)(r_destination)
        //    g = (g_brush)(alpha_g) + (1 - alpha_g)(g_destination)
        //    b = (b_brush)(alpha_b) + (1 - alpha_b)(b_destination)
        //    a = (a_brush)(alpha_a) + (1 - alpha_a)(a_destination)
        //
        // Note that the equation for "a" is double counting the brush alpha. The
        // brush alpha is already included in the alpha values calculated by the
        // shader (i.e. "alpha_a" already includes "a_brush"). Multiplying the shader
        // output by "a_brush" again would be wrong and would produce a lower (more
        // transparent) value for alpha. So we force the value of "a_brush" passed in
        // through D3DRS_BLENDFACTOR to be 0xFF here to prevent double counting it.
        //
        IFC(SetRenderState(
            D3DRS_BLENDFACTOR,
            dwForegroundColor | 0xFF000000
            ));

        IFC(SetConstantRegisters_SolidBrush_PS20(dwForegroundColor, gammaIndex));
    }
    else
    {
        // We should not be using the ps11 shaders anymore
        Assert(false);
        
        //
        // The fix for the double-counting bug mentioned above was only made to the ps20 shaders,
        // so ps11 remains unchanged
        //
        IFC(SetRenderState(
            D3DRS_BLENDFACTOR,
            dwForegroundColor
            ));

        IFC(SetConstantRegisters_SolidBrush_PS11(dwForegroundColor, gammaIndex));

        IFC(SetFilterMode(1, m_pTextFilterMode));
        IFC(SetFilterMode(2, m_pTextFilterMode));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::SetRenderState_Text_ClearType_TextureBrush
//
//  Synopsis:
//      Set up the pipeline to do blend a glyph alpha-mask in clear type mode,
//      using textured brush
//
//------------------------------------------------------------------------------
HRESULT
CD3DRenderState::SetRenderState_Text_ClearType_TextureBrush(
    UINT gammaIndex,
    float flEffectAlpha
    )
{
    HRESULT hr = S_OK;

    IFC(SetRenderState(
        D3DRS_DIFFUSEMATERIALSOURCE,
        D3DMCS_COLOR1
        ));

    IFC(SetRenderState(
        D3DRS_SPECULARMATERIALSOURCE,
        D3DMCS_COLOR1
        ));

    IFC(SetPixelShader(m_pPixelShaders[PXS_CTTB]));
    IFC(SetVertexShader(NULL));

    IFC(SetConstantRegisters_TexturedBrush(gammaIndex, flEffectAlpha));

    IFC(SetAlphaBlendMode(&sc_abmSrcOverPremultiplied));

    IFC(SetFilterMode(0, &sc_fmLinear));
    IFC(SetFilterMode(1, m_pTextFilterMode));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::SetRenderState_Text_GreyScale_SolidBrush
//
//  Synopsis:
//      Set up the pipeline to do blend a glyph alpha-mask in grey scale, using
//      solid brush
//
//------------------------------------------------------------------------------
HRESULT
CD3DRenderState::SetRenderState_Text_GreyScale_SolidBrush(
    DWORD dwForegroundColor,
    UINT gammaIndex
    )
{
    HRESULT hr = S_OK;

    IFC(SetRenderState(
        D3DRS_DIFFUSEMATERIALSOURCE,
        D3DMCS_COLOR1
        ));

    IFC(SetRenderState(
        D3DRS_SPECULARMATERIALSOURCE,
        D3DMCS_COLOR1
        ));

    IFC(SetPixelShader(m_pPixelShaders[PXS_GSSB]));
    IFC(SetVertexShader(NULL));

    IFC(SetAlphaBlendMode(&sc_abmSrcVectorAlphaWithBlendFactor));
    IFC(SetFilterMode(0, m_pTextFilterMode));

    if (m_fDrawTextUsingPS20)
    {
        //
        // The shader outputs alpha values rather than colors. We pass the brush
        // alpha to the shader in a constant register, and the shader then produces
        // the final alpha value used in all 4 channels.
        //
        // The actual brush color is passed to the blend stage as the blend factor.
        // The blend mode used here uses D3DRS_BLENDFACTOR as the source coefficient
        // and (1 - source) as the destination coefficient, giving
        //     output = (BlendFactor)(Source) + (1 - Source)(Destination)
        //            = (Brush)(ShaderAlphas) + (1 - ShaderAlphas)(Destination)
        //
        // So r = (r_brush)(alpha) + (1 - alpha)(r_destination)
        //    g = (g_brush)(alpha) + (1 - alpha)(g_destination)
        //    b = (b_brush)(alpha) + (1 - alpha)(b_destination)
        //    a = (a_brush)(alpha) + (1 - alpha)(a_destination)
        //
        // Note that the equation for "a" is double counting the brush alpha. The
        // brush alpha is already included in the alpha value calculated by the
        // shader (i.e. "alpha" already includes "a_brush"). Multiplying the shader
        // output by "a_brush" again would be wrong and would produce a lower (more
        // transparent) value for alpha. So we force the value of "a_brush" passed in
        // through D3DRS_BLENDFACTOR to be 0xFF here to prevent double counting it.
        //
        IFC(SetRenderState(
            D3DRS_BLENDFACTOR,
            dwForegroundColor | 0xFF000000
            ));
        IFC(SetConstantRegisters_SolidBrush_PS20(dwForegroundColor, gammaIndex));
    }
    else
    {
        // We should not be using the ps11 shaders anymore
        Assert(false);

        //
        // The fix for the double-counting bug mentioned above was only made to the ps20 shaders,
        // so ps11 remains unchanged
        //
        IFC(SetRenderState(
            D3DRS_BLENDFACTOR,
            dwForegroundColor
            ));
        IFC(SetConstantRegisters_SolidBrush_PS11(dwForegroundColor, gammaIndex));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::SetRenderState_Text_GreyScale_TextureBrush
//
//  Synopsis:
//      Set up the pipeline to do blend a glyph alpha-mask in grey scale mode,
//      using textured brush
//
//------------------------------------------------------------------------------
HRESULT
CD3DRenderState::SetRenderState_Text_GreyScale_TextureBrush(
    UINT gammaIndex,
    float flEffectAlpha
    )
{
    HRESULT hr = S_OK;

    IFC(SetRenderState(
        D3DRS_DIFFUSEMATERIALSOURCE,
        D3DMCS_COLOR1
        ));

    IFC(SetRenderState(
        D3DRS_SPECULARMATERIALSOURCE,
        D3DMCS_COLOR1
        ));

    IFC(SetPixelShader(m_pPixelShaders[PXS_GSTB]));
    IFC(SetVertexShader(NULL));

    IFC(SetAlphaBlendMode(&sc_abmSrcOverPremultiplied));

    static const float constantReg0[4] = {.25f, .5f, .25f, 0};
    IFC(m_pStateManager->SetPixelShaderConstantF(0, constantReg0, 1));

    IFC(SetConstantRegisters_TexturedBrush(gammaIndex, flEffectAlpha));

    IFC(SetFilterMode(0, &sc_fmLinear));
    IFC(SetFilterMode(1, m_pTextFilterMode));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::SetColorChannelRed
//
//  Synopsis:
//      Set up the red-only color write mask to output rasterizer, set up
//      corresponding color mask to pixel shader constant register #0
//
//------------------------------------------------------------------------------
HRESULT
CD3DRenderState::SetColorChannelRed()
{
    HRESULT hr = S_OK;

    Assert(m_pDeviceNoRef->CanMaskColorChannels());

    // We intentionally don't keep current color mask state
    // because the way how it is used assumes that every call
    // will indeed change the state of color write enable mask
    IFC(SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED));

    static const float constantReg0[4] = {1, 0, 0, 0};
    IFC(m_pStateManager->SetPixelShaderConstantF(0, constantReg0, 1));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::SetColorChannelGreen
//
//  Synopsis:
//      Set up the green-only color write mask to output rasterizer, set up
//      corresponding color mask to pixel shader constant register #0
//
//------------------------------------------------------------------------------
HRESULT
CD3DRenderState::SetColorChannelGreen()
{
    HRESULT hr = S_OK;

    Assert(m_pDeviceNoRef->CanMaskColorChannels());

    IFC(SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_GREEN));

    static const float constantReg0[4] = {0, 1, 0, 0};
    IFC(m_pStateManager->SetPixelShaderConstantF(0, constantReg0, 1));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::SetColorChannelBlue
//
//  Synopsis:
//      Set up the blue-only color write mask to output rasterizer, set up
//      corresponding color mask to pixel shader constant register #0
//
//------------------------------------------------------------------------------
HRESULT
CD3DRenderState::SetColorChannelBlue()
{
    HRESULT hr = S_OK;

    Assert(m_pDeviceNoRef->CanMaskColorChannels());

    IFC(SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_BLUE) );

    static const float constantReg0[4] = {0, 0, 1, 0};
    IFC(m_pStateManager->SetPixelShaderConstantF(0, constantReg0, 1));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DRenderState::RestoreColorChannels
//
//  Synopsis:
//      Reset the color write mask to default state
//
//------------------------------------------------------------------------------
HRESULT
CD3DRenderState::RestoreColorChannels()
{
    HRESULT hr = S_OK;

    Assert(m_pDeviceNoRef->CanMaskColorChannels());
    IFC(SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALL));

Cleanup:
    RRETURN(hr);
}





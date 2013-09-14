/*
 * Copyright 2010 Christian Costa
 * Copyright 2011 Rico Schأ¼ller
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"
#define NONAMELESSUNION
#include "wine/debug.h"
#include "wine/unicode.h"

#include "windef.h"
#include "wingdi.h"
#include "d3dx9_36_private.h"

/* Constants for special INT/FLOAT conversation */
#define INT_FLOAT_MULTI 255.0f
#define INT_FLOAT_MULTI_INVERSE (1/INT_FLOAT_MULTI)

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

enum STATE_CLASS
{
    SC_LIGHTENABLE,
    SC_FVF,
    SC_LIGHT,
    SC_MATERIAL,
    SC_NPATCHMODE,
    SC_PIXELSHADER,
    SC_RENDERSTATE,
    SC_SETSAMPLER,
    SC_SAMPLERSTATE,
    SC_TEXTURE,
    SC_TEXTURESTAGE,
    SC_TRANSFORM,
    SC_VERTEXSHADER,
    SC_SHADERCONST,
    SC_UNKNOWN,
};

enum MATERIAL_TYPE
{
    MT_DIFFUSE,
    MT_AMBIENT,
    MT_SPECULAR,
    MT_EMISSIVE,
    MT_POWER,
};

enum LIGHT_TYPE
{
    LT_TYPE,
    LT_DIFFUSE,
    LT_SPECULAR,
    LT_AMBIENT,
    LT_POSITION,
    LT_DIRECTION,
    LT_RANGE,
    LT_FALLOFF,
    LT_ATTENUATION0,
    LT_ATTENUATION1,
    LT_ATTENUATION2,
    LT_THETA,
    LT_PHI,
};

enum SHADER_CONSTANT_TYPE
{
    SCT_VSFLOAT,
    SCT_VSBOOL,
    SCT_VSINT,
    SCT_PSFLOAT,
    SCT_PSBOOL,
    SCT_PSINT,
};

enum STATE_TYPE
{
    ST_CONSTANT,
    ST_PARAMETER,
    ST_FXLC,
};

struct d3dx_parameter
{
    char *name;
    char *semantic;
    void *data;
    D3DXPARAMETER_CLASS class;
    D3DXPARAMETER_TYPE  type;
    UINT rows;
    UINT columns;
    UINT element_count;
    UINT annotation_count;
    UINT member_count;
    DWORD flags;
    UINT bytes;

    struct d3dx_parameter *annotations;
    struct d3dx_parameter *members;
};

struct d3dx_state
{
    UINT operation;
    UINT index;
    enum STATE_TYPE type;
    struct d3dx_parameter *parameter;
};

struct d3dx_sampler
{
    UINT state_count;
    struct d3dx_state *states;
};

struct d3dx_pass
{
    char *name;
    UINT state_count;
    UINT annotation_count;

    struct d3dx_state *states;
    struct d3dx_parameter *annotations;
};

struct d3dx_technique
{
    char *name;
    UINT pass_count;
    UINT annotation_count;

    struct d3dx_parameter *annotations;
    struct d3dx_pass *passes;
};

struct d3dx9_base_effect
{
    struct ID3DXEffectImpl *effect;

    UINT parameter_count;
    UINT technique_count;

    struct d3dx_parameter *parameters;
    struct d3dx_technique *techniques;
};

struct ID3DXEffectImpl
{
    ID3DXEffect ID3DXEffect_iface;
    LONG ref;

    struct d3dx9_base_effect base_effect;

    struct ID3DXEffectStateManager *manager;
    struct IDirect3DDevice9 *device;
    struct ID3DXEffectPool *pool;
    struct d3dx_technique *active_technique;
    struct d3dx_pass *active_pass;
    BOOL started;
    DWORD flags;
};

struct ID3DXEffectCompilerImpl
{
    ID3DXEffectCompiler ID3DXEffectCompiler_iface;
    LONG ref;

    struct d3dx9_base_effect base_effect;
};

static struct d3dx_parameter *get_parameter_by_name(struct d3dx9_base_effect *base,
        struct d3dx_parameter *parameter, const char *name);
static struct d3dx_parameter *get_annotation_by_name(UINT count, struct d3dx_parameter *parameters,
        const char *name);
static HRESULT d3dx9_parse_state(struct d3dx_state *state, const char *data, const char **ptr, D3DXHANDLE *objects);
static void free_parameter_state(struct d3dx_parameter *param, BOOL element, BOOL child, enum STATE_TYPE st);

static const struct
{
    enum STATE_CLASS class;
    UINT op;
    const char *name;
}
state_table[] =
{
    /* Render sates */
    {SC_RENDERSTATE, D3DRS_ZENABLE, "D3DRS_ZENABLE"}, /* 0x0 */
    {SC_RENDERSTATE, D3DRS_FILLMODE, "D3DRS_FILLMODE"},
    {SC_RENDERSTATE, D3DRS_SHADEMODE, "D3DRS_SHADEMODE"},
    {SC_RENDERSTATE, D3DRS_ZWRITEENABLE, "D3DRS_ZWRITEENABLE"},
    {SC_RENDERSTATE, D3DRS_ALPHATESTENABLE, "D3DRS_ALPHATESTENABLE"},
    {SC_RENDERSTATE, D3DRS_LASTPIXEL, "D3DRS_LASTPIXEL"},
    {SC_RENDERSTATE, D3DRS_SRCBLEND, "D3DRS_SRCBLEND"},
    {SC_RENDERSTATE, D3DRS_DESTBLEND, "D3DRS_DESTBLEND"},
    {SC_RENDERSTATE, D3DRS_CULLMODE, "D3DRS_CULLMODE"},
    {SC_RENDERSTATE, D3DRS_ZFUNC, "D3DRS_ZFUNC"},
    {SC_RENDERSTATE, D3DRS_ALPHAREF, "D3DRS_ALPHAREF"},
    {SC_RENDERSTATE, D3DRS_ALPHAFUNC, "D3DRS_ALPHAFUNC"},
    {SC_RENDERSTATE, D3DRS_DITHERENABLE, "D3DRS_DITHERENABLE"},
    {SC_RENDERSTATE, D3DRS_ALPHABLENDENABLE, "D3DRS_ALPHABLENDENABLE"},
    {SC_RENDERSTATE, D3DRS_FOGENABLE, "D3DRS_FOGENABLE"},
    {SC_RENDERSTATE, D3DRS_SPECULARENABLE, "D3DRS_SPECULARENABLE"},
    {SC_RENDERSTATE, D3DRS_FOGCOLOR, "D3DRS_FOGCOLOR"}, /* 0x10 */
    {SC_RENDERSTATE, D3DRS_FOGTABLEMODE, "D3DRS_FOGTABLEMODE"},
    {SC_RENDERSTATE, D3DRS_FOGSTART, "D3DRS_FOGSTART"},
    {SC_RENDERSTATE, D3DRS_FOGEND, "D3DRS_FOGEND"},
    {SC_RENDERSTATE, D3DRS_FOGDENSITY, "D3DRS_FOGDENSITY"},
    {SC_RENDERSTATE, D3DRS_RANGEFOGENABLE, "D3DRS_RANGEFOGENABLE"},
    {SC_RENDERSTATE, D3DRS_STENCILENABLE, "D3DRS_STENCILENABLE"},
    {SC_RENDERSTATE, D3DRS_STENCILFAIL, "D3DRS_STENCILFAIL"},
    {SC_RENDERSTATE, D3DRS_STENCILZFAIL, "D3DRS_STENCILZFAIL"},
    {SC_RENDERSTATE, D3DRS_STENCILPASS, "D3DRS_STENCILPASS"},
    {SC_RENDERSTATE, D3DRS_STENCILFUNC, "D3DRS_STENCILFUNC"},
    {SC_RENDERSTATE, D3DRS_STENCILREF, "D3DRS_STENCILREF"},
    {SC_RENDERSTATE, D3DRS_STENCILMASK, "D3DRS_STENCILMASK"},
    {SC_RENDERSTATE, D3DRS_STENCILWRITEMASK, "D3DRS_STENCILWRITEMASK"},
    {SC_RENDERSTATE, D3DRS_TEXTUREFACTOR, "D3DRS_TEXTUREFACTOR"},
    {SC_RENDERSTATE, D3DRS_WRAP0, "D3DRS_WRAP0"},
    {SC_RENDERSTATE, D3DRS_WRAP1, "D3DRS_WRAP1"}, /* 0x20 */
    {SC_RENDERSTATE, D3DRS_WRAP2, "D3DRS_WRAP2"},
    {SC_RENDERSTATE, D3DRS_WRAP3, "D3DRS_WRAP3"},
    {SC_RENDERSTATE, D3DRS_WRAP4, "D3DRS_WRAP4"},
    {SC_RENDERSTATE, D3DRS_WRAP5, "D3DRS_WRAP5"},
    {SC_RENDERSTATE, D3DRS_WRAP6, "D3DRS_WRAP6"},
    {SC_RENDERSTATE, D3DRS_WRAP7, "D3DRS_WRAP7"},
    {SC_RENDERSTATE, D3DRS_WRAP8, "D3DRS_WRAP8"},
    {SC_RENDERSTATE, D3DRS_WRAP9, "D3DRS_WRAP9"},
    {SC_RENDERSTATE, D3DRS_WRAP10, "D3DRS_WRAP10"},
    {SC_RENDERSTATE, D3DRS_WRAP11, "D3DRS_WRAP11"},
    {SC_RENDERSTATE, D3DRS_WRAP12, "D3DRS_WRAP12"},
    {SC_RENDERSTATE, D3DRS_WRAP13, "D3DRS_WRAP13"},
    {SC_RENDERSTATE, D3DRS_WRAP14, "D3DRS_WRAP14"},
    {SC_RENDERSTATE, D3DRS_WRAP15, "D3DRS_WRAP15"},
    {SC_RENDERSTATE, D3DRS_CLIPPING, "D3DRS_CLIPPING"},
    {SC_RENDERSTATE, D3DRS_LIGHTING, "D3DRS_LIGHTING"}, /* 0x30 */
    {SC_RENDERSTATE, D3DRS_AMBIENT, "D3DRS_AMBIENT"},
    {SC_RENDERSTATE, D3DRS_FOGVERTEXMODE, "D3DRS_FOGVERTEXMODE"},
    {SC_RENDERSTATE, D3DRS_COLORVERTEX, "D3DRS_COLORVERTEX"},
    {SC_RENDERSTATE, D3DRS_LOCALVIEWER, "D3DRS_LOCALVIEWER"},
    {SC_RENDERSTATE, D3DRS_NORMALIZENORMALS, "D3DRS_NORMALIZENORMALS"},
    {SC_RENDERSTATE, D3DRS_DIFFUSEMATERIALSOURCE, "D3DRS_DIFFUSEMATERIALSOURCE"},
    {SC_RENDERSTATE, D3DRS_SPECULARMATERIALSOURCE, "D3DRS_SPECULARMATERIALSOURCE"},
    {SC_RENDERSTATE, D3DRS_AMBIENTMATERIALSOURCE, "D3DRS_AMBIENTMATERIALSOURCE"},
    {SC_RENDERSTATE, D3DRS_EMISSIVEMATERIALSOURCE, "D3DRS_EMISSIVEMATERIALSOURCE"},
    {SC_RENDERSTATE, D3DRS_VERTEXBLEND, "D3DRS_VERTEXBLEND"},
    {SC_RENDERSTATE, D3DRS_CLIPPLANEENABLE, "D3DRS_CLIPPLANEENABLE"},
    {SC_RENDERSTATE, D3DRS_POINTSIZE, "D3DRS_POINTSIZE"},
    {SC_RENDERSTATE, D3DRS_POINTSIZE_MIN, "D3DRS_POINTSIZE_MIN"},
    {SC_RENDERSTATE, D3DRS_POINTSIZE_MAX, "D3DRS_POINTSIZE_MAX"},
    {SC_RENDERSTATE, D3DRS_POINTSPRITEENABLE, "D3DRS_POINTSPRITEENABLE"},
    {SC_RENDERSTATE, D3DRS_POINTSCALEENABLE, "D3DRS_POINTSCALEENABLE"}, /* 0x40 */
    {SC_RENDERSTATE, D3DRS_POINTSCALE_A, "D3DRS_POINTSCALE_A"},
    {SC_RENDERSTATE, D3DRS_POINTSCALE_B, "D3DRS_POINTSCALE_B"},
    {SC_RENDERSTATE, D3DRS_POINTSCALE_C, "D3DRS_POINTSCALE_C"},
    {SC_RENDERSTATE, D3DRS_MULTISAMPLEANTIALIAS, "D3DRS_MULTISAMPLEANTIALIAS"},
    {SC_RENDERSTATE, D3DRS_MULTISAMPLEMASK, "D3DRS_MULTISAMPLEMASK"},
    {SC_RENDERSTATE, D3DRS_PATCHEDGESTYLE, "D3DRS_PATCHEDGESTYLE"},
    {SC_RENDERSTATE, D3DRS_DEBUGMONITORTOKEN, "D3DRS_DEBUGMONITORTOKEN"},
    {SC_RENDERSTATE, D3DRS_INDEXEDVERTEXBLENDENABLE, "D3DRS_INDEXEDVERTEXBLENDENABLE"},
    {SC_RENDERSTATE, D3DRS_COLORWRITEENABLE, "D3DRS_COLORWRITEENABLE"},
    {SC_RENDERSTATE, D3DRS_TWEENFACTOR, "D3DRS_TWEENFACTOR"},
    {SC_RENDERSTATE, D3DRS_BLENDOP, "D3DRS_BLENDOP"},
    {SC_RENDERSTATE, D3DRS_POSITIONDEGREE, "D3DRS_POSITIONDEGREE"},
    {SC_RENDERSTATE, D3DRS_NORMALDEGREE, "D3DRS_NORMALDEGREE"},
    {SC_RENDERSTATE, D3DRS_SCISSORTESTENABLE, "D3DRS_SCISSORTESTENABLE"},
    {SC_RENDERSTATE, D3DRS_SLOPESCALEDEPTHBIAS, "D3DRS_SLOPESCALEDEPTHBIAS"},
    {SC_RENDERSTATE, D3DRS_ANTIALIASEDLINEENABLE, "D3DRS_ANTIALIASEDLINEENABLE"}, /* 0x50 */
    {SC_RENDERSTATE, D3DRS_MINTESSELLATIONLEVEL, "D3DRS_MINTESSELLATIONLEVEL"},
    {SC_RENDERSTATE, D3DRS_MAXTESSELLATIONLEVEL, "D3DRS_MAXTESSELLATIONLEVEL"},
    {SC_RENDERSTATE, D3DRS_ADAPTIVETESS_X, "D3DRS_ADAPTIVETESS_X"},
    {SC_RENDERSTATE, D3DRS_ADAPTIVETESS_Y, "D3DRS_ADAPTIVETESS_Y"},
    {SC_RENDERSTATE, D3DRS_ADAPTIVETESS_Z, "D3DRS_ADAPTIVETESS_Z"},
    {SC_RENDERSTATE, D3DRS_ADAPTIVETESS_W, "D3DRS_ADAPTIVETESS_W"},
    {SC_RENDERSTATE, D3DRS_ENABLEADAPTIVETESSELLATION, "D3DRS_ENABLEADAPTIVETESSELLATION"},
    {SC_RENDERSTATE, D3DRS_TWOSIDEDSTENCILMODE, "D3DRS_TWOSIDEDSTENCILMODE"},
    {SC_RENDERSTATE, D3DRS_CCW_STENCILFAIL, "D3DRS_CCW_STENCILFAIL"},
    {SC_RENDERSTATE, D3DRS_CCW_STENCILZFAIL, "D3DRS_CCW_STENCILZFAIL"},
    {SC_RENDERSTATE, D3DRS_CCW_STENCILPASS, "D3DRS_CCW_STENCILPASS"},
    {SC_RENDERSTATE, D3DRS_CCW_STENCILFUNC, "D3DRS_CCW_STENCILFUNC"},
    {SC_RENDERSTATE, D3DRS_COLORWRITEENABLE1, "D3DRS_COLORWRITEENABLE1"},
    {SC_RENDERSTATE, D3DRS_COLORWRITEENABLE2, "D3DRS_COLORWRITEENABLE2"},
    {SC_RENDERSTATE, D3DRS_COLORWRITEENABLE3, "D3DRS_COLORWRITEENABLE3"},
    {SC_RENDERSTATE, D3DRS_BLENDFACTOR, "D3DRS_BLENDFACTOR"}, /* 0x60 */
    {SC_RENDERSTATE, D3DRS_SRGBWRITEENABLE, "D3DRS_SRGBWRITEENABLE"},
    {SC_RENDERSTATE, D3DRS_DEPTHBIAS, "D3DRS_DEPTHBIAS"},
    {SC_RENDERSTATE, D3DRS_SEPARATEALPHABLENDENABLE, "D3DRS_SEPARATEALPHABLENDENABLE"},
    {SC_RENDERSTATE, D3DRS_SRCBLENDALPHA, "D3DRS_SRCBLENDALPHA"},
    {SC_RENDERSTATE, D3DRS_DESTBLENDALPHA, "D3DRS_DESTBLENDALPHA"},
    {SC_RENDERSTATE, D3DRS_BLENDOPALPHA, "D3DRS_BLENDOPALPHA"},
    /* Texture stages */
    {SC_TEXTURESTAGE, D3DTSS_COLOROP, "D3DTSS_COLOROP"},
    {SC_TEXTURESTAGE, D3DTSS_COLORARG0, "D3DTSS_COLORARG0"},
    {SC_TEXTURESTAGE, D3DTSS_COLORARG1, "D3DTSS_COLORARG1"},
    {SC_TEXTURESTAGE, D3DTSS_COLORARG2, "D3DTSS_COLORARG2"},
    {SC_TEXTURESTAGE, D3DTSS_ALPHAOP, "D3DTSS_ALPHAOP"},
    {SC_TEXTURESTAGE, D3DTSS_ALPHAARG0, "D3DTSS_ALPHAARG0"},
    {SC_TEXTURESTAGE, D3DTSS_ALPHAARG1, "D3DTSS_ALPHAARG1"},
    {SC_TEXTURESTAGE, D3DTSS_ALPHAARG2, "D3DTSS_ALPHAARG2"},
    {SC_TEXTURESTAGE, D3DTSS_RESULTARG, "D3DTSS_RESULTARG"},
    {SC_TEXTURESTAGE, D3DTSS_BUMPENVMAT00, "D3DTSS_BUMPENVMAT00"}, /* 0x70 */
    {SC_TEXTURESTAGE, D3DTSS_BUMPENVMAT01, "D3DTSS_BUMPENVMAT01"},
    {SC_TEXTURESTAGE, D3DTSS_BUMPENVMAT10, "D3DTSS_BUMPENVMAT10"},
    {SC_TEXTURESTAGE, D3DTSS_BUMPENVMAT11, "D3DTSS_BUMPENVMAT11"},
    {SC_TEXTURESTAGE, D3DTSS_TEXCOORDINDEX, "D3DTSS_TEXCOORDINDEX"},
    {SC_TEXTURESTAGE, D3DTSS_BUMPENVLSCALE, "D3DTSS_BUMPENVLSCALE"},
    {SC_TEXTURESTAGE, D3DTSS_BUMPENVLOFFSET, "D3DTSS_BUMPENVLOFFSET"},
    {SC_TEXTURESTAGE, D3DTSS_TEXTURETRANSFORMFLAGS, "D3DTSS_TEXTURETRANSFORMFLAGS"},
    {SC_TEXTURESTAGE, D3DTSS_CONSTANT, "D3DTSS_CONSTANT"},
    /* NPatchMode */
    {SC_NPATCHMODE, 0, "NPatchMode"},
    /* FVF */
    {SC_FVF, 0, "FVF"},
    /* Transform */
    {SC_TRANSFORM, D3DTS_PROJECTION, "D3DTS_PROJECTION"},
    {SC_TRANSFORM, D3DTS_VIEW, "D3DTS_VIEW"},
    {SC_TRANSFORM, D3DTS_WORLD, "D3DTS_WORLD"},
    {SC_TRANSFORM, D3DTS_TEXTURE0, "D3DTS_TEXTURE0"},
    /* Material */
    {SC_MATERIAL, MT_DIFFUSE, "MaterialDiffuse"},
    {SC_MATERIAL, MT_AMBIENT, "MaterialAmbient"}, /* 0x80 */
    {SC_MATERIAL, MT_SPECULAR, "MaterialSpecular"},
    {SC_MATERIAL, MT_EMISSIVE, "MaterialEmissive"},
    {SC_MATERIAL, MT_POWER, "MaterialPower"},
    /* Light */
    {SC_LIGHT, LT_TYPE, "LightType"},
    {SC_LIGHT, LT_DIFFUSE, "LightDiffuse"},
    {SC_LIGHT, LT_SPECULAR, "LightSpecular"},
    {SC_LIGHT, LT_AMBIENT, "LightAmbient"},
    {SC_LIGHT, LT_POSITION, "LightPosition"},
    {SC_LIGHT, LT_DIRECTION, "LightDirection"},
    {SC_LIGHT, LT_RANGE, "LightRange"},
    {SC_LIGHT, LT_FALLOFF, "LightFallOff"},
    {SC_LIGHT, LT_ATTENUATION0, "LightAttenuation0"},
    {SC_LIGHT, LT_ATTENUATION1, "LightAttenuation1"},
    {SC_LIGHT, LT_ATTENUATION2, "LightAttenuation2"},
    {SC_LIGHT, LT_THETA, "LightTheta"},
    {SC_LIGHT, LT_PHI, "LightPhi"}, /* 0x90 */
    /* Ligthenable */
    {SC_LIGHTENABLE, 0, "LightEnable"},
    /* Vertexshader */
    {SC_VERTEXSHADER, 0, "Vertexshader"},
    /* Pixelshader */
    {SC_PIXELSHADER, 0, "Pixelshader"},
    /* Shader constants */
    {SC_SHADERCONST, SCT_VSFLOAT, "VertexShaderConstantF"},
    {SC_SHADERCONST, SCT_VSBOOL, "VertexShaderConstantB"},
    {SC_SHADERCONST, SCT_VSINT, "VertexShaderConstantI"},
    {SC_SHADERCONST, SCT_VSFLOAT, "VertexShaderConstant"},
    {SC_SHADERCONST, SCT_VSFLOAT, "VertexShaderConstant1"},
    {SC_SHADERCONST, SCT_VSFLOAT, "VertexShaderConstant2"},
    {SC_SHADERCONST, SCT_VSFLOAT, "VertexShaderConstant3"},
    {SC_SHADERCONST, SCT_VSFLOAT, "VertexShaderConstant4"},
    {SC_SHADERCONST, SCT_PSFLOAT, "PixelShaderConstantF"},
    {SC_SHADERCONST, SCT_PSBOOL, "PixelShaderConstantB"},
    {SC_SHADERCONST, SCT_PSINT, "PixelShaderConstantI"},
    {SC_SHADERCONST, SCT_PSFLOAT, "PixelShaderConstant"},
    {SC_SHADERCONST, SCT_PSFLOAT, "PixelShaderConstant1"}, /* 0xa0 */
    {SC_SHADERCONST, SCT_PSFLOAT, "PixelShaderConstant2"},
    {SC_SHADERCONST, SCT_PSFLOAT, "PixelShaderConstant3"},
    {SC_SHADERCONST, SCT_PSFLOAT, "PixelShaderConstant4"},
    /* Texture */
    {SC_TEXTURE, 0, "Texture"},
    /* Sampler states */
    {SC_SAMPLERSTATE, D3DSAMP_ADDRESSU, "AddressU"},
    {SC_SAMPLERSTATE, D3DSAMP_ADDRESSV, "AddressV"},
    {SC_SAMPLERSTATE, D3DSAMP_ADDRESSW, "AddressW"},
    {SC_SAMPLERSTATE, D3DSAMP_BORDERCOLOR, "BorderColor"},
    {SC_SAMPLERSTATE, D3DSAMP_MAGFILTER, "MagFilter"},
    {SC_SAMPLERSTATE, D3DSAMP_MINFILTER, "MinFilter"},
    {SC_SAMPLERSTATE, D3DSAMP_MIPFILTER, "MipFilter"},
    {SC_SAMPLERSTATE, D3DSAMP_MIPMAPLODBIAS, "MipMapLodBias"},
    {SC_SAMPLERSTATE, D3DSAMP_MAXMIPLEVEL, "MaxMipLevel"},
    {SC_SAMPLERSTATE, D3DSAMP_MAXANISOTROPY, "MaxAnisotropy"},
    {SC_SAMPLERSTATE, D3DSAMP_SRGBTEXTURE, "SRGBTexture"},
    {SC_SAMPLERSTATE, D3DSAMP_ELEMENTINDEX, "ElementIndex"}, /* 0xb0 */
    {SC_SAMPLERSTATE, D3DSAMP_DMAPOFFSET, "DMAPOffset"},
    /* Set sampler */
    {SC_SETSAMPLER, 0, "Sampler"},
};

static inline void read_dword(const char **ptr, DWORD *d)
{
    memcpy(d, *ptr, sizeof(*d));
    *ptr += sizeof(*d);
}

static void skip_dword_unknown(const char **ptr, unsigned int count)
{
    unsigned int i;
    DWORD d;

    FIXME("Skipping %u unknown DWORDs:\n", count);
    for (i = 0; i < count; ++i)
    {
        read_dword(ptr, &d);
        FIXME("\t0x%08x\n", d);
    }
}

static inline struct d3dx_parameter *get_parameter_struct(D3DXHANDLE handle)
{
    return (struct d3dx_parameter *) handle;
}

static inline D3DXHANDLE get_parameter_handle(struct d3dx_parameter *parameter)
{
    return (D3DXHANDLE) parameter;
}

static inline D3DXHANDLE get_technique_handle(struct d3dx_technique *technique)
{
    return (D3DXHANDLE) technique;
}

static inline D3DXHANDLE get_pass_handle(struct d3dx_pass *pass)
{
    return (D3DXHANDLE) pass;
}

static struct d3dx_technique *get_technique_by_name(struct d3dx9_base_effect *base, const char *name)
{
    UINT i;

    if (!name) return NULL;

    for (i = 0; i < base->technique_count; ++i)
    {
        if (!strcmp(base->techniques[i].name, name))
            return &base->techniques[i];
    }

    return NULL;
}

static struct d3dx_technique *get_valid_technique(struct d3dx9_base_effect *base, D3DXHANDLE technique)
{
    unsigned int i;

    for (i = 0; i < base->technique_count; ++i)
    {
        if (get_technique_handle(&base->techniques[i]) == technique)
            return &base->techniques[i];
    }

    return get_technique_by_name(base, technique);
}

static struct d3dx_pass *get_valid_pass(struct d3dx9_base_effect *base, D3DXHANDLE pass)
{
    unsigned int i, k;

    for (i = 0; i < base->technique_count; ++i)
    {
        struct d3dx_technique *technique = &base->techniques[i];

        for (k = 0; k < technique->pass_count; ++k)
        {
            if (get_pass_handle(&technique->passes[k]) == pass)
                return &technique->passes[k];
        }
    }

    return NULL;
}

static struct d3dx_parameter *get_valid_sub_parameter(struct d3dx_parameter *param, D3DXHANDLE parameter)
{
    unsigned int i, count;
    struct d3dx_parameter *p;

    for (i = 0; i < param->annotation_count; ++i)
    {
        if (get_parameter_handle(&param->annotations[i]) == parameter)
            return &param->annotations[i];

        p = get_valid_sub_parameter(&param->annotations[i], parameter);
        if (p) return p;
    }

    count = param->element_count ? param->element_count : param->member_count;
    for (i = 0; i < count; ++i)
    {
        if (get_parameter_handle(&param->members[i]) == parameter)
            return &param->members[i];

        p = get_valid_sub_parameter(&param->members[i], parameter);
        if (p) return p;
    }

    return NULL;
}

static struct d3dx_parameter *get_valid_parameter(struct d3dx9_base_effect *base, D3DXHANDLE parameter)
{
    unsigned int i, k, m;
    struct d3dx_parameter *p;

    for (i = 0; i < base->parameter_count; ++i)
    {
        if (get_parameter_handle(&base->parameters[i]) == parameter)
            return &base->parameters[i];

        p = get_valid_sub_parameter(&base->parameters[i], parameter);
        if (p) return p;
    }

    for (i = 0; i < base->technique_count; ++i)
    {
        struct d3dx_technique *technique = &base->techniques[i];

        for (k = 0; k < technique->pass_count; ++k)
        {
            struct d3dx_pass *pass = &technique->passes[k];

            for (m = 0; m < pass->annotation_count; ++m)
            {
                if (get_parameter_handle(&pass->annotations[m]) == parameter)
                    return &pass->annotations[m];

                p = get_valid_sub_parameter(&pass->annotations[m], parameter);
                if (p) return p;
            }
        }

        for (k = 0; k < technique->annotation_count; ++k)
        {
            if (get_parameter_handle(&technique->annotations[k]) == parameter)
                return &technique->annotations[k];

            p = get_valid_sub_parameter(&technique->annotations[k], parameter);
            if (p) return p;
        }
    }

    return get_parameter_by_name(base, NULL, parameter);
}

static void free_state(struct d3dx_state *state)
{
    free_parameter_state(state->parameter, FALSE, FALSE, state->type);
}

static void free_sampler(struct d3dx_sampler *sampler)
{
    UINT i;

    for (i = 0; i < sampler->state_count; ++i)
    {
        free_state(&sampler->states[i]);
    }
    HeapFree(GetProcessHeap(), 0, sampler->states);
}

static void free_parameter(struct d3dx_parameter *param, BOOL element, BOOL child)
{
    free_parameter_state(param, element, child, ST_CONSTANT);
}

static void free_parameter_state(struct d3dx_parameter *param, BOOL element, BOOL child, enum STATE_TYPE st)
{
    unsigned int i;

    TRACE("Free parameter %p, name %s, type %s, child %s, state_type %x\n", param, param->name,
            debug_d3dxparameter_type(param->type), child ? "yes" : "no", st);

    if (!param)
        return;

    if (param->annotations)
    {
        for (i = 0; i < param->annotation_count; ++i)
            free_parameter(&param->annotations[i], FALSE, FALSE);
        HeapFree(GetProcessHeap(), 0, param->annotations);
        param->annotations = NULL;
    }

    if (param->members)
    {
        unsigned int count = param->element_count ? param->element_count : param->member_count;

        for (i = 0; i < count; ++i)
            free_parameter(&param->members[i], param->element_count != 0, TRUE);
        HeapFree(GetProcessHeap(), 0, param->members);
        param->members = NULL;
    }

    if (param->class == D3DXPC_OBJECT && !param->element_count)
    {
        switch (param->type)
        {
            case D3DXPT_STRING:
                HeapFree(GetProcessHeap(), 0, *(LPSTR *)param->data);
                if (!child) HeapFree(GetProcessHeap(), 0, param->data);
                break;

            case D3DXPT_TEXTURE:
            case D3DXPT_TEXTURE1D:
            case D3DXPT_TEXTURE2D:
            case D3DXPT_TEXTURE3D:
            case D3DXPT_TEXTURECUBE:
            case D3DXPT_PIXELSHADER:
            case D3DXPT_VERTEXSHADER:
                if (st == ST_CONSTANT)
                {
                    if (*(IUnknown **)param->data) IUnknown_Release(*(IUnknown **)param->data);
                }
                else
                {
                    HeapFree(GetProcessHeap(), 0, *(LPSTR *)param->data);
                }
                if (!child) HeapFree(GetProcessHeap(), 0, param->data);
                break;

            case D3DXPT_SAMPLER:
            case D3DXPT_SAMPLER1D:
            case D3DXPT_SAMPLER2D:
            case D3DXPT_SAMPLER3D:
            case D3DXPT_SAMPLERCUBE:
                if (st == ST_CONSTANT)
                {
                    free_sampler((struct d3dx_sampler *)param->data);
                }
                else
                {
                    HeapFree(GetProcessHeap(), 0, *(LPSTR *)param->data);
                }
                /* samplers have always own data, so free that */
                HeapFree(GetProcessHeap(), 0, param->data);
                break;

            default:
                FIXME("Unhandled type %s\n", debug_d3dxparameter_type(param->type));
                break;
        }
    }
    else
    {
        if (!child)
        {
            if (st != ST_CONSTANT)
            {
                HeapFree(GetProcessHeap(), 0, *(LPSTR *)param->data);
            }
            HeapFree(GetProcessHeap(), 0, param->data);
        }
    }

    /* only the parent has to release name and semantic */
    if (!element)
    {
        HeapFree(GetProcessHeap(), 0, param->name);
        HeapFree(GetProcessHeap(), 0, param->semantic);
    }
}

static void free_pass(struct d3dx_pass *pass)
{
    unsigned int i;

    TRACE("Free pass %p\n", pass);

    if (!pass)
        return;

    if (pass->annotations)
    {
        for (i = 0; i < pass->annotation_count; ++i)
            free_parameter(&pass->annotations[i], FALSE, FALSE);
        HeapFree(GetProcessHeap(), 0, pass->annotations);
        pass->annotations = NULL;
    }

    if (pass->states)
    {
        for (i = 0; i < pass->state_count; ++i)
            free_state(&pass->states[i]);
        HeapFree(GetProcessHeap(), 0, pass->states);
        pass->states = NULL;
    }

    HeapFree(GetProcessHeap(), 0, pass->name);
    pass->name = NULL;
}

static void free_technique(struct d3dx_technique *technique)
{
    unsigned int i;

    TRACE("Free technique %p\n", technique);

    if (!technique)
        return;

    if (technique->annotations)
    {
        for (i = 0; i < technique->annotation_count; ++i)
            free_parameter(&technique->annotations[i], FALSE, FALSE);
        HeapFree(GetProcessHeap(), 0, technique->annotations);
        technique->annotations = NULL;
    }

    if (technique->passes)
    {
        for (i = 0; i < technique->pass_count; ++i)
            free_pass(&technique->passes[i]);
        HeapFree(GetProcessHeap(), 0, technique->passes);
        technique->passes = NULL;
    }

    HeapFree(GetProcessHeap(), 0, technique->name);
    technique->name = NULL;
}

static void d3dx9_base_effect_cleanup(struct d3dx9_base_effect *base)
{
    unsigned int i;

    TRACE("base %p.\n", base);

    if (base->parameters)
    {
        for (i = 0; i < base->parameter_count; ++i)
            free_parameter(&base->parameters[i], FALSE, FALSE);
        HeapFree(GetProcessHeap(), 0, base->parameters);
        base->parameters = NULL;
    }

    if (base->techniques)
    {
        for (i = 0; i < base->technique_count; ++i)
            free_technique(&base->techniques[i]);
        HeapFree(GetProcessHeap(), 0, base->techniques);
        base->techniques = NULL;
    }
}

static void free_effect(struct ID3DXEffectImpl *effect)
{
    TRACE("Free effect %p\n", effect);

    d3dx9_base_effect_cleanup(&effect->base_effect);

    if (effect->pool)
    {
        effect->pool->lpVtbl->Release(effect->pool);
    }

    if (effect->manager)
    {
        IUnknown_Release(effect->manager);
    }

    IDirect3DDevice9_Release(effect->device);
}

static void free_effect_compiler(struct ID3DXEffectCompilerImpl *compiler)
{
    TRACE("Free effect compiler %p\n", compiler);

    d3dx9_base_effect_cleanup(&compiler->base_effect);
}

static void get_vector(struct d3dx_parameter *param, D3DXVECTOR4 *vector)
{
    UINT i;

    for (i = 0; i < 4; ++i)
    {
        if (i < param->columns)
            set_number((FLOAT *)vector + i, D3DXPT_FLOAT, (DWORD *)param->data + i, param->type);
        else
            ((FLOAT *)vector)[i] = 0.0f;
    }
}

static void set_vector(struct d3dx_parameter *param, CONST D3DXVECTOR4 *vector)
{
    UINT i;

    for (i = 0; i < param->columns; ++i)
    {
        set_number((FLOAT *)param->data + i, param->type, (FLOAT *)vector + i, D3DXPT_FLOAT);
    }
}

static void get_matrix(struct d3dx_parameter *param, D3DXMATRIX *matrix, BOOL transpose)
{
    UINT i, k;

    for (i = 0; i < 4; ++i)
    {
        for (k = 0; k < 4; ++k)
        {
            FLOAT *tmp = transpose ? (FLOAT *)&matrix->u.m[k][i] : (FLOAT *)&matrix->u.m[i][k];

            if ((i < param->rows) && (k < param->columns))
                set_number(tmp, D3DXPT_FLOAT, (DWORD *)param->data + i * param->columns + k, param->type);
            else
                *tmp = 0.0f;
        }
    }
}

static void set_matrix(struct d3dx_parameter *param, const D3DXMATRIX *matrix, BOOL transpose)
{
    UINT i, k;

    for (i = 0; i < param->rows; ++i)
    {
        for (k = 0; k < param->columns; ++k)
        {
            set_number((FLOAT *)param->data + i * param->columns + k, param->type,
                    transpose ? &matrix->u.m[k][i] : &matrix->u.m[i][k], D3DXPT_FLOAT);
        }
    }
}

static struct d3dx_parameter *get_parameter_element_by_name(struct d3dx_parameter *parameter, const char *name)
{
    UINT element;
    struct d3dx_parameter *temp_parameter;
    const char *part;

    TRACE("parameter %p, name %s\n", parameter, debugstr_a(name));

    if (!name || !*name) return NULL;

    element = atoi(name);
    part = strchr(name, ']') + 1;

    /* check for empty [] && element range */
    if ((part - name) > 1 && parameter->element_count > element)
    {
        temp_parameter = &parameter->members[element];

        switch (*part++)
        {
            case '.':
                return get_parameter_by_name(NULL, temp_parameter, part);

            case '@':
                return get_annotation_by_name(temp_parameter->annotation_count, temp_parameter->annotations, part);

            case '\0':
                TRACE("Returning parameter %p\n", temp_parameter);
                return temp_parameter;

            default:
                FIXME("Unhandled case \"%c\"\n", *--part);
                break;
        }
    }

    TRACE("Parameter not found\n");
    return NULL;
}

static struct d3dx_parameter *get_annotation_by_name(UINT count, struct d3dx_parameter *annotations,
        const char *name)
{
    UINT i, length;
    struct d3dx_parameter *temp_parameter;
    const char *part;

    TRACE("count %u, annotations %p, name %s\n", count, annotations, debugstr_a(name));

    if (!name || !*name) return NULL;

    length = strcspn( name, "[.@" );
    part = name + length;

    for (i = 0; i < count; ++i)
    {
        temp_parameter = &annotations[i];

        if (!strcmp(temp_parameter->name, name))
        {
            TRACE("Returning annotation %p\n", temp_parameter);
            return temp_parameter;
        }
        else if (strlen(temp_parameter->name) == length && !strncmp(temp_parameter->name, name, length))
        {
            switch (*part++)
            {
                case '.':
                    return get_parameter_by_name(NULL, temp_parameter, part);

                case '[':
                    return get_parameter_element_by_name(temp_parameter, part);

                default:
                    FIXME("Unhandled case \"%c\"\n", *--part);
                    break;
            }
        }
    }

    TRACE("Annotation not found\n");
    return NULL;
}

static struct d3dx_parameter *get_parameter_by_name(struct d3dx9_base_effect *base,
        struct d3dx_parameter *parameter, const char *name)
{
    UINT i, count, length;
    struct d3dx_parameter *temp_parameter;
    struct d3dx_parameter *parameters;
    const char *part;

    TRACE("base %p, parameter %p, name %s\n", base, parameter, debugstr_a(name));

    if (!name || !*name) return NULL;

    if (!parameter)
    {
        count = base->parameter_count;
        parameters = base->parameters;
    }
    else
    {
        count = parameter->member_count;
        parameters = parameter->members;
    }

    length = strcspn( name, "[.@" );
    part = name + length;

    for (i = 0; i < count; i++)
    {
        temp_parameter = &parameters[i];

        if (!strcmp(temp_parameter->name, name))
        {
            TRACE("Returning parameter %p\n", temp_parameter);
            return temp_parameter;
        }
        else if (strlen(temp_parameter->name) == length && !strncmp(temp_parameter->name, name, length))
        {
            switch (*part++)
            {
                case '.':
                    return get_parameter_by_name(NULL, temp_parameter, part);

                case '@':
                    return get_annotation_by_name(temp_parameter->annotation_count, temp_parameter->annotations, part);

                case '[':
                    return get_parameter_element_by_name(temp_parameter, part);

                default:
                    FIXME("Unhandled case \"%c\"\n", *--part);
                    break;
            }
        }
    }

    TRACE("Parameter not found\n");
    return NULL;
}

static inline DWORD d3dx9_effect_version(DWORD major, DWORD minor)
{
    return (0xfeff0000 | ((major) << 8) | (minor));
}

static HRESULT d3dx9_base_effect_get_desc(struct d3dx9_base_effect *base, D3DXEFFECT_DESC *desc)
{
    if (!desc)
    {
        WARN("Invalid argument specified.\n");
        return D3DERR_INVALIDCALL;
    }

    FIXME("partial stub!\n");

    /* TODO: add creator and function count. */
    desc->Creator = NULL;
    desc->Functions = 0;
    desc->Parameters = base->parameter_count;
    desc->Techniques = base->technique_count;

    return D3D_OK;
}

static HRESULT d3dx9_base_effect_get_parameter_desc(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, D3DXPARAMETER_DESC *desc)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (!desc || !param)
    {
        WARN("Invalid argument specified.\n");
        return D3DERR_INVALIDCALL;
    }

    desc->Name = param->name;
    desc->Semantic = param->semantic;
    desc->Class = param->class;
    desc->Type = param->type;
    desc->Rows = param->rows;
    desc->Columns = param->columns;
    desc->Elements = param->element_count;
    desc->Annotations = param->annotation_count;
    desc->StructMembers = param->member_count;
    desc->Flags = param->flags;
    desc->Bytes = param->bytes;

    return D3D_OK;
}

static HRESULT d3dx9_base_effect_get_technique_desc(struct d3dx9_base_effect *base,
        D3DXHANDLE technique, D3DXTECHNIQUE_DESC *desc)
{
    struct d3dx_technique *tech = technique ? get_valid_technique(base, technique) : &base->techniques[0];

    if (!desc || !tech)
    {
        WARN("Invalid argument specified.\n");
        return D3DERR_INVALIDCALL;
    }

    desc->Name = tech->name;
    desc->Passes = tech->pass_count;
    desc->Annotations = tech->annotation_count;

    return D3D_OK;
}

static HRESULT d3dx9_base_effect_get_pass_desc(struct d3dx9_base_effect *base,
        D3DXHANDLE pass, D3DXPASS_DESC *desc)
{
    struct d3dx_pass *p = get_valid_pass(base, pass);

    if (!desc || !p)
    {
        WARN("Invalid argument specified.\n");
        return D3DERR_INVALIDCALL;
    }

    desc->Name = p->name;
    desc->Annotations = p->annotation_count;

    FIXME("Pixel shader and vertex shader are not supported, yet.\n");
    desc->pVertexShaderFunction = NULL;
    desc->pPixelShaderFunction = NULL;

    return D3D_OK;
}

static HRESULT d3dx9_base_effect_get_function_desc(struct d3dx9_base_effect *base,
        D3DXHANDLE shader, D3DXFUNCTION_DESC *desc)
{
    FIXME("stub!\n");

    return E_NOTIMPL;
}

static D3DXHANDLE d3dx9_base_effect_get_parameter(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, UINT index)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (!parameter)
    {
        if (index < base->parameter_count)
        {
            TRACE("Returning parameter %p.\n", &base->parameters[index]);
            return get_parameter_handle(&base->parameters[index]);
        }
    }
    else
    {
        if (param && !param->element_count && index < param->member_count)
        {
            TRACE("Returning parameter %p.\n", &param->members[index]);
            return get_parameter_handle(&param->members[index]);
        }
    }

    WARN("Invalid argument specified.\n");

    return NULL;
}

static D3DXHANDLE d3dx9_base_effect_get_parameter_by_name(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, const char *name)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);
    D3DXHANDLE handle;

    if (!name)
    {
        handle = get_parameter_handle(param);
        TRACE("Returning parameter %p.\n", handle);
        return handle;
    }

    handle = get_parameter_handle(get_parameter_by_name(base, param, name));
    TRACE("Returning parameter %p.\n", handle);

    return handle;
}

static D3DXHANDLE d3dx9_base_effect_get_parameter_by_semantic(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, const char *semantic)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);
    struct d3dx_parameter *temp_param;
    UINT i;

    if (!parameter)
    {
        for (i = 0; i < base->parameter_count; ++i)
        {
            temp_param = &base->parameters[i];

            if (!temp_param->semantic)
            {
                if (!semantic)
                {
                    TRACE("Returning parameter %p\n", temp_param);
                    return get_parameter_handle(temp_param);
                }
                continue;
            }

            if (!strcasecmp(temp_param->semantic, semantic))
            {
                TRACE("Returning parameter %p\n", temp_param);
                return get_parameter_handle(temp_param);
            }
        }
    }
    else if (param)
    {
        for (i = 0; i < param->member_count; ++i)
        {
            temp_param = &param->members[i];

            if (!temp_param->semantic)
            {
                if (!semantic)
                {
                    TRACE("Returning parameter %p\n", temp_param);
                    return get_parameter_handle(temp_param);
                }
                continue;
            }

            if (!strcasecmp(temp_param->semantic, semantic))
            {
                TRACE("Returning parameter %p\n", temp_param);
                return get_parameter_handle(temp_param);
            }
        }
    }

    WARN("Invalid argument specified\n");

    return NULL;
}

static D3DXHANDLE d3dx9_base_effect_get_parameter_element(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, UINT index)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (!param)
    {
        if (index < base->parameter_count)
        {
            TRACE("Returning parameter %p.\n", &base->parameters[index]);
            return get_parameter_handle(&base->parameters[index]);
        }
    }
    else
    {
        if (index < param->element_count)
        {
            TRACE("Returning parameter %p.\n", &param->members[index]);
            return get_parameter_handle(&param->members[index]);
        }
    }

    WARN("Invalid argument specified\n");

    return NULL;
}

static D3DXHANDLE d3dx9_base_effect_get_technique(struct d3dx9_base_effect *base, UINT index)
{
    if (index >= base->technique_count)
    {
        WARN("Invalid argument specified.\n");
        return NULL;
    }

    TRACE("Returning technique %p.\n", &base->techniques[index]);

    return get_technique_handle(&base->techniques[index]);
}

static D3DXHANDLE d3dx9_base_effect_get_technique_by_name(struct d3dx9_base_effect *base, const char *name)
{
    struct d3dx_technique *tech = get_technique_by_name(base, name);

    if (tech)
    {
        D3DXHANDLE t = get_technique_handle(tech);
        TRACE("Returning technique %p\n", t);
        return t;
    }

    WARN("Invalid argument specified.\n");

    return NULL;
}

static D3DXHANDLE d3dx9_base_effect_get_pass(struct d3dx9_base_effect *base,
        D3DXHANDLE technique, UINT index)
{
    struct d3dx_technique *tech = get_valid_technique(base, technique);

    if (tech && index < tech->pass_count)
    {
        TRACE("Returning pass %p\n", &tech->passes[index]);
        return get_pass_handle(&tech->passes[index]);
    }

    WARN("Invalid argument specified.\n");

    return NULL;
}

static D3DXHANDLE d3dx9_base_effect_get_pass_by_name(struct d3dx9_base_effect *base,
        D3DXHANDLE technique, const char *name)
{
    struct d3dx_technique *tech = get_valid_technique(base, technique);

    if (tech && name)
    {
        unsigned int i;

        for (i = 0; i < tech->pass_count; ++i)
        {
            struct d3dx_pass *pass = &tech->passes[i];

            if (!strcmp(pass->name, name))
            {
                TRACE("Returning pass %p\n", pass);
                return get_pass_handle(pass);
            }
        }
    }

    WARN("Invalid argument specified.\n");

    return NULL;
}

static D3DXHANDLE d3dx9_base_effect_get_function(struct d3dx9_base_effect *base, UINT index)
{
    FIXME("stub!\n");

    return NULL;
}

static D3DXHANDLE d3dx9_base_effect_get_function_by_name(struct d3dx9_base_effect *base, const char *name)
{
    FIXME("stub!\n");

    return NULL;
}

static UINT get_annotation_from_object(struct d3dx9_base_effect *base,
        D3DXHANDLE object, struct d3dx_parameter **annotations)
{
    struct d3dx_parameter *param = get_valid_parameter(base, object);
    struct d3dx_pass *pass = get_valid_pass(base, object);
    struct d3dx_technique *technique = get_valid_technique(base, object);

    if (pass)
    {
        *annotations = pass->annotations;
        return pass->annotation_count;
    }
    else if (technique)
    {
        *annotations = technique->annotations;
        return technique->annotation_count;
    }
    else if (param)
    {
        *annotations = param->annotations;
        return param->annotation_count;
    }
    else
    {
        FIXME("Functions are not handled, yet!\n");
        return 0;
    }
}

static D3DXHANDLE d3dx9_base_effect_get_annotation(struct d3dx9_base_effect *base,
        D3DXHANDLE object, UINT index)
{
    struct d3dx_parameter *annotations = NULL;
    UINT annotation_count = 0;

    annotation_count = get_annotation_from_object(base, object, &annotations);

    if (index < annotation_count)
    {
        TRACE("Returning parameter %p\n", &annotations[index]);
        return get_parameter_handle(&annotations[index]);
    }

    WARN("Invalid argument specified\n");

    return NULL;
}

static D3DXHANDLE d3dx9_base_effect_get_annotation_by_name(struct d3dx9_base_effect *base,
        D3DXHANDLE object, const char *name)
{
    struct d3dx_parameter *annotation = NULL;
    struct d3dx_parameter *annotations = NULL;
    UINT annotation_count = 0;

    if (!name)
    {
        WARN("Invalid argument specified\n");
        return NULL;
    }

    annotation_count = get_annotation_from_object(base, object, &annotations);

    annotation = get_annotation_by_name(annotation_count, annotations, name);
    if (annotation)
    {
        TRACE("Returning parameter %p\n", annotation);
        return get_parameter_handle(annotation);
    }

    WARN("Invalid argument specified\n");

    return NULL;
}

static HRESULT d3dx9_base_effect_set_value(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, const void *data, UINT bytes)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (!param)
    {
        WARN("Invalid parameter %p specified\n", parameter);
        return D3DERR_INVALIDCALL;
    }

    /* samplers don't touch data */
    if (param->class == D3DXPC_OBJECT && (param->type == D3DXPT_SAMPLER
            || param->type == D3DXPT_SAMPLER1D || param->type == D3DXPT_SAMPLER2D
            || param->type == D3DXPT_SAMPLER3D || param->type == D3DXPT_SAMPLERCUBE))
    {
        TRACE("Sampler: returning E_FAIL\n");
        return E_FAIL;
    }

    if (data && param->bytes <= bytes)
    {
        switch (param->type)
        {
            case D3DXPT_VOID:
            case D3DXPT_BOOL:
            case D3DXPT_INT:
            case D3DXPT_FLOAT:
                TRACE("Copy %u bytes\n", param->bytes);
                memcpy(param->data, data, param->bytes);
                break;

            default:
                FIXME("Unhandled type %s\n", debug_d3dxparameter_type(param->type));
                break;
        }

        return D3D_OK;
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_get_value(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, void *data, UINT bytes)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (!param)
    {
        WARN("Invalid parameter %p specified\n", parameter);
        return D3DERR_INVALIDCALL;
    }

    /* samplers don't touch data */
    if (param->class == D3DXPC_OBJECT && (param->type == D3DXPT_SAMPLER
            || param->type == D3DXPT_SAMPLER1D || param->type == D3DXPT_SAMPLER2D
            || param->type == D3DXPT_SAMPLER3D || param->type == D3DXPT_SAMPLERCUBE))
    {
        TRACE("Sampler: returning E_FAIL\n");
        return E_FAIL;
    }

    if (data && param->bytes <= bytes)
    {
        TRACE("Type %s\n", debug_d3dxparameter_type(param->type));

        switch (param->type)
        {
            case D3DXPT_VOID:
            case D3DXPT_BOOL:
            case D3DXPT_INT:
            case D3DXPT_FLOAT:
            case D3DXPT_STRING:
                break;

            case D3DXPT_VERTEXSHADER:
            case D3DXPT_PIXELSHADER:
            case D3DXPT_TEXTURE:
            case D3DXPT_TEXTURE1D:
            case D3DXPT_TEXTURE2D:
            case D3DXPT_TEXTURE3D:
            case D3DXPT_TEXTURECUBE:
            {
                UINT i;

                for (i = 0; i < (param->element_count ? param->element_count : 1); ++i)
                {
                    IUnknown *unk = ((IUnknown **)param->data)[i];
                    if (unk) IUnknown_AddRef(unk);
                }
                break;
            }

            default:
                FIXME("Unhandled type %s\n", debug_d3dxparameter_type(param->type));
                break;
        }

        TRACE("Copy %u bytes\n", param->bytes);
        memcpy(data, param->data, param->bytes);
        return D3D_OK;
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_set_bool(struct d3dx9_base_effect *base, D3DXHANDLE parameter, BOOL b)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (param && !param->element_count && param->rows == 1 && param->columns == 1)
    {
        set_number(param->data, param->type, &b, D3DXPT_BOOL);
        return D3D_OK;
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_get_bool(struct d3dx9_base_effect *base, D3DXHANDLE parameter, BOOL *b)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (b && param && !param->element_count && param->rows == 1 && param->columns == 1)
    {
        set_number(b, D3DXPT_BOOL, param->data, param->type);
        TRACE("Returning %s\n", *b ? "TRUE" : "FALSE");
        return D3D_OK;
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_set_bool_array(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, const BOOL *b, UINT count)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (param)
    {
        UINT i, size = min(count, param->bytes / sizeof(DWORD));

        TRACE("Class %s\n", debug_d3dxparameter_class(param->class));

        switch (param->class)
        {
            case D3DXPC_SCALAR:
            case D3DXPC_VECTOR:
            case D3DXPC_MATRIX_ROWS:
                for (i = 0; i < size; ++i)
                {
                    /* don't crop the input, use D3DXPT_INT instead of D3DXPT_BOOL */
                    set_number((DWORD *)param->data + i, param->type, &b[i], D3DXPT_INT);
                }
                return D3D_OK;

            case D3DXPC_OBJECT:
            case D3DXPC_STRUCT:
                break;

            default:
                FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
                break;
        }
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_get_bool_array(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, BOOL *b, UINT count)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (b && param && (param->class == D3DXPC_SCALAR
            || param->class == D3DXPC_VECTOR
            || param->class == D3DXPC_MATRIX_ROWS
            || param->class == D3DXPC_MATRIX_COLUMNS))
    {
        UINT i, size = min(count, param->bytes / sizeof(DWORD));

        for (i = 0; i < size; ++i)
        {
            set_number(&b[i], D3DXPT_BOOL, (DWORD *)param->data + i, param->type);
        }
        return D3D_OK;
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_set_int(struct d3dx9_base_effect *base, D3DXHANDLE parameter, INT n)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (param && !param->element_count)
    {
        if (param->rows == 1 && param->columns == 1)
        {
            set_number(param->data, param->type, &n, D3DXPT_INT);
            return D3D_OK;
        }

        /*
         * Split the value, if parameter is a vector with dimension 3 or 4.
         */
        if (param->type == D3DXPT_FLOAT &&
            ((param->class == D3DXPC_VECTOR && param->columns != 2) ||
            (param->class == D3DXPC_MATRIX_ROWS && param->rows != 2 && param->columns == 1)))
        {
            TRACE("Vector fixup\n");

            *(FLOAT *)param->data = ((n & 0xff0000) >> 16) * INT_FLOAT_MULTI_INVERSE;
            ((FLOAT *)param->data)[1] = ((n & 0xff00) >> 8) * INT_FLOAT_MULTI_INVERSE;
            ((FLOAT *)param->data)[2] = (n & 0xff) * INT_FLOAT_MULTI_INVERSE;
            if (param->rows * param->columns > 3)
            {
                ((FLOAT *)param->data)[3] = ((n & 0xff000000) >> 24) * INT_FLOAT_MULTI_INVERSE;
            }
            return D3D_OK;
        }
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_get_int(struct d3dx9_base_effect *base, D3DXHANDLE parameter, INT *n)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (n && param && !param->element_count)
    {
        if (param->columns == 1 && param->rows == 1)
        {
            set_number(n, D3DXPT_INT, param->data, param->type);
            TRACE("Returning %i\n", *n);
            return D3D_OK;
        }

        if (param->type == D3DXPT_FLOAT &&
                ((param->class == D3DXPC_VECTOR && param->columns != 2)
                || (param->class == D3DXPC_MATRIX_ROWS && param->rows != 2 && param->columns == 1)))
        {
            TRACE("Vector fixup\n");

            /* all components (3,4) are clamped (0,255) and put in the INT */
            *n = (INT)(min(max(0.0f, *((FLOAT *)param->data + 2)), 1.0f) * INT_FLOAT_MULTI);
            *n += ((INT)(min(max(0.0f, *((FLOAT *)param->data + 1)), 1.0f) * INT_FLOAT_MULTI)) << 8;
            *n += ((INT)(min(max(0.0f, *((FLOAT *)param->data + 0)), 1.0f) * INT_FLOAT_MULTI)) << 16;
            if (param->columns * param->rows > 3)
            {
                *n += ((INT)(min(max(0.0f, *((FLOAT *)param->data + 3)), 1.0f) * INT_FLOAT_MULTI)) << 24;
            }

            TRACE("Returning %i\n", *n);
            return D3D_OK;
        }
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_set_int_array(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, const INT *n, UINT count)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (param)
    {
        UINT i, size = min(count, param->bytes / sizeof(DWORD));

        TRACE("Class %s\n", debug_d3dxparameter_class(param->class));

        switch (param->class)
        {
            case D3DXPC_SCALAR:
            case D3DXPC_VECTOR:
            case D3DXPC_MATRIX_ROWS:
                for (i = 0; i < size; ++i)
                {
                    set_number((DWORD *)param->data + i, param->type, &n[i], D3DXPT_INT);
                }
                return D3D_OK;

            case D3DXPC_OBJECT:
            case D3DXPC_STRUCT:
                break;

            default:
                FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
                break;
        }
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_get_int_array(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, INT *n, UINT count)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (n && param && (param->class == D3DXPC_SCALAR
            || param->class == D3DXPC_VECTOR
            || param->class == D3DXPC_MATRIX_ROWS
            || param->class == D3DXPC_MATRIX_COLUMNS))
    {
        UINT i, size = min(count, param->bytes / sizeof(DWORD));

        for (i = 0; i < size; ++i)
        {
            set_number(&n[i], D3DXPT_INT, (DWORD *)param->data + i, param->type);
        }
        return D3D_OK;
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_set_float(struct d3dx9_base_effect *base, D3DXHANDLE parameter, float f)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (param && !param->element_count && param->rows == 1 && param->columns == 1)
    {
        set_number((DWORD *)param->data, param->type, &f, D3DXPT_FLOAT);
        return D3D_OK;
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_get_float(struct d3dx9_base_effect *base, D3DXHANDLE parameter, float *f)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (f && param && !param->element_count && param->columns == 1 && param->rows == 1)
    {
        set_number(f, D3DXPT_FLOAT, (DWORD *)param->data, param->type);
        TRACE("Returning %f\n", *f);
        return D3D_OK;
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_set_float_array(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, const float *f, UINT count)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (param)
    {
        UINT i, size = min(count, param->bytes / sizeof(DWORD));

        TRACE("Class %s\n", debug_d3dxparameter_class(param->class));

        switch (param->class)
        {
            case D3DXPC_SCALAR:
            case D3DXPC_VECTOR:
            case D3DXPC_MATRIX_ROWS:
                for (i = 0; i < size; ++i)
                {
                    set_number((DWORD *)param->data + i, param->type, &f[i], D3DXPT_FLOAT);
                }
                return D3D_OK;

            case D3DXPC_OBJECT:
            case D3DXPC_STRUCT:
                break;

            default:
                FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
                break;
        }
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_get_float_array(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, float *f, UINT count)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (f && param && (param->class == D3DXPC_SCALAR
            || param->class == D3DXPC_VECTOR
            || param->class == D3DXPC_MATRIX_ROWS
            || param->class == D3DXPC_MATRIX_COLUMNS))
    {
        UINT i, size = min(count, param->bytes / sizeof(DWORD));

        for (i = 0; i < size; ++i)
        {
            set_number(&f[i], D3DXPT_FLOAT, (DWORD *)param->data + i, param->type);
        }
        return D3D_OK;
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_set_vector(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, const D3DXVECTOR4 *vector)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (param && !param->element_count)
    {
        TRACE("Class %s\n", debug_d3dxparameter_class(param->class));

        switch (param->class)
        {
            case D3DXPC_SCALAR:
            case D3DXPC_VECTOR:
                if (param->type == D3DXPT_INT && param->bytes == 4)
                {
                    DWORD tmp;

                    TRACE("INT fixup\n");
                    tmp = (DWORD)(max(min(vector->z, 1.0f), 0.0f) * INT_FLOAT_MULTI);
                    tmp += ((DWORD)(max(min(vector->y, 1.0f), 0.0f) * INT_FLOAT_MULTI)) << 8;
                    tmp += ((DWORD)(max(min(vector->x, 1.0f), 0.0f) * INT_FLOAT_MULTI)) << 16;
                    tmp += ((DWORD)(max(min(vector->w, 1.0f), 0.0f) * INT_FLOAT_MULTI)) << 24;

                    *(INT *)param->data = tmp;
                    return D3D_OK;
                }
                set_vector(param, vector);
                return D3D_OK;

            case D3DXPC_MATRIX_ROWS:
            case D3DXPC_OBJECT:
            case D3DXPC_STRUCT:
                break;

            default:
                FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
                break;
        }
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_get_vector(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, D3DXVECTOR4 *vector)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (vector && param && !param->element_count)
    {
        TRACE("Class %s\n", debug_d3dxparameter_class(param->class));

        switch (param->class)
        {
            case D3DXPC_SCALAR:
            case D3DXPC_VECTOR:
                if (param->type == D3DXPT_INT && param->bytes == 4)
                {
                    TRACE("INT fixup\n");
                    vector->x = (((*(INT *)param->data) & 0xff0000) >> 16) * INT_FLOAT_MULTI_INVERSE;
                    vector->y = (((*(INT *)param->data) & 0xff00) >> 8) * INT_FLOAT_MULTI_INVERSE;
                    vector->z = ((*(INT *)param->data) & 0xff) * INT_FLOAT_MULTI_INVERSE;
                    vector->w = (((*(INT *)param->data) & 0xff000000) >> 24) * INT_FLOAT_MULTI_INVERSE;
                    return D3D_OK;
                }
                get_vector(param, vector);
                return D3D_OK;

            case D3DXPC_MATRIX_ROWS:
            case D3DXPC_OBJECT:
            case D3DXPC_STRUCT:
                break;

            default:
                FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
                break;
        }
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_set_vector_array(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, const D3DXVECTOR4 *vector, UINT count)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (param && param->element_count && param->element_count >= count)
    {
        UINT i;

        TRACE("Class %s\n", debug_d3dxparameter_class(param->class));

        switch (param->class)
        {
            case D3DXPC_VECTOR:
                for (i = 0; i < count; ++i)
                {
                    set_vector(&param->members[i], &vector[i]);
                }
                return D3D_OK;

            case D3DXPC_SCALAR:
            case D3DXPC_MATRIX_ROWS:
            case D3DXPC_OBJECT:
            case D3DXPC_STRUCT:
                break;

            default:
                FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
                break;
        }
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_get_vector_array(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, D3DXVECTOR4 *vector, UINT count)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (!count) return D3D_OK;

    if (vector && param && count <= param->element_count)
    {
        UINT i;

        TRACE("Class %s\n", debug_d3dxparameter_class(param->class));

        switch (param->class)
        {
            case D3DXPC_VECTOR:
                for (i = 0; i < count; ++i)
                {
                    get_vector(&param->members[i], &vector[i]);
                }
                return D3D_OK;

            case D3DXPC_SCALAR:
            case D3DXPC_MATRIX_ROWS:
            case D3DXPC_OBJECT:
            case D3DXPC_STRUCT:
                break;

            default:
                FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
                break;
        }
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_set_matrix(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, const D3DXMATRIX *matrix)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (param && !param->element_count)
    {
        TRACE("Class %s\n", debug_d3dxparameter_class(param->class));

        switch (param->class)
        {
            case D3DXPC_MATRIX_ROWS:
                set_matrix(param, matrix, FALSE);
                return D3D_OK;

            case D3DXPC_SCALAR:
            case D3DXPC_VECTOR:
            case D3DXPC_OBJECT:
            case D3DXPC_STRUCT:
                break;

            default:
                FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
                break;
        }
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_get_matrix(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, D3DXMATRIX *matrix)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (matrix && param && !param->element_count)
    {
        TRACE("Class %s\n", debug_d3dxparameter_class(param->class));

        switch (param->class)
        {
            case D3DXPC_MATRIX_ROWS:
                get_matrix(param, matrix, FALSE);
                return D3D_OK;

            case D3DXPC_SCALAR:
            case D3DXPC_VECTOR:
            case D3DXPC_OBJECT:
            case D3DXPC_STRUCT:
                break;

            default:
                FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
                break;
        }
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_set_matrix_array(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, const D3DXMATRIX *matrix, UINT count)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (param && param->element_count >= count)
    {
        UINT i;

        TRACE("Class %s\n", debug_d3dxparameter_class(param->class));

        switch (param->class)
        {
            case D3DXPC_MATRIX_ROWS:
                for (i = 0; i < count; ++i)
                {
                    set_matrix(&param->members[i], &matrix[i], FALSE);
                }
                return D3D_OK;

            case D3DXPC_SCALAR:
            case D3DXPC_VECTOR:
            case D3DXPC_OBJECT:
            case D3DXPC_STRUCT:
                break;

            default:
                FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
                break;
        }
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_get_matrix_array(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, D3DXMATRIX *matrix, UINT count)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (!count) return D3D_OK;

    if (matrix && param && count <= param->element_count)
    {
        UINT i;

        TRACE("Class %s\n", debug_d3dxparameter_class(param->class));

        switch (param->class)
        {
            case D3DXPC_MATRIX_ROWS:
                for (i = 0; i < count; ++i)
                {
                    get_matrix(&param->members[i], &matrix[i], FALSE);
                }
                return D3D_OK;

            case D3DXPC_SCALAR:
            case D3DXPC_VECTOR:
            case D3DXPC_OBJECT:
            case D3DXPC_STRUCT:
                break;

            default:
                FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
                break;
        }
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_set_matrix_pointer_array(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, const D3DXMATRIX **matrix, UINT count)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (param && count <= param->element_count)
    {
        UINT i;

        switch (param->class)
        {
            case D3DXPC_MATRIX_ROWS:
                for (i = 0; i < count; ++i)
                {
                    set_matrix(&param->members[i], matrix[i], FALSE);
                }
                return D3D_OK;

            case D3DXPC_SCALAR:
            case D3DXPC_VECTOR:
            case D3DXPC_OBJECT:
                break;

            default:
                FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
                break;
        }
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_get_matrix_pointer_array(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, D3DXMATRIX **matrix, UINT count)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (!count) return D3D_OK;

    if (param && matrix && count <= param->element_count)
    {
        UINT i;

        TRACE("Class %s\n", debug_d3dxparameter_class(param->class));

        switch (param->class)
        {
            case D3DXPC_MATRIX_ROWS:
                for (i = 0; i < count; ++i)
                {
                    get_matrix(&param->members[i], matrix[i], FALSE);
                }
                return D3D_OK;

            case D3DXPC_SCALAR:
            case D3DXPC_VECTOR:
            case D3DXPC_OBJECT:
                break;

            default:
                FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
                break;
        }
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_set_matrix_transpose(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, const D3DXMATRIX *matrix)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (param && !param->element_count)
    {
        TRACE("Class %s\n", debug_d3dxparameter_class(param->class));

        switch (param->class)
        {
            case D3DXPC_MATRIX_ROWS:
                set_matrix(param, matrix, TRUE);
                return D3D_OK;

            case D3DXPC_SCALAR:
            case D3DXPC_VECTOR:
            case D3DXPC_OBJECT:
            case D3DXPC_STRUCT:
                break;

            default:
                FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
                break;
        }
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_get_matrix_transpose(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, D3DXMATRIX *matrix)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (matrix && param && !param->element_count)
    {
        TRACE("Class %s\n", debug_d3dxparameter_class(param->class));

        switch (param->class)
        {
            case D3DXPC_SCALAR:
            case D3DXPC_VECTOR:
                get_matrix(param, matrix, FALSE);
                return D3D_OK;

            case D3DXPC_MATRIX_ROWS:
                get_matrix(param, matrix, TRUE);
                return D3D_OK;

            case D3DXPC_OBJECT:
            case D3DXPC_STRUCT:
                break;

            default:
                FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
                break;
        }
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_set_matrix_transpose_array(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, const D3DXMATRIX *matrix, UINT count)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (param && param->element_count >= count)
    {
        UINT i;

        TRACE("Class %s\n", debug_d3dxparameter_class(param->class));

        switch (param->class)
        {
            case D3DXPC_MATRIX_ROWS:
                for (i = 0; i < count; ++i)
                {
                    set_matrix(&param->members[i], &matrix[i], TRUE);
                }
                return D3D_OK;

            case D3DXPC_SCALAR:
            case D3DXPC_VECTOR:
            case D3DXPC_OBJECT:
            case D3DXPC_STRUCT:
                break;

            default:
                FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
                break;
        }
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_get_matrix_transpose_array(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, D3DXMATRIX *matrix, UINT count)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (!count) return D3D_OK;

    if (matrix && param && count <= param->element_count)
    {
        UINT i;

        TRACE("Class %s\n", debug_d3dxparameter_class(param->class));

        switch (param->class)
        {
            case D3DXPC_MATRIX_ROWS:
                for (i = 0; i < count; ++i)
                {
                    get_matrix(&param->members[i], &matrix[i], TRUE);
                }
                return D3D_OK;

            case D3DXPC_SCALAR:
            case D3DXPC_VECTOR:
            case D3DXPC_OBJECT:
            case D3DXPC_STRUCT:
                break;

            default:
                FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
                break;
        }
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_set_matrix_transpose_pointer_array(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, const D3DXMATRIX **matrix, UINT count)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (param && count <= param->element_count)
    {
        UINT i;

        switch (param->class)
        {
            case D3DXPC_MATRIX_ROWS:
                for (i = 0; i < count; ++i)
                {
                    set_matrix(&param->members[i], matrix[i], TRUE);
                }
                return D3D_OK;

            case D3DXPC_SCALAR:
            case D3DXPC_VECTOR:
            case D3DXPC_OBJECT:
                break;

            default:
                FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
                break;
        }
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_get_matrix_transpose_pointer_array(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, D3DXMATRIX **matrix, UINT count)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (!count) return D3D_OK;

    if (matrix && param && count <= param->element_count)
    {
        UINT i;

        TRACE("Class %s\n", debug_d3dxparameter_class(param->class));

        switch (param->class)
        {
            case D3DXPC_MATRIX_ROWS:
                for (i = 0; i < count; ++i)
                {
                    get_matrix(&param->members[i], matrix[i], TRUE);
                }
                return D3D_OK;

            case D3DXPC_SCALAR:
            case D3DXPC_VECTOR:
            case D3DXPC_OBJECT:
                break;

            default:
                FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
                break;
        }
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_set_string(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, const char *string)
{
    FIXME("stub!\n");

    return E_NOTIMPL;
}

static HRESULT d3dx9_base_effect_get_string(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, const char **string)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (string && param && !param->element_count && param->type == D3DXPT_STRING)
    {
        *string = *(LPCSTR *)param->data;
        TRACE("Returning %s\n", debugstr_a(*string));
        return D3D_OK;
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_set_texture(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, struct IDirect3DBaseTexture9 *texture)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (param && !param->element_count &&
            (param->type == D3DXPT_TEXTURE || param->type == D3DXPT_TEXTURE1D
            || param->type == D3DXPT_TEXTURE2D || param->type ==  D3DXPT_TEXTURE3D
            || param->type == D3DXPT_TEXTURECUBE))
    {
        struct IDirect3DBaseTexture9 *oltexture = *(struct IDirect3DBaseTexture9 **)param->data;

        if (texture) IDirect3DBaseTexture9_AddRef(texture);
        if (oltexture) IDirect3DBaseTexture9_Release(oltexture);

        *(struct IDirect3DBaseTexture9 **)param->data = texture;

        return D3D_OK;
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_get_texture(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, struct IDirect3DBaseTexture9 **texture)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (texture && param && !param->element_count &&
            (param->type == D3DXPT_TEXTURE || param->type == D3DXPT_TEXTURE1D
            || param->type == D3DXPT_TEXTURE2D || param->type ==  D3DXPT_TEXTURE3D
            || param->type == D3DXPT_TEXTURECUBE))
    {
        *texture = *(struct IDirect3DBaseTexture9 **)param->data;
        if (*texture) IDirect3DBaseTexture9_AddRef(*texture);
        TRACE("Returning %p\n", *texture);
        return D3D_OK;
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_get_pixel_shader(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, struct IDirect3DPixelShader9 **shader)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (shader && param && !param->element_count && param->type == D3DXPT_PIXELSHADER)
    {
        if ((*shader = *(struct IDirect3DPixelShader9 **)param->data))
            IDirect3DPixelShader9_AddRef(*shader);
        TRACE("Returning %p.\n", *shader);
        return D3D_OK;
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_get_vertex_shader(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, struct IDirect3DVertexShader9 **shader)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (shader && param && !param->element_count && param->type == D3DXPT_VERTEXSHADER)
    {
        if ((*shader = *(struct IDirect3DVertexShader9 **)param->data))
            IDirect3DVertexShader9_AddRef(*shader);
        TRACE("Returning %p.\n", *shader);
        return D3D_OK;
    }

    WARN("Invalid argument specified\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_set_array_range(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, UINT start, UINT end)
{
    FIXME("stub!\n");

    return E_NOTIMPL;
}

static inline struct ID3DXEffectImpl *impl_from_ID3DXEffect(ID3DXEffect *iface)
{
    return CONTAINING_RECORD(iface, struct ID3DXEffectImpl, ID3DXEffect_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI ID3DXEffectImpl_QueryInterface(ID3DXEffect *iface, REFIID riid, void **object)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_ID3DXEffect))
    {
        iface->lpVtbl->AddRef(iface);
        *object = iface;
        return S_OK;
    }

    ERR("Interface %s not found\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXEffectImpl_AddRef(ID3DXEffect *iface)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    TRACE("(%p)->(): AddRef from %u\n", This, This->ref);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ID3DXEffectImpl_Release(ID3DXEffect *iface)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): Release from %u\n", This, ref + 1);

    if (!ref)
    {
        free_effect(This);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** ID3DXBaseEffect methods ***/
static HRESULT WINAPI ID3DXEffectImpl_GetDesc(ID3DXEffect *iface, D3DXEFFECT_DESC *desc)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    return d3dx9_base_effect_get_desc(&effect->base_effect, desc);
}

static HRESULT WINAPI ID3DXEffectImpl_GetParameterDesc(ID3DXEffect *iface,
        D3DXHANDLE parameter, D3DXPARAMETER_DESC *desc)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, desc %p.\n", iface, parameter, desc);

    return d3dx9_base_effect_get_parameter_desc(&effect->base_effect, parameter, desc);
}

static HRESULT WINAPI ID3DXEffectImpl_GetTechniqueDesc(ID3DXEffect *iface,
        D3DXHANDLE technique, D3DXTECHNIQUE_DESC *desc)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, technique %p, desc %p.\n", iface, technique, desc);

    return d3dx9_base_effect_get_technique_desc(&effect->base_effect, technique, desc);
}

static HRESULT WINAPI ID3DXEffectImpl_GetPassDesc(ID3DXEffect *iface, D3DXHANDLE pass, D3DXPASS_DESC *desc)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, pass %p, desc %p.\n", iface, pass, desc);

    return d3dx9_base_effect_get_pass_desc(&effect->base_effect, pass, desc);
}

static HRESULT WINAPI ID3DXEffectImpl_GetFunctionDesc(ID3DXEffect *iface, D3DXHANDLE shader, D3DXFUNCTION_DESC *desc)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, shader %p, desc %p.\n", iface, shader, desc);

    return d3dx9_base_effect_get_function_desc(&effect->base_effect, shader, desc);
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetParameter(ID3DXEffect *iface, D3DXHANDLE parameter, UINT index)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, index %u.\n", iface, parameter, index);

    return d3dx9_base_effect_get_parameter(&effect->base_effect, parameter, index);
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetParameterByName(ID3DXEffect *iface,
        D3DXHANDLE parameter, const char *name)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, name %s.\n", iface, parameter, debugstr_a(name));

    return d3dx9_base_effect_get_parameter_by_name(&effect->base_effect, parameter, name);
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetParameterBySemantic(ID3DXEffect *iface,
        D3DXHANDLE parameter, const char *semantic)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, semantic %s.\n", iface, parameter, debugstr_a(semantic));

    return d3dx9_base_effect_get_parameter_by_semantic(&effect->base_effect, parameter, semantic);
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetParameterElement(ID3DXEffect *iface, D3DXHANDLE parameter, UINT index)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, index %u.\n", iface, parameter, index);

    return d3dx9_base_effect_get_parameter_element(&effect->base_effect, parameter, index);
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetTechnique(ID3DXEffect *iface, UINT index)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, index %u.\n", iface, index);

    return d3dx9_base_effect_get_technique(&effect->base_effect, index);
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetTechniqueByName(ID3DXEffect *iface, const char *name)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    return d3dx9_base_effect_get_technique_by_name(&effect->base_effect, name);
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetPass(ID3DXEffect *iface, D3DXHANDLE technique, UINT index)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, technique %p, index %u.\n", iface, technique, index);

    return d3dx9_base_effect_get_pass(&effect->base_effect, technique, index);
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetPassByName(ID3DXEffect *iface,
        D3DXHANDLE technique, const char *name)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, technique %p, name %s.\n", iface, technique, debugstr_a(name));

    return d3dx9_base_effect_get_pass_by_name(&effect->base_effect, technique, name);
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetFunction(ID3DXEffect *iface, UINT index)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, index %u.\n", iface, index);

    return d3dx9_base_effect_get_function(&effect->base_effect, index);
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetFunctionByName(ID3DXEffect *iface, const char *name)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    return d3dx9_base_effect_get_function_by_name(&effect->base_effect, name);
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetAnnotation(ID3DXEffect *iface, D3DXHANDLE object, UINT index)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, object %p, index %u.\n", iface, object, index);

    return d3dx9_base_effect_get_annotation(&effect->base_effect, object, index);
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetAnnotationByName(ID3DXEffect *iface,
        D3DXHANDLE object, const char *name)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, object %p, name %s.\n", iface, object, debugstr_a(name));

    return d3dx9_base_effect_get_annotation_by_name(&effect->base_effect, object, name);
}

static HRESULT WINAPI ID3DXEffectImpl_SetValue(ID3DXEffect *iface,
        D3DXHANDLE parameter, const void *data, UINT bytes)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, data %p, bytes %u.\n", iface, parameter, data, bytes);

    return d3dx9_base_effect_set_value(&effect->base_effect, parameter, data, bytes);
}

static HRESULT WINAPI ID3DXEffectImpl_GetValue(ID3DXEffect *iface,
        D3DXHANDLE parameter, void *data, UINT bytes)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, data %p, bytes %u.\n", iface, parameter, data, bytes);

    return d3dx9_base_effect_get_value(&effect->base_effect, parameter, data, bytes);
}

static HRESULT WINAPI ID3DXEffectImpl_SetBool(ID3DXEffect *iface, D3DXHANDLE parameter, BOOL b)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, b %#x.\n", iface, parameter, b);

    return d3dx9_base_effect_set_bool(&effect->base_effect, parameter, b);
}

static HRESULT WINAPI ID3DXEffectImpl_GetBool(ID3DXEffect *iface, D3DXHANDLE parameter, BOOL *b)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, b %p.\n", iface, parameter, b);

    return d3dx9_base_effect_get_bool(&effect->base_effect, parameter, b);
}

static HRESULT WINAPI ID3DXEffectImpl_SetBoolArray(ID3DXEffect *iface,
        D3DXHANDLE parameter, const BOOL *b, UINT count)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, b %p, count %u.\n", iface, parameter, b, count);

    return d3dx9_base_effect_set_bool_array(&effect->base_effect, parameter, b, count);
}

static HRESULT WINAPI ID3DXEffectImpl_GetBoolArray(ID3DXEffect *iface,
        D3DXHANDLE parameter, BOOL *b, UINT count)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, b %p, count %u.\n", iface, parameter, b, count);

    return d3dx9_base_effect_get_bool_array(&effect->base_effect, parameter, b, count);
}

static HRESULT WINAPI ID3DXEffectImpl_SetInt(ID3DXEffect *iface, D3DXHANDLE parameter, INT n)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, n %d.\n", iface, parameter, n);

    return d3dx9_base_effect_set_int(&effect->base_effect, parameter, n);
}

static HRESULT WINAPI ID3DXEffectImpl_GetInt(ID3DXEffect *iface, D3DXHANDLE parameter, INT *n)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, n %p.\n", iface, parameter, n);

    return d3dx9_base_effect_get_int(&effect->base_effect, parameter, n);
}

static HRESULT WINAPI ID3DXEffectImpl_SetIntArray(ID3DXEffect *iface,
        D3DXHANDLE parameter, const INT *n, UINT count)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, n %p, count %u.\n", iface, parameter, n, count);

    return d3dx9_base_effect_set_int_array(&effect->base_effect, parameter, n, count);
}

static HRESULT WINAPI ID3DXEffectImpl_GetIntArray(ID3DXEffect *iface,
        D3DXHANDLE parameter, INT *n, UINT count)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, n %p, count %u.\n", iface, parameter, n, count);

    return d3dx9_base_effect_get_int_array(&effect->base_effect, parameter, n, count);
}

static HRESULT WINAPI ID3DXEffectImpl_SetFloat(ID3DXEffect *iface, D3DXHANDLE parameter, float f)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, f %.8e.\n", iface, parameter, f);

    return d3dx9_base_effect_set_float(&effect->base_effect, parameter, f);
}

static HRESULT WINAPI ID3DXEffectImpl_GetFloat(ID3DXEffect *iface, D3DXHANDLE parameter, float *f)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, f %p.\n", iface, parameter, f);

    return d3dx9_base_effect_get_float(&effect->base_effect, parameter, f);
}

static HRESULT WINAPI ID3DXEffectImpl_SetFloatArray(ID3DXEffect *iface,
        D3DXHANDLE parameter, const float *f, UINT count)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, f %p, count %u.\n", iface, parameter, f, count);

    return d3dx9_base_effect_set_float_array(&effect->base_effect, parameter, f, count);
}

static HRESULT WINAPI ID3DXEffectImpl_GetFloatArray(ID3DXEffect *iface,
        D3DXHANDLE parameter, float *f, UINT count)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, f %p, count %u.\n", iface, parameter, f, count);

    return d3dx9_base_effect_get_float_array(&effect->base_effect, parameter, f, count);
}

static HRESULT WINAPI ID3DXEffectImpl_SetVector(ID3DXEffect *iface,
        D3DXHANDLE parameter, const D3DXVECTOR4 *vector)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, vector %p.\n", iface, parameter, vector);

    return d3dx9_base_effect_set_vector(&effect->base_effect, parameter, vector);
}

static HRESULT WINAPI ID3DXEffectImpl_GetVector(ID3DXEffect *iface,
        D3DXHANDLE parameter, D3DXVECTOR4 *vector)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, vector %p.\n", iface, parameter, vector);

    return d3dx9_base_effect_get_vector(&effect->base_effect, parameter, vector);
}

static HRESULT WINAPI ID3DXEffectImpl_SetVectorArray(ID3DXEffect *iface,
        D3DXHANDLE parameter, const D3DXVECTOR4 *vector, UINT count)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, vector %p, count %u.\n", iface, parameter, vector, count);

    return d3dx9_base_effect_set_vector_array(&effect->base_effect, parameter, vector, count);
}

static HRESULT WINAPI ID3DXEffectImpl_GetVectorArray(ID3DXEffect *iface,
        D3DXHANDLE parameter, D3DXVECTOR4 *vector, UINT count)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, vector %p, count %u.\n", iface, parameter, vector, count);

    return d3dx9_base_effect_get_vector_array(&effect->base_effect, parameter, vector, count);
}

static HRESULT WINAPI ID3DXEffectImpl_SetMatrix(ID3DXEffect *iface,
        D3DXHANDLE parameter, const D3DXMATRIX *matrix)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, matrix %p.\n", iface, parameter, matrix);

    return d3dx9_base_effect_set_matrix(&effect->base_effect, parameter, matrix);
}

static HRESULT WINAPI ID3DXEffectImpl_GetMatrix(ID3DXEffect *iface,
        D3DXHANDLE parameter, D3DXMATRIX *matrix)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, matrix %p.\n", iface, parameter, matrix);

    return d3dx9_base_effect_get_matrix(&effect->base_effect, parameter, matrix);
}

static HRESULT WINAPI ID3DXEffectImpl_SetMatrixArray(ID3DXEffect *iface,
        D3DXHANDLE parameter, const D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, matrix %p, count %u.\n", iface, parameter, matrix, count);

    return d3dx9_base_effect_set_matrix_array(&effect->base_effect, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectImpl_GetMatrixArray(ID3DXEffect *iface,
        D3DXHANDLE parameter, D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, matrix %p, count %u.\n", iface, parameter, matrix, count);

    return d3dx9_base_effect_get_matrix_array(&effect->base_effect, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectImpl_SetMatrixPointerArray(ID3DXEffect *iface,
        D3DXHANDLE parameter, const D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, matrix %p, count %u.\n", iface, parameter, matrix, count);

    return d3dx9_base_effect_set_matrix_pointer_array(&effect->base_effect, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectImpl_GetMatrixPointerArray(ID3DXEffect *iface,
        D3DXHANDLE parameter, D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, matrix %p, count %u.\n", iface, parameter, matrix, count);

    return d3dx9_base_effect_get_matrix_pointer_array(&effect->base_effect, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectImpl_SetMatrixTranspose(ID3DXEffect *iface,
        D3DXHANDLE parameter, const D3DXMATRIX *matrix)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, matrix %p.\n", iface, parameter, matrix);

    return d3dx9_base_effect_set_matrix_transpose(&effect->base_effect, parameter, matrix);
}

static HRESULT WINAPI ID3DXEffectImpl_GetMatrixTranspose(ID3DXEffect *iface,
        D3DXHANDLE parameter, D3DXMATRIX *matrix)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, matrix %p.\n", iface, parameter, matrix);

    return d3dx9_base_effect_get_matrix_transpose(&effect->base_effect, parameter, matrix);
}

static HRESULT WINAPI ID3DXEffectImpl_SetMatrixTransposeArray(ID3DXEffect *iface,
        D3DXHANDLE parameter, const D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, matrix %p, count %u.\n", iface, parameter, matrix, count);

    return d3dx9_base_effect_set_matrix_transpose_array(&effect->base_effect, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectImpl_GetMatrixTransposeArray(ID3DXEffect *iface,
        D3DXHANDLE parameter, D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, matrix %p, count %u.\n", iface, parameter, matrix, count);

    return d3dx9_base_effect_get_matrix_transpose_array(&effect->base_effect, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectImpl_SetMatrixTransposePointerArray(ID3DXEffect *iface,
        D3DXHANDLE parameter, const D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, matrix %p, count %u.\n", iface, parameter, matrix, count);

    return d3dx9_base_effect_set_matrix_transpose_pointer_array(&effect->base_effect, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectImpl_GetMatrixTransposePointerArray(ID3DXEffect *iface,
        D3DXHANDLE parameter, D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, matrix %p, count %u.\n", iface, parameter, matrix, count);

    return d3dx9_base_effect_get_matrix_transpose_pointer_array(&effect->base_effect, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectImpl_SetString(ID3DXEffect *iface, D3DXHANDLE parameter, const char *string)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, string %s.\n", iface, parameter, debugstr_a(string));

    return d3dx9_base_effect_set_string(&effect->base_effect, parameter, string);
}

static HRESULT WINAPI ID3DXEffectImpl_GetString(ID3DXEffect *iface, D3DXHANDLE parameter, const char **string)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, string %p.\n", iface, parameter, string);

    return d3dx9_base_effect_get_string(&effect->base_effect, parameter, string);
}

static HRESULT WINAPI ID3DXEffectImpl_SetTexture(struct ID3DXEffect *iface,
        D3DXHANDLE parameter, struct IDirect3DBaseTexture9 *texture)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, texture %p.\n", iface, parameter, texture);

    return d3dx9_base_effect_set_texture(&effect->base_effect, parameter, texture);
}

static HRESULT WINAPI ID3DXEffectImpl_GetTexture(struct ID3DXEffect *iface,
        D3DXHANDLE parameter, struct IDirect3DBaseTexture9 **texture)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, texture %p.\n", iface, parameter, texture);

    return d3dx9_base_effect_get_texture(&effect->base_effect, parameter, texture);
}

static HRESULT WINAPI ID3DXEffectImpl_GetPixelShader(ID3DXEffect *iface,
        D3DXHANDLE parameter, struct IDirect3DPixelShader9 **shader)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, shader %p.\n", iface, parameter, shader);

    return d3dx9_base_effect_get_pixel_shader(&effect->base_effect, parameter, shader);
}

static HRESULT WINAPI ID3DXEffectImpl_GetVertexShader(struct ID3DXEffect *iface,
        D3DXHANDLE parameter, struct IDirect3DVertexShader9 **shader)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, shader %p.\n", iface, parameter, shader);

    return d3dx9_base_effect_get_vertex_shader(&effect->base_effect, parameter, shader);
}

static HRESULT WINAPI ID3DXEffectImpl_SetArrayRange(ID3DXEffect *iface, D3DXHANDLE parameter, UINT start, UINT end)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, parameter %p, start %u, end %u.\n", iface, parameter, start, end);

    return d3dx9_base_effect_set_array_range(&effect->base_effect, parameter, start, end);
}

/*** ID3DXEffect methods ***/
static HRESULT WINAPI ID3DXEffectImpl_GetPool(ID3DXEffect *iface, ID3DXEffectPool **pool)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, pool %p\n", This, pool);

    if (!pool)
    {
        WARN("Invalid argument supplied.\n");
        return D3DERR_INVALIDCALL;
    }

    if (This->pool)
    {
        This->pool->lpVtbl->AddRef(This->pool);
    }

    *pool = This->pool;

    TRACE("Returning pool %p\n", *pool);

    return S_OK;
}

static HRESULT WINAPI ID3DXEffectImpl_SetTechnique(ID3DXEffect *iface, D3DXHANDLE technique)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    struct d3dx9_base_effect *base = &This->base_effect;
    struct d3dx_technique *tech = get_valid_technique(base, technique);

    TRACE("iface %p, technique %p\n", This, technique);

    if (tech)
    {
        This->active_technique = tech;
        TRACE("Technique %p\n", tech);
        return D3D_OK;
    }

    WARN("Invalid argument supplied.\n");

    return D3DERR_INVALIDCALL;
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_GetCurrentTechnique(ID3DXEffect *iface)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    TRACE("iface %p\n", This);

    return get_technique_handle(This->active_technique);
}

static HRESULT WINAPI ID3DXEffectImpl_ValidateTechnique(ID3DXEffect* iface, D3DXHANDLE technique)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(%p): stub\n", This, technique);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXEffectImpl_FindNextValidTechnique(ID3DXEffect* iface, D3DXHANDLE technique, D3DXHANDLE* next_technique)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(%p, %p): stub\n", This, technique, next_technique);

    return E_NOTIMPL;
}

static BOOL WINAPI ID3DXEffectImpl_IsParameterUsed(ID3DXEffect* iface, D3DXHANDLE parameter, D3DXHANDLE technique)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(%p, %p): stub\n", This, parameter, technique);

    return FALSE;
}

static HRESULT WINAPI ID3DXEffectImpl_Begin(ID3DXEffect *iface, UINT *passes, DWORD flags)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    struct d3dx_technique *technique = This->active_technique;

    FIXME("iface %p, passes %p, flags %#x partial stub\n", iface, passes, flags);

    if (passes && technique)
    {
        if (flags & ~(D3DXFX_DONOTSAVESTATE | D3DXFX_DONOTSAVESAMPLERSTATE | D3DXFX_DONOTSAVESHADERSTATE))
            WARN("Invalid flags (%#x) specified.\n", flags);

        if (This->manager || flags & D3DXFX_DONOTSAVESTATE)
        {
            TRACE("State capturing disabled.\n");
        }
        else
        {
            FIXME("State capturing not supported, yet!\n");
        }

        *passes = technique->pass_count;
        This->started = TRUE;
        This->flags = flags;

        return D3D_OK;
    }

    WARN("Invalid argument supplied.\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT WINAPI ID3DXEffectImpl_BeginPass(ID3DXEffect *iface, UINT pass)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);
    struct d3dx_technique *technique = This->active_technique;

    TRACE("iface %p, pass %u\n", This, pass);

    if (technique && pass < technique->pass_count && !This->active_pass)
    {
        This->active_pass = &technique->passes[pass];

        FIXME("No states applied, yet!\n");

        return D3D_OK;
    }

    WARN("Invalid argument supplied.\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT WINAPI ID3DXEffectImpl_CommitChanges(ID3DXEffect* iface)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_EndPass(ID3DXEffect *iface)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    TRACE("iface %p\n", This);

    if (This->active_pass)
    {
         This->active_pass = NULL;
         return D3D_OK;
    }

    WARN("Invalid call.\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT WINAPI ID3DXEffectImpl_End(ID3DXEffect *iface)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("iface %p partial stub\n", iface);

    if (!This->started)
        return D3D_OK;

    if (This->manager || This->flags & D3DXFX_DONOTSAVESTATE)
    {
        TRACE("State restoring disabled.\n");
    }
    else
    {
        FIXME("State restoring not supported, yet!\n");
    }

    This->started = FALSE;

    return D3D_OK;
}

static HRESULT WINAPI ID3DXEffectImpl_GetDevice(ID3DXEffect *iface, struct IDirect3DDevice9 **device)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, device %p\n", This, device);

    if (!device)
    {
        WARN("Invalid argument supplied.\n");
        return D3DERR_INVALIDCALL;
    }

    IDirect3DDevice9_AddRef(This->device);

    *device = This->device;

    TRACE("Returning device %p\n", *device);

    return S_OK;
}

static HRESULT WINAPI ID3DXEffectImpl_OnLostDevice(ID3DXEffect* iface)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_OnResetDevice(ID3DXEffect* iface)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_SetStateManager(ID3DXEffect *iface, ID3DXEffectStateManager *manager)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, manager %p\n", This, manager);

    if (manager) IUnknown_AddRef(manager);
    if (This->manager) IUnknown_Release(This->manager);

    This->manager = manager;

    return D3D_OK;
}

static HRESULT WINAPI ID3DXEffectImpl_GetStateManager(ID3DXEffect *iface, ID3DXEffectStateManager **manager)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    TRACE("iface %p, manager %p\n", This, manager);

    if (!manager)
    {
        WARN("Invalid argument supplied.\n");
        return D3DERR_INVALIDCALL;
    }

    if (This->manager) IUnknown_AddRef(This->manager);
    *manager = This->manager;

    return D3D_OK;
}

static HRESULT WINAPI ID3DXEffectImpl_BeginParameterBlock(ID3DXEffect* iface)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(): stub\n", This);

    return E_NOTIMPL;
}

static D3DXHANDLE WINAPI ID3DXEffectImpl_EndParameterBlock(ID3DXEffect* iface)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(): stub\n", This);

    return NULL;
}

static HRESULT WINAPI ID3DXEffectImpl_ApplyParameterBlock(ID3DXEffect* iface, D3DXHANDLE parameter_block)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(%p): stub\n", This, parameter_block);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_DeleteParameterBlock(ID3DXEffect* iface, D3DXHANDLE parameter_block)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(%p): stub\n", This, parameter_block);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_CloneEffect(ID3DXEffect *iface,
        struct IDirect3DDevice9 *device, struct ID3DXEffect **effect)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(%p, %p): stub\n", This, device, effect);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectImpl_SetRawValue(ID3DXEffect* iface, D3DXHANDLE parameter, LPCVOID data, UINT byte_offset, UINT bytes)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(%p, %p, %u, %u): stub\n", This, parameter, data, byte_offset, bytes);

    return E_NOTIMPL;
}

static const struct ID3DXEffectVtbl ID3DXEffect_Vtbl =
{
    /*** IUnknown methods ***/
    ID3DXEffectImpl_QueryInterface,
    ID3DXEffectImpl_AddRef,
    ID3DXEffectImpl_Release,
    /*** ID3DXBaseEffect methods ***/
    ID3DXEffectImpl_GetDesc,
    ID3DXEffectImpl_GetParameterDesc,
    ID3DXEffectImpl_GetTechniqueDesc,
    ID3DXEffectImpl_GetPassDesc,
    ID3DXEffectImpl_GetFunctionDesc,
    ID3DXEffectImpl_GetParameter,
    ID3DXEffectImpl_GetParameterByName,
    ID3DXEffectImpl_GetParameterBySemantic,
    ID3DXEffectImpl_GetParameterElement,
    ID3DXEffectImpl_GetTechnique,
    ID3DXEffectImpl_GetTechniqueByName,
    ID3DXEffectImpl_GetPass,
    ID3DXEffectImpl_GetPassByName,
    ID3DXEffectImpl_GetFunction,
    ID3DXEffectImpl_GetFunctionByName,
    ID3DXEffectImpl_GetAnnotation,
    ID3DXEffectImpl_GetAnnotationByName,
    ID3DXEffectImpl_SetValue,
    ID3DXEffectImpl_GetValue,
    ID3DXEffectImpl_SetBool,
    ID3DXEffectImpl_GetBool,
    ID3DXEffectImpl_SetBoolArray,
    ID3DXEffectImpl_GetBoolArray,
    ID3DXEffectImpl_SetInt,
    ID3DXEffectImpl_GetInt,
    ID3DXEffectImpl_SetIntArray,
    ID3DXEffectImpl_GetIntArray,
    ID3DXEffectImpl_SetFloat,
    ID3DXEffectImpl_GetFloat,
    ID3DXEffectImpl_SetFloatArray,
    ID3DXEffectImpl_GetFloatArray,
    ID3DXEffectImpl_SetVector,
    ID3DXEffectImpl_GetVector,
    ID3DXEffectImpl_SetVectorArray,
    ID3DXEffectImpl_GetVectorArray,
    ID3DXEffectImpl_SetMatrix,
    ID3DXEffectImpl_GetMatrix,
    ID3DXEffectImpl_SetMatrixArray,
    ID3DXEffectImpl_GetMatrixArray,
    ID3DXEffectImpl_SetMatrixPointerArray,
    ID3DXEffectImpl_GetMatrixPointerArray,
    ID3DXEffectImpl_SetMatrixTranspose,
    ID3DXEffectImpl_GetMatrixTranspose,
    ID3DXEffectImpl_SetMatrixTransposeArray,
    ID3DXEffectImpl_GetMatrixTransposeArray,
    ID3DXEffectImpl_SetMatrixTransposePointerArray,
    ID3DXEffectImpl_GetMatrixTransposePointerArray,
    ID3DXEffectImpl_SetString,
    ID3DXEffectImpl_GetString,
    ID3DXEffectImpl_SetTexture,
    ID3DXEffectImpl_GetTexture,
    ID3DXEffectImpl_GetPixelShader,
    ID3DXEffectImpl_GetVertexShader,
    ID3DXEffectImpl_SetArrayRange,
    /*** ID3DXEffect methods ***/
    ID3DXEffectImpl_GetPool,
    ID3DXEffectImpl_SetTechnique,
    ID3DXEffectImpl_GetCurrentTechnique,
    ID3DXEffectImpl_ValidateTechnique,
    ID3DXEffectImpl_FindNextValidTechnique,
    ID3DXEffectImpl_IsParameterUsed,
    ID3DXEffectImpl_Begin,
    ID3DXEffectImpl_BeginPass,
    ID3DXEffectImpl_CommitChanges,
    ID3DXEffectImpl_EndPass,
    ID3DXEffectImpl_End,
    ID3DXEffectImpl_GetDevice,
    ID3DXEffectImpl_OnLostDevice,
    ID3DXEffectImpl_OnResetDevice,
    ID3DXEffectImpl_SetStateManager,
    ID3DXEffectImpl_GetStateManager,
    ID3DXEffectImpl_BeginParameterBlock,
    ID3DXEffectImpl_EndParameterBlock,
    ID3DXEffectImpl_ApplyParameterBlock,
    ID3DXEffectImpl_DeleteParameterBlock,
    ID3DXEffectImpl_CloneEffect,
    ID3DXEffectImpl_SetRawValue
};

static inline struct ID3DXEffectCompilerImpl *impl_from_ID3DXEffectCompiler(ID3DXEffectCompiler *iface)
{
    return CONTAINING_RECORD(iface, struct ID3DXEffectCompilerImpl, ID3DXEffectCompiler_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI ID3DXEffectCompilerImpl_QueryInterface(ID3DXEffectCompiler *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_ID3DXEffectCompiler))
    {
        iface->lpVtbl->AddRef(iface);
        *object = iface;
        return S_OK;
    }

    ERR("Interface %s not found\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXEffectCompilerImpl_AddRef(ID3DXEffectCompiler *iface)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p: AddRef from %u\n", iface, This->ref);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ID3DXEffectCompilerImpl_Release(ID3DXEffectCompiler *iface)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("iface %p: Release from %u\n", iface, ref + 1);

    if (!ref)
    {
        free_effect_compiler(This);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** ID3DXBaseEffect methods ***/
static HRESULT WINAPI ID3DXEffectCompilerImpl_GetDesc(ID3DXEffectCompiler *iface, D3DXEFFECT_DESC *desc)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    return d3dx9_base_effect_get_desc(&compiler->base_effect, desc);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetParameterDesc(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, D3DXPARAMETER_DESC *desc)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, desc %p.\n", iface, parameter, desc);

    return d3dx9_base_effect_get_parameter_desc(&compiler->base_effect, parameter, desc);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetTechniqueDesc(ID3DXEffectCompiler *iface,
        D3DXHANDLE technique, D3DXTECHNIQUE_DESC *desc)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, technique %p, desc %p.\n", iface, technique, desc);

    return d3dx9_base_effect_get_technique_desc(&compiler->base_effect, technique, desc);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetPassDesc(ID3DXEffectCompiler *iface,
        D3DXHANDLE pass, D3DXPASS_DESC *desc)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, pass %p, desc %p.\n", iface, pass, desc);

    return d3dx9_base_effect_get_pass_desc(&compiler->base_effect, pass, desc);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetFunctionDesc(ID3DXEffectCompiler *iface,
        D3DXHANDLE shader, D3DXFUNCTION_DESC *desc)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, shader %p, desc %p.\n", iface, shader, desc);

    return d3dx9_base_effect_get_function_desc(&compiler->base_effect, shader, desc);
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetParameter(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, UINT index)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, index %u.\n", iface, parameter, index);

    return d3dx9_base_effect_get_parameter(&compiler->base_effect, parameter, index);
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetParameterByName(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const char *name)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, name %s.\n", iface, parameter, debugstr_a(name));

    return d3dx9_base_effect_get_parameter_by_name(&compiler->base_effect, parameter, name);
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetParameterBySemantic(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const char *semantic)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, semantic %s.\n", iface, parameter, debugstr_a(semantic));

    return d3dx9_base_effect_get_parameter_by_semantic(&compiler->base_effect, parameter, semantic);
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetParameterElement(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, UINT index)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, index %u.\n", iface, parameter, index);

    return d3dx9_base_effect_get_parameter_element(&compiler->base_effect, parameter, index);
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetTechnique(ID3DXEffectCompiler *iface, UINT index)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, index %u.\n", iface, index);

    return d3dx9_base_effect_get_technique(&compiler->base_effect, index);
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetTechniqueByName(ID3DXEffectCompiler *iface, const char *name)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    return d3dx9_base_effect_get_technique_by_name(&compiler->base_effect, name);
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetPass(ID3DXEffectCompiler *iface, D3DXHANDLE technique, UINT index)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, technique %p, index %u.\n", iface, technique, index);

    return d3dx9_base_effect_get_pass(&compiler->base_effect, technique, index);
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetPassByName(ID3DXEffectCompiler *iface,
        D3DXHANDLE technique, const char *name)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, technique %p, name %s.\n", iface, technique, debugstr_a(name));

    return d3dx9_base_effect_get_pass_by_name(&compiler->base_effect, technique, name);
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetFunction(ID3DXEffectCompiler *iface, UINT index)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, index %u.\n", iface, index);

    return d3dx9_base_effect_get_function(&compiler->base_effect, index);
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetFunctionByName(ID3DXEffectCompiler *iface, const char *name)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, name %s.\n", iface, debugstr_a(name));

    return d3dx9_base_effect_get_function_by_name(&compiler->base_effect, name);
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetAnnotation(ID3DXEffectCompiler *iface,
        D3DXHANDLE object, UINT index)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, object %p, index %u.\n", iface, object, index);

    return d3dx9_base_effect_get_annotation(&compiler->base_effect, object, index);
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetAnnotationByName(ID3DXEffectCompiler *iface,
        D3DXHANDLE object, const char *name)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, object %p, name %s.\n", iface, object, debugstr_a(name));

    return d3dx9_base_effect_get_annotation_by_name(&compiler->base_effect, object, name);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetValue(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const void *data, UINT bytes)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, data %p, bytes %u.\n", iface, parameter, data, bytes);

    return d3dx9_base_effect_set_value(&compiler->base_effect, parameter, data, bytes);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetValue(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, void *data, UINT bytes)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, data %p, bytes %u.\n", iface, parameter, data, bytes);

    return d3dx9_base_effect_get_value(&compiler->base_effect, parameter, data, bytes);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetBool(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, BOOL b)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, b %#x.\n", iface, parameter, b);

    return d3dx9_base_effect_set_bool(&compiler->base_effect, parameter, b);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetBool(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, BOOL *b)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, b %p.\n", iface, parameter, b);

    return d3dx9_base_effect_get_bool(&compiler->base_effect, parameter, b);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetBoolArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const BOOL *b, UINT count)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, b %p, count %u.\n", iface, parameter, b, count);

    return d3dx9_base_effect_set_bool_array(&compiler->base_effect, parameter, b, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetBoolArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, BOOL *b, UINT count)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, b %p, count %u.\n", iface, parameter, b, count);

    return d3dx9_base_effect_get_bool_array(&compiler->base_effect, parameter, b, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetInt(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, INT n)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, n %d.\n", iface, parameter, n);

    return d3dx9_base_effect_set_int(&compiler->base_effect, parameter, n);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetInt(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, INT *n)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, n %p.\n", iface, parameter, n);

    return d3dx9_base_effect_get_int(&compiler->base_effect, parameter, n);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetIntArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const INT *n, UINT count)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, n %p, count %u.\n", iface, parameter, n, count);

    return d3dx9_base_effect_set_int_array(&compiler->base_effect, parameter, n, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetIntArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, INT *n, UINT count)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, n %p, count %u.\n", iface, parameter, n, count);

    return d3dx9_base_effect_get_int_array(&compiler->base_effect, parameter, n, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetFloat(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, float f)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, f %.8e.\n", iface, parameter, f);

    return d3dx9_base_effect_set_float(&compiler->base_effect, parameter, f);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetFloat(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, float *f)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, f %p.\n", iface, parameter, f);

    return d3dx9_base_effect_get_float(&compiler->base_effect, parameter, f);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetFloatArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const float *f, UINT count)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, f %p, count %u.\n", iface, parameter, f, count);

    return d3dx9_base_effect_set_float_array(&compiler->base_effect, parameter, f, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetFloatArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, float *f, UINT count)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, f %p, count %u.\n", iface, parameter, f, count);

    return d3dx9_base_effect_get_float_array(&compiler->base_effect, parameter, f, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetVector(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const D3DXVECTOR4 *vector)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, vector %p.\n", iface, parameter, vector);

    return d3dx9_base_effect_set_vector(&compiler->base_effect, parameter, vector);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetVector(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, D3DXVECTOR4 *vector)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, vector %p.\n", iface, parameter, vector);

    return d3dx9_base_effect_get_vector(&compiler->base_effect, parameter, vector);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetVectorArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const D3DXVECTOR4 *vector, UINT count)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, vector %p, count %u.\n", iface, parameter, vector, count);

    return d3dx9_base_effect_set_vector_array(&compiler->base_effect, parameter, vector, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetVectorArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, D3DXVECTOR4 *vector, UINT count)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, vector %p, count %u.\n", iface, parameter, vector, count);

    return d3dx9_base_effect_get_vector_array(&compiler->base_effect, parameter, vector, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetMatrix(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const D3DXMATRIX *matrix)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, matrix %p.\n", iface, parameter, matrix);

    return d3dx9_base_effect_set_matrix(&compiler->base_effect, parameter, matrix);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetMatrix(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, D3DXMATRIX *matrix)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, matrix %p.\n", iface, parameter, matrix);

    return d3dx9_base_effect_get_matrix(&compiler->base_effect, parameter, matrix);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetMatrixArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, matrix %p, count %u.\n", iface, parameter, matrix, count);

    return d3dx9_base_effect_set_matrix_array(&compiler->base_effect, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetMatrixArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, matrix %p, count %u.\n", iface, parameter, matrix, count);

    return d3dx9_base_effect_get_matrix_array(&compiler->base_effect, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetMatrixPointerArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, matrix %p, count %u.\n", iface, parameter, matrix, count);

    return d3dx9_base_effect_set_matrix_pointer_array(&compiler->base_effect, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetMatrixPointerArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, matrix %p, count %u.\n", iface, parameter, matrix, count);

    return d3dx9_base_effect_get_matrix_pointer_array(&compiler->base_effect, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetMatrixTranspose(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const D3DXMATRIX *matrix)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, matrix %p.\n", iface, parameter, matrix);

    return d3dx9_base_effect_set_matrix_transpose(&compiler->base_effect, parameter, matrix);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetMatrixTranspose(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, D3DXMATRIX *matrix)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, matrix %p.\n", iface, parameter, matrix);

    return d3dx9_base_effect_get_matrix_transpose(&compiler->base_effect, parameter, matrix);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetMatrixTransposeArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, matrix %p, count %u.\n", iface, parameter, matrix, count);

    return d3dx9_base_effect_set_matrix_transpose_array(&compiler->base_effect, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetMatrixTransposeArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, matrix %p, count %u.\n", iface, parameter, matrix, count);

    return d3dx9_base_effect_get_matrix_transpose_array(&compiler->base_effect, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetMatrixTransposePointerArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, matrix %p, count %u.\n", iface, parameter, matrix, count);

    return d3dx9_base_effect_set_matrix_transpose_pointer_array(&compiler->base_effect, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetMatrixTransposePointerArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, matrix %p, count %u.\n", iface, parameter, matrix, count);

    return d3dx9_base_effect_get_matrix_transpose_pointer_array(&compiler->base_effect, parameter, matrix, count);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetString(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const char *string)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, string %s.\n", iface, parameter, debugstr_a(string));

    return d3dx9_base_effect_set_string(&compiler->base_effect, parameter, string);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetString(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const char **string)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, string %p.\n", iface, parameter, string);

    return d3dx9_base_effect_get_string(&compiler->base_effect, parameter, string);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetTexture(struct ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, struct IDirect3DBaseTexture9 *texture)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, texture %p.\n", iface, parameter, texture);

    return d3dx9_base_effect_set_texture(&compiler->base_effect, parameter, texture);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetTexture(struct ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, struct IDirect3DBaseTexture9 **texture)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, texture %p.\n", iface, parameter, texture);

    return d3dx9_base_effect_get_texture(&compiler->base_effect, parameter, texture);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetPixelShader(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, struct IDirect3DPixelShader9 **shader)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, shader %p.\n", iface, parameter, shader);

    return d3dx9_base_effect_get_pixel_shader(&compiler->base_effect, parameter, shader);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetVertexShader(struct ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, struct IDirect3DVertexShader9 **shader)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, shader %p.\n", iface, parameter, shader);

    return d3dx9_base_effect_get_vertex_shader(&compiler->base_effect, parameter, shader);
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetArrayRange(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, UINT start, UINT end)
{
    struct ID3DXEffectCompilerImpl *compiler = impl_from_ID3DXEffectCompiler(iface);

    TRACE("iface %p, parameter %p, start %u, end %u.\n", iface, parameter, start, end);

    return d3dx9_base_effect_set_array_range(&compiler->base_effect, parameter, start, end);
}

/*** ID3DXEffectCompiler methods ***/
static HRESULT WINAPI ID3DXEffectCompilerImpl_SetLiteral(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, BOOL literal)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);

    FIXME("iface %p, parameter %p, literal %u\n", This, parameter, literal);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetLiteral(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, BOOL *literal)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);

    FIXME("iface %p, parameter %p, literal %p\n", This, parameter, literal);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_CompileEffect(ID3DXEffectCompiler *iface, DWORD flags,
        ID3DXBuffer **effect, ID3DXBuffer **error_msgs)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);

    FIXME("iface %p, flags %#x, effect %p, error_msgs %p stub\n", This, flags, effect, error_msgs);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_CompileShader(ID3DXEffectCompiler *iface, D3DXHANDLE function,
        const char *target, DWORD flags, ID3DXBuffer **shader, ID3DXBuffer **error_msgs,
        ID3DXConstantTable **constant_table)
{
    struct ID3DXEffectCompilerImpl *This = impl_from_ID3DXEffectCompiler(iface);

    FIXME("iface %p, function %p, target %p, flags %#x, shader %p, error_msgs %p, constant_table %p stub\n",
            This, function, target, flags, shader, error_msgs, constant_table);

    return E_NOTIMPL;
}

static const struct ID3DXEffectCompilerVtbl ID3DXEffectCompiler_Vtbl =
{
    /*** IUnknown methods ***/
    ID3DXEffectCompilerImpl_QueryInterface,
    ID3DXEffectCompilerImpl_AddRef,
    ID3DXEffectCompilerImpl_Release,
    /*** ID3DXBaseEffect methods ***/
    ID3DXEffectCompilerImpl_GetDesc,
    ID3DXEffectCompilerImpl_GetParameterDesc,
    ID3DXEffectCompilerImpl_GetTechniqueDesc,
    ID3DXEffectCompilerImpl_GetPassDesc,
    ID3DXEffectCompilerImpl_GetFunctionDesc,
    ID3DXEffectCompilerImpl_GetParameter,
    ID3DXEffectCompilerImpl_GetParameterByName,
    ID3DXEffectCompilerImpl_GetParameterBySemantic,
    ID3DXEffectCompilerImpl_GetParameterElement,
    ID3DXEffectCompilerImpl_GetTechnique,
    ID3DXEffectCompilerImpl_GetTechniqueByName,
    ID3DXEffectCompilerImpl_GetPass,
    ID3DXEffectCompilerImpl_GetPassByName,
    ID3DXEffectCompilerImpl_GetFunction,
    ID3DXEffectCompilerImpl_GetFunctionByName,
    ID3DXEffectCompilerImpl_GetAnnotation,
    ID3DXEffectCompilerImpl_GetAnnotationByName,
    ID3DXEffectCompilerImpl_SetValue,
    ID3DXEffectCompilerImpl_GetValue,
    ID3DXEffectCompilerImpl_SetBool,
    ID3DXEffectCompilerImpl_GetBool,
    ID3DXEffectCompilerImpl_SetBoolArray,
    ID3DXEffectCompilerImpl_GetBoolArray,
    ID3DXEffectCompilerImpl_SetInt,
    ID3DXEffectCompilerImpl_GetInt,
    ID3DXEffectCompilerImpl_SetIntArray,
    ID3DXEffectCompilerImpl_GetIntArray,
    ID3DXEffectCompilerImpl_SetFloat,
    ID3DXEffectCompilerImpl_GetFloat,
    ID3DXEffectCompilerImpl_SetFloatArray,
    ID3DXEffectCompilerImpl_GetFloatArray,
    ID3DXEffectCompilerImpl_SetVector,
    ID3DXEffectCompilerImpl_GetVector,
    ID3DXEffectCompilerImpl_SetVectorArray,
    ID3DXEffectCompilerImpl_GetVectorArray,
    ID3DXEffectCompilerImpl_SetMatrix,
    ID3DXEffectCompilerImpl_GetMatrix,
    ID3DXEffectCompilerImpl_SetMatrixArray,
    ID3DXEffectCompilerImpl_GetMatrixArray,
    ID3DXEffectCompilerImpl_SetMatrixPointerArray,
    ID3DXEffectCompilerImpl_GetMatrixPointerArray,
    ID3DXEffectCompilerImpl_SetMatrixTranspose,
    ID3DXEffectCompilerImpl_GetMatrixTranspose,
    ID3DXEffectCompilerImpl_SetMatrixTransposeArray,
    ID3DXEffectCompilerImpl_GetMatrixTransposeArray,
    ID3DXEffectCompilerImpl_SetMatrixTransposePointerArray,
    ID3DXEffectCompilerImpl_GetMatrixTransposePointerArray,
    ID3DXEffectCompilerImpl_SetString,
    ID3DXEffectCompilerImpl_GetString,
    ID3DXEffectCompilerImpl_SetTexture,
    ID3DXEffectCompilerImpl_GetTexture,
    ID3DXEffectCompilerImpl_GetPixelShader,
    ID3DXEffectCompilerImpl_GetVertexShader,
    ID3DXEffectCompilerImpl_SetArrayRange,
    /*** ID3DXEffectCompiler methods ***/
    ID3DXEffectCompilerImpl_SetLiteral,
    ID3DXEffectCompilerImpl_GetLiteral,
    ID3DXEffectCompilerImpl_CompileEffect,
    ID3DXEffectCompilerImpl_CompileShader,
};

static HRESULT d3dx9_parse_sampler(struct d3dx_sampler *sampler, const char *data, const char **ptr, D3DXHANDLE *objects)
{
    HRESULT hr;
    UINT i;
    struct d3dx_state *states;

    read_dword(ptr, &sampler->state_count);
    TRACE("Count: %u\n", sampler->state_count);

    states = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*states) * sampler->state_count);
    if (!states)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < sampler->state_count; ++i)
    {
        hr = d3dx9_parse_state(&states[i], data, ptr, objects);
        if (hr != D3D_OK)
        {
            WARN("Failed to parse state %u\n", i);
            goto err_out;
        }
    }

    sampler->states = states;

    return D3D_OK;

err_out:

    for (i = 0; i < sampler->state_count; ++i)
    {
        free_state(&states[i]);
    }

    HeapFree(GetProcessHeap(), 0, states);

    return hr;
}

static HRESULT d3dx9_parse_value(struct d3dx_parameter *param, void *value, const char *data, const char **ptr, D3DXHANDLE *objects)
{
    unsigned int i;
    HRESULT hr;
    UINT old_size = 0;
    DWORD id;

    if (param->element_count)
    {
        param->data = value;

        for (i = 0; i < param->element_count; ++i)
        {
            struct d3dx_parameter *member = &param->members[i];

            hr = d3dx9_parse_value(member, value ? (char *)value + old_size : NULL, data, ptr, objects);
            if (hr != D3D_OK)
            {
                WARN("Failed to parse value %u\n", i);
                return hr;
            }

            old_size += member->bytes;
        }

        return D3D_OK;
    }

    switch(param->class)
    {
        case D3DXPC_SCALAR:
        case D3DXPC_VECTOR:
        case D3DXPC_MATRIX_ROWS:
        case D3DXPC_MATRIX_COLUMNS:
            param->data = value;
            break;

        case D3DXPC_STRUCT:
            param->data = value;

            for (i = 0; i < param->member_count; ++i)
            {
                struct d3dx_parameter *member = &param->members[i];

                hr = d3dx9_parse_value(member, (char *)value + old_size, data, ptr, objects);
                if (hr != D3D_OK)
                {
                    WARN("Failed to parse value %u\n", i);
                    return hr;
                }

                old_size += member->bytes;
            }
            break;

        case D3DXPC_OBJECT:
            switch (param->type)
            {
                case D3DXPT_STRING:
                case D3DXPT_TEXTURE:
                case D3DXPT_TEXTURE1D:
                case D3DXPT_TEXTURE2D:
                case D3DXPT_TEXTURE3D:
                case D3DXPT_TEXTURECUBE:
                case D3DXPT_PIXELSHADER:
                case D3DXPT_VERTEXSHADER:
                    read_dword(ptr, &id);
                    TRACE("Id: %u\n", id);
                    objects[id] = get_parameter_handle(param);
                    param->data = value;
                    break;

                case D3DXPT_SAMPLER:
                case D3DXPT_SAMPLER1D:
                case D3DXPT_SAMPLER2D:
                case D3DXPT_SAMPLER3D:
                case D3DXPT_SAMPLERCUBE:
                {
                    struct d3dx_sampler *sampler;

                    sampler = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*sampler));
                    if (!sampler)
                        return E_OUTOFMEMORY;

                    hr = d3dx9_parse_sampler(sampler, data, ptr, objects);
                    if (hr != D3D_OK)
                    {
                        HeapFree(GetProcessHeap(), 0, sampler);
                        WARN("Failed to parse sampler\n");
                        return hr;
                    }

                    param->data = sampler;
                    break;
                }

                default:
                    FIXME("Unhandled type %s\n", debug_d3dxparameter_type(param->type));
                    break;
            }
            break;

        default:
            FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
            break;
    }

    return D3D_OK;
}

static HRESULT d3dx9_parse_init_value(struct d3dx_parameter *param, const char *data, const char *ptr, D3DXHANDLE *objects)
{
    UINT size = param->bytes;
    HRESULT hr;
    void *value = NULL;

    TRACE("param size: %u\n", size);

    if (size)
    {
        value = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
        if (!value)
        {
            ERR("Failed to allocate data memory.\n");
            return E_OUTOFMEMORY;
        }

        switch(param->class)
        {
            case D3DXPC_OBJECT:
                break;

            case D3DXPC_SCALAR:
            case D3DXPC_VECTOR:
            case D3DXPC_MATRIX_ROWS:
            case D3DXPC_MATRIX_COLUMNS:
            case D3DXPC_STRUCT:
                TRACE("Data: %s.\n", debugstr_an(ptr, size));
                memcpy(value, ptr, size);
                break;

            default:
                FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
                break;
        }
    }

    hr = d3dx9_parse_value(param, value, data, &ptr, objects);
    if (hr != D3D_OK)
    {
        WARN("Failed to parse value\n");
        HeapFree(GetProcessHeap(), 0, value);
        return hr;
    }

    return D3D_OK;
}

static HRESULT d3dx9_parse_name(char **name, const char *ptr)
{
    DWORD size;

    read_dword(&ptr, &size);
    TRACE("Name size: %#x\n", size);

    if (!size)
    {
        return D3D_OK;
    }

    *name = HeapAlloc(GetProcessHeap(), 0, size);
    if (!*name)
    {
        ERR("Failed to allocate name memory.\n");
        return E_OUTOFMEMORY;
    }

    TRACE("Name: %s.\n", debugstr_an(ptr, size));
    memcpy(*name, ptr, size);

    return D3D_OK;
}

static HRESULT d3dx9_copy_data(char **str, const char **ptr)
{
    DWORD size;

    read_dword(ptr, &size);
    TRACE("Data size: %#x\n", size);

    *str = HeapAlloc(GetProcessHeap(), 0, size);
    if (!*str)
    {
        ERR("Failed to allocate name memory.\n");
        return E_OUTOFMEMORY;
    }

    TRACE("Data: %s.\n", debugstr_an(*ptr, size));
    memcpy(*str, *ptr, size);

    *ptr += ((size + 3) & ~3);

    return D3D_OK;
}

static HRESULT d3dx9_parse_data(struct d3dx_parameter *param, const char **ptr, struct IDirect3DDevice9 *device)
{
    DWORD size;
    HRESULT hr;

    TRACE("Parse data for parameter %s, type %s\n", debugstr_a(param->name), debug_d3dxparameter_type(param->type));

    read_dword(ptr, &size);
    TRACE("Data size: %#x\n", size);

    if (!size)
    {
        TRACE("Size is 0\n");
        *(void **)param->data = NULL;
        return D3D_OK;
    }

    switch (param->type)
    {
        case D3DXPT_STRING:
            /* re-read with size (sizeof(DWORD) = 4) */
            hr = d3dx9_parse_name((LPSTR *)param->data, *ptr - 4);
            if (hr != D3D_OK)
            {
                WARN("Failed to parse string data\n");
                return hr;
            }
            break;

        case D3DXPT_VERTEXSHADER:
            if (FAILED(hr = IDirect3DDevice9_CreateVertexShader(device, (DWORD *)*ptr, param->data)))
            {
                WARN("Failed to create vertex shader\n");
                return hr;
            }
            break;

        case D3DXPT_PIXELSHADER:
            if (FAILED(hr = IDirect3DDevice9_CreatePixelShader(device, (DWORD *)*ptr, param->data)))
            {
                WARN("Failed to create pixel shader\n");
                return hr;
            }
            break;

        default:
            FIXME("Unhandled type %s\n", debug_d3dxparameter_type(param->type));
            break;
    }


    *ptr += ((size + 3) & ~3);

    return D3D_OK;
}

static HRESULT d3dx9_parse_effect_typedef(struct d3dx_parameter *param, const char *data, const char **ptr,
        struct d3dx_parameter *parent, UINT flags)
{
    DWORD offset;
    HRESULT hr;
    UINT i;

    param->flags = flags;

    if (!parent)
    {
        read_dword(ptr, &param->type);
        TRACE("Type: %s\n", debug_d3dxparameter_type(param->type));

        read_dword(ptr, &param->class);
        TRACE("Class: %s\n", debug_d3dxparameter_class(param->class));

        read_dword(ptr, &offset);
        TRACE("Type name offset: %#x\n", offset);
        hr = d3dx9_parse_name(&param->name, data + offset);
        if (hr != D3D_OK)
        {
            WARN("Failed to parse name\n");
            goto err_out;
        }

        read_dword(ptr, &offset);
        TRACE("Type semantic offset: %#x\n", offset);
        hr = d3dx9_parse_name(&param->semantic, data + offset);
        if (hr != D3D_OK)
        {
            WARN("Failed to parse semantic\n");
            goto err_out;
        }

        read_dword(ptr, &param->element_count);
        TRACE("Elements: %u\n", param->element_count);

        switch (param->class)
        {
            case D3DXPC_VECTOR:
                read_dword(ptr, &param->columns);
                TRACE("Columns: %u\n", param->columns);

                read_dword(ptr, &param->rows);
                TRACE("Rows: %u\n", param->rows);

                /* sizeof(DWORD) * rows * columns */
                param->bytes = 4 * param->rows * param->columns;
                break;

            case D3DXPC_SCALAR:
            case D3DXPC_MATRIX_ROWS:
            case D3DXPC_MATRIX_COLUMNS:
                read_dword(ptr, &param->rows);
                TRACE("Rows: %u\n", param->rows);

                read_dword(ptr, &param->columns);
                TRACE("Columns: %u\n", param->columns);

                /* sizeof(DWORD) * rows * columns */
                param->bytes = 4 * param->rows * param->columns;
                break;

            case D3DXPC_STRUCT:
                read_dword(ptr, &param->member_count);
                TRACE("Members: %u\n", param->member_count);
                break;

            case D3DXPC_OBJECT:
                switch (param->type)
                {
                    case D3DXPT_STRING:
                    case D3DXPT_PIXELSHADER:
                    case D3DXPT_VERTEXSHADER:
                    case D3DXPT_TEXTURE:
                    case D3DXPT_TEXTURE1D:
                    case D3DXPT_TEXTURE2D:
                    case D3DXPT_TEXTURE3D:
                    case D3DXPT_TEXTURECUBE:
                        param->bytes = sizeof(void *);
                        break;

                    case D3DXPT_SAMPLER:
                    case D3DXPT_SAMPLER1D:
                    case D3DXPT_SAMPLER2D:
                    case D3DXPT_SAMPLER3D:
                    case D3DXPT_SAMPLERCUBE:
                        param->bytes = 0;
                        break;

                    default:
                        FIXME("Unhandled type %s\n", debug_d3dxparameter_type(param->type));
                        break;
                }
                break;

            default:
                FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
                break;
        }
    }
    else
    {
        /* elements */
        param->type = parent->type;
        param->class = parent->class;
        param->name = parent->name;
        param->semantic = parent->semantic;
        param->element_count = 0;
        param->annotation_count = 0;
        param->member_count = parent->member_count;
        param->bytes = parent->bytes;
        param->rows = parent->rows;
        param->columns = parent->columns;
    }

    if (param->element_count)
    {
        unsigned int param_bytes = 0;
        const char *save_ptr = *ptr;

        param->members = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*param->members) * param->element_count);
        if (!param->members)
        {
            ERR("Out of memory\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }

        for (i = 0; i < param->element_count; ++i)
        {
            *ptr = save_ptr;

            hr = d3dx9_parse_effect_typedef(&param->members[i], data, ptr, param, flags);
            if (hr != D3D_OK)
            {
                WARN("Failed to parse member %u\n", i);
                goto err_out;
            }

            param_bytes += param->members[i].bytes;
        }

        param->bytes = param_bytes;
    }
    else if (param->member_count)
    {
        param->members = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*param->members) * param->member_count);
        if (!param->members)
        {
            ERR("Out of memory\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }

        for (i = 0; i < param->member_count; ++i)
        {
            hr = d3dx9_parse_effect_typedef(&param->members[i], data, ptr, NULL, flags);
            if (hr != D3D_OK)
            {
                WARN("Failed to parse member %u\n", i);
                goto err_out;
            }

            param->bytes += param->members[i].bytes;
        }
    }
    return D3D_OK;

err_out:

    if (param->members)
    {
        unsigned int count = param->element_count ? param->element_count : param->member_count;

        for (i = 0; i < count; ++i)
            free_parameter(&param->members[i], param->element_count != 0, TRUE);
        HeapFree(GetProcessHeap(), 0, param->members);
        param->members = NULL;
    }

    if (!parent)
    {
        HeapFree(GetProcessHeap(), 0, param->name);
        HeapFree(GetProcessHeap(), 0, param->semantic);
    }
    param->name = NULL;
    param->semantic = NULL;

    return hr;
}

static HRESULT d3dx9_parse_effect_annotation(struct d3dx_parameter *anno, const char *data, const char **ptr, D3DXHANDLE *objects)
{
    DWORD offset;
    const char *ptr2;
    HRESULT hr;

    anno->flags = D3DX_PARAMETER_ANNOTATION;

    read_dword(ptr, &offset);
    TRACE("Typedef offset: %#x\n", offset);
    ptr2 = data + offset;
    hr = d3dx9_parse_effect_typedef(anno, data, &ptr2, NULL, D3DX_PARAMETER_ANNOTATION);
    if (hr != D3D_OK)
    {
        WARN("Failed to parse type definition\n");
        return hr;
    }

    read_dword(ptr, &offset);
    TRACE("Value offset: %#x\n", offset);
    hr = d3dx9_parse_init_value(anno, data, data + offset, objects);
    if (hr != D3D_OK)
    {
        WARN("Failed to parse value\n");
        return hr;
    }

    return D3D_OK;
}

static HRESULT d3dx9_parse_state(struct d3dx_state *state, const char *data, const char **ptr, D3DXHANDLE *objects)
{
    DWORD offset;
    const char *ptr2;
    HRESULT hr;
    struct d3dx_parameter *parameter;

    parameter = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*parameter));
    if (!parameter)
        return E_OUTOFMEMORY;

    state->type = ST_CONSTANT;

    read_dword(ptr, &state->operation);
    TRACE("Operation: %#x (%s)\n", state->operation, state_table[state->operation].name);

    read_dword(ptr, &state->index);
    TRACE("Index: %#x\n", state->index);

    read_dword(ptr, &offset);
    TRACE("Typedef offset: %#x\n", offset);
    ptr2 = data + offset;
    hr = d3dx9_parse_effect_typedef(parameter, data, &ptr2, NULL, 0);
    if (hr != D3D_OK)
    {
        WARN("Failed to parse type definition\n");
        goto err_out;
    }

    read_dword(ptr, &offset);
    TRACE("Value offset: %#x\n", offset);
    hr = d3dx9_parse_init_value(parameter, data, data + offset, objects);
    if (hr != D3D_OK)
    {
        WARN("Failed to parse value\n");
        goto err_out;
    }

    state->parameter = parameter;

    return D3D_OK;

err_out:

    free_parameter(parameter, FALSE, FALSE);

    return hr;
}

static HRESULT d3dx9_parse_effect_parameter(struct d3dx_parameter *param, const char *data, const char **ptr, D3DXHANDLE *objects)
{
    DWORD offset;
    HRESULT hr;
    unsigned int i;
    const char *ptr2;

    read_dword(ptr, &offset);
    TRACE("Typedef offset: %#x\n", offset);
    ptr2 = data + offset;

    read_dword(ptr, &offset);
    TRACE("Value offset: %#x\n", offset);

    read_dword(ptr, &param->flags);
    TRACE("Flags: %#x\n", param->flags);

    read_dword(ptr, &param->annotation_count);
    TRACE("Annotation count: %u\n", param->annotation_count);

    hr = d3dx9_parse_effect_typedef(param, data, &ptr2, NULL, param->flags);
    if (hr != D3D_OK)
    {
        WARN("Failed to parse type definition\n");
        return hr;
    }

    hr = d3dx9_parse_init_value(param, data, data + offset, objects);
    if (hr != D3D_OK)
    {
        WARN("Failed to parse value\n");
        return hr;
    }

    if (param->annotation_count)
    {
        param->annotations = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                sizeof(*param->annotations) * param->annotation_count);
        if (!param->annotations)
        {
            ERR("Out of memory\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }

        for (i = 0; i < param->annotation_count; ++i)
        {
            hr = d3dx9_parse_effect_annotation(&param->annotations[i], data, ptr, objects);
            if (hr != D3D_OK)
            {
                WARN("Failed to parse annotation\n");
                goto err_out;
            }
        }
    }

    return D3D_OK;

err_out:

    if (param->annotations)
    {
        for (i = 0; i < param->annotation_count; ++i)
            free_parameter(&param->annotations[i], FALSE, FALSE);
        HeapFree(GetProcessHeap(), 0, param->annotations);
        param->annotations = NULL;
    }

    return hr;
}

static HRESULT d3dx9_parse_effect_pass(struct d3dx_pass *pass, const char *data, const char **ptr, D3DXHANDLE *objects)
{
    DWORD offset;
    HRESULT hr;
    unsigned int i;
    struct d3dx_state *states = NULL;
    char *name = NULL;

    read_dword(ptr, &offset);
    TRACE("Pass name offset: %#x\n", offset);
    hr = d3dx9_parse_name(&name, data + offset);
    if (hr != D3D_OK)
    {
        WARN("Failed to parse name\n");
        goto err_out;
    }

    read_dword(ptr, &pass->annotation_count);
    TRACE("Annotation count: %u\n", pass->annotation_count);

    read_dword(ptr, &pass->state_count);
    TRACE("State count: %u\n", pass->state_count);

    if (pass->annotation_count)
    {
        pass->annotations = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                sizeof(*pass->annotations) * pass->annotation_count);
        if (!pass->annotations)
        {
            ERR("Out of memory\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }

        for (i = 0; i < pass->annotation_count; ++i)
        {
            hr = d3dx9_parse_effect_annotation(&pass->annotations[i], data, ptr, objects);
            if (hr != D3D_OK)
            {
                WARN("Failed to parse annotation %u\n", i);
                goto err_out;
            }
        }
    }

    if (pass->state_count)
    {
        states = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*states) * pass->state_count);
        if (!states)
        {
            ERR("Out of memory\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }

        for (i = 0; i < pass->state_count; ++i)
        {
            hr = d3dx9_parse_state(&states[i], data, ptr, objects);
            if (hr != D3D_OK)
            {
                WARN("Failed to parse annotation %u\n", i);
                goto err_out;
            }
        }
    }

    pass->name = name;
    pass->states = states;

    return D3D_OK;

err_out:

    if (pass->annotations)
    {
        for (i = 0; i < pass->annotation_count; ++i)
            free_parameter(&pass->annotations[i], FALSE, FALSE);
        HeapFree(GetProcessHeap(), 0, pass->annotations);
        pass->annotations = NULL;
    }

    if (states)
    {
        for (i = 0; i < pass->state_count; ++i)
        {
            free_state(&states[i]);
        }
        HeapFree(GetProcessHeap(), 0, states);
    }

    HeapFree(GetProcessHeap(), 0, name);

    return hr;
}

static HRESULT d3dx9_parse_effect_technique(struct d3dx_technique *technique, const char *data, const char **ptr, D3DXHANDLE *objects)
{
    DWORD offset;
    HRESULT hr;
    unsigned int i;
    char *name = NULL;

    read_dword(ptr, &offset);
    TRACE("Technique name offset: %#x\n", offset);
    hr = d3dx9_parse_name(&name, data + offset);
    if (hr != D3D_OK)
    {
        WARN("Failed to parse name\n");
        goto err_out;
    }

    read_dword(ptr, &technique->annotation_count);
    TRACE("Annotation count: %u\n", technique->annotation_count);

    read_dword(ptr, &technique->pass_count);
    TRACE("Pass count: %u\n", technique->pass_count);

    if (technique->annotation_count)
    {
        technique->annotations = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                sizeof(*technique->annotations) * technique->annotation_count);
        if (!technique->annotations)
        {
            ERR("Out of memory\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }

        for (i = 0; i < technique->annotation_count; ++i)
        {
            hr = d3dx9_parse_effect_annotation(&technique->annotations[i], data, ptr, objects);
            if (hr != D3D_OK)
            {
                WARN("Failed to parse annotation %u\n", i);
                goto err_out;
            }
        }
    }

    if (technique->pass_count)
    {
        technique->passes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                sizeof(*technique->passes) * technique->pass_count);
        if (!technique->passes)
        {
            ERR("Out of memory\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }

        for (i = 0; i < technique->pass_count; ++i)
        {
            hr = d3dx9_parse_effect_pass(&technique->passes[i], data, ptr, objects);
            if (hr != D3D_OK)
            {
                WARN("Failed to parse pass %u\n", i);
                goto err_out;
            }
        }
    }

    technique->name = name;

    return D3D_OK;

err_out:

    if (technique->passes)
    {
        for (i = 0; i < technique->pass_count; ++i)
            free_pass(&technique->passes[i]);
        HeapFree(GetProcessHeap(), 0, technique->passes);
        technique->passes = NULL;
    }

    if (technique->annotations)
    {
        for (i = 0; i < technique->annotation_count; ++i)
            free_parameter(&technique->annotations[i], FALSE, FALSE);
        HeapFree(GetProcessHeap(), 0, technique->annotations);
        technique->annotations = NULL;
    }

    HeapFree(GetProcessHeap(), 0, name);

    return hr;
}

static HRESULT d3dx9_parse_resource(struct d3dx9_base_effect *base, const char *data, const char **ptr)
{
    DWORD technique_index;
    DWORD index, state_index, usage, element_index;
    struct d3dx_state *state;
    struct d3dx_parameter *param;
    HRESULT hr = E_FAIL;

    read_dword(ptr, &technique_index);
    TRACE("techn: %u\n", technique_index);

    read_dword(ptr, &index);
    TRACE("index: %u\n", index);

    read_dword(ptr, &element_index);
    TRACE("element_index: %u\n", element_index);

    read_dword(ptr, &state_index);
    TRACE("state_index: %u\n", state_index);

    read_dword(ptr, &usage);
    TRACE("usage: %u\n", usage);

    if (technique_index == 0xffffffff)
    {
        struct d3dx_parameter *parameter;
        struct d3dx_sampler *sampler;

        if (index >= base->parameter_count)
        {
            FIXME("Index out of bounds: index %u >= parameter_count %u\n", index, base->parameter_count);
            return E_FAIL;
        }

        parameter = &base->parameters[index];
        if (element_index != 0xffffffff)
        {
            if (element_index >= parameter->element_count && parameter->element_count != 0)
            {
                FIXME("Index out of bounds: element_index %u >= element_count %u\n", element_index, parameter->element_count);
                return E_FAIL;
            }

            if (parameter->element_count != 0) parameter = &parameter->members[element_index];
        }

        sampler = parameter->data;
        if (state_index >= sampler->state_count)
        {
            FIXME("Index out of bounds: state_index %u >= state_count %u\n", state_index, sampler->state_count);
            return E_FAIL;
        }

        state = &sampler->states[state_index];
    }
    else
    {
        struct d3dx_technique *technique;
        struct d3dx_pass *pass;

        if (technique_index >= base->technique_count)
        {
            FIXME("Index out of bounds: technique_index %u >= technique_count %u\n", technique_index, base->technique_count);
            return E_FAIL;
        }

        technique = &base->techniques[technique_index];
        if (index >= technique->pass_count)
        {
            FIXME("Index out of bounds: index %u >= pass_count %u\n", index, technique->pass_count);
            return E_FAIL;
        }

        pass = &technique->passes[index];
        if (state_index >= pass->state_count)
        {
            FIXME("Index out of bounds: state_index %u >= state_count %u\n", state_index, pass->state_count);
            return E_FAIL;
        }

        state = &pass->states[state_index];
    }

    param = state->parameter;

    switch (usage)
    {
        case 0:
            TRACE("usage 0: type %s\n", debug_d3dxparameter_type(param->type));
            switch (param->type)
            {
                case D3DXPT_VERTEXSHADER:
                case D3DXPT_PIXELSHADER:
                    state->type = ST_CONSTANT;
                    hr = d3dx9_parse_data(param, ptr, base->effect->device);
                    break;

                case D3DXPT_BOOL:
                case D3DXPT_INT:
                case D3DXPT_FLOAT:
                case D3DXPT_STRING:
                    state->type = ST_FXLC;
                    hr = d3dx9_copy_data(param->data, ptr);
                    break;

                default:
                    FIXME("Unhandled type %s\n", debug_d3dxparameter_type(param->type));
                    break;
            }
            break;

        case 1:
            state->type = ST_PARAMETER;
            hr = d3dx9_copy_data(param->data, ptr);
            if (hr == D3D_OK)
            {
                TRACE("Mapping to parameter %s\n", *(char **)param->data);
            }
            break;

        default:
            FIXME("Unknown usage %x\n", usage);
            break;
    }

    return hr;
}

static HRESULT d3dx9_parse_effect(struct d3dx9_base_effect *base, const char *data, UINT data_size, DWORD start)
{
    const char *ptr = data + start;
    D3DXHANDLE *objects = NULL;
    UINT stringcount, objectcount, resourcecount;
    HRESULT hr;
    UINT i;

    read_dword(&ptr, &base->parameter_count);
    TRACE("Parameter count: %u\n", base->parameter_count);

    read_dword(&ptr, &base->technique_count);
    TRACE("Technique count: %u\n", base->technique_count);

    skip_dword_unknown(&ptr, 1);

    read_dword(&ptr, &objectcount);
    TRACE("Object count: %u\n", objectcount);

    objects = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*objects) * objectcount);
    if (!objects)
    {
        ERR("Out of memory\n");
        hr = E_OUTOFMEMORY;
        goto err_out;
    }

    if (base->parameter_count)
    {
        base->parameters = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                sizeof(*base->parameters) * base->parameter_count);
        if (!base->parameters)
        {
            ERR("Out of memory\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }

        for (i = 0; i < base->parameter_count; ++i)
        {
            hr = d3dx9_parse_effect_parameter(&base->parameters[i], data, &ptr, objects);
            if (hr != D3D_OK)
            {
                WARN("Failed to parse parameter %u\n", i);
                goto err_out;
            }
        }
    }

    if (base->technique_count)
    {
        base->techniques = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                sizeof(*base->techniques) * base->technique_count);
        if (!base->techniques)
        {
            ERR("Out of memory\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }

        for (i = 0; i < base->technique_count; ++i)
        {
            hr = d3dx9_parse_effect_technique(&base->techniques[i], data, &ptr, objects);
            if (hr != D3D_OK)
            {
                WARN("Failed to parse technique %u\n", i);
                goto err_out;
            }
        }
    }

    read_dword(&ptr, &stringcount);
    TRACE("String count: %u\n", stringcount);

    read_dword(&ptr, &resourcecount);
    TRACE("Resource count: %u\n", resourcecount);

    for (i = 0; i < stringcount; ++i)
    {
        DWORD id;
        struct d3dx_parameter *param;

        read_dword(&ptr, &id);
        TRACE("Id: %u\n", id);

        param = get_parameter_struct(objects[id]);

        hr = d3dx9_parse_data(param, &ptr, base->effect->device);
        if (hr != D3D_OK)
        {
            WARN("Failed to parse data %u\n", i);
            goto err_out;
        }
    }

    for (i = 0; i < resourcecount; ++i)
    {
        TRACE("parse resource %u\n", i);

        hr = d3dx9_parse_resource(base, data, &ptr);
        if (hr != D3D_OK)
        {
            WARN("Failed to parse resource %u\n", i);
            goto err_out;
        }
    }

    HeapFree(GetProcessHeap(), 0, objects);

    return D3D_OK;

err_out:

    if (base->techniques)
    {
        for (i = 0; i < base->technique_count; ++i)
            free_technique(&base->techniques[i]);
        HeapFree(GetProcessHeap(), 0, base->techniques);
        base->techniques = NULL;
    }

    if (base->parameters)
    {
        for (i = 0; i < base->parameter_count; ++i)
        {
            free_parameter(&base->parameters[i], FALSE, FALSE);
        }
        HeapFree(GetProcessHeap(), 0, base->parameters);
        base->parameters = NULL;
    }

    HeapFree(GetProcessHeap(), 0, objects);

    return hr;
}

static HRESULT d3dx9_base_effect_init(struct d3dx9_base_effect *base,
        const char *data, SIZE_T data_size, struct ID3DXEffectImpl *effect)
{
    DWORD tag, offset;
    const char *ptr = data;
    HRESULT hr;

    TRACE("base %p, data %p, data_size %lu, effect %p\n", base, data, data_size, effect);

    base->effect = effect;

    read_dword(&ptr, &tag);
    TRACE("Tag: %x\n", tag);

    if (tag != d3dx9_effect_version(9, 1))
    {
        /* todo: compile hlsl ascii code */
        FIXME("HLSL ascii effects not supported, yet\n");

        /* Show the start of the shader for debugging info. */
        TRACE("effect:\n%s\n", debugstr_an(data, data_size > 40 ? 40 : data_size));
    }
    else
    {
        read_dword(&ptr, &offset);
        TRACE("Offset: %x\n", offset);

        hr = d3dx9_parse_effect(base, ptr, data_size, offset);
        if (hr != D3D_OK)
        {
            FIXME("Failed to parse effect.\n");
            return hr;
        }
    }

    return D3D_OK;
}

static HRESULT d3dx9_effect_init(struct ID3DXEffectImpl *effect, struct IDirect3DDevice9 *device,
        const char *data, SIZE_T data_size, struct ID3DXEffectPool *pool)
{
    HRESULT hr;

    TRACE("effect %p, device %p, data %p, data_size %lu, pool %p\n", effect, device, data, data_size, pool);

    effect->ID3DXEffect_iface.lpVtbl = &ID3DXEffect_Vtbl;
    effect->ref = 1;

    if (pool) pool->lpVtbl->AddRef(pool);
    effect->pool = pool;

    IDirect3DDevice9_AddRef(device);
    effect->device = device;

    if (FAILED(hr = d3dx9_base_effect_init(&effect->base_effect, data, data_size, effect)))
    {
        FIXME("Failed to parse effect, hr %#x.\n", hr);
        free_effect(effect);
        return hr;
    }

    /* initialize defaults - check because of unsupported ascii effects */
    if (effect->base_effect.techniques)
    {
        effect->active_technique = &effect->base_effect.techniques[0];
        effect->active_pass = NULL;
    }

    return D3D_OK;
}

HRESULT WINAPI D3DXCreateEffectEx(struct IDirect3DDevice9 *device, const void *srcdata, UINT srcdatalen,
        const D3DXMACRO *defines, struct ID3DXInclude *include, const char *skip_constants, DWORD flags,
        struct ID3DXEffectPool *pool, struct ID3DXEffect **effect, struct ID3DXBuffer **compilation_errors)
{
    struct ID3DXEffectImpl *object;
    HRESULT hr;

    FIXME("(%p, %p, %u, %p, %p, %p, %#x, %p, %p, %p): semi-stub\n", device, srcdata, srcdatalen, defines, include,
        skip_constants, flags, pool, effect, compilation_errors);

    if (compilation_errors)
        *compilation_errors = NULL;

    if (!device || !srcdata)
        return D3DERR_INVALIDCALL;

    if (!srcdatalen)
        return E_FAIL;

    /* Native dll allows effect to be null so just return D3D_OK after doing basic checks */
    if (!effect)
        return D3D_OK;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    hr = d3dx9_effect_init(object, device, srcdata, srcdatalen, pool);
    if (FAILED(hr))
    {
        WARN("Failed to initialize shader reflection\n");
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    *effect = &object->ID3DXEffect_iface;

    TRACE("Created ID3DXEffect %p\n", object);

    return D3D_OK;
}

HRESULT WINAPI D3DXCreateEffect(struct IDirect3DDevice9 *device, const void *srcdata, UINT srcdatalen,
        const D3DXMACRO *defines, struct ID3DXInclude *include, DWORD flags,
        struct ID3DXEffectPool *pool, struct ID3DXEffect **effect, struct ID3DXBuffer **compilation_errors)
{
    TRACE("(%p, %p, %u, %p, %p, %#x, %p, %p, %p): Forwarded to D3DXCreateEffectEx\n", device, srcdata, srcdatalen, defines,
        include, flags, pool, effect, compilation_errors);

    return D3DXCreateEffectEx(device, srcdata, srcdatalen, defines, include, NULL, flags, pool, effect, compilation_errors);
}

static HRESULT d3dx9_effect_compiler_init(struct ID3DXEffectCompilerImpl *compiler, const char *data, SIZE_T data_size)
{
    HRESULT hr;

    TRACE("effect %p, data %p, data_size %lu\n", compiler, data, data_size);

    compiler->ID3DXEffectCompiler_iface.lpVtbl = &ID3DXEffectCompiler_Vtbl;
    compiler->ref = 1;

    if (FAILED(hr = d3dx9_base_effect_init(&compiler->base_effect, data, data_size, NULL)))
    {
        FIXME("Failed to parse effect, hr %#x.\n", hr);
        free_effect_compiler(compiler);
        return hr;
    }

    return D3D_OK;
}

HRESULT WINAPI D3DXCreateEffectCompiler(const char *srcdata, UINT srcdatalen, const D3DXMACRO *defines,
        ID3DXInclude *include, DWORD flags, ID3DXEffectCompiler **compiler, ID3DXBuffer **parse_errors)
{
    struct ID3DXEffectCompilerImpl *object;
    HRESULT hr;

    TRACE("srcdata %p, srcdatalen %u, defines %p, include %p, flags %#x, compiler %p, parse_errors %p\n",
            srcdata, srcdatalen, defines, include, flags, compiler, parse_errors);

    if (!srcdata || !compiler)
    {
        WARN("Invalid arguments supplied\n");
        return D3DERR_INVALIDCALL;
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    hr = d3dx9_effect_compiler_init(object, srcdata, srcdatalen);
    if (FAILED(hr))
    {
        WARN("Failed to initialize effect compiler\n");
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    *compiler = &object->ID3DXEffectCompiler_iface;

    TRACE("Created ID3DXEffectCompiler %p\n", object);

    return D3D_OK;
}

struct ID3DXEffectPoolImpl
{
    ID3DXEffectPool ID3DXEffectPool_iface;
    LONG ref;
};

static inline struct ID3DXEffectPoolImpl *impl_from_ID3DXEffectPool(ID3DXEffectPool *iface)
{
    return CONTAINING_RECORD(iface, struct ID3DXEffectPoolImpl, ID3DXEffectPool_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI ID3DXEffectPoolImpl_QueryInterface(ID3DXEffectPool *iface, REFIID riid, void **object)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_ID3DXEffectPool))
    {
        iface->lpVtbl->AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("Interface %s not found\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXEffectPoolImpl_AddRef(ID3DXEffectPool *iface)
{
    struct ID3DXEffectPoolImpl *This = impl_from_ID3DXEffectPool(iface);

    TRACE("(%p)->(): AddRef from %u\n", This, This->ref);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ID3DXEffectPoolImpl_Release(ID3DXEffectPool *iface)
{
    struct ID3DXEffectPoolImpl *This = impl_from_ID3DXEffectPool(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): Release from %u\n", This, ref + 1);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static const struct ID3DXEffectPoolVtbl ID3DXEffectPool_Vtbl =
{
    /*** IUnknown methods ***/
    ID3DXEffectPoolImpl_QueryInterface,
    ID3DXEffectPoolImpl_AddRef,
    ID3DXEffectPoolImpl_Release
};

HRESULT WINAPI D3DXCreateEffectPool(ID3DXEffectPool **pool)
{
    struct ID3DXEffectPoolImpl *object;

    TRACE("(%p)\n", pool);

    if (!pool)
        return D3DERR_INVALIDCALL;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->ID3DXEffectPool_iface.lpVtbl = &ID3DXEffectPool_Vtbl;
    object->ref = 1;

    *pool = &object->ID3DXEffectPool_iface;

    return S_OK;
}

HRESULT WINAPI D3DXCreateEffectFromFileExW(struct IDirect3DDevice9 *device, const WCHAR *srcfile,
        const D3DXMACRO *defines, struct ID3DXInclude *include, const char *skipconstants, DWORD flags,
        struct ID3DXEffectPool *pool, struct ID3DXEffect **effect, struct ID3DXBuffer **compilationerrors)
{
    LPVOID buffer;
    HRESULT ret;
    DWORD size;

    TRACE("(%s): relay\n", debugstr_w(srcfile));

    if (!device || !srcfile)
        return D3DERR_INVALIDCALL;

    ret = map_view_of_file(srcfile, &buffer, &size);

    if (FAILED(ret))
        return D3DXERR_INVALIDDATA;

    ret = D3DXCreateEffectEx(device, buffer, size, defines, include, skipconstants, flags, pool, effect, compilationerrors);
    UnmapViewOfFile(buffer);

    return ret;
}

HRESULT WINAPI D3DXCreateEffectFromFileExA(struct IDirect3DDevice9 *device, const char *srcfile,
        const D3DXMACRO *defines, struct ID3DXInclude *include, const char *skipconstants, DWORD flags,
        struct ID3DXEffectPool *pool, struct ID3DXEffect **effect, struct ID3DXBuffer **compilationerrors)
{
    LPWSTR srcfileW;
    HRESULT ret;
    DWORD len;

    TRACE("(void): relay\n");

    if (!srcfile)
        return D3DERR_INVALIDCALL;

    len = MultiByteToWideChar(CP_ACP, 0, srcfile, -1, NULL, 0);
    srcfileW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(*srcfileW));
    MultiByteToWideChar(CP_ACP, 0, srcfile, -1, srcfileW, len);

    ret = D3DXCreateEffectFromFileExW(device, srcfileW, defines, include, skipconstants, flags, pool, effect, compilationerrors);
    HeapFree(GetProcessHeap(), 0, srcfileW);

    return ret;
}

HRESULT WINAPI D3DXCreateEffectFromFileW(struct IDirect3DDevice9 *device, const WCHAR *srcfile,
        const D3DXMACRO *defines, struct ID3DXInclude *include, DWORD flags, struct ID3DXEffectPool *pool,
        struct ID3DXEffect **effect, struct ID3DXBuffer **compilationerrors)
{
    TRACE("(void): relay\n");
    return D3DXCreateEffectFromFileExW(device, srcfile, defines, include, NULL, flags, pool, effect, compilationerrors);
}

HRESULT WINAPI D3DXCreateEffectFromFileA(struct IDirect3DDevice9 *device, const char *srcfile,
        const D3DXMACRO *defines, struct ID3DXInclude *include, DWORD flags, struct ID3DXEffectPool *pool,
        struct ID3DXEffect **effect, struct ID3DXBuffer **compilationerrors)
{
    TRACE("(void): relay\n");
    return D3DXCreateEffectFromFileExA(device, srcfile, defines, include, NULL, flags, pool, effect, compilationerrors);
}

HRESULT WINAPI D3DXCreateEffectFromResourceExW(struct IDirect3DDevice9 *device, HMODULE srcmodule,
        const WCHAR *srcresource, const D3DXMACRO *defines, struct ID3DXInclude *include, const char *skipconstants,
        DWORD flags, struct ID3DXEffectPool *pool, struct ID3DXEffect **effect, struct ID3DXBuffer **compilationerrors)
{
    HRSRC resinfo;
    void *buffer;
    DWORD size;

    TRACE("device %p, srcmodule %p, srcresource %s, defines %p, include %p, skipconstants %s, "
            "flags %#x, pool %p, effect %p, compilationerrors %p.\n",
            device, srcmodule, debugstr_w(srcresource), defines, include, debugstr_a(skipconstants),
            flags, pool, effect, compilationerrors);

    if (!device)
        return D3DERR_INVALIDCALL;

    if (!(resinfo = FindResourceW(srcmodule, srcresource, (const WCHAR *)RT_RCDATA)))
        return D3DXERR_INVALIDDATA;

    if (FAILED(load_resource_into_memory(srcmodule, resinfo, &buffer, &size)))
        return D3DXERR_INVALIDDATA;

    return D3DXCreateEffectEx(device, buffer, size, defines, include,
            skipconstants, flags, pool, effect, compilationerrors);
}

HRESULT WINAPI D3DXCreateEffectFromResourceExA(struct IDirect3DDevice9 *device, HMODULE srcmodule,
        const char *srcresource, const D3DXMACRO *defines, struct ID3DXInclude *include, const char *skipconstants,
        DWORD flags, struct ID3DXEffectPool *pool, struct ID3DXEffect **effect, struct ID3DXBuffer **compilationerrors)
{
    HRSRC resinfo;
    void *buffer;
    DWORD size;

    TRACE("device %p, srcmodule %p, srcresource %s, defines %p, include %p, skipconstants %s, "
            "flags %#x, pool %p, effect %p, compilationerrors %p.\n",
            device, srcmodule, debugstr_a(srcresource), defines, include, debugstr_a(skipconstants),
            flags, pool, effect, compilationerrors);

    if (!device)
        return D3DERR_INVALIDCALL;

    if (!(resinfo = FindResourceA(srcmodule, srcresource, (const char *)RT_RCDATA)))
        return D3DXERR_INVALIDDATA;

    if (FAILED(load_resource_into_memory(srcmodule, resinfo, &buffer, &size)))
        return D3DXERR_INVALIDDATA;

    return D3DXCreateEffectEx(device, buffer, size, defines, include,
            skipconstants, flags, pool, effect, compilationerrors);
}

HRESULT WINAPI D3DXCreateEffectFromResourceW(struct IDirect3DDevice9 *device, HMODULE srcmodule,
        const WCHAR *srcresource, const D3DXMACRO *defines, struct ID3DXInclude *include, DWORD flags,
        struct ID3DXEffectPool *pool, struct ID3DXEffect **effect, struct ID3DXBuffer **compilationerrors)
{
    TRACE("(void): relay\n");
    return D3DXCreateEffectFromResourceExW(device, srcmodule, srcresource, defines, include, NULL, flags, pool, effect, compilationerrors);
}

HRESULT WINAPI D3DXCreateEffectFromResourceA(struct IDirect3DDevice9 *device, HMODULE srcmodule,
        const char *srcresource, const D3DXMACRO *defines, struct ID3DXInclude *include, DWORD flags,
        struct ID3DXEffectPool *pool, struct ID3DXEffect **effect, struct ID3DXBuffer **compilationerrors)
{
    TRACE("(void): relay\n");
    return D3DXCreateEffectFromResourceExA(device, srcmodule, srcresource, defines, include, NULL, flags, pool, effect, compilationerrors);
}

HRESULT WINAPI D3DXCreateEffectCompilerFromFileW(const WCHAR *srcfile, const D3DXMACRO *defines,
        ID3DXInclude *include, DWORD flags, ID3DXEffectCompiler **effectcompiler, ID3DXBuffer **parseerrors)
{
    LPVOID buffer;
    HRESULT ret;
    DWORD size;

    TRACE("(%s): relay\n", debugstr_w(srcfile));

    if (!srcfile)
        return D3DERR_INVALIDCALL;

    ret = map_view_of_file(srcfile, &buffer, &size);

    if (FAILED(ret))
        return D3DXERR_INVALIDDATA;

    ret = D3DXCreateEffectCompiler(buffer, size, defines, include, flags, effectcompiler, parseerrors);
    UnmapViewOfFile(buffer);

    return ret;
}

HRESULT WINAPI D3DXCreateEffectCompilerFromFileA(const char *srcfile, const D3DXMACRO *defines,
        ID3DXInclude *include, DWORD flags, ID3DXEffectCompiler **effectcompiler, ID3DXBuffer **parseerrors)
{
    LPWSTR srcfileW;
    HRESULT ret;
    DWORD len;

    TRACE("(void): relay\n");

    if (!srcfile)
        return D3DERR_INVALIDCALL;

    len = MultiByteToWideChar(CP_ACP, 0, srcfile, -1, NULL, 0);
    srcfileW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(*srcfileW));
    MultiByteToWideChar(CP_ACP, 0, srcfile, -1, srcfileW, len);

    ret = D3DXCreateEffectCompilerFromFileW(srcfileW, defines, include, flags, effectcompiler, parseerrors);
    HeapFree(GetProcessHeap(), 0, srcfileW);

    return ret;
}

HRESULT WINAPI D3DXCreateEffectCompilerFromResourceA(HMODULE srcmodule, const char *srcresource,
        const D3DXMACRO *defines, ID3DXInclude *include, DWORD flags,
        ID3DXEffectCompiler **effectcompiler, ID3DXBuffer **parseerrors)
{
    HRSRC resinfo;
    void *buffer;
    DWORD size;

    TRACE("srcmodule %p, srcresource %s, defines %p, include %p, flags %#x, effectcompiler %p, parseerrors %p.\n",
            srcmodule, debugstr_a(srcresource), defines, include, flags, effectcompiler, parseerrors);

    if (!(resinfo = FindResourceA(srcmodule, srcresource, (const char *)RT_RCDATA)))
        return D3DXERR_INVALIDDATA;

    if (FAILED(load_resource_into_memory(srcmodule, resinfo, &buffer, &size)))
        return D3DXERR_INVALIDDATA;

    return D3DXCreateEffectCompiler(buffer, size, defines, include, flags, effectcompiler, parseerrors);
}

HRESULT WINAPI D3DXCreateEffectCompilerFromResourceW(HMODULE srcmodule, const WCHAR *srcresource,
        const D3DXMACRO *defines, ID3DXInclude *include, DWORD flags,
        ID3DXEffectCompiler **effectcompiler, ID3DXBuffer **parseerrors)
{
    HRSRC resinfo;
    void *buffer;
    DWORD size;

    TRACE("srcmodule %p, srcresource %s, defines %p, include %p, flags %#x, effectcompiler %p, parseerrors %p.\n",
            srcmodule, debugstr_w(srcresource), defines, include, flags, effectcompiler, parseerrors);

    if (!(resinfo = FindResourceW(srcmodule, srcresource, (const WCHAR *)RT_RCDATA)))
        return D3DXERR_INVALIDDATA;

    if (FAILED(load_resource_into_memory(srcmodule, resinfo, &buffer, &size)))
        return D3DXERR_INVALIDDATA;

    return D3DXCreateEffectCompiler(buffer, size, defines, include, flags, effectcompiler, parseerrors);
}

HRESULT WINAPI D3DXDisassembleEffect(ID3DXEffect *effect, BOOL enable_color_code, ID3DXBuffer **disassembly)
{
    FIXME("(%p, %u, %p): stub\n", effect, enable_color_code, disassembly);

    return D3DXERR_INVALIDDATA;
}

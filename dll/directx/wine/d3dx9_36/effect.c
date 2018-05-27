/*
 * Copyright 2010 Christian Costa
 * Copyright 2011 Rico Schüller
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

#include <stdio.h>

#include "d3dx9_private.h"
#include "d3dcompiler.h"

/* Constants for special INT/FLOAT conversation */
#define INT_FLOAT_MULTI 255.0f
#define INT_FLOAT_MULTI_INVERSE (1/INT_FLOAT_MULTI)

static const char parameter_magic_string[4] = {'@', '!', '#', '\xFF'};

#define PARAMETER_FLAG_SHARED 1

#define INITIAL_POOL_SIZE 16

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
    ST_ARRAY_SELECTOR,
};

struct d3dx_object
{
    UINT size;
    void *data;
    struct d3dx_parameter *param;
    BOOL creation_failed;
};

struct d3dx_state
{
    UINT operation;
    UINT index;
    enum STATE_TYPE type;
    struct d3dx_parameter parameter;
    struct d3dx_parameter *referenced_param;
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

    ULONG64 update_version;
};

struct d3dx_technique
{
    char *name;
    UINT pass_count;
    UINT annotation_count;

    struct d3dx_parameter *annotations;
    struct d3dx_pass *passes;

    struct IDirect3DStateBlock9 *saved_state;
};

struct d3dx9_base_effect
{
    struct ID3DXEffectImpl *effect;

    UINT parameter_count;
    UINT technique_count;
    UINT object_count;

    struct d3dx_top_level_parameter *parameters;
    struct d3dx_technique *techniques;
    struct d3dx_object *objects;

    struct d3dx_effect_pool *pool;
    DWORD flags;

    ULONG64 version_counter;

    struct wine_rb_tree param_tree;
    char *full_name_tmp;
    unsigned int full_name_tmp_size;
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
    DWORD begin_flags;

    D3DLIGHT9 current_light[8];
    unsigned int light_updated;
    D3DMATERIAL9 current_material;
    BOOL material_updated;
};

#define INITIAL_SHARED_DATA_SIZE 4

struct d3dx_effect_pool
{
    ID3DXEffectPool ID3DXEffectPool_iface;
    LONG refcount;

    struct d3dx_shared_data *shared_data;
    unsigned int size;

    ULONG64 version_counter;
};

struct ID3DXEffectCompilerImpl
{
    ID3DXEffectCompiler ID3DXEffectCompiler_iface;
    LONG ref;
};

static struct d3dx_parameter *get_annotation_by_name(struct d3dx9_base_effect *base,
        unsigned int count, struct d3dx_parameter *parameters, const char *name);
static HRESULT d3dx9_parse_state(struct d3dx9_base_effect *base, struct d3dx_state *state,
        const char *data, const char **ptr, struct d3dx_object *objects);
static void free_parameter(struct d3dx_parameter *param, BOOL element, BOOL child);

typedef BOOL (*walk_parameter_dep_func)(void *data, struct d3dx_parameter *param);

static const struct
{
    enum STATE_CLASS class;
    UINT op;
    const char *name;
}
state_table[] =
{
    /* Render states */
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
    /* Lightenable */
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

    WARN("Skipping %u unknown DWORDs:\n", count);
    for (i = 0; i < count; ++i)
    {
        read_dword(ptr, &d);
        WARN("\t0x%08x\n", d);
    }
}

static inline D3DXHANDLE get_parameter_handle(struct d3dx_parameter *parameter)
{
    return (D3DXHANDLE)parameter;
}

static inline D3DXHANDLE get_technique_handle(struct d3dx_technique *technique)
{
    return (D3DXHANDLE)technique;
}

static inline D3DXHANDLE get_pass_handle(struct d3dx_pass *pass)
{
    return (D3DXHANDLE)pass;
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

static struct d3dx_parameter *get_valid_parameter(struct d3dx9_base_effect *base, D3DXHANDLE parameter)
{
    struct d3dx_parameter *handle_param = (struct d3dx_parameter *)parameter;

    if (handle_param && !strncmp(handle_param->magic_string, parameter_magic_string,
            sizeof(parameter_magic_string)))
        return handle_param;

    return base->flags & D3DXFX_LARGEADDRESSAWARE ? NULL : get_parameter_by_name(base, NULL, parameter);
}

static void free_state(struct d3dx_state *state)
{
    free_parameter(&state->parameter, FALSE, FALSE);
}

static void free_object(struct d3dx_object *object)
{
    HeapFree(GetProcessHeap(), 0, object->data);
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

static void d3dx_pool_release_shared_parameter(struct d3dx_top_level_parameter *param);

static void free_parameter_data(struct d3dx_parameter *param, BOOL child)
{
    if (!param->data)
        return;
    if (param->class == D3DXPC_OBJECT && !param->element_count)
    {
        switch (param->type)
        {
            case D3DXPT_STRING:
                HeapFree(GetProcessHeap(), 0, *(char **)param->data);
                break;

            case D3DXPT_TEXTURE:
            case D3DXPT_TEXTURE1D:
            case D3DXPT_TEXTURE2D:
            case D3DXPT_TEXTURE3D:
            case D3DXPT_TEXTURECUBE:
            case D3DXPT_PIXELSHADER:
            case D3DXPT_VERTEXSHADER:
                if (*(IUnknown **)param->data) IUnknown_Release(*(IUnknown **)param->data);
                break;

            case D3DXPT_SAMPLER:
            case D3DXPT_SAMPLER1D:
            case D3DXPT_SAMPLER2D:
            case D3DXPT_SAMPLER3D:
            case D3DXPT_SAMPLERCUBE:
                free_sampler((struct d3dx_sampler *)param->data);
                break;

            default:
                FIXME("Unhandled type %s\n", debug_d3dxparameter_type(param->type));
                break;
        }
    }
    if (!child)
        HeapFree(GetProcessHeap(), 0, param->data);
}

static void free_parameter(struct d3dx_parameter *param, BOOL element, BOOL child)
{
    unsigned int i;

    TRACE("Free parameter %p, name %s, type %s, element %#x, child %#x.\n", param, param->name,
            debug_d3dxparameter_type(param->type), element, child);

    if (param->param_eval)
        d3dx_free_param_eval(param->param_eval);

    if (param->members)
    {
        unsigned int count = param->element_count ? param->element_count : param->member_count;

        for (i = 0; i < count; ++i)
            free_parameter(&param->members[i], param->element_count != 0, TRUE);
        HeapFree(GetProcessHeap(), 0, param->members);
    }

    free_parameter_data(param, child);

    /* only the parent has to release name and semantic */
    if (!element)
    {
        HeapFree(GetProcessHeap(), 0, param->name);
        HeapFree(GetProcessHeap(), 0, param->semantic);
    }
}

static void free_top_level_parameter(struct d3dx_top_level_parameter *param)
{
    if (param->annotations)
    {
        unsigned int i;

        for (i = 0; i < param->annotation_count; ++i)
            free_parameter(&param->annotations[i], FALSE, FALSE);
        HeapFree(GetProcessHeap(), 0, param->annotations);
    }
    d3dx_pool_release_shared_parameter(param);
    free_parameter(&param->param, FALSE, FALSE);
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

    if (technique->saved_state)
    {
        IDirect3DStateBlock9_Release(technique->saved_state);
        technique->saved_state = NULL;
    }

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

    heap_free(base->full_name_tmp);

    if (base->parameters)
    {
        for (i = 0; i < base->parameter_count; ++i)
            free_top_level_parameter(&base->parameters[i]);
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

    if (base->objects)
    {
        for (i = 0; i < base->object_count; ++i)
        {
            free_object(&base->objects[i]);
        }
        HeapFree(GetProcessHeap(), 0, base->objects);
        base->objects = NULL;
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

static void set_vector(struct d3dx_parameter *param, const D3DXVECTOR4 *vector)
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

static void set_matrix(struct d3dx_parameter *param, const D3DXMATRIX *matrix)
{
    UINT i, k;

    if (param->type == D3DXPT_FLOAT)
    {
        if (param->columns == 4)
            memcpy(param->data, matrix->u.m, param->rows * 4 * sizeof(float));
        else
            for (i = 0; i < param->rows; ++i)
                memcpy((float *)param->data + i * param->columns, matrix->u.m + i, param->columns * sizeof(float));
        return;
    }

    for (i = 0; i < param->rows; ++i)
    {
        for (k = 0; k < param->columns; ++k)
        {
            set_number((FLOAT *)param->data + i * param->columns + k, param->type,
                    &matrix->u.m[i][k], D3DXPT_FLOAT);
        }
    }
}

static void set_matrix_transpose(struct d3dx_parameter *param, const D3DXMATRIX *matrix)
{
    UINT i, k;

    for (i = 0; i < param->rows; ++i)
    {
        for (k = 0; k < param->columns; ++k)
        {
            set_number((FLOAT *)param->data + i * param->columns + k, param->type,
                    &matrix->u.m[k][i], D3DXPT_FLOAT);
        }
    }
}

static struct d3dx_parameter *get_parameter_element_by_name(struct d3dx9_base_effect *base,
        struct d3dx_parameter *parameter, const char *name)
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
                return get_parameter_by_name(base, temp_parameter, part);

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

static struct d3dx_parameter *get_annotation_by_name(struct d3dx9_base_effect *base,
        unsigned int count, struct d3dx_parameter *annotations, const char *name)
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
                    return get_parameter_by_name(base, temp_parameter, part);

                case '[':
                    return get_parameter_element_by_name(base, temp_parameter, part);

                default:
                    FIXME("Unhandled case \"%c\"\n", *--part);
                    break;
            }
        }
    }

    TRACE("Annotation not found\n");
    return NULL;
}

struct d3dx_parameter *get_parameter_by_name(struct d3dx9_base_effect *base,
        struct d3dx_parameter *parameter, const char *name)
{
    struct d3dx_parameter *temp_parameter;
    unsigned int name_len, param_name_len;
    unsigned int i, count, length;
    struct wine_rb_entry *entry;
    unsigned int full_name_size;
    const char *part;
    char *full_name;

    TRACE("base %p, parameter %p, name %s\n", base, parameter, debugstr_a(name));

    if (!name || !*name) return NULL;

    if (!parameter)
    {
        if ((entry = wine_rb_get(&base->param_tree, name)))
            return WINE_RB_ENTRY_VALUE(entry, struct d3dx_parameter, rb_entry);
        return NULL;
    }

    /* Pass / technique annotations are not in the parameters tree. */
    if (parameter->full_name)
    {
        name_len = strlen(name);
        param_name_len = strlen(parameter->full_name);
        full_name_size = name_len + param_name_len + 2;
        if (base->full_name_tmp_size < full_name_size)
        {
            if (!(full_name = heap_realloc(base->full_name_tmp, full_name_size)))
            {
                ERR("Out of memory.\n");
                return NULL;
            }
            base->full_name_tmp = full_name;
            base->full_name_tmp_size = full_name_size;
        }
        else
        {
            full_name = base->full_name_tmp;
        }
        memcpy(full_name, parameter->full_name, param_name_len);
        full_name[param_name_len] = '.';
        memcpy(full_name + param_name_len + 1, name, name_len);
        full_name[param_name_len + 1 + name_len] = 0;

        if ((entry = wine_rb_get(&base->param_tree, full_name)))
            return WINE_RB_ENTRY_VALUE(entry, struct d3dx_parameter, rb_entry);
        return NULL;
    }

    count = parameter ? parameter->member_count : base->parameter_count;

    length = strcspn( name, "[.@" );
    part = name + length;

    for (i = 0; i < count; i++)
    {
        temp_parameter = !parameter ? &base->parameters[i].param
                : &parameter->members[i];

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
                    return get_parameter_by_name(base, temp_parameter, part);

                case '@':
                {
                    struct d3dx_top_level_parameter *top_param
                            = top_level_parameter_from_parameter(temp_parameter);

                    return parameter ? NULL : get_annotation_by_name(base, top_param->annotation_count,
                            top_param->annotations, part);
                }
                case '[':
                    return get_parameter_element_by_name(base, temp_parameter, part);

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
    desc->Annotations = is_top_level_parameter(param)
            ? top_level_parameter_from_parameter(param)->annotation_count : 0;
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

static HRESULT d3dx9_get_param_value_ptr(struct d3dx_pass *pass, struct d3dx_state *state,
        void **param_value, struct d3dx_parameter **out_param,
        BOOL update_all, BOOL *param_dirty)
{
    struct d3dx_parameter *param = &state->parameter;

    *param_value = NULL;
    *out_param = NULL;
    *param_dirty = FALSE;

    switch (state->type)
    {
        case ST_PARAMETER:
            param = state->referenced_param;
            *param_dirty = is_param_dirty(param, pass->update_version);
            /* fallthrough */
        case ST_CONSTANT:
            *out_param = param;
            *param_value = param->data;
            return D3D_OK;
        case ST_ARRAY_SELECTOR:
        {
            unsigned int array_idx;
            static const struct d3dx_parameter array_idx_param =
                {"", NULL, NULL, NULL, NULL, D3DXPC_SCALAR, D3DXPT_INT, 1, 1, 0, 0, 0, sizeof(array_idx)};
            HRESULT hr;
            struct d3dx_parameter *ref_param, *selected_param;

            if (!param->param_eval)
            {
                FIXME("Preshader structure is null.\n");
                return D3DERR_INVALIDCALL;
            }
            /* We override with the update_version of the pass because we want
             * to force index recomputation and check for out of bounds. */
            if (is_param_eval_input_dirty(param->param_eval, pass->update_version))
            {
                if (FAILED(hr = d3dx_evaluate_parameter(param->param_eval, &array_idx_param, &array_idx)))
                    return hr;
            }
            else
            {
                array_idx = state->index;
            }
            ref_param = state->referenced_param;
            TRACE("Array index %u, stored array index %u, element_count %u.\n", array_idx, state->index,
                    ref_param->element_count);
            /* According to the tests, native d3dx handles the case of array index evaluated to -1
             * in a specific way, always selecting first array element and not returning error. */
            if (array_idx == ~0u)
            {
                WARN("Array index is -1, setting to 0.\n");
                array_idx = 0;
            }

            if (array_idx >= ref_param->element_count)
            {
                WARN("Computed array index %u is larger than array size %u.\n",
                        array_idx, ref_param->element_count);
                return E_FAIL;
            }
            selected_param = &ref_param->members[array_idx];
            *param_dirty = state->index != array_idx || is_param_dirty(selected_param, pass->update_version);
            state->index = array_idx;

            *param_value = selected_param->data;
            *out_param = selected_param;
            return D3D_OK;
        }
        case ST_FXLC:
            if (param->param_eval)
            {
                *out_param = param;
                *param_value = param->data;
                /* We check with the update_version of the pass because the
                 * same preshader might be used by both the vertex and the
                 * pixel shader (that can happen e.g. for sampler states). */
                if (update_all || is_param_eval_input_dirty(param->param_eval, pass->update_version))
                {
                    *param_dirty = TRUE;
                    return d3dx_evaluate_parameter(param->param_eval, param, *param_value);
                }
                else
                    return D3D_OK;
            }
            else
            {
                FIXME("No preshader for FXLC parameter.\n");
                return D3DERR_INVALIDCALL;
            }
    }
    return E_NOTIMPL;
}

static HRESULT d3dx9_base_effect_get_pass_desc(struct d3dx9_base_effect *base,
        D3DXHANDLE pass_handle, D3DXPASS_DESC *desc)
{
    struct d3dx_pass *pass = get_valid_pass(base, pass_handle);
    unsigned int i;

    if (!desc || !pass)
    {
        WARN("Invalid argument specified.\n");
        return D3DERR_INVALIDCALL;
    }

    desc->Name = pass->name;
    desc->Annotations = pass->annotation_count;

    desc->pVertexShaderFunction = NULL;
    desc->pPixelShaderFunction = NULL;

    if (base->flags & D3DXFX_NOT_CLONEABLE)
        return D3D_OK;

    for (i = 0; i < pass->state_count; ++i)
    {
        struct d3dx_state *state = &pass->states[i];

        if (state_table[state->operation].class == SC_VERTEXSHADER
                || state_table[state->operation].class == SC_PIXELSHADER)
        {
            struct d3dx_parameter *param;
            void *param_value;
            BOOL param_dirty;
            HRESULT hr;
            void *data;

            if (FAILED(hr = d3dx9_get_param_value_ptr(pass, &pass->states[i], &param_value, &param,
                    FALSE, &param_dirty)))
                return hr;

            data = param->object_id ? base->objects[param->object_id].data : NULL;
            if (state_table[state->operation].class == SC_VERTEXSHADER)
                desc->pVertexShaderFunction = data;
            else
                desc->pPixelShaderFunction = data;
        }
    }

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
            return get_parameter_handle(&base->parameters[index].param);
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

    WARN("Parameter not found.\n");

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
            temp_param = &base->parameters[i].param;

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

    WARN("Parameter not found.\n");

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
            return get_parameter_handle(&base->parameters[index].param);
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

    WARN("Parameter not found.\n");

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

    WARN("Technique not found.\n");

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

    WARN("Pass not found.\n");

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

    WARN("Pass not found.\n");

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
        if (is_top_level_parameter(param))
        {
            struct d3dx_top_level_parameter *top_param
                    = top_level_parameter_from_parameter(param);

            *annotations = top_param->annotations;
            return top_param->annotation_count;
        }
        else
        {
            *annotations = NULL;
            return 0;
        }
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

    WARN("Annotation not found.\n");

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

    annotation = get_annotation_by_name(base, annotation_count, annotations, name);
    if (annotation)
    {
        TRACE("Returning parameter %p\n", annotation);
        return get_parameter_handle(annotation);
    }

    WARN("Annotation not found.\n");

    return NULL;
}

static BOOL walk_parameter_tree(struct d3dx_parameter *param, walk_parameter_dep_func param_func,
        void *data)
{
    unsigned int i;
    unsigned int member_count;

    if (param_func(data, param))
        return TRUE;

    member_count = param->element_count ? param->element_count : param->member_count;
    for (i = 0; i < member_count; ++i)
    {
        if (walk_parameter_tree(&param->members[i], param_func, data))
            return TRUE;
    }
    return FALSE;
}

static ULONG64 *get_version_counter_ptr(struct d3dx9_base_effect *base)
{
    return base->pool ? &base->pool->version_counter : &base->version_counter;
}

static ULONG64 next_effect_update_version(struct d3dx9_base_effect *base)
{
    return next_update_version(get_version_counter_ptr(base));
}

static void set_dirty(struct d3dx_parameter *param)
{
    struct d3dx_shared_data *shared_data;
    struct d3dx_top_level_parameter *top_param = param->top_level_param;
    ULONG64 new_update_version = next_update_version(top_param->version_counter);

    if ((shared_data = top_param->shared_data))
        shared_data->update_version = new_update_version;
    else
        top_param->update_version = new_update_version;
}

static HRESULT set_string(char **param_data, const char *string)
{
    HeapFree(GetProcessHeap(), 0, *param_data);
    *param_data = HeapAlloc(GetProcessHeap(), 0, strlen(string) + 1);
    if (!*param_data)
    {
        ERR("Out of memory.\n");
        return E_OUTOFMEMORY;
    }
    strcpy(*param_data, string);
    return D3D_OK;
}

static HRESULT d3dx9_base_effect_set_value(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, const void *data, UINT bytes)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);
    unsigned int i;

    if (!param)
    {
        WARN("Invalid parameter %p specified\n", parameter);
        return D3DERR_INVALIDCALL;
    }

    /* samplers don't touch data */
    if (param->class == D3DXPC_OBJECT && is_param_type_sampler(param->type))
    {
        TRACE("Sampler: returning E_FAIL\n");
        return E_FAIL;
    }

    if (data && param->bytes <= bytes)
    {
        switch (param->type)
        {
            case D3DXPT_TEXTURE:
            case D3DXPT_TEXTURE1D:
            case D3DXPT_TEXTURE2D:
            case D3DXPT_TEXTURE3D:
            case D3DXPT_TEXTURECUBE:
                for (i = 0; i < (param->element_count ? param->element_count : 1); ++i)
                {
                    IUnknown *old_texture = ((IUnknown **)param->data)[i];
                    IUnknown *new_texture = ((IUnknown **)data)[i];

                    if (new_texture == old_texture)
                        continue;

                    if (new_texture)
                        IUnknown_AddRef(new_texture);
                    if (old_texture)
                        IUnknown_Release(old_texture);
                }
            /* fallthrough */
            case D3DXPT_VOID:
            case D3DXPT_BOOL:
            case D3DXPT_INT:
            case D3DXPT_FLOAT:
                TRACE("Copy %u bytes.\n", param->bytes);
                memcpy(param->data, data, param->bytes);
                set_dirty(param);
                break;

            case D3DXPT_STRING:
            {
                HRESULT hr;

                set_dirty(param);
                for (i = 0; i < (param->element_count ? param->element_count : 1); ++i)
                {
                    if (FAILED(hr = set_string(&((char **)param->data)[i], ((const char **)data)[i])))
                        return hr;
                }
                break;
            }

            default:
                FIXME("Unhandled type %s.\n", debug_d3dxparameter_type(param->type));
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
    if (param->class == D3DXPC_OBJECT && is_param_type_sampler(param->type))
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

    WARN("Parameter not found.\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_set_bool(struct d3dx9_base_effect *base, D3DXHANDLE parameter, BOOL b)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (param && !param->element_count && param->rows == 1 && param->columns == 1)
    {
        set_number(param->data, param->type, &b, D3DXPT_BOOL);
        set_dirty(param);
        return D3D_OK;
    }

    WARN("Parameter not found.\n");

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

    WARN("Parameter not found.\n");

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
                set_dirty(param);
                return D3D_OK;

            case D3DXPC_OBJECT:
            case D3DXPC_STRUCT:
                break;

            default:
                FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
                break;
        }
    }

    WARN("Parameter not found.\n");

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

    WARN("Parameter not found.\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_set_int(struct d3dx9_base_effect *base, D3DXHANDLE parameter, INT n)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (param && !param->element_count)
    {
        if (param->rows == 1 && param->columns == 1)
        {
            DWORD value;

            set_number(&value, param->type, &n, D3DXPT_INT);
            if (value != *(DWORD *)param->data)
                set_dirty(param);
             *(DWORD *)param->data = value;
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
            set_dirty(param);
            return D3D_OK;
        }
    }

    WARN("Parameter not found.\n");

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

    WARN("Parameter not found.\n");

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
                set_dirty(param);
                return D3D_OK;

            case D3DXPC_OBJECT:
            case D3DXPC_STRUCT:
                break;

            default:
                FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
                break;
        }
    }

    WARN("Parameter not found.\n");

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

    WARN("Parameter not found.\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_set_float(struct d3dx9_base_effect *base, D3DXHANDLE parameter, float f)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (param && !param->element_count && param->rows == 1 && param->columns == 1)
    {
        DWORD value;

        set_number(&value, param->type, &f, D3DXPT_FLOAT);
        if (value != *(DWORD *)param->data)
            set_dirty(param);
         *(DWORD *)param->data = value;
        return D3D_OK;
    }

    WARN("Parameter not found.\n");

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

    WARN("Parameter not found.\n");

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
                set_dirty(param);
                return D3D_OK;

            case D3DXPC_OBJECT:
            case D3DXPC_STRUCT:
                break;

            default:
                FIXME("Unhandled class %s\n", debug_d3dxparameter_class(param->class));
                break;
        }
    }

    WARN("Parameter not found.\n");

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

    WARN("Parameter not found.\n");

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
                set_dirty(param);
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
                if (param->type == D3DXPT_FLOAT)
                {
                    memcpy(param->data, vector, param->columns * sizeof(float));
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

    WARN("Parameter not found.\n");

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

    WARN("Parameter not found.\n");

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
                set_dirty(param);
                if (param->type == D3DXPT_FLOAT)
                {
                    if (param->columns == 4)
                        memcpy(param->data, vector, count * 4 * sizeof(float));
                    else
                        for (i = 0; i < count; ++i)
                            memcpy((float *)param->data + param->columns * i, vector + i,
                                    param->columns * sizeof(float));
                    return D3D_OK;
                }

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

    WARN("Parameter not found.\n");

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

    WARN("Parameter not found.\n");

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
                set_matrix(param, matrix);
                set_dirty(param);
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

    WARN("Parameter not found.\n");

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

    WARN("Parameter not found.\n");

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
                set_dirty(param);
                for (i = 0; i < count; ++i)
                {
                    set_matrix(&param->members[i], &matrix[i]);
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

    WARN("Parameter not found.\n");

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

    WARN("Parameter not found.\n");

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
                set_dirty(param);
                for (i = 0; i < count; ++i)
                {
                    set_matrix(&param->members[i], matrix[i]);
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

    WARN("Parameter not found.\n");

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

    WARN("Parameter not found.\n");

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
                set_dirty(param);
                set_matrix_transpose(param, matrix);
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

    WARN("Parameter not found.\n");

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

    WARN("Parameter not found.\n");

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
                set_dirty(param);
                for (i = 0; i < count; ++i)
                {
                    set_matrix_transpose(&param->members[i], &matrix[i]);
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

    WARN("Parameter not found.\n");

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

    WARN("Parameter not found.\n");

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
                set_dirty(param);
                for (i = 0; i < count; ++i)
                {
                    set_matrix_transpose(&param->members[i], matrix[i]);
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

    WARN("Parameter not found.\n");

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

    WARN("Parameter not found.\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_set_string(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, const char *string)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (param && param->type == D3DXPT_STRING)
    {
        set_dirty(param);
        return set_string(param->data, string);
    }

    WARN("Parameter not found.\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_get_string(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, const char **string)
{
    struct d3dx_parameter *param = get_valid_parameter(base, parameter);

    if (string && param && !param->element_count && param->type == D3DXPT_STRING)
    {
        *string = *(const char **)param->data;
        TRACE("Returning %s.\n", debugstr_a(*string));
        return D3D_OK;
    }

    WARN("Parameter not found.\n");

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

        if (texture == oltexture)
            return D3D_OK;

        if (texture) IDirect3DBaseTexture9_AddRef(texture);
        if (oltexture) IDirect3DBaseTexture9_Release(oltexture);

        *(struct IDirect3DBaseTexture9 **)param->data = texture;
        set_dirty(param);

        return D3D_OK;
    }

    WARN("Parameter not found.\n");

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

    WARN("Parameter not found.\n");

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

    WARN("Parameter not found.\n");

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

    WARN("Parameter not found.\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT d3dx9_base_effect_set_array_range(struct d3dx9_base_effect *base,
        D3DXHANDLE parameter, UINT start, UINT end)
{
    FIXME("stub!\n");

    return E_NOTIMPL;
}

static void d3dx9_set_light_parameter(enum LIGHT_TYPE op, D3DLIGHT9 *light, void *value)
{
    static const struct
    {
        unsigned int offset;
        const char *name;
    }
    light_tbl[] =
    {
        {FIELD_OFFSET(D3DLIGHT9, Type),         "LC_TYPE"},
        {FIELD_OFFSET(D3DLIGHT9, Diffuse),      "LT_DIFFUSE"},
        {FIELD_OFFSET(D3DLIGHT9, Specular),     "LT_SPECULAR"},
        {FIELD_OFFSET(D3DLIGHT9, Ambient),      "LT_AMBIENT"},
        {FIELD_OFFSET(D3DLIGHT9, Position),     "LT_POSITION"},
        {FIELD_OFFSET(D3DLIGHT9, Direction),    "LT_DIRECTION"},
        {FIELD_OFFSET(D3DLIGHT9, Range),        "LT_RANGE"},
        {FIELD_OFFSET(D3DLIGHT9, Falloff),      "LT_FALLOFF"},
        {FIELD_OFFSET(D3DLIGHT9, Attenuation0), "LT_ATTENUATION0"},
        {FIELD_OFFSET(D3DLIGHT9, Attenuation1), "LT_ATTENUATION1"},
        {FIELD_OFFSET(D3DLIGHT9, Attenuation2), "LT_ATTENUATION2"},
        {FIELD_OFFSET(D3DLIGHT9, Theta),        "LT_THETA"},
        {FIELD_OFFSET(D3DLIGHT9, Phi),          "LT_PHI"}
    };
    switch (op)
    {
        case LT_TYPE:
            TRACE("LT_TYPE %u.\n", *(D3DLIGHTTYPE *)value);
            light->Type = *(D3DLIGHTTYPE *)value;
            break;
        case LT_DIFFUSE:
        case LT_SPECULAR:
        case LT_AMBIENT:
        {
            D3DCOLORVALUE c = *(D3DCOLORVALUE *)value;

            TRACE("%s (%.8e %.8e %.8e %.8e).\n", light_tbl[op].name, c.r, c.g, c.b, c.a);
            *(D3DCOLORVALUE *)((BYTE *)light + light_tbl[op].offset) = c;
            break;
        }
        case LT_POSITION:
        case LT_DIRECTION:
        {
            D3DVECTOR v = *(D3DVECTOR *)value;

            TRACE("%s (%.8e %.8e %.8e).\n", light_tbl[op].name, v.x, v.y, v.z);
            *(D3DVECTOR *)((BYTE *)light + light_tbl[op].offset) = v;
            break;
        }
        case LT_RANGE:
        case LT_FALLOFF:
        case LT_ATTENUATION0:
        case LT_ATTENUATION1:
        case LT_ATTENUATION2:
        case LT_THETA:
        case LT_PHI:
        {
            float v = *(float *)value;
            TRACE("%s %.8e.\n", light_tbl[op].name, v);
            *(float *)((BYTE *)light + light_tbl[op].offset) = v;
            break;
        }
        default:
            WARN("Unknown light parameter %u.\n", op);
            break;
    }
}

static void d3dx9_set_material_parameter(enum MATERIAL_TYPE op, D3DMATERIAL9 *material, void *value)
{
    static const struct
    {
        unsigned int offset;
        const char *name;
    }
    material_tbl[] =
    {
        {FIELD_OFFSET(D3DMATERIAL9, Diffuse),  "MT_DIFFUSE"},
        {FIELD_OFFSET(D3DMATERIAL9, Ambient),  "MT_AMBIENT"},
        {FIELD_OFFSET(D3DMATERIAL9, Specular), "MT_SPECULAR"},
        {FIELD_OFFSET(D3DMATERIAL9, Emissive), "MT_EMISSIVE"},
        {FIELD_OFFSET(D3DMATERIAL9, Power),    "MT_POWER"}
    };

    switch (op)
    {
        case MT_POWER:
        {
            float v = *(float *)value;

            TRACE("%s %.8e.\n", material_tbl[op].name, v);
            material->Power = v;
            break;
        }
        case MT_DIFFUSE:
        case MT_AMBIENT:
        case MT_SPECULAR:
        case MT_EMISSIVE:
        {
            D3DCOLORVALUE c = *(D3DCOLORVALUE *)value;

            TRACE("%s, value (%.8e %.8e %.8e %.8e).\n", material_tbl[op].name, c.r, c.g, c.b, c.a);
            *(D3DCOLORVALUE *)((BYTE *)material + material_tbl[op].offset) = c;
            break;
        }
        default:
            WARN("Unknown material parameter %u.\n", op);
            break;
    }
}

static HRESULT d3dx_set_shader_const_state(struct ID3DXEffectImpl *effect, enum SHADER_CONSTANT_TYPE op, UINT index,
        struct d3dx_parameter *param, void *value_ptr)
{
    static const struct
    {
        D3DXPARAMETER_TYPE type;
        UINT elem_size;
        const char *name;
    }
    const_tbl[] =
    {
        {D3DXPT_FLOAT, sizeof(float) * 4, "SCT_VSFLOAT"},
        {D3DXPT_BOOL,  sizeof(BOOL),      "SCT_VSBOOL"},
        {D3DXPT_INT,   sizeof(int) * 4,   "SCT_VSINT"},
        {D3DXPT_FLOAT, sizeof(float) * 4, "SCT_PSFLOAT"},
        {D3DXPT_BOOL,  sizeof(BOOL),      "SCT_PSBOOL"},
        {D3DXPT_INT,   sizeof(int) * 4,   "SCT_PSINT"},
    };
    unsigned int element_count;

    if (op < 0 || op > SCT_PSINT)
    {
        FIXME("Unknown op %u.\n", op);
        return D3DERR_INVALIDCALL;
    }
    element_count = param->bytes / const_tbl[op].elem_size;
    TRACE("%s, index %u, element_count %u.\n", const_tbl[op].name, index, element_count);
    if (param->type != const_tbl[op].type)
    {
        FIXME("Unexpected param type %u.\n", param->type);
        return D3DERR_INVALIDCALL;
    }
    if (param->bytes % const_tbl[op].elem_size != 0)
    {
        FIXME("Unexpected param size %u, rows %u, cols %u.\n", param->bytes, param->rows, param->columns);
        return D3DERR_INVALIDCALL;
    }

    switch (op)
    {
        case SCT_VSFLOAT:
            return SET_D3D_STATE(effect, SetVertexShaderConstantF, index, (const float *)value_ptr, element_count);
        case SCT_VSBOOL:
            return SET_D3D_STATE(effect, SetVertexShaderConstantB, index, (const BOOL *)value_ptr, element_count);
        case SCT_VSINT:
            return SET_D3D_STATE(effect, SetVertexShaderConstantI, index, (const int *)value_ptr, element_count);
        case SCT_PSFLOAT:
            return SET_D3D_STATE(effect, SetPixelShaderConstantF, index, (const float *)value_ptr, element_count);
        case SCT_PSBOOL:
            return SET_D3D_STATE(effect, SetPixelShaderConstantB, index, (const BOOL *)value_ptr, element_count);
        case SCT_PSINT:
            return SET_D3D_STATE(effect, SetPixelShaderConstantI, index, (const int *)value_ptr, element_count);
    }
    return D3D_OK;
}

static HRESULT d3dx9_apply_state(struct ID3DXEffectImpl *effect, struct d3dx_pass *pass,
        struct d3dx_state *state, unsigned int parent_index, BOOL update_all);

static HRESULT d3dx_set_shader_constants(struct ID3DXEffectImpl *effect, struct d3dx_pass *pass,
        struct d3dx_parameter *param, BOOL vs, BOOL update_all)
{
    HRESULT hr, ret;
    struct d3dx_parameter **params;
    D3DXCONSTANT_DESC *cdesc;
    unsigned int parameters_count;
    unsigned int i, j;

    if (!param->param_eval)
    {
        FIXME("param_eval structure is null.\n");
        return D3DERR_INVALIDCALL;
    }
    if (FAILED(hr = d3dx_param_eval_set_shader_constants(effect->manager, effect->device,
            param->param_eval, update_all)))
        return hr;
    params = param->param_eval->shader_inputs.inputs_param;
    cdesc = param->param_eval->shader_inputs.inputs;
    parameters_count = param->param_eval->shader_inputs.input_count;
    ret = D3D_OK;
    for (i = 0; i < parameters_count; ++i)
    {
        if (params[i] && params[i]->class == D3DXPC_OBJECT && is_param_type_sampler(params[i]->type))
        {
            struct d3dx_sampler *sampler;
            unsigned int sampler_idx;

            for (sampler_idx = 0; sampler_idx < cdesc[i].RegisterCount; ++sampler_idx)
            {
                sampler = params[i]->element_count ? params[i]->members[sampler_idx].data : params[i]->data;
                TRACE("sampler %s, register index %u, state count %u.\n", debugstr_a(params[i]->name),
                        cdesc[i].RegisterIndex, sampler->state_count);
                for (j = 0; j < sampler->state_count; ++j)
                {
                    if (FAILED(hr = d3dx9_apply_state(effect, pass, &sampler->states[j],
                            cdesc[i].RegisterIndex + sampler_idx + (vs ? D3DVERTEXTEXTURESAMPLER0 : 0),
                            update_all)))
                        ret = hr;
                }
            }
        }
    }
    return ret;
}

static HRESULT d3dx9_apply_state(struct ID3DXEffectImpl *effect, struct d3dx_pass *pass,
        struct d3dx_state *state, unsigned int parent_index, BOOL update_all)
{
    struct d3dx_parameter *param;
    void *param_value;
    BOOL param_dirty;
    HRESULT hr;

    TRACE("operation %u, index %u, type %u.\n", state->operation, state->index, state->type);

    if (FAILED(hr = d3dx9_get_param_value_ptr(pass, state, &param_value, &param,
            update_all, &param_dirty)))
    {
        if (!update_all && hr == E_FAIL)
        {
            /* Native d3dx9 returns D3D_OK from CommitChanges() involving
             * out of bounds array access and does not touch the affected
             * states. */
            WARN("Returning D3D_OK on out of bounds array access.\n");
            return D3D_OK;
        }
        return hr;
    }

    if (!(update_all || param_dirty
            || state_table[state->operation].class == SC_VERTEXSHADER
            || state_table[state->operation].class == SC_PIXELSHADER
            || state_table[state->operation].class == SC_SETSAMPLER))
        return D3D_OK;

    switch (state_table[state->operation].class)
    {
        case SC_RENDERSTATE:
            TRACE("%s, operation %u, value %u.\n", state_table[state->operation].name,
                    state_table[state->operation].op, *(DWORD *)param_value);
            return SET_D3D_STATE(effect, SetRenderState, state_table[state->operation].op, *(DWORD *)param_value);
        case SC_FVF:
            TRACE("%s, value %#x.\n", state_table[state->operation].name, *(DWORD *)param_value);
            return SET_D3D_STATE(effect, SetFVF, *(DWORD *)param_value);
        case SC_TEXTURE:
        {
            UINT unit;

            unit = parent_index == ~0u ? state->index : parent_index;
            TRACE("%s, unit %u, value %p.\n", state_table[state->operation].name, unit,
                    *(IDirect3DBaseTexture9 **)param_value);
            return SET_D3D_STATE(effect, SetTexture, unit, *(IDirect3DBaseTexture9 **)param_value);
        }
        case SC_TEXTURESTAGE:
            TRACE("%s, stage %u, value %u.\n", state_table[state->operation].name, state->index, *(DWORD *)param_value);
            return SET_D3D_STATE(effect, SetTextureStageState, state->index,
                        state_table[state->operation].op, *(DWORD *)param_value);
        case SC_SETSAMPLER:
        {
            struct d3dx_sampler *sampler;
            HRESULT ret, hr;
            unsigned int i;

            sampler = (struct d3dx_sampler *)param_value;
            TRACE("%s, sampler %u, applying %u states.\n", state_table[state->operation].name, state->index,
                    sampler->state_count);
            ret = D3D_OK;
            for (i = 0; i < sampler->state_count; i++)
            {
                if (FAILED(hr = d3dx9_apply_state(effect, pass, &sampler->states[i], state->index, update_all)))
                    ret = hr;
            }
            return ret;
        }
        case SC_SAMPLERSTATE:
        {
            UINT sampler;

            sampler = parent_index == ~0u ? state->index : parent_index;
            TRACE("%s, sampler %u, value %u.\n", state_table[state->operation].name, sampler, *(DWORD *)param_value);
            return SET_D3D_STATE(effect, SetSamplerState, sampler, state_table[state->operation].op,
                    *(DWORD *)param_value);
        }
        case SC_VERTEXSHADER:
            TRACE("%s, shader %p.\n", state_table[state->operation].name, *(IDirect3DVertexShader9 **)param_value);
            if ((update_all || param_dirty)
                    && FAILED(hr = SET_D3D_STATE(effect, SetVertexShader,
                    *(IDirect3DVertexShader9 **)param_value)))
                ERR("Could not set vertex shader, hr %#x.\n", hr);
            else if (*(IDirect3DVertexShader9 **)param_value)
                hr = d3dx_set_shader_constants(effect, pass, param, TRUE, update_all || param_dirty);
            return hr;
        case SC_PIXELSHADER:
            TRACE("%s, shader %p.\n", state_table[state->operation].name, *(IDirect3DPixelShader9 **)param_value);
            if ((update_all || param_dirty)
                    && FAILED(hr = SET_D3D_STATE(effect, SetPixelShader,
                    *(IDirect3DPixelShader9 **)param_value)))
                ERR("Could not set pixel shader, hr %#x.\n", hr);
            else if (*(IDirect3DPixelShader9 **)param_value)
                hr = d3dx_set_shader_constants(effect, pass, param, FALSE, update_all || param_dirty);
            return hr;
        case SC_TRANSFORM:
            TRACE("%s, state %u.\n", state_table[state->operation].name, state->index);
            return SET_D3D_STATE(effect, SetTransform, state_table[state->operation].op + state->index,
                    (D3DMATRIX *)param_value);
        case SC_LIGHTENABLE:
            TRACE("%s, index %u, value %u.\n", state_table[state->operation].name, state->index, *(BOOL *)param_value);
            return SET_D3D_STATE(effect, LightEnable, state->index, *(BOOL *)param_value);
        case SC_LIGHT:
        {
            TRACE("%s, index %u, op %u.\n", state_table[state->operation].name, state->index,
                    state_table[state->operation].op);
            d3dx9_set_light_parameter(state_table[state->operation].op,
                    &effect->current_light[state->index], param_value);
            effect->light_updated |= 1u << state->index;
            return D3D_OK;
        }
        case SC_MATERIAL:
        {
            TRACE("%s, index %u, op %u.\n", state_table[state->operation].name, state->index,
                    state_table[state->operation].op);
            d3dx9_set_material_parameter(state_table[state->operation].op,
                    &effect->current_material, param_value);
            effect->material_updated = TRUE;
            return D3D_OK;
        }
        case SC_NPATCHMODE:
            TRACE("%s, nsegments %f.\n", state_table[state->operation].name, *(float *)param_value);
            return SET_D3D_STATE(effect, SetNPatchMode, *(float *)param_value);
        case SC_SHADERCONST:
            TRACE("%s, index %u, op %u.\n", state_table[state->operation].name, state->index,
                state_table[state->operation].op);
            return d3dx_set_shader_const_state(effect, state_table[state->operation].op, state->index,
                param, param_value);
        default:
            FIXME("%s not handled.\n", state_table[state->operation].name);
            break;
    }
    return D3D_OK;
}

static HRESULT d3dx9_apply_pass_states(struct ID3DXEffectImpl *effect, struct d3dx_pass *pass, BOOL update_all)
{
    unsigned int i;
    HRESULT ret;
    HRESULT hr;
    ULONG64 new_update_version = next_effect_update_version(&effect->base_effect);

    TRACE("effect %p, pass %p, state_count %u.\n", effect, pass, pass->state_count);

    ret = D3D_OK;
    for (i = 0; i < pass->state_count; ++i)
    {
        if (FAILED(hr = d3dx9_apply_state(effect, pass, &pass->states[i], ~0u, update_all)))
        {
            WARN("Error applying state, hr %#x.\n", hr);
            ret = hr;
        }
    }

    if (effect->light_updated)
    {
        for (i = 0; i < ARRAY_SIZE(effect->current_light); ++i)
        {
            if ((effect->light_updated & (1u << i))
                    && FAILED(hr = SET_D3D_STATE(effect, SetLight, i, &effect->current_light[i])))
            {
                WARN("Error setting light, hr %#x.\n", hr);
                ret = hr;
            }
        }
        effect->light_updated = 0;
    }

    if (effect->material_updated
            && FAILED(hr = SET_D3D_STATE(effect, SetMaterial, &effect->current_material)))
    {
        WARN("Error setting material, hr %#x.\n", hr);
        ret = hr;
    }
    effect->material_updated = FALSE;

    pass->update_version = new_update_version;
    return ret;
}

static void param_set_data_pointer(struct d3dx_parameter *param, unsigned char *data, BOOL child, BOOL free_data)
{
    unsigned char *member_data = data;
    unsigned int i, count;

    count = param->element_count ? param->element_count : param->member_count;
    for (i = 0; i < count; ++i)
    {
        param_set_data_pointer(&param->members[i], member_data, TRUE, free_data);
        if (data)
            member_data += param->members[i].bytes;
    }
    if (free_data)
        free_parameter_data(param, child);
    param->data = data;
}

static BOOL is_same_parameter(void *param1_, struct d3dx_parameter *param2)
{
    struct d3dx_parameter *param1 = (struct d3dx_parameter *)param1_;
    BOOL matches;
    unsigned int i, member_count;

    matches = !strcmp(param1->name, param2->name) && param1->class == param2->class
            && param1->type == param2->type && param1->rows == param2->rows
            && param1->columns == param2->columns && param1->element_count == param2->element_count
            && param1->member_count == param2->member_count;

    member_count = param1->element_count ? param1->element_count : param1->member_count;

    if (!matches || !member_count)
        return matches;

    for (i = 0; i < member_count; ++i)
    {
        if (!is_same_parameter(&param1->members[i], &param2->members[i]))
            return FALSE;
    }
    return TRUE;
}

static HRESULT d3dx_pool_sync_shared_parameter(struct d3dx_effect_pool *pool, struct d3dx_top_level_parameter *param)
{
    unsigned int i, free_entry_index;
    unsigned int new_size, new_count;

    if (!(param->param.flags & PARAMETER_FLAG_SHARED) || !pool || is_param_type_sampler(param->param.type))
        return D3D_OK;

    free_entry_index = pool->size;
    for (i = 0; i < pool->size; ++i)
    {
        if (!pool->shared_data[i].count)
            free_entry_index = i;
        else if (is_same_parameter(&param->param, &pool->shared_data[i].parameters[0]->param))
            break;
    }
    if (i == pool->size)
    {
        i = free_entry_index;
        if (i == pool->size)
        {
            struct d3dx_shared_data *new_alloc;

            if (!pool->size)
            {
                new_size = INITIAL_POOL_SIZE;
                new_alloc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                        sizeof(*pool->shared_data) * new_size);
                if (!new_alloc)
                {
                    ERR("Out of memory.\n");
                    return E_OUTOFMEMORY;
                }
            }
            else
            {
                new_size = pool->size * 2;
                new_alloc = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, pool->shared_data,
                        sizeof(*pool->shared_data) * new_size);
                if (!new_alloc)
                {
                    ERR("Out of memory.\n");
                    return E_OUTOFMEMORY;
                }
                if (new_alloc != pool->shared_data)
                {
                    unsigned int j, k;

                    for (j = 0; j < pool->size; ++j)
                        for (k = 0; k < new_alloc[j].count; ++k)
                            new_alloc[j].parameters[k]->shared_data = &new_alloc[j];
                }
            }
            pool->shared_data = new_alloc;
            pool->size = new_size;
        }
        pool->shared_data[i].data = param->param.data;
    }
    else
    {
        param_set_data_pointer(&param->param, pool->shared_data[i].data, FALSE, TRUE);
    }
    new_count = ++pool->shared_data[i].count;
    if (new_count >= pool->shared_data[i].size)
    {
        if (!pool->shared_data[i].size)
        {
            new_size = INITIAL_SHARED_DATA_SIZE;
            pool->shared_data[i].parameters = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                    sizeof(*pool->shared_data[i].parameters) * INITIAL_SHARED_DATA_SIZE);
        }
        else
        {
            new_size = pool->shared_data[i].size * 2;
            pool->shared_data[i].parameters = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                    pool->shared_data[i].parameters,
                    sizeof(*pool->shared_data[i].parameters) * new_size);
        }
        pool->shared_data[i].size = new_size;
    }

    param->shared_data = &pool->shared_data[i];
    pool->shared_data[i].parameters[new_count - 1] = param;

    TRACE("name %s, parameter idx %u, new refcount %u.\n", debugstr_a(param->param.name), i,
            new_count);

    return D3D_OK;
}

static BOOL param_zero_data_func(void *dummy, struct d3dx_parameter *param)
{
    param->data = NULL;
    return FALSE;
}

static void d3dx_pool_release_shared_parameter(struct d3dx_top_level_parameter *param)
{
    unsigned int new_count;

    if (!(param->param.flags & PARAMETER_FLAG_SHARED) || !param->shared_data)
        return;
    new_count = --param->shared_data->count;

    TRACE("param %p, param->shared_data %p, new_count %d.\n", param, param->shared_data, new_count);

    if (new_count)
    {
        unsigned int i;

        for (i = 0; i < new_count; ++i)
        {
            if (param->shared_data->parameters[i] == param)
            {
                memmove(&param->shared_data->parameters[i],
                        &param->shared_data->parameters[i + 1],
                        sizeof(param->shared_data->parameters[i]) * (new_count - i));
                break;
            }
        }
        walk_parameter_tree(&param->param, param_zero_data_func, NULL);
    }
    else
    {
        HeapFree(GetProcessHeap(), 0, param->shared_data->parameters);
        /* Zeroing table size is required as the entry in pool parameters table can be reused. */
        param->shared_data->size = 0;
        param->shared_data = NULL;
    }
}

static inline struct d3dx_effect_pool *impl_from_ID3DXEffectPool(ID3DXEffectPool *iface)
{
    return CONTAINING_RECORD(iface, struct d3dx_effect_pool, ID3DXEffectPool_iface);
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

    WARN("Technique not found.\n");

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
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);
    struct d3dx9_base_effect *base = &effect->base_effect;
    struct d3dx_technique *tech = get_valid_technique(base, technique);
    HRESULT ret = D3D_OK;
    unsigned int i, j;

    FIXME("iface %p, technique %p semi-stub.\n", iface, technique);

    if (!tech)
    {
        ret = D3DERR_INVALIDCALL;
        goto done;
    }
    for (i = 0; i < tech->pass_count; ++i)
    {
        struct d3dx_pass *pass = &tech->passes[i];

        for (j = 0; j < pass->state_count; ++j)
        {
            struct d3dx_state *state = &pass->states[j];

            if (state_table[state->operation].class == SC_VERTEXSHADER
                    || state_table[state->operation].class == SC_PIXELSHADER)
            {
                struct d3dx_parameter *param;
                void *param_value;
                BOOL param_dirty;
                HRESULT hr;

                if (FAILED(hr = d3dx9_get_param_value_ptr(pass, &pass->states[j], &param_value, &param,
                        FALSE, &param_dirty)))
                    return hr;

                if (param->object_id && base->objects[param->object_id].creation_failed)
                {
                    ret = E_FAIL;
                    goto done;
                }
            }
        }
    }
done:
    TRACE("Returning %#x.\n", ret);
    return ret;
}

static HRESULT WINAPI ID3DXEffectImpl_FindNextValidTechnique(ID3DXEffect *iface,
        D3DXHANDLE technique, D3DXHANDLE *next_technique)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);
    struct d3dx9_base_effect *base = &effect->base_effect;
    struct d3dx_technique *prev_tech, *tech;
    unsigned int i;

    TRACE("iface %p, technique %p, next_technique %p.\n", iface, technique, next_technique);

    if (technique)
    {
        if (!(prev_tech = get_valid_technique(base, technique)))
            return D3DERR_INVALIDCALL;

        for (i = 0; i < base->technique_count; ++i)
        {
            tech = &base->techniques[i];
            if (tech == prev_tech)
            {
                ++i;
                break;
            }
        }
    }
    else
    {
        i = 0;
    }

    for (; i < base->technique_count; ++i)
    {
        tech = &base->techniques[i];
        if (SUCCEEDED(ID3DXEffectImpl_ValidateTechnique(iface, get_technique_handle(tech))))
        {
            *next_technique = get_technique_handle(tech);
            return D3D_OK;
        }
    }

    *next_technique = get_technique_handle(&base->techniques[0]);
    return S_FALSE;
}

static BOOL walk_parameter_dep(struct d3dx_parameter *param, walk_parameter_dep_func param_func,
        void *data);

static BOOL walk_param_eval_dep(struct d3dx_param_eval *param_eval, walk_parameter_dep_func param_func,
        void *data)
{
    struct d3dx_parameter **params;
    unsigned int i, param_count;

    if (!param_eval)
        return FALSE;

    params = param_eval->shader_inputs.inputs_param;
    param_count = param_eval->shader_inputs.input_count;
    for (i = 0; i < param_count; ++i)
    {
        if (walk_parameter_dep(params[i], param_func, data))
            return TRUE;
    }

    params = param_eval->pres.inputs.inputs_param;
    param_count = param_eval->pres.inputs.input_count;
    for (i = 0; i < param_count; ++i)
    {
        if (walk_parameter_dep(params[i], param_func, data))
            return TRUE;
    }
    return FALSE;
}

static BOOL walk_state_dep(struct d3dx_state *state, walk_parameter_dep_func param_func,
        void *data)
{
    if (state->type == ST_CONSTANT && is_param_type_sampler(state->parameter.type))
    {
        if (walk_parameter_dep(&state->parameter, param_func, data))
            return TRUE;
    }
    else if (state->type == ST_ARRAY_SELECTOR || state->type == ST_PARAMETER)
    {
        if (walk_parameter_dep(state->referenced_param, param_func, data))
            return TRUE;
    }
    return walk_param_eval_dep(state->parameter.param_eval, param_func, data);
}

static BOOL walk_parameter_dep(struct d3dx_parameter *param, walk_parameter_dep_func param_func,
        void *data)
{
    unsigned int i;
    unsigned int member_count;

    param = &param->top_level_param->param;
    if (param_func(data, param))
        return TRUE;

    if (walk_param_eval_dep(param->param_eval, param_func, data))
        return TRUE;

    if (param->class == D3DXPC_OBJECT && is_param_type_sampler(param->type))
    {
        struct d3dx_sampler *sampler;
        unsigned int sampler_idx;
        unsigned int samplers_count = max(param->element_count, 1);

        for (sampler_idx = 0; sampler_idx < samplers_count; ++sampler_idx)
        {
            sampler = param->element_count ? param->members[sampler_idx].data : param->data;
            for (i = 0; i < sampler->state_count; ++i)
            {
                if (walk_state_dep(&sampler->states[i], param_func, data))
                    return TRUE;
            }
        }
        return FALSE;
    }

    member_count = param->element_count ? param->element_count : param->member_count;
    for (i = 0; i < member_count; ++i)
    {
        if (walk_param_eval_dep(param->members[i].param_eval, param_func, data))
            return TRUE;
    }

    return FALSE;
}

static BOOL is_parameter_used(struct d3dx_parameter *param, struct d3dx_technique *tech)
{
    unsigned int i, j;
    struct d3dx_pass *pass;

    if (!tech || !param)
        return FALSE;

    for (i = 0; i < tech->pass_count; ++i)
    {
        pass = &tech->passes[i];
        for (j = 0; j < pass->state_count; ++j)
        {
            if (walk_state_dep(&pass->states[j], is_same_parameter, param))
                return TRUE;
        }
    }
    return FALSE;
}

static BOOL WINAPI ID3DXEffectImpl_IsParameterUsed(ID3DXEffect* iface, D3DXHANDLE parameter, D3DXHANDLE technique)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);
    struct d3dx_parameter *param = get_valid_parameter(&effect->base_effect, parameter);
    struct d3dx_technique *tech = get_valid_technique(&effect->base_effect, technique);
    BOOL ret;

    TRACE("iface %p, parameter %p, technique %p.\n", iface, parameter, technique);
    TRACE("param %p, name %s, tech %p.\n", param, param ? debugstr_a(param->name) : "", tech);

    ret = is_parameter_used(param, tech);
    TRACE("Returning %#x.\n", ret);
    return ret;
}

static HRESULT WINAPI ID3DXEffectImpl_Begin(ID3DXEffect *iface, UINT *passes, DWORD flags)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);
    struct d3dx_technique *technique = effect->active_technique;

    TRACE("iface %p, passes %p, flags %#x.\n", iface, passes, flags);

    if (technique)
    {
        if (flags & ~(D3DXFX_DONOTSAVESTATE | D3DXFX_DONOTSAVESAMPLERSTATE | D3DXFX_DONOTSAVESHADERSTATE))
            WARN("Invalid flags (%#x) specified.\n", flags);

        if (flags & D3DXFX_DONOTSAVESTATE)
        {
            TRACE("State capturing disabled.\n");
        }
        else
        {
            HRESULT hr;
            unsigned int i;

            if (!technique->saved_state)
            {
                ID3DXEffectStateManager *manager;

                manager = effect->manager;
                effect->manager = NULL;
                if (FAILED(hr = IDirect3DDevice9_BeginStateBlock(effect->device)))
                    ERR("BeginStateBlock failed, hr %#x.\n", hr);
                for (i = 0; i < technique->pass_count; i++)
                    d3dx9_apply_pass_states(effect, &technique->passes[i], TRUE);
                if (FAILED(hr = IDirect3DDevice9_EndStateBlock(effect->device, &technique->saved_state)))
                    ERR("EndStateBlock failed, hr %#x.\n", hr);
                effect->manager = manager;
            }
            if (FAILED(hr = IDirect3DStateBlock9_Capture(technique->saved_state)))
                ERR("StateBlock Capture failed, hr %#x.\n", hr);
        }

        if (passes)
            *passes = technique->pass_count;
        effect->started = TRUE;
        effect->begin_flags = flags;

        return D3D_OK;
    }

    WARN("Invalid argument supplied.\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT WINAPI ID3DXEffectImpl_BeginPass(ID3DXEffect *iface, UINT pass)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);
    struct d3dx_technique *technique = effect->active_technique;

    TRACE("iface %p, pass %u\n", effect, pass);

    if (technique && pass < technique->pass_count && !effect->active_pass)
    {
        HRESULT hr;

        memset(effect->current_light, 0, sizeof(effect->current_light));
        memset(&effect->current_material, 0, sizeof(effect->current_material));

        if (SUCCEEDED(hr = d3dx9_apply_pass_states(effect, &technique->passes[pass], TRUE)))
            effect->active_pass = &technique->passes[pass];
        return hr;
    }

    WARN("Invalid argument supplied.\n");

    return D3DERR_INVALIDCALL;
}

static HRESULT WINAPI ID3DXEffectImpl_CommitChanges(ID3DXEffect *iface)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);

    TRACE("iface %p.\n", iface);

    if (!effect->active_pass)
    {
        WARN("Called without an active pass.\n");
        return D3D_OK;
    }
    return d3dx9_apply_pass_states(effect, effect->active_pass, FALSE);
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
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);
    struct d3dx_technique *technique = effect->active_technique;

    TRACE("iface %p.\n", iface);

    if (!effect->started)
        return D3D_OK;

    if (effect->begin_flags & D3DXFX_DONOTSAVESTATE)
    {
        TRACE("State restoring disabled.\n");
    }
    else
    {
        HRESULT hr;

        if (technique && technique->saved_state)
        {
            if (FAILED(hr = IDirect3DStateBlock9_Apply(technique->saved_state)))
                ERR("State block apply failed, hr %#x.\n", hr);
        }
        else
            ERR("No saved state.\n");
    }

    effect->started = FALSE;

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

static BOOL param_on_lost_device(void *data, struct d3dx_parameter *param)
{
    struct IDirect3DVolumeTexture9 *volume_texture;
    struct IDirect3DCubeTexture9 *cube_texture;
    struct IDirect3DTexture9 *texture;
    D3DSURFACE_DESC surface_desc;
    D3DVOLUME_DESC volume_desc;

    if (param->class == D3DXPC_OBJECT && !param->element_count)
    {
        switch (param->type)
        {
            case D3DXPT_TEXTURE:
            case D3DXPT_TEXTURE1D:
            case D3DXPT_TEXTURE2D:
                texture = *(IDirect3DTexture9 **)param->data;
                if (!texture)
                    return FALSE;
                IDirect3DTexture9_GetLevelDesc(texture, 0, &surface_desc);
                if (surface_desc.Pool != D3DPOOL_DEFAULT)
                    return FALSE;
                break;
            case D3DXPT_TEXTURE3D:
                volume_texture = *(IDirect3DVolumeTexture9 **)param->data;
                if (!volume_texture)
                    return FALSE;
                IDirect3DVolumeTexture9_GetLevelDesc(volume_texture, 0, &volume_desc);
                if (volume_desc.Pool != D3DPOOL_DEFAULT)
                    return FALSE;
                break;
            case D3DXPT_TEXTURECUBE:
                cube_texture = *(IDirect3DCubeTexture9 **)param->data;
                if (!cube_texture)
                    return FALSE;
                IDirect3DTexture9_GetLevelDesc(cube_texture, 0, &surface_desc);
                if (surface_desc.Pool != D3DPOOL_DEFAULT)
                    return FALSE;
                break;
            default:
                return FALSE;
        }
        IUnknown_Release(*(IUnknown **)param->data);
        *(IUnknown **)param->data = NULL;
    }
    return FALSE;
}

static HRESULT WINAPI ID3DXEffectImpl_OnLostDevice(ID3DXEffect* iface)
{
    struct ID3DXEffectImpl *effect = impl_from_ID3DXEffect(iface);
    struct d3dx9_base_effect *base = &effect->base_effect;
    unsigned int i;

    TRACE("iface %p.\n", iface);

    for (i = 0; i < base->parameter_count; ++i)
        walk_parameter_tree(&base->parameters[i].param, param_on_lost_device, NULL);

    return D3D_OK;
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

#if D3DX_SDK_VERSION >= 26
static HRESULT WINAPI ID3DXEffectImpl_DeleteParameterBlock(ID3DXEffect* iface, D3DXHANDLE parameter_block)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(%p): stub\n", This, parameter_block);

    return E_NOTIMPL;
}
#endif

static HRESULT WINAPI ID3DXEffectImpl_CloneEffect(ID3DXEffect *iface,
        struct IDirect3DDevice9 *device, struct ID3DXEffect **effect)
{
    struct ID3DXEffectImpl *This = impl_from_ID3DXEffect(iface);

    FIXME("(%p)->(%p, %p): stub\n", This, device, effect);

    if (!effect)
        return D3DERR_INVALIDCALL;

    if (This->base_effect.flags & D3DXFX_NOT_CLONEABLE)
        return E_FAIL;

    if (!device)
        return D3DERR_INVALIDCALL;

    iface->lpVtbl->AddRef(iface);
    *effect = iface;
    return S_OK;
}

#if D3DX_SDK_VERSION >= 27
static HRESULT WINAPI ID3DXEffectImpl_SetRawValue(ID3DXEffect *iface,
        D3DXHANDLE parameter, const void *data, UINT byte_offset, UINT bytes)
{
    FIXME("iface %p, parameter %p, data %p, byte_offset %u, bytes %u stub!\n",
            iface, parameter, data, byte_offset, bytes);

    return E_NOTIMPL;
}
#endif

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
#if D3DX_SDK_VERSION >= 26
    ID3DXEffectImpl_DeleteParameterBlock,
#endif
    ID3DXEffectImpl_CloneEffect,
#if D3DX_SDK_VERSION >= 27
    ID3DXEffectImpl_SetRawValue
#endif
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
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** ID3DXBaseEffect methods ***/
static HRESULT WINAPI ID3DXEffectCompilerImpl_GetDesc(ID3DXEffectCompiler *iface, D3DXEFFECT_DESC *desc)
{
    FIXME("iface %p, desc %p stub!\n", iface, desc);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetParameterDesc(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, D3DXPARAMETER_DESC *desc)
{
    FIXME("iface %p, parameter %p, desc %p stub!\n", iface, parameter, desc);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetTechniqueDesc(ID3DXEffectCompiler *iface,
        D3DXHANDLE technique, D3DXTECHNIQUE_DESC *desc)
{
    FIXME("iface %p, technique %p, desc %p stub!\n", iface, technique, desc);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetPassDesc(ID3DXEffectCompiler *iface,
        D3DXHANDLE pass, D3DXPASS_DESC *desc)
{
    FIXME("iface %p, pass %p, desc %p stub!\n", iface, pass, desc);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetFunctionDesc(ID3DXEffectCompiler *iface,
        D3DXHANDLE shader, D3DXFUNCTION_DESC *desc)
{
    FIXME("iface %p, shader %p, desc %p stub!\n", iface, shader, desc);

    return E_NOTIMPL;
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetParameter(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, UINT index)
{
    FIXME("iface %p, parameter %p, index %u stub!\n", iface, parameter, index);

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetParameterByName(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const char *name)
{
    FIXME("iface %p, parameter %p, name %s stub!\n", iface, parameter, debugstr_a(name));

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetParameterBySemantic(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const char *semantic)
{
    FIXME("iface %p, parameter %p, semantic %s stub!\n", iface, parameter, debugstr_a(semantic));

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetParameterElement(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, UINT index)
{
    FIXME("iface %p, parameter %p, index %u stub!\n", iface, parameter, index);

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetTechnique(ID3DXEffectCompiler *iface, UINT index)
{
    FIXME("iface %p, index %u stub!\n", iface, index);

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetTechniqueByName(ID3DXEffectCompiler *iface, const char *name)
{
    FIXME("iface %p, name %s stub!\n", iface, debugstr_a(name));

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetPass(ID3DXEffectCompiler *iface, D3DXHANDLE technique, UINT index)
{
    FIXME("iface %p, technique %p, index %u stub!\n", iface, technique, index);

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetPassByName(ID3DXEffectCompiler *iface,
        D3DXHANDLE technique, const char *name)
{
    FIXME("iface %p, technique %p, name %s stub!\n", iface, technique, debugstr_a(name));

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetFunction(ID3DXEffectCompiler *iface, UINT index)
{
    FIXME("iface %p, index %u stub!\n", iface, index);

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetFunctionByName(ID3DXEffectCompiler *iface, const char *name)
{
    FIXME("iface %p, name %s stub!\n", iface, debugstr_a(name));

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetAnnotation(ID3DXEffectCompiler *iface,
        D3DXHANDLE object, UINT index)
{
    FIXME("iface %p, object %p, index %u stub!\n", iface, object, index);

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXEffectCompilerImpl_GetAnnotationByName(ID3DXEffectCompiler *iface,
        D3DXHANDLE object, const char *name)
{
    FIXME("iface %p, object %p, name %s stub!\n", iface, object, debugstr_a(name));

    return NULL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetValue(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const void *data, UINT bytes)
{
    FIXME("iface %p, parameter %p, data %p, bytes %u stub!\n", iface, parameter, data, bytes);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetValue(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, void *data, UINT bytes)
{
    FIXME("iface %p, parameter %p, data %p, bytes %u stub!\n", iface, parameter, data, bytes);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetBool(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, BOOL b)
{
    FIXME("iface %p, parameter %p, b %#x stub!\n", iface, parameter, b);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetBool(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, BOOL *b)
{
    FIXME("iface %p, parameter %p, b %p stub!\n", iface, parameter, b);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetBoolArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const BOOL *b, UINT count)
{
    FIXME("iface %p, parameter %p, b %p, count %u stub!\n", iface, parameter, b, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetBoolArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, BOOL *b, UINT count)
{
    FIXME("iface %p, parameter %p, b %p, count %u stub!\n", iface, parameter, b, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetInt(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, INT n)
{
    FIXME("iface %p, parameter %p, n %d stub!\n", iface, parameter, n);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetInt(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, INT *n)
{
    FIXME("iface %p, parameter %p, n %p stub!\n", iface, parameter, n);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetIntArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const INT *n, UINT count)
{
    FIXME("iface %p, parameter %p, n %p, count %u stub!\n", iface, parameter, n, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetIntArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, INT *n, UINT count)
{
    FIXME("iface %p, parameter %p, n %p, count %u stub!\n", iface, parameter, n, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetFloat(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, float f)
{
    FIXME("iface %p, parameter %p, f %.8e stub!\n", iface, parameter, f);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetFloat(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, float *f)
{
    FIXME("iface %p, parameter %p, f %p stub!\n", iface, parameter, f);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetFloatArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const float *f, UINT count)
{
    FIXME("iface %p, parameter %p, f %p, count %u stub!\n", iface, parameter, f, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetFloatArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, float *f, UINT count)
{
    FIXME("iface %p, parameter %p, f %p, count %u stub!\n", iface, parameter, f, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetVector(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const D3DXVECTOR4 *vector)
{
    FIXME("iface %p, parameter %p, vector %p stub!\n", iface, parameter, vector);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetVector(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, D3DXVECTOR4 *vector)
{
    FIXME("iface %p, parameter %p, vector %p stub!\n", iface, parameter, vector);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetVectorArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const D3DXVECTOR4 *vector, UINT count)
{
    FIXME("iface %p, parameter %p, vector %p, count %u stub!\n", iface, parameter, vector, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetVectorArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, D3DXVECTOR4 *vector, UINT count)
{
    FIXME("iface %p, parameter %p, vector %p, count %u stub!\n", iface, parameter, vector, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetMatrix(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const D3DXMATRIX *matrix)
{
    FIXME("iface %p, parameter %p, matrix %p stub!\n", iface, parameter, matrix);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetMatrix(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, D3DXMATRIX *matrix)
{
    FIXME("iface %p, parameter %p, matrix %p stub!\n", iface, parameter, matrix);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetMatrixArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const D3DXMATRIX *matrix, UINT count)
{
    FIXME("iface %p, parameter %p, matrix %p, count %u stub!\n", iface, parameter, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetMatrixArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, D3DXMATRIX *matrix, UINT count)
{
    FIXME("iface %p, parameter %p, matrix %p, count %u stub!\n", iface, parameter, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetMatrixPointerArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const D3DXMATRIX **matrix, UINT count)
{
    FIXME("iface %p, parameter %p, matrix %p, count %u stub!\n", iface, parameter, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetMatrixPointerArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, D3DXMATRIX **matrix, UINT count)
{
    FIXME("iface %p, parameter %p, matrix %p, count %u stub!\n", iface, parameter, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetMatrixTranspose(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const D3DXMATRIX *matrix)
{
    FIXME("iface %p, parameter %p, matrix %p stub!\n", iface, parameter, matrix);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetMatrixTranspose(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, D3DXMATRIX *matrix)
{
    FIXME("iface %p, parameter %p, matrix %p stub!\n", iface, parameter, matrix);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetMatrixTransposeArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const D3DXMATRIX *matrix, UINT count)
{
    FIXME("iface %p, parameter %p, matrix %p, count %u stub!\n", iface, parameter, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetMatrixTransposeArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, D3DXMATRIX *matrix, UINT count)
{
    FIXME("iface %p, parameter %p, matrix %p, count %u stub!\n", iface, parameter, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetMatrixTransposePointerArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const D3DXMATRIX **matrix, UINT count)
{
    FIXME("iface %p, parameter %p, matrix %p, count %u stub!\n", iface, parameter, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetMatrixTransposePointerArray(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, D3DXMATRIX **matrix, UINT count)
{
    FIXME("iface %p, parameter %p, matrix %p, count %u stub!\n", iface, parameter, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetString(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const char *string)
{
    FIXME("iface %p, parameter %p, string %s stub!\n", iface, parameter, debugstr_a(string));

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetString(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, const char **string)
{
    FIXME("iface %p, parameter %p, string %p stub!\n", iface, parameter, string);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetTexture(struct ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, struct IDirect3DBaseTexture9 *texture)
{
    FIXME("iface %p, parameter %p, texture %p stub!\n", iface, parameter, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetTexture(struct ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, struct IDirect3DBaseTexture9 **texture)
{
    FIXME("iface %p, parameter %p, texture %p stub!\n", iface, parameter, texture);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetPixelShader(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, struct IDirect3DPixelShader9 **shader)
{
    FIXME("iface %p, parameter %p, shader %p stub!\n", iface, parameter, shader);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetVertexShader(struct ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, struct IDirect3DVertexShader9 **shader)
{
    FIXME("iface %p, parameter %p, shader %p stub!\n", iface, parameter, shader);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_SetArrayRange(ID3DXEffectCompiler *iface,
        D3DXHANDLE parameter, UINT start, UINT end)
{
    FIXME("iface %p, parameter %p, start %u, end %u stub!\n", iface, parameter, start, end);

    return E_NOTIMPL;
}

/*** ID3DXEffectCompiler methods ***/
static HRESULT WINAPI ID3DXEffectCompilerImpl_SetLiteral(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, BOOL literal)
{
    FIXME("iface %p, parameter %p, literal %#x stub!\n", iface, parameter, literal);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_GetLiteral(ID3DXEffectCompiler *iface, D3DXHANDLE parameter, BOOL *literal)
{
    FIXME("iface %p, parameter %p, literal %p stub!\n", iface, parameter, literal);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_CompileEffect(ID3DXEffectCompiler *iface, DWORD flags,
        ID3DXBuffer **effect, ID3DXBuffer **error_msgs)
{
    FIXME("iface %p, flags %#x, effect %p, error_msgs %p stub!\n", iface, flags, effect, error_msgs);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXEffectCompilerImpl_CompileShader(ID3DXEffectCompiler *iface, D3DXHANDLE function,
        const char *target, DWORD flags, ID3DXBuffer **shader, ID3DXBuffer **error_msgs,
        ID3DXConstantTable **constant_table)
{
    FIXME("iface %p, function %p, target %s, flags %#x, shader %p, error_msgs %p, constant_table %p stub!\n",
            iface, function, debugstr_a(target), flags, shader, error_msgs, constant_table);

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

static HRESULT d3dx9_parse_sampler(struct d3dx9_base_effect *base, struct d3dx_sampler *sampler,
        const char *data, const char **ptr, struct d3dx_object *objects)
{
    HRESULT hr;
    UINT i;

    read_dword(ptr, &sampler->state_count);
    TRACE("Count: %u\n", sampler->state_count);

    sampler->states = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*sampler->states) * sampler->state_count);
    if (!sampler->states)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < sampler->state_count; ++i)
    {
        hr = d3dx9_parse_state(base, &sampler->states[i], data, ptr, objects);
        if (hr != D3D_OK)
        {
            WARN("Failed to parse state %u\n", i);
            goto err_out;
        }
    }

    return D3D_OK;

err_out:

    for (i = 0; i < sampler->state_count; ++i)
    {
        free_state(&sampler->states[i]);
    }
    HeapFree(GetProcessHeap(), 0, sampler->states);
    sampler->states = NULL;

    return hr;
}

static HRESULT d3dx9_parse_value(struct d3dx9_base_effect *base, struct d3dx_parameter *param,
        void *value, const char *data, const char **ptr, struct d3dx_object *objects)
{
    unsigned int i;
    HRESULT hr;
    UINT old_size = 0;

    if (param->element_count)
    {
        param->data = value;

        for (i = 0; i < param->element_count; ++i)
        {
            struct d3dx_parameter *member = &param->members[i];

            hr = d3dx9_parse_value(base, member, value ? (char *)value + old_size : NULL, data, ptr, objects);
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

                hr = d3dx9_parse_value(base, member, (char *)value + old_size, data, ptr, objects);
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
                    read_dword(ptr, &param->object_id);
                    TRACE("Id: %u\n", param->object_id);
                    objects[param->object_id].param = param;
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

                    hr = d3dx9_parse_sampler(base, sampler, data, ptr, objects);
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

static HRESULT d3dx9_parse_init_value(struct d3dx9_base_effect *base, struct d3dx_parameter *param,
        const char *data, const char *ptr, struct d3dx_object *objects)
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

    hr = d3dx9_parse_value(base, param, value, data, &ptr, objects);
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

static HRESULT d3dx9_copy_data(struct d3dx9_base_effect *base, unsigned int object_id, const char **ptr)
{
    struct d3dx_object *object = &base->objects[object_id];

    if (object->size || object->data)
    {
        if (object_id)
            FIXME("Overwriting object id %u!\n", object_id);
        else
            TRACE("Overwriting object id 0.\n");

        HeapFree(GetProcessHeap(), 0, object->data);
        object->data = NULL;
    }

    read_dword(ptr, &object->size);
    TRACE("Data size: %#x.\n", object->size);

    if (!object->size)
        return D3D_OK;

    object->data = HeapAlloc(GetProcessHeap(), 0, object->size);
    if (!object->data)
    {
        ERR("Failed to allocate object memory.\n");
        return E_OUTOFMEMORY;
    }

    TRACE("Data: %s.\n", debugstr_an(*ptr, object->size));
    memcpy(object->data, *ptr, object->size);

    *ptr += ((object->size + 3) & ~3);

    return D3D_OK;
}

static void param_set_magic_number(struct d3dx_parameter *param)
{
    memcpy(param->magic_string, parameter_magic_string, sizeof(parameter_magic_string));
}

static int param_rb_compare(const void *key, const struct wine_rb_entry *entry)
{
    const char *name = key;
    struct d3dx_parameter *param = WINE_RB_ENTRY_VALUE(entry, struct d3dx_parameter, rb_entry);

    return strcmp(name, param->full_name);
}

static void add_param_to_tree(struct d3dx9_base_effect *base, struct d3dx_parameter *param,
        struct d3dx_parameter *parent, char separator, unsigned int element)
{
    const char *parent_name = parent ? parent->full_name : NULL;
    unsigned int i;

    TRACE("Adding parameter %p (%s - parent %p, element %u) to the rbtree.\n",
            param, debugstr_a(param->name), parent, element);

    if (parent_name)
    {
        unsigned int parent_name_len = strlen(parent_name);
        unsigned int name_len = strlen(param->name);
        unsigned int part_str_len;
        unsigned int len;
        char part_str[16];

        if (separator == '[')
        {
            sprintf(part_str, "[%u]", element);
            part_str_len = strlen(part_str);
            name_len = 0;
        }
        else
        {
            part_str[0] = separator;
            part_str[1] = 0;
            part_str_len = 1;
        }
        len = parent_name_len + part_str_len + name_len + 1;

        if (!(param->full_name = heap_alloc(len)))
        {
            ERR("Out of memory.\n");
            return;
        }

        memcpy(param->full_name, parent_name, parent_name_len);
        memcpy(param->full_name + parent_name_len, part_str, part_str_len);
        memcpy(param->full_name + parent_name_len + part_str_len, param->name, name_len);
        param->full_name[len - 1] = 0;
    }
    else
    {
        unsigned int len = strlen(param->name) + 1;

        if (!(param->full_name = heap_alloc(len)))
        {
            ERR("Out of memory.\n");
            return;
        }

        memcpy(param->full_name, param->name, len);
    }
    TRACE("Full name is %s.\n", param->full_name);
    wine_rb_put(&base->param_tree, param->full_name, &param->rb_entry);

    if (is_top_level_parameter(param))
        for (i = 0; i < param->top_level_param->annotation_count; ++i)
            add_param_to_tree(base, &param->top_level_param->annotations[i], param, '@', 0);

    if (param->element_count)
        for (i = 0; i < param->element_count; ++i)
            add_param_to_tree(base, &param->members[i], param, '[', i);
    else
        for (i = 0; i < param->member_count; ++i)
            add_param_to_tree(base, &param->members[i], param, '.', 0);
}

static HRESULT d3dx9_parse_effect_typedef(struct d3dx9_base_effect *base, struct d3dx_parameter *param,
	const char *data, const char **ptr, struct d3dx_parameter *parent, UINT flags)
{
    DWORD offset;
    HRESULT hr;
    UINT i;

    param->flags = flags;

    if (!parent)
    {
        read_dword(ptr, (DWORD *)&param->type);
        TRACE("Type: %s\n", debug_d3dxparameter_type(param->type));

        read_dword(ptr, (DWORD *)&param->class);
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

            param_set_magic_number(&param->members[i]);
            hr = d3dx9_parse_effect_typedef(base, &param->members[i], data, ptr, param, flags);
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
            param_set_magic_number(&param->members[i]);
            hr = d3dx9_parse_effect_typedef(base, &param->members[i], data, ptr, NULL, flags);
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

static HRESULT d3dx9_parse_effect_annotation(struct d3dx9_base_effect *base, struct d3dx_parameter *anno,
        const char *data, const char **ptr, struct d3dx_object *objects)
{
    DWORD offset;
    const char *ptr2;
    HRESULT hr;

    anno->flags = D3DX_PARAMETER_ANNOTATION;

    read_dword(ptr, &offset);
    TRACE("Typedef offset: %#x\n", offset);
    ptr2 = data + offset;
    hr = d3dx9_parse_effect_typedef(base, anno, data, &ptr2, NULL, D3DX_PARAMETER_ANNOTATION);
    if (hr != D3D_OK)
    {
        WARN("Failed to parse type definition\n");
        return hr;
    }

    read_dword(ptr, &offset);
    TRACE("Value offset: %#x\n", offset);
    hr = d3dx9_parse_init_value(base, anno, data, data + offset, objects);
    if (hr != D3D_OK)
    {
        WARN("Failed to parse value\n");
        return hr;
    }

    return D3D_OK;
}

static HRESULT d3dx9_parse_state(struct d3dx9_base_effect *base, struct d3dx_state *state,
        const char *data, const char **ptr, struct d3dx_object *objects)
{
    DWORD offset;
    const char *ptr2;
    HRESULT hr;

    state->type = ST_CONSTANT;

    read_dword(ptr, &state->operation);
    TRACE("Operation: %#x (%s)\n", state->operation, state_table[state->operation].name);

    read_dword(ptr, &state->index);
    TRACE("Index: %#x\n", state->index);

    read_dword(ptr, &offset);
    TRACE("Typedef offset: %#x\n", offset);
    ptr2 = data + offset;
    hr = d3dx9_parse_effect_typedef(base, &state->parameter, data, &ptr2, NULL, 0);
    if (hr != D3D_OK)
    {
        WARN("Failed to parse type definition\n");
        goto err_out;
    }

    read_dword(ptr, &offset);
    TRACE("Value offset: %#x\n", offset);
    hr = d3dx9_parse_init_value(base, &state->parameter, data, data + offset, objects);
    if (hr != D3D_OK)
    {
        WARN("Failed to parse value\n");
        goto err_out;
    }

    return D3D_OK;

err_out:

    free_parameter(&state->parameter, FALSE, FALSE);

    return hr;
}

static HRESULT d3dx9_parse_effect_parameter(struct d3dx9_base_effect *base, struct d3dx_top_level_parameter *param,
        const char *data, const char **ptr, struct d3dx_object *objects)
{
    DWORD offset;
    HRESULT hr;
    unsigned int i;
    const char *ptr2;

    read_dword(ptr, &offset);
    TRACE("Typedef offset: %#x.\n", offset);
    ptr2 = data + offset;

    read_dword(ptr, &offset);
    TRACE("Value offset: %#x.\n", offset);

    read_dword(ptr, &param->param.flags);
    TRACE("Flags: %#x.\n", param->param.flags);

    read_dword(ptr, &param->annotation_count);
    TRACE("Annotation count: %u.\n", param->annotation_count);

    hr = d3dx9_parse_effect_typedef(base, &param->param, data, &ptr2, NULL, param->param.flags);
    if (hr != D3D_OK)
    {
        WARN("Failed to parse type definition.\n");
        return hr;
    }

    hr = d3dx9_parse_init_value(base, &param->param, data, data + offset, objects);
    if (hr != D3D_OK)
    {
        WARN("Failed to parse value.\n");
        return hr;
    }

    if (param->annotation_count)
    {
        param->annotations = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                sizeof(*param->annotations) * param->annotation_count);
        if (!param->annotations)
        {
            ERR("Out of memory.\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }

        for (i = 0; i < param->annotation_count; ++i)
        {
            param_set_magic_number(&param->annotations[i]);
            hr = d3dx9_parse_effect_annotation(base, &param->annotations[i], data, ptr, objects);
            if (hr != D3D_OK)
            {
                WARN("Failed to parse annotation.\n");
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

static HRESULT d3dx9_parse_effect_pass(struct d3dx9_base_effect *base, struct d3dx_pass *pass,
        const char *data, const char **ptr, struct d3dx_object *objects)
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
            param_set_magic_number(&pass->annotations[i]);
            hr = d3dx9_parse_effect_annotation(base, &pass->annotations[i], data, ptr, objects);
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
            hr = d3dx9_parse_state(base, &states[i], data, ptr, objects);
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

static HRESULT d3dx9_parse_effect_technique(struct d3dx9_base_effect *base, struct d3dx_technique *technique,
        const char *data, const char **ptr, struct d3dx_object *objects)
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
            param_set_magic_number(&technique->annotations[i]);
            hr = d3dx9_parse_effect_annotation(base, &technique->annotations[i], data, ptr, objects);
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
            hr = d3dx9_parse_effect_pass(base, &technique->passes[i], data, ptr, objects);
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

static HRESULT d3dx9_create_object(struct d3dx9_base_effect *base, struct d3dx_object *object)
{
    struct d3dx_parameter *param = object->param;
    struct IDirect3DDevice9 *device = base->effect->device;
    HRESULT hr;

    if (*(char **)param->data)
        ERR("Parameter data already allocated.\n");

    switch (param->type)
    {
        case D3DXPT_STRING:
            *(char **)param->data = HeapAlloc(GetProcessHeap(), 0, object->size);
            if (!*(char **)param->data)
            {
                ERR("Out of memory.\n");
                return E_OUTOFMEMORY;
            }
            memcpy(*(char **)param->data, object->data, object->size);
            break;
        case D3DXPT_VERTEXSHADER:
            if (FAILED(hr = IDirect3DDevice9_CreateVertexShader(device, object->data,
                    (IDirect3DVertexShader9 **)param->data)))
            {
                WARN("Failed to create vertex shader.\n");
                object->creation_failed = TRUE;
            }
            break;
        case D3DXPT_PIXELSHADER:
            if (FAILED(hr = IDirect3DDevice9_CreatePixelShader(device, object->data,
                    (IDirect3DPixelShader9 **)param->data)))
            {
                WARN("Failed to create pixel shader.\n");
                object->creation_failed = TRUE;
            }
            break;
        default:
            break;
    }
    return D3D_OK;
}

static HRESULT d3dx9_parse_array_selector(struct d3dx9_base_effect *base, struct d3dx_state *state,
        const char **skip_constants, unsigned int skip_constants_count)
{
    DWORD string_size;
    struct d3dx_parameter *param = &state->parameter;
    struct d3dx_object *object = &base->objects[param->object_id];
    char *ptr = object->data;
    HRESULT ret;

    TRACE("Parsing array entry selection state for parameter %p.\n", param);

    string_size = *(DWORD *)ptr;
    state->referenced_param = get_parameter_by_name(base, NULL, ptr + 4);
    if (state->referenced_param)
    {
        TRACE("Mapping to parameter %s.\n", debugstr_a(state->referenced_param->name));
    }
    else
    {
        FIXME("Referenced parameter %s not found.\n", ptr + 4);
        return D3DXERR_INVALIDDATA;
    }
    TRACE("Unknown DWORD: 0x%.8x.\n", *(DWORD *)(ptr + string_size));

    if (string_size % sizeof(DWORD))
        FIXME("Unaligned string_size %u.\n", string_size);
    if (FAILED(ret = d3dx_create_param_eval(base, (DWORD *)(ptr + string_size) + 1,
            object->size - (string_size + sizeof(DWORD)), D3DXPT_INT, &param->param_eval,
            get_version_counter_ptr(base), NULL, 0)))
        return ret;
    ret = D3D_OK;
    param = state->referenced_param;
    if (param->type == D3DXPT_VERTEXSHADER || param->type == D3DXPT_PIXELSHADER)
    {
        unsigned int i;

        for (i = 0; i < param->element_count; i++)
        {
            if (param->members[i].type != param->type)
            {
                FIXME("Unexpected member parameter type %u, expected %u.\n", param->members[i].type, param->type);
                return D3DXERR_INVALIDDATA;
            }
            if (!param->members[i].param_eval)
            {
                TRACE("Creating preshader for object %u.\n", param->members[i].object_id);
                object = &base->objects[param->members[i].object_id];
                if (FAILED(ret = d3dx_create_param_eval(base, object->data, object->size, param->type,
                        &param->members[i].param_eval, get_version_counter_ptr(base),
                        skip_constants, skip_constants_count)))
                    break;
            }
        }
    }
    return ret;
}

static HRESULT d3dx9_parse_resource(struct d3dx9_base_effect *base, const char *data, const char **ptr,
        const char **skip_constants, unsigned int skip_constants_count)
{
    DWORD technique_index;
    DWORD index, state_index, usage, element_index;
    struct d3dx_state *state;
    struct d3dx_parameter *param;
    struct d3dx_object *object;
    HRESULT hr = E_FAIL;

    read_dword(ptr, &technique_index);
    TRACE("technique_index: %u\n", technique_index);

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

        parameter = &base->parameters[index].param;
        if (element_index != 0xffffffff)
        {
            if (element_index >= parameter->element_count && parameter->element_count != 0)
            {
                FIXME("Index out of bounds: element_index %u >= element_count %u\n", element_index, parameter->element_count);
                return E_FAIL;
            }

            if (parameter->element_count)
                parameter = &parameter->members[element_index];
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

    TRACE("State operation %#x (%s).\n", state->operation, state_table[state->operation].name);
    param = &state->parameter;
    TRACE("Using object id %u.\n", param->object_id);
    object = &base->objects[param->object_id];

    TRACE("Usage %u: class %s, type %s.\n", usage, debug_d3dxparameter_class(param->class),
            debug_d3dxparameter_type(param->type));
    switch (usage)
    {
        case 0:
            switch (param->type)
            {
                case D3DXPT_VERTEXSHADER:
                case D3DXPT_PIXELSHADER:
                    state->type = ST_CONSTANT;
                    if (FAILED(hr = d3dx9_copy_data(base, param->object_id, ptr)))
                        return hr;

                    if (object->data)
                    {
                        if (FAILED(hr = d3dx9_create_object(base, object)))
                            return hr;
                        if (FAILED(hr = d3dx_create_param_eval(base, object->data, object->size, param->type,
                                &param->param_eval, get_version_counter_ptr(base),
                                skip_constants, skip_constants_count)))
                            return hr;
                    }
                    break;

                case D3DXPT_BOOL:
                case D3DXPT_INT:
                case D3DXPT_FLOAT:
                case D3DXPT_STRING:
                    state->type = ST_FXLC;
                    if (FAILED(hr = d3dx9_copy_data(base, param->object_id, ptr)))
                        return hr;
                    if (FAILED(hr = d3dx_create_param_eval(base, object->data, object->size, param->type,
                            &param->param_eval, get_version_counter_ptr(base), NULL, 0)))
                        return hr;
                    break;

                default:
                    FIXME("Unhandled type %s\n", debug_d3dxparameter_type(param->type));
                    break;
            }
            break;

        case 1:
            state->type = ST_PARAMETER;
            if (FAILED(hr = d3dx9_copy_data(base, param->object_id, ptr)))
                return hr;

            TRACE("Looking for parameter %s.\n", debugstr_a(object->data));
            state->referenced_param = get_parameter_by_name(base, NULL, object->data);
            if (state->referenced_param)
            {
                struct d3dx_parameter *refpar = state->referenced_param;

                TRACE("Mapping to parameter %p, having object id %u.\n", refpar, refpar->object_id);
                if (refpar->type == D3DXPT_VERTEXSHADER || refpar->type == D3DXPT_PIXELSHADER)
                {
                    struct d3dx_object *refobj = &base->objects[refpar->object_id];

                    if (!refpar->param_eval)
                    {
                        if (FAILED(hr = d3dx_create_param_eval(base, refobj->data, refobj->size,
                                refpar->type, &refpar->param_eval, get_version_counter_ptr(base),
                                skip_constants, skip_constants_count)))
                            return hr;
                    }
                }
            }
            else
            {
                FIXME("Referenced parameter %s not found.\n", (char *)object->data);
                return D3DXERR_INVALIDDATA;
            }
            break;

        case 2:
            state->type = ST_ARRAY_SELECTOR;
            if (FAILED(hr = d3dx9_copy_data(base, param->object_id, ptr)))
                return hr;
            hr = d3dx9_parse_array_selector(base, state, skip_constants, skip_constants_count);
            break;

        default:
            FIXME("Unknown usage %x\n", usage);
            break;
    }

    return hr;
}

static BOOL param_set_top_level_param(void *top_level_param, struct d3dx_parameter *param)
{
    param->top_level_param = top_level_param;
    return FALSE;
}

static HRESULT d3dx9_parse_effect(struct d3dx9_base_effect *base, const char *data, UINT data_size,
        DWORD start, const char **skip_constants, unsigned int skip_constants_count)
{
    const char *ptr = data + start;
    UINT stringcount, resourcecount;
    HRESULT hr;
    UINT i;

    read_dword(&ptr, &base->parameter_count);
    TRACE("Parameter count: %u.\n", base->parameter_count);

    read_dword(&ptr, &base->technique_count);
    TRACE("Technique count: %u.\n", base->technique_count);

    skip_dword_unknown(&ptr, 1);

    read_dword(&ptr, &base->object_count);
    TRACE("Object count: %u.\n", base->object_count);

    base->objects = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*base->objects) * base->object_count);
    if (!base->objects)
    {
        ERR("Out of memory.\n");
        hr = E_OUTOFMEMORY;
        goto err_out;
    }

    wine_rb_init(&base->param_tree, param_rb_compare);
    if (base->parameter_count)
    {
        base->parameters = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                sizeof(*base->parameters) * base->parameter_count);
        if (!base->parameters)
        {
            ERR("Out of memory.\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }

        for (i = 0; i < base->parameter_count; ++i)
        {
            param_set_magic_number(&base->parameters[i].param);
            hr = d3dx9_parse_effect_parameter(base, &base->parameters[i], data, &ptr, base->objects);
            if (hr != D3D_OK)
            {
                WARN("Failed to parse parameter %u.\n", i);
                goto err_out;
            }
            walk_parameter_tree(&base->parameters[i].param, param_set_top_level_param,
                &base->parameters[i]);
            add_param_to_tree(base, &base->parameters[i].param, NULL, 0, 0);
        }
    }

    if (base->technique_count)
    {
        base->techniques = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                sizeof(*base->techniques) * base->technique_count);
        if (!base->techniques)
        {
            ERR("Out of memory.\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }

        for (i = 0; i < base->technique_count; ++i)
        {
            TRACE("Parsing technique %u.\n", i);
            hr = d3dx9_parse_effect_technique(base, &base->techniques[i], data, &ptr, base->objects);
            if (hr != D3D_OK)
            {
                WARN("Failed to parse technique %u.\n", i);
                goto err_out;
            }
        }
    }

    read_dword(&ptr, &stringcount);
    TRACE("String count: %u.\n", stringcount);

    read_dword(&ptr, &resourcecount);
    TRACE("Resource count: %u.\n", resourcecount);

    for (i = 0; i < stringcount; ++i)
    {
        DWORD id;

        read_dword(&ptr, &id);
        TRACE("id: %u.\n", id);

        if (FAILED(hr = d3dx9_copy_data(base, id, &ptr)))
            goto err_out;

        if (base->objects[id].data)
        {
            if (FAILED(hr = d3dx9_create_object(base, &base->objects[id])))
                goto err_out;
        }
    }

    for (i = 0; i < resourcecount; ++i)
    {
        TRACE("parse resource %u.\n", i);

        hr = d3dx9_parse_resource(base, data, &ptr, skip_constants, skip_constants_count);
        if (hr != D3D_OK)
        {
            WARN("Failed to parse resource %u.\n", i);
            goto err_out;
        }
    }

    for (i = 0; i < base->parameter_count; ++i)
    {
        if (FAILED(hr = d3dx_pool_sync_shared_parameter(base->pool, &base->parameters[i])))
            goto err_out;
        base->parameters[i].version_counter = base->pool
                ? &base->pool->version_counter
                : &base->version_counter;
        set_dirty(&base->parameters[i].param);
    }
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
            free_top_level_parameter(&base->parameters[i]);
        }
        HeapFree(GetProcessHeap(), 0, base->parameters);
        base->parameters = NULL;
    }

    if (base->objects)
    {
        for (i = 0; i < base->object_count; ++i)
        {
            free_object(&base->objects[i]);
        }
        HeapFree(GetProcessHeap(), 0, base->objects);
        base->objects = NULL;
    }

    return hr;
}

#define INITIAL_CONST_NAMES_SIZE 4

static char *next_valid_constant_name(char **string)
{
    char *ret = *string;
    char *next;

    while (*ret && !isalpha(*ret) && *ret != '_')
        ++ret;
    if (!*ret)
        return NULL;

    next = ret + 1;
    while (isalpha(*next) || isdigit(*next) || *next == '_')
        ++next;
    if (*next)
        *next++ = 0;
    *string = next;
    return ret;
}

static const char **parse_skip_constants_string(char *skip_constants_string, unsigned int *names_count)
{
    const char **names, **new_alloc;
    const char *name;
    char *s;
    unsigned int size = INITIAL_CONST_NAMES_SIZE;

    names = HeapAlloc(GetProcessHeap(), 0, sizeof(*names) * size);
    if (!names)
        return NULL;

    *names_count = 0;
    s = skip_constants_string;
    while ((name = next_valid_constant_name(&s)))
    {
        if (*names_count == size)
        {
            size *= 2;
            new_alloc = HeapReAlloc(GetProcessHeap(), 0, names, sizeof(*names) * size);
            if (!new_alloc)
            {
                HeapFree(GetProcessHeap(), 0, names);
                return NULL;
            }
            names = new_alloc;
        }
        names[(*names_count)++] = name;
    }
    new_alloc = HeapReAlloc(GetProcessHeap(), 0, names, *names_count * sizeof(*names));
    if (!new_alloc)
        return names;
    return new_alloc;
}

static HRESULT d3dx9_base_effect_init(struct d3dx9_base_effect *base,
        const char *data, SIZE_T data_size, const D3D_SHADER_MACRO *defines, ID3DInclude *include,
        UINT eflags, ID3DBlob **errors, struct ID3DXEffectImpl *effect, struct d3dx_effect_pool *pool,
        const char *skip_constants_string)
{
    DWORD tag, offset;
    const char *ptr = data;
    HRESULT hr;
    ID3DBlob *bytecode = NULL, *temp_errors = NULL;
    char *skip_constants_buffer = NULL;
    const char **skip_constants = NULL;
    unsigned int skip_constants_count = 0;
#if D3DX_SDK_VERSION <= 36
    UINT compile_flags = D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY;
#else
    UINT compile_flags = 0;
#endif
    unsigned int i, j;

    TRACE("base %p, data %p, data_size %lu, defines %p, include %p, eflags %#x, errors %p, "
            "effect %p, pool %p, skip_constants %s.\n",
            base, data, data_size, defines, include, eflags, errors, effect, pool,
            debugstr_a(skip_constants_string));

    base->effect = effect;
    base->pool = pool;
    base->flags = eflags;

    read_dword(&ptr, &tag);
    TRACE("Tag: %x\n", tag);

    if (tag != d3dx9_effect_version(9, 1))
    {
        TRACE("HLSL ASCII effect, trying to compile it.\n");
        hr = D3DCompile(data, data_size, NULL, defines, include,
                NULL, "fx_2_0", compile_flags, eflags, &bytecode, &temp_errors);
        if (FAILED(hr))
        {
            WARN("Failed to compile ASCII effect.\n");
            if (bytecode)
                ID3D10Blob_Release(bytecode);
            if (temp_errors)
            {
                const char *error_string = ID3D10Blob_GetBufferPointer(temp_errors);
                const char *string_ptr;

                while (*error_string)
                {
                    string_ptr = error_string;
                    while (*string_ptr && *string_ptr != '\n' && *string_ptr != '\r'
                           && string_ptr - error_string < 80)
                        ++string_ptr;
                    TRACE("%s\n", debugstr_an(error_string, string_ptr - error_string));
                    error_string = string_ptr;
                    while (*error_string == '\n' || *error_string == '\r')
                        ++error_string;
                }
            }
            if (errors)
                *errors = temp_errors;
            else if (temp_errors)
                ID3D10Blob_Release(temp_errors);
            return hr;
        }
        if (!bytecode)
        {
            FIXME("No output from effect compilation.\n");
            return D3DERR_INVALIDCALL;
        }
        if (errors)
            *errors = temp_errors;
        else if (temp_errors)
            ID3D10Blob_Release(temp_errors);

        ptr = ID3D10Blob_GetBufferPointer(bytecode);
        read_dword(&ptr, &tag);
        TRACE("Tag: %x\n", tag);
    }

    if (skip_constants_string)
    {
        skip_constants_buffer = HeapAlloc(GetProcessHeap(), 0,
                sizeof(*skip_constants_buffer) * (strlen(skip_constants_string) + 1));
        if (!skip_constants_buffer)
        {
            if (bytecode)
                ID3D10Blob_Release(bytecode);
            return E_OUTOFMEMORY;
        }
        strcpy(skip_constants_buffer, skip_constants_string);

        if (!(skip_constants = parse_skip_constants_string(skip_constants_buffer, &skip_constants_count)))
        {
            HeapFree(GetProcessHeap(), 0, skip_constants_buffer);
            if (bytecode)
                ID3D10Blob_Release(bytecode);
            return E_OUTOFMEMORY;
        }
    }
    read_dword(&ptr, &offset);
    TRACE("Offset: %x\n", offset);

    hr = d3dx9_parse_effect(base, ptr, data_size, offset, skip_constants, skip_constants_count);
    if (bytecode)
        ID3D10Blob_Release(bytecode);
    if (hr != D3D_OK)
    {
        FIXME("Failed to parse effect.\n");
        HeapFree(GetProcessHeap(), 0, skip_constants_buffer);
        HeapFree(GetProcessHeap(), 0, skip_constants);
        return hr;
    }

    for (i = 0; i < skip_constants_count; ++i)
    {
        struct d3dx_parameter *param;
        param = get_parameter_by_name(base, NULL, skip_constants[i]);
        if (param)
        {
            for (j = 0; j < base->technique_count; ++j)
            {
                if (is_parameter_used(param, &base->techniques[j]))
                {
                    WARN("skip_constants parameter %s is used in technique %u.\n",
                            debugstr_a(skip_constants[i]), j);
                    HeapFree(GetProcessHeap(), 0, skip_constants_buffer);
                    HeapFree(GetProcessHeap(), 0, skip_constants);
                    d3dx9_base_effect_cleanup(base);
                    return D3DERR_INVALIDCALL;
                }
            }
        }
        else
        {
            TRACE("skip_constants parameter %s not found.\n",
                    debugstr_a(skip_constants[i]));
        }
    }

    HeapFree(GetProcessHeap(), 0, skip_constants_buffer);
    HeapFree(GetProcessHeap(), 0, skip_constants);

    return D3D_OK;
}

static HRESULT d3dx9_effect_init(struct ID3DXEffectImpl *effect, struct IDirect3DDevice9 *device,
        const char *data, SIZE_T data_size, const D3D_SHADER_MACRO *defines, ID3DInclude *include,
        UINT eflags, ID3DBlob **error_messages, struct ID3DXEffectPool *pool, const char *skip_constants)
{
    HRESULT hr;
    struct d3dx_effect_pool *pool_impl = NULL;

    TRACE("effect %p, device %p, data %p, data_size %lu, pool %p\n", effect, device, data, data_size, pool);

    effect->ID3DXEffect_iface.lpVtbl = &ID3DXEffect_Vtbl;
    effect->ref = 1;

    if (pool)
    {
        pool->lpVtbl->AddRef(pool);
        pool_impl = impl_from_ID3DXEffectPool(pool);
    }
    effect->pool = pool;

    IDirect3DDevice9_AddRef(device);
    effect->device = device;

    if (FAILED(hr = d3dx9_base_effect_init(&effect->base_effect, data, data_size, defines, include,
            eflags, error_messages, effect, pool_impl, skip_constants)))
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

    TRACE("device %p, srcdata %p, srcdatalen %u, defines %p, include %p,"
            " skip_constants %p, flags %#x, pool %p, effect %p, compilation_errors %p.\n",
            device, srcdata, srcdatalen, defines, include,
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

    hr = d3dx9_effect_init(object, device, srcdata, srcdatalen, (const D3D_SHADER_MACRO *)defines,
            (ID3DInclude *)include, flags, (ID3DBlob **)compilation_errors, pool, skip_constants);
    if (FAILED(hr))
    {
        WARN("Failed to create effect object.\n");
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

static HRESULT d3dx9_effect_compiler_init(struct ID3DXEffectCompilerImpl *compiler,
        const char *data, SIZE_T data_size, const D3D_SHADER_MACRO *defines, ID3DInclude *include,
        UINT eflags, ID3DBlob **error_messages)
{
    TRACE("compiler %p, data %p, data_size %lu, defines %p, include %p, eflags %#x, error_messages %p.\n",
            compiler, data, data_size, defines, include, eflags, error_messages);

    compiler->ID3DXEffectCompiler_iface.lpVtbl = &ID3DXEffectCompiler_Vtbl;
    compiler->ref = 1;

    FIXME("ID3DXEffectCompiler implementation is only a stub.\n");

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

    hr = d3dx9_effect_compiler_init(object, srcdata, srcdatalen, (const D3D_SHADER_MACRO *)defines,
            (ID3DInclude *)include, flags, (ID3DBlob **)parse_errors);
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

/*** IUnknown methods ***/
static HRESULT WINAPI d3dx_effect_pool_QueryInterface(ID3DXEffectPool *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

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

static ULONG WINAPI d3dx_effect_pool_AddRef(ID3DXEffectPool *iface)
{
    struct d3dx_effect_pool *pool = impl_from_ID3DXEffectPool(iface);
    ULONG refcount = InterlockedIncrement(&pool->refcount);

    TRACE("%p increasing refcount to %u.\n", pool, refcount);

    return refcount;
}

static void free_effect_pool(struct d3dx_effect_pool *pool)
{
    unsigned int i;

    for (i = 0; i < pool->size; ++i)
    {
        if (pool->shared_data[i].count)
        {
            unsigned int j;

            WARN("Releasing pool with referenced parameters.\n");

            param_set_data_pointer(&pool->shared_data[i].parameters[0]->param, NULL, FALSE, TRUE);
            pool->shared_data[i].parameters[0]->shared_data = NULL;

            for (j = 1; j < pool->shared_data[i].count; ++j)
            {
                walk_parameter_tree(&pool->shared_data[i].parameters[j]->param, param_zero_data_func, NULL);
                pool->shared_data[i].parameters[j]->shared_data = NULL;
            }
            HeapFree(GetProcessHeap(), 0, pool->shared_data[i].parameters);
        }
    }
    HeapFree(GetProcessHeap(), 0, pool->shared_data);
    HeapFree(GetProcessHeap(), 0, pool);
}

static ULONG WINAPI d3dx_effect_pool_Release(ID3DXEffectPool *iface)
{
    struct d3dx_effect_pool *pool = impl_from_ID3DXEffectPool(iface);
    ULONG refcount = InterlockedDecrement(&pool->refcount);

    TRACE("%p decreasing refcount to %u.\n", pool, refcount);

    if (!refcount)
        free_effect_pool(pool);

    return refcount;
}

static const struct ID3DXEffectPoolVtbl ID3DXEffectPool_Vtbl =
{
    /*** IUnknown methods ***/
    d3dx_effect_pool_QueryInterface,
    d3dx_effect_pool_AddRef,
    d3dx_effect_pool_Release
};

HRESULT WINAPI D3DXCreateEffectPool(ID3DXEffectPool **pool)
{
    struct d3dx_effect_pool *object;

    TRACE("pool %p.\n", pool);

    if (!pool)
        return D3DERR_INVALIDCALL;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->ID3DXEffectPool_iface.lpVtbl = &ID3DXEffectPool_Vtbl;
    object->refcount = 1;

    *pool = &object->ID3DXEffectPool_iface;

    return S_OK;
}

HRESULT WINAPI D3DXCreateEffectFromFileExW(struct IDirect3DDevice9 *device, const WCHAR *srcfile,
        const D3DXMACRO *defines, struct ID3DXInclude *include, const char *skipconstants, DWORD flags,
        struct ID3DXEffectPool *pool, struct ID3DXEffect **effect, struct ID3DXBuffer **compilationerrors)
{
    struct d3dx_include_from_file include_from_file;
    const void *buffer;
    unsigned int size;
    char *filename_a;
    HRESULT ret;

    TRACE("device %p, srcfile %s, defines %p, include %p, skipconstants %s, "
            "flags %#x, pool %p, effect %p, compilationerrors %p.\n",
            device, debugstr_w(srcfile), defines, include, debugstr_a(skipconstants),
            flags, pool, effect, compilationerrors);

    if (!device || !srcfile)
        return D3DERR_INVALIDCALL;

    if (!include)
    {
        include_from_file.ID3DXInclude_iface.lpVtbl = &d3dx_include_from_file_vtbl;
        include = &include_from_file.ID3DXInclude_iface;
    }

    size = WideCharToMultiByte(CP_ACP, 0, srcfile, -1, NULL, 0, NULL, NULL);
    filename_a = heap_alloc(size);
    if (!filename_a)
        return E_OUTOFMEMORY;
    WideCharToMultiByte(CP_ACP, 0, srcfile, -1, filename_a, size, NULL, NULL);

    EnterCriticalSection(&from_file_mutex);
    ret = ID3DXInclude_Open(include, D3DXINC_LOCAL, filename_a, NULL, &buffer, &size);
    if (FAILED(ret))
    {
        LeaveCriticalSection(&from_file_mutex);
        heap_free(filename_a);
        return D3DXERR_INVALIDDATA;
    }

    ret = D3DXCreateEffectEx(device, buffer, size, defines, include, skipconstants, flags, pool,
            effect, compilationerrors);

    ID3DXInclude_Close(include, buffer);
    LeaveCriticalSection(&from_file_mutex);
    heap_free(filename_a);
    return ret;
}

HRESULT WINAPI D3DXCreateEffectFromFileExA(struct IDirect3DDevice9 *device, const char *srcfile,
        const D3DXMACRO *defines, struct ID3DXInclude *include, const char *skipconstants, DWORD flags,
        struct ID3DXEffectPool *pool, struct ID3DXEffect **effect, struct ID3DXBuffer **compilationerrors)
{
    WCHAR *srcfileW;
    HRESULT ret;
    DWORD len;

    TRACE("device %p, srcfile %s, defines %p, include %p, skipconstants %s, "
            "flags %#x, pool %p, effect %p, compilationerrors %p.\n",
            device, debugstr_a(srcfile), defines, include, debugstr_a(skipconstants),
            flags, pool, effect, compilationerrors);

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
    void *buffer;
    HRESULT ret;
    DWORD size;

    TRACE("srcfile %s, defines %p, include %p, flags %#x, effectcompiler %p, parseerrors %p.\n",
            debugstr_w(srcfile), defines, include, flags, effectcompiler, parseerrors);

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
    WCHAR *srcfileW;
    HRESULT ret;
    DWORD len;

    TRACE("srcfile %s, defines %p, include %p, flags %#x, effectcompiler %p, parseerrors %p.\n",
            debugstr_a(srcfile), defines, include, flags, effectcompiler, parseerrors);

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

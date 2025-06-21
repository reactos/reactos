/*
 * Copyright 2008-2009 Henri Verbeet for CodeWeavers
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

#ifndef __WINE_D3D10_PRIVATE_H
#define __WINE_D3D10_PRIVATE_H

#include <math.h>
#include <stdint.h>

#include "wine/debug.h"
#include "wine/rbtree.h"

#define COBJMACROS
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"

#include "d3d10.h"
#include "d3dcompiler.h"
#include "utils.h"

/*
 * This doesn't belong here, but for some functions it is possible to return that value,
 * see http://msdn.microsoft.com/en-us/library/bb205278%28v=VS.85%29.aspx
 * The original definition is in D3DX10core.h.
 */
#define D3DERR_INVALIDCALL 0x8876086c

enum d3d10_effect_object_type_flags
{
    D3D10_EOT_FLAG_GS_SO = 0x1,
};

enum d3d10_effect_object_operation
{
    D3D10_EOO_CONST = 1,
    D3D10_EOO_VAR = 2,
    D3D10_EOO_CONST_INDEX = 3,
    D3D10_EOO_VAR_INDEX = 4,
    D3D10_EOO_INDEX_EXPRESSION = 5,
    D3D10_EOO_VALUE_EXPRESSION = 6,
    D3D10_EOO_ANONYMOUS_SHADER = 7,
};

struct d3d10_matrix
{
    float m[4][4];
};

struct d3d10_effect_shader_resource
{
    D3D10_SHADER_INPUT_TYPE in_type;
    unsigned int bind_point;
    unsigned int bind_count;

    struct d3d10_effect_variable *variable;
};

struct d3d10_effect_shader_signature
{
    char *signature;
    unsigned int signature_size;
    unsigned int element_count;
    D3D10_SIGNATURE_PARAMETER_DESC *elements;
};

struct d3d10_effect_shader_variable
{
    ID3D10ShaderReflection *reflection;
    ID3D10Blob *input_signature;
    ID3D10Blob *bytecode;
    union
    {
        ID3D10VertexShader *vs;
        ID3D10PixelShader *ps;
        ID3D10GeometryShader *gs;
        IUnknown *object;
    } shader;

    unsigned int resource_count;
    struct d3d10_effect_shader_resource *resources;
    char *stream_output_declaration;
    unsigned int isinline : 1;
};

struct d3d10_effect_prop_dependencies
{
    struct d3d10_effect_prop_dependency *entries;
    SIZE_T count;
    SIZE_T capacity;
};

struct d3d10_effect_sampler_desc
{
    D3D10_SAMPLER_DESC desc;
    struct d3d10_effect_variable *texture;
};

struct d3d10_effect_state_object_variable
{
    union
    {
        D3D10_RASTERIZER_DESC rasterizer;
        D3D10_DEPTH_STENCIL_DESC depth_stencil;
        D3D10_BLEND_DESC blend;
        struct d3d10_effect_sampler_desc sampler;
    } desc;
    union
    {
        ID3D10RasterizerState *rasterizer;
        ID3D10DepthStencilState *depth_stencil;
        ID3D10BlendState *blend;
        ID3D10SamplerState *sampler;
        IUnknown *object;
    } object;
    unsigned int index;
    struct d3d10_effect_prop_dependencies dependencies;
};

struct d3d10_effect_resource_variable
{
    ID3D10ShaderResourceView **srv;
    BOOL parent;
};

struct d3d10_effect_buffer_variable
{
    ID3D10Buffer *buffer;
    ID3D10ShaderResourceView *resource_view;

    BOOL changed;
    BYTE *local_buffer;
};

/* ID3D10EffectType */
struct d3d10_effect_type
{
    ID3D10EffectType ID3D10EffectType_iface;

    char *name;
    D3D10_SHADER_VARIABLE_TYPE basetype;
    D3D10_SHADER_VARIABLE_CLASS type_class;
    unsigned int flags;

    DWORD id;
    struct wine_rb_entry entry;
    struct d3d10_effect *effect;

    unsigned int element_count;
    unsigned int size_unpacked;
    unsigned int stride;
    unsigned int size_packed;
    unsigned int member_count;
    unsigned int column_count;
    unsigned int row_count;
    struct d3d10_effect_type *elementtype;
    struct d3d10_effect_type_member *members;
};

struct d3d10_effect_type_member
{
    char *name;
    char *semantic;
    uint32_t buffer_offset;
    struct d3d10_effect_type *type;
};

struct d3d10_effect_annotations
{
    struct d3d10_effect_variable *elements;
    unsigned int count;
};

/* ID3D10EffectVariable */
struct d3d10_effect_variable
{
    ID3D10EffectVariable ID3D10EffectVariable_iface;

    struct d3d10_effect_variable *buffer;
    struct d3d10_effect_type *type;

    char *name;
    char *semantic;
    uint32_t buffer_offset;
    uint32_t flag;
    uint32_t data_size;
    unsigned int explicit_bind_point;
    struct d3d10_effect *effect;
    struct d3d10_effect_variable *elements;
    struct d3d10_effect_variable *members;
    struct d3d10_effect_annotations annotations;

    union
    {
        struct d3d10_effect_state_object_variable state;
        struct d3d10_effect_shader_variable shader;
        struct d3d10_effect_buffer_variable buffer;
        struct d3d10_effect_resource_variable resource;
    } u;
};

struct d3d10_effect_pass_shader_desc
{
    struct d3d10_effect_variable *shader;
    unsigned int index;
};

/* ID3D10EffectPass */
struct d3d10_effect_pass
{
    ID3D10EffectPass ID3D10EffectPass_iface;

    struct d3d10_effect_technique *technique;
    char *name;
    struct d3d10_effect_annotations annotations;

    struct d3d10_effect_prop_dependencies dependencies;
    struct d3d10_effect_pass_shader_desc vs;
    struct d3d10_effect_pass_shader_desc ps;
    struct d3d10_effect_pass_shader_desc gs;
    struct d3d10_effect_variable *rasterizer;
    struct d3d10_effect_variable *depth_stencil;
    struct d3d10_effect_variable *blend;
    UINT stencil_ref;
    UINT sample_mask;
    float blend_factor[4];
};

/* ID3D10EffectTechnique */
struct d3d10_effect_technique
{
    ID3D10EffectTechnique ID3D10EffectTechnique_iface;

    struct d3d10_effect *effect;
    char *name;
    struct d3d10_effect_annotations annotations;
    unsigned int pass_count;
    struct d3d10_effect_pass *passes;
};

struct d3d10_effect_anonymous_shader
{
    struct d3d10_effect_variable shader;
    struct d3d10_effect_type type;
};

enum d3d10_effect_flags
{
    D3D10_EFFECT_OPTIMIZED = 0x1,
    D3D10_EFFECT_IS_POOL   = 0x2,
};

struct d3d10_effect_var_array
{
    struct d3d10_effect_variable **v;
    unsigned int current;
    unsigned int count;
};

/* ID3D10Effect */
struct d3d10_effect
{
    ID3D10Effect ID3D10Effect_iface;
    ID3D10EffectPool ID3D10EffectPool_iface;
    LONG refcount;

    ID3D10Device *device;
    struct d3d10_effect *pool;
    uint32_t version;
    unsigned int local_buffer_count;
    unsigned int numeric_variable_count;
    unsigned int object_count;
    unsigned int shared_buffer_count;
    unsigned int shared_object_count;
    unsigned int technique_count;
    uint32_t index_offset;
    unsigned int texture_count;
    unsigned int anonymous_shader_count;
    uint32_t flags;

    unsigned int anonymous_shader_current;

    struct wine_rb_tree types;
    struct d3d10_effect_variable *local_buffers;
    struct d3d10_effect_variable *object_variables;
    struct d3d10_effect_anonymous_shader *anonymous_shaders;
    struct d3d10_effect_var_array shaders;
    struct d3d10_effect_var_array samplers;
    struct d3d10_effect_var_array rtvs;
    struct d3d10_effect_var_array dsvs;
    struct d3d10_effect_var_array blend_states;
    struct d3d10_effect_var_array ds_states;
    struct d3d10_effect_var_array rs_states;
    struct d3d10_effect_technique *techniques;
};

HRESULT d3d10_effect_parse(struct d3d10_effect *effect, const void *data, SIZE_T data_size);

/* D3D10Core */
HRESULT WINAPI D3D10CoreCreateDevice(IDXGIFactory *factory, IDXGIAdapter *adapter,
        unsigned int flags, D3D_FEATURE_LEVEL feature_level, ID3D10Device **device);

/* d3dcompiler_39 function prototypes */
HRESULT WINAPI D3DCompileFromMemory(const void *data, SIZE_T data_size, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, const char *entrypoint,
        const char *target, UINT sflags, UINT eflags, ID3DBlob **shader, ID3DBlob **error_messages);

HRESULT WINAPI D3DDisassembleCode(const void *data, SIZE_T data_size,
        UINT flags, const char *comments, ID3DBlob **disassembly);

#endif /* __WINE_D3D10_PRIVATE_H */

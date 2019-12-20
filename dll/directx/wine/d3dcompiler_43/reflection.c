/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
 * Copyright 2010 Rico Schüller
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
 *
 */

#include "initguid.h"
#include "d3dcompiler_private.h"
#include "wine/winternl.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dcompiler);

enum D3DCOMPILER_SIGNATURE_ELEMENT_SIZE
{
    D3DCOMPILER_SIGNATURE_ELEMENT_SIZE6 = 6,
    D3DCOMPILER_SIGNATURE_ELEMENT_SIZE7 = 7,
};

#define D3DCOMPILER_SHADER_TARGET_VERSION_MASK 0xffff
#define D3DCOMPILER_SHADER_TARGET_SHADERTYPE_MASK 0xffff0000

struct d3dcompiler_shader_signature
{
    D3D11_SIGNATURE_PARAMETER_DESC *elements;
    UINT element_count;
    char *string_data;
};

struct d3dcompiler_shader_reflection_type
{
    ID3D11ShaderReflectionType ID3D11ShaderReflectionType_iface;

    DWORD id;
    struct wine_rb_entry entry;

    struct d3dcompiler_shader_reflection *reflection;

    D3D11_SHADER_TYPE_DESC desc;
    struct d3dcompiler_shader_reflection_type_member *members;
    char *name;
};

struct d3dcompiler_shader_reflection_type_member
{
    char *name;
    DWORD offset;
    struct d3dcompiler_shader_reflection_type *type;
};

struct d3dcompiler_shader_reflection_variable
{
    ID3D11ShaderReflectionVariable ID3D11ShaderReflectionVariable_iface;

    struct d3dcompiler_shader_reflection_constant_buffer *constant_buffer;
    struct d3dcompiler_shader_reflection_type *type;

    char *name;
    UINT start_offset;
    UINT size;
    UINT flags;
    void *default_value;
};

struct d3dcompiler_shader_reflection_constant_buffer
{
    ID3D11ShaderReflectionConstantBuffer ID3D11ShaderReflectionConstantBuffer_iface;

    struct d3dcompiler_shader_reflection *reflection;

    char *name;
    D3D_CBUFFER_TYPE type;
    UINT variable_count;
    UINT size;
    UINT flags;

    struct d3dcompiler_shader_reflection_variable *variables;
};

/* ID3D11ShaderReflection */
struct d3dcompiler_shader_reflection
{
    ID3D11ShaderReflection ID3D11ShaderReflection_iface;
    LONG refcount;

    DWORD target;
    char *creator;
    UINT flags;
    UINT version;
    UINT bound_resource_count;
    UINT constant_buffer_count;

    UINT mov_instruction_count;
    UINT conversion_instruction_count;
    UINT instruction_count;
    UINT emit_instruction_count;
    D3D_PRIMITIVE_TOPOLOGY gs_output_topology;
    UINT gs_max_output_vertex_count;
    D3D_PRIMITIVE input_primitive;
    UINT cut_instruction_count;
    UINT dcl_count;
    UINT static_flow_control_count;
    UINT float_instruction_count;
    UINT temp_register_count;
    UINT int_instruction_count;
    UINT uint_instruction_count;
    UINT temp_array_count;
    UINT array_instruction_count;
    UINT texture_normal_instructions;
    UINT texture_load_instructions;
    UINT texture_comp_instructions;
    UINT texture_bias_instructions;
    UINT texture_gradient_instructions;
    UINT dynamic_flow_control_count;
    UINT c_control_points;
    D3D_TESSELLATOR_OUTPUT_PRIMITIVE hs_output_primitive;
    D3D_TESSELLATOR_PARTITIONING hs_prtitioning;
    D3D_TESSELLATOR_DOMAIN tessellator_domain;

    struct d3dcompiler_shader_signature *isgn;
    struct d3dcompiler_shader_signature *osgn;
    struct d3dcompiler_shader_signature *pcsg;
    char *resource_string;
    D3D11_SHADER_INPUT_BIND_DESC *bound_resources;
    struct d3dcompiler_shader_reflection_constant_buffer *constant_buffers;
    struct wine_rb_tree types;
};

static struct d3dcompiler_shader_reflection_type *get_reflection_type(struct d3dcompiler_shader_reflection *reflection, const char *data, DWORD offset);

static const struct ID3D11ShaderReflectionConstantBufferVtbl d3dcompiler_shader_reflection_constant_buffer_vtbl;
static const struct ID3D11ShaderReflectionVariableVtbl d3dcompiler_shader_reflection_variable_vtbl;
static const struct ID3D11ShaderReflectionTypeVtbl d3dcompiler_shader_reflection_type_vtbl;

/* null objects - needed for invalid calls */
static struct d3dcompiler_shader_reflection_constant_buffer null_constant_buffer = {{&d3dcompiler_shader_reflection_constant_buffer_vtbl}};
static struct d3dcompiler_shader_reflection_type null_type = {{&d3dcompiler_shader_reflection_type_vtbl}};
static struct d3dcompiler_shader_reflection_variable null_variable = {{&d3dcompiler_shader_reflection_variable_vtbl},
    &null_constant_buffer, &null_type};

static BOOL copy_name(const char *ptr, char **name)
{
    size_t name_len;

    if (!ptr) return TRUE;

    name_len = strlen(ptr) + 1;
    if (name_len == 1)
    {
        return TRUE;
    }

    *name = HeapAlloc(GetProcessHeap(), 0, name_len);
    if (!*name)
    {
        ERR("Failed to allocate name memory.\n");
        return FALSE;
    }

    memcpy(*name, ptr, name_len);

    return TRUE;
}

static BOOL copy_value(const char *ptr, void **value, DWORD size)
{
    if (!ptr || !size) return TRUE;

    *value = HeapAlloc(GetProcessHeap(), 0, size);
    if (!*value)
    {
        ERR("Failed to allocate value memory.\n");
        return FALSE;
    }

    memcpy(*value, ptr, size);

    return TRUE;
}

static int d3dcompiler_shader_reflection_type_compare(const void *key, const struct wine_rb_entry *entry)
{
    const struct d3dcompiler_shader_reflection_type *t = WINE_RB_ENTRY_VALUE(entry, const struct d3dcompiler_shader_reflection_type, entry);
    const DWORD *id = key;

    return *id - t->id;
}

static void free_type_member(struct d3dcompiler_shader_reflection_type_member *member)
{
    if (member)
    {
        HeapFree(GetProcessHeap(), 0, member->name);
    }
}

static void d3dcompiler_shader_reflection_type_destroy(struct wine_rb_entry *entry, void *context)
{
    struct d3dcompiler_shader_reflection_type *t = WINE_RB_ENTRY_VALUE(entry, struct d3dcompiler_shader_reflection_type, entry);
    unsigned int i;

    TRACE("reflection type %p.\n", t);

    if (t->members)
    {
        for (i = 0; i < t->desc.Members; ++i)
        {
            free_type_member(&t->members[i]);
        }
        HeapFree(GetProcessHeap(), 0, t->members);
    }

    heap_free(t->name);
    HeapFree(GetProcessHeap(), 0, t);
}

static void free_signature(struct d3dcompiler_shader_signature *sig)
{
    TRACE("Free signature %p\n", sig);

    HeapFree(GetProcessHeap(), 0, sig->elements);
    HeapFree(GetProcessHeap(), 0, sig->string_data);
}

static void free_variable(struct d3dcompiler_shader_reflection_variable *var)
{
    if (var)
    {
        HeapFree(GetProcessHeap(), 0, var->name);
        HeapFree(GetProcessHeap(), 0, var->default_value);
    }
}

static void free_constant_buffer(struct d3dcompiler_shader_reflection_constant_buffer *cb)
{
    if (cb->variables)
    {
        unsigned int i;

        for (i = 0; i < cb->variable_count; ++i)
        {
            free_variable(&cb->variables[i]);
        }
        HeapFree(GetProcessHeap(), 0, cb->variables);
    }

    HeapFree(GetProcessHeap(), 0, cb->name);
}

static void reflection_cleanup(struct d3dcompiler_shader_reflection *ref)
{
    TRACE("Cleanup %p\n", ref);

    if (ref->isgn)
    {
        free_signature(ref->isgn);
        HeapFree(GetProcessHeap(), 0, ref->isgn);
    }

    if (ref->osgn)
    {
        free_signature(ref->osgn);
        HeapFree(GetProcessHeap(), 0, ref->osgn);
    }

    if (ref->pcsg)
    {
        free_signature(ref->pcsg);
        HeapFree(GetProcessHeap(), 0, ref->pcsg);
    }

    if (ref->constant_buffers)
    {
        unsigned int i;

        for (i = 0; i < ref->constant_buffer_count; ++i)
        {
            free_constant_buffer(&ref->constant_buffers[i]);
        }
    }

    wine_rb_destroy(&ref->types, d3dcompiler_shader_reflection_type_destroy, NULL);
    HeapFree(GetProcessHeap(), 0, ref->constant_buffers);
    HeapFree(GetProcessHeap(), 0, ref->bound_resources);
    HeapFree(GetProcessHeap(), 0, ref->resource_string);
    HeapFree(GetProcessHeap(), 0, ref->creator);
}

/* IUnknown methods */

static inline struct d3dcompiler_shader_reflection *impl_from_ID3D11ShaderReflection(ID3D11ShaderReflection *iface)
{
    return CONTAINING_RECORD(iface, struct d3dcompiler_shader_reflection, ID3D11ShaderReflection_iface);
}

static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_QueryInterface(ID3D11ShaderReflection *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11ShaderReflection)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3dcompiler_shader_reflection_AddRef(ID3D11ShaderReflection *iface)
{
    struct d3dcompiler_shader_reflection *This = impl_from_ID3D11ShaderReflection(iface);
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3dcompiler_shader_reflection_Release(ID3D11ShaderReflection *iface)
{
    struct d3dcompiler_shader_reflection *This = impl_from_ID3D11ShaderReflection(iface);
    ULONG refcount = InterlockedDecrement(&This->refcount);

    TRACE("%p decreasing refcount to %u\n", This, refcount);

    if (!refcount)
    {
        reflection_cleanup(This);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refcount;
}

/* ID3D11ShaderReflection methods */

static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetDesc(ID3D11ShaderReflection *iface, D3D11_SHADER_DESC *desc)
{
    struct d3dcompiler_shader_reflection *This = impl_from_ID3D11ShaderReflection(iface);

    FIXME("iface %p, desc %p partial stub!\n", iface, desc);

    if (!desc)
    {
        WARN("Invalid argument specified\n");
        return E_FAIL;
    }

    desc->Version = This->version;
    desc->Creator = This->creator;
    desc->Flags = This->flags;
    desc->ConstantBuffers = This->constant_buffer_count;
    desc->BoundResources = This->bound_resource_count;
    desc->InputParameters = This->isgn ? This->isgn->element_count : 0;
    desc->OutputParameters = This->osgn ? This->osgn->element_count : 0;
    desc->InstructionCount = This->instruction_count;
    desc->TempRegisterCount = This->temp_register_count;
    desc->TempArrayCount = This->temp_array_count;
    desc->DefCount = 0;
    desc->DclCount = This->dcl_count;
    desc->TextureNormalInstructions = This->texture_normal_instructions;
    desc->TextureLoadInstructions = This->texture_load_instructions;
    desc->TextureCompInstructions = This->texture_comp_instructions;
    desc->TextureBiasInstructions = This->texture_bias_instructions;
    desc->TextureGradientInstructions = This->texture_gradient_instructions;
    desc->FloatInstructionCount = This->float_instruction_count;
    desc->IntInstructionCount = This->int_instruction_count;
    desc->UintInstructionCount = This->uint_instruction_count;
    desc->StaticFlowControlCount = This->static_flow_control_count;
    desc->DynamicFlowControlCount = This->dynamic_flow_control_count;
    desc->MacroInstructionCount = 0;
    desc->ArrayInstructionCount = This->array_instruction_count;
    desc->CutInstructionCount = This->cut_instruction_count;
    desc->EmitInstructionCount = This->emit_instruction_count;
    desc->GSOutputTopology = This->gs_output_topology;
    desc->GSMaxOutputVertexCount = This->gs_max_output_vertex_count;
    desc->InputPrimitive = This->input_primitive;
    desc->PatchConstantParameters = This->pcsg ? This->pcsg->element_count : 0;
    desc->cGSInstanceCount = 0;
    desc->cControlPoints = This->c_control_points;
    desc->HSOutputPrimitive = This->hs_output_primitive;
    desc->HSPartitioning = This->hs_prtitioning;
    desc->TessellatorDomain = This->tessellator_domain;
    desc->cBarrierInstructions = 0;
    desc->cInterlockedInstructions = 0;
    desc->cTextureStoreInstructions = 0;

    return S_OK;
}

static struct ID3D11ShaderReflectionConstantBuffer * STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetConstantBufferByIndex(
        ID3D11ShaderReflection *iface, UINT index)
{
    struct d3dcompiler_shader_reflection *This = impl_from_ID3D11ShaderReflection(iface);

    TRACE("iface %p, index %u\n", iface, index);

    if (index >= This->constant_buffer_count)
    {
        WARN("Invalid argument specified\n");
        return &null_constant_buffer.ID3D11ShaderReflectionConstantBuffer_iface;
    }

    return &This->constant_buffers[index].ID3D11ShaderReflectionConstantBuffer_iface;
}

static struct ID3D11ShaderReflectionConstantBuffer * STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetConstantBufferByName(
        ID3D11ShaderReflection *iface, const char *name)
{
    struct d3dcompiler_shader_reflection *This = impl_from_ID3D11ShaderReflection(iface);
    unsigned int i;

    TRACE("iface %p, name %s\n", iface, debugstr_a(name));

    if (!name)
    {
        WARN("Invalid argument specified\n");
        return &null_constant_buffer.ID3D11ShaderReflectionConstantBuffer_iface;
    }

    for (i = 0; i < This->constant_buffer_count; ++i)
    {
        struct d3dcompiler_shader_reflection_constant_buffer *d = &This->constant_buffers[i];

        if (!strcmp(d->name, name))
        {
            TRACE("Returning ID3D11ShaderReflectionConstantBuffer %p.\n", d);
            return &d->ID3D11ShaderReflectionConstantBuffer_iface;
        }
    }

    WARN("Invalid name specified\n");

    return &null_constant_buffer.ID3D11ShaderReflectionConstantBuffer_iface;
}

static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetResourceBindingDesc(
        ID3D11ShaderReflection *iface, UINT index, D3D11_SHADER_INPUT_BIND_DESC *desc)
{
    struct d3dcompiler_shader_reflection *This = impl_from_ID3D11ShaderReflection(iface);

    TRACE("iface %p, index %u, desc %p\n", iface, index, desc);

    if (!desc || index >= This->bound_resource_count)
    {
        WARN("Invalid argument specified\n");
        return E_INVALIDARG;
    }

    *desc = This->bound_resources[index];

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetInputParameterDesc(
        ID3D11ShaderReflection *iface, UINT index, D3D11_SIGNATURE_PARAMETER_DESC *desc)
{
    struct d3dcompiler_shader_reflection *This = impl_from_ID3D11ShaderReflection(iface);

    TRACE("iface %p, index %u, desc %p\n", iface, index, desc);

    if (!desc || !This->isgn || index >= This->isgn->element_count)
    {
        WARN("Invalid argument specified\n");
        return E_INVALIDARG;
    }

    *desc = This->isgn->elements[index];

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetOutputParameterDesc(
        ID3D11ShaderReflection *iface, UINT index, D3D11_SIGNATURE_PARAMETER_DESC *desc)
{
    struct d3dcompiler_shader_reflection *This = impl_from_ID3D11ShaderReflection(iface);

    TRACE("iface %p, index %u, desc %p\n", iface, index, desc);

    if (!desc || !This->osgn || index >= This->osgn->element_count)
    {
        WARN("Invalid argument specified\n");
        return E_INVALIDARG;
    }

    *desc = This->osgn->elements[index];

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetPatchConstantParameterDesc(
        ID3D11ShaderReflection *iface, UINT index, D3D11_SIGNATURE_PARAMETER_DESC *desc)
{
    struct d3dcompiler_shader_reflection *This = impl_from_ID3D11ShaderReflection(iface);

    TRACE("iface %p, index %u, desc %p\n", iface, index, desc);

    if (!desc || !This->pcsg || index >= This->pcsg->element_count)
    {
        WARN("Invalid argument specified\n");
        return E_INVALIDARG;
    }

    *desc = This->pcsg->elements[index];

    return S_OK;
}

static struct ID3D11ShaderReflectionVariable * STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetVariableByName(
        ID3D11ShaderReflection *iface, const char *name)
{
    struct d3dcompiler_shader_reflection *This = impl_from_ID3D11ShaderReflection(iface);
    unsigned int i, k;

    TRACE("iface %p, name %s\n", iface, debugstr_a(name));

    if (!name)
    {
        WARN("Invalid name specified\n");
        return &null_variable.ID3D11ShaderReflectionVariable_iface;
    }

    for (i = 0; i < This->constant_buffer_count; ++i)
    {
        struct d3dcompiler_shader_reflection_constant_buffer *cb = &This->constant_buffers[i];

        for (k = 0; k < cb->variable_count; ++k)
        {
            struct d3dcompiler_shader_reflection_variable *v = &cb->variables[k];

            if (!strcmp(v->name, name))
            {
                TRACE("Returning ID3D11ShaderReflectionVariable %p.\n", v);
                return &v->ID3D11ShaderReflectionVariable_iface;
            }
        }
    }

    WARN("Invalid name specified\n");

    return &null_variable.ID3D11ShaderReflectionVariable_iface;
}

static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetResourceBindingDescByName(
        ID3D11ShaderReflection *iface, const char *name, D3D11_SHADER_INPUT_BIND_DESC *desc)
{
    struct d3dcompiler_shader_reflection *This = impl_from_ID3D11ShaderReflection(iface);
    unsigned int i;

    TRACE("iface %p, name %s, desc %p\n", iface, debugstr_a(name), desc);

    if (!desc || !name)
    {
        WARN("Invalid argument specified\n");
        return E_INVALIDARG;
    }

    for (i = 0; i < This->bound_resource_count; ++i)
    {
        D3D11_SHADER_INPUT_BIND_DESC *d = &This->bound_resources[i];

        if (!strcmp(d->Name, name))
        {
            TRACE("Returning D3D11_SHADER_INPUT_BIND_DESC %p.\n", d);
            *desc = *d;
            return S_OK;
        }
    }

    WARN("Invalid name specified\n");

    return E_INVALIDARG;
}

static UINT STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetMovInstructionCount(
        ID3D11ShaderReflection *iface)
{
    struct d3dcompiler_shader_reflection *This = impl_from_ID3D11ShaderReflection(iface);

    TRACE("iface %p\n", iface);

    return This->mov_instruction_count;
}

static UINT STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetMovcInstructionCount(
        ID3D11ShaderReflection *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static UINT STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetConversionInstructionCount(
        ID3D11ShaderReflection *iface)
{
    struct d3dcompiler_shader_reflection *This = impl_from_ID3D11ShaderReflection(iface);

    TRACE("iface %p\n", iface);

    return This->conversion_instruction_count;
}

static UINT STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetBitwiseInstructionCount(
        ID3D11ShaderReflection *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static D3D_PRIMITIVE STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetGSInputPrimitive(
        ID3D11ShaderReflection *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static BOOL STDMETHODCALLTYPE d3dcompiler_shader_reflection_IsSampleFrequencyShader(
        ID3D11ShaderReflection *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static UINT STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetNumInterfaceSlots(
        ID3D11ShaderReflection *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetMinFeatureLevel(
        ID3D11ShaderReflection *iface, D3D_FEATURE_LEVEL *level)
{
    FIXME("iface %p, level %p stub!\n", iface, level);

    return E_NOTIMPL;
}

static UINT STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetThreadGroupSize(
        ID3D11ShaderReflection *iface, UINT *sizex, UINT *sizey, UINT *sizez)
{
    FIXME("iface %p, sizex %p, sizey %p, sizez %p stub!\n", iface, sizex, sizey, sizez);

    return 0;
}

static UINT64 STDMETHODCALLTYPE d3dcompiler_shader_reflection_GetRequiresFlags(
        ID3D11ShaderReflection *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static const struct ID3D11ShaderReflectionVtbl d3dcompiler_shader_reflection_vtbl =
{
    /* IUnknown methods */
    d3dcompiler_shader_reflection_QueryInterface,
    d3dcompiler_shader_reflection_AddRef,
    d3dcompiler_shader_reflection_Release,
    /* ID3D11ShaderReflection methods */
    d3dcompiler_shader_reflection_GetDesc,
    d3dcompiler_shader_reflection_GetConstantBufferByIndex,
    d3dcompiler_shader_reflection_GetConstantBufferByName,
    d3dcompiler_shader_reflection_GetResourceBindingDesc,
    d3dcompiler_shader_reflection_GetInputParameterDesc,
    d3dcompiler_shader_reflection_GetOutputParameterDesc,
    d3dcompiler_shader_reflection_GetPatchConstantParameterDesc,
    d3dcompiler_shader_reflection_GetVariableByName,
    d3dcompiler_shader_reflection_GetResourceBindingDescByName,
    d3dcompiler_shader_reflection_GetMovInstructionCount,
    d3dcompiler_shader_reflection_GetMovcInstructionCount,
    d3dcompiler_shader_reflection_GetConversionInstructionCount,
    d3dcompiler_shader_reflection_GetBitwiseInstructionCount,
    d3dcompiler_shader_reflection_GetGSInputPrimitive,
    d3dcompiler_shader_reflection_IsSampleFrequencyShader,
    d3dcompiler_shader_reflection_GetNumInterfaceSlots,
    d3dcompiler_shader_reflection_GetMinFeatureLevel,
    d3dcompiler_shader_reflection_GetThreadGroupSize,
    d3dcompiler_shader_reflection_GetRequiresFlags,
};

/* ID3D11ShaderReflectionConstantBuffer methods */

static inline struct d3dcompiler_shader_reflection_constant_buffer *impl_from_ID3D11ShaderReflectionConstantBuffer(ID3D11ShaderReflectionConstantBuffer *iface)
{
    return CONTAINING_RECORD(iface, struct d3dcompiler_shader_reflection_constant_buffer, ID3D11ShaderReflectionConstantBuffer_iface);
}

static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_constant_buffer_GetDesc(
        ID3D11ShaderReflectionConstantBuffer *iface, D3D11_SHADER_BUFFER_DESC *desc)
{
    struct d3dcompiler_shader_reflection_constant_buffer *This = impl_from_ID3D11ShaderReflectionConstantBuffer(iface);

    TRACE("iface %p, desc %p\n", iface, desc);

    if (This == &null_constant_buffer)
    {
        WARN("Null constant buffer specified\n");
        return E_FAIL;
    }

    if (!desc)
    {
        WARN("Invalid argument specified\n");
        return E_FAIL;
    }

    desc->Name = This->name;
    desc->Type = This->type;
    desc->Variables = This->variable_count;
    desc->Size = This->size;
    desc->uFlags = This->flags;

    return S_OK;
}

static ID3D11ShaderReflectionVariable * STDMETHODCALLTYPE d3dcompiler_shader_reflection_constant_buffer_GetVariableByIndex(
        ID3D11ShaderReflectionConstantBuffer *iface, UINT index)
{
    struct d3dcompiler_shader_reflection_constant_buffer *This = impl_from_ID3D11ShaderReflectionConstantBuffer(iface);

    TRACE("iface %p, index %u\n", iface, index);

    if (index >= This->variable_count)
    {
        WARN("Invalid index specified\n");
        return &null_variable.ID3D11ShaderReflectionVariable_iface;
    }

    return &This->variables[index].ID3D11ShaderReflectionVariable_iface;
}

static ID3D11ShaderReflectionVariable * STDMETHODCALLTYPE d3dcompiler_shader_reflection_constant_buffer_GetVariableByName(
        ID3D11ShaderReflectionConstantBuffer *iface, const char *name)
{
    struct d3dcompiler_shader_reflection_constant_buffer *This = impl_from_ID3D11ShaderReflectionConstantBuffer(iface);
    unsigned int i;

    TRACE("iface %p, name %s\n", iface, debugstr_a(name));

    if (!name)
    {
        WARN("Invalid argument specified\n");
        return &null_variable.ID3D11ShaderReflectionVariable_iface;
    }

    for (i = 0; i < This->variable_count; ++i)
    {
        struct d3dcompiler_shader_reflection_variable *v = &This->variables[i];

        if (!strcmp(v->name, name))
        {
            TRACE("Returning ID3D11ShaderReflectionVariable %p.\n", v);
            return &v->ID3D11ShaderReflectionVariable_iface;
        }
    }

    WARN("Invalid name specified\n");

    return &null_variable.ID3D11ShaderReflectionVariable_iface;
}

static const struct ID3D11ShaderReflectionConstantBufferVtbl d3dcompiler_shader_reflection_constant_buffer_vtbl =
{
    /* ID3D11ShaderReflectionConstantBuffer methods */
    d3dcompiler_shader_reflection_constant_buffer_GetDesc,
    d3dcompiler_shader_reflection_constant_buffer_GetVariableByIndex,
    d3dcompiler_shader_reflection_constant_buffer_GetVariableByName,
};

/* ID3D11ShaderReflectionVariable methods */

static inline struct d3dcompiler_shader_reflection_variable *impl_from_ID3D11ShaderReflectionVariable(ID3D11ShaderReflectionVariable *iface)
{
    return CONTAINING_RECORD(iface, struct d3dcompiler_shader_reflection_variable, ID3D11ShaderReflectionVariable_iface);
}

static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_variable_GetDesc(
        ID3D11ShaderReflectionVariable *iface, D3D11_SHADER_VARIABLE_DESC *desc)
{
    struct d3dcompiler_shader_reflection_variable *This = impl_from_ID3D11ShaderReflectionVariable(iface);

    TRACE("iface %p, desc %p\n", iface, desc);

    if (This == &null_variable)
    {
        WARN("Null variable specified\n");
        return E_FAIL;
    }

    if (!desc)
    {
        WARN("Invalid argument specified\n");
        return E_FAIL;
    }

    desc->Name = This->name;
    desc->StartOffset = This->start_offset;
    desc->Size = This->size;
    desc->uFlags = This->flags;
    desc->DefaultValue = This->default_value;

    return S_OK;
}

static ID3D11ShaderReflectionType * STDMETHODCALLTYPE d3dcompiler_shader_reflection_variable_GetType(
        ID3D11ShaderReflectionVariable *iface)
{
    struct d3dcompiler_shader_reflection_variable *This = impl_from_ID3D11ShaderReflectionVariable(iface);

    TRACE("iface %p\n", iface);

    return &This->type->ID3D11ShaderReflectionType_iface;
}

static ID3D11ShaderReflectionConstantBuffer * STDMETHODCALLTYPE d3dcompiler_shader_reflection_variable_GetBuffer(
        ID3D11ShaderReflectionVariable *iface)
{
    struct d3dcompiler_shader_reflection_variable *This = impl_from_ID3D11ShaderReflectionVariable(iface);

    TRACE("iface %p\n", iface);

    return &This->constant_buffer->ID3D11ShaderReflectionConstantBuffer_iface;
}

static UINT STDMETHODCALLTYPE d3dcompiler_shader_reflection_variable_GetInterfaceSlot(
        ID3D11ShaderReflectionVariable *iface, UINT index)
{
    FIXME("iface %p, index %u stub!\n", iface, index);

    return 0;
}

static const struct ID3D11ShaderReflectionVariableVtbl d3dcompiler_shader_reflection_variable_vtbl =
{
    /* ID3D11ShaderReflectionVariable methods */
    d3dcompiler_shader_reflection_variable_GetDesc,
    d3dcompiler_shader_reflection_variable_GetType,
    d3dcompiler_shader_reflection_variable_GetBuffer,
    d3dcompiler_shader_reflection_variable_GetInterfaceSlot,
};

/* ID3D11ShaderReflectionType methods */

static inline struct d3dcompiler_shader_reflection_type *impl_from_ID3D11ShaderReflectionType(ID3D11ShaderReflectionType *iface)
{
    return CONTAINING_RECORD(iface, struct d3dcompiler_shader_reflection_type, ID3D11ShaderReflectionType_iface);
}

static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_type_GetDesc(
        ID3D11ShaderReflectionType *iface, D3D11_SHADER_TYPE_DESC *desc)
{
    struct d3dcompiler_shader_reflection_type *This = impl_from_ID3D11ShaderReflectionType(iface);

    TRACE("iface %p, desc %p\n", iface, desc);

    if (This == &null_type)
    {
        WARN("Null type specified\n");
        return E_FAIL;
    }

    if (!desc)
    {
        WARN("Invalid argument specified\n");
        return E_FAIL;
    }

    *desc = This->desc;

    return S_OK;
}

static ID3D11ShaderReflectionType * STDMETHODCALLTYPE d3dcompiler_shader_reflection_type_GetMemberTypeByIndex(
        ID3D11ShaderReflectionType *iface, UINT index)
{
    struct d3dcompiler_shader_reflection_type *This = impl_from_ID3D11ShaderReflectionType(iface);

    TRACE("iface %p, index %u\n", iface, index);

    if (index >= This->desc.Members)
    {
        WARN("Invalid index specified\n");
        return &null_type.ID3D11ShaderReflectionType_iface;
    }

    return &This->members[index].type->ID3D11ShaderReflectionType_iface;
}

static ID3D11ShaderReflectionType * STDMETHODCALLTYPE d3dcompiler_shader_reflection_type_GetMemberTypeByName(
        ID3D11ShaderReflectionType *iface, const char *name)
{
    struct d3dcompiler_shader_reflection_type *This = impl_from_ID3D11ShaderReflectionType(iface);
    unsigned int i;

    TRACE("iface %p, name %s\n", iface, debugstr_a(name));

    if (!name)
    {
        WARN("Invalid argument specified\n");
        return &null_type.ID3D11ShaderReflectionType_iface;
    }

    for (i = 0; i < This->desc.Members; ++i)
    {
        struct d3dcompiler_shader_reflection_type_member *member = &This->members[i];

        if (!strcmp(member->name, name))
        {
            TRACE("Returning ID3D11ShaderReflectionType %p.\n", member->type);
            return &member->type->ID3D11ShaderReflectionType_iface;
        }
    }

    WARN("Invalid name specified\n");

    return &null_type.ID3D11ShaderReflectionType_iface;
}

static const char * STDMETHODCALLTYPE d3dcompiler_shader_reflection_type_GetMemberTypeName(
        ID3D11ShaderReflectionType *iface, UINT index)
{
    struct d3dcompiler_shader_reflection_type *This = impl_from_ID3D11ShaderReflectionType(iface);

    TRACE("iface %p, index %u\n", iface, index);

    if (This == &null_type)
    {
        WARN("Null type specified\n");
        return "$Invalid";
    }

    if (index >= This->desc.Members)
    {
        WARN("Invalid index specified\n");
        return NULL;
    }

    return This->members[index].name;
}

static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_type_IsEqual(
        ID3D11ShaderReflectionType *iface, ID3D11ShaderReflectionType *type)
{
    struct d3dcompiler_shader_reflection_type *This = impl_from_ID3D11ShaderReflectionType(iface);

    TRACE("iface %p, type %p\n", iface, type);

    if (This == &null_type)
    {
        WARN("Null type specified\n");
        return E_FAIL;
    }

    if (iface == type)
        return S_OK;

    return S_FALSE;
}

static ID3D11ShaderReflectionType * STDMETHODCALLTYPE d3dcompiler_shader_reflection_type_GetSubType(
        ID3D11ShaderReflectionType *iface)
{
    FIXME("iface %p stub!\n", iface);

    return NULL;
}

static ID3D11ShaderReflectionType * STDMETHODCALLTYPE d3dcompiler_shader_reflection_type_GetBaseClass(
        ID3D11ShaderReflectionType *iface)
{
    FIXME("iface %p stub!\n", iface);

    return NULL;
}

static UINT STDMETHODCALLTYPE d3dcompiler_shader_reflection_type_GetNumInterfaces(
        ID3D11ShaderReflectionType *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static ID3D11ShaderReflectionType * STDMETHODCALLTYPE d3dcompiler_shader_reflection_type_GetInterfaceByIndex(
        ID3D11ShaderReflectionType *iface, UINT index)
{
    FIXME("iface %p, index %u stub!\n", iface, index);

    return NULL;
}

static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_type_IsOfType(
        ID3D11ShaderReflectionType *iface, ID3D11ShaderReflectionType *type)
{
    FIXME("iface %p, type %p stub!\n", iface, type);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3dcompiler_shader_reflection_type_ImplementsInterface(
        ID3D11ShaderReflectionType *iface, ID3D11ShaderReflectionType *base)
{
    FIXME("iface %p, base %p stub!\n", iface, base);

    return E_NOTIMPL;
}

static const struct ID3D11ShaderReflectionTypeVtbl d3dcompiler_shader_reflection_type_vtbl =
{
    /* ID3D11ShaderReflectionType methods */
    d3dcompiler_shader_reflection_type_GetDesc,
    d3dcompiler_shader_reflection_type_GetMemberTypeByIndex,
    d3dcompiler_shader_reflection_type_GetMemberTypeByName,
    d3dcompiler_shader_reflection_type_GetMemberTypeName,
    d3dcompiler_shader_reflection_type_IsEqual,
    d3dcompiler_shader_reflection_type_GetSubType,
    d3dcompiler_shader_reflection_type_GetBaseClass,
    d3dcompiler_shader_reflection_type_GetNumInterfaces,
    d3dcompiler_shader_reflection_type_GetInterfaceByIndex,
    d3dcompiler_shader_reflection_type_IsOfType,
    d3dcompiler_shader_reflection_type_ImplementsInterface,
};

static HRESULT d3dcompiler_parse_stat(struct d3dcompiler_shader_reflection *r, const char *data, DWORD data_size)
{
    const char *ptr = data;
    DWORD size = data_size >> 2;

    TRACE("Size %u\n", size);

    read_dword(&ptr, &r->instruction_count);
    TRACE("InstructionCount: %u\n", r->instruction_count);

    read_dword(&ptr, &r->temp_register_count);
    TRACE("TempRegisterCount: %u\n", r->temp_register_count);

    skip_dword_unknown(&ptr, 1);

    read_dword(&ptr, &r->dcl_count);
    TRACE("DclCount: %u\n", r->dcl_count);

    read_dword(&ptr, &r->float_instruction_count);
    TRACE("FloatInstructionCount: %u\n", r->float_instruction_count);

    read_dword(&ptr, &r->int_instruction_count);
    TRACE("IntInstructionCount: %u\n", r->int_instruction_count);

    read_dword(&ptr, &r->uint_instruction_count);
    TRACE("UintInstructionCount: %u\n", r->uint_instruction_count);

    read_dword(&ptr, &r->static_flow_control_count);
    TRACE("StaticFlowControlCount: %u\n", r->static_flow_control_count);

    read_dword(&ptr, &r->dynamic_flow_control_count);
    TRACE("DynamicFlowControlCount: %u\n", r->dynamic_flow_control_count);

    skip_dword_unknown(&ptr, 1);

    read_dword(&ptr, &r->temp_array_count);
    TRACE("TempArrayCount: %u\n", r->temp_array_count);

    read_dword(&ptr, &r->array_instruction_count);
    TRACE("ArrayInstructionCount: %u\n", r->array_instruction_count);

    read_dword(&ptr, &r->cut_instruction_count);
    TRACE("CutInstructionCount: %u\n", r->cut_instruction_count);

    read_dword(&ptr, &r->emit_instruction_count);
    TRACE("EmitInstructionCount: %u\n", r->emit_instruction_count);

    read_dword(&ptr, &r->texture_normal_instructions);
    TRACE("TextureNormalInstructions: %u\n", r->texture_normal_instructions);

    read_dword(&ptr, &r->texture_load_instructions);
    TRACE("TextureLoadInstructions: %u\n", r->texture_load_instructions);

    read_dword(&ptr, &r->texture_comp_instructions);
    TRACE("TextureCompInstructions: %u\n", r->texture_comp_instructions);

    read_dword(&ptr, &r->texture_bias_instructions);
    TRACE("TextureBiasInstructions: %u\n", r->texture_bias_instructions);

    read_dword(&ptr, &r->texture_gradient_instructions);
    TRACE("TextureGradientInstructions: %u\n", r->texture_gradient_instructions);

    read_dword(&ptr, &r->mov_instruction_count);
    TRACE("MovInstructionCount: %u\n", r->mov_instruction_count);

    skip_dword_unknown(&ptr, 1);

    read_dword(&ptr, &r->conversion_instruction_count);
    TRACE("ConversionInstructionCount: %u\n", r->conversion_instruction_count);

    skip_dword_unknown(&ptr, 1);

#ifdef __REACTOS__ /* DWORD* cast added */
    read_dword(&ptr, (DWORD*)&r->input_primitive);
#else
    read_dword(&ptr, &r->input_primitive);
#endif
    TRACE("InputPrimitive: %x\n", r->input_primitive);

#ifdef __REACTOS__ /* DWORD* cast added */
    read_dword(&ptr, (DWORD*)&r->gs_output_topology);
#else
    read_dword(&ptr, &r->gs_output_topology);
#endif
    TRACE("GSOutputTopology: %x\n", r->gs_output_topology);

    read_dword(&ptr, &r->gs_max_output_vertex_count);
    TRACE("GSMaxOutputVertexCount: %u\n", r->gs_max_output_vertex_count);

    skip_dword_unknown(&ptr, 2);

    /* old dx10 stat size */
    if (size == 28) return S_OK;

    skip_dword_unknown(&ptr, 1);

    /* dx10 stat size */
    if (size == 29) return S_OK;

    skip_dword_unknown(&ptr, 1);

    read_dword(&ptr, &r->c_control_points);
    TRACE("cControlPoints: %u\n", r->c_control_points);

#ifdef __REACTOS__ /* DWORD* cast added */
    read_dword(&ptr, (DWORD*)&r->hs_output_primitive);
#else
    read_dword(&ptr, &r->hs_output_primitive);
#endif
    TRACE("HSOutputPrimitive: %x\n", r->hs_output_primitive);

#ifdef __REACTOS__ /* DWORD* cast added */
    read_dword(&ptr, (DWORD*)&r->hs_prtitioning);
#else
    read_dword(&ptr, &r->hs_prtitioning);
#endif
    TRACE("HSPartitioning: %x\n", r->hs_prtitioning);

#ifdef __REACTOS__ /* DWORD* cast added */
    read_dword(&ptr, (DWORD*)&r->tessellator_domain);
#else
    read_dword(&ptr, &r->tessellator_domain);
#endif
    TRACE("TessellatorDomain: %x\n", r->tessellator_domain);

    skip_dword_unknown(&ptr, 3);

    /* dx11 stat size */
    if (size == 37) return S_OK;

    FIXME("Unhandled size %u\n", size);

    return E_FAIL;
}

static HRESULT d3dcompiler_parse_type_members(struct d3dcompiler_shader_reflection *ref,
        struct d3dcompiler_shader_reflection_type_member *member, const char *data, const char **ptr)
{
    DWORD offset;

    read_dword(ptr, &offset);
    if (!copy_name(data + offset, &member->name))
    {
        ERR("Failed to copy name.\n");
        return E_OUTOFMEMORY;
    }
    TRACE("Member name: %s.\n", debugstr_a(member->name));

    read_dword(ptr, &offset);
    TRACE("Member type offset: %x\n", offset);

    member->type = get_reflection_type(ref, data, offset);
    if (!member->type)
    {
        ERR("Failed to get member type\n");
        HeapFree(GetProcessHeap(), 0, member->name);
        return E_FAIL;
    }

    read_dword(ptr, &member->offset);
    TRACE("Member offset %x\n", member->offset);

    return S_OK;
}

static HRESULT d3dcompiler_parse_type(struct d3dcompiler_shader_reflection_type *type, const char *data, DWORD offset)
{
    const char *ptr = data + offset;
    DWORD temp;
    D3D11_SHADER_TYPE_DESC *desc;
    unsigned int i;
    struct d3dcompiler_shader_reflection_type_member *members = NULL;
    HRESULT hr;
    DWORD member_offset;

    desc = &type->desc;

    read_dword(&ptr, &temp);
    desc->Class = temp & 0xffff;
    desc->Type = temp >> 16;
    TRACE("Class %s, Type %s\n", debug_d3dcompiler_shader_variable_class(desc->Class),
            debug_d3dcompiler_shader_variable_type(desc->Type));

    read_dword(&ptr, &temp);
    desc->Rows = temp & 0xffff;
    desc->Columns = temp >> 16;
    TRACE("Rows %u, Columns %u\n", desc->Rows, desc->Columns);

    read_dword(&ptr, &temp);
    desc->Elements = temp & 0xffff;
    desc->Members = temp >> 16;
    TRACE("Elements %u, Members %u\n", desc->Elements, desc->Members);

    read_dword(&ptr, &member_offset);
    TRACE("Member Offset %u\n", member_offset);

    if ((type->reflection->target & D3DCOMPILER_SHADER_TARGET_VERSION_MASK) >= 0x500)
        skip_dword_unknown(&ptr, 4);

    if (desc->Members)
    {
        const char *ptr2 = data + member_offset;

        members = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*members) * desc->Members);
        if (!members)
        {
            ERR("Failed to allocate type memory.\n");
            return E_OUTOFMEMORY;
        }

        for (i = 0; i < desc->Members; ++i)
        {
            hr = d3dcompiler_parse_type_members(type->reflection, &members[i], data, &ptr2);
            if (hr != S_OK)
            {
                FIXME("Failed to parse type members.\n");
                goto err_out;
            }
        }
    }

    if ((type->reflection->target & D3DCOMPILER_SHADER_TARGET_VERSION_MASK) >= 0x500)
    {
        read_dword(&ptr, &offset);
        if (!copy_name(data + offset, &type->name))
        {
            ERR("Failed to copy name.\n");
            heap_free(members);
            return E_OUTOFMEMORY;
        }
        desc->Name = type->name;
        TRACE("Type name: %s.\n", debugstr_a(type->name));
    }

    type->members = members;

    return S_OK;

err_out:
    for (i = 0; i < desc->Members; ++i)
    {
        free_type_member(&members[i]);
    }
    HeapFree(GetProcessHeap(), 0, members);
    return hr;
}

static struct d3dcompiler_shader_reflection_type *get_reflection_type(struct d3dcompiler_shader_reflection *reflection, const char *data, DWORD offset)
{
    struct d3dcompiler_shader_reflection_type *type;
    struct wine_rb_entry *entry;
    HRESULT hr;

    entry = wine_rb_get(&reflection->types, &offset);
    if (entry)
    {
        TRACE("Returning existing type.\n");
        return WINE_RB_ENTRY_VALUE(entry, struct d3dcompiler_shader_reflection_type, entry);
    }

    type = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*type));
    if (!type)
        return NULL;

    type->ID3D11ShaderReflectionType_iface.lpVtbl = &d3dcompiler_shader_reflection_type_vtbl;
    type->id = offset;
    type->reflection = reflection;

    hr = d3dcompiler_parse_type(type, data, offset);
    if (FAILED(hr))
    {
        ERR("Failed to parse type info, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, type);
        return NULL;
    }

    if (wine_rb_put(&reflection->types, &offset, &type->entry) == -1)
    {
        ERR("Failed to insert type entry.\n");
        HeapFree(GetProcessHeap(), 0, type);
        return NULL;
    }

    return type;
}

static HRESULT d3dcompiler_parse_variables(struct d3dcompiler_shader_reflection_constant_buffer *cb,
        const char *data, DWORD data_size, const char *ptr)
{
    struct d3dcompiler_shader_reflection_variable *variables;
    unsigned int i;
    HRESULT hr;

    variables = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb->variable_count * sizeof(*variables));
    if (!variables)
    {
        ERR("Failed to allocate variables memory.\n");
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < cb->variable_count; i++)
    {
        struct d3dcompiler_shader_reflection_variable *v = &variables[i];
        DWORD offset;

        v->ID3D11ShaderReflectionVariable_iface.lpVtbl = &d3dcompiler_shader_reflection_variable_vtbl;
        v->constant_buffer = cb;

        read_dword(&ptr, &offset);
        if (!copy_name(data + offset, &v->name))
        {
            ERR("Failed to copy name.\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }
        TRACE("Variable name: %s.\n", debugstr_a(v->name));

        read_dword(&ptr, &v->start_offset);
        TRACE("Variable offset: %u\n", v->start_offset);

        read_dword(&ptr, &v->size);
        TRACE("Variable size: %u\n", v->size);

        read_dword(&ptr, &v->flags);
        TRACE("Variable flags: %u\n", v->flags);

        read_dword(&ptr, &offset);
        TRACE("Variable type offset: %x\n", offset);
        v->type = get_reflection_type(cb->reflection, data, offset);
        if (!v->type)
        {
            ERR("Failed to get type.\n");
            hr = E_FAIL;
            goto err_out;
        }

        read_dword(&ptr, &offset);
        TRACE("Variable default value offset: %x\n", offset);
        if (!copy_value(data + offset, &v->default_value, offset ? v->size : 0))
        {
            ERR("Failed to copy name.\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }

        if ((cb->reflection->target & D3DCOMPILER_SHADER_TARGET_VERSION_MASK) >= 0x500)
            skip_dword_unknown(&ptr, 4);
    }

    cb->variables = variables;

    return S_OK;

err_out:
    for (i = 0; i < cb->variable_count; i++)
    {
        free_variable(&variables[i]);
    }
    HeapFree(GetProcessHeap(), 0, variables);
    return hr;
}

static HRESULT d3dcompiler_parse_rdef(struct d3dcompiler_shader_reflection *r, const char *data, DWORD data_size)
{
    const char *ptr = data;
    DWORD size = data_size >> 2;
    DWORD offset, cbuffer_offset, resource_offset, creator_offset;
    unsigned int i, string_data_offset, string_data_size;
    char *string_data = NULL, *creator = NULL;
    D3D11_SHADER_INPUT_BIND_DESC *bound_resources = NULL;
    struct d3dcompiler_shader_reflection_constant_buffer *constant_buffers = NULL;
    HRESULT hr;

    TRACE("Size %u\n", size);

    read_dword(&ptr, &r->constant_buffer_count);
    TRACE("Constant buffer count: %u\n", r->constant_buffer_count);

    read_dword(&ptr, &cbuffer_offset);
    TRACE("Constant buffer offset: %#x\n", cbuffer_offset);

    read_dword(&ptr, &r->bound_resource_count);
    TRACE("Bound resource count: %u\n", r->bound_resource_count);

    read_dword(&ptr, &resource_offset);
    TRACE("Bound resource offset: %#x\n", resource_offset);

    read_dword(&ptr, &r->target);
    TRACE("Target: %#x\n", r->target);

    read_dword(&ptr, &r->flags);
    TRACE("Flags: %u\n", r->flags);

    read_dword(&ptr, &creator_offset);
    TRACE("Creator at offset %#x.\n", creator_offset);

    if (!copy_name(data + creator_offset, &creator))
    {
        ERR("Failed to copy name.\n");
        return E_OUTOFMEMORY;
    }
    TRACE("Creator: %s.\n", debugstr_a(creator));

    /* todo: Parse RD11 */
    if ((r->target & D3DCOMPILER_SHADER_TARGET_VERSION_MASK) >= 0x500)
    {
        skip_dword_unknown(&ptr, 8);
    }

    if (r->bound_resource_count)
    {
        /* 8 for each bind desc */
        string_data_offset = resource_offset + r->bound_resource_count * 8 * sizeof(DWORD);
        string_data_size = (cbuffer_offset ? cbuffer_offset : creator_offset) - string_data_offset;

        string_data = HeapAlloc(GetProcessHeap(), 0, string_data_size);
        if (!string_data)
        {
            ERR("Failed to allocate string data memory.\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }
        memcpy(string_data, data + string_data_offset, string_data_size);

        bound_resources = HeapAlloc(GetProcessHeap(), 0, r->bound_resource_count * sizeof(*bound_resources));
        if (!bound_resources)
        {
            ERR("Failed to allocate resources memory.\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }

        ptr = data + resource_offset;
        for (i = 0; i < r->bound_resource_count; i++)
        {
            D3D11_SHADER_INPUT_BIND_DESC *desc = &bound_resources[i];

            read_dword(&ptr, &offset);
            desc->Name = string_data + (offset - string_data_offset);
            TRACE("Input bind Name: %s\n", debugstr_a(desc->Name));

#ifdef __REACTOS__ /* DWORD* cast added */
            read_dword(&ptr, (DWORD*)&desc->Type);
#else
            read_dword(&ptr, &desc->Type);
#endif
            TRACE("Input bind Type: %#x\n", desc->Type);

#ifdef __REACTOS__ /* DWORD* cast added */
            read_dword(&ptr, (DWORD*)&desc->ReturnType);
#else
            read_dword(&ptr, &desc->ReturnType);
#endif
            TRACE("Input bind ReturnType: %#x\n", desc->ReturnType);

#ifdef __REACTOS__ /* DWORD* cast added */
            read_dword(&ptr, (DWORD*)&desc->Dimension);
#else
            read_dword(&ptr, &desc->Dimension);
#endif
            TRACE("Input bind Dimension: %#x\n", desc->Dimension);

            read_dword(&ptr, &desc->NumSamples);
            TRACE("Input bind NumSamples: %u\n", desc->NumSamples);

            read_dword(&ptr, &desc->BindPoint);
            TRACE("Input bind BindPoint: %u\n", desc->BindPoint);

            read_dword(&ptr, &desc->BindCount);
            TRACE("Input bind BindCount: %u\n", desc->BindCount);

            read_dword(&ptr, &desc->uFlags);
            TRACE("Input bind uFlags: %u\n", desc->uFlags);
        }
    }

    if (r->constant_buffer_count)
    {
        constant_buffers = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, r->constant_buffer_count * sizeof(*constant_buffers));
        if (!constant_buffers)
        {
            ERR("Failed to allocate constant buffer memory.\n");
            hr = E_OUTOFMEMORY;
            goto err_out;
        }

        ptr = data + cbuffer_offset;
        for (i = 0; i < r->constant_buffer_count; i++)
        {
            struct d3dcompiler_shader_reflection_constant_buffer *cb = &constant_buffers[i];

            cb->ID3D11ShaderReflectionConstantBuffer_iface.lpVtbl = &d3dcompiler_shader_reflection_constant_buffer_vtbl;
            cb->reflection = r;

            read_dword(&ptr, &offset);
            if (!copy_name(data + offset, &cb->name))
            {
                ERR("Failed to copy name.\n");
                hr = E_OUTOFMEMORY;
                goto err_out;
            }
            TRACE("Name: %s.\n", debugstr_a(cb->name));

            read_dword(&ptr, &cb->variable_count);
            TRACE("Variable count: %u\n", cb->variable_count);

            read_dword(&ptr, &offset);
            TRACE("Variable offset: %x\n", offset);

            hr = d3dcompiler_parse_variables(cb, data, data_size, data + offset);
            if (hr != S_OK)
            {
                FIXME("Failed to parse variables.\n");
                goto err_out;
            }

            read_dword(&ptr, &cb->size);
            TRACE("Cbuffer size: %u\n", cb->size);

            read_dword(&ptr, &cb->flags);
            TRACE("Cbuffer flags: %u\n", cb->flags);

#ifdef __REACTOS__ /* DWORD* cast added */
            read_dword(&ptr, (DWORD*)&cb->type);
#else
            read_dword(&ptr, &cb->type);
#endif
            TRACE("Cbuffer type: %#x\n", cb->type);
        }
    }

    r->creator = creator;
    r->resource_string = string_data;
    r->bound_resources = bound_resources;
    r->constant_buffers = constant_buffers;

    return S_OK;

err_out:
    for (i = 0; i < r->constant_buffer_count; ++i)
    {
        free_constant_buffer(&constant_buffers[i]);
    }
    HeapFree(GetProcessHeap(), 0, constant_buffers);
    HeapFree(GetProcessHeap(), 0, bound_resources);
    HeapFree(GetProcessHeap(), 0, string_data);
    HeapFree(GetProcessHeap(), 0, creator);

    return hr;
}

static HRESULT d3dcompiler_parse_signature(struct d3dcompiler_shader_signature *s, struct dxbc_section *section, DWORD target)
{
    D3D11_SIGNATURE_PARAMETER_DESC *d;
    unsigned int string_data_offset;
    unsigned int string_data_size;
    const char *ptr = section->data;
    char *string_data;
    unsigned int i;
    DWORD count;
    enum D3DCOMPILER_SIGNATURE_ELEMENT_SIZE element_size;

    switch (section->tag)
    {
        case TAG_OSG5:
            element_size = D3DCOMPILER_SIGNATURE_ELEMENT_SIZE7;
            break;

        case TAG_ISGN:
        case TAG_OSGN:
        case TAG_PCSG:
            element_size = D3DCOMPILER_SIGNATURE_ELEMENT_SIZE6;
            break;

        default:
            FIXME("Unhandled section %s!\n", debugstr_an((const char *)&section->tag, 4));
            element_size = D3DCOMPILER_SIGNATURE_ELEMENT_SIZE6;
            break;
    }

    read_dword(&ptr, &count);
    TRACE("%u elements\n", count);

    skip_dword_unknown(&ptr, 1);

    d = HeapAlloc(GetProcessHeap(), 0, count * sizeof(*d));
    if (!d)
    {
        ERR("Failed to allocate signature memory.\n");
        return E_OUTOFMEMORY;
    }

    /* 2 DWORDs for the header, element_size for each element. */
    string_data_offset = 2 * sizeof(DWORD) + count * element_size * sizeof(DWORD);
    string_data_size = section->data_size - string_data_offset;

    string_data = HeapAlloc(GetProcessHeap(), 0, string_data_size);
    if (!string_data)
    {
        ERR("Failed to allocate string data memory.\n");
        HeapFree(GetProcessHeap(), 0, d);
        return E_OUTOFMEMORY;
    }
    memcpy(string_data, section->data + string_data_offset, string_data_size);

    for (i = 0; i < count; ++i)
    {
        UINT name_offset;
        DWORD mask;

        if (element_size == D3DCOMPILER_SIGNATURE_ELEMENT_SIZE7)
        {
            read_dword(&ptr, &d[i].Stream);
        }
        else
        {
            d[i].Stream = 0;
        }

        read_dword(&ptr, &name_offset);
        d[i].SemanticName = string_data + (name_offset - string_data_offset);
        read_dword(&ptr, &d[i].SemanticIndex);
#ifdef __REACTOS__ /* DWORD* casts added */
        read_dword(&ptr, (DWORD*)&d[i].SystemValueType);
        read_dword(&ptr, (DWORD*)&d[i].ComponentType);
#else
        read_dword(&ptr, &d[i].SystemValueType);
        read_dword(&ptr, &d[i].ComponentType);
#endif
        read_dword(&ptr, &d[i].Register);
        read_dword(&ptr, &mask);
        d[i].ReadWriteMask = (mask >> 8) & 0xff;
        d[i].Mask = mask & 0xff;

        /* pixel shaders have a special handling for SystemValueType in the output signature */
        if (((target & D3DCOMPILER_SHADER_TARGET_SHADERTYPE_MASK) == 0xffff0000) && (section->tag == TAG_OSG5 || section->tag == TAG_OSGN))
        {
            TRACE("Pixelshader output signature fixup.\n");

            if (d[i].Register == 0xffffffff)
            {
                if (!_strnicmp(d[i].SemanticName, "sv_depth", -1))
                    d[i].SystemValueType = D3D_NAME_DEPTH;
                else if (!_strnicmp(d[i].SemanticName, "sv_coverage", -1))
                    d[i].SystemValueType = D3D_NAME_COVERAGE;
                else if (!_strnicmp(d[i].SemanticName, "sv_depthgreaterequal", -1))
                    d[i].SystemValueType = D3D_NAME_DEPTH_GREATER_EQUAL;
                else if (!_strnicmp(d[i].SemanticName, "sv_depthlessequal", -1))
                    d[i].SystemValueType = D3D_NAME_DEPTH_LESS_EQUAL;
            }
            else
            {
                d[i].SystemValueType = D3D_NAME_TARGET;
            }
        }

        TRACE("semantic: %s, semantic idx: %u, sysval_semantic %#x, "
                "type %u, register idx: %u, use_mask %#x, input_mask %#x, stream %u\n",
                debugstr_a(d[i].SemanticName), d[i].SemanticIndex, d[i].SystemValueType,
                d[i].ComponentType, d[i].Register, d[i].Mask, d[i].ReadWriteMask, d[i].Stream);
    }

    s->elements = d;
    s->element_count = count;
    s->string_data = string_data;

    return S_OK;
}

static HRESULT d3dcompiler_parse_shdr(struct d3dcompiler_shader_reflection *r, const char *data, DWORD data_size)
{
    const char *ptr = data;

    read_dword(&ptr, &r->version);
    TRACE("Shader version: %u\n", r->version);

    /* todo: Check if anything else is needed from the shdr or shex blob. */

    return S_OK;
}

static HRESULT d3dcompiler_shader_reflection_init(struct d3dcompiler_shader_reflection *reflection,
        const void *data, SIZE_T data_size)
{
    struct dxbc src_dxbc;
    HRESULT hr;
    unsigned int i;

    reflection->ID3D11ShaderReflection_iface.lpVtbl = &d3dcompiler_shader_reflection_vtbl;
    reflection->refcount = 1;

    wine_rb_init(&reflection->types, d3dcompiler_shader_reflection_type_compare);

    hr = dxbc_parse(data, data_size, &src_dxbc);
    if (FAILED(hr))
    {
        WARN("Failed to parse reflection\n");
        return hr;
    }

    for (i = 0; i < src_dxbc.count; ++i)
    {
        struct dxbc_section *section = &src_dxbc.sections[i];

        switch (section->tag)
        {
            case TAG_RDEF:
                hr = d3dcompiler_parse_rdef(reflection, section->data, section->data_size);
                if (FAILED(hr))
                {
                    WARN("Failed to parse RDEF section.\n");
                    goto err_out;
                }
                break;

            case TAG_ISGN:
                reflection->isgn = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*reflection->isgn));
                if (!reflection->isgn)
                {
                    ERR("Failed to allocate ISGN memory.\n");
                    hr = E_OUTOFMEMORY;
                    goto err_out;
                }

                hr = d3dcompiler_parse_signature(reflection->isgn, section, reflection->target);
                if (FAILED(hr))
                {
                    WARN("Failed to parse section ISGN.\n");
                    goto err_out;
                }
                break;

            case TAG_OSG5:
            case TAG_OSGN:
                reflection->osgn = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*reflection->osgn));
                if (!reflection->osgn)
                {
                    ERR("Failed to allocate OSGN memory.\n");
                    hr = E_OUTOFMEMORY;
                    goto err_out;
                }

                hr = d3dcompiler_parse_signature(reflection->osgn, section, reflection->target);
                if (FAILED(hr))
                {
                    WARN("Failed to parse section OSGN.\n");
                    goto err_out;
                }
                break;

            case TAG_PCSG:
                reflection->pcsg = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*reflection->pcsg));
                if (!reflection->pcsg)
                {
                    ERR("Failed to allocate PCSG memory.\n");
                    hr = E_OUTOFMEMORY;
                    goto err_out;
                }

                hr = d3dcompiler_parse_signature(reflection->pcsg, section, reflection->target);
                if (FAILED(hr))
                {
                    WARN("Failed to parse section PCSG.\n");
                    goto err_out;
                }
                break;

            case TAG_SHEX:
            case TAG_SHDR:
                hr = d3dcompiler_parse_shdr(reflection, section->data, section->data_size);
                if (FAILED(hr))
                {
                    WARN("Failed to parse SHDR section.\n");
                    goto err_out;
                }
                break;

            case TAG_STAT:
                hr = d3dcompiler_parse_stat(reflection, section->data, section->data_size);
                if (FAILED(hr))
                {
                    WARN("Failed to parse section STAT.\n");
                    goto err_out;
                }
                break;

            default:
                FIXME("Unhandled section %s!\n", debugstr_an((const char *)&section->tag, 4));
                break;
        }
    }

    dxbc_destroy(&src_dxbc);

    return hr;

err_out:
    reflection_cleanup(reflection);
    dxbc_destroy(&src_dxbc);

    return hr;
}

HRESULT WINAPI D3DReflect(const void *data, SIZE_T data_size, REFIID riid, void **reflector)
{
    struct d3dcompiler_shader_reflection *object;
    HRESULT hr;
    const DWORD *temp = data;

    TRACE("data %p, data_size %lu, riid %s, blob %p\n", data, data_size, debugstr_guid(riid), reflector);

    if (!data || data_size < 32)
    {
        WARN("Invalid argument supplied.\n");
        return D3DERR_INVALIDCALL;
    }

    if (temp[6] != data_size)
    {
        WARN("Wrong size supplied.\n");
        return E_FAIL;
    }

    if (!IsEqualGUID(riid, &IID_ID3D11ShaderReflection))
    {
        WARN("Wrong riid %s, accept only %s!\n", debugstr_guid(riid), debugstr_guid(&IID_ID3D11ShaderReflection));
        return E_NOINTERFACE;
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    hr = d3dcompiler_shader_reflection_init(object, data, data_size);
    if (FAILED(hr))
    {
        WARN("Failed to initialize shader reflection\n");
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    *reflector = object;

    TRACE("Created ID3D11ShaderReflection %p\n", object);

    return S_OK;
}

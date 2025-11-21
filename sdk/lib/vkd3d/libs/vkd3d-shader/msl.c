/*
 * Copyright 2024 Feifan He for CodeWeavers
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

#include "vkd3d_shader_private.h"

struct msl_src
{
    struct vkd3d_string_buffer *str;
};

struct msl_dst
{
    const struct vkd3d_shader_dst_param *vsir;
    struct vkd3d_string_buffer *register_name;
    struct vkd3d_string_buffer *mask;
};

struct msl_generator
{
    struct vsir_program *program;
    struct vkd3d_string_buffer_cache string_buffers;
    struct vkd3d_string_buffer *buffer;
    struct vkd3d_shader_location location;
    struct vkd3d_shader_message_context *message_context;
    unsigned int indent;
    const char *prefix;
    bool failed;

    const struct vkd3d_shader_interface_info *interface_info;
    const struct vkd3d_shader_scan_descriptor_info1 *descriptor_info;
};

static void VKD3D_PRINTF_FUNC(3, 4) msl_compiler_error(struct msl_generator *gen,
        enum vkd3d_shader_error error, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vkd3d_shader_verror(gen->message_context, &gen->location, error, fmt, args);
    va_end(args);
    gen->failed = true;
}

static const char *msl_get_prefix(enum vkd3d_shader_type type)
{
    switch (type)
    {
        case VKD3D_SHADER_TYPE_VERTEX:
            return "vs";
        case VKD3D_SHADER_TYPE_HULL:
            return "hs";
        case VKD3D_SHADER_TYPE_DOMAIN:
            return "ds";
        case VKD3D_SHADER_TYPE_GEOMETRY:
            return "gs";
        case VKD3D_SHADER_TYPE_PIXEL:
            return "ps";
        case VKD3D_SHADER_TYPE_COMPUTE:
            return "cs";
        default:
            return NULL;
    }
}

static void msl_print_indent(struct vkd3d_string_buffer *buffer, unsigned int indent)
{
    vkd3d_string_buffer_printf(buffer, "%*s", 4 * indent, "");
}

static void msl_print_register_datatype(struct vkd3d_string_buffer *buffer,
        struct msl_generator *gen, enum vkd3d_data_type data_type)
{
    vkd3d_string_buffer_printf(buffer, ".");
    switch (data_type)
    {
        case VKD3D_DATA_FLOAT:
            vkd3d_string_buffer_printf(buffer, "f");
            break;
        case VKD3D_DATA_INT:
            vkd3d_string_buffer_printf(buffer, "i");
            break;
        case VKD3D_DATA_UINT:
            vkd3d_string_buffer_printf(buffer, "u");
            break;
        default:
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled register datatype %#x.", data_type);
            vkd3d_string_buffer_printf(buffer, "<unrecognised register datatype %#x>", data_type);
            break;
    }
}

static void msl_print_register_name(struct vkd3d_string_buffer *buffer,
        struct msl_generator *gen, const struct vkd3d_shader_register *reg)
{
    switch (reg->type)
    {
        case VKD3DSPR_TEMP:
            vkd3d_string_buffer_printf(buffer, "r[%u]", reg->idx[0].offset);
            msl_print_register_datatype(buffer, gen, reg->data_type);
            break;

        case VKD3DSPR_INPUT:
            if (reg->idx_count != 1)
            {
                msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                        "Internal compiler error: Unhandled input register index count %u.", reg->idx_count);
                vkd3d_string_buffer_printf(buffer, "<unhandled register %#x>", reg->type);
                break;
            }
            if (reg->idx[0].rel_addr)
            {
                msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                        "Internal compiler error: Unhandled input register indirect addressing.");
                vkd3d_string_buffer_printf(buffer, "<unhandled register %#x>", reg->type);
                break;
            }
            vkd3d_string_buffer_printf(buffer, "v[%u]", reg->idx[0].offset);
            msl_print_register_datatype(buffer, gen, reg->data_type);
            break;

        case VKD3DSPR_OUTPUT:
            if (reg->idx_count != 1)
            {
                msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                        "Internal compiler error: Unhandled output register index count %u.", reg->idx_count);
                vkd3d_string_buffer_printf(buffer, "<unhandled register %#x>", reg->type);
                break;
            }
            if (reg->idx[0].rel_addr)
            {
                msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                        "Internal compiler error: Unhandled output register indirect addressing.");
                vkd3d_string_buffer_printf(buffer, "<unhandled register %#x>", reg->type);
                break;
            }
            vkd3d_string_buffer_printf(buffer, "o[%u]", reg->idx[0].offset);
            msl_print_register_datatype(buffer, gen, reg->data_type);
            break;

        case VKD3DSPR_CONSTBUFFER:
            if (reg->idx_count != 3)
            {
                msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                        "Internal compiler error: Unhandled constant buffer register index count %u.", reg->idx_count);
                vkd3d_string_buffer_printf(buffer, "<unhandled register %#x>", reg->type);
                break;
            }
            if (reg->idx[0].rel_addr || reg->idx[2].rel_addr)
            {
                msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                        "Internal compiler error: Unhandled constant buffer register indirect addressing.");
                vkd3d_string_buffer_printf(buffer, "<unhandled register %#x>", reg->type);
                break;
            }
            vkd3d_string_buffer_printf(buffer, "descriptors.cb_%u[%u]", reg->idx[0].offset, reg->idx[2].offset);
            msl_print_register_datatype(buffer, gen, reg->data_type);
            break;

        default:
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled register type %#x.", reg->type);
            vkd3d_string_buffer_printf(buffer, "<unrecognised register %#x>", reg->type);
            break;
    }
}

static void msl_print_swizzle(struct vkd3d_string_buffer *buffer, uint32_t swizzle, uint32_t mask)
{
    const char swizzle_chars[] = "xyzw";
    unsigned int i;

    vkd3d_string_buffer_printf(buffer, ".");
    for (i = 0; i < VKD3D_VEC4_SIZE; ++i)
    {
        if (mask & (VKD3DSP_WRITEMASK_0 << i))
            vkd3d_string_buffer_printf(buffer, "%c", swizzle_chars[vsir_swizzle_get_component(swizzle, i)]);
    }
}

static void msl_print_write_mask(struct vkd3d_string_buffer *buffer, uint32_t write_mask)
{
    vkd3d_string_buffer_printf(buffer, ".");
    if (write_mask & VKD3DSP_WRITEMASK_0)
        vkd3d_string_buffer_printf(buffer, "x");
    if (write_mask & VKD3DSP_WRITEMASK_1)
        vkd3d_string_buffer_printf(buffer, "y");
    if (write_mask & VKD3DSP_WRITEMASK_2)
        vkd3d_string_buffer_printf(buffer, "z");
    if (write_mask & VKD3DSP_WRITEMASK_3)
        vkd3d_string_buffer_printf(buffer, "w");
}

static void msl_src_cleanup(struct msl_src *src, struct vkd3d_string_buffer_cache *cache)
{
    vkd3d_string_buffer_release(cache, src->str);
}

static void msl_src_init(struct msl_src *msl_src, struct msl_generator *gen,
        const struct vkd3d_shader_src_param *vsir_src, uint32_t mask)
{
    const struct vkd3d_shader_register *reg = &vsir_src->reg;

    msl_src->str = vkd3d_string_buffer_get(&gen->string_buffers);

    if (reg->non_uniform)
        msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                "Internal compiler error: Unhandled 'non-uniform' modifier.");
    if (vsir_src->modifiers)
        msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                "Internal compiler error: Unhandled source modifier(s) %#x.", vsir_src->modifiers);

    msl_print_register_name(msl_src->str, gen, reg);
    if (reg->dimension == VSIR_DIMENSION_VEC4)
        msl_print_swizzle(msl_src->str, vsir_src->swizzle, mask);
}

static void msl_dst_cleanup(struct msl_dst *dst, struct vkd3d_string_buffer_cache *cache)
{
    vkd3d_string_buffer_release(cache, dst->mask);
    vkd3d_string_buffer_release(cache, dst->register_name);
}

static uint32_t msl_dst_init(struct msl_dst *msl_dst, struct msl_generator *gen,
        const struct vkd3d_shader_instruction *ins, const struct vkd3d_shader_dst_param *vsir_dst)
{
    uint32_t write_mask = vsir_dst->write_mask;

    if (ins->flags & VKD3DSI_PRECISE_XYZW)
        msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                "Internal compiler error: Unhandled 'precise' modifier.");
    if (vsir_dst->reg.non_uniform)
        msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                "Internal compiler error: Unhandled 'non-uniform' modifier.");

    msl_dst->vsir = vsir_dst;
    msl_dst->register_name = vkd3d_string_buffer_get(&gen->string_buffers);
    msl_dst->mask = vkd3d_string_buffer_get(&gen->string_buffers);

    msl_print_register_name(msl_dst->register_name, gen, &vsir_dst->reg);
    msl_print_write_mask(msl_dst->mask, write_mask);

    return write_mask;
}

static void VKD3D_PRINTF_FUNC(3, 4) msl_print_assignment(
        struct msl_generator *gen, struct msl_dst *dst, const char *format, ...)
{
    va_list args;

    if (dst->vsir->shift)
        msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                "Internal compiler error: Unhandled destination shift %#x.", dst->vsir->shift);
    if (dst->vsir->modifiers)
        msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                "Internal compiler error: Unhandled destination modifier(s) %#x.", dst->vsir->modifiers);

    msl_print_indent(gen->buffer, gen->indent);
    vkd3d_string_buffer_printf(gen->buffer, "%s%s = ", dst->register_name->buffer, dst->mask->buffer);

    va_start(args, format);
    vkd3d_string_buffer_vprintf(gen->buffer, format, args);
    va_end(args);

    vkd3d_string_buffer_printf(gen->buffer, ";\n");
}

static void msl_unhandled(struct msl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    msl_print_indent(gen->buffer, gen->indent);
    vkd3d_string_buffer_printf(gen->buffer, "/* <unhandled instruction %#x> */\n", ins->opcode);
    msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
            "Internal compiler error: Unhandled instruction %#x.", ins->opcode);
}

static void msl_mov(struct msl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    struct msl_src src;
    struct msl_dst dst;
    uint32_t mask;

    mask = msl_dst_init(&dst, gen, ins, &ins->dst[0]);
    msl_src_init(&src, gen, &ins->src[0], mask);

    msl_print_assignment(gen, &dst, "%s", src.str->buffer);

    msl_src_cleanup(&src, &gen->string_buffers);
    msl_dst_cleanup(&dst, &gen->string_buffers);
}

static void msl_ret(struct msl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    msl_print_indent(gen->buffer, gen->indent);
    vkd3d_string_buffer_printf(gen->buffer, "return;\n");
}

static void msl_handle_instruction(struct msl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    gen->location = ins->location;

    switch (ins->opcode)
    {
        case VKD3DSIH_DCL_INPUT:
        case VKD3DSIH_DCL_OUTPUT:
        case VKD3DSIH_DCL_OUTPUT_SIV:
        case VKD3DSIH_NOP:
            break;
        case VKD3DSIH_MOV:
            msl_mov(gen, ins);
            break;
        case VKD3DSIH_RET:
            msl_ret(gen, ins);
            break;
        default:
            msl_unhandled(gen, ins);
            break;
    }
}

static bool msl_check_shader_visibility(const struct msl_generator *gen,
        enum vkd3d_shader_visibility visibility)
{
    enum vkd3d_shader_type t = gen->program->shader_version.type;

    switch (visibility)
    {
        case VKD3D_SHADER_VISIBILITY_ALL:
            return true;
        case VKD3D_SHADER_VISIBILITY_VERTEX:
            return t == VKD3D_SHADER_TYPE_VERTEX;
        case VKD3D_SHADER_VISIBILITY_HULL:
            return t == VKD3D_SHADER_TYPE_HULL;
        case VKD3D_SHADER_VISIBILITY_DOMAIN:
            return t == VKD3D_SHADER_TYPE_DOMAIN;
        case VKD3D_SHADER_VISIBILITY_GEOMETRY:
            return t == VKD3D_SHADER_TYPE_GEOMETRY;
        case VKD3D_SHADER_VISIBILITY_PIXEL:
            return t == VKD3D_SHADER_TYPE_PIXEL;
        case VKD3D_SHADER_VISIBILITY_COMPUTE:
            return t == VKD3D_SHADER_TYPE_COMPUTE;
        default:
            WARN("Invalid shader visibility %#x.\n", visibility);
            return false;
    }
}

static bool msl_get_cbv_binding(const struct msl_generator *gen,
        unsigned int register_space, unsigned int register_idx, unsigned int *binding_idx)
{
    const struct vkd3d_shader_interface_info *interface_info = gen->interface_info;
    const struct vkd3d_shader_resource_binding *binding;
    unsigned int i;

    if (!interface_info)
        return false;

    for (i = 0; i < interface_info->binding_count; ++i)
    {
        binding = &interface_info->bindings[i];

        if (binding->type != VKD3D_SHADER_DESCRIPTOR_TYPE_CBV)
            continue;
        if (binding->register_space != register_space)
            continue;
        if (binding->register_index != register_idx)
            continue;
        if (!msl_check_shader_visibility(gen, binding->shader_visibility))
            continue;
        if (!(binding->flags & VKD3D_SHADER_BINDING_FLAG_BUFFER))
            continue;
        *binding_idx = i;
        return true;
    }

    return false;
}

static void msl_generate_cbv_declaration(struct msl_generator *gen,
        const struct vkd3d_shader_descriptor_info1 *cbv)
{
    const struct vkd3d_shader_descriptor_binding *binding;
    struct vkd3d_string_buffer *buffer = gen->buffer;
    unsigned int binding_idx;
    size_t size;

    if (cbv->count != 1)
    {
        msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_BINDING_NOT_FOUND,
                "Constant buffer %u has unsupported descriptor array size %u.", cbv->register_id, cbv->count);
        return;
    }

    if (!msl_get_cbv_binding(gen, cbv->register_space, cbv->register_index, &binding_idx))
    {
        msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_BINDING_NOT_FOUND,
                "No descriptor binding specified for constant buffer %u.", cbv->register_id);
        return;
    }

    binding = &gen->interface_info->bindings[binding_idx].binding;

    if (binding->set != 0)
    {
        msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_BINDING_NOT_FOUND,
                "Unsupported binding set %u specified for constant buffer %u.", binding->set, cbv->register_id);
        return;
    }

    if (binding->count != 1)
    {
        msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_BINDING_NOT_FOUND,
                "Unsupported binding count %u specified for constant buffer %u.", binding->count, cbv->register_id);
        return;
    }

    size = align(cbv->buffer_size, VKD3D_VEC4_SIZE * sizeof(uint32_t));
    size /= VKD3D_VEC4_SIZE * sizeof(uint32_t);

    vkd3d_string_buffer_printf(buffer,
            "constant vkd3d_vec4 *cb_%u [[id(%u)]];", cbv->register_id, binding->binding);
};

static void msl_generate_descriptor_struct_declarations(struct msl_generator *gen)
{
    const struct vkd3d_shader_scan_descriptor_info1 *info = gen->descriptor_info;
    const struct vkd3d_shader_descriptor_info1 *descriptor;
    struct vkd3d_string_buffer *buffer = gen->buffer;
    unsigned int i;

    if (!info->descriptor_count)
        return;

    vkd3d_string_buffer_printf(buffer, "struct vkd3d_%s_descriptors\n{\n", gen->prefix);

    for (i = 0; i < info->descriptor_count; ++i)
    {
        descriptor = &info->descriptors[i];

        msl_print_indent(buffer, 1);
        switch (descriptor->type)
        {
            case VKD3D_SHADER_DESCRIPTOR_TYPE_CBV:
                msl_generate_cbv_declaration(gen, descriptor);
                break;

            default:
                vkd3d_string_buffer_printf(buffer, "/* <unhandled descriptor type %#x> */", descriptor->type);
                msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                        "Internal compiler error: Unhandled descriptor type %#x.", descriptor->type);
                break;
        }
        vkd3d_string_buffer_printf(buffer, "\n");
    }

    vkd3d_string_buffer_printf(buffer, "};\n\n");
}

static void msl_generate_input_struct_declarations(struct msl_generator *gen)
{
    const struct shader_signature *signature = &gen->program->input_signature;
    enum vkd3d_shader_type type = gen->program->shader_version.type;
    struct vkd3d_string_buffer *buffer = gen->buffer;
    const struct signature_element *e;
    unsigned int i;

    vkd3d_string_buffer_printf(buffer, "struct vkd3d_%s_in\n{\n", gen->prefix);

    for (i = 0; i < signature->element_count; ++i)
    {
        e = &signature->elements[i];

        if (e->target_location == SIGNATURE_TARGET_LOCATION_UNUSED)
            continue;

        if (e->sysval_semantic)
        {
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled system value %#x.", e->sysval_semantic);
            continue;
        }

        if (e->min_precision != VKD3D_SHADER_MINIMUM_PRECISION_NONE)
        {
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled minimum precision %#x.", e->min_precision);
            continue;
        }

        if (e->interpolation_mode != VKD3DSIM_NONE)
        {
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled interpolation mode %#x.", e->interpolation_mode);
            continue;
        }

        if(e->register_count > 1)
        {
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled register count %u.", e->register_count);
            continue;
        }

        msl_print_indent(gen->buffer, 1);

        switch(e->component_type)
        {
            case VKD3D_SHADER_COMPONENT_FLOAT:
                vkd3d_string_buffer_printf(buffer, "float4 ");
                break;
            case VKD3D_SHADER_COMPONENT_INT:
                vkd3d_string_buffer_printf(buffer, "int4 ");
                break;
            case VKD3D_SHADER_COMPONENT_UINT:
                vkd3d_string_buffer_printf(buffer, "uint4 ");
                break;
            default:
                vkd3d_string_buffer_printf(buffer, "<unhandled component type %#x> ", e->component_type);
                msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                        "Internal compiler error: Unhandled component type %#x.", e->component_type);
                break;
        }

        vkd3d_string_buffer_printf(buffer, "shader_in_%u ", i);

        switch (type)
        {
            case VKD3D_SHADER_TYPE_VERTEX:
                vkd3d_string_buffer_printf(gen->buffer, "[[attribute(%u)]]", e->target_location);
                break;
            case VKD3D_SHADER_TYPE_PIXEL:
                vkd3d_string_buffer_printf(gen->buffer, "[[user(locn%u)]]", e->target_location);
                break;
            default:
                msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                        "Internal compiler error: Unhandled shader type %#x.", type);
                break;
        }

        vkd3d_string_buffer_printf(buffer, ";\n");
    }

    vkd3d_string_buffer_printf(buffer, "};\n\n");
}

static void msl_generate_vertex_output_element_attribute(struct msl_generator *gen, const struct signature_element *e)
{
    switch (e->sysval_semantic)
    {
        case VKD3D_SHADER_SV_POSITION:
            vkd3d_string_buffer_printf(gen->buffer, "[[position]]");
            break;
        case VKD3D_SHADER_SV_NONE:
            vkd3d_string_buffer_printf(gen->buffer, "[[user(locn%u)]]", e->target_location);
            break;
        default:
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled vertex shader system value %#x.", e->sysval_semantic);
            break;
    }
}

static void msl_generate_pixel_output_element_attribute(struct msl_generator *gen, const struct signature_element *e)
{
    switch (e->sysval_semantic)
    {
        case VKD3D_SHADER_SV_TARGET:
            vkd3d_string_buffer_printf(gen->buffer, "[[color(%u)]]", e->target_location);
            break;
        default:
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled pixel shader system value %#x.", e->sysval_semantic);
            break;
    }
}

static void msl_generate_output_struct_declarations(struct msl_generator *gen)
{
    const struct shader_signature *signature = &gen->program->output_signature;
    enum vkd3d_shader_type type = gen->program->shader_version.type;
    struct vkd3d_string_buffer *buffer = gen->buffer;
    const struct signature_element *e;
    unsigned int i;

    vkd3d_string_buffer_printf(buffer, "struct vkd3d_%s_out\n{\n", gen->prefix);

    for (i = 0; i < signature->element_count; ++i)
    {
        e = &signature->elements[i];

        if (e->target_location == SIGNATURE_TARGET_LOCATION_UNUSED)
            continue;

        if (e->min_precision != VKD3D_SHADER_MINIMUM_PRECISION_NONE)
        {
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled minimum precision %#x.", e->min_precision);
            continue;
        }

        if (e->interpolation_mode != VKD3DSIM_NONE)
        {
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled interpolation mode %#x.", e->interpolation_mode);
            continue;
        }

        if(e->register_count > 1)
        {
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled register count %u.", e->register_count);
            continue;
        }

        msl_print_indent(gen->buffer, 1);

        switch(e->component_type)
        {
            case VKD3D_SHADER_COMPONENT_FLOAT:
                vkd3d_string_buffer_printf(buffer, "float4 ");
                break;
            case VKD3D_SHADER_COMPONENT_INT:
                vkd3d_string_buffer_printf(buffer, "int4 ");
                break;
            case VKD3D_SHADER_COMPONENT_UINT:
                vkd3d_string_buffer_printf(buffer, "uint4 ");
                break;
            default:
                vkd3d_string_buffer_printf(buffer, "<unhandled component type %#x> ", e->component_type);
                msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                        "Internal compiler error: Unhandled component type %#x.", e->component_type);
                break;
        }

        vkd3d_string_buffer_printf(buffer, "shader_out_%u ", i);

        switch (type)
        {
            case VKD3D_SHADER_TYPE_VERTEX:
                msl_generate_vertex_output_element_attribute(gen, e);
                break;
            case VKD3D_SHADER_TYPE_PIXEL:
                msl_generate_pixel_output_element_attribute(gen, e);
                break;
            default:
                msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                        "Internal compiler error: Unhandled shader type %#x.", type);
                break;
        }

        vkd3d_string_buffer_printf(buffer, ";\n");
    }

    vkd3d_string_buffer_printf(buffer, "};\n\n");
}

static void msl_generate_entrypoint_prologue(struct msl_generator *gen)
{
    const struct shader_signature *signature = &gen->program->input_signature;
    struct vkd3d_string_buffer *buffer = gen->buffer;
    const struct signature_element *e;
    unsigned int i;

    for (i = 0; i < signature->element_count; ++i)
    {
        e = &signature->elements[i];

        if (e->target_location == SIGNATURE_TARGET_LOCATION_UNUSED)
            continue;

        vkd3d_string_buffer_printf(buffer, "    %s_in[%u]", gen->prefix, e->register_index);
        if (e->sysval_semantic == VKD3D_SHADER_SV_NONE)
        {
            msl_print_register_datatype(buffer, gen, vkd3d_data_type_from_component_type(e->component_type));
            msl_print_write_mask(buffer, e->mask);
            vkd3d_string_buffer_printf(buffer, " = input.shader_in_%u", i);
            msl_print_write_mask(buffer, e->mask);
        }
        else
        {
            vkd3d_string_buffer_printf(buffer, " = <unhandled sysval %#x>", e->sysval_semantic);
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled system value %#x input.", e->sysval_semantic);
        }
        vkd3d_string_buffer_printf(buffer, ";\n");
    }
}

static void msl_generate_entrypoint_epilogue(struct msl_generator *gen)
{
    const struct shader_signature *signature = &gen->program->output_signature;
    struct vkd3d_string_buffer *buffer = gen->buffer;
    const struct signature_element *e;
    unsigned int i;

    for (i = 0; i < signature->element_count; ++i)
    {
        e = &signature->elements[i];

        if (e->target_location == SIGNATURE_TARGET_LOCATION_UNUSED)
            continue;

        switch (e->sysval_semantic)
        {
            case VKD3D_SHADER_SV_NONE:
            case VKD3D_SHADER_SV_TARGET:
            case VKD3D_SHADER_SV_POSITION:
                vkd3d_string_buffer_printf(buffer, "    output.shader_out_%u", i);
                msl_print_write_mask(buffer, e->mask);
                vkd3d_string_buffer_printf(buffer, " = %s_out[%u]", gen->prefix, e->register_index);
                msl_print_register_datatype(buffer, gen, vkd3d_data_type_from_component_type(e->component_type));
                msl_print_write_mask(buffer, e->mask);
                break;
            default:
                vkd3d_string_buffer_printf(buffer, "    <unhandled sysval %#x>", e->sysval_semantic);
                msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                        "Internal compiler error: Unhandled system value %#x input.", e->sysval_semantic);
        }
        vkd3d_string_buffer_printf(buffer, ";\n");
    }
}

static void msl_generate_entrypoint(struct msl_generator *gen)
{
    enum vkd3d_shader_type type = gen->program->shader_version.type;

    switch (type)
    {
        case VKD3D_SHADER_TYPE_VERTEX:
            vkd3d_string_buffer_printf(gen->buffer, "vertex ");
            break;
        case VKD3D_SHADER_TYPE_PIXEL:
            vkd3d_string_buffer_printf(gen->buffer, "fragment ");
            break;
        default:
            msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                    "Internal compiler error: Unhandled shader type %#x.", type);
            return;
    }

    vkd3d_string_buffer_printf(gen->buffer, "vkd3d_%s_out shader_entry(\n", gen->prefix);

    if (gen->descriptor_info->descriptor_count)
    {
        msl_print_indent(gen->buffer, 2);
        /* TODO: Configurable argument buffer binding location. */
        vkd3d_string_buffer_printf(gen->buffer,
                "constant vkd3d_%s_descriptors& descriptors [[buffer(0)]],\n", gen->prefix);
    }

    msl_print_indent(gen->buffer, 2);
    vkd3d_string_buffer_printf(gen->buffer, "vkd3d_%s_in input [[stage_in]])\n{\n", gen->prefix);

    /* TODO: declare #maximum_register + 1 */
    vkd3d_string_buffer_printf(gen->buffer, "    vkd3d_vec4 %s_in[%u];\n", gen->prefix, 32);
    vkd3d_string_buffer_printf(gen->buffer, "    vkd3d_vec4 %s_out[%u];\n", gen->prefix, 32);
    vkd3d_string_buffer_printf(gen->buffer, "    vkd3d_%s_out output;\n", gen->prefix);

    msl_generate_entrypoint_prologue(gen);

    vkd3d_string_buffer_printf(gen->buffer, "    %s_main(%s_in, %s_out", gen->prefix, gen->prefix, gen->prefix);
    if (gen->descriptor_info->descriptor_count)
        vkd3d_string_buffer_printf(gen->buffer, ", descriptors");
    vkd3d_string_buffer_printf(gen->buffer, ");\n");

    msl_generate_entrypoint_epilogue(gen);

    vkd3d_string_buffer_printf(gen->buffer, "    return output;\n}\n");
}

static int msl_generator_generate(struct msl_generator *gen, struct vkd3d_shader_code *out)
{
    const struct vkd3d_shader_instruction_array *instructions = &gen->program->instructions;
    unsigned int i;

    MESSAGE("Generating a MSL shader. This is unsupported; you get to keep all the pieces if it breaks.\n");

    vkd3d_string_buffer_printf(gen->buffer, "/* Generated by %s. */\n\n", vkd3d_shader_get_version(NULL, NULL));

    if (gen->program->global_flags)
        msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                "Internal compiler error: Unhandled global flags %#"PRIx64".", (uint64_t)gen->program->global_flags);

    vkd3d_string_buffer_printf(gen->buffer, "union vkd3d_vec4\n{\n");
    vkd3d_string_buffer_printf(gen->buffer, "    uint4 u;\n");
    vkd3d_string_buffer_printf(gen->buffer, "    int4 i;\n");
    vkd3d_string_buffer_printf(gen->buffer, "    float4 f;\n};\n\n");

    msl_generate_descriptor_struct_declarations(gen);
    msl_generate_input_struct_declarations(gen);
    msl_generate_output_struct_declarations(gen);

    vkd3d_string_buffer_printf(gen->buffer,
            "void %s_main(thread vkd3d_vec4 *v, "
            "thread vkd3d_vec4 *o",
            gen->prefix);
    if (gen->descriptor_info->descriptor_count)
        vkd3d_string_buffer_printf(gen->buffer, ", constant vkd3d_%s_descriptors& descriptors", gen->prefix);
    vkd3d_string_buffer_printf(gen->buffer, ")\n{\n");

    ++gen->indent;

    if (gen->program->temp_count)
    {
        msl_print_indent(gen->buffer, gen->indent);
        vkd3d_string_buffer_printf(gen->buffer, "vkd3d_vec4 r[%u];\n\n", gen->program->temp_count);
    }

    for (i = 0; i < instructions->count; ++i)
    {
        msl_handle_instruction(gen, &instructions->elements[i]);
    }

    --gen->indent;

    vkd3d_string_buffer_printf(gen->buffer, "}\n\n");

    msl_generate_entrypoint(gen);

    if (TRACE_ON())
        vkd3d_string_buffer_trace(gen->buffer);

    if (gen->failed)
        return VKD3D_ERROR_INVALID_SHADER;

    vkd3d_shader_code_from_string_buffer(out, gen->buffer);

    return VKD3D_OK;
}

static void msl_generator_cleanup(struct msl_generator *gen)
{
    vkd3d_string_buffer_release(&gen->string_buffers, gen->buffer);
    vkd3d_string_buffer_cache_cleanup(&gen->string_buffers);
}

static int msl_generator_init(struct msl_generator *gen, struct vsir_program *program,
        const struct vkd3d_shader_compile_info *compile_info,
        const struct vkd3d_shader_scan_descriptor_info1 *descriptor_info,
        struct vkd3d_shader_message_context *message_context)
{
    enum vkd3d_shader_type type = program->shader_version.type;

    memset(gen, 0, sizeof(*gen));
    gen->program = program;
    vkd3d_string_buffer_cache_init(&gen->string_buffers);
    if (!(gen->buffer = vkd3d_string_buffer_get(&gen->string_buffers)))
    {
        vkd3d_string_buffer_cache_cleanup(&gen->string_buffers);
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }
    gen->message_context = message_context;
    if (!(gen->prefix = msl_get_prefix(type)))
    {
        msl_compiler_error(gen, VKD3D_SHADER_ERROR_MSL_INTERNAL,
                "Internal compiler error: Unhandled shader type %#x.", type);
        return VKD3D_ERROR_INVALID_SHADER;
    }
    gen->interface_info = vkd3d_find_struct(compile_info->next, INTERFACE_INFO);
    gen->descriptor_info = descriptor_info;

    return VKD3D_OK;
}

int msl_compile(struct vsir_program *program, uint64_t config_flags,
        const struct vkd3d_shader_scan_descriptor_info1 *descriptor_info,
        const struct vkd3d_shader_compile_info *compile_info, struct vkd3d_shader_code *out,
        struct vkd3d_shader_message_context *message_context)
{
    struct msl_generator generator;
    int ret;

    if ((ret = vsir_program_transform(program, config_flags, compile_info, message_context)) < 0)
        return ret;

    VKD3D_ASSERT(program->normalisation_level == VSIR_FULLY_NORMALISED_IO);

    if ((ret = msl_generator_init(&generator, program, compile_info, descriptor_info, message_context)) < 0)
        return ret;
    ret = msl_generator_generate(&generator, out);
    msl_generator_cleanup(&generator);

    return ret;
}

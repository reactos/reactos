/*
 * Copyright 2021 Atharva Nimbalkar
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

struct glsl_resource_type_info
{
    /* The number of coordinates needed to sample the resource type. */
    size_t coord_size;
    /* Whether the resource type is an array type. */
    bool array;
    /* Whether the resource type has a shadow/comparison variant. */
    bool shadow;
    /* The type suffix for resource type. I.e., the "2D" part of "usampler2D"
     * or "iimage2D". */
    const char *type_suffix;
};

struct glsl_src
{
    struct vkd3d_string_buffer *str;
};

struct glsl_dst
{
    const struct vkd3d_shader_dst_param *vsir;
    struct vkd3d_string_buffer *register_name;
    struct vkd3d_string_buffer *mask;
};

struct vkd3d_glsl_generator
{
    struct vsir_program *program;
    struct vkd3d_string_buffer_cache string_buffers;
    struct vkd3d_string_buffer *buffer;
    struct vkd3d_shader_location location;
    struct vkd3d_shader_message_context *message_context;
    unsigned int indent;
    const char *prefix;
    bool failed;

    struct shader_limits
    {
        unsigned int input_count;
        unsigned int output_count;
    } limits;
    bool interstage_input;
    bool interstage_output;

    const struct vkd3d_shader_interface_info *interface_info;
    const struct vkd3d_shader_descriptor_offset_info *offset_info;
    const struct vkd3d_shader_scan_descriptor_info1 *descriptor_info;
    const struct vkd3d_shader_scan_combined_resource_sampler_info *combined_sampler_info;
};

static void shader_glsl_print_subscript(struct vkd3d_string_buffer *buffer, struct vkd3d_glsl_generator *gen,
        const struct vkd3d_shader_src_param *rel_addr, unsigned int offset);

static void VKD3D_PRINTF_FUNC(3, 4) vkd3d_glsl_compiler_error(
        struct vkd3d_glsl_generator *generator,
        enum vkd3d_shader_error error, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vkd3d_shader_verror(generator->message_context, &generator->location, error, fmt, args);
    va_end(args);
    generator->failed = true;
}

static const char *shader_glsl_get_prefix(enum vkd3d_shader_type type)
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

static const struct glsl_resource_type_info *shader_glsl_get_resource_type_info(enum vkd3d_shader_resource_type t)
{
    static const struct glsl_resource_type_info info[] =
    {
        {0, 0, 0, "None"},      /* VKD3D_SHADER_RESOURCE_NONE */
        {1, 0, 0, "Buffer"},    /* VKD3D_SHADER_RESOURCE_BUFFER */
        {1, 0, 1, "1D"},        /* VKD3D_SHADER_RESOURCE_TEXTURE_1D */
        {2, 0, 1, "2D"},        /* VKD3D_SHADER_RESOURCE_TEXTURE_2D */
        {2, 0, 0, "2DMS"},      /* VKD3D_SHADER_RESOURCE_TEXTURE_2DMS */
        {3, 0, 0, "3D"},        /* VKD3D_SHADER_RESOURCE_TEXTURE_3D */
        {3, 0, 1, "Cube"},      /* VKD3D_SHADER_RESOURCE_TEXTURE_CUBE */
        {2, 1, 1, "1DArray"},   /* VKD3D_SHADER_RESOURCE_TEXTURE_1DARRAY */
        {3, 1, 1, "2DArray"},   /* VKD3D_SHADER_RESOURCE_TEXTURE_2DARRAY */
        {3, 1, 0, "2DMSArray"}, /* VKD3D_SHADER_RESOURCE_TEXTURE_2DMSARRAY */
        {4, 1, 1, "CubeArray"}, /* VKD3D_SHADER_RESOURCE_TEXTURE_CUBEARRAY */
    };

    if (!t || t >= ARRAY_SIZE(info))
        return NULL;

    return &info[t];
}

static const struct vkd3d_shader_descriptor_info1 *shader_glsl_get_descriptor(struct vkd3d_glsl_generator *gen,
        enum vkd3d_shader_descriptor_type type, unsigned int idx, unsigned int space)
{
    const struct vkd3d_shader_scan_descriptor_info1 *info = gen->descriptor_info;

    for (unsigned int i = 0; i < info->descriptor_count; ++i)
    {
        const struct vkd3d_shader_descriptor_info1 *d = &info->descriptors[i];

        if (d->type == type && d->register_space == space && d->register_index == idx)
            return d;
    }

    return NULL;
}

static const struct vkd3d_shader_descriptor_info1 *shader_glsl_get_descriptor_by_id(
        struct vkd3d_glsl_generator *gen, enum vkd3d_shader_descriptor_type type, unsigned int id)
{
    const struct vkd3d_shader_scan_descriptor_info1 *info = gen->descriptor_info;

    for (unsigned int i = 0; i < info->descriptor_count; ++i)
    {
        const struct vkd3d_shader_descriptor_info1 *d = &info->descriptors[i];

        if (d->type == type && d->register_id == id)
            return d;
    }

    return NULL;
}

static void shader_glsl_print_indent(struct vkd3d_string_buffer *buffer, unsigned int indent)
{
    vkd3d_string_buffer_printf(buffer, "%*s", 4 * indent, "");
}

static void shader_glsl_print_combined_sampler_name(struct vkd3d_string_buffer *buffer,
        struct vkd3d_glsl_generator *gen, unsigned int resource_index,
        unsigned int resource_space, unsigned int sampler_index, unsigned int sampler_space)
{
    vkd3d_string_buffer_printf(buffer, "%s_t_%u", gen->prefix, resource_index);
    if (resource_space)
        vkd3d_string_buffer_printf(buffer, "_%u", resource_space);
    if (sampler_index != VKD3D_SHADER_DUMMY_SAMPLER_INDEX)
    {
        vkd3d_string_buffer_printf(buffer, "_s_%u", sampler_index);
        if (sampler_space)
            vkd3d_string_buffer_printf(buffer, "_%u", sampler_space);
    }
}

static void shader_glsl_print_image_name(struct vkd3d_string_buffer *buffer,
        struct vkd3d_glsl_generator *gen, unsigned int idx, unsigned int space)
{
    vkd3d_string_buffer_printf(buffer, "%s_image_%u", gen->prefix, idx);
    if (space)
        vkd3d_string_buffer_printf(buffer, "_%u", space);
}

static void shader_glsl_print_register_name(struct vkd3d_string_buffer *buffer,
        struct vkd3d_glsl_generator *gen, const struct vkd3d_shader_register *reg)
{
    switch (reg->type)
    {
        case VKD3DSPR_TEMP:
            vkd3d_string_buffer_printf(buffer, "r[%u]", reg->idx[0].offset);
            break;

        case VKD3DSPR_INPUT:
            if (reg->idx_count != 1)
            {
                vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                        "Internal compiler error: Unhandled input register index count %u.", reg->idx_count);
                vkd3d_string_buffer_printf(buffer, "<unhandled register %#x>", reg->type);
                break;
            }
            if (reg->idx[0].rel_addr)
            {
                vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                        "Internal compiler error: Unhandled input register indirect addressing.");
                vkd3d_string_buffer_printf(buffer, "<unhandled register %#x>", reg->type);
                break;
            }
            vkd3d_string_buffer_printf(buffer, "%s_in[%u]", gen->prefix, reg->idx[0].offset);
            break;

        case VKD3DSPR_OUTPUT:
            if (reg->idx_count != 1)
            {
                vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                        "Internal compiler error: Unhandled output register index count %u.", reg->idx_count);
                vkd3d_string_buffer_printf(buffer, "<unhandled register %#x>", reg->type);
                break;
            }
            if (reg->idx[0].rel_addr)
            {
                vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                        "Internal compiler error: Unhandled output register indirect addressing.");
                vkd3d_string_buffer_printf(buffer, "<unhandled register %#x>", reg->type);
                break;
            }
            vkd3d_string_buffer_printf(buffer, "%s_out[%u]", gen->prefix, reg->idx[0].offset);
            break;

        case VKD3DSPR_DEPTHOUT:
            if (gen->program->shader_version.type != VKD3D_SHADER_TYPE_PIXEL)
                vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                        "Internal compiler error: Unhandled depth output in shader type #%x.",
                        gen->program->shader_version.type);
            vkd3d_string_buffer_printf(buffer, "gl_FragDepth");
            break;

        case VKD3DSPR_IMMCONST:
            switch (reg->dimension)
            {
                case VSIR_DIMENSION_SCALAR:
                    vkd3d_string_buffer_printf(buffer, "%#xu", reg->u.immconst_u32[0]);
                    break;

                case VSIR_DIMENSION_VEC4:
                    vkd3d_string_buffer_printf(buffer, "uvec4(%#xu, %#xu, %#xu, %#xu)",
                            reg->u.immconst_u32[0], reg->u.immconst_u32[1],
                            reg->u.immconst_u32[2], reg->u.immconst_u32[3]);
                    break;

                default:
                    vkd3d_string_buffer_printf(buffer, "<unhandled_dimension %#x>", reg->dimension);
                    vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                            "Internal compiler error: Unhandled dimension %#x.", reg->dimension);
                    break;
            }
            break;

        case VKD3DSPR_CONSTBUFFER:
            if (reg->idx_count != 3)
            {
                vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                        "Internal compiler error: Unhandled constant buffer register index count %u.", reg->idx_count);
                vkd3d_string_buffer_printf(buffer, "<unhandled register %#x>", reg->type);
                break;
            }
            if (reg->idx[0].rel_addr || reg->idx[2].rel_addr)
            {
                vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                        "Internal compiler error: Unhandled constant buffer register indirect addressing.");
                vkd3d_string_buffer_printf(buffer, "<unhandled register %#x>", reg->type);
                break;
            }
            vkd3d_string_buffer_printf(buffer, "%s_cb_%u[%u]",
                    gen->prefix, reg->idx[0].offset, reg->idx[2].offset);
            break;

        case VKD3DSPR_THREADID:
            vkd3d_string_buffer_printf(buffer, "gl_GlobalInvocationID");
            break;

        case VKD3DSPR_IDXTEMP:
            vkd3d_string_buffer_printf(buffer, "x%u", reg->idx[0].offset);
            shader_glsl_print_subscript(buffer, gen, reg->idx[1].rel_addr, reg->idx[1].offset);
            break;

        default:
            vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                    "Internal compiler error: Unhandled register type %#x.", reg->type);
            vkd3d_string_buffer_printf(buffer, "<unrecognised register %#x>", reg->type);
            break;
    }
}

static void shader_glsl_print_swizzle(struct vkd3d_string_buffer *buffer, uint32_t swizzle, uint32_t mask)
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

static void shader_glsl_print_write_mask(struct vkd3d_string_buffer *buffer, uint32_t write_mask)
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

static void glsl_src_cleanup(struct glsl_src *src, struct vkd3d_string_buffer_cache *cache)
{
    vkd3d_string_buffer_release(cache, src->str);
}

static void shader_glsl_print_bitcast(struct vkd3d_string_buffer *dst, struct vkd3d_glsl_generator *gen,
        const char *src, enum vkd3d_data_type dst_data_type, enum vkd3d_data_type src_data_type, unsigned int size)
{
    if (dst_data_type == VKD3D_DATA_UNORM || dst_data_type == VKD3D_DATA_SNORM)
        dst_data_type = VKD3D_DATA_FLOAT;
    if (src_data_type == VKD3D_DATA_UNORM || src_data_type == VKD3D_DATA_SNORM)
        src_data_type = VKD3D_DATA_FLOAT;

    if (dst_data_type == src_data_type)
    {
        vkd3d_string_buffer_printf(dst, "%s", src);
        return;
    }

    if (src_data_type == VKD3D_DATA_FLOAT)
    {
        switch (dst_data_type)
        {
            case VKD3D_DATA_INT:
                vkd3d_string_buffer_printf(dst, "floatBitsToInt(%s)", src);
                return;
            case VKD3D_DATA_UINT:
                vkd3d_string_buffer_printf(dst, "floatBitsToUint(%s)", src);
                return;
            default:
                break;
        }
    }

    if (src_data_type == VKD3D_DATA_UINT)
    {
        switch (dst_data_type)
        {
            case VKD3D_DATA_FLOAT:
                vkd3d_string_buffer_printf(dst, "uintBitsToFloat(%s)", src);
                return;
            case VKD3D_DATA_INT:
                if (size == 1)
                    vkd3d_string_buffer_printf(dst, "int(%s)", src);
                else
                    vkd3d_string_buffer_printf(dst, "ivec%u(%s)", size, src);
                return;
            default:
                break;
        }
    }

    vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
            "Internal compiler error: Unhandled bitcast from %#x to %#x.",
            src_data_type, dst_data_type);
    vkd3d_string_buffer_printf(dst, "%s", src);
}

static void shader_glsl_print_src(struct vkd3d_string_buffer *buffer, struct vkd3d_glsl_generator *gen,
        const struct vkd3d_shader_src_param *vsir_src, uint32_t mask, enum vkd3d_data_type data_type)
{
    const struct vkd3d_shader_register *reg = &vsir_src->reg;
    struct vkd3d_string_buffer *register_name, *str;
    enum vkd3d_data_type src_data_type;
    unsigned int size;

    register_name = vkd3d_string_buffer_get(&gen->string_buffers);

    if (reg->non_uniform)
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Unhandled 'non-uniform' modifier.");

    if (reg->type == VKD3DSPR_IMMCONST || reg->type == VKD3DSPR_THREADID)
        src_data_type = VKD3D_DATA_UINT;
    else
        src_data_type = VKD3D_DATA_FLOAT;

    shader_glsl_print_register_name(register_name, gen, reg);

    if (!vsir_src->modifiers)
        str = buffer;
    else
        str = vkd3d_string_buffer_get(&gen->string_buffers);

    size = reg->dimension == VSIR_DIMENSION_VEC4 ? 4 : 1;
    shader_glsl_print_bitcast(str, gen, register_name->buffer, data_type, src_data_type, size);
    if (reg->dimension == VSIR_DIMENSION_VEC4)
        shader_glsl_print_swizzle(str, vsir_src->swizzle, mask);

    switch (vsir_src->modifiers)
    {
        case VKD3DSPSM_NONE:
            break;
        case VKD3DSPSM_NEG:
            vkd3d_string_buffer_printf(buffer, "-%s", str->buffer);
            break;
        case VKD3DSPSM_ABS:
            vkd3d_string_buffer_printf(buffer, "abs(%s)", str->buffer);
            break;
        default:
            vkd3d_string_buffer_printf(buffer, "<unhandled modifier %#x>(%s)",
                    vsir_src->modifiers, str->buffer);
            vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                    "Internal compiler error: Unhandled source modifier(s) %#x.", vsir_src->modifiers);
            break;
    }

    if (str != buffer)
        vkd3d_string_buffer_release(&gen->string_buffers, str);
    vkd3d_string_buffer_release(&gen->string_buffers, register_name);
}

static void glsl_src_init(struct glsl_src *glsl_src, struct vkd3d_glsl_generator *gen,
        const struct vkd3d_shader_src_param *vsir_src, uint32_t mask)
{
    glsl_src->str = vkd3d_string_buffer_get(&gen->string_buffers);
    shader_glsl_print_src(glsl_src->str, gen, vsir_src, mask, vsir_src->reg.data_type);
}

static void glsl_dst_cleanup(struct glsl_dst *dst, struct vkd3d_string_buffer_cache *cache)
{
    vkd3d_string_buffer_release(cache, dst->mask);
    vkd3d_string_buffer_release(cache, dst->register_name);
}

static uint32_t glsl_dst_init(struct glsl_dst *glsl_dst, struct vkd3d_glsl_generator *gen,
        const struct vkd3d_shader_instruction *ins, const struct vkd3d_shader_dst_param *vsir_dst)
{
    uint32_t write_mask = vsir_dst->write_mask;

    if (ins->flags & VKD3DSI_PRECISE_XYZW)
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Unhandled 'precise' modifier.");
    if (vsir_dst->reg.non_uniform)
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Unhandled 'non-uniform' modifier.");

    glsl_dst->vsir = vsir_dst;
    glsl_dst->register_name = vkd3d_string_buffer_get(&gen->string_buffers);
    glsl_dst->mask = vkd3d_string_buffer_get(&gen->string_buffers);

    shader_glsl_print_register_name(glsl_dst->register_name, gen, &vsir_dst->reg);
    shader_glsl_print_write_mask(glsl_dst->mask, write_mask);

    return write_mask;
}

static void shader_glsl_print_subscript(struct vkd3d_string_buffer *buffer, struct vkd3d_glsl_generator *gen,
        const struct vkd3d_shader_src_param *rel_addr, unsigned int offset)
{
    struct glsl_src r;

    if (!rel_addr)
    {
        vkd3d_string_buffer_printf(buffer, "[%u]", offset);
        return;
    }

    glsl_src_init(&r, gen, rel_addr, VKD3DSP_WRITEMASK_0);
    vkd3d_string_buffer_printf(buffer, "[%s", r.str->buffer);
    if (offset)
        vkd3d_string_buffer_printf(buffer, " + %u", offset);
    else
        vkd3d_string_buffer_printf(buffer, "]");
    glsl_src_cleanup(&r, &gen->string_buffers);
}

static void VKD3D_PRINTF_FUNC(4, 0) shader_glsl_vprint_assignment(struct vkd3d_glsl_generator *gen,
        struct glsl_dst *dst, enum vkd3d_data_type data_type, const char *format, va_list args)
{
    struct vkd3d_string_buffer *buffer = gen->buffer;
    uint32_t modifiers = dst->vsir->modifiers;
    bool close = true;

    if (dst->vsir->shift)
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Unhandled destination shift %#x.", dst->vsir->shift);
    if (modifiers & ~VKD3DSPDM_SATURATE)
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Unhandled destination modifier(s) %#x.", modifiers);

    shader_glsl_print_indent(buffer, gen->indent);
    vkd3d_string_buffer_printf(buffer, "%s%s = ", dst->register_name->buffer, dst->mask->buffer);
    if (modifiers & VKD3DSPDM_SATURATE)
        vkd3d_string_buffer_printf(buffer, "clamp(");

    switch (data_type)
    {
        default:
            vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                    "Internal compiler error: Unhandled destination register data type %#x.", data_type);
            /* fall through */
        case VKD3D_DATA_FLOAT:
            close = false;
            break;
        case VKD3D_DATA_INT:
            vkd3d_string_buffer_printf(buffer, "intBitsToFloat(");
            break;
        case VKD3D_DATA_UINT:
            vkd3d_string_buffer_printf(buffer, "uintBitsToFloat(");
            break;
    }

    vkd3d_string_buffer_vprintf(buffer, format, args);

    if (close)
        vkd3d_string_buffer_printf(buffer, ")");
    if (modifiers & VKD3DSPDM_SATURATE)
        vkd3d_string_buffer_printf(buffer, ", 0.0, 1.0)");
    vkd3d_string_buffer_printf(buffer, ";\n");
}

static void VKD3D_PRINTF_FUNC(3, 4) shader_glsl_print_assignment(
        struct vkd3d_glsl_generator *gen, struct glsl_dst *dst, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    shader_glsl_vprint_assignment(gen, dst, dst->vsir->reg.data_type, format, args);
    va_end(args);
}

static void VKD3D_PRINTF_FUNC(4, 5) shader_glsl_print_assignment_ext(struct vkd3d_glsl_generator *gen,
        struct glsl_dst *dst, enum vkd3d_data_type data_type, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    shader_glsl_vprint_assignment(gen, dst, data_type, format, args);
    va_end(args);
}

static void shader_glsl_unhandled(struct vkd3d_glsl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    shader_glsl_print_indent(gen->buffer, gen->indent);
    vkd3d_string_buffer_printf(gen->buffer, "/* <unhandled instruction %#x> */\n", ins->opcode);
    vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
            "Internal compiler error: Unhandled instruction %#x.", ins->opcode);
}

static void shader_glsl_binop(struct vkd3d_glsl_generator *gen,
        const struct vkd3d_shader_instruction *ins, const char *op)
{
    struct glsl_src src[2];
    struct glsl_dst dst;
    uint32_t mask;

    mask = glsl_dst_init(&dst, gen, ins, &ins->dst[0]);
    glsl_src_init(&src[0], gen, &ins->src[0], mask);
    glsl_src_init(&src[1], gen, &ins->src[1], mask);

    shader_glsl_print_assignment(gen, &dst, "%s %s %s", src[0].str->buffer, op, src[1].str->buffer);

    glsl_src_cleanup(&src[1], &gen->string_buffers);
    glsl_src_cleanup(&src[0], &gen->string_buffers);
    glsl_dst_cleanup(&dst, &gen->string_buffers);
}

static void shader_glsl_dot(struct vkd3d_glsl_generator *gen,
        const struct vkd3d_shader_instruction *ins, uint32_t src_mask)
{
    unsigned int component_count;
    struct glsl_src src[2];
    struct glsl_dst dst;
    uint32_t dst_mask;

    dst_mask = glsl_dst_init(&dst, gen, ins, &ins->dst[0]);
    glsl_src_init(&src[0], gen, &ins->src[0], src_mask);
    glsl_src_init(&src[1], gen, &ins->src[1], src_mask);

    if ((component_count = vsir_write_mask_component_count(dst_mask)) > 1)
        shader_glsl_print_assignment(gen, &dst, "vec%d(dot(%s, %s))",
                component_count, src[0].str->buffer, src[1].str->buffer);
    else
        shader_glsl_print_assignment(gen, &dst, "dot(%s, %s)",
                src[0].str->buffer, src[1].str->buffer);

    glsl_src_cleanup(&src[1], &gen->string_buffers);
    glsl_src_cleanup(&src[0], &gen->string_buffers);
    glsl_dst_cleanup(&dst, &gen->string_buffers);
}

static void shader_glsl_intrinsic(struct vkd3d_glsl_generator *gen,
        const struct vkd3d_shader_instruction *ins, const char *op)
{
    struct vkd3d_string_buffer *args;
    struct glsl_src src;
    struct glsl_dst dst;
    unsigned int i;
    uint32_t mask;

    mask = glsl_dst_init(&dst, gen, ins, &ins->dst[0]);
    args = vkd3d_string_buffer_get(&gen->string_buffers);

    for (i = 0; i < ins->src_count; ++i)
    {
        glsl_src_init(&src, gen, &ins->src[i], mask);
        vkd3d_string_buffer_printf(args, "%s%s", i ? ", " : "", src.str->buffer);
        glsl_src_cleanup(&src, &gen->string_buffers);
    }
    shader_glsl_print_assignment(gen, &dst, "%s(%s)", op, args->buffer);

    vkd3d_string_buffer_release(&gen->string_buffers, args);
    glsl_dst_cleanup(&dst, &gen->string_buffers);
}

static void shader_glsl_relop(struct vkd3d_glsl_generator *gen,
        const struct vkd3d_shader_instruction *ins, const char *scalar_op, const char *vector_op)
{
    unsigned int mask_size;
    struct glsl_src src[2];
    struct glsl_dst dst;
    uint32_t mask;

    mask = glsl_dst_init(&dst, gen, ins, &ins->dst[0]);
    glsl_src_init(&src[0], gen, &ins->src[0], mask);
    glsl_src_init(&src[1], gen, &ins->src[1], mask);

    if ((mask_size = vsir_write_mask_component_count(mask)) > 1)
        shader_glsl_print_assignment(gen, &dst, "uvec%u(%s(%s, %s)) * 0xffffffffu",
                mask_size, vector_op, src[0].str->buffer, src[1].str->buffer);
    else
        shader_glsl_print_assignment(gen, &dst, "%s %s %s ? 0xffffffffu : 0u",
                src[0].str->buffer, scalar_op, src[1].str->buffer);

    glsl_src_cleanup(&src[1], &gen->string_buffers);
    glsl_src_cleanup(&src[0], &gen->string_buffers);
    glsl_dst_cleanup(&dst, &gen->string_buffers);
}

static void shader_glsl_cast(struct vkd3d_glsl_generator *gen, const struct vkd3d_shader_instruction *ins,
        const char *scalar_constructor, const char *vector_constructor)
{
    unsigned int component_count;
    struct glsl_src src;
    struct glsl_dst dst;
    uint32_t mask;

    mask = glsl_dst_init(&dst, gen, ins, &ins->dst[0]);
    glsl_src_init(&src, gen, &ins->src[0], mask);

    if ((component_count = vsir_write_mask_component_count(mask)) > 1)
        shader_glsl_print_assignment(gen, &dst, "%s%u(%s)",
                vector_constructor, component_count, src.str->buffer);
    else
        shader_glsl_print_assignment(gen, &dst, "%s(%s)",
                scalar_constructor, src.str->buffer);

    glsl_src_cleanup(&src, &gen->string_buffers);
    glsl_dst_cleanup(&dst, &gen->string_buffers);
}

static void shader_glsl_end_block(struct vkd3d_glsl_generator *gen)
{
    --gen->indent;
    shader_glsl_print_indent(gen->buffer, gen->indent);
    vkd3d_string_buffer_printf(gen->buffer, "}\n");
}

static void shader_glsl_begin_block(struct vkd3d_glsl_generator *gen)
{
    shader_glsl_print_indent(gen->buffer, gen->indent);
    vkd3d_string_buffer_printf(gen->buffer, "{\n");
    ++gen->indent;
}

static void shader_glsl_if(struct vkd3d_glsl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    const char *condition;
    struct glsl_src src;

    glsl_src_init(&src, gen, &ins->src[0], VKD3DSP_WRITEMASK_0);

    shader_glsl_print_indent(gen->buffer, gen->indent);
    condition = ins->flags == VKD3D_SHADER_CONDITIONAL_OP_NZ ? "bool" : "!bool";
    vkd3d_string_buffer_printf(gen->buffer, "if (%s(%s))\n", condition, src.str->buffer);

    glsl_src_cleanup(&src, &gen->string_buffers);

    shader_glsl_begin_block(gen);
}

static void shader_glsl_else(struct vkd3d_glsl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    shader_glsl_end_block(gen);
    shader_glsl_print_indent(gen->buffer, gen->indent);
    vkd3d_string_buffer_printf(gen->buffer, "else\n");
    shader_glsl_begin_block(gen);
}

static void shader_glsl_loop(struct vkd3d_glsl_generator *gen)
{
    shader_glsl_print_indent(gen->buffer, gen->indent);
    vkd3d_string_buffer_printf(gen->buffer, "for (;;)\n");
    shader_glsl_begin_block(gen);
}

static void shader_glsl_break(struct vkd3d_glsl_generator *gen)
{
    shader_glsl_print_indent(gen->buffer, gen->indent);
    vkd3d_string_buffer_printf(gen->buffer, "break;\n");
}

static void shader_glsl_continue(struct vkd3d_glsl_generator *gen)
{
    shader_glsl_print_indent(gen->buffer, gen->indent);
    vkd3d_string_buffer_printf(gen->buffer, "continue;\n");
}

static void shader_glsl_switch(struct vkd3d_glsl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    struct glsl_src src;

    glsl_src_init(&src, gen, &ins->src[0], VKD3DSP_WRITEMASK_0);

    shader_glsl_print_indent(gen->buffer, gen->indent);
    vkd3d_string_buffer_printf(gen->buffer, "switch (%s)\n", src.str->buffer);
    shader_glsl_begin_block(gen);

    glsl_src_cleanup(&src, &gen->string_buffers);
}

static void shader_glsl_case(struct vkd3d_glsl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    struct glsl_src src;

    glsl_src_init(&src, gen, &ins->src[0], VKD3DSP_WRITEMASK_0);

    shader_glsl_print_indent(gen->buffer, gen->indent);
    vkd3d_string_buffer_printf(gen->buffer, "case %s:\n", src.str->buffer);

    glsl_src_cleanup(&src, &gen->string_buffers);
}

static void shader_glsl_default(struct vkd3d_glsl_generator *gen)
{
    shader_glsl_print_indent(gen->buffer, gen->indent);
    vkd3d_string_buffer_printf(gen->buffer, "default:\n");
}

static void shader_glsl_print_texel_offset(struct vkd3d_string_buffer *buffer, struct vkd3d_glsl_generator *gen,
        unsigned int offset_size, const struct vkd3d_shader_texel_offset *offset)
{
    switch (offset_size)
    {
        case 1:
            vkd3d_string_buffer_printf(buffer, "%d", offset->u);
            break;
        case 2:
            vkd3d_string_buffer_printf(buffer, "ivec2(%d, %d)", offset->u, offset->v);
            break;
        default:
            vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                    "Internal compiler error: Invalid texel offset size %u.", offset_size);
            /* fall through */
        case 3:
            vkd3d_string_buffer_printf(buffer, "ivec3(%d, %d, %d)", offset->u, offset->v, offset->w);
            break;
    }
}

static void shader_glsl_ld(struct vkd3d_glsl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    unsigned int resource_id, resource_idx, resource_space, sample_count;
    const struct glsl_resource_type_info *resource_type_info;
    const struct vkd3d_shader_descriptor_info1 *d;
    enum vkd3d_shader_component_type sampled_type;
    enum vkd3d_shader_resource_type resource_type;
    struct vkd3d_string_buffer *fetch;
    enum vkd3d_data_type data_type;
    struct glsl_src coord;
    struct glsl_dst dst;
    uint32_t coord_mask;

    if (vkd3d_shader_instruction_has_texel_offset(ins))
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Unhandled texel fetch offset.");

    if (ins->src[1].reg.idx[0].rel_addr || ins->src[1].reg.idx[1].rel_addr)
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_UNSUPPORTED,
                "Descriptor indexing is not supported.");

    resource_id = ins->src[1].reg.idx[0].offset;
    resource_idx = ins->src[1].reg.idx[1].offset;
    if ((d = shader_glsl_get_descriptor_by_id(gen, VKD3D_SHADER_DESCRIPTOR_TYPE_SRV, resource_id)))
    {
        resource_type = d->resource_type;
        resource_space = d->register_space;
        sample_count = d->sample_count;
        sampled_type = vkd3d_component_type_from_resource_data_type(d->resource_data_type);
        data_type = vkd3d_data_type_from_component_type(sampled_type);
    }
    else
    {
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Undeclared resource descriptor %u.", resource_id);
        resource_space = 0;
        resource_type = VKD3D_SHADER_RESOURCE_TEXTURE_2D;
        sample_count = 1;
        data_type = VKD3D_DATA_FLOAT;
    }

    if ((resource_type_info = shader_glsl_get_resource_type_info(resource_type)))
    {
        coord_mask = vkd3d_write_mask_from_component_count(resource_type_info->coord_size);
    }
    else
    {
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Unhandled resource type %#x.", resource_type);
        coord_mask = vkd3d_write_mask_from_component_count(2);
    }

    glsl_dst_init(&dst, gen, ins, &ins->dst[0]);
    glsl_src_init(&coord, gen, &ins->src[0], coord_mask);
    fetch = vkd3d_string_buffer_get(&gen->string_buffers);

    vkd3d_string_buffer_printf(fetch, "texelFetch(");
    shader_glsl_print_combined_sampler_name(fetch, gen, resource_idx,
            resource_space, VKD3D_SHADER_DUMMY_SAMPLER_INDEX, 0);
    vkd3d_string_buffer_printf(fetch, ", %s", coord.str->buffer);
    if (resource_type != VKD3D_SHADER_RESOURCE_BUFFER)
    {
        vkd3d_string_buffer_printf(fetch, ", ");
        if (ins->opcode != VKD3DSIH_LD2DMS)
            shader_glsl_print_src(fetch, gen, &ins->src[0], VKD3DSP_WRITEMASK_3, ins->src[0].reg.data_type);
        else if (sample_count == 1)
            /* If the resource isn't a true multisample resource, this is the
             * "lod" parameter instead of the "sample" parameter. */
            vkd3d_string_buffer_printf(fetch, "0");
        else
            shader_glsl_print_src(fetch, gen, &ins->src[2], VKD3DSP_WRITEMASK_0, ins->src[2].reg.data_type);
    }
    vkd3d_string_buffer_printf(fetch, ")");
    shader_glsl_print_swizzle(fetch, ins->src[1].swizzle, ins->dst[0].write_mask);

    shader_glsl_print_assignment_ext(gen, &dst, data_type, "%s", fetch->buffer);

    vkd3d_string_buffer_release(&gen->string_buffers, fetch);
    glsl_src_cleanup(&coord, &gen->string_buffers);
    glsl_dst_cleanup(&dst, &gen->string_buffers);
}

static void shader_glsl_print_shadow_coord(struct vkd3d_string_buffer *buffer, struct vkd3d_glsl_generator *gen,
        const struct vkd3d_shader_src_param *coord, const struct vkd3d_shader_src_param *ref, unsigned int coord_size)
{
    uint32_t coord_mask = vkd3d_write_mask_from_component_count(coord_size);

    switch (coord_size)
    {
        case 1:
            vkd3d_string_buffer_printf(buffer, "vec3(");
            shader_glsl_print_src(buffer, gen, coord, coord_mask, coord->reg.data_type);
            vkd3d_string_buffer_printf(buffer, ", 0.0, ");
            shader_glsl_print_src(buffer, gen, ref, VKD3DSP_WRITEMASK_0, ref->reg.data_type);
            vkd3d_string_buffer_printf(buffer, ")");
            break;

        case 4:
            shader_glsl_print_src(buffer, gen, coord, coord_mask, coord->reg.data_type);
            vkd3d_string_buffer_printf(buffer, ", ");
            shader_glsl_print_src(buffer, gen, ref, VKD3DSP_WRITEMASK_0, ref->reg.data_type);
            break;

        default:
            vkd3d_string_buffer_printf(buffer, "vec%u(", coord_size + 1);
            shader_glsl_print_src(buffer, gen, coord, coord_mask, coord->reg.data_type);
            vkd3d_string_buffer_printf(buffer, ", ");
            shader_glsl_print_src(buffer, gen, ref, VKD3DSP_WRITEMASK_0, ref->reg.data_type);
            vkd3d_string_buffer_printf(buffer, ")");
            break;
    }
}

static void shader_glsl_sample(struct vkd3d_glsl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    bool shadow_sampler, array, bias, dynamic_offset, gather, grad, lod, lod_zero, offset, shadow;
    const struct glsl_resource_type_info *resource_type_info;
    const struct vkd3d_shader_src_param *resource, *sampler;
    unsigned int resource_id, resource_idx, resource_space;
    unsigned int sampler_id, sampler_idx, sampler_space;
    const struct vkd3d_shader_descriptor_info1 *d;
    enum vkd3d_shader_component_type sampled_type;
    enum vkd3d_shader_resource_type resource_type;
    unsigned int component_idx, coord_size;
    struct vkd3d_string_buffer *sample;
    enum vkd3d_data_type data_type;
    struct glsl_dst dst;

    bias = ins->opcode == VKD3DSIH_SAMPLE_B;
    dynamic_offset = ins->opcode == VKD3DSIH_GATHER4_PO;
    gather = ins->opcode == VKD3DSIH_GATHER4 || ins->opcode == VKD3DSIH_GATHER4_PO;
    grad = ins->opcode == VKD3DSIH_SAMPLE_GRAD;
    lod = ins->opcode == VKD3DSIH_SAMPLE_LOD || ins->opcode == VKD3DSIH_SAMPLE_C_LZ;
    lod_zero = ins->opcode == VKD3DSIH_SAMPLE_C_LZ;
    offset = dynamic_offset || vkd3d_shader_instruction_has_texel_offset(ins);
    shadow = ins->opcode == VKD3DSIH_SAMPLE_C || ins->opcode == VKD3DSIH_SAMPLE_C_LZ;

    resource = &ins->src[1 + dynamic_offset];
    sampler = &ins->src[2 + dynamic_offset];

    if (resource->reg.idx[0].rel_addr || resource->reg.idx[1].rel_addr
            || sampler->reg.idx[0].rel_addr || sampler->reg.idx[1].rel_addr)
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_UNSUPPORTED,
                "Descriptor indexing is not supported.");

    resource_id = resource->reg.idx[0].offset;
    resource_idx = resource->reg.idx[1].offset;
    if ((d = shader_glsl_get_descriptor_by_id(gen, VKD3D_SHADER_DESCRIPTOR_TYPE_SRV, resource_id)))
    {
        resource_type = d->resource_type;
        resource_space = d->register_space;
        sampled_type = vkd3d_component_type_from_resource_data_type(d->resource_data_type);
        data_type = vkd3d_data_type_from_component_type(sampled_type);
    }
    else
    {
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Undeclared resource descriptor %u.", resource_id);
        resource_space = 0;
        resource_type = VKD3D_SHADER_RESOURCE_TEXTURE_2D;
        data_type = VKD3D_DATA_FLOAT;
    }

    if ((resource_type_info = shader_glsl_get_resource_type_info(resource_type)))
    {
        coord_size = resource_type_info->coord_size;
        array = resource_type_info->array;
    }
    else
    {
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Unhandled resource type %#x.", resource_type);
        coord_size = 2;
        array = false;
    }

    sampler_id = sampler->reg.idx[0].offset;
    sampler_idx = sampler->reg.idx[1].offset;
    if ((d = shader_glsl_get_descriptor_by_id(gen, VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER, sampler_id)))
    {
        sampler_space = d->register_space;
        shadow_sampler = d->flags & VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_SAMPLER_COMPARISON_MODE;

        if (shadow)
        {
            if (!shadow_sampler)
                vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                        "Internal compiler error: Sampler %u is not a comparison sampler.", sampler_id);
        }
        else
        {
            if (shadow_sampler)
                vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                        "Internal compiler error: Sampler %u is a comparison sampler.", sampler_id);
        }
    }
    else
    {
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Undeclared sampler descriptor %u.", sampler_id);
        sampler_space = 0;
    }

    glsl_dst_init(&dst, gen, ins, &ins->dst[0]);
    sample = vkd3d_string_buffer_get(&gen->string_buffers);

    if (gather)
        vkd3d_string_buffer_printf(sample, "textureGather");
    else if (grad)
        vkd3d_string_buffer_printf(sample, "textureGrad");
    else if (lod)
        vkd3d_string_buffer_printf(sample, "textureLod");
    else
        vkd3d_string_buffer_printf(sample, "texture");
    vkd3d_string_buffer_printf(sample, "%s(", offset ? "Offset" : "");
    shader_glsl_print_combined_sampler_name(sample, gen, resource_idx, resource_space, sampler_idx, sampler_space);
    vkd3d_string_buffer_printf(sample, ", ");
    if (shadow)
        shader_glsl_print_shadow_coord(sample, gen, &ins->src[0], &ins->src[3], coord_size);
    else
        shader_glsl_print_src(sample, gen, &ins->src[0],
                vkd3d_write_mask_from_component_count(coord_size), ins->src[0].reg.data_type);
    if (grad)
    {
        vkd3d_string_buffer_printf(sample, ", ");
        shader_glsl_print_src(sample, gen, &ins->src[3],
                vkd3d_write_mask_from_component_count(coord_size - array), ins->src[3].reg.data_type);
        vkd3d_string_buffer_printf(sample, ", ");
        shader_glsl_print_src(sample, gen, &ins->src[4],
                vkd3d_write_mask_from_component_count(coord_size - array), ins->src[4].reg.data_type);
    }
    else if (lod_zero)
    {
        vkd3d_string_buffer_printf(sample, ", 0.0");
    }
    else if (lod)
    {
        vkd3d_string_buffer_printf(sample, ", ");
        shader_glsl_print_src(sample, gen, &ins->src[3], VKD3DSP_WRITEMASK_0, ins->src[3].reg.data_type);
    }
    if (offset)
    {
        vkd3d_string_buffer_printf(sample, ", ");
        if (dynamic_offset)
            shader_glsl_print_src(sample, gen, &ins->src[1],
                    vkd3d_write_mask_from_component_count(coord_size - array), ins->src[1].reg.data_type);
        else
            shader_glsl_print_texel_offset(sample, gen, coord_size - array, &ins->texel_offset);
    }
    if (bias)
    {
        vkd3d_string_buffer_printf(sample, ", ");
        shader_glsl_print_src(sample, gen, &ins->src[3], VKD3DSP_WRITEMASK_0, ins->src[3].reg.data_type);
    }
    else if (gather)
    {
        if ((component_idx = vsir_swizzle_get_component(sampler->swizzle, 0)))
            vkd3d_string_buffer_printf(sample, ", %d", component_idx);
    }
    vkd3d_string_buffer_printf(sample, ")");
    shader_glsl_print_swizzle(sample, resource->swizzle, ins->dst[0].write_mask);

    shader_glsl_print_assignment_ext(gen, &dst, data_type, "%s", sample->buffer);

    vkd3d_string_buffer_release(&gen->string_buffers, sample);
    glsl_dst_cleanup(&dst, &gen->string_buffers);
}

static void shader_glsl_load_uav_typed(struct vkd3d_glsl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    const struct glsl_resource_type_info *resource_type_info;
    enum vkd3d_shader_component_type component_type;
    const struct vkd3d_shader_descriptor_info1 *d;
    enum vkd3d_shader_resource_type resource_type;
    unsigned int uav_id, uav_idx, uav_space;
    struct vkd3d_string_buffer *load;
    struct glsl_src coord;
    struct glsl_dst dst;
    uint32_t coord_mask;

    if (ins->src[1].reg.idx[0].rel_addr || ins->src[1].reg.idx[1].rel_addr)
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_UNSUPPORTED,
                "Descriptor indexing is not supported.");

    uav_id = ins->src[1].reg.idx[0].offset;
    uav_idx = ins->src[1].reg.idx[1].offset;
    if ((d = shader_glsl_get_descriptor_by_id(gen, VKD3D_SHADER_DESCRIPTOR_TYPE_UAV, uav_id)))
    {
        resource_type = d->resource_type;
        uav_space = d->register_space;
        component_type = vkd3d_component_type_from_resource_data_type(d->resource_data_type);
    }
    else
    {
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Undeclared UAV descriptor %u.", uav_id);
        uav_space = 0;
        resource_type = VKD3D_SHADER_RESOURCE_TEXTURE_2D;
        component_type = VKD3D_SHADER_COMPONENT_FLOAT;
    }

    if ((resource_type_info = shader_glsl_get_resource_type_info(resource_type)))
    {
        coord_mask = vkd3d_write_mask_from_component_count(resource_type_info->coord_size);
    }
    else
    {
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Unhandled UAV type %#x.", resource_type);
        coord_mask = vkd3d_write_mask_from_component_count(2);
    }

    glsl_dst_init(&dst, gen, ins, &ins->dst[0]);
    glsl_src_init(&coord, gen, &ins->src[0], coord_mask);
    load = vkd3d_string_buffer_get(&gen->string_buffers);

    vkd3d_string_buffer_printf(load, "imageLoad(");
    shader_glsl_print_image_name(load, gen, uav_idx, uav_space);
    vkd3d_string_buffer_printf(load, ", %s)", coord.str->buffer);
    shader_glsl_print_swizzle(load, ins->src[1].swizzle, ins->dst[0].write_mask);

    shader_glsl_print_assignment_ext(gen, &dst,
            vkd3d_data_type_from_component_type(component_type), "%s", load->buffer);

    vkd3d_string_buffer_release(&gen->string_buffers, load);
    glsl_src_cleanup(&coord, &gen->string_buffers);
    glsl_dst_cleanup(&dst, &gen->string_buffers);
}

static void shader_glsl_store_uav_typed(struct vkd3d_glsl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    const struct glsl_resource_type_info *resource_type_info;
    enum vkd3d_shader_component_type component_type;
    const struct vkd3d_shader_descriptor_info1 *d;
    enum vkd3d_shader_resource_type resource_type;
    unsigned int uav_id, uav_idx, uav_space;
    struct vkd3d_string_buffer *image_data;
    struct glsl_src image_coord;
    uint32_t coord_mask;

    if (ins->dst[0].reg.idx[0].rel_addr || ins->dst[0].reg.idx[1].rel_addr)
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_UNSUPPORTED,
                "Descriptor indexing is not supported.");

    uav_id = ins->dst[0].reg.idx[0].offset;
    uav_idx = ins->dst[0].reg.idx[1].offset;
    if ((d = shader_glsl_get_descriptor_by_id(gen, VKD3D_SHADER_DESCRIPTOR_TYPE_UAV, uav_id)))
    {
        resource_type = d->resource_type;
        uav_space = d->register_space;
        component_type = vkd3d_component_type_from_resource_data_type(d->resource_data_type);
    }
    else
    {
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Undeclared UAV descriptor %u.", uav_id);
        uav_space = 0;
        resource_type = VKD3D_SHADER_RESOURCE_TEXTURE_2D;
        component_type = VKD3D_SHADER_COMPONENT_FLOAT;
    }

    if ((resource_type_info = shader_glsl_get_resource_type_info(resource_type)))
    {
        coord_mask = vkd3d_write_mask_from_component_count(resource_type_info->coord_size);
    }
    else
    {
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Unhandled UAV type %#x.", resource_type);
        coord_mask = vkd3d_write_mask_from_component_count(2);
    }

    glsl_src_init(&image_coord, gen, &ins->src[0], coord_mask);
    image_data = vkd3d_string_buffer_get(&gen->string_buffers);

    if (ins->src[1].reg.dimension == VSIR_DIMENSION_SCALAR)
    {
        switch (component_type)
        {
            case VKD3D_SHADER_COMPONENT_UINT:
                vkd3d_string_buffer_printf(image_data, "uvec4(");
                break;
            case VKD3D_SHADER_COMPONENT_INT:
                vkd3d_string_buffer_printf(image_data, "ivec4(");
                break;
            default:
                vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                        "Internal compiler error: Unhandled component type %#x.", component_type);
                /* fall through */
            case VKD3D_SHADER_COMPONENT_FLOAT:
                vkd3d_string_buffer_printf(image_data, "vec4(");
                break;
        }
    }
    shader_glsl_print_src(image_data, gen, &ins->src[1], VKD3DSP_WRITEMASK_ALL,
            vkd3d_data_type_from_component_type(component_type));
    if (ins->src[1].reg.dimension == VSIR_DIMENSION_SCALAR)
        vkd3d_string_buffer_printf(image_data, ", 0, 0, 0)");

    shader_glsl_print_indent(gen->buffer, gen->indent);
    vkd3d_string_buffer_printf(gen->buffer, "imageStore(");
    shader_glsl_print_image_name(gen->buffer, gen, uav_idx, uav_space);
    vkd3d_string_buffer_printf(gen->buffer, ", %s, %s);\n", image_coord.str->buffer, image_data->buffer);

    vkd3d_string_buffer_release(&gen->string_buffers, image_data);
    glsl_src_cleanup(&image_coord, &gen->string_buffers);
}

static void shader_glsl_unary_op(struct vkd3d_glsl_generator *gen,
        const struct vkd3d_shader_instruction *ins, const char *op)
{
    struct glsl_src src;
    struct glsl_dst dst;
    uint32_t mask;

    mask = glsl_dst_init(&dst, gen, ins, &ins->dst[0]);
    glsl_src_init(&src, gen, &ins->src[0], mask);

    shader_glsl_print_assignment(gen, &dst, "%s%s", op, src.str->buffer);

    glsl_src_cleanup(&src, &gen->string_buffers);
    glsl_dst_cleanup(&dst, &gen->string_buffers);
}

static void shader_glsl_mov(struct vkd3d_glsl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    struct glsl_src src;
    struct glsl_dst dst;
    uint32_t mask;

    mask = glsl_dst_init(&dst, gen, ins, &ins->dst[0]);
    glsl_src_init(&src, gen, &ins->src[0], mask);

    shader_glsl_print_assignment(gen, &dst, "%s", src.str->buffer);

    glsl_src_cleanup(&src, &gen->string_buffers);
    glsl_dst_cleanup(&dst, &gen->string_buffers);
}

static void shader_glsl_movc(struct vkd3d_glsl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    unsigned int component_count;
    struct glsl_src src[3];
    struct glsl_dst dst;
    uint32_t mask;

    mask = glsl_dst_init(&dst, gen, ins, &ins->dst[0]);
    glsl_src_init(&src[0], gen, &ins->src[0], mask);
    glsl_src_init(&src[1], gen, &ins->src[1], mask);
    glsl_src_init(&src[2], gen, &ins->src[2], mask);

    if ((component_count = vsir_write_mask_component_count(mask)) > 1)
        shader_glsl_print_assignment(gen, &dst, "mix(%s, %s, bvec%u(%s))",
                src[2].str->buffer, src[1].str->buffer, component_count, src[0].str->buffer);
    else
        shader_glsl_print_assignment(gen, &dst, "mix(%s, %s, bool(%s))",
                src[2].str->buffer, src[1].str->buffer, src[0].str->buffer);

    glsl_src_cleanup(&src[2], &gen->string_buffers);
    glsl_src_cleanup(&src[1], &gen->string_buffers);
    glsl_src_cleanup(&src[0], &gen->string_buffers);
    glsl_dst_cleanup(&dst, &gen->string_buffers);
}

static void shader_glsl_mul_extended(struct vkd3d_glsl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    struct glsl_src src[2];
    struct glsl_dst dst;
    uint32_t mask;

    if (ins->dst[0].reg.type != VKD3DSPR_NULL)
    {
        /* FIXME: imulExtended()/umulExtended() from ARB_gpu_shader5/GLSL 4.00+. */
        mask = glsl_dst_init(&dst, gen, ins, &ins->dst[0]);
        shader_glsl_print_assignment(gen, &dst, "<unhandled 64-bit multiplication>");
        glsl_dst_cleanup(&dst, &gen->string_buffers);

        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Unhandled 64-bit integer multiplication.");
    }

    if (ins->dst[1].reg.type != VKD3DSPR_NULL)
    {
        mask = glsl_dst_init(&dst, gen, ins, &ins->dst[1]);
        glsl_src_init(&src[0], gen, &ins->src[0], mask);
        glsl_src_init(&src[1], gen, &ins->src[1], mask);

        shader_glsl_print_assignment(gen, &dst, "%s * %s", src[0].str->buffer, src[1].str->buffer);

        glsl_src_cleanup(&src[1], &gen->string_buffers);
        glsl_src_cleanup(&src[0], &gen->string_buffers);
        glsl_dst_cleanup(&dst, &gen->string_buffers);
    }
}

static void shader_glsl_print_sysval_name(struct vkd3d_string_buffer *buffer, struct vkd3d_glsl_generator *gen,
        enum vkd3d_shader_sysval_semantic sysval, unsigned int idx)
{
    const struct vkd3d_shader_version *version = &gen->program->shader_version;

    switch (sysval)
    {
        case VKD3D_SHADER_SV_POSITION:
            if (version->type == VKD3D_SHADER_TYPE_COMPUTE)
            {
                vkd3d_string_buffer_printf(buffer, "<unhandled sysval %#x>", sysval);
                vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                        "Internal compiler error: Unhandled SV_POSITION in shader type #%x.", version->type);
                break;
            }
            if (idx)
                vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                        "Internal compiler error: Unhandled SV_POSITION index %u.", idx);
            if (version->type == VKD3D_SHADER_TYPE_PIXEL)
                vkd3d_string_buffer_printf(buffer, "gl_FragCoord");
            else
                vkd3d_string_buffer_printf(buffer, "gl_Position");
            break;

        case VKD3D_SHADER_SV_VERTEX_ID:
            if (version->type != VKD3D_SHADER_TYPE_VERTEX)
                vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                        "Internal compiler error: Unhandled SV_VERTEX_ID in shader type #%x.", version->type);
            vkd3d_string_buffer_printf(buffer, "intBitsToFloat(ivec4(gl_VertexID, 0, 0, 0))");
            break;

        case VKD3D_SHADER_SV_IS_FRONT_FACE:
            if (version->type != VKD3D_SHADER_TYPE_PIXEL)
                vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                        "Internal compiler error: Unhandled SV_IS_FRONT_FACE in shader type #%x.", version->type);
            vkd3d_string_buffer_printf(buffer,
                    "uintBitsToFloat(uvec4(gl_FrontFacing ? 0xffffffffu : 0u, 0u, 0u, 0u))");
            break;

        case VKD3D_SHADER_SV_SAMPLE_INDEX:
            if (version->type != VKD3D_SHADER_TYPE_PIXEL)
                vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                        "Internal compiler error: Unhandled SV_SAMPLE_INDEX in shader type #%x.", version->type);
            vkd3d_string_buffer_printf(buffer, "intBitsToFloat(ivec4(gl_SampleID, 0, 0, 0))");
            break;

        case VKD3D_SHADER_SV_TARGET:
            if (version->type != VKD3D_SHADER_TYPE_PIXEL)
                vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                        "Internal compiler error: Unhandled SV_TARGET in shader type #%x.", version->type);
            vkd3d_string_buffer_printf(buffer, "shader_out_%u", idx);
            break;

        default:
            vkd3d_string_buffer_printf(buffer, "<unhandled sysval %#x>", sysval);
            vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                    "Internal compiler error: Unhandled system value %#x.", sysval);
            break;
    }
}

static void shader_glsl_shader_prologue(struct vkd3d_glsl_generator *gen)
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

        shader_glsl_print_indent(buffer, gen->indent);
        vkd3d_string_buffer_printf(buffer, "%s_in[%u]", gen->prefix, e->register_index);
        shader_glsl_print_write_mask(buffer, e->mask);
        if (e->sysval_semantic == VKD3D_SHADER_SV_NONE)
        {
            if (gen->interstage_input)
            {
                vkd3d_string_buffer_printf(buffer, " = shader_in.reg_%u", e->target_location);
                if (e->target_location >= gen->limits.input_count)
                    vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                            "Internal compiler error: Input element %u specifies target location %u, "
                            "but only %u inputs are supported.",
                            i, e->target_location, gen->limits.input_count);
            }
            else
            {
                switch (e->component_type)
                {
                    case VKD3D_SHADER_COMPONENT_UINT:
                        vkd3d_string_buffer_printf(buffer, " = uintBitsToFloat(shader_in_%u)", i);
                        break;
                    case VKD3D_SHADER_COMPONENT_INT:
                        vkd3d_string_buffer_printf(buffer, " = intBitsToFloat(shader_in_%u)", i);
                        break;
                    default:
                        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                                "Internal compiler error: Unhandled input component type %#x.", e->component_type);
                        /* fall through */
                    case VKD3D_SHADER_COMPONENT_FLOAT:
                        vkd3d_string_buffer_printf(buffer, " = shader_in_%u", i);
                        break;
                }
            }
        }
        else
        {
            vkd3d_string_buffer_printf(buffer, " = ");
            shader_glsl_print_sysval_name(buffer, gen, e->sysval_semantic, e->semantic_index);
        }
        shader_glsl_print_write_mask(buffer, e->mask);
        vkd3d_string_buffer_printf(buffer, ";\n");
    }
}

static void shader_glsl_shader_epilogue(struct vkd3d_glsl_generator *gen)
{
    const struct shader_signature *signature = &gen->program->output_signature;
    struct vkd3d_string_buffer *buffer = gen->buffer;
    enum vkd3d_shader_component_type type;
    const struct signature_element *e;
    unsigned int i;

    for (i = 0; i < signature->element_count; ++i)
    {
        e = &signature->elements[i];

        if (e->target_location == SIGNATURE_TARGET_LOCATION_UNUSED)
            continue;

        type = e->component_type;
        shader_glsl_print_indent(buffer, gen->indent);
        if (e->sysval_semantic == VKD3D_SHADER_SV_NONE)
        {
            if (gen->interstage_output)
            {
                type = VKD3D_SHADER_COMPONENT_FLOAT;
                vkd3d_string_buffer_printf(buffer, "shader_out.reg_%u", e->target_location);
                if (e->target_location >= gen->limits.output_count)
                    vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                            "Internal compiler error: Output element %u specifies target location %u, "
                            "but only %u outputs are supported.",
                            i, e->target_location, gen->limits.output_count);
            }
            else
            {
                vkd3d_string_buffer_printf(buffer, "<unhandled output %u>", e->target_location);
                vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                        "Internal compiler error: Unhandled output.");
            }
        }
        else
        {
            shader_glsl_print_sysval_name(buffer, gen, e->sysval_semantic, e->semantic_index);
        }
        shader_glsl_print_write_mask(buffer, e->mask);
        switch (type)
        {
            case VKD3D_SHADER_COMPONENT_UINT:
                vkd3d_string_buffer_printf(buffer, " = floatBitsToUint(%s_out[%u])", gen->prefix, e->register_index);
                break;
            case VKD3D_SHADER_COMPONENT_INT:
                vkd3d_string_buffer_printf(buffer, " = floatBitsToInt(%s_out[%u])", gen->prefix, e->register_index);
                break;
            default:
                vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                        "Internal compiler error: Unhandled output component type %#x.", e->component_type);
                /* fall through */
            case VKD3D_SHADER_COMPONENT_FLOAT:
                vkd3d_string_buffer_printf(buffer, " = %s_out[%u]", gen->prefix, e->register_index);
                break;
        }
        shader_glsl_print_write_mask(buffer, e->mask);
        vkd3d_string_buffer_printf(buffer, ";\n");
    }
}

static void shader_glsl_ret(struct vkd3d_glsl_generator *gen, const struct vkd3d_shader_instruction *ins)
{
    const struct vkd3d_shader_version *version = &gen->program->shader_version;

    if (version->major >= 4)
    {
        shader_glsl_shader_epilogue(gen);
        shader_glsl_print_indent(gen->buffer, gen->indent);
        vkd3d_string_buffer_printf(gen->buffer, "return;\n");
    }
}

static void shader_glsl_dcl_indexable_temp(struct vkd3d_glsl_generator *gen,
        const struct vkd3d_shader_instruction *ins)
{
    shader_glsl_print_indent(gen->buffer, gen->indent);
    vkd3d_string_buffer_printf(gen->buffer, "vec4 x%u[%u];\n",
            ins->declaration.indexable_temp.register_idx,
            ins->declaration.indexable_temp.register_size);
}

static void vkd3d_glsl_handle_instruction(struct vkd3d_glsl_generator *gen,
        const struct vkd3d_shader_instruction *ins)
{
    gen->location = ins->location;

    switch (ins->opcode)
    {
        case VKD3DSIH_ADD:
        case VKD3DSIH_IADD:
            shader_glsl_binop(gen, ins, "+");
            break;
        case VKD3DSIH_AND:
            shader_glsl_binop(gen, ins, "&");
            break;
        case VKD3DSIH_BREAK:
            shader_glsl_break(gen);
            break;
        case VKD3DSIH_CASE:
            shader_glsl_case(gen, ins);
            break;
        case VKD3DSIH_CONTINUE:
            shader_glsl_continue(gen);
            break;
        case VKD3DSIH_DCL_INDEXABLE_TEMP:
            shader_glsl_dcl_indexable_temp(gen, ins);
            break;
        case VKD3DSIH_DCL_INPUT:
        case VKD3DSIH_DCL_INPUT_PS:
        case VKD3DSIH_DCL_INPUT_PS_SGV:
        case VKD3DSIH_DCL_INPUT_PS_SIV:
        case VKD3DSIH_DCL_INPUT_SGV:
        case VKD3DSIH_DCL_OUTPUT:
        case VKD3DSIH_DCL_OUTPUT_SIV:
        case VKD3DSIH_NOP:
            break;
        case VKD3DSIH_DEFAULT:
            shader_glsl_default(gen);
            break;
        case VKD3DSIH_DIV:
            shader_glsl_binop(gen, ins, "/");
            break;
        case VKD3DSIH_DP2:
            shader_glsl_dot(gen, ins, vkd3d_write_mask_from_component_count(2));
            break;
        case VKD3DSIH_DP3:
            shader_glsl_dot(gen, ins, vkd3d_write_mask_from_component_count(3));
            break;
        case VKD3DSIH_DP4:
            shader_glsl_dot(gen, ins, VKD3DSP_WRITEMASK_ALL);
            break;
        case VKD3DSIH_ELSE:
            shader_glsl_else(gen, ins);
            break;
        case VKD3DSIH_ENDIF:
        case VKD3DSIH_ENDLOOP:
        case VKD3DSIH_ENDSWITCH:
            shader_glsl_end_block(gen);
            break;
        case VKD3DSIH_EQO:
        case VKD3DSIH_IEQ:
            shader_glsl_relop(gen, ins, "==", "equal");
            break;
        case VKD3DSIH_EXP:
            shader_glsl_intrinsic(gen, ins, "exp2");
            break;
        case VKD3DSIH_FRC:
            shader_glsl_intrinsic(gen, ins, "fract");
            break;
        case VKD3DSIH_FTOI:
            shader_glsl_cast(gen, ins, "int", "ivec");
            break;
        case VKD3DSIH_FTOU:
            shader_glsl_cast(gen, ins, "uint", "uvec");
            break;
        case VKD3DSIH_GATHER4:
        case VKD3DSIH_GATHER4_PO:
        case VKD3DSIH_SAMPLE:
        case VKD3DSIH_SAMPLE_B:
        case VKD3DSIH_SAMPLE_C:
        case VKD3DSIH_SAMPLE_C_LZ:
        case VKD3DSIH_SAMPLE_GRAD:
        case VKD3DSIH_SAMPLE_LOD:
            shader_glsl_sample(gen, ins);
            break;
        case VKD3DSIH_GEO:
        case VKD3DSIH_IGE:
            shader_glsl_relop(gen, ins, ">=", "greaterThanEqual");
            break;
        case VKD3DSIH_IF:
            shader_glsl_if(gen, ins);
            break;
        case VKD3DSIH_MAD:
            shader_glsl_intrinsic(gen, ins, "fma");
            break;
        case VKD3DSIH_ILT:
        case VKD3DSIH_LTO:
        case VKD3DSIH_ULT:
            shader_glsl_relop(gen, ins, "<", "lessThan");
            break;
        case VKD3DSIH_IMAX:
        case VKD3DSIH_MAX:
        case VKD3DSIH_UMAX:
            shader_glsl_intrinsic(gen, ins, "max");
            break;
        case VKD3DSIH_MIN:
        case VKD3DSIH_UMIN:
            shader_glsl_intrinsic(gen, ins, "min");
            break;
        case VKD3DSIH_IMUL:
            shader_glsl_mul_extended(gen, ins);
            break;
        case VKD3DSIH_INE:
        case VKD3DSIH_NEU:
            shader_glsl_relop(gen, ins, "!=", "notEqual");
            break;
        case VKD3DSIH_INEG:
            shader_glsl_unary_op(gen, ins, "-");
            break;
        case VKD3DSIH_ISHL:
            shader_glsl_binop(gen, ins, "<<");
            break;
        case VKD3DSIH_ISHR:
        case VKD3DSIH_USHR:
            shader_glsl_binop(gen, ins, ">>");
            break;
        case VKD3DSIH_ITOF:
        case VKD3DSIH_UTOF:
            shader_glsl_cast(gen, ins, "float", "vec");
            break;
        case VKD3DSIH_LD:
        case VKD3DSIH_LD2DMS:
            shader_glsl_ld(gen, ins);
            break;
        case VKD3DSIH_LD_UAV_TYPED:
            shader_glsl_load_uav_typed(gen, ins);
            break;
        case VKD3DSIH_LOG:
            shader_glsl_intrinsic(gen, ins, "log2");
            break;
        case VKD3DSIH_LOOP:
            shader_glsl_loop(gen);
            break;
        case VKD3DSIH_MOV:
            shader_glsl_mov(gen, ins);
            break;
        case VKD3DSIH_MOVC:
            shader_glsl_movc(gen, ins);
            break;
        case VKD3DSIH_MUL:
            shader_glsl_binop(gen, ins, "*");
            break;
        case VKD3DSIH_NOT:
            shader_glsl_unary_op(gen, ins, "~");
            break;
        case VKD3DSIH_OR:
            shader_glsl_binop(gen, ins, "|");
            break;
        case VKD3DSIH_RET:
            shader_glsl_ret(gen, ins);
            break;
        case VKD3DSIH_ROUND_NE:
            shader_glsl_intrinsic(gen, ins, "roundEven");
            break;
        case VKD3DSIH_ROUND_NI:
            shader_glsl_intrinsic(gen, ins, "floor");
            break;
        case VKD3DSIH_ROUND_PI:
            shader_glsl_intrinsic(gen, ins, "ceil");
            break;
        case VKD3DSIH_ROUND_Z:
            shader_glsl_intrinsic(gen, ins, "trunc");
            break;
        case VKD3DSIH_RSQ:
            shader_glsl_intrinsic(gen, ins, "inversesqrt");
            break;
        case VKD3DSIH_SQRT:
            shader_glsl_intrinsic(gen, ins, "sqrt");
            break;
        case VKD3DSIH_STORE_UAV_TYPED:
            shader_glsl_store_uav_typed(gen, ins);
            break;
        case VKD3DSIH_SWITCH:
            shader_glsl_switch(gen, ins);
            break;
        default:
            shader_glsl_unhandled(gen, ins);
            break;
    }
}

static bool shader_glsl_check_shader_visibility(const struct vkd3d_glsl_generator *gen,
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

static bool shader_glsl_get_uav_binding(const struct vkd3d_glsl_generator *gen, unsigned int register_space,
        unsigned int register_idx, enum vkd3d_shader_resource_type resource_type, unsigned int *binding_idx)
{
    const struct vkd3d_shader_interface_info *interface_info = gen->interface_info;
    const struct vkd3d_shader_resource_binding *binding;
    enum vkd3d_shader_binding_flag resource_type_flag;
    unsigned int i;

    if (!interface_info)
        return false;

    resource_type_flag = resource_type == VKD3D_SHADER_RESOURCE_BUFFER
            ? VKD3D_SHADER_BINDING_FLAG_BUFFER : VKD3D_SHADER_BINDING_FLAG_IMAGE;

    for (i = 0; i < interface_info->binding_count; ++i)
    {
        binding = &interface_info->bindings[i];

        if (binding->type != VKD3D_SHADER_DESCRIPTOR_TYPE_UAV)
            continue;
        if (binding->register_space != register_space)
            continue;
        if (binding->register_index != register_idx)
            continue;
        if (!shader_glsl_check_shader_visibility(gen, binding->shader_visibility))
            continue;
        if (!(binding->flags & resource_type_flag))
            continue;
        *binding_idx = i;
        return true;
    }

    return false;
}

static void shader_glsl_generate_uav_declaration(struct vkd3d_glsl_generator *gen,
        const struct vkd3d_shader_descriptor_info1 *uav)
{
    const struct glsl_resource_type_info *resource_type_info;
    const char *image_type_prefix, *image_type, *read_format;
    const struct vkd3d_shader_descriptor_binding *binding;
    const struct vkd3d_shader_descriptor_offset *offset;
    struct vkd3d_string_buffer *buffer = gen->buffer;
    enum vkd3d_shader_component_type component_type;
    unsigned int binding_idx;

    if (uav->count != 1)
    {
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_UNSUPPORTED,
                "UAV %u has unsupported descriptor array size %u.", uav->register_id, uav->count);
        return;
    }

    if (!shader_glsl_get_uav_binding(gen, uav->register_space,
            uav->register_index, uav->resource_type, &binding_idx))
    {
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_BINDING_NOT_FOUND,
                "No descriptor binding specified for UAV %u.", uav->register_id);
        return;
    }

    binding = &gen->interface_info->bindings[binding_idx].binding;

    if (binding->set != 0)
    {
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_BINDING_NOT_FOUND,
                "Unsupported binding set %u specified for UAV %u.", binding->set, uav->register_id);
        return;
    }

    if (binding->count != 1)
    {
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_BINDING_NOT_FOUND,
                "Unsupported binding count %u specified for UAV %u.", binding->count, uav->register_id);
        return;
    }

    if (gen->offset_info && gen->offset_info->binding_offsets)
    {
        offset = &gen->offset_info->binding_offsets[binding_idx];
        if (offset->static_offset || offset->dynamic_offset_index != ~0u)
        {
            vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                    "Internal compiler error: Unhandled descriptor offset specified for UAV %u.",
                    uav->register_id);
            return;
        }
    }

    if ((resource_type_info = shader_glsl_get_resource_type_info(uav->resource_type)))
    {
        image_type = resource_type_info->type_suffix;
    }
    else
    {
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Unhandled UAV type %#x.", uav->resource_type);
        image_type = "<unhandled image type>";
    }

    switch ((component_type = vkd3d_component_type_from_resource_data_type(uav->resource_data_type)))
    {
        case VKD3D_SHADER_COMPONENT_UINT:
            image_type_prefix = "u";
            read_format = "r32ui";
            break;
        case VKD3D_SHADER_COMPONENT_INT:
            image_type_prefix = "i";
            read_format = "r32i";
            break;
        default:
            vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                    "Internal compiler error: Unhandled component type %#x for UAV %u.",
                    component_type, uav->register_id);
            /* fall through */
        case VKD3D_SHADER_COMPONENT_FLOAT:
            image_type_prefix = "";
            read_format = "r32f";
            break;
    }

    vkd3d_string_buffer_printf(buffer, "layout(binding = %u", binding->binding);
    if (uav->flags & VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_UAV_READ)
        vkd3d_string_buffer_printf(buffer, ", %s) ", read_format);
    else
        vkd3d_string_buffer_printf(buffer, ") writeonly ");
    vkd3d_string_buffer_printf(buffer, "uniform %simage%s ", image_type_prefix, image_type);
    shader_glsl_print_image_name(buffer, gen, uav->register_index, uav->register_space);
    vkd3d_string_buffer_printf(buffer, ";\n");
}

static bool shader_glsl_get_cbv_binding(const struct vkd3d_glsl_generator *gen,
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
        if (!shader_glsl_check_shader_visibility(gen, binding->shader_visibility))
            continue;
        if (!(binding->flags & VKD3D_SHADER_BINDING_FLAG_BUFFER))
            continue;
        *binding_idx = i;
        return true;
    }

    return false;
}

static void shader_glsl_generate_cbv_declaration(struct vkd3d_glsl_generator *gen,
        const struct vkd3d_shader_descriptor_info1 *cbv)
{
    const struct vkd3d_shader_descriptor_binding *binding;
    const struct vkd3d_shader_descriptor_offset *offset;
    struct vkd3d_string_buffer *buffer = gen->buffer;
    const char *prefix = gen->prefix;
    unsigned int binding_idx;
    size_t size;

    if (cbv->count != 1)
    {
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_UNSUPPORTED,
                "Constant buffer %u has unsupported descriptor array size %u.", cbv->register_id, cbv->count);
        return;
    }

    if (!shader_glsl_get_cbv_binding(gen, cbv->register_space, cbv->register_index, &binding_idx))
    {
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_BINDING_NOT_FOUND,
                "No descriptor binding specified for constant buffer %u.", cbv->register_id);
        return;
    }

    binding = &gen->interface_info->bindings[binding_idx].binding;

    if (binding->set != 0)
    {
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_BINDING_NOT_FOUND,
                "Unsupported binding set %u specified for constant buffer %u.", binding->set, cbv->register_id);
        return;
    }

    if (binding->count != 1)
    {
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_BINDING_NOT_FOUND,
                "Unsupported binding count %u specified for constant buffer %u.", binding->count, cbv->register_id);
        return;
    }

    if (gen->offset_info && gen->offset_info->binding_offsets)
    {
        offset = &gen->offset_info->binding_offsets[binding_idx];
        if (offset->static_offset || offset->dynamic_offset_index != ~0u)
        {
            vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                    "Internal compiler error: Unhandled descriptor offset specified for constant buffer %u.",
                    cbv->register_id);
            return;
        }
    }

    size = align(cbv->buffer_size, VKD3D_VEC4_SIZE * sizeof(uint32_t));
    size /= VKD3D_VEC4_SIZE * sizeof(uint32_t);

    vkd3d_string_buffer_printf(buffer,
            "layout(std140, binding = %u) uniform block_%s_cb_%u { vec4 %s_cb_%u[%zu]; };\n",
            binding->binding, prefix, cbv->register_id, prefix, cbv->register_id, size);
}

static bool shader_glsl_get_combined_sampler_binding(const struct vkd3d_glsl_generator *gen,
        const struct vkd3d_shader_combined_resource_sampler_info *crs,
        enum vkd3d_shader_resource_type resource_type, unsigned int *binding_idx)
{
    const struct vkd3d_shader_interface_info *interface_info = gen->interface_info;
    const struct vkd3d_shader_combined_resource_sampler *s;
    enum vkd3d_shader_binding_flag resource_type_flag;
    unsigned int i;

    if (!interface_info)
        return false;

    resource_type_flag = resource_type == VKD3D_SHADER_RESOURCE_BUFFER
            ? VKD3D_SHADER_BINDING_FLAG_BUFFER : VKD3D_SHADER_BINDING_FLAG_IMAGE;

    for (i = 0; i < interface_info->combined_sampler_count; ++i)
    {
        s = &interface_info->combined_samplers[i];

        if (s->resource_space != crs->resource_space)
            continue;
        if (s->resource_index != crs->resource_index)
            continue;
        if (crs->sampler_index != VKD3D_SHADER_DUMMY_SAMPLER_INDEX)
        {
            if (s->sampler_space != crs->sampler_space)
                continue;
            if (s->sampler_index != crs->sampler_index)
                continue;
        }
        if (!shader_glsl_check_shader_visibility(gen, s->shader_visibility))
            continue;
        if (!(s->flags & resource_type_flag))
            continue;
        *binding_idx = i;
        return true;
    }

    return false;
}

static void shader_glsl_generate_sampler_declaration(struct vkd3d_glsl_generator *gen,
        const struct vkd3d_shader_combined_resource_sampler_info *crs)
{
    const struct vkd3d_shader_descriptor_info1 *sampler, *srv;
    const struct glsl_resource_type_info *resource_type_info;
    const struct vkd3d_shader_descriptor_binding *binding;
    struct vkd3d_string_buffer *buffer = gen->buffer;
    enum vkd3d_shader_component_type component_type;
    const char *sampler_type, *sampler_type_prefix;
    enum vkd3d_shader_resource_type resource_type;
    unsigned int binding_idx;
    bool shadow = false;

    if (crs->sampler_index != VKD3D_SHADER_DUMMY_SAMPLER_INDEX)
    {
        if (!(sampler = shader_glsl_get_descriptor(gen, VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER,
                crs->sampler_index, crs->sampler_space)))
        {
            vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                    "Internal compiler error: No descriptor found for sampler %u, space %u.",
                    crs->sampler_index, crs->sampler_space);
            return;
        }
        shadow = sampler->flags & VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_SAMPLER_COMPARISON_MODE;
    }

    if (!(srv = shader_glsl_get_descriptor(gen, VKD3D_SHADER_DESCRIPTOR_TYPE_SRV,
            crs->resource_index, crs->resource_space)))
    {
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: No descriptor found for resource %u, space %u.",
                crs->resource_index, crs->resource_space);
        return;
    }

    resource_type = srv->resource_type;
    if (srv->sample_count == 1)
    {
        /* The OpenGL API distinguishes between multi-sample textures with
         * sample count 1 and single-sample textures. Direct3D and Vulkan
         * don't make this distinction at the API level, but Direct3D shaders
         * are capable of expressing both. We therefore map such multi-sample
         * textures to their single-sample equivalents here. */
        if (resource_type == VKD3D_SHADER_RESOURCE_TEXTURE_2DMS)
            resource_type = VKD3D_SHADER_RESOURCE_TEXTURE_2D;
        else if (resource_type == VKD3D_SHADER_RESOURCE_TEXTURE_2DMSARRAY)
            resource_type = VKD3D_SHADER_RESOURCE_TEXTURE_2DARRAY;
    }

    if ((resource_type_info = shader_glsl_get_resource_type_info(resource_type)))
    {
        sampler_type = resource_type_info->type_suffix;
        if (shadow && !resource_type_info->shadow)
            vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_UNSUPPORTED,
                    "Comparison samplers are not supported with resource type %#x.", resource_type);
    }
    else
    {
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Unhandled resource type %#x for combined resource/sampler "
                "for resource %u, space %u and sampler %u, space %u.", resource_type,
                crs->resource_index, crs->resource_space, crs->sampler_index, crs->sampler_space);
        sampler_type = "<unhandled sampler type>";
    }

    switch ((component_type = vkd3d_component_type_from_resource_data_type(srv->resource_data_type)))
    {
        case VKD3D_SHADER_COMPONENT_UINT:
            sampler_type_prefix = "u";
            break;
        case VKD3D_SHADER_COMPONENT_INT:
            sampler_type_prefix = "i";
            break;
        case VKD3D_SHADER_COMPONENT_FLOAT:
            sampler_type_prefix = "";
            break;
        default:
            vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                    "Internal compiler error: Unhandled component type %#x for combined resource/sampler "
                    "for resource %u, space %u and sampler %u, space %u.", component_type,
                    crs->resource_index, crs->resource_space, crs->sampler_index, crs->sampler_space);
            sampler_type_prefix = "";
            break;
    }

    if (!shader_glsl_get_combined_sampler_binding(gen, crs, resource_type, &binding_idx))
    {
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_BINDING_NOT_FOUND,
                "No descriptor binding specified for combined resource/sampler "
                "for resource %u, space %u and sampler %u, space %u.",
                crs->resource_index, crs->resource_space, crs->sampler_index, crs->sampler_space);
        return;
    }

    binding = &gen->interface_info->combined_samplers[binding_idx].binding;

    if (binding->set != 0)
    {
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_BINDING_NOT_FOUND,
                "Unsupported binding set %u specified for combined resource/sampler "
                "for resource %u, space %u and sampler %u, space %u.", binding->set,
                crs->resource_index, crs->resource_space, crs->sampler_index, crs->sampler_space);
        return;
    }

    if (binding->count != 1)
    {
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_BINDING_NOT_FOUND,
                "Unsupported binding count %u specified for combined resource/sampler "
                "for resource %u, space %u and sampler %u, space %u.", binding->count,
                crs->resource_index, crs->resource_space, crs->sampler_index, crs->sampler_space);
        return;
    }

    vkd3d_string_buffer_printf(buffer, "layout(binding = %u) uniform %ssampler%s%s ",
            binding->binding, sampler_type_prefix, sampler_type, shadow ? "Shadow" : "");
    shader_glsl_print_combined_sampler_name(buffer, gen, crs->resource_index,
            crs->resource_space, crs->sampler_index, crs->sampler_space);
    vkd3d_string_buffer_printf(buffer, ";\n");
}

static void shader_glsl_generate_descriptor_declarations(struct vkd3d_glsl_generator *gen)
{
    const struct vkd3d_shader_scan_combined_resource_sampler_info *sampler_info = gen->combined_sampler_info;
    const struct vkd3d_shader_scan_descriptor_info1 *info = gen->descriptor_info;
    const struct vkd3d_shader_descriptor_info1 *descriptor;
    unsigned int i;

    for (i = 0; i < info->descriptor_count; ++i)
    {
        descriptor = &info->descriptors[i];

        switch (descriptor->type)
        {
            case VKD3D_SHADER_DESCRIPTOR_TYPE_SRV:
            case VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER:
                /* GLSL uses combined resource/sampler descriptors.*/
                break;

            case VKD3D_SHADER_DESCRIPTOR_TYPE_UAV:
                shader_glsl_generate_uav_declaration(gen, descriptor);
                break;

            case VKD3D_SHADER_DESCRIPTOR_TYPE_CBV:
                shader_glsl_generate_cbv_declaration(gen, descriptor);
                break;

            default:
                vkd3d_string_buffer_printf(gen->buffer, "/* <unhandled descriptor type %#x> */\n", descriptor->type);
                vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                        "Internal compiler error: Unhandled descriptor type %#x.", descriptor->type);
                break;
        }
    }
    for (i = 0; i < sampler_info->combined_sampler_count; ++i)
    {
        shader_glsl_generate_sampler_declaration(gen, &sampler_info->combined_samplers[i]);
    }
    if (info->descriptor_count)
        vkd3d_string_buffer_printf(gen->buffer, "\n");
}

static const struct signature_element *signature_get_element_by_location(
        const struct shader_signature *signature, unsigned int location)
{
    const struct signature_element *e;
    unsigned int i;

    for (i = 0; i < signature->element_count; ++i)
    {
        e = &signature->elements[i];

        if (e->target_location != location)
            continue;

        return e;
    }

    return NULL;
}

static const char *shader_glsl_get_interpolation(struct vkd3d_glsl_generator *gen,
        const struct shader_signature *signature, const char *type, unsigned int location)
{
    enum vkd3d_shader_interpolation_mode m;
    const struct signature_element *e;

    if ((e = signature_get_element_by_location(signature, location)))
        m = e->interpolation_mode;
    else
        m = VKD3DSIM_NONE;

    switch (m)
    {
        case VKD3DSIM_NONE:
        case VKD3DSIM_LINEAR:
            return "";
        case VKD3DSIM_CONSTANT:
            return "flat ";
        default:
            vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                    "Internal compiler error: Unhandled interpolation mode %#x for %s location %u.", m, type, location);
            return "";
    }
}

static void shader_glsl_generate_interface_block(struct vkd3d_glsl_generator *gen,
        const struct shader_signature *signature, const char *type, unsigned int count)
{
    struct vkd3d_string_buffer *buffer = gen->buffer;
    const char *interpolation;
    unsigned int i;

    vkd3d_string_buffer_printf(buffer, "%s shader_in_out\n{\n", type);
    for (i = 0; i < count; ++i)
    {
        interpolation = shader_glsl_get_interpolation(gen, signature, type, i);
        vkd3d_string_buffer_printf(buffer, "    %svec4 reg_%u;\n", interpolation, i);
    }
    vkd3d_string_buffer_printf(buffer, "} shader_%s;\n", type);
}

static void shader_glsl_generate_input_declarations(struct vkd3d_glsl_generator *gen)
{
    const struct shader_signature *signature = &gen->program->input_signature;
    struct vkd3d_string_buffer *buffer = gen->buffer;
    const struct signature_element *e;
    unsigned int i, count;

    if (!gen->interstage_input)
    {
        for (i = 0, count = 0; i < signature->element_count; ++i)
        {
            e = &signature->elements[i];

            if (e->target_location == SIGNATURE_TARGET_LOCATION_UNUSED || e->sysval_semantic)
                continue;

            if (e->min_precision != VKD3D_SHADER_MINIMUM_PRECISION_NONE)
            {
                vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                        "Internal compiler error: Unhandled minimum precision %#x.", e->min_precision);
                continue;
            }

            if (e->interpolation_mode != VKD3DSIM_NONE)
            {
                vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                        "Internal compiler error: Unhandled interpolation mode %#x.", e->interpolation_mode);
                continue;
            }

            vkd3d_string_buffer_printf(buffer, "layout(location = %u) in ", e->target_location);
            switch (e->component_type)
            {
                case VKD3D_SHADER_COMPONENT_UINT:
                    vkd3d_string_buffer_printf(buffer, "uvec4");
                    break;
                case VKD3D_SHADER_COMPONENT_INT:
                    vkd3d_string_buffer_printf(buffer, "ivec4");
                    break;
                case VKD3D_SHADER_COMPONENT_FLOAT:
                    vkd3d_string_buffer_printf(buffer, "vec4");
                    break;
                default:
                    vkd3d_string_buffer_printf(buffer, "<unhandled type %#x>", e->component_type);
                    vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                            "Internal compiler error: Unhandled input component type %#x.", e->component_type);
                    break;
            }
            vkd3d_string_buffer_printf(buffer, " shader_in_%u;\n", i);
            ++count;
        }
        if (count)
            vkd3d_string_buffer_printf(buffer, "\n");
    }
    else if (gen->limits.input_count)
    {
        shader_glsl_generate_interface_block(gen, signature, "in", gen->limits.input_count);
        vkd3d_string_buffer_printf(buffer, "\n");
    }
}

static void shader_glsl_generate_output_declarations(struct vkd3d_glsl_generator *gen)
{
    const struct shader_signature *signature = &gen->program->output_signature;
    struct vkd3d_string_buffer *buffer = gen->buffer;
    const struct signature_element *e;
    unsigned int i, count;

    if (!gen->interstage_output)
    {
        for (i = 0, count = 0; i < signature->element_count; ++i)
        {
            e = &signature->elements[i];

            if (e->target_location == SIGNATURE_TARGET_LOCATION_UNUSED)
                continue;

            if (e->sysval_semantic != VKD3D_SHADER_SV_TARGET)
            {
                vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                        "Internal compiler error: Unhandled system value %#x.", e->sysval_semantic);
                continue;
            }

            if (e->min_precision != VKD3D_SHADER_MINIMUM_PRECISION_NONE)
            {
                vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                        "Internal compiler error: Unhandled minimum precision %#x.", e->min_precision);
                continue;
            }

            if (e->interpolation_mode != VKD3DSIM_NONE)
            {
                vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                        "Internal compiler error: Unhandled interpolation mode %#x.", e->interpolation_mode);
                continue;
            }

            vkd3d_string_buffer_printf(buffer, "layout(location = %u) out ", e->target_location);
            switch (e->component_type)
            {
                case VKD3D_SHADER_COMPONENT_UINT:
                    vkd3d_string_buffer_printf(buffer, "uvec4");
                    break;
                case VKD3D_SHADER_COMPONENT_INT:
                    vkd3d_string_buffer_printf(buffer, "ivec4");
                    break;
                case VKD3D_SHADER_COMPONENT_FLOAT:
                    vkd3d_string_buffer_printf(buffer, "vec4");
                    break;
                default:
                    vkd3d_string_buffer_printf(buffer, "<unhandled type %#x>", e->component_type);
                    vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                            "Internal compiler error: Unhandled output component type %#x.", e->component_type);
                    break;
            }
            vkd3d_string_buffer_printf(buffer, " shader_out_%u;\n", e->semantic_index);
            ++count;
        }
        if (count)
            vkd3d_string_buffer_printf(buffer, "\n");
    }
    else if (gen->limits.output_count)
    {
        shader_glsl_generate_interface_block(gen, signature, "out", gen->limits.output_count);
        vkd3d_string_buffer_printf(buffer, "\n");
    }
}

static void shader_glsl_handle_global_flags(struct vkd3d_string_buffer *buffer,
        struct vkd3d_glsl_generator *gen, enum vsir_global_flags flags)
{
    if (flags & VKD3DSGF_FORCE_EARLY_DEPTH_STENCIL)
    {
        vkd3d_string_buffer_printf(buffer, "layout(early_fragment_tests) in;\n");
        flags &= ~VKD3DSGF_FORCE_EARLY_DEPTH_STENCIL;
    }

    if (flags)
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Unhandled global flags %#"PRIx64".", (uint64_t)flags);
}

static void shader_glsl_generate_declarations(struct vkd3d_glsl_generator *gen)
{
    const struct vsir_program *program = gen->program;
    struct vkd3d_string_buffer *buffer = gen->buffer;
    const struct vsir_thread_group_size *group_size;

    if (program->shader_version.type == VKD3D_SHADER_TYPE_COMPUTE)
    {
        group_size = &program->thread_group_size;
        vkd3d_string_buffer_printf(buffer, "layout(local_size_x = %u, local_size_y = %u, local_size_z = %u) in;\n\n",
                group_size->x, group_size->y, group_size->z);
    }

    shader_glsl_handle_global_flags(buffer, gen, program->global_flags);

    shader_glsl_generate_descriptor_declarations(gen);
    shader_glsl_generate_input_declarations(gen);
    shader_glsl_generate_output_declarations(gen);

    if (gen->limits.input_count)
        vkd3d_string_buffer_printf(buffer, "vec4 %s_in[%u];\n", gen->prefix, gen->limits.input_count);
    if (gen->limits.output_count)
        vkd3d_string_buffer_printf(buffer, "vec4 %s_out[%u];\n", gen->prefix, gen->limits.output_count);
    if (program->temp_count)
        vkd3d_string_buffer_printf(buffer, "vec4 r[%u];\n", program->temp_count);
    vkd3d_string_buffer_printf(buffer, "\n");
}

static int vkd3d_glsl_generator_generate(struct vkd3d_glsl_generator *gen, struct vkd3d_shader_code *out)
{
    const struct vkd3d_shader_instruction_array *instructions = &gen->program->instructions;
    struct vkd3d_string_buffer *buffer = gen->buffer;
    unsigned int i;
    void *code;

    MESSAGE("Generating a GLSL shader. This is unsupported; you get to keep all the pieces if it breaks.\n");

    vkd3d_string_buffer_printf(buffer, "#version 440\n\n");

    vkd3d_string_buffer_printf(buffer, "/* Generated by %s. */\n\n", vkd3d_shader_get_version(NULL, NULL));

    shader_glsl_generate_declarations(gen);

    vkd3d_string_buffer_printf(buffer, "void main()\n{\n");

    ++gen->indent;
    shader_glsl_shader_prologue(gen);
    for (i = 0; i < instructions->count; ++i)
    {
        vkd3d_glsl_handle_instruction(gen, &instructions->elements[i]);
    }

    vkd3d_string_buffer_printf(buffer, "}\n");

    if (TRACE_ON())
        vkd3d_string_buffer_trace(buffer);

    if (gen->failed)
        return VKD3D_ERROR_INVALID_SHADER;

    if ((code = vkd3d_malloc(buffer->buffer_size)))
    {
        memcpy(code, buffer->buffer, buffer->content_size);
        out->size = buffer->content_size;
        out->code = code;
    }
    else return VKD3D_ERROR_OUT_OF_MEMORY;

    return VKD3D_OK;
}

static void vkd3d_glsl_generator_cleanup(struct vkd3d_glsl_generator *gen)
{
    vkd3d_string_buffer_release(&gen->string_buffers, gen->buffer);
    vkd3d_string_buffer_cache_cleanup(&gen->string_buffers);
}

static void shader_glsl_init_limits(struct vkd3d_glsl_generator *gen, const struct vkd3d_shader_version *version)
{
    struct shader_limits *limits = &gen->limits;

    if (version->major < 4 || version->major >= 6)
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Unhandled shader version %u.%u.", version->major, version->minor);

    switch (version->type)
    {
        case VKD3D_SHADER_TYPE_VERTEX:
            limits->input_count = 32;
            limits->output_count = 32;
            break;
        case VKD3D_SHADER_TYPE_PIXEL:
            limits->input_count = 32;
            limits->output_count = 8;
            break;
        case VKD3D_SHADER_TYPE_COMPUTE:
            limits->input_count = 0;
            limits->output_count = 0;
            break;
        default:
            vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                    "Internal compiler error: Unhandled shader type %#x.", version->type);
            limits->input_count = 0;
            limits->output_count = 0;
            break;
    }
}

static void vkd3d_glsl_generator_init(struct vkd3d_glsl_generator *gen,
        struct vsir_program *program, const struct vkd3d_shader_compile_info *compile_info,
        const struct vkd3d_shader_scan_descriptor_info1 *descriptor_info,
        const struct vkd3d_shader_scan_combined_resource_sampler_info *combined_sampler_info,
        struct vkd3d_shader_message_context *message_context)
{
    enum vkd3d_shader_type type = program->shader_version.type;

    memset(gen, 0, sizeof(*gen));
    gen->program = program;
    vkd3d_string_buffer_cache_init(&gen->string_buffers);
    gen->buffer = vkd3d_string_buffer_get(&gen->string_buffers);
    gen->location.source_name = compile_info->source_name;
    gen->message_context = message_context;
    if (!(gen->prefix = shader_glsl_get_prefix(type)))
    {
        vkd3d_glsl_compiler_error(gen, VKD3D_SHADER_ERROR_GLSL_INTERNAL,
                "Internal compiler error: Unhandled shader type %#x.", type);
        gen->prefix = "unknown";
    }
    shader_glsl_init_limits(gen, &program->shader_version);
    gen->interstage_input = type != VKD3D_SHADER_TYPE_VERTEX && type != VKD3D_SHADER_TYPE_COMPUTE;
    gen->interstage_output = type != VKD3D_SHADER_TYPE_PIXEL && type != VKD3D_SHADER_TYPE_COMPUTE;

    gen->interface_info = vkd3d_find_struct(compile_info->next, INTERFACE_INFO);
    gen->offset_info = vkd3d_find_struct(compile_info->next, DESCRIPTOR_OFFSET_INFO);
    gen->descriptor_info = descriptor_info;
    gen->combined_sampler_info = combined_sampler_info;
}

int glsl_compile(struct vsir_program *program, uint64_t config_flags,
        const struct vkd3d_shader_scan_descriptor_info1 *descriptor_info,
        const struct vkd3d_shader_scan_combined_resource_sampler_info *combined_sampler_info,
        const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_code *out, struct vkd3d_shader_message_context *message_context)
{
    struct vkd3d_glsl_generator generator;
    int ret;

    if ((ret = vsir_program_transform(program, config_flags, compile_info, message_context)) < 0)
        return ret;

    VKD3D_ASSERT(program->normalisation_level == VSIR_FULLY_NORMALISED_IO);

    vkd3d_glsl_generator_init(&generator, program, compile_info,
            descriptor_info, combined_sampler_info, message_context);
    ret = vkd3d_glsl_generator_generate(&generator, out);
    vkd3d_glsl_generator_cleanup(&generator);

    return ret;
}

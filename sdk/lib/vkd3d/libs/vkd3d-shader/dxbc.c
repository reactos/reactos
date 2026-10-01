/*
 * Copyright 2008-2009 Henri Verbeet for CodeWeavers
 * Copyright 2010 Rico Schüller
 * Copyright 2017 Józef Kucia for CodeWeavers
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

#define DXBC_CHECKSUM_SKIP_BYTE_COUNT 20

static void compute_dxbc_checksum(const void *dxbc, size_t size, uint32_t checksum[4])
{
    const uint8_t *ptr = dxbc;

    VKD3D_ASSERT(size > DXBC_CHECKSUM_SKIP_BYTE_COUNT);
    ptr += DXBC_CHECKSUM_SKIP_BYTE_COUNT;
    size -= DXBC_CHECKSUM_SKIP_BYTE_COUNT;

    vkd3d_compute_md5(ptr, size, checksum, VKD3D_MD5_DXBC);
}

void dxbc_writer_init(struct dxbc_writer *dxbc)
{
    memset(dxbc, 0, sizeof(*dxbc));
}

void dxbc_writer_add_section(struct dxbc_writer *dxbc, uint32_t tag, const void *data, size_t size)
{
    struct vkd3d_shader_dxbc_section_desc *section;

    VKD3D_ASSERT(dxbc->section_count < ARRAY_SIZE(dxbc->sections));

    section = &dxbc->sections[dxbc->section_count++];
    section->tag = tag;
    section->data.code = data;
    section->data.size = size;
}

int vkd3d_shader_serialize_dxbc(size_t section_count, const struct vkd3d_shader_dxbc_section_desc *sections,
        struct vkd3d_shader_code *dxbc, char **messages)
{
    size_t size_position, offsets_position, checksum_position, i;
    struct vkd3d_bytecode_buffer buffer = {0};
    uint32_t checksum[4];

    TRACE("section_count %zu, sections %p, dxbc %p, messages %p.\n", section_count, sections, dxbc, messages);

    if (messages)
        *messages = NULL;

    put_u32(&buffer, TAG_DXBC);

    checksum_position = bytecode_get_size(&buffer);
    for (i = 0; i < 4; ++i)
        put_u32(&buffer, 0);

    put_u32(&buffer, 1); /* version */
    size_position = put_u32(&buffer, 0);
    put_u32(&buffer, section_count);

    offsets_position = bytecode_get_size(&buffer);
    for (i = 0; i < section_count; ++i)
        put_u32(&buffer, 0);

    for (i = 0; i < section_count; ++i)
    {
        set_u32(&buffer, offsets_position + i * sizeof(uint32_t), bytecode_align(&buffer));
        put_u32(&buffer, sections[i].tag);
        put_u32(&buffer, sections[i].data.size);
        bytecode_put_bytes(&buffer, sections[i].data.code, sections[i].data.size);
    }
    set_u32(&buffer, size_position, bytecode_get_size(&buffer));

    compute_dxbc_checksum(buffer.data, buffer.size, checksum);
    for (i = 0; i < 4; ++i)
        set_u32(&buffer, checksum_position + i * sizeof(uint32_t), checksum[i]);

    if (!buffer.status)
    {
        dxbc->code = buffer.data;
        dxbc->size = buffer.size;
    }
    return buffer.status;
}

int dxbc_writer_write(struct dxbc_writer *dxbc, struct vkd3d_shader_code *out)
{
    return vkd3d_shader_serialize_dxbc(dxbc->section_count, dxbc->sections, out, NULL);
}

static bool require_space(size_t offset, size_t count, size_t size, size_t data_size)
{
    return !count || (data_size - offset) / count >= size;
}

static uint32_t read_u32(const char **ptr)
{
    unsigned int ret;
    memcpy(&ret, *ptr, sizeof(ret));
    *ptr += sizeof(ret);
    return ret;
}

static float read_float(const char **ptr)
{
    union
    {
        uint32_t i;
        float f;
    } u;
    STATIC_ASSERT(sizeof(float) == sizeof(uint32_t));
    u.i = read_u32(ptr);
    return u.f;
}

static void skip_dword_unknown(const char **ptr, unsigned int count)
{
    unsigned int i;
    uint32_t d;

    if (!count)
        return;

    WARN("Skipping %u unknown DWORDs:\n", count);
    for (i = 0; i < count; ++i)
    {
        d = read_u32(ptr);
        WARN("\t0x%08x\n", d);
    }
}

static const char *shader_get_string(const char *data, size_t data_size, size_t offset)
{
    size_t len, max_len;

    if (offset >= data_size)
    {
        WARN("Invalid offset %#zx (data size %#zx).\n", offset, data_size);
        return NULL;
    }

    max_len = data_size - offset;
    len = strnlen(data + offset, max_len);

    if (len == max_len)
        return NULL;

    return data + offset;
}

static int parse_dxbc(const struct vkd3d_shader_code *dxbc, struct vkd3d_shader_message_context *message_context,
        const char *source_name, uint32_t flags, struct vkd3d_shader_dxbc_desc *desc)
{
    const struct vkd3d_shader_location location = {.source_name = source_name};
    struct vkd3d_shader_dxbc_section_desc *sections, *section;
    uint32_t checksum[4], calculated_checksum[4];
    const char *data = dxbc->code;
    size_t data_size = dxbc->size;
    const char *ptr = data;
    uint32_t chunk_count;
    uint32_t total_size;
    uint32_t version;
    unsigned int i;
    uint32_t tag;

    if (data_size < VKD3D_DXBC_HEADER_SIZE)
    {
        WARN("Invalid data size %zu.\n", data_size);
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXBC_INVALID_SIZE,
                "DXBC size %zu is smaller than the DXBC header size.", data_size);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    tag = read_u32(&ptr);
    TRACE("tag: %#x.\n", tag);

    if (tag != TAG_DXBC)
    {
        WARN("Wrong tag.\n");
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXBC_INVALID_MAGIC, "Invalid DXBC magic.");
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    checksum[0] = read_u32(&ptr);
    checksum[1] = read_u32(&ptr);
    checksum[2] = read_u32(&ptr);
    checksum[3] = read_u32(&ptr);
    if (!(flags & VKD3D_SHADER_PARSE_DXBC_IGNORE_CHECKSUM))
    {
        compute_dxbc_checksum(data, data_size, calculated_checksum);
        if (memcmp(checksum, calculated_checksum, sizeof(checksum)))
        {
            WARN("Checksum {0x%08x, 0x%08x, 0x%08x, 0x%08x} does not match "
                    "calculated checksum {0x%08x, 0x%08x, 0x%08x, 0x%08x}.\n",
                    checksum[0], checksum[1], checksum[2], checksum[3],
                    calculated_checksum[0], calculated_checksum[1],
                    calculated_checksum[2], calculated_checksum[3]);
            vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXBC_INVALID_CHECKSUM,
                    "Invalid DXBC checksum.");
            return VKD3D_ERROR_INVALID_ARGUMENT;
        }
    }

    version = read_u32(&ptr);
    TRACE("version: %#x.\n", version);
    if (version != 0x00000001)
    {
        WARN("Got unexpected DXBC version %#x.\n", version);
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXBC_INVALID_VERSION,
                "DXBC version %#x is not supported.", version);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    total_size = read_u32(&ptr);
    TRACE("total size: %#x\n", total_size);

    chunk_count = read_u32(&ptr);
    TRACE("chunk count: %#x\n", chunk_count);

    if (!(sections = vkd3d_calloc(chunk_count, sizeof(*sections))))
    {
        vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXBC_OUT_OF_MEMORY, "Out of memory.");
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }

    for (i = 0; i < chunk_count; ++i)
    {
        uint32_t chunk_tag, chunk_size;
        const char *chunk_ptr;
        uint32_t chunk_offset;

        chunk_offset = read_u32(&ptr);
        TRACE("chunk %u at offset %#x\n", i, chunk_offset);

        if (chunk_offset >= data_size || !require_space(chunk_offset, 2, sizeof(uint32_t), data_size))
        {
            WARN("Invalid chunk offset %#x (data size %zu).\n", chunk_offset, data_size);
            vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXBC_INVALID_CHUNK_OFFSET,
                    "DXBC chunk %u has invalid offset %#x (data size %#zx).", i, chunk_offset, data_size);
            vkd3d_free(sections);
            return VKD3D_ERROR_INVALID_ARGUMENT;
        }

        chunk_ptr = data + chunk_offset;

        chunk_tag = read_u32(&chunk_ptr);
        chunk_size = read_u32(&chunk_ptr);

        if (!require_space(chunk_ptr - data, 1, chunk_size, data_size))
        {
            WARN("Invalid chunk size %#x (data size %zu, chunk offset %#x).\n",
                    chunk_size, data_size, chunk_offset);
            vkd3d_shader_error(message_context, &location, VKD3D_SHADER_ERROR_DXBC_INVALID_CHUNK_SIZE,
                    "DXBC chunk %u has invalid size %#x (data size %#zx, chunk offset %#x).",
                    i, chunk_offset, data_size, chunk_offset);
            vkd3d_free(sections);
            return VKD3D_ERROR_INVALID_ARGUMENT;
        }

        section = &sections[i];
        section->tag = chunk_tag;
        section->data.code = chunk_ptr;
        section->data.size = chunk_size;
    }

    desc->tag = tag;
    memcpy(desc->checksum, checksum, sizeof(checksum));
    desc->version = version;
    desc->size = total_size;
    desc->section_count = chunk_count;
    desc->sections = sections;

    return VKD3D_OK;
}

void vkd3d_shader_free_dxbc(struct vkd3d_shader_dxbc_desc *dxbc)
{
    TRACE("dxbc %p.\n", dxbc);

    vkd3d_free(dxbc->sections);
}

static int for_each_dxbc_section(const struct vkd3d_shader_code *dxbc,
        struct vkd3d_shader_message_context *message_context, const char *source_name,
        int (*section_handler)(const struct vkd3d_shader_dxbc_section_desc *section,
        struct vkd3d_shader_message_context *message_context, void *ctx), void *ctx)
{
    struct vkd3d_shader_dxbc_desc desc;
    unsigned int i;
    int ret;

    if ((ret = parse_dxbc(dxbc, message_context, source_name, 0, &desc)) < 0)
        return ret;

    for (i = 0; i < desc.section_count; ++i)
    {
        if ((ret = section_handler(&desc.sections[i], message_context, ctx)) < 0)
            break;
    }

    vkd3d_shader_free_dxbc(&desc);

    return ret;
}

int vkd3d_shader_parse_dxbc(const struct vkd3d_shader_code *dxbc,
        uint32_t flags, struct vkd3d_shader_dxbc_desc *desc, char **messages)
{
    struct vkd3d_shader_message_context message_context;
    int ret;

    TRACE("dxbc {%p, %zu}, flags %#x, desc %p, messages %p.\n", dxbc->code, dxbc->size, flags, desc, messages);

    if (messages)
        *messages = NULL;
    vkd3d_shader_message_context_init(&message_context, VKD3D_SHADER_LOG_INFO);

    ret = parse_dxbc(dxbc, &message_context, NULL, flags, desc);

    vkd3d_shader_message_context_trace_messages(&message_context);
    if (!vkd3d_shader_message_context_copy_messages(&message_context, messages) && ret >= 0)
    {
        vkd3d_shader_free_dxbc(desc);
        ret = VKD3D_ERROR_OUT_OF_MEMORY;
    }
    vkd3d_shader_message_context_cleanup(&message_context);

    if (ret < 0)
        memset(desc, 0, sizeof(*desc));

    return ret;
}

/* Shader Model 6 shaders use these special values in the output signature,
 * but Shader Model 4/5 just use VKD3D_SHADER_SV_NONE. Normalize to SM6. */
static enum vkd3d_shader_sysval_semantic map_fragment_output_sysval(const char *name)
{
    if (!ascii_strcasecmp(name, "sv_target"))
        return VKD3D_SHADER_SV_TARGET;
    if (!ascii_strcasecmp(name, "sv_depth"))
        return VKD3D_SHADER_SV_DEPTH;
    if (!ascii_strcasecmp(name, "sv_coverage"))
        return VKD3D_SHADER_SV_COVERAGE;
    if (!ascii_strcasecmp(name, "sv_depthgreaterequal"))
        return VKD3D_SHADER_SV_DEPTH_GREATER_EQUAL;
    if (!ascii_strcasecmp(name, "sv_depthlessequal"))
        return VKD3D_SHADER_SV_DEPTH_LESS_EQUAL;
    if (!ascii_strcasecmp(name, "sv_stencilref"))
        return VKD3D_SHADER_SV_STENCIL_REF;

    return VKD3D_SHADER_SV_NONE;
}

static int shader_parse_signature(const struct vkd3d_shader_dxbc_section_desc *section,
        struct vkd3d_shader_message_context *message_context, struct shader_signature *s)
{
    bool has_stream_index, has_min_precision;
    const char *data = section->data.code;
    uint32_t count, header_size;
    struct signature_element *e;
    const char *ptr = data;
    unsigned int i, j;

    if (!require_space(0, 2, sizeof(uint32_t), section->data.size))
    {
        WARN("Invalid data size %#zx.\n", section->data.size);
        vkd3d_shader_error(message_context, NULL, VKD3D_SHADER_ERROR_DXBC_INVALID_SIGNATURE,
                "Section size %zu is smaller than the minimum signature header size.\n", section->data.size);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    count = read_u32(&ptr);
    TRACE("%u elements.\n", count);

    header_size = read_u32(&ptr);
    i = header_size / sizeof(uint32_t);
    if (align(header_size, sizeof(uint32_t)) != header_size || i < 2
            || !require_space(2, i - 2, sizeof(uint32_t), section->data.size))
    {
        WARN("Invalid header size %#x.\n", header_size);
        vkd3d_shader_error(message_context, NULL, VKD3D_SHADER_ERROR_DXBC_INVALID_SIGNATURE,
                "Signature header size %#x is invalid.\n", header_size);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    skip_dword_unknown(&ptr, i - 2);

    if (!require_space(ptr - data, count, 6 * sizeof(uint32_t), section->data.size))
    {
        WARN("Invalid count %#x (data size %#zx).\n", count, section->data.size);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    if (!(e = vkd3d_calloc(count, sizeof(*e))))
    {
        ERR("Failed to allocate input signature memory.\n");
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }

    has_min_precision = section->tag == TAG_OSG1 || section->tag == TAG_PSG1 || section->tag == TAG_ISG1;
    has_stream_index = section->tag == TAG_OSG5 || has_min_precision;

    for (i = 0; i < count; ++i)
    {
        size_t name_offset;
        const char *name;
        uint32_t mask;

        if (has_stream_index)
            e[i].stream_index = read_u32(&ptr);
        else
            e[i].stream_index = 0;

        name_offset = read_u32(&ptr);
        if (!(name = shader_get_string(data, section->data.size, name_offset))
                || !(e[i].semantic_name = vkd3d_strdup(name)))
        {
            WARN("Invalid name offset %#zx (data size %#zx).\n", name_offset, section->data.size);
            for (j = 0; j < i; ++j)
            {
                vkd3d_free((void *)e[j].semantic_name);
            }
            vkd3d_free(e);
            return VKD3D_ERROR_INVALID_ARGUMENT;
        }
        e[i].semantic_index = read_u32(&ptr);
        e[i].sysval_semantic = read_u32(&ptr);
        e[i].component_type = read_u32(&ptr);
        e[i].register_index = read_u32(&ptr);
        e[i].target_location = e[i].register_index;
        e[i].register_count = 1;
        mask = read_u32(&ptr);
        e[i].mask = mask & 0xff;
        e[i].used_mask = (mask >> 8) & 0xff;
        switch (section->tag)
        {
            case TAG_OSGN:
            case TAG_OSG1:
            case TAG_OSG5:
                if (e[i].sysval_semantic == VKD3D_SHADER_SV_NONE)
                    e[i].sysval_semantic = map_fragment_output_sysval(e[i].semantic_name);
                break;
        }

        if (has_min_precision)
            e[i].min_precision = read_u32(&ptr);
        else
            e[i].min_precision = VKD3D_SHADER_MINIMUM_PRECISION_NONE;

        e[i].interpolation_mode = VKD3DSIM_NONE;

        TRACE("Stream: %u, semantic: %s, semantic idx: %u, sysval_semantic %#x, "
                "type %u, register idx: %u, use_mask %#x, input_mask %#x, precision %u.\n",
                e[i].stream_index, debugstr_a(e[i].semantic_name), e[i].semantic_index, e[i].sysval_semantic,
                e[i].component_type, e[i].register_index, e[i].used_mask, e[i].mask, e[i].min_precision);
    }

    s->elements = e;
    s->element_count = count;

    return VKD3D_OK;
}

static int isgn_handler(const struct vkd3d_shader_dxbc_section_desc *section,
        struct vkd3d_shader_message_context *message_context, void *ctx)
{
    struct shader_signature *is = ctx;

    if (section->tag != TAG_ISGN && section->tag != TAG_ISG1)
        return VKD3D_OK;

    if (is->elements)
    {
        FIXME("Multiple input signatures.\n");
        shader_signature_cleanup(is);
    }
    return shader_parse_signature(section, message_context, is);
}

int shader_parse_input_signature(const struct vkd3d_shader_code *dxbc,
        struct vkd3d_shader_message_context *message_context, struct shader_signature *signature)
{
    int ret;

    memset(signature, 0, sizeof(*signature));
    if ((ret = for_each_dxbc_section(dxbc, message_context, NULL, isgn_handler, signature)) < 0)
        ERR("Failed to parse input signature.\n");

    return ret;
}

static int shdr_handler(const struct vkd3d_shader_dxbc_section_desc *section,
        struct vkd3d_shader_message_context *message_context, void *context)
{
    struct dxbc_shader_desc *desc = context;
    int ret;

    switch (section->tag)
    {
        case TAG_ISGN:
        case TAG_ISG1:
            if (desc->input_signature.elements)
            {
                FIXME("Multiple input signatures.\n");
                break;
            }
            if ((ret = shader_parse_signature(section, message_context, &desc->input_signature)) < 0)
                return ret;
            break;

        case TAG_OSGN:
        case TAG_OSG5:
        case TAG_OSG1:
            if (desc->output_signature.elements)
            {
                FIXME("Multiple output signatures.\n");
                break;
            }
            if ((ret = shader_parse_signature(section, message_context, &desc->output_signature)) < 0)
                return ret;
            break;

        case TAG_PCSG:
        case TAG_PSG1:
            if (desc->patch_constant_signature.elements)
            {
                FIXME("Multiple patch constant signatures.\n");
                break;
            }
            if ((ret = shader_parse_signature(section, message_context, &desc->patch_constant_signature)) < 0)
                return ret;
            break;

        case TAG_DXIL:
        case TAG_SHDR:
        case TAG_SHEX:
            if ((section->tag == TAG_DXIL) != desc->is_dxil)
            {
                TRACE("Skipping chunk %#x.\n", section->tag);
                break;
            }
            if (desc->byte_code)
                FIXME("Multiple shader code chunks.\n");
            desc->byte_code = section->data.code;
            desc->byte_code_size = section->data.size;
            break;

        case TAG_AON9:
            TRACE("Skipping AON9 shader code chunk.\n");
            break;

        default:
            TRACE("Skipping chunk %#x.\n", section->tag);
            break;
    }

    return VKD3D_OK;
}

void free_dxbc_shader_desc(struct dxbc_shader_desc *desc)
{
    shader_signature_cleanup(&desc->input_signature);
    shader_signature_cleanup(&desc->output_signature);
    shader_signature_cleanup(&desc->patch_constant_signature);
}

int shader_extract_from_dxbc(const struct vkd3d_shader_code *dxbc,
        struct vkd3d_shader_message_context *message_context, const char *source_name, struct dxbc_shader_desc *desc)
{
    int ret;

    ret = for_each_dxbc_section(dxbc, message_context, source_name, shdr_handler, desc);
    if (!desc->byte_code)
        ret = VKD3D_ERROR_INVALID_ARGUMENT;

    if (ret < 0)
    {
        WARN("Failed to parse shader, vkd3d result %d.\n", ret);
        free_dxbc_shader_desc(desc);
    }

    return ret;
}

/* root signatures */
#define VKD3D_ROOT_SIGNATURE_1_0_ROOT_DESCRIPTOR_FLAGS VKD3D_SHADER_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE

#define VKD3D_ROOT_SIGNATURE_1_0_DESCRIPTOR_RANGE_FLAGS \
        (VKD3D_SHADER_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | VKD3D_SHADER_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE)

struct root_signature_parser_context
{
    const char *data;
    unsigned int data_size;
};

static int shader_parse_descriptor_ranges(struct root_signature_parser_context *context,
        unsigned int offset, unsigned int count, struct vkd3d_shader_descriptor_range *ranges)
{
    const char *ptr;
    unsigned int i;

    if (!require_space(offset, 5 * count, sizeof(uint32_t), context->data_size))
    {
        WARN("Invalid data size %#x (offset %u, count %u).\n", context->data_size, offset, count);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    ptr = &context->data[offset];

    for (i = 0; i < count; ++i)
    {
        ranges[i].range_type = read_u32(&ptr);
        ranges[i].descriptor_count = read_u32(&ptr);
        ranges[i].base_shader_register = read_u32(&ptr);
        ranges[i].register_space = read_u32(&ptr);
        ranges[i].descriptor_table_offset = read_u32(&ptr);

        TRACE("Type %#x, descriptor count %u, base shader register %u, "
                "register space %u, offset %u.\n",
                ranges[i].range_type, ranges[i].descriptor_count,
                ranges[i].base_shader_register, ranges[i].register_space,
                ranges[i].descriptor_table_offset);
    }

    return VKD3D_OK;
}

static void shader_validate_descriptor_range1(const struct vkd3d_shader_descriptor_range1 *range)
{
    unsigned int unknown_flags = range->flags & ~(VKD3D_SHADER_DESCRIPTOR_RANGE_FLAG_NONE
            | VKD3D_SHADER_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE
            | VKD3D_SHADER_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE
            | VKD3D_SHADER_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE
            | VKD3D_SHADER_DESCRIPTOR_RANGE_FLAG_DATA_STATIC
            | VKD3D_SHADER_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_STATIC_KEEPING_BUFFER_BOUNDS_CHECKS);

    if (unknown_flags)
        FIXME("Unknown descriptor range flags %#x.\n", unknown_flags);
}

static int shader_parse_descriptor_ranges1(struct root_signature_parser_context *context,
        unsigned int offset, unsigned int count, struct vkd3d_shader_descriptor_range1 *ranges)
{
    const char *ptr;
    unsigned int i;

    if (!require_space(offset, 6 * count, sizeof(uint32_t), context->data_size))
    {
        WARN("Invalid data size %#x (offset %u, count %u).\n", context->data_size, offset, count);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    ptr = &context->data[offset];

    for (i = 0; i < count; ++i)
    {
        ranges[i].range_type = read_u32(&ptr);
        ranges[i].descriptor_count = read_u32(&ptr);
        ranges[i].base_shader_register = read_u32(&ptr);
        ranges[i].register_space = read_u32(&ptr);
        ranges[i].flags = read_u32(&ptr);
        ranges[i].descriptor_table_offset = read_u32(&ptr);

        TRACE("Type %#x, descriptor count %u, base shader register %u, "
                "register space %u, flags %#x, offset %u.\n",
                ranges[i].range_type, ranges[i].descriptor_count,
                ranges[i].base_shader_register, ranges[i].register_space,
                ranges[i].flags, ranges[i].descriptor_table_offset);

        shader_validate_descriptor_range1(&ranges[i]);
    }

    return VKD3D_OK;
}

static int shader_parse_descriptor_table(struct root_signature_parser_context *context,
        unsigned int offset, struct vkd3d_shader_root_descriptor_table *table)
{
    struct vkd3d_shader_descriptor_range *ranges;
    unsigned int count;
    const char *ptr;

    if (!require_space(offset, 2, sizeof(uint32_t), context->data_size))
    {
        WARN("Invalid data size %#x (offset %u).\n", context->data_size, offset);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    ptr = &context->data[offset];

    count = read_u32(&ptr);
    offset = read_u32(&ptr);

    TRACE("Descriptor range count %u.\n", count);

    table->descriptor_range_count = count;

    if (!(ranges = vkd3d_calloc(count, sizeof(*ranges))))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    table->descriptor_ranges = ranges;
    return shader_parse_descriptor_ranges(context, offset, count, ranges);
}

static int shader_parse_descriptor_table1(struct root_signature_parser_context *context,
        unsigned int offset, struct vkd3d_shader_root_descriptor_table1 *table)
{
    struct vkd3d_shader_descriptor_range1 *ranges;
    unsigned int count;
    const char *ptr;

    if (!require_space(offset, 2, sizeof(uint32_t), context->data_size))
    {
        WARN("Invalid data size %#x (offset %u).\n", context->data_size, offset);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    ptr = &context->data[offset];

    count = read_u32(&ptr);
    offset = read_u32(&ptr);

    TRACE("Descriptor range count %u.\n", count);

    table->descriptor_range_count = count;

    if (!(ranges = vkd3d_calloc(count, sizeof(*ranges))))
        return VKD3D_ERROR_OUT_OF_MEMORY;
    table->descriptor_ranges = ranges;
    return shader_parse_descriptor_ranges1(context, offset, count, ranges);
}

static int shader_parse_root_constants(struct root_signature_parser_context *context,
        unsigned int offset, struct vkd3d_shader_root_constants *constants)
{
    const char *ptr;

    if (!require_space(offset, 3, sizeof(uint32_t), context->data_size))
    {
        WARN("Invalid data size %#x (offset %u).\n", context->data_size, offset);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    ptr = &context->data[offset];

    constants->shader_register = read_u32(&ptr);
    constants->register_space = read_u32(&ptr);
    constants->value_count = read_u32(&ptr);

    TRACE("Shader register %u, register space %u, 32-bit value count %u.\n",
            constants->shader_register, constants->register_space, constants->value_count);

    return VKD3D_OK;
}

static int shader_parse_root_descriptor(struct root_signature_parser_context *context,
        unsigned int offset, struct vkd3d_shader_root_descriptor *descriptor)
{
    const char *ptr;

    if (!require_space(offset, 2, sizeof(uint32_t), context->data_size))
    {
        WARN("Invalid data size %#x (offset %u).\n", context->data_size, offset);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    ptr = &context->data[offset];

    descriptor->shader_register = read_u32(&ptr);
    descriptor->register_space = read_u32(&ptr);

    TRACE("Shader register %u, register space %u.\n",
            descriptor->shader_register, descriptor->register_space);

    return VKD3D_OK;
}

static void shader_validate_root_descriptor1(const struct vkd3d_shader_root_descriptor1 *descriptor)
{
    unsigned int unknown_flags = descriptor->flags & ~(VKD3D_SHADER_ROOT_DESCRIPTOR_FLAG_NONE
            | VKD3D_SHADER_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE
            | VKD3D_SHADER_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE
            | VKD3D_SHADER_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);

    if (unknown_flags)
        FIXME("Unknown root descriptor flags %#x.\n", unknown_flags);
}

static int shader_parse_root_descriptor1(struct root_signature_parser_context *context,
        unsigned int offset, struct vkd3d_shader_root_descriptor1 *descriptor)
{
    const char *ptr;

    if (!require_space(offset, 3, sizeof(uint32_t), context->data_size))
    {
        WARN("Invalid data size %#x (offset %u).\n", context->data_size, offset);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    ptr = &context->data[offset];

    descriptor->shader_register = read_u32(&ptr);
    descriptor->register_space = read_u32(&ptr);
    descriptor->flags = read_u32(&ptr);

    TRACE("Shader register %u, register space %u, flags %#x.\n",
            descriptor->shader_register, descriptor->register_space, descriptor->flags);

    shader_validate_root_descriptor1(descriptor);

    return VKD3D_OK;
}

static int shader_parse_root_parameters(struct root_signature_parser_context *context,
        unsigned int offset, unsigned int count, struct vkd3d_shader_root_parameter *parameters)
{
    const char *ptr;
    unsigned int i;
    int ret;

    if (!require_space(offset, 3 * count, sizeof(uint32_t), context->data_size))
    {
        WARN("Invalid data size %#x (offset %u, count %u).\n", context->data_size, offset, count);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    ptr = &context->data[offset];

    for (i = 0; i < count; ++i)
    {
        parameters[i].parameter_type = read_u32(&ptr);
        parameters[i].shader_visibility = read_u32(&ptr);
        offset = read_u32(&ptr);

        TRACE("Type %#x, shader visibility %#x.\n",
                parameters[i].parameter_type, parameters[i].shader_visibility);

        switch (parameters[i].parameter_type)
        {
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
                ret = shader_parse_descriptor_table(context, offset, &parameters[i].u.descriptor_table);
                break;
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
                ret = shader_parse_root_constants(context, offset, &parameters[i].u.constants);
                break;
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_CBV:
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_SRV:
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_UAV:
                ret = shader_parse_root_descriptor(context, offset, &parameters[i].u.descriptor);
                break;
            default:
                FIXME("Unrecognized type %#x.\n", parameters[i].parameter_type);
                return VKD3D_ERROR_INVALID_ARGUMENT;
        }

        if (ret < 0)
            return ret;
    }

    return VKD3D_OK;
}

static int shader_parse_root_parameters1(struct root_signature_parser_context *context,
        uint32_t offset, unsigned int count, struct vkd3d_shader_root_parameter1 *parameters)
{
    const char *ptr;
    unsigned int i;
    int ret;

    if (!require_space(offset, 3 * count, sizeof(uint32_t), context->data_size))
    {
        WARN("Invalid data size %#x (offset %u, count %u).\n", context->data_size, offset, count);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    ptr = &context->data[offset];

    for (i = 0; i < count; ++i)
    {
        parameters[i].parameter_type = read_u32(&ptr);
        parameters[i].shader_visibility = read_u32(&ptr);
        offset = read_u32(&ptr);

        TRACE("Type %#x, shader visibility %#x.\n",
                parameters[i].parameter_type, parameters[i].shader_visibility);

        switch (parameters[i].parameter_type)
        {
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
                ret = shader_parse_descriptor_table1(context, offset, &parameters[i].u.descriptor_table);
                break;
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
                ret = shader_parse_root_constants(context, offset, &parameters[i].u.constants);
                break;
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_CBV:
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_SRV:
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_UAV:
                ret = shader_parse_root_descriptor1(context, offset, &parameters[i].u.descriptor);
                break;
            default:
                FIXME("Unrecognized type %#x.\n", parameters[i].parameter_type);
                return VKD3D_ERROR_INVALID_ARGUMENT;
        }

        if (ret < 0)
            return ret;
    }

    return VKD3D_OK;
}

static int shader_parse_static_samplers(struct root_signature_parser_context *context,
        unsigned int offset, unsigned int count, struct vkd3d_shader_static_sampler_desc *sampler_descs)
{
    const char *ptr;
    unsigned int i;

    if (!require_space(offset, 13 * count, sizeof(uint32_t), context->data_size))
    {
        WARN("Invalid data size %#x (offset %u, count %u).\n", context->data_size, offset, count);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    ptr = &context->data[offset];

    for (i = 0; i < count; ++i)
    {
        sampler_descs[i].filter = read_u32(&ptr);
        sampler_descs[i].address_u = read_u32(&ptr);
        sampler_descs[i].address_v = read_u32(&ptr);
        sampler_descs[i].address_w = read_u32(&ptr);
        sampler_descs[i].mip_lod_bias = read_float(&ptr);
        sampler_descs[i].max_anisotropy = read_u32(&ptr);
        sampler_descs[i].comparison_func = read_u32(&ptr);
        sampler_descs[i].border_colour = read_u32(&ptr);
        sampler_descs[i].min_lod = read_float(&ptr);
        sampler_descs[i].max_lod = read_float(&ptr);
        sampler_descs[i].shader_register = read_u32(&ptr);
        sampler_descs[i].register_space = read_u32(&ptr);
        sampler_descs[i].shader_visibility = read_u32(&ptr);
    }

    return VKD3D_OK;
}

static int shader_parse_root_signature(const struct vkd3d_shader_code *data,
        struct vkd3d_shader_versioned_root_signature_desc *desc)
{
    struct vkd3d_shader_root_signature_desc *v_1_0 = &desc->u.v_1_0;
    struct root_signature_parser_context context;
    unsigned int count, offset, version;
    const char *ptr = data->code;
    int ret;

    context.data = data->code;
    context.data_size = data->size;

    if (!require_space(0, 6, sizeof(uint32_t), data->size))
    {
        WARN("Invalid data size %#zx.\n", data->size);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    version = read_u32(&ptr);
    TRACE("Version %#x.\n", version);
    if (version != VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0 && version != VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_1)
    {
        FIXME("Unknown version %#x.\n", version);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    desc->version = version;

    count = read_u32(&ptr);
    offset = read_u32(&ptr);
    TRACE("Parameter count %u, offset %u.\n", count, offset);

    if (desc->version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0)
    {
        v_1_0->parameter_count = count;
        if (v_1_0->parameter_count)
        {
            struct vkd3d_shader_root_parameter *parameters;
            if (!(parameters = vkd3d_calloc(v_1_0->parameter_count, sizeof(*parameters))))
                return VKD3D_ERROR_OUT_OF_MEMORY;
            v_1_0->parameters = parameters;
            if ((ret = shader_parse_root_parameters(&context, offset, count, parameters)) < 0)
                return ret;
        }
    }
    else
    {
        struct vkd3d_shader_root_signature_desc1 *v_1_1 = &desc->u.v_1_1;

        VKD3D_ASSERT(version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_1);

        v_1_1->parameter_count = count;
        if (v_1_1->parameter_count)
        {
            struct vkd3d_shader_root_parameter1 *parameters;
            if (!(parameters = vkd3d_calloc(v_1_1->parameter_count, sizeof(*parameters))))
                return VKD3D_ERROR_OUT_OF_MEMORY;
            v_1_1->parameters = parameters;
            if ((ret = shader_parse_root_parameters1(&context, offset, count, parameters)) < 0)
                return ret;
        }
    }

    count = read_u32(&ptr);
    offset = read_u32(&ptr);
    TRACE("Static sampler count %u, offset %u.\n", count, offset);

    v_1_0->static_sampler_count = count;
    if (v_1_0->static_sampler_count)
    {
        struct vkd3d_shader_static_sampler_desc *samplers;
        if (!(samplers = vkd3d_calloc(v_1_0->static_sampler_count, sizeof(*samplers))))
            return VKD3D_ERROR_OUT_OF_MEMORY;
        v_1_0->static_samplers = samplers;
        if ((ret = shader_parse_static_samplers(&context, offset, count, samplers)) < 0)
            return ret;
    }

    v_1_0->flags = read_u32(&ptr);
    TRACE("Flags %#x.\n", v_1_0->flags);

    return VKD3D_OK;
}

static int rts0_handler(const struct vkd3d_shader_dxbc_section_desc *section,
        struct vkd3d_shader_message_context *message_context, void *context)
{
    struct vkd3d_shader_versioned_root_signature_desc *desc = context;

    if (section->tag != TAG_RTS0)
        return VKD3D_OK;

    return shader_parse_root_signature(&section->data, desc);
}

int vkd3d_shader_parse_root_signature(const struct vkd3d_shader_code *dxbc,
        struct vkd3d_shader_versioned_root_signature_desc *root_signature, char **messages)
{
    struct vkd3d_shader_message_context message_context;
    int ret;

    TRACE("dxbc {%p, %zu}, root_signature %p, messages %p.\n", dxbc->code, dxbc->size, root_signature, messages);

    memset(root_signature, 0, sizeof(*root_signature));
    if (messages)
        *messages = NULL;
    vkd3d_shader_message_context_init(&message_context, VKD3D_SHADER_LOG_INFO);

    ret = for_each_dxbc_section(dxbc, &message_context, NULL, rts0_handler, root_signature);
    vkd3d_shader_message_context_trace_messages(&message_context);
    if (!vkd3d_shader_message_context_copy_messages(&message_context, messages))
        ret = VKD3D_ERROR_OUT_OF_MEMORY;

    vkd3d_shader_message_context_cleanup(&message_context);
    if (ret < 0)
        vkd3d_shader_free_root_signature(root_signature);

    return ret;
}

static unsigned int versioned_root_signature_get_parameter_count(
        const struct vkd3d_shader_versioned_root_signature_desc *desc)
{
    if (desc->version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0)
        return desc->u.v_1_0.parameter_count;
    else
        return desc->u.v_1_1.parameter_count;
}

static enum vkd3d_shader_root_parameter_type versioned_root_signature_get_parameter_type(
        const struct vkd3d_shader_versioned_root_signature_desc *desc, unsigned int i)
{
    if (desc->version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0)
        return desc->u.v_1_0.parameters[i].parameter_type;
    else
        return desc->u.v_1_1.parameters[i].parameter_type;
}

static enum vkd3d_shader_visibility versioned_root_signature_get_parameter_shader_visibility(
        const struct vkd3d_shader_versioned_root_signature_desc *desc, unsigned int i)
{
    if (desc->version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0)
        return desc->u.v_1_0.parameters[i].shader_visibility;
    else
        return desc->u.v_1_1.parameters[i].shader_visibility;
}

static const struct vkd3d_shader_root_constants *versioned_root_signature_get_root_constants(
        const struct vkd3d_shader_versioned_root_signature_desc *desc, unsigned int i)
{
    if (desc->version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0)
        return &desc->u.v_1_0.parameters[i].u.constants;
    else
        return &desc->u.v_1_1.parameters[i].u.constants;
}

static unsigned int versioned_root_signature_get_static_sampler_count(
        const struct vkd3d_shader_versioned_root_signature_desc *desc)
{
    if (desc->version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0)
        return desc->u.v_1_0.static_sampler_count;
    else
        return desc->u.v_1_1.static_sampler_count;
}

static const struct vkd3d_shader_static_sampler_desc *versioned_root_signature_get_static_samplers(
        const struct vkd3d_shader_versioned_root_signature_desc *desc)
{
    if (desc->version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0)
        return desc->u.v_1_0.static_samplers;
    else
        return desc->u.v_1_1.static_samplers;
}

static unsigned int versioned_root_signature_get_flags(const struct vkd3d_shader_versioned_root_signature_desc *desc)
{
    if (desc->version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0)
        return desc->u.v_1_0.flags;
    else
        return desc->u.v_1_1.flags;
}

struct root_signature_writer_context
{
    struct vkd3d_shader_message_context message_context;

    struct vkd3d_bytecode_buffer buffer;

    size_t total_size_position;
    size_t chunk_position;
};

static size_t get_chunk_offset(struct root_signature_writer_context *context)
{
    return bytecode_get_size(&context->buffer) - context->chunk_position;
}

static void shader_write_root_signature_header(struct root_signature_writer_context *context)
{
    struct vkd3d_bytecode_buffer *buffer = &context->buffer;
    unsigned int i;

    put_u32(buffer, TAG_DXBC);

    /* The checksum is computed when all data is generated. */
    for (i = 0; i < 4; ++i)
        put_u32(buffer, 0);
    put_u32(buffer, 1);
    context->total_size_position = put_u32(buffer, 0xffffffff);
    put_u32(buffer, 1); /* chunk count */
    put_u32(buffer, bytecode_get_size(buffer) + sizeof(uint32_t)); /* chunk offset */
    put_u32(buffer, TAG_RTS0);
    put_u32(buffer, 0xffffffff);
    context->chunk_position = bytecode_get_size(buffer);
}

static void shader_write_descriptor_ranges(struct vkd3d_bytecode_buffer *buffer,
        const struct vkd3d_shader_root_descriptor_table *table)
{
    const struct vkd3d_shader_descriptor_range *ranges = table->descriptor_ranges;
    unsigned int i;

    for (i = 0; i < table->descriptor_range_count; ++i)
    {
        put_u32(buffer, ranges[i].range_type);
        put_u32(buffer, ranges[i].descriptor_count);
        put_u32(buffer, ranges[i].base_shader_register);
        put_u32(buffer, ranges[i].register_space);
        put_u32(buffer, ranges[i].descriptor_table_offset);
    }
}

static void shader_write_descriptor_ranges1(struct vkd3d_bytecode_buffer *buffer,
        const struct vkd3d_shader_root_descriptor_table1 *table)
{
    const struct vkd3d_shader_descriptor_range1 *ranges = table->descriptor_ranges;
    unsigned int i;

    for (i = 0; i < table->descriptor_range_count; ++i)
    {
        put_u32(buffer, ranges[i].range_type);
        put_u32(buffer, ranges[i].descriptor_count);
        put_u32(buffer, ranges[i].base_shader_register);
        put_u32(buffer, ranges[i].register_space);
        put_u32(buffer, ranges[i].flags);
        put_u32(buffer, ranges[i].descriptor_table_offset);
    }
}

static void shader_write_descriptor_table(struct root_signature_writer_context *context,
        const struct vkd3d_shader_root_descriptor_table *table)
{
    struct vkd3d_bytecode_buffer *buffer = &context->buffer;

    put_u32(buffer, table->descriptor_range_count);
    put_u32(buffer, get_chunk_offset(context) + sizeof(uint32_t)); /* offset */

    shader_write_descriptor_ranges(buffer, table);
}

static void shader_write_descriptor_table1(struct root_signature_writer_context *context,
        const struct vkd3d_shader_root_descriptor_table1 *table)
{
    struct vkd3d_bytecode_buffer *buffer = &context->buffer;

    put_u32(buffer, table->descriptor_range_count);
    put_u32(buffer, get_chunk_offset(context) + sizeof(uint32_t)); /* offset */

    shader_write_descriptor_ranges1(buffer, table);
}

static void shader_write_root_constants(struct vkd3d_bytecode_buffer *buffer,
        const struct vkd3d_shader_root_constants *constants)
{
    put_u32(buffer, constants->shader_register);
    put_u32(buffer, constants->register_space);
    put_u32(buffer, constants->value_count);
}

static void shader_write_root_descriptor(struct vkd3d_bytecode_buffer *buffer,
        const struct vkd3d_shader_root_descriptor *descriptor)
{
    put_u32(buffer, descriptor->shader_register);
    put_u32(buffer, descriptor->register_space);
}

static void shader_write_root_descriptor1(struct vkd3d_bytecode_buffer *buffer,
        const struct vkd3d_shader_root_descriptor1 *descriptor)
{
    put_u32(buffer, descriptor->shader_register);
    put_u32(buffer, descriptor->register_space);
    put_u32(buffer, descriptor->flags);
}

static int shader_write_root_parameters(struct root_signature_writer_context *context,
        const struct vkd3d_shader_versioned_root_signature_desc *desc)
{
    unsigned int parameter_count = versioned_root_signature_get_parameter_count(desc);
    struct vkd3d_bytecode_buffer *buffer = &context->buffer;
    size_t parameters_position;
    unsigned int i;

    parameters_position = bytecode_align(buffer);
    for (i = 0; i < parameter_count; ++i)
    {
        put_u32(buffer, versioned_root_signature_get_parameter_type(desc, i));
        put_u32(buffer, versioned_root_signature_get_parameter_shader_visibility(desc, i));
        put_u32(buffer, 0xffffffff); /* offset */
    }

    for (i = 0; i < parameter_count; ++i)
    {
        set_u32(buffer, parameters_position + ((3 * i + 2) * sizeof(uint32_t)), get_chunk_offset(context));

        switch (versioned_root_signature_get_parameter_type(desc, i))
        {
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
                if (desc->version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0)
                    shader_write_descriptor_table(context, &desc->u.v_1_0.parameters[i].u.descriptor_table);
                else
                    shader_write_descriptor_table1(context, &desc->u.v_1_1.parameters[i].u.descriptor_table);
                break;
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
                shader_write_root_constants(buffer, versioned_root_signature_get_root_constants(desc, i));
                break;
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_CBV:
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_SRV:
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_UAV:
                if (desc->version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0)
                    shader_write_root_descriptor(buffer, &desc->u.v_1_0.parameters[i].u.descriptor);
                else
                    shader_write_root_descriptor1(buffer, &desc->u.v_1_1.parameters[i].u.descriptor);
                break;
            default:
                FIXME("Unrecognized type %#x.\n", versioned_root_signature_get_parameter_type(desc, i));
                vkd3d_shader_error(&context->message_context, NULL, VKD3D_SHADER_ERROR_RS_INVALID_ROOT_PARAMETER_TYPE,
                        "Invalid/unrecognised root signature root parameter type %#x.",
                        versioned_root_signature_get_parameter_type(desc, i));
                return VKD3D_ERROR_INVALID_ARGUMENT;
        }
    }

    return VKD3D_OK;
}

static void shader_write_static_samplers(struct vkd3d_bytecode_buffer *buffer,
        const struct vkd3d_shader_versioned_root_signature_desc *desc)
{
    const struct vkd3d_shader_static_sampler_desc *samplers = versioned_root_signature_get_static_samplers(desc);
    unsigned int i;

    for (i = 0; i < versioned_root_signature_get_static_sampler_count(desc); ++i)
    {
        put_u32(buffer, samplers[i].filter);
        put_u32(buffer, samplers[i].address_u);
        put_u32(buffer, samplers[i].address_v);
        put_u32(buffer, samplers[i].address_w);
        put_f32(buffer, samplers[i].mip_lod_bias);
        put_u32(buffer, samplers[i].max_anisotropy);
        put_u32(buffer, samplers[i].comparison_func);
        put_u32(buffer, samplers[i].border_colour);
        put_f32(buffer, samplers[i].min_lod);
        put_f32(buffer, samplers[i].max_lod);
        put_u32(buffer, samplers[i].shader_register);
        put_u32(buffer, samplers[i].register_space);
        put_u32(buffer, samplers[i].shader_visibility);
    }
}

static int shader_write_root_signature(struct root_signature_writer_context *context,
        const struct vkd3d_shader_versioned_root_signature_desc *desc)
{
    struct vkd3d_bytecode_buffer *buffer = &context->buffer;
    size_t samplers_offset_position;
    int ret;

    put_u32(buffer, desc->version);
    put_u32(buffer, versioned_root_signature_get_parameter_count(desc));
    put_u32(buffer, get_chunk_offset(context) + 4 * sizeof(uint32_t)); /* offset */
    put_u32(buffer, versioned_root_signature_get_static_sampler_count(desc));
    samplers_offset_position = put_u32(buffer, 0xffffffff);
    put_u32(buffer, versioned_root_signature_get_flags(desc));

    if ((ret = shader_write_root_parameters(context, desc)) < 0)
        return ret;

    set_u32(buffer, samplers_offset_position, get_chunk_offset(context));
    shader_write_static_samplers(buffer, desc);
    return 0;
}

static int validate_descriptor_table_v_1_0(const struct vkd3d_shader_root_descriptor_table *descriptor_table,
        struct vkd3d_shader_message_context *message_context)
{
    bool have_srv_uav_cbv = false;
    bool have_sampler = false;
    unsigned int i;

    for (i = 0; i < descriptor_table->descriptor_range_count; ++i)
    {
        const struct vkd3d_shader_descriptor_range *r = &descriptor_table->descriptor_ranges[i];

        if (r->range_type == VKD3D_SHADER_DESCRIPTOR_TYPE_SRV
                || r->range_type == VKD3D_SHADER_DESCRIPTOR_TYPE_UAV
                || r->range_type == VKD3D_SHADER_DESCRIPTOR_TYPE_CBV)
        {
            have_srv_uav_cbv = true;
        }
        else if (r->range_type == VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER)
        {
            have_sampler = true;
        }
        else
        {
            WARN("Invalid descriptor range type %#x.\n", r->range_type);
            vkd3d_shader_error(message_context, NULL, VKD3D_SHADER_ERROR_RS_INVALID_DESCRIPTOR_RANGE_TYPE,
                    "Invalid root signature descriptor range type %#x.", r->range_type);
            return VKD3D_ERROR_INVALID_ARGUMENT;
        }
    }

    if (have_srv_uav_cbv && have_sampler)
    {
        WARN("Samplers cannot be mixed with CBVs/SRVs/UAVs in descriptor tables.\n");
        vkd3d_shader_error(message_context, NULL, VKD3D_SHADER_ERROR_RS_MIXED_DESCRIPTOR_RANGE_TYPES,
                "Encountered both CBV/SRV/UAV and sampler descriptor ranges in the same root descriptor table.");
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    return VKD3D_OK;
}

static int validate_descriptor_table_v_1_1(const struct vkd3d_shader_root_descriptor_table1 *descriptor_table,
        struct vkd3d_shader_message_context *message_context)
{
    bool have_srv_uav_cbv = false;
    bool have_sampler = false;
    unsigned int i;

    for (i = 0; i < descriptor_table->descriptor_range_count; ++i)
    {
        const struct vkd3d_shader_descriptor_range1 *r = &descriptor_table->descriptor_ranges[i];

        if (r->range_type == VKD3D_SHADER_DESCRIPTOR_TYPE_SRV
                || r->range_type == VKD3D_SHADER_DESCRIPTOR_TYPE_UAV
                || r->range_type == VKD3D_SHADER_DESCRIPTOR_TYPE_CBV)
        {
            have_srv_uav_cbv = true;
        }
        else if (r->range_type == VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER)
        {
            have_sampler = true;
        }
        else
        {
            WARN("Invalid descriptor range type %#x.\n", r->range_type);
            vkd3d_shader_error(message_context, NULL, VKD3D_SHADER_ERROR_RS_INVALID_DESCRIPTOR_RANGE_TYPE,
                    "Invalid root signature descriptor range type %#x.", r->range_type);
            return VKD3D_ERROR_INVALID_ARGUMENT;
        }
    }

    if (have_srv_uav_cbv && have_sampler)
    {
        WARN("Samplers cannot be mixed with CBVs/SRVs/UAVs in descriptor tables.\n");
        vkd3d_shader_error(message_context, NULL, VKD3D_SHADER_ERROR_RS_MIXED_DESCRIPTOR_RANGE_TYPES,
                "Encountered both CBV/SRV/UAV and sampler descriptor ranges in the same root descriptor table.");
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    return VKD3D_OK;
}

static int validate_root_signature_desc(const struct vkd3d_shader_versioned_root_signature_desc *desc,
        struct vkd3d_shader_message_context *message_context)
{
    int ret = VKD3D_OK;
    unsigned int i;

    for (i = 0; i < versioned_root_signature_get_parameter_count(desc); ++i)
    {
        enum vkd3d_shader_root_parameter_type type;

        type = versioned_root_signature_get_parameter_type(desc, i);
        if (type == VKD3D_SHADER_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
        {
            if (desc->version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0)
                ret = validate_descriptor_table_v_1_0(&desc->u.v_1_0.parameters[i].u.descriptor_table, message_context);
            else
                ret = validate_descriptor_table_v_1_1(&desc->u.v_1_1.parameters[i].u.descriptor_table, message_context);
        }

        if (ret < 0)
            break;
    }

    return ret;
}

int vkd3d_shader_serialize_root_signature(const struct vkd3d_shader_versioned_root_signature_desc *root_signature,
        struct vkd3d_shader_code *dxbc, char **messages)
{
    struct root_signature_writer_context context;
    size_t total_size, chunk_size;
    uint32_t checksum[4];
    unsigned int i;
    int ret;

    TRACE("root_signature %p, dxbc %p, messages %p.\n", root_signature, dxbc, messages);

    if (messages)
        *messages = NULL;

    memset(&context, 0, sizeof(context));
    vkd3d_shader_message_context_init(&context.message_context, VKD3D_SHADER_LOG_INFO);

    if (root_signature->version != VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0
            && root_signature->version != VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_1)
    {
        ret = VKD3D_ERROR_INVALID_ARGUMENT;
        WARN("Root signature version %#x not supported.\n", root_signature->version);
        vkd3d_shader_error(&context.message_context, NULL, VKD3D_SHADER_ERROR_RS_INVALID_VERSION,
                "Root signature version %#x is not supported.", root_signature->version);
        goto done;
    }

    if ((ret = validate_root_signature_desc(root_signature, &context.message_context)) < 0)
        goto done;

    memset(dxbc, 0, sizeof(*dxbc));
    shader_write_root_signature_header(&context);

    if ((ret = shader_write_root_signature(&context, root_signature)) < 0)
    {
        vkd3d_free(context.buffer.data);
        goto done;
    }

    if (context.buffer.status)
    {
        vkd3d_shader_error(&context.message_context, NULL, VKD3D_SHADER_ERROR_RS_OUT_OF_MEMORY,
                "Out of memory while writing root signature.");
        vkd3d_free(context.buffer.data);
        goto done;
    }

    total_size = bytecode_get_size(&context.buffer);
    chunk_size = get_chunk_offset(&context);
    set_u32(&context.buffer, context.total_size_position, total_size);
    set_u32(&context.buffer, context.chunk_position - sizeof(uint32_t), chunk_size);

    dxbc->code = context.buffer.data;
    dxbc->size = total_size;

    compute_dxbc_checksum(dxbc->code, dxbc->size, checksum);
    for (i = 0; i < 4; ++i)
        set_u32(&context.buffer, (i + 1) * sizeof(uint32_t), checksum[i]);

    ret = VKD3D_OK;

done:
    vkd3d_shader_message_context_trace_messages(&context.message_context);
    if (!vkd3d_shader_message_context_copy_messages(&context.message_context, messages))
        ret = VKD3D_ERROR_OUT_OF_MEMORY;
    vkd3d_shader_message_context_cleanup(&context.message_context);
    return ret;
}

static void free_descriptor_ranges(const struct vkd3d_shader_root_parameter *parameters, unsigned int count)
{
    unsigned int i;

    if (!parameters)
        return;

    for (i = 0; i < count; ++i)
    {
        const struct vkd3d_shader_root_parameter *p = &parameters[i];

        if (p->parameter_type == VKD3D_SHADER_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
            vkd3d_free((void *)p->u.descriptor_table.descriptor_ranges);
    }
}

static int convert_root_parameters_to_v_1_0(struct vkd3d_shader_root_parameter *dst,
        const struct vkd3d_shader_root_parameter1 *src, unsigned int count)
{
    const struct vkd3d_shader_descriptor_range1 *ranges1;
    struct vkd3d_shader_descriptor_range *ranges;
    unsigned int i, j;
    int ret;

    for (i = 0; i < count; ++i)
    {
        const struct vkd3d_shader_root_parameter1 *p1 = &src[i];
        struct vkd3d_shader_root_parameter *p = &dst[i];

        p->parameter_type = p1->parameter_type;
        switch (p->parameter_type)
        {
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
                ranges = NULL;
                if ((p->u.descriptor_table.descriptor_range_count = p1->u.descriptor_table.descriptor_range_count))
                {
                    if (!(ranges = vkd3d_calloc(p->u.descriptor_table.descriptor_range_count, sizeof(*ranges))))
                    {
                        ret = VKD3D_ERROR_OUT_OF_MEMORY;
                        goto fail;
                    }
                }
                p->u.descriptor_table.descriptor_ranges = ranges;
                ranges1 = p1->u.descriptor_table.descriptor_ranges;
                for (j = 0; j < p->u.descriptor_table.descriptor_range_count; ++j)
                {
                    ranges[j].range_type = ranges1[j].range_type;
                    ranges[j].descriptor_count = ranges1[j].descriptor_count;
                    ranges[j].base_shader_register = ranges1[j].base_shader_register;
                    ranges[j].register_space = ranges1[j].register_space;
                    ranges[j].descriptor_table_offset = ranges1[j].descriptor_table_offset;
                }
                break;
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
                p->u.constants = p1->u.constants;
                break;
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_CBV:
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_SRV:
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_UAV:
                p->u.descriptor.shader_register = p1->u.descriptor.shader_register;
                p->u.descriptor.register_space = p1->u.descriptor.register_space;
                break;
            default:
                WARN("Invalid root parameter type %#x.\n", p->parameter_type);
                ret = VKD3D_ERROR_INVALID_ARGUMENT;
                goto fail;

        }
        p->shader_visibility = p1->shader_visibility;
    }

    return VKD3D_OK;

fail:
    free_descriptor_ranges(dst, i);
    return ret;
}

static int convert_root_signature_to_v1_0(struct vkd3d_shader_versioned_root_signature_desc *dst,
        const struct vkd3d_shader_versioned_root_signature_desc *src)
{
    const struct vkd3d_shader_root_signature_desc1 *src_desc = &src->u.v_1_1;
    struct vkd3d_shader_root_signature_desc *dst_desc = &dst->u.v_1_0;
    struct vkd3d_shader_static_sampler_desc *samplers = NULL;
    struct vkd3d_shader_root_parameter *parameters = NULL;
    int ret;

    if ((dst_desc->parameter_count = src_desc->parameter_count))
    {
        if (!(parameters = vkd3d_calloc(dst_desc->parameter_count, sizeof(*parameters))))
        {
            ret = VKD3D_ERROR_OUT_OF_MEMORY;
            goto fail;
        }
        if ((ret = convert_root_parameters_to_v_1_0(parameters, src_desc->parameters, src_desc->parameter_count)) < 0)
            goto fail;
    }
    dst_desc->parameters = parameters;
    if ((dst_desc->static_sampler_count = src_desc->static_sampler_count))
    {
        if (!(samplers = vkd3d_calloc(dst_desc->static_sampler_count, sizeof(*samplers))))
        {
            ret = VKD3D_ERROR_OUT_OF_MEMORY;
            goto fail;
        }
        memcpy(samplers, src_desc->static_samplers, src_desc->static_sampler_count * sizeof(*samplers));
    }
    dst_desc->static_samplers = samplers;
    dst_desc->flags = src_desc->flags;

    return VKD3D_OK;

fail:
    free_descriptor_ranges(parameters, dst_desc->parameter_count);
    vkd3d_free(parameters);
    vkd3d_free(samplers);
    return ret;
}

static void free_descriptor_ranges1(const struct vkd3d_shader_root_parameter1 *parameters, unsigned int count)
{
    unsigned int i;

    if (!parameters)
        return;

    for (i = 0; i < count; ++i)
    {
        const struct vkd3d_shader_root_parameter1 *p = &parameters[i];

        if (p->parameter_type == VKD3D_SHADER_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
            vkd3d_free((void *)p->u.descriptor_table.descriptor_ranges);
    }
}

static int convert_root_parameters_to_v_1_1(struct vkd3d_shader_root_parameter1 *dst,
        const struct vkd3d_shader_root_parameter *src, unsigned int count)
{
    const struct vkd3d_shader_descriptor_range *ranges;
    struct vkd3d_shader_descriptor_range1 *ranges1;
    unsigned int i, j;
    int ret;

    for (i = 0; i < count; ++i)
    {
        const struct vkd3d_shader_root_parameter *p = &src[i];
        struct vkd3d_shader_root_parameter1 *p1 = &dst[i];

        p1->parameter_type = p->parameter_type;
        switch (p1->parameter_type)
        {
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
                ranges1 = NULL;
                if ((p1->u.descriptor_table.descriptor_range_count = p->u.descriptor_table.descriptor_range_count))
                {
                    if (!(ranges1 = vkd3d_calloc(p1->u.descriptor_table.descriptor_range_count, sizeof(*ranges1))))
                    {
                        ret = VKD3D_ERROR_OUT_OF_MEMORY;
                        goto fail;
                    }
                }
                p1->u.descriptor_table.descriptor_ranges = ranges1;
                ranges = p->u.descriptor_table.descriptor_ranges;
                for (j = 0; j < p1->u.descriptor_table.descriptor_range_count; ++j)
                {
                    ranges1[j].range_type = ranges[j].range_type;
                    ranges1[j].descriptor_count = ranges[j].descriptor_count;
                    ranges1[j].base_shader_register = ranges[j].base_shader_register;
                    ranges1[j].register_space = ranges[j].register_space;
                    ranges1[j].flags = VKD3D_ROOT_SIGNATURE_1_0_DESCRIPTOR_RANGE_FLAGS;
                    ranges1[j].descriptor_table_offset = ranges[j].descriptor_table_offset;
                }
                break;
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
                p1->u.constants = p->u.constants;
                break;
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_CBV:
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_SRV:
            case VKD3D_SHADER_ROOT_PARAMETER_TYPE_UAV:
                p1->u.descriptor.shader_register = p->u.descriptor.shader_register;
                p1->u.descriptor.register_space = p->u.descriptor.register_space;
                p1->u.descriptor.flags = VKD3D_ROOT_SIGNATURE_1_0_ROOT_DESCRIPTOR_FLAGS;
                break;
            default:
                WARN("Invalid root parameter type %#x.\n", p1->parameter_type);
                ret = VKD3D_ERROR_INVALID_ARGUMENT;
                goto fail;

        }
        p1->shader_visibility = p->shader_visibility;
    }

    return VKD3D_OK;

fail:
    free_descriptor_ranges1(dst, i);
    return ret;
}

static int convert_root_signature_to_v1_1(struct vkd3d_shader_versioned_root_signature_desc *dst,
        const struct vkd3d_shader_versioned_root_signature_desc *src)
{
    const struct vkd3d_shader_root_signature_desc *src_desc = &src->u.v_1_0;
    struct vkd3d_shader_root_signature_desc1 *dst_desc = &dst->u.v_1_1;
    struct vkd3d_shader_static_sampler_desc *samplers = NULL;
    struct vkd3d_shader_root_parameter1 *parameters = NULL;
    int ret;

    if ((dst_desc->parameter_count = src_desc->parameter_count))
    {
        if (!(parameters = vkd3d_calloc(dst_desc->parameter_count, sizeof(*parameters))))
        {
            ret = VKD3D_ERROR_OUT_OF_MEMORY;
            goto fail;
        }
        if ((ret = convert_root_parameters_to_v_1_1(parameters, src_desc->parameters, src_desc->parameter_count)) < 0)
            goto fail;
    }
    dst_desc->parameters = parameters;
    if ((dst_desc->static_sampler_count = src_desc->static_sampler_count))
    {
        if (!(samplers = vkd3d_calloc(dst_desc->static_sampler_count, sizeof(*samplers))))
        {
            ret = VKD3D_ERROR_OUT_OF_MEMORY;
            goto fail;
        }
        memcpy(samplers, src_desc->static_samplers, src_desc->static_sampler_count * sizeof(*samplers));
    }
    dst_desc->static_samplers = samplers;
    dst_desc->flags = src_desc->flags;

    return VKD3D_OK;

fail:
    free_descriptor_ranges1(parameters, dst_desc->parameter_count);
    vkd3d_free(parameters);
    vkd3d_free(samplers);
    return ret;
}

int vkd3d_shader_convert_root_signature(struct vkd3d_shader_versioned_root_signature_desc *dst,
        enum vkd3d_shader_root_signature_version version, const struct vkd3d_shader_versioned_root_signature_desc *src)
{
    int ret;

    TRACE("dst %p, version %#x, src %p.\n", dst, version, src);

    if (src->version == version)
    {
        WARN("Nothing to convert.\n");
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    if (version != VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0 && version != VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_1)
    {
        WARN("Root signature version %#x not supported.\n", version);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    if (src->version != VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0
            && src->version != VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_1)
    {
        WARN("Root signature version %#x not supported.\n", src->version);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    memset(dst, 0, sizeof(*dst));
    dst->version = version;

    if (version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0)
    {
        ret = convert_root_signature_to_v1_0(dst, src);
    }
    else
    {
        VKD3D_ASSERT(version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_1);
        ret = convert_root_signature_to_v1_1(dst, src);
    }

    return ret;
}

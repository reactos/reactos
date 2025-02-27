/*
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
#include "vkd3d_version.h"
#include "hlsl.h"

#include <stdio.h>
#include <math.h>

static inline int char_to_int(char c)
{
    if ('0' <= c && c <= '9')
        return c - '0';
    if ('A' <= c && c <= 'F')
        return c - 'A' + 10;
    if ('a' <= c && c <= 'f')
        return c - 'a' + 10;
    return -1;
}

uint32_t vkd3d_parse_integer(const char *s)
{
    uint32_t base = 10, ret = 0;
    int digit;

    if (*s == '0')
    {
        base = 8;
        ++s;
        if (*s == 'x' || *s == 'X')
        {
            base = 16;
            ++s;
        }
    }

    while ((digit = char_to_int(*s++)) >= 0)
        ret = ret * base + (uint32_t)digit;
    return ret;
}

void vkd3d_string_buffer_init(struct vkd3d_string_buffer *buffer)
{
    buffer->buffer_size = 16;
    buffer->content_size = 0;
    buffer->buffer = vkd3d_malloc(buffer->buffer_size);
    VKD3D_ASSERT(buffer->buffer);
    memset(buffer->buffer, 0, buffer->buffer_size);
}

void vkd3d_string_buffer_cleanup(struct vkd3d_string_buffer *buffer)
{
    vkd3d_free(buffer->buffer);
}

void vkd3d_string_buffer_clear(struct vkd3d_string_buffer *buffer)
{
    vkd3d_string_buffer_truncate(buffer, 0);
}

void vkd3d_string_buffer_truncate(struct vkd3d_string_buffer *buffer, size_t size)
{
    if (size < buffer->content_size)
    {
        buffer->buffer[size] = '\0';
        buffer->content_size = size;
    }
}

static bool vkd3d_string_buffer_resize(struct vkd3d_string_buffer *buffer, int rc)
{
    unsigned int new_buffer_size = rc >= 0 ? buffer->content_size + rc + 1 : buffer->buffer_size * 2;

    if (!vkd3d_array_reserve((void **)&buffer->buffer, &buffer->buffer_size, new_buffer_size, 1))
    {
        ERR("Failed to grow buffer.\n");
        buffer->buffer[buffer->content_size] = '\0';
        return false;
    }
    return true;
}

int vkd3d_string_buffer_vprintf(struct vkd3d_string_buffer *buffer, const char *format, va_list args)
{
    unsigned int rem;
    va_list a;
    int rc;

    for (;;)
    {
        rem = buffer->buffer_size - buffer->content_size;
        va_copy(a, args);
        rc = vsnprintf(&buffer->buffer[buffer->content_size], rem, format, a);
        va_end(a);
        if (rc >= 0 && (unsigned int)rc < rem)
        {
            buffer->content_size += rc;
            return 0;
        }

        if (!vkd3d_string_buffer_resize(buffer, rc))
            return -1;
    }
}

int vkd3d_string_buffer_printf(struct vkd3d_string_buffer *buffer, const char *format, ...)
{
    va_list args;
    int ret;

    va_start(args, format);
    ret = vkd3d_string_buffer_vprintf(buffer, format, args);
    va_end(args);

    return ret;
}

int vkd3d_string_buffer_print_f32(struct vkd3d_string_buffer *buffer, float f)
{
    unsigned int idx = buffer->content_size + 1;
    int ret;

    if (!(ret = vkd3d_string_buffer_printf(buffer, "%.8e", f)) && isfinite(f))
    {
        if (signbit(f))
            ++idx;
        buffer->buffer[idx] = '.';
    }

    return ret;
}

int vkd3d_string_buffer_print_f64(struct vkd3d_string_buffer *buffer, double d)
{
    unsigned int idx = buffer->content_size + 1;
    int ret;

    if (!(ret = vkd3d_string_buffer_printf(buffer, "%.16e", d)) && isfinite(d))
    {
        if (signbit(d))
            ++idx;
        buffer->buffer[idx] = '.';
    }

    return ret;
}

void vkd3d_string_buffer_trace_(const struct vkd3d_string_buffer *buffer, const char *function)
{
    vkd3d_shader_trace_text_(buffer->buffer, buffer->content_size, function);
}

void vkd3d_shader_trace_text_(const char *text, size_t size, const char *function)
{
    const char *p, *q, *end = text + size;

    if (!TRACE_ON())
        return;

    for (p = text; p < end; p = q)
    {
        if (!(q = memchr(p, '\n', end - p)))
            q = end;
        else
            ++q;
        vkd3d_dbg_printf(VKD3D_DBG_LEVEL_TRACE, function, "%.*s", (int)(q - p), p);
    }
}

void vkd3d_string_buffer_cache_init(struct vkd3d_string_buffer_cache *cache)
{
    memset(cache, 0, sizeof(*cache));
}

void vkd3d_string_buffer_cache_cleanup(struct vkd3d_string_buffer_cache *cache)
{
    unsigned int i;

    for (i = 0; i < cache->count; ++i)
    {
        vkd3d_string_buffer_cleanup(cache->buffers[i]);
        vkd3d_free(cache->buffers[i]);
    }
    vkd3d_free(cache->buffers);
    vkd3d_string_buffer_cache_init(cache);
}

struct vkd3d_string_buffer *vkd3d_string_buffer_get(struct vkd3d_string_buffer_cache *cache)
{
    struct vkd3d_string_buffer *buffer;

    if (!cache->count)
    {
        if (!vkd3d_array_reserve((void **)&cache->buffers, &cache->capacity,
                cache->max_count + 1, sizeof(*cache->buffers)))
            return NULL;
        ++cache->max_count;

        if (!(buffer = vkd3d_malloc(sizeof(*buffer))))
            return NULL;
        vkd3d_string_buffer_init(buffer);
    }
    else
    {
        buffer = cache->buffers[--cache->count];
    }
    vkd3d_string_buffer_clear(buffer);
    return buffer;
}

void vkd3d_string_buffer_release(struct vkd3d_string_buffer_cache *cache, struct vkd3d_string_buffer *buffer)
{
    if (!buffer)
        return;
    VKD3D_ASSERT(cache->count + 1 <= cache->max_count);
    cache->buffers[cache->count++] = buffer;
}

void vkd3d_shader_code_from_string_buffer(struct vkd3d_shader_code *code, struct vkd3d_string_buffer *buffer)
{
    code->code = buffer->buffer;
    code->size = buffer->content_size;

    buffer->buffer = NULL;
    buffer->buffer_size = 0;
    buffer->content_size = 0;
}

void vkd3d_shader_message_context_init(struct vkd3d_shader_message_context *context,
        enum vkd3d_shader_log_level log_level)
{
    context->log_level = log_level;
    vkd3d_string_buffer_init(&context->messages);
}

void vkd3d_shader_message_context_cleanup(struct vkd3d_shader_message_context *context)
{
    vkd3d_string_buffer_cleanup(&context->messages);
}

void vkd3d_shader_message_context_trace_messages_(const struct vkd3d_shader_message_context *context,
        const char *function)
{
    vkd3d_string_buffer_trace_(&context->messages, function);
}

bool vkd3d_shader_message_context_copy_messages(struct vkd3d_shader_message_context *context, char **out)
{
    char *messages;

    if (!out)
        return true;

    *out = NULL;

    if (!context->messages.content_size)
        return true;

    if (!(messages = vkd3d_malloc(context->messages.content_size + 1)))
        return false;
    memcpy(messages, context->messages.buffer, context->messages.content_size + 1);
    *out = messages;
    return true;
}

void vkd3d_shader_vnote(struct vkd3d_shader_message_context *context, const struct vkd3d_shader_location *location,
        enum vkd3d_shader_log_level level, const char *format, va_list args)
{
    if (context->log_level < level)
        return;

    if (location)
    {
        const char *source_name = location->source_name ? location->source_name : "<anonymous>";

        if (location->line)
            vkd3d_string_buffer_printf(&context->messages, "%s:%u:%u: ",
                    source_name, location->line, location->column);
        else
            vkd3d_string_buffer_printf(&context->messages, "%s: ", source_name);
    }
    vkd3d_string_buffer_vprintf(&context->messages, format, args);
    vkd3d_string_buffer_printf(&context->messages, "\n");
}

void vkd3d_shader_vwarning(struct vkd3d_shader_message_context *context, const struct vkd3d_shader_location *location,
        enum vkd3d_shader_error error, const char *format, va_list args)
{
    if (context->log_level < VKD3D_SHADER_LOG_WARNING)
        return;

    if (location)
    {
        const char *source_name = location->source_name ? location->source_name : "<anonymous>";

        if (location->line)
            vkd3d_string_buffer_printf(&context->messages, "%s:%u:%u: W%04u: ",
                    source_name, location->line, location->column, error);
        else
            vkd3d_string_buffer_printf(&context->messages, "%s: W%04u: ", source_name, error);
    }
    else
    {
        vkd3d_string_buffer_printf(&context->messages, "W%04u: ", error);
    }
    vkd3d_string_buffer_vprintf(&context->messages, format, args);
    vkd3d_string_buffer_printf(&context->messages, "\n");
}

void vkd3d_shader_warning(struct vkd3d_shader_message_context *context, const struct vkd3d_shader_location *location,
        enum vkd3d_shader_error error, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vkd3d_shader_vwarning(context, location, error, format, args);
    va_end(args);
}

void vkd3d_shader_verror(struct vkd3d_shader_message_context *context, const struct vkd3d_shader_location *location,
        enum vkd3d_shader_error error, const char *format, va_list args)
{
    if (context->log_level < VKD3D_SHADER_LOG_ERROR)
        return;

    if (location)
    {
        const char *source_name = location->source_name ? location->source_name : "<anonymous>";

        if (location->line)
            vkd3d_string_buffer_printf(&context->messages, "%s:%u:%u: E%04u: ",
                    source_name, location->line, location->column, error);
        else
            vkd3d_string_buffer_printf(&context->messages, "%s: E%04u: ", source_name, error);
    }
    else
    {
        vkd3d_string_buffer_printf(&context->messages, "E%04u: ", error);
    }
    vkd3d_string_buffer_vprintf(&context->messages, format, args);
    vkd3d_string_buffer_printf(&context->messages, "\n");
}

void vkd3d_shader_error(struct vkd3d_shader_message_context *context, const struct vkd3d_shader_location *location,
        enum vkd3d_shader_error error, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vkd3d_shader_verror(context, location, error, format, args);
    va_end(args);
}

size_t bytecode_align(struct vkd3d_bytecode_buffer *buffer)
{
    size_t aligned_size = align(buffer->size, 4);

    if (!vkd3d_array_reserve((void **)&buffer->data, &buffer->capacity, aligned_size, 1))
    {
        buffer->status = VKD3D_ERROR_OUT_OF_MEMORY;
        return aligned_size;
    }

    memset(buffer->data + buffer->size, 0xab, aligned_size - buffer->size);
    buffer->size = aligned_size;
    return aligned_size;
}

size_t bytecode_put_bytes_unaligned(struct vkd3d_bytecode_buffer *buffer, const void *bytes, size_t size)
{
    size_t offset = buffer->size;

    if (buffer->status)
        return offset;

    if (!vkd3d_array_reserve((void **)&buffer->data, &buffer->capacity, offset + size, 1))
    {
        buffer->status = VKD3D_ERROR_OUT_OF_MEMORY;
        return offset;
    }
    memcpy(buffer->data + offset, bytes, size);
    buffer->size = offset + size;
    return offset;
}

size_t bytecode_put_bytes(struct vkd3d_bytecode_buffer *buffer, const void *bytes, size_t size)
{
    bytecode_align(buffer);
    return bytecode_put_bytes_unaligned(buffer, bytes, size);
}

size_t bytecode_reserve_bytes(struct vkd3d_bytecode_buffer *buffer, size_t size)
{
    size_t offset = bytecode_align(buffer);

    if (buffer->status)
        return offset;

    if (!vkd3d_array_reserve((void **)&buffer->data, &buffer->capacity, offset + size, 1))
    {
        buffer->status = VKD3D_ERROR_OUT_OF_MEMORY;
        return offset;
    }

    memset(buffer->data + offset, 0, size);
    buffer->size = offset + size;
    return offset;
}

static void bytecode_set_bytes(struct vkd3d_bytecode_buffer *buffer, size_t offset,
        const void *value, size_t size)
{
    if (buffer->status)
        return;

    VKD3D_ASSERT(vkd3d_bound_range(offset, size, buffer->size));
    memcpy(buffer->data + offset, value, size);
}

void set_u32(struct vkd3d_bytecode_buffer *buffer, size_t offset, uint32_t value)
{
    bytecode_set_bytes(buffer, offset, &value, sizeof(value));
}

void set_string(struct vkd3d_bytecode_buffer *buffer, size_t offset, const char *string, size_t length)
{
    bytecode_set_bytes(buffer, offset, string, length);
}

struct shader_dump_data
{
    uint8_t checksum[16];
    const char *path;
    const char *profile;
    const char *source_suffix;
    const char *target_suffix;
};

static void vkd3d_shader_dump_shader(const struct shader_dump_data *dump_data,
        const void *data, size_t size, bool source)
{
    static const char hexadecimal_digits[] = "0123456789abcdef";
    const uint8_t *checksum = dump_data->checksum;
    char str_checksum[33];
    unsigned int pos = 0;
    char filename[1024];
    unsigned int i;
    FILE *f;

    if (!dump_data->path)
        return;

    for (i = 0; i < ARRAY_SIZE(dump_data->checksum); ++i)
    {
        str_checksum[2 * i] = hexadecimal_digits[checksum[i] >> 4];
        str_checksum[2 * i + 1] = hexadecimal_digits[checksum[i] & 0xf];
    }
    str_checksum[32] = '\0';

    pos = snprintf(filename, ARRAY_SIZE(filename), "%s/vkd3d-shader-%s", dump_data->path, str_checksum);

    if (dump_data->profile)
        pos += snprintf(filename + pos, ARRAY_SIZE(filename) - pos, "-%s", dump_data->profile);

    if (source)
        pos += snprintf(filename + pos, ARRAY_SIZE(filename) - pos, "-source.%s", dump_data->source_suffix);
    else
        pos += snprintf(filename + pos, ARRAY_SIZE(filename) - pos, "-target.%s", dump_data->target_suffix);

    TRACE("Dumping shader to \"%s\".\n", filename);
    if ((f = fopen(filename, "wb")))
    {
        if (fwrite(data, 1, size, f) != size)
            WARN("Failed to write shader to %s.\n", filename);
        if (fclose(f))
            WARN("Failed to close stream %s.\n", filename);
    }
    else
    {
        WARN("Failed to open %s for dumping shader.\n", filename);
    }
}

static const char *shader_get_source_type_suffix(enum vkd3d_shader_source_type type)
{
    switch (type)
    {
        case VKD3D_SHADER_SOURCE_DXBC_TPF:
            return "dxbc";
        case VKD3D_SHADER_SOURCE_HLSL:
            return "hlsl";
        case VKD3D_SHADER_SOURCE_D3D_BYTECODE:
            return "d3dbc";
        case VKD3D_SHADER_SOURCE_DXBC_DXIL:
            return "dxil";
        default:
            FIXME("Unhandled source type %#x.\n", type);
            return "bin";
    }
}

static const char *shader_get_target_type_suffix(enum vkd3d_shader_target_type type)
{
    switch (type)
    {
        case VKD3D_SHADER_TARGET_SPIRV_BINARY:
            return "spv";
        case VKD3D_SHADER_TARGET_SPIRV_TEXT:
            return "spv.s";
        case VKD3D_SHADER_TARGET_D3D_ASM:
            return "d3d.s";
        case VKD3D_SHADER_TARGET_D3D_BYTECODE:
            return "d3dbc";
        case VKD3D_SHADER_TARGET_DXBC_TPF:
            return "dxbc";
        case VKD3D_SHADER_TARGET_GLSL:
            return "glsl";
        case VKD3D_SHADER_TARGET_FX:
            return "fx";
        case VKD3D_SHADER_TARGET_MSL:
            return "msl";
        default:
            FIXME("Unhandled target type %#x.\n", type);
            return "bin";
    }
}

static void fill_shader_dump_data(const struct vkd3d_shader_compile_info *compile_info,
        struct shader_dump_data *data)
{
    static bool enabled = true;

    data->path = NULL;

    if (!enabled)
        return;

    if (!(data->path = getenv("VKD3D_SHADER_DUMP_PATH")))
    {
        enabled = false;
        return;
    }

    data->profile = NULL;
    if (compile_info->source_type == VKD3D_SHADER_SOURCE_HLSL)
    {
        const struct vkd3d_shader_hlsl_source_info *hlsl_source_info;

        if ((hlsl_source_info = vkd3d_find_struct(compile_info->next, HLSL_SOURCE_INFO)))
            data->profile = hlsl_source_info->profile;
    }

    vkd3d_compute_md5(compile_info->source.code, compile_info->source.size,
            (uint32_t *)data->checksum, VKD3D_MD5_STANDARD);
    data->source_suffix = shader_get_source_type_suffix(compile_info->source_type);
    data->target_suffix = shader_get_target_type_suffix(compile_info->target_type);
}

static void init_scan_signature_info(const struct vkd3d_shader_compile_info *info)
{
    struct vkd3d_shader_scan_signature_info *signature_info;

    if ((signature_info = vkd3d_find_struct(info->next, SCAN_SIGNATURE_INFO)))
    {
        memset(&signature_info->input, 0, sizeof(signature_info->input));
        memset(&signature_info->output, 0, sizeof(signature_info->output));
        memset(&signature_info->patch_constant, 0, sizeof(signature_info->patch_constant));
    }
}

static const struct vkd3d_debug_option vkd3d_shader_config_options[] =
{
    {"force_validation", VKD3D_SHADER_CONFIG_FLAG_FORCE_VALIDATION}, /* force validation of internal shader representations */
};

uint64_t vkd3d_shader_init_config_flags(void)
{
    uint64_t config_flags;
    const char *config;

    config = getenv("VKD3D_SHADER_CONFIG");
    config_flags = vkd3d_parse_debug_options(config, vkd3d_shader_config_options, ARRAY_SIZE(vkd3d_shader_config_options));

    if (config_flags)
        TRACE("VKD3D_SHADER_CONFIG='%s'.\n", config);

    return config_flags;
}

void vkd3d_shader_parser_init(struct vkd3d_shader_parser *parser, struct vsir_program *program,
        struct vkd3d_shader_message_context *message_context, const char *source_name)
{
    parser->message_context = message_context;
    parser->location.source_name = source_name;
    parser->location.line = 1;
    parser->location.column = 0;
    parser->program = program;
}

void VKD3D_PRINTF_FUNC(3, 4) vkd3d_shader_parser_error(struct vkd3d_shader_parser *parser,
        enum vkd3d_shader_error error, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vkd3d_shader_verror(parser->message_context, &parser->location, error, format, args);
    va_end(args);

    parser->failed = true;
}

void VKD3D_PRINTF_FUNC(3, 4) vkd3d_shader_parser_warning(struct vkd3d_shader_parser *parser,
        enum vkd3d_shader_error error, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vkd3d_shader_vwarning(parser->message_context, &parser->location, error, format, args);
    va_end(args);
}

static int vkd3d_shader_validate_compile_info(const struct vkd3d_shader_compile_info *compile_info,
        bool validate_target_type)
{
    const enum vkd3d_shader_source_type *source_types;
    const enum vkd3d_shader_target_type *target_types;
    unsigned int count, i;

    if (compile_info->type != VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO)
    {
        WARN("Invalid structure type %#x.\n", compile_info->type);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    source_types = vkd3d_shader_get_supported_source_types(&count);
    for (i = 0; i < count; ++i)
    {
        if (source_types[i] == compile_info->source_type)
            break;
    }
    if (i == count)
    {
        WARN("Invalid shader source type %#x.\n", compile_info->source_type);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    if (validate_target_type)
    {
        target_types = vkd3d_shader_get_supported_target_types(compile_info->source_type, &count);
        for (i = 0; i < count; ++i)
        {
            if (target_types[i] == compile_info->target_type)
                break;
        }
        if (i == count)
        {
            WARN("Invalid shader target type %#x.\n", compile_info->target_type);
            return VKD3D_ERROR_INVALID_ARGUMENT;
        }
    }

    return VKD3D_OK;
}

static enum vkd3d_result vsir_parse(const struct vkd3d_shader_compile_info *compile_info, uint64_t config_flags,
        struct vkd3d_shader_message_context *message_context, struct vsir_program *program)
{
    enum vkd3d_result ret;

    switch (compile_info->source_type)
    {
        case VKD3D_SHADER_SOURCE_D3D_BYTECODE:
            ret = d3dbc_parse(compile_info, config_flags, message_context, program);
            break;

        case VKD3D_SHADER_SOURCE_DXBC_TPF:
            ret = tpf_parse(compile_info, config_flags, message_context, program);
            break;

        case VKD3D_SHADER_SOURCE_DXBC_DXIL:
            ret = dxil_parse(compile_info, config_flags, message_context, program);
            break;

        default:
            ERR("Unsupported source type %#x.\n", compile_info->source_type);
            ret = VKD3D_ERROR_INVALID_ARGUMENT;
            break;
    }

    if (ret < 0)
    {
        WARN("Failed to parse shader.\n");
        return ret;
    }

    if ((ret = vsir_program_validate(program, config_flags, compile_info->source_name, message_context)) < 0)
    {
        WARN("Failed to validate shader after parsing, ret %d.\n", ret);

        if (TRACE_ON())
            vsir_program_trace(program);

        vsir_program_cleanup(program);
        return ret;
    }

    if (compile_info->target_type != VKD3D_SHADER_TARGET_NONE)
        ret = vsir_program_transform_early(program, config_flags, compile_info, message_context);
    return ret;
}

void vkd3d_shader_free_messages(char *messages)
{
    TRACE("messages %p.\n", messages);

    vkd3d_free(messages);
}

static bool vkd3d_shader_signature_from_shader_signature(struct vkd3d_shader_signature *signature,
        const struct shader_signature *src)
{
    unsigned int i;

    signature->element_count = src->element_count;
    if (!src->elements)
    {
        VKD3D_ASSERT(!signature->element_count);
        signature->elements = NULL;
        return true;
    }

    if (!(signature->elements = vkd3d_calloc(signature->element_count, sizeof(*signature->elements))))
        return false;

    for (i = 0; i < signature->element_count; ++i)
    {
        struct vkd3d_shader_signature_element *d = &signature->elements[i];
        struct signature_element *e = &src->elements[i];

        if (!(d->semantic_name = vkd3d_strdup(e->semantic_name)))
        {
            for (unsigned int j = 0; j < i; ++j)
            {
                vkd3d_free((void *)signature->elements[j].semantic_name);
            }
            vkd3d_free(signature->elements);
            return false;
        }
        d->semantic_index = e->semantic_index;
        d->stream_index = e->stream_index;
        d->sysval_semantic = e->sysval_semantic;
        d->component_type = e->component_type;
        d->register_index = e->register_index;
        if (e->register_count > 1)
            FIXME("Arrayed elements are not supported yet.\n");
        d->mask = e->mask;
        d->used_mask = e->used_mask;
        d->min_precision = e->min_precision;
    }

    return true;
}

struct vkd3d_shader_scan_context
{
    const struct vkd3d_shader_version *version;

    struct vkd3d_shader_scan_descriptor_info1 *scan_descriptor_info;
    size_t descriptors_size;

    struct vkd3d_shader_message_context *message_context;
    struct vkd3d_shader_location location;

    struct vkd3d_shader_cf_info
    {
        enum
        {
            VKD3D_SHADER_BLOCK_IF,
            VKD3D_SHADER_BLOCK_LOOP,
            VKD3D_SHADER_BLOCK_SWITCH,
        } type;
        bool inside_block;
        bool has_default;
    } *cf_info;
    size_t cf_info_size;
    size_t cf_info_count;

    enum vkd3d_shader_api_version api_version;

    struct vkd3d_shader_scan_combined_resource_sampler_info *combined_sampler_info;
    size_t combined_samplers_size;
};

static VKD3D_PRINTF_FUNC(3, 4) void vkd3d_shader_scan_error(struct vkd3d_shader_scan_context *context,
        enum vkd3d_shader_error error, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vkd3d_shader_verror(context->message_context, &context->location, error, format, args);
    va_end(args);
}

static void VKD3D_PRINTF_FUNC(3, 4) vkd3d_shader_scan_warning(struct vkd3d_shader_scan_context *context,
        enum vkd3d_shader_error error, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vkd3d_shader_vwarning(context->message_context, &context->location, error, format, args);
    va_end(args);
}

static void vkd3d_shader_scan_context_init(struct vkd3d_shader_scan_context *context,
        const struct vkd3d_shader_version *version,
        const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_scan_descriptor_info1 *scan_descriptor_info,
        struct vkd3d_shader_scan_combined_resource_sampler_info *combined_sampler_info,
        struct vkd3d_shader_message_context *message_context)
{
    unsigned int i;

    memset(context, 0, sizeof(*context));
    context->version = version;
    context->scan_descriptor_info = scan_descriptor_info;
    context->message_context = message_context;
    context->location.source_name = compile_info->source_name;
    context->location.line = 2; /* Line 1 is the version token. */
    context->api_version = VKD3D_SHADER_API_VERSION_1_2;
    context->combined_sampler_info = combined_sampler_info;

    for (i = 0; i < compile_info->option_count; ++i)
    {
        const struct vkd3d_shader_compile_option *option = &compile_info->options[i];

        if (option->name == VKD3D_SHADER_COMPILE_OPTION_API_VERSION)
            context->api_version = option->value;
    }
}

static void vkd3d_shader_scan_context_cleanup(struct vkd3d_shader_scan_context *context)
{
    vkd3d_free(context->cf_info);
}

static struct vkd3d_shader_cf_info *vkd3d_shader_scan_get_current_cf_info(struct vkd3d_shader_scan_context *context)
{
    if (!context->cf_info_count)
        return NULL;
    return &context->cf_info[context->cf_info_count - 1];
}

static struct vkd3d_shader_cf_info *vkd3d_shader_scan_push_cf_info(struct vkd3d_shader_scan_context *context)
{
    struct vkd3d_shader_cf_info *cf_info;

    if (!vkd3d_array_reserve((void **)&context->cf_info, &context->cf_info_size,
            context->cf_info_count + 1, sizeof(*context->cf_info)))
    {
        ERR("Failed to allocate UAV range.\n");
        return false;
    }

    cf_info = &context->cf_info[context->cf_info_count++];
    memset(cf_info, 0, sizeof(*cf_info));

    return cf_info;
}

static void vkd3d_shader_scan_pop_cf_info(struct vkd3d_shader_scan_context *context)
{
    VKD3D_ASSERT(context->cf_info_count);

    --context->cf_info_count;
}

static struct vkd3d_shader_cf_info *vkd3d_shader_scan_find_innermost_breakable_cf_info(
        struct vkd3d_shader_scan_context *context)
{
    size_t count = context->cf_info_count;
    struct vkd3d_shader_cf_info *cf_info;

    while (count)
    {
        cf_info = &context->cf_info[--count];
        if (cf_info->type == VKD3D_SHADER_BLOCK_LOOP
                || cf_info->type == VKD3D_SHADER_BLOCK_SWITCH)
            return cf_info;
    }

    return NULL;
}

static struct vkd3d_shader_cf_info *vkd3d_shader_scan_find_innermost_loop_cf_info(
        struct vkd3d_shader_scan_context *context)
{
    size_t count = context->cf_info_count;
    struct vkd3d_shader_cf_info *cf_info;

    while (count)
    {
        cf_info = &context->cf_info[--count];
        if (cf_info->type == VKD3D_SHADER_BLOCK_LOOP)
            return cf_info;
    }

    return NULL;
}

static void vkd3d_shader_scan_add_uav_flag(const struct vkd3d_shader_scan_context *context,
        const struct vkd3d_shader_register *reg, uint32_t flag)
{
    unsigned int range_id = reg->idx[0].offset;
    unsigned int i;

    if (!context->scan_descriptor_info)
        return;

    for (i = 0; i < context->scan_descriptor_info->descriptor_count; ++i)
    {
        if (context->scan_descriptor_info->descriptors[i].type == VKD3D_SHADER_DESCRIPTOR_TYPE_UAV
                && context->scan_descriptor_info->descriptors[i].register_id == range_id)
        {
            context->scan_descriptor_info->descriptors[i].flags |= flag;
            break;
        }
    }
}

static bool vkd3d_shader_instruction_is_uav_read(const struct vkd3d_shader_instruction *instruction)
{
    enum vkd3d_shader_opcode opcode = instruction->opcode;

    return (VKD3DSIH_ATOMIC_AND <= opcode && opcode <= VKD3DSIH_ATOMIC_XOR)
            || (VKD3DSIH_IMM_ATOMIC_ALLOC <= opcode && opcode <= VKD3DSIH_IMM_ATOMIC_XOR)
            || opcode == VKD3DSIH_LD_UAV_TYPED
            || (opcode == VKD3DSIH_LD_RAW && instruction->src[1].reg.type == VKD3DSPR_UAV)
            || (opcode == VKD3DSIH_LD_STRUCTURED && instruction->src[2].reg.type == VKD3DSPR_UAV);
}

static void vkd3d_shader_scan_record_uav_read(struct vkd3d_shader_scan_context *context,
        const struct vkd3d_shader_register *reg)
{
    vkd3d_shader_scan_add_uav_flag(context, reg, VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_UAV_READ);
}

static bool vkd3d_shader_instruction_is_uav_counter(const struct vkd3d_shader_instruction *instruction)
{
    enum vkd3d_shader_opcode opcode = instruction->opcode;

    return opcode == VKD3DSIH_IMM_ATOMIC_ALLOC || opcode == VKD3DSIH_IMM_ATOMIC_CONSUME;
}

static void vkd3d_shader_scan_record_uav_counter(struct vkd3d_shader_scan_context *context,
        const struct vkd3d_shader_register *reg)
{
    vkd3d_shader_scan_add_uav_flag(context, reg, VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_UAV_COUNTER);
}

static bool vkd3d_shader_instruction_is_uav_atomic_op(const struct vkd3d_shader_instruction *instruction)
{
    enum vkd3d_shader_opcode opcode = instruction->opcode;

    return (VKD3DSIH_ATOMIC_AND <= opcode && opcode <= VKD3DSIH_ATOMIC_XOR)
            || (VKD3DSIH_IMM_ATOMIC_ALLOC <= opcode && opcode <= VKD3DSIH_IMM_ATOMIC_XOR);
}

static void vkd3d_shader_scan_record_uav_atomic_op(struct vkd3d_shader_scan_context *context,
        const struct vkd3d_shader_register *reg)
{
    vkd3d_shader_scan_add_uav_flag(context, reg, VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_UAV_ATOMICS);
}

static struct vkd3d_shader_descriptor_info1 *vkd3d_shader_scan_add_descriptor(struct vkd3d_shader_scan_context *context,
        enum vkd3d_shader_descriptor_type type, const struct vkd3d_shader_register *reg,
        const struct vkd3d_shader_register_range *range, enum vkd3d_shader_resource_type resource_type,
        enum vkd3d_shader_resource_data_type resource_data_type)
{
    struct vkd3d_shader_scan_descriptor_info1 *info = context->scan_descriptor_info;
    struct vkd3d_shader_descriptor_info1 *d;

    if (!info)
        return NULL;

    if (!vkd3d_array_reserve((void **)&info->descriptors, &context->descriptors_size,
            info->descriptor_count + 1, sizeof(*info->descriptors)))
    {
        ERR("Failed to allocate descriptor info.\n");
        return NULL;
    }

    d = &info->descriptors[info->descriptor_count];
    memset(d, 0, sizeof(*d));
    d->type = type;
    d->register_id = reg->idx[0].offset;
    d->register_space = range->space;
    d->register_index = range->first;
    d->resource_type = resource_type;
    d->resource_data_type = resource_data_type;
    d->count = (range->last == ~0u) ? ~0u : range->last - range->first + 1;
    ++info->descriptor_count;

    return d;
}

static void vkd3d_shader_scan_constant_buffer_declaration(struct vkd3d_shader_scan_context *context,
        const struct vkd3d_shader_instruction *instruction)
{
    const struct vkd3d_shader_constant_buffer *cb = &instruction->declaration.cb;
    struct vkd3d_shader_descriptor_info1 *d;

    if (!(d = vkd3d_shader_scan_add_descriptor(context, VKD3D_SHADER_DESCRIPTOR_TYPE_CBV,
            &cb->src.reg, &cb->range, VKD3D_SHADER_RESOURCE_BUFFER, VKD3D_SHADER_RESOURCE_DATA_UINT)))
        return;
    d->buffer_size = cb->size;
}

static void vkd3d_shader_scan_sampler_declaration(struct vkd3d_shader_scan_context *context,
        const struct vkd3d_shader_instruction *instruction)
{
    const struct vkd3d_shader_sampler *sampler = &instruction->declaration.sampler;
    struct vkd3d_shader_descriptor_info1 *d;

    if (!(d = vkd3d_shader_scan_add_descriptor(context, VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER,
            &sampler->src.reg, &sampler->range, VKD3D_SHADER_RESOURCE_NONE, VKD3D_SHADER_RESOURCE_DATA_UINT)))
        return;

    if (instruction->flags & VKD3DSI_SAMPLER_COMPARISON_MODE)
        d->flags |= VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_SAMPLER_COMPARISON_MODE;
}

static void vkd3d_shader_scan_combined_sampler_declaration(
        struct vkd3d_shader_scan_context *context, const struct vkd3d_shader_semantic *semantic)
{
    vkd3d_shader_scan_add_descriptor(context, VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER, &semantic->resource.reg.reg,
            &semantic->resource.range, VKD3D_SHADER_RESOURCE_NONE, VKD3D_SHADER_RESOURCE_DATA_UINT);
    vkd3d_shader_scan_add_descriptor(context, VKD3D_SHADER_DESCRIPTOR_TYPE_SRV, &semantic->resource.reg.reg,
            &semantic->resource.range, semantic->resource_type, VKD3D_SHADER_RESOURCE_DATA_FLOAT);
}

static const struct vkd3d_shader_descriptor_info1 *find_descriptor(
        const struct vkd3d_shader_scan_descriptor_info1 *info,
        enum vkd3d_shader_descriptor_type type, unsigned int register_id)
{
    for (unsigned int i = 0; i < info->descriptor_count; ++i)
    {
        const struct vkd3d_shader_descriptor_info1 *d = &info->descriptors[i];

        if (d->type == type && d->register_id == register_id)
            return d;
    }

    return NULL;
}

static void vkd3d_shader_scan_combined_sampler_usage(struct vkd3d_shader_scan_context *context,
        const struct vkd3d_shader_register *resource, const struct vkd3d_shader_register *sampler)
{
    struct vkd3d_shader_scan_combined_resource_sampler_info *info;
    struct vkd3d_shader_combined_resource_sampler_info *s;
    unsigned resource_space = 0, sampler_space = 0;
    unsigned int resource_idx, sampler_idx, i;

    if (!(info = context->combined_sampler_info))
        return;

    if (resource->type == VKD3DSPR_RESOURCE)
        resource_idx = resource->idx[1].offset;
    else
        resource_idx = resource->idx[0].offset;

    if (!sampler)
        sampler_idx = VKD3D_SHADER_DUMMY_SAMPLER_INDEX;
    else if (sampler->type == VKD3DSPR_SAMPLER)
        sampler_idx = sampler->idx[1].offset;
    else
        sampler_idx = sampler->idx[0].offset;

    if (vkd3d_shader_ver_ge(context->version, 5, 1))
    {
        const struct vkd3d_shader_descriptor_info1 *d;
        bool dynamic_resource, dynamic_sampler;

        if ((dynamic_resource = resource->idx[1].rel_addr))
            vkd3d_shader_scan_warning(context, VKD3D_SHADER_WARNING_VSIR_DYNAMIC_DESCRIPTOR_ARRAY,
                    "Resource descriptor array %u is being dynamically indexed, "
                    "not recording a combined resource-sampler pair.", resource->idx[0].offset);
        if ((dynamic_sampler = sampler && sampler->idx[1].rel_addr))
            vkd3d_shader_scan_warning(context, VKD3D_SHADER_WARNING_VSIR_DYNAMIC_DESCRIPTOR_ARRAY,
                    "Sampler descriptor array %u is being dynamically indexed, "
                    "not recording a combined resource-sampler pair.", sampler->idx[0].offset);
        if (dynamic_resource || dynamic_sampler)
            return;

        if ((d = find_descriptor(context->scan_descriptor_info,
                VKD3D_SHADER_DESCRIPTOR_TYPE_SRV, resource->idx[0].offset)))
            resource_space = d->register_space;

        if (sampler && (d = find_descriptor(context->scan_descriptor_info,
                VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER, sampler->idx[0].offset)))
            sampler_space = d->register_space;
    }

    for (i = 0; i < info->combined_sampler_count; ++i)
    {
        s = &info->combined_samplers[i];
        if (s->resource_space == resource_space && s->resource_index == resource_idx
                && s->sampler_space == sampler_space && s->sampler_index == sampler_idx)
            return;
    }

    if (!vkd3d_array_reserve((void **)&info->combined_samplers, &context->combined_samplers_size,
            info->combined_sampler_count + 1, sizeof(*info->combined_samplers)))
    {
        ERR("Failed to allocate combined sampler info.\n");
        return;
    }

    s = &info->combined_samplers[info->combined_sampler_count++];
    s->resource_space = resource_space;
    s->resource_index = resource_idx;
    s->sampler_space = sampler_space;
    s->sampler_index = sampler_idx;
}

static void vkd3d_shader_scan_resource_declaration(struct vkd3d_shader_scan_context *context,
        const struct vkd3d_shader_resource *resource, enum vkd3d_shader_resource_type resource_type,
        enum vkd3d_shader_resource_data_type resource_data_type,
        unsigned int sample_count, unsigned int structure_stride, bool raw, uint32_t flags)
{
    struct vkd3d_shader_descriptor_info1 *d;
    enum vkd3d_shader_descriptor_type type;

    if (resource->reg.reg.type == VKD3DSPR_UAV)
        type = VKD3D_SHADER_DESCRIPTOR_TYPE_UAV;
    else
        type = VKD3D_SHADER_DESCRIPTOR_TYPE_SRV;
    if (!(d = vkd3d_shader_scan_add_descriptor(context, type, &resource->reg.reg,
            &resource->range, resource_type, resource_data_type)))
        return;
    d->sample_count = sample_count;
    d->structure_stride = structure_stride;
    if (raw)
        d->flags |= VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_RAW_BUFFER;
    if (type == VKD3D_SHADER_DESCRIPTOR_TYPE_UAV)
        d->uav_flags = flags;
}

static void vkd3d_shader_scan_typed_resource_declaration(struct vkd3d_shader_scan_context *context,
        const struct vkd3d_shader_instruction *instruction)
{
    const struct vkd3d_shader_semantic *semantic = &instruction->declaration.semantic;
    enum vkd3d_shader_resource_data_type resource_data_type;

    if (semantic->resource_data_type[0] != semantic->resource_data_type[1] ||
        semantic->resource_data_type[0] != semantic->resource_data_type[2] ||
        semantic->resource_data_type[0] != semantic->resource_data_type[3])
        FIXME("Resource data types are different (%d, %d, %d, %d).\n",
                semantic->resource_data_type[0],
                semantic->resource_data_type[1],
                semantic->resource_data_type[2],
                semantic->resource_data_type[3]);

    switch (semantic->resource_data_type[0])
    {
        case VKD3D_DATA_UNORM:
            resource_data_type = VKD3D_SHADER_RESOURCE_DATA_UNORM;
            break;
        case VKD3D_DATA_SNORM:
            resource_data_type = VKD3D_SHADER_RESOURCE_DATA_SNORM;
            break;
        case VKD3D_DATA_INT:
            resource_data_type = VKD3D_SHADER_RESOURCE_DATA_INT;
            break;
        case VKD3D_DATA_UINT:
            resource_data_type = VKD3D_SHADER_RESOURCE_DATA_UINT;
            break;
        case VKD3D_DATA_FLOAT:
            resource_data_type = VKD3D_SHADER_RESOURCE_DATA_FLOAT;
            break;
        case VKD3D_DATA_MIXED:
            resource_data_type = VKD3D_SHADER_RESOURCE_DATA_MIXED;
            break;
        case VKD3D_DATA_DOUBLE:
            resource_data_type = VKD3D_SHADER_RESOURCE_DATA_DOUBLE;
            break;
        case VKD3D_DATA_CONTINUED:
            resource_data_type = VKD3D_SHADER_RESOURCE_DATA_CONTINUED;
            break;
        default:
            ERR("Invalid resource data type %#x.\n", semantic->resource_data_type[0]);
            resource_data_type = VKD3D_SHADER_RESOURCE_DATA_FLOAT;
            break;
    }

    if (context->api_version < VKD3D_SHADER_API_VERSION_1_3
            && resource_data_type >= VKD3D_SHADER_RESOURCE_DATA_MIXED)
    {
        ERR("Invalid resource data type %#x for API version %#x.\n",
                semantic->resource_data_type[0], context->api_version);
        resource_data_type = VKD3D_SHADER_RESOURCE_DATA_FLOAT;
    }

    vkd3d_shader_scan_resource_declaration(context, &semantic->resource,
            semantic->resource_type, resource_data_type, semantic->sample_count, 0, false, instruction->flags);
}

static int vkd3d_shader_scan_instruction(struct vkd3d_shader_scan_context *context,
        const struct vkd3d_shader_instruction *instruction)
{
    const struct vkd3d_shader_register *sampler_reg;
    struct vkd3d_shader_cf_info *cf_info;
    unsigned int i;

    context->location = instruction->location;

    switch (instruction->opcode)
    {
        case VKD3DSIH_DCL_CONSTANT_BUFFER:
            vkd3d_shader_scan_constant_buffer_declaration(context, instruction);
            break;
        case VKD3DSIH_DCL_SAMPLER:
            vkd3d_shader_scan_sampler_declaration(context, instruction);
            break;
        case VKD3DSIH_DCL:
            if (instruction->declaration.semantic.resource_type == VKD3D_SHADER_RESOURCE_NONE)
                break;

            if (instruction->declaration.semantic.resource.reg.reg.type == VKD3DSPR_COMBINED_SAMPLER)
            {
                vkd3d_shader_scan_combined_sampler_declaration(context, &instruction->declaration.semantic);
                break;
            }
            /* fall through */
        case VKD3DSIH_DCL_UAV_TYPED:
            vkd3d_shader_scan_typed_resource_declaration(context, instruction);
            break;
        case VKD3DSIH_DCL_RESOURCE_RAW:
        case VKD3DSIH_DCL_UAV_RAW:
            vkd3d_shader_scan_resource_declaration(context, &instruction->declaration.raw_resource.resource,
                    VKD3D_SHADER_RESOURCE_BUFFER, VKD3D_SHADER_RESOURCE_DATA_UINT, 0, 0, true, instruction->flags);
            break;
        case VKD3DSIH_DCL_RESOURCE_STRUCTURED:
        case VKD3DSIH_DCL_UAV_STRUCTURED:
            vkd3d_shader_scan_resource_declaration(context, &instruction->declaration.structured_resource.resource,
                    VKD3D_SHADER_RESOURCE_BUFFER, VKD3D_SHADER_RESOURCE_DATA_UINT, 0,
                    instruction->declaration.structured_resource.byte_stride, false, instruction->flags);
            break;
        case VKD3DSIH_IF:
        case VKD3DSIH_IFC:
            cf_info = vkd3d_shader_scan_push_cf_info(context);
            cf_info->type = VKD3D_SHADER_BLOCK_IF;
            cf_info->inside_block = true;
            break;
        case VKD3DSIH_ELSE:
            if (!(cf_info = vkd3d_shader_scan_get_current_cf_info(context)) || cf_info->type != VKD3D_SHADER_BLOCK_IF)
            {
                vkd3d_shader_scan_error(context, VKD3D_SHADER_ERROR_TPF_MISMATCHED_CF,
                        "Encountered ‘else’ instruction without corresponding ‘if’ block.");
                return VKD3D_ERROR_INVALID_SHADER;
            }
            cf_info->inside_block = true;
            break;
        case VKD3DSIH_ENDIF:
            if (!(cf_info = vkd3d_shader_scan_get_current_cf_info(context)) || cf_info->type != VKD3D_SHADER_BLOCK_IF)
            {
                vkd3d_shader_scan_error(context, VKD3D_SHADER_ERROR_TPF_MISMATCHED_CF,
                        "Encountered ‘endif’ instruction without corresponding ‘if’ block.");
                return VKD3D_ERROR_INVALID_SHADER;
            }
            vkd3d_shader_scan_pop_cf_info(context);
            break;
        case VKD3DSIH_LOOP:
            cf_info = vkd3d_shader_scan_push_cf_info(context);
            cf_info->type = VKD3D_SHADER_BLOCK_LOOP;
            cf_info->inside_block = true;
            break;
        case VKD3DSIH_ENDLOOP:
            if (!(cf_info = vkd3d_shader_scan_get_current_cf_info(context)) || cf_info->type != VKD3D_SHADER_BLOCK_LOOP)
            {
                vkd3d_shader_scan_error(context, VKD3D_SHADER_ERROR_TPF_MISMATCHED_CF,
                        "Encountered ‘endloop’ instruction without corresponding ‘loop’ block.");
                return VKD3D_ERROR_INVALID_SHADER;
            }
            vkd3d_shader_scan_pop_cf_info(context);
            break;
        case VKD3DSIH_SWITCH:
            cf_info = vkd3d_shader_scan_push_cf_info(context);
            cf_info->type = VKD3D_SHADER_BLOCK_SWITCH;
            break;
        case VKD3DSIH_ENDSWITCH:
            if (!(cf_info = vkd3d_shader_scan_get_current_cf_info(context))
                    || cf_info->type != VKD3D_SHADER_BLOCK_SWITCH || cf_info->inside_block)
            {
                vkd3d_shader_scan_error(context, VKD3D_SHADER_ERROR_TPF_MISMATCHED_CF,
                        "Encountered ‘endswitch’ instruction without corresponding ‘switch’ block.");
                return VKD3D_ERROR_INVALID_SHADER;
            }
            vkd3d_shader_scan_pop_cf_info(context);
            break;
        case VKD3DSIH_CASE:
            if (!(cf_info = vkd3d_shader_scan_get_current_cf_info(context))
                    || cf_info->type != VKD3D_SHADER_BLOCK_SWITCH)
            {
                vkd3d_shader_scan_error(context, VKD3D_SHADER_ERROR_TPF_MISMATCHED_CF,
                        "Encountered ‘case’ instruction outside switch block.");
                return VKD3D_ERROR_INVALID_SHADER;
            }
            cf_info->inside_block = true;
            break;
        case VKD3DSIH_DEFAULT:
            if (!(cf_info = vkd3d_shader_scan_get_current_cf_info(context))
                    || cf_info->type != VKD3D_SHADER_BLOCK_SWITCH)
            {
                vkd3d_shader_scan_error(context, VKD3D_SHADER_ERROR_TPF_MISMATCHED_CF,
                        "Encountered ‘default’ instruction outside switch block.");
                return VKD3D_ERROR_INVALID_SHADER;
            }
            if (cf_info->has_default)
            {
                vkd3d_shader_scan_error(context, VKD3D_SHADER_ERROR_TPF_MISMATCHED_CF,
                        "Encountered duplicate ‘default’ instruction inside the current switch block.");
                return VKD3D_ERROR_INVALID_SHADER;
            }
            cf_info->inside_block = true;
            cf_info->has_default = true;
            break;
        case VKD3DSIH_BREAK:
            if (!(cf_info = vkd3d_shader_scan_find_innermost_breakable_cf_info(context)))
            {
                vkd3d_shader_scan_error(context, VKD3D_SHADER_ERROR_TPF_MISMATCHED_CF,
                        "Encountered ‘break’ instruction outside breakable block.");
                return VKD3D_ERROR_INVALID_SHADER;
            }
            cf_info->inside_block = false;
            break;
        case VKD3DSIH_BREAKP:
            if (!(cf_info = vkd3d_shader_scan_find_innermost_loop_cf_info(context)))
            {
                vkd3d_shader_scan_error(context, VKD3D_SHADER_ERROR_TPF_MISMATCHED_CF,
                        "Encountered ‘breakp’ instruction outside loop.");
                return VKD3D_ERROR_INVALID_SHADER;
            }
            break;
        case VKD3DSIH_CONTINUE:
            if (!(cf_info = vkd3d_shader_scan_find_innermost_loop_cf_info(context)))
            {
                vkd3d_shader_scan_error(context, VKD3D_SHADER_ERROR_TPF_MISMATCHED_CF,
                        "Encountered ‘continue’ instruction outside loop.");
                return VKD3D_ERROR_INVALID_SHADER;
            }
            cf_info->inside_block = false;
            break;
        case VKD3DSIH_CONTINUEP:
            if (!(cf_info = vkd3d_shader_scan_find_innermost_loop_cf_info(context)))
            {
                vkd3d_shader_scan_error(context, VKD3D_SHADER_ERROR_TPF_MISMATCHED_CF,
                        "Encountered ‘continue’ instruction outside loop.");
                return VKD3D_ERROR_INVALID_SHADER;
            }
            break;
        case VKD3DSIH_RET:
            if (context->cf_info_count)
                context->cf_info[context->cf_info_count - 1].inside_block = false;
            break;
        case VKD3DSIH_TEX:
            if (context->version->major == 1)
                sampler_reg = &instruction->dst[0].reg;
            else
                sampler_reg = &instruction->src[1].reg;
            vkd3d_shader_scan_combined_sampler_usage(context, sampler_reg, sampler_reg);
            break;
        case VKD3DSIH_TEXBEM:
        case VKD3DSIH_TEXBEML:
        case VKD3DSIH_TEXDP3TEX:
        case VKD3DSIH_TEXM3x2TEX:
        case VKD3DSIH_TEXM3x3SPEC:
        case VKD3DSIH_TEXM3x3TEX:
        case VKD3DSIH_TEXM3x3VSPEC:
        case VKD3DSIH_TEXREG2AR:
        case VKD3DSIH_TEXREG2GB:
        case VKD3DSIH_TEXREG2RGB:
            sampler_reg = &instruction->dst[0].reg;
            vkd3d_shader_scan_combined_sampler_usage(context, sampler_reg, sampler_reg);
            break;
        case VKD3DSIH_GATHER4:
        case VKD3DSIH_GATHER4_C:
        case VKD3DSIH_SAMPLE:
        case VKD3DSIH_SAMPLE_B:
        case VKD3DSIH_SAMPLE_C:
        case VKD3DSIH_SAMPLE_C_LZ:
        case VKD3DSIH_SAMPLE_GRAD:
        case VKD3DSIH_SAMPLE_LOD:
            vkd3d_shader_scan_combined_sampler_usage(context, &instruction->src[1].reg, &instruction->src[2].reg);
            break;
        case VKD3DSIH_GATHER4_PO:
        case VKD3DSIH_GATHER4_PO_C:
            vkd3d_shader_scan_combined_sampler_usage(context, &instruction->src[2].reg, &instruction->src[3].reg);
            break;
        case VKD3DSIH_LD:
        case VKD3DSIH_LD2DMS:
            vkd3d_shader_scan_combined_sampler_usage(context, &instruction->src[1].reg, NULL);
            break;
        case VKD3DSIH_BUFINFO:
        case VKD3DSIH_SAMPLE_INFO:
            if (instruction->src[0].reg.type == VKD3DSPR_RESOURCE)
                vkd3d_shader_scan_combined_sampler_usage(context, &instruction->src[0].reg, NULL);
            break;
        case VKD3DSIH_LD_RAW:
        case VKD3DSIH_RESINFO:
            if (instruction->src[1].reg.type == VKD3DSPR_RESOURCE)
                vkd3d_shader_scan_combined_sampler_usage(context, &instruction->src[1].reg, NULL);
            break;
        case VKD3DSIH_LD_STRUCTURED:
            if (instruction->src[2].reg.type == VKD3DSPR_RESOURCE)
                vkd3d_shader_scan_combined_sampler_usage(context, &instruction->src[2].reg, NULL);
            break;
        default:
            break;
    }

    if (vkd3d_shader_instruction_is_uav_read(instruction))
    {
        for (i = 0; i < instruction->dst_count; ++i)
        {
            if (instruction->dst[i].reg.type == VKD3DSPR_UAV)
                vkd3d_shader_scan_record_uav_read(context, &instruction->dst[i].reg);
        }
        for (i = 0; i < instruction->src_count; ++i)
        {
            if (instruction->src[i].reg.type == VKD3DSPR_UAV)
                vkd3d_shader_scan_record_uav_read(context, &instruction->src[i].reg);
        }
    }

    if (vkd3d_shader_instruction_is_uav_counter(instruction))
        vkd3d_shader_scan_record_uav_counter(context, &instruction->src[0].reg);

    if (vkd3d_shader_instruction_is_uav_atomic_op(instruction))
    {
        for (i = 0; i < instruction->dst_count; ++i)
        {
            if (instruction->dst[i].reg.type == VKD3DSPR_UAV)
                vkd3d_shader_scan_record_uav_atomic_op(context, &instruction->dst[i].reg);
        }
    }

    return VKD3D_OK;
}

static enum vkd3d_result convert_descriptor_info(struct vkd3d_shader_scan_descriptor_info *info,
        const struct vkd3d_shader_scan_descriptor_info1 *info1)
{
    unsigned int i;

    if (!(info->descriptors = vkd3d_calloc(info1->descriptor_count, sizeof(*info->descriptors))))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    for (i = 0; i < info1->descriptor_count; ++i)
    {
        const struct vkd3d_shader_descriptor_info1 *src = &info1->descriptors[i];
        struct vkd3d_shader_descriptor_info *dst = &info->descriptors[i];

        dst->type = src->type;
        dst->register_space = src->register_space;
        dst->register_index = src->register_index;
        dst->resource_type = src->resource_type;
        dst->resource_data_type = src->resource_data_type;
        dst->flags = src->flags;
        dst->count = src->count;
    }
    info->descriptor_count = info1->descriptor_count;

    return VKD3D_OK;
}

static void vkd3d_shader_free_scan_descriptor_info1(struct vkd3d_shader_scan_descriptor_info1 *scan_descriptor_info)
{
    TRACE("scan_descriptor_info %p.\n", scan_descriptor_info);

    vkd3d_free(scan_descriptor_info->descriptors);
}

static int vsir_program_scan(struct vsir_program *program, const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_message_context *message_context,
        struct vkd3d_shader_scan_descriptor_info1 *descriptor_info1)
{
    struct vkd3d_shader_scan_combined_resource_sampler_info *combined_sampler_info;
    struct vkd3d_shader_scan_descriptor_info1 local_descriptor_info1 = {0};
    struct vkd3d_shader_scan_descriptor_info *descriptor_info;
    struct vkd3d_shader_scan_signature_info *signature_info;
    struct vkd3d_shader_instruction *instruction;
    struct vkd3d_shader_scan_context context;
    int ret = VKD3D_OK;
    unsigned int i;

    descriptor_info = vkd3d_find_struct(compile_info->next, SCAN_DESCRIPTOR_INFO);
    if (descriptor_info1)
    {
        descriptor_info1->descriptors = NULL;
        descriptor_info1->descriptor_count = 0;
    }
    else if (descriptor_info)
    {
        descriptor_info1 = &local_descriptor_info1;
    }
    signature_info = vkd3d_find_struct(compile_info->next, SCAN_SIGNATURE_INFO);

    if ((combined_sampler_info = vkd3d_find_struct(compile_info->next, SCAN_COMBINED_RESOURCE_SAMPLER_INFO)))
    {
        combined_sampler_info->combined_samplers = NULL;
        combined_sampler_info->combined_sampler_count = 0;
        if (!descriptor_info1)
            descriptor_info1 = &local_descriptor_info1;
    }

    vkd3d_shader_scan_context_init(&context, &program->shader_version, compile_info,
            descriptor_info1, combined_sampler_info, message_context);

    if (TRACE_ON())
        vsir_program_trace(program);

    for (i = 0; i < program->instructions.count; ++i)
    {
        instruction = &program->instructions.elements[i];
        if ((ret = vkd3d_shader_scan_instruction(&context, instruction)) < 0)
            break;
    }

    for (i = 0; i < ARRAY_SIZE(program->flat_constant_count); ++i)
    {
        struct vkd3d_shader_register_range range = {.space = 0, .first = i, .last = i};
        struct vkd3d_shader_register reg = {.idx[0].offset = i, .idx_count = 1};
        unsigned int size = program->flat_constant_count[i];
        struct vkd3d_shader_descriptor_info1 *d;

        if (size)
        {
            if ((d = vkd3d_shader_scan_add_descriptor(&context, VKD3D_SHADER_DESCRIPTOR_TYPE_CBV, &reg,
                    &range, VKD3D_SHADER_RESOURCE_BUFFER, VKD3D_SHADER_RESOURCE_DATA_UINT)))
                d->buffer_size = size * 16;
        }
    }

    if (!ret && signature_info)
    {
        if (!vkd3d_shader_signature_from_shader_signature(&signature_info->input, &program->input_signature)
                || !vkd3d_shader_signature_from_shader_signature(&signature_info->output,
                        &program->output_signature)
                || !vkd3d_shader_signature_from_shader_signature(&signature_info->patch_constant,
                        &program->patch_constant_signature))
        {
            ret = VKD3D_ERROR_OUT_OF_MEMORY;
        }
    }

    if (!ret && descriptor_info)
        ret = convert_descriptor_info(descriptor_info, descriptor_info1);

    if (ret < 0)
    {
        if (combined_sampler_info)
            vkd3d_shader_free_scan_combined_resource_sampler_info(combined_sampler_info);
        if (descriptor_info)
            vkd3d_shader_free_scan_descriptor_info(descriptor_info);
        if (descriptor_info1)
            vkd3d_shader_free_scan_descriptor_info1(descriptor_info1);
        if (signature_info)
            vkd3d_shader_free_scan_signature_info(signature_info);
    }
    else
    {
        vkd3d_shader_free_scan_descriptor_info1(&local_descriptor_info1);
    }
    vkd3d_shader_scan_context_cleanup(&context);
    return ret;
}

int vkd3d_shader_scan(const struct vkd3d_shader_compile_info *compile_info, char **messages)
{
    struct vkd3d_shader_message_context message_context;
    struct shader_dump_data dump_data;
    int ret;

    TRACE("compile_info %p, messages %p.\n", compile_info, messages);

    if (messages)
        *messages = NULL;

    if ((ret = vkd3d_shader_validate_compile_info(compile_info, false)) < 0)
        return ret;

    init_scan_signature_info(compile_info);

    vkd3d_shader_message_context_init(&message_context, compile_info->log_level);

    fill_shader_dump_data(compile_info, &dump_data);
    vkd3d_shader_dump_shader(&dump_data, compile_info->source.code, compile_info->source.size, true);

    if (compile_info->source_type == VKD3D_SHADER_SOURCE_HLSL)
    {
        FIXME("HLSL support not implemented.\n");
        ret = VKD3D_ERROR_NOT_IMPLEMENTED;
    }
    else
    {
        uint64_t config_flags = vkd3d_shader_init_config_flags();
        struct vsir_program program;

        if (!(ret = vsir_parse(compile_info, config_flags, &message_context, &program)))
        {
            ret = vsir_program_scan(&program, compile_info, &message_context, NULL);
            vsir_program_cleanup(&program);
        }
    }

    vkd3d_shader_message_context_trace_messages(&message_context);
    if (!vkd3d_shader_message_context_copy_messages(&message_context, messages))
        ret = VKD3D_ERROR_OUT_OF_MEMORY;
    vkd3d_shader_message_context_cleanup(&message_context);
    return ret;
}

int vsir_program_compile(struct vsir_program *program, uint64_t config_flags,
        const struct vkd3d_shader_compile_info *compile_info, struct vkd3d_shader_code *out,
        struct vkd3d_shader_message_context *message_context)
{
    struct vkd3d_shader_scan_combined_resource_sampler_info combined_sampler_info;
    struct vkd3d_shader_scan_descriptor_info1 scan_descriptor_info;
    struct vkd3d_shader_compile_info scan_info;
    int ret;

    scan_info = *compile_info;

    switch (compile_info->target_type)
    {
        case VKD3D_SHADER_TARGET_D3D_ASM:
            ret = d3d_asm_compile(program, compile_info, out, VSIR_ASM_FLAG_NONE);
            break;

        case VKD3D_SHADER_TARGET_GLSL:
            combined_sampler_info.type = VKD3D_SHADER_STRUCTURE_TYPE_SCAN_COMBINED_RESOURCE_SAMPLER_INFO;
            combined_sampler_info.next = scan_info.next;
            scan_info.next = &combined_sampler_info;
            if ((ret = vsir_program_scan(program, &scan_info, message_context, &scan_descriptor_info)) < 0)
                return ret;
            ret = glsl_compile(program, config_flags, &scan_descriptor_info,
                    &combined_sampler_info, compile_info, out, message_context);
            vkd3d_shader_free_scan_combined_resource_sampler_info(&combined_sampler_info);
            vkd3d_shader_free_scan_descriptor_info1(&scan_descriptor_info);
            break;

        case VKD3D_SHADER_TARGET_SPIRV_BINARY:
        case VKD3D_SHADER_TARGET_SPIRV_TEXT:
            if ((ret = vsir_program_scan(program, &scan_info, message_context, &scan_descriptor_info)) < 0)
                return ret;
            ret = spirv_compile(program, config_flags, &scan_descriptor_info,
                    compile_info, out, message_context);
            vkd3d_shader_free_scan_descriptor_info1(&scan_descriptor_info);
            break;

        case VKD3D_SHADER_TARGET_MSL:
            if ((ret = vsir_program_scan(program, &scan_info, message_context, &scan_descriptor_info)) < 0)
                return ret;
            ret = msl_compile(program, config_flags, &scan_descriptor_info, compile_info, out, message_context);
            vkd3d_shader_free_scan_descriptor_info1(&scan_descriptor_info);
            break;

        default:
            /* Validation should prevent us from reaching this. */
            vkd3d_unreachable();
    }

    return ret;
}

static int compile_hlsl(const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_code *out, struct vkd3d_shader_message_context *message_context)
{
    struct vkd3d_shader_code preprocessed;
    int ret;

    if ((ret = preproc_lexer_parse(compile_info, &preprocessed, message_context)))
        return ret;

    ret = hlsl_compile_shader(&preprocessed, compile_info, out, message_context);

    vkd3d_shader_free_shader_code(&preprocessed);
    return ret;
}

int vkd3d_shader_compile(const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_code *out, char **messages)
{
    struct vkd3d_shader_message_context message_context;
    struct shader_dump_data dump_data;
    int ret;

    TRACE("compile_info %p, out %p, messages %p.\n", compile_info, out, messages);

    if (messages)
        *messages = NULL;

    if ((ret = vkd3d_shader_validate_compile_info(compile_info, true)) < 0)
        return ret;

    init_scan_signature_info(compile_info);

    vkd3d_shader_message_context_init(&message_context, compile_info->log_level);

    fill_shader_dump_data(compile_info, &dump_data);
    vkd3d_shader_dump_shader(&dump_data, compile_info->source.code, compile_info->source.size, true);

    if (compile_info->source_type == VKD3D_SHADER_SOURCE_HLSL)
    {
        ret = compile_hlsl(compile_info, out, &message_context);
    }
    else if (compile_info->source_type == VKD3D_SHADER_SOURCE_FX)
    {
        ret = fx_parse(compile_info, out, &message_context);
    }
    else
    {
        uint64_t config_flags = vkd3d_shader_init_config_flags();
        struct vsir_program program;

        if (!(ret = vsir_parse(compile_info, config_flags, &message_context, &program)))
        {
            ret = vsir_program_compile(&program, config_flags, compile_info, out, &message_context);
            vsir_program_cleanup(&program);
        }
    }

    if (ret >= 0)
        vkd3d_shader_dump_shader(&dump_data, out->code, out->size, false);

    vkd3d_shader_message_context_trace_messages(&message_context);
    if (!vkd3d_shader_message_context_copy_messages(&message_context, messages))
        ret = VKD3D_ERROR_OUT_OF_MEMORY;
    vkd3d_shader_message_context_cleanup(&message_context);
    return ret;
}

void vkd3d_shader_free_scan_combined_resource_sampler_info(
        struct vkd3d_shader_scan_combined_resource_sampler_info *info)
{
    TRACE("info %p.\n", info);

    vkd3d_free(info->combined_samplers);
}

void vkd3d_shader_free_scan_descriptor_info(struct vkd3d_shader_scan_descriptor_info *scan_descriptor_info)
{
    TRACE("scan_descriptor_info %p.\n", scan_descriptor_info);

    vkd3d_free(scan_descriptor_info->descriptors);
}

void vkd3d_shader_free_scan_signature_info(struct vkd3d_shader_scan_signature_info *info)
{
    TRACE("info %p.\n", info);

    vkd3d_shader_free_shader_signature(&info->input);
    vkd3d_shader_free_shader_signature(&info->output);
    vkd3d_shader_free_shader_signature(&info->patch_constant);
}

void vkd3d_shader_free_shader_code(struct vkd3d_shader_code *shader_code)
{
    TRACE("shader_code %p.\n", shader_code);

    vkd3d_free((void *)shader_code->code);
}

static void vkd3d_shader_free_root_signature_v_1_0(struct vkd3d_shader_root_signature_desc *root_signature)
{
    unsigned int i;

    for (i = 0; i < root_signature->parameter_count; ++i)
    {
        const struct vkd3d_shader_root_parameter *parameter = &root_signature->parameters[i];

        if (parameter->parameter_type == VKD3D_SHADER_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
            vkd3d_free((void *)parameter->u.descriptor_table.descriptor_ranges);
    }
    vkd3d_free((void *)root_signature->parameters);
    vkd3d_free((void *)root_signature->static_samplers);

    memset(root_signature, 0, sizeof(*root_signature));
}

static void vkd3d_shader_free_root_signature_v_1_1(struct vkd3d_shader_root_signature_desc1 *root_signature)
{
    unsigned int i;

    for (i = 0; i < root_signature->parameter_count; ++i)
    {
        const struct vkd3d_shader_root_parameter1 *parameter = &root_signature->parameters[i];

        if (parameter->parameter_type == VKD3D_SHADER_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
            vkd3d_free((void *)parameter->u.descriptor_table.descriptor_ranges);
    }
    vkd3d_free((void *)root_signature->parameters);
    vkd3d_free((void *)root_signature->static_samplers);

    memset(root_signature, 0, sizeof(*root_signature));
}

void vkd3d_shader_free_root_signature(struct vkd3d_shader_versioned_root_signature_desc *desc)
{
    TRACE("desc %p.\n", desc);

    if (desc->version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0)
    {
        vkd3d_shader_free_root_signature_v_1_0(&desc->u.v_1_0);
    }
    else if (desc->version == VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_1)
    {
        vkd3d_shader_free_root_signature_v_1_1(&desc->u.v_1_1);
    }
    else if (desc->version)
    {
        FIXME("Unknown version %#x.\n", desc->version);
        return;
    }

    desc->version = 0;
}

void shader_signature_cleanup(struct shader_signature *signature)
{
    for (unsigned int i = 0; i < signature->element_count; ++i)
    {
        vkd3d_free((void *)signature->elements[i].semantic_name);
    }
    vkd3d_free(signature->elements);
    signature->elements = NULL;
    signature->elements_capacity = 0;
    signature->element_count = 0;
}

int vkd3d_shader_parse_input_signature(const struct vkd3d_shader_code *dxbc,
        struct vkd3d_shader_signature *signature, char **messages)
{
    struct vkd3d_shader_message_context message_context;
    struct shader_signature shader_signature;
    int ret;

    TRACE("dxbc {%p, %zu}, signature %p, messages %p.\n", dxbc->code, dxbc->size, signature, messages);

    if (messages)
        *messages = NULL;
    vkd3d_shader_message_context_init(&message_context, VKD3D_SHADER_LOG_INFO);

    ret = shader_parse_input_signature(dxbc, &message_context, &shader_signature);
    vkd3d_shader_message_context_trace_messages(&message_context);
    if (!vkd3d_shader_message_context_copy_messages(&message_context, messages))
        ret = VKD3D_ERROR_OUT_OF_MEMORY;

    vkd3d_shader_message_context_cleanup(&message_context);

    if (!vkd3d_shader_signature_from_shader_signature(signature, &shader_signature))
        ret = VKD3D_ERROR_OUT_OF_MEMORY;

    shader_signature_cleanup(&shader_signature);
    return ret;
}

struct vkd3d_shader_signature_element *vkd3d_shader_find_signature_element(
        const struct vkd3d_shader_signature *signature, const char *semantic_name,
        unsigned int semantic_index, unsigned int stream_index)
{
    struct vkd3d_shader_signature_element *e;
    unsigned int i;

    TRACE("signature %p, semantic_name %s, semantic_index %u, stream_index %u.\n",
            signature, debugstr_a(semantic_name), semantic_index, stream_index);

    e = signature->elements;
    for (i = 0; i < signature->element_count; ++i)
    {
        if (!ascii_strcasecmp(e[i].semantic_name, semantic_name)
                && e[i].semantic_index == semantic_index
                && e[i].stream_index == stream_index)
            return &e[i];
    }

    return NULL;
}

void vkd3d_shader_free_shader_signature(struct vkd3d_shader_signature *signature)
{
    TRACE("signature %p.\n", signature);

    for (unsigned int i = 0; i < signature->element_count; ++i)
    {
        vkd3d_free((void *)signature->elements[i].semantic_name);
    }
    vkd3d_free(signature->elements);
    signature->elements = NULL;
}

const char *vkd3d_shader_get_version(unsigned int *major, unsigned int *minor)
{
    int x, y;

    TRACE("major %p, minor %p.\n", major, minor);

    if (major || minor)
    {
        vkd3d_parse_version(PACKAGE_VERSION, &x, &y);
        if (major)
            *major = x;
        if (minor)
            *minor = y;
    }

    return "vkd3d-shader " PACKAGE_VERSION VKD3D_VCS_ID;
}

const enum vkd3d_shader_source_type *vkd3d_shader_get_supported_source_types(unsigned int *count)
{
    static const enum vkd3d_shader_source_type types[] =
    {
        VKD3D_SHADER_SOURCE_DXBC_TPF,
        VKD3D_SHADER_SOURCE_HLSL,
        VKD3D_SHADER_SOURCE_D3D_BYTECODE,
#ifdef VKD3D_SHADER_UNSUPPORTED_DXIL
        VKD3D_SHADER_SOURCE_DXBC_DXIL,
#endif
        VKD3D_SHADER_SOURCE_FX,
    };

    TRACE("count %p.\n", count);

    *count = ARRAY_SIZE(types);
    return types;
}

const enum vkd3d_shader_target_type *vkd3d_shader_get_supported_target_types(
        enum vkd3d_shader_source_type source_type, unsigned int *count)
{
    static const enum vkd3d_shader_target_type dxbc_tpf_types[] =
    {
        VKD3D_SHADER_TARGET_SPIRV_BINARY,
#ifdef HAVE_SPIRV_TOOLS
        VKD3D_SHADER_TARGET_SPIRV_TEXT,
#endif
        VKD3D_SHADER_TARGET_D3D_ASM,
#ifdef VKD3D_SHADER_UNSUPPORTED_GLSL
        VKD3D_SHADER_TARGET_GLSL,
#endif
#ifdef VKD3D_SHADER_UNSUPPORTED_MSL
        VKD3D_SHADER_TARGET_MSL,
#endif
    };

    static const enum vkd3d_shader_target_type hlsl_types[] =
    {
        VKD3D_SHADER_TARGET_SPIRV_BINARY,
#ifdef HAVE_SPIRV_TOOLS
        VKD3D_SHADER_TARGET_SPIRV_TEXT,
#endif
        VKD3D_SHADER_TARGET_D3D_ASM,
        VKD3D_SHADER_TARGET_D3D_BYTECODE,
        VKD3D_SHADER_TARGET_DXBC_TPF,
        VKD3D_SHADER_TARGET_FX,
    };

    static const enum vkd3d_shader_target_type d3dbc_types[] =
    {
        VKD3D_SHADER_TARGET_SPIRV_BINARY,
#ifdef HAVE_SPIRV_TOOLS
        VKD3D_SHADER_TARGET_SPIRV_TEXT,
#endif
        VKD3D_SHADER_TARGET_D3D_ASM,
    };

#ifdef VKD3D_SHADER_UNSUPPORTED_DXIL
    static const enum vkd3d_shader_target_type dxbc_dxil_types[] =
    {
        VKD3D_SHADER_TARGET_SPIRV_BINARY,
# ifdef HAVE_SPIRV_TOOLS
        VKD3D_SHADER_TARGET_SPIRV_TEXT,
# endif
        VKD3D_SHADER_TARGET_D3D_ASM,
    };
#endif

    static const enum vkd3d_shader_target_type fx_types[] =
    {
        VKD3D_SHADER_TARGET_D3D_ASM,
    };

    TRACE("source_type %#x, count %p.\n", source_type, count);

    switch (source_type)
    {
        case VKD3D_SHADER_SOURCE_DXBC_TPF:
            *count = ARRAY_SIZE(dxbc_tpf_types);
            return dxbc_tpf_types;

        case VKD3D_SHADER_SOURCE_HLSL:
            *count = ARRAY_SIZE(hlsl_types);
            return hlsl_types;

        case VKD3D_SHADER_SOURCE_D3D_BYTECODE:
            *count = ARRAY_SIZE(d3dbc_types);
            return d3dbc_types;

#ifdef VKD3D_SHADER_UNSUPPORTED_DXIL
        case VKD3D_SHADER_SOURCE_DXBC_DXIL:
            *count = ARRAY_SIZE(dxbc_dxil_types);
            return dxbc_dxil_types;
#endif

        case VKD3D_SHADER_SOURCE_FX:
            *count = ARRAY_SIZE(fx_types);
            return fx_types;

        default:
            *count = 0;
            return NULL;
    }
}

int vkd3d_shader_preprocess(const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_code *out, char **messages)
{
    struct vkd3d_shader_message_context message_context;
    int ret;

    TRACE("compile_info %p, out %p, messages %p.\n", compile_info, out, messages);

    if (messages)
        *messages = NULL;

    if ((ret = vkd3d_shader_validate_compile_info(compile_info, false)) < 0)
        return ret;

    vkd3d_shader_message_context_init(&message_context, compile_info->log_level);

    ret = preproc_lexer_parse(compile_info, out, &message_context);

    vkd3d_shader_message_context_trace_messages(&message_context);
    if (!vkd3d_shader_message_context_copy_messages(&message_context, messages))
        ret = VKD3D_ERROR_OUT_OF_MEMORY;
    vkd3d_shader_message_context_cleanup(&message_context);
    return ret;
}

void vkd3d_shader_set_log_callback(PFN_vkd3d_log callback)
{
    vkd3d_dbg_set_log_callback(callback);
}

static struct vkd3d_shader_param_node *shader_param_allocator_node_create(
        struct vkd3d_shader_param_allocator *allocator)
{
    struct vkd3d_shader_param_node *node;

    if (!(node = vkd3d_malloc(offsetof(struct vkd3d_shader_param_node, param[allocator->count * allocator->stride]))))
        return NULL;
    node->next = NULL;
    return node;
}

static void shader_param_allocator_init(struct vkd3d_shader_param_allocator *allocator,
        unsigned int count, unsigned int stride)
{
    allocator->count = max(count, MAX_REG_OUTPUT);
    allocator->stride = stride;
    allocator->head = NULL;
    allocator->current = NULL;
    allocator->index = allocator->count;
}

static void shader_param_allocator_destroy(struct vkd3d_shader_param_allocator *allocator)
{
    struct vkd3d_shader_param_node *current = allocator->head;

    while (current)
    {
        struct vkd3d_shader_param_node *next = current->next;
        vkd3d_free(current);
        current = next;
    }
}

void *shader_param_allocator_get(struct vkd3d_shader_param_allocator *allocator, unsigned int count)
{
    void *params;

    if (!allocator->current || count > allocator->count - allocator->index)
    {
        struct vkd3d_shader_param_node *next;

        /* Monolithic switch has no definite parameter count limit. */
        allocator->count = max(allocator->count, count);

        if (!(next = shader_param_allocator_node_create(allocator)))
            return NULL;
        if (allocator->current)
            allocator->current->next = next;
        else
            allocator->head = next;
        allocator->current = next;
        allocator->index = 0;
    }

    params = &allocator->current->param[allocator->index * allocator->stride];
    allocator->index += count;
    return params;
}

bool shader_instruction_array_init(struct vkd3d_shader_instruction_array *instructions, unsigned int reserve)
{
    memset(instructions, 0, sizeof(*instructions));
    /* Size the parameter initial allocations so they are large enough for most shaders. The
     * code path for chained allocations will be tested if a few shaders need to use it. */
    shader_param_allocator_init(&instructions->dst_params, reserve - reserve / 8u,
            sizeof(*instructions->elements->dst));
    shader_param_allocator_init(&instructions->src_params, reserve * 2u, sizeof(*instructions->elements->src));
    return shader_instruction_array_reserve(instructions, reserve);
}

bool shader_instruction_array_reserve(struct vkd3d_shader_instruction_array *instructions, unsigned int reserve)
{
    if (!vkd3d_array_reserve((void **)&instructions->elements, &instructions->capacity, reserve,
            sizeof(*instructions->elements)))
    {
        ERR("Failed to allocate instructions.\n");
        return false;
    }
    return true;
}

bool shader_instruction_array_insert_at(struct vkd3d_shader_instruction_array *instructions,
        unsigned int idx, unsigned int count)
{
    VKD3D_ASSERT(idx <= instructions->count);

    if (!shader_instruction_array_reserve(instructions, instructions->count + count))
        return false;

    memmove(&instructions->elements[idx + count], &instructions->elements[idx],
            (instructions->count - idx) * sizeof(*instructions->elements));
    memset(&instructions->elements[idx], 0, count * sizeof(*instructions->elements));

    instructions->count += count;

    return true;
}

bool shader_instruction_array_add_icb(struct vkd3d_shader_instruction_array *instructions,
        struct vkd3d_shader_immediate_constant_buffer *icb)
{
    if (!vkd3d_array_reserve((void **)&instructions->icbs, &instructions->icb_capacity, instructions->icb_count + 1,
            sizeof(*instructions->icbs)))
        return false;
    instructions->icbs[instructions->icb_count++] = icb;
    return true;
}

static struct vkd3d_shader_src_param *shader_instruction_array_clone_src_params(
        struct vkd3d_shader_instruction_array *instructions, const struct vkd3d_shader_src_param *params,
        unsigned int count);

static bool shader_register_clone_relative_addresses(struct vkd3d_shader_register *reg,
        struct vkd3d_shader_instruction_array *instructions)
{
    unsigned int i;

    for (i = 0; i < reg->idx_count; ++i)
    {
        if (!reg->idx[i].rel_addr)
            continue;

        if (!(reg->idx[i].rel_addr = shader_instruction_array_clone_src_params(instructions, reg->idx[i].rel_addr, 1)))
            return false;
    }

    return true;
}

static struct vkd3d_shader_dst_param *shader_instruction_array_clone_dst_params(
        struct vkd3d_shader_instruction_array *instructions, const struct vkd3d_shader_dst_param *params,
        unsigned int count)
{
    struct vkd3d_shader_dst_param *dst_params;
    unsigned int i;

    if (!(dst_params = shader_dst_param_allocator_get(&instructions->dst_params, count)))
        return NULL;

    memcpy(dst_params, params, count * sizeof(*params));
    for (i = 0; i < count; ++i)
    {
        if (!shader_register_clone_relative_addresses(&dst_params[i].reg, instructions))
            return NULL;
    }

    return dst_params;
}

static struct vkd3d_shader_src_param *shader_instruction_array_clone_src_params(
        struct vkd3d_shader_instruction_array *instructions, const struct vkd3d_shader_src_param *params,
        unsigned int count)
{
    struct vkd3d_shader_src_param *src_params;
    unsigned int i;

    if (!(src_params = shader_src_param_allocator_get(&instructions->src_params, count)))
        return NULL;

    memcpy(src_params, params, count * sizeof(*params));
    for (i = 0; i < count; ++i)
    {
        if (!shader_register_clone_relative_addresses(&src_params[i].reg, instructions))
            return NULL;
    }

    return src_params;
}

/* NOTE: Immediate constant buffers are not cloned, so the source must not be destroyed while the
 * destination is in use. This seems like a reasonable requirement given how this is currently used. */
bool shader_instruction_array_clone_instruction(struct vkd3d_shader_instruction_array *instructions,
        unsigned int dst, unsigned int src)
{
    struct vkd3d_shader_instruction *ins = &instructions->elements[dst];

    *ins = instructions->elements[src];

    if (ins->dst_count && ins->dst && !(ins->dst = shader_instruction_array_clone_dst_params(instructions,
            ins->dst, ins->dst_count)))
        return false;

    return !ins->src_count || !!(ins->src = shader_instruction_array_clone_src_params(instructions,
            ins->src, ins->src_count));
}

void shader_instruction_array_destroy(struct vkd3d_shader_instruction_array *instructions)
{
    unsigned int i;

    vkd3d_free(instructions->elements);
    shader_param_allocator_destroy(&instructions->dst_params);
    shader_param_allocator_destroy(&instructions->src_params);
    for (i = 0; i < instructions->icb_count; ++i)
        vkd3d_free(instructions->icbs[i]);
    vkd3d_free(instructions->icbs);
}

void vkd3d_shader_build_varying_map(const struct vkd3d_shader_signature *output_signature,
        const struct vkd3d_shader_signature *input_signature,
        unsigned int *ret_count, struct vkd3d_shader_varying_map *varyings)
{
    unsigned int count = 0;
    unsigned int i;

    TRACE("output_signature %p, input_signature %p, ret_count %p, varyings %p.\n",
            output_signature, input_signature, ret_count, varyings);

    for (i = 0; i < input_signature->element_count; ++i)
    {
        const struct vkd3d_shader_signature_element *input_element, *output_element;

        input_element = &input_signature->elements[i];

        if (input_element->sysval_semantic != VKD3D_SHADER_SV_NONE)
            continue;

        varyings[count].input_register_index = input_element->register_index;
        varyings[count].input_mask = input_element->mask;

        if ((output_element = vkd3d_shader_find_signature_element(output_signature,
                input_element->semantic_name, input_element->semantic_index, 0)))
        {
            varyings[count].output_signature_index = output_element - output_signature->elements;
        }
        else
        {
            varyings[count].output_signature_index = output_signature->element_count;
        }

        ++count;
    }

    *ret_count = count;
}

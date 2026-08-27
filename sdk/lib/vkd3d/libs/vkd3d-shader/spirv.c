/*
 * Copyright 2017 Józef Kucia for CodeWeavers
 * Copyright 2021 Conor McCarthy for CodeWeavers
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
#include "wine/rbtree.h"

#include <stdarg.h>
#include <stdio.h>

#ifdef HAVE_SPIRV_UNIFIED1_SPIRV_H
# include "spirv/unified1/spirv.h"
#else
# include "vulkan/spirv.h"
#endif  /* HAVE_SPIRV_UNIFIED1_SPIRV_H */
#ifdef HAVE_SPIRV_UNIFIED1_GLSL_STD_450_H
# include "spirv/unified1/GLSL.std.450.h"
#else
# include "vulkan/GLSL.std.450.h"
#endif  /* HAVE_SPIRV_UNIFIED1_GLSL_STD_450_H */

#ifdef HAVE_SPIRV_TOOLS
# include "spirv-tools/libspirv.h"

static spv_target_env spv_target_env_from_vkd3d(enum vkd3d_shader_spirv_environment environment)
{
    switch (environment)
    {
        case VKD3D_SHADER_SPIRV_ENVIRONMENT_OPENGL_4_5:
            return SPV_ENV_OPENGL_4_5;
        case VKD3D_SHADER_SPIRV_ENVIRONMENT_VULKAN_1_0:
            return SPV_ENV_VULKAN_1_0;
        case VKD3D_SHADER_SPIRV_ENVIRONMENT_VULKAN_1_1:
            return SPV_ENV_VULKAN_1_1;
        default:
            ERR("Invalid environment %#x.\n", environment);
            return SPV_ENV_VULKAN_1_0;
    }
}

static uint32_t get_binary_to_text_options(enum vkd3d_shader_compile_option_formatting_flags formatting)
{
    uint32_t out = 0;
    unsigned int i;

    static const struct
    {
        enum vkd3d_shader_compile_option_formatting_flags vkd3d;
        uint32_t spv;
        bool invert;
    }
    valuemap[] =
    {
        {VKD3D_SHADER_COMPILE_OPTION_FORMATTING_COLOUR,     SPV_BINARY_TO_TEXT_OPTION_COLOR           },
        {VKD3D_SHADER_COMPILE_OPTION_FORMATTING_INDENT,     SPV_BINARY_TO_TEXT_OPTION_INDENT          },
        {VKD3D_SHADER_COMPILE_OPTION_FORMATTING_OFFSETS,    SPV_BINARY_TO_TEXT_OPTION_SHOW_BYTE_OFFSET},
        {VKD3D_SHADER_COMPILE_OPTION_FORMATTING_HEADER,     SPV_BINARY_TO_TEXT_OPTION_NO_HEADER,        true},
        {VKD3D_SHADER_COMPILE_OPTION_FORMATTING_RAW_IDS,    SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES,   true},
    };

    for (i = 0; i < ARRAY_SIZE(valuemap); ++i)
    {
        if (valuemap[i].invert == !(formatting & valuemap[i].vkd3d))
            out |= valuemap[i].spv;
    }

    return out;
}

static enum vkd3d_result vkd3d_spirv_binary_to_text(const struct vkd3d_shader_code *spirv,
        enum vkd3d_shader_spirv_environment environment,
        enum vkd3d_shader_compile_option_formatting_flags formatting, struct vkd3d_shader_code *out)
{
    spv_diagnostic diagnostic = NULL;
    spv_text text = NULL;
    spv_context context;
    spv_result_t spvret;
    enum vkd3d_result result = VKD3D_OK;

    context = spvContextCreate(spv_target_env_from_vkd3d(environment));

    if (!(spvret = spvBinaryToText(context, spirv->code, spirv->size / sizeof(uint32_t),
            get_binary_to_text_options(formatting), &text, &diagnostic)))
    {
        const char *p, *q, *end, *pad, *truncate;
        struct vkd3d_string_buffer buffer;
        size_t line_len;

        vkd3d_string_buffer_init(&buffer);

        for (p = text->str, end = p + text->length; p < end; p = q)
        {
            if (!(q = memchr(p, '\n', end - p)))
                q = end;
            else
                ++q;

            /* FIXME: Note that when colour output is enabled, we count colour
             * escape codes towards the line length. It's possible to fix
             * that, but not completely trivial. */
            for (pad = "", line_len = 100; q - p > line_len; line_len = 100 - strlen(pad))
            {
                if (!(truncate = memchr(p + line_len, ' ', q - p - line_len)))
                    break;
                vkd3d_string_buffer_printf(&buffer, "%s%.*s\n", pad, (int)(truncate - p), p);
                p = truncate + 1;
                if (formatting & VKD3D_SHADER_COMPILE_OPTION_FORMATTING_INDENT)
                    pad = "                       ";
                else
                    pad = "        ";
            }
            vkd3d_string_buffer_printf(&buffer, "%s%.*s", pad, (int)(q - p), p);
        }

        vkd3d_shader_code_from_string_buffer(out, &buffer);
    }
    else
    {
        FIXME("Failed to convert SPIR-V binary to text, ret %d.\n", spvret);
        FIXME("Diagnostic message: %s.\n", debugstr_a(diagnostic->error));
        result = VKD3D_ERROR;
    }

    spvTextDestroy(text);
    spvDiagnosticDestroy(diagnostic);
    spvContextDestroy(context);

    return result;
}

static void vkd3d_spirv_dump(const struct vkd3d_shader_code *spirv,
        enum vkd3d_shader_spirv_environment environment)
{
    static const enum vkd3d_shader_compile_option_formatting_flags formatting
            = VKD3D_SHADER_COMPILE_OPTION_FORMATTING_INDENT | VKD3D_SHADER_COMPILE_OPTION_FORMATTING_HEADER;
    struct vkd3d_shader_code text;

    if (!vkd3d_spirv_binary_to_text(spirv, environment, formatting, &text))
    {
        vkd3d_shader_trace_text(text.code, text.size);
        vkd3d_shader_free_shader_code(&text);
    }
}

static bool vkd3d_spirv_validate(struct vkd3d_string_buffer *buffer, const struct vkd3d_shader_code *spirv,
        enum vkd3d_shader_spirv_environment environment)
{
    spv_diagnostic diagnostic = NULL;
    spv_context context;
    spv_result_t ret;

    context = spvContextCreate(spv_target_env_from_vkd3d(environment));

    if ((ret = spvValidateBinary(context, spirv->code, spirv->size / sizeof(uint32_t),
            &diagnostic)))
    {
        vkd3d_string_buffer_printf(buffer, "%s", diagnostic->error);
    }

    spvDiagnosticDestroy(diagnostic);
    spvContextDestroy(context);

    return !ret;
}

#else

static enum vkd3d_result vkd3d_spirv_binary_to_text(const struct vkd3d_shader_code *spirv,
        enum vkd3d_shader_spirv_environment environment,
        enum vkd3d_shader_compile_option_formatting_flags formatting, struct vkd3d_shader_code *out)
{
    return VKD3D_ERROR;
}
static void vkd3d_spirv_dump(const struct vkd3d_shader_code *spirv,
        enum vkd3d_shader_spirv_environment environment) {}
static bool vkd3d_spirv_validate(struct vkd3d_string_buffer *buffer, const struct vkd3d_shader_code *spirv,
        enum vkd3d_shader_spirv_environment environment)
{
    return true;
}

#endif  /* HAVE_SPIRV_TOOLS */

enum vkd3d_shader_input_sysval_semantic vkd3d_siv_from_sysval_indexed(enum vkd3d_shader_sysval_semantic sysval,
        unsigned int index)
{
    switch (sysval)
    {
        case VKD3D_SHADER_SV_COVERAGE:
        case VKD3D_SHADER_SV_DEPTH:
        case VKD3D_SHADER_SV_DEPTH_GREATER_EQUAL:
        case VKD3D_SHADER_SV_DEPTH_LESS_EQUAL:
        case VKD3D_SHADER_SV_NONE:
        case VKD3D_SHADER_SV_STENCIL_REF:
        case VKD3D_SHADER_SV_TARGET:
            return VKD3D_SIV_NONE;
        case VKD3D_SHADER_SV_POSITION:
            return VKD3D_SIV_POSITION;
        case VKD3D_SHADER_SV_CLIP_DISTANCE:
            return VKD3D_SIV_CLIP_DISTANCE;
        case VKD3D_SHADER_SV_CULL_DISTANCE:
            return VKD3D_SIV_CULL_DISTANCE;
        case VKD3D_SHADER_SV_INSTANCE_ID:
            return VKD3D_SIV_INSTANCE_ID;
        case VKD3D_SHADER_SV_IS_FRONT_FACE:
            return VKD3D_SIV_IS_FRONT_FACE;
        case VKD3D_SHADER_SV_PRIMITIVE_ID:
            return VKD3D_SIV_PRIMITIVE_ID;
        case VKD3D_SHADER_SV_RENDER_TARGET_ARRAY_INDEX:
            return VKD3D_SIV_RENDER_TARGET_ARRAY_INDEX;
        case VKD3D_SHADER_SV_SAMPLE_INDEX:
            return VKD3D_SIV_SAMPLE_INDEX;
        case VKD3D_SHADER_SV_TESS_FACTOR_QUADEDGE:
            return VKD3D_SIV_QUAD_U0_TESS_FACTOR + index;
        case VKD3D_SHADER_SV_TESS_FACTOR_QUADINT:
            return VKD3D_SIV_QUAD_U_INNER_TESS_FACTOR + index;
        case VKD3D_SHADER_SV_TESS_FACTOR_TRIEDGE:
            return VKD3D_SIV_TRIANGLE_U_TESS_FACTOR + index;
        case VKD3D_SHADER_SV_TESS_FACTOR_TRIINT:
            return VKD3D_SIV_TRIANGLE_INNER_TESS_FACTOR;
        case VKD3D_SHADER_SV_TESS_FACTOR_LINEDET:
            return VKD3D_SIV_LINE_DETAIL_TESS_FACTOR;
        case VKD3D_SHADER_SV_TESS_FACTOR_LINEDEN:
            return VKD3D_SIV_LINE_DENSITY_TESS_FACTOR;
        case VKD3D_SHADER_SV_VERTEX_ID:
            return VKD3D_SIV_VERTEX_ID;
        case VKD3D_SHADER_SV_VIEWPORT_ARRAY_INDEX:
            return VKD3D_SIV_VIEWPORT_ARRAY_INDEX;
        default:
            FIXME("Unhandled sysval %#x, index %u.\n", sysval, index);
            return VKD3D_SIV_NONE;
    }
}

#define VKD3D_SPIRV_VERSION_1_0 0x00010000
#define VKD3D_SPIRV_VERSION_1_3 0x00010300
#define VKD3D_SPIRV_GENERATOR_ID 18
#define VKD3D_SPIRV_GENERATOR_VERSION 14
#define VKD3D_SPIRV_GENERATOR_MAGIC vkd3d_make_u32(VKD3D_SPIRV_GENERATOR_VERSION, VKD3D_SPIRV_GENERATOR_ID)

struct vkd3d_spirv_stream
{
    uint32_t *words;
    size_t capacity;
    size_t word_count;

    struct list inserted_chunks;
};

static void vkd3d_spirv_stream_init(struct vkd3d_spirv_stream *stream)
{
    stream->capacity = 256;
    if (!(stream->words = vkd3d_calloc(stream->capacity, sizeof(*stream->words))))
        stream->capacity = 0;
    stream->word_count = 0;

    list_init(&stream->inserted_chunks);
}

struct vkd3d_spirv_chunk
{
    struct list entry;
    size_t location;
    size_t word_count;
    uint32_t words[];
};

static void vkd3d_spirv_stream_clear(struct vkd3d_spirv_stream *stream)
{
    struct vkd3d_spirv_chunk *c1, *c2;

    stream->word_count = 0;

    LIST_FOR_EACH_ENTRY_SAFE(c1, c2, &stream->inserted_chunks, struct vkd3d_spirv_chunk, entry)
        vkd3d_free(c1);

    list_init(&stream->inserted_chunks);
}

static void vkd3d_spirv_stream_free(struct vkd3d_spirv_stream *stream)
{
    vkd3d_free(stream->words);

    vkd3d_spirv_stream_clear(stream);
}

static void vkd3d_shader_code_from_spirv_stream(struct vkd3d_shader_code *code, struct vkd3d_spirv_stream *stream)
{
    code->code = stream->words;
    code->size = stream->word_count * sizeof(*stream->words);

    stream->words = NULL;
    stream->capacity = 0;
    stream->word_count = 0;
}

static size_t vkd3d_spirv_stream_current_location(struct vkd3d_spirv_stream *stream)
{
    return stream->word_count;
}

static void vkd3d_spirv_stream_insert(struct vkd3d_spirv_stream *stream,
        size_t location, const uint32_t *words, unsigned int word_count)
{
    struct vkd3d_spirv_chunk *chunk, *current;

    if (!(chunk = vkd3d_malloc(offsetof(struct vkd3d_spirv_chunk, words[word_count]))))
        return;

    chunk->location = location;
    chunk->word_count = word_count;
    memcpy(chunk->words, words, word_count * sizeof(*words));

    LIST_FOR_EACH_ENTRY(current, &stream->inserted_chunks, struct vkd3d_spirv_chunk, entry)
    {
        if (current->location > location)
        {
            list_add_before(&current->entry, &chunk->entry);
            return;
        }
    }

    list_add_tail(&stream->inserted_chunks, &chunk->entry);
}

static bool vkd3d_spirv_stream_append(struct vkd3d_spirv_stream *dst_stream,
        const struct vkd3d_spirv_stream *src_stream)
{
    size_t word_count, src_word_count = src_stream->word_count;
    struct vkd3d_spirv_chunk *chunk;
    size_t src_location = 0;

    VKD3D_ASSERT(list_empty(&dst_stream->inserted_chunks));

    LIST_FOR_EACH_ENTRY(chunk, &src_stream->inserted_chunks, struct vkd3d_spirv_chunk, entry)
        src_word_count += chunk->word_count;

    if (!vkd3d_array_reserve((void **)&dst_stream->words, &dst_stream->capacity,
            dst_stream->word_count + src_word_count, sizeof(*dst_stream->words)))
        return false;

    VKD3D_ASSERT(dst_stream->word_count + src_word_count <= dst_stream->capacity);
    LIST_FOR_EACH_ENTRY(chunk, &src_stream->inserted_chunks, struct vkd3d_spirv_chunk, entry)
    {
        VKD3D_ASSERT(src_location <= chunk->location);
        word_count = chunk->location - src_location;
        memcpy(&dst_stream->words[dst_stream->word_count], &src_stream->words[src_location],
                word_count * sizeof(*src_stream->words));
        dst_stream->word_count += word_count;
        src_location += word_count;
        VKD3D_ASSERT(src_location == chunk->location);

        memcpy(&dst_stream->words[dst_stream->word_count], chunk->words,
                chunk->word_count * sizeof(*chunk->words));
        dst_stream->word_count += chunk->word_count;
    }

    word_count = src_stream->word_count - src_location;
    memcpy(&dst_stream->words[dst_stream->word_count], &src_stream->words[src_location],
            word_count * sizeof(*src_stream->words));
    dst_stream->word_count += word_count;
    return true;
}

struct vkd3d_spirv_builder
{
    uint64_t capability_mask;
    SpvCapability *capabilities;
    size_t capabilities_size;
    size_t capabilities_count;
    uint32_t ext_instr_set_glsl_450;
    uint32_t invocation_count;
    SpvExecutionModel execution_model;

    uint32_t current_id;
    uint32_t main_function_id;
    struct rb_tree declarations;
    uint32_t type_sampler_id;
    uint32_t type_bool_id;
    uint32_t type_void_id;
    uint32_t scope_subgroup_id;
    uint32_t numeric_type_ids[VKD3D_SHADER_COMPONENT_TYPE_COUNT][VKD3D_VEC4_SIZE];

    struct vkd3d_spirv_stream debug_stream; /* debug instructions */
    struct vkd3d_spirv_stream annotation_stream; /* decoration instructions */
    struct vkd3d_spirv_stream global_stream; /* types, constants, global variables */
    struct vkd3d_spirv_stream function_stream; /* function definitions */

    struct vkd3d_spirv_stream execution_mode_stream; /* execution mode instructions */

    struct vkd3d_spirv_stream original_function_stream;
    struct vkd3d_spirv_stream insertion_stream;
    size_t insertion_location;

    size_t main_function_location;

    /* entry point interface */
    uint32_t *iface;
    size_t iface_capacity;
    size_t iface_element_count;
};

static uint32_t vkd3d_spirv_alloc_id(struct vkd3d_spirv_builder *builder)
{
    return builder->current_id++;
}

static bool vkd3d_spirv_capability_is_enabled(struct vkd3d_spirv_builder *builder,
        SpvCapability cap)
{
    size_t i;

    if (cap < sizeof(builder->capability_mask) * CHAR_BIT)
        return (builder->capability_mask >> cap) & 1;

    for (i = 0; i < builder->capabilities_count; ++i)
        if (builder->capabilities[i] == cap)
            return true;

    return false;
}

static void vkd3d_spirv_enable_capability(struct vkd3d_spirv_builder *builder,
        SpvCapability cap)
{
    if (cap < sizeof(builder->capability_mask) * CHAR_BIT)
    {
        builder->capability_mask |= 1ull << cap;
        return;
    }

    if (vkd3d_spirv_capability_is_enabled(builder, cap))
        return;

    vkd3d_array_reserve((void **)&builder->capabilities, &builder->capabilities_size,
            builder->capabilities_count + 1, sizeof(*builder->capabilities));

    builder->capabilities[builder->capabilities_count++] = cap;
}

static uint32_t vkd3d_spirv_get_glsl_std450_instr_set(struct vkd3d_spirv_builder *builder)
{
    if (!builder->ext_instr_set_glsl_450)
        builder->ext_instr_set_glsl_450 = vkd3d_spirv_alloc_id(builder);

    return builder->ext_instr_set_glsl_450;
}

static void vkd3d_spirv_add_iface_variable(struct vkd3d_spirv_builder *builder,
        uint32_t id)
{
    if (!vkd3d_array_reserve((void **)&builder->iface, &builder->iface_capacity,
            builder->iface_element_count + 1, sizeof(*builder->iface)))
        return;

    builder->iface[builder->iface_element_count++] = id;
}

static void vkd3d_spirv_set_execution_model(struct vkd3d_spirv_builder *builder,
        SpvExecutionModel model)
{
    builder->execution_model = model;

    switch (model)
    {
        case SpvExecutionModelVertex:
        case SpvExecutionModelFragment:
        case SpvExecutionModelGLCompute:
            vkd3d_spirv_enable_capability(builder, SpvCapabilityShader);
            break;
        case SpvExecutionModelTessellationControl:
        case SpvExecutionModelTessellationEvaluation:
            vkd3d_spirv_enable_capability(builder, SpvCapabilityTessellation);
            break;
        case SpvExecutionModelGeometry:
            vkd3d_spirv_enable_capability(builder, SpvCapabilityGeometry);
            break;
        default:
            ERR("Unhandled execution model %#x.\n", model);
    }
}

static uint32_t vkd3d_spirv_opcode_word(SpvOp op, unsigned int word_count)
{
    VKD3D_ASSERT(!(op & ~SpvOpCodeMask));
    return (word_count << SpvWordCountShift) | op;
}

static void vkd3d_spirv_build_word(struct vkd3d_spirv_stream *stream, uint32_t word)
{
    if (!vkd3d_array_reserve((void **)&stream->words, &stream->capacity,
            stream->word_count + 1, sizeof(*stream->words)))
        return;

    stream->words[stream->word_count++] = word;
}

static unsigned int vkd3d_spirv_string_word_count(const char *str)
{
    return align(strlen(str) + 1, sizeof(uint32_t)) / sizeof(uint32_t);
}

static void vkd3d_spirv_build_string(struct vkd3d_spirv_stream *stream,
        const char *str, unsigned int word_count)
{
    unsigned int word_idx, i;
    const char *ptr = str;

    for (word_idx = 0; word_idx < word_count; ++word_idx)
    {
        uint32_t word = 0;
        for (i = 0; i < sizeof(uint32_t) && *ptr; ++i)
            word |= (uint32_t)*ptr++ << (8 * i);
        vkd3d_spirv_build_word(stream, word);
    }
}

typedef uint32_t (*vkd3d_spirv_build_pfn)(struct vkd3d_spirv_builder *builder);
typedef uint32_t (*vkd3d_spirv_build1_pfn)(struct vkd3d_spirv_builder *builder,
        uint32_t operand0);
typedef uint32_t (*vkd3d_spirv_build1v_pfn)(struct vkd3d_spirv_builder *builder,
        uint32_t operand0, const uint32_t *operands, unsigned int operand_count);
typedef uint32_t (*vkd3d_spirv_build2_pfn)(struct vkd3d_spirv_builder *builder,
        uint32_t operand0, uint32_t operand1);
typedef uint32_t (*vkd3d_spirv_build7_pfn)(struct vkd3d_spirv_builder *builder,
        uint32_t operand0, uint32_t operand1, uint32_t operand2, uint32_t operand3,
        uint32_t operand4, uint32_t operand5, uint32_t operand6);

static uint32_t vkd3d_spirv_build_once(struct vkd3d_spirv_builder *builder,
        uint32_t *id, vkd3d_spirv_build_pfn build_pfn)
{
    if (!(*id))
        *id = build_pfn(builder);
    return *id;
}

#define MAX_SPIRV_DECLARATION_PARAMETER_COUNT 7

struct vkd3d_spirv_declaration
{
    struct rb_entry entry;

    SpvOp op;
    unsigned int parameter_count;
    uint32_t parameters[MAX_SPIRV_DECLARATION_PARAMETER_COUNT];
    uint32_t id;
};

static int vkd3d_spirv_declaration_compare(const void *key, const struct rb_entry *e)
{
    const struct vkd3d_spirv_declaration *a = key;
    const struct vkd3d_spirv_declaration *b = RB_ENTRY_VALUE(e, const struct vkd3d_spirv_declaration, entry);
    int ret;

    if ((ret = vkd3d_u32_compare(a->op, b->op)))
        return ret;
    if ((ret = vkd3d_u32_compare(a->parameter_count, b->parameter_count)))
        return ret;
    VKD3D_ASSERT(a->parameter_count <= ARRAY_SIZE(a->parameters));
    return memcmp(&a->parameters, &b->parameters, a->parameter_count * sizeof(*a->parameters));
}

static void vkd3d_spirv_declaration_free(struct rb_entry *entry, void *context)
{
    struct vkd3d_spirv_declaration *d = RB_ENTRY_VALUE(entry, struct vkd3d_spirv_declaration, entry);

    vkd3d_free(d);
}

static void vkd3d_spirv_insert_declaration(struct vkd3d_spirv_builder *builder,
        const struct vkd3d_spirv_declaration *declaration)
{
    struct vkd3d_spirv_declaration *d;

    VKD3D_ASSERT(declaration->parameter_count <= ARRAY_SIZE(declaration->parameters));

    if (!(d = vkd3d_malloc(sizeof(*d))))
        return;
    memcpy(d, declaration, sizeof(*d));
    if (rb_put(&builder->declarations, d, &d->entry) == -1)
    {
        ERR("Failed to insert declaration entry.\n");
        vkd3d_free(d);
    }
}

static uint32_t vkd3d_spirv_build_once1(struct vkd3d_spirv_builder *builder,
        SpvOp op, uint32_t operand0, vkd3d_spirv_build1_pfn build_pfn)
{
    struct vkd3d_spirv_declaration declaration;
    struct rb_entry *entry;

    declaration.op = op;
    declaration.parameter_count = 1;
    declaration.parameters[0] = operand0;

    if ((entry = rb_get(&builder->declarations, &declaration)))
        return RB_ENTRY_VALUE(entry, struct vkd3d_spirv_declaration, entry)->id;

    declaration.id = build_pfn(builder, operand0);
    vkd3d_spirv_insert_declaration(builder, &declaration);
    return declaration.id;
}

static uint32_t vkd3d_spirv_build_once1v(struct vkd3d_spirv_builder *builder,
        SpvOp op, uint32_t operand0, const uint32_t *operands, unsigned int operand_count,
        vkd3d_spirv_build1v_pfn build_pfn)
{
    struct vkd3d_spirv_declaration declaration;
    unsigned int i, param_idx = 0;
    struct rb_entry *entry;

    if (operand_count >= ARRAY_SIZE(declaration.parameters))
    {
        WARN("Unsupported parameter count %u (opcode %#x).\n", operand_count + 1, op);
        return build_pfn(builder, operand0, operands, operand_count);
    }

    declaration.op = op;
    declaration.parameters[param_idx++] = operand0;
    for (i = 0; i < operand_count; ++i)
        declaration.parameters[param_idx++] = operands[i];
    declaration.parameter_count = param_idx;

    if ((entry = rb_get(&builder->declarations, &declaration)))
        return RB_ENTRY_VALUE(entry, struct vkd3d_spirv_declaration, entry)->id;

    declaration.id = build_pfn(builder, operand0, operands, operand_count);
    vkd3d_spirv_insert_declaration(builder, &declaration);
    return declaration.id;
}

static uint32_t vkd3d_spirv_build_once2(struct vkd3d_spirv_builder *builder,
        SpvOp op, uint32_t operand0, uint32_t operand1, vkd3d_spirv_build2_pfn build_pfn)
{
    struct vkd3d_spirv_declaration declaration;
    struct rb_entry *entry;

    declaration.op = op;
    declaration.parameter_count = 2;
    declaration.parameters[0] = operand0;
    declaration.parameters[1] = operand1;

    if ((entry = rb_get(&builder->declarations, &declaration)))
        return RB_ENTRY_VALUE(entry, struct vkd3d_spirv_declaration, entry)->id;

    declaration.id = build_pfn(builder, operand0, operand1);
    vkd3d_spirv_insert_declaration(builder, &declaration);
    return declaration.id;
}

static uint32_t vkd3d_spirv_build_once7(struct vkd3d_spirv_builder *builder,
        SpvOp op, const uint32_t *operands, vkd3d_spirv_build7_pfn build_pfn)
{
    struct vkd3d_spirv_declaration declaration;
    struct rb_entry *entry;

    declaration.op = op;
    declaration.parameter_count = 7;
    memcpy(&declaration.parameters, operands, declaration.parameter_count * sizeof(*operands));

    if ((entry = rb_get(&builder->declarations, &declaration)))
        return RB_ENTRY_VALUE(entry, struct vkd3d_spirv_declaration, entry)->id;

    declaration.id = build_pfn(builder, operands[0], operands[1], operands[2],
            operands[3], operands[4], operands[5], operands[6]);
    vkd3d_spirv_insert_declaration(builder, &declaration);
    return declaration.id;
}

/*
 * vkd3d_spirv_build_op[1-3][v]()
 * vkd3d_spirv_build_op_[t][r][1-3][v]()
 *
 * t   - result type
 * r   - result id
 * 1-3 - the number of operands
 * v   - variable number of operands
 */
static void vkd3d_spirv_build_op(struct vkd3d_spirv_stream *stream, SpvOp op)
{
    vkd3d_spirv_build_word(stream, vkd3d_spirv_opcode_word(op, 1));
}

static void vkd3d_spirv_build_op1(struct vkd3d_spirv_stream *stream,
        SpvOp op, uint32_t operand)
{
    vkd3d_spirv_build_word(stream, vkd3d_spirv_opcode_word(op, 2));
    vkd3d_spirv_build_word(stream, operand);
}

static void vkd3d_spirv_build_op1v(struct vkd3d_spirv_stream *stream,
        SpvOp op, uint32_t operand0, const uint32_t *operands, unsigned int operand_count)
{
    unsigned int i;
    vkd3d_spirv_build_word(stream, vkd3d_spirv_opcode_word(op, 2 + operand_count));
    vkd3d_spirv_build_word(stream, operand0);
    for (i = 0; i < operand_count; ++i)
        vkd3d_spirv_build_word(stream, operands[i]);
}

static void vkd3d_spirv_build_op2v(struct vkd3d_spirv_stream *stream,
        SpvOp op, uint32_t operand0, uint32_t operand1,
        const uint32_t *operands, unsigned int operand_count)
{
    unsigned int i;
    vkd3d_spirv_build_word(stream, vkd3d_spirv_opcode_word(op, 3 + operand_count));
    vkd3d_spirv_build_word(stream, operand0);
    vkd3d_spirv_build_word(stream, operand1);
    for (i = 0; i < operand_count; ++i)
        vkd3d_spirv_build_word(stream, operands[i]);
}

static void vkd3d_spirv_build_op3v(struct vkd3d_spirv_stream *stream,
        SpvOp op, uint32_t operand0, uint32_t operand1, uint32_t operand2,
        const uint32_t *operands, unsigned int operand_count)
{
    unsigned int i;
    vkd3d_spirv_build_word(stream, vkd3d_spirv_opcode_word(op, 4 + operand_count));
    vkd3d_spirv_build_word(stream, operand0);
    vkd3d_spirv_build_word(stream, operand1);
    vkd3d_spirv_build_word(stream, operand2);
    for (i = 0; i < operand_count; ++i)
        vkd3d_spirv_build_word(stream, operands[i]);
}

static void vkd3d_spirv_build_op2(struct vkd3d_spirv_stream *stream,
        SpvOp op, uint32_t operand0, uint32_t operand1)
{
    return vkd3d_spirv_build_op2v(stream, op, operand0, operand1, NULL, 0);
}

static void vkd3d_spirv_build_op3(struct vkd3d_spirv_stream *stream,
        SpvOp op, uint32_t operand0, uint32_t operand1, uint32_t operand2)
{
    return vkd3d_spirv_build_op2v(stream, op, operand0, operand1, &operand2, 1);
}

static uint32_t vkd3d_spirv_build_op_rv(struct vkd3d_spirv_builder *builder,
        struct vkd3d_spirv_stream *stream, SpvOp op,
        const uint32_t *operands, unsigned int operand_count)
{
    uint32_t result_id = vkd3d_spirv_alloc_id(builder);
    vkd3d_spirv_build_op1v(stream, op, result_id, operands, operand_count);
    return result_id;
}

static uint32_t vkd3d_spirv_build_op_r(struct vkd3d_spirv_builder *builder,
        struct vkd3d_spirv_stream *stream, SpvOp op)
{
    return vkd3d_spirv_build_op_rv(builder, stream, op, NULL, 0);
}

static uint32_t vkd3d_spirv_build_op_r1(struct vkd3d_spirv_builder *builder,
        struct vkd3d_spirv_stream *stream, SpvOp op, uint32_t operand0)
{
    return vkd3d_spirv_build_op_rv(builder, stream, op, &operand0, 1);
}

static uint32_t vkd3d_spirv_build_op_r2(struct vkd3d_spirv_builder *builder,
        struct vkd3d_spirv_stream *stream, SpvOp op, uint32_t operand0, uint32_t operand1)
{
    uint32_t operands[] = {operand0, operand1};
    return vkd3d_spirv_build_op_rv(builder, stream, op, operands, ARRAY_SIZE(operands));
}

static uint32_t vkd3d_spirv_build_op_r1v(struct vkd3d_spirv_builder *builder,
        struct vkd3d_spirv_stream *stream, SpvOp op, uint32_t operand0,
        const uint32_t *operands, unsigned int operand_count)
{
    uint32_t result_id = vkd3d_spirv_alloc_id(builder);
    vkd3d_spirv_build_op2v(stream, op, result_id, operand0, operands, operand_count);
    return result_id;
}

static uint32_t vkd3d_spirv_build_op_trv(struct vkd3d_spirv_builder *builder,
        struct vkd3d_spirv_stream *stream, SpvOp op, uint32_t result_type,
        const uint32_t *operands, unsigned int operand_count)
{
    uint32_t result_id = vkd3d_spirv_alloc_id(builder);
    vkd3d_spirv_build_op2v(stream, op, result_type, result_id, operands, operand_count);
    return result_id;
}

static uint32_t vkd3d_spirv_build_op_tr(struct vkd3d_spirv_builder *builder,
        struct vkd3d_spirv_stream *stream, SpvOp op, uint32_t result_type)
{
    return vkd3d_spirv_build_op_trv(builder, stream, op, result_type, NULL, 0);
}

static uint32_t vkd3d_spirv_build_op_tr1(struct vkd3d_spirv_builder *builder,
        struct vkd3d_spirv_stream *stream, SpvOp op, uint32_t result_type,
        uint32_t operand0)
{
    return vkd3d_spirv_build_op_trv(builder, stream, op, result_type, &operand0, 1);
}

static uint32_t vkd3d_spirv_build_op_tr2(struct vkd3d_spirv_builder *builder,
        struct vkd3d_spirv_stream *stream, SpvOp op, uint32_t result_type,
        uint32_t operand0, uint32_t operand1)
{
    uint32_t operands[] = {operand0, operand1};
    return vkd3d_spirv_build_op_trv(builder, stream, op, result_type,
            operands, ARRAY_SIZE(operands));
}

static uint32_t vkd3d_spirv_build_op_tr3(struct vkd3d_spirv_builder *builder,
        struct vkd3d_spirv_stream *stream, SpvOp op, uint32_t result_type,
        uint32_t operand0, uint32_t operand1, uint32_t operand2)
{
    uint32_t operands[] = {operand0, operand1, operand2};
    return vkd3d_spirv_build_op_trv(builder, stream, op, result_type,
            operands, ARRAY_SIZE(operands));
}

static uint32_t vkd3d_spirv_build_op_tr1v(struct vkd3d_spirv_builder *builder,
        struct vkd3d_spirv_stream *stream, SpvOp op, uint32_t result_type,
        uint32_t operand0, const uint32_t *operands, unsigned int operand_count)
{
    uint32_t result_id = vkd3d_spirv_alloc_id(builder);
    vkd3d_spirv_build_op3v(stream, op, result_type, result_id, operand0, operands, operand_count);
    return result_id;
}

static uint32_t vkd3d_spirv_build_op_tr2v(struct vkd3d_spirv_builder *builder,
        struct vkd3d_spirv_stream *stream, SpvOp op, uint32_t result_type,
        uint32_t operand0, uint32_t operand1, const uint32_t *operands, unsigned int operand_count)
{
    uint32_t result_id = vkd3d_spirv_alloc_id(builder);
    unsigned int i;
    vkd3d_spirv_build_word(stream, vkd3d_spirv_opcode_word(op, 5 + operand_count));
    vkd3d_spirv_build_word(stream, result_type);
    vkd3d_spirv_build_word(stream, result_id);
    vkd3d_spirv_build_word(stream, operand0);
    vkd3d_spirv_build_word(stream, operand1);
    for (i = 0; i < operand_count; ++i)
        vkd3d_spirv_build_word(stream, operands[i]);
    return result_id;
}

static void vkd3d_spirv_begin_function_stream_insertion(struct vkd3d_spirv_builder *builder,
        size_t location)
{
    VKD3D_ASSERT(builder->insertion_location == ~(size_t)0);

    if (vkd3d_spirv_stream_current_location(&builder->function_stream) == location)
        return;

    builder->original_function_stream = builder->function_stream;
    builder->function_stream = builder->insertion_stream;
    builder->insertion_location = location;
}

static void vkd3d_spirv_end_function_stream_insertion(struct vkd3d_spirv_builder *builder)
{
    struct vkd3d_spirv_stream *insertion_stream = &builder->insertion_stream;

    if (builder->insertion_location == ~(size_t)0)
        return;

    builder->insertion_stream = builder->function_stream;
    builder->function_stream = builder->original_function_stream;

    vkd3d_spirv_stream_insert(&builder->function_stream, builder->insertion_location,
            insertion_stream->words, insertion_stream->word_count);
    vkd3d_spirv_stream_clear(insertion_stream);
    builder->insertion_location = ~(size_t)0;
}

static void vkd3d_spirv_build_op_capability(struct vkd3d_spirv_stream *stream,
        SpvCapability cap)
{
    vkd3d_spirv_build_op1(stream, SpvOpCapability, cap);
}

static void vkd3d_spirv_build_op_extension(struct vkd3d_spirv_stream *stream,
        const char *name)
{
    unsigned int name_size = vkd3d_spirv_string_word_count(name);
    vkd3d_spirv_build_word(stream, vkd3d_spirv_opcode_word(SpvOpExtension, 1 + name_size));
    vkd3d_spirv_build_string(stream, name, name_size);
}

static void vkd3d_spirv_build_op_ext_inst_import(struct vkd3d_spirv_stream *stream,
        uint32_t result_id, const char *name)
{
    unsigned int name_size = vkd3d_spirv_string_word_count(name);
    vkd3d_spirv_build_word(stream, vkd3d_spirv_opcode_word(SpvOpExtInstImport, 2 + name_size));
    vkd3d_spirv_build_word(stream, result_id);
    vkd3d_spirv_build_string(stream, name, name_size);
}

static uint32_t vkd3d_spirv_build_op_ext_inst(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t inst_set, uint32_t inst_number,
        uint32_t *operands, unsigned int operand_count)
{
    return vkd3d_spirv_build_op_tr2v(builder, &builder->function_stream,
            SpvOpExtInst, result_type, inst_set, inst_number, operands, operand_count);
}

static void vkd3d_spirv_build_op_memory_model(struct vkd3d_spirv_stream *stream,
        SpvAddressingModel addressing_model, SpvMemoryModel memory_model)
{
    vkd3d_spirv_build_op2(stream, SpvOpMemoryModel, addressing_model, memory_model);
}

static void vkd3d_spirv_build_op_entry_point(struct vkd3d_spirv_stream *stream,
        SpvExecutionModel model, uint32_t function_id, const char *name,
        uint32_t *interface_list, unsigned int interface_size)
{
    unsigned int i, name_size = vkd3d_spirv_string_word_count(name);
    vkd3d_spirv_build_word(stream, vkd3d_spirv_opcode_word(SpvOpEntryPoint, 3 + name_size + interface_size));
    vkd3d_spirv_build_word(stream, model);
    vkd3d_spirv_build_word(stream, function_id);
    vkd3d_spirv_build_string(stream, name, name_size);
    for (i = 0; i < interface_size; ++i)
        vkd3d_spirv_build_word(stream, interface_list[i]);
}

static void vkd3d_spirv_build_op_execution_mode(struct vkd3d_spirv_stream *stream,
        uint32_t entry_point, SpvExecutionMode mode, const uint32_t *literals, unsigned int literal_count)
{
    vkd3d_spirv_build_op2v(stream, SpvOpExecutionMode, entry_point, mode, literals, literal_count);
}

static void vkd3d_spirv_build_op_name(struct vkd3d_spirv_builder *builder,
        uint32_t id, const char *fmt, ...)
{
    struct vkd3d_spirv_stream *stream = &builder->debug_stream;
    unsigned int name_size;
    char name[1024];
    va_list args;

    va_start(args, fmt);
    vsnprintf(name, ARRAY_SIZE(name), fmt, args);
    name[ARRAY_SIZE(name) - 1] = '\0';
    va_end(args);

    name_size = vkd3d_spirv_string_word_count(name);
    vkd3d_spirv_build_word(stream, vkd3d_spirv_opcode_word(SpvOpName, 2 + name_size));
    vkd3d_spirv_build_word(stream, id);
    vkd3d_spirv_build_string(stream, name, name_size);
}

static void vkd3d_spirv_build_op_member_name(struct vkd3d_spirv_builder *builder,
        uint32_t type_id, uint32_t member, const char *fmt, ...)
{
    struct vkd3d_spirv_stream *stream = &builder->debug_stream;
    unsigned int name_size;
    char name[1024];
    va_list args;

    va_start(args, fmt);
    vsnprintf(name, ARRAY_SIZE(name), fmt, args);
    name[ARRAY_SIZE(name) - 1] = '\0';
    va_end(args);

    name_size = vkd3d_spirv_string_word_count(name);
    vkd3d_spirv_build_word(stream, vkd3d_spirv_opcode_word(SpvOpMemberName, 3 + name_size));
    vkd3d_spirv_build_word(stream, type_id);
    vkd3d_spirv_build_word(stream, member);
    vkd3d_spirv_build_string(stream, name, name_size);
}

static void vkd3d_spirv_build_op_decorate(struct vkd3d_spirv_builder *builder,
        uint32_t target_id, SpvDecoration decoration,
        uint32_t *literals, uint32_t literal_count)
{
    vkd3d_spirv_build_op2v(&builder->annotation_stream,
            SpvOpDecorate, target_id, decoration, literals, literal_count);
}

static void vkd3d_spirv_build_op_decorate1(struct vkd3d_spirv_builder *builder,
        uint32_t target_id, SpvDecoration decoration, uint32_t operand0)
{
    return vkd3d_spirv_build_op_decorate(builder, target_id, decoration, &operand0, 1);
}

static void vkd3d_spirv_build_op_member_decorate(struct vkd3d_spirv_builder *builder,
        uint32_t structure_type_id, uint32_t member_idx, SpvDecoration decoration,
        uint32_t *literals, uint32_t literal_count)
{
    vkd3d_spirv_build_op3v(&builder->annotation_stream, SpvOpMemberDecorate,
            structure_type_id, member_idx, decoration, literals, literal_count);
}

static void vkd3d_spirv_build_op_member_decorate1(struct vkd3d_spirv_builder *builder,
        uint32_t structure_type_id, uint32_t member_idx, SpvDecoration decoration, uint32_t operand0)
{
    vkd3d_spirv_build_op_member_decorate(builder, structure_type_id, member_idx, decoration, &operand0, 1);
}

static uint32_t vkd3d_spirv_build_op_type_void(struct vkd3d_spirv_builder *builder)
{
    return vkd3d_spirv_build_op_r(builder, &builder->global_stream, SpvOpTypeVoid);
}

static uint32_t vkd3d_spirv_get_op_type_void(struct vkd3d_spirv_builder *builder)
{
    return vkd3d_spirv_build_once(builder, &builder->type_void_id, vkd3d_spirv_build_op_type_void);
}

static uint32_t vkd3d_spirv_build_op_type_bool(struct vkd3d_spirv_builder *builder)
{
    return vkd3d_spirv_build_op_r(builder, &builder->global_stream, SpvOpTypeBool);
}

static uint32_t vkd3d_spirv_get_op_type_bool(struct vkd3d_spirv_builder *builder)
{
    return vkd3d_spirv_build_once(builder, &builder->type_bool_id, vkd3d_spirv_build_op_type_bool);
}

static uint32_t vkd3d_spirv_build_op_type_float(struct vkd3d_spirv_builder *builder,
        uint32_t width)
{
    return vkd3d_spirv_build_op_r1(builder, &builder->global_stream, SpvOpTypeFloat, width);
}

static uint32_t vkd3d_spirv_get_op_type_float(struct vkd3d_spirv_builder *builder,
        uint32_t width)
{
    return vkd3d_spirv_build_once1(builder, SpvOpTypeFloat, width, vkd3d_spirv_build_op_type_float);
}

static uint32_t vkd3d_spirv_build_op_type_int(struct vkd3d_spirv_builder *builder,
        uint32_t width, uint32_t signedness)
{
    return vkd3d_spirv_build_op_r2(builder, &builder->global_stream, SpvOpTypeInt, width, signedness);
}

static uint32_t vkd3d_spirv_get_op_type_int(struct vkd3d_spirv_builder *builder,
        uint32_t width, uint32_t signedness)
{
    return vkd3d_spirv_build_once2(builder, SpvOpTypeInt, width, signedness,
            vkd3d_spirv_build_op_type_int);
}

static uint32_t vkd3d_spirv_build_op_type_vector(struct vkd3d_spirv_builder *builder,
        uint32_t component_type, uint32_t component_count)
{
    return vkd3d_spirv_build_op_r2(builder, &builder->global_stream,
            SpvOpTypeVector, component_type, component_count);
}

static uint32_t vkd3d_spirv_get_op_type_vector(struct vkd3d_spirv_builder *builder,
        uint32_t component_type, uint32_t component_count)
{
    return vkd3d_spirv_build_once2(builder, SpvOpTypeVector, component_type, component_count,
            vkd3d_spirv_build_op_type_vector);
}

static uint32_t vkd3d_spirv_build_op_type_array(struct vkd3d_spirv_builder *builder,
        uint32_t element_type, uint32_t length_id)
{
    return vkd3d_spirv_build_op_r2(builder, &builder->global_stream,
            SpvOpTypeArray, element_type, length_id);
}

static uint32_t vkd3d_spirv_get_op_type_array(struct vkd3d_spirv_builder *builder,
        uint32_t element_type, uint32_t length_id)
{
    return vkd3d_spirv_build_once2(builder, SpvOpTypeArray, element_type, length_id,
            vkd3d_spirv_build_op_type_array);
}

static uint32_t vkd3d_spirv_build_op_type_runtime_array(struct vkd3d_spirv_builder *builder, uint32_t element_type)
{
    return vkd3d_spirv_build_op_r1(builder, &builder->global_stream, SpvOpTypeRuntimeArray, element_type);
}

static uint32_t vkd3d_spirv_get_op_type_runtime_array(struct vkd3d_spirv_builder *builder, uint32_t element_type)
{
    return vkd3d_spirv_build_once1(builder, SpvOpTypeRuntimeArray,
            element_type, vkd3d_spirv_build_op_type_runtime_array);
}

static uint32_t vkd3d_spirv_build_op_type_struct(struct vkd3d_spirv_builder *builder,
        uint32_t *members, unsigned int member_count)
{
    return vkd3d_spirv_build_op_rv(builder, &builder->global_stream,
            SpvOpTypeStruct, members, member_count);
}

static uint32_t vkd3d_spirv_build_op_type_sampler(struct vkd3d_spirv_builder *builder)
{
    return vkd3d_spirv_build_op_r(builder, &builder->global_stream, SpvOpTypeSampler);
}

static uint32_t vkd3d_spirv_get_op_type_sampler(struct vkd3d_spirv_builder *builder)
{
    return vkd3d_spirv_build_once(builder, &builder->type_sampler_id, vkd3d_spirv_build_op_type_sampler);
}

/* Access qualifiers are not supported. */
static uint32_t vkd3d_spirv_build_op_type_image(struct vkd3d_spirv_builder *builder,
        uint32_t sampled_type_id, uint32_t dim, uint32_t depth, uint32_t arrayed,
        uint32_t ms, uint32_t sampled, uint32_t format)
{
    uint32_t operands[] = {sampled_type_id, dim, depth, arrayed, ms, sampled, format};
    return vkd3d_spirv_build_op_rv(builder, &builder->global_stream,
            SpvOpTypeImage, operands, ARRAY_SIZE(operands));
}

static uint32_t vkd3d_spirv_get_op_type_image(struct vkd3d_spirv_builder *builder,
        uint32_t sampled_type_id, SpvDim dim, uint32_t depth, uint32_t arrayed,
        uint32_t ms, uint32_t sampled, SpvImageFormat format)
{
    uint32_t operands[] = {sampled_type_id, dim, depth, arrayed, ms, sampled, format};
    return vkd3d_spirv_build_once7(builder, SpvOpTypeImage, operands,
            vkd3d_spirv_build_op_type_image);
}

static uint32_t vkd3d_spirv_build_op_type_sampled_image(struct vkd3d_spirv_builder *builder,
        uint32_t image_type_id)
{
    return vkd3d_spirv_build_op_r1(builder, &builder->global_stream,
            SpvOpTypeSampledImage, image_type_id);
}

static uint32_t vkd3d_spirv_get_op_type_sampled_image(struct vkd3d_spirv_builder *builder,
        uint32_t image_type_id)
{
    return vkd3d_spirv_build_once1(builder, SpvOpTypeSampledImage, image_type_id,
            vkd3d_spirv_build_op_type_sampled_image);
}

static uint32_t vkd3d_spirv_build_op_type_function(struct vkd3d_spirv_builder *builder,
        uint32_t return_type, const uint32_t *param_types, unsigned int param_count)
{
    return vkd3d_spirv_build_op_r1v(builder, &builder->global_stream,
            SpvOpTypeFunction, return_type, param_types, param_count);
}

static uint32_t vkd3d_spirv_get_op_type_function(struct vkd3d_spirv_builder *builder,
        uint32_t return_type, const uint32_t *param_types, unsigned int param_count)
{
    return vkd3d_spirv_build_once1v(builder, SpvOpTypeFunction, return_type,
            param_types, param_count, vkd3d_spirv_build_op_type_function);
}

static uint32_t vkd3d_spirv_build_op_type_pointer(struct vkd3d_spirv_builder *builder,
        uint32_t storage_class, uint32_t type_id)
{
    return vkd3d_spirv_build_op_r2(builder, &builder->global_stream,
            SpvOpTypePointer, storage_class, type_id);
}

static uint32_t vkd3d_spirv_get_op_type_pointer(struct vkd3d_spirv_builder *builder,
        uint32_t storage_class, uint32_t type_id)
{
    return vkd3d_spirv_build_once2(builder, SpvOpTypePointer, storage_class, type_id,
            vkd3d_spirv_build_op_type_pointer);
}

static uint32_t vkd3d_spirv_build_op_constant_bool(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t value)
{
    return vkd3d_spirv_build_op_tr(builder, &builder->global_stream,
            value ? SpvOpConstantTrue : SpvOpConstantFalse, result_type);
}

static uint32_t vkd3d_spirv_get_op_constant_bool(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t value)
{
    return vkd3d_spirv_build_once2(builder, value ? SpvOpConstantTrue : SpvOpConstantFalse, result_type, value,
            vkd3d_spirv_build_op_constant_bool);
}

/* Types larger than 32-bits are not supported. */
static uint32_t vkd3d_spirv_build_op_constant(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t value)
{
    return vkd3d_spirv_build_op_tr1(builder, &builder->global_stream,
            SpvOpConstant, result_type, value);
}

static uint32_t vkd3d_spirv_get_op_constant(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t value)
{
    return vkd3d_spirv_build_once2(builder, SpvOpConstant, result_type, value,
            vkd3d_spirv_build_op_constant);
}

static uint32_t vkd3d_spirv_build_op_constant64(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, const uint32_t *values, unsigned int value_count)
{
    VKD3D_ASSERT(value_count == 2);
    return vkd3d_spirv_build_op_trv(builder, &builder->global_stream,
            SpvOpConstant, result_type, values, value_count);
}

static uint32_t vkd3d_spirv_get_op_constant64(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint64_t value)
{
    return vkd3d_spirv_build_once1v(builder, SpvOpConstant, result_type,
            (const uint32_t *)&value, 2, vkd3d_spirv_build_op_constant64);
}

static uint32_t vkd3d_spirv_build_op_constant_null(struct vkd3d_spirv_builder *builder, uint32_t result_type)
{
    return vkd3d_spirv_build_op_tr(builder, &builder->global_stream, SpvOpConstantNull, result_type);
}

static uint32_t vkd3d_spirv_get_op_constant_null(struct vkd3d_spirv_builder *builder, uint32_t result_type)
{
    return vkd3d_spirv_build_once1(builder, SpvOpConstantNull, result_type, vkd3d_spirv_build_op_constant_null);
}

static uint32_t vkd3d_spirv_build_op_constant_composite(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, const uint32_t *constituents, unsigned int constituent_count)
{
    return vkd3d_spirv_build_op_trv(builder, &builder->global_stream,
            SpvOpConstantComposite, result_type, constituents, constituent_count);
}

static uint32_t vkd3d_spirv_build_op_spec_constant_composite(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, const uint32_t *constituents, unsigned int constituent_count)
{
    return vkd3d_spirv_build_op_trv(builder, &builder->global_stream,
            SpvOpSpecConstantComposite, result_type, constituents, constituent_count);
}

static uint32_t vkd3d_spirv_get_op_constant_composite(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, const uint32_t *constituents, unsigned int constituent_count)
{
    return vkd3d_spirv_build_once1v(builder, SpvOpConstantComposite, result_type,
            constituents, constituent_count, vkd3d_spirv_build_op_constant_composite);
}

static uint32_t vkd3d_spirv_build_op_spec_constant(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t value)
{
    return vkd3d_spirv_build_op_tr1(builder, &builder->global_stream,
            SpvOpSpecConstant, result_type, value);
}

static uint32_t vkd3d_spirv_build_op_variable(struct vkd3d_spirv_builder *builder,
        struct vkd3d_spirv_stream *stream, uint32_t type_id, uint32_t storage_class, uint32_t initializer)
{
    return vkd3d_spirv_build_op_tr1v(builder, stream,
            SpvOpVariable, type_id, storage_class, &initializer, !!initializer);
}

static uint32_t vkd3d_spirv_build_op_function(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t result_id, uint32_t function_control, uint32_t function_type)
{
    vkd3d_spirv_build_op3v(&builder->function_stream,
            SpvOpFunction, result_type, result_id, function_control, &function_type, 1);
    return result_id;
}

static uint32_t vkd3d_spirv_build_op_function_parameter(struct vkd3d_spirv_builder *builder,
        uint32_t result_type)
{
    return vkd3d_spirv_build_op_tr(builder, &builder->function_stream,
            SpvOpFunctionParameter, result_type);
}

static void vkd3d_spirv_build_op_function_end(struct vkd3d_spirv_builder *builder)
{
    vkd3d_spirv_build_op(&builder->function_stream, SpvOpFunctionEnd);
}

static uint32_t vkd3d_spirv_build_op_function_call(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t function_id, const uint32_t *arguments, unsigned int argument_count)
{
    return vkd3d_spirv_build_op_tr1v(builder, &builder->function_stream,
            SpvOpFunctionCall, result_type, function_id, arguments, argument_count);
}

static uint32_t vkd3d_spirv_build_op_undef(struct vkd3d_spirv_builder *builder, uint32_t type_id)
{
    return vkd3d_spirv_build_op_tr(builder, &builder->global_stream, SpvOpUndef, type_id);
}

static uint32_t vkd3d_spirv_get_op_undef(struct vkd3d_spirv_builder *builder, uint32_t type_id)
{
    return vkd3d_spirv_build_once1(builder, SpvOpUndef, type_id, vkd3d_spirv_build_op_undef);
}

static uint32_t vkd3d_spirv_build_op_access_chain(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t base_id, uint32_t *indexes, uint32_t index_count)
{
    return vkd3d_spirv_build_op_tr1v(builder, &builder->function_stream,
            SpvOpAccessChain, result_type, base_id, indexes, index_count);
}

static uint32_t vkd3d_spirv_build_op_access_chain1(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t base_id, uint32_t index)
{
    return vkd3d_spirv_build_op_access_chain(builder, result_type, base_id, &index, 1);
}

static uint32_t vkd3d_spirv_build_op_in_bounds_access_chain(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t base_id, uint32_t *indexes, uint32_t index_count)
{
    return vkd3d_spirv_build_op_tr1v(builder, &builder->function_stream,
            SpvOpInBoundsAccessChain, result_type, base_id, indexes, index_count);
}

static uint32_t vkd3d_spirv_build_op_in_bounds_access_chain1(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t base_id, uint32_t index)
{
    return vkd3d_spirv_build_op_in_bounds_access_chain(builder, result_type, base_id, &index, 1);
}

static uint32_t vkd3d_spirv_build_op_vector_shuffle(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t vector1_id, uint32_t vector2_id,
        const uint32_t *components, uint32_t component_count)
{
    return vkd3d_spirv_build_op_tr2v(builder, &builder->function_stream, SpvOpVectorShuffle,
            result_type, vector1_id, vector2_id, components, component_count);
}

static uint32_t vkd3d_spirv_build_op_composite_construct(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, const uint32_t *constituents, unsigned int constituent_count)
{
    return vkd3d_spirv_build_op_trv(builder, &builder->function_stream, SpvOpCompositeConstruct,
            result_type, constituents, constituent_count);
}

static uint32_t vkd3d_spirv_build_op_composite_extract(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t composite_id, const uint32_t *indexes, unsigned int index_count)
{
    return vkd3d_spirv_build_op_tr1v(builder, &builder->function_stream, SpvOpCompositeExtract,
            result_type, composite_id, indexes, index_count);
}

static uint32_t vkd3d_spirv_build_op_composite_extract1(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t composite_id, uint32_t index)
{
    return vkd3d_spirv_build_op_composite_extract(builder, result_type, composite_id, &index, 1);
}

static uint32_t vkd3d_spirv_build_op_composite_insert(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t object_id, uint32_t composite_id,
        const uint32_t *indexes, unsigned int index_count)
{
    return vkd3d_spirv_build_op_tr2v(builder, &builder->function_stream, SpvOpCompositeInsert,
            result_type, object_id, composite_id, indexes, index_count);
}

static uint32_t vkd3d_spirv_build_op_composite_insert1(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t object_id, uint32_t composite_id, uint32_t index)
{
    return vkd3d_spirv_build_op_composite_insert(builder, result_type, object_id, composite_id, &index, 1);
}

static uint32_t vkd3d_spirv_build_op_load(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t pointer_id, uint32_t memory_access)
{
    if (!memory_access)
        return vkd3d_spirv_build_op_tr1(builder, &builder->function_stream, SpvOpLoad,
                result_type, pointer_id);
    else
        return vkd3d_spirv_build_op_tr2(builder, &builder->function_stream, SpvOpLoad,
                result_type, pointer_id, memory_access);
}

static void vkd3d_spirv_build_op_store(struct vkd3d_spirv_builder *builder,
        uint32_t pointer_id, uint32_t object_id, uint32_t memory_access)
{
    if (!memory_access)
        return vkd3d_spirv_build_op2(&builder->function_stream, SpvOpStore,
                pointer_id, object_id);
    else
        return vkd3d_spirv_build_op3(&builder->function_stream, SpvOpStore,
                pointer_id, object_id, memory_access);
}

static void vkd3d_spirv_build_op_copy_memory(struct vkd3d_spirv_builder *builder,
        uint32_t target_id, uint32_t source_id, uint32_t memory_access)
{
    if (!memory_access)
        return vkd3d_spirv_build_op2(&builder->function_stream, SpvOpCopyMemory,
                target_id, source_id);
    else
        return vkd3d_spirv_build_op3(&builder->function_stream, SpvOpCopyMemory,
                target_id, source_id, memory_access);
}

static uint32_t vkd3d_spirv_build_op_select(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t condition_id, uint32_t object0_id, uint32_t object1_id)
{
    return vkd3d_spirv_build_op_tr3(builder, &builder->function_stream,
            SpvOpSelect, result_type, condition_id, object0_id, object1_id);
}

static void vkd3d_spirv_build_op_kill(struct vkd3d_spirv_builder *builder)
{
    vkd3d_spirv_build_op(&builder->function_stream, SpvOpKill);
}

static void vkd3d_spirv_build_op_demote_to_helper_invocation(struct vkd3d_spirv_builder *builder)
{
    vkd3d_spirv_build_op(&builder->function_stream, SpvOpDemoteToHelperInvocationEXT);
}

static void vkd3d_spirv_build_op_return(struct vkd3d_spirv_builder *builder)
{
    vkd3d_spirv_build_op(&builder->function_stream, SpvOpReturn);
}

static uint32_t vkd3d_spirv_build_op_label(struct vkd3d_spirv_builder *builder,
        uint32_t label_id)
{
    vkd3d_spirv_build_op1(&builder->function_stream, SpvOpLabel, label_id);
    return label_id;
}

/* Loop control parameters are not supported. */
static void vkd3d_spirv_build_op_loop_merge(struct vkd3d_spirv_builder *builder,
        uint32_t merge_block, uint32_t continue_target, SpvLoopControlMask loop_control)
{
    vkd3d_spirv_build_op3(&builder->function_stream, SpvOpLoopMerge,
            merge_block, continue_target, loop_control);
}

static void vkd3d_spirv_build_op_selection_merge(struct vkd3d_spirv_builder *builder,
        uint32_t merge_block, uint32_t selection_control)
{
    vkd3d_spirv_build_op2(&builder->function_stream, SpvOpSelectionMerge,
            merge_block, selection_control);
}

static void vkd3d_spirv_build_op_branch(struct vkd3d_spirv_builder *builder, uint32_t label)
{
    vkd3d_spirv_build_op1(&builder->function_stream, SpvOpBranch, label);
}

/* Branch weights are not supported. */
static void vkd3d_spirv_build_op_branch_conditional(struct vkd3d_spirv_builder *builder,
        uint32_t condition, uint32_t true_label, uint32_t false_label)
{
    vkd3d_spirv_build_op3(&builder->function_stream, SpvOpBranchConditional,
            condition, true_label, false_label);
}

static void vkd3d_spirv_build_op_switch(struct vkd3d_spirv_builder *builder,
        uint32_t selector, uint32_t default_label, uint32_t *targets, unsigned int target_count)
{
    vkd3d_spirv_build_op2v(&builder->function_stream, SpvOpSwitch,
            selector, default_label, targets, 2 * target_count);
}

static uint32_t vkd3d_spirv_build_op_iadd(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t operand0, uint32_t operand1)
{
    return vkd3d_spirv_build_op_tr2(builder, &builder->function_stream,
            SpvOpIAdd, result_type, operand0, operand1);
}

static uint32_t vkd3d_spirv_build_op_imul(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t operand0, uint32_t operand1)
{
    return vkd3d_spirv_build_op_tr2(builder, &builder->function_stream,
            SpvOpIMul, result_type, operand0, operand1);
}

static uint32_t vkd3d_spirv_build_op_udiv(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t operand0, uint32_t operand1)
{
    return vkd3d_spirv_build_op_tr2(builder, &builder->function_stream,
            SpvOpUDiv, result_type, operand0, operand1);
}

static uint32_t vkd3d_spirv_build_op_isub(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t operand0, uint32_t operand1)
{
    return vkd3d_spirv_build_op_tr2(builder, &builder->function_stream,
            SpvOpISub, result_type, operand0, operand1);
}

static uint32_t vkd3d_spirv_build_op_fdiv(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t operand0, uint32_t operand1)
{
    return vkd3d_spirv_build_op_tr2(builder, &builder->function_stream,
            SpvOpFDiv, result_type, operand0, operand1);
}

static uint32_t vkd3d_spirv_build_op_fnegate(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t operand)
{
    return vkd3d_spirv_build_op_tr1(builder, &builder->function_stream,
            SpvOpFNegate, result_type, operand);
}

static uint32_t vkd3d_spirv_build_op_snegate(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t operand)
{
    return vkd3d_spirv_build_op_tr1(builder, &builder->function_stream,
            SpvOpSNegate, result_type, operand);
}

static uint32_t vkd3d_spirv_build_op_and(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t operand0, uint32_t operand1)
{
    return vkd3d_spirv_build_op_tr2(builder, &builder->function_stream,
            SpvOpBitwiseAnd, result_type, operand0, operand1);
}

static uint32_t vkd3d_spirv_build_op_shift_left_logical(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t base, uint32_t shift)
{
    return vkd3d_spirv_build_op_tr2(builder, &builder->function_stream,
            SpvOpShiftLeftLogical, result_type, base, shift);
}

static uint32_t vkd3d_spirv_build_op_shift_right_logical(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t base, uint32_t shift)
{
    return vkd3d_spirv_build_op_tr2(builder, &builder->function_stream,
            SpvOpShiftRightLogical, result_type, base, shift);
}

static uint32_t vkd3d_spirv_build_op_logical_and(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t operand0, uint32_t operand1)
{
    return vkd3d_spirv_build_op_tr2(builder, &builder->function_stream,
            SpvOpLogicalAnd, result_type, operand0, operand1);
}

static uint32_t vkd3d_spirv_build_op_uless_than(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t operand0, uint32_t operand1)
{
    return vkd3d_spirv_build_op_tr2(builder, &builder->function_stream,
            SpvOpULessThan, result_type, operand0, operand1);
}

static uint32_t vkd3d_spirv_build_op_uless_than_equal(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t operand0, uint32_t operand1)
{
    return vkd3d_spirv_build_op_tr2(builder, &builder->function_stream,
            SpvOpULessThanEqual, result_type, operand0, operand1);
}

static uint32_t vkd3d_spirv_build_op_is_inf(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t operand)
{
    return vkd3d_spirv_build_op_tr1(builder, &builder->function_stream, SpvOpIsInf, result_type, operand);
}

static uint32_t vkd3d_spirv_build_op_is_nan(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t operand)
{
    return vkd3d_spirv_build_op_tr1(builder, &builder->function_stream, SpvOpIsNan, result_type, operand);
}

static uint32_t vkd3d_spirv_build_op_logical_equal(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t operand0, uint32_t operand1)
{
    return vkd3d_spirv_build_op_tr2(builder, &builder->function_stream,
            SpvOpLogicalEqual, result_type, operand0, operand1);
}

static uint32_t vkd3d_spirv_build_op_logical_or(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t operand0, uint32_t operand1)
{
    return vkd3d_spirv_build_op_tr2(builder, &builder->function_stream,
            SpvOpLogicalOr, result_type, operand0, operand1);
}

static uint32_t vkd3d_spirv_build_op_logical_not(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t operand)
{
    return vkd3d_spirv_build_op_tr1(builder, &builder->function_stream, SpvOpLogicalNot, result_type, operand);
}

static uint32_t vkd3d_spirv_build_op_convert_utof(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t unsigned_value)
{
    return vkd3d_spirv_build_op_tr1(builder, &builder->function_stream,
            SpvOpConvertUToF, result_type, unsigned_value);
}

static uint32_t vkd3d_spirv_build_op_bitcast(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t operand)
{
    return vkd3d_spirv_build_op_tr1(builder, &builder->function_stream,
            SpvOpBitcast, result_type, operand);
}

static uint32_t vkd3d_spirv_build_op_image_texel_pointer(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t image_id, uint32_t coordinate_id, uint32_t sample_id)
{
    return vkd3d_spirv_build_op_tr3(builder, &builder->function_stream,
            SpvOpImageTexelPointer, result_type, image_id, coordinate_id, sample_id);
}

static uint32_t vkd3d_spirv_build_op_sampled_image(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t image_id, uint32_t sampler_id)
{
    return vkd3d_spirv_build_op_tr2(builder, &builder->function_stream,
            SpvOpSampledImage, result_type, image_id, sampler_id);
}

static uint32_t vkd3d_spirv_build_op_image(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t sampled_image_id)
{
    return vkd3d_spirv_build_op_tr1(builder, &builder->function_stream,
            SpvOpImage, result_type, sampled_image_id);
}

static uint32_t vkd3d_spirv_build_image_instruction(struct vkd3d_spirv_builder *builder,
        SpvOp op, uint32_t result_type, const uint32_t *operands, unsigned int operand_count,
        uint32_t image_operands_mask, const uint32_t *image_operands, unsigned int image_operand_count)
{
    unsigned int index = 0, i;
    uint32_t w[10];

    VKD3D_ASSERT(operand_count <= ARRAY_SIZE(w));
    for (i = 0; i < operand_count; ++i)
        w[index++] = operands[i];

    if (image_operands_mask)
    {
        VKD3D_ASSERT(index + 1 + image_operand_count <= ARRAY_SIZE(w));
        w[index++] = image_operands_mask;
        for (i = 0; i < image_operand_count; ++i)
            w[index++] = image_operands[i];
    }

    return vkd3d_spirv_build_op_trv(builder, &builder->function_stream,
            op, result_type, w, index);
}

static uint32_t vkd3d_spirv_build_op_image_sample(struct vkd3d_spirv_builder *builder,
        SpvOp op, uint32_t result_type, uint32_t sampled_image_id, uint32_t coordinate_id,
        uint32_t image_operands_mask, const uint32_t *image_operands, unsigned int image_operand_count)
{
    const uint32_t operands[] = {sampled_image_id, coordinate_id};

    if (op == SpvOpImageSampleExplicitLod)
        VKD3D_ASSERT(image_operands_mask & (SpvImageOperandsLodMask | SpvImageOperandsGradMask));
    else
        VKD3D_ASSERT(op == SpvOpImageSampleImplicitLod);

    return vkd3d_spirv_build_image_instruction(builder, op, result_type,
            operands, ARRAY_SIZE(operands), image_operands_mask, image_operands, image_operand_count);
}

static uint32_t vkd3d_spirv_build_op_image_sample_dref(struct vkd3d_spirv_builder *builder,
        SpvOp op, uint32_t result_type, uint32_t sampled_image_id, uint32_t coordinate_id, uint32_t dref_id,
        uint32_t image_operands_mask, const uint32_t *image_operands, unsigned int image_operand_count)
{
    const uint32_t operands[] = {sampled_image_id, coordinate_id, dref_id};

    if (op == SpvOpImageSampleDrefExplicitLod)
        VKD3D_ASSERT(image_operands_mask & (SpvImageOperandsLodMask | SpvImageOperandsGradMask));
    else
        VKD3D_ASSERT(op == SpvOpImageSampleDrefImplicitLod);

    return vkd3d_spirv_build_image_instruction(builder, op, result_type,
            operands, ARRAY_SIZE(operands), image_operands_mask, image_operands, image_operand_count);
}

static uint32_t vkd3d_spirv_build_op_image_gather(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t sampled_image_id, uint32_t coordinate_id, uint32_t component_id,
        uint32_t image_operands_mask, const uint32_t *image_operands, unsigned int image_operand_count)
{
    const uint32_t operands[] = {sampled_image_id, coordinate_id, component_id};
    return vkd3d_spirv_build_image_instruction(builder, SpvOpImageGather, result_type,
            operands, ARRAY_SIZE(operands), image_operands_mask, image_operands, image_operand_count);
}

static uint32_t vkd3d_spirv_build_op_image_dref_gather(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t sampled_image_id, uint32_t coordinate_id, uint32_t dref_id,
        uint32_t image_operands_mask, const uint32_t *image_operands, unsigned int image_operand_count)
{
    const uint32_t operands[] = {sampled_image_id, coordinate_id, dref_id};
    return vkd3d_spirv_build_image_instruction(builder, SpvOpImageDrefGather, result_type,
            operands, ARRAY_SIZE(operands), image_operands_mask, image_operands, image_operand_count);
}

static uint32_t vkd3d_spirv_build_op_image_fetch(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t image_id, uint32_t coordinate_id,
        uint32_t image_operands_mask, const uint32_t *image_operands, unsigned int image_operand_count)
{
    const uint32_t operands[] = {image_id, coordinate_id};
    return vkd3d_spirv_build_image_instruction(builder, SpvOpImageFetch, result_type,
            operands, ARRAY_SIZE(operands), image_operands_mask, image_operands, image_operand_count);
}

static uint32_t vkd3d_spirv_build_op_image_read(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t image_id, uint32_t coordinate_id,
        uint32_t image_operands_mask, const uint32_t *image_operands, unsigned int image_operand_count)
{
    const uint32_t operands[] = {image_id, coordinate_id};
    return vkd3d_spirv_build_image_instruction(builder, SpvOpImageRead, result_type,
            operands, ARRAY_SIZE(operands), image_operands_mask, image_operands, image_operand_count);
}

static void vkd3d_spirv_build_op_image_write(struct vkd3d_spirv_builder *builder,
        uint32_t image_id, uint32_t coordinate_id, uint32_t texel_id,
        uint32_t image_operands, const uint32_t *operands, unsigned int operand_count)
{
    if (image_operands)
        FIXME("Image operands not supported.\n");

    vkd3d_spirv_build_op3(&builder->function_stream, SpvOpImageWrite,
            image_id, coordinate_id, texel_id);
}

static uint32_t vkd3d_spirv_build_op_array_length(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t struct_id, uint32_t member_id)
{
    return vkd3d_spirv_build_op_tr2(builder, &builder->function_stream,
            SpvOpArrayLength, result_type, struct_id, member_id);
}

static uint32_t vkd3d_spirv_build_op_image_query_size_lod(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t image_id, uint32_t lod_id)
{
    return vkd3d_spirv_build_op_tr2(builder, &builder->function_stream,
            SpvOpImageQuerySizeLod, result_type, image_id, lod_id);
}

static uint32_t vkd3d_spirv_build_op_image_query_size(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t image_id)
{
    return vkd3d_spirv_build_op_tr1(builder, &builder->function_stream,
            SpvOpImageQuerySize, result_type, image_id);
}

static uint32_t vkd3d_spirv_build_op_image_query_levels(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t image_id)
{
    return vkd3d_spirv_build_op_tr1(builder, &builder->function_stream,
            SpvOpImageQueryLevels, result_type, image_id);
}

static uint32_t vkd3d_spirv_build_op_image_query_samples(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t image_id)
{
    return vkd3d_spirv_build_op_tr1(builder, &builder->function_stream,
            SpvOpImageQuerySamples, result_type, image_id);
}

static uint32_t vkd3d_spirv_build_op_image_query_lod(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t image_id, uint32_t coordinate_id)
{
    return vkd3d_spirv_build_op_tr2(builder, &builder->function_stream,
            SpvOpImageQueryLod, result_type, image_id, coordinate_id);
}

static void vkd3d_spirv_build_op_emit_vertex(struct vkd3d_spirv_builder *builder)
{
    return vkd3d_spirv_build_op(&builder->function_stream, SpvOpEmitVertex);
}

static void vkd3d_spirv_build_op_end_primitive(struct vkd3d_spirv_builder *builder)
{
    return vkd3d_spirv_build_op(&builder->function_stream, SpvOpEndPrimitive);
}

static void vkd3d_spirv_build_op_control_barrier(struct vkd3d_spirv_builder *builder,
        uint32_t execution_id, uint32_t memory_id, uint32_t memory_semantics_id)
{
    vkd3d_spirv_build_op3(&builder->function_stream,
            SpvOpControlBarrier, execution_id, memory_id, memory_semantics_id);
}

static void vkd3d_spirv_build_op_memory_barrier(struct vkd3d_spirv_builder *builder,
        uint32_t memory_id, uint32_t memory_semantics_id)
{
    vkd3d_spirv_build_op2(&builder->function_stream,
            SpvOpMemoryBarrier, memory_id, memory_semantics_id);
}

static uint32_t vkd3d_spirv_build_op_scope_subgroup(struct vkd3d_spirv_builder *builder)
{
    return vkd3d_spirv_get_op_constant(builder, vkd3d_spirv_get_op_type_int(builder, 32, 0), SpvScopeSubgroup);
}

static uint32_t vkd3d_spirv_get_op_scope_subgroup(struct vkd3d_spirv_builder *builder)
{
    return vkd3d_spirv_build_once(builder, &builder->scope_subgroup_id, vkd3d_spirv_build_op_scope_subgroup);
}

static uint32_t vkd3d_spirv_build_op_group_nonuniform_quad_swap(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t val_id, uint32_t op_id)
{
    vkd3d_spirv_enable_capability(builder, SpvCapabilityGroupNonUniformQuad);
    return vkd3d_spirv_build_op_tr3(builder, &builder->function_stream, SpvOpGroupNonUniformQuadSwap, result_type,
            vkd3d_spirv_get_op_scope_subgroup(builder), val_id, op_id);
}

static uint32_t vkd3d_spirv_build_op_group_nonuniform_quad_broadcast(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t val_id, uint32_t index_id)
{
    vkd3d_spirv_enable_capability(builder, SpvCapabilityGroupNonUniformQuad);
    return vkd3d_spirv_build_op_tr3(builder, &builder->function_stream, SpvOpGroupNonUniformQuadBroadcast, result_type,
            vkd3d_spirv_get_op_scope_subgroup(builder), val_id, index_id);
}

static uint32_t vkd3d_spirv_build_op_group_nonuniform_ballot(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t val_id)
{
    vkd3d_spirv_enable_capability(builder, SpvCapabilityGroupNonUniformBallot);
    return vkd3d_spirv_build_op_tr2(builder, &builder->function_stream, SpvOpGroupNonUniformBallot,
            result_type, vkd3d_spirv_get_op_scope_subgroup(builder), val_id);
}

static uint32_t vkd3d_spirv_build_op_group_nonuniform_ballot_bit_count(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, SpvGroupOperation group_op, uint32_t val_id)
{
    vkd3d_spirv_enable_capability(builder, SpvCapabilityGroupNonUniformBallot);
    return vkd3d_spirv_build_op_tr3(builder, &builder->function_stream, SpvOpGroupNonUniformBallotBitCount,
            result_type, vkd3d_spirv_get_op_scope_subgroup(builder), group_op, val_id);
}

static uint32_t vkd3d_spirv_build_op_group_nonuniform_elect(struct vkd3d_spirv_builder *builder)
{
    vkd3d_spirv_enable_capability(builder, SpvCapabilityGroupNonUniform);
    return vkd3d_spirv_build_op_tr1(builder, &builder->function_stream, SpvOpGroupNonUniformElect,
            vkd3d_spirv_get_op_type_bool(builder), vkd3d_spirv_get_op_scope_subgroup(builder));
}

static uint32_t vkd3d_spirv_build_op_group_nonuniform_broadcast(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t val_id, uint32_t lane_id)
{
    vkd3d_spirv_enable_capability(builder, SpvCapabilityGroupNonUniformBallot);
    return vkd3d_spirv_build_op_tr3(builder, &builder->function_stream, SpvOpGroupNonUniformBroadcast, result_type,
            vkd3d_spirv_get_op_scope_subgroup(builder), val_id, lane_id);
}

static uint32_t vkd3d_spirv_build_op_group_nonuniform_shuffle(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t val_id, uint32_t lane_id)
{
    vkd3d_spirv_enable_capability(builder, SpvCapabilityGroupNonUniformShuffle);
    return vkd3d_spirv_build_op_tr3(builder, &builder->function_stream, SpvOpGroupNonUniformShuffle, result_type,
            vkd3d_spirv_get_op_scope_subgroup(builder), val_id, lane_id);
}

static uint32_t vkd3d_spirv_build_op_group_nonuniform_broadcast_first(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t val_id)
{
    vkd3d_spirv_enable_capability(builder, SpvCapabilityGroupNonUniformBallot);
    return vkd3d_spirv_build_op_tr2(builder, &builder->function_stream, SpvOpGroupNonUniformBroadcastFirst,
            result_type, vkd3d_spirv_get_op_scope_subgroup(builder), val_id);
}

static uint32_t vkd3d_spirv_build_op_glsl_std450_tr1(struct vkd3d_spirv_builder *builder,
        enum GLSLstd450 op, uint32_t result_type, uint32_t operand)
{
    uint32_t id = vkd3d_spirv_get_glsl_std450_instr_set(builder);
    return vkd3d_spirv_build_op_ext_inst(builder, result_type, id, op, &operand, 1);
}

static uint32_t vkd3d_spirv_build_op_glsl_std450_fabs(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t operand)
{
    return vkd3d_spirv_build_op_glsl_std450_tr1(builder, GLSLstd450FAbs, result_type, operand);
}

static uint32_t vkd3d_spirv_build_op_glsl_std450_sin(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t operand)
{
    return vkd3d_spirv_build_op_glsl_std450_tr1(builder, GLSLstd450Sin, result_type, operand);
}

static uint32_t vkd3d_spirv_build_op_glsl_std450_cos(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t operand)
{
    return vkd3d_spirv_build_op_glsl_std450_tr1(builder, GLSLstd450Cos, result_type, operand);
}

static uint32_t vkd3d_spirv_build_op_glsl_std450_max(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t x, uint32_t y)
{
    uint32_t glsl_std450_id = vkd3d_spirv_get_glsl_std450_instr_set(builder);
    uint32_t operands[] = {x, y};
    return vkd3d_spirv_build_op_ext_inst(builder, result_type, glsl_std450_id,
            GLSLstd450NMax, operands, ARRAY_SIZE(operands));
}

static uint32_t vkd3d_spirv_build_op_glsl_std450_umin(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t x, uint32_t y)
{
    uint32_t glsl_std450_id = vkd3d_spirv_get_glsl_std450_instr_set(builder);
    uint32_t operands[] = {x, y};
    return vkd3d_spirv_build_op_ext_inst(builder, result_type, glsl_std450_id,
            GLSLstd450UMin, operands, ARRAY_SIZE(operands));
}

static uint32_t vkd3d_spirv_build_op_glsl_std450_nclamp(struct vkd3d_spirv_builder *builder,
        uint32_t result_type, uint32_t x, uint32_t min, uint32_t max)
{
    uint32_t glsl_std450_id = vkd3d_spirv_get_glsl_std450_instr_set(builder);
    uint32_t operands[] = {x, min, max};
    return vkd3d_spirv_build_op_ext_inst(builder, result_type, glsl_std450_id,
            GLSLstd450NClamp, operands, ARRAY_SIZE(operands));
}

static uint32_t vkd3d_spirv_get_type_id(struct vkd3d_spirv_builder *builder,
        enum vkd3d_shader_component_type component_type, unsigned int component_count)
{
    uint32_t scalar_id, type_id;

    VKD3D_ASSERT(component_type < VKD3D_SHADER_COMPONENT_TYPE_COUNT);
    if (!component_count || component_count > VKD3D_VEC4_SIZE)
    {
        ERR("Invalid component count %u.\n", component_count);
        return 0;
    }

    if ((type_id = builder->numeric_type_ids[component_type][component_count - 1]))
        return type_id;

    if (component_count == 1)
    {
        switch (component_type)
        {
            case VKD3D_SHADER_COMPONENT_VOID:
                type_id = vkd3d_spirv_get_op_type_void(builder);
                break;
            case VKD3D_SHADER_COMPONENT_FLOAT:
                type_id = vkd3d_spirv_get_op_type_float(builder, 32);
                break;
            case VKD3D_SHADER_COMPONENT_INT:
            case VKD3D_SHADER_COMPONENT_UINT:
                type_id = vkd3d_spirv_get_op_type_int(builder, 32, component_type == VKD3D_SHADER_COMPONENT_INT);
                break;
            case VKD3D_SHADER_COMPONENT_BOOL:
                type_id = vkd3d_spirv_get_op_type_bool(builder);
                break;
            case VKD3D_SHADER_COMPONENT_DOUBLE:
                type_id = vkd3d_spirv_get_op_type_float(builder, 64);
                break;
            case VKD3D_SHADER_COMPONENT_UINT64:
                type_id = vkd3d_spirv_get_op_type_int(builder, 64, 0);
                break;
            default:
                FIXME("Unhandled component type %#x.\n", component_type);
                return 0;
        }
    }
    else
    {
        VKD3D_ASSERT(component_type != VKD3D_SHADER_COMPONENT_VOID);
        scalar_id = vkd3d_spirv_get_type_id(builder, component_type, 1);
        type_id = vkd3d_spirv_get_op_type_vector(builder, scalar_id, component_count);
    }

    builder->numeric_type_ids[component_type][component_count - 1] = type_id;

    return type_id;
}

static uint32_t vkd3d_spirv_get_type_id_for_data_type(struct vkd3d_spirv_builder *builder,
        enum vkd3d_data_type data_type, unsigned int component_count)
{
    enum vkd3d_shader_component_type component_type;

    component_type = vkd3d_component_type_from_data_type(data_type);
    return vkd3d_spirv_get_type_id(builder, component_type, component_count);
}

static void vkd3d_spirv_builder_init(struct vkd3d_spirv_builder *builder, const char *entry_point)
{
    vkd3d_spirv_stream_init(&builder->debug_stream);
    vkd3d_spirv_stream_init(&builder->annotation_stream);
    vkd3d_spirv_stream_init(&builder->global_stream);
    vkd3d_spirv_stream_init(&builder->function_stream);
    vkd3d_spirv_stream_init(&builder->execution_mode_stream);

    vkd3d_spirv_stream_init(&builder->insertion_stream);
    builder->insertion_location = ~(size_t)0;

    builder->current_id = 1;

    rb_init(&builder->declarations, vkd3d_spirv_declaration_compare);

    builder->main_function_id = vkd3d_spirv_alloc_id(builder);
    vkd3d_spirv_build_op_name(builder, builder->main_function_id, "%s", entry_point);
}

static void vkd3d_spirv_builder_begin_main_function(struct vkd3d_spirv_builder *builder)
{
    uint32_t void_id, function_type_id;

    void_id = vkd3d_spirv_get_op_type_void(builder);
    function_type_id = vkd3d_spirv_get_op_type_function(builder, void_id, NULL, 0);

    vkd3d_spirv_build_op_function(builder, void_id,
            builder->main_function_id, SpvFunctionControlMaskNone, function_type_id);
}

static void vkd3d_spirv_builder_free(struct vkd3d_spirv_builder *builder)
{
    vkd3d_spirv_stream_free(&builder->debug_stream);
    vkd3d_spirv_stream_free(&builder->annotation_stream);
    vkd3d_spirv_stream_free(&builder->global_stream);
    vkd3d_spirv_stream_free(&builder->function_stream);
    vkd3d_spirv_stream_free(&builder->execution_mode_stream);

    vkd3d_spirv_stream_free(&builder->insertion_stream);

    vkd3d_free(builder->capabilities);

    rb_destroy(&builder->declarations, vkd3d_spirv_declaration_free, NULL);

    vkd3d_free(builder->iface);
}

static bool vkd3d_spirv_compile_module(struct vkd3d_spirv_builder *builder,
        struct vkd3d_shader_code *spirv, const char *entry_point, enum vkd3d_shader_spirv_environment environment)
{
    uint64_t capability_mask = builder->capability_mask;
    struct vkd3d_spirv_stream stream;
    unsigned int i;

    vkd3d_spirv_stream_init(&stream);

    vkd3d_spirv_build_word(&stream, SpvMagicNumber);
    vkd3d_spirv_build_word(&stream, (environment == VKD3D_SHADER_SPIRV_ENVIRONMENT_VULKAN_1_1)
            ? VKD3D_SPIRV_VERSION_1_3 : VKD3D_SPIRV_VERSION_1_0);
    vkd3d_spirv_build_word(&stream, VKD3D_SPIRV_GENERATOR_MAGIC);
    vkd3d_spirv_build_word(&stream, builder->current_id); /* bound */
    vkd3d_spirv_build_word(&stream, 0); /* schema, reserved */

    /* capabilities */
    for (i = 0; capability_mask; ++i)
    {
        if (capability_mask & 1)
            vkd3d_spirv_build_op_capability(&stream, i);
        capability_mask >>= 1;
    }
    for (i = 0; i < builder->capabilities_count; ++i)
        vkd3d_spirv_build_op_capability(&stream, builder->capabilities[i]);

    /* extensions */
    if (vkd3d_spirv_capability_is_enabled(builder, SpvCapabilityDrawParameters))
        vkd3d_spirv_build_op_extension(&stream, "SPV_KHR_shader_draw_parameters");
    if (vkd3d_spirv_capability_is_enabled(builder, SpvCapabilityDemoteToHelperInvocationEXT))
        vkd3d_spirv_build_op_extension(&stream, "SPV_EXT_demote_to_helper_invocation");
    if (vkd3d_spirv_capability_is_enabled(builder, SpvCapabilityRuntimeDescriptorArrayEXT)
            || vkd3d_spirv_capability_is_enabled(builder, SpvCapabilityUniformBufferArrayDynamicIndexing)
            || vkd3d_spirv_capability_is_enabled(builder, SpvCapabilityUniformTexelBufferArrayDynamicIndexingEXT)
            || vkd3d_spirv_capability_is_enabled(builder, SpvCapabilitySampledImageArrayDynamicIndexing)
            || vkd3d_spirv_capability_is_enabled(builder, SpvCapabilityStorageBufferArrayDynamicIndexing)
            || vkd3d_spirv_capability_is_enabled(builder, SpvCapabilityStorageTexelBufferArrayDynamicIndexingEXT)
            || vkd3d_spirv_capability_is_enabled(builder, SpvCapabilityStorageImageArrayDynamicIndexing)
            || vkd3d_spirv_capability_is_enabled(builder, SpvCapabilityShaderNonUniformEXT))
        vkd3d_spirv_build_op_extension(&stream, "SPV_EXT_descriptor_indexing");
    if (vkd3d_spirv_capability_is_enabled(builder, SpvCapabilityFragmentShaderPixelInterlockEXT)
            || vkd3d_spirv_capability_is_enabled(builder, SpvCapabilityFragmentShaderSampleInterlockEXT))
        vkd3d_spirv_build_op_extension(&stream, "SPV_EXT_fragment_shader_interlock");
    if (vkd3d_spirv_capability_is_enabled(builder, SpvCapabilityStencilExportEXT))
        vkd3d_spirv_build_op_extension(&stream, "SPV_EXT_shader_stencil_export");
    if (vkd3d_spirv_capability_is_enabled(builder, SpvCapabilityShaderViewportIndexLayerEXT))
        vkd3d_spirv_build_op_extension(&stream, "SPV_EXT_shader_viewport_index_layer");

    if (builder->ext_instr_set_glsl_450)
        vkd3d_spirv_build_op_ext_inst_import(&stream, builder->ext_instr_set_glsl_450, "GLSL.std.450");

    /* entry point declarations */
    vkd3d_spirv_build_op_memory_model(&stream, SpvAddressingModelLogical, SpvMemoryModelGLSL450);
    vkd3d_spirv_build_op_entry_point(&stream, builder->execution_model, builder->main_function_id,
            entry_point, builder->iface, builder->iface_element_count);

    /* execution mode declarations */
    if (builder->invocation_count)
        vkd3d_spirv_build_op_execution_mode(&builder->execution_mode_stream,
                builder->main_function_id, SpvExecutionModeInvocations, &builder->invocation_count, 1);

    if (!vkd3d_spirv_stream_append(&stream, &builder->execution_mode_stream)
            || !vkd3d_spirv_stream_append(&stream, &builder->debug_stream)
            || !vkd3d_spirv_stream_append(&stream, &builder->annotation_stream)
            || !vkd3d_spirv_stream_append(&stream, &builder->global_stream)
            || !vkd3d_spirv_stream_append(&stream, &builder->function_stream))
    {
        vkd3d_spirv_stream_free(&stream);
        return false;
    }

    vkd3d_shader_code_from_spirv_stream(spirv, &stream);
    vkd3d_spirv_stream_free(&stream);

    return true;
}

static const struct vkd3d_spirv_resource_type
{
    enum vkd3d_shader_resource_type resource_type;

    SpvDim dim;
    uint32_t arrayed;
    uint32_t ms;
    unsigned int coordinate_component_count;

    SpvCapability capability;
    SpvCapability uav_capability;
}
vkd3d_spirv_resource_type_table[] =
{
    {VKD3D_SHADER_RESOURCE_BUFFER,            SpvDimBuffer, 0, 0, 1,
            SpvCapabilitySampledBuffer, SpvCapabilityImageBuffer},
    {VKD3D_SHADER_RESOURCE_TEXTURE_1D,        SpvDim1D,     0, 0, 1,
            SpvCapabilitySampled1D, SpvCapabilityImage1D},
    {VKD3D_SHADER_RESOURCE_TEXTURE_2DMS,      SpvDim2D,     0, 1, 2},
    {VKD3D_SHADER_RESOURCE_TEXTURE_2D,        SpvDim2D,     0, 0, 2},
    {VKD3D_SHADER_RESOURCE_TEXTURE_3D,        SpvDim3D,     0, 0, 3},
    {VKD3D_SHADER_RESOURCE_TEXTURE_CUBE,      SpvDimCube,   0, 0, 3},
    {VKD3D_SHADER_RESOURCE_TEXTURE_1DARRAY,   SpvDim1D,     1, 0, 2,
            SpvCapabilitySampled1D, SpvCapabilityImage1D},
    {VKD3D_SHADER_RESOURCE_TEXTURE_2DARRAY,   SpvDim2D,     1, 0, 3},
    {VKD3D_SHADER_RESOURCE_TEXTURE_2DMSARRAY, SpvDim2D,     1, 1, 3},
    {VKD3D_SHADER_RESOURCE_TEXTURE_CUBEARRAY, SpvDimCube,   1, 0, 4,
            SpvCapabilitySampledCubeArray, SpvCapabilityImageCubeArray},
};

static const struct vkd3d_spirv_resource_type *vkd3d_get_spirv_resource_type(
        enum vkd3d_shader_resource_type resource_type)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(vkd3d_spirv_resource_type_table); ++i)
    {
        const struct vkd3d_spirv_resource_type* current = &vkd3d_spirv_resource_type_table[i];

        if (current->resource_type == resource_type)
            return current;
    }

    FIXME("Unhandled resource type %#x.\n", resource_type);
    return NULL;
}

struct vkd3d_symbol_register
{
    enum vkd3d_shader_register_type type;
    unsigned int idx;
};

struct vkd3d_symbol_resource
{
    enum vkd3d_shader_register_type type;
    unsigned int idx;
};

struct vkd3d_symbol_sampler
{
    unsigned int id;
};

struct vkd3d_symbol_combined_sampler
{
    enum vkd3d_shader_register_type resource_type;
    unsigned int resource_id;
    unsigned int sampler_space;
    unsigned int sampler_index;
};

struct vkd3d_symbol_descriptor_array
{
    uint32_t ptr_type_id;
    unsigned int set;
    unsigned int binding;
    unsigned int push_constant_index;
    bool write_only;
    bool coherent;
};

struct vkd3d_symbol_register_data
{
    SpvStorageClass storage_class;
    uint32_t member_idx;
    enum vkd3d_shader_component_type component_type;
    unsigned int write_mask;
    unsigned int structure_stride;
    unsigned int binding_base_idx;
    bool is_aggregate; /* An aggregate, i.e. a structure or an array. */
};

struct vkd3d_symbol_resource_data
{
    struct vkd3d_shader_register_range range;
    enum vkd3d_shader_component_type sampled_type;
    uint32_t type_id;
    const struct vkd3d_spirv_resource_type *resource_type_info;
    unsigned int structure_stride;
    bool raw;
    unsigned int binding_base_idx;
    uint32_t uav_counter_id;
    const struct vkd3d_symbol *uav_counter_array;
    unsigned int uav_counter_base_idx;
};

struct vkd3d_symbol_sampler_data
{
    struct vkd3d_shader_register_range range;
};

struct vkd3d_descriptor_binding_address
{
    unsigned int binding_base_idx;
    unsigned int push_constant_index;
};

struct vkd3d_symbol_descriptor_array_data
{
    SpvStorageClass storage_class;
    uint32_t contained_type_id;
};

struct vkd3d_symbol
{
    struct rb_entry entry;

    enum
    {
        VKD3D_SYMBOL_REGISTER,
        VKD3D_SYMBOL_RESOURCE,
        VKD3D_SYMBOL_SAMPLER,
        VKD3D_SYMBOL_COMBINED_SAMPLER,
        VKD3D_SYMBOL_DESCRIPTOR_ARRAY,
    } type;

    union
    {
        struct vkd3d_symbol_register reg;
        struct vkd3d_symbol_resource resource;
        struct vkd3d_symbol_sampler sampler;
        struct vkd3d_symbol_combined_sampler combined_sampler;
        struct vkd3d_symbol_descriptor_array descriptor_array;
    } key;

    uint32_t id;
    /* The array declaration which this symbol maps to, or NULL. */
    const struct vkd3d_symbol *descriptor_array;

    union
    {
        struct vkd3d_symbol_register_data reg;
        struct vkd3d_symbol_resource_data resource;
        struct vkd3d_symbol_sampler_data sampler;
        struct vkd3d_symbol_descriptor_array_data descriptor_array;
    } info;
};

static int vkd3d_symbol_compare(const void *key, const struct rb_entry *entry)
{
    const struct vkd3d_symbol *a = key;
    const struct vkd3d_symbol *b = RB_ENTRY_VALUE(entry, const struct vkd3d_symbol, entry);
    int ret;

    if ((ret = vkd3d_u32_compare(a->type, b->type)))
        return ret;
    return memcmp(&a->key, &b->key, sizeof(a->key));
}

static void vkd3d_symbol_free(struct rb_entry *entry, void *context)
{
    struct vkd3d_symbol *s = RB_ENTRY_VALUE(entry, struct vkd3d_symbol, entry);

    vkd3d_free(s);
}

static void vkd3d_symbol_make_register(struct vkd3d_symbol *symbol,
        const struct vkd3d_shader_register *reg)
{
    symbol->type = VKD3D_SYMBOL_REGISTER;
    memset(&symbol->key, 0, sizeof(symbol->key));
    symbol->key.reg.type = reg->type;

    switch (reg->type)
    {
        case VKD3DSPR_INPUT:
        case VKD3DSPR_OUTPUT:
        case VKD3DSPR_PATCHCONST:
            symbol->key.reg.idx = reg->idx_count ? reg->idx[reg->idx_count - 1].offset : ~0u;
            VKD3D_ASSERT(!reg->idx_count || symbol->key.reg.idx != ~0u);
            break;

        case VKD3DSPR_IMMCONSTBUFFER:
            symbol->key.reg.idx = reg->idx_count > 1 ? reg->idx[0].offset : 0;
            break;

        default:
            symbol->key.reg.idx = reg->idx_count ? reg->idx[0].offset : ~0u;
    }
}

static void vkd3d_symbol_make_io(struct vkd3d_symbol *symbol,
        enum vkd3d_shader_register_type type, unsigned int index)
{
    symbol->type = VKD3D_SYMBOL_REGISTER;
    memset(&symbol->key, 0, sizeof(symbol->key));
    symbol->key.reg.type = type;
    symbol->key.reg.idx = index;
}

static void vkd3d_symbol_set_register_info(struct vkd3d_symbol *symbol,
        uint32_t val_id, SpvStorageClass storage_class,
        enum vkd3d_shader_component_type component_type, uint32_t write_mask)
{
    symbol->id = val_id;
    symbol->descriptor_array = NULL;
    symbol->info.reg.storage_class = storage_class;
    symbol->info.reg.member_idx = 0;
    symbol->info.reg.component_type = component_type;
    symbol->info.reg.write_mask = write_mask;
    symbol->info.reg.structure_stride = 0;
    symbol->info.reg.binding_base_idx = 0;
    symbol->info.reg.is_aggregate = false;
}

static void vkd3d_symbol_make_resource(struct vkd3d_symbol *symbol,
        const struct vkd3d_shader_register *reg)
{
    symbol->type = VKD3D_SYMBOL_RESOURCE;
    memset(&symbol->key, 0, sizeof(symbol->key));
    symbol->key.resource.type = reg->type;
    symbol->key.resource.idx = reg->idx[0].offset;
}

static void vkd3d_symbol_make_sampler(struct vkd3d_symbol *symbol,
        const struct vkd3d_shader_register *reg)
{
    symbol->type = VKD3D_SYMBOL_SAMPLER;
    memset(&symbol->key, 0, sizeof(symbol->key));
    symbol->key.sampler.id = reg->idx[0].offset;
}

static void vkd3d_symbol_make_combined_sampler(struct vkd3d_symbol *symbol,
        const struct vkd3d_shader_register *resource_reg, unsigned int sampler_space, unsigned int sampler_index)
{
    symbol->type = VKD3D_SYMBOL_COMBINED_SAMPLER;
    memset(&symbol->key, 0, sizeof(symbol->key));
    symbol->key.combined_sampler.resource_type = resource_reg->type;
    symbol->key.combined_sampler.resource_id = resource_reg->idx[0].offset;
    symbol->key.combined_sampler.sampler_space = sampler_space;
    symbol->key.combined_sampler.sampler_index = sampler_index;
}

static struct vkd3d_symbol *vkd3d_symbol_dup(const struct vkd3d_symbol *symbol)
{
    struct vkd3d_symbol *s;

    if (!(s = vkd3d_malloc(sizeof(*s))))
        return NULL;

    return memcpy(s, symbol, sizeof(*s));
}

static const char *debug_vkd3d_symbol(const struct vkd3d_symbol *symbol)
{
    switch (symbol->type)
    {
        case VKD3D_SYMBOL_REGISTER:
            return vkd3d_dbg_sprintf("register %#x, %u",
                    symbol->key.reg.type, symbol->key.reg.idx);
        case VKD3D_SYMBOL_RESOURCE:
            return vkd3d_dbg_sprintf("resource %#x, %u",
                    symbol->key.resource.type, symbol->key.resource.idx);
        case VKD3D_SYMBOL_SAMPLER:
            return vkd3d_dbg_sprintf("sampler %u",
                    symbol->key.sampler.id);
        default:
            return vkd3d_dbg_sprintf("type %#x", symbol->type);
    }
}

struct vkd3d_push_constant_buffer_binding
{
    struct vkd3d_shader_register reg;
    struct vkd3d_shader_push_constant_buffer pc;
    unsigned int size;
};

struct vkd3d_shader_phase
{
    uint32_t function_id;
    size_t function_location;
};

struct vkd3d_shader_spec_constant
{
    enum vkd3d_shader_parameter_name name;
    uint32_t id;
};

struct vkd3d_hull_shader_variables
{
    uint32_t tess_level_outer_id;
    uint32_t tess_level_inner_id;
    uint32_t patch_constants_id;
};

struct ssa_register_info
{
    enum vkd3d_data_type data_type;
    uint32_t id;
};

struct spirv_compiler
{
    struct vkd3d_spirv_builder spirv_builder;
    const struct vsir_program *program;

    struct vkd3d_shader_message_context *message_context;
    struct vkd3d_shader_location location;
    bool failed;

    bool strip_debug;
    bool ssbo_uavs;
    bool uav_read_without_format;
    SpvExecutionMode fragment_coordinate_origin;

    struct rb_tree symbol_table;
    uint32_t temp_id;
    unsigned int temp_count;
    struct vkd3d_hull_shader_variables hs;
    uint32_t sample_positions_id;

    enum vkd3d_shader_type shader_type;

    struct vkd3d_shader_interface_info shader_interface;
    struct vkd3d_shader_descriptor_offset_info offset_info;
    uint32_t descriptor_offsets_member_id;
    uint32_t push_constants_var_id;
    uint32_t *descriptor_offset_ids;
    struct vkd3d_push_constant_buffer_binding *push_constants;
    const struct vkd3d_shader_spirv_target_info *spirv_target_info;

    struct
    {
        uint32_t buffer_id;
    } *spirv_parameter_info;

    bool prolog_emitted;
    struct shader_signature input_signature;
    struct shader_signature output_signature;
    struct shader_signature patch_constant_signature;
    const struct vkd3d_shader_transform_feedback_info *xfb_info;
    struct vkd3d_shader_output_info
    {
        uint32_t id;
        enum vkd3d_shader_component_type component_type;
        uint32_t array_element_mask;
    } *output_info;
    uint32_t private_output_variable[MAX_REG_OUTPUT + 1]; /* 1 entry for oDepth */
    uint32_t private_output_variable_write_mask[MAX_REG_OUTPUT + 1]; /* 1 entry for oDepth */
    uint32_t epilogue_function_id;
    uint32_t discard_function_id;

    uint32_t binding_idx;

    const struct vkd3d_shader_scan_descriptor_info1 *scan_descriptor_info;
    unsigned int input_control_point_count;
    unsigned int output_control_point_count;

    bool use_vocp;
    bool use_invocation_interlock;
    bool emit_point_size;

    enum vkd3d_shader_opcode phase;
    bool emit_default_control_point_phase;
    struct vkd3d_shader_phase control_point_phase;
    struct vkd3d_shader_phase patch_constant_phase;

    uint32_t current_spec_constant_id;
    unsigned int spec_constant_count;
    struct vkd3d_shader_spec_constant *spec_constants;
    size_t spec_constants_size;
    enum vkd3d_shader_compile_option_formatting_flags formatting;
    enum vkd3d_shader_compile_option_feature_flags features;
    enum vkd3d_shader_api_version api_version;
    bool write_tess_geom_point_size;

    struct vkd3d_string_buffer_cache string_buffers;

    struct ssa_register_info *ssa_register_info;
    unsigned int ssa_register_count;

    uint64_t config_flags;

    uint32_t *block_label_ids;
    unsigned int block_count;
    const char **block_names;
    size_t block_name_count;
};

static bool is_in_default_phase(const struct spirv_compiler *compiler)
{
    return compiler->phase == VKD3DSIH_INVALID;
}

static bool is_in_control_point_phase(const struct spirv_compiler *compiler)
{
    return compiler->phase == VKD3DSIH_HS_CONTROL_POINT_PHASE;
}

static bool is_in_fork_or_join_phase(const struct spirv_compiler *compiler)
{
    return compiler->phase == VKD3DSIH_HS_FORK_PHASE || compiler->phase == VKD3DSIH_HS_JOIN_PHASE;
}

static void spirv_compiler_emit_initial_declarations(struct spirv_compiler *compiler);
static size_t spirv_compiler_get_current_function_location(struct spirv_compiler *compiler);
static void spirv_compiler_emit_main_prolog(struct spirv_compiler *compiler);
static void spirv_compiler_emit_io_declarations(struct spirv_compiler *compiler);

static const char *spirv_compiler_get_entry_point_name(const struct spirv_compiler *compiler)
{
    const struct vkd3d_shader_spirv_target_info *info = compiler->spirv_target_info;

    return info && info->entry_point ? info->entry_point : "main";
}

static void spirv_compiler_destroy(struct spirv_compiler *compiler)
{
    vkd3d_free(compiler->output_info);

    vkd3d_free(compiler->push_constants);
    vkd3d_free(compiler->descriptor_offset_ids);

    vkd3d_free(compiler->spirv_parameter_info);

    vkd3d_spirv_builder_free(&compiler->spirv_builder);

    rb_destroy(&compiler->symbol_table, vkd3d_symbol_free, NULL);

    vkd3d_free(compiler->spec_constants);

    vkd3d_string_buffer_cache_cleanup(&compiler->string_buffers);

    shader_signature_cleanup(&compiler->input_signature);
    shader_signature_cleanup(&compiler->output_signature);
    shader_signature_cleanup(&compiler->patch_constant_signature);

    vkd3d_free(compiler->ssa_register_info);
    vkd3d_free(compiler->block_label_ids);

    vkd3d_free(compiler);
}

static struct spirv_compiler *spirv_compiler_create(const struct vsir_program *program,
        const struct vkd3d_shader_compile_info *compile_info,
        const struct vkd3d_shader_scan_descriptor_info1 *scan_descriptor_info,
        struct vkd3d_shader_message_context *message_context, uint64_t config_flags)
{
    const struct vkd3d_shader_interface_info *shader_interface;
    const struct vkd3d_shader_descriptor_offset_info *offset_info;
    const struct vkd3d_shader_spirv_target_info *target_info;
    struct spirv_compiler *compiler;
    unsigned int i;

    if (!(compiler = vkd3d_malloc(sizeof(*compiler))))
        return NULL;

    memset(compiler, 0, sizeof(*compiler));
    compiler->message_context = message_context;
    compiler->location.source_name = compile_info->source_name;
    compiler->config_flags = config_flags;

    if ((target_info = vkd3d_find_struct(compile_info->next, SPIRV_TARGET_INFO)))
    {
        switch (target_info->environment)
        {
            case VKD3D_SHADER_SPIRV_ENVIRONMENT_OPENGL_4_5:
            case VKD3D_SHADER_SPIRV_ENVIRONMENT_VULKAN_1_0:
            case VKD3D_SHADER_SPIRV_ENVIRONMENT_VULKAN_1_1:
                break;
            default:
                WARN("Invalid target environment %#x.\n", target_info->environment);
                vkd3d_free(compiler);
                return NULL;
        }

        compiler->spirv_target_info = target_info;
    }

    vkd3d_spirv_builder_init(&compiler->spirv_builder, spirv_compiler_get_entry_point_name(compiler));

    compiler->formatting = VKD3D_SHADER_COMPILE_OPTION_FORMATTING_INDENT
            | VKD3D_SHADER_COMPILE_OPTION_FORMATTING_HEADER;
    compiler->write_tess_geom_point_size = true;
    compiler->fragment_coordinate_origin = SpvExecutionModeOriginUpperLeft;

    for (i = 0; i < compile_info->option_count; ++i)
    {
        const struct vkd3d_shader_compile_option *option = &compile_info->options[i];

        switch (option->name)
        {
            case VKD3D_SHADER_COMPILE_OPTION_STRIP_DEBUG:
                compiler->strip_debug = !!option->value;
                break;

            case VKD3D_SHADER_COMPILE_OPTION_BUFFER_UAV:
                if (option->value == VKD3D_SHADER_COMPILE_OPTION_BUFFER_UAV_STORAGE_TEXEL_BUFFER)
                    compiler->ssbo_uavs = false;
                else if (option->value == VKD3D_SHADER_COMPILE_OPTION_BUFFER_UAV_STORAGE_BUFFER)
                    compiler->ssbo_uavs = true;
                else
                    WARN("Ignoring unrecognised value %#x for option %#x.\n", option->value, option->name);
                break;

            case VKD3D_SHADER_COMPILE_OPTION_FORMATTING:
                compiler->formatting = option->value;
                break;

            case VKD3D_SHADER_COMPILE_OPTION_API_VERSION:
                compiler->api_version = option->value;
                break;

            case VKD3D_SHADER_COMPILE_OPTION_TYPED_UAV:
                if (option->value == VKD3D_SHADER_COMPILE_OPTION_TYPED_UAV_READ_FORMAT_R32)
                    compiler->uav_read_without_format = false;
                else if (option->value == VKD3D_SHADER_COMPILE_OPTION_TYPED_UAV_READ_FORMAT_UNKNOWN)
                    compiler->uav_read_without_format = true;
                else
                    WARN("Ignoring unrecognised value %#x for option %#x.\n", option->value, option->name);
                break;

            case VKD3D_SHADER_COMPILE_OPTION_WRITE_TESS_GEOM_POINT_SIZE:
                compiler->write_tess_geom_point_size = option->value;
                break;

            case VKD3D_SHADER_COMPILE_OPTION_FRAGMENT_COORDINATE_ORIGIN:
                if (option->value == VKD3D_SHADER_COMPILE_OPTION_FRAGMENT_COORDINATE_ORIGIN_UPPER_LEFT)
                    compiler->fragment_coordinate_origin = SpvExecutionModeOriginUpperLeft;
                else if (option->value == VKD3D_SHADER_COMPILE_OPTION_FRAGMENT_COORDINATE_ORIGIN_LOWER_LEFT)
                    compiler->fragment_coordinate_origin = SpvExecutionModeOriginLowerLeft;
                else
                    WARN("Ignoring unrecognised value %#x for option %#x.\n", option->value, option->name);
                break;

            case VKD3D_SHADER_COMPILE_OPTION_FEATURE:
                compiler->features = option->value;
                break;

            default:
                WARN("Ignoring unrecognised option %#x with value %#x.\n", option->name, option->value);
                break;
        }
    }

    /* Explicit enabling of float64 was not required for API versions <= 1.10. */
    if (compiler->api_version <= VKD3D_SHADER_API_VERSION_1_10)
        compiler->features |= VKD3D_SHADER_COMPILE_OPTION_FEATURE_FLOAT64;

    rb_init(&compiler->symbol_table, vkd3d_symbol_compare);

    compiler->shader_type = program->shader_version.type;

    if ((shader_interface = vkd3d_find_struct(compile_info->next, INTERFACE_INFO)))
    {
        compiler->xfb_info = vkd3d_find_struct(compile_info->next, TRANSFORM_FEEDBACK_INFO);

        compiler->shader_interface = *shader_interface;
        if (shader_interface->push_constant_buffer_count)
        {
            if (!(compiler->push_constants = vkd3d_calloc(shader_interface->push_constant_buffer_count,
                    sizeof(*compiler->push_constants))))
            {
                spirv_compiler_destroy(compiler);
                return NULL;
            }
            for (i = 0; i < shader_interface->push_constant_buffer_count; ++i)
                compiler->push_constants[i].pc = shader_interface->push_constant_buffers[i];
        }

        if ((offset_info = vkd3d_find_struct(shader_interface->next, DESCRIPTOR_OFFSET_INFO)))
        {
            compiler->offset_info = *offset_info;
            if (compiler->offset_info.descriptor_table_count && !(compiler->descriptor_offset_ids = vkd3d_calloc(
                    compiler->offset_info.descriptor_table_count, sizeof(*compiler->descriptor_offset_ids))))
            {
                spirv_compiler_destroy(compiler);
                return NULL;
            }
        }
    }

    if (compiler->shader_type == VKD3D_SHADER_TYPE_VERTEX)
        compiler->emit_point_size = true;
    else if (compiler->shader_type != VKD3D_SHADER_TYPE_GEOMETRY)
        compiler->emit_point_size = compiler->xfb_info && compiler->xfb_info->element_count;

    compiler->scan_descriptor_info = scan_descriptor_info;

    compiler->phase = VKD3DSIH_INVALID;

    vkd3d_string_buffer_cache_init(&compiler->string_buffers);

    spirv_compiler_emit_initial_declarations(compiler);

    return compiler;
}

static bool spirv_compiler_use_storage_buffer(const struct spirv_compiler *compiler,
        const struct vkd3d_symbol_resource_data *resource)
{
    return compiler->ssbo_uavs && resource->resource_type_info->resource_type == VKD3D_SHADER_RESOURCE_BUFFER;
}

static enum vkd3d_shader_spirv_environment spirv_compiler_get_target_environment(
        const struct spirv_compiler *compiler)
{
    const struct vkd3d_shader_spirv_target_info *info = compiler->spirv_target_info;

    return info ? info->environment : VKD3D_SHADER_SPIRV_ENVIRONMENT_VULKAN_1_0;
}

static bool spirv_compiler_is_opengl_target(const struct spirv_compiler *compiler)
{
    return spirv_compiler_get_target_environment(compiler) == VKD3D_SHADER_SPIRV_ENVIRONMENT_OPENGL_4_5;
}

static bool spirv_compiler_is_spirv_min_1_3_target(const struct spirv_compiler *compiler)
{
    return spirv_compiler_get_target_environment(compiler) == VKD3D_SHADER_SPIRV_ENVIRONMENT_VULKAN_1_1;
}

static bool spirv_compiler_is_target_extension_supported(const struct spirv_compiler *compiler,
        enum vkd3d_shader_spirv_extension extension)
{
    const struct vkd3d_shader_spirv_target_info *info = compiler->spirv_target_info;
    unsigned int i;

    for (i = 0; info && i < info->extension_count; ++i)
    {
        if (info->extensions[i] == extension)
            return true;
    }

    return false;
}

static bool spirv_compiler_check_shader_visibility(const struct spirv_compiler *compiler,
        enum vkd3d_shader_visibility visibility)
{
    switch (visibility)
    {
        case VKD3D_SHADER_VISIBILITY_ALL:
            return true;
        case VKD3D_SHADER_VISIBILITY_VERTEX:
            return compiler->shader_type == VKD3D_SHADER_TYPE_VERTEX;
        case VKD3D_SHADER_VISIBILITY_HULL:
            return compiler->shader_type == VKD3D_SHADER_TYPE_HULL;
        case VKD3D_SHADER_VISIBILITY_DOMAIN:
            return compiler->shader_type == VKD3D_SHADER_TYPE_DOMAIN;
        case VKD3D_SHADER_VISIBILITY_GEOMETRY:
            return compiler->shader_type == VKD3D_SHADER_TYPE_GEOMETRY;
        case VKD3D_SHADER_VISIBILITY_PIXEL:
            return compiler->shader_type == VKD3D_SHADER_TYPE_PIXEL;
        case VKD3D_SHADER_VISIBILITY_COMPUTE:
            return compiler->shader_type == VKD3D_SHADER_TYPE_COMPUTE;
        default:
            ERR("Invalid shader visibility %#x.\n", visibility);
            return false;
    }
}

static struct vkd3d_push_constant_buffer_binding *spirv_compiler_find_push_constant_buffer(
        const struct spirv_compiler *compiler, const struct vkd3d_shader_register_range *range)
{
    unsigned int register_space = range->space;
    unsigned int reg_idx = range->first;
    unsigned int i;

    if (range->first != range->last)
        return NULL;

    for (i = 0; i < compiler->shader_interface.push_constant_buffer_count; ++i)
    {
        struct vkd3d_push_constant_buffer_binding *current = &compiler->push_constants[i];

        if (!spirv_compiler_check_shader_visibility(compiler, current->pc.shader_visibility))
            continue;

        if (current->pc.register_space == register_space && current->pc.register_index == reg_idx)
            return current;
    }

    return NULL;
}

static bool spirv_compiler_has_combined_sampler_for_resource(const struct spirv_compiler *compiler,
        const struct vkd3d_shader_register_range *range)
{
    const struct vkd3d_shader_interface_info *shader_interface = &compiler->shader_interface;
    const struct vkd3d_shader_combined_resource_sampler *combined_sampler;
    unsigned int i;

    if (!shader_interface->combined_sampler_count)
        return false;

    if (range->last != range->first)
        return false;

    for (i = 0; i < shader_interface->combined_sampler_count; ++i)
    {
        combined_sampler = &shader_interface->combined_samplers[i];

        if (!spirv_compiler_check_shader_visibility(compiler, combined_sampler->shader_visibility))
            continue;

        if ((combined_sampler->resource_space == range->space
                && combined_sampler->resource_index == range->first))
            return true;
    }

    return false;
}

static bool spirv_compiler_has_combined_sampler_for_sampler(const struct spirv_compiler *compiler,
        const struct vkd3d_shader_register_range *range)
{
    const struct vkd3d_shader_interface_info *shader_interface = &compiler->shader_interface;
    const struct vkd3d_shader_combined_resource_sampler *combined_sampler;
    unsigned int i;

    if (!shader_interface->combined_sampler_count)
        return false;

    if (range->last != range->first)
        return false;

    for (i = 0; i < shader_interface->combined_sampler_count; ++i)
    {
        combined_sampler = &shader_interface->combined_samplers[i];

        if (!spirv_compiler_check_shader_visibility(compiler, combined_sampler->shader_visibility))
            continue;

        if (combined_sampler->sampler_space == range->space
                && combined_sampler->sampler_index == range->first)
            return true;
    }

    return false;
}

static void VKD3D_PRINTF_FUNC(3, 4) spirv_compiler_error(struct spirv_compiler *compiler,
        enum vkd3d_shader_error error, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vkd3d_shader_verror(compiler->message_context, &compiler->location, error, format, args);
    va_end(args);
    compiler->failed = true;
}

static void VKD3D_PRINTF_FUNC(3, 4) spirv_compiler_warning(struct spirv_compiler *compiler,
        enum vkd3d_shader_error error, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vkd3d_shader_vwarning(compiler->message_context, &compiler->location, error, format, args);
    va_end(args);
}

static struct vkd3d_string_buffer *vkd3d_shader_register_range_string(struct spirv_compiler *compiler,
        const struct vkd3d_shader_register_range *range)
{
    struct vkd3d_string_buffer *buffer = vkd3d_string_buffer_get(&compiler->string_buffers);

    if (!buffer)
        return NULL;

    if (range->last != ~0u)
        vkd3d_string_buffer_printf(buffer, "[%u:%u]", range->first, range->last);
    else
        vkd3d_string_buffer_printf(buffer, "[%u:*]", range->first);

    return buffer;
}

static uint32_t spirv_compiler_get_label_id(struct spirv_compiler *compiler, unsigned int block_id)
{
    --block_id;
    if (!compiler->block_label_ids[block_id])
        compiler->block_label_ids[block_id] = vkd3d_spirv_alloc_id(&compiler->spirv_builder);
    return compiler->block_label_ids[block_id];
}

static struct vkd3d_shader_descriptor_binding spirv_compiler_get_descriptor_binding(
        struct spirv_compiler *compiler, const struct vkd3d_shader_register *reg,
        const struct vkd3d_shader_register_range *range, enum vkd3d_shader_resource_type resource_type,
        bool is_uav_counter, struct vkd3d_descriptor_binding_address *binding_address)
{
    const struct vkd3d_shader_interface_info *shader_interface = &compiler->shader_interface;
    unsigned int register_last = (range->last == ~0u) ? range->first : range->last;
    const struct vkd3d_shader_descriptor_offset *binding_offsets;
    enum vkd3d_shader_descriptor_type descriptor_type;
    enum vkd3d_shader_binding_flag resource_type_flag;
    struct vkd3d_shader_descriptor_binding binding;
    unsigned int i;

    if (reg->type == VKD3DSPR_CONSTBUFFER)
        descriptor_type = VKD3D_SHADER_DESCRIPTOR_TYPE_CBV;
    else if (reg->type == VKD3DSPR_RESOURCE)
        descriptor_type = VKD3D_SHADER_DESCRIPTOR_TYPE_SRV;
    else if (reg->type == VKD3DSPR_UAV)
        descriptor_type = VKD3D_SHADER_DESCRIPTOR_TYPE_UAV;
    else if (reg->type == VKD3DSPR_SAMPLER)
        descriptor_type = VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER;
    else
    {
        FIXME("Unhandled register type %#x.\n", reg->type);
        spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_INVALID_REGISTER_TYPE,
                "Encountered invalid/unhandled register type %#x.", reg->type);
        goto done;
    }

    resource_type_flag = resource_type == VKD3D_SHADER_RESOURCE_BUFFER
            ? VKD3D_SHADER_BINDING_FLAG_BUFFER : VKD3D_SHADER_BINDING_FLAG_IMAGE;

    if (is_uav_counter)
    {
        VKD3D_ASSERT(descriptor_type == VKD3D_SHADER_DESCRIPTOR_TYPE_UAV);
        binding_offsets = compiler->offset_info.uav_counter_offsets;
        for (i = 0; i < shader_interface->uav_counter_count; ++i)
        {
            const struct vkd3d_shader_uav_counter_binding *current = &shader_interface->uav_counters[i];

            if (!spirv_compiler_check_shader_visibility(compiler, current->shader_visibility))
                continue;

            if (current->register_space != range->space || current->register_index > range->first
                    || current->binding.count <= register_last - current->register_index)
                continue;

            if (current->offset)
            {
                FIXME("Atomic counter offsets are not supported yet.\n");
                spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_INVALID_DESCRIPTOR_BINDING,
                        "Descriptor binding for UAV counter %u, space %u has unsupported ‘offset’ %u.",
                        range->first, range->space, current->offset);
            }

            binding_address->binding_base_idx = current->register_index
                    - (binding_offsets ? binding_offsets[i].static_offset : 0);
            binding_address->push_constant_index = binding_offsets ? binding_offsets[i].dynamic_offset_index : ~0u;
            return current->binding;
        }
        if (shader_interface->uav_counter_count)
        {
            FIXME("Could not find descriptor binding for UAV counter %u, space %u.\n", range->first, range->space);
            spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_DESCRIPTOR_BINDING_NOT_FOUND,
                    "Could not find descriptor binding for UAV counter %u, space %u.", range->first, range->space);
        }
    }
    else
    {
        binding_offsets = compiler->offset_info.binding_offsets;
        for (i = 0; i < shader_interface->binding_count; ++i)
        {
            const struct vkd3d_shader_resource_binding *current = &shader_interface->bindings[i];

            if (!(current->flags & resource_type_flag))
                continue;

            if (!spirv_compiler_check_shader_visibility(compiler, current->shader_visibility))
                continue;

            if (current->type != descriptor_type || current->register_space != range->space
                    || current->register_index > range->first
                    || current->binding.count <= register_last - current->register_index)
                continue;

            binding_address->binding_base_idx = current->register_index
                    - (binding_offsets ? binding_offsets[i].static_offset : 0);
            binding_address->push_constant_index = binding_offsets ? binding_offsets[i].dynamic_offset_index : ~0u;
            return current->binding;
        }
        if (shader_interface->binding_count)
        {
            struct vkd3d_string_buffer *buffer = vkd3d_shader_register_range_string(compiler, range);
            const char *range_str = buffer ? buffer->buffer : "";
            FIXME("Could not find descriptor binding for type %#x, space %u, registers %s, shader type %#x.\n",
                    descriptor_type, range->space, range_str, compiler->shader_type);
            spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_DESCRIPTOR_BINDING_NOT_FOUND,
                    "Could not find descriptor binding for type %#x, space %u, registers %s, shader type %#x.",
                    descriptor_type, range->space, range_str, compiler->shader_type);
            vkd3d_string_buffer_release(&compiler->string_buffers, buffer);
        }
    }

done:
    binding_address->binding_base_idx = range->first;
    binding_address->push_constant_index = ~0u;
    binding.set = 0;
    binding.count = 1;
    binding.binding = compiler->binding_idx++;
    return binding;
}

static void spirv_compiler_emit_descriptor_binding(struct spirv_compiler *compiler,
        uint32_t variable_id, const struct vkd3d_shader_descriptor_binding *binding)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;

    vkd3d_spirv_build_op_decorate1(builder, variable_id, SpvDecorationDescriptorSet, binding->set);
    vkd3d_spirv_build_op_decorate1(builder, variable_id, SpvDecorationBinding, binding->binding);
}

static void spirv_compiler_decorate_nonuniform(struct spirv_compiler *compiler,
        uint32_t expression_id)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;

    vkd3d_spirv_enable_capability(builder, SpvCapabilityShaderNonUniformEXT);
    vkd3d_spirv_build_op_decorate(builder, expression_id, SpvDecorationNonUniformEXT, NULL, 0);
}

static const struct vkd3d_symbol *spirv_compiler_put_symbol(struct spirv_compiler *compiler,
        const struct vkd3d_symbol *symbol)
{
    struct vkd3d_symbol *s;

    s = vkd3d_symbol_dup(symbol);
    if (rb_put(&compiler->symbol_table, s, &s->entry) == -1)
    {
        ERR("Failed to insert symbol entry (%s).\n", debug_vkd3d_symbol(symbol));
        vkd3d_free(s);
        return NULL;
    }
    return s;
}

static uint32_t spirv_compiler_get_constant(struct spirv_compiler *compiler,
        enum vkd3d_shader_component_type component_type, unsigned int component_count, const uint32_t *values)
{
    uint32_t type_id, scalar_type_id, component_ids[VKD3D_VEC4_SIZE];
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    unsigned int i;

    VKD3D_ASSERT(0 < component_count && component_count <= VKD3D_VEC4_SIZE);
    type_id = vkd3d_spirv_get_type_id(builder, component_type, component_count);

    switch (component_type)
    {
        case VKD3D_SHADER_COMPONENT_UINT:
        case VKD3D_SHADER_COMPONENT_INT:
        case VKD3D_SHADER_COMPONENT_FLOAT:
            break;
        case VKD3D_SHADER_COMPONENT_BOOL:
            if (component_count == 1)
                return vkd3d_spirv_get_op_constant_bool(builder, type_id, *values);
            FIXME("Unsupported vector of bool.\n");
            spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_INVALID_TYPE,
                    "Vectors of bool type are not supported.");
            return vkd3d_spirv_get_op_undef(builder, type_id);
        default:
            FIXME("Unhandled component_type %#x.\n", component_type);
            return vkd3d_spirv_get_op_undef(builder, type_id);
    }

    if (component_count == 1)
    {
        return vkd3d_spirv_get_op_constant(builder, type_id, *values);
    }
    else
    {
        scalar_type_id = vkd3d_spirv_get_type_id(builder, component_type, 1);
        for (i = 0; i < component_count; ++i)
            component_ids[i] = vkd3d_spirv_get_op_constant(builder, scalar_type_id, values[i]);
        return vkd3d_spirv_get_op_constant_composite(builder, type_id, component_ids, component_count);
    }
}

static uint32_t spirv_compiler_get_constant64(struct spirv_compiler *compiler,
        enum vkd3d_shader_component_type component_type, unsigned int component_count, const uint64_t *values)
{
    uint32_t type_id, scalar_type_id, component_ids[VKD3D_DVEC2_SIZE];
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    unsigned int i;

    VKD3D_ASSERT(0 < component_count && component_count <= VKD3D_DVEC2_SIZE);
    type_id = vkd3d_spirv_get_type_id(builder, component_type, component_count);

    if (component_type != VKD3D_SHADER_COMPONENT_DOUBLE && component_type != VKD3D_SHADER_COMPONENT_UINT64)
    {
        FIXME("Unhandled component_type %#x.\n", component_type);
        return vkd3d_spirv_get_op_undef(builder, type_id);
    }

    if (component_count == 1)
    {
        return vkd3d_spirv_get_op_constant64(builder, type_id, *values);
    }
    else
    {
        scalar_type_id = vkd3d_spirv_get_type_id(builder, component_type, 1);
        for (i = 0; i < component_count; ++i)
            component_ids[i] = vkd3d_spirv_get_op_constant64(builder, scalar_type_id, values[i]);
        return vkd3d_spirv_get_op_constant_composite(builder, type_id, component_ids, component_count);
    }
}

static uint32_t spirv_compiler_get_constant_uint(struct spirv_compiler *compiler,
        uint32_t value)
{
    return spirv_compiler_get_constant(compiler, VKD3D_SHADER_COMPONENT_UINT, 1, &value);
}

static uint32_t spirv_compiler_get_constant_float(struct spirv_compiler *compiler,
        float value)
{
    return spirv_compiler_get_constant(compiler, VKD3D_SHADER_COMPONENT_FLOAT, 1, (uint32_t *)&value);
}

static uint32_t spirv_compiler_get_constant_vector(struct spirv_compiler *compiler,
        enum vkd3d_shader_component_type component_type, unsigned int component_count, uint32_t value)
{
    const uint32_t values[] = {value, value, value, value};
    return spirv_compiler_get_constant(compiler, component_type, component_count, values);
}

static uint32_t spirv_compiler_get_constant_uint_vector(struct spirv_compiler *compiler,
        uint32_t value, unsigned int component_count)
{
    return spirv_compiler_get_constant_vector(compiler, VKD3D_SHADER_COMPONENT_UINT, component_count, value);
}

static uint32_t spirv_compiler_get_constant_float_vector(struct spirv_compiler *compiler,
        float value, unsigned int component_count)
{
    const float values[] = {value, value, value, value};
    return spirv_compiler_get_constant(compiler, VKD3D_SHADER_COMPONENT_FLOAT,
            component_count, (const uint32_t *)values);
}

static uint32_t spirv_compiler_get_constant_double_vector(struct spirv_compiler *compiler,
        double value, unsigned int component_count)
{
    const double values[] = {value, value};
    return spirv_compiler_get_constant64(compiler, VKD3D_SHADER_COMPONENT_DOUBLE,
            component_count, (const uint64_t *)values);
}

static uint32_t spirv_compiler_get_constant_uint64_vector(struct spirv_compiler *compiler,
        uint64_t value, unsigned int component_count)
{
    const uint64_t values[] = {value, value};
    return spirv_compiler_get_constant64(compiler, VKD3D_SHADER_COMPONENT_UINT64, component_count, values);
}

static uint32_t spirv_compiler_get_type_id_for_reg(struct spirv_compiler *compiler,
        const struct vkd3d_shader_register *reg, uint32_t write_mask)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;

    return vkd3d_spirv_get_type_id(builder,
            vkd3d_component_type_from_data_type(reg->data_type),
            vsir_write_mask_component_count(write_mask));
}

static uint32_t spirv_compiler_get_type_id_for_dst(struct spirv_compiler *compiler,
        const struct vkd3d_shader_dst_param *dst)
{
    return spirv_compiler_get_type_id_for_reg(compiler, &dst->reg, dst->write_mask);
}

static bool spirv_compiler_get_register_name(char *buffer, unsigned int buffer_size,
        const struct vkd3d_shader_register *reg)
{
    unsigned int idx;

    idx = reg->idx_count ? reg->idx[reg->idx_count - 1].offset : 0;
    switch (reg->type)
    {
        case VKD3DSPR_RESOURCE:
            snprintf(buffer, buffer_size, "t%u", reg->idx[0].offset);
            break;
        case VKD3DSPR_UAV:
            snprintf(buffer, buffer_size, "u%u", reg->idx[0].offset);
            break;
        case VKD3DSPR_SAMPLER:
            snprintf(buffer, buffer_size, "s%u", reg->idx[0].offset);
            break;
        case VKD3DSPR_CONSTBUFFER:
            snprintf(buffer, buffer_size, "cb%u_%u", reg->idx[0].offset, reg->idx[1].offset);
            break;
        case VKD3DSPR_RASTOUT:
            if (idx == VSIR_RASTOUT_POINT_SIZE)
            {
                snprintf(buffer, buffer_size, "oPts");
                break;
            }
            FIXME("Unhandled rastout register %#x.\n", idx);
            return false;
        case VKD3DSPR_INPUT:
            snprintf(buffer, buffer_size, "v%u", idx);
            break;
        case VKD3DSPR_OUTPUT:
            snprintf(buffer, buffer_size, "o%u", idx);
            break;
        case VKD3DSPR_COLOROUT:
            snprintf(buffer, buffer_size, "oC%u", idx);
            break;
        case VKD3DSPR_DEPTHOUT:
        case VKD3DSPR_DEPTHOUTGE:
        case VKD3DSPR_DEPTHOUTLE:
            snprintf(buffer, buffer_size, "oDepth");
            break;
        case VKD3DSPR_GSINSTID:
            snprintf(buffer, buffer_size, "vGSInstanceID");
            break;
        case VKD3DSPR_PATCHCONST:
            snprintf(buffer, buffer_size, "vpc%u", idx);
            break;
        case VKD3DSPR_TESSCOORD:
            snprintf(buffer, buffer_size, "vDomainLocation");
            break;
        case VKD3DSPR_THREADID:
            snprintf(buffer, buffer_size, "vThreadID");
            break;
        case VKD3DSPR_LOCALTHREADID:
            snprintf(buffer, buffer_size, "vThreadIDInGroup");
            break;
        case VKD3DSPR_LOCALTHREADINDEX:
            snprintf(buffer, buffer_size, "vThreadIDInGroupFlattened");
            break;
        case VKD3DSPR_THREADGROUPID:
            snprintf(buffer, buffer_size, "vThreadGroupID");
            break;
        case VKD3DSPR_GROUPSHAREDMEM:
            snprintf(buffer, buffer_size, "g%u", reg->idx[0].offset);
            break;
        case VKD3DSPR_IDXTEMP:
            snprintf(buffer, buffer_size, "x%u", idx);
            break;
        case VKD3DSPR_COVERAGE:
            snprintf(buffer, buffer_size, "vCoverage");
            break;
        case VKD3DSPR_SAMPLEMASK:
            snprintf(buffer, buffer_size, "oMask");
            break;
        case VKD3DSPR_OUTPOINTID:
        case VKD3DSPR_PRIMID:
            /* SPIRV-Tools disassembler generates names for SPIR-V built-ins. */
            return false;
        case VKD3DSPR_OUTSTENCILREF:
            snprintf(buffer, buffer_size, "oStencilRef");
            break;
        case VKD3DSPR_WAVELANECOUNT:
            snprintf(buffer, buffer_size, "vWaveLaneCount");
            break;
        case VKD3DSPR_WAVELANEINDEX:
            snprintf(buffer, buffer_size, "vWaveLaneIndex");
            break;
        case VKD3DSPR_POINT_COORD:
            snprintf(buffer, buffer_size, "vPointCoord");
            break;
        default:
            FIXME("Unhandled register %#x.\n", reg->type);
            snprintf(buffer, buffer_size, "unrecognized_%#x", reg->type);
            return false;
    }

    return true;
}

/* TODO: UAV counters: vkd3d_spirv_build_op_name(builder, counter_var_id, "u%u_counter", reg->idx[0].offset); */
static void spirv_compiler_emit_register_debug_name(struct vkd3d_spirv_builder *builder,
        uint32_t id, const struct vkd3d_shader_register *reg)
{
    char debug_name[256];
    if (spirv_compiler_get_register_name(debug_name, ARRAY_SIZE(debug_name), reg))
        vkd3d_spirv_build_op_name(builder, id, "%s", debug_name);
}

static uint32_t spirv_compiler_emit_array_variable(struct spirv_compiler *compiler,
        struct vkd3d_spirv_stream *stream, SpvStorageClass storage_class,
        enum vkd3d_shader_component_type component_type, unsigned int component_count,
        const unsigned int *array_lengths, unsigned int length_count)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t type_id, length_id, ptr_type_id;
    unsigned int i;

    type_id = vkd3d_spirv_get_type_id(builder, component_type, component_count);
    for (i = 0; i < length_count; ++i)
    {
        if (!array_lengths[i])
            continue;
        length_id = spirv_compiler_get_constant_uint(compiler, array_lengths[i]);
        type_id = vkd3d_spirv_get_op_type_array(builder, type_id, length_id);
    }

    ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, storage_class, type_id);
    return vkd3d_spirv_build_op_variable(builder, stream, ptr_type_id, storage_class, 0);
}

static uint32_t spirv_compiler_emit_variable(struct spirv_compiler *compiler,
        struct vkd3d_spirv_stream *stream, SpvStorageClass storage_class,
        enum vkd3d_shader_component_type component_type, unsigned int component_count)
{
    return spirv_compiler_emit_array_variable(compiler, stream, storage_class,
            component_type, component_count, NULL, 0);
}

static const struct vkd3d_spec_constant_info
{
    enum vkd3d_shader_parameter_name name;
    uint32_t default_value;
    const char *debug_name;
}
vkd3d_shader_parameters[] =
{
    {VKD3D_SHADER_PARAMETER_NAME_RASTERIZER_SAMPLE_COUNT, 1, "sample_count"},
    {VKD3D_SHADER_PARAMETER_NAME_ALPHA_TEST_REF, 0, "alpha_test_ref"},
};

static const struct vkd3d_spec_constant_info *get_spec_constant_info(enum vkd3d_shader_parameter_name name)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(vkd3d_shader_parameters); ++i)
    {
        if (vkd3d_shader_parameters[i].name == name)
            return &vkd3d_shader_parameters[i];
    }

    FIXME("Unhandled parameter name %#x.\n", name);
    return NULL;
}

static uint32_t spirv_compiler_alloc_spec_constant_id(struct spirv_compiler *compiler, unsigned int count)
{
    uint32_t ret;

    if (!compiler->current_spec_constant_id)
    {
        unsigned int i, id = 0;

        for (i = 0; i < compiler->program->parameter_count; ++i)
        {
            const struct vkd3d_shader_parameter1 *current = &compiler->program->parameters[i];

            if (current->type == VKD3D_SHADER_PARAMETER_TYPE_SPECIALIZATION_CONSTANT)
            {
                switch (current->data_type)
                {
                    case VKD3D_SHADER_PARAMETER_DATA_TYPE_FLOAT32_VEC4:
                        id = max(current->u.specialization_constant.id + 4, id);
                        break;

                    default:
                        id = max(current->u.specialization_constant.id + 1, id);
                        break;
                }
            }
        }

        compiler->current_spec_constant_id = id;
    }

    ret = compiler->current_spec_constant_id;
    compiler->current_spec_constant_id += count;
    return ret;
}

static uint32_t spirv_compiler_emit_spec_constant(struct spirv_compiler *compiler,
        enum vkd3d_shader_parameter_name name, uint32_t spec_id,
        enum vkd3d_data_type type, unsigned int component_count)
{
    uint32_t scalar_type_id, vector_type_id, id, default_value, components[4];
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_spec_constant_info *info;

    info = get_spec_constant_info(name);
    default_value = info ? info->default_value : 0;

    scalar_type_id = vkd3d_spirv_get_type_id(builder, vkd3d_component_type_from_data_type(type), 1);
    vector_type_id = vkd3d_spirv_get_type_id(builder, vkd3d_component_type_from_data_type(type), component_count);

    for (unsigned int i = 0; i < component_count; ++i)
    {
        components[i] = vkd3d_spirv_build_op_spec_constant(builder, scalar_type_id, default_value);
        vkd3d_spirv_build_op_decorate1(builder, components[i], SpvDecorationSpecId, spec_id + i);
    }

    if (component_count == 1)
        id = components[0];
    else
        id = vkd3d_spirv_build_op_spec_constant_composite(builder, vector_type_id, components, component_count);

    if (info)
        vkd3d_spirv_build_op_name(builder, id, "%s", info->debug_name);

    if (vkd3d_array_reserve((void **)&compiler->spec_constants, &compiler->spec_constants_size,
            compiler->spec_constant_count + 1, sizeof(*compiler->spec_constants)))
    {
        struct vkd3d_shader_spec_constant *constant = &compiler->spec_constants[compiler->spec_constant_count++];
        constant->name = name;
        constant->id = id;
    }

    return id;
}

static uint32_t spirv_compiler_get_spec_constant(struct spirv_compiler *compiler,
        enum vkd3d_shader_parameter_name name, uint32_t spec_id,
        enum vkd3d_data_type type, unsigned int component_count)
{
    unsigned int i;

    for (i = 0; i < compiler->spec_constant_count; ++i)
    {
        if (compiler->spec_constants[i].name == name)
            return compiler->spec_constants[i].id;
    }

    return spirv_compiler_emit_spec_constant(compiler, name, spec_id, type, component_count);
}

static uint32_t spirv_compiler_get_buffer_parameter(struct spirv_compiler *compiler,
        const struct vkd3d_shader_parameter1 *parameter, enum vkd3d_data_type type, unsigned int component_count)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    unsigned int index = parameter - compiler->program->parameters;
    uint32_t type_id, ptr_id, ptr_type_id;

    type_id = vkd3d_spirv_get_type_id(builder, vkd3d_component_type_from_data_type(type), component_count);
    ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, SpvStorageClassUniform, type_id);
    ptr_id = vkd3d_spirv_build_op_access_chain1(builder, ptr_type_id,
            compiler->spirv_parameter_info[index].buffer_id,
            spirv_compiler_get_constant_uint(compiler, 0));
    return vkd3d_spirv_build_op_load(builder, type_id, ptr_id, SpvMemoryAccessMaskNone);
}

static const struct
{
    enum vkd3d_data_type type;
    unsigned int component_count;
}
parameter_data_type_map[] =
{
    [VKD3D_SHADER_PARAMETER_DATA_TYPE_FLOAT32]      = {VKD3D_DATA_FLOAT, 1},
    [VKD3D_SHADER_PARAMETER_DATA_TYPE_UINT32]       = {VKD3D_DATA_UINT,  1},
    [VKD3D_SHADER_PARAMETER_DATA_TYPE_FLOAT32_VEC4] = {VKD3D_DATA_FLOAT, 4},
};

static uint32_t spirv_compiler_emit_shader_parameter(struct spirv_compiler *compiler,
        enum vkd3d_shader_parameter_name name, enum vkd3d_data_type type, unsigned int component_count)
{
    const struct vkd3d_shader_parameter1 *parameter;

    if (!(parameter = vsir_program_get_parameter(compiler->program, name)))
    {
        WARN("Unresolved shader parameter %#x.\n", name);
        goto default_parameter;
    }

    if (parameter_data_type_map[parameter->data_type].type != type
            || parameter_data_type_map[parameter->data_type].component_count != component_count)
        ERR("Expected type %#x, count %u for parameter %#x, got %#x.\n",
                type, component_count, name, parameter->data_type);

    if (parameter->type == VKD3D_SHADER_PARAMETER_TYPE_IMMEDIATE_CONSTANT)
        return spirv_compiler_get_constant(compiler, vkd3d_component_type_from_data_type(type),
                component_count, (const uint32_t *)&parameter->u.immediate_constant);

    if (parameter->type == VKD3D_SHADER_PARAMETER_TYPE_SPECIALIZATION_CONSTANT)
        return spirv_compiler_get_spec_constant(compiler, name,
                parameter->u.specialization_constant.id, type, component_count);
    if (parameter->type == VKD3D_SHADER_PARAMETER_TYPE_BUFFER)
        return spirv_compiler_get_buffer_parameter(compiler, parameter, type, component_count);

    FIXME("Unhandled parameter type %#x.\n", parameter->type);

default_parameter:
    return spirv_compiler_get_spec_constant(compiler,
            name, spirv_compiler_alloc_spec_constant_id(compiler, component_count), type, component_count);
}

static uint32_t spirv_compiler_emit_construct_vector(struct spirv_compiler *compiler,
        enum vkd3d_shader_component_type component_type, unsigned int component_count,
        uint32_t val_id, unsigned int val_component_idx, unsigned int val_component_count)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t components[VKD3D_VEC4_SIZE];
    uint32_t type_id, result_id;
    unsigned int i;

    VKD3D_ASSERT(val_component_idx < val_component_count);

    type_id = vkd3d_spirv_get_type_id(builder, component_type, component_count);
    if (val_component_count == 1)
    {
        for (i = 0; i < component_count; ++i)
            components[i] = val_id;
        result_id = vkd3d_spirv_build_op_composite_construct(builder,
                type_id, components, component_count);
    }
    else
    {
        for (i = 0; i < component_count; ++i)
            components[i] = val_component_idx;
        result_id = vkd3d_spirv_build_op_vector_shuffle(builder,
                type_id, val_id, val_id, components, component_count);
    }
    return result_id;
}

static uint32_t spirv_compiler_emit_load_src(struct spirv_compiler *compiler,
        const struct vkd3d_shader_src_param *src, uint32_t write_mask);

static uint32_t spirv_compiler_emit_register_addressing(struct spirv_compiler *compiler,
        const struct vkd3d_shader_register_index *reg_index)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t type_id, addr_id;

    if (!reg_index->rel_addr)
        return spirv_compiler_get_constant_uint(compiler, reg_index->offset);

    addr_id = spirv_compiler_emit_load_src(compiler, reg_index->rel_addr, VKD3DSP_WRITEMASK_0);
    if (reg_index->offset)
    {
        type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, 1);
        addr_id = vkd3d_spirv_build_op_iadd(builder, type_id,
                addr_id, spirv_compiler_get_constant_uint(compiler, reg_index->offset));
    }
    return addr_id;
}

struct vkd3d_shader_register_info
{
    uint32_t id;
    const struct vkd3d_symbol *descriptor_array;
    SpvStorageClass storage_class;
    enum vkd3d_shader_component_type component_type;
    unsigned int write_mask;
    uint32_t member_idx;
    unsigned int structure_stride;
    unsigned int binding_base_idx;
    bool is_aggregate;
};

static bool spirv_compiler_get_register_info(struct spirv_compiler *compiler,
        const struct vkd3d_shader_register *reg, struct vkd3d_shader_register_info *register_info)
{
    struct vkd3d_symbol reg_symbol, *symbol;
    struct rb_entry *entry;

    VKD3D_ASSERT(!register_is_constant_or_undef(reg));

    if (reg->type == VKD3DSPR_TEMP)
    {
        VKD3D_ASSERT(reg->idx[0].offset < compiler->temp_count);
        register_info->id = compiler->temp_id + reg->idx[0].offset;
        register_info->storage_class = SpvStorageClassPrivate;
        register_info->descriptor_array = NULL;
        register_info->member_idx = 0;
        register_info->component_type = VKD3D_SHADER_COMPONENT_FLOAT;
        register_info->write_mask = VKD3DSP_WRITEMASK_ALL;
        register_info->structure_stride = 0;
        register_info->binding_base_idx = 0;
        register_info->is_aggregate = false;
        return true;
    }

    vkd3d_symbol_make_register(&reg_symbol, reg);
    if (!(entry = rb_get(&compiler->symbol_table, &reg_symbol)))
    {
        spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_INVALID_REGISTER_TYPE,
                "Unrecognized register (%s).\n", debug_vkd3d_symbol(&reg_symbol));
        memset(register_info, 0, sizeof(*register_info));
        return false;
    }

    symbol = RB_ENTRY_VALUE(entry, struct vkd3d_symbol, entry);
    register_info->id = symbol->id;
    register_info->descriptor_array = symbol->descriptor_array;
    register_info->storage_class = symbol->info.reg.storage_class;
    register_info->member_idx = symbol->info.reg.member_idx;
    register_info->component_type = symbol->info.reg.component_type;
    register_info->write_mask = symbol->info.reg.write_mask;
    register_info->structure_stride = symbol->info.reg.structure_stride;
    register_info->binding_base_idx = symbol->info.reg.binding_base_idx;
    register_info->is_aggregate = symbol->info.reg.is_aggregate;

    return true;
}

static bool spirv_compiler_enable_descriptor_indexing(struct spirv_compiler *compiler,
        enum vkd3d_shader_register_type reg_type, enum vkd3d_shader_resource_type resource_type)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;

    if (!spirv_compiler_is_target_extension_supported(compiler,
            VKD3D_SHADER_SPIRV_EXTENSION_EXT_DESCRIPTOR_INDEXING))
        return false;

    switch (reg_type)
    {
        case VKD3DSPR_CONSTBUFFER:
            vkd3d_spirv_enable_capability(builder, SpvCapabilityUniformBufferArrayDynamicIndexing);
            break;
        case VKD3DSPR_RESOURCE:
            vkd3d_spirv_enable_capability(builder, resource_type == VKD3D_SHADER_RESOURCE_BUFFER
                    ? SpvCapabilityUniformTexelBufferArrayDynamicIndexingEXT
                    : SpvCapabilitySampledImageArrayDynamicIndexing);
            break;
        case VKD3DSPR_UAV:
            if (resource_type == VKD3D_SHADER_RESOURCE_BUFFER)
                vkd3d_spirv_enable_capability(builder, compiler->ssbo_uavs
                        ? SpvCapabilityStorageBufferArrayDynamicIndexing
                        : SpvCapabilityStorageTexelBufferArrayDynamicIndexingEXT);
            else
                vkd3d_spirv_enable_capability(builder, SpvCapabilityStorageImageArrayDynamicIndexing);
            break;
        case VKD3DSPR_SAMPLER:
            break;
        default:
            ERR("Unhandled register type %#x.\n", reg_type);
            break;
    }

    return true;
}

static uint32_t spirv_compiler_get_descriptor_index(struct spirv_compiler *compiler,
        const struct vkd3d_shader_register *reg, const struct vkd3d_symbol *array_symbol,
        unsigned int binding_base_idx, enum vkd3d_shader_resource_type resource_type)
{
    const struct vkd3d_symbol_descriptor_array *array_key = &array_symbol->key.descriptor_array;
    struct vkd3d_shader_register_index index = reg->idx[1];
    unsigned int push_constant_index;
    uint32_t index_id;

    if ((push_constant_index = array_key->push_constant_index) != ~0u || index.rel_addr)
    {
        if (!spirv_compiler_enable_descriptor_indexing(compiler, reg->type, resource_type))
        {
            FIXME("The target environment does not support descriptor indexing.\n");
            spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_DESCRIPTOR_IDX_UNSUPPORTED,
                    "Cannot dynamically index a descriptor array of type %#x, id %u. "
                    "The target environment does not support descriptor indexing.", reg->type, reg->idx[0].offset);
        }
    }

    index.offset -= binding_base_idx;
    index_id = spirv_compiler_emit_register_addressing(compiler, &index);

    if (push_constant_index != ~0u)
    {
        struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
        uint32_t type_id, ptr_type_id, ptr_id, offset_id, index_ids[2];

        type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, 1);
        if (!(offset_id = compiler->descriptor_offset_ids[push_constant_index]))
        {
            index_ids[0] = compiler->descriptor_offsets_member_id;
            index_ids[1] = spirv_compiler_get_constant_uint(compiler, push_constant_index);
            ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, SpvStorageClassPushConstant, type_id);
            vkd3d_spirv_begin_function_stream_insertion(builder,
                    spirv_compiler_get_current_function_location(compiler));
            ptr_id = vkd3d_spirv_build_op_in_bounds_access_chain(builder, ptr_type_id,
                    compiler->push_constants_var_id, index_ids, 2);
            offset_id = vkd3d_spirv_build_op_load(builder, type_id, ptr_id, SpvMemoryAccessMaskNone);
            vkd3d_spirv_end_function_stream_insertion(builder);
            compiler->descriptor_offset_ids[push_constant_index] = offset_id;
        }
        index_id = vkd3d_spirv_build_op_iadd(builder, type_id, index_id, offset_id);
    }

   return index_id;
}

static void spirv_compiler_emit_dereference_register(struct spirv_compiler *compiler,
        const struct vkd3d_shader_register *reg, struct vkd3d_shader_register_info *register_info)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    unsigned int component_count, index_count = 0;
    uint32_t type_id, ptr_type_id;
    uint32_t indexes[3];

    if (reg->type == VKD3DSPR_CONSTBUFFER)
    {
        VKD3D_ASSERT(!reg->idx[0].rel_addr);
        if (register_info->descriptor_array)
            indexes[index_count++] = spirv_compiler_get_descriptor_index(compiler, reg,
                    register_info->descriptor_array, register_info->binding_base_idx, VKD3D_SHADER_RESOURCE_BUFFER);
        indexes[index_count++] = spirv_compiler_get_constant_uint(compiler, register_info->member_idx);
        indexes[index_count++] = spirv_compiler_emit_register_addressing(compiler, &reg->idx[2]);
    }
    else if (reg->type == VKD3DSPR_IMMCONSTBUFFER)
    {
        indexes[index_count++] = spirv_compiler_emit_register_addressing(compiler, &reg->idx[reg->idx_count - 1]);
    }
    else if (reg->type == VKD3DSPR_IDXTEMP)
    {
        indexes[index_count++] = spirv_compiler_emit_register_addressing(compiler, &reg->idx[1]);
    }
    else if (register_info->is_aggregate)
    {
        /* Indices for these are swapped compared to the generated SPIR-V. */
        if (reg->idx_count > 2)
            indexes[index_count++] = spirv_compiler_emit_register_addressing(compiler, &reg->idx[1]);
        if (reg->idx_count > 1)
            indexes[index_count++] = spirv_compiler_emit_register_addressing(compiler, &reg->idx[0]);
        if (!index_count)
            /* A register sysval which is an array in SPIR-V, e.g. SAMPLEMASK. */
            indexes[index_count++] = spirv_compiler_get_constant_uint(compiler, 0);
    }
    else
    {
        if (reg->idx_count && reg->idx[reg->idx_count - 1].rel_addr)
            FIXME("Relative addressing not implemented.\n");

        /* Handle arrayed registers, e.g. v[3][0]. */
        if (reg->idx_count > 1 && !vsir_register_is_descriptor(reg))
            indexes[index_count++] = spirv_compiler_emit_register_addressing(compiler, &reg->idx[0]);
    }

    /* Alignment is supported only in the Kernel execution model and is an optimisation only. */
    if (reg->alignment)
        TRACE("Ignoring alignment %u.\n", reg->alignment);

    if (index_count)
    {
        component_count = vsir_write_mask_component_count(register_info->write_mask);
        type_id = vkd3d_spirv_get_type_id(builder, register_info->component_type, component_count);
        ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, register_info->storage_class, type_id);
        register_info->id = vkd3d_spirv_build_op_access_chain(builder, ptr_type_id,
                register_info->id, indexes, index_count);
        if (reg->non_uniform)
            spirv_compiler_decorate_nonuniform(compiler, register_info->id);
    }
}

static uint32_t spirv_compiler_get_register_id(struct spirv_compiler *compiler,
        const struct vkd3d_shader_register *reg)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    struct vkd3d_shader_register_info register_info;

    if (spirv_compiler_get_register_info(compiler, reg, &register_info))
    {
        spirv_compiler_emit_dereference_register(compiler, reg, &register_info);
        return register_info.id;
    }

    return spirv_compiler_emit_variable(compiler, &builder->global_stream,
            SpvStorageClassPrivate, VKD3D_SHADER_COMPONENT_FLOAT, VKD3D_VEC4_SIZE);
}

static bool vkd3d_swizzle_is_equal(uint32_t dst_write_mask, uint32_t swizzle, uint32_t write_mask)
{
    return vkd3d_compact_swizzle(VKD3D_SHADER_NO_SWIZZLE, dst_write_mask) == vkd3d_compact_swizzle(swizzle, write_mask);
}

static bool vkd3d_swizzle_is_scalar(uint32_t swizzle, const struct vkd3d_shader_register *reg)
{
    unsigned int component_idx = vsir_swizzle_get_component(swizzle, 0);

    if (vsir_swizzle_get_component(swizzle, 1) != component_idx)
        return false;

    if (data_type_is_64_bit(reg->data_type))
        return true;

    return vsir_swizzle_get_component(swizzle, 2) == component_idx
            && vsir_swizzle_get_component(swizzle, 3) == component_idx;
}

static uint32_t spirv_compiler_emit_swizzle(struct spirv_compiler *compiler,
        uint32_t val_id, uint32_t val_write_mask, enum vkd3d_shader_component_type component_type,
        uint32_t swizzle, uint32_t write_mask)
{
    unsigned int i, component_idx, component_count, val_component_count;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t type_id, components[VKD3D_VEC4_SIZE];

    component_count = vsir_write_mask_component_count(write_mask);
    val_component_count = vsir_write_mask_component_count(val_write_mask);

    if (component_count == val_component_count
            && (component_count == 1 || vkd3d_swizzle_is_equal(val_write_mask, swizzle, write_mask)))
        return val_id;

    type_id = vkd3d_spirv_get_type_id(builder, component_type, component_count);

    if (component_count == 1)
    {
        component_idx = vsir_write_mask_get_component_idx(write_mask);
        component_idx = vsir_swizzle_get_component(swizzle, component_idx);
        component_idx -= vsir_write_mask_get_component_idx(val_write_mask);
        return vkd3d_spirv_build_op_composite_extract1(builder, type_id, val_id, component_idx);
    }

    if (val_component_count == 1)
    {
        for (i = 0, component_idx = 0; i < VKD3D_VEC4_SIZE; ++i)
        {
            if (write_mask & (VKD3DSP_WRITEMASK_0 << i))
            {
                VKD3D_ASSERT(VKD3DSP_WRITEMASK_0 << vsir_swizzle_get_component(swizzle, i) == val_write_mask);
                components[component_idx++] = val_id;
            }
        }
        return vkd3d_spirv_build_op_composite_construct(builder, type_id, components, component_count);
    }

    for (i = 0, component_idx = 0; i < VKD3D_VEC4_SIZE; ++i)
    {
        if (write_mask & (VKD3DSP_WRITEMASK_0 << i))
            components[component_idx++] = vsir_swizzle_get_component(swizzle, i);
    }
    return vkd3d_spirv_build_op_vector_shuffle(builder,
            type_id, val_id, val_id, components, component_count);
}

static uint32_t spirv_compiler_emit_vector_shuffle(struct spirv_compiler *compiler,
        uint32_t vector1_id, uint32_t vector2_id, uint32_t swizzle, uint32_t write_mask,
        enum vkd3d_shader_component_type component_type, unsigned int component_count)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t components[VKD3D_VEC4_SIZE];
    uint32_t type_id;
    unsigned int i;

    VKD3D_ASSERT(component_count <= ARRAY_SIZE(components));

    for (i = 0; i < component_count; ++i)
    {
        if (write_mask & (VKD3DSP_WRITEMASK_0 << i))
            components[i] = vsir_swizzle_get_component(swizzle, i);
        else
            components[i] = VKD3D_VEC4_SIZE + vsir_swizzle_get_component(swizzle, i);
    }

    type_id = vkd3d_spirv_get_type_id(builder, component_type, component_count);
    return vkd3d_spirv_build_op_vector_shuffle(builder,
            type_id, vector1_id, vector2_id, components, component_count);
}

static uint32_t spirv_compiler_emit_int_to_bool(struct spirv_compiler *compiler,
        enum vkd3d_shader_conditional_op condition, enum vkd3d_data_type data_type,
        unsigned int component_count, uint32_t val_id)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t type_id;
    SpvOp op;

    VKD3D_ASSERT(!(condition & ~(VKD3D_SHADER_CONDITIONAL_OP_NZ | VKD3D_SHADER_CONDITIONAL_OP_Z)));

    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_BOOL, component_count);
    op = condition & VKD3D_SHADER_CONDITIONAL_OP_Z ? SpvOpIEqual : SpvOpINotEqual;
    return vkd3d_spirv_build_op_tr2(builder, &builder->function_stream, op, type_id, val_id,
            data_type == VKD3D_DATA_UINT64
            ? spirv_compiler_get_constant_uint64_vector(compiler, 0, component_count)
            : spirv_compiler_get_constant_uint_vector(compiler, 0, component_count));
}

static uint32_t spirv_compiler_emit_bool_to_int(struct spirv_compiler *compiler,
        unsigned int component_count, uint32_t val_id, bool signedness)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t type_id, true_id, false_id;

    true_id = spirv_compiler_get_constant_uint_vector(compiler, signedness ? 0xffffffff : 1, component_count);
    false_id = spirv_compiler_get_constant_uint_vector(compiler, 0, component_count);
    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, component_count);
    return vkd3d_spirv_build_op_select(builder, type_id, val_id, true_id, false_id);
}

static uint32_t spirv_compiler_emit_bool_to_int64(struct spirv_compiler *compiler,
        unsigned int component_count, uint32_t val_id, bool signedness)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t type_id, true_id, false_id;

    true_id = spirv_compiler_get_constant_uint64_vector(compiler, signedness ? UINT64_MAX : 1,
            component_count);
    false_id = spirv_compiler_get_constant_uint64_vector(compiler, 0, component_count);
    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT64, component_count);
    return vkd3d_spirv_build_op_select(builder, type_id, val_id, true_id, false_id);
}

static uint32_t spirv_compiler_emit_bool_to_float(struct spirv_compiler *compiler,
        unsigned int component_count, uint32_t val_id, bool signedness)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t type_id, true_id, false_id;

    true_id = spirv_compiler_get_constant_float_vector(compiler, signedness ? -1.0f : 1.0f, component_count);
    false_id = spirv_compiler_get_constant_float_vector(compiler, 0.0f, component_count);
    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_FLOAT, component_count);
    return vkd3d_spirv_build_op_select(builder, type_id, val_id, true_id, false_id);
}

static uint32_t spirv_compiler_emit_bool_to_double(struct spirv_compiler *compiler,
        unsigned int component_count, uint32_t val_id, bool signedness)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t type_id, true_id, false_id;

    true_id = spirv_compiler_get_constant_double_vector(compiler, signedness ? -1.0 : 1.0, component_count);
    false_id = spirv_compiler_get_constant_double_vector(compiler, 0.0, component_count);
    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_DOUBLE, component_count);
    return vkd3d_spirv_build_op_select(builder, type_id, val_id, true_id, false_id);
}

/* Based on the implementation in the OpenGL Mathematics library. */
static uint32_t half_to_float(uint16_t value)
{
    uint32_t s = (value & 0x8000u) << 16;
    uint32_t e = (value >> 10) & 0x1fu;
    uint32_t m = value & 0x3ffu;

    if (!e)
    {
        if (!m)
        {
            /* Plus or minus zero */
            return s;
        }
        else
        {
            /* Denormalized number -- renormalize it */

            while (!(m & 0x400u))
            {
                m <<= 1;
                --e;
            }

            ++e;
            m &= ~0x400u;
        }
    }
    else if (e == 31u)
    {
        /* Positive or negative infinity for zero 'm'.
         * Nan for non-zero 'm' -- preserve sign and significand bits */
        return s | 0x7f800000u | (m << 13);
    }

    /* Normalized number */
    e += 127u - 15u;
    m <<= 13;

    /* Assemble s, e and m. */
    return s | (e << 23) | m;
}

static uint32_t convert_raw_constant32(enum vkd3d_data_type data_type, unsigned int uint_value)
{
    int16_t i;

    /* TODO: native 16-bit support. */
    if (data_type != VKD3D_DATA_UINT16 && data_type != VKD3D_DATA_HALF)
        return uint_value;

    if (data_type == VKD3D_DATA_HALF)
        return half_to_float(uint_value);

    /* Values in DXIL have no signedness, so it is ambiguous whether 16-bit constants should or
     * should not be sign-extended when 16-bit execution is not supported. The AMD RX 580 Windows
     * driver has no 16-bit support, and sign-extends all 16-bit constant ints to 32 bits. These
     * results differ from SM 5. The RX 6750 XT supports 16-bit execution, so constants are not
     * extended, and results match SM 5. It seems best to replicate the sign-extension, and if
     * execution is 16-bit, the values will be truncated. */
    i = uint_value;
    return (int32_t)i;
}

static uint32_t spirv_compiler_emit_load_constant(struct spirv_compiler *compiler,
        const struct vkd3d_shader_register *reg, uint32_t swizzle, uint32_t write_mask)
{
    unsigned int component_count = vsir_write_mask_component_count(write_mask);
    uint32_t values[VKD3D_VEC4_SIZE] = {0};
    unsigned int i, j;

    VKD3D_ASSERT(reg->type == VKD3DSPR_IMMCONST);

    if (reg->dimension == VSIR_DIMENSION_SCALAR)
    {
        for (i = 0; i < component_count; ++i)
            values[i] = convert_raw_constant32(reg->data_type, reg->u.immconst_u32[0]);
    }
    else
    {
        for (i = 0, j = 0; i < VKD3D_VEC4_SIZE; ++i)
        {
            if (write_mask & (VKD3DSP_WRITEMASK_0 << i))
                values[j++] = convert_raw_constant32(reg->data_type,
                        reg->u.immconst_u32[vsir_swizzle_get_component(swizzle, i)]);
        }
    }

    return spirv_compiler_get_constant(compiler,
            vkd3d_component_type_from_data_type(reg->data_type), component_count, values);
}

static uint32_t spirv_compiler_emit_load_constant64(struct spirv_compiler *compiler,
        const struct vkd3d_shader_register *reg, uint32_t swizzle, uint32_t write_mask)
{
    unsigned int component_count = vsir_write_mask_component_count(write_mask);
    uint64_t values[VKD3D_DVEC2_SIZE] = {0};
    unsigned int i, j;

    VKD3D_ASSERT(reg->type == VKD3DSPR_IMMCONST64);

    if (reg->dimension == VSIR_DIMENSION_SCALAR)
    {
        for (i = 0; i < component_count; ++i)
            values[i] = *reg->u.immconst_u64;
    }
    else
    {
        for (i = 0, j = 0; i < VKD3D_DVEC2_SIZE; ++i)
        {
            if (write_mask & (VKD3DSP_WRITEMASK_0 << i))
                values[j++] = reg->u.immconst_u64[vsir_swizzle_get_component(swizzle, i)];
        }
    }

    return spirv_compiler_get_constant64(compiler,
            vkd3d_component_type_from_data_type(reg->data_type), component_count, values);
}

static uint32_t spirv_compiler_emit_load_undef(struct spirv_compiler *compiler,
        const struct vkd3d_shader_register *reg, uint32_t write_mask)
{
    unsigned int component_count = vsir_write_mask_component_count(write_mask);
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t type_id;

    VKD3D_ASSERT(reg->type == VKD3DSPR_UNDEF);

    type_id = vkd3d_spirv_get_type_id_for_data_type(builder, reg->data_type, component_count);
    return vkd3d_spirv_get_op_undef(builder, type_id);
}

static uint32_t spirv_compiler_emit_load_scalar(struct spirv_compiler *compiler,
        const struct vkd3d_shader_register *reg, uint32_t swizzle, uint32_t write_mask,
        const struct vkd3d_shader_register_info *reg_info)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t type_id, ptr_type_id, index, reg_id, val_id;
    unsigned int component_idx, reg_component_count;
    enum vkd3d_shader_component_type component_type;
    uint32_t skipped_component_mask;

    VKD3D_ASSERT(!register_is_constant_or_undef(reg));
    VKD3D_ASSERT(vsir_write_mask_component_count(write_mask) == 1);

    component_idx = vsir_write_mask_get_component_idx(write_mask);
    component_idx = vsir_swizzle_get_component(swizzle, component_idx);
    skipped_component_mask = ~reg_info->write_mask & ((VKD3DSP_WRITEMASK_0 << component_idx) - 1);
    if (skipped_component_mask)
        component_idx -= vsir_write_mask_component_count(skipped_component_mask);
    component_type = vkd3d_component_type_from_data_type(reg->data_type);

    reg_component_count = vsir_write_mask_component_count(reg_info->write_mask);

    if (component_idx >= vsir_write_mask_component_count(reg_info->write_mask))
    {
        ERR("Invalid component_idx %u for register %#x, %u (write_mask %#x).\n",
                component_idx, reg->type, reg->idx[0].offset, reg_info->write_mask);
    }

    type_id = vkd3d_spirv_get_type_id(builder, reg_info->component_type, 1);
    reg_id = reg_info->id;
    if (reg_component_count != 1)
    {
        ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, reg_info->storage_class, type_id);
        index = spirv_compiler_get_constant_uint(compiler, component_idx);
        reg_id = vkd3d_spirv_build_op_in_bounds_access_chain1(builder, ptr_type_id, reg_id, index);
    }

    val_id = vkd3d_spirv_build_op_load(builder, type_id, reg_id, SpvMemoryAccessMaskNone);

    if (component_type != reg_info->component_type)
    {
        if (component_type == VKD3D_SHADER_COMPONENT_BOOL)
        {
            if (reg_info->component_type != VKD3D_SHADER_COMPONENT_UINT)
            {
                type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, 1);
                val_id = vkd3d_spirv_build_op_bitcast(builder, type_id, val_id);
            }
            val_id = spirv_compiler_emit_int_to_bool(compiler, VKD3D_SHADER_CONDITIONAL_OP_NZ,
                    VKD3D_DATA_UINT, 1, val_id);
        }
        else
        {
            type_id = vkd3d_spirv_get_type_id(builder, component_type, 1);
            val_id = vkd3d_spirv_build_op_bitcast(builder, type_id, val_id);
        }
    }

    return val_id;
}

static uint32_t spirv_compiler_emit_constant_array(struct spirv_compiler *compiler,
        const struct vkd3d_shader_immediate_constant_buffer *icb, uint32_t *type_id_out)
{
    uint32_t *elements, elem_type_id, length_id, type_id, const_id;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    enum vkd3d_shader_component_type component_type;
    unsigned int i, element_count, component_count;

    element_count = icb->element_count;

    component_type = vkd3d_component_type_from_data_type(icb->data_type);
    component_count = icb->component_count;
    elem_type_id = vkd3d_spirv_get_type_id_for_data_type(builder, icb->data_type, component_count);
    length_id = spirv_compiler_get_constant_uint(compiler, element_count);
    type_id = vkd3d_spirv_get_op_type_array(builder, elem_type_id, length_id);

    if (type_id_out)
        *type_id_out = type_id;

    if (icb->is_null)
    {
        /* All values are null. Workgroup memory initialisers require OpConstantNull. */
        return vkd3d_spirv_get_op_constant_null(builder, type_id);
    }

    if (!(elements = vkd3d_calloc(element_count, sizeof(*elements))))
    {
        ERR("Failed to allocate %u elements.", element_count);
        spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_OUT_OF_MEMORY,
                "Failed to allocate %u constant array elements.", element_count);
        return 0;
    }

    switch (icb->data_type)
    {
        case VKD3D_DATA_HALF:
        case VKD3D_DATA_UINT16:
            /* Scalar only. */
            for (i = 0; i < element_count; ++i)
                elements[i] = vkd3d_spirv_get_op_constant(builder, elem_type_id,
                        convert_raw_constant32(icb->data_type, icb->data[i]));
            break;
        case VKD3D_DATA_FLOAT:
        case VKD3D_DATA_INT:
        case VKD3D_DATA_UINT:
            for (i = 0; i < element_count; ++i)
                elements[i] = spirv_compiler_get_constant(compiler, component_type, component_count,
                        &icb->data[component_count * i]);
            break;
        case VKD3D_DATA_DOUBLE:
        case VKD3D_DATA_UINT64:
        {
            uint64_t *data = (uint64_t *)icb->data;
            for (i = 0; i < element_count; ++i)
                elements[i] = spirv_compiler_get_constant64(compiler, component_type, component_count,
                        &data[component_count * i]);
            break;
        }
        default:
            FIXME("Unhandled data type %u.\n", icb->data_type);
            spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_INVALID_TYPE,
                    "Immediate constant buffer data type %u is unhandled.", icb->data_type);
            break;
    }

    const_id = vkd3d_spirv_build_op_constant_composite(builder, type_id, elements, element_count);
    vkd3d_free(elements);
    return const_id;
}

static const struct ssa_register_info *spirv_compiler_get_ssa_register_info(const struct spirv_compiler *compiler,
        const struct vkd3d_shader_register *reg)
{
    VKD3D_ASSERT(reg->idx[0].offset < compiler->ssa_register_count);
    VKD3D_ASSERT(reg->idx_count == 1);
    return &compiler->ssa_register_info[reg->idx[0].offset];
}

static void spirv_compiler_set_ssa_register_info(const struct spirv_compiler *compiler,
        const struct vkd3d_shader_register *reg, uint32_t val_id)
{
    unsigned int i = reg->idx[0].offset;
    VKD3D_ASSERT(i < compiler->ssa_register_count);
    compiler->ssa_register_info[i].data_type = reg->data_type;
    compiler->ssa_register_info[i].id = val_id;
}

static uint32_t spirv_compiler_emit_load_ssa_reg(struct spirv_compiler *compiler,
        const struct vkd3d_shader_register *reg, enum vkd3d_shader_component_type component_type,
        uint32_t swizzle)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    enum vkd3d_shader_component_type reg_component_type;
    const struct ssa_register_info *ssa;
    unsigned int component_idx;
    uint32_t type_id, val_id;

    ssa = spirv_compiler_get_ssa_register_info(compiler, reg);
    val_id = ssa->id;
    if (!val_id)
    {
        /* Should only be from a missing instruction implementation. */
        VKD3D_ASSERT(compiler->failed);
        return 0;
    }
    VKD3D_ASSERT(vkd3d_swizzle_is_scalar(swizzle, reg));

    reg_component_type = vkd3d_component_type_from_data_type(ssa->data_type);

    if (reg->dimension == VSIR_DIMENSION_SCALAR)
    {
        if (component_type != reg_component_type)
        {
            type_id = vkd3d_spirv_get_type_id(builder, component_type, 1);
            val_id = vkd3d_spirv_build_op_bitcast(builder, type_id, val_id);
        }

        return val_id;
    }

    if (component_type != reg_component_type)
    {
        /* Required for resource loads with sampled type int, because DXIL has no signedness.
         * Only 128-bit vector sizes are used. */
        type_id = vkd3d_spirv_get_type_id(builder, component_type, VKD3D_VEC4_SIZE);
        val_id = vkd3d_spirv_build_op_bitcast(builder, type_id, val_id);
    }

    type_id = vkd3d_spirv_get_type_id(builder, component_type, 1);
    component_idx = vsir_swizzle_get_component(swizzle, 0);
    return vkd3d_spirv_build_op_composite_extract1(builder, type_id, val_id, component_idx);
}

static uint32_t spirv_compiler_emit_load_reg(struct spirv_compiler *compiler,
        const struct vkd3d_shader_register *reg, uint32_t swizzle, uint32_t write_mask)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    enum vkd3d_shader_component_type component_type;
    struct vkd3d_shader_register_info reg_info;
    unsigned int component_count;
    uint32_t type_id, val_id;
    uint32_t val_write_mask;

    if (reg->type == VKD3DSPR_IMMCONST)
        return spirv_compiler_emit_load_constant(compiler, reg, swizzle, write_mask);
    else if (reg->type == VKD3DSPR_IMMCONST64)
        return spirv_compiler_emit_load_constant64(compiler, reg, swizzle, write_mask);
    else if (reg->type == VKD3DSPR_UNDEF)
        return spirv_compiler_emit_load_undef(compiler, reg, write_mask);
    else if (reg->type == VKD3DSPR_PARAMETER)
        return spirv_compiler_emit_shader_parameter(compiler, reg->idx[0].offset,
                reg->data_type, reg->dimension == VSIR_DIMENSION_VEC4 ? 4 : 1);

    component_count = vsir_write_mask_component_count(write_mask);
    component_type = vkd3d_component_type_from_data_type(reg->data_type);

    if (reg->type == VKD3DSPR_SSA)
        return spirv_compiler_emit_load_ssa_reg(compiler, reg, component_type, swizzle);

    if (!spirv_compiler_get_register_info(compiler, reg, &reg_info))
    {
        type_id = vkd3d_spirv_get_type_id(builder, component_type, component_count);
        return vkd3d_spirv_get_op_undef(builder, type_id);
    }
    spirv_compiler_emit_dereference_register(compiler, reg, &reg_info);

    val_write_mask = (data_type_is_64_bit(reg->data_type) && !component_type_is_64_bit(reg_info.component_type))
            ? vsir_write_mask_32_from_64(write_mask) : write_mask;

    /* Intermediate value (no storage class). */
    if (reg_info.storage_class == SpvStorageClassMax)
    {
        val_id = reg_info.id;
    }
    else if (vsir_write_mask_component_count(val_write_mask) == 1)
    {
        return spirv_compiler_emit_load_scalar(compiler, reg, swizzle, write_mask, &reg_info);
    }
    else
    {
        type_id = vkd3d_spirv_get_type_id(builder,
                reg_info.component_type, vsir_write_mask_component_count(reg_info.write_mask));
        val_id = vkd3d_spirv_build_op_load(builder, type_id, reg_info.id, SpvMemoryAccessMaskNone);
    }

    swizzle = data_type_is_64_bit(reg->data_type) ? vsir_swizzle_32_from_64(swizzle) : swizzle;
    val_id = spirv_compiler_emit_swizzle(compiler,
            val_id, reg_info.write_mask, reg_info.component_type, swizzle, val_write_mask);

    if (component_type != reg_info.component_type)
    {
        if (component_type == VKD3D_SHADER_COMPONENT_BOOL)
        {
            if (reg_info.component_type != VKD3D_SHADER_COMPONENT_UINT)
            {
                type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, component_count);
                val_id = vkd3d_spirv_build_op_bitcast(builder, type_id, val_id);
            }
            val_id = spirv_compiler_emit_int_to_bool(compiler, VKD3D_SHADER_CONDITIONAL_OP_NZ,
                    VKD3D_DATA_UINT, component_count, val_id);
        }
        else
        {
            type_id = vkd3d_spirv_get_type_id(builder, component_type, component_count);
            val_id = vkd3d_spirv_build_op_bitcast(builder, type_id, val_id);
        }
    }

    return val_id;
}

static void spirv_compiler_emit_execution_mode(struct spirv_compiler *compiler,
        SpvExecutionMode mode, const uint32_t *literals, unsigned int literal_count)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;

    vkd3d_spirv_build_op_execution_mode(&builder->execution_mode_stream,
            builder->main_function_id, mode, literals, literal_count);
}

static void spirv_compiler_emit_execution_mode1(struct spirv_compiler *compiler,
        SpvExecutionMode mode, const uint32_t literal)
{
    spirv_compiler_emit_execution_mode(compiler, mode, &literal, 1);
}

static uint32_t spirv_compiler_emit_abs(struct spirv_compiler *compiler,
        const struct vkd3d_shader_register *reg, uint32_t write_mask, uint32_t val_id)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t type_id;

    type_id = spirv_compiler_get_type_id_for_reg(compiler, reg, write_mask);
    if (data_type_is_floating_point(reg->data_type))
        return vkd3d_spirv_build_op_glsl_std450_fabs(builder, type_id, val_id);

    FIXME("Unhandled data type %#x.\n", reg->data_type);
    return val_id;
}

static uint32_t spirv_compiler_emit_neg(struct spirv_compiler *compiler,
        const struct vkd3d_shader_register *reg, uint32_t write_mask, uint32_t val_id)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t type_id;

    type_id = spirv_compiler_get_type_id_for_reg(compiler, reg, write_mask);
    if (data_type_is_floating_point(reg->data_type))
        return vkd3d_spirv_build_op_fnegate(builder, type_id, val_id);
    else if (data_type_is_integer(reg->data_type))
        return vkd3d_spirv_build_op_snegate(builder, type_id, val_id);

    FIXME("Unhandled data type %#x.\n", reg->data_type);
    return val_id;
}

static uint32_t spirv_compiler_emit_src_modifier(struct spirv_compiler *compiler,
        const struct vkd3d_shader_register *reg, uint32_t write_mask,
        enum vkd3d_shader_src_modifier modifier, uint32_t val_id)
{
    switch (modifier)
    {
        case VKD3DSPSM_NONE:
            break;
        case VKD3DSPSM_NEG:
            return spirv_compiler_emit_neg(compiler, reg, write_mask, val_id);
        case VKD3DSPSM_ABS:
            return spirv_compiler_emit_abs(compiler, reg, write_mask, val_id);
        case VKD3DSPSM_ABSNEG:
            val_id = spirv_compiler_emit_abs(compiler, reg, write_mask, val_id);
            return spirv_compiler_emit_neg(compiler, reg, write_mask, val_id);
        default:
            FIXME("Unhandled src modifier %#x.\n", modifier);
            break;
    }

    return val_id;
}

static uint32_t spirv_compiler_emit_load_src(struct spirv_compiler *compiler,
        const struct vkd3d_shader_src_param *src, uint32_t write_mask)
{
    uint32_t val_id;

    val_id = spirv_compiler_emit_load_reg(compiler, &src->reg, src->swizzle, write_mask);
    return spirv_compiler_emit_src_modifier(compiler, &src->reg, write_mask, src->modifiers, val_id);
}

static uint32_t spirv_compiler_emit_load_src_with_type(struct spirv_compiler *compiler,
        const struct vkd3d_shader_src_param *src, uint32_t write_mask, enum vkd3d_shader_component_type component_type)
{
    struct vkd3d_shader_src_param src_param = *src;

    src_param.reg.data_type = vkd3d_data_type_from_component_type(component_type);
    return spirv_compiler_emit_load_src(compiler, &src_param, write_mask);
}

static void spirv_compiler_emit_store_scalar(struct spirv_compiler *compiler,
        uint32_t dst_id, uint32_t dst_write_mask, enum vkd3d_shader_component_type component_type,
        SpvStorageClass storage_class, uint32_t write_mask, uint32_t val_id)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t type_id, ptr_type_id, index;
    unsigned int component_idx;

    if (vsir_write_mask_component_count(dst_write_mask) > 1)
    {
        type_id = vkd3d_spirv_get_type_id(builder, component_type, 1);
        ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, storage_class, type_id);
        component_idx = vsir_write_mask_get_component_idx(write_mask);
        component_idx -= vsir_write_mask_get_component_idx(dst_write_mask);
        index = spirv_compiler_get_constant_uint(compiler, component_idx);
        dst_id = vkd3d_spirv_build_op_in_bounds_access_chain1(builder, ptr_type_id, dst_id, index);
    }

    vkd3d_spirv_build_op_store(builder, dst_id, val_id, SpvMemoryAccessMaskNone);
}

static void spirv_compiler_emit_store(struct spirv_compiler *compiler,
        uint32_t dst_id, uint32_t dst_write_mask, enum vkd3d_shader_component_type component_type,
        SpvStorageClass storage_class, uint32_t write_mask, uint32_t val_id)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    unsigned int component_count, dst_component_count;
    uint32_t components[VKD3D_VEC4_SIZE];
    unsigned int i, src_idx, dst_idx;
    uint32_t type_id, dst_val_id;

    VKD3D_ASSERT(write_mask);

    component_count = vsir_write_mask_component_count(write_mask);
    dst_component_count = vsir_write_mask_component_count(dst_write_mask);

    if (dst_component_count == 1 && component_count != 1)
    {
        type_id = vkd3d_spirv_get_type_id(builder, component_type, 1);
        val_id = vkd3d_spirv_build_op_composite_extract1(builder, type_id, val_id,
                vsir_write_mask_get_component_idx(dst_write_mask));
        write_mask &= dst_write_mask;
        component_count = 1;
    }

    if (component_count == 1)
    {
        return spirv_compiler_emit_store_scalar(compiler,
                dst_id, dst_write_mask, component_type, storage_class, write_mask, val_id);
    }

    if (dst_component_count != component_count)
    {
        type_id = vkd3d_spirv_get_type_id(builder, component_type, dst_component_count);
        dst_val_id = vkd3d_spirv_build_op_load(builder, type_id, dst_id, SpvMemoryAccessMaskNone);

        VKD3D_ASSERT(component_count <= ARRAY_SIZE(components));

        for (i = 0, src_idx = 0, dst_idx = 0; dst_idx < VKD3D_VEC4_SIZE; ++dst_idx)
        {
            if (write_mask & (VKD3DSP_WRITEMASK_0 << dst_idx))
                components[i] = dst_component_count + src_idx++;
            else
                components[i] = i;

            if (dst_write_mask & (VKD3DSP_WRITEMASK_0 << dst_idx))
                ++i;
        }

        val_id = vkd3d_spirv_build_op_vector_shuffle(builder,
                type_id, dst_val_id, val_id, components, dst_component_count);
    }

    vkd3d_spirv_build_op_store(builder, dst_id, val_id, SpvMemoryAccessMaskNone);
}

static void spirv_compiler_emit_store_reg(struct spirv_compiler *compiler,
        const struct vkd3d_shader_register *reg, uint32_t write_mask, uint32_t val_id)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    enum vkd3d_shader_component_type component_type;
    struct vkd3d_shader_register_info reg_info;
    uint32_t src_write_mask = write_mask;
    uint32_t type_id;

    VKD3D_ASSERT(!register_is_constant_or_undef(reg));

    if (reg->type == VKD3DSPR_SSA)
    {
        spirv_compiler_set_ssa_register_info(compiler, reg, val_id);
        return;
    }

    if (!spirv_compiler_get_register_info(compiler, reg, &reg_info))
        return;
    spirv_compiler_emit_dereference_register(compiler, reg, &reg_info);

    component_type = vkd3d_component_type_from_data_type(reg->data_type);
    if (component_type != reg_info.component_type)
    {
        if (data_type_is_64_bit(reg->data_type))
            src_write_mask = vsir_write_mask_32_from_64(write_mask);
        if (component_type == VKD3D_SHADER_COMPONENT_BOOL)
            val_id = spirv_compiler_emit_bool_to_int(compiler,
                    vsir_write_mask_component_count(src_write_mask), val_id, false);
        type_id = vkd3d_spirv_get_type_id(builder, reg_info.component_type,
                vsir_write_mask_component_count(src_write_mask));
        val_id = vkd3d_spirv_build_op_bitcast(builder, type_id, val_id);
        component_type = reg_info.component_type;
    }

    spirv_compiler_emit_store(compiler,
            reg_info.id, reg_info.write_mask, component_type, reg_info.storage_class, src_write_mask, val_id);
}

static uint32_t spirv_compiler_emit_sat(struct spirv_compiler *compiler,
        const struct vkd3d_shader_register *reg, uint32_t write_mask, uint32_t val_id)
{
    unsigned int component_count = vsir_write_mask_component_count(write_mask);
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t type_id, zero_id, one_id;

    if (reg->data_type == VKD3D_DATA_DOUBLE)
    {
        zero_id = spirv_compiler_get_constant_double_vector(compiler, 0.0, component_count);
        one_id = spirv_compiler_get_constant_double_vector(compiler, 1.0, component_count);
    }
    else
    {
        zero_id = spirv_compiler_get_constant_float_vector(compiler, 0.0f, component_count);
        one_id = spirv_compiler_get_constant_float_vector(compiler, 1.0f, component_count);
    }

    type_id = spirv_compiler_get_type_id_for_reg(compiler, reg, write_mask);
    if (data_type_is_floating_point(reg->data_type))
        return vkd3d_spirv_build_op_glsl_std450_nclamp(builder, type_id, val_id, zero_id, one_id);

    FIXME("Unhandled data type %#x.\n", reg->data_type);
    return val_id;
}

static void spirv_compiler_emit_store_dst(struct spirv_compiler *compiler,
        const struct vkd3d_shader_dst_param *dst, uint32_t val_id)
{
    uint32_t modifiers = dst->modifiers;

    /* It is always legitimate to ignore _pp. */
    modifiers &= ~VKD3DSPDM_PARTIALPRECISION;

    if (modifiers & VKD3DSPDM_SATURATE)
    {
        val_id = spirv_compiler_emit_sat(compiler, &dst->reg, dst->write_mask, val_id);
        modifiers &= ~VKD3DSPDM_SATURATE;
    }

    if (dst->modifiers & VKD3DSPDM_MSAMPCENTROID)
    {
        FIXME("Ignoring _centroid modifier.\n");
        modifiers &= ~VKD3DSPDM_MSAMPCENTROID;
    }

    VKD3D_ASSERT(!modifiers);

    spirv_compiler_emit_store_reg(compiler, &dst->reg, dst->write_mask, val_id);
}

static void spirv_compiler_emit_store_dst_swizzled(struct spirv_compiler *compiler,
        const struct vkd3d_shader_dst_param *dst, uint32_t val_id,
        enum vkd3d_shader_component_type component_type, uint32_t swizzle)
{
    struct vkd3d_shader_dst_param typed_dst = *dst;
    val_id = spirv_compiler_emit_swizzle(compiler,
            val_id, VKD3DSP_WRITEMASK_ALL, component_type, swizzle, dst->write_mask);
    /* XXX: The register data type could be fixed by the shader parser. For SM5
     * shaders the data types are stored in instructions modifiers.
     */
    typed_dst.reg.data_type = vkd3d_data_type_from_component_type(component_type);
    spirv_compiler_emit_store_dst(compiler, &typed_dst, val_id);
}

static void spirv_compiler_emit_store_dst_components(struct spirv_compiler *compiler,
        const struct vkd3d_shader_dst_param *dst, enum vkd3d_shader_component_type component_type,
        uint32_t *component_ids)
{
    unsigned int component_count = vsir_write_mask_component_count(dst->write_mask);
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t type_id, dst_type_id, val_id;

    type_id = vkd3d_spirv_get_type_id(builder, component_type, component_count);
    if (component_count > 1)
    {
        val_id = vkd3d_spirv_build_op_composite_construct(builder,
                type_id, component_ids, component_count);
    }
    else
    {
        val_id = *component_ids;
    }

    dst_type_id = vkd3d_spirv_get_type_id_for_data_type(builder, dst->reg.data_type, component_count);
    if (dst_type_id != type_id)
        val_id = vkd3d_spirv_build_op_bitcast(builder, dst_type_id, val_id);

    spirv_compiler_emit_store_dst(compiler, dst, val_id);
}

static void spirv_compiler_emit_store_dst_scalar(struct spirv_compiler *compiler,
        const struct vkd3d_shader_dst_param *dst, uint32_t val_id,
        enum vkd3d_shader_component_type component_type, uint32_t swizzle)
{
    unsigned int component_count = vsir_write_mask_component_count(dst->write_mask);
    uint32_t component_ids[VKD3D_VEC4_SIZE];
    unsigned int component_idx, i;

    component_idx = vsir_write_mask_get_component_idx(dst->write_mask);
    for (i = 0; i < component_count; ++i)
    {
        if (vsir_swizzle_get_component(swizzle, component_idx + i))
            ERR("Invalid swizzle %#x for scalar value, write mask %#x.\n", swizzle, dst->write_mask);

        component_ids[i] = val_id;
    }
    spirv_compiler_emit_store_dst_components(compiler, dst, component_type, component_ids);
}

static void spirv_compiler_decorate_builtin(struct spirv_compiler *compiler,
        uint32_t target_id, SpvBuiltIn builtin)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;

    switch (builtin)
    {
        case SpvBuiltInPrimitiveId:
            if (compiler->shader_type == VKD3D_SHADER_TYPE_PIXEL)
                vkd3d_spirv_enable_capability(builder, SpvCapabilityGeometry);
            break;
        case SpvBuiltInFragDepth:
            spirv_compiler_emit_execution_mode(compiler, SpvExecutionModeDepthReplacing, NULL, 0);
            break;
        case SpvBuiltInLayer:
            switch (compiler->shader_type)
            {
                case VKD3D_SHADER_TYPE_PIXEL:
                case VKD3D_SHADER_TYPE_GEOMETRY:
                    vkd3d_spirv_enable_capability(builder, SpvCapabilityGeometry);
                    break;

                case VKD3D_SHADER_TYPE_VERTEX:
                case VKD3D_SHADER_TYPE_DOMAIN:
                    if (!spirv_compiler_is_target_extension_supported(compiler,
                            VKD3D_SHADER_SPIRV_EXTENSION_EXT_VIEWPORT_INDEX_LAYER))
                    {
                        FIXME("The target environment does not support decoration Layer.\n");
                        spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_UNSUPPORTED_FEATURE,
                                "Cannot use SV_RenderTargetArrayIndex. "
                                "The target environment does not support decoration Layer.");
                    }
                    vkd3d_spirv_enable_capability(builder, SpvCapabilityShaderViewportIndexLayerEXT);
                    break;

                default:
                    spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_INVALID_SHADER,
                            "Invalid use of SV_RenderTargetArrayIndex.");
                    break;
            }
            break;
        case SpvBuiltInViewportIndex:
            switch (compiler->shader_type)
            {
                case VKD3D_SHADER_TYPE_PIXEL:
                case VKD3D_SHADER_TYPE_GEOMETRY:
                    vkd3d_spirv_enable_capability(builder, SpvCapabilityMultiViewport);
                    break;

                case VKD3D_SHADER_TYPE_VERTEX:
                case VKD3D_SHADER_TYPE_DOMAIN:
                    if (!spirv_compiler_is_target_extension_supported(compiler,
                            VKD3D_SHADER_SPIRV_EXTENSION_EXT_VIEWPORT_INDEX_LAYER))
                    {
                        FIXME("The target environment does not support decoration ViewportIndex.\n");
                        spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_UNSUPPORTED_FEATURE,
                                "Cannot use SV_ViewportArrayIndex. "
                                "The target environment does not support decoration ViewportIndex.");
                    }
                    vkd3d_spirv_enable_capability(builder, SpvCapabilityShaderViewportIndexLayerEXT);
                    break;

                default:
                    spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_INVALID_SHADER,
                            "Invalid use of SV_ViewportArrayIndex.");
                    break;
            }
            break;
        case SpvBuiltInSampleId:
            vkd3d_spirv_enable_capability(builder, SpvCapabilitySampleRateShading);
            break;
        case SpvBuiltInClipDistance:
            vkd3d_spirv_enable_capability(builder, SpvCapabilityClipDistance);
            break;
        case SpvBuiltInCullDistance:
            vkd3d_spirv_enable_capability(builder, SpvCapabilityCullDistance);
            break;
        case SpvBuiltInSubgroupSize:
        case SpvBuiltInSubgroupLocalInvocationId:
            vkd3d_spirv_enable_capability(builder, SpvCapabilityGroupNonUniform);
            break;
        default:
            break;
    }

    vkd3d_spirv_build_op_decorate1(builder, target_id, SpvDecorationBuiltIn, builtin);
}

static void spirv_compiler_emit_interpolation_decorations(struct spirv_compiler *compiler,
        enum vkd3d_shader_component_type component_type, uint32_t id, enum vkd3d_shader_interpolation_mode mode)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;

    switch (mode)
    {
        case VKD3DSIM_NONE:
            /* VUID-StandaloneSpirv-Flat-04744: integer or double types must be
             * decorated 'Flat' for fragment shaders. */
            if (compiler->shader_type != VKD3D_SHADER_TYPE_PIXEL || component_type == VKD3D_SHADER_COMPONENT_FLOAT)
                break;
            /* fall through */
        case VKD3DSIM_CONSTANT:
            vkd3d_spirv_build_op_decorate(builder, id, SpvDecorationFlat, NULL, 0);
            break;
        case VKD3DSIM_LINEAR:
            break;
        case VKD3DSIM_LINEAR_CENTROID:
            vkd3d_spirv_build_op_decorate(builder, id, SpvDecorationCentroid, NULL, 0);
            break;
        case VKD3DSIM_LINEAR_NOPERSPECTIVE:
            vkd3d_spirv_build_op_decorate(builder, id, SpvDecorationNoPerspective, NULL, 0);
            break;
        case VKD3DSIM_LINEAR_SAMPLE:
            vkd3d_spirv_enable_capability(builder, SpvCapabilitySampleRateShading);
            vkd3d_spirv_build_op_decorate(builder, id, SpvDecorationSample, NULL, 0);
            break;
        case VKD3DSIM_LINEAR_NOPERSPECTIVE_SAMPLE:
            vkd3d_spirv_build_op_decorate(builder, id, SpvDecorationNoPerspective, NULL, 0);
            vkd3d_spirv_enable_capability(builder, SpvCapabilitySampleRateShading);
            vkd3d_spirv_build_op_decorate(builder, id, SpvDecorationSample, NULL, 0);
            break;
        default:
            FIXME("Unhandled interpolation mode %#x.\n", mode);
            break;
    }
}

typedef uint32_t (*vkd3d_spirv_builtin_fixup_pfn)(struct spirv_compiler *compiler,
        uint32_t val_id);

static uint32_t spirv_compiler_emit_draw_parameter_fixup(struct spirv_compiler *compiler,
        uint32_t index_id, SpvBuiltIn base)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t base_var_id, base_id, type_id;

    vkd3d_spirv_enable_capability(builder, SpvCapabilityDrawParameters);

    base_var_id = spirv_compiler_emit_variable(compiler, &builder->global_stream,
            SpvStorageClassInput, VKD3D_SHADER_COMPONENT_INT, 1);
    vkd3d_spirv_add_iface_variable(builder, base_var_id);
    spirv_compiler_decorate_builtin(compiler, base_var_id, base);

    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_INT, 1);
    base_id = vkd3d_spirv_build_op_load(builder,
            type_id, base_var_id, SpvMemoryAccessMaskNone);

    return vkd3d_spirv_build_op_isub(builder, type_id, index_id, base_id);
}

/* Substitute "VertexIndex - BaseVertex" for SV_VertexID. */
static uint32_t sv_vertex_id_fixup(struct spirv_compiler *compiler,
        uint32_t vertex_index_id)
{
    return spirv_compiler_emit_draw_parameter_fixup(compiler,
            vertex_index_id, SpvBuiltInBaseVertex);
}

/* Substitute "InstanceIndex - BaseInstance" for SV_InstanceID. */
static uint32_t sv_instance_id_fixup(struct spirv_compiler *compiler,
        uint32_t instance_index_id)
{
    return spirv_compiler_emit_draw_parameter_fixup(compiler,
            instance_index_id, SpvBuiltInBaseInstance);
}

static uint32_t sv_front_face_fixup(struct spirv_compiler *compiler,
        uint32_t front_facing_id)
{
    return spirv_compiler_emit_bool_to_int(compiler, 1, front_facing_id, true);
}

/* frag_coord.w = 1.0f / frag_coord.w */
static uint32_t frag_coord_fixup(struct spirv_compiler *compiler,
        uint32_t frag_coord_id)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t type_id, w_id;

    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_FLOAT, 1);
    w_id = vkd3d_spirv_build_op_composite_extract1(builder, type_id, frag_coord_id, 3);
    w_id = vkd3d_spirv_build_op_fdiv(builder, type_id,
            spirv_compiler_get_constant_float(compiler, 1.0f), w_id);
    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_FLOAT, VKD3D_VEC4_SIZE);
    return vkd3d_spirv_build_op_composite_insert1(builder, type_id, w_id, frag_coord_id, 3);
}

struct vkd3d_spirv_builtin
{
    enum vkd3d_shader_component_type component_type;
    unsigned int component_count;
    SpvBuiltIn spirv_builtin;
    vkd3d_spirv_builtin_fixup_pfn fixup_pfn;
    unsigned int spirv_array_size;
    unsigned int member_idx;
};

/*
 * The following tables are based on the "14.6. Built-In Variables" section
 * from the Vulkan spec.
 */
static const struct
{
    enum vkd3d_shader_sysval_semantic sysval;
    struct vkd3d_spirv_builtin builtin;
    enum vkd3d_shader_spirv_environment environment;
}
vkd3d_system_value_builtins[] =
{
    {VKD3D_SHADER_SV_VERTEX_ID,             {VKD3D_SHADER_COMPONENT_INT, 1, SpvBuiltInVertexId},
            VKD3D_SHADER_SPIRV_ENVIRONMENT_OPENGL_4_5},
    {VKD3D_SHADER_SV_INSTANCE_ID,           {VKD3D_SHADER_COMPONENT_INT, 1, SpvBuiltInInstanceId},
            VKD3D_SHADER_SPIRV_ENVIRONMENT_OPENGL_4_5},

    {VKD3D_SHADER_SV_POSITION,              {VKD3D_SHADER_COMPONENT_FLOAT, 4, SpvBuiltInPosition}},
    {VKD3D_SHADER_SV_VERTEX_ID,             {VKD3D_SHADER_COMPONENT_INT, 1, SpvBuiltInVertexIndex, sv_vertex_id_fixup}},
    {VKD3D_SHADER_SV_INSTANCE_ID,           {VKD3D_SHADER_COMPONENT_INT, 1, SpvBuiltInInstanceIndex, sv_instance_id_fixup}},

    {VKD3D_SHADER_SV_PRIMITIVE_ID,          {VKD3D_SHADER_COMPONENT_INT, 1, SpvBuiltInPrimitiveId}},

    {VKD3D_SHADER_SV_RENDER_TARGET_ARRAY_INDEX, {VKD3D_SHADER_COMPONENT_INT, 1, SpvBuiltInLayer}},
    {VKD3D_SHADER_SV_VIEWPORT_ARRAY_INDEX,      {VKD3D_SHADER_COMPONENT_INT, 1, SpvBuiltInViewportIndex}},

    {VKD3D_SHADER_SV_IS_FRONT_FACE,         {VKD3D_SHADER_COMPONENT_BOOL, 1, SpvBuiltInFrontFacing, sv_front_face_fixup}},

    {VKD3D_SHADER_SV_SAMPLE_INDEX,          {VKD3D_SHADER_COMPONENT_UINT, 1, SpvBuiltInSampleId}},

    {VKD3D_SHADER_SV_CLIP_DISTANCE,         {VKD3D_SHADER_COMPONENT_FLOAT, 1, SpvBuiltInClipDistance, NULL, 1}},
    {VKD3D_SHADER_SV_CULL_DISTANCE,         {VKD3D_SHADER_COMPONENT_FLOAT, 1, SpvBuiltInCullDistance, NULL, 1}},

    {VKD3D_SHADER_SV_TESS_FACTOR_QUADEDGE,  {VKD3D_SHADER_COMPONENT_FLOAT, 1, SpvBuiltInTessLevelOuter, NULL, 4}},
    {VKD3D_SHADER_SV_TESS_FACTOR_QUADINT,   {VKD3D_SHADER_COMPONENT_FLOAT, 1, SpvBuiltInTessLevelInner, NULL, 2}},

    {VKD3D_SHADER_SV_TESS_FACTOR_TRIEDGE,   {VKD3D_SHADER_COMPONENT_FLOAT, 1, SpvBuiltInTessLevelOuter, NULL, 4}},
    {VKD3D_SHADER_SV_TESS_FACTOR_TRIINT,    {VKD3D_SHADER_COMPONENT_FLOAT, 1, SpvBuiltInTessLevelInner, NULL, 2}},

    {VKD3D_SHADER_SV_TESS_FACTOR_LINEDEN,   {VKD3D_SHADER_COMPONENT_FLOAT, 1, SpvBuiltInTessLevelOuter, NULL, 4, 0}},
    {VKD3D_SHADER_SV_TESS_FACTOR_LINEDET,   {VKD3D_SHADER_COMPONENT_FLOAT, 1, SpvBuiltInTessLevelOuter, NULL, 4, 1}},
};
static const struct vkd3d_spirv_builtin vkd3d_pixel_shader_position_builtin =
{
    VKD3D_SHADER_COMPONENT_FLOAT, 4, SpvBuiltInFragCoord, frag_coord_fixup,
};
static const struct vkd3d_spirv_builtin vkd3d_output_point_size_builtin =
{
    VKD3D_SHADER_COMPONENT_FLOAT, 1, SpvBuiltInPointSize,
};
static const struct
{
    enum vkd3d_shader_register_type reg_type;
    struct vkd3d_spirv_builtin builtin;
}
vkd3d_register_builtins[] =
{
    {VKD3DSPR_THREADID,         {VKD3D_SHADER_COMPONENT_INT, 3, SpvBuiltInGlobalInvocationId}},
    {VKD3DSPR_LOCALTHREADID,    {VKD3D_SHADER_COMPONENT_INT, 3, SpvBuiltInLocalInvocationId}},
    {VKD3DSPR_LOCALTHREADINDEX, {VKD3D_SHADER_COMPONENT_INT, 1, SpvBuiltInLocalInvocationIndex}},
    {VKD3DSPR_THREADGROUPID,    {VKD3D_SHADER_COMPONENT_INT, 3, SpvBuiltInWorkgroupId}},

    {VKD3DSPR_GSINSTID,         {VKD3D_SHADER_COMPONENT_INT, 1, SpvBuiltInInvocationId}},
    {VKD3DSPR_OUTPOINTID,       {VKD3D_SHADER_COMPONENT_INT, 1, SpvBuiltInInvocationId}},

    {VKD3DSPR_PRIMID,           {VKD3D_SHADER_COMPONENT_INT, 1, SpvBuiltInPrimitiveId}},

    {VKD3DSPR_TESSCOORD,        {VKD3D_SHADER_COMPONENT_FLOAT, 3, SpvBuiltInTessCoord}},

    {VKD3DSPR_POINT_COORD,      {VKD3D_SHADER_COMPONENT_FLOAT, 2, SpvBuiltInPointCoord}},

    {VKD3DSPR_COVERAGE,         {VKD3D_SHADER_COMPONENT_UINT, 1, SpvBuiltInSampleMask, NULL, 1}},
    {VKD3DSPR_SAMPLEMASK,       {VKD3D_SHADER_COMPONENT_UINT, 1, SpvBuiltInSampleMask, NULL, 1}},

    {VKD3DSPR_DEPTHOUT,         {VKD3D_SHADER_COMPONENT_FLOAT, 1, SpvBuiltInFragDepth}},
    {VKD3DSPR_DEPTHOUTGE,       {VKD3D_SHADER_COMPONENT_FLOAT, 1, SpvBuiltInFragDepth}},
    {VKD3DSPR_DEPTHOUTLE,       {VKD3D_SHADER_COMPONENT_FLOAT, 1, SpvBuiltInFragDepth}},

    {VKD3DSPR_OUTSTENCILREF,    {VKD3D_SHADER_COMPONENT_UINT, 1, SpvBuiltInFragStencilRefEXT}},

    {VKD3DSPR_WAVELANECOUNT,    {VKD3D_SHADER_COMPONENT_UINT, 1, SpvBuiltInSubgroupSize}},
    {VKD3DSPR_WAVELANEINDEX,    {VKD3D_SHADER_COMPONENT_UINT, 1, SpvBuiltInSubgroupLocalInvocationId}},
};

static void spirv_compiler_emit_register_execution_mode(struct spirv_compiler *compiler,
        enum vkd3d_shader_register_type type)
{
    switch (type)
    {
        case VKD3DSPR_DEPTHOUTGE:
            spirv_compiler_emit_execution_mode(compiler, SpvExecutionModeDepthGreater, NULL, 0);
            break;
        case VKD3DSPR_DEPTHOUTLE:
            spirv_compiler_emit_execution_mode(compiler, SpvExecutionModeDepthLess, NULL, 0);
            break;
        case VKD3DSPR_OUTSTENCILREF:
            if (!spirv_compiler_is_target_extension_supported(compiler,
                    VKD3D_SHADER_SPIRV_EXTENSION_EXT_STENCIL_EXPORT))
            {
                FIXME("The target environment does not support stencil export.\n");
                spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_UNSUPPORTED_FEATURE,
                        "Cannot export stencil reference value. "
                        "The target environment does not support stencil export.");
            }
            vkd3d_spirv_enable_capability(&compiler->spirv_builder, SpvCapabilityStencilExportEXT);
            spirv_compiler_emit_execution_mode(compiler, SpvExecutionModeStencilRefReplacingEXT, NULL, 0);
            break;
        default:
            return;
    }
}

static const struct vkd3d_spirv_builtin *get_spirv_builtin_for_sysval(
        const struct spirv_compiler *compiler, enum vkd3d_shader_sysval_semantic sysval)
{
    enum vkd3d_shader_spirv_environment environment;
    unsigned int i;

    if (sysval == VKD3D_SHADER_SV_NONE || sysval == VKD3D_SHADER_SV_TARGET)
        return NULL;

    /* In pixel shaders, SV_Position is mapped to SpvBuiltInFragCoord. */
    if (sysval == VKD3D_SHADER_SV_POSITION && compiler->shader_type == VKD3D_SHADER_TYPE_PIXEL)
        return &vkd3d_pixel_shader_position_builtin;

    environment = spirv_compiler_get_target_environment(compiler);
    for (i = 0; i < ARRAY_SIZE(vkd3d_system_value_builtins); ++i)
    {
        if (vkd3d_system_value_builtins[i].sysval == sysval
                && (!vkd3d_system_value_builtins[i].environment
                || vkd3d_system_value_builtins[i].environment == environment))
            return &vkd3d_system_value_builtins[i].builtin;
    }

    FIXME("Unhandled builtin (sysval %#x).\n", sysval);

    return NULL;
}

static const struct vkd3d_spirv_builtin *get_spirv_builtin_for_register(
        enum vkd3d_shader_register_type reg_type)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(vkd3d_register_builtins); ++i)
    {
        if (vkd3d_register_builtins[i].reg_type == reg_type)
            return &vkd3d_register_builtins[i].builtin;
    }

    return NULL;
}

static const struct vkd3d_spirv_builtin *vkd3d_get_spirv_builtin(const struct spirv_compiler *compiler,
        enum vkd3d_shader_register_type reg_type, enum vkd3d_shader_sysval_semantic sysval)
{
    const struct vkd3d_spirv_builtin *builtin;

    if ((builtin = get_spirv_builtin_for_sysval(compiler, sysval)))
        return builtin;
    if ((builtin = get_spirv_builtin_for_register(reg_type)))
        return builtin;

    if ((sysval != VKD3D_SHADER_SV_NONE && sysval != VKD3D_SHADER_SV_TARGET)
            || (reg_type != VKD3DSPR_OUTPUT && reg_type != VKD3DSPR_PATCHCONST))
    {
        FIXME("Unhandled builtin (register type %#x, sysval %#x).\n", reg_type, sysval);
    }
    return NULL;
}

static uint32_t spirv_compiler_get_invocation_id(struct spirv_compiler *compiler)
{
    struct vkd3d_shader_register r;

    VKD3D_ASSERT(compiler->shader_type == VKD3D_SHADER_TYPE_HULL);

    vsir_register_init(&r, VKD3DSPR_OUTPOINTID, VKD3D_DATA_FLOAT, 0);
    return spirv_compiler_get_register_id(compiler, &r);
}

static uint32_t spirv_compiler_emit_load_invocation_id(struct spirv_compiler *compiler)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t type_id, id;

    id = spirv_compiler_get_invocation_id(compiler);
    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_INT, 1);
    return vkd3d_spirv_build_op_load(builder, type_id, id, SpvMemoryAccessMaskNone);
}

static void spirv_compiler_emit_shader_phase_name(struct spirv_compiler *compiler,
        uint32_t id, const char *suffix)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const char *name;

    if (!suffix)
        suffix = "";

    switch (compiler->phase)
    {
        case VKD3DSIH_HS_CONTROL_POINT_PHASE:
            name = "control";
            break;
        case VKD3DSIH_HS_FORK_PHASE:
            name = "fork";
            break;
        case VKD3DSIH_HS_JOIN_PHASE:
            name = "join";
            break;
        default:
            ERR("Invalid phase type %#x.\n", compiler->phase);
            return;
    }
    vkd3d_spirv_build_op_name(builder, id, "%s%s", name, suffix);
}

static const struct vkd3d_shader_phase *spirv_compiler_get_current_shader_phase(
        struct spirv_compiler *compiler)
{
    if (is_in_default_phase(compiler))
        return NULL;

    return is_in_control_point_phase(compiler) ? &compiler->control_point_phase : &compiler->patch_constant_phase;
}

static void spirv_compiler_decorate_xfb_output(struct spirv_compiler *compiler,
        uint32_t id, unsigned int component_count, const struct signature_element *signature_element)
{
    const struct vkd3d_shader_transform_feedback_info *xfb_info = compiler->xfb_info;
    const struct vkd3d_shader_transform_feedback_element *xfb_element;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    unsigned int offset, stride, i;

    if (!xfb_info)
        return;

    offset = 0;
    xfb_element = NULL;
    for (i = 0; i < xfb_info->element_count; ++i)
    {
        const struct vkd3d_shader_transform_feedback_element *e = &xfb_info->elements[i];

        if (e->stream_index == signature_element->stream_index
                && !ascii_strcasecmp(e->semantic_name, signature_element->semantic_name)
                && e->semantic_index == signature_element->semantic_index)
        {
            xfb_element = e;
            break;
        }
    }

    if (!xfb_element)
        return;

    for (i = 0; xfb_element != &xfb_info->elements[i]; ++i)
        if (xfb_info->elements[i].output_slot == xfb_element->output_slot)
            offset += 4 * xfb_info->elements[i].component_count;

    if (xfb_element->component_index || xfb_element->component_count > component_count)
    {
        FIXME("Unhandled component range %u, %u.\n", xfb_element->component_index, xfb_element->component_count);
        return;
    }

    if (xfb_element->output_slot < xfb_info->buffer_stride_count)
    {
        stride = xfb_info->buffer_strides[xfb_element->output_slot];
    }
    else
    {
        stride = 0;
        for (i = 0; i < xfb_info->element_count; ++i)
        {
            const struct vkd3d_shader_transform_feedback_element *e = &xfb_info->elements[i];

            if (e->stream_index == xfb_element->stream_index && e->output_slot == xfb_element->output_slot)
                stride += 4 * e->component_count;
        }
    }

    vkd3d_spirv_build_op_decorate1(builder, id, SpvDecorationXfbBuffer, xfb_element->output_slot);
    vkd3d_spirv_build_op_decorate1(builder, id, SpvDecorationXfbStride, stride);
    vkd3d_spirv_build_op_decorate1(builder, id, SpvDecorationOffset, offset);
}

static uint32_t spirv_compiler_emit_builtin_variable_v(struct spirv_compiler *compiler,
        const struct vkd3d_spirv_builtin *builtin, SpvStorageClass storage_class, const unsigned int *array_sizes,
        unsigned int size_count)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    unsigned int sizes[2];
    uint32_t id;

    VKD3D_ASSERT(size_count <= ARRAY_SIZE(sizes));
    memcpy(sizes, array_sizes, size_count * sizeof(sizes[0]));
    array_sizes = sizes;
    sizes[0] = max(sizes[0], builtin->spirv_array_size);

    id = spirv_compiler_emit_array_variable(compiler, &builder->global_stream, storage_class,
            builtin->component_type, builtin->component_count, array_sizes, size_count);
    vkd3d_spirv_add_iface_variable(builder, id);
    spirv_compiler_decorate_builtin(compiler, id, builtin->spirv_builtin);

    if (compiler->shader_type == VKD3D_SHADER_TYPE_PIXEL && storage_class == SpvStorageClassInput
            && builtin->component_type != VKD3D_SHADER_COMPONENT_FLOAT
            && builtin->component_type != VKD3D_SHADER_COMPONENT_BOOL)
        vkd3d_spirv_build_op_decorate(builder, id, SpvDecorationFlat, NULL, 0);

    return id;
}

static uint32_t spirv_compiler_emit_builtin_variable(struct spirv_compiler *compiler,
        const struct vkd3d_spirv_builtin *builtin, SpvStorageClass storage_class, unsigned int array_size)
{
    return spirv_compiler_emit_builtin_variable_v(compiler, builtin, storage_class, &array_size, 1);
}

static bool needs_private_io_variable(const struct vkd3d_spirv_builtin *builtin)
{
    return builtin && builtin->fixup_pfn;
}

static unsigned int shader_signature_next_location(const struct shader_signature *signature)
{
    unsigned int i, max_row;

    if (!signature)
        return 0;

    for (i = 0, max_row = 0; i < signature->element_count; ++i)
        max_row = max(max_row, signature->elements[i].register_index + signature->elements[i].register_count);
    return max_row;
}

static uint32_t spirv_compiler_emit_input(struct spirv_compiler *compiler,
        enum vkd3d_shader_register_type reg_type, unsigned int element_idx)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    unsigned int component_idx, input_component_count;
    const struct signature_element *signature_element;
    const struct shader_signature *shader_signature;
    enum vkd3d_shader_component_type component_type;
    const struct vkd3d_spirv_builtin *builtin;
    enum vkd3d_shader_sysval_semantic sysval;
    uint32_t write_mask, reg_write_mask;
    struct vkd3d_symbol *symbol = NULL;
    uint32_t val_id, input_id, var_id;
    uint32_t type_id, float_type_id;
    struct vkd3d_symbol reg_symbol;
    SpvStorageClass storage_class;
    struct rb_entry *entry = NULL;
    bool use_private_var = false;
    unsigned int array_sizes[2];

    shader_signature = reg_type == VKD3DSPR_PATCHCONST
            ? &compiler->patch_constant_signature : &compiler->input_signature;

    signature_element = &shader_signature->elements[element_idx];
    sysval = signature_element->sysval_semantic;
    /* The Vulkan spec does not explicitly forbid passing varyings from the
     * TCS to the TES via builtins. However, Mesa doesn't seem to handle it
     * well, and we don't actually need them to be in builtins. */
    if (compiler->shader_type == VKD3D_SHADER_TYPE_DOMAIN && reg_type != VKD3DSPR_PATCHCONST)
        sysval = VKD3D_SHADER_SV_NONE;

    builtin = get_spirv_builtin_for_sysval(compiler, sysval);

    array_sizes[0] = signature_element->register_count;
    array_sizes[1] = (reg_type == VKD3DSPR_PATCHCONST ? 0 : compiler->input_control_point_count);
    if (array_sizes[0] == 1 && !vsir_sysval_semantic_is_tess_factor(signature_element->sysval_semantic)
            && (!vsir_sysval_semantic_is_clip_cull(signature_element->sysval_semantic) || array_sizes[1]))
    {
        array_sizes[0] = 0;
    }

    write_mask = signature_element->mask;

    if (builtin)
    {
        component_type = builtin->component_type;
        input_component_count = builtin->component_count;
        component_idx = 0;
    }
    else
    {
        component_type = signature_element->component_type;
        input_component_count = vsir_write_mask_component_count(signature_element->mask);
        component_idx = vsir_write_mask_get_component_idx(signature_element->mask);
    }

    if (needs_private_io_variable(builtin))
    {
        use_private_var = true;
        reg_write_mask = write_mask;
    }
    else
    {
        component_idx = vsir_write_mask_get_component_idx(write_mask);
        reg_write_mask = write_mask >> component_idx;
    }

    storage_class = SpvStorageClassInput;

    vkd3d_symbol_make_io(&reg_symbol, reg_type, element_idx);

    if ((entry = rb_get(&compiler->symbol_table, &reg_symbol)))
    {
        /* Except for vicp there should be one declaration per signature element. Sources of
         * duplicate declarations are: a single register split into multiple declarations having
         * different components, which should have been merged, and declarations in one phase
         * being repeated in another (i.e. vcp/vocp), which should have been deleted. */
        if (reg_type != VKD3DSPR_INPUT || !is_in_fork_or_join_phase(compiler))
            FIXME("Duplicate input definition found.\n");
        symbol = RB_ENTRY_VALUE(entry, struct vkd3d_symbol, entry);
        return symbol->id;
    }

    if (builtin)
    {
        input_id = spirv_compiler_emit_builtin_variable_v(compiler, builtin, storage_class, array_sizes, 2);
        if (reg_type == VKD3DSPR_PATCHCONST)
            vkd3d_spirv_build_op_decorate(builder, input_id, SpvDecorationPatch, NULL, 0);
    }
    else
    {
        unsigned int location = signature_element->target_location;

        input_id = spirv_compiler_emit_array_variable(compiler, &builder->global_stream,
                storage_class, component_type, input_component_count, array_sizes, 2);
        vkd3d_spirv_add_iface_variable(builder, input_id);
        if (reg_type == VKD3DSPR_PATCHCONST)
        {
            vkd3d_spirv_build_op_decorate(builder, input_id, SpvDecorationPatch, NULL, 0);
            location += shader_signature_next_location(&compiler->input_signature);
        }
        vkd3d_spirv_build_op_decorate1(builder, input_id, SpvDecorationLocation, location);
        if (component_idx)
            vkd3d_spirv_build_op_decorate1(builder, input_id, SpvDecorationComponent, component_idx);

        spirv_compiler_emit_interpolation_decorations(compiler, component_type, input_id,
                signature_element->interpolation_mode);
    }

    var_id = input_id;
    if (use_private_var)
    {
        storage_class = SpvStorageClassPrivate;
        var_id = spirv_compiler_emit_array_variable(compiler, &builder->global_stream,
                storage_class, VKD3D_SHADER_COMPONENT_FLOAT, VKD3D_VEC4_SIZE, array_sizes, 2);
    }

    vkd3d_symbol_set_register_info(&reg_symbol, var_id, storage_class,
            use_private_var ? VKD3D_SHADER_COMPONENT_FLOAT : component_type,
            use_private_var ? VKD3DSP_WRITEMASK_ALL : reg_write_mask);
    reg_symbol.info.reg.is_aggregate = array_sizes[0] || array_sizes[1];
    VKD3D_ASSERT(!builtin || !builtin->spirv_array_size || use_private_var || array_sizes[0] || array_sizes[1]);
    spirv_compiler_put_symbol(compiler, &reg_symbol);

    vkd3d_spirv_build_op_name(builder, var_id, reg_type == VKD3DSPR_PATCHCONST ? "vpc%u" : "v%u", element_idx);

    if (use_private_var)
    {
        struct vkd3d_shader_register dst_reg;

        vsir_register_init(&dst_reg, reg_type, VKD3D_DATA_FLOAT, 1);
        dst_reg.idx[0].offset = element_idx;

        type_id = vkd3d_spirv_get_type_id(builder, component_type, input_component_count);

        val_id = vkd3d_spirv_build_op_load(builder, type_id, input_id, SpvMemoryAccessMaskNone);

        if (builtin && builtin->fixup_pfn)
            val_id = builtin->fixup_pfn(compiler, val_id);

        if (component_type != VKD3D_SHADER_COMPONENT_FLOAT)
        {
            float_type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_FLOAT, input_component_count);
            val_id = vkd3d_spirv_build_op_bitcast(builder, float_type_id, val_id);
        }

        val_id = spirv_compiler_emit_swizzle(compiler, val_id,
                vkd3d_write_mask_from_component_count(input_component_count),
                VKD3D_SHADER_COMPONENT_FLOAT, VKD3D_SHADER_NO_SWIZZLE, signature_element->mask >> component_idx);

        spirv_compiler_emit_store_reg(compiler, &dst_reg, signature_element->mask, val_id);
    }

    return input_id;
}

static void spirv_compiler_emit_input_register(struct spirv_compiler *compiler,
        const struct vkd3d_shader_dst_param *dst)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_register *reg = &dst->reg;
    const struct vkd3d_spirv_builtin *builtin;
    struct vkd3d_symbol reg_symbol;
    struct rb_entry *entry;
    uint32_t write_mask;
    uint32_t input_id;

    VKD3D_ASSERT(!reg->idx_count || !reg->idx[0].rel_addr);
    VKD3D_ASSERT(reg->idx_count < 2);

    if (!(builtin = get_spirv_builtin_for_register(reg->type)))
    {
        FIXME("Unhandled register %#x.\n", reg->type);
        return;
    }

    /* vPrim may be declared in multiple hull shader phases. */
    vkd3d_symbol_make_register(&reg_symbol, reg);
    if ((entry = rb_get(&compiler->symbol_table, &reg_symbol)))
        return;

    input_id = spirv_compiler_emit_builtin_variable(compiler, builtin, SpvStorageClassInput, 0);

    write_mask = vkd3d_write_mask_from_component_count(builtin->component_count);
    vkd3d_symbol_set_register_info(&reg_symbol, input_id,
            SpvStorageClassInput, builtin->component_type, write_mask);
    reg_symbol.info.reg.is_aggregate = builtin->spirv_array_size;
    spirv_compiler_put_symbol(compiler, &reg_symbol);
    spirv_compiler_emit_register_debug_name(builder, input_id, reg);
}

static unsigned int get_shader_output_swizzle(const struct spirv_compiler *compiler,
        unsigned int register_idx)
{
    const struct vkd3d_shader_spirv_target_info *info;

    if (!(info = compiler->spirv_target_info))
        return VKD3D_SHADER_NO_SWIZZLE;
    if (register_idx >= info->output_swizzle_count)
        return VKD3D_SHADER_NO_SWIZZLE;
    return info->output_swizzles[register_idx];
}

static bool is_dual_source_blending(const struct spirv_compiler *compiler)
{
    const struct vkd3d_shader_spirv_target_info *info = compiler->spirv_target_info;

    return compiler->shader_type == VKD3D_SHADER_TYPE_PIXEL && info && info->dual_source_blending;
}

static void calculate_clip_or_cull_distance_mask(const struct signature_element *e, uint32_t *mask)
{
    unsigned int write_mask;

    if (e->semantic_index >= sizeof(*mask) * CHAR_BIT / VKD3D_VEC4_SIZE)
    {
        FIXME("Invalid semantic index %u for clip/cull distance.\n", e->semantic_index);
        return;
    }

    write_mask = e->mask >> vsir_write_mask_get_component_idx(e->mask);
    *mask |= (write_mask & VKD3DSP_WRITEMASK_ALL) << (VKD3D_VEC4_SIZE * e->semantic_index);
}

/* Emits arrayed SPIR-V built-in variables. */
static void spirv_compiler_emit_shader_signature_outputs(struct spirv_compiler *compiler)
{
    const struct shader_signature *output_signature = &compiler->output_signature;
    uint32_t clip_distance_mask = 0, clip_distance_id = 0;
    uint32_t cull_distance_mask = 0, cull_distance_id = 0;
    const struct vkd3d_spirv_builtin *builtin;
    unsigned int i, count;

    for (i = 0; i < output_signature->element_count; ++i)
    {
        const struct signature_element *e = &output_signature->elements[i];

        switch (e->sysval_semantic)
        {
            case VKD3D_SHADER_SV_CLIP_DISTANCE:
                calculate_clip_or_cull_distance_mask(e, &clip_distance_mask);
                break;

            case VKD3D_SHADER_SV_CULL_DISTANCE:
                calculate_clip_or_cull_distance_mask(e, &cull_distance_mask);
                break;

            default:
                break;
        }
    }

    if (clip_distance_mask)
    {
        count = vkd3d_popcount(clip_distance_mask);
        builtin = get_spirv_builtin_for_sysval(compiler, VKD3D_SHADER_SV_CLIP_DISTANCE);
        clip_distance_id = spirv_compiler_emit_builtin_variable(compiler,
                builtin, SpvStorageClassOutput, count);
    }

    if (cull_distance_mask)
    {
        count = vkd3d_popcount(cull_distance_mask);
        builtin = get_spirv_builtin_for_sysval(compiler, VKD3D_SHADER_SV_CULL_DISTANCE);
        cull_distance_id = spirv_compiler_emit_builtin_variable(compiler,
                builtin, SpvStorageClassOutput, count);
    }

    for (i = 0; i < output_signature->element_count; ++i)
    {
        const struct signature_element *e = &output_signature->elements[i];

        switch (e->sysval_semantic)
        {
            case VKD3D_SHADER_SV_CLIP_DISTANCE:
                compiler->output_info[i].id = clip_distance_id;
                compiler->output_info[i].component_type = VKD3D_SHADER_COMPONENT_FLOAT;
                compiler->output_info[i].array_element_mask = clip_distance_mask;
                break;

            case VKD3D_SHADER_SV_CULL_DISTANCE:
                compiler->output_info[i].id = cull_distance_id;
                compiler->output_info[i].component_type = VKD3D_SHADER_COMPONENT_FLOAT;
                compiler->output_info[i].array_element_mask = cull_distance_mask;
                break;

            default:
                break;
        }
    }
}

static void spirv_compiler_emit_output_register(struct spirv_compiler *compiler,
        const struct vkd3d_shader_dst_param *dst)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_register *reg = &dst->reg;
    const struct vkd3d_spirv_builtin *builtin;
    struct vkd3d_symbol reg_symbol;
    uint32_t write_mask;
    uint32_t output_id;

    VKD3D_ASSERT(!reg->idx_count || !reg->idx[0].rel_addr);
    VKD3D_ASSERT(reg->idx_count < 2);

    if (reg->type == VKD3DSPR_RASTOUT && reg->idx[0].offset == VSIR_RASTOUT_POINT_SIZE)
    {
        builtin = &vkd3d_output_point_size_builtin;
    }
    else if (!(builtin = get_spirv_builtin_for_register(reg->type)))
    {
        FIXME("Unhandled register %#x.\n", reg->type);
        return;
    }

    output_id = spirv_compiler_emit_builtin_variable(compiler, builtin, SpvStorageClassOutput, 0);

    vkd3d_symbol_make_register(&reg_symbol, reg);
    write_mask = vkd3d_write_mask_from_component_count(builtin->component_count);
    vkd3d_symbol_set_register_info(&reg_symbol, output_id,
            SpvStorageClassOutput, builtin->component_type, write_mask);
    reg_symbol.info.reg.is_aggregate = builtin->spirv_array_size;
    spirv_compiler_put_symbol(compiler, &reg_symbol);
    spirv_compiler_emit_register_execution_mode(compiler, reg->type);
    spirv_compiler_emit_register_debug_name(builder, output_id, reg);
}

static uint32_t spirv_compiler_emit_shader_phase_builtin_variable(struct spirv_compiler *compiler,
        const struct vkd3d_spirv_builtin *builtin, const unsigned int *array_sizes, unsigned int size_count)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t *variable_id, id;

    variable_id = NULL;

    if (builtin->spirv_builtin == SpvBuiltInTessLevelOuter)
        variable_id = &compiler->hs.tess_level_outer_id;
    else if (builtin->spirv_builtin == SpvBuiltInTessLevelInner)
        variable_id = &compiler->hs.tess_level_inner_id;

    if (variable_id && *variable_id)
        return *variable_id;

    id = spirv_compiler_emit_builtin_variable_v(compiler, builtin, SpvStorageClassOutput, array_sizes, size_count);
    if (is_in_fork_or_join_phase(compiler))
        vkd3d_spirv_build_op_decorate(builder, id, SpvDecorationPatch, NULL, 0);

    if (variable_id)
        *variable_id = id;
    return id;
}

static void spirv_compiler_emit_output(struct spirv_compiler *compiler,
        enum vkd3d_shader_register_type reg_type, unsigned int element_idx)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    unsigned int component_idx, output_component_count;
    const struct signature_element *signature_element;
    enum vkd3d_shader_component_type component_type;
    const struct shader_signature *shader_signature;
    const struct vkd3d_spirv_builtin *builtin;
    enum vkd3d_shader_sysval_semantic sysval;
    uint32_t write_mask;
    bool use_private_variable = false;
    struct vkd3d_symbol reg_symbol;
    SpvStorageClass storage_class;
    unsigned int array_sizes[2];
    bool is_patch_constant;
    uint32_t id, var_id;

    is_patch_constant = (reg_type == VKD3DSPR_PATCHCONST);

    shader_signature = is_patch_constant ? &compiler->patch_constant_signature : &compiler->output_signature;

    signature_element = &shader_signature->elements[element_idx];
    sysval = signature_element->sysval_semantic;
    /* Don't use builtins for TCS -> TES varyings. See spirv_compiler_emit_input(). */
    if (compiler->shader_type == VKD3D_SHADER_TYPE_HULL && !is_patch_constant)
        sysval = VKD3D_SHADER_SV_NONE;
    array_sizes[0] = signature_element->register_count;
    array_sizes[1] = (reg_type == VKD3DSPR_PATCHCONST ? 0 : compiler->output_control_point_count);
    if (array_sizes[0] == 1 && !vsir_sysval_semantic_is_tess_factor(signature_element->sysval_semantic))
        array_sizes[0] = 0;

    builtin = vkd3d_get_spirv_builtin(compiler, reg_type, sysval);

    write_mask = signature_element->mask;

    component_idx = vsir_write_mask_get_component_idx(write_mask);
    output_component_count = vsir_write_mask_component_count(write_mask);
    if (builtin)
    {
        component_type = builtin->component_type;
        if (!builtin->spirv_array_size)
            output_component_count = builtin->component_count;
    }
    else
    {
        component_type = signature_element->component_type;
    }

    storage_class = SpvStorageClassOutput;

    if (needs_private_io_variable(builtin))
        use_private_variable = true;

    if (!is_patch_constant
            && (get_shader_output_swizzle(compiler, signature_element->register_index) != VKD3D_SHADER_NO_SWIZZLE
            || (compiler->output_info[element_idx].id && compiler->output_info[element_idx].array_element_mask)))
    {
        use_private_variable = true;
    }

    vkd3d_symbol_make_io(&reg_symbol, reg_type, element_idx);

    if (rb_get(&compiler->symbol_table, &reg_symbol))
    {
        /* See spirv_compiler_emit_input() for possible causes. */
        FIXME("Duplicate output definition found.\n");
        return;
    }

    if (!is_patch_constant && compiler->output_info[element_idx].id)
    {
        id = compiler->output_info[element_idx].id;
    }
    else if (builtin)
    {
        if (spirv_compiler_get_current_shader_phase(compiler))
            id = spirv_compiler_emit_shader_phase_builtin_variable(compiler, builtin, array_sizes, 2);
        else
            id = spirv_compiler_emit_builtin_variable_v(compiler, builtin, storage_class, array_sizes, 2);

        spirv_compiler_emit_register_execution_mode(compiler, reg_type);
    }
    else if (signature_element->target_location == SIGNATURE_TARGET_LOCATION_UNUSED)
    {
        storage_class = SpvStorageClassPrivate;
        id = spirv_compiler_emit_array_variable(compiler, &builder->global_stream,
                storage_class, component_type, output_component_count, array_sizes, 2);
    }
    else
    {
        unsigned int location = signature_element->target_location;

        if (is_patch_constant)
            location += shader_signature_next_location(&compiler->output_signature);
        else if (compiler->shader_type == VKD3D_SHADER_TYPE_PIXEL
                && signature_element->sysval_semantic == VKD3D_SHADER_SV_TARGET)
            location = signature_element->semantic_index;

        id = spirv_compiler_emit_array_variable(compiler, &builder->global_stream,
                storage_class, component_type, output_component_count, array_sizes, 2);
        vkd3d_spirv_add_iface_variable(builder, id);

        if (is_dual_source_blending(compiler) && location < 2)
        {
            vkd3d_spirv_build_op_decorate1(builder, id, SpvDecorationLocation, 0);
            vkd3d_spirv_build_op_decorate1(builder, id, SpvDecorationIndex, location);
        }
        else
        {
            vkd3d_spirv_build_op_decorate1(builder, id, SpvDecorationLocation, location);
        }

        if (component_idx)
            vkd3d_spirv_build_op_decorate1(builder, id, SpvDecorationComponent, component_idx);
    }

    if (is_patch_constant)
        vkd3d_spirv_build_op_decorate(builder, id, SpvDecorationPatch, NULL, 0);

    spirv_compiler_decorate_xfb_output(compiler, id, output_component_count, signature_element);

    if (!is_patch_constant)
    {
        compiler->output_info[element_idx].id = id;
        compiler->output_info[element_idx].component_type = component_type;
    }

    var_id = id;
    if (use_private_variable)
    {
        storage_class = SpvStorageClassPrivate;
        var_id = spirv_compiler_emit_variable(compiler, &builder->global_stream,
                storage_class, VKD3D_SHADER_COMPONENT_FLOAT, VKD3D_VEC4_SIZE);
    }

    vkd3d_symbol_set_register_info(&reg_symbol, var_id, storage_class,
            use_private_variable ? VKD3D_SHADER_COMPONENT_FLOAT : component_type,
            use_private_variable ? VKD3DSP_WRITEMASK_ALL : write_mask);
    reg_symbol.info.reg.is_aggregate = array_sizes[0] || array_sizes[1];
    VKD3D_ASSERT(!builtin || !builtin->spirv_array_size || use_private_variable || array_sizes[0] || array_sizes[1]);

    spirv_compiler_put_symbol(compiler, &reg_symbol);

    vkd3d_spirv_build_op_name(builder, var_id, reg_type == VKD3DSPR_PATCHCONST ? "vpc%u" : "o%u", element_idx);

    if (use_private_variable)
    {
        compiler->private_output_variable[element_idx] = var_id;
        compiler->private_output_variable_write_mask[element_idx] |= write_mask >> component_idx;
        if (!compiler->epilogue_function_id)
            compiler->epilogue_function_id = vkd3d_spirv_alloc_id(builder);
    }
}

static uint32_t spirv_compiler_get_output_array_index(struct spirv_compiler *compiler,
        const struct signature_element *e)
{
    enum vkd3d_shader_sysval_semantic sysval = e->sysval_semantic;
    const struct vkd3d_spirv_builtin *builtin;

    builtin = get_spirv_builtin_for_sysval(compiler, sysval);

    switch (sysval)
    {
        case VKD3D_SHADER_SV_TESS_FACTOR_LINEDEN:
        case VKD3D_SHADER_SV_TESS_FACTOR_LINEDET:
            return builtin->member_idx;
        default:
            return e->semantic_index;
    }
}

static void spirv_compiler_emit_store_shader_output(struct spirv_compiler *compiler,
        const struct shader_signature *signature, const struct signature_element *output,
        const struct vkd3d_shader_output_info *output_info,
        uint32_t output_index_id, uint32_t val_id, uint32_t write_mask)
{
    uint32_t dst_write_mask, use_mask, uninit_mask, swizzle, mask;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t type_id, zero_id, ptr_type_id, chain_id, object_id;
    const struct signature_element *element;
    unsigned int i, index, array_idx;
    uint32_t output_id;

    dst_write_mask = output->mask;
    use_mask = output->used_mask;
    if (!output->sysval_semantic)
    {
        for (i = 0; i < signature->element_count; ++i)
        {
            element = &signature->elements[i];
            if (element->register_index != output->register_index)
                continue;
            if (element->sysval_semantic)
                continue;
            dst_write_mask |= element->mask;
            use_mask |= element->used_mask;
        }
    }
    index = vsir_write_mask_get_component_idx(output->mask);
    dst_write_mask >>= index;
    use_mask >>= index;
    write_mask &= dst_write_mask;

    if (!write_mask)
        return;

    if (output_info->component_type != VKD3D_SHADER_COMPONENT_FLOAT)
    {
        type_id = vkd3d_spirv_get_type_id(builder, output_info->component_type, VKD3D_VEC4_SIZE);
        val_id = vkd3d_spirv_build_op_bitcast(builder, type_id, val_id);
    }

    swizzle = get_shader_output_swizzle(compiler, output->register_index);
    uninit_mask = dst_write_mask & ~use_mask;
    if (uninit_mask)
    {
        /* Set values to 0 for not initialized shader output components. */
        write_mask |= uninit_mask;
        zero_id = spirv_compiler_get_constant_vector(compiler,
                output_info->component_type, VKD3D_VEC4_SIZE, 0);
        val_id = spirv_compiler_emit_vector_shuffle(compiler,
                zero_id, val_id, swizzle, uninit_mask, output_info->component_type,
                vsir_write_mask_component_count(write_mask));
    }
    else
    {
        val_id = spirv_compiler_emit_swizzle(compiler,
                val_id, VKD3DSP_WRITEMASK_ALL, output_info->component_type, swizzle, write_mask);
    }

    output_id = output_info->id;
    if (output_index_id)
    {
        type_id = vkd3d_spirv_get_type_id(builder,
                output_info->component_type, vsir_write_mask_component_count(dst_write_mask));
        ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, SpvStorageClassOutput, type_id);
        output_id = vkd3d_spirv_build_op_access_chain1(builder, ptr_type_id, output_id, output_index_id);
    }

    if (!output_info->array_element_mask)
    {
        spirv_compiler_emit_store(compiler,
                output_id, dst_write_mask, output_info->component_type, SpvStorageClassOutput, write_mask, val_id);
        return;
    }

    type_id = vkd3d_spirv_get_type_id(builder, output_info->component_type, 1);
    ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, SpvStorageClassOutput, type_id);
    mask = output_info->array_element_mask;
    array_idx = spirv_compiler_get_output_array_index(compiler, output);
    mask &= (1u << (array_idx * VKD3D_VEC4_SIZE)) - 1;
    for (i = 0, index = vkd3d_popcount(mask); i < VKD3D_VEC4_SIZE; ++i)
    {
        if (!(write_mask & (VKD3DSP_WRITEMASK_0 << i)))
            continue;

        chain_id = vkd3d_spirv_build_op_access_chain1(builder,
                ptr_type_id, output_id, spirv_compiler_get_constant_uint(compiler, index));
        object_id = spirv_compiler_emit_swizzle(compiler, val_id, write_mask,
                output_info->component_type, VKD3D_SHADER_NO_SWIZZLE, VKD3DSP_WRITEMASK_0 << i);
        spirv_compiler_emit_store(compiler, chain_id, VKD3DSP_WRITEMASK_0,
                output_info->component_type, SpvStorageClassOutput, VKD3DSP_WRITEMASK_0 << i, object_id);
        ++index;
    }
}

static void spirv_compiler_emit_shader_epilogue_function(struct spirv_compiler *compiler)
{
    uint32_t param_type_id[MAX_REG_OUTPUT + 1], param_id[MAX_REG_OUTPUT + 1] = {0};
    uint32_t void_id, type_id, ptr_type_id, function_type_id, function_id;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct shader_signature *signature;
    uint32_t output_index_id = 0;
    bool is_patch_constant;
    unsigned int i, count;

    STATIC_ASSERT(ARRAY_SIZE(compiler->private_output_variable) == ARRAY_SIZE(param_id));
    STATIC_ASSERT(ARRAY_SIZE(compiler->private_output_variable) == ARRAY_SIZE(param_type_id));
    STATIC_ASSERT(ARRAY_SIZE(compiler->private_output_variable) == ARRAY_SIZE(compiler->private_output_variable_write_mask));

    is_patch_constant = is_in_fork_or_join_phase(compiler);

    signature = is_patch_constant ? &compiler->patch_constant_signature : &compiler->output_signature;

    function_id = compiler->epilogue_function_id;

    void_id = vkd3d_spirv_get_op_type_void(builder);
    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_FLOAT, 4);
    ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, SpvStorageClassPrivate, type_id);
    for (i = 0, count = 0; i < ARRAY_SIZE(compiler->private_output_variable); ++i)
    {
        if (compiler->private_output_variable[i])
            param_type_id[count++] = ptr_type_id;
    }
    function_type_id = vkd3d_spirv_get_op_type_function(builder, void_id, param_type_id, count);

    vkd3d_spirv_build_op_function(builder, void_id, function_id,
            SpvFunctionControlMaskNone, function_type_id);

    for (i = 0; i < ARRAY_SIZE(compiler->private_output_variable); ++i)
    {
        if (compiler->private_output_variable[i])
            param_id[i] = vkd3d_spirv_build_op_function_parameter(builder, ptr_type_id);
    }

    vkd3d_spirv_build_op_label(builder, vkd3d_spirv_alloc_id(builder));

    for (i = 0; i < ARRAY_SIZE(compiler->private_output_variable); ++i)
    {
        if (compiler->private_output_variable[i])
            param_id[i] = vkd3d_spirv_build_op_load(builder, type_id, param_id[i], SpvMemoryAccessMaskNone);
    }

    if (is_in_control_point_phase(compiler))
        output_index_id = spirv_compiler_emit_load_invocation_id(compiler);

    for (i = 0; i < signature->element_count; ++i)
    {
        if (!compiler->output_info[i].id)
            continue;

        if (!param_id[i])
            continue;

        spirv_compiler_emit_store_shader_output(compiler, signature,
                &signature->elements[i], &compiler->output_info[i], output_index_id,
                param_id[i], compiler->private_output_variable_write_mask[i]);
    }

    vkd3d_spirv_build_op_return(&compiler->spirv_builder);
    vkd3d_spirv_build_op_function_end(builder);

    memset(compiler->private_output_variable, 0, sizeof(compiler->private_output_variable));
    memset(compiler->private_output_variable_write_mask, 0, sizeof(compiler->private_output_variable_write_mask));
    compiler->epilogue_function_id = 0;
}

static void spirv_compiler_emit_hull_shader_builtins(struct spirv_compiler *compiler)
{
    struct vkd3d_shader_dst_param dst;

    memset(&dst, 0, sizeof(dst));
    vsir_register_init(&dst.reg, VKD3DSPR_OUTPOINTID, VKD3D_DATA_FLOAT, 0);
    dst.write_mask = VKD3DSP_WRITEMASK_0;
    spirv_compiler_emit_input_register(compiler, &dst);
}

static void spirv_compiler_emit_initial_declarations(struct spirv_compiler *compiler)
{
    const struct vkd3d_shader_transform_feedback_info *xfb_info = compiler->xfb_info;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;

    switch (compiler->shader_type)
    {
        case VKD3D_SHADER_TYPE_VERTEX:
            vkd3d_spirv_set_execution_model(builder, SpvExecutionModelVertex);
            break;
        case VKD3D_SHADER_TYPE_HULL:
            vkd3d_spirv_set_execution_model(builder, SpvExecutionModelTessellationControl);
            spirv_compiler_emit_hull_shader_builtins(compiler);
            break;
        case VKD3D_SHADER_TYPE_DOMAIN:
            vkd3d_spirv_set_execution_model(builder, SpvExecutionModelTessellationEvaluation);
            break;
        case VKD3D_SHADER_TYPE_GEOMETRY:
            vkd3d_spirv_set_execution_model(builder, SpvExecutionModelGeometry);
            builder->invocation_count = 1;
            break;
        case VKD3D_SHADER_TYPE_PIXEL:
            vkd3d_spirv_set_execution_model(builder, SpvExecutionModelFragment);
            spirv_compiler_emit_execution_mode(compiler, compiler->fragment_coordinate_origin, NULL, 0);
            break;
        case VKD3D_SHADER_TYPE_COMPUTE:
            vkd3d_spirv_set_execution_model(builder, SpvExecutionModelGLCompute);
            break;
        default:
            ERR("Invalid shader type %#x.\n", compiler->shader_type);
    }

    if (xfb_info && xfb_info->element_count)
    {
        vkd3d_spirv_enable_capability(builder, SpvCapabilityTransformFeedback);
        spirv_compiler_emit_execution_mode(compiler, SpvExecutionModeXfb, NULL, 0);
    }

    if (compiler->shader_type != VKD3D_SHADER_TYPE_HULL)
    {
        vkd3d_spirv_builder_begin_main_function(builder);
    }
}

static size_t spirv_compiler_get_current_function_location(struct spirv_compiler *compiler)
{
    const struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_phase *phase;

    if ((phase = spirv_compiler_get_current_shader_phase(compiler)))
        return phase->function_location;

    return builder->main_function_location;
}

static void spirv_compiler_emit_global_flags(struct spirv_compiler *compiler, enum vsir_global_flags flags)
{
    if (flags & VKD3DSGF_FORCE_EARLY_DEPTH_STENCIL)
    {
        spirv_compiler_emit_execution_mode(compiler, SpvExecutionModeEarlyFragmentTests, NULL, 0);
        flags &= ~VKD3DSGF_FORCE_EARLY_DEPTH_STENCIL;
    }

    if (flags & (VKD3DSGF_ENABLE_DOUBLE_PRECISION_FLOAT_OPS | VKD3DSGF_ENABLE_11_1_DOUBLE_EXTENSIONS))
    {
        if (compiler->features & VKD3D_SHADER_COMPILE_OPTION_FEATURE_FLOAT64)
        {
            vkd3d_spirv_enable_capability(&compiler->spirv_builder, SpvCapabilityFloat64);
        }
        else
        {
            WARN("Unsupported 64-bit float ops.\n");
            spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_UNSUPPORTED_FEATURE,
                    "The target environment does not support 64-bit floating point.");
        }
        flags &= ~(VKD3DSGF_ENABLE_DOUBLE_PRECISION_FLOAT_OPS | VKD3DSGF_ENABLE_11_1_DOUBLE_EXTENSIONS);
    }

    if (flags & VKD3DSGF_ENABLE_INT64)
    {
        if (compiler->features & VKD3D_SHADER_COMPILE_OPTION_FEATURE_INT64)
        {
            vkd3d_spirv_enable_capability(&compiler->spirv_builder, SpvCapabilityInt64);
        }
        else
        {
            WARN("Unsupported 64-bit integer ops.\n");
            spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_UNSUPPORTED_FEATURE,
                    "The target environment does not support 64-bit integers.");
        }
        flags &= ~VKD3DSGF_ENABLE_INT64;
    }

    if (flags & VKD3DSGF_ENABLE_WAVE_INTRINSICS)
    {
        if (!(compiler->features & VKD3D_SHADER_COMPILE_OPTION_FEATURE_WAVE_OPS))
        {
            WARN("Unsupported wave ops.\n");
            spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_UNSUPPORTED_FEATURE,
                    "The target environment does not support wave ops.");
        }
        else if (!spirv_compiler_is_spirv_min_1_3_target(compiler))
        {
            WARN("Wave ops enabled but environment does not support SPIR-V 1.3 or greater.\n");
            spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_UNSUPPORTED_FEATURE,
                    "The target environment uses wave ops but does not support SPIR-V 1.3 or greater.");
        }
        flags &= ~VKD3DSGF_ENABLE_WAVE_INTRINSICS;
    }

    if (flags & ~(VKD3DSGF_REFACTORING_ALLOWED | VKD3DSGF_ENABLE_RAW_AND_STRUCTURED_BUFFERS))
        FIXME("Unhandled global flags %#"PRIx64".\n", (uint64_t)flags);
    else if (flags)
        WARN("Unhandled global flags %#"PRIx64".\n", (uint64_t)flags);
}

static void spirv_compiler_emit_temps(struct spirv_compiler *compiler, uint32_t count)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    size_t function_location;
    unsigned int i;
    uint32_t id;

    function_location = spirv_compiler_get_current_function_location(compiler);
    vkd3d_spirv_begin_function_stream_insertion(builder, function_location);

    VKD3D_ASSERT(!compiler->temp_count);
    compiler->temp_count = count;
    for (i = 0; i < compiler->temp_count; ++i)
    {
        id = spirv_compiler_emit_variable(compiler, &builder->global_stream,
                SpvStorageClassPrivate, VKD3D_SHADER_COMPONENT_FLOAT, VKD3D_VEC4_SIZE);
        if (!i)
            compiler->temp_id = id;
        VKD3D_ASSERT(id == compiler->temp_id + i);

        vkd3d_spirv_build_op_name(builder, id, "r%u", i);
    }

    vkd3d_spirv_end_function_stream_insertion(builder);
}

static void spirv_compiler_allocate_ssa_register_ids(struct spirv_compiler *compiler, unsigned int count)
{
    VKD3D_ASSERT(!compiler->ssa_register_info);
    if (!(compiler->ssa_register_info = vkd3d_calloc(count, sizeof(*compiler->ssa_register_info))))
    {
        ERR("Failed to allocate SSA register value id array, count %u.\n", count);
        spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_OUT_OF_MEMORY,
                "Failed to allocate SSA register value id array of count %u.", count);
    }
    compiler->ssa_register_count = count;
}

static void spirv_compiler_emit_dcl_indexable_temp(struct spirv_compiler *compiler,
          const struct vkd3d_shader_instruction *instruction)
{
    const struct vkd3d_shader_indexable_temp *temp = &instruction->declaration.indexable_temp;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t id, type_id, length_id, ptr_type_id, init_id = 0;
    enum vkd3d_shader_component_type component_type;
    struct vkd3d_shader_register reg;
    struct vkd3d_symbol reg_symbol;
    SpvStorageClass storage_class;
    size_t function_location;

    /* Indexable temps may be used by more than one function in hull shaders, and
     * declarations generally should not occur within VSIR code blocks unless function
     * scope is specified, e.g. DXIL alloca. */
    storage_class = temp->has_function_scope ? SpvStorageClassFunction : SpvStorageClassPrivate;

    vsir_register_init(&reg, VKD3DSPR_IDXTEMP, VKD3D_DATA_FLOAT, 1);
    reg.idx[0].offset = temp->register_idx;

    /* Alignment is supported only in the Kernel execution model and is an optimisation only. */
    if (temp->alignment)
        TRACE("Ignoring alignment %u.\n", temp->alignment);

    function_location = spirv_compiler_get_current_function_location(compiler);
    vkd3d_spirv_begin_function_stream_insertion(builder, function_location);

    component_type = vkd3d_component_type_from_data_type(temp->data_type);
    type_id = vkd3d_spirv_get_type_id(builder, component_type, temp->component_count);
    length_id = spirv_compiler_get_constant_uint(compiler, temp->register_size);
    type_id = vkd3d_spirv_get_op_type_array(builder, type_id, length_id);
    ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, storage_class, type_id);
    if (temp->initialiser)
        init_id = spirv_compiler_emit_constant_array(compiler, temp->initialiser, NULL);
    id = vkd3d_spirv_build_op_variable(builder, &builder->function_stream, ptr_type_id, storage_class, init_id);

    spirv_compiler_emit_register_debug_name(builder, id, &reg);

    vkd3d_spirv_end_function_stream_insertion(builder);

    vkd3d_symbol_make_register(&reg_symbol, &reg);
    vkd3d_symbol_set_register_info(&reg_symbol, id, storage_class,
            component_type, vkd3d_write_mask_from_component_count(temp->component_count));
    spirv_compiler_put_symbol(compiler, &reg_symbol);
}

static void spirv_compiler_emit_push_constant_buffers(struct spirv_compiler *compiler)
{
    unsigned int i, j, count, reg_idx, descriptor_offsets_member_idx = 0;
    const SpvStorageClass storage_class = SpvStorageClassPushConstant;
    uint32_t vec4_id, length_id, struct_id, pointer_type_id, var_id;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    struct vkd3d_symbol reg_symbol;
    uint32_t *member_ids;

    count = !!compiler->offset_info.descriptor_table_count;
    for (i = 0; i < compiler->shader_interface.push_constant_buffer_count; ++i)
    {
        const struct vkd3d_push_constant_buffer_binding *cb = &compiler->push_constants[i];

        if (cb->reg.type)
            ++count;
    }
    if (!count)
        return;

    if (!(member_ids = vkd3d_calloc(count, sizeof(*member_ids))))
        return;

    vec4_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_FLOAT, VKD3D_VEC4_SIZE);

    for (i = 0, j = 0; i < compiler->shader_interface.push_constant_buffer_count; ++i)
    {
        const struct vkd3d_push_constant_buffer_binding *cb = &compiler->push_constants[i];
        if (!cb->reg.type)
            continue;

        length_id = spirv_compiler_get_constant_uint(compiler, cb->size);
        member_ids[j]  = vkd3d_spirv_build_op_type_array(builder, vec4_id, length_id);
        vkd3d_spirv_build_op_decorate1(builder, member_ids[j], SpvDecorationArrayStride, 16);

        ++j;
    }

    if (compiler->offset_info.descriptor_table_count)
    {
        uint32_t type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, 1);
        length_id = spirv_compiler_get_constant_uint(compiler, compiler->offset_info.descriptor_table_count);
        member_ids[j] = vkd3d_spirv_build_op_type_array(builder, type_id, length_id);
        vkd3d_spirv_build_op_decorate1(builder, member_ids[j], SpvDecorationArrayStride, 4);
        descriptor_offsets_member_idx = j;
        compiler->descriptor_offsets_member_id = spirv_compiler_get_constant_uint(compiler, j);
        VKD3D_ASSERT(j == count - 1);
    }

    struct_id = vkd3d_spirv_build_op_type_struct(builder, member_ids, count);
    vkd3d_spirv_build_op_decorate(builder, struct_id, SpvDecorationBlock, NULL, 0);
    vkd3d_spirv_build_op_name(builder, struct_id, "push_cb_struct");
    vkd3d_free(member_ids);

    pointer_type_id = vkd3d_spirv_get_op_type_pointer(builder, storage_class, struct_id);
    var_id = vkd3d_spirv_build_op_variable(builder, &builder->global_stream,
            pointer_type_id, storage_class, 0);
    compiler->push_constants_var_id = var_id;
    vkd3d_spirv_build_op_name(builder, var_id, "push_cb");

    for (i = 0, j = 0; i < compiler->shader_interface.push_constant_buffer_count; ++i)
    {
        const struct vkd3d_push_constant_buffer_binding *cb = &compiler->push_constants[i];
        if (!cb->reg.type)
            continue;

        reg_idx = cb->reg.idx[0].offset;
        vkd3d_spirv_build_op_member_decorate1(builder, struct_id, j,
                SpvDecorationOffset, cb->pc.offset);
        vkd3d_spirv_build_op_member_name(builder, struct_id, j, "cb%u", reg_idx);

        vkd3d_symbol_make_register(&reg_symbol, &cb->reg);
        vkd3d_symbol_set_register_info(&reg_symbol, var_id, storage_class,
                VKD3D_SHADER_COMPONENT_FLOAT, VKD3DSP_WRITEMASK_ALL);
        reg_symbol.info.reg.member_idx = j;
        spirv_compiler_put_symbol(compiler, &reg_symbol);

        ++j;
    }
    if (compiler->offset_info.descriptor_table_count)
    {
        vkd3d_spirv_build_op_member_decorate1(builder, struct_id, descriptor_offsets_member_idx,
                SpvDecorationOffset, compiler->offset_info.descriptor_table_offset);
    }
}

static const struct vkd3d_shader_descriptor_info1 *spirv_compiler_get_descriptor_info(
        struct spirv_compiler *compiler, enum vkd3d_shader_descriptor_type type,
        const struct vkd3d_shader_register_range *range)
{
    const struct vkd3d_shader_scan_descriptor_info1 *descriptor_info = compiler->scan_descriptor_info;
    unsigned int register_last = (range->last == ~0u) ? range->first : range->last;
    const struct vkd3d_shader_descriptor_info1 *d;
    unsigned int i;

    for (i = 0; i < descriptor_info->descriptor_count; ++i)
    {
        d = &descriptor_info->descriptors[i];
        if (d->type == type && d->register_space == range->space && d->register_index <= range->first
                    && (d->count == ~0u || d->count > register_last - d->register_index))
            return d;
    }

    return NULL;
}

struct vkd3d_descriptor_variable_info
{
    const struct vkd3d_symbol *array_symbol;
    unsigned int binding_base_idx;
};

static void spirv_compiler_decorate_descriptor(struct spirv_compiler *compiler,
        uint32_t var_id, bool write_only, bool coherent)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;

    if (write_only)
        vkd3d_spirv_build_op_decorate(builder, var_id, SpvDecorationNonReadable, NULL, 0);
    if (coherent)
        vkd3d_spirv_build_op_decorate(builder, var_id, SpvDecorationCoherent, NULL, 0);
}

static uint32_t spirv_compiler_build_descriptor_variable(struct spirv_compiler *compiler,
        SpvStorageClass storage_class, uint32_t type_id, const struct vkd3d_shader_register *reg,
        const struct vkd3d_shader_register_range *range, enum vkd3d_shader_resource_type resource_type,
        const struct vkd3d_shader_descriptor_info1 *descriptor, bool is_uav_counter,
        struct vkd3d_descriptor_variable_info *var_info)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    struct vkd3d_descriptor_binding_address binding_address;
    struct vkd3d_shader_descriptor_binding binding;
    uint32_t array_type_id, ptr_type_id, var_id;
    bool write_only = false, coherent = false;
    struct vkd3d_symbol symbol;
    struct rb_entry *entry;

    binding = spirv_compiler_get_descriptor_binding(compiler, reg, range,
            resource_type, is_uav_counter, &binding_address);
    var_info->binding_base_idx = binding_address.binding_base_idx;

    if (descriptor->type == VKD3D_SHADER_DESCRIPTOR_TYPE_UAV && !is_uav_counter)
    {
        write_only = !(descriptor->flags & VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_UAV_READ);
        /* ROVs are implicitly globally coherent. */
        coherent = descriptor->uav_flags & (VKD3DSUF_GLOBALLY_COHERENT | VKD3DSUF_RASTERISER_ORDERED_VIEW);
    }

    if (binding.count == 1 && range->first == binding_address.binding_base_idx && range->last != ~0u
            && binding_address.push_constant_index == ~0u)
    {
        ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, storage_class, type_id);
        var_id = vkd3d_spirv_build_op_variable(builder, &builder->global_stream,
                ptr_type_id, storage_class, 0);

        spirv_compiler_emit_descriptor_binding(compiler, var_id, &binding);
        spirv_compiler_emit_register_debug_name(builder, var_id, reg);
        spirv_compiler_decorate_descriptor(compiler, var_id, write_only, coherent);

        var_info->array_symbol = NULL;
        return var_id;
    }

    vkd3d_spirv_enable_capability(builder, SpvCapabilityRuntimeDescriptorArrayEXT);
    array_type_id = vkd3d_spirv_get_op_type_runtime_array(builder, type_id);
    ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, storage_class, array_type_id);

    /* Declare one array variable per Vulkan binding, and use it for
     * all array declarations which map to it. */
    symbol.type = VKD3D_SYMBOL_DESCRIPTOR_ARRAY;
    memset(&symbol.key, 0, sizeof(symbol.key));
    symbol.key.descriptor_array.ptr_type_id = ptr_type_id;
    symbol.key.descriptor_array.set = binding.set;
    symbol.key.descriptor_array.binding = binding.binding;
    symbol.key.descriptor_array.push_constant_index = binding_address.push_constant_index;
    symbol.key.descriptor_array.write_only = write_only;
    symbol.key.descriptor_array.coherent = coherent;
    if ((entry = rb_get(&compiler->symbol_table, &symbol)))
    {
        var_info->array_symbol = RB_ENTRY_VALUE(entry, struct vkd3d_symbol, entry);
        return var_info->array_symbol->id;
    }

    var_id = vkd3d_spirv_build_op_variable(builder, &builder->global_stream,
        ptr_type_id, storage_class, 0);
    spirv_compiler_emit_descriptor_binding(compiler, var_id, &binding);
    spirv_compiler_emit_register_debug_name(builder, var_id, reg);
    spirv_compiler_decorate_descriptor(compiler, var_id, write_only, coherent);

    symbol.id = var_id;
    symbol.descriptor_array = NULL;
    symbol.info.descriptor_array.storage_class = storage_class;
    symbol.info.descriptor_array.contained_type_id = type_id;
    var_info->array_symbol = spirv_compiler_put_symbol(compiler, &symbol);

    return var_id;
}

static void spirv_compiler_emit_cbv_declaration(struct spirv_compiler *compiler,
        const struct vkd3d_shader_register_range *range, const struct vkd3d_shader_descriptor_info1 *descriptor)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t vec4_id, array_type_id, length_id, struct_id, var_id;
    const SpvStorageClass storage_class = SpvStorageClassUniform;
    unsigned int size_in_bytes = descriptor->buffer_size;
    struct vkd3d_push_constant_buffer_binding *push_cb;
    struct vkd3d_descriptor_variable_info var_info;
    struct vkd3d_shader_register reg;
    struct vkd3d_symbol reg_symbol;
    unsigned int size;

    vsir_register_init(&reg, VKD3DSPR_CONSTBUFFER, VKD3D_DATA_FLOAT, 3);
    reg.idx[0].offset = descriptor->register_id;
    reg.idx[1].offset = range->first;
    reg.idx[2].offset = range->last;

    size = align(size_in_bytes, VKD3D_VEC4_SIZE * sizeof(uint32_t));
    size /= VKD3D_VEC4_SIZE * sizeof(uint32_t);

    if ((push_cb = spirv_compiler_find_push_constant_buffer(compiler, range)))
    {
        /* Push constant buffers are handled in
         * spirv_compiler_emit_push_constant_buffers().
         */
        push_cb->reg = reg;
        push_cb->size = size;
        if (size_in_bytes > push_cb->pc.size)
        {
            WARN("Constant buffer size %u exceeds push constant size %u.\n",
                    size_in_bytes, push_cb->pc.size);
        }
        return;
    }

    vec4_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_FLOAT, VKD3D_VEC4_SIZE);
    length_id = spirv_compiler_get_constant_uint(compiler, size);
    array_type_id = vkd3d_spirv_build_op_type_array(builder, vec4_id, length_id);
    vkd3d_spirv_build_op_decorate1(builder, array_type_id, SpvDecorationArrayStride, 16);

    struct_id = vkd3d_spirv_build_op_type_struct(builder, &array_type_id, 1);
    vkd3d_spirv_build_op_decorate(builder, struct_id, SpvDecorationBlock, NULL, 0);
    vkd3d_spirv_build_op_member_decorate1(builder, struct_id, 0, SpvDecorationOffset, 0);
    vkd3d_spirv_build_op_name(builder, struct_id, "cb%u_struct", size);

    var_id = spirv_compiler_build_descriptor_variable(compiler, storage_class, struct_id,
            &reg, range, VKD3D_SHADER_RESOURCE_BUFFER, descriptor, false, &var_info);

    vkd3d_symbol_make_register(&reg_symbol, &reg);
    vkd3d_symbol_set_register_info(&reg_symbol, var_id, storage_class,
            VKD3D_SHADER_COMPONENT_FLOAT, VKD3DSP_WRITEMASK_ALL);
    reg_symbol.descriptor_array = var_info.array_symbol;
    reg_symbol.info.reg.binding_base_idx = var_info.binding_base_idx;
    spirv_compiler_put_symbol(compiler, &reg_symbol);
}

static void spirv_compiler_emit_dcl_immediate_constant_buffer(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    const struct vkd3d_shader_immediate_constant_buffer *icb = instruction->declaration.icb;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t type_id, const_id, ptr_type_id, icb_id;
    struct vkd3d_shader_register reg;
    struct vkd3d_symbol reg_symbol;

    const_id = spirv_compiler_emit_constant_array(compiler, icb, &type_id);
    ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, SpvStorageClassPrivate, type_id);
    icb_id = vkd3d_spirv_build_op_variable(builder, &builder->global_stream,
            ptr_type_id, SpvStorageClassPrivate, const_id);
    vkd3d_spirv_build_op_name(builder, icb_id, "icb");

    /* Set an index count of 2 so vkd3d_symbol_make_register() uses idx[0] as a buffer id. */
    vsir_register_init(&reg, VKD3DSPR_IMMCONSTBUFFER, VKD3D_DATA_FLOAT, 2);
    reg.idx[0].offset = icb->register_idx;
    vkd3d_symbol_make_register(&reg_symbol, &reg);
    vkd3d_symbol_set_register_info(&reg_symbol, icb_id, SpvStorageClassPrivate,
                vkd3d_component_type_from_data_type(icb->data_type),
                vkd3d_write_mask_from_component_count(icb->component_count));
    spirv_compiler_put_symbol(compiler, &reg_symbol);
}

static void spirv_compiler_emit_sampler_declaration(struct spirv_compiler *compiler,
        const struct vkd3d_shader_register_range *range, const struct vkd3d_shader_descriptor_info1 *descriptor)
{
    const SpvStorageClass storage_class = SpvStorageClassUniformConstant;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    struct vkd3d_descriptor_variable_info var_info;
    struct vkd3d_shader_register reg;
    struct vkd3d_symbol reg_symbol;
    uint32_t type_id, var_id;

    vsir_register_init(&reg, VKD3DSPR_SAMPLER, VKD3D_DATA_FLOAT, 1);
    reg.idx[0].offset = descriptor->register_id;

    vkd3d_symbol_make_sampler(&reg_symbol, &reg);
    reg_symbol.info.sampler.range = *range;
    spirv_compiler_put_symbol(compiler, &reg_symbol);

    if (spirv_compiler_has_combined_sampler_for_sampler(compiler, range))
        return;

    type_id = vkd3d_spirv_get_op_type_sampler(builder);
    var_id = spirv_compiler_build_descriptor_variable(compiler, storage_class, type_id,
            &reg, range, VKD3D_SHADER_RESOURCE_NONE, descriptor, false, &var_info);

    vkd3d_symbol_make_register(&reg_symbol, &reg);
    vkd3d_symbol_set_register_info(&reg_symbol, var_id, storage_class,
            VKD3D_SHADER_COMPONENT_FLOAT, VKD3DSP_WRITEMASK_ALL);
    reg_symbol.descriptor_array = var_info.array_symbol;
    reg_symbol.info.reg.binding_base_idx = var_info.binding_base_idx;
    spirv_compiler_put_symbol(compiler, &reg_symbol);
}

static const struct vkd3d_spirv_resource_type *spirv_compiler_enable_resource_type(
        struct spirv_compiler *compiler, enum vkd3d_shader_resource_type resource_type, bool is_uav)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_spirv_resource_type *resource_type_info;

    if (!(resource_type_info = vkd3d_get_spirv_resource_type(resource_type)))
        return NULL;

    if (resource_type_info->capability)
        vkd3d_spirv_enable_capability(builder, resource_type_info->capability);
    if (is_uav && resource_type_info->uav_capability)
        vkd3d_spirv_enable_capability(builder, resource_type_info->uav_capability);

    return resource_type_info;
}

static SpvImageFormat image_format_for_image_read(enum vkd3d_shader_component_type data_type)
{
    /* The following formats are supported by Direct3D 11 hardware for UAV
     * typed loads. A newer hardware may support more formats for UAV typed
     * loads (see StorageImageReadWithoutFormat SPIR-V capability).
     */
    switch (data_type)
    {
        case VKD3D_SHADER_COMPONENT_FLOAT:
            return SpvImageFormatR32f;
        case VKD3D_SHADER_COMPONENT_INT:
            return SpvImageFormatR32i;
        case VKD3D_SHADER_COMPONENT_UINT:
            return SpvImageFormatR32ui;
        default:
            FIXME("Unhandled type %#x.\n", data_type);
            return SpvImageFormatUnknown;
    }
}

static uint32_t spirv_compiler_get_image_type_id(struct spirv_compiler *compiler,
        const struct vkd3d_shader_register *reg, const struct vkd3d_shader_register_range *range,
        const struct vkd3d_spirv_resource_type *resource_type_info, enum vkd3d_shader_component_type data_type,
        bool raw_structured)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_descriptor_info1 *d;
    bool uav_read, uav_atomics;
    uint32_t sampled_type_id;
    SpvImageFormat format;

    format = SpvImageFormatUnknown;
    if (reg->type == VKD3DSPR_UAV)
    {
        d = spirv_compiler_get_descriptor_info(compiler,
                VKD3D_SHADER_DESCRIPTOR_TYPE_UAV, range);
        uav_read = !!(d->flags & VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_UAV_READ);
        uav_atomics = !!(d->flags & VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_UAV_ATOMICS);
        if (raw_structured || uav_atomics || (uav_read && !compiler->uav_read_without_format))
            format = image_format_for_image_read(data_type);
        else if (uav_read)
            vkd3d_spirv_enable_capability(builder, SpvCapabilityStorageImageReadWithoutFormat);
    }

    sampled_type_id = vkd3d_spirv_get_type_id(builder, data_type, 1);
    return vkd3d_spirv_get_op_type_image(builder, sampled_type_id, resource_type_info->dim,
            2, resource_type_info->arrayed, resource_type_info->ms,
            reg->type == VKD3DSPR_UAV ? 2 : 1, format);
}

static void spirv_compiler_emit_combined_sampler_declarations(struct spirv_compiler *compiler,
        const struct vkd3d_shader_register *resource, const struct vkd3d_shader_register_range *resource_range,
        enum vkd3d_shader_resource_type resource_type, enum vkd3d_shader_component_type sampled_type,
        unsigned int structure_stride, bool raw, const struct vkd3d_spirv_resource_type *resource_type_info)
{
    const struct vkd3d_shader_interface_info *shader_interface = &compiler->shader_interface;
    const SpvStorageClass storage_class = SpvStorageClassUniformConstant;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_combined_resource_sampler *current;
    uint32_t image_type_id, type_id, ptr_type_id, var_id;
    enum vkd3d_shader_binding_flag resource_type_flag;
    struct vkd3d_symbol symbol;
    unsigned int i;

    resource_type_flag = resource_type == VKD3D_SHADER_RESOURCE_BUFFER
            ? VKD3D_SHADER_BINDING_FLAG_BUFFER : VKD3D_SHADER_BINDING_FLAG_IMAGE;

    for (i = 0; i < shader_interface->combined_sampler_count; ++i)
    {
        current = &shader_interface->combined_samplers[i];

        if (current->resource_space != resource_range->space || current->resource_index != resource_range->first)
            continue;

        if (!(current->flags & resource_type_flag))
            continue;

        if (!spirv_compiler_check_shader_visibility(compiler, current->shader_visibility))
            continue;

        if (current->binding.count != 1)
        {
            FIXME("Descriptor arrays are not supported.\n");
            spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_INVALID_DESCRIPTOR_BINDING,
                    "Combined descriptor binding for resource %u, space %u, "
                    "and sampler %u, space %u has unsupported ‘count’ %u.",
                    resource_range->first, resource_range->space, current->sampler_index,
                    current->sampler_space, current->binding.count);
        }

        image_type_id = spirv_compiler_get_image_type_id(compiler, resource, resource_range,
                resource_type_info, sampled_type, structure_stride || raw);
        type_id = vkd3d_spirv_get_op_type_sampled_image(builder, image_type_id);

        ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, storage_class, type_id);
        var_id = vkd3d_spirv_build_op_variable(builder, &builder->global_stream,
                ptr_type_id, storage_class, 0);

        spirv_compiler_emit_descriptor_binding(compiler, var_id, &current->binding);

        if (current->sampler_index == VKD3D_SHADER_DUMMY_SAMPLER_INDEX)
            vkd3d_spirv_build_op_name(builder, var_id, "t%u_%u_dummy_sampler", resource_range->space,
                    resource_range->first);
        else
            vkd3d_spirv_build_op_name(builder, var_id, "t%u_%u_s%u_%u", resource_range->space, resource_range->first,
                    current->sampler_space, current->sampler_index);

        vkd3d_symbol_make_combined_sampler(&symbol, resource,
                current->sampler_index == VKD3D_SHADER_DUMMY_SAMPLER_INDEX ? 0 : current->sampler_space,
                current->sampler_index);
        symbol.id = var_id;
        symbol.descriptor_array = NULL;
        symbol.info.resource.range = *resource_range;
        symbol.info.resource.sampled_type = sampled_type;
        symbol.info.resource.type_id = image_type_id;
        symbol.info.resource.resource_type_info = resource_type_info;
        symbol.info.resource.structure_stride = structure_stride;
        symbol.info.resource.raw = raw;
        symbol.info.resource.uav_counter_id = 0;
        symbol.info.resource.uav_counter_array = NULL;
        symbol.info.resource.uav_counter_base_idx = 0;
        spirv_compiler_put_symbol(compiler, &symbol);
    }
}

static void spirv_compiler_emit_resource_declaration(struct spirv_compiler *compiler,
        const struct vkd3d_shader_register_range *range, const struct vkd3d_shader_descriptor_info1 *descriptor)
{
    bool raw = descriptor->flags & VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_RAW_BUFFER;
    enum vkd3d_shader_resource_type resource_type = descriptor->resource_type;
    struct vkd3d_descriptor_variable_info var_info, counter_var_info = {0};
    bool is_uav = descriptor->type == VKD3D_SHADER_DESCRIPTOR_TYPE_UAV;
    unsigned int structure_stride = descriptor->structure_stride / 4;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    SpvStorageClass storage_class = SpvStorageClassUniformConstant;
    uint32_t counter_type_id, type_id, var_id, counter_var_id = 0;
    const struct vkd3d_spirv_resource_type *resource_type_info;
    unsigned int sample_count = descriptor->sample_count;
    enum vkd3d_shader_component_type sampled_type;
    struct vkd3d_symbol resource_symbol;
    struct vkd3d_shader_register reg;

    vsir_register_init(&reg, is_uav ? VKD3DSPR_UAV : VKD3DSPR_RESOURCE, VKD3D_DATA_FLOAT, 1);
    reg.idx[0].offset = descriptor->register_id;

    if (resource_type == VKD3D_SHADER_RESOURCE_TEXTURE_2DMS && sample_count == 1)
        resource_type = VKD3D_SHADER_RESOURCE_TEXTURE_2D;
    else if (resource_type == VKD3D_SHADER_RESOURCE_TEXTURE_2DMSARRAY && sample_count == 1)
        resource_type = VKD3D_SHADER_RESOURCE_TEXTURE_2DARRAY;

    if (!(resource_type_info = spirv_compiler_enable_resource_type(compiler,
            resource_type, is_uav)))
    {
        FIXME("Unrecognized resource type.\n");
        return;
    }

    sampled_type = vkd3d_component_type_from_resource_data_type(descriptor->resource_data_type);

    if (!is_uav && spirv_compiler_has_combined_sampler_for_resource(compiler, range))
    {
        spirv_compiler_emit_combined_sampler_declarations(compiler, &reg, range,
                resource_type, sampled_type, structure_stride, raw, resource_type_info);
        return;
    }

    if (compiler->ssbo_uavs && is_uav && resource_type == VKD3D_SHADER_RESOURCE_BUFFER)
    {
        uint32_t array_type_id, struct_id;

        type_id = vkd3d_spirv_get_type_id(builder, sampled_type, 1);

        array_type_id = vkd3d_spirv_get_op_type_runtime_array(builder, type_id);
        vkd3d_spirv_build_op_decorate1(builder, array_type_id, SpvDecorationArrayStride, 4);

        struct_id = vkd3d_spirv_build_op_type_struct(builder, &array_type_id, 1);
        vkd3d_spirv_build_op_decorate(builder, struct_id, SpvDecorationBufferBlock, NULL, 0);
        vkd3d_spirv_build_op_member_decorate1(builder, struct_id, 0, SpvDecorationOffset, 0);

        type_id = struct_id;
        storage_class = SpvStorageClassUniform;
    }
    else
    {
        type_id = spirv_compiler_get_image_type_id(compiler, &reg, range,
                resource_type_info, sampled_type, structure_stride || raw);
    }

    var_id = spirv_compiler_build_descriptor_variable(compiler, storage_class,
            type_id, &reg, range, resource_type, descriptor, false, &var_info);

    if (is_uav)
    {
        if (descriptor->uav_flags & VKD3DSUF_RASTERISER_ORDERED_VIEW)
        {
            if (compiler->shader_type != VKD3D_SHADER_TYPE_PIXEL)
                spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_UNSUPPORTED_FEATURE,
                        "Rasteriser-ordered views are only supported in fragment shaders.");
            else if (!spirv_compiler_is_target_extension_supported(compiler,
                    VKD3D_SHADER_SPIRV_EXTENSION_EXT_FRAGMENT_SHADER_INTERLOCK))
                spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_UNSUPPORTED_FEATURE,
                        "Cannot enable fragment shader interlock. "
                        "The target environment does not support fragment shader interlock.");
            else
                compiler->use_invocation_interlock = true;
        }

        if (descriptor->flags & VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_UAV_COUNTER)
        {
            VKD3D_ASSERT(structure_stride); /* counters are valid only for structured buffers */

            counter_type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, 1);
            if (spirv_compiler_is_opengl_target(compiler))
            {
                vkd3d_spirv_enable_capability(builder, SpvCapabilityAtomicStorage);
                storage_class = SpvStorageClassAtomicCounter;
                type_id = counter_type_id;
            }
            else if (compiler->ssbo_uavs)
            {
                uint32_t length_id, array_type_id, struct_id;

                length_id = spirv_compiler_get_constant_uint(compiler, 1);
                array_type_id = vkd3d_spirv_build_op_type_array(builder, counter_type_id, length_id);
                vkd3d_spirv_build_op_decorate1(builder, array_type_id, SpvDecorationArrayStride, 4);

                struct_id = vkd3d_spirv_build_op_type_struct(builder, &array_type_id, 1);
                vkd3d_spirv_build_op_decorate(builder, struct_id, SpvDecorationBufferBlock, NULL, 0);
                vkd3d_spirv_build_op_member_decorate1(builder, struct_id, 0, SpvDecorationOffset, 0);

                storage_class = SpvStorageClassUniform;
                type_id = struct_id;
            }

            counter_var_id = spirv_compiler_build_descriptor_variable(compiler, storage_class,
                    type_id, &reg, range, resource_type, descriptor, true, &counter_var_info);
        }
    }

    vkd3d_symbol_make_resource(&resource_symbol, &reg);
    resource_symbol.id = var_id;
    resource_symbol.descriptor_array = var_info.array_symbol;
    resource_symbol.info.resource.range = *range;
    resource_symbol.info.resource.sampled_type = sampled_type;
    resource_symbol.info.resource.type_id = type_id;
    resource_symbol.info.resource.resource_type_info = resource_type_info;
    resource_symbol.info.resource.structure_stride = structure_stride;
    resource_symbol.info.resource.raw = raw;
    resource_symbol.info.resource.binding_base_idx = var_info.binding_base_idx;
    resource_symbol.info.resource.uav_counter_id = counter_var_id;
    resource_symbol.info.resource.uav_counter_array = counter_var_info.array_symbol;
    resource_symbol.info.resource.uav_counter_base_idx = counter_var_info.binding_base_idx;
    spirv_compiler_put_symbol(compiler, &resource_symbol);
}

static void spirv_compiler_emit_workgroup_memory(struct spirv_compiler *compiler,
        const struct vkd3d_shader_register *reg, unsigned int alignment, unsigned int size,
        unsigned int structure_stride, bool zero_init)
{
    uint32_t type_id, array_type_id, length_id, pointer_type_id, var_id, init_id;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const SpvStorageClass storage_class = SpvStorageClassWorkgroup;
    struct vkd3d_symbol reg_symbol;

    /* Alignment is supported only in the Kernel execution model. */
    if (alignment)
        TRACE("Ignoring alignment %u.\n", alignment);

    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, 1);
    length_id = spirv_compiler_get_constant_uint(compiler, size);
    array_type_id = vkd3d_spirv_get_op_type_array(builder, type_id, length_id);

    pointer_type_id = vkd3d_spirv_get_op_type_pointer(builder, storage_class, array_type_id);
    init_id = zero_init ? vkd3d_spirv_get_op_constant_null(builder, array_type_id) : 0;
    var_id = vkd3d_spirv_build_op_variable(builder, &builder->global_stream,
            pointer_type_id, storage_class, init_id);

    spirv_compiler_emit_register_debug_name(builder, var_id, reg);

    vkd3d_symbol_make_register(&reg_symbol, reg);
    vkd3d_symbol_set_register_info(&reg_symbol, var_id, storage_class,
            VKD3D_SHADER_COMPONENT_UINT, VKD3DSP_WRITEMASK_0);
    reg_symbol.info.reg.structure_stride = structure_stride;
    spirv_compiler_put_symbol(compiler, &reg_symbol);
}

static void spirv_compiler_emit_dcl_tgsm_raw(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    const struct vkd3d_shader_tgsm_raw *tgsm_raw = &instruction->declaration.tgsm_raw;
    spirv_compiler_emit_workgroup_memory(compiler, &tgsm_raw->reg.reg, tgsm_raw->alignment,
            tgsm_raw->byte_count / 4, 0, tgsm_raw->zero_init);
}

static void spirv_compiler_emit_dcl_tgsm_structured(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    const struct vkd3d_shader_tgsm_structured *tgsm_structured = &instruction->declaration.tgsm_structured;
    unsigned int stride = tgsm_structured->byte_stride / 4;
    spirv_compiler_emit_workgroup_memory(compiler, &tgsm_structured->reg.reg, tgsm_structured->alignment,
            tgsm_structured->structure_count * stride, stride, tgsm_structured->zero_init);
}

static void spirv_compiler_emit_dcl_input(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    const struct vkd3d_shader_dst_param *dst = &instruction->declaration.dst;

    /* INPUT and PATCHCONST are handled in spirv_compiler_emit_io_declarations().
     * OUTPOINTID is handled in spirv_compiler_emit_hull_shader_builtins(). */
    if (dst->reg.type != VKD3DSPR_INPUT && dst->reg.type != VKD3DSPR_PATCHCONST
            && dst->reg.type != VKD3DSPR_OUTPOINTID)
        spirv_compiler_emit_input_register(compiler, dst);
}

static void spirv_compiler_emit_dcl_output(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    const struct vkd3d_shader_dst_param *dst = &instruction->declaration.dst;

    if (dst->reg.type != VKD3DSPR_OUTPUT && dst->reg.type != VKD3DSPR_PATCHCONST)
        spirv_compiler_emit_output_register(compiler, dst);
}

static void spirv_compiler_emit_dcl_stream(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    unsigned int stream_idx = instruction->src[0].reg.idx[0].offset;

    if (stream_idx)
        FIXME("Multiple streams are not supported yet.\n");
}

static void spirv_compiler_emit_output_vertex_count(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    spirv_compiler_emit_execution_mode1(compiler,
            SpvExecutionModeOutputVertices, instruction->declaration.count);
}

static void spirv_compiler_emit_dcl_input_primitive(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    enum vkd3d_primitive_type primitive_type = instruction->declaration.primitive_type.type;
    SpvExecutionMode mode;

    switch (primitive_type)
    {
        case VKD3D_PT_POINTLIST:
            mode = SpvExecutionModeInputPoints;
            break;
        case VKD3D_PT_LINELIST:
            mode = SpvExecutionModeInputLines;
            break;
        case VKD3D_PT_LINELIST_ADJ:
            mode = SpvExecutionModeInputLinesAdjacency;
            break;
        case VKD3D_PT_TRIANGLELIST:
            mode = SpvExecutionModeTriangles;
            break;
        case VKD3D_PT_TRIANGLELIST_ADJ:
            mode = SpvExecutionModeInputTrianglesAdjacency;
            break;
        default:
            FIXME("Unhandled primitive type %#x.\n", primitive_type);
            return;
    }

    spirv_compiler_emit_execution_mode(compiler, mode, NULL, 0);
}

static void spirv_compiler_emit_point_size(struct spirv_compiler *compiler)
{
    if (compiler->program->has_point_size)
        return;

    /* Set the point size. Point sprites are not supported in d3d10+, but
     * point primitives can still be used with e.g. stream output. Vulkan
     * requires the point size to always be explicitly defined when outputting
     * points.
     *
     * If shaderTessellationAndGeometryPointSize is disabled, we must not write
     * PointSize for tessellation and geometry shaders. In that case the point
     * size defaults to 1.0. */
    if (spirv_compiler_is_opengl_target(compiler) || compiler->shader_type == VKD3D_SHADER_TYPE_VERTEX
            || compiler->write_tess_geom_point_size)
    {
        vkd3d_spirv_build_op_store(&compiler->spirv_builder,
                spirv_compiler_emit_builtin_variable(compiler,
                        &vkd3d_output_point_size_builtin, SpvStorageClassOutput, 0),
                spirv_compiler_get_constant_float(compiler, 1.0f), SpvMemoryAccessMaskNone);
    }
}

static void spirv_compiler_emit_dcl_output_topology(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    enum vkd3d_primitive_type primitive_type = instruction->declaration.primitive_type.type;
    SpvExecutionMode mode;

    switch (primitive_type)
    {
        case VKD3D_PT_POINTLIST:
            mode = SpvExecutionModeOutputPoints;
            compiler->emit_point_size = true;
            break;
        case VKD3D_PT_LINESTRIP:
            mode = SpvExecutionModeOutputLineStrip;
            break;
        case VKD3D_PT_TRIANGLESTRIP:
            mode = SpvExecutionModeOutputTriangleStrip;
            break;
        default:
            ERR("Unexpected primitive type %#x.\n", primitive_type);
            return;
    }

    spirv_compiler_emit_execution_mode(compiler, mode, NULL, 0);
}

static void spirv_compiler_emit_dcl_gs_instances(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    compiler->spirv_builder.invocation_count = instruction->declaration.count;
}

static void spirv_compiler_emit_dcl_tessellator_domain(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    enum vkd3d_tessellator_domain domain = instruction->declaration.tessellator_domain;
    SpvExecutionMode mode;

    if (compiler->shader_type == VKD3D_SHADER_TYPE_HULL && spirv_compiler_is_opengl_target(compiler))
        return;

    switch (domain)
    {
        case VKD3D_TESSELLATOR_DOMAIN_LINE:
            mode = SpvExecutionModeIsolines;
            break;
        case VKD3D_TESSELLATOR_DOMAIN_TRIANGLE:
            mode = SpvExecutionModeTriangles;
            break;
        case VKD3D_TESSELLATOR_DOMAIN_QUAD:
            mode = SpvExecutionModeQuads;
            break;
        default:
            FIXME("Invalid tessellator domain %#x.\n", domain);
            return;
    }

    spirv_compiler_emit_execution_mode(compiler, mode, NULL, 0);
}

static void spirv_compiler_emit_tessellator_output_primitive(struct spirv_compiler *compiler,
        enum vkd3d_shader_tessellator_output_primitive primitive)
{
    SpvExecutionMode mode;

    if (compiler->shader_type == VKD3D_SHADER_TYPE_HULL && spirv_compiler_is_opengl_target(compiler))
        return;

    switch (primitive)
    {
        case VKD3D_SHADER_TESSELLATOR_OUTPUT_POINT:
            mode = SpvExecutionModePointMode;
            break;
        case VKD3D_SHADER_TESSELLATOR_OUTPUT_LINE:
            return;
        case VKD3D_SHADER_TESSELLATOR_OUTPUT_TRIANGLE_CW:
            mode = SpvExecutionModeVertexOrderCw;
            break;
        case VKD3D_SHADER_TESSELLATOR_OUTPUT_TRIANGLE_CCW:
            mode = SpvExecutionModeVertexOrderCcw;
            break;
        default:
            FIXME("Invalid tessellator output primitive %#x.\n", primitive);
            return;
    }

    spirv_compiler_emit_execution_mode(compiler, mode, NULL, 0);
}

static void spirv_compiler_emit_tessellator_partitioning(struct spirv_compiler *compiler,
        enum vkd3d_shader_tessellator_partitioning partitioning)
{
    SpvExecutionMode mode;

    if (compiler->shader_type == VKD3D_SHADER_TYPE_HULL && spirv_compiler_is_opengl_target(compiler))
        return;

    switch (partitioning)
    {
        case VKD3D_SHADER_TESSELLATOR_PARTITIONING_INTEGER:
        case VKD3D_SHADER_TESSELLATOR_PARTITIONING_POW2:
            mode = SpvExecutionModeSpacingEqual;
            break;
        case VKD3D_SHADER_TESSELLATOR_PARTITIONING_FRACTIONAL_ODD:
            mode = SpvExecutionModeSpacingFractionalOdd;
            break;
        case VKD3D_SHADER_TESSELLATOR_PARTITIONING_FRACTIONAL_EVEN:
            mode = SpvExecutionModeSpacingFractionalEven;
            break;
        default:
            FIXME("Invalid tessellator partitioning %#x.\n", partitioning);
            return;
    }

    spirv_compiler_emit_execution_mode(compiler, mode, NULL, 0);
}

static void spirv_compiler_emit_thread_group_size(struct spirv_compiler *compiler,
        const struct vsir_thread_group_size *group_size)
{
    const uint32_t local_size[] = {group_size->x, group_size->y, group_size->z};

    spirv_compiler_emit_execution_mode(compiler,
            SpvExecutionModeLocalSize, local_size, ARRAY_SIZE(local_size));
}

static void spirv_compiler_emit_default_control_point_phase(struct spirv_compiler *compiler);

static void spirv_compiler_leave_shader_phase(struct spirv_compiler *compiler)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;

    if (is_in_control_point_phase(compiler) && compiler->emit_default_control_point_phase)
        spirv_compiler_emit_default_control_point_phase(compiler);

    vkd3d_spirv_build_op_function_end(builder);

    if (is_in_control_point_phase(compiler))
    {
        if (compiler->epilogue_function_id)
        {
            spirv_compiler_emit_shader_phase_name(compiler, compiler->epilogue_function_id, "_epilogue");
            spirv_compiler_emit_shader_epilogue_function(compiler);
        }

        /* Fork and join phases share output registers (patch constants).
         * Control point phase has separate output registers. */
        memset(compiler->private_output_variable, 0, sizeof(compiler->private_output_variable));
        memset(compiler->private_output_variable_write_mask, 0, sizeof(compiler->private_output_variable_write_mask));
    }
}

static void spirv_compiler_enter_shader_phase(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t function_id, void_id, function_type_id;
    struct vkd3d_shader_phase *phase;

    VKD3D_ASSERT(compiler->phase != instruction->opcode);

    if (!is_in_default_phase(compiler))
        spirv_compiler_leave_shader_phase(compiler);

    function_id = vkd3d_spirv_alloc_id(builder);

    void_id = vkd3d_spirv_get_op_type_void(builder);
    function_type_id = vkd3d_spirv_get_op_type_function(builder, void_id, NULL, 0);
    vkd3d_spirv_build_op_function(builder, void_id, function_id,
            SpvFunctionControlMaskNone, function_type_id);

    compiler->phase = instruction->opcode;
    spirv_compiler_emit_shader_phase_name(compiler, function_id, NULL);

    phase = (instruction->opcode == VKD3DSIH_HS_CONTROL_POINT_PHASE)
        ? &compiler->control_point_phase : &compiler->patch_constant_phase;
    phase->function_id = function_id;
    /* The insertion location must be set after the label is emitted. */
    phase->function_location = 0;

    if (instruction->opcode == VKD3DSIH_HS_CONTROL_POINT_PHASE)
        compiler->emit_default_control_point_phase = instruction->flags;
}

static void spirv_compiler_initialise_block(struct spirv_compiler *compiler)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;

    /* Insertion locations must point immediately after the function's initial label. */
    if (compiler->shader_type == VKD3D_SHADER_TYPE_HULL)
    {
        struct vkd3d_shader_phase *phase = (compiler->phase == VKD3DSIH_HS_CONTROL_POINT_PHASE)
            ? &compiler->control_point_phase : &compiler->patch_constant_phase;
        if (!phase->function_location)
            phase->function_location = vkd3d_spirv_stream_current_location(&builder->function_stream);
    }
    else if (!builder->main_function_location)
    {
        builder->main_function_location = vkd3d_spirv_stream_current_location(&builder->function_stream);
    }

    /* I/O declarations can result in emission of fixups, which must occur after the initial label. */
    if (!compiler->prolog_emitted)
    {
        spirv_compiler_emit_main_prolog(compiler);
        spirv_compiler_emit_io_declarations(compiler);
        compiler->prolog_emitted = true;
    }
}

static void spirv_compiler_emit_default_control_point_phase(struct spirv_compiler *compiler)
{
    const struct shader_signature *output_signature = &compiler->output_signature;
    const struct shader_signature *input_signature = &compiler->input_signature;
    uint32_t type_id, output_ptr_type_id, input_id, dst_id, invocation_id;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    enum vkd3d_shader_component_type component_type;
    struct vkd3d_shader_src_param invocation;
    struct vkd3d_shader_register input_reg;
    unsigned int component_count;
    unsigned int i;

    vkd3d_spirv_build_op_label(builder, vkd3d_spirv_alloc_id(builder));
    spirv_compiler_initialise_block(compiler);
    invocation_id = spirv_compiler_emit_load_invocation_id(compiler);

    memset(&invocation, 0, sizeof(invocation));
    vsir_register_init(&invocation.reg, VKD3DSPR_OUTPOINTID, VKD3D_DATA_INT, 0);
    invocation.swizzle = VKD3D_SHADER_NO_SWIZZLE;

    vsir_register_init(&input_reg, VKD3DSPR_INPUT, VKD3D_DATA_FLOAT, 2);
    input_reg.idx[0].offset = 0;
    input_reg.idx[0].rel_addr = &invocation;
    input_reg.idx[1].offset = 0;
    input_id = spirv_compiler_get_register_id(compiler, &input_reg);

    VKD3D_ASSERT(input_signature->element_count == output_signature->element_count);
    for (i = 0; i < output_signature->element_count; ++i)
    {
        const struct signature_element *output = &output_signature->elements[i];
        const struct signature_element *input = &input_signature->elements[i];
        struct vkd3d_shader_register_info output_reg_info;
        struct vkd3d_shader_register output_reg;

        VKD3D_ASSERT(input->mask == output->mask);
        VKD3D_ASSERT(input->component_type == output->component_type);

        input_reg.idx[1].offset = i;
        input_id = spirv_compiler_get_register_id(compiler, &input_reg);

        vsir_register_init(&output_reg, VKD3DSPR_OUTPUT, VKD3D_DATA_FLOAT, 1);
        output_reg.idx[0].offset = i;
        spirv_compiler_get_register_info(compiler, &output_reg, &output_reg_info);

        component_type = output->component_type;
        component_count = vsir_write_mask_component_count(output->mask);
        type_id = vkd3d_spirv_get_type_id(builder, component_type, component_count);
        output_ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, SpvStorageClassOutput, type_id);

        dst_id = vkd3d_spirv_build_op_access_chain1(builder, output_ptr_type_id, output_reg_info.id, invocation_id);

        vkd3d_spirv_build_op_copy_memory(builder, dst_id, input_id, SpvMemoryAccessMaskNone);
    }

    vkd3d_spirv_build_op_return(builder);
}

static void spirv_compiler_emit_barrier(struct spirv_compiler *compiler,
        SpvScope execution_scope, SpvScope memory_scope, SpvMemorySemanticsMask semantics)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t execution_id, memory_id, semantics_id;

    memory_id = spirv_compiler_get_constant_uint(compiler, memory_scope);
    semantics_id = spirv_compiler_get_constant_uint(compiler, semantics);

    if (execution_scope != SpvScopeMax)
    {
        execution_id = spirv_compiler_get_constant_uint(compiler, execution_scope);
        vkd3d_spirv_build_op_control_barrier(builder, execution_id, memory_id, semantics_id);
    }
    else
    {
        vkd3d_spirv_build_op_memory_barrier(builder, memory_id, semantics_id);
    }
}

static void spirv_compiler_emit_hull_shader_barrier(struct spirv_compiler *compiler)
{
    spirv_compiler_emit_barrier(compiler,
            SpvScopeWorkgroup, SpvScopeInvocation, SpvMemorySemanticsMaskNone);
}

static void spirv_compiler_emit_shader_epilogue_invocation(struct spirv_compiler *compiler)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t arguments[MAX_REG_OUTPUT];
    uint32_t void_id, function_id;
    unsigned int i, count;

    if ((function_id = compiler->epilogue_function_id))
    {
        void_id = vkd3d_spirv_get_op_type_void(builder);
        for (i = 0, count = 0; i < ARRAY_SIZE(compiler->private_output_variable); ++i)
        {
            if (compiler->private_output_variable[i])
                arguments[count++] = compiler->private_output_variable[i];
        }

        vkd3d_spirv_build_op_function_call(builder, void_id, function_id, arguments, count);
    }
}

static void spirv_compiler_emit_hull_shader_main(struct spirv_compiler *compiler)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t void_id;

    /* If a patch constant function used descriptor indexing the offsets must be reloaded. */
    memset(compiler->descriptor_offset_ids, 0, compiler->offset_info.descriptor_table_count
            * sizeof(*compiler->descriptor_offset_ids));
    vkd3d_spirv_builder_begin_main_function(builder);
    vkd3d_spirv_build_op_label(builder, vkd3d_spirv_alloc_id(builder));

    void_id = vkd3d_spirv_get_op_type_void(builder);

    vkd3d_spirv_build_op_function_call(builder, void_id, compiler->control_point_phase.function_id, NULL, 0);

    if (compiler->use_vocp)
        spirv_compiler_emit_hull_shader_barrier(compiler);

    /* TODO: only call the patch constant function for invocation 0. The simplest way
     * is to avoid use of private variables there, otherwise we would need a separate
     * patch constant epilogue also only called from invocation 0. */
    vkd3d_spirv_build_op_function_call(builder, void_id, compiler->patch_constant_phase.function_id, NULL, 0);
    spirv_compiler_emit_shader_epilogue_invocation(compiler);
    vkd3d_spirv_build_op_return(builder);
    vkd3d_spirv_build_op_function_end(builder);
}

static SpvOp spirv_compiler_map_alu_instruction(const struct vkd3d_shader_instruction *instruction)
{
    static const struct
    {
        enum vkd3d_shader_opcode opcode;
        SpvOp spirv_op;
    }
    alu_ops[] =
    {
        {VKD3DSIH_ADD,        SpvOpFAdd},
        {VKD3DSIH_AND,        SpvOpBitwiseAnd},
        {VKD3DSIH_BFREV,      SpvOpBitReverse},
        {VKD3DSIH_COUNTBITS,  SpvOpBitCount},
        {VKD3DSIH_DADD,       SpvOpFAdd},
        {VKD3DSIH_DDIV,       SpvOpFDiv},
        {VKD3DSIH_DIV,        SpvOpFDiv},
        {VKD3DSIH_DMUL,       SpvOpFMul},
        {VKD3DSIH_DTOF,       SpvOpFConvert},
        {VKD3DSIH_DTOI,       SpvOpConvertFToS},
        {VKD3DSIH_DTOU,       SpvOpConvertFToU},
        {VKD3DSIH_FREM,       SpvOpFRem},
        {VKD3DSIH_FTOD,       SpvOpFConvert},
        {VKD3DSIH_IADD,       SpvOpIAdd},
        {VKD3DSIH_INEG,       SpvOpSNegate},
        {VKD3DSIH_ISHL,       SpvOpShiftLeftLogical},
        {VKD3DSIH_ISHR,       SpvOpShiftRightArithmetic},
        {VKD3DSIH_ISINF,      SpvOpIsInf},
        {VKD3DSIH_ISNAN,      SpvOpIsNan},
        {VKD3DSIH_ITOD,       SpvOpConvertSToF},
        {VKD3DSIH_ITOF,       SpvOpConvertSToF},
        {VKD3DSIH_ITOI,       SpvOpSConvert},
        {VKD3DSIH_MUL,        SpvOpFMul},
        {VKD3DSIH_NOT,        SpvOpNot},
        {VKD3DSIH_OR,         SpvOpBitwiseOr},
        {VKD3DSIH_USHR,       SpvOpShiftRightLogical},
        {VKD3DSIH_UTOD,       SpvOpConvertUToF},
        {VKD3DSIH_UTOF,       SpvOpConvertUToF},
        {VKD3DSIH_UTOU,       SpvOpUConvert},
        {VKD3DSIH_XOR,        SpvOpBitwiseXor},
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(alu_ops); ++i)
    {
        if (alu_ops[i].opcode == instruction->opcode)
            return alu_ops[i].spirv_op;
    }

    return SpvOpMax;
}

static SpvOp spirv_compiler_map_logical_instruction(const struct vkd3d_shader_instruction *instruction)
{
    switch (instruction->opcode)
    {
        case VKD3DSIH_AND:
            return SpvOpLogicalAnd;
        case VKD3DSIH_OR:
            return SpvOpLogicalOr;
        case VKD3DSIH_XOR:
            return SpvOpLogicalNotEqual;
        default:
            return SpvOpMax;
    }
}

static void spirv_compiler_emit_bool_cast(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t val_id;

    VKD3D_ASSERT(src->reg.data_type == VKD3D_DATA_BOOL && dst->reg.data_type != VKD3D_DATA_BOOL);

    val_id = spirv_compiler_emit_load_src(compiler, src, dst->write_mask);
    if (dst->reg.data_type == VKD3D_DATA_HALF || dst->reg.data_type == VKD3D_DATA_FLOAT)
    {
        val_id = spirv_compiler_emit_bool_to_float(compiler, 1, val_id, instruction->opcode == VKD3DSIH_ITOF);
    }
    else if (dst->reg.data_type == VKD3D_DATA_DOUBLE)
    {
        /* ITOD is not supported. Frontends which emit bool casts must use ITOF for double. */
        val_id = spirv_compiler_emit_bool_to_double(compiler, 1, val_id, instruction->opcode == VKD3DSIH_ITOF);
    }
    else if (dst->reg.data_type == VKD3D_DATA_UINT16 || dst->reg.data_type == VKD3D_DATA_UINT)
    {
        val_id = spirv_compiler_emit_bool_to_int(compiler, 1, val_id, instruction->opcode == VKD3DSIH_ITOI);
    }
    else if (dst->reg.data_type == VKD3D_DATA_UINT64)
    {
        val_id = spirv_compiler_emit_bool_to_int64(compiler, 1, val_id, instruction->opcode == VKD3DSIH_ITOI);
    }
    else
    {
        WARN("Unhandled data type %u.\n", dst->reg.data_type);
        spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_INVALID_TYPE,
                "Register data type %u is unhandled.", dst->reg.data_type);
    }

    spirv_compiler_emit_store_dst(compiler, dst, val_id);
}

static enum vkd3d_result spirv_compiler_emit_alu_instruction(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t src_ids[SPIRV_MAX_SRC_COUNT];
    uint32_t type_id, val_id;
    SpvOp op = SpvOpMax;
    unsigned int i;

    if (src->reg.data_type == VKD3D_DATA_UINT64 && instruction->opcode == VKD3DSIH_COUNTBITS)
    {
        /* At least some drivers support this anyway, but if validation is enabled it will fail. */
        FIXME("Unsupported 64-bit source for bit count.\n");
        spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_NOT_IMPLEMENTED,
                "64-bit source for bit count is not supported.");
        return VKD3D_ERROR_INVALID_SHADER;
    }

    if (src->reg.data_type == VKD3D_DATA_BOOL)
    {
        if (dst->reg.data_type == VKD3D_DATA_BOOL)
        {
            /* VSIR supports logic ops AND/OR/XOR on bool values. */
            op = spirv_compiler_map_logical_instruction(instruction);
        }
        else if (instruction->opcode == VKD3DSIH_ITOF || instruction->opcode == VKD3DSIH_UTOF
                || instruction->opcode == VKD3DSIH_ITOI || instruction->opcode == VKD3DSIH_UTOU)
        {
            /* VSIR supports cast from bool to signed/unsigned integer types and floating point types,
             * where bool is treated as a 1-bit integer and a signed 'true' value converts to -1. */
            spirv_compiler_emit_bool_cast(compiler, instruction);
            return VKD3D_OK;
        }
    }
    else
    {
        op = spirv_compiler_map_alu_instruction(instruction);
    }

    if (op == SpvOpMax)
    {
        ERR("Unexpected instruction %#x.\n", instruction->opcode);
        spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_INVALID_HANDLER,
                "Encountered invalid/unhandled instruction handler %#x.", instruction->opcode);
        return VKD3D_ERROR_INVALID_SHADER;
    }

    VKD3D_ASSERT(instruction->dst_count == 1);
    VKD3D_ASSERT(instruction->src_count <= SPIRV_MAX_SRC_COUNT);

    type_id = spirv_compiler_get_type_id_for_dst(compiler, dst);

    for (i = 0; i < instruction->src_count; ++i)
        src_ids[i] = spirv_compiler_emit_load_src(compiler, &src[i], dst->write_mask);

    /* The SPIR-V specification states, "The resulting value is undefined if
     * Shift is greater than or equal to the bit width of the components of
     * Base." Direct3D applies only the lowest 5 bits of the shift.
     *
     * Microsoft fxc will compile immediate constants larger than 5 bits.
     * Fixing up the constants would be more elegant, but the simplest way is
     * to let this handle constants too. */
    if (!(instruction->flags & VKD3DSI_SHIFT_UNMASKED) && (instruction->opcode == VKD3DSIH_ISHL
            || instruction->opcode == VKD3DSIH_ISHR || instruction->opcode == VKD3DSIH_USHR))
    {
        uint32_t mask_id = spirv_compiler_get_constant_vector(compiler,
                VKD3D_SHADER_COMPONENT_UINT, vsir_write_mask_component_count(dst->write_mask), 0x1f);
        src_ids[1] = vkd3d_spirv_build_op_and(builder, type_id, src_ids[1], mask_id);
    }

    val_id = vkd3d_spirv_build_op_trv(builder, &builder->function_stream, op, type_id,
            src_ids, instruction->src_count);
    if (instruction->flags & VKD3DSI_PRECISE_XYZW)
        vkd3d_spirv_build_op_decorate(builder, val_id, SpvDecorationNoContraction, NULL, 0);

    spirv_compiler_emit_store_dst(compiler, dst, val_id);
    return VKD3D_OK;
}

static void spirv_compiler_emit_isfinite(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t type_id, src_id, isinf_id, isnan_id, val_id;

    type_id = spirv_compiler_get_type_id_for_dst(compiler, dst);
    src_id = spirv_compiler_emit_load_src(compiler, src, dst->write_mask);
    /* OpIsFinite is only available in Kernel mode. */
    isinf_id = vkd3d_spirv_build_op_is_inf(builder, type_id, src_id);
    isnan_id = vkd3d_spirv_build_op_is_nan(builder, type_id, src_id);
    val_id = vkd3d_spirv_build_op_logical_equal(builder, type_id, isinf_id, isnan_id);
    spirv_compiler_emit_store_dst(compiler, dst, val_id);
}

static enum GLSLstd450 spirv_compiler_map_ext_glsl_instruction(
        const struct vkd3d_shader_instruction *instruction)
{
    static const struct
    {
        enum vkd3d_shader_opcode opcode;
        enum GLSLstd450 glsl_inst;
    }
    glsl_insts[] =
    {
        {VKD3DSIH_ABS,             GLSLstd450FAbs},
        {VKD3DSIH_ACOS,            GLSLstd450Acos},
        {VKD3DSIH_ASIN,            GLSLstd450Asin},
        {VKD3DSIH_ATAN,            GLSLstd450Atan},
        {VKD3DSIH_DFMA,            GLSLstd450Fma},
        {VKD3DSIH_DMAX,            GLSLstd450NMax},
        {VKD3DSIH_DMIN,            GLSLstd450NMin},
        {VKD3DSIH_EXP,             GLSLstd450Exp2},
        {VKD3DSIH_FIRSTBIT_HI,     GLSLstd450FindUMsb},
        {VKD3DSIH_FIRSTBIT_LO,     GLSLstd450FindILsb},
        {VKD3DSIH_FIRSTBIT_SHI,    GLSLstd450FindSMsb},
        {VKD3DSIH_FRC,             GLSLstd450Fract},
        {VKD3DSIH_HCOS,            GLSLstd450Cosh},
        {VKD3DSIH_HSIN,            GLSLstd450Sinh},
        {VKD3DSIH_HTAN,            GLSLstd450Tanh},
        {VKD3DSIH_IMAX,            GLSLstd450SMax},
        {VKD3DSIH_IMIN,            GLSLstd450SMin},
        {VKD3DSIH_LOG,             GLSLstd450Log2},
        {VKD3DSIH_MAD,             GLSLstd450Fma},
        {VKD3DSIH_MAX,             GLSLstd450NMax},
        {VKD3DSIH_MIN,             GLSLstd450NMin},
        {VKD3DSIH_ROUND_NE,        GLSLstd450RoundEven},
        {VKD3DSIH_ROUND_NI,        GLSLstd450Floor},
        {VKD3DSIH_ROUND_PI,        GLSLstd450Ceil},
        {VKD3DSIH_ROUND_Z,         GLSLstd450Trunc},
        {VKD3DSIH_RSQ,             GLSLstd450InverseSqrt},
        {VKD3DSIH_SQRT,            GLSLstd450Sqrt},
        {VKD3DSIH_TAN,             GLSLstd450Tan},
        {VKD3DSIH_UMAX,            GLSLstd450UMax},
        {VKD3DSIH_UMIN,            GLSLstd450UMin},
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(glsl_insts); ++i)
    {
        if (glsl_insts[i].opcode == instruction->opcode)
            return glsl_insts[i].glsl_inst;
    }

    return GLSLstd450Bad;
}

static void spirv_compiler_emit_ext_glsl_instruction(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    uint32_t instr_set_id, type_id, val_id, rev_val_id, uint_max_id, condition_id;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t src_id[SPIRV_MAX_SRC_COUNT];
    unsigned int i, component_count;
    enum GLSLstd450 glsl_inst;

    if (src[0].reg.data_type == VKD3D_DATA_UINT64 && (instruction->opcode == VKD3DSIH_FIRSTBIT_HI
            || instruction->opcode == VKD3DSIH_FIRSTBIT_LO || instruction->opcode == VKD3DSIH_FIRSTBIT_SHI))
    {
        /* At least some drivers support this anyway, but if validation is enabled it will fail. */
        FIXME("Unsupported 64-bit source for handler %#x.\n", instruction->opcode);
        spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_NOT_IMPLEMENTED,
                "64-bit source for handler %#x is not supported.", instruction->opcode);
        return;
    }

    glsl_inst = spirv_compiler_map_ext_glsl_instruction(instruction);
    if (glsl_inst == GLSLstd450Bad)
    {
        ERR("Unexpected instruction %#x.\n", instruction->opcode);
        return;
    }

    instr_set_id = vkd3d_spirv_get_glsl_std450_instr_set(builder);

    VKD3D_ASSERT(instruction->dst_count == 1);
    VKD3D_ASSERT(instruction->src_count <= SPIRV_MAX_SRC_COUNT);

    type_id = spirv_compiler_get_type_id_for_dst(compiler, dst);

    for (i = 0; i < instruction->src_count; ++i)
        src_id[i] = spirv_compiler_emit_load_src(compiler, &src[i], dst->write_mask);

    val_id = vkd3d_spirv_build_op_ext_inst(builder, type_id,
            instr_set_id, glsl_inst, src_id, instruction->src_count);

    if (instruction->opcode == VKD3DSIH_FIRSTBIT_HI
            || instruction->opcode == VKD3DSIH_FIRSTBIT_SHI)
    {
        /* In D3D bits are numbered from the most significant bit. */
        component_count = vsir_write_mask_component_count(dst->write_mask);
        uint_max_id = spirv_compiler_get_constant_uint_vector(compiler, UINT32_MAX, component_count);
        condition_id = vkd3d_spirv_build_op_tr2(builder, &builder->function_stream, SpvOpIEqual,
                vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_BOOL, component_count), val_id, uint_max_id);
        rev_val_id = vkd3d_spirv_build_op_isub(builder, type_id,
                spirv_compiler_get_constant_uint_vector(compiler, 31, component_count), val_id);
        val_id = vkd3d_spirv_build_op_select(builder, type_id, condition_id, val_id, rev_val_id);
    }

    spirv_compiler_emit_store_dst(compiler, dst, val_id);
}

static void spirv_compiler_emit_mov(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    uint32_t val_id, dst_val_id, type_id, dst_id, src_id, write_mask32, swizzle32;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    struct vkd3d_shader_register_info dst_reg_info, src_reg_info;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    unsigned int i, component_count, write_mask;
    uint32_t components[VKD3D_VEC4_SIZE];

    if (register_is_constant_or_undef(&src->reg) || src->reg.type == VKD3DSPR_SSA || dst->reg.type == VKD3DSPR_SSA
            || src->reg.type == VKD3DSPR_PARAMETER || dst->modifiers || src->modifiers)
        goto general_implementation;

    spirv_compiler_get_register_info(compiler, &dst->reg, &dst_reg_info);
    spirv_compiler_get_register_info(compiler, &src->reg, &src_reg_info);

    if (dst_reg_info.component_type != src_reg_info.component_type
            || dst_reg_info.write_mask != src_reg_info.write_mask)
        goto general_implementation;

    if (vkd3d_swizzle_is_equal(dst_reg_info.write_mask, src->swizzle, src_reg_info.write_mask))
    {
        dst_id = spirv_compiler_get_register_id(compiler, &dst->reg);
        src_id = spirv_compiler_get_register_id(compiler, &src->reg);

        vkd3d_spirv_build_op_copy_memory(builder, dst_id, src_id, SpvMemoryAccessMaskNone);
        return;
    }

    write_mask32 = data_type_is_64_bit(dst->reg.data_type) ? vsir_write_mask_32_from_64(dst->write_mask) : dst->write_mask;
    swizzle32 = data_type_is_64_bit(src->reg.data_type) ? vsir_swizzle_32_from_64(src->swizzle) : src->swizzle;
    component_count = vsir_write_mask_component_count(write_mask32);
    if (component_count != 1 && component_count != VKD3D_VEC4_SIZE
            && dst_reg_info.write_mask == VKD3DSP_WRITEMASK_ALL)
    {
        dst_id = spirv_compiler_get_register_id(compiler, &dst->reg);
        src_id = spirv_compiler_get_register_id(compiler, &src->reg);

        type_id = vkd3d_spirv_get_type_id(builder, dst_reg_info.component_type, VKD3D_VEC4_SIZE);
        val_id = vkd3d_spirv_build_op_load(builder, type_id, src_id, SpvMemoryAccessMaskNone);
        dst_val_id = vkd3d_spirv_build_op_load(builder, type_id, dst_id, SpvMemoryAccessMaskNone);

        for (i = 0; i < ARRAY_SIZE(components); ++i)
        {
            if (write_mask32 & (VKD3DSP_WRITEMASK_0 << i))
                components[i] = VKD3D_VEC4_SIZE + vsir_swizzle_get_component(swizzle32, i);
            else
                components[i] = i;
        }

        val_id = vkd3d_spirv_build_op_vector_shuffle(builder,
                type_id, dst_val_id, val_id, components, VKD3D_VEC4_SIZE);

        vkd3d_spirv_build_op_store(builder, dst_id, val_id, SpvMemoryAccessMaskNone);
        return;
    }

general_implementation:
    write_mask = dst->write_mask;
    if (src->reg.type == VKD3DSPR_IMMCONST64 && !data_type_is_64_bit(dst->reg.data_type))
        write_mask = vsir_write_mask_64_from_32(write_mask);
    else if (!data_type_is_64_bit(src->reg.data_type) && data_type_is_64_bit(dst->reg.data_type))
        write_mask = vsir_write_mask_32_from_64(write_mask);

    val_id = spirv_compiler_emit_load_src(compiler, src, write_mask);
    if (dst->reg.data_type != src->reg.data_type)
    {
        val_id = vkd3d_spirv_build_op_bitcast(builder, vkd3d_spirv_get_type_id_for_data_type(builder,
                dst->reg.data_type, vsir_write_mask_component_count(dst->write_mask)), val_id);
    }
    spirv_compiler_emit_store_dst(compiler, dst, val_id);
}

static void spirv_compiler_emit_movc(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t condition_id, src1_id, src2_id, type_id, val_id;
    unsigned int component_count;

    condition_id = spirv_compiler_emit_load_src(compiler, &src[0], dst->write_mask);
    src1_id = spirv_compiler_emit_load_src(compiler, &src[1], dst->write_mask);
    src2_id = spirv_compiler_emit_load_src(compiler, &src[2], dst->write_mask);

    component_count = vsir_write_mask_component_count(dst->write_mask);
    type_id = spirv_compiler_get_type_id_for_dst(compiler, dst);

    if (src[0].reg.data_type != VKD3D_DATA_BOOL)
    {
        if (instruction->opcode == VKD3DSIH_CMP)
            condition_id = vkd3d_spirv_build_op_tr2(builder, &builder->function_stream, SpvOpFOrdGreaterThanEqual,
                    vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_BOOL, component_count), condition_id,
                    spirv_compiler_get_constant_float_vector(compiler, 0.0f, component_count));
        else
            condition_id = spirv_compiler_emit_int_to_bool(compiler,
                    VKD3D_SHADER_CONDITIONAL_OP_NZ, src[0].reg.data_type, component_count, condition_id);
    }
    val_id = vkd3d_spirv_build_op_select(builder, type_id, condition_id, src1_id, src2_id);

    spirv_compiler_emit_store_dst(compiler, dst, val_id);
}

static void spirv_compiler_emit_swapc(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t condition_id, src1_id, src2_id, type_id, val_id;
    unsigned int component_count;

    VKD3D_ASSERT(dst[0].write_mask == dst[1].write_mask);

    condition_id = spirv_compiler_emit_load_src(compiler, &src[0], dst->write_mask);
    src1_id = spirv_compiler_emit_load_src(compiler, &src[1], dst->write_mask);
    src2_id = spirv_compiler_emit_load_src(compiler, &src[2], dst->write_mask);

    component_count = vsir_write_mask_component_count(dst->write_mask);
    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_FLOAT, component_count);

    condition_id = spirv_compiler_emit_int_to_bool(compiler,
            VKD3D_SHADER_CONDITIONAL_OP_NZ, src[0].reg.data_type, component_count, condition_id);

    val_id = vkd3d_spirv_build_op_select(builder, type_id, condition_id, src2_id, src1_id);
    spirv_compiler_emit_store_dst(compiler, &dst[0], val_id);
    val_id = vkd3d_spirv_build_op_select(builder, type_id, condition_id, src1_id, src2_id);
    spirv_compiler_emit_store_dst(compiler, &dst[1], val_id);
}

static void spirv_compiler_emit_dot(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    enum vkd3d_shader_component_type component_type;
    uint32_t type_id, val_id, src_ids[2];
    unsigned int component_count, i;
    uint32_t write_mask;

    component_count = vsir_write_mask_component_count(dst->write_mask);
    component_type = vkd3d_component_type_from_data_type(dst->reg.data_type);

    if (instruction->opcode == VKD3DSIH_DP4)
        write_mask = VKD3DSP_WRITEMASK_ALL;
    else if (instruction->opcode == VKD3DSIH_DP3)
        write_mask = VKD3DSP_WRITEMASK_0 | VKD3DSP_WRITEMASK_1 | VKD3DSP_WRITEMASK_2;
    else
        write_mask = VKD3DSP_WRITEMASK_0 | VKD3DSP_WRITEMASK_1;

    VKD3D_ASSERT(instruction->src_count == ARRAY_SIZE(src_ids));
    for (i = 0; i < ARRAY_SIZE(src_ids); ++i)
        src_ids[i] = spirv_compiler_emit_load_src(compiler, &src[i], write_mask);

    type_id = vkd3d_spirv_get_type_id(builder, component_type, 1);

    val_id = vkd3d_spirv_build_op_tr2(builder, &builder->function_stream,
            SpvOpDot, type_id, src_ids[0], src_ids[1]);
    if (component_count > 1)
    {
        val_id = spirv_compiler_emit_construct_vector(compiler,
                component_type, component_count, val_id, 0, 1);
    }
    if (instruction->flags & VKD3DSI_PRECISE_XYZW)
        vkd3d_spirv_build_op_decorate(builder, val_id, SpvDecorationNoContraction, NULL, 0);

    spirv_compiler_emit_store_dst(compiler, dst, val_id);
}

static void spirv_compiler_emit_rcp(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t type_id, src_id, val_id, div_id;
    unsigned int component_count;

    component_count = vsir_write_mask_component_count(dst->write_mask);
    type_id = spirv_compiler_get_type_id_for_dst(compiler, dst);

    src_id = spirv_compiler_emit_load_src(compiler, src, dst->write_mask);
    if (src->reg.data_type == VKD3D_DATA_DOUBLE)
        div_id = spirv_compiler_get_constant_double_vector(compiler, 1.0, component_count);
    else
        div_id = spirv_compiler_get_constant_float_vector(compiler, 1.0f, component_count);
    val_id = vkd3d_spirv_build_op_fdiv(builder, type_id, div_id, src_id);
    spirv_compiler_emit_store_dst(compiler, dst, val_id);
}

static void spirv_compiler_emit_sincos(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    const struct vkd3d_shader_dst_param *dst_sin = &instruction->dst[0];
    const struct vkd3d_shader_dst_param *dst_cos = &instruction->dst[1];
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t type_id, src_id, sin_id = 0, cos_id = 0;

    if (dst_sin->reg.type != VKD3DSPR_NULL)
    {
        type_id = spirv_compiler_get_type_id_for_dst(compiler, dst_sin);
        src_id = spirv_compiler_emit_load_src(compiler, src, dst_sin->write_mask);

        sin_id = vkd3d_spirv_build_op_glsl_std450_sin(builder, type_id, src_id);
    }

    if (dst_cos->reg.type != VKD3DSPR_NULL)
    {
        if (dst_sin->reg.type == VKD3DSPR_NULL || dst_cos->write_mask != dst_sin->write_mask)
        {
            type_id = spirv_compiler_get_type_id_for_dst(compiler, dst_cos);
            src_id = spirv_compiler_emit_load_src(compiler, src, dst_cos->write_mask);
        }

        cos_id = vkd3d_spirv_build_op_glsl_std450_cos(builder, type_id, src_id);
    }

    if (sin_id)
        spirv_compiler_emit_store_dst(compiler, dst_sin, sin_id);

    if (cos_id)
        spirv_compiler_emit_store_dst(compiler, dst_cos, cos_id);
}

static void spirv_compiler_emit_imul(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t type_id, val_id, src0_id, src1_id;

    if (dst[0].reg.type != VKD3DSPR_NULL)
        FIXME("Extended multiplies not implemented.\n"); /* SpvOpSMulExtended/SpvOpUMulExtended */

    if (dst[1].reg.type == VKD3DSPR_NULL)
        return;

    type_id = spirv_compiler_get_type_id_for_dst(compiler, &dst[1]);

    src0_id = spirv_compiler_emit_load_src(compiler, &src[0], dst[1].write_mask);
    src1_id = spirv_compiler_emit_load_src(compiler, &src[1], dst[1].write_mask);

    val_id = vkd3d_spirv_build_op_imul(builder, type_id, src0_id, src1_id);

    spirv_compiler_emit_store_dst(compiler, &dst[1], val_id);
}

static void spirv_compiler_emit_imad(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t type_id, val_id, src_ids[3];
    unsigned int i, component_count;

    component_count = vsir_write_mask_component_count(dst->write_mask);
    type_id = vkd3d_spirv_get_type_id_for_data_type(builder, dst->reg.data_type, component_count);

    for (i = 0; i < ARRAY_SIZE(src_ids); ++i)
        src_ids[i] = spirv_compiler_emit_load_src(compiler, &src[i], dst->write_mask);

    val_id = vkd3d_spirv_build_op_imul(builder, type_id, src_ids[0], src_ids[1]);
    val_id = vkd3d_spirv_build_op_iadd(builder, type_id, val_id, src_ids[2]);

    spirv_compiler_emit_store_dst(compiler, dst, val_id);
}

static void spirv_compiler_emit_int_div(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    uint32_t type_id, val_id, src0_id, src1_id, condition_id, uint_max_id;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    unsigned int component_count = 0;
    SpvOp div_op, mod_op;

    div_op = instruction->opcode == VKD3DSIH_IDIV ? SpvOpSDiv : SpvOpUDiv;
    mod_op = instruction->opcode == VKD3DSIH_IDIV ? SpvOpSRem : SpvOpUMod;

    if (dst[0].reg.type != VKD3DSPR_NULL)
    {
        component_count = vsir_write_mask_component_count(dst[0].write_mask);
        type_id = spirv_compiler_get_type_id_for_dst(compiler, &dst[0]);

        src0_id = spirv_compiler_emit_load_src(compiler, &src[0], dst[0].write_mask);
        src1_id = spirv_compiler_emit_load_src(compiler, &src[1], dst[0].write_mask);

        condition_id = spirv_compiler_emit_int_to_bool(compiler,
                VKD3D_SHADER_CONDITIONAL_OP_NZ, src[1].reg.data_type, component_count, src1_id);
        if (dst[0].reg.data_type == VKD3D_DATA_UINT64)
            uint_max_id = spirv_compiler_get_constant_uint64_vector(compiler, UINT64_MAX, component_count);
        else
            uint_max_id = spirv_compiler_get_constant_uint_vector(compiler, 0xffffffff, component_count);

        val_id = vkd3d_spirv_build_op_tr2(builder, &builder->function_stream, div_op, type_id, src0_id, src1_id);
        /* The SPIR-V spec says: "The resulting value is undefined if Operand 2 is 0." */
        val_id = vkd3d_spirv_build_op_select(builder, type_id, condition_id, val_id, uint_max_id);

        spirv_compiler_emit_store_dst(compiler, &dst[0], val_id);
    }

    if (dst[1].reg.type != VKD3DSPR_NULL)
    {
        if (!component_count || dst[0].write_mask != dst[1].write_mask)
        {
            component_count = vsir_write_mask_component_count(dst[1].write_mask);
            type_id = spirv_compiler_get_type_id_for_dst(compiler, &dst[1]);

            src0_id = spirv_compiler_emit_load_src(compiler, &src[0], dst[1].write_mask);
            src1_id = spirv_compiler_emit_load_src(compiler, &src[1], dst[1].write_mask);

            condition_id = spirv_compiler_emit_int_to_bool(compiler,
                    VKD3D_SHADER_CONDITIONAL_OP_NZ, src[1].reg.data_type, component_count, src1_id);
            if (dst[1].reg.data_type == VKD3D_DATA_UINT64)
                uint_max_id = spirv_compiler_get_constant_uint64_vector(compiler, UINT64_MAX, component_count);
            else
                uint_max_id = spirv_compiler_get_constant_uint_vector(compiler, 0xffffffff, component_count);
        }

        val_id = vkd3d_spirv_build_op_tr2(builder, &builder->function_stream, mod_op, type_id, src0_id, src1_id);
        /* The SPIR-V spec says: "The resulting value is undefined if Operand 2 is 0." */
        val_id = vkd3d_spirv_build_op_select(builder, type_id, condition_id, val_id, uint_max_id);

        spirv_compiler_emit_store_dst(compiler, &dst[1], val_id);
    }
}

static void spirv_compiler_emit_ftoi(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    uint32_t src_id, int_min_id, int_max_id, zero_id, float_max_id, condition_id, val_id;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t src_type_id, dst_type_id, condition_type_id;
    enum vkd3d_shader_component_type component_type;
    unsigned int component_count;

    VKD3D_ASSERT(instruction->dst_count == 1);
    VKD3D_ASSERT(instruction->src_count == 1);

    /* OpConvertFToI has undefined results if the result cannot be represented
     * as a signed integer, but Direct3D expects the result to saturate,
     * and for NaN to yield zero. */

    component_count = vsir_write_mask_component_count(dst->write_mask);
    src_type_id = spirv_compiler_get_type_id_for_reg(compiler, &src->reg, dst->write_mask);
    dst_type_id = spirv_compiler_get_type_id_for_dst(compiler, dst);
    src_id = spirv_compiler_emit_load_src(compiler, src, dst->write_mask);

    if (src->reg.data_type == VKD3D_DATA_DOUBLE)
    {
        int_min_id = spirv_compiler_get_constant_double_vector(compiler, -2147483648.0, component_count);
        float_max_id = spirv_compiler_get_constant_double_vector(compiler, 2147483648.0, component_count);
    }
    else
    {
        int_min_id = spirv_compiler_get_constant_float_vector(compiler, -2147483648.0f, component_count);
        float_max_id = spirv_compiler_get_constant_float_vector(compiler, 2147483648.0f, component_count);
    }

    val_id = vkd3d_spirv_build_op_glsl_std450_max(builder, src_type_id, src_id, int_min_id);

    /* VSIR allows the destination of a signed conversion to be unsigned. */
    component_type = vkd3d_component_type_from_data_type(dst->reg.data_type);

    int_max_id = spirv_compiler_get_constant_vector(compiler, component_type, component_count, INT_MAX);
    condition_type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_BOOL, component_count);
    condition_id = vkd3d_spirv_build_op_tr2(builder, &builder->function_stream,
            SpvOpFOrdGreaterThanEqual, condition_type_id, val_id, float_max_id);

    val_id = vkd3d_spirv_build_op_tr1(builder, &builder->function_stream, SpvOpConvertFToS, dst_type_id, val_id);
    val_id = vkd3d_spirv_build_op_select(builder, dst_type_id, condition_id, int_max_id, val_id);

    zero_id = spirv_compiler_get_constant_vector(compiler, component_type, component_count, 0);
    condition_id = vkd3d_spirv_build_op_tr1(builder, &builder->function_stream, SpvOpIsNan, condition_type_id, src_id);
    val_id = vkd3d_spirv_build_op_select(builder, dst_type_id, condition_id, zero_id, val_id);

    spirv_compiler_emit_store_dst(compiler, dst, val_id);
}

static void spirv_compiler_emit_ftou(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    uint32_t src_id, zero_id, uint_max_id, float_max_id, condition_id, val_id;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t src_type_id, dst_type_id, condition_type_id;
    unsigned int component_count;

    VKD3D_ASSERT(instruction->dst_count == 1);
    VKD3D_ASSERT(instruction->src_count == 1);

    /* OpConvertFToU has undefined results if the result cannot be represented
     * as an unsigned integer, but Direct3D expects the result to saturate,
     * and for NaN to yield zero. */

    component_count = vsir_write_mask_component_count(dst->write_mask);
    src_type_id = spirv_compiler_get_type_id_for_reg(compiler, &src->reg, dst->write_mask);
    dst_type_id = spirv_compiler_get_type_id_for_dst(compiler, dst);
    src_id = spirv_compiler_emit_load_src(compiler, src, dst->write_mask);

    if (src->reg.data_type == VKD3D_DATA_DOUBLE)
    {
        zero_id = spirv_compiler_get_constant_double_vector(compiler, 0.0, component_count);
        float_max_id = spirv_compiler_get_constant_double_vector(compiler, 4294967296.0, component_count);
    }
    else
    {
        zero_id = spirv_compiler_get_constant_float_vector(compiler, 0.0f, component_count);
        float_max_id = spirv_compiler_get_constant_float_vector(compiler, 4294967296.0f, component_count);
    }

    val_id = vkd3d_spirv_build_op_glsl_std450_max(builder, src_type_id, src_id, zero_id);

    uint_max_id = spirv_compiler_get_constant_uint_vector(compiler, UINT_MAX, component_count);
    condition_type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_BOOL, component_count);
    condition_id = vkd3d_spirv_build_op_tr2(builder, &builder->function_stream,
            SpvOpFOrdGreaterThanEqual, condition_type_id, val_id, float_max_id);

    val_id = vkd3d_spirv_build_op_tr1(builder, &builder->function_stream, SpvOpConvertFToU, dst_type_id, val_id);
    val_id = vkd3d_spirv_build_op_select(builder, dst_type_id, condition_id, uint_max_id, val_id);

    spirv_compiler_emit_store_dst(compiler, dst, val_id);
}

static void spirv_compiler_emit_bitfield_instruction(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    uint32_t src_ids[4], constituents[VKD3D_VEC4_SIZE], type_id, mask_id, size_id, max_count_id;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    enum vkd3d_shader_component_type component_type;
    unsigned int i, j, k, src_count, size;
    uint32_t write_mask;
    SpvOp op;

    src_count = instruction->src_count;
    VKD3D_ASSERT(2 <= src_count && src_count <= ARRAY_SIZE(src_ids));

    component_type = vkd3d_component_type_from_data_type(dst->reg.data_type);
    type_id = vkd3d_spirv_get_type_id(builder, component_type, 1);
    size = (src[src_count - 1].reg.data_type == VKD3D_DATA_UINT64) ? 0x40 : 0x20;
    mask_id = spirv_compiler_get_constant_uint(compiler, size - 1);
    size_id = spirv_compiler_get_constant_uint(compiler, size);

    switch (instruction->opcode)
    {
        case VKD3DSIH_BFI:  op = SpvOpBitFieldInsert; break;
        case VKD3DSIH_IBFE: op = SpvOpBitFieldSExtract; break;
        case VKD3DSIH_UBFE: op = SpvOpBitFieldUExtract; break;
        default:
            ERR("Unexpected instruction %#x.\n", instruction->opcode);
            return;
    }

    VKD3D_ASSERT(dst->write_mask & VKD3DSP_WRITEMASK_ALL);
    for (i = 0, k = 0; i < VKD3D_VEC4_SIZE; ++i)
    {
        if (!(write_mask = dst->write_mask & (VKD3DSP_WRITEMASK_0 << i)))
            continue;

        for (j = 0; j < src_count; ++j)
        {
            src_ids[src_count - j - 1] = spirv_compiler_emit_load_src_with_type(compiler,
                    &src[j], write_mask, component_type);
        }

        /* In SPIR-V, the last two operands are Offset and Count. */
        for (j = src_count - 2; j < src_count; ++j)
        {
            src_ids[j] = vkd3d_spirv_build_op_and(builder, type_id, src_ids[j], mask_id);
        }
        max_count_id = vkd3d_spirv_build_op_isub(builder, type_id, size_id, src_ids[src_count - 2]);
        src_ids[src_count - 1] = vkd3d_spirv_build_op_glsl_std450_umin(builder, type_id,
                src_ids[src_count - 1], max_count_id);

        constituents[k++] = vkd3d_spirv_build_op_trv(builder, &builder->function_stream,
                op, type_id, src_ids, src_count);
    }

    spirv_compiler_emit_store_dst_components(compiler, dst, component_type, constituents);
}

static void spirv_compiler_emit_f16tof32(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    uint32_t instr_set_id, type_id, scalar_type_id, src_id, result_id;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t components[VKD3D_VEC4_SIZE];
    uint32_t write_mask;
    unsigned int i, j;

    instr_set_id = vkd3d_spirv_get_glsl_std450_instr_set(builder);
    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_FLOAT, 2);
    scalar_type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_FLOAT, 1);

    /* FIXME: Consider a single UnpackHalf2x16 instruction per 2 components. */
    VKD3D_ASSERT(dst->write_mask & VKD3DSP_WRITEMASK_ALL);
    for (i = 0, j = 0; i < VKD3D_VEC4_SIZE; ++i)
    {
        if (!(write_mask = dst->write_mask & (VKD3DSP_WRITEMASK_0 << i)))
            continue;

        src_id = spirv_compiler_emit_load_src(compiler, src, write_mask);
        result_id = vkd3d_spirv_build_op_ext_inst(builder, type_id,
                instr_set_id, GLSLstd450UnpackHalf2x16, &src_id, 1);
        components[j++] = vkd3d_spirv_build_op_composite_extract1(builder,
                scalar_type_id, result_id, 0);
    }

    spirv_compiler_emit_store_dst_components(compiler,
            dst, vkd3d_component_type_from_data_type(dst->reg.data_type), components);
}

static void spirv_compiler_emit_f32tof16(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    uint32_t instr_set_id, type_id, scalar_type_id, src_id, zero_id, constituents[2];
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t components[VKD3D_VEC4_SIZE];
    uint32_t write_mask;
    unsigned int i, j;

    instr_set_id = vkd3d_spirv_get_glsl_std450_instr_set(builder);
    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_FLOAT, 2);
    scalar_type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, 1);
    zero_id = spirv_compiler_get_constant_float(compiler, 0.0f);

    /* FIXME: Consider a single PackHalf2x16 instruction per 2 components. */
    VKD3D_ASSERT(dst->write_mask & VKD3DSP_WRITEMASK_ALL);
    for (i = 0, j = 0; i < VKD3D_VEC4_SIZE; ++i)
    {
        if (!(write_mask = dst->write_mask & (VKD3DSP_WRITEMASK_0 << i)))
            continue;

        src_id = spirv_compiler_emit_load_src(compiler, src, write_mask);
        constituents[0] = src_id;
        constituents[1] = zero_id;
        src_id = vkd3d_spirv_build_op_composite_construct(builder,
                type_id, constituents, ARRAY_SIZE(constituents));
        components[j++] = vkd3d_spirv_build_op_ext_inst(builder, scalar_type_id,
                instr_set_id, GLSLstd450PackHalf2x16, &src_id, 1);
    }

    spirv_compiler_emit_store_dst_components(compiler,
            dst, vkd3d_component_type_from_data_type(dst->reg.data_type), components);
}

static void spirv_compiler_emit_comparison_instruction(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t src0_id, src1_id, type_id, result_id;
    unsigned int component_count;
    SpvOp op;

    switch (instruction->opcode)
    {
        case VKD3DSIH_DEQO:
        case VKD3DSIH_EQO: op = SpvOpFOrdEqual; break;
        case VKD3DSIH_EQU: op = SpvOpFUnordEqual; break;
        case VKD3DSIH_DGEO:
        case VKD3DSIH_GEO: op = SpvOpFOrdGreaterThanEqual; break;
        case VKD3DSIH_GEU: op = SpvOpFUnordGreaterThanEqual; break;
        case VKD3DSIH_IEQ: op = SpvOpIEqual; break;
        case VKD3DSIH_IGE: op = SpvOpSGreaterThanEqual; break;
        case VKD3DSIH_ILT: op = SpvOpSLessThan; break;
        case VKD3DSIH_INE: op = SpvOpINotEqual; break;
        case VKD3DSIH_DLT:
        case VKD3DSIH_LTO: op = SpvOpFOrdLessThan; break;
        case VKD3DSIH_LTU: op = SpvOpFUnordLessThan; break;
        case VKD3DSIH_NEO: op = SpvOpFOrdNotEqual; break;
        case VKD3DSIH_DNE:
        case VKD3DSIH_NEU: op = SpvOpFUnordNotEqual; break;
        case VKD3DSIH_UGE: op = SpvOpUGreaterThanEqual; break;
        case VKD3DSIH_ULT: op = SpvOpULessThan; break;
        default:
            ERR("Unexpected instruction %#x.\n", instruction->opcode);
            return;
    }

    component_count = vsir_write_mask_component_count(dst->write_mask);

    src0_id = spirv_compiler_emit_load_src(compiler, &src[0], dst->write_mask);
    src1_id = spirv_compiler_emit_load_src(compiler, &src[1], dst->write_mask);

    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_BOOL, component_count);
    result_id = vkd3d_spirv_build_op_tr2(builder, &builder->function_stream,
            op, type_id, src0_id, src1_id);

    if (dst->reg.data_type != VKD3D_DATA_BOOL)
        result_id = spirv_compiler_emit_bool_to_int(compiler, component_count, result_id, true);
    spirv_compiler_emit_store_reg(compiler, &dst->reg, dst->write_mask, result_id);
}

static void spirv_compiler_emit_orderedness_instruction(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t type_id, src0_id, src1_id, val_id;

    type_id = spirv_compiler_get_type_id_for_dst(compiler, dst);
    src0_id = spirv_compiler_emit_load_src(compiler, &src[0], dst->write_mask);
    src1_id = spirv_compiler_emit_load_src(compiler, &src[1], dst->write_mask);
    /* OpOrdered and OpUnordered are only available in Kernel mode. */
    src0_id = vkd3d_spirv_build_op_is_nan(builder, type_id, src0_id);
    src1_id = vkd3d_spirv_build_op_is_nan(builder, type_id, src1_id);
    val_id = vkd3d_spirv_build_op_logical_or(builder, type_id, src0_id, src1_id);
    if (instruction->opcode == VKD3DSIH_ORD)
        val_id = vkd3d_spirv_build_op_logical_not(builder, type_id, val_id);
    spirv_compiler_emit_store_dst(compiler, dst, val_id);
}

static void spirv_compiler_emit_float_comparison_instruction(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t src0_id, src1_id, type_id, result_id;
    unsigned int component_count;
    SpvOp op;

    switch (instruction->opcode)
    {
        case VKD3DSIH_SLT: op = SpvOpFOrdLessThan; break;
        case VKD3DSIH_SGE: op = SpvOpFOrdGreaterThanEqual; break;
        default:
            vkd3d_unreachable();
    }

    component_count = vsir_write_mask_component_count(dst->write_mask);

    src0_id = spirv_compiler_emit_load_src(compiler, &src[0], dst->write_mask);
    src1_id = spirv_compiler_emit_load_src(compiler, &src[1], dst->write_mask);

    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_BOOL, component_count);
    result_id = vkd3d_spirv_build_op_tr2(builder, &builder->function_stream, op, type_id, src0_id, src1_id);

    result_id = spirv_compiler_emit_bool_to_float(compiler, component_count, result_id, false);
    spirv_compiler_emit_store_reg(compiler, &dst->reg, dst->write_mask, result_id);
}

static uint32_t spirv_compiler_emit_conditional_branch(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction, uint32_t target_block_id)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t condition_id, merge_block_id;

    condition_id = spirv_compiler_emit_load_src(compiler, src, VKD3DSP_WRITEMASK_0);
    condition_id = spirv_compiler_emit_int_to_bool(compiler, instruction->flags, src->reg.data_type, 1, condition_id);

    merge_block_id = vkd3d_spirv_alloc_id(builder);

    vkd3d_spirv_build_op_selection_merge(builder, merge_block_id, SpvSelectionControlMaskNone);
    vkd3d_spirv_build_op_branch_conditional(builder, condition_id, target_block_id, merge_block_id);

    return merge_block_id;
}

static void spirv_compiler_end_invocation_interlock(struct spirv_compiler *compiler)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;

    if (vkd3d_spirv_capability_is_enabled(builder, SpvCapabilitySampleRateShading))
    {
        spirv_compiler_emit_execution_mode(compiler, SpvExecutionModeSampleInterlockOrderedEXT, NULL, 0);
        vkd3d_spirv_enable_capability(builder, SpvCapabilityFragmentShaderSampleInterlockEXT);
    }
    else
    {
        spirv_compiler_emit_execution_mode(compiler, SpvExecutionModePixelInterlockOrderedEXT, NULL, 0);
        vkd3d_spirv_enable_capability(builder, SpvCapabilityFragmentShaderPixelInterlockEXT);
    }
    vkd3d_spirv_build_op(&builder->function_stream, SpvOpEndInvocationInterlockEXT);
}

static void spirv_compiler_emit_return(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;

    if (compiler->use_invocation_interlock)
        spirv_compiler_end_invocation_interlock(compiler);

    if (compiler->shader_type != VKD3D_SHADER_TYPE_GEOMETRY && (is_in_default_phase(compiler)
            || is_in_control_point_phase(compiler)))
        spirv_compiler_emit_shader_epilogue_invocation(compiler);

    vkd3d_spirv_build_op_return(builder);
}

static void spirv_compiler_emit_retc(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t target_id, merge_block_id;

    target_id = vkd3d_spirv_alloc_id(builder);
    merge_block_id = spirv_compiler_emit_conditional_branch(compiler, instruction, target_id);

    vkd3d_spirv_build_op_label(builder, target_id);
    spirv_compiler_emit_return(compiler, instruction);
    vkd3d_spirv_build_op_label(builder, merge_block_id);
}

static uint32_t spirv_compiler_get_discard_function_id(struct spirv_compiler *compiler)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;

    if (!compiler->discard_function_id)
        compiler->discard_function_id = vkd3d_spirv_alloc_id(builder);

    return compiler->discard_function_id;
}

static void spirv_compiler_emit_discard_function(struct spirv_compiler *compiler)
{
    uint32_t void_id, bool_id, function_type_id, condition_id, target_block_id, merge_block_id;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;

    vkd3d_spirv_build_op_name(builder, compiler->discard_function_id, "discard");

    void_id = vkd3d_spirv_get_op_type_void(builder);
    bool_id = vkd3d_spirv_get_op_type_bool(builder);
    function_type_id = vkd3d_spirv_get_op_type_function(builder, void_id, &bool_id, 1);

    vkd3d_spirv_build_op_function(builder, void_id, compiler->discard_function_id,
            SpvFunctionControlMaskNone, function_type_id);
    condition_id = vkd3d_spirv_build_op_function_parameter(builder, bool_id);

    vkd3d_spirv_build_op_label(builder, vkd3d_spirv_alloc_id(builder));

    target_block_id = vkd3d_spirv_alloc_id(builder);
    merge_block_id = vkd3d_spirv_alloc_id(builder);
    vkd3d_spirv_build_op_selection_merge(builder, merge_block_id, SpvSelectionControlMaskNone);
    vkd3d_spirv_build_op_branch_conditional(builder, condition_id, target_block_id, merge_block_id);

    vkd3d_spirv_build_op_label(builder, target_block_id);

    if (spirv_compiler_is_target_extension_supported(compiler,
            VKD3D_SHADER_SPIRV_EXTENSION_EXT_DEMOTE_TO_HELPER_INVOCATION))
    {
        vkd3d_spirv_enable_capability(builder, SpvCapabilityDemoteToHelperInvocationEXT);
        vkd3d_spirv_build_op_demote_to_helper_invocation(builder);
        vkd3d_spirv_build_op_branch(builder, merge_block_id);
    }
    else
    {
        vkd3d_spirv_build_op_kill(builder);
    }

    vkd3d_spirv_build_op_label(builder, merge_block_id);
    vkd3d_spirv_build_op_return(builder);
    vkd3d_spirv_build_op_function_end(builder);
}

static void spirv_compiler_emit_discard(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t condition_id, void_id;

    /* discard is not a block terminator in VSIR, and emitting it as such in SPIR-V would cause
     * a mismatch between the VSIR structure and the SPIR-V one, which would cause problems if
     * structurisation is necessary. Therefore we emit it as a function call. */
    condition_id = spirv_compiler_emit_load_src(compiler, src, VKD3DSP_WRITEMASK_0);
    if (src->reg.data_type != VKD3D_DATA_BOOL)
        condition_id = spirv_compiler_emit_int_to_bool(compiler,
                instruction->flags, src->reg.data_type, 1, condition_id);
    else if (instruction->flags & VKD3D_SHADER_CONDITIONAL_OP_Z)
        condition_id = vkd3d_spirv_build_op_logical_not(builder, vkd3d_spirv_get_op_type_bool(builder), condition_id);
    void_id = vkd3d_spirv_get_op_type_void(builder);
    vkd3d_spirv_build_op_function_call(builder, void_id, spirv_compiler_get_discard_function_id(compiler),
            &condition_id, 1);
}

static bool spirv_compiler_init_blocks(struct spirv_compiler *compiler, unsigned int block_count)
{
    compiler->block_count = block_count;

    if (!(compiler->block_label_ids = vkd3d_calloc(block_count, sizeof(*compiler->block_label_ids))))
        return false;

    return true;
}

static void spirv_compiler_emit_label(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_src_param *src = instruction->src;
    unsigned int block_id = src->reg.idx[0].offset;
    uint32_t label_id;

    label_id = spirv_compiler_get_label_id(compiler, block_id);
    vkd3d_spirv_build_op_label(builder, label_id);

    --block_id;
    if (block_id < compiler->block_name_count && compiler->block_names[block_id])
        vkd3d_spirv_build_op_name(builder, label_id, compiler->block_names[block_id]);

    spirv_compiler_initialise_block(compiler);
}

static void spirv_compiler_emit_merge(struct spirv_compiler *compiler,
        uint32_t merge_block_id, uint32_t continue_block_id)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;

    if (!merge_block_id)
        return;

    merge_block_id = spirv_compiler_get_label_id(compiler, merge_block_id);
    if (!continue_block_id)
    {
        vkd3d_spirv_build_op_selection_merge(builder, merge_block_id, SpvSelectionControlMaskNone);
    }
    else
    {
        continue_block_id = spirv_compiler_get_label_id(compiler, continue_block_id);
        vkd3d_spirv_build_op_loop_merge(builder, merge_block_id, continue_block_id, SpvLoopControlMaskNone);
    }
}

static void spirv_compiler_emit_branch(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t condition_id;

    if (vsir_register_is_label(&src[0].reg))
    {
        if (instruction->src_count > 1)
        {
            /* Loop merge only. Must have a merge block and a continue block. */
            if (instruction->src_count == 3)
                spirv_compiler_emit_merge(compiler, src[1].reg.idx[0].offset, src[2].reg.idx[0].offset);
            else
                ERR("Invalid branch with %u sources.\n", instruction->src_count);
        }
        vkd3d_spirv_build_op_branch(builder, spirv_compiler_get_label_id(compiler, src[0].reg.idx[0].offset));
        return;
    }

    if (!vkd3d_swizzle_is_scalar(src->swizzle, &src->reg))
    {
        WARN("Unexpected src swizzle %#x.\n", src->swizzle);
        spirv_compiler_warning(compiler, VKD3D_SHADER_WARNING_SPV_INVALID_SWIZZLE,
                "The swizzle for a branch condition value is not scalar.");
    }

    condition_id = spirv_compiler_emit_load_src(compiler, &src[0], VKD3DSP_WRITEMASK_0);
    if (src[0].reg.data_type != VKD3D_DATA_BOOL)
        condition_id = spirv_compiler_emit_int_to_bool(compiler,
                VKD3D_SHADER_CONDITIONAL_OP_NZ, src[0].reg.data_type, 1, condition_id);
    /* Emit the merge immediately before the branch instruction. */
    if (instruction->src_count >= 4)
        spirv_compiler_emit_merge(compiler, src[3].reg.idx[0].offset,
                (instruction->src_count > 4) ? src[4].reg.idx[0].offset : 0);
    else
        ERR("Invalid branch with %u sources.\n", instruction->src_count);
    vkd3d_spirv_build_op_branch_conditional(builder, condition_id,
            spirv_compiler_get_label_id(compiler, src[1].reg.idx[0].offset),
            spirv_compiler_get_label_id(compiler, src[2].reg.idx[0].offset));
}

static void spirv_compiler_emit_switch(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t val_id, default_id;
    unsigned int i, word_count;
    uint32_t *cases;

    if (!vkd3d_swizzle_is_scalar(src[0].swizzle, &src[0].reg))
    {
        WARN("Unexpected src swizzle %#x.\n", src[0].swizzle);
        spirv_compiler_warning(compiler, VKD3D_SHADER_WARNING_SPV_INVALID_SWIZZLE,
                "The swizzle for a switch value is not scalar.");
    }

    word_count = instruction->src_count - 3;
    if (!(cases = vkd3d_calloc(word_count, sizeof(*cases))))
    {
        spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_OUT_OF_MEMORY,
                "Failed to allocate %u words for switch cases.", word_count);
        return;
    }

    val_id = spirv_compiler_emit_load_src(compiler, &src[0], VKD3DSP_WRITEMASK_0);
    default_id = spirv_compiler_get_label_id(compiler, src[1].reg.idx[0].offset);
    /* No instructions may occur between the merge and the switch. */
    spirv_compiler_emit_merge(compiler, src[2].reg.idx[0].offset, 0);

    src = &src[3];
    for (i = 0; i < word_count; i += 2)
    {
        cases[i] = src[i].reg.u.immconst_u32[0];
        cases[i + 1] = spirv_compiler_get_label_id(compiler, src[i + 1].reg.idx[0].offset);
    }

    vkd3d_spirv_build_op_switch(builder, val_id, default_id, cases, word_count / 2u);

    vkd3d_free(cases);
}

static void spirv_compiler_emit_deriv_instruction(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    const struct instruction_info *info;
    uint32_t type_id, src_id, val_id;
    unsigned int i;

    static const struct instruction_info
    {
        enum vkd3d_shader_opcode opcode;
        SpvOp op;
        bool needs_derivative_control;
    }
    deriv_instructions[] =
    {
        {VKD3DSIH_DSX,        SpvOpDPdx},
        {VKD3DSIH_DSX_COARSE, SpvOpDPdxCoarse, true},
        {VKD3DSIH_DSX_FINE,   SpvOpDPdxFine,   true},
        {VKD3DSIH_DSY,        SpvOpDPdy},
        {VKD3DSIH_DSY_COARSE, SpvOpDPdyCoarse, true},
        {VKD3DSIH_DSY_FINE,   SpvOpDPdyFine,   true},
    };

    info = NULL;
    for (i = 0; i < ARRAY_SIZE(deriv_instructions); ++i)
    {
        if (deriv_instructions[i].opcode == instruction->opcode)
        {
            info = &deriv_instructions[i];
            break;
        }
    }
    if (!info)
    {
        ERR("Unexpected instruction %#x.\n", instruction->opcode);
        return;
    }

    if (info->needs_derivative_control)
        vkd3d_spirv_enable_capability(builder, SpvCapabilityDerivativeControl);

    VKD3D_ASSERT(instruction->dst_count == 1);
    VKD3D_ASSERT(instruction->src_count == 1);

    type_id = spirv_compiler_get_type_id_for_dst(compiler, dst);
    src_id = spirv_compiler_emit_load_src(compiler, src, dst->write_mask);
    val_id = vkd3d_spirv_build_op_tr1(builder, &builder->function_stream, info->op, type_id, src_id);
    spirv_compiler_emit_store_dst(compiler, dst, val_id);
}

struct vkd3d_shader_image
{
    uint32_t id;
    uint32_t image_id;
    uint32_t sampled_image_id;

    enum vkd3d_shader_component_type sampled_type;
    uint32_t image_type_id;
    const struct vkd3d_spirv_resource_type *resource_type_info;
    unsigned int structure_stride;
    bool raw;
};

#define VKD3D_IMAGE_FLAG_NONE    0x0
#define VKD3D_IMAGE_FLAG_DEPTH   0x1
#define VKD3D_IMAGE_FLAG_NO_LOAD 0x2
#define VKD3D_IMAGE_FLAG_SAMPLED 0x4

static const struct vkd3d_symbol *spirv_compiler_find_resource(struct spirv_compiler *compiler,
        const struct vkd3d_shader_register *resource_reg)
{
    struct vkd3d_symbol resource_key;
    struct rb_entry *entry;

    vkd3d_symbol_make_resource(&resource_key, resource_reg);
    entry = rb_get(&compiler->symbol_table, &resource_key);
    VKD3D_ASSERT(entry);
    return RB_ENTRY_VALUE(entry, struct vkd3d_symbol, entry);
}

static const struct vkd3d_symbol *spirv_compiler_find_combined_sampler(struct spirv_compiler *compiler,
        const struct vkd3d_shader_register *resource_reg, const struct vkd3d_shader_register *sampler_reg)
{
    const struct vkd3d_shader_interface_info *shader_interface = &compiler->shader_interface;
    unsigned int sampler_space, sampler_index;
    struct vkd3d_symbol key;
    struct rb_entry *entry;

    if (!shader_interface->combined_sampler_count)
        return NULL;

    if (sampler_reg)
    {
        const struct vkd3d_symbol *sampler_symbol;

        vkd3d_symbol_make_sampler(&key, sampler_reg);
        if (!(entry = rb_get(&compiler->symbol_table, &key)))
            return NULL;
        sampler_symbol = RB_ENTRY_VALUE(entry, struct vkd3d_symbol, entry);
        sampler_space = sampler_symbol->info.sampler.range.space;
        sampler_index = sampler_symbol->info.sampler.range.first;
    }
    else
    {
        sampler_space = 0;
        sampler_index = VKD3D_SHADER_DUMMY_SAMPLER_INDEX;
    }

    vkd3d_symbol_make_combined_sampler(&key, resource_reg, sampler_space, sampler_index);
    if ((entry = rb_get(&compiler->symbol_table, &key)))
        return RB_ENTRY_VALUE(entry, struct vkd3d_symbol, entry);
    return NULL;
}

static void spirv_compiler_prepare_image(struct spirv_compiler *compiler,
        struct vkd3d_shader_image *image, const struct vkd3d_shader_register *resource_reg,
        const struct vkd3d_shader_register *sampler_reg, unsigned int flags)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t sampler_var_id, sampler_id, sampled_image_type_id;
    const struct vkd3d_symbol *symbol = NULL;
    bool load, sampled;

    load = !(flags & VKD3D_IMAGE_FLAG_NO_LOAD);
    sampled = flags & VKD3D_IMAGE_FLAG_SAMPLED;

    if (resource_reg->type == VKD3DSPR_RESOURCE)
        symbol = spirv_compiler_find_combined_sampler(compiler, resource_reg, sampler_reg);
    if (!symbol)
        symbol = spirv_compiler_find_resource(compiler, resource_reg);

    if (symbol->descriptor_array)
    {
        const struct vkd3d_symbol_descriptor_array_data *array_data = &symbol->descriptor_array->info.descriptor_array;
        uint32_t ptr_type_id, index_id;

        index_id = spirv_compiler_get_descriptor_index(compiler, resource_reg, symbol->descriptor_array,
                symbol->info.resource.binding_base_idx, symbol->info.resource.resource_type_info->resource_type);

        ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, array_data->storage_class,
                array_data->contained_type_id);
        image->image_type_id = array_data->contained_type_id;

        image->id = vkd3d_spirv_build_op_access_chain(builder, ptr_type_id, symbol->id, &index_id, 1);
    }
    else
    {
        image->id = symbol->id;
        image->image_type_id = symbol->info.resource.type_id;
    }
    image->sampled_type = symbol->info.resource.sampled_type;
    image->resource_type_info = symbol->info.resource.resource_type_info;
    image->structure_stride = symbol->info.resource.structure_stride;
    image->raw = symbol->info.resource.raw;

    if (symbol->type == VKD3D_SYMBOL_COMBINED_SAMPLER)
    {
        sampled_image_type_id = vkd3d_spirv_get_op_type_sampled_image(builder, image->image_type_id);
        image->sampled_image_id = vkd3d_spirv_build_op_load(builder,
                sampled_image_type_id, image->id, SpvMemoryAccessMaskNone);
        image->image_id = !sampled ? vkd3d_spirv_build_op_image(builder,
                image->image_type_id, image->sampled_image_id) : 0;
        return;
    }

    if (load)
    {
        image->image_id = vkd3d_spirv_build_op_load(builder, image->image_type_id, image->id, SpvMemoryAccessMaskNone);
        if (resource_reg->non_uniform)
            spirv_compiler_decorate_nonuniform(compiler, image->image_id);
    }
    else
    {
        image->image_id = 0;
    }

    image->image_type_id = spirv_compiler_get_image_type_id(compiler, resource_reg,
            &symbol->info.resource.range, image->resource_type_info,
            image->sampled_type, image->structure_stride || image->raw);

    if (sampled)
    {
        struct vkd3d_shader_register_info register_info;

        VKD3D_ASSERT(image->image_id);
        VKD3D_ASSERT(sampler_reg);

        if (!spirv_compiler_get_register_info(compiler, sampler_reg, &register_info))
            ERR("Failed to get sampler register info.\n");
        sampler_var_id = register_info.id;
        if (register_info.descriptor_array)
        {
            const struct vkd3d_symbol_descriptor_array_data *array_data
                    = &register_info.descriptor_array->info.descriptor_array;
            uint32_t ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder,
                    register_info.storage_class, array_data->contained_type_id);
            uint32_t array_idx = spirv_compiler_get_descriptor_index(compiler, sampler_reg,
                    register_info.descriptor_array, register_info.binding_base_idx, VKD3D_SHADER_RESOURCE_NONE);
            sampler_var_id = vkd3d_spirv_build_op_access_chain(builder, ptr_type_id, register_info.id, &array_idx, 1);
        }

        sampler_id = vkd3d_spirv_build_op_load(builder,
                vkd3d_spirv_get_op_type_sampler(builder), sampler_var_id, SpvMemoryAccessMaskNone);
        if (sampler_reg->non_uniform)
            spirv_compiler_decorate_nonuniform(compiler, sampler_id);

        sampled_image_type_id = vkd3d_spirv_get_op_type_sampled_image(builder, image->image_type_id);
        image->sampled_image_id = vkd3d_spirv_build_op_sampled_image(builder,
                sampled_image_type_id, image->image_id, sampler_id);
        if (resource_reg->non_uniform)
            spirv_compiler_decorate_nonuniform(compiler, image->sampled_image_id);
    }
    else
    {
        image->sampled_image_id = 0;
    }
}

static uint32_t spirv_compiler_emit_texel_offset(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction,
        const struct vkd3d_spirv_resource_type *resource_type_info)
{
    unsigned int component_count = resource_type_info->coordinate_component_count - resource_type_info->arrayed;
    const struct vkd3d_shader_texel_offset *offset = &instruction->texel_offset;
    int32_t data[4] = {offset->u, offset->v, offset->w, 0};

    VKD3D_ASSERT(resource_type_info->dim != SpvDimCube);
    return spirv_compiler_get_constant(compiler,
            VKD3D_SHADER_COMPONENT_INT, component_count, (const uint32_t *)data);
}

static void spirv_compiler_emit_ld(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t type_id, coordinate_id, val_id;
    SpvImageOperandsMask operands_mask = 0;
    unsigned int image_operand_count = 0;
    struct vkd3d_shader_image image;
    uint32_t image_operands[2];
    uint32_t coordinate_mask;
    bool multisample;

    multisample = instruction->opcode == VKD3DSIH_LD2DMS;

    spirv_compiler_prepare_image(compiler, &image, &src[1].reg, NULL, VKD3D_IMAGE_FLAG_NONE);

    type_id = vkd3d_spirv_get_type_id(builder, image.sampled_type, VKD3D_VEC4_SIZE);
    coordinate_mask = (1u << image.resource_type_info->coordinate_component_count) - 1;
    coordinate_id = spirv_compiler_emit_load_src(compiler, &src[0], coordinate_mask);
    if (image.resource_type_info->resource_type != VKD3D_SHADER_RESOURCE_BUFFER && !multisample)
    {
        operands_mask |= SpvImageOperandsLodMask;
        image_operands[image_operand_count++] = spirv_compiler_emit_load_src(compiler,
                &src[0], VKD3DSP_WRITEMASK_3);
    }
    if (vkd3d_shader_instruction_has_texel_offset(instruction))
    {
        operands_mask |= SpvImageOperandsConstOffsetMask;
        image_operands[image_operand_count++] = spirv_compiler_emit_texel_offset(compiler,
                instruction, image.resource_type_info);
    }
    if (multisample && image.resource_type_info->ms)
    {
        operands_mask |= SpvImageOperandsSampleMask;
        image_operands[image_operand_count++] = spirv_compiler_emit_load_src(compiler,
                &src[2], VKD3DSP_WRITEMASK_0);
    }
    VKD3D_ASSERT(image_operand_count <= ARRAY_SIZE(image_operands));
    val_id = vkd3d_spirv_build_op_image_fetch(builder, type_id,
            image.image_id, coordinate_id, operands_mask, image_operands, image_operand_count);

    spirv_compiler_emit_store_dst_swizzled(compiler,
            dst, val_id, image.sampled_type, src[1].swizzle);
}

static void spirv_compiler_emit_lod(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    const struct vkd3d_shader_src_param *resource, *sampler;
    uint32_t type_id, coordinate_id, val_id;
    struct vkd3d_shader_image image;

    vkd3d_spirv_enable_capability(builder, SpvCapabilityImageQuery);

    resource = &src[1];
    sampler = &src[2];
    spirv_compiler_prepare_image(compiler, &image,
            &resource->reg, &sampler->reg, VKD3D_IMAGE_FLAG_SAMPLED);

    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_FLOAT, 2);
    coordinate_id = spirv_compiler_emit_load_src(compiler, &src[0], VKD3DSP_WRITEMASK_ALL);
    val_id = vkd3d_spirv_build_op_image_query_lod(builder,
            type_id, image.sampled_image_id, coordinate_id);

    spirv_compiler_emit_store_dst_swizzled(compiler,
            dst, val_id, image.sampled_type, resource->swizzle);
}

static void spirv_compiler_emit_sample(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    const struct vkd3d_shader_src_param *resource, *sampler;
    unsigned int image_operand_count = 0, component_count;
    uint32_t sampled_type_id, coordinate_id, val_id;
    SpvImageOperandsMask operands_mask = 0;
    struct vkd3d_shader_image image;
    uint32_t image_operands[3];
    uint32_t coordinate_mask;
    SpvOp op;

    resource = &src[1];
    sampler = &src[2];
    spirv_compiler_prepare_image(compiler, &image,
            &resource->reg, &sampler->reg, VKD3D_IMAGE_FLAG_SAMPLED);

    switch (instruction->opcode)
    {
        case VKD3DSIH_SAMPLE:
            op = SpvOpImageSampleImplicitLod;
            break;
        case VKD3DSIH_SAMPLE_B:
            op = SpvOpImageSampleImplicitLod;
            operands_mask |= SpvImageOperandsBiasMask;
            image_operands[image_operand_count++] = spirv_compiler_emit_load_src(compiler,
                    &src[3], VKD3DSP_WRITEMASK_0);
            break;
        case VKD3DSIH_SAMPLE_GRAD:
            op = SpvOpImageSampleExplicitLod;
            operands_mask |= SpvImageOperandsGradMask;
            component_count = image.resource_type_info->coordinate_component_count - image.resource_type_info->arrayed;
            coordinate_mask = (1u << component_count) - 1;
            image_operands[image_operand_count++] = spirv_compiler_emit_load_src(compiler,
                    &src[3], coordinate_mask);
            image_operands[image_operand_count++] = spirv_compiler_emit_load_src(compiler,
                    &src[4], coordinate_mask);
            break;
        case VKD3DSIH_SAMPLE_LOD:
            op = SpvOpImageSampleExplicitLod;
            operands_mask |= SpvImageOperandsLodMask;
            image_operands[image_operand_count++] = spirv_compiler_emit_load_src(compiler,
                    &src[3], VKD3DSP_WRITEMASK_0);
            break;
        default:
            ERR("Unexpected instruction %#x.\n", instruction->opcode);
            return;
    }

    if (vkd3d_shader_instruction_has_texel_offset(instruction))
    {
        operands_mask |= SpvImageOperandsConstOffsetMask;
        image_operands[image_operand_count++] = spirv_compiler_emit_texel_offset(compiler,
                instruction, image.resource_type_info);
    }

    sampled_type_id = vkd3d_spirv_get_type_id(builder, image.sampled_type, VKD3D_VEC4_SIZE);
    coordinate_id = spirv_compiler_emit_load_src(compiler, &src[0], VKD3DSP_WRITEMASK_ALL);
    VKD3D_ASSERT(image_operand_count <= ARRAY_SIZE(image_operands));
    val_id = vkd3d_spirv_build_op_image_sample(builder, op, sampled_type_id,
            image.sampled_image_id, coordinate_id, operands_mask, image_operands, image_operand_count);

    spirv_compiler_emit_store_dst_swizzled(compiler,
            dst, val_id, image.sampled_type, resource->swizzle);
}

static void spirv_compiler_emit_sample_c(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t sampled_type_id, coordinate_id, dref_id, val_id;
    SpvImageOperandsMask operands_mask = 0;
    unsigned int image_operand_count = 0;
    struct vkd3d_shader_image image;
    uint32_t image_operands[2];
    SpvOp op;

    if (instruction->opcode == VKD3DSIH_SAMPLE_C_LZ)
    {
        op = SpvOpImageSampleDrefExplicitLod;
        operands_mask |= SpvImageOperandsLodMask;
        image_operands[image_operand_count++]
                = spirv_compiler_get_constant_float(compiler, 0.0f);
    }
    else
    {
        op = SpvOpImageSampleDrefImplicitLod;
    }

    spirv_compiler_prepare_image(compiler,
            &image, &src[1].reg, &src[2].reg, VKD3D_IMAGE_FLAG_SAMPLED | VKD3D_IMAGE_FLAG_DEPTH);

    if (vkd3d_shader_instruction_has_texel_offset(instruction))
    {
        operands_mask |= SpvImageOperandsConstOffsetMask;
        image_operands[image_operand_count++] = spirv_compiler_emit_texel_offset(compiler,
                instruction, image.resource_type_info);
    }

    sampled_type_id = vkd3d_spirv_get_type_id(builder, image.sampled_type, 1);
    coordinate_id = spirv_compiler_emit_load_src(compiler, &src[0], VKD3DSP_WRITEMASK_ALL);
    dref_id = spirv_compiler_emit_load_src(compiler, &src[3], VKD3DSP_WRITEMASK_0);
    val_id = vkd3d_spirv_build_op_image_sample_dref(builder, op, sampled_type_id,
            image.sampled_image_id, coordinate_id, dref_id, operands_mask,
            image_operands, image_operand_count);

    spirv_compiler_emit_store_dst_scalar(compiler,
            dst, val_id, image.sampled_type, src[1].swizzle);
}

static void spirv_compiler_emit_gather4(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    const struct vkd3d_shader_src_param *addr, *offset, *resource, *sampler;
    uint32_t sampled_type_id, coordinate_id, component_id, dref_id, val_id;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    unsigned int image_flags = VKD3D_IMAGE_FLAG_SAMPLED;
    unsigned int component_count, component_idx;
    SpvImageOperandsMask operands_mask = 0;
    unsigned int image_operand_count = 0;
    struct vkd3d_shader_image image;
    uint32_t image_operands[1];
    uint32_t coordinate_mask;
    bool extended_offset;

    if (instruction->opcode == VKD3DSIH_GATHER4_C
            || instruction->opcode == VKD3DSIH_GATHER4_PO_C)
        image_flags |= VKD3D_IMAGE_FLAG_DEPTH;

    extended_offset = instruction->opcode == VKD3DSIH_GATHER4_PO
            || instruction->opcode == VKD3DSIH_GATHER4_PO_C;

    addr = &src[0];
    offset = extended_offset ? &src[1] : NULL;
    resource = &src[1 + extended_offset];
    sampler = &src[2 + extended_offset];

    spirv_compiler_prepare_image(compiler, &image,
            &resource->reg, &sampler->reg, image_flags);

    if (offset)
    {
        component_count = image.resource_type_info->coordinate_component_count - image.resource_type_info->arrayed;
        VKD3D_ASSERT(image.resource_type_info->dim != SpvDimCube);
        vkd3d_spirv_enable_capability(builder, SpvCapabilityImageGatherExtended);
        operands_mask |= SpvImageOperandsOffsetMask;
        image_operands[image_operand_count++] = spirv_compiler_emit_load_src(compiler,
                offset, (1u << component_count) - 1);
    }
    else if (vkd3d_shader_instruction_has_texel_offset(instruction))
    {
        operands_mask |= SpvImageOperandsConstOffsetMask;
        image_operands[image_operand_count++] = spirv_compiler_emit_texel_offset(compiler,
                instruction, image.resource_type_info);
    }

    sampled_type_id = vkd3d_spirv_get_type_id(builder, image.sampled_type, VKD3D_VEC4_SIZE);
    coordinate_mask = (1u << image.resource_type_info->coordinate_component_count) - 1;
    coordinate_id = spirv_compiler_emit_load_src(compiler, addr, coordinate_mask);
    if (image_flags & VKD3D_IMAGE_FLAG_DEPTH)
    {
        dref_id = spirv_compiler_emit_load_src(compiler,
                &src[3 + extended_offset], VKD3DSP_WRITEMASK_0);
        val_id = vkd3d_spirv_build_op_image_dref_gather(builder, sampled_type_id,
                image.sampled_image_id, coordinate_id, dref_id,
                operands_mask, image_operands, image_operand_count);
    }
    else
    {
        component_idx = vsir_swizzle_get_component(sampler->swizzle, 0);
        /* Nvidia driver requires signed integer type. */
        component_id = spirv_compiler_get_constant(compiler,
                VKD3D_SHADER_COMPONENT_INT, 1, &component_idx);
        val_id = vkd3d_spirv_build_op_image_gather(builder, sampled_type_id,
                image.sampled_image_id, coordinate_id, component_id,
                operands_mask, image_operands, image_operand_count);
    }

    spirv_compiler_emit_store_dst_swizzled(compiler,
            dst, val_id, image.sampled_type, resource->swizzle);
}

static uint32_t spirv_compiler_emit_raw_structured_addressing(
        struct spirv_compiler *compiler, uint32_t type_id, unsigned int stride,
        const struct vkd3d_shader_src_param *src0, uint32_t src0_mask,
        const struct vkd3d_shader_src_param *src1, uint32_t src1_mask)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_src_param *offset;
    uint32_t structure_id = 0, offset_id;
    uint32_t offset_write_mask;

    if (stride)
    {
        structure_id = spirv_compiler_emit_load_src(compiler, src0, src0_mask);
        structure_id = vkd3d_spirv_build_op_imul(builder, type_id,
                structure_id, spirv_compiler_get_constant_uint(compiler, stride));
    }
    offset = stride ? src1 : src0;
    offset_write_mask = stride ? src1_mask : src0_mask;

    offset_id = spirv_compiler_emit_load_src(compiler, offset, offset_write_mask);
    offset_id = vkd3d_spirv_build_op_shift_right_logical(builder, type_id,
            offset_id, spirv_compiler_get_constant_uint(compiler, 2));

    if (structure_id)
        return vkd3d_spirv_build_op_iadd(builder, type_id, structure_id, offset_id);
    else
        return offset_id;
}

static void spirv_compiler_emit_ld_raw_structured_srv_uav(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    uint32_t coordinate_id, type_id, val_id, texel_type_id, ptr_type_id, ptr_id;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    const struct vkd3d_shader_src_param *resource;
    const struct vkd3d_symbol *resource_symbol;
    uint32_t base_coordinate_id, component_idx;
    uint32_t constituents[VKD3D_VEC4_SIZE];
    struct vkd3d_shader_image image;
    bool storage_buffer_uav = false;
    uint32_t indices[2];
    unsigned int i, j;
    SpvOp op;

    resource = &src[instruction->src_count - 1];

    if (resource->reg.type == VKD3DSPR_UAV)
    {
        resource_symbol = spirv_compiler_find_resource(compiler, &resource->reg);
        storage_buffer_uav = spirv_compiler_use_storage_buffer(compiler, &resource_symbol->info.resource);
    }

    if (storage_buffer_uav)
    {
        texel_type_id = vkd3d_spirv_get_type_id(builder, resource_symbol->info.resource.sampled_type, 1);
        ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, SpvStorageClassUniform, texel_type_id);

        type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, 1);
        base_coordinate_id = spirv_compiler_emit_raw_structured_addressing(compiler,
                type_id, resource_symbol->info.resource.structure_stride,
                &src[0], VKD3DSP_WRITEMASK_0, &src[1], VKD3DSP_WRITEMASK_0);

        VKD3D_ASSERT(dst->write_mask & VKD3DSP_WRITEMASK_ALL);
        for (i = 0, j = 0; i < VKD3D_VEC4_SIZE; ++i)
        {
            if (!(dst->write_mask & (VKD3DSP_WRITEMASK_0 << i)))
                continue;

            component_idx = vsir_swizzle_get_component(resource->swizzle, i);
            coordinate_id = base_coordinate_id;
            if (component_idx)
                coordinate_id = vkd3d_spirv_build_op_iadd(builder, type_id,
                        coordinate_id, spirv_compiler_get_constant_uint(compiler, component_idx));
            indices[0] = spirv_compiler_get_constant_uint(compiler, 0);
            indices[1] = coordinate_id;

            ptr_id = vkd3d_spirv_build_op_access_chain(builder, ptr_type_id, resource_symbol->id, indices, 2);
            constituents[j++] = vkd3d_spirv_build_op_load(builder, texel_type_id, ptr_id, SpvMemoryAccessMaskNone);
        }
    }
    else
    {
        if (resource->reg.type == VKD3DSPR_RESOURCE)
            op = SpvOpImageFetch;
        else
            op = SpvOpImageRead;

        spirv_compiler_prepare_image(compiler, &image, &resource->reg, NULL, VKD3D_IMAGE_FLAG_NONE);

        type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, 1);
        base_coordinate_id = spirv_compiler_emit_raw_structured_addressing(compiler,
                type_id, image.structure_stride, &src[0], VKD3DSP_WRITEMASK_0, &src[1], VKD3DSP_WRITEMASK_0);

        texel_type_id = vkd3d_spirv_get_type_id(builder, image.sampled_type, VKD3D_VEC4_SIZE);
        VKD3D_ASSERT(dst->write_mask & VKD3DSP_WRITEMASK_ALL);
        for (i = 0, j = 0; i < VKD3D_VEC4_SIZE; ++i)
        {
            if (!(dst->write_mask & (VKD3DSP_WRITEMASK_0 << i)))
                continue;

            component_idx = vsir_swizzle_get_component(resource->swizzle, i);
            coordinate_id = base_coordinate_id;
            if (component_idx)
                coordinate_id = vkd3d_spirv_build_op_iadd(builder, type_id,
                        coordinate_id, spirv_compiler_get_constant_uint(compiler, component_idx));

            val_id = vkd3d_spirv_build_op_tr2(builder, &builder->function_stream,
                    op, texel_type_id, image.image_id, coordinate_id);
            constituents[j++] = vkd3d_spirv_build_op_composite_extract1(builder,
                    type_id, val_id, 0);
        }
    }
    spirv_compiler_emit_store_dst_components(compiler, dst, VKD3D_SHADER_COMPONENT_UINT, constituents);
}

static void spirv_compiler_emit_ld_tgsm(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t coordinate_id, type_id, ptr_type_id, ptr_id;
    const struct vkd3d_shader_src_param *resource;
    struct vkd3d_shader_register_info reg_info;
    uint32_t base_coordinate_id, component_idx;
    uint32_t constituents[VKD3D_VEC4_SIZE];
    unsigned int i, j;

    resource = &src[instruction->src_count - 1];
    if (!spirv_compiler_get_register_info(compiler, &resource->reg, &reg_info))
        return;

    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, 1);
    ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, reg_info.storage_class, type_id);
    base_coordinate_id = spirv_compiler_emit_raw_structured_addressing(compiler,
            type_id, reg_info.structure_stride, &src[0], VKD3DSP_WRITEMASK_0, &src[1], VKD3DSP_WRITEMASK_0);

    VKD3D_ASSERT(dst->write_mask & VKD3DSP_WRITEMASK_ALL);
    for (i = 0, j = 0; i < VKD3D_VEC4_SIZE; ++i)
    {
        if (!(dst->write_mask & (VKD3DSP_WRITEMASK_0 << i)))
            continue;

        component_idx = vsir_swizzle_get_component(resource->swizzle, i);
        coordinate_id = base_coordinate_id;
        if (component_idx)
            coordinate_id = vkd3d_spirv_build_op_iadd(builder, type_id,
                    coordinate_id, spirv_compiler_get_constant_uint(compiler, component_idx));

        ptr_id = vkd3d_spirv_build_op_access_chain1(builder, ptr_type_id, reg_info.id, coordinate_id);
        constituents[j++] = vkd3d_spirv_build_op_load(builder, type_id, ptr_id, SpvMemoryAccessMaskNone);
    }
    spirv_compiler_emit_store_dst_components(compiler, dst, VKD3D_SHADER_COMPONENT_UINT, constituents);
}

static void spirv_compiler_emit_ld_raw_structured(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    enum vkd3d_shader_register_type reg_type = instruction->src[instruction->src_count - 1].reg.type;
    switch (reg_type)
    {
        case VKD3DSPR_RESOURCE:
        case VKD3DSPR_UAV:
            spirv_compiler_emit_ld_raw_structured_srv_uav(compiler, instruction);
            break;
        case VKD3DSPR_GROUPSHAREDMEM:
            spirv_compiler_emit_ld_tgsm(compiler, instruction);
            break;
        default:
            ERR("Unexpected register type %#x.\n", reg_type);
    }
}

static void spirv_compiler_emit_store_uav_raw_structured(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    uint32_t coordinate_id, type_id, val_id, data_id, ptr_type_id, ptr_id;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    const struct vkd3d_symbol *resource_symbol;
    uint32_t base_coordinate_id, component_idx;
    const struct vkd3d_shader_src_param *data;
    struct vkd3d_shader_image image;
    unsigned int component_count;
    uint32_t indices[2];

    resource_symbol = spirv_compiler_find_resource(compiler, &dst->reg);

    if (spirv_compiler_use_storage_buffer(compiler, &resource_symbol->info.resource))
    {
        type_id = vkd3d_spirv_get_type_id(builder, resource_symbol->info.resource.sampled_type, 1);
        ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, SpvStorageClassUniform, type_id);

        type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, 1);
        base_coordinate_id = spirv_compiler_emit_raw_structured_addressing(compiler,
                type_id, resource_symbol->info.resource.structure_stride,
                &src[0], VKD3DSP_WRITEMASK_0, &src[1], VKD3DSP_WRITEMASK_0);

        data = &src[instruction->src_count - 1];
        VKD3D_ASSERT(data->reg.data_type == VKD3D_DATA_UINT);
        val_id = spirv_compiler_emit_load_src(compiler, data, dst->write_mask);

        component_count = vsir_write_mask_component_count(dst->write_mask);
        for (component_idx = 0; component_idx < component_count; ++component_idx)
        {
            data_id = component_count > 1 ?
                    vkd3d_spirv_build_op_composite_extract1(builder, type_id, val_id, component_idx) : val_id;

            coordinate_id = base_coordinate_id;
            if (component_idx)
                coordinate_id = vkd3d_spirv_build_op_iadd(builder, type_id,
                        coordinate_id, spirv_compiler_get_constant_uint(compiler, component_idx));
            indices[0] = spirv_compiler_get_constant_uint(compiler, 0);
            indices[1] = coordinate_id;

            ptr_id = vkd3d_spirv_build_op_access_chain(builder, ptr_type_id, resource_symbol->id, indices, 2);
            vkd3d_spirv_build_op_store(builder, ptr_id, data_id, SpvMemoryAccessMaskNone);
        }
    }
    else
    {
        type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, 1);
        spirv_compiler_prepare_image(compiler, &image, &dst->reg, NULL, VKD3D_IMAGE_FLAG_NONE);
        base_coordinate_id = spirv_compiler_emit_raw_structured_addressing(compiler,
                type_id, image.structure_stride, &src[0], VKD3DSP_WRITEMASK_0, &src[1], VKD3DSP_WRITEMASK_0);

        data = &src[instruction->src_count - 1];
        VKD3D_ASSERT(data->reg.data_type == VKD3D_DATA_UINT);
        val_id = spirv_compiler_emit_load_src(compiler, data, dst->write_mask);

        component_count = vsir_write_mask_component_count(dst->write_mask);
        for (component_idx = 0; component_idx < component_count; ++component_idx)
        {
            /* Mesa Vulkan drivers require the texel parameter to be a vector. */
            data_id = spirv_compiler_emit_construct_vector(compiler, VKD3D_SHADER_COMPONENT_UINT,
                    VKD3D_VEC4_SIZE, val_id, component_idx, component_count);

            coordinate_id = base_coordinate_id;
            if (component_idx)
                coordinate_id = vkd3d_spirv_build_op_iadd(builder, type_id,
                        coordinate_id, spirv_compiler_get_constant_uint(compiler, component_idx));

            vkd3d_spirv_build_op_image_write(builder, image.image_id, coordinate_id,
                    data_id, SpvImageOperandsMaskNone, NULL, 0);
        }
    }

}

static void spirv_compiler_emit_store_tgsm(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    uint32_t coordinate_id, type_id, val_id, ptr_type_id, ptr_id, data_id;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t base_coordinate_id, component_idx;
    struct vkd3d_shader_register_info reg_info;
    struct vkd3d_shader_src_param data;
    unsigned int component_count;

    if (!spirv_compiler_get_register_info(compiler, &dst->reg, &reg_info))
        return;

    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, 1);
    ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, reg_info.storage_class, type_id);
    base_coordinate_id = spirv_compiler_emit_raw_structured_addressing(compiler,
            type_id, reg_info.structure_stride, &src[0], VKD3DSP_WRITEMASK_0, &src[1], VKD3DSP_WRITEMASK_0);

    data = src[instruction->src_count - 1];
    data.reg.data_type = VKD3D_DATA_UINT;
    val_id = spirv_compiler_emit_load_src(compiler, &data, dst->write_mask);

    component_count = vsir_write_mask_component_count(dst->write_mask);
    for (component_idx = 0; component_idx < component_count; ++component_idx)
    {
        data_id = component_count > 1 ?
                vkd3d_spirv_build_op_composite_extract1(builder, type_id, val_id, component_idx) : val_id;

        coordinate_id = base_coordinate_id;
        if (component_idx)
            coordinate_id = vkd3d_spirv_build_op_iadd(builder, type_id,
                    coordinate_id, spirv_compiler_get_constant_uint(compiler, component_idx));

        ptr_id = vkd3d_spirv_build_op_access_chain1(builder, ptr_type_id, reg_info.id, coordinate_id);
        vkd3d_spirv_build_op_store(builder, ptr_id, data_id, SpvMemoryAccessMaskNone);
    }
}

static void spirv_compiler_emit_store_raw_structured(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    enum vkd3d_shader_register_type reg_type = instruction->dst[0].reg.type;
    switch (reg_type)
    {
        case VKD3DSPR_UAV:
            spirv_compiler_emit_store_uav_raw_structured(compiler, instruction);
            break;
        case VKD3DSPR_GROUPSHAREDMEM:
            spirv_compiler_emit_store_tgsm(compiler, instruction);
            break;
        default:
            ERR("Unexpected register type %#x.\n", reg_type);
    }
}

static void spirv_compiler_emit_ld_uav_typed(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t coordinate_id, type_id, val_id, ptr_type_id, ptr_id;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    const struct vkd3d_symbol *resource_symbol;
    struct vkd3d_shader_image image;
    uint32_t coordinate_mask;
    uint32_t indices[2];

    resource_symbol = spirv_compiler_find_resource(compiler, &src[1].reg);

    if (spirv_compiler_use_storage_buffer(compiler, &resource_symbol->info.resource))
    {
        type_id = vkd3d_spirv_get_type_id(builder, resource_symbol->info.resource.sampled_type, 1);
        ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, SpvStorageClassUniform, type_id);
        coordinate_id = spirv_compiler_emit_load_src(compiler, &src[0], VKD3DSP_WRITEMASK_0);
        indices[0] = spirv_compiler_get_constant_uint(compiler, 0);
        indices[1] = coordinate_id;

        ptr_id = vkd3d_spirv_build_op_access_chain(builder, ptr_type_id, resource_symbol->id, indices, 2);
        val_id = vkd3d_spirv_build_op_load(builder, type_id, ptr_id, SpvMemoryAccessMaskNone);

        spirv_compiler_emit_store_dst_swizzled(compiler, dst, val_id,
                resource_symbol->info.resource.sampled_type, src[1].swizzle);
    }
    else
    {
        spirv_compiler_prepare_image(compiler, &image, &src[1].reg, NULL, VKD3D_IMAGE_FLAG_NONE);
        type_id = vkd3d_spirv_get_type_id(builder, image.sampled_type, VKD3D_VEC4_SIZE);
        coordinate_mask = (1u << image.resource_type_info->coordinate_component_count) - 1;
        coordinate_id = spirv_compiler_emit_load_src(compiler, &src[0], coordinate_mask);

        val_id = vkd3d_spirv_build_op_image_read(builder, type_id,
                image.image_id, coordinate_id, SpvImageOperandsMaskNone, NULL, 0);

        spirv_compiler_emit_store_dst_swizzled(compiler,
                dst, val_id, image.sampled_type, src[1].swizzle);
    }
}

static void spirv_compiler_emit_store_uav_typed(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    uint32_t coordinate_id, texel_id, type_id, val_id, ptr_type_id, ptr_id;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    const struct vkd3d_symbol *resource_symbol;
    struct vkd3d_shader_image image;
    uint32_t coordinate_mask;
    uint32_t indices[2];

    resource_symbol = spirv_compiler_find_resource(compiler, &dst->reg);

    if (spirv_compiler_use_storage_buffer(compiler, &resource_symbol->info.resource))
    {
        type_id = vkd3d_spirv_get_type_id(builder, resource_symbol->info.resource.sampled_type, 1);
        ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, SpvStorageClassUniform, type_id);
        coordinate_id = spirv_compiler_emit_load_src(compiler, &src[0], VKD3DSP_WRITEMASK_0);
        indices[0] = spirv_compiler_get_constant_uint(compiler, 0);
        indices[1] = coordinate_id;

        val_id = spirv_compiler_emit_load_src_with_type(compiler, &src[1],
                VKD3DSP_WRITEMASK_0, resource_symbol->info.resource.sampled_type);
        ptr_id = vkd3d_spirv_build_op_access_chain(builder, ptr_type_id, resource_symbol->id, indices, 2);
        vkd3d_spirv_build_op_store(builder, ptr_id, val_id, SpvMemoryAccessMaskNone);
    }
    else
    {
        vkd3d_spirv_enable_capability(builder, SpvCapabilityStorageImageWriteWithoutFormat);

        spirv_compiler_prepare_image(compiler, &image, &dst->reg, NULL, VKD3D_IMAGE_FLAG_NONE);
        coordinate_mask = (1u << image.resource_type_info->coordinate_component_count) - 1;
        coordinate_id = spirv_compiler_emit_load_src(compiler, &src[0], coordinate_mask);
        texel_id = spirv_compiler_emit_load_src_with_type(compiler, &src[1], dst->write_mask, image.sampled_type);

        vkd3d_spirv_build_op_image_write(builder, image.image_id, coordinate_id, texel_id,
                SpvImageOperandsMaskNone, NULL, 0);
    }
}

static void spirv_compiler_emit_uav_counter_instruction(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    unsigned int memory_semantics = SpvMemorySemanticsMaskNone;
    uint32_t ptr_type_id, type_id, counter_id, result_id;
    uint32_t coordinate_id, sample_id, pointer_id;
    const struct vkd3d_symbol *resource_symbol;
    uint32_t operands[3];
    SpvOp op;

    op = instruction->opcode == VKD3DSIH_IMM_ATOMIC_ALLOC
            ? SpvOpAtomicIIncrement : SpvOpAtomicIDecrement;

    resource_symbol = spirv_compiler_find_resource(compiler, &src->reg);
    counter_id = resource_symbol->info.resource.uav_counter_id;
    VKD3D_ASSERT(counter_id);

    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, 1);

    if (resource_symbol->info.resource.uav_counter_array)
    {
        const struct vkd3d_symbol_descriptor_array_data *array_data;
        uint32_t index_id;

        index_id = spirv_compiler_get_descriptor_index(compiler, &src->reg,
                resource_symbol->info.resource.uav_counter_array,
                resource_symbol->info.resource.uav_counter_base_idx,
                resource_symbol->info.resource.resource_type_info->resource_type);

        array_data = &resource_symbol->info.resource.uav_counter_array->info.descriptor_array;
        ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, array_data->storage_class,
                array_data->contained_type_id);

        counter_id = vkd3d_spirv_build_op_access_chain(builder, ptr_type_id, counter_id, &index_id, 1);
    }

    if (spirv_compiler_is_opengl_target(compiler))
    {
        pointer_id = counter_id;
        memory_semantics |= SpvMemorySemanticsAtomicCounterMemoryMask;
    }
    else if (compiler->ssbo_uavs)
    {
        ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, SpvStorageClassUniform, type_id);
        coordinate_id = spirv_compiler_get_constant_uint(compiler, 0);
        operands[0] = spirv_compiler_get_constant_uint(compiler, 0);
        operands[1] = coordinate_id;
        pointer_id = vkd3d_spirv_build_op_access_chain(builder,
                ptr_type_id, counter_id, operands, 2);
    }
    else
    {
        ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, SpvStorageClassImage, type_id);
        coordinate_id = sample_id = spirv_compiler_get_constant_uint(compiler, 0);
        pointer_id = vkd3d_spirv_build_op_image_texel_pointer(builder,
                ptr_type_id, counter_id, coordinate_id, sample_id);
    }

    operands[0] = pointer_id;
    operands[1] = spirv_compiler_get_constant_uint(compiler, SpvScopeDevice);
    operands[2] = spirv_compiler_get_constant_uint(compiler, memory_semantics);
    result_id = vkd3d_spirv_build_op_trv(builder, &builder->function_stream,
            op, type_id, operands, ARRAY_SIZE(operands));
    if (op == SpvOpAtomicIDecrement)
    {
        /* SpvOpAtomicIDecrement returns the original value. */
        result_id = vkd3d_spirv_build_op_isub(builder, type_id, result_id,
                spirv_compiler_get_constant_uint(compiler, 1));
    }
    spirv_compiler_emit_store_dst(compiler, dst, result_id);
}

static SpvOp spirv_compiler_map_atomic_instruction(const struct vkd3d_shader_instruction *instruction)
{
    static const struct
    {
        enum vkd3d_shader_opcode opcode;
        SpvOp spirv_op;
    }
    atomic_ops[] =
    {
        {VKD3DSIH_ATOMIC_AND,          SpvOpAtomicAnd},
        {VKD3DSIH_ATOMIC_CMP_STORE,    SpvOpAtomicCompareExchange},
        {VKD3DSIH_ATOMIC_IADD,         SpvOpAtomicIAdd},
        {VKD3DSIH_ATOMIC_IMAX,         SpvOpAtomicSMax},
        {VKD3DSIH_ATOMIC_IMIN,         SpvOpAtomicSMin},
        {VKD3DSIH_ATOMIC_OR,           SpvOpAtomicOr},
        {VKD3DSIH_ATOMIC_UMAX,         SpvOpAtomicUMax},
        {VKD3DSIH_ATOMIC_UMIN,         SpvOpAtomicUMin},
        {VKD3DSIH_ATOMIC_XOR,          SpvOpAtomicXor},
        {VKD3DSIH_IMM_ATOMIC_AND,      SpvOpAtomicAnd},
        {VKD3DSIH_IMM_ATOMIC_CMP_EXCH, SpvOpAtomicCompareExchange},
        {VKD3DSIH_IMM_ATOMIC_EXCH,     SpvOpAtomicExchange},
        {VKD3DSIH_IMM_ATOMIC_IADD,     SpvOpAtomicIAdd},
        {VKD3DSIH_IMM_ATOMIC_IMAX,     SpvOpAtomicSMax},
        {VKD3DSIH_IMM_ATOMIC_IMIN,     SpvOpAtomicSMin},
        {VKD3DSIH_IMM_ATOMIC_OR,       SpvOpAtomicOr},
        {VKD3DSIH_IMM_ATOMIC_UMAX,     SpvOpAtomicUMax},
        {VKD3DSIH_IMM_ATOMIC_UMIN,     SpvOpAtomicUMin},
        {VKD3DSIH_IMM_ATOMIC_XOR,      SpvOpAtomicXor},
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(atomic_ops); ++i)
    {
        if (atomic_ops[i].opcode == instruction->opcode)
            return atomic_ops[i].spirv_op;
    }

    return SpvOpMax;
}

static bool is_imm_atomic_instruction(enum vkd3d_shader_opcode opcode)
{
    return VKD3DSIH_IMM_ATOMIC_ALLOC <= opcode && opcode <= VKD3DSIH_IMM_ATOMIC_XOR;
}

static void spirv_compiler_emit_atomic_instruction(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    const struct vkd3d_symbol *resource_symbol = NULL;
    uint32_t ptr_type_id, type_id, val_id, result_id;
    enum vkd3d_shader_component_type component_type;
    const struct vkd3d_shader_dst_param *resource;
    uint32_t coordinate_id, sample_id, pointer_id;
    struct vkd3d_shader_register_info reg_info;
    SpvMemorySemanticsMask memory_semantic;
    struct vkd3d_shader_image image;
    unsigned int structure_stride;
    uint32_t coordinate_mask;
    uint32_t operands[6];
    unsigned int i = 0;
    SpvScope scope;
    bool raw;
    SpvOp op;

    resource = is_imm_atomic_instruction(instruction->opcode) ? &dst[1] : &dst[0];

    op = spirv_compiler_map_atomic_instruction(instruction);
    if (op == SpvOpMax)
    {
        ERR("Unexpected instruction %#x.\n", instruction->opcode);
        return;
    }

    if (resource->reg.type == VKD3DSPR_GROUPSHAREDMEM)
    {
        scope = SpvScopeWorkgroup;
        coordinate_mask = 1u;
        if (!spirv_compiler_get_register_info(compiler, &resource->reg, &reg_info))
            return;
        structure_stride = reg_info.structure_stride;
        raw = !structure_stride;
    }
    else
    {
        scope = SpvScopeDevice;
        resource_symbol = spirv_compiler_find_resource(compiler, &resource->reg);

        if (spirv_compiler_use_storage_buffer(compiler, &resource_symbol->info.resource))
        {
            coordinate_mask = VKD3DSP_WRITEMASK_0;
            structure_stride = resource_symbol->info.resource.structure_stride;
            raw = resource_symbol->info.resource.raw;
        }
        else
        {
            spirv_compiler_prepare_image(compiler, &image, &resource->reg, NULL, VKD3D_IMAGE_FLAG_NO_LOAD);
            coordinate_mask = (1u << image.resource_type_info->coordinate_component_count) - 1;
            structure_stride = image.structure_stride;
            raw = image.raw;
        }
    }

    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, 1);
    if (structure_stride || raw)
    {
        VKD3D_ASSERT(!raw != !structure_stride);
        coordinate_id = spirv_compiler_emit_raw_structured_addressing(compiler,
                type_id, structure_stride, &src[0], VKD3DSP_WRITEMASK_0,
                &src[0], VKD3DSP_WRITEMASK_1);
    }
    else
    {
        VKD3D_ASSERT(resource->reg.type != VKD3DSPR_GROUPSHAREDMEM);
        coordinate_id = spirv_compiler_emit_load_src(compiler, &src[0], coordinate_mask);
    }

    if (resource->reg.type == VKD3DSPR_GROUPSHAREDMEM)
    {
        component_type = VKD3D_SHADER_COMPONENT_UINT;
        ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, reg_info.storage_class, type_id);
        pointer_id = vkd3d_spirv_build_op_access_chain1(builder, ptr_type_id, reg_info.id, coordinate_id);
    }
    else
    {
        if (spirv_compiler_use_storage_buffer(compiler, &resource_symbol->info.resource))
        {
            component_type = resource_symbol->info.resource.sampled_type;
            type_id = vkd3d_spirv_get_type_id(builder, component_type, 1);
            ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, SpvStorageClassUniform, type_id);
            operands[0] = spirv_compiler_get_constant_uint(compiler, 0);
            operands[1] = coordinate_id;
            pointer_id = vkd3d_spirv_build_op_access_chain(builder, ptr_type_id, resource_symbol->id, operands, 2);
        }
        else
        {
            component_type = image.sampled_type;
            type_id = vkd3d_spirv_get_type_id(builder, image.sampled_type, 1);
            ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, SpvStorageClassImage, type_id);
            sample_id = spirv_compiler_get_constant_uint(compiler, 0);
            pointer_id = vkd3d_spirv_build_op_image_texel_pointer(builder,
                    ptr_type_id, image.id, coordinate_id, sample_id);
        }
    }

    val_id = spirv_compiler_emit_load_src_with_type(compiler, &src[1], VKD3DSP_WRITEMASK_0, component_type);

    if (instruction->flags & VKD3DARF_VOLATILE)
    {
        WARN("Ignoring 'volatile' attribute.\n");
        spirv_compiler_warning(compiler, VKD3D_SHADER_WARNING_SPV_IGNORING_FLAG,
                "Ignoring the 'volatile' attribute flag for atomic instruction %#x.", instruction->opcode);
    }

    memory_semantic = (instruction->flags & VKD3DARF_SEQ_CST)
            ? SpvMemorySemanticsSequentiallyConsistentMask
            : SpvMemorySemanticsMaskNone;

    operands[i++] = pointer_id;
    operands[i++] = spirv_compiler_get_constant_uint(compiler, scope);
    operands[i++] = spirv_compiler_get_constant_uint(compiler, memory_semantic);
    if (instruction->src_count >= 3)
    {
        operands[i++] = spirv_compiler_get_constant_uint(compiler, memory_semantic);
        operands[i++] = spirv_compiler_emit_load_src_with_type(compiler, &src[2], VKD3DSP_WRITEMASK_0, component_type);
    }
    operands[i++] = val_id;
    result_id = vkd3d_spirv_build_op_trv(builder, &builder->function_stream,
            op, type_id, operands, i);

    if (is_imm_atomic_instruction(instruction->opcode))
        spirv_compiler_emit_store_dst(compiler, dst, result_id);
}

static void spirv_compiler_emit_bufinfo(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    const struct vkd3d_symbol *resource_symbol;
    uint32_t type_id, val_id, stride_id;
    struct vkd3d_shader_image image;
    uint32_t constituents[2];
    unsigned int write_mask;

    if (compiler->ssbo_uavs && src->reg.type == VKD3DSPR_UAV)
    {
        resource_symbol = spirv_compiler_find_resource(compiler, &src->reg);

        type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, 1);
        val_id = vkd3d_spirv_build_op_array_length(builder, type_id, resource_symbol->id, 0);
        write_mask = VKD3DSP_WRITEMASK_0;
    }
    else
    {
        vkd3d_spirv_enable_capability(builder, SpvCapabilityImageQuery);

        spirv_compiler_prepare_image(compiler, &image, &src->reg, NULL, VKD3D_IMAGE_FLAG_NONE);

        type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, 1);
        val_id = vkd3d_spirv_build_op_image_query_size(builder, type_id, image.image_id);
        write_mask = VKD3DSP_WRITEMASK_0;
    }

    if (image.structure_stride)
    {
        stride_id = spirv_compiler_get_constant_uint(compiler, image.structure_stride);
        constituents[0] = vkd3d_spirv_build_op_udiv(builder, type_id, val_id, stride_id);
        constituents[1] = stride_id;
        type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, ARRAY_SIZE(constituents));
        val_id = vkd3d_spirv_build_op_composite_construct(builder,
                type_id, constituents, ARRAY_SIZE(constituents));
        write_mask |= VKD3DSP_WRITEMASK_1;
    }
    else if (image.raw)
    {
        val_id = vkd3d_spirv_build_op_shift_left_logical(builder, type_id,
                val_id, spirv_compiler_get_constant_uint(compiler, 2));
    }

    val_id = spirv_compiler_emit_swizzle(compiler, val_id, write_mask,
            VKD3D_SHADER_COMPONENT_UINT, src->swizzle, dst->write_mask);
    spirv_compiler_emit_store_dst(compiler, dst, val_id);
}

static void spirv_compiler_emit_resinfo(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t type_id, lod_id, val_id, miplevel_count_id;
    enum vkd3d_shader_component_type component_type;
    uint32_t constituents[VKD3D_VEC4_SIZE];
    unsigned int i, size_component_count;
    struct vkd3d_shader_image image;
    bool supports_mipmaps;

    vkd3d_spirv_enable_capability(builder, SpvCapabilityImageQuery);

    spirv_compiler_prepare_image(compiler, &image, &src[1].reg, NULL, VKD3D_IMAGE_FLAG_NONE);
    size_component_count = image.resource_type_info->coordinate_component_count;
    if (image.resource_type_info->dim == SpvDimCube)
        --size_component_count;
    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, size_component_count);

    supports_mipmaps = src[1].reg.type != VKD3DSPR_UAV && !image.resource_type_info->ms;
    if (supports_mipmaps)
    {
        lod_id = spirv_compiler_emit_load_src(compiler, &src[0], VKD3DSP_WRITEMASK_0);
        val_id = vkd3d_spirv_build_op_image_query_size_lod(builder, type_id, image.image_id, lod_id);
        type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, 1);
        miplevel_count_id = vkd3d_spirv_build_op_image_query_levels(builder, type_id, image.image_id);
    }
    else
    {
        val_id = vkd3d_spirv_build_op_image_query_size(builder, type_id, image.image_id);
        /* For UAVs the returned miplevel count is always 1. */
        miplevel_count_id = spirv_compiler_get_constant_uint(compiler, 1);
    }

    constituents[0] = val_id;
    for (i = 0; i < 3 - size_component_count; ++i)
        constituents[i + 1] = spirv_compiler_get_constant_uint(compiler, 0);
    constituents[i + 1] = miplevel_count_id;
    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, VKD3D_VEC4_SIZE);
    val_id = vkd3d_spirv_build_op_composite_construct(builder,
            type_id, constituents, i + 2);

    component_type = VKD3D_SHADER_COMPONENT_FLOAT;

    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_FLOAT, VKD3D_VEC4_SIZE);
    if (instruction->flags == VKD3DSI_RESINFO_UINT)
    {
        /* SSA registers must match the specified result type. */
        if (!register_is_ssa(&dst->reg))
            val_id = vkd3d_spirv_build_op_bitcast(builder, type_id, val_id);
        else
            component_type = VKD3D_SHADER_COMPONENT_UINT;
    }
    else
    {
        if (instruction->flags)
            FIXME("Unhandled flags %#x.\n", instruction->flags);
        val_id = vkd3d_spirv_build_op_convert_utof(builder, type_id, val_id);
    }
    val_id = spirv_compiler_emit_swizzle(compiler, val_id, VKD3DSP_WRITEMASK_ALL,
            component_type, src[1].swizzle, dst->write_mask);

    spirv_compiler_emit_store_dst(compiler, dst, val_id);
}

static uint32_t spirv_compiler_emit_query_sample_count(struct spirv_compiler *compiler,
        const struct vkd3d_shader_src_param *src)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    struct vkd3d_shader_image image;
    uint32_t type_id, val_id;

    if (src->reg.type == VKD3DSPR_RASTERIZER)
    {
        val_id = spirv_compiler_emit_shader_parameter(compiler,
                VKD3D_SHADER_PARAMETER_NAME_RASTERIZER_SAMPLE_COUNT, VKD3D_DATA_UINT, 1);
    }
    else
    {
        vkd3d_spirv_enable_capability(builder, SpvCapabilityImageQuery);

        spirv_compiler_prepare_image(compiler, &image, &src->reg, NULL, VKD3D_IMAGE_FLAG_NONE);
        type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, 1);
        val_id = vkd3d_spirv_build_op_image_query_samples(builder, type_id, image.image_id);
    }

    return val_id;
}

static void spirv_compiler_emit_sample_info(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t constituents[VKD3D_VEC4_SIZE];
    uint32_t type_id, val_id;
    unsigned int i;

    val_id = spirv_compiler_emit_query_sample_count(compiler, src);

    constituents[0] = val_id;
    for (i = 1; i < VKD3D_VEC4_SIZE; ++i)
        constituents[i] = spirv_compiler_get_constant_uint(compiler, 0);
    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, VKD3D_VEC4_SIZE);
    val_id = vkd3d_spirv_build_op_composite_construct(builder, type_id, constituents, VKD3D_VEC4_SIZE);

    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_FLOAT, VKD3D_VEC4_SIZE);
    if (instruction->flags == VKD3DSI_SAMPLE_INFO_UINT)
    {
        val_id = vkd3d_spirv_build_op_bitcast(builder, type_id, val_id);
    }
    else
    {
        if (instruction->flags)
            FIXME("Unhandled flags %#x.\n", instruction->flags);
        val_id = vkd3d_spirv_build_op_convert_utof(builder, type_id, val_id);
    }

    val_id = spirv_compiler_emit_swizzle(compiler, val_id, VKD3DSP_WRITEMASK_ALL,
            VKD3D_SHADER_COMPONENT_FLOAT, src->swizzle, dst->write_mask);

    spirv_compiler_emit_store_dst(compiler, dst, val_id);
}

/* XXX: This is correct only when standard sample positions are used. */
static void spirv_compiler_emit_sample_position(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    /* Standard sample locations from the Vulkan spec. */
    static const float standard_sample_positions[][2] =
    {
        /* 1 sample */
        { 0.0 / 16.0,  0.0 / 16.0},
        /* 2 samples */
        { 4.0 / 16.0,  4.0 / 16.0},
        {-4.0 / 16.0, -4.0 / 16.0},
        /* 4 samples */
        {-2.0 / 16.0, -6.0 / 16.0},
        { 6.0 / 16.0, -2.0 / 16.0},
        {-6.0 / 16.0,  2.0 / 16.0},
        { 2.0 / 16.0,  6.0 / 16.0},
        /* 8 samples */
        { 1.0 / 16.0, -3.0 / 16.0},
        {-1.0 / 16.0,  3.0 / 16.0},
        { 5.0 / 16.0,  1.0 / 16.0},
        {-3.0 / 16.0, -5.0 / 16.0},
        {-5.0 / 16.0,  5.0 / 16.0},
        {-7.0 / 16.0, -1.0 / 16.0},
        { 3.0 / 16.0,  7.0 / 16.0},
        { 7.0 / 16.0, -7.0 / 16.0},
        /* 16 samples */
        { 1.0 / 16.0,  1.0 / 16.0},
        {-1.0 / 16.0, -3.0 / 16.0},
        {-3.0 / 16.0,  2.0 / 16.0},
        { 4.0 / 16.0, -1.0 / 16.0},
        {-5.0 / 16.0, -2.0 / 16.0},
        { 2.0 / 16.0,  5.0 / 16.0},
        { 5.0 / 16.0,  3.0 / 16.0},
        { 3.0 / 16.0, -5.0 / 16.0},
        {-2.0 / 16.0,  6.0 / 16.0},
        { 0.0 / 16.0, -7.0 / 16.0},
        {-4.0 / 16.0, -6.0 / 16.0},
        {-6.0 / 16.0,  4.0 / 16.0},
        {-8.0 / 16.0,  0.0 / 16.0},
        { 7.0 / 16.0, -4.0 / 16.0},
        { 6.0 / 16.0,  7.0 / 16.0},
        {-7.0 / 16.0, -8.0 / 16.0},
    };
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t constituents[ARRAY_SIZE(standard_sample_positions)];
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    uint32_t array_type_id, length_id, index_id, id;
    uint32_t sample_count_id, sample_index_id;
    uint32_t type_id, bool_id, ptr_type_id;
    unsigned int i;

    sample_count_id = spirv_compiler_emit_query_sample_count(compiler, &instruction->src[0]);
    sample_index_id = spirv_compiler_emit_load_src(compiler, &instruction->src[1], VKD3DSP_WRITEMASK_0);

    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, 1);
    index_id = vkd3d_spirv_build_op_iadd(builder, type_id, sample_count_id, sample_index_id);
    index_id = vkd3d_spirv_build_op_isub(builder,
            type_id, index_id, spirv_compiler_get_constant_uint(compiler, 1));

    /* Validate sample index. */
    bool_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_BOOL, 1);
    id = vkd3d_spirv_build_op_logical_and(builder, bool_id,
            vkd3d_spirv_build_op_uless_than(builder, bool_id, sample_index_id, sample_count_id),
            vkd3d_spirv_build_op_uless_than_equal(builder,
                    bool_id, sample_index_id, spirv_compiler_get_constant_uint(compiler, 16)));
    index_id = vkd3d_spirv_build_op_select(builder, type_id,
            id, index_id, spirv_compiler_get_constant_uint(compiler, 0));

    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_FLOAT, 2);
    if (!(id = compiler->sample_positions_id))
    {
        length_id = spirv_compiler_get_constant_uint(compiler, ARRAY_SIZE(standard_sample_positions));
        array_type_id = vkd3d_spirv_get_op_type_array(builder, type_id, length_id);

        for (i = 0; i < ARRAY_SIZE(standard_sample_positions); ++ i)
        {
            constituents[i] = spirv_compiler_get_constant(compiler,
                    VKD3D_SHADER_COMPONENT_FLOAT, 2, (const uint32_t *)standard_sample_positions[i]);
        }

        id = vkd3d_spirv_build_op_constant_composite(builder, array_type_id, constituents, ARRAY_SIZE(constituents));
        ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, SpvStorageClassPrivate, array_type_id);
        id = vkd3d_spirv_build_op_variable(builder, &builder->global_stream, ptr_type_id, SpvStorageClassPrivate, id);
        vkd3d_spirv_build_op_name(builder, id, "sample_pos");
        compiler->sample_positions_id = id;
    }

    ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, SpvStorageClassPrivate, type_id);
    id = vkd3d_spirv_build_op_in_bounds_access_chain1(builder, ptr_type_id, id, index_id);
    id = vkd3d_spirv_build_op_load(builder, type_id, id, SpvMemoryAccessMaskNone);

    id = spirv_compiler_emit_swizzle(compiler, id, VKD3DSP_WRITEMASK_0 | VKD3DSP_WRITEMASK_1,
            VKD3D_SHADER_COMPONENT_FLOAT, instruction->src[0].swizzle, dst->write_mask);
    spirv_compiler_emit_store_dst(compiler, dst, id);
}

static void spirv_compiler_emit_eval_attrib(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    const struct vkd3d_shader_register *input = &src[0].reg;
    uint32_t instr_set_id, type_id, val_id, src_ids[2];
    struct vkd3d_shader_register_info register_info;
    unsigned int src_count = 0;
    enum GLSLstd450 op;

    if (!spirv_compiler_get_register_info(compiler, input, &register_info))
        return;

    if (register_info.storage_class != SpvStorageClassInput)
    {
        FIXME("Not supported for storage class %#x.\n", register_info.storage_class);
        return;
    }

    vkd3d_spirv_enable_capability(builder, SpvCapabilityInterpolationFunction);

    src_ids[src_count++] = register_info.id;

    if (instruction->opcode == VKD3DSIH_EVAL_CENTROID)
    {
        op = GLSLstd450InterpolateAtCentroid;
    }
    else
    {
        VKD3D_ASSERT(instruction->opcode == VKD3DSIH_EVAL_SAMPLE_INDEX);
        op = GLSLstd450InterpolateAtSample;
        src_ids[src_count++] = spirv_compiler_emit_load_src(compiler, &src[1], VKD3DSP_WRITEMASK_0);
    }

    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_FLOAT,
            vsir_write_mask_component_count(register_info.write_mask));

    instr_set_id = vkd3d_spirv_get_glsl_std450_instr_set(builder);
    val_id = vkd3d_spirv_build_op_ext_inst(builder, type_id, instr_set_id, op, src_ids, src_count);

    val_id = spirv_compiler_emit_swizzle(compiler, val_id, register_info.write_mask,
            VKD3D_SHADER_COMPONENT_FLOAT, src[0].swizzle, dst->write_mask);

    spirv_compiler_emit_store_dst(compiler, dst, val_id);
}

/* From the Vulkan spec:
 *
 *   "Scope for execution must be limited to: * Workgroup * Subgroup"
 *
 *   "Scope for memory must be limited to: * Device * Workgroup * Invocation"
 */
static void spirv_compiler_emit_sync(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    unsigned int memory_semantics = SpvMemorySemanticsAcquireReleaseMask;
    SpvScope execution_scope = SpvScopeMax;
    SpvScope memory_scope = SpvScopeDevice;
    uint32_t flags = instruction->flags;

    if (flags & VKD3DSSF_GROUP_SHARED_MEMORY)
    {
        memory_scope = SpvScopeWorkgroup;
        memory_semantics |= SpvMemorySemanticsWorkgroupMemoryMask;
        flags &= ~VKD3DSSF_GROUP_SHARED_MEMORY;
    }

    if (flags & VKD3DSSF_THREAD_GROUP)
    {
        execution_scope = SpvScopeWorkgroup;
        flags &= ~VKD3DSSF_THREAD_GROUP;
    }

    if (flags & (VKD3DSSF_THREAD_GROUP_UAV | VKD3DSSF_GLOBAL_UAV))
    {
        bool group_uav = flags & VKD3DSSF_THREAD_GROUP_UAV;
        bool global_uav = flags & VKD3DSSF_GLOBAL_UAV;

        if (group_uav && global_uav)
        {
            WARN("Invalid UAV sync flag combination; assuming global.\n");
            spirv_compiler_warning(compiler, VKD3D_SHADER_WARNING_SPV_INVALID_UAV_FLAGS,
                    "The flags for a UAV sync instruction are contradictory; assuming global sync.");
        }
        memory_scope = global_uav ? SpvScopeDevice : SpvScopeWorkgroup;
        memory_semantics |= SpvMemorySemanticsUniformMemoryMask | SpvMemorySemanticsImageMemoryMask;
        flags &= ~(VKD3DSSF_THREAD_GROUP_UAV | VKD3DSSF_GLOBAL_UAV);
    }

    if (flags)
    {
        FIXME("Unhandled sync flags %#x.\n", flags);
        memory_scope = SpvScopeDevice;
        execution_scope = SpvScopeWorkgroup;
        memory_semantics |= SpvMemorySemanticsUniformMemoryMask
                | SpvMemorySemanticsSubgroupMemoryMask
                | SpvMemorySemanticsWorkgroupMemoryMask
                | SpvMemorySemanticsCrossWorkgroupMemoryMask
                | SpvMemorySemanticsAtomicCounterMemoryMask
                | SpvMemorySemanticsImageMemoryMask;
    }

    spirv_compiler_emit_barrier(compiler, execution_scope, memory_scope, memory_semantics);
}

static void spirv_compiler_emit_emit_stream(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    unsigned int stream_idx;

    if (instruction->opcode == VKD3DSIH_EMIT_STREAM)
        stream_idx = instruction->src[0].reg.idx[0].offset;
    else
        stream_idx = 0;

    if (stream_idx)
    {
        FIXME("Multiple streams are not supported yet.\n");
        return;
    }

    spirv_compiler_emit_shader_epilogue_invocation(compiler);
    vkd3d_spirv_build_op_emit_vertex(builder);
}

static void spirv_compiler_emit_cut_stream(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    unsigned int stream_idx;

    if (instruction->opcode == VKD3DSIH_CUT_STREAM)
        stream_idx = instruction->src[0].reg.idx[0].offset;
    else
        stream_idx = 0;

    if (stream_idx)
    {
        FIXME("Multiple streams are not supported yet.\n");
        return;
    }

    vkd3d_spirv_build_op_end_primitive(builder);
}

static uint32_t map_quad_read_across_direction(enum vkd3d_shader_opcode opcode)
{
    switch (opcode)
    {
        case VKD3DSIH_QUAD_READ_ACROSS_X:
            return 0;
        case VKD3DSIH_QUAD_READ_ACROSS_Y:
            return 1;
        case VKD3DSIH_QUAD_READ_ACROSS_D:
            return 2;
        default:
            vkd3d_unreachable();
    }
}

static void spirv_compiler_emit_quad_read_across(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t type_id, direction_type_id, direction_id, val_id;

    type_id = vkd3d_spirv_get_type_id_for_data_type(builder, dst->reg.data_type,
            vsir_write_mask_component_count(dst->write_mask));
    direction_type_id = vkd3d_spirv_get_type_id_for_data_type(builder, VKD3D_DATA_UINT, 1);
    val_id = spirv_compiler_emit_load_src(compiler, src, dst->write_mask);
    direction_id = map_quad_read_across_direction(instruction->opcode);
    direction_id = vkd3d_spirv_get_op_constant(builder, direction_type_id, direction_id);
    val_id = vkd3d_spirv_build_op_group_nonuniform_quad_swap(builder, type_id, val_id, direction_id);

    spirv_compiler_emit_store_dst(compiler, dst, val_id);
}

static void spirv_compiler_emit_quad_read_lane_at(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t type_id, val_id, lane_id;

    if (!register_is_constant_or_undef(&src[1].reg))
    {
        FIXME("Unsupported non-constant quad read lane index.\n");
        spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_NOT_IMPLEMENTED,
                "Non-constant quad read lane indices are not supported.");
        return;
    }

    type_id = vkd3d_spirv_get_type_id_for_data_type(builder, dst->reg.data_type,
            vsir_write_mask_component_count(dst->write_mask));
    val_id = spirv_compiler_emit_load_src(compiler, &src[0], dst->write_mask);
    lane_id = spirv_compiler_emit_load_src(compiler, &src[1], VKD3DSP_WRITEMASK_0);
    val_id = vkd3d_spirv_build_op_group_nonuniform_quad_broadcast(builder, type_id, val_id, lane_id);

    spirv_compiler_emit_store_dst(compiler, dst, val_id);
}

static SpvOp map_wave_bool_op(enum vkd3d_shader_opcode opcode)
{
    switch (opcode)
    {
        case VKD3DSIH_WAVE_ACTIVE_ALL_EQUAL:
            return SpvOpGroupNonUniformAllEqual;
        case VKD3DSIH_WAVE_ALL_TRUE:
            return SpvOpGroupNonUniformAll;
        case VKD3DSIH_WAVE_ANY_TRUE:
            return SpvOpGroupNonUniformAny;
        default:
            vkd3d_unreachable();
    }
}

static void spirv_compiler_emit_wave_bool_op(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t type_id, val_id;
    SpvOp op;

    vkd3d_spirv_enable_capability(builder, SpvCapabilityGroupNonUniformVote);

    op = map_wave_bool_op(instruction->opcode);
    type_id = vkd3d_spirv_get_op_type_bool(builder);
    val_id = spirv_compiler_emit_load_src(compiler, src, dst->write_mask);
    val_id = vkd3d_spirv_build_op_tr2(builder, &builder->function_stream, op,
            type_id, vkd3d_spirv_get_op_scope_subgroup(builder), val_id);

    spirv_compiler_emit_store_dst(compiler, dst, val_id);
}

static uint32_t spirv_compiler_emit_group_nonuniform_ballot(struct spirv_compiler *compiler,
        const struct vkd3d_shader_src_param *src)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    uint32_t type_id, val_id;

    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, VKD3D_VEC4_SIZE);
    val_id = spirv_compiler_emit_load_src(compiler, src, VKD3DSP_WRITEMASK_0);
    val_id = vkd3d_spirv_build_op_group_nonuniform_ballot(builder, type_id, val_id);

    return val_id;
}

static void spirv_compiler_emit_wave_active_ballot(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    uint32_t val_id;

    val_id = spirv_compiler_emit_group_nonuniform_ballot(compiler, instruction->src);
    spirv_compiler_emit_store_dst(compiler, dst, val_id);
}

static SpvOp map_wave_alu_op(enum vkd3d_shader_opcode opcode, bool is_float)
{
    switch (opcode)
    {
        case VKD3DSIH_WAVE_ACTIVE_BIT_AND:
            return SpvOpGroupNonUniformBitwiseAnd;
        case VKD3DSIH_WAVE_ACTIVE_BIT_OR:
            return SpvOpGroupNonUniformBitwiseOr;
        case VKD3DSIH_WAVE_ACTIVE_BIT_XOR:
            return SpvOpGroupNonUniformBitwiseXor;
        case VKD3DSIH_WAVE_OP_ADD:
            return is_float ? SpvOpGroupNonUniformFAdd : SpvOpGroupNonUniformIAdd;
        case VKD3DSIH_WAVE_OP_IMAX:
            return SpvOpGroupNonUniformSMax;
        case VKD3DSIH_WAVE_OP_IMIN:
            return SpvOpGroupNonUniformSMin;
        case VKD3DSIH_WAVE_OP_MAX:
            return SpvOpGroupNonUniformFMax;
        case VKD3DSIH_WAVE_OP_MIN:
            return SpvOpGroupNonUniformFMin;
        case VKD3DSIH_WAVE_OP_MUL:
            return is_float ? SpvOpGroupNonUniformFMul : SpvOpGroupNonUniformIMul;
        case VKD3DSIH_WAVE_OP_UMAX:
            return SpvOpGroupNonUniformUMax;
        case VKD3DSIH_WAVE_OP_UMIN:
            return SpvOpGroupNonUniformUMin;
        default:
            vkd3d_unreachable();
    }
}

static void spirv_compiler_emit_wave_alu_op(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t type_id, val_id;
    SpvOp op;

    op = map_wave_alu_op(instruction->opcode, data_type_is_floating_point(src->reg.data_type));

    type_id = vkd3d_spirv_get_type_id_for_data_type(builder, dst->reg.data_type,
            vsir_write_mask_component_count(dst->write_mask));
    val_id = spirv_compiler_emit_load_src(compiler, &src[0], dst->write_mask);

    vkd3d_spirv_enable_capability(builder, SpvCapabilityGroupNonUniformArithmetic);
    val_id = vkd3d_spirv_build_op_tr3(builder, &builder->function_stream, op, type_id,
            vkd3d_spirv_get_op_scope_subgroup(builder),
            (instruction->flags & VKD3DSI_WAVE_PREFIX) ? SpvGroupOperationExclusiveScan : SpvGroupOperationReduce,
            val_id);

    spirv_compiler_emit_store_dst(compiler, dst, val_id);
}

static void spirv_compiler_emit_wave_bit_count(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    SpvGroupOperation group_op;
    uint32_t type_id, val_id;

    group_op = (instruction->opcode == VKD3DSIH_WAVE_PREFIX_BIT_COUNT) ? SpvGroupOperationExclusiveScan
            : SpvGroupOperationReduce;

    val_id = spirv_compiler_emit_group_nonuniform_ballot(compiler, instruction->src);
    type_id = vkd3d_spirv_get_type_id(builder, VKD3D_SHADER_COMPONENT_UINT, 1);
    val_id = vkd3d_spirv_build_op_group_nonuniform_ballot_bit_count(builder, type_id, group_op, val_id);

    spirv_compiler_emit_store_dst(compiler, dst, val_id);
}

static void spirv_compiler_emit_wave_is_first_lane(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    uint32_t val_id;

    val_id = vkd3d_spirv_build_op_group_nonuniform_elect(builder);
    spirv_compiler_emit_store_dst(compiler, dst, val_id);
}

static void spirv_compiler_emit_wave_read_lane_at(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t type_id, lane_id, val_id;

    type_id = vkd3d_spirv_get_type_id_for_data_type(builder, dst->reg.data_type,
            vsir_write_mask_component_count(dst->write_mask));
    val_id = spirv_compiler_emit_load_src(compiler, &src[0], dst->write_mask);
    lane_id = spirv_compiler_emit_load_src(compiler, &src[1], VKD3DSP_WRITEMASK_0);

    /* TODO: detect values loaded from a const buffer? */
    if (register_is_constant_or_undef(&src[1].reg))
    {
        /* Uniform lane_id only. */
        val_id = vkd3d_spirv_build_op_group_nonuniform_broadcast(builder, type_id, val_id, lane_id);
    }
    else
    {
        /* WaveReadLaneAt supports non-uniform lane ids, so if lane_id is not constant it may not be uniform. */
        val_id = vkd3d_spirv_build_op_group_nonuniform_shuffle(builder, type_id, val_id, lane_id);
    }

    spirv_compiler_emit_store_dst(compiler, dst, val_id);
}

static void spirv_compiler_emit_wave_read_lane_first(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    const struct vkd3d_shader_dst_param *dst = instruction->dst;
    const struct vkd3d_shader_src_param *src = instruction->src;
    uint32_t type_id, val_id;

    type_id = vkd3d_spirv_get_type_id_for_data_type(builder, dst->reg.data_type,
            vsir_write_mask_component_count(dst->write_mask));
    val_id = spirv_compiler_emit_load_src(compiler, src, dst->write_mask);
    val_id = vkd3d_spirv_build_op_group_nonuniform_broadcast_first(builder, type_id, val_id);

    spirv_compiler_emit_store_dst(compiler, dst, val_id);
}

/* This function is called after declarations are processed. */
static void spirv_compiler_emit_main_prolog(struct spirv_compiler *compiler)
{
    spirv_compiler_emit_push_constant_buffers(compiler);

    if (compiler->emit_point_size)
        spirv_compiler_emit_point_size(compiler);

    /* Maybe in the future we can try to shrink the size of the interlocked
     * section. */
    if (compiler->use_invocation_interlock)
        vkd3d_spirv_build_op(&compiler->spirv_builder.function_stream, SpvOpBeginInvocationInterlockEXT);
}

static int spirv_compiler_handle_instruction(struct spirv_compiler *compiler,
        const struct vkd3d_shader_instruction *instruction)
{
    int ret = VKD3D_OK;

    compiler->location = instruction->location;

    switch (instruction->opcode)
    {
        case VKD3DSIH_DCL_INDEXABLE_TEMP:
            spirv_compiler_emit_dcl_indexable_temp(compiler, instruction);
            break;
        case VKD3DSIH_DCL_IMMEDIATE_CONSTANT_BUFFER:
            spirv_compiler_emit_dcl_immediate_constant_buffer(compiler, instruction);
            break;
        case VKD3DSIH_DCL_TGSM_RAW:
            spirv_compiler_emit_dcl_tgsm_raw(compiler, instruction);
            break;
        case VKD3DSIH_DCL_TGSM_STRUCTURED:
            spirv_compiler_emit_dcl_tgsm_structured(compiler, instruction);
            break;
        case VKD3DSIH_DCL_INPUT_PS:
        case VKD3DSIH_DCL_INPUT:
            spirv_compiler_emit_dcl_input(compiler, instruction);
            break;
        case VKD3DSIH_DCL_OUTPUT:
            spirv_compiler_emit_dcl_output(compiler, instruction);
            break;
        case VKD3DSIH_DCL_STREAM:
            spirv_compiler_emit_dcl_stream(compiler, instruction);
            break;
        case VKD3DSIH_DCL_VERTICES_OUT:
            spirv_compiler_emit_output_vertex_count(compiler, instruction);
            break;
        case VKD3DSIH_DCL_INPUT_PRIMITIVE:
            spirv_compiler_emit_dcl_input_primitive(compiler, instruction);
            break;
        case VKD3DSIH_DCL_OUTPUT_TOPOLOGY:
            spirv_compiler_emit_dcl_output_topology(compiler, instruction);
            break;
        case VKD3DSIH_DCL_GS_INSTANCES:
            spirv_compiler_emit_dcl_gs_instances(compiler, instruction);
            break;
        case VKD3DSIH_DCL_OUTPUT_CONTROL_POINT_COUNT:
            spirv_compiler_emit_output_vertex_count(compiler, instruction);
            break;
        case VKD3DSIH_DCL_TESSELLATOR_DOMAIN:
            spirv_compiler_emit_dcl_tessellator_domain(compiler, instruction);
            break;
        case VKD3DSIH_DCL_TESSELLATOR_OUTPUT_PRIMITIVE:
            spirv_compiler_emit_tessellator_output_primitive(compiler,
                    instruction->declaration.tessellator_output_primitive);
            break;
        case VKD3DSIH_DCL_TESSELLATOR_PARTITIONING:
            spirv_compiler_emit_tessellator_partitioning(compiler,
                    instruction->declaration.tessellator_partitioning);
            break;
        case VKD3DSIH_HS_CONTROL_POINT_PHASE:
        case VKD3DSIH_HS_FORK_PHASE:
        case VKD3DSIH_HS_JOIN_PHASE:
            spirv_compiler_enter_shader_phase(compiler, instruction);
            break;
        case VKD3DSIH_DMOV:
        case VKD3DSIH_MOV:
            spirv_compiler_emit_mov(compiler, instruction);
            break;
        case VKD3DSIH_DMOVC:
        case VKD3DSIH_MOVC:
        case VKD3DSIH_CMP:
            spirv_compiler_emit_movc(compiler, instruction);
            break;
        case VKD3DSIH_SWAPC:
            spirv_compiler_emit_swapc(compiler, instruction);
            break;
        case VKD3DSIH_ADD:
        case VKD3DSIH_AND:
        case VKD3DSIH_BFREV:
        case VKD3DSIH_COUNTBITS:
        case VKD3DSIH_DADD:
        case VKD3DSIH_DDIV:
        case VKD3DSIH_DIV:
        case VKD3DSIH_DMUL:
        case VKD3DSIH_DTOF:
        case VKD3DSIH_FREM:
        case VKD3DSIH_FTOD:
        case VKD3DSIH_IADD:
        case VKD3DSIH_INEG:
        case VKD3DSIH_ISHL:
        case VKD3DSIH_ISHR:
        case VKD3DSIH_ISINF:
        case VKD3DSIH_ISNAN:
        case VKD3DSIH_ITOD:
        case VKD3DSIH_ITOF:
        case VKD3DSIH_ITOI:
        case VKD3DSIH_MUL:
        case VKD3DSIH_NOT:
        case VKD3DSIH_OR:
        case VKD3DSIH_USHR:
        case VKD3DSIH_UTOD:
        case VKD3DSIH_UTOF:
        case VKD3DSIH_UTOU:
        case VKD3DSIH_XOR:
            ret = spirv_compiler_emit_alu_instruction(compiler, instruction);
            break;
        case VKD3DSIH_ISFINITE:
            spirv_compiler_emit_isfinite(compiler, instruction);
            break;
        case VKD3DSIH_ABS:
        case VKD3DSIH_ACOS:
        case VKD3DSIH_ASIN:
        case VKD3DSIH_ATAN:
        case VKD3DSIH_HCOS:
        case VKD3DSIH_HSIN:
        case VKD3DSIH_HTAN:
        case VKD3DSIH_DFMA:
        case VKD3DSIH_DMAX:
        case VKD3DSIH_DMIN:
        case VKD3DSIH_EXP:
        case VKD3DSIH_FIRSTBIT_HI:
        case VKD3DSIH_FIRSTBIT_LO:
        case VKD3DSIH_FIRSTBIT_SHI:
        case VKD3DSIH_FRC:
        case VKD3DSIH_IMAX:
        case VKD3DSIH_IMIN:
        case VKD3DSIH_LOG:
        case VKD3DSIH_MAD:
        case VKD3DSIH_MAX:
        case VKD3DSIH_MIN:
        case VKD3DSIH_ROUND_NE:
        case VKD3DSIH_ROUND_NI:
        case VKD3DSIH_ROUND_PI:
        case VKD3DSIH_ROUND_Z:
        case VKD3DSIH_RSQ:
        case VKD3DSIH_SQRT:
        case VKD3DSIH_TAN:
        case VKD3DSIH_UMAX:
        case VKD3DSIH_UMIN:
            spirv_compiler_emit_ext_glsl_instruction(compiler, instruction);
            break;
        case VKD3DSIH_DP4:
        case VKD3DSIH_DP3:
        case VKD3DSIH_DP2:
            spirv_compiler_emit_dot(compiler, instruction);
            break;
        case VKD3DSIH_DRCP:
        case VKD3DSIH_RCP:
            spirv_compiler_emit_rcp(compiler, instruction);
            break;
        case VKD3DSIH_SINCOS:
            spirv_compiler_emit_sincos(compiler, instruction);
            break;
        case VKD3DSIH_IMUL:
        case VKD3DSIH_UMUL:
            spirv_compiler_emit_imul(compiler, instruction);
            break;
        case VKD3DSIH_IMAD:
            spirv_compiler_emit_imad(compiler, instruction);
            break;
        case VKD3DSIH_IDIV:
        case VKD3DSIH_UDIV:
            spirv_compiler_emit_int_div(compiler, instruction);
            break;
        case VKD3DSIH_DTOI:
        case VKD3DSIH_FTOI:
            spirv_compiler_emit_ftoi(compiler, instruction);
            break;
        case VKD3DSIH_DTOU:
        case VKD3DSIH_FTOU:
            spirv_compiler_emit_ftou(compiler, instruction);
            break;
        case VKD3DSIH_DEQO:
        case VKD3DSIH_DGEO:
        case VKD3DSIH_DLT:
        case VKD3DSIH_DNE:
        case VKD3DSIH_EQO:
        case VKD3DSIH_EQU:
        case VKD3DSIH_GEO:
        case VKD3DSIH_GEU:
        case VKD3DSIH_IEQ:
        case VKD3DSIH_IGE:
        case VKD3DSIH_ILT:
        case VKD3DSIH_INE:
        case VKD3DSIH_LTO:
        case VKD3DSIH_LTU:
        case VKD3DSIH_NEO:
        case VKD3DSIH_NEU:
        case VKD3DSIH_UGE:
        case VKD3DSIH_ULT:
            spirv_compiler_emit_comparison_instruction(compiler, instruction);
            break;
        case VKD3DSIH_ORD:
        case VKD3DSIH_UNO:
            spirv_compiler_emit_orderedness_instruction(compiler, instruction);
            break;
        case VKD3DSIH_SLT:
        case VKD3DSIH_SGE:
            spirv_compiler_emit_float_comparison_instruction(compiler, instruction);
            break;
        case VKD3DSIH_BFI:
        case VKD3DSIH_IBFE:
        case VKD3DSIH_UBFE:
            spirv_compiler_emit_bitfield_instruction(compiler, instruction);
            break;
        case VKD3DSIH_F16TOF32:
            spirv_compiler_emit_f16tof32(compiler, instruction);
            break;
        case VKD3DSIH_F32TOF16:
            spirv_compiler_emit_f32tof16(compiler, instruction);
            break;
        case VKD3DSIH_RET:
            spirv_compiler_emit_return(compiler, instruction);
            break;
        case VKD3DSIH_RETP:
            spirv_compiler_emit_retc(compiler, instruction);
            break;
        case VKD3DSIH_DISCARD:
            spirv_compiler_emit_discard(compiler, instruction);
            break;
        case VKD3DSIH_LABEL:
            spirv_compiler_emit_label(compiler, instruction);
            break;
        case VKD3DSIH_BRANCH:
            spirv_compiler_emit_branch(compiler, instruction);
            break;
        case VKD3DSIH_SWITCH_MONOLITHIC:
            spirv_compiler_emit_switch(compiler, instruction);
            break;
        case VKD3DSIH_DSX:
        case VKD3DSIH_DSX_COARSE:
        case VKD3DSIH_DSX_FINE:
        case VKD3DSIH_DSY:
        case VKD3DSIH_DSY_COARSE:
        case VKD3DSIH_DSY_FINE:
            spirv_compiler_emit_deriv_instruction(compiler, instruction);
            break;
        case VKD3DSIH_LD2DMS:
        case VKD3DSIH_LD:
            spirv_compiler_emit_ld(compiler, instruction);
            break;
        case VKD3DSIH_LOD:
            spirv_compiler_emit_lod(compiler, instruction);
            break;
        case VKD3DSIH_SAMPLE:
        case VKD3DSIH_SAMPLE_B:
        case VKD3DSIH_SAMPLE_GRAD:
        case VKD3DSIH_SAMPLE_LOD:
            spirv_compiler_emit_sample(compiler, instruction);
            break;
        case VKD3DSIH_SAMPLE_C:
        case VKD3DSIH_SAMPLE_C_LZ:
            spirv_compiler_emit_sample_c(compiler, instruction);
            break;
        case VKD3DSIH_GATHER4:
        case VKD3DSIH_GATHER4_C:
        case VKD3DSIH_GATHER4_PO:
        case VKD3DSIH_GATHER4_PO_C:
            spirv_compiler_emit_gather4(compiler, instruction);
            break;
        case VKD3DSIH_LD_RAW:
        case VKD3DSIH_LD_STRUCTURED:
            spirv_compiler_emit_ld_raw_structured(compiler, instruction);
            break;
        case VKD3DSIH_STORE_RAW:
        case VKD3DSIH_STORE_STRUCTURED:
            spirv_compiler_emit_store_raw_structured(compiler, instruction);
            break;
        case VKD3DSIH_LD_UAV_TYPED:
            spirv_compiler_emit_ld_uav_typed(compiler, instruction);
            break;
        case VKD3DSIH_STORE_UAV_TYPED:
            spirv_compiler_emit_store_uav_typed(compiler, instruction);
            break;
        case VKD3DSIH_IMM_ATOMIC_ALLOC:
        case VKD3DSIH_IMM_ATOMIC_CONSUME:
            spirv_compiler_emit_uav_counter_instruction(compiler, instruction);
            break;
        case VKD3DSIH_ATOMIC_AND:
        case VKD3DSIH_ATOMIC_CMP_STORE:
        case VKD3DSIH_ATOMIC_IADD:
        case VKD3DSIH_ATOMIC_IMAX:
        case VKD3DSIH_ATOMIC_IMIN:
        case VKD3DSIH_ATOMIC_OR:
        case VKD3DSIH_ATOMIC_UMAX:
        case VKD3DSIH_ATOMIC_UMIN:
        case VKD3DSIH_ATOMIC_XOR:
        case VKD3DSIH_IMM_ATOMIC_AND:
        case VKD3DSIH_IMM_ATOMIC_CMP_EXCH:
        case VKD3DSIH_IMM_ATOMIC_EXCH:
        case VKD3DSIH_IMM_ATOMIC_IADD:
        case VKD3DSIH_IMM_ATOMIC_IMAX:
        case VKD3DSIH_IMM_ATOMIC_IMIN:
        case VKD3DSIH_IMM_ATOMIC_OR:
        case VKD3DSIH_IMM_ATOMIC_UMAX:
        case VKD3DSIH_IMM_ATOMIC_UMIN:
        case VKD3DSIH_IMM_ATOMIC_XOR:
            spirv_compiler_emit_atomic_instruction(compiler, instruction);
            break;
        case VKD3DSIH_BUFINFO:
            spirv_compiler_emit_bufinfo(compiler, instruction);
            break;
        case VKD3DSIH_RESINFO:
            spirv_compiler_emit_resinfo(compiler, instruction);
            break;
        case VKD3DSIH_SAMPLE_INFO:
            spirv_compiler_emit_sample_info(compiler, instruction);
            break;
        case VKD3DSIH_SAMPLE_POS:
            spirv_compiler_emit_sample_position(compiler, instruction);
            break;
        case VKD3DSIH_EVAL_CENTROID:
        case VKD3DSIH_EVAL_SAMPLE_INDEX:
            spirv_compiler_emit_eval_attrib(compiler, instruction);
            break;
        case VKD3DSIH_SYNC:
            spirv_compiler_emit_sync(compiler, instruction);
            break;
        case VKD3DSIH_EMIT:
        case VKD3DSIH_EMIT_STREAM:
            spirv_compiler_emit_emit_stream(compiler, instruction);
            break;
        case VKD3DSIH_CUT:
        case VKD3DSIH_CUT_STREAM:
            spirv_compiler_emit_cut_stream(compiler, instruction);
            break;
        case VKD3DSIH_QUAD_READ_ACROSS_D:
        case VKD3DSIH_QUAD_READ_ACROSS_X:
        case VKD3DSIH_QUAD_READ_ACROSS_Y:
            spirv_compiler_emit_quad_read_across(compiler, instruction);
            break;
        case VKD3DSIH_QUAD_READ_LANE_AT:
            spirv_compiler_emit_quad_read_lane_at(compiler, instruction);
            break;
        case VKD3DSIH_WAVE_ACTIVE_ALL_EQUAL:
        case VKD3DSIH_WAVE_ALL_TRUE:
        case VKD3DSIH_WAVE_ANY_TRUE:
            spirv_compiler_emit_wave_bool_op(compiler, instruction);
            break;
        case VKD3DSIH_WAVE_ACTIVE_BALLOT:
            spirv_compiler_emit_wave_active_ballot(compiler, instruction);
            break;
        case VKD3DSIH_WAVE_ACTIVE_BIT_AND:
        case VKD3DSIH_WAVE_ACTIVE_BIT_OR:
        case VKD3DSIH_WAVE_ACTIVE_BIT_XOR:
        case VKD3DSIH_WAVE_OP_ADD:
        case VKD3DSIH_WAVE_OP_IMAX:
        case VKD3DSIH_WAVE_OP_IMIN:
        case VKD3DSIH_WAVE_OP_MAX:
        case VKD3DSIH_WAVE_OP_MIN:
        case VKD3DSIH_WAVE_OP_MUL:
        case VKD3DSIH_WAVE_OP_UMAX:
        case VKD3DSIH_WAVE_OP_UMIN:
            spirv_compiler_emit_wave_alu_op(compiler, instruction);
            break;
        case VKD3DSIH_WAVE_ALL_BIT_COUNT:
        case VKD3DSIH_WAVE_PREFIX_BIT_COUNT:
            spirv_compiler_emit_wave_bit_count(compiler, instruction);
            break;
        case VKD3DSIH_WAVE_IS_FIRST_LANE:
            spirv_compiler_emit_wave_is_first_lane(compiler, instruction);
            break;
        case VKD3DSIH_WAVE_READ_LANE_AT:
            spirv_compiler_emit_wave_read_lane_at(compiler, instruction);
            break;
        case VKD3DSIH_WAVE_READ_LANE_FIRST:
            spirv_compiler_emit_wave_read_lane_first(compiler, instruction);
            break;
        case VKD3DSIH_DCL_HS_MAX_TESSFACTOR:
        case VKD3DSIH_DCL_INPUT_CONTROL_POINT_COUNT:
        case VKD3DSIH_DCL_INPUT_SGV:
        case VKD3DSIH_DCL_INPUT_SIV:
        case VKD3DSIH_DCL_INPUT_PS_SGV:
        case VKD3DSIH_DCL_INPUT_PS_SIV:
        case VKD3DSIH_DCL_OUTPUT_SIV:
        case VKD3DSIH_DCL_RESOURCE_RAW:
        case VKD3DSIH_DCL_RESOURCE_STRUCTURED:
        case VKD3DSIH_DCL_UAV_RAW:
        case VKD3DSIH_DCL_UAV_STRUCTURED:
        case VKD3DSIH_HS_DECLS:
        case VKD3DSIH_NOP:
            /* nothing to do */
            break;
        default:
            FIXME("Unhandled instruction %#x.\n", instruction->opcode);
            spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_INVALID_HANDLER,
                    "Encountered invalid/unhandled instruction handler %#x.", instruction->opcode);
            break;
    }

    return ret;
}

static void spirv_compiler_emit_io_declarations(struct spirv_compiler *compiler)
{
    for (unsigned int i = 0; i < compiler->input_signature.element_count; ++i)
        spirv_compiler_emit_input(compiler, VKD3DSPR_INPUT, i);

    for (unsigned int i = 0; i < compiler->output_signature.element_count; ++i)
    {
        /* PS outputs other than TARGET have dedicated registers and therefore
         * go through spirv_compiler_emit_dcl_output() for now. */
        if (compiler->shader_type == VKD3D_SHADER_TYPE_PIXEL
                && compiler->output_signature.elements[i].sysval_semantic != VKD3D_SHADER_SV_TARGET)
            continue;
        spirv_compiler_emit_output(compiler, VKD3DSPR_OUTPUT, i);
    }

    for (unsigned int i = 0; i < compiler->patch_constant_signature.element_count; ++i)
    {
        if (compiler->shader_type == VKD3D_SHADER_TYPE_HULL)
            spirv_compiler_emit_output(compiler, VKD3DSPR_PATCHCONST, i);
        else
            spirv_compiler_emit_input(compiler, VKD3DSPR_PATCHCONST, i);
    }

    if (compiler->program->has_point_size)
    {
        struct vkd3d_shader_dst_param dst;

        vsir_dst_param_init(&dst, VKD3DSPR_RASTOUT, VKD3D_DATA_FLOAT, 1);
        dst.reg.idx[0].offset = VSIR_RASTOUT_POINT_SIZE;
        spirv_compiler_emit_output_register(compiler, &dst);
    }

    if (compiler->program->has_point_coord)
    {
        struct vkd3d_shader_dst_param dst;

        vsir_dst_param_init(&dst, VKD3DSPR_POINT_COORD, VKD3D_DATA_FLOAT, 0);
        spirv_compiler_emit_input_register(compiler, &dst);
    }
}

static void spirv_compiler_emit_descriptor_declarations(struct spirv_compiler *compiler)
{
    unsigned int i;

    for (i = 0; i < compiler->scan_descriptor_info->descriptor_count; ++i)
    {
        const struct vkd3d_shader_descriptor_info1 *descriptor = &compiler->scan_descriptor_info->descriptors[i];
        struct vkd3d_shader_register_range range;

        range.first = descriptor->register_index;
        if (descriptor->count == ~0u)
            range.last = ~0u;
        else
            range.last = descriptor->register_index + descriptor->count - 1;
        range.space = descriptor->register_space;

        switch (descriptor->type)
        {
            case VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER:
                spirv_compiler_emit_sampler_declaration(compiler, &range, descriptor);
                break;

            case VKD3D_SHADER_DESCRIPTOR_TYPE_CBV:
                spirv_compiler_emit_cbv_declaration(compiler, &range, descriptor);
                break;

            case VKD3D_SHADER_DESCRIPTOR_TYPE_SRV:
            case VKD3D_SHADER_DESCRIPTOR_TYPE_UAV:
                spirv_compiler_emit_resource_declaration(compiler, &range, descriptor);
                break;

            default:
                vkd3d_unreachable();
        }
    }
}

static int spirv_compiler_generate_spirv(struct spirv_compiler *compiler, struct vsir_program *program,
        const struct vkd3d_shader_compile_info *compile_info, struct vkd3d_shader_code *spirv)
{
    const struct vkd3d_shader_spirv_target_info *info = compiler->spirv_target_info;
    const struct vkd3d_shader_spirv_domain_shader_target_info *ds_info;
    struct vkd3d_spirv_builder *builder = &compiler->spirv_builder;
    struct vkd3d_shader_instruction_array instructions;
    enum vkd3d_shader_spirv_environment environment;
    enum vkd3d_result result = VKD3D_OK;
    unsigned int i, max_element_count;

    if ((result = vsir_program_transform(program, compiler->config_flags,
            compile_info, compiler->message_context)) < 0)
        return result;

    VKD3D_ASSERT(program->normalisation_level == VSIR_FULLY_NORMALISED_IO);

    max_element_count = max(program->output_signature.element_count, program->patch_constant_signature.element_count);
    if (!(compiler->output_info = vkd3d_calloc(max_element_count, sizeof(*compiler->output_info))))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    if (program->temp_count)
        spirv_compiler_emit_temps(compiler, program->temp_count);
    if (program->ssa_count)
        spirv_compiler_allocate_ssa_register_ids(compiler, program->ssa_count);
    if (compiler->shader_type == VKD3D_SHADER_TYPE_COMPUTE)
        spirv_compiler_emit_thread_group_size(compiler, &program->thread_group_size);
    spirv_compiler_emit_global_flags(compiler, program->global_flags);

    spirv_compiler_emit_descriptor_declarations(compiler);

    compiler->spirv_parameter_info = vkd3d_calloc(program->parameter_count, sizeof(*compiler->spirv_parameter_info));
    for (i = 0; i < program->parameter_count; ++i)
    {
        const struct vkd3d_shader_parameter1 *parameter = &program->parameters[i];

        if (parameter->type == VKD3D_SHADER_PARAMETER_TYPE_BUFFER)
        {
            uint32_t type_id, struct_id, ptr_type_id, var_id;

            type_id = vkd3d_spirv_get_type_id(builder,
                    vkd3d_component_type_from_data_type(parameter_data_type_map[parameter->data_type].type),
                    parameter_data_type_map[parameter->data_type].component_count);

            struct_id = vkd3d_spirv_build_op_type_struct(builder, &type_id, 1);
            vkd3d_spirv_build_op_decorate(builder, struct_id, SpvDecorationBlock, NULL, 0);
            vkd3d_spirv_build_op_member_decorate1(builder, struct_id, 0,
                    SpvDecorationOffset, parameter->u.buffer.offset);

            ptr_type_id = vkd3d_spirv_get_op_type_pointer(builder, SpvStorageClassUniform, struct_id);
            var_id = vkd3d_spirv_build_op_variable(builder, &builder->global_stream,
                    ptr_type_id, SpvStorageClassUniform, 0);

            vkd3d_spirv_build_op_decorate1(builder, var_id, SpvDecorationDescriptorSet, parameter->u.buffer.set);
            vkd3d_spirv_build_op_decorate1(builder, var_id, SpvDecorationBinding, parameter->u.buffer.binding);

            compiler->spirv_parameter_info[i].buffer_id = var_id;
        }
    }

    if (program->block_count && !spirv_compiler_init_blocks(compiler, program->block_count))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    compiler->program = program;

    instructions = program->instructions;
    memset(&program->instructions, 0, sizeof(program->instructions));

    compiler->input_signature = program->input_signature;
    compiler->output_signature = program->output_signature;
    compiler->patch_constant_signature = program->patch_constant_signature;
    memset(&program->input_signature, 0, sizeof(program->input_signature));
    memset(&program->output_signature, 0, sizeof(program->output_signature));
    memset(&program->patch_constant_signature, 0, sizeof(program->patch_constant_signature));
    compiler->use_vocp = program->use_vocp;
    compiler->block_names = program->block_names;
    compiler->block_name_count = program->block_name_count;

    compiler->input_control_point_count = program->input_control_point_count;
    compiler->output_control_point_count = program->output_control_point_count;

    if (compiler->shader_type != VKD3D_SHADER_TYPE_HULL)
        spirv_compiler_emit_shader_signature_outputs(compiler);

    for (i = 0; i < instructions.count && result >= 0; ++i)
    {
        result = spirv_compiler_handle_instruction(compiler, &instructions.elements[i]);
    }

    shader_instruction_array_destroy(&instructions);

    if (result < 0)
        return result;

    if (!is_in_default_phase(compiler))
        spirv_compiler_leave_shader_phase(compiler);
    else
        vkd3d_spirv_build_op_function_end(builder);

    if (compiler->shader_type == VKD3D_SHADER_TYPE_HULL)
        spirv_compiler_emit_hull_shader_main(compiler);

    if (compiler->shader_type == VKD3D_SHADER_TYPE_DOMAIN)
    {
        if (info && (ds_info = vkd3d_find_struct(compile_info->next, SPIRV_DOMAIN_SHADER_TARGET_INFO)))
        {
            spirv_compiler_emit_tessellator_output_primitive(compiler, ds_info->output_primitive);
            spirv_compiler_emit_tessellator_partitioning(compiler, ds_info->partitioning);
        }
        else if (spirv_compiler_is_opengl_target(compiler))
        {
            ERR("vkd3d_shader_spirv_domain_shader_target_info is required for "
                    "OpenGL tessellation evaluation shader.\n");
        }
    }

    if (compiler->discard_function_id)
        spirv_compiler_emit_discard_function(compiler);

    if (compiler->epilogue_function_id)
    {
        vkd3d_spirv_build_op_name(builder, compiler->epilogue_function_id, "epilogue");
        spirv_compiler_emit_shader_epilogue_function(compiler);
    }

    if (compiler->strip_debug)
        vkd3d_spirv_stream_clear(&builder->debug_stream);

    environment = spirv_compiler_get_target_environment(compiler);
    if (!vkd3d_spirv_compile_module(builder, spirv, spirv_compiler_get_entry_point_name(compiler), environment))
        return VKD3D_ERROR;

    if (TRACE_ON() || compiler->config_flags & VKD3D_SHADER_CONFIG_FLAG_FORCE_VALIDATION)
    {
        struct vkd3d_string_buffer buffer;

        if (TRACE_ON())
            vkd3d_spirv_dump(spirv, environment);

        vkd3d_string_buffer_init(&buffer);
        if (!vkd3d_spirv_validate(&buffer, spirv, environment))
        {
            FIXME("Failed to validate SPIR-V binary.\n");
            vkd3d_shader_trace_text(buffer.buffer, buffer.content_size);

            if (compiler->config_flags & VKD3D_SHADER_CONFIG_FLAG_FORCE_VALIDATION)
            {
                spirv_compiler_error(compiler, VKD3D_SHADER_ERROR_SPV_INVALID_SHADER,
                        "Execution generated an invalid shader, failing compilation:\n%s",
                        buffer.buffer);
            }
        }
        vkd3d_string_buffer_cleanup(&buffer);
    }

    if (compiler->failed)
        return VKD3D_ERROR_INVALID_SHADER;

    if (compile_info->target_type == VKD3D_SHADER_TARGET_SPIRV_TEXT)
    {
        struct vkd3d_shader_code text;
        if (vkd3d_spirv_binary_to_text(spirv, environment, compiler->formatting, &text) != VKD3D_OK)
            return VKD3D_ERROR;
        vkd3d_shader_free_shader_code(spirv);
        *spirv = text;
    }

    return VKD3D_OK;
}

int spirv_compile(struct vsir_program *program, uint64_t config_flags,
        const struct vkd3d_shader_scan_descriptor_info1 *scan_descriptor_info,
        const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_code *out, struct vkd3d_shader_message_context *message_context)
{
    struct spirv_compiler *spirv_compiler;
    int ret;

    if (!(spirv_compiler = spirv_compiler_create(program, compile_info,
            scan_descriptor_info, message_context, config_flags)))
    {
        ERR("Failed to create SPIR-V compiler.\n");
        return VKD3D_ERROR;
    }

    ret = spirv_compiler_generate_spirv(spirv_compiler, program, compile_info, out);

    spirv_compiler_destroy(spirv_compiler);
    return ret;
}

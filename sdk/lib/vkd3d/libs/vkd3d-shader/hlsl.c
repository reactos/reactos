/*
 * HLSL utility functions
 *
 * Copyright 2012 Matteo Bruni for CodeWeavers
 * Copyright 2019-2020 Zebediah Figura for CodeWeavers
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

#include "hlsl.h"
#include <stdio.h>

void hlsl_note(struct hlsl_ctx *ctx, const struct vkd3d_shader_location *loc,
        enum vkd3d_shader_log_level level, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vkd3d_shader_vnote(ctx->message_context, loc, level, fmt, args);
    va_end(args);
}

void hlsl_error(struct hlsl_ctx *ctx, const struct vkd3d_shader_location *loc,
        enum vkd3d_shader_error error, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vkd3d_shader_verror(ctx->message_context, loc, error, fmt, args);
    va_end(args);

    if (!ctx->result)
        ctx->result = VKD3D_ERROR_INVALID_SHADER;
}

void hlsl_warning(struct hlsl_ctx *ctx, const struct vkd3d_shader_location *loc,
        enum vkd3d_shader_error error, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vkd3d_shader_vwarning(ctx->message_context, loc, error, fmt, args);
    va_end(args);
}

void hlsl_fixme(struct hlsl_ctx *ctx, const struct vkd3d_shader_location *loc, const char *fmt, ...)
{
    struct vkd3d_string_buffer *string;
    va_list args;

    va_start(args, fmt);
    string = hlsl_get_string_buffer(ctx);
    vkd3d_string_buffer_printf(string, "Aborting due to not yet implemented feature: ");
    vkd3d_string_buffer_vprintf(string, fmt, args);
    vkd3d_shader_error(ctx->message_context, loc, VKD3D_SHADER_ERROR_HLSL_NOT_IMPLEMENTED, "%s", string->buffer);
    hlsl_release_string_buffer(ctx, string);
    va_end(args);

    if (!ctx->result)
        ctx->result = VKD3D_ERROR_NOT_IMPLEMENTED;
}

char *hlsl_sprintf_alloc(struct hlsl_ctx *ctx, const char *fmt, ...)
{
    struct vkd3d_string_buffer *string;
    va_list args;
    char *ret;

    if (!(string = hlsl_get_string_buffer(ctx)))
        return NULL;
    va_start(args, fmt);
    if (vkd3d_string_buffer_vprintf(string, fmt, args) < 0)
    {
        va_end(args);
        hlsl_release_string_buffer(ctx, string);
        return NULL;
    }
    va_end(args);
    ret = hlsl_strdup(ctx, string->buffer);
    hlsl_release_string_buffer(ctx, string);
    return ret;
}

bool hlsl_add_var(struct hlsl_ctx *ctx, struct hlsl_ir_var *decl, bool local_var)
{
    struct hlsl_scope *scope = ctx->cur_scope;
    struct hlsl_ir_var *var;

    if (decl->name)
    {
        LIST_FOR_EACH_ENTRY(var, &scope->vars, struct hlsl_ir_var, scope_entry)
        {
            if (var->name && !strcmp(decl->name, var->name))
                return false;
        }
        if (local_var && scope->upper->upper == ctx->globals)
        {
            /* Check whether the variable redefines a function parameter. */
            LIST_FOR_EACH_ENTRY(var, &scope->upper->vars, struct hlsl_ir_var, scope_entry)
            {
                if (var->name && !strcmp(decl->name, var->name))
                    return false;
            }
        }
    }

    list_add_tail(&scope->vars, &decl->scope_entry);
    return true;
}

struct hlsl_ir_var *hlsl_get_var(struct hlsl_scope *scope, const char *name)
{
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(var, &scope->vars, struct hlsl_ir_var, scope_entry)
    {
        if (var->name && !strcmp(name, var->name))
            return var;
    }
    if (!scope->upper)
        return NULL;
    return hlsl_get_var(scope->upper, name);
}

void hlsl_free_state_block_entry(struct hlsl_state_block_entry *entry)
{
    unsigned int i;

    vkd3d_free(entry->name);
    for (i = 0; i < entry->args_count; ++i)
        hlsl_src_remove(&entry->args[i]);
    vkd3d_free(entry->args);
    hlsl_block_cleanup(entry->instrs);
    vkd3d_free(entry->instrs);
    vkd3d_free(entry);
}

void hlsl_free_state_block(struct hlsl_state_block *state_block)
{
    unsigned int k;

    VKD3D_ASSERT(state_block);
    for (k = 0; k < state_block->count; ++k)
        hlsl_free_state_block_entry(state_block->entries[k]);
    vkd3d_free(state_block->entries);
    vkd3d_free(state_block);
}

void hlsl_free_var(struct hlsl_ir_var *decl)
{
    unsigned int k, i;

    vkd3d_free((void *)decl->name);
    hlsl_cleanup_semantic(&decl->semantic);
    for (k = 0; k <= HLSL_REGSET_LAST_OBJECT; ++k)
        vkd3d_free((void *)decl->objects_usage[k]);

    if (decl->default_values)
    {
        unsigned int component_count = hlsl_type_component_count(decl->data_type);

        for (k = 0; k < component_count; ++k)
            vkd3d_free((void *)decl->default_values[k].string);
        vkd3d_free(decl->default_values);
    }

    for (i = 0; i < decl->state_block_count; ++i)
        hlsl_free_state_block(decl->state_blocks[i]);
    vkd3d_free(decl->state_blocks);

    vkd3d_free(decl);
}

bool hlsl_type_is_row_major(const struct hlsl_type *type)
{
    /* Default to column-major if the majority isn't explicitly set, which can
     * happen for anonymous nodes. */
    return !!(type->modifiers & HLSL_MODIFIER_ROW_MAJOR);
}

unsigned int hlsl_type_minor_size(const struct hlsl_type *type)
{
    if (type->class != HLSL_CLASS_MATRIX || hlsl_type_is_row_major(type))
        return type->dimx;
    else
        return type->dimy;
}

unsigned int hlsl_type_major_size(const struct hlsl_type *type)
{
    if (type->class != HLSL_CLASS_MATRIX || hlsl_type_is_row_major(type))
        return type->dimy;
    else
        return type->dimx;
}

unsigned int hlsl_type_element_count(const struct hlsl_type *type)
{
    switch (type->class)
    {
        case HLSL_CLASS_VECTOR:
            return type->dimx;
        case HLSL_CLASS_MATRIX:
            return hlsl_type_major_size(type);
        case HLSL_CLASS_ARRAY:
            return type->e.array.elements_count;
        case HLSL_CLASS_STRUCT:
            return type->e.record.field_count;
        default:
            return 0;
    }
}

const struct hlsl_type *hlsl_get_multiarray_element_type(const struct hlsl_type *type)
{
    if (type->class == HLSL_CLASS_ARRAY)
        return hlsl_get_multiarray_element_type(type->e.array.type);
    return type;
}

unsigned int hlsl_get_multiarray_size(const struct hlsl_type *type)
{
    if (type->class == HLSL_CLASS_ARRAY)
        return hlsl_get_multiarray_size(type->e.array.type) * type->e.array.elements_count;
    return 1;
}

bool hlsl_type_is_resource(const struct hlsl_type *type)
{
    switch (type->class)
    {
        case HLSL_CLASS_ARRAY:
            return hlsl_type_is_resource(type->e.array.type);

        case HLSL_CLASS_SAMPLER:
        case HLSL_CLASS_TEXTURE:
        case HLSL_CLASS_UAV:
            return true;

        default:
            return false;
    }
}

bool hlsl_type_is_shader(const struct hlsl_type *type)
{
    switch (type->class)
    {
        case HLSL_CLASS_ARRAY:
            return hlsl_type_is_shader(type->e.array.type);

        case HLSL_CLASS_COMPUTE_SHADER:
        case HLSL_CLASS_DOMAIN_SHADER:
        case HLSL_CLASS_GEOMETRY_SHADER:
        case HLSL_CLASS_HULL_SHADER:
        case HLSL_CLASS_PIXEL_SHADER:
        case HLSL_CLASS_VERTEX_SHADER:
            return true;

        case HLSL_CLASS_SCALAR:
        case HLSL_CLASS_VECTOR:
        case HLSL_CLASS_MATRIX:
        case HLSL_CLASS_STRUCT:
        case HLSL_CLASS_DEPTH_STENCIL_STATE:
        case HLSL_CLASS_DEPTH_STENCIL_VIEW:
        case HLSL_CLASS_EFFECT_GROUP:
        case HLSL_CLASS_ERROR:
        case HLSL_CLASS_PASS:
        case HLSL_CLASS_RASTERIZER_STATE:
        case HLSL_CLASS_RENDER_TARGET_VIEW:
        case HLSL_CLASS_SAMPLER:
        case HLSL_CLASS_STRING:
        case HLSL_CLASS_TECHNIQUE:
        case HLSL_CLASS_TEXTURE:
        case HLSL_CLASS_UAV:
        case HLSL_CLASS_CONSTANT_BUFFER:
        case HLSL_CLASS_BLEND_STATE:
        case HLSL_CLASS_VOID:
        case HLSL_CLASS_NULL:
            return false;
    }
    return false;
}

/* Only intended to be used for derefs (after copies have been lowered to components or vectors) or
 * resources, since for both their data types span across a single regset. */
static enum hlsl_regset type_get_regset(const struct hlsl_type *type)
{
    switch (type->class)
    {
        case HLSL_CLASS_SCALAR:
        case HLSL_CLASS_VECTOR:
        case HLSL_CLASS_MATRIX:
            return HLSL_REGSET_NUMERIC;

        case HLSL_CLASS_ARRAY:
            return type_get_regset(type->e.array.type);

        case HLSL_CLASS_SAMPLER:
            return HLSL_REGSET_SAMPLERS;

        case HLSL_CLASS_TEXTURE:
            return HLSL_REGSET_TEXTURES;

        case HLSL_CLASS_UAV:
            return HLSL_REGSET_UAVS;

        default:
            break;
    }

    vkd3d_unreachable();
}

enum hlsl_regset hlsl_deref_get_regset(struct hlsl_ctx *ctx, const struct hlsl_deref *deref)
{
    return type_get_regset(hlsl_deref_get_type(ctx, deref));
}

unsigned int hlsl_type_get_sm4_offset(const struct hlsl_type *type, unsigned int offset)
{
    /* Align to the next vec4 boundary if:
     *  (a) the type is a struct or array type, or
     *  (b) the type would cross a vec4 boundary; i.e. a vec3 and a
     *      vec1 can be packed together, but not a vec3 and a vec2.
     */
    if (type->class == HLSL_CLASS_STRUCT || type->class == HLSL_CLASS_ARRAY
            || (offset & 3) + type->reg_size[HLSL_REGSET_NUMERIC] > 4)
        return align(offset, 4);
    return offset;
}

static void hlsl_type_calculate_reg_size(struct hlsl_ctx *ctx, struct hlsl_type *type)
{
    bool is_sm4 = (ctx->profile->major_version >= 4);
    unsigned int k;

    for (k = 0; k <= HLSL_REGSET_LAST; ++k)
        type->reg_size[k] = 0;

    switch (type->class)
    {
        case HLSL_CLASS_SCALAR:
        case HLSL_CLASS_VECTOR:
            type->reg_size[HLSL_REGSET_NUMERIC] = is_sm4 ? type->dimx : 4;
            break;

        case HLSL_CLASS_MATRIX:
            if (hlsl_type_is_row_major(type))
                type->reg_size[HLSL_REGSET_NUMERIC] = is_sm4 ? (4 * (type->dimy - 1) + type->dimx) : (4 * type->dimy);
            else
                type->reg_size[HLSL_REGSET_NUMERIC] = is_sm4 ? (4 * (type->dimx - 1) + type->dimy) : (4 * type->dimx);
            break;

        case HLSL_CLASS_ARRAY:
        {
            if (type->e.array.elements_count == HLSL_ARRAY_ELEMENTS_COUNT_IMPLICIT)
                break;

            for (k = 0; k <= HLSL_REGSET_LAST; ++k)
            {
                unsigned int element_size = type->e.array.type->reg_size[k];

                if (is_sm4 && k == HLSL_REGSET_NUMERIC)
                    type->reg_size[k] = (type->e.array.elements_count - 1) * align(element_size, 4) + element_size;
                else
                    type->reg_size[k] = type->e.array.elements_count * element_size;
            }

            break;
        }

        case HLSL_CLASS_STRUCT:
        {
            unsigned int i;

            type->dimx = 0;
            for (i = 0; i < type->e.record.field_count; ++i)
            {
                struct hlsl_struct_field *field = &type->e.record.fields[i];

                for (k = 0; k <= HLSL_REGSET_LAST; ++k)
                {
                    if (k == HLSL_REGSET_NUMERIC)
                        type->reg_size[k] = hlsl_type_get_sm4_offset(field->type, type->reg_size[k]);
                    field->reg_offset[k] = type->reg_size[k];
                    type->reg_size[k] += field->type->reg_size[k];
                }

                type->dimx += field->type->dimx * field->type->dimy * hlsl_get_multiarray_size(field->type);
            }
            break;
        }

        case HLSL_CLASS_SAMPLER:
            type->reg_size[HLSL_REGSET_SAMPLERS] = 1;
            break;

        case HLSL_CLASS_TEXTURE:
            type->reg_size[HLSL_REGSET_TEXTURES] = 1;
            break;

        case HLSL_CLASS_UAV:
            type->reg_size[HLSL_REGSET_UAVS] = 1;
            break;

        case HLSL_CLASS_DEPTH_STENCIL_STATE:
        case HLSL_CLASS_DEPTH_STENCIL_VIEW:
        case HLSL_CLASS_EFFECT_GROUP:
        case HLSL_CLASS_ERROR:
        case HLSL_CLASS_PASS:
        case HLSL_CLASS_PIXEL_SHADER:
        case HLSL_CLASS_RASTERIZER_STATE:
        case HLSL_CLASS_RENDER_TARGET_VIEW:
        case HLSL_CLASS_STRING:
        case HLSL_CLASS_TECHNIQUE:
        case HLSL_CLASS_VERTEX_SHADER:
        case HLSL_CLASS_VOID:
        case HLSL_CLASS_CONSTANT_BUFFER:
        case HLSL_CLASS_COMPUTE_SHADER:
        case HLSL_CLASS_DOMAIN_SHADER:
        case HLSL_CLASS_HULL_SHADER:
        case HLSL_CLASS_GEOMETRY_SHADER:
        case HLSL_CLASS_BLEND_STATE:
        case HLSL_CLASS_NULL:
            break;
    }
}

/* Returns the size of a type, considered as part of an array of that type, within a specific
 * register set. As such it includes padding after the type, when applicable. */
unsigned int hlsl_type_get_array_element_reg_size(const struct hlsl_type *type, enum hlsl_regset regset)
{
    if (regset == HLSL_REGSET_NUMERIC)
        return align(type->reg_size[regset], 4);
    return type->reg_size[regset];
}

static struct hlsl_type *hlsl_new_simple_type(struct hlsl_ctx *ctx, const char *name, enum hlsl_type_class class)
{
    struct hlsl_type *type;

    if (!(type = hlsl_alloc(ctx, sizeof(*type))))
        return NULL;
    if (!(type->name = hlsl_strdup(ctx, name)))
    {
        vkd3d_free(type);
        return NULL;
    }
    type->class = class;
    hlsl_type_calculate_reg_size(ctx, type);

    list_add_tail(&ctx->types, &type->entry);

    return type;
}

static struct hlsl_type *hlsl_new_type(struct hlsl_ctx *ctx, const char *name, enum hlsl_type_class type_class,
        enum hlsl_base_type base_type, unsigned dimx, unsigned dimy)
{
    struct hlsl_type *type;

    if (!(type = hlsl_alloc(ctx, sizeof(*type))))
        return NULL;
    if (!(type->name = hlsl_strdup(ctx, name)))
    {
        vkd3d_free(type);
        return NULL;
    }
    type->class = type_class;
    type->e.numeric.type = base_type;
    type->dimx = dimx;
    type->dimy = dimy;
    hlsl_type_calculate_reg_size(ctx, type);

    list_add_tail(&ctx->types, &type->entry);

    return type;
}

static bool type_is_single_component(const struct hlsl_type *type)
{
    switch (type->class)
    {
        case HLSL_CLASS_DEPTH_STENCIL_STATE:
        case HLSL_CLASS_DEPTH_STENCIL_VIEW:
        case HLSL_CLASS_ERROR:
        case HLSL_CLASS_PIXEL_SHADER:
        case HLSL_CLASS_SCALAR:
        case HLSL_CLASS_SAMPLER:
        case HLSL_CLASS_STRING:
        case HLSL_CLASS_RASTERIZER_STATE:
        case HLSL_CLASS_RENDER_TARGET_VIEW:
        case HLSL_CLASS_TEXTURE:
        case HLSL_CLASS_UAV:
        case HLSL_CLASS_VERTEX_SHADER:
        case HLSL_CLASS_COMPUTE_SHADER:
        case HLSL_CLASS_DOMAIN_SHADER:
        case HLSL_CLASS_HULL_SHADER:
        case HLSL_CLASS_GEOMETRY_SHADER:
        case HLSL_CLASS_BLEND_STATE:
        case HLSL_CLASS_NULL:
            return true;

        case HLSL_CLASS_VECTOR:
        case HLSL_CLASS_MATRIX:
        case HLSL_CLASS_STRUCT:
        case HLSL_CLASS_ARRAY:
        case HLSL_CLASS_CONSTANT_BUFFER:
            return false;

        case HLSL_CLASS_EFFECT_GROUP:
        case HLSL_CLASS_PASS:
        case HLSL_CLASS_TECHNIQUE:
        case HLSL_CLASS_VOID:
            break;
    }
    vkd3d_unreachable();
}

/* Given a type and a component index, this function moves one step through the path required to
 * reach that component within the type.
 * It returns the first index of this path.
 * It sets *type_ptr to the (outermost) type within the original type that contains the component.
 * It sets *index_ptr to the index of the component within *type_ptr.
 * So, this function can be called several times in sequence to obtain all the path's indexes until
 * the component is finally reached. */
static unsigned int traverse_path_from_component_index(struct hlsl_ctx *ctx,
        struct hlsl_type **type_ptr, unsigned int *index_ptr)
{
    struct hlsl_type *type = *type_ptr;
    unsigned int index = *index_ptr;

    VKD3D_ASSERT(!type_is_single_component(type));
    VKD3D_ASSERT(index < hlsl_type_component_count(type));

    switch (type->class)
    {
        case HLSL_CLASS_VECTOR:
            VKD3D_ASSERT(index < type->dimx);
            *type_ptr = hlsl_get_scalar_type(ctx, type->e.numeric.type);
            *index_ptr = 0;
            return index;

        case HLSL_CLASS_MATRIX:
        {
            unsigned int y = index / type->dimx, x = index % type->dimx;
            bool row_major = hlsl_type_is_row_major(type);

            VKD3D_ASSERT(index < type->dimx * type->dimy);
            *type_ptr = hlsl_get_vector_type(ctx, type->e.numeric.type, row_major ? type->dimx : type->dimy);
            *index_ptr = row_major ? x : y;
            return row_major ? y : x;
        }

        case HLSL_CLASS_ARRAY:
        {
            unsigned int elem_comp_count = hlsl_type_component_count(type->e.array.type);
            unsigned int array_index;

            *type_ptr = type->e.array.type;
            *index_ptr = index % elem_comp_count;
            array_index = index / elem_comp_count;
            VKD3D_ASSERT(array_index < type->e.array.elements_count);
            return array_index;
        }

        case HLSL_CLASS_STRUCT:
        {
            struct hlsl_struct_field *field;
            unsigned int field_comp_count, i;

            for (i = 0; i < type->e.record.field_count; ++i)
            {
                field = &type->e.record.fields[i];
                field_comp_count = hlsl_type_component_count(field->type);
                if (index < field_comp_count)
                {
                    *type_ptr = field->type;
                    *index_ptr = index;
                    return i;
                }
                index -= field_comp_count;
            }
            vkd3d_unreachable();
        }

        case HLSL_CLASS_CONSTANT_BUFFER:
        {
            *type_ptr = type->e.resource.format;
            return traverse_path_from_component_index(ctx, type_ptr, index_ptr);
        }

        default:
            vkd3d_unreachable();
    }
}

struct hlsl_type *hlsl_type_get_component_type(struct hlsl_ctx *ctx, struct hlsl_type *type,
        unsigned int index)
{
    while (!type_is_single_component(type))
        traverse_path_from_component_index(ctx, &type, &index);

    return type;
}

unsigned int hlsl_type_get_component_offset(struct hlsl_ctx *ctx, struct hlsl_type *type,
        unsigned int index, enum hlsl_regset *regset)
{
    unsigned int offset[HLSL_REGSET_LAST + 1] = {0};
    struct hlsl_type *next_type;
    unsigned int idx, r;

    while (!type_is_single_component(type))
    {
        next_type = type;
        idx = traverse_path_from_component_index(ctx, &next_type, &index);

        switch (type->class)
        {
            case HLSL_CLASS_VECTOR:
                offset[HLSL_REGSET_NUMERIC] += idx;
                break;

            case HLSL_CLASS_MATRIX:
                offset[HLSL_REGSET_NUMERIC] += 4 * idx;
                break;

            case HLSL_CLASS_STRUCT:
                for (r = 0; r <= HLSL_REGSET_LAST; ++r)
                    offset[r] += type->e.record.fields[idx].reg_offset[r];
                break;

            case HLSL_CLASS_ARRAY:
                for (r = 0; r <= HLSL_REGSET_LAST; ++r)
                {
                    if (r == HLSL_REGSET_NUMERIC)
                        offset[r] += idx * align(type->e.array.type->reg_size[r], 4);
                    else
                        offset[r] += idx * type->e.array.type->reg_size[r];
                }
                break;

            case HLSL_CLASS_DEPTH_STENCIL_STATE:
            case HLSL_CLASS_DEPTH_STENCIL_VIEW:
            case HLSL_CLASS_PIXEL_SHADER:
            case HLSL_CLASS_RASTERIZER_STATE:
            case HLSL_CLASS_RENDER_TARGET_VIEW:
            case HLSL_CLASS_SAMPLER:
            case HLSL_CLASS_STRING:
            case HLSL_CLASS_TEXTURE:
            case HLSL_CLASS_UAV:
            case HLSL_CLASS_VERTEX_SHADER:
            case HLSL_CLASS_COMPUTE_SHADER:
            case HLSL_CLASS_DOMAIN_SHADER:
            case HLSL_CLASS_HULL_SHADER:
            case HLSL_CLASS_GEOMETRY_SHADER:
            case HLSL_CLASS_BLEND_STATE:
                VKD3D_ASSERT(idx == 0);
                break;

            case HLSL_CLASS_EFFECT_GROUP:
            case HLSL_CLASS_ERROR:
            case HLSL_CLASS_PASS:
            case HLSL_CLASS_TECHNIQUE:
            case HLSL_CLASS_VOID:
            case HLSL_CLASS_SCALAR:
            case HLSL_CLASS_CONSTANT_BUFFER:
            case HLSL_CLASS_NULL:
                vkd3d_unreachable();
        }
        type = next_type;
    }

    *regset = type_get_regset(type);
    return offset[*regset];
}

static bool init_deref(struct hlsl_ctx *ctx, struct hlsl_deref *deref, struct hlsl_ir_var *var,
        unsigned int path_len)
{
    deref->var = var;
    deref->path_len = path_len;
    deref->rel_offset.node = NULL;
    deref->const_offset = 0;
    deref->data_type = NULL;

    if (path_len == 0)
    {
        deref->path = NULL;
        return true;
    }

    if (!(deref->path = hlsl_calloc(ctx, deref->path_len, sizeof(*deref->path))))
    {
        deref->var = NULL;
        deref->path_len = 0;
        return false;
    }

    return true;
}

bool hlsl_init_deref_from_index_chain(struct hlsl_ctx *ctx, struct hlsl_deref *deref, struct hlsl_ir_node *chain)
{
    struct hlsl_ir_index *index;
    struct hlsl_ir_load *load;
    unsigned int chain_len, i;
    struct hlsl_ir_node *ptr;

    deref->path = NULL;
    deref->path_len = 0;
    deref->rel_offset.node = NULL;
    deref->const_offset = 0;

    VKD3D_ASSERT(chain);
    if (chain->type == HLSL_IR_INDEX)
        VKD3D_ASSERT(!hlsl_index_is_noncontiguous(hlsl_ir_index(chain)));

    /* Find the length of the index chain */
    chain_len = 0;
    ptr = chain;
    while (ptr->type == HLSL_IR_INDEX)
    {
        index = hlsl_ir_index(ptr);

        chain_len++;
        ptr = index->val.node;
    }

    if (ptr->type != HLSL_IR_LOAD)
    {
        hlsl_error(ctx, &chain->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_LVALUE, "Invalid l-value.");
        return false;
    }
    load = hlsl_ir_load(ptr);

    if (!init_deref(ctx, deref, load->src.var, load->src.path_len + chain_len))
        return false;

    for (i = 0; i < load->src.path_len; ++i)
        hlsl_src_from_node(&deref->path[i], load->src.path[i].node);

    chain_len = 0;
    ptr = chain;
    while (ptr->type == HLSL_IR_INDEX)
    {
        unsigned int p = deref->path_len - 1 - chain_len;

        index = hlsl_ir_index(ptr);
        if (hlsl_index_is_noncontiguous(index))
        {
            hlsl_src_from_node(&deref->path[p], deref->path[p + 1].node);
            hlsl_src_remove(&deref->path[p + 1]);
            hlsl_src_from_node(&deref->path[p + 1], index->idx.node);
        }
        else
        {
            hlsl_src_from_node(&deref->path[p], index->idx.node);
        }

        chain_len++;
        ptr = index->val.node;
    }
    VKD3D_ASSERT(deref->path_len == load->src.path_len + chain_len);

    return true;
}

struct hlsl_type *hlsl_deref_get_type(struct hlsl_ctx *ctx, const struct hlsl_deref *deref)
{
    struct hlsl_type *type;
    unsigned int i;

    VKD3D_ASSERT(deref);

    if (hlsl_deref_is_lowered(deref))
        return deref->data_type;

    type = deref->var->data_type;
    for (i = 0; i < deref->path_len; ++i)
        type = hlsl_get_element_type_from_path_index(ctx, type, deref->path[i].node);
    return type;
}

/* Initializes a deref from another deref (prefix) and a component index.
 * *block is initialized to contain the new constant node instructions used by the deref's path. */
static bool init_deref_from_component_index(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct hlsl_deref *deref, const struct hlsl_deref *prefix, unsigned int index,
        const struct vkd3d_shader_location *loc)
{
    unsigned int path_len, path_index, deref_path_len, i;
    struct hlsl_type *path_type;
    struct hlsl_ir_node *c;

    hlsl_block_init(block);

    path_len = 0;
    path_type = hlsl_deref_get_type(ctx, prefix);
    path_index = index;
    while (!type_is_single_component(path_type))
    {
        traverse_path_from_component_index(ctx, &path_type, &path_index);
        ++path_len;
    }

    if (!init_deref(ctx, deref, prefix->var, prefix->path_len + path_len))
        return false;

    deref_path_len = 0;
    for (i = 0; i < prefix->path_len; ++i)
        hlsl_src_from_node(&deref->path[deref_path_len++], prefix->path[i].node);

    path_type = hlsl_deref_get_type(ctx, prefix);
    path_index = index;
    while (!type_is_single_component(path_type))
    {
        unsigned int next_index = traverse_path_from_component_index(ctx, &path_type, &path_index);

        if (!(c = hlsl_new_uint_constant(ctx, next_index, loc)))
        {
            hlsl_block_cleanup(block);
            return false;
        }
        hlsl_block_add_instr(block, c);

        hlsl_src_from_node(&deref->path[deref_path_len++], c);
    }

    VKD3D_ASSERT(deref_path_len == deref->path_len);

    return true;
}

struct hlsl_type *hlsl_get_element_type_from_path_index(struct hlsl_ctx *ctx, const struct hlsl_type *type,
        struct hlsl_ir_node *idx)
{
    VKD3D_ASSERT(idx);

    switch (type->class)
    {
        case HLSL_CLASS_VECTOR:
            return hlsl_get_scalar_type(ctx, type->e.numeric.type);

        case HLSL_CLASS_MATRIX:
            if (hlsl_type_is_row_major(type))
                return hlsl_get_vector_type(ctx, type->e.numeric.type, type->dimx);
            else
                return hlsl_get_vector_type(ctx, type->e.numeric.type, type->dimy);

        case HLSL_CLASS_ARRAY:
            return type->e.array.type;

        case HLSL_CLASS_STRUCT:
        {
            struct hlsl_ir_constant *c = hlsl_ir_constant(idx);

            VKD3D_ASSERT(c->value.u[0].u < type->e.record.field_count);
            return type->e.record.fields[c->value.u[0].u].type;
        }

        default:
            vkd3d_unreachable();
    }
}

struct hlsl_type *hlsl_new_array_type(struct hlsl_ctx *ctx, struct hlsl_type *basic_type, unsigned int array_size)
{
    struct hlsl_type *type;

    if (!(type = hlsl_alloc(ctx, sizeof(*type))))
        return NULL;

    type->class = HLSL_CLASS_ARRAY;
    type->modifiers = basic_type->modifiers;
    type->e.array.elements_count = array_size;
    type->e.array.type = basic_type;
    type->dimx = basic_type->dimx;
    type->dimy = basic_type->dimy;
    type->sampler_dim = basic_type->sampler_dim;
    hlsl_type_calculate_reg_size(ctx, type);

    list_add_tail(&ctx->types, &type->entry);

    return type;
}

struct hlsl_type *hlsl_new_struct_type(struct hlsl_ctx *ctx, const char *name,
        struct hlsl_struct_field *fields, size_t field_count)
{
    struct hlsl_type *type;

    if (!(type = hlsl_alloc(ctx, sizeof(*type))))
        return NULL;
    type->class = HLSL_CLASS_STRUCT;
    type->name = name;
    type->dimy = 1;
    type->e.record.fields = fields;
    type->e.record.field_count = field_count;
    hlsl_type_calculate_reg_size(ctx, type);

    list_add_tail(&ctx->types, &type->entry);

    return type;
}

struct hlsl_type *hlsl_new_texture_type(struct hlsl_ctx *ctx, enum hlsl_sampler_dim dim,
        struct hlsl_type *format, unsigned int sample_count)
{
    struct hlsl_type *type;

    if (!(type = hlsl_alloc(ctx, sizeof(*type))))
        return NULL;
    type->class = HLSL_CLASS_TEXTURE;
    type->dimx = 4;
    type->dimy = 1;
    type->sampler_dim = dim;
    type->e.resource.format = format;
    type->sample_count = sample_count;
    hlsl_type_calculate_reg_size(ctx, type);
    list_add_tail(&ctx->types, &type->entry);
    return type;
}

struct hlsl_type *hlsl_new_uav_type(struct hlsl_ctx *ctx, enum hlsl_sampler_dim dim,
        struct hlsl_type *format, bool rasteriser_ordered)
{
    struct hlsl_type *type;

    if (!(type = hlsl_alloc(ctx, sizeof(*type))))
        return NULL;
    type->class = HLSL_CLASS_UAV;
    type->dimx = format->dimx;
    type->dimy = 1;
    type->sampler_dim = dim;
    type->e.resource.format = format;
    type->e.resource.rasteriser_ordered = rasteriser_ordered;
    hlsl_type_calculate_reg_size(ctx, type);
    list_add_tail(&ctx->types, &type->entry);
    return type;
}

struct hlsl_type *hlsl_new_cb_type(struct hlsl_ctx *ctx, struct hlsl_type *format)
{
    struct hlsl_type *type;

    if (!(type = hlsl_alloc(ctx, sizeof(*type))))
        return NULL;
    type->class = HLSL_CLASS_CONSTANT_BUFFER;
    type->dimy = 1;
    type->e.resource.format = format;
    hlsl_type_calculate_reg_size(ctx, type);
    list_add_tail(&ctx->types, &type->entry);
    return type;
}

static const char * get_case_insensitive_typename(const char *name)
{
    static const char *const names[] =
    {
        "dword",
        "float",
        "geometryshader",
        "matrix",
        "pixelshader",
        "texture",
        "vector",
        "vertexshader",
        "string",
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(names); ++i)
    {
        if (!ascii_strcasecmp(names[i], name))
            return names[i];
    }

    return NULL;
}

struct hlsl_type *hlsl_get_type(struct hlsl_scope *scope, const char *name, bool recursive, bool case_insensitive)
{
    struct rb_entry *entry = rb_get(&scope->types, name);

    if (entry)
        return RB_ENTRY_VALUE(entry, struct hlsl_type, scope_entry);

    if (scope->upper)
    {
        if (recursive)
            return hlsl_get_type(scope->upper, name, recursive, case_insensitive);
    }
    else
    {
        if (case_insensitive && (name = get_case_insensitive_typename(name)))
        {
            if ((entry = rb_get(&scope->types, name)))
                return RB_ENTRY_VALUE(entry, struct hlsl_type, scope_entry);
        }
    }

    return NULL;
}

struct hlsl_ir_function *hlsl_get_function(struct hlsl_ctx *ctx, const char *name)
{
    struct rb_entry *entry;

    if ((entry = rb_get(&ctx->functions, name)))
        return RB_ENTRY_VALUE(entry, struct hlsl_ir_function, entry);
    return NULL;
}

struct hlsl_ir_function_decl *hlsl_get_first_func_decl(struct hlsl_ctx *ctx, const char *name)
{
    struct hlsl_ir_function *func;
    struct rb_entry *entry;

    if ((entry = rb_get(&ctx->functions, name)))
    {
        func = RB_ENTRY_VALUE(entry, struct hlsl_ir_function, entry);
        return LIST_ENTRY(list_head(&func->overloads), struct hlsl_ir_function_decl, entry);
    }

    return NULL;
}

unsigned int hlsl_type_component_count(const struct hlsl_type *type)
{
    switch (type->class)
    {
        case HLSL_CLASS_SCALAR:
        case HLSL_CLASS_VECTOR:
        case HLSL_CLASS_MATRIX:
            return type->dimx * type->dimy;

        case HLSL_CLASS_STRUCT:
        {
            unsigned int count = 0, i;

            for (i = 0; i < type->e.record.field_count; ++i)
                count += hlsl_type_component_count(type->e.record.fields[i].type);
            return count;
        }

        case HLSL_CLASS_ARRAY:
            return hlsl_type_component_count(type->e.array.type) * type->e.array.elements_count;

        case HLSL_CLASS_CONSTANT_BUFFER:
            return hlsl_type_component_count(type->e.resource.format);

        case HLSL_CLASS_DEPTH_STENCIL_STATE:
        case HLSL_CLASS_DEPTH_STENCIL_VIEW:
        case HLSL_CLASS_ERROR:
        case HLSL_CLASS_PIXEL_SHADER:
        case HLSL_CLASS_RASTERIZER_STATE:
        case HLSL_CLASS_RENDER_TARGET_VIEW:
        case HLSL_CLASS_SAMPLER:
        case HLSL_CLASS_STRING:
        case HLSL_CLASS_TEXTURE:
        case HLSL_CLASS_UAV:
        case HLSL_CLASS_VERTEX_SHADER:
        case HLSL_CLASS_COMPUTE_SHADER:
        case HLSL_CLASS_DOMAIN_SHADER:
        case HLSL_CLASS_HULL_SHADER:
        case HLSL_CLASS_GEOMETRY_SHADER:
        case HLSL_CLASS_BLEND_STATE:
        case HLSL_CLASS_NULL:
            return 1;

        case HLSL_CLASS_EFFECT_GROUP:
        case HLSL_CLASS_PASS:
        case HLSL_CLASS_TECHNIQUE:
        case HLSL_CLASS_VOID:
            break;
    }

    vkd3d_unreachable();
}

bool hlsl_types_are_equal(const struct hlsl_type *t1, const struct hlsl_type *t2)
{
    if (t1 == t2)
        return true;

    if (t1->class != t2->class)
        return false;

    switch (t1->class)
    {
        case HLSL_CLASS_SCALAR:
        case HLSL_CLASS_VECTOR:
        case HLSL_CLASS_MATRIX:
            if (t1->e.numeric.type != t2->e.numeric.type)
                return false;
            if ((t1->modifiers & HLSL_MODIFIER_ROW_MAJOR)
                    != (t2->modifiers & HLSL_MODIFIER_ROW_MAJOR))
                return false;
            if (t1->dimx != t2->dimx)
                return false;
            if (t1->dimy != t2->dimy)
                return false;
            return true;

        case HLSL_CLASS_UAV:
            if (t1->e.resource.rasteriser_ordered != t2->e.resource.rasteriser_ordered)
                return false;
            /* fall through */
        case HLSL_CLASS_TEXTURE:
            if (t1->sampler_dim != HLSL_SAMPLER_DIM_GENERIC
                    && !hlsl_types_are_equal(t1->e.resource.format, t2->e.resource.format))
                return false;
            /* fall through */
        case HLSL_CLASS_SAMPLER:
            if (t1->sampler_dim != t2->sampler_dim)
                return false;
            return true;

        case HLSL_CLASS_STRUCT:
            if (t1->e.record.field_count != t2->e.record.field_count)
                return false;

            for (size_t i = 0; i < t1->e.record.field_count; ++i)
            {
                const struct hlsl_struct_field *field1 = &t1->e.record.fields[i];
                const struct hlsl_struct_field *field2 = &t2->e.record.fields[i];

                if (!hlsl_types_are_equal(field1->type, field2->type))
                    return false;

                if (strcmp(field1->name, field2->name))
                    return false;
            }
            return true;

        case HLSL_CLASS_ARRAY:
            return t1->e.array.elements_count == t2->e.array.elements_count
                    && hlsl_types_are_equal(t1->e.array.type, t2->e.array.type);

        case HLSL_CLASS_TECHNIQUE:
            return t1->e.version == t2->e.version;

        case HLSL_CLASS_CONSTANT_BUFFER:
            return hlsl_types_are_equal(t1->e.resource.format, t2->e.resource.format);

        case HLSL_CLASS_DEPTH_STENCIL_STATE:
        case HLSL_CLASS_DEPTH_STENCIL_VIEW:
        case HLSL_CLASS_EFFECT_GROUP:
        case HLSL_CLASS_ERROR:
        case HLSL_CLASS_PASS:
        case HLSL_CLASS_PIXEL_SHADER:
        case HLSL_CLASS_RASTERIZER_STATE:
        case HLSL_CLASS_RENDER_TARGET_VIEW:
        case HLSL_CLASS_STRING:
        case HLSL_CLASS_VERTEX_SHADER:
        case HLSL_CLASS_VOID:
        case HLSL_CLASS_COMPUTE_SHADER:
        case HLSL_CLASS_DOMAIN_SHADER:
        case HLSL_CLASS_HULL_SHADER:
        case HLSL_CLASS_GEOMETRY_SHADER:
        case HLSL_CLASS_BLEND_STATE:
        case HLSL_CLASS_NULL:
            return true;
    }

    vkd3d_unreachable();
}

struct hlsl_type *hlsl_type_clone(struct hlsl_ctx *ctx, struct hlsl_type *old,
        unsigned int default_majority, uint32_t modifiers)
{
    struct hlsl_type *type;

    if (!(type = hlsl_alloc(ctx, sizeof(*type))))
        return NULL;

    if (old->name)
    {
        type->name = hlsl_strdup(ctx, old->name);
        if (!type->name)
        {
            vkd3d_free(type);
            return NULL;
        }
    }
    type->class = old->class;
    type->dimx = old->dimx;
    type->dimy = old->dimy;
    type->modifiers = old->modifiers | modifiers;
    if (!(type->modifiers & HLSL_MODIFIERS_MAJORITY_MASK))
        type->modifiers |= default_majority;
    type->sampler_dim = old->sampler_dim;
    type->is_minimum_precision = old->is_minimum_precision;
    type->sample_count = old->sample_count;

    switch (old->class)
    {
        case HLSL_CLASS_SCALAR:
        case HLSL_CLASS_VECTOR:
        case HLSL_CLASS_MATRIX:
            type->e.numeric.type = old->e.numeric.type;
            break;

        case HLSL_CLASS_ARRAY:
            if (!(type->e.array.type = hlsl_type_clone(ctx, old->e.array.type, default_majority, modifiers)))
            {
                vkd3d_free((void *)type->name);
                vkd3d_free(type);
                return NULL;
            }
            type->e.array.elements_count = old->e.array.elements_count;
            break;

        case HLSL_CLASS_STRUCT:
        {
            size_t field_count = old->e.record.field_count, i;

            type->e.record.field_count = field_count;

            if (!(type->e.record.fields = hlsl_calloc(ctx, field_count, sizeof(*type->e.record.fields))))
            {
                vkd3d_free((void *)type->name);
                vkd3d_free(type);
                return NULL;
            }

            for (i = 0; i < field_count; ++i)
            {
                const struct hlsl_struct_field *src_field = &old->e.record.fields[i];
                struct hlsl_struct_field *dst_field = &type->e.record.fields[i];

                dst_field->loc = src_field->loc;
                if (!(dst_field->type = hlsl_type_clone(ctx, src_field->type, default_majority, modifiers)))
                {
                    vkd3d_free(type->e.record.fields);
                    vkd3d_free((void *)type->name);
                    vkd3d_free(type);
                    return NULL;
                }
                dst_field->name = hlsl_strdup(ctx, src_field->name);
                if (src_field->semantic.name)
                {
                    dst_field->semantic.name = hlsl_strdup(ctx, src_field->semantic.name);
                    dst_field->semantic.index = src_field->semantic.index;
                }
            }
            break;
        }

        case HLSL_CLASS_UAV:
            type->e.resource.rasteriser_ordered = old->e.resource.rasteriser_ordered;
            /* fall through */
        case HLSL_CLASS_TEXTURE:
            type->e.resource.format = old->e.resource.format;
            break;

        case HLSL_CLASS_TECHNIQUE:
            type->e.version = old->e.version;
            break;

        default:
            break;
    }

    hlsl_type_calculate_reg_size(ctx, type);

    list_add_tail(&ctx->types, &type->entry);
    return type;
}

bool hlsl_scope_add_type(struct hlsl_scope *scope, struct hlsl_type *type)
{
    if (hlsl_get_type(scope, type->name, false, false))
        return false;

    rb_put(&scope->types, type->name, &type->scope_entry);
    return true;
}

struct hlsl_ir_node *hlsl_new_cast(struct hlsl_ctx *ctx, struct hlsl_ir_node *node, struct hlsl_type *type,
        const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *cast;

    cast = hlsl_new_unary_expr(ctx, HLSL_OP1_CAST, node, loc);
    if (cast)
        cast->data_type = type;
    return cast;
}

struct hlsl_ir_node *hlsl_new_copy(struct hlsl_ctx *ctx, struct hlsl_ir_node *node)
{
    /* Use a cast to the same type as a makeshift identity expression. */
    return hlsl_new_cast(ctx, node, node->data_type, &node->loc);
}

struct hlsl_ir_var *hlsl_new_var(struct hlsl_ctx *ctx, const char *name, struct hlsl_type *type,
        const struct vkd3d_shader_location *loc, const struct hlsl_semantic *semantic, uint32_t modifiers,
        const struct hlsl_reg_reservation *reg_reservation)
{
    struct hlsl_ir_var *var;
    unsigned int k;

    if (!(var = hlsl_alloc(ctx, sizeof(*var))))
        return NULL;

    var->name = name;
    var->data_type = type;
    var->loc = *loc;
    if (semantic)
        var->semantic = *semantic;
    var->storage_modifiers = modifiers;
    if (reg_reservation)
        var->reg_reservation = *reg_reservation;

    for (k = 0; k <= HLSL_REGSET_LAST_OBJECT; ++k)
    {
        unsigned int i, obj_count = type->reg_size[k];

        if (obj_count == 0)
            continue;

        if (!(var->objects_usage[k] = hlsl_calloc(ctx, obj_count, sizeof(*var->objects_usage[0]))))
        {
            for (i = 0; i < k; ++i)
                vkd3d_free(var->objects_usage[i]);
            vkd3d_free(var);
            return NULL;
        }
    }

    return var;
}

struct hlsl_ir_var *hlsl_new_synthetic_var(struct hlsl_ctx *ctx, const char *template,
        struct hlsl_type *type, const struct vkd3d_shader_location *loc)
{
    struct vkd3d_string_buffer *string;
    struct hlsl_ir_var *var;

    if (!(string = hlsl_get_string_buffer(ctx)))
        return NULL;
    vkd3d_string_buffer_printf(string, "<%s-%u>", template, ctx->internal_name_counter++);
    var = hlsl_new_synthetic_var_named(ctx, string->buffer, type, loc, true);
    hlsl_release_string_buffer(ctx, string);
    return var;
}

struct hlsl_ir_var *hlsl_new_synthetic_var_named(struct hlsl_ctx *ctx, const char *name,
    struct hlsl_type *type, const struct vkd3d_shader_location *loc, bool dummy_scope)
{
    struct hlsl_ir_var *var;
    const char *name_copy;

    if (!(name_copy = hlsl_strdup(ctx, name)))
        return NULL;
    var = hlsl_new_var(ctx, name_copy, type, loc, NULL, 0, NULL);
    if (var)
    {
        if (dummy_scope)
            list_add_tail(&ctx->dummy_scope->vars, &var->scope_entry);
        else
            list_add_tail(&ctx->globals->vars, &var->scope_entry);
        var->is_synthetic = true;
    }
    return var;
}

static bool type_is_single_reg(const struct hlsl_type *type)
{
    return type->class == HLSL_CLASS_SCALAR || type->class == HLSL_CLASS_VECTOR;
}

bool hlsl_copy_deref(struct hlsl_ctx *ctx, struct hlsl_deref *deref, const struct hlsl_deref *other)
{
    unsigned int i;

    memset(deref, 0, sizeof(*deref));

    if (!other)
        return true;

    VKD3D_ASSERT(!hlsl_deref_is_lowered(other));

    if (!init_deref(ctx, deref, other->var, other->path_len))
        return false;

    for (i = 0; i < deref->path_len; ++i)
        hlsl_src_from_node(&deref->path[i], other->path[i].node);

    return true;
}

void hlsl_cleanup_deref(struct hlsl_deref *deref)
{
    unsigned int i;

    for (i = 0; i < deref->path_len; ++i)
        hlsl_src_remove(&deref->path[i]);
    vkd3d_free(deref->path);

    deref->path = NULL;
    deref->path_len = 0;

    hlsl_src_remove(&deref->rel_offset);
    deref->const_offset = 0;
}

/* Initializes a simple variable dereference, so that it can be passed to load/store functions. */
void hlsl_init_simple_deref_from_var(struct hlsl_deref *deref, struct hlsl_ir_var *var)
{
    memset(deref, 0, sizeof(*deref));
    deref->var = var;
}

static void init_node(struct hlsl_ir_node *node, enum hlsl_ir_node_type type,
        struct hlsl_type *data_type, const struct vkd3d_shader_location *loc)
{
    memset(node, 0, sizeof(*node));
    node->type = type;
    node->data_type = data_type;
    node->loc = *loc;
    list_init(&node->uses);
}

struct hlsl_ir_node *hlsl_new_simple_store(struct hlsl_ctx *ctx, struct hlsl_ir_var *lhs, struct hlsl_ir_node *rhs)
{
    struct hlsl_deref lhs_deref;

    hlsl_init_simple_deref_from_var(&lhs_deref, lhs);
    return hlsl_new_store_index(ctx, &lhs_deref, NULL, rhs, 0, &rhs->loc);
}

struct hlsl_ir_node *hlsl_new_store_index(struct hlsl_ctx *ctx, const struct hlsl_deref *lhs,
        struct hlsl_ir_node *idx, struct hlsl_ir_node *rhs, unsigned int writemask, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_store *store;
    unsigned int i;

    VKD3D_ASSERT(lhs);
    VKD3D_ASSERT(!hlsl_deref_is_lowered(lhs));

    if (!(store = hlsl_alloc(ctx, sizeof(*store))))
        return NULL;
    init_node(&store->node, HLSL_IR_STORE, NULL, loc);

    if (!init_deref(ctx, &store->lhs, lhs->var, lhs->path_len + !!idx))
    {
        vkd3d_free(store);
        return NULL;
    }
    for (i = 0; i < lhs->path_len; ++i)
        hlsl_src_from_node(&store->lhs.path[i], lhs->path[i].node);
    if (idx)
        hlsl_src_from_node(&store->lhs.path[lhs->path_len], idx);

    hlsl_src_from_node(&store->rhs, rhs);

    if (!writemask && type_is_single_reg(rhs->data_type))
        writemask = (1 << rhs->data_type->dimx) - 1;
    store->writemask = writemask;

    return &store->node;
}

bool hlsl_new_store_component(struct hlsl_ctx *ctx, struct hlsl_block *block,
        const struct hlsl_deref *lhs, unsigned int comp, struct hlsl_ir_node *rhs)
{
    struct hlsl_block comp_path_block;
    struct hlsl_ir_store *store;

    hlsl_block_init(block);

    if (!(store = hlsl_alloc(ctx, sizeof(*store))))
        return false;
    init_node(&store->node, HLSL_IR_STORE, NULL, &rhs->loc);

    if (!init_deref_from_component_index(ctx, &comp_path_block, &store->lhs, lhs, comp, &rhs->loc))
    {
        vkd3d_free(store);
        return false;
    }
    hlsl_block_add_block(block, &comp_path_block);
    hlsl_src_from_node(&store->rhs, rhs);

    if (type_is_single_reg(rhs->data_type))
        store->writemask = (1 << rhs->data_type->dimx) - 1;

    hlsl_block_add_instr(block, &store->node);

    return true;
}

struct hlsl_ir_node *hlsl_new_call(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *decl,
        const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_call *call;

    if (!(call = hlsl_alloc(ctx, sizeof(*call))))
        return NULL;

    init_node(&call->node, HLSL_IR_CALL, NULL, loc);
    call->decl = decl;
    return &call->node;
}

struct hlsl_ir_node *hlsl_new_constant(struct hlsl_ctx *ctx, struct hlsl_type *type,
        const struct hlsl_constant_value *value, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_constant *c;

    VKD3D_ASSERT(type->class <= HLSL_CLASS_VECTOR || type->class == HLSL_CLASS_NULL);

    if (!(c = hlsl_alloc(ctx, sizeof(*c))))
        return NULL;

    init_node(&c->node, HLSL_IR_CONSTANT, type, loc);
    c->value = *value;

    return &c->node;
}

struct hlsl_ir_node *hlsl_new_bool_constant(struct hlsl_ctx *ctx, bool b, const struct vkd3d_shader_location *loc)
{
    struct hlsl_constant_value value;

    value.u[0].u = b ? ~0u : 0;
    return hlsl_new_constant(ctx, hlsl_get_scalar_type(ctx, HLSL_TYPE_BOOL), &value, loc);
}

struct hlsl_ir_node *hlsl_new_float_constant(struct hlsl_ctx *ctx, float f,
        const struct vkd3d_shader_location *loc)
{
    struct hlsl_constant_value value;

    value.u[0].f = f;
    return hlsl_new_constant(ctx, hlsl_get_scalar_type(ctx, HLSL_TYPE_FLOAT), &value, loc);
}

struct hlsl_ir_node *hlsl_new_int_constant(struct hlsl_ctx *ctx, int32_t n, const struct vkd3d_shader_location *loc)
{
    struct hlsl_constant_value value;

    value.u[0].i = n;
    return hlsl_new_constant(ctx, hlsl_get_scalar_type(ctx, HLSL_TYPE_INT), &value, loc);
}

struct hlsl_ir_node *hlsl_new_uint_constant(struct hlsl_ctx *ctx, unsigned int n,
        const struct vkd3d_shader_location *loc)
{
    struct hlsl_constant_value value;

    value.u[0].u = n;
    return hlsl_new_constant(ctx, hlsl_get_scalar_type(ctx, HLSL_TYPE_UINT), &value, loc);
}

struct hlsl_ir_node *hlsl_new_string_constant(struct hlsl_ctx *ctx, const char *str,
        const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_string_constant *s;

    if (!(s = hlsl_alloc(ctx, sizeof(*s))))
        return NULL;

    init_node(&s->node, HLSL_IR_STRING_CONSTANT, ctx->builtin_types.string, loc);

    if (!(s->string = hlsl_strdup(ctx, str)))
    {
        hlsl_free_instr(&s->node);
        return NULL;
    }
    return &s->node;
}

struct hlsl_ir_node *hlsl_new_null_constant(struct hlsl_ctx *ctx, const struct vkd3d_shader_location *loc)
{
    struct hlsl_constant_value value = { 0 };
    return hlsl_new_constant(ctx, ctx->builtin_types.null, &value, loc);
}

struct hlsl_ir_node *hlsl_new_expr(struct hlsl_ctx *ctx, enum hlsl_ir_expr_op op,
        struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS],
        struct hlsl_type *data_type, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_expr *expr;
    unsigned int i;

    if (!(expr = hlsl_alloc(ctx, sizeof(*expr))))
        return NULL;
    init_node(&expr->node, HLSL_IR_EXPR, data_type, loc);
    expr->op = op;
    for (i = 0; i < HLSL_MAX_OPERANDS; ++i)
        hlsl_src_from_node(&expr->operands[i], operands[i]);
    return &expr->node;
}

struct hlsl_ir_node *hlsl_new_unary_expr(struct hlsl_ctx *ctx, enum hlsl_ir_expr_op op,
        struct hlsl_ir_node *arg, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS] = {arg};

    return hlsl_new_expr(ctx, op, operands, arg->data_type, loc);
}

struct hlsl_ir_node *hlsl_new_binary_expr(struct hlsl_ctx *ctx, enum hlsl_ir_expr_op op,
        struct hlsl_ir_node *arg1, struct hlsl_ir_node *arg2)
{
    struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS] = {arg1, arg2};

    return hlsl_new_expr(ctx, op, operands, arg1->data_type, &arg1->loc);
}

struct hlsl_ir_node *hlsl_new_ternary_expr(struct hlsl_ctx *ctx, enum hlsl_ir_expr_op op,
        struct hlsl_ir_node *arg1, struct hlsl_ir_node *arg2, struct hlsl_ir_node *arg3)
{
    struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS] = {arg1, arg2, arg3};

    VKD3D_ASSERT(hlsl_types_are_equal(arg1->data_type, arg2->data_type));
    VKD3D_ASSERT(hlsl_types_are_equal(arg1->data_type, arg3->data_type));
    return hlsl_new_expr(ctx, op, operands, arg1->data_type, &arg1->loc);
}

static struct hlsl_ir_node *hlsl_new_error_expr(struct hlsl_ctx *ctx)
{
    static const struct vkd3d_shader_location loc = {.source_name = "<error>"};
    struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS] = {0};

    /* Use a dummy location; we should never report any messages related to
     * this expression. */
    return hlsl_new_expr(ctx, HLSL_OP0_ERROR, operands, ctx->builtin_types.error, &loc);
}

struct hlsl_ir_node *hlsl_new_if(struct hlsl_ctx *ctx, struct hlsl_ir_node *condition,
        struct hlsl_block *then_block, struct hlsl_block *else_block, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_if *iff;

    if (!(iff = hlsl_alloc(ctx, sizeof(*iff))))
        return NULL;
    init_node(&iff->node, HLSL_IR_IF, NULL, loc);
    hlsl_src_from_node(&iff->condition, condition);
    hlsl_block_init(&iff->then_block);
    hlsl_block_add_block(&iff->then_block, then_block);
    hlsl_block_init(&iff->else_block);
    if (else_block)
        hlsl_block_add_block(&iff->else_block, else_block);
    return &iff->node;
}

struct hlsl_ir_switch_case *hlsl_new_switch_case(struct hlsl_ctx *ctx, unsigned int value,
        bool is_default, struct hlsl_block *body, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_switch_case *c;

    if (!(c = hlsl_alloc(ctx, sizeof(*c))))
        return NULL;

    c->value = value;
    c->is_default = is_default;
    hlsl_block_init(&c->body);
    if (body)
        hlsl_block_add_block(&c->body, body);
    c->loc = *loc;

    return c;
}

struct hlsl_ir_node *hlsl_new_switch(struct hlsl_ctx *ctx, struct hlsl_ir_node *selector,
        struct list *cases, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_switch *s;

    if (!(s = hlsl_alloc(ctx, sizeof(*s))))
        return NULL;
    init_node(&s->node, HLSL_IR_SWITCH, NULL, loc);
    hlsl_src_from_node(&s->selector, selector);
    list_init(&s->cases);
    if (cases)
        list_move_head(&s->cases, cases);

    return &s->node;
}

struct hlsl_ir_node *hlsl_new_vsir_instruction_ref(struct hlsl_ctx *ctx, unsigned int vsir_instr_idx,
        struct hlsl_type *type, const struct hlsl_reg *reg, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_vsir_instruction_ref *vsir_instr;

    if (!(vsir_instr = hlsl_alloc(ctx, sizeof(*vsir_instr))))
        return NULL;
    init_node(&vsir_instr->node, HLSL_IR_VSIR_INSTRUCTION_REF, type, loc);
    vsir_instr->vsir_instr_idx = vsir_instr_idx;

    if (reg)
        vsir_instr->node.reg = *reg;

    return &vsir_instr->node;
}

struct hlsl_ir_load *hlsl_new_load_index(struct hlsl_ctx *ctx, const struct hlsl_deref *deref,
        struct hlsl_ir_node *idx, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_load *load;
    struct hlsl_type *type;
    unsigned int i;

    VKD3D_ASSERT(!hlsl_deref_is_lowered(deref));

    type = hlsl_deref_get_type(ctx, deref);
    if (idx)
        type = hlsl_get_element_type_from_path_index(ctx, type, idx);

    if (!(load = hlsl_alloc(ctx, sizeof(*load))))
        return NULL;
    init_node(&load->node, HLSL_IR_LOAD, type, loc);

    if (!init_deref(ctx, &load->src, deref->var, deref->path_len + !!idx))
    {
        vkd3d_free(load);
        return NULL;
    }
    for (i = 0; i < deref->path_len; ++i)
        hlsl_src_from_node(&load->src.path[i], deref->path[i].node);
    if (idx)
        hlsl_src_from_node(&load->src.path[deref->path_len], idx);

    return load;
}

struct hlsl_ir_load *hlsl_new_load_parent(struct hlsl_ctx *ctx, const struct hlsl_deref *deref,
        const struct vkd3d_shader_location *loc)
{
    /* This deref can only exists temporarily because it is not the real owner of its members. */
    struct hlsl_deref tmp_deref;

    VKD3D_ASSERT(deref->path_len >= 1);

    tmp_deref = *deref;
    tmp_deref.path_len = deref->path_len - 1;
    return hlsl_new_load_index(ctx, &tmp_deref, NULL, loc);
}

struct hlsl_ir_load *hlsl_new_var_load(struct hlsl_ctx *ctx, struct hlsl_ir_var *var,
        const struct vkd3d_shader_location *loc)
{
    struct hlsl_deref var_deref;

    hlsl_init_simple_deref_from_var(&var_deref, var);
    return hlsl_new_load_index(ctx, &var_deref, NULL, loc);
}

struct hlsl_ir_node *hlsl_new_load_component(struct hlsl_ctx *ctx, struct hlsl_block *block,
        const struct hlsl_deref *deref, unsigned int comp, const struct vkd3d_shader_location *loc)
{
    struct hlsl_type *type, *comp_type;
    struct hlsl_block comp_path_block;
    struct hlsl_ir_load *load;

    hlsl_block_init(block);

    if (!(load = hlsl_alloc(ctx, sizeof(*load))))
        return NULL;

    type = hlsl_deref_get_type(ctx, deref);
    comp_type = hlsl_type_get_component_type(ctx, type, comp);
    init_node(&load->node, HLSL_IR_LOAD, comp_type, loc);

    if (!init_deref_from_component_index(ctx, &comp_path_block, &load->src, deref, comp, loc))
    {
        vkd3d_free(load);
        return NULL;
    }
    hlsl_block_add_block(block, &comp_path_block);

    hlsl_block_add_instr(block, &load->node);

    return &load->node;
}

struct hlsl_ir_node *hlsl_new_resource_load(struct hlsl_ctx *ctx,
        const struct hlsl_resource_load_params *params, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_resource_load *load;

    if (!(load = hlsl_alloc(ctx, sizeof(*load))))
        return NULL;
    init_node(&load->node, HLSL_IR_RESOURCE_LOAD, params->format, loc);
    load->load_type = params->type;

    if (!hlsl_init_deref_from_index_chain(ctx, &load->resource, params->resource))
    {
        vkd3d_free(load);
        return NULL;
    }

    if (params->sampler)
    {
        if (!hlsl_init_deref_from_index_chain(ctx, &load->sampler, params->sampler))
        {
            hlsl_cleanup_deref(&load->resource);
            vkd3d_free(load);
            return NULL;
        }
    }

    hlsl_src_from_node(&load->coords, params->coords);
    hlsl_src_from_node(&load->sample_index, params->sample_index);
    hlsl_src_from_node(&load->texel_offset, params->texel_offset);
    hlsl_src_from_node(&load->lod, params->lod);
    hlsl_src_from_node(&load->ddx, params->ddx);
    hlsl_src_from_node(&load->ddy, params->ddy);
    hlsl_src_from_node(&load->cmp, params->cmp);
    load->sampling_dim = params->sampling_dim;
    if (load->sampling_dim == HLSL_SAMPLER_DIM_GENERIC)
        load->sampling_dim = hlsl_deref_get_type(ctx, &load->resource)->sampler_dim;
    return &load->node;
}

struct hlsl_ir_node *hlsl_new_resource_store(struct hlsl_ctx *ctx, const struct hlsl_deref *resource,
        struct hlsl_ir_node *coords, struct hlsl_ir_node *value, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_resource_store *store;

    if (!(store = hlsl_alloc(ctx, sizeof(*store))))
        return NULL;
    init_node(&store->node, HLSL_IR_RESOURCE_STORE, NULL, loc);
    hlsl_copy_deref(ctx, &store->resource, resource);
    hlsl_src_from_node(&store->coords, coords);
    hlsl_src_from_node(&store->value, value);
    return &store->node;
}

struct hlsl_ir_node *hlsl_new_swizzle(struct hlsl_ctx *ctx, uint32_t s, unsigned int components,
        struct hlsl_ir_node *val, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_swizzle *swizzle;
    struct hlsl_type *type;

    if (!(swizzle = hlsl_alloc(ctx, sizeof(*swizzle))))
        return NULL;
    VKD3D_ASSERT(hlsl_is_numeric_type(val->data_type));
    if (components == 1)
        type = hlsl_get_scalar_type(ctx, val->data_type->e.numeric.type);
    else
        type = hlsl_get_vector_type(ctx, val->data_type->e.numeric.type, components);
    init_node(&swizzle->node, HLSL_IR_SWIZZLE, type, loc);
    hlsl_src_from_node(&swizzle->val, val);
    swizzle->swizzle = s;
    return &swizzle->node;
}

struct hlsl_ir_node *hlsl_new_compile(struct hlsl_ctx *ctx, enum hlsl_compile_type compile_type,
        const char *profile_name, struct hlsl_ir_node **args, unsigned int args_count,
        struct hlsl_block *args_instrs, const struct vkd3d_shader_location *loc)
{
    const struct hlsl_profile_info *profile_info = NULL;
    struct hlsl_ir_compile *compile;
    struct hlsl_type *type = NULL;
    unsigned int i;

    switch (compile_type)
    {
        case HLSL_COMPILE_TYPE_COMPILE:
            if (!(profile_info = hlsl_get_target_info(profile_name)))
            {
                hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_PROFILE, "Unknown profile \"%s\".", profile_name);
                return NULL;
            }

            if (profile_info->type == VKD3D_SHADER_TYPE_PIXEL)
                type = hlsl_get_type(ctx->cur_scope, "PixelShader", true, true);
            else if (profile_info->type == VKD3D_SHADER_TYPE_VERTEX)
                type = hlsl_get_type(ctx->cur_scope, "VertexShader", true, true);

            if (!type)
            {
                hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_PROFILE, "Invalid profile \"%s\".", profile_name);
                return NULL;
            }

            break;

        case HLSL_COMPILE_TYPE_CONSTRUCTGSWITHSO:
            type = hlsl_get_type(ctx->cur_scope, "GeometryShader", true, true);
            break;
    }

    if (!(compile = hlsl_alloc(ctx, sizeof(*compile))))
        return NULL;

    init_node(&compile->node, HLSL_IR_COMPILE, type, loc);

    compile->compile_type = compile_type;
    compile->profile = profile_info;

    hlsl_block_init(&compile->instrs);
    hlsl_block_add_block(&compile->instrs, args_instrs);

    compile->args_count = args_count;
    if (!(compile->args = hlsl_alloc(ctx, sizeof(*compile->args) * args_count)))
    {
        vkd3d_free(compile);
        return NULL;
    }
    for (i = 0; i < compile->args_count; ++i)
        hlsl_src_from_node(&compile->args[i], args[i]);

    return &compile->node;
}

bool hlsl_state_block_add_entry(struct hlsl_state_block *state_block,
        struct hlsl_state_block_entry *entry)
{
    if (!vkd3d_array_reserve((void **)&state_block->entries,
            &state_block->capacity, state_block->count + 1,
            sizeof(*state_block->entries)))
        return false;

    state_block->entries[state_block->count++] = entry;
    return true;
}

struct hlsl_ir_node *hlsl_new_sampler_state(struct hlsl_ctx *ctx,
        const struct hlsl_state_block *state_block, struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_sampler_state *sampler_state;
    struct hlsl_type *type = ctx->builtin_types.sampler[HLSL_SAMPLER_DIM_GENERIC];

    if (!(sampler_state = hlsl_alloc(ctx, sizeof(*sampler_state))))
        return NULL;

    init_node(&sampler_state->node, HLSL_IR_SAMPLER_STATE, type, loc);

    if (!(sampler_state->state_block = hlsl_alloc(ctx, sizeof(*sampler_state->state_block))))
    {
        vkd3d_free(sampler_state);
        return NULL;
    }

    if (state_block)
    {
        for (unsigned int i = 0; i < state_block->count; ++i)
        {
            const struct hlsl_state_block_entry *src = state_block->entries[i];
            struct hlsl_state_block_entry *entry;

            if (!(entry = clone_stateblock_entry(ctx, src, src->name, src->lhs_has_index, src->lhs_index, false, 0)))
            {
                hlsl_free_instr(&sampler_state->node);
                return NULL;
            }

            if (!hlsl_state_block_add_entry(sampler_state->state_block, entry))
            {
                hlsl_free_instr(&sampler_state->node);
                return NULL;
            }
        }
    }

    return &sampler_state->node;
}

struct hlsl_ir_node *hlsl_new_stateblock_constant(struct hlsl_ctx *ctx, const char *name,
        struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_stateblock_constant *constant;
    struct hlsl_type *type = hlsl_get_scalar_type(ctx, HLSL_TYPE_INT);

    if (!(constant = hlsl_alloc(ctx, sizeof(*constant))))
        return NULL;

    init_node(&constant->node, HLSL_IR_STATEBLOCK_CONSTANT, type, loc);

    if (!(constant->name = hlsl_alloc(ctx, strlen(name) + 1)))
    {
        vkd3d_free(constant);
        return NULL;
    }
    strcpy(constant->name, name);

    return &constant->node;
}

bool hlsl_index_is_noncontiguous(struct hlsl_ir_index *index)
{
    struct hlsl_type *type = index->val.node->data_type;

    return type->class == HLSL_CLASS_MATRIX && !hlsl_type_is_row_major(type);
}

bool hlsl_index_is_resource_access(struct hlsl_ir_index *index)
{
    const struct hlsl_type *type = index->val.node->data_type;

    return type->class == HLSL_CLASS_TEXTURE || type->class == HLSL_CLASS_UAV;
}

bool hlsl_index_chain_has_resource_access(struct hlsl_ir_index *index)
{
    if (hlsl_index_is_resource_access(index))
        return true;
    if (index->val.node->type == HLSL_IR_INDEX)
        return hlsl_index_chain_has_resource_access(hlsl_ir_index(index->val.node));
    return false;
}

struct hlsl_ir_node *hlsl_new_index(struct hlsl_ctx *ctx, struct hlsl_ir_node *val,
        struct hlsl_ir_node *idx, const struct vkd3d_shader_location *loc)
{
    struct hlsl_type *type = val->data_type;
    struct hlsl_ir_index *index;

    if (!(index = hlsl_alloc(ctx, sizeof(*index))))
        return NULL;

    if (type->class == HLSL_CLASS_TEXTURE || type->class == HLSL_CLASS_UAV)
        type = type->e.resource.format;
    else if (type->class == HLSL_CLASS_MATRIX)
        type = hlsl_get_vector_type(ctx, type->e.numeric.type, type->dimx);
    else
        type = hlsl_get_element_type_from_path_index(ctx, type, idx);

    init_node(&index->node, HLSL_IR_INDEX, type, loc);
    hlsl_src_from_node(&index->val, val);
    hlsl_src_from_node(&index->idx, idx);
    return &index->node;
}

struct hlsl_ir_node *hlsl_new_jump(struct hlsl_ctx *ctx, enum hlsl_ir_jump_type type,
        struct hlsl_ir_node *condition, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_jump *jump;

    if (!(jump = hlsl_alloc(ctx, sizeof(*jump))))
        return NULL;
    init_node(&jump->node, HLSL_IR_JUMP, NULL, loc);
    jump->type = type;
    hlsl_src_from_node(&jump->condition, condition);
    return &jump->node;
}

struct hlsl_ir_node *hlsl_new_loop(struct hlsl_ctx *ctx,
        struct hlsl_block *block, enum hlsl_ir_loop_unroll_type unroll_type,
        unsigned int unroll_limit, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_loop *loop;

    if (!(loop = hlsl_alloc(ctx, sizeof(*loop))))
        return NULL;
    init_node(&loop->node, HLSL_IR_LOOP, NULL, loc);
    hlsl_block_init(&loop->body);
    hlsl_block_add_block(&loop->body, block);

    loop->unroll_type = unroll_type;
    loop->unroll_limit = unroll_limit;
    return &loop->node;
}

struct clone_instr_map
{
    struct
    {
        const struct hlsl_ir_node *src;
        struct hlsl_ir_node *dst;
    } *instrs;
    size_t count, capacity;
};

static struct hlsl_ir_node *clone_instr(struct hlsl_ctx *ctx,
        struct clone_instr_map *map, const struct hlsl_ir_node *instr);

static bool clone_block(struct hlsl_ctx *ctx, struct hlsl_block *dst_block,
        const struct hlsl_block *src_block, struct clone_instr_map *map)
{
    const struct hlsl_ir_node *src;
    struct hlsl_ir_node *dst;

    hlsl_block_init(dst_block);

    LIST_FOR_EACH_ENTRY(src, &src_block->instrs, struct hlsl_ir_node, entry)
    {
        if (!(dst = clone_instr(ctx, map, src)))
        {
            hlsl_block_cleanup(dst_block);
            return false;
        }
        hlsl_block_add_instr(dst_block, dst);

        if (!list_empty(&src->uses))
        {
            if (!hlsl_array_reserve(ctx, (void **)&map->instrs, &map->capacity, map->count + 1, sizeof(*map->instrs)))
            {
                hlsl_block_cleanup(dst_block);
                return false;
            }

            map->instrs[map->count].dst = dst;
            map->instrs[map->count].src = src;
            ++map->count;
        }
    }
    return true;
}

static struct hlsl_ir_node *map_instr(const struct clone_instr_map *map, struct hlsl_ir_node *src)
{
    size_t i;

    if (!src)
        return NULL;

    for (i = 0; i < map->count; ++i)
    {
        if (map->instrs[i].src == src)
            return map->instrs[i].dst;
    }

    return src;
}

static bool clone_deref(struct hlsl_ctx *ctx, struct clone_instr_map *map,
        struct hlsl_deref *dst, const struct hlsl_deref *src)
{
    unsigned int i;

    VKD3D_ASSERT(!hlsl_deref_is_lowered(src));

    if (!init_deref(ctx, dst, src->var, src->path_len))
        return false;

    for (i = 0; i < src->path_len; ++i)
        hlsl_src_from_node(&dst->path[i], map_instr(map, src->path[i].node));

    return true;
}

static void clone_src(struct clone_instr_map *map, struct hlsl_src *dst, const struct hlsl_src *src)
{
    hlsl_src_from_node(dst, map_instr(map, src->node));
}

static struct hlsl_ir_node *clone_call(struct hlsl_ctx *ctx, struct hlsl_ir_call *src)
{
    return hlsl_new_call(ctx, src->decl, &src->node.loc);
}

static struct hlsl_ir_node *clone_constant(struct hlsl_ctx *ctx, struct hlsl_ir_constant *src)
{
    return hlsl_new_constant(ctx, src->node.data_type, &src->value, &src->node.loc);
}

static struct hlsl_ir_node *clone_expr(struct hlsl_ctx *ctx, struct clone_instr_map *map, struct hlsl_ir_expr *src)
{
    struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS];
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(operands); ++i)
        operands[i] = map_instr(map, src->operands[i].node);

    return hlsl_new_expr(ctx, src->op, operands, src->node.data_type, &src->node.loc);
}

static struct hlsl_ir_node *clone_if(struct hlsl_ctx *ctx, struct clone_instr_map *map, struct hlsl_ir_if *src)
{
    struct hlsl_block then_block, else_block;
    struct hlsl_ir_node *dst;

    if (!clone_block(ctx, &then_block, &src->then_block, map))
        return NULL;
    if (!clone_block(ctx, &else_block, &src->else_block, map))
    {
        hlsl_block_cleanup(&then_block);
        return NULL;
    }

    if (!(dst = hlsl_new_if(ctx, map_instr(map, src->condition.node), &then_block, &else_block, &src->node.loc)))
    {
        hlsl_block_cleanup(&then_block);
        hlsl_block_cleanup(&else_block);
        return NULL;
    }

    return dst;
}

static struct hlsl_ir_node *clone_jump(struct hlsl_ctx *ctx, struct clone_instr_map *map, struct hlsl_ir_jump *src)
{
    return hlsl_new_jump(ctx, src->type, map_instr(map, src->condition.node), &src->node.loc);
}

static struct hlsl_ir_node *clone_load(struct hlsl_ctx *ctx, struct clone_instr_map *map, struct hlsl_ir_load *src)
{
    struct hlsl_ir_load *dst;

    if (!(dst = hlsl_alloc(ctx, sizeof(*dst))))
        return NULL;
    init_node(&dst->node, HLSL_IR_LOAD, src->node.data_type, &src->node.loc);

    if (!clone_deref(ctx, map, &dst->src, &src->src))
    {
        vkd3d_free(dst);
        return NULL;
    }
    return &dst->node;
}

static struct hlsl_ir_node *clone_loop(struct hlsl_ctx *ctx, struct clone_instr_map *map, struct hlsl_ir_loop *src)
{
    struct hlsl_ir_node *dst;
    struct hlsl_block body;

    if (!clone_block(ctx, &body, &src->body, map))
        return NULL;

    if (!(dst = hlsl_new_loop(ctx, &body, src->unroll_type, src->unroll_limit, &src->node.loc)))
    {
        hlsl_block_cleanup(&body);
        return NULL;
    }
    return dst;
}

static struct hlsl_ir_node *clone_resource_load(struct hlsl_ctx *ctx,
        struct clone_instr_map *map, struct hlsl_ir_resource_load *src)
{
    struct hlsl_ir_resource_load *dst;

    if (!(dst = hlsl_alloc(ctx, sizeof(*dst))))
        return NULL;
    init_node(&dst->node, HLSL_IR_RESOURCE_LOAD, src->node.data_type, &src->node.loc);
    dst->load_type = src->load_type;
    if (!clone_deref(ctx, map, &dst->resource, &src->resource))
    {
        vkd3d_free(dst);
        return NULL;
    }
    if (!clone_deref(ctx, map, &dst->sampler, &src->sampler))
    {
        hlsl_cleanup_deref(&dst->resource);
        vkd3d_free(dst);
        return NULL;
    }
    clone_src(map, &dst->coords, &src->coords);
    clone_src(map, &dst->lod, &src->lod);
    clone_src(map, &dst->ddx, &src->ddx);
    clone_src(map, &dst->ddy, &src->ddy);
    clone_src(map, &dst->sample_index, &src->sample_index);
    clone_src(map, &dst->cmp, &src->cmp);
    clone_src(map, &dst->texel_offset, &src->texel_offset);
    dst->sampling_dim = src->sampling_dim;
    return &dst->node;
}

static struct hlsl_ir_node *clone_resource_store(struct hlsl_ctx *ctx,
        struct clone_instr_map *map, struct hlsl_ir_resource_store *src)
{
    struct hlsl_ir_resource_store *dst;

    if (!(dst = hlsl_alloc(ctx, sizeof(*dst))))
        return NULL;
    init_node(&dst->node, HLSL_IR_RESOURCE_STORE, NULL, &src->node.loc);
    if (!clone_deref(ctx, map, &dst->resource, &src->resource))
    {
        vkd3d_free(dst);
        return NULL;
    }
    clone_src(map, &dst->coords, &src->coords);
    clone_src(map, &dst->value, &src->value);
    return &dst->node;
}

static struct hlsl_ir_node *clone_string_constant(struct hlsl_ctx *ctx, struct hlsl_ir_string_constant *src)
{
    return hlsl_new_string_constant(ctx, src->string, &src->node.loc);
}

static struct hlsl_ir_node *clone_store(struct hlsl_ctx *ctx, struct clone_instr_map *map, struct hlsl_ir_store *src)
{
    struct hlsl_ir_store *dst;

    if (!(dst = hlsl_alloc(ctx, sizeof(*dst))))
        return NULL;
    init_node(&dst->node, HLSL_IR_STORE, NULL, &src->node.loc);

    if (!clone_deref(ctx, map, &dst->lhs, &src->lhs))
    {
        vkd3d_free(dst);
        return NULL;
    }
    clone_src(map, &dst->rhs, &src->rhs);
    dst->writemask = src->writemask;
    return &dst->node;
}

static struct hlsl_ir_node *clone_swizzle(struct hlsl_ctx *ctx,
        struct clone_instr_map *map, struct hlsl_ir_swizzle *src)
{
    return hlsl_new_swizzle(ctx, src->swizzle, src->node.data_type->dimx,
            map_instr(map, src->val.node), &src->node.loc);
}

static struct hlsl_ir_node *clone_index(struct hlsl_ctx *ctx, struct clone_instr_map *map,
        struct hlsl_ir_index *src)
{
    struct hlsl_ir_node *dst;

    if (!(dst = hlsl_new_index(ctx, map_instr(map, src->val.node), map_instr(map, src->idx.node),
            &src->node.loc)))
        return NULL;
    return dst;
}

static struct hlsl_ir_node *clone_compile(struct hlsl_ctx *ctx,
        struct clone_instr_map *map, struct hlsl_ir_compile *compile)
{
    const char *profile_name = NULL;
    struct hlsl_ir_node **args;
    struct hlsl_ir_node *node;
    struct hlsl_block block;
    unsigned int i;

    if (!(clone_block(ctx, &block, &compile->instrs, map)))
        return NULL;

    if (!(args = hlsl_alloc(ctx, sizeof(*args) * compile->args_count)))
    {
        hlsl_block_cleanup(&block);
        return NULL;
    }
    for (i = 0; i < compile->args_count; ++i)
    {
        args[i] = map_instr(map, compile->args[i].node);
        VKD3D_ASSERT(args[i]);
    }

    if (compile->profile)
        profile_name = compile->profile->name;

    if (!(node = hlsl_new_compile(ctx, compile->compile_type, profile_name,
            args, compile->args_count, &block, &compile->node.loc)))
    {
        hlsl_block_cleanup(&block);
        vkd3d_free(args);
        return NULL;
    }

    vkd3d_free(args);
    return node;
}

static struct hlsl_ir_node *clone_sampler_state(struct hlsl_ctx *ctx,
        struct clone_instr_map *map, struct hlsl_ir_sampler_state *sampler_state)
{
    return hlsl_new_sampler_state(ctx, sampler_state->state_block,
            &sampler_state->node.loc);
}

static struct hlsl_ir_node *clone_stateblock_constant(struct hlsl_ctx *ctx,
        struct clone_instr_map *map, struct hlsl_ir_stateblock_constant *constant)
{
    return hlsl_new_stateblock_constant(ctx, constant->name, &constant->node.loc);
}

struct hlsl_state_block_entry *clone_stateblock_entry(struct hlsl_ctx *ctx,
        const struct hlsl_state_block_entry *src, const char *name, bool lhs_has_index,
        unsigned int lhs_index, bool single_arg, unsigned int arg_index)
{
    struct hlsl_state_block_entry *entry;
    struct clone_instr_map map = { 0 };

    if (!(entry = hlsl_alloc(ctx, sizeof(*entry))))
        return NULL;
    entry->name = hlsl_strdup(ctx, name);
    entry->lhs_has_index = lhs_has_index;
    entry->lhs_index = lhs_index;
    if (!(entry->instrs = hlsl_alloc(ctx, sizeof(*entry->instrs))))
    {
        hlsl_free_state_block_entry(entry);
        return NULL;
    }

    if (single_arg)
        entry->args_count = 1;
    else
        entry->args_count = src->args_count;

    if (!(entry->args = hlsl_alloc(ctx, sizeof(*entry->args) * entry->args_count)))
    {
        hlsl_free_state_block_entry(entry);
        return NULL;
    }

    hlsl_block_init(entry->instrs);
    if (!clone_block(ctx, entry->instrs, src->instrs, &map))
    {
        hlsl_free_state_block_entry(entry);
        return NULL;
    }

    if (single_arg)
    {
        clone_src(&map, entry->args, &src->args[arg_index]);
    }
    else
    {
        for (unsigned int i = 0; i < src->args_count; ++i)
            clone_src(&map, &entry->args[i], &src->args[i]);
    }
    vkd3d_free(map.instrs);

    return entry;
}

void hlsl_free_ir_switch_case(struct hlsl_ir_switch_case *c)
{
    hlsl_block_cleanup(&c->body);
    list_remove(&c->entry);
    vkd3d_free(c);
}

void hlsl_cleanup_ir_switch_cases(struct list *cases)
{
    struct hlsl_ir_switch_case *c, *next;

    LIST_FOR_EACH_ENTRY_SAFE(c, next, cases, struct hlsl_ir_switch_case, entry)
    {
        hlsl_free_ir_switch_case(c);
    }
}

static struct hlsl_ir_node *clone_switch(struct hlsl_ctx *ctx,
        struct clone_instr_map *map, struct hlsl_ir_switch *s)
{
    struct hlsl_ir_switch_case *c, *d;
    struct hlsl_ir_node *ret;
    struct hlsl_block body;
    struct list cases;

    list_init(&cases);

    LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
    {
        if (!(clone_block(ctx, &body, &c->body, map)))
        {
            hlsl_cleanup_ir_switch_cases(&cases);
            return NULL;
        }

        d = hlsl_new_switch_case(ctx, c->value, c->is_default, &body, &c->loc);
        hlsl_block_cleanup(&body);
        if (!d)
        {
            hlsl_cleanup_ir_switch_cases(&cases);
            return NULL;
        }

        list_add_tail(&cases, &d->entry);
    }

    ret = hlsl_new_switch(ctx, map_instr(map, s->selector.node), &cases, &s->node.loc);
    hlsl_cleanup_ir_switch_cases(&cases);

    return ret;
}

static struct hlsl_ir_node *clone_instr(struct hlsl_ctx *ctx,
        struct clone_instr_map *map, const struct hlsl_ir_node *instr)
{
    switch (instr->type)
    {
        case HLSL_IR_CALL:
            return clone_call(ctx, hlsl_ir_call(instr));

        case HLSL_IR_CONSTANT:
            return clone_constant(ctx, hlsl_ir_constant(instr));

        case HLSL_IR_EXPR:
            return clone_expr(ctx, map, hlsl_ir_expr(instr));

        case HLSL_IR_IF:
            return clone_if(ctx, map, hlsl_ir_if(instr));

        case HLSL_IR_INDEX:
            return clone_index(ctx, map, hlsl_ir_index(instr));

        case HLSL_IR_JUMP:
            return clone_jump(ctx, map, hlsl_ir_jump(instr));

        case HLSL_IR_LOAD:
            return clone_load(ctx, map, hlsl_ir_load(instr));

        case HLSL_IR_LOOP:
            return clone_loop(ctx, map, hlsl_ir_loop(instr));

        case HLSL_IR_RESOURCE_LOAD:
            return clone_resource_load(ctx, map, hlsl_ir_resource_load(instr));

        case HLSL_IR_RESOURCE_STORE:
            return clone_resource_store(ctx, map, hlsl_ir_resource_store(instr));

        case HLSL_IR_STRING_CONSTANT:
            return clone_string_constant(ctx, hlsl_ir_string_constant(instr));

        case HLSL_IR_STORE:
            return clone_store(ctx, map, hlsl_ir_store(instr));

        case HLSL_IR_SWITCH:
            return clone_switch(ctx, map, hlsl_ir_switch(instr));

        case HLSL_IR_SWIZZLE:
            return clone_swizzle(ctx, map, hlsl_ir_swizzle(instr));

        case HLSL_IR_COMPILE:
            return clone_compile(ctx, map, hlsl_ir_compile(instr));

        case HLSL_IR_SAMPLER_STATE:
            return clone_sampler_state(ctx, map, hlsl_ir_sampler_state(instr));

        case HLSL_IR_STATEBLOCK_CONSTANT:
            return clone_stateblock_constant(ctx, map, hlsl_ir_stateblock_constant(instr));

        case HLSL_IR_VSIR_INSTRUCTION_REF:
            vkd3d_unreachable();
    }

    vkd3d_unreachable();
}

bool hlsl_clone_block(struct hlsl_ctx *ctx, struct hlsl_block *dst_block, const struct hlsl_block *src_block)
{
    struct clone_instr_map map = {0};
    bool ret;

    ret = clone_block(ctx, dst_block, src_block, &map);
    vkd3d_free(map.instrs);
    return ret;
}

struct hlsl_ir_function_decl *hlsl_new_func_decl(struct hlsl_ctx *ctx,
        struct hlsl_type *return_type, const struct hlsl_func_parameters *parameters,
        const struct hlsl_semantic *semantic, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *constant, *store;
    struct hlsl_ir_function_decl *decl;

    if (!(decl = hlsl_alloc(ctx, sizeof(*decl))))
        return NULL;
    hlsl_block_init(&decl->body);
    decl->return_type = return_type;
    decl->parameters = *parameters;
    decl->loc = *loc;
    list_init(&decl->extern_vars);

    if (!hlsl_types_are_equal(return_type, ctx->builtin_types.Void))
    {
        if (!(decl->return_var = hlsl_new_synthetic_var(ctx, "retval", return_type, loc)))
        {
            vkd3d_free(decl);
            return NULL;
        }
        decl->return_var->semantic = *semantic;
    }

    if (!(decl->early_return_var = hlsl_new_synthetic_var(ctx, "early_return",
            hlsl_get_scalar_type(ctx, HLSL_TYPE_BOOL), loc)))
        return decl;

    if (!(constant = hlsl_new_bool_constant(ctx, false, loc)))
        return decl;
    hlsl_block_add_instr(&decl->body, constant);

    if (!(store = hlsl_new_simple_store(ctx, decl->early_return_var, constant)))
        return decl;
    hlsl_block_add_instr(&decl->body, store);

    return decl;
}

struct hlsl_buffer *hlsl_new_buffer(struct hlsl_ctx *ctx, enum hlsl_buffer_type type, const char *name,
        uint32_t modifiers, const struct hlsl_reg_reservation *reservation, struct hlsl_scope *annotations,
        const struct vkd3d_shader_location *loc)
{
    struct hlsl_buffer *buffer;

    if (!(buffer = hlsl_alloc(ctx, sizeof(*buffer))))
        return NULL;
    buffer->type = type;
    buffer->name = name;
    buffer->modifiers = modifiers;
    if (reservation)
        buffer->reservation = *reservation;
    buffer->annotations = annotations;
    buffer->loc = *loc;
    list_add_tail(&ctx->buffers, &buffer->entry);
    return buffer;
}

static int compare_hlsl_types_rb(const void *key, const struct rb_entry *entry)
{
    const struct hlsl_type *type = RB_ENTRY_VALUE(entry, const struct hlsl_type, scope_entry);
    const char *name = key;

    if (name == type->name)
        return 0;

    if (!name || !type->name)
    {
        ERR("hlsl_type without a name in a scope?\n");
        return -1;
    }
    return strcmp(name, type->name);
}

static struct hlsl_scope *hlsl_new_scope(struct hlsl_ctx *ctx, struct hlsl_scope *upper)
{
    struct hlsl_scope *scope;

    if (!(scope = hlsl_alloc(ctx, sizeof(*scope))))
        return NULL;
    list_init(&scope->vars);
    rb_init(&scope->types, compare_hlsl_types_rb);
    scope->upper = upper;
    list_add_tail(&ctx->scopes, &scope->entry);
    return scope;
}

void hlsl_push_scope(struct hlsl_ctx *ctx)
{
    struct hlsl_scope *new_scope;

    if (!(new_scope = hlsl_new_scope(ctx, ctx->cur_scope)))
        return;

    TRACE("Pushing a new scope.\n");
    ctx->cur_scope = new_scope;
}

void hlsl_pop_scope(struct hlsl_ctx *ctx)
{
    struct hlsl_scope *prev_scope = ctx->cur_scope->upper;

    VKD3D_ASSERT(prev_scope);
    TRACE("Popping current scope.\n");
    ctx->cur_scope = prev_scope;
}

static bool func_decl_matches(const struct hlsl_ir_function_decl *decl,
        const struct hlsl_func_parameters *parameters)
{
    size_t i;

    if (parameters->count != decl->parameters.count)
        return false;

    for (i = 0; i < parameters->count; ++i)
    {
        if (!hlsl_types_are_equal(parameters->vars[i]->data_type, decl->parameters.vars[i]->data_type))
            return false;
    }
    return true;
}

struct hlsl_ir_function_decl *hlsl_get_func_decl(struct hlsl_ctx *ctx, const char *name,
        const struct hlsl_func_parameters *parameters)
{
    struct hlsl_ir_function_decl *decl;
    struct hlsl_ir_function *func;

    if (!(func = hlsl_get_function(ctx, name)))
        return NULL;

    LIST_FOR_EACH_ENTRY(decl, &func->overloads, struct hlsl_ir_function_decl, entry)
    {
        if (func_decl_matches(decl, parameters))
            return decl;
    }

    return NULL;
}

struct vkd3d_string_buffer *hlsl_type_to_string(struct hlsl_ctx *ctx, const struct hlsl_type *type)
{
    struct vkd3d_string_buffer *string, *inner_string;

    static const char *const base_types[] =
    {
        [HLSL_TYPE_FLOAT] = "float",
        [HLSL_TYPE_HALF] = "half",
        [HLSL_TYPE_DOUBLE] = "double",
        [HLSL_TYPE_INT] = "int",
        [HLSL_TYPE_UINT] = "uint",
        [HLSL_TYPE_BOOL] = "bool",
    };

    static const char *const dimensions[] =
    {
        [HLSL_SAMPLER_DIM_1D]        = "1D",
        [HLSL_SAMPLER_DIM_2D]        = "2D",
        [HLSL_SAMPLER_DIM_3D]        = "3D",
        [HLSL_SAMPLER_DIM_CUBE]      = "Cube",
        [HLSL_SAMPLER_DIM_1DARRAY]   = "1DArray",
        [HLSL_SAMPLER_DIM_2DARRAY]   = "2DArray",
        [HLSL_SAMPLER_DIM_2DMS]      = "2DMS",
        [HLSL_SAMPLER_DIM_2DMSARRAY] = "2DMSArray",
        [HLSL_SAMPLER_DIM_CUBEARRAY] = "CubeArray",
    };

    if (!(string = hlsl_get_string_buffer(ctx)))
        return NULL;

    if (type->name)
    {
        vkd3d_string_buffer_printf(string, "%s", type->name);
        return string;
    }

    switch (type->class)
    {
        case HLSL_CLASS_SCALAR:
            VKD3D_ASSERT(type->e.numeric.type < ARRAY_SIZE(base_types));
            vkd3d_string_buffer_printf(string, "%s", base_types[type->e.numeric.type]);
            return string;

        case HLSL_CLASS_VECTOR:
            VKD3D_ASSERT(type->e.numeric.type < ARRAY_SIZE(base_types));
            vkd3d_string_buffer_printf(string, "%s%u", base_types[type->e.numeric.type], type->dimx);
            return string;

        case HLSL_CLASS_MATRIX:
            VKD3D_ASSERT(type->e.numeric.type < ARRAY_SIZE(base_types));
            vkd3d_string_buffer_printf(string, "%s%ux%u", base_types[type->e.numeric.type], type->dimy, type->dimx);
            return string;

        case HLSL_CLASS_ARRAY:
        {
            const struct hlsl_type *t;

            for (t = type; t->class == HLSL_CLASS_ARRAY; t = t->e.array.type)
                ;

            if ((inner_string = hlsl_type_to_string(ctx, t)))
            {
                vkd3d_string_buffer_printf(string, "%s", inner_string->buffer);
                hlsl_release_string_buffer(ctx, inner_string);
            }

            for (t = type; t->class == HLSL_CLASS_ARRAY; t = t->e.array.type)
            {
                if (t->e.array.elements_count == HLSL_ARRAY_ELEMENTS_COUNT_IMPLICIT)
                    vkd3d_string_buffer_printf(string, "[]");
                else
                    vkd3d_string_buffer_printf(string, "[%u]", t->e.array.elements_count);
            }
            return string;
        }

        case HLSL_CLASS_STRUCT:
            vkd3d_string_buffer_printf(string, "<anonymous struct>");
            return string;

        case HLSL_CLASS_TEXTURE:
            if (type->sampler_dim == HLSL_SAMPLER_DIM_RAW_BUFFER)
            {
                vkd3d_string_buffer_printf(string, "ByteAddressBuffer");
                return string;
            }

            if (type->sampler_dim == HLSL_SAMPLER_DIM_GENERIC)
            {
                vkd3d_string_buffer_printf(string, "Texture");
                return string;
            }

            VKD3D_ASSERT(hlsl_is_numeric_type(type->e.resource.format));
            VKD3D_ASSERT(type->e.resource.format->e.numeric.type < ARRAY_SIZE(base_types));
            if (type->sampler_dim == HLSL_SAMPLER_DIM_BUFFER)
            {
                vkd3d_string_buffer_printf(string, "Buffer");
            }
            else
            {
                VKD3D_ASSERT(type->sampler_dim < ARRAY_SIZE(dimensions));
                vkd3d_string_buffer_printf(string, "Texture%s", dimensions[type->sampler_dim]);
            }
            if ((inner_string = hlsl_type_to_string(ctx, type->e.resource.format)))
            {
                vkd3d_string_buffer_printf(string, "<%s>", inner_string->buffer);
                hlsl_release_string_buffer(ctx, inner_string);
            }
            return string;

        case HLSL_CLASS_UAV:
            if (type->sampler_dim == HLSL_SAMPLER_DIM_RAW_BUFFER)
            {
                vkd3d_string_buffer_printf(string, "RWByteAddressBuffer");
                return string;
            }
            if (type->sampler_dim == HLSL_SAMPLER_DIM_BUFFER)
                vkd3d_string_buffer_printf(string, "RWBuffer");
            else if (type->sampler_dim == HLSL_SAMPLER_DIM_STRUCTURED_BUFFER)
                vkd3d_string_buffer_printf(string, "RWStructuredBuffer");
            else
                vkd3d_string_buffer_printf(string, "RWTexture%s", dimensions[type->sampler_dim]);
            if ((inner_string = hlsl_type_to_string(ctx, type->e.resource.format)))
            {
                vkd3d_string_buffer_printf(string, "<%s>", inner_string->buffer);
                hlsl_release_string_buffer(ctx, inner_string);
            }
            return string;

        case HLSL_CLASS_CONSTANT_BUFFER:
            vkd3d_string_buffer_printf(string, "ConstantBuffer");
            if ((inner_string = hlsl_type_to_string(ctx, type->e.resource.format)))
            {
                vkd3d_string_buffer_printf(string, "<%s>", inner_string->buffer);
                hlsl_release_string_buffer(ctx, inner_string);
            }
            return string;

        case HLSL_CLASS_ERROR:
            vkd3d_string_buffer_printf(string, "<error type>");
            return string;

        case HLSL_CLASS_DEPTH_STENCIL_STATE:
        case HLSL_CLASS_DEPTH_STENCIL_VIEW:
        case HLSL_CLASS_EFFECT_GROUP:
        case HLSL_CLASS_PASS:
        case HLSL_CLASS_PIXEL_SHADER:
        case HLSL_CLASS_RASTERIZER_STATE:
        case HLSL_CLASS_RENDER_TARGET_VIEW:
        case HLSL_CLASS_SAMPLER:
        case HLSL_CLASS_STRING:
        case HLSL_CLASS_TECHNIQUE:
        case HLSL_CLASS_VERTEX_SHADER:
        case HLSL_CLASS_VOID:
        case HLSL_CLASS_COMPUTE_SHADER:
        case HLSL_CLASS_DOMAIN_SHADER:
        case HLSL_CLASS_HULL_SHADER:
        case HLSL_CLASS_GEOMETRY_SHADER:
        case HLSL_CLASS_BLEND_STATE:
        case HLSL_CLASS_NULL:
            break;
    }

    vkd3d_string_buffer_printf(string, "<unexpected type>");
    return string;
}

struct vkd3d_string_buffer *hlsl_component_to_string(struct hlsl_ctx *ctx, const struct hlsl_ir_var *var,
        unsigned int index)
{
    struct hlsl_type *type = var->data_type, *current_type;
    struct vkd3d_string_buffer *buffer;
    unsigned int element_index;

    if (!(buffer = hlsl_get_string_buffer(ctx)))
        return NULL;

    vkd3d_string_buffer_printf(buffer, "%s", var->name);

    while (!type_is_single_component(type))
    {
        current_type = type;
        element_index = traverse_path_from_component_index(ctx, &type, &index);
        if (current_type->class == HLSL_CLASS_STRUCT)
            vkd3d_string_buffer_printf(buffer, ".%s", current_type->e.record.fields[element_index].name);
        else
            vkd3d_string_buffer_printf(buffer, "[%u]", element_index);
    }

    return buffer;
}

const char *debug_hlsl_type(struct hlsl_ctx *ctx, const struct hlsl_type *type)
{
    struct vkd3d_string_buffer *string;
    const char *ret;

    if (!(string = hlsl_type_to_string(ctx, type)))
        return NULL;
    ret = vkd3d_dbg_sprintf("%s", string->buffer);
    hlsl_release_string_buffer(ctx, string);
    return ret;
}

struct vkd3d_string_buffer *hlsl_modifiers_to_string(struct hlsl_ctx *ctx, uint32_t modifiers)
{
    struct vkd3d_string_buffer *string;

    if (!(string = hlsl_get_string_buffer(ctx)))
        return NULL;

    if (modifiers & HLSL_STORAGE_EXTERN)
        vkd3d_string_buffer_printf(string, "extern ");
    if (modifiers & HLSL_STORAGE_LINEAR)
        vkd3d_string_buffer_printf(string, "linear ");
    if (modifiers & HLSL_STORAGE_NOINTERPOLATION)
        vkd3d_string_buffer_printf(string, "nointerpolation ");
    if (modifiers & HLSL_STORAGE_CENTROID)
        vkd3d_string_buffer_printf(string, "centroid ");
    if (modifiers & HLSL_STORAGE_NOPERSPECTIVE)
        vkd3d_string_buffer_printf(string, "noperspective ");
    if (modifiers & HLSL_MODIFIER_PRECISE)
        vkd3d_string_buffer_printf(string, "precise ");
    if (modifiers & HLSL_STORAGE_SHARED)
        vkd3d_string_buffer_printf(string, "shared ");
    if (modifiers & HLSL_STORAGE_GROUPSHARED)
        vkd3d_string_buffer_printf(string, "groupshared ");
    if (modifiers & HLSL_STORAGE_STATIC)
        vkd3d_string_buffer_printf(string, "static ");
    if (modifiers & HLSL_STORAGE_UNIFORM)
        vkd3d_string_buffer_printf(string, "uniform ");
    if (modifiers & HLSL_MODIFIER_VOLATILE)
        vkd3d_string_buffer_printf(string, "volatile ");
    if (modifiers & HLSL_MODIFIER_CONST)
        vkd3d_string_buffer_printf(string, "const ");
    if (modifiers & HLSL_MODIFIER_ROW_MAJOR)
        vkd3d_string_buffer_printf(string, "row_major ");
    if (modifiers & HLSL_MODIFIER_COLUMN_MAJOR)
        vkd3d_string_buffer_printf(string, "column_major ");
    if ((modifiers & (HLSL_STORAGE_IN | HLSL_STORAGE_OUT)) == (HLSL_STORAGE_IN | HLSL_STORAGE_OUT))
        vkd3d_string_buffer_printf(string, "inout ");
    else if (modifiers & HLSL_STORAGE_IN)
        vkd3d_string_buffer_printf(string, "in ");
    else if (modifiers & HLSL_STORAGE_OUT)
        vkd3d_string_buffer_printf(string, "out ");

    if (string->content_size)
        string->buffer[--string->content_size] = 0;

    return string;
}

const char *hlsl_node_type_to_string(enum hlsl_ir_node_type type)
{
    static const char * const names[] =
    {
        [HLSL_IR_CALL           ] = "HLSL_IR_CALL",
        [HLSL_IR_CONSTANT       ] = "HLSL_IR_CONSTANT",
        [HLSL_IR_EXPR           ] = "HLSL_IR_EXPR",
        [HLSL_IR_IF             ] = "HLSL_IR_IF",
        [HLSL_IR_INDEX          ] = "HLSL_IR_INDEX",
        [HLSL_IR_LOAD           ] = "HLSL_IR_LOAD",
        [HLSL_IR_LOOP           ] = "HLSL_IR_LOOP",
        [HLSL_IR_JUMP           ] = "HLSL_IR_JUMP",
        [HLSL_IR_RESOURCE_LOAD  ] = "HLSL_IR_RESOURCE_LOAD",
        [HLSL_IR_RESOURCE_STORE ] = "HLSL_IR_RESOURCE_STORE",
        [HLSL_IR_STRING_CONSTANT] = "HLSL_IR_STRING_CONSTANT",
        [HLSL_IR_STORE          ] = "HLSL_IR_STORE",
        [HLSL_IR_SWITCH         ] = "HLSL_IR_SWITCH",
        [HLSL_IR_SWIZZLE        ] = "HLSL_IR_SWIZZLE",

        [HLSL_IR_COMPILE]             = "HLSL_IR_COMPILE",
        [HLSL_IR_SAMPLER_STATE]       = "HLSL_IR_SAMPLER_STATE",
        [HLSL_IR_STATEBLOCK_CONSTANT] = "HLSL_IR_STATEBLOCK_CONSTANT",
        [HLSL_IR_VSIR_INSTRUCTION_REF] = "HLSL_IR_VSIR_INSTRUCTION_REF",
    };

    if (type >= ARRAY_SIZE(names))
        return "Unexpected node type";
    return names[type];
}

const char *hlsl_jump_type_to_string(enum hlsl_ir_jump_type type)
{
    static const char * const names[] =
    {
        [HLSL_IR_JUMP_BREAK]       = "HLSL_IR_JUMP_BREAK",
        [HLSL_IR_JUMP_CONTINUE]    = "HLSL_IR_JUMP_CONTINUE",
        [HLSL_IR_JUMP_DISCARD_NEG] = "HLSL_IR_JUMP_DISCARD_NEG",
        [HLSL_IR_JUMP_DISCARD_NZ]  = "HLSL_IR_JUMP_DISCARD_NZ",
        [HLSL_IR_JUMP_RETURN]      = "HLSL_IR_JUMP_RETURN",
    };

    VKD3D_ASSERT(type < ARRAY_SIZE(names));
    return names[type];
}

static void dump_instr(struct hlsl_ctx *ctx, struct vkd3d_string_buffer *buffer, const struct hlsl_ir_node *instr);

static void dump_block(struct hlsl_ctx *ctx, struct vkd3d_string_buffer *buffer, const struct hlsl_block *block)
{
    struct hlsl_ir_node *instr;

    LIST_FOR_EACH_ENTRY(instr, &block->instrs, struct hlsl_ir_node, entry)
    {
        dump_instr(ctx, buffer, instr);
        vkd3d_string_buffer_printf(buffer, "\n");
    }
}

static void dump_src(struct vkd3d_string_buffer *buffer, const struct hlsl_src *src)
{
    if (src->node->index)
        vkd3d_string_buffer_printf(buffer, "@%u", src->node->index);
    else
        vkd3d_string_buffer_printf(buffer, "%p", src->node);
}

static void dump_ir_var(struct hlsl_ctx *ctx, struct vkd3d_string_buffer *buffer, const struct hlsl_ir_var *var)
{
    if (var->storage_modifiers)
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_modifiers_to_string(ctx, var->storage_modifiers)))
            vkd3d_string_buffer_printf(buffer, "%s ", string->buffer);
        hlsl_release_string_buffer(ctx, string);
    }
    vkd3d_string_buffer_printf(buffer, "%s %s", debug_hlsl_type(ctx, var->data_type), var->name);
    if (var->semantic.name)
        vkd3d_string_buffer_printf(buffer, " : %s%u", var->semantic.name, var->semantic.index);
}

static void dump_deref(struct vkd3d_string_buffer *buffer, const struct hlsl_deref *deref)
{
    unsigned int i;

    if (deref->var)
    {
        vkd3d_string_buffer_printf(buffer, "%s", deref->var->name);
        if (!hlsl_deref_is_lowered(deref))
        {
            if (deref->path_len)
            {
                vkd3d_string_buffer_printf(buffer, "[");
                for (i = 0; i < deref->path_len; ++i)
                {
                    vkd3d_string_buffer_printf(buffer, "[");
                    dump_src(buffer, &deref->path[i]);
                    vkd3d_string_buffer_printf(buffer, "]");
                }
                vkd3d_string_buffer_printf(buffer, "]");
            }
        }
        else
        {
            bool show_rel, show_const;

            show_rel = deref->rel_offset.node;
            show_const = deref->const_offset != 0 || !show_rel;

            vkd3d_string_buffer_printf(buffer, "[");
            if (show_rel)
                dump_src(buffer, &deref->rel_offset);
            if (show_rel && show_const)
                vkd3d_string_buffer_printf(buffer, " + ");
            if (show_const)
                vkd3d_string_buffer_printf(buffer, "%uc", deref->const_offset);
            vkd3d_string_buffer_printf(buffer, "]");
        }
    }
    else
    {
        vkd3d_string_buffer_printf(buffer, "(nil)");
    }
}

const char *debug_hlsl_writemask(unsigned int writemask)
{
    static const char components[] = {'x', 'y', 'z', 'w'};
    char string[5];
    unsigned int i = 0, pos = 0;

    VKD3D_ASSERT(!(writemask & ~VKD3DSP_WRITEMASK_ALL));

    while (writemask)
    {
        if (writemask & 1)
            string[pos++] = components[i];
        writemask >>= 1;
        i++;
    }
    string[pos] = '\0';
    return vkd3d_dbg_sprintf(".%s", string);
}

const char *debug_hlsl_swizzle(uint32_t swizzle, unsigned int size)
{
    static const char components[] = {'x', 'y', 'z', 'w'};
    char string[5];
    unsigned int i;

    VKD3D_ASSERT(size <= ARRAY_SIZE(components));
    for (i = 0; i < size; ++i)
        string[i] = components[hlsl_swizzle_get_component(swizzle, i)];
    string[size] = 0;
    return vkd3d_dbg_sprintf(".%s", string);
}

static void dump_ir_call(struct hlsl_ctx *ctx, struct vkd3d_string_buffer *buffer, const struct hlsl_ir_call *call)
{
    const struct hlsl_ir_function_decl *decl = call->decl;
    struct vkd3d_string_buffer *string;
    size_t i;

    if (!(string = hlsl_type_to_string(ctx, decl->return_type)))
        return;

    vkd3d_string_buffer_printf(buffer, "call %s %s(", string->buffer, decl->func->name);
    hlsl_release_string_buffer(ctx, string);

    for (i = 0; i < decl->parameters.count; ++i)
    {
        const struct hlsl_ir_var *param = decl->parameters.vars[i];

        if (!(string = hlsl_type_to_string(ctx, param->data_type)))
            return;

        if (i)
            vkd3d_string_buffer_printf(buffer, ", ");
        vkd3d_string_buffer_printf(buffer, "%s", string->buffer);

        hlsl_release_string_buffer(ctx, string);
    }
    vkd3d_string_buffer_printf(buffer, ")");
}

static void dump_ir_constant(struct vkd3d_string_buffer *buffer, const struct hlsl_ir_constant *constant)
{
    struct hlsl_type *type = constant->node.data_type;
    unsigned int x;

    if (type->dimx != 1)
        vkd3d_string_buffer_printf(buffer, "{");
    for (x = 0; x < type->dimx; ++x)
    {
        const union hlsl_constant_value_component *value = &constant->value.u[x];

        switch (type->e.numeric.type)
        {
            case HLSL_TYPE_BOOL:
                vkd3d_string_buffer_printf(buffer, "%s ", value->u ? "true" : "false");
                break;

            case HLSL_TYPE_DOUBLE:
                vkd3d_string_buffer_printf(buffer, "%.16e ", value->d);
                break;

            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                vkd3d_string_buffer_printf(buffer, "%.8e ", value->f);
                break;

            case HLSL_TYPE_INT:
                vkd3d_string_buffer_printf(buffer, "%d ", value->i);
                break;

            case HLSL_TYPE_UINT:
                vkd3d_string_buffer_printf(buffer, "%u ", value->u);
                break;

            default:
                vkd3d_unreachable();
        }
    }
    if (type->dimx != 1)
        vkd3d_string_buffer_printf(buffer, "}");
}

const char *debug_hlsl_expr_op(enum hlsl_ir_expr_op op)
{
    static const char *const op_names[] =
    {
        [HLSL_OP0_ERROR]        = "error",
        [HLSL_OP0_VOID]         = "void",
        [HLSL_OP0_RASTERIZER_SAMPLE_COUNT] = "GetRenderTargetSampleCount",

        [HLSL_OP1_ABS]          = "abs",
        [HLSL_OP1_BIT_NOT]      = "~",
        [HLSL_OP1_CAST]         = "cast",
        [HLSL_OP1_CEIL]         = "ceil",
        [HLSL_OP1_COS]          = "cos",
        [HLSL_OP1_COS_REDUCED]  = "cos_reduced",
        [HLSL_OP1_DSX]          = "dsx",
        [HLSL_OP1_DSX_COARSE]   = "dsx_coarse",
        [HLSL_OP1_DSX_FINE]     = "dsx_fine",
        [HLSL_OP1_DSY]          = "dsy",
        [HLSL_OP1_DSY_COARSE]   = "dsy_coarse",
        [HLSL_OP1_DSY_FINE]     = "dsy_fine",
        [HLSL_OP1_EXP2]         = "exp2",
        [HLSL_OP1_F16TOF32]     = "f16tof32",
        [HLSL_OP1_F32TOF16]     = "f32tof16",
        [HLSL_OP1_FLOOR]        = "floor",
        [HLSL_OP1_FRACT]        = "fract",
        [HLSL_OP1_LOG2]         = "log2",
        [HLSL_OP1_LOGIC_NOT]    = "!",
        [HLSL_OP1_NEG]          = "-",
        [HLSL_OP1_NRM]          = "nrm",
        [HLSL_OP1_RCP]          = "rcp",
        [HLSL_OP1_REINTERPRET]  = "reinterpret",
        [HLSL_OP1_ROUND]        = "round",
        [HLSL_OP1_RSQ]          = "rsq",
        [HLSL_OP1_SAT]          = "sat",
        [HLSL_OP1_SIGN]         = "sign",
        [HLSL_OP1_SIN]          = "sin",
        [HLSL_OP1_SIN_REDUCED]  = "sin_reduced",
        [HLSL_OP1_SQRT]         = "sqrt",
        [HLSL_OP1_TRUNC]        = "trunc",

        [HLSL_OP2_ADD]         = "+",
        [HLSL_OP2_BIT_AND]     = "&",
        [HLSL_OP2_BIT_OR]      = "|",
        [HLSL_OP2_BIT_XOR]     = "^",
        [HLSL_OP2_CRS]         = "crs",
        [HLSL_OP2_DIV]         = "/",
        [HLSL_OP2_DOT]         = "dot",
        [HLSL_OP2_EQUAL]       = "==",
        [HLSL_OP2_GEQUAL]      = ">=",
        [HLSL_OP2_LESS]        = "<",
        [HLSL_OP2_LOGIC_AND]   = "&&",
        [HLSL_OP2_LOGIC_OR]    = "||",
        [HLSL_OP2_LSHIFT]      = "<<",
        [HLSL_OP2_MAX]         = "max",
        [HLSL_OP2_MIN]         = "min",
        [HLSL_OP2_MOD]         = "%",
        [HLSL_OP2_MUL]         = "*",
        [HLSL_OP2_NEQUAL]      = "!=",
        [HLSL_OP2_RSHIFT]      = ">>",
        [HLSL_OP2_SLT]         = "slt",

        [HLSL_OP3_CMP]         = "cmp",
        [HLSL_OP3_DP2ADD]      = "dp2add",
        [HLSL_OP3_TERNARY]     = "ternary",
        [HLSL_OP3_MAD]         = "mad",
    };

    return op_names[op];
}

static void dump_ir_expr(struct vkd3d_string_buffer *buffer, const struct hlsl_ir_expr *expr)
{
    unsigned int i;

    vkd3d_string_buffer_printf(buffer, "%s (", debug_hlsl_expr_op(expr->op));
    for (i = 0; i < HLSL_MAX_OPERANDS && expr->operands[i].node; ++i)
    {
        dump_src(buffer, &expr->operands[i]);
        vkd3d_string_buffer_printf(buffer, " ");
    }
    vkd3d_string_buffer_printf(buffer, ")");
}

static void dump_ir_if(struct hlsl_ctx *ctx, struct vkd3d_string_buffer *buffer, const struct hlsl_ir_if *if_node)
{
    vkd3d_string_buffer_printf(buffer, "if (");
    dump_src(buffer, &if_node->condition);
    vkd3d_string_buffer_printf(buffer, ") {\n");
    dump_block(ctx, buffer, &if_node->then_block);
    vkd3d_string_buffer_printf(buffer, "      %10s   } else {\n", "");
    dump_block(ctx, buffer, &if_node->else_block);
    vkd3d_string_buffer_printf(buffer, "      %10s   }", "");
}

static void dump_ir_jump(struct vkd3d_string_buffer *buffer, const struct hlsl_ir_jump *jump)
{
    switch (jump->type)
    {
        case HLSL_IR_JUMP_BREAK:
            vkd3d_string_buffer_printf(buffer, "break");
            break;

        case HLSL_IR_JUMP_CONTINUE:
            vkd3d_string_buffer_printf(buffer, "continue");
            break;

        case HLSL_IR_JUMP_DISCARD_NEG:
            vkd3d_string_buffer_printf(buffer, "discard_neg");
            break;

        case HLSL_IR_JUMP_DISCARD_NZ:
            vkd3d_string_buffer_printf(buffer, "discard_nz");
            break;

        case HLSL_IR_JUMP_RETURN:
            vkd3d_string_buffer_printf(buffer, "return");
            break;

        case HLSL_IR_JUMP_UNRESOLVED_CONTINUE:
            vkd3d_string_buffer_printf(buffer, "unresolved_continue");
            break;
    }
}

static void dump_ir_loop(struct hlsl_ctx *ctx, struct vkd3d_string_buffer *buffer, const struct hlsl_ir_loop *loop)
{
    vkd3d_string_buffer_printf(buffer, "for (;;) {\n");
    dump_block(ctx, buffer, &loop->body);
    vkd3d_string_buffer_printf(buffer, "      %10s   }", "");
}

static void dump_ir_resource_load(struct vkd3d_string_buffer *buffer, const struct hlsl_ir_resource_load *load)
{
    static const char *const type_names[] =
    {
        [HLSL_RESOURCE_LOAD] = "load_resource",
        [HLSL_RESOURCE_SAMPLE] = "sample",
        [HLSL_RESOURCE_SAMPLE_CMP] = "sample_cmp",
        [HLSL_RESOURCE_SAMPLE_CMP_LZ] = "sample_cmp_lz",
        [HLSL_RESOURCE_SAMPLE_LOD] = "sample_lod",
        [HLSL_RESOURCE_SAMPLE_LOD_BIAS] = "sample_biased",
        [HLSL_RESOURCE_SAMPLE_GRAD] = "sample_grad",
        [HLSL_RESOURCE_GATHER_RED] = "gather_red",
        [HLSL_RESOURCE_GATHER_GREEN] = "gather_green",
        [HLSL_RESOURCE_GATHER_BLUE] = "gather_blue",
        [HLSL_RESOURCE_GATHER_ALPHA] = "gather_alpha",
        [HLSL_RESOURCE_SAMPLE_INFO] = "sample_info",
        [HLSL_RESOURCE_RESINFO] = "resinfo",
    };

    VKD3D_ASSERT(load->load_type < ARRAY_SIZE(type_names));
    vkd3d_string_buffer_printf(buffer, "%s(resource = ", type_names[load->load_type]);
    dump_deref(buffer, &load->resource);
    vkd3d_string_buffer_printf(buffer, ", sampler = ");
    dump_deref(buffer, &load->sampler);
    if (load->coords.node)
    {
        vkd3d_string_buffer_printf(buffer, ", coords = ");
        dump_src(buffer, &load->coords);
    }
    if (load->sample_index.node)
    {
        vkd3d_string_buffer_printf(buffer, ", sample index = ");
        dump_src(buffer, &load->sample_index);
    }
    if (load->texel_offset.node)
    {
        vkd3d_string_buffer_printf(buffer, ", offset = ");
        dump_src(buffer, &load->texel_offset);
    }
    if (load->lod.node)
    {
        vkd3d_string_buffer_printf(buffer, ", lod = ");
        dump_src(buffer, &load->lod);
    }
    if (load->ddx.node)
    {
        vkd3d_string_buffer_printf(buffer, ", ddx = ");
        dump_src(buffer, &load->ddx);
    }
    if (load->ddy.node)
    {
        vkd3d_string_buffer_printf(buffer, ", ddy = ");
        dump_src(buffer, &load->ddy);
    }
    if (load->cmp.node)
    {
        vkd3d_string_buffer_printf(buffer, ", cmp = ");
        dump_src(buffer, &load->cmp);
    }
    vkd3d_string_buffer_printf(buffer, ")");
}

static void dump_ir_resource_store(struct vkd3d_string_buffer *buffer, const struct hlsl_ir_resource_store *store)
{
    vkd3d_string_buffer_printf(buffer, "store_resource(resource = ");
    dump_deref(buffer, &store->resource);
    vkd3d_string_buffer_printf(buffer, ", coords = ");
    dump_src(buffer, &store->coords);
    vkd3d_string_buffer_printf(buffer, ", value = ");
    dump_src(buffer, &store->value);
    vkd3d_string_buffer_printf(buffer, ")");
}

static void dump_ir_string(struct vkd3d_string_buffer *buffer, const struct hlsl_ir_string_constant *string)
{
    vkd3d_string_buffer_printf(buffer, "\"%s\"", debugstr_a(string->string));
}

static void dump_ir_store(struct vkd3d_string_buffer *buffer, const struct hlsl_ir_store *store)
{
    vkd3d_string_buffer_printf(buffer, "= (");
    dump_deref(buffer, &store->lhs);
    if (store->writemask != VKD3DSP_WRITEMASK_ALL)
        vkd3d_string_buffer_printf(buffer, "%s", debug_hlsl_writemask(store->writemask));
    vkd3d_string_buffer_printf(buffer, " ");
    dump_src(buffer, &store->rhs);
    vkd3d_string_buffer_printf(buffer, ")");
}

static void dump_ir_swizzle(struct vkd3d_string_buffer *buffer, const struct hlsl_ir_swizzle *swizzle)
{
    unsigned int i;

    dump_src(buffer, &swizzle->val);
    if (swizzle->val.node->data_type->dimy > 1)
    {
        vkd3d_string_buffer_printf(buffer, ".");
        for (i = 0; i < swizzle->node.data_type->dimx; ++i)
            vkd3d_string_buffer_printf(buffer, "_m%u%u", (swizzle->swizzle >> i * 8) & 0xf, (swizzle->swizzle >> (i * 8 + 4)) & 0xf);
    }
    else
    {
        vkd3d_string_buffer_printf(buffer, "%s", debug_hlsl_swizzle(swizzle->swizzle, swizzle->node.data_type->dimx));
    }
}

static void dump_ir_index(struct vkd3d_string_buffer *buffer, const struct hlsl_ir_index *index)
{
    dump_src(buffer, &index->val);
    vkd3d_string_buffer_printf(buffer, "[idx:");
    dump_src(buffer, &index->idx);
    vkd3d_string_buffer_printf(buffer, "]");
}

static void dump_ir_compile(struct hlsl_ctx *ctx, struct vkd3d_string_buffer *buffer,
        const struct hlsl_ir_compile *compile)
{
    unsigned int i;

    switch (compile->compile_type)
    {
        case HLSL_COMPILE_TYPE_COMPILE:
            vkd3d_string_buffer_printf(buffer, "compile %s {\n", compile->profile->name);
            break;

        case HLSL_COMPILE_TYPE_CONSTRUCTGSWITHSO:
            vkd3d_string_buffer_printf(buffer, "ConstructGSWithSO {\n");
            break;
    }

    dump_block(ctx, buffer, &compile->instrs);

    vkd3d_string_buffer_printf(buffer, "      %10s   } (", "");
    for (i = 0; i < compile->args_count; ++i)
    {
        dump_src(buffer, &compile->args[i]);
        if (i + 1 < compile->args_count)
            vkd3d_string_buffer_printf(buffer, ", ");
    }
    vkd3d_string_buffer_printf(buffer, ")");
}

static void dump_ir_sampler_state(struct vkd3d_string_buffer *buffer,
        const struct hlsl_ir_sampler_state *sampler_state)
{
    vkd3d_string_buffer_printf(buffer, "sampler_state {...}");
}

static void dump_ir_stateblock_constant(struct vkd3d_string_buffer *buffer,
        const struct hlsl_ir_stateblock_constant *constant)
{
    vkd3d_string_buffer_printf(buffer, "%s", constant->name);
}

static void dump_ir_switch(struct hlsl_ctx *ctx, struct vkd3d_string_buffer *buffer, const struct hlsl_ir_switch *s)
{
    struct hlsl_ir_switch_case *c;

    vkd3d_string_buffer_printf(buffer, "switch (");
    dump_src(buffer, &s->selector);
    vkd3d_string_buffer_printf(buffer, ") {\n");

    LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
    {
        if (c->is_default)
        {
            vkd3d_string_buffer_printf(buffer, "      %10s   default: {\n", "");
        }
        else
        {
            vkd3d_string_buffer_printf(buffer, "      %10s   case %u : {\n", "", c->value);
        }

        dump_block(ctx, buffer, &c->body);
        vkd3d_string_buffer_printf(buffer, "      %10s   }\n", "");
    }

    vkd3d_string_buffer_printf(buffer, "      %10s   }", "");
}

static void dump_instr(struct hlsl_ctx *ctx, struct vkd3d_string_buffer *buffer, const struct hlsl_ir_node *instr)
{
    if (instr->index)
        vkd3d_string_buffer_printf(buffer, "%4u: ", instr->index);
    else
        vkd3d_string_buffer_printf(buffer, "%p: ", instr);

    vkd3d_string_buffer_printf(buffer, "%10s | ", instr->data_type ? debug_hlsl_type(ctx, instr->data_type) : "");

    switch (instr->type)
    {
        case HLSL_IR_CALL:
            dump_ir_call(ctx, buffer, hlsl_ir_call(instr));
            break;

        case HLSL_IR_CONSTANT:
            dump_ir_constant(buffer, hlsl_ir_constant(instr));
            break;

        case HLSL_IR_EXPR:
            dump_ir_expr(buffer, hlsl_ir_expr(instr));
            break;

        case HLSL_IR_IF:
            dump_ir_if(ctx, buffer, hlsl_ir_if(instr));
            break;

        case HLSL_IR_INDEX:
            dump_ir_index(buffer, hlsl_ir_index(instr));
            break;

        case HLSL_IR_JUMP:
            dump_ir_jump(buffer, hlsl_ir_jump(instr));
            break;

        case HLSL_IR_LOAD:
            dump_deref(buffer, &hlsl_ir_load(instr)->src);
            break;

        case HLSL_IR_LOOP:
            dump_ir_loop(ctx, buffer, hlsl_ir_loop(instr));
            break;

        case HLSL_IR_RESOURCE_LOAD:
            dump_ir_resource_load(buffer, hlsl_ir_resource_load(instr));
            break;

        case HLSL_IR_RESOURCE_STORE:
            dump_ir_resource_store(buffer, hlsl_ir_resource_store(instr));
            break;

        case HLSL_IR_STRING_CONSTANT:
            dump_ir_string(buffer, hlsl_ir_string_constant(instr));
            break;

        case HLSL_IR_STORE:
            dump_ir_store(buffer, hlsl_ir_store(instr));
            break;

        case HLSL_IR_SWITCH:
            dump_ir_switch(ctx, buffer, hlsl_ir_switch(instr));
            break;

        case HLSL_IR_SWIZZLE:
            dump_ir_swizzle(buffer, hlsl_ir_swizzle(instr));
            break;

        case HLSL_IR_COMPILE:
            dump_ir_compile(ctx, buffer, hlsl_ir_compile(instr));
            break;

        case HLSL_IR_SAMPLER_STATE:
            dump_ir_sampler_state(buffer, hlsl_ir_sampler_state(instr));
            break;

        case HLSL_IR_STATEBLOCK_CONSTANT:
            dump_ir_stateblock_constant(buffer, hlsl_ir_stateblock_constant(instr));
            break;

        case HLSL_IR_VSIR_INSTRUCTION_REF:
            vkd3d_string_buffer_printf(buffer, "vsir_program instruction %u",
                    hlsl_ir_vsir_instruction_ref(instr)->vsir_instr_idx);
            break;
    }
}

void hlsl_dump_function(struct hlsl_ctx *ctx, const struct hlsl_ir_function_decl *func)
{
    struct vkd3d_string_buffer buffer;
    size_t i;

    vkd3d_string_buffer_init(&buffer);
    vkd3d_string_buffer_printf(&buffer, "Dumping function %s.\n", func->func->name);
    vkd3d_string_buffer_printf(&buffer, "Function parameters:\n");
    for (i = 0; i < func->parameters.count; ++i)
    {
        dump_ir_var(ctx, &buffer, func->parameters.vars[i]);
        vkd3d_string_buffer_printf(&buffer, "\n");
    }
    if (func->has_body)
        dump_block(ctx, &buffer, &func->body);

    vkd3d_string_buffer_trace(&buffer);
    vkd3d_string_buffer_cleanup(&buffer);
}

void hlsl_dump_var_default_values(const struct hlsl_ir_var *var)
{
    unsigned int k, component_count = hlsl_type_component_count(var->data_type);
    struct vkd3d_string_buffer buffer;

    vkd3d_string_buffer_init(&buffer);
    if (!var->default_values)
    {
        vkd3d_string_buffer_printf(&buffer, "var \"%s\" has no default values.\n", var->name);
        vkd3d_string_buffer_trace(&buffer);
        vkd3d_string_buffer_cleanup(&buffer);
        return;
    }

    vkd3d_string_buffer_printf(&buffer, "var \"%s\" default values:", var->name);
    for (k = 0; k < component_count; ++k)
    {
        bool is_string = var->default_values[k].string;

        if (k % 4 == 0 || is_string)
            vkd3d_string_buffer_printf(&buffer, "\n   ");

        if (is_string)
            vkd3d_string_buffer_printf(&buffer, " %s", debugstr_a(var->default_values[k].string));
        else
            vkd3d_string_buffer_printf(&buffer, " 0x%08x", var->default_values[k].number.u);
    }
    vkd3d_string_buffer_printf(&buffer, "\n");

    vkd3d_string_buffer_trace(&buffer);
    vkd3d_string_buffer_cleanup(&buffer);
}

void hlsl_replace_node(struct hlsl_ir_node *old, struct hlsl_ir_node *new)
{
    struct hlsl_src *src, *next;

    VKD3D_ASSERT(old->data_type == new->data_type || old->data_type->dimx == new->data_type->dimx);
    VKD3D_ASSERT(old->data_type == new->data_type || old->data_type->dimy == new->data_type->dimy);

    LIST_FOR_EACH_ENTRY_SAFE(src, next, &old->uses, struct hlsl_src, entry)
    {
        hlsl_src_remove(src);
        hlsl_src_from_node(src, new);
    }
    list_remove(&old->entry);
    hlsl_free_instr(old);
}


void hlsl_free_type(struct hlsl_type *type)
{
    struct hlsl_struct_field *field;
    size_t i;

    vkd3d_free((void *)type->name);
    if (type->class == HLSL_CLASS_STRUCT)
    {
        for (i = 0; i < type->e.record.field_count; ++i)
        {
            field = &type->e.record.fields[i];

            vkd3d_free((void *)field->name);
            hlsl_cleanup_semantic(&field->semantic);
        }
        vkd3d_free((void *)type->e.record.fields);
    }
    vkd3d_free(type);
}

void hlsl_free_instr_list(struct list *list)
{
    struct hlsl_ir_node *node, *next_node;

    if (!list)
        return;
    /* Iterate in reverse, to avoid use-after-free when unlinking sources from
     * the "uses" list. */
    LIST_FOR_EACH_ENTRY_SAFE_REV(node, next_node, list, struct hlsl_ir_node, entry)
        hlsl_free_instr(node);
}

void hlsl_block_cleanup(struct hlsl_block *block)
{
    hlsl_free_instr_list(&block->instrs);
}

static void free_ir_call(struct hlsl_ir_call *call)
{
    vkd3d_free(call);
}

static void free_ir_constant(struct hlsl_ir_constant *constant)
{
    vkd3d_free(constant);
}

static void free_ir_expr(struct hlsl_ir_expr *expr)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(expr->operands); ++i)
        hlsl_src_remove(&expr->operands[i]);
    vkd3d_free(expr);
}

static void free_ir_if(struct hlsl_ir_if *if_node)
{
    hlsl_block_cleanup(&if_node->then_block);
    hlsl_block_cleanup(&if_node->else_block);
    hlsl_src_remove(&if_node->condition);
    vkd3d_free(if_node);
}

static void free_ir_jump(struct hlsl_ir_jump *jump)
{
    hlsl_src_remove(&jump->condition);
    vkd3d_free(jump);
}

static void free_ir_load(struct hlsl_ir_load *load)
{
    hlsl_cleanup_deref(&load->src);
    vkd3d_free(load);
}

static void free_ir_loop(struct hlsl_ir_loop *loop)
{
    hlsl_block_cleanup(&loop->body);
    vkd3d_free(loop);
}

static void free_ir_resource_load(struct hlsl_ir_resource_load *load)
{
    hlsl_cleanup_deref(&load->sampler);
    hlsl_cleanup_deref(&load->resource);
    hlsl_src_remove(&load->coords);
    hlsl_src_remove(&load->lod);
    hlsl_src_remove(&load->ddx);
    hlsl_src_remove(&load->ddy);
    hlsl_src_remove(&load->cmp);
    hlsl_src_remove(&load->texel_offset);
    hlsl_src_remove(&load->sample_index);
    vkd3d_free(load);
}

static void free_ir_string_constant(struct hlsl_ir_string_constant *string)
{
    vkd3d_free(string->string);
    vkd3d_free(string);
}

static void free_ir_resource_store(struct hlsl_ir_resource_store *store)
{
    hlsl_cleanup_deref(&store->resource);
    hlsl_src_remove(&store->coords);
    hlsl_src_remove(&store->value);
    vkd3d_free(store);
}

static void free_ir_store(struct hlsl_ir_store *store)
{
    hlsl_src_remove(&store->rhs);
    hlsl_cleanup_deref(&store->lhs);
    vkd3d_free(store);
}

static void free_ir_swizzle(struct hlsl_ir_swizzle *swizzle)
{
    hlsl_src_remove(&swizzle->val);
    vkd3d_free(swizzle);
}

static void free_ir_switch(struct hlsl_ir_switch *s)
{
    hlsl_src_remove(&s->selector);
    hlsl_cleanup_ir_switch_cases(&s->cases);

    vkd3d_free(s);
}

static void free_ir_index(struct hlsl_ir_index *index)
{
    hlsl_src_remove(&index->val);
    hlsl_src_remove(&index->idx);
    vkd3d_free(index);
}

static void free_ir_compile(struct hlsl_ir_compile *compile)
{
    unsigned int i;

    for (i = 0; i < compile->args_count; ++i)
        hlsl_src_remove(&compile->args[i]);

    hlsl_block_cleanup(&compile->instrs);
    vkd3d_free(compile);
}

static void free_ir_sampler_state(struct hlsl_ir_sampler_state *sampler_state)
{
    if (sampler_state->state_block)
        hlsl_free_state_block(sampler_state->state_block);
    vkd3d_free(sampler_state);
}

static void free_ir_stateblock_constant(struct hlsl_ir_stateblock_constant *constant)
{
    vkd3d_free(constant->name);
    vkd3d_free(constant);
}

void hlsl_free_instr(struct hlsl_ir_node *node)
{
    VKD3D_ASSERT(list_empty(&node->uses));

    switch (node->type)
    {
        case HLSL_IR_CALL:
            free_ir_call(hlsl_ir_call(node));
            break;

        case HLSL_IR_CONSTANT:
            free_ir_constant(hlsl_ir_constant(node));
            break;

        case HLSL_IR_EXPR:
            free_ir_expr(hlsl_ir_expr(node));
            break;

        case HLSL_IR_IF:
            free_ir_if(hlsl_ir_if(node));
            break;

        case HLSL_IR_INDEX:
            free_ir_index(hlsl_ir_index(node));
            break;

        case HLSL_IR_JUMP:
            free_ir_jump(hlsl_ir_jump(node));
            break;

        case HLSL_IR_LOAD:
            free_ir_load(hlsl_ir_load(node));
            break;

        case HLSL_IR_LOOP:
            free_ir_loop(hlsl_ir_loop(node));
            break;

        case HLSL_IR_RESOURCE_LOAD:
            free_ir_resource_load(hlsl_ir_resource_load(node));
            break;

        case HLSL_IR_STRING_CONSTANT:
            free_ir_string_constant(hlsl_ir_string_constant(node));
            break;

        case HLSL_IR_RESOURCE_STORE:
            free_ir_resource_store(hlsl_ir_resource_store(node));
            break;

        case HLSL_IR_STORE:
            free_ir_store(hlsl_ir_store(node));
            break;

        case HLSL_IR_SWIZZLE:
            free_ir_swizzle(hlsl_ir_swizzle(node));
            break;

        case HLSL_IR_SWITCH:
            free_ir_switch(hlsl_ir_switch(node));
            break;

        case HLSL_IR_COMPILE:
            free_ir_compile(hlsl_ir_compile(node));
            break;

        case HLSL_IR_SAMPLER_STATE:
            free_ir_sampler_state(hlsl_ir_sampler_state(node));
            break;

        case HLSL_IR_STATEBLOCK_CONSTANT:
            free_ir_stateblock_constant(hlsl_ir_stateblock_constant(node));
            break;

        case HLSL_IR_VSIR_INSTRUCTION_REF:
            vkd3d_free(hlsl_ir_vsir_instruction_ref(node));
            break;
    }
}

void hlsl_free_attribute(struct hlsl_attribute *attr)
{
    unsigned int i;

    for (i = 0; i < attr->args_count; ++i)
        hlsl_src_remove(&attr->args[i]);
    hlsl_block_cleanup(&attr->instrs);
    vkd3d_free((void *)attr->name);
    vkd3d_free(attr);
}

void hlsl_cleanup_semantic(struct hlsl_semantic *semantic)
{
    vkd3d_free((void *)semantic->name);
    vkd3d_free((void *)semantic->raw_name);
    memset(semantic, 0, sizeof(*semantic));
}

bool hlsl_clone_semantic(struct hlsl_ctx *ctx, struct hlsl_semantic *dst, const struct hlsl_semantic *src)
{
    *dst = *src;
    dst->name = dst->raw_name = NULL;
    if (src->name && !(dst->name = hlsl_strdup(ctx, src->name)))
        return false;
    if (src->raw_name && !(dst->raw_name = hlsl_strdup(ctx, src->raw_name)))
    {
        hlsl_cleanup_semantic(dst);
        return false;
    }

    return true;
}

static void free_function_decl(struct hlsl_ir_function_decl *decl)
{
    unsigned int i;

    for (i = 0; i < decl->attr_count; ++i)
        hlsl_free_attribute((void *)decl->attrs[i]);
    vkd3d_free((void *)decl->attrs);

    vkd3d_free(decl->parameters.vars);
    hlsl_block_cleanup(&decl->body);
    vkd3d_free(decl);
}

static void free_function(struct hlsl_ir_function *func)
{
    struct hlsl_ir_function_decl *decl, *next;

    LIST_FOR_EACH_ENTRY_SAFE(decl, next, &func->overloads, struct hlsl_ir_function_decl, entry)
        free_function_decl(decl);
    vkd3d_free((void *)func->name);
    vkd3d_free(func);
}

static void free_function_rb(struct rb_entry *entry, void *context)
{
    free_function(RB_ENTRY_VALUE(entry, struct hlsl_ir_function, entry));
}

void hlsl_add_function(struct hlsl_ctx *ctx, char *name, struct hlsl_ir_function_decl *decl)
{
    struct hlsl_ir_function *func;
    struct rb_entry *func_entry;

    if (ctx->internal_func_name)
    {
        char *internal_name;

        if (!(internal_name = hlsl_strdup(ctx, ctx->internal_func_name)))
            return;
        vkd3d_free(name);
        name = internal_name;
    }

    func_entry = rb_get(&ctx->functions, name);
    if (func_entry)
    {
        func = RB_ENTRY_VALUE(func_entry, struct hlsl_ir_function, entry);
        decl->func = func;
        list_add_tail(&func->overloads, &decl->entry);
        vkd3d_free(name);
        return;
    }
    func = hlsl_alloc(ctx, sizeof(*func));
    func->name = name;
    list_init(&func->overloads);
    decl->func = func;
    list_add_tail(&func->overloads, &decl->entry);
    rb_put(&ctx->functions, func->name, &func->entry);
}

uint32_t hlsl_map_swizzle(uint32_t swizzle, unsigned int writemask)
{
    uint32_t ret = 0;
    unsigned int i;

    /* Leave replicate swizzles alone; some instructions need them. */
    if (swizzle == HLSL_SWIZZLE(X, X, X, X)
            || swizzle == HLSL_SWIZZLE(Y, Y, Y, Y)
            || swizzle == HLSL_SWIZZLE(Z, Z, Z, Z)
            || swizzle == HLSL_SWIZZLE(W, W, W, W))
        return swizzle;

    for (i = 0; i < 4; ++i)
    {
        if (writemask & (1 << i))
        {
            ret |= (swizzle & 3) << (i * 2);
            swizzle >>= 2;
        }
    }
    return ret;
}

uint32_t hlsl_swizzle_from_writemask(unsigned int writemask)
{
    static const unsigned int swizzles[16] =
    {
        0,
        HLSL_SWIZZLE(X, X, X, X),
        HLSL_SWIZZLE(Y, Y, Y, Y),
        HLSL_SWIZZLE(X, Y, X, X),
        HLSL_SWIZZLE(Z, Z, Z, Z),
        HLSL_SWIZZLE(X, Z, X, X),
        HLSL_SWIZZLE(Y, Z, X, X),
        HLSL_SWIZZLE(X, Y, Z, X),
        HLSL_SWIZZLE(W, W, W, W),
        HLSL_SWIZZLE(X, W, X, X),
        HLSL_SWIZZLE(Y, W, X, X),
        HLSL_SWIZZLE(X, Y, W, X),
        HLSL_SWIZZLE(Z, W, X, X),
        HLSL_SWIZZLE(X, Z, W, X),
        HLSL_SWIZZLE(Y, Z, W, X),
        HLSL_SWIZZLE(X, Y, Z, W),
    };

    return swizzles[writemask & 0xf];
}

unsigned int hlsl_combine_writemasks(unsigned int first, unsigned int second)
{
    unsigned int ret = 0, i, j = 0;

    for (i = 0; i < 4; ++i)
    {
        if (first & (1 << i))
        {
            if (second & (1 << j++))
                ret |= (1 << i);
        }
    }

    return ret;
}

uint32_t hlsl_combine_swizzles(uint32_t first, uint32_t second, unsigned int dim)
{
    uint32_t ret = 0;
    unsigned int i;
    for (i = 0; i < dim; ++i)
    {
        unsigned int s = hlsl_swizzle_get_component(second, i);
        ret |= hlsl_swizzle_get_component(first, s) << HLSL_SWIZZLE_SHIFT(i);
    }
    return ret;
}

const struct hlsl_profile_info *hlsl_get_target_info(const char *target)
{
    unsigned int i;

    static const struct hlsl_profile_info profiles[] =
    {
        {"cs_4_0",              VKD3D_SHADER_TYPE_COMPUTE,  4, 0, 0, 0, false},
        {"cs_4_1",              VKD3D_SHADER_TYPE_COMPUTE,  4, 1, 0, 0, false},
        {"cs_5_0",              VKD3D_SHADER_TYPE_COMPUTE,  5, 0, 0, 0, false},
        {"cs_5_1",              VKD3D_SHADER_TYPE_COMPUTE,  5, 1, 0, 0, false},
        {"ds_5_0",              VKD3D_SHADER_TYPE_DOMAIN,   5, 0, 0, 0, false},
        {"ds_5_1",              VKD3D_SHADER_TYPE_DOMAIN,   5, 1, 0, 0, false},
        {"fx_2_0",              VKD3D_SHADER_TYPE_EFFECT,   2, 0, 0, 0, false},
        {"fx_4_0",              VKD3D_SHADER_TYPE_EFFECT,   4, 0, 0, 0, false},
        {"fx_4_1",              VKD3D_SHADER_TYPE_EFFECT,   4, 1, 0, 0, false},
        {"fx_5_0",              VKD3D_SHADER_TYPE_EFFECT,   5, 0, 0, 0, false},
        {"gs_4_0",              VKD3D_SHADER_TYPE_GEOMETRY, 4, 0, 0, 0, false},
        {"gs_4_1",              VKD3D_SHADER_TYPE_GEOMETRY, 4, 1, 0, 0, false},
        {"gs_5_0",              VKD3D_SHADER_TYPE_GEOMETRY, 5, 0, 0, 0, false},
        {"gs_5_1",              VKD3D_SHADER_TYPE_GEOMETRY, 5, 1, 0, 0, false},
        {"hs_5_0",              VKD3D_SHADER_TYPE_HULL,     5, 0, 0, 0, false},
        {"hs_5_1",              VKD3D_SHADER_TYPE_HULL,     5, 1, 0, 0, false},
        {"ps.1.0",              VKD3D_SHADER_TYPE_PIXEL,    1, 0, 0, 0, false},
        {"ps.1.1",              VKD3D_SHADER_TYPE_PIXEL,    1, 1, 0, 0, false},
        {"ps.1.2",              VKD3D_SHADER_TYPE_PIXEL,    1, 2, 0, 0, false},
        {"ps.1.3",              VKD3D_SHADER_TYPE_PIXEL,    1, 3, 0, 0, false},
        {"ps.1.4",              VKD3D_SHADER_TYPE_PIXEL,    1, 4, 0, 0, false},
        {"ps.2.0",              VKD3D_SHADER_TYPE_PIXEL,    2, 0, 0, 0, false},
        {"ps.2.a",              VKD3D_SHADER_TYPE_PIXEL,    2, 1, 0, 0, false},
        {"ps.2.b",              VKD3D_SHADER_TYPE_PIXEL,    2, 2, 0, 0, false},
        {"ps.2.sw",             VKD3D_SHADER_TYPE_PIXEL,    2, 0, 0, 0, true},
        {"ps.3.0",              VKD3D_SHADER_TYPE_PIXEL,    3, 0, 0, 0, false},
        {"ps_1_0",              VKD3D_SHADER_TYPE_PIXEL,    1, 0, 0, 0, false},
        {"ps_1_1",              VKD3D_SHADER_TYPE_PIXEL,    1, 1, 0, 0, false},
        {"ps_1_2",              VKD3D_SHADER_TYPE_PIXEL,    1, 2, 0, 0, false},
        {"ps_1_3",              VKD3D_SHADER_TYPE_PIXEL,    1, 3, 0, 0, false},
        {"ps_1_4",              VKD3D_SHADER_TYPE_PIXEL,    1, 4, 0, 0, false},
        {"ps_2_0",              VKD3D_SHADER_TYPE_PIXEL,    2, 0, 0, 0, false},
        {"ps_2_a",              VKD3D_SHADER_TYPE_PIXEL,    2, 1, 0, 0, false},
        {"ps_2_b",              VKD3D_SHADER_TYPE_PIXEL,    2, 2, 0, 0, false},
        {"ps_2_sw",             VKD3D_SHADER_TYPE_PIXEL,    2, 0, 0, 0, true},
        {"ps_3_0",              VKD3D_SHADER_TYPE_PIXEL,    3, 0, 0, 0, false},
        {"ps_3_sw",             VKD3D_SHADER_TYPE_PIXEL,    3, 0, 0, 0, true},
        {"ps_4_0",              VKD3D_SHADER_TYPE_PIXEL,    4, 0, 0, 0, false},
        {"ps_4_0_level_9_0",    VKD3D_SHADER_TYPE_PIXEL,    4, 0, 9, 0, false},
        {"ps_4_0_level_9_1",    VKD3D_SHADER_TYPE_PIXEL,    4, 0, 9, 1, false},
        {"ps_4_0_level_9_3",    VKD3D_SHADER_TYPE_PIXEL,    4, 0, 9, 3, false},
        {"ps_4_1",              VKD3D_SHADER_TYPE_PIXEL,    4, 1, 0, 0, false},
        {"ps_5_0",              VKD3D_SHADER_TYPE_PIXEL,    5, 0, 0, 0, false},
        {"ps_5_1",              VKD3D_SHADER_TYPE_PIXEL,    5, 1, 0, 0, false},
        {"tx_1_0",              VKD3D_SHADER_TYPE_TEXTURE,  1, 0, 0, 0, false},
        {"vs.1.0",              VKD3D_SHADER_TYPE_VERTEX,   1, 0, 0, 0, false},
        {"vs.1.1",              VKD3D_SHADER_TYPE_VERTEX,   1, 1, 0, 0, false},
        {"vs.2.0",              VKD3D_SHADER_TYPE_VERTEX,   2, 0, 0, 0, false},
        {"vs.2.a",              VKD3D_SHADER_TYPE_VERTEX,   2, 1, 0, 0, false},
        {"vs.2.sw",             VKD3D_SHADER_TYPE_VERTEX,   2, 0, 0, 0, true},
        {"vs.3.0",              VKD3D_SHADER_TYPE_VERTEX,   3, 0, 0, 0, false},
        {"vs.3.sw",             VKD3D_SHADER_TYPE_VERTEX,   3, 0, 0, 0, true},
        {"vs_1_0",              VKD3D_SHADER_TYPE_VERTEX,   1, 0, 0, 0, false},
        {"vs_1_1",              VKD3D_SHADER_TYPE_VERTEX,   1, 1, 0, 0, false},
        {"vs_2_0",              VKD3D_SHADER_TYPE_VERTEX,   2, 0, 0, 0, false},
        {"vs_2_a",              VKD3D_SHADER_TYPE_VERTEX,   2, 1, 0, 0, false},
        {"vs_2_sw",             VKD3D_SHADER_TYPE_VERTEX,   2, 0, 0, 0, true},
        {"vs_3_0",              VKD3D_SHADER_TYPE_VERTEX,   3, 0, 0, 0, false},
        {"vs_3_sw",             VKD3D_SHADER_TYPE_VERTEX,   3, 0, 0, 0, true},
        {"vs_4_0",              VKD3D_SHADER_TYPE_VERTEX,   4, 0, 0, 0, false},
        {"vs_4_0_level_9_0",    VKD3D_SHADER_TYPE_VERTEX,   4, 0, 9, 0, false},
        {"vs_4_0_level_9_1",    VKD3D_SHADER_TYPE_VERTEX,   4, 0, 9, 1, false},
        {"vs_4_0_level_9_3",    VKD3D_SHADER_TYPE_VERTEX,   4, 0, 9, 3, false},
        {"vs_4_1",              VKD3D_SHADER_TYPE_VERTEX,   4, 1, 0, 0, false},
        {"vs_5_0",              VKD3D_SHADER_TYPE_VERTEX,   5, 0, 0, 0, false},
        {"vs_5_1",              VKD3D_SHADER_TYPE_VERTEX,   5, 1, 0, 0, false},
    };

    for (i = 0; i < ARRAY_SIZE(profiles); ++i)
    {
        if (!strcmp(target, profiles[i].name))
            return &profiles[i];
    }

    return NULL;
}

static int compare_function_rb(const void *key, const struct rb_entry *entry)
{
    const char *name = key;
    const struct hlsl_ir_function *func = RB_ENTRY_VALUE(entry, const struct hlsl_ir_function,entry);

    return strcmp(name, func->name);
}

static void declare_predefined_types(struct hlsl_ctx *ctx)
{
    struct vkd3d_string_buffer *name;
    unsigned int x, y, bt, i, v;
    struct hlsl_type *type;

    static const char * const names[] =
    {
        [HLSL_TYPE_FLOAT]  = "float",
        [HLSL_TYPE_HALF]   = "half",
        [HLSL_TYPE_DOUBLE] = "double",
        [HLSL_TYPE_INT]    = "int",
        [HLSL_TYPE_UINT]   = "uint",
        [HLSL_TYPE_BOOL]   = "bool",
    };

    static const char *const variants_float[] = {"min10float", "min16float"};
    static const char *const variants_int[] = {"min12int", "min16int"};
    static const char *const variants_uint[] = {"min16uint"};

    static const char *const sampler_names[] =
    {
        [HLSL_SAMPLER_DIM_GENERIC]    = "sampler",
        [HLSL_SAMPLER_DIM_COMPARISON] = "SamplerComparisonState",
        [HLSL_SAMPLER_DIM_1D]         = "sampler1D",
        [HLSL_SAMPLER_DIM_2D]         = "sampler2D",
        [HLSL_SAMPLER_DIM_3D]         = "sampler3D",
        [HLSL_SAMPLER_DIM_CUBE]       = "samplerCUBE",
    };

    static const struct
    {
        char name[20];
        enum hlsl_type_class class;
        enum hlsl_base_type base_type;
        unsigned int dimx, dimy;
    }
    effect_types[] =
    {
        {"dword",           HLSL_CLASS_SCALAR, HLSL_TYPE_UINT,          1, 1},
        {"vector",          HLSL_CLASS_VECTOR, HLSL_TYPE_FLOAT,         4, 1},
        {"matrix",          HLSL_CLASS_MATRIX, HLSL_TYPE_FLOAT,         4, 4},
    };

    static const struct
    {
        const char *name;
        unsigned int version;
    }
    technique_types[] =
    {
        {"technique",    9},
        {"technique10", 10},
        {"technique11", 11},
    };

    if (!(name = hlsl_get_string_buffer(ctx)))
        return;

    for (bt = 0; bt <= HLSL_TYPE_LAST_SCALAR; ++bt)
    {
        for (y = 1; y <= 4; ++y)
        {
            for (x = 1; x <= 4; ++x)
            {
                vkd3d_string_buffer_clear(name);
                vkd3d_string_buffer_printf(name, "%s%ux%u", names[bt], y, x);
                type = hlsl_new_type(ctx, name->buffer, HLSL_CLASS_MATRIX, bt, x, y);
                hlsl_scope_add_type(ctx->globals, type);
                ctx->builtin_types.matrix[bt][x - 1][y - 1] = type;

                if (y == 1)
                {
                    vkd3d_string_buffer_clear(name);
                    vkd3d_string_buffer_printf(name, "%s%u", names[bt], x);
                    type = hlsl_new_type(ctx, name->buffer, HLSL_CLASS_VECTOR, bt, x, y);
                    hlsl_scope_add_type(ctx->globals, type);
                    ctx->builtin_types.vector[bt][x - 1] = type;

                    if (x == 1)
                    {
                        vkd3d_string_buffer_clear(name);
                        vkd3d_string_buffer_printf(name, "%s", names[bt]);
                        type = hlsl_new_type(ctx, name->buffer, HLSL_CLASS_SCALAR, bt, x, y);
                        hlsl_scope_add_type(ctx->globals, type);
                        ctx->builtin_types.scalar[bt] = type;
                    }
                }
            }
        }
    }

    for (bt = 0; bt <= HLSL_TYPE_LAST_SCALAR; ++bt)
    {
        const char *const *variants;
        unsigned int n_variants;

        switch (bt)
        {
            case HLSL_TYPE_FLOAT:
                variants = variants_float;
                n_variants = ARRAY_SIZE(variants_float);
                break;

            case HLSL_TYPE_INT:
                variants = variants_int;
                n_variants = ARRAY_SIZE(variants_int);
                break;

            case HLSL_TYPE_UINT:
                variants = variants_uint;
                n_variants = ARRAY_SIZE(variants_uint);
                break;

            default:
                n_variants = 0;
                variants = NULL;
                break;
        }

        for (v = 0; v < n_variants; ++v)
        {
            for (y = 1; y <= 4; ++y)
            {
                for (x = 1; x <= 4; ++x)
                {
                    vkd3d_string_buffer_clear(name);
                    vkd3d_string_buffer_printf(name, "%s%ux%u", variants[v], y, x);
                    type = hlsl_new_type(ctx, name->buffer, HLSL_CLASS_MATRIX, bt, x, y);
                    type->is_minimum_precision = 1;
                    hlsl_scope_add_type(ctx->globals, type);

                    if (y == 1)
                    {
                        vkd3d_string_buffer_clear(name);
                        vkd3d_string_buffer_printf(name, "%s%u", variants[v], x);
                        type = hlsl_new_type(ctx, name->buffer, HLSL_CLASS_VECTOR, bt, x, y);
                        type->is_minimum_precision = 1;
                        hlsl_scope_add_type(ctx->globals, type);

                        if (x == 1)
                        {
                            vkd3d_string_buffer_clear(name);
                            vkd3d_string_buffer_printf(name, "%s", variants[v]);
                            type = hlsl_new_type(ctx, name->buffer, HLSL_CLASS_SCALAR, bt, x, y);
                            type->is_minimum_precision = 1;
                            hlsl_scope_add_type(ctx->globals, type);
                        }
                    }
                }
            }
        }
    }

    for (bt = 0; bt <= HLSL_SAMPLER_DIM_LAST_SAMPLER; ++bt)
    {
        type = hlsl_new_simple_type(ctx, sampler_names[bt], HLSL_CLASS_SAMPLER);
        type->sampler_dim = bt;
        ctx->builtin_types.sampler[bt] = type;
    }

    ctx->builtin_types.Void = hlsl_new_simple_type(ctx, "void", HLSL_CLASS_VOID);
    ctx->builtin_types.null = hlsl_new_type(ctx, "NULL", HLSL_CLASS_NULL, HLSL_TYPE_UINT, 1, 1);
    ctx->builtin_types.string = hlsl_new_simple_type(ctx, "string", HLSL_CLASS_STRING);
    ctx->builtin_types.error = hlsl_new_simple_type(ctx, "<error type>", HLSL_CLASS_ERROR);
    hlsl_scope_add_type(ctx->globals, ctx->builtin_types.string);
    hlsl_scope_add_type(ctx->globals, hlsl_new_simple_type(ctx, "DepthStencilView", HLSL_CLASS_DEPTH_STENCIL_VIEW));
    hlsl_scope_add_type(ctx->globals, hlsl_new_simple_type(ctx, "DepthStencilState", HLSL_CLASS_DEPTH_STENCIL_STATE));
    hlsl_scope_add_type(ctx->globals, hlsl_new_simple_type(ctx, "fxgroup", HLSL_CLASS_EFFECT_GROUP));
    hlsl_scope_add_type(ctx->globals, hlsl_new_simple_type(ctx, "pass", HLSL_CLASS_PASS));
    hlsl_scope_add_type(ctx->globals, hlsl_new_simple_type(ctx, "pixelshader", HLSL_CLASS_PIXEL_SHADER));
    hlsl_scope_add_type(ctx->globals, hlsl_new_simple_type(ctx, "RasterizerState", HLSL_CLASS_RASTERIZER_STATE));
    hlsl_scope_add_type(ctx->globals, hlsl_new_simple_type(ctx, "RenderTargetView", HLSL_CLASS_RENDER_TARGET_VIEW));
    hlsl_scope_add_type(ctx->globals, hlsl_new_simple_type(ctx, "texture", HLSL_CLASS_TEXTURE));
    hlsl_scope_add_type(ctx->globals, hlsl_new_simple_type(ctx, "vertexshader", HLSL_CLASS_VERTEX_SHADER));
    hlsl_scope_add_type(ctx->globals, hlsl_new_simple_type(ctx, "ComputeShader", HLSL_CLASS_COMPUTE_SHADER));
    hlsl_scope_add_type(ctx->globals, hlsl_new_simple_type(ctx, "DomainShader", HLSL_CLASS_DOMAIN_SHADER));
    hlsl_scope_add_type(ctx->globals, hlsl_new_simple_type(ctx, "HullShader", HLSL_CLASS_HULL_SHADER));
    hlsl_scope_add_type(ctx->globals, hlsl_new_simple_type(ctx, "GeometryShader", HLSL_CLASS_GEOMETRY_SHADER));
    hlsl_scope_add_type(ctx->globals, hlsl_new_simple_type(ctx, "BlendState", HLSL_CLASS_BLEND_STATE));

    for (i = 0; i < ARRAY_SIZE(effect_types); ++i)
    {
        type = hlsl_new_type(ctx, effect_types[i].name, effect_types[i].class,
                effect_types[i].base_type, effect_types[i].dimx, effect_types[i].dimy);
        hlsl_scope_add_type(ctx->globals, type);
    }

    for (i = 0; i < ARRAY_SIZE(technique_types); ++i)
    {
        type = hlsl_new_simple_type(ctx, technique_types[i].name, HLSL_CLASS_TECHNIQUE);
        type->e.version = technique_types[i].version;
        hlsl_scope_add_type(ctx->globals, type);
    }

    hlsl_release_string_buffer(ctx, name);
}

static bool hlsl_ctx_init(struct hlsl_ctx *ctx, const struct vkd3d_shader_compile_info *compile_info,
        const struct hlsl_profile_info *profile, struct vkd3d_shader_message_context *message_context)
{
    unsigned int i;

    memset(ctx, 0, sizeof(*ctx));

    ctx->profile = profile;

    ctx->message_context = message_context;

    if (!(ctx->source_files = hlsl_alloc(ctx, sizeof(*ctx->source_files))))
        return false;
    if (!(ctx->source_files[0] = hlsl_strdup(ctx, compile_info->source_name ? compile_info->source_name : "<anonymous>")))
    {
        vkd3d_free(ctx->source_files);
        return false;
    }
    ctx->source_files_count = 1;
    ctx->location.source_name = ctx->source_files[0];
    ctx->location.line = ctx->location.column = 1;
    vkd3d_string_buffer_cache_init(&ctx->string_buffers);

    list_init(&ctx->scopes);

    if (!(ctx->dummy_scope = hlsl_new_scope(ctx, NULL)))
    {
        vkd3d_free((void *)ctx->source_files[0]);
        vkd3d_free(ctx->source_files);
        return false;
    }
    hlsl_push_scope(ctx);
    ctx->globals = ctx->cur_scope;

    list_init(&ctx->types);
    declare_predefined_types(ctx);

    rb_init(&ctx->functions, compare_function_rb);

    hlsl_block_init(&ctx->static_initializers);
    list_init(&ctx->extern_vars);

    list_init(&ctx->buffers);

    if (!(ctx->globals_buffer = hlsl_new_buffer(ctx, HLSL_BUFFER_CONSTANT,
            hlsl_strdup(ctx, "$Globals"), 0, NULL, NULL, &ctx->location)))
        return false;
    if (!(ctx->params_buffer = hlsl_new_buffer(ctx, HLSL_BUFFER_CONSTANT,
            hlsl_strdup(ctx, "$Params"), 0, NULL, NULL, &ctx->location)))
        return false;
    ctx->cur_buffer = ctx->globals_buffer;

    ctx->warn_implicit_truncation = true;

    for (i = 0; i < compile_info->option_count; ++i)
    {
        const struct vkd3d_shader_compile_option *option = &compile_info->options[i];

        switch (option->name)
        {
            case VKD3D_SHADER_COMPILE_OPTION_PACK_MATRIX_ORDER:
                if (option->value == VKD3D_SHADER_COMPILE_OPTION_PACK_MATRIX_ROW_MAJOR)
                    ctx->matrix_majority = HLSL_MODIFIER_ROW_MAJOR;
                else if (option->value == VKD3D_SHADER_COMPILE_OPTION_PACK_MATRIX_COLUMN_MAJOR)
                    ctx->matrix_majority = HLSL_MODIFIER_COLUMN_MAJOR;
                break;

            case VKD3D_SHADER_COMPILE_OPTION_BACKWARD_COMPATIBILITY:
                ctx->semantic_compat_mapping = option->value & VKD3D_SHADER_COMPILE_OPTION_BACKCOMPAT_MAP_SEMANTIC_NAMES;
                ctx->double_as_float_alias = option->value & VKD3D_SHADER_COMPILE_OPTION_DOUBLE_AS_FLOAT_ALIAS;
                break;

            case VKD3D_SHADER_COMPILE_OPTION_CHILD_EFFECT:
                ctx->child_effect = option->value;
                break;

            case VKD3D_SHADER_COMPILE_OPTION_WARN_IMPLICIT_TRUNCATION:
                ctx->warn_implicit_truncation = option->value;
                break;

            case VKD3D_SHADER_COMPILE_OPTION_INCLUDE_EMPTY_BUFFERS_IN_EFFECTS:
                ctx->include_empty_buffers = option->value;
                break;

            default:
                break;
        }
    }

    if (!(ctx->error_instr = hlsl_new_error_expr(ctx)))
        return false;
    hlsl_block_add_instr(&ctx->static_initializers, ctx->error_instr);

    ctx->domain = VKD3D_TESSELLATOR_DOMAIN_INVALID;
    ctx->output_control_point_count = UINT_MAX;
    ctx->output_primitive = 0;
    ctx->partitioning = 0;

    return true;
}

static void hlsl_ctx_cleanup(struct hlsl_ctx *ctx)
{
    struct hlsl_buffer *buffer, *next_buffer;
    struct hlsl_scope *scope, *next_scope;
    struct hlsl_ir_var *var, *next_var;
    struct hlsl_type *type, *next_type;
    unsigned int i;

    for (i = 0; i < ctx->source_files_count; ++i)
        vkd3d_free((void *)ctx->source_files[i]);
    vkd3d_free(ctx->source_files);
    vkd3d_string_buffer_cache_cleanup(&ctx->string_buffers);

    rb_destroy(&ctx->functions, free_function_rb, NULL);

    /* State blocks must be free before the variables, because they contain instructions that may
     * refer to them. */
    LIST_FOR_EACH_ENTRY_SAFE(scope, next_scope, &ctx->scopes, struct hlsl_scope, entry)
    {
        LIST_FOR_EACH_ENTRY_SAFE(var, next_var, &scope->vars, struct hlsl_ir_var, scope_entry)
        {
            for (i = 0; i < var->state_block_count; ++i)
                hlsl_free_state_block(var->state_blocks[i]);
            vkd3d_free(var->state_blocks);
            var->state_blocks = NULL;
            var->state_block_count = 0;
            var->state_block_capacity = 0;
        }
    }

    hlsl_block_cleanup(&ctx->static_initializers);

    LIST_FOR_EACH_ENTRY_SAFE(scope, next_scope, &ctx->scopes, struct hlsl_scope, entry)
    {
        LIST_FOR_EACH_ENTRY_SAFE(var, next_var, &scope->vars, struct hlsl_ir_var, scope_entry)
            hlsl_free_var(var);
        rb_destroy(&scope->types, NULL, NULL);
        vkd3d_free(scope);
    }

    LIST_FOR_EACH_ENTRY_SAFE(type, next_type, &ctx->types, struct hlsl_type, entry)
        hlsl_free_type(type);

    LIST_FOR_EACH_ENTRY_SAFE(buffer, next_buffer, &ctx->buffers, struct hlsl_buffer, entry)
    {
        vkd3d_free((void *)buffer->name);
        vkd3d_free(buffer);
    }

    vkd3d_free(ctx->constant_defs.regs);
}

int hlsl_compile_shader(const struct vkd3d_shader_code *hlsl, const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_code *out, struct vkd3d_shader_message_context *message_context)
{
    enum vkd3d_shader_target_type target_type = compile_info->target_type;
    const struct vkd3d_shader_hlsl_source_info *hlsl_source_info;
    struct hlsl_ir_function_decl *decl, *entry_func = NULL;
    const struct hlsl_profile_info *profile;
    struct hlsl_ir_function *func;
    const char *entry_point;
    struct hlsl_ctx ctx;
    int ret;

    if (!(hlsl_source_info = vkd3d_find_struct(compile_info->next, HLSL_SOURCE_INFO)))
    {
        ERR("No HLSL source info given.\n");
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    entry_point = hlsl_source_info->entry_point ? hlsl_source_info->entry_point : "main";

    if (!(profile = hlsl_get_target_info(hlsl_source_info->profile)))
    {
        FIXME("Unknown compilation target %s.\n", debugstr_a(hlsl_source_info->profile));
        return VKD3D_ERROR_NOT_IMPLEMENTED;
    }

    if (target_type != VKD3D_SHADER_TARGET_FX && profile->type == VKD3D_SHADER_TYPE_EFFECT)
    {
        vkd3d_shader_error(message_context, NULL, VKD3D_SHADER_ERROR_HLSL_INCOMPATIBLE_PROFILE,
                "The '%s' target profile is only compatible with the 'fx' target type.", profile->name);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    else if (target_type == VKD3D_SHADER_TARGET_D3D_BYTECODE && profile->major_version > 3)
    {
        vkd3d_shader_error(message_context, NULL, VKD3D_SHADER_ERROR_HLSL_INCOMPATIBLE_PROFILE,
                "The '%s' target profile is incompatible with the 'd3dbc' target type.", profile->name);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    else if (target_type == VKD3D_SHADER_TARGET_DXBC_TPF && profile->major_version < 4)
    {
        vkd3d_shader_error(message_context, NULL, VKD3D_SHADER_ERROR_HLSL_INCOMPATIBLE_PROFILE,
                "The '%s' target profile is incompatible with the 'dxbc-tpf' target type.", profile->name);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }
    else if (target_type == VKD3D_SHADER_TARGET_FX && profile->type != VKD3D_SHADER_TYPE_EFFECT)
    {
        vkd3d_shader_error(message_context, NULL, VKD3D_SHADER_ERROR_HLSL_INCOMPATIBLE_PROFILE,
                "The '%s' target profile is incompatible with the 'fx' target type.", profile->name);
        return VKD3D_ERROR_INVALID_ARGUMENT;
    }

    if (!hlsl_ctx_init(&ctx, compile_info, profile, message_context))
        return VKD3D_ERROR_OUT_OF_MEMORY;

    if ((ret = hlsl_lexer_compile(&ctx, hlsl)) == 2)
    {
        hlsl_ctx_cleanup(&ctx);
        return VKD3D_ERROR_OUT_OF_MEMORY;
    }

    if (ctx.result)
    {
        hlsl_ctx_cleanup(&ctx);
        return ctx.result;
    }

    /* If parsing failed without an error condition being recorded, we
     * plausibly hit some unimplemented feature. */
    if (ret)
    {
        hlsl_ctx_cleanup(&ctx);
        return VKD3D_ERROR_NOT_IMPLEMENTED;
    }

    if (ctx.profile->type == VKD3D_SHADER_TYPE_EFFECT)
    {
        ret = hlsl_emit_effect_binary(&ctx, out);

        hlsl_ctx_cleanup(&ctx);
        return ret;
    }

    if ((func = hlsl_get_function(&ctx, entry_point)))
    {
        LIST_FOR_EACH_ENTRY(decl, &func->overloads, struct hlsl_ir_function_decl, entry)
        {
            if (!decl->has_body)
                continue;
            if (entry_func)
            {
                /* Depending on d3dcompiler version, either the first or last is
                 * selected. */
                hlsl_fixme(&ctx, &decl->loc, "Multiple valid entry point definitions.");
            }
            entry_func = decl;
        }
    }

    if (!entry_func)
    {
        const struct vkd3d_shader_location loc = {.source_name = compile_info->source_name};

        hlsl_error(&ctx, &loc, VKD3D_SHADER_ERROR_HLSL_NOT_DEFINED,
                "Entry point \"%s\" is not defined.", entry_point);
        hlsl_ctx_cleanup(&ctx);
        return VKD3D_ERROR_INVALID_SHADER;
    }

    if (target_type == VKD3D_SHADER_TARGET_SPIRV_BINARY
            || target_type == VKD3D_SHADER_TARGET_SPIRV_TEXT
            || target_type == VKD3D_SHADER_TARGET_D3D_ASM)
    {
        uint64_t config_flags = vkd3d_shader_init_config_flags();
        struct vkd3d_shader_compile_info info = *compile_info;
        struct vsir_program program;

        if (profile->major_version < 4)
        {
            if ((ret = hlsl_emit_bytecode(&ctx, entry_func, VKD3D_SHADER_TARGET_D3D_BYTECODE, &info.source)) < 0)
                goto done;
            info.source_type = VKD3D_SHADER_SOURCE_D3D_BYTECODE;
            ret = d3dbc_parse(&info, config_flags, message_context, &program);
        }
        else
        {
            if ((ret = hlsl_emit_bytecode(&ctx, entry_func, VKD3D_SHADER_TARGET_DXBC_TPF, &info.source)) < 0)
                goto done;
            info.source_type = VKD3D_SHADER_SOURCE_DXBC_TPF;
            ret = tpf_parse(&info, config_flags, message_context, &program);
        }
        if (ret >= 0)
        {
            ret = vsir_program_compile(&program, config_flags, &info, out, message_context);
            vsir_program_cleanup(&program);
        }
        vkd3d_shader_free_shader_code(&info.source);
    }
    else
    {
        ret = hlsl_emit_bytecode(&ctx, entry_func, target_type, out);
    }

done:
    hlsl_ctx_cleanup(&ctx);
    return ret;
}

struct hlsl_ir_function_decl *hlsl_compile_internal_function(struct hlsl_ctx *ctx, const char *name, const char *hlsl)
{
    const struct hlsl_ir_function_decl *saved_cur_function = ctx->cur_function;
    struct vkd3d_shader_code code = {.code = hlsl, .size = strlen(hlsl)};
    const char *saved_internal_func_name = ctx->internal_func_name;
    struct vkd3d_string_buffer *internal_name;
    struct hlsl_ir_function_decl *func;
    void *saved_scanner = ctx->scanner;
    int ret;

    TRACE("name %s, hlsl %s.\n", debugstr_a(name), debugstr_a(hlsl));

    /* The actual name of the function is mangled with a unique prefix, both to
     * allow defining multiple variants of a function with the same name, and to
     * avoid polluting the user name space. */

    if (!(internal_name = hlsl_get_string_buffer(ctx)))
        return NULL;
    vkd3d_string_buffer_printf(internal_name, "<%s-%u>", name, ctx->internal_name_counter++);

    /* Save and restore everything that matters.
     * Note that saving the scope stack is hard, and shouldn't be necessary. */

    hlsl_push_scope(ctx);
    ctx->scanner = NULL;
    ctx->internal_func_name = internal_name->buffer;
    ctx->cur_function = NULL;
    ret = hlsl_lexer_compile(ctx, &code);
    ctx->scanner = saved_scanner;
    ctx->internal_func_name = saved_internal_func_name;
    ctx->cur_function = saved_cur_function;
    hlsl_pop_scope(ctx);
    if (ret)
    {
        ERR("Failed to compile intrinsic, error %u.\n", ret);
        hlsl_release_string_buffer(ctx, internal_name);
        return NULL;
    }
    func = hlsl_get_first_func_decl(ctx, internal_name->buffer);
    hlsl_release_string_buffer(ctx, internal_name);
    return func;
}
